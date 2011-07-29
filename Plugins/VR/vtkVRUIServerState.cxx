#include "vtkVRUIServerState.h"

// ----------------------------------------------------------------------------
vtkVRUIServerState::vtkVRUIServerState()
{
}

// ----------------------------------------------------------------------------
vtkstd::vector<vtkSmartPointer<vtkVRUITrackerState> > *
vtkVRUIServerState::GetTrackerStates()
{
  return &(this->TrackerStates);
}

// ----------------------------------------------------------------------------
vtkstd::vector<bool> *vtkVRUIServerState::GetButtonStates()
{
  return &(this->ButtonStates);
}

// ----------------------------------------------------------------------------
vtkstd::vector<float> *vtkVRUIServerState::GetValuatorStates()
{
  return &(this->ValuatorStates);
}
