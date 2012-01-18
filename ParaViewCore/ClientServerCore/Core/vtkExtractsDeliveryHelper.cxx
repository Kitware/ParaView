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
#include "vtkExtractsDeliveryHelper.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkSocketController.h"
#include "vtkTrivialProducer.h"

#include <assert.h>

vtkStandardNewMacro(vtkExtractsDeliveryHelper);
//----------------------------------------------------------------------------
vtkExtractsDeliveryHelper::vtkExtractsDeliveryHelper() :
  ProcessIsProducer(true),
  NumberOfSimulationProcesses(0),
  NumberOfVisualizationProcesses(0)
{
  this->SetParallelController(
    vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkExtractsDeliveryHelper::~vtkExtractsDeliveryHelper()
{
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::SetSimulation2VisualizationController(
  vtkSocketController* cont)
{
  if (this->Simulation2VisualizationController != cont)
    {
    this->Simulation2VisualizationController = cont;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::SetParallelController(
  vtkMultiProcessController* cont)
{
  if (this->ParallelController != cont)
    {
    this->ParallelController = cont;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::ClearAllExtracts()
{
  this->ExtractConsumers.clear();
  this->ExtractProducers.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::AddExtractConsumer(
  const char* key, vtkTrivialProducer* consumer)
{
  assert (this->ProcessIsProducer == false);
  assert (key != NULL && consumer != NULL);

  this->ExtractConsumers[key] = consumer;
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::AddExtractProducer(
  const char* key, vtkAlgorithmOutput* producerPort)
{
  assert (this->ProcessIsProducer == true);
  assert (key != NULL && producerPort != NULL);

  this->ExtractProducers[key] = producerPort;
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::Update()
{
  if (this->ProcessIsProducer)
    {
    cout << "Push extracts for: " << endl;
    ExtractProducersType::iterator iter;
    for (iter = this->ExtractProducers.begin();
      iter != this->ExtractProducers.end(); ++iter)
      {
      cout << "  " << iter->first.c_str() << endl;
      }

    // update all inputs.
    for (iter = this->ExtractProducers.begin();
      iter != this->ExtractProducers.end(); ++iter)
      {
      iter->second->GetProducer()->Update();
      }

    // reduce to N procs where N is the number of Vis procs.
    int M = this->NumberOfSimulationProcesses;
    int N = this->NumberOfVisualizationProcesses;
    if (M > N)
      {
      vtkWarningMacro("FIXME: Need to reduce M to N ");
      }

    vtkSocketController* comm = this->Simulation2VisualizationController;
    if (comm)
      {
      for (iter = this->ExtractProducers.begin();
        iter != this->ExtractProducers.end(); ++iter)
        {
        vtkMultiProcessStream stream;
        stream << iter->first;
        comm->Send(stream, 1, 12000);
        comm->Send(
          iter->second->GetProducer()->GetOutputDataObject(iter->second->GetIndex()),
          1, 12001);
        }
      // mark end.
      vtkMultiProcessStream stream;
      stream << std::string("null");
      comm->Send(stream, 1, 12000);
      }
    }
  else
    {
    vtkSocketController* comm = this->Simulation2VisualizationController;
    if (comm)
      {
      while (true)
        {
        std::string key;
        vtkMultiProcessStream stream;
        comm->Receive(stream, 1, 12000);
        stream >> key;
        if (key == "null")
          {
          break;
          }
        cout << "Received extract for: " << key.c_str() << endl;
        vtkDataObject* extract = comm->ReceiveDataObject(1, 12001);
        ExtractConsumersType::iterator iter;
        iter = this->ExtractConsumers.find(key);
        if (iter != this->ExtractConsumers.end())
          {
          iter->second->SetOutput(extract);
          }
        else
          {
          vtkWarningMacro("Received unidentified extract " <<
            key.c_str() << ". Ignoring.");
          }
        extract->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
