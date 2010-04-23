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
// .NAME vtkCPLinearScalarFieldFunction - Class for specifying scalars at points.
// .SECTION Description
// Class for specifying a scalar field that is linear with respect to the
// coordinate components as well as time.

#ifndef __vtkCPLinearScalarFieldFunction_h
#define __vtkCPLinearScalarFieldFunction_h

#include "vtkCPScalarFieldFunction.h"

class VTK_EXPORT vtkCPLinearScalarFieldFunction : public vtkCPScalarFieldFunction
{
public:
  static vtkCPLinearScalarFieldFunction * New();
  vtkTypeMacro(vtkCPLinearScalarFieldFunction, vtkCPScalarFieldFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compute the field value at Point.
  virtual double ComputeComponenentAtPoint(unsigned int component, double point[3],
                                           unsigned long timeStep, double time);

  // Description:
  // Set/get the constant value for the field.
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);

  // Description:
  // Set/get the XMultiplier for the field.
  vtkSetMacro(XMultiplier, double);
  vtkGetMacro(XMultiplier, double);

  // Description:
  // Set/get the YMultiplier for the field.
  vtkSetMacro(YMultiplier, double);
  vtkGetMacro(YMultiplier, double);

  // Description:
  // Set/get the ZMultiplier for the field.
  vtkSetMacro(ZMultiplier, double);
  vtkGetMacro(ZMultiplier, double);

  // Description:
  // Set/get the TimeMultiplier for the field.
  vtkSetMacro(TimeMultiplier, double);
  vtkGetMacro(TimeMultiplier, double);

protected:
  vtkCPLinearScalarFieldFunction();
  ~vtkCPLinearScalarFieldFunction();

private:
  vtkCPLinearScalarFieldFunction(const vtkCPLinearScalarFieldFunction&); // Not implemented
  void operator=(const vtkCPLinearScalarFieldFunction&); // Not implemented

  // Description:
  // The constant value for the scalar field.
  double Constant;

  // Description:
  // The XMultiplier for the scalar field.
  double XMultiplier;

  // Description:
  // The YMultiplier for the scalar field.
  double YMultiplier;

  // Description:
  // The ZMultiplier for the scalar field.
  double ZMultiplier;

  // Description:
  // The TimeMultiplier for the scalar field.
  double TimeMultiplier;
};

#endif
