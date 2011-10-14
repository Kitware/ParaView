/*
 * Polynomial evaluation
 *
 */

void dvpoly(int n, double *x, int incx, int m, double *c, int incc,
       double *y, int incy)
{
#ifdef i860
  register double sum, xp;
  register int    i, j;

  for(i = 0; i < n; i++) {
    xp  = x[i*incx];
    sum = c[0];
    for(j = 1; j <= m; j++) sum += sum * xp + c[j*incc];
    y[i*incy] = sum;
  }
#else
  register double *cp, xp, sum;
  register int     mc;

  while (n--) {
    xp  = *x;
    cp  =  c + incc;
    sum = *c;
    for(mc = 0; mc < m; mc++, cp += incc) sum += sum * xp + *cp;
    *y  = sum;
    y  += incy;
    x  += incx;
  }
#endif
  return;
}
