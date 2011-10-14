/*
 * element-wise cosine
 */

#include <math.h>

void dvcos(int n, double *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = cos (*x);
    x += incx;
    y += incy;
  }
  return;
}
