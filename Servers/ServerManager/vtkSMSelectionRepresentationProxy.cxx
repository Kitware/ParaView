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
#include "vtkProp3D.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMDataLabelRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSelectionRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSelectionRepresentationProxy::vtkSMSelectionRepresentationProxy()
{
  this->UserRequestedVisibility = true;
  this->PointLabelVisibility = 1;
  this->CellLabelVisibility = 1;

  this->GeometryFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;

  this->LabelRepresentation = 0;
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
    vtkErrorMacro("Could not create strategy for selection pipeline.");
    return false;
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
  this->GeometryFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("GeometryFilter"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->LODMapper = this->GetSubProxy("LODMapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");

  this->LabelRepresentation = vtkSMDataLabelRepresentationProxy::SafeDownCast(
    this->GetSubProxy("LabelRepresentation"));

  this->GeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LODMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::EndCreateVTKObjects()
{
  // Setup selection pipeline connections.
  vtkSMSourceProxy* input = this->GetInputProxy();
  // Ensure that the source proxy has created extract selection filters.
  input->CreateSelectionProxies();

  vtkSMSourceProxy* esProxy = input->GetSelectionOutput(this->OutputPort);
  if (!esProxy)
    {
    vtkErrorMacro("Input proxy does not support selection extraction.");
    return false;
    }

  this->Connect(esProxy, this->GeometryFilter);

  this->Connect(esProxy, this->LabelRepresentation);
  
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
  this->UpdateVisibility();

  this->Superclass::Update(view);

  if (this->ViewInformation->Has(vtkSMRenderViewProxy::USE_LOD()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->Prop3D->GetProperty("EnableLOD"));
    ivp->SetElement(0, 
      this->ViewInformation->Get(vtkSMRenderViewProxy::USE_LOD()));
    this->Prop3D->UpdateProperty("EnableLOD");
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

  if(this->LabelRepresentation && this->LabelRepresentation->GetVisibility())
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
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetUseViewUpdateTime(bool use)
{
  this->Superclass::SetUseViewUpdateTime(use);
  if (this->LabelRepresentation)
    {
    this->LabelRepresentation->SetUseViewUpdateTime(use);
    }

}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  if (this->LabelRepresentation)
    {
    this->LabelRepresentation->SetViewUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::SetVisibility(int visible)
{
  // Selection visibility if a function of the user requested visibility and
  // whether any selection input is available.
  this->UserRequestedVisibility = (visible!=0);
  this->UpdateVisibility();
}

//----------------------------------------------------------------------------
bool vtkSMSelectionRepresentationProxy::GetVisibility()
{
  vtkSMSourceProxy* input = this->GetInputProxy();
  return (this->UserRequestedVisibility && input != 0 && input->GetSelectionInput(
      this->OutputPort) != 0);
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::UpdateVisibility()
{
  bool visible = this->GetVisibility();
  if (this->LabelRepresentation)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LabelRepresentation->GetProperty("PointLabelVisibility"));
    ivp->SetElement(0, ((visible && this->PointLabelVisibility)? 1 : 0));
    this->LabelRepresentation->UpdateProperty("PointLabelVisibility");
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LabelRepresentation->GetProperty("CellLabelVisibility"));
    ivp->SetElement(0, ((visible && this->CellLabelVisibility)? 1 : 0));
    this->LabelRepresentation->UpdateProperty("CellLabelVisibility");
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Prop3D->GetProperty("Visibility"));
  ivp->SetElement(0, visible? 1 : 0);
  this->Prop3D->UpdateProperty("Visibility");
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Prop3D: " << this->Prop3D << endl;
  os << indent << "PointLabelVisibility: " << this->PointLabelVisibility << endl;
  os << indent << "CellLabelVisibility: " << this->CellLabelVisibility << endl;
}


