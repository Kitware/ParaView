// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRTravelTrackballExoSplitStyleProxy.h"

#include "vtkCamera.h"
#include "vtkIndent.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRTravelTrackballExoSplitStyleProxy);

// -----------------------------------------------------------------------------
// Constructor method
vtkSMVRTravelTrackballExoSplitStyleProxy::vtkSMVRTravelTrackballExoSplitStyleProxy()
  : Superclass()
{
  this->AddTrackerRole("Tracker");
  this->AddButtonRole("Reset");
  this->AddButtonRole("Rotate");
  this->AddButtonRole("Translate");
  this->EnableTranslate = false;
  this->EnableRotate = false;
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelTrackballExoSplitStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableTranslate: " << this->EnableTranslate << endl;
  os << indent << "EnableRotate: " << this->EnableRotate << endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRTravelTrackballExoSplitStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    return true;
  }

  vtkCamera* camera = vtkSMVRInteractorStyleProxy::GetActiveCamera();

  if (!camera)
  {
    vtkWarningMacro("This interactor styles requires an active camera");
    return true;
  }

  double diffPosition[3];
  double currPosition[3] = { this->TrackerMatrix->GetElement(0, 3),
    this->TrackerMatrix->GetElement(1, 3), this->TrackerMatrix->GetElement(2, 3) };

  diffPosition[0] = currPosition[0] - this->LastRecordedPosition[0];
  diffPosition[1] = currPosition[1] - this->LastRecordedPosition[1];
  diffPosition[2] = currPosition[2] - this->LastRecordedPosition[2];

  this->LastRecordedPosition[0] = currPosition[0];
  this->LastRecordedPosition[1] = currPosition[1];
  this->LastRecordedPosition[2] = currPosition[2];

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->EnableTranslate && this->PositionRecorded)
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Get(*this->ModelMatrix->Element, 16);

    this->ModelViewMatrix->DeepCopy(camera->GetModelViewTransformMatrix());

    // compute inverse view matrix
    vtkMatrix4x4::Invert(this->ModelViewMatrix, this->InvViewMatrix);
    vtkMatrix4x4::Multiply4x4(this->ModelMatrix, this->InvViewMatrix, this->InvViewMatrix);

    double dist = camera->GetDistance();

    this->ModelMatrix->DeepCopy(this->ModelViewMatrix);

    this->ModelMatrix->SetElement(
      0, 3, this->ModelMatrix->GetElement(0, 3) + diffPosition[0] * dist);
    this->ModelMatrix->SetElement(
      1, 3, this->ModelMatrix->GetElement(1, 3) + diffPosition[1] * dist);
    this->ModelMatrix->SetElement(
      2, 3, this->ModelMatrix->GetElement(2, 3) + diffPosition[2] * dist);

    vtkMatrix4x4::Multiply4x4(this->InvViewMatrix, this->ModelMatrix, this->ModelMatrix);

    // Store the target matrix back on the proxy
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->ModelMatrix);

    this->ControlledProxy->UpdateVTKObjects();
  }

  if (this->EnableRotate && this->PositionRecorded)
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Get(*this->ModelMatrix->Element, 16);

    this->ModelViewMatrix->DeepCopy(camera->GetModelViewTransformMatrix());

    // compute inverse view matrix and inverse model/view matrix
    vtkMatrix4x4::Invert(this->ModelViewMatrix, this->InvViewMatrix);
    vtkMatrix4x4::Multiply4x4(this->ModelMatrix, this->InvViewMatrix, this->InvViewMatrix);

    double center[4];
    vtkSMPropertyHelper(this->ControlledProxy, "CenterOfRotation").Get(center, 3);
    center[3] = 1.0;
    this->ModelViewMatrix->MultiplyPoint(center, center);

    // compute the transformation to apply
    this->TransformMatrix->Identity();
    this->TransformMatrix->SetElement(0, 3, -center[0]);
    this->TransformMatrix->SetElement(1, 3, -center[1]);
    this->TransformMatrix->SetElement(2, 3, -center[2]);

    this->Transform->Identity();
    this->Transform->RotateX(-diffPosition[1] * 300.0);
    this->Transform->RotateY(diffPosition[0] * 300.0);

    vtkMatrix4x4::Multiply4x4(
      this->Transform->GetMatrix(), this->TransformMatrix, this->TransformMatrix);

    this->TransformMatrix->SetElement(0, 3, this->TransformMatrix->GetElement(0, 3) + center[0]);
    this->TransformMatrix->SetElement(1, 3, this->TransformMatrix->GetElement(1, 3) + center[1]);
    this->TransformMatrix->SetElement(2, 3, this->TransformMatrix->GetElement(2, 3) + center[2]);

    // apply the transformation
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix, this->ModelViewMatrix, this->ModelMatrix);
    vtkMatrix4x4::Multiply4x4(this->InvViewMatrix, this->ModelMatrix, this->ModelMatrix);

    // Store the target matrix back on the proxy
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->ModelMatrix);

    this->ControlledProxy->UpdateVTKObjects();
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelTrackballExoSplitStyleProxy::HandleButton(const vtkVREvent& event)
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return;
  }

  this->PositionRecorded = false;

  std::string role = this->GetButtonRole(event.name);
  if (role == "Translate")
  {
    this->EnableTranslate = event.data.button.state;
  }
  else if (role == "Rotate")
  {
    this->EnableRotate = event.data.button.state;
  }
  else if (role == "Reset" && event.data.button.state == 1)
  {
    this->ModelMatrix->Identity();
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->ModelMatrix);
    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelTrackballExoSplitStyleProxy::HandleTracker(const vtkVREvent& event)
{
  if (this->GetTrackerRole(event.name) != "Tracker" ||
    (!this->EnableTranslate && !this->EnableRotate))
  {
    return;
  }

  if (!this->PositionRecorded)
  {
    this->LastRecordedPosition[0] = event.data.tracker.matrix[3];
    this->LastRecordedPosition[1] = event.data.tracker.matrix[7];
    this->LastRecordedPosition[2] = event.data.tracker.matrix[11];
    this->PositionRecorded = true;
  }

  this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
}
