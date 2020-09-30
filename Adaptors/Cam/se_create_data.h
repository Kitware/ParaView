/*=========================================================================

  Program:   ParaView
  Module:    se_create_data.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 *
 */

#ifndef Adaptors_se_create_data_h
#define Adaptors_se_create_data_h

#include "vtkPVAdaptorsCamModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the Catalyst Coprocessor
 * @warning Make sure you pass a zero terminated string
 */
void VTKPVADAPTORSCAM_EXPORT se_coprocessorinitializewithpython_(const char* pythonScriptName);

/**
 * Creates grids for 2d and 3d cubed-spheres
 */
void VTKPVADAPTORSCAM_EXPORT se_create_grid_(int* ne, int* np, int* nlon, double* lonRad, int* nlat,
  double* latRad, int* nlev, double* lev, int* nCells2d, int* maxNcols, int* mpiRank);

/**
 * for timestep 0: creates the points and cells for the grids.
 * for all timesteps: copies data from the simulation to Catalyst.
 */
void VTKPVADAPTORSCAM_EXPORT se_add_chunk_(int* nstep, int* chunkSize, double* lonRad,
  double* latRad, double* psScalar, double* tScalar, double* uScalar, double* vScalar);

/**
 * Checks if Catalyst needs to coprocess data
 */
int VTKPVADAPTORSCAM_EXPORT se_requestdatadescription_(int* timeStep, double* time);

/**
 * Checks if the grids need to be created
 */
int VTKPVADAPTORSCAM_EXPORT se_needtocreategrid_();

/**
 * calls the coprocessor
 */
void VTKPVADAPTORSCAM_EXPORT se_coprocess_();

void VTKPVADAPTORSCAM_EXPORT se_catalyst_finalize_();

#ifdef __cplusplus
}
#endif
#endif
