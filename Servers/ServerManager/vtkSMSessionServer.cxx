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

#include "vtkClientServerStream.h"
#include "vtkInstantiator.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkSmartPointer.h"
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
bool vtkSMSessionServer::Connect()
{
  vtksys_ios::ostringstream url;

  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();

  switch (pm->GetProcessType())
    {
  case vtkProcessModule2::PROCESS_SERVER:
    url << "cs";
    url << ((options->GetReverseConnection())?  "rc://" : "://");
    url << options->GetClientHostName() << ":" << options->GetServerPort();
    break;

  case vtkProcessModule2::PROCESS_RENDER_SERVER:
  case vtkProcessModule2::PROCESS_DATA_SERVER:
    url << "cdsrs";
    url << ((options->GetReverseConnection())?  "rc://" : "://");
    url << options->GetClientHostName() << ":" << options->GetDataServerPort()
      << "/"
      << options->GetClientHostName() << ":" << options->GetRenderServerPort();
    break;

  default:
    vtkErrorMacro("vtkSMSessionServer cannot be created on this process type.");
    return false;
    }

  cout << "Connection URL: " << url.str() << endl;
  return this->Connect(url.str().c_str());
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
    stream << "tcp://localhost:" << port << "?" << handshake.str();
    client_url = stream.str();

    using_reverse_connect = true;
    }

  vtkMultiProcessController* ccontroller =
    nam->NewConnection(client_url.c_str());
  if (ccontroller)
    {
    this->SetClientController(ccontroller);
    ccontroller->Delete();
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

  case vtkSMSessionClient::GATHER_INFORMATION:
      {
      vtkstd::string classname;
      vtkTypeUInt32 location, globalid;
      stream >> location >> classname >> globalid;
      this->GatherInformationInternal(location, classname.c_str(), globalid);
      }
    break;
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::GatherInformationInternal(
  vtkTypeUInt32 location, const char* classname, vtkTypeUInt32 globalid)
{
  vtkSmartPointer<vtkObject> o;
  o.TakeReference(vtkInstantiator::CreateInstance(classname));

  vtkPVInformation* info = vtkPVInformation::SafeDownCast(o);
  if (info)
    {
    this->Superclass::GatherInformation(location, info, globalid);

    vtkClientServerStream css;
    info->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = static_cast<int>(length);
    this->ClientController->Send(&len, 1, 1,
      vtkSMSessionClient::REPLY_GATHER_INFORMATION_TAG);
    this->ClientController->Send(const_cast<unsigned char*>(data),
      length, 1, vtkSMSessionClient::REPLY_GATHER_INFORMATION_TAG);
    }
  else
    {
    vtkErrorMacro("Could not create information object.");
    // let client know that gather failed.
    int len = 0;
    this->ClientController->Send(&len, 1, 1,
      vtkSMSessionClient::REPLY_GATHER_INFORMATION_TAG);
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
