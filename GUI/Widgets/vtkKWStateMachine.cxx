/*=========================================================================

  Module:    vtkKWStateMachine.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachine.h"

#include "vtkKWStateMachineInput.h"
#include "vtkKWStateMachineState.h"
#include "vtkKWStateMachineTransition.h"
#include "vtkKWStateMachineCluster.h"

#include "vtkObjectFactory.h"

#include <vtksys/stl/string>
#include <vtksys/stl/map>
#include <vtksys/stl/vector>
#include <vtksys/stl/deque>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachine);
vtkCxxRevisionMacro(vtkKWStateMachine, "1.3");

//----------------------------------------------------------------------------
class vtkKWStateMachineInternals
{
public:

  // States

  typedef vtksys_stl::vector<vtkKWStateMachineState*> StatePoolType;
  typedef vtksys_stl::vector<vtkKWStateMachineState*>::iterator StatePoolIterator;
  StatePoolType StatePool;

  // Inputs

  typedef vtksys_stl::vector<vtkKWStateMachineInput*> InputPoolType;
  typedef vtksys_stl::vector<vtkKWStateMachineInput*>::iterator InputPoolIterator;
  InputPoolType InputPool;

  // Transitions

  typedef vtksys_stl::vector<vtkKWStateMachineTransition*> TransitionPoolType;
  typedef vtksys_stl::vector<vtkKWStateMachineTransition*>::iterator TransitionPoolIterator;
  TransitionPoolType TransitionPool;

  TransitionPoolType TransitionHistoryPool;

  // Clusters

  typedef vtksys_stl::vector<vtkKWStateMachineCluster*> ClusterPoolType;
  typedef vtksys_stl::vector<vtkKWStateMachineCluster*>::iterator ClusterPoolIterator;
  ClusterPoolType ClusterPool;

  // Input queue

  typedef vtksys_stl::deque<vtkKWStateMachineInput*> InputQueueType;
  typedef vtksys_stl::deque<vtkKWStateMachineInput*>::iterator InputQueueIterator;
  InputQueueType InputQueue;

  // Input To Transition map
  // State to Input map
  // => transition tables

  typedef vtksys_stl::map<vtkKWStateMachineInput*, vtkKWStateMachineTransition*> InputToTransitionMapType;
  typedef vtksys_stl::map<vtkKWStateMachineInput*, vtkKWStateMachineTransition*>::iterator InputToTransitionMapIterator;

  typedef vtksys_stl::map<vtkKWStateMachineState*, InputToTransitionMapType> StateToInputMapType;
  typedef vtksys_stl::map<vtkKWStateMachineState*, InputToTransitionMapType>::iterator StateToInputMapIterator;

  StateToInputMapType TransitionTable;
};

//----------------------------------------------------------------------------
vtkKWStateMachine::vtkKWStateMachine()
{
  this->Internals = new vtkKWStateMachineInternals;

  this->InitialState = NULL;
  this->CurrentState = NULL;

  this->CurrentStateChangedCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachine::~vtkKWStateMachine()
{
  this->RemoveAllTransitions();
  this->RemoveAllStates();
  this->RemoveAllInputs();
  this->RemoveAllClusters();

  delete this->Internals;
  this->Internals = NULL;

  if (this->CurrentStateChangedCommand)
    {
    delete [] this->CurrentStateChangedCommand;
    this->CurrentStateChangedCommand = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::HasState(vtkKWStateMachineState *state)
{
  if (state)
    {
    vtkKWStateMachineInternals::StatePoolIterator it = 
      this->Internals->StatePool.begin();
    vtkKWStateMachineInternals::StatePoolIterator end = 
      this->Internals->StatePool.end();
    for (; it != end; ++it)
      {
      if ((*it) == state)
        {
        return 1;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::AddState(vtkKWStateMachineState *state)
{
  if (!state)
    {
    vtkErrorMacro("Can not add NULL state to pool!");
    return 0;
    }

  // Already in the pool ?

  if (this->HasState(state))
    {
    vtkErrorMacro("The state is already in the pool!");
    return 0;
    }
  
  if (!state->GetApplication())
    {
    state->SetApplication(this->GetApplication());
    }

  this->Internals->StatePool.push_back(state);
  state->Register(this);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::GetNumberOfStates()
{
  if (this->Internals)
    {
    return this->Internals->StatePool.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWStateMachineState* vtkKWStateMachine::GetNthState(int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfStates() || !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->StatePool[rank];
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveState(vtkKWStateMachineState *state)
{
  if (!state)
    {
    return;
    }

  // Remove state

  vtkKWStateMachineInternals::StatePoolIterator it = 
    this->Internals->StatePool.begin();
  vtkKWStateMachineInternals::StatePoolIterator end = 
    this->Internals->StatePool.end();
  for (; it != end; ++it)
    {
    if ((*it) == state)
      {
      (*it)->UnRegister(this);
      this->Internals->StatePool.erase(it);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveAllStates()
{
  if (this->Internals)
    {
    // Inefficient but there might be too many things to do in 
    // RemoveState, let's not duplicate and go out of sync

    while (this->Internals->StatePool.size())
      {
      this->RemoveState(
        (*this->Internals->StatePool.begin()));
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::HasInput(vtkKWStateMachineInput *input)
{
  if (input)
    {
    vtkKWStateMachineInternals::InputPoolIterator it = 
      this->Internals->InputPool.begin();
    vtkKWStateMachineInternals::InputPoolIterator end = 
      this->Internals->InputPool.end();
    for (; it != end; ++it)
      {
      if ((*it) == input)
        {
        return 1;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::AddInput(vtkKWStateMachineInput *input)
{
  if (!input)
    {
    vtkErrorMacro("Can not add NULL input to pool!");
    return 0;
    }

  // Already in the pool ?

  if (this->HasInput(input))
    {
    vtkErrorMacro("The input is already in the pool!");
    return 0;
    }
  
  if (!input->GetApplication())
    {
    input->SetApplication(this->GetApplication());
    }

  this->Internals->InputPool.push_back(input);
  input->Register(this);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::GetNumberOfInputs()
{
  if (this->Internals)
    {
    return this->Internals->InputPool.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput* vtkKWStateMachine::GetNthInput(int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfInputs() || !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->InputPool[rank];
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveInput(vtkKWStateMachineInput *input)
{
  if (!input)
    {
    return;
    }

  // Remove input

  vtkKWStateMachineInternals::InputPoolIterator it = 
    this->Internals->InputPool.begin();
  vtkKWStateMachineInternals::InputPoolIterator end = 
    this->Internals->InputPool.end();
  for (; it != end; ++it)
    {
    if ((*it) == input)
      {
      (*it)->UnRegister(this);
      this->Internals->InputPool.erase(it);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveAllInputs()
{
  if (this->Internals)
    {
    // Inefficient but there might be too many things to do in 
    // RemoveInput, let's not duplicate and go out of sync

    while (this->Internals->InputPool.size())
      {
      this->RemoveInput(
        (*this->Internals->InputPool.begin()));
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::HasTransition(vtkKWStateMachineTransition *transition)
{
  if (transition)
    {
    vtkKWStateMachineInternals::TransitionPoolIterator it = 
      this->Internals->TransitionPool.begin();
    vtkKWStateMachineInternals::TransitionPoolIterator end = 
      this->Internals->TransitionPool.end();
    for (; it != end; ++it)
      {
      if ((*it) == transition)
        {
        return 1;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::AddTransition(vtkKWStateMachineTransition *transition)
{
  if (!transition)
    {
    vtkErrorMacro("Can not add NULL transition to pool!");
    return 0;
    }

  // Already in the pool ?

  if (this->HasTransition(transition))
    {
    vtkErrorMacro("The transition is already in the pool!");
    return 0;
    }

  // Transition is complete ?

  if (!transition->IsComplete())
    {
    vtkErrorMacro("The transition is not complete! It is either missing its originating state, destination state, and/or input!");
    return 0;
    }

  // Parameters are known

  if (!this->HasState(transition->GetOriginState()))
    {
    vtkErrorMacro("Can not add a transition originating from a state unknown to the machine!");
    return 0;
    }

  if (!this->HasInput(transition->GetInput()))
    {
    vtkErrorMacro("Can not add a transition triggered by an input unknown to the machine!");
    return 0;
    }
  
  if (!this->HasState(transition->GetDestinationState()))
    {
    vtkErrorMacro("Can not add a transition leading to a state unknown to the machine!");
    return 0;
    }

  if (!transition->GetApplication())
    {
    transition->SetApplication(this->GetApplication());
    }

  this->Internals->TransitionPool.push_back(transition);
  transition->Register(this);

  // Add the transition to our table

  vtkKWStateMachineInternals::InputToTransitionMapType &input_to_transition =
    this->Internals->TransitionTable[transition->GetOriginState()];
  input_to_transition[transition->GetInput()] = transition;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::GetNumberOfTransitions()
{
  if (this->Internals)
    {
    return this->Internals->TransitionPool.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWStateMachine::GetNthTransition(int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfTransitions() || !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->TransitionPool[rank];
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveTransition(vtkKWStateMachineTransition *transition)
{
  if (!transition)
    {
    return;
    }

  // Remove transition

  vtkKWStateMachineInternals::TransitionPoolIterator it = 
    this->Internals->TransitionPool.begin();
  vtkKWStateMachineInternals::TransitionPoolIterator end = 
    this->Internals->TransitionPool.end();
  for (; it != end; ++it)
    {
    if ((*it) == transition)
      {
      (*it)->UnRegister(this);
      this->Internals->TransitionPool.erase(it);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveAllTransitions()
{
  if (this->Internals)
    {
    // Inefficient but there might be too many things to do in 
    // RemoveTransition, let's not duplicate and go out of sync

    while (this->Internals->TransitionPool.size())
      {
      this->RemoveTransition(
        (*this->Internals->TransitionPool.begin()));
      }
    }
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWStateMachine::CreateTransition(
  vtkKWStateMachineState *origin,
  vtkKWStateMachineInput *input,
  vtkKWStateMachineState *destination)
{
  if (!origin || !input || !destination)
    {
    vtkErrorMacro("Can not create transition with incomplete parameters!");
    return NULL;
    }
  
  vtkKWStateMachineTransition *transition =
    this->FindTransition(origin, input, destination);
  if (transition)
    {
    return transition;
    }
  
  transition = vtkKWStateMachineTransition::New();
  transition->SetOriginState(origin);
  transition->SetInput(input);
  transition->SetDestinationState(destination);

  int res = this->AddTransition(transition);
  transition->Delete();
  return res ? transition : NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWStateMachine::FindTransition(
  vtkKWStateMachineState *origin,
  vtkKWStateMachineInput *input)
{
  // Find all transitions for the state

  if (this->Internals->TransitionTable.find(origin) ==
      this->Internals->TransitionTable.end())
    {
    return NULL;
    }

  vtkKWStateMachineInternals::InputToTransitionMapType &input_to_transition =
    this->Internals->TransitionTable[origin];

  // Find the specific transition for the state and the input

  vtkKWStateMachineInternals::InputToTransitionMapIterator it = 
    input_to_transition.find(input);
  if (it == input_to_transition.end())
    {
    return NULL;
    }
  
  return it->second;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWStateMachine::FindTransition(
  vtkKWStateMachineState *origin,
  vtkKWStateMachineInput *input,
  vtkKWStateMachineState *destination)
{
  vtkKWStateMachineTransition *transition = 
    this->FindTransition(origin, input);
  if (transition && transition->GetDestinationState() == destination)
    {
    return transition;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::PushTransitionToHistory(
  vtkKWStateMachineTransition *transition)
{
  if (transition)
    {
    this->Internals->TransitionHistoryPool.push_back(transition);
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::GetNumberOfTransitionsInHistory()
{
  return this->Internals->TransitionHistoryPool.size();
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWStateMachine::GetNthTransitionInHistory(
  int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfTransitionsInHistory() || 
      !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->TransitionHistoryPool[rank];
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::HasCluster(vtkKWStateMachineCluster *cluster)
{
  if (cluster)
    {
    vtkKWStateMachineInternals::ClusterPoolIterator it = 
      this->Internals->ClusterPool.begin();
    vtkKWStateMachineInternals::ClusterPoolIterator end = 
      this->Internals->ClusterPool.end();
    for (; it != end; ++it)
      {
      if ((*it) == cluster)
        {
        return 1;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::AddCluster(vtkKWStateMachineCluster *cluster)
{
  if (!cluster)
    {
    vtkErrorMacro("Can not add NULL cluster to pool!");
    return 0;
    }

  // Already in the pool ?

  if (this->HasCluster(cluster))
    {
    vtkErrorMacro("The cluster is already in the pool!");
    return 0;
    }
  
  if (!cluster->GetApplication())
    {
    cluster->SetApplication(this->GetApplication());
    }

  this->Internals->ClusterPool.push_back(cluster);
  cluster->Register(this);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::GetNumberOfClusters()
{
  if (this->Internals)
    {
    return this->Internals->ClusterPool.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWStateMachineCluster* vtkKWStateMachine::GetNthCluster(int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfClusters() || !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->ClusterPool[rank];
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveCluster(vtkKWStateMachineCluster *cluster)
{
  if (!cluster)
    {
    return;
    }

  // Remove cluster

  vtkKWStateMachineInternals::ClusterPoolIterator it = 
    this->Internals->ClusterPool.begin();
  vtkKWStateMachineInternals::ClusterPoolIterator end = 
    this->Internals->ClusterPool.end();
  for (; it != end; ++it)
    {
    if ((*it) == cluster)
      {
      (*it)->UnRegister(this);
      this->Internals->ClusterPool.erase(it);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::RemoveAllClusters()
{
  if (this->Internals)
    {
    // Inefficient but there might be too many things to do in 
    // RemoveCluster, let's not duplicate and go out of sync

    while (this->Internals->ClusterPool.size())
      {
      this->RemoveCluster(
        (*this->Internals->ClusterPool.begin()));
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::SetInitialState(vtkKWStateMachineState *state)
{
  if (this->InitialState)
    {
    vtkErrorMacro("Not allowed to reset the initial state!");
    return 0;
    }

  if (!state)
    {
    return 0;
    }

  this->InitialState = state;
  this->CurrentState = state;
  this->Modified();

  this->InvokeCurrentStateChangedCommand();
  this->InvokeEvent(vtkKWStateMachine::CurrentStateChangedEvent);

  this->CurrentState->Enter();

  return 1;
}

//----------------------------------------------------------------------------
vtkKWStateMachineState* vtkKWStateMachine::GetPreviousState()
{
  if (this->Internals && this->GetNumberOfTransitionsInHistory())
    {
    vtkKWStateMachineTransition *transition = this->GetNthTransitionInHistory(
      this->GetNumberOfTransitionsInHistory() - 1);
    if (transition)
      {
      return transition->GetOriginState();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::PushInput(vtkKWStateMachineInput *input)
{
  if (this->Internals)
    {
    this->Internals->InputQueue.push_back(input);
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::ProcessInputs()
{
  if (this->Internals)
    {
    while (!this->Internals->InputQueue.empty())
      {
      vtkKWStateMachineInput *input = this->Internals->InputQueue.front();
      this->Internals->InputQueue.pop_front();
      this->ProcessInput(input);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::ProcessInput(vtkKWStateMachineInput *input)
{
  if (!this->CurrentState)
    {
    vtkErrorMacro("The initial state has not been defined!");
    return;
    }

  // Find the specific transition for the current state and this input

  vtkKWStateMachineTransition *transition = 
    this->FindTransition(this->CurrentState, input);
  if (!transition)
    {
    ostrstream err;
    err << "No transition has been defined for the current state (";
    if (this->CurrentState->GetName())
      {
      err << this->CurrentState->GetName();
      }
    else
      {
      err << this->CurrentState->GetId();
      }
    err << ") given this input (";
    if (input->GetName())
      {
      err << input->GetName();
      }
    else
      {
      err << input->GetId();
      }
    err << ")!" << ends;
    vtkWarningMacro(<< err.str());
    return;
    }

  // Leave the current state

  transition->Start();

  if (this->CurrentState)
    {
    this->CurrentState->Leave();
    }

  this->PushTransitionToHistory(transition);

  // Go to the new state

  this->CurrentState = transition->GetDestinationState();

  this->InvokeCurrentStateChangedCommand();
  this->InvokeEvent(vtkKWStateMachine::CurrentStateChangedEvent);

  if (this->CurrentState)
    {
    this->CurrentState->Enter();
    }
    
  transition->End();
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::SetCurrentStateChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->CurrentStateChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::InvokeCurrentStateChangedCommand()
{
  if (this->HasCurrentStateChangedCommand())
    {
    this->InvokeObjectMethodCommand(this->CurrentStateChangedCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachine::HasCurrentStateChangedCommand()
{
  return this->CurrentStateChangedCommand && *this->CurrentStateChangedCommand;
}

//----------------------------------------------------------------------------
void vtkKWStateMachine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InitialState: ";
  if (this->InitialState)
    {
    os << endl;
    this->InitialState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "CurrentState: ";
  if (this->CurrentState)
    {
    os << endl;
    this->CurrentState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
