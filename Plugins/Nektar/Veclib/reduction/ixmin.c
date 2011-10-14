/*
 * index of minimum value
 */

int idmin(int n, double *x, int incx)
{
  register int    i,
                  indx = ( n > 0 ) ? 0 : -1;
  register double xmin = *x;

  for(i = 0;i < n;i++) {
    if( *x < xmin ) {
      xmin = *x;
      indx = i;
    }
    x += incx;
  }
  return indx;
}

int ismin(int n, float *x, int incx)
{
  register int   i,
                 indx = ( n > 0 ) ? 0 : -1;
  register float xmin = *x;

  for(i = 0; i < n; i++) {
    if( *x < xmin ) {
      xmin = *x;
      indx = i;
    }
    x += incx;
  }
  return indx;
}

/* FORTRAN versions */


int idmin_(int *np, double *x, int *incxp)
{
  register int i,
               n    = *np,
               incx = *incxp,
               indx = ( n > 0 ) ? 0 : -1;
  register double xmin = *x;

  for (i = 0; i < n; i++) {
    if( *x < xmin ) {
      xmin = *x;
      indx = i;
    }
  }
  return ++indx;
}

int ismin_(int *np, float *x, int *incxp)
{
  register int i,
               n    = *np,
               incx = *incxp,
               indx = ( n > 0 ) ? 0 : -1;
  register float xmin = *x;

  for (i = 0; i < n; i++) {
    if( *x < xmin ) {
      xmin = *x;
      indx = i;
    }
    x += incx;
  }
  return ++indx;
}
