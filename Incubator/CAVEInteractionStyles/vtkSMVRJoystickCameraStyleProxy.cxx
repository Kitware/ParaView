// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRJoystickCameraStyleProxy.h"

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
constexpr std::string_view MOVE_JOYSTICK_FORWARD_ROLE = "Move joystick forward";
constexpr std::string_view MOVE_JOYSTICK_SIDE_ROLE = "Move joystick side";
constexpr std::string_view ORIENTATION_JOYSTICK_X_ROLE = "Orientation joystick X";
constexpr std::string_view ORIENTATION_JOYSTICK_Y_ROLE = "Orientation joystick Y";

constexpr double PITCH_CLAMP_LIMIT = 1.4;

// Joysticks are never at 0.0 so we define a minimum threshold value to consider the user is
// moving the joystick
constexpr double JOYSTICK_MIN_THRESHOLD = 0.1;
}

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRJoystickCameraStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRJoystickCameraStyleProxy::vtkSMVRJoystickCameraStyleProxy()
{
  this->AddValuatorRole(std::string(::MOVE_JOYSTICK_FORWARD_ROLE));
  this->AddValuatorRole(std::string(::MOVE_JOYSTICK_SIDE_ROLE));
  this->AddValuatorRole(std::string(::ORIENTATION_JOYSTICK_X_ROLE));
  this->AddValuatorRole(std::string(::ORIENTATION_JOYSTICK_Y_ROLE));
}

// ----------------------------------------------------------------------------
void vtkSMVRJoystickCameraStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Move Forward: " << this->MoveForward << std::endl;
  os << indent << "Move Right: " << this->MoveRight << std::endl;
  os << indent << "Move joystick sensitivity: " << this->MoveCameraSensitivity << std::endl;

  os << indent << "Move X: " << this->OrientationX << std::endl;
  os << indent << "Move Y: " << this->OrientationY << std::endl;
  os << indent << "Look rotation joystick sensitivity: " << this->LookRotationSensitivity
     << std::endl;

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
void vtkSMVRJoystickCameraStyleProxy::UpdateVTKObjects()
{
  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("LookRotationSensitivity"));
  this->SetLookRotationSensitivity(dvp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("MoveCameraSensitivity"));
  this->SetMoveCameraSensitivity(dvp->GetElement(0));

  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("UpAxis"));
  this->SetUpAxis(static_cast<vtkSMVRJoystickCameraStyleProxy::Axis>(ivp->GetElement(0)));

  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("InvertXAxis"));
  this->SetInvertXAxis(static_cast<bool>(ivp->GetElement(0)));

  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("InvertYAxis"));
  this->SetInvertYAxis(static_cast<bool>(ivp->GetElement(0)));
}

// ----------------------------------------------------------------------------
bool vtkSMVRJoystickCameraStyleProxy::Update()
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

  double newYaw = std::atan2(forwardVector[fwdAxisIdx], forwardVector[rightAxisIdx]) -
    this->LookRotationSensitivity * this->OrientationX;
  double newPitch =
    std::asin(forwardVector[upAxisIdx]) - this->LookRotationSensitivity * this->OrientationY;

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

  double moveVector[3] = { this->MoveCameraSensitivity *
      (this->MoveForward * newForwardVector[0] + this->MoveRight * newRightVector.GetX()),
    this->MoveCameraSensitivity *
      (this->MoveForward * newForwardVector[1] + this->MoveRight * newRightVector.GetY()),
    this->MoveCameraSensitivity *
      (this->MoveForward * newForwardVector[2] + this->MoveRight * newRightVector.GetZ()) };

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
void vtkSMVRJoystickCameraStyleProxy::HandleValuator(const vtkVREvent& event)
{
  const unsigned int moveFwdIdx =
    this->GetChannelIndexForValuatorRole(std::string(::MOVE_JOYSTICK_FORWARD_ROLE));
  const unsigned int moveSideIdx =
    this->GetChannelIndexForValuatorRole(std::string(::MOVE_JOYSTICK_SIDE_ROLE));

  this->MoveRight = std::abs(event.data.valuator.channel[moveSideIdx]) > ::JOYSTICK_MIN_THRESHOLD
    ? event.data.valuator.channel[moveSideIdx]
    : 0;
  this->MoveForward = std::abs(event.data.valuator.channel[moveFwdIdx]) > ::JOYSTICK_MIN_THRESHOLD
    ? -event.data.valuator.channel[moveFwdIdx]
    : 0;

  const unsigned int orientationXIdx =
    this->GetChannelIndexForValuatorRole(std::string(::ORIENTATION_JOYSTICK_X_ROLE));
  const unsigned int orientationYIdx =
    this->GetChannelIndexForValuatorRole(std::string(::ORIENTATION_JOYSTICK_Y_ROLE));

  this->OrientationX =
    std::abs(event.data.valuator.channel[orientationXIdx]) > ::JOYSTICK_MIN_THRESHOLD
    ? event.data.valuator.channel[orientationXIdx]
    : 0;
  this->OrientationY =
    std::abs(event.data.valuator.channel[orientationYIdx]) > ::JOYSTICK_MIN_THRESHOLD
    ? event.data.valuator.channel[orientationYIdx]
    : 0;

  if (this->InvertXAxis)
  {
    this->OrientationX *= -1;
  }
  if (this->InvertYAxis)
  {
    this->OrientationY *= -1;
  }
}
