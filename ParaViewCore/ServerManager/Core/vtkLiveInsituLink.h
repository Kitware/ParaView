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

#include "vtkSMObject.h"
#include "vtkSmartPointer.h"

class vtkMultiProcessController;
class vtkSMSessionProxyManager;
class vtkPVXMLElement;

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
//BTX
protected:
  vtkLiveInsituLink();
  ~vtkLiveInsituLink();

  enum RMITags
    {
    UPDATE_RMI_TAG=8800,
    POSTPROCESS_RMI_TAG=8800,
    };


  void InitializeVisualization();
  void InitializeSimulation();

  void OnConnectionCreatedEvent();
  void InsituProcessConnected(vtkMultiProcessController* controller);

  char* Hostname;
  int InsituPort;
  int ProcessType;
  vtkMultiProcessController* Controller;

  char* InsituXMLState;
  vtkSmartPointer<vtkPVXMLElement> XMLState;

  void SetController(vtkMultiProcessController*);

private:
  vtkLiveInsituLink(const vtkLiveInsituLink&); // Not implemented
  void operator=(const vtkLiveInsituLink&); // Not implemented

  void SetProxyManager(vtkSMSessionProxyManager*);
  vtkSMSessionProxyManager* ProxyManager;

  vtkSetStringMacro(URL);
  char* URL;
//ETX
};

#endif
