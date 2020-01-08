/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRFileSeriesReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAMRFileSeriesReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkAMRFileSeriesReader);

vtkAMRFileSeriesReader::vtkAMRFileSeriesReader()
{
}

int vtkAMRFileSeriesReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
  return 1;
}

int vtkAMRFileSeriesReader::RequestUpdateTime(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    vtkGenericWarningMacro("Time update is requested but there is no time.");
    return 1;
  }
  return 1;
}

int vtkAMRFileSeriesReader::RequestUpdateTimeDependentInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    return 1;
  }
  // double upTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  int index = this->ChooseInput(outInfo);
  if (index >= static_cast<int>(this->GetNumberOfFileNames()))
  {
    // this happens when there are no files set. That's an acceptable condition
    // when the file-series is not an essential filename eg. the Q file for
    // Plot3D reader.
    index = 0;
  }

  // Make sure that the reader file name is set correctly and that
  // RequestInformation has been called.
  this->RequestInformationForInput(index, NULL, outputVector);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkAMRFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
