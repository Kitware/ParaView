/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCoreUtils.h"

#include <vtkMath.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkCoreUtils);

//----------------------------------------------------------------------------
vtkCoreUtils::vtkCoreUtils() = default;

//----------------------------------------------------------------------------
vtkCoreUtils::~vtkCoreUtils() = default;

//----------------------------------------------------------------------------
float vtkCoreUtils::RadiansFromDegrees(float x)
{
  return vtkMath::RadiansFromDegrees(x);
}

//----------------------------------------------------------------------------
double vtkCoreUtils::RadiansFromDegrees(double x)
{
  return vtkMath::RadiansFromDegrees(x);
}

//----------------------------------------------------------------------------
void vtkCoreUtils::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
