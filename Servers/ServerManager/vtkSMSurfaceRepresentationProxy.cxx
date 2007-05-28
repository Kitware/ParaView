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
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSurfaceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSurfaceRepresentationProxy, "1.10");
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
bool vtkSMSurfaceRepresentationProxy::GetOrderedCompositingNeeded()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Opacity"));
  return (dvp && dvp->GetElement(0) < 1.0);
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
    view->NewStrategy(VTK_POLY_DATA));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  this->AddStrategy(strategy);

  strategy->SetEnableLOD(true);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  this->Connect(this->GeometryFilter, strategy);
  this->Connect(strategy->GetOutput(), this->Mapper);
  this->Connect(strategy->GetLODOutput(), this->LODMapper);

  // Initialize strategy for the selection pipeline.
  strategy.TakeReference(
    view->NewStrategy(VTK_POLY_DATA));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("Could not create strategy for selection pipeline. Disabling selection.");
    this->SetSelectionSupported(false);
    }
  else
    {
    this->AddStrategyForSelection(strategy);
    strategy->SetEnableLOD(true);
    strategy->SetEnableCaching(false); // no cache needed for selection.
    strategy->UpdateVTKObjects();

    this->Connect(this->SelectionGeometryFilter, strategy);
    this->Connect(strategy->GetOutput(), this->SelectionMapper);
    this->Connect(strategy->GetLODOutput(), this->SelectionLODMapper);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
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
bool vtkSMSurfaceRepresentationProxy::EndCreateVTKObjects()
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

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
static void vtkSMSurfaceRepresentationProxyAddSourceIDs(
  vtkSelection* sel, vtkClientServerID propId, vtkClientServerID sourceId,
  vtkClientServerID originalSourceId)
{
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkSMSurfaceRepresentationProxyAddSourceIDs(sel->GetChild(cc),
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
void vtkSMSurfaceRepresentationProxy::ConvertSurfaceSelectionToVolumeSelection(
  vtkSelection* selInput, vtkSelection* selOutput)
{
  // Process selInput to add SOURCE_ID() and ORIGINAL_SOURCE_ID() keys to it to
  // help the converter in converting the selection.

  vtkClientServerID sourceId = this->GeometryFilter->GetID();
  vtkClientServerID originalSourceId;
  vtkSMProxy* input = this->GetInputProxy();
  if (vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(input))
    {
    // For compound proxies, the selected proxy is the consumed proxy.
    originalSourceId = cp->GetConsumableProxy()->GetID();
    }
  else
    {
    originalSourceId = input->GetID();
    }

  vtkSMSurfaceRepresentationProxyAddSourceIDs(selInput,
    this->Prop3D->GetID(), sourceId, originalSourceId);
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
    this->ConnectionID, selInput, selOutput);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSurfaceRepresentationProxy::ConvertSelection(
  vtkSelection* surfaceSel)
{
  if (!this->GetVisibility())
    {
    return 0;
    }

  vtkSmartPointer<vtkSelection> mySelection = 
    vtkSmartPointer<vtkSelection>::New();
  mySelection->GetProperties()->Copy(surfaceSel->GetProperties(), 0);

  unsigned int numChildren = surfaceSel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkSelection* child = surfaceSel->GetChild(cc);
    vtkInformation* properties = child->GetProperties();
    if (!properties->Has(vtkSelection::PROP_ID()))
      {
      continue;
      }
    vtkClientServerID propId;
    propId.ID = static_cast<vtkTypeUInt32>(properties->Get(
        vtkSelection::PROP_ID()));
    if (propId == this->Prop3D->GetID())
      {
      vtkSelection* myChild = vtkSelection::New();
      myChild->ShallowCopy(child);
      mySelection->AddChild(myChild);
      myChild->Delete();
      }
    }

  if (mySelection->GetNumberOfChildren() == 0)
    {
    return 0;
    }

  // Convert surface selection to volume selection.
  vtkSmartPointer<vtkSelection> volSelection = 
    vtkSmartPointer<vtkSelection>::New();
  this->ConvertSurfaceSelectionToVolumeSelection(surfaceSel, volSelection);

  // Create a selection source for the selection.
  vtkSMProxy* selectionSource = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      this->ConnectionID, volSelection);
  
  return selectionSource;
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


