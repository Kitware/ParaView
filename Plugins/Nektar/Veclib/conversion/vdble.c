/*
 * Convert single to double precision
 */

void vdble(int n, float *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = (double) *x;
    x += incx;
    y += incy;
  }

  return;
}
