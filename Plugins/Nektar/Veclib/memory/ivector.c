/*
 * Single-precision integer vector
 */

int *ivector(nl,nh)
int nl,nh;
{
  int *v;

  v=(int *)malloc((unsigned) (nh-nl+1)*sizeof(int));
  if (!v) error_handler("allocation failure in ivector()");
  return v-nl;
}

void free_ivector(v,nl,nh)
int *v,nl,nh;
{
  free((char*) (v+nl));
}
