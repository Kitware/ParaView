/*
 * Single-precision integer vector
 */

int **imatrix(nrl,nrh,ncl,nch)
int nrl,nrh,ncl,nch;
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip,
         *m1,
         **m2;

  m1 = (int*) malloc((unsigned) size * sizeof(int));
  if(!m1) error_handler("allocation failure 1 in imatrix()");

  m2 = (int **) malloc((unsigned)(nrh-nrl+1)*(sizeof(int*)));
  if(!m2) error_handler("allocation failure 2 in imatrix()");
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

void free_imatrix(m2,nrl,nrh,ncl,nch)
int **m2,nrl,nrh,ncl,nch;
{
  int *m1 = *(m2 + nrl);

  free((char *)(m2 + nrl));
  free((char *)(m1 + ncl));
}
