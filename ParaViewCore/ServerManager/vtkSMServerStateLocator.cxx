/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerStateLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMServerStateLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMServerStateLocator);
//---------------------------------------------------------------------------
vtkSMServerStateLocator::vtkSMServerStateLocator()
{
  this->Session = NULL;
}

//---------------------------------------------------------------------------
vtkSMServerStateLocator::~vtkSMServerStateLocator()
{
  this->SetSession(NULL);
}

//---------------------------------------------------------------------------
void vtkSMServerStateLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
bool vtkSMServerStateLocator::FindState(vtkTypeUInt32 globalID,
                                        vtkSMMessage* stateToFill )
{
  if(!this->Superclass::FindState(globalID, stateToFill) && this->Session)
    {
    vtkSMMessage newState;
    newState.set_global_id(globalID);
    newState.set_location(vtkPVSession::DATA_SERVER_ROOT);
    this->Session->PullState(&newState);
    this->RegisterState(&newState);
    stateToFill->Clear();
    stateToFill->CopyFrom(newState);
    return newState.HasExtension(ProxyState::xml_group);
    }

  // Not found
  return false;
}
//---------------------------------------------------------------------------
vtkSMSession* vtkSMServerStateLocator::GetSession()
{
  return this->Session.GetPointer();
}
//---------------------------------------------------------------------------
void vtkSMServerStateLocator::SetSession(vtkSMSession* session)
{
  this->Session = session;
}
