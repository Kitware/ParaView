// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTemporalMultiplexing.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchArrayList.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkMultiDimensionalImplicitBackend.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <map>
#include <vector>

struct vtkTemporalMultiplexing::vtkInternals
{
  std::map<std::string, std::vector<vtkSmartPointer<vtkDataArray>>> Arrays;
};

//------------------------------------------------------------------------------
namespace
{
//------------------------------------------------------------------------------
struct CreateArrayVector
{
  template <typename TArray, typename ValueType = vtk::GetAPIType<TArray>>
  void operator()(TArray* vtkNotUsed(inArray), vtkSmartPointer<vtkDataArray>& outArray)
  {
    outArray.TakeReference(vtkAOSDataArrayTemplate<ValueType>::New());
  }
};

//------------------------------------------------------------------------------
struct ConstructMDArray
{
  template <typename TArray, typename ValueType = vtk::GetAPIType<TArray>>
  void operator()(TArray* vtkNotUsed(dummyArray), const std::string& name,
    const std::vector<vtkSmartPointer<vtkDataArray>>& arrayVector, vtkTable* output)
  {
    // Downcast each vtkDataArray into vtkAOSDataArrayTemplate for the backend
    std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>> aosVector;
    aosVector.reserve(arrayVector.size());

    vtkSMPTools::For(0, arrayVector.size(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType idx = begin; idx < end; idx++)
      {
        vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>> aosArray =
          vtkAOSDataArrayTemplate<ValueType>::FastDownCast(arrayVector[idx]);
        aosVector.emplace_back(aosArray);
      }
    });

    vtkNew<vtkMultiDimensionalArray<ValueType>> mdArray;
    mdArray->SetName(name.c_str());
    mdArray->ConstructBackend(aosVector);
    output->AddColumn(mdArray);
  }
};

//------------------------------------------------------------------------------
struct ConstructVTKIdTypeMDArray
{
  // Template specialization of ConstructMDArray is not possible due to vtkIdType
  // being a typedef
  template <typename TArray>
  void operator()(TArray* vtkNotUsed(dummyArray), const std::string& name,
    const std::vector<vtkSmartPointer<vtkDataArray>>& arrayVector, vtkTable* output)
  {
    // Downcast each vtkDataArray into vtkAOSDataArrayTemplate for the backend
    std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<vtkIdType>>> aosVector;
    aosVector.reserve(arrayVector.size());

    vtkSMPTools::For(0, arrayVector.size(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType idx = begin; idx < end; idx++)
      {
        vtkSmartPointer<vtkAOSDataArrayTemplate<vtkIdType>> aosArray =
          vtkAOSDataArrayTemplate<vtkIdType>::FastDownCast(arrayVector[idx]);
        aosVector.emplace_back(aosArray);
      }
    });

    vtkNew<vtkMultiDimensionalArray<vtkIdType>> mdArray;
    mdArray->SetName(name.c_str());
    mdArray->ConstructBackend(aosVector);
    output->AddColumn(mdArray);
  }
};
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalMultiplexing);

//------------------------------------------------------------------------------
vtkTemporalMultiplexing::vtkTemporalMultiplexing()
  : Internals(new vtkTemporalMultiplexing::vtkInternals())
{
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::EnableAttributeArray(const std::string& arrName)
{
  if (!arrName.empty())
  {
    if (this->SelectedArrays.insert(arrName).second)
    {
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::ClearAttributeArrays()
{
  if (!this->SelectedArrays.empty())
  {
    this->SelectedArrays.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkTemporalMultiplexing::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalMultiplexing::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve total number of timesteps
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 1;
  }

  // Output is not temporal
  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalMultiplexing::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (inTimes)
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalMultiplexing::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector, 0);

  if (!output)
  {
    vtkErrorMacro("Missing valid output.");
    return 0;
  }

  if (this->SelectedArrays.empty())
  {
    // Reset table to empty state
    output->Initialize();
    return 1;
  }

  // Check that dataset is actually temporal
  if (this->NumberOfTimeSteps <= 0)
  {
    vtkWarningMacro("There should be at least one timestep (non temporal).");
    return 0;
  }

  // Retrieve data set attributes (point or cell data)
  vtkDataSetAttributes* dataAttributes = nullptr;
  vtkIdType nbArrays = 0;

  if (this->FieldAssociation == vtkDataObject::POINT)
  {
    dataAttributes = input->GetPointData();
    nbArrays = input->GetNumberOfPoints();
  }
  else if (this->FieldAssociation == vtkDataObject::CELL)
  {
    dataAttributes = input->GetCellData();
    nbArrays = input->GetNumberOfCells();
  }
  else
  {
    vtkWarningMacro("Invalid field association. Only point and cell associations are supported. "
                    "Defaulting to point association.");
    dataAttributes = input->GetPointData();
    nbArrays = input->GetNumberOfPoints();
  }

  // For the first request, let the pipeline know it should loop and setup arrays
  if (this->CurrentTimeIndex == 0)
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->Internals->Arrays.clear();
    this->PrepareVectorsOfArrays(dataAttributes, nbArrays);
  }

  // Retrieve each data array then add it to the vector
  // of arrays for the current timestep
  this->FillArraysForCurrentTimestep(dataAttributes);

  // Stop looping when the last timestep has been processed and prepare output
  this->CurrentTimeIndex++;

  if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
  {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
    this->CreateMultiDimensionalArrays(output);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::PrepareVectorsOfArrays(
  vtkDataSetAttributes* attributes, vtkIdType nbArrays)
{
  using SupportedArrays = vtkArrayDispatch::Arrays;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  CreateArrayVector worker;

  for (const auto& name : this->SelectedArrays)
  {
    vtkDataArray* array = vtkDataArray::SafeDownCast(attributes->GetAbstractArray(name.c_str()));

    if (!array)
    {
      continue;
    }

    // AOS arrays must be created for multidimensional arrays
    std::vector<vtkSmartPointer<vtkDataArray>> arrays;
    arrays.reserve(nbArrays);

    // Create AOS array with correct type
    vtkSmartPointer<vtkDataArray> refArray;

    if (!Dispatcher::Execute(array, worker, refArray))
    {
      worker(array, refArray);
    }

    for (vtkIdType idx = 0; idx < nbArrays; idx++)
    {
      vtkSmartPointer<vtkDataArray> newArray;
      newArray.TakeReference(refArray->NewInstance());
      newArray->SetNumberOfComponents(array->GetNumberOfComponents());
      newArray->SetNumberOfTuples(this->NumberOfTimeSteps);
      newArray->SetName(name.c_str());
      arrays.emplace_back(newArray);
    }

    this->Internals->Arrays[name] = arrays;
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::FillArraysForCurrentTimestep(vtkDataSetAttributes* attributes)
{
  vtkIdType nbArrays = this->Internals->Arrays.begin()->second.size();

  for (auto arrayVec : this->Internals->Arrays)
  {
    vtkDataArray* array = attributes->GetArray(arrayVec.first.c_str());

    if (!array)
    {
      continue;
    }

    vtkIdType nbComp = array->GetNumberOfComponents();

    vtkSMPTools::For(0, nbArrays, [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType idx = begin; idx < end; idx++)
      {
        for (vtkIdType comp = 0; comp < nbComp; comp++)
        {
          arrayVec.second[idx]->SetComponent(
            this->CurrentTimeIndex, comp, array->GetComponent(idx, comp));
        }
      }
    });
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::CreateMultiDimensionalArrays(vtkTable* output)
{
  // Create multi dimensional arrays from each vector of arrays
  using SupportedArrays = vtkArrayDispatch::Arrays;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  ConstructMDArray worker;
  ConstructVTKIdTypeMDArray idTypeWorker;

  for (const auto& arrayInfo : this->Internals->Arrays)
  {
    if (vtkAOSDataArrayTemplate<vtkIdType>::FastDownCast(arrayInfo.second[0]))
    {
      if (!Dispatcher::Execute(
            arrayInfo.second[0], idTypeWorker, arrayInfo.first, arrayInfo.second, output))
      {
        idTypeWorker(arrayInfo.second[0].Get(), arrayInfo.first, arrayInfo.second, output);
      }
    }
    else
    {
      if (!Dispatcher::Execute(
            arrayInfo.second[0], worker, arrayInfo.first, arrayInfo.second, output))
      {
        worker(arrayInfo.second[0].Get(), arrayInfo.first, arrayInfo.second, output);
      }
    }
  }
}

//--------------------------------------- --------------------------------------
void vtkTemporalMultiplexing::PrintSelf(std::ostream& os, vtkIndent indent)
{
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "CurrentTimeIndex: " << this->CurrentTimeIndex << endl;
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  this->Superclass::PrintSelf(os, indent);
}
