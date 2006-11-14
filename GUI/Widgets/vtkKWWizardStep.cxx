/*=========================================================================

  Module:    vtkKWWizardStep.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWizardStep.h"

#include "vtkKWStateMachineState.h"
#include "vtkKWStateMachineInput.h"
#include "vtkKWStateMachineTransition.h"
#include "vtkKWTkUtilities.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWizardStep);
vtkCxxRevisionMacro(vtkKWWizardStep, "1.1");

vtkIdType vtkKWWizardStep::IdCounter = 1;
vtkKWWizardStepCleanup vtkKWWizardStep::Cleanup;
vtkKWStateMachineInput* vtkKWWizardStep::ValidationInput = NULL;
vtkKWStateMachineInput* vtkKWWizardStep::ValidationSucceededInput = NULL;
vtkKWStateMachineInput* vtkKWWizardStep::ValidationFailedInput = NULL;

//----------------------------------------------------------------------------
vtkKWWizardStepCleanup::~vtkKWWizardStepCleanup()
{
  vtkKWWizardStep::SetValidationInput(NULL);
  vtkKWWizardStep::SetValidationSucceededInput(NULL);
  vtkKWWizardStep::SetValidationFailedInput(NULL);
}

//----------------------------------------------------------------------------
vtkKWWizardStep::vtkKWWizardStep()
{
  this->Id                         = vtkKWWizardStep::IdCounter++;
  this->Name                       = NULL;
  this->Description                = NULL;
  this->InteractionState           = NULL;
  this->ValidationState            = NULL;
  this->ValidationTransition       = NULL;
  this->ValidationFailedTransition = NULL;
  this->GoToSelfInput              = NULL;
  this->GoBackToSelfInput          = NULL;
  this->ShowUserInterfaceCommand   = NULL;
  this->HideUserInterfaceCommand   = NULL;
  this->ValidationCommand          = NULL;
  this->CanGoToSelfCommand         = NULL;
}

//----------------------------------------------------------------------------
vtkKWWizardStep::~vtkKWWizardStep()
{
  if (this->InteractionState)
    {
    this->InteractionState->Delete();
    this->InteractionState = NULL;
    }

  if (this->ValidationState)
    {
    this->ValidationState->Delete();
    this->ValidationState = NULL;
    }

  if (this->ValidationTransition)
    {
    this->ValidationTransition->Delete();
    this->ValidationTransition = NULL;
    }

  if (this->ValidationFailedTransition)
    {
    this->ValidationFailedTransition->Delete();
    this->ValidationFailedTransition = NULL;
    }

  if (this->GoToSelfInput)
    {
    this->GoToSelfInput->Delete();
    this->GoToSelfInput = NULL;
    }

  if (this->GoBackToSelfInput)
    {
    this->GoBackToSelfInput->Delete();
    this->GoBackToSelfInput = NULL;
    }

  if (this->ShowUserInterfaceCommand)
    {
    delete [] this->ShowUserInterfaceCommand;
    this->ShowUserInterfaceCommand = NULL;
    }

  if (this->HideUserInterfaceCommand)
    {
    delete [] this->HideUserInterfaceCommand;
    this->HideUserInterfaceCommand = NULL;
    }

  if (this->ValidationCommand)
    {
    delete [] this->ValidationCommand;
    this->ValidationCommand = NULL;
    }

  if (this->CanGoToSelfCommand)
    {
    delete [] this->CanGoToSelfCommand;
    this->CanGoToSelfCommand = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWStateMachineState* vtkKWWizardStep::GetInteractionState()
{
  if (!this->InteractionState)
    {
    this->InteractionState = vtkKWStateMachineState::New();
    ostrstream name;
    if (this->Name)
      {
      name << this->Name;
      }
    else
      {
      name << this->Id;
      }
    name << "|" << "I" << ends;
    this->InteractionState->SetName(name.str());
    this->AddCallbackCommandObserver(
      this->InteractionState, vtkKWStateMachineState::EnterEvent);
    }
  return this->InteractionState;
}

//----------------------------------------------------------------------------
vtkKWStateMachineState* vtkKWWizardStep::GetValidationState()
{
  if (!this->ValidationState)
    {
    this->ValidationState = vtkKWStateMachineState::New();
    ostrstream name;
    if (this->Name)
      {
      name << this->Name;
      }
    else
      {
      name << this->Id;
      }
    name << "|" << "V" << ends;
    this->ValidationState->SetName(name.str());
    }
  return this->ValidationState;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWWizardStep::GetValidationTransition()
{
  if (!this->ValidationTransition)
    {
    this->ValidationTransition = vtkKWStateMachineTransition::New();
    this->ValidationTransition->SetOriginState(
      this->GetInteractionState());
    this->ValidationTransition->SetInput(
      vtkKWWizardStep::GetValidationInput());
    this->ValidationTransition->SetDestinationState(
      this->GetValidationState());
    this->AddCallbackCommandObserver(
      this->ValidationTransition, vtkKWStateMachineTransition::EndEvent);
    }
  return this->ValidationTransition;
}

//----------------------------------------------------------------------------
vtkKWStateMachineTransition* vtkKWWizardStep::GetValidationFailedTransition()
{
  if (!this->ValidationFailedTransition)
    {
    this->ValidationFailedTransition = vtkKWStateMachineTransition::New();
    this->ValidationFailedTransition->SetOriginState(
      this->GetValidationState());
    this->ValidationFailedTransition->SetInput(
      vtkKWWizardStep::GetValidationFailedInput());
    this->ValidationFailedTransition->SetDestinationState(
      this->GetInteractionState());
    }
  return this->ValidationFailedTransition;
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput* vtkKWWizardStep::GetGoToSelfInput()
{
  if (!this->GoToSelfInput)
    {
    this->GoToSelfInput = vtkKWStateMachineInput::New();
    ostrstream name;
    name << "go to: ";
    if (this->Name)
      {
      name << this->Name;
      }
    else
      {
      name << this->Id;
      }
    name << ends;
    this->GoToSelfInput->SetName(name.str());
    }
  return this->GoToSelfInput;
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput* vtkKWWizardStep::GetGoBackToSelfInput()
{
  if (!this->GoBackToSelfInput)
    {
    this->GoBackToSelfInput = vtkKWStateMachineInput::New();
    ostrstream name;
    name << "back to: ";
    if (this->Name)
      {
      name << this->Name;
      }
    else
      {
      name << this->Id;
      }
    name << ends;
    this->GoBackToSelfInput->SetName(name.str());
    }
  return this->GoBackToSelfInput;
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput* vtkKWWizardStep::GetValidationInput()
{
  if (!vtkKWWizardStep::ValidationInput)
    {
    vtkKWWizardStep::ValidationInput = vtkKWStateMachineInput::New();
    vtkKWWizardStep::ValidationInput->SetName("validate");
    }
  return vtkKWWizardStep::ValidationInput;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetValidationInput(vtkKWStateMachineInput *input)
{
  if (vtkKWWizardStep::ValidationInput == input)
    {
    return;
    }

  if (vtkKWWizardStep::ValidationInput)
    {
    vtkKWWizardStep::ValidationInput->Delete();;
    }

  vtkKWWizardStep::ValidationInput = input;
  if (input)
    {
    input->Register(NULL);
    }
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput* vtkKWWizardStep::GetValidationSucceededInput()
{
  if (!vtkKWWizardStep::ValidationSucceededInput)
    {
    vtkKWWizardStep::ValidationSucceededInput = vtkKWStateMachineInput::New();
    vtkKWWizardStep::ValidationSucceededInput->SetName("valid");
    }
  return vtkKWWizardStep::ValidationSucceededInput;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetValidationSucceededInput(vtkKWStateMachineInput *input)
{
  if (vtkKWWizardStep::ValidationSucceededInput == input)
    {
    return;
    }

  if (vtkKWWizardStep::ValidationSucceededInput)
    {
    vtkKWWizardStep::ValidationSucceededInput->Delete();;
    }

  vtkKWWizardStep::ValidationSucceededInput = input;
  if (input)
    {
    input->Register(NULL);
    }
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput* vtkKWWizardStep::GetValidationFailedInput()
{
  if (!vtkKWWizardStep::ValidationFailedInput)
    {
    vtkKWWizardStep::ValidationFailedInput = vtkKWStateMachineInput::New();
    vtkKWWizardStep::ValidationFailedInput->SetName("invalid");
    }
  return vtkKWWizardStep::ValidationFailedInput;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetValidationFailedInput(vtkKWStateMachineInput *input)
{
  if (vtkKWWizardStep::ValidationFailedInput == input)
    {
    return;
    }

  if (vtkKWWizardStep::ValidationFailedInput)
    {
    vtkKWWizardStep::ValidationFailedInput->Delete();;
    }

  vtkKWWizardStep::ValidationFailedInput = input;
  if (input)
    {
    input->Register(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetShowUserInterfaceCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->ShowUserInterfaceCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::InvokeShowUserInterfaceCommand()
{
  if (this->HasShowUserInterfaceCommand())
    {
    this->InvokeObjectMethodCommand(this->ShowUserInterfaceCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWWizardStep::HasShowUserInterfaceCommand()
{
  return this->ShowUserInterfaceCommand && *this->ShowUserInterfaceCommand;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetHideUserInterfaceCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->HideUserInterfaceCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::InvokeHideUserInterfaceCommand()
{
  if (this->HasHideUserInterfaceCommand())
    {
    this->InvokeObjectMethodCommand(this->HideUserInterfaceCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWWizardStep::HasHideUserInterfaceCommand()
{
  return this->HideUserInterfaceCommand && *this->HideUserInterfaceCommand;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetValidationCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->ValidationCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::InvokeValidationCommand()
{
  if (this->HasValidationCommand())
    {
    this->InvokeObjectMethodCommand(this->ValidationCommand);
    }
}

//----------------------------------------------------------------------------
int vtkKWWizardStep::HasValidationCommand()
{
  return this->ValidationCommand && *this->ValidationCommand;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::SetCanGoToSelfCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->CanGoToSelfCommand, object, method);
}

//----------------------------------------------------------------------------
int vtkKWWizardStep::InvokeCanGoToSelfCommand()
{
  if (this->HasCanGoToSelfCommand())
    {
    return atoi(vtkKWTkUtilities::EvaluateSimpleString(
                  this->GetApplication(), this->CanGoToSelfCommand));
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWWizardStep::HasCanGoToSelfCommand()
{
  return this->CanGoToSelfCommand && *this->CanGoToSelfCommand;
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::RemoveCallbackCommandObservers()
{
  this->Superclass::RemoveCallbackCommandObservers();

  if (this->InteractionState)
    {
    this->RemoveCallbackCommandObserver(
      this->InteractionState, vtkKWStateMachineState::EnterEvent);
    }

  if (this->ValidationTransition)
    {
    this->RemoveCallbackCommandObserver(
      this->ValidationTransition, vtkKWStateMachineTransition::EndEvent);
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::ProcessCallbackCommandEvents(vtkObject *caller,
                                                   unsigned long event,
                                                   void *calldata)
{
  if (caller == this->InteractionState)
    {
    switch (event)
      {
      case vtkKWStateMachineState::EnterEvent:
        this->InvokeShowUserInterfaceCommand();
        break;
      }
    }
  else if (caller == this->ValidationTransition)
    {
    switch (event)
      {
      case vtkKWStateMachineTransition::EndEvent:
        this->InvokeValidationCommand();
        break;
      }
    }

  this->Superclass::ProcessCallbackCommandEvents(caller, event, calldata);
}

//----------------------------------------------------------------------------
void vtkKWWizardStep::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << (this->Name ? this->Name : "None") << endl;
  os << indent << "Description: " << (this->Description ? this->Description : "None") << endl;

  os << indent << "InteractionState: ";
  if (this->InteractionState)
    {
    os << endl;
    this->InteractionState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ValidationState: ";
  if (this->ValidationState)
    {
    os << endl;
    this->ValidationState->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ValidationTransition: ";
  if (this->ValidationTransition)
    {
    os << endl;
    this->ValidationTransition->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ValidationFailedTransition: ";
  if (this->ValidationFailedTransition)
    {
    os << endl;
    this->ValidationFailedTransition->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "GoToSelfInput: ";
  if (this->GoToSelfInput)
    {
    os << endl;
    this->GoToSelfInput->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "GoBackToSelfInput: ";
  if (this->GoBackToSelfInput)
    {
    os << endl;
    this->GoBackToSelfInput->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "vtkKWWizardStep::ValidationInput: ";
  if (vtkKWWizardStep::ValidationInput)
    {
    os << endl;
    vtkKWWizardStep::ValidationInput->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "vtkKWWizardStep::ValidationSucceededInput: ";
  if (vtkKWWizardStep::ValidationSucceededInput)
    {
    os << endl;
    vtkKWWizardStep::ValidationSucceededInput->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "vtkKWWizardStep::ValidationFailedInput: ";
  if (vtkKWWizardStep::ValidationFailedInput)
    {
    os << endl;
    vtkKWWizardStep::ValidationFailedInput->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
