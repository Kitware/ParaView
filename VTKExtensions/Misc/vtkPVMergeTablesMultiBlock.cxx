/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMergeTablesMultiBlock.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMergeTablesMultiBlock.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkPVMergeTablesMultiBlock);
//----------------------------------------------------------------------------
vtkPVMergeTablesMultiBlock::vtkPVMergeTablesMultiBlock()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVMergeTablesMultiBlock::~vtkPVMergeTablesMultiBlock()
{
}

//----------------------------------------------------------------------------
int vtkPVMergeTablesMultiBlock::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
static vtkSmartPointer<vtkTable> vtkPVMergeTablesMultiBlockMerge(
  const std::vector<vtkTable*>& inputs)
{
  assert(inputs.size() > 0);
  if (inputs.size() == 1)
  {
    return inputs[0];
  }

  auto result = vtkSmartPointer<vtkTable>::New();
  auto resultRowData = result->GetRowData();
  result->DeepCopy(inputs[0]);

  // resize the table to avoid resize over and over again.
  vtkIdType numrows = 0;
  for (vtkTable* table : inputs)
  {
    numrows += table->GetNumberOfRows();
  }
  for (int cc = 0, max = resultRowData->GetNumberOfArrays(); cc < max; cc++)
  {
    // note: this does not update MaxId i.e. resultRowData->GetNumberOfTuples()
    // remains unchanged.
    resultRowData->GetAbstractArray(cc)->Resize(numrows);
  }

  for (size_t idx = 1; idx < inputs.size(); ++idx)
  {
    auto table = inputs[idx];
    assert(table && table->GetNumberOfColumns() > 0 && table->GetNumberOfRows() > 0);

    auto resultCount = resultRowData->GetNumberOfTuples();
    auto tableRowData = table->GetRowData();
    auto tableCount = tableRowData->GetNumberOfTuples();

    // note: won't cause any resizes, since we resized already.
    resultRowData->SetNumberOfTuples(resultCount + tableCount);
    for (vtkIdType cc = 0; cc < tableCount; ++cc)
    {
      resultRowData->SetTuple(cc + resultCount, cc, tableRowData);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkPVMergeTablesMultiBlock::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  assert(output);
  output->CopyStructure(vtkDataObjectTree::GetData(inputVector[0], 0));

  // let's start by collecting all non-trivial inputs to merge.
  std::vector<vtkDataObjectTree*> nonempty_inputs;
  const int num_connections = this->GetNumberOfInputConnections(0);
  for (int cc = 0; cc < num_connections; ++cc)
  {
    if (auto cd = vtkDataObjectTree::GetData(inputVector[0], cc))
    {
      auto iter = cd->NewIterator();
      iter->InitTraversal();
      if (!iter->IsDoneWithTraversal())
      {
        // non-empty
        nonempty_inputs.push_back(cd);
      }
      iter->Delete();
    }
  }

  if (nonempty_inputs.size() == 0)
  {
    return 1;
  }

  auto nameKey = vtkCompositeDataSet::NAME();

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(output->NewIterator());
  iter->SkipEmptyNodesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    std::vector<vtkTable*> leaves;
    const char* bname =
      iter->HasCurrentMetaData() ? iter->GetCurrentMetaData()->Get(nameKey) : nullptr;
    for (auto cd : nonempty_inputs)
    {
      if (auto* table = vtkTable::SafeDownCast(cd->GetDataSet(iter)))
      {
        if (table->GetNumberOfRows() > 0 && table->GetNumberOfColumns() > 0)
        {
          leaves.push_back(table);
        }
        if (bname == nullptr && cd->HasMetaData(iter))
        {
          bname = cd->GetMetaData(iter)->Get(nameKey);
        }
      }
    }
    if (leaves.size())
    {
      auto result = ::vtkPVMergeTablesMultiBlockMerge(leaves);
      output->SetDataSet(iter, result);
      if (bname)
      {
        output->GetMetaData(iter)->Set(nameKey, bname);
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVMergeTablesMultiBlock::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
