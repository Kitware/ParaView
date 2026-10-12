// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSlowFilter.h"

#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <thread>

vtkStandardNewMacro(vtkSlowFilter);

//------------------------------------------------------------------------------
int vtkSlowFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkSlowFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);
  output->ShallowCopy(input);

  std::size_t numberOfPoint = input->GetNumberOfPoints();

  for (std::size_t i = 0; i < numberOfPoint; i += 5)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(this->SleepTime));
    if (i % 50 == 0)
    {
      if (this->CheckAbortAndInvoke())
      {
        break;
      }
      this->UpdateProgress(static_cast<double>(i) / static_cast<double>(numberOfPoint));
    }
  }
  this->InvokeCleanupAbortCheck();

  return 1;
}

//------------------------------------------------------------------------------
void vtkSlowFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "SleepTime: " << this->SleepTime << endl;
  this->Superclass::PrintSelf(os, indent);
}
