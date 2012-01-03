/*
 *  assign a scalar to a vector
 */

void dfill (int n, double alpha, double *x, int incx)
{
  while (n--) {
    *x = alpha;
    x += incx;
  }
  return;
}

void dfill_ (int *np, double *alphap, double *x, int *incxp)
{
  dfill (*np, *alphap, x, *incxp);
  return;
}

void ifill (int n, int alpha, int *x, int incx)
{
  while (n--) {
    *x = alpha;
    x += incx;
  }
  return;
}
