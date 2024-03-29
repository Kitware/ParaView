// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the NumberOfComponents.
   */
  unsigned int GetNumberOfComponents() override { return 3; };

  /**
   * Compute the field value at Point.
   */
  double ComputeComponenentAtPoint(
    unsigned int component, double point[3], unsigned long timeStep, double time) override = 0;

protected:
  vtkCPVector3FieldFunction();
  ~vtkCPVector3FieldFunction() override;

private:
  vtkCPVector3FieldFunction(const vtkCPVector3FieldFunction&) = delete;
  void operator=(const vtkCPVector3FieldFunction&) = delete;
};

#endif
