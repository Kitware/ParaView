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
#include "vtkPVProcessModule.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeRenderManager.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDeskTopRenderModule);
vtkCxxRevisionMacro(vtkPVDeskTopRenderModule, "1.1");



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
  vtkPVProcessModule * pm = this->ProcessModule;

  // Tree Composite
  if (this->DisplayManagerID.ID && pm)
    {
    pm->DeleteStreamObject(this->DisplayManagerID);
    pm->SendStream(vtkProcessModule::RENDER_SERVER); 
    this->DisplayManagerID.ID = 0;
    }
  if (this->CompositeID.ID && pm)
    {
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStream(vtkProcessModule::CLIENT);
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
    this->CompositeID.ID = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::SetProcessModule(vtkPVProcessModule *pm)
{
  if (this->ProcessModule)
    {
    if (pm == NULL)
      {
      this->ProcessModule->UnRegister(this);
      this->ProcessModule = NULL;
      return;
      }
    vtkErrorMacro("ProcessModule already set.");
    return;
    }
  if (pm == NULL)
    {
    return;
    }  

  this->ProcessModule = pm;
  this->ProcessModule->Register(this);

  this->RendererID = pm->NewStreamObject("vtkIceTRenderer");
  this->Renderer2DID = pm->NewStreamObject("vtkRenderer");
  this->RenderWindowID = pm->NewStreamObject("vtkRenderWindow");
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->RendererID));
  this->Renderer2D = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->Renderer2DID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->RenderWindowID));
  
  if (pm->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }
  
  // We cannot erase the zbuffer.  We need it for picking.  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->Renderer2DID << "EraseOff" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->Renderer2DID << "SetLayer" << 2 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->RenderWindowID << "SetNumberOfLayers" << 3
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->RenderWindowID << "AddRenderer" << this->RendererID
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->RenderWindowID << "AddRenderer" << this->Renderer2DID
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  
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
  pm->SendStream(vtkProcessModule::RENDER_SERVER);

  // **********************************************************

  // create a vtkDesktopDeliveryClient on the client
  this->CompositeID = pm->NewStreamObject("vtkDesktopDeliveryClient");
  pm->SendStream(vtkProcessModule::CLIENT);
  // create a vtkDesktopDeliveryServer on the server, but use
  // the same id
  pm->GetStream() << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
                  <<  this->CompositeID <<  vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);

  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetProcessModuleID() << "GetSocketController"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  vtkClientServerStream tmp = pm->GetStream();
  pm->SendStream(vtkProcessModule::CLIENT);
  pm->GetStream() = tmp;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
  

  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetParallelRenderManager" << this->DisplayManagerID
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);

  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  // Default to off so that the render window does not show up until necessary. 
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "UseCompositingOff"
                  << vtkClientServerStream::End;
  tmp = pm->GetStream();
  pm->SendStream(vtkProcessModule::CLIENT);
  pm->GetStream() = tmp;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);

  this->InitializeObservers();
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

