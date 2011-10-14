/*
 * xvreal: y = REAL(x)
 */

#include "complex.h"

void zvreal(int n, zcomplex *x, int incx, double *y, int incy)
{
  while( n-- ) {
    *y = x->r;
    x += incx;
    y += incy;
  }
  return;
}
