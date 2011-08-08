/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCollaborationManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCollaborationManager.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
#include "vtkReservedRemoteObjectIds.h"
#include "vtkPVMultiClientsInformation.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>

//****************************************************************************
//                              Internal class
//****************************************************************************
class vtkSMCollaborationManager::vtkInternal
{
public:
  vtkInternal()
    {
    this->Me = 0;
    this->Clear();
    }

  ~vtkInternal()
    {
    this->Clear();
    }

  const char* GetUserName(int userId)
    {
    vtkstd::string &name = this->UserNames[userId];
    if(name.empty())
      {

      }
    return name.c_str();
    }

  bool UpdateMaster(int newMaster)
    {
    if(this->Master != newMaster)
      {
      this->Master = newMaster;
      this->UpdateState(-1);
      this->Manager->InvokeEvent(
          (unsigned long)vtkSMCollaborationManager::UpdateMasterUser,
          (void*) &newMaster);
      return true;
      }
    return false;
    }

  bool UpdateUserName(int userId, const char* userName)
    {
    if(userName)
      {
      if(this->UserNames[userId] != userName)
        {
        this->UserNames[userId] = userName;
        this->UpdateState(-1);
        this->Manager->InvokeEvent(
            (unsigned long)vtkSMCollaborationManager::UpdateUserName,
            (void*) &userId);
        return true;
        }
      }
    else
      {
      this->UserNames.erase(userId);
      this->UpdateState(-1);
      }
    return false;
    }

  void Clear()
    {
    this->UserNames.clear();
    this->Users.clear();
    this->Master = 0;
    this->State.Clear();
    }

  void UpdateState(int followCamUserId)
    {
    this->State.ClearExtension(ClientsInformation::user);
    int size = this->Users.size();
    for(int i=0; i < size; ++i)
      {
      ClientsInformation_ClientInfo* user =
          this->State.AddExtension(ClientsInformation::user);
      user->set_user(this->Users[i]);
      user->set_name(this->GetUserName(this->Users[i]));
      if(this->Users[i] == this->Master)
        {
        user->set_is_master(true);
        }
      if(this->Users[i] == followCamUserId)
        {
        user->set_follow_cam(true);
        }
      }
    }

  bool LoadState(const vtkSMMessage* msg)
    {
    int size = msg->ExtensionSize(ClientsInformation::user);
    bool foundChanges = (size != static_cast<int>(this->Users.size()));
    // Update User list first
    this->Users.clear();
    for(int i=0; i < size; ++i)
      {
      const ClientsInformation_ClientInfo* user =
          &msg->GetExtension(ClientsInformation::user, i);
      int id = user->user();
      this->Users.push_back(id);
      }

    // Update user name/master info
    for(int i=0; i < size; ++i)
      {
      const ClientsInformation_ClientInfo* user =
          &msg->GetExtension(ClientsInformation::user, i);
      int id = user->user();
      foundChanges = this->UpdateUserName(id, user->name().c_str()) || foundChanges;
      if(user->is_master())
        {
        foundChanges = this->UpdateMaster(id) || foundChanges;
        }

      if(user->follow_cam())
        {
        // Invoke event...
        this->Manager->InvokeEvent(
            (unsigned long)vtkSMCollaborationManager::FollowUserCamera,
            (void*) &id);
        }
      }

    return foundChanges;
    }

  vtkWeakPointer<vtkSMCollaborationManager> Manager;
  vtkstd::map<int, vtkstd::string>          UserNames;
  vtkstd::vector<int>                       Users;
  int                                       Me;
  int                                       Master;
  vtkSMMessage                              State;
};
//****************************************************************************
vtkStandardNewMacro(vtkSMCollaborationManager);
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMCollaborationManager::GetReservedGlobalID()
{
  return vtkReservedRemoteObjectIds::RESERVED_COLLABORATION_COMMUNICATOR_ID;
}
//----------------------------------------------------------------------------
vtkSMCollaborationManager::vtkSMCollaborationManager()
{
  this->SetLocation(vtkPVSession::DATA_SERVER_ROOT);
  this->Internal = new vtkInternal();
  this->Internal->Manager = this;
  this->SetGlobalID(vtkSMCollaborationManager::GetReservedGlobalID());
}

//----------------------------------------------------------------------------
vtkSMCollaborationManager::~vtkSMCollaborationManager()
{
  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkSMCollaborationManager::LoadState( const vtkSMMessage* msg,
                                           vtkSMProxyLocator* vtkNotUsed(locator))
{
  // Check if it's a local state or if it's a state that use the
  // CollaborationManager as communication channel accros clients
  if(msg->ExtensionSize(ClientsInformation::user) > 0)
    {
    // For me
    if(this->Internal->LoadState(msg))
      {
      this->InvokeEvent(UpdateUserList);
      }
    }
  else
    {
    // For Observers
    vtkSMMessage* msgCopy = new vtkSMMessage();
    msgCopy->CopyFrom(*msg);
    this->InvokeEvent(CollaborationNotification, msgCopy);
    }
}
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMCollaborationManager::GetGlobalID()
{
  if(!this->HasGlobalID())
    {
    this->SetGlobalID(vtkSMCollaborationManager::GetReservedGlobalID());
    }
  return this->Superclass::GetGlobalID();
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::SendToOtherClients(vtkSMMessage* msg)
{
  this->PushState(msg);
}
//----------------------------------------------------------------------------
void vtkSMCollaborationManager::PromoteToMaster(int clientId)
{
  this->Internal->UpdateMaster(clientId);
  this->UpdateUserInformations();
}
//----------------------------------------------------------------------------
void vtkSMCollaborationManager::FollowUser(int clientId)
{
  if(this->IsMaster())
    {
    this->Internal->UpdateState(clientId);
    this->UpdateUserInformations();
    }
}

//----------------------------------------------------------------------------
bool vtkSMCollaborationManager::IsMaster()
{
  return (this->Internal->Me == this->Internal->Master);
}

//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetMasterId()
{
  return this->Internal->Master;
}
//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetUserId()
{
  return this->Internal->Me;
}
//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetUserId(int index)
{
  return this->Internal->Users[index];
}
//----------------------------------------------------------------------------
const char* vtkSMCollaborationManager::GetUserLabel(int userID)
{
  return this->Internal->GetUserName(userID);
}
//----------------------------------------------------------------------------
void vtkSMCollaborationManager::SetUserLabel(const char* userName)
{
  this->SetUserLabel(this->Internal->Me, userName);
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::SetUserLabel(int userId, const char* userName)
{
  if(this->Internal->UpdateUserName(userId, userName))
    {
    this->UpdateUserInformations();
    }
}

//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetNumberOfConnectedClients()
{
  return this->Internal->Users.size();
}

//----------------------------------------------------------------------------
const vtkSMMessage* vtkSMCollaborationManager::GetFullState()
{
  this->Internal->State.set_location(vtkPVSession::DATA_SERVER_ROOT);
  this->Internal->State.set_global_id(vtkSMCollaborationManager::GetReservedGlobalID());
  this->Internal->State.SetExtension(DefinitionHeader::client_class, "vtkSMCollaborationManager");
  this->Internal->State.SetExtension(DefinitionHeader::server_class, "vtkSICollaborationManager");

 return &this->Internal->State;
}
//----------------------------------------------------------------------------
void vtkSMCollaborationManager::UpdateUserInformations()
{
  // Make sure to add declaration to state message
  this->GetFullState();
  this->PushState(&this->Internal->State);

  // If we are the only client fetch the data of ourself
  if(this->GetNumberOfConnectedClients() == 0)
    {
    vtkSMMessage msg;
    msg.CopyFrom(*this->GetFullState());
    this->PullState(&msg);
    this->LoadState(&msg, NULL);
    }
}
//----------------------------------------------------------------------------
void vtkSMCollaborationManager::SetSession(vtkSMSession* session)
{
  this->Superclass::SetSession(session);

  // Check our current user Id and store it
  this->Internal->Me = this->Session->GetServerInformation()->GetClientId();

}
