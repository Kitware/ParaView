/*
 *  byte-reversal routines
 */

void dbrev (int n, double *x, int incx, double *y, int incy)
{
  char  *cx, *cy;
  register int i;

  while (n--) {
    cx = (char*) x;
    cy = (char*) y;
    for (i = 0; i < 4; i++) {
      char d  = cx[i];
      cy[i]   = cx[7-i];
      cy[7-i] = d;
    }
    x += incx;
    y += incy;
  }
  return;
}


void sbrev (int n, float *x, int incx, float *y, int incy)
{
  char  *cx, *cy;
  register int i;

  while (n--) {
    cx = (char*) x;
    cy = (char*) y;
    for (i = 0; i < 2; i++) {
      char d  = cx[i];
      cy[i]   = cx[3-i];
      cy[3-i] = d;
    }
    x += incx;
    y += incy;
  }
  return;
}

void ibrev (int n, int *x, int incx, int *y, int incy)
{
  char  *cx, *cy;
  register int i;

  while (n--) {
    cx = (char*) x;
    cy = (char*) y;
    for (i = 0; i < 2; i++) {
      char d  = cx[i];
      cy[i]   = cx[3-i];
      cy[3-i] = d;
    }
    x += incx;
    y += incy;
  }
  return;
}
