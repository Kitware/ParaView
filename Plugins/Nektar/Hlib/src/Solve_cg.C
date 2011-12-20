/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Hlib/src/Solve_cg.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 09:59:30 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <math.h>
#include <veclib.h>
#include "hotel.h"

#include <stdio.h>

//--------------------------------------------------------------------------
// WARNING THE FULL ITERATIVE SOLVER ASSUMES THAT THE SYSTEM IS NOT SINGULAR
// i.e. THERE MUST BE AT LEAST ONE FIXED DEGREE OF FREEDOM
//--------------------------------------------------------------------------


/* Private functions */
static void setup_RHS (Element_List *U, Element_List *F,
           double *r, double *u0,  Bndry *Ubc, Bsystem *B, SolveType Stype);
static void precon_cg(Element_List *E, Bsystem *B, SolveType Stype);
static void fill_helm_diag(Element_List *U, Metric *lambda);
static void fill_mass_diag(Element_List *U);

void A_cg(Element_List *U, Element_List *Uf, Bsystem *B,
    double *p, double *w, SolveType Stype);

/* ------------------------------------------------------------------------ *
 * Solve_CG() - Conjugate Gradient Solver                                   *
 *                                                                          *
 * This routine solves the algebraic system                                 *
 *                                                                          *
 *                            A u = - B f                                   *
 *                                                                          *
 * using the conjugate gradient method.  The inputs are the initial guess   *
 * "Uo" , the force "f", the boundary conditions and the global matrix      *
 * system.                                                                  *
 *                                                                          *
 * ------------------------------------------------------------------------ */
double *mult_cg;

static void scatter_modes(Element_List *U, double *u0, Bsystem *B);
static void gather_modes(Element_List *U, double *u0, Bsystem *B);
static void solve_system_CG (Element_List *U, Element_List *Uf,
          double *rhs, double *uo, Bsystem *B, SolveType Stype);

int     Ntot;

void Solve_CG(Element_List *U, Element_List *Uf, Bndry *Ubc, Bsystem *Ubsys,
     SolveType Stype){
  int i;
  Element *E;


  Ntot = Ubsys->nglobal;

  for(E=U->fhead;E;E = E->next)
    Ntot += E->Nmodes-E->Nbmodes;

  double *RHS = dvector(0,Ntot-1);
  double *U0  = dvector(0,Ntot-1);

  mult_cg = dvector(0, Ubsys->nsolve-1);
  dzero(Ubsys->nsolve,mult_cg,1);
  for(E=U->fhead;E;E = E->next)
    for(i=0;i<E->Nbmodes;++i)
      if(Ubsys->bmap[E->id][i] < Ubsys->nsolve)
  mult_cg[Ubsys->bmap[E->id][i]] += 1.;

  DO_PARALLEL
    parallel_gather(mult_cg, Ubsys);

  dzero(Ntot, RHS, 1);
  dzero(Ntot,  U0, 1);

#if 1
  if(!Ubsys->Pmat){
    setup_signchange(U,Ubsys);
    precon_cg(U,Ubsys,Stype);
  }
#endif
  setup_RHS      (U,Uf,RHS,U0,Ubc,Ubsys,Stype);
  solve_system_CG(U,Uf,RHS,U0,Ubsys,Stype);

  U->Set_state('t');

  free(RHS); free(U0);
}

double full_tol;
static double full_tolv[3];
static int    full_toli=0;

static void Set_tolerance(Element_List *E_L, Element_List *Ef_L){
  Element *E  = E_L->fhead;

  full_tol += ddot(Ef_L->hjtot, Ef_L->base_hj, 1, Ef_L->base_hj,1);

  /* gather full_tolerances together to make global full_tolerance */
  DO_PARALLEL{
    double tmp;
    gdsum(&full_tol,1,&tmp);
  }
  full_tol = sqrt(full_tol);

  if(E->type != 'p'){
    full_tolv[full_toli] = full_tol;

    if(E->dim() == 2){
      full_tol = sqrt(full_tolv[0]*full_tolv[0] +
          full_tolv[1]*full_tolv[1])/2.;
      full_toli = (++full_toli)%2;
    }
    else{
      full_tol = sqrt(full_tolv[0]*full_tolv[0] +
          full_tolv[1]*full_tolv[1] +full_tolv[2]*full_tolv[2])/3.;
      full_toli = (++full_toli)%3;
    }
  }

  /* for those wierd cases make sure full_tol is not less than 10e-3 */
  full_tol = (full_tol < 10e-3)? 1.0: full_tol;
}


/* ------------------------------------------------------------------------ *
 * Setup_RHS() - Compute the RHS of the algebraic system                    *
 *                                                                          *
 * This routine computes the initial residual of the discrete algebraic     *
 * system, defined as:                                                      *
 *                                                                          *
 *                       r = -(w,f) - (w,h)  +  a(w,g)                      *
 *                                         b                                *
 *                                                                          *
 * where (.,.)_b is the L2-inner product defined over the boundary.         *
 * It also returns the dsaveraged initial field in uo                       *
 * ------------------------------------------------------------------------ */
static void setup_RHS (Element_List *U, Element_List *F,
           double *r, double *u0,  Bndry *Ubc, Bsystem *B, SolveType Stype)
{
  int i;
  Bndry   *bc;
  Element  *E;
#if 1
  i = B->nglobal;
  SignChange(U,B);
  for(E=U->fhead;E;E=E->next){
    dscatr(E->Nbmodes,E->vert->hj,B->bmap[E->id],u0);
    dcopy (E->Nmodes-E->Nbmodes, E->vert[0].hj +E->Nbmodes, 1, u0+i, 1);
    i+= E->Nmodes-E->Nbmodes;
  }
  SignChange(U,B);
#else
  gather_modes(U, u0, B);
  dvdiv(B->nsolve, u0, 1, mult_cg, 1, u0, 1);
#endif
  if(F->fhead->state == 'p')
    F->Iprod(F);

  if(Stype == Helm)
    dneg (F->hjtot, F->base_hj, 1);

  for(bc = Ubc; bc; bc = bc->next)
    if(bc->type == 'F' || bc->type == 'R')
      F->flist[bc->elmt->id]->Add_flux_terms(bc);

  Set_tolerance(U,F); /* set solver tolerances */

  /* compute r = F - A u */
  U->Trans(U,J_to_Q);
  if(Stype == Helm)
    U->HelmHoltz (B->lambda);
  else
    U->Iprod(U);
  dvsub(U->hjtot, F->base_hj, 1, U->base_hj, 1, U->base_hj, 1);

  gather_modes(U, r, B);
  dzero(B->nglobal-B->nsolve, r+B->nsolve, 1); // mask knowns

  return;
}

static void full_gddot(double *alpha, double *r, double *s, Bsystem *B){
  *alpha = 0.0;
  int i;
  double tmp = 0.0;


  DO_PARALLEL{
    for(i=0;i<B->nsolve;++i)
      *alpha += B->pll->mult[i]*r[i]*s[i];

    *alpha += ddot(Ntot-B->nglobal, r+B->nglobal, 1, s+B->nglobal, 1);
    gdsum(alpha,1,&tmp);
  }
  else
    *alpha += ddot(Ntot, r, 1, s, 1);
}
void PreconFullOverlap(Bsystem *B, double *pin, double *zout);


#ifdef Lanczos
#define MAX_lanc_iter     990
#endif


static void solve_system_CG (Element_List *U, Element_List *Uf,
           double *rhs, double *uo, Bsystem *B, SolveType Stype){

  int    iter = 0;
  double tolcg, alpha, beta, eps, rtz, rtz_old, epsfac, epsloc;
  double *u = (double*)0;
  double *r = (double*)0;
  double *w = (double*)0;
  double *z = (double*)0;
  double *p = (double*)0;

  double *PC = (double*)0;
  int MAX_ITERATIONS;


  /* Temporary arrays */
  u  = dvector(0,Ntot-1);          /* Solution              */
  p  = dvector(0,Ntot-1);          /* Solution              */
  r  = dvector(0,Ntot-1);          /* residual              */
  z  = dvector(0,Ntot-1);          /* precondition solution */
  w  = dvector(0,Ntot-1);          /* A*Search direction    */

  dcopy (Ntot, rhs, 1, r, 1);
  dcopy (Ntot, rhs, 1, p, 1);
  dzero (Ntot, w, 1);
  dzero (Ntot, u, 1);
  dzero (Ntot, z, 1);


  PC = B->Pmat->info.diag.idiag;

  tolcg  = (U->fhead->type == 'p')? dparam("TOLCGP"):dparam("TOLCG");
  epsfac = (full_tol)? 1.0/full_tol : 1.0;
  full_gddot(&epsloc, r, r, B);
  eps = sqrt(epsloc)*epsfac;

  if (option("verbose") > 1)
    printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
     U->fhead->type, iter, eps/epsfac, epsfac, tolcg);

  DO_PARALLEL{
    if(B->pll->nsolve == B->pll->nglobal){  // i.e. singular solve
      fprintf(stderr, "singular full iterative solve\n");
      exit(-1);
    }
    MAX_ITERATIONS = B->pll->nsolve*10;
  }
  else
    {
      MAX_ITERATIONS = Ntot*10;

      if(B->nsolve == B->nglobal){  // i.e. singular solve
      fprintf(stderr, "singular full iterative solve\n");
      exit(-1);
      }
    }


#ifdef Lanczos
  double *beta_l,*alpha_l;/* define the vectors to be used in the Lanczos
                             Algorithm */
  double *D_l, *E_l;      /* define additional vectors for the Lanczos Alg. */
  int info =0;

  alpha_l = dvector(0,MAX_ITERATIONS-1); /* Lanczos Alg           */
  beta_l  = dvector(0,MAX_ITERATIONS-1); /* Lanczos Alg           */
  dzero (MAX_ITERATIONS,alpha_l,1);  /* zero the matrices  */
  dzero (MAX_ITERATIONS,beta_l,1);   /* zero the matrices  */
#endif




  /* =========================================================== *
   *            ---- Conjugate Gradient Iteration ----           *
   * =========================================================== */

  while (eps > tolcg && iter++ < MAX_ITERATIONS ){
    if(B->Precon == Pre_Diag)
      dvmul(Ntot,PC,1,r,1,z,1);
    else if(B->Precon == Pre_Overlap)
      PreconFullOverlap(B, r, z);
    full_gddot(&rtz, r, z, B);

    if (iter > 1) {                         /* Update search direction */
      beta = rtz / rtz_old;

#ifdef Lanczos
        if (iter <= MAX_lanc_iter)     beta_l[iter] = beta;
#endif

      dsvtvp(Ntot, beta, p, 1, z, 1, p, 1);
    }
    else
      dcopy(Ntot, z, 1, p, 1);

    A_cg(U,Uf,B,p,w,Stype);

    // need to do global sum ??
    full_gddot(&alpha, p, w, B);
    alpha = rtz/alpha;

#ifdef Lanczos
      if (iter <= MAX_lanc_iter)     alpha_l[iter] = alpha;
#endif


    daxpy(Ntot, alpha, p, 1, u, 1);            /* Update solution...   */
    daxpy(Ntot,-alpha, w, 1, r, 1);            /* ...and residual      */

    rtz_old = rtz;

    full_gddot(&epsloc, r, r, B);
    eps = sqrt(epsloc)*epsfac;

    ROOTONLY
      if (option("verbose") > 1)
  printf("\tField %c: %3d iterations, error = %#14.6g\n",
         U->fhead->type, iter, eps/epsfac);
  }

  /* =========================================================== *
   *                        End of Loop                          *
   * =========================================================== */

#ifdef Lanczos
/* Start to set up the Lanczos matrix, T, which is a tria-diagonal
   matrix and call the Lapack routine "dsterf" to compute all the
   eigenvalues of a tria-diagonal matrix */


  if (iter   > MAX_lanc_iter){
    iter = MAX_lanc_iter;
    printf("Lanczos: Conj.Gradient not fully converged, stop at 990 iter.\n");
  };

  D_l = dvector(0, iter-1);  /* allocate memory */
  E_l = dvector(0, iter-1);
  dzero (iter, D_l, 1);
  dzero (iter, E_l, 1);

  D_l[0] = 1.0/alpha_l[1];

  int i;
  for (i=1; i < iter; ++i)
    D_l[i] = beta_l[i+1]/alpha_l[i]  + 1.0/alpha_l[i+1];
  for (i=0;i<iter-1;++i)
    E_l[i] = -1.0*sqrt(beta_l[i+2])/(alpha_l[i+1]);

  dsterf(iter,D_l,E_l,info);
  if (info) error_msg("Lanczos Method, dsterf  -- info is not zero!!");

  /* print maximum and minimum eigenvalues */
  printf("Minimum and Maximum Eigenvalues are:%.8f,%.8f\n",D_l[0],D_l[iter-1]);
  printf("Condition number(MAXeig/MINeig) is:%.8f\n",(D_l[iter-1])/D_l[0]);
#endif



  /* Save solution and clean up */
  dvadd(Ntot,  u, 1, uo, 1, p, 1);
  scatter_modes(U, p, B);

  if (iter > MAX_ITERATIONS){
    error_msg (Bsolve_CG failed to converge);
  }
  else if (option("verbose") > 1)
    printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
     U->fhead->type, iter, eps/epsfac, epsfac, tolcg);

  /* Free temporary vectors */
  free(u); free(r); free(w); free(z); free(p);
  return;
}

static void precon_cg(Element_List *U, Bsystem *B, SolveType Stype){
  int      ltot;
  double   *p;
  double   *store;

  ltot = U->hjtot;

  store = dvector(0,ltot-1);  dzero(ltot, store, 1);
  p     = dvector(0,Ntot-1);  dzero(Ntot, p, 1);
  B->Pmat = (MatPre *) calloc(1,sizeof(MatPre));
  B->Pmat->info.diag.ndiag = Ntot;
  B->Pmat->info.diag.idiag = p;

  // store contents of E
  dcopy(ltot, U->base_hj,1,store,1);

  if(Stype == Helm)
    fill_helm_diag(U, B->lambda);
  else
    fill_mass_diag(U);

  // do "double sigchange since coefficients are all +ve
  SignChange(U,B);
  gather_modes(U, p, B);
  dvrecp(Ntot,p,1,p,1);

  dcopy (ltot, store, 1, U->base_hj, 1);

  free(store);

  return;
}

static void fill_mass_diag(Element_List *U){
  Element *E;

  for(E=U->fhead;E;E=E->next)
    E->fill_diag_massmat();
}

static void fill_helm_diag(Element_List *U, Metric *lambda){
  Element *E;

  for(E=U->fhead;E;E=E->next)
    E->fill_diag_helmmat(lambda+E->id);
}

static void gather_modes(Element_List *U, double *u0, Bsystem *B){
  register int  i;
  Element  *E = U->fhead;
  double  *sc = B->signchange;

  dzero (Ntot, u0, 1);
  for(;E;E = E->next){
    for(i = 0; i < E->Nbmodes; ++i)
      u0[B->bmap[E->id][i]] += sc[i]*E->vert[0].hj[i];
    sc += E->Nbmodes;
  }

  DO_PARALLEL
    parallel_gather(u0,B);

  u0 += B->nglobal;
  for(E=U->fhead;E;E=E->next){
    dcopy(E->Nmodes-E->Nbmodes, E->vert[0].hj+E->Nbmodes, 1, u0, 1);
    u0 += E->Nmodes-E->Nbmodes;
  }
}

static void scatter_modes(Element_List *U, double *p, Bsystem *B){
  Element *E = U->fhead;
  int cnt = B->nglobal;
  double  *sc = B->signchange;

  dzero(U->hjtot, U->base_hj, 1);

  for(;E;E = E->next){
    dgathr(E->Nbmodes, p, B->bmap[E->id], E->vert[0].hj);
    dvmul (E->Nbmodes, sc, 1, E->vert[0].hj, 1, E->vert[0].hj, 1);
    sc += E->Nbmodes;

    dcopy(E->Nmodes-E->Nbmodes, p+cnt, 1, E->vert[0].hj+E->Nbmodes, 1);
    cnt += E->Nmodes-E->Nbmodes;
  }
}

void A_cg(Element_List *U, Element_List *Uf, Bsystem *B,
    double *p, double *w, SolveType Stype){

  scatter_modes(U, p, B);

#if 1
  U->Trans(U,J_to_Q);
  if(Stype == Helm)
    U->HelmHoltz   (B->lambda);
  else
    U->Iprod(U);
#else
  Element *E;
  for(E=U->fhead;E;E=E->next){
    E->Trans(E, J_to_Q);
    E->HelmHoltz(B->lambda+E->id);
  }
#endif
  gather_modes(U, w, B);
  dzero(B->nglobal-B->nsolve, w+B->nsolve, 1);

}
