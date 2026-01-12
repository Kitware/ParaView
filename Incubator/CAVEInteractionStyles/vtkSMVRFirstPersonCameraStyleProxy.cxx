// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRFirstPersonCameraStyleProxy.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVRQueue.h"

namespace
{
constexpr std::string_view MOVE_FORWARD_ROLE = "Move forward";
constexpr std::string_view MOVE_BACKWARD_ROLE = "Move backward";
constexpr std::string_view MOVE_RIGHT_ROLE = "Move right";
constexpr std::string_view MOVE_LEFT_ROLE = "Move left";
constexpr std::string_view MOUSE_POS_X_ROLE = "Mouse position X";
constexpr std::string_view MOUSE_POS_Y_ROLE = "Mouse position Y";

constexpr double PITCH_CLAMP_LIMIT = 1.4;
}

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRFirstPersonCameraStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRFirstPersonCameraStyleProxy::vtkSMVRFirstPersonCameraStyleProxy()
{
  this->AddButtonRole(std::string(::MOVE_FORWARD_ROLE));
  this->AddButtonRole(std::string(::MOVE_BACKWARD_ROLE));
  this->AddButtonRole(std::string(::MOVE_LEFT_ROLE));
  this->AddButtonRole(std::string(::MOVE_RIGHT_ROLE));

  this->AddValuatorRole(std::string(::MOUSE_POS_X_ROLE));
  this->AddValuatorRole(std::string(::MOUSE_POS_Y_ROLE));
}

// ----------------------------------------------------------------------------
void vtkSMVRFirstPersonCameraStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Move Forward: " << this->MoveForward << std::endl;
  os << indent << "Move speed Forward: " << this->MoveSpeedForward << std::endl;

  os << indent << "Move Right: " << this->MoveRight << std::endl;
  os << indent << "Move speed Right: " << this->MoveSpeedRight << std::endl;

  os << indent << "Move X: " << this->MousePosX << std::endl;
  os << indent << "Move Y: " << this->MousePosY << std::endl;
  os << indent << "Mouse sensitivity X: " << this->MouseSensitivityX << std::endl;
  os << indent << "Mouse sensitivity Y: " << this->MouseSensitivityY << std::endl;

  os << indent << "Up Axis : ";
  switch (this->UpAxis)
  {
    case X_AXIS:
      os << "X Axis" << std::endl;
      break;
    case Y_AXIS:
      os << "Y Axis" << std::endl;
      break;
    case Z_AXIS:
      os << "Z Axis" << std::endl;
      break;
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRFirstPersonCameraStyleProxy::UpdateVTKObjects()
{
  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("MouseSensitivityX"));
  this->SetMouseSensitivityX(dvp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("MouseSensitivityY"));
  this->SetMouseSensitivityY(dvp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("MoveSpeedForward"));
  this->SetMoveSpeedForward(dvp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("MoveSpeedRight"));
  this->SetMoveSpeedRight(dvp->GetElement(0));

  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("UpAxis"));
  this->SetUpAxis(static_cast<vtkSMVRFirstPersonCameraStyleProxy::Axis>(ivp->GetElement(0)));

  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("InvertXAxis"));
  this->SetInvertXAxis(static_cast<bool>(ivp->GetElement(0)));

  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("InvertYAxis"));
  this->SetInvertYAxis(static_cast<bool>(ivp->GetElement(0)));
}

// ----------------------------------------------------------------------------
bool vtkSMVRFirstPersonCameraStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr)
  {
    vtkWarningMacro("Cannot operate without a controlled proxy");
    return true;
  }
  if (vtkSMRenderViewProxy::SafeDownCast(this->ControlledProxy) == nullptr)
  {
    vtkWarningMacro("Cannot operate with a controlled proxy that is not a render view");
    return true;
  }

  double camFocalPoint[3];
  vtkSMPropertyHelper(this->ControlledProxy, "CameraFocalPoint").Get(camFocalPoint, 3);

  double camPosition[3];
  vtkSMPropertyHelper(this->ControlledProxy, "CameraPosition").Get(camPosition, 3);

  double camViewUp[3];
  vtkSMPropertyHelper(this->ControlledProxy, "CameraViewUp").Get(camViewUp, 3);

  vtkVector3d forwardVector(camFocalPoint[0] - camPosition[0], camFocalPoint[1] - camPosition[1],
    camFocalPoint[2] - camPosition[2]);
  double lengthForwardVector = forwardVector.Norm();
  forwardVector.Normalize();

  const unsigned int upAxisIdx = static_cast<unsigned int>(this->UpAxis);
  const unsigned int rightAxisIdx = (upAxisIdx + 1) % 3;
  const unsigned int fwdAxisIdx = (upAxisIdx + 2) % 3;

  double newYaw = this->MouseSensitivityX * (-this->MousePosX + 0.5);
  double newPitch = this->MouseSensitivityY * (-this->MousePosY + 0.5);

  newPitch = std::clamp(
    newPitch, -::PITCH_CLAMP_LIMIT, ::PITCH_CLAMP_LIMIT); // To prevent an alignment with camera up

  const double cosPitch = std::cos(newPitch);
  double newForwardVector[3];
  newForwardVector[fwdAxisIdx] = cosPitch * std::sin(newYaw);
  newForwardVector[upAxisIdx] = std::sin(newPitch);
  newForwardVector[rightAxisIdx] = cosPitch * std::cos(newYaw);

  vtkVector3d newRightVector;
  vtkMath::Cross(newForwardVector, camViewUp, newRightVector.GetData());
  newRightVector.Normalize();

  double moveVector[3] = { this->MoveForward * this->MoveSpeedForward * newForwardVector[0] +
      this->MoveRight * this->MoveSpeedRight * newRightVector.GetX(),
    this->MoveForward * this->MoveSpeedForward * newForwardVector[1] +
      this->MoveRight * this->MoveSpeedRight * newRightVector.GetY(),
    this->MoveForward * this->MoveSpeedForward * newForwardVector[2] +
      this->MoveRight * this->MoveSpeedRight * newRightVector.GetZ() };

  camPosition[0] += moveVector[0];
  camPosition[1] += moveVector[1];
  camPosition[2] += moveVector[2];
  vtkSMPropertyHelper(this->ControlledProxy, "CameraPosition").Set(camPosition, 3);

  double newFocalPoint[3] = { camPosition[0] + newForwardVector[0] * lengthForwardVector,
    camPosition[1] + newForwardVector[1] * lengthForwardVector,
    camPosition[2] + newForwardVector[2] * lengthForwardVector };
  vtkSMPropertyHelper(this->ControlledProxy, "CameraFocalPoint").Set(newFocalPoint, 3);

  this->ControlledProxy->UpdateVTKObjects();
  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRFirstPersonCameraStyleProxy::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);
  if (role == std::string(::MOVE_FORWARD_ROLE))
  {
    this->MoveForward = static_cast<int>(event.data.button.state > 0);
  }
  if (role == std::string(::MOVE_LEFT_ROLE))
  {
    this->MoveRight = -static_cast<int>(event.data.button.state > 0);
  }
  if (role == std::string(::MOVE_RIGHT_ROLE))
  {
    this->MoveRight = static_cast<int>(event.data.button.state > 0);
  }
  if (role == std::string(::MOVE_BACKWARD_ROLE))
  {
    this->MoveForward = -static_cast<int>(event.data.button.state > 0);
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRFirstPersonCameraStyleProxy::HandleValuator(const vtkVREvent& event)
{
  const unsigned int mousePosXIdx =
    this->GetChannelIndexForValuatorRole(std::string(::MOUSE_POS_X_ROLE));
  const unsigned int mousePosYIdx =
    this->GetChannelIndexForValuatorRole(std::string(::MOUSE_POS_Y_ROLE));

  if (this->InvertXAxis)
  {
    this->MousePosX = event.data.valuator.channel[mousePosXIdx];
  }
  else
  {
    this->MousePosX = 1.0 - event.data.valuator.channel[mousePosXIdx];
  }

  if (this->InvertYAxis)
  {
    this->MousePosY = event.data.valuator.channel[mousePosYIdx];
  }
  else
  {
    this->MousePosY = 1.0 - event.data.valuator.channel[mousePosYIdx];
  }
}
