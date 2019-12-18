/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThreshold.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVThreshold.h"

#include "vtkAppendFilter.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridThreshold.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkUnstructuredGrid.h"

#include <limits>

vtkStandardNewMacro(vtkPVThreshold);

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
    vtkNew<vtkHyperTreeGridThreshold> thresholdFilter;
    if (this->ThresholdFunction == &vtkThreshold::Lower)
    {
      thresholdFilter->ThresholdBetween(
        this->LowerThreshold, std::numeric_limits<double>::infinity());
    }
    else if (this->ThresholdFunction == &vtkThreshold::Upper)
    {
      thresholdFilter->ThresholdBetween(
        -std::numeric_limits<double>::infinity(), this->UpperThreshold);
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
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVThreshold::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
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
  vtkInformationStringVectorKey::SafeDownCast(
    info->GetKey(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))
    ->Append(info, "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVThreshold::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}
