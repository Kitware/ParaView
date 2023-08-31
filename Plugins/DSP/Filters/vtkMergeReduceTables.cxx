// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMergeReduceTables.h"

#include "vtkCommand.h"
#include "vtkDSPIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMergeReduceTables);

// ----------------------------------------------------------------------------
vtkMergeReduceTables::vtkMergeReduceTables()
{
  // Add operation types
  for (auto type : { "Mean", "Sum", "Min", "Max" })
  {
    this->OperationSelection->AddArray(type, false);
  }

  // Add observers for selections update
  this->ColumnToReduceSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkMergeReduceTables::Modified);
  this->ColumnToCopySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkMergeReduceTables::Modified);
  this->OperationSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkMergeReduceTables::Modified);
}

//------------------------------------------------------------------------------
int vtkMergeReduceTables::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
int vtkMergeReduceTables::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);

  if (!input)
  {
    vtkErrorMacro("No valid input!");
    return 0;
  }

  // Create DSP iterator according to input type
  auto iter = vtkDSPIterator::GetInstance(input);

  if (!iter)
  {
    vtkErrorMacro("Unable to generate iterator for the given input.");
    return 0;
  }

  iter->GoToFirstItem();

  // Check if input is empty
  if (iter->IsDoneWithTraversal())
  {
    return 1;
  }

  vtkIdType nbIterations = iter->GetNumberOfIterations();
  vtkTable* firstTable = iter->GetCurrentTable();

  if (!firstTable)
  {
    vtkErrorMacro("Could not retrieve the first table.");
    return 0;
  }

  // Retrieve optional ghost array
  vtkDataArray* ghostArray = vtkArrayDownCast<vtkDataArray>(
    firstTable->GetColumnByName(vtkDataSetAttributes::GhostArrayName()));
  bool hasGhost = (ghostArray != nullptr);

  vtkTable* output = vtkTable::GetData(outputVector);
  vtkIdType nbRows = firstTable->GetNumberOfRows();
  const bool wantMean = this->OperationSelection->ArrayIsEnabled("Mean");
  const bool wantMin = this->OperationSelection->ArrayIsEnabled("Min");
  const bool wantMax = this->OperationSelection->ArrayIsEnabled("Max");
  const bool wantSum = this->OperationSelection->ArrayIsEnabled("Sum");

  // Loop over columns, tables, rows, and operations
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

      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkTable* table = iter->GetCurrentTable();
        if (!table)
        {
          vtkErrorMacro("Could not retrieve table, aborting.");
          return 0;
        }

        // Skip ghost points to avoid duplicates
        if (hasGhost)
        {
          ghostArray = vtkArrayDownCast<vtkDataArray>(
            table->GetColumnByName(vtkDataSetAttributes::GhostArrayName()));

          if (ghostArray && ghostArray->GetNumberOfTuples() > 0 &&
            ghostArray->GetComponent(0, 0) != 0)
          {
            nbIterations--;
            continue;
          }
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

        if (wantSum || wantMean)
        {
          this->ComputeSum(arrayRange, sumRange);
        }

        if (wantMin)
        {
          this->ComputeMin(arrayRange, minRange);
        }

        if (wantMax)
        {
          this->ComputeMax(arrayRange, maxRange);
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
          [&](ArrayType x) { return x / static_cast<double>(nbIterations); });

        output->AddColumn(mean);
      }
    }
  }

  vtkSmartPointer<vtkTable> localTable = output;

  // Retrieve controller for distributed context
  auto controller = vtkMultiProcessController::GetGlobalController();
  const vtkIdType nbProcesses = controller->GetNumberOfProcesses();

  // Gather all tables and number of iterations onto process 0
  std::vector<vtkSmartPointer<vtkDataObject>> allRanksTables(nbProcesses);
  std::vector<vtkIdType> allRanksNbIterations(nbProcesses, 0);
  controller->Gather(localTable, allRanksTables, 0);
  controller->Gather(&nbIterations, allRanksNbIterations.data(), 1, 0);

  // Reduce results on process 0
  if (controller->GetLocalProcessId() == 0)
  {
    // Compute total number of iterations for the mean
    vtkIdType totalNbIterations = std::accumulate(
      allRanksNbIterations.begin(), allRanksNbIterations.end(), static_cast<vtkIdType>(0));

    // Start from first table
    vtkTable* reducedTable = vtkTable::SafeDownCast(allRanksTables[0]);

    // Adjust partial mean values in the first table for reduction
    for (vtkIdType col = 0; col < reducedTable->GetNumberOfColumns(); col++)
    {
      std::string colName = reducedTable->GetColumnName(col);
      std::size_t pos = colName.find_last_of('_');

      if (pos == std::string::npos)
      {
        continue;
      }

      const auto suffix = colName.substr(pos + 1);

      if (suffix == "Mean")
      {
        vtkDataArray* meanArray =
          vtkArrayDownCast<vtkDataArray>(reducedTable->GetColumnByName(colName.c_str()));
        auto meanRange = vtk::DataArrayValueRange(meanArray);
        using ArrayType = decltype(meanRange)::value_type;

        vtkSMPTools::Transform(
          meanRange.cbegin(), meanRange.cend(), meanRange.begin(), [&](ArrayType x) {
            return ((allRanksNbIterations[0] / static_cast<double>(totalNbIterations)) * x);
          });
      }
    }

    for (vtkIdType idx = 1; idx < nbProcesses; idx++)
    {
      vtkTable* procTable = vtkTable::SafeDownCast(allRanksTables[idx]);

      for (vtkIdType col = 0; col < procTable->GetNumberOfColumns(); col++)
      {
        std::string colName = procTable->GetColumnName(col);
        std::size_t pos = colName.find_last_of('_');

        if (pos == std::string::npos)
        {
          continue;
        }

        const auto suffix = colName.substr(pos + 1);

        vtkDataArray* array =
          vtkArrayDownCast<vtkDataArray>(procTable->GetColumnByName(colName.c_str()));
        vtkDataArray* baseArray =
          vtkArrayDownCast<vtkDataArray>(reducedTable->GetColumnByName(colName.c_str()));

        auto arrayRange = vtk::DataArrayValueRange(array);
        auto baseRange = vtk::DataArrayValueRange(baseArray);

        if (suffix == "Sum")
        {
          this->ComputeSum(arrayRange, baseRange);
        }
        else if (suffix == "Min")
        {
          this->ComputeMin(arrayRange, baseRange);
        }
        else if (suffix == "Max")
        {
          this->ComputeMax(arrayRange, baseRange);
        }
        else if (suffix == "Mean")
        {
          using ArrayType = decltype(arrayRange)::value_type;
          vtkSMPTools::Transform(arrayRange.cbegin(), arrayRange.cend(), baseRange.cbegin(),
            baseRange.begin(), [&](ArrayType x, ArrayType y) {
              return (allRanksNbIterations[idx] / static_cast<double>(totalNbIterations)) * x + y;
            });
        }
      }
    }

    output->ShallowCopy(reducedTable);
  }
  else
  {
    output->Initialize();
  }

  return 1;
}

//--------------------------------------- --------------------------------------
void vtkMergeReduceTables::ComputeSum(RangeType srcRange, RangeType dstRange) const
{
  using ArrayType = decltype(srcRange)::value_type;
  vtkSMPTools::Transform(srcRange.cbegin(), srcRange.cend(), dstRange.cbegin(), dstRange.begin(),
    [](ArrayType x, ArrayType y) { return x + y; });
}

//--------------------------------------- --------------------------------------
void vtkMergeReduceTables::ComputeMin(RangeType srcRange, RangeType dstRange) const
{
  using ArrayType = decltype(srcRange)::value_type;
  vtkSMPTools::Transform(srcRange.cbegin(), srcRange.cend(), dstRange.cbegin(), dstRange.begin(),
    [](ArrayType x, ArrayType y) { return std::min(x, y); });
}

//--------------------------------------- --------------------------------------
void vtkMergeReduceTables::ComputeMax(RangeType srcRange, RangeType dstRange) const
{
  using ArrayType = decltype(srcRange)::value_type;
  vtkSMPTools::Transform(srcRange.cbegin(), srcRange.cend(), dstRange.cbegin(), dstRange.begin(),
    [](ArrayType x, ArrayType y) { return std::max(x, y); });
}

//--------------------------------------- --------------------------------------
void vtkMergeReduceTables::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColumnToReduceSelection:\n";
  this->ColumnToReduceSelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ColumnToCopySelection:\n";
  this->ColumnToCopySelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "OperationSelection:\n";
  this->OperationSelection->PrintSelf(os, indent.GetNextIndent());
}
