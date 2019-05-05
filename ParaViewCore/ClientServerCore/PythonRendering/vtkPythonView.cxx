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

#include "vtkPythonUtil.h"
#include "vtkPythonView.h"

#include "vtkObjectFactory.h"

#include "vtkDataObject.h"
#include "vtkInformationRequestKey.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPyObject.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"

#include <algorithm>
#include <sstream>

class vtkPythonView::vtkInternals
{
  bool Initialized;

  vtkSmartPyObject WrappingModule;
  vtkSmartPyObject PythonViewModule;
  vtkSmartPyObject ScriptModule;
  std::string ScriptCode;

  bool InitializePython()
  {
    if (!this->Initialized)
    {
      this->Initialized = true;
      vtkPythonInterpreter::Initialize();
      vtkPythonScopeGilEnsurer gilEnsurer;

      // import the wrapping module.
      this->WrappingModule.TakeReference(
        PyImport_ImportModule("paraview.modules.vtkPVClientServerCorePythonRendering"));
      if (!this->WrappingModule)
      {
        vtkGenericWarningMacro("Failed to import `vtkPVClientServerCorePythonRendering`.");
        if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
          return false;
        }
      }

      this->PythonViewModule.TakeReference(PyImport_ImportModule("paraview.python_view"));
      if (!this->PythonViewModule)
      {
        vtkGenericWarningMacro("Failed to import 'paraview.python_view' module.");
        if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
          return false;
        }
      }
    }
    return this->PythonViewModule;
  }

  /**
   * Compile and build a Python module object from the given code.
   */
  vtkSmartPyObject BuildModule(const std::string& code, const std::string& fname = "Script")
  {
    if (!this->InitializePython() || code.empty())
    {
      return vtkSmartPyObject();
    }

    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject codeObj(Py_CompileString(code.c_str(), fname.c_str(), Py_file_input));
    if (!codeObj)
    {
      PyErr_Print();
      PyErr_Clear();
      return vtkSmartPyObject();
    }
    vtkSmartPyObject module(PyImport_ExecCodeModule(const_cast<char*>("vtkPythonView"), codeObj));
    return module;
  }

public:
  vtkInternals()
    : Initialized(false)
  {
  }
  ~vtkInternals() {}

  bool Prepare(const std::string& script)
  {
    if (script != this->ScriptCode)
    {
      this->ScriptCode = script;
      this->ScriptModule = this->BuildModule(this->ScriptCode);
    }
    return this->ScriptModule;
  }

  bool CallSetupData(vtkPythonView* self)
  {
    if (!this->ScriptModule)
    {
      return false;
    }

    if (PyObject_HasAttrString(this->ScriptModule, "setup_data") != 1)
    {
      // not having `setup_data` defined in the script is acceptable.
      return true;
    }

    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject methodName(PyString_FromString("setup_data"));
    vtkSmartPyObject view(vtkPythonUtil::GetObjectFromPointer(self));
    vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(
      this->ScriptModule, methodName.GetPointer(), view.GetPointer(), NULL));
    return retVal;
  }

  bool CallRender(vtkPythonView* self, int width, int height)
  {
    if (!this->ScriptModule)
    {
      return false;
    }

    if (PyObject_HasAttrString(this->ScriptModule, "render") != 1)
    {
      // not having `render` defined in the script is acceptable.
      return true;
    }

    vtkSmartPyObject renderFunction(PyObject_GetAttrString(this->ScriptModule, "render"));
    assert(renderFunction);

    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject methodName(PyString_FromString("call_render"));
    vtkSmartPyObject view(vtkPythonUtil::GetObjectFromPointer(self));
    vtkSmartPyObject widthObj(PyInt_FromLong(width));
    vtkSmartPyObject heightObj(PyInt_FromLong(height));
    vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(this->PythonViewModule,
      methodName.GetPointer(), renderFunction.GetPointer(), view.GetPointer(),
      widthObj.GetPointer(), heightObj.GetPointer(), NULL));
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return false;
    }

    return true;
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
  this->GetRenderWindow()->AddRenderer(this->Renderer);
  this->Magnification[0] = this->Magnification[1] = 1;
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
  if (!this->Internals->Prepare(this->Script ? this->Script : ""))
  {
    return;
  }

  this->Superclass::Update();

  vtkTimerLog::MarkStartEvent("vtkPythonView::Update");
  // Call 'setup_data' on ranks where data is available for "transformation".
  if (this->IsLocalDataAvailable())
  {
    this->Internals->CallSetupData(this);
  }
  this->CallProcessViewRequest(vtkPythonView::REQUEST_DELIVER_DATA_TO_CLIENT(),
    this->RequestInformation, this->ReplyInformationVector);
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
  auto window = this->GetRenderWindow();
  vtkRendererCollection* rens = window->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  rens->InitTraversal(cookie);
  while (vtkRenderer* ren = rens->GetNextRenderer(cookie))
  {
    ren->SetRenderWindow(NULL);
    window->RemoveRenderer(ren);
  }

  window->AddRenderer(renderer);
  this->Renderer = renderer;
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
  vtkPythonRepresentation* representation = this->GetVisibleRepresentation(visibleObjectIndex);
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
  vtkPythonRepresentation* representation = this->GetVisibleRepresentation(visibleObjectIndex);
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
  vtkPythonRepresentation* representation = this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
  {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return 0;
  }

  return representation->GetNumberOfAttributeArrays(attributeType);
}

//----------------------------------------------------------------------------
const char* vtkPythonView::GetAttributeArrayName(
  int visibleObjectIndex, int attributeType, int arrayIndex)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation = this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
  {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return NULL;
  }

  return representation->GetAttributeArrayName(attributeType, arrayIndex);
}

//----------------------------------------------------------------------------
void vtkPythonView::SetAttributeArrayStatus(
  int visibleObjectIndex, int attributeType, const char* name, int status)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation = this->GetVisibleRepresentation(visibleObjectIndex);
  if (!representation)
  {
    vtkErrorMacro(<< "No visible representation at index " << visibleObjectIndex);
    return;
  }

  representation->SetAttributeArrayStatus(attributeType, name, status);
}

//----------------------------------------------------------------------------
int vtkPythonView::GetAttributeArrayStatus(
  int visibleObjectIndex, int attributeType, const char* name)
{
  // Forward to the visible representation
  vtkPythonRepresentation* representation = this->GetVisibleRepresentation(visibleObjectIndex);
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
  this->SetImageData(NULL);

  // Now draw the image
  int width = this->Size[0] * this->Magnification[0];
  int height = this->Size[1] * this->Magnification[1];

  this->Internals->CallRender(this, width, height);

  // this->ImageData should be set by the call_render() function invoked above.
  if (this->ImageData)
  {
    this->RenderTexture->SetInputData(this->ImageData);
    this->Renderer->TexturedBackgroundOn();
  }
  else
  {
    this->Renderer->TexturedBackgroundOff();
  }
  this->GetRenderWindow()->Render();
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
  os << indent << "Magnification: " << this->Magnification[0] << ", " << this->Magnification[1]
     << endl;
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
