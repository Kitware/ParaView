// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPLinearScalarFieldFunction.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCPLinearScalarFieldFunction);

//----------------------------------------------------------------------------
vtkCPLinearScalarFieldFunction::vtkCPLinearScalarFieldFunction()
{
  this->Constant = 0;
  this->XMultiplier = 0;
  this->YMultiplier = 0;
  this->ZMultiplier = 0;
  this->TimeMultiplier = 0;
}

//----------------------------------------------------------------------------
vtkCPLinearScalarFieldFunction::~vtkCPLinearScalarFieldFunction() = default;

//----------------------------------------------------------------------------
double vtkCPLinearScalarFieldFunction::ComputeComponenentAtPoint(
  unsigned int component, double point[3], unsigned long vtkNotUsed(timeStep), double time)
{
  if (component != 0)
  {
    vtkWarningMacro("Bad component value");
  }
  double value = this->Constant + point[0] * this->XMultiplier + point[1] * this->YMultiplier +
    point[2] * this->ZMultiplier + time * this->TimeMultiplier;
  return value;
}

//----------------------------------------------------------------------------
void vtkCPLinearScalarFieldFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Constant: " << this->Constant << endl;
  os << indent << "XMultiplier: " << this->XMultiplier << endl;
  os << indent << "YMultiplier: " << this->YMultiplier << endl;
  os << indent << "ZMultiplier: " << this->ZMultiplier << endl;
  os << indent << "TimeMultiplier: " << this->TimeMultiplier << endl;
}
