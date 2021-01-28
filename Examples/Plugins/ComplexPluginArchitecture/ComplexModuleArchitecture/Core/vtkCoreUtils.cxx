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

#include <vtkObjectFactory.h>

#define _USE_MATH_DEFINES
#include <math.h>

vtkStandardNewMacro(vtkCoreUtils);

//----------------------------------------------------------------------------
float vtkCoreUtils::RadiansFromDegrees(float x)
{
  return x * M_PI / 180.0f;
}

//----------------------------------------------------------------------------
double vtkCoreUtils::RadiansFromDegrees(double x)
{
  return x * M_PI / 180.0;
}

//----------------------------------------------------------------------------
void vtkCoreUtils::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
