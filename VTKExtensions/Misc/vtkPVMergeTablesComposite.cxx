// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVMergeTablesComposite.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVMergeTables.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkPVMergeTablesComposite);
//----------------------------------------------------------------------------
vtkPVMergeTablesComposite::vtkPVMergeTablesComposite()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVMergeTablesComposite::~vtkPVMergeTablesComposite() = default;

//----------------------------------------------------------------------------
int vtkPVMergeTablesComposite::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPVMergeTablesComposite::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  int outputType = VTK_PARTITIONED_DATA_SET_COLLECTION;
  if (inputDO && inputDO->GetDataObjectType() == VTK_MULTIBLOCK_DATA_SET)
  {
    outputType = VTK_MULTIBLOCK_DATA_SET;
  }

  return vtkDataObjectAlgorithm::SetOutputDataObject(
           outputType, outputVector->GetInformationObject(0), /*exact*/ true)
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
int vtkPVMergeTablesComposite::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObjectTree* output = vtkDataObjectTree::GetData(outputVector, 0);
  assert(output);

  // let's start by collecting all non-trivial inputs to merge.
  std::vector<vtkDataObjectTree*> nonempty_inputs;
  const int num_connections = this->GetNumberOfInputConnections(0);
  for (int cc = 0; cc < num_connections; ++cc)
  {
    if (auto dObjTree = vtkDataObjectTree::GetData(inputVector[0], cc))
    {
      auto iter = dObjTree->NewIterator();
      iter->InitTraversal();
      if (!iter->IsDoneWithTraversal())
      {
        // non-empty
        nonempty_inputs.push_back(dObjTree);
      }
      iter->Delete();
    }
  }

  if (nonempty_inputs.empty())
  {
    return 1;
  }
  if (this->MergeStrategy == vtkPVMergeTablesComposite::ALL)
  {
    this->MergeAllTables(nonempty_inputs, output);
  }
  else // MergeStrategy == vtkPVMergeTablesComposite::LEAVES
  {
    this->MergeLeavesTables(nonempty_inputs, output);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVMergeTablesComposite::MergeLeavesTables(
  const std::vector<vtkDataObjectTree*>& inputs, vtkDataObjectTree* output)
{
  output->CopyStructure(inputs[0]);

  auto nameKey = vtkCompositeDataSet::NAME();
  if (output->IsA("vtkMultiBlockDataSet"))
  {
    auto iter = vtk::TakeSmartPointer(output->NewTreeIterator());
    iter->SkipEmptyNodesOff();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      std::vector<vtkTable*> leaves;
      const char* bname =
        iter->HasCurrentMetaData() ? iter->GetCurrentMetaData()->Get(nameKey) : nullptr;
      for (auto dObjTree : inputs)
      {
        if (auto* table = vtkTable::SafeDownCast(dObjTree->GetDataSet(iter)))
        {
          if (table->GetNumberOfRows() > 0 && table->GetNumberOfColumns() > 0)
          {
            leaves.push_back(table);
          }
          if (bname == nullptr && dObjTree->HasMetaData(iter))
          {
            bname = dObjTree->GetMetaData(iter)->Get(nameKey);
          }
        }
      }
      if (!leaves.empty())
      {
        auto result = vtkPVMergeTables::MergeTables(leaves);
        output->SetDataSet(iter, result);
        if (bname)
        {
          output->GetMetaData(iter)->Set(nameKey, bname);
        }
      }
    }
  }
  else // output->IsA("vtkPartitionedDataSetCollection")
  {
    auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(output);
    for (unsigned int i = 0; i < outputPDC->GetNumberOfPartitionedDataSets(); ++i)
    {
      const char* bname =
        outputPDC->HasChildMetaData(i) ? outputPDC->GetChildMetaData(i)->Get(nameKey) : nullptr;
      std::vector<vtkTable*> leaves;
      for (auto dObjTree : inputs)
      {
        auto inputPDCI = vtkPartitionedDataSetCollection::SafeDownCast(dObjTree);
        if (bname == nullptr && inputPDCI->HasChildMetaData(i))
        {
          bname = inputPDCI->GetChildMetaData(i)->Get(nameKey);
        }
        for (vtkIdType j = 0; j < inputPDCI->GetNumberOfPartitions(i); ++j)
        {
          if (auto table = vtkTable::SafeDownCast(inputPDCI->GetPartitionAsDataObject(i, j)))
          {
            if (table->GetNumberOfRows() > 0 && table->GetNumberOfColumns() > 0)
            {
              leaves.push_back(table);
            }
          }
        }
      }
      if (!leaves.empty())
      {
        auto result = vtkPVMergeTables::MergeTables(leaves);
        outputPDC->SetNumberOfPartitions(i, 1);
        outputPDC->SetPartition(i, 0, result);
        if (bname)
        {
          output->GetChildMetaData(i)->Set(nameKey, bname);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVMergeTablesComposite::MergeAllTables(
  const std::vector<vtkDataObjectTree*>& inputs, vtkDataObjectTree* output)
{
  std::vector<vtkTable*> tables;
  for (const auto& input : inputs)
  {
    auto iter = vtk::TakeSmartPointer(input->NewTreeIterator());
    iter->SkipEmptyNodesOff();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
      if (table && table->GetNumberOfRows() > 0 && table->GetNumberOfColumns() > 0)
      {
        tables.push_back(table);
      }
    }
  }
  auto mergedTable = vtkPVMergeTables::MergeTables(tables);
  if (auto mb = vtkMultiBlockDataSet::SafeDownCast(output))
  {
    mb->SetBlock(0, mergedTable);
  }
  else if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(output))
  {
    pdc->SetPartition(0, 0, mergedTable);
  }
}

//----------------------------------------------------------------------------
void vtkPVMergeTablesComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
