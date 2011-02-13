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
#include "vtkSMSessionBase.h"

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMSessionCore.h"
#include "vtkWeakPointer.h"

#include <vtksys/ios/sstream>
#include <assert.h>

//----------------------------------------------------------------------------
vtkSMSessionBase::vtkSMSessionBase()
{
  this->SessionCore = vtkSMSessionCore::New();

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
    this, &vtkSMSessionBase::Activate);
  controller->AddObserver(vtkCommand::EndEvent,
    this, &vtkSMSessionBase::DeActivate);
}

//----------------------------------------------------------------------------
vtkSMSessionBase::~vtkSMSessionBase()
{
  this->SessionCore->Delete();
  this->SessionCore = NULL;

  this->LocalServerInformation->Delete();
  this->LocalServerInformation = NULL;
}

//----------------------------------------------------------------------------
vtkSMProxyDefinitionManager* vtkSMSessionBase::GetProxyDefinitionManager()
{
  return this->SessionCore->GetProxyDefinitionManager();
}

//----------------------------------------------------------------------------
vtkSMSessionBase::ServerFlags vtkSMSessionBase::GetProcessRoles()
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
  case vtkProcessModule::PROCESS_SYMMETRIC_BATCH:
    return (process_id == 0)?
      vtkPVSession::CLIENT_AND_SERVERS :
      vtkPVSession::SERVERS;

  default:
    break;
    }
  return this->Superclass::GetProcessRoles();
}

//----------------------------------------------------------------------------
vtkPVServerInformation* vtkSMSessionBase::GetServerInformation()
{
  return this->LocalServerInformation;
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::PushState(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->PushState(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::PullState(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->PullState(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::ExecuteStream(
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
const vtkClientServerStream& vtkSMSessionBase::GetLastResult(
  vtkTypeUInt32 vtkNotUsed(location))
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  return this->SessionCore->GetLastResult();
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::DeletePMObject(vtkSMMessage* msg)
{
  this->Activate();

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->SessionCore->DeletePMObject(msg);

  this->DeActivate();
}

//----------------------------------------------------------------------------
vtkPMObject* vtkSMSessionBase::GetPMObject(vtkTypeUInt32 globalid)
{
  return this->SessionCore? this->SessionCore->GetPMObject(globalid) : NULL;
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::PrepareProgressInternal()
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for vtkSMSessionCore helper.
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
void vtkSMSessionBase::CleanupPendingProgressInternal()
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for vtkSMSessionCore helper.
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
bool vtkSMSessionBase::GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  return this->SessionCore->GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
vtkObject* vtkSMSessionBase::GetRemoteObject(vtkTypeUInt32 globalid)
{
  return this->SessionCore->GetRemoteObject(globalid);
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::RegisterRemoteObject(vtkTypeUInt32 gid, vtkObject* obj)
{
  this->SessionCore->RegisterRemoteObject(gid, obj);
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::UnRegisterRemoteObject(vtkTypeUInt32 gid)
{
  // FIXME_COLLABORATION: This is not right. Why isn't the PMObject deleted when
  // the SMObject was cleaning itself up?
  //// Make sure to delete PMObject as well
  //vtkSMMessage deleteMsg;
  //deleteMsg.set_location(obj->GetLocation());
  //deleteMsg.set_global_id(obj->GetGlobalID());
  this->SessionCore->UnRegisterRemoteObject(gid);
  //this->DeletePMObject(&deleteMsg);
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::GetAllRemoteObjects(vtkCollection* collection)
{
  this->SessionCore->GetAllRemoteObjects(collection);
}

//----------------------------------------------------------------------------
void vtkSMSessionBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

