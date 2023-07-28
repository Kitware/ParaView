// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
