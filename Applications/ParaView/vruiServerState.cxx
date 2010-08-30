#include "vruiServerState.h"

// ----------------------------------------------------------------------------
vruiServerState::vruiServerState()
{
}

// ----------------------------------------------------------------------------
vtkstd::vector<vtkSmartPointer<vruiTrackerState> > *GetTrackerStates()
{
  return &(this->TrackerStates);
}

// ----------------------------------------------------------------------------
vtkstd::vector<bool> *GetButtonStates()
{
  return &(this->ButtonStates);
}

// ----------------------------------------------------------------------------
vtkstd::vector<float> *GetValuatorStates()
{
  return &(this->ValuatorStates);
}
