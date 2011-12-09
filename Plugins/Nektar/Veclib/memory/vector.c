/*
 * Single-precision vector
 */

float *vector(nl,nh)
int nl,nh;
{
  float *v;

  v=(float *)malloc((unsigned) (nh-nl+1)*sizeof(float));
  if (!v) error_handler("allocation failure in vector()");
  return v-nl;
}

void free_vector(v,nl,nh)
float *v;
int nl,nh;
{
  free((char*) (v+nl));
}
