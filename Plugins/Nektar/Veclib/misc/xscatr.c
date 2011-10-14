/*
 *  vector scatter
 */

void dscatr(int n, double *x, int *y, double *z)
{
#ifdef i860
  register int i;
  for(i = 0; i < n; i++)
    z[ y[i] ] = x[i];
#else
  while (n--) *(z + *(y++)) = *(x++);
#endif
  return;
}

void sscatr(int n, float *x, int *y, float *z)
{
#ifdef i860
  register int i;
  for(i = 0; i < n; i++)
    z[ y[i] ] = x[i];
#else
  while (n--) *(z + *(y++)) = *(x++);
#endif
  return;
}
