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
#include "vtkSMSessionServer.h"

#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkPVConfig.h"
#include "vtkSMSessionClient.h"
#include "vtkSocketCommunicator.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

#include <assert.h>


namespace
{
  void RMICallback(void *localArg,
    void *remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
    {
    vtkSMSessionServer* self = reinterpret_cast<vtkSMSessionServer*>(localArg);
    self->OnClientServerMessageRMI(remoteArg, remoteArgLength);
    }

  void CloseSessionCallback(void *localArg,
    void *vtkNotUsed(remoteArg), int vtkNotUsed(remoteArgLength),
    int vtkNotUsed(remoteProcessId))
    {
    vtkSMSessionServer* self = reinterpret_cast<vtkSMSessionServer*>(localArg);
    self->OnCloseSessionRMI();
    }
};

vtkStandardNewMacro(vtkSMSessionServer);
//----------------------------------------------------------------------------
vtkSMSessionServer::vtkSMSessionServer()
{
  this->ClientController = 0;
}

//----------------------------------------------------------------------------
vtkSMSessionServer::~vtkSMSessionServer()
{
  this->SetClientController(0);
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::SetClientController(
  vtkMultiProcessController* controller)
{
  if (this->ClientController == controller)
    {
    return;
    }

  if (this->ClientController)
    {
    this->ClientController->RemoveAllRMICallbacks(
      vtkSMSessionClient::CLIENT_SERVER_MESSAGE_RMI);
    this->ClientController->RemoveAllRMICallbacks(
      vtkSMSessionClient::CLOSE_SESSION);
    }

  vtkSetObjectBodyMacro(
    ClientController, vtkMultiProcessController, controller);

  if (this->ClientController)
    {
    this->ClientController->AddRMICallback(
      &RMICallback, this,
      vtkSMSessionClient::CLIENT_SERVER_MESSAGE_RMI);
    this->ClientController->AddRMICallback(
      &CloseSessionCallback, this,
      vtkSMSessionClient::CLOSE_SESSION);
    }
}

//----------------------------------------------------------------------------
bool vtkSMSessionServer::Connect(const char* url)
{
  vtkNetworkAccessManager* nam =
    vtkProcessModule2::GetProcessModule()->GetNetworkAccessManager();

  vtksys::RegularExpression pvserver("^cs://([^:]+)?(:([0-9]+))?");
  vtksys::RegularExpression pvserver_reverse ("^csrc://([^:]+)(:([0-9]+))?");
  // TODO: handle render-server/data-server urls

  vtksys_ios::ostringstream handshake;
  handshake << "handshake=paraview." << PARAVIEW_VERSION_FULL;
  // Add connect-id if needed (or maybe we extract that from url as well (just
  // like vtkNetworkAccessManager).

  vtkstd::string client_url;
  bool using_reverse_connect = false;
  if (pvserver.find(url))
    {
    int port = atoi(pvserver.match(3).c_str());
    port = (port == 0)? 11111: port;

    vtksys_ios::ostringstream stream;
    stream << "tcp://localhost:" << port << "?listen=true&" << handshake.str();
    client_url = stream.str();
    }
  else if (pvserver_reverse.find(url))
    {
    int port = atoi(pvserver_reverse.match(3).c_str());
    port = (port == 0)? 11111: port;
    vtksys_ios::ostringstream stream;
    stream << "tcp://localhost:" << port << "?listen=true&" << handshake.str();
    client_url = stream.str();

    using_reverse_connect = true;
    }

  vtkMultiProcessController* ccontroller =
    nam->NewConnection(client_url.c_str());
  if (nam)
    {
    this->SetClientController(ccontroller);
    nam->Delete();
    }

  // TODO:
  // Setup the socket connnection between data-server and render-server.
  // this->SetupDataServerRenderServerConnection();

  return (this->ClientController != NULL);
}

//----------------------------------------------------------------------------
bool vtkSMSessionServer::GetIsAlive()
{
  // TODO: check for validity
  return (this->ClientController != NULL);
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::PushState(vtkSMMessage* msg)
{
  this->Superclass::PushState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::OnClientServerMessageRMI(void* message, int message_length)
{
  vtkMultiProcessStream stream;
  stream.SetRawData(reinterpret_cast<const unsigned char*>(message),
    message_length);
  int type;
  stream >> type;
  switch (type)
    {
  case vtkSMSessionClient::PUSH:
      {
      vtkstd::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);
      this->PushState(&msg);
      }
    break;

    }
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::OnCloseSessionRMI()
{
  if (this->GetIsAlive())
    {
    vtkSocketCommunicator::SafeDownCast(
      this->ClientController->GetCommunicator())->CloseConnection();
    this->SetClientController(0);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
