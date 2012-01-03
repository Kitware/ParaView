/*
 * element-wise square root
 */

#include <math.h>

void dvsqrt(int n, double *x, int incx, double *y, int incy)
{
  while (n--) {
    *y  = sqrt( *x );
    x  += incx;
    y  += incy;
  }
  return;
}

/* FORTRAN versions */

void svsqrt_(int *np, float *x, int *incxp, float *y, int *incyp)
{
  register int n    = *np,
               incx = *incxp,
               incy = *incyp;
  while (n--) {
    *y  = (float) sqrt( (double) *x );
    x  += incx;
    y  += incy;
  }
  return;
}
