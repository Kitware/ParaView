/*=========================================================================

  Module:    vtkKWWizardWorkflow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWizardWorkflow.h"

#include "vtkKWWizardStep.h"
#include "vtkKWStateMachineState.h"
#include "vtkKWStateMachineCluster.h"
#include "vtkKWStateMachineTransition.h"

#include "vtkObjectFactory.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/map>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWizardWorkflow);
vtkCxxRevisionMacro(vtkKWWizardWorkflow, "1.1");

//----------------------------------------------------------------------------
class vtkKWWizardWorkflowInternals
{
public:

  // Steps

  typedef vtksys_stl::vector<vtkKWWizardStep*> StepPoolType;
  typedef vtksys_stl::vector<vtkKWWizardStep*>::iterator StepPoolIterator;
  StepPoolType StepPool;

  StepPoolType StepNavigationStackPool;

  // State to Step
  // Provide a quick way to find which step a state belongs to (if any)

  typedef vtksys_stl::map<vtkKWStateMachineState*, vtkKWWizardStep*> StateToStepMapType;
  typedef vtksys_stl::map<vtkKWStateMachineState*, vtkKWWizardStep*>::iterator StateToStepMapIterator;

  StateToStepMapType StateToStepPool;

};

//----------------------------------------------------------------------------
vtkKWWizardWorkflow::vtkKWWizardWorkflow()
{
  this->Internals = new vtkKWWizardWorkflowInternals;

  this->AddInput(vtkKWWizardStep::GetValidationInput());
  this->AddInput(vtkKWWizardStep::GetValidationSucceededInput());
  this->AddInput(vtkKWWizardStep::GetValidationFailedInput());

  this->GoToState = NULL;
  this->FinishStep = NULL;

  this->NavigationStackedChangedCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWWizardWorkflow::~vtkKWWizardWorkflow()
{
  this->RemoveAllSteps();

  delete this->Internals;
  this->Internals = NULL;

  if (this->GoToState)
    {
    this->GoToState->Delete();
    this->GoToState = NULL;
    }

  if (this->NavigationStackedChangedCommand)
    {
    delete [] this->NavigationStackedChangedCommand;
    this->NavigationStackedChangedCommand = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::HasStep(vtkKWWizardStep *step)
{
  if (step)
    {
    vtkKWWizardWorkflowInternals::StepPoolIterator it = 
      this->Internals->StepPool.begin();
    vtkKWWizardWorkflowInternals::StepPoolIterator end = 
      this->Internals->StepPool.end();
    for (; it != end; ++it)
      {
      if ((*it) == step)
        {
        return 1;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::AddStep(vtkKWWizardStep *step)
{
  if (!step)
    {
    vtkErrorMacro("Can not add NULL step to pool!");
    return 0;
    }

  // Already in the pool ?

  if (this->HasStep(step))
    {
    vtkErrorMacro("The step is already in the pool!");
    return 0;
    }
  
  if (!step->GetApplication())
    {
    step->SetApplication(this->GetApplication());
    }

  this->Internals->StepPool.push_back(step);
  step->Register(this);

  // Add the step components to the state machine

  this->AddState(step->GetInteractionState());
  this->Internals->StateToStepPool[step->GetInteractionState()] = step;

  this->AddState(step->GetValidationState());
  this->Internals->StateToStepPool[step->GetValidationState()] = step;

  this->AddTransition(step->GetValidationTransition());
  this->AddTransition(step->GetValidationFailedTransition());

  vtkKWStateMachineCluster *cluster = vtkKWStateMachineCluster::New();
  cluster->SetName(step->GetName());
  cluster->AddState(step->GetInteractionState());
  cluster->AddState(step->GetValidationState());
  this->AddCluster(cluster);
  cluster->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::AddNextStep(vtkKWWizardStep *step)
{
  int res = this->AddStep(step);

  if (res)
    {
    int nb_steps = this->GetNumberOfSteps();
    if (nb_steps >= 2)
      {
      vtkKWWizardStep *prev_step = this->GetNthStep(nb_steps - 2);
      if (prev_step)
        {
        res = this->CreateNextTransition(
          prev_step, vtkKWWizardStep::GetValidationSucceededInput(), step);
        res &= this->CreateBackTransition(prev_step, step);
        }
      }
    }

  return res;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::CreateNextTransition(
  vtkKWWizardStep *origin, 
  vtkKWStateMachineInput *next_input,
  vtkKWWizardStep *destination)
{
  if (!origin || !destination)
    {
    return 0;
    }

  vtkKWStateMachineTransition *transition;

  // Transition from origin to destination

  transition = this->CreateTransition(
    origin->GetValidationState(),
    next_input,
    destination->GetInteractionState());

  if (transition)
    {
    transition->SetStartCommand(origin, "InvokeHideUserInterfaceCommand");
    }

  return  1;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::CreateBackTransition(
  vtkKWWizardStep *origin, 
  vtkKWWizardStep *destination)
{
  if (!origin || !destination)
    {
    return 0;
    }

  vtkKWStateMachineTransition *transition;

  // Transition from destination back to origin

  if (!this->FindTransition(destination->GetInteractionState(),
                            origin->GetGoBackToSelfInput(),
                            origin->GetInteractionState()))
    {
    if (!this->HasInput(origin->GetGoBackToSelfInput()))
      {
      this->AddInput(origin->GetGoBackToSelfInput());
      }
    
    transition = this->CreateTransition(
      destination->GetInteractionState(),
      origin->GetGoBackToSelfInput(),
      origin->GetInteractionState());

    if (transition)
      {
      transition->SetStartCommand(
        destination, "InvokeHideUserInterfaceCommand");
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::CreateGoToTransition(
  vtkKWWizardStep *origin, 
  vtkKWWizardStep *destination)
{
  if (!origin || !destination)
    {
    return 0;
    }

  vtkKWStateMachineTransition *transition;

  // Transition from origin to go-to hub

  if (!this->FindTransition(origin->GetInteractionState(),
                            destination->GetGoToSelfInput(),
                            this->GetGoToState()))
    {
    if (!this->HasInput(destination->GetGoToSelfInput()))
      {
      this->AddInput(destination->GetGoToSelfInput());
      }
      
    transition = this->CreateTransition(
      origin->GetInteractionState(),
      destination->GetGoToSelfInput(),
      this->GetGoToState());

    // The transition has a callback responsible for allowing us
    // to travel to the destination or not, through the go-to hub state. 
    // It will push the corresponding inputs for us.
    
    if (transition)
      {
      char command[256];
      sprintf(command, "TryToGoToStepCallback %s %s",
              origin->GetTclName(), destination->GetTclName());
      transition->SetEndCommand(this, command);
      }
    }

  // Transition from go-to hub to destination (i.e. we were allowed, and
  // the callback pushed destination->GetGoToSelfInput() for us).

  if (!this->FindTransition(this->GetGoToState(), 
                            destination->GetGoToSelfInput(),
                            destination->GetInteractionState()))
    {
    if (!this->HasInput(destination->GetGoToSelfInput()))
      {
      this->AddInput(destination->GetGoToSelfInput());
      }

    this->CreateTransition(
      this->GetGoToState(), 
      destination->GetGoToSelfInput(),
      destination->GetInteractionState());
    }
    
  // Transition from go-to hub back to origin (i.e. we were denied, and
  // the callback pushed origin->GetGoBackToSelfInput() for us.

  if (!this->FindTransition(this->GetGoToState(), 
                            origin->GetGoBackToSelfInput(),
                            origin->GetInteractionState()))
    {
    if (!this->HasInput(origin->GetGoToSelfInput()))
      {
      this->AddInput(origin->GetGoToSelfInput());
      }
    
    this->CreateTransition(
      this->GetGoToState(), 
      origin->GetGoBackToSelfInput(),
      origin->GetInteractionState());
    }

  // Transition from destination back to origin, as a convenience.

  this->CreateBackTransition(origin, destination);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::CreateGoToTransitions(
  vtkKWWizardStep *destination)
{
  if (!destination)
    {
    return 0;
    }
  int res = 1;
  vtkKWWizardWorkflowInternals::StepPoolIterator it = 
    this->Internals->StepPool.begin();
  vtkKWWizardWorkflowInternals::StepPoolIterator end = 
    this->Internals->StepPool.end();
  for (; it != end; ++it)
    {
    if ((*it) != destination)
      {
      res &= this->CreateGoToTransition((*it), destination);
      }
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::CreateGoToTransitionsToFinishStep()
{
  return this->CreateGoToTransitions(this->GetFinishStep());
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::GetNumberOfSteps()
{
  if (this->Internals)
    {
    return this->Internals->StepPool.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::GetNthStep(int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfSteps() || !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->StepPool[rank];
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::RemoveStep(vtkKWWizardStep *step)
{
  if (!step)
    {
    return;
    }

  // Remove step

  vtkObject *obj;

  vtkKWWizardWorkflowInternals::StepPoolIterator it = 
    this->Internals->StepPool.begin();
  vtkKWWizardWorkflowInternals::StepPoolIterator end = 
    this->Internals->StepPool.end();
  for (; it != end; ++it)
    {
    if ((*it) == step)
      {
      obj = (vtkObject*)(*it);
      (*it)->UnRegister(this);
      this->Internals->StepPool.erase(it);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::RemoveAllSteps()
{
  if (this->Internals)
    {
    // Inefficient but there might be too many things to do in 
    // RemoveStep, let's not duplicate and go out of sync

    while (this->Internals->StepPool.size())
      {
      this->RemoveStep(
        (*this->Internals->StepPool.begin()));
      }
    }
}

//----------------------------------------------------------------------------
vtkKWStateMachineState* vtkKWWizardWorkflow::GetGoToState()
{
  if (!this->GoToState)
    {
    this->GoToState = vtkKWStateMachineState::New();
    this->GoToState->SetName("Go To");
    this->AddState(this->GoToState);
    }
  return this->GoToState;
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::SetFinishStep(vtkKWWizardStep *arg)
{
  if (this->FinishStep == arg)
    {
    return;
    }

  this->FinishStep = arg;

  this->Modified();
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::GetFinishStep()
{
  if (this->FinishStep)
    {
    return this->FinishStep;
    }

  int nb_steps = this->GetNumberOfSteps();
  if (nb_steps)
    {
    return this->GetNthStep(nb_steps - 1);
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::GetStepFromState(
  vtkKWStateMachineState *state)
{
  if (this->Internals && state)
    {
    vtkKWWizardWorkflowInternals::StateToStepMapIterator it = 
      this->Internals->StateToStepPool.find(state);
    if (it != this->Internals->StateToStepPool.end())
      {
      return it->second;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::GetInitialStep()
{
  return this->GetStepFromState(this->GetInitialState());
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::SetInitialStep(vtkKWWizardStep *step)
{
  if (step)
    {
    return this->SetInitialState(step->GetInteractionState());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::GetCurrentStep()
{
  return this->GetStepFromState(this->GetCurrentState());
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::SetNavigationStackedChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->NavigationStackedChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::InvokeNavigationStackedChangedCommand()
{
  if (this->HasNavigationStackedChangedCommand())
    {
    this->InvokeObjectMethodCommand(this->NavigationStackedChangedCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::HasNavigationStackedChangedCommand()
{
  return this->NavigationStackedChangedCommand && *this->NavigationStackedChangedCommand;
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::InvokeCurrentStateChangedCommand()
{
  vtkKWWizardStep *previous_step = NULL;
  int nb_steps_in_stack = this->GetNumberOfStepsInNavigationStack();
  if (nb_steps_in_stack)
    {
    previous_step = this->GetNthStepInNavigationStack(nb_steps_in_stack - 1);
    }

  vtkKWWizardStep *current_step = this->GetCurrentStep();

  if (current_step && (!previous_step || previous_step != current_step))
    {
    this->PushStepToNavigationStack(current_step);
    }

  this->Superclass::InvokeCurrentStateChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::PushStepToNavigationStack(
  vtkKWWizardStep *step)
{
  if (step)
    {
    this->Internals->StepNavigationStackPool.push_back(step);
    this->InvokeNavigationStackedChangedCommand();
    this->InvokeEvent(vtkKWWizardWorkflow::NavigationStackedChangedEvent);
    }
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::PopStepFromNavigationStack()
{
  if (this->GetNumberOfStepsInNavigationStack())
    {
    vtkKWWizardStep *step = 
      this->Internals->StepNavigationStackPool.back();
    this->Internals->StepNavigationStackPool.pop_back();
    this->InvokeNavigationStackedChangedCommand();
    this->InvokeEvent(vtkKWWizardWorkflow::NavigationStackedChangedEvent);
    return step;
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWizardWorkflow::GetNumberOfStepsInNavigationStack()
{
  return this->Internals->StepNavigationStackPool.size();
}

//----------------------------------------------------------------------------
vtkKWWizardStep* vtkKWWizardWorkflow::GetNthStepInNavigationStack(
  int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfStepsInNavigationStack() || 
      !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->StepNavigationStackPool[rank];
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::AttemptToGoToPreviousStep()
{
  // Do we have a previous step in the navigation stack ?

  int nb_steps_in_stack = this->GetNumberOfStepsInNavigationStack();
  if (nb_steps_in_stack < 2) // both previous and current step on the stack?
    {
    return;
    }

  vtkKWWizardStep *previous_step = 
    this->GetNthStepInNavigationStack(nb_steps_in_stack - 2);
  vtkKWWizardStep *current_step = this->GetCurrentStep();

  if (previous_step && previous_step != current_step)
    {
    // We do, so let's try to go there

    this->PushInput(previous_step->GetGoBackToSelfInput());
    this->ProcessInputs();

    vtkKWWizardStep *new_current_step = this->GetCurrentStep();
    if (new_current_step == previous_step)
      {
      // If it worked:
      // Say our navigation stack so far was, in terms of steps, not states:
      //   a)  A -> B -> C -> D, 
      // we were at D, we tried to go back to C, succeeded, leading to:
      //   b)  A -> B -> C -> D -> C
      // but really what we want now is also be able to go back to B using
      // the same mechanism, since we had reached C from B. So we need to
      // remove the last two navigation entries:
      //   c)  A -> B -> C
      // so we are still at step C, but from the navigation point of view
      // going back would lead us to B this time, (not D according to b).
      // Now the problem is that we may had had more transitions in between
      // steps, i.e. instead of b) we had:
      //   b') A -> B -> C -> D -> ... -> ... -> C 
      // so let's just pop the navigation entries until we are back to a)
      // and also remove one more to actually be in c) (hence the >=, not >).

      while (this->GetNumberOfStepsInNavigationStack() >=
             nb_steps_in_stack)
        {
        this->PopStepFromNavigationStack();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::AttemptToGoToNextStep()
{
  this->PushInput(vtkKWWizardStep::GetValidationInput());
  this->ProcessInputs();
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::AttemptToGoToFinishStep()
{
  vtkKWWizardStep *finish_step = this->GetFinishStep();
  if (finish_step)
    {
    this->PushInput(finish_step->GetGoToSelfInput());
    this->ProcessInputs();
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::TryToGoToStepCallback(
  vtkKWWizardStep *origin, vtkKWWizardStep *destination)
{
  // We can go to destination, hide the origin UI, and go

  if (destination->InvokeCanGoToSelfCommand())
    {
    origin->InvokeHideUserInterfaceCommand();
    this->PushInput(destination->GetGoToSelfInput());
    }
  
  // We can not go to destination, go back to where we come from

  else
    {
    this->PushInput(origin->GetGoBackToSelfInput());
    }

  this->ProcessInputs();
}

//----------------------------------------------------------------------------
void vtkKWWizardWorkflow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GoToState: ";
  if (this->GoToState)
    {
    os << endl;
    this->GoToState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "FinishStep: ";
  if (this->FinishStep)
    {
    os << endl;
    this->FinishStep->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
