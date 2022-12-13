/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkMergeReduceTableBlocks.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMergeReduceTableBlocks.h"

#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>
#include <string>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMergeReduceTableBlocks);

// ----------------------------------------------------------------------------
vtkMergeReduceTableBlocks::vtkMergeReduceTableBlocks()
{
  // Add operation types
  for (auto type : { "Mean", "Sum", "Min", "Max" })
  {
    this->OperationSelection->AddArray(type, false);
  }

  // Add observers for selections update
  this->ColumnToReduceSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkMergeReduceTableBlocks::Modified);
  this->ColumnToCopySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkMergeReduceTableBlocks::Modified);
  this->OperationSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkMergeReduceTableBlocks::Modified);
}

//------------------------------------------------------------------------------
int vtkMergeReduceTableBlocks::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkMergeReduceTableBlocks::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMultiBlockDataSet* input = vtkMultiBlockDataSet::GetData(inputVector[0]);
  if (!input)
  {
    vtkErrorMacro("No valid multiblock input!");
    return 0;
  }
  if (input->GetNumberOfBlocks() == 0)
  {
    return 1;
  }

  vtkTable* firstTable = vtkTable::SafeDownCast(input->GetBlock(0));
  if (!firstTable)
  {
    vtkErrorMacro("Expected a vtkTable at block index 0, aborting");
    return 0;
  }

  vtkTable* output = vtkTable::GetData(outputVector);
  vtkIdType nbRows = firstTable->GetNumberOfRows();
  const bool wantMean = this->OperationSelection->ArrayIsEnabled("Mean");
  const bool wantMin = this->OperationSelection->ArrayIsEnabled("Min");
  const bool wantMax = this->OperationSelection->ArrayIsEnabled("Max");
  const bool wantSum = this->OperationSelection->ArrayIsEnabled("Sum");

  // Loop over columns, blocks, rows, and operations
  for (vtkIdType arrIdx = 0; arrIdx < firstTable->GetNumberOfColumns(); arrIdx++)
  {
    std::string colName = firstTable->GetColumnName(arrIdx);

    if (!this->ColumnToReduceSelection->ArrayIsEnabled(colName.c_str()) &&
      !this->ColumnToCopySelection->ArrayIsEnabled(colName.c_str()))
    {
      continue;
    }

    vtkDataArray* array = vtkDataArray::SafeDownCast(firstTable->GetColumnByName(colName.c_str()));
    if (!array)
    {
      vtkErrorMacro("Could not find array named " << colName << ".");
      return 0;
    }

    // Shallow copy array to output if needed
    if (this->ColumnToCopySelection->ArrayIsEnabled(colName.c_str()))
    {
      vtkSmartPointer<vtkDataArray> outArray;
      outArray.TakeReference(array->NewInstance());
      outArray->ShallowCopy(array);
      output->AddColumn(outArray);
    }

    if (this->ColumnToReduceSelection->ArrayIsEnabled(colName.c_str()))
    {
      // Prepare arrays based on input array type
      auto sum = vtk::TakeSmartPointer(array->NewInstance());
      auto min = vtk::TakeSmartPointer(array->NewInstance());
      auto max = vtk::TakeSmartPointer(array->NewInstance());
      vtkIdType nbComp = array->GetNumberOfComponents();

      if (wantSum || wantMean)
      {
        sum->SetNumberOfComponents(nbComp);
        sum->SetNumberOfTuples(nbRows);
        sum->SetName((colName + "_Sum").c_str());
        auto sumRange = vtk::DataArrayValueRange(sum);
        vtkSMPTools::Fill(sumRange.begin(), sumRange.end(), 0.0);
      }

      if (wantMin)
      {
        min->SetNumberOfComponents(nbComp);
        min->SetNumberOfTuples(nbRows);
        min->SetName((colName + "_Min").c_str());
        auto minRange = vtk::DataArrayValueRange(min);
        vtkSMPTools::Fill(minRange.begin(), minRange.end(), VTK_DOUBLE_MAX);
      }

      if (wantMax)
      {
        max->SetNumberOfComponents(nbComp);
        max->SetNumberOfTuples(nbRows);
        max->SetName((colName + "_Max").c_str());
        auto maxRange = vtk::DataArrayValueRange(max);
        vtkSMPTools::Fill(maxRange.begin(), maxRange.end(), VTK_DOUBLE_MIN);
      }

      auto iter = vtk::TakeSmartPointer(input->NewIterator());
      iter->SkipEmptyNodesOn();

      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
        if (!table)
        {
          vtkErrorMacro(
            "Expected a vtkTable at block index " << iter->GetCurrentFlatIndex() << ", aborting");
          return 0;
        }

        array = vtkDataArray::SafeDownCast(table->GetColumnByName(colName.c_str()));
        if (!array)
        {
          vtkErrorMacro("Could not find array named " << colName << ".");
          return 0;
        }

        auto arrayRange = vtk::DataArrayValueRange(array);
        auto sumRange = vtk::DataArrayValueRange(sum);
        auto minRange = vtk::DataArrayValueRange(min);
        auto maxRange = vtk::DataArrayValueRange(max);

        // Type of output arrays elements should be the same as array due to the use of NewInstance
        using ArrayType = decltype(arrayRange)::value_type;

        if (wantSum || wantMean)
        {
          vtkSMPTools::Transform(arrayRange.cbegin(), arrayRange.cend(), sumRange.cbegin(),
            sumRange.begin(), [](ArrayType x, ArrayType y) { return x + y; });
        }

        if (wantMin)
        {
          vtkSMPTools::Transform(arrayRange.cbegin(), arrayRange.cend(), minRange.cbegin(),
            minRange.begin(), [](ArrayType x, ArrayType y) { return std::min(x, y); });
        }

        if (wantMax)
        {
          vtkSMPTools::Transform(arrayRange.cbegin(), arrayRange.cend(), maxRange.cbegin(),
            maxRange.begin(), [](ArrayType x, ArrayType y) { return std::max(x, y); });
        }
      }

      // Assign columns for the current array being reduced
      if (wantSum)
      {
        output->AddColumn(sum);
      }

      if (wantMin)
      {
        output->AddColumn(min);
      }

      if (wantMax)
      {
        output->AddColumn(max);
      }

      if (wantMean)
      {
        vtkNew<vtkDoubleArray> mean;
        mean->SetNumberOfComponents(nbComp);
        mean->SetNumberOfTuples(nbRows);
        mean->SetName((colName + "_Mean").c_str());
        auto meanRange = vtk::DataArrayValueRange(mean);
        auto sumRange = vtk::DataArrayValueRange(sum);

        using ArrayType = decltype(sumRange)::value_type;
        vtkSMPTools::Transform(sumRange.cbegin(), sumRange.cend(), meanRange.begin(),
          [input](ArrayType x) { return x / static_cast<double>(input->GetNumberOfBlocks()); });

        output->AddColumn(mean);
      }
    }
  }

  return 1;
}

//--------------------------------------- --------------------------------------
void vtkMergeReduceTableBlocks::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColumnToReduceSelection:\n";
  this->ColumnToReduceSelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ColumnToCopySelection:\n";
  this->ColumnToCopySelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "OperationSelection:\n";
  this->OperationSelection->PrintSelf(os, indent.GetNextIndent());
}
