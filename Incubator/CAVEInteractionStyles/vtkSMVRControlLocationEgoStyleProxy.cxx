// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRControlLocationEgoStyleProxy.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRControlLocationEgoStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRControlLocationEgoStyleProxy::vtkSMVRControlLocationEgoStyleProxy()
  : Superclass()
{
  this->AddButtonRole("Grab");
  this->AddButtonRole("Reset");
  this->AddTrackerRole("Tracker");
  this->EnableNavigate = false;
  this->IsInitialRecorded = false;
  this->FirstUpdate = true;
  this->OriginalOrigin[0] = 0;
  this->OriginalOrigin[1] = 0;
  this->OriginalOrigin[2] = 0;
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationEgoStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableNavigate: " << this->EnableNavigate << endl;
  os << indent << "IsInitialRecorded: " << this->IsInitialRecorded << endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlLocationEgoStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return true;
  }

  // Save the original property value to be used as the "Reset" value
  if (this->FirstUpdate)
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Get(this->OriginalOrigin, 3);
    this->FirstUpdate = false;
  }

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->EnableNavigate && this->IsInitialRecorded)
  {
    vtkMatrix4x4::Multiply4x4(this->TrackerMatrix.GetPointer(),
      this->SavedInverseWandMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    double point[4];
    point[0] = this->Origin[0];
    point[1] = this->Origin[1];
    point[2] = this->Origin[2];
    point[3] = 1.0;

    // Update the point, accounting for any travel represented by the
    // model transform matrix.
    this->SavedModelMatrix->MultiplyPoint(point, point);
    this->TransformMatrix->MultiplyPoint(point, point);
    this->SavedInverseModelMatrix->MultiplyPoint(point, point);

    // Set the updated 3-tuple property on the proxy.
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Set(point, 3);

    this->ControlledProxy->UpdateVTKObjects();
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationEgoStyleProxy::HandleButton(const vtkVREvent& event)
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return;
  }

  this->IsInitialRecorded = false;

  std::string role = this->GetButtonRole(event.name);

  if (role == "Grab")
  {
    this->EnableNavigate = event.data.button.state;
  }
  else if (role == "Reset" && event.data.button.state == 1)
  {
    // Reset all working matrices to identity
    this->SavedModelMatrix->Identity();
    this->SavedInverseModelMatrix->Identity();
    this->SavedInverseWandMatrix->Identity();
    this->TrackerMatrix->Identity();
    this->TransformMatrix->Identity();

    // Reset stored origin ivar
    for (size_t i = 0; i < 3; ++i)
    {
      this->Origin[i] = this->OriginalOrigin[i];
    }

    // Reset controlled proxy/property
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(&(this->OriginalOrigin[0]), 3);
    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationEgoStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role != "Tracker" || !this->EnableNavigate)
  {
    return;
  }

  vtkCamera* camera = vtkSMVRInteractorStyleProxy::GetActiveCamera();
  if (!camera)
  {
    vtkWarningMacro(<< " HandleTracker: Cannot grab active camera.");
    return;
  }

  if (!this->IsInitialRecorded)
  {
    // save the model transform matrix and its inverse
    this->SavedModelMatrix->DeepCopy(camera->GetModelTransformMatrix());
    vtkMatrix4x4::Invert(
      this->SavedModelMatrix.GetPointer(), this->SavedInverseModelMatrix.GetPointer());

    // save the inverse wand matrix
    this->SavedInverseWandMatrix->DeepCopy(event.data.tracker.matrix);
    vtkMatrix4x4::Invert(
      this->SavedInverseWandMatrix.GetPointer(), this->SavedInverseWandMatrix.GetPointer());

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(this->Origin, 3);
    this->Origin[3] = 1;
    this->IsInitialRecorded = true;
  }

  this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
}
