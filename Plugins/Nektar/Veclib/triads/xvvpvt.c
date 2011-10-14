/*
 * dvvpvt(): z = (w + x) * y
 */

void dvvpvt(int n, double *w, int incw, double *x, int incx,
                   double *y, int incy, double *z, int incz)
{
  while( n-- ) {
    (*z) = (*y) * ( (*w) + (*x) );
    w   += incw;
    x   += incx;
    y   += incy;
    z   += incz;
  }
  return;
}
