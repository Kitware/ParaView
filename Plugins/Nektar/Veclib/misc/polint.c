/*
 * polint() - Polynomial interpolation
 *
 * The following function performs polyomial interpolation through a given
 * set of points using Neville's algorithm.  Given arrays xa[1..n] and
 * ya[1..n], each of length n, this routine returns a value y and error
 * estimate dy.
 *
 * Reference: Numerical Recipes, pp. 80-82.
 *
 */

#include <stdio.h>
#include <math.h>

void polint(double *xa, double *ya, int n, double x, double *y, double *dy){
  register int ns  = 1;
  double       dif = fabs(x - xa[1]),*c, *d;
  register int i,m;
  double       den, dift, ho, hp, w;

  c = (double *) malloc(n*sizeof(double))-1;
  d = (double *) malloc(n*sizeof(double))-1;

  for(i = 1;i <= n;i++) {
    if((dift = fabs(x-xa[i])) < dif) {
      ns  = i;
      dif = dift;
    }
    c[i] = ya[i];
    d[i] = ya[i];
  }

  *y = ya[ns--];
  for(m = 1;m < n;m++) {
    for(i = 1;i <= n - m;i++) {
      ho = xa[i] - x;
      hp = xa[i+m] -x;
      w  = c[i+1] - d[i];
      if( (den = ho-hp) == 0.0) {
  printf("Error in routine POLINT");
  exit(-1);
      }
      den  = w  / den;
      d[i] = hp * den;
      c[i] = ho * den;
    }
    *y += (*dy=((ns<<1) < (n-m) ? c[ns+1] : d[ns--]));
  }

  free_dvector(c, 1);
  free_dvector(d, 1);
  return;
}

double dpoly(int n, double x, double *xp, double *yp)
{
  double value, err;
  polint (xp - 1, yp - 1, n, x, &value, &err);
  return value;
}
