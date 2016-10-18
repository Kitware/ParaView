/*=========================================================================

  Program:   ParaView
  Module:    CAdaptorAPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef CAdaptorAPI_h
#define CAdaptorAPI_h

#include "vtkPVCatalystModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C" {
#endif

// call at the start of the simulation
void VTKPVCATALYST_EXPORT coprocessorinitialize();

// call at the end of the simulation
void VTKPVCATALYST_EXPORT coprocessorfinalize();

// this is the function that determines whether or not there
// is anything to coprocess this time step
void VTKPVCATALYST_EXPORT requestdatadescription(
  int* timeStep, double* time, int* coprocessThisTimeStep);

// this function sets needgrid to 1 if it does not have a copy of the grid
// it sets needgrid to 0 if it does have a copy of the grid but does not
// check if the grid is modified or needs to be updated
void VTKPVCATALYST_EXPORT needtocreategrid(int* needGrid);

// do the actual coprocessing.  it is assumed that the vtkCPDataDescription
// has been filled in elsewhere.
void VTKPVCATALYST_EXPORT coprocess();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
// VTK-HeaderTest-Exclude: CAdaptorAPI.h
