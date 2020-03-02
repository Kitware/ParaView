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
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

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
  vtkDataSetAttributes::FieldList fields;
  for (int idx = 0; idx < num_inputs; ++idx)
  {
    vtkTable* curTable = inputs[idx];
    if (curTable && curTable->GetNumberOfRows() > 0 && curTable->GetNumberOfColumns() > 0)
    {
      fields.IntersectFieldList(curTable->GetRowData());
    }
  }

  auto outRD = output->GetRowData();
  // passing sz=0 ensures that fields simply uses the accumulated counts for
  // number of rows.
  fields.CopyAllocate(outRD, vtkDataSetAttributes::PASSDATA, /*sz=*/0, /*ext=*/0);

  vtkIdType outStartRow = 0;
  for (int idx = 0, fieldsInputIdx = 0; idx < num_inputs; ++idx)
  {
    vtkTable* curTable = inputs[idx];
    if (!curTable || curTable->GetNumberOfRows() == 0 || curTable->GetNumberOfColumns() == 0)
    {
      continue;
    }

    auto inRD = curTable->GetRowData();
    const auto inNumRows = inRD->GetNumberOfTuples();
    fields.CopyData(fieldsInputIdx, inRD, 0, inNumRows, outRD, outStartRow);
    outStartRow += inNumRows;
    ++fieldsInputIdx;
  }
}

//----------------------------------------------------------------------------
int vtkPVMergeTables::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int num_connections = this->GetNumberOfInputConnections(0);
  std::vector<vtkTable*> inputs;

  // Get output table
  vtkTable* outputTable = vtkTable::GetData(outputVector, 0);
  if (vtkTable::GetData(inputVector[0], 0))
  {
    for (int idx = 0; idx < num_connections; ++idx)
    {
      inputs.push_back(vtkTable::GetData(inputVector[0], idx));
    }
  }
  else
  {
    for (int idx = 0; idx < num_connections; ++idx)
    {
      vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], idx);
      if (!inputCD)
      {
        continue;
      }

      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(inputCD->NewIterator());
      iter->SkipEmptyNodesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkTable* input = vtkTable::SafeDownCast(inputCD->GetDataSet(iter));
        if (input)
        {
          inputs.push_back(input);
        }
      }
    }
  }
  ::vtkPVMergeTablesMerge(outputTable, inputs.data(), static_cast<int>(inputs.size()));
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVMergeTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
