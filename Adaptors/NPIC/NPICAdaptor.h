/*=========================================================================

  Program:   ParaView
  Module:    NPICAdaptor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef Adaptors_NPICAdaptor_h
#define Adaptors_NPICAdaptor_h

#include "vtkPVAdaptorsNPICModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C" {
#endif

void VTKPVADAPTORSNPIC_EXPORT createstructuredgrid_(
  int* myid, int* xdim, int* ystart, int* ystop, double* xspc, double* yspc);

void VTKPVADAPTORSNPIC_EXPORT add_scalar_(char* fname, int* len, double* data, int* size);

void VTKPVADAPTORSNPIC_EXPORT add_vector_(
  char* fname, int* len, double* data0, double* data1, double* data2, int* size);

void VTKPVADAPTORSNPIC_EXPORT add_pressure_(int* index, double* data, int* size);

#ifdef __cplusplus
}
#endif
#endif
