// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVThreshold.h"

#include "vtkAppendFilter.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridThreshold.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

#include <limits>

vtkStandardNewMacro(vtkPVThreshold);

//----------------------------------------------------------------------------
void vtkPVThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVThreshold::ThresholdUsingSuperclassInstance(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkNew<Superclass> instance;

  instance->SetThresholdFunction(this->GetThresholdFunction());
  instance->SetUpperThreshold(this->GetUpperThreshold());
  instance->SetLowerThreshold(this->GetLowerThreshold());
  instance->SetComponentMode(this->GetComponentMode());
  instance->SetSelectedComponent(this->GetSelectedComponent());
  instance->SetAllScalars(this->GetAllScalars());
  instance->SetUseContinuousCellRange(this->GetUseContinuousCellRange());
  instance->SetInvert(this->GetInvert());
  instance->SetOutputPointsPrecision(this->GetOutputPointsPrecision());

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  instance->SetInputDataObject(inputDO);
  instance->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  if (instance->GetExecutive()->Update())
  {
    outputDO->ShallowCopy(instance->GetOutput());
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVThreshold::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro(<< "Failed to get input information.");
    return 0;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro(<< "Failed to get output information.");
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  if (vtkHyperTreeGrid::SafeDownCast(inDataObj))
  {
    vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
    if (inScalars && inScalars->GetNumberOfComponents() > 1)
    {
      outDataObj->ShallowCopy(inDataObj);
      vtkWarningMacro(
        << "Hyper Tree Grid does not support multi-components arrays: copying input.");
      return 1;
    }

    // Match behavior from vtkThreshold
    vtkNew<vtkHyperTreeGridThreshold> thresholdFilter;
    thresholdFilter->SetMemoryStrategy(this->MemoryStrategy);
    if (this->ThresholdFunction == &vtkThreshold::Lower)
    {
      thresholdFilter->ThresholdBetween(
        -std::numeric_limits<double>::infinity(), this->LowerThreshold);
    }
    else if (this->ThresholdFunction == &vtkThreshold::Upper)
    {
      thresholdFilter->ThresholdBetween(
        this->UpperThreshold, std::numeric_limits<double>::infinity());
    }
    else if (this->ThresholdFunction == &vtkThreshold::Between)
    {
      thresholdFilter->ThresholdBetween(this->LowerThreshold, this->UpperThreshold);
    }
    else
    {
      vtkErrorMacro("Threshold function not found");
    }

    vtkDataObject* inputClone = inDataObj->NewInstance();
    inputClone->ShallowCopy(inDataObj);
    thresholdFilter->SetInputData(0, inputClone);
    inputClone->FastDelete();

    thresholdFilter->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
    thresholdFilter->Update();
    outDataObj->ShallowCopy(thresholdFilter->GetOutput(0));

    return 1;
  }

  return this->ThresholdUsingSuperclassInstance(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVThreshold::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // this is needed since vtkUnstructuredGridAlgorithm does not have a
  // `RequestDataObject`.
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVThreshold::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkHyperTreeGrid::GetData(inInfo))
  {
    vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outInfo);
    if (!output)
    {
      output = vtkHyperTreeGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  else if (vtkDataSet::GetData(inInfo))
  {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVThreshold::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVThreshold::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}
