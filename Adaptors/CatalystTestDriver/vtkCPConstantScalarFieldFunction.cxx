/*=========================================================================

  Program:   ParaView
  Module:    vtkCPConstantScalarFieldFunction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
