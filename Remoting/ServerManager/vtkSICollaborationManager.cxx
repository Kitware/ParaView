/*=========================================================================

  Program:   ParaView
  Module:    vtkSICollaborationManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSICollaborationManager.h"

#include "vtkCommand.h"
#include "vtkCompositeMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVMultiClientsInformation.h"
#include "vtkPVSession.h"
#include "vtkPVSessionServer.h"
#include "vtkProcessModule.h"
#include "vtkReservedRemoteObjectIds.h"
#include "vtkSMMessage.h"

#include <assert.h>
#include <map>
#include <sstream>
#include <string>
#include <vtkNew.h>
#include <vtkWeakPointer.h>

//****************************************************************************
//                            Internal class
//****************************************************************************
class vtkSICollaborationManager::vtkInternal : public vtkCommand
{
public:
  static vtkInternal* New(vtkSICollaborationManager* owner) { return new vtkInternal(owner); }

  vtkInternal(vtkSICollaborationManager* owner)
  {
    this->Owner = owner;
    this->DisableBroadcast = false;
    this->ServerState.set_location(vtkPVSession::DATA_SERVER_ROOT);
    this->ServerState.set_global_id(
      vtkReservedRemoteObjectIds::RESERVED_COLLABORATION_COMMUNICATOR_ID);
    this->ServerState.SetExtension(DefinitionHeader::client_class, "vtkSMCollaborationManager");
    this->ServerState.SetExtension(DefinitionHeader::server_class, "vtkSICollaborationManager");

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    this->ServerSession = vtkPVSessionServer::SafeDownCast(pm->GetSession());
    if (this->ServerSession)
    {
      this->MultiProcessController = vtkCompositeMultiProcessController::SafeDownCast(
        this->ServerSession->GetController(vtkPVSession::CLIENT));
      if (this->MultiProcessController)
      {
        // We don't need to remove it later the vtkCommand take care of that when
        // the object get deleted
        this->MultiProcessController->AddObserver(
          vtkCompositeMultiProcessController::CompositeMultiProcessControllerChanged, this);
      }
    }
  }

  bool UpdateFromUsers(vtkSMMessage* msg)
  {
    bool findChanges = false;
    this->DisableBroadcast = true;
    int size = msg->ExtensionSize(ClientsInformation::user);
    for (int i = 0; i < size; ++i)
    {
      const ClientsInformation_ClientInfo* user = &msg->GetExtension(ClientsInformation::user, i);
      int id = user->user();
      findChanges = findChanges || (this->UserNames[id] != user->name());
      this->UserNames[id] = user->name().c_str();
      if (user->is_master() && this->MultiProcessController)
      {
        findChanges = findChanges || (this->MultiProcessController->GetMasterController() != id);
        this->MultiProcessController->SetMasterController(id);

        if (this->ServerSession)
        {
          findChanges = findChanges || (this->ServerSession->GetDisableFurtherConnections() !=
                                         user->disable_further_connections());
          this->ServerSession->SetDisableFurtherConnections(user->disable_further_connections());
          unsigned int serverConnectId = this->ServerSession->GetConnectID();
          findChanges = findChanges || serverConnectId != user->connect_id();
          this->ServerSession->SetConnectID(user->connect_id());
        }
      }
    }
    this->DisableBroadcast = false;
    return findChanges;
  }

  vtkSMMessage* BuildServerStateMessage()
  {
    this->ServerInformations->CopyFromObject(nullptr);
    int master = this->ServerInformations->GetMasterId();

    this->ServerState.ClearExtension(ClientsInformation::user);
    this->ServerState.ExtensionSize(ClientsInformation::user); // force the clear to work
    for (int i = 0; i < this->ServerInformations->GetNumberOfClients(); i++)
    {
      ClientsInformation_ClientInfo* user =
        this->ServerState.AddExtension(ClientsInformation::user);
      int userId = this->ServerInformations->GetClientId(i);
      user->set_user(userId);
      if (this->UserNames[userId].empty())
      {
        std::ostringstream newUserName;
        newUserName << "User " << userId;
        this->UserNames[userId] = newUserName.str().c_str();
      }
      user->set_name(this->UserNames[userId]);
      if (userId == master)
      {
        user->set_is_master(true);
      }
      if (this->ServerSession)
      {
        user->set_disable_further_connections(this->ServerSession->GetDisableFurtherConnections());
        user->set_connect_id(this->ServerSession->GetConnectID());
      }
    }

    return &this->ServerState;
  }

  bool CanBroadcast() { return (this->ServerSession && !this->DisableBroadcast); }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId),
    void* vtkNotUsed(callData)) override
  {
    // A client has (dis)connect, let's notify the other clients
    if (this->Owner && this->CanBroadcast())
    {
      this->Owner->BroadcastToClients(this->BuildServerStateMessage());
    }
  }

  vtkWeakPointer<vtkPVSessionServer> ServerSession;
  vtkNew<vtkPVMultiClientsInformation> ServerInformations;
  vtkSMMessage ServerState;
  std::map<int, std::string> UserNames;
  bool DisableBroadcast;
  vtkWeakPointer<vtkSICollaborationManager> Owner;
  vtkWeakPointer<vtkCompositeMultiProcessController> MultiProcessController;
};

//****************************************************************************
vtkStandardNewMacro(vtkSICollaborationManager);
//----------------------------------------------------------------------------
vtkSICollaborationManager::vtkSICollaborationManager()
{
  this->Internal = vtkInternal::New(this);
}

//----------------------------------------------------------------------------
vtkSICollaborationManager::~vtkSICollaborationManager()
{
  this->Internal->Delete();
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
void vtkSICollaborationManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSICollaborationManager::Push(vtkSMMessage* msg)
{
  if (this->Internal->UpdateFromUsers(msg) && this->Internal->CanBroadcast())
  {
    this->BroadcastToClients(this->Internal->BuildServerStateMessage());
  }
}

//----------------------------------------------------------------------------
void vtkSICollaborationManager::Pull(vtkSMMessage* msg)
{
  msg->CopyFrom(*this->Internal->BuildServerStateMessage());
}
//----------------------------------------------------------------------------
void vtkSICollaborationManager::BroadcastToClients(vtkSMMessage* msg)
{
  this->Internal->ServerSession->NotifyAllClients(msg);
}
