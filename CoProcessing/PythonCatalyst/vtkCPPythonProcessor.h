/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Simply provided for backwards compatibility. Use vtkCPProcessor instead.

#ifndef vtkCPPythonProcessor_h
#define vtkCPPythonProcessor_h

#warning Please use vtkCPProcessor directly. vtkCPPythonProcessor is \
         no longer available/needed.

#include "vtkCPProcessor.h"
#define vtkCPPythonProcessor vtkCPProcessor

#endif
// VTK-HeaderTest-Exclude: vtkCPPythonProcessor.h
