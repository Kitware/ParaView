/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionOfXList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkFunctionList
 * @brief   Function implementations of possible FunctionOfX.
 *
 */

#ifndef vtkFunctionOfXList_h
#define vtkFunctionOfXList_h

#include "vtkSetGet.h"

#include <cmath>

double VTK_FUNC_X(double x);
double VTK_FUNC_X2(double x);
double VTK_FUNC_NXLOGX(double x);
double VTK_FUNC_1_X(double x);
double VTK_FUNC_NULL(double vtkNotUsed(x));
double VTK_FUNC_1(double vtkNotUsed(x));

#endif
