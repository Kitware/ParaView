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
#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"
#include "vtkPVCatalystSessionCore.h"
#include "vtkPVServerInformation.h"
#include "vtkPVSessionCore.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkReservedRemoteObjectIds.h"
#include "vtkSMDeserializerProtobuf.h"
#include "vtkSMMessage.h"
#include "vtkSMPluginManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStateLocator.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <sstream>
#include <vtkNew.h>

//----------------------------------------------------------------------------
// STATICS
vtkSmartPointer<vtkProcessModuleAutoMPI> vtkSMSession::AutoMPI =
  vtkSmartPointer<vtkProcessModuleAutoMPI>::New();
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSession);
//----------------------------------------------------------------------------
vtkSMSession* vtkSMSession::New(vtkPVSessionBase* otherSession)
{
  return vtkSMSession::New(otherSession->GetSessionCore());
}
//----------------------------------------------------------------------------
vtkSMSession* vtkSMSession::New(vtkPVSessionCore* otherSessionCore)
{
  vtkSMSession* session = new vtkSMSession(true, otherSessionCore);
  session->InitializeObjectBase();
  return session;
}

//----------------------------------------------------------------------------
vtkSMSession::vtkSMSession(
  bool initialize_during_constructor /*=true*/, vtkPVSessionCore* preExistingSessionCore /*=NULL*/)
  : vtkPVSessionBase(preExistingSessionCore ? preExistingSessionCore : vtkPVSessionCore::New())
{
  if (!preExistingSessionCore)
  {
    this->SessionCore->UnRegister(NULL);
  }

  this->SessionProxyManager = NULL;
  this->StateLocator = vtkSMStateLocator::New();
  this->IsAutoMPI = false;

  // Create and setup deserializer for the local ProxyLocator
  vtkNew<vtkSMDeserializerProtobuf> deserializer;
  deserializer->SetStateLocator(this->StateLocator);

  // Create and setup proxy locator
  this->ProxyLocator = vtkSMProxyLocator::New();
  this->ProxyLocator->SetDeserializer(deserializer.GetPointer());
  this->ProxyLocator->UseSessionToLocateProxy(true);

  // don't set the session on ProxyLocator right now since the
  // SessionProxyManager is not setup until Initialize().
  // we will initialize it in this->Initialize().

  if (initialize_during_constructor)
  {
    this->Initialize();
  }
}

//----------------------------------------------------------------------------
vtkSMSession::~vtkSMSession()
{
  if (vtkSMProxyManager::IsInitialized())
  {
    vtkSMProxyManager::GetProxyManager()->GetPluginManager()->UnRegisterSession(this);
  }

  this->StateLocator->Delete();
  this->ProxyLocator->Delete();
  if (this->SessionProxyManager)
  {
    this->SessionProxyManager->Delete();
    this->SessionProxyManager = NULL;
  }
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
  // check is global-undo-stack builder is set.
  vtkSMUndoStackBuilder* usb = vtkSMProxyManager::GetProxyManager()->GetUndoStackBuilder();

  if (usb == NULL || (this->GetProcessRoles() & vtkPVSession::CLIENT) == 0)
  {
    return;
  }

  vtkTypeUInt32 globalId = msg->global_id();
  vtkSMRemoteObject* remoteObj = vtkSMRemoteObject::SafeDownCast(this->GetRemoteObject(globalId));

  if (remoteObj && !remoteObj->IsPrototype() && remoteObj->GetFullState())
  {
    vtkSMMessage newState;
    newState.CopyFrom(*remoteObj->GetFullState());

    // Need to provide id/location as the full state may not have them yet
    newState.set_global_id(globalId);
    newState.set_location(msg->location());

    // Store state in cache
    vtkSMMessage oldState;
    bool createAction = !this->StateLocator->FindState(globalId, &oldState,
      /* We want only a local lookup => false */ false);

    // This is a filtering Hack, I don't like it. :-(
    if (newState.GetExtension(ProxyState::xml_name) != "Camera")
    {
      this->StateLocator->RegisterState(&newState);
    }

    // Propagate to undo stack builder if possible
    if (createAction)
    {
      usb->OnCreateObject(this, &newState);
    }
    else if (oldState.SerializeAsString() != newState.SerializeAsString())
    {
      // Update
      usb->OnStateChange(this, globalId, &oldState, &newState);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMSession::Initialize()
{
  assert(this->SessionProxyManager == NULL);

  // Remember, although vtkSMSession is always only created on the client side,
  // in batch mode, vtkSMSession is created on all nodes.

  // All these initializations need to be done on all nodes in symmetric-batch
  // mode. In non-symmetric-batch mode. Which means we are a CLIENT,
  // so if we are not then we stop the initialisation here !
  if (!(this->GetProcessRoles() & vtkPVSession::CLIENT))
  {
    return;
  }

  // Initialize the proxy manager.
  // this updates proxy definitions if we are connected to a remote server.
  this->SessionProxyManager = vtkSMSessionProxyManager::New(this);
  this->ProxyLocator->SetSession(this);

  // Initialize the plugin manager.
  vtkSMProxyManager::GetProxyManager()->GetPluginManager()->RegisterSession(this);
}

//----------------------------------------------------------------------------
int vtkSMSession::GetNumberOfProcesses(vtkTypeUInt32 servers)
{
  (void)servers;
  return vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions();
}

//----------------------------------------------------------------------------
bool vtkSMSession::IsMPIInitialized(vtkTypeUInt32)
{
  return vtkProcessModule::GetProcessModule()->IsMPIInitialized();
}

//----------------------------------------------------------------------------
void vtkSMSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMSession::Disconnect(vtkSMSession* session)
{
  if (!session)
  {
    return;
  }

  if (session->IsMultiClients())
  {
    // Ensure that the session no longer sends state updates to the server-side.
    // This is critical to ensure we don't mess up the collaboration state.
    session->PreDisconnection();
  }

  // Unregister all proxies.
  session->GetSessionProxyManager()->UnRegisterProxies();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->UnRegisterSession(session);
  // Although I'd like to have this check, when Disconnect() is called from
  // Python, we have 1 reference that gets cleared with the Python call returns.
  // if (session != NULL)
  //  {
  //  vtkGenericWarningMacro(
  //    "vtkSMSession wasn't destroyed after UnRegisterSession. "
  //    "This indicates a development problem. Please notify the "
  //    "developers.");
  //  }
}

//----------------------------------------------------------------------------
void vtkSMSession::Disconnect(vtkIdType sid)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkWeakPointer<vtkSMSession> session = vtkSMSession::SafeDownCast(pm->GetSession(sid));
  if (!session)
  {
    vtkGenericWarningMacro("Failed to locate session " << sid << ". Cannot disconnect.");
    return;
  }

  vtkSMSession::Disconnect(session);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToSelf(int timeout)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType sid = 0;

  if (vtkSMSession::AutoMPI->IsPossible())
  {
    int port = vtkSMSession::AutoMPI->ConnectToRemoteBuiltInSelf();
    if (port > 0)
    {
      sid = vtkSMSession::ConnectToRemoteInternal("localhost", port, true, timeout);
      if (sid > 0)
      {
        return sid;
      }
    }
    vtkGenericWarningMacro("Failed to automatically launch 'pvserver' for multi-core support. "
                           "Defaulting to local session.");
  }

  vtkSMSession* session = vtkSMSession::New();
  sid = pm->RegisterSession(session);
  session->Delete();
  return sid;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToCatalyst()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType sid = 0;
  vtkNew<vtkPVCatalystSessionCore> sessionCore;
  vtkSMSession* session = vtkSMSession::New(sessionCore.GetPointer());
  sid = pm->RegisterSession(session);
  session->Delete();
  return sid;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemote(const char* hostname, int port, int timeout)
{
  return vtkSMSession::ConnectToRemoteInternal(hostname, port, false, timeout);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemoteInternal(
  const char* hostname, int port, bool is_auto_mpi, int timeout)
{
  std::ostringstream sname;
  sname << "cs://" << hostname << ":" << port;
  vtkSMSessionClient* session = vtkSMSessionClient::New();
  session->IsAutoMPI = is_auto_mpi;
  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str(), timeout))
  {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    sid = pm->RegisterSession(session);
  }
  session->Delete();
  return sid;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMSession::ConnectToRemote(
  const char* dshost, int dsport, const char* rshost, int rsport, int timeout)
{
  std::ostringstream sname;
  sname << "cdsrs://" << dshost << ":" << dsport << "/" << rshost << ":" << rsport;
  vtkSMSessionClient* session = vtkSMSessionClient::New();
  vtkIdType sid = 0;
  if (session->Connect(sname.str().c_str(), timeout))
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
  bool (*Callback)();
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
      if (!continue_waiting && this->Session)
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
vtkIdType vtkSMSession::ReverseConnectToRemote(int dsport, int rsport, bool (*callback)())
{
  vtkTemp temp;
  temp.Callback = callback;

  std::ostringstream sname;
  if (rsport <= -1)
  {
    sname << "csrc://localhost:" << dsport;
  }
  else
  {
    sname << "cdsrsrc://localhost:" << dsport << "/localhost:" << rsport;
  }

  vtkSMSessionClient* session = vtkSMSessionClient::New();
  temp.Session = session;
  unsigned long id = session->AddObserver(vtkCommand::ProgressEvent, &temp, &vtkTemp::OnEvent);

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

//----------------------------------------------------------------------------
void vtkSMSession::ProcessNotification(const vtkSMMessage* message)
{
  vtkTypeUInt32 id = message->global_id();

  // Find the object for whom this message is meant.
  vtkSMRemoteObject* remoteObj = vtkSMRemoteObject::SafeDownCast(this->GetRemoteObject(id));

  // cout << "##########     Server notification    ##########" << endl;
  // cout << id << " = " << remoteObj << "(" << (remoteObj?
  //    remoteObj->GetClassName() : "null") << ")" << endl;
  // state.PrintDebugString();
  // cout << "###################################################" << endl;

  // ProcessingRemoteNotification = true prevent
  // "ignore_synchronization" properties to be loaded...
  // Therefore camera properties won't be shared
  // (I don't understand this comment, but copying it from the original code).
  if (remoteObj)
  {
    bool previousValue = this->StartProcessingRemoteNotification();
    remoteObj->EnableLocalPushOnly();
    remoteObj->LoadState(message, this->GetProxyLocator());

    vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(remoteObj);
    if (proxy)
    {
      proxy->UpdateVTKObjects();
    }

    remoteObj->DisableLocalPushOnly();
    this->StopProcessingRemoteNotification(previousValue);
  }
}
