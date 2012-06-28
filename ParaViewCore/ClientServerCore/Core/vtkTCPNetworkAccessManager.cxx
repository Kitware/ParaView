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
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

#include <string>
#include <vtksys/ios/sstream>
#include <vector>
#include <map>

// set this to 1 if you want to generate a log file with all the raw socket
// communication.
#define GENERATE_DEBUG_LOG 0

#define MAX_SOCKETS 256

class vtkTCPNetworkAccessManager::vtkInternals
{
public:
  typedef std::vector<vtkWeakPointer<vtkSocketController> >
    VectorOfControllers;
  VectorOfControllers Controllers;
  typedef std::map<int, vtkSmartPointer<vtkServerSocket> >
    MapToServerSockets;
  MapToServerSockets ServerSockets;
};

vtkStandardNewMacro(vtkTCPNetworkAccessManager);
//----------------------------------------------------------------------------
vtkTCPNetworkAccessManager::vtkTCPNetworkAccessManager()
{
  this->Internals = new vtkInternals();
  this->AbortPendingConnectionFlag = false;

  // It's essential to initialize the socket controller to initialize sockets on
  // Windows.
  vtkSocketController* controller =  vtkSocketController::New();
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
  vtksys::RegularExpression
    re_connect("^tcp://([^:]+)?:([0-9]+)\\?\?((&?[a-zA-Z0-9%]+=[^&]+)*)");
  vtksys::RegularExpression key_val("([a-zA-Z0-9%]+)=([^&]+)");

  std::map<std::string, std::string> parameters;
  if (re_connect.find(url))
    {
    std::string hostname = re_connect.match(1);
    int port = atoi(re_connect.match(2).c_str());

    // there some issue with RegularExpression that I cannot extract parameters.
    // hence we do this:
    std::vector<vtksys::String> param_vals =
      vtksys::SystemTools::SplitString(re_connect.match(3).c_str(), '&');
    for (size_t cc=0; cc < param_vals.size(); cc++)
      {
      if (key_val.find(param_vals[cc]))
        {
        std::string key = key_val.match(1);
        std::string value = key_val.match(2);
        parameters[key] = value;
        }
      }

    const char* handshake = NULL;
    if (parameters.find("handshake") != parameters.end())
      {
      handshake = parameters["handshake"].c_str();
      }

    if (parameters["listen"] == "true" &&
        parameters["multiple"] == "true")
      {
      return this->WaitForConnection(port, false, handshake,
        parameters["nonblocking"] == "true");
      }
    else if (parameters["listen"] == "true")
      {
      return this->WaitForConnection(port, true, handshake,
        parameters["nonblocking"] == "true");
      }
    else
      {
      return this->ConnectToRemote(hostname.c_str(), port, handshake);
      }
    }
  else
    {
    vtkErrorMacro("Malformed URL: " << (url? url : "(empty)"));
    }

  return NULL;
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

  vtkSocketController* ctrlWithBufferToEmpty = NULL;
  int size=0;
  vtkInternals::VectorOfControllers::iterator iter1;
  for (iter1 = this->Internals->Controllers.begin();
    iter1 != this->Internals->Controllers.end(); ++iter1)
    {
    vtkSocketController* controller = iter1->GetPointer();
    if (!controller)
      {
      // skip null controllers.
      continue;
      }
    vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
      controller->GetCommunicator());
    vtkSocket* socket = comm->GetSocket();
    if (socket && socket->GetConnected())
      {
      sockets_to_select[size] = socket->GetSocketDescriptor();
      controller_or_server_socket[size] = controller;
      if(comm->HasBufferredMessages())
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
    if (iter2->second.GetPointer() &&
      iter2->second.GetPointer()->GetConnected())
      {
      sockets_to_select[size] =
        iter2->second.GetPointer()->GetSocketDescriptor();
      controller_or_server_socket[size] = iter2->second.GetPointer();
      size++;
      }
    }

  if (size == 0 || this->AbortPendingConnectionFlag)
    {
    return -1;
    }

  // Try to empty RMI buffered messages if any
  if(ctrlWithBufferToEmpty && (ctrlWithBufferToEmpty->ProcessRMIs(0,1) ==
                               vtkMultiProcessController::RMI_NO_ERROR))
    {
    return 1;
    }

  int selected_index = -1;
  int result = vtkSocket::SelectSockets(sockets_to_select, size,
                                        timeout_msecs, &selected_index);
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
    int port= ss->GetServerPort();
    this->InvokeEvent(vtkCommand::ConnectionCreatedEvent, &port);
    return 1;
    }
  else
    {
    // We use smart pointer here to make sure the controller will live
    // during the whole ProcessRMIs call. As that call can release
    // the controller while executing.
    vtkSmartPointer<vtkMultiProcessController> controller =
      vtkMultiProcessController::SafeDownCast(
        controller_or_server_socket[selected_index]);
    result = controller->ProcessRMIs(0, 1);
    if (result == vtkMultiProcessController::RMI_NO_ERROR)
      {
      // all's well.
      return 1;
      }

    // Close cleanly the socket in error
    vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
        controller->GetCommunicator());
    comm->CloseConnection();

    return can_quit_if_error ? -1 /* Quit */ :
                                1 /* Pretend it's OK */;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkTCPNetworkAccessManager::ConnectToRemote(
  const char* hostname, int port, const char* handshake)
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
    if (timer->GetElapsedTime() > 60.0)
      {
      vtkErrorMacro(<< "Connect timeout.");
      return NULL;
      }
    vtkWarningMacro(<< "Connect failed.  Retrying for "
      << (60.0 - timer->GetElapsedTime()) << " more seconds.");
    vtksys::SystemTools::Delay(1000);
    }

  vtkSocketController* controller = vtkSocketController::New();
  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
    controller->GetCommunicator());
#if GENERATE_DEBUG_LOG
  vtksys_ios::ostringstream mystr;
  mystr << "/tmp/client."<< getpid() << ".log";
  comm->LogToFile(mystr.str().c_str());
#endif
  comm->SetSocket(cs);
  if (!comm->Handshake() ||
    !this->ParaViewHandshake(controller, false, handshake))
    {
    controller->Delete();
    vtkErrorMacro("Failed to connect to " << hostname << ":" << port
      << ". Client-Server Handshake failed. Please verify that the client and"
      << " server versions are compatible with each other");
    return NULL;
    }
  this->Internals->Controllers.push_back(controller);
  return controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkTCPNetworkAccessManager::WaitForConnection(
  int port, bool once, const char* handshake, bool nonblocking)
{
  vtkServerSocket* server_socket = NULL;
  if (this->Internals->ServerSockets.find(port) !=
    this->Internals->ServerSockets.end())
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
      return NULL;
      }
    this->Internals->ServerSockets[port] = server_socket;
    server_socket->FastDelete();
    }

  vtksys::SystemInformation sys_info;
  sys_info.RunOSCheck();
  const char* sys_hostname = sys_info.GetHostname()?
    sys_info.GetHostname() : "localhost";

  // print out a status message.
  cout << "Accepting connection(s): " << sys_hostname << ":"
      << server_socket->GetServerPort() << endl;

  this->AbortPendingConnectionFlag = false;
  vtkSocketController* controller = NULL;

  while (this->AbortPendingConnectionFlag == false && controller == NULL)
    {
    vtkClientSocket* client_socket = NULL;
    if (nonblocking)
      {
      client_socket = server_socket->WaitForConnection(100);
      }
    else
      {
      while (this->AbortPendingConnectionFlag == false &&
        ((client_socket = server_socket->WaitForConnection(1000)) == NULL))
        {
        double progress=0.5;
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }
    if (!client_socket)
      {
      return NULL;
      }

    controller = vtkSocketController::New();
    vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
      controller->GetCommunicator());
    comm->SetSocket(client_socket);
    client_socket->FastDelete();
    if (comm->Handshake()==0 ||
      !this->ParaViewHandshake(controller, true, handshake))
      {
      controller->Delete();
      controller = NULL;
      // handshake failed, must be bogus client, continue waiting (unless
      // this->AbortPendingConnectionFlag == true).
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


//----------------------------------------------------------------------------
bool vtkTCPNetworkAccessManager::ParaViewHandshake(
  vtkMultiProcessController* controller, bool server_side, const char* handshake)
{
  if (server_side)
    {
    int size = handshake? static_cast<int>(strlen(handshake)+1) : -1;

    int othersize;
    char* other_handshake = NULL;

    controller->Receive(&othersize, 1, 1, 99991);

    if (othersize > 0)
      {
      other_handshake = new char[othersize];
      controller->Receive(other_handshake, othersize, 1, 99991);
      }

    int accept = (size == othersize &&
     (size == -1 || strcmp(handshake, other_handshake) == 0))? 1 : 0;
    controller->Send(&accept, 1, 1, 99990);
    delete []other_handshake;
    return (accept == 1);
    }
  else
    {
    int size = handshake? static_cast<int>(strlen(handshake)+1) : -1;
    controller->Send(&size, 1, 1, 99991);
    if (size > 0)
      {
      controller->Send(handshake, size, 1, 99991);
      }
    int accept;
    controller->Receive(&accept, 1, 1, 99990);
    return (accept == 1);
    }
}

//----------------------------------------------------------------------------
void vtkTCPNetworkAccessManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
