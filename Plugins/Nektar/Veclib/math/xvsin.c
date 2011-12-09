/*
 * element-wise sine
 */

#include <math.h>

void dvsin(int n, double *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = sin (*x);
    x += incx;
    y += incy;
  }
  return;
}
