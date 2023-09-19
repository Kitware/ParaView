// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMultiDimensionBrowser.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDSPArrayList.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"

#include <limits>

namespace
{
using AllMDArrays = vtkArrayDispatch::MultiDimensionalArrays;

using IdsArrays = vtkTypeList::Unique<vtkTypeList::Create<vtkMultiDimensionalArray<vtkIdType>,
  vtkMultiDimensionalArray<int>, vtkMultiDimensionalArray<long>,
  vtkMultiDimensionalArray<long long>, vtkMultiDimensionalArray<unsigned int>,
  vtkMultiDimensionalArray<unsigned long>, vtkMultiDimensionalArray<unsigned long long>>>::Result;

//------------------------------------------------------------------------------
/**
 * Construct a vtkMultiDimensionalArray
 *
 * Create a new multidimensional array. Shallow copy input and set Index to it.
 * Add this new array to the output vtkTable, replacing any existing
 * array with the same name.
 *
 * This works with vtkMultiDimensionalArray input only.
 */
struct PrepareMDArrayCopy
{
  template <typename TArray>
  void operator()(TArray* inputArray, vtkTable* output, int index)
  {
    vtkNew<TArray> outputArray;
    outputArray->ImplicitShallowCopy(inputArray);
    if (index >= 0 && index < inputArray->GetBackend()->GetNumberOfArrays())
    {
      outputArray->SetIndex(index);
    }
    output->AddColumn(outputArray);
  }
};

//------------------------------------------------------------------------------
/**
 * Get the maximum dimension for given vtkMultiDimensionalArray.
 */
struct MDArrayInfo
{
  vtkIdType LocalSize = 0;

  template <typename TArray>
  void operator()(TArray* inputArray)
  {
    auto backend = inputArray->GetBackend();
    this->LocalSize = backend->GetNumberOfArrays();
  }
};

/**
 * Find a value in the given array.
 */
struct MDFindMax
{
  vtkIdType Max = 0;

  template <typename TArray>
  void operator()(TArray* array)
  {
    // use a dummy array to avoid modification on the actual array.
    vtkNew<TArray> dummyArray;
    dummyArray->ImplicitShallowCopy(array);
    auto range = vtk::DataArrayValueRange(dummyArray);
    for (vtkIdType index = 0; index < dummyArray->GetBackend()->GetNumberOfArrays(); index++)
    {
      dummyArray->SetIndex(index);
      this->Max = std::max(this->Max, static_cast<vtkIdType>(range[0]));
    }
  }
};

/**
 * Find a value in the given array.
 */
struct MDFindValue
{
  vtkIdType IndexOfValue = 0;
  bool ValueFound = false;

  template <typename TArray>
  void operator()(TArray* array, vtkIdType target)
  {
    // use a dummy array to avoid modification on the actual array.
    vtkNew<TArray> dummyArray;
    dummyArray->ImplicitShallowCopy(array);
    auto range = vtk::DataArrayValueRange(dummyArray);
    for (vtkIdType index = 0; index < dummyArray->GetBackend()->GetNumberOfArrays(); index++)
    {
      dummyArray->SetIndex(index);
      if (target == static_cast<vtkIdType>(range[0]))
      {
        this->IndexOfValue = index;
        this->ValueFound = true;
        break;
      }
    }
  }
};

}; // end of anonymous namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiDimensionBrowser);

//------------------------------------------------------------------------------
vtkIdType vtkMultiDimensionBrowser::ComputeLocalGlobalIdMax()
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<IdsArrays>;
  ::MDFindMax maxWorker;

  auto inputData = this->GetInputDataObject(0, 0);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputData);
  vtkIdType localMax = 0;
  if (Dispatcher::Execute(inputArray, maxWorker))
  {
    localMax = maxWorker.Max;
  }

  return localMax;
}

//------------------------------------------------------------------------------
vtkIdType vtkMultiDimensionBrowser::ComputeLocalSize()
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<AllMDArrays>;
  ::MDArrayInfo infoWorker;

  auto table = vtkTable::SafeDownCast(this->GetInputDataObject(0, 0));
  auto rowData = table->GetRowData();

  vtkIdType nbOfIndexes = std::numeric_limits<vtkIdType>::max();
  for (int arrayIdx = 0; arrayIdx < rowData->GetNumberOfArrays(); arrayIdx++)
  {
    if (Dispatcher::Execute(rowData->GetArray(arrayIdx), infoWorker))
    {
      nbOfIndexes = std::min(nbOfIndexes, infoWorker.LocalSize);
    }
  }

  return nbOfIndexes;
}

//------------------------------------------------------------------------------
vtkIdType vtkMultiDimensionBrowser::ComputeIndexMax()
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (this->UseGlobalIds)
  {
    const auto localMax = this->ComputeLocalGlobalIdMax();
    vtkIdType globalMax = localMax;
    if (controller && controller->GetNumberOfProcesses() > 1)
    {
      controller->AllReduce(&localMax, &globalMax, 1, vtkCommunicator::MAX_OP);
    }
    return globalMax;
  }

  const auto localSize = this->ComputeLocalSize();
  vtkIdType globalSize = localSize;
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    controller->AllReduce(&localSize, &globalSize, 1, vtkCommunicator::SUM_OP);
  }

  return globalSize - 1;
}

//------------------------------------------------------------------------------
void vtkMultiDimensionBrowser::UpdateGlobalIndexRange()
{
  this->IndexRange[0] = 0;
  this->IndexRange[1] = this->ComputeIndexMax();
}

//------------------------------------------------------------------------------
bool vtkMultiDimensionBrowser::IsIndexInRange()
{
  return this->Index >= this->IndexRange[0] && this->Index <= this->IndexRange[1];
}

//------------------------------------------------------------------------------
bool vtkMultiDimensionBrowser::UpdateLocalIndex()
{
  if (this->UseGlobalIds)
  {
    return this->MapToLocalGlobalId();
  }

  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    return this->MapToLocalIndex();
  }

  this->LocalIndex = this->Index;
  return true;
}

//------------------------------------------------------------------------------
bool vtkMultiDimensionBrowser::MapToLocalGlobalId()
{
  auto inputData = this->GetInputDataObject(0, 0);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputData);
  using Dispatcher = vtkArrayDispatch::DispatchByArray<IdsArrays>;
  ::MDFindValue findWorker;

  if (Dispatcher::Execute(inputArray, findWorker, this->Index))
  {
    if (findWorker.ValueFound)
    {
      this->LocalIndex = findWorker.IndexOfValue;
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------
bool vtkMultiDimensionBrowser::MapToLocalIndex()
{
  auto controller = vtkMultiProcessController::GetGlobalController();

  const auto localSize = this->ComputeLocalSize();
  if (!controller)
  {
    this->LocalIndex = this->Index;
    return this->LocalIndex < localSize && this->LocalIndex >= 0;
  }
  std::vector<vtkIdType> processesSize;
  processesSize.reserve(controller->GetNumberOfProcesses());
  controller->AllGather(&localSize, processesSize.data(), 1);
  vtkIdType offset = 0;
  for (vtkIdType rank = 0; rank < controller->GetLocalProcessId(); rank++)
  {
    offset += processesSize[rank];
  }

  this->LocalIndex = this->Index - offset;

  return this->LocalIndex < localSize && this->LocalIndex >= 0;
}

//------------------------------------------------------------------------------
bool vtkMultiDimensionBrowser::CreateOutputArray(vtkDataArray* sourceArray, vtkTable* output)
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<AllMDArrays>;
  ::PrepareMDArrayCopy worker;
  if (Dispatcher::Execute(sourceArray, worker, output, this->LocalIndex))
  {
    vtkDebugMacro("Index set on new multidimensional array " << sourceArray->GetName());
    return true;
  }

  vtkDebugMacro("Simple shallow copy for non multidimensional array " << sourceArray->GetName());
  return false;
}

//------------------------------------------------------------------------------
int vtkMultiDimensionBrowser::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  this->UpdateGlobalIndexRange();
  return 1;
}

//------------------------------------------------------------------------------
int vtkMultiDimensionBrowser::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkTable* input = vtkTable::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  if (!input || !output)
  {
    vtkErrorMacro("Missing input or output!");
    return 0;
  }

  if (input->GetNumberOfColumns() == 0)
  {
    return 1;
  }

  output->ShallowCopy(input);

  this->UpdateGlobalIndexRange();
  if (!this->IsIndexInRange())
  {
    vtkWarningMacro("Index " << this->Index << " is out of range [" << this->IndexRange[0] << ", "
                             << this->IndexRange[1] << "]. Nothing done.");
  }
  else
  {
    auto rowData = input->GetRowData();
    auto hasLocalIndex = this->UpdateLocalIndex();
    for (int arrayIdx = 0; arrayIdx < rowData->GetNumberOfArrays(); arrayIdx++)
    {
      this->CreateOutputArray(rowData->GetArray(arrayIdx), output);
    }
    if (!hasLocalIndex)
    {
      output->SetNumberOfRows(0);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkMultiDimensionBrowser::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Index: " << this->Index << "\n";
  os << indent << "IndexRange: [" << this->IndexRange[0] << ", " << this->IndexRange[1] << "]"
     << "\n";

  this->Superclass::PrintSelf(os, indent);
}
