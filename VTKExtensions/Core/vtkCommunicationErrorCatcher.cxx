// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCommunicationErrorCatcher.h"

#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCommunicationErrorCatcher::vtkCommunicationErrorCatcher(vtkMultiProcessController* controller)
{
  this->Controller = controller;
  if (controller)
  {
    this->Communicator = controller->GetCommunicator();
  }
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkCommunicationErrorCatcher::vtkCommunicationErrorCatcher(vtkCommunicator* communicator)
{
  this->Communicator = communicator;
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkCommunicationErrorCatcher::~vtkCommunicationErrorCatcher()
{
  if (this->Communicator && this->CommunicatorObserverId)
  {
    this->Communicator->RemoveObserver(this->CommunicatorObserverId);
  }
  if (this->Controller && this->ControllerObserverId)
  {
    this->Controller->RemoveObserver(this->ControllerObserverId);
  }
}

//----------------------------------------------------------------------------
void vtkCommunicationErrorCatcher::Initialize()
{
  this->ErrorsRaised = false;
  this->ControllerObserverId = 0;
  this->CommunicatorObserverId = 0;

  if (this->Controller)
  {
    this->ControllerObserverId = this->Controller->AddObserver(
      vtkCommand::ErrorEvent, this, &vtkCommunicationErrorCatcher::OnErrorEvent);
  }
  if (this->Communicator)
  {
    this->CommunicatorObserverId = this->Communicator->AddObserver(
      vtkCommand::ErrorEvent, this, &vtkCommunicationErrorCatcher::OnErrorEvent);
  }
}

//----------------------------------------------------------------------------
void vtkCommunicationErrorCatcher::OnErrorEvent(
  vtkObject* caller, unsigned long eventid, void* calldata)
{
  if (caller && calldata && eventid == vtkCommand::ErrorEvent)
  {
    const char* error_message = reinterpret_cast<const char*>(calldata);
    if (error_message && error_message[0])
    {
      this->ErrorMessages += error_message;
    }
    this->ErrorsRaised = true;
  }
}
