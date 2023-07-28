// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPTensorFieldFunction
 * @brief   Abstract class for specifying tensor fields at points.
 *
 * Abstract class for specifying tensor fields at specified points.
 */

#ifndef vtkCPTensorFieldFunction_h
#define vtkCPTensorFieldFunction_h

#include "vtkObject.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPTensorFieldFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkCPTensorFieldFunction, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the NumberOfComponents.  This is abstract to make sure
   * that the value for the NumberOfComponents cannot be changed.
   */
  virtual unsigned int GetNumberOfComponents() = 0;

  /**
   * Compute the field value at Point.
   */
  virtual double ComputeComponenentAtPoint(
    unsigned int component, double point[3], unsigned long timeStep, double time) = 0;

protected:
  vtkCPTensorFieldFunction();
  ~vtkCPTensorFieldFunction() override;

private:
  vtkCPTensorFieldFunction(const vtkCPTensorFieldFunction&) = delete;
  void operator=(const vtkCPTensorFieldFunction&) = delete;
};

#endif
