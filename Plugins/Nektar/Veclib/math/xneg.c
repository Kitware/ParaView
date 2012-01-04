/*
 * Change sign
 */

void dneg(int n, double *x, int incx)
{
  while (n--) {
    *x = -(*x);
    x += incx;
  }
}

void sneg(int n, float *x, int incx)
{
  while (n--) {
    *x = -(*x);
    x += incx;
  }
}

void dneg_(int *np, double *x, int *incxp)
{
  register int incx = *incxp,
               n    = *np;

  while( n-- ) {
    *x = -(*x);
    x += incx;
  }
  return;
}
