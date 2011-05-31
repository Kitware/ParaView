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
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkPVSessionCore.h"
#include "vtkWeakPointer.h"

#include <vtksys/ios/sstream>
#include <assert.h>

//----------------------------------------------------------------------------
vtkPVSessionBase::vtkPVSessionBase()
{
  this->SessionCore = vtkPVSessionCore::New();

  // initialize local process information.
  this->LocalServerInformation = vtkPVServerInformation::New();
  this->LocalServerInformation->CopyFromObject(NULL);

  // This ensure that whenever a message is received on  the parallel
  // controller, this session is marked active. This is essential for
  // satellites when running in parallel.
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  if(!controller)
    {
    vtkWarningMacro("No vtkMultiProcessController for Session. The session won't work correctly.");
    return;
    }

  controller->AddObserver(vtkCommand::StartEvent,
    this, &vtkPVSessionBase::Activate);
  controller->AddObserver(vtkCommand::EndEvent,
    this, &vtkPVSessionBase::DeActivate);
}

//----------------------------------------------------------------------------
vtkPVSessionBase::~vtkPVSessionBase()
{
  if(vtkProcessModule::GetProcessModule())
    {
    vtkProcessModule::GetProcessModule()->InvokeEvent(vtkCommand::ExitEvent);
    }

  this->SessionCore->Delete();
  this->SessionCore = NULL;

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
    return (process_id == 0)?
      vtkPVSession::CLIENT_AND_SERVERS :
      vtkPVSession::SERVERS;

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
  vtkTypeUInt32 location, const vtkClientServerStream& stream,
  bool ignore_errors/*=false*/)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->ExecuteStream(location, stream, ignore_errors);

  this->DeActivate();
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVSessionBase::GetLastResult(
  vtkTypeUInt32 vtkNotUsed(location))
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  return this->SessionCore->GetLastResult();
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::DeleteSIObject(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->DeleteSIObject(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
vtkSIObject* vtkPVSessionBase::GetSIObject(vtkTypeUInt32 globalid)
{
  return this->SessionCore? this->SessionCore->GetSIObject(globalid) : NULL;
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::PrepareProgressInternal()
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for vtkPVSessionCore helper.
            << "GetActiveProgressHandler"
            << vtkClientServerStream::End;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << substream
         << "PrepareProgress"
         << vtkClientServerStream::End;
  this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  //this->Superclass::PrepareProgressInternal();
  //FIXME_COLLABORATION - I don't like code that skips superclass implentations.
  //Rethink this.
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::CleanupPendingProgressInternal()
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for vtkPVSessionCore helper.
            << "GetActiveProgressHandler"
            << vtkClientServerStream::End;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << substream
         << "CleanupPendingProgress"
         << vtkClientServerStream::End;
  this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  //this->Superclass::CleanupPendingProgressInternal();
  //FIXME_COLLABORATION
}

//----------------------------------------------------------------------------
bool vtkPVSessionBase::GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  return this->SessionCore->GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
vtkObject* vtkPVSessionBase::GetRemoteObject(vtkTypeUInt32 globalid)
{
  return this->SessionCore->GetRemoteObject(globalid);
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::RegisterRemoteObject(vtkTypeUInt32 gid, vtkObject* obj)
{
  this->SessionCore->RegisterRemoteObject(gid, obj);
}

//----------------------------------------------------------------------------
void vtkPVSessionBase::UnRegisterRemoteObject(vtkTypeUInt32 gid, vtkTypeUInt32 location)
{
  this->SessionCore->UnRegisterRemoteObject(gid);

  // Also delete remote resources as well
  vtkSMMessage deleteMsg;
  deleteMsg.set_global_id(gid);
  deleteMsg.set_location(location);
  this->DeleteSIObject(&deleteMsg);
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
