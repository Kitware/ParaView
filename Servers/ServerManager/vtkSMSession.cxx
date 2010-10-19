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
#include "vtkSMProxyManager.h"
#include "vtkSMSessionCore.h"
#include "vtkWeakPointer.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMSession);
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
}

//----------------------------------------------------------------------------
vtkSMSession::~vtkSMSession()
{
  this->Core->Delete();
  this->Core = NULL;
  this->ProxyManager->Delete();
  this->ProxyManager = NULL;

  if(this->UndoStackBuilder)
    {
    this->UndoStackBuilder->Delete();
    this->UndoStackBuilder = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSMSession::PushState(vtkSMMessage* msg)
{
  // Manage Undo/Redo if possible
  if(this->UndoStackBuilder)
    {
    vtkSMProxy *proxy = vtkSMProxy::SafeDownCast(this->GetRemoteObject(msg->global_id()));
    if(proxy)
      {
      vtkSMMessage fullState = *proxy->GetFullState();

      // Need to provide id/location as the full state have not them yet
      fullState.set_global_id(msg->global_id());
      fullState.set_location(msg->location());

      this->UndoStackBuilder->OnNewState(this, msg->global_id(), &fullState);
      }
    else
      {
      cout << "Push a state that is not related to a proxt." << endl;
      }
    }

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->PushState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::PullState(vtkSMMessage* msg)
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->PullState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::Invoke(vtkSMMessage* msg)
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->Invoke(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::DeletePMObject(vtkSMMessage* msg)
{
  // Manage Undo/Redo if possible
  if(this->UndoStackBuilder)
    {
    this->UndoStackBuilder->OnNewState(this, msg->global_id(), NULL);
    }

  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->DeletePMObject(msg);
}

//----------------------------------------------------------------------------
vtkPMObject* vtkSMSession::GetPMObject(vtkTypeUInt32 globalid)
{
  this->Core->GetPMObject(globalid);
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
