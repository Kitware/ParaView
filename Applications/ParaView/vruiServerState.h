#ifndef vruiServerState_h
#define vruiServerState_h

#include "vruiTrackerState.h"
#include "vtkSmartPointer.h"

class vruiServerState
{
public:
  // Description:
  // Default constructor. All arrays are of size 0.
  vruiServerState();

  // Description:
  // Return the state of all the trackers.
  vtkstd::vector<vtkSmartPointer<vruiTrackerState> > *GetTrackerStates();

  // Description:
  // Return the state of all the buttons.
  vtkstd::vector<bool> *GetButtonStates();

  // Description:
  // Return the state of all the valuators (whatever it is).
  vtkstd::vector<bool> *GetButtonStates();

protected:
  vtkstd::vector<vtkSmartPointer<vruiTrackerState> > TrackerStates;
  vtkstd::vector<bool> ButtonStates;
  vtkstd::vector<float> ValuatorStates;

private:
  vruiServerState(const vruiServerState&); // Not implemented.
  void operator=(const vruiServerState&); // Not implemented.
};

#endif // #ifndef vruiServerState_h
