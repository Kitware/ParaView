/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTemporalDataInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTemporalDataInformation.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

vtkStandardNewMacro(vtkPVTemporalDataInformation);
//----------------------------------------------------------------------------
vtkPVTemporalDataInformation::vtkPVTemporalDataInformation() = default;

//----------------------------------------------------------------------------
vtkPVTemporalDataInformation::~vtkPVTemporalDataInformation() = default;

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::CopyFromObject(vtkObject* object)
{
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(object);
  vtkAlgorithmOutput* port = vtkAlgorithmOutput::SafeDownCast(object);
  if (algo)
  {
    port = algo->GetOutputPort(this->GetPortNumber());
  }

  if (!port)
  {
    vtkErrorMacro("vtkPVTemporalDataInformation needs a vtkAlgorithm or "
                  " a vtkAlgorithmOutput.");
    return;
  }

  port->GetProducer()->Update();
  vtkDataObject* dobj = port->GetProducer()->GetOutputDataObject(port->GetIndex());

  // Collect current information.
  this->Superclass::CopyFromObject(port);
  if (!this->GetHasTime() || this->GetTimeRange()[0] == this->GetTimeRange()[1])
  {
    // nothing temporal about this data! Nothing to do.
    return;
  }

  // We are not assured that this data has time. We currently only handle
  // timesteps properly, for contiguous time-range, we simply use the first and
  // last time value as the 2 timesteps.

  vtkInformation* pipelineInfo = port->GetProducer()->GetOutputInformation(port->GetIndex());
  std::vector<double> timesteps;
  if (pipelineInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* ptimesteps = pipelineInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int length = pipelineInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    timesteps.resize(length);
    for (int cc = 0; cc < length; cc++)
    {
      timesteps[cc] = ptimesteps[cc];
    }
  }
  else if (pipelineInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    double* ptimesteps = pipelineInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    timesteps.push_back(ptimesteps[0]);
    timesteps.push_back(ptimesteps[1]);
  }

  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(port->GetProducer()->GetExecutive());
  if (!sddp)
  {
    vtkErrorMacro("This class expects vtkStreamingDemandDrivenPipeline.");
    return;
  }

  double current_time = this->GetTime();
  for (auto time : timesteps)
  {
    if (time == current_time)
    {
      // skip the timestep already seen.
      continue;
    }
    pipelineInfo->Set(sddp->UPDATE_TIME_STEP(), time);
    sddp->Update(port->GetIndex());

    dobj = port->GetProducer()->GetOutputDataObject(port->GetIndex());

    vtkNew<vtkPVDataInformation> dinfo;
    dinfo->CopyFromObject(dobj);
    this->AddInformation(dinfo);
  }
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
