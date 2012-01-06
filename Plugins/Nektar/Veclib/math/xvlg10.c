/*
 * Element-wise base 10 logarithm
 */

extern double log10(double);

void dvlg10(int n, double *x, int incx, double *y, int incy)
{
  while( n-- ) {
    *y = log10( *x );
    x += incx;
    y += incy;
  }
  return;
}
