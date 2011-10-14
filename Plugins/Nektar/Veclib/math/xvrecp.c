/*
 * xvrecp: y = 1 / x
 */

void dvrecp(int n, double *x, int incx, double *y, int incy)
{
  register double one = 1.0;

  while (n--) {
    *y = one / *x;
    x += incx;
    y += incy;
  }
  return;
}

/* FORTRAN versions */

void svrecp_(int *np, float *x, int *incxp, float *y, int *incyp)
{
  register int   n    = *np,
                 incx = *incxp,
                 incy = *incyp;
  register float one  = 1.0;

  while (n--) {
    *y = one / *x;
    x += incx;
    y += incy;
  }
  return;
}
