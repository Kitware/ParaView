/*=========================================================================

  Program:   ParaView
  Module:    vtkCommunicationErrorCatcher.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
