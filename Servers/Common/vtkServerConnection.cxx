/*=========================================================================

  Program:   ParaView
  Module:    vtkServerConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkServerConnection.h"

#include "vtkClientServerStream.h"
#include "vtkClientSocket.h"
#include "vtkCommand.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION_MAJOR etc.
#include "vtkPVOptions.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkPVServerInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkSocketCommunicator.h"

#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>


vtkStandardNewMacro(vtkServerConnection);
//-----------------------------------------------------------------------------
vtkServerConnection::vtkServerConnection()
{
  this->RenderServerSocketController = 0;
  this->NumberOfServerProcesses = 0;
  this->MPIMToNSocketConnectionID.ID = 0;
  this->ServerInformation = vtkPVServerInformation::New();
  this->LastResultStream = new vtkClientServerStream;

}

//-----------------------------------------------------------------------------
vtkServerConnection::~vtkServerConnection()
{
  if (this->RenderServerSocketController)
    {
    this->RenderServerSocketController->Delete();
    this->RenderServerSocketController = NULL;
    }
  this->ServerInformation->Delete();
  delete this->LastResultStream;
}

//-----------------------------------------------------------------------------
void vtkServerConnection::OnSocketError()
{
  if (!this->AbortConnection)
    {
    vtkErrorMacro("Server Connection Closed!");
    }
  this->Superclass::OnSocketError();
}

//-----------------------------------------------------------------------------
void vtkServerConnection::Finalize()
{
  if (this->MPIMToNSocketConnectionID.ID)
    {
    vtkClientServerStream stream;
    vtkProcessModule::GetProcessModule()->DeleteStreamObject(
      this->MPIMToNSocketConnectionID, stream);
    this->SendStream(vtkProcessModule::DATA_SERVER | 
      vtkProcessModule::RENDER_SERVER, stream);
    this->MPIMToNSocketConnectionID.ID = 0;
    }
  
  if (this->RenderServerSocketController)
    {
    this->RenderServerSocketController->CloseConnection();
    this->RenderServerSocketController->Finalize(1);
    }
  this->GetSocketController()->CloseConnection();
  this->Superclass::Finalize();
}

//-----------------------------------------------------------------------------
int vtkServerConnection::SetRenderServerSocket(vtkClientSocket* soc)
{
  if (!this->RenderServerSocketController)
    {
    this->RenderServerSocketController = vtkSocketController::New();
    }
  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
    this->RenderServerSocketController->GetCommunicator());
  if (!comm)
    {
    vtkErrorMacro("Failed to get the socket communicator!");
    return 0;
    }
  comm->SetSocket(soc);
  soc->AddObserver(vtkCommand::ErrorEvent, this->GetObserver());
  comm->AddObserver(vtkCommand::ErrorEvent, this->GetObserver());
  return comm->Handshake();
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkServerConnection::CreateSendFlag(vtkTypeUInt32 serverFlags)
{
  if (this->RenderServerSocketController)
    {
    // Using separate connection RenderServer. So the serverFlags remain unchaged.
    return serverFlags;
    }

  // for data server only mode convert all render server calls
  // into data server calls
 
  vtkTypeUInt32 sendflag = 0;
  if (serverFlags & vtkProcessModule::CLIENT)
    {
    sendflag |= vtkProcessModule::CLIENT;
    }
  if(serverFlags & vtkProcessModule::RENDER_SERVER)
    {
    sendflag |= vtkProcessModule::DATA_SERVER;
    }
  if(serverFlags & vtkProcessModule::RENDER_SERVER_ROOT)
    {
    sendflag |= vtkProcessModule::DATA_SERVER_ROOT;
    }
  if(serverFlags & vtkProcessModule::DATA_SERVER)
    {
    sendflag |= vtkProcessModule::DATA_SERVER;
    }
  if(serverFlags & vtkProcessModule::DATA_SERVER_ROOT)
    {
    sendflag |= vtkProcessModule::DATA_SERVER_ROOT;
    }
  return sendflag;
}
//-----------------------------------------------------------------------------
// send a stream to the data server
int vtkServerConnection::SendStreamToDataServer(vtkClientServerStream& stream)
{
  return this->SendStreamToServer(this->GetSocketController(), stream);
}

//-----------------------------------------------------------------------------
// send a stream to the data server root mpi process
int vtkServerConnection::SendStreamToDataServerRoot(vtkClientServerStream& stream)
{
  return this->SendStreamToRoot(this->GetSocketController(), stream);
}

//-----------------------------------------------------------------------------
// send a stream to the render server
int vtkServerConnection::SendStreamToRenderServer(vtkClientServerStream& stream)
{
  return this->SendStreamToServer(this->RenderServerSocketController, stream);
}

//-----------------------------------------------------------------------------
// send a stream to the render server root mpi process
int vtkServerConnection::SendStreamToRenderServerRoot(vtkClientServerStream& stream)
{
  return this->SendStreamToRoot(this->RenderServerSocketController, stream);
}

//-----------------------------------------------------------------------------
// send a stream to the self connection.
int vtkServerConnection::SendStreamToClient(vtkClientServerStream& stream)
{
  // Eventually, each remote connection will have its own self connection
  // (i.e. separate Interpreters for each connection). Here, we will use
  // the self connection  for this remote connection.
  // For now, we simply use the common self connection.
  this->Activate();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int ret = pm->SendStream(
    vtkProcessModuleConnectionManager::GetSelfConnectionID(),
    vtkProcessModule::CLIENT, stream, 0);
  this->Deactivate();
  return ret;
}

//-----------------------------------------------------------------------------
int vtkServerConnection::SendStreamToServer(vtkSocketController* controller,
  vtkClientServerStream& stream)
{
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  controller->TriggerRMI(1, (void*)data, static_cast<int>(len), 
    vtkRemoteConnection::CLIENT_SERVER_RMI_TAG);
  return 0;
}

//-----------------------------------------------------------------------------
int vtkServerConnection::SendStreamToRoot(vtkSocketController* controller,
  vtkClientServerStream& stream)
{
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  controller->TriggerRMI(1, (void*)data, static_cast<int>(len), 
    vtkRemoteConnection::CLIENT_SERVER_ROOT_RMI_TAG);
  return 0;
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkServerConnection::GetLastResult(vtkTypeUInt32 
  serverFlags)
{
  vtkTypeUInt32 sendflag = this->CreateSendFlag(serverFlags);
  if (sendflag & vtkProcessModule::CLIENT)
    {
    // return the last result from the self connection.
    // For now, this is the same single SelfConnection kept by ProcessModule.
    // Eventually, each connection will have its own self connection.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    return pm->GetLastResult(vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT);
    }
  if ((sendflag & vtkProcessModule::DATA_SERVER_ROOT) 
    || (sendflag & vtkProcessModule::DATA_SERVER))
    {
    return this->GetLastResultInternal(this->GetSocketController());
    }
  if ((sendflag & vtkProcessModule::RENDER_SERVER_ROOT) || 
    (sendflag &  vtkProcessModule::RENDER_SERVER))
    {
    return this->GetLastResultInternal(this->GetRenderServerSocketController());
    }

  vtkWarningMacro("GetLastResult called with bad server flag. "
    << "Returning DATA SERVER result.");
  return this->GetLastResultInternal(this->GetSocketController());
  
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkServerConnection::GetLastResultInternal(
  vtkSocketController* controller)
{
  if (this->AbortConnection)
    {
    // Don't get last restult on an aborted connection.
    this->LastResultStream->Reset();
    return *this->LastResultStream;
    }

  int length =0;
  controller->TriggerRMI(1, "", 
    vtkRemoteConnection::CLIENT_SERVER_LAST_RESULT_TAG);
  controller->Receive(&length, 1, 1, 
    vtkRemoteConnection::ROOT_RESULT_LENGTH_TAG);
  if (length <= 0)
    {
    this->LastResultStream->Reset();
    *this->LastResultStream
      << vtkClientServerStream::Error
      << "vtkServerConnection::GetLastResultInternal() received no data from the"
      << " server." << vtkClientServerStream::End;
    }
  else
    {
    unsigned char* result = new unsigned char[length];
    controller->Receive((char*)result, length, 1, 
      vtkRemoteConnection::ROOT_RESULT_TAG);
    this->LastResultStream->SetData(result, length);
    delete[] result;
    }
  return *this->LastResultStream;
}

//-----------------------------------------------------------------------------
void vtkServerConnection::GatherInformation(vtkTypeUInt32 serverFlags, 
  vtkPVInformation* info, vtkClientServerID id)
{
  if (this->AbortConnection)
    {
    // Don't gather info on an aborted connection.
    return;
    }
  serverFlags = this->CreateSendFlag(serverFlags);

  if (serverFlags & vtkProcessModule::CLIENT)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkObject* object = vtkObject::SafeDownCast(pm->GetObjectFromID(id));
    if (!object)
      {
      vtkErrorMacro("Failed to locate object with ID: " << id);
      return;
      }

    info->CopyFromObject(object);
    }

  if (serverFlags & vtkProcessModule::DATA_SERVER ||
    serverFlags & vtkProcessModule::DATA_SERVER_ROOT)
    {
    this->GatherInformationFromController(this->GetSocketController(), 
      info, id);
    return;
    }
  
  if ( (serverFlags & vtkProcessModule::RENDER_SERVER 
      || serverFlags & vtkProcessModule::RENDER_SERVER_ROOT) 
    && this->RenderServerSocketController)
    {
    this->GatherInformationFromController(this->RenderServerSocketController,
      info, id);
    return;
    }
}

//-----------------------------------------------------------------------------
void vtkServerConnection::GatherInformationFromController(vtkSocketController* controller,
  vtkPVInformation* info, vtkClientServerID id)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Assign // dummy command.
    << info->GetClassName()
    << id << vtkClientServerStream::End;
  const unsigned char* data;
  size_t length;
  stream.GetData(&data, &length);
  controller->TriggerRMI(1, (void*)(data), static_cast<int>(length),
    vtkRemoteConnection::CLIENT_SERVER_GATHER_INFORMATION_RMI_TAG);
  
  int length2 = 0;
  controller->Receive(&length2, 1, 1,
    vtkRemoteConnection::ROOT_INFORMATION_LENGTH_TAG);
  if (length2 <= 0)
    {
    vtkErrorMacro("Server failed to gather information.");
    return;
    }
  unsigned char* data2 = new unsigned char[length2];
  if (!controller->Receive((char*)data2, length2, 1, 
    vtkRemoteConnection::ROOT_INFORMATION_TAG))
    {
    vtkErrorMacro("Failed to receive information correctly.");
    delete [] data2;
    return;
    }
  stream.SetData(data2, length2);
  info->CopyFromStream(&stream);
  delete [] data2;
}

//-----------------------------------------------------------------------------
int vtkServerConnection::Initialize(int argc, char** argv, int *partitionId)
{
  this->Superclass::Initialize(argc, argv, partitionId);
  // returns 0 on success, 1 on error.

  
  // Authenticate with DataServer.
  if (!this->AuthenticateWithServer(this->GetSocketController()))
    {
    vtkErrorMacro("Failed to authenticate with Data Server.");
    return 1;
    }

  // Authenticate with RenderServer.
  if (!this->AuthenticateWithServer(this->RenderServerSocketController))
    {
    vtkErrorMacro("Failed to authenticate with Render Server.");
    return 1;
    }
 
  if (!this->SetupDataServerRenderServerConnection())
    {
    vtkErrorMacro("Failed to synchronize Data Server and Render Server.");
    return 1;
    }

  // Collect and keep the server information.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVServerInformation* temp = vtkPVServerInformation::New();
  this->GatherInformation(
    vtkProcessModule::RENDER_SERVER,
    temp,
    pm->GetProcessModuleID());

  this->ServerInformation->AddInformation(temp);

  this->GatherInformation(
    vtkProcessModule::DATA_SERVER,
    temp,
    pm->GetProcessModuleID());
  this->ServerInformation->AddInformation(temp);
  temp->Delete();

  return 0;
}


//-----------------------------------------------------------------------------
int vtkServerConnection::AuthenticateWithServer(vtkSocketController* controller)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();

  if (!options->GetClientMode())
    {
    // Sanity Check.
    vtkErrorMacro("vtkServerConnection must be instantiated only on a client.");
    return 0;
    }

  if (!controller)
    {
    // No controller, we are not using it. 
    return 1;
    }

  // If any send or receive fails we simply give up.
  // Send connect ID to the server.
  int cid = options->GetConnectID();
  if (!controller->Send(&cid, 1, 1, 
      vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG))
    {
    return 0;
    }

  int match = 0;
  controller->Receive(&match, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
  if (!match)
    {
    vtkErrorMacro("Connection ID mismatch. Make sure to specify the right "
                  "connection id on the server and client. This error can "
                  "also occur if you have mismatching client and server "
                  "versions even if you are not using connection ids.");
    return 0;
    }

  // Send the client version
  int version;
  version = PARAVIEW_VERSION_MAJOR;
  if (!controller->Send(&version, 1, 1, 
      vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG))
    {
    return 0;
    }
  version = PARAVIEW_VERSION_MINOR;
  if (!controller->Send(&version, 1, 1, 
      vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG))
    {
    return 0;
    }
  version = PARAVIEW_VERSION_PATCH;
  if (!controller->Send(&version, 1, 1,
      vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG))
    {
    return 0;
    }

  match = 0;
  controller->Receive(&match, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
  if (!match)
    {
    vtkErrorMacro("Version mismatch.");
    return 0;
    }

  // Receive the number of server processes as an handshake.
  int numServerProcs =  0;
  if (!controller->Receive(&numServerProcs, 1, 1,
      vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG) || 
    (numServerProcs == 0))
    {
    vtkErrorMacro("Failed to receive handshake message.");
    return 0;
    }

  // Since no of Data Server Processes >= no. of Render Server Processes,
  // this check ensures we only maintain the Data Server processes count.
  this->NumberOfServerProcesses = 
    (numServerProcs > this->NumberOfServerProcesses)? numServerProcs :
    this->NumberOfServerProcesses;

  controller->GetCommunicator()->AddObserver(vtkCommand::WrongTagEvent, 
    this->GetObserver());
  return 1;
}

//-----------------------------------------------------------------------------
int vtkServerConnection::SetupDataServerRenderServerConnection()
{
  if (!this->RenderServerSocketController)
    {
    // Not using a separate render server. Nothing to do here.
    return 1;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();

  vtkClientServerStream stream;
  int numOfRenderServerNodes = 0;
  int connectingServer;
  int waitingServer;
  
  if (options->GetRenderServerMode() == 1)
    {
    connectingServer = vtkProcessModule::DATA_SERVER;
    waitingServer = vtkProcessModule::RENDER_SERVER;
    }
  else // i.e. if (options->GetRenderServerMode() == 2).
    {
    waitingServer = vtkProcessModule::DATA_SERVER;
    connectingServer = vtkProcessModule::RENDER_SERVER;
    }
  
  // Create a vtkMPIMToNSocketConnection object on both the 
  // servers.  This object holds the vtkSocketCommunicator object
  // for each machine and makes the connections
  vtkClientServerID id = 
    pm->NewStreamObject("vtkMPIMToNSocketConnection", stream);
  this->MPIMToNSocketConnectionID = id;
  this->SendStream(
    vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);
  stream.Reset();

  vtkMPIMToNSocketConnectionPortInformation* info 
    = vtkMPIMToNSocketConnectionPortInformation::New();
  
  // if the data server is going to wait for the render server
  // then we have to tell the data server how many connections to make
  // if the render server is waiting, it already knows how many to make
  if (waitingServer == vtkProcessModule::DATA_SERVER)
    {
    // Get number of processes on the render server.
    this->GatherInformation(vtkProcessModule::RENDER_SERVER, info, id);

    // Tell data server this number.
    numOfRenderServerNodes = info->GetNumberOfConnections();

    stream << vtkClientServerStream::Invoke << id
      << "SetNumberOfConnections" << numOfRenderServerNodes
      << vtkClientServerStream::End;
    this->SendStream(vtkProcessModule::DATA_SERVER, stream);
    stream.Reset();
    }

  // Now initialize the waiting server and have it set up the connections.
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetRenderNodePort" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << id << "SetPortNumber" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetMachinesFileName" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << id << "SetMachinesFileName" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetNumberOfMachines"
         << vtkClientServerStream::End;
  this->SendStream(waitingServer, stream);
  stream.Reset();
  unsigned int numMachines = 0;
  this->GetLastResult(waitingServer).GetArgument(0, 0, &numMachines);

  unsigned int idx;
  for (idx = 0; idx < numMachines; idx++)
    {
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetMachineName" << idx
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << id << "SetMachineName" << idx
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  stream << vtkClientServerStream::Invoke 
         << id << "SetupWaitForConnection"
         << vtkClientServerStream::End;
  this->SendStream(waitingServer, stream); 
  stream.Reset();

  // Get the information about the connection after the call to
  // SetupWaitForConnection
  if(connectingServer == vtkProcessModule::DATA_SERVER)
    {
    this->GatherInformation(vtkProcessModule::RENDER_SERVER, info, id);
    numOfRenderServerNodes = info->GetNumberOfConnections();
    }
  else
    {
    this->GatherInformation(vtkProcessModule::DATA_SERVER, info, id);
    }
  
  // let the connecting server know how many render nodes 
  stream << vtkClientServerStream::Invoke 
         << id << "SetNumberOfConnections" << numOfRenderServerNodes
         << vtkClientServerStream::End;

  // set up host/port information for the connecting
  // server so it will know what machines to connect to
  for(int i=0; i < numOfRenderServerNodes; ++i)
    {
    stream << vtkClientServerStream::Invoke 
           << id 
           << "SetPortInformation" 
           << static_cast<unsigned int>(i)
           << info->GetProcessPort(i)
           << info->GetProcessHostName(i)
           << vtkClientServerStream::End;
    }
  this->SendStream(connectingServer, stream);
  stream.Reset();
  // all should be ready now to wait and connect

  // tell the waiting server to wait for the connections
  stream << vtkClientServerStream::Invoke 
         << id << "WaitForConnection"
         << vtkClientServerStream::End;
  this->SendStream(waitingServer, stream);
  stream.Reset();

  // tell the connecting server to make the connections
  stream << vtkClientServerStream::Invoke 
         << id << "Connect"
         << vtkClientServerStream::End;
  this->SendStream(connectingServer, stream);
  stream.Reset();
  info->Delete();
  return 1;
}


//-----------------------------------------------------------------------------
void vtkServerConnection::PushUndo(const char* label, vtkPVXMLElement* uelem)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("ClientServerUndoRedo");
  root->SetAttribute("label", label);
  root->AddNestedElement(uelem);

  vtksys_ios::ostringstream xml_stream;
  root->PrintXML(xml_stream, vtkIndent());
  root->Delete();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << label
    << xml_stream.str().c_str()
    << vtkClientServerStream::End;

  // Send the string to server.
  vtkSocketController* controller = this->GetSocketController();
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  
  controller->TriggerRMI(1, (void*)(data), static_cast<int>(len), 
    vtkRemoteConnection::CLIENT_SERVER_PUSH_UNDO_XML_TAG);
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkServerConnection::NewNextUndo()
{
  vtkSocketController* controller = this->GetSocketController();
  controller->TriggerRMI(1, NULL, 0, vtkRemoteConnection::UNDO_XML_TAG);
  int length;
  controller->Receive(&length, 1, 1, 
    vtkRemoteConnection::UNDO_XML_TAG);
  if (length <= 0)
    {
    vtkErrorMacro("Failed to Undo. Server did not return any state.");
    return 0;
    }

  char* data = new char[length+1];
  controller->Receive(data, length, 1, vtkRemoteConnection::UNDO_XML_TAG);
  data[length] = 0;

  // Now parse the XML.
  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
  if (!parser->Parse(data))
    {
    vtkErrorMacro("Server Undo state response is invalid. Must be valid XML.");
    delete [] data;
    return 0;
    }
  
  if (!parser->GetRootElement() || 
    strcmp(parser->GetRootElement()->GetName(), "ClientServerUndoRedo") != 0)
    {
    vtkErrorMacro("Invalid XML, expected ClientServerUndoRedo element.");
    delete [] data;
    return 0;
    }
  
  vtkPVXMLElement* undoelem = parser->GetRootElement()->GetNestedElement(0);
  if (undoelem)
    {
    undoelem->Register(this);
    }
  delete [] data;
  return undoelem;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkServerConnection::NewNextRedo()
{
  vtkSocketController* controller = this->GetSocketController();
  controller->TriggerRMI(1, NULL, 0, vtkRemoteConnection::REDO_XML_TAG);
  int length;
  controller->Receive(&length, 1, 1, 
    vtkRemoteConnection::REDO_XML_TAG);
  if (length <= 0)
    {
    vtkErrorMacro("Failed to Redo. Server did not return any state.");
    return 0;
    }

  char* data = new char[length+1];
  controller->Receive(data, length, 1, vtkRemoteConnection::REDO_XML_TAG);
  data[length] = 0;

  // Now parse the XML.
  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
  if (!parser->Parse(data))
    {
    vtkErrorMacro("Server Undo state response is invalid. Must be valid XML.");
    delete [] data;
    return 0;
    }

  if (!parser->GetRootElement() || 
    strcmp(parser->GetRootElement()->GetName(), "ClientServerUndoRedo") != 0)
    {
    vtkErrorMacro("Invalid XML, expected ClientServerUndoRedo element.");
    delete [] data;
    return 0;
    }

  vtkPVXMLElement* undoelem = parser->GetRootElement()->GetNestedElement(0);
  if (undoelem)
    {
    undoelem->Register(this);
    }
  delete [] data;
  return undoelem;
}

//-----------------------------------------------------------------------------
void vtkServerConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MPIMToNSocketConnectionID: " 
    << this->MPIMToNSocketConnectionID << endl;

  os << indent << "ServerInformation: ";
  if (this->ServerInformation)
    {
    this->ServerInformation->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
