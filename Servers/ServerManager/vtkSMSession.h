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

#include "vtkPVSession.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class vtkCollection;
class vtkPMObject;
class vtkPVInformation;
class vtkSMPluginManager;
class vtkSMProxyDefinitionManager;
class vtkSMRemoteObject;
class vtkSMSessionCore;
class vtkSMUndoStackBuilder;

class VTK_EXPORT vtkSMSession : public vtkPVSession
{
public:
  static vtkSMSession* New();
  vtkTypeMacro(vtkSMSession, vtkPVSession);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual bool GetIsAlive() { return true; }

  // Description:
  // Return the URL that define where the session is connected to.
  // This can be used to id two remote sessions running in the same application
  // But NOT the builtin ones.
  virtual const char* GetURI() { return "builtin:"; }

  // Description:
  // Returns a ServerFlags indicate the nature of the current processes. e.g. if
  // the current processes acts as a data-server and a render-server, it returns
  // DATA_SERVER | RENDER_SERVER.
  virtual ServerFlags GetProcessRoles();

  // Description:
  // vtkPVServerInformation is an information-object that provides information
  // about the server processes. These include server-side capabilities as well
  // as server-side command line arguments e.g. tile-display parameters. Use
  // this method to obtain the server-side information.
  // Overridden to provide support for non-remote-server case. We simply read
  // the local process information and return it.
  virtual vtkPVServerInformation* GetServerInformation();

  // Description:
  // Allow the user to bind an UndoStackBuilder with the given session
  virtual void SetUndoStackBuilder(vtkSMUndoStackBuilder*);
  vtkGetObjectMacro(UndoStackBuilder, vtkSMUndoStackBuilder);

  // Description:
  // Get the ProxyDefinitionManager.
  vtkSMProxyDefinitionManager* GetProxyDefinitionManager();

  // Description:
  // Called to do any initializations after a successful session has been
  // established.
  virtual void Initialize();

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
  void RegisterRemoteObject(vtkSMRemoteObject* obj);

  // Description:
  // Unregister a remote object
  void UnRegisterRemoteObject(vtkSMRemoteObject* obj);

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
  // Returns the vtkSMPluginManager attached to this session.
  vtkGetObjectMacro(PluginManager, vtkSMPluginManager);

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

  // Description:
  // These are static helper methods that help create standard ParaView
  // sessions. They register the session with the process module and return the
  // session id. Returns 0 on failure.
  // This overload is used to create a built-in session.
  static vtkIdType ConnectToSelf();

  // Description:
  // These are static helper methods that help create standard ParaView
  // sessions. They register the session with the process module and return the
  // session id. Returns 0 on failure.
  // This overload is used to create a client-server session on client.
  static vtkIdType ConnectToRemote(const char* hostname, int port);

  // Description:
  // These are static helper methods that help create standard ParaView
  // sessions. They register the session with the process module and return the
  // session id. Returns 0 on failure.
  // This overload is used to create a client-dataserver-renderserver session on
  // client.
  static vtkIdType ConnectToRemote(const char* dshost, int dsport,
    const char* rshost, int rsport);

//BTX
protected:
  vtkSMSession();
  ~vtkSMSession();

  vtkSMSessionCore* Core;
  vtkSMUndoStackBuilder* UndoStackBuilder;
  vtkSMPluginManager* PluginManager;

  // FIXME should be managed smartly between client and server.
  vtkTypeUInt32 LastGUID;

private:
  vtkSMSession(const vtkSMSession&); // Not implemented
  void operator=(const vtkSMSession&); // Not implemented

  vtkPVServerInformation* LocalServerInformation;
//ETX
};

#endif
