/*=========================================================================

  Module:    vtkKWStateMachineCluster.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineCluster.h"

#include "vtkKWStateMachineState.h"

#include "vtkObjectFactory.h"

#include <vtksys/stl/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineCluster);
vtkCxxRevisionMacro(vtkKWStateMachineCluster, "1.2");

vtkIdType vtkKWStateMachineCluster::IdCounter = 1;

//----------------------------------------------------------------------------
class vtkKWStateMachineClusterInternals
{
public:

  // States

  typedef vtksys_stl::vector<vtkKWStateMachineState*> StatePoolType;
  typedef vtksys_stl::vector<vtkKWStateMachineState*>::iterator StatePoolIterator;
  StatePoolType StatePool;
};

//----------------------------------------------------------------------------
vtkKWStateMachineCluster::vtkKWStateMachineCluster()
{
  this->Id = vtkKWStateMachineCluster::IdCounter++;
  this->Internals = new vtkKWStateMachineClusterInternals;
  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachineCluster::~vtkKWStateMachineCluster()
{
  this->RemoveAllStates();
  this->SetName(NULL);

  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
int vtkKWStateMachineCluster::HasState(vtkKWStateMachineState *state)
{
  if (state)
    {
    vtkKWStateMachineClusterInternals::StatePoolIterator it = 
      this->Internals->StatePool.begin();
    vtkKWStateMachineClusterInternals::StatePoolIterator end = 
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
int vtkKWStateMachineCluster::AddState(vtkKWStateMachineState *state)
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
int vtkKWStateMachineCluster::GetNumberOfStates()
{
  if (this->Internals)
    {
    return this->Internals->StatePool.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWStateMachineState* vtkKWStateMachineCluster::GetNthState(int rank)
{
  if (rank < 0 || rank >= this->GetNumberOfStates() || !this->Internals)
    {
    vtkErrorMacro("Index out of range");
    return NULL;
    }

  return this->Internals->StatePool[rank];
}

//----------------------------------------------------------------------------
void vtkKWStateMachineCluster::RemoveState(vtkKWStateMachineState *state)
{
  if (!state)
    {
    return;
    }

  // Remove state

  vtkObject *obj;

  vtkKWStateMachineClusterInternals::StatePoolIterator it = 
    this->Internals->StatePool.begin();
  vtkKWStateMachineClusterInternals::StatePoolIterator end = 
    this->Internals->StatePool.end();
  for (; it != end; ++it)
    {
    if ((*it) == state)
      {
      obj = (vtkObject*)(*it);
      (*it)->UnRegister(this);
      this->Internals->StatePool.erase(it);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachineCluster::RemoveAllStates()
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
void vtkKWStateMachineCluster::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << (this->Name ? this->Name : "None") << endl;
}
