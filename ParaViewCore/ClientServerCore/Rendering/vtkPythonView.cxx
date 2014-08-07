/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be the first thing that's included
#include "vtkPythonView.h"

#include "vtkObjectFactory.h"

#include "vtkDataObject.h"
#include "vtkInformationRequestKey.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPythonInterpreter.h"
#include "vtkSmartPyObject.h"
#include "vtkPythonRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"

#include <algorithm>
#include <vtksys/ios/sstream>


class vtkPythonView::vtkInternals
{
  PyObject* CustomLocals;
public:
  vtkInternals() : CustomLocals(0) {}
  ~vtkInternals()
    {
    this->CleanupObjects();
    }

  PyObject* GetCustomLocalsPyObject()
    {
    if (this->CustomLocals)
      {
      return this->CustomLocals;
      }

    // Make sure the python interpreter is initialized
    vtkPythonInterpreter::Initialize(1);

    const char* code = "__vtkPythonViewLocals={'__builtins__':__builtins__}\n";
    PyRun_SimpleString(const_cast<char *>(code));

    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);
    this->CustomLocals = PyDict_GetItemString(global_dict, "__vtkPythonViewLocals");
    if (!this->CustomLocals)
      {
        vtkGenericWarningMacro("Failed to locate the __vtkPythonViewLocals object.");
        return NULL;
      }
    Py_INCREF(this->CustomLocals);

    PyRun_SimpleString(const_cast<char*>("del __vtkPythonViewLocals"));

    return this->CustomLocals;
    }

  void CleanupObjects()
    {
    Py_XDECREF(this->CustomLocals);
    this->CustomLocals = NULL;
    if (vtkPythonInterpreter::IsInitialized())
      {
      const char* code = "import gc; gc.collect()\n";
      vtkPythonInterpreter::RunSimpleString(code);
      }
    }

  void ResetCustomLocals()
  {
    this->CleanupObjects();
  }

};

vtkStandardNewMacro(vtkPythonView);

//----------------------------------------------------------------------------
vtkPythonView::vtkPythonView()
{
  this->Internals = new vtkPythonView::vtkInternals();
  this->RenderTexture = vtkSmartPointer<vtkTexture>::New();
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->Renderer->SetBackgroundTexture(this->RenderTexture);
  this->RenderWindow.TakeReference(this->SynchronizedWindows->NewRenderWindow());
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Magnification = 1;
  this->ImageData = NULL;

  this->Script = NULL;
}

//----------------------------------------------------------------------------
vtkPythonView::~vtkPythonView()
{
  // Clean up memory
  delete this->Internals;
  this->SetScript(NULL);
  this->SetImageData(NULL);
}

//----------------------------------------------------------------------------
vtkInformationKeyMacro(vtkPythonView, REQUEST_DELIVER_DATA_TO_CLIENT, Request);

//----------------------------------------------------------------------------
void vtkPythonView::Update()
{
  vtkTimerLog::MarkStartEvent("vtkPythonView::Update");

  this->Internals->ResetCustomLocals();

  if (this->Script && strlen(this->Script) > 0)
    {

    this->CallProcessViewRequest(vtkPVView::REQUEST_UPDATE(),
                                 this->RequestInformation,
                                 this->ReplyInformationVector);

    // Define the view in Python by creating a new instance of the
    // Python vtkPythonView class from the pointer to the C++
    // vtkPythonView instance.
    char addressOfThis[1024];
    sprintf(addressOfThis, "%p", this);
    char *address = addressOfThis;
    if ((addressOfThis[0] == '0') &&
        ((addressOfThis[1] == 'x') || (addressOfThis[1] == 'X')))
      {
      address += 2;
      }

    // Import necessary items from ParaView
    vtksys_ios::ostringstream importStream;
    importStream << "import paraview" << endl
                 << "from vtkPVClientServerCoreRenderingPython import vtkPythonView" << endl
                 << "pythonView = vtkPythonView('" << addressOfThis << " ')" << endl;
    this->RunSimpleStringWithCustomLocals(importStream.str().c_str());

    // Evaluate the user-defined script. It should define two functions,
    // setup_data(view) and render(view, figure) that each take a
    // vtkPythonView (the render function also takes a matplotlib.figure
    // as the second argument).  If these functions are not defined in
    // this script, they must be defined in the global Python
    // interpreter by some other means (e.g. a script executed by
    // pvpython).
    this->RunSimpleStringWithCustomLocals(this->Script);

    // Update the data array settings. Do this only on servers where local data is available
    if (this->IsLocalDataAvailable())
      {
      vtksys_ios::ostringstream setupDataCommandStream;
      setupDataCommandStream
        << "from paraview import python_view\n"
        << "try:\n"
        << "  python_view.call_setup_data(setup_data, pythonView)\n"
        << "except:\n"
        << "  pass\n";
      this->RunSimpleStringWithCustomLocals(setupDataCommandStream.str().c_str());
      }

    this->CallProcessViewRequest(vtkPythonView::REQUEST_DELIVER_DATA_TO_CLIENT(),
                                 this->RequestInformation,
                                 this->ReplyInformationVector);
    }

  vtkTimerLog::MarkEndEvent("vtkPythonView::Update");
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPythonView::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
void vtkPythonView::SetRenderer(vtkRenderer* renderer)
{
  vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  rens->InitTraversal(cookie);
  while(vtkRenderer *ren = rens->GetNextRenderer(cookie))
    {
    ren->SetRenderWindow(NULL);
    this->RenderWindow->RemoveRenderer(ren);
    }

  this->RenderWindow->AddRenderer(renderer);
  this->Renderer = renderer;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPythonView::GetRenderWindow()
{
  return this->RenderWindow;
}

//----------------------------------------------------------------------------
void vtkPythonView::SetRenderWindow(vtkRenderWindow * renWin)
{
  if (!renWin)
    {
    vtkErrorMacro(<< "SetRenderWindow called with a null window pointer."
                  << " That can't be right.");
    return;
    }

  // move renderers to new window
  vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  while(rens->GetNumberOfItems())
    {
    vtkRenderer* ren = rens->GetFirstRenderer();
    ren->SetRenderWindow(NULL);
    renWin->AddRenderer(ren);
    this->RenderWindow->RemoveRenderer(ren);
    }

}

//----------------------------------------------------------------------------
int vtkPythonView::GetNumberOfVisibleDataObjects()
{
  int numberOfVisibleRepresentations = 0;
  int numberOfRepresentations = this->GetNumberOfRepresentations();
  for (int i = 0; i < numberOfRepresentations; ++i)
    {
    vtkPVDataRepresentation* representation =
      vtkPVDataRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (representation && representation->GetVisibility())
      {
      numberOfVisibleRepresentations++;
      }
    }

  return numberOfVisibleRepresentations;
}

//----------------------------------------------------------------------------
vtkPythonRepresentation* vtkPythonView::GetVisibleRepresentation(int visibleObjectIndex)
{
  if (visibleObjectIndex < 0 || visibleObjectIndex >= this->GetNumberOfVisibleDataObjects())
    {
    return NULL;
    }

  int numberOfVisibleRepresentations = 0;
  int numberOfRepresentations = this->GetNumberOfRepresentations();
  for (int i = 0; i < numberOfRepresentations; ++i)
    {
    vtkPythonRepresentation* representation =
      vtkPythonRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (representation && representation->GetVisibility())
      {
      if (visibleObjectIndex == numberOfVisibleRepresentations)
        {
        return representation;
        }
      numberOfVisibleRepresentations++;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPythonView::GetVisibleDataObjectForSetup(int visibleObjectIndex)
{
  vtkPythonRepresentation* representation =
    this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
    {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return NULL;
    }

  return representation->GetLocalInput();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPythonView::GetVisibleDataObjectForRendering(int visibleObjectIndex)
{
  vtkPythonRepresentation* representation =
    this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
    {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return NULL;
    }

  return representation->GetClientDataObject();
}

//----------------------------------------------------------------------------
int vtkPythonView::GetNumberOfAttributeArrays(int visibleObjectIndex, int attributeType)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation =
    this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
    {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return 0;
    }

  return representation->GetNumberOfAttributeArrays(attributeType);
}

//----------------------------------------------------------------------------
const char* vtkPythonView::GetAttributeArrayName(int visibleObjectIndex,
                                                 int attributeType,
                                                 int arrayIndex)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation =
    this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
    {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return NULL;
    }

  return representation->GetAttributeArrayName(attributeType, arrayIndex);
}

//----------------------------------------------------------------------------
void vtkPythonView::SetAttributeArrayStatus(int visibleObjectIndex,
                                            int attributeType,
                                            const char* name,
                                            int status)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation =
    this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
    {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return;
    }

  representation->SetAttributeArrayStatus(attributeType, name, status);
}

//----------------------------------------------------------------------------
int vtkPythonView::GetAttributeArrayStatus(int visibleObjectIndex,
                                           int attributeType,
                                           const char* name)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation =
    this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
    {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return 0;
    }

  return representation->GetAttributeArrayStatus(attributeType, name);
}

//----------------------------------------------------------------------------
void vtkPythonView::EnableAllAttributeArrays()
{
  int numRepresentations = this->GetNumberOfRepresentations();
  for (int i = 0; i < numRepresentations; ++i)
    {
    vtkPythonRepresentation* representation =
      vtkPythonRepresentation::SafeDownCast(this->GetRepresentation(i));    
    if (representation)
      {
      representation->EnableAllAttributeArrays();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPythonView::DisableAllAttributeArrays()
{
  int numRepresentations = this->GetNumberOfRepresentations();
  for (int i = 0; i < numRepresentations; ++i)
    {
    vtkPythonRepresentation* representation =
      vtkPythonRepresentation::SafeDownCast(this->GetRepresentation(i));    
    if (representation)
      {
      representation->DisableAllAttributeArrays();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPythonView::StillRender()
{
  // Render only on the client
  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    this->SetImageData(NULL);

    // Now draw the image
    int width  = this->Size[0] * this->Magnification;
    int height = this->Size[1] * this->Magnification;

    vtksys_ios::ostringstream renderCommandStream;
    renderCommandStream
      << "from paraview import python_view\n"
      << "try:\n"
      << "  python_view.call_render(render, pythonView, " << width << ", " << height << ")\n"
      << "except:\n"
      << "  pass\n";
    this->RunSimpleStringWithCustomLocals(renderCommandStream.str().c_str());

    // this->ImageData should be set by the call_render() function
    // invoked above.
    if (this->ImageData)
      {
      this->RenderTexture->SetInputData(this->ImageData);
      this->Renderer->TexturedBackgroundOn();
      }
    else
      {
      this->Renderer->TexturedBackgroundOff();
      }
    this->RenderWindow->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPythonView::InteractiveRender()
{
  this->StillRender();
}

//----------------------------------------------------------------------------
bool vtkPythonView::IsLocalDataAvailable()
{
  // Check with the representations to see if they have local
  // input.
  bool available = false;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkPythonRepresentation* representation =
      vtkPythonRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (!representation)
      {
      vtkErrorMacro(<< "Should only have vtkPythonRepresentations");
      continue;
      }

    if (representation->GetLocalInput())
      {
      available = true;
      break;
      }
    }

  return available;
}

//----------------------------------------------------------------------------
int vtkPythonView::RunSimpleStringWithCustomLocals(const char* code)
{
  // The embedded python interpreter cannot handle DOS line-endings, see
  // http://sourceforge.net/tracker/?group_id=5470&atid=105470&func=detail&aid=1167922
  std::string buffer = code ? code : "";
  buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());

  PyObject* context = this->Internals->GetCustomLocalsPyObject();
  vtkSmartPyObject result(PyRun_String(const_cast<char*>(buffer.c_str()),
                                       Py_file_input, context, context));

  if (result)
    {
    PyErr_Print();
    return -1;
    }

  result = NULL;
  if (Py_FlushLine())
    {
    PyErr_Clear();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderTexture: ";
  if (this->RenderTexture)
    {
    os << endl;
    this->RenderTexture->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Renderer: ";
  if (this->Renderer)
    {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RenderWindow: ";
  if (this->RenderWindow)
    {
    os << endl;
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Magnification: " << this->Magnification << endl;
  os << indent << "Script: \n" << this->Script << endl;
  os << indent << "ImageData: ";
  if (this->ImageData)
    {
    os << endl;
    this->ImageData->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
