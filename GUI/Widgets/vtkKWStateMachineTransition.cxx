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
vtkCxxRevisionMacro(vtkKWStateMachineTransition, "1.2");

vtkIdType vtkKWStateMachineTransition::IdCounter = 1;

//----------------------------------------------------------------------------
vtkKWStateMachineTransition::vtkKWStateMachineTransition()
{
  this->Id = vtkKWStateMachineTransition::IdCounter++;
  this->OriginState = NULL;
  this->Input = NULL;
  this->DestinationState = NULL;

  this->EndCommand = NULL;
  this->StartCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition::~vtkKWStateMachineTransition()
{
  this->SetOriginState(NULL);
  this->SetInput(NULL);
  this->SetDestinationState(NULL);

  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    this->EndCommand = NULL;
    }
  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    this->StartCommand = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachineTransition::IsComplete()
{
  return (this->OriginState && this->Input && this->DestinationState);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::End()
{
  this->InvokeEndCommand();
  this->InvokeEvent(vtkKWStateMachineTransition::EndEvent);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::SetEndCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->EndCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::InvokeEndCommand()
{
  if (this->HasEndCommand())
    {
    this->InvokeObjectMethodCommand(this->EndCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachineTransition::HasEndCommand()
{
  return this->EndCommand && *this->EndCommand;
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::Start()
{
  this->InvokeStartCommand();
  this->InvokeEvent(vtkKWStateMachineTransition::StartEvent);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::SetStartCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->StartCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::InvokeStartCommand()
{
  if (this->HasStartCommand())
    {
    this->InvokeObjectMethodCommand(this->StartCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWStateMachineTransition::HasStartCommand()
{
  return this->StartCommand && *this->StartCommand;
}

//----------------------------------------------------------------------------
void vtkKWStateMachineTransition::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Id: " << this->Id << endl;

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
