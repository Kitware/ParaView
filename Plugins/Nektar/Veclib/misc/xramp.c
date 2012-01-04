/*
 * Ramp function
 */

void dramp(int n, double *ap, double *bp, double *x, const int incx)
{
  double   alpha = *ap,
           beta  = *bp;
  register i;

  for (i = 0; i < n; i++) {
    *x = alpha + (double) i * beta;
    x += incx;
  }

  return;
}

void sramp(int n, float *ap, float *bp, float *x, const int incx)
{
  float    alpha = *ap,
           beta  = *bp;
  register i;

  for (i = 0; i < n; i++) {
    *x = alpha + (float) i * beta;
    x += incx;
  }

  return;
}

void iramp(int n, int *ap, int *bp, int *x, const int incx)
{
  register i;
  register alpha = *ap,
           beta  = *bp;

  for(i = 0; i < n; i++) {
    *x = alpha + i * beta;
    x += incx;
  }
  return;
}
