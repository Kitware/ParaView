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

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMDisplayWindowProxy);
vtkCxxRevisionMacro(vtkSMDisplayWindowProxy, "1.14");

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

}

//---------------------------------------------------------------------------
vtkSMDisplayWindowProxy::~vtkSMDisplayWindowProxy()
{
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

  vtkClientServerStream str;

  vtkSMProxy* imageWriter = vtkSMProxy::New();
  imageWriter->SetServers(vtkProcessModule::CLIENT);

  imageWriter->SetVTKClassName(writerName);
  imageWriter->CreateVTKObjects(1);
  
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

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(imageWriter->Servers, str, 0);
  str.Reset();

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

  vtkSMProxy* actorProxy = display->GetSubProxy("actor");
  if (!actorProxy)
    {
    vtkErrorMacro("No actor sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkSMProxy* rendererProxy = this->GetSubProxy("renderer");
  if (!rendererProxy)
    {
    vtkErrorMacro("No renderer sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkClientServerStream str;
  int numActors = actorProxy->GetNumberOfIDs();
  for (int i=0; i<numActors; i++)
    {
    str << vtkClientServerStream::Invoke 
        << rendererProxy->GetID(0) 
        << "AddActor" 
        << actorProxy->GetID(i)
        << vtkClientServerStream::End;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, str, 0);
  str.Reset();
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
}
