/*
 * convert integer to float
 */

void dvfloa(int n, int *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = (double) *x;
    x += incx;
    y += incy;
  }
  return;
}

void svfloa(int n, int *x, int incx, float *y, int incy)
{
  while (n--) {
    *y = (float) *x;
    x += incx;
    y += incy;
  }
  return;
}
