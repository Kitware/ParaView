/*
 * vector absolute maximum
 */

#include <math.h>
#define max(a,b) ((a) > (b)) ? (a) : (b)

void dvamax(int n, double *x, int incx, double *y, int incy,
       double *z, int incz)
{
  register double absx, absy;

  while( n-- ) {
    absx = fabs( *x );
    absy = fabs( *y );
    *z   = max( absx, absy );
    x   += incx;
    y   += incy;
    z   += incz;
  }
  return;
}
