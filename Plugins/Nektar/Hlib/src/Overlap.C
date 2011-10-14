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

/* general utilities routines */
static void  PreconPatch(Bsystem *B, double *r, double *z);
void
APatch(Element_List *U, Element_List *Uf, Bsystem *B, double *p, double *w, int patchid);
void GathrBndryPatch(Element_List *U, double *u, Bsystem *B, int patchid);
static void
Bsolve_CGPatch(Element_List *U, Element_List *Uf, Bsystem *B, double *p);
void
  AOverlap(Element_List *U, Element_List *Uf, Bsystem *B, double *p, double *w);
void ScatrBndryOverlap(double *u, Element_List *U, Bsystem *B);
void GathrBndryOverlap(Element_List *U, double *u, Bsystem *B);
void SignChangeOverlap(Element_List *U, Bsystem *B);
void setup_signchangeOverlap(Element_List *U, Bsystem *B);

void GenPatches(Element_List *U, Element_List *Uf, Bsystem *B);
void PreconDirectOverlap(Bsystem *B, double *pin, double *zout);



void ReadPatchesDirect(char *fname, Element_List *U, Element_List *Uf,
           Bsystem *B){

  Element *E,*F;
  int  i,j,k,l;
  Vert *vb;
  int skip = 0;

  B->overlap         = (Overlap*) calloc(1, sizeof(Overlap));

  Overlap *OL = B->overlap;

  OL->type   = (OverlapType) iparam("NLAPTYPE");
  OL->coarse = iparam("NCOARSE");

  // Count total number of solves
  OL->Ntotal = B->nglobal;
  for(k=0;k<U->nel;++k)
    OL->Ntotal += U->flist[k]->Nmodes-U->flist[k]->Nbmodes;

  switch(OL->type){
  case File:
    {
      OL->npatches = iparam("NPATCHES");
      char buf[BUFSIZ];
      sprintf(buf, "%s.part.%d", fname, OL->npatches);

      FILE *fin = fopen(buf, "r");

      int *partition = ivector(0, U->nel-1);

      // FILE PARTITION
      for(k=0;k<U->nel;++k){
  fgets(buf, BUFSIZ, fin);
  sscanf(buf, "%d" , partition+k);
      }

      ++OL->npatches;

      // assumes vertex link list is set
      OL->maskflags = imatrix(0, OL->npatches-1, 0, U->nel-1);
      izero(OL->npatches*U->nel, OL->maskflags[0], 1);
      for(i=0;i<OL->npatches-1;++i){
  for(k=0;k<U->nel;++k){
    if(partition[k] == i){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
        for(F=U->fhead;F;F=F->next)
    for(l=0;l<F->Nverts;++l)
      if(F->vert[l].gid == E->vert[j].gid)
        OL->maskflags[i][F->id] = 1;
    }
  }
      }

      ifill(U->nel, 1, OL->maskflags[i], 1);

      OL->solveflags = dmatrix(0, OL->npatches-1, 0, B->nglobal-1);
      dfill(OL->npatches*B->nglobal, 1.0, OL->solveflags[0], 1);
      for(i=0;i<OL->npatches-1;++i){
  for(k=0;k<U->nel;++k)
    if(OL->maskflags[i][k] == 0)
      for(j=0;j<U->flist[k]->Nbmodes;++j)
        OL->solveflags[i][B->bmap[k][j]] = 0.;
  dzero(B->nglobal-B->nsolve, OL->solveflags[i]+B->nsolve, 1);
      }
      dzero(B->nglobal, OL->solveflags[i], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    OL->solveflags[i][B->bmap[k][j]] = 1.;

      free(partition);
      fclose(fin);
      break;
    }
  case VertexPatch:
    {
      Edge *eb;

      int cnt=0;

      // PATCH AROUND VERTEX + FILL INS + VERTEX PRECONDITIONER

      int extrapatches = 0, id;

      if(U->fhead->dim() == 2){

  OL->npatches = B->nv_solve +  1;

  OL->maskflags = imatrix(0, OL->npatches-1, 0, U->nel-1);
  izero(OL->npatches*U->nel, OL->maskflags[0], 1);

  // setup vertex
  cnt = 0;
  for(i=0;i<B->nsolve;++i){
    skip = 0;
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
        if(E->vert[j].gid == i){
    OL->maskflags[cnt][E->id] = 1;
    skip = 1;
        }
    }
    cnt += skip;
  }

  // find elements that do not belong to more than one patch

  for(k=0;k<U->nel;++k){
    E = U->flist[k];

    for(j=0;j<E->Nverts;++j)
      if(E->vert[j].solve)
        break;
    if(j==E->Nverts){
      for(j=0;j<E->Nedges;++j){
        if(E->edge[j].base){
    if(E->edge[j].link)
      id = E->edge[j].link->eid;
    else
      id = E->edge[j].base->eid;
    for(i=0;i<OL->npatches-1;++i)
      if(OL->maskflags[i][id]){
        OL->maskflags[i][E->id] = 1;
        fprintf(stdout, "Adding elmt: %d to part: %d\n",
          E->id, i);
        break;
      }
        }

      }
    }
  }
      }
      else{

  // use all global vertices to form patches

  OL->npatches = B->nv_solve +1;
  int *fl = ivector(0, B->nglobal-1);
  izero(B->nglobal, fl, 1);
  for(k=0;k<U->nel;++k){
    E = U->flist[k];
    for(j=0;j<E->Nverts;++j)
      if(!E->vert[j].solve && !fl[E->vert[j].gid]){
        fl[E->vert[j].gid] = 1;
        ++OL->npatches;
      }
  }

  OL->maskflags = imatrix(0, OL->npatches-1, 0, U->nel-1);
  izero(OL->npatches*U->nel, OL->maskflags[0], 1);

  // setup vertex
  cnt = 0;
  for(i=0;i<OL->npatches-1;++i){
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
        if(E->vert[j].gid == i){
    OL->maskflags[i][E->id] = 1;
        }
        else if(E->vert[j].gid == i+B->nsolve-B->nv_solve){
    OL->maskflags[i][E->id] = 1;
        }
    }
  }
      }

      ifill(U->nel, 1, OL->maskflags[OL->npatches-1], 1);
      OL->solveflags = dmatrix(0, OL->npatches-1, 0, B->nsolve-1);

      dfill(OL->npatches*B->nsolve, 1.0, OL->solveflags[0], 1);
      for(i=0;i<OL->npatches-1;++i){
  for(k=0;k<U->nel;++k)
    if(OL->maskflags[i][k] == 0){
      for(j=0;j<U->flist[k]->Nbmodes;++j)
        if(B->bmap[k][j] < B->nsolve)
    OL->solveflags[i][B->bmap[k][j]] = 0.;
    }
      }


      dzero(B->nsolve, OL->solveflags[i], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    if(B->bmap[k][j] < B->nsolve )
      OL->solveflags[i][B->bmap[k][j]] = 1.;

      break;
    }
  case ElementPatch:
    {
      // ELEMENT PATCHES + GLOBAL VERTEX PRECON

      int extrapatches = 0, id;

      OL->npatches = U->nel + 1;

      OL->solveflags = dmatrix(0, OL->npatches-1, 0, B->nsolve-1);

      dfill(OL->npatches*B->nsolve, 0.0, OL->solveflags[0], 1);

      for(k=0;k<U->nel;++k){
  E = U->flist[k];
  for(j=0;j<E->Nbmodes;++j)
    if(B->bmap[k][j] < B->nsolve){
      OL->solveflags[k][B->bmap[k][j]] = 1.;
    }
      }

      dzero(B->nsolve, OL->solveflags[U->nel], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    if(B->bmap[k][j] < B->nsolve )
      OL->solveflags[U->nel][B->bmap[k][j]] = 1.;
      break;
    }
  }

  OL->patchex = dvector(0, B->nsolve-1);
  dfill(B->nsolve, 1.0, OL->patchex, 1);
  for(i=0;i<OL->npatches;++i){
    for(j=0;j<B->nsolve;++j)
      if(OL->solveflags[i][j])
  OL->patchex[j] = 0.;
  }
#if 1
  for(j=0;j<B->nsolve;++j)
    if(OL->patchex[j])
      for(k=0;k<U->nel;++k)
  for(i=0;i<U->flist[k]->Nbmodes;++i)
    if(B->bmap[k][i] == j)
      fprintf(stderr, "Hole in elmt: %d bmode: %d\n", k+1, i);

#endif



  iparam_set("NPATCHES",OL->npatches);
  fprintf(stderr, "NPATCHES = %d\n", OL->npatches);

  double **save = dmatrix(0, 3, 0, max(U->htot,U->hjtot)-1);
  char ustate = U->fhead->state;
  char ufstate = Uf->fhead->state;
  dcopy(U->htot, U->base_h,  1, save[0], 1);
  dcopy(U->htot, Uf->base_h,  1, save[1], 1);
  dcopy(U->hjtot, U->base_hj, 1, save[2], 1);
  dcopy(U->hjtot, Uf->base_hj, 1, save[3], 1);

  GenPatches(U, Uf, B);
#if 0
  FILE *fmesh = fopen("patches.plt", "w");
  for(i=0;i<OL->npatches;++i){
    for(k=0;k<U->nel;++k){
      if(OL->maskflags[i][k]){
  E = U->flist[k];
  if(E->dim() == 2)
    dfill(E->qtot, (double)i, E->h[0], 1);
  else
    dfill(E->qtot, (double)i, E->h_3d[0][0], 1);
  E->dump_mesh(fmesh);
      }
    }
  }
  fclose(fmesh);

#endif
  dcopy(U->htot, save[0], 1, U->base_h, 1);
  dcopy(U->htot, save[1], 1, Uf->base_h, 1);
  dcopy(U->hjtot, save[2], 1, U->base_hj, 1);
  dcopy(U->hjtot, save[3], 1, Uf->base_hj, 1);
  U->Set_state(ustate);
  Uf->Set_state(ufstate);

  free_dmatrix(save, 0, 0);

}


#define MAX_ITERATIONS   max(nsolve,1000)
void  PreconOverlap(Element_List *U, Element_List *Uf,
         Bsystem *B, double *pin, double *zout){
  const  int nsolve = B->nsolve, nvs = B->nv_solve;
  int    iter = 0;
  double tolcg, alpha, beta, eps, rtz, rtz_old, epsfac;
  static double *u = (double*)0;
  static double *r = (double*)0;
  static double *w = (double*)0;
  static double *z = (double*)0;
  static double *p = (double*)0;
  static int nsol = 0, nglob = 0;
  int totiter = 0;
  Overlap *OL = B->overlap;

  //  Multi_RHS *mrhs;

  if(nsolve > nsol){
    if(nsol){
      free(u);      free(r);      free(z);
    }

    /* Temporary arrays */
    u  = dvector(0,B->nglobal-1);          /* Solution              */
    r  = dvector(0,B->nglobal-1);          /* residual              */
    z  = dvector(0,B->nglobal-1);          /* precondition solution */
    p  = dvector(0,B->nglobal-1);          /* precondition solution */
    nsol = nsolve;
  }
  if(B->nglobal > nglob){
    if(nglob)
      free(w);
    w  = dvector(0,B->nglobal-1);      /* A*Search direction    */
    nglob = B->nglobal;
  }

  int patchid = 0;
  dzero (nsolve, zout, 1);

  for(patchid=0;patchid<OL->npatches;++patchid){
    iter = 0;
    dzero (B->nglobal, w, 1);
    dzero (B->nglobal, r, 1);
    dzero (B->nglobal, p, 1);
    dzero (B->nglobal, u, 1);
    dvmul (B->nsolve, pin, 1, OL->solveflags[patchid], 1, r, 1);
    dvmul (B->nsolve, pin, 1, OL->solveflags[patchid], 1, p, 1);

    tolcg = dparam("TOLCGO");
    if(!tolcg)
      tolcg  = dparam("TOLCG");


    double tol = ddot(nsolve, r, 1, r, 1);
    tol = sqrt(tol);
    //    if(tol > dparam("TOLCG")){
      {
      epsfac = (tol)? 1.0/tol : 1.0;
      eps    = sqrt(ddot(nsolve,r,1,r,1))*epsfac;


#if 0
    if (option("verbose") > 1)
      printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
       U->fhead->type, iter, eps/epsfac, epsfac, tolcg);
#endif
    /* =========================================================== *
     *            ---- Conjugate Gradient Iteration for Patch      *
     * =========================================================== */

    while (eps > tolcg && iter++ < MAX_ITERATIONS )
      {
  PreconPatch(B,r,z);

  rtz  = ddot (nsolve, r, 1, z, 1);

  if (iter > 1) {                         /* Update search direction */
    beta = rtz / rtz_old;
    dsvtvp(nsolve, beta, p, 1, z, 1, p, 1);
  }
  else
    dcopy(nsolve, z, 1, p, 1);

  APatch(U,Uf,B,p,w,patchid);

  alpha = ddot(nsolve, p, 1, w, 1);
  if(alpha){
    alpha = rtz/alpha;

    daxpy(nsolve, alpha, p, 1, u, 1);            /* Update solution...   */
    daxpy(nsolve,-alpha, w, 1, r, 1);            /* ...and residual      */

    rtz_old = rtz;
    eps = sqrt(ddot(nsolve, r, 1, r, 1))*epsfac; /* Compute new L2-error */
  }
  else
    break;
#if 0
  printf("\tField %c: %3d iterations, error = %#14.6g %lg %lg\n",
         U->fhead->type, iter, eps/epsfac, epsfac, tolcg);
#endif
      }

    /* =========================================================== *
     *                        End of Loop                          *
     * =========================================================== */

    if (iter > MAX_ITERATIONS){
      printf("\tpatchid: %d Field %c: %3d iterations, error = %#14.6g %lg %lg\n",
       patchid, U->fhead->type, iter, eps/epsfac, epsfac, tolcg);
      error_msg (Patch Bsolve_CG failed to converge);
    }
#if 0
    else if (option("verbose") > 1)
      printf("\tpatchid: %d Field %c: %3d iterations, error = %#14.6g %lg %lg\n",
       patchid, U->fhead->type, iter, eps/epsfac, epsfac, tolcg);
#endif


    /* Save solution and clean up */
    dvadd(nsolve, u, 1, zout, 1, zout, 1);
    }
    totiter += iter;
  }

  if(!totiter)
    dcopy(nsolve, pin, 1, zout, 1);

  return;
}

/*

   Patch solver

   */

/* Scatter the points from u to U using the map in B->bmap */
void ScatrBndryPatch(double *u, Element_List *U, Bsystem *B, int patchid){
  register int i,k;
  int      nel = B->nel, Nbmodes;
  double *hj;
  double *sc = B->signchange;
  Overlap *OL = B->overlap;

  dvmul(B->nsolve, OL->solveflags[patchid], 1, u, 1, u, 1);

  for(k = 0; k < nel; ++k) {
    Nbmodes = U->flist[k]->Nbmodes;
    hj = U->flist[k]->vert->hj;

    dgathr(Nbmodes,  u, B->bmap[k], hj);
    dvmul (Nbmodes, sc, 1, hj, 1, hj,1);
    sc += Nbmodes;
  }

}

/* Gather points from U into 'u' using the map in B->bmap */
void GathrBndryPatch(Element_List *U, double *u, Bsystem *B, int patchid){
  register int i,k;
  int      *bmap, Nbmodes;
  double   *s;
  Overlap *OL = B->overlap;

  dzero(B->nglobal,u,1);

  double *sc = B->signchange;
  for(k = 0; k < B->nel; ++k){
    Nbmodes = U->flist[k]->Nbmodes;

    if(OL->maskflags[patchid][k] == 1){
      bmap = B->bmap[k];

      s = U->flist[k]->vert[0].hj;

      for(i = 0; i < Nbmodes; ++i)
  u[bmap[i]] += sc[i]*s[i];
    }
    sc += Nbmodes;
  }
  dvmul(B->nsolve, OL->solveflags[patchid], 1, u, 1, u, 1);
}

double dmax(int np, double *d, int skip){
  int i;
  double dm = -1e30;

  for(i=0;i<np;++i)
    dm = (d[i*skip] > dm) ? d[i*skip]:dm;
  return dm;
}

double dmin(int np, double *d, int skip){
  int i;
  double dm = 1e30;

  for(i=0;i<np;++i)
    dm = (d[i*skip] < dm) ? d[i*skip]:dm;
  return dm;
}


void
APatch(Element_List *U, Element_List *Uf, Bsystem *B, double *p, double *w, int patchid){

  int nel = B->nel;
  double   **a = B->Gmat->a;
  register int k;
  int i,j;

  Overlap *OL = B->overlap;

#if 0
  int cnt = 0;
  int totcnt = 0;
  FILE *fout = fopen("matrix.plt", "w");
  fprintf(fout, "VARIABLES = i,j,d\n");
  fprintf(fout, "ZONE F = POINT, I=%d, J=%d\n", B->nglobal, B->nglobal);

  totcnt = 0;

  double **matr = dmatrix(0, B->nglobal-1, 0, B->nglobal-1);

  for(i=0;i<B->nglobal;++i){

    dzero(B->nglobal, p, 1);
    p[i] = 1.;
#endif
    /* put p boundary modes into U and impose continuity */
    ScatrBndryPatch(p,Uf,B,patchid);
#if 0
    for(k = 0; k < nel; ++k){
      if(OL->maskflags[patchid][k] == 0)
  if(dmin( Uf->flist[k]->Nbmodes, Uf->flist[k]->vert[0].hj, 1) != 0
     || dmax(Uf->flist[k]->Nbmodes, Uf->flist[k]->vert[0].hj, 1) != 0)
    fprintf(stdout, "Non-zero after scatr in out-of-patch elmt\n");
    }
#endif

    for(k = 0; k < nel; ++k){
      if(OL->maskflags[patchid][k] == 1)
  dspmv('L',U->flist[k]->Nbmodes,1.0,a[U->flist[k]->geom->id],
        Uf->flist[k]->vert->hj,1,0.0,U->flist[k]->vert->hj,1);
#if 1
      else
  dzero(U->flist[k]->Nbmodes,U->flist[k]->vert->hj,1);
#endif
    }

    /* do sign change back and put into w */
    GathrBndryPatch(U,w,B,patchid);
#if 0
    for(j=0;j<B->nglobal;++j)
      fprintf(fout, "%d %d %lg\n", i,j,w[j]);
    dcopy(B->nglobal, w, 1, matr[i], 1);
    if(dmax(B->nglobal, w, 1) == 0 && dmin(B->nglobal, w, 1)  == 0)
      totcnt += 1;
  }
  fprintf(stdout, "%d zero rows\n", totcnt);


  totcnt = 0;
  for(i=0;i<B->nglobal;++i)
    if(dmax(B->nglobal, matr[0]+i, B->nglobal) ==0 &&
       dmin(B->nglobal, matr[0]+i, B->nglobal) ==0)
      totcnt += 1;

  fprintf(stdout, "%d zero cols\n", totcnt);


  for(i=0;i<B->nglobal;++i)
    if(matr[i][i] < 1e-7 && matr[i][i]<0)
      fprintf(stdout, "%d diag entry < 0\n", i);

  exit(-1);
#endif
}



static void  PreconPatch(Bsystem *B, double *r, double *z){
  switch(B->Precon){
  case Pre_Diag:
    dvmul(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,1,r,1,z,1);
    break;
  case Pre_Block:
    break;
  case Pre_None:
    break;
  case Pre_Overlap:
    dvmul(B->Pmat->info.overlap.ndiag,B->Pmat->info.overlap.idiag,1,r,1,z,1);
    break;
  }
}


void
Acolumn(Element_List *U, Element_List *Uf, Bsystem *B, int colid, double *col){

  int nel = B->nel;
  double   **a = B->Gmat->a;
  register int k;
  int i,j;

  dzero(B->nglobal, col, 1);
  col[colid] = 1.;

  ScatrBndry(col,Uf,B);

  for(k = 0; k < nel; ++k)
    dspmv('L',U->flist[k]->Nbmodes,1.0,a[U->flist[k]->geom->id],
    Uf->flist[k]->vert->hj,1,0.0,U->flist[k]->vert->hj,1);

  /* do sign change back and put into w */
  GathrBndry(U,col,B);
}

#if 0
extern "C"
{
 void  DSILUS(int *N,int *NELT, int *IA,int *JA, double *A, int *ISYM,
        int *NL, int *IL, int *JL, double *L, double *DINV,
        int *NU, int *IU, int *JU, double *U,
        int *NROW, int *NCOL);
}

class Slap {
public:
  int     N;
  double *B;
  double *X;
  int   NELT;
  int   *IA;
  int   *JA;
  double  *A;
  int   ISYM;
  int   ITOL;
  double TOL;
  int  ITMAX;
  int   ITER;
  double ERR;
  int   IERR;
  int  IUNIT;
  double *RWORK;
  int  LENW;

  int *IWORK;
  int LENIW;

  int NL;
  int NU;

  int    *IL;
  int    *JL;
  double *L;

  int    *IU;
  int    *JU;
  double *U;

  double *DINV;
  int   *NROW;
  int   *NCOL;


  Slap(double **mat, int L);
  void Solve(double *b, double *x);
  void Refresh(double **mat);
};



Slap::Slap(double **mat, int Len){
  int i,j, cnt=0, cnta=0;

  N = Len;
  B = NULL; // RHS vector
  X = NULL; // X_0 initial guess vector

  for(i=0;i<N*N;++i)
    cnt += (mat[0][i]) ? 1:0;
  NELT = cnt;

  A  = dvector(0, cnt-1);
  IA = ivector(0, cnt-1);
  JA = ivector(0, cnt-1);

  cnt = 0;
  for(i=0;i<N;++i){
    A[cnt] = mat[i][i];
    IA[cnt] = i+1;
    JA[i] = cnt+1;
    ++cnt;
    for(j=0;j<N;++j)
      if(i!=j && mat[j][i]){
  A[cnt] = mat[j][i];
  IA[cnt] = j+1;
  ++cnt;
      }
  }

  ISYM = 0;
  ITOL = 1;

  TOL = 1e-2;
  ITMAX = 1000;

  ITER = NULL;
  ERR  = NULL;
  IERR = NULL;
  IUNIT = NULL;


  NU = 0;
  for(i=0;i<N;++i)
    for(j=i;j<N;++j)
      NU += (mat[i][j]) ? 1:0;

  NL = 0;
  for(i=0;i<N;++i)
    for(j=0;j<i+1;++j)
      NL += (mat[i][j]) ? 1:0;

  LENW = NL+NU+4*N;
  IWORK = ivector(0, LENW-1);

  LENIW = NL+NU+4*N+10;
  RWORK = dvector(0, LENW-1);

  IL = ivector(0, NL-1);
  JL = ivector(0, NL-1);
   L = dvector(0, NL-1);

  IU = ivector(0, NU-1);
  JU = ivector(0, NU-1);
   U = dvector(0, NU-1);

   NROW= ivector(0, N-1);
   NCOL= ivector(0, N-1);

   /*
  DSILUS(&N,&NELT,IA,JA,A,&ISYM,
   &NL, IL, JL, L, DINV, &NU, IU, JU, U, NROW, NCOL);
   */
}

extern "C"{
void  dsilur_(int *N, double *B, double *X, int *NELT, int *IA, int *JA,
       double *A, int *ISYM, int *ITOL, double *TOL,
       int *ITMAX, int *ITER, double *ERR, int *IERR,
       int *IUNIT, double *WORK, int *LENW, int *IWORK, int *LENIW);
}
void Slap::Solve(double *b, double *x){

  B = b;
  X = x;

  //   DIR(&N, B, X, &NELT, IA, JA, A, &ISYM, DSMV, DSLUI,

  // need to break into factorization and solve
  dsilur_(&N, B, X, &NELT, IA, JA, A, &ISYM, &ITOL, &TOL,
   &ITMAX, &ITER, &ERR, &IERR, &IUNIT, RWORK, &LENW, IWORK, &LENIW);
  fprintf(stdout, "dsilur returns: %d\n", IERR);
}
void Slap::Refresh(double **mat){
  int i,j,cnt=0;

  for(i=0;i<N;++i){
    A[cnt] = mat[i][i];
    IA[cnt] = i+1;
    JA[i] = cnt+1;
    ++cnt;
    for(j=0;j<N;++j)
      if(i!=j && mat[j][i]){
  A[cnt] = mat[j][i];
  IA[cnt] = j+1;
  ++cnt;
      }
  }
}



Slap **slaps = NULL;

#endif


void GenPatches(Element_List *U, Element_List *Uf, Bsystem *B){
  int i,j,k,L;
  int cnt = 0;
  int skip;
  int maxlen = 0;
  Overlap *OL = B->overlap;

  OL->patchmaps    = (int **) calloc(OL->npatches, sizeof(int*));
  OL->patchlengths = ivector(0, OL->npatches-1);

  for(i=0;i<OL->npatches;++i){
    cnt = 0;
    for(j=0;j<B->nsolve;++j)
      cnt += (int) OL->solveflags[i][j];

    OL->patchlengths[i] = cnt;
    OL->patchmaps[i] = ivector(0, cnt-1);

    maxlen = max(maxlen, cnt);
    skip = 0;
    for(j=0;j<B->nsolve;++j)
      if(OL->solveflags[i][j]){
  OL->patchmaps[i][skip] = j;
  ++skip;
      }
    //    fprintf(stderr, "A dimension=%d\n",cnt);
  }

  fprintf(stderr, "Max local A dimension=%d\n", maxlen);

  int *pskip  = ivector(0, OL->npatches-1);
  izero(OL->npatches, pskip, 1);

  double ***A = 0;

  A = (double***) calloc(OL->npatches, sizeof(double**));
  for(j=0;j<OL->npatches;++j)
    if(OL->patchlengths[j])
      A[j] = dmatrix(0, OL->patchlengths[j]-1, 0, OL->patchlengths[j]-1);

  double *col = dvector(0, B->nglobal-1);
  for(i=0;i<B->nsolve;++i){
    Acolumn(U,Uf,B,i,col);
    for(j=0;j<OL->npatches;++j){
      if(pskip[j] != OL->patchlengths[j] &&
   OL->patchmaps[j][pskip[j]] == i){
  for(k=0;k<OL->patchlengths[j];++k)
    A[j][pskip[j]][k] = col[OL->patchmaps[j][k]];

  ++pskip[j];
      }
    }
  }

  fprintf(stderr, "Done local A construction\n");
  OL->patchbw = ivector(0, OL->npatches-1);
  izero(OL->npatches, OL->patchbw, 1);
  for(j=0;j<OL->npatches;++j){
    for(k=0;k<OL->patchlengths[j];++k)
      for(i=0;i<OL->patchlengths[j];++i)
  if(A[j][k][OL->patchlengths[j]-i-1]){
    OL->patchbw[j] = max(OL->patchlengths[j]-i-1-k,OL->patchbw[j]);
    break;
  }
    OL->patchbw[j] +=1;
    fprintf(stdout, "patch: %d rank: %d bw: %d\n", j,OL->patchlengths[j],OL->patchbw[j]);
  }

#if 0
  slaps = (Slap**) calloc(OL->npatches, sizeof(Slap*));

  for(i=0;i<OL->npatches;++i)
    slaps[i] = new Slap(A[i], OL->patchlengths[i]);
#endif

  OL->patchinvA = (double**) calloc(OL->npatches, sizeof(double*));
  for(j=0;j<OL->npatches;++j){
    L = OL->patchlengths[j];

    if(L){
      if(L> 2*OL->patchbw[j]){
  OL->patchinvA[j] = dvector(0, OL->patchbw[j]*L-1);
  PackMatrix(A[j], L, OL->patchinvA[j], OL->patchbw[j]);
  FacMatrix(OL->patchinvA[j],L,OL->patchbw[j]);
      }
      else{
  OL->patchinvA[j] = dvector(0, L*(L+1)/2-1);
  PackMatrix(A[j], L, OL->patchinvA[j], L);
  FacMatrix(OL->patchinvA[j],L,L);
      }
#if 0
      OL->patchinvA[j] = dvector(0, L-1);
      dzero(L, OL->patchinvA[j], 1);
      for(i=0;i<L;++i){
  for(k=0;k<L;++k)
    OL->patchinvA[j][i] +=  A[j][i][k];
  OL->patchinvA[j][i] = 1./OL->patchinvA[j][i];
      }
#endif
      free_dmatrix(A[j], 0, 0);

    }
  }


  fprintf(stderr, "Done local A factorizations\n");

}

void PreconDirectOverlap(Bsystem *B, double *pin, double *zout){
  double *tmp = dvector(0, B->nsolve-1);
  int i, j, k, L, *map, info;
  double *x = dvector(0, B->nsolve-1);
  Overlap *OL = B->overlap;

  dzero(B->nsolve, zout, 1);

  for(i=0;i<OL->npatches;++i){
    L   = OL->patchlengths[i];

    if(L){
      map = OL->patchmaps[i];
      for(j=0;j<L;++j)
  tmp[j] = pin[map[j]];

      if(L>2*OL->patchbw[i])
  dpbtrs('L',L,OL->patchbw[i]-1,1,OL->patchinvA[i],OL->patchbw[i],tmp,L,info);
      else
  dpptrs('L', L, 1, OL->patchinvA[i], tmp, L, info);

      //      dvmul(L, OL->patchinvA[i], 1, tmp, 1, tmp, 1);
#if 0
      dzero(B->nsolve, x, 1);
      slaps[i]->Refresh(A[i]);
      slaps[i]->Solve(tmp, x);
#endif
      for(j=0;j<L;++j)
  zout[map[j]] += tmp[j];
    }
  }

#if 0
  dvmul(B->nsolve, pin, 1, OL->patchex, 1, tmp, 1);
  dvvtvp(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,1,tmp,1,
   zout,1,zout,1);
#endif
  free(x);
  free(tmp);
}



#if 0
// stuff

void ReadPatchesOverlap(char *fname, Element_List *U, Bsystem *B){

  Element *E,*F;
  int  i,j,k,l;
  Vert *vb;
  int skip = 0;

  B->overlap         = (Overlap*) calloc(1, sizeof(Overlap));

  Overlap *OL = B->overlap;

  OL->type   = (OverlapType) iparam("OVERLAPTYPE");
  OL->coarse = iparam("OVERLAPCOARSE");
  OL->npatches = iparam("NPATCHES");
  // Count total number of solves
  OL->Ntotal = B->nsolve;

  switch(OL->type){
  case File:
    {
      char buf[BUFSIZ];
      sprintf(buf, "%s.part.%d", fname, B->npatches);

      FILE *fin = fopen(buf, "r");

      int *partition = ivector(0, U->nel-1);

      for(k=0;k<U->nel;++k){
  fgets(buf, BUFSIZ, fin);
  sscanf(buf, "%d" , partition+k);
      }

      ++B->npatches;

      // assumes vertex link list is set
      B->maskflags = imatrix(0, B->npatches-1, 0, U->nel-1);
      izero(B->npatches*U->nel, B->maskflags[0], 1);
      for(i=0;i<B->npatches-1;++i){
  for(k=0;k<U->nel;++k){
    if(partition[k] == i){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
        for(F=U->fhead;F;F=F->next)
    for(l=0;l<F->Nverts;++l)
      if(F->vert[l].gid == E->vert[j].gid)
        B->maskflags[i][F->id] = 1;
    }
  }
      }

      ifill(U->nel, 1, B->maskflags[i], 1);

      B->solveflags = dmatrix(0, B->npatches-1, 0, B->nglobal-1);
      dfill(B->npatches*B->nglobal, 1.0, B->solveflags[0], 1);
      for(i=0;i<B->npatches-1;++i){
  for(k=0;k<U->nel;++k)
    if(B->maskflags[i][k] == 0)
      for(j=0;j<U->flist[k]->Nbmodes;++j)
        B->solveflags[i][B->bmap[k][j]] = 0.;
  dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);
      }
      dzero(B->nglobal, B->solveflags[i], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    B->solveflags[i][B->bmap[k][j]] = 1.;
      dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);

      free(partition);
      fclose(fin);
    }

#if 0

  int extrapatches = 0;
  int *flags = ivector(0, B->nglobal-1);
  izero(B->nglobal, flags, 1);
  for(k=0;k<U->nel;++k){
    E = U->flist[k];
    for(j=0;j<E->Nverts;++j)
      if(E->vert[j].solve)
  break;
    if(j==E->Nverts){
      for(j=0;j<E->Nverts;++j){
  if(!flags[E->vert[j].gid]){
    ++extrapatches;
    flags[E->vert[j].gid] = 1;
  }
      }
    }
  }

  B->npatches = B->nv_solve + extrapatches + 1;

  B->maskflags = imatrix(0, B->npatches-1, 0, U->nel-1);
  izero(B->npatches*U->nel, B->maskflags[0], 1);

  // setup vertex
  for(i=0;i<B->nv_solve;++i)
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
  if(E->vert[j].gid == i)
    B->maskflags[i][E->id] = 1;
    }

  for(i=B->nsolve;i<B->nglobal;++i){
    if(flags[i]){
      for(k=0;k<U->nel;++k){
  E = U->flist[k];
  for(j=0;j<E->Nverts;++j)
    if(E->vert[j].gid == i)
      B->maskflags[skip+B->nv_solve][E->id] = 1;
      }
      ++skip;
    }
  }
  ifill(U->nel, 1, B->maskflags[B->npatches-1], 1);

  B->solveflags = dmatrix(0, B->npatches-1, 0, B->nglobal-1);
  dfill(B->npatches*B->nglobal, 1.0, B->solveflags[0], 1);
  for(i=0;i<B->npatches-1;++i){
    for(k=0;k<U->nel;++k)
      if(B->maskflags[i][k] == 0)
  for(j=0;j<U->flist[k]->Nbmodes;++j)
    B->solveflags[i][B->bmap[k][j]] = 0.;
    dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);
  }

  dzero(B->nglobal, B->solveflags[i], 1);
  for(k=0;k<U->nel;++k)
    for(j=0;j<U->flist[k]->Nverts;++j)
      B->solveflags[i][B->bmap[k][j]] = 1.;
  dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);
#endif


#if 0
  int maxvid = 0;
  for(k=0;k<U->nel;++k){
    E = U->flist[k];
    for(j=0;j<E->Nverts;++j)
      maxvid = max(E->vert[j].gid, maxvid);
  }
  B->npatches  = B->nv_solve + maxvid-B->nsolve +1;
  B->maskflags = imatrix(0, B->npatches-1, 0, U->nel-1);
  izero(B->npatches*U->nel, B->maskflags[0], 1);

  for(i=0;i<B->nv_solve;++i)
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
  if(E->vert[j].gid == i)
    B->maskflags[i][E->id] = 1;
    }

  for(i=0;i<maxvid-B->nsolve;++i)
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
  if((E->vert[j].gid - B->nsolve) == i)
    B->maskflags[i+B->nv_solve][E->id] = 1;
    }
  i = maxvid-B->nsolve + B->nv_solve;

  ifill(U->nel, 1, B->maskflags[i], 1);

  B->solveflags = dmatrix(0, B->npatches-1, 0, B->nglobal-1);
  dfill(B->npatches*B->nglobal, 1.0, B->solveflags[0], 1);
  for(i=0;i<B->npatches-1;++i){
    for(k=0;k<U->nel;++k)
      if(B->maskflags[i][k] == 0)
  for(j=0;j<U->flist[k]->Nbmodes;++j)
    B->solveflags[i][B->bmap[k][j]] = 0.;
    dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);
  }

  dzero(B->nglobal, B->solveflags[i], 1);
  for(k=0;k<U->nel;++k)
    for(j=0;j<U->flist[k]->Nverts;++j)
      B->solveflags[i][B->bmap[k][j]] = 1.;
  dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);
#endif
  iparam_set("NPATCHES",B->npatches);
  fprintf(stderr, "NPATCHES = %d\n", B->npatches);

  double *save = dvector(0, U->htot-1);
  dcopy(U->htot, U->base_h, 1, save, 1);
  FILE *fmesh = fopen("patches.plt", "w");
  for(i=0;i<B->npatches;++i){
    for(k=0;k<U->nel;++k){
      if(B->maskflags[i][k]){
  E = U->flist[k];
  if(E->dim() == 2)
    dfill(E->qtot, (double)i, E->h[0], 1);
  else
    dfill(E->qtot, (double)i, E->h_3d[0][0], 1);
  E->dump_mesh(fmesh);
      }
    }
  }
  dcopy(U->htot, save, 1, U->base_h, 1);
  fclose(fmesh);

#if 1
  for(i=0;i<B->npatches;++i){
    int cnt = 0;
    for(j=0;j<B->nsolve;++j)
      if(B->solveflags[i][j])
  ++cnt;
    fprintf(stderr, "Patchid: %d entries: %d\n", i,cnt);
  }
#endif
}


#if 0

  // EDGE PATCHES + GLOBAL VERTEX PRECON

  int extrapatches = 0, id;

  B->npatches = B->ne_solve + 1;

  B->solveflags = dmatrix(0, B->npatches-1, 0, B->nsolve-1);

  dfill(B->npatches*B->nsolve, 0.0, B->solveflags[0], 1);

  cnt = 0;
  for(i=0;i<B->nsolve;++i){
    skip = 0;
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nedges;++j)
  if(E->edge[j].gid == i){
    for(l=0;l<E->edge[j].l;++l)
      B->solveflags[cnt][B->edge[i]+l] = 1.;
    skip = 1;
  }
    }
    cnt += skip;
  }

  dzero(B->nsolve, B->solveflags[i], 1);
  for(k=0;k<U->nel;++k)
    for(j=0;j<U->flist[k]->Nverts;++j)
      if(B->bmap[k][j] < B->nsolve )
  B->solveflags[B->ne_solve][B->bmap[k][j]] = 1.;
#endif


#if 0
  int maxvid = 0;
  for(k=0;k<U->nel;++k){
    E = U->flist[k];
    for(j=0;j<E->Nverts;++j)
      maxvid = max(E->vert[j].gid, maxvid);
  }
  B->npatches  = B->nv_solve + maxvid-B->nsolve +1;
  B->maskflags = imatrix(0, B->npatches-1, 0, U->nel-1);
  izero(B->npatches*U->nel, B->maskflags[0], 1);

  for(i=0;i<B->nv_solve;++i)
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
  if(E->vert[j].gid == i)
    B->maskflags[i][E->id] = 1;
    }

  for(i=0;i<maxvid-B->nsolve;++i)
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
  if((E->vert[j].gid - B->nsolve) == i)
    B->maskflags[i+B->nv_solve][E->id] = 1;
    }
  i = maxvid-B->nsolve + B->nv_solve;


  ifill(U->nel, 1, B->maskflags[i], 1);

  B->solveflags = dmatrix(0, B->npatches-1, 0, B->nglobal-1);
  dfill(B->npatches*B->nglobal, 1.0, B->solveflags[0], 1);
  for(i=0;i<B->npatches-1;++i){
    for(k=0;k<U->nel;++k)
      if(B->maskflags[i][k] == 0)
  for(j=0;j<U->flist[k]->Nbmodes;++j)
    B->solveflags[i][B->bmap[k][j]] = 0.;
    dzero(B->nglobal-B->nsolve, B->solveflags[i]+B->nsolve, 1);
  }

  dzero(B->nglobal, B->solveflags[i], 1);
  for(k=0;k<U->nel;++k)
    for(j=0;j<U->flist[k]->Nverts;++j)
      B->solveflags[i][B->bmap[k][j]] = 1.;
#endif


#if 0

  // GLOBAL VERTEX PRECON

  int extrapatches = 0, id;

  B->npatches = 1;

  B->solveflags = dmatrix(0, B->npatches-1, 0, B->nsolve-1);

  dzero(B->nsolve, B->solveflags[0], 1);
  for(k=0;k<U->nel;++k)
    for(j=0;j<U->flist[k]->Nverts;++j)
      if(B->bmap[k][j] < B->nsolve )
  B->solveflags[0][B->bmap[k][j]] = 1.;
#endif

#endif
