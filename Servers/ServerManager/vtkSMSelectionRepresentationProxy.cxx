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

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkLabeledDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMDataLabelRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSelectionRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSelectionRepresentationProxy, "1.9");
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

  this->LabelRepresentation = 0;

  this->SetSelectionSupported(false);
}

//----------------------------------------------------------------------------
vtkSMSelectionRepresentationProxy::~vtkSMSelectionRepresentationProxy()
{
  this->LabelRepresentation = 0;
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetViewInformation(vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);
  if (this->LabelRepresentation)
    {
    this->LabelRepresentation->SetViewInformation(info);
    }
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
  this->LabelRepresentation->AddToView(view);
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
  this->LabelRepresentation->RemoveFromView(view);
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

  this->LabelRepresentation = vtkSMDataLabelRepresentationProxy::SafeDownCast(
    this->GetSubProxy("LabelRepresentation"));

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
  this->Connect(this->GetInputProxy(), this->ExtractSelection, 
    "Input", this->OutputPort);
  this->Connect(this->EmptySelectionSource, this->ExtractSelection, 
    "Selection");
  this->Connect(this->ExtractSelection, this->GeometryFilter);

  this->Connect(this->ExtractSelection, this->LabelRepresentation);
  
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");

  // Selection prop is not pickable.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Prop3D->GetProperty("Pickable"));
  ivp->SetElement(0, 0);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ScalarVisibility"));
  ivp->SetElement(0, 0);
  this->Prop3D->UpdateVTKObjects();

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Property->GetProperty("Ambient"));
  dvp->SetElement(0, 1.0);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Property->GetProperty("Diffuse"));
  dvp->SetElement(0, 0.0);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Property->GetProperty("Specular"));
  dvp->SetElement(0, 0.0);
  this->Property->UpdateVTKObjects();

  // Set LabelRepresentation LabelMode default to FieldData
  if(this->LabelRepresentation)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LabelRepresentation->GetProperty("PointLabelMode"));
    ivp->SetElement(0, VTK_LABEL_FIELD_DATA);

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LabelRepresentation->GetProperty("CellLabelMode"));
    ivp->SetElement(0, VTK_LABEL_FIELD_DATA);
    this->LabelRepresentation->UpdateVTKObjects();
    }

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

  if(this->LabelRepresentation && 
    this->LabelRepresentation->GetVisibility())
  {
  this->LabelRepresentation->Update(view);
  }
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);
  if (this->LabelRepresentation)
    {
    this->LabelRepresentation->SetUpdateTime(time);
    }

  if (this->ViewInformation->Has(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())
    && this->ViewInformation->Get(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())==1)
    {
    // We must use LOD on client side.
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->Prop3D->GetID()
            << "SetEnableLOD" << 1
            << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT, stream);   
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
  if (this->EmptySelectionSource && this->ExtractSelection)
    {
    this->Connect(this->EmptySelectionSource, this->ExtractSelection, "Selection");
    }
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetVisibility(int visible)
{
  if (this->LabelRepresentation && !visible)
    {
    //this->SetPointLabelVisibility(visible);
    //this->SetCellLabelVisibility(visible);
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LabelRepresentation->GetProperty("PointLabelVisibility"));
    ivp->SetElement(0, visible);
    this->LabelRepresentation->UpdateProperty("PointLabelVisibility");
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LabelRepresentation->GetProperty("CellLabelVisibility"));
    ivp->SetElement(0, visible);    
    this->LabelRepresentation->UpdateProperty("CellLabelVisibility");
    }

  vtkSMProxy* prop3D = this->GetSubProxy("Prop3D");
  vtkSMProxy* prop2D = this->GetSubProxy("Prop2D");

  if (prop3D)
  {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop3D->GetProperty("Visibility"));
    ivp->SetElement(0, visible);
    prop3D->UpdateProperty("Visibility");
  }

  if (prop2D)
  {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop2D->GetProperty("Visibility"));
    ivp->SetElement(0, visible);
    prop2D->UpdateProperty("Visibility");
  }
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Prop3D: " << this->Prop3D << endl;
}


