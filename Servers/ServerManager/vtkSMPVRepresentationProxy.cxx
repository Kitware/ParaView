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
vtkCxxRevisionMacro(vtkSMPVRepresentationProxy, "1.3");
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->Representation = VTK_SURFACE;
  this->SurfaceRepresentation = 0;
  this->VolumeRepresentation = 0;
  this->OutlineRepresentation = 0;

  this->ActiveRepresentation = 0;
  this->SelectionVisibility = 1;

  this->SetSelectionSupported(true);
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
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::EndCreateVTKObjects()
{
  this->SurfaceRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("SurfaceRepresentation"));
  this->VolumeRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("VolumeRepresentation"));
  this->OutlineRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("OutlineRepresentation"));

  this->Connect(this->GetInputProxy(), this->SurfaceRepresentation);
  this->Connect(this->GetInputProxy(), this->OutlineRepresentation);

  vtkSMPVRepresentationProxySetInt(this->SurfaceRepresentation, "Visibility", 0);
  vtkSMPVRepresentationProxySetInt(this->OutlineRepresentation, "Visibility", 0);


  this->SurfaceRepresentation->SetSelectionSupported(false);
  this->OutlineRepresentation->SetSelectionSupported(false);
  if (this->VolumeRepresentation)
    {
    this->Connect(this->GetInputProxy(), this->VolumeRepresentation);
    vtkSMPVRepresentationProxySetInt(this->VolumeRepresentation, "Visibility", 0);
    this->VolumeRepresentation->SetSelectionSupported(false);
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
        }

    case POINTS:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Representation", VTK_POINTS);
      break;

    case WIREFRAME:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Representation", VTK_WIREFRAME);
      break;

    case SURFACE:
    default:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Representation", VTK_SURFACE);
      }

    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      this->GetVisibility());
    this->Modified();
    // TODO: Do we need to call MarkModified();
    this->MarkModified(0);
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
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPVRepresentationProxy::GetDisplayedDataInformation()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetDisplayedDataInformation();
    }

  return this->Superclass::GetDisplayedDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPVRepresentationProxy::GetFullResDataInformation()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetFullResDataInformation();
    }

  return this->Superclass::GetFullResDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->Update(view);
    }

  this->Superclass::Update(view);
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
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


