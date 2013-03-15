/*=========================================================================

  Program:   ParaView
  Module:    CPythonAdaptorAPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __CAdaptorAPI_h
#define __CAdaptorAPI_h

#include "vtkPVPythonCatalystModule.h"

#include "CAdaptorAPI.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names. This extends CAdaptorAPI.h to add a new
// initialization function that take a python script.

#ifdef __cplusplus
extern "C" {
#endif

  // call at the start of the simulation
  void VTKPVPYTHONCATALYST_EXPORT coprocessorinitializewithpython(
    char* pythonFileName, int* pythonFileNameLength);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
// VTK-HeaderTest-Exclude: CPythonAdaptorAPI.h
