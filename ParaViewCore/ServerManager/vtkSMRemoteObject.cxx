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
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
vtkSMRemoteObject::vtkSMRemoteObject()
{
  this->GlobalID = 0;
  this->GlobalIDString = NULL;
  this->Location = 0;
  this->Session = NULL;
  this->Prototype = false;
}

//----------------------------------------------------------------------------
vtkSMRemoteObject::~vtkSMRemoteObject()
{
  if(this->Session && this->GlobalID != 0)
    {
    this->Session->UnRegisterRemoteObject(this->GlobalID, this->Location);
    }
  this->SetSession(0);
  delete [] this->GlobalIDString;
  this->GlobalIDString = NULL;
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
  // Register object if possible
  if(this->Session && this->GlobalID != 0)
    {
    this->Session->RegisterRemoteObject(this->GlobalID, this);
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
bool vtkSMRemoteObject::HasGlobalID()
{
  return this->GlobalID != 0;
}

//---------------------------------------------------------------------------
vtkTypeUInt32 vtkSMRemoteObject::GetGlobalID()
{
  if (this->Session != NULL && this->GlobalID == 0)
    {
    this->GlobalID = this->GetSession()->GetNextGlobalUniqueIdentifier();
    // Register object
    this->Session->RegisterRemoteObject(this->GlobalID, this);

    vtksys_ios::ostringstream cname;
    cname << this->GlobalID;

    delete [] this->GlobalIDString;
    this->GlobalIDString = vtksys::SystemTools::DuplicateString(
      cname.str().c_str());
    }

  return this->GlobalID;
}

//---------------------------------------------------------------------------
const char* vtkSMRemoteObject::GetGlobalIDAsString()
{
  if (!this->GlobalIDString)
    {
    this->GetGlobalID();
    }

  return this->GlobalIDString;
}

//---------------------------------------------------------------------------
void vtkSMRemoteObject::SetGlobalID(vtkTypeUInt32 guid)
{
  if(this->GlobalID == guid)
    {
    return;
    }

  if(this->GlobalID != 0)
    {
    vtkErrorMacro("GlobalID must NOT be changed once it has been assigned.\n"
                  "Try to set " << guid << " to replace the current "
                  << this->GlobalID << " value.");
    abort();
    }

  // Keep new ID
  this->GlobalID = guid;

  // Register object if possible
  if(this->Session && this->GlobalID != 0)
    {
    this->Session->RegisterRemoteObject(this->GlobalID, this);
    }
}

//---------------------------------------------------------------------------
void vtkSMRemoteObject::PushState(vtkSMMessage* msg)
{
  if(this->Location == 0)
    {
    return; // This object is a prototype and has no location
    }

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
    vtkErrorMacro("Attempting to PushState() on a " << this->GetClassName()
      << " after the session has been destroyed.");
    }
}

//---------------------------------------------------------------------------
bool vtkSMRemoteObject::PullState(vtkSMMessage* msg)
{
  if(this->Location == 0)
    {
    return true; // This object is a prototype and has no location
    }

  msg->set_global_id(this->GlobalID);
  msg->set_location(this->Location);
  if(this->GetSession())
    {
    this->GetSession()->PullState(msg);
    }
  else
    {
    vtkErrorMacro("Attempting to PullState() on a " << this->GetClassName()
      << " after the session has been destroyed.");
    return false;
    }
  return true; // Successful call
}

//---------------------------------------------------------------------------
vtkClientServerStream& operator<< (vtkClientServerStream& stream,
  const SIOBJECT& manipulator)
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for the vtkSMSessionCore helper.
            << "GetSIObject"
            << manipulator.Reference->GetGlobalID()
            << vtkClientServerStream::End;
  stream << substream;
  return stream;
}
