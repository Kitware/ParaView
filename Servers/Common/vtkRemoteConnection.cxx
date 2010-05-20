/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemoteConnection.h"

#include "vtkClientSocket.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"

#include <vtkstd/vector>

class vtkRemoteConnection::vtkInternal : public
                                         vtkstd::vector<vtkRemoteConnection*>
{
};


//-----------------------------------------------------------------------------
vtkRemoteConnection::vtkRemoteConnection()
{
  this->Internal = new vtkInternal();
  this->Controller = vtkSocketController::New();
}

//-----------------------------------------------------------------------------
vtkRemoteConnection::~vtkRemoteConnection()
{
  this->Finalize();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSocketController* vtkRemoteConnection::GetSocketController()
{
  return vtkSocketController::SafeDownCast(this->Controller);
}

//-----------------------------------------------------------------------------
int vtkRemoteConnection::SetSocket(vtkClientSocket* soc)
{
  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
    this->GetSocketController()->GetCommunicator());
  if (!comm)
    {
    vtkErrorMacro("Failed to get the socket communicator!");
    return 0;
    }
  comm->SetSocket(soc);
  soc->AddObserver(vtkCommand::ErrorEvent, this->GetObserver());
  comm->AddObserver(vtkCommand::ErrorEvent, this->GetObserver());
  return comm->Handshake();
}

//-----------------------------------------------------------------------------
int vtkRemoteConnection::ProcessCommunication()
{
  // Just process one RMI message.
  this->Activate(); 
  int ret = this->Controller->ProcessRMIs(0, 1);
  this->Deactivate();

  if (ret != vtkMultiProcessController::RMI_NO_ERROR)
    {
    // Processing error or connection closed.
    return 0;
    }

  return !this->AbortConnection;
}

//-----------------------------------------------------------------------------
void vtkRemoteConnection::Activate()
{
  this->Internal->push_back(
    vtkProcessModule::GetProcessModule()->GetActiveRemoteConnection());
  vtkProcessModule::GetProcessModule()->SetActiveRemoteConnection(this);
}

//-----------------------------------------------------------------------------
void vtkRemoteConnection::Deactivate()
{
  vtkProcessModule::GetProcessModule()->SetActiveRemoteConnection(
    this->Internal->back());
  this->Internal->pop_back();
}

//-----------------------------------------------------------------------------
void vtkRemoteConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
