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
/**
 * @class   vtkSMSession
 *
 * vtkSMSession is the default ParaView session. This class can be used as the
 * session for non-client-server configurations eg. builtin mode or batch.
*/

#ifndef vtkSMSession_h
#define vtkSMSession_h

#include "vtkPVSessionBase.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSmartPointer.h"                // needed for vtkSmartPointer.

class vtkProcessModuleAutoMPI;
class vtkSMCollaborationManager;
class vtkSMProxyLocator;
class vtkSMSessionProxyManager;
class vtkSMStateLocator;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSession : public vtkPVSessionBase
{
public:
  static vtkSMSession* New();
  static vtkSMSession* New(vtkPVSessionBase* otherSession);
  static vtkSMSession* New(vtkPVSessionCore* otherSessionCore);
  vtkTypeMacro(vtkSMSession, vtkPVSessionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //---------------------------------------------------------------------------
  // API for collaboration management
  //---------------------------------------------------------------------------

  /**
   * Return the instance of vtkSMCollaborationManager that will be
   * lazy created at the first call.
   * By default we return NULL
   */
  virtual vtkSMCollaborationManager* GetCollaborationManager() { return NULL; }

  //---------------------------------------------------------------------------
  // API for client-side components of a session.
  //---------------------------------------------------------------------------

  /**
   * Return the URL that define where the session is connected to. URI has
   * enough information to know the type of connection, server hosts and ports.
   */
  virtual const char* GetURI() { return "builtin:"; }

  //@{
  /**
   * Returns the vtkSMSessionProxyManager associated with this session.
   */
  vtkGetObjectMacro(SessionProxyManager, vtkSMSessionProxyManager);
  //@}

  /**
   * Returns the number of processes on the given server/s. If more than 1
   * server is identified, then it returns the maximum number of processes e.g.
   * is servers = DATA_SERVER | RENDER_SERVER and there are 3 data-server nodes
   * and 2 render-server nodes, then this method will return 3.
   * Implementation provided simply returns the number of local processes.
   */
  virtual int GetNumberOfProcesses(vtkTypeUInt32 servers);

  /**
   * Returns whether or not MPI is initialized on the specified server/s. If
   * more than 1 server is identified it will return true only if all of the
   * servers have MPI initialized.
   */
  virtual bool IsMPIInitialized(vtkTypeUInt32 servers);

  //---------------------------------------------------------------------------
  // API for Proxy Finder/ReNew
  //---------------------------------------------------------------------------

  vtkGetObjectMacro(ProxyLocator, vtkSMProxyLocator);

  enum RenderingMode
  {
    RENDERING_NOT_AVAILABLE = 0x00,
    RENDERING_UNIFIED = 0x01,
    RENDERING_SPLIT = 0x02
  };

  /**
   * Convenient method to determine if the rendering is done in a pvrenderer
   * or not.
   * For built-in or pvserver you will get RENDERING_UNIFIED and for a setting
   * with a pvrenderer you will get RENDERING_SPLIT.
   * If the session is something else it should reply RENDERING_NOT_AVAILABLE.
   */
  virtual unsigned int GetRenderClientMode();

  //---------------------------------------------------------------------------
  // Undo/Redo related API.
  //---------------------------------------------------------------------------

  //@{
  /**
   * Provide an access to the session state locator that can provide the last
   * state of a given remote object that have been pushed.
   * That locator will be filled by RemoteObject state only if
   * the UndoStackBuilder in vtkSMProxyManager is non-null.
   */
  vtkGetObjectMacro(StateLocator, vtkSMStateLocator);
  //@}

  //---------------------------------------------------------------------------
  // Superclass Implementations
  //---------------------------------------------------------------------------

  /**
   * Builtin session is always alive.
   */
  bool GetIsAlive() override { return true; }

  /**
   * Returns a ServerFlags indicate the nature of the current processes. e.g. if
   * the current processes acts as a data-server and a render-server, it returns
   * DATA_SERVER | RENDER_SERVER.
   * The implementation provided by this class returns
   * vtkPVSession::CLIENT_AND_SERVERS suitable for builtin-mode.
   */
  ServerFlags GetProcessRoles() override;

  /**
   * Push the state message. Overridden to ensure that the information in the
   * undo-redo state manager is updated.
   */
  void PushState(vtkSMMessage* msg) override;

  /**
   * Sends the message to all clients.
   */
  void NotifyAllClients(const vtkSMMessage* msg) override { this->ProcessNotification(msg); }

  /**
   * Sends the message to all but the active client-session.
   */
  void NotifyOtherClients(const vtkSMMessage*) override { /* nothing to do. */}

  //---------------------------------------------------------------------------
  // API for Collaboration management
  //---------------------------------------------------------------------------

  // Called before application quit or session disconnection
  virtual void PreDisconnection() {}

  //---------------------------------------------------------------------------
  // Static methods to create and register sessions easily.
  //---------------------------------------------------------------------------

  /**
   * These are static helper methods that help create Catalyst ParaView
   * sessions. They register the session with the process module and return the
   * session id. Returns 0 on failure.
   * This overload is used to create a catalyst built-in session.
   */
  static vtkIdType ConnectToCatalyst();

  /**
   * These are static helper methods that help create standard ParaView
   * sessions. They register the session with the process module and return the
   * session id. It will try to connect for timeout seconds, -1 meaning infinite retries.
   * and 0 means no retry. Returns 0 on failure.
   * This overload is used to create a built-in session.
   */
  static vtkIdType ConnectToSelf(int timeout = 60);

  /**
   * These are static helper methods that help create standard ParaView
   * sessions. They register the session with the process module and return the
   * session id. It will try to connect for timeout seconds, -1 meaning infinite retries.
   * and 0 means no retry. Returns 0 on failure. Returns 0 on failure.
   * This overload is used to create a client-server session on client.
   */
  static vtkIdType ConnectToRemote(const char* hostname, int port, int timeout = 60);

  /**
   * Same as ConnectToRemote() except that it waits for a reverse connection.
   * This is a blocking call. One can optionally provide a callback that can be
   * called periodically while this call is blocked.
   * The callback should return true, if the connection should continue waiting,
   * else return false to abort the wait.
   */
  static vtkIdType ReverseConnectToRemote(int port)
  {
    return vtkSMSession::ReverseConnectToRemote(port, (bool (*)())NULL);
  }
  static vtkIdType ReverseConnectToRemote(int port, bool (*callback)());

  /**
   * These are static helper methods that help create standard ParaView
   * sessions. They register the session with the process module and return the
   * session id. It will try to connect for timeout seconds, -1 meaning infinite retries.
   * and 0 means no retry. Returns 0 on failure.
   * This overload is used to create a client-dataserver-renderserver session on client.
   */
  static vtkIdType ConnectToRemote(
    const char* dshost, int dsport, const char* rshost, int rsport, int timeout = 60);

  /**
   * Same as ConnectToRemote() except that it waits for a reverse connection.
   * This is a blocking call. One can optionally provide a callback that can be
   * called periodically while this call is blocked.
   * The callback should return true, if the connection should continue waiting,
   * else return false to abort the wait.
   */
  static vtkIdType ReverseConnectToRemote(int dsport, int rsport)
  {
    return vtkSMSession::ReverseConnectToRemote(dsport, rsport, NULL);
  }
  static vtkIdType ReverseConnectToRemote(int dsport, int rsport, bool (*callback)());

  //@{
  /**
   * Use this method to disconnect from a session. This ensures that
   * appropriate cleanup happens before the disconnect such as unregistering
   * proxies. It also ensures that if in collaboration mode, the proxy
   * unregistering doesn't affect other connected clients.
   */
  static void Disconnect(vtkIdType sessionid);
  static void Disconnect(vtkSMSession* session);
  //@}

  //@{
  /**
   * This flag if set indicates that the current session
   * module has automatically started "pvservers" as MPI processes as
   * default pipeline.
   */
  vtkGetMacro(IsAutoMPI, bool);
  //@}

protected:
  // Subclasses should set initialize_during_constructor to false so that
  // this->Initialize() is not called in constructor but only after the session
  // has been created/setup correctly.
  vtkSMSession(
    bool initialize_during_constructor = true, vtkPVSessionCore* preExistingSessionCore = NULL);
  ~vtkSMSession() override;

  /**
   * Internal method used by ConnectToRemote().
   */
  static vtkIdType ConnectToRemoteInternal(
    const char* hostname, int port, bool is_auto_mpi, int timeout = 60);

  /**
   * Process the Notifation message sent using API to communicate from
   * server-to-client.
   */
  virtual void ProcessNotification(const vtkSMMessage*);

  /**
   * Initialize various internal classes after the session has been setup
   * correctly.
   */
  virtual void Initialize();

  /**
   * This method has been externalized so classes that heritate from vtkSMSession
   * and override PushState could easily keep track of the StateHistory and
   * maintain the UndoRedo mecanisme.
   */
  void UpdateStateHistory(vtkSMMessage* msg);

  vtkSMSessionProxyManager* SessionProxyManager;
  vtkSMStateLocator* StateLocator;
  vtkSMProxyLocator* ProxyLocator;

  bool IsAutoMPI;

private:
  vtkSMSession(const vtkSMSession&) = delete;
  void operator=(const vtkSMSession&) = delete;

  // AutoMPI helper class
  static vtkSmartPointer<vtkProcessModuleAutoMPI> AutoMPI;
};

#endif
