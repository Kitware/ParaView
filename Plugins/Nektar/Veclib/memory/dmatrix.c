/*
 * Double-precision matrix - contiguous elements
 */

#include <stdio.h>
#include "util.h"

double **dmatrix(int nrl, int nrh, int ncl, int nch)
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip;
  double *m1,
         **m2;

  m1 = (double *) malloc( size * sizeof(double) );
  if(!m1) error_handler("allocation failure 1 in dmatrix()");

  m2 = (double **) malloc((unsigned)(nrh-nrl+1)*sizeof(double*));
  if(!m2) error_handler("allocation failure 2 in dmatrix()");
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

double **dsubmatrix(double **a, int oldrl, int oldrh, int oldcl, int oldch,
                                int newrl, int newcl)
{
  int i,j;
  double **m;

  m=(double **) malloc((unsigned) (oldrh-oldrl+1)*sizeof(double*));
  if (!m) error_handler("allocation failure in submatrix()");
  m -= newrl;

  for(i=oldrl,j=newrl;i<=oldrh;i++,j++) m[j]=a[i]+oldcl-newcl;

  return m;
}

double **convert_dmatrix(double *a, int nrl, int nrh, int ncl, int nch)
{
  int    i, j, nrow, ncol;
  double **m;

  nrow = nrh - nrl + 1;
  ncol = nch - ncl + 1;
  m    = (double**) malloc((unsigned) nrow * sizeof(double*));
  if (!m) error_handler("allocation failure in convert_dmatrix()");
  m   -= nrl;

  for(i = 0, j = nrl; i < nrow; i++, j++)
    m[j] = a + ncol * i - ncl;

  return m;
}

void free_dmatrix(double **m2, int nrl, int nrh, int ncl, int nch)
{
  double *m1 = *(m2 + nrl);

  free((char*) (m2 + nrl));
  free((char*) (m1 + ncl));

  return;
}

void free_dsubmatrix(double **b, int nrl, int nrh, int ncl, int nch)
{
  free((char*) (b+nrl));
  return;
}

void free_convert_dmatrix(double **b, int nrl, int nrh, int ncl, int nch)
{
  free((char*) (b+nrl));
}
