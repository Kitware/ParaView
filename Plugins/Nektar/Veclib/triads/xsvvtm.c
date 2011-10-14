/*
 * xsvvtm: z = alpha - x * y
 */

void dsvvtm (int n, double alpha,
       double *x, int incx, double *y, int incy, double *z, int incz)
{
  while (n--) {
    *z = alpha - (*x) * (*y);
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}
