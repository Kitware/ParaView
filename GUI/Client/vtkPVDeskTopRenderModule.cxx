/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDeskTopRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDeskTopRenderModule.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeRenderManager.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDeskTopRenderModule);
vtkCxxRevisionMacro(vtkPVDeskTopRenderModule, "1.6");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVDeskTopRenderModule::vtkPVDeskTopRenderModule()
{
  this->DisplayManagerID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVDeskTopRenderModule::~vtkPVDeskTopRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;
  
  vtkPVProcessModule * pm = 0;
  if ( pvApp )
    {
    pm = pvApp->GetProcessModule();
    }

  // Tree Composite
  if (this->DisplayManagerID.ID && pvApp && pm)
    {
    pm->DeleteStreamObject(this->DisplayManagerID);
    pm->SendStreamToServer(); 
    this->DisplayManagerID.ID = 0;
    }
  if (this->CompositeID.ID && pvApp && pm)
    {
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStreamToClient();
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStreamToServerRoot();
    this->CompositeID.ID = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::SetPVApplication(vtkPVApplication *pvApp)
{
  if (this->PVApplication)
    {
    if (pvApp == NULL)
      {
      this->PVApplication->UnRegister(this);
      this->PVApplication = NULL;
      return;
      }
    vtkErrorMacro("PVApplication already set.");
    return;
    }
  if (pvApp == NULL)
    {
    return;
    }  

  vtkPVProcessModule * pm = pvApp->GetProcessModule();
  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);

  this->RendererID = pm->NewStreamObject("vtkIceTRenderer");
  this->RenderWindowID = pm->NewStreamObject("vtkRenderWindow");
  pm->SendStreamToClientAndServer();
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->RendererID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->RenderWindowID));
  
  if (pvApp->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->RenderWindowID << "AddRenderer" << this->RendererID
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  
  this->DisplayManagerID = pm->NewStreamObject("vtkIceTRenderManager");
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetTileDimensions"
                  << 1 << 1 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << pm->GetProcessModuleID()
                  << "GetController"
                  << vtkClientServerStream::End;
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetController"
                  << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID 
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();

  // **********************************************************

  // create a vtkDesktopDeliveryClient on the client
  this->CompositeID = pm->NewStreamObject("vtkDesktopDeliveryClient");
  pm->SendStreamToClient();
  // create a vtkDesktopDeliveryServer on the server, but use
  // the same id
  pm->GetStream() << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
                  <<  this->CompositeID <<  vtkClientServerStream::End;
  pm->SendStreamToServerRoot();

  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetApplicationID() << "GetSocketController"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  vtkClientServerStream tmp = pm->GetStream();
  pm->SendStreamToClient();
  pm->GetStream() = tmp;
  pm->SendStreamToServerRoot();
  

  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetParallelRenderManager" << this->DisplayManagerID
                  << vtkClientServerStream::End;
  pm->SendStreamToServerRoot();

  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "UseCompositingOn"
                  << vtkClientServerStream::End;
  tmp = pm->GetStream();
  pm->SendStreamToClient();
  pm->GetStream() = tmp;
  pm->SendStreamToServerRoot();
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::StillRender()
{
  this->Superclass::StillRender();
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::InteractiveRender()
{
  this->Superclass::InteractiveRender();
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

