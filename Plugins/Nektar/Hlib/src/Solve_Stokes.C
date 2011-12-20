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
#include <time.h>
#include "hotel.h"

#if 0
double       st, cps = (double)CLOCKS_PER_SEC;

#ifdef __LIBCATAMOUNT__
#define Timing(s) \
 fprintf(stdout,"%s Took %g seconds\n",s,dclock()-st); \
 st = dclock();
#else
#define Timing(s) \
 fprintf(stdout,"%s Took %g seconds\n",s,(clock()-st)/cps); \
 st = clock();
#endif

double       st1;

#ifdef __LIBCATAMOUNT__
#define Timing1(s) \
 fprintf(stdout,"%s Took %g seconds\n",s,dclock()-st1); \
 st1 = dclock();
#else
#define Timing1(s) \
 fprintf(stdout,"%s Took %g seconds\n",s,(clock()-st1)/cps); \
 st1 = clock();
#endif

#else
double st,st1;
#define Timing(s) \
 /* Nothing */
#define Timing1(s) \
 /* Nothing */
#endif

/* general utilities routines */
static void setupRHS         (Element_List **V, Element_List **Vf,double *rhs,
          double *u0, Bndry **Vbc, Bsystem **Vbsys);

static void saveinit         (Element_List **, double *, Bsystem **);

void solve_boundary(Element_List **, Element_List **, double *, double *,
        Bsystem **);
void solve_pressure(Element_List **, Element_List **, double *, double *,
        Bsystem **);
static void solve_velocity_interior(Element_List **V, Bsystem **Vbsys);
static void ScatrBndry_Stokes(double *u, Element_List **V, Bsystem **B);
static void Bsolve_Stokes_PCG (Element_List **V,Bsystem **B, double *p);
static void Bsolve_Stokes_PCR (Element_List **V,Bsystem **B, double *p);

static void divergence_free_init(Element_List **V, double *u0, double *r,
         Bsystem **B, double **wk);

void Solve_Stokes(Element_List **V, Element_List **Vf, Bndry **Vbc,
      Bsystem **Vbsys){

  static double *RHS = (double*)0;
  static double *U0  = (double*)0;
  static int nglob = 0;
  static int eDIM = V[0]->flist[0]->dim();

  if(Vbsys[eDIM]->nglobal > nglob){
    if(nglob){
      free(U0);
      free(RHS);
    }

    nglob = Vbsys[eDIM]->nglobal;
    RHS   = dvector(0,nglob-1);
    U0    = dvector(0,nglob-1);
  }

  dzero(nglob,U0,1);

#ifdef __LIBCATAMOUNT__
  st = dclock();
#else
  st = clock();
#endif

  /* ----------------------------*/
  /* Compute the Right-Hand Side */ setupRHS(V,Vf,RHS,U0,Vbc,Vbsys);
  /*-----------------------------*/
  Timing("SetupRHS.......");

  /*-----------------------------*/
  /* Solve the boundary system   */ solve_boundary(V,Vf,RHS,U0,Vbsys);
  /*-----------------------------*/
  Timing("Boundary.......");

  /*-----------------------------*/
  /* Solve the pressure system   */ solve_pressure(V,Vf,RHS,U0,Vbsys);
  /*-----------------------------*/
  Timing("Pressure.......");

  /*-----------------------------*/
  /* solve the Interior system   */ solve_velocity_interior(V,Vbsys);
  /*-----------------------------*/
  Timing("VInterior......");

}


static void saveinit(Element_List **V, double *u0, Bsystem **B){
  register int i;
  int       eDIM = V[0]->flist[0]->dim();
  int       **bmap = B[eDIM]->bmap;
  Element  *E;

  for(i = 0; i < eDIM; ++i){
    SignChange(V[i],B[i]);
    for(E=V[i]->fhead;E;E=E->next)
      dscatr(E->Nbmodes,E->vert->hj,bmap[E->id]+i*E->Nbmodes,u0);
    SignChange(V[i],B[i]);
  }
}

/* compute right hand side and put leave in U */

static void setupRHS (Element_List **V, Element_List **Vf,double *rhs,
          double *u0, Bndry **Vbc, Bsystem **Vbsys){
  register int i,k;
  int      N,nbl;
  int      eDIM = V[0]->flist[0]->dim();
  Bsystem *PB   = Vbsys[eDIM],*B = Vbsys[0];
  int      nel  = B->nel,info;
  int      **ipiv    = B->Gmat->cipiv;
  double   **binvc   = B->Gmat->binvc;
  double   **invc    = B->Gmat->invc;
  double   ***dbinvc = B->Gmat->dbinvc;
  double   **p_binvc  = PB->Gmat->binvc;
  Element  *E,*E1;
  Bndry    *Ebc;
  double   *tmp;

  if(eDIM == 2)
    tmp = dvector(0,max(8*LGmax,(LGmax-2)*(LGmax-2)));
  else
    tmp = dvector(0,18*LGmax*LGmax);

  B  = Vbsys[0];
  PB = Vbsys[eDIM];

#ifdef __LIBCATAMOUNT__
  st1 = dclock();
#else
  st1 = clock();
#endif

  /* save initial condition */
  saveinit(V,u0,Vbsys);
  Timing1("saveinit..........");

  /* take inner product if in physical space */
  for(i = 0; i < eDIM; ++i){
    if(Vf[i]->fhead->state == 'p')
      Vf[i]->Iprod(Vf[i]);
  }

  /* zero pressure field */
  dzero(Vf[eDIM]->hjtot,Vf[eDIM]->base_hj,1);
  Timing1("zeroing...........");

  /* condense out interior from u-vel + p */
  for(i = 0; i < eDIM; ++i)
    for(E=Vf[i]->fhead;E;E=E->next){
      nbl = E->Nbmodes;
      N   = E->Nmodes - nbl;
      if(N)
  dgemv('T', N, nbl, -1., binvc[E->geom->id], N,
      E->vert->hj+nbl, 1, 1., E->vert->hj,1);
    }
  Timing1("first condense(v).");

  for(i = 0; i < eDIM; ++i)
    for(E=Vf[i]->fhead;E;E=E->next){
      nbl = E->Nbmodes;
      N   = E->Nmodes - nbl;
      if(N) {
  E1 = Vf[eDIM]->flist[E->id];
  if(B->lambda->wave){
    dcopy(N,E->vert->hj+nbl,1,tmp,1);
    dgetrs('N', N, 1, invc[E->geom->id], N,ipiv[E->geom->id],tmp,N,info);
    dgemv('T', N, E1->Nmodes, -1., dbinvc[i][E->geom->id], N,
    tmp, 1, 1., E1->vert->hj,1);
  }
  else{
     dgemv('T', N, E1->Nmodes, -1., dbinvc[i][E->geom->id], N,
    E->vert->hj+nbl, 1, 1., E1->vert->hj,1);
  }
      }
   }

  Timing1("first condense(p).");

  /* add flux terms */
  for(i = 0; i < eDIM; ++i)
    for(Ebc = Vbc[i]; Ebc; Ebc = Ebc->next)
      if(Ebc->type == 'F' || Ebc->type == 'R')
  Vf[i]->flist[Ebc->elmt->id]->Add_flux_terms(Ebc);

  /* second level of factorisation to orthogonalise basis to p */
  for(E=Vf[eDIM]->fhead;E;E=E->next){

    E1 = Vf[0]->flist[E->id];

    nbl = eDIM*E1->Nbmodes + 1;
    N   = E->Nmodes-1;

    dgemv('T', N, nbl, -1.0, p_binvc[E->geom->id], N,
    E->vert->hj+1, 1, 0.0, tmp,1);

    for(i = 0; i < eDIM; ++i){
      E1 = Vf[i]->flist[E->id];
      dvadd(E1->Nbmodes,tmp+i*E1->Nbmodes,1,E1->vert->hj,1,E1->vert->hj,1);
    }

    E->vert->hj[0] += tmp[nbl-1];
  }

  Timing1("second condense...");

  /* subtract boundary initial conditions */
  if(PB->smeth == iterative){
    double **wk;
    double **a = PB->Gmat->a;

    if(eDIM == 2)
      wk = dmatrix(0,1,0,eDIM*4*LGmax);
    else
      wk = dmatrix(0,1,0,eDIM*6*LGmax*LGmax);

    for(k = 0; k < nel; ++k){
      nbl = V[0]->flist[k]->Nbmodes;

      /* gather vector */
      for(i = 0; i < eDIM; ++i)
  dcopy(nbl,V[i]->flist[k]->vert->hj,1,wk[0]+i*nbl,1);

      dspmv('U',eDIM*nbl+1,1.0,a[V[0]->flist[k]->geom->id],
    wk[0],1,0.0,wk[1],1);

      /* subtract of Vf */
      for(i = 0; i < eDIM; ++i)
  dvsub(nbl,Vf[i]->flist[k]->vert->hj,1,wk[1]+i*nbl,1,
        Vf[i]->flist[k]->vert->hj,1);
      Vf[eDIM]->flist[k]->vert->hj[0] -= wk[1][eDIM*nbl];
    }

    GathrBndry_Stokes(Vf,rhs,Vbsys);

    free_dmatrix(wk,0,0);
  }
  else{
    if(Vbc[0]->DirRHS){
      GathrBndry_Stokes(Vf,rhs,Vbsys);

      /* subtract of bcs */
      dvsub(PB->nsolve,rhs,1,Vbc[0]->DirRHS,1,rhs,1);

      /* zero ic vector */
      dzero(PB->nsolve,u0,1);
    }
    else{

      /* zero out interior components since only deal with boundary initial
   conditions (interior is always direct) */

      for(i = 0; i < eDIM; ++i)
  for(E = V[i]->fhead; E; E = E->next){
    nbl = E->Nbmodes;
    N   = E->Nmodes - nbl;
    dzero(N, E->vert->hj + nbl, 1);
  }

      /* inner product of divergence for pressure forcing */
      for(i = 0; i < eDIM; ++i)
  V[i]->Trans(V[i], J_to_Q);

      V[0]->Grad(V[eDIM],0,0,'x');
      V[1]->Grad(0,Vf[eDIM],0,'y');
      dvadd(V[1]->htot,V[eDIM]->base_h,1,Vf[eDIM]->base_h,1,
      V[eDIM]->base_h,1);

      if(eDIM == 3){
  V[2]->Grad(0,V[eDIM],0,'z');
  dvadd(V[2]->htot,V[eDIM]->base_h,1,Vf[eDIM]->base_h,1,
        V[eDIM]->base_h,1);
      }

#ifndef PCONTBASE
      for(k = 0; k < nel; ++k)
  V[eDIM]->flist[k]->Ofwd(*V[eDIM]->flist[k]->h,
        V[eDIM]->flist[k]->vert->hj,
        V[eDIM]->flist[k]->dgL);
#else
      V[eDIM]->Iprod(V[eDIM]);
#endif

      for(i = 0; i < eDIM; ++i){
  for(k = 0; k < nel; ++k){
    E   = V[i]->flist[k];
    nbl = E->Nbmodes;
    N   = E->Nmodes - nbl;

    E->HelmHoltz(PB->lambda+k);

    dscal(E->Nmodes, -B->lambda[k].d, E->vert->hj, 1);

    if(N) {
      /* condense out interior terms in velocity */
      dgemv('T', N, nbl, -1., binvc[E->geom->id], N,
      E->vert->hj+nbl, 1, 1., E->vert->hj,1);

      /* condense out interior terms in pressure*/
      E1 = V[eDIM]->flist[k];
      if(B->lambda->wave){
        dcopy(N,E->vert->hj+nbl,1,tmp,1);
        dgetrs('N',N,1,invc[E->geom->id],N,ipiv[E->geom->id],tmp,N,info);
        dgemv('T', N, E1->Nmodes, -1., dbinvc[i][E->geom->id], N,
        tmp, 1, 1., E1->vert->hj,1);
      }
      else{
        dgemv('T', N, E1->Nmodes, -1., dbinvc[i][E->geom->id], N,
        E->vert->hj+nbl, 1, 1., E1->vert->hj,1);
      }
    }
  }
      }

      /* second level of factorisation to orthogonalise basis to  p */
      /* p - vel */
      for(E=V[eDIM]->fhead;E;E=E->next){

  E1 = V[0]->flist[E->id];

  nbl = eDIM*E1->Nbmodes + 1;
  N   = E->Nmodes-1;

  dgemv('T', N, nbl, -1.0, p_binvc[E->geom->id], N,
        E->vert->hj+1, 1, 0.0, tmp,1);

  for(i = 0; i < eDIM; ++i){
    E1 = V[i]->flist[E->id];
    dvadd(E1->Nbmodes,tmp+i*E1->Nbmodes,1,E1->vert->hj,1,E1->vert->hj,1);
    dvadd(E1->Nbmodes,E1->vert->hj,1,Vf[i]->flist[E->id]->vert->hj,1,
    Vf[i]->flist[E->id]->vert->hj,1);
  }

  Vf[eDIM]->flist[E->id]->vert->hj[0] += E->vert->hj[0] + tmp[nbl-1];
      }
      Timing1("bc condense.......");

      GathrBndry_Stokes(Vf,rhs,Vbsys);
      Timing1("GatherBndry.......");
    }
  }

  /* finally copy inner product of f into v for inner solve */
  for(i = 0; i < eDIM; ++i)
    for(E  = V[i]->fhead; E; E= E->next){
      nbl = E->Nbmodes;
      N   = E->Nmodes - nbl;
      E1 = Vf[i]->flist[E->id];
      dcopy(N, E1->vert->hj+nbl, 1, E->vert->hj+nbl, 1);
    }
  for(E = Vf[eDIM]->fhead; E; E = E->next){
    E1 = V[eDIM]->flist[E->id];
    dcopy(E->Nmodes,E->vert->hj,1,E1->vert->hj,1);
  }

  dzero(PB->nglobal-PB->nsolve, rhs + PB->nsolve, 1);

  free(tmp);
}


void solve_boundary(Element_List **V, Element_List **Vf,
        double *rhs, double *u0, Bsystem **Vbsys){
  int    eDIM = V[0]->flist[0]->dim();
  const  int nsolve = Vbsys[eDIM]->nsolve;
  int    info;
  Bsystem *B = Vbsys[0], *PB = Vbsys[eDIM];

  if(nsolve){
    const  int bwidth = PB->Gmat->bwidth_a;


    if(B->rslv){ /* recursive Static condensation solver */

      Rsolver *R    = PB->rslv;
      int    nrecur = R->nrecur;
      int    aslv   = R->rdata[nrecur-1].cstart, bw = R->Ainfo.bwidth_a;

      Recur_setrhs(R,rhs);

      if(PB->singular)
  rhs[PB->singular-1] = 0.0;

      if(B->smeth == direct){
  if(aslv)
    if(2*bw < aslv){       /* banded matrix */
      error_msg(error in solve_boundary_pressure);
    }
    else                /* symmetric matrix */
      dsptrs('L', aslv, 1, R->A.inva, R->A.pivota, rhs, aslv, info);
      }
      else{
  error_msg(Implement recursive iterative solver);
  /*Recur_Bsolve_CG(PB,rhs,U->flist[0]->type);*/
      }

      Recur_backslv(R,rhs,'n');
    }
    else{

      if(PB->singular)
  rhs[PB->singular-1] = 0.0;

      if(B->smeth == iterative){
  if(iparam("ITER_PCR")){
    Bsolve_Stokes_PCR(V, Vbsys, rhs);
  }
  else
    Bsolve_Stokes_PCG(V, Vbsys, rhs);
      }
      else{
  if(B->lambda->wave){
     if(3*bwidth < nsolve){ /* banded matrix */
      error_msg(pack non-symmetrix solver not completed);
    }
    else                  /* symmetric matrix */
      dgetrs('N', nsolve,1, *PB->Gmat->inva, nsolve,
       PB->Gmat->pivota, rhs, nsolve,info);
  }
  else{
     if(2*bwidth < nsolve) /* banded matrix */
      dpbtrs('L', nsolve, bwidth-1, 1, *PB->Gmat->inva, bwidth,
       rhs, nsolve, info);
    else                  /* symmetric matrix */
      dsptrs('L', nsolve,1, *PB->Gmat->inva, PB->Gmat->pivota, rhs,
       nsolve,info);
  }
      }
    }
  }

  /* add intial conditions for pressure and  internal velocity solve*/
  dvadd(PB->nglobal,u0,1,rhs,1,rhs,1);
  ScatrBndry_Stokes(rhs,V,Vbsys);
}



void solve_pressure(Element_List **V, Element_List **Vf,
       double *rhs, double *u0, Bsystem **Vbsys){
  register int i;
  int    eDIM = V[0]->flist[0]->dim();
  int    info,N,nbl,nblv,id,*bmap;
  Bsystem *B = Vbsys[0], *PB = Vbsys[eDIM];
  Element *E;
  double  *hj,*tmp;
  double  *sc = B->signchange;

  if(eDIM == 2)
    tmp = dvector(0,8*LGmax);
  else
    tmp = dvector(0,18*(LGmax-1)*(LGmax-1));


  /* back solve for pressure */
  for(E=V[eDIM]->fhead;E;E=E->next){
    N   = E->Nmodes - 1;
    id  = E->geom->id;
    hj  = E->vert->hj + 1;

    E->state = 't';

    /* solve interior and negate */
    if(PB->lambda->wave){
      dgetrs('N', N, 1, PB->Gmat->invc[id], N, PB->Gmat->cipiv[id],hj, N,info);
    }
    else{
      dpptrs('L', N, 1, PB->Gmat->invc[id], hj, N, info);
      dneg(N,hj,1);
    }

    bmap  = PB->bmap[E->id];
    nblv  = V[0]->flist[E->id]->Nbmodes;
    nbl   = eDIM*nblv+1;

    for(i = 0; i < nbl;  ++i) tmp[i] = rhs[bmap[i]];
    for(i = 0; i < eDIM; ++i) dvmul(nblv,sc,1,tmp+nblv*i,1,tmp+nblv*i,1);

    if(PB->lambda->wave)
      dgemv('N', N, nbl,-1.0, PB->Gmat->invcd[id], N, tmp, 1, 1.,hj,1);
    else
      dgemv('N', N, nbl,-1.0, PB->Gmat->binvc[id], N, tmp, 1, 1.,hj,1);

    sc += nblv;
  }

  free(tmp);
}

static void solve_velocity_interior(Element_List **V, Bsystem **Vbsys){
  register  int i;
  int       N,nbl,rows,id,info;
  int       *bw = Vbsys[0]->Gmat->bwidth_c;
  int       eDIM = V[0]->flist[0]->dim();
  double    *hj;
  int      **ipiv   = Vbsys[0]->Gmat->cipiv;
  double   **invc   = Vbsys[0]->Gmat-> invc;
  double   **binvc  = Vbsys[0]->Gmat->binvc;
  double   **invcd  = Vbsys[0]->Gmat->invcd;
  double  ***dbinvc = Vbsys[0]->Gmat->dbinvc;
  Element  *E,*P;

  for(i = 0; i < eDIM; ++i)
    for(E=V[i]->fhead;E;E=E->next){
      N = E->Nmodes - E->Nbmodes;

      E->state = 't';

      if(!N) continue;


      id  = E->geom->id;
      nbl = E->Nbmodes;
      hj  = E->vert->hj + nbl;

      P    = V[eDIM]->flist[E->id];
      rows = P->Nmodes;

      if(Vbsys[0]->lambda->wave){
  /* dbinvc only store dbi in this formulation */
  dgemv('N', N, rows, -1., dbinvc[i][id], N, P->vert->hj, 1, 1.0,hj,1);

  dgetrs('T',N,rows,invc[id],N,ipiv[id],dbinvc[i][id],N,info);

  if(N > 3*bw[id])
    dgbtrs('N', N, bw[id]-1,bw[id]-1, 1, invc[id], 3*bw[id]-2, ipiv[id],
     hj, N, info);
  else
    dgetrs('N', N, 1, invc[id], N, ipiv[id], hj, N, info);

  dgemv('N', N, nbl , -1.,  invcd   [id], N, E->vert->hj, 1, 1.0,hj,1);
      }
      else{
   if(N > 2*bw[id])
    dpbtrs('L', N, bw[id]-1, 1, invc[id], bw[id],  hj, N, info);
  else
    dpptrs('L', N, 1, invc[id], hj, N, info);

  dgemv('N', N, nbl , -1.,  binvc   [id], N, E->vert->hj, 1, 1.0,hj,1);
  dgemv('N', N, rows, -1., dbinvc[i][id], N, P->vert->hj, 1, 1.0,hj,1);
      }

    }

}


/* Gather points from U into 'u' using the map in B->bmap */
void GathrBndry_Stokes(Element_List **V, double *u, Bsystem **B){
  register int i,j,k;
  int      *bmap, Nbmodes;
  int      eDIM = V[0]->flist[0]->dim();
  double   *s;
  double   *sc = B[0]->signchange;

  dzero(B[eDIM]->nglobal,u,1);

  for(k = 0; k < B[0]->nel; ++k){
    bmap    = B[eDIM]->bmap[k];

    Nbmodes = V[0]->flist[k]->Nbmodes;

    for(j = 0; j < eDIM; ++j){
      s = V[j]->flist[k]->vert[0].hj;

      for(i = 0; i < Nbmodes; ++i)
  u[bmap[i+j*Nbmodes]] += sc[i]*s[i];
    }

    u[bmap[eDIM*Nbmodes]] += V[eDIM]->flist[k]->vert[0].hj[0];
    sc += Nbmodes;
  }
}

/* Scatter the points from u to U using the map in B->bmap */
static void ScatrBndry_Stokes(double *u, Element_List **V, Bsystem **B){
  register int j,k;
  int      eDIM = V[0]->flist[0]->dim();
  int      nel = B[0]->nel, Nbmodes, *bmap;
  double  *hj;
  double  *sc = B[0]->signchange;

  for(k = 0; k < nel; ++k) {
    bmap    = B[eDIM]->bmap[k];

    Nbmodes = V[0]->flist[k]->Nbmodes;

    for(j = 0; j < eDIM; ++j){
      hj = V[j]->flist[k]->vert->hj;
      dgathr(Nbmodes,  u, bmap + j*Nbmodes, hj);
      dvmul (Nbmodes, sc, 1, hj, 1, hj,1);
    }

    V[eDIM]->flist[k]->vert[0].hj[0] = u[bmap[eDIM*Nbmodes]];

    sc += Nbmodes;
  }
}

#define MAX_ITERATIONS nsolve*nsolve

static void  Bsolve_Stokes_PCG(Element_List **V, Bsystem **B, double *p){
  int    eDIM = V[0]->flist[0]->dim();
  const  int nsolve = B[eDIM]->nsolve;
  int    iter = 0;
  double tolcg, alpha, beta, eps, rtz, rtz_old, epsfac;
  static double *u0 = (double*)0;
  static double *u  = (double*)0;
  static double *r  = (double*)0;
  static double *w  = (double*)0;
  static double *z  = (double*)0, **wk;
  static int nsol = 0, nglob = 0;

  if(nsolve > nsol){
    if(nsol){
      free(u0); free(u); free(r); free(z); free_dmatrix(wk,0,0);
    }

    /* Temporary arrays */
    u0 = dvector(0,B[eDIM]->nglobal-1);/* intial divergence-free solution */
    u  = dvector(0,nsolve-1);          /* Solution                        */
    r  = dvector(0,nsolve-1);          /* residual                        */
    z  = dvector(0,nsolve-1);          /* precondition solution           */

    if(eDIM == 2)
      wk = dmatrix(0,1,0,eDIM*4*LGmax);
    else
      wk = dmatrix(0,1,0,eDIM*6*LGmax*LGmax);

    nsol = nsolve;
  }
  if(B[eDIM]->nglobal > nglob){
    if(nglob)
      free(w);
    w  = dvector(0,B[eDIM]->nglobal-1);      /* A*Search direction    */
    nglob = B[eDIM]->nglobal;
  }

  divergence_free_init(V,u0,p,B,wk);

  dzero (B[eDIM]->nglobal, w, 1);
  dzero (nsolve, u, 1);
  dcopy (nsolve, p, 1, r, 1);

  tolcg  = dparam("TOLCG");
  epsfac = 1.0;
  eps    = sqrt(ddot(nsolve,r,1,r,1))*epsfac;

  if (option("verbose") > 1)
    printf("\t %3d iterations, error = %#14.6g %lg %lg\n",
     iter, eps/epsfac, epsfac, tolcg);

  rtz = eps;
  /* =========================================================== *
   *            ---- Conjugate Gradient Iteration ----           *
   * =========================================================== */

  while (eps > tolcg && iter++ < MAX_ITERATIONS ){
    /* while (sqrt(rtz) > tolcg && iter++ < MAX_ITERATIONS ){*/
    Precon_Stokes(V[0],B[eDIM],r,z);
    rtz  = ddot (nsolve, r, 1, z, 1);

    if (iter > 1) {                         /* Update search direction */
      beta = rtz / rtz_old;
      dsvtvp(nsolve, beta, p, 1, z, 1, p, 1);
    }
    else
      dcopy(nsolve, z, 1, p, 1);

    A_Stokes(V,B,p,w,wk);

    alpha = rtz/ddot(nsolve, p, 1, w, 1);

    daxpy(nsolve, alpha, p, 1, u, 1);            /* Update solution...   */
    daxpy(nsolve,-alpha, w, 1, r, 1);            /* ...and residual      */

    rtz_old = rtz;
    eps = sqrt(ddot(nsolve, r, 1, r, 1))*epsfac; /* Compute new L2-error */

    fprintf(stdout,"%d %lg %lg\n",iter,eps,sqrt(rtz));
  }

  /* =========================================================== *
   *                        End of Loop                          *
   * =========================================================== */

  /* Save solution and clean up */

  dcopy(nsolve,u,1,p,1);

  /* add back u0 */
  dvadd(nsolve,u0,1,p,1,p,1);

  if (iter > MAX_ITERATIONS){
    error_msg (Bsolve_Stokes_CG failed to converge);
  }
  else if (option("verbose") > 1)
    printf("\t %3d iterations, error = %#14.6g %lg %lg\n",
     iter, eps/epsfac, epsfac, tolcg);

  return;
}

static void  Bsolve_Stokes_PCR(Element_List **V, Bsystem **B, double *p){
  int    eDIM = V[0]->flist[0]->dim();
  const  int nsolve  = B[eDIM]->nsolve;
  const  int nglobal = B[eDIM]->nglobal;
  int    iter = 0;
  double tolcg, alpha, beta, eps, Aps, epsfac;
  static double *u  = (double*)0;
  static double *s  = (double*)0;
  static double *r1 = (double*)0;
  static double *r2 = (double*)0;
  static double *Ap = (double*)0;
  static double *Ar = (double*)0;
  static double *z  = (double*)0, **wk;
  static int nsol = 0, nglob = 0;

  if(nsolve > nsol){
    if(nsol){
      free(u); free(s); free(r1); free(z); free_dmatrix(wk,0,0);
    }

    /* Temporary arrays */
    u   = dvector(0,nsolve-1);          /* Solution                        */
    s   = dvector(0,nsolve-1);
    r1  = dvector(0,nsolve-1);          /* residual                        */
    z   = dvector(0,nsolve-1);          /* precondition solution           */

    if(eDIM == 2)
      wk = dmatrix(0,1,0,eDIM*4*LGmax);
    else
      wk = dmatrix(0,1,0,eDIM*6*LGmax*LGmax);

    nsol = nsolve;
  }
  if(nglobal > nglob){
    if(nglob){
      free(Ar); free(Ap); free(r2);
    }
    Ar = dvector(0,nglobal-1);      /* A*r Search direction    */
    Ap = dvector(0,nglobal-1);      /* A*p Search direction    */
    r2 = dvector(0,nglobal-1);          /* residual                        */
    nglob = nglobal;
  }

  dzero (B[eDIM]->nglobal, Ap, 1);
  dzero (B[eDIM]->nglobal, Ar, 1);
  dzero (B[eDIM]->nglobal, r2, 1);
  dzero (nsolve, u, 1);
  dcopy (nsolve, p, 1, r1, 1);

  tolcg  = dparam("TOLCG");
  epsfac = 1.0;
  eps    = sqrt(ddot(nsolve,r1,1,r1,1))*epsfac;

  if (option("verbose") > 1)
    printf("\t %3d iterations, error = %#14.6g %lg %lg\n",
     iter, eps/epsfac, epsfac, tolcg);

  /* =========================================================== *
   *            ---- Conjugate Gradient Iteration ----           *
   * =========================================================== */

  while (eps > tolcg && iter++ < MAX_ITERATIONS ){

    if (iter > 1) {                         /* Update search direction */
      A_Stokes(V,B,r2,Ar,wk);
      beta = -ddot(nsolve,Ar,1,s,1)/Aps;
      dsvtvp (nsolve,beta,p,1,r2,1,p,1);
      dsvtvp (nsolve,beta,Ap,1,Ar,1,Ap,1);
    }
    else {
      Precon_Stokes(V[0],B[eDIM],r1,r2);
      dcopy (nsolve, r2, 1, p, 1);
      A_Stokes(V,B,p,Ap,wk);
    }
    Precon_Stokes(V[0],B[eDIM],Ap,s);

    Aps   = ddot(nsolve,s,1,Ap,1);
    alpha = ddot(nsolve,r2,1,Ap,1)/Aps;

    daxpy(nsolve, alpha, p , 1, u, 1);            /* Update solution...   */
    daxpy(nsolve,-alpha, Ap, 1, r1, 1);           /* ...and residual      */
    daxpy(nsolve,-alpha, s , 1, r2, 1);           /* ...and residual      */

    eps = sqrt(ddot(nsolve, r1, 1, r1, 1))*epsfac; /* Compute new L2-error */

    fprintf(stdout,"%d %lg %lg\n",iter,eps,sqrt(ddot(nsolve,r2,1,r2,1)));
  }

  /* =========================================================== *
   *                        End of Loop                          *
   * =========================================================== */

  /* Save solution and clean up */
  dcopy(nsolve,u,1,p,1);

  if (iter > MAX_ITERATIONS){
    error_msg (Bsolve_Stokes_CG failed to converge);
  }
  else if (option("verbose") > 1)
    printf("\t %3d iterations, error = %#14.6g %lg %lg\n",
     iter, eps/epsfac, epsfac, tolcg);

  return;
}


void  Precon_Stokes(Element_List *U, Bsystem *B, double *r, double *z){

  switch(B->Precon){
  case Pre_Diag:
    dvmul(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,1,r,1,z,1);
    break;
  case Pre_Block:{
    register int i;
    int eDIM = U->fhead->dim();
    int nedge = B->Pmat->info.block.nedge;
    int info,l,cnt;
    double *tmp = dvector(0,LGmax-1);
    static double *pinv;

    dcopy(B->nsolve,r,1,z,1);

#ifdef BVERT
    dpptrs('U',B->Pmat->info.block.nvert,1,B->Pmat->info.block.ivert,
     z,B->Pmat->info.block.nvert,info);
#else
    dvmul(B->Pmat->info.block.nvert,B->Pmat->info.block.ivert,1,z,1,z,1);
#endif

    cnt = B->Pmat->info.block.nvert;
    for(i = 0; i < nedge; ++i){
      l = B->Pmat->info.block.Ledge[i];
      dpptrs('U',l,1,B->Pmat->info.block.iedge[i],z+cnt,l,info);
      cnt += l;
    }

    if(!pinv){
      pinv = dvector(0,B->nsolve-cnt-1);
      for(i = 0; i < B->nsolve-cnt; ++i)
  pinv[i] = U->flist[i]->get_1diag_massmat(0);
      dvrecp(B->nsolve-cnt,pinv,1,pinv,1);
    }
    /* finally multiply pressure dof by inverse of mass matrix */
    dvmul(B->nsolve-cnt,pinv,1,r+cnt,1,z+cnt,1);

    free(tmp);
  }
    break;
  case Pre_None:
    dcopy(B->nsolve,r,1,z,1);
    break;
  case Pre_Diag_Stokes:{
    register int i,j,k;
    double *tmp = dvector(0,B->nglobal-1),*sc,*b,*p,*wk;
    int eDIM = U->fhead->dim();
    int **bmap = B->bmap;
    int nel = B->nel,asize,Nbmodes,info;

    if(eDIM == 2)
      wk = dvector(0,eDIM*4*LGmax);
    else
      wk = dvector(0,eDIM*6*LGmax*LGmax);

    dzero(B->nglobal,tmp,1);
    dcopy(B->nsolve-nel,r,1,tmp,1);

    /* multiply by invc */
    dvmul(B->nsolve-nel,B->Pmat->info.diagst.idiag,1,tmp,1,tmp,1);

    /* multiply by b^T */
    sc = B->signchange;
    p  = z + B->nsolve-nel;
    dcopy(nel,r+B->nsolve-nel,1,p,1);
    dneg (nel-1,p+1,1);
    /* start at 1 to omit singular pressure point */
    if(!(B->singular))
      error_msg(issue in setting singular modes in Stokes_Precon not resolved);

    for(j = 1; j < nel; ++j){
      Nbmodes   = U->flist[j]->Nbmodes;
      asize     = Nbmodes*eDIM+1;

      dzero (asize,wk,1);
      dgathr(eDIM*Nbmodes,  tmp, bmap[j],  wk);
      dvmul (eDIM*Nbmodes, sc, 1, wk, 1, wk, 1);

      b     = B->Gmat->a[U->flist[j]->geom->id] + asize*(asize-1)/2;
      p[j] += ddot(asize-1,b,1,wk,1);
      sc   += asize;
    }

    /* solve for p */
    dpptrs('L', nel, 1, B->Pmat->info.diagst.binvcb,p,nel,info);

    /* generate initial vector as tmp = B p */
    sc = B->signchange;
    dzero(B->nglobal,tmp,1);
    /* start from 1 due to  singular pressure  system */
    for(k = 1; k < nel; ++k){
      Nbmodes = U->flist[k]->Nbmodes;
      asize   = Nbmodes*eDIM+1;
      b       = B->Gmat->a[U->flist[k]->geom->id] + asize*(asize-1)/2;

      dsmul(asize-1,p[k],b,1,wk,1);

      for(i = 0; i < eDIM*Nbmodes; ++i)
  tmp[bmap[k][i]] += sc[i]*wk[i];

      sc += asize;
    }

    /* set z = r - tmp */
    dvsub(B->nsolve-nel,r,1,tmp,1,z,1);
    /* u = invc*z */
    dvmul(B->nsolve-nel,B->Pmat->info.diagst.idiag,1,z,1,z,1);

    free(tmp); free(wk);
  }
    break;
  case Pre_Block_Stokes:{
    register int i,j,k;
    double  *tmp = dvector(0,B->nglobal-1),*sc,*b,*p,*wk;
    int   eDIM = U->fhead->dim(),cnt,l;
    int **bmap = B->bmap;
    int   nel = B->nel,asize,Nbmodes,info;
    int   nedge = B->Pmat->info.blockst.nedge;

    if(eDIM == 2)
      wk = dvector(0,eDIM*4*LGmax);
    else
      wk = dvector(0,eDIM*6*LGmax*LGmax);

    dzero(B->nglobal,tmp,1);
    dcopy(B->nsolve-nel,r,1,tmp,1);

    /* multiply by invc */
#ifdef BVERT
    dpptrs('U',B->Pmat->info.blockst.nvert,1,B->Pmat->info.blockst.ivert,
    tmp,B->Pmat->info.blockst.nvert,info);
#else
    dvmul(B->Pmat->info.blockst.nvert,B->Pmat->info.blockst.ivert,1,
    tmp,1,tmp,1);
#endif

    cnt = B->Pmat->info.blockst.nvert;

    for(i = 0; i < nedge; ++i){
      l = B->Pmat->info.blockst.Ledge[i];
      dpptrs('U',l,1,B->Pmat->info.blockst.iedge[i],tmp+cnt,l,info);
      cnt += l;
    }

    /* multiply by b^T */
    sc = B->signchange;
    p  = z + B->nsolve-nel;
    dcopy(nel,r+B->nsolve-nel,1,p,1);
    dneg(nel-1,p+1,1);
    if(!(B->singular))
      error_msg(issue in setting singular modes in Stokes_Precon not resolved);
    /* start at 1 to omit singular pressure point */
    for(j = 1; j < nel; ++j){
      Nbmodes   = U->flist[j]->Nbmodes;
      asize     = Nbmodes*eDIM+1;

      dzero (asize,wk,1);
      dgathr(eDIM*Nbmodes,  tmp, bmap[j],  wk);
      dvmul (eDIM*Nbmodes, sc, 1, wk, 1, wk, 1);

      b     = B->Gmat->a[U->flist[j]->geom->id] + asize*(asize-1)/2;
      p[j] += ddot(asize-1,b,1,wk,1);
      sc   += asize;
    }

    /* solve for p */
    dpptrs('L', nel, 1, B->Pmat->info.blockst.binvcb,p,nel,info);

    /* generate initial vector as tmp = B p */
    sc = B->signchange;
    dzero(B->nglobal,tmp,1);
    /* start from 1 due to  singular pressure  system */
    for(k = 1; k < nel; ++k){
      Nbmodes = U->flist[k]->Nbmodes;
      asize   = Nbmodes*eDIM+1;
      b       = B->Gmat->a[U->flist[k]->geom->id] + asize*(asize-1)/2;

      dsmul(asize-1,p[k],b,1,wk,1);

      for(i = 0; i < eDIM*Nbmodes; ++i)
  tmp[bmap[k][i]] += sc[i]*wk[i];

      sc += asize;
    }

    /* set z = r - tmp */
    dvsub(B->nsolve-nel,r,1,tmp,1,z,1);

    /* u = invc*z */
#ifdef BVERT
    dpptrs('U',B->Pmat->info.blockst.nvert,1,B->Pmat->info.blockst.ivert,
    z,B->Pmat->info.blockst.nvert,info);
#else
    dvmul(B->Pmat->info.blockst.nvert,B->Pmat->info.blockst.ivert,1,z,1,z,1);
#endif
    cnt = B->Pmat->info.blockst.nvert;

    for(i = 0; i < nedge; ++i){
      l = B->Pmat->info.blockst.Ledge[i];
      dpptrs('U',l,1,B->Pmat->info.blockst.iedge[i],z+cnt,l,info);
      cnt += l;
    }

    free(tmp); free(wk);
  }
    break;
  default:
    fprintf(stderr,"Preconditioner (Precon=%d) not known\n",B->Precon);
    exit(-1);
  }
}

void A_Stokes(Element_List **V, Bsystem **B, double *p, double *w, double **wk)
{

  int      eDIM = V[0]->flist[0]->dim();
  int      nel = B[0]->nel, Nbmodes;
  double   **a = B[eDIM]->Gmat->a;
  int      **bmap = B[eDIM]->bmap;
  double   *sc = B[0]->signchange;
  register int i,j,k;

  dzero(B[eDIM]->nsolve,w,1);

  for(k = 0; k < nel; ++k){
    Nbmodes = V[0]->flist[k]->Nbmodes;

    /* Put p boundary modes into wk[0] and impose continuity */
    dgathr(eDIM*Nbmodes+1,  p, bmap[k], wk[0]);
    for(j = 0; j < eDIM; ++j)
      dvmul (Nbmodes, sc, 1, wk[0] + j*Nbmodes, 1, wk[0] + j*Nbmodes,1);

    dspmv('U',eDIM*Nbmodes+1,1.0,a[V[0]->flist[k]->geom->id],
    wk[0],1,0.0,wk[1],1);

    /* do sign change back and put into w */
    for(j = 0; j < eDIM; ++j)
      for(i = 0; i < Nbmodes; ++i)
  w[bmap[k][i+j*Nbmodes]] += sc[i]*wk[1][i+j*Nbmodes];
    w[bmap[k][eDIM*Nbmodes]] += wk[1][eDIM*Nbmodes];

    sc += Nbmodes;
  }

  /* set first pressure constant to zero */
  if(B[eDIM]->singular)
    w[eDIM*B[0]->nsolve] = 0.0;
}

static void divergence_free_init(Element_List **V, double *u0, double *r,
         Bsystem **B, double **wk){
  register int i,j,k,cnt;
  int      eDIM = V[0]->fhead->dim();
  int      asize,info,Nbmodes;
  int      **bmap  = B[eDIM]->bmap;
  int      nel     = B[eDIM]->nel;
  int      nglobal = B[eDIM]->nglobal;
  int      nsolve  = B[eDIM]->nsolve;
  int      vsolve  = B[0]->nsolve;
  double   **a  = B[eDIM]->Gmat->a,*x,*b,*sc,*sc1;
  static double *invb;

  x = dvector(0,nel-1);

  /* form and invert B' B system */
  if(!invb){
    invb = dvector(0,nel*(nel+1)/2-1);

    sc = B[0]->signchange;
    for(cnt = 0,k = 0; k < nel; ++k){
      /* gather B from local systems */
      Nbmodes = V[0]->flist[k]->Nbmodes;
      asize   = Nbmodes*eDIM+1;
      b       = a[V[0]->flist[k]->geom->id] + asize*(asize-1)/2;

      dzero(nglobal,u0,1);
      for(j = 0; j < eDIM; ++j){
  for(i = 0; i < Nbmodes; ++i)
    u0[bmap[k][i+j*Nbmodes]] += sc[i]*b[i+j*Nbmodes];
      }
      dzero(nglobal-nsolve,u0+nsolve,1);

      /* take inner product with B' */
      sc1 = B[0]->signchange;
      for(j = k; j < nel; ++j,cnt++){
  dzero(asize,wk[0],1);
  dgathr(asize-1,  u0, bmap[j],  wk[0]);
  for(i = 0; i < eDIM; ++i)
    dvmul (Nbmodes, sc1, 1, wk[0]+i*Nbmodes, 1, wk[0]+i*Nbmodes, 1);

  Nbmodes   = V[0]->flist[j]->Nbmodes;
  asize     = Nbmodes*eDIM+1;
  b         = a[V[0]->flist[j]->geom->id] + asize*(asize-1)/2;
  invb[cnt] = ddot(asize-1,b,1,wk[0],1);
  sc1      += Nbmodes;
      }
      sc += V[0]->flist[k]->Nbmodes;
    }
    /* take out first row to deal with singularity  */
    if(B[eDIM]->singular)
      dzero(nel,invb,1);  invb[0] = 1.0;

    dpptrf('L', nel, invb, info);
  }

  /* solve B' B x = r */
  dcopy (nel,r+eDIM*vsolve,1,x,1);
  dpptrs('L', nel, 1, invb, x, nel, info);

  dzero(B[eDIM]->nglobal,u0,1);

  /* generate initial vector as u0 = B x */
  sc = B[0]->signchange;
  for(k = 0; k < nel; ++k){
    Nbmodes = V[0]->flist[k]->Nbmodes;
    asize   = Nbmodes*eDIM+1;
    b       = a[V[0]->flist[k]->geom->id] + asize*(asize-1)/2;

    dsmul(asize-1,x[k],b,1,wk[0],1);

    for(j = 0; j < eDIM; ++j){
      for(i = 0; i < Nbmodes; ++i)
  u0[bmap[k][i+j*Nbmodes]] += sc[i]*wk[0][i+j*Nbmodes];
    }
    sc += Nbmodes;
  }

  dzero(nglobal-eDIM*vsolve, u0 + eDIM*vsolve, 1);

  /* subtract off a*u0 from r */
  sc      = B[0]->signchange;
  for(k = 0; k < nel; ++k){
    Nbmodes = V[0]->flist[k]->Nbmodes;

    dzero(eDIM*Nbmodes+1,wk[0],1);
    dgathr(eDIM*Nbmodes+1, u0, bmap[k], wk[0]);
    for(j = 0; j < eDIM; ++j)
      dvmul (Nbmodes, sc, 1, wk[0] + j*Nbmodes, 1, wk[0] + j*Nbmodes,1);

    dspmv('U',eDIM*Nbmodes+1,1.0,a[V[0]->flist[k]->geom->id],
    wk[0],1,0.0,wk[1],1);

    for(j = 0; j < eDIM; ++j)
      for(i = 0; i < Nbmodes; ++i)
  r[bmap[k][i+j*Nbmodes]] -= sc[i]*wk[1][i+j*Nbmodes];
    r[bmap[k][eDIM*Nbmodes]] -= wk[1][eDIM*Nbmodes];

    sc += Nbmodes;
  }

  r[eDIM*vsolve] = 0.0;
  dzero(nglobal-nsolve, r + nsolve, 1);

  free(x);
}
