/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientServerRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientServerRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMClientServerRenderSyncManagerHelper.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMClientServerRenderViewProxy);
vtkCxxRevisionMacro(vtkSMClientServerRenderViewProxy, "1.12");

//----------------------------------------------------------------------------
vtkSMClientServerRenderViewProxy::vtkSMClientServerRenderViewProxy()
{
  this->RenderSyncManager = 0;
  this->RenderersID = 0;
}

//----------------------------------------------------------------------------
vtkSMClientServerRenderViewProxy::~vtkSMClientServerRenderViewProxy()
{
  if (this->RenderSyncManager && (this->RenderersID !=0))
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
void vtkSMClientServerRenderViewProxy::InitializeForMultiView(
  vtkSMViewProxy* view)
{
  vtkSMClientServerRenderViewProxy* otherView =
    vtkSMClientServerRenderViewProxy::SafeDownCast(view);
  if (!otherView)
    {
    vtkErrorMacro("Other view must be a vtkSMClientServerRenderViewProxy.");
    return;
    }

  if (this->ObjectsCreated)
    {
    vtkErrorMacro("InitializeForMultiView must be called before CreateVTKObjects.");
    return;
    }

  otherView->UpdateVTKObjects();

  this->SharedServerRenderSyncManagerID = otherView->RenderSyncManager->GetID();
  this->SharedRenderWindowID = otherView->RenderWindowProxy->GetID();

  this->Superclass::InitializeForMultiView(view);
}

//----------------------------------------------------------------------------
bool vtkSMClientServerRenderViewProxy::BeginCreateVTKObjects()
{
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

  return true;
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // * Initialize the RenderSyncManager.
  this->InitializeRenderSyncManager();
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::InitializeRenderSyncManager()
{
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

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
  stream  << vtkClientServerStream::Invoke
          << this->RenderSyncManager->GetID()
          << "SetAnnotationLayerVisible"
          << 0
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
void vtkSMClientServerRenderViewProxy::BeginStillRender()
{
  this->Superclass::BeginStillRender();

  // Signal the desktop delivery client that it must now
  // employ loss-less compression if compression is enabled.
  vtkSMProperty *prop
    = this->RenderSyncManager->GetProperty("LossLessCompression");
  vtkSMIntVectorProperty* ivp=dynamic_cast<vtkSMIntVectorProperty*>(prop);
  if (ivp)
    {
    ivp->SetElement(0,1);
    this->RenderSyncManager->UpdateProperty("LossLessCompression");
    }
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::BeginInteractiveRender()
{
  this->Superclass::BeginInteractiveRender();

  // Signal the desktop delivery client that it may
  // now use lossy-compression if it's enabled and
  // the compressor can provide it.
  vtkSMProperty *prop
    = this->RenderSyncManager->GetProperty("LossLessCompression");
  vtkSMIntVectorProperty* ivp=dynamic_cast<vtkSMIntVectorProperty*>(prop);
  if (ivp)
    {
    ivp->SetElement(0,0);
    this->RenderSyncManager->UpdateProperty("LossLessCompression");
    }
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::SetUseCompositing(bool usecompositing)
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
void vtkSMClientServerRenderViewProxy::SetGUISize(int x, int y)
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
void vtkSMClientServerRenderViewProxy::SetViewPosition(int x, int y)
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
double vtkSMClientServerRenderViewProxy::GetZBufferValue(int x, int y)
{
  if (!this->LastCompositingDecision)
    {
    // When rendering on client, the client Z buffer value is indeed correct.
    return this->Superclass::GetZBufferValue(x, y);
    }

  // Get the z value from the server process. Since this view doesn't support
  // multiple server processes/compositing, we simply get the z value from the
  // server root.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->RenderSyncManager->GetID()
    << "SetCaptureZBuffer" << 1
    <<  vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT, stream);

  this->StillRender();

  stream << vtkClientServerStream::Invoke
    << this->RenderSyncManager->GetID()
    << "SetCaptureZBuffer" << 0
    <<  vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
    << this->RenderSyncManager->GetID()
    << "GetZBufferValue"
    << x << y 
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT, stream);
  const vtkClientServerStream& res = 
    pm->GetLastResult(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT);

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
void vtkSMClientServerRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


