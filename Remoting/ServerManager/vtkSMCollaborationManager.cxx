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
#include "vtkPVMultiClientsInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkReservedRemoteObjectIds.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"

#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"

#include <map>
#include <string>
#include <vector>

//****************************************************************************
//                              Internal class
//****************************************************************************
class vtkSMCollaborationManager::vtkInternal
{
public:
  vtkInternal(vtkSMCollaborationManager* m)
  {
    this->Manager = m;
    this->ObserverTag = 0;
    this->Me = 0;
    this->UserToFollow = 0;
    this->DisableFurtherConnections = false;
    this->ConnectID = 0;
    this->Clear();
  }

  ~vtkInternal()
  {
    this->Clear();
    if (this->Manager && this->Manager->GetSession() && this->ObserverTag != 0)
    {
      this->Manager->GetSession()->RemoveObserver(this->ObserverTag);
      this->ObserverTag = 0;
    }
  }

  void Init()
  {
    // Check our current user Id and store it
    this->Me = this->Manager->GetSession()->GetServerInformation()->GetClientId();
    this->ObserverTag =
      this->Manager->GetSession()->AddObserver(vtkPVSessionBase::ProcessingRemoteEnd, this,
        &vtkSMCollaborationManager::vtkInternal::StopProcessingRemoteNotificationCallback);
  }

  const char* GetUserName(int userId) { return this->UserNames[userId].c_str(); }

  bool UpdateMaster(int newMaster)
  {
    if (this->Master != newMaster)
    {
      this->Master = newMaster;

      // If no user to follow yet, then we follow master...
      this->UpdateState((this->UserToFollow == 0) ? newMaster : this->UserToFollow);
      this->Manager->InvokeEvent(
        (unsigned long)vtkSMCollaborationManager::UpdateMasterUser, (void*)&newMaster);
      return true;
    }
    return false;
  }

  bool UpdateUserName(int userId, const char* userName)
  {
    if (userName)
    {
      if (this->UserNames[userId] != userName)
      {
        this->UserNames[userId] = userName;
        this->UpdateState(this->UserToFollow);
        this->Manager->InvokeEvent(
          (unsigned long)vtkSMCollaborationManager::UpdateUserName, (void*)&userId);
        return true;
      }
    }
    else
    {
      this->UserNames.erase(userId);
      this->UpdateState(this->UserToFollow);
    }
    return false;
  }

  bool SetDisableFurtherConnections(bool disable)
  {
    if (disable != this->DisableFurtherConnections)
    {
      this->DisableFurtherConnections = disable;
      this->UpdateState(this->UserToFollow == 0 ? this->Master : this->UserToFollow);
      return true;
    }
    return false;
  }

  bool UpdateConnectID(int connectID)
  {
    if (this->ConnectID != connectID)
    {
      this->ConnectID = connectID;
      this->UpdateState(this->UserToFollow == 0 ? this->Master : this->UserToFollow);
      return true;
    }
    return false;
  }

  void Clear()
  {
    this->UserNames.clear();
    this->Users.clear();
    this->Master = 0;
    this->ConnectID = 0;
    this->DisableFurtherConnections = false;
    this->State.Clear();
    this->PendingCameraUpdate.Clear();
    this->LocalCameraStateCache.clear();
  }

  void UpdateState(int followCamUserId)
  {
    this->UserToFollow = followCamUserId;
    this->State.ClearExtension(ClientsInformation::user);
    size_t size = this->Users.size();
    for (size_t i = 0; i < size; ++i)
    {
      ClientsInformation_ClientInfo* user = this->State.AddExtension(ClientsInformation::user);
      user->set_user(this->Users[i]);
      user->set_name(this->GetUserName(this->Users[i]));
      if (this->Users[i] == this->Master)
      {
        user->set_is_master(true);
      }
      if (this->Users[i] == followCamUserId)
      {
        user->set_follow_cam(true);
      }
      user->set_disable_further_connections(this->DisableFurtherConnections);
      user->set_connect_id(this->ConnectID);
    }
  }

  bool LoadState(const vtkSMMessage* msg)
  {
    int size = msg->ExtensionSize(ClientsInformation::user);
    bool foundChanges = (size != static_cast<int>(this->Users.size()));
    // Update User list first
    this->Users.clear();
    for (int i = 0; i < size; ++i)
    {
      const ClientsInformation_ClientInfo* user = &msg->GetExtension(ClientsInformation::user, i);
      int id = user->user();
      this->Users.push_back(id);
    }

    // Update user name/master info
    int newFollow = 0;
    for (int i = 0; i < size; ++i)
    {
      const ClientsInformation_ClientInfo* user = &msg->GetExtension(ClientsInformation::user, i);
      int id = user->user();
      foundChanges = this->UpdateUserName(id, user->name().c_str()) || foundChanges;
      if (user->is_master())
      {
        foundChanges = this->UpdateMaster(id) || foundChanges;
      }
      foundChanges =
        this->SetDisableFurtherConnections(user->disable_further_connections()) || foundChanges;
      foundChanges = this->UpdateConnectID(user->connect_id()) || foundChanges;

      if (user->follow_cam())
      {
        // Invoke event...
        newFollow = id;
        this->Manager->InvokeEvent(
          (unsigned long)vtkSMCollaborationManager::FollowUserCamera, (void*)&id);
      }
    }
    if (newFollow)
    {
      this->UserToFollow = newFollow;
    }

    return foundChanges || newFollow;
  }

  // Return the camera update message user origin otherwise
  // if not a camera update message we return -1;
  int StoreCameraByUser(const vtkSMMessage* msg)
  {
    if (msg->HasExtension(DefinitionHeader::client_class) &&
      msg->GetExtension(DefinitionHeader::client_class) == "vtkSMCameraProxy")
    {
      int currentUserId = static_cast<int>(msg->client_id());
      this->LocalCameraStateCache[currentUserId].CopyFrom(*msg);
      return currentUserId;
    }
    return -1;
  }

  void UpdateCamera(const vtkSMMessage* msg)
  {
    vtkTypeUInt32 cameraId = msg->global_id();
    vtkSMProxyLocator* locator = this->Manager->GetSession()->GetProxyLocator();
    vtkSMProxy* proxy = locator->LocateProxy(cameraId);

    // As camera do not synch its properties while IsProcessingRemoteNotification
    // there is no point of updating it when we are in that case.
    // So we just push back that request to later...
    if (proxy && !proxy->GetSession()->IsProcessingRemoteNotification())
    {
      // Update Proxy
      proxy->EnableLocalPushOnly();
      proxy->LoadState(msg, locator);
      proxy->UpdateVTKObjects();
      proxy->DisableLocalPushOnly();

      // Fire event so the Qt layer could trigger a render
      this->Manager->InvokeEvent(vtkSMCollaborationManager::CameraChanged);
    }
    else if (proxy->GetSession()->IsProcessingRemoteNotification())
    {
      this->PendingCameraUpdate.CopyFrom(*msg);
    }
  }

  void StopProcessingRemoteNotificationCallback(vtkObject*, unsigned long, void*)
  {
    if (this->PendingCameraUpdate.has_global_id())
    {
      this->UpdateCamera(&this->PendingCameraUpdate);
      this->PendingCameraUpdate.Clear();
    }
  }

  vtkWeakPointer<vtkSMCollaborationManager> Manager;
  std::map<int, std::string> UserNames;
  std::vector<int> Users;
  int Me;
  int UserToFollow;
  int Master;
  int ConnectID;
  vtkSMMessage State;
  vtkSMMessage PendingCameraUpdate;
  std::map<int, vtkSMMessage> LocalCameraStateCache;
  unsigned long ObserverTag;
  bool DisableFurtherConnections;
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
  this->Internal = new vtkInternal(this);
  this->SetGlobalID(vtkSMCollaborationManager::GetReservedGlobalID());
}

//----------------------------------------------------------------------------
vtkSMCollaborationManager::~vtkSMCollaborationManager()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkSMCollaborationManager::LoadState(
  const vtkSMMessage* msg, vtkSMProxyLocator* vtkNotUsed(locator))
{
  // Check if it's a local state or if it's a state that use the
  // CollaborationManager as communication channel accros clients
  if (msg->ExtensionSize(ClientsInformation::user) > 0)
  {
    // For me
    if (this->Internal->LoadState(msg))
    {
      this->InvokeEvent(UpdateUserList);
    }
  }
  else
  {
    // Handle camera synchro
    if (this->Internal->UserToFollow == this->Internal->StoreCameraByUser(msg) &&
      this->Internal->UserToFollow != -1)
    {
      this->Internal->UpdateCamera(msg);
    }

    // For Observers
    vtkSMMessage* msgCopy = new vtkSMMessage();
    msgCopy->CopyFrom(*msg);
    this->InvokeEvent(CollaborationNotification, msgCopy);
  }
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::DisableFurtherConnections(bool disable)
{
  this->Internal->SetDisableFurtherConnections(disable);
  this->UpdateUserInformations();
}

//-----------------------------------------------------------------------------
void vtkSMCollaborationManager::SetConnectID(int connectID)
{
  this->Internal->UpdateConnectID(connectID);
  this->UpdateUserInformations();
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMCollaborationManager::GetGlobalID()
{
  if (!this->HasGlobalID())
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
int vtkSMCollaborationManager::GetFollowedUser()
{
  return this->Internal->UserToFollow;
}
//----------------------------------------------------------------------------
void vtkSMCollaborationManager::FollowUser(int clientId)
{
  if (this->Internal->UserToFollow == clientId)
  {
    return;
  }

  if (this->IsMaster())
  {
    this->Internal->UpdateState(clientId);
    this->UpdateUserInformations();
  }
  else // Follow someone else on my own
  {
    this->Internal->UserToFollow = clientId;
  }

  // Update the camera
  if (clientId != -1 &&
    this->Internal->LocalCameraStateCache.find(clientId) !=
      this->Internal->LocalCameraStateCache.end())
  {
    this->Internal->UpdateCamera(&this->Internal->LocalCameraStateCache[clientId]);
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
bool vtkSMCollaborationManager::GetDisableFurtherConnections()
{
  return this->Internal->DisableFurtherConnections;
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
  if (this->Internal->UpdateUserName(userId, userName))
  {
    this->UpdateUserInformations();
  }
}

//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetNumberOfConnectedClients()
{
  return static_cast<int>(this->Internal->Users.size());
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
  if (this->GetNumberOfConnectedClients() == 0)
  {
    vtkSMMessage msg;
    msg.CopyFrom(*this->GetFullState());
    this->PullState(&msg);
    this->LoadState(&msg, nullptr);
  }
}

//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetServerConnectID()
{
  return this->Internal->ConnectID;
}

//----------------------------------------------------------------------------
int vtkSMCollaborationManager::GetConnectID()
{
  vtkSMSessionClient* session = vtkSMSessionClient::SafeDownCast(this->GetSession());
  return session ? session->GetConnectID() : -1;
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::SetSession(vtkSMSession* session)
{
  this->Superclass::SetSession(session);
  this->Internal->Init();
}
