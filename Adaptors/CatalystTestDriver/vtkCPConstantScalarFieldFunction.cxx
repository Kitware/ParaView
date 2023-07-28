// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPConstantScalarFieldFunction.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCPConstantScalarFieldFunction);

//----------------------------------------------------------------------------
vtkCPConstantScalarFieldFunction::vtkCPConstantScalarFieldFunction()
{
  this->Constant = 0;
}

//----------------------------------------------------------------------------
vtkCPConstantScalarFieldFunction::~vtkCPConstantScalarFieldFunction() = default;

//----------------------------------------------------------------------------
double vtkCPConstantScalarFieldFunction::ComputeComponenentAtPoint(unsigned int component,
  double* vtkNotUsed(point), unsigned long vtkNotUsed(timeStep), double vtkNotUsed(time))
{
  if (component != 0)
  {
    vtkWarningMacro("Bad component value");
  }
  return this->Constant;
}

//----------------------------------------------------------------------------
void vtkCPConstantScalarFieldFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Constant: " << this->Constant << endl;
}
