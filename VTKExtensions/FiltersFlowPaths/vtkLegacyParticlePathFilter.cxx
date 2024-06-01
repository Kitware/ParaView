// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLegacyParticlePathFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN

vtkObjectFactoryNewMacro(vtkLegacyParticlePathFilter);

//------------------------------------------------------------------------------
int vtkLegacyParticlePathFilter::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::RequestInformation(request, inputVector, outputVector);

  if (!this->NoPriorTimeStepAccess)
  {
    for (int i = 0; i < static_cast<int>(this->InputTimeSteps.size()); ++i)
    {
      if (this->InputTimeSteps[i] > this->TerminationTime)
      {
        vtkInformation* outInfo = outputVector->GetInformationObject(0);
        outInfo->Set(
          vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->InputTimeSteps.data(), i);
        double range[2] = { this->InputTimeSteps.front(), this->InputTimeSteps.back() };
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
        this->InputTimeSteps.resize(i);
        break;
      }
    }
  }

  return retVal;
}

VTK_ABI_NAMESPACE_END
