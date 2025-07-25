// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRControlLocationRelativeStyleProxy.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"

#include <algorithm>
#include <sstream>

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRControlLocationRelativeStyleProxy);

// -----------------------------------------------------------------------------
vtkSMVRControlLocationRelativeStyleProxy::vtkSMVRControlLocationRelativeStyleProxy()
  : Superclass()
{
  this->AddTrackerRole("Tracker");
  this->AddButtonRole("Reset");
  this->AddButtonRole("Move");
  this->EnableMovePoint = false;
  this->FirstUpdate = true;
  this->OriginalPoint[0] = 0;
  this->OriginalPoint[1] = 0;
  this->OriginalPoint[2] = 0;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlLocationRelativeStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Inside Update() but missing proxy or property");
    return true;
  }

  // Save the original property value to be used as the "Reset" value
  if (this->FirstUpdate)
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Get(this->OriginalPoint, 3);
    this->FirstUpdate = false;
  }

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->EnableMovePoint && this->PositionRecorded)
  {
    double diffPosition[3];
    double currPosition[3] = { this->TrackerMatrix->GetElement(0, 3),
      this->TrackerMatrix->GetElement(1, 3), this->TrackerMatrix->GetElement(2, 3) };

    diffPosition[0] = currPosition[0] - this->LastRecordedPosition[0];
    diffPosition[1] = currPosition[1] - this->LastRecordedPosition[1];
    diffPosition[2] = currPosition[2] - this->LastRecordedPosition[2];

    this->LastRecordedPosition[0] = currPosition[0];
    this->LastRecordedPosition[1] = currPosition[1];
    this->LastRecordedPosition[2] = currPosition[2];

    vtkCamera* camera = nullptr;
    camera = vtkSMVRInteractorStyleProxy::GetActiveCamera();
    if (!camera)
    {
      vtkWarningMacro("Cannot get active camera.");
      return true;
    }

    double point[4];
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(point, 3);
    point[3] = 1.0;

    vtkNew<vtkMatrix4x4> modelViewMatrix;
    vtkNew<vtkMatrix4x4> invModelViewMatrix;
    modelViewMatrix->DeepCopy(camera->GetModelViewTransformMatrix());
    vtkMatrix4x4::Invert(modelViewMatrix, invModelViewMatrix);

    modelViewMatrix->MultiplyPoint(point, point);

    // the translation is scaled by the depth distance
    point[0] -= diffPosition[0] * point[2];
    point[1] -= diffPosition[1] * point[2];
    point[2] -= diffPosition[2] * point[2];

    invModelViewMatrix->MultiplyPoint(point, point);

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Set(point, 3);

    this->ControlledProxy->UpdateVTKObjects();
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationRelativeStyleProxy::HandleButton(const vtkVREvent& data)
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return;
  }

  this->PositionRecorded = false;

  std::string role = this->GetButtonRole(data.name);
  if (role == "Move")
  {
    this->EnableMovePoint = data.data.button.state;
  }
  else if (role == "Reset" && data.data.button.state == 1)
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(this->OriginalPoint, 3);
    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationRelativeStyleProxy::HandleTracker(const vtkVREvent& data)
{
  if (this->GetTrackerRole(data.name) != "Tracker" || !this->EnableMovePoint)
  {
    return;
  }

  if (!this->PositionRecorded)
  {
    this->LastRecordedPosition[0] = data.data.tracker.matrix[3];
    this->LastRecordedPosition[1] = data.data.tracker.matrix[7];
    this->LastRecordedPosition[2] = data.data.tracker.matrix[11];
    this->PositionRecorded = true;
  }

  this->TrackerMatrix->DeepCopy(data.data.tracker.matrix);
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationRelativeStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableMovePoint: " << this->EnableMovePoint << endl;
}
