/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTRenderModule.h"

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
vtkStandardNewMacro(vtkPVIceTRenderModule);
vtkCxxRevisionMacro(vtkPVIceTRenderModule, "1.10");

//----------------------------------------------------------------------------
vtkPVIceTRenderModule::vtkPVIceTRenderModule()
{
  this->DisplayManagerID.ID = 0;

  this->ReductionFactor = 2;
}

//----------------------------------------------------------------------------
vtkPVIceTRenderModule::~vtkPVIceTRenderModule()
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
void vtkPVIceTRenderModule::SetProcessModule(vtkProcessModule *pm)
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

  // Make an ICE-T renderer on the server and a regular renderer on the client.
  this->RendererID = pvm->NewStreamObject("vtkIceTRenderer");
  pvm->SendStream(vtkProcessModule::RENDER_SERVER);
  stream << vtkClientServerStream::New << "vtkRenderer" << this->RendererID
         << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::CLIENT);

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

  // Anti-aliasing screws up the compositing.  Turn it off.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions() > 1))
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->RenderWindowID << "SetMultiSamples" << 0
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }

  stream << vtkClientServerStream::Invoke << this->RenderWindowID 
    << "FullScreenOn" 
    << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER);

  if (pvm->GetOptions()->GetUseStereoRendering())
    {
    stream << vtkClientServerStream::Invoke << this->RenderWindowID 
      << "StereoCapableWindowOn" 
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->RenderWindowID 
      << "StereoRenderOn" 
      << vtkClientServerStream::End;
    //stream << vtkClientServerStream::Invoke << this->RenderWindowID 
    //                << "SetStereoTypeToCrystalEyes" 
    //                << vtkClientServerStream::End;
    pvm->SendStream(vtkProcessModule::RENDER_SERVER);

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
  pvm->SendStream(vtkProcessModule::RENDER_SERVER);
  int *tileDim = pvm->GetOptions()->GetTileDimensions();
  cout << "Size: " << tileDim[0] << ", " << tileDim[1] << endl;
  stream << vtkClientServerStream::Invoke
                  << this->DisplayManagerID
                  << "SetTileDimensions"
                  << tileDim[0] << tileDim[1]
                  << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER);

  stream << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER);

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

  this->CompositeID = pvm->NewStreamObject("vtkDesktopDeliveryClient");
  if (tileDim[0])
    {
    stream << vtkClientServerStream::Invoke << this->CompositeID
           << "UseTileDisplayOn" << vtkClientServerStream::End;
    }
  pvm->SendStream(vtkProcessModule::CLIENT);
  // Create a vtkDesktopDeliveryServer on the server, but use the same id.
  stream << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
         << this->CompositeID << vtkClientServerStream::End;
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
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->CompositeID
                  << "UseCompositingOn"
                  << vtkClientServerStream::End;
  pvm->SendStream(  vtkProcessModule::CLIENT
                  | vtkProcessModule::RENDER_SERVER_ROOT);

  // The client server manager needs to set parameters on the IceT manager.
  stream << vtkClientServerStream::Invoke << this->CompositeID
         << "SetParallelRenderManager" << this->DisplayManagerID
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->CompositeID
         << "RemoteDisplayOff" << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::SetUseCompositeCompression(int)
{
  // IceT does not have this option.
  // The purpose of this function is to override default behavior in
  // vtkPVCompositeRenderModule::SetUseCompositeCompression which create an error
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DisplayManagerID: " << this->DisplayManagerID.ID << endl;
}

