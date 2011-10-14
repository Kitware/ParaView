#include <math.h>
#include <veclib.h>
#include "hotel.h"

#include <stdio.h>

#define error_handler(a) { fprintf(stderr,a); exit(-1); }

double ***dtarray(int nwl, int nwh, int nrl, int nrh, int ncl, int nch)
{
  register int i,j;
  int    skipc = nch - ncl + 1,
         skipr = nrh - nrl + 1,
         size  = (nwh - nwl + 1) * skipr * skipc;
  double *m1,
         **m2,
         ***m3;

  m1 = (double *) malloc( size * sizeof(double) );
  if(!m1) error_handler("allocation failure 1 in dtarray()");

  m2 = (double **) malloc((unsigned)(nwh-nwl+1)*skipr*sizeof(double*));
  if(!m2) error_handler("allocation failure 2 in dtarray()");

  m3 = (double ***) malloc((unsigned)(nwh-nwl+1)*sizeof(double*));
  if(!m3) error_handler("allocation failure 3 in dtarray()");

  for(i = 0;i <= (nwh-nwl);i++) {
    m3[i]  = m2 + i * skipr;
    m3[i] -= nrl;
    for(j = 0; j <= (nrh-nrl); ++j, m1 += skipc ) {
      m2[i*skipr + j] = m1;
      m2[i] -= ncl;
    }
  }
  m3 -= nwl;

  return m3;
}

void free_dtarray(double ***m3, int nwl, int nrl, int ncl)
{
  free((char*) m3[nwl][nrl]);
  free((char*) m3[nwl]);
  free((char*) m3);

  return;
}

double **dtmatrix(int l){
  register int i;
  double *m,**m1;

  m1 = (double **)malloc(l*sizeof(double *));
  m  = (double *) malloc((l*(l+1)/2)*sizeof(double));
  for(i = 0; i < l; m+=(l-i), ++i)
    m1[i] = m;

  return m1;
}


/*-------------------------------------------------------------------------*
 * generate a vector of Mode arrays starting at nl and going to nu         *
 * The double arrays are declared in modemem                               *
 *-------------------------------------------------------------------------*/
Mode *mvector(int nl, int nu){
  Mode *m = (Mode *)malloc((nu-nl+1)*sizeof(Mode));
  if(!m) fprintf(stderr,"allocation failure in mvector()");
  return m-nl;
}

Mode *mvecset(int nl, int nu, int qa,int qb,int qc){
  register int i;
  Mode *m =  mvector(nl,nu);

  m[nl].a = dvector(0,(nu-nl+1)*(qa*qb+qc)-1);
  dzero((nu-nl+1)*(qa*qb+qc),m[nl].a,1);

  m[nl].b = m[nl].a + qa;
  m[nl].c = m[nl].b + qb;

  for(i = nl+1; i <= nu; ++i){
    m[i].a = m[i-1].a + (qa+qb+qc);
    m[i].b = m[i].a + qa;
    m[i].c = m[i].b + qb;
  }

  return m;
}

void free_mvec(Mode *m){
  free(m->a); free(m);
}

double mdot(int qa, int qb, int qc, Mode *x, Mode *y){
  return ddot(qa,x->a,1,y->a,1)+ddot(qb,x->b,1,y->b,1)+ddot(qc,x->c,1,y->c,1);
}

void Tri_mvmul2d(int qa, int qb, int qc, Mode *x, Mode *y, Mode *z){
  dvmul(qa,x->a,1,y->a,1,z->a,1);
  dvmul(qb,x->b,1,y->b,1,z->b,1);
}
