/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionBase.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSessionBase.h"

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVMultiClientsInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVSessionCore.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <sstream>

//----------------------------------------------------------------------------
vtkPVSessionBase::vtkPVSessionBase()
{
  this->InitSessionBase(vtkPVSessionCore::New());
  this->SessionCore->UnRegister(NULL);
}
//----------------------------------------------------------------------------
vtkPVSessionBase::vtkPVSessionBase(vtkPVSessionCore* coreToUse)
{
  this->InitSessionBase(coreToUse);
}
//----------------------------------------------------------------------------
void vtkPVSessionBase::InitSessionBase(vtkPVSessionCore* coreToUse)
{
  this->ProcessingRemoteNotification = false;
  this->SessionCore = coreToUse;
  if (this->SessionCore)
  {
    this->SessionCore->Register(NULL);
  }

  // initialize local process information.
  this->LocalServerInformation = vtkPVServerInformation::New();
  this->LocalServerInformation->CopyFromObject(NULL);

  // This ensure that whenever a message is received on  the parallel
  // controller, this session is marked active. This is essential for
  // satellites when running in parallel.
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  this->ActivateObserverTag = this->DesactivateObserverTag = 0;

  if (!controller)
  {
    vtkWarningMacro("No vtkMultiProcessController for Session. The session won't work correctly.");
    return;
  }

  this->ActivateObserverTag =
    controller->AddObserver(vtkCommand::StartEvent, this, &vtkPVSessionBase::Activate);
  this->DesactivateObserverTag =
    controller->AddObserver(vtkCommand::EndEvent, this, &vtkPVSessionBase::DeActivate);
}

//----------------------------------------------------------------------------
vtkPVSessionBase::~vtkPVSessionBase()
{
  // Make sure we disable Activate/Deactivate observer
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller && this->ActivateObserverTag && this->DesactivateObserverTag)
  {
    controller->RemoveObserver(this->ActivateObserverTag);
    controller->RemoveObserver(this->DesactivateObserverTag);
  }

  if (this->SessionCore)
  {
    this->SessionCore->Delete();
    this->SessionCore = NULL;
  }

  this->LocalServerInformation->Delete();
  this->LocalServerInformation = NULL;
}

//----------------------------------------------------------------------------
vtkSIProxyDefinitionManager* vtkPVSessionBase::GetProxyDefinitionManager()
{
  return this->SessionCore->GetProxyDefinitionManager();
}

//----------------------------------------------------------------------------
vtkPVSessionBase::ServerFlags vtkPVSessionBase::GetProcessRoles()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  assert(pm != NULL);

  int process_id = pm->GetPartitionId();
  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_SERVER:
      return vtkPVSession::SERVERS;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      return vtkPVSession::DATA_SERVER;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      return vtkPVSession::RENDER_SERVER;

    case vtkProcessModule::PROCESS_BATCH:
      return (process_id == 0) ? vtkPVSession::CLIENT_AND_SERVERS : vtkPVSession::SERVERS;

    default:
      break;
  }
  return this->Superclass::GetProcessRoles();
}

//----------------------------------------------------------------------------
vtkPVServerInformation* vtkPVSessionBase::GetServerInformation()
{
  return this->LocalServerInformation;
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::PushState(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->PushState(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::PullState(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->PullState(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::ExecuteStream(
  vtkTypeUInt32 location, const vtkClientServerStream& stream, bool ignore_errors /*=false*/)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->ExecuteStream(location, stream, ignore_errors);

  this->DeActivate();
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVSessionBase::GetLastResult(vtkTypeUInt32 vtkNotUsed(location))
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  return this->SessionCore->GetLastResult();
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::UnRegisterSIObject(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->UnRegisterSIObject(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::RegisterSIObject(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->RegisterSIObject(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
vtkSIObject* vtkPVSessionBase::GetSIObject(vtkTypeUInt32 globalid)
{
  return this->SessionCore ? this->SessionCore->GetSIObject(globalid) : NULL;
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::PrepareProgressInternal()
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for vtkPVSessionCore helper.
            << "GetActiveProgressHandler" << vtkClientServerStream::End;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << substream << "PrepareProgress"
         << vtkClientServerStream::End;
  this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  // this->Superclass::PrepareProgressInternal();
  // FIXME_COLLABORATION - I don't like code that skips superclass implementations.
  // Rethink this.
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::CleanupPendingProgressInternal()
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for vtkPVSessionCore helper.
            << "GetActiveProgressHandler" << vtkClientServerStream::End;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << substream << "CleanupPendingProgress"
         << vtkClientServerStream::End;
  this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  // this->Superclass::CleanupPendingProgressInternal();
  // FIXME_COLLABORATION
}

//----------------------------------------------------------------------------
bool vtkPVSessionBase::GatherInformation(
  vtkTypeUInt32 location, vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  return this->SessionCore->GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
vtkObject* vtkPVSessionBase::GetRemoteObject(vtkTypeUInt32 globalid)
{
  return this->SessionCore->GetRemoteObject(globalid);
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::RegisterRemoteObject(
  vtkTypeUInt32 gid, vtkTypeUInt32 location, vtkObject* obj)
{
  this->SessionCore->RegisterRemoteObject(gid, obj);

  // Also tell the remote resources that it is used
  vtkSMMessage registerMsg;
  registerMsg.set_global_id(gid);
  registerMsg.set_location(location);
  this->RegisterSIObject(&registerMsg);

  this->InvokeEvent(vtkPVSessionBase::RegisterRemoteObjectEvent, &gid);
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::UnRegisterRemoteObject(vtkTypeUInt32 gid, vtkTypeUInt32 location)
{
  this->SessionCore->UnRegisterRemoteObject(gid);

  // Also delete remote resources as well
  vtkSMMessage deleteMsg;
  deleteMsg.set_global_id(gid);
  deleteMsg.set_location(location);
  this->UnRegisterSIObject(&deleteMsg);

  this->InvokeEvent(vtkPVSessionBase::UnRegisterRemoteObjectEvent, &gid);
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::GetAllRemoteObjects(vtkCollection* collection)
{
  this->SessionCore->GetAllRemoteObjects(collection);
}

//----------------------------------------------------------------------------
vtkMPIMToNSocketConnection* vtkPVSessionBase::GetMPIMToNSocketConnection()
{
  return this->SessionCore->GetMPIMToNSocketConnection();
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVSessionBase::GetNextGlobalUniqueIdentifier()
{
  vtkTypeUInt32 id = this->SessionCore->GetNextGlobalUniqueIdentifier();
  return id;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVSessionBase::GetNextChunkGlobalUniqueIdentifier(vtkTypeUInt32 chunkSize)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerID(1) // ID for the vtkSMSessionCore helper.
         << "GetNextGlobalIdChunk" << chunkSize << vtkClientServerStream::End;
  this->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream);

  // Extract the first id of the new chunk
  vtkTypeUInt32 id;
  this->GetLastResult(vtkPVSession::DATA_SERVER_ROOT).GetArgument(0, 0, &id);
  return id;
}
//----------------------------------------------------------------------------
bool vtkPVSessionBase::StartProcessingRemoteNotification()
{
  bool tmp = this->ProcessingRemoteNotification;
  this->ProcessingRemoteNotification = true;
  return tmp;
}
//----------------------------------------------------------------------------
void vtkPVSessionBase::StopProcessingRemoteNotification(bool previousValue)
{
  this->ProcessingRemoteNotification = previousValue;
  if (!previousValue)
  {
    this->InvokeEvent(vtkPVSessionBase::ProcessingRemoteEnd);
  }
}
//----------------------------------------------------------------------------
bool vtkPVSessionBase::IsProcessingRemoteNotification()
{
  return this->ProcessingRemoteNotification;
}
//----------------------------------------------------------------------------
void vtkPVSessionBase::UseSessionCoreOf(vtkPVSessionBase* other)
{
  if (other)
  {
    this->SetSessionCore(other->GetSessionCore());
  }
  else
  {
    vtkErrorMacro("No vtkPVSessionBase provided");
  }
}

//----------------------------------------------------------------------------
vtkPVSessionCore* vtkPVSessionBase::GetSessionCore() const
{
  return this->SessionCore;
}
//----------------------------------------------------------------------------
void vtkPVSessionBase::SetSessionCore(vtkPVSessionCore* other)
{
  if (this->SessionCore)
  {
    this->SessionCore->Delete();
  }
  this->SessionCore = other;
  if (this->SessionCore)
  {
    this->SessionCore->Register(this);
  }
}
