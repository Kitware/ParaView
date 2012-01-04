/*
 *  Scalar times a vector
 */

void dsmul (int n, double alpha, double *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = alpha * (*x);
    x += incx;
    y += incy;
  }
  return;
}
