// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRTravelGrabEgoStyleProxy.h"

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkVRQueue.h"

#include <iostream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRTravelGrabEgoStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRTravelGrabEgoStyleProxy::vtkSMVRTravelGrabEgoStyleProxy()
  : Superclass()
{
  this->AddTrackerRole("Tracker");
  this->AddButtonRole("Navigate");
  this->AddButtonRole("Reset");
  this->EnableNavigate = false;
  this->IsInitialRecorded = false;
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelGrabEgoStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableNavigate: " << this->EnableNavigate << endl;
  os << indent << "IsInitialRecorded: " << this->IsInitialRecorded << endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRTravelGrabEgoStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Cannot operate without a controlled proxy and property");
    return true;
  }

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->EnableNavigate && this->IsInitialRecorded)
  {
    // Subtract out the saved wand orientation and apply the result
    // to the saved model matrix
    vtkMatrix4x4::Multiply4x4(this->TrackerMatrix.GetPointer(),
      this->SavedInverseWandMatrix.GetPointer(), this->TransformMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->SavedPropertyMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    // Set the new matrix for the proxy.
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->TransformMatrix);

    this->ControlledProxy->UpdateVTKObjects();
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelGrabEgoStyleProxy::HandleButton(const vtkVREvent& event)
{
  // Make sure the user selected a property name on the chosen proxy, and
  // check the proxy too, just to be safe.
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Cannot operate without a controlled proxy and property");
    return;
  }

  this->IsInitialRecorded = false;

  std::string role = this->GetButtonRole(event.name);

  if (role == "Navigate")
  {
    this->EnableNavigate = event.data.button.state;
  }
  else if (role == "Reset" && event.data.button.state != 0)
  {
    // Reset all working matrices
    this->SavedPropertyMatrix->Identity();
    this->SavedInverseWandMatrix->Identity();
    this->TransformMatrix->Identity();
    this->TrackerMatrix->Identity();

    // Reset the controlled proxy/property
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->TransformMatrix);
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelGrabEgoStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role != "Tracker" || EnableNavigate == false)
  {
    return;
  }

  if (!this->IsInitialRecorded)
  {
    // If this is the first time in here after initially pressing the button,
    // just save the current model transform matrix and inverse wand matrix
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Get(*this->SavedPropertyMatrix->Element, 16);

    this->SavedInverseWandMatrix->DeepCopy(event.data.tracker.matrix);
    vtkMatrix4x4::Invert(
      this->SavedInverseWandMatrix.GetPointer(), this->SavedInverseWandMatrix.GetPointer());

    this->IsInitialRecorded = true;
  }

  // Store the latest tracker matrix, saving computation for Update()
  this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
}
