/*=========================================================================

  Module:    vtkKWWizardWorkflow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWizardWorkflow - a wizard workflow engine.
// .SECTION Description
// This class is the basis for a wizard workflow engine, i.e. a state
// machine with a enhancements to support wizard steps (vtkKWWizardStep).
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWWizardStep vtkKWStateMachine vtkKWStateMachineState

#ifndef __vtkKWWizardWorkflow_h
#define __vtkKWWizardWorkflow_h

#include "vtkKWStateMachine.h"

class vtkKWWizardStep;
class vtkKWStateMachineState;
class vtkKWWizardWorkflowInternals;

class KWWidgets_EXPORT vtkKWWizardWorkflow : public vtkKWStateMachine
{
public:
  static vtkKWWizardWorkflow* New();
  vtkTypeRevisionMacro(vtkKWWizardWorkflow, vtkKWStateMachine);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a step.
  // Note that the step's components will be added automatically to the state
  // machine (i.e. its InteractionState and ValidationState states as well as
  // its ValidationTransition and ValidationFailedTransition transitions). 
  // A new cluster will be created for both InteractionState and 
  // ValidationState states.
  // Return 1 on success, 0 otherwise.
  virtual int AddStep(vtkKWWizardStep *step);
  virtual int HasStep(vtkKWWizardStep *step);
  virtual int GetNumberOfSteps();
  virtual vtkKWWizardStep* GetNthStep(int rank);

  // Description:
  // Add a next step, connecting it to the previous added step (if any).
  // The convenience method will:
  //  - call AddStep(), 
  //  - create a transition from the previously added step (if any) *to* this
  //    step, by calling the CreateNextTransition() method,
  //  - create a transition from this step *back* to the previously added step
  //    (if any), by calling the CreateBackTransition() method.
  // Return 1 on success, 0 otherwise.
  virtual int AddNextStep(vtkKWWizardStep *step);

  // Description:
  // Create a transition from an originating step to a destination step. 
  // The destination step should semantically be a "next" step, i.e. from
  // a workflow perspective, the destination step is meant to appear "after"
  // the originating step.
  // More specifically, this method creates a transition from the origin's
  // ValidationState state to the destination's InteractionState, triggered
  // by next_input. The transition's StartCommand callback is automatically
  // set to invoke the originating step's HideUserInterfaceCommand callback,
  // effectively hiding the originating step's UI before the destination
  // state is reached.
  // This method is used by the AddNextStep() method to connect a newly added
  // step to a previously added step (if any). The input used in that case is
  // vtkKWWizardStep::ValidationSucceededInput, and is expected to be pushed
  // by the previously added step's ValidationCommand callback.
  virtual int CreateNextTransition(
    vtkKWWizardStep *origin, 
    vtkKWStateMachineInput *next_input,
    vtkKWWizardStep *destination);

  // Description:
  // Create a transition *back* from a destination step to an originating step.
  // The destination step should semantically be a "next" step, i.e. from
  // a workflow perspective, the destination step is meant to appear "after"
  // the originating step.
  // More specifically, this method creates a transition from the 
  // destination's InteractionState state to the origin's InteractionState 
  // state, triggered by the origin step's GoBackToSelfInput input. 
  // The transition's StartCommand callback is automatically set to invoke
  // the destination step's HideUserInterfaceCommand callback,
  // effectively hiding the destination step's UI before the origin state
  // is reached back.
  virtual int CreateBackTransition(
    vtkKWWizardStep *origin, 
    vtkKWWizardStep *destination);

  // Description:
  // Create a go-to transition from an originating step to a destination step. 
  // The destination step does NOT have to be a "next" step semantically, i.e.
  // from a workflow perspective, the destination step can be meant to appear
  // "after" or "before" the originating step. Such a transition is designed
  // to reach a step directly, effectively bypassing all others steps: this
  // should be used *very* carefully (as it bends the state machine principles
  // to some extent), and is provided only to implement features such as the
  // "Finish" button in a wizard widget.
  // More specifically, this method creates 4 transitions:
  // 1) A transition from the origin's InteractionState to an internal 
  //    GoToState state acting as a hub, triggered the destination step's 
  //    GoToSelfInput input. The transition's EndCommand callback is 
  //    automatically set to invoke the TryToGoToStepCallback callback, 
  //    which is in turn responsible for checking if the destination step can
  //    be reached, by invoking its CanGoToSelfCommand callback. On success, 
  //    the destination step's UI is hidden by calling its 
  //    HideUserInterfaceCommand callback, and its GoToSelfInput input is 
  //    pushed again to trigger transition 2). On error, the origin step's
  //    GoBackToSelfInput input is pushed to trigger transition 3).
  // 2) A transition from the internal GoToState hub state to the
  //    destination step's InteractionState state, triggered by the destination
  //    step's GoToSelfInput input that will be pushed by the 
  //    TryToGoToStepCallback callback attached to transition 1). This will
  //    effectively lead the state machine to the destination state.
  // 3) A transition from the internal GoToState hub state back to the
  //    origin step's InteractionState state, triggered by the origin
  //    step's GoBackToSelfInput input that will be pushed by the 
  //    TryToGoToStepCallback callback attached to transition 1).
  // 4) a transition from the destination's InteractionState state back to
  //    the origin's InteractionState state, triggered by the origin step's
  //    GoBackToSelfInput input (by calling the CreateBackTransition method).

  virtual int CreateGoToTransition(
    vtkKWWizardStep *origin, 
    vtkKWWizardStep *destination);

  // Description:
  // Create a go-to transition from all known steps added so far to a 
  // destination step. See the CreateGoToTransition() method for more details.
  virtual int CreateGoToTransitions(vtkKWWizardStep *destination);

  // Description:
  // Set/Get the initial step. This is a convenience method to set the
  // vtkKWStateMachine::InitialState to a specific step's InteractionState
  // state. Check vtkKWStateMachine for more details.
  // Note that the initial state can not be reset.
  // Note that setting the initial state is actually the same as entering
  // it (i.e. the state's Enter() method will be called). In this case,
  // this will trigger the step's ShowUserInterfaceCommand callback, 
  // effectively showing this step's UI. For that reason, this method should
  // be the last method you call after setting up the whole workflow.
  // Return 1 on success, 0 otherwise.
  virtual vtkKWWizardStep* GetInitialStep();
  virtual int SetInitialStep(vtkKWWizardStep*);

  // Description:
  // Get the current step, i.e. the step vtkKWStateMachine::CurrentState
  // belongs too.
  virtual vtkKWWizardStep* GetCurrentStep();

  // Description:
  // Set/Get the finish step (if not set, GetFinishStep() will return
  // the last added step). This is not mandatory and is mainly used
  // for user interface (vtkKWWizardWidget) or IO purposes.
  // The finish step should semantically be the "last" step, i.e. from
  // a workflow perspective, the finish step is meant to appear "after"
  // all other steps, at the end.
  virtual void SetFinishStep(vtkKWWizardStep*);
  virtual vtkKWWizardStep* GetFinishStep();

  // Description:
  // Create a go-to transition from all known steps added so far to the 
  // Finish step. See FinishStep and the CreateGoToTransitions() and 
  // CreateGoToTransition() methods for more details.
  virtual int CreateGoToTransitionsToFinishStep();

  // Description:
  // Get the step a state belongs to (if any)
  virtual vtkKWWizardStep* GetStepFromState(vtkKWStateMachineState*);

  // Description:
  // The wizard workflow tries its best to keep a step navigation stack, i.e.
  // the path that lead to the current step. This is *not* a step history,
  // as going "back" (by calling AttemptToGoToPreviousStep() for example) will
  // actually remove steps from the stack.
  virtual int GetNumberOfStepsInNavigationStack();
  virtual vtkKWWizardStep* GetNthStepInNavigationStack(int rank);

  // Description:
  // Attempt to navigate in the workflow by moving the next, previous, or
  // finish step.
  // The AttemptToGoToNextStep() method pushes a
  // vtkKWWizardStep::ValidationInput input. 
  // The AttemptToGoToFinishStep() method pushes the FinishStep's 
  // GoToSelfInput input.
  // The AttemptToGoToPreviousStep() method pushes the previous step's
  // GoBackToSelfInput input (if any) and updates the navigation stack.
  virtual void AttemptToGoToNextStep();
  virtual void AttemptToGoToPreviousStep();
  virtual void AttemptToGoToFinishStep();

  // Description:
  // Specifies a command to associate with this workflow. This command is 
  // invoked when the navigation stack has changed, as a result of moving
  // forward (or backward) to a new step.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetNavigationStackedChangedCommand(
    vtkObject *object, const char *method);
  virtual void InvokeNavigationStackedChangedCommand();
  virtual int HasNavigationStackedChangedCommand();

  // Description:
  // Events. The NavigationStackedChangedCommand is invoked when the navigation
  // stack has changed, as a result of moving forward (or backward) to a 
  // new step.
  //BTX
  enum
  {
    NavigationStackedChangedEvent = 10000
  };
  //ETX

  // Description:
  // Callbacks.
  virtual void TryToGoToStepCallback(
    vtkKWWizardStep *origin, vtkKWWizardStep *destination);

  // Description:
  // Specifies a command to associate with this state machine. This command is 
  // invoked when the state machine current state has changed.
  // Override to allow the workflow engine to keep track of a navigation stack.
  virtual void InvokeCurrentStateChangedCommand();

protected:
  vtkKWWizardWorkflow();
  ~vtkKWWizardWorkflow();

  // Description:
  // Remove step(s).
  virtual void RemoveStep(vtkKWWizardStep *step);
  virtual void RemoveAllSteps();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWWizardWorkflowInternals *Internals;
  //ETX

  // Description:
  // Get the goto state
  vtkKWStateMachineState *GoToState;
  virtual vtkKWStateMachineState* GetGoToState();

  char *NavigationStackedChangedCommand;

  // Description:
  // Push/pop a step to/from the navigation stack..
  virtual void PushStepToNavigationStack(vtkKWWizardStep*);
  virtual vtkKWWizardStep* PopStepFromNavigationStack();

private:

  vtkKWWizardStep *FinishStep;

  vtkKWWizardWorkflow(const vtkKWWizardWorkflow&); // Not implemented
  void operator=(const vtkKWWizardWorkflow&); // Not implemented
};

#endif
