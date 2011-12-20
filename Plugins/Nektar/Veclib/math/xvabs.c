/*
 * xvabs: y = |x|
 */

#include <math.h>

void dvabs(int n, double *x, int incx, double *y, int incy)
{
  while( n-- ) {
    *y = fabs( *x );
    x += incx;
    y += incy;
  }
  return;
}
