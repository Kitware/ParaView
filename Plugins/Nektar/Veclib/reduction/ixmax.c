/*
 * index of maximum value
 */

int idmax(int n, double *x, int incx)
{
  register int    i,
                  indx = ( n > 0 ) ? 0 : -1;
  register double xmax = *x;

  for (i = 0; i < n; i++) {
    if (*x > xmax) {
      xmax = *x;
      indx = i;
    }
    x += incx;
  }
  return indx;
}

int ismax(int n, float *x, int incx)
{
  register int   i,
                 indx = ( n > 0 ) ? 0 : -1;
  register float xmax = *x;

  for (i = 0;i < n;i++) {
    if (*x > xmax) {
      xmax = *x;
      indx = i;
    }
    x += incx;
  }
  return indx;
}


int iimax(int n, int *x, int incx)
{
  register int   i,
                 indx = ( n > 0 ) ? 0 : -1;
  register int   xmax = *x;

  for (i = 0;i < n;i++) {
    if (*x > xmax) {
      xmax = *x;
      indx = i;
    }
    x += incx;
  }
  return indx;
}


/* FORTRAN versions */

int idmax_(int *np, double *x, int *incxp)
{
  register int    i,
                  n    = *np,
                  incx = *incxp,
                  indx = ( n > 0 ) ? 0 : -1;
  register double xmax = *x;

  for(i = 0;i < n;i++) {
    if( *x > xmax ) {
      xmax = *x;
      indx = i;
    }
    x += incx;
  }
  return ++indx;
}

int ismax_(int *np, float *x, int *incxp)
{
  register int   i,
                 n    = *np,
                 incx = *incxp,
                 indx = ( n > 0 ) ? 0 : -1;
  register float xmax = *x;

  for(i = 0;i < n;i++) {
    if( *x > xmax ) {
      xmax = *x;
      indx = i;
    }
    x += incx;
  }
  return ++indx;
}
