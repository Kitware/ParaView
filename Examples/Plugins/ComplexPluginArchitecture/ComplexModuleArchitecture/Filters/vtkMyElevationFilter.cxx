// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMyElevationFilter.h"

#include "vtkCoreUtils.h"
#include "vtkSharedUtils.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkMyElevationFilter);

int vtkMyElevationFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  std::cout << "Did you know that Pi value in radians is "
            << vtkCoreUtils::RadiansFromDegrees(
                 vtkSharedUtils::DegreesFromRadians(vtkSharedUtils::Pi()))
            << " ?" << std::endl;
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkMyElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
