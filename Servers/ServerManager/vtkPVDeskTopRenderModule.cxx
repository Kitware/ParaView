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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDeskTopRenderModule);
vtkCxxRevisionMacro(vtkPVDeskTopRenderModule, "1.9");



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

  vtkClientServerStream stream;

  // Tree Composite
  if (this->DisplayManagerID.ID && pm)
    {
    pm->DeleteStreamObject(this->DisplayManagerID, stream);
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream); 
    this->DisplayManagerID.ID = 0;
    }
  if (this->CompositeID.ID && pm)
    {
    pm->DeleteStreamObject(this->CompositeID, stream);
    pm->SendStream(vtkProcessModule::CLIENT, stream);
    pm->DeleteStreamObject(this->CompositeID, stream);
    pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);
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

  vtkClientServerStream stream;
  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->ProcessModule = pvm;
  this->ProcessModule->Register(this);

  // Make an ICE-T renderer on the server and a regular renderer on the client.
  this->RendererID = pvm->NewStreamObject("vtkIceTRenderer", stream);
  pvm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
  stream << vtkClientServerStream::New << "vtkRenderer" << this->RendererID
         << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::CLIENT, stream);

  this->Renderer2DID = pvm->NewStreamObject("vtkRenderer", stream);
  this->RenderWindowID = pvm->NewStreamObject("vtkRenderWindow", stream);
  pvm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pvm->GetObjectFromID(this->RendererID));
  this->Renderer2D = 
    vtkRenderer::SafeDownCast(
      pvm->GetObjectFromID(this->Renderer2DID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pvm->GetObjectFromID(this->RenderWindowID));
  
  // Anti-aliasing screws up the compositing.  Turn it off.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions() > 1))
    {
    stream << vtkClientServerStream::Invoke
           << this->RenderWindowID << "SetMultiSamples" << 0
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
    }

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
  pvm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);

  this->DisplayManagerID = 
    pvm->NewStreamObject("vtkIceTRenderManager", stream);

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
  pvm->SendStream(vtkProcessModule::RENDER_SERVER, stream);

  // **********************************************************

  // create a vtkDesktopDeliveryClient on the client
  this->CompositeID = pvm->NewStreamObject("vtkDesktopDeliveryClient", stream);
  pvm->SendStream(vtkProcessModule::CLIENT, stream);
  // create a vtkDesktopDeliveryServer on the server, but use
  // the same id
  stream << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
                  <<  this->CompositeID <<  vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  stream << vtkClientServerStream::Invoke << pvm->GetProcessModuleID()
                  << "GetRenderServerSocketController"
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pvm->SendStream( 
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER_ROOT, stream);
  

  stream << vtkClientServerStream::Invoke << this->CompositeID
         << "SetParallelRenderManager" << this->DisplayManagerID
         << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);

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
  pvm->SendStream(  
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER_ROOT, stream);

  if ( this->ProcessModule->GetOptions()->GetUseOffscreenRendering() )
    {
    stream
      << vtkClientServerStream::Invoke << this->DisplayManagerID
      << "InitializeOffScreen" << vtkClientServerStream::End;
    pvm->SendStream(vtkProcessModule::RENDER_SERVER, stream);

    stream
      << vtkClientServerStream::Invoke << this->CompositeID
      << "InitializeOffScreen" << vtkClientServerStream::End;
    pvm->SendStream(  
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER_ROOT, stream);
    }

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

