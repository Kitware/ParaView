/*
 * Vector sum
 */

double dsum(int n, double *x, int incx)
{
  register double sum = 0.0;

  while( n-- ) {
    sum += *x;
    x   += incx;
  }
  return sum;
}

int isum(int n, int *x, int incx)
{
  register int sum = 0;

  while (n--) {
    sum += *x;
    x   += incx;
  }

  return sum;
}

/* FORTRAN versions */

float ssum_(int *np, float *x, int *incxp)
{
  register int   n    = *np,
                 incx = *incxp;
  register float sum  = 0.0;

  while (n--) {
    sum += *x;
    x   += incx;
  }
  return sum;
}
