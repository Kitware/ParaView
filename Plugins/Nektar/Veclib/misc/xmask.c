/*
 * Conditional assignment: z = ( y ) ? ( w ) : ( x )
 */

void dmask(int n, double *w, int incw, double *x, int incx,
            int    *y, int incy, double *z, int incz)
{
  while( n-- ) {
    *z   = ( *y ) ? ( *w ) : ( *x );
    w   += incw;
    x   += incx;
    y   += incy;
    z   += incz;
  }
  return;
}
