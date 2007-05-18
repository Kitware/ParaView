/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUniformGridVolumeRepresentationProxy.h"

#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUniformGridVolumeRepresentationProxy);
vtkCxxRevisionMacro(vtkSMUniformGridVolumeRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMUniformGridVolumeRepresentationProxy::vtkSMUniformGridVolumeRepresentationProxy()
{
  this->VolumeFixedPointRayCastMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;

  this->ExtractSelection = 0;
  this->SelectionGeometryFilter = 0;
  this->SelectionMapper = 0;
  this->SelectionLODMapper = 0;
  this->SelectionProp3D = 0;
  this->SelectionProperty = 0;

  // This representation supports selection.
  this->SetSelectionSupported(true);
}

//----------------------------------------------------------------------------
vtkSMUniformGridVolumeRepresentationProxy::~vtkSMUniformGridVolumeRepresentationProxy()
{
  this->VolumeFixedPointRayCastMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;

  this->ExtractSelection = 0;
  this->SelectionGeometryFilter = 0;
  this->SelectionMapper = 0;
  this->SelectionLODMapper = 0;
  this->SelectionProp3D = 0;
  this->SelectionProperty = 0;
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::GetSelectionVisibility()
{
  if (!this->Superclass::GetSelectionVisibility())
    {
    return false;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetProperty("Selection"));
  return (pp && pp->GetNumberOfProxies() > 0);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::UpdateSelectionPropVisibility()
{
  int visibility  = this->GetSelectionVisibility()? 1 : 0;
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SelectionProp3D->GetProperty("Visibility"));
  ivp->SetElement(0, visibility);
  this->SelectionProp3D->UpdateProperty("Visibility");
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->UpdateSelectionPropVisibility();
  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  renderView->AddPropToRenderer(this->VolumeActor);
  if (this->GetSelectionSupported())
    {
    renderView->AddPropToRenderer(this->SelectionProp3D);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->VolumeActor);
  renderView->RemovePropFromRenderer(this->SelectionProp3D);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_UNIFORM_GRID, vtkSMRenderViewProxy::VOLUME));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  this->SetStrategy(strategy);

  strategy->SetEnableLOD(false);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMPipelineRepresentationProxy::AddToView()).

  this->Connect(this->GetInputProxy(), strategy, "Input");
  this->Connect(strategy->GetOutput(), this->VolumeFixedPointRayCastMapper);

  // Initialize strategy for the selection pipeline.
  strategy.TakeReference(
    view->NewStrategy(VTK_POLY_DATA, vtkSMRenderViewProxy::SURFACE));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("Could not create strategy for selection pipeline. Disabling selection.");
    this->SetSelectionSupported(false);
    }
  else
    {
    this->SetStrategyForSelection(strategy);
    strategy->SetEnableLOD(true);
    strategy->UpdateVTKObjects();

    strategy->SetInput(this->SelectionGeometryFilter);
    this->Connect(strategy->GetOutput(), this->SelectionMapper);
    this->Connect(strategy->GetLODOutput(), this->SelectionLODMapper);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::BeginCreateVTKObjects(int numObjects)
{
  if (!this->Superclass::BeginCreateVTKObjects(numObjects))
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->VolumeFixedPointRayCastMapper = this->GetSubProxy(
    "VolumeFixedPointRayCastMapper");
  this->VolumeActor = this->GetSubProxy("VolumeActor");
  this->VolumeProperty = this->GetSubProxy("VolumeProperty");

  this->VolumeFixedPointRayCastMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeActor->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  // Initialize selection pipeline subproxies.
  this->ExtractSelection = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ExtractSelection"));
  this->SelectionGeometryFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("SelectionGeometryFilter"));
  this->SelectionMapper = this->GetSubProxy("SelectionMapper");
  this->SelectionLODMapper = this->GetSubProxy("SelectionLODMapper");
  this->SelectionProp3D = this->GetSubProxy("SelectionProp3D");
  this->SelectionProperty = this->GetSubProxy("SelectionProperty");

  this->ExtractSelection->SetServers(vtkProcessModule::DATA_SERVER);
  this->SelectionGeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->SelectionMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->SelectionLODMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->SelectionProp3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->SelectionProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::EndCreateVTKObjects(int numObjects)
{
  this->Connect(this->VolumeFixedPointRayCastMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeProperty, this->VolumeActor, "Property");

  // Setup selection pipeline connections.
  this->Connect(this->GetInputProxy(), this->ExtractSelection);
  this->Connect(this->ExtractSelection, this->SelectionGeometryFilter);
  this->Connect(this->SelectionMapper, this->SelectionProp3D, "Mapper");
  this->Connect(this->SelectionLODMapper, this->SelectionProp3D, "LODMapper");
  this->Connect(this->SelectionProperty, this->SelectionProp3D, "Property");

  // Selection prop is not pickable.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SelectionProp3D->GetProperty("Pickable"));
  ivp->SetElement(0, 0);
  this->SelectionProp3D->UpdateProperty("Pickable");

  return this->Superclass::EndCreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::GetSelectableProps(
  vtkCollection* collection)
{
  collection->AddItem(this->VolumeActor);
}

//----------------------------------------------------------------------------
static void vtkSMUniformGridVolumeRepresentationProxyAddSourceIDs(
  vtkSelection* sel, vtkClientServerID propId, vtkClientServerID sourceId,
  vtkClientServerID originalSourceId)
{
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkSMUniformGridVolumeRepresentationProxyAddSourceIDs(sel->GetChild(cc),
      propId, sourceId, originalSourceId);
    }

  vtkInformation* properties = sel->GetProperties();
  if (!properties->Has(vtkSelection::PROP_ID()) || 
    propId.ID != static_cast<vtkTypeUInt32>(
      properties->Get(vtkSelection::PROP_ID())))
    {
    return;
    }
  properties->Set(vtkSelection::SOURCE_ID(), sourceId.ID);
  properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
    originalSourceId.ID);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::ConvertSurfaceSelectionToVolumeSelection(
  vtkSelection* selInput, vtkSelection* selOutput)
{
  // Process selInput to add SOURCE_ID() and ORIGINAL_SOURCE_ID() keys to it to
  // help the converter in converting the selection.

  vtkClientServerID originalSourceId;
  vtkSMProxy* input = this->GetInputProxy();
  if (vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(input))
    {
    // For compound proxies, the selected proxy is the consumed proxy.
    originalSourceId = cp->GetConsumableProxy()->GetID(0);
    }
  else
    {
    originalSourceId = input->GetID(0);
    }

  vtkSMUniformGridVolumeRepresentationProxyAddSourceIDs(selInput,
    this->VolumeActor->GetID(0), originalSourceId, originalSourceId);
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
    this->ConnectionID, selInput, selOutput);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


