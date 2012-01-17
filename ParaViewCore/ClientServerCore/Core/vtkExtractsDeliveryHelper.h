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
// .NAME vtkExtractsDeliveryHelper
// .SECTION Description
//

#ifndef __vtkExtractsDeliveryHelper_h
#define __vtkExtractsDeliveryHelper_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkObject.h"
#include "vtkSmartPointer.h"

class vtkAlgorithmOutput;
class vtkSocketController;
class vtkMultiProcessController;
class vtkTrivialProducer;

#include <map>
#include <string>

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkExtractsDeliveryHelper : public vtkObject
{
public:
  static vtkExtractsDeliveryHelper* New();
  vtkTypeMacro(vtkExtractsDeliveryHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(ProcessIsProducer, bool);
  vtkGetMacro(ProcessIsProducer, bool);

  // Controller to used to communicate between sim and viz.
  void SetSimulation2VisualizationController(vtkSocketController*);

  // The MPI communicator to communicate between the process in the process
  // group. This is only used on the simulation processes.
  void SetParallelController(vtkMultiProcessController*);

  // Reset all information about extracts.
  void ClearAllExtracts();

  // Register extracts. This method is used on the Visualization processes to
  // register the producer
  void AddExtractConsumer(const char* key, vtkTrivialProducer* consumer);
  void AddExtractProducer(const char* key, vtkAlgorithmOutput* producerPort);

  void Update();

  vtkSetMacro(NumberOfVisualizationProcesses, int);
  vtkSetMacro(NumberOfSimulationProcesses, int);

//BTX
protected:
  vtkExtractsDeliveryHelper();
  ~vtkExtractsDeliveryHelper();

  bool ProcessIsProducer;
  int NumberOfSimulationProcesses;
  int NumberOfVisualizationProcesses;

  std::map<std::string, vtkSmartPointer<vtkTrivialProducer> > ExtractConsumers;
  std::map<std::string, vtkSmartPointer<vtkAlgorithmOutput> > ExtractProducers;

  vtkSmartPointer<vtkSocketController> Simulation2VisualizationController;
  vtkSmartPointer<vtkMultiProcessController> ParallelController;

private:
  vtkExtractsDeliveryHelper(const vtkExtractsDeliveryHelper&); // Not implemented
  void operator=(const vtkExtractsDeliveryHelper&); // Not implemented
//ETX
};

#endif
