/*
 * vector divide: z = x / y
 */

void dvdiv(int n, double *x, int incx, double *y, int incy,
      double *z, int incz)
{
  while( n-- ) {
    *z = *x / *y;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}
