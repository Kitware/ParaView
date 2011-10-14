/*
 * vector allocation with arbitrary indexing
 */

#include <stdlib.h>
#include "vecerr.h"

double *dvector(int nl, int nh)
{
  double *v;

  v = (double*) malloc((nh-nl+1)*sizeof(double));
  if (!v) vecerr("dvector", vNOMEM);
  return v-nl;
}

float *svector(int nl, int nh)
{
  float *v;

  v = (float*) malloc((nh-nl+1)*sizeof(float));
  if (!v) vecerr("svector", vNOMEM);
  return v-nl;
}

int *ivector(int nl, int nh)
{
  int *v;

  v = (int*) malloc((nh-nl+1)*sizeof(int));
  if (!v) vecerr("ivector", vNOMEM);
  return v-nl;
}

void free_dvector(double *v, int nl)
{
  free(v+nl);
}

void free_svector(float *v, int nl)
{
  free(v+nl);
  return;
}

void free_ivector(int *v, int nl)
{
  free(v+nl);
  return;
}
