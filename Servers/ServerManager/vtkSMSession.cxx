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
#include "vtkProcessModule2.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMSessionCore.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkWeakPointer.h"

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

  this->UndoStackBuilder = NULL;

  // The 10 first ID are reserved
  //  - 1: vtkSMProxyManager
  this->LastGUID = 10;

  // Reserved Id management
  this->RegisterRemoteObject(1, this->ProxyManager);
}

//----------------------------------------------------------------------------
vtkSMSession::~vtkSMSession()
{
  this->Core->Delete();
  this->Core = NULL;
  this->ProxyManager->Delete();
  this->ProxyManager = NULL;
  this->SetUndoStackBuilder(0);
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
  return this->Core->GetPMObject(globalid);
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMSession::GetProxyManager()
{
  return this->ProxyManager;
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
void vtkSMSession::RegisterRemoteObject(vtkTypeUInt32 globalid, vtkSMRemoteObject* obj)
{
  this->Core->RegisterRemoteObject(globalid, obj);
}

//----------------------------------------------------------------------------
void vtkSMSession::UnRegisterRemoteObject(vtkTypeUInt32 globalid)
{
  // Make sure to delete PMObject as well
  vtkSMMessage deleteMsg;
  deleteMsg.set_location(this->GetRemoteObject(globalid)->GetLocation());
  deleteMsg.set_global_id(globalid);

  this->Core->UnRegisterRemoteObject(globalid);
  this->DeletePMObject(&deleteMsg);
}
//----------------------------------------------------------------------------
void vtkSMSession::GetAllRemoteObjects(vtkCollection* collection)
{
  this->Core->GetAllRemoteObjects(collection);
}
