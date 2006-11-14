/*=========================================================================

  Module:    vtkKWWizardStep.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWizardStep - a wizard step.
// .SECTION Description
// This class is the basis for a wizard step. A wizard step is a 
// placeholder for various states, transitions and inputs that are used
// in a typical wizard workflow. Such steps can be added to instances of
// the vtkKWWizardWorkflow class (subclass of vtkKWStateMachine). 
// A wizard workflow can be manipulated from a user interface through either
// the vtkKWWizardWidget or vtkKWWizardDialog classes.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWWizardWorkflow vtkKWStateMachine vtkKWWizardWidget vtkKWWizardDialog

#ifndef __vtkKWWizardStep_h
#define __vtkKWWizardStep_h

#include "vtkKWObject.h"

class vtkKWStateMachineState;
class vtkKWStateMachineInput;
class vtkKWStateMachineTransition;

//BTX
class KWWidgets_EXPORT vtkKWWizardStepCleanup
{
public:
  vtkKWWizardStepCleanup() {};
  ~vtkKWWizardStepCleanup();
};
//ETX

class KWWidgets_EXPORT vtkKWWizardStep : public vtkKWObject
{
public:
  static vtkKWWizardStep* New();
  vtkTypeRevisionMacro(vtkKWWizardStep, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get id.
  vtkGetMacro(Id, vtkIdType);

  // Description:
  // Set/Get simple name.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // Set/Get short description.
  vtkGetStringMacro(Description);
  vtkSetStringMacro(Description);

  // Description:
  // Specifies a command to associate with this step. This command is 
  // typically invoked when the user interface for that step needs
  // to be shown.
  // Wizard developpers should set ShowUserInterfaceCommand to point to a
  // method of their choice to display the step's UI; it will be invoked 
  // automatically when the state machine enters the step's InteractionState
  // state.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetShowUserInterfaceCommand(
    vtkObject *object, const char *method);
  virtual void InvokeShowUserInterfaceCommand();
  virtual int HasShowUserInterfaceCommand();

  // Description:
  // Specifies a command to associate with this step. This command is 
  // typically invoked when the user interface for that step needs 
  // to be hidden.
  // Wizard developpers should set HideUserInterfaceCommand to point to a
  // method of their choice to hide the step's UI; it will be invoked 
  // automatically by transitions that move the state machine from
  // one step to another step, such as the ones created by 
  // the vtkKWWizardWorkflow::AddNextStep(), 
  // vtkKWWizardWorkflow::CreateNextTransition() or 
  // vtkKWWizardWorkflow::CreateBackTransition() methods.
  // While this callback can be used to release resources that were allocated
  // specifically for a step's UI, most of the time calling the
  // vtkKWWizardWidget::ClearPage() method will do the trick when the wizard
  // workflow is working in conjunction with a vtkKWWizardWidget.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetHideUserInterfaceCommand(
    vtkObject *object, const char *method);
  virtual void InvokeHideUserInterfaceCommand();
  virtual int HasHideUserInterfaceCommand();

  // Description:
  // Specifies a command to associate with this step. This command is 
  // typically invoked when the user interface for that step needs 
  // to be validated.
  // This very important method is called when the ValidationTransition
  // transition is triggered by the ValidationInput input, effectively moving
  // the state machine from the InteractionState state to the ValidationState
  // state.
  // It is the responsibility of this callback to push inputs that will
  // move the state machine to the next step (using the 
  // ValidationSucceededInput input for example), or back to the 
  // InteractionState on error (using the ValidationFailedInput input and the
  // ValidationFailedTransition transition). User-defined inputs can be
  // pushed as well if the step has potentially multiple "valid" next steps.
  // Pushing the ValidationSucceededInput input will trigger transitions
  // such as the one created by the vtkKWWizardWorkflow::AddNextStep() or 
  // vtkKWWizardWorkflow::CreateNextTransition() methods.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetValidationCommand(
    vtkObject *object, const char *method);
  virtual void InvokeValidationCommand();
  virtual int HasValidationCommand();

  // Description:
  // Specifies a command to associate with this step. This command is 
  // typically invoked when there is a need to know if one can go
  // directly to this step, effectively bypassing all others steps: this
  // should be used *very* carefully, and is provided only to implement
  // features such as the "Finish" button in a wizard widget.
  // This command should return 1 if the step can be reached, 0 otherwise.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetCanGoToSelfCommand(
    vtkObject *object, const char *method);
  virtual int InvokeCanGoToSelfCommand();
  virtual int HasCanGoToSelfCommand();

  // Description:
  // Get the step's interaction state. This state is used to
  // display the user interface pertaining to this step, then wait for more
  // user inputs. Note that this class listens to the InteractionState's 
  // vtkKWStateMachineState::EnterEvent event; as this event is triggered, the 
  // InvokeShowUserInterfaceCommand() method is automatically called.
  // Wizard developpers should set ShowUserInterfaceCommand to point to a
  // method of their choice to display the step's UI; it will be invoked 
  // automatically when the state machine enters the step's InteractionState
  // state.
  // Access to this state is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; ShowUserInterfaceCommand
  // is however the key component to define for this state to work as expected.
  virtual vtkKWStateMachineState* GetInteractionState();

  // Description:
  // Get the step's validation state. This state is used to validate the user
  // interface pertaining to this step (as displayed by the InteractionState
  // state), then branch to the next step's InteractionState state on success,
  // or back to the current step's InteractionState state on error. The state
  // acts as a hub:  the validation itself is performed by the 
  // ValidationCommand callback attached to the ValidationTransition 
  // transition that sits between the InteractionState state and the 
  // ValidationState state.
  // Access to this state is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; ValidationCommand
  // is however the key component to define for this state to work as expected.
  virtual vtkKWStateMachineState* GetValidationState();

  // Description:
  // Get the step's validation transition. This transition is used to validate
  // the user interface pertaining to this step (as displayed by the 
  // InteractionState state), then branch to the next step's InteractionState
  // state on success, or back to the current step's InteractionState state
  // on error. More specifically:
  //   - its originating state is the InteractionState state, 
  //   - its destination state is the ValidationState state, 
  //   - it is triggered by the ValidationInput input. 
  // Note that this class listens to the ValidationTransition's 
  // vtkKWStateMachineTransition::EndEvent event; as this even is triggered, 
  // the InvokeValidationCommand() method is automatically called.
  // Wizard developpers should set ValidationCommand to point to a
  // method of their choice to validate the step's UI; it will be invoked
  // automatically when the state machine triggers the ValidationTransition
  // transition. The wizard workflow (or wizard widget) will typically push a 
  // ValidationInput input on the queue to request a step to be validated
  // and move to the next step. If the state machine is at an InteractionState
  // state, the corresponding step's ValidationTransition transition will be
  // triggered, the state machine will move to the ValidationState state and
  // validation will occur through the ValidationCommand callback. This 
  // callback will push inputs that in turn will move the state machine to
  // the next step (using the ValidationSucceededInput input for example), or
  // back to the InteractionState on error (using the ValidationFailedInput 
  // input and the ValidationFailedTransition transition).
  // Access to this transition is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; ValidationCommand is
  // however the key component to define for this transition to work as
  // expected, since it is where the ValidationSucceededInput, 
  // ValidationFailedInput and user-defined inputs should be pushed.
  virtual vtkKWStateMachineTransition* GetValidationTransition();

  // Description:
  // Get the step's validation input. This singleton input is used to trigger
  // the ValidationTransition transition and move the state machine from the 
  // InteractionState state to the ValidationState state.
  // Access to this input is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; the wizard workflow (or
  // wizard widget) will typically push a ValidationInput input on the queue
  // to request a step to be validated and move to the next step. If the state
  // machine is at an InteractionState state, the corresponding step's 
  // ValidationTransition transition will be triggered, the state machine will
  // move to the ValidationState state and validation will occur through the
  // ValidationCommand callback.
  static vtkKWStateMachineInput* GetValidationInput();

  // Description:
  // Get the step's validation successful input. This singleton input is used 
  // in the ValidationCommand callback and in conjunction with the 
  // workflow class (vtkKWWizardWorkflow) to trigger a transition from the
  // step's ValidationState state to the next step's InteractionState state. 
  // It is, as far as the workflow is concerned, the input that moves
  // the state machine from one step to the other. The corresponding 
  // transition can be created automatically by the
  // vtkKWWizardWorkflow::AddNextStep() or 
  // vtkKWWizardWorkflow::CreateNextTransition() methods.
  // ValidationCommand is the key component where this input is used.
  static vtkKWStateMachineInput* GetValidationSucceededInput();

  // Description:
  // Get the step's validation failed input. This singleton input is used 
  // in the ValidationCommand callback and in conjunction with the workflow
  // class (vtkKWWizardWorkflow) to trigger the ValidationFailedTransition 
  // transition from the step's ValidationState state back to the step's 
  // InteractionState state. 
  // ValidationCommand is the key component where this input is used.
  static vtkKWStateMachineInput* GetValidationFailedInput();

  // Description:
  // Get the step's validation failed transition. This transition is used 
  // to bring the state machine from the ValidationState state back to the
  // InteractionState state, when validation of the user interface pertaining 
  // to this step failed (as displayed by the InteractionState state).
  // More specifically:
  //   - its originating state is the ValidationState state, 
  //   - its destination state is the InteractionState state, 
  //   - it is triggered by the ValidationFailedInput input. 
  // Important: it is up to the wizard developpers to push the
  // ValidationFailedInput input on the state machine queue *from* the 
  // ValidationCommand callback for the state machine to trigger that 
  // transition and go back to the InteractionState state.
  // Access to this transition is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; ValidationCommand is however
  // the key component to define for this transition to work as expected,
  // since it is where the ValidationFailedInput input should be pushed.
  virtual vtkKWStateMachineTransition* GetValidationFailedTransition();

  // Description:
  // Get the step's go to self input. This input is used to trigger
  // transition that are meant to move the state machine directly to this step,
  // effectively bypassing all others steps: this should be used very 
  // carefully, and is provided only to implement features such as the
  // "Finish" button in a wizard widget.
  // Access to this input is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; it is used by the
  // vtkKWWizardWorkflow::CreateGoToTransition() method to implement
  // transitions to specific steps directly (the "Finish" step, for example).
  virtual vtkKWStateMachineInput* GetGoToSelfInput();

  // Description:
  // Get the step's go back to self input. This input is used to trigger
  // transitions that are meant to move the state machine back to the 
  // previous step (if any): this should be used very carefully, and is 
  // provided only to implement features such as the "Back" or "Finish" button
  // in a wizard widget.
  // Access to this input is given for advanced customization. In the vast
  // majority of wizards, it should be ignored; it is used by the
  // vtkKWWizardWorkflow::CreateBackTransition() method to implement
  // transitions back to specific steps directly.
  virtual vtkKWStateMachineInput* GetGoBackToSelfInput();

  // Description:
  // Add all the default observers needed by that object, or remove
  // all the observers that were added through AddCallbackCommandObserver.
  // Subclasses can override these methods to add/remove their own default
  // observers, but should call the superclass too.
  virtual void RemoveCallbackCommandObservers();

protected:
  vtkKWWizardStep();
  ~vtkKWWizardStep();

  vtkIdType Id;
  char *Name;
  char *Description;

  char *ShowUserInterfaceCommand;
  char *HideUserInterfaceCommand;
  char *ValidationCommand;
  char *CanGoToSelfCommand;

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  // Subclasses can oberride this method to process their own events, but
  // should call the superclass too.
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  
private:

  vtkKWStateMachineState *InteractionState;
  vtkKWStateMachineState *ValidationState;

  vtkKWStateMachineTransition *ValidationTransition;
  vtkKWStateMachineTransition *ValidationFailedTransition;

  vtkKWStateMachineInput *GoToSelfInput;
  vtkKWStateMachineInput *GoBackToSelfInput;

  static vtkIdType IdCounter;

  static vtkKWStateMachineInput *ValidationInput;
  static vtkKWStateMachineInput *ValidationSucceededInput;
  static vtkKWStateMachineInput *ValidationFailedInput;

  //BTX
  // Used to delete our singletons.
  static vtkKWWizardStepCleanup Cleanup;
  friend class vtkKWWizardStepCleanup;
  //ETX

  static void SetValidationInput(vtkKWStateMachineInput*);
  static void SetValidationSucceededInput(vtkKWStateMachineInput*);
  static void SetValidationFailedInput(vtkKWStateMachineInput*);

  vtkKWWizardStep(const vtkKWWizardStep&); // Not implemented
  void operator=(const vtkKWWizardStep&); // Not implemented
};

#endif
