/*
 * negative of a vector
 */

void dvneg(int n, double *x, int incx, double *y, int incy)
{
  while( n-- ) {
    *y = -(*x);
    x += incx;
    y += incy;
  }
  return;
}
