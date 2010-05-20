/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTMultiDisplayRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"

vtkInformationKeyMacro(vtkSMIceTMultiDisplayRenderViewProxy, CLIENT_COLLECT, Integer);
vtkInformationKeyMacro(vtkSMIceTMultiDisplayRenderViewProxy, CLIENT_RENDER, Integer);

vtkStandardNewMacro(vtkSMIceTMultiDisplayRenderViewProxy);
//-----------------------------------------------------------------------------
vtkSMIceTMultiDisplayRenderViewProxy::vtkSMIceTMultiDisplayRenderViewProxy()
{
  this->EnableTiles = true;
  this->CollectGeometryThreshold = 10.0;
  this->StillRenderImageReductionFactor = 1;
  this->TileDisplayCompositeThreshold = 20.0;

  this->LastClientCollectionDecision = false;
  this->Information->Set(CLIENT_RENDER(), 1);
  this->Information->Set(CLIENT_COLLECT(), 0);

  this->GUISizeCompact[0] = this->GUISizeCompact[1] = 0;
  this->ViewSizeCompact[0] = this->ViewSizeCompact[1] = 0;
  this->ViewPositionCompact[0] = this->ViewPositionCompact[1] = 0;
}

//-----------------------------------------------------------------------------
vtkSMIceTMultiDisplayRenderViewProxy::~vtkSMIceTMultiDisplayRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::EndCreateVTKObjects()
{
  // Obtain information about the tiles from the process module options.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(this->ConnectionID);
  if (serverInfo)
    {
    serverInfo->GetTileMullions(this->TileMullions);
    serverInfo->GetTileDimensions(this->TileDimensions);
    }

  this->Superclass::EndCreateVTKObjects();

  if (!this->RemoteRenderAvailable)
    {
    vtkErrorMacro("Display not accessible on server. "
      "Cannot render on tiles with inaccesible display.");
    return;
    }

  // Make the server-side windows fullscreen 
  // (unless PV_ICET_WINDOW_BORDERS is set)
  vtkSMIntVectorProperty* ivp;
  if (!getenv("PV_ICET_WINDOW_BORDERS"))
    {
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->RenderWindowProxy->GetID()
            << "SetFullScreen" << 1
            << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::RENDER_SERVER,
      stream);
    }

  // We always render on the server-side when using tile displays.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("UseCompositing"));
  if (ivp)
    {
    ivp->SetElement(0, 1); 
    }
  this->RenderSyncManager->UpdateProperty("UseCompositing");

  // Don't set composited images back to the client.
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetRemoteDisplay" << 0
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER_ROOT,
    stream);
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::BeginStillRender()
{
  // Determine if we are going to collect on the client side, or the data is too
  // large and we should simply show outlines on the client.
  this->LastClientCollectionDecision = 
    this->GetClientCollectionDecision(this->GetVisibileFullResDataSize());

  // Update information object with client collection decision.
  this->SetClientCollect(this->LastClientCollectionDecision);

  this->Superclass::BeginStillRender();

  // Update image reduction factor.
  this->SetImageReductionFactorInternal(this->StillRenderImageReductionFactor);
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::BeginInteractiveRender()
{
  // Determine if we are going to collect on the client side, or the data is too
  // large and we should simply shown outlines on the client.
  this->LastClientCollectionDecision = 
    this->GetClientCollectionDecision(this->GetVisibileFullResDataSize());

  // Update information object with client collection decision.
  this->SetClientCollect(this->LastClientCollectionDecision);

  this->Superclass::BeginInteractiveRender();
}

//-----------------------------------------------------------------------------
bool vtkSMIceTMultiDisplayRenderViewProxy::GetCompositingDecision(
    unsigned long totalMemory, int vtkNotUsed(stillRender))
{
  if (!this->RemoteRenderAvailable)
    {
    // Cannot remote render due to setup issues.
    return false;
    }

  if (static_cast<float>(totalMemory)/1000.0 < this->TileDisplayCompositeThreshold)
    {
    return false; // Local render.
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMIceTMultiDisplayRenderViewProxy::GetClientCollectionDecision(
  unsigned long totalMemory)
{
  return (static_cast<double>(totalMemory)/1024.0 < this->CollectGeometryThreshold);
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::SetClientCollect(bool decision)
{
  this->Information->Set(CLIENT_COLLECT(), (decision? 1 : 0)); 
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMIceTMultiDisplayRenderViewProxy::
NewStrategyInternal(int dataType)
{
  vtkSMRepresentationStrategy* strategy = 
    this->Superclass::NewStrategyInternal(dataType);
  if (strategy)
    {
    strategy->SetKeepLODPipelineUpdated(true);
    }
  return strategy;
}


//----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::SetGUISizeCompact(int x, int y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetGUISizeCompact" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::SetViewPositionCompact(int x, int y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetViewPositionCompact" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::SetViewSizeCompact(int x, int y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetViewSizeCompact" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}


//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::SetGUISize(int x, int y)
{
  this->Superclass::SetGUISize(x, y);

  // Set the GUI size on the client-server render sync manager. The manager will
  // then use the provided information to set the server-side view size (and
  // more importantly the positions).
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetGUISize" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::SetViewPosition(int x, int y)
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

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CollectGeometryThreshold: " 
    << this->CollectGeometryThreshold << endl;
  os << indent << "StillRenderImageReductionFactor: " 
    << this->StillRenderImageReductionFactor << endl;
  os << indent << "TileDisplayCompositeThreshold: " 
    << this->TileDisplayCompositeThreshold << endl;
}

