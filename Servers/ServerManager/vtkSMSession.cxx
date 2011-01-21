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

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
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
#include "vtkSMProxyManager.h"
#include "vtkWeakPointer.h"

#include <vtksys/ios/sstream>
#include <assert.h>

//*****************************************************************************
//                              Internal Class
//*****************************************************************************
class vtkSMSession::vtkInternals
{
public:
  // Return previous message and replace it with the new one.
  // If the previous message does not exist it has no global id
  vtkSMMessage ReplaceState(vtkTypeUInt32 globalId, const vtkSMMessage* state)
    {
    vtkSMMessage result;

    // Look if we have it
    vtkstd::map<vtkTypeUInt32, vtkSMMessage>::iterator iter;
    iter = this->StateMap.find(globalId);

    // We do find a previous state
    if(iter != this->StateMap.end())
      {
      result.CopyFrom(iter->second);
      if(!state)
        {
        // It means the object should be deleted
        this->StateMap.erase(iter);
        }
      }
    if(state)
      {
      this->StateMap[globalId].CopyFrom(*state);
      }
    return result;
    }

  void FillWithLastState( vtkTypeUInt32 globalId, vtkSMMessage* stateToFill )
    {
    stateToFill->CopyFrom(this->StateMap[globalId]);
    }

private:
  vtkstd::map<vtkTypeUInt32, vtkSMMessage> StateMap;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMSession);
vtkCxxSetObjectMacro(vtkSMSession, UndoStackBuilder, vtkSMUndoStackBuilder);
//----------------------------------------------------------------------------
vtkSMSession::vtkSMSession()
{
  this->Internals = new vtkInternals();
  this->StateManagement = true; // Allow to store state in local cache for Uno/Redo
  this->Core = vtkSMSessionCore::New();
  this->PluginManager = vtkSMPluginManager::New();
  this->PluginManager->SetSession(this);

  this->UndoStackBuilder = NULL;

  // The 10 first ID are reserved
  //  - 1: vtkSMProxyManager
  this->LastGUID = 10;

  this->LocalServerInformation = vtkPVServerInformation::New();
  this->LocalServerInformation->CopyFromObject(NULL);

  // This ensure that whenever a message is received on  the parallel
  // controller, this session is marked active. This is essential for
  // satellites when running in parallel.
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  controller->AddObserver(vtkCommand::StartEvent,
    this, &vtkSMSession::Activate);
  controller->AddObserver(vtkCommand::EndEvent,
    this, &vtkSMSession::DeActivate);
}

//----------------------------------------------------------------------------
vtkSMSession::~vtkSMSession()
{
  if (vtkSMObject::GetProxyManager())
    {
    vtkSMObject::GetProxyManager()->SetSession(NULL);
    }

  this->PluginManager->Delete();
  this->PluginManager = NULL;
  this->SetUndoStackBuilder(0);
  this->Core->Delete();
  this->Core = NULL;

  this->LocalServerInformation->Delete();
  this->LocalServerInformation = 0;

  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkSMProxyDefinitionManager* vtkSMSession::GetProxyDefinitionManager()
{
  return this->Core->GetProxyDefinitionManager();
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
  if(this->StateManagement)
    {
    vtkTypeUInt32 globalId = msg->global_id();
    vtkSMRemoteObject *remoteObj = this->GetRemoteObject(globalId);

    if(remoteObj && !remoteObj->IsPrototype())
      {
      vtkSMMessage newState;
      newState.CopyFrom(*remoteObj->GetFullState());

      // Need to provide id/location as the full state may not have them yet
      newState.set_global_id(globalId);
      newState.set_location(msg->location());

      // Store state in cache
      vtkSMMessage oldState = this->Internals->ReplaceState(globalId, &newState);

      // Propagate to undo stack builder if possible
      if(this->UndoStackBuilder)
        {
        if(oldState.has_global_id())
          {
          // Update
          if(oldState.SerializeAsString() != newState.SerializeAsString())
            {
            this->UndoStackBuilder->OnUpdate(this, globalId, &oldState, &newState);
            }
          }
        }
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
  if(this->StateManagement)
    {
    // FIXME Store state in cache
    // vtkSMMessage previousState = this->Internals->ReplaceState(msg->global_id(),NULL);
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
  vtkSMObject::GetProxyManager()->SetSession(this);

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
    sid = pm->RegisterSession(session);
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
    sid = pm->RegisterSession(session);
    }
  session->Delete();
  return sid;
}

namespace
{
  class vtkTemp
    {
  public:
    bool (*Callback) ();
    vtkSMSessionClient* Session;
    vtkTemp()
      {
      this->Callback = NULL;
      this->Session = NULL;
      }
    void OnEvent()
      {
      if (this->Callback != NULL)
        {
        bool continue_waiting = (*this->Callback)();
        if (!continue_waiting && this->Session )
          {
          this->Session->SetAbortConnect(true);
          }
        }
      }
    };
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ReverseConnectToRemote(int port, bool (*callback)())
{
  return vtkSMSession::ReverseConnectToRemote(port, -1, callback);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ReverseConnectToRemote(
  int dsport, int rsport, bool (*callback)())
{
  vtkTemp temp;
  temp.Callback = callback;

  vtksys_ios::ostringstream sname;
  if (rsport <= -1)
    {
    sname << "csrc://localhost:" << dsport;
    }
  else
    {
    sname << "cdsrsrc://localhost:" << dsport << "/localhost:"<< rsport;
    }

  vtkSMSessionClient* session = vtkSMSessionClient::New();
  temp.Session = session;
  unsigned long id = session->AddObserver(vtkCommand::ProgressEvent,
    &temp, &vtkTemp::OnEvent);

  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str()))
    {
    session->Initialize();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    sid = pm->RegisterSession(session);
    }
  session->RemoveObserver(id);
  session->Delete();
  return sid;
}
//----------------------------------------------------------------------------
void vtkSMSession::GetRemoteObjectLastState( vtkTypeUInt32 globalId ,
                                             vtkSMMessage* lastRemoteObjectState)
{
  if(this->StateManagement)
    {
    this->Internals->FillWithLastState(globalId, lastRemoteObjectState);
    }
}
