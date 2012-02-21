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
// .NAME vtkLiveInsituLink
// .SECTION Description
//

#ifndef __vtkLiveInsituLink_h
#define __vtkLiveInsituLink_h

#include "vtkPVSessionBase.h"
#include "vtkSmartPointer.h"
#include "vtkSMObject.h"
#include "vtkWeakPointer.h"

class vtkMultiProcessController;
class vtkSMSessionProxyManager;
class vtkPVXMLElement;
class vtkPVSessionBase;
class vtkTrivialProducer;
class vtkExtractsDeliveryHelper;
class VTK_EXPORT vtkLiveInsituLink : public vtkSMObject
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

  vtkSetMacro(ProxyId, unsigned int);
  vtkGetMacro(ProxyId, unsigned int);

  // Description:
  // Initializes the link.
  void Initialize() { this->Initialize(NULL); }
  void Initialize(vtkSMSessionProxyManager*);

  // API to be used from the insitu library.
  void SimulationInitialize(vtkSMSessionProxyManager* pxm);
  void SimulationUpdate(double time);
  void SimulationPostProcess(double time);

  // API to be used from the Visualization side.
  void OnSimulationUpdate(double time);
  void OnSimulationPostProcess(double time);

  enum NotificationTags
    {
    CONNECTED = 1200,
    NEXT_TIMESTEP_AVAILABLE = 1201
    };

  void UpdateInsituXMLState(const char* txt)
    {
    this->InsituXMLStateChanged = true;
    this->SetInsituXMLState(txt);
    }

//BTX
  void InsituProcessConnected(vtkMultiProcessController* controller);
protected:
  vtkLiveInsituLink();
  ~vtkLiveInsituLink();

  enum RMITags
    {
    UPDATE_RMI_TAG=8800,
    POSTPROCESS_RMI_TAG=8801,
    INITIALIZE_CONNECTION=8802
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
