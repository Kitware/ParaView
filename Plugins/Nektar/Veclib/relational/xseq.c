/*
 * Vector equal to scalar
 */

void iseq(int n, int *ap, int *x, int incx, int *y, int incy)
{
  register int alpha = *ap,
               true  = 1,
               false = 0;

  while (n--) {
    *y = (alpha == *x) ? true : false;
    x += incx;
    y += incy;
  }
  return;
}
