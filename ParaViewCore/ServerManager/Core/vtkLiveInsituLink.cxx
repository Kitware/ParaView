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
#include "vtkLiveInsituLink.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkCommunicationErrorCatcher.h"
#include "vtkExtractsDeliveryHelper.h"
#include "vtkMultiProcessStream.h"
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSessionBase.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkSMInsituStateLoader.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkTrivialProducer.h"

#include <assert.h>
#include <map>
#include <set>
#include <string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

// #define vtkLiveInsituLinkDebugMacro(x) cout << __LINE__ << " " x << endl;
#define vtkLiveInsituLinkDebugMacro(x)

namespace
{
  void InitializeConnectionRMI(void *localArg,
    void *vtkNotUsed(remoteArg),
    int vtkNotUsed(remoteArgLength),
    int vtkNotUsed(remoteProcessId))
    {
    vtkLiveInsituLink* self = reinterpret_cast<vtkLiveInsituLink*>(localArg);
    self->InsituProcessConnected(NULL);
    }

  void DropCatalystParaViewConnectionRMI(void *localArg,
    void *vtkNotUsed(remoteArg),
    int vtkNotUsed(remoteArgLength),
    int vtkNotUsed(remoteProcessId))
    {
    vtkLiveInsituLink* self = reinterpret_cast<vtkLiveInsituLink*>(localArg);
    self->DropCatalystParaViewConnection();
    }

  void UpdateRMI(void *localArg,
    void *remoteArg, int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
    {
    vtkLiveInsituLink* self = reinterpret_cast<vtkLiveInsituLink*>(localArg);
    double time = *(reinterpret_cast<double*>(remoteArg));
    if (self->GetProcessType() == vtkLiveInsituLink::VISUALIZATION)
      {
      self->OnSimulationUpdate(time);
      }
    else
      {
      self->SimulationUpdate(time);
      }
    }

  void PostProcessRMI(void *localArg,
    void *remoteArg, int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
    {
    vtkLiveInsituLink* self = reinterpret_cast<vtkLiveInsituLink*>(localArg);
    double time = *(reinterpret_cast<double*>(remoteArg));
 
    if (self->GetProcessType() == vtkLiveInsituLink::VISUALIZATION)
      {
      self->OnSimulationPostProcess(time);
      }
    else
      {
      self->SimulationPostProcess(time);
      }
    }
}

class vtkLiveInsituLink::vtkInternals
{
public:
  struct Key
    {
    std::string Group;
    std::string Name;
    int Port;
    std::string ToString() const
      {
      vtksys_ios::ostringstream key;
      key << this->Group.c_str() << ":" << this->Name.c_str() << ":" <<
        this->Port;
      return key.str();
      }
    bool operator < (const Key& other) const
      {
      return this->Group < other.Group ||
        this->Name < other.Name ||
        this->Port < other.Port; 
      }
    Key() : Port(0) {}
    Key(const char* group, const char* name, int port):
      Group(group), Name(name), Port(port) {}
    };

  bool IsNew(vtkTypeUInt32 proxyId, unsigned int port, vtkPVDataInformation* info)
  {
    vtkIdType id = static_cast<vtkIdType>(proxyId) * 100 + static_cast<vtkIdType>(port);
    vtkClientServerStream stream;
    info->CopyToStream(&stream);
    size_t length = 0;
    const unsigned char *data = NULL;
    stream.GetData(&data, &length);
    std::string rawData((const char*)data, length);

    // Search if existing value is the same
    std::map<vtkIdType, std::string>::iterator iter;
    iter = this->LastSentDataInformationMap.find(id);
    if((iter != this->LastSentDataInformationMap.end()) && (iter->second == rawData))
      {
      return false;
      }

    // Store the new MTime
    this->LastSentDataInformationMap[id] = rawData;

    return true;
  }

  typedef std::map<Key, vtkSmartPointer<vtkTrivialProducer> > ExtractsMap;
  ExtractsMap Extracts;
  std::map<vtkIdType, std::string> LastSentDataInformationMap;
};

vtkStandardNewMacro(vtkLiveInsituLink);
//----------------------------------------------------------------------------
vtkLiveInsituLink::vtkLiveInsituLink():
  Hostname(0),
  InsituPort(0),
  ProcessType(SIMULATION),
  ProxyId(0),
  InsituXMLStateChanged(false),
  ExtractsChanged(false),
  InsituXMLState(0),
  URL(0),
  Internals(new vtkInternals())
{
  this->SetHostname("localhost");
}

//----------------------------------------------------------------------------
vtkLiveInsituLink::~vtkLiveInsituLink()
{
  this->SetHostname(0);
  this->SetURL(0);

  delete []this->InsituXMLState;
  this->InsituXMLState = 0;

  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::Initialize(vtkSMSessionProxyManager* pxm)
{
  if (this->Controller)
    {
    // already initialized. all's well.
    return;
    }

  if (this->ExtractsDeliveryHelper != NULL)
    {
    // already initialized (this->Controller is non-existant on satellites)
    return;
    }

  this->CoprocessorProxyManager = pxm;

  switch (this->ProcessType)
    {
  case VISUALIZATION:
    this->InitializeVisualization();
    break;

  case SIMULATION:
    this->InitializeSimulation();
    break;
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::InitializeVisualization()
{
  // This method gets called on the DataServer nodes.
  // On the root-node, we need to setup the server-socket to accept connections
  // from VTK Insitu code.
  // On satellites, we need to setup MPI-RMI handlers to ensure that the
  // satellites respond to a VTK Insitu connection setup correctly.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  // int numProcs = pm->GetNumberOfLocalPartitions();

  vtkLiveInsituLinkDebugMacro(<<myId << ": InitializeVisualization");
  if (myId == 0)
    {
    // save the visualization session reference so that we can communicate back to
    // the client.
    this->VisualizationSession =
      vtkPVSessionBase::SafeDownCast(pm->GetActiveSession());

    vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();

    // if the Catalyst connection ever drops, we need to communicate to the
    // client on the VisualizationSession that the catalyst server no longer
    // exists.
    nam->AddObserver(vtkCommand::ConnectionClosedEvent,
      this, &vtkLiveInsituLink::OnConnectionClosedEvent);

    vtksys_ios::ostringstream url;
    url << "tcp://localhost:" << this->InsituPort << "?"
      << "listen=true&nonblocking=true&"
      << "handshake=paraview.insitu." << PARAVIEW_VERSION;
    this->SetURL(url.str().c_str());
    vtkMultiProcessController* controller = nam->NewConnection(this->URL);
    if (controller)
      {
      // controller would generally be NULL, however due to magically timing,
      // the insitu lib may indeed connect just as we setup the socket, so we
      // handle that case.
      this->InsituProcessConnected(controller);
      controller->Delete();
      }
    else
      {
      nam->AddObserver(vtkCommand::ConnectionCreatedEvent,
        this, &vtkLiveInsituLink::OnConnectionCreatedEvent);
      }
    }
  else
    {
    vtkMultiProcessController* parallelController =
      vtkMultiProcessController::GetGlobalController();

    // add callback to listen to "events" from root node.
    // the command channel between sim and vis nodes is only setup on the root
    // nodes (there are socket connections between satellites for data x'fer but
    // not for any other kind of communication.
    parallelController->AddRMICallback(&InitializeConnectionRMI, this,
      INITIALIZE_CONNECTION);
    parallelController->AddRMICallback(&UpdateRMI, this, UPDATE_RMI_TAG);
    parallelController->AddRMICallback(&PostProcessRMI, this,
      POSTPROCESS_RMI_TAG);
    parallelController->AddRMICallback(&DropCatalystParaViewConnectionRMI, this,
      DROP_CAT2PV_CONNECTION);
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::InitializeSimulation()
{
  // vtkLiveInsituLink::Initialize() should not call this method unless
  // Controller==NULL.
  assert(this->Controller == NULL);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  int numProcs = pm->GetNumberOfLocalPartitions();

  if (!pm->GetSymmetricMPIMode() && numProcs > 1)
    {
    vtkErrorMacro(
      "Running in parallel without symmetric mode is not supported. "
      "Aborting for debugging purposes.");
    abort();
    }

  if (myId == 0)
    {
    vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();

    vtksys_ios::ostringstream url;
    url << "tcp://" << this->Hostname << ":" << this->InsituPort << "?"
      << "timeout=0&handshake=paraview.insitu." << PARAVIEW_VERSION;
    this->SetURL(url.str().c_str());
    // Suppress any error messages while attempting to connect to ParaView
    // visualization engine. 
    int display_errors = vtkObject::GetGlobalWarningDisplay();
    vtkObject::GlobalWarningDisplayOff();
    vtkMultiProcessController* controller = nam->NewConnection(this->URL);
    vtkObject::SetGlobalWarningDisplay(display_errors);
    if (numProcs > 1)
      {
      int connection_established = controller != NULL? 1 : 0;
      pm->GetGlobalController()->Broadcast(&connection_established, 1, 0);
      }
    if (controller)
      {
      // controller would generally be NULL, however due to magically timing,
      // the insitu lib may indeed connect just as we setup the socket, so we
      // handle that case.
      this->InsituProcessConnected(controller);
      controller->Delete();
      }
    // nothing to do, no server to connect to.
    }
  else
    {
    int connection_established = 0;
    pm->GetGlobalController()->Broadcast(&connection_established, 1, 0);
    if (connection_established)
      {
      this->InsituProcessConnected(NULL);
      }
    }
}

//----------------------------------------------------------------------------
// Callback on Visualization process when a simulation connects to it.
void vtkLiveInsituLink::OnConnectionCreatedEvent()
{
  assert(this->ProcessType == VISUALIZATION);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
  vtkMultiProcessController* controller = nam->NewConnection(this->URL);
  if (controller)
    {
    this->InsituProcessConnected(controller);
    controller->Delete();
    }
}

//----------------------------------------------------------------------------
// Callback on Visualization process when a connection dies during
// ProcessEvents().
void vtkLiveInsituLink::OnConnectionClosedEvent(
  vtkObject*, unsigned long, void* calldata)
{
  vtkObject* object = reinterpret_cast<vtkObject*>(calldata);
  vtkMultiProcessController* controller =
    vtkMultiProcessController::SafeDownCast(object);
  if (controller && this->Controller == controller)
    {
    // drop connection.

    // since this is called only on root node, we need to tell all satellites to
    // drop the connection too.
    vtkMultiProcessController* parallelController =
      vtkMultiProcessController::GetGlobalController();
    int numProcs = parallelController->GetNumberOfProcesses();

    if (numProcs > 1)
      {
      assert(parallelController->GetLocalProcessId() == 0);
      parallelController->TriggerRMIOnAllChildren(DROP_CAT2PV_CONNECTION);
      }
    this->DropCatalystParaViewConnection();

    // notify client.
    if (this->VisualizationSession)
      {
      assert(parallelController->GetLocalProcessId() == 0);
      vtkSMMessage message;
      message.set_global_id(this->ProxyId);
      message.set_location(vtkPVSession::CLIENT);
      message.SetExtension(ProxyState::xml_group, "Catalyst_Communication");
      message.SetExtension(ProxyState::xml_name, "Catalyst_Communication");

      // Add custom user_data
      ProxyState_UserData* user_data = message.AddExtension(ProxyState::user_data);
      user_data->set_key("LiveAction");
      Variant* variant = user_data->add_variant();
      variant->set_type(Variant::INT); // Arbitrary
      variant->add_integer(DISCONNECTED);

      // Send message
      this->VisualizationSession->NotifyAllClients(&message);
      }
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::DropCatalystParaViewConnection()
{
  this->Controller = 0;
  this->ExtractsDeliveryHelper = 0;
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::InsituProcessConnected(vtkMultiProcessController* controller)
{
  assert(this->Controller == NULL);
  assert(this->ExtractsDeliveryHelper.GetPointer() == NULL);

  this->Controller = controller;

  this->ExtractsDeliveryHelper =
    vtkSmartPointer<vtkExtractsDeliveryHelper>::New();
  this->ExtractsDeliveryHelper->SetProcessIsProducer(
    this->ProcessType == VISUALIZATION? false : true);

  vtkMultiProcessController* parallelController =
    vtkMultiProcessController::GetGlobalController();
  int numProcs = parallelController->GetNumberOfProcesses();
  int myId = parallelController->GetLocalProcessId();

  if (myId == 0)
    {
    assert(controller != NULL);
    }
  else
    {
    assert(controller == NULL);
    }

  switch (this->ProcessType)
    {
  case VISUALIZATION:
    // Visualization side is the "slave" side in this relationship. We listen to
    // commands from SIMULATION.
    if (myId == 0)
      {
      unsigned int size=0;
      controller->Receive(&size, 1, 1, 8000);

      delete [] this->InsituXMLState;
      this->InsituXMLState = new char[size + 1];
      controller->Receive(this->InsituXMLState, size, 1, 8001);
      this->InsituXMLState[size] = 0;
      this->InsituXMLStateChanged = false;

      // setup RMI callbacks.
      controller->AddRMICallback(&UpdateRMI, this, UPDATE_RMI_TAG);
      controller->AddRMICallback(&PostProcessRMI, this, POSTPROCESS_RMI_TAG);

      // setup M2N connection.
      int otherProcs;
      controller->Send(&numProcs, 1, 1, 8002);
      controller->Receive(&otherProcs, 1, 1, 8003);
      this->ExtractsDeliveryHelper->SetNumberOfVisualizationProcesses(numProcs);
      this->ExtractsDeliveryHelper->SetNumberOfSimulationProcesses(otherProcs);

      if (numProcs > 1)
        {
        parallelController->TriggerRMIOnAllChildren(INITIALIZE_CONNECTION);
        parallelController->Broadcast(&otherProcs, 1, 0);
        }
      }
    else
      {
      int otherProcs = 0;
      parallelController->Broadcast(&otherProcs, 1, 0);
      this->ExtractsDeliveryHelper->SetNumberOfVisualizationProcesses(numProcs);
      this->ExtractsDeliveryHelper->SetNumberOfSimulationProcesses(otherProcs);
      }

    // wait for each of the sim processes to setup a socket connection to the
    // vis nodes for data x'fer.
    if (myId < std::min(
        this->ExtractsDeliveryHelper->GetNumberOfVisualizationProcesses(),
        this->ExtractsDeliveryHelper->GetNumberOfSimulationProcesses()))
      {
      vtkSocketController* sim2vis = vtkSocketController::New();
      if (!sim2vis->WaitForConnection(this->InsituPort + 1 + myId))
        {
        abort();
        }
      this->ExtractsDeliveryHelper->SetSimulation2VisualizationController(
        sim2vis);
      sim2vis->Delete();
      }

    // notify client.
    if (this->VisualizationSession)
      {
      assert(myId == 0);
      vtkSMMessage message;
      message.set_global_id(this->ProxyId);
      message.set_location(vtkPVSession::CLIENT);
      message.SetExtension(ProxyState::xml_group, "Catalyst_Communication");
      message.SetExtension(ProxyState::xml_name, "Catalyst_Communication");

      // Add custom user_data
      ProxyState_UserData* user_data = message.AddExtension(ProxyState::user_data);
      user_data->set_key("LiveAction");
      Variant* variant = user_data->add_variant();
      variant->set_type(Variant::INT); // Arbitrary
      variant->add_integer(CONNECTED);
      variant->add_txt(this->InsituXMLState);

      // Send message
      this->VisualizationSession->NotifyAllClients(&message);
      }
    break;

  case SIMULATION:
    if (myId ==0)
      {
      // send startup state to the visualization process.
      vtkPVXMLElement* xml = this->CoprocessorProxyManager->SaveXMLState();

      // Filter XML to remove any view/representation/timeKeeper/animation proxy
      vtkLiveInsituLink::FilterXMLState(xml);

      vtksys_ios::ostringstream xml_string;
      xml->PrintXML(xml_string, vtkIndent());
      xml->Delete();

      unsigned int size = static_cast<unsigned int>(xml_string.str().size());
      controller->Send(&size, 1, 1, 8000);
      controller->Send(xml_string.str().c_str(),
        static_cast<vtkIdType>(xml_string.str().size()), 1, 8001);

      // setup M2N connection.
      int otherProcs;
      controller->Receive(&otherProcs, 1, 1, 8002);
      controller->Send(&numProcs, 1, 1, 8003);
      this->ExtractsDeliveryHelper->SetNumberOfVisualizationProcesses(otherProcs);
      this->ExtractsDeliveryHelper->SetNumberOfSimulationProcesses(numProcs);
      if (numProcs > 1)
        {
        parallelController->Broadcast(&otherProcs, 1, 0);
        }
      }
    else
      {
      int otherProcs = 0;
      parallelController->Broadcast(&otherProcs, 1, 0);
      this->ExtractsDeliveryHelper->SetNumberOfVisualizationProcesses(otherProcs);
      this->ExtractsDeliveryHelper->SetNumberOfSimulationProcesses(numProcs);
      }

    // connect to the sim-nodes for data x'fer.
    if (myId < std::min(
        this->ExtractsDeliveryHelper->GetNumberOfVisualizationProcesses(),
        this->ExtractsDeliveryHelper->GetNumberOfSimulationProcesses()))
      {
      vtkSocketController* sim2vis = vtkSocketController::New();
      vtksys::SystemTools::Delay(1000);
      if (!sim2vis->ConnectTo(this->Hostname, this->InsituPort + 1 + myId))
        {
        abort();
        }
      this->ExtractsDeliveryHelper->SetSimulation2VisualizationController(
        sim2vis);
      sim2vis->Delete();
      }
    break;
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::SimulationInitialize(vtkSMSessionProxyManager* pxm)
{
  assert(this->ProcessType == SIMULATION);

  this->Initialize(pxm);
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::SimulationUpdate(double time)
{
  assert(this->ProcessType == SIMULATION);

  if (!this->CoprocessorProxyManager)
    {
    vtkErrorMacro(
      "Please call vtkLiveInsituLink::Initialize(vtkSMSessionProxyManager*) "
      "before using vtkLiveInsituLink::SimulationUpdate().");
    return;
    }

  if (!this->ExtractsDeliveryHelper.GetPointer())
    {
    // We are not connected to ParaView Vis Engine. That can happen if the
    // ParaView Vis Engine was not ready the last time we attempted to connect
    // to it, or ParaView Vis Engine disconnected. We make a fresh attempt to
    // connect to it.
    this->SimulationInitialize(this->CoprocessorProxyManager);
    }

  if (!this->ExtractsDeliveryHelper.GetPointer())
    {
    // ParaView is still not ready. Another time.
    return;
    }

  // Okay, ParaView Visualization connection is currently valid, but it may
  // break, so add error interceptor.
  vtkCommunicationErrorCatcher catcher(this->Controller);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  int numProcs = pm->GetNumberOfLocalPartitions();

  char* buffer = NULL;
  int buffer_size = 0;

  vtkMultiProcessStream extractsMessage;
  extractsMessage << 0; // indicating nothing changed.
  std::vector<vtkTypeUInt32> idMappingInStateLoading;

  if (myId == 0)
    {
    // steps to perform:
    // 1. Check with visualization root-node to see if it has new coprocessing
    //    state updates. If so receive them and broadcast to all satellites.
    // 2. Update the CoprocessorProxyManager using the most recent XML state we
    //    have.
    if (this->Controller)
      {
      // Notify the vis root-node.
      this->Controller->TriggerRMI(1, &time, static_cast<int>(sizeof(double)),
        UPDATE_RMI_TAG);

      // Get status of the state. Did it change? If so receive the state and
      // broadcast it to satellites.
      this->Controller->Receive(&buffer_size, 1, 1, 8010);
      if (buffer_size > 0)
        {
        vtkLiveInsituLinkDebugMacro("receiving modified state from Vis");
        buffer = new char[buffer_size + 1];
        this->Controller->Receive(buffer, buffer_size, 1, 8011);
        buffer[buffer_size] = 0;
        }

      // Get the information about extracts. When the extracts have changed or
      // not is encoded in the stream itself.
      this->Controller->Receive(extractsMessage, 1, 8012);
      }

    if (numProcs > 1)
      {
      pm->GetGlobalController()->Broadcast(&buffer_size, 1, 0);
      if (buffer_size > 0)
        {
        pm->GetGlobalController()->Broadcast(buffer, buffer_size, 0);
        }
      pm->GetGlobalController()->Broadcast(extractsMessage, 0);
      }
    }
  else
    {
    assert(numProcs > 1);
    pm->GetGlobalController()->Broadcast(&buffer_size, 1, 0);
    if (buffer_size > 0)
      {
      buffer = new char[buffer_size + 1];
      pm->GetGlobalController()->Broadcast(buffer, buffer_size, 0);
      buffer[buffer_size] = 0;
      }
    pm->GetGlobalController()->Broadcast(extractsMessage, 0);
    }

  // ** here on, all the code is executed on all processes (root and
  // satellites).

  if (buffer && buffer_size > 0)
    {
    vtkNew<vtkPVXMLParser> parser;
    if (parser->Parse(buffer))
      {
      this->XMLState = parser->GetRootElement();
      }
    }
  delete[] buffer;

  int drop_connection = catcher.GetErrorsRaised()? 1 : 0;
  if (numProcs > 1)
    {
    pm->GetGlobalController()->Broadcast(&drop_connection, 1, 0);
    }

  if (drop_connection)
    {
    this->DropCatalystParaViewConnection();
    return;
    }


  if (this->XMLState)
    {
    vtkNew<vtkSMInsituStateLoader> loader;
    loader->KeepIdMappingOn();
    loader->SetSessionProxyManager(this->CoprocessorProxyManager);
    this->CoprocessorProxyManager->LoadXMLState(this->XMLState, loader.GetPointer());
    int mappingSize = 0;
    vtkTypeUInt32* inSituMapping = loader->GetMappingArray(mappingSize);
    // Save mapping outside that scope
    for(int i=0;i<mappingSize;++i)
      {
      idMappingInStateLoading.push_back(inSituMapping[i]);
      }
    }

  // Process information about extracts.
  int extracts_valid = 0;
  extractsMessage >> extracts_valid;
  if (extracts_valid)
    {
    assert(this->ExtractsDeliveryHelper.GetPointer() != NULL);
    this->ExtractsDeliveryHelper->ClearAllExtracts();
    int numberOfExtracts;
    extractsMessage >> numberOfExtracts;
    for (int cc=0; cc < numberOfExtracts; cc++)
      {
      std::string group, name;
      int port;
      extractsMessage >> group >> name >> port;

      vtkSMProxy* proxy = this->CoprocessorProxyManager->GetProxy(
        group.c_str(), name.c_str());
      if (proxy)
        {
        vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(
          proxy->GetClientSideObject());
        if (algo)
          {
          vtkInternals::Key key(group.c_str(), name.c_str(), port);
          this->ExtractsDeliveryHelper->AddExtractProducer(
            key.ToString().c_str(), algo->GetOutputPort(port));
          }
        else
          {
          vtkErrorMacro("No vtkAlgorithm: " << group.c_str() << ", " << name.c_str());
          }
        }
      else
        {
        vtkErrorMacro("No proxy: " << group.c_str() << ", " << name.c_str());
        }
      }
    }

  // Share the id mapping between inSitu and Viz root node
  if(this->Controller)
    {
    int mappingSize = static_cast<int>(idMappingInStateLoading.size());
    this->Controller->Send(&mappingSize, 1, 1, 8013);
    if(mappingSize > 0)
      {
      this->Controller->Send(&idMappingInStateLoading[0], mappingSize, 1, 8014);
      }
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::SimulationPostProcess(double time)
{
  assert(this->ProcessType == SIMULATION);

  if (!this->ExtractsDeliveryHelper)
    {
    // if this->ExtractsDeliveryHelper is NULL it means we are not connected to
    // any ParaView Visualization Engine yet. Just skip.
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();

  vtkCommunicationErrorCatcher catcher(this->Controller);
  if (myId == 0 && this->Controller)
    {
    // notify vis root node that we are ready to ship extracts.
    this->Controller->TriggerRMI(1, &time, static_cast<int>(sizeof(double)),
      POSTPROCESS_RMI_TAG);
    }

  int drop_connection = catcher.GetErrorsRaised()? 1 : 0;
  if (pm->GetNumberOfLocalPartitions() > 1)
    {
    pm->GetGlobalController()->Broadcast(&drop_connection, 1, 0);
    }

  if (drop_connection)
    {
    // ParaView has disconnected. Clean up the connection.
    this->DropCatalystParaViewConnection();
    return;
    }

  assert(this->ExtractsDeliveryHelper);

  // We're done coprocessing. Deliver the extracts to the visualization
  // processes.
  this->ExtractsDeliveryHelper->Update();

  // Update DataInformations
  if (myId == 0 && this->Controller)
    {
    vtkClientServerStream stream;
    vtkNew<vtkSMProxyIterator> proxyIterator;
    proxyIterator->SetSessionProxyManager(this->CoprocessorProxyManager);
    proxyIterator->SetModeToOneGroup();
    proxyIterator->Begin("sources");

    // Serialized DataInformation
    stream << vtkClientServerStream::Reply;
    while(!proxyIterator->IsAtEnd())
      {
      vtkSMSourceProxy* source =
          vtkSMSourceProxy::SafeDownCast(proxyIterator->GetProxy());
      if( source )
        {
        for(unsigned int port = 0; port < source->GetNumberOfOutputPorts(); ++port)
          {
          if(this->Internals->IsNew(source->GetGlobalID(), port, source->GetDataInformation(port)))
            {
            vtkClientServerStream dataStream;
            source->GetDataInformation(port)->CopyToStream(&dataStream);
            // Serialize the data
            stream << source->GetGlobalID() << port << dataStream;
            }
          }
        }
      proxyIterator->Next();
      }
    stream << vtkClientServerStream::End;

    // notify vis root node that we are ready to ship extracts.
    const unsigned char* data;
    size_t size;
    stream.GetData(&data, &size);
    vtkIdType idtype_size = static_cast<vtkIdType>(size);
    this->Controller->Send(&idtype_size, 1, 1, 674523);
    this->Controller->Send(&data[0], idtype_size, 1, 674524);
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::OnSimulationUpdate(double time)
{
  assert(this->ProcessType == VISUALIZATION);

  // this method get called on:
  // - root node when "sim" notifies the root viz node.
  // - satellizes when "root" viz node notifies the satellizes.
  vtkLiveInsituLinkDebugMacro(
    "vtkLiveInsituLink::OnSimulationUpdate: " << time);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  if (myId == 0 && pm->GetNumberOfLocalPartitions() > 1)
    {
    pm->GetGlobalController()->TriggerRMIOnAllChildren(
      &time, static_cast<int>(sizeof(double)), UPDATE_RMI_TAG);
    }

  if (myId == 0)
    {
    // The steps to perform are:
    // 1. Send coprocessing pipeline state to the simulation root node, if the
    //    state has changed since the last time we sent it.
    // 2. Send information about extracts to the simulation root, if the
    //    requested extracts has changed.

    int xml_state_size = 0;
    if (this->InsituXMLStateChanged)
      {
      xml_state_size = static_cast<int>(strlen(this->InsituXMLState));
      }
    // xml_state_size of 0 indicates that there are not updates to the state.
    // the CoProcessor simply uses the state it received most recently.
    this->Controller->Send(&xml_state_size, 1, 1, 8010);
    if (xml_state_size > 0)
      {
      vtkLiveInsituLinkDebugMacro("Sending modified state to simulation.");
      this->Controller->Send(this->InsituXMLState, xml_state_size, 1, 8011);
      }

    vtkMultiProcessStream extractsMessage;
    if (this->ExtractsChanged)
      {
      extractsMessage << 1;
      extractsMessage << static_cast<int>(this->Internals->Extracts.size());
      for (vtkInternals::ExtractsMap::iterator iter=this->Internals->Extracts.begin();
        iter != this->Internals->Extracts.end(); ++iter)
        {
        extractsMessage << iter->first.Group << iter->first.Name << iter->first.Port;
        }
      }
    else
      {
      extractsMessage << 0;
      }
    this->Controller->Send(extractsMessage, 1, 8012);

    // Read the server id mapping
    int numberOfIds = 0;
    this->Controller->Receive(&numberOfIds, 1, 1, 8013);
    if(numberOfIds > 0)
      {
      vtkTypeUInt32* idMapTable = new vtkTypeUInt32[numberOfIds];
      this->Controller->Receive(idMapTable, numberOfIds, 1, 8014);

      // notify the client with the updated ID mapping.
      if (this->VisualizationSession)
        {
        // here we may let the client know exactly what extracts were updated, if
        // all were not updated. Currently we just assume all extracts are
        // redelivered and modified.
        vtkSMMessage message;
        message.set_global_id(this->ProxyId);
        message.set_location(vtkPVSession::CLIENT);
        message.SetExtension(ProxyState::xml_group, "Catalyst_Communication");
        message.SetExtension(ProxyState::xml_name, "Catalyst_Communication");

        // Add custom user_data
        ProxyState_UserData* user_data = message.AddExtension(ProxyState::user_data);
        user_data->set_key("IdMapping");
        Variant* variant = user_data->add_variant();
        variant->set_type(Variant::IDTYPE); // Arbitrary

        for(int i=0; i < numberOfIds; ++i)
          {
          variant->add_idtype(idMapTable[i]);
          }

        // Send message
        this->VisualizationSession->NotifyAllClients(&message);
        delete[] idMapTable;
        }
      }
    }
  else
    {
    // There's nothing to do on satellites for this call right now.
    }

  this->InsituXMLStateChanged = false;
  this->ExtractsChanged = false;
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::OnSimulationPostProcess(double time)
{
  assert(this->ProcessType == VISUALIZATION);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  if (myId == 0 && pm->GetNumberOfLocalPartitions() > 1)
    {
    pm->GetGlobalController()->TriggerRMIOnAllChildren(
      &time, static_cast<int>(sizeof(double)), POSTPROCESS_RMI_TAG);
    }
  
  vtkLiveInsituLinkDebugMacro(
    "vtkLiveInsituLink::OnSimulationPostProcess: " << time);

  // Obtains extracts from the simulations processes.
  bool dataAvailable = this->ExtractsDeliveryHelper->Update();

  // Retreive the vtkPVDataInformations
  std::map<std::pair<vtkTypeUInt32,unsigned int>,std::string> dataInformationToSend;
  if (myId == 0 && this->Controller)
    {
    // Convert serialized version
    vtkIdType size = 0;
    this->Controller->Receive(&size, 1, 1, 674523);
    unsigned char* data = new unsigned char[size];
    this->Controller->Receive((char*)data, size, 1, 674524);
    vtkClientServerStream mainStream;
    mainStream.SetData(data, size);

    int nbArgs = mainStream.GetNumberOfArguments(0);
    int arg = 0;
    vtkTypeUInt32 id;
    unsigned int port;
    vtkClientServerStream stream;
    while(arg < nbArgs)
      {
      mainStream.GetArgument(0, arg++, &id);
      mainStream.GetArgument(0, arg++, &port);
      mainStream.GetArgument(0, arg++, &stream);
      const unsigned char *oldStr;
      size_t oldStrSize;
      stream.GetData(&oldStr, &oldStrSize);
      std::string newStr((const char*)oldStr, oldStrSize);

      dataInformationToSend[std::pair<vtkTypeUInt32, unsigned int>(id,port)] = newStr;
      }
    }

  // notify the client that updated data is available.
  if (this->VisualizationSession && dataAvailable)
    {
    assert (myId == 0);

    // here we may let the client know exactly what extracts were updated, if
    // all were not updated. Currently we just assume all extracts are
    // redelivered and modified.
    vtkSMMessage message;
    message.set_global_id(this->ProxyId);
    message.set_location(vtkPVSession::CLIENT);
    message.SetExtension(ProxyState::xml_group, "Catalyst_Communication");
    message.SetExtension(ProxyState::xml_name, "Catalyst_Communication");

    // Add custom user_data
    ProxyState_UserData* user_data = message.AddExtension(ProxyState::user_data);
    user_data->set_key("LiveAction");
    Variant* variant = user_data->add_variant();
    variant->set_type(Variant::INT); // Arbitrary
    variant->add_integer(NEXT_TIMESTEP_AVAILABLE);

    // Add custom user_data for data information
    if(dataInformationToSend.size() > 0)
      {
      ProxyState_UserData* dataInfo = message.AddExtension(ProxyState::user_data);
      dataInfo->set_key("UpdateDataInformation");
      std::map<std::pair<vtkTypeUInt32,unsigned int>,std::string>::iterator iter;
      for( iter = dataInformationToSend.begin();
           iter != dataInformationToSend.end();
           iter++ )
        {
        Variant* variant2 = dataInfo->add_variant();
        variant2->set_type(Variant::PROXY); // Arbitrary
        variant2->add_proxy_global_id(iter->first.first);
        variant2->add_port_number(iter->first.second);
        variant2->add_binary(iter->second);
        }
      }

    // Send message
    this->VisualizationSession->NotifyAllClients(&message);
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::RegisterExtract(vtkTrivialProducer* producer,
    const char* groupname, const char* proxyname, int portnumber)
{
  assert(this->ProcessType == VISUALIZATION);

  if (!this->ExtractsDeliveryHelper)
    {
    vtkWarningMacro("Connection to simulation has been dropped!!!");
    return;
    }

  vtkLiveInsituLinkDebugMacro(
    "Adding Extract: " << groupname << ", " << proxyname);

  vtkInternals::Key key(groupname, proxyname, portnumber);
  this->Internals->Extracts[key] = producer;
  this->ExtractsChanged = true;
  this->ExtractsDeliveryHelper->AddExtractConsumer(
    key.ToString().c_str(), producer);
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::UnRegisterExtract(vtkTrivialProducer* producer)
{
  assert(this->ProcessType == VISUALIZATION);

  if (!this->ExtractsDeliveryHelper)
    {
    vtkWarningMacro("Connection to simulation has been dropped!!!");
    return;
    }

  for (vtkInternals::ExtractsMap::iterator iter=this->Internals->Extracts.begin();
    iter != this->Internals->Extracts.end(); ++iter)
    {
    if (iter->second.GetPointer() == producer)
      {
      this->ExtractsDeliveryHelper->RemoveExtractConsumer(
        iter->first.ToString().c_str());
      this->Internals->Extracts.erase(iter);
      this->ExtractsChanged = true;
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkLiveInsituLink::FilterXMLState(vtkPVXMLElement* xmlState)
{
  if(xmlState == NULL)
    {
    return false;
    }

  // Init search set if needed
  static std::set<std::string> groupSet;
  static std::set<std::string> nameSet;

  // Fill search set if empty
  if(groupSet.size() == 0)
    {
    groupSet.insert("animation");
    groupSet.insert("misc");
    groupSet.insert("representations");
    groupSet.insert("views");
    // ---
    nameSet.insert("timekeeper");
    nameSet.insert("animation");
    nameSet.insert("views");
    nameSet.insert("representations");
    }

  bool changed = false;
  if(!strcmp(xmlState->GetName(), "Proxy"))
    {
    std::string group = xmlState->GetAttribute("group");
    if(groupSet.find(group) != groupSet.end())
      {
      xmlState->GetParent()->RemoveNestedElement(xmlState);
      return true;
      }
    }
  else if(!strcmp(xmlState->GetName(), "ProxyCollection"))
    {
    std::string name = xmlState->GetAttribute("name");
    if(nameSet.find(name) != nameSet.end())
      {
      xmlState->GetParent()->RemoveNestedElement(xmlState);
      return true;
      }
    }
  else
    {
    for(unsigned int i=0; i < xmlState->GetNumberOfNestedElements(); ++i)
      {
      if(vtkLiveInsituLink::FilterXMLState(xmlState->GetNestedElement(i)))
        {
        changed = true;
        i--;
        }
      }
    }
  return changed;
}
