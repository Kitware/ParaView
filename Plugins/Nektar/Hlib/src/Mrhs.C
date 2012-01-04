#include <math.h>
#include <veclib.h>
#include "hotel.h"

#if 0
#define MINENERGY
#endif
#define ORIG

static Multi_RHS *Setup_Multi_RHS (Bsystem *Ubsys, int Nrhs, int nsolve,
           char type);

Multi_RHS *Get_Multi_RHS(Bsystem *Ubsys, int Nrhs, int nsolve, char type){
  Multi_RHS *mrhs;

  for(mrhs = Ubsys->mrhs; mrhs; mrhs = mrhs->next){
    if(mrhs->type == type)
      return mrhs;
  }

  /* if not previously set up initialise a new stucture */
  mrhs = Setup_Multi_RHS(Ubsys,Nrhs,nsolve,type);

  mrhs->next = Ubsys->mrhs;
  Ubsys->mrhs = mrhs;

  return mrhs;
}

static Multi_RHS *Setup_Multi_RHS(Bsystem *Ubsys, int Nrhs, int nsolve,
          char type){
  Multi_RHS *mrhs;

  mrhs = (Multi_RHS*) calloc(1, sizeof(Multi_RHS));
  mrhs->type = type;
  mrhs->step   = 0;

  mrhs->nsolve = nsolve;
  mrhs->Nrhs   = Nrhs;

  mrhs->alpha  = dvector(0, Nrhs-1);
  dzero(Nrhs, mrhs->alpha, 1);
#ifdef ORIG
  mrhs->bt     = dmatrix(0, Nrhs-1, 0, nsolve-1);
  dzero(Nrhs*nsolve, mrhs->bt[0], 1);
#else
  mrhs->xbar   = dvector(0, Ubsys->nglobal-1);
#endif
  mrhs->xt     = dmatrix(0, Nrhs-1, 0, nsolve-1);
  dzero(Nrhs*nsolve, mrhs->xt[0], 1);

  return mrhs;
}

#ifdef ORIG
void Mrhs_rhs(Element_List *U, Element_List *Uf, Bsystem *B,
        Multi_RHS *mrhs, double *rhs){
  register int i;
  DO_PARALLEL{
    int j;
    double *wk = dvector(0,mrhs->step);
    for(i = 0; i < mrhs->step; ++i){
      mrhs->alpha[i] = 0.0;
      for(j = 0; j < mrhs->nsolve; ++j)
  mrhs->alpha[i] += B->pll->mult[j]*mrhs->bt[i][j]*rhs[j];
    }
    gdsum(mrhs->alpha,mrhs->step,wk);
    free(wk);
  }
  else
    for(i = 0; i < mrhs->step; ++i)
      mrhs->alpha[i] = ddot(mrhs->nsolve, mrhs->bt[i], 1, rhs, 1);

  for(i = 0; i < mrhs->step; ++i)
    daxpy(mrhs->nsolve, -mrhs->alpha[i], mrhs->bt[i], 1, rhs, 1);
}
#else
void Mrhs_rhs(Element_List *U, Element_List *Uf, Bsystem *B,
        Multi_RHS *mrhs, double *rhs){
  double *bhat;
  register int i;

  bhat = dvector(0, B->nglobal-1);
  dzero(B->nglobal, bhat, 1);
  dzero(B->nglobal,mrhs->xbar,1);

  DO_PARALLEL{
    int j;
    double *wk = dvector(0,mrhs->step);
    for(i = 0; i < mrhs->step; ++i){
      mrhs->alpha[i] = 0.0;
      for(j = 0; j < mrhs->nsolve; ++j)
  mrhs->alpha[i] += B->pll->mult[j]*mrhs->xt[i][j]*rhs[j];
    }
    gdsum(mrhs->alpha,mrhs->step,wk);

    for(i = 0; i < mrhs->step; ++i)
      daxpy(mrhs->nsolve,mrhs->alpha[i],mrhs->xt[i],1,mrhs->xbar,1);

    free(wk);
  }
  else
    for(i = 0; i < mrhs->step; ++i){
      mrhs->alpha[i] = ddot(mrhs->nsolve, mrhs->xt[i], 1, rhs, 1);
      daxpy(mrhs->nsolve,mrhs->alpha[i],mrhs->xt[i],1,mrhs->xbar,1);
    }

  A_fast(U,Uf,B,mrhs->xbar,bhat);

  dvsub(mrhs->nsolve,rhs,1,bhat,1,rhs,1);

  free(bhat);
}
#endif

static int ida_min(int n, double *d, int skip){
  int i;
  int ida = 0;
  for(i = 1; i < n; i += skip)
    ida = (fabs(d[i]) < fabs(d[ida])) ? i : ida;

  return ida;
}


#ifdef ORIG
void Update_Mrhs(Element_List *U, Element_List *Uf,
     Bsystem *B, Multi_RHS *mrhs, double *sol){
  int         i,j,lt;
  int         nsolve = mrhs->nsolve;
  double     *bhat,*xhat,*wk,norm;

  bhat = dvector(0, B->nglobal-1);
  dzero(B->nglobal, bhat, 1);
  xhat = dvector(0, B->nglobal-1);
  dzero(B->nglobal, xhat, 1);
  wk = dvector(0,mrhs->step);

  /* xhat = xt */
  dcopy (nsolve, sol, 1, xhat, 1);

  /* Update x^n = sol + sum alpha_k.xt_k */
  for(i = 0; i < mrhs->step; ++i)
    daxpy(nsolve, mrhs->alpha[i], mrhs->xt[i], 1, sol, 1);

  /* find mode with minimum "enegy" */
#ifdef MINENERGY
  if(mrhs->step == mrhs->Nrhs)
    lt = ida_min(mrhs->Nrhs, mrhs->alpha, 1);
  else
    lt = mrhs->step;
#else
  if(mrhs->step == mrhs->Nrhs){
    mrhs->step = 0;
    lt = 0;
  }
  else
    lt = mrhs->step;
#endif

  /*  bhat = A xhat */
  A_fast(U,Uf,B,xhat,bhat);

  DO_PARALLEL{
    parallel_gather(bhat,B);
    for(i = 0; i < mrhs->step; ++i){
      mrhs->alpha[i] = 0.0;
      for(j = 0; j < nsolve; ++j)
  mrhs->alpha[i]  += B->pll->mult[j]*mrhs->bt[i][j]*bhat[j];
    }
    gdsum(mrhs->alpha,mrhs->step,wk);
  }
  else
    for(i = 0; i < mrhs->step; ++i)
      mrhs->alpha[i] = ddot(nsolve, mrhs->bt[i], 1, bhat, 1);

  mrhs->alpha[lt] = 0.0;

  for(i = 0; i < mrhs->step; ++i){
    daxpy(nsolve, -mrhs->alpha[i], mrhs->bt[i], 1, bhat, 1);
    daxpy(nsolve, -mrhs->alpha[i], mrhs->xt[i], 1, xhat, 1);
  }

  DO_PARALLEL{
    norm =  0.0;
    for(j = 0; j < mrhs->nsolve; ++j)
      norm += B->pll->mult[j]*bhat[j]*bhat[j];
    gdsum(&norm,1,wk);
  }
  else{
    norm = ddot(nsolve, bhat, 1, bhat, 1);
  }

  norm = norm ? 1.0/sqrt(norm): 1.0;

  dsmul(nsolve, norm, bhat, 1, mrhs->bt[lt], 1);
  dsmul(nsolve, norm, xhat, 1, mrhs->xt[lt], 1);

  mrhs->step = min(mrhs->step+1,mrhs->Nrhs);

  free(xhat); free(bhat); free(wk);
}
#else
void Update_Mrhs(Element_List *U, Element_List *Uf,
     Bsystem *B, Multi_RHS *mrhs, double *sol){
  int         i,j,lt;
  int         nsolve = mrhs->nsolve;
  double      *bhat,*wk,norm;

  bhat = dvector(0, B->nglobal-1);
  dzero(B->nglobal, bhat, 1);
  wk = dvector(0,mrhs->step);

  /* restart find mode with minimum "enegy" */
  if(mrhs->step == mrhs->Nrhs){
    /* Update x^n = sol + xbar */
    dvadd(nsolve,mrhs->xbar,1,sol,1,sol,1);

    mrhs->step = 0;
    dcopy(mrhs->nsolve,sol,1,mrhs->xt[0],1);
  }
  else{
    dcopy(nsolve,sol,1,mrhs->xt[mrhs->step],1);

    //  bhat = A xhat
    A_fast(U,Uf,B,sol,bhat);

    // Update x^n = sol + xbar
    dvadd(nsolve,mrhs->xbar,1,sol,1,sol,1);

    DO_PARALLEL{
      parallel_gather(bhat,B);
      for(i = 0; i < mrhs->step; ++i){
  mrhs->alpha[i] = 0.0;
  for(j = 0; j < nsolve; ++j)
    mrhs->alpha[i]  += B->pll->mult[j]*mrhs->bt[i][j]*bhat[j];
      }
      gdsum(mrhs->alpha,mrhs->step,wk);
    }
    else
      for(i = 0; i < mrhs->step; ++i)
  mrhs->alpha[i] = ddot(nsolve, mrhs->xt[i], 1, bhat, 1);

    for(i = 0; i < mrhs->step; ++i)
      daxpy(nsolve, -mrhs->alpha[i], mrhs->xt[i], 1,
      mrhs->xt[mrhs->step], 1);
  }

  DO_PARALLEL{
    norm =  0.0;
    for(j = 0; j < mrhs->nsolve; ++j)
      norm += B->pll->mult[j]*mrhs->xt[mrhs->step][j]*mrhs->xt[mrhs->step][j];
    gdsum(&norm,1,wk);
  }
  else
    norm = ddot(nsolve, mrhs->xt[mrhs->step], 1, mrhs->xt[mrhs->step], 1);

  norm = norm ? 1.0/sqrt(norm): 1.0;
  dsmul(nsolve, norm, mrhs->xt[mrhs->step], 1, mrhs->xt[mrhs->step], 1);

  mrhs->step = min(mrhs->step+1,mrhs->Nrhs);

  free(bhat); free(wk);
}
#endif
