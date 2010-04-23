/*=========================================================================

  Program:   ParaView
  Module:    vtkCPScalarFieldFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPScalarFieldFunction - Abstract class for specifying scalars at points.
// .SECTION Description
// Abstract class for specifying scalars at specified points.  

#ifndef __vtkCPScalarFieldFunction_h
#define __vtkCPScalarFieldFunction_h

#include "vtkCPTensorFieldFunction.h"

class VTK_EXPORT vtkCPScalarFieldFunction : public vtkCPTensorFieldFunction
{
public:
  vtkTypeMacro(vtkCPScalarFieldFunction, vtkCPTensorFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the NumberOfComponents.  This is abstract to make sure 
  // that the value for the NumberOfComponents cannot be changed.
  virtual unsigned int GetNumberOfComponents() {return 1;};

  // Description:
  // Compute the field value at Point.
  virtual double ComputeComponenentAtPoint(unsigned int component, double point[3],
                                           unsigned long timeStep, double time) = 0;

protected:
  vtkCPScalarFieldFunction();
  ~vtkCPScalarFieldFunction();

private:
  vtkCPScalarFieldFunction(const vtkCPScalarFieldFunction&); // Not implemented
  void operator=(const vtkCPScalarFieldFunction&); // Not implemented
};

#endif
