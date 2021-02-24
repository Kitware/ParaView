/*=========================================================================

  Program:   ParaView
  Module:    vtkTCPNetworkAccessManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTCPNetworkAccessManager.h"

#include "vtkClientSocket.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkServerSocket.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkWeakPointer.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// set this to 1 if you want to generate a log file with all the raw socket
// communication.
#define GENERATE_DEBUG_LOG 0

#define MAX_SOCKETS 256

class vtkTCPNetworkAccessManager::vtkInternals
{
public:
  typedef std::vector<vtkWeakPointer<vtkSocketController> > VectorOfControllers;
  VectorOfControllers Controllers;
  typedef std::map<int, vtkSmartPointer<vtkServerSocket> > MapToServerSockets;
  MapToServerSockets ServerSockets;
};

vtkStandardNewMacro(vtkTCPNetworkAccessManager);
//----------------------------------------------------------------------------
vtkTCPNetworkAccessManager::vtkTCPNetworkAccessManager()
{
  this->Internals = new vtkInternals();
  this->AbortPendingConnectionFlag = false;
  this->WrongConnectID = false;

  // It's essential to initialize the socket controller to initialize sockets on
  // Windows.
  vtkSocketController* controller = vtkSocketController::New();
  controller->Initialize();
  controller->Delete();
}

//----------------------------------------------------------------------------
vtkTCPNetworkAccessManager::~vtkTCPNetworkAccessManager()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkTCPNetworkAccessManager::NewConnection(const char* url)
{
  vtksys::RegularExpression re_connect("^tcp://([^:]+)?:([0-9]+)\\?\?((&?[a-zA-Z0-9%]+=[^&]+)*)");
  vtksys::RegularExpression key_val("([a-zA-Z0-9%]+)=([^&]+)");

  std::map<std::string, std::string> parameters;
  if (re_connect.find(url))
  {
    std::string hostname = re_connect.match(1);
    int port = atoi(re_connect.match(2).c_str());

    // there some issue with RegularExpression that I cannot extract parameters.
    // hence we do this:
    std::vector<std::string> param_vals =
      vtksys::SystemTools::SplitString(re_connect.match(3).c_str(), '&');
    for (size_t cc = 0; cc < param_vals.size(); cc++)
    {
      if (key_val.find(param_vals[cc]))
      {
        std::string key = key_val.match(1);
        std::string value = key_val.match(2);
        parameters[key] = value;
      }
    }

    const char* handshake = nullptr;
    if (parameters.find("handshake") != parameters.end())
    {
      handshake = parameters["handshake"].c_str();
    }
    int timeout_in_seconds = 60;
    if (parameters.find("timeout") != parameters.end())
    {
      timeout_in_seconds = atoi(parameters["timeout"].c_str());
    }

    this->WrongConnectID = false;

    if (parameters["listen"] == "true" && parameters["multiple"] == "true")
    {
      return this->WaitForConnection(port, false, handshake, parameters["nonblocking"] == "true");
    }
    else if (parameters["listen"] == "true")
    {
      return this->WaitForConnection(port, true, handshake, parameters["nonblocking"] == "true");
    }
    else
    {
      return this->ConnectToRemote(hostname.c_str(), port, handshake, timeout_in_seconds);
    }
  }
  else
  {
    vtkErrorMacro("Malformed URL: " << (url ? url : "(empty)"));
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkTCPNetworkAccessManager::AbortPendingConnection()
{
  this->AbortPendingConnectionFlag = true;
}

//----------------------------------------------------------------------------
bool vtkTCPNetworkAccessManager::GetPendingConnectionsPresent()
{
  // FIXME_COLLABORATION
  cout << "Need to fix this to report real pending connections" << endl;
  return false;
}

//----------------------------------------------------------------------------
void vtkTCPNetworkAccessManager::DisableFurtherConnections(int port, bool disable)
{
  if (disable)
  {
    if (this->Internals->ServerSockets.find(port) != this->Internals->ServerSockets.end())
    {
      this->Internals->ServerSockets.at(port)->CloseSocket();
      this->Internals->ServerSockets.erase(port);
    }
  }
  else
  {
    vtkServerSocket* server_socket = vtkServerSocket::New();
    if (server_socket->CreateServer(port) != 0)
    {
      vtkErrorMacro("Failed to set up server socket.");
      server_socket->Delete();
      return;
    }
    this->Internals->ServerSockets[port] = server_socket;
    server_socket->FastDelete();
  }
}

//----------------------------------------------------------------------------
bool vtkTCPNetworkAccessManager::GetWrongConnectID()
{
  return this->WrongConnectID;
}

//----------------------------------------------------------------------------
bool vtkTCPNetworkAccessManager::GetNetworkEventsAvailable()
{
  return (this->ProcessEventsInternal(1, false) == 1);
}

//----------------------------------------------------------------------------
int vtkTCPNetworkAccessManager::ProcessEvents(unsigned long timeout_msecs)
{
  return this->ProcessEventsInternal(timeout_msecs, true);
}

//----------------------------------------------------------------------------
int vtkTCPNetworkAccessManager::ProcessEventsInternal(
  unsigned long timeout_msecs, bool do_processing)
{
  int sockets_to_select[MAX_SOCKETS];
  vtkObject* controller_or_server_socket[MAX_SOCKETS];

  vtkSocketController* ctrlWithBufferToEmpty = nullptr;
  int size = 0;
  vtkInternals::VectorOfControllers::iterator iter1;
  for (iter1 = this->Internals->Controllers.begin(); iter1 != this->Internals->Controllers.end();
       ++iter1)
  {
    vtkSocketController* controller = iter1->GetPointer();
    if (!controller)
    {
      // skip nullptr controllers.
      continue;
    }
    vtkSocketCommunicator* comm =
      vtkSocketCommunicator::SafeDownCast(controller->GetCommunicator());
    vtkSocket* socket = comm->GetSocket();
    if (socket && socket->GetConnected())
    {
      sockets_to_select[size] = socket->GetSocketDescriptor();
      controller_or_server_socket[size] = controller;
      if (comm->HasBufferredMessages())
      {
        ctrlWithBufferToEmpty = controller;
        if (!do_processing)
        {
          // we do have events to process, but we were told not to process them,
          // so just return and say we have something to process here.
          return 1;
        }
      }
      size++;
    }
  }

  // Only one client connected, so if it fails, just quit...
  bool can_quit_if_error = (size == 1);

  // Now add server sockets.
  vtkInternals::MapToServerSockets::iterator iter2;
  for (iter2 = this->Internals->ServerSockets.begin();
       iter2 != this->Internals->ServerSockets.end(); ++iter2)
  {
    if (iter2->second.GetPointer() && iter2->second.GetPointer()->GetConnected())
    {
      sockets_to_select[size] = iter2->second.GetPointer()->GetSocketDescriptor();
      controller_or_server_socket[size] = iter2->second.GetPointer();
      size++;
    }
  }

  if (size == 0 || this->AbortPendingConnectionFlag)
  {
    // Connection failed / aborted.
    return -1;
  }

  // Try to empty RMI buffered messages if any
  if (ctrlWithBufferToEmpty &&
    (ctrlWithBufferToEmpty->ProcessRMIs(0, 1) == vtkMultiProcessController::RMI_NO_ERROR))
  {
    return 1;
  }

  int selected_index = -1;
  int result = vtkSocket::SelectSockets(sockets_to_select, size, timeout_msecs, &selected_index);
  if (result <= 0)
  {
    return result;
  }
  if (result > 0 && !do_processing)
  {
    // we were told not to do any processing, so just let the caller know that
    // we have events to process.
    return 1;
  }

  if (controller_or_server_socket[selected_index]->IsA("vtkServerSocket"))
  {
    vtkServerSocket* ss =
      static_cast<vtkServerSocket*>(controller_or_server_socket[selected_index]);
    int port = ss->GetServerPort();
    this->InvokeEvent(vtkCommand::ConnectionCreatedEvent, &port);
    return 1;
  }
  else
  {
    // We use smart pointer here to make sure the controller will live
    // during the whole ProcessRMIs call. As that call can release
    // the controller while executing.
    vtkSmartPointer<vtkMultiProcessController> controller =
      vtkMultiProcessController::SafeDownCast(controller_or_server_socket[selected_index]);
    result = controller->ProcessRMIs(0, 1);
    if (result == vtkMultiProcessController::RMI_NO_ERROR)
    {
      // all's well.
      return 1;
    }

    if (can_quit_if_error)
    {
      vtkErrorMacro("Some error in socket processing.");
    }

    // Close cleanly the socket in error
    vtkSocketCommunicator* comm =
      vtkSocketCommunicator::SafeDownCast(controller->GetCommunicator());
    comm->CloseConnection();

    // Fire an event letting the world know that the connection was closed.
    this->InvokeEvent(vtkCommand::ConnectionClosedEvent, controller);

    return can_quit_if_error ? -1 /* Quit */ : 1 /* Pretend it's OK */;
  }
}

//----------------------------------------------------------------------------
void vtkTCPNetworkAccessManager::PrintHandshakeError(int errorcode, bool server_side)
{
  switch (errorcode)
  {
    case HANDSHAKE_NO_ERROR:
    default:
      vtkErrorMacro("\n"
                    "**********************************************************************\n"
                    "Connection failed during handshake. This is likely because the connection\n"
                    " was dropped during the handshake.\n"
                    "**********************************************************************\n");
      break;
    case HANDSHAKE_SOCKET_COMMUNICATOR_DIFFERENT:
      vtkErrorMacro("\n"
                    "**********************************************************************\n"
                    "Connection failed during handshake. vtkSocketCommunicator::GetVersion()\n"
                    " returns different values on the two connecting processes\n"
                    " (Current value: "
        << vtkSocketCommunicator::GetVersion()
        << ").\n"
           "**********************************************************************\n");
      break;
    case HANDSHAKE_DIFFERENT_PV_VERSIONS:
      vtkErrorMacro("\n"
                    "**********************************************************************\n"
                    " Connection failed during handshake.  The server has a different ParaView"
                    " version than the client.\n"
                    "**********************************************************************\n");
      break;
    case HANDSHAKE_DIFFERENT_CONNECTION_IDS:
      if (server_side)
      {
        vtkWarningMacro("\n"
                        " Connection failed during handshake. The server has a different \n"
                        " connection id than the client.\n"
                        "\n");
      }
      this->WrongConnectID = true;
      break;
    case HANDSHAKE_DIFFERENT_RENDERING_BACKENDS:
      vtkErrorMacro("\n"
                    "**********************************************************************\n"
                    " Connection failed during handshake.  The server has a different rendering"
                    " backend from the client.\n"
                    "**********************************************************************\n");
      break;
    case HANDSHAKE_UNKNOWN_ERROR:
      vtkErrorMacro(
        "\n"
        "************************************************************************\n"
        "Connection failed during handshake.  Unknown error parsing the handshake string\n"
        "************************************************************************\n");
      break;
  }
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkTCPNetworkAccessManager::ConnectToRemote(
  const char* hostname, int port, const char* handshake, int timeout_in_seconds)
{

  // Create client socket.
  // Create a RemoteConnection (Server/Client)
  // Set the client socket on its controller.
  // Manage the client socket.
  vtkSmartPointer<vtkClientSocket> cs = vtkSmartPointer<vtkClientSocket>::New();
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  while (1)
  {
    if (cs->ConnectToServer(hostname, port) != -1)
    {
      break;
    }
    timer->StopTimer();
    if (timeout_in_seconds >= 0)
    {
      if (timeout_in_seconds == 0 || timer->GetElapsedTime() > timeout_in_seconds)
      {
        vtkErrorMacro(<< "Connect timeout.");
        return nullptr;
      }
      vtkWarningMacro(<< "Connect failed. Retrying for "
                      << (timeout_in_seconds - timer->GetElapsedTime()) << " more seconds.");
    }
    else
    {
      vtkWarningMacro("Connect failed.  Retrying.");
    }
    vtksys::SystemTools::Delay(1000);
  }

  vtkSocketController* controller = vtkSocketController::New();
  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(controller->GetCommunicator());
#if GENERATE_DEBUG_LOG
  std::ostringstream mystr;
  mystr << "/tmp/client." << getpid() << ".log";
  comm->LogToFile(mystr.str().c_str());
#endif
  comm->SetSocket(cs);
  int errorcode = HANDSHAKE_SOCKET_COMMUNICATOR_DIFFERENT;
  if (!comm->Handshake() || (errorcode = this->ParaViewHandshake(controller, false, handshake)))
  {
    controller->Delete();
    // handshake failed, must be bogus client, continue waiting (unless
    // this->AbortPendingConnectionFlag == true).
    this->PrintHandshakeError(errorcode, false);
    return nullptr;
  }
  this->Internals->Controllers.push_back(controller);
  return controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkTCPNetworkAccessManager::WaitForConnection(
  int port, bool once, const char* handshake, bool nonblocking)
{
  vtkServerSocket* server_socket = nullptr;
  if (this->Internals->ServerSockets.find(port) != this->Internals->ServerSockets.end())
  {
    server_socket = this->Internals->ServerSockets[port];
  }
  else
  {
    server_socket = vtkServerSocket::New();
    if (server_socket->CreateServer(port) != 0)
    {
      vtkErrorMacro("Failed to set up server socket.");
      server_socket->Delete();
      return nullptr;
    }
    this->Internals->ServerSockets[port] = server_socket;
    server_socket->FastDelete();
  }

  vtksys::SystemInformation sys_info;
  sys_info.RunOSCheck();
  const char* sys_hostname = sys_info.GetHostname() ? sys_info.GetHostname() : "localhost";

  // print out a status message.
  cout << "Accepting connection(s): " << sys_hostname << ":" << server_socket->GetServerPort()
       << endl;

  this->AbortPendingConnectionFlag = false;
  vtkSocketController* controller = nullptr;

  while (this->AbortPendingConnectionFlag == false && controller == nullptr)
  {
    vtkClientSocket* client_socket = nullptr;
    if (nonblocking)
    {
      client_socket = server_socket->WaitForConnection(100);
    }
    else
    {
      while (this->AbortPendingConnectionFlag == false &&
        ((client_socket = server_socket->WaitForConnection(1000)) == nullptr))
      {
        double progress = 0.5;
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }
    }
    if (!client_socket)
    {
      return nullptr;
    }

    controller = vtkSocketController::New();
    vtkSocketCommunicator* comm =
      vtkSocketCommunicator::SafeDownCast(controller->GetCommunicator());
    comm->SetSocket(client_socket);
    client_socket->FastDelete();
    int errorcode = HANDSHAKE_SOCKET_COMMUNICATOR_DIFFERENT;
    if (comm->Handshake() == 0 ||
      (errorcode = this->ParaViewHandshake(controller, true, handshake)))
    {
      controller->Delete();
      controller = nullptr;
      this->PrintHandshakeError(errorcode, true);
      if (!once)
      {
        break;
      }
    }
  }

  if (controller)
  {
    this->Internals->Controllers.push_back(controller);
  }

  if (once)
  {
    server_socket->CloseSocket();
    this->Internals->ServerSockets.erase(port);
  }

  return controller;
}

int vtkTCPNetworkAccessManager::AnalyzeHandshakeAndGetErrorCode(
  const char* clientHS, const char* serverHS)
{
  vtksys::RegularExpression re(
    "^paraview-([0-9]+\\.[0-9]+)\\.(connect_id\\.([0-9]+)\\.)?renderingbackend\\.([^\\.]+)$");
  bool success = re.find(serverHS);
  // Since this regex and the server handshake are from the same version
  // of ParaView, if it doesn't match then something is very wrong.
  if (!success)
  {
    vtkErrorMacro(
      << "Server connect id did not match regular expression on server.  This shouldn't happen.");
    return HANDSHAKE_UNKNOWN_ERROR;
  }
  std::string serverVersion = re.match(1);
  std::string serverConnectId = re.match(3);
  std::string serverBackend = re.match(4);

  bool clientMatched = re.find(clientHS);
  if (!clientMatched)
  {
    vtkErrorMacro(
      << "Client Handshake didn't parse.  The client is likely a different version of ParaView.");
    return HANDSHAKE_UNKNOWN_ERROR;
  }
  std::string clientVersion = re.match(1);
  std::string clientConnectId = re.match(3);
  std::string clientBackend = re.match(4);

  if (clientVersion != serverVersion)
  {
    vtkErrorMacro(<< "Client and server are different ParaView versions. Client reports version: "
                  << clientVersion << " but server is version " << serverVersion);
    return HANDSHAKE_DIFFERENT_PV_VERSIONS;
  }

  if (clientConnectId != serverConnectId)
  {
    vtkErrorMacro(<< "Client and server connection ids do not match");
    return HANDSHAKE_DIFFERENT_CONNECTION_IDS;
  }

  if (clientBackend != serverBackend)
  {
    vtkErrorMacro(<< "Client and server have different rendering backends. Client: \""
                  << clientBackend << "\" Server: \"" << serverBackend << "\"");
    return HANDSHAKE_DIFFERENT_RENDERING_BACKENDS;
  }

  // If this function was called, there was a difference but since we got here without detecting it
  // we have no idea what it is.
  vtkErrorMacro(
    << "Unknown error in handshakes, client and server are likely different versions of ParaView.");
  return HANDSHAKE_UNKNOWN_ERROR;
}

//----------------------------------------------------------------------------
int vtkTCPNetworkAccessManager::ParaViewHandshake(
  vtkMultiProcessController* controller, bool server_side, const char* _handshake)
{
  const std::string handshake = _handshake ? _handshake : "";
  int size = static_cast<int>(handshake.size() + 1);
  if (server_side)
  {
    std::string other_handshake;
    int othersize;
    controller->Receive(&othersize, 1, 1, 99991);
    if (othersize > 0)
    {
      char* _other_handshake = new char[othersize];
      controller->Receive(_other_handshake, othersize, 1, 99991);
      other_handshake = _other_handshake;
      delete[] _other_handshake;
    }
    int errorCode = HANDSHAKE_NO_ERROR;
    if (handshake != other_handshake)
    {
      errorCode = this->AnalyzeHandshakeAndGetErrorCode(other_handshake.c_str(), handshake.c_str());
    }
    controller->Send(&errorCode, 1, 1, 99990);
    return errorCode;
  }
  else
  {
    controller->Send(&size, 1, 1, 99991);
    if (size > 0)
    {
      controller->Send(handshake.c_str(), size, 1, 99991);
    }
    int errorCode;
    controller->Receive(&errorCode, 1, 1, 99990);
    return errorCode;
  }
}

//----------------------------------------------------------------------------
void vtkTCPNetworkAccessManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
