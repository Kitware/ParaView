/*=========================================================================

  Program:   ParaView
  Module:    PhastaAdaptor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief Simulation code.
 *
 * Phasta is a simulation code. It is a closed source code. A copy of the Physics
 * Manual is at http://permalink.lanl.gov/object/tr?what=info:lanl-repo/lareport/LA-14425-M
 */

#ifndef vtkPhastaAdaptor_h
#define vtkPhastaAdaptor_h

#include "vtkPVAdaptorsPhastaModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C" {
#endif

void VTKPVADAPTORSPHASTA_EXPORT createpointsandallocatecells(
  int* numPoints, double* coordsArray, int* numCells);

void VTKPVADAPTORSPHASTA_EXPORT insertblockofcells(
  int* numCellsInBlock, int* numPointsPerCell, int* cellConnectivity);

void VTKPVADAPTORSPHASTA_EXPORT addfields(
  int* nshg, int* ndof, double* dofArray, int* compressibleFlow);

#ifdef __cplusplus
}
#endif
#endif
