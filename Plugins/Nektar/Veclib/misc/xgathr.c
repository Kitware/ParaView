/*
 *  vector gather
 */

#include "complex.h"

void dgathr(int n, double *x, int *y, double *z)
{
  while (n--) *z++ = *(x + *y++);
  return;
}

void sgathr(int n, float *x, int *y, float *z)
{
  while (n--) *z++ = *(x + *y++);
  return;
}

void zgathr(int n, zcomplex *x, int *y, zcomplex *z)
{
  while (n--) { z->r = (x + *y)->r; z++->i = (x + *y++)->i; }
  return;
}
