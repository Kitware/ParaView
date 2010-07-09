/*=========================================================================

  Program:   ParaView
  Module:    vtkTableStreamer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTableStreamer.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkUnsignedIntArray.h"

static void vtkFillComponent(vtkUnsignedIntArray* array,
  int component, unsigned int value)
{
  vtkIdType numTuples = array->GetNumberOfTuples();
  int numComps = array->GetNumberOfComponents();
  unsigned int *ptr = array->GetPointer(0);
  for (vtkIdType cc=component; cc < numTuples*numComps; cc+=numComps)
    {
    ptr[cc] = value;
    }
}
 
vtkStandardNewMacro(vtkTableStreamer);
vtkCxxSetObjectMacro(vtkTableStreamer, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkTableStreamer::vtkTableStreamer()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->BlockSize = 1024;
  this->Block = 0;
}

//----------------------------------------------------------------------------
vtkTableStreamer::~vtkTableStreamer()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkTableStreamer::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkTableStreamer::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkTableStreamer::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inInfo);
  vtkTable* inputTable = vtkTable::GetData(inInfo);
  vtkDataObject* newOutput = 0;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (inputCD)
    {
    if (vtkMultiBlockDataSet::GetData(outInfo))
      {
      return 1;
      }
    newOutput = vtkMultiBlockDataSet::New();
    }
  else if (inputTable)
    {
    if (vtkTable::GetData(outInfo))
      {
      return 1;
      }
    newOutput = vtkTable::New();
    }
  if (newOutput)
    {
    newOutput->SetPipelineInformation(outInfo);
    newOutput->Delete();
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTableStreamer::RequestData(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkstd::vector<vtkstd::pair<vtkIdType, vtkIdType> > indices;
  if (!this->DetermineIndicesToPass(inputDO, indices))
    {
    return 0;
    }

  vtkSmartPointer<vtkCompositeDataSet> input =
    vtkCompositeDataSet::SafeDownCast(inputDO);
  if (!input)
    {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
    mb->SetBlock(0, inputDO);
    input = mb;
    mb->Delete();
    }

  vtkSmartPointer<vtkMultiBlockDataSet> output =
    vtkMultiBlockDataSet::SafeDownCast(outputDO);
  if (!output)
    {
    output = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    }
  output->CopyStructure(input);

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->SkipEmptyNodesOff();

  bool something_added = false;
  int cc=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem(), cc++)
    {
    vtkTable* curTable = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    vtkIdType curOffset = indices[cc].first;
    vtkIdType curCount = indices[cc].second;
    if (curCount <= 0)
      {
      continue;
      }

    something_added = true;
    vtkTable* outTable = vtkTable::New();
    output->SetDataSet(iter, outTable);
    outTable->Delete();

    outTable->GetRowData()->CopyAllocate(curTable->GetRowData());
    outTable->GetRowData()->SetNumberOfTuples(curCount);

    int dimensions[3] = {0, 0, 0};
    vtkSmartPointer<vtkIdTypeArray> structuredIndices;
    if (curTable->GetFieldData()->GetArray("STRUCTURED_DIMENSIONS"))
      {
      vtkIntArray::SafeDownCast(
        curTable->GetFieldData()->GetArray("STRUCTURED_DIMENSIONS"))->
        GetTupleValue(0, dimensions);
      structuredIndices = vtkSmartPointer<vtkIdTypeArray>::New();
      structuredIndices->SetNumberOfComponents(3);
      structuredIndices->SetNumberOfTuples(curCount);
      structuredIndices->SetName("Structured Coordinates");
      }

    vtkSmartPointer<vtkUnsignedIntArray> compositeIndex;
    if (iter->GetCurrentMetaData()->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
      iter->GetCurrentMetaData()->Has(vtkSelectionNode::HIERARCHICAL_INDEX()))
      {
      compositeIndex = vtkSmartPointer<vtkUnsignedIntArray>::New();
      compositeIndex->SetName("vtkCompositeIndexArray");
      compositeIndex->SetNumberOfComponents(2);
      compositeIndex->SetNumberOfTuples(curCount);
      ::vtkFillComponent(compositeIndex, 0, static_cast<unsigned int>(
          iter->GetCurrentMetaData()->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())));
      ::vtkFillComponent(compositeIndex, 1, static_cast<unsigned int>(
          iter->GetCurrentMetaData()->Get(vtkSelectionNode::HIERARCHICAL_INDEX())));

      }
    else if (iter->GetCurrentMetaData()->Has(vtkSelectionNode::COMPOSITE_INDEX()))
      {
      compositeIndex = vtkSmartPointer<vtkUnsignedIntArray>::New();
      compositeIndex->SetName("vtkCompositeIndexArray");
      compositeIndex->SetNumberOfComponents(1);
      compositeIndex->SetNumberOfTuples(curCount);
      ::vtkFillComponent(compositeIndex, 0, static_cast<unsigned int>(
          iter->GetCurrentMetaData()->Get(vtkSelectionNode::COMPOSITE_INDEX())));
      }

    // TODO: add Hierarchical index information.
    for (vtkIdType jj=0; jj < curCount; jj++)
      {
      vtkIdType inIndex = curOffset+jj;
      outTable->GetRowData()->CopyData(
        curTable->GetRowData(), inIndex, jj);
      if (structuredIndices)
        {
        // Compute i,j,k from point id.
        vtkIdType tuple[3];
        tuple[0] = (inIndex % dimensions[0]);
        tuple[1] = (inIndex/dimensions[0]) % dimensions[1];
        tuple[2] = (inIndex/(dimensions[0]*dimensions[1]));
        structuredIndices->SetTupleValue(jj, tuple);
        }
      }
    if (structuredIndices)
      {
      outTable->GetRowData()->AddArray(structuredIndices);
      }
    if (compositeIndex)
      {
      outTable->GetRowData()->AddArray(compositeIndex);
      }
    }
  iter->Delete();
    
  if (!outputDO->IsA("vtkMultiBlockDataSet") && something_added)
    {
    outputDO->ShallowCopy(output->GetBlock(0));
    }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkTableStreamer::CountRows(vtkDataObject* dObj,
  vtkstd::vector<vtkIdType>& counts,
  vtkstd::vector<vtkIdType>& offsets)
{
  counts.clear();
  offsets.clear();

  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  vtkTable* table = vtkTable::SafeDownCast(dObj);
  if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    iter->SkipEmptyNodesOff();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkTable* cur_table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
      vtkIdType row_count =  cur_table? cur_table->GetNumberOfRows() : 0;
      counts.push_back(row_count);
      offsets.push_back(0);
      }
    iter->Delete();
    }
  else if (table)
    {
    counts.push_back(table->GetNumberOfRows());
    offsets.push_back(0);
    }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
    {
    int myId = this->Controller->GetLocalProcessId();
    int numProcs = this->Controller->GetNumberOfProcesses();
    vtkIdType *recvBuffer = new vtkIdType[counts.size()*numProcs];
    if (!this->Controller->AllGather(&counts[0], recvBuffer,
        static_cast<vtkIdType>(counts.size())))
      {
      vtkErrorMacro("Communication error.");
      counts.clear();
      return false;
      }
    vtkIdType* pRecvBuffer = recvBuffer;
    for (int cc=0; cc < numProcs; cc++)
      {
      if (cc == myId)
        {
        continue;
        }
      for (size_t jj=0; jj < counts.size(); jj++)
        {
        counts[jj] += *pRecvBuffer;
        if (cc < myId)
          {
          offsets[jj] += *pRecvBuffer;
          }
        pRecvBuffer++;
        }
      }
    delete [] recvBuffer;
    }
  return true;
}

//----------------------------------------------------------------------------
inline bool vtkTableStreamerIntersect(const vtkIdType& minX, const vtkIdType& maxX,
  const vtkIdType& minY, const vtkIdType& maxY,
  vtkIdType& offset, vtkIdType& count)
{
  if (maxX <= minY || minX >= maxY)
    {
    // no overlap at all.
    return false;
    }

  if (minX == maxX)
    {
    // no rows in current block.
    return false;
    }

  offset = (minY > minX)? (minY - minX) : 0;

  vtkIdType end = (maxX < maxY)? maxX : maxY;
  count = end - (minX + offset);
  return true;
}

//----------------------------------------------------------------------------
bool vtkTableStreamer::DetermineIndicesToPass(vtkDataObject* inputDO,
  vtkstd::vector<vtkstd::pair<vtkIdType, vtkIdType> >& result)
{
  vtkstd::vector<vtkIdType> counts;
  vtkstd::vector<vtkIdType> offsets;
  if (!this->CountRows(inputDO, counts, offsets))
    {
    return false;
    }

  vtkSmartPointer<vtkCompositeDataSet> input =
    vtkCompositeDataSet::SafeDownCast(inputDO);
  if (!input)
    {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
    mb->SetBlock(0, inputDO);
    input = mb;
    mb->Delete();
    }

  vtkIdType blockStartIndex = this->Block*this->BlockSize;
  vtkIdType blockEndIndex = blockStartIndex + this->BlockSize;
  // To pass: [blockStartIndex, blockEndIndex).

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->SkipEmptyNodesOff();

  vtkIdType offset = 0;
  int cc=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem(), cc++)
    {
    vtkTable* curTable = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    vtkIdType currentStartIndex = offset + offsets[cc];
    vtkIdType currentNumRows = curTable? curTable->GetNumberOfRows(): 0;
    vtkIdType currentEndIndex = currentStartIndex + currentNumRows;

    vtkIdType curOffset = 0, curCount = 0;
    if (vtkTableStreamerIntersect(currentStartIndex, currentEndIndex,
        blockStartIndex, blockEndIndex,
        curOffset, curCount))
      {
      vtkstd::pair<vtkIdType, vtkIdType> value(curOffset, curCount);
      result.push_back(value);
      }
    else
      {
      vtkstd::pair<vtkIdType, vtkIdType> value(0, 0);
      result.push_back(value);
      }
    offset += counts[cc];
    }
  iter->Delete();
  return true;
}

//----------------------------------------------------------------------------
void vtkTableStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


