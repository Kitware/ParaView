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

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkPVConfig.h"
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
  this->AbortConnect = false;
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
}

//----------------------------------------------------------------------------
bool vtkSMSessionClient::Connect(const char* url)
{
  vtksys::RegularExpression pvserver("^cs://([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvserver_reverse ("^csrc://([^:]+)?(:([0-9]+))?");

  vtksys::RegularExpression pvrenderserver(
    "^cdsrs://([^:]+)(:([0-9]+))?/([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvrenderserver_reverse (
    "^cdsrsrc://(([^:]+)?(:([0-9]+))?/([^:]+)?(:([0-9]+))?)?");

  vtksys_ios::ostringstream handshake;
  handshake << "handshake=paraview." << PARAVIEW_VERSION_FULL;
  // Add connect-id if needed (or maybe we extract that from url as well (just
  // like vtkNetworkAccessManager).

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
    int dsport = atoi(pvrenderserver.match(3).c_str());
    dsport = (dsport == 0)? 11111 : dsport;

    vtkstd::string renderserverhost = pvrenderserver.match(4);
    int rsport = atoi(pvrenderserver.match(6).c_str());
    rsport = (rsport == 0)? 22221 : rsport;

    vtksys_ios::ostringstream stream;
    stream << "tcp://" << dataserverhost << ":" << dsport
      << "?" << handshake.str();
    data_server_url = stream.str().c_str();

    stream.clear();
    stream << "tcp://" << renderserverhost << ":" << rsport
      << "?" << handshake.str();
    render_server_url = stream.str();
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
    vtkProcessModule2::GetProcessModule()->GetNetworkAccessManager();
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
  // this->SetupDataServerRenderServerConnection();

  return (this->DataServerController && (!need_rcontroller||
      this->RenderServerController));
}

//----------------------------------------------------------------------------
bool vtkSMSessionClient::GetIsAlive()
{
  // TODO: add check to test connection existence.
  return (this->DataServerController != NULL);
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
void vtkSMSessionClient::PushState(vtkSMMessage* msg)
{
  if (this->RenderServerController == NULL)
    {
    // re-route all render-server messages to data-server.
    if (msg->location() & vtkProcessModule2::RENDER_SERVER)
      {
      msg->set_location(msg->location() | vtkProcessModule2::DATA_SERVER);
      msg->set_location(msg->location() & ~vtkProcessModule2::RENDER_SERVER);
      }
    if (msg->location() & vtkProcessModule2::RENDER_SERVER_ROOT)
      {
      msg->set_location(msg->location() | vtkProcessModule2::DATA_SERVER_ROOT);
      msg->set_location(msg->location() & ~vtkProcessModule2::RENDER_SERVER_ROOT);
      }
    }

  if ( (msg->location() & vtkProcessModule2::DATA_SERVER) != 0 ||
    (msg->location() & vtkProcessModule2::DATA_SERVER_ROOT) != 0)
    {
    // FIXME integrate the new protobuf transport layer
    // FIXME xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    vtkMultiProcessStream stream;
    stream << static_cast<int>(PUSH);
    stream << msg->SerializeAsString();
//    stream << static_cast<int>(PUSH) << servers << objectid << state;
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->DataServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);
    }

  if (this->RenderServerController != NULL &&
    (msg->location() & vtkProcessModule2::RENDER_SERVER) != 0 ||
    (msg->location() & vtkProcessModule2::RENDER_SERVER_ROOT) != 0)
    {
    // FIXME integrate the new protobuf transport layer
    // FIXME xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    vtkMultiProcessStream stream;
    stream << static_cast<int>(PUSH);
    stream << msg->SerializeAsString();
    // stream << PUSH << servers << objectid << state;
    vtkstd::vector<unsigned char> raw_message;
    stream.GetRawData(raw_message);
    this->RenderServerController->TriggerRMIOnAllChildren(
      &raw_message[0], static_cast<int>(raw_message.size()),
      CLIENT_SERVER_MESSAGE_RMI);
    }

  if ( (msg->location() & vtkProcessModule2::CLIENT) != 0)
    {
    this->Superclass::PushState(msg);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
