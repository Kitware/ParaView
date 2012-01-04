/*
 * xvvtvm():   z = w * x - y
 */

void dvvtvm(int n, double *w, int incw, double *x, int incx,
             double *y, int incy, double *z, int incz)
{
  while( n-- ) {
    *z = (*w) * (*x) - (*y);
    w += incw;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}
