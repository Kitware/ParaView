/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleConnectionManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessModuleConnectionManager.h"

#include "vtkClientConnection.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkClientSocket.h"
#include "vtkConnectionIterator.h"
#include "vtkMPISelfConnection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnection.h"
#include "vtkProcessModuleConnectionManagerInternals.h"
#include "vtkPVOptions.h"
#include "vtkPVServerSocket.h"
#include "vtkSelfConnection.h"
#include "vtkServerConnection.h"
#include "vtkServerSocket.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCollection.h"
#include "vtkSocketController.h"


#include <vtkstd/map>
#include <vtkstd/deque>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProcessModuleConnectionManager);
vtkCxxRevisionMacro(vtkProcessModuleConnectionManager, "1.5");

//-----------------------------------------------------------------------------
vtkProcessModuleConnectionManager::vtkProcessModuleConnectionManager()
{
  this->Internals = new vtkProcessModuleConnectionManagerInternals;
  this->SocketCollection = vtkSocketCollection::New();
  this->UniqueConnectionID.ID = 5;
  this->UniqueServerSocketID = 0;
  this->ClientMode = 0;
}

//-----------------------------------------------------------------------------
vtkProcessModuleConnectionManager::~vtkProcessModuleConnectionManager()
{
  delete this->Internals;
  this->SocketCollection->Delete();
}

//-----------------------------------------------------------------------------
vtkConnectionIterator* vtkProcessModuleConnectionManager::NewIterator()
{
  vtkConnectionIterator* iter = vtkConnectionIterator::New();
  iter->SetConnectionManager(this);
  return iter;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::Initialize(int argc, char** argv, 
  int clientMode)
{
  this->ClientMode = clientMode;

  // Ensure that sockets are initialized. Now the SocketController never
  // needs to be initialized again.
  vtkSocketController* dummy = vtkSocketController::New();
  dummy->Initialize();
  dummy->Delete();

  // Create and initialize the self connection. This would also initialize
  // the MPIController, if any.
  vtkSelfConnection* sc = 0;
  if (this->ClientMode)
    {
    // No MPI needed in on a pure Client.
    sc = vtkSelfConnection::New();
    }
  else
    {
    sc = vtkMPISelfConnection::New();
    }
  this->SetConnection(vtkProcessModuleConnectionManager::GetSelfConnectionID(),
    sc);
  sc->Delete();

  return sc->Initialize(argc, argv); 
  // sc->Initialize() blocks on Satellite nodes, but never blocks on
  // the Client or ServerRoot.
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::Finalize()
{
  // Close open server sockets.
  this->StopAcceptingAllConnections();

  vtkConnectionIterator* iter = this->NewIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkProcessModuleConnection* conn = iter->GetCurrentConnection();
    conn->Finalize();
    }
  iter->Delete();

}

//-----------------------------------------------------------------------------
vtkProcessModuleConnection* vtkProcessModuleConnectionManager::
GetConnectionFromID(vtkConnectionID connectionID)
{
  vtkConnectionIterator* iter = this->NewIterator();
  iter->SetMatchConnectionID(connectionID);
  iter->Begin();
  if (iter->IsAtEnd())
    {
    vtkErrorMacro("Invalid connection ID: " << connectionID);
    iter->Delete();
    return NULL;
    }
  vtkProcessModuleConnection* conn = iter->GetCurrentConnection();
  iter->Delete();
  return conn;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::AcceptConnectionsOnPort(int port, int type)
{
  vtkPVServerSocket* ss = vtkPVServerSocket::New();
  if (ss->CreateServer(port) != 0)
    {
    vtkErrorMacro("Failed to set up server socket.");
    ss->Delete();
    return -1;
    }
  ss->SetType(type);

  int id = this->UniqueServerSocketID++;
  this->Internals->IntToServerSocketMap[id] = ss;
  ss->Delete();

  // Add the server socket to the list of managed sockets.
  this->AddManagedSocket(ss, 0);
  return id;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::StopAcceptingConnections(int id)
{
  if (id < 0)
    {
    vtkErrorMacro("Invalid ServerSocket id: " << id);
    return;
    }
  vtkProcessModuleConnectionManagerInternals::MapOfIntToPVServerSocket::iterator iter;
  if ( (iter = this->Internals->IntToServerSocketMap.find(id)) == 
    this->Internals->IntToServerSocketMap.end())
    {
    vtkErrorMacro("Invalid ServerSocket id: " << id);
    return;
    }

  vtkServerSocket* ss = iter->second.GetPointer();
  this->RemoveManagedSocket(ss);
  ss->CloseSocket();
  this->Internals->IntToServerSocketMap.erase(iter);
}
//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::StopAcceptingAllConnections()
{
  vtkProcessModuleConnectionManagerInternals::MapOfIntToPVServerSocket::iterator 
    iter = this->Internals->IntToServerSocketMap.begin();
  for (; iter != this->Internals->IntToServerSocketMap.end(); ++iter)
    {
    vtkServerSocket* ss = iter->second.GetPointer();
    this->RemoveManagedSocket(ss);
    ss->CloseSocket();   
    }
  this->Internals->IntToServerSocketMap.clear();
}

//-----------------------------------------------------------------------------
vtkConnectionID vtkProcessModuleConnectionManager::OpenConnection(
  const char* hostname, int port)
{
  vtkConnectionID id;
  id.ID = 0;

  if (!hostname || !port)
    {
    vtkErrorMacro("Invalid host or port number.");
    return id;
    }
 
  // Create client socket.
  // Create a RemoteConnection (Server/Client)
  // Set the client socket on its controller.
  // Manage the client socket.
  vtkClientSocket* cs = vtkClientSocket::New();
  if (cs->ConnectToServer(hostname, port) == -1)
    {
    cs->Delete();
    return id;
    }
  id = this->CreateConnection(cs, 0, 1); 
  cs->Delete();
  return id;
}

//-----------------------------------------------------------------------------
vtkConnectionID vtkProcessModuleConnectionManager::OpenConnection(
  const char* datahostname, int dataport,
  const char* renderhostname, int renderport)
{
  vtkConnectionID id;
  id.ID = 0;

  if (!datahostname || !dataport || !renderhostname || !renderport)
    {
    vtkErrorMacro("Invalid host or port number.");
    return id;
    }
  
  vtkClientSocket* dcs = vtkClientSocket::New();
  if (dcs->ConnectToServer(datahostname, dataport) == -1)
    {
    vtkErrorMacro("Data Server connection failed.");
    dcs->Delete();
    return id;
    }
  
  vtkClientSocket* rcs = vtkClientSocket::New();
  if (rcs->ConnectToServer(renderhostname, renderport) == -1)
    {
    dcs->Delete();
    rcs->Delete();
    vtkErrorMacro("Render Server connection failed.");
    return id;
    }
  id = this->CreateConnection(dcs, rcs, 1);
  dcs->Delete();
  rcs->Delete();
  return id;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::MonitorConnections(
  unsigned long msec/* = 0*/)
{
  // Select sockets.
  // If activity:
  //  If on existing Connection, send it to the connection to process.
  //    if processing failed, we deem connection dead and drop it.
  //  If new connection, create a new connection, authenticate etc etc.
  int ret = this->SocketCollection->SelectSockets(msec);
  if (ret <= 0)
    {
    // Timeout or error.
    return ret;
    }
  
  ret = 0;
  vtkSocket* selectedSocket = this->SocketCollection->GetLastSelectedSocket();
  if (vtkPVServerSocket::SafeDownCast(selectedSocket))
    {
    vtkPVServerSocket* ss = vtkPVServerSocket::SafeDownCast(selectedSocket);
    // A new connection. 
    vtkClientSocket* cc = ss->WaitForConnection(10);
    if (cc)
      {
      // Determine the nature of the server socket.
      vtkClientSocket* rs = 0;
      vtkClientSocket* ds = 0;
      vtkConnectionID id = { 0 };
      switch (ss->GetType())
        {
      case RENDER_SERVER:
        if (this->Internals->DataServerConnections.size() > 0)
          {
          ds = this->Internals->DataServerConnections.front();
          rs = cc;
          id = this->CreateConnection(ds, rs, 0);
          this->Internals->DataServerConnections.pop_front();
          }
        else
          {
          this->Internals->RenderServerConnections.push_back(cc);
          }
        break;
        
      case DATA_SERVER:
        if (this->Internals->RenderServerConnections.size() > 0)
          {
          ds = cc;
          rs = this->Internals->RenderServerConnections.front();
          id = this->CreateConnection(ds, rs, 0);
          this->Internals->RenderServerConnections.pop_front();
          }
        else
          {
          this->Internals->DataServerConnections.push_back(cc);
          }
        break;
        
      case RENDER_AND_DATA_SERVER:
        id = this->CreateConnection(cc, 0, 0);
        break;
        }

      if (id.ID)
        {
        ret = 2; // connection created.
        }
      else
        {
        ret = 1; // processed 1 message alright -- may be just received 1 of the 2 required connections.
        }
      cc->Delete();
      }
    else
      {
      vtkWarningMacro("New connection dropped.");
      }
    }
  else
    {
    // Locate the connection.
    vtkRemoteConnection* rc = vtkRemoteConnection::SafeDownCast(
      this->GetManagedConnection(selectedSocket));
    if (!rc)
      {
      vtkErrorMacro("Failed to find connection! Should not happen.");
      return -1;
      }
    ret = rc->ProcessCommunication();
    if (!ret)
      {
      this->DropConnection(rc);
      return 3;
      }
    }
  
  return ret;  
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::IsServerConnection(
  vtkConnectionID connection)
{
  if (connection == vtkProcessModuleConnectionManager::GetAllConnectionsID() ||
    connection == vtkProcessModuleConnectionManager::GetAllServerConnectionsID() ||
    connection == vtkProcessModuleConnectionManager::GetRootServerConnectionID())
    {
    vtkErrorMacro("Cannot call IsServerConnection with collective connections ID.");
    return 0;
    }
  if (connection != vtkProcessModuleConnectionManager::GetSelfConnectionID())
    {
    // Any remote connection is ofcourse a server connection.
    return 1;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();
  if (options->GetClientMode() || options->GetServerMode() || options->GetRenderServerMode())
    {
    // Only in client/server mode is the self connection not a Server Connection.
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModuleConnectionManager::
GetConnectionClientServerID(vtkConnectionID id)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(id);
  if (!conn)
    {
    vtkClientServerID nullid = {0};
    return nullid;
    }
  return conn->GetSelfID();
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::SendStream(vtkConnectionID connectionID, 
  vtkTypeUInt32 serverFlags, vtkClientServerStream& stream, int reset)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(connectionID);
  if (conn)
    {
    conn->SendStream(serverFlags, stream);
    }

  if(reset)
    {
    stream.Reset();
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::GetNumberOfPartitions(vtkConnectionID id)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(id);
  if (conn)
    {
    return conn->GetNumberOfPartitions();
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::GatherInformation(
  vtkConnectionID connectionID, vtkTypeUInt32 serverFlags,
  vtkPVInformation* info, vtkClientServerID id)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(connectionID);
  if (conn)
    {
    conn->GatherInformation(serverFlags, info, id);
    }
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModuleConnectionManager::GetLastResult(
    vtkConnectionID connectionID, vtkTypeUInt32 server)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(connectionID);
  if (conn)
    {
    return conn->GetLastResult(server);
    }

  static vtkClientServerStream s;
  return s;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::LoadModule(vtkConnectionID connectionID, 
  const char* name, const char* dir)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(connectionID);
  if (conn)
    {
    if (conn->LoadModule(name, dir))
      {
      return 1;
      }
    else
      {
      vtkErrorMacro("Failed to load Module on connection "  << connectionID);
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkPVServerInformation* vtkProcessModuleConnectionManager::GetServerInformation(
  vtkConnectionID id)
{
  vtkServerConnection* conn = vtkServerConnection::SafeDownCast(
    this->GetConnectionFromID(id));
  if (conn)
    {
    return conn->GetServerInformation();
    }
  return 0;
}
//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModuleConnectionManager::GetMPIMToNSocketConnectionID(
  vtkConnectionID id)
{
  vtkServerConnection* conn = vtkServerConnection::SafeDownCast(
    this->GetConnectionFromID(id));
  if (conn)
    {
    return conn->GetMPIMToNSocketConnectionID();
    }
  vtkClientServerID nullid = {0 };
  return nullid;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::AddManagedSocket(vtkSocket* soc,
  vtkProcessModuleConnection* conn )
{
  this->SocketCollection->AddItem(soc);
  if (conn)
    {
    this->Internals->SocketToConnectionMap[soc] = conn;
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::RemoveManagedSocket(vtkSocket* soc)
{
  this->SocketCollection->RemoveItem(soc);
}

//-----------------------------------------------------------------------------
vtkProcessModuleConnection* vtkProcessModuleConnectionManager::GetManagedConnection(
  vtkSocket* soc)
{
  vtkProcessModuleConnectionManagerInternals::MapOfSocketToConnection::iterator iter;
  iter = this->Internals->SocketToConnectionMap.find(soc);
  if (iter == this->Internals->SocketToConnectionMap.end())
    {
    return NULL;
    }
  return iter->second.GetPointer();
}

//-----------------------------------------------------------------------------
vtkConnectionID vtkProcessModuleConnectionManager::GetUniqueConnectionID()
{
  vtkConnectionID id;
  id.ID = this->UniqueConnectionID.ID++;
  return id;
}

//-----------------------------------------------------------------------------
vtkConnectionID vtkProcessModuleConnectionManager::CreateConnection(
  vtkClientSocket* cs, vtkClientSocket* renderserver_socket, 
  int connecting_side_handshake)
{
  vtkConnectionID id;
  id.ID = 0;

  vtkRemoteConnection* rc = this->NewRemoteConnection();
  if (rc)
    {
    if (rc->SetSocket(cs, connecting_side_handshake) == 0)
      {
      rc->Delete();
      vtkErrorMacro("Handshake failed.");
      return id;
      }

    if (renderserver_socket && vtkServerConnection::SafeDownCast(rc))
      {
      vtkServerConnection* sc = vtkServerConnection::SafeDownCast(rc);
      if (sc->SetRenderServerSocket(renderserver_socket, 
          connecting_side_handshake) == 0)
        {
        rc->Delete();
        vtkErrorMacro("RenderServer Handshake failed.");
        return id;
        }
      }

    if (rc->Initialize(0, 0)==0)
      {
      // Handshake and authentication succeded.
      // Connection is set up perfect!
      id = this->GetUniqueConnectionID();
      this->SetConnection(id, rc);
      this->AddManagedSocket(cs, rc);
      }
    rc->Delete();
    } 
  return id;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::SetConnection(vtkConnectionID id,
  vtkProcessModuleConnection* conn)
{
  this->Internals->IDToConnectionMap[id] = conn; 
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::DropConnection(
  vtkProcessModuleConnection* conn)
{
  // Locate socket for this connection and close it.
  vtkProcessModuleConnectionManagerInternals::MapOfSocketToConnection::iterator iter;
  
  for (iter = this->Internals->SocketToConnectionMap.begin();
    iter != this->Internals->SocketToConnectionMap.end(); ++iter)
    {
    if (iter->second.GetPointer() == conn)
      {
      this->RemoveManagedSocket(iter->first);
      this->Internals->SocketToConnectionMap.erase(iter);
      break;
      }
    }

  vtkProcessModuleConnectionManagerInternals::MapOfIDToConnection::iterator iter2;
  for (iter2 = this->Internals->IDToConnectionMap.begin();
    iter2 != this->Internals->IDToConnectionMap.end(); ++iter2)
    {
    if (iter2->second.GetPointer() == conn)
      {
      this->Internals->IDToConnectionMap.erase(iter2);
      break;
      }
    }

  // Now conn is an invalid pointer.
  // When conn got destroyed, it close the socket as well 
  // as the controller was finalized, so all cleaned up.
}

//-----------------------------------------------------------------------------
unsigned int vtkProcessModuleConnectionManager::GetNumberOfConnections()
{
  return this->Internals->IDToConnectionMap.size();
}

//-----------------------------------------------------------------------------
vtkRemoteConnection* vtkProcessModuleConnectionManager::NewRemoteConnection()
{
  if (this->ClientMode)
    {
    return vtkServerConnection::New();
    }
  return vtkClientConnection::New();
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
