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
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkReservedRemoteObjectIds.h"

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
}

//----------------------------------------------------------------------------
vtkSMCollaborationManager::~vtkSMCollaborationManager()
{
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkSMCollaborationManager::LoadState( const vtkSMMessage* msg,
                                                vtkSMProxyLocator* locator)
{
  // Execute locally
  vtkSMMessage* msgCopy = new vtkSMMessage();
  msgCopy->CopyFrom(*msg);
  this->InvokeEvent(CollaborationNotification, msgCopy);
}
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMCollaborationManager::GetGlobalID()
{
  if(!this->HasGlobalID())
    {
    this->SetGlobalID(vtkSMCollaborationManager::GetReservedGlobalID());
    }
  return this->Superclass::GetGlobalID();
}

//----------------------------------------------------------------------------
void vtkSMCollaborationManager::SendToOtherClients(vtkSMMessage* msg)
{
  msg->set_share_only(true);
  this->PushState(msg);
}
