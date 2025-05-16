// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTemporalMultiplexing.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchArrayList.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
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
#include <memory>
#include <vector>

//------------------------------------------------------------------------------
namespace
{
const char* TIME_ARRAY_NAME = "Time";

template <typename ValueType>
using WorkerDataContainerT = typename vtkMultiDimensionalImplicitBackend<ValueType>::DataContainerT;

class Worker
{
public:
  virtual void operator()(vtkDataArray* input, vtkIdType currentTimeIndex, vtkIdType offset) = 0;
  virtual void InitData(vtkIdType nbOfArrays, vtkIdType nbOfTuples, int nbOfComponents,
    const std::string& arrayName) = 0;
  virtual vtkSmartPointer<vtkDataArray> ConstructMDArray() = 0;

  std::string ArrayName;
};

template <typename ValueType>
class TypedWorker : public Worker
{
public:
  /**
   * Set the data for current time index in each array (ie. for each point/cell).
   * Note that the array offset is only useful in the context of composite data
   * sets where the hidden dimension (nb of points/cells) is fragmented in the
   * input and we have to do a multi-pass here.
   */
  void operator()(vtkDataArray* input, vtkIdType currentTimeIndex, vtkIdType arrayOffset) override
  {
    vtkIdType nbOfArrays = input->GetNumberOfTuples();

    vtkSMPTools::For(0, nbOfArrays,
      [&](vtkIdType begin, vtkIdType end)
      {
        const vtkIdType valueIdx = currentTimeIndex * this->NbOfComponents;
        for (vtkIdType arrayIdx = begin; arrayIdx < end; ++arrayIdx)
        {
          for (int comp = 0; comp < this->NbOfComponents; ++comp)
          {
            (*this->Data)[arrayIdx + arrayOffset][valueIdx + comp] =
              input->GetComponent(arrayIdx, comp);
          }
        }
      });
  }

  void InitData(vtkIdType nbOfArrays, vtkIdType nbOfTuples, int nbOfComponents,
    const std::string& arrayName) override
  {
    this->Data =
      std::make_shared<WorkerDataContainerT<ValueType>>(WorkerDataContainerT<ValueType>());
    this->Data->resize(nbOfArrays);
    this->NbOfTuples = nbOfTuples;
    this->NbOfComponents = nbOfComponents;
    this->ArrayName = arrayName;

    const vtkIdType nbOfValues = nbOfTuples * nbOfComponents;

    vtkSMPTools::For(0, nbOfArrays,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType arrayIdx = begin; arrayIdx < end; ++arrayIdx)
        {
          (*this->Data)[arrayIdx].resize(nbOfValues);
        }
      });
  }

  vtkSmartPointer<vtkDataArray> ConstructMDArray() override
  {
    vtkNew<vtkMultiDimensionalArray<ValueType>> mdArray;
    mdArray->ConstructBackend(this->Data, this->NbOfTuples, this->NbOfComponents);
    mdArray->SetName(this->ArrayName.c_str());
    return mdArray;
  }

private:
  std::shared_ptr<WorkerDataContainerT<ValueType>> Data;
  vtkIdType NbOfTuples = 0;
  int NbOfComponents = 0;
};

/**
 * Worker dedicated to construct the correct type of workers. Instead
 * of dispatching every time step, this pattern enables us to dispatch
 * only once in the beginning of the procedure and then use some
 * polymorphism to "store" the types in the typed workers.
 */
struct WorkerCreator
{
  template <typename ArrayT>
  void operator()(ArrayT* vtkNotUsed(array), std::shared_ptr<Worker>& worker)
  {
    worker = std::make_shared<TypedWorker<vtk::GetAPIType<ArrayT>>>();
  }
};
}

//------------------------------------------------------------------------------
struct vtkTemporalMultiplexing::vtkInternals
{
  std::vector<std::shared_ptr<::Worker>> Workers;
  int NumberOfTimeSteps = 0;
  int CurrentTimeIndex = 0;
};

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
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
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
    this->Internals->NumberOfTimeSteps =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->Internals->NumberOfTimeSteps = 1;
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
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      inTimes[this->Internals->CurrentTimeIndex]);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalMultiplexing::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector, 0);

  if (!input || !output)
  {
    vtkErrorMacro("Missing valid input or output.");
    return 0;
  }

  if (!vtkCompositeDataSet::SafeDownCast(input) && !vtkDataSet::SafeDownCast(input))
  {
    vtkErrorMacro("Input should be a vtkDataSet or vtkCompositeDataSet.");
    return 0;
  }

  if (this->SelectedArrays.empty())
  {
    // Reset table to empty state
    output->Initialize();
    return 1;
  }

  // Check that dataset is actually temporal
  if (this->Internals->NumberOfTimeSteps <= 0)
  {
    vtkWarningMacro("There should be at least one timestep (non temporal).");
    return 0;
  }

  if (this->FieldAssociation != vtkDataObject::POINT &&
    this->FieldAssociation != vtkDataObject::CELL)
  {
    vtkWarningMacro("Invalid field association. Only point and cell associations are supported. "
                    "Defaulting to point association.");
    this->FieldAssociation = vtkDataObject::POINT;
  }

  // For the first request, let the pipeline know it should loop and setup arrays
  if (this->Internals->CurrentTimeIndex == 0)
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    vtkSmartPointer<vtkDataSetAttributes> attributes;
    vtkIdType nbOfArrays = 0;
    this->GetArraysInformation(input, attributes, nbOfArrays);
    this->PrepareVectorsOfArrays(attributes, nbOfArrays);
  }

  // Retrieve each data array then add it to the vector
  // of arrays for the current timestep
  if (auto inputCDS = vtkCompositeDataSet::SafeDownCast(input))
  {
    this->FillArraysForCurrentTimestep(inputCDS);
  }
  else if (auto inputDS = vtkDataSet::SafeDownCast(input))
  {
    this->FillArraysForCurrentTimestep(inputDS);
  }
  else
  {
    vtkErrorMacro("Input should be vtkDataSet or vtkCompositeDataSet.");
    return 0;
  }

  // Stop looping when the last timestep has been processed and prepare output
  this->Internals->CurrentTimeIndex++;

  if (this->Internals->CurrentTimeIndex == this->Internals->NumberOfTimeSteps)
  {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->Internals->CurrentTimeIndex = 0;
    this->CreateMultiDimensionalArrays(output);

    if (this->GenerateTimeColumn)
    {
      this->CreateTimeArray(inputVector, output);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::GetArraysInformation(
  vtkDataObject* input, vtkSmartPointer<vtkDataSetAttributes>& attributes, vtkIdType& nbOfArrays)
{
  if (auto inputCDS = vtkCompositeDataSet::SafeDownCast(input))
  {
    // For array initialization, retrieving one object is enough
    for (auto node : vtk::Range(inputCDS))
    {
      vtkDataSet* inputObj = vtkDataSet::SafeDownCast(node);

      if (inputObj)
      {
        if (this->FieldAssociation == vtkDataObject::POINT)
        {
          nbOfArrays = inputCDS->GetNumberOfPoints();
          attributes = inputObj->GetPointData();
        }
        else
        {
          nbOfArrays = inputCDS->GetNumberOfCells();
          attributes = inputObj->GetCellData();
        }

        break;
      }
    }
  }
  else if (auto inputDS = vtkDataSet::SafeDownCast(input))
  {
    if (this->FieldAssociation == vtkDataObject::POINT)
    {
      nbOfArrays = inputDS->GetNumberOfPoints();
      attributes = inputDS->GetPointData();
    }
    else
    {
      nbOfArrays = inputDS->GetNumberOfCells();
      attributes = inputDS->GetCellData();
    }
  }
  else
  {
    vtkWarningMacro("Input should be vtkDataSet or vtkCompositeDataSet.");
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::PrepareVectorsOfArrays(
  const vtkSmartPointer<vtkDataSetAttributes>& attributes, vtkIdType nbOfArrays)
{
  using SupportedArrays = vtkArrayDispatch::AllArrays;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedArrays>;

  this->Internals->Workers.clear();
  this->Internals->Workers.reserve(this->SelectedArrays.size());

  ::WorkerCreator workerCreator;

  for (const auto& name : this->SelectedArrays)
  {
    vtkDataArray* array = attributes->GetArray(name.c_str());
    if (!array)
    {
      continue;
    }

    std::shared_ptr<::Worker> typeErasedWorker;
    Dispatcher::Execute(array, workerCreator, typeErasedWorker);
    if (typeErasedWorker)
    {
      this->Internals->Workers.emplace_back(typeErasedWorker);
      typeErasedWorker->InitData(
        nbOfArrays, this->Internals->NumberOfTimeSteps, array->GetNumberOfComponents(), name);
    }
    else
    {
      vtkWarningMacro(<< "Ignoring array " << name << " : type " << array->GetArrayTypeAsString()
                      << " could not be dispatched.");
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::FillArraysForCurrentTimestep(vtkDataSet* inputDS)
{
  vtkDataSetAttributes* attributes = inputDS->GetAttributes(this->FieldAssociation);

  for (auto worker : this->Internals->Workers)
  {
    vtkDataArray* array = attributes->GetArray(worker->ArrayName.c_str());
    if (!array)
    {
      continue;
    }

    ::Worker& workerRef = *worker;
    workerRef(array, this->Internals->CurrentTimeIndex, 0);
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::FillArraysForCurrentTimestep(vtkCompositeDataSet* inputCDS)
{
  for (auto worker : this->Internals->Workers)
  {
    vtkIdType offset = 0;

    // Iterate over datasets
    for (auto node : vtk::Range(inputCDS))
    {
      vtkDataSet* dataset = vtkDataSet::SafeDownCast(node);
      if (!dataset)
      {
        continue;
      }

      vtkDataSetAttributes* attributes = dataset->GetAttributes(this->FieldAssociation);
      vtkDataArray* array = attributes->GetArray(worker->ArrayName.c_str());
      if (!array)
      {
        // To avoid partial arrays on composite
        break;
      }

      ::Worker& workerRef = *worker;
      workerRef(array, this->Internals->CurrentTimeIndex, offset);
      offset += array->GetNumberOfTuples();
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::CreateMultiDimensionalArrays(vtkTable* output)
{
  // Create multi dimensional arrays from each vector of arrays
  for (const auto& worker : this->Internals->Workers)
  {
    vtkSmartPointer<vtkDataArray> mdArray = worker->ConstructMDArray();
    output->AddColumn(mdArray);
  }
}

//------------------------------------------------------------------------------
void vtkTemporalMultiplexing::CreateTimeArray(vtkInformationVector** inputVector, vtkTable* output)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    return;
  }

  double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  vtkNew<vtkDoubleArray> timeArray;
  timeArray->SetName(::TIME_ARRAY_NAME);
  timeArray->SetArray(inTimes, this->Internals->NumberOfTimeSteps, 1);
  output->AddColumn(timeArray);
}

//--------------------------------------- --------------------------------------
void vtkTemporalMultiplexing::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: " << this->FieldAssociation << std::endl;
  os << indent << "Selected Arrays:" << std::endl;
  vtkIndent nextIndent = indent.GetNextIndent();
  std::for_each(this->SelectedArrays.cbegin(), this->SelectedArrays.cend(),
    [&](const std::string& arrayName) { os << nextIndent << arrayName << std::endl; });
}
