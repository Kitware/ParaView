/*
 * xvvmvt(): z = (w - x) * y
 */

void dvvmvt(int n, double *w, int incw, double *x, int incx,
                   double *y, int incy, double *z, int incz)
{
  register int i;
  for(i = 0; i < n; i++) {
    *z = ( (*w) - (*x) ) * (*y);
    w += incw;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}
