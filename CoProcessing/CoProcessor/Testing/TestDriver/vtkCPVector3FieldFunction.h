/*=========================================================================

  Program:   ParaView
  Module:    vtkCPVector3FieldFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPVector3FieldFunction - Abstract class for specifying vectors at points.
// .SECTION Description
// Abstract class for specifying vector values at specified points.  

#ifndef __vtkCPVector3FieldFunction_h
#define __vtkCPVector3FieldFunction_h

#include "vtkCPTensorFieldFunction.h"

class VTK_EXPORT vtkCPVector3FieldFunction : public vtkCPTensorFieldFunction
{
public:
  vtkTypeMacro(vtkCPVector3FieldFunction, vtkCPTensorFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the NumberOfComponents.  
  virtual unsigned int GetNumberOfComponents() {return 3;};

  // Description:
  // Compute the field value at Point.
  virtual double ComputeComponenentAtPoint(unsigned int component, double point[3],
                                           unsigned long timeStep, double time) = 0;

protected:
  vtkCPVector3FieldFunction();
  ~vtkCPVector3FieldFunction();

private:
  vtkCPVector3FieldFunction(const vtkCPVector3FieldFunction&); // Not implemented
  void operator=(const vtkCPVector3FieldFunction&); // Not implemented
};

#endif
