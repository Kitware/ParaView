/*
 * Single-precision matrix - contiguous elements
 */

float **matrix(nrl,nrh,ncl,nch)
int nrl,nrh,ncl,nch;
{
  int   i,
        skip = nch - ncl + 1,
        size = (nrh - nrl + 1) * skip;
  float *m1,
        **m2;

  m1 = (float *) malloc( size * sizeof(float) );
  if(!m1) error_handler("allocation failure 1 in matrix()");

  m2 = (float**) malloc((unsigned)(nrh-nrl+1)*sizeof(float*));
  if(!m2) error_handler("allocation failure 2 in matrix()");
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

float **submatrix(a,oldrl,oldrh,oldcl,oldch,newrl,newcl)
float **a;
int oldrl,oldrh,oldcl,oldch,newrl,newcl;
{
  int i,j;
  float **m;

  m=(float **) malloc((unsigned) (oldrh-oldrl+1)*sizeof(float*));
  if (!m) error_handler("allocation failure in submatrix()");
  m -= newrl;

  for(i=oldrl,j=newrl;i<=oldrh;i++,j++) m[j]=a[i]+oldcl-newcl;

  return m;
}

float **convert_matrix(a,nrl,nrh,ncl,nch)
float *a;
int nrl,nrh,ncl,nch;
{
  int i,j,nrow,ncol;
  float **m;

  nrow=nrh-nrl+1;
  ncol=nch-ncl+1;
  m = (float **) malloc((unsigned) (nrow)*sizeof(float*));
  if (!m) error_handler("allocation failure in convert_matrix()");
  m -= nrl;
  for(i=0,j=nrl;i<=nrow-1;i++,j++) m[j]=a+ncol*i-ncl;
  return m;
}

void free_matrix(m2,nrl,nrh,ncl,nch)
float **m2;
int nrl,nrh,ncl,nch;
{
  float *m1 = *(m2 + nrl);

  free((char*) (m2 + nrl));
  free((char*) (m1 + ncl));

  return;
}

void free_submatrix(b,nrl,nrh,ncl,nch)
float **b;
int nrl,nrh,ncl,nch;
{
  free((char*) (b+nrl));
}

void free_convert_matrix(b,nrl,nrh,ncl,nch)
float **b;
int nrl,nrh,ncl,nch;
{
  free((char*) (b+nrl));
}
