// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the NumberOfComponents.  This is abstract to make sure
   * that the value for the NumberOfComponents cannot be changed.
   */
  unsigned int GetNumberOfComponents() override { return 1; };

  /**
   * Compute the field value at Point.
   */
  double ComputeComponenentAtPoint(
    unsigned int component, double point[3], unsigned long timeStep, double time) override = 0;

protected:
  vtkCPScalarFieldFunction();
  ~vtkCPScalarFieldFunction() override;

private:
  vtkCPScalarFieldFunction(const vtkCPScalarFieldFunction&) = delete;
  void operator=(const vtkCPScalarFieldFunction&) = delete;
};

#endif
