/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionRepresentationProxy.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSelectionRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSelectionRepresentationProxy, "1.3");
//----------------------------------------------------------------------------
vtkSMSelectionRepresentationProxy::vtkSMSelectionRepresentationProxy()
{
  this->ExtractSelection = 0;
  this->GeometryFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
  this->EmptySelectionSource = 0;

  this->SetSelectionSupported(false);
}

//----------------------------------------------------------------------------
vtkSMSelectionRepresentationProxy::~vtkSMSelectionRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::AddToView(vtkSMViewProxy* view)
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
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->Prop3D);
  return this->Superclass::RemoveFromView(view);
}


//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;

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

    this->Connect(this->GeometryFilter, strategy);
    this->Connect(strategy->GetOutput(), this->Mapper);
    this->Connect(strategy->GetLODOutput(), this->LODMapper);

    strategy->SetEnableLOD(true);
    strategy->SetEnableCaching(false); // no cache needed for selection.
    strategy->UpdateVTKObjects();

    this->AddStrategy(strategy);
    }

  return true;
}


//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Initialize selection pipeline subproxies.
  this->ExtractSelection = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ExtractSelection"));
  this->GeometryFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("GeometryFilter"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->LODMapper = this->GetSubProxy("LODMapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");
  this->EmptySelectionSource = this->GetSubProxy("EmptySelectionSource");

  this->ExtractSelection->SetServers(vtkProcessModule::DATA_SERVER);
  this->GeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LODMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->EmptySelectionSource->SetServers(vtkProcessModule::DATA_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::EndCreateVTKObjects()
{
  // Setup selection pipeline connections.
  this->Connect(this->GetInputProxy(), this->ExtractSelection);
  this->Connect(this->EmptySelectionSource, this->ExtractSelection, 
    "Selection");
  this->Connect(this->ExtractSelection, this->GeometryFilter);
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");

  // Selection prop is not pickable.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Prop3D->GetProperty("Pickable"));
  ivp->SetElement(0, 0);
  this->Prop3D->UpdateProperty("Pickable");

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);

  if (this->ViewInformation->Has(vtkSMRenderViewProxy::USE_LOD()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->Prop3D->GetProperty("EnableLOD"));
    ivp->SetElement(0, 
      this->ViewInformation->Get(vtkSMRenderViewProxy::USE_LOD()));
    this->Prop3D->UpdateProperty("EnableLOD");
    }
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetSelection(vtkSMSourceProxy* selection)
{
  this->Connect(selection, this->ExtractSelection, "Selection");
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::CleanSelectionInput()
{
  this->Connect(this->EmptySelectionSource, this->ExtractSelection, "Selection");
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Prop3D: " << this->Prop3D << endl;
}


