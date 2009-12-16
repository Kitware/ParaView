/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessModule
// .SECTION Description
// This is the class that encapsulates all the process initialization.
// The processes module creates a vtkProcessModuleConnectionManager which
// is used to setup connections to self, and remote machines (either client or 
// servers). The Self connection manages MPI process communication, if any.
// Each connection is assigned a Unique ID. SendStream etc are expected to
// provide the ID of the connection on which the stream is to be sent. 

#ifndef __vtkProcessModule_h
#define __vtkProcessModule_h

#include "vtkObject.h"
#include "vtkClientServerID.h" // needed for UniqueID.

class vtkCacheSizeKeeper;
class vtkCallbackCommand;
class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkDataObject;
class vtkKWProcessStatistics;
class vtkMultiProcessController;
class vtkProcessModuleCleanup;
class vtkProcessModuleConnection;
class vtkProcessModuleConnectionManager;
class vtkProcessModuleGUIHelper;
class vtkProcessModuleInternals;
class vtkProcessModuleObserver;
class vtkPVInformation;
class vtkPVOptions;
class vtkPVProgressHandler;
class vtkPVServerInformation;
class vtkPVXMLElement;
class vtkRemoteConnection;
class vtkSocketController;
class vtkStringList;
class vtkTimerLog;
class vtkSocket;

class VTK_EXPORT vtkProcessModule : public vtkObject
{
public:
  static vtkProcessModule* New();
  vtkTypeRevisionMacro(vtkProcessModule, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description: 
  // These flags are used to specify destination servers for the
  // SendStream function.
  enum ServerFlags
    {
    DATA_SERVER = 0x01,
    DATA_SERVER_ROOT = 0x02,
    RENDER_SERVER = 0x04,
    RENDER_SERVER_ROOT = 0x08,
    SERVERS = DATA_SERVER | RENDER_SERVER,
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
    };

  enum ProgressEventEnum
    {
    PROGRESS_EVENT_TAG = 31415
    };

  enum ExceptionEventEnum
    {
    EXCEPTION_EVENT_TAG = 31416,
    EXCEPTION_BAD_ALLOC = 31417,
    EXCEPTION_UNKNOWN   = 31418
    };

  static inline int GetRootId(int serverId)
    {
    if (serverId & CLIENT)
      {
      return CLIENT;
      }
    
    if (serverId == DATA_SERVER_ROOT || serverId == RENDER_SERVER_ROOT)
      {
      return serverId;
      }
    
    if (serverId == (DATA_SERVER | RENDER_SERVER) )
      {
      return DATA_SERVER_ROOT;
      }
    return serverId << 1;
    }
//ETX

  // Description:
  // Get a directory listing for the given directory.  Returns 1 for
  // success, and 0 for failure (when the directory does not exist).
  virtual int GetDirectoryListing(vtkIdType connectionID, const char* dir, 
    vtkStringList* dirs, vtkStringList* files, int save);
 
//BTX
  // Description:
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise.  connectionID identifies the connection. 
  // serverFlags can be used to indicate if this is a data server module
  // or a render server module. The directory argument may be used 
  // to specify a directory in which to look for the module. 
  virtual int LoadModule(vtkIdType connectionID,
    vtkTypeUInt32 serverFlags, const char* name, const char* directory);
//ETX

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  virtual int LoadModuleInternal(const char* name, const char* directory);

  // Description:
  // Returns a data object of the given type. This is a utility
  // method used to increase performance. The first time the
  // data object of a given type is requested, it is instantiated
  // and put in map. The following calls do not cause instantiation.
  // Used while comparing data types for input matching.
  vtkDataObject* GetDataObjectOfType(const char* classname);

//BTX
  // Description:
  // This is the method to be called to gather information.
  // This method require the ID of the connection from which
  // this information must be collected.
  virtual void GatherInformation(vtkIdType connectionID,
    vtkTypeUInt32 serverFlags, vtkPVInformation* info, vtkClientServerID id);
//ETX
  virtual void GatherInformation(vtkIdType connectionID,
    vtkTypeUInt32 serverFlags, vtkPVInformation* info, int id)
  {
    this->GatherInformation(connectionID, serverFlags, info, 
                            vtkClientServerID(id));
  }
  
  // Description:
  // Start the process modules. It will create the application
  // and start the event loop. Returns 0 on success.
  virtual int Start(int argc, char** argv);

  // Description:
  // Return the vtk object associated with the given id for the
  // client.  If the id is for an object on another node then 0 is
  // returned.
  virtual vtkObjectBase* GetObjectFromID(int id)
  {
    return this->GetObjectFromID(vtkClientServerID(id));
  }

//BTX
  // Description:
  // These methods append commands to the given vtkClientServerStream
  // to construct or delete a vtk object.  For construction, the type
  // of the object is specified by string name and the new unique
  // object id is returned.  For deletion the object is specified by
  // its id.  These methods do not send the stream anywhere, so the
  // caller must use SendStream() to actually perform the operation.
  vtkClientServerID NewStreamObject(const char*, vtkClientServerStream& stream);
  void DeleteStreamObject(vtkClientServerID, vtkClientServerStream& stream);

  // Description:
  // This method is an overload for the 
  // NewStreamObject(const char*, vtkClientServerStream&). It is similar to the other
  // since is also creates a new vtkobject, however, it does not assign a new
  // object ID for it, instead the id passed as argument is used. 
  // Additionally, it updates the internal ID count, so that next UniqueID 
  // requested from the process module will certainly be greater than the one
  // passed to this method.
  vtkClientServerID NewStreamObject(const char*, vtkClientServerStream& stream,
    vtkClientServerID id);

  
  // Description:
  // Return the vtk object associated with the given id for the
  // client.  If the id is for an object on another node then 0 is
  // returned.
  virtual vtkObjectBase* GetObjectFromID(vtkClientServerID);
  virtual vtkClientServerID GetIDFromObject(vtkObjectBase*);

  // Description:
  // Return the last result for the specified server.  In this case,
  // the server should be exactly one of the ServerFlags, and not a
  // combination of servers.  For an MPI server the result from the
  // root node is returned.  There is no connection to the individual
  // nodes of a server.
  virtual const vtkClientServerStream& GetLastResult(vtkIdType connectionID,
    vtkTypeUInt32 server);

  // Description:
  // Send a vtkClientServerStream to the specified servers.  Servers
  // are specified with a bit vector.  To send to more than one server
  // use the bitwise or operator to combine servers.  The resetStream
  // flag determines if Reset is called to clear the stream after it
  // is sent.
  int SendStream(vtkIdType connectionID, vtkTypeUInt32 server, 
    vtkClientServerStream& stream, int resetStream=1);

  // Description:
  // Get the interpreter used on the local process.
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);

  // Description:
  // Initialize/Finalize the process module's
  // vtkClientServerInterpreter.
  virtual void InitializeInterpreter();
  virtual void FinalizeInterpreter();
//ETX

  // Description:
  // Initialize and finalize process module.
  void Initialize();
  void Finalize();
  
  // Description:
  // Set/Get whether to report errors from the Interpreter.
  vtkGetMacro(ReportInterpreterErrors, int);
  vtkSetMacro(ReportInterpreterErrors, int);
  vtkBooleanMacro(ReportInterpreterErrors, int);

  // Description:
  // This is the controller used to communicate with the MPI nodes
  // by the SelfConnection. 
  vtkMultiProcessController* GetController();

  // Description:
  // Get the partition piece.  -1 means no assigned piece.
  virtual int GetPartitionId();

  // Description:
  // Returns the number of partitions for the local processes. On client
  // it returns the number of client process partitions (which is always 1),
  // while on any server node, it returns the number of server partitions.
  virtual int GetNumberOfLocalPartitions();
  
  // Description:
  // Get the number of processes participating in sharing the data for
  // the given connection id. This is only called 
  // on the client to get the number of partitions on of server
  // the client is connected to.
  // On server side, the connection id is ignored
  // (since server has no connections) and simply returns the
  // number of local partitions (same as GetNumberOfLocalPartitions()).
  virtual int GetNumberOfPartitions(vtkIdType id);

//BTX
  // Description:
  // Get a unique vtkClientServerID for this process module.
  vtkClientServerID GetUniqueID();

  // Description:
  // This method updates the internal datastructure that allocates unique ID to
  // ensure that the \c id does not get assigned.
  void ReserveID(vtkClientServerID id);

  // Description:
  // Get the vtkClientServerID used for the ProcessModule.
  vtkClientServerID GetProcessModuleID();
//ETX
  int GetProcessModuleIDAsInt()
  {
  vtkClientServerID id = this->GetProcessModuleID();
  return id.ID;
  }

  // Description:
  // Get/Set the global process module.
  static vtkProcessModule* GetProcessModule();
  static void SetProcessModule(vtkProcessModule* pm);

  // Description:
  // Register object with progress handler.
  void RegisterProgressEvent(vtkObject* po, int id);
 
  // Description:
  // Internal method- called when an exception Tag is received 
  // from the server.
  void ExceptionEvent(const char* message);
  // Description:
  // Internal method - called on the server only. This method
  // should report to all client why the server is exiting.
  void ExceptionEvent(int type);

  //BTX
  // Description:
  virtual void SendPrepareProgress(vtkIdType connectionID,
    vtkTypeUInt32 servers=CLIENT|DATA_SERVER);
  virtual void SendCleanupPendingProgress(vtkIdType connectionID);
  vtkPVProgressHandler* GetActiveProgressHandler();
  //ETX

  // Description:
  // Internal methods. Do not call directly.
  void PrepareProgress();
  void CleanupPendingProgress();

  // Description:
  // Set the local progress. This simply forwards the call to GUIHelper,
  // if any.
  void SetLocalProgress(const char* filter, int progress);
  vtkGetMacro(ProgressRequests, int);
  vtkSetMacro(ProgressRequests, int);

  // Description:
  // Set and get the application options
  vtkGetObjectMacro(Options, vtkPVOptions);
  virtual void SetOptions(vtkPVOptions* op);

  // Description:
  // Set the gui helper
  void SetGUIHelper(vtkProcessModuleGUIHelper*);
  vtkGetObjectMacro(GUIHelper, vtkProcessModuleGUIHelper);

 // Description:
  // Get a pointer to the log file.
  ofstream* GetLogFile();
  virtual void CreateLogFile();

  // Description:
  // For logging start and end execute events.
  void LogStartEvent(const char* str);
  void LogEndEvent(const char* str);

  // Description:
  // More timer log access methods. 
  void SetLogBufferLength(int length);
  void SetLogBufferLength(vtkIdType connectionID,
                          vtkTypeUInt32 servers,
                          int length);
  void ResetLog();
  void ResetLog(vtkIdType connectionID, vtkTypeUInt32 servers);
  void SetEnableLog(int flag);
  void SetEnableLog(vtkIdType connectionID, vtkTypeUInt32 servers,
                    int flag);

  // Description:
  // Time threshold for event (start-end) when getting the log with indents.
  vtkSetMacro(LogThreshold, double);
  void SetLogThreshold(vtkIdType connectionID, vtkTypeUInt32 servers,
                       double threshold);
  vtkGetMacro(LogThreshold, double);

  // Description:
  // We need to get the data path for the demo on the server.
  const char* GetPath(const char* tag, const char* relativePath, const char* file);
  
  // Description:  
  // This method leaks memory.  It is a quick and dirty way to set different 
  // DISPLAY environment variables on the render server.  I think the string 
  // cannot be deleted until paraview exits.  The var should have the form:
  // "DISPLAY=amber1"
  virtual void SetProcessEnvironmentVariable(int processId, const char* var);

  // Description:
  // Propagate from the options so that it is available in CS
  int GetRenderNodePort();
  char* GetMachinesFileName();
  int GetClientMode();
  unsigned int GetNumberOfMachines();
  const char* GetMachineName(unsigned int idx);

  // Description:
  // Returns the information about command line arguments on the server for the
  // given connection.  This returns 0 if the connection is not a remote 
  // connection.
  vtkPVServerInformation* GetServerInformation(vtkIdType id);

//BTX
  // Description:
  // Get the ID used for MPIMToNSocketConnection for the given connection.
  vtkClientServerID GetMPIMToNSocketConnectionID(vtkIdType id);
  
//ETX


  // Description:
  // Given a vtkProcessModuleConnection, this method returns the connection
  // ID for it.
  vtkIdType GetConnectionID(vtkProcessModuleConnection* connection);

  // Description:
  // Get the active remote connection. The notion of active conntion
  // is here only for the sake of ProgressHandler.
  vtkGetObjectMacro(ActiveRemoteConnection, vtkRemoteConnection);

  // Description:
  // Get the socket controller associated with the ActiveRemoteConnection;
  // ActiveRemoteConnection is the connection which is processing the current
  // stream.
  vtkSocketController* GetActiveSocketController();

  // Description:
  // Get the render-server socket controlled for the ActiveRemoteConnection,
  // if any. 
  // ActiveRemoteConnection is the connection which is processing the current
  // stream.
  vtkSocketController* GetActiveRenderServerSocketController();

  // Description:
  // For the given connection, returns 1 if the connection is
  // a remote server connection with separate socket connections
  // for data server and render server.
  int GetRenderClientMode(vtkIdType cid);

  // Description:
  // Get and Set the application installation directory
//BTX
  enum CommunicationIds
  {
    MultiDisplayDummy=948346,
    MultiDisplayRootRender,
    MultiDisplaySatelliteRender,
    MultiDisplayInfo,
    PickBestProc,
    PickBestDist2,
    IceTWinInfo,
    IceTNumTilesX,
    IceTNumTilesY,
    IceTTileRanks,
    IceTRenInfo,
    GlyphNPointsGather,
    GlyphNPointsScatter,
    TreeCompositeDataFlag,
    TreeCompositeStatus,
    DuplicatePDNProcs,
    DuplicatePDNRecLen,
    DuplicatePDNAllBuffers,
    IntegrateAttrInfo,
    IntegrateAttrData,
    PickMakeGIDs,
    TemporalPickHasData,
    TemporalPicksData
  };

//ETX
  
  // Description:
  // When this flag is set, it implies that the server (or client) can
  // accept multiple remote connections. This class only affects when
  // running in client-server mode.
  vtkSetMacro(SupportMultipleConnections, int);
  vtkGetMacro(SupportMultipleConnections, int);
  vtkBooleanMacro(SupportMultipleConnections, int);
  
  // Description:
  // Connect to remote process. Returns the connection Id for the newly
  // created connection, on failure, NullConnectionID is returned.  This
  // method is intended to be used when SupportMultipleConnections is true,
  // to connect to a server, in forward connection mode. When
  // SupportMultipleConnections is 1, the client does not connect to a
  // server in the call to Start(), the GUI is expected to explicitly call
  // this method to connect to the server. Also, it is the responsibility
  // of the GUI to call the appropriate overloaded method when running in
  // render client mode.  TODO: this method can work even on the server --
  // however, currently, the server connects to a remote client only in
  // reverse connection mode, in which case the server can connect to 1 and
  // only 1 client, and that connection is established in Start()
  // itselt. Hence this method has not useful.
  vtkIdType ConnectToRemote(const char* serverhost, int port);
  vtkIdType ConnectToRemote(const char* dataserver_host, int dataserver_port,
    const char* renderserver_host, int renderserver_port);

  // Description:
  // Setups up a server socket on the given port. On success, returns a non-zero
  // identifier for the server socket which can be used to close the socket
  // using StopAcceptingConnections(). 
  int AcceptConnectionsOnPort(int port);

  // Description:
  // Setups up a server socket on the given port. On success, 
  // ds_id and rs_id are set to non-zero identifiers for the data and render
  // server sockets, respectively.
  void AcceptConnectionsOnPort(int data_server_port, int render_server_port,
    int &ds_id, int &rs_id);

  // Description:
  // Closes a server socket with the given id. The id is the same id returned 
  // by AcceptConnectionsOnPort().
  void StopAcceptingConnections(int id);
  void StopAcceptingAllConnections();

  // Description:
  // Returns if the process module is correctly accepting connections on any
  // port. This typically means its waiting for some remote process to connect
  // to it.
  bool IsAcceptingConnections();

  // Description:
  // This creates a new SelfConnection. This is experimental feature.
  vtkIdType ConnectToSelf();

  // Description:
  // Returns the number of connections (including the SelfConnection).
  int GetNumberOfConnections();
  
  // Description:
  // Close the connection. The connection must be a remote connection.
  void Disconnect(vtkIdType id);
 
  // Description:
  // Returns 1 is the connection is a connection with a remote server (or client).
  int IsRemote(vtkIdType id);

  // Description:
  // Checks if any new connections are available, if so, creates
  // vtkConnections for them. The call will wait for a timeout of msec
  // milliseconds for a new connection to arrive. Timeout of 0 will wait
  // indefinitely until a new connection arrives.  This method is intended
  // for use by the client when running in reverse connection mode with
  // SupportMultipleConnections set to 1. When SupportMultipleConnections
  // is set to 1 Start() does not wait for server to attempt to connect to
  // the client.  The gui must explicitly call this method to check if any
  // new connections are pending.  Return value: -1 on error, 0 on timeout,
  // 1 if a new connection has been established.
  vtkIdType MonitorConnections(unsigned long msec);
  
  // Description:
  // Clear this flag to override using of MPI for self connection.  This
  // flag should be changed, if at all, only before calling
  // vtkProcessModule::Initialize() after which it has no effect. By
  // default, this flag is set, i.e. MPI is used for self connection if
  // available i.e.  VTK is built with MPI. Typically, one does not need to
  // change this flag manually, vtkPVMain sets this flag depending upon
  // whether MPI was initialized or not.
  vtkSetMacro(UseMPI, int);
  vtkGetMacro(UseMPI, int);

  // Description:
  // Push an undo set xml state on the undo stack for the given connection.
  void PushUndo(vtkIdType id, const char* label, vtkPVXMLElement* root);

  // Description:
  // Get the next undo  xml from the connection.
  // This method allocates  a new vtkPVXMLElement. It is the responsibility 
  // of caller to \c Delete it. 
  // \returns NULL on failure, otherwise the XML element is returned.
  vtkPVXMLElement* NewNextUndo(vtkIdType id);
 
  // Description:
  // Get the next redo  xml from the connection.
  // This method allocates  a new vtkPVXMLElement. It is the responsibility 
  // of caller to \c Delete it. 
  // \returns NULL on failure, otherwise the XML element is returned.
  vtkPVXMLElement* NewNextRedo(vtkIdType id);

  // Description:
  // This flag is used to shut the SendStream from sending any thing to
  // the servers. This is useful for reiviving a server manager.
  vtkSetMacro(SendStreamToClientOnly, int);
  vtkGetMacro(SendStreamToClientOnly, int);
  vtkBooleanMacro(SendStreamToClientOnly, int);

  // Description:
  // Get/Set the vtkCacheSizeKeeper objects that can
  // be used to keep track of the cache size for this process.
  // To gather information about caches on all processes,
  // use vtkPVCacheSizeInformation.
  vtkGetObjectMacro(CacheSizeKeeper, vtkCacheSizeKeeper);

  // Description:
  // Returns the value (0-100) of the last progress that was
  // reported.
  int GetLastProgress() { return this->LastProgress; }

  // Description:
  // Returns the name of the algorithm the last progress was reported for.
  vtkGetStringMacro(LastProgressName);

//BTX
  // Description:
  // Initialize the log file.
  static void InitializeDebugLog(ostream& ref);

  // Description:
  // Used to dump debug messages.
  static void DebugLog(const char*);

  // Description:
  // Finalize debug log.
  static void FinalizeDebugLog();

  typedef void (*InterpreterInitializationCallback)(vtkClientServerInterpreter*);

  // Description:
  // Use this method to initialize the interpreter. This register the callback
  // if the process module hasn't been created yet, so that the callbacks get
  // invoked once the process module is created. This is generally necessary for
  // plugins.
  static void InitializeInterpreter(InterpreterInitializationCallback callback);

  // Description:
  // Add a socket to be managed. conn is the connection is any, 
  // associated with the socket.
  // INTENDED FOR SPECIALIST USE ONLY. IF YOU ARE WONDERING WHAT DOES METHOD
  // DOES THEN YOU SHOULDN'T BE USING THIS METHOD.
  void AddManagedSocket(vtkSocket* soc, vtkProcessModuleConnection* conn);

  // Description:
  // Remove a socket from being managed.
  // INTENDED FOR SPECIALIST USE ONLY. IF YOU ARE WONDERING WHAT DOES METHOD
  // DOES THEN YOU SHOULDN'T BE USING THIS METHOD.
  void RemoveManagedSocket(vtkSocket* soc);

protected:
  vtkProcessModule();
  ~vtkProcessModule();

  static vtkProcessModule* ProcessModule;

  static void InterpreterCallbackFunction(vtkObject* caller,
    unsigned long eid, void* cd, void* d);
  virtual void InterpreterCallback(unsigned long eid, void*);

  // Description:
  // Detemines the log file prefix depending upon node type.
  virtual const char* DetermineLogFilePrefix();


  // Description:
  // Execute event on callback.
  void ExecuteEvent(vtkObject* o, unsigned long event, void* calldata);

  // Description:
  // Starts the client. It will create the application
  // and start the event loop. Returns 0 on success. This method is called
  // only on the client process with the GUI.
  virtual int StartClient(int argc, char** argv);

  // Description:
  // Starts the server node event loop. Return 0 on success. This method is 
  // called only on the server root node.
  // msec is the inactivity timeout. If no socket activity happens on the server
  // for msec milliseconds, the server exists. Timeout of 0 implies no timeout, the
  // server waits indefinitely.
  virtual int StartServer(unsigned long msec);

  // Description:
  // Called to set up the connections, if any. Thus for processes
  // that have server sockets, this call creates the sockets and binds them to
  // appropriate ports. Note this this call does not create any remote connections;
  // it simply prepares the state to accept connections.
  // Returns 0 on error, 1 on success.
  int InitializeConnections();

  // Description:
  // Connect to a remote server or client already waiting for us.
  // Returns 0 on error, 1 on success.
  int ConnectToRemote();

  // Description:
  // Setup a wait connection that is waiting for a remote process to
  // connect to it.  This can be either the client or the server.
  // Returns 0 on error, 1 on success.
  int SetupWaitForConnection();

  // Description:
  // Return 1 if the connection should wait, and 0 otherwise.
  int ShouldWaitForConnection();
 
  // Description:
  // Called on client in reverse connection mode. Returns after the
  // client has connected to a RenderServer/DataServer (in case of 
  // client-render-server) or Server.
  int ClientWaitForConnection();
  
  // Description:
  // ProcessModule runs on a SingleThread. Hence, we have the notion 
  // of which connection has some activity at a given time. This "active"
  // connection is identified by ActiveRemoteConnection. This only applies 
  // to remote connections. A remote connection is active
  // when it is processing some stream on the SelfConnection.
  // It is the responsibility of vtkRemoteConnection (and subclasses)
  // to set this pointer appropriately. Note that this is reference counted.
  vtkRemoteConnection* ActiveRemoteConnection;
  void SetActiveRemoteConnection(vtkRemoteConnection*);
  // so that this class can access SetActiveConnection.
  friend class vtkRemoteConnection;
  
  vtkClientServerID UniqueID;
  
  vtkClientServerInterpreter* Interpreter;
  vtkCallbackCommand* InterpreterObserver;
  int ReportInterpreterErrors;
  friend class vtkProcessModuleObserver;

  vtkProcessModuleInternals* Internals;
  vtkProcessModuleObserver* Observer;
  vtkProcessModuleConnectionManager* ConnectionManager;

  int ProgressRequests;

  vtkPVOptions* Options;
  vtkProcessModuleGUIHelper* GUIHelper;

  // Description:
  // This is an empty server information object used when 
  // no actual server exists.
  vtkPVServerInformation* ServerInformation;
  double LogThreshold;
  ofstream *LogFile;
  vtkTimerLog* Timer;
  vtkKWProcessStatistics* MemoryInformation;

  vtkSetStringMacro(LastProgressName);
  char* LastProgressName;
  int LastProgress;

  // Description:
  // When this flag is set, it implies that the server (or client)
  // can accept multiple remote connections. This class only affects when running
  // in client-server mode.
  int SupportMultipleConnections;

  // Description:
  // When SupportMultipleConnections is true, this flag is set to true after the
  // connection has been established during the process module initialization.
  // That way, any further requests to make new connections will raise errors
  // and will be discarded.
  bool DisableNewConnections;

  // This flag is set when ExceptionEvent method is called.
  // On the server, connections are no longer monitored once
  // ExceptionRaised is set.
  int ExceptionRaised;

  // This flag is used to shut the SendStream from sending any thing to
  // the servers. This is useful for reiviving a server manager.
  int SendStreamToClientOnly;

  int UseMPI;

  vtkCacheSizeKeeper* CacheSizeKeeper;
private:
  vtkProcessModule(const vtkProcessModule&); // Not implemented.
  void operator=(const vtkProcessModule&); // Not implemented.

  vtkIdType LastConnectionID;
  static ostream* DebugLogStream;

  friend class vtkProcessModuleCleanup;
  class vtkInterpreterInitializationCallbackVector;
  static vtkInterpreterInitializationCallbackVector* InitializationCallbacks;
//ETX
};


#endif

