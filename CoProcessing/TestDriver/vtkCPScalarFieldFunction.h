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
/**
 * @class   vtkCPScalarFieldFunction
 * @brief   Abstract class for specifying scalars at points.
 *
 * Abstract class for specifying scalars at specified points.
*/

#ifndef vtkCPScalarFieldFunction_h
#define vtkCPScalarFieldFunction_h

#include "vtkCPTensorFieldFunction.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPScalarFieldFunction : public vtkCPTensorFieldFunction
{
public:
  vtkTypeMacro(vtkCPScalarFieldFunction, vtkCPTensorFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the NumberOfComponents.  This is abstract to make sure
   * that the value for the NumberOfComponents cannot be changed.
   */
  virtual unsigned int GetNumberOfComponents() VTK_OVERRIDE { return 1; };

  /**
   * Compute the field value at Point.
   */
  virtual double ComputeComponenentAtPoint(
    unsigned int component, double point[3], unsigned long timeStep, double time) VTK_OVERRIDE = 0;

protected:
  vtkCPScalarFieldFunction();
  ~vtkCPScalarFieldFunction();

private:
  vtkCPScalarFieldFunction(const vtkCPScalarFieldFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCPScalarFieldFunction&) VTK_DELETE_FUNCTION;
};

#endif
