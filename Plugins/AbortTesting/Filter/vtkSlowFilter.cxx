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

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSlowFilter);

//------------------------------------------------------------------------------
int vtkSlowFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkSlowFilter::RequestData(vtkInformation*, vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);

  std::size_t numberOfPoint = input->GetNumberOfPoints();

  for (std::size_t i = 0; i < numberOfPoint; i += 5)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    if (i % 50 == 0)
    {
      if (this->CheckAbort())
      {
        break;
      }
      this->UpdateProgress(static_cast<double>(i) / static_cast<double>(numberOfPoint));
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkSlowFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
