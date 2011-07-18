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

#include "vtkPVSessionBase.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkSMPluginManager;
class vtkSMUndoStackBuilder;
class vtkSMStateLocator;
class vtkProcessModuleAutoMPI;

class VTK_EXPORT vtkSMSession : public vtkPVSessionBase
{
public:
  static vtkSMSession* New();
  vtkTypeMacro(vtkSMSession, vtkPVSessionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  //---------------------------------------------------------------------------
  // API for client-side components of a session.
  //---------------------------------------------------------------------------

  // Description:
  // Return the URL that define where the session is connected to. URI has
  // enough information to know the type of connection, server hosts and ports.
  virtual const char* GetURI() { return "builtin:"; }

  // Description:
  // Returns the vtkSMPluginManager attached to this session.
  vtkGetObjectMacro(PluginManager, vtkSMPluginManager);

  // Description:
  // Returns the number of processes on the given server/s. If more than 1
  // server is identified, than it returns the maximum number of processes e.g.
  // is servers = DATA_SERVER | RENDER_SERVER and there are 3 data-server nodes
  // and 2 render-server nodes, then this method will return 3.
  // Implementation provided simply returns the number of local processes.
  virtual int GetNumberOfProcesses(vtkTypeUInt32 servers);

  // Description:
  // Provides a unique identifier across processes of that session. Default
  // implementation simply uses a local variable to keep track of ids already
  // assigned.
  virtual vtkTypeUInt32 GetNextGlobalUniqueIdentifier()
    {
    this->LastGUID++;
    return this->LastGUID;
    }

  enum RenderingMode
    {
    RENDERING_NOT_AVAILABLE = 0x00,
    RENDERING_UNIFIED = 0x01,
    RENDERING_SPLIT = 0x02
    };

  // Description:
  // Convenient method to determine if the rendering is done in a pvrenderer
  // or not.
  // For built-in or pvserver you will get RENDERING_UNIFIED and for a setting
  // with a pvrenderer you will get RENDERING_SPLIT.
  // If the session is something else it should reply RENDERING_NOT_AVAILABLE.
  virtual unsigned int GetRenderClientMode();

  //---------------------------------------------------------------------------
  // Undo/Redo related API.
  //---------------------------------------------------------------------------

  // Description:
  // Allow the user to bind an UndoStackBuilder with the given session
  virtual void SetUndoStackBuilder(vtkSMUndoStackBuilder*);
  vtkGetObjectMacro(UndoStackBuilder, vtkSMUndoStackBuilder);


  // Description:
  // Flag used to disable state caching needed for undo/redo. This overcome the
  // presence of undo stack builder in the session.
  vtkBooleanMacro(StateManagement, bool);
  vtkSetMacro(StateManagement, bool);
  vtkGetMacro(StateManagement, bool);


  // Description:
  // Provide an access to the session state locator that can provide the last
  // state of a given remote object that have been pushed.
  // That locator will be filled by RemoteObject state only if
  // StateManagement is set to true.
  vtkGetObjectMacro(StateLocator, vtkSMStateLocator);

  //---------------------------------------------------------------------------
  // Superclass Implementations
  //---------------------------------------------------------------------------

  // Description:
  // Builtin session is always alive.
  virtual bool GetIsAlive() { return true; }

  // Description:
  // Returns a ServerFlags indicate the nature of the current processes. e.g. if
  // the current processes acts as a data-server and a render-server, it returns
  // DATA_SERVER | RENDER_SERVER.
  // The implementation provided by this class returns
  // vtkPVSession::CLIENT_AND_SERVERS suitable for builtin-mode.
  virtual ServerFlags GetProcessRoles();

//BTX
  // Description:
  // Push the state message. Overridden to ensure that the information in the
  // undo-redo state manager is updated.
  virtual void PushState(vtkSMMessage* msg);
//ETX


  //---------------------------------------------------------------------------
  // Static methods to create and register sessions easily.
  //---------------------------------------------------------------------------

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
  // Same as ConnectToRemote() except that it waits for a reverse connection.
  // This is a blocking call. One can optionally provide a callback that can be
  // called periodically while this call is blocked.
  // The callback should return true, if the connection should continue waiting,
  // else return false to abort the wait.
  static vtkIdType ReverseConnectToRemote(int port)
    { return vtkSMSession::ReverseConnectToRemote(port, (bool (*)()) NULL); }
  static vtkIdType ReverseConnectToRemote(int port, bool (*callback)());

  // Description:
  // These are static helper methods that help create standard ParaView
  // sessions. They register the session with the process module and return the
  // session id. Returns 0 on failure.
  // This overload is used to create a client-dataserver-renderserver session on
  // client.
  static vtkIdType ConnectToRemote(const char* dshost, int dsport,
    const char* rshost, int rsport);

  // Description:
  // Same as ConnectToRemote() except that it waits for a reverse connection.
  // This is a blocking call. One can optionally provide a callback that can be
  // called periodically while this call is blocked.
  // The callback should return true, if the connection should continue waiting,
  // else return false to abort the wait.
  static vtkIdType ReverseConnectToRemote(int dsport, int rsport)
    { return vtkSMSession::ReverseConnectToRemote(dsport, rsport, NULL); }
  static vtkIdType ReverseConnectToRemote(int dsport, int rsport, bool (*callback)());

  // Description:
  // This flag if set indicates that the current session
  // module has automatically started "pvservers" as MPI processes as
  // default pipeline.
  vtkGetMacro(IsAutoMPI, bool);

//BTX
protected:
  // Subclasses should set initialize_during_constructor to false so that
  // this->Initialize() is not called in constructor but only after the session
  // has been created/setup correctly.
  vtkSMSession(bool initialize_during_constructor=true);
  ~vtkSMSession();

  // Used by the Auto-MPI to prevent remote rendering, otherwise we should
  // always allow it.
  static vtkIdType ConnectToRemote(const char* hostname, int port,
                                   bool allowRemoteRendering);

  // Description:
  // Initialize various internal classes after the session has been setup
  // correctly.
  virtual void Initialize();

  // Description:
  // This method has been externalized so classes that heritate from vtkSMSession
  // and override PushState could easily keep track of the StateHistory and
  // maintain the UndoRedo mecanisme.
  void UpdateStateHistory(vtkSMMessage* msg);

  vtkSMUndoStackBuilder* UndoStackBuilder;
  vtkSMPluginManager* PluginManager;
  vtkSMStateLocator* StateLocator;
  bool StateManagement;

  // GlobalID managed locally
  vtkTypeUInt32 LastGUID;
  bool IsAutoMPI;

private:
  vtkSMSession(const vtkSMSession&); // Not implemented
  void operator=(const vtkSMSession&); // Not implemented

  // AutoMPI helper class
  static vtkSmartPointer<vtkProcessModuleAutoMPI> AutoMPI;
//ETX
};

#endif
