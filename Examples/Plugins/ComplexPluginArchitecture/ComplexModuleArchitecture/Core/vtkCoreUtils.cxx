// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
