// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPConstantScalarFieldFunction
 * @brief   Class for specifying constant scalars at points.
 *
 * Class for specifying a constant scalar field.
 */

#ifndef vtkCPConstantScalarFieldFunction_h
#define vtkCPConstantScalarFieldFunction_h

#include "vtkCPScalarFieldFunction.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPConstantScalarFieldFunction
  : public vtkCPScalarFieldFunction
{
public:
  static vtkCPConstantScalarFieldFunction* New();
  vtkTypeMacro(vtkCPConstantScalarFieldFunction, vtkCPScalarFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute the field value at Point which is an array of length 3.
   */
  double ComputeComponenentAtPoint(
    unsigned int component, double* point, unsigned long timeStep, double time) override;

  ///@{
  /**
   * Set/get the constant value for the field.
   */
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);
  ///@}

protected:
  vtkCPConstantScalarFieldFunction();
  ~vtkCPConstantScalarFieldFunction() override;

private:
  vtkCPConstantScalarFieldFunction(const vtkCPConstantScalarFieldFunction&) = delete;
  void operator=(const vtkCPConstantScalarFieldFunction&) = delete;

  ///@{
  /**
   * The constant value for the scalar field.
   */
  double Constant;
};
//@}

#endif
