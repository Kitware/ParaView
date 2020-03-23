/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVTrivialProducer.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrivialProducer.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>
#include <vector>

struct vtkPVTrivialProducerInternal
{
  std::vector<double> TimeSteps;
  double FindNearestTime(double time) const
  {
    // we can't assume TimeSteps is sorted (see logic in SetOutput(.., time)),
    // although in most cases it is.
    double nearest_time = time;
    double delta = VTK_DOUBLE_MAX;
    for (const auto& t : this->TimeSteps)
    {
      const auto tdelta = std::abs(t - time);
      if (tdelta < delta)
      {
        delta = tdelta;
        nearest_time = t;
      }
    }
    return nearest_time;
  }
};

vtkStandardNewMacro(vtkPVTrivialProducer);
//----------------------------------------------------------------------------
vtkPVTrivialProducer::vtkPVTrivialProducer()
{
  this->Internals = new vtkPVTrivialProducerInternal;
}

//----------------------------------------------------------------------------
vtkPVTrivialProducer::~vtkPVTrivialProducer()
{
  if (this->Internals)
  {
    delete this->Internals;
    this->Internals = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::SetOutput(vtkDataObject* output)
{
  this->Superclass::SetOutput(output);
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::SetOutput(vtkDataObject* output, double time)
{
  if (this->Internals->TimeSteps.empty() == false && time <= this->Internals->TimeSteps.back())
  {
    vtkWarningMacro("New time step is not after last time step.");
  }
  this->Internals->TimeSteps.push_back(time);

  this->Modified();
  this->SetOutput(output);
}

//----------------------------------------------------------------------------
int vtkPVTrivialProducer::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::ProcessRequest(request, inputVector, outputVector))
  {
    return 0;
  }

  const auto& internals = (*this->Internals);

  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    auto dobj = vtkDataObject::GetData(outputVector, 0);
    if (!internals.TimeSteps.empty() && dobj != nullptr)
    {
      double uTime = outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
        ? outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
        : internals.TimeSteps.back();

      // we really don't produce any other timestep besides the last one. Let's
      // check, however, if someone requested another timestep what we can
      // satisfy. In that case, let's report a warning.
      if (internals.FindNearestTime(uTime) != internals.TimeSteps.back())
      {
        vtkNumberToString convert;
        vtkWarningMacro("Cannot produce requested time '" << convert(uTime) << "', only '"
                                                          << convert(internals.TimeSteps.back())
                                                          << "' is available.");
      }

      dobj->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), internals.TimeSteps.back());
    }
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    if (!internals.TimeSteps.empty())
    {
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &internals.TimeSteps[0],
        static_cast<int>(internals.TimeSteps.size()));
      double timeRange[2] = { this->Internals->TimeSteps.front(),
        this->Internals->TimeSteps.back() };
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
