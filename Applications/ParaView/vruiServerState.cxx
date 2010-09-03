#include "vruiServerState.h"

// ----------------------------------------------------------------------------
vruiServerState::vruiServerState()
{
}

// ----------------------------------------------------------------------------
vtkstd::vector<vtkSmartPointer<vruiTrackerState> > *
vruiServerState::GetTrackerStates()
{
  return &(this->TrackerStates);
}

// ----------------------------------------------------------------------------
vtkstd::vector<bool> *vruiServerState::GetButtonStates()
{
  return &(this->ButtonStates);
}

// ----------------------------------------------------------------------------
vtkstd::vector<float> *vruiServerState::GetValuatorStates()
{
  return &(this->ValuatorStates);
}
