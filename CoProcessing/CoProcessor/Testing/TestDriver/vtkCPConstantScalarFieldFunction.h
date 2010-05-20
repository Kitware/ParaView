/*=========================================================================

  Program:   ParaView
  Module:    vtkCPConstantScalarFieldFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPConstantScalarFieldFunction - Class for specifying constant scalars at points.
// .SECTION Description
// Class for specifying a constant scalar field.

#ifndef __vtkCPConstantScalarFieldFunction_h
#define __vtkCPConstantScalarFieldFunction_h

#include "vtkCPScalarFieldFunction.h"

class VTK_EXPORT vtkCPConstantScalarFieldFunction : public vtkCPScalarFieldFunction
{
public:
  static vtkCPConstantScalarFieldFunction * New();
  vtkTypeMacro(vtkCPConstantScalarFieldFunction, vtkCPScalarFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compute the field value at Point which is an array of length 3.
  virtual double ComputeComponenentAtPoint(unsigned int component, double* point,
                                           unsigned long timeStep, double time);

  // Description:
  // Set/get the constant value for the field.
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);

protected:
  vtkCPConstantScalarFieldFunction();
  ~vtkCPConstantScalarFieldFunction();

private:
  vtkCPConstantScalarFieldFunction(const vtkCPConstantScalarFieldFunction&); // Not implemented
  void operator=(const vtkCPConstantScalarFieldFunction&); // Not implemented

  // Description:
  // The constant value for the scalar field.
  double Constant;
};

#endif
