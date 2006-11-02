/*=========================================================================

  Module:    vtkKWStateMachine.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachine - a state machine.
// .SECTION Description
// This class is the basis for a state machine framework. 
// A state machine is defined by a set of states, a set of inputs and a
// transition matrix that defines for each pair of (state,input) what is
// the next state to assume.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWStateMachineInput vtkKWStateMachineState vtkKWStateMachineTransition

#ifndef __vtkKWStateMachine_h
#define __vtkKWStateMachine_h

#include "vtkKWObject.h"

class vtkKWStateMachineState;
class vtkKWStateMachineTransition;
class vtkKWStateMachineInput;
class vtkKWStateMachineInternals;
class vtkKWStateMachineCluster;

class KWWidgets_EXPORT vtkKWStateMachine : public vtkKWObject
{
public:
  static vtkKWStateMachine* New();
  vtkTypeRevisionMacro(vtkKWStateMachine, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a state.
  // Return 1 on success, 0 otherwise.
  virtual int AddState(vtkKWStateMachineState *state);
  virtual int HasState(vtkKWStateMachineState *state);
  virtual int GetNumberOfStates();
  virtual vtkKWStateMachineState* GetNthState(int rank);

  // Description:
  // Add an input.
  // Return 1 on success, 0 otherwise.
  virtual int AddInput(vtkKWStateMachineInput *input);
  virtual int HasInput(vtkKWStateMachineInput *input);
  virtual int GetNumberOfInputs();
  virtual vtkKWStateMachineInput* GetNthInput(int rank);

  // Description:
  // Add a transition.
  // Return 1 on success, 0 otherwise.
  virtual int AddTransition(vtkKWStateMachineTransition *transition);
  virtual int HasTransition(vtkKWStateMachineTransition *transition);
  virtual int GetNumberOfTransitions();
  virtual vtkKWStateMachineTransition* GetNthTransition(int rank);

  // Description:
  // Create and add a new transition.
  // Return transition on success, NULL otherwise.
  virtual vtkKWStateMachineTransition* CreateTransition(
    vtkKWStateMachineState *state,
    vtkKWStateMachineInput *input,
    vtkKWStateMachineState *new_state);
  
  // Description:
  // Set the initial state, get the current state.
  vtkGetObjectMacro(CurrentState, vtkKWStateMachineState);
  vtkGetObjectMacro(InitialState, vtkKWStateMachineState);
  virtual void SetInitialState(vtkKWStateMachineState*);

  // Description:
  // Push a new input in the queue of inputs to be processed. 
  virtual void PushInput(vtkKWStateMachineInput *input);

  // Description:
  // Perform the state transition and invoke the corresponding action for
  // every pending input stored in the input queue.
  virtual void ProcessInputs();

  // Description:
  // Add a cluster. Clusters are not used by the state machine per se, they
  // are just a convenient way to group states logically together, and can
  // be used by state machine writers to display groups accordingly.
  // Return 1 on success, 0 otherwise.
  virtual int AddCluster(vtkKWStateMachineCluster *cluster);
  virtual int HasCluster(vtkKWStateMachineCluster *cluster);
  virtual int GetNumberOfClusters();
  virtual vtkKWStateMachineCluster* GetNthCluster(int rank);

protected:
  vtkKWStateMachine();
  ~vtkKWStateMachine();

  vtkKWStateMachineState *InitialState;
  vtkKWStateMachineState *CurrentState;

  // Description:
  // Remove state(s).
  virtual void RemoveState(vtkKWStateMachineState *state);
  virtual void RemoveAllStates();

  // Description:
  // Remove input(s).
  virtual void RemoveInput(vtkKWStateMachineInput *input);
  virtual void RemoveAllInputs();

  // Description:
  // Remove transition(s).
  virtual void RemoveTransition(vtkKWStateMachineTransition *transition);
  virtual void RemoveAllTransitions();

  // Description:
  // Remove cluster(s).
  virtual void RemoveCluster(vtkKWStateMachineCluster *cluster);
  virtual void RemoveAllClusters();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWStateMachineInternals *Internals;
  //ETX

  // Description:
  // Process one input.
  virtual void ProcessInput(vtkKWStateMachineInput *input);

private:

  vtkKWStateMachine(const vtkKWStateMachine&); // Not implemented
  void operator=(const vtkKWStateMachine&); // Not implemented
};

#endif
