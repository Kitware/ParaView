/*=========================================================================

  Program:   ParaView
  Module:    CTHAdaptor.h

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

#ifndef Adaptors_CTHAdaptor_h
#define Adaptors_CTHAdaptor_h

#include "vtkPVAdaptorsCTHModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C" {
#endif

void VTKPVADAPTORSCTH_EXPORT handler(int sig);

/**
 * CTH INTERFACE.
 * C functions prototyped in pvspy_public.h.
 * called from within spy_fortran.c in cth
 *
 * @warning This function currently does not perform anything.
 */
void VTKPVADAPTORSCTH_EXPORT pvspy_qa(char* qadate, char* qatime, char* qajobn);

void VTKPVADAPTORSCTH_EXPORT pvspy_fil(char* filename, int len, char* runid, int* error);

int VTKPVADAPTORSCTH_EXPORT pvspy_vizcheck(int cycle, double ptime);

void VTKPVADAPTORSCTH_EXPORT pvspy_viz(int cycle, double ptime, double pdt, int, int);

void VTKPVADAPTORSCTH_EXPORT pvspy_fin();

void VTKPVADAPTORSCTH_EXPORT pvspy_stm(int igm, int n_blocks, int nmat, int max_mat,
  int NCFieldNames, int NMFieldNames, double* x0, double* x1, int max_level);

void VTKPVADAPTORSCTH_EXPORT pvspy_scf(int field_id, char* field_name, char* comment, int matid);

void VTKPVADAPTORSCTH_EXPORT pvspy_smf(int field_id, char* field_name, char* comment);

void VTKPVADAPTORSCTH_EXPORT pvspy_scx(int block_id, int field_id, int k, int j, double* istrip);

void VTKPVADAPTORSCTH_EXPORT pvspy_smx(
  int block_id, int field_id, int mat, int k, int j, double* istrip);

void VTKPVADAPTORSCTH_EXPORT pvspy_stb(int block_id, int Nx, int Ny, int Nz, double* x, double* y,
  double* z, int allocated, int active, int level);

void VTKPVADAPTORSCTH_EXPORT pvspy_sta(int block_id, int allocated, int active, int level,
  int max_level, int bxbot, int bxtop, int bybot, int bytop, int bzbot, int bztop, int npxma11,
  int npxma21, int npxma12, int npxma22, int npyma11, int npyma21, int npyma12, int npyma22,
  int npzma11, int npzma21, int npzma12, int npzma22, int npxpa11, int npxpa21, int npxpa12,
  int npxpa22, int npypa11, int npypa21, int npypa12, int npypa22, int npzpa11, int npzpa21,
  int npzpa12, int npzpa22, int nbxma11, int nbxma21, int nbxma12, int nbxma22, int nbyma11,
  int nbyma21, int nbyma12, int nbyma22, int nbzma11, int nbzma21, int nbzma12, int nbzma22,
  int nbxpa11, int nbxpa21, int nbxpa12, int nbxpa22, int nbypa11, int nbypa21, int nbypa12,
  int nbypa22, int nbzpa11, int nbzpa21, int nbzpa12, int nbzpa22);

/**
 * @warning This function does not do anything
 */
void pvspy_trc(
  int num, double* xt, double* yt, double* zt, int* id, int* lt, int* it, int* jt, int* kt);

#ifdef __cplusplus
}
#endif
#endif
