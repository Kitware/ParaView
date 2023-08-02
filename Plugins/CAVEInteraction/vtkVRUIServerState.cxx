// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRUIServerState.h"

// ----------------------------------------------------------------------------
vtkVRUIServerState::vtkVRUIServerState() = default;

// ----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkVRUITrackerState>>* vtkVRUIServerState::GetTrackerStates()
{
  return &(this->TrackerStates);
}

// ----------------------------------------------------------------------------
std::vector<bool>* vtkVRUIServerState::GetButtonStates()
{
  return &(this->ButtonStates);
}

// ----------------------------------------------------------------------------
std::vector<float>* vtkVRUIServerState::GetValuatorStates()
{
  return &(this->ValuatorStates);
}
