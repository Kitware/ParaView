/*
 * vector times vector plus vector
 */

void dvvtvp(int n, double *w, int incw, double *x, int incx,
             double *y, int incy, double *z, int incz)
{
  while( n-- ) {
    *z = (*w) * (*x) + (*y);
    w += incw;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}

void dvvtvp_(int *np, double *w, int *iwp, double *x, int *ixp,
                      double *y, int *iyp, double *z, int *izp)
{
  register int n    = *np,
               incw = *iwp,
               incx = *ixp,
               incy = *iyp,
               incz = *izp;
  while( n-- ) {
    *z = (*w) * (*x) + (*y);
    w += incw;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}
