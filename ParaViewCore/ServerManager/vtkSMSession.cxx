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
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMSessionClient.h"
#include "vtkSMStateLocator.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUndoStack.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkWeakPointer.h"
#include "vtkPVRenderView.h"
#include "vtkPVServerInformation.h"
#include "vtkReservedRemoteObjectIds.h"

#include <vtksys/ios/sstream>
#include <assert.h>

//----------------------------------------------------------------------------
// STATICS
vtkSmartPointer<vtkProcessModuleAutoMPI> vtkSMSession::AutoMPI =
    vtkSmartPointer<vtkProcessModuleAutoMPI>::New();
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSession);
vtkCxxSetObjectMacro(vtkSMSession, UndoStackBuilder, vtkSMUndoStackBuilder);
//----------------------------------------------------------------------------
vtkSMSession::vtkSMSession(bool initialize_during_constructor/*=true*/)
{
  this->StateLocator = vtkSMStateLocator::New();
  this->StateManagement = true; // Allow to store state in local cache for Uno/Redo
  this->PluginManager = vtkSMPluginManager::New();
  this->PluginManager->SetSession(this);
  this->UndoStackBuilder = NULL;
  this->IsAutoMPI = false;

  // Start after the reserved one
  this->LastGUID = vtkReservedRemoteObjectIds::RESERVED_MAX_IDS;

  if (initialize_during_constructor)
    {
    this->Initialize();
    }
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
  this->StateLocator->Delete();
}

//----------------------------------------------------------------------------
vtkSMSession::ServerFlags vtkSMSession::GetProcessRoles()
{
  if (vtkProcessModule::GetProcessModule() &&
    vtkProcessModule::GetProcessModule()->GetPartitionId() > 0 &&
    !vtkProcessModule::GetProcessModule()->GetSymmetricMPIMode())
    {
    return vtkPVSession::SERVERS;
    }

  return vtkPVSession::CLIENT_AND_SERVERS;
}

//----------------------------------------------------------------------------
void vtkSMSession::PushState(vtkSMMessage* msg)
{
  // Manage Undo/Redo if possible
  this->UpdateStateHistory(msg);

  this->Superclass::PushState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::UpdateStateHistory(vtkSMMessage* msg)
{
  if( this->StateManagement &&
      ((this->GetProcessRoles() & vtkPVSession::CLIENT) != 0) )
    {
    vtkTypeUInt32 globalId = msg->global_id();
    vtkSMRemoteObject *remoteObj =
      vtkSMRemoteObject::SafeDownCast(this->GetRemoteObject(globalId));

    //cout << "UpdateStateHistory: " << globalId << endl;

    if(remoteObj && !remoteObj->IsPrototype())
      {
      vtkSMMessage newState;
      newState.CopyFrom(*remoteObj->GetFullState());

      // Need to provide id/location as the full state may not have them yet
      newState.set_global_id(globalId);
      newState.set_location(msg->location());

      // Store state in cache
      vtkSMMessage oldState;
      bool createAction = !this->StateLocator->FindState(globalId, &oldState);

      // This is a filtering Hack, I don't like it. :-(
      if(newState.GetExtension(ProxyState::xml_name) != "Camera")
        {
        this->StateLocator->RegisterState(&newState);
        }

      // Propagate to undo stack builder if possible
      if(this->UndoStackBuilder)
        {
        if(createAction)
          {
          // Do we want to manage object creation ?
          this->UndoStackBuilder->GetUndoStack()->InvokeEvent(vtkSMUndoStack::ObjectCreationEvent, &newState);
          }
        else
          {
          // Update
          if(oldState.SerializeAsString() != newState.SerializeAsString())
            {
            this->UndoStackBuilder->OnStateChange( this, globalId,
                                                   &oldState, &newState);
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMSession::Initialize()
{
  // initialization should never happens on the satellites.
  if (this->GetProcessRoles() & vtkPVSession::CLIENT)
    {
    // Make sure that the client as the server XML definition
    vtkSMObject::GetProxyManager()->SetSession(this);

    this->PluginManager->SetSession(this);
    this->PluginManager->Initialize();
    }
}

//----------------------------------------------------------------------------
int vtkSMSession::GetNumberOfProcesses(vtkTypeUInt32 servers)
{
  (void)servers;
  return vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions();
}

//----------------------------------------------------------------------------
void vtkSMSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToSelf()
{
  vtkPVRenderView::AllowRemoteRendering(true);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType sid = 0;

  if(vtkSMSession::AutoMPI->IsPossible())
    {
    int port = vtkSMSession::AutoMPI->ConnectToRemoteBuiltInSelf();
    // Disable Remote-rendering
    sid = vtkSMSession::ConnectToRemote("localhost", port, false);
    vtkSMSession::SafeDownCast(pm->GetSession(sid))->IsAutoMPI = true;
    }
  else
    {
    vtkSMSession* session = vtkSMSession::New();
    sid = pm->RegisterSession(session);
    session->Delete();
    }

  return sid;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemote(const char* hostname, int port)
{
  // By default we allow remote rendering.
  // Only Auto-MPI has the right to disable it
  return vtkSMSession::ConnectToRemote(hostname, port, true);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemote(const char* hostname, int port,
                                        bool allowRemoteRendering)
{
  vtkPVRenderView::AllowRemoteRendering(allowRemoteRendering);
  vtksys_ios::ostringstream sname;
  sname << "cs://" << hostname << ":" << port;
  vtkSMSessionClient* session = vtkSMSessionClient::New();
  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str()))
    {
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
  vtkPVRenderView::AllowRemoteRendering(true);
  vtksys_ios::ostringstream sname;
  sname << "cdsrs://" << dshost << ":" << dsport << "/"
    << rshost << ":" << rsport;
  vtkSMSessionClient* session = vtkSMSessionClient::New();
  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str()))
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    sid = pm->RegisterSession(session);
    }
  session->Delete();
  return sid;
}

//----------------------------------------------------------------------------
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
  vtkPVRenderView::AllowRemoteRendering(true);
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
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    sid = pm->RegisterSession(session);
    }
  session->RemoveObserver(id);
  session->Delete();
  return sid;
}
//----------------------------------------------------------------------------
unsigned int vtkSMSession::GetRenderClientMode()
{
  if (this->GetIsAutoMPI())
    {
    return vtkSMSession::RENDERING_SPLIT;
    }
  if (this->GetController(vtkPVSession::DATA_SERVER_ROOT) !=
      this->GetController(vtkPVSession::RENDER_SERVER_ROOT))
    {
    // when the two controller are different, we have a separate render-server
    // and data-server session.
    return vtkSMSession::RENDERING_SPLIT;
    }

  vtkPVServerInformation* server_info = this->GetServerInformation();
  if (server_info && server_info->GetNumberOfMachines() > 0)
    {
    return vtkSMSession::RENDERING_SPLIT;
    }

  return vtkSMSession::RENDERING_UNIFIED;
}
