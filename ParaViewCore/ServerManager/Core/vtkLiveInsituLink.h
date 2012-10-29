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
// .NAME vtkLiveInsituLink - link for live-coprocessing.
// .SECTION Description
// vtkLiveInsituLink manages the communication link between Catalyst and
// ParaView visualization server. vtkLiveInsituLink is created on both ends of
// the live-coprocessing channel i.e. in Catalyst code (by instantiating
// vtkLiveInsituLink directly) and in ParaView application (by using a proxy
// that instantiates the vtkLiveInsituLink).

#ifndef __vtkLiveInsituLink_h
#define __vtkLiveInsituLink_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports

#include "vtkSMObject.h"
#include "vtkWeakPointer.h"  // Needed for Weak pointer
#include "vtkSmartPointer.h" // Needed for Smart pointer

class vtkMultiProcessController;
class vtkSMSessionProxyManager;
class vtkPVXMLElement;
class vtkPVSessionBase;
class vtkTrivialProducer;
class vtkExtractsDeliveryHelper;

class VTKPVSERVERMANAGERCORE_EXPORT vtkLiveInsituLink : public vtkSMObject
{
public:
  static vtkLiveInsituLink* New();
  vtkTypeMacro(vtkLiveInsituLink, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the port number. This is the port on which the root data-server node
  // will open a server-socket to accept connections from VTK InSitu Library.
  vtkSetMacro(InsituPort, int);
  vtkGetMacro(InsituPort, int);

  // Description:
  // Set the host name.
  vtkSetStringMacro(Hostname);
  vtkGetStringMacro(Hostname);

  // Called on the vis-process to register a producer for an extract.
  void RegisterExtract(vtkTrivialProducer* producer,
    const char* groupname, const char* proxyname, int portnumber);
  void UnRegisterExtract(vtkTrivialProducer* producer);

  // Description:
  // Set/Get the link type i.e. whether the current process is the visualization
  // process or the insitu process.
  enum
    {
    VISUALIZATION=0,
    SIMULATION=1
    };
  vtkSetClampMacro(ProcessType, int, VISUALIZATION, SIMULATION);
  vtkGetMacro(ProcessType, int);

  // Description:
  // When instantiated on the ParaView visualization server side using a
  // vtkSMProxy, ProxyId is used to identify the proxy corresponding to this
  // instance. That helps us construct notification messages that the
  // visualization server can send to the client.
  vtkSetMacro(ProxyId, unsigned int);
  vtkGetMacro(ProxyId, unsigned int);

  // Description:
  // Initializes the link.
  void Initialize() { this->Initialize(NULL); }
  void Initialize(vtkSMSessionProxyManager*);

  // **************************************************************************
  //      *** API to be used from the insitu library ***

  // Description:
  // Must be called at the beginning with the proxy manager. vtkLiveInsituLink
  // makes an attempt to connect to ParaView,  however that attempt may fail if
  // ParaView is not yet ready to accept connections. In that case,
  // vtkLiveInsituLink will make an attempt to connect on every subsequent
  // SimulationUpdate() call.
  void SimulationInitialize(vtkSMSessionProxyManager* pxm);

  // Description:
  // Every time Catalyst is ready to communicate with ParaView visualization
  // engine call this method. The goal of this call is too get the latest
  // updates from ParaView including changes to state for the co-processing
  // pipeline or changes in what extract the visualization engine is expecting.
  // This method's primary goal is to obtain information from ParaView vis
  // engine. If no active connection to ParaView visualization engine exists,
  // this will make an attempt to connect to ParaView.
  void SimulationUpdate(double time);

  // Description:
  // Every time Catalyst is ready to push extracts to ParaView visualization
  // engine, call this method. If no active ParaView visualization engine
  // connection exists (or the connection dies), then this method does nothing
  // (besides some bookkeeping).  Otherwise, this will push any extracts
  // requested to the ParaView visualization engine.
  void SimulationPostProcess(double time);

  // **************************************************************************

  // **************************************************************************
  // API to be used from the Visualization side.
  void OnSimulationUpdate(double time);
  void OnSimulationPostProcess(double time);
  // **************************************************************************

  enum NotificationTags
    {
    CONNECTED = 1200,
    NEXT_TIMESTEP_AVAILABLE = 1201,
    DISCONNECTED = 1202
    };

  void UpdateInsituXMLState(const char* txt)
    {
    this->InsituXMLStateChanged = true;
    this->SetInsituXMLState(txt);
    }

  // Description:
  // This method will remove references to proxy that shouldn't be shared with ParaView
  // Return true if something has been removed
  static bool FilterXMLState(vtkPVXMLElement* xmlState);

//BTX
  // ***************************************************************
  // Internal methods, public for callbacks.
  void InsituProcessConnected(vtkMultiProcessController* controller);

  // Description:
  // Called to drop the connection between Catalyst and ParaView.
  void DropCatalystParaViewConnection();

protected:
  vtkLiveInsituLink();
  ~vtkLiveInsituLink();

  enum RMITags
    {
    UPDATE_RMI_TAG=8800,
    POSTPROCESS_RMI_TAG=8801,
    INITIALIZE_CONNECTION=8802,
    DROP_CAT2PV_CONNECTION=8803
    };

  // Description:
  // Called by Initialize() to initialize on a visualization process.
  void InitializeVisualization();

  // Description:
  // Called by Initialize() to initialize on a simulation process.
  void InitializeSimulation();

  // Description:
  // Callback on Visualization process when a simulation connects to it.
  void OnConnectionCreatedEvent();

  // Description:
  // Callback on Visualization process when a connection dies during
  // vtkNetworkAccessManager::ProcessEvents().
  void OnConnectionClosedEvent(
    vtkObject*, unsigned long eventid, void* calldata);

  char* Hostname;
  int InsituPort;
  int ProcessType;
  unsigned int ProxyId;

  bool InsituXMLStateChanged;
  bool ExtractsChanged;

  char* InsituXMLState;
  vtkSmartPointer<vtkPVXMLElement> XMLState;
  vtkWeakPointer<vtkPVSessionBase> VisualizationSession;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkExtractsDeliveryHelper> ExtractsDeliveryHelper;

private:
  vtkLiveInsituLink(const vtkLiveInsituLink&); // Not implemented
  void operator=(const vtkLiveInsituLink&); // Not implemented

  vtkWeakPointer<vtkSMSessionProxyManager> CoprocessorProxyManager;

  vtkSetStringMacro(URL);
  char* URL;

  vtkSetStringMacro(InsituXMLState);

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
