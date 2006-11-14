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
  // Add a transition. The transition must be complete, i.e. its originating
  // and destination states must be set, as well as its input. Furthermore,
  // said parameters must be known to the state machine, i.e. one should
  // make sure the states and input have been added to the state machine first.
  // Return 1 on success, 0 otherwise.
  virtual int AddTransition(vtkKWStateMachineTransition *transition);
  virtual int HasTransition(vtkKWStateMachineTransition *transition);
  virtual int GetNumberOfTransitions();
  virtual vtkKWStateMachineTransition* GetNthTransition(int rank);

  // Description:
  // Create and add a new transition. If a transition object has already
  // been added with the same parameters, it will be used instead.
  // Return transition on success, NULL otherwise.
  virtual vtkKWStateMachineTransition* CreateTransition(
    vtkKWStateMachineState *origin,
    vtkKWStateMachineInput *input,
    vtkKWStateMachineState *destination);

  // Description:
  // Find a transition.
  // Return transition on success, NULL otherwise.
  virtual vtkKWStateMachineTransition* FindTransition(
    vtkKWStateMachineState *origin,
    vtkKWStateMachineInput *input);
  virtual vtkKWStateMachineTransition* FindTransition(
    vtkKWStateMachineState *origin,
    vtkKWStateMachineInput *input,
    vtkKWStateMachineState *destination);
  
  // Description:
  // Set/Get the initial state.
  // This call bootstraps the state machine, it should therefore be the
  // last method you call after setting up the whole state machine.
  // Note that the initial state can not be reset.
  // Note that setting the initial state is actually the same as entering
  // it (i.e. the state's Enter() method will be called).
  // Return 1 on success, 0 otherwise.
  vtkGetObjectMacro(InitialState, vtkKWStateMachineState);
  virtual int SetInitialState(vtkKWStateMachineState*);

  // Description:
  // Get the current and previous state.
  vtkGetObjectMacro(CurrentState, vtkKWStateMachineState);
  vtkKWStateMachineState* GetPreviousState();

  // Description:
  // Push a new input in the queue of inputs to be processed. 
  virtual void PushInput(vtkKWStateMachineInput *input);

  // Description:
  // Perform the state transition and invoke the corresponding action for
  // every pending input stored in the input queue.
  // For each input in the queue:
  // - a transition T is searched accepting the current state C and the input,
  // - if found:
  //    - T's Start() method is triggered,
  //    - C's Leave() method is triggered,
  //    - T is pushed to the history (see GetNthTransitionInHistory),
  //    - C becomes T's DestinationState (i.e. current state = new state),
  //    - CurrentStateChangedCommand and CurrentStateChangedEvent are invoked,
  //    - C (i.e. T's DestinationState)'s Enter() method is triggered,
  //    - T's End() method is triggered.
  virtual void ProcessInputs();

  // Description:
  // The state machine keeps an history of all the transitions that were 
  // applied so far.
  virtual int GetNumberOfTransitionsInHistory();
  virtual vtkKWStateMachineTransition* GetNthTransitionInHistory(int rank);

  // Description:
  // Add a cluster. Clusters are not used by the state machine per se, they
  // are just a convenient way to group states logically together, and can
  // be used by state machine writers (see vtkKWStateMachineDOTWriter)
  // to display clusters as groups.
  // Return 1 on success, 0 otherwise.
  virtual int AddCluster(vtkKWStateMachineCluster *cluster);
  virtual int HasCluster(vtkKWStateMachineCluster *cluster);
  virtual int GetNumberOfClusters();
  virtual vtkKWStateMachineCluster* GetNthCluster(int rank);

  // Description:
  // Specifies a command to associate with this state machine. This command is 
  // invoked when the state machine current state has changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetCurrentStateChangedCommand(
    vtkObject *object, const char *method);
  virtual void InvokeCurrentStateChangedCommand();
  virtual int HasCurrentStateChangedCommand();

  // Description:
  // Events. The CurrentStateChangedCommand is invoked when the state machine 
  // current state has changed.
  //BTX
  enum
  {
    CurrentStateChangedEvent = 10000
  };
  //ETX

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

  char *CurrentStateChangedCommand;

  // Description:
  // Push transition to the history.
  virtual void PushTransitionToHistory(vtkKWStateMachineTransition*);

private:

  vtkKWStateMachine(const vtkKWStateMachine&); // Not implemented
  void operator=(const vtkKWStateMachine&); // Not implemented
};

#endif
