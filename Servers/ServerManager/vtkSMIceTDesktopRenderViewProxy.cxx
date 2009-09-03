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
#include "vtkRenderWindow.h"
#include "vtkSMClientServerRenderSyncManagerHelper.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMIceTDesktopRenderViewProxy);
vtkCxxRevisionMacro(vtkSMIceTDesktopRenderViewProxy, "1.18.2.1");

//----------------------------------------------------------------------------
vtkSMIceTDesktopRenderViewProxy::vtkSMIceTDesktopRenderViewProxy()
{
  this->RenderSyncManager = 0;
  this->SquirtLevel = 0;
}

//----------------------------------------------------------------------------
vtkSMIceTDesktopRenderViewProxy::~vtkSMIceTDesktopRenderViewProxy()
{
  if (this->RenderSyncManager && (this->RenderersID !=0) )
    {
    // Remove renderers from the RenderSyncManager.
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
      << this->RenderSyncManager->GetID()
      << "RemoveAllRenderers" << this->RenderersID
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER_ROOT, stream);
    this->RenderersID = 0;
    }
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

  if (!otherView->GetObjectsCreated())
    {
    vtkErrorMacro(
      "InitializeForMultiView was called before the other view was intialized.");
    return;
    }

  this->SharedServerRenderSyncManagerID = otherView->RenderSyncManager->GetID();

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
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");
  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined.");
    return false;
    }

  vtkSMClientServerRenderSyncManagerHelper::CreateRenderWindow(
    this->RenderWindowProxy, this->SharedRenderWindowID);

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
 
  vtkSMClientServerRenderSyncManagerHelper::CreateRenderSyncManager(
    this->RenderSyncManager, this->SharedServerRenderSyncManagerID,
    "vtkPVDesktopDeliveryServer");

  // When using tile displays it is essential that we disable automatic tile
  // parameter synchronization.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("SynchronizeTileProperties"));
  ivp->SetElement(0, this->EnableTiles? 0 : 1);
  this->RenderSyncManager->UpdateVTKObjects();

  if (this->UsingIceTRenderers)
    {
    // We need to create vtkIceTRenderer on the server side and vtkRenderer on
    // the client.
    this->RendererProxy->SetServers(vtkProcessModule::CLIENT);
    this->RendererProxy->GetID(); // this calls CreateVTKObjects().
    
    stream  << vtkClientServerStream::New 
            << "vtkIceTRenderer" 
            << this->RendererProxy->GetID()
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    this->RendererProxy->SetServers(
                                    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    this->RendererProxy->UpdateVTKObjects();
    }

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
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // RenderSyncManager needs the parallel render manager on the server side to
  // pass parameters to the parallel render manager.
  stream  << vtkClientServerStream::Invoke
          << this->RenderSyncManager->GetID()
          << "SetParallelRenderManager"
          << this->ParallelRenderManager->GetID()
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

  vtkSMClientServerRenderSyncManagerHelper::InitializeRenderSyncManager(
    this->RenderSyncManager, this->RenderWindowProxy);

  // Make the render sync manager aware of our renderers.
  this->RenderersID = static_cast<int>(this->GetSelfID().ID);
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "AddRenderer" 
          << this->RenderersID
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
          << this->RenderersID
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::BeginStillRender()
{
  this->Superclass::BeginStillRender();

  // Make squirt compression loss-less, if enabled.
  this->SetSquirtLevelInternal(this->SquirtLevel ? 1 : 0);
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
  // We skip the code in vtkSMIceTCompositeViewProxy which is applicable in
  // non-client server modes alone.
  this->vtkSMRenderViewProxy::SetGUISize(x, y);

  // We don't have to pass the GUI size or view positions to the
  // RenderSyncManager at all since if no values are set the
  // vtkPVDesktopDeliveryClient uses the client side render window properties
  // which is exactly what we want.
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetViewPosition(int x, int y)
{
  // We skip the code in vtkSMIceTCompositeViewProxy which is applicable in
  // non-client server modes alone.
  this->vtkSMRenderViewProxy::SetViewPosition(x, y);

  // We don't have to pass the GUI size or view positions to the
  // RenderSyncManager at all since if no values are set the
  // vtkPVDesktopDeliveryClient uses the client side render window properties
  // which is exactly what we want.
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMIceTDesktopRenderViewProxy::CaptureWindow(int magnification)
{
  // We skip the code in vtkSMIceTCompositeViewProxy which is applicable in
  // non-client server modes alone.
  return this->vtkSMRenderViewProxy::CaptureWindow(magnification);
}

//----------------------------------------------------------------------------
double vtkSMIceTDesktopRenderViewProxy::GetZBufferValue(int x, int y)
{
  if (!this->LastCompositingDecision)
    {
    // When rendering on client, the client Z buffer value is indeed correct.
    return this->Superclass::GetZBufferValue(x, y);
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke
    << this->RendererProxy->GetID()
    << "SetCollectDepthBuffer"
    << 1
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

  // Trigger a fresh render which will collect the Z buffer values.
  this->StillRender();

  stream << vtkClientServerStream::Invoke
    << this->ParallelRenderManager->GetID()
    << "GetZBufferValue"
    << x << y << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT, stream);
  const vtkClientServerStream& res = 
    pm->GetLastResult(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT);

  stream << vtkClientServerStream::Invoke
    << this->RendererProxy->GetID()
    << "SetCollectDepthBuffer"
    << 0
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
    {
    return 0;
    }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
    {
    return 0;
    }

  float result = 0.0;
  return res.GetArgument(0, 0, &result)? result : 0;
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::SetViewSize(int width, int height)
{
  this->RenderWindow->SetSize(width, height);
}

//----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;
}


