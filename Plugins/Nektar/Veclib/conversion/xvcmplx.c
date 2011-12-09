/*
 * xvcmplx: z = CMPLX(x,y)
 */

#include "complex.h"

void zvcmplx(int n, double *x, int incx, double *y, int incy,
            zcomplex *z, int incz)
{
  while (n--) {
    z->r = (*x); z->i = (*y);
    x   += incx;
    y   += incy;
    z   += incz;
  }
  return;
}
