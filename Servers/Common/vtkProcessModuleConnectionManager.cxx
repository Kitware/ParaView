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
#include "vtkCommand.h"
#include "vtkConnectionIterator.h"
#include "vtkMPISelfConnection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnection.h"
#include "vtkProcessModuleConnectionManagerInternals.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVServerSocket.h"
#include "vtkSelfConnection.h"
#include "vtkServerConnection.h"
#include "vtkServerSocket.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCollection.h"
#include "vtkSocketController.h"
#include "vtkSynchronousMPISelfConnection.h"
#include "vtkTimerLog.h"

#include "vtksys/SystemTools.hxx"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/map>
#include <vtkstd/deque>

class vtkProcessModuleConnectionManagerObserver : public vtkCommand
{
public:
  static vtkProcessModuleConnectionManagerObserver* New()
    {
    return new vtkProcessModuleConnectionManagerObserver;
    }

  void SetTarget(vtkProcessModuleConnectionManager* t)
    {
    this->Target = t;
    }
  virtual void Execute(vtkObject* caller, unsigned long eventid,
    void* calldata)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventid, calldata);
      }
    }
protected:
  vtkProcessModuleConnectionManagerObserver()
    {
    this->Target = 0;
    }
  ~vtkProcessModuleConnectionManagerObserver()
    {
    this->Target = 0;
    }
  vtkProcessModuleConnectionManager* Target;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProcessModuleConnectionManager);

//-----------------------------------------------------------------------------
vtkProcessModuleConnectionManager::vtkProcessModuleConnectionManager()
{
  this->Internals = new vtkProcessModuleConnectionManagerInternals;
  this->Observer = vtkProcessModuleConnectionManagerObserver::New();
  this->Observer->SetTarget(this);
  
  this->SocketCollection = vtkSocketCollection::New();
  this->UniqueConnectionID = 5;
  this->UniqueServerSocketID = 0;
  this->ClientMode = 0;
}

//-----------------------------------------------------------------------------
vtkProcessModuleConnectionManager::~vtkProcessModuleConnectionManager()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();
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
  int clientMode, int *partitionId)
{
  this->ClientMode = clientMode;

  // Ensure that sockets are initialized. Now the SocketController never
  // needs to be initialized again.
  vtkSocketController* dummy = vtkSocketController::New();
  dummy->Initialize();
  dummy->Delete();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  // Create and initialize the self connection. This would also initialize
  // the MPIController, if any.
  vtkSelfConnection* sc = pm->GetOptions()->NewSelfConnection();
  if (!sc)
    {
    if (this->ClientMode || !vtkProcessModule::GetProcessModule()->GetUseMPI())
      {
      // No MPI needed in on a pure Client.
      sc = vtkSelfConnection::New();
      }
    else if (vtkProcessModule::GetProcessModule()->GetOptions()->
      GetSymmetricMPIMode())
      {
      sc = vtkSynchronousMPISelfConnection::New();
      }
    else
      {
      sc = vtkMPISelfConnection::New();
      }
    }
  this->SetConnection(vtkProcessModuleConnectionManager::GetSelfConnectionID(),
    sc);
  sc->Delete();

  return sc->Initialize(argc, argv, partitionId); 
  // sc->Initialize() blocks on Satellite nodes, but never blocks on
  // the Client or ServerRoot.
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::Finalize()
{
  // Close open server sockets.
  this->StopAcceptingAllConnections();

  while (!this->Internals->IDToConnectionMap.empty())
    {
    vtkProcessModuleConnection* conn =
      this->Internals->IDToConnectionMap.begin()->second;
    conn->Finalize();
    this->DropConnection(conn);
    }
}

//-----------------------------------------------------------------------------
vtkProcessModuleConnection* vtkProcessModuleConnectionManager::
GetConnectionFromID(vtkIdType connectionID)
{
  vtkConnectionIterator* iter = this->NewIterator();
  iter->SetMatchConnectionID(connectionID);
  iter->Begin();
  if (iter->IsAtEnd())
    {
    if (connectionID != GetNullConnectionID())
      {
      vtkErrorMacro("Invalid connection ID: " << connectionID);
      }
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

  int id = ++this->UniqueServerSocketID;
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
bool vtkProcessModuleConnectionManager::IsAcceptingConnections()
{
  return (this->Internals->IntToServerSocketMap.size() > 0);
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModuleConnectionManager::OpenSelfConnection()
{
  vtkIdType cid = this->GetUniqueConnectionID();
  vtkSelfConnection* selfConnection = vtkSelfConnection::New();
  this->SetConnection(cid, selfConnection);
  int partitionId;
  selfConnection->Initialize(0, NULL, &partitionId);
  selfConnection->Delete();
  this->InvokeEvent(vtkCommand::ConnectionCreatedEvent, &cid);
  return cid;
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModuleConnectionManager::OpenConnection(
  const char* hostname, int port)
{
  vtkIdType id = vtkProcessModuleConnectionManager::NullConnectionID;

  if (!hostname || !port)
    {
    vtkErrorMacro("Invalid host or port number.");
    return id;
    }
 
  // Create client socket.
  // Create a RemoteConnection (Server/Client)
  // Set the client socket on its controller.
  // Manage the client socket.
  VTK_CREATE(vtkClientSocket, cs);
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  while (1)
    {
    if (cs->ConnectToServer(hostname, port) != -1)
      {
      id = this->CreateConnection(cs, 0);
      break;
      }
    timer->StopTimer();
    if (timer->GetElapsedTime() > 60.0)
      {
      vtkErrorMacro(<< "Connect timeout.");
      break;
      }
    vtkWarningMacro(<< "Connect failed.  Retrying for "
                    << (60.0 - timer->GetElapsedTime()) << " more seconds.");
    vtksys::SystemTools::Delay(1000);
    }
  return id;
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModuleConnectionManager::OpenConnection(
  const char* datahostname, int dataport,
  const char* renderhostname, int renderport)
{
  vtkIdType id = vtkProcessModuleConnectionManager::NullConnectionID;

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
  id = this->CreateConnection(dcs, rcs);
  dcs->Delete();
  rcs->Delete();
  return id;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::MonitorConnections(
  unsigned long msec/* = 0*/)
{
  if (this->SocketCollection->GetNumberOfItems() == 0)
    {
    // No server sockets to monitor.
    // Return with an error indication.
    return -1;
    }
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
      vtkIdType id = vtkProcessModuleConnectionManager::NullConnectionID;
      switch (ss->GetType())
        {
      case RENDER_SERVER:
        if (this->Internals->DataServerConnections.size() > 0)
          {
          ds = this->Internals->DataServerConnections.front();
          rs = cc;
          id = this->CreateConnection(ds, rs);
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
          id = this->CreateConnection(ds, rs);
          this->Internals->RenderServerConnections.pop_front();
          }
        else
          {
          this->Internals->DataServerConnections.push_back(cc);
          }
        break;
        
      case RENDER_AND_DATA_SERVER:
        id = this->CreateConnection(cc, 0);
        break;
        }

      if (id != vtkProcessModuleConnectionManager::NullConnectionID)
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
void vtkProcessModuleConnectionManager::CloseConnection(vtkIdType id)
{
  if (id == vtkProcessModuleConnectionManager::GetSelfConnectionID())
    {
    vtkWarningMacro("Cannot drop self connection.");
    return;
    }
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(id);
  if (conn)
    {
    conn->Finalize();
    this->DropConnection(conn);
    }

}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::DropAbortedConnections()
{
  int ret = 0;
  vtkConnectionIterator* iter = vtkConnectionIterator::New();
  for (iter->Begin(); !iter->IsAtEnd(); )
    {
    vtkRemoteConnection* rc = vtkRemoteConnection::SafeDownCast(
      iter->GetCurrentConnection());
    iter->Next();

    if (rc && rc->GetAbortConnection())
      {
      this->DropConnection(rc);
      ret++;
      }
    }
  iter->Delete();
  return ret;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::IsServerConnection(
  vtkIdType connection)
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
vtkIdType vtkProcessModuleConnectionManager::GetConnectionID(
  vtkProcessModuleConnection* conn)
{
  if (!conn)
    {
    return vtkProcessModuleConnectionManager::GetNullConnectionID();
    }

  vtkConnectionIterator* iter = this->NewIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    if (iter->GetCurrentConnection() == conn)
      {
      vtkIdType cid = iter->GetCurrentConnectionID();
      iter->Delete();
      return cid;
      }
    }
  iter->Delete();
  return vtkProcessModuleConnectionManager::GetNullConnectionID();
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnectionManager::SendStream(vtkIdType connectionID, 
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
int vtkProcessModuleConnectionManager::GetNumberOfPartitions(vtkIdType id)
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
  vtkIdType connectionID, vtkTypeUInt32 serverFlags,
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
    vtkIdType connectionID, vtkTypeUInt32 server)
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
int vtkProcessModuleConnectionManager::LoadModule(vtkIdType connectionID, 
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
  vtkIdType id)
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
int vtkProcessModuleConnectionManager::GetRenderClientMode(vtkIdType id)
{
  vtkServerConnection* conn = vtkServerConnection::SafeDownCast(
    this->GetConnectionFromID(id));
  if (conn)
    {
    return (conn->GetRenderServerSocketController()? 1 : 0);
    }

  return 0;
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
vtkIdType vtkProcessModuleConnectionManager::GetUniqueConnectionID()
{
  return this->UniqueConnectionID++;
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModuleConnectionManager::CreateConnection(
  vtkClientSocket* cs, vtkClientSocket* renderserver_socket)
{
  vtkIdType id = vtkProcessModuleConnectionManager::NullConnectionID;

  vtkRemoteConnection* rc = this->NewRemoteConnection();
  if (rc)
    {
    if (rc->SetSocket(cs) == 0)
      {
      rc->Delete();
      vtkErrorMacro("Handshake failed. You are probably using mismatching "
                    "versions of client and server.");
      return id;
      }

    if (renderserver_socket && vtkServerConnection::SafeDownCast(rc))
      {
      vtkServerConnection* sc = vtkServerConnection::SafeDownCast(rc);
      if (sc->SetRenderServerSocket(renderserver_socket) == 0)
        {
        rc->Delete();
        vtkErrorMacro("RenderServer Handshake failed.");
        return id;
        }
      }

    if (rc->Initialize(0, 0, NULL)!=0) // 0 == SUCCESS.
      {
      vtkErrorMacro("Rejecting new connection.");
      rc->Delete();
      return id;
      }

    // Handshake and authentication succeded.
    // Connection is set up perfect!
    id = this->GetUniqueConnectionID();
    this->SetConnection(id, rc);
    this->AddManagedSocket(cs, rc);
    rc->Delete();
    } 

  // Invoke connection created event.
  this->InvokeEvent(vtkCommand::ConnectionCreatedEvent, &id);
  return id;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::SetConnection(vtkIdType id,
  vtkProcessModuleConnection* conn)
{
  this->Internals->IDToConnectionMap[id] = conn; 
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::DropConnection(
  vtkProcessModuleConnection* conn)
{
  conn->RemoveObserver(this->Observer);

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

  vtkIdType dropped_id = vtkProcessModuleConnectionManager::GetNullConnectionID();
  vtkProcessModuleConnectionManagerInternals::MapOfIDToConnection::iterator iter2;
  for (iter2 = this->Internals->IDToConnectionMap.begin();
    iter2 != this->Internals->IDToConnectionMap.end(); ++iter2)
    {
    if (iter2->second.GetPointer() == conn)
      {
      dropped_id = iter2->first;
      this->Internals->IDToConnectionMap.erase(iter2);
      break;
      }
    }

  // Now conn is an invalid pointer.
  // When conn got destroyed, it close the socket as well 
  // as the controller was finalized, so all cleaned up.

  // Let the world know.
  this->InvokeEvent(vtkCommand::ConnectionClosedEvent, &dropped_id);
}

//-----------------------------------------------------------------------------
unsigned int vtkProcessModuleConnectionManager::GetNumberOfConnections()
{
  return static_cast<unsigned int>(this->Internals->IDToConnectionMap.size());
}

//-----------------------------------------------------------------------------
vtkRemoteConnection* vtkProcessModuleConnectionManager::NewRemoteConnection()
{
  vtkRemoteConnection* rc = 0;
  if (this->ClientMode)
    {
    rc = vtkServerConnection::New();
    }
  else
    {
    rc = vtkClientConnection::New();
    }
  rc->AddObserver(vtkCommand::AbortCheckEvent, this->Observer);
  return rc;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid, void* vtkNotUsed(data))
{
  if (eventid == vtkCommand::AbortCheckEvent)
    {
    this->InvokeEvent(vtkCommand::AbortCheckEvent);
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::PushUndo(vtkIdType id, 
  const char* label, vtkPVXMLElement* root)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(id);
  if (!conn)
    {
    vtkErrorMacro("Failed to locate connection with id " << id);
    return;
    }
  conn->PushUndo(label, root);
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkProcessModuleConnectionManager::NewNextUndo(
  vtkIdType id)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(id);
  if (!conn)
    {
    vtkErrorMacro("Failed to locate connection with id " << id);
    return 0;
    }
  return conn->NewNextUndo();
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkProcessModuleConnectionManager::NewNextRedo(
  vtkIdType id)
{
  vtkProcessModuleConnection* conn = this->GetConnectionFromID(id);
  if (!conn)
    {
    vtkErrorMacro("Failed to locate connection with id " << id);
    return 0;
    }
  return conn->NewNextRedo();
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnectionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
