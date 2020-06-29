/*=========================================================================

  Program:   ParaView
  Module:    fv_create_data.h

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

#ifndef Adaptors_fv_create_data_h
#define Adaptors_fv_create_data_h

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
 * Initializes the Catalyst Coprocessor.
 * @warning Make sure you pass a zero terminated string
 */
void VTKPVADAPTORSCAM_EXPORT fv_coprocessorinitializewithpython_(const char* pythonScriptName);

/**
 * Creates the Grids for 2D, 3D rectilinear and 2D, 3D spherical
 */
void VTKPVADAPTORSCAM_EXPORT fv_create_grid_(int* dim, double* lonCoord, double* latCoord,
  double* levCoord, int* nCells2d, int* maxNcols, int* myRank);

/**
 * for timestep 0: creates the points and cells for the grids.
 * for all timesteps: copies data from the simulation to Catalyst.
 */
void VTKPVADAPTORSCAM_EXPORT fv_add_chunk_(int* nstep, int* chunkSize, double* lonRad,
  double* latRad, double* psScalar, double* tScalar, double* uScalar, double* vScalar);

void VTKPVADAPTORSCAM_EXPORT fv_catalyst_finalize_();

/**
 * Checks if Catalyst needs to coprocess data
 */
int VTKPVADAPTORSCAM_EXPORT fv_requestdatadescription_(int* timeStep, double* time);

/**
 * Checks if Catalyst needs to coprocess data
 */
int VTKPVADAPTORSCAM_EXPORT fv_requestdatadescription_(int* timeStep, double* time);

/**
 * Checks if the grids need to be created
 */
int VTKPVADAPTORSCAM_EXPORT fv_needtocreategrid_();

/**
 * Calls the coprocessor
 */
void VTKPVADAPTORSCAM_EXPORT fv_coprocess_();

#ifdef __cplusplus
}
#endif
#endif
