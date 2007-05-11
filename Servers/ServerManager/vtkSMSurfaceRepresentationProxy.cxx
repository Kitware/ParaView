/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSurfaceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSurfaceRepresentationProxy.h"

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSurfaceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSurfaceRepresentationProxy, "1.4");
//----------------------------------------------------------------------------
vtkSMSurfaceRepresentationProxy::vtkSMSurfaceRepresentationProxy()
{
  this->GeometryFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;

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
vtkSMSurfaceRepresentationProxy::~vtkSMSurfaceRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::GetSelectionVisibility()
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
void vtkSMSurfaceRepresentationProxy::UpdateSelectionPropVisibility()
{
  int visibility  = this->GetSelectionVisibility()? 1 : 0;
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SelectionProp3D->GetProperty("Visibility"));
  ivp->SetElement(0, visibility);
  this->SelectionProp3D->UpdateProperty("Visibility");
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->UpdateSelectionPropVisibility();
  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::AddToView(vtkSMViewProxy* view)
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

  renderView->AddPropToRenderer(this->Prop3D);
  if (this->GetSelectionSupported())
    {
    renderView->AddPropToRenderer(this->SelectionProp3D);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->Prop3D);
  renderView->RemovePropFromRenderer(this->SelectionProp3D);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_POLY_DATA, vtkSMRenderViewProxy::SURFACE));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  this->SetStrategy(strategy);

  strategy->SetEnableLOD(true);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMPipelineRepresentationProxy::AddToView()).

  strategy->SetInput(this->GeometryFilter);
  this->Connect(strategy->GetOutput(), this->Mapper);
  this->Connect(strategy->GetLODOutput(), this->LODMapper);

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
bool vtkSMSurfaceRepresentationProxy::BeginCreateVTKObjects(int numObjects)
{
  if (!this->Superclass::BeginCreateVTKObjects(numObjects))
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->GeometryFilter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("GeometryFilter"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->LODMapper = this->GetSubProxy("LODMapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");

  this->GeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LODMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
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
bool vtkSMSurfaceRepresentationProxy::EndCreateVTKObjects(int numObjects)
{
  this->Connect(this->GetInputProxy(), this->GeometryFilter, "Input");
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");

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
void vtkSMSurfaceRepresentationProxy::GetSelectableProps(
  vtkCollection* collection)
{
  collection->AddItem(this->Prop3D);
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::ConvertSurfaceSelectionToVolumeSelection(
  vtkSelection* input, vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
    this->ConnectionID, input, output);
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


