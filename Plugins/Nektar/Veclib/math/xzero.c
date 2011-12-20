/*
 * Zero a vector
 */

#include <string.h>

void dzero(int n, double *x, const int incx)
{
  register double zero = 0.;

  if (incx == 1)
    memset (x, '\0', n * sizeof(double));
  else
    while (n--) {
      *x = zero;
      x += incx;
    }

  return;
}

void szero(int n, float *x, const int incx)
{
  register float zero = 0.0;

  if (incx == 1)
    memset (x, '\0', n * sizeof(float));
  else
    while (n--) {
      *x = zero;
      x += incx;
    }

  return;
}

void izero(int n, int *x, const int incx)
{
  register int zero = 0;

  if (incx == 1)
    memset (x, '\0', n * sizeof(int));
  else
    while (n--) {
      *x = zero;
      x += incx;
    }

  return;
}

/* FORTRAN versions */

void szero_(int *np, float *x, int *incxp)
{
  register int   n    = *np,
                 incx = *incxp;
  register float zero = 0.0;

  while (n--) {
    *x = zero;
    x += incx;
  }
  return;
}

void izero_(int *np, int *x, int *incxp)
{
  register int zero = 0,
               incx = *incxp,
               n    = *np;
  while (n--) {
    *x = zero;
    x += incx;
  }
  return;
}
