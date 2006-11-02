/*=========================================================================

  Module:    vtkKWStateMachineTransition.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineTransition.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include "vtkKWStateMachineState.h"
#include "vtkKWStateMachineInput.h"

vtkCxxSetObjectMacro(vtkKWStateMachineTransition,OriginState,vtkKWStateMachineState);
vtkCxxSetObjectMacro(vtkKWStateMachineTransition,Input,vtkKWStateMachineInput);
vtkCxxSetObjectMacro(vtkKWStateMachineTransition,DestinationState,vtkKWStateMachineState);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineTransition);
vtkCxxRevisionMacro(vtkKWStateMachineTransition, "1.1");

vtkIdType vtkKWStateMachineTransition::IdCounter = 1;

//----------------------------------------------------------------------------
vtkKWStateMachineTransition::vtkKWStateMachineTransition()
{
  this->Id = vtkKWStateMachineTransition::IdCounter++;
  this->OriginState = NULL;
  this->Input = NULL;
  this->DestinationState = NULL;
  this->Command = NULL;
  this->Event = vtkCommand::NoEvent;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition::~vtkKWStateMachineTransition()
{
  this->SetOriginState(NULL);
  this->SetInput(NULL);
  this->SetDestinationState(NULL);

  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachineTransition::IsComplete()
{
  return (this->OriginState && this->Input && this->DestinationState);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::TriggerAction()
{
  this->InvokeCommand();
  this->InvokeEvent(this->Event);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::SetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::InvokeCommand()
{
  this->InvokeObjectMethodCommand(this->Command);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Id: " << this->Id << endl;
  os << indent << "Event: " << this->Event << endl;

  os << indent << "OriginState: ";
  if (this->OriginState)
    {
    os << endl;
    this->OriginState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "Input: ";
  if (this->Input)
    {
    os << endl;
    this->Input->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "DestinationState: ";
  if (this->DestinationState)
    {
    os << endl;
    this->DestinationState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
