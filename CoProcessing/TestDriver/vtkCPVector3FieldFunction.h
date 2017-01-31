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
/**
 * @class   vtkCPVector3FieldFunction
 * @brief   Abstract class for specifying vectors at points.
 *
 * Abstract class for specifying vector values at specified points.
*/

#ifndef vtkCPVector3FieldFunction_h
#define vtkCPVector3FieldFunction_h

#include "vtkCPTensorFieldFunction.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPVector3FieldFunction : public vtkCPTensorFieldFunction
{
public:
  vtkTypeMacro(vtkCPVector3FieldFunction, vtkCPTensorFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the NumberOfComponents.
   */
  virtual unsigned int GetNumberOfComponents() VTK_OVERRIDE { return 3; };

  /**
   * Compute the field value at Point.
   */
  virtual double ComputeComponenentAtPoint(
    unsigned int component, double point[3], unsigned long timeStep, double time) VTK_OVERRIDE = 0;

protected:
  vtkCPVector3FieldFunction();
  ~vtkCPVector3FieldFunction();

private:
  vtkCPVector3FieldFunction(const vtkCPVector3FieldFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCPVector3FieldFunction&) VTK_DELETE_FUNCTION;
};

#endif
