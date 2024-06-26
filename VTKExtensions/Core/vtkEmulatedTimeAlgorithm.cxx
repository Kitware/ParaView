// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEmulatedTimeAlgorithm.h"

#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

//------------------------------------------------------------------------------
vtkEmulatedTimeAlgorithm::vtkEmulatedTimeAlgorithm()
{
  this->TimeRange[0] = 0.;
  this->TimeRange[1] = 0.;
}

//------------------------------------------------------------------------------
vtkEmulatedTimeAlgorithm::~vtkEmulatedTimeAlgorithm() = default;

//------------------------------------------------------------------------------
void vtkEmulatedTimeAlgorithm::Modified()
{
  this->NeedsInitialization = true;
  this->Superclass::Modified();
}

//------------------------------------------------------------------------------
bool vtkEmulatedTimeAlgorithm::GetNeedsUpdate(double time)
{
  bool isRequested = false;
  double requestedTime = 0.;
  auto found = std::find_if(this->TimeSteps.rbegin(), this->TimeSteps.rend(),
    [&time](double current) { return time >= current; });
  if (found != this->TimeSteps.rend())
  {
    requestedTime = *found;
    isRequested = true;
  }
  else if (time < this->TimeRange[0])
  {
    requestedTime = this->TimeRange[0];
    isRequested = true;
  }

  double lastRequestedTime = this->RequestedTime;
  if (isRequested && requestedTime != lastRequestedTime)
  {
    this->Superclass::Modified();
    this->NeedsUpdate = true;
    this->RequestedTime = requestedTime;
  }
  return this->NeedsUpdate;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkEmulatedTimeAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()) && this->NeedsInitialization)
  {
    vtkTypeBool ret = this->RequestInformation(request, inputVector, outputVector);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      int nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      double* rsteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      this->TimeSteps.assign(rsteps, rsteps + nSteps);
    }
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
      const double* timeRange = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      if (len != 2)
      {
        vtkWarningMacro(<< "Inappropriate time range.");
      }
      else
      {
        this->TimeRange[0] = timeRange[0];
        this->TimeRange[1] = timeRange[1];
      }
    }
    else if (!this->TimeSteps.empty())
    {
      this->TimeRange[0] = this->TimeSteps.front();
      this->TimeRange[1] = this->TimeSteps.back();
    }
    this->NeedsInitialization = false;
    return ret;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkEmulatedTimeAlgorithm::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->NeedsUpdate)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->RequestedTime);
    this->NeedsUpdate = false;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkEmulatedTimeAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NeedsUpdate: " << this->NeedsUpdate << "\n";
  os << indent << "RequestedTime: " << this->RequestedTime << "\n";
  os << indent << "TimeRange: [" << this->TimeRange[0] << ", " << this->TimeRange[1] << "]\n";
  os << indent << "TimeSteps:";
  if (this->TimeSteps.empty())
  {
    os << "( empty )\n";
  }
  else
  {
    os << "[";
    for (int idx = 0; idx < this->TimeSteps.size() - 1; ++idx)
    {
      os << this->TimeSteps[idx] << ", ";
    }
    os << this->TimeSteps[this->TimeSteps.size() - 1] << "]\n";
  }
}
