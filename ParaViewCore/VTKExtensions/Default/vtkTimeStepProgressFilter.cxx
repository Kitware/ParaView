/*=========================================================================

  Program:   ParaView
  Module:    vtkTimeStepProgressFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTimeStepProgressFilter.h"

#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>

vtkStandardNewMacro(vtkTimeStepProgressFilter);
//----------------------------------------------------------------------------
vtkTimeStepProgressFilter::vtkTimeStepProgressFilter()
{
  this->UseTimeRange = false;
  this->NumTimeSteps = 0;
  this->TimeSteps = NULL;
  this->TimeRange[0] = 0;
  this->TimeRange[1] = 1;
}

//----------------------------------------------------------------------------
vtkTimeStepProgressFilter::~vtkTimeStepProgressFilter()
{
  if (this->TimeSteps != NULL)
  {
    delete[] this->TimeSteps;
  }
}

//----------------------------------------------------------------------------
int vtkTimeStepProgressFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkTimeStepProgressFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Store and copy time info
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), this->TimeRange);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), this->TimeRange, 2);
    this->UseTimeRange = true;
  }
  else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->NumTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    if (this->TimeSteps != NULL)
    {
      delete[] this->TimeSteps;
    }
    this->TimeSteps = new double[this->NumTimeSteps];

    for (int i = 0; i < this->NumTimeSteps; i++)
    {
      this->TimeSteps[i] = inTimes[i];
    }
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps, this->NumTimeSteps);
    this->UseTimeRange = false;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkTimeStepProgressFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  vtkInformation* inputInfo = input ? input->GetInformation() : 0;

  double val = 0;
  if (inputInfo && inputInfo->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    double time = inputInfo->Get(vtkDataObject::DATA_TIME_STEP());
    if (this->UseTimeRange && time >= this->TimeRange[0] && time <= this->TimeRange[1])
    {
      val = (time - this->TimeRange[0]) / (this->TimeRange[1] - this->TimeRange[0]);
    }
    else if (this->NumTimeSteps != 0)
    {
      int i = std::lower_bound(this->TimeSteps, this->TimeSteps + this->NumTimeSteps, time) -
        this->TimeSteps;
      if (this->TimeSteps[i] != time)
      {
        if (std::abs((this->TimeSteps[i - 1] - time)) < std::abs((time - this->TimeSteps[i])))
        {
          i--;
        }
      }
      val = static_cast<double>(i) / this->NumTimeSteps;
    }
  }
  vtkNew<vtkDoubleArray> data;
  data->SetName("ProgressRate");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(val);
  output->AddColumn(data.Get());
  return 1;
}

//----------------------------------------------------------------------------
void vtkTimeStepProgressFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
