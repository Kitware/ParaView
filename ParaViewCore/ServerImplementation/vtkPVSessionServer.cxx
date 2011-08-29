/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionServer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSessionServer.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkInstantiator.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkObjectFactory.h"
#include "vtkSIProxy.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"
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
    vtkPVSessionServer* self = reinterpret_cast<vtkPVSessionServer*>(localArg);
    self->OnClientServerMessageRMI(remoteArg, remoteArgLength);
    }

  void CloseSessionCallback(void *localArg,
    void *vtkNotUsed(remoteArg), int vtkNotUsed(remoteArgLength),
    int vtkNotUsed(remoteProcessId))
    {
    vtkPVSessionServer* self = reinterpret_cast<vtkPVSessionServer*>(localArg);
    self->OnCloseSessionRMI();
    }
};

vtkStandardNewMacro(vtkPVSessionServer);
//----------------------------------------------------------------------------
vtkPVSessionServer::vtkPVSessionServer()
{
  this->ClientController = 0;
  this->ActivateObserverId = 0;
  this->DeActivateObserverId = 0;
}

//----------------------------------------------------------------------------
vtkPVSessionServer::~vtkPVSessionServer()
{
  this->SetClientController(0);
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVSessionServer::GetController(ServerFlags processType)
{
  switch (processType)
    {
  case CLIENT:
    return this->ClientController;

  default:
    // we shouldn't warn.
    // vtkWarningMacro("Invalid processtype of GetController(): " << processType);
    break;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::SetClientController(
  vtkMultiProcessController* controller)
{
  if (this->ClientController == controller)
    {
    return;
    }

  if (this->ClientController)
    {
    this->ClientController->RemoveAllRMICallbacks(
      vtkPVSessionServer::CLIENT_SERVER_MESSAGE_RMI);
    this->ClientController->RemoveAllRMICallbacks(
      vtkPVSessionServer::CLOSE_SESSION);
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
      vtkPVSessionServer::CLIENT_SERVER_MESSAGE_RMI);
    this->ClientController->AddRMICallback(
      &CloseSessionCallback, this,
      vtkPVSessionServer::CLOSE_SESSION);
    this->ActivateObserverId = this->ClientController->AddObserver(
      vtkCommand::StartEvent, this, &vtkPVSessionServer::Activate);
    this->DeActivateObserverId = this->ClientController->AddObserver(
      vtkCommand::EndEvent, this, &vtkPVSessionServer::DeActivate);
    this->ClientController->GetCommunicator()->AddObserver(
      vtkCommand::WrongTagEvent, this, &vtkPVSessionServer::OnWrongTagEvent);
    }
}

//----------------------------------------------------------------------------
bool vtkPVSessionServer::Connect()
{
  vtksys_ios::ostringstream url;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (pm->GetPartitionId() > 0)
    {
    return true;
    }

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
    vtkErrorMacro("vtkPVSessionServer cannot be created on this process type.");
    return false;
    }

  cout << "Connection URL: " << url.str() << endl;
  return this->Connect(url.str().c_str());
}

//----------------------------------------------------------------------------
bool vtkPVSessionServer::Connect(const char* url)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetPartitionId() > 0)
    {
    return true;
    }


  vtkNetworkAccessManager* nam =
    vtkProcessModule::GetProcessModule()->GetNetworkAccessManager();

  vtksys::RegularExpression pvserver("^cs://([^:]+)?(:([0-9]+))?");
  vtksys::RegularExpression pvserver_reverse ("^csrc://([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvrenderserver(
    "^cdsrs://([^:]+)(:([0-9]+))?/([^:]+)(:([0-9]+))?");
  vtksys::RegularExpression pvrenderserver_reverse (
    "^cdsrsrc://([^:]+)?(:([0-9]+))?/([^:]+)?(:([0-9]+))?");

  vtksys_ios::ostringstream handshake;
  handshake << "handshake=paraview." << PARAVIEW_VERSION;
  // Add connect-id if needed (or maybe we extract that from url as well (just
  // like vtkNetworkAccessManager).

  vtkstd::string client_url;
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
    }

  vtkMultiProcessController* ccontroller =
    nam->NewConnection(client_url.c_str());
  if (ccontroller)
    {
    this->SetClientController(ccontroller);
    ccontroller->Delete();
    cout << "Client connected." << endl;
    }

  return (this->ClientController != NULL);
}

//----------------------------------------------------------------------------
bool vtkPVSessionServer::GetIsAlive()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetPartitionId() > 0)
    {
    return true;
    }

  // TODO: check for validity
  return (this->ClientController != NULL);
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::OnClientServerMessageRMI(void* message, int message_length)
{
  vtkMultiProcessStream stream;
  stream.SetRawData( reinterpret_cast<const unsigned char*>(message),
                     message_length);
  int type;
  stream >> type;
  switch (type)
    {
  case vtkPVSessionServer::PUSH:
      {
      vtkstd::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);
      this->PushState(&msg);
      }
    break;

  case vtkPVSessionServer::PULL:
      {
      vtkstd::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);
      this->PullState(&msg);

      // Send the result back to client
      vtkMultiProcessStream css;
      css << msg.SerializeAsString();
      this->ClientController->Send( css, 1, vtkPVSessionServer::REPLY_PULL);
      }
    break;

  case vtkPVSessionServer::DELETE_SI:
      {
      vtkstd::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);
      this->DeleteSIObject(&msg);
      }
    break;

  case vtkPVSessionServer::EXECUTE_STREAM:
      {
      int ignore_errors, size;
      stream >> ignore_errors >> size;
      unsigned char* css_data = new unsigned char[size+1];
      this->ClientController->Receive(css_data, size, 1,
        vtkPVSessionServer::EXECUTE_STREAM_TAG);
      vtkClientServerStream cssStream;
      cssStream.SetData(css_data, size);
      this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS,
        cssStream, ignore_errors != 0);
      delete [] css_data;
      }
    break;

  case vtkPVSessionServer::LAST_RESULT:
      {
      this->SendLastResultToClient();
      }
    break;

  case vtkPVSessionServer::GATHER_INFORMATION:
      {
      vtkstd::string classname;
      vtkTypeUInt32 location, globalid;
      stream >> location >> classname >> globalid;
      this->GatherInformationInternal(location, classname.c_str(), globalid,
        stream);
      }
    break;

    }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::SendLastResultToClient()
{
  const vtkClientServerStream& reply = this->GetLastResult(
    vtkPVSession::CLIENT_AND_SERVERS);

  const unsigned char* data; size_t size_size_t;
  int size;

  reply.GetData(&data, &size_size_t);
  size = static_cast<int>(size_size_t);

  this->ClientController->Send(&size, 1, 1,
    vtkPVSessionServer::REPLY_LAST_RESULT);
  this->ClientController->Send(data, size, 1,
    vtkPVSessionServer::REPLY_LAST_RESULT);
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::GatherInformationInternal(
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

    this->GatherInformation(location, info, globalid);

    vtkClientServerStream css;
    info->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = static_cast<int>(length);
    this->ClientController->Send(&len, 1, 1,
      vtkPVSessionServer::REPLY_GATHER_INFORMATION_TAG);
    this->ClientController->Send(const_cast<unsigned char*>(data),
      length, 1, vtkPVSessionServer::REPLY_GATHER_INFORMATION_TAG);
    }
  else
    {
    vtkErrorMacro("Could not create information object.");
    // let client know that gather failed.
    int len = 0;
    this->ClientController->Send(&len, 1, 1,
      vtkPVSessionServer::REPLY_GATHER_INFORMATION_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::OnCloseSessionRMI()
{
  if (this->GetIsAlive())
    {
    vtkSocketCommunicator::SafeDownCast(
      this->ClientController->GetCommunicator())->CloseConnection();
    this->SetClientController(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
