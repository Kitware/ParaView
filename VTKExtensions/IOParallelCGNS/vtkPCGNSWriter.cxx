// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Maritime Research Institute Netherlands (MARIN)
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPCGNSWriter.h"

#include "vtkAppendDataSets.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <map>
#include <sstream>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
void Flatten(const vtkSmartPointer<vtkPartitionedDataSet>& mergedPD,
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
          mergedPD->SetPartition(item++, dataObject);
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

// A multiblock dataset may consist of nested blocks in blocks. This is not
// something the CGNS standard supports. Therefore, flatten nested blocks into
// a list of blocks.
//------------------------------------------------------------------------------
void Flatten(const vtkSmartPointer<vtkMultiBlockDataSet>& mergedMB,
  const std::vector<vtkSmartPointer<vtkDataObject>>& collected)
{
  if (collected.empty())
  {
    return;
  }

  auto mbFirst = vtkMultiBlockDataSet::SafeDownCast(collected.front());
  if (mbFirst)
  {
    for (unsigned int i = 0; i < mbFirst->GetNumberOfBlocks(); ++i)
    {
      auto block = mbFirst->GetBlock(i);
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
          auto mb = vtkMultiBlockDataSet::SafeDownCast(entry);
          if (mb)
          {
            auto grid = mb->GetBlock(i);

            // the following is needed to distinguish
            // between surface and volume grids in the CGNSWriter
            // => set the output data type
            append->SetOutputDataSetType(grid->GetDataObjectType());
            append->AddInputData(grid);
          }
        }

        append->Update(); // perform the merge!
        mergedMB->SetBlock(i, append->GetOutputDataObject(0));
        if (mbFirst->HasMetaData(i))
        {
          mergedMB->GetMetaData(i)->Append(mbFirst->GetMetaData(i));
        }
      }
      else if (block->IsA("vtkMultiPieceDataSet"))
      {
        std::vector<vtkSmartPointer<vtkDataObject>> sub;
        for (auto& entry : collected)
        {
          sub.push_back((vtkMultiBlockDataSet::SafeDownCast(entry)->GetBlock(i)));
        }
        vtkNew<vtkMultiPieceDataSet> mergedSub;
        ::Flatten(mergedSub.GetPointer(), sub);
        mergedMB->SetBlock(i, mergedSub);
        if (mbFirst->HasMetaData(i))
        {
          mergedMB->GetMetaData(i)->Append(mbFirst->GetMetaData(i));
        }
      }
      else if (block->IsA("vtkMultiBlockDataSet"))
      {
        std::vector<vtkSmartPointer<vtkDataObject>> sub;
        for (auto& entry : collected)
        {
          sub.push_back(vtkMultiBlockDataSet::SafeDownCast(entry)->GetBlock(i));
        }

        vtkNew<vtkMultiBlockDataSet> mergedSub;
        ::Flatten(mergedSub.GetPointer(), sub);
        mergedMB->SetBlock(i, mergedSub);
        if (mbFirst->HasMetaData(i))
        {
          mergedMB->GetMetaData(i)->Append(mbFirst->GetMetaData(i));
        }
      }
      else
      {
        vtkLog(ERROR, "Block with data type '" << block->GetClassName() << "' not supported.");
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------
void Flatten(const vtkSmartPointer<vtkPartitionedDataSetCollection>& mergedPCD,
  const std::vector<vtkSmartPointer<vtkDataObject>>& collected)
{
  if (collected.empty())
  {
    return;
  }

  auto pdcFirst = vtkPartitionedDataSetCollection::SafeDownCast(collected.front());
  const unsigned numberOfPartitionedDatasets = pdcFirst->GetNumberOfPartitionedDataSets();
  mergedPCD->SetNumberOfPartitionedDataSets(numberOfPartitionedDatasets);

  for (unsigned i = 0; i < numberOfPartitionedDatasets; ++i)
  {
    std::vector<vtkSmartPointer<vtkDataObject>> sub;
    for (auto& entry : collected)
    {
      sub.push_back(vtkPartitionedDataSetCollection::SafeDownCast(entry)->GetPartitionedDataSet(i));
    }
    vtkNew<vtkPartitionedDataSet> mergedSub;
    ::Flatten(mergedSub.GetPointer(), sub);
    mergedPCD->SetPartitionedDataSet(i, mergedSub);
    if (pdcFirst->HasMetaData(i))
    {
      mergedPCD->GetMetaData(i)->Append(pdcFirst->GetMetaData(i));
    }
  }
}

} // anonymous namespace

//------------------------------------------------------------------------------
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
      // set the information present on the original input
      toWrite->SetInformation(myOriginalInput->GetInformation());
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
