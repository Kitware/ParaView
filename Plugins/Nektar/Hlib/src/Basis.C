/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
/**************************************************************************/

#include <math.h>
#include <veclib.h>
#include <polylib.h>
#include "hotel.h"
#include <smart_ptr.hpp>

#include <stdio.h>

using namespace polylib;

int LZero = 0;

void set_LZero(int val){
  LZero = val;
}

/* zeros weights info */
typedef struct zwinfo {
  int     q;   /* number of quadrature points                       */
  char    dir; /* direction of zeros/weights either 'a', 'b' or 'c' */
  double *z;   /* zeros   */
  double *w;   /* weigths */
  struct zwinfo *next;
} ZWinfo;

/*-------------------------------------------------------------------*
 *  link list for zeros and weights for jacobi polynomial in [0,1]   *
 *  at the gauss points. Note: alpha > -1 , beta > -1                *
 *-------------------------------------------------------------------*/

static ZWinfo *zwinf,*zwbase;
static ZWinfo *addzw(int, char);

void  getzw(int q, double **z, double **w, char dir){
  /* check link list */

  for(zwinf = zwbase; zwinf; zwinf = zwinf->next)
    if(zwinf->q == q)
      if(zwinf->dir == dir){
  *z = zwinf->z;
  *w = zwinf->w;
  return;
      }

  /* else add new zeros and weights */

  zwinf  = zwbase;
  zwbase = addzw(q,dir);
  zwbase->next = zwinf;

  *z = zwbase->z;
  *w = zwbase->w;

  return;
}

/*--------------------------------------------------------------------------*
 * This function gets the zeros and weights in [-1,1]. In the 'a' direction *
 * alpha = 0, beta = 0. In the 'b' alpha = 1, beta = 0 and in the 'c'       *
 * direction alpha = 2, beta = 0.                                           *
 * Note: that the 'b','c' direction weights are scaled by (1/2)^alpha       *
 * to be consistent with the jacobean d(r,s)/d(t,s)                         *
 *--------------------------------------------------------------------------*/

static ZWinfo *addzw(int n, char dir){
  int i;
  ZWinfo *zw = (ZWinfo *)calloc(1,sizeof(ZWinfo));

  zw->q     = n;
  zw->dir   = dir;

  zw->z = dvector(0,n-1);
  zw->w = dvector(0,n-1);
  switch(dir){
  case 'a':
    zwgll(zw->z,zw->w,n);
    break;
  case 'b':
    if(LZero){ // use Legendre weights
      zwgrj(zw->z,zw->w,n,0.0,0.0);
      for(i = 0; i < n; ++i)
  zw->w[i] *= (1-zw->z[i])*0.5;
    }
    else{
      zwgrj(zw->z,zw->w,n,1.0,0.0);
      dscal(n,0.5,zw->w,1);
    }
    break;
  case 'c':
    if(LZero){
      zwgrj(zw->z,zw->w,n,0.0,0.0);
      for(i = 0; i < n; ++i){
  zw->w[i] *= (1-zw->z[i])*0.5;
  zw->w[i] *= (1-zw->z[i])*0.5;
      }
    }
    else{
      zwgrj(zw->z,zw->w,n,2.0,0.0);
      dscal(n,0.25,zw->w,1);
    }
    break;
  case 'g': /* just Gauss points */
    zwgl (zw->z,zw->w,n);
    break;
  case 'h': /* Gauss points with 1,0 weight */
    if(LZero){
      zwgj (zw->z,zw->w,n,0.0,0.0);
      for(i = 0; i < n; ++i)
  zw->w[i] *= (1-zw->z[i])*0.5;
    }
    else{
      zwgj (zw->z,zw->w,n,1.0,0.0);
      dscal(n,0.5,zw->w,1);
    }
    break;
  case 'i':
    if(LZero){
      zwgj (zw->z,zw->w,n,0.0,0.0);
      for(i = 0; i < n; ++i){
  zw->w[i] *= (1-zw->z[i])*0.5;
  zw->w[i] *= (1-zw->z[i])*0.5;
      }
    }
    else{
      zwgj (zw->z,zw->w,n,2.0,0.0);
      dscal(n,0.25,zw->w,1);
    }
    break;
  case 'j':
    zwgrj(zw->z,zw->w,n,0.0,0.0);
    break;
  default:
    fprintf(stderr,"addzw - incorect direction specified");
    break;
  }

  return zw;
}

typedef struct iminfo {
  int        q1;  /* number of quadrature points                       */
  int        q2;
  InterpDir  dir; /* direction of zeros/weights either 'a', 'b' or 'c' */
  double    **im; /* zeros   */
  struct iminfo *next;
} IMinfo;

/*---------------------------------------------------------------------*
 *  link list for interpolation matrices:                              *
 *     InterpDir :  a2b   go from gll       points to grj_(1,0) points *
 *                  b2a   go from grj_(1,0) points to gll points       *
 *                  b2c   go from grj_(1,0) points to grj_(2,0) points *
 *                  c2b   go from grj_(2,0) points to grj_(2,0) points *
 *---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*
 *  Matrix storage for interpolation matrices:                              *
 *     InterpDir :  a2a   go from gll       points to gll       points *
 *                  a2b   go from gll       points to grj_(1,0) points *
 *                  a2g   go from gll       points to gl        points *
 *                  b2a   go from grj_(1,0) points to gll points       *
 *                  b2g   go from grj_(1,0) points to glj_(2,0) points *
 *                  b2c   go from grj_(1,0) points to grj_(2,0) points *
 *                  b2g   go from grj_(1,0) points to gl        points *
 *                  c2b   go from grj_(2,0) points to grj_(1,0) points *
 *                  c2c   go from grj_(2,0) points to grj_(2,0) points *
 *                  c2g   go from grj_(2,0) points to gl        points *
 *---------------------------------------------------------------------*/




static IMinfo *iminf,*imbase;
static IMinfo *addim(int,int,InterpDir);

void  getim(int q1, int q2, double ***im, InterpDir dir){

  /* check link list */

  for(iminf = imbase; iminf; iminf = iminf->next)
    if(iminf->q1 == q1)
      if(iminf->q2 == q2)
  if(iminf->dir == dir){
    *im = iminf->im;
    return;
  }

  /* else add new interpolation matrix */

  iminf  = imbase;
  imbase = addim(q1,q2,dir);
  imbase->next = iminf;

  *im = imbase->im;

  return;
}

static IMinfo *addim(int n1, int n2, InterpDir dir){
  IMinfo *im = (IMinfo *)malloc(sizeof(IMinfo));
  double *z1,*z2,*w;

  im->q1    = n1;
  im->q2    = n2;
  im->dir   = dir;

  im->im = dmatrix(0,n2-1,0,n1-1);

  switch(dir){
  case a2a:  /* interpolate a to a */
    {
      getzw(n1,&z1,&w,'a');
      getzw(n2,&z2,&w,'a');
      igllm(im->im,z1,z2,n1,n2);
      break;
    }
  case a2b:  /* interpolate a to b */
    {
      getzw(n1,&z1,&w,'a');
      getzw(n2,&z2,&w,'b');
      igllm(im->im,z1,z2,n1,n2);
      break;
    }
  case a2g:  /* interpolate a to g (gauss) */
    {
      getzw(n1,&z1,&w,'a');
      getzw(n2,&z2,&w,'g');
      igllm(im->im,z1,z2,n1,n2);
      break;
    }
  case b2a:  /* interpolate b to a */
    {
      getzw(n1,&z1,&w,'b');
      getzw(n2,&z2,&w,'a');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  case b2b:  /* interpolate b to b */
    {
      getzw(n1,&z1,&w,'b');
      getzw(n2,&z2,&w,'b');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  case b2c:  /* interpolate b to c */
    {
      getzw(n1,&z1,&w,'b');
      getzw(n2,&z2,&w,'c');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  case b2g:  /* interpolate b to g (gauss) */
    {
      getzw(n1,&z1,&w,'b');
      getzw(n2,&z2,&w,'g');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  case b2h:  /* interpolate b (Radau) to h (gauss) */
    {
      getzw(n1,&z1,&w,'b');
      getzw(n2,&z2,&w,'h');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  case c2b:  /* interpolate c to b */
    {
      getzw(n1,&z1,&w,'c');
      getzw(n2,&z2,&w,'b');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,2.0,0.0);
      break;
    }
  case c2c:  /* interpolate c to c */
    {
      getzw(n1,&z1,&w,'c');
      getzw(n2,&z2,&w,'c');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,2.0,0.0);
      break;
    }
  case c2g:  /* interpolate c to g (gauss) */
    {
      getzw(n1,&z1,&w,'c');
      getzw(n2,&z2,&w,'g');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,2.0,0.0);
      break;
    }
  case c2h:  /* interpolate c (Radau1) to h (gauss) */
    {
      getzw(n1,&z1,&w,'c');
      getzw(n2,&z2,&w,'h');
      if(LZero)
  igrjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igrjm(im->im,z1,z2,n1,n2,2.0,0.0);
      break;
    }
  case g2g:  /* interpolate g (gauss) to g (gauss) */
    {
      getzw(n1,&z1,&w,'g');
      getzw(n2,&z2,&w,'g');
      igjm(im->im,z1,z2,n1,n2,0.0,0.0);
      break;
    }
  case g2a:  /* interpolate g (gauss) to g (gauss) */
    {
      getzw(n1,&z1,&w,'g');
      getzw(n2,&z2,&w,'a');
      igjm(im->im,z1,z2,n1,n2,0.0,0.0);
      break;
    }
  case g2b:
    {
      getzw(n1,&z1,&w,'g');
      getzw(n2,&z2,&w,'b');
      igjm(im->im,z1,z2,n1,n2,0.0,0.0);
      break;
    }
  case g2c:
    {
      getzw(n1,&z1,&w,'g');
      getzw(n2,&z2,&w,'c');
      igjm(im->im,z1,z2,n1,n2,0.0,0.0);
      break;
    }
  case h2b:
    {
      getzw(n1,&z1,&w,'h');
      getzw(n2,&z2,&w,'b');
      if(LZero)
  igjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  case h2c:
    {
      getzw(n1,&z1,&w,'h');
      getzw(n2,&z2,&w,'c');
      if(LZero)
  igjm(im->im,z1,z2,n1,n2,0.0,0.0);
      else
  igjm(im->im,z1,z2,n1,n2,1.0,0.0);
      break;
    }
  default:
    fprintf(stderr,"addim - incorect direction specified");
    break;
  }

  return im;
}


// L_2 normalise a

void normalise_mode(int q, double *v, char dir){
  if(option("TWOTWO")){
    double *z, *w, d, hd;
    double *tmp  = dvector(0, q-1);
    double *tmp1 = dvector(0, q-1);

    getzw(q, &z, &w, dir);
    dvmul(q, v, 1, v, 1, tmp, 1);

    d = ddot(q, tmp, 1, w, 1);
    hd = sqrt(d);

    dscal(q, 1./hd, v, 1);

    free(tmp); free(tmp1);
  }

  return;
}


void normalised_iprod_1d(int q, double *h, int L, double *m, double *res,
       double *invnorm, char dir){

  double *wk = dvector(0, q-1);
  double *z, *w;
  getzw(q, &z, &w, dir);

  dvmul(q, w, 1, h, 1, wk, 1);

  dgemv('T', q, L, 1., m, q, wk, 1, 0.0, res, 1);

  dvmul(L, invnorm, 1, res, 1, res, 1);

  free(wk);
}

// assumes 'a'x'a'
// interior modes..
void quad_normalised_iprod_2d(int q, double *h, int L, double *m, double *res,
            double *invnorm)
{
  int i;
  double *wk  = dvector(0, q*q-1);
  double *wk1 = dvector(0, q*q-1);
  double *z, *w;
  getzw(q, &z, &w, 'a');

  // multiply field by weights
  for(i = 0; i < q; ++i) dvmul(q, w, 1, h+i*q,1, wk+i*q,1);
  for(i = 0; i < q; ++i) dvmul(q, w, 1,  wk+i,q,   wk+i,q);

  dgemm('T','N', L, q, q, 1.0,  m, q, wk, q, 0.0, wk1, L);  // 'a' direction
  dgemm('N','N', L, L, q, 1.0, wk, L,  m, q, 0.0, res, L);  // 'b' direction

  dvmul(L*L, invnorm, 1, res, 1, res, 1);
  free(wk);  free(wk1);
}

// assumes 'a'x'b'
// interior modes..
void tri_normalised_iprod_2d(int qa, char dira, char dirb,  int qb, double *h, int L, double *ma, double *mb, double *res, double *invnorm)
{
  int i;
  double *wk = dvector(0, max(qa, qb)*max(qa,qb) -1);
  double *wk1 = dvector(0, max(qa, qb)*max(qa,qb) -1);
  double *z, *w, *wk1_a, *res_a;


  // multiply field by weights
  getzw(qa, &z, &w, dira);
  for(i = 0; i < qb; ++i) dvmul(qa,    w,1, h+i*qa, 1, wk+i*qa, 1);
  getzw(qb, &z, &w, dirb);
  for(i = 0; i < qb; ++i) dscal(qa, w[i], wk+i*qa, 1);

  // 'a' direction integrals
  dgemm('T','N',qb, L, qa, 1.0, wk, qa, ma, qa, 0.0, wk1, qb);

  // 'b' direction integrals
  wk1_a = wk1;
  res_a = res;
  // +1 -- ignore first edge mode
  for(i = 0; i < L; mb += qb*(L-i+1), wk1_a += qb, res_a+=L-i, ++i)
    dgemv('T', qb, L-i, 1.0, mb, qb, wk1_a, 1, 0.0, res_a, 1);

  dvmul(L*(L+1)/2, invnorm, 1, res, 1, res, 1);

  free(wk);  free(wk1);
}




static double norm(int np, double *d, char dir){
  double fac =0.;
  double *tmp = dvector(0, np-1);
  double *z, *w;
  getzw(np, &z, &w, dir);

  dvmul(np, d, 1, d, 1, tmp, 1);
  fac = sqrt(ddot(np, tmp, 1, w, 1));

  free(tmp);
  return fac;
}


/* interpolation matrix */

namespace {

// Use this to free memory allocated to BasisA_G, BasisA_GL, BasisA_GR
struct DestroyBasisA {
  void operator()(double *** basis)
  {
    // i starts at 1 because of the offset to the address after memory
    // allocation
    for (int i = 1; i <= QGmax; ++i) {
      if (basis[i]) {
  free_dmatrix(basis[i], 0, 0);
      }
    }
    // The increment is needed to return to the address of the allocation.
    free(++basis);
  }
};

// Use this to free memory allocated to BasisB_G, BasisB_GR, BasisC_GR, BasisC_G
struct DestroyBasisBC {
  void operator()(double **** basis)
  {
    // i starts at 1 because of the offset to the address after memory
    // allocation
    for (int i = 1; i <= QGmax; ++i) {
      if (basis[i]) {
  free(basis[i][0][0]);
  free(basis[i][0]);
  free(basis[i]);
      }
    }
    // The increment is needed to return to the address of the allocation.
    free(++basis);
  }
};

// wrap double *** BasisA_G inside a scoped_ptr.
nektar::scoped_ptr<double **, DestroyBasisA> BasisA_G;
nektar::scoped_ptr<double **, DestroyBasisA> BasisA_GL;
nektar::scoped_ptr<double **, DestroyBasisA> BasisA_GR;
// wrap double **** BasisB_G inside a scoped_ptr.
nektar::scoped_ptr<double ***, DestroyBasisBC> BasisB_G;
nektar::scoped_ptr<double ***, DestroyBasisBC> BasisB_GR;
nektar::scoped_ptr<double ***, DestroyBasisBC> BasisC_G;
nektar::scoped_ptr<double ***, DestroyBasisBC> BasisC_GR;

} // namespace anon

/* This function initialises the orthogonal basis statically defined
   matrices based on QGmax. These arrays may be accessed from [1-QGmax]*/

void init_ortho_basis(void){

  double *** ptr3star = (double ***) calloc(QGmax, sizeof(double **));
  BasisA_G.reset(ptr3star - 1);

  ptr3star = (double ***) calloc(QGmax, sizeof(double **));
  BasisA_GL.reset(ptr3star - 1);

  ptr3star = (double ***) calloc(QGmax, sizeof(double **));
  BasisA_GR.reset(ptr3star - 1);

  double **** ptr4star = (double ****) calloc(QGmax, sizeof(double ***));
  BasisB_G.reset(ptr4star - 1);

  ptr4star = (double ****) calloc(QGmax, sizeof(double ***));
  BasisB_GR.reset(ptr4star - 1);

  ptr4star = (double ****) calloc(QGmax, sizeof(double ***));
  BasisC_G.reset(ptr4star - 1);

  ptr4star = (double ****) calloc(QGmax, sizeof(double ***));
  BasisC_GR.reset(ptr4star - 1);
}

/* Gets the values of "mode" (0..LGmax) degree Leg. poly on "size"
   Gauss points, stores the result in BasisA_G[size][mode] array */
void get_moda_G  (int size, double ***mat){

  if (!BasisA_G[size]){
    double *w, *z;
    int i;

    BasisA_G[size] = dmatrix(0,LGmax-1,0,size-1);
    getzw(size,&z,&w,'g');

    for (i=0; i < LGmax; ++i){
      jacobf(size, z, BasisA_G[size][i], i, 0.0, 0.0);
      /* normalise so that  (g,g)=1*/
#ifndef NONORM
      dscal(size,sqrt(0.5*(2.0*i+1.0)),BasisA_G[size][i],1);
#endif
    }
  }

  *mat = BasisA_G[size];
  return;
}

/* computes the values of a_mode[mode_a][j], mode_a=0..LGmax,
   j=0..size-1, at size Gauss-Lob. points, stores the result in
   BasisA_GL[size][mode_a] array */
void get_moda_GL (int size, double ***mat){

  if (!BasisA_GL[size]){
    double *w, *z;
    int i;
    double *tmp = dvector(0, size-1);
    BasisA_GL[size] = dmatrix(0,LGmax-1,0,size-1);
    getzw(size,&z,&w,'a');

    for (i=0; i < LGmax; ++i){
      jacobf(size, z, BasisA_GL[size][i], i, 0.0, 0.0);

#ifndef NONORM
      /* normalise so that (g,g)=1*/
      dscal(size,sqrt(0.5*(2.0*i+1.0)),BasisA_GL[size][i],1);
#endif
    }
    free(tmp);

  }
  *mat = BasisA_GL[size];
  return;
}


/* computes the values of a_mode[mode_a][j], mode_a=0..LGmax,
   j=0..size-1, at size Gauss-Lob. points, stores the result in
   BasisA_GL[size][mode_a] array */
void get_moda_GR (int size, double ***mat){

  if (!BasisA_GR[size]){
    double *w, *z;
    int i;
    double *tmp = dvector(0, size-1);
    BasisA_GR[size] = dmatrix(0,LGmax-1,0,size-1);
    getzw(size,&z,&w,'b');

    for (i=0; i < LGmax; ++i){
      jacobf(size, z, BasisA_GR[size][i], i, 0.0, 0.0);
#ifndef NONORM
      /* normalise so that (g,g)=1*/
      dscal(size,sqrt(0.5*(2.0*i+1.0)),BasisA_GR[size][i],1);
#endif
    }
    free(tmp);

  }
  *mat = BasisA_GR[size];
  return;
}

/* computes the values of b dependent modes at either the gauss or
   gauss radau points depending on the function and stores the result
   in BasisB_G array */

static double ***get_Bmodes(int size, double *z);
static double ***get_Cmodes(int size, double *z);

void get_modb_G (int size, double ****mat){

  if (!BasisB_G[size]){
    double *w,*z;
    getzw (size,&z,&w,'h');
    BasisB_G[size] = get_Bmodes(size,z);
  }

  *mat = BasisB_G[size];
  return;
}

void get_modb_GR (int size, double ****mat){

  if (!BasisB_GR[size]){
    double *w,*z;
    getzw (size,&z,&w,'b');
    BasisB_GR[size] = get_Bmodes(size,z);
  }

  *mat = BasisB_GR[size];
  return;
}

static double ***get_Bmodes(int size, double *z){
  double ***mat;
  double fac;
  int i,j,cnt;

  /* declare matrix */
  mat       = (double ***)malloc(LGmax*sizeof(double **));
  mat[0]    = (double **) malloc(LGmax*(LGmax+1)/2*sizeof(double *));
  mat[0][0] = (double *)  malloc(LGmax*(LGmax+1)/2*size*sizeof(double));

  for(i = 0,cnt=0; i < LGmax; cnt+=LGmax-i,++i)
    mat[i] = mat[0] + cnt;

  for(i = 0,cnt=0; i < LGmax; ++i)
    for(j = 0; j < LGmax-i; ++j,++cnt)
      mat[i][j] = mat[0][0] + size*cnt;

  dfill(size,1.0,mat[0][0],1);
  for(j = 1; j < LGmax; ++j){
    jacobf(size,z,mat[0][j],j,1.0,0.0);
    dvmul(size,mat[0][0],1,mat[0][j],1,mat[0][j],1);
  }

  if(LGmax > 1){
    for(i = 0; i < size; ++i)
      mat[1][0][i] = 1.0 - z[i];
    for(j = 1; j < LGmax-1; ++j){
      jacobf(size,z,mat[1][j],j,3.0,0.0);
      dvmul(size,mat[1][0],1,mat[1][j],1,mat[1][j],1);
    }

    for(i = 2; i < LGmax; ++i){
      dvmul(size,mat[1][0],1,mat[i-1][0],1,mat[i][0],1);
      for(j = 1; j < LGmax-i; ++j){
  jacobf(size,z,mat[i][j],j,2.0*i+1.0,0.0);
  dvmul(size,mat[i][0],1,mat[i][j],1,mat[i][j],1);
      }
    }
  }

#ifndef NONORM
  /* normalise (recalling factor of 2 for weight (1-b)/2 */
  for(i =0, fac = 1.0; i < LGmax; ++i, fac *= 0.25)
    for(j = 0; j < LGmax-i; ++j)
      dscal(size,sqrt((i+j+1.0)*fac),mat[i][j],1);
#endif

  return mat;
}

void get_modc_GR (int size, double ****mat){

  if (!BasisC_GR[size]){
    double *w,*z;
    getzw (size,&z,&w,'c');
    BasisC_GR[size] = get_Cmodes(size,z);
  }

  *mat = BasisC_GR[size];
  return;
}

void get_modc_G (int size, double ****mat){

  if (!BasisC_G[size]){
    double *w,*z;
    getzw (size,&z,&w,'h');
    BasisC_G[size] = get_Cmodes(size,z);
  }

  *mat = BasisC_G[size];
  return;
}


static double ***get_Cmodes(int size, double *z){
  double ***mat;
  double fac;
  int i,j,cnt;

  /* declare matrix */
  mat       = (double ***)malloc(LGmax*sizeof(double **));
  mat[0]    = (double **) malloc(LGmax*(LGmax+1)/2*sizeof(double *));
  mat[0][0] = (double *)  malloc(LGmax*(LGmax+1)/2*size*sizeof(double));

  for(i = 0,cnt=0; i < LGmax; cnt+=LGmax-i,++i)
    mat[i] = mat[0] + cnt;

  for(i = 0,cnt=0; i < LGmax; ++i)
    for(j = 0; j < LGmax-i; ++j,++cnt)
      mat[i][j] = mat[0][0] + size*cnt;

  dfill(size,1.0,mat[0][0],1);
  for(j = 1; j < LGmax; ++j){
    jacobf(size,z,mat[0][j],j,2.0,0.0);
    dvmul(size,mat[0][0],1,mat[0][j],1,mat[0][j],1);
  }

  if(LGmax > 1){
    for(i = 0; i < size; ++i)
      mat[1][0][i] = 1.0 - z[i];
    for(j = 1; j < LGmax-1; ++j){
      jacobf(size,z,mat[1][j],j,4.0,0.0);
      dvmul(size,mat[1][0],1,mat[1][j],1,mat[1][j],1);
    }

    for(i = 2; i < LGmax; ++i){
      dvmul(size,mat[1][0],1,mat[i-1][0],1,mat[i][0],1);
      for(j = 1; j < LGmax-i; ++j){
  jacobf(size,z,mat[i][j],j,2.0*i+2.0,0.0);
  dvmul(size,mat[i][0],1,mat[i][j],1,mat[i][j],1);
      }
    }
  }

#ifndef NONORM
  /* normalise (recalling factor of 2 for weight (1-b)/2 */
  for(i =0, fac = 1.; i < LGmax; ++i, fac *= 0.25)
    for(j = 0; j < LGmax-i; ++j)
      dscal(size,sqrt((i+j+1.5)*fac),mat[i][j],1);
#endif

  return mat;
}

void   Tri_reset_basis(void);
void   Quad_reset_basis(void);

void   Tet_reset_basis(void);
void   Pyr_reset_basis(void);
void   Prism_reset_basis(void);
void   Hex_reset_basis(void);

void reset_bases(){
  Tri_reset_basis();
  Quad_reset_basis();

  Tet_reset_basis();
  Pyr_reset_basis();
  Prism_reset_basis();
  Hex_reset_basis();
}


namespace {

struct DestroyMod_BasisA {
  void operator()(double *** basis)
  {
    // i starts at 1 because of the offset to the address after memory
    // allocation.
    for (int i = 1; i <= QGmax; ++i) {
      if (basis[i]) {
  free_dmatrix(basis[i], 0, 0);
      }
    }
    // The increment is needed to return to the address of the allocation.
    free(++basis);
  }
};

struct DestroyMod_BasisB {
  void operator()(double **** basis)
  {
    // i starts at 1 because of the offset to the address after memory
    // allocation.
    for (int i = 1; i <= QGmax; ++i) {
      if (basis[i]) {
  free(basis[i][0][0]);
  free(basis[i][0]);
  free(basis[i]);
      }
    }
    // The increment is needed to return to the address of the allocation.
    free(++basis);
  }
};

// wrap double *** Mod_BasisA inside a scoped_ptr.
nektar::scoped_ptr<double **, DestroyMod_BasisA> Mod_BasisA;
// wrap double **** Mod_BasisB inside a scoped_ptr.
nektar::scoped_ptr<double ***, DestroyMod_BasisB> Mod_BasisB;

//  // Not used.
//  double **** Mod_BasisC;

} // namespace anon

void init_mod_basis(void){

  double *** ptr3star = (double ***) calloc(QGmax, sizeof(double **));
  Mod_BasisA.reset(ptr3star - 1);

  double **** ptr4star = (double ****)calloc(QGmax, sizeof(double ***));
  Mod_BasisB.reset(ptr4star - 1);

//    // Not used.
//    Mod_BasisC  = (double ****)calloc(QGmax, sizeof(double ***));
//    Mod_BasisC -= 1;
}

/* Computes the values of modified a_mode[mode_a][j], mode_a=0..LGmax,
   j=0..size-1, at size Gauss-Lob. points, stores the result in
   Mod_BasisA[size][mode_a] array
   (1+a)/2
   (1-a)/2
   (1-a)/2 (1+a)/2
   (1-a)/2 (1+a)/2 P_1^{1,1}(a)
   (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)*/

void get_modA(int size, double ***mat){

  if (!Mod_BasisA[size]){
    double *w, *z;
    int i;
    double *tmp = dvector(0, size-1), one = 1.0;

    Mod_BasisA[size] = dmatrix(0,LGmax-1,0,size-1);

    getzw(size,&z,&w,'a');

    dsadd(size,one,z,1,Mod_BasisA[size][0],1);
    dscal(size,0.5,Mod_BasisA[size][0],1);
    dvsub(size,&one,0,z,1,Mod_BasisA[size][1],1);
    dscal(size,0.5,Mod_BasisA[size][1],1);
    if(LGmax > 2){
      dvmul(size, Mod_BasisA[size][0],1,Mod_BasisA[size][1],1,
      Mod_BasisA[size][2],1);
      for (i=3; i < LGmax; ++i){
  jacobf(size, z, Mod_BasisA[size][i],i-2,1.0,1.0);
  dvmul (size, Mod_BasisA[size][2],1, Mod_BasisA[size][i],1,
         Mod_BasisA[size][i],1);
      }
    }

    free(tmp);
  }

  *mat = Mod_BasisA[size];

  return;
}

static double ***get_Mod_Bmodes(int size, double *z);
void get_modB(int size, double ****mat){

  if (!Mod_BasisB[size]){
    double *w,*z;
    getzw (size,&z,&w,'b');
    Mod_BasisB[size] = get_Mod_Bmodes(size,z);
  }

  *mat = Mod_BasisB[size];

  return;
}

/*          (1+b)/2
            (1-b)/2
            (1-b)/2 (1+b)/2 P_{m-1}^{1,1}(b)
            {(1-b)/2}^l
      {(1-b)/2}^2 (1+b)/2 P_{m-1}^{3,1}(b)
      {(1-b)/2}^l (1+b)/2 P_{m-1}^{2l-1,1}(b) */

static double ***get_Mod_Bmodes(int size, double *z){
  double ***mat;
  double fac, one = 1.0;
  int i,j,cnt;

  /* declare matrix */
  mat       = (double ***) malloc(LGmax*sizeof(double **));
  mat[0]    = (double **)  malloc(LGmax*(LGmax+1)/2*sizeof(double *));
  mat[0][0] = (double *)   malloc(LGmax*(LGmax+1)/2*size*sizeof(double));

  for(i = 0,cnt=0; i < LGmax; cnt+=LGmax-i,++i)
    mat[i] = mat[0] + cnt;

  for(i = 0,cnt=0; i < LGmax; ++i)
    for(j = 0; j < LGmax-i; ++j,++cnt)
      mat[i][j] = mat[0][0] + size*cnt;

  dsadd(size,one,z,1,mat[0][0],1);
  dscal(size,0.5,mat[0][0],1);
  dvsub(size,&one,0,z,1,mat[0][1],1);
  dscal(size,0.5,mat[0][1],1);

  if(LGmax > 2){
    dvmul(size, mat[0][0],1,mat[0][1],1,mat[0][2],1);
    for (j=3; j < LGmax; ++j){
      jacobf(size, z, mat[0][j],j-2,1.0,1.0);
      dvmul (size, mat[0][2],1, mat[0][j],1,mat[0][j],1);
    }

    dvmul(size,mat[0][1],1,mat[0][1],1,mat[1][0],1);
    dvmul(size,mat[0][0],1,mat[1][0],1,mat[1][1],1);
    for(j = 2; j < LGmax-1; ++j){
      jacobf(size,z,mat[1][j],j-1,3.0,1.0);
      dvmul(size,mat[1][1],1,mat[1][j],1,mat[1][j],1);
    }

    for(i = 2; i < LGmax-1; ++i){
      dvmul(size,mat[0][1],1,mat[i-1][0],1,mat[i][0],1);
      dvmul(size,mat[0][0],1,mat[i  ][0],1,mat[i][1],1);
      for(j = 2; j < LGmax-i; ++j){
  jacobf(size,z,mat[i][j],j-1,2.0*i+1.0,1.0);
  dvmul(size,mat[i][1],1,mat[i][j],1,mat[i][j],1);
      }
    }
  }

  return mat;
}


namespace {

struct Destroy_Tri_bbase {
  void operator()(Basis **** basis)
  {
    if(!basis)
      return;

    int i,j,k;
    for(i=0;i<LGmax+1;++i)
      for(j=0;j<QGmax+1;++j)
  for(k=0;k<QGmax+1;++k)
    if(basis[i][j][k]) {
      free(basis[i][j][k]->vert[2].a);
      free(basis[i][j][k]->vert[2].b);

      free(basis[i][j][k]->vert);
      free(basis[i][j][k]->edge);
      free(basis[i][j][k]->face);
      free(basis[i][j][k]);
    }

    for(i=0;i<LGmax+1;++i){
      for(j=0;j<QGmax+1;++j)
  free(basis[i][j]);
      free(basis[i]);
    }

    free(basis);
  }
};

// wrap Basis **** Tri_bbase inside a scoped_ptr
nektar::scoped_ptr<Basis ***, Destroy_Tri_bbase> Tri_bbase;

} //namespace anon

static Basis ****Quad_bbase=NULL;
static Basis  *Tet_bbinf=NULL,*Tet_bbase=NULL;
static Basis  *Pyr_bbinf=NULL,*Pyr_bbase=NULL;
static Basis  *Prism_bbinf=NULL,*Prism_bbase=NULL;
static Basis  *Hex_bbinf=NULL,*Hex_bbase=NULL;


Basis *Tri_addbase(int L, int qa, int qb, int qc);
static Basis *Quad_addbase(int L, int qa, int qb, int qc);
static Basis *Tet_addbase(int L, int qa, int qb, int qc);
static Basis *Pyr_addbase(int L, int qa, int qb, int qc);
static Basis *Prism_addbase(int L, int qa, int qb, int qc);
static Basis *Hex_addbase(int L, int qa, int qb, int qc);

static void Tri_normalise(Basis *b, int qa, int qb, int qc);
static void Quad_normalise(Basis *b, char type );
static void Tet_normalise(Basis *b, int , int , int );
static void Pyr_normalise(Basis *, int , int , int );
static void Prism_normalise(Basis *, int , int , int );
static void Hex_normalise(Basis *, int , int , int );

/*

Function name: Element::getbasis

Function Purpose:

Function Notes:

*/


Basis *Tri::getbasis(){

  int i,j;
  if(!Tri_bbase.get()){
    Tri_bbase.reset((Basis****) calloc(LGmax+1, sizeof(Basis***)));
    for(i=0;i<LGmax+1;++i){
      Tri_bbase[i] = (Basis***) calloc(QGmax+1, sizeof(Basis**));
      for(j=0;j<QGmax+1;++j)
  Tri_bbase[i][j] = (Basis**) calloc(QGmax+1, sizeof(Basis*));
    }
  }
  if(!Tri_bbase[lmax][qa][qb])
    Tri_bbase[lmax][qa][qb] = Tri_addbase(lmax,qa,qb,qc);
  return Tri_bbase[lmax][qa][qb];
}

Basis *Quad::getbasis(){

  int i,j;
  if(!Quad_bbase){
    Quad_bbase = (Basis****) calloc(LGmax+1, sizeof(Basis***));
    for(i=0;i<LGmax+1;++i){
      Quad_bbase[i] = (Basis***) calloc(QGmax+1, sizeof(Basis**));
      for(j=0;j<QGmax+1;++j)
  Quad_bbase[i][j] = (Basis**) calloc(QGmax+1, sizeof(Basis*));
    }
  }
  if(!Quad_bbase[lmax][qa][qb])
    Quad_bbase[lmax][qa][qb] = Quad_addbase(lmax,qa,qb,qc);
  return Quad_bbase[lmax][qa][qb];
}

Basis *Tet::getbasis(){
  /* check link list */

  for(Tet_bbinf = Tet_bbase; Tet_bbinf; Tet_bbinf = Tet_bbinf->next)
    if(Tet_bbinf->id == lmax){
      return Tet_bbinf;
    }

  /* else add new zeros and weights */
  Tet_bbinf = Tet_bbase;
  Tet_bbase = Tet_addbase(lmax,qa,qb,qc);
  Tet_bbase->next = Tet_bbinf;

  return Tet_bbase;
}

Basis *Pyr::getbasis(){
  /* check link list */

  for(Pyr_bbinf = Pyr_bbase; Pyr_bbinf; Pyr_bbinf = Pyr_bbinf->next)
    if(Pyr_bbinf->id == lmax){
      return Pyr_bbinf;
    }

  /* else add new zeros and weights */
  Pyr_bbinf = Pyr_bbase;
  Pyr_bbase = Pyr_addbase(lmax,qa,qb,qc);
  Pyr_bbase->next = Pyr_bbinf;

  return Pyr_bbase;
}




Basis *Prism::getbasis(){
  /* check link list */

  for(Prism_bbinf = Prism_bbase; Prism_bbinf; Prism_bbinf = Prism_bbinf->next)
    if(Prism_bbinf->id == lmax){
      return Prism_bbinf;
    }

  /* else add new zeros and weights */
  Prism_bbinf = Prism_bbase;
  Prism_bbase = Prism_addbase(lmax,qa,qb,qc);
  Prism_bbase->next = Prism_bbinf;

  return Prism_bbase;
}




Basis *Hex::getbasis(){
  /* check link list */

  for(Hex_bbinf = Hex_bbase; Hex_bbinf; Hex_bbinf = Hex_bbinf->next)
    if(Hex_bbinf->id == lmax){
      return Hex_bbinf;
    }

  /* else add new zeros and weights */
  Hex_bbinf = Hex_bbase;
  Hex_bbase = Hex_addbase(lmax,qa,qb,qc);
  Hex_bbase->next = Hex_bbinf;

  return Hex_bbase;
}




Basis *Element::getbasis(){return (Basis*)NULL;}




Basis *Tri_addbase(int L, int qa, int qb, int qc){
  Basis   *b = Tri_mem_base(L,qa,qb,qc);

  /* set up storage */
  Tri_mem_modes(b);

  /* set up vertices */
  Tri_set_vertices(b->vert,qa,'a');
  Tri_set_vertices(b->vert,qb,'b');

 if(L>2){
    Tri_set_edges(b,qa,'a');
    Tri_set_edges(b,qb,'b');
  }

  if(L>3)
    Tri_set_faces(b,qb,'b');

  if(option("Normalise"))
     Tri_normalise(b,qa,qb,qc); /* normalise basis so that L2 of each mode = 1 */

  return b;
}




static Basis *Quad_addbase(int L, int qa, int qb, int qc){
  Basis   *b = Quad_mem_base(L,qa,qb,qc);

  /* set up storage */
  Quad_mem_modes(b);

  /* set up vertices */
  Quad_set_vertices(b,qa,'a');
  Quad_set_vertices(b,qb,'b');

  if(L>2){
    Quad_set_edges(b,qa,'a');
    Quad_set_edges(b,qb,'b');
  }

  if(L>2)
    Quad_set_faces(b,qb,'b');

  /* normalise basis so that L2 of each mode = 1 */
  if(option("Normalise"))
    Quad_normalise(b,'B');

  return b;
}




static Basis *Tet_addbase(int L, int qa, int qb, int qc){
  Basis   *b = Tet_mem_base(L,qa,qb,qc);

  /* set up storage */
  Tet_mem_modes(b);

  /* set up vertices */
  Tet_set_vertices(b->vert,qa,'a');
  Tet_set_vertices(b->vert,qb,'b');
  Tet_set_vertices(b->vert,qc,'c');

  if(L>2){
    Tet_set_edges(b,qa,'a');
    Tet_set_edges(b,qb,'b');
    Tet_set_edges(b,qc,'c');
  }

  if(L>3){
    Tet_set_faces(b,qa,'a');
    Tet_set_faces(b,qb,'b');
    Tet_set_faces(b,qc,'c');
 }

  if(option("Normalise"))
     Tet_normalise(b,qa,qb,qc); /* normalise basis so that L2 of each mode = 1 */

  return b;
}



static Basis *Pyr_addbase(int L, int qa, int qb, int qc){
  Basis   *b = Pyr_mem_base(L,qa,qb,qc);

  /* set up storage */
  Pyr_mem_modes(b);

  /* set up vertices */
  Pyr_set_vertices(b->vert,qa,'a');
  Pyr_set_vertices(b->vert,qb,'b');
  Pyr_set_vertices(b->vert,qc,'c');

  if(L>2){
    Pyr_set_edges(b,qa,'a');
    Pyr_set_edges(b,qa,'b');
    Pyr_set_edges(b,qc,'c');
  }

  if(L>3){
    Pyr_set_faces(b,qa,'a');
    Pyr_set_faces(b,qa,'b');
    Pyr_set_faces(b,qc,'c');
  }

  if(option("Normalise"))
    Pyr_normalise(b,qa,qb,qc); /* normalise basis so that L2 of each mode = 1 */

  return b;
}



static Basis *Prism_addbase(int L, int qa, int qb, int qc){
  Basis   *b = Prism_mem_base(L,qa,qb,qc);

  /* set up storage */
  Prism_mem_modes(b);

  /* set up vertices */
  Prism_set_vertices(b->vert,qa,'a');
  Prism_set_vertices(b->vert,qb,'b');
  Prism_set_vertices(b->vert,qc,'c');

  if(L>2){
    Prism_set_edges(b,qa,'a');
    Prism_set_edges(b,qa,'b');
    Prism_set_edges(b,qc,'c');
  }

  if(L>3){
    Prism_set_faces(b,qa,'a');
    Prism_set_faces(b,qa,'b');
    Prism_set_faces(b,qc,'c');
  }

  if(option("Normalise"))
    Prism_normalise(b,qa,qb,qc); /* normalise basis so that L2 of each mode = 1 */

  return b;
}


static Basis *Hex_addbase(int L, int qa, int qb, int qc){
  Basis   *b = Hex_mem_base(L,qa,qb,qc);

  /* set up storage */
  Hex_mem_modes(b);

  /* set up vertices */
  Hex_set_vertices(b->vert,qa,'a');

  if(L>2)
    Hex_set_edges(b,qa,'a');

  if(L>2)
    Hex_set_faces(b,qa,'a');

  if(option("Normalise"))
    Hex_normalise(b,qa,qb,qc); /* normalise basis so that L2 of each mode = 1 */

  return b;
}


void Tri_reset_basis(void){

  Tri_bbase.reset();
#if 0
  int i,j,k;
  if(Tri_bbase.get()){
    for(i=0;i<LGmax+1;++i)
      for(j=0;j<QGmax+1;++j)
  for(k=0;k<QGmax+1;++k)
    if(Tri_bbase[i][j][k]){
      free(Tri_bbase[i][j][k]->vert[2].a);
      free(Tri_bbase[i][j][k]->vert[2].b);

      free(Tri_bbase[i][j][k]->vert);
      free(Tri_bbase[i][j][k]->edge);
      free(Tri_bbase[i][j][k]->face);
      free(Tri_bbase[i][j][k]);
    }
    for(i=0;i<LGmax+1;++i){
      for(j=0;j<QGmax+1;++j)
  free(Tri_bbase[i][j]);
      free(Tri_bbase[i]);
    }
    free(Tri_bbase.get());
  }
#endif
}

void Tri_reset_basis(Basis *B){

  if(B){
    free(B->vert[2].a);
    free(B->vert[2].b);

    free(B->vert);
    free(B->edge);
    free(B->face);
    free(B);
  }
}

void Quad_reset_basis(void){

  int i,j,k;
  if(Quad_bbase){
    for(i=0;i<LGmax+1;++i)
      for(j=0;j<QGmax+1;++j)
  for(k=0;k<QGmax+1;++k)
    if(Quad_bbase[i][j][k]){
      free(Quad_bbase[i][j][k]->vert[1].a);
      free(Quad_bbase[i][j][k]->vert[2].b);

      free(Quad_bbase[i][j][k]->vert);
      free(Quad_bbase[i][j][k]->edge);
      free(Quad_bbase[i][j][k]->face);
      free(Quad_bbase[i][j][k]);
    }
    for(i=0;i<LGmax+1;++i){
      for(j=0;j<QGmax+1;++j)
  free(Quad_bbase[i][j]);
      free(Quad_bbase[i]);
    }
    free(Quad_bbase);
    Quad_bbase = NULL;
  }
}



void Tet_reset_basis(void){
  for(Tet_bbinf = Tet_bbase; Tet_bbinf; Tet_bbinf = Tet_bbinf->next){
    free(Tet_bbinf->vert[2].a);
    free(Tet_bbinf->vert[3].b);
    free(Tet_bbinf->vert[3].c);

    free(Tet_bbinf->vert);
    free(Tet_bbinf->edge);
    free(Tet_bbinf->face);
    if(Tet_bbinf->intr)
      free(Tet_bbinf->intr);
  }
  if(Tet_bbase)
    while(Tet_bbase){
      Tet_bbinf = Tet_bbase->next;
      free(Tet_bbase);
      Tet_bbase = Tet_bbinf;
    }
  Tet_bbase = (Basis *)NULL;
}



void Pyr_reset_basis(void){
  for(Pyr_bbinf = Pyr_bbase; Pyr_bbinf; Pyr_bbinf = Pyr_bbinf->next){
    free(Pyr_bbinf->vert[4].a);
    // small leak here

    free(Pyr_bbinf->vert);
    free(Pyr_bbinf->edge);
    free(Pyr_bbinf->face);
    if(Pyr_bbinf->intr)
      free(Pyr_bbinf->intr);
  }
  if(Pyr_bbase)
    while(Pyr_bbase){
      Pyr_bbinf = Pyr_bbase->next;
      free(Pyr_bbase);
      Pyr_bbase = Pyr_bbinf;
    }
  Pyr_bbase = (Basis *)NULL;
}


void Prism_reset_basis(void){
  for(Prism_bbinf = Prism_bbase; Prism_bbinf; Prism_bbinf = Prism_bbinf->next){
    free(Prism_bbinf->vert[4].a);
    // small leak here

    free(Prism_bbinf->vert);
    free(Prism_bbinf->edge);
    free(Prism_bbinf->face);
    if(Prism_bbinf->intr)
      free(Prism_bbinf->intr);
  }
  if(Prism_bbase)
    while(Prism_bbase){
      Prism_bbinf = Prism_bbase->next;
      free(Prism_bbase);
      Prism_bbase = Prism_bbinf;
    }
  Prism_bbase = (Basis *)NULL;
}


void Hex_reset_basis(void){
  for(Hex_bbinf = Hex_bbase; Hex_bbinf; Hex_bbinf = Hex_bbinf->next){
    free(Hex_bbinf->vert[0].a);

    free(Hex_bbinf->vert);
    free(Hex_bbinf->edge);
    free(Hex_bbinf->face);
    if(Hex_bbinf->intr)
      free(Hex_bbinf->intr);
  }
  if(Hex_bbase)
    while(Hex_bbase){
      Hex_bbinf = Hex_bbase->next;
      free(Hex_bbase);
      Hex_bbase = Hex_bbinf;
    }
  Hex_bbase = (Basis *)NULL;
}


/* set up pointer structure for basis */
Basis *Tri_mem_base(int L, int qa, int qb, int qc)
{
  register int i,j;
  Basis *b = (Basis *)calloc(1,sizeof(Basis));
  Mode    *m;

  b->id = L;
  b->qa = qa;
  b->qb = qb;
  b->qc = qc;

  /* set up basis so that all mode structure *
   * are consequative from Basis->v[0]       */

  b->vert = m = mvector(0,NTri_verts + NTri_edges*(L-2) + (L-3)*(L-2)/2);

  m += NTri_verts;

  /* set up edges */
  b->edge = (Mode **)calloc(NTri_edges,sizeof(Mode *));
  if(L>2) for(i = 0 ; i < NTri_edges; ++i,m+=(L-2))  b->edge[i] = m;

  /* set up faces */
  b->face = (Mode ***)calloc(NTri_faces,sizeof(Mode **));
  if(L>3){
    for(i = 0; i < NTri_faces; ++i){
      b->face[i] = (Mode **)calloc((L-3),sizeof(Mode *));
      for(j = 0; j < L-3; m+= (L-3-j),++j)
  b->face[i][j] = m;
    }
  }

  return b;
}


//  vertex ==           1 mode
//  edge   ==         L-2 modes
//  face   == (L-2)*(L-2) modes

/* set up pointer structure for basis */
Basis *Quad_mem_base(int L, int qa, int qb, int qc)
{
  register int i,j;
  Basis *b = (Basis *)malloc(sizeof(Basis));
  Mode    *m;

  b->id = L;
  b->qa = qa;
  b->qb = qb;
  b->qc = qc;

  /* set up basis so that all mode structure *
   * are consequative from Basis->v[0]       */

  b->vert = m = mvector(0,NQuad_verts + NQuad_edges*(L-2) + (L-2)*(L-2));

  m += NQuad_verts;

  /* set up edges */
  b->edge = (Mode **)malloc(NQuad_edges*sizeof(Mode *));
  if(L>2) for(i = 0 ; i < NQuad_edges; ++i,m+=(L-2))  b->edge[i] = m;

  /* set up faces */
  b->face = (Mode ***)malloc(NQuad_faces*sizeof(Mode **));
  if(L>2){
    for(i = 0; i < NQuad_faces; ++i){
      b->face[i] = (Mode **)malloc((L-2)*sizeof(Mode *));
      for(j = 0; j < L-2; m+= (L-2),++j)
  b->face[i][j] = m;
    }
  }

  return b;
}


/* set up pointer structure for basis */
Basis *Tet_mem_base(int L, int qa, int qb, int qc)
{
  register int i,j;
  Basis *b = (Basis *)calloc(1,sizeof(Basis));
  Mode    *m;

  b->id = L;
  b->qa = qa;
  b->qb = qb;
  b->qc = qc;

  /* set up basis so that all mode structure *
   * are consequative from Basis->v[0]       */

  b->vert = m = mvector(0,NTet_verts + NTet_edges*(L-2) +
      NTet_faces*(L-3)*(L-2)/2 + (L-4)*(L-3)*(L-2)/6-1);

  m += NTet_verts;

  /* set up edges */
  b->edge = (Mode **)malloc(NTet_edges*sizeof(Mode *));
  if(L>2) for(i = 0 ; i < NTet_edges; ++i,m+=(L-2))  b->edge[i] = m;

  /* set up faces */
  b->face = (Mode ***)malloc(NTet_faces*sizeof(Mode **));
  if(L>3){
    for(i = 0; i < NTet_faces; ++i){
      b->face[i] = (Mode **)malloc((L-3)*sizeof(Mode *));
      for(j = 0; j < L-3; m+= (L-3-j),++j)
  b->face[i][j] = m;
    }
  }

  /* set up interior (3D) */
  if(L>4){
    b->intr = (Mode ***)malloc((L-4)*sizeof(Mode **));
    for(i = 0; i < L-4; ++i){
      b->intr[i] = (Mode **)malloc((L-4)*sizeof(Mode *));
      for(j = 0; j < L-4-i; m+= (L-4-i-j),++j)
  b->intr[i][j] = m;
    }
  }

  return b;
}



/* set up pointer structure for basis */
Basis *Pyr_mem_base(int L, int qa, int qb, int qc)
{
  register int i,j;
  Basis *b = (Basis *)calloc(1,sizeof(Basis));
  Mode    *m;

  b->id = L;
  b->qa = qa;
  b->qb = qb;
  b->qc = qc;

  /* set up basis so that all mode structure *
   * are consequative from Basis->v[0]       */

  b->vert = m = mvector(0,NPyr_verts +
      NPyr_edges*(L-2) +
       (L-2)*(L-2) +
      4*(L-3)*(L-2)/2 +
      (L-3)*(L-2)*(L-1)/6);  // interior

  m += NPyr_verts;

  /* set up edges */
  b->edge = (Mode **)malloc(NPyr_edges*sizeof(Mode *));
  if(L>2)
    for(i = 0 ; i < NPyr_edges; ++i,m+=(L-2))
      b->edge[i] = m;

  /* set up faces */
  b->face = (Mode ***)malloc(NPyr_faces*sizeof(Mode **));
  if(L>2){

    b->face[0] = (Mode **)malloc((L-2)*sizeof(Mode *));
    for(j = 0; j < L-2; m+= (L-2),++j)
      b->face[0][j] = m;

    for(i=1;i<NPyr_faces;++i){
      b->face[i] = (Mode **)malloc((L-3)*sizeof(Mode *));
      for(j = 0; j < L-3; m+= (L-3-j),++j)
  b->face[i][j] = m;
    }
  }

  /* set up interior (3D) */  // here
  if(L>3){
    b->intr = (Mode ***)malloc((L-3)*sizeof(Mode **));

    for(i = 0; i < L-3; ++i){
      b->intr[i] = (Mode **)malloc((L-3)*sizeof(Mode *));
      for(j = 0; j < L-3-i;  m += L-3-i-j, ++j){
  b->intr[i][j] = m;
      }
    }
  }

  return b;
}


/* set up pointer structure for basis */
Basis *Prism_mem_base(int L, int qa, int qb, int qc)
{
  register int i,j;
  Basis *b = (Basis *)calloc(1,sizeof(Basis));
  Mode    *m;

  b->id = L;
  b->qa = qa;
  b->qb = qb;
  b->qc = qc;

  /* set up basis so that all mode structure *
   * are consequative from Basis->v[0]       */

  b->vert = m = mvector(0,NPrism_verts +
      NPrism_edges*(L-2) +
      3*(L-2)*(L-2) +
      2*(L-3)*(L-2)/2 +
      (L-2)*(L-3)*(L-2)/2-1);

  m += NPrism_verts;

  /* set up edges */
  b->edge = (Mode **)calloc(NPrism_edges,sizeof(Mode *));
  if(L>2)
    for(i = 0 ; i < NPrism_edges; ++i,m+=(L-2))
      b->edge[i] = m;

  /* set up faces */
  b->face = (Mode ***)calloc(NPrism_faces,sizeof(Mode **));
  if(L>2){

    b->face[0] = (Mode **)malloc((L-2)*sizeof(Mode *));
    for(j = 0; j < L-2; m+= (L-2),++j)
      b->face[0][j] = m;

    if(L>3){
      b->face[1] = (Mode **)malloc((L-3)*sizeof(Mode *));
      for(j = 0; j < L-3; m+= (L-3-j),++j)
  b->face[1][j] = m;
    }
    b->face[2] = (Mode **)malloc((L-2)*sizeof(Mode *));
    for(j = 0; j < L-2; m+= (L-2),++j)
      b->face[2][j] = m;
    if(L>3){
      b->face[3] = (Mode **)malloc((L-3)*sizeof(Mode *));
      for(j = 0; j < L-3; m+= (L-3-j),++j)
  b->face[3][j] = m;
    }
    b->face[4] = (Mode **)malloc((L-2)*sizeof(Mode *));
    for(j = 0; j < L-2; m+= (L-2),++j)
      b->face[4][j] = m;

  }

  /* set up interior (3D) */
  if(L>3){
    b->intr = (Mode ***)malloc((L-3)*sizeof(Mode **));

    for(i = 0; i < L-3; ++i){
      b->intr[i] = (Mode **)malloc((L-2)*sizeof(Mode *));
      for(j = 0; j < L-2; m+= L-3-i,++j)
  b->intr[i][j] = m;
    }
  }

  return b;
}


/* set up pointer structure for basis */
Basis *Hex_mem_base(int L, int qa, int qb, int qc)
{
  register int i,j;
  Basis *b = (Basis *)calloc(1,sizeof(Basis));
  Mode    *m;

  b->id = L;
  b->qa = qa;
  b->qb = qb;
  b->qc = qc;

  /* set up basis so that all mode structure *
   * are consequative from Basis->v[0]       */

  b->vert = m = mvector(0,NHex_verts + NHex_edges*(L-2) + NHex_faces*(L-2)*(L-2)
      + (L-2)*(L-2)*(L-2)-1);

  m += NHex_verts;

  /* set up edges */
  b->edge = (Mode **)malloc(NHex_edges*sizeof(Mode *));
  if(L>2) for(i = 0 ; i < NHex_edges; ++i,m+=(L-2))  b->edge[i] = m;

  /* set up faces */
  b->face = (Mode ***)malloc(NHex_faces*sizeof(Mode **));
  if(L>2){
    for(i = 0; i < NHex_faces; ++i){
      b->face[i] = (Mode **)malloc((L-2)*sizeof(Mode *));
      for(j = 0; j < L-2; m+= (L-2),++j)
  b->face[i][j] = m;
    }
  }

  /* set up interior (3D) */
  if(L>2){
    b->intr = (Mode ***)malloc((L-2)*sizeof(Mode **));

    for(i = 0; i < L-2; ++i){
      b->intr[i] = (Mode **)malloc((L-2)*sizeof(Mode *));
      for(j = 0; j < L-2; m+= L-2,++j)
  b->intr[i][j] = m;
    }
  }

  return b;
}


/*----------------------------------------------------------------------------*
 * Set up memory for modal storage. Not all modes need to have independent    *
 * storage since the modes are repeated. The independent terms in 'a' 'b'     *
 *                                                                            *
 *      'a'                                 'b'                               *
 *  1.0                               1.0  (3d only )                         *
 *  (1+a)/2                           (1+b)/2                                 *
 *  (1-a)/2                           (1-b)/2                                 *
 *  (1-a)/2 (1+a)/2                   (1-b)/2 (1+b)/2 P_{m-1}^{1,1}(b)        *
 *  (1-a)/2 (1+a)/2 P_1^{1,1}(a)      {(1-b)/2}^l                             *
 *  (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)  {(1-b)/2}^2 (1+b)/2 P_{m-1}^{3,1}(b)    *
 *                                    {(1-b)/2}^l (1+b)/2 P_{m-1}^{2l-1,1}(b) *
 *                                                                            *
 * Each polynomial in a,b,c are stored contiguously in this order and then    *
 * the appropriate pointers are assigned to the correct position in this      *
 * vector. This allows matrix operations in sumfactorisation routines         *
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 * this routine sets up the storage in continuous vectors as described above *
 * it also sets up the pointer system so that the modes can be accessed      *
 * individually                                                              *
 *---------------------------------------------------------------------------*/

void Tri_mem_modes(Basis *b){
  register int i,j;
  const    int L = b->id, qa = b->qa, qb = b->qb;
  double   *s;
  Mode     *v = b->vert, **e = b->edge, ***f = b->face;

  /* allocate memory for 'a' polynomials */
  s = dvector(0,qa*(L+1)-1);

  v[2].a = s; s += qa;

  /* set up pointers */
  v[1].a = s; s += qa;
  v[0].a = s; s += qa;

  for(i = 0; i < L-2; ++i, s+=qa){
    e[0][i].a = s;
    e[1][i].a = v[1].a;
    e[2][i].a = v[0].a;
  }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-3-i; ++j){
      f[0][i][j].a = e[0][i].a;
    }

  /* allocation for b */
  s = dvector(0,qb*(L+(L-2)*(L-1)/2)-1);

  v[2].b = s;              s += qb;
  v[0].b = s;  v[1].b = s; s += qb;

  for(i = 0; i < L-2; ++i, s+=qb){
    e[1][i].b = s;
    e[2][i].b = e[1][i].b;
  }

  for(i = 0; i < L-2; ++i){
    e[0][i].b = s; s+=qb;
    for(j = 0; j < L-3-i; ++j,s+=qb){
      f[0][i][j].b = s;
    }
  }
}




/*--------------------------------------------------------------------------*
 * Set up memory for modal storage. Not all modes need to have independent  *
 * storage since the modes are repeated. The independent terms in 'a' 'b'   *
 *      'a'                                 'b'                             *
 *  (1+a)/2                           (1+b)/2                               *
 *  (1-a)/2                           (1-b)/2                               *
 *  (1-a)/2 (1+a)/2                   (1-b)/2 (1+b)/2                       *
 *  (1-a)/2 (1+a)/2 P_1^{1,1}(a)      (1-b)/2 (1+b)/2 P_1^{1,1}(b)          *
 *  (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)  (1-b)/2 (1+b)/2 P_{l-2}^{1,1}(b)      *
 *                                                                          *
 *                                                                          *
 * Each polynomial in a,b,c are stored contiguously in this order and then  *
 * the appropriate pointers are assigned to the correct position in this    *
 * vector. This allows matrix operations in sumfactorisation routines       *
 *--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*
 * this routine sets up the storage in continuous vectors as described above*
 * it also sets up the pointer system so that the modes can be accessed     *
 * individually                                                             *
 *--------------------------------------------------------------------------*/

// DONE -- checked

void Quad_mem_modes(Basis *b){
  register int i,j;
  const    int L = b->id, qa = b->qa, qb = b->qb;
  double   *s;
  Mode     *v = b->vert, **e = b->edge, ***f = b->face;

  // count up number of different modes
  /* allocate memory for 'a' polynomials */
  s = dvector(0,qa*L-1);

  /* set up pointers */
  v[1].a = v[2].a = s; s += qa;
  v[0].a = v[3].a = s; s += qa;

  for(i = 0; i < L-2; ++i, s+=qa){
    e[0][i].a = s;
    e[1][i].a = v[1].a;
    e[2][i].a = s;
    e[3][i].a = v[0].a;
  }

  for(i = 0; i < L-2; ++i)
    for(j = 0; j < L-2; ++j)
      f[0][i][j].a = e[0][i].a;

  /* allocation for b */
  s = dvector(0,qb*L-1);

  v[3].b = v[2].b = s; s += qb;
  v[1].b = v[0].b = s; s += qb;

  for(i = 0; i < L-2; ++i, s+=qb){
    e[0][i].b = v[1].b;
    e[1][i].b = s;
    e[2][i].b = v[2].b;
    e[3][i].b = s;
  }

  for(i = 0; i < L-2; ++i)
    for(j = 0; j < L-2; ++j)
      f[0][i][j].b = e[1][j].b;

}


/*----------------------------------------------------------------------------*
 * Set up memory for modal storage. Not all modes need to have independent    *
 * storage since the modes are repeated. The independent terms in 'a' 'b'     *
 * and 'c' are:                                                               *
 *                                                                            *
 *      'a'                                 'b'                               *
 *  1.0                               1.0  (3d only )                         *
 *  (1+a)/2                           (1+b)/2                                 *
 *  (1-a)/2                           (1-b)/2                                 *
 *  (1-a)/2 (1+a)/2                   (1-b)/2 (1+b)/2 P_{m-1}^{1,1}(b)        *
 *  (1-a)/2 (1+a)/2 P_1^{1,1}(a)      {(1-b)/2}^l                             *
 *  (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)  {(1-b)/2}^2 (1+b)/2 P_{m-1}^{3,1}(b)    *
 *                                    {(1-b)/2}^l (1+b)/2 P_{m-1}^{2l-1,1}(b) *
 *                                                                            *
 *                             'c'                                            *
 *                  (1+c)/2                                                   *
 *                  (1-c)/2                                                   *
 *                  {(1-c)/2}^2                                               *
 *                  {(1-c)/2}^{l+m}                                           *
 *                  (1-c)/2 (1+c)/2 P_{n-1}^{1,1}(c)                          *
 *                  {(1-c)/2}^2 (1+c)/2 P_{n-1}^{3,1}(c)                      *
 *                  {(1-c)/2}^{l+m} (1+c)/2 P_{n-1}^{2L+2m-1,1}(c)            *
 *                                                                            *
 * Each polynomial in a,b,c are stored contiguously in this order and then    *
 * the appropriate pointers are assigned to the correct position in this      *
 * vector. This allows matrix operations in sumfactorisation routines         *
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 * this routine sets up the storage in continuous vectors as described above *
 * it also sets up the pointer system so that the modes can be accessed      *
 * individually                                                              *
 *---------------------------------------------------------------------------*/

void Tet_mem_modes(Basis *b){
  register int i,j;
  const    int L = b->id, qa = b->qa, qb = b->qb;
  double   *s;
  Mode     *v = b->vert, **e = b->edge, ***f = b->face;

  register int k;
  int      qc = b->qc;
  Mode     ***in = b->intr;

  /* allocate memory for 'a' polynomials */
  s = dvector(0,qa*(L+1)-1);

  v[2].a = s; s += qa;

  /* set up pointers */
  v[1].a = s; s += qa;
  v[0].a = s; s += qa;
  v[3].a = v[2].a;

  for(i = 0; i < L-2; ++i, s+=qa){
    e[0][i].a = s;
    e[1][i].a = v[1].a;
    e[2][i].a = v[0].a;

    e[3][i].a = v[0].a;
    e[4][i].a = v[1].a;
    e[5][i].a = v[2].a;

  }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-3-i; ++j){
      f[0][i][j].a = e[0][i].a;
      f[1][i][j].a = e[0][i].a;
      f[2][i][j].a = e[1][i].a;
      f[3][i][j].a = e[2][i].a;
    }

  for(i = 0; i < L-4; ++i)
    for(j = 0; j < L-4-i; ++j)
      for(k = 0; k < L-4-i-j; ++k)
  in[i][j][k].a = f[0][i][j].a;

  /* allocation for b */

  s = dvector(0,qb*(L+(L-2)*(L-1)/2+1)-1);

  /* set up modal pointers */
  v[3].b = s; s += qb;
  v[2].b = s; s += qb;
  v[0].b = v[1].b = s; s+=qb;

  for(i = 0; i < L-2; ++i, s+=qb){
    e[1][i].b = s;
    e[2][i].b = e[1][i].b;
    e[3][i].b = v[0].b;
    e[4][i].b = v[1].b;
    e[5][i].b = v[2].b;
  }

  for(i = 0; i < L-2; ++i){
    e[0][i].b = s; s+=qb;
    for(j = 0; j < L-3-i; ++j,s+=qb){
      f[0][i][j].b = s;

      f[1][i][j].b = e[0][i].b;
      f[2][i][j].b = e[1][i].b;
      f[3][i][j].b = e[2][i].b;
    }
  }

  for(i = 0; i < L-4; ++i)
    for(j = 0; j < L-4-i; ++j)
      for(k = 0; k < L-4-i-j; ++k)
  in[i][j][k].b = f[0][i][j].b;

  /* allocation for c */
  s = dvector(0,qc*(L+(L-2)*(L-1)/2)-1);

  v[3].c = s; s += qc;
  v[0].c = v[1].c = v[2].c = s; s += qc;

  for(i = 0; i < L-2; ++i, s+=qc){
    e[0][i].c = s;
    e[1][i].c = e[0][i].c;
    e[2][i].c = e[0][i].c;
  }

  for(i = 0; i < L-2; ++i, s+=qc){
    e[3][i].c = s;
    e[4][i].c = e[3][i].c;
    e[5][i].c = e[3][i].c;
  }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-3-i; ++j,s+=qc){
      f[1][i][j].c = s;
      f[2][i][j].c = f[1][i][j].c;
      f[3][i][j].c = f[2][i][j].c;
    }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < (L-3-i); ++j)
      f[0][i][j].c = e[0][j+i+1].c;

  for(i = 0; i < L-4; ++i)
    for(j = 0; j < L-4-i; ++j)
      for(k = 0; k < L-4-i-j; ++k)
  in[i][j][k].c = f[1][j+i+1][k].c;

}



/*----------------------------------------------------------------------------*
 * Set up memory for modal storage. Not all modes need to have independent    *
 * storage since the modes are repeated. The independent terms in 'a' 'b'     *
 * and 'c' are:                                                               *
 *                                                                            *
 *      'a'                                 'b'                               *
 *  (1-a)/2
 *  (1+a)/2                           (1-b)/2                                 *
 *  (1-a)/2 (1+a)/2                   (1-b)/2 (1+b)/2 P_{m-1}^{1,1}(b)        *
 *  (1-a)/2 (1+a)/2 P_1^{1,1}(a)      {(1-b)/2}^l                             *
 *  (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)  {(1-b)/2}^2 (1+b)/2 P_{m-1}^{3,1}(b)    *
 *                                    {(1-b)/2}^l (1+b)/2 P_{m-1}^{2l-1,1}(b) *
 *                                                                            *
 *                             'c'                                            *
 *                  (1+c)/2                                                   *
 *                  (1-c)/2                                                   *
 *                  {(1-c)/2}^2                                               *
 *                  {(1-c)/2}^{l+m}                                           *
 *                  (1-c)/2 (1+c)/2 P_{n-1}^{1,1}(c)                          *
 *                  {(1-c)/2}^2 (1+c)/2 P_{n-1}^{3,1}(c)                      *
 *                  {(1-c)/2}^{l+m} (1+c)/2 P_{n-1}^{2L+2m-1,1}(c)            *
 *                                                                            *
 * Each polynomial in a,b,c are stored contiguously in this order and then    *
 * the appropriate pointers are assigned to the correct position in this      *
 * vector. This allows matrix operations in sumfactorisation routines         *
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 * this routine sets up the storage in continuous vectors as described above *
 * it also sets up the pointer system so that the modes can be accessed      *
 * individually                                                              *
 *---------------------------------------------------------------------------*/

void Pyr_mem_modes(Basis *b){
  register int i,j,k;
  const    int L = b->id, qa = b->qa,  qc = b->qc;
  double   *sa, *onea, *v1a, *v2a, *eia;
  double   *sc, *onec, *v1c, *v2c, *eic;

  Mode     *v = b->vert, **e = b->edge, ***f = b->face;
  Mode     ***intr = b->intr;

  /* allocate memory for 'a' polynomials */
  sa = dvector(0,qa*(L+1)-1);
  sc  = dvector(0,qc*(1+L+L-2+
          (L-2)*(L-2)+(L-3)*(L-2)/2+
          (L-3)*(L-2)*(L-1)/6)-1);  // interior

  onea = sa; sa += qa;         onec = sc; sc+=qc;
  v1a  = sa; sa += qa;          v1c = sc; sc+=qc;
  eia  = sa; sa += (L-2)*qa;
  v2a  = sa; sa += qa;          v2c = sc; sc+=qc;

  // done
  v[0].a = v1a;  v[0].b = v1a; v[0].c = v1c;
  v[1].a = v2a;  v[1].b = v1a; v[1].c = v1c;
  v[2].a = v2a;  v[2].b = v2a; v[2].c = v1c;
  v[3].a = v1a;  v[3].b = v2a; v[3].c = v1c;
  v[4].a = onea; v[4].b = onea; v[4].c = v2c;

  sa = eia;

  for(i = 0; i < L-2; ++i, sa+=qa, sc += qc){
    eia = sa;  eic = sc;

    e[0][i].a = eia;  e[0][i].b = v1a;
    e[1][i].a = v2a;  e[1][i].b = eia;
    e[2][i].a = eia;  e[2][i].b = v2a;
    e[3][i].a = v1a;  e[3][i].b = eia;
    e[4][i].a = v1a;  e[4][i].b = v1a;  e[4][i].c = eic;
    e[5][i].a = v2a;  e[5][i].b = v1a;  e[5][i].c = eic;
    e[6][i].a = v2a;  e[6][i].b = v2a;  e[6][i].c = eic;
    e[7][i].a = v1a;  e[7][i].b = v2a;  e[7][i].c = eic;
  }

  for(i = 0; i < L-2; ++i, sc += qc)
    e[3][i].c = e[2][i].c = e[1][i].c = e[0][i].c = sc;

  for(i = 0; i < L-2; ++i)
    for(j = 0; j < L-2; ++j,sc+=qc){
      f[0][i][j].a = e[0][j].a; f[0][i][j].b = e[0][i].a; f[0][i][j].c = sc;
    }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-3-i; ++j, sc += qc){
      f[1][i][j].a = e[0][i].a; f[1][i][j].b = v1a; f[1][i][j].c = sc;
      f[3][i][j].a = e[0][i].a; f[3][i][j].b = v2a; f[3][i][j].c = sc;

      f[2][i][j].a = v2a; f[2][i][j].b = e[0][i].a; f[2][i][j].c = sc;
      f[4][i][j].a = v1a; f[4][i][j].b = e[0][i].a; f[4][i][j].c = sc;
    }

  // interior
  // here

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-3-i; ++j)
      for(k = 0; k < L-3-i-j; ++k,sc+=qc){
  intr[i][j][k].a = e[0][i].a;
  intr[i][j][k].b = e[0][j].a;
  intr[i][j][k].c = sc;
      }
}


/*----------------------------------------------------------------------------*
 * Set up memory for modal storage. Not all modes need to have independent    *
 * storage since the modes are repeated. The independent terms in 'a' 'b'     *
 * and 'c' are:                                                               *
 *                                                                            *
 *      'a'                                 'b'                               *
 *  (1-a)/2
 *  (1+a)/2                           (1-b)/2                                 *
 *  (1-a)/2 (1+a)/2                   (1-b)/2 (1+b)/2 P_{m-1}^{1,1}(b)        *
 *  (1-a)/2 (1+a)/2 P_1^{1,1}(a)      {(1-b)/2}^l                             *
 *  (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)  {(1-b)/2}^2 (1+b)/2 P_{m-1}^{3,1}(b)    *
 *                                    {(1-b)/2}^l (1+b)/2 P_{m-1}^{2l-1,1}(b) *
 *                                                                            *
 *                             'c'                                            *
 *                  (1+c)/2                                                   *
 *                  (1-c)/2                                                   *
 *                  {(1-c)/2}^2                                               *
 *                  {(1-c)/2}^{l+m}                                           *
 *                  (1-c)/2 (1+c)/2 P_{n-1}^{1,1}(c)                          *
 *                  {(1-c)/2}^2 (1+c)/2 P_{n-1}^{3,1}(c)                      *
 *                  {(1-c)/2}^{l+m} (1+c)/2 P_{n-1}^{2L+2m-1,1}(c)            *
 *                                                                            *
 * Each polynomial in a,b,c are stored contiguously in this order and then    *
 * the appropriate pointers are assigned to the correct position in this      *
 * vector. This allows matrix operations in sumfactorisation routines         *
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 * this routine sets up the storage in continuous vectors as described above *
 * it also sets up the pointer system so that the modes can be accessed      *
 * individually                                                              *
 *---------------------------------------------------------------------------*/

void Prism_mem_modes(Basis *b){
  register int i,j,k;
  const    int L = b->id, qa = b->qa,  qc = b->qc;
  double   *sa, *onea, *v1a, *v2a, *eia;
  double   *sc, *onec, *v1c, *v2c, *eic;

  Mode     *v = b->vert, **e = b->edge, ***f = b->face;
  Mode     ***intr = b->intr;

  /* allocate memory for 'a' polynomials */
  sa = dvector(0,qa*(L+1)-1);   sc  = dvector(0,qc*(1+L+L-2+(L-3)*(L-2)/2)-1);

  onea = sa; sa += qa;         onec = sc; sc+=qc;
  v1a  = sa; sa += qa;          v1c = sc; sc+=qc;
  eia  = sa; sa += (L-2)*qa;
  v2a  = sa; sa += qa;          v2c = sc; sc+=qc;

  // done
  v[0].a = v1a;  v[0].b = v1a; v[0].c = v1c;
  v[1].a = v2a;  v[1].b = v1a; v[1].c = v1c;
  v[2].a = v2a;  v[2].b = v2a; v[2].c = v1c;
  v[3].a = v1a;  v[3].b = v2a; v[3].c = v1c;
  v[4].a = onea; v[4].b = v1a; v[4].c = v2c;
  v[5].a = onea; v[5].b = v2a; v[5].c = v2c;

  sa = eia;

  for(i = 0; i < L-2; ++i, sa+=qa, sc += qc){
    eia = sa;  eic = sc;

    e[0][i].a = eia;  e[0][i].b = v1a;
    e[1][i].a = v2a;  e[1][i].b = eia;  e[1][i].c = v1c;
    e[2][i].a = eia;  e[2][i].b = v2a;
    e[3][i].a = v1a;  e[3][i].b = eia;  e[3][i].c = v1c;
    e[4][i].a = v1a;  e[4][i].b = v1a;  e[4][i].c = eic;
    e[5][i].a = v2a;  e[5][i].b = v1a;  e[5][i].c = eic;
    e[6][i].a = v2a;  e[6][i].b = v2a;  e[6][i].c = eic;
    e[7][i].a = v1a;  e[7][i].b = v2a;  e[7][i].c = eic;
    e[8][i].a = onea; e[8][i].b = eia;  e[8][i].c = v2c;
  }

  for(i = 0; i < L-2; ++i, sc += qc)
    e[2][i].c = e[0][i].c = sc;

  for(i = 0; i < L-2; ++i)
    for(j = 0; j < L-2; ++j){
      eia = e[0][j].a; eic = e[4][i].c;

      f[0][i][j].a = eia; f[0][j][i].b = eia; f[0][i][j].c = e[0][j].c;
      f[2][i][j].a = v2a; f[2][i][j].b = eia; f[2][i][j].c = eic;
      f[4][i][j].b = eia; f[4][i][j].a = v1a; f[4][i][j].c = eic;

    }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-3-i; ++j, sc += qc){
      f[1][i][j].a = e[0][i].a; f[1][i][j].b = v1a; f[1][i][j].c = sc;
      f[3][i][j].a = e[0][i].a; f[3][i][j].b = v2a; f[3][i][j].c = sc;
    }

  for(i = 0; i < L-3; ++i)
    for(j = 0; j < L-2; ++j)
      for(k = 0; k < L-3-i; ++k){
  intr[i][j][k].a = e[0][i].a;
  intr[i][j][k].b = e[0][j].a;
  intr[i][j][k].c = f[1][i][k].c;
      }
}

/*----------------------------------------------------------------------------*
 * Set up memory for modal storage. Not all modes need to have independent    *
 * storage since the modes are repeated. The independent terms in 'a' 'b'     *
 * and 'c' are:                                                               *
 *                                                                            *
 *      'a'                                 'b'                               *
 *  (1-a)/2
 *  (1+a)/2                           (1-b)/2                                 *
 *  (1-a)/2 (1+a)/2                   (1-b)/2 (1+b)/2 P_{m-1}^{1,1}(b)        *
 *  (1-a)/2 (1+a)/2 P_1^{1,1}(a)      {(1-b)/2}^l                             *
 *  (1-a)/2 (1+a)/2 P_{l-2}^{1,1}(a)  {(1-b)/2}^2 (1+b)/2 P_{m-1}^{3,1}(b)    *
 *                                    {(1-b)/2}^l (1+b)/2 P_{m-1}^{2l-1,1}(b) *
 *                                                                            *
 *                             'c'                                            *
 *                  (1+c)/2                                                   *
 *                  (1-c)/2                                                   *
 *                  {(1-c)/2}^2                                               *
 *                  {(1-c)/2}^{l+m}                                           *
 *                  (1-c)/2 (1+c)/2 P_{n-1}^{1,1}(c)                          *
 *                  {(1-c)/2}^2 (1+c)/2 P_{n-1}^{3,1}(c)                      *
 *                  {(1-c)/2}^{l+m} (1+c)/2 P_{n-1}^{2L+2m-1,1}(c)            *
 *                                                                            *
 * Each polynomial in a,b,c are stored contiguously in this order and then    *
 * the appropriate pointers are assigned to the correct position in this      *
 * vector. This allows matrix operations in sumfactorisation routines         *
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 * this routine sets up the storage in continuous vectors as described above *
 * it also sets up the pointer system so that the modes can be accessed      *
 * individually                                                              *
 *---------------------------------------------------------------------------*/

 void Hex_mem_modes(Basis *b){
  register int i,j,k;
  const    int L = b->id, qa = b->qa;
  double   *s, *v1, *v2, *ei, *eia, *eja;
  Mode     *v = b->vert, **e = b->edge, ***f = b->face;

  Mode     ***intr = b->intr;

  /* allocate memory for 'a' polynomials */

  s = dvector(0,qa*L-1);

  v1 = s; s+= qa;
  ei = s; s+= qa*(L-2);
  v2 = s; s+= qa;

  v[0].a = v1; v[0].b = v1; v[0].c = v1;
  v[1].a = v2; v[1].b = v1; v[1].c = v1;
  v[2].a = v2; v[2].b = v2; v[2].c = v1;
  v[3].a = v1; v[3].b = v2; v[3].c = v1;

  v[4].a = v1; v[4].b = v1; v[4].c = v2;
  v[5].a = v2; v[5].b = v1; v[5].c = v2;
  v[6].a = v2; v[6].b = v2; v[6].c = v2;
  v[7].a = v1; v[7].b = v2; v[7].c = v2;

  for(i = 0; i < L-2; ++i, ei+=qa){

    e[0][i].a = ei; e[0][i].b = v1; e[0][i].c = v1;
    e[1][i].b = ei; e[1][i].a = v2; e[1][i].c = v1;
    e[2][i].a = ei; e[2][i].b = v2; e[2][i].c = v1;
    e[3][i].b = ei; e[3][i].a = v1; e[3][i].c = v1;

    e[4][i].c = ei; e[4][i].a = v1; e[4][i].b = v1;
    e[5][i].c = ei; e[5][i].a = v2; e[5][i].b = v1;
    e[6][i].c = ei; e[6][i].a = v2; e[6][i].b = v2;
    e[7][i].c = ei; e[7][i].a = v1; e[7][i].b = v2;

    e[8][i].a  = ei; e[8][i].b  = v1; e[8][i].c  = v2;
    e[9][i].b  = ei; e[9][i].a  = v2; e[9][i].c  = v2;
    e[10][i].a = ei; e[10][i].b = v2; e[10][i].c = v2;
    e[11][i].b = ei; e[11][i].a = v1; e[11][i].c = v2;
  }

  for(i = 0; i < L-2; ++i)
    for(j = 0; j < L-2; ++j){
      eia = e[0][j].a; eja = e[0][i].a;

      f[0][i][j].a = eia; f[0][i][j].b = eja; f[0][i][j].c =  v1;
      f[1][i][j].a = eia; f[1][i][j].b =  v1; f[1][i][j].c = eja;
      f[2][i][j].a =  v2; f[2][i][j].b = eia; f[2][i][j].c = eja;
      f[3][i][j].a = eia; f[3][i][j].b =  v2; f[3][i][j].c = eja;
      f[4][i][j].a =  v1; f[4][i][j].b = eia; f[4][i][j].c = eja;
      f[5][i][j].a = eia; f[5][i][j].b = eja; f[5][i][j].c =  v2;
    }

  for(i = 0; i < L-2; ++i)
    for(j = 0; j < L-2; ++j)
      for(k = 0; k < L-2; ++k){
  intr[i][j][k].a = e[0][k].a;
  intr[i][j][k].b = e[0][j].a;
  intr[i][j][k].c = e[0][i].a;
      }
}

void  Tri_set_vertices(Mode *v, int q, char dir){
  double   one = 1.0,*z,*w;
  getzw(q,&z,&w,dir);

  switch (dir){
  case 'a':
    dvsub(q,&one,0,z,1,v[0].a,1);
    dscal(q,0.5,v[0].a,1);
    dsadd(q,one,z,1,v[1].a,1);
    dscal(q,0.5,v[1].a,1);
    dfill(q,one,v[2].a,1);
    break;
  case 'b':
    dvsub(q,&one,0,z,1,v[0].b,1);
    dscal(q,0.5,v[0].b,1);
    dsadd(q,one,z,1,v[2].b,1);
    dscal(q,0.5,v[2].b,1);
    break;
  }
}



void  Quad_set_vertices(Basis *b, int q, char dir){
  double   one = 1.0,*z,*w;
  Mode     *v = b->vert;
  getzw(q,&z,&w,'a');

  switch (dir){
  case 'a':
    dvsub(q,&one,0,z,1,v[0].a,1);
    dscal(q,0.5,v[0].a,1);
    dsadd(q,one,z,1,v[1].a,1);
    dscal(q,0.5,v[1].a,1);
    break;
  case 'b':
    dvsub(q,&one,0,z,1,v[1].b,1);
    dscal(q,0.5,v[1].b,1);
    dsadd(q,one,z,1,v[3].b,1);
    dscal(q,0.5,v[3].b,1);
    break;
  }
}



void  Tet_set_vertices(Mode *v, int q, char dir){
  double   one = 1.0,*z,*w;
  getzw(q,&z,&w,dir);

  switch (dir){
  case 'a':
    dvsub(q,&one,0,z,1,v[0].a,1);
    dsmul(q,0.5,v[0].a,1,v[0].a,1);
    dsadd(q,one,z,1,v[1].a,1);
    dsmul(q,0.5,v[1].a,1,v[1].a,1);
    dfill(q,one,v[2].a,1);
    break;
  case 'b':
    dvsub(q,&one,0,z,1,v[0].b,1);
    dsmul(q,0.5,v[0].b,1,v[0].b,1);
    dsadd(q,one,z,1,v[2].b,1);
    dsmul(q,0.5,v[2].b,1,v[2].b,1);

    dfill(q,one,v[3].b,1);
    break;
  case 'c':
    dvsub(q,&one,0 ,z,1,v[0].c,1);
    dsmul(q,0.5,v[0].c,1,v[0].c,1);

    dsadd(q,one,z,1,v[3].c,1);
    dsmul(q,0.5,v[3].c,1,v[3].c,1);

    break;
  }
}


void  Pyr_set_vertices(Mode *v, int q, char dir){
  double   one = 1.0,*z,*w;

  switch (dir){
  case 'a':
    getzw(q,&z,&w,'a');

    dvsub(q,&one,0,z,1,v[0].a,1);
    dscal(q,0.5,v[0].a,1);         // (1-a)/2
    dsadd(q,one,z,1,v[1].a,1);
    dscal(q,0.5,v[1].a,1);         // (1+a)/2

    dfill(q,one,v[4].a,1);         //  1
    break;
  case 'b':
    // already set up in 'a'
    break;
  case 'c':
    getzw(q,&z,&w,'c');

    dvsub(q,&one,0,z,1,v[0].c,1);
    dscal(q,0.5,v[0].c,1);          // (1-c)/2
    dsadd(q,one,z,1,v[4].c,1);
    dscal(q,0.5,v[4].c,1);          // (1+c)/2

    break;
  }
}

void  Prism_set_vertices(Mode *v, int q, char dir){
  double   one = 1.0,*z,*w;

  switch (dir){
  case 'a':
    getzw(q,&z,&w,'a');

    dvsub(q,&one,0,z,1,v[0].a,1);
    dscal(q,0.5,v[0].a,1);         // (1-a)/2
    dsadd(q,one,z,1,v[1].a,1);
    dscal(q,0.5,v[1].a,1);         // (1+a)/2

    dfill(q,one,v[4].a,1);         //  1
    break;
  case 'b':
    // already set up in 'a'
    break;
  case 'c':
    getzw(q,&z,&w,'b');

    dvsub(q,&one,0,z,1,v[0].c,1);
    dscal(q,0.5,v[0].c,1);          // (1-c)/2
    dsadd(q,one,z,1,v[4].c,1);
    dscal(q,0.5,v[4].c,1);          // (1+c)/2

    break;
  }
}



void  Hex_set_vertices(Mode *v, int q, char dir){
  double   one = 1.0,*z,*w;
  getzw(q,&z,&w,'a');

  switch (dir){
  case 'a':
    dvsub(q,&one,0,z,1,v[0].a,1);
    dsmul(q,0.5,v[0].a,1,v[0].a,1);
    dsadd(q,one,z,1,v[1].a,1);
    dsmul(q,0.5,v[1].a,1,v[1].a,1);
    break;
  case 'b':
    break;
  case 'c':
    break;
  }
}



void Tri_set_edges(Basis *b, int q, char dir){
  register int i;
  int      L = b->id-2;
  double   *z,*w;
  Mode     *v = b->vert, **e = b->edge;

  getzw(q,&z,&w,dir);

  switch (dir){
  case 'a':
    dvmul(q,v[0].a,1,v[1].a,1,e[0][0].a,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[0][i].a,i,1.0,1.0);
      dvmul (q,e[0][0].a,1,e[0][i].a,1,e[0][i].a,1);
    }
    break;
  case 'b':
    dvmul(q,v[0].b,1,v[0].b,1,e[0][0].b,1);
    dvmul(q,v[1].b,1,v[2].b,1,e[1][0].b,1);
    for(i = 1; i < L; ++i){
      dvmul (q,e[0][i-1].b,1,v[0].b,1,e[0][i].b,1);
      jacobf(q,z,e[1][i].b,i,1.0,1.0);
      dvmul (q,e[1][0].b,1,e[1][i].b,1,e[1][i].b,1);
    }
    break;
  }
}

void Quad_set_edges(Basis *b, int q, char dir){
  register int i;
  int      L = b->id-2;
  double   *z,*w;
  Mode     *v = b->vert, **e = b->edge;

  getzw(q,&z,&w,'a');

  switch (dir){
  case 'a':
    dvmul(q,v[0].a,1,v[1].a,1,e[0][0].a,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[0][i].a,i,1.0,1.0);
      dvmul (q,e[0][0].a,1,e[0][i].a,1,e[0][i].a,1);
    }
    break;
  case 'b':
    dvmul(q,v[0].b,1,v[2].b,1,e[1][0].b,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[1][i].b,i,1.0,1.0);
      dvmul (q,e[1][0].b,1,e[1][i].b,1,e[1][i].b,1);
    }
    break;
  }
}

void Tet_set_edges(Basis *b, int q, char dir){
  register int i;
  int      L = b->id-2;
  double   *z,*w;
  Mode     *v = b->vert, **e = b->edge;

  double   alpha = 1.;
  double    beta = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  getzw(q,&z,&w,dir);

  switch (dir){
  case 'a':
    dvmul(q,v[0].a,1,v[1].a,1,e[0][0].a,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[0][i].a,i,alpha,beta);
      dvmul (q,e[0][0].a,1,e[0][i].a,1,e[0][i].a,1);
    }
    break;
  case 'b':
    dvmul(q,v[0].b,1,v[0].b,1,e[0][0].b,1);
    dvmul(q,v[1].b,1,v[2].b,1,e[1][0].b,1);
    for(i = 1; i < L; ++i){
      dvmul (q,e[0][i-1].b,1,v[0].b,1,e[0][i].b,1);

      jacobf(q,z,e[1][i].b,i,alpha,beta);
      dvmul (q,e[1][0].b,1,e[1][i].b,1,e[1][i].b,1);
    }
    break;
  case 'c':
    dvmul(q,v[0].c,1,v[0].c,1,e[0][0].c,1);
    dvmul(q,v[2].c,1,v[3].c,1,e[3][0].c,1);
    for(i = 1; i < L; ++i){
      dvmul (q,e[0][i-1].c,1,v[0].c,1,e[0][i].c,1);

      jacobf(q,z,e[3][i].c,i,alpha,beta);
      dvmul (q,e[3][0].c,1,e[3][i].c,1,e[3][i].c,1);
    }
    break;
  }
}



void Pyr_set_edges(Basis *b, int q, char dir){
  register int i;
  int      L = b->id-2;
  double   *z,*w;
  Mode     *v = b->vert, **e = b->edge;

  double alpha = 1., beta = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  switch (dir){
  case 'a':
    getzw(q,&z,&w,'a');

    dvmul(q,v[0].a,1,v[1].a,1,e[0][0].a,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[0][i].a,i,alpha,beta);
      dvmul (q,e[0][0].a,1,e[0][i].a,1,e[0][i].a,1);
    }
    break;
  case 'b':
    break;
  case 'c':
    getzw(q,&z,&w,'c');

    //    dfill(q,one,e[1][0].c, 1);
    dvmul(q, v[0].c, 1, v[0].c, 1, e[0][0].c, 1);

    for(i=1;i<L;++i)
      dvmul(q, v[0].c, 1, e[0][i-1].c, 1, e[0][i].c, 1);

    dvmul(q,v[0].c,1,v[4].c,1,e[4][0].c,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[4][i].c,i,alpha,beta);
      dvmul (q,e[4][0].c,1,e[4][i].c,1,e[4][i].c,1);
    }

    break;
  }
}


void Prism_set_edges(Basis *b, int q, char dir){
  register int i;
  int      L = b->id-2;
  double   *z,*w;
  Mode     *v = b->vert, **e = b->edge;

  double   alpha = 1.;
  double    beta = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  switch (dir){
  case 'a':
    getzw(q,&z,&w,'a');

    dvmul(q,v[0].a,1,v[1].a,1,e[0][0].a,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[0][i].a,i,alpha,beta);
      dvmul (q,e[0][0].a,1,e[0][i].a,1,e[0][i].a,1);
    }
    break;
  case 'b':
    break;
  case 'c':
    getzw(q,&z,&w,'b');

    //    dfill(q,one,e[1][0].c, 1);

    dvmul(q, v[0].c, 1, v[0].c, 1, e[0][0].c, 1);

    for(i=1;i<L;++i)
      dvmul(q, v[0].c, 1, e[0][i-1].c, 1, e[0][i].c, 1);

    dvmul(q,v[0].c,1,v[4].c,1,e[4][0].c,1);

    for(i = 1; i < L; ++i){
      jacobf(q,z,e[4][i].c,i,alpha,beta);
      dvmul (q,e[4][0].c,1,e[4][i].c,1,e[4][i].c,1);
    }

    break;
  }
}


void Hex_set_edges(Basis *b, int q, char dir){
  register int i;
  int      L = b->id-2;
  double   *z,*w;
  Mode     *v = b->vert, **e = b->edge;

  double   alpha = 1.;
  double    beta = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  getzw(q,&z,&w,'a');

  switch (dir){
  case 'a':
    dvmul(q,v[0].a,1,v[1].a,1,e[0][0].a,1);
    for(i = 1; i < L; ++i){
      jacobf(q,z,e[0][i].a,i,alpha,beta);
      dvmul (q,e[0][0].a,1,e[0][i].a,1,e[0][i].a,1);
    }
    break;
  case 'b':
    break;
  case 'c':

    break;
  }
}


void Tri_set_faces(Basis *b, int q, char dir){
  register int i,j;
  int      L = b->id;
  double  *z,*w;
  Mode   *v = b->vert, **e = b->edge, ***f = b->face;

  switch (dir){
  case 'a':
    break;
  case 'b':
    getzw(q,&z,&w,dir);
    for(i = 0; i < L-3; ++i)
      dvmul(q,v[2].b,1,e[0][i].b,1,f[0][i][0].b,1);

    for(i = 0; i < L-3; ++i)
      for(j = 1; j < L-3-i; ++j){
  jacobf(q,z,f[0][i][j].b,j,2.0*i+3.0,1.0);
  dvmul (q,f[0][i][0].b,1,f[0][i][j].b,1,f[0][i][j].b,1);
      }

    break;
  }
}

void Quad_set_faces(Basis *, int , char ){
  // already set by memory pointer to edge modes
}


void Tet_set_faces(Basis *b, int q, char dir){
  register int i,j;
  int      L = b->id;
  double  *z,*w;
  Mode   *v = b->vert, **e = b->edge, ***f = b->face;

  double   alpha = 1.;
  double    beta = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  switch (dir){
  case 'a':
    break;
  case 'b':
    getzw(q,&z,&w,dir);
    for(i = 0; i < L-3; ++i){
      dvmul(q,v[2].b,1,e[0][i].b,1,f[0][i][0].b,1);
    }
    for(i = 0; i < L-3; ++i)
      for(j = 1; j < L-3-i; ++j){
  alpha = 2.0*i+3.;
  beta  = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  jacobf(q,z,f[0][i][j].b,j,alpha,beta);
  dvmul (q,f[0][i][0].b,1,f[0][i][j].b,1,f[0][i][j].b,1);
      }

    break;
  case 'c':
    getzw(q,&z,&w,dir);

    for(i = 0; i < L-3; ++i){
      dvmul(q,v[3].c,1,e[0][i].c,1,f[1][i][0].c,1);
    }

    for(i = 0; i < L-3; ++i)
      for(j = 1; j < L-3-i; ++j){

  alpha = 2.0*i+3.;
  beta  = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  jacobf(q,z,f[1][i][j].c,j,alpha,beta);
  dvmul (q,f[1][i][0].c,1,f[1][i][j].c,1,f[1][i][j].c,1);
      }
    break;
  }
}


void Pyr_set_faces(Basis *b, int q, char dir){
  int     L = b->id, i,j,k;
  Mode   *v = b->vert, **e = b->edge, ***f = b->face, ***in  = b->intr;
  double *w, *z;
  int cnt = 0;

  double  alpha, beta;

  switch (dir){
  case 'a':
    break;
  case 'b':
    break;
  case 'c':

    getzw(q,&z,&w,'c');

    for(i = 0; i < L-2; ++i)
      for(j = 0; j < L-2; ++j){
  k = min(L, i+j);
  dcopy(q, e[0][k].c, 1, f[0][i][j].c, 1);
      }
    //  dvmul(q, e[0][j].c, 1, e[0][i].c, 1,f[0][i][j].c,1);

    for(i = 0; i < L-3; ++i){
      dvmul(q,v[4].c,1,e[0][i].c,1,f[1][i][0].c,1);
    }
    for(i = 0; i < L-3; ++i)
      for(j = 1; j < L-3-i; ++j){

  alpha = 2.*i+3.; beta = 1.;

  if(option("TWOTWO")){
    alpha += 1.; beta += 1.;
  }

  jacobf(q,z,f[1][i][j].c,j,alpha,beta);
  dvmul (q,f[1][i][0].c,1,f[1][i][j].c,1,f[1][i][j].c,1);
      }

    // interior
    for(i = 0; i < L-3; ++i)
      for(j = 0; j < L-3-i; ++j)
  for(k=0;k < L-3-i-j;++k){

    alpha = 2.*(i+j)+3.; beta = 1.;

    if(option("TWOTWO")){
      alpha += 1.; beta += 1.;
    }

    //    jacobf(q,z,in[i][j][k].c,k,2.0*(i+j)+3.0,1.0);
    jacobf(q,z,in[i][j][k].c,k,alpha,beta);

    dvmul (q,f[0][i][j].c,1,in[i][j][k].c,1,in[i][j][k].c,1);
    dvmul (q,      v[4].c,1,in[i][j][k].c,1,in[i][j][k].c,1);

    ++cnt;
  }

    break;
  }
}


void Prism_set_faces(Basis *b, int q, char dir){
  int     L = b->id, i,j;
  Mode   *v = b->vert, **e = b->edge, ***f = b->face;
  double *w, *z;

  double alpha, beta;

  switch (dir){
  case 'a':
    break;
  case 'b':
    break;
  case 'c':

    getzw(q,&z,&w,'b');
    for(i = 0; i < L-3; ++i){
      dvmul(q,v[4].c,1,e[0][i].c,1,f[1][i][0].c,1);
    }
    for(i = 0; i < L-3; ++i)
      for(j = 1; j < L-3-i; ++j){

  alpha = 2.0*i+3.;
  beta  = 1.;

  if(option("TWOTWO")){
    alpha += 1.;    beta += 1.;
  }

  jacobf(q,z,f[1][i][j].c,j,alpha,beta);
  dvmul (q,f[1][i][0].c,1,f[1][i][j].c,1,f[1][i][j].c,1);
      }

    break;
  }
}



void Hex_set_faces(Basis *b, int q, char dir){
  switch (dir){
  case 'a':
    break;
  case 'b':
    break;
  case 'c':
    break;
  }
}


/* normalised basis in an L2 fashion.                         *
 * don't want to normalise vertices since have a collocation  *
 * property that would be destroyed by it. Want normalisation *
 * of edges and faces to match so that can match expansion    *
 * coefficients for C^0 continuity                            */

static void Tri_normalise(Basis *b, int qa, int qb, int qc){
  register int i;
  double *wa,*wb,*s,integral;
  double *tmp;
  const  int L = b->id;

  qc=qc; // compiler fix

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');

  tmp = dvector(0,QGmax-1);

  /* sort out edge normalisation using side 1 `a' component */
  if(L>2){
    s = b->edge[0][0].a;
    for(i = 0; i < L-2; ++i, s+=qa){
      dvmul(qa,s,1,wa,1,tmp,1);
      integral = 1.0/sqrt(ddot(qa,s,1,tmp,1));
      dscal(qa,integral,s,1);
      dscal(qb,integral,b->edge[1][i].b,1);
    }
  }
  /* normalise face modes using `b' component */
  if(L>3){
    s = b->face[0][0][0].b;
    for(i = 0; i < (L-3)*(L-2)/2; ++i,s+=qb){
      dvmul(qb,s,1,wb,1,tmp,1);
      integral = 1.0/sqrt(ddot(qb,s,1,tmp,1));
      dscal(qb,integral,s,1);
    }
  }
  free(tmp);
}



static void Quad_normalise(Basis *b, char type ){

    register int i;
    int L = b->id;
    int qa = b->qa;
    int qb = b->qb;

  switch(type){
  case 'B': /* babuska and Szabo's normalisation */

    if(L > 2){
      for(i = 0; i < L-2;++i){
  dscal(qa,sqrt((2*i+3)*2.0)/(double)(i+1),b->edge[0][i].a,1);
  dscal(qa,sqrt((2*i+3)*2.0)/(double)(i+1),b->edge[2][i].a,1);
  dscal(qb,sqrt((2*i+3)*2.0)/(double)(i+1),b->edge[1][i].b,1);
  dscal(qb,sqrt((2*i+3)*2.0)/(double)(i+1),b->edge[3][i].b,1);
      }
    }
    break;
  default:
    error_msg("type not known");
  }
}

/* normalised basis in an L2 fashion.                         *
 * don't want to normalise vertices since have a collocation  *
 * property that would be destroyed by it. Want normalisation *
 * of edges and faces to match so that can match expansion    *
 * coefficients for C^0 continuity                            */

static void Tet_normalise(Basis *b, int , int , int ){


}


/* normalised basis in an L2 fashion.                         *
 * don't want to normalise vertices since have a collocation  *
 * property that would be destroyed by it. Want normalisation *
 * of edges and faces to match so that can match expansion    *
 * coefficients for C^0 continuity                            */

static void Pyr_normalise(Basis *, int , int , int ){

}


/* normalised basis in an L2 fashion.                         *
 * don't want to normalise vertices since have a collocation  *
 * property that would be destroyed by it. Want normalisation *
 * of edges and faces to match so that can match expansion    *
 * coefficients for C^0 continuity                            */

static void Prism_normalise(Basis *, int , int , int ){

}


/* normalised basis in an L2 fashion.                         *
 * don't want to normalise vertices since have a collocation  *
 * property that would be destroyed by it. Want normalisation *
 * of edges and faces to match so that can match expansion    *
 * coefficients for C^0 continuity                            */

static void Hex_normalise(Basis *, int , int , int ){

}


/*

Function name: Element::fillElmt

Function Purpose:

Argument 1: Mode *v
Purpose:

Function Notes:

*/

void Tri::fillElmt(Mode *v){
  register int i;

  for(i = 0; i < qb; ++i)
    dcopy(qa,v->a,1,h[i],1);
  for(i = 0; i < qb; ++i)
    dscal(qa,v->b[i],h[i],1);
}




void Quad::fillElmt(Mode *v){
  register int i;

  for(i = 0; i < qb; ++i)
    dcopy(qa,v->a,1,h[i],1);
  for(i = 0; i < qb; ++i)
    dscal(qa,v->b[i],h[i],1);
}




void Tet::fillElmt(Mode *v){
  register int i, j;

  for(i = 0; i < qb*qc; ++i)
    dcopy(qa,v->a,1,(*h_3d)[i],1);

  for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dsmul(qa,v->b[j],h_3d[i][j],1,h_3d[i][j],1);

  for(i = 0; i < qc; ++i)
    dsmul(qa*qb,v->c[i],*(h_3d[i]),1,*(h_3d[i]),1);
}




void Pyr::fillElmt(Mode *v){
  register int i, j;

  for(i = 0; i < qb*qc; ++i)
    dcopy(qa,v->a,1,(*h_3d)[i],1);

 for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dsmul(qa,v->b[j],h_3d[i][j],1,h_3d[i][j],1);

  for(i = 0; i < qc; ++i)
    dsmul(qa*qb,v->c[i],*(h_3d[i]),1,*(h_3d[i]),1);
}




void Prism::fillElmt(Mode *v){
  register int i, j;

  for(i = 0; i < qb*qc; ++i)
    dcopy(qa,v->a,1,(*h_3d)[i],1);

 for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dsmul(qa,v->b[j],h_3d[i][j],1,h_3d[i][j],1);

  for(i = 0; i < qc; ++i)
    dsmul(qa*qb,v->c[i],*(h_3d[i]),1,*(h_3d[i]),1);
}




void Hex::fillElmt(Mode *v){
  register int i, j;

  for(i = 0; i < qb*qc; ++i)
    dcopy(qa,v->a,1,(*h_3d)[i],1);

 for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dsmul(qa,v->b[j],h_3d[i][j],1,h_3d[i][j],1);

  for(i = 0; i < qc; ++i)
    dsmul(qa*qb,v->c[i],*(h_3d[i]),1,*(h_3d[i]),1);
}




void Element::fillElmt(Mode *){ERR;}
