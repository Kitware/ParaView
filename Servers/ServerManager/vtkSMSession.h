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
// .NAME vtkSMSession
// .SECTION Description
// vtkSMSession is the default ParaView session. This class can be used as the
// session for non-client-server configurations eg. builtin mode or batch.
#ifndef __vtkSMSession_h
#define __vtkSMSession_h

#include "vtkSession.h"
#include "vtkSMMessage.h"
#include "vtkSMUndoStackBuilder.h"

class vtkSMProxyManager;
class vtkSMSessionCore;
class vtkPVInformation;
class vtkPMObject;
class vtkSMRemoteObject;
class vtkCollection;

class VTK_EXPORT vtkSMSession : public vtkSession
{
public:
  static vtkSMSession* New();
  vtkTypeMacro(vtkSMSession, vtkSession);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual bool GetIsAlive() { return true; }

  // Description:
  // Allow the user to bind an UndoStackBuilder with the given session
  vtkSetObjectMacro(UndoStackBuilder, vtkSMUndoStackBuilder);
  vtkGetObjectMacro(UndoStackBuilder, vtkSMUndoStackBuilder);


//BTX
  // Description:
  // Push the state message.
  virtual void PushState(vtkSMMessage* msg);

  // Description:
  // Pull the state message.
  virtual void PullState(vtkSMMessage* msg);

  // Description:
  // Invoke a method remotely
  virtual void Invoke(vtkSMMessage* msg);

  // Description:
  // Delete server side object. (PMObject)
  virtual void DeletePMObject(vtkSMMessage* msg);
//ETX

  // Description:
  // Provides access to the session core.
  vtkGetObjectMacro(Core, vtkSMSessionCore);

  // Description:
  // Returns a vtkPMObject or subclass given its global id, if any.
  vtkPMObject* GetPMObject(vtkTypeUInt32 globalid);

  // Description:
  // Return a vtkSMRemoteObject given its global id if any otherwise return NULL;
  vtkSMRemoteObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Register a remote object
  void RegisterRemoteObject(vtkTypeUInt32 globalid, vtkSMRemoteObject* obj);

  // Description:
  // Unregister a remote object
  void UnRegisterRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Allow the user to fill its vtkCollection with all RemoteObject
  // This could be usefull when you want to hold a reference to them to
  // prevent any deletion across several method call.
  virtual void GetAllRemoteObjects(vtkCollection* collection);

  // Description:
  // Provides a unique identifier across processes of that session
  virtual vtkTypeUInt32 GetNextGlobalUniqueIdentifier()
    {
    this->LastGUID++;
    return this->LastGUID;
    }

  // Description:
  // Returns the vtkSMProxyManager attached to that session. Note that the
  // ProxyManager may not be active on every process e.g. in client-server
  // configurations, the proxy manager is active only on the client. Using the
  // proxy-manager on the server in such a session can have unexpected results.
  virtual vtkSMProxyManager* GetProxyManager();

  // Description:
  // Gather information about an object referred by the \c globalid.
  // \c location identifies the processes to gather the information from.
  virtual bool GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid);

  // Description:
  // Returns the number of processes on the given server/s. If more than 1
  // server is identified, than it returns the maximum number of processes e.g.
  // is servers = DATA_SERVER | RENDER_SERVER and there are 3 data-server nodes
  // and 2 render-server nodes, then this method will return 3.
  virtual int GetNumberOfProcesses(vtkTypeUInt32 servers);

//BTX
protected:
  vtkSMSession();
  ~vtkSMSession();

  vtkSMSessionCore* Core;
  vtkSMProxyManager* ProxyManager;
  vtkSMUndoStackBuilder* UndoStackBuilder;

  // FIXME should be managed smartly between client and server.
  vtkTypeUInt32 LastGUID;

private:
  vtkSMSession(const vtkSMSession&); // Not implemented
  void operator=(const vtkSMSession&); // Not implemented
//ETX
};

#endif
