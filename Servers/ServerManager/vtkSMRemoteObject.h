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
// corresponding PMObject which can be local or remote.

#ifndef __vtkSMRemoteObject_h
#define __vtkSMRemoteObject_h

#include "vtkSMObject.h"
#include "vtkSMMessage.h"
#include "vtkWeakPointer.h"

class vtkSMSession;

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
  // Same as Push() except that the msg is not treated as a state message instead
  // just an instantaneous trigger that is not synchronized among processes.
  void Invoke(vtkSMMessage* msg);

  // Description:
  // This method return the full object state that can be used to create that
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState() = 0;

  // Description:
  // This method is used to initialise the object to the given state
  virtual void LoadState(const vtkSMMessage* msg) = 0;

  // Description:
  // Destroys the vtkPMObject associated with this->GlobalID.
  void DestroyPMObject();

  // Description:
  // Set the GlobalUniqueId
  void SetGlobalID(vtkTypeUInt32 guid);

  // Global-ID for this vtkSMRemoteObject. This is assigned when needed.
  // Assigned at :
  // - First push
  // - or when the RemoteObject is created by the ProcessModule remotely.
  vtkTypeUInt32 GlobalID;

  // Location flag identify the processes on which the vtkPMObject
  // corresponding to this vtkSMRemoteObject exist.
  vtkTypeUInt32 Location;

  // Identifies the session id to which this object is related.
  vtkWeakPointer<vtkSMSession> Session;

private:
  vtkSMRemoteObject(const vtkSMRemoteObject&); // Not implemented
  void operator=(const vtkSMRemoteObject&);       // Not implemented
};

#endif // #ifndef __vtkSMRemoteObject_h
