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

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkPVMergeTables);
//----------------------------------------------------------------------------
vtkPVMergeTables::vtkPVMergeTables() = default;

//----------------------------------------------------------------------------
vtkPVMergeTables::~vtkPVMergeTables() = default;

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
void vtkPVMergeTables::MergeTables(vtkTable* output, const std::vector<vtkTable*>& tables)
{
  vtkDataSetAttributes::FieldList fields;
  for (auto& curTable : tables)
  {
    if (curTable->GetNumberOfRows() > 0 && curTable->GetNumberOfColumns() > 0)
    {
      fields.IntersectFieldList(curTable->GetRowData());
    }
  }

  auto outRD = output->GetRowData();
  // passing sz=0 ensures that fields simply uses the accumulated counts for
  // number of rows.
  fields.CopyAllocate(outRD, vtkDataSetAttributes::PASSDATA, /*sz=*/0, /*ext=*/0);

  vtkIdType outStartRow = 0;
  int fieldsInputIdx = 0;
  for (auto& curTable : tables)
  {
    if (curTable->GetNumberOfRows() == 0 || curTable->GetNumberOfColumns() == 0)
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
  auto inputs = vtkPVMergeTables::GetTables(inputVector[0]);
  auto output = vtkTable::GetData(outputVector, 0);

  vtkPVMergeTables::MergeTables(output, inputs);
  return 1;
}

//----------------------------------------------------------------------------
std::vector<vtkTable*> vtkPVMergeTables::GetTables(vtkInformationVector* inputVector)
{
  std::vector<vtkTable*> tables;
  const int numInputs = inputVector->GetNumberOfInformationObjects();
  for (int inputIdx = 0; inputIdx < numInputs; ++inputIdx)
  {
    if (auto table = vtkTable::GetData(inputVector, inputIdx))
    {
      tables.push_back(table);
    }
    else if (auto cd = vtkCompositeDataSet::GetData(inputVector, inputIdx))
    {
      using Opts = vtk::CompositeDataSetOptions;
      for (auto node : vtk::Range(cd, Opts::SkipEmptyNodes))
      {
        if ((table = vtkTable::SafeDownCast(node.GetDataObject())))
        {
          tables.push_back(table);
        }
      }
    }
  }
  return tables;
}

//----------------------------------------------------------------------------
void vtkPVMergeTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
