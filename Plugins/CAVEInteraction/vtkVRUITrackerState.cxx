// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRUITrackerState.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkVRUITrackerState);

// ----------------------------------------------------------------------------
vtkVRUITrackerState::vtkVRUITrackerState()
{
  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;
  this->UnitQuaternion[0] = 0.0;
  this->UnitQuaternion[1] = 0.0;
  this->UnitQuaternion[2] = 0.0;
  this->UnitQuaternion[3] = 1.0;

  this->LinearVelocity[0] = 0.0;
  this->LinearVelocity[1] = 0.0;
  this->LinearVelocity[2] = 0.0;

  this->AngularVelocity[0] = 0.0;
  this->AngularVelocity[1] = 0.0;
  this->AngularVelocity[2] = 0.0;
}

// ----------------------------------------------------------------------------
vtkVRUITrackerState::~vtkVRUITrackerState() = default;

// ----------------------------------------------------------------------------
void vtkVRUITrackerState::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
