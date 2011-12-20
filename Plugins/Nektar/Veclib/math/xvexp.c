/*
 * Element-wise exponentiation:  y[i] = exp( x[i] )
 */

extern double exp(double);

void dvexp(int n, double *x, int incx, double *y, int incy)
{
  while( n-- ) {
    *y = exp(*x);
    x += incx;
    y += incy;
  }
  return;
}
