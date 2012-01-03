/**************************************************************************/
//                                                                        //
//   Author:    S.Sherwin                                                 //
//   Design:    S.Sherwin                                                 //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
/**************************************************************************/

#include <math.h>
#include <veclib.h>
#include <string.h>
#include "hotel.h"
#include "Tri.h"
#include "Quad.h"

static void  Precon(Bsystem *B, double *r, double *z);

/* subtract off recursive interior patch coupling */

void Recur_setrhs(Rsolver *R, double *rhs){
  register int i,j,n;
  int   nrecur = R->nrecur;
  int   cstart,asize,csize;
  Recur *rdata = R->rdata;
  double *tmp = dvector(0,R->max_asize);

  for(i = 0; i < nrecur; ++i){

    cstart = rdata[i].cstart;

    for(n = 0; n < rdata[i].npatch; ++n){
      asize = rdata[i].patchlen_a[n];
      csize = rdata[i].patchlen_c[n];
      if(asize&&csize){
  dgemv('T', csize, asize, 1., rdata[i].binvc[n], csize,
        rhs + cstart, 1, 0., tmp,1);

  for(j = 0; j < asize; ++j)
    rhs[rdata[i].map[n][j]] -= tmp[j];
      }
      cstart += csize;
    }
  }

  free(tmp);
}

void Recur_backslv(Rsolver *R, double *rhs, char trip){
  register int i,j,n;
  int     nrecur = R->nrecur;
  int     cstart,asize,csize,bw,info;
  Recur   *rdata = R->rdata;
  double  *tmp   = dvector(0,R->max_asize);

  for(i = nrecur-1; i >= 0; --i){
    cstart = rdata[i].cstart;

    for(n = 0; n < rdata[i].npatch; ++n){
      asize = rdata[i].patchlen_a[n];
      csize = rdata[i].patchlen_c[n];
      bw    = rdata[i].bwidth_c[n];

      if(csize){
  if(trip == 'p'){
    if(2*bw < csize)    /* banded matrix */
      dpbtrs('L', csize, bw-1, 1, rdata[i].invc[n], bw, rhs + cstart,
       csize, info);
    else                /* symmatric matrix */
      dpptrs('L', csize, 1, rdata[i].invc[n], rhs + cstart, csize, info);
  }
  else{
    if(2*bw < csize){   /* banded matrix */
      error_msg(error in H_SolveR.c);
    }
    else                /* symmatric matrix */
      dsptrs('L', csize, 1, rdata[i].invc[n], rdata[i].pivotc[n],
       rhs + cstart, csize, info);
  }

  if(asize){
    for(j = 0; j < asize; ++j)
      tmp[j] = rhs[rdata[i].map[n][j]];

    dgemv('N', csize, asize,-1., rdata[i].binvc[n], csize,
    tmp, 1, 1.0, rhs + cstart,1);
  }
      }
      cstart += csize;
    }
  }

  free(tmp);
}

#define MAX_ITERATIONS 2*nsolve

static void Recur_A (Rsolver *R, double *p, double *w, double *wk);

void Recur_Bsolve_CG(Bsystem *B, double *p, char type)
{
  Rsolver    *R = B->rslv;
  const  int nsolve = R->rdata[R->nrecur-1].cstart, nvs = R->Ainfo.nv_solve;
  int    iter = 0;
  double tolcg, alpha, beta, eps, rtz, rtz_old, epsfac;
  double *w,*u,*r,*z,*wk;
  extern double tol;

  /* Temporary arrays */
  u  = dvector(0,nsolve-1);          /* Solution              */
  r  = dvector(0,nsolve-1);          /* residual              */
  z  = dvector(0,nsolve-1);          /* precondition solution */
  w  = dvector(0,nsolve-1);          /* A*Search direction    */

  wk = dvector(0,2*R->max_asize-1);  /* work space            */

  dzero (nsolve, u, 1);
  dcopy (nsolve, p, 1, r, 1);

  // currently this is being down outside multi-level recursion in Solve.C
  if(B->singular&&0) /* take off mean of vertex modes */
    dsadd(nvs, -dsum(nvs, r, 1)/(double)nvs, r, 1, r, 1);

  tolcg  = (type == 'p')? dparam("TOLCGP"):dparam("TOLCG");
  epsfac = (tol)? 1.0/tol : 1.0;
  eps    = sqrt(ddot(nsolve,r,1,r,1))*epsfac;


  if (option("verbose") > 1)
    printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
     type, iter, eps/epsfac, epsfac, tolcg);
  /* =========================================================== *
   *            ---- Conjugate Gradient Iteration ----           *
   * =========================================================== */

  while (eps > tolcg && iter++ < MAX_ITERATIONS )
    {
      Precon(B,r,z);

      rtz  = ddot (nsolve, r, 1, z, 1);

      if (iter > 1) {                         /* Update search direction */
  beta = rtz / rtz_old;
  dsvtvp(nsolve, beta, p, 1, z, 1, p, 1);
      }
      else
  dcopy(nsolve, z, 1, p, 1);

      Recur_A(R,p,w,wk);

      alpha = rtz/ddot(nsolve, p, 1, w, 1);

      daxpy(nsolve, alpha, p, 1, u, 1);            /* Update solution...   */
      daxpy(nsolve,-alpha, w, 1, r, 1);            /* ...and residual      */

      rtz_old = rtz;
      eps = sqrt(ddot(nsolve, r, 1, r, 1))*epsfac; /* Compute new L2-error */

    }

  /* =========================================================== *
   *                        End of Loop                          *
   * =========================================================== */

  /* Save solution and clean up */

  dcopy(nsolve,u,1,p,1);

  if (iter > MAX_ITERATIONS){
    error_msg (Recur_Bsolve_CG failed to converge);
  }
  else if (option("verbose") > 1)
    printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
     type, iter, eps/epsfac, epsfac, tolcg);

  /* Free temporary vectors */
  free(u); free(r); free(w); free(z); free(wk);
  return;
}

static void Recur_A(Rsolver *R, double *p, double *w, double *wk){
  double   **a    = R->A.a;
  Recur    *rdata = R->rdata + R->nrecur-1;
  int      npatch = rdata->npatch;
  int      *alen  = rdata->patchlen_a;
  int      **map  = rdata->map;
  register int i,k;
  double   *wk1 = wk + R->max_asize;

  memset (w, '\0', rdata->cstart * sizeof(double));

  /* put p boundary modes into U and impose continuity */
  for(k = 0; k < npatch; ++k){
    /* gather in terms for patch k */
    dgathr(alen[k],p,map[k],wk);

    /* multiply by a */
    dspmv('L',alen[k],1.0,a[k],wk,1,0.0,wk1,1);

    /* scatter back terms and put in w */
    for(i = 0; i < alen[k]; ++i)
      w[map[k][i]] += wk1[i];
  }
}


static void  Precon(Bsystem *B, double *r, double *z){
  switch(B->Precon){
  case Pre_Diag:
    dvmul(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,1,r,1,z,1);
    break;
  case Pre_Block:
    fprintf(stderr,"Recursive Block precondition not set up\n");
    exit(1);
    break;
  case Pre_None:
      dcopy(B->rslv->rdata[B->rslv->nrecur-1].cstart,r,1,z,1);
  }
}
