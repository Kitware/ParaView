// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPTableFFT.h"

#include "vtkArrayDispatch.h"
#include "vtkDSPIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

namespace
{

struct Aggregator
{
public:
  virtual void operator()(vtkDataArray* array) = 0;
  virtual vtkSmartPointer<vtkDataArray> GetAggregate() = 0;
  virtual ~Aggregator() = default;
};

template <typename ArrayT>
struct TypedAggregator : Aggregator
{
public:
  using ValueTypeT = vtk::GetAPIType<ArrayT>;

  TypedAggregator(ArrayT* input)
  {
    if (input)
    {
      this->Name = input->GetName();
      this->NumberOfComponents =
        (input->GetNumberOfComponents() != 0 ? input->GetNumberOfComponents() : 1);
    }
  }

  void operator()(vtkDataArray* array) override
  {
    if (!array)
    {
      vtkErrorWithObjectMacro(nullptr, "Could not aggregate array");
      return;
    }
    auto typedArray = ArrayT::FastDownCast(array);
    if (!typedArray)
    {
      typedArray = ArrayT::SafeDownCast(array);
    }

    if (!typedArray)
    {
      vtkErrorWithObjectMacro(nullptr, "Could not aggregate array " << array->GetName());
      return;
    }

    auto range = vtk::DataArrayValueRange(typedArray);
    std::vector<ValueTypeT> buffer(range.size());
    vtkSMPTools::Transform(
      range.begin(), range.end(), buffer.begin(), [](ValueTypeT val) { return val; });
    this->Data->emplace_back(std::move(buffer));
  }

  vtkSmartPointer<vtkDataArray> GetAggregate() override
  {
    vtkNew<vtkMultiDimensionalArray<ValueTypeT>> output;
    output->SetName(this->Name.c_str());
    output->ConstructBackend(
      this->Data, this->Data->at(0).size() / this->NumberOfComponents, this->NumberOfComponents);
    return output;
  }

  ~TypedAggregator() override = default;

private:
  std::string Name;
  vtkIdType NumberOfComponents = 1;
  std::shared_ptr<std::vector<std::vector<ValueTypeT>>> Data =
    std::make_shared<std::vector<std::vector<ValueTypeT>>>();
};

struct DispatchInitializeAggregator
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, std::shared_ptr<Aggregator>& aggregator)
  {
    aggregator = std::make_shared<TypedAggregator<ArrayT>>(arr);
  }
};

}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkDSPTableFFT);

//------------------------------------------------------------------------------
int vtkDSPTableFFT::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Could not retrieve input vector");
    return 0;
  }

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!input)
  {
    return 1;
  }

  auto dspIterator = vtkDSPIterator::GetInstance(input);

  bool isFirstRun = true;
  std::vector<std::shared_ptr<::Aggregator>> aggregators;
  vtkSmartPointer<vtkInformationVector> filteredInput;
  for (dspIterator->GoToFirstItem(); !dspIterator->IsDoneWithTraversal();
       dspIterator->GoToNextItem())
  {
    filteredInput = vtkSmartPointer<vtkInformationVector>::New();
    filteredInput->Copy(inputVector[0], true); // deep copy
    filteredInput->GetInformationObject(0)->Set(
      vtkDataObject::DATA_OBJECT(), dspIterator->GetCurrentTable());
    vtkInformationVector* filteredVectorInput[1];
    filteredVectorInput[0] = filteredInput.Get();
    if (this->Superclass::RequestData(request, filteredVectorInput, outputVector) == 0)
    {
      vtkErrorMacro("Error executing superclass filter on data");
      return 0;
    }

    auto outInfo = outputVector->GetInformationObject(0);
    if (!outInfo)
    {
      vtkErrorMacro("Could not get output information");
      return 0;
    }

    auto result = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!result)
    {
      vtkErrorMacro("Could not get output as a table");
      return 0;
    }

    if (isFirstRun)
    {
      for (vtkIdType iArr = 0; iArr < result->GetRowData()->GetNumberOfArrays(); ++iArr)
      {
        auto arr = result->GetRowData()->GetArray(iArr);
        if (!arr)
        {
          continue;
        }

        using SupportedArrays = vtkArrayDispatch::Arrays;
        using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedArrays>;

        std::shared_ptr<::Aggregator> aggregator;
        ::DispatchInitializeAggregator init;
        if (!Dispatcher::Execute(arr, init, aggregator))
        {
          init(arr, aggregator);
        }
        aggregators.emplace_back(std::move(aggregator));
      }
      isFirstRun = false;
    }

    vtkIdType iArr = 0;
    for (auto aggregator : aggregators)
    {
      (*aggregator)(result->GetRowData()->GetArray(iArr));
      iArr++;
    }
  }

  vtkNew<vtkTable> output;
  for (auto aggregator : aggregators)
  {
    output->GetRowData()->AddArray(aggregator->GetAggregate());
  }

  auto outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not get output information");
    return 0;
  }
  outInfo->Set(vtkDataObject::DATA_OBJECT(), output);

  return 1;
}

VTK_ABI_NAMESPACE_END
