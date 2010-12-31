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
#include "vtkSMSessionClient.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVServerInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSocketCommunicator.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

#include <assert.h>

vtkStandardNewMacro(vtkSMSessionClient);
vtkCxxSetObjectMacro(vtkSMSessionClient, RenderServerController,
  vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkSMSessionClient, DataServerController,
  vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkSMSessionClient::vtkSMSessionClient()
{
  // This session can only be created on the client.
  this->RenderServerController = NULL;
  this->DataServerController = NULL;
  this->URI = NULL;
  this->AbortConnect = false;

  this->DataServerInformation = vtkPVServerInformation::New();
  this->RenderServerInformation = vtkPVServerInformation::New();
  this->ServerInformation = vtkPVServerInformation::New();
}

//----------------------------------------------------------------------------
vtkSMSessionClient::~vtkSMSessionClient()
{
  if (this->GetIsAlive())
    {
    this->CloseSession();
    }
  this->SetRenderServerController(0);
  this->SetDataServerController(0);
  this->DataServerInformation->Delete();
  this->RenderServerInformation->Delete();
  this->ServerInformation->Delete();
  this->SetURI(0);
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkSMSessionClient::GetController(ServerFlags processType)
{
  switch (processType)
    {
  case CLIENT:
    return NULL;

  case DATA_SERVER:
  case DATA_SERVER_ROOT:
    return this->DataServerController;

  case RENDER_SERVER:
  case RENDER_SERVER_ROOT:
    return (this->RenderServerController? this->RenderServerController :
      this->DataServerController);

  default:
    vtkWarningMacro("Invalid processtype of GetController(): " << processType);
    }

  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSMSessionClient::Connect(const char* url)
{
  this->SetURI(url);
  vtksys::RegularExpression pvserver("^cs://([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvserver_reverse ("^csrc://([^:]+)?(:([0-9]+))?");

  vtksys::RegularExpression pvrenderserver(
    "^cdsrs://([^:]+):([0-9]+)/([^:]+):([0-9]+)");
  vtksys::RegularExpression pvrenderserver_reverse (
    "^cdsrsrc://(([^:]+)?(:([0-9]+))?/([^:]+)?(:([0-9]+))?)?");

  vtksys_ios::ostringstream handshake;
  handshake << "handshake=paraview." << PARAVIEW_VERSION_FULL;
  // Add connect-id if needed (or maybe we extract that from url as well
  // (just like vtkNetworkAccessManager).

  vtkstd::string data_server_url;
  vtkstd::string render_server_url;

  bool using_reverse_connect = false;
  if (pvserver.find(url))
    {
    vtkstd::string hostname = pvserver.match(1);
    int port = atoi(pvserver.match(3).c_str());
    port = (port == 0)? 11111: port;

    vtksys_ios::ostringstream stream;
    stream << "tcp://" << hostname << ":" << port << "?" << handshake.str();
    data_server_url = stream.str();
    }
  else if (pvserver_reverse.find(url))
    {
    int port = atoi(pvserver_reverse.match(3).c_str());
    port = (port == 0)? 11111: port;
    vtksys_ios::ostringstream stream;
    stream << "tcp://localhost:" << port << "?listen=true&nonblocking=true&" << handshake.str();
    data_server_url = stream.str();

    using_reverse_connect = true;
    }
  else if (pvrenderserver.find(url))
    {
    vtkstd::string dataserverhost = pvrenderserver.match(1);
    int dsport = atoi(pvrenderserver.match(2).c_str());
    dsport = (dsport == 0)? 11111 : dsport;

    vtkstd::string renderserverhost = pvrenderserver.match(3);
    int rsport = atoi(pvrenderserver.match(4).c_str());
    rsport = (rsport == 0)? 22221 : rsport;

    vtksys_ios::ostringstream stream;
    stream << "tcp://" << dataserverhost << ":" << dsport
      << "?" << handshake.str();
    data_server_url = stream.str().c_str();

    vtksys_ios::ostringstream stream2;
    stream2 << "tcp://" << renderserverhost << ":" << rsport
      << "?" << handshake.str();
    render_server_url = stream2.str();
    }
  else if (pvrenderserver_reverse.find(url))
    {
    int dsport = atoi(pvrenderserver_reverse.match(4).c_str());
    dsport = (dsport == 0)? 11111 : dsport;
    int rsport = atoi(pvrenderserver_reverse.match(7).c_str());
    rsport = (rsport == 0)? 22221 : rsport;

    vtksys_ios::ostringstream stream;
    stream << "tcp://localhost:" << dsport
      << "?listen=true&nonblocking=true&" << handshake.str();
    data_server_url = stream.str().c_str();

    stream.clear();
    stream << "tcp://localhost:" << rsport
      << "?listen=true&nonblocking=true&" << handshake.str();
    render_server_url = stream.str();
    using_reverse_connect = true;
    }

  bool need_rcontroller = render_server_url.size() > 0;
  vtkNetworkAccessManager* nam =
    vtkProcessModule::GetProcessModule()->GetNetworkAccessManager();
  vtkMultiProcessController* dcontroller =
    nam->NewConnection(data_server_url.c_str());
  vtkMultiProcessController* rcontroller = need_rcontroller?
    nam->NewConnection(render_server_url.c_str()) : NULL;

  this->AbortConnect = false;
  while (!this->AbortConnect &&
    (dcontroller == NULL || (need_rcontroller && rcontroller == NULL)))
    {
    int result = nam->ProcessEvents(1000);
    if (result == 1) // some activity
      {
      dcontroller = dcontroller? dcontroller :
        nam->NewConnection(data_server_url.c_str());
      rcontroller = (rcontroller || !need_rcontroller)? rcontroller :
        nam->NewConnection(render_server_url.c_str());
      }
    else if (result == 0) // timeout
      {
      double foo=0.5;
      this->InvokeEvent(vtkCommand::ProgressEvent, &foo);
      }
    else if (result == -1)
      {
      vtkErrorMacro("Some error in socket processing.");
      break;
      }
    }
  if (dcontroller)
    {
    this->SetDataServerController(dcontroller);
    dcontroller->Delete();
    }
  if (rcontroller)
    {
    this->SetRenderServerController(rcontroller);
    rcontroller->Delete();
    }

  bool success = (this->DataServerController && (!need_rcontroller||
      this->RenderServerController));

  if (success)
    {
    this->GatherInformation(vtkProcessModule::DATA_SERVER_ROOT,
      this->DataServerInformation, 0);
    this->GatherInformation(vtkProcessModule::RENDER_SERVER_ROOT,
      this->RenderServerInformation, 0);

    // Keep the combined server information to return when
    // GetServerInformation() is called.
    this->ServerInformation->AddInformation(this->RenderServerInformation);
    this->ServerInformation->AddInformation(this->DataServerInformation);
    }

  // TODO: test with following expressions.
  // vtkSMSessionClient::Connect("cs://localhost");
  // vtkSMSessionClient::Connect("cs://localhost:2212");
  // vtkSMSessionClient::Connect("csrc://:2212");
  // vtkSMSessionClient::Connect("csrc://");
  // vtkSMSessionClient::Connect("csrc://localhost:2212");


  // vtkSMSessionClient::Connect("cdsrs://localhost/localhost");
  // vtkSMSessionClient::Connect("cdsrs://localhost:99999/localhost");
  // vtkSMSessionClient::Connect("cdsrs://localhost/localhost:99999");
  // vtkSMSessionClient::Connect("cdsrs://localhost:66666/localhost:99999");

  // vtkSMSessionClient::Connect("cdsrsrc://");
  // vtkSMSessionClient::Connect("cdsrsrc://localhost:2212/:23332");
  // vtkSMSessionClient::Connect("cdsrsrc://:2212/:23332");
  // vtkSMSessionClient::Connect("cdsrsrc:///:23332");

  // TODO:
  // Setup the socket connnection between data-server and render-server.
  if (success && this->DataServerController && this->RenderServerController)
    {
    this->SetupDataServerRenderServerConnection();
    }

  // Make sure that the client as the server XML definition
  if(success)
    {
    this->GetProxyManager()->LoadXMLDefinitionFromServer();
    }

  return success;
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::SetupDataServerRenderServerConnection()
{
  vtkSMProxy* mpiMToN = this->GetProxyManager()->NewProxy(
    "internals", "MPIMToNSocketConnection");
  vtkSMPropertyHelper(mpiMToN, "WaitingProcess").Set(
    vtkProcessModule::PROCESS_RENDER_SERVER);
  mpiMToN->UpdateVTKObjects();

  vtkMPIMToNSocketConnectionPortInformation* info =
    vtkMPIMToNSocketConnectionPortInformation::New();
  this->GatherInformation(RENDER_SERVER, info, mpiMToN->GetGlobalID());

  vtkSMPropertyHelper helper(mpiMToN, "Connections");
  for (int cc = 0; cc < info->GetNumberOfConnections(); cc++)
    {
    vtksys_ios::ostringstream processNo;
    processNo << cc;
    vtksys_ios::ostringstream str;
    str << info->GetProcessPort(cc);
    helper.Set(3*cc, processNo.str().c_str());
    helper.Set(3*cc+1, str.str().c_str());
    helper.Set(3*cc+2, info->GetProcessHostName(cc));
    }
  mpiMToN->UpdateVTKObjects();
  mpiMToN->InvokeCommand("Connect");

  this->GetProxyManager()->RegisterProxy("__internals__",
    "m2n_socket",
    mpiMToN);
  mpiMToN->Delete();

  // Now let the server-side sessions know what's the vtkMPIMToNSocketConnection
  // instance to use for data-server to render-server connection.
  vtkMultiProcessStream stream;
  stream << static_cast<int>(REGISTER_MTON_SOCKET_CONNECTION)
         << mpiMToN->GetGlobalID();
  vtkstd::vector<unsigned char> raw_message;
  stream.GetRawData(raw_message);
  this->DataServerController->TriggerRMIOnAllChildren(
    &raw_message[0], static_cast<int>(raw_message.size()),
    CLIENT_SERVER_MESSAGE_RMI);
  this->RenderServerController->TriggerRMIOnAllChildren(
    &raw_message[0], static_cast<int>(raw_message.size()),
    CLIENT_SERVER_MESSAGE_RMI);
}

//----------------------------------------------------------------------------
bool vtkSMSessionClient::GetIsAlive()
{
  // TODO: add check to test connection existence.
  return (this->DataServerController != NULL);
}

namespace
{
  template <class T>
  T vtkMax(const T& a, const T& b) { return (a < b)? b: a; }
};

//----------------------------------------------------------------------------
int vtkSMSessionClient::GetNumberOfProcesses(vtkTypeUInt32 servers)
{
  int num_procs = 0;
  if (servers & vtkProcessModule::CLIENT)
    {
    num_procs = vtkMax(num_procs,
      this->Superclass::GetNumberOfProcesses(servers));
    }
  if (servers & vtkProcessModule::DATA_SERVER ||
    servers & vtkProcessModule::DATA_SERVER_ROOT)
    {
    num_procs = vtkMax(num_procs,
      this->DataServerInformation->GetNumberOfProcesses());
    }

  if (servers & vtkProcessModule::RENDER_SERVER ||
    servers & vtkProcessModule::RENDER_SERVER_ROOT)
    {
    num_procs = vtkMax(num_procs,
      this->RenderServerInformation->GetNumberOfProcesses());
    }

  return num_procs;
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::CloseSession()
{
  if (this->DataServerController)
    {
    this->DataServerController->TriggerRMIOnAllChildren(
      CLOSE_SESSION);
    vtkSocketCommunicator::SafeDownCast(
      this->DataServerController->GetCommunicator())->CloseConnection();
    this->SetDataServerController(0);
    }
  if (this->RenderServerController)
    {
    this->RenderServerController->TriggerRMIOnAllChildren(
      CLOSE_SESSION);
    vtkSocketCommunicator::SafeDownCast(
      this->RenderServerController->GetCommunicator())->CloseConnection();
    this->SetRenderServerController(0);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::PushState(vtkSMMessage* message)
{
  if (this->RenderServerController == NULL)
    {
    // re-route all render-server messages to data-server.
    if (message->location() & vtkProcessModule::RENDER_SERVER)
      {
      message->set_location(message->location() | vtkProcessModule::DATA_SERVER);
      message->set_location(message->location() & ~vtkProcessModule::RENDER_SERVER);
      }
    if (message->location() & vtkProcessModule::RENDER_SERVER_ROOT)
      {
      message->set_location(message->location() | vtkProcessModule::DATA_SERVER_ROOT);
      message->set_location(message->location() & ~vtkProcessModule::RENDER_SERVER_ROOT);
      }
    }

  if ( (message->location() & vtkProcessModule::DATA_SERVER) != 0 ||
    (message->location() & vtkProcessModule::DATA_SERVER_ROOT) != 0)
    {
    vtkMultiProcessStream stream;
    stream << static_cast<int>(PUSH);
    stream << message->SerializeAsString();
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->DataServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);
    }

  if (this->RenderServerController != NULL &&
    ((message->location() & vtkProcessModule::RENDER_SERVER) != 0 ||
    (message->location() & vtkProcessModule::RENDER_SERVER_ROOT) != 0))
    {
    vtkMultiProcessStream stream;
    stream << static_cast<int>(PUSH);
    stream << message->SerializeAsString();
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->RenderServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);
    }

  if ( (message->location() & vtkProcessModule::CLIENT) != 0)
    {
    this->Superclass::PushState(message);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::PullState(vtkSMMessage* message)
{
  if (this->RenderServerController == NULL)
    {
    // re-route all render-server messages to data-server.
    if (message->location() & vtkProcessModule::RENDER_SERVER)
      {
      message->set_location(message->location() | vtkProcessModule::DATA_SERVER);
      message->set_location(message->location() & ~vtkProcessModule::RENDER_SERVER);
      }
    if (message->location() & vtkProcessModule::RENDER_SERVER_ROOT)
      {
      message->set_location(message->location() | vtkProcessModule::DATA_SERVER_ROOT);
      message->set_location(message->location() & ~vtkProcessModule::RENDER_SERVER_ROOT);
      }
    }

  // We make sure that only ONE location is targeted with a priority order
  // (1) Client (2) DataServer (3) RenderServer
  if ( (message->location() & vtkProcessModule::CLIENT) != 0)
    {
    this->Superclass::PullState(message);
    // Everything is local no communication needed (Send/Reply)
    }
  else if ( (message->location() & vtkProcessModule::DATA_SERVER) != 0 ||
    (message->location() & vtkProcessModule::DATA_SERVER_ROOT) != 0)
    {
    vtkMultiProcessStream stream;
    stream << static_cast<int>(PULL);
    stream << message->SerializeAsString();
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->DataServerController->TriggerRMIOnAllChildren(
        &raw_message[0], static_cast<int>(raw_message.size()),
        CLIENT_SERVER_MESSAGE_RMI);

    // Get the reply
    vtkMultiProcessStream replyStream;
    this->DataServerController->Receive(replyStream, 1, REPLY_PULL);
    vtkstd::string string;
    replyStream >> string;
    message->ParseFromString(string);
    }
  else if (this->RenderServerController != NULL &&
           ((message->location() & vtkProcessModule::RENDER_SERVER) != 0 ||
            (message->location() & vtkProcessModule::RENDER_SERVER_ROOT) != 0))
    {
    vtkMultiProcessStream stream;
    stream << static_cast<int>(PULL);
    stream << message->SerializeAsString();
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->RenderServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);

    // Get the reply
    vtkMultiProcessStream replyStream;
    this->DataServerController->Receive(replyStream, 1, REPLY_PULL);
    vtkstd::string string;
    replyStream >> string;
    message->ParseFromString(string);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::Invoke(vtkSMMessage* message)
{
  if (this->RenderServerController == NULL)
    {
    // re-route all render-server messages to data-server.
    if (message->location() & vtkProcessModule::RENDER_SERVER)
      {
      message->set_location(message->location() | vtkProcessModule::DATA_SERVER);
      message->set_location(message->location() & ~vtkProcessModule::RENDER_SERVER);
      }
    if (message->location() & vtkProcessModule::RENDER_SERVER_ROOT)
      {
      message->set_location(message->location() | vtkProcessModule::DATA_SERVER_ROOT);
      message->set_location(message->location() & ~vtkProcessModule::RENDER_SERVER_ROOT);
      }
    }

  if ( (message->location() & vtkProcessModule::DATA_SERVER) != 0 ||
    (message->location() & vtkProcessModule::DATA_SERVER_ROOT) != 0)
    {
    vtkMultiProcessStream stream;
    stream << static_cast<int>(INVOKE);
    stream << message->SerializeAsString();
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->DataServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);
    }

  if (this->RenderServerController != NULL &&
    ((message->location() & vtkProcessModule::RENDER_SERVER) != 0 ||
    (message->location() & vtkProcessModule::RENDER_SERVER_ROOT) != 0))
    {
    vtkMultiProcessStream stream;
    stream << static_cast<int>(INVOKE);
    stream << message->SerializeAsString();
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->RenderServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);
    }

  if ( (message->location() & vtkProcessModule::CLIENT) != 0)
    {
    this->Superclass::Invoke(message);
    }
}

//----------------------------------------------------------------------------
bool vtkSMSessionClient::GatherInformation(
  vtkTypeUInt32 location, vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  if (this->RenderServerController == NULL)
    {
    // re-route all render-server messages to data-server.
    if (location & vtkProcessModule::RENDER_SERVER)
      {
      location |= vtkProcessModule::DATA_SERVER;
      location &= ~vtkProcessModule::RENDER_SERVER;
      }
    if (location & vtkProcessModule::RENDER_SERVER_ROOT)
      {
      location |= vtkProcessModule::DATA_SERVER_ROOT;
      location &= ~vtkProcessModule::RENDER_SERVER_ROOT;
      }
    }

  if ( (location & vtkProcessModule::CLIENT) != 0)
    {
    bool ret_value = this->Superclass::GatherInformation(
      location, information, globalid);
    if (information->GetRootOnly())
      {
      return ret_value;
      }
    }

  vtkMultiProcessStream stream;
  stream << static_cast<int>(GATHER_INFORMATION)
    << location
    << information->GetClassName()
    << globalid;
  information->CopyParametersToStream(stream);
  vtkstd::vector<unsigned char> raw_message;
  stream.GetRawData(raw_message);

  vtkMultiProcessController* controller = NULL;

  if ( (location & vtkProcessModule::DATA_SERVER) != 0 ||
    (location & vtkProcessModule::DATA_SERVER_ROOT) != 0)
    {
    controller = this->DataServerController;
    }

  else if (this->RenderServerController != NULL &&
    ((location & vtkProcessModule::RENDER_SERVER) != 0 ||
    (location & vtkProcessModule::RENDER_SERVER_ROOT) != 0))
    {
    controller = this->RenderServerController;
    }

  if (controller)
    {
    controller->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);

    int length2 = 0;
    controller->Receive(&length2, 1, 1, REPLY_GATHER_INFORMATION_TAG);
    if (length2 <= 0)
      {
      vtkErrorMacro("Server failed to gather information.");
      return false;
      }
    unsigned char* data2 = new unsigned char[length2];
    if (!controller->Receive((char*)data2, length2, 1,
        REPLY_GATHER_INFORMATION_TAG))
      {
      vtkErrorMacro("Failed to receive information correctly.");
      delete [] data2;
      return false;
      }
    vtkClientServerStream csstream;
    csstream.SetData(data2, length2);
    information->CopyFromStream(&csstream);
    delete [] data2;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
