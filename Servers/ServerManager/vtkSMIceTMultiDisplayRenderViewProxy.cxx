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
#include "vtkPVOptions.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"

vtkInformationKeyMacro(vtkSMIceTMultiDisplayRenderViewProxy, CLIENT_COLLECT, Integer);
vtkInformationKeyMacro(vtkSMIceTMultiDisplayRenderViewProxy, CLIENT_RENDER, Integer);

vtkStandardNewMacro(vtkSMIceTMultiDisplayRenderViewProxy);
vtkCxxRevisionMacro(vtkSMIceTMultiDisplayRenderViewProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMIceTMultiDisplayRenderViewProxy::vtkSMIceTMultiDisplayRenderViewProxy()
{
  this->CollectGeometryThreshold = 10.0;
  this->StillRenderImageReductionFactor = 1;

  this->LastClientCollectionDecision = false;
  this->Information->Set(CLIENT_RENDER(), 1);
  this->Information->Set(CLIENT_COLLECT(), 0);
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
  int *tileDims =  pm->GetOptions()->GetTileDimensions();
  this->TileDimensions[0] = tileDims[0];
  this->TileDimensions[1] = tileDims[1];
  int *tileMulls =  pm->GetOptions()->GetTileMullions();
  this->TileMullions[0] = tileMulls[0];
  this->TileMullions[1] = tileMulls[1];

  // Make the server-side windows fullscreen 
  // (unless PV_ICET_WINDOW_BORDERS is set)
  vtkSMIntVectorProperty* ivp;
  if (!getenv("PV_ICET_WINDOW_BORDERS"))
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->RenderWindowProxy->GetProperty("FullScreen"));
    if (ivp)
      {
      ivp->SetElement(0, 1);
      }
    else
      {
      vtkErrorMacro("Failed to find property FullScreen on RenderWindowProxy.");
      }
    }

  this->Superclass::EndCreateVTKObjects();

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
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::BeginStillRender()
{
  // Determine if we are going to collect on the client side, or the data is too
  // large and we should simply shown outlines on the client.
  this->LastClientCollectionDecision = 
    this->GetClientCollectionDecision(this->GetVisibileFullResDataSize());

  // Update information object with client collection decision.
  this->SetClientCollect(this->LastClientCollectionDecision);

  // Ensure that the representations are updated again since the client
  // collection decision may have changed.
  this->SetForceRepresentationUpdate(true);

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

  // Ensure that the representations are updated again since the client
  // collection decision may have changed.
  this->SetForceRepresentationUpdate(true);

  this->Superclass::BeginInteractiveRender();
}

//-----------------------------------------------------------------------------
bool vtkSMIceTMultiDisplayRenderViewProxy::GetCompositingDecision(
    unsigned long vtkNotUsed(totalMemory), int vtkNotUsed(stillRender))
{
  // Always render on the server side.
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


//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CollectGeometryThreshold: " 
    << this->CollectGeometryThreshold << endl;
  os << indent << "StillRenderImageReductionFactor: " 
    << this->StillRenderImageReductionFactor << endl;
}
