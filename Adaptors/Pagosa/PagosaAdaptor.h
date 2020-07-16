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
 *
 * @param mx,my,mx            number of grid cells in each logical direction
 * @param x0,y0,z0            origin on this Processor Element (PE)
 * @param dx,dy,dz            grid cell size
 * @param my_id               this PE number
 * @param tot_pes             total number of PEs
 * @param nframe, nframelen   string and stringlength
 * @param version, versionlen string and stringlength
 */
void VTKPVADAPTORSPAGOSA_EXPORT setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0,
  double* y0, double* z0, double* dx, double* dy, double* dz, int* my_id, const int* tot_pes,
  char* nframe, int* nframelen, char* version, int* versionlen);

/**
 * Update the vtkNonOverlappingAMR headers for every frame (time step)
 * This holds ImageData which does not change size,
 * but frame, version, cycle and simulation time change with each frame
 *
 * @param nframe, nframelen   string and stringlength
 * @param version, versionlen string and stringlength
 * @param cycleNum            simulation cycle number
 * @param simTime             simulation time
 */
void VTKPVADAPTORSPAGOSA_EXPORT setgridgeometry_(
  char* nframe, int* nframelen, char* version, int* versionlen, int* cycleNum, double* simTime);

/**
 * Add field data in the first grid of nonoverlapping AMR
 *
 * @param fname, len    name, namelength
 * @param mx,my,mz      number of grid cells in each logical direction
 * @param my_id         this PE number
 * @param data          data
 * @param down_convert  if .true. convert data to unsigned character
 */
void VTKPVADAPTORSPAGOSA_EXPORT addgridfield_(
  char* fname, int* len, int* mx, int* my, int* mz, int* my_id, float* data, bool* down_convert);

/**
 * Initialize unstructured grid for ALL markers and allocate total size.
 *
 * @param nvp                 Total number of markers in simulation
 * @param nframe, nframelen   string and stringlength
 * @param version, versionlen string and stringlength
 * @param cycleNum            simulation cycle number
 * @param simTime             simulation time
 */
void VTKPVADAPTORSPAGOSA_EXPORT setmarkergeometry_(int* nvp, char* nframe, int* nframelen,
  char* version, int* versionlen, int* cycleNum, double* simTime);

/**
 * Add a field to the unstructured grid of markers
 *
 * @param numberAdded     number of markers added on this PE
 * @param xloc,yloc,zloc  coordinates for each marker added
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkergeometry_(
  int* numberAdded, float* xloc, float* yloc, float* zloc);

/*
 * Set a scalar field in the unstructured grid of markers
 *
 * @param fname, len   Name of data, len(fname)
 * @param numberAdded  number of markers added on this PE
 * @param data         Data by marker
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkerscalarfield_(
  char* fname, int* len, int* numberAdded, float* data);

/**
 * Set a 3 element vector field in the unstructured grid of markers
 *
 * @param fname, len   Name of data, len(fname)
 * @param numberAdded  number of markers added on this PE
 * @param data0,1,2    Data by marker
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkervectorfield_(
  char* fname, int* len, int* numberAdded, float* data0, float* data1, float* data2);

/**
 * Set a 6 element tensor field in the unstructured grid of markers
 *
 * @param fname, len   Name of data, len(fname)
 * @param numberAdded  number of markers added on this PE
 * @param data0,..,5   Data by marker
 */
void VTKPVADAPTORSPAGOSA_EXPORT addmarkertensorfield_(char* fname, int* len, int* numberAdded,
  float* data0, float* data1, float* data2, float* data3, float* data4, float* data5);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
