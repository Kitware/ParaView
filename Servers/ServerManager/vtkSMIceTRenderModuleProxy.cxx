/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTRenderModuleProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMIceTMultiDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkRenderWindow.h"

vtkStandardNewMacro(vtkSMIceTRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMIceTRenderModuleProxy, "1.8");

//-----------------------------------------------------------------------------
vtkSMIceTRenderModuleProxy::vtkSMIceTRenderModuleProxy()
{
  this->SetDisplayXMLName("IceTMultiDisplay");
  // don't send locally rendered images back to the client.
  this->RemoteDisplay = 0;

  this->CollectGeometryThreshold = 100.0;

  this->StillReductionFactor = 1;
}

//-----------------------------------------------------------------------------
vtkSMIceTRenderModuleProxy::~vtkSMIceTRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::InitializeCompositingPipeline()
{
  vtkSMIntVectorProperty* ivp;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int *tileDims =  pm->GetOptions()->GetTileDimensions();
  this->TileDimensions[0] = tileDims[0];
  this->TileDimensions[1] = tileDims[1];
  int *tileMulls =  pm->GetOptions()->GetTileMullions();
  this->TileMullions[0] = tileMulls[0];
  this->TileMullions[1] = tileMulls[1];


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
  // TODO: full screen must be sent only to render servers. I am sending that to
  // client as well, but it doesn;t seem to have any effect on the client.
  // verify that it is fine.
  this->Superclass::InitializeCompositingPipeline();

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("UseCompositing"));
  if (ivp)
    {
    // In multi display mode, the server windows must be shown immediately.
    ivp->SetElement(0, 1); 
    }

  this->CompositeManagerProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------

void vtkSMIceTRenderModuleProxy::InteractiveRender()
{
  this->ChooseSuppressGeometryCollection();
  this->Superclass::InteractiveRender();
}

//-----------------------------------------------------------------------------

void vtkSMIceTRenderModuleProxy::StillRender()
{
  this->ChooseSuppressGeometryCollection();
  this->GetRenderWindow()->SetDesiredUpdateRate(5.0);
  this->ComputeReductionFactor(this->StillReductionFactor);

  this->Superclass::StillRender();
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::BeginStillRender()
{
  this->Superclass::BeginStillRender();
  // HACK to make the client use LOD when compositing.
  if (!this->LocalRender)
    {
    this->Helper->SetLODFlag(1);
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::EndStillRender()
{
  if (!this->LocalRender)
    {
    this->Helper->SetLODFlag(0);
    }
  this->Superclass::EndStillRender();
  
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::BeginInteractiveRender()
{
  this->Superclass::BeginInteractiveRender();
  // Force LOD on the client when Compositing (but not using LOD).
  if (!this->LocalRender && !this->GetUseLODDecision())
    {
    this->Helper->SetLODFlag(1);
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::EndInteractiveRender()
{
  // Reset force LOD on the client when Compositing (but not using LOD).
  if (!this->LocalRender && !this->GetUseLODDecision())
    {
    this->Helper->SetLODFlag(0);
    }
  this->Superclass::EndInteractiveRender();
}

//-----------------------------------------------------------------------------
int vtkSMIceTRenderModuleProxy::GetLocalRenderDecision(unsigned long mem, 
  int stillRender)
{
  if (this->GetSuppressGeometryCollectionDecision())
    {
    return 0;
    }
  if (!stillRender && this->GetUseLODDecision())
    {
    return 1; 
    }
  return this->Superclass::GetLocalRenderDecision(mem, stillRender);
}

//-----------------------------------------------------------------------------
int vtkSMIceTRenderModuleProxy::GetSuppressGeometryCollectionDecision()
{
  if (  this->GetTotalVisibleGeometryMemorySize()
      < this->CollectGeometryThreshold*1000)
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::ChooseSuppressGeometryCollection()
{
  int decision = this->GetSuppressGeometryCollectionDecision();

  vtkCollection* displays = this->GetDisplays();
  displays->InitTraversal();
  while (vtkObject *obj = displays->GetNextItemAsObject())
    {
    vtkSMIceTMultiDisplayProxy *pDisp
      = vtkSMIceTMultiDisplayProxy::SafeDownCast(obj);
    if (pDisp && pDisp->GetVisibilityCM())
      {
      // Just setting locally is fine.  Don't need to use properties.
      pDisp->SetSuppressGeometryCollection(decision);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CollectGeometryThreshold: "
     << this->CollectGeometryThreshold << endl;
  os << indent << "StillReductionFactor: "
     << this->StillReductionFactor << endl;
}
