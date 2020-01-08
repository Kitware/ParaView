/*=========================================================================

  Program:   ParaView
  Module:    FortranPythonAdaptorAPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef FortranAdaptorAPI_h
#define FortranAdaptorAPI_h

#include "FortranPythonAdaptorAPIMangling.h"
#include "vtkPVPythonCatalystModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#include "CPythonAdaptorAPI.h"

#endif
// VTK-HeaderTest-Exclude: FortranPythonAdaptorAPI.h
