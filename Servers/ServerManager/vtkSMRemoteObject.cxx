/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObject.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRemoteObject.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"

//----------------------------------------------------------------------------
vtkSMRemoteObject::vtkSMRemoteObject()
{
  this->GlobalID = 0;
}

//----------------------------------------------------------------------------
vtkSMRemoteObject::~vtkSMRemoteObject()
{
}

//----------------------------------------------------------------------------
vtkSMSession* vtkSMRemoteObject::GetSession()
{
  return this->Session;
}

//----------------------------------------------------------------------------
void vtkSMRemoteObject::SetSession(vtkSMSession* session)
{
  if (this->Session != session)
    {
    this->Session = session;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSMRemoteObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Session: " << this->Session << endl;
  os << indent << "GlobalID: " << this->GlobalID << endl;
}

//---------------------------------------------------------------------------
vtkTypeUInt32 vtkSMRemoteObject::GetGlobalID()
{
  if (this->Session != NULL && this->GlobalID == 0)
    {
    this->GlobalID = this->GetSession()->GetNextGlobalUniqueIdentifier();
    }
  return this->GlobalID;
}

//---------------------------------------------------------------------------
void vtkSMRemoteObject::PushState(vtkSMMessage* msg)
{
  // Check if a GUID has been assigned to that object otherwise assign a new one
  vtkTypeUInt32 gid = this->GetGlobalID();
  msg->set_global_id(gid);
  msg->set_location(this->Location);
  if (this->GetSession())
    {
    this->GetSession()->PushState(msg);
    }
  else
    {
    // FIXME Throw exception or error feed back : Not PVSession found !
    }
}

//---------------------------------------------------------------------------
bool vtkSMRemoteObject::PullState(vtkSMMessage* msg)
{
  msg->set_global_id(this->GlobalID);
  msg->set_location(this->Location);
  if(this->GetSession())
    {
    this->GetSession()->PullState(msg);
    }
  else
    {
    cout << "No session found" << endl;
    // FIXME Throw exception or error feed back : Not PVSession found !
    return false;
    }
  return true; // Successful call
}

//---------------------------------------------------------------------------
void vtkSMRemoteObject::Invoke(vtkSMMessage* msg)
{
  msg->set_global_id(this->GlobalID);
  msg->set_location(this->Location);
  if(this->GetSession())
    {
    // FIXME this->GetSession()->InvokeEvent(this->ConnectionID, msg);
    }
  else
    {
    // FIXME Throw exception or error feed back : Not PVSession found !
    }
}

//---------------------------------------------------------------------------
void vtkSMRemoteObject::DestroyPMObject()
{
  Message msg;
  msg.set_global_id(this->GlobalID);
  msg.set_location(this->Location);
  this->GetSession()->DeletePMObject(&msg);
}
