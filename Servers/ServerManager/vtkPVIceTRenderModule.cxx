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
#include "vtkPVServerInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTRenderModule);
vtkCxxRevisionMacro(vtkPVIceTRenderModule, "1.2");

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
void vtkPVIceTRenderModule::SetProcessModule(vtkPVProcessModule *pm)
{
  if (this->ProcessModule)
    {
    if (pm == 0)
      {
      this->ProcessModule->UnRegister(this);
      this->ProcessModule = 0;
      return;
      }
    vtkErrorMacro("ProcessModule already set.");
    return;
    }
  if (pm == 0)
    {
    return;
    }  

  // Maybe I should not reference count this object to avoid
  // a circular reference.
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
  pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                  << "FullScreenOn" 
                  << vtkClientServerStream::End;

  if (pm->GetUseStereoRendering())
    {
    pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                    << "StereoCapableWindowOn" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                    << "StereoRenderOn" 
                    << vtkClientServerStream::End;
    //pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
    //                << "SetStereoTypeToCrystalEyes" 
    //                << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::RENDER_SERVER);
  
  // Why have this on the client?
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
  pm->SendStream(vtkProcessModule::RENDER_SERVER);
  int *tileDim = pm->GetServerInformation()->GetTileDimensions();
  cout << "Size: " << tileDim[0] << ", " << tileDim[1] << endl;
  pm->GetStream() <<  vtkClientServerStream::Invoke
                  << this->DisplayManagerID
                  << "SetTileDimensions"
                  << tileDim[0] << tileDim[1]
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER);

  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER);

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
  this->CompositeID = pm->NewStreamObject("vtkIceTClientCompositeManager");
  vtkClientServerStream tmp = pm->GetStream();
  pm->SendStream(vtkProcessModule::CLIENT);
  pm->GetStream() = tmp;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
                  << "GetRenderServerSocketController"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetClientController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetProcessModuleID() << "GetClientMode"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetClientFlag" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End; 
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
  // copy the stream before it is sent and reset
  vtkClientServerStream copy = pm->GetStream();
  pm->SendStream(vtkProcessModule::CLIENT); // send the stream to the client
  pm->GetStream() = copy; // now copy the copy into the current stream
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT); // send the same stream to the server root

  // The client server manager needs to set parameters on the IceT manager.
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetIceTManager" << this->DisplayManagerID
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT);
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::StillRender()
{
  // No reduction for still render.
  /*
  if (this->ProcessModule && this->DisplayManagerTclName)
    {
    this->ProcessModule->RootScript("%s SetImageReductionFactor 1",
                                         this->DisplayManagerTclName);
    this->ProcessModule->BroadcastScript("%s ParallelRenderingOn",
                                         this->DisplayManagerTclName);
    }
  */

  this->Superclass::StillRender();
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::InteractiveRender()
{
  /*
  if (this->ProcessModule && this->DisplayManagerTclName)
    {
    this->ProcessModule->RootScript("%s SetImageReductionFactor %d",
                                         this->DisplayManagerTclName,
                                         this->ReductionFactor);
    this->ProcessModule->BroadcastScript("%s ParallelRenderingOn",
                                         this->DisplayManagerTclName);
    }
  */
  this->Superclass::InteractiveRender();
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::SetUseCompositeCompression(int)
{
  // IceT does not have this option.
}


//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
}

