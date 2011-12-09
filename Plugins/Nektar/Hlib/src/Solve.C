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
#include "hotel.h"
#include <string.h>


/* general utilities routines */
static void setupRHS  (Element_List *, Element_List *, double *, double *,
           Bndry *, Bsystem *, SolveType);
static void saveinit  (Element_List *, double *, Bsystem *);
static void GathrBndryNS(Element_List *U, double *u, Bsystem *B);

static void Set_tolerance_RHS(Bsystem *Ubsys, double *RHS, Element_List *U);

void Solve(Element_List *U, Element_List *Uf, Bndry *Ubc, Bsystem *Ubsys,
     SolveType Stype){

  static double *RHS = (double*)0;
  static double *U0  = (double*)0;
  static int nglob = 0;


  if(Ubsys->nglobal > nglob){
    if(nglob){
      free(U0);
      free(RHS);
    }

    nglob = Ubsys->nglobal;
    RHS = dvector(0,nglob-1);
    U0  = dvector(0,nglob-1);
  }

  dzero(Ubsys->nglobal, RHS, 1);
  dzero(Ubsys->nglobal,  U0, 1);


  /* ----------------------------*/
  /* Compute the Right-Hand Side */ setupRHS(U,Uf,RHS,U0,Ubc,Ubsys,Stype);
  /*-----------------------------*/

  /*-----------------------------*/
  /* Solve the boundary system   */ solve_boundary(U,Uf,RHS,U0,Ubsys);
  /*-----------------------------*/

  /*-----------------------------*/
  /* solve the Interior system   */ solve_interior(U,Ubsys);
  /*-----------------------------*/

  //  free(RHS); free(U0);
}

static void saveinit(Element_List *U, double *u0, Bsystem *B){
  int      **bmap = B->bmap;
  Element *E;

  SignChange(U,B);
  for(E=U->fhead;E;E=E->next)
    dscatr(E->Nbmodes,E->vert->hj,bmap[E->id],u0);
  SignChange(U,B);

}

double tol;
double tol_b;
static double tolv[3];
static int    toli=0;

static void Set_tolerance_RHS(Bsystem *Ubsys, double *RHS, Element_List *U){

#if 1
  Element *E  = U->fhead;

  for(tol_b = 0.0;E;E = E->next)
    tol_b += ddot(E->Nbmodes ,E->vert->hj,1,E->vert->hj,1);

  DO_PARALLEL{ /* gather tolerances together to make global tolerance */
    double tmp;
    gdsum(&tol_b,1,&tmp);
  }
#else
  gddot(&tol_b, RHS, RHS, Ubsys->pll->mult, Ubsys->nsolve);

  // tol_b = ddot(Ubsys->nsolve,RHS,1,RHS,1);
  //DO_PARALLEL{ /* gather tolerances together to make global tolerance */
  //  double tmp;
  //  gdsum(&tol_b,1,&tmp);
  //}
#endif

  if(U->fhead->type == 'p')
     tol_b = sqrt(tol_b);

  /* for those wierd cases make sure tol is not less than 10e-3 */
  tol_b = (tol_b < 10e-3)? 1.0: tol_b;
}

static void Set_tolerance(Element_List *E_L, Element_List *Ef_L){
  Element *E  = E_L->fhead;
  Element *Ef = Ef_L->fhead;

  for(tol = 0.0;Ef;Ef = Ef->next)
    tol += ddot(Ef->Nbmodes ,Ef->vert->hj,1,Ef->vert->hj,1);

  DO_PARALLEL{ /* gather tolerances together to make global tolerance */
    double tmp;
    gdsum(&tol,1,&tmp);
  }

  tol = sqrt(tol);

  /*
  if(E->type != 'p'){
    tolv[toli] = tol;

    if(E->dim() == 2){
      tol = sqrt(tolv[0]*tolv[0] + tolv[1]*tolv[1])/2.;
      toli = (++toli)%2;
    }
    else{
      tol = sqrt(tolv[0]*tolv[0] + tolv[1]*tolv[1] + tolv[2]*tolv[2])
  /3.;
      toli = (++toli)%3;
    }
  }
  */

  /* for those wierd cases make sure tol is not less than 10e-3 */
  tol = (tol < 10e-3)? 1.0: tol;

}


/* compute right hand side and put leave in U */

static void setupRHS (Element_List *U, Element_List *Uf,
          double *rhs, double *u0,
          Bndry *Ubc, Bsystem *B, SolveType Stype){
  register int k;
  int      nel = B->nel;
  int      N,nbl;
  double   **binvc = B->Gmat->binvc;
  Element *E;
  Bndry    *Ebc;

  /* save initial condition */
  saveinit(U,u0,B);

  if(Uf->fhead->state == 'p')  /* take inner product if in physical space */
    Uf->Iprod(Uf);

  // fixed for contiguous memory model

  if(Stype == Helm)            /* negate if helmholtz solve               */
    dneg(Uf->hjtot, Uf->base_hj, 1);

  if(B->nsolve){               /* subtract off interior coupling          */
    for(E=Uf->fhead;E;E=E->next){
      nbl = E->Nbmodes;
      N   = E->Nmodes - nbl;
      if(N) dgemv('T', N, nbl, -1., binvc[E->geom->id], N,
      E->vert->hj+nbl, 1, 1., E->vert->hj,1);
    }
  }

  /* add flux terms      */
  for(Ebc = Ubc; Ebc; Ebc = Ebc->next)
    if(Ebc->type == 'F' || Ebc->type == 'R')
      Uf->flist[Ebc->elmt->id]->Add_flux_terms(Ebc);

  /* subtract off Boundary initial condition */
  if(B->smeth == iterative&&!B->rslv){
    double **a = B->Gmat->a;

    if(B->Precon == Pre_LEnergy){
      double **Rv, **Rvi, **Re;
      int    Ne, Nf;
      Element *Ef;

      for(E = Uf->fhead; E; E = E->next){
  E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
  RxV(E->vert->hj,Rv,Re,E->Nverts,Ne,Nf,E->vert->hj);
      }
      Set_tolerance(U,Uf); /* set solver tolerances */

      for(E = U->fhead, Ef = Uf->fhead; E; E = E->next, Ef = Ef->next){
  E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
  IRTxV(E->vert->hj,Rvi,Re,E->Nverts,Ne,Nf,E->vert->hj);

  dspmv('L',E->Nbmodes,-1.,a[E->geom->id],
        E->vert->hj,1,1.,Ef->vert->hj,1);
  dcopy(Ef->Nmodes,Ef->vert->hj,1,E->vert->hj,1);
  E->state = 't';
      }
    }
    else{
      Set_tolerance(U,Uf); /* set solver tolerances */
      for(k = 0; k < nel; ++k){
  dspmv('L',Uf->flist[k]->Nbmodes,-1.0,a[U->flist[k]->geom->id],
        U->flist[k]->vert->hj,1,1.,Uf->flist[k]->vert->hj,1);
  dcopy(Uf->flist[k]->Nmodes,Uf->flist[k]->vert->hj,1,
        U->flist[k]->vert->hj,1);
  U->flist[k]->state = 't';
      }
    }

    GathrBndry(U,rhs,B);
    //Set_tolerance_RHS(B,rhs,U); /* set solver tolerances according to RHS */
    dzero(B->nglobal-B->nsolve, rhs + B->nsolve, 1);
  }
  else{
    if(Ubc&&Ubc->DirRHS){
      /* for non pressure solve have preset vector                 */
      /* note that this implies that the bc's are time independent */
      /* or can be expressed as B(x)F(t) where F(t) is given       */

      // fixed for contiguous memory model

      dcopy(Uf->hjtot, Uf->base_hj, 1, U->base_hj, 1);
      U->Set_state('t');

      GathrBndry(U,rhs,B);
      dzero(B->nglobal-B->nsolve, rhs + B->nsolve, 1);

      /* subtract of bcs */
      dvsub(B->nsolve,rhs,1,Ubc->DirRHS,1,rhs,1);

      /* zero ic vector */
      dzero(B->nsolve,u0,1);
    }
    else{
      for(k = 0; k < nel; ++k)
  dzero(U->flist[k]->Nmodes - U->flist[k]->Nbmodes,
        U->flist[k]->vert->hj + U->flist[k]->Nbmodes, 1);

      if(Stype == Helm){
  U->Trans(U, J_to_Q);
  U->HelmHoltz(B->lambda);
      }
      else{
  U->Trans(U, J_to_Q);
  U->Iprod(U);
      }

      for(k = 0; k < nel; ++k){
  nbl = U->flist[k]->Nbmodes;
  N   = U->flist[k]->Nmodes - nbl;
  if(N) dgemv('T',N, nbl, -1., binvc[U->flist[k]->geom->id], N,
        U->flist[k]->vert->hj+nbl, 1, 1.0,
        U->flist[k]->vert->hj,1);

  dvsub(nbl,Uf->flist[k]->vert->hj,1, U->flist[k]->vert->hj, 1,
        U->flist[k]->vert->hj, 1);

  dcopy(N  ,Uf->flist[k]->vert->hj+nbl,1, U->flist[k]->vert->hj+nbl, 1);
      }
      GathrBndry(U,rhs,B);
      dzero(B->nglobal-B->nsolve, rhs + B->nsolve, 1);
    }
  }
}

static void Bsolve_CG   (Element_List *, Element_List *, Bsystem *, double *);

void solve_boundary(Element_List *U, Element_List *Uf,
        double *rhs, double *u0, Bsystem *B){
  const  int nsolve = B->nsolve;
  int    info;

  if(nsolve){
    if(B->rslv){ /* recursive Static condensation solver */

      Rsolver *R    = B->rslv;
      int    nrecur = R->nrecur;
      int    aslv   = R->rdata[nrecur-1].cstart, bw = R->Ainfo.bwidth_a;

      /* take off mean of vertex modes */
      if((B->smeth == iterative)&&(B->singular)){
  double mean;
  mean   = dsum(B->nv_solve, rhs, 1);
  mean /= (double)B->nv_solve;
  dsadd(B->nv_solve, -mean, rhs, 1, rhs, 1);
      }

      Recur_setrhs(R,rhs);

      if(B->smeth == direct){
  if(B->singular) rhs[B->singular-1] = 0.0;
  if(aslv)
    if(2*bw < aslv)       /* banded matrix */
      dpbtrs('L', aslv, bw-1, 1, R->A.inva, bw, rhs, aslv, info);
    else                /* symmatric matrix */
      dpptrs('L', aslv, 1, R->A.inva, rhs, aslv, info);
      }
      else
  Recur_Bsolve_CG(B,rhs,U->flist[0]->type);

      Recur_backslv(R,rhs,'p');
    }
    else{
      const  int bwidth = B->Gmat->bwidth_a;

      if(B->smeth == iterative)
  Bsolve_CG(U, Uf, B, rhs);
      else{
  if(B->singular)   rhs[B->singular-1] = 0.0;

  if(2*bwidth < nsolve) /* banded matrix */
    dpbtrs('L', nsolve, bwidth-1, 1, *B->Gmat->inva, bwidth,
     rhs, nsolve, info);
  else                  /* symmatric matrix */
    dpptrs('L', nsolve, 1, *B->Gmat->inva, rhs, nsolve, info);
      }
    }
  }

  /* add intial conditions */
  dvadd(B->nglobal,u0,1,rhs,1,rhs,1);

  ScatrBndry(rhs,U,B);
}


void solve_interior(Element_List *U, Bsystem *Ubsys){
  int       N,nbl,id,info;
  int       *bw = Ubsys->Gmat->bwidth_c;
  double   *hj;
  double  **invc  = Ubsys->Gmat-> invc;
  double  **binvc = Ubsys->Gmat->binvc;
  Element *E;

  for(E=U->fhead;E;E=E->next)
    {
      N = E->Nmodes - E->Nbmodes;
      if(!N) continue;

      id  = E->geom->id;
      nbl = E->Nbmodes;
      hj  = E->vert->hj + nbl;

      if(N > 2*bw[id])
  dpbtrs('L', N, bw[id]-1, 1, invc[id], bw[id],  hj, N, info);
      else
  dpptrs('L', N, 1, invc[id], hj, N, info);

      dgemv('N', N, nbl,-1., binvc[id], N, E->vert->hj, 1, 1.,hj,1);
    }
}

#define WALLVAL -9999

void SetBCs(Element_List *EL, Bndry *Ubc, Bsystem *Ubsys){

  if(!Ubsys->signchange)
    setup_signchange(EL,Ubsys);

  register int i;
  int      eid,face,nsolve,nglobal,skip,gid,eDIM,ll;
  Vert    *v;
  Edge    *e;
  double  *bc,scal;
  register int j;
  int      nfv;
  Face     *f;
  Element  *E;

  eDIM = EL->fhead->dim();

  nsolve  = Ubsys->nsolve;
  nglobal = Ubsys->nglobal;
  DO_PARALLEL{
    if(Ubsys->pll->nsolve == Ubsys->pll->nglobal)
      return;
  }
  else
    if(!(nglobal-nsolve)) return;

  bc = dvector(0,nglobal-nsolve);
  dzero(nglobal-nsolve, bc, 1);

  /* copy all Dirichlet values from Ubc into bc */
  for(;Ubc;Ubc= Ubc->next){
    switch(Ubc->type){
    case 'V': case 'v': case 'o': case 'm': case 'M':
      scal = 1.0;
      eid  = Ubc->elmt->id;
      face = Ubc->face;

      E = EL->flist[eid];
      nfv = E->Nfverts(face);

      for(i = 0; i < nfv; ++i){
  v = E->vert + E->vnum(face,i);
  bc[v->gid-nsolve]  = scal*Ubc->bvert[i];
      }

      if(eDIM == 2){
  e = E->edge + face;
  skip = Ubsys->edge[e->gid] - nsolve;
  if(e->l)
    dsmul(e->l, scal, Ubc->bedge[0], 1, bc + skip,1);
      }
      else{
  for(i = 0; i < nfv; ++i){
    e    = E->edge+E->ednum(face,i);
    skip = Ubsys->edge[e->gid] - nsolve;
    dsmul(e->l,scal,Ubc->bedge[i],1,bc+skip,1);
    if(e->con) /* keep in universal format */
      for(j =1;j < e->l; j+=2) bc[skip+j] *=-1.;
  }

  f = E->face + face;
  skip = Ubsys->face[f->gid] - nsolve;

  ll = (nfv == 3) ? f->l*(f->l+1)/2 : f->l*f->l;
  if(ll)
    dsmul(ll, scal, *Ubc->bface, 1, bc + skip,1);
      }
      break;
    case 'W': // treat wall as special case so wall have precedence.
      eid  = Ubc->elmt->id;
      face = Ubc->face;

      E = EL->flist[eid];
      nfv = E->Nfverts(face);

      for(i = 0; i < nfv; ++i){
  v = E->vert + E->vnum(face,i);
  bc[v->gid-nsolve]  = WALLVAL;
      }

      if(eDIM == 2){
  e = E->edge + face;
  skip = Ubsys->edge[e->gid] - nsolve;
  if(e->l)
    dfill(e->l, WALLVAL, bc + skip,1);
      }
      else{
  for(i = 0; i < nfv; ++i){
    e    = E->edge+E->ednum(face,i);
    skip = Ubsys->edge[e->gid] - nsolve;
    dfill(e->l,WALLVAL,bc+skip,1);
  }

  f = E->face + face;
  skip = Ubsys->face[f->gid] - nsolve;

  ll = (nfv == 3) ? f->l*(f->l+1)/2 : f->l*f->l;
  if(ll)
    dfill(ll, WALLVAL, bc + skip,1);
      }
    }
  }

  DO_PARALLEL   BCreduce(bc,Ubsys);

  for(i = 0; i < nglobal-nsolve; ++i) // set all walls
    if(bc[i] == WALLVAL)  bc[i] = 0.0;

  /* copy all values from bc to U*/
  for(E=EL->fhead;E;E = E->next){
    for(i = 0; i < E->Nverts; ++i)
      if(!E->vert[i].solve){
  E->vert[i].hj[0] = bc[E->vert[i].gid-nsolve];
      }
    for(i = 0; i < E->Nedges; ++i)
      if((gid=E->edge[i].gid) >= Ubsys->ne_solve){
  e = E->edge + i;
  skip = Ubsys->edge[gid] - nsolve;
  dcopy(e->l,bc+skip,1,e->hj,1);

  if(e->con && eDIM == 3)
    dscal(e->l/2,-1.0,e->hj+1,2);
      }

    if(eDIM == 3)
      for(i = 0; i < E->Nfaces; ++i)
  if((gid=E->face[i].gid) >= Ubsys->nf_solve){
    f = E->face + i;
    skip = Ubsys->face[gid] - nsolve;

    ll = (E->Nfverts(i) == 3) ? f->l*(f->l+1)/2 : f->l*f->l;
    if(ll)
      dcopy(ll,bc+skip,1,*f->hj,1);
  }
  }
  free(bc);
}

static void Precon   (Element_List *, Element_List *,
          Bsystem *,  double *,  double *);


void gddot(double *alpha, double *r, double *s, double *mult,
       int nsolve){
  int i;
  double tmp = 0.;
  DO_PARALLEL{
    for(i = 0, *alpha = 0.0; i < nsolve; ++i)
      *alpha +=  mult[i]*r[i]*s[i];
    gdsum(alpha,1,&tmp);
  }
  else
    *alpha = ddot(nsolve, r, 1, s, 1);
}


#ifdef Lanczos
#define MAX_lanc_iter     990
#endif

static void Bsolve_CG(Element_List *U, Element_List *Uf,
          Bsystem *B, double *p){
  register int i;
  const  int nsolve = B->nsolve, nvs = B->nv_solve;
  int    iter = 0;
  double tolcg   = 0.0,   beta = 0.0, eps = 0.0, temp_local_eps = 0.0;
  double rtz_old = 0.0, epsfac = 0.0, rtz = 0.0;
  double epsloc = 0.0, alpha = 0.0, tmp = 0.0, L2u = 0.0;
  static double *u = (double*)0;
  static double *r = (double*)0;
  static double *w = (double*)0;
  static double *z = (double*)0;
  static int nsol = 0, nglob = 0;
  static int  Nrhs = option("MRHS_NRHS");
  Multi_RHS *mrhs;

  static double *u_km1 = (double*)0;

  int active_handle = get_active_handle();


#ifdef Lanczos
  double *beta_l,*alpha_l;/* define the vectors to be used in the Lanczos
           Algorithm */
  double *D_l, *E_l;      /* define additional vectors for the Lanczos Alg. */
  int MIN_LANCZOS_ITER = iparam("MIN_LANCZOS_ITER");
#endif

  if(nsolve > nsol){
    if(nsol){
      free(u);      free(r);      free(z);  free(u_km1);
    }

    /* Temporary arrays */
    u  = dvector(0,B->nglobal-1);          /* Solution              */
    r  = dvector(0,B->nglobal-1);          /* residual              */
    z  = dvector(0,B->nglobal-1);          /* precondition solution */
    u_km1 = dvector(0,B->nglobal-1);
    nsol = nsolve;
  }

  if(B->nglobal > nglob){
    if(nglob)
      free(w);
    w  = dvector(0,B->nglobal-1);      /* A*Search direction    */
    nglob = B->nglobal;
  }

  dzero (B->nglobal, w, 1);
  dzero (B->nglobal, u, 1); // nglobal size needed for MRHS
  dcopy (nsolve, p, 1, r, 1);

  tolcg  = (U->fhead->type == 'p')? dparam("TOLCGP"):dparam("TOLCG");
  epsfac = (tol)? 1.0/tol : 1.0;

  //epsfac = 1.0;

  /* =========================================================== *
   *            ---- Conjugate Gradient Iteration ----           *
   * =========================================================== */
  DO_PARALLEL{
    int MAX_ITERATIONS = max(B->pll->nsolve, 1000);

#ifdef Lanczos
    ROOTONLY{
      alpha_l = dvector(0,MAX_ITERATIONS-1); /* Lanczos Alg           */
      beta_l  = dvector(0,MAX_ITERATIONS-1); /* Lanczos Alg           */
      dzero (MAX_ITERATIONS,alpha_l,1);
      dzero (MAX_ITERATIONS,beta_l,1);
    }
#endif
    parallel_gather(r,B);

    if(B->singular){
      if(B->Precon != Pre_LEnergy){ /* take off mean of vertex modes */
  double mean[2], work[2];

  mean[0] = 0.0; mean[1] = 0.0;
  for(i = 0; i < nvs; ++i) {
    mean[0] += B->pll->mult[i]*r[i];
    mean[1] += B->pll->mult[i];
  }

  gdsum(mean,2,work);
  mean[0] = -1.0*mean[0]/mean[1];

  dsadd(nvs, mean[0], r, 1, r, 1);
      }
      else
  if(B->pll->singular)
     r[B->pll->singular-1] = 0.0;
    }


    if(Nrhs){
      mrhs = Get_Multi_RHS(B,Nrhs,nsolve,U->fhead->type);
      Mrhs_rhs(U,Uf,B,mrhs,r);
    }

    /* sort out initial eps so that exact solution is treated properly */
    gddot(&epsloc, r, r, B->pll->mult, nsolve);
    eps = sqrt(epsloc)*epsfac;
   // memcpy(u_km1,u,nsolve*sizeof(double));


    if (option("verbose") > 1)
      ROOTONLY printf("\tInitial eps = %lg; residual_L2 = %lg;\n",eps,sqrt(epsloc));

#ifdef Lanczos
    while ((eps > tolcg ||iter < MIN_LANCZOS_ITER)&& iter++ < MAX_ITERATIONS)
#else
    while (eps > tolcg && iter++ < MAX_ITERATIONS )
#endif
     {
       Precon(U,Uf,B,r,z);

       gddot(&rtz, r, z, B->pll->mult, nsolve);

       if (iter > 1) {                            /* Update search direction */
   beta = rtz / rtz_old;
#ifdef Lanczos
   ROOTONLY
     if (iter <= MAX_lanc_iter)  beta_l[iter] = beta;
#endif
   dsvtvp(nsolve, beta, p, 1, z, 1, p, 1);
       }
       else
   dcopy(nsolve, z, 1, p, 1);

      /* calculate local residual */
       A_fast(U,Uf,B,p,w);

       if(B->pll->singular && B->Precon == Pre_LEnergy)
   w[B->pll->singular-1] = 0.0;

       alpha = ddot(B->nsolve,p,1,w,1);
       gdsum(&alpha, 1, &tmp);
       alpha = rtz/alpha;

#ifdef Lanczos
       ROOTONLY
   if (iter <= MAX_lanc_iter) alpha_l[iter] = alpha;
#endif
       parallel_gather(w,B);

       if(B->singular && B->Precon == Pre_LEnergy)
   w[B->singular-1] = 0.0;

       daxpy(nsolve, alpha, p, 1, u, 1);         /* Update solution...      */
       daxpy(nsolve,-alpha, w, 1, r, 1);         /* ...and residual         */

       rtz_old = rtz;

       gddot(&epsloc, r, r, B->pll->mult, nsolve);
       eps = sqrt(epsloc)*epsfac;                /* local L2 error          */

/************
       daxpy(nsolve, -1.0, u, 1, u_km1, 1);
       gddot(&L2u, u_km1, u_km1, B->pll->mult, nsolve);
       ROOTONLY{
        if (option("verbose") > 1)
          printf("\t L2u(%d) = %lg; eps(%d) = %lg; residual_L2(%d) = %lg;\n",iter,sqrt(L2u),iter,eps,iter,sqrt(epsloc));
       }
       memcpy(u_km1,u,nsolve*sizeof(double));
************/
     }

    /* =========================================================== *
     *                        End of Loop                          *
     * =========================================================== */

#ifdef Lanczos
    /* Start to set up the Lanczos matrix, T, which is a tria-diagonal
       matrix and call the Lapack routine "dsterf" to compute all the
       eigenvalues of a tria-diagonal matrix */

    ROOTONLY{
      int i,info;

      if (iter   > MAX_lanc_iter){
  iter = MAX_lanc_iter;
  printf("Lanczos: Conj.Gradient not fully converged, "
         "stoped at 990 iter.\n");
      };

      D_l = dvector(0, iter-1);  /* allocate memory */
      E_l = dvector(0, iter-1);

      dzero (iter, D_l, 1);
      dzero (iter, E_l, 1);

      D_l[0] = 1.0/alpha_l[1];

      for (i=1; i < iter; ++i)
  D_l[i] = beta_l[i+1]/alpha_l[i]  + 1.0/alpha_l[i+1];
      for (i=0;i<iter-1;++i)
  E_l[i] = -1.0*sqrt(beta_l[i+2])/(alpha_l[i+1]);

      dsterf(iter,D_l,E_l,info);
      if (info) error_msg("Lanczos Method, dsterf  -- info is not zero!!");

      /* print maximum and minimum eigenvalues */
      printf("Min, Max, Condition nu: %.8f, %.8f %.8f\n",
       D_l[0],D_l[iter-1],D_l[iter-1]/D_l[0]);
      free(D_l); free(E_l); free(alpha_l); free(beta_l);
    }
#endif

    if(Nrhs)
      Update_Mrhs(U,Uf,B,mrhs,u);

    /* Save solution and clean up */
    dcopy(nsolve,u,1,p,1);

    if (iter > MAX_ITERATIONS){
      fprintf(stderr, "Iter = %d, Nprocs = %d\n",
        iter, pllinfo[active_handle].nprocs);
      error_msg (Bsolve_CG failed to converge);
    }
    else if (option("verbose") > 1)
      ROOTONLY printf("\tField %c: %3d iterations, error, L2_sol_err = %#14.6g %lg %lg %lg\n",
          U->fhead->type, iter, eps, epsfac, tolcg,sqrt(L2u));
    }
  else{
    int MAX_ITERATIONS = max(nsolve, 1000);

#ifdef Lanczos
    alpha_l = dvector(0,MAX_ITERATIONS-1); /* Lanczos Alg           */
    beta_l  = dvector(0,MAX_ITERATIONS-1); /* Lanczos Alg           */
    dzero (MAX_ITERATIONS,alpha_l,1);  /* zero the matrices  */
    dzero (MAX_ITERATIONS,beta_l,1);   /* zero the matrices  */
#endif

    if(B->singular){
      if(B->Precon != Pre_LEnergy){ /* take off mean of vertex modes */
  epsfac  = dsum(nvs, r, 1);
  epsfac /= (double)nvs;
  dsadd(nvs, -epsfac, r, 1, r, 1);
      }
      else r[B->singular-1] = 0.0;
    }

    if(Nrhs){
      mrhs = Get_Multi_RHS(B,Nrhs,nsolve,U->fhead->type);
      Mrhs_rhs(U,Uf,B,mrhs,r);
    }

    tolcg  = (U->fhead->type == 'p')? dparam("TOLCGP"):dparam("TOLCG");
    epsfac = (tol)? 1.0/tol : 1.0;
    eps    = sqrt(ddot(nsolve,r,1,r,1))*epsfac;

    if (option("verbose") > 1)
      printf("\tInitial eps : %lg\n",eps);


    /* =========================================================== *
     *            ---- Conjugate Gradient Iteration ----           *
     * =========================================================== */

#ifdef Lanczos
    while ((eps > tolcg ||iter < MIN_LANCZOS_ITER)&& iter++ < MAX_ITERATIONS)
#else
    while (eps > tolcg && iter++ < MAX_ITERATIONS )
#endif
      {
       Precon(U,Uf,B,r,z);
  rtz  = ddot (nsolve, r, 1, z, 1);

  if (iter > 1) {                         /* Update search direction */
    beta = rtz / rtz_old;
#ifdef Lanczos
    if (iter <= MAX_lanc_iter)  beta_l[iter] = beta;
#endif
    dsvtvp(nsolve, beta, p, 1, z, 1, p, 1);
  }
  else
    dcopy(nsolve, z, 1, p, 1);

  A_fast(U,Uf,B,p,w);

  if(B->singular && B->Precon == Pre_LEnergy)
    w[B->singular-1] = 0.0;

  alpha = rtz/ddot(nsolve, p, 1, w, 1);

#ifdef Lanczos
  if (iter <= MAX_lanc_iter) alpha_l[iter] = alpha;
#endif

  daxpy(nsolve, alpha, p, 1, u, 1);        /* Update solution...   */
  daxpy(nsolve,-alpha, w, 1, r, 1);        /* ...and residual      */

  rtz_old = rtz;
  eps = sqrt(ddot(nsolve, r, 1, r, 1))*epsfac; /*Compute new L2-error*/
      }


    /* =========================================================== *
     *                        End of Loop                          *
     * =========================================================== */
#ifdef Lanczos
    /* Start to set up the Lanczos matrix, T, which is a tria-diagonal
       matrix and call the Lapack routine "dsterf" to compute all the
       eigenvalues of a tria-diagonal matrix */

    int i,info;

    if (iter   > MAX_lanc_iter){
      iter = MAX_lanc_iter;
      printf("Lanczos: Conj.Gradient not fully converged,stop at 990 iter.\n");
    };

    D_l = dvector(0, iter-1);  /* allocate memory */
    E_l = dvector(0, iter-1);

    dzero (iter, D_l, 1);
    dzero (iter, E_l, 1);

    D_l[0] = 1.0/alpha_l[1];

    for (i=1; i < iter; ++i)
      D_l[i] = beta_l[i+1]/alpha_l[i]  + 1.0/alpha_l[i+1];
    for (i=0;i<iter-1;++i)
      E_l[i] = -1.0*sqrt(beta_l[i+2])/(alpha_l[i+1]);

    dsterf(iter,D_l,E_l,info);
    if (info) error_msg("Lanczos Method, dsterf  -- info is not zero!!");

    /* print maximum and minimum eigenvalues */
    printf("Min, Max, Condition nu: %.8f, %.8f %.8f\n",
     D_l[0],D_l[iter-1],D_l[iter-1]/D_l[0]);
    free(D_l); free(E_l); free(alpha_l); free(beta_l);
#endif

    if(Nrhs)
      Update_Mrhs(U,Uf,B,mrhs,u);

    /* Save solution and clean up */
    dcopy(nsolve,u,1,p,1);

    if (iter > MAX_ITERATIONS){
      fprintf(stderr, "Iter = %d, Nprocs = %d\n",
        iter, pllinfo[active_handle].nprocs);
      error_msg (Bsolve_CG failed to converge);
    }
    else if (option("verbose") > 1)
      ROOTONLY printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
          U->fhead->type, iter, eps, epsfac, tolcg);
    }

    if(B->Precon == Pre_LEnergy){ /* tranform back solution to original basis*/
      double **Rv, **Rvi, **Re;
      double *mult = B->Pmat->info.lenergy.mult;
      double alpha = 0;
      int i, Ne, Nf;
      Element *E;

      /* transform residual */
      if(LGmax > 2){
  dzero(B->nglobal-B->nsolve,p+B->nsolve,1);
  ScatrBndry(p,U,B);

  for(E = U->fhead; E; E = E->next){
    E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
    RTxV(E->vert->hj,Rv,Re,E->Nverts,Ne,Nf,E->vert->hj);
  }
  GathrBndryNS (U,p,B);
      }
    }

    return;
  }


  void PreconOverlap(Element_List *U, Element_List *Uf,
       Bsystem *B, double *pin, double *zout);
void PreconDirectOverlap(Bsystem *B, double *pin, double *zout);

 static void  Precon(Element_List *U, Element_List *Uf, Bsystem *B,
        double *r, double *z){
  static int counter = 0;

  switch(B->Precon){
  case Pre_Diag:
    dvmul(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,1,r,1,z,1);
    break;
  case Pre_Block:
    break;
  case Pre_None:
    if(B->rslv)
      dcopy(B->rslv->rdata[B->rslv->nrecur-1].cstart,r,1,z,1);
    else
      dcopy(B->nsolve,r,1,z,1);
    break;
  case Pre_LEnergy: /* Low Energy */
    {
      register int i,j;
      MatPre  *M = B->Pmat;
      int nvs = M->info.lenergy.nvert,
    nes = M->info.lenergy.nedge,
    nel = B->nel;
      int      l,info = 0;
      double   *s,*s1;
      double   **Rv, **Rvi, **Re;
      double   *tmp  = dvector(0,B->nglobal+1);
      const    int nfs = M->info.lenergy.nface;
      int      Ne, Nf;
      Element  *E;
      static    int  doblock = (!option("NoVertBlock"));

      dzero (B->nglobal,z,1);

      /* vertex space */
      if(nvs&&doblock){
  int bw = M->info.lenergy.bw;

  DO_PARALLEL{
    double *ztmp = dvector(0,2*nvs-1);

    dzero(nvs,ztmp,1);

    dcopy (B->nsolve,r,1,z,1);
    dvmul(B->nsolve,B->Pmat->info.lenergy.mult,1,z,1,z,1);
          //dvmul_inc1(B->nsolve,B->Pmat->info.lenergy.mult,z,z);

    ScatrBndry (z,U,B);

#ifndef LE_VERT_OLD
    // there is a bug in this routines!!
    int id;
    int nv_lpsolve = B->pll->nv_lpsolve;
    int csize  = B->nv_solve - nv_lpsolve;
    double *fint = dvector(0,csize);


          double *sclp_ztmp,*sclp_ztmp_local,*sclp_ztmp_work;
    double start_time,end_time;


    dzero(csize,fint,1);

    for(E = U->fhead; E; E = E->next){
      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
      RvxV(E->vert->hj,Rvi,E->Nverts,Ne,Nf,E->vert->hj);
      for(j = 0; j < E->Nverts; ++j)
        if((id=B->bmap[E->id][j]) < B->nv_solve)
    if(id < nv_lpsolve)
      ztmp[B->pll->solvemap[id]] += E->vert->hj[j];
    else
      fint[id-nv_lpsolve] += E->vert->hj[j];
    }

    /* subtract off interior terms */
    for(i = 0; i < nv_lpsolve; ++i)
      ztmp[B->pll->solvemap[i]] -=
        ddot(csize,*M->info.lenergy.ivert_B+i*csize,1,fint,1);


    // solve global modes on interface

          if (M->info.lenergy.ivert_type == 'S'){
            /*  original solver */

            if(B->singular)
        ztmp[B->singular-1] = 0.0;

            DO_PARALLEL
        gdsum(ztmp,nvs,ztmp+nvs);

            int *pll_solvemap = B->pll->solvemap;
            //start_time = dclock();
      dpptrs('L', nvs, 1, *M->info.lenergy.ivert, ztmp, nvs, info);
            //end_time = dclock();
      if(info) fprintf(stderr,"Error solving  matrix in InvtPrecon\n");
            //printf("Lapack: %2.8f  \n",end_time-start_time);

            dgathr(nv_lpsolve,ztmp,B->pll->solvemap,z);
    }
    else{ //parallel solver - ScaLapack

#ifdef PARALLEL


#if 1
            //gdsum(ztmp,nvs,ztmp+nvs);
            //double timer_start = dclock_mpi();
            gather_vector(B,ztmp,ztmp+nvs);
            //double timer_end   = dclock_mpi();
            //timer_end = timer_end-timer_start;
            //gdmax(&timer_end,1,&timer_start);
            //ROOTONLY
            //  fprintf(stderr,"gather_vector: max time = %e \n",timer_start);

            parallel_dgemv(M->info.lenergy.BLACS_PARAMS,M->info.lenergy.ivert_local,ztmp,ztmp+nvs,
                            M->info.lenergy.first_row,M->info.lenergy.first_col,
                            M->info.lenergy.col_displs,M->info.lenergy.col_rcvcnt);
            scatter_vector(B, ztmp+(M->info.lenergy.first_row), z);
            info = 0;
#else
      sclp_ztmp_local = dvector(0,M->info.lenergy.BLACS_PARAMS[11]-1);
            gather_topology_nektar(M->info.lenergy.BLACS_PARAMS, ztmp, ztmp+nvs);
             //blacs_dgather_rhs_nektar(M->info.lenergy.BLACS_PARAMS,ztmp);

        /*  solve the same system using ScaLapack*/
            /* redistribute RHS as 2D cyclic block */

            if (M->info.lenergy.BLACS_PARAMS[6] == 0)
              dgathr(M->info.lenergy.BLACS_PARAMS[11],ztmp,M->info.lenergy.map_row,sclp_ztmp_local);

      //start_time = dclock();
            pdgemv_nektar(M->info.lenergy.BLACS_PARAMS,
                                 M->info.lenergy.DESC_ivert,
                                 M->info.lenergy.DESC_rhs,
                                 M->info.lenergy.ivert_local,
                                 M->info.lenergy.ivert_ipvt,
                                 sclp_ztmp_local);
             info = 0;
      //end_time = dclock();

           // printf("ScaLapack: %2.8f  \n",end_time-start_time);
            /* LG: only sclp_ztmp_local of first proc in each row of procs is useful.
               Redistribute local solution into global array on these procs only then scatter the result from
               the first proc in a row to the rest. Then use dgather on each proc.
             */

            dzero(2*nvs,ztmp,1);

            //start_time = dclock();

            if (M->info.lenergy.BLACS_PARAMS[6] == 0){
        for (j = 0; j < M->info.lenergy.BLACS_PARAMS[11]; j++)
          ztmp[M->info.lenergy.map_row[j]] = sclp_ztmp_local[j];
      }

            //gdsum(ztmp,nvs,ztmp+nvs);

            scatter_topology_nektar(M->info.lenergy.BLACS_PARAMS, ztmp, ztmp+nvs);
            dgathr(nv_lpsolve,ztmp,B->pll->solvemap,z);
            //end_time = dclock();
            //printf("ScaLapack_postproc: %2.8f  \n",end_time-start_time);

            free(sclp_ztmp_local);
#endif
          ;
#endif
    }

#if 0
            static int FLAG_INDEX = 0;
            FILE *Fsolution;
            char fname[128];
      /*
            ROOTONLY{
              sprintf(fname,"Solution_%d_%d.dat",FLAG_INDEX,mynode());
              Fsolution = fopen(fname,"w");
              for (j = 0; j < nvs ; j++)
                fprintf(Fsolution," %2.16f \n ",sclp_ztmp[j]);

              fclose(Fsolution);
      }
      */
      ROOTONLY{
              sprintf(fname,"SolutionG_%d_%d.dat",FLAG_INDEX,mynode());
        Fsolution = fopen(fname,"w");
              for (j = 0; j < nvs ; j++)
                fprintf(Fsolution," %2.16f \n ",ztmp[j]);
              fclose(Fsolution);
      }
            FLAG_INDEX++;
#endif
    free(ztmp);


    if(csize){
      // solve for interior mode
      dpptrs('L', csize, 1, *M->info.lenergy.ivert_C, fint, csize, info);
      if(info) fprintf(stderr,"Error solving  matrix in InvtPrecon\n");

      // substract inv(c) B^t contribution
      for(i = 0; i < csize; ++i)
        z[nv_lpsolve+i] = fint[i] -
    ddot(nv_lpsolve,M->info.lenergy.ivert_B[0]+i,csize,z,1);
    }

    free(fint);
#else
    for(E = U->fhead; E; E = E->next){
      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
      RvxV(E->vert->hj,Rvi,E->Nverts,Ne,Nf,E->vert->hj);
      for(j = 0; j < E->Nverts; ++j)
        if(B->bmap[E->id][j] < B->nv_solve)
    ztmp[B->pll->solvemap[B->bmap[E->id][j]]] += E->vert->hj[j];
    }

    gdsum(ztmp,nvs,ztmp+nvs);

    if(B->singular) ztmp[B->singular-1] = 0.0;

    if(2*bw < nvs)
      dpbtrs('L', nvs, bw-1, 1, *M->info.lenergy.ivert,
       bw, ztmp, nvs, info);
    else
      dpptrs('L', nvs, 1, *M->info.lenergy.ivert, ztmp, nvs, info);

    dgathr(B->nv_solve,ztmp,B->pll->solvemap,z);

#endif

    // put solution back into LE expansion
    dzero(B->nglobal-B->nv_solve,z+B->nv_solve,1);
    for(E = U->fhead; E; E = E->next){
      dzero(E->Nbmodes,E->vert->hj,1);
      for(j = 0; j < E->Nverts; ++j)
        if(B->bmap[E->id][j] < B->nv_solve)
    E->vert->hj[j] = z[B->bmap[E->id][j]];
        else
    E->vert->hj[j] = 0.0;
      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
      RvTxV(E->vert->hj,Rvi,E->Nverts,Ne,Nf,E->vert->hj);
    }

    GathrBndryNS (U,z,B);
    dzero(B->nglobal-B->nsolve,z+B->nsolve,1);
  }
  else{
    dcopy (B->nsolve,r,1,z,1);
    dvmul (B->nsolve,B->Pmat->info.lenergy.mult,1,z,1,z,1);
    ScatrBndry (z,U,B);
    dzero(nvs,z,1);

    for(E = U->fhead; E; E = E->next){
      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
      RvxV(E->vert->hj,Rvi,E->Nverts,Ne,Nf,E->vert->hj);
      for(j = 0; j < E->Nverts; ++j)
        z[B->bmap[E->id][j]] += E->vert->hj[j];
    }

    if(B->singular) z[B->singular-1] = 0.0;

    if(2*bw < nvs)
      dpbtrs('L', nvs, bw-1, 1, *M->info.lenergy.ivert, bw, z, nvs, info);
    else
      dpptrs('L', nvs, 1, *M->info.lenergy.ivert, z, nvs, info);

    dzero(B->nglobal-nvs,z+nvs,1);

    for(E = U->fhead; E; E = E->next){
      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
      dzero(E->Nbmodes,E->vert->hj,1);
      for(j = 0; j < E->Nverts; ++j)
        E->vert->hj[j] = z[B->bmap[E->id][j]];
      RvTxV(E->vert->hj,Rvi,E->Nverts,Ne,Nf,E->vert->hj);
    }

    GathrBndryNS (U,z,B);
    dzero(B->nglobal-B->nsolve,z+B->nsolve,1);
  }
  if(info) error_msg(error inverting coarse solve in precon);
      }

      /* transform residual */
      if(LGmax > 2){
  dcopy(B->nsolve,r,1,tmp,1);
  DO_PARALLEL
    if(B->pll->singular) tmp[B->pll->singular-1] = 0.0;
  else
    if(B->singular) tmp[B->singular-1] = 0.0;

  nvs = B->nv_solve; /* reset nvs for parallel solver */
  if(nvs){
    dvmul(nvs,M->info.lenergy.levert,1,tmp,1,tmp,1);
          //dvmul_inc1(nvs,M->info.lenergy.levert,tmp,tmp);
  }
  for(i = 0, s=tmp+nvs; i < nes; ++i){
    l = M->info.lenergy.Ledge[i];
    if(l) dpptrs('L', l, 1, M->info.lenergy.iedge[i], s, l, info);
    s  +=l;
  }
  for(i = 0; i < nfs; ++i){
    l = M->info.lenergy.Lface[i];
    if(l) dpptrs('L', l, 1, M->info.lenergy.iface[i], s, l, info);
    s  +=l;
  }
  dvadd(B->nsolve,tmp,1,z,1,z,1);
      }
      free(tmp);
    }
    break;
  case Pre_Overlap:
    //dvmul(B->Pmat->info.overlap.ndiag,B->Pmat->info.overlap.idiag,1,r,1,z,1);
    //PreconOverlap(U, Uf, B, r, z);

    PreconDirectOverlap(B, r, z);

    break;
  }
}

void setup_signchange(Element_List *U, Bsystem *B){
  double *tmp = dvector(0, U->hjtot-1);
  dcopy(U->hjtot, U->base_hj, 1, tmp, 1);

  Element *E;
  int cnt = 0;
  double *sc;

  for(E=U->fhead;E;E=E->next)
    cnt += E->Nbmodes;

  sc = B->signchange = dvector(0, cnt-1);
  dfill(U->hjtot, 1., U->base_hj, 1);
  for(E=U->fhead;E;E=E->next){
    E->Sign_Change();
    dcopy(E->Nbmodes, E->vert[0].hj, 1, sc, 1);
    sc += E->Nbmodes;
  }

  dcopy(U->hjtot, tmp, 1, U->base_hj, 1);
  free(tmp);
}

/* multiply out connectivity sign changes */
void SignChange(Element_List *U, Bsystem *B)
{
  double *sc = B->signchange;
  Element *E;

  for(E=U->fhead;E;sc+=E->Nbmodes, E=E->next)
    dvmul(E->Nbmodes, sc, 1, E->vert[0].hj, 1, E->vert[0].hj, 1);
}

/* Gather points from U into 'u' using the map in B->bmap */
void GathrBndry(Element_List *U, double *u, Bsystem *B){
  register int i,k;
  int      *bmap, Nbmodes;
  double   *s;

  dzero(B->nsolve,u,1);

  double *sc = B->signchange;
  for(k = 0; k < B->nel; ++k){

    Nbmodes = U->flist[k]->Nbmodes;
    bmap = B->bmap[k];

    s = U->flist[k]->vert[0].hj;

    for(i = 0; i < Nbmodes; ++i)
      u[bmap[i]] += sc[i]*s[i];
    sc += Nbmodes;
  }
}

/* Gather points from U into 'u' using the map in B->bmap */
static void GathrBndryNS(Element_List *U, double *u, Bsystem *B){
  int      nel = B->nel;
  double   *sc = B->signchange;
  Element  *E;

  for(E = U->fhead; E; E = E->next){
    dvmul (E->Nbmodes,sc,1,E->vert->hj,1,E->vert->hj,1);
    dscatr(E->Nbmodes,E->vert->hj,B->bmap[E->id],u);
    sc += E->Nbmodes;
  }

}

/* Scatter the points from u to U using the map in B->bmap */
void ScatrBndry(double *u, Element_List *U, Bsystem *B){
  register int k;
  int      nel = B->nel, Nbmodes;
  double *hj;
  double *sc = B->signchange;

  for(k = 0; k < nel; ++k) {
    Nbmodes = U->flist[k]->Nbmodes;
    hj = U->flist[k]->vert->hj;
    dgathr(Nbmodes,  u, B->bmap[k], hj);
    dvmul (Nbmodes, sc, 1, hj, 1, hj,1);
    sc += Nbmodes;
  }
}

void A(Element_List *U, Element_List *Uf, Bsystem *B, double *p, double *w){
  int nel = B->nel;
  double   **a = B->Gmat->a;
  register int k;

  /* put p boundary modes into U and impose continuity */
  ScatrBndry (p,Uf,B);

  for(k = 0; k < nel; ++k)
    dspmv('L',U->flist[k]->Nbmodes,1.0,a[U->flist[k]->geom->id],
    Uf->flist[k]->vert->hj,1,0.0,U->flist[k]->vert->hj,1);

  /* do sign change back and put into w */
  GathrBndry (U,w,B);
}


void A_fast(Element_List *U, Element_List *Uf,
       Bsystem *B, double *p, double *w){

  int      nel = B->nel, Nbmodes, geomid, *bmap;
  double   **a = B->Gmat->a, *Uhj, *Ufhj;
  double   *sc = B->signchange;
  register int i,k;

  dzero(B->nsolve, w, 1);
  /* put p boundary modes into U and impose continuity */
  for(k = 0; k < nel; ++k){
    Uhj  =  U->flist[k]->vert[0].hj;
    Ufhj = Uf->flist[k]->vert[0].hj;

    Nbmodes = U->flist[k]->Nbmodes;
    geomid  = U->flist[k]->geom->id;

    bmap = B->bmap[k];

    // gather modes
    dgathr(Nbmodes, p, bmap, Ufhj);
    // fix connectivity
    dvmul (Nbmodes, sc, 1, Ufhj, 1, Ufhj, 1);
    // A.p
    dspmv('L',Nbmodes, 1.0, a[geomid], Ufhj, 1, 0.0, Uhj, 1);
    // scatter modes
    for(i = 0; i < Nbmodes; ++i)
      w[bmap[i]] += sc[i]*Uhj[i];

    sc += Nbmodes;
  }
}

#define MAX_ITERATIONS   nsolve*nsolve

double One_elmt_PCG(int nsolve, double *A, double *Minv, double *p){
  register int i;
  int    iter = 0,info;
  double tolcg,  beta, eps, rtz_old, epsfac, rtz;
  double epsloc, alpha = 0, clk, cond;
  double *w,*u,*r,*z;
  double *beta_l,*alpha_l;/* define the vectors to be used in the Lanczos
                             Algorithm */
  double *D_l, *E_l;      /* define additional vectors for the Lanczos Alg. */
  int MIN_LANCZOS_ITER = iparam("MIN_LANCZOS_ITER");

  /* Temporary arrays */
  u  = dvector(0,nsolve);        /* Solution              */
  r  = dvector(0,nsolve);        /* residual              */
  z  = dvector(0,nsolve);        /* precondition solution */
  w  = dvector(0,nsolve);        /* A*Search direction    */

  /* Lanczos arrays */
  alpha_l = dvector(0,MAX_ITERATIONS); /* Lanczos Alg           */
  beta_l  = dvector(0,MAX_ITERATIONS); /* Lanczos Alg           */
  dzero (MAX_ITERATIONS+1,alpha_l,1);
  dzero (MAX_ITERATIONS+1, beta_l,1);

  dzero (nsolve, u, 1);
  dcopy (nsolve, p, 1, r, 1);

  tolcg  = dparam("TOLCG");
  epsfac = 1.0;

  /* =========================================================== *
   *            ---- Conjugate Gradient Iteration ----           *
   * =========================================================== */

  epsloc = ddot(nsolve,r,1,r,1);
  eps = sqrt(epsloc)*epsfac;

  if (option("verbose") > 1)    printf("\tInitial eps : %lg\n",eps);

  while ((eps > tolcg ||iter < MIN_LANCZOS_ITER)&& iter++ < MAX_ITERATIONS){

    /* forward multiply */
    dspmv('L',nsolve,1.0,Minv,r,1,0.0,z,1);

    rtz  = ddot (nsolve, r, 1, z, 1);

    if (iter > 1) {                           /* Update search direction */
      beta = rtz / rtz_old;
      beta_l[iter] = beta;
      dsvtvp(nsolve, beta, p, 1, z, 1, p, 1);
    }
    else
      dcopy(nsolve, z, 1, p, 1);

    /* forward multiply */
    dspmv('L',nsolve,1.0,A,p,1,0.0,w,1);

    alpha = rtz/ddot(nsolve, p, 1, w, 1);
    alpha_l[iter] = alpha;

    daxpy(nsolve, alpha, p, 1, u, 1);          /* Update solution...      */
    daxpy(nsolve,-alpha, w, 1, r, 1);          /* ...and residual         */

    epsloc = ddot(nsolve,r,1,r,1);

    rtz_old = rtz;
    eps = sqrt(epsloc)*epsfac;                 /* local L2 error       */
    fprintf(stdout,"%d: %lf\n",iter,eps);
  }

  /* =========================================================== *
   *                        End of Loop                          *
   * =========================================================== */

  /* Start to set up the Lanczos matrix, T, which is a tria-diagonal
     matrix and call the Lapack routine "dsterf" to compute all the
     eigenvalues of a tria-diagonal matrix */

  D_l = dvector(0, iter-1);  /* allocate memory */
  E_l = dvector(0, iter-1);
  dzero (iter, D_l, 1);
  dzero (iter, E_l, 1);

  D_l[0] = 1.0/alpha_l[1];

  for (i=1; i < iter; ++i)
    D_l[i] = beta_l[i+1]/alpha_l[i]  + 1.0/alpha_l[i+1];
  for (i=0;i<iter-1;++i)
    E_l[i] = -1.0*sqrt(beta_l[i+2])/(alpha_l[i+1]);

  dsterf(iter,D_l,E_l,info);
  if(info) error_msg("Lanczos Method, dsterf  -- info is not zero!!");

  /* print maximum and minimum eigenvalues */
  cond = D_l[iter-1]/D_l[0];

  /* Save solution and clean up */
  dcopy(nsolve,u,1,p,1);

  if (iter > nsolve*nsolve){
    fprintf(stderr,"iter: %d Max_iterations: %d\n",iter,
      nsolve*nsolve);
  }

  /* Free temporary vectors */
  free(u); free(r); free(w); free(z);
  free(D_l); free(E_l); free(alpha_l); free(beta_l);
  return cond;
}
