/*
 * matrix allocation with contiguous storage
 */

#include <stdlib.h>
#include "vecerr.h"

double **dmatrix(int nrl, int nrh, int ncl, int nch)
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip;
  double *m1,
         **m2;

  m1 = (double*) malloc( size * sizeof(double) );
  if(!m1) vecerr("dmatrix-a", vNOMEM);

  m2 = (double**) malloc((nrh-nrl+1)*sizeof(double*));
  if(!m2) vecerr("dmatrix-b", vNOMEM);
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

float **smatrix(int nrl, int nrh, int ncl, int nch)
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip;
  float *m1,
         **m2;

  m1 = (float*) malloc( size * sizeof(float) );
  if(!m1) vecerr("dmatrix-a", vNOMEM);

  m2 = (float**) malloc((nrh-nrl+1)*sizeof(float*));
  if(!m2) vecerr("dmatrix-b", vNOMEM);
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

int **imatrix(int nrl, int nrh, int ncl, int nch)
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip;
  int *m1,
         **m2;

  m1 = (int*) malloc( size * sizeof(int) );
  if(!m1) vecerr("dmatrix-a", vNOMEM);

  m2 = (int**) malloc((nrh-nrl+1)*sizeof(int*));
  if(!m2) vecerr("dmatrix-b", vNOMEM);
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

void free_dmatrix(double **m2, int nrl, int ncl)
{
  double *m1 = *(m2 + nrl);

  free(m2 + nrl);
  free(m1 + ncl);

  return;
}

void free_smatrix(float **m2, int nrl, int ncl)
{
  float *m1 = *(m2 + nrl);

  free(m2 + nrl);
  free(m1 + ncl);

  return;
}


void free_imatrix(int **m2, int nrl, int ncl)
{
  int *m1 = *(m2 + nrl);

  free(m2 + nrl);
  free(m1 + ncl);

  return;
}
