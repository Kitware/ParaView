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
#include "vtkPythonView.h"

#include "vtkObjectFactory.h"

#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformationRequestKey.h"
#include "vtkMatplotlibUtilities.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPythonView);

//----------------------------------------------------------------------------
vtkPythonView::vtkPythonView()
{
  this->RenderTexture = vtkSmartPointer<vtkTexture>::New();
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->Renderer->SetBackgroundTexture(this->RenderTexture);
  this->RenderWindow.TakeReference(this->SynchronizedWindows->NewRenderWindow());
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Magnification = 1;
  this->MatplotlibUtilities = vtkSmartPointer<vtkMatplotlibUtilities>::New();

  this->Script = NULL;
}

//----------------------------------------------------------------------------
vtkPythonView::~vtkPythonView()
{
  this->SetScript(NULL);
}

//----------------------------------------------------------------------------
vtkInformationKeyMacro(vtkPythonView, REQUEST_DELIVER_DATA_TO_CLIENT, Request);

//----------------------------------------------------------------------------
void vtkPythonView::Update()
{
  vtkTimerLog::MarkStartEvent("vtkPythonView::Update");

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
                 << "from paraview import python_view as pv" << endl
                 << "from vtkPVClientServerCoreRenderingPython import vtkPythonView" << endl
                 << "pythonView  = vtkPythonView('" << addressOfThis << " ')" << endl;
    vtkPythonInterpreter::RunSimpleString(importStream.str().c_str());

    // Evaluate the user-defined script. It should define two functions,
    // setup_data(view) and render(view, figure) that each take a
    // vtkPythonView (the render function also takes a matplotlib.figure
    // as the second argument).  If these functions are not defined in
    // this script, they must be defined in the global Python
    // interpreter by some other means (e.g. a script executed by
    // pvpython).
    vtkPythonInterpreter::RunSimpleString(this->Script);

    // Update the data array settings. Do this only on servers where local data is available
    if (this->IsLocalDataAvailable())
      {
      vtksys_ios::ostringstream setupDataCommandStream;
      setupDataCommandStream << "setup_data_available = False\n"
                             << "try:\n"
                             << "  setup_data\n"
                             << "  setup_data_available = True\n"
                             << "except:\n"
                             << "  print 'No setup_data(pythonView) function defined'\n"
                             << "if setup_data_available:\n"
                             << "  setup_data(pythonView)\n";
      vtkPythonInterpreter::RunSimpleString(setupDataCommandStream.str().c_str());
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
    vtkImageData* imageData = this->GenerateImage();

    if (imageData)
      {
      this->RenderTexture->SetInputData(imageData);
      imageData->Delete();
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
vtkImageData* vtkPythonView::GenerateImage()
{
  if (!this->MatplotlibUtilities)
    {
    vtkErrorMacro(<< "matplotlib is not available. Python views will not work.");
    return NULL;
    }

  // Now draw the image
  int width  = this->Size[0] * this->Magnification;
  int height = this->Size[1] * this->Magnification;

  if (!this->Script || strlen(this->Script) < 1)
    {
    return this->MatplotlibUtilities->
      ImageFromScript("", "pythonViewCanvas", width, height);
    }

  vtksys_ios::ostringstream renderCommandStream;
  renderCommandStream << "render_available = False\n"
                      << "try:\n"
                      << "  render\n"
                      << "  render_available = True\n"
                      << "except NameError:\n"
                      << "  print 'No render(pythonView,figure) function defined'\n"
                      << "if render_available:\n"
                      << "  render(pythonView, pythonViewCanvasFigure)\n";
  vtkImageData* imageData = this->MatplotlibUtilities->
    ImageFromScript(renderCommandStream.str().c_str(), "pythonViewCanvas", width, height);

  return imageData;
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

  os << indent << "RenderTexture: " << this->RenderTexture << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "Magnification: " << this->Magnification << endl;
  os << indent << "Script: \n" << this->Script << endl;
}
