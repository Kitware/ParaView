/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile FortranAdaptorAPI.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef FortranAdaptorAPI_h
#define FortranAdaptorAPI_h

#include "CPUseFortran.h"

#ifdef COPROCESSOR_MANGLE_FORTRAN
#include "FortranAdaptorAPIMangling.h"
#endif

#ifdef __cplusplus
class vtkCPDataDescription;
class vtkDataSet;

// This code is meant to be used as an API for Fortran and C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names. Note though that the function names
// will only get mangled if a Fortran compiler is available and
// defined in the ParaView build. CPUseFortran.h is configured
// and put in the build tree and is used to define
// COPROCESSOR_MANGLE_FORTRAN if the function names should get
// mangled for Fortran linking. Note that Fortran compilers
// ignore capitalization so if coding styles dictate that the
// function/subroutine names be capitalized it won't affect
// linking.

// This namespace may be able to also be used with the extern "C"
// functions.  It worked with GCC V4.4.5 but I'm not sure it will
// work with all compilers and linkers so I'm not going to do it.
namespace ParaViewCoProcessing
{
  // function to return the singleton/static vtkCPDataDescription object
  // that contains the grid and fields stuff
  vtkCPDataDescription* GetCoProcessorData();

  // Clear all of the field data from the grids.
  void ClearFieldDataFromGrid(vtkDataSet* grid);

  // For Fortran strings we can't figure out from C/C++ code
  // how long they are.  This function returns true if successful,
  // false otherwise (e.g. if CStringMaxLength <= FortranStringLength).
  bool ConvertFortranStringToCString(
    char* fortranString, int fortranStringLength,
    char* cString, int cStringMaxLength);
}

extern "C" {
#endif

// for now assume that the coprocessor is run through a python script
void coprocessorinitialize(char* pythonFileName,
                           int* pythonFileNameLength);

// call at the end of the simulation
void coprocessorfinalize();

// this is the function that determines whether or not there
// is anything to coprocess this time step
void requestdatadescription(int* timeStep, double* time,
                            int* coprocessThisTimeStep);

// this function sets needgrid to 1 if it does not have a copy of the grid
// it sets needgrid to 0 if it does have a copy of the grid but does not
// check if the grid is modified or needs to be updated
void needtocreategrid(int* needGrid);

// do the actual coprocessing.  it is assumed that the vtkCPDataDescription
// has been filled in elsewhere.
void coprocess();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
