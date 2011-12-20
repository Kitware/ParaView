#include <stdio.h>
#include <malloc.h>
#include "util.h"

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

double *dvector(nl,nh)
int nl,nh;
{
  double *v;

  v=(double *)malloc((unsigned) (nh-nl+1)*sizeof(double));
  if (!v) error_handler("allocation failure in dvector()");
  return v-nl;
}

void free_dvector(v,nl,nh)
double *v;
int nl,nh;
{
  free((char*) (v+nl));
}

float **matrix(nrl,nrh,ncl,nch)
int nrl,nrh,ncl,nch;
{
  int i;
  float **m;

  m=(float **) malloc((unsigned) (nrh-nrl+1)*sizeof(float*));
  if (!m) error_handler("allocation failure 1 in matrix()");
  m -= nrl;

  for(i=nrl;i<=nrh;i++) {
    m[i]=(float *) malloc((unsigned) (nch-ncl+1)*sizeof(float));
    if (!m[i]) error_handler("allocation failure 2 in matrix()");
    m[i] -= ncl;
  }
  return m;
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

void free_matrix(m,nrl,nrh,ncl,nch)
float **m;
int nrl,nrh,ncl,nch;
{
  int i;

  for(i=nrh;i>=nrl;i--) free((char*) (m[i]+ncl));
  free((char*) (m+nrl));
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

int **imatrix(nrl,nrh,ncl,nch)
int nrl,nrh,ncl,nch;
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip,
         *m1,
         **m2;

  m1 = (int *) malloc((unsigned) size * sizeof(int));
  if(!m1) error_handler("allocation failure 1 in imatrix2()");

  m2 = (int **) malloc((unsigned)(nrh-nrl+1)*(sizeof(int*)));
  if(!m2) error_handler("allocation failure 2 in imatrix2()");
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

double **dmatrix(nrl,nrh,ncl,nch)
int nrl,nrh,ncl,nch;
{
  int    i,
         skip = nch - ncl + 1,
         size = (nrh - nrl + 1) * skip;
  double *m1,
         **m2;

  m1 = (double *) malloc( size * sizeof(double) );
  if(!m1) error_handler("allocation failure 1 in dmatrix2()");

  m2 = (double **) malloc((unsigned)(nrh-nrl+1)*sizeof(double*));
  if(!m2) error_handler("allocation failure 2 in dmatrix2()");
  m2 -= nrl;

  for(i = nrl;i <= nrh;i++) {
    m2[i]  = m1 + (i - nrl) * skip;
    m2[i] -= ncl;
  }
  return m2;
}

double **dsubmatrix(a,oldrl,oldrh,oldcl,oldch,newrl,newcl)
double **a;
int oldrl,oldrh,oldcl,oldch,newrl,newcl;
{
  int i,j;
  double **m;

  m=(double **) malloc((unsigned) (oldrh-oldrl+1)*sizeof(double*));
  if (!m) error_handler("allocation failure in submatrix()");
  m -= newrl;

  for(i=oldrl,j=newrl;i<=oldrh;i++,j++) m[j]=a[i]+oldcl-newcl;

  return m;
}

double **convert_dmatrix(a,nrl,nrh,ncl,nch)
double *a;
int nrl,nrh,ncl,nch;
{
  int i,j,nrow,ncol;
  double **m;

  nrow=nrh-nrl+1;
  ncol=nch-ncl+1;
  m = (double **) malloc((unsigned) (nrow)*sizeof(double*));
  if (!m) error_handler("allocation failure in convert_dmatrix()");
  m -= nrl;
  for(i=0,j=nrl;i<=nrow-1;i++,j++) m[j]=a+ncol*i-ncl;
  return m;
}

void free_dmatrix(m2,nrl,nrh,ncl,nch)
double **m2;
int nrl,nrh,ncl,nch;
{
  double *m1 = *(m2 + nrl);

  free((char *) (m2 + nrl));
  free((char *) (m1 + ncl));
}

void free_dsubmatrix(b,nrl,nrh,ncl,nch)
double **b;
int nrl,nrh,ncl,nch;
{
  free((char*) (b+nrl));
}

void free_convert_dmatrix(b,nrl,nrh,ncl,nch)
double **b;
int nrl,nrh,ncl,nch;
{
  free((char*) (b+nrl));
}

void error_handler(error_text)
char error_text[];
{
  void exit();

  fprintf(stderr,"utility library: ");
  fprintf(stderr,"%s\n",error_text);
  fprintf(stderr,"...now exiting to system...\n");
  exit(1);
}
