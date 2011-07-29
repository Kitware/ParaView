/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRemoteObject - baseclass for all proxy-objects that have counter
// parts on server as well as client processes.
// .SECTION Description
// Abstract class involved in ServerManager class hierarchy that has a
// corresponding SIObject which can be local or remote.

#ifndef __vtkSMRemoteObject_h
#define __vtkSMRemoteObject_h

#include "vtkSMObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkClientServerStream;
class vtkSMSession;
class vtkSMStateLocator;

class VTK_EXPORT vtkSMRemoteObject : public vtkSMObject
{
// My friends are...
  friend class vtkSMStateHelper; // To pull state

public:
  vtkTypeMacro(vtkSMRemoteObject,vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the location where the underlying VTK-objects are created. The
  // value can be contructed by or-ing vtkSMSession::ServerFlags
  vtkSetMacro(Location, vtkTypeUInt32);
  vtkGetMacro(Location, vtkTypeUInt32);

  // Description:
  // Get/Set the session on wihch this object exists.
  // Note that session is not reference counted.
  void SetSession(vtkSMSession*);
  vtkSMSession* GetSession();

  // Description:
  // Get the global unique id for this object. If none is set and the session is
  // valid, a new global id will be assigned automatically.
  vtkTypeUInt32 GetGlobalID();
  const char* GetGlobalIDAsString();

  // Description:
  // Allow the user to test if the RemoteObject has already a GlobalID without
  // assigning a new one to it.
  bool HasGlobalID();

  // Description:
  // Allow user to set the remote object to be discard for Undo/Redo
  // action. By default, any remote object is Undoable.
  vtkBooleanMacro(Prototype, bool);
  bool IsPrototype() {return this->Prototype;}
  vtkSetMacro(Prototype, bool);

//BTX

  // Description:
  // This method return the full object state that can be used to create that
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState()
    { return NULL; }

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMStateLocator* locator,
                          bool definitionOnly )
    {
    (void) msg;
    (void) locator;
    (void) definitionOnly;
    }


protected:
  // Description:
  // Default constructor.
  vtkSMRemoteObject();

  // Description:
  // Destructor.
  virtual ~vtkSMRemoteObject();

  // Description:
  // Subclasses can call this method to send a message to its state
  // object on  the server processes specified.
  void PushState(vtkSMMessage* msg);

  // Description:
  // Subclasses can call this method to pull the state from the
  // state-object on the server processes specified. Returns true on successful
  // fetch. The message is updated with the fetched state.
  bool PullState(vtkSMMessage* msg);

  // Description:
  // Set the GlobalUniqueId
  void SetGlobalID(vtkTypeUInt32 guid);

  // Global-ID for this vtkSMRemoteObject. This is assigned when needed.
  // Assigned at :
  // - First push
  // - or when the RemoteObject is created by the ProcessModule remotely.
  vtkTypeUInt32 GlobalID;

  // Location flag identify the processes on which the vtkSIObject
  // corresponding to this vtkSMRemoteObject exist.
  vtkTypeUInt32 Location;

  // Identifies the session id to which this object is related.
  vtkWeakPointer<vtkSMSession> Session;

  // Allow remote object to be discard for any state management such as
  // Undo/Redo, Register/UnRegister (in ProxyManager) and so on...
  bool Prototype;

private:
  vtkSMRemoteObject(const vtkSMRemoteObject&); // Not implemented
  void operator=(const vtkSMRemoteObject&);       // Not implemented

  char* GlobalIDString;
//ETX
};

// This defines a manipulator for the vtkClientServerStream that can be used on
// the to indicate to the interpreter that the placeholder is to be replaced by
// the vtkSIProxy instance for the given vtkSMProxy instance.
// e.g.
// <code>
// vtkClientServerStream stream;
// stream << vtkClientServerStream::Invoke
//        << SIOBJECT(proxyA)
//        << "MethodName"
//        << vtkClientServerStream::End;
// </code>
// Will result in calling the vtkSIProxy::MethodName() when the stream in
// interpreted.
class VTK_EXPORT SIOBJECT
{
  vtkSMRemoteObject* Reference;
  friend VTK_EXPORT vtkClientServerStream& operator<<(
    vtkClientServerStream& stream, const SIOBJECT& manipulator);
public:
  SIOBJECT(vtkSMRemoteObject* rmobject) : Reference(rmobject) {}
};

VTK_EXPORT vtkClientServerStream& operator<< (vtkClientServerStream& stream,
  const SIOBJECT& manipulator);

#endif // #ifndef __vtkSMRemoteObject_h
