/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPCGNSWriter.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
Copyright (c) Maritime Research Institute Netherlands (MARIN)
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPCGNSWriter.h"

#include <vtkAppendDataSets.h>
#include <vtkDataObject.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkMPIController.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <map>
#include <sstream>
#include <vector>

namespace
{
// A multiblock dataset may consist of nested blocks in blocks. This is not
// something the CGNS standard supports. Therefore, flatten nested blocks into
// a list of blocks.
//------------------------------------------------------------------------------
void Flatten(const vtkSmartPointer<vtkMultiBlockDataSet>& merged,
  const std::vector<vtkSmartPointer<vtkDataObject>>& collected)
{
  if (collected.empty())
    return;

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(collected.front());
  if (mb)
  {
    for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i)
    {
      auto block = mb->GetBlock(i);
      if (!block)
      {
        continue;
      }

      if (block->IsA("vtkPointSet"))
      {
        vtkNew<vtkAppendDataSets> append;
        append->SetMergePoints(true);

        for (auto& entry : collected)
        {
          vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(entry);
          if (mb2)
          {
            auto grid = mb2->GetBlock(i);

            // the following is needed to distinguish
            // between surface and volume grids in the CGNSWriter
            // => set the output data type
            append->SetOutputDataSetType(grid->GetDataObjectType());
            append->AddInputData(grid);
          }
        }

        append->Update(); // perform the merge!
        merged->SetBlock(i, append->GetOutputDataObject(0));
        merged->GetMetaData(i)->Append(mb->GetMetaData(i));
      }
      else if (block->IsA("vtkMultiBlockDataSet"))
      {
        std::vector<vtkSmartPointer<vtkDataObject>> sub;
        for (auto& entry : collected)
        {
          sub.push_back(vtkSmartPointer<vtkDataObject>::Take(
            vtkMultiBlockDataSet::SafeDownCast(entry)->GetBlock(i)));
        }

        auto mergedSub = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        ::Flatten(mergedSub, sub);
        merged->SetBlock(i, mergedSub);
        merged->GetMetaData(i)->Append(mb->GetMetaData(i));
      }
    }
  }
}

void Flatten(const vtkSmartPointer<vtkPartitionedDataSet>& merged,
  const std::vector<vtkSmartPointer<vtkDataObject>>& collected)
{
  unsigned item(0);
  for (auto& entry : collected)
  {
    vtkPartitionedDataSet* partition = vtkPartitionedDataSet::SafeDownCast(entry);
    if (partition)
    {
      const unsigned nPartitions = partition->GetNumberOfPartitions();
      for (unsigned i = 0; i < nPartitions; ++i)
      {
        vtkDataObject* dataObject = partition->GetPartitionAsDataObject(i);
        if (dataObject)
        {
          merged->SetPartition(item++, dataObject);
        }
      }
    }
    else
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Expected a vtkPartitionedDataSet, got " << entry->GetClassName());
    }
  }
}

void Flatten(const vtkSmartPointer<vtkPartitionedDataSetCollection>& mergedCollection,
  const std::vector<vtkSmartPointer<vtkDataObject>>& collected)
{
  for (auto& entry : collected)
  {
    vtkPartitionedDataSetCollection* partitionedCollection =
      vtkPartitionedDataSetCollection::SafeDownCast(entry);
    if (partitionedCollection)
    {
      const unsigned nDataSets = partitionedCollection->GetNumberOfPartitionedDataSets();
      mergedCollection->SetNumberOfPartitionedDataSets(nDataSets);

      for (unsigned idx = 0; idx < nDataSets; ++idx)
      {
        const auto merged = mergedCollection->GetPartitionedDataSet(idx);
        const unsigned nPartitions = partitionedCollection->GetNumberOfPartitions(idx);
        for (unsigned i = 0; i < nPartitions; ++i)
        {
          vtkDataObject* dataObject = partitionedCollection->GetPartitionAsDataObject(idx, i);
          if (dataObject)
          {
            merged->SetPartition(i, dataObject);
          }
        }
        if (partitionedCollection->HasMetaData(idx))
        {
          mergedCollection->GetMetaData(idx)->Append(partitionedCollection->GetMetaData(idx));
        }
      }
    }
    else
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Expected a vtkPartitionedDataSetCollection, got " << entry->GetClassName());
    }
  }
}

} // anonymous namespace

vtkObjectFactoryNewMacro(vtkPCGNSWriter);

//------------------------------------------------------------------------------
vtkPCGNSWriter::vtkPCGNSWriter()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
void vtkPCGNSWriter::SetController(vtkMultiProcessController* controller)
{
  vtkMPIController* mpicontroller = vtkMPIController::SafeDownCast(controller);
  if (controller && !mpicontroller)
  {
    vtkErrorMacro(<< "Only vtkMPIController supported as multi-process controller.");
    return;
  }
  vtkSetSmartPointerBodyMacro(Controller, vtkMultiProcessController, controller);

  if (mpicontroller)
  {
    this->NumberOfPieces = mpicontroller->GetNumberOfProcesses();
    this->RequestPiece = mpicontroller->GetLocalProcessId();
  }
  else
  {
    this->NumberOfPieces = 0;
    this->RequestPiece = -1;
  }
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkPCGNSWriter::GetController()
{
  return this->Controller;
};

//------------------------------------------------------------------------------
void vtkPCGNSWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of pieces " << this->NumberOfPieces << endl;
  os << indent << "Request piece " << this->RequestPiece << endl;
  os << indent << "Controller ";
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//------------------------------------------------------------------------------
int vtkPCGNSWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkPCGNSWriter::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  return vtkCGNSWriter::RequestInformation(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkPCGNSWriter::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // call base-class to set the time values
  vtkCGNSWriter::RequestUpdateExtent(request, inputVector, outputVector);

  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->RequestPiece);

  return 1;
}

//------------------------------------------------------------------------------
void vtkPCGNSWriter::WriteData()
{
  vtkMPIController* mpicontroller = vtkMPIController::SafeDownCast(this->Controller);
  if (!mpicontroller)
  {
    vtkErrorMacro(<< "No MPI controller found");
    this->SetErrorCode(2L);
    return;
  }
  if (!this->OriginalInput)
  {
    this->SetErrorCode(3L);
    return;
  }

  this->WasWritingSuccessful = false;

  std::vector<vtkSmartPointer<vtkDataObject>> collected;
  // what happens in the Gather step is that each part is
  // serialized on its processor using vtkUnstructuredGridWriter
  // that writes in binary to a string. This string is then
  // sent to the master process and re-assembled into
  // the vector of vtkDataObject pointers. These can then
  // be appended/merged into one dataset and written to disk
  // using the serial CGNS writer.
  if (0 != mpicontroller->Gather(this->OriginalInput, collected, 0))
  {
    if (mpicontroller->GetLocalProcessId() == 0)
    {
      // append/merge into one data set and use
      // the superclass' WriteData on that.
      vtkDataObject* toWrite(nullptr);
      if (this->OriginalInput->IsA("vtkPointSet"))
      {
        vtkNew<vtkAppendDataSets> appender;
        appender->SetMergePoints(true);
        for (auto& entry : collected)
        {
          appender->AddInputData(entry);
        }

        appender->Update();
        toWrite = appender->GetOutputDataObject(0);
        toWrite->Register(this);
      }
      else if (this->OriginalInput->IsA("vtkMultiBlockDataSet"))
      {
        vtkNew<vtkMultiBlockDataSet> mb;
        ::Flatten(mb.GetPointer(), collected);
        toWrite = mb;
        toWrite->Register(this);
      }
      else if (this->OriginalInput->IsA("vtkPartitionedDataSet"))
      {
        vtkNew<vtkPartitionedDataSet> partitioned;
        ::Flatten(partitioned.GetPointer(), collected);
        toWrite = partitioned;
        toWrite->Register(this);
      }
      else if (this->OriginalInput->IsA("vtkPartitionedDataSetCollection"))
      {
        vtkNew<vtkPartitionedDataSetCollection> partitionedCollection;
        ::Flatten(partitionedCollection.GetPointer(), collected);
        toWrite = partitionedCollection;
        toWrite->Register(this);
      }
      else
      {
        vtkErrorMacro(<< "Unable to write data object of class "
                      << this->OriginalInput->GetClassName());
        this->WasWritingSuccessful = false;
        return;
      }

      // exchange OriginalInput and write with the superclass
      vtkDataObject* myOriginalInput = this->OriginalInput;
      this->OriginalInput = toWrite;
      vtkCGNSWriter::WriteData();
      this->OriginalInput = myOriginalInput;
      toWrite->UnRegister(this);
    }
    else
    {
      this->WasWritingSuccessful = true;
    }
  }

  if (!this->WriteAllTimeSteps && this->TimeValues)
  {
    this->TimeValues->Delete();
    this->TimeValues = nullptr;
  }
}
