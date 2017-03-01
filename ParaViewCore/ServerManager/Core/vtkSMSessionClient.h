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
 * @class   vtkSMSessionClient
 *
 * vtkSMSessionClient is a remote-session that connects to a remote server.
 * vtkSMSessionClient supports both connecting a pvserver as well as connecting
 * a pvdataserver/pvrenderserver.
*/

#ifndef vtkSMSessionClient_h
#define vtkSMSessionClient_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMSession.h"

class vtkMultiProcessController;
class vtkPVServerInformation;
class vtkSMCollaborationManager;
class vtkSMProxyLocator;
class vtkSMProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSessionClient : public vtkSMSession
{
public:
  static vtkSMSessionClient* New();
  vtkTypeMacro(vtkSMSessionClient, vtkSMSession);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the url used to connect the current session to a server
   */
  virtual const char* GetURI() VTK_OVERRIDE { return this->URI; };

  /**
   * Connects a remote server. URL can be of the following format:
   * cs://<pvserver-host>:<pvserver-port>
   * cdsrs://<pvdataserver-host>:<pvdataserver-port>/<pvrenderserver-host>:<pvrenderserver-port>
   * In both cases the port is optional. When not provided default
   * pvserver/pvdataserver port // is 11111, while default pvrenderserver port
   * is 22221.
   * For reverse connect i.e. the client waits for the server to connect back,
   * simply add "rc" to the protocol e.g.
   * csrc://<pvserver-host>:<pvserver-port>
   * cdsrsrc://<pvdataserver-host>:<pvdataserver-port>/<pvrenderserver-host>:<pvrenderserver-port>
   * In this case, the hostname is irrelevant and is ignored.
   */
  virtual bool Connect(const char* url);

  /**
   * Returns true is this session is active/alive/valid.
   */
  virtual bool GetIsAlive() VTK_OVERRIDE;

  /**
   * Returns a ServerFlags indicate the nature of the current processes. e.g. if
   * the current processes acts as a data-server and a render-server, it returns
   * DATA_SERVER | RENDER_SERVER.
   * Overridden to return CLIENT since this process only acts as the client.
   */
  virtual ServerFlags GetProcessRoles() VTK_OVERRIDE { return CLIENT; }

  /**
   * Returns the controller used to communicate with the process. Value must be
   * DATA_SERVER_ROOT or RENDER_SERVER_ROOT or CLIENT.
   */
  virtual vtkMultiProcessController* GetController(ServerFlags processType) VTK_OVERRIDE;

  /**
   * vtkPVServerInformation is an information-object that provides information
   * about the server processes. These include server-side capabilities as well
   * as server-side command line arguments e.g. tile-display parameters. Use
   * this method to obtain the server-side information.
   * Overridden to provide return the information gathered from data-server and
   * render-server.
   */
  virtual vtkPVServerInformation* GetServerInformation() VTK_OVERRIDE
  {
    return this->ServerInformation;
  }

  /**
   * Called to do any initializations after a successful session has been
   * established. Initialize the data-server-render-server connection, if
   * applicable.
   */
  virtual void Initialize() VTK_OVERRIDE;

  //@{
  /**
   * Push the state.
   */
  virtual void PushState(vtkSMMessage* msg) VTK_OVERRIDE;
  virtual void PullState(vtkSMMessage* message) VTK_OVERRIDE;
  virtual void ExecuteStream(vtkTypeUInt32 location, const vtkClientServerStream& stream,
    bool ignore_errors = false) VTK_OVERRIDE;
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 location) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * When Connect() is waiting for a server to connect back to the client (in
   * reverse connect mode), then it periodically fires ProgressEvent.
   * Application can add observer to this signal and set this flag to true, if
   * it wants to abort the wait for the server.
   */
  vtkSetMacro(AbortConnect, bool);
  //@}

  /**
   * Gracefully exits the session.
   */
  void CloseSession();

  /**
   * Gather information about an object referred by the \c globalid.
   * \c location identifies the processes to gather the information from.
   * Overridden to fetch the information from server if needed, otherwise it's
   * handled locally.
   */
  virtual bool GatherInformation(
    vtkTypeUInt32 location, vtkPVInformation* information, vtkTypeUInt32 globalid) VTK_OVERRIDE;

  /**
   * Returns the number of processes on the given server/s. If more than 1
   * server is identified, than it returns the maximum number of processes e.g.
   * is servers = DATA_SERVER | RENDER_SERVER and there are 3 data-server nodes
   * and 2 render-server nodes, then this method will return 3.
   */
  virtual int GetNumberOfProcesses(vtkTypeUInt32 servers) VTK_OVERRIDE;

  /**
   * Returns whether or not MPI is initialized on the specified server/s. If
   * more than 1 server is identified it will return true only if all of the
   * servers have MPI initialized.
   */
  virtual bool IsMPIInitialized(vtkTypeUInt32 servers) VTK_OVERRIDE;

  //---------------------------------------------------------------------------
  // API for Collaboration management
  //---------------------------------------------------------------------------

  // Called before application quit or session disconnection
  // Used to prevent quiting client to delete proxy of a running session.
  virtual void PreDisconnection() VTK_OVERRIDE;

  /**
   * Flag used to know if it is a good time to handle server notification.
   */
  virtual bool IsNotBusy();
  /**
   * BusyWork should be declared inside method that will request several
   * network call that we don't want to interupt such as GatherInformation
   * and Pull.
   */
  virtual void StartBusyWork();
  /**
   * BusyWork should be declared inside method that will request several
   * network call that we don't want to interupt such as GatherInformation
   * and Pull.
   */
  virtual void EndBusyWork();

  /**
   * Return the instance of vtkSMCollaborationManager that will be
   * lazy created at the first call.
   */
  virtual vtkSMCollaborationManager* GetCollaborationManager() VTK_OVERRIDE;

  //---------------------------------------------------------------------------
  // API for GlobalId management
  //---------------------------------------------------------------------------

  /**
   * Provides the next available identifier. This implementation works locally.
   * without any code distribution. To support the distributed architecture
   * the vtkSMSessionClient overide those method to call them on the DATA_SERVER
   * vtkPVSessionBase instance.
   */
  virtual vtkTypeUInt32 GetNextGlobalUniqueIdentifier() VTK_OVERRIDE;

  /**
   * Return the first Id of the requested chunk.
   * 1 = ReverveNextIdChunk(10); | Reserved ids [1,2,3,4,5,6,7,8,9,10]
   * 11 = ReverveNextIdChunk(10);| Reserved ids [11,12,13,14,15,16,17,18,19,20]
   * b = a + 10;
   */
  virtual vtkTypeUInt32 GetNextChunkGlobalUniqueIdentifier(vtkTypeUInt32 chunkSize) VTK_OVERRIDE;

  void OnServerNotificationMessageRMI(void* message, int message_length);

protected:
  vtkSMSessionClient();
  ~vtkSMSessionClient();

  void SetRenderServerController(vtkMultiProcessController*);
  void SetDataServerController(vtkMultiProcessController*);

  void SetupDataServerRenderServerConnection();

  /**
   * Delete server side object. (SIObject)
   */
  virtual void UnRegisterSIObject(vtkSMMessage* msg) VTK_OVERRIDE;

  /**
   * Notify server side object that it is used by one more client. (SIObject)
   */
  virtual void RegisterSIObject(vtkSMMessage* msg) VTK_OVERRIDE;

  /**
   * Translates the location to a real location based on whether a separate
   * render-server exists.
   */
  vtkTypeUInt32 GetRealLocation(vtkTypeUInt32);

  // Both maybe the same when connected to pvserver.
  vtkMultiProcessController* RenderServerController;
  vtkMultiProcessController* DataServerController;

  vtkPVServerInformation* DataServerInformation;
  vtkPVServerInformation* RenderServerInformation;
  vtkPVServerInformation* ServerInformation;
  vtkClientServerStream* ServerLastInvokeResult;

  vtkSetStringMacro(URI);

  bool AbortConnect;
  char* URI;

  // This flag allow us to disable remote Object deletion in a collaboration
  // context when a client is leaving a visalization session.
  // Typically we don't want this client to broadcast to the other to delete all
  // the proxy because it does not need them anymore as it is leaving...
  bool NoMoreDelete;

  // Field used to communicate with other clients
  vtkSMCollaborationManager* CollaborationCommunicator;

  /**
   * Callback when any vtkMultiProcessController subclass fires a WrongTagEvent.
   * Return true if the event was handle locally.
   */
  virtual bool OnWrongTagEvent(
    vtkObject* caller, unsigned long eventid, void* calldata) VTK_OVERRIDE;

  /**
   * Callback when any vtkMultiProcessController subclass fires a ErrorEvent.
   */
  virtual void OnConnectionLost(vtkObject* caller, unsigned long eventid, void* calldata);

private:
  vtkSMSessionClient(const vtkSMSessionClient&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMSessionClient&) VTK_DELETE_FUNCTION;

  int NotBusy;
  vtkTypeUInt32 LastGlobalID;
  vtkTypeUInt32 LastGlobalIDAvailable;
};

#endif
