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
/**
 * @class   vtkExtractsDeliveryHelper
 *
 *
*/

#ifndef vtkExtractsDeliveryHelper_h
#define vtkExtractsDeliveryHelper_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"                 // needed for smart pointer

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkMultiProcessController;
class vtkSocketController;
class vtkTrivialProducer;

#include <map>    // needed for typedef
#include <string> // needed for typedef

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkExtractsDeliveryHelper : public vtkObject
{
public:
  static vtkExtractsDeliveryHelper* New();
  vtkTypeMacro(vtkExtractsDeliveryHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  void RemoveExtractConsumer(const char* key);

  void AddExtractProducer(const char* key, vtkAlgorithmOutput* producerPort);

  /**
   * Returns true if the data has been made available.
   */
  bool Update();

  vtkSetMacro(NumberOfVisualizationProcesses, int);
  vtkGetMacro(NumberOfVisualizationProcesses, int);
  vtkSetMacro(NumberOfSimulationProcesses, int);
  vtkGetMacro(NumberOfSimulationProcesses, int);

protected:
  vtkExtractsDeliveryHelper();
  ~vtkExtractsDeliveryHelper() override;

  vtkDataObject* Collect(int nodes_to_collect_to, vtkDataObject*);

  bool ProcessIsProducer;
  int NumberOfSimulationProcesses;
  int NumberOfVisualizationProcesses;

  // the bool is to keep track of whether the trivial producer has had
  // its output set yet. we don't want to update the pipeline until
  // it gets its output.
  typedef std::map<std::string, std::pair<vtkSmartPointer<vtkTrivialProducer>, bool> >
    ExtractConsumersType;
  ExtractConsumersType ExtractConsumers;

  typedef std::map<std::string, vtkSmartPointer<vtkAlgorithmOutput> > ExtractProducersType;
  ExtractProducersType ExtractProducers;

  vtkSmartPointer<vtkSocketController> Simulation2VisualizationController;
  vtkSmartPointer<vtkMultiProcessController> ParallelController;

private:
  vtkExtractsDeliveryHelper(const vtkExtractsDeliveryHelper&) = delete;
  void operator=(const vtkExtractsDeliveryHelper&) = delete;
};

#endif
