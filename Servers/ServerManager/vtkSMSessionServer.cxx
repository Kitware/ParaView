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

#include "vtkCommand.h"
#include "vtkClientServerStream.h"
#include "vtkInstantiator.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkObjectFactory.h"
#include "vtkPMProxy.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"
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
  this->MPIMToNSocketConnection = NULL;
  this->ActivateObserverId = 0;
  this->DeActivateObserverId = 0;
}

//----------------------------------------------------------------------------
vtkSMSessionServer::~vtkSMSessionServer()
{
  this->SetClientController(0);
}

//----------------------------------------------------------------------------
vtkSMSessionServer::ServerFlags vtkSMSessionServer::GetProcessRoles()
{
  switch (vtkProcessModule::GetProcessType())
    {
  case vtkProcessModule::PROCESS_SERVER:
    return SERVERS;

  case vtkProcessModule::PROCESS_DATA_SERVER:
    return DATA_SERVER;

  case vtkProcessModule::PROCESS_RENDER_SERVER:
    return RENDER_SERVER;

  default:
    return NONE;
    }
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkSMSessionServer::GetController(ServerFlags processType)
{
  switch (processType)
    {
  case CLIENT:
    return this->ClientController;

  default:
    vtkWarningMacro("Invalid processtype of GetController(): " << processType);
    }
  return NULL;
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
    this->ClientController->RemoveObserver(this->ActivateObserverId);
    this->ClientController->RemoveObserver(this->DeActivateObserverId);
    this->ActivateObserverId = 0;
    this->DeActivateObserverId = 0;
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
    this->ActivateObserverId = this->ClientController->AddObserver(
      vtkCommand::StartEvent, this, &vtkSMSessionServer::Activate);
    this->DeActivateObserverId = this->ClientController->AddObserver(
      vtkCommand::EndEvent, this, &vtkSMSessionServer::DeActivate);
    }
}

//----------------------------------------------------------------------------
bool vtkSMSessionServer::Connect()
{
  vtksys_ios::ostringstream url;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();

  switch (pm->GetProcessType())
    {
  case vtkProcessModule::PROCESS_SERVER:
    url << "cs";
    url << ((options->GetReverseConnection())?  "rc://" : "://");
    url << options->GetClientHostName() << ":" << options->GetServerPort();
    break;

  case vtkProcessModule::PROCESS_RENDER_SERVER:
  case vtkProcessModule::PROCESS_DATA_SERVER:
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
    vtkProcessModule::GetProcessModule()->GetNetworkAccessManager();

  vtksys::RegularExpression pvserver("^cs://([^:]+)?(:([0-9]+))?");
  vtksys::RegularExpression pvserver_reverse ("^csrc://([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvrenderserver(
    "^cdsrs://([^:]+)(:([0-9]+))?/([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvrenderserver_reverse (
    "^cdsrsrc://([^:]+)?(:([0-9]+))?/([^:]+)?(:([0-9]+))?");

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
    vtkstd::string hostname = pvserver_reverse.match(1);
    int port = atoi(pvserver_reverse.match(3).c_str());
    port = (port == 0)? 11111: port;
    vtksys_ios::ostringstream stream;
    stream << "tcp://" << hostname.c_str() << ":" << port << "?" << handshake.str();
    client_url = stream.str();

    using_reverse_connect = true;
    }
  else if (pvrenderserver.find(url))
    {
    int dsport = atoi(pvrenderserver.match(3).c_str());
    dsport = (dsport == 0)? 11111 : dsport;

    int rsport = atoi(pvrenderserver.match(6).c_str());
    rsport = (rsport == 0)? 22221 : rsport;

    if (vtkProcessModule::GetProcessType() ==
      vtkProcessModule::PROCESS_RENDER_SERVER)
      {
      vtksys_ios::ostringstream stream;
      stream << "tcp://localhost:" << rsport << "?listen=true&" << handshake.str();
      client_url = stream.str();
      }
    else if (vtkProcessModule::GetProcessType() ==
      vtkProcessModule::PROCESS_DATA_SERVER)
      {
      vtksys_ios::ostringstream stream;
      stream << "tcp://localhost:" << dsport << "?listen=true&" << handshake.str();
      client_url = stream.str();
      }
    }
  else if (pvrenderserver_reverse.find(url))
    {
    vtkstd::string dataserverhost = pvrenderserver.match(1);
    int dsport = atoi(pvrenderserver.match(3).c_str());
    dsport = (dsport == 0)? 11111 : dsport;

    vtkstd::string renderserverhost = pvrenderserver.match(4);
    int rsport = atoi(pvrenderserver.match(6).c_str());
    rsport = (rsport == 0)? 22221 : rsport;

    if (vtkProcessModule::GetProcessType() ==
      vtkProcessModule::PROCESS_RENDER_SERVER)
      {
      vtksys_ios::ostringstream stream;
      stream << "tcp://" << dataserverhost.c_str() << ":" << rsport << "?" << handshake.str();
      client_url = stream.str();
      }
    else if (vtkProcessModule::GetProcessType() ==
      vtkProcessModule::PROCESS_DATA_SERVER)
      {
      vtksys_ios::ostringstream stream;
      stream << "tcp://" << renderserverhost.c_str() << ":" << dsport << "?" << handshake.str();
      client_url = stream.str();
      }

    using_reverse_connect = true;
    }


  vtkMultiProcessController* ccontroller =
    nam->NewConnection(client_url.c_str());
  if (ccontroller)
    {
    this->SetClientController(ccontroller);
    ccontroller->Delete();
    }

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
  // DEBUG msg->PrintDebugString();
  this->Superclass::PushState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::PullState(vtkSMMessage* msg)
{
  // Only the root node is allowed to do the pull
  if(vtkProcessModule::GetProcessModule()->GetGlobalController()->GetLocalProcessId() != 0)
    return;

  // We are on the root node
  // DEBUG msg->PrintDebugString();
  this->Superclass::PullState(msg);

  // Send the result back to client
  vtkMultiProcessStream css;
  css << msg->SerializeAsString();
  this->ClientController->Send( css, 1, vtkSMSessionClient::REPLY_PULL);
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::OnClientServerMessageRMI(void* message, int message_length)
{
  vtkMultiProcessStream stream;
  stream.SetRawData( reinterpret_cast<const unsigned char*>(message),
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
    case vtkSMSessionClient::PULL:
        {
        vtkstd::string string;
        stream >> string;
        vtkSMMessage msg;
        msg.ParseFromString(string);
        this->PullState(&msg);
        }
      break;

  case vtkSMSessionClient::EXECUTE_STREAM:
      {
      int ignore_errors, size;
      stream >> ignore_errors >> size;
      unsigned char* css_data = new unsigned char[size+1];
      this->ClientController->Receive(css_data, size, 1,
        vtkSMSessionClient::EXECUTE_STREAM_TAG);
      vtkClientServerStream stream;
      stream.SetData(css_data, size);
      this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS,
        stream, ignore_errors != 0);
      delete [] css_data;
      }
    break;

  case vtkSMSessionClient::LAST_RESULT:
      {
      this->SendLastResultToClient();
      }
    break;

  case vtkSMSessionClient::GATHER_INFORMATION:
      {
      vtkstd::string classname;
      vtkTypeUInt32 location, globalid;
      stream >> location >> classname >> globalid;
      this->GatherInformationInternal(location, classname.c_str(), globalid,
        stream);
      }
    break;

  case vtkSMSessionClient::REGISTER_MTON_SOCKET_CONNECTION:
      {
      vtkTypeUInt32 globalid;
      stream >> globalid;
      vtkPMProxy* m2nPMProxy = vtkPMProxy::SafeDownCast(
        this->GetPMObject(globalid));
      assert(m2nPMProxy != NULL);
      this->MPIMToNSocketConnection = vtkMPIMToNSocketConnection::SafeDownCast(
        m2nPMProxy->GetVTKObject());
      }
    break;
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::SendLastResultToClient()
{
  const vtkClientServerStream& reply = this->GetLastResult(
    vtkPVSession::CLIENT_AND_SERVERS);

  const unsigned char* data; size_t size_size_t;
  int size;

  reply.GetData(&data, &size_size_t);
  size = static_cast<int>(size);

  this->ClientController->Send(&size, 1, 1,
    vtkSMSessionClient::REPLY_LAST_RESULT);
  this->ClientController->Send(data, size, 1,
    vtkSMSessionClient::REPLY_LAST_RESULT);
}

//----------------------------------------------------------------------------
void vtkSMSessionServer::GatherInformationInternal(
  vtkTypeUInt32 location, const char* classname, vtkTypeUInt32 globalid,
  vtkMultiProcessStream& stream)
{
  vtkSmartPointer<vtkObject> o;
  o.TakeReference(vtkInstantiator::CreateInstance(classname));

  vtkPVInformation* info = vtkPVInformation::SafeDownCast(o);
  if (info)
    {
    // ensures that the vtkPVInformation has the same ivars locally as on the
    // client.
    info->CopyParametersFromStream(stream);

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
