/*
 * Conditional assignment
 */

void dcndst(int n, double *x, int incx, int *y, int incy,
                   double *z, int incz)
{
  while (n--) {
    if( *y ) *z = *x;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}

void scndst(int n, float *x, int incx, int *y, int incy,
                   float *z, int incz)
{
  while (n--) {
    if( *y ) *z = *x;
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}
