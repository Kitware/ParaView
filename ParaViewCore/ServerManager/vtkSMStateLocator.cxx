/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"

//***************************************************************************
//                        Internal class Definition
//***************************************************************************
class vtkSMStateLocator::vtkInternal {
public:
  void RegisterState(vtkTypeUInt32 globalId, const vtkSMMessage* state)
    {
    this->StateMap[globalId].CopyFrom(*state);
    }

  void UnRegisterState(vtkTypeUInt32 globalId)
    {
    this->StateMap.erase(globalId);
    }

  bool FindState( vtkTypeUInt32 globalId, vtkSMMessage* stateToFill )
    {
    if(this->StateMap.find(globalId) != this->StateMap.end())
      {
      if(stateToFill)
        {
        stateToFill->CopyFrom(this->StateMap[globalId]);
        }
      return true;
      }
    return false;
    }

private:
  vtkstd::map<vtkTypeUInt32, vtkSMMessage> StateMap;
};
//***************************************************************************
vtkStandardNewMacro(vtkSMStateLocator);
//---------------------------------------------------------------------------
vtkSMStateLocator::vtkSMStateLocator()
{
  this->ParentLocator = 0;
  this->Internals = new vtkInternal();
}

//---------------------------------------------------------------------------
vtkSMStateLocator::~vtkSMStateLocator()
{
  this->SetParentLocator(0);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMStateLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
bool vtkSMStateLocator::FindState(vtkTypeUInt32 globalID,
                                  vtkSMMessage* stateToFill )
{
  if(stateToFill == NULL)
    {
    return false;
    }
  else
    {
    stateToFill->Clear();
    }

  if(this->Internals->FindState(globalID, stateToFill))
    {
    return true;
    }
  if(this->ParentLocator)
    {
    return this->ParentLocator->FindState(globalID, stateToFill);
    }
  return false;
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::RegisterState( const vtkSMMessage* state )
{
  this->Internals->RegisterState(state->global_id(), state);
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::UnRegisterState( vtkTypeUInt32 globalID, bool force )
{
  this->Internals->UnRegisterState(globalID);
  if(force && this->ParentLocator)
    {
    this->ParentLocator->UnRegisterState(globalID, force);
    }
}
//---------------------------------------------------------------------------
bool vtkSMStateLocator::IsStateLocal( vtkTypeUInt32 globalID )
{
  return this->Internals->FindState(globalID, NULL);
}
//---------------------------------------------------------------------------
bool vtkSMStateLocator::IsStateAvailable(vtkTypeUInt32 globalID)
{
  return ( this->IsStateLocal(globalID) ||
           ( this->ParentLocator &&
             this->ParentLocator->IsStateAvailable(globalID)));
}
