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

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMSessionProxyManager.h"

#include <assert.h>
#include <string>
#include <vtksys/ios/sstream>


namespace
{
  void UpdateRMI(void *localArg,
    void *remoteArg, int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
    {
    vtkLiveInsituLink* self = reinterpret_cast<vtkLiveInsituLink*>(localArg);
    double time = *(reinterpret_cast<double*>(remoteArg));
 
    self->OnSimulationUpdate(time);
    }

  void PostProcessRMI(void *localArg,
    void *remoteArg, int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
    {
    vtkLiveInsituLink* self = reinterpret_cast<vtkLiveInsituLink*>(localArg);
    double time = *(reinterpret_cast<double*>(remoteArg));
 
    self->OnSimulationPostProcess(time);
    }
}

vtkStandardNewMacro(vtkLiveInsituLink);
vtkCxxSetObjectMacro(vtkLiveInsituLink, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkLiveInsituLink, ProxyManager, vtkSMSessionProxyManager);
//----------------------------------------------------------------------------
vtkLiveInsituLink::vtkLiveInsituLink():
  Hostname(0),
  InsituPort(0),
  ProcessType(SIMULATION),
  Controller(0),
  InsituXMLState(0),
  ProxyManager(0),
  URL(0)
{
  this->SetHostname("localhost");
}

//----------------------------------------------------------------------------
vtkLiveInsituLink::~vtkLiveInsituLink()
{
  this->SetHostname(0);
  this->SetController(0);
  this->SetProxyManager(0);
  this->SetURL(0);

  delete []this->InsituXMLState;
  this->InsituXMLState = 0;
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::Initialize(vtkSMSessionProxyManager* pxm)
{
  if (this->Controller)
    {
    // already initialized. all's well.
    return;
    }

  this->SetProxyManager(pxm);

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

  if (myId == 0)
    {
    vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();

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
      }
    else
      {
      nam->AddObserver(vtkCommand::ConnectionCreatedEvent,
        this, &vtkLiveInsituLink::OnConnectionCreatedEvent);
      }
    }
  else
    {
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::InitializeSimulation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  // int numProcs = pm->GetNumberOfLocalPartitions();


  if (myId == 0)
    {
    vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();

    vtksys_ios::ostringstream url;
    url << "tcp://" << this->Hostname << ":" << this->InsituPort << "?"
      << "handshake=paraview.insitu." << PARAVIEW_VERSION;
    this->SetURL(url.str().c_str());
    vtkMultiProcessController* controller = nam->NewConnection(this->URL);
    if (controller)
      {
      // controller would generally be NULL, however due to magically timing,
      // the insitu lib may indeed connect just as we setup the socket, so we
      // handle that case.
      this->InsituProcessConnected(controller);
      }
    // nothing to do, no server to connect to.
    }
  else
    {
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::OnConnectionCreatedEvent()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
  vtkMultiProcessController* controller = nam->NewConnection(this->URL);
  if (controller)
    {
    this->InsituProcessConnected(controller);
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::InsituProcessConnected(vtkMultiProcessController* controller)
{
  this->SetController(controller);
  switch (this->ProcessType)
    {
  case VISUALIZATION:
    // Visualization side is the "slave" side in this relationship. We listen to
    // commands from SIMULATION.
      {
      unsigned int size=0;
      controller->Receive(&size, 1, 1, 8000);

      delete [] this->InsituXMLState;
      this->InsituXMLState = new char[size + 1];
      controller->Receive(this->InsituXMLState, size, 1, 8001);
      this->InsituXMLState[size] = 0;

      cout << "Received InsituXMLState" << endl;

      // setup RMI callbacks.
      controller->AddRMICallback(&UpdateRMI, this, UPDATE_RMI_TAG);
      controller->AddRMICallback(&PostProcessRMI, this, POSTPROCESS_RMI_TAG);
      }
    break;

  case SIMULATION:
      {
      cout << "Sending InsituXMLState" << endl;

      // send startup state to the visualization process.
      vtkPVXMLElement* xml = this->ProxyManager->SaveXMLState();
      vtksys_ios::ostringstream xml_string;
      xml->PrintXML(xml_string, vtkIndent());
      xml->Delete();

      unsigned int size = static_cast<unsigned int>(xml_string.str().size());
      controller->Send(&size, 1, 1, 8000);
      controller->Send(xml_string.str().c_str(),
        static_cast<vtkIdType>(xml_string.str().size()), 1, 8001);
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

  // Ping the Visualization process to see if it has any updated state. If not,
  // just load the most recent state received.

  // FIXME: Controller will be NULL on satellites.
  if (this->Controller)
    {
    this->Controller->TriggerRMI(1, &time, static_cast<int>(sizeof(double)),
      UPDATE_RMI_TAG);

    // Get status of the state. Did is change?
    int xml_state_size = 0;
    this->Controller->Receive(&xml_state_size, 1, 1, 8010);

    if (xml_state_size > 0)
      {
      char* buffer = new char[xml_state_size + 1];
      this->Controller->Receive(buffer, xml_state_size, 1, 8011);
      buffer[xml_state_size] = 0;

      vtkNew<vtkPVXMLParser> parser;
      if (parser->Parse(buffer))
        {
        this->XMLState = parser->GetRootElement();
        }
      delete[] buffer;
      }
    }

  if (this->XMLState)
    {
    this->ProxyManager->LoadXMLState(this->XMLState /* FIXME: , loader */);
    }
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::SimulationPostProcess(double time)
{
  assert(this->ProcessType == SIMULATION);
  // We're done insitu-processing.
  // Pass on the extracts to the visualization process.

  // FIXME: Controller will be NULL on satellites.
  if (this->Controller)
    {
    this->Controller->TriggerRMI(1, &time, static_cast<int>(sizeof(double)),
      POSTPROCESS_RMI_TAG);
    }
}


//----------------------------------------------------------------------------
void vtkLiveInsituLink::OnSimulationUpdate(double time)
{
  assert(this->ProcessType == VISUALIZATION);
  // send updated state to simulation, if any.

  int xml_state_size = 0;
  this->Controller->Send(&xml_state_size, 1, 1, 8010);
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::OnSimulationPostProcess(double time)
{
  assert(this->ProcessType == VISUALIZATION);
}

//----------------------------------------------------------------------------
void vtkLiveInsituLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
