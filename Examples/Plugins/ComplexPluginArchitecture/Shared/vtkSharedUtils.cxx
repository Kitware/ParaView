/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSharedUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#define _USE_MATH_DEFINES

#include "vtkSharedUtils.h"

#include <vtkObjectFactory.h>

#include <cmath>

vtkStandardNewMacro(vtkSharedUtils);

//----------------------------------------------------------------------------
vtkSharedUtils::vtkSharedUtils() = default;

//----------------------------------------------------------------------------
vtkSharedUtils::~vtkSharedUtils() = default;

//----------------------------------------------------------------------------
double vtkSharedUtils::Pi()
{
  return M_PI;
}

//----------------------------------------------------------------------------
float vtkSharedUtils::DegreesFromRadians(float x)
{
  return x * 180.0f / M_PI;
}

//----------------------------------------------------------------------------
double vtkSharedUtils::DegreesFromRadians(double x)
{
  return x * 180.0 / M_PI;
}

//----------------------------------------------------------------------------
void vtkSharedUtils::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
