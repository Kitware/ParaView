/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2004 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 *
 * Author: Brian Wylie bnwylie@sandia.gov */

#ifndef SPCTH_INTERFACE_H
#define SPCTH_INTERFACE_H

#include <stdio.h>
#include "spcthConfig.h"

/* Inhibit C++ name-mangling for functions but not for system calls. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***********************************************************************
 * Dummy structure that is only wrapper around the Spy code, so that the 
 * code that uses spcth reader is not poluted with the Spy code.
 ***********************************************************************/
struct _SpyFile;
typedef struct _SPCTH
  {
  struct _SpyFile* Spy;
  } SPCTH;

/************************************************************************
 * Initialize and finalize SPCTH interface. Initialize creates the
 * structure and finalize destroys it.
 ************************************************************************/
SPCTH_EXPORT SPCTH* spcth_initialize();
SPCTH_EXPORT void spcth_finalize(SPCTH* spcth);

/************************************************************************
 * Open up the spy file and fill in the data structures to be used later
 ************************************************************************/
SPCTH_EXPORT int spcth_openSpyFile(SPCTH* spcth, const char *filename);

/*******************************
 * Time step accessors
 *******************************/
SPCTH_EXPORT int spcth_getNumTimeSteps(SPCTH* spcth);
SPCTH_EXPORT double spcth_getTimeStepValue(SPCTH* spcth, int index);
SPCTH_EXPORT int spcth_setTimeStep(SPCTH* spcth, double time_val);

/*******************************
 * Check if the dataset is AMR
 *******************************/
SPCTH_EXPORT int spcth_isAMR(SPCTH* spcth);

/********************************************************************
 * Note: You must call openSpyFile && setTimeStep before
 *       calling any of the functions below
 ********************************************************************/

/*******************************
 * Data Block accessors
 *******************************/

//! How many data blocks are in the file
SPCTH_EXPORT int spcth_getNumberOfDataBlocksForCurrentTime(SPCTH* spcth);

//! What are the dimensions of the particular data block
SPCTH_EXPORT void spcth_getDataBlockDimensions(SPCTH* spcth, int block_index, int *x, int *y, int *z);

//! For AMR dataset, what is the level of the particular block
SPCTH_EXPORT int spcth_getDataBlockLevel(SPCTH* spcth, int block_index);

//! For rectilinear grid, get spacing vectors for the particular block
SPCTH_EXPORT int spcth_getDataBlockVectors(SPCTH* spcth, int block_index,
  double *vx, double *vy, double *vz);

//! Get bounds of a particular block
SPCTH_EXPORT int spcth_getDataBlockBounds(SPCTH* spcth, int block_index, double *bounds);

/*******************************
 * Cell Field accessors
 *******************************/
SPCTH_EXPORT int spcth_getNumberOfCellFields(SPCTH* spcth);
SPCTH_EXPORT const char* spcth_getCellFieldName(SPCTH* spcth, int index);
SPCTH_EXPORT const char* spcth_getCellFieldDescription(SPCTH* spcth, int index);
SPCTH_EXPORT int spcth_getCellFieldData(SPCTH* spcth, int block_index, int field_index, double *data);

#ifdef __cplusplus
}
#endif

#endif //SPCTH_INTERFACE_H
