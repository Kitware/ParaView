/*=========================================================================

  Module:    vtkKWStateMachineState.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineState.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineState);
vtkCxxRevisionMacro(vtkKWStateMachineState, "1.2");

vtkIdType vtkKWStateMachineState::IdCounter = 1;

//----------------------------------------------------------------------------
vtkKWStateMachineState::vtkKWStateMachineState()
{
  this->Id = vtkKWStateMachineState::IdCounter++;
  this->Name = NULL;
  this->Description = NULL;
  this->Accepting = 0;

  this->EnterCommand = NULL;
  this->LeaveCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachineState::~vtkKWStateMachineState()
{
  this->SetName(NULL);
  this->SetDescription(NULL);
  if (this->EnterCommand)
    {
    delete [] this->EnterCommand;
    this->EnterCommand = NULL;
    }
  if (this->LeaveCommand)
    {
    delete [] this->LeaveCommand;
    this->LeaveCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::Enter()
{
  this->InvokeEnterCommand();
  this->InvokeEvent(vtkKWStateMachineState::EnterEvent);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::SetEnterCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->EnterCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::InvokeEnterCommand()
{
  if (this->HasEnterCommand())
    {
    this->InvokeObjectMethodCommand(this->EnterCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachineState::HasEnterCommand()
{
  return this->EnterCommand && *this->EnterCommand;
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::Leave()
{
  this->InvokeLeaveCommand();
  this->InvokeEvent(vtkKWStateMachineState::LeaveEvent);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::SetLeaveCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->LeaveCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::InvokeLeaveCommand()
{
  if (this->HasLeaveCommand())
    {
    this->InvokeObjectMethodCommand(this->LeaveCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachineState::HasLeaveCommand()
{
  return this->LeaveCommand && *this->LeaveCommand;
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Id: " << this->Id << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "None") << endl;
  os << indent << "Description: " 
     << (this->Description ? this->Description : "None") << endl;
  os << indent << "Accepting: " 
     << (this->Accepting ? "On" : "Off") << endl;
}
