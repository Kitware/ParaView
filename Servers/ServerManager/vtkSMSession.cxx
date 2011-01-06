/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSession.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkSMMessage.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionCore.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkWeakPointer.h"

#include <vtksys/ios/sstream>
#include <assert.h>

vtkStandardNewMacro(vtkSMSession);
vtkCxxSetObjectMacro(vtkSMSession, UndoStackBuilder, vtkSMUndoStackBuilder);
//----------------------------------------------------------------------------
vtkSMSession::vtkSMSession()
{
  this->Core = vtkSMSessionCore::New();
  this->ProxyManager = vtkSMProxyManager::New();
  this->ProxyManager->SetSession(this);
  this->ProxyManager->SetProxyDefinitionManager(
      this->Core->GetProxyDefinitionManager());
  this->PluginManager = vtkSMPluginManager::New();
  this->PluginManager->SetSession(this);

  this->UndoStackBuilder = NULL;

  // The 10 first ID are reserved
  //  - 1: vtkSMProxyManager
  this->LastGUID = 10;

  // Reserved Id management
  this->RegisterRemoteObject(this->ProxyManager);

  this->LocalServerInformation = vtkPVServerInformation::New();
  this->LocalServerInformation->CopyFromObject(NULL);
}

//----------------------------------------------------------------------------
vtkSMSession::~vtkSMSession()
{
  this->PluginManager->Delete();
  this->PluginManager = NULL;
  this->ProxyManager->Delete();
  this->ProxyManager = NULL;
  this->SetUndoStackBuilder(0);
  this->Core->Delete();
  this->Core = NULL;

  this->LocalServerInformation->Delete();
  this->LocalServerInformation = 0;
}

//----------------------------------------------------------------------------
vtkSMSession::ServerFlags vtkSMSession::GetProcessRoles()
{
  if (vtkProcessModule::GetProcessModule() &&
    vtkProcessModule::GetProcessModule()->GetPartitionId() > 0)
    {
    return SERVERS;
    }
  return this->Superclass::GetProcessRoles();
}

//----------------------------------------------------------------------------
vtkPVServerInformation* vtkSMSession::GetServerInformation()
{
  return this->LocalServerInformation;
}

//----------------------------------------------------------------------------
void vtkSMSession::PushState(vtkSMMessage* msg)
{
  this->Activate();

  // Manage Undo/Redo if possible
  if(this->UndoStackBuilder)
    {
    vtkTypeUInt32 globalId = msg->global_id();
    vtkSMRemoteObject *remoteObj = this->GetRemoteObject(globalId);

    if(remoteObj)
      {
      vtkSMMessage fullState;
      fullState.CopyFrom(*remoteObj->GetFullState());

      // Need to provide id/location as the full state may not have them yet
      fullState.set_global_id(globalId);
      fullState.set_location(msg->location());

      this->UndoStackBuilder->OnNewState(this, globalId, &fullState);
      }
    else
      {
      cout << "Push a state that is not related to a proxy." << endl;
      }
    }

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->PushState(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkSMSession::PullState(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->PullState(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkSMSession::Invoke(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->Invoke(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkSMSession::DeletePMObject(vtkSMMessage* msg)
{
  this->Activate();

  // Manage Undo/Redo if possible
  if(this->UndoStackBuilder)
    {
    this->UndoStackBuilder->OnNewState(this, msg->global_id(), NULL);
    }

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->DeletePMObject(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
vtkPMObject* vtkSMSession::GetPMObject(vtkTypeUInt32 globalid)
{
  return this->Core? this->Core->GetPMObject(globalid) : NULL;
}

//----------------------------------------------------------------------------
void vtkSMSession::Initialize()
{
  // Make sure that the client as the server XML definition
  this->GetProxyManager()->LoadXMLDefinitionFromServer();
  this->PluginManager->SetSession(this);
  this->PluginManager->Initialize();
}

//----------------------------------------------------------------------------
bool vtkSMSession::GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  return this->Core->GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
int vtkSMSession::GetNumberOfProcesses(vtkTypeUInt32 servers)
{
  (void)servers;
  return this->Core->GetNumberOfProcesses();
}

//----------------------------------------------------------------------------
void vtkSMSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Core->PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkSMRemoteObject* vtkSMSession::GetRemoteObject(vtkTypeUInt32 globalid)
{
  return this->Core->GetRemoteObject(globalid);
}
//----------------------------------------------------------------------------
void vtkSMSession::RegisterRemoteObject(vtkSMRemoteObject* obj)
{
  assert(obj != NULL);

  this->Core->RegisterRemoteObject(obj);
}

//----------------------------------------------------------------------------
void vtkSMSession::UnRegisterRemoteObject(vtkSMRemoteObject* obj)
{
  assert(obj != NULL);

  // Make sure to delete PMObject as well
  vtkSMMessage deleteMsg;
  deleteMsg.set_location(obj->GetLocation());
  deleteMsg.set_global_id(obj->GetGlobalID());
  this->Core->UnRegisterRemoteObject(obj);
  this->DeletePMObject(&deleteMsg);
}

//----------------------------------------------------------------------------
void vtkSMSession::GetAllRemoteObjects(vtkCollection* collection)
{
  this->Core->GetAllRemoteObjects(collection);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToSelf()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  session->Initialize();
  vtkIdType sid = pm->RegisterSession(session);
  session->Delete();
  return sid;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemote(const char* hostname, int port)
{
  vtksys_ios::ostringstream sname;
  sname << "cs://" << hostname << ":" << port;
  vtkSMSessionClient* session = vtkSMSessionClient::New();
  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str()))
    {
    session->Initialize();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkIdType sid = pm->RegisterSession(session);
    }
  session->Delete();
  return sid;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemote(const char* dshost, int dsport,
  const char* rshost, int rsport)
{
  vtksys_ios::ostringstream sname;
  sname << "cdsrs://" << dshost << ":" << dsport << "/"
    << rshost << ":" << rsport;
  vtkSMSessionClient* session = vtkSMSessionClient::New();
  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str()))
    {
    session->Initialize();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkIdType sid = pm->RegisterSession(session);
    }
  session->Delete();
  return sid;
}
