/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayWindowProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplayWindowProxy.h"

#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMDisplayerProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSMDisplayerProxy.h"
#include "vtkPVRenderModule.h"
#include "vtkSMPropertyIterator.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMDisplayWindowProxy);
vtkCxxRevisionMacro(vtkSMDisplayWindowProxy, "1.19");
vtkCxxSetObjectMacro(vtkSMDisplayWindowProxy,RenderModule,vtkPVRenderModule);

struct vtkSMDisplayWindowProxyInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > DisplayVectorType;
  DisplayVectorType Displayers;
};

//---------------------------------------------------------------------------
vtkSMDisplayWindowProxy::vtkSMDisplayWindowProxy()
{
  this->SetVTKClassName("vtkRenderWindow");

  // TODO revise this: where should windowtoimage be?
  this->WindowToImage = vtkSMProxy::New();
  this->WindowToImage->SetServers(vtkProcessModule::CLIENT);
  
  this->DWInternals = new vtkSMDisplayWindowProxyInternals;
  this->RenderModule = 0;
}

//---------------------------------------------------------------------------
vtkSMDisplayWindowProxy::~vtkSMDisplayWindowProxy()
{
  this->SetRenderModule(0);
  this->WindowToImage->Delete();
  delete this->DWInternals;
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream str;
  int i;

  // TODO revise
  // These are good defaults for batch scripting but should
  // be made more general for other uses.
#ifndef _WIN32
  for (i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(i) << "DoubleBufferOff"
      << vtkClientServerStream::End;
    }
#endif

  vtkSMProxy* rendererProxy = this->GetSubProxy("renderer");
  if (!rendererProxy)
    {
    vtkErrorMacro("No renderer sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << this->GetID(i) << "AddRenderer" << rendererProxy->GetID(i)
          << vtkClientServerStream::End;
      }
    }

  vtkSMProxy* cameraProxy = this->GetSubProxy("camera");
  if (!cameraProxy)
    {
    vtkErrorMacro("No camera sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << rendererProxy->GetID(i) 
          << "SetActiveCamera" 
          << cameraProxy->GetID(i)
          << vtkClientServerStream::End;
      }
    }

  vtkSMProxy* interactorProxy = this->GetSubProxy("interactor");
  if (!interactorProxy)
    {
    vtkErrorMacro("No interactor sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    //Set the RenderWindow on the interactor
    for(i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke
          << interactorProxy->GetID(i)
          << "SetRenderWindow"
          << this->GetID(i)
          << vtkClientServerStream::End;
      }
    }
  vtkSMProxy* compositeProxy = this->GetSubProxy("composite");
  if (!compositeProxy)
    {
    vtkErrorMacro("No composite sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << compositeProxy->GetID(i) 
          << "SetRenderWindow" 
          << this->GetID(i)
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << compositeProxy->GetID(i) 
          << "SetUseCompositing" << 1
          << vtkClientServerStream::End;
      if (compositeProxy->GetVTKClassName() &&
          strcmp(compositeProxy->GetVTKClassName(), "vtkPVTreeComposite")==0)
        {
        str << vtkClientServerStream::Invoke 
            << compositeProxy->GetID(i) 
            << "SetEnableAbort" << 0
            << vtkClientServerStream::End;
        }
      str << vtkClientServerStream::Invoke 
          << compositeProxy->GetID(i) 
          << "InitializeRMIs" 
          << vtkClientServerStream::End;
      }
    }

  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, str, 0);
    }

  str.Reset();

  this->WindowToImage->SetVTKClassName("vtkWindowToImageFilter");
  this->WindowToImage->CreateVTKObjects(1);
  
  if (numObjects > 0)
    {
    str << vtkClientServerStream::Invoke 
        << this->WindowToImage->GetID(0) 
        << "SetInput" 
        << this->GetID(0)
        << vtkClientServerStream::End;
    }
  

  pm->SendStream(this->WindowToImage->Servers, str, 0);
  
  if(this->RenderModule && this->GetSubProxy("interactor") && numObjects > 0)
    { 
    //set interactor on the render module
    this->RenderModule->SetInteractorID(
      this->GetSubProxy("interactor")->GetID(0));
    }
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::SaveState(const char* name, ostream* file, vtkIndent indent)
{
  //Update the Camera positions and call superclass method.

  vtkProcessModule *pm =vtkProcessModule::GetProcessModule();
  vtkSMProxy* cameraProxy = this->GetSubProxy("camera");
  vtkCamera* camera = (cameraProxy)? vtkCamera::SafeDownCast(
      pm->GetObjectFromID(cameraProxy->GetID(0))) : 0;
  if (camera)
    {
    double position[3], focalpoint[3], viewup[3];
    double angle, clip_range[2];
    vtkSMDoubleVectorProperty *dvp;
    camera->GetPosition(position);
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("CameraPosition"));
    if (dvp)
      {
      dvp->SetElements(position);
      }
      
    camera->GetFocalPoint(focalpoint);
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("CameraFocalPoint"));
    if (dvp)
      {
      dvp->SetElements(focalpoint);
      }
    
    camera->GetViewUp(viewup);
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("CameraViewUp"));
    if (dvp)
      {
      dvp->SetElements(viewup);
      }
    
    camera->GetClippingRange(clip_range);
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("CameraClippingRange"));
    if (dvp)
      {
      dvp->SetElements(clip_range);
      }
    
    angle = camera->GetViewAngle();
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("CameraViewAngle"));
    if (dvp)
      {
      dvp->SetElements1(angle);
      }
    }
  else
    {
    vtkErrorMacro("No camera. Cannot save camera state correctly");
    }
  this->Superclass::SaveState(name,file,indent);
}
//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::UpdateSelfAndAllInputs()
{
  //Updates self before the inputs
  this->CreateVTKObjects(1);
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (this->GetSubProxy("interactor"))
    {
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(
      pm->GetObjectFromID(this->GetSubProxy("interactor")->GetID(0)));
    if (iren)
      {
      vtkInteractorStyleTrackballCamera *istyle = vtkInteractorStyleTrackballCamera::New();
      iren->SetInteractorStyle(istyle);
      istyle->Delete();
      }
    }
  else
    {
    vtkErrorMacro("No interactor in configuration. Cannot set interactor style");
    }
  this->Superclass::UpdateSelfAndAllInputs();
}
//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::TileWindows(int xsize, int ysize, int nColumns)
{
  vtkClientServerStream str;

  vtkSMProxy* compositeProxy = this->GetSubProxy("composite");
  if (!compositeProxy)
    {
    vtkErrorMacro("No composite sub-proxy was defined. Please make sure that "
      "the configuration file defines it.");
    }
  else
    {
    unsigned int numObjects = compositeProxy->GetNumberOfIDs();
    for (unsigned i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
        << compositeProxy->GetID(i) 
        << "TileWindows" 
        << xsize << ysize << nColumns
        << vtkClientServerStream::End;
      }
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendStream(compositeProxy->Servers, str, 0);
    }

}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::WriteImage(const char* filename,
  const char* writerName)
{
  if (!filename || !writerName)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream str;

  vtkSMProxy* imageWriter = vtkSMProxy::New();
  imageWriter->SetServers(vtkProcessModule::CLIENT);

  imageWriter->SetVTKClassName(writerName);
  imageWriter->CreateVTKObjects(1);

  pm->SendPrepareProgress();

  str << vtkClientServerStream::Invoke 
    << this->WindowToImage->GetID(0) 
    << "GetOutput" 
    << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
    << imageWriter->GetID(0) 
    << "SetInput" 
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
    << this->WindowToImage->GetID(0) 
    << "Modified" 
    << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
    << imageWriter->GetID(0) 
    << "SetFileName" 
    << filename
    << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
    << imageWriter->GetID(0) 
    << "Write" 
    << vtkClientServerStream::End;

  pm->SendStream(imageWriter->Servers, str, 0);
  str.Reset();
  pm->SendCleanupPendingProgress();

  imageWriter->Delete();

}

//---------------------------------------------------------------------------
vtkCamera* vtkSMDisplayWindowProxy::GetCamera(unsigned int idx)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm)
    {
    vtkSMProxy* cameraProxy = this->GetSubProxy("camera");
    if (cameraProxy && cameraProxy->GetNumberOfIDs() > idx)
      {
      return vtkCamera::SafeDownCast(
        pm->GetObjectFromID(cameraProxy->GetID(idx)));
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkRenderWindow* vtkSMDisplayWindowProxy::GetRenderWindow()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm && this->GetNumberOfIDs() > 0)
    {
    return vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->GetID(0)));
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::AddDisplayer(vtkSMProxy* display)
{
  vtkSMDisplayWindowProxyInternals::DisplayVectorType::iterator iter =
    this->DWInternals->Displayers.begin();

  for(; iter != this->DWInternals->Displayers.end(); iter++)
    {
    if ( display == iter->GetPointer()) { break; }
    }

  if ( iter != this->DWInternals->Displayers.end() )
    {
    vtkDebugMacro("Displayer has been added before. Ignoring.");
    return;
    }

  this->DWInternals->Displayers.push_back(display);

  this->CreateVTKObjects(1);
  vtkSMDisplayerProxy* dProxy = vtkSMDisplayerProxy::SafeDownCast(display);
  if (dProxy)
    {
    dProxy->AddToDisplayWindow(this);
    }
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::StillRender()
{
  int numObjects = this->GetNumberOfIDs();
  vtkClientServerStream str;
  for (int i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
      << this->GetID(i) << "Render"
      << vtkClientServerStream::End;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  // Call render on the client only. The composite manager should
  // take care of the rest.
  pm->SendStream(vtkProcessModule::CLIENT, str, 0);
  str.Reset();
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::InteractiveRender()
{
  // TODO implement this
  this->StillRender();
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderModule: " << this->RenderModule << endl;
}
