/*=========================================================================

  Program:   ParaView
  Module:    PagosaAdaptor.h

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
 * Pagosa is a simulation code. It is a closed source code. A copy of the Physics
 * Manual is at http://permalink.lanl.gov/object/tr?what=info:lanl-repo/lareport/LA-14425-M
 */

#ifndef vtkPagosaAdaptor_h
#define vtkPagosaAdaptor_h

#include "vtkPVAdaptorsPagosaModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define the data structures to hold the in situ output VTK data
 * vtkNonOverlappingAMR is required for using the MaterialInterface filter
 * vtkUnstructuredGrid will hold the data currently written to .cosmo files.
 */
void VTKPVADAPTORSPAGOSA_EXPORT setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0,
  double* y0, double* z0, double* dx, double* dy, double* dz, unsigned int* my_id,
  const int* tot_pes);

/**
 * Set a field in the first grid of nonoverlapping AMR
 */
void VTKPVADAPTORSPAGOSA_EXPORT setcoprocessorfield_(char* fname, int* len, int* mx, int* my,
  int* mz, unsigned int* my_id, float* data, bool* down_convert);

/**
 * Initialize unstructured grid for markers and allocate size.
 *
 * @note my_id is not used
 */
void VTKPVADAPTORSPAGOSA_EXPORT setmarkergeometry_(int* nvp, int* my_id = nullptr);

/**
 * Add a field to the unstructured grid of markers
 *
 * @param numberOfParticles Particles in added field
 * @param xloc, yloc, zloc Location
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkergeometry_(
  int* numberOfParticles, float* xloc, float* yloc, float* zloc);

/*
 * Set a scalar field in the unstructured grid of markers
 *
 * @param fname Name of data
 * @param len Length of data name
 * @param numberOfParticles Particles in field
 * @param data Data by particle
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkerscalarfield_(
  char* fname, int* len, int* numberOfParticles, float* data);

/**
 * Set a vector field in the unstructured grid of markers
 *
 * @param fname Name of data
 * @param len Length of data name
 * @param numberOfParticles Particles in field
 * @param data0, data1, data2 Data by particle
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkervectorfield_(
  char* fname, int* len, int* numberOfParticles, float* data0, float* data1, float* data2);

/**
 * Set a 6 element tensor field in the unstructured grid of markers
 *
 * @param fname Name of data
 * @param len Length of data name
 * @param numberOfParticles Particles in field
 * @param data0, data1, data2, data3, data4, data5 Data by particle
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkertensorfield_(char* fname, int* len, int* numberOfParticles,
  float* data0, float* data1, float* data2, float* data3, float* data4, float* data5);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
