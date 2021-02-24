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
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessControllerHelper.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSocketController.h"
#include "vtkStructuredGrid.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"

#include <assert.h>

vtkStandardNewMacro(vtkExtractsDeliveryHelper);
//----------------------------------------------------------------------------
vtkExtractsDeliveryHelper::vtkExtractsDeliveryHelper()
  : ProcessIsProducer(true)
  , NumberOfSimulationProcesses(0)
  , NumberOfVisualizationProcesses(0)
{
  this->SetParallelController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkExtractsDeliveryHelper::~vtkExtractsDeliveryHelper() = default;

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::SetSimulation2VisualizationController(vtkSocketController* cont)
{
  if (this->Simulation2VisualizationController != cont)
  {
    this->Simulation2VisualizationController = cont;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::SetParallelController(vtkMultiProcessController* cont)
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
void vtkExtractsDeliveryHelper::AddExtractConsumer(const char* key, vtkTrivialProducer* consumer)
{
  assert(this->ProcessIsProducer == false);
  assert(key != nullptr && consumer != nullptr);

  this->ExtractConsumers[key] =
    std::make_pair<vtkSmartPointer<vtkTrivialProducer>, bool>(consumer, false);
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::RemoveExtractConsumer(const char* key)
{
  this->ExtractConsumers.erase(key);
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::AddExtractProducer(
  const char* key, vtkAlgorithmOutput* producerPort)
{
  assert(this->ProcessIsProducer == true);
  assert(key != nullptr && producerPort != nullptr);

  this->ExtractProducers[key] = producerPort;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExtractsDeliveryHelper::Collect(int node_count, vtkDataObject* dObj)
{
  int numProcs = this->ParallelController->GetNumberOfProcesses();
  int myId = this->ParallelController->GetLocalProcessId();
  if (myId >= node_count)
  {
    int destination = myId % node_count;
    this->ParallelController->Send(dObj, destination, 13001);
    return nullptr;
  }
  else
  {
    std::vector<vtkDataObject*> pieces;
    vtkDataObject* clone = dObj->NewInstance();
    clone->ShallowCopy(dObj);
    pieces.push_back(clone);

    for (int cc = 1; myId + cc * node_count < numProcs; cc++)
    {
      vtkDataObject* piece =
        this->ParallelController->ReceiveDataObject(vtkMultiProcessController::ANY_SOURCE, 13001);
      if (piece)
      {
        pieces.push_back(piece);
      }
    }

    vtkDataObject* result = nullptr;
    if (pieces.size() > 1)
    {
      result = vtkMultiProcessControllerHelper::MergePieces(
        &pieces[0], static_cast<unsigned int>(pieces.size()));
    }
    else
    {
      result = dObj;
      dObj->Register(this);
    }

    for (size_t cc = 0; cc < pieces.size(); cc++)
    {
      pieces[cc]->Delete();
      pieces[cc] = nullptr;
    }

    return result;
  }
}

//----------------------------------------------------------------------------
bool vtkExtractsDeliveryHelper::Update()
{
  bool retVal = true;
  if (this->ProcessIsProducer)
  {
    //    cout << "Push extracts for: " << endl;
    //    for (ExtractProducersType::iterator iter = this->ExtractProducers.begin();
    //      iter != this->ExtractProducers.end(); ++iter)
    //      {
    //      cout << "  " << iter->first.c_str() << endl;
    //      }

    // update all inputs. We shouldn't call Update() here since that messes up
    // the time/piece requests that'd be set by paraview. The co-processing code
    // should ensure all pipelines are updated.
    // for (iter = this->ExtractProducers.begin();
    //  iter != this->ExtractProducers.end(); ++iter)
    //  {
    //  iter->second->GetProducer()->Update();
    //  }

    // reduce to N procs where N is the number of Vis procs.
    int M = this->NumberOfSimulationProcesses;
    int N = this->NumberOfVisualizationProcesses;

    std::map<std::string, vtkSmartPointer<vtkDataObject> > gathered_extracts;
    if (M > N)
    {
      // when simulation processes in greater than vis processes, the simulation
      // processes will gather data on the first N processes and then ship that
      // over.
      for (ExtractProducersType::iterator iter = this->ExtractProducers.begin();
           iter != this->ExtractProducers.end(); ++iter)
      {
        vtkDataObject* dObj = this->Collect(
          N, iter->second->GetProducer()->GetOutputDataObject(iter->second->GetIndex()));
        gathered_extracts[iter->first].TakeReference(dObj);
      }
    }

    if (N >= M)
    {
      // totally acceptable case, nothing special to do. Only the first M
      // visualization processes have data. One can use D3 for load balancing.
    }

    vtkSocketController* comm = this->Simulation2VisualizationController;
    if (comm)
    {
      for (ExtractProducersType::iterator iter = this->ExtractProducers.begin();
           iter != this->ExtractProducers.end(); ++iter)
      {
        vtkMultiProcessStream stream;
        stream << iter->first;
        comm->Send(stream, 1, 12000);
        vtkDataObject* dObj = (M > N)
          ? gathered_extracts[iter->first].GetPointer()
          : iter->second->GetProducer()->GetOutputDataObject(iter->second->GetIndex());
        comm->Send(dObj, 1, 12001);
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
      std::vector<vtkSmartPointer<vtkCompositeDataSet> > compositeDSToShare;
      vtkMultiProcessStream data_types_stream;
      while (true)
      {
        int needToShare = 0;
        std::string key;
        vtkMultiProcessStream stream;
        comm->Receive(stream, 1, 12000);
        stream >> key;
        if (key == "null")
        {
          break;
        }
        //        cout << "Received extract for: " << key.c_str() << endl;
        vtkDataObject* extract = comm->ReceiveDataObject(1, 12001);
        ExtractConsumersType::iterator iter;
        iter = this->ExtractConsumers.find(key);
        if (iter != this->ExtractConsumers.end())
        {
          iter->second.first->SetOutput(extract);
          iter->second.second = true;
        }
        else
        {
          vtkWarningMacro("Received unidentified extract " << key.c_str() << ". Ignoring.");
        }

        // Composite dataset need to convey their data structure across
        // processes, let's create those empty data object with the proper
        // data structure to share ONLY if needed.
        if (extract->IsA("vtkCompositeDataSet"))
        {
          vtkCompositeDataSet* dsToShare = vtkCompositeDataSet::SafeDownCast(
            vtkDataObjectTypes::NewDataObject(extract->GetClassName()));
          compositeDSToShare.push_back(dsToShare);
          dsToShare->CopyStructure(vtkCompositeDataSet::SafeDownCast(extract));
          dsToShare->FastDelete();
          needToShare = 1;
        }
        data_types_stream << key.c_str() << extract->GetClassName() << needToShare;
        extract->Delete();
      }
      data_types_stream << "null";
      this->ParallelController->Broadcast(data_types_stream, 0);

      // Send the empty data object that need to share its structure
      std::vector<vtkSmartPointer<vtkCompositeDataSet> >::iterator dsIter;
      for (dsIter = compositeDSToShare.begin(); dsIter != compositeDSToShare.end(); dsIter++)
      {
        this->ParallelController->Broadcast(dsIter->GetPointer(), 0);
      }
    }
    else
    {
      vtkMultiProcessStream data_types_stream;
      this->ParallelController->Broadcast(data_types_stream, 0);
      std::string key;
      int needToReceiveDataObject;
      data_types_stream >> key;
      while (key != "null")
      {
        std::string data_type;
        data_types_stream >> data_type >> needToReceiveDataObject;

        ExtractConsumersType::iterator iter = this->ExtractConsumers.find(key);
        if (iter != this->ExtractConsumers.end())
        {
          vtkDataObject* dObj = vtkDataObjectTypes::NewDataObject(data_type.c_str());

          // Fill with proper data structure if needed
          if (needToReceiveDataObject != 0)
          {
            this->ParallelController->Broadcast(dObj, 0);
          }

          iter->second.first->SetOutput(dObj);
          iter->second.second = true;
          dObj->FastDelete();
        }
        else
        {
          vtkWarningMacro("Received unidentified extract " << key.c_str() << ". Ignoring.");
        }

        // Move forward
        data_types_stream >> key;
      }
    }
    // figure out if we have all of the extracted data objects on the server
    for (ExtractConsumersType::iterator iter = this->ExtractConsumers.begin();
         iter != this->ExtractConsumers.end(); iter++)
    {
      retVal = retVal && iter->second.second;
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
void vtkExtractsDeliveryHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
