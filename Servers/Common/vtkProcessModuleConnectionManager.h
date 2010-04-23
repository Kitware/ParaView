/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleConnectionManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessModuleConnectionManager - manager for connections
// in a ProcessModule.
// .SECTION Description
// This is a manager of the simultaneous connections in ProcessModule.
// The first connection in the Manager is always the \b "Self" connection.
// This is the connection used to communicate with all the MPI processes
// of this node. All other connections (if any), are client-server connections
// using sockets. SendStream() on ConnectionManager ensures that the stream
// is sent to the right connection.
// ConnectionManager also provides mechanism to monitor all the client-server
// connections in the collection for activity simultaneously.
//
// ConnectionManager can be configured to accept connections. In this case,
// it sets up a server socket (vtkServerSocket) at the given port. As and when
// new connections arrive on this socket, a new connection object
// (vtkProcessModuleConnection) is created and validated. Once validated,
// it gets added to the internal collection of connections.
//
// ConnectionManager can be simply told to open a connection with a remote host
// socket. In that case, it set up a socket (vtkClientSocket) and attempts to
// connect the given host, If successfully connected and authenticated, it will
// add the connection to the internal store.
//
// Every connection is assigned an unique connection Id. Special connection Ids
// are available to represent a particular type of connection. eg.
// \li SelfConnectionID - represents the connection between the root node and
//     the satellites.
// \li AllConnectionsID - represents all the connections present including
//     the SelfConnection and any Remote Connections.
// \li RootServerConnectionID - represents the first \b server connection.
//     What connection qualifies a  server depends on the mode of operation.
//     When in ParaView mode (with or without MPI), the SelfConnection is
//     indeed a ServerConnection. When running in client mode (where it
//     connects/accepts connections from remote data (and render) servers),
//     SelfConnection is not a ServerConnection. Only remote connections are
//     ServerConnections in such a case. Providing this ID makes this decision
//     completely transparent to the ServerManager.
// \li AllServerConnectionsID - represents all \b server connections. Refer to
//     description of RootServerConnectionID for details about what qualifies
//     as a \b server connection.

#ifndef __vtkProcessModuleConnectionManager_h
#define __vtkProcessModuleConnectionManager_h


#include "vtkObject.h"
#include "vtkClientServerID.h" // Needed for ClientServerID.

class vtkClientServerStream;
class vtkClientSocket;
class vtkConnectionIterator;
class vtkProcessModuleConnection;
class vtkProcessModuleConnectionManagerObserver;
class vtkProcessModuleConnectionManagerInternals;
class vtkPVInformation;
class vtkPVServerInformation;
class vtkPVXMLElement;
class vtkRemoteConnection;
class vtkSocket;
class vtkSocketCollection;

class VTK_EXPORT vtkProcessModuleConnectionManager : public vtkObject
{
public:
  static vtkProcessModuleConnectionManager* New();
  vtkTypeMacro(vtkProcessModuleConnectionManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Used for invalid/null connections.
  static vtkIdType GetNullConnectionID()
    {
    return static_cast<vtkIdType>(vtkProcessModuleConnectionManager::NullConnectionID);
    }

  // Description:
  // The ID used for SelfConnection.
  static vtkIdType GetSelfConnectionID()
    {
    return static_cast<vtkIdType>(vtkProcessModuleConnectionManager::SelfConnectionID);
    }

  // The ID used for All Connections.
  static vtkIdType GetAllConnectionsID()
    {
    return static_cast<vtkIdType>(vtkProcessModuleConnectionManager::AllConnectionsID);
    }

  // The ID used for Connection to all Servers. What connection qualifies a
  // server depends on the mode of operation. When in ParaView mode (with or
  // without MPI), the SelfConnection is indeed a ServerConnection. When
  // running in client mode (where it connects/accepts connections from remote
  // data (and render) servers), SelfConnection is not a ServerConnection.
  // Only remote connections are ServerConnections in such a case. Providing
  // this ID makes this decision complete transparent to the ServerManager.
  static vtkIdType GetAllServerConnectionsID()
    {
    return static_cast<vtkIdType>(vtkProcessModuleConnectionManager::AllServerConnectionID);
    }

  // This ID represents the first server connection. What connection qualifies a
  // server depends on the mode of operation. When in ParaView mode (with or
  // without MPI), the SelfConnection is indeed a ServerConnection. When
  // running in client mode (where it connects/accepts connections from remote
  // data (and render) servers), SelfConnection is not a ServerConnection.
  // Only remote connections are ServerConnections in such a case. Providing
  // this ID makes this decision completely transparent to the ServerManager.
  static vtkIdType GetRootServerConnectionID()
    {
    return static_cast<vtkIdType>(vtkProcessModuleConnectionManager::RootServerConnectionID);
    }

//ETX
  // Description:
  // Initializes the manager. Among other things, this setsup
  // the first connection i.e. the SelfConnection.
  // Returns 1 on error; 0 on success.
  // clientMode must be set when vtkProcessModuleConnectionManager
  // is being initialized on a client, else must be set to 0.
  // The partitionId (which can be used to determined if we are a server
  // satellite) is returned in the given pointer.
  int Initialize(int argc, char** argv, int clientMode, int *partitionId);

  // Description:
  // Finalizes the manager. This will trigger closing of all the connections,
  // breaking RMI loops etc. After this call, no connections are valid.
  // This also closes the server sockets, if any.
  void Finalize();

//BTX
  // ServerSocket Types.
  enum
    {
    RENDER_SERVER = 0x01,
    DATA_SERVER = 0x02,
    RENDER_AND_DATA_SERVER = RENDER_SERVER | DATA_SERVER
    };

  // Description:
  // This obtains a root connection for the connection.
  static inline vtkIdType GetRootConnection(vtkIdType connection)
    {
    if (connection == vtkProcessModuleConnectionManager::GetAllConnectionsID())
      {
      return vtkProcessModuleConnectionManager::GetSelfConnectionID();
      }
    if (connection == vtkProcessModuleConnectionManager::GetAllServerConnectionsID())
      {
      return vtkProcessModuleConnectionManager::GetRootServerConnectionID();
      }
    return connection;
    }
//ETX
  // Description:
  // Configures a server socket on a given port and adds it to the
  // internal socket store. This call does not wait for any connection.
  // New connection will be processed during MonitorConnections().
  // It is possible to set up more than 1 server socket. On success,
  // returns the ID of the server socket. Note that server socket IDs
  // and ConnectionIDs are different entities. Returns -1 on error.
  int AcceptConnectionsOnPort(int port, int type);

  // Description:
  // Returns if the process module is correctly accepting connections on any
  // port. This typically means its waiting for some remote process to connect
  // to it.
  bool IsAcceptingConnections();

  // Description:
  // Stop accepting connections on a server socket with the given id.
  // This is the id returned on AcceptConnectionsOnPort().
  void StopAcceptingConnections(int id);

  // Description:
  // Closes all sockets accepting connections.
  void StopAcceptingAllConnections();

  // Description:
  // Connects to a remote host. If connection is successful
  // it is added to the store of managed connections. Returns the
  // ConnectionID for the connection if success, -1 otherwise.
  vtkIdType OpenConnection(const char* hostname, int port);

  // Description:
  // This overload allows for opening two separate connections to
  // a data server and a render server.
  vtkIdType OpenConnection(const char* datahostname, int dataport,
  const char* renderhostname, int renderport);

  // Description:
  // This creates a new self connection. This is an experimental
  // feature.
  vtkIdType OpenSelfConnection();

//BTX
  // Description:
  // Close the connection with the given connection Id. Must be
  // a remote connection.
  void CloseConnection(vtkIdType id);
//ETX

  // Description:
  // Monitors all managed connections for activity as well as
  // monitors all ports accepting connections for activity.
  // If a new connection is received, it is authenticated
  // and managed automatically. Any data received on any of
  // connections is processed by those connections.
  // For now, this call returns after timeout expires or
  // after first activity.
  // Returns -1 on error, 0 on timeout, 1 on successfully processing
  // an activity, 2 when a new connection is established,
  // 3 when a connection is dropped/closed.
  int MonitorConnections(unsigned long msec = 0);


  // Description:
  // If a processes uses MonitorConnections(), aborted remote connections
  // are caught and cleaned up. However, the paraview client doesn't use
  // MonitorConnections() since the client is not listening for activity from
  // the server. Hence, if the server connection is aborted due to communication
  // error, altough it's flagged aborted, the connection is not cleanedup.
  // There still exists the vtkProcessModuleConnection object associated with it.
  // Call this method to clean up all such aborted connections.
  // Returns the number of aborted connections that were dropped (or cleaned up).
  int DropAbortedConnections();

  // Description:
  // Get a connection iterator.
//BTX
  vtkConnectionIterator* NewIterator();
//ETX

//BTX
  // Description:
  // Returns the connection with the given ID. If none present returns NULL.
  vtkProcessModuleConnection* GetConnectionFromID(vtkIdType id);

  // Description:
  // Given a vtkProcessModuleConnection, this method returns the connection
  // ID for it.
  vtkIdType GetConnectionID(vtkProcessModuleConnection* connection);

  // Description:
  // Sends streams to the appropriate connection.
  // serverFlags are passed on to the connection. This method also
  // gurantees that the SelfConnection is present in the group of connections
  // indicated by connectionID, the stream will be send to the SelfConnection
  // only after it has been sent to all the other connections.
  // Returns -1 if the connectionID is invalid, 0 otherwise.
  int SendStream(vtkIdType connectionID, vtkTypeUInt32 serverFlags,
    vtkClientServerStream& stream, int reset);

  // Description:
  // Called to gather information.
  void GatherInformation(vtkIdType connectionID,
    vtkTypeUInt32 serverFlags, vtkPVInformation* info, vtkClientServerID id);

  // Description:
  // Return the last result for the specified server.  In this case,
  // the server should be exactly one of the ServerFlags, and not a
  // combination of servers.
  virtual const vtkClientServerStream& GetLastResult(
    vtkIdType connectionID, vtkTypeUInt32 server);

  // Description:
  // Called by ProcessModule to load a module.
  int LoadModule(vtkIdType connectionID, const char* name, const char* dir);

  // Description:
  // Earlier, the ServerInformation was synchronized with the
  // ClientOptions.  This no longer is appropriate. Hence, we provide
  // access to the server information on each connection.
  vtkPVServerInformation* GetServerInformation(vtkIdType id);

  // Description:
  // Get the MPIMToNSocketConnectionID for a given connection.
  vtkClientServerID GetMPIMToNSocketConnectionID(vtkIdType id);

  // Description:
  // Get the number of processes from a given connection.
  // When SelfConnection, it indicates the number of mpi processes
  // of Self. When vtkServerConnection, implies the number of
  // data server processes on the server.
  // vtkClientConnection does not support this call, returns 1.
  int GetNumberOfPartitions(vtkIdType id);

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
  // Get the number of connections open.
  unsigned int GetNumberOfConnections();

  // Description:
  // For the given connection, returns 1 if the connection is
  // a remote server connection with separate socket connections
  // for data server and render server.
  int GetRenderClientMode(vtkIdType cid);

  // Description:
  // Add a socket to be managed. conn is the connection is any,
  // associated with the socket.
  void AddManagedSocket(vtkSocket* soc, vtkProcessModuleConnection* conn);

  // Description:
  // Remove a socket from being managed.
  void RemoveManagedSocket(vtkSocket* soc);

  // Description:
  // Given a socket, this method returns the Connection that is
  // active on that socket.
  vtkProcessModuleConnection* GetManagedConnection(vtkSocket* soc);

//ETX
protected:
  vtkProcessModuleConnectionManager();
  ~vtkProcessModuleConnectionManager();

  // Description:
  // Instantiates a vtkRemoteConnection() subclass.
  // The actual class instantiated depends on this->ClientMode.
  vtkRemoteConnection* NewRemoteConnection();

  // Description:
  // Internal method to create a connection from a client socket
  // and start managing it.
  vtkIdType CreateConnection(vtkClientSocket* cs,
    vtkClientSocket* renderserver_socket);

  // Description:
  // Returns a unique connection ID.
  vtkIdType GetUniqueConnectionID();

  // Description:
  // Set the connection for a particular id.
  void SetConnection(vtkIdType id, vtkProcessModuleConnection* conn);

  // Description:
  // Drops a connection from the Manager.
  void DropConnection(vtkProcessModuleConnection* conn);

  // Description:
  // Determines if this process waits for connections or connects to remote hosts.
  int ShouldWaitForConnection();

  // Description:
  // Connect to a remote server or client already waiting for us.
  void ConnectToRemote();

  // Description:
  // Setup a wait connection that is waiting for a remote process to
  // connect to it.  This can be either the client or the server.
  void SetupWaitForConnection();

  // Description:
  // This method is only of use for the Client ServerManager.
  // Given a connection ID, this method tells if the connection is a Server
  // Connection. When client is running with MPI, SelfConnection qualifies as
  // a server connection. When running in client-server mode, only remote
  // connections qualify as server connections.
  int IsServerConnection(vtkIdType connection);

  // Description:
  // Event callback.
  virtual void ExecuteEvent(vtkObject* caller, unsigned long eventid, void* data);
//BTX
  // A collection of all open sockets.
  vtkSocketCollection* SocketCollection;
  vtkProcessModuleConnectionManagerInternals* Internals;

  vtkIdType UniqueConnectionID;
  int UniqueServerSocketID;
  int ClientMode;

  friend class vtkConnectionIterator;
  friend class vtkProcessModuleConnectionManagerObserver;
  vtkProcessModuleConnectionManagerObserver* Observer;

  enum ConnectionID {
    NullConnectionID = 0,
    SelfConnectionID,
    AllConnectionsID,
    AllServerConnectionID,
    RootServerConnectionID
  };
//ETX
private:
  vtkProcessModuleConnectionManager(const vtkProcessModuleConnectionManager&); // Not implemented.
  void operator=(const vtkProcessModuleConnectionManager&); // Not implemented.


};


#endif

