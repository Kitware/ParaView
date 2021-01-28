/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
