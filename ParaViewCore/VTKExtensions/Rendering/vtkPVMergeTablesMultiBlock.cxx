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

#include "assert.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariant.h"

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
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVMergeTablesMultiBlock::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVMergeTablesMultiBlock::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
static void vtkPVMergeTablesMultiBlockMerge(vtkTable* output, vtkTable* inputs[], int num_inputs)
{
  for (int idx = 0; idx < num_inputs; ++idx)
  {
    vtkTable* curTable = inputs[idx];
    if (!curTable || curTable->GetNumberOfRows() == 0 || curTable->GetNumberOfColumns() == 0)
    {
      continue;
    }

    if (output->GetNumberOfRows() == 0)
    {
      // Copy output structure from the first non-empty input.
      output->DeepCopy(curTable);
      continue;
    }

    vtkIdType numRows = curTable->GetNumberOfRows();
    vtkIdType numCols = curTable->GetNumberOfColumns();
    for (vtkIdType i = 0; i < numRows; i++)
    {
      vtkIdType curRow = output->InsertNextBlankRow();
      for (vtkIdType j = 0; j < numCols; j++)
      {
        output->SetValue(curRow, j, curTable->GetValue(i, j));
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVMergeTablesMultiBlock::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
  }

  vtkCompositeDataSet* input0 = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkMultiBlockDataSet* outputTables = vtkMultiBlockDataSet::GetData(outputVector, 0);
  outputTables->CopyStructure(input0);
  assert(outputTables);

  int num_connections = this->GetNumberOfInputConnections(0);
  vtkCompositeDataIterator* iter = input0->NewIterator();
  iter->SkipEmptyNodesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkTable** inputs = new vtkTable*[num_connections];
    bool has_input = false;
    for (int idx = 0; idx < num_connections; ++idx)
    {
      vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], idx);
      if (!inputCD)
      {
        continue;
      }
      vtkSmartPointer<vtkCompositeDataIterator> iter2;
      iter2.TakeReference(inputCD->NewIterator());
      if (iter2->IsDoneWithTraversal())
      {
        // trivial case, the composite dataset being merged is empty, simply
        // ignore it.
        inputs[idx] = NULL;
      }
      else
      {
        inputs[idx] = vtkTable::SafeDownCast(inputCD->GetDataSet(iter));
        has_input |= (inputs[idx] != NULL);
      }
    }
    // don't add an empty vtkTable is all inputs are NULL.
    if (has_input)
    {
      vtkTable* outputTable = vtkTable::New();
      ::vtkPVMergeTablesMultiBlockMerge(outputTable, inputs, num_connections);
      outputTables->SetDataSet(iter, outputTable);
      outputTable->Delete();
    }
    delete[] inputs;
  }
  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVMergeTablesMultiBlock::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
