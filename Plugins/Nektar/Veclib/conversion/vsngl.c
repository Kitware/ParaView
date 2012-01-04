/*
 * Convert double to single precision
 */

void vsngl(int n, double *x, int incx, float *y, int incy)
{
  if (incx == 1 && incy == 1)
    while (n--)
      *y++ = (float) *x++;
  else
    while (n--) {
      *y  = (float) *x;
       x += incx;
       y += incy;
    }

  return;
}
