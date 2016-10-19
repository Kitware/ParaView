/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMergeTables.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMergeTables.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkPVMergeTables);
//----------------------------------------------------------------------------
vtkPVMergeTables::vtkPVMergeTables()
{
}

//----------------------------------------------------------------------------
vtkPVMergeTables::~vtkPVMergeTables()
{
}

//----------------------------------------------------------------------------
int vtkPVMergeTables::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVMergeTables::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
static void vtkPVMergeTablesMerge(vtkTable* output, vtkTable* inputs[], int num_inputs)
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
int vtkPVMergeTables::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int num_connections = this->GetNumberOfInputConnections(0);

  // Get output table
  vtkTable* outputTable = vtkTable::GetData(outputVector, 0);

  if (vtkTable::GetData(inputVector[0], 0))
  {
    vtkTable** inputs = new vtkTable*[num_connections];
    for (int idx = 0; idx < num_connections; ++idx)
    {
      inputs[idx] = vtkTable::GetData(inputVector[0], idx);
    }
    ::vtkPVMergeTablesMerge(outputTable, inputs, num_connections);
    delete[] inputs;
    return 1;
  }

  vtkCompositeDataSet* input0 = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataIterator* iter = input0->NewIterator();
  iter->SkipEmptyNodesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkTable** inputs = new vtkTable*[num_connections];
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
      }
    }
    ::vtkPVMergeTablesMerge(outputTable, inputs, num_connections);
    delete[] inputs;
  }
  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVMergeTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
