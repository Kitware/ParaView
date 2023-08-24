// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMultiDimensionBrowser.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDSPArrayList.h"
#include "vtkDataSetAttributes.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkTable.h"

#include <limits>

namespace
{
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
    outputArray->SetIndex(index);
    output->AddColumn(outputArray);
  }
};

//------------------------------------------------------------------------------
/**
 * Get the maximum dimension for given vtkMultiDimensionalArray.
 */
struct MDArrayInfo
{
  int DimensionMax = 0;

  template <typename TArray>
  void operator()(TArray* inputArray)
  {
    auto backend = inputArray->GetBackend();
    this->DimensionMax = backend->GetNumberOfArrays() - 1;
  }
};

}; // end of anonymous namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiDimensionBrowser);

//------------------------------------------------------------------------------
int vtkMultiDimensionBrowser::ComputeIndexMax()
{
  using SupportedArrays = vtkArrayDispatch::MultiDimensionalArrays;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  ::MDArrayInfo infoWorker;

  auto table = vtkTable::SafeDownCast(this->GetInputDataObject(0, 0));
  auto rowData = table->GetRowData();

  int indexMax = std::numeric_limits<int>::max();
  for (int arrayIdx = 0; arrayIdx < rowData->GetNumberOfArrays(); arrayIdx++)
  {
    if (Dispatcher::Execute(rowData->GetArray(arrayIdx), infoWorker))
    {
      indexMax = std::min(indexMax, infoWorker.DimensionMax);
    }
  }

  return indexMax;
}

//------------------------------------------------------------------------------
void vtkMultiDimensionBrowser::UpdateIndexRange()
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
bool vtkMultiDimensionBrowser::CreateOutputArray(vtkDataArray* sourceArray, vtkTable* output)
{
  using SupportedArrays = vtkArrayDispatch::MultiDimensionalArrays;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  ::PrepareMDArrayCopy worker;
  if (Dispatcher::Execute(sourceArray, worker, output, this->Index))
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
  this->UpdateIndexRange();
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

  this->UpdateIndexRange();
  if (!this->IsIndexInRange())
  {
    vtkWarningMacro("Index " << this->Index << " is out of range [" << this->IndexRange[0] << ", "
                             << this->IndexRange[1] << "]. Nothing done.");
  }
  else
  {
    auto rowData = input->GetRowData();
    for (int arrayIdx = 0; arrayIdx < rowData->GetNumberOfArrays(); arrayIdx++)
    {
      this->CreateOutputArray(rowData->GetArray(arrayIdx), output);
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
