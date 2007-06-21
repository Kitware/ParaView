/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTDesktopRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTDesktopRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkSMIntVectorProperty.h"

#ifndef PV_IMPLEMENT_CLIENT_SERVER_WO_ICET
vtkStandardNewMacro(vtkSMIceTDesktopRenderViewProxy);
vtkCxxRevisionMacro(vtkSMIceTDesktopRenderViewProxy, "1.6");
#endif

//----------------------------------------------------------------------------
vtkSMIceTDesktopRenderViewProxy::vtkSMIceTDesktopRenderViewProxy()
{
  this->RenderSyncManager = 0;
  this->SquirtLevel = 0;
}

//----------------------------------------------------------------------------
vtkSMIceTDesktopRenderViewProxy::~vtkSMIceTDesktopRenderViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::InitializeForMultiView(
  vtkSMViewProxy* view)
{
  vtkSMIceTDesktopRenderViewProxy* otherView =
    vtkSMIceTDesktopRenderViewProxy::SafeDownCast(view);
  if (!otherView)
    {
    vtkErrorMacro("Other view must be a vtkSMIceTDesktopRenderViewProxy.");
    return;
    }

  if (this->ObjectsCreated)
    {
    vtkErrorMacro("InitializeForMultiView must be called before CreateVTKObjects.");
    return;
    }

  otherView->UpdateVTKObjects();

  this->SharedServerRenderSyncManagerID =
    otherView->SharedServerRenderSyncManagerID.IsNull()?
    otherView->RenderSyncManager->GetID():
    otherView->SharedServerRenderSyncManagerID;

  this->Superclass::InitializeForMultiView(view);
}

//----------------------------------------------------------------------------
bool vtkSMIceTDesktopRenderViewProxy::BeginCreateVTKObjects()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // vtkSMIceTCompositeViewProxy (i.e. the superclass) uses the shared render
  // window directly, however, in client-server mode, we share the render
  // windows only on the server side. Hence we create client side instances for
  // the render window.
  if (!this->SharedRenderWindowID.IsNull())
    {
    this->RenderWindowProxy = this->GetSubProxy("RenderWindow");
    if (!this->RenderWindowProxy)
      {
      vtkErrorMacro("RenderWindow subproxy must be defined.");
      return false;
      }

    this->RenderWindowProxy->SetServers(vtkProcessModule::CLIENT);
    this->RenderWindowProxy->UpdateVTKObjects();

    stream  << vtkClientServerStream::Assign
            << this->RenderWindowProxy->GetID()
            << this->SharedRenderWindowID
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

    this->RenderWindowProxy->SetServers(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->RenderSyncManager = this->GetSubProxy("RenderSyncManager");
  if (!this->RenderSyncManager)
    {
    vtkErrorMacro("RenderSyncManager subproxy must be defined.");
    return false;
    }

  // RenderSyncManager proxy represents vtkPVDesktopDeliveryClient on the client
  // side and vtkPVDesktopDeliveryServer on the server root.
  // Additionally, if SharedServerRenderSyncManagerID is set, then the server side
  // vtkPVDesktopDeliveryServer instance is shared among all views.
  
  // XML Configuration defines the client side class for RenderSyncManager.
  this->RenderSyncManager->SetServers(vtkProcessModule::CLIENT);
  this->RenderSyncManager->UpdateVTKObjects();
  // This will create the client side  vtkPVDesktopDeliveryClient.
  
  if (!this->SharedServerRenderSyncManagerID.IsNull())
    {
    stream  << vtkClientServerStream::Assign
            << this->RenderSyncManager->GetID()
            << this->SharedServerRenderSyncManagerID
            << vtkClientServerStream::End;
    }
  else
    {
    stream  << vtkClientServerStream::New
            << "vtkPVDesktopDeliveryServer"
            << this->RenderSyncManager->GetID()
            << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

  this->RenderSyncManager->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER_ROOT);

  // We need to create vtkIceTRenderer on the server side and vtkRenderer on
  // the client.
  this->RendererProxy->SetServers(vtkProcessModule::CLIENT);
  this->RendererProxy->UpdateVTKObjects();

  stream  << vtkClientServerStream::New 
          << "vtkIceTRenderer" 
          << this->RendererProxy->GetID()
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  this->RendererProxy->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // * Initialize the RenderSyncManager.
  this->InitializeRenderSyncManager();
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::InitializeRenderSyncManager()
{
  vtkSMIntVectorProperty* ivp;
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (this->GetSubProxy("ParallelRenderManager"))
    {
    // RenderSyncManager needs the parallel render manager on the server side to
    // pass parameters to the parallel render manager.
    stream  << vtkClientServerStream::Invoke
            << this->RenderSyncManager->GetID()
            << "SetParallelRenderManager"
            << this->GetSubProxy("ParallelRenderManager")->GetID()
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER_ROOT, stream);
    }

  // Synchronize the environment among all the render server nodes.
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(this->ConnectionID);
  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  for (idx = 0; idx < numMachines; idx++)
    {
    if (serverInfo->GetEnvironment(idx))
      {
      stream  << vtkClientServerStream::Invoke 
              << pm->GetProcessModuleID() 
              << "SetProcessEnvironmentVariable" 
              << idx 
              << serverInfo->GetEnvironment(idx)
              << vtkClientServerStream::End;
      }
    }
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, 
    stream);

  // RenderSyncManager needs access to the socket controller between the client 
  // and the render server root. This is the socket connection that is used by
  // the vtkPVDesktopDeliveryClient and vtkPVDesktopDeliveryServer to
  // communicate. So, set that up.
  stream  << vtkClientServerStream::Invoke 
          << pm->GetProcessModuleID()
          << "GetRenderServerSocketController"
          << pm->GetConnectionClientServerID(this->ConnectionID)
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetController" 
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    this->RenderSyncManager->GetServers(), stream);

  // In case we are using MultiView support, the server side RenderWindow may
  // have more renderers than this view module has (since all view modules share
  // the server side render windows). In that case, we don't want to sync the
  // render window renderers.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("SyncRenderWindowRenderers"));
  if (!ivp)
    {
    vtkErrorMacro("Falied to find property SyncRenderWindowRenderers");
    return;
    }
  ivp->SetElement(0, 0);
  this->RenderSyncManager->UpdateVTKObjects();

  // Setup RMI callbacks. 
  // FIXME: Make InitializeRMIs idempotent.
  this->RenderSyncManager->InvokeCommand("InitializeRMIs");

  this->Connect(this->RenderWindowProxy, this->RenderSyncManager, "RenderWindow");

  // Update the server process so that the render window is set before
  // we initialize offscreen rendering.
  this->RenderSyncManager->UpdateVTKObjects();

  if (getenv("PV_DISABLE_COMPOSITE_INTERRUPTS"))
    {
    // Does anything support EnableAbort right now?
    this->RenderSyncManager->InvokeCommand("EnableAbort");
    }

  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    // Non-mesa, X offscreen rendering requires access to the display
    vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
    pm->GatherInformation(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
    if (di->GetCanOpenDisplay())
      {
      this->RenderSyncManager->InvokeCommand("InitializeOffScreen");
      }
    di->Delete();
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("UseCompositing"));
  if (ivp)
    {
    // So that the server window does not popup until needed.
    ivp->SetElement(0, 0); 
    }

  this->RenderSyncManager->UpdateVTKObjects();

  // Make the render sync manager aware of our renderers.
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "AddRenderer" 
          << (int)this->GetSelfID().ID
          << this->RendererProxy->GetID() 
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "AddRenderer" 
          << this->RendererProxy->GetID()
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetId" 
          << (int)this->GetSelfID().ID
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::BeginStillRender()
{
  this->Superclass::BeginStillRender();

  // Disable squirt compression.
  this->SetSquirtLevelInternal(0);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::BeginInteractiveRender()
{
  this->Superclass::BeginInteractiveRender();

  // Use user-specified squirt compression.
  this->SetSquirtLevelInternal(this->SquirtLevel);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetImageReductionFactorInternal(int factor)
{
  // We don't need to set the image reduction factor on the
  // ParallelRenderManager, but on the RenderSyncManager.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("ImageReductionFactor"));
  if (ivp)
    {
    ivp->SetElement(0, factor);
    this->RenderSyncManager->UpdateProperty("ImageReductionFactor");
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetSquirtLevelInternal(int level)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("SquirtLevel"));
  if (ivp)
    {
    ivp->SetElement(0, level);
    this->RenderSyncManager->UpdateProperty("SquirtLevel");
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetUseCompositing(bool usecompositing)
{
  // We don't need to set the UseCompositing flag on the
  // ParallelRenderManager, but on the RenderSyncManager.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("UseCompositing"));
  if (ivp)
    {
    ivp->SetElement(0, usecompositing? 1 : 0);
    this->RenderSyncManager->UpdateProperty("UseCompositing");
    } 

  // Update the view information so that all representations/strategies will be
  // made aware of the new UseCompositing state.
  this->Information->Set(USE_COMPOSITING(), usecompositing? 1: 0);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetGUISize(int x, int y)
{
  this->vtkSMRenderViewProxy::SetGUISize(x, y);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetGUISize" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetViewPosition(int x, int y)
{
  this->Superclass::SetViewPosition(x, y);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetWindowPosition" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


