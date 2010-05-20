/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMPropRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPropRepresentationProxy::vtkSMPropRepresentationProxy()
{
  this->SelectionRepresentation = 0;
  this->SelectionVisibility = 0;

  // This link is used to link the properties of the representation prop to the 
  // properties of the selection prop so that they appear to be tranformed
  // similarly.
  this->SelectionPropLink = vtkSMProxyLink::New();
  this->SelectionPropLink->AddException("EnableLOD");
  this->SelectionPropLink->AddException("LODMapper");
  this->SelectionPropLink->AddException("Mapper");
  this->SelectionPropLink->AddException("Pickable");
  this->SelectionPropLink->AddException("Property");
  this->SelectionPropLink->AddException("Texture");
  this->SelectionPropLink->AddException("Visibility");
}

//----------------------------------------------------------------------------
vtkSMPropRepresentationProxy::~vtkSMPropRepresentationProxy()
{
  this->SelectionPropLink->Delete();
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::SetViewInformation(vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);
  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->SetViewInformation(info);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::SetVisibility(int visible)
{
  if (this->SelectionRepresentation && !visible)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->SelectionRepresentation->GetProperty("Visibility"));
    ivp->SetElement(0, visible);
    this->SelectionRepresentation->UpdateProperty("Visibility");
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
void vtkSMPropRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->SelectionRepresentation)
    {
    // First update the selection representation visibility.
    int sel_visibility = (this->GetVisibility() && this->SelectionVisibility);
   
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->SelectionRepresentation->GetProperty("Visibility"));
    ivp->SetElement(0, sel_visibility);
    this->SelectionRepresentation->UpdateProperty("Visibility");

    // Now update the selection representation.
    this->SelectionRepresentation->Update(view);  
    }

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::AddToView(vtkSMViewProxy* view)
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

  if (this->GetSubProxy("Prop3D"))
    {
    renderView->AddPropToRenderer(this->GetSubProxy("Prop3D"));
    }
  if (this->GetSubProxy("Prop2D"))
    {
    renderView->AddPropToRenderer(this->GetSubProxy("Prop2D"));
    }

  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->AddToView(view);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (this->GetSubProxy("Prop3D"))
    {
    renderView->RemovePropFromRenderer(this->GetSubProxy("Prop3D"));
    }
  if (this->GetSubProxy("Prop2D"))
    {
    renderView->RemovePropFromRenderer(this->GetSubProxy("Prop2D"));
    }

  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->RemoveFromView(view);
    }
  return this->Superclass::RemoveFromView(view);
}


//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::LinkSelectionProp(vtkSMProxy* prop)
{
  if (this->SelectionRepresentation && prop)
    {
    this->SelectionPropLink->AddLinkedProxy(prop, vtkSMLink::INPUT);
    }
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->SelectionRepresentation = 
    vtkSMDataRepresentationProxy::SafeDownCast(this->GetSubProxy(
        "SelectionRepresentation"));
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::EndCreateVTKObjects()
{
  if (this->SelectionRepresentation)
    {
    // Setup selection pipeline connections.
    this->Connect(this->GetInputProxy(), this->SelectionRepresentation, 
      "Input", this->OutputPort);

    // Link actor properties with the seleciton actor so that actor
    // transformations work.
    this->SelectionPropLink->AddLinkedProxy(
      vtkSMSelectionRepresentationProxy::SafeDownCast(
        this->SelectionRepresentation)->GetProp3D(),
      vtkSMLink::OUTPUT);
    }
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::GetActiveStrategies(
  vtkSMRepresentationStrategyVector& activeStrategies)
{
  this->Superclass::GetActiveStrategies(activeStrategies);
  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->GetActiveStrategies(activeStrategies);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);
  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->SetUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::SetUseViewUpdateTime(bool use)
{
  this->Superclass::SetUseViewUpdateTime(use);
  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->SetUseViewUpdateTime(use);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->SetViewUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectionVisibility: " << this->SelectionVisibility << endl;
}


