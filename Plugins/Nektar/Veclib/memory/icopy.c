/*
 * Copy a vector
 *
 * NOTE: This does not follow BLAS conventions on the skip.  A negative
 *       skip goes backward in memory from the input location, not from
 *       the end of the array.
 */

#include <string.h>

void icopy (int n, int *x, int incx, int *y, int incy)
{
  if (incx == 1 && incy == 1)
    memcpy (y, x, n * sizeof(int));
  else
    while (n--) {
      *y = *x;
      x += incx;
      y += incy;
    }

  return;
}
