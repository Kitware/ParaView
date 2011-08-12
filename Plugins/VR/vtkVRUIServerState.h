#ifndef vtkVRUIServerState_h
#define vtkVRUIServerState_h

#include "vtkVRUITrackerState.h"
#include "vtkSmartPointer.h"
#include <vtkstd/vector>

class vtkVRUIServerState
{
public:
  // Description:
  // Default constructor. All arrays are of size 0.
  vtkVRUIServerState();

  // Description:
  // Return the state of all the trackers.
  vtkstd::vector<vtkSmartPointer<vtkVRUITrackerState> > *GetTrackerStates();

  // Description:
  // Return the state of all the buttons.
  vtkstd::vector<bool> *GetButtonStates();

  // Description:
  // Return the state of all the valuators (whatever it is).
  vtkstd::vector<float> *GetValuatorStates();

protected:
  vtkstd::vector<vtkSmartPointer<vtkVRUITrackerState> > TrackerStates;
  vtkstd::vector<bool> ButtonStates;
  vtkstd::vector<float> ValuatorStates;

private:
  vtkVRUIServerState(const vtkVRUIServerState&); // Not implemented.
  void operator=(const vtkVRUIServerState&); // Not implemented.
};

#endif // #ifndef vtkVRUIServerState_h
