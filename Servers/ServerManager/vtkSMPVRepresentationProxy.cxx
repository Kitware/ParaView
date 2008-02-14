/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSurfaceRepresentationProxy.h"

inline void vtkSMPVRepresentationProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    }
  proxy->UpdateProperty(pname);
}

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
vtkCxxRevisionMacro(vtkSMPVRepresentationProxy, "1.16");
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->Representation = VTK_SURFACE;
  this->SurfaceRepresentation = 0;
  this->VolumeRepresentation = 0;
  this->OutlineRepresentation = 0;
  this->CubeAxesRepresentation = 0;
  this->CubeAxesVisibility = 0;
  this->ActiveRepresentation = 0;
  this->SliceRepresentation = 0;
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetViewInformation(vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);
  if (this->SurfaceRepresentation)
    {
    this->SurfaceRepresentation->SetViewInformation(info);
    }

  if (this->OutlineRepresentation)
    {
    this->OutlineRepresentation->SetViewInformation(info);
    }

  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->SetViewInformation(info);
    }

  if (this->CubeAxesRepresentation)
    {
    this->CubeAxesRepresentation->SetViewInformation(info);
    }

  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->SetViewInformation(info);
    }

}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::EndCreateVTKObjects()
{
  this->SurfaceRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("SurfaceRepresentation"));
  this->VolumeRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("VolumeRepresentation"));
  this->SliceRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("SliceRepresentation"));
  this->OutlineRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("OutlineRepresentation"));
  this->CubeAxesRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("CubeAxesRepresentation"));

  this->Connect(this->GetInputProxy(), this->SurfaceRepresentation,
    "Input", this->OutputPort);
  this->Connect(this->GetInputProxy(), this->OutlineRepresentation, 
    "Input", this->OutputPort);
  this->Connect(this->GetInputProxy(), this->CubeAxesRepresentation,
    "Input", this->OutputPort);

  vtkSMPVRepresentationProxySetInt(this->SurfaceRepresentation, "Visibility", 0);
  vtkSMPVRepresentationProxySetInt(this->OutlineRepresentation, "Visibility", 0);
  vtkSMPVRepresentationProxySetInt(this->CubeAxesRepresentation, "Visibility", 0);

  if (this->VolumeRepresentation)
    {
    this->Connect(this->GetInputProxy(), this->VolumeRepresentation,
      "Input", this->OutputPort);
    vtkSMPVRepresentationProxySetInt(this->VolumeRepresentation, "Visibility", 0);
    }

  if (this->SliceRepresentation)
    {
    this->Connect(this->GetInputProxy(), this->SliceRepresentation,
      "Input", this->OutputPort);
    vtkSMPVRepresentationProxySetInt(this->SliceRepresentation, "Visibility",
      0);
    }


  // Fire start/end events fired by the representations so that the world knows
  // that the representation has been updated,
  vtkCommand* observer = this->GetObserver();
  this->SurfaceRepresentation->AddObserver(vtkCommand::StartEvent, observer);
  this->SurfaceRepresentation->AddObserver(vtkCommand::EndEvent, observer);

  this->OutlineRepresentation->AddObserver(vtkCommand::StartEvent, observer);
  this->OutlineRepresentation->AddObserver(vtkCommand::EndEvent, observer);

  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->AddObserver(vtkCommand::StartEvent, observer);
    this->VolumeRepresentation->AddObserver(vtkCommand::EndEvent, observer);
    }

  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->AddObserver(vtkCommand::StartEvent, observer);
    this->SliceRepresentation->AddObserver(vtkCommand::EndEvent, observer);
    }


  // Setup the ActiveRepresentation pointer.
  int repr = this->Representation;
  this->Representation = -1;
  this->SetRepresentation(repr);

  this->LinkSelectionProp(vtkSMSurfaceRepresentationProxy::SafeDownCast(
      this->SurfaceRepresentation)->GetProp3D());

  // This will pass the ViewInformation to all the representations.
  this->SetViewInformation(this->ViewInformation);

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  this->SurfaceRepresentation->AddToView(view);
  this->OutlineRepresentation->AddToView(view);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->AddToView(view);
    }
  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->AddToView(view);
    }
  this->CubeAxesRepresentation->AddToView(view);

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  this->SurfaceRepresentation->RemoveFromView(view);
  this->OutlineRepresentation->RemoveFromView(view);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->RemoveFromView(view);
    }
  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->RemoveFromView(view);
    }
  this->CubeAxesRepresentation->RemoveFromView(view);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetRepresentation(int repr)
{
  if (this->Representation != repr)
    {
    this->Representation = repr;
    if (this->ActiveRepresentation)
      {
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 0);
      }

    switch (this->Representation)
      {
    case OUTLINE:
      this->ActiveRepresentation = this->OutlineRepresentation;
      break;

    case VOLUME:
      if (this->VolumeRepresentation)
        {
        this->ActiveRepresentation = this->VolumeRepresentation;
        break;
        }
      else
        {
        vtkErrorMacro("Volume representation not supported.");
        this->SetRepresentation(OUTLINE);
        return;
        }

    case SLICE:
      if (this->SliceRepresentation)
        {
        this->ActiveRepresentation = this->SliceRepresentation;
        break;
        }
      else
        {
        vtkErrorMacro("Slice representation not supported.");
        this->SetRepresentation(OUTLINE);
        return;
        }

    case POINTS:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, 
        "Representation", VTK_POINTS);
      break;

    case WIREFRAME:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, 
        "Representation", VTK_WIREFRAME);
      break;

    case SURFACE_WITH_EDGES:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, 
        "Representation", VTK_SURFACE_WITH_EDGES);
      break;

    case SURFACE:
    default:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, 
        "Representation", VTK_SURFACE);
      }

    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      this->GetVisibility());
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetVisibility(int visible)
{
  if (this->ActiveRepresentation)
    {
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      visible);
    }
  vtkSMPVRepresentationProxySetInt(this->CubeAxesRepresentation, "Visibility",
    visible && this->CubeAxesVisibility);
  this->CubeAxesRepresentation->UpdateVTKObjects();
     
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetCubeAxesVisibility(int visible)
{
  this->CubeAxesVisibility = visible;
  vtkSMPVRepresentationProxySetInt(this->CubeAxesRepresentation, "Visibility",
    visible && this->GetVisibility());
  this->CubeAxesRepresentation->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->Update(view);
    }
  this->CubeAxesRepresentation->Update(view);

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* 
vtkSMPVRepresentationProxy::GetRepresentedDataInformation(bool update)
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetRepresentedDataInformation(update);
    }

  return this->Superclass::GetRepresentedDataInformation(update);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::UpdateRequired()
{
  if (this->ActiveRepresentation)
    {
    if (this->ActiveRepresentation->UpdateRequired())
      {
      return true;
      }
    }

  if (this->CubeAxesRepresentation->UpdateRequired())
    {
    return true;
    }

  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);
  this->SurfaceRepresentation->SetUpdateTime(time);
  this->OutlineRepresentation->SetUpdateTime(time);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->SetUpdateTime(time);
    }
  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->SetUpdateTime(time);
    }
  this->CubeAxesRepresentation->SetUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetUseViewUpdateTime(bool use)
{
  this->Superclass::SetUseViewUpdateTime(use);

  this->SurfaceRepresentation->SetUseViewUpdateTime(use);
  this->OutlineRepresentation->SetUseViewUpdateTime(use);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->SetUseViewUpdateTime(use);
    }
  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->SetUseViewUpdateTime(use);
    }
  this->CubeAxesRepresentation->SetUseViewUpdateTime(use);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  this->SurfaceRepresentation->SetViewUpdateTime(time);
  this->OutlineRepresentation->SetViewUpdateTime(time);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->SetViewUpdateTime(time);
    }
  if (this->SliceRepresentation)
    {
    this->SliceRepresentation->SetViewUpdateTime(time);
    }
  this->CubeAxesRepresentation->SetViewUpdateTime(time);
}


//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this && this->ActiveRepresentation)
    {
    this->ActiveRepresentation->MarkModified(modifiedProxy);
    }

  this->Superclass::MarkModified(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::GetActiveStrategies(
  vtkSMRepresentationStrategyVector& activeStrategies)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->GetActiveStrategies(activeStrategies);
    }

  this->Superclass::GetActiveStrategies(activeStrategies);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::ConvertSelection(vtkSelection* input)
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->ConvertSelection(input);
    }

  return this->Superclass::ConvertSelection(input);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetOrderedCompositingNeeded()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetOrderedCompositingNeeded();
    }

  return this->Superclass::GetOrderedCompositingNeeded();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::GetProcessedConsumer()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetProcessedConsumer();
    }

  return this->Superclass::GetProcessedConsumer();
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HasVisibleProp3D(vtkProp3D* prop)
{
  if(!prop)
  {
    return false;
  }

  if(this->Superclass::HasVisibleProp3D(prop))
  {
    return true;
  }

  if (this->GetVisibility() && this->ActiveRepresentation)
    {
    vtkSMPropRepresentationProxy* repr = 
      vtkSMPropRepresentationProxy::SafeDownCast(this->ActiveRepresentation);
    if (repr && repr->HasVisibleProp3D(prop))
      {
      return true;
      }
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Representation: " ;
  switch (this->Representation)
    {
  case SURFACE:
    os << "Surface";
    break;

  case WIREFRAME:
    os << "Wireframe";
    break;

  case POINTS:
    os << "Points";
    break;

  case OUTLINE:
    os << "Outline";
    break;

  case VOLUME:
    os << "Volume";
    break;

  default:
    os << "(unknown)";
    }
  os << endl;
}


