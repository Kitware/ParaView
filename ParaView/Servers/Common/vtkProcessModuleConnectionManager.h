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
// .NAME vtkProcessModuleConnectionManager
// .SECTION Description
// This is a manager of the simultaneous connections in ProcessModule.
// The first connection in the Manager is always the "Self" connection.
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
// socket. In that case, it set up a socket (vtkClientSocket) and attempts to connect 
// the given host, If successfully connected and authenticated, it will
// add the connection to the internal store.


#ifndef __vtkProcessModuleConnectionManager_h
#define __vtkProcessModuleConnectionManager_h


#include "vtkObject.h"
#include "vtkConnectionID.h" // Needed for SelfConnectionID.
#include "vtkClientServerID.h" // Needed for ClientServerID.

class vtkClientServerStream;
class vtkClientSocket;
class vtkConnectionIterator;
class vtkProcessModuleConnection;
class vtkProcessModuleConnectionManagerInternals;
class vtkPVInformation;
class vtkPVServerInformation;
class vtkRemoteConnection;
class vtkSocket;
class vtkSocketCollection;

class VTK_EXPORT vtkProcessModuleConnectionManager : public vtkObject
{
public:
  static vtkProcessModuleConnectionManager* New();
  vtkTypeRevisionMacro(vtkProcessModuleConnectionManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  // The ID used for SelfConnection.
  static vtkConnectionID GetSelfConnectionID() 
    { 
    vtkConnectionID id;
    id.ID = 0;
    return id; 
    }

  // The ID used for All Connections.
  static vtkConnectionID GetAllConnectionsID() 
    { 
    vtkConnectionID id;
    id.ID = 1;
    return id; 
    }

  // The ID used for Connection to all Servers. What connection qualifies a 
  // server depends on the mode of operation. When in ParaView mode (with or 
  // without MPI), the SelfConnection is indeed a ServerConnection. When 
  // running in client mode (where it connects/accepts connections from remote 
  // data (and render) servers), SelfConnection is not a ServerConnection. 
  // Only remote connections are ServerConnections in such a case. Providing 
  // this ID makes this decision complete transparent to the ServerManager.
  static vtkConnectionID GetAllServerConnectionsID() 
    { 
    vtkConnectionID id;
    id.ID = 2;
    return id; 
    }

  // This ID represents the first server connection. What connection qualifies a 
  // server depends on the mode of operation. When in ParaView mode (with or 
  // without MPI), the SelfConnection is indeed a ServerConnection. When 
  // running in client mode (where it connects/accepts connections from remote 
  // data (and render) servers), SelfConnection is not a ServerConnection. 
  // Only remote connections are ServerConnections in such a case. Providing 
  // this ID makes this decision completely transparent to the ServerManager.
  static vtkConnectionID GetRootServerConnectionID()
    { 
    vtkConnectionID id;
    id.ID = 3;
    return id; 
    }

//ETX  
  // Description:
  // Initializes the manager. Among other things, this setsup
  // the first connection i.e. the SelfConnection.
  // Returns 1 on error; 0 on success.
  // clientMode must be set when vtkProcessModuleConnectionManager 
  // is being initialized on a client, else must be set to 0.
  int Initialize(int argc, char** argv, int clientMode);
 
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
  static inline vtkConnectionID GetRootConnection(vtkConnectionID connection)
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
  vtkConnectionID OpenConnection(const char* hostname, int port);

  // Description:
  // This overload allows for opening two separate connections to
  // a data server and a render server.
  vtkConnectionID OpenConnection(const char* datahostname, int dataport,
  const char* renderhostname, int renderport);

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
  // Get a connection iterator.
  vtkConnectionIterator* NewIterator();

//BTX
  // Description:
  // Returns the connection with the given ID. If none present returns NULL.
  vtkProcessModuleConnection* GetConnectionFromID(vtkConnectionID id);

  // Description:
  // Given a vtkConnectionID, this call returns the ClientServer ID
  // assigned to that connection.
  vtkClientServerID GetConnectionClientServerID(vtkConnectionID);

  // Description:
  // Sends streams to the appropriate connection.
  // serverFlags are passed on to the connection. This method also
  // gurantees that the SelfConnection is present in the group of connections
  // indicated by connectionID, the stream will be send to the SelfConnection
  // only after it has been sent to all the other connections.
  // Returns -1 if the connectionID is invalid, 0 otherwise.
  int SendStream(vtkConnectionID connectionID, vtkTypeUInt32 serverFlags,
    vtkClientServerStream& stream, int reset);
  
  // Description:
  // Called to gather information.
  void GatherInformation(vtkConnectionID connectionID, 
    vtkTypeUInt32 serverFlags, vtkPVInformation* info, vtkClientServerID id);

  // Description:
  // Return the last result for the specified server.  In this case,
  // the server should be exactly one of the ServerFlags, and not a
  // combination of servers.  
  virtual const vtkClientServerStream& GetLastResult(
    vtkConnectionID connectionID, vtkTypeUInt32 server);

  // Description:
  // Called by ProcessModule to load a module.
  int LoadModule(vtkConnectionID connectionID, const char* name, const char* dir);

  // Description:
  // Earlier, the ServerInformation was synchronized with the
  // ClientOptions.  This no longer is appropriate. Hence, we provide
  // access to the server information on each connection.
  vtkPVServerInformation* GetServerInformation(vtkConnectionID id);

  // Description:
  // Get the MPIMToNSocketConnectionID for a given connection.
  vtkClientServerID GetMPIMToNSocketConnectionID(vtkConnectionID id);

  // Description:
  // Get the number of processes from a given connection.
  // When SelfConnection, it indicates the number of mpi processes 
  // of Self. When vtkServerConnection, implies the number of 
  // data server processes on the server.
  // vtkClientConnection does not support this call, returns 1.
  int GetNumberOfPartitions(vtkConnectionID id);
//ETX
protected:
  vtkProcessModuleConnectionManager();
  ~vtkProcessModuleConnectionManager();

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
  
  // Description:
  // Get the number of connections open.
  unsigned int GetNumberOfConnections();

  // Description:
  // Instantiates a vtkRemoteConnection() subclass.
  // The actual class instantiated depends on this->ClientMode.
  vtkRemoteConnection* NewRemoteConnection();


  // Description:
  // Internal method to create a connection from a client socket
  // and start managing it.
  vtkConnectionID CreateConnection(vtkClientSocket* cs, 
    vtkClientSocket* renderserver_socket, int connecting_side_handshake);
  
  // Description:
  // Returns a unique connection ID.
  vtkConnectionID GetUniqueConnectionID();

  // Description:
  // Set the connection for a particular id.
  void SetConnection(vtkConnectionID id, vtkProcessModuleConnection* conn);
  
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
  int IsServerConnection(vtkConnectionID connection);
//BTX
  // A collection of all open sockets.
  vtkSocketCollection* SocketCollection;
  vtkProcessModuleConnectionManagerInternals* Internals;
  
  vtkConnectionID UniqueConnectionID;
  int UniqueServerSocketID;
  int ClientMode;

  friend class vtkConnectionIterator;
//ETX
private:
  vtkProcessModuleConnectionManager(const vtkProcessModuleConnectionManager&); // Not implemented.
  void operator=(const vtkProcessModuleConnectionManager&); // Not implemented.
};


#endif

