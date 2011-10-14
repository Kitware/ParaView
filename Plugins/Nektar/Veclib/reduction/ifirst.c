/*
 * Index of first logical true value
 */

int ifirst(int n, int *x, int incx)
{
  register int i, ic = -1;

  for(i = 0; i < n; i++) {
    if( *x ) {
      ic = i;
      break;
    }
    x += incx;
  }
  return ic;
}
