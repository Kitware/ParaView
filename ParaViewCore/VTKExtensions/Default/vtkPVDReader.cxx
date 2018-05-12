/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDReader.h"

#include "vtkDataObject.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkPVDReader);

//----------------------------------------------------------------------------
vtkPVDReader::vtkPVDReader()
{
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
}

//----------------------------------------------------------------------------
vtkPVDReader::~vtkPVDReader()
{
}

//----------------------------------------------------------------------------
void vtkPVDReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << " " << this->TimeStepRange[1]
     << "\n";
}

//----------------------------------------------------------------------------
void vtkPVDReader::SetTimeStep(int index)
{
  this->SetRestrictionAsIndex("timestep", index);
}

//----------------------------------------------------------------------------
int vtkPVDReader::GetTimeStep()
{
  return this->GetRestrictionAsIndex("timestep");
}

//----------------------------------------------------------------------------
void vtkPVDReader::ReadXMLData()
{
  // need to Parse the file first
  if (!this->ReadXMLInformation())
  {
    return;
  }

  vtkInformation* outInfo = this->GetCurrentOutputInformation();

  int tsLength = 0;
  double* steps = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  // Check if a particular time was requested.
  if (steps && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int numReqTimeSteps = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    if (numReqTimeSteps > 0)
    {
      // find the first time value larger than requested time value
      // this logic could be improved
      int cnt = 0;
      while (cnt < tsLength - 1 && steps[cnt] < requestedTimeStep)
      {
        cnt++;
      }
      // because steps[cnt] is sorted, we need this loop to find the actual
      // index that corresponds to the time we found in case the list did not
      // arrive in min to max sorted order already
      int cnt2 = 0;
      bool found = false;
      while (cnt2 < tsLength && !found)
      {
        double val = strtod(this->GetAttributeValue("timestep", cnt2), NULL);
        if (val == steps[cnt])
        {
          found = true;
        }
        else
        {
          cnt2++;
        }
      }
      if (!found)
      {
        cnt2 = 0;
      }
      this->SetRestrictionImpl("timestep", this->GetAttributeValue("timestep", cnt2), false);
      // This is what we will read.
      vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), steps[cnt]);
    }
  }

  this->ReadXMLDataImpl();
}

//----------------------------------------------------------------------------
int vtkPVDReader::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // need to Parse the file first
  if (!this->ReadXMLInformation())
  {
    vtkErrorMacro("Could not read file information");
    return 0;
  }

  // If the file has timesteps and no restriction was set, set the
  // restriction for the first timestep. This is to avoid reading all
  // the timesteps at once.
  if (this->GetAttributeIndex("timestep") != -1)
  {
    if (!this->GetRestriction("timestep"))
    {
      int index = this->GetAttributeIndex("timestep");
      int numTimeSteps = this->GetNumberOfAttributeValues(index);
      if (numTimeSteps > 0)
      {
        this->SetRestrictionImpl("timestep", this->GetAttributeValue("timestep", 0), false);
      }
    }
  }

  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkPVDReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  int index = this->GetAttributeIndex("timestep");
  int numTimeSteps = this->GetNumberOfAttributeValues(index);
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = numTimeSteps - 1;
  if (this->TimeStepRange[1] == -1)
  {
    this->TimeStepRange[1] = 0;
  }
  std::vector<double> timeSteps(numTimeSteps);
  for (int i = 0; i < numTimeSteps; i++)
  {
    const char* attr = this->GetAttributeValue(index, i);
    char* res = 0;
    double val = strtod(attr, &res);
    if (res == attr)
    {
      vtkErrorMacro("Could not parse timestep string: " << attr << " Setting time value to 0");
      timeSteps[i] = 0.0;
    }
    else
    {
      timeSteps[i] = val;
    }
  }
  std::sort(timeSteps.begin(), timeSteps.end());
  if (!timeSteps.empty())
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps[0], numTimeSteps);
    double timeRange[2];
    timeRange[0] = timeSteps[0];
    timeRange[1] = timeSteps[numTimeSteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
}
