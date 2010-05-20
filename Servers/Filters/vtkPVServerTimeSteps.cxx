/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerTimeSteps.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerTimeSteps.h"

#include "vtkAlgorithm.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerTimeSteps);

//----------------------------------------------------------------------------
class vtkPVServerTimeStepsInternals
{
public:
  vtkClientServerStream Result;
};


//----------------------------------------------------------------------------
vtkPVServerTimeSteps::vtkPVServerTimeSteps()
{
  this->Internal = new vtkPVServerTimeStepsInternals;
}

//----------------------------------------------------------------------------
vtkPVServerTimeSteps::~vtkPVServerTimeSteps()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVServerTimeSteps::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVServerTimeSteps::GetTimeSteps(
  vtkAlgorithm* algo)
{
  this->Internal->Result.Reset();
  this->Internal->Result << vtkClientServerStream::Reply;
  vtkInformation* outInfo = algo->GetExecutive()->GetOutputInformation(0);
  if (outInfo)
    {
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
      {
      const  double* timeSteps
        = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
        double timeRange[2];
      if (len > 0)
        {
        timeRange[0] = timeSteps[0];
        timeRange[1] = timeSteps[len-1];
        this->Internal->Result
          << vtkClientServerStream::InsertArray(timeRange, 2);
        }
      else
        {
        this->Internal->Result
          << vtkClientServerStream::InsertArray(timeRange, 0);
        }
      this->Internal->Result 
        << vtkClientServerStream::InsertArray(timeSteps, len);
      }
    else if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
      {
      const double *timeRange
        = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      if (len != 2)
        {
        vtkWarningMacro(<< "Filter reports inappropriate time range.");
        }
      this->Internal->Result
        << vtkClientServerStream::InsertArray(timeRange, 2);
      }
    }
  this->Internal->Result << vtkClientServerStream::End;
  return this->Internal->Result;
}
