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
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDeskTopRenderModule);
vtkCxxRevisionMacro(vtkPVDeskTopRenderModule, "1.3.2.2");



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

//#############################
//#############################
//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::SetProcessModule(vtkProcessModule *pm)
{
  vtkPVProcessModule* pvm = vtkPVProcessModule::SafeDownCast(pm);
  if (this->ProcessModule)
    {
    if (pvm == NULL)
      {
      this->ProcessModule->UnRegister(this);
      this->ProcessModule = NULL;
      return;
      }
    vtkErrorMacro("ProcessModule already set.");
    return;
    }
  if (pvm == NULL)
    {
    return;
    }  

  vtkClientServerStream& stream = pvm->GetStream();
  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->ProcessModule = pvm;
  this->ProcessModule->Register(this);

  this->RendererID = pvm->NewStreamObject("vtkIceTRenderer");
  this->Renderer2DID = pvm->NewStreamObject("vtkRenderer");
  this->RenderWindowID = pvm->NewStreamObject("vtkRenderWindow");
  pvm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pvm->GetObjectFromID(this->RendererID));
  this->Renderer2D = 
    vtkRenderer::SafeDownCast(
      pvm->GetObjectFromID(this->Renderer2DID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pvm->GetObjectFromID(this->RenderWindowID));
  
  if (pvm->GetOptions()->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }

  // We cannot erase the zbuffer.  We need it for picking.  
  stream << vtkClientServerStream::Invoke
         << this->Renderer2DID << "EraseOff" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->Renderer2DID << "SetLayer" << 2 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "SetNumberOfLayers" << 3
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->RendererID
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->Renderer2DID
         << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->DisplayManagerID = pvm->NewStreamObject("vtkIceTRenderManager");

  stream << vtkClientServerStream::Invoke
                  << this->DisplayManagerID
                  << "SetTileDimensions"
                  << 1 << 1 
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke
                  << pvm->GetProcessModuleID()
                  << "GetController"
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetController"
                  << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->DisplayManagerID 
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER);

  // **********************************************************

  // create a vtkDesktopDeliveryClient on the client
  this->CompositeID = pvm->NewStreamObject("vtkDesktopDeliveryClient");
  pvm->SendStream(vtkProcessModule::CLIENT);
  // create a vtkDesktopDeliveryServer on the server, but use
  // the same id
  stream << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
                  <<  this->CompositeID <<  vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  stream << vtkClientServerStream::Invoke << pvm->GetProcessModuleID()
                  << "GetRenderServerSocketController"
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  vtkClientServerStream tmp = pvm->GetStream();
  pvm->SendStream(vtkProcessModule::CLIENT);
  pvm->GetStream() = tmp;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
  

  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetParallelRenderManager" << this->DisplayManagerID
                  << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);

  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  // Default to off so that the render window does not show up until necessary. 
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "UseCompositingOff"
                  << vtkClientServerStream::End;
  if ( this->ProcessModule->GetServerInformation()->GetUseOffscreenRendering() )
    {
    stream
      << vtkClientServerStream::Invoke << this->CompositeID
      << "InitializeOffScreen" << vtkClientServerStream::End;
    }
  tmp = stream;
  pvm->SendStream(vtkProcessModule::CLIENT);
  stream = tmp;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);

  this->InitializeObservers();
}
//#############################E
//#############################E

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

