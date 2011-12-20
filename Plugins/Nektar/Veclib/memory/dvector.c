/*
 * Double-precision vector
 */

double *dvector(nl,nh)
int nl,nh;
{
  double *v;

  v = (double *)malloc((unsigned) (nh-nl+1)*sizeof(double));
  if (!v) error_handler("allocation failure in dvector()");
  return v-nl;
}

void free_dvector(v,nl,nh)
double *v;
int nl,nh;
{
  free((char*) (v+nl));
}
