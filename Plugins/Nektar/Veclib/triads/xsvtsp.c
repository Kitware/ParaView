/*
 * xsvtsp: y = alpha * x + beta
 */

void dsvtsp (int n, double alpha, double beta,
       double *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = (alpha * (*x)) + beta;
    x += incx;
    y += incy;
  }
  return;
}
