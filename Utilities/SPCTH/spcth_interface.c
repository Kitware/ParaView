/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2004 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */
/* Author: Brian Wylie bnwylie@sandia.gov */

#include <stdio.h>
#include <stdlib.h>

#include "spcth_interface.h"
#include "spy_file.h"

/********************************************************************/
SPCTH* spcth_initialize()
{
  SPCTH* sp = (SPCTH*) malloc(sizeof(SPCTH));
  sp->Spy = spy_initialize();
  return sp;
}

/********************************************************************/
void spcth_finalize(SPCTH* spcth)
{
  spy_finalize(spcth->Spy);
  free(spcth);
}

/********************************************************************
 * Open up the spy file and fill in data structures to be used later
 ********************************************************************/
int spcth_openSpyFile(SPCTH* spcth, const char *filename)
{
  return spy_open_file_for_input(spcth->Spy, filename);
}

/********************************************************************
 * Time step accessors
 ********************************************************************/
int spcth_getNumTimeSteps(SPCTH* spcth) {

  SpyFileDump *dmp;

  /* Count up number of time steps */
  int count = 0;
  for(dmp=spcth->Spy->FirstDump; dmp!=NULL ; dmp=dmp->next) count++;

  return count;
}

/********************************************************************/
double spcth_getTimeStepValue(SPCTH* spcth, int index) {

  SpyFileDump *dmp;
  int i;

  /* Run through dumps until you get the right index */
  for(dmp=spcth->Spy->FirstDump, i=0; dmp!=NULL && i<index ; dmp=dmp->next, ++i);

  return dmp->Time;
}

/********************************************************************/
int spcth_setTimeStep(SPCTH* spcth, double time_val) {

  return spy_file_in(spcth->Spy, time_val);
}

/********************************************************************/
int spcth_isAMR(SPCTH* spcth)
{
  return spcth_getNumberOfDataBlocksForCurrentTime(spcth) != 1;
}

/********************************************************************
 * Data Block accessors
 ********************************************************************/
int spcth_getNumberOfDataBlocksForCurrentTime(SPCTH* spcth) {

  Structured_Block_Data *blk;
  int count=0;

  /* Count up the block list */
  for (blk=spy_FirstBlock(spcth->Spy); blk!=NULL; blk=spy_NextBlock(spcth->Spy), ++count );

  return count;
}

/********************************************************************/
void spcth_getDataBlockDimensions(SPCTH* spcth, int block_index, int *x, int *y, int *z) {

  Structured_Block_Data *blk;
  int count=0;

  /* Step through blocks until the right index */
  for (blk=spy_FirstBlock(spcth->Spy); blk!=NULL && count<block_index; blk=spy_NextBlock(spcth->Spy), ++count );

  *x = blk->Nx;
  *y = blk->Ny;
  *z = blk->Nz;
}

/********************************************************************/
int spcth_getDataBlockLevel(SPCTH* spcth, int block_index) {

  Structured_Block_Data *blk;
  int count=0;

  /* Step through blocks until the right index */
  for (blk=spy_FirstBlock(spcth->Spy); blk!=NULL && count<block_index; blk=spy_NextBlock(spcth->Spy), ++count );

  return blk->level;
}

/********************************************************************/
int spcth_getDataBlockVectors(SPCTH* spcth, int block_index,
  double **vx, double **vy, double **vz)
{
  Structured_Block_Data *blk;
  int count=0;

  /* Sanity check */
  if (!vx || !vy || !vz) return 0;

  /* Step through blocks until the right index */
  for (blk=spy_FirstBlock(spcth->Spy);
    blk!=NULL && count<block_index;
    blk=spy_NextBlock(spcth->Spy), ++count )
    {
    }

  *vx = blk->x;
  *vy = blk->y;
  *vz = blk->z;

  /* Successfully populated bounds */
  return 1;
}
/********************************************************************/
int spcth_getDataBlockBounds(SPCTH* spcth, int block_index, double *bounds) {

  Structured_Block_Data *blk;
  int count=0;

  /* Sanity check */
  if (bounds == NULL) return 0;

  /* Step through blocks until the right index */
  for (blk=spy_FirstBlock(spcth->Spy);
    blk!=NULL && count<block_index;
    blk=spy_NextBlock(spcth->Spy), ++count )
    {
    }

  bounds[0] = blk->x[0];
  bounds[1] = blk->x[blk->Nx];
  bounds[2] = blk->y[0];
  bounds[3] = blk->y[blk->Ny];
  bounds[4] = blk->z[0];
  bounds[5] = blk->z[blk->Nz];

  /* Successfully populated bounds */
  return 1;
}

/********************************************************************
 * Cell Field accessors
 ********************************************************************/

/********************************************************************
 * All of the fields are really indexed indirectly
 * In fact there is some crazy thing that happens
 * with Material fields. A material field will have
 * an index of like '201', well you divide that by
 * 100 to get material field '2' substract 1 to get
 * the right c-array index '1' and then use the remainer
 * for the second index into the material data arrays.
 * And no I'm not joking... but the end user of this
 * API should never have to know any of that...
 ********************************************************************/

/********************************************************************
 * START Internal Functions (not part of API)
 ********************************************************************/

/********************************************************************
 * This function looks up the SPCTH index
 ********************************************************************/
static int getFieldSPCTHIndex(SPCTH* spcth, int index) 
{

  /* Sanity Check */
  if (index >= spcth->Spy->NumSavedVariables)
    {
    fprintf(stderr,"Failed Sanity Check! - Trying to get index lookup past end of array\n");
    exit(1);
    }
  return spcth->Spy->SavedVariables[index];
}

/********************************************************************/
static int isMaterialIndex(int index)
{
  if (index > 100) 
    return 1;
  else
    return 0;
}

/********************************************************************/
static int getMaterialIndex(int index)
{
  return index/100 - 1;
}

/********************************************************************/
static int getMaterialSubIndex(int index)
{
  return index%100 - 1;
}
/********************************************************************
 * END Interal Functions (not part of API)
 ********************************************************************/


/********************************************************************
 * Get the number of SAVED cell scalar fields
 ********************************************************************/
int spcth_getNumberOfCellFields(SPCTH* spcth) 
{
  return spcth->Spy->NumSavedVariables;
}

/********************************************************************
 * Get the name of the cell scalar field
 ********************************************************************/
const char* spcth_getCellFieldName(SPCTH* spcth, int index) 
{
  static char buffer[80]; 
  int mat_index;
  int spcth_index = getFieldSPCTHIndex(spcth, index);

  if (isMaterialIndex(spcth_index)) 
    {
    /* This logic is a long story. There are 5 'categories' */
    /* of materials. The MField_id array contains those     */
    /* categories. The index is based on hundreds... so     */
    /* 100 = VOLM, 200 = M, 300 = PM, etc...                */
    /* We append the material number to the category.       */
    /* Example VOLM - 1                                     */
    mat_index = getMaterialIndex(spcth_index);
    sprintf(buffer, "%s - %d", spcth->Spy->stm_data.MField_id[mat_index], getMaterialSubIndex(spcth_index));
    return buffer;
    }
  else
    {
    return spcth->Spy->stm_data.CField_id[spcth_index];
    }
}

/********************************************************************
 * Get the description of the cell scalar field
 ********************************************************************/
const char* spcth_getCellFieldDescription(SPCTH* spcth, int index) 
{
  static char buffer[80]; 
  int mat_index;
  int spcth_index = getFieldSPCTHIndex(spcth, index);

  if (isMaterialIndex(spcth_index)) 
    {
    /* This logic is a long story. There are 5 'categories' */
    /* of materials. The MField_id array contains those     */
    /* categories. The index is based on hundreds... so     */
    /* 100 = VOLM, 200 = M, 300 = PM, etc...                */
    /* We append the material number to the category.       */
    /* Example VOLM - 1                                     */
    mat_index = getMaterialIndex(spcth_index);
    sprintf(buffer, "%s - %d", spcth->Spy->stm_data.MField_comment[mat_index], getMaterialSubIndex(spcth_index));
    return buffer;
    }
  else
    {
    return spcth->Spy->stm_data.CField_comment[spcth_index];
    }
}


/********************************************************************
 * Get the scalar field data for a particular block
 ********************************************************************/
int spcth_getCellFieldData(SPCTH* spcth, int block_index, int field_index, double *data)
{
  Structured_Block_Data *blk;
  int count=0;
  int i,j,k,x,y,z;
  double ***spcth_data;
  double *data_ptr = data;
  int spcth_index;

  /* Step through blocks until the right index */
  for (blk=spy_FirstBlock(spcth->Spy);
    blk!=NULL && count<block_index;
    blk=spy_NextBlock(spcth->Spy), ++count )
    {
    }

  /* Sanity check  */
  if (!blk->allocated)
    {
    return 0;
    }

  /* See if data block is valid */
  if (blk==NULL || blk->CField==NULL)
    {
    data = NULL;
    }

  /* Sanity check 2 */
  if (data == NULL)
    {
    return 0;
    }

  /*
   * This method reads entire field for all the blocks, but there is a check
   * inside the method so that it does it only once, even though it is called
   * for every single block.
   */
  spy_read_variable_data(spcth->Spy, field_index);

  /* Get a handle to the data */
  /* The data may be either cell or material data */
  /* depending on the spcth index for this field  */
  spcth_index = getFieldSPCTHIndex(spcth, field_index);
  spcth_data = spy_GetField(blk, spcth_index);

  /* Okay all this memory shuffling is because SPCTH is storing the  */
  /* data in the following format [field_index][z][y][x] */
  spcth_getDataBlockDimensions(spcth, block_index, &x, &y, &z);
  for(i=0; i<z; ++i)
    {
    for(j=0; j<y; ++j)
      {
      for(k=0; k<x; ++k) 
        {
        *data_ptr++ = spcth_data[i][j][k];
        }
      }
    }

  /* Sucessfully filled data array */
  return 1;
}

