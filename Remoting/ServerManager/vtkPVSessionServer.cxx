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
#include "vtkClientServerStreamInstantiator.h"
#include "vtkCommand.h"
#include "vtkCompositeMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkPVSessionCore.h"
#include "vtkProcessModule.h"
#include "vtkReservedRemoteObjectIds.h"
#include "vtkSIProxy.h"
#include "vtkSIProxyDefinitionManager.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"

#include <assert.h>
#include <map>
#include <sstream>
#include <string>
#include <string>
#include <vector>
#include <vtksys/RegularExpression.hxx>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
namespace
{
void RMICallback(
  void* localArg, void* remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  vtkPVSessionServer* self = reinterpret_cast<vtkPVSessionServer*>(localArg);
  self->OnClientServerMessageRMI(remoteArg, remoteArgLength);
}

void CloseSessionCallback(void* localArg, void* vtkNotUsed(remoteArg),
  int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
{
  vtkPVSessionServer* self = reinterpret_cast<vtkPVSessionServer*>(localArg);
  self->OnCloseSessionRMI();
}
};
//****************************************************************************/
class vtkPVSessionServer::vtkInternals
{
public:
  vtkInternals(vtkPVSessionServer* owner)
  {
    this->SatelliteServerSession = (vtkProcessModule::GetProcessModule()->GetPartitionId() > 0);
    this->Owner = owner;

    // Attach callbacks
    this->CompositeMultiProcessController->AddRMICallback(
      &RMICallback, this->Owner, vtkPVSessionServer::CLIENT_SERVER_MESSAGE_RMI);

    this->CompositeMultiProcessController->AddRMICallback(
      &CloseSessionCallback, this->Owner, vtkPVSessionServer::CLOSE_SESSION);

    this->CompositeMultiProcessController->AddObserver(
      vtkCompositeMultiProcessController::CompositeMultiProcessControllerChanged, this,
      &vtkPVSessionServer::vtkInternals::ReleaseDeadClientSIObjects);

    this->Owner->GetSessionCore()->GetProxyDefinitionManager()->AddObserver(
      vtkCommand::RegisterEvent, this,
      &vtkPVSessionServer::vtkInternals::CallBackProxyDefinitionManagerHasChanged);
    this->Owner->GetSessionCore()->GetProxyDefinitionManager()->AddObserver(
      vtkCommand::UnRegisterEvent, this,
      &vtkPVSessionServer::vtkInternals::CallBackProxyDefinitionManagerHasChanged);
  }
  //-----------------------------------------------------------------
  void CloseActiveController()
  {
    // FIXME: Maybe we want to keep listening even if no more client is
    // connected.
    if (this->CompositeMultiProcessController->UnRegisterActiveController() == 0)
    {
      vtkProcessModule::GetProcessModule()->GetNetworkAccessManager()->AbortPendingConnection();
    }
  }
  //-----------------------------------------------------------------
  void CreateController(
    vtkObject* vtkNotUsed(src), unsigned long vtkNotUsed(event), void* vtkNotUsed(data))
  {
    vtkNetworkAccessManager* nam = vtkProcessModule::GetProcessModule()->GetNetworkAccessManager();
    vtkSocketController* ccontroller =
      vtkSocketController::SafeDownCast(nam->NewConnection(this->ClientURL.c_str()));
    if (ccontroller)
    {
      ccontroller->GetCommunicator()->AddObserver(
        vtkCommand::WrongTagEvent, this->Owner, &vtkPVSessionServer::OnWrongTagEvent);

      this->CompositeMultiProcessController->RegisterController(ccontroller);
      ccontroller->FastDelete();
    }
  }
  //-----------------------------------------------------------------
  void SetClientURL(const char* client_url) { this->ClientURL = client_url; }
  //-----------------------------------------------------------------
  void SetBaseURL(const char* url) { this->BaseURL = url; }
  //-----------------------------------------------------------------
  std::string ComputeURL(const char* url)
  {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVOptions* options = pm->GetOptions();

    vtksys::RegularExpression pvserver("^cs://([^:]+)?(:([0-9]+))?");
    vtksys::RegularExpression pvserver_reverse("^csrc://([^:]+)(:([0-9]+))?");
    vtksys::RegularExpression pvrenderserver("^cdsrs://([^:]+)(:([0-9]+))?/([^:]+)(:([0-9]+))?");
    vtksys::RegularExpression pvrenderserver_reverse(
      "^cdsrsrc://([^:]+)?(:([0-9]+))?/([^:]+)?(:([0-9]+))?");

    std::ostringstream handshake;
    handshake << "handshake=paraview-" << PARAVIEW_VERSION;
    // Add connect-id if needed. The connect-id is added to the handshake that
    // must match on client and server processes.
    if (options->GetConnectID() != 0)
    {
      handshake << ".connect_id." << options->GetConnectID();
    }
    // Add rendering backend information.
    handshake << ".renderingbackend.opengl2";

    // for forward connections, port number 0 is acceptable, while for
    // reverse-connections it's not.
    std::string client_url;
    if (pvserver.find(url))
    {
      int port = atoi(pvserver.match(3).c_str());
      port = (port < 0) ? 11111 : port;

      std::ostringstream stream;
      stream << "tcp://localhost:" << port << "?listen=true&" << handshake.str();
      stream << ((this->Owner->MultipleConnection && !this->Owner->DisableFurtherConnections)
          ? "&multiple=true"
          : "");
      client_url = stream.str();
    }
    else if (pvserver_reverse.find(url))
    {
      std::string hostname = pvserver_reverse.match(1);
      int port = atoi(pvserver_reverse.match(3).c_str());
      port = (port <= 0) ? 11111 : port;
      std::ostringstream stream;
      stream << "tcp://" << hostname.c_str() << ":" << port << "?" << handshake.str();
      client_url = stream.str();
    }
    else if (pvrenderserver.find(url))
    {
      int dsport = atoi(pvrenderserver.match(3).c_str());
      dsport = (dsport < 0) ? 11111 : dsport;

      int rsport = atoi(pvrenderserver.match(6).c_str());
      rsport = (rsport < 0) ? 22221 : rsport;

      if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
      {
        std::ostringstream stream;
        stream << "tcp://localhost:" << rsport << "?listen=true&" << handshake.str();
        client_url = stream.str();
      }
      else if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_DATA_SERVER)
      {
        std::ostringstream stream;
        stream << "tcp://localhost:" << dsport << "?listen=true&" << handshake.str();
        stream << ((this->Owner->MultipleConnection && !this->Owner->DisableFurtherConnections)
            ? "&multiple=true"
            : "");
        client_url = stream.str();
      }
    }
    else if (pvrenderserver_reverse.find(url))
    {
      std::string dataserverhost = pvrenderserver_reverse.match(1);
      int dsport = atoi(pvrenderserver_reverse.match(3).c_str());
      dsport = (dsport <= 0) ? 11111 : dsport;

      std::string renderserverhost = pvrenderserver_reverse.match(4);
      int rsport = atoi(pvrenderserver_reverse.match(6).c_str());
      rsport = (rsport <= 0) ? 22221 : rsport;

      if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
      {
        std::ostringstream stream;
        stream << "tcp://" << dataserverhost.c_str() << ":" << rsport << "?" << handshake.str();
        client_url = stream.str();
      }
      else if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_DATA_SERVER)
      {
        std::ostringstream stream;
        stream << "tcp://" << renderserverhost.c_str() << ":" << dsport << "?" << handshake.str();
        client_url = stream.str();
      }
    }

    return client_url;
  }

  //-----------------------------------------------------------------
  void UpdateClientURL()
  {
    std::string url = this->ComputeURL(this->BaseURL.c_str());
    this->SetClientURL(url.c_str());
  }

  //-----------------------------------------------------------------
  void SetConnectID(int newConnectID)
  {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVOptions* options = pm->GetOptions();
    if (options->GetConnectID() != newConnectID)
    {
      options->SetConnectID(newConnectID);
      this->UpdateClientURL();
    }
  }
  //-----------------------------------------------------------------
  void NotifyOtherClients(const vtkSMMessage* msgToBroadcast)
  {
    std::string data = msgToBroadcast->SerializeAsString();
    this->CompositeMultiProcessController->TriggerRMI2All(1, (void*)data.c_str(),
      static_cast<int>(data.size()), vtkPVSessionServer::SERVER_NOTIFICATION_MESSAGE_RMI, false);
  }
  //-----------------------------------------------------------------
  void NotifyAllClients(const vtkSMMessage* msgToBroadcast)
  {
    std::string data = msgToBroadcast->SerializeAsString();
    this->CompositeMultiProcessController->TriggerRMI2All(1, (void*)data.c_str(),
      static_cast<int>(data.size()), vtkPVSessionServer::SERVER_NOTIFICATION_MESSAGE_RMI, true);
  }
  //-----------------------------------------------------------------
  vtkCompositeMultiProcessController* GetActiveController()
  {
    if (this->SatelliteServerSession)
    {
      return nullptr;
    }
    return this->CompositeMultiProcessController.GetPointer();
  }
  //-----------------------------------------------------------------
  // Manage share_only message and return true if no processing should occurs
  bool StoreShareOnly(vtkSMMessage* msg)
  {
    if (msg && msg->share_only())
    {
      vtkTypeUInt32 id = msg->global_id();
      this->ShareOnlyCache[id].CopyFrom(*msg);
      return true;
    }
    return false;
  }
  //-----------------------------------------------------------------
  bool IsSatelliteSession() { return this->SatelliteServerSession; }

  //-----------------------------------------------------------------
  // Return true if the message was updated by the ShareOnlyCache
  bool RetreiveShareOnly(vtkSMMessage* msg)
  {
    std::map<vtkTypeUInt32, vtkSMMessage>::iterator iter =
      this->ShareOnlyCache.find(msg->global_id());
    if (iter != this->ShareOnlyCache.end())
    {
      msg->CopyFrom(iter->second);
      return true;
    }
    return false;
  }
  //-----------------------------------------------------------------
  void ReleaseDeadClientSIObjects(
    vtkObject* vtkNotUsed(src), unsigned long vtkNotUsed(event), void* vtkNotUsed(data))
  {
    int nbCtrls = this->CompositeMultiProcessController->GetNumberOfControllers();

    std::vector<int> alivedClients(nbCtrls);
    for (int i = 0; i < nbCtrls; i++)
    {
      alivedClients.push_back(this->CompositeMultiProcessController->GetControllerId(i));
    }
    if (alivedClients.size() > 0)
    {
      this->Owner->SessionCore->GarbageCollectSIObject(
        &alivedClients[0], static_cast<int>(alivedClients.size()));
    }
  }
  //-----------------------------------------------------------------
  void CallBackProxyDefinitionManagerHasChanged(
    vtkObject* vtkNotUsed(src), unsigned long vtkNotUsed(event), void* vtkNotUsed(data))
  {
    vtkSMMessage proxyDefinitionManagerState;
    this->Owner->GetSessionCore()
      ->GetSIObject(vtkReservedRemoteObjectIds::RESERVED_PROXY_DEFINITION_MANAGER_ID)
      ->Pull(&proxyDefinitionManagerState);
    this->NotifyOtherClients(&proxyDefinitionManagerState);
  }

private:
  vtkNew<vtkCompositeMultiProcessController> CompositeMultiProcessController;
  vtkWeakPointer<vtkPVSessionServer> Owner;
  std::string ClientURL;
  std::string BaseURL;
  std::map<vtkTypeUInt32, vtkSMMessage> ShareOnlyCache;
  bool SatelliteServerSession;
};
//****************************************************************************/
vtkStandardNewMacro(vtkPVSessionServer);
//----------------------------------------------------------------------------
vtkPVSessionServer::vtkPVSessionServer()
  : vtkPVSessionBase()
{
  this->Internal = new vtkInternals(this);

  // By default we act as a server for a single client
  this->MultipleConnection = false;
  this->DisableFurtherConnections = false;

  // On server side only one session is available so we just set it Active()
  // forever
  if (vtkProcessModule::GetProcessModule())
  {
    this->Activate();
  }
}

//----------------------------------------------------------------------------
vtkPVSessionServer::~vtkPVSessionServer()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVSessionServer::GetController(ServerFlags processType)
{
  switch (processType)
  {
    case CLIENT:
      return this->Internal->GetActiveController();

    default:
      // we shouldn't warn.
      // vtkWarningMacro("Invalid processtype of GetController(): " << processType);
      break;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVSessionServer::Connect()
{
  std::ostringstream url;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (this->Internal->IsSatelliteSession())
  {
    return true;
  }

  vtkPVServerOptions* options = vtkPVServerOptions::SafeDownCast(pm->GetOptions());
  if (!options)
  {
    vtkErrorMacro("Missing vtkPVServerOptions. "
                  "Process must use vtkPVServerOptions (or subclass).");
    return false;
  }

  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_SERVER:
      if (options->GetReverseConnection())
      {
        url << "csrc://";
        url << options->GetClientHostName() << ":" << options->GetServerPort();
      }
      else
      {
        url << "cs://";
        url << options->GetHostName() << ":" << options->GetServerPort();
      }
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      if (options->GetReverseConnection())
      {
        url << "cdsrsrc://" << options->GetClientHostName() << ":11111" // default ds-port
            << "/" << options->GetClientHostName() << ":" << options->GetServerPort();
      }
      else
      {
        url << "cdsrs://"
            << "<data-server-hostname>:11111"
            << "/" << options->GetHostName() << ":" << options->GetServerPort();
      }
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      if (options->GetReverseConnection())
      {
        url << "cdsrsrc://" << options->GetClientHostName() << ":" << options->GetServerPort()
            << "/" << options->GetClientHostName() << ":22221"; // default rs-port
      }
      else
      {
        url << "cdsrs://" << options->GetHostName() << ":" << options->GetServerPort() << "/"
            << "<render-server-hostname>:22221";
      }
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

  this->Internal->SetBaseURL(url);
  vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
  std::string client_url = this->Internal->ComputeURL(url);
  vtkMultiProcessController* ccontroller = nam->NewConnection(client_url.c_str());
  this->Internal->SetClientURL(client_url.c_str());
  if (ccontroller)
  {
    this->Internal->GetActiveController()->RegisterController(ccontroller);
    ccontroller->FastDelete();
    cout << "Client connected." << endl;
  }

  if (this->MultipleConnection && !this->DisableFurtherConnections &&
    this->Internal->GetActiveController())
  {
    // Listen for new client controller creation
    nam->AddObserver(
      vtkCommand::ConnectionCreatedEvent, this->Internal, &vtkInternals::CreateController);
  }

  return (this->Internal->GetActiveController() != nullptr);
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
  return (this->Internal->GetActiveController() != nullptr);
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::SetDisableFurtherConnections(bool disable)
{
  if (this->DisableFurtherConnections != disable)
  {
    this->DisableFurtherConnections = disable;
    this->Internal->UpdateClientURL();

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
    vtkPVServerOptions* options = vtkPVServerOptions::SafeDownCast(pm->GetOptions());
    int port = options->GetServerPort();
    nam->DisableFurtherConnections(port, disable);

    if (!disable)
    {
      nam->AddObserver(
        vtkCommand::ConnectionCreatedEvent, this->Internal, &vtkInternals::CreateController);
    }
    else
    {
      nam->RemoveObservers(vtkCommand::ConnectionCreatedEvent);
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::SetConnectID(int newConnectID)
{
  this->Internal->SetConnectID(newConnectID);
}

//----------------------------------------------------------------------------
int vtkPVSessionServer::GetConnectID()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();
  return options->GetConnectID();
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::OnClientServerMessageRMI(void* message, int message_length)
{
  vtkMultiProcessStream stream;
  stream.SetRawData(reinterpret_cast<const unsigned char*>(message), message_length);
  int type;
  stream >> type;
  switch (type)
  {
    case vtkPVSessionServer::PUSH:
    {
      std::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);

      //      cout << "=================================" << endl;
      //      msg.PrintDebugString();
      //      cout << "=================================" << endl;

      // Do we skip the processing ?
      if (!this->Internal->StoreShareOnly(&msg))
      {
        this->PushState(&msg);
      }

      // Notify when ProxyManager state has changed
      // or any other state change
      this->NotifyOtherClients(&msg);
    }
    break;

    case vtkPVSessionServer::PULL:
    {
      std::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);

      // Use cache or process the call
      if (!this->Internal->RetreiveShareOnly(&msg))
      {
        this->PullState(&msg);
      }

      // Send the result back to client
      vtkMultiProcessStream css;
      css << msg.SerializeAsString();
      this->Internal->GetActiveController()->Send(css, 1, vtkPVSessionServer::REPLY_PULL);
    }
    break;
    case vtkPVSessionServer::REGISTER_SI:
    {
      std::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);
      this->RegisterSIObject(&msg);
    }
    break;
    case vtkPVSessionServer::UNREGISTER_SI:
    {
      std::string string;
      stream >> string;
      vtkSMMessage msg;
      msg.ParseFromString(string);
      this->UnRegisterSIObject(&msg);
    }
    break;

    case vtkPVSessionServer::EXECUTE_STREAM:
    {
      int ignore_errors, size;
      stream >> ignore_errors >> size;
      unsigned char* css_data = new unsigned char[size + 1];
      this->Internal->GetActiveController()->Receive(
        css_data, size, 1, vtkPVSessionServer::EXECUTE_STREAM_TAG);
      vtkClientServerStream cssStream;
      cssStream.SetData(css_data, size);
      this->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, cssStream, ignore_errors != 0);
      delete[] css_data;
    }
    break;

    case vtkPVSessionServer::LAST_RESULT:
    {
      this->SendLastResultToClient();
    }
    break;

    case vtkPVSessionServer::GATHER_INFORMATION:
    {
      std::string classname;
      vtkTypeUInt32 location, globalid;
      stream >> location >> classname >> globalid;
      this->GatherInformationInternal(location, classname.c_str(), globalid, stream);
    }
    break;
  }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::SendLastResultToClient()
{
  const vtkClientServerStream& reply = this->GetLastResult(vtkPVSession::CLIENT_AND_SERVERS);

  const unsigned char* data;
  size_t size_size_t;
  int size;

  reply.GetData(&data, &size_size_t);
  size = static_cast<int>(size_size_t);

  this->Internal->GetActiveController()->Send(&size, 1, 1, vtkPVSessionServer::REPLY_LAST_RESULT);
  this->Internal->GetActiveController()->Send(data, size, 1, vtkPVSessionServer::REPLY_LAST_RESULT);
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::GatherInformationInternal(vtkTypeUInt32 location, const char* classname,
  vtkTypeUInt32 globalid, vtkMultiProcessStream& stream)
{
  vtkSmartPointer<vtkObjectBase> o;
  o.TakeReference(vtkClientServerStreamInstantiator::CreateInstance(classname));

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
    this->Internal->GetActiveController()->Send(
      &len, 1, 1, vtkPVSessionServer::REPLY_GATHER_INFORMATION_TAG);
    this->Internal->GetActiveController()->Send(const_cast<unsigned char*>(data), length, 1,
      vtkPVSessionServer::REPLY_GATHER_INFORMATION_TAG);
  }
  else
  {
    vtkErrorMacro(
      "Could not create information object: `" << (classname ? classname : "(nullptr)") << "`.");
    // let client know that gather failed.
    int len = 0;
    this->Internal->GetActiveController()->Send(
      &len, 1, 1, vtkPVSessionServer::REPLY_GATHER_INFORMATION_TAG);
  }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::OnCloseSessionRMI()
{
  if (this->GetIsAlive())
  {
    this->Internal->CloseActiveController();
  }
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkPVSessionServer::NotifyOtherClients(const vtkSMMessage* msg)
{
  this->Internal->NotifyOtherClients(msg);
}

//----------------------------------------------------------------------------
void vtkPVSessionServer::NotifyAllClients(const vtkSMMessage* msg)
{
  this->Internal->NotifyAllClients(msg);
}
