/*=========================================================================

  Program:   ParaView
  Module:    vtkCPLinearScalarFieldFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCPLinearScalarFieldFunction
 * @brief   Class for specifying scalars at points.
 *
 * Class for specifying a scalar field that is linear with respect to the
 * coordinate components as well as time.
*/

#ifndef vtkCPLinearScalarFieldFunction_h
#define vtkCPLinearScalarFieldFunction_h

#include "vtkCPScalarFieldFunction.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPLinearScalarFieldFunction
  : public vtkCPScalarFieldFunction
{
public:
  static vtkCPLinearScalarFieldFunction* New();
  vtkTypeMacro(vtkCPLinearScalarFieldFunction, vtkCPScalarFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute the field value at Point.
   */
  virtual double ComputeComponenentAtPoint(
    unsigned int component, double point[3], unsigned long timeStep, double time) override;

  //@{
  /**
   * Set/get the constant value for the field.
   */
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);
  //@}

  //@{
  /**
   * Set/get the XMultiplier for the field.
   */
  vtkSetMacro(XMultiplier, double);
  vtkGetMacro(XMultiplier, double);
  //@}

  //@{
  /**
   * Set/get the YMultiplier for the field.
   */
  vtkSetMacro(YMultiplier, double);
  vtkGetMacro(YMultiplier, double);
  //@}

  //@{
  /**
   * Set/get the ZMultiplier for the field.
   */
  vtkSetMacro(ZMultiplier, double);
  vtkGetMacro(ZMultiplier, double);
  //@}

  //@{
  /**
   * Set/get the TimeMultiplier for the field.
   */
  vtkSetMacro(TimeMultiplier, double);
  vtkGetMacro(TimeMultiplier, double);
  //@}

protected:
  vtkCPLinearScalarFieldFunction();
  ~vtkCPLinearScalarFieldFunction();

private:
  vtkCPLinearScalarFieldFunction(const vtkCPLinearScalarFieldFunction&) = delete;
  void operator=(const vtkCPLinearScalarFieldFunction&) = delete;

  /**
   * The constant value for the scalar field.
   */
  double Constant;

  /**
   * The XMultiplier for the scalar field.
   */
  double XMultiplier;

  /**
   * The YMultiplier for the scalar field.
   */
  double YMultiplier;

  /**
   * The ZMultiplier for the scalar field.
   */
  double ZMultiplier;

  //@{
  /**
   * The TimeMultiplier for the scalar field.
   */
  double TimeMultiplier;
};
//@}

#endif
