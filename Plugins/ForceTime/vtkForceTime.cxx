/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkForceTime.h"

#include "vtkTemporalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkForceTime);

//----------------------------------------------------------------------------
vtkForceTime::vtkForceTime()
{
  this->ForcedTime = 0.0;
  this->IgnorePipelineTime = 1;
}

//----------------------------------------------------------------------------
vtkForceTime::~vtkForceTime()
{
}

//----------------------------------------------------------------------------
void vtkForceTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ForcedTime: " << this->ForcedTime << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
}
//----------------------------------------------------------------------------
// Change the information
int vtkForceTime::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
    double range[2];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),range);
    if(this->IgnorePipelineTime)
      {
      range[0] = this->ForcedTime;
      range[1] = this->ForcedTime;
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
      range,2);
    }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double *outTimes;
    if(this->IgnorePipelineTime)
      {
      outTimes = new double [numTimes];
      int i;
      for (i=0; i<numTimes; ++i)
        {
        outTimes[i] = this->ForcedTime;
        }
      }
    else
      {
      outTimes = inTimes;
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 outTimes,numTimes);

    if(this->IgnorePipelineTime)
      {
      delete [] outTimes;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkForceTime::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject *inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *outData = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // shallow copy the data
  if (inData && outData)
    {
    outData->ShallowCopy(inData);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkForceTime::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // override the time request if IgnorePipelineTime is on.
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *upTimes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numTimes =
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    double *inTimes = new double [numTimes];

    for (int i=0; i<numTimes; ++i)
      {
      if(this->IgnorePipelineTime)
        {
        inTimes[i] = this->ForcedTime;
        }
      else
        {
        inTimes[i] = upTimes[i];
        }
      }
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                inTimes,numTimes);
    delete [] inTimes;
    }

  return 1;
}
