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
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerTimeSteps);
vtkCxxRevisionMacro(vtkPVServerTimeSteps, "1.2.2.1");

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
    const  double* timeSteps = 
      outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (timeSteps)
      {
      int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      if (len > 0)
        {
        this->Internal->Result 
          << vtkClientServerStream::InsertArray(timeSteps, len);
        }
      }
    }
  this->Internal->Result << vtkClientServerStream::End;
  return this->Internal->Result;
}
