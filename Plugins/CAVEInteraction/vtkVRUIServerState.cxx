// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
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
