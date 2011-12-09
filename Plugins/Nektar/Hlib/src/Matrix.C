/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//                                                                        //
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

static void Bsystem_mem (Bsystem *Ubsys, Element *U);
static void invert_a    (Bsystem *Ubsys);

void MemPrecon (Element *U, Bsystem *B);
static void ZeroPrecon(Bsystem *B, int dim);
static void FillPrecon(Element *E, Bsystem *B);
#if (defined(DUMPMASS) || defined(DUMPHELM))
static void dumpmat(Element *E,LocMat *mat);
#endif

static void Trans2LeBasis    (Element *U,Bsystem *B);
static void TransFromLeBasis (Element *U,Bsystem *B);

int bandwidthV(Element *E, Bsystem *B);

void Tri_HelmMat(Element *T, LocMat *helm, double lambda);


/* added by Leopold Grinber */
/* needed for parallel Lenergy Vertex solver */
void transform_Vec2Mat_index(int N, int k, int *row, int *col);



void GenMat(Element_List *U, Bndry *Ubc, Bsystem *Ubsys, Metric *lambda,
      SolveType Stype){
  LocMat  *mass = 0,*helm;
  const   int nel = U->nel;
  int     na,nb,nc,regen = 1;
  int     iter = (int) Ubsys->smeth;
  Bndry   *Bc;
  Element *E;
  static int SVV = dparam("SVVfactor") ? 1:0;

  double timer_start,timer_end,timer_start_2,timer_end_2;
  timer_start = dclock();

  if(Ubsys->Gmat)
    regen = 0;

  if(!Ubsys->signchange) setup_signchange(U,Ubsys);

  if(!Ubsys->Gmat)
    Bsystem_mem(Ubsys,U->fhead);
  else
    if(Ubsys->Precon == Pre_LEnergy) // transform back to std basis
      TransFromLeBasis(U->fhead,Ubsys);

  if(Ubsys->rslv) MemRecur(Ubsys,0,'p');

  if(Ubsys->smeth == iterative)
    if(!Ubsys->Pmat) /* only redeclare if not previously defined */
      MemPrecon(U->fhead,Ubsys);
    else
     if(option("ReCalcPrecon"))
      ZeroPrecon(Ubsys,U->fhead->dim());


  timer_end = dclock();
  ROOTONLY
     fprintf(stderr,"  GenMat: STEP 1  done in %f sec \n",timer_end-timer_start);

  timer_start = dclock();

  if(regen){ // only generate matrix if Ubsys not previously defined
    switch(Stype){
    case Mass:
      for(E = U->fhead; E;E=E->next){
  mass = E->mat_mem ();
  if(!iter||!Ubsys->Gmat->invc[E->geom->id]){
    if(!E->curvX)
      E->MassMat(mass);
    else
      E->MassMatC(mass);

    E->condense(mass,Ubsys,'y');

  }

  if(!iter) E->project(mass,Ubsys);
  E->mat_free (mass);
      }
      break;
    case Helm:
      for(E = U->fhead; E;E=E->next){
  helm = E->mat_mem ();
  if(Ubsys->rslv||
     (Ubsys->smeth == direct)||
     !Ubsys->Gmat->invc[E->geom->id]){

    if(E->identify()==Nek_Tri && !E->curvX && !lambda[E->id].p && !SVV)
      Tri_HelmMat(E, helm, lambda[E->id].d);
    else
      E->HelmMatC (helm,lambda+E->id);


    for(Bc=Ubc;Bc;Bc=Bc->next)
      if(Bc->type == 'R' && Bc->elmt->id == E->id)
        Add_robin_matrix(E, helm, Bc);

    E->condense  (helm,Ubsys,'y');
  }

  if(Ubsys->smeth == direct || Ubsys->rslv)
    E->project (helm,Ubsys);

  E->mat_free (helm);
      }

      DO_PARALLEL {
  if((!lambda || (!lambda->p && !lambda->d))
     && Ubsys->pll->nsolve == Ubsys->pll->nglobal){
    int i,k,gid;

    Ubsys->singular = 1;
    // set local singular map
    Ubsys->pll->singular = 0;
    for(k = 0; k < nel; ++k)
      for(i = 0; i < U->flist[k]->Nverts; ++i){
        gid = U->flist[k]->vert[i].gid;
        if(gid < Ubsys->pll->nv_lpsolve)
    if((Ubsys->singular-1) == Ubsys->pll->solvemap[gid])
      Ubsys->pll->singular = gid+1;
      }
  }
  else{
    Ubsys->singular = 0;
    Ubsys->pll->singular = 0;
  }
      }
      else
  if((!lambda || (!lambda->p && !lambda->d))
     &&(Ubsys->nsolve == Ubsys->nglobal)){
    Ubsys->singular = Ubsys->bmap[iparam("IESING")][iparam("IVSING")]+1;

    /* check to see if singular point is in inner boundary solve */
    if(Ubsys->rslv){
      int i,k;
      int cstart  = Ubsys->rslv->rdata[Ubsys->rslv->nrecur-1].cstart;
      if(Ubsys->singular > cstart){
        for(k = iparam("IESING"); k < nel; ++k)
    for(i = 0; i < U->flist[k]->Nverts; ++i)
      if(Ubsys->bmap[k][i] < cstart){
        Ubsys->singular = Ubsys->bmap[k][i] + 1;
        goto finished;
      }
        for(k = 0; k < iparam("IESING"); ++k)
    for(i = 0; i < U->flist[k]->Nverts; ++i)
      if(Ubsys->bmap[k][i] < cstart){
        Ubsys->singular = Ubsys->bmap[k][i] + 1;
        goto finished;
      }
      finished:;
      }
    }
  }
  else
    Ubsys->singular = 0;
    break;
    }
  }
  timer_end = dclock();
#ifdef PARALLEL
  gsync();
#endif
  ROOTONLY
     fprintf(stderr,"  GenMat: STEP 2  done in %f sec \n",timer_end-timer_start);
  timer_start = dclock();

  if(Ubsys->rslv){ /* recursive solver setup */
    register int i,j;
    double **a;
    Recur  *rdata;
    Condense_Recur(Ubsys,0,'p');

    for(i = 0; i < Ubsys->rslv->nrecur-1; ++i){
      rdata = Ubsys->rslv->rdata+i;
      a     = Ubsys->rslv->A.a;

      MemRecur(Ubsys,i+1,'p');

      for(j = 0; j < rdata->npatch; ++j)
  Project_Recur(j,rdata->patchlen_a[j],a[j],rdata->map[j],i+1,Ubsys);

      Condense_Recur(Ubsys,i+1,'p');

      for(j = 0; j < rdata->npatch; ++j) free(a[j]); free(a);
    }
    if(Ubsys->smeth == direct)
      Rinvert_a(Ubsys,'p');
    else
      Rpack_a  (Ubsys,'l');
  }
  else /* standard statically condensed set up */
    if(Ubsys->smeth == direct)
      invert_a  (Ubsys);
    else{
      if(option("ReCalcPrecon")){
        timer_start_2 = dclock();
  FillPrecon(U->fhead, Ubsys);
        timer_end_2 = dclock();
        ROOTONLY
           fprintf(stderr,"  GenMat: FillPrecon  done in %f sec \n",timer_end_2-timer_start_2);
  timer_start_2 = dclock();
        InvtPrecon(Ubsys);
        timer_end_2 = dclock();
        ROOTONLY
           fprintf(stderr,"  GenMat: InvtPrecon  done in %f sec \n",timer_end_2-timer_start_2);
      }
      else
  if(Ubsys->Precon == Pre_LEnergy){ /* transform basis for
            low energy preconditioner */
          timer_start_2 = dclock();
    Trans2LeBasis(U->fhead,Ubsys);
          timer_end_2 = dclock();
          ROOTONLY
            fprintf(stderr,"  GenMat: Trans2LeBasis  done in %f sec \n",timer_end_2-timer_start_2);
        }
    }

  /* zero U so don't start with funny values */
  U->zerofield();
  timer_end = dclock();
  ROOTONLY
     fprintf(stderr,"  GenMat: STEP 3  done in %f sec \n",timer_end-timer_start);

}

  static void invert_a(Bsystem *Ubsys){
    const int bwidth = Ubsys->Gmat->bwidth_a;
  const int nsolve = Ubsys->nsolve;
  int info=0;
  if(nsolve){
    if(2*bwidth < nsolve){    /* banded */
      if(Ubsys->singular){
  register int k;
  int gid = Ubsys->singular-1;

  dzero(bwidth,Ubsys->Gmat->inva[gid],1);
  for(k = 0; k < min(bwidth,gid+1); ++k)
    Ubsys->Gmat->inva[gid-k][k] = 0.0;
  Ubsys->Gmat->inva[gid][0] = 1.0;
      }
      dpbtrf('L', nsolve,   bwidth-1, *Ubsys->Gmat->inva, bwidth, info);
    }
    else{
      if(Ubsys->singular){  /* symmetric */
  register int k;
  int gid = Ubsys->singular-1;
  double *s;

  s = *Ubsys->Gmat->inva+gid;
  for(k = 0; k < gid; ++k, s += nsolve-k )
    s[0] = 0.0;
  dzero(nsolve-gid,s,1);
  s[0] = 1.0;
      }
      dpptrf('L', nsolve,   *Ubsys->Gmat->inva, info);
    }
  }
  if(info) fprintf(stderr,"invertA: info not zero\n");
}

static void Bsystem_mem(Bsystem *Ubsys, Element *U){
  const int nsolve = Ubsys->nsolve, nfam = Ubsys->families;

  Ubsys->Gmat = (MatSys  *)calloc(1,sizeof(MatSys));

  if(!Ubsys->rslv) /* if not a recursive solver */
    if(Ubsys->smeth == iterative)
      Ubsys->Gmat->a = (double **)calloc(nfam,sizeof(double *));
    else{
      if(nsolve){   /* banded solver */
  Ubsys->Gmat->bwidth_a = bandwidth(U,Ubsys);
  if(2*Ubsys->Gmat->bwidth_a < nsolve){
    Ubsys->Gmat->inva = dmatrix(0,nsolve-1,0,Ubsys->Gmat->bwidth_a-1);
    dzero(nsolve*Ubsys->Gmat->bwidth_a,*Ubsys->Gmat->inva,1);
  }
  else{      /* symmetric solver */
    Ubsys->Gmat->inva  = dmatrix(0,0,0,nsolve*(nsolve+1)/2-1);
    dzero(nsolve*(nsolve+1)/2,*Ubsys->Gmat->inva,1);
  }
      }
      else
  Ubsys->Gmat->inva = (double **)malloc(sizeof(double *));
    }

  Ubsys->Gmat->bwidth_c = ivector(0,nfam-1);
  Ubsys->Gmat-> invc    = (double **)calloc(nfam,sizeof(double *));
  Ubsys->Gmat->binvc    = (double **)calloc(nfam,sizeof(double *));

  return;
}

static void  SetLowEnergyModes(Element *U, Bsystem *B);
/* Matrix preconditioning system */
void MemPrecon(Element *U, Bsystem *B){
  register int i;
  int      gid,ll,tot;
  const    int nvs = B->nv_solve, nes = B->ne_solve;
  const    int nfs = B->nf_solve;
  Element  *E;
  MatPre   *M = B->Pmat = (MatPre *) calloc(1,sizeof(MatPre));

  switch(B->Precon){
  case Pre_Diag:
    M->info.diag.ndiag = B->nsolve;
    M->info.diag.idiag = dvector(0,B->nsolve-1);
    dzero(B->nsolve,M->info.diag.idiag,1);
    break;
  case Pre_Block:
    M->info.block.nvert = nvs;

    M->info.block.iedge = (double **) calloc((nes)?nes:1,sizeof(double *));
    M->info.block.Ledge = ivector(0,(nes)?nes-1:0);
    izero(nes, M->info.block.Ledge, 1);

    /* set edges */
    for(E=U;E;E=E->next)
      for(i = 0; i < E->Nedges; ++i){
  gid = E->edge[i].gid;
  if(gid < nes)
    M->info.block.Ledge[gid] = E->edge[i].l;
      }

    if(U->dim() == 3){
      M->info.block.iface = (double **) calloc((nfs)?nfs:1,sizeof(double *));
      M->info.block.Lface = ivector(0,(nfs)?nfs-1:0);
      izero(nfs, M->info.block.Lface, 1);

      /* faces */
      for(E=U;E;E=E->next)
  for(i = 0; i < E->Nfaces; ++i){
    gid = E->face[i].gid;
    ll = E->face[i].l;
    ll = (E->Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
    if(gid < nfs)
      M->info.block.Lface[gid] = ll;
  }
    }

    /* declare memory */
    M->info.block.ivert = dvector(0,nvs-1);
    dzero(nvs,M->info.block.ivert,1);

    for(i = 0; i < nes; ++i){
      ll = M->info.block.Ledge[i];
      ll = ll*(ll+1)/2;
      M->info.block.iedge[i] = dvector(0,ll-1);
      dzero(ll,M->info.block.iedge[i],1);
    }

    if(U->dim() == 3)
      for(i = 0; i < nfs; ++i){
  ll = M->info.block.Lface[i];
  ll = ll*(ll+1)/2;
  M->info.block.iface[i] = dvector(0,ll-1);
  dzero(ll,M->info.block.iface[i],1);
      }
    break;
  case Pre_Overlap:
    M->info.overlap.ndiag = B->nsolve;
    M->info.overlap.idiag = dvector(0,B->nsolve-1);
    dzero(B->nsolve,M->info.overlap.idiag,1);
    break;
  case Pre_None:
    break;
  case Pre_LEnergy: /* Low Energy vertex and edge preconditioner */
    int    doblock = (!option("NoVertBlock"));

    if(option("variable")) /* need mapping to be generalised */
      error_msg(Low Energy preconditioner not set up for variable order);

    SetLowEnergyModes(U,B);

    M->info.lenergy.mult = dvector(0,B->nsolve+1);
    dzero(B->nsolve,M->info.lenergy.mult,1);
    M->info.lenergy.nedge = nes;

    DO_PARALLEL{
#ifndef LE_VERT_OLD

      M->info.lenergy.nvert = B->pll->nv_gpsolve;
      M->info.lenergy.bw    = B->pll->nv_gpsolve;
      ll = B->nv_solve - B->pll->nv_lpsolve;

      M->info.lenergy.ivert_B = dmatrix(0,max(B->pll->nv_lpsolve-1,0),
          0,max(ll-1,0));

      dzero(ll*B->pll->nv_lpsolve,M->info.lenergy.ivert_B[0],1);
      M->info.lenergy.ivert_C = dmatrix(0,0,0,max(ll*(ll+1)/2-1,0));
      dzero(ll*(ll+1)/2,M->info.lenergy.ivert_C[0],1);
#else
      int wk;
      M->info.lenergy.nvert = B->pll->nv_solve;
      M->info.lenergy.bw    = bandwidthV(U,B);
      gimax(&(M->info.lenergy.bw),1,&wk);
#endif
    }
    else{
      M->info.lenergy.nvert = nvs;
      M->info.lenergy.bw    = bandwidthV(U,B);
    }

    M->info.lenergy.iedge = (double **) calloc((nes)?nes:1,sizeof(double *));
    M->info.lenergy.Ledge = ivector(0,(nes)?nes-1:0);
    izero(nes, M->info.lenergy.Ledge, 1);



    /* P - for ScaLapack, S - for Lapack*/
#ifdef PARALLEL
    M->info.lenergy.ivert_type = 'P';
#else
    M->info.lenergy.ivert_type = 'S';
#endif

    /* declare memory */
    if(ll = M->info.lenergy.nvert) {
      if(doblock)
  if(2*M->info.lenergy.bw < ll){
    if (M->info.lenergy.ivert_type == 'S'){
      M->info.lenergy.ivert = dmatrix(0,ll-1,0,M->info.lenergy.bw-1);
      dzero(ll*(M->info.lenergy.bw),M->info.lenergy.ivert[0],1);
    }
  }
  else{
    ll = ll*(ll+1)/2;
    if (M->info.lenergy.ivert_type == 'S'){
      M->info.lenergy.ivert = dmatrix(0,0,0,ll-1);
      dzero(ll,M->info.lenergy.ivert[0],1);
    }
  }
       /* create sparce matrix to store local values of ivert for Parallel Solver*/

      if (M->info.lenergy.ivert_type == 'P'){
  M->info.lenergy.SM_local = new SMatrix[1];
        /* initialize sparce matrix */
  M->info.lenergy.SM_local[0].allocate_SMatrix(1,M->info.lenergy.nvert,M->info.lenergy.nvert);
      }

      /* use B->nv_solve to deal with parallel case */
      M->info.lenergy.levert = dvector(0,B->nv_solve-1);
      dzero(B->nv_solve,M->info.lenergy.levert,1);
    }

    /* set edges */
    for(E=U;E;E=E->next)
      for(i = 0; i < E->Nedges; ++i){
  gid = E->edge[i].gid;
  if(gid < nes)
    M->info.lenergy.Ledge[gid] = E->edge[i].l;
      }

    M->info.lenergy.nface = nfs;
    M->info.lenergy.iface = (double **) calloc((nfs)?nfs:1,sizeof(double *));
    M->info.lenergy.Lface = ivector(0,(nfs)?nfs-1:0);
    izero(nfs, M->info.lenergy.Lface, 1);


    /* faces */
    for(E=U;E;E=E->next)
      for(i = 0; i < E->Nfaces; ++i){
  gid = E->face[i].gid;
  if(gid < nfs){
    ll  = E->face[i].l;
    ll  = (E->Nfverts(i) == 3)? ll*(ll+1)/2: ll*ll;
    M->info.lenergy.Lface[gid] = ll;
  }
      }

    for(i = 0,tot = 0; i < nes; ++i){
      ll = M->info.lenergy.Ledge[i];
      tot += ll*(ll+1)/2;
    }


    M->info.lenergy.iedge[0] = dvector(0,max(tot-1,0));
    dzero(tot,M->info.lenergy.iedge[0],1);


    for(i = 1; i < nes; ++i){
      ll = M->info.lenergy.Ledge[i-1];
      ll = ll*(ll+1)/2;
      M->info.lenergy.iedge[i] = M->info.lenergy.iedge[i-1] + ll;
    }

    for(i = 0,tot=0; i < nfs; ++i){
      ll  = M->info.lenergy.Lface[i];
      tot += ll*(ll+1)/2;
    }

    M->info.lenergy.iface[0] = dvector(0,max(tot-1,0));
    dzero(tot,M->info.lenergy.iface[0],1);

    for(i = 1; i < nfs; ++i){
      ll = M->info.lenergy.Lface[i-1];
      ll = ll*(ll+1)/2;
      M->info.lenergy.iface[i] = M->info.lenergy.iface[i-1] + ll;
    }
    break;
  }
}

/* Zero Matrix preconditioning system - needed for reuse*/
static void ZeroPrecon(Bsystem *B, int dim){
  register int i;
  int      gid,ll,tot;
  const    int nvs = B->nv_solve, nes = B->ne_solve;
  const    int nfs = B->nf_solve;
  MatPre   *M = B->Pmat;

  switch(B->Precon){
  case Pre_Diag:
    dzero(B->nsolve,M->info.diag.idiag,1);
    break;
  case Pre_Block:
    izero(nes, M->info.block.Ledge, 1);
    if(dim == 3)
      izero(nfs, M->info.block.Lface, 1);

    dzero(nvs,M->info.block.ivert,1);

    for(i = 0; i < nes; ++i){
      ll = M->info.block.Ledge[i];
      ll = ll*(ll+1)/2;
      dzero(ll,M->info.block.iedge[i],1);
    }

    if(dim == 3)
      for(i = 0; i < nfs; ++i){
  ll = M->info.block.Lface[i];
  ll = ll*(ll+1)/2;
  dzero(ll,M->info.block.iface[i],1);
      }
    break;
  case Pre_Overlap:
    dzero(B->nsolve,M->info.overlap.idiag,1);
    break;
  case Pre_None:
    break;
  case Pre_LEnergy: /* Low Energy vertex and edge preconditioner */
    int    doblock = (!option("NoVertBlock"));

    if(option("variable")) /* need mapping to be generalised */
      error_msg(Low Energy preconditioner not set up for variable order);

    dzero(B->nsolve,M->info.lenergy.mult,1);

    DO_PARALLEL{
#ifndef LE_VERT_OLD
      ll = B->nv_solve - B->pll->nv_lpsolve;
      dzero(ll*B->pll->nv_lpsolve,M->info.lenergy.ivert_B[0],1);
      dzero(ll*(ll+1)/2,M->info.lenergy.ivert_C[0],1);
#endif
    }
    izero(nes, M->info.lenergy.Ledge, 1);

    if(ll = M->info.lenergy.nvert) {
      if(doblock)
  if(2*M->info.lenergy.bw < ll){
          if (M->info.lenergy.ivert_type == 'S')
      dzero(ll*(M->info.lenergy.bw),M->info.lenergy.ivert[0],1);
  }
  else{
    ll = ll*(ll+1)/2;
          if (M->info.lenergy.ivert_type == 'S')
      dzero(ll,M->info.lenergy.ivert[0],1);
  }

      dzero(B->nv_solve,M->info.lenergy.levert,1);
    }

    izero(nfs, M->info.lenergy.Lface, 1);

    for(i = 0,tot = 0; i < nes; ++i){
      ll = M->info.lenergy.Ledge[i];
      tot += ll*(ll+1)/2;
    }
    dzero(tot,M->info.lenergy.iedge[0],1);

    for(i = 0,tot=0; i < nfs; ++i){
      ll  = M->info.lenergy.Lface[i];
      tot += ll*(ll+1)/2;
    }
    dzero(tot,M->info.lenergy.iface[0],1);
    break;
  }
}

static void FillPrecon(Element *U, Bsystem *B){
  register int i,j;
  const    int nvs = B->nv_solve;
  int      gid,pos,l,nbl;
  MatPre   *M = B->Pmat;
  double   *A;
  Element *E;

  switch(B->Precon){
  case Pre_Diag:
    dzero(B->nsolve,M->info.diag.idiag,1);
    break;
  case Pre_Block:
    dzero(nvs,M->info.block.ivert,1);

    for(i = 0; i < B->ne_solve; ++i){
      l = M->info.block.Ledge[i];
      l = l*(l+1)/2;
      dzero(l,M->info.block.iedge[i],1);
    }

    if(U->dim() == 3)
      for(i = 0; i < B->nf_solve; ++i){
  l = M->info.block.Lface[i];
  l = l*(l+1)/2;
  dzero(l,M->info.block.iface[i],1);
      }
    break;
  case Pre_None:
    break;
  case Pre_Overlap:
    dzero(B->nsolve,M->info.overlap.idiag,1);
    break;
  }

  switch(B->Precon){
  case Pre_Diag:
    for(E=U;E;E=E->next){
      A   = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;
      pos = 0;

      for(i=0;i<nbl;pos += nbl-i, ++i)
  if(B->bmap[E->id][i] < B->nsolve)
    M->info.diag.idiag[B->bmap[E->id][i]] += A[pos];
    }

    DO_PARALLEL
      parallel_gather(M->info.diag.idiag,B);
    break;
  case Pre_Block:
    fprintf(stderr,"Block Precondition is not set up in H_Matrix.C\n");
    exit(-1);
    break;
  case Pre_None:
    break;
  case Pre_LEnergy: /* Low Energy preconditioner */
    {
      int    Ne,Nf,k,gid,gid1,nel = B->nel;
      int    l1,cnt,sign,tot,h,nvs;
      int    bw = M->info.lenergy.bw;
      double **Rv, **Rvi, **Re, *s, *s1;
      int    doblock = (!option("NoVertBlock"));

      nvs = M->info.lenergy.nvert; /* reset to global (parallel) number */

      if(doblock){
#ifndef LE_VERT_OLD
  DO_PARALLEL{
    int id,id1;
          int ivert_index_vector, ivert_Row_index, ivert_Col_index;
    int nv_gpsolve = B->pll->nv_gpsolve;
    int nv_lpsolve = B->pll->nv_lpsolve;
    int csize = B->nv_solve - B->pll->nv_lpsolve;

    /* setup linear vertex system */
    for(E =U; E; E = E->next){
      A = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;

      for(i = 0,pos = 0; i < E->Nverts; ++i,pos+=nbl--){
        gid = E->vert[i].gid;
        if(gid < B->nv_solve){ // project matrix to local matrices
    if(gid < nv_lpsolve){ // boundary-boundary
      id = B->pll->solvemap[gid];

                  ivert_index_vector = (id*(id+1))/2+id*(nv_gpsolve-id);
                  transform_Vec2Mat_index(nvs, ivert_index_vector,  /* input */
                                          &ivert_Row_index, &ivert_Col_index ); /* output*/
                  if (M->info.lenergy.ivert_type == 'P')
                    M->info.lenergy.SM_local[0].add_point_value_SMatrix(ivert_Row_index,ivert_Col_index,A[pos]);
      else
        M->info.lenergy.ivert[0][(id*(id+1))/2+id*(nv_gpsolve-id)] += A[pos];
    }
    else{ // interior-interior
      M->info.lenergy.ivert_C[0][((gid-nv_lpsolve)*
        (gid-nv_lpsolve+1))/2+(gid-nv_lpsolve)*
              (csize-(gid-nv_lpsolve))] += A[pos];
    }

    for(j = i+1; j < E->Nverts; ++j){
      gid1 = E->vert[j].gid;
      if(gid1 < B->nv_solve){ // boundary-boundary
        if((gid < nv_lpsolve)&&(gid1 < nv_lpsolve)){
          id1 = B->pll->solvemap[gid1];
          if(id < id1){
                        ivert_index_vector = (id*(id+1))/2+id*(nv_gpsolve-id) + id1-id;
                        transform_Vec2Mat_index(nvs, ivert_index_vector,  /* input */
                                          &ivert_Row_index, &ivert_Col_index ); /* output*/
                        if (M->info.lenergy.ivert_type == 'P')
                          M->info.lenergy.SM_local[0].add_point_value_SMatrix(ivert_Row_index,ivert_Col_index,A[pos+j-i]);
                        else
        M->info.lenergy.ivert[0][(id*(id+1))/2+id*(nv_gpsolve-id) + id1-id] += A[pos+j-i];
          }
          else if(id == id1){/* special case of periodic element*/
                        ivert_index_vector = (id*(id+1))/2+id*(nv_gpsolve-id1);
                        transform_Vec2Mat_index(nvs, ivert_index_vector,  /* input */
                                          &ivert_Row_index, &ivert_Col_index ); /* output*/
                        if (M->info.lenergy.ivert_type == 'P')
                          M->info.lenergy.SM_local[0].add_point_value_SMatrix(ivert_Row_index,ivert_Col_index,2.0*A[pos+j-i]);
                        else
        M->info.lenergy.ivert[0][(id*(id+1))/2+id*(nv_gpsolve-id1)]  += 2*A[pos+j-i];
          }
          else{
                        ivert_index_vector = (id1*(id1+1))/2+id1*(nv_gpsolve-id1)+id-id1;
                        transform_Vec2Mat_index(nvs, ivert_index_vector,  /* input */
                                          &ivert_Row_index, &ivert_Col_index ); /* output*/
                        if (M->info.lenergy.ivert_type == 'P')
                          M->info.lenergy.SM_local[0].add_point_value_SMatrix(ivert_Row_index,ivert_Col_index,A[pos+j-i]);
                        else
        M->info.lenergy.ivert[0][(id1*(id1+1))/2 + id1*(nv_gpsolve-id1)+id-id1] += A[pos+j-i];
          }
        }  // interior-interior
        else if ((gid >= nv_lpsolve)&&(gid1 >= nv_lpsolve)){
          if(gid < gid1)
      M->info.lenergy.ivert_C[0][((gid-nv_lpsolve)*
         (gid-nv_lpsolve+1))/2+(gid-nv_lpsolve)*
         (csize - (gid-nv_lpsolve))+gid1-gid] += A[pos+j-i];
          else if(gid == gid1)/* special case of periodic element*/
      M->info.lenergy.ivert_C[0][((gid-nv_lpsolve)*
         (gid-nv_lpsolve+1))/2+ (gid-nv_lpsolve)*
          (csize - (gid1-nv_lpsolve))]  += 2*A[pos+j-i];
          else
      M->info.lenergy.ivert_C[0][((gid1-nv_lpsolve)*
          (gid1-nv_lpsolve+1))/2+(gid1-nv_lpsolve)*
          (csize-(gid1-nv_lpsolve))+gid-gid1] += A[pos+j-i];
        }
        else{ // interior-boundary
          if(gid1 >= nv_lpsolve)
      M->info.lenergy.ivert_B[gid][gid1-nv_lpsolve] +=
        A[pos+j-i];
          else // symmmetric case
      M->info.lenergy.ivert_B[gid1][gid-nv_lpsolve] +=
        A[pos+j-i];
        }
      }
    }
        }
      }
    }
  }
  else
#endif
  {
    /* setup linear vertex system */
    for(E =U; E; E = E->next){
      A = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;

      for(i = 0,pos = 0; i < E->Nverts; ++i,pos+=nbl--){
        gid = E->vert[i].gid;
        if(gid < B->nv_solve){
    DO_PARALLEL /* reset gid */
      gid = B->pll->solvemap[gid];
    if(2*bw < nvs){ /* banded */
      M->info.lenergy.ivert[gid][0] += A[pos];
    }
    else{
      M->info.lenergy.ivert[0][(gid*(gid+1))/2+gid*(nvs-gid)]+=
        A[pos];
    }
    for(j = i+1; j < E->Nverts; ++j){
      gid1 = E->vert[j].gid;
      if(gid1 < B->nv_solve){
        DO_PARALLEL /* reset gid1 */
          gid1 = B->pll->solvemap[gid1];
        if(2*bw < nvs){ /* banded */
          if(gid < gid1)
      M->info.lenergy.ivert[gid][gid1-gid]   += A[pos+j-i];
          else if(gid == gid1)/* special case of periodic element*/
      M->info.lenergy.ivert[gid][0]          += 2*A[pos+j-i];
          else
      M->info.lenergy.ivert[gid1][gid-gid1]  += A[pos+j-i];
        }
        else{ /* symmetric */
          if(gid < gid1)
      M->info.lenergy.ivert[0][(gid*(gid+1))/2+gid*(nvs-gid)
            + gid1-gid] += A[pos+j-i];
          else if(gid == gid1)/* special case of periodic element*/
      M->info.lenergy.ivert[0][(gid*(gid+1))/2+gid*(nvs-gid1)]
        += 2*A[pos+j-i];
          else
      M->info.lenergy.ivert[0][(gid1*(gid1+1))/2+
             gid1*(nvs-gid1)+gid-gid1]+= A[pos+j-i];
        }
      }
    }
        }
      }
    }
  }
      }

      /* Generat s2 */
      Trans2LeBasis(U,B);

      for(E=U;E;E = E->next){
  A = B->Gmat->a[E->geom->id];
  nbl = E->Nbmodes;

  gid = E->geom->id;
  for(i = 0; i < E->Nbmodes; ++i)
    if(B->bmap[E->id][i] < B->nsolve)
      M->info.lenergy.mult[B->bmap[E->id][i]] += 1;

  for(i = 0,pos=0; i < E->Nverts; pos+=nbl--,++i){
    gid = E->vert[i].gid;
    if(gid < B->nv_solve) /* use B definition for parallel solve */
        M->info.lenergy.levert[gid] += A[pos];
    }

  for(i = 0; i < E->Nedges; ++i){
    gid = E->edge[i].gid;
    l   = E->edge[i].l;
    if(gid < B->ne_solve){
      if(E->edge[i].con){
        for(j=0, s=M->info.lenergy.iedge[gid];j<l;s+=l-j,++j,pos+=nbl--)
    for(k=j,sign=1; k<l; ++k,sign=-sign)
      s[k-j] += sign*A[pos+k-j];
      }
      else
        for(j=0,s=M->info.lenergy.iedge[gid];j<l; s+=l-j, ++j,pos+=nbl--)
    dvadd(l-j,A+pos,1,s,1,s,1);
    }
    else
      for(j = 0; j < l; ++j,pos+=nbl--);
  }

  for(i = 0; i < E->Nfaces; ++i){
    gid = E->face[i].gid;
    l = E->face[i].l;
    if(E->Nfverts(i) == 3){
      l = l*(l+1)/2;
      if(gid < B->nf_solve){
        if(E->face[i].con){
    l = E->face[i].l;
    for(j = 0, s=M->info.lenergy.iface[gid]; j < l; ++j)
      for(k = 0; k < l-j; ++k, pos+=nbl--){
        dvadd(l-j-k,A+pos,1,s,1,s,1);
        s1 = A + pos + l-j-k; s += l-j-k;
        for(h = j+1,sign=-1; h < l; s+=l-h, s1+=l-h, ++h,
      sign=-sign)
          daxpy(l-h,sign,s1,1,s,1);
      }
        }
        else
    for(j=0,s=M->info.lenergy.iface[gid];j<l;s+=l-j,++j,pos+=nbl--)
      dvadd(l-j,A+pos,1,s,1,s,1);
      }
      else
        for(j = 0; j < l; ++j,pos+=nbl--);
    }
    else{ /* square face */
      int h1,sign1,sign2;
      l = l*l;
      if(gid < B->nf_solve){
        switch(E->face[i].con){
        case 0:
    for(j=0,s=M->info.lenergy.iface[gid];j<l;s+=l-j,++j,pos+=nbl--)
      dvadd(l-j,A+pos,1,s,1,s,1);
    break;
        case 1: case 5:// negate in 'a' direction ( a runs faster??)
    l = E->face[i].l;
    for(j = 0, s=M->info.lenergy.iface[gid]; j < l; ++j)
      for(k = 0, sign1=1; k < l; ++k, pos+=nbl--, sign1=-sign1){
        for(h = 0,sign=1; h < l-k; ++h,sign=-sign)
          s[h] += sign*A[pos + h];
        s1 = A + pos + l-k; s += l-k;
        for(h = j+1; h < l; ++h,s+=l,s1+=l)
          for(h1 = 0,sign=sign1; h1 < l; ++h1,sign=-sign)
      s[h1] += sign*s1[h1];
      }
    break;
        case 2: case 6:// negate in 'b' direction
    l = E->face[i].l;
    for(j = 0, s=M->info.lenergy.iface[gid]; j < l; ++j)
      for(k = 0; k < l; ++k, pos+=nbl--){
        for(h = 0; h < l-k; ++h)
          s[h] += A[pos + h];
        s1 = A + pos + l-k; s += l-k;
        for(h = j+1,sign1=-1; h < l; ++h,sign1=-sign1,s+=l,s1+=l)
          for(h1 = 0; h1 < l; ++h1)
      s[h1] += sign1*s1[h1];
      }
    break;
        case 3: case 7:// negate in both  directions
    l = E->face[i].l;
    for(j = 0, s=M->info.lenergy.iface[gid]; j < l; ++j)
      for(k = 0,sign2=1; k < l; ++k, pos+=nbl--,sign2=-sign2){
        for(h = 0,sign=1; h < l-k; ++h,sign=-sign)
          s[h] += sign*A[pos + h];
        s1 = A + pos + l-k; s += l-k;
        for(h = j+1,sign1=-1; h < l; ++h,sign1=-sign1,s+=l,s1+=l)
          for(h1 = 0,sign=sign2; h1 < l; ++h1,sign=-sign)
      s[h1] += sign1*sign*s1[h1];
      }
    break;
        }
      }
      else
        for(j = 0; j < l; ++j,pos+=nbl--);
    }
  }
      }

      DO_PARALLEL{
  double alpha = 0;
  parallel_gather(M->info.lenergy.mult,B);


  if(nvs){
    double *tmp;

    if(doblock){
      // currently new vertex summation done in invtprecon
#ifdef LE_VERT_OLD
      tmp = dvector(0,max(bw*nvs,nvs*(nvs+1)/2)-1);

      if(2*bw < nvs){    /* banded */
        gdsum(*B->Pmat->info.lenergy.ivert,bw*nvs,tmp);
      }
      else{
        gdsum(*B->Pmat->info.lenergy.ivert,nvs*(nvs+1)/2,tmp);
      }
      free(tmp);
#endif
    }

    tmp = dvector(0,B->nsolve-1);
    /* assume vertex are stored first */
    dzero(B->nsolve,tmp,1);
    dcopy(B->nv_solve,M->info.lenergy.levert,1,tmp,1);
    parallel_gather(tmp,B);
    dcopy(B->nv_solve,tmp,1,M->info.lenergy.levert,1);

    free(tmp);
  }
#ifdef PARALLEL
        gsync();
#endif
        ROOTONLY fprintf(stderr,"before  Set_Comm_GatherBlockMatrices \n");
  if(!B->egather)
    Set_Comm_GatherBlockMatrices(U,B);
#ifdef PARALLEL
        gsync();
#endif
        ROOTONLY fprintf(stderr,"after  Set_Comm_GatherBlockMatrices \n");

  GatherBlockMatrices(U,B);
#ifdef PARALLEL
        gsync();
#endif
        ROOTONLY fprintf(stderr,"after  GatherBlockMatrices \n");

      }
    }
  break;
  case Pre_Overlap:
#if 0
    for(E=U;E;E=E->next){
      A   = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;

      for(i = 0,pos = 0; i < E->Nverts; ++i,pos+=nbl--){
  gid = E->vert[i].gid;
  if(gid < nvs)
    M->info.overlap.idiag[gid] += A[pos];
      }

      for(i = 0; i < E->Nedges; ++i){
  gid = E->edge[i].gid;
  l   = E->edge[i].l;
  if(gid < B->ne_solve){
    for(j = 0; j < l; ++j,pos+=nbl--)
      M->info.overlap.idiag[B->edge[gid]+j] += A[pos];
  }
  else
    for(j = 0; j < l; ++j,pos+=nbl--);
      }

      if(E->dim() == 3)
  for(i = 0; i < E->Nfaces; ++i){
    gid = E->face[i].gid;
    l =  E->face[i].l;
    l = (E->Nfverts(i) == 3) ? l*(l+1)/2: l*l;
    if(gid < B->nf_solve){
      for(j = 0; j < l; ++j,pos+=nbl--){
        M->info.overlap.idiag[B->face[gid]+j] += A[pos];
      }
    }
    else
      for(j = 0; j < l; ++j,pos+=nbl--);
  }
    }
#endif
    for(E=U;E;E=E->next){
      A   = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;

      pos = 0;
      for(i=0;i<nbl;pos += nbl-i, ++i)
  if(B->bmap[E->id][i] < B->nsolve)
    M->info.diag.idiag[B->bmap[E->id][i]] += A[pos];
    }


    DO_PARALLEL
      parallel_gather(M->info.diag.idiag,B);
    break;
  }
}

static void Trans2LeBasis(Element *U, Bsystem *B){
  register int i,j;
  double **AL;
  double *A;
  int    *ifam = ivector(0,B->families-1),nbl;
  int    Ne, Nf, pos;
  double **Rv, **Rvi, **Re, *s, *s1;
  Element *E;
  double *tmp = dvector(0,8*(LGmax-1)*(LGmax-1)-1);

  ifill(B->families,1,ifam,1);

  for(E=U;E;E = E->next){
    if(ifam[E->geom->id]){

      A   = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;

      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);

      if(Ne){
  AL = dmatrix(0,nbl-1,0,nbl-1);
  /* copy out into full matrix */
  for(i = 0,pos = 0; i < nbl; ++i)
    for(j = i; j < nbl; ++j, ++pos)
      AL[i][j] = AL[j][i] = A[pos];

  /*AL R^t*/
  for(i = 0; i < nbl; ++i)
    RxV(AL[i],Rv,Re,E->Nverts,Ne,Nf,AL[i]);

  /* R AL */
  for(i = 0; i < nbl; ++i){
    dcopy(nbl,*AL+i,nbl,tmp,1);
    RxV(tmp,Rv,Re,E->Nverts,Ne,Nf,tmp);
    dcopy(nbl,tmp,1,*AL+i,nbl);
  }

  PackMatrix(AL,nbl,B->Gmat->a[E->geom->id],nbl);
  free_dmatrix(AL,0,0);
      }

      ifam[E->geom->id]=0;
    }
  }
  free(ifam); free(tmp);
}

static void TransFromLeBasis(Element *U,Bsystem *B){
  register int i,j;
  double **AL;
  double *A;
  int    *ifam = ivector(0,B->families-1),nbl;
  int    Ne, Nf, pos;
  double **Rv, **Rvi, **Re, *s, *s1;
  Element *E;
  double *tmp = dvector(0,8*(LGmax-1)*(LGmax-1)-1);

  ifill(B->families,1,ifam,1);

  for(E=U;E;E = E->next){
    if(ifam[E->geom->id]){

      A   = B->Gmat->a[E->geom->id];
      nbl = E->Nbmodes;

      E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);

      if(Ne){
  AL = dmatrix(0,nbl-1,0,nbl-1);
  /* copy out into full matrix */
  for(i = 0,pos = 0; i < nbl; ++i)
    for(j = i; j < nbl; ++j, ++pos)
      AL[i][j] = AL[j][i] = A[pos];

  /* AL R^t*/
  for(i = 0; i < nbl; ++i)
    IRxV(AL[i],Rvi,Re,E->Nverts,Ne,Nf,AL[i]);

  /* R AL */
  for(i = 0; i < nbl; ++i){
    dcopy(nbl,*AL+i,nbl,tmp,1);
    IRxV(tmp,Rvi,Re,E->Nverts,Ne,Nf,tmp);
    dcopy(nbl,tmp,1,*AL+i,nbl);
  }

  PackMatrix(AL,nbl,B->Gmat->a[E->geom->id],nbl);
  free_dmatrix(AL,0,0);
      }

      ifam[E->geom->id]=0;
    }
  }
  free(ifam); free(tmp);
}


void InvtPrecon(Bsystem *B){
  switch(B->Precon){
  case Pre_Diag:
    dvrecp(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,
     1,B->Pmat->info.diag.idiag,1);
    break;
  case Pre_Block:
    break;
  case Pre_Overlap:
    dvrecp(B->Pmat->info.overlap.ndiag,B->Pmat->info.overlap.idiag,
     1,B->Pmat->info.overlap.idiag,1);
    break;
  case Pre_None:
    break;
   case Pre_LEnergy: /* Low Energy basis preconditioner */
    {
      register int i;
      int    l;
      int    info=0,nvert,bwidth;
      int    doblock = (!option("NoVertBlock"));

      if((nvert = B->Pmat->info.lenergy.nvert)&&doblock){

  /* find bandwidth */
  bwidth = B->Pmat->info.lenergy.bw;
#ifndef LE_VERT_OLD
  DO_PARALLEL{
    int info,id,id1,j;
    int aglob = B->pll->nv_gpsolve;
    int asize = B->pll->nv_lpsolve;
    int csize = B->nv_solve -  B->pll->nv_lpsolve;
          int ivert_index_vector, ivert_Row_index, ivert_Col_index;

    double *inva;
          if ( B->Pmat->info.lenergy.ivert_type == 'S')
        inva  = B->Pmat->info.lenergy.ivert[0];

    double *invc  = B->Pmat->info.lenergy.ivert_C[0];
    double *binvc = B->Pmat->info.lenergy.ivert_B[0];
    double *tmp;
          double add_value = 0.0;


    if(csize) {
      tmp = dvector(0,csize*asize-1);

      /* calc invc(c) */
      FacMatrix(invc,csize,csize);

      /* save b */
      dcopy(csize*asize,binvc,1,tmp,1);

      /* calc inv(c)*b^T */
      dpptrs('L',csize,asize,invc,binvc,csize,info);
      if(info) fprintf(stderr,"error in inv(c) b^T \n");

#if 1
    static int FLAG_INDEX_IVERT1=0;

          FILE *sparse_ivert;
          char sparse_ivert_name[128];
          sprintf(sparse_ivert_name,"Asparse_ivert_%d_%d.dat",FLAG_INDEX_IVERT1,mynode());
          sparse_ivert = fopen(sparse_ivert_name,"w");
    B->Pmat->info.lenergy.SM_local[0].dump_SMatrix(sparse_ivert);
          fclose(sparse_ivert);
    FLAG_INDEX_IVERT1++;
#endif
      /* A - b inv(c) b^T */
      for(i = 0; i < asize; ++i)
        for(j = i; j < asize; ++j){
    id  = B->pll->solvemap[i];
    id1 = B->pll->solvemap[j];
    if(id < id1){

                  ivert_index_vector = id*(id+1)/2+(aglob-id)*id+id1-id;
                  transform_Vec2Mat_index(B->Pmat->info.lenergy.nvert,
                                          ivert_index_vector,  /* input */
                                          &ivert_Row_index, &ivert_Col_index ); /* output*/
                  add_value = -ddot(csize,tmp+i*csize,1,binvc+j*csize,1);

                  if (B->Pmat->info.lenergy.ivert_type == 'P')
                    B->Pmat->info.lenergy.SM_local[0].add_point_value_SMatrix(ivert_Row_index,ivert_Col_index,add_value);
                  else
                    inva[id*(id+1)/2+(aglob-id)*id+id1-id] += add_value;

      //inva[id*(id+1)/2+(aglob-id)*id+id1-id]
      //  -= ddot(csize,tmp+i*csize,1,binvc+j*csize,1);
    }
    else{

                  ivert_index_vector = id1*(id1+1)/2+(aglob-id1)*id1+id-id1;
                  transform_Vec2Mat_index(B->Pmat->info.lenergy.nvert,
                                          ivert_index_vector,  /* input */
                                          &ivert_Row_index, &ivert_Col_index ); /* output*/
                  add_value = -ddot(csize,tmp+i*csize,1,binvc+j*csize,1);
                  if (B->Pmat->info.lenergy.ivert_type == 'P')
                    B->Pmat->info.lenergy.SM_local[0].add_point_value_SMatrix(ivert_Row_index,ivert_Col_index,add_value);
                  else
        inva[id1*(id1+1)/2+(aglob-id1)*id1+id-id1] += add_value;

      //      inva[id1*(id1+1)/2+(aglob-id1)*id1+id-id1]
      //  -= ddot(csize,tmp+i*csize,1,binvc+j*csize,1);
    }
        }
      free(tmp);

    }

#if 1

          static int FLAG_INDEX_IVERT2=0;

          FILE *sparse_ivert;
          char sparse_ivert_name[128];
          sprintf(sparse_ivert_name,"Bsparse_ivert_%d_%d.dat",FLAG_INDEX_IVERT2,mynode());
          sparse_ivert = fopen(sparse_ivert_name,"w");
    B->Pmat->info.lenergy.SM_local[0].dump_SMatrix(sparse_ivert);
          fclose(sparse_ivert);
          FLAG_INDEX_IVERT2++;

      /* save ivert before gather */
    /*
           FILE *ivert_local;
            char fname_ivert_local[128];
            int itmp;
            sprintf(fname_ivert_local,"ivert_local_%d_%d.dat",FLAG_INDEX_IVERT,mynode());
            ivert_local = fopen(fname_ivert_local,"w");
            for (itmp = 0; itmp < aglob*(aglob+1)/2; itmp++)
        fprintf(ivert_local," %2.16f \n",inva[itmp]);
            fclose(ivert_local);
    */
#endif

    // gather together global matrix system . Should not be done if ScaLapeck is used
      if (B->Pmat->info.lenergy.ivert_type == 'S'){
          tmp = dvector(0,aglob*(aglob+1)/2-1);
        gdsum(inva,aglob*(aglob+1)/2,tmp);
        free(tmp);
      }

#if 0
      /* save ivert after gather */
            FILE *ivert_global;
            char fname_ivert_global[128];
            sprintf(fname_ivert_global,"ivert_global_%d_%d.dat",FLAG_INDEX_IVERT,mynode());
            ivert_global = fopen(fname_ivert_global,"w");
            for (itmp = 0; itmp < aglob*(aglob+1)/2; itmp++)
        fprintf(ivert_global," %2.16f \n",inva[itmp]);
            fclose(ivert_global);
#endif


      if (B->Pmat->info.lenergy.ivert_type == 'S'){
          if(B->singular){ /* zero out all but diagonal contribution */
    register int k;
    int gid = B->singular-1;
    double *s;

    s = inva + gid;
    for(k = 0; k < gid; ++k, s += aglob-k )
      s[0] = 0.0;
    dzero(aglob-gid,s,1);
    s[0] = 1.0;
        }


  /*            //  extract and invert diagonal of ivert
              B->Pmat->info.lenergy.inv_diag_ivert = dvector(0,aglob-1);
              j = 0;
              for (i = 0; i < aglob; i++){
                    B->Pmat->info.lenergy.inv_diag_ivert[i] = 1.0/inva[j];
                    j = j+(aglob-i);
              }
  */
        /* invert A - b inv(c) b^T */
        FacMatrix(inva,aglob,aglob);
      }
            else{
#ifdef PARALLEL
              /* initialize BLAC   */
        InitBLAC(B);
              /* redistribute and factorize inva */
              GatherScatterIvert(B);

              int first_row_col[2];
              matrix_2DcyclicToNormal(B->Pmat->info.lenergy.BLACS_PARAMS,B->Pmat->info.lenergy.ivert_local, &first_row_col[0]);
              B->Pmat->info.lenergy.first_row = first_row_col[0];
              B->Pmat->info.lenergy.first_col = first_row_col[1];
              parallel_dgemv_params(B->Pmat->info.lenergy.BLACS_PARAMS,B->Pmat->info.lenergy.col_displs,B->Pmat->info.lenergy.col_rcvcnt);
              set_mapping_parallel_dgemv(B);
#endif
            ;
      }


#if 0
      /* save ivert after factorization */
            sprintf(fname_ivert_global,"ivert_globalF_%d_%d.dat",FLAG_INDEX_IVERT,mynode());
            ivert_global = fopen(fname_ivert_global,"w");
            for (itmp = 0; itmp < aglob*(aglob+1)/2; itmp++)
        fprintf(ivert_global," %2.16f \n",inva[itmp]);
            fclose(ivert_global);
      FLAG_INDEX_IVERT++;
#endif

  } //end of DO_PARALLEL
  else
#endif
  {
    if(2*bwidth < nvert){    /* banded */
      if(B->singular){
        register int k;
        int gid = B->singular-1;

        dzero(bwidth,B->Pmat->info.lenergy.ivert[gid],1);
        for(k = 0; k < min(bwidth,gid+1); ++k)
    B->Pmat->info.lenergy.ivert[gid-k][k] = 0.0;
        B->Pmat->info.lenergy.ivert[gid][0] = 1.0;
      }
      dpbtrf('L',nvert,bwidth-1,B->Pmat->info.lenergy.ivert[0],
       bwidth,info);
    }
    else{
      if(B->singular){ /* zero out all but diagonal contribution */
        register int k;
        int gid = B->singular-1;
        double *s;

        s = B->Pmat->info.lenergy.ivert[0]+gid;
        for(k = 0; k < gid; ++k, s += B->Pmat->info.lenergy.nvert-k )
    s[0] = 0.0;
        dzero(B->Pmat->info.lenergy.nvert-gid,s,1);
        s[0] = 1.0;
      }

      dpptrf('L',nvert,B->Pmat->info.lenergy.ivert[0],info);
    }
    if(info) fprintf(stderr,"InvtPrecon: info not zero\n");
  }
      }

      if(B->Pmat->info.lenergy.nvert){
  if(B->nv_solve)
    dvrecp(B->nv_solve,B->Pmat->info.lenergy.levert,1,
     B->Pmat->info.lenergy.levert,1);
      }

      if(B->Pmat->info.lenergy.nedge||B->Pmat->info.lenergy.nface)
  dvrecp(B->nsolve,B->Pmat->info.lenergy.mult,1,
         B->Pmat->info.lenergy.mult,1);
      for(i = 0; i < B->Pmat->info.lenergy.nedge; ++i){
  if(l = B->Pmat->info.lenergy.Ledge[i]){
    dpptrf('L',l,B->Pmat->info.lenergy.iedge[i],info);
    if(info) fprintf(stderr,"info not zero\n");
  }
      }

      for(i = 0; i < B->Pmat->info.lenergy.nface; ++i)
  if(l = B->Pmat->info.lenergy.Lface[i]){
    dpptrf('L',l,B->Pmat->info.lenergy.iface[i],info);
    if(info) fprintf(stderr,"info not zero\n");
  }
    }
    break;
 }
}

/* pack 'a' matrix into 'b' */
void PackMatrix(double **a, int n, double *b, int bwidth){
  register int i;

  if(n>2*bwidth){ /* banded symmetric lower triangular form */
    double *s;

    for(i = 0,s=b; i < n-bwidth; ++i,s+=bwidth)
      dcopy(bwidth,a[i]+i,1,s,1);

    for(i = n-bwidth; i < n; ++i,s+=bwidth)
      dcopy(n-i,a[i]+i,1,s,1);
  }
  else{        /* symmetric lower triangular form */
    register int j;
    for(i = 0,j=0; i < n; j+=n-i++)
      dcopy(n-i, a[i]+i, 1, b+j, 1);
  }
}

/* factor positive symmertric and banded matrices */
void FacMatrix(double *a, int n, int bwidth){
  int info=0;
  if(n>2*bwidth)
    dpbtrf('L',n,bwidth-1,a,bwidth,info);
  else
    dpptrf('L',n,a,info);

  if(info) {
    fprintf(stderr, "FacMatrix: n=%d bw=%d info=%d\n",
      n,bwidth,info);
    error_msg(FacMatrixP: info not zero);
  }
}


void transform_Vec2Mat_index(int N, int k, int *row, int *col){
  int j, block_max;
  block_max = N-1;

  for (j = 0; j < N; j++){
     if (k > block_max)
        block_max += N-j-1;
     else{
        col[0] = j;
        row[0] = N-1-(block_max-k);
        break;
     }
   }
}


#if (defined(DUMPMASS) || defined(DUMPHELM))
static void dumpmat(Element *E,LocMat *mat){
  register int i,j;
  int asize;
  int csize;

  csize = E->Nbmodes;
  asize = E->Nmodes - csize;

  for(i=0; i< asize; ++i)
    for(j = 0; j < asize; ++j)
      if(fabs(mat->a[i][j]) > 1e-10)
  fprintf(stdout,"%d %d %lg \n",i,j,mat->a[i][j]);

  for(i=0; i< csize; ++i)
    for(j = 0; j < csize; ++j)
      if(fabs(mat->c[i][j]) > 1e-10)
  fprintf(stdout,"%d %d %lg \n",i+asize,j+asize,mat->c[i][j]);

  for(i = 0; i < asize; ++i)
    for(j = 0; j  < csize; ++j)
      if(fabs(mat->b[i][j])> 1e-10){
  fprintf(stdout,"%d %d %lg \n",i,j+asize,mat->b[i][j]);
  fprintf(stdout,"%d %d %lg \n",j+asize,i,mat->b[i][j]);
      }
}
#endif

int bandwidth(Element *E, Bsystem *Bsys){
  register int i;
  const    int nes = Bsys->ne_solve;
  int      high,low,bwidth=0;

  int      l;
  const    int nfs = Bsys->nf_solve;


  for(;E; E = E->next){
    low = 1000000; high = 0;
    for(i = 0; i < E->Nverts; ++i)
      if(E->vert[i].solve){
  low  = min(low, E->vert[i].gid);
  high = max(high,E->vert[i].gid);
      }

    for(i = 0; i < E->Nedges; ++i)
      if(E->edge[i].gid < nes){
  low  = min(low, Bsys->edge[E->edge[i].gid]);
  high = max(high,Bsys->edge[E->edge[i].gid] + E->edge[i].l);
      }

    // needs to be fixed
    if(E->dim() == 3)
      for(i = 0; i < E->Nfaces; ++i)
  if(E->face[i].gid < nfs){
    l = E->face[i].l;
    l = (E->Nfverts(i) == 3) ? l*(l+1)/2 : l*l;

    low  = min(low, Bsys->face[E->face[i].gid]);
    high = max(high,Bsys->face[E->face[i].gid] + l);
  }


    bwidth = ((high-low+1) > bwidth)? (high-low+1) : bwidth;
  }
  return bwidth;
}

int bandwidthV(Element *E, Bsystem *B){
  register int j;
  int      high,low,bwidth=0;

  DO_PARALLEL{
    int id;
    for(;E;E = E->next){
      low = B->pll->nsolve+1; high = 0;
      for(j = 0; j < E->Nverts; ++j)
  if(B->bmap[E->id][j] < B->nsolve){
    id = B->pll->solvemap[B->bmap[E->id][j]];
    low  = (id < low ) ? id : low;
    high = (id > high) ? id : high;
  }
      bwidth = ((high-low+1) > bwidth)? (high-low+1) : bwidth;
    }
  }
  else{
    for(;E;E = E->next){
      low = B->nsolve+1; high = 0;
      for(j = 0; j < E->Nverts; ++j)
  if(B->bmap[E->id][j] < B->nsolve){
    low  = (B->bmap[E->id][j] < low ) ? B->bmap[E->id][j] : low;
    high = (B->bmap[E->id][j] > high) ? B->bmap[E->id][j] : high;
  }

      bwidth = ((high-low+1) > bwidth)? (high-low+1) : bwidth;
    }
  }
  return bwidth;
}


void Bsystem_mem_free(Bsystem *Ubsys, Element_List *U){
  const int nsolve = Ubsys->nsolve, nfam = Ubsys->families;
  int i,j;

  if(!Ubsys->rslv) /* if not a recursive solver */
    if(Ubsys->smeth == iterative){
      for(i=0;i<nfam;++i)
  free(Ubsys->Gmat->a[i]);

      free(Ubsys->Gmat->a);
    }
    else{
      if(nsolve){   /* banded solver    */
  Ubsys->Gmat->bwidth_a = 0;
  free_dmatrix(Ubsys->Gmat->inva,0,0);
      }
      else
  free(Ubsys->Gmat->inva); // MIGHT NEED TO FIX
    }


  if(nfam>1)
    free(Ubsys->Gmat->bwidth_c);

  for(i=0;i<nfam;++i){
    free(Ubsys->Gmat->invc[i]);
    free(Ubsys->Gmat->binvc[i]);
  }
  free(Ubsys->Gmat->invc);
  free(Ubsys->Gmat->binvc);
  free(Ubsys->Gmat);

  Ubsys->Gmat = (MatSys *)NULL;

#if 0 //  re-use preconditioner memory
  if (Ubsys->smeth == iterative) {
    switch(Ubsys->Precon){
    case Pre_Diag:
      free(Ubsys->Pmat->info.diag.idiag);
      free(Ubsys->Pmat);
      break;
    case Pre_Block:
      break;
    case Pre_None:
      break;
    case Pre_Overlap:
      free(Ubsys->Pmat->info.overlap.idiag);
      free(Ubsys->Pmat);
    }
  }
#endif

  if (Ubsys->rslv) {
    Rsolver *rslv = Ubsys->rslv;
    Recur  *rdata;
    for(j=0;j<rslv->nrecur;++j){
      rdata = rslv->rdata + j;

      for(i = 0; i < rdata->npatch; ++i){
  if(rdata->patchlen_a[i])
    free(rdata->binvc[i]);

  free(rdata->invc[i]);
      }
      free(rdata->binvc);    rdata->binvc=NULL;
      free(rdata->invc);     rdata->invc =NULL;
      free(rdata->bwidth_c); rdata->bwidth_c=NULL;
    }

  }

  return;
}

void  Add_robin_matrix(Element *E, LocMat *helm, Bndry *rbc){
  int      edge = rbc->face;
  int      qa   = E->qa;
  double  *tmp,*z,*w, d;
  int     i,j;
  Basis   *b = E->getbasis();
  double  sigma = rbc->sigma;
  int     start, vn1, vn2, eni, enj;

  for(i=0, start=E->Nverts;i<edge;++i)
    start += E->edge[i].l;

  getzw(qa,&z,&w,'a');
  tmp = dvector(0,qa-1);

  /* Inner product of vertex modes (0,0)*/
  vn1 = E->vnum(edge,0);
  dvmul(qa,b->vert[0].a,1,w,1,tmp,1); // 0->i
  if(E->curve)
    dvmul(qa,rbc->sjac.p,1,tmp,1,tmp,1);
  else
    dsmul(qa,rbc->sjac.d,tmp,1,tmp,1);
  d =  ddot(qa,tmp,1,b->vert[0].a,1);

  helm->a[vn1][vn1] += sigma*d;

  /* Inner product of vertex modes (1,1)*/
  vn2 = E->vnum(edge,1);
  dvmul(qa,b->vert[1].a,1,w,1,tmp,1);
  if(E->curve)
    dvmul(qa,rbc->sjac.p,1,tmp,1,tmp,1);
  else
    dsmul(qa,rbc->sjac.d,tmp,1,tmp,1);
  d = ddot(qa,tmp,1,b->vert[1].a,1);
  helm->a[vn2][vn2] += sigma*d;

  /* Inner product of vertex modes (0,1)*/
  dvmul(qa,b->vert[0].a,1,w,1,tmp,1);
  if(E->curve)
    dvmul(qa,rbc->sjac.p,1,tmp,1,tmp,1);
  else
    dsmul(qa,rbc->sjac.d,tmp,1,tmp,1);
  d = ddot(qa,tmp,1,b->vert[1].a,1);
  helm->a[vn1][vn2] += sigma*d;
  helm->a[vn2][vn1] += sigma*d;

  /* Inner product of vertex0 with edge modes */
  dvmul(qa,b->vert[0].a,1,w,1,tmp,1);
  if(E->curve)
    dvmul(qa,rbc->sjac.p,1,tmp,1,tmp,1);
  else
    dsmul(qa,rbc->sjac.d,tmp,1,tmp,1);
  for(j = 0; j < E->edge[edge].l; ++j){
    enj = start+j;
    d = ddot(qa,tmp,1,b->edge[0][j].a,1);
    helm->a[enj][vn1] += sigma*d;
    helm->a[vn1][enj] += sigma*d;
  }

  /* Inner product of vertex1 with edge modes */
  dvmul(qa,b->vert[1].a,1,w,1,tmp,1);
  if(E->curve)
    dvmul(qa,rbc->sjac.p,1,tmp,1,tmp,1);
  else
    dsmul(qa,rbc->sjac.d,tmp,1,tmp,1);
  for(j = 0; j < E->edge[edge].l; ++j){
    enj = start+j;
    d = ddot(qa,tmp,1,b->edge[0][j].a,1);
    helm->a[enj][vn2] += sigma*d;
    helm->a[vn2][enj] += sigma*d;
  }

  /* Inner product of edge modes with edge modes */
  for(i = 0; i < E->edge[edge].l; ++i){
    dvmul(qa,b->edge[0][i].a,1,w,1,tmp,1);
    if(E->curve)
      dvmul(qa,rbc->sjac.p,1,tmp,1,tmp,1);
    else
      dsmul(qa,rbc->sjac.d,tmp,1,tmp,1);

    eni = start+i;
    for(j = i; j < E->edge[edge].l; ++j){
      enj = start+j;
      d = ddot(qa,tmp,1,b->edge[0][j].a,1);
      helm->a[eni][enj] += sigma*d;
      if(eni!=enj)
        helm->a[enj][eni] += sigma*d;
    }
  }

  //  plotdmatrix(helm->a, 0, helm->asize-1, 0, helm->asize-1);

  free(tmp);
}

void CorrectRobinFluxes(Bndry *B){
  register int i;
  const    int face = B->face;
  const    int qa = B->elmt->qa, l = B->elmt->edge[face].l;
  double   *wa,*tmp,*fi, fac;
  Mode     *m;
  Element  *E = B->elmt;
  Basis    *Base = E->getbasis();
  int       gid, vid, vn1, vn2;
  Bndry    *bc, *Ubc;

  fi  = dvector(0,qa-1);
  tmp = dvector(0,qa-1);

  for(Ubc=B;Ubc;Ubc=Ubc->next)
    if(Ubc->type=='R'){
      E = B->elmt;
      vn1 = E->vnum(B->face,0);
      vn2 = E->vnum(B->face,1);

      if(E->vert[vn1].solve && E->vert[vn2].solve) break;

      gid = E->vert[vn1].gid;
      getzw(qa,&wa,&wa,'a');
      /* Add shared, Dirichlet, vertex modes */
      if(!E->vert[vn1].solve)
        for(bc = Ubc; bc; bc=bc->next){
    E = bc->elmt;
          for(i=0;i<E->Nfverts(bc->face);++i){
            vid  = E->vnum(bc->face,i);
            if(gid == E->vert[vid].gid &&
         (bc->type == 'V' || bc->type == 'W')){
              fac = -bc->bvert[i]*bc->sigma;
              dsmul(qa,fac,Base->vert[0].a,1,fi,1);
              break;
            }
          }
  }

      gid = E->vert[vn2].gid;
      if(!E->vert[vn2].solve)
        for(bc = Ubc; bc; bc=bc->next){
    E = bc->elmt;
          for(i=0;i<E->Nfverts(bc->face);++i){
            vid  = E->vnum(bc->face,i);
            if(gid == E->vert[vid].gid &&
               (bc->type == 'V' || bc->type == 'W')){
              fac = -bc->bvert[i]*bc->sigma;
              dsvtvp(qa, fac, Base->vert[1].a, 1, fi, 1, fi, 1);
              break;
            }
          }
  }

      tmp = dvector(0,qa-1);
      m   = Base->edge[0];

      if(B->elmt->curve)
        dvmul(qa,B->sjac.p,1,fi,1,fi,1);
      else
        dsmul(qa,B->sjac.d,fi,1,fi,1);

      /* calculate inner product over surface */
      dvmul(qa,Base->vert[0].a,1,wa,1,tmp,1);
      B->bvert[0] += ddot(qa,tmp,1,fi,1);
      dvmul(qa,Base->vert[1].a,1,wa,1,tmp,1);
      B->bvert[1] += ddot(qa,tmp,1,fi,1);

      for(i = 0; i < l; ++i){
        dvmul(qa,m[i].a,1,wa,1,tmp,1);
        B->bedge[0][i] += ddot(qa,tmp,1,fi,1);
      }
    }

  free(tmp); free(fi);
}


void Setup_Preconditioner(Element_List *U, Bndry *Ubc, Bsystem *Ubsys,
        Metric *lambda, SolveType Stype){

  switch(Ubsys->Precon){
  case Pre_Diag:
    if(Ubsys->Pmat->info.diag.idiag)
      free(Ubsys->Pmat->info.diag.idiag);
    break;
  }
  if (Ubsys->smeth == iterative) MemPrecon(U->fhead,Ubsys);
  FillPrecon(U->fhead, Ubsys);
  InvtPrecon(Ubsys);
}

/* factor symmertric and banded matrices  which are not nec. positive definite*/
void FacMatrixSym(double *a, int *pivot, int n, int bwidth){
  int info;
  if(n>2*bwidth){
    error_msg(not implemented banded non-positive definite factorisation);
    dpbtrf('L',n,bwidth-1,a,bwidth,info);
  }
  else
    dsptrf('L',n,a,pivot,info);

  if(info) {
    fprintf(stderr, "FacMatrixSym: n=%d bw=%d info=%d\n",
      n,bwidth,info);
    error_msg(FacMatrixP: info not zero);
  }
}

// Helmholtz only

void UpdateMatList(Element_List *U, Bndry *Ubc, Bsystem *Ubsys,
       Metric *lambda, SolveType Stype,
       int *list, int init, int id){

  LocMat  *mass = 0,*helm, *oldhelm;
  int     na,nb,nc;
  Element *E;
  static double ****alist;
  double *dsave = dvector(0, LGmax*LGmax*LGmax-1);

  // If init is non-zero -> setup storage for a-b^t c^-1 b for each elmt
  //                        also setup general matrix storage

  if(init && Ubsys->smeth == direct){
    if(!alist)
      alist = (double****) calloc(MAXFIELDS, sizeof(double***));
    if(!alist[id])
      alist[id] = (double***) calloc(U->nel, sizeof(double**));
    Bsystem_mem(Ubsys,U->fhead);
    if (Ubsys->smeth == iterative)
      MemPrecon(U->fhead,Ubsys);
  }

  // setup dummy storage for project routine for unchanging elmts
  oldhelm = (LocMat*) calloc(1, sizeof(LocMat));

  for(E = U->fhead; E;E=E->next){
    if(list[E->id] || init){
      // save field values
      dcopy(E->Nmodes, E->vert[0].hj, 1, dsave, 1);

      // setup local storage for matrices
      helm = E->mat_mem ();
      if(lambda){
  mass = E->mat_mem ();
  na = mass->asize*mass->asize;
  nb = mass->asize*mass->csize;
  nc = mass->csize*mass->csize;
      }
      // generate elmemental matrices
      if(E->curvX || lambda[E->id].p)
  E->HelmMatC (helm,lambda+E->id);
      else{
  if(E->identify()==Nek_Tri)
    Tri_HelmMat(E, helm, lambda[E->id].d);
  else
    {
      E->LapMat(helm);

      if(lambda[E->id].d){
        E->MassMat(mass);
        daxpy(na,lambda[E->id].d,*mass->a,1,*helm->a,1);

        if(nc){
    daxpy(nb,lambda[E->id].d,*mass->b,1,*helm->b,1);
    daxpy(nc,lambda[E->id].d,*mass->c,1,*helm->c,1);
        }
      }
    }
      }
      // calculate a-b^t c^-1 b
      E->condense  (helm,Ubsys,'y');

      // store result
      if(init && Ubsys->smeth == direct){
  alist[id][E->id] = dmatrix(0, helm->asize-1, 0, helm->asize-1);
  dcopy(helm->asize*helm->asize, helm->a[0], 1,
        alist[id][E->id][0], 1);
      }

      // free mass matrix
      if(lambda) E->mat_free (mass);

      // project local bndry matrix to global matrix
      if(Ubsys->smeth == direct || Ubsys->rslv)
  E->project (helm,Ubsys);

      // free remaining temporary local matrices
      E->mat_free (helm);

      // restore field values
      dcopy(E->Nmodes, dsave, 1, E->vert[0].hj, 1);
    }
    else{
      // project stored a-b^t c^-1 b to global matrix
      if(Ubsys->smeth == direct || Ubsys->rslv){
  oldhelm->a = alist[id][E->id];
  E->project (oldhelm,Ubsys);
      }
    }
  }

  DO_PARALLEL {
    if((!lambda || (!lambda->p && !lambda->d))
       && Ubsys->pll->nsolve == Ubsys->pll->nglobal){
      int i,k;

      Ubsys->singular = 1;
      // set local singular map
      Ubsys->pll->singular = 0;
      for(k = 0; k < U->nel; ++k)
  for(i = 0; i < U->flist[k]->Nverts; ++i)
    if((Ubsys->singular-1) == Ubsys->pll->solvemap[Ubsys->bmap[k][i]])
      Ubsys->pll->singular = Ubsys->bmap[k][i]+1;
    }
    else{
      Ubsys->singular = 0;
      Ubsys->pll->singular = 0;
    }
  }
  else
    if((!lambda || (!lambda->p && !lambda->d))
       &&(Ubsys->nsolve == Ubsys->nglobal)){
      Ubsys->singular = Ubsys->bmap[iparam("IESING")][iparam("IVSING")]+1;
      }
    else
      Ubsys->singular = 0;

  if(Ubsys->smeth == direct)
    invert_a  (Ubsys);
  else{
    FillPrecon(U->fhead, Ubsys);
    InvtPrecon(Ubsys);
  }

  free(dsave);
  free(oldhelm);
}

  static void dump_le_modes(Element *E, double **Rv, double **Re, int nverts);

static void SetLowEnergyModes(Element *U, Bsystem *B){
  register int i;
  int *verts,Ne,Nf,ntypes;
  double **Rv,**Rvi,**Re;
  Element *E;

  verts = ivector(0,8);
  izero(9,verts,1);

  for(E=U; E; E=E->next)
    verts[E->Nverts] = 1;

  /* set up low energy matrices */
  for(i = 4;i < 9; ++i)
    if(verts[i])
      for(E=U;E;E=E->next)
  if(E->Nverts == i){
    E->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);
#if DUMPLEMODES
    dump_le_modes(E,Rv,Re,i);
#endif
    break;
  }

#if DUMPLEMODES
  fprintf(stderr,"Finished dumping low energy modes \n");
  exit(1);
#endif

  ntypes = isum(9,verts,1);

  if(verts[4]&&verts[6]){ /* if mixed element formulation then
           replace vertex, edge and triangular
           face modes with tet forms */
    int j,k;
    int cnt,cnt1,le,ll;
    int TNe,TNf,**id1,**id2;
    double **TRv,**TRvi,**TRe;
    Element *Et,*Ep;
    int Nverts,Nedges,Nfaces;

    /* search for a tetrahedral element */
    for(Et = U;Et; Et = Et->next)
      if(Et->Nverts == 4)
  break;

    /* search for a prismatic element */
    for(Ep = U;Ep; Ep = Ep->next)
      if(Ep->Nverts == 6)
  break;

    Et->LowEnergyModes(B,&TNe,&TNf,&TRv,&TRvi,&TRe);
    Ep->LowEnergyModes(B,&Ne,&Nf,&Rv,&Rvi,&Re);

    Nverts = Ep->Nverts;
    Nedges = Ep->Nedges;
    Nfaces = Ep->Nfaces;

    id1 = imatrix(0,Nedges-1,0,Nedges-1);
    id2 = imatrix(0,Nedges-1,0,Nedges-1);

    /* define edges of tet to be used around prism vertices */
    /* use edge id1[i][j] of vertex id2[i][j] from the tet definition
       on vertex i, edge j of the prism definition            */
    ifill(Nedges*Nedges,-1,id1[0],1);
    ifill(Nedges*Nedges,-1,id2[0],1);

    id1[0][0] = 0;     id1[0][3] = 2;     id1[0][4] = 3;
    id2[0][0] = 0;     id2[0][3] = 0;     id2[0][4] = 0;

    id1[1][0] = 0;     id1[1][1] = 1;     id1[1][5] = 4;
    id2[1][0] = 1;     id2[1][1] = 1;     id2[1][5] = 1;

    id1[2][1] = 1;     id1[2][2] = 0;     id1[2][6] = 5;
    id2[2][1] = 2;     id2[2][2] = 1;     id2[2][6] = 2;

    id1[3][3] = 2;     id1[3][2] = 0;     id1[3][7] = 5;
    id2[3][3] = 2;     id2[3][2] = 0;     id2[3][7] = 2;

    id1[4][4] = 3;     id1[4][5] = 4;     id1[4][8] = 2;
    id2[4][4] = 3;     id2[4][5] = 3;     id2[4][8] = 0;

    id1[5][6] = 3;     id1[5][7] = 4;     id1[5][8] = 2;
    id2[5][6] = 3;     id2[5][7] = 3;     id2[5][8] = 2;

    ll = Ep->edge->l;
    for(i = 0; i < Nverts; ++i)
      for(j = 0; j < Nedges; ++j)
  if((id1[i][j]+1)&&(ll))
    dcopy(ll,TRv[id2[i][j]] + id1[i][j]*ll,1,Rv[i]+j*ll,1);

    if(TNf){
      ifill(Nedges*Nedges,-1,id1[0],1);
      ifill(Nedges*Nedges,-1,id2[0],1);

      /* use face id1[i][j] of vertex id2[i][j] from the tet definition
   on vertex i, face j of the prism definition            */

      id1[0][1] = 1;     id1[1][1] = 1;    id1[2][3] = 1;
      id2[0][1] = 0;     id2[1][1] = 1;    id2[2][3] = 1;

      id1[3][3] = 1;     id1[4][1] = 1;    id1[5][3] = 1;
      id2[3][3] = 0;     id2[4][1] = 3;    id2[5][3] = 3;

      ll = Ep->face[1].l*(Ep->face[1].l+1)/2;

      cnt1 = Et->edge->l*Et->Nedges;
      for(i = 0; i < Nverts; ++i){
  cnt  = Ep->edge->l*Nedges;
  for(j = 0; j < Nfaces; ++j){
    if(id1[i][j]+1)
      dcopy(ll,TRv[id2[i][j]] + cnt1 + id1[i][j]*ll,1,Rv[i] + cnt,1);

    if(Ep->Nfverts(j) == 3)
      cnt += ll;
    else
      cnt += Ep->face[j].l*Ep->face[j].l;
  }
      }

      /* use face id1[i][j] of edge id2[i][j] from the tet definition
   on edge i, face j of the prism definition            */

      ifill(Nedges*Nedges,-1,id1[0],1);
      ifill(Nedges*Nedges,-1,id2[0],1);

      id1[0][1] = 1;     id1[2][3] = 1;    id1[4][1] = 1;
      id2[0][1] = 0;     id2[2][3] = 0;    id2[4][1] = 3;

      id1[5][1] = 1;     id1[6][3] = 1;    id1[7][3] = 1;
      id2[5][1] = 4;     id2[6][3] = 4;    id2[7][3] = 3;

      ll = Ep->face[1].l*(Ep->face[1].l+1)/2;
      le = Ep->edge->l;
      for(i = 0; i < Nedges; ++i)
  for(k= 0; k < Ep->edge[i].l; ++k){
    for(j = 0, cnt = 0; j < Nfaces; ++j){
      if(id1[i][j]+1)
        dcopy(ll,TRe[le*id2[i][j]+k]+id1[i][j]*ll,1,Re[i*le+k]+cnt,1);

      if(Ep->Nfverts(j) == 3)
        cnt += ll;
      else
        cnt += Ep->face[j].l*Ep->face[j].l;
    }
  }
    }
    free_imatrix(id1,0,0); free_imatrix(id2,0,0);

    /* calculate inverse */
    dsmul(Nverts*(Ne + Nf),-1,Rv[0],1,Rvi[0],1);
    if(Nf)
      for(i = 0; i < Nverts; ++i)
  dgemv('N',Nf,Ne,-1.0,Re[0],Nf,Rvi[i],1,1.0,Rvi[i]+Ne,1);

  }
  else if(ntypes >1 )
    fprintf(stderr,"Warning LowEnergyModes may not be compatible");
}

static void  dump_le_modes(Element *E, double **Rv, double **Re,int nverts){
  register int i,j,l,k,k1;
  double nx1,ny1,nz1,nx2,ny2,nz2,nx,ny,nz;
  double *z, *w,fac;
  double scal,oset;
  int qa = E->qa, qb = E->qb, qc = E->qc,l1, Nf;
  FILE *fp;
  char file[BUFSIZ];
  Coord X;
  double *save;
  Basis *b;

  save = dvector(0,E->Nmodes-1);
  dcopy(E->Nmodes,E->vert->hj,1,save,1);

  reset_bases();

  scal = dparam("lescal");
  if(!scal)
    scal = 1.0;
  oset = dparam("leoffset");

  /* redefine basis to be equispaced */
  getzw(qa,&z,&w,'a');
  for(i = 0; i < qa; ++i)
    z[i] = 2.0*i/(double)(qa-1)-1.0;
  getzw(qb,&z,&w,'b');
  for(i = 0; i < qb; ++i)
    z[i] = 2.0*i/(double)(qb-1)-1.0;
  getzw(qc,&z,&w,'c');
  for(i = 0; i < qc; ++i)
    z[i] = 2.0*i/(double)(qc-1)-1.0;

  b = E->getbasis();

  X.x = dvector(0,qa*qb*qc-1);
  X.y = dvector(0,qa*qb*qc-1);
  X.z = dvector(0,qa*qb*qc-1);
  /* get coordinates */
  E->coord(&X);

  sprintf(file,"Leverts_%d.tec",nverts);
  fp = fopen(file,"w");
  fprintf(fp,"VARIABLES = x y z u\n");
  /* fill Element btrans and dump vertices */
  /* dump geometry */
  fprintf(fp,"GEOMETRY T=LINE3D,  C=RED\n");
  switch(E->Nverts){
  case 4:
    fprintf(fp,"1\n");
    fprintf(fp,"8\n");
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);

    for(i = 0; i < E->Nverts; ++i){
      dzero(E->Nmodes,E->vert->hj,1);
      E->vert[i].hj[0] = 1.0;
#ifndef ORIGMODES
      dcopy(E->Nbmodes-E->Nverts,Rv[i],1,E->vert->hj+E->Nverts,1);
#endif

      E->Trans(E,J_to_Q);
      dsadd(qa*qb*qc,oset,E->h_3d[0][0],1,E->h_3d[0][0],1);

      /* write out solution */
      for(j = 0; j < E->Nfaces; ++j){
  switch(j){
  case 0:
    nx1 = E->vert[0].x - E->vert[1].x;
    ny1 = E->vert[0].y - E->vert[1].y;
    nz1 = E->vert[0].z - E->vert[1].z;
    nx2 = E->vert[2].x - E->vert[1].x;
    ny2 = E->vert[2].y - E->vert[1].y;
    nz2 = E->vert[2].z - E->vert[1].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
      i+1,j+1,qa,qb);
    for(l = 0; l < qb; ++l)
      for(k = 0; k < qa; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa + k]+nx*E->h_3d[0][0][l*qa+k],
          X.y[l*qa + k]+ny*E->h_3d[0][0][l*qa+k],
          X.z[l*qa + k]+nz*E->h_3d[0][0][l*qa+k],E->h_3d[0][0][l*qa+k]);
    break;
  case 1:
    nx1 = E->vert[1].x - E->vert[0].x;
    ny1 = E->vert[1].y - E->vert[0].y;
    nz1 = E->vert[1].z - E->vert[0].z;
    nx2 = E->vert[3].x - E->vert[0].x;
    ny2 = E->vert[3].y - E->vert[0].y;
    nz2 = E->vert[3].z - E->vert[0].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
      i+1,j+1,qa,qc);
    for(l = 0; l < qc; ++l)
      for(k = 0; k < qa; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa*qb+k]+nx*E->h_3d[0][0][l*qa*qb+k],
          X.y[l*qa*qb+k]+ny*E->h_3d[0][0][l*qa*qb+k],
          X.z[l*qa*qb+k]+nz*E->h_3d[0][0][l*qa*qb+k],
          E->h_3d[0][0][l*qa*qb+k]);
    break;
  case 2:
    nx1 = E->vert[2].x - E->vert[1].x;
    ny1 = E->vert[2].y - E->vert[1].y;
    nz1 = E->vert[2].z - E->vert[1].z;
    nx2 = E->vert[3].x - E->vert[1].x;
    ny2 = E->vert[3].y - E->vert[1].y;
    nz2 = E->vert[3].z - E->vert[1].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
      i+1,j+1,qb,qc);
    for(l = 0; l < qc; ++l)
      for(k = 0; k < qb; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa*qb+k*qa+qa-1]+nx*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
          X.y[l*qa*qb+k*qa+qa-1]+ny*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
          X.z[l*qa*qb+k*qa+qa-1]+nz*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
          E->h_3d[0][0][l*qa*qb+k*qa+qa-1]);
    break;
  case 3:
    nx1 = E->vert[0].x - E->vert[2].x;
    ny1 = E->vert[0].y - E->vert[2].y;
    nz1 = E->vert[0].z - E->vert[2].z;
    nx2 = E->vert[3].x - E->vert[2].x;
    ny2 = E->vert[3].y - E->vert[2].y;
    nz2 = E->vert[3].z - E->vert[2].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
      i+1,j+1,qb,qc);
    for(l = 0; l < qc; ++l)
      for(k = 0; k < qb; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa*qb+k*qa]+nx*E->h_3d[0][0][l*qa*qb+k*qa],
          X.y[l*qa*qb+k*qa]+ny*E->h_3d[0][0][l*qa*qb+k*qa],
          X.z[l*qa*qb+k*qa]+nz*E->h_3d[0][0][l*qa*qb+k*qa],
          E->h_3d[0][0][l*qa*qb+k*qa]);
    break;
  }
      }
    }

    break;
  case 6:
    fprintf(fp,"1\n");
    fprintf(fp,"12\n");
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[4].x,E->vert[4].y,E->vert[4].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[5].x,E->vert[5].y,E->vert[5].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[5].x,E->vert[5].y,E->vert[5].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[4].x,E->vert[4].y,E->vert[4].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);

    for(i = 0; i < E->Nverts; ++i){
      dzero(E->Nmodes,E->vert->hj,1);
      E->vert[i].hj[0] = 1.0;
#ifndef ORIGMODES
      dcopy(E->Nbmodes-E->Nverts,Rv[i],1,E->vert->hj+E->Nverts,1);
#endif
      E->Trans(E,J_to_Q);
      dsadd(qa*qb*qc,oset,E->h_3d[0][0],1,E->h_3d[0][0],1);

      /* write out solution */
      for(j = 0; j < E->Nfaces; ++j){
  switch(j){
  case 0:
    nx1 = E->vert[0].x - E->vert[1].x;
    ny1 = E->vert[0].y - E->vert[1].y;
    nz1 = E->vert[0].z - E->vert[1].z;
    nx2 = E->vert[2].x - E->vert[1].x;
    ny2 = E->vert[2].y - E->vert[1].y;
    nz2 = E->vert[2].z - E->vert[1].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d  face %d\" I=%d, J=%d,"
      " F=POINT\n",  i+1, j+1,qa,qb);
    for(l = 0; l < qb; ++l)
      for(k = 0; k < qa; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa + k]+nx*E->h_3d[0][0][l*qa+k],
          X.y[l*qa + k]+ny*E->h_3d[0][0][l*qa+k],
          X.z[l*qa + k]+nz*E->h_3d[0][0][l*qa+k],
          E->h_3d[0][0][l*qa+k]);
    break;
  case 1:
    nx1 = E->vert[1].x - E->vert[0].x;
    ny1 = E->vert[1].y - E->vert[0].y;
    nz1 = E->vert[1].z - E->vert[0].z;
    nx2 = E->vert[4].x - E->vert[0].x;
    ny2 = E->vert[4].y - E->vert[0].y;
    nz2 = E->vert[4].z - E->vert[0].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
      i+1,j+1,qa,qc);
    for(l = 0; l < qc; ++l)
      for(k = 0; k < qa; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa*qb+k]+nx*E->h_3d[0][0][l*qa*qb+k],
          X.y[l*qa*qb+k]+ny*E->h_3d[0][0][l*qa*qb+k],
          X.z[l*qa*qb+k]+nz*E->h_3d[0][0][l*qa*qb+k],
          E->h_3d[0][0][l*qa*qb+k]);
    break;
  case 2:
    nx1 = E->vert[2].x - E->vert[1].x;
    ny1 = E->vert[2].y - E->vert[1].y;
    nz1 = E->vert[2].z - E->vert[1].z;
    nx2 = E->vert[4].x - E->vert[1].x;
    ny2 = E->vert[4].y - E->vert[1].y;
    nz2 = E->vert[4].z - E->vert[1].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
      i+1,j+1,qb,qc);
    for(l = 0; l < qc; ++l)
      for(k = 0; k < qb; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa*qb+k*qa+qa-1]+nx*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
          X.y[l*qa*qb+k*qa+qa-1]+ny*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
          X.z[l*qa*qb+k*qa+qa-1]+nz*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
          E->h_3d[0][0][l*qa*qb+k*qa+qa-1]);
    break;
    case 3:
      nx1 = E->vert[3].x - E->vert[2].x;
      ny1 = E->vert[3].y - E->vert[2].y;
      nz1 = E->vert[3].z - E->vert[2].z;
      nx2 = E->vert[5].x - E->vert[2].x;
      ny2 = E->vert[5].y - E->vert[2].y;
      nz2 = E->vert[5].z - E->vert[2].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Vert %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,j+1,qa,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qa; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+(qa-1)*qb+k]+
      nx*E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k],
      X.y[l*qa*qb+(qa-1)*qb+k]+
      ny*E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k],
      X.z[l*qa*qb+(qa-1)*qb+k]+
      nz*E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k],
      E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k]);
      break;
  case 4:
    nx1 = E->vert[0].x - E->vert[3].x;
    ny1 = E->vert[0].y - E->vert[3].y;
    nz1 = E->vert[0].z - E->vert[3].z;
    nx2 = E->vert[5].x - E->vert[3].x;
    ny2 = E->vert[5].y - E->vert[3].y;
    nz2 = E->vert[5].z - E->vert[3].z;
    nx = ny1*nz2 - ny2*nz1;
    ny = nx2*nz1 - nx1*nz2;
    nz = nx1*ny2 - nx2*ny1;
    fac = sqrt(nx*nx + ny*ny + nz*nz);
    nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

    fprintf(fp,"ZONE T=\"Vert %d  face %d\" I=%d, J=%d, F=POINT\n",
        i+1,j+1,qb,qc);
    for(l = 0; l < qc; ++l)
      for(k = 0; k < qb; ++k)
        fprintf(fp,"%lf %lf %lf %lf\n",
          X.x[l*qa*qb+k*qa]+nx*E->h_3d[0][0][l*qa*qb+k*qa],
          X.y[l*qa*qb+k*qa]+ny*E->h_3d[0][0][l*qa*qb+k*qa],
          X.z[l*qa*qb+k*qa]+nz*E->h_3d[0][0][l*qa*qb+k*qa],
          E->h_3d[0][0][l*qa*qb+k*qa]);
    break;
  default:
    fprintf(stderr,"Please define geom in dump_le_modes\n");
    exit(-1);
    break;
  }
      }
    }
  }

  sprintf(file,"Leedges_%d.tec",nverts);
  fp = fopen(file,"w");
  fprintf(fp,"VARIABLES = x y z u\n");
  /* fill Element btrans and dump vertices */
  /* dump geometry */
  fprintf(fp,"GEOMETRY T=LINE3D,  C=RED\n");

  switch(E->Nverts){
  case 4:
    fprintf(fp,"1\n");
    fprintf(fp,"8\n");
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);

    for(i = 0; i < E->Nedges; ++i){
      l1 = E->edge[i].l;
      Nf = E->Nbmodes - E->Nverts - E->Nedges*l1;
      for(k1 = 0; k1 < l1; ++k1){
  dzero(E->Nmodes,E->vert->hj,1);
  E->edge[i].hj[k1] = 1.0;
#ifndef ORIGMODES
  if(Nf)
    dcopy(Nf,Re[i*l1+k1],1,E->face[0].hj[0],1);
#endif

  E->Trans(E,J_to_Q);
  dsadd(qa*qb*qc,oset,E->h_3d[0][0],1,E->h_3d[0][0],1);

  /* write out solution */
  for(j = 0; j < E->Nfaces; ++j){
    switch(j){
    case 0:
      nx1 = E->vert[0].x - E->vert[1].x;
      ny1 = E->vert[0].y - E->vert[1].y;
      nz1 = E->vert[0].z - E->vert[1].z;
      nx2 = E->vert[2].x - E->vert[1].x;
      ny2 = E->vert[2].y - E->vert[1].y;
      nz2 = E->vert[2].z - E->vert[1].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d,"
        " F=POINT\n",  i+1,k1+1, j+1,qa,qb);
      for(l = 0; l < qb; ++l)
        for(k = 0; k < qa; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa + k]+nx*E->h_3d[0][0][l*qa+k],
      X.y[l*qa + k]+ny*E->h_3d[0][0][l*qa+k],
      X.z[l*qa + k]+nz*E->h_3d[0][0][l*qa+k],
      E->h_3d[0][0][l*qa+k]);
      break;
    case 1:
      nx1 = E->vert[1].x - E->vert[0].x;
      ny1 = E->vert[1].y - E->vert[0].y;
      nz1 = E->vert[1].z - E->vert[0].z;
      nx2 = E->vert[3].x - E->vert[0].x;
      ny2 = E->vert[3].y - E->vert[0].y;
      nz2 = E->vert[3].z - E->vert[0].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qa,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qa; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+k]+nx*E->h_3d[0][0][l*qa*qb+k],
      X.y[l*qa*qb+k]+ny*E->h_3d[0][0][l*qa*qb+k],
      X.z[l*qa*qb+k]+nz*E->h_3d[0][0][l*qa*qb+k],
      E->h_3d[0][0][l*qa*qb+k]);
      break;
    case 2:
      nx1 = E->vert[2].x - E->vert[1].x;
      ny1 = E->vert[2].y - E->vert[1].y;
      nz1 = E->vert[2].z - E->vert[1].z;
      nx2 = E->vert[3].x - E->vert[1].x;
      ny2 = E->vert[3].y - E->vert[1].y;
      nz2 = E->vert[3].z - E->vert[1].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qb,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qb; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+k*qa+qa-1]+nx*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
      X.y[l*qa*qb+k*qa+qa-1]+ny*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
      X.z[l*qa*qb+k*qa+qa-1]+nz*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
      E->h_3d[0][0][l*qa*qb+k*qa+qa-1]);
      break;
    case 3:
      nx1 = E->vert[0].x - E->vert[2].x;
      ny1 = E->vert[0].y - E->vert[2].y;
      nz1 = E->vert[0].z - E->vert[2].z;
      nx2 = E->vert[3].x - E->vert[2].x;
      ny2 = E->vert[3].y - E->vert[2].y;
      nz2 = E->vert[3].z - E->vert[2].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qb,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qb; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+k*qa]+nx*E->h_3d[0][0][l*qa*qb+k*qa],
      X.y[l*qa*qb+k*qa]+ny*E->h_3d[0][0][l*qa*qb+k*qa],
      X.z[l*qa*qb+k*qa]+nz*E->h_3d[0][0][l*qa*qb+k*qa],
      E->h_3d[0][0][l*qa*qb+k*qa]);
      break;
    }
  }
      }
    }
    break;
  case 6:
    fprintf(fp,"1\n");
    fprintf(fp,"12\n");
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[0].x,E->vert[0].y,E->vert[0].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[4].x,E->vert[4].y,E->vert[4].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[5].x,E->vert[5].y,E->vert[5].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[3].x,E->vert[3].y,E->vert[3].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[2].x,E->vert[2].y,E->vert[2].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[5].x,E->vert[5].y,E->vert[5].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[4].x,E->vert[4].y,E->vert[4].z);
    fprintf(fp,"%lf %lf %lf\n",E->vert[1].x,E->vert[1].y,E->vert[1].z);

    for(i = 0; i < E->Nedges; ++i){
      l1 = E->edge[i].l;
      Nf = E->Nbmodes - E->Nverts - E->Nedges*l1;
      for(k1 = 0; k1 < l1; ++k1){
  dzero(E->Nmodes,E->vert->hj,1);
  E->edge[i].hj[k1] = 1.0;

#ifndef ORIGMODES
  if(Nf)
    dcopy(Nf,Re[i*l1+k1],1,E->face[0].hj[0],1);
#endif

  E->Trans(E,J_to_Q);
  dsadd(qa*qb*qc,oset,E->h_3d[0][0],1,E->h_3d[0][0],1);

  /* write out solution */
  for(j = 0; j < E->Nfaces; ++j){
    switch(j){
    case 0:
      nx1 = E->vert[0].x - E->vert[1].x;
      ny1 = E->vert[0].y - E->vert[1].y;
      nz1 = E->vert[0].z - E->vert[1].z;
      nx2 = E->vert[2].x - E->vert[1].x;
      ny2 = E->vert[2].y - E->vert[1].y;
      nz2 = E->vert[2].z - E->vert[1].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d,"
        " F=POINT\n",  i+1,k1+1, j+1,qa,qb);
      for(l = 0; l < qb; ++l)
        for(k = 0; k < qa; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa + k]+nx*E->h_3d[0][0][l*qa+k],
      X.y[l*qa + k]+ny*E->h_3d[0][0][l*qa+k],
      X.z[l*qa + k]+nz*E->h_3d[0][0][l*qa+k],E->h_3d[0][0][l*qa+k]);
      break;
    case 1:
      nx1 = E->vert[1].x - E->vert[0].x;
      ny1 = E->vert[1].y - E->vert[0].y;
      nz1 = E->vert[1].z - E->vert[0].z;
      nx2 = E->vert[4].x - E->vert[0].x;
      ny2 = E->vert[4].y - E->vert[0].y;
      nz2 = E->vert[4].z - E->vert[0].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qa,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qa; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+k]+nx*E->h_3d[0][0][l*qa*qb+k],
      X.y[l*qa*qb+k]+ny*E->h_3d[0][0][l*qa*qb+k],
      X.z[l*qa*qb+k]+nz*E->h_3d[0][0][l*qa*qb+k],
      E->h_3d[0][0][l*qa*qb+k]);
      break;
    case 2:
      nx1 = E->vert[2].x - E->vert[1].x;
      ny1 = E->vert[2].y - E->vert[1].y;
      nz1 = E->vert[2].z - E->vert[1].z;
      nx2 = E->vert[4].x - E->vert[1].x;
      ny2 = E->vert[4].y - E->vert[1].y;
      nz2 = E->vert[4].z - E->vert[1].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qb,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qb; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+k*qa+qa-1]+nx*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
      X.y[l*qa*qb+k*qa+qa-1]+ny*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
      X.z[l*qa*qb+k*qa+qa-1]+nz*E->h_3d[0][0][l*qa*qb+k*qa+qa-1],
      E->h_3d[0][0][l*qa*qb+k*qa+qa-1]);
      break;
    case 3:
      nx1 = E->vert[3].x - E->vert[2].x;
      ny1 = E->vert[3].y - E->vert[2].y;
      nz1 = E->vert[3].z - E->vert[2].z;
      nx2 = E->vert[5].x - E->vert[2].x;
      ny2 = E->vert[5].y - E->vert[2].y;
      nz2 = E->vert[5].z - E->vert[2].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qa,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qa; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+(qa-1)*qb+k]+
      nx*E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k],
      X.y[l*qa*qb+(qa-1)*qb+k]+
      ny*E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k],
      X.z[l*qa*qb+(qa-1)*qb+k]+
      nz*E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k],
      E->h_3d[0][0][l*qa*qb+(qa-1)*qb+k]);
      break;
    case 4:
      nx1 = E->vert[0].x - E->vert[3].x;
      ny1 = E->vert[0].y - E->vert[3].y;
      nz1 = E->vert[0].z - E->vert[3].z;
      nx2 = E->vert[5].x - E->vert[3].x;
      ny2 = E->vert[5].y - E->vert[3].y;
      nz2 = E->vert[5].z - E->vert[3].z;
      nx = ny1*nz2 - ny2*nz1;
      ny = nx2*nz1 - nx1*nz2;
      nz = nx1*ny2 - nx2*ny1;
      fac = sqrt(nx*nx + ny*ny + nz*nz);
      nx *= scal/fac; ny *= scal/fac; nz *= scal/fac;

      fprintf(fp,"ZONE T=\"Edge %d mode %d face %d\" I=%d, J=%d, F=POINT\n",
        i+1,k1+1,j+1,qb,qc);
      for(l = 0; l < qc; ++l)
        for(k = 0; k < qb; ++k)
    fprintf(fp,"%lf %lf %lf %lf\n",
      X.x[l*qa*qb+k*qa]+nx*E->h_3d[0][0][l*qa*qb+k*qa],
      X.y[l*qa*qb+k*qa]+ny*E->h_3d[0][0][l*qa*qb+k*qa],
      X.z[l*qa*qb+k*qa]+nz*E->h_3d[0][0][l*qa*qb+k*qa],
      E->h_3d[0][0][l*qa*qb+k*qa]);
      break;
    }
  }
      }
    }

    break;
  default:
    fprintf(stderr,"Please define geom in dump_le_modes\n");
    exit(-1);
    break;
  }

  dcopy(E->Nmodes,save,1,E->vert->hj,1);
  free(save); free(X.x); free(X.y); free(X.z);

}

void Tri::LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  error_msg(LowEnergyModes not define in Matrix.C);
}

void Tri::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
}

void Quad::LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  error_msg(LowEnergyModes not define in Matrix.C);
}
void Quad::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
}

static void LowEnergyEdges(double **R, LocMat *mat, int edge, int *el,
         int *fl, Element *E);
static void LowEnergyVertices(double *R, LocMat *mat, int vert, int *el,
            int *fl, Element *E);

typedef struct leMatrix {
  int   Ne;
  int   Nf;
  double lambda;
  double **Rv;
  double **Rvi;
  double **Re;
  struct leMatrix *next;
} LeMatrix;

void Tet::LowEnergyModes(Bsystem *B, int *Ne, int *Nf, double ***Rv,
       double ***Rvi, double ***Re){
  LeMatrix *Rvm;
  static LeMatrix *Rv_mat;

  if(option("variable"))
    error_msg(LowEnergyModes not set up for variable order);

  // check link list
  for(Rvm = Rv_mat; Rvm; Rvm = Rvm->next)
    if(Rvm->lambda == B->lambda->d){
      Ne [0] = Rvm->Ne;
      Nf [0] = Rvm->Nf;
      Rv [0] = Rvm->Rv;
      Rvi[0] = Rvm->Rvi;
      Re [0] = Rvm->Re;
      return;
    }

  // make new Le matrix
  Rvm = (LeMatrix *) malloc(sizeof(LeMatrix));
  MakeLowEnergyModes(B,&Rvm->Ne,&Rvm->Nf,&Rvm->Rv,&Rvm->Rvi,&Rvm->Re);
  Rvm->lambda = B->lambda->d;

  // attach to top of link list
  Rvm->next = Rv_mat;
  Rv_mat = Rvm;

  Ne [0] = Rvm->Ne;
  Nf [0] = Rvm->Nf;
  Rv [0] = Rvm->Rv;
  Rvi[0] = Rvm->Rvi;
  Re [0] = Rvm->Re;

  return;
}

void Tet::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  register int i;
  int         *el,*fl;
  LocMat      *mat;
  Geom         Gtmp;
  Cmodes      *C;

  el = ivector(0,Nedges);
  fl = ivector(0,Nfaces);

  /* set up cummalative list of edges location in matrix */
  el[0] = 0;
  for(i = 1; i <= Nedges; ++i)
    el[i] = el[i-1] + edge[i-1].l;

      /* set up cummalative list of face location in matrix */
  fl[0] = 0;
  for(i = 1; i <= Nfaces; ++i)
    fl[i] = fl[i-1] + face[i-1].l*(face[i-1].l+1)/2;

  Ne[0] = el[Nedges];
  Nf[0] = fl[Nfaces];

  if(Ne[0] + Nf[0]){
    Rv[0]  = dmatrix(0,Nverts-1,0,Ne[0] + Nf[0]-1);
    dzero(Nverts*(Ne[0] + Nf[0]),**Rv,1);
    Rvi[0] = dmatrix(0,Nverts-1,0,Ne[0] + Nf[0]-1);
  }
  else{
    Rv[0]  = dmatrix(0,0,0,0);
    Rvi[0] = dmatrix(0,0,0,0);
  }

  if(Ne[0]&&Nf[0]){
    Re[0] = dmatrix(0,Ne[0]-1,0,Nf[0]-1);
    dzero(Ne[0]*Nf[0],**Re,1);
  }
  else
    Re[0] = dmatrix(0,0,0,0);

  /* reset element to equilateral tet */
  memcpy(&Gtmp,geom,sizeof(Geom));
  C = curvX;
  curvX = (Cmodes *)NULL;

  /* set up geometric factors for equilateral tetrahedron */
  geom->jac.d = 1.0;
  geom->rx.d =  1.0;
  geom->ry.d = -1/sqrt(3.);
  geom->rz.d = -1/sqrt(6.);
  geom->sx.d =  0.0;
  geom->sy.d =  2/sqrt(3.);
  geom->sz.d = -1/sqrt(6.);
  geom->tx.d =  0.0;
  geom->ty.d =  0.0;
  geom->tz.d =  sqrt(1.5);

  mat = mat_mem();
  HelmMatC(mat,B->lambda);
  i = option("iterative");
  option_set("iterative",1);
  condense(mat,B,'n');       /* statically condense system */
  option_set("iterative",i);

      /* generate low energy vertex modes */
  if(Ne[0]+Nf[0])
    for(i = 0; i < Nverts; ++i)
      LowEnergyVertices(Rv[0][i],mat,i,el,fl,this);

  /* generate low energy edge modes */
  if(Nf[0])
    for(i = 0; i < Nedges; ++i)
      LowEnergyEdges(Re[0]+el[i],mat,i,el,fl,this);

  dsmul(Nverts*(el[Nedges]+fl[Nfaces]),-1,*Rv[0],1,*Rvi[0],1);
  if(fl[Nfaces])
    for(i = 0; i < Nverts; ++i)
      dgemv('N',fl[Nfaces],el[Nedges],-1.0,Re[0][0],fl[Nfaces],
      Rvi[0][i],1,1.0,Rvi[0][i]+el[Nedges],1);

  memcpy(geom,&Gtmp,sizeof(Geom));
  curvX = C;

  mat_free(mat);
  free(el); free(fl);
}

void Pyr::LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  error_msg(LowEnergyModes not define in Matrix.C);
}


void Pyr::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
}

void Prism::LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  LeMatrix *Rvm;
  static LeMatrix *Rv_mat;

  if(option("variable"))
    error_msg(LowEnergyModes not set up for variable order);

  // check link list
  for(Rvm = Rv_mat; Rvm; Rvm = Rvm->next)
    if(Rvm->lambda == B->lambda->d){
      Ne [0] = Rvm->Ne;
      Nf [0] = Rvm->Nf;
      Rv [0] = Rvm->Rv;
      Rvi[0] = Rvm->Rvi;
      Re [0] = Rvm->Re;
      return;
    }

  // make new Le matrix
  Rvm = (LeMatrix *) malloc(sizeof(LeMatrix));
  MakeLowEnergyModes(B,&Rvm->Ne,&Rvm->Nf,&Rvm->Rv,&Rvm->Rvi,&Rvm->Re);
  Rvm->lambda = B->lambda->d;

  // attach to top of link list
  Rvm->next = Rv_mat;
  Rv_mat = Rvm;

  Ne [0] = Rvm->Ne;
  Nf [0] = Rvm->Nf;
  Rv [0] = Rvm->Rv;
  Rvi[0] = Rvm->Rvi;
  Re [0] = Rvm->Re;

  return;
}

void Prism::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  register int i;
  int *el,*fl;
  LocMat *mat;
  double *geomsave;

  /* default PSfactor = 1.0 */
  double prism_reshape_factor = dparam("PSfactor");

  el = ivector(0,Nedges);
  fl = ivector(0,Nfaces);

  /* set up cummalative list of edges location in matrix */
  el[0] = 0;
  for(i = 1; i <= Nedges; ++i)
    el[i] = el[i-1] + edge[i-1].l;

  /* set up cummalative list of face location in matrix */
  fl[0] = 0;
  for(i = 1; i <= Nfaces; ++i)
    if(Nfverts(i-1) == 3)
      fl[i] = fl[i-1] + face[i-1].l*(face[i-1].l+1)/2;
    else
      fl[i] = fl[i-1] + face[i-1].l*face[i-1].l;

  Ne[0] = el[Nedges];
  Nf[0] = fl[Nfaces];

  if(Ne[0]+Nf[0]){
    Rv[0]  = dmatrix(0,Nverts-1,0,Ne[0] + Nf[0]-1);
    dzero(Nverts*(Ne[0] + Nf[0]),**Rv,1);
    Rvi[0] = dmatrix(0,Nverts-1,0,Ne[0] + Nf[0]-1);
  }
  else{
    Rv[0]  = dmatrix(0,0,0,0);
    Rvi[0] = dmatrix(0,0,0,0);
  }
  if(Ne[0]&&Nf[0]){
    Re[0] = dmatrix(0,Ne[0]-1,0,Nf[0]-1);
    dzero(Ne[0]*Nf[0],**Re,1);
  }
  else
    Re[0] = dmatrix(0,0,0,0);

  /* reset element to prism with equilateral ends */
  /* set up geometric factors for equilateral tetrahedron */

  if(!curvX)
    error_msg(Prisms: LowEnergyModes did not expect non-curved element);

  geomsave = dvector(0,qtot*10-1);
  dcopy(qtot*10,geom->rx.p,1,geomsave,1);

  dfill(qtot,1.0*prism_reshape_factor,geom->jac.p,1);
  dfill(qtot,1.0,geom->rx.p,1);
  dzero(qtot,geom->ry.p,1);
  dfill(qtot,-1/sqrt(3.0),geom->rz.p,1);
  dzero(qtot,geom->sx.p,1);
  dfill(qtot,1.0/prism_reshape_factor,geom->sy.p,1);
  dzero(qtot,geom->sz.p,1);
  dzero(qtot,geom->tx.p,1);
  dzero(qtot,geom->ty.p,1);
  dfill(qtot,2/sqrt(3.0),geom->tz.p,1);

  mat = mat_mem();
  HelmMatC(mat,B->lambda);
  i = option("iterative");
  option_set("iterative",1);
  condense(mat,B,'n');       /* statically condense system */
  option_set("iterative",i);

    /* generate low energy vertex modes */
  if(Ne[0]+Nf[0])
    for(i = 0; i < Nverts; ++i)
      LowEnergyVertices(Rv[0][i],mat,i,el,fl,this);

  /* generate low energy edge modes */
  if(Nf[0])
    for(i = 0; i < Nedges; ++i)
      LowEnergyEdges(*Re+el[i],mat,i,el,fl,this);

  /* calculate inverse */
  dsmul(Nverts*(el[Nedges]+fl[Nfaces]),-1,*Rv[0],1,*Rvi[0],1);
  if(fl[Nfaces])
    for(i = 0; i < Nverts; ++i)
      dgemv('N',fl[Nfaces],el[Nedges],-1.0,Re[0][0],fl[Nfaces],
      Rvi[0][i],1,1.0,Rvi[0][i]+el[Nedges],1);

  /* reset geofac */
  dcopy(qtot*10,geomsave,1,geom->rx.p,1);

  mat_free(mat);
  free(geomsave); free(el); free(fl);

}

void Hex::LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  if(option("variable"))
    error_msg(LowEnergyModes not set up for variable order);

  fprintf(stderr,"Warning:LowEnergyModes not debugged in Matrix.C");

  if(type == 'p'){ /* pressure system */
    static double **Rv_store, **Rvi_store, **Re_store;
    static int Ne_store, Nf_store;

    /* setup matrices */
    if(!Rv_store)
      MakeLowEnergyModes(B,&Ne_store,&Nf_store,&Rvi_store,&Rvi_store,
       &Re_store);

    Ne [0] = Ne_store;
    Nf [0] = Nf_store;
    Rv [0] = Rv_store;
    Rvi[0] = Rvi_store;
    Re [0] = Re_store;
  }
  else{
    static double **Rv_store, **Rvi_store, **Re_store;
    static int Ne_store, Nf_store;

    /* setup matrices */
    if(!Rv_store)
      MakeLowEnergyModes(B,&Ne_store,&Nf_store,&Rvi_store,&Rvi_store,
          &Re_store);
    Ne [0] = Ne_store;
    Nf [0] = Nf_store;
    Rv [0] = Rv_store;
    Rvi[0] = Rvi_store;
    Re [0] = Re_store;
  }
}

void Hex::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){
  register int i;
  int *el,*fl;
  double *geomsave;
  LocMat *mat;

  el = ivector(0,Nedges);
  fl = ivector(0,Nfaces);

  /* set up cummalative list of edges location in matrix */
  el[0] = 0;
  for(i = 1; i <= Nedges; ++i)
    el[i] = el[i-1] + edge[i-1].l;

  /* set up cummalative list of face location in matrix */
  fl[0] = 0;
  for(i = 1; i <= Nfaces; ++i)
    fl[i] = fl[i-1] + face[i-1].l*(face[i-1].l+1)/2;

  Ne[0] = Ne[0] = el[Nedges];
  Nf[0] = Nf[0] = fl[Nfaces];

  if(Ne[0]+Nf[0]){
    Rv[0]  = dmatrix(0,Nverts-1,0,Ne[0] + Nf[0]-1);
    dzero(Nverts*(Ne[0] + Nf[0]),*Rv[0],1);
    Rvi[0] = dmatrix(0,Nverts-1,0,Ne[0] + Nf[0]-1);
  }
  else{
    Rv[0]  = dmatrix(0,0,0,0);
    Rvi[0] = dmatrix(0,0,0,0);
  }
  if(Ne[0]&&Nf[0]){
    Re[0] = dmatrix(0,Ne[0]-1,0,Nf[0]-1);
    dzero(Ne[0]*Nf[0],*Re[0],1);
  }
  else
    Re[0] = dmatrix(0,0,0,0);

  /* reset element to prism with equilateral ends */
  /* set up geometric factors for equilateral tetrahedron */

  if(!curvX)
    error_msg(Hex: LowEnergyModes did not expect non-curved element);

  geomsave = dvector(0,qtot*10-1);
  dcopy(qtot*10,geom->rx.p,1,geomsave,1);

  dfill(qtot,1.0,geom->jac.p,1);
  dfill(qtot,1.0,geom->rx.p,1);
  dzero(qtot,geom->ry.p,1);
  dzero(qtot,geom->rz.p,1);
  dzero(qtot,geom->sx.p,1);
  dfill(qtot,1.0,geom->sy.p,1);
  dzero(qtot,geom->sz.p,1);
  dzero(qtot,geom->tx.p,1);
  dzero(qtot,geom->ty.p,1);
  dfill(qtot,1.0,geom->tz.p,1);

  mat = mat_mem();
  HelmMatC(mat,B->lambda);
  i = option("iterative");
  option_set("iterative",1);
  condense(mat,B,'n');       /* statically condense system */
  option_set("iterative",i);

  /* generate low energy vertex modes */
  if(Ne[0]+Nf[0])
    for(i = 0; i < Nverts; ++i)
      LowEnergyVertices(Rv[0][i],mat,i,el,fl,this);

  /* generate low energy edge modes */
  if(Nf[0])
    for(i = 0; i < Nedges; ++i)
      LowEnergyEdges(Re[0]+el[i],mat,i,el,fl,this);

  /* calculate inverse */
  dsmul(Nverts*(el[Nedges]+fl[Nfaces]),-1,*Rv[0],1,*Rvi[0],1);
  if(fl[Nfaces])
    for(i = 0; i < Nverts; ++i)
      dgemv('N',fl[Nfaces],el[Nedges],-1.0,Re[0][0],fl[Nfaces],
      Rvi[0][i],1,1.0,Rvi[0][i]+el[Nedges],1);

  /* reset geofac */
  dcopy(qtot*10,geomsave,1,geom->rx.p,1);

  mat_free(mat);

  free(geomsave); free(el); free(fl);
}

static void LowEnergyVertices(double *R, LocMat *mat, int vert,
            int *el, int *fl, Element *E){
  register int i,j,k,n;
  int face[3], edge[3],info;
  double *a,*f;
  double **A = mat->a;
  int    l,ll,Nv,Ne,Nf,cnt,len;

  Nv = E->Nverts;
  Ne = E->Nedges;
  Nf = E->Nfaces;

  switch(Nv){
  case 4: /* Tet */
    switch(vert){
    case 0:
      edge[0] = 0; edge[1] = 2; edge[2] = 3;
      face[0] = 0; face[1] = 1; face[2] = 3;
      break;
    case 1:
      edge[0] = 0; edge[1] = 1; edge[2] = 4;
      face[0] = 0; face[1] = 1; face[2] = 2;
      break;
    case 2:
      edge[0] = 1; edge[1] = 2; edge[2] = 5;
      face[0] = 0; face[1] = 2; face[2] = 3;
      break;
    case 3:
      edge[0] = 3; edge[1] = 4; edge[2] = 5;
      face[0] = 1; face[1] = 2; face[2] = 3;
      break;
    }
    break;
  case 5: /* Pyramid */
    error_msg(LowEnergyVertices not set up for pyramids);
    break;
  case 6: /* Prism   */
    /* set up edges + faces to solve for vertex = vert*/
    switch(vert){
    case 0:
      edge[0] = 0; edge[1] = 3; edge[2] = 4;
      face[0] = 0; face[1] = 1; face[2] = 4;
      break;
    case 1:
      edge[0] = 0; edge[1] = 1; edge[2] = 5;
      face[0] = 0; face[1] = 1; face[2] = 2;
      break;
    case 2:
      edge[0] = 1; edge[1] = 2; edge[2] = 6;
      face[0] = 0; face[1] = 2; face[2] = 3;
      break;
    case 3:
      edge[0] = 2; edge[1] = 3; edge[2] = 7;
      face[0] = 0; face[1] = 3; face[2] = 4;
      break;
    case 4:
      edge[0] = 4; edge[1] = 5; edge[2] = 8;
      face[0] = 1; face[1] = 2; face[2] = 4;
      break;
    case 5:
      edge[0] = 6; edge[1] = 7; edge[2] = 8;
      face[0] = 2; face[1] = 3; face[2] = 4;
      break;
    }

    break;
  case 8:
    /* set up edges + faces to solve for vertex = vert*/
    switch(vert){
    case 0:
      edge[0] = 0; edge[1] = 3; edge[2] = 4;
      face[0] = 0; face[1] = 1; face[2] = 4;
      break;
    case 1:
      edge[0] = 0; edge[1] = 1; edge[2] = 5;
      face[0] = 0; face[1] = 1; face[2] = 2;
      break;
    case 2:
      edge[0] = 1; edge[1] = 2; edge[2] = 6;
      face[0] = 0; face[1] = 2; face[2] = 3;
      break;
    case 3:
      edge[0] = 2; edge[1] = 3; edge[2] = 7;
      face[0] = 0; face[1] = 3; face[2] = 4;
      break;
    case 4:
      edge[0] = 4; edge[1] = 5; edge[2] = 8;
      face[0] = 1; face[1] = 2; face[2] = 4;
      break;
    case 5:
      edge[0] = 6; edge[1] = 7; edge[2] = 8;
      face[0] = 2; face[1] = 3; face[2] = 4;
      break;
    }
    break;
  }

  len = 0;
  for(i = 0; i < 3; ++i){
    len += el[edge[i]+1] - el[edge[i]];
    len += fl[face[i]+1] - fl[face[i]];
  }

  if(len){
    f = dvector(0,len-1);
    a = dvector(0,len*(len+1)/2-1);

    /* extract edge and edge-face information */
    for(i = 0,n=0,cnt=0; i < 3; ++i)
      for(j = 0; j < (l=E->edge[edge[i]].l); ++j,++cnt){
  f[cnt] = A[Nv+el[edge[i]]+j][vert];
  dcopy(l-j,A[Nv+el[edge[i]]+j]+Nv+el[edge[i]]+j,1,a+n,1);
  n+=l-j;
  for(k = i+1; k < 3; ++k){
    ll = el[edge[i]+1] - el[edge[i]];
    dcopy(ll,A[Nv+el[edge[i]]+j]+Nv+el[edge[k]],1,a+n,1);
    n+=ll;
  }
  for(k = 0; k < 3; ++k){
    ll = fl[face[k]+1] - fl[face[k]];
    dcopy(ll,A[Nv+el[edge[i]]+j]+Nv+el[Ne]+fl[face[k]],1,a+n,1);
    n+=ll;
  }
      }

    /* extract face information */
    for(i = 0; i < 3; ++i){
      l = fl[face[i]+1] - fl[face[i]];
      for(j = 0; j < l; ++j,++cnt){
  f[cnt] = A[Nv+el[Ne]+fl[face[i]]+j][vert];
  dcopy(l-j,A[Nv+el[Ne]+fl[face[i]]+j]+
        Nv+el[Ne]+fl[face[i]]+j,1,a+n,1);
  n+=l-j;
  for(k = i+1; k < 3; ++k){
    ll = fl[face[k]+1] - fl[face[k]];
    dcopy(ll,A[Nv+el[Ne]+fl[face[i]]+j]+
    Nv+el[Ne]+fl[face[k]],1,a+n,1);
    n+=ll;
  }
      }
    }

    dpptrf('L',len,a,info);
    if(info)
      error_msg(info not zero in factorisation of LowEnergyVertices);

    dneg(len,f,1);
    dpptrs('L',len,1,a,f,len,info);
    if(info)
      error_msg(info not zero in solve of LowEnergyVertices);

    /* copy back solution */
    for(i = 0,cnt=0; i < 3; ++i){
      l = el[edge[i]+1] - el[edge[i]];
      dcopy(l,f+cnt,1,R+el[edge[i]],1);
      cnt += l;
    }
    for(i = 0; i < 3; ++i){
      l = fl[face[i]+1] - fl[face[i]];
      dcopy(l,f+cnt,1,R+el[Ne]+fl[face[i]],1);
      cnt += l;
    }

    free(a); free(f);
  }
}

static void LowEnergyEdges(double **R, LocMat *mat, int edge, int *el,
         int *fl, Element *E){
  register int i,j,k,n;
  int face[2],info;
  double *a,**f;
  double **A = mat->a;
  int    l,ll,Nv,Ne,Nf,cnt,elen,flen;

  Nv = E->Nverts;
  Ne = E->Nedges;
  Nf = E->Nfaces;

  switch(Nv){
  case 4: /* Tet */
    switch(edge){
    case 0:
      face[0] = 0; face[1] = 1;
      break;
    case 1:
      face[0] = 0; face[1] = 2;
      break;
    case 2:
      face[0] = 0; face[1] = 3;
      break;
    case 3:
      face[0] = 1; face[1] = 3;
      break;
    case 4:
      face[0] = 1; face[1] = 2;
      break;
    case 5:
      face[0] = 2; face[1] = 3;
      break;
    }
    break;
  case 5: /* Pyramid */
    error_msg(LowEnergyVertices not set up for pyramids);
    break;
  case 6: /* Prism   */
    /* set up edges + faces to solve for vertex = vert*/
    switch(edge){
    case 0:
      face[0] = 0; face[1] = 1;
      break;
    case 1:
      face[0] = 0; face[1] = 2;
      break;
    case 2:
      face[0] = 0; face[1] = 3;
      break;
    case 3:
      face[0] = 0; face[1] = 4;
      break;
    case 4:
      face[0] = 1; face[1] = 4;
      break;
    case 5:
      face[0] = 1; face[1] = 2;
      break;
    case 6:
      face[0] = 2; face[1] = 3;
      break;
    case 7:
      face[0] = 3; face[1] = 4;
      break;
    case 8:
      face[0] = 2; face[1] = 4;
      break;
    }
    break;
  case 8:
    error_msg(LowEnergyVertices not set up for pyramids);
    break;
  }

  elen = el[edge+1] - el[edge];
  flen = fl[face[0]+1] - fl[face[0]] + fl[face[1]+1] - fl[face[1]];

  if(flen){
    f = dmatrix(0,elen-1,0,flen-1);
    a = dvector(0,flen*(flen+1)/2-1);

    /* extract face information */
    for(i = 0,n=0,cnt=0; i < 2; ++i){
      l = fl[face[i]+1] - fl[face[i]];
      for(j = 0; j < l; ++j,++cnt){
  dcopy(elen,A[Nv+el[Ne]+fl[face[i]]+j] + Nv + el[edge] ,1,*f+cnt,flen);
  dcopy(l-j, A[Nv+el[Ne]+fl[face[i]]+j] + Nv+el[Ne] + fl[face[i]] + j
        ,1,a+n,1);
  n+=l-j;
  for(k = i+1; k < 2; ++k){
    ll = fl[face[k]+1] - fl[face[k]];
    dcopy(ll,A[Nv+el[Ne]+fl[face[i]]+j]+
    Nv+el[Ne]+fl[face[k]],1,a+n,1);
    n+=ll;
  }
      }
    }

    dpptrf('L',flen,a,info);

    if(info)
      error_msg(info not zero in factorisation of LowEnergyEdges);

    dneg(flen*elen,*f,1);
    dpptrs('L',flen,elen,a,*f,flen,info);
    if(info)
      error_msg(info not zero in solve of LowEnergyEdges);

    /* copy back solution */
    for(i = 0,cnt=0; i < 2; ++i){
      l = fl[face[i]+1] - fl[face[i]];
      for(j = 0; j < elen; ++j)
  dcopy(l,f[j]+cnt,1,R[j]+fl[face[i]],1);
      cnt += l;
    }
    free(a); free_dmatrix(f,0,0);
  }
}

void Element::LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){ERR;}
void Element::MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
       double ***Rv, double ***Rvi, double ***Re){ERR;}
// return Low every transformation matrices

/*

Function name: Element::MassMat

Function Purpose:

Argument 1: LocMat *mass
Purpose:

Function Notes:

*/

void  Tri::MassMat(LocMat *mass){
  register int i,i1,j,k,n,n1;
  int      H;
  int      ll;
  Basis   *B;
  Mode    *w,*mtmp,*m,*m1;
  double  *z;
  double jac = geom->jac.d;
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  B    = getbasis();

  w    = mvector(0,0);
  mtmp = mvecset(0,0,qa,qb,qc);

  getzw(qa,&z,&w[0].a,'a');
  getzw(qb,&z,&w[0].b,'b');

  /* sort A and B matrices */
  m = B->vert;
  for(i = 0,n=0; i < Nverts; ++i,++n){
    Tri_mvmul2d(qa,qb,qc,m+i,w,mtmp);
    /* vertex with vertex */
    for(j = i,n1=n; j < Nverts; ++j,++n1)
      a[n1][n] = a[n][n1] = jac*iprod(m+j,mtmp);
    /* vertex with edge */
    for(j = 0; j < Nedges; ++j){
      m1 = B->edge[j];
      for(k = 0; k < edge[j].l; ++k,++n1)
  a[n1][n] = a[n][n1] = jac*iprod(m1+k,mtmp);
    }

    /* vertices with interior */
    ll = face->l;
    for(k = 0, n1=0; k < ll; ++k){
      m1 = B->face[0][k];
      for(H = 0; H < ll-k; ++H,++n1)
  b[n][n1] = jac*iprod(m1+H,mtmp);
    }
  }

  for(i = 0; i < Nedges; ++i){
    m = B->edge[i];
    for(i1 = 0; i1 < edge[i].l; ++i1,++n){
      Tri_mvmul2d(qa,qb,qc,m+i1,w,mtmp);
      /* edge with edge */
      for(k = i1,n1=n; k < edge[i].l; ++k,++n1)
  a[n1][n] = a[n][n1] = jac*iprod(m+k,mtmp);

      for(j = i+1; j < Nedges; ++j){
  m1 = B->edge[j];
  for(k = 0; k < edge[j].l; ++k,++n1)
    a[n1][n] = a[n][n1] = jac*iprod(m1+k,mtmp);
      }

      ll = face[0].l;
      for(k = 0,n1=0; k < ll; ++k){
  m1 = B->face[0][k];
  for(H = 0; H < ll-k; ++H,++n1)
    b[n][n1] = jac*iprod(m1+H,mtmp);
      }
    }
  }

  /* fill c */
  ll = face[0].l;
  for(i = 0,n=0; i < ll; ++i){
    m = B->face[0][i];
    for(i1=0; i1 < ll-i;++i1,++n){
      Tri_mvmul2d(qa,qb,qc,m+i1,w,mtmp);

      for(H = i1,n1=n; H < ll-i; ++H,++n1)
  c[n1][n] = c[n][n1] = jac*iprod(m+H,mtmp);
      for(k = i+1; k < ll; ++k){
  m1 = B->face[0][k];
  for(H = 0; H < ll-k; ++H,++n1)
    c[n1][n] = c[n][n1] = jac*iprod(m1+H,mtmp);
      }
    }
  }
  // recent change
  free((char*)w);
  free_mvec(mtmp);
}

void Quad::MassMat (LocMat *mass){
  MassMatC(mass);
}

void  Tet::MassMat(LocMat *mass){
  MassMatC(mass);
}

void  Pyr::MassMat(LocMat *mass){
  MassMatC(mass);
}

void  Prism::MassMat(LocMat *mass){
  MassMatC(mass);
}

void  Hex::MassMat(LocMat *mass){
  MassMatC(mass);
}

void Element::MassMat (LocMat *){ERR;}             // return mass-matri(ces)

/*

Function name: Element::MassMatC

Function Purpose:

Argument 1: LocMat *mass
Purpose:

Function Notes:

*/

void  Tri::MassMatC(LocMat *mass){
  register int i,j,n;
  const   int  nbl = Nbmodes, N = Nmodes - nbl, csize = mass->csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  /* fill A */
  for(i = 0, n = 0; i < Nverts; ++i,++n){
    fillElmt(B->vert+i);
    Iprod(this);
    dcopy(nbl,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      Iprod(this);
      dcopy(nbl,vert->hj    ,1,a[n],1);
    }

  /* fill B and C */

  L = face->l;
  for(i = 0,n=0; i < L; ++i)
    for(j = 0; j < L-i; ++j, ++n){
      fillElmt(B->face[0][i]+j);
      Iprod(this);
      dcopy(nbl,vert->hj    ,1,*b+n,csize);
      dcopy(N  ,vert->hj+nbl,1,c[n],1);
    }
}

void Quad::MassMatC(LocMat *mass){

  double *save = dvector(0, qtot+Nmodes-1);
  dcopy(qtot, h[0], 1, save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save+qtot, 1);

  register int i,j,n;
  const   int  nbl = Nbmodes, N = Nmodes - nbl, csize = mass->csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  /* fill A */
  for(i = 0, n = 0; i < Nverts; ++i,++n){
    fillElmt(B->vert+i);
    Iprod(this);
    dcopy(nbl,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      Iprod(this);
      dcopy(nbl,vert->hj    ,1,a[n],1);
    }

  /* fill B and C */

  L = face->l;
  for(i = 0,n=0; i < L; ++i)
    for(j = 0; j < L; ++j, ++n){
      fillElmt(B->face[0][i]+j);
      Iprod(this);
      dcopy(nbl,vert->hj    ,1,*b+n,csize);
      dcopy(N  ,vert->hj+nbl,1,c[n],1);
    }

  dcopy(qtot, save, 1, h[0], 1);
  dcopy(Nmodes, save+qtot, 1, vert[0].hj, 1);
  free(save);

  //  dump_sc(mass->asize, mass->csize, mass->a, mass->b, mass->c);
}

void  Tet::MassMatC(LocMat *mass){
  register int i,j,k,n;
  const   int  nbl = Nbmodes, N = Nmodes - nbl, csize = mass->csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  /* fill A */
  for(i = 0, n = 0; i < Nverts; ++i,++n){
    fillElmt(B->vert+i);
    Iprod(this);
    dcopy(nbl,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      Iprod(this);
      dcopy(nbl,vert->hj    ,1,a[n],1);
    }

  for(i = 0; i < Nfaces; ++i)
    for(j = 0; j < (L = face[i].l); ++j)
      for(k = 0; k < L-j; ++k, ++n){
      fillElmt(B->face[i][j]+k);
      Iprod(this);
      dcopy(nbl,vert->hj    ,1,a[n],1);
    }

  /* fill B and C */

  L = interior_l;

  for(i = 0,n=0; i < L; ++i)
    for(j = 0; j < L-i; ++j)
      for(k = 0; k < L-i-j; ++k, ++n){
  fillElmt(B->intr[i][j]+k);
  Iprod(this);
  dcopy(nbl,vert->hj    ,1,*b+n,csize);
  dcopy(N,  vert->hj+nbl,1,c[n],1);
      }

  //  dump_sc(nbl, csize, a, b, c);

}

void  Pyr::MassMatC(LocMat *mass){
  register int i,j,k,n;
  int          kk;
  int      asize, csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  asize = Nbmodes;
  csize = Nmodes - Nbmodes;

  /* fill A */
  for(i = 0, n = 0; i < NPyr_verts; ++i,++n){
    fillElmt(B->vert+i);
    Iprod(this);
    dcopy(asize,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < NPyr_edges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      Iprod(this);
      dcopy(asize,vert->hj    ,1,a[n],1);
    }

  for(i = 0; i < NPyr_faces; ++i)
    for(j = 0; j < (L = face[i].l); ++j){
      kk = (Nfverts(i) == 3) ? face[i].l-j:face[i].l;
      for(k = 0; k < kk; ++k, ++n){
  fillElmt(B->face[i][j]+k);
  Iprod(this);
  dcopy(asize,vert->hj    ,1,a[n],1);
      }
    }
  /* fill B and C */

  L = interior_l;
  if(csize)
    for(i = 0,n=0; i < L; ++i)
      for(j = 0; j < L-i; ++j)
  for(k = 0; k < L-i-j; ++k){

  fillElmt(B->intr[i][j]+k);
  Iprod(this);
  dcopy(asize,vert->hj      ,1,*b+n,csize);
  dcopy(csize,vert->hj+asize,1,c[n],1);
  ++n;
  }

  //  dump_sc(mass->asize, mass->csize, mass->a, mass->b, mass->c);
}

void  Prism::MassMatC(LocMat *mass){
  register int i,j,k,n;
  int          kk;
  int      asize, csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  asize = Nbmodes;
  csize = Nmodes - Nbmodes;

  /* fill A */
  for(i = 0, n = 0; i < NPrism_verts; ++i,++n){
    fillElmt(B->vert+i);
    Iprod(this);
    dcopy(asize,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < NPrism_edges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      Iprod(this);
      dcopy(asize,vert->hj    ,1,a[n],1);
    }

  for(i = 0; i < NPrism_faces; ++i)
    for(j = 0; j < (L = face[i].l); ++j){
      kk = (Nfverts(i) == 3) ? face[i].l-j:face[i].l;
      for(k = 0; k < kk; ++k, ++n){
  fillElmt(B->face[i][j]+k);
  Iprod(this);
  dcopy(asize,vert->hj    ,1,a[n],1);
      }
    }
  /* fill B and C */

  L = interior_l;

  for(i = 0,n=0; i < L-1; ++i)
    for(j = 0; j < L; ++j)
      for(k = 0; k < L-1-i; ++k, ++n){
  fillElmt(B->intr[i][j]+k);
  Iprod(this);
  dcopy(asize,vert->hj      ,1,*b+n,csize);
  dcopy(csize,vert->hj+asize,1,c[n],1);
      }
}

void  Hex::MassMatC(LocMat *mass){
  register int i,j,k,n;
  int      asize, csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  asize = Nbmodes;
  csize = Nmodes - Nbmodes;

  /* fill A */
  for(i = 0, n = 0; i < NHex_verts; ++i,++n){
    fillElmt(B->vert+i);
    Iprod(this);
    dcopy(asize,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < NHex_edges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      Iprod(this);
      dcopy(asize,vert->hj    ,1,a[n],1);
    }

  for(i = 0; i < NHex_faces; ++i)
    for(j = 0; j < (L = face[i].l); ++j)
      for(k = 0; k < L; ++k, ++n){
      fillElmt(B->face[i][j]+k);
      Iprod(this);
      dcopy(asize,vert->hj    ,1,a[n],1);
    }

  /* fill B and C */

  L = interior_l;

  for(i = 0,n=0; i < L; ++i)
    for(j = 0; j < L; ++j)
      for(k = 0; k < L; ++k, ++n){
  fillElmt(B->intr[i][j]+k);
  Iprod(this);
  dcopy(asize,vert->hj      ,1,*b+n,csize);
  dcopy(csize,vert->hj+asize,1,c[n],1);
      }
}

void Element::MassMatC(LocMat *){ERR;}

/*

Function name: Element::HelmMatC

Function Purpose:

Argument 1: LocMat *mass
Purpose:

Argument 2: Metric *lambda
Purpose:

Function Notes:

*/

void  Tri::HelmMatC(LocMat *mass, Metric *lambda){
  register int i,j,n;
  int nbl = Nbmodes, N = Nmodes - nbl, asize = mass->asize,csize = mass->csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c,
         **d = mass->d;

  /* fill A */
  for(i = 0,n=0; i < Nverts; ++i,++n){
    fillElmt(B->vert+i);
    HelmHoltz(lambda);
    dcopy(nbl,vert->hj,1,*a+n,asize);
    if(lambda->wave&&N)
      dcopy(N,vert->hj+nbl,1,d[n],1);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      HelmHoltz(lambda);
      dcopy(nbl,vert->hj,1,*a+n,asize);
      if(lambda->wave&&N)
  dcopy(N,vert->hj+nbl,1,d[n],1);
    }

  /* fill B and C */
  L = face->l;
  for(i = 0,n=0; i < L; ++i)
    for(j = 0; j < L-i; ++j, ++n){
      fillElmt(B->face[0][i]+j);
      HelmHoltz(lambda);
      dcopy(nbl,vert->hj    ,1,*b+n,csize);
      dcopy(N  ,vert->hj+nbl,1,*c+n,csize);
    }
}

void  Quad::HelmMatC(LocMat *mass, Metric *lambda){

  double *save = dvector(0, qtot+Nmodes-1);
  dcopy(qtot, h[0], 1, save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save+qtot, 1);

  register int i,j,n;
  int nbl = Nbmodes, N = Nmodes - nbl, csize = mass->csize,asize = mass->asize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c,
         **d = mass->d;
  char orig_state = state;
  /* fill A */
  for(i = 0,n=0; i < Nverts; ++i,++n){
    fillElmt(B->vert+i);
    HelmHoltz(lambda);
    dcopy(nbl,vert->hj,1,*a+n,asize);
    if(lambda->wave&&N)
      dcopy(N,vert->hj+nbl,1,d[n],1);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      HelmHoltz(lambda);
      dcopy(nbl,vert->hj,1,*a+n,asize);
      if(lambda->wave&&N)
  dcopy(N,vert->hj+nbl,1,d[n],1);
    }

  /* fill B and C */
  L = face->l;
  for(i = 0,n=0; i < L;++i)
    for(j = 0; j < L; ++j,++n){
      fillElmt(B->face[0][i]+j);
      HelmHoltz(lambda);
      dcopy(nbl,vert->hj    ,1,*b+n,csize);
      dcopy(N  ,vert->hj+nbl,1,*c+n,csize);
    }
  //  dump_sc(mass->asize, mass->csize, mass->a, mass->b, mass->c);

  state = orig_state;

  dcopy(qtot, save, 1, h[0], 1);
  dcopy(Nmodes, save+qtot, 1, vert[0].hj, 1);
  free(save);
}




void  Tet::HelmMatC(LocMat *mass, Metric *lambda){
  register int i,j,k,n;
  int nbl = Nbmodes, N = Nmodes - nbl, csize = mass->csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  /* fill A */
  for(i = 0,n=0; i < Nverts; ++i,++n){
    fillElmt(B->vert+i);
    HelmHoltz(lambda);
    dcopy(nbl,vert->hj,1,a[n],1);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      HelmHoltz(lambda);
      dcopy(nbl,vert->hj,1,a[n],1);
    }

  for(i = 0; i < Nfaces; ++i)
    for(j = 0; j < (L = face[i].l); ++j)
      for(k = 0; k < L-j; ++k, ++n){
  fillElmt(B->face[i][j]+k);
  HelmHoltz(lambda);
  dcopy(nbl,vert->hj,1,a[n],1);
      }


  /* fill B and C */
  L = interior_l;
  for(i = 0,n = 0; i < L; ++i)
    for(j = 0; j < L-i; ++j)
      for(k = 0; k < L-i-j; ++k,++n){
  fillElmt(B->intr[i][j]+k);
  HelmHoltz(lambda);
  dcopy(nbl,vert->hj    ,1,*b+n,csize);
  dcopy(N  ,vert->hj+nbl,1,c[n],1);
      }

  //  dump_sc(Nbmodes, Nmodes-Nbmodes, a, b, c);

}




void  Pyr::HelmMatC(LocMat *mass, Metric *lambda){
 register int i,j,k,n;
  int          kk;
  int      asize, csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  asize = Nbmodes;
  csize = Nmodes - Nbmodes;

  /* fill A */
  for(i = 0, n = 0; i < NPyr_verts; ++i,++n){
    fillElmt(B->vert+i);
    HelmHoltz(lambda);
    dcopy(asize,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < NPyr_edges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      HelmHoltz(lambda);
      dcopy(asize,vert->hj    ,1,a[n],1);
    }

  for(i = 0; i < NPyr_faces; ++i)
    for(j = 0; j < (L = face[i].l); ++j){
      kk = (Nfverts(i) == 3) ? face[i].l-j:face[i].l;
      for(k = 0; k < kk; ++k, ++n){
  fillElmt(B->face[i][j]+k);
  HelmHoltz(lambda);
  dcopy(asize,vert->hj    ,1,a[n],1);
      }
    }
  /* fill B and C */

  L = interior_l;
  if(csize)
    for(i = 0,n=0; i < L; ++i)
      for(j = 0; j < L-i; ++j)
  for(k = 0; k < L-i-j; ++k){
    fillElmt(B->intr[i][j]+k);
    HelmHoltz(lambda);
    dcopy(asize,vert->hj      ,1,*b+n,csize);
    dcopy(csize,vert->hj+asize,1,c[n],1);
    ++n;
  }
  //  dump_sc(mass->asize, mass->csize, mass->a, mass->b, mass->c);
}




void  Prism::HelmMatC(LocMat *mass, Metric *lambda){
 register int i,j,k,n;
  int          kk;
  int      asize, csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  asize = Nbmodes;
  csize = Nmodes - Nbmodes;

  /* fill A */
  for(i = 0, n = 0; i < NPrism_verts; ++i,++n){
    fillElmt(B->vert+i);
    HelmHoltz(lambda);
    dcopy(asize,vert->hj    ,1,a[n],1);
  }

  for(i = 0; i < NPrism_edges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      HelmHoltz(lambda);
      dcopy(asize,vert->hj    ,1,a[n],1);
    }

  for(i = 0; i < NPrism_faces; ++i)
    for(j = 0; j < (L = face[i].l); ++j){
      kk = (Nfverts(i) == 3) ? face[i].l-j:face[i].l;
      for(k = 0; k < kk; ++k, ++n){
  fillElmt(B->face[i][j]+k);
  HelmHoltz(lambda);
  dcopy(asize,vert->hj    ,1,a[n],1);
      }
    }
  /* fill B and C */

  L = interior_l;

  for(i = 0,n=0; i < L-1; ++i)
    for(j = 0; j < L; ++j)
      for(k = 0; k < L-1-i; ++k, ++n){
  fillElmt(B->intr[i][j]+k);
  HelmHoltz(lambda);
  dcopy(asize,vert->hj      ,1,*b+n,csize);
  dcopy(csize,vert->hj+asize,1,c[n],1);
      }
}




void  Hex::HelmMatC(LocMat *mass, Metric *lambda){
  register int i,j,k,n;
  int nbl = Nbmodes, N = Nmodes - nbl, csize = mass->csize;
  int      L;
  Basis   *B = getbasis();
  double **a = mass->a,
         **b = mass->b,
         **c = mass->c;

  /* fill A */
  for(i = 0,n=0; i < NHex_verts; ++i,++n){
    fillElmt(B->vert+i);
    HelmHoltz(lambda);
    dcopy(nbl,vert->hj,1,a[n],1);
  }

  for(i = 0; i < NHex_edges; ++i)
    for(j = 0; j < (L = edge[i].l); ++j, ++n){
      fillElmt(B->edge[i]+j);
      HelmHoltz(lambda);
      dcopy(nbl,vert->hj,1,a[n],1);
    }

  for(i = 0; i < NHex_faces; ++i)
    for(j = 0; j < (L = face[i].l); ++j)
      for(k = 0; k < L; ++k, ++n){
  fillElmt(B->face[i][j]+k);
  HelmHoltz(lambda);
  dcopy(nbl,vert->hj,1,a[n],1);
      }


  /* fill B and C */
  L = interior_l;
  for(i = 0,n = 0; i < L; ++i)
    for(j = 0; j < L; ++j)
      for(k = 0; k < L; ++k,++n){
  fillElmt(B->intr[i][j]+k);
  HelmHoltz(lambda);
  dcopy(nbl,vert->hj    ,1,*b+n,csize);
  dcopy(N  ,vert->hj+nbl,1,c[n],1);
      }

  //  dump_sc(Nbmodes, Nmodes-Nbmodes, a, b, c);

}




void Element::HelmMatC(LocMat *, Metric *){ERR;}     // return Helmholtz op.



/*

Function name: Element::LapMat

Function Purpose:

Argument 1: LocMat *lap
Purpose:

Function Notes:

*/

void  Tri::LapMat(LocMat *lap){
  register int i,j,k,n;
  int      ll,nbl;
  Basis   *B,*DB;
  Mode    *w,*m,*m1,*md,*md1,**gb,**gb1,*fac;
  double  *z,jac = geom->jac.d;
  double **a = lap->a,
         **b = lap->b,
         **c = lap->c;

  B      = getbasis();
  DB     = derbasis();

  fac    = B->vert;
  w      = mvector(0,0);
  gb     = (Mode **) calloc(Nmodes,sizeof(Mode *));
  gb[0]  = mvecset(0,3*Nmodes,qa,qb,qc);
  gb1    = (Mode **) calloc(Nmodes,sizeof(Mode *));
  gb1[0] = mvecset(0,3*Nmodes,qa,qb,qc);

  for(i = 1; i < Nmodes; ++i) gb[i]  = gb[i-1]+3;
  for(i = 1; i < Nmodes; ++i) gb1[i] = gb1[i-1]+3;

  nbl = Nmodes - face[0].l*(face[0].l+1)/2;

  getzw(qa,&z,&w[0].a,'a');
  getzw(qb,&z,&w[0].b,'b');

  /* fill gb with basis info for laplacian calculation */
  m  = B ->vert;
  md = DB->vert;
  for(i = 0,n=0; i < Nverts; ++i,++n)
    fill_gradbase(gb[n],m+i,md+i,fac);

  for(i = 0; i < Nedges; ++i){
    m1  = B ->edge[i];
    md1 = DB->edge[i];
    for(j = 0; j < edge[i].l; ++j,++n)
      fill_gradbase(gb[n],m1+j,md1+j,fac);
  }

  /* faces */
  for(i = 0; i < Nfaces; ++i){
    ll = face[i].l;
    for(j = 0; j < ll; ++j){
      m1  = B ->face[i][j];
      md1 = DB->face[i][j];
      for(k = 0; k < ll-j; ++k,++n)
  fill_gradbase(gb[n],m1+k,md1+k,fac);
    }
  }

  /* multiply by weights */
  for(n = 0; n < Nmodes; ++n){
    Tri_mvmul2d(qa,qb,qc,gb[n]  ,w,gb1[n]);
    Tri_mvmul2d(qa,qb,qc,gb[n]+1,w,gb1[n]+1);
  }

  /* `A' matrix */
  for(i = 0; i < nbl; ++i)
    for(j = i; j < nbl; ++j)
      a[i][j] = a[j][i] = jac*iprodlap(gb[i],gb1[j],fac+1);

  /* `B' matrix */
  for(i = 0; i < nbl; ++i)
    for(j = nbl; j < Nmodes; ++j)
      b[i][j-nbl] = jac*iprodlap(gb[i],gb1[j],fac+1);

  /* 'C' matrix */
  for(i = nbl; i < Nmodes; ++i)
    for(j = nbl; j < Nmodes; ++j)
      c[i-nbl][j-nbl] = c[j-nbl][i-nbl] = jac*iprodlap(gb[i],gb1[j],fac+1);

  free_mvec(gb[0]) ; free((char *) gb);
  free_mvec(gb1[0]); free((char *) gb1);
  // new addition 7/18/96
  free((char*)w);
}




void  Quad::LapMat(LocMat *){
  fprintf(stderr,"Quad::LapMat   should be calling Quad::HelmMatC\n");
}




void  Tet::LapMat(LocMat *lap){
  Metric *lambda = (Metric*)calloc(1, sizeof(Metric));
  HelmMatC(lap, lambda);
  free(lambda);
}




void  Pyr::LapMat(LocMat *lap){
  Metric *lambda = (Metric*)calloc(1, sizeof(Metric));
  HelmMatC(lap, lambda);
  free(lambda);
}




void  Prism::LapMat(LocMat *lap){
  Metric *lambda = (Metric*)calloc(1, sizeof(Metric));
  HelmMatC(lap, lambda);
  free(lambda);
}




void  Hex::LapMat(LocMat *lap){
  Metric *lambda = (Metric*)calloc(1, sizeof(Metric));
  HelmMatC(lap, lambda);
  free(lambda);
}




void Element::LapMat  (LocMat *){ERR;}             // return Laplacian op.



/*

Function name: Element::project

Function Purpose:

Argument 1: LocMat *m
Purpose:

Argument 2: Bsystem *Ubsys
Purpose:

Function Notes:

*/

void Tri::project(LocMat *m, Bsystem *Ubsys){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int asize  = m->asize;
  double   **a = m->a;

  list = Ubsys->bmap[id];

  if(Ubsys->rslv){ /* project a to first recurrsion */
    /* close pack the a matrix to elimate any extra zero for variable case */
    for(i = 1; i < asize; ++i)
      dcopy(asize,a[i],1,*a+i*asize,1);
    Project_Recur(id, asize, *a, list, 0, Ubsys);
  }
  else{           /* normal static condenstion */
    double **inva = Ubsys->Gmat->inva;
    const    int bwidth = Ubsys->Gmat->bwidth_a;

    if(nsolve = Ubsys->nsolve){
      if(2*bwidth < nsolve){
  /* store as packed banded system */
  for(i = 0; i < asize; ++i)
  if((pi=list[i]) < nsolve)
    for(j  = 0; j < asize; ++j)
      if((pj=list[j]) < nsolve)
        if(pj >= pi)
    inva[pi][pj-pi] += a[i][j];
      }
      else{
  /* store as symmetric system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj <= pi)
      inva[0][(pj*(pj+1))/2 + (nsolve-pj)*pj + (pi-pj)] += a[i][j];
      }

    }
  }
}

void Quad::project(LocMat *m, Bsystem *Ubsys){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int asize  = m->asize;
  double   **a = m->a;

  list = Ubsys->bmap[id];

  if(Ubsys->rslv){ /* project a to first recurrsion */
    /* close pack the a matrix to elimate any extra zero for variable case */
    for(i = 1; i < asize; ++i)
      dcopy(asize,a[i],1,*a+i*asize,1);
    Project_Recur(id,asize, *a, list, 0, Ubsys);
  }
  else{           /* normal static condenstion */

    const    int bwidth = Ubsys->Gmat->bwidth_a;
    double  **inva = Ubsys->Gmat->inva;

    if(nsolve = Ubsys->nsolve){
      if(2*bwidth < nsolve){
  /* store as packed banded system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj >= pi)
      inva[pi][pj-pi] += a[i][j];
      }
      else{
  /* store as symmetric system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj <= pi)
      inva[0][(pj*(pj+1))/2 + (nsolve-pj)*pj + (pi-pj)] += a[i][j];
      }
    }
  }
}




void Tet::project(LocMat *m, Bsystem *Ubsys){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int asize  = m->asize;
  const    int bwidth = Ubsys->Gmat->bwidth_a;
  double   **a = m->a, **inva = Ubsys->Gmat->inva;

  list = Ubsys->bmap[id];

  if(Ubsys->rslv){ /* project a to first recurrsion */
    /* close pack the a matrix to elimate any extra zero for variable case */
    for(i = 1; i < asize; ++i)
      dcopy(asize,a[i],1,*a+i*asize,1);
    Project_Recur(id, asize, *a, list, 0, Ubsys);
  }
  else{           /* normal static condenstion */
    if(nsolve = Ubsys->nsolve){
      if(2*bwidth < nsolve){
  /* store as packed banded system */
  for(i = 0; i < asize; ++i)
  if((pi=list[i]) < nsolve)
    for(j  = 0; j < asize; ++j)
      if((pj=list[j]) < nsolve)
        if(pj >= pi)
    inva[pi][pj-pi] += a[i][j];
      }
      else{
  /* store as symmeTetc system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj <= pi)
      inva[0][(pj*(pj+1))/2 + (nsolve-pj)*pj + (pi-pj)] += a[i][j];
      }

    }
  }
}




void Pyr::project(LocMat *m, Bsystem *Ubsys){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int asize  = m->asize;
  const    int bwidth = Ubsys->Gmat->bwidth_a;
  double   **a = m->a, **inva = Ubsys->Gmat->inva;

  list = Ubsys->bmap[id];

  if(Ubsys->rslv){ /* project a to first recurrsion */
    /* close pack the a matrix to elimate any extra zero for variable case */
    for(i = 1; i < asize; ++i)
      dcopy(asize,a[i],1,*a+i*asize,1);
    Project_Recur(id, asize, *a, list, 0, Ubsys);
  }
  else{           /* normal static condenstion */
    if(nsolve = Ubsys->nsolve){
      if(2*bwidth < nsolve){
  /* store as packed banded system */
  for(i = 0; i < asize; ++i)
  if((pi=list[i]) < nsolve)
    for(j  = 0; j < asize; ++j)
      if((pj=list[j]) < nsolve)
        if(pj >= pi)
    inva[pi][pj-pi] += a[i][j];
      }
      else{
  /* store as symmePyrc system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj <= pi)
      inva[0][(pj*(pj+1))/2 + (nsolve-pj)*pj + (pi-pj)] += a[i][j];
      }

    }
  }
}




void Prism::project(LocMat *m, Bsystem *Ubsys){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int asize  = m->asize;
  const    int bwidth = Ubsys->Gmat->bwidth_a;
  double   **a = m->a, **inva = Ubsys->Gmat->inva;

  list = Ubsys->bmap[id];

  if(Ubsys->rslv){ /* project a to first recurrsion */
    /* close pack the "a" matrix to elimate any extra zero for variable case */
    for(i = 1; i < asize; ++i)
      dcopy(asize,a[i],1,*a+i*asize,1);
    Project_Recur(id, asize, *a, list, 0, Ubsys);
  }
  else{           /* normal static condenstion */
    if(nsolve = Ubsys->nsolve){
      if(2*bwidth < nsolve){
  /* store as packed banded system */
  for(i = 0; i < asize; ++i)
  if((pi=list[i]) < nsolve)
    for(j  = 0; j < asize; ++j)
      if((pj=list[j]) < nsolve)
        if(pj >= pi)
    inva[pi][pj-pi] += a[i][j];
      }
      else{
  /* store as symmetric system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj <= pi)
      inva[0][(pj*(pj+1))/2 + (nsolve-pj)*pj + (pi-pj)] += a[i][j];
      }

    }
  }
}




void Hex::project(LocMat *m, Bsystem *Ubsys){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int asize  = m->asize;
  const    int bwidth = Ubsys->Gmat->bwidth_a;
  double   **a = m->a, **inva = Ubsys->Gmat->inva;

  list = Ubsys->bmap[id];

  if(Ubsys->rslv){ /* project a to first recurrsion */
    /* close pack the a matrix to elimate any extra zero for variable case */
    for(i = 1; i < asize; ++i)
      dcopy(asize,a[i],1,*a+i*asize,1);
    Project_Recur(id, asize, *a, list, 0, Ubsys);
  }
  else{           /* normal static condenstion */
    if(nsolve = Ubsys->nsolve){
      if(2*bwidth < nsolve){
  /* store as packed banded system */
  for(i = 0; i < asize; ++i)
  if((pi=list[i]) < nsolve)
    for(j  = 0; j < asize; ++j)
      if((pj=list[j]) < nsolve)
        if(pj >= pi)
    inva[pi][pj-pi] += a[i][j];
      }
      else{
  /* store as symmeHexc system */
  for(i = 0; i < asize; ++i)
    if((pi=list[i]) < nsolve)
      for(j  = 0; j < asize; ++j)
        if((pj=list[j]) < nsolve)
    if(pj <= pi)
      inva[0][(pj*(pj+1))/2 + (nsolve-pj)*pj + (pi-pj)] += a[i][j];
      }

    }
  }
}




void Element::project(LocMat *, Bsystem *){ERR;}




/*

Function name: Element::condense

Function Purpose:

Argument 1: LocMat *m
Purpose:

Argument 2: Bsystem *Ubsys
Purpose:

Argument 3: char trip
Purpose: if set to 'y' then puts matrix into Ubsys->Gmat structure.

Function Notes:

*/

void Tri::condense(LocMat *m, Bsystem *Ubsys, char pack){
  register int i,j,k;
  int      asize,csize,L,info,n;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id;
  double  **a = m->a, **b = m->b, **c = m->c;
  double  *invc, *binvc;

  /* calc local boundary matrix size */
  asize = Nverts;
  for(i = 0; i < Nedges; ++i)
    asize += edge[i].l;

  csize = Nfmodes();

  m->asize = asize;

  if(csize){
    if(curvX || (Ubsys->lambda->p))
      bw[geom_id] = csize;
    else{
      bw[geom_id] = 2*face->l+1;
    }

    if(csize > 2*bw[geom_id])
      invc  = dvector(0,csize*bw[geom_id]-1);
    else
      invc  = dvector(0,csize*(csize+1)/2-1);

    binvc = dvector(0,asize*csize-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    PackMatrix(c,csize,invc,bw[geom_id]);

    /* calc invc(c) */
    FacMatrix(invc,csize,bw[geom_id]);

    /* calc inv(c)*b^T */
    if(csize > 2*bw[geom_id])
      dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],binvc,csize,info);
    else
      dpptrs('L',csize,asize,invc,binvc,csize,info);

    /* A - b inv(c) b^T */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);
  }

  if(pack == 'y'){
    if(Ubsys->smeth == iterative&&!Ubsys->rslv){
      Ubsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);
      PackMatrix(a,asize,Ubsys->Gmat->a[geom_id],asize);
    }
    else{
      /* sort connectivity issues */
      n = Nverts;
      for(i = 0; i < Nedges; ++i){
  L = edge[i].l;
  if(edge[i].con){
  for(j = 1; j < L; j+=2){
    dscal(asize,-1.0,a[n+j],1);
    for(k=0; k < asize; ++k)
      a[k][n+j] *= -1.0;
  }
      }
  n += L;
      }
    }

    if(csize)
      if(Ubsys->Gmat->invc[geom_id]){
  free(invc);
  free(binvc);
      }
      else{
  Ubsys->Gmat-> invc[geom_id] = invc;
  Ubsys->Gmat->binvc[geom_id] = binvc;
      }
    else /* just set it to non-zero value */
      Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);
  }
  else{ /* free up local arrays */
    if(csize){
      free(invc);
      free(binvc);
    }
  }
}


void Quad::condense(LocMat *m, Bsystem *Ubsys, char pack){
  register int i,j,k;
  int      asize,csize,L,info,n;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id;
  double  **a = m->a, **b = m->b, **c = m->c;
  double  *invc, *binvc;

  /* calc local boundary matrix size */
  asize = Nverts;
  for(i = 0; i < Nedges; ++i)
    asize += edge[i].l;

  csize = face->l*face->l;

  m->asize = asize;

  if(csize){

    bw[geom_id] = csize;
#if 0
    if(curve->type != T_Straight || (Ubsys->lambda->p))
      bw[geom_id] = csize;
    else{
      bw[geom_id] = 3*face->l+1;
    }
#endif
    if(csize > 2*bw[geom_id])
      invc  = dvector(0,csize*bw[geom_id]-1);
    else
      invc  = dvector(0,csize*(csize+1)/2-1);

    binvc = dvector(0,asize*csize-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    PackMatrix(c,csize,invc,bw[geom_id]);

    /* calc invc(c) */
    FacMatrix(invc,csize,bw[geom_id]);

    /* calc inv(c)*b^T */
    if(csize > 2*bw[geom_id])
      dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],binvc,csize,info);
    else
      dpptrs('L',csize,asize,invc,binvc,csize,info);

    /* A - b inv(c) b^T */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);
  }

  // Possible
  if(pack == 'y'){
    if(Ubsys->smeth == iterative&&!Ubsys->rslv){
      // tcew -- store as a symmetrix (full) matrix ??
      Ubsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);
      PackMatrix(a,asize,Ubsys->Gmat->a[geom_id],asize);
    }
    else{
      /* sort connectivity issues */
      n = Nverts;
      for(i = 0; i < Nedges; ++i){
  L = edge[i].l;
      if(edge[i].con){
  for(j = 1; j < L; j+=2){
    dscal(asize,-1.0,a[n+j],1);
    for(k=0; k < asize; ++k)
      a[k][n+j] *= -1.0;
  }
      }
  n += L;
      }
    }

    if(csize)
      if(Ubsys->Gmat->invc[geom_id]){
      free(invc);
      free(binvc);
    }
      else{
  Ubsys->Gmat-> invc[geom_id] = invc;
  Ubsys->Gmat->binvc[geom_id] = binvc;
    }
    else /* just set it to non-zero value */
      Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);
  }
  else{ /* free up local arrays */
    if(csize){
      free(invc);
      free(binvc);
    }
  }
}

void Tet::condense(LocMat *m, Bsystem *Ubsys, char pack){

  register int i,j,k,j1,n1;
  int      asize,csize,L,info,n;
  int     *bw = Ubsys->Gmat->bwidth_c, geomid = geom->id;
  double  **a = m->a, **b = m->b, **c = m->c;
  double  *invc, *binvc;

  /* calc local boundary matrix size */
  asize = Nbmodes;
  csize = Nmodes-Nbmodes;

  m->asize = asize;

  if(csize){
    if(curvX)
      bw[geomid] = csize;
    else
      bw[geomid] = interior_l*(interior_l+1);

    if(csize > 2*bw[geomid])
      invc  = dvector(0,csize*bw[geomid]-1);
    else
      invc  = dvector(0,csize*(csize+1)/2-1);

    binvc = dvector(0,asize*csize-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    PackMatrix(c,csize,invc,bw[geomid]);

    /* calc invc(c) */
    FacMatrix(invc,csize,bw[geomid]);

    /* calc inv(c)*b^T */
    if(csize > 2*bw[geomid])
      dpbtrs('L',csize,bw[geomid]-1,asize,invc,bw[geomid],binvc,csize,info);
    else
      dpptrs('L',csize,asize,invc,binvc,csize,info);

    /* A - b inv(c) b^T */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);
  }

  if(pack == 'y'){
    if(Ubsys->smeth == iterative&&!Ubsys->rslv){
      Ubsys->Gmat->a[geomid] = dvector(0,asize*(asize+1)/2-1);
      PackMatrix(a,asize,Ubsys->Gmat->a[geomid],asize);
    }
    else{
      /* sort connectivity issues */
      n = Nverts;
      for(i = 0; i < Nedges; ++i){
  L = edge[i].l;
  if(edge[i].con){
    for(j = 1; j < L; j+=2){
      dscal(asize,-1.0,a[n+j],1);
      for(k=0; k < asize; ++k)
        a[k][n+j] *= -1.0;
    }
  }
  n += L;
      }

      for(i=0;i<Nfaces;++i){
  L = face[i].l;
  if(face[i].con){
    for(j=1,n1=n+L;j<L;j+=2){
      for(j1=0;j1<L-j;++j1){
        dsmul(asize,-1.0,a[n1+j1],1,a[n1+j1],1);
        for(k=0;k<asize;++k)
    a[k][n1+j1] *= -1.0;
      }
      n1+=2*(L-j)-1;
    }
  }
  n += L*(L+1)/2;
      }
    }

    if(csize)
      if(Ubsys->Gmat->invc[geomid]){
  free(invc);
  free(binvc);
      }
      else{
  Ubsys->Gmat-> invc[geomid] = invc;
  Ubsys->Gmat->binvc[geomid] = binvc;
      }
    else /* just set it to non-zero value */
      Ubsys->Gmat->invc[geomid] = (double *)malloc(1);
  }
  else{ /* free up local arrays */
    if(csize){
      free(invc);
      free(binvc);
    }
  }
}

void Pyr::condense(LocMat *m, Bsystem *Ubsys, char pack){

  register int i,j,k;
  int      asize,csize,L,info,n,n1,j1;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id;
  double  **a = m->a, **b = m->b, **c = m->c;
  double  *invc, *binvc;

  /* calc local boundary matrix size */
  asize = Nbmodes;
  csize = Nmodes-Nbmodes;

  m->asize = asize;

  if(csize){

    bw[geom_id] = csize;

    if(csize > bw[geom_id])
      invc  = dvector(0,csize*bw[geom_id]-1);
    else
      invc  = dvector(0,csize*(csize+1)/2-1);

    binvc = dvector(0,asize*csize-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    PackMatrix(c,csize,invc,bw[geom_id]);

    /* calc invc(c) */
    FacMatrix(invc,csize,bw[geom_id]);

    /* calc inv(c)*b^T */
    if(csize > 2*bw[geom_id])
      dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],binvc,csize,info);
    else
      dpptrs('L',csize,asize,invc,binvc,csize,info);

    /* A - b inv(c) b^T */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);
  }

  if(pack == 'y'){
    if(Ubsys->smeth == iterative&&!Ubsys->rslv){
      Ubsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);
      PackMatrix(a,asize,Ubsys->Gmat->a[geom_id],asize);
    }
    else{
      /* sort connectivity issues */
      n = NPyr_verts;
      for(i = 0; i < NPyr_edges; ++i){
  L = edge[i].l;
  if(edge[i].con){
    for(j = 1; j < L; j+=2){
      dscal(asize,-1.0,a[n+j],1);
      for(k=0; k < asize; ++k)
        a[k][n+j] *= -1.0;
    }
  }
  n += L;
      }
#if 1
      // needs to be fixed
      for(i=0;i<NPyr_faces;++i){
  L = face[i].l;

  // fix for triangle faces
  if(Nfverts(i) == 4){
    switch(face[i].con){
    case 0:
      // nothing to be done
      break;
    case 1: case 5:
      // negate in 'a' direction
      for(k=0;k<L;++k)
        for(j=1;j<L;j+=2){
    dscal(asize,-1.0,a[n+j+L*k],1);
        dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      break;
    case 2: case 6:
      // negate in 'b' direction
      for(k=1;k<L;k+=2)
        for(j=0;j<L;++j){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      break;
    case 3: case  7:
      // negate in 'a' direction
      for(k=0;k<L;++k)
        for(j=1;j<L;j+=2){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      // negate in 'b' direction
      for(k=1;k<L;k+=2)
        for(j=0;j<L;++j){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
      }
      break;
    }
    n += L*L;
  }
  else{
    if(face[i].con){
      for(j=1,n1=n+L;j<L;j+=2){
        for(j1=0;j1<L-j;++j1){
    dsmul(asize,-1.0,a[n1+j1],1,a[n1+j1],1);
    for(k=0;k<asize;++k)
    a[k][n1+j1] *= -1.0;
        }
        n1+=2*(L-j)-1;
      }
    }
    n += L*(L+1)/2;
  }
    }
#endif
    }

    if(csize)
      if(Ubsys->Gmat->invc[geom_id]){
  free(invc);
  free(binvc);
      }
      else{
  Ubsys->Gmat-> invc[geom_id] = invc;
  Ubsys->Gmat->binvc[geom_id] = binvc;
      }
    else /* just set it to non-zero value */
      Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);
  }
  else{ /* free up local arrays */
    if(csize){
      free(invc);
      free(binvc);
    }
  }
}

void Prism::condense(LocMat *m, Bsystem *Ubsys, char pack){

  register int i,j,k;
  int      asize,csize,L,info,n,n1,j1;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id;
  double  **a = m->a, **b = m->b, **c = m->c;
  double  *invc, *binvc;

  /* calc local boundary matrix size */
  asize = Nbmodes;
  csize = Nmodes-Nbmodes;

  m->asize = asize;

  if(csize){

    bw[geom_id] = csize;

    if(csize > bw[geom_id])
      invc  = dvector(0,csize*bw[geom_id]-1);
    else
      invc  = dvector(0,csize*(csize+1)/2-1);

    binvc = dvector(0,asize*csize-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    PackMatrix(c,csize,invc,bw[geom_id]);

    /* calc invc(c) */
    FacMatrix(invc,csize,bw[geom_id]);

    /* calc inv(c)*b^T */
    if(csize > 2*bw[geom_id])
      dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],binvc,csize,info);
    else
      dpptrs('L',csize,asize,invc,binvc,csize,info);

    /* A - b inv(c) b^T */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);
  }

  if(pack == 'y'){
    if(Ubsys->smeth == iterative&&!Ubsys->rslv){
      Ubsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);
      PackMatrix(a,asize,Ubsys->Gmat->a[geom_id],asize);
    }
    else{
      /* sort connectivity issues */
      n = NPrism_verts;
      for(i = 0; i < NPrism_edges; ++i){
  L = edge[i].l;
  if(edge[i].con){
    for(j = 1; j < L; j+=2){
      dscal(asize,-1.0,a[n+j],1);
      for(k=0; k < asize; ++k)
        a[k][n+j] *= -1.0;
    }
  }
  n += L;
      }

      // needs to be fixed
      for(i=0;i<NPrism_faces;++i){
  L = face[i].l;

  // fix for triangle faces
  if(Nfverts(i) == 4){
    switch(face[i].con ){
    case 0:
    // nothing to be done
      break;
    case 1: case 5:
      // negate in 'a' direction
      for(k=0;k<L;++k)
        for(j=1;j<L;j+=2){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      break;
    case 2: case 6:
      // negate in 'b' direction
      for(k=1;k<L;k+=2)
        for(j=0;j<L;++j){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      break;
    case 3: case 7:
    // negate in 'a' direction
      for(k=0;k<L;++k)
        for(j=1;j<L;j+=2){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      // negate in 'b' direction
      for(k=1;k<L;k+=2)
        for(j=0;j<L;++j){
    dscal(asize,-1.0,a[n+j+L*k],1);
    dscal(asize,-1.0,a[0]+n+j+L*k, asize);
        }
      break;
    }
    n += L*L;
  }
  else{
    if(face[i].con){
      for(j=1,n1=n+L;j<L;j+=2){
        for(j1=0;j1<L-j;++j1){
    dsmul(asize,-1.0,a[n1+j1],1,a[n1+j1],1);
    for(k=0;k<asize;++k)
      a[k][n1+j1] *= -1.0;
        }
        n1+=2*(L-j)-1;
      }
    }
    n += L*(L+1)/2;
  }
      }
    }

    if(csize)
      if(Ubsys->Gmat->invc[geom_id]){
  free(invc);
  free(binvc);
      }
      else{
  Ubsys->Gmat-> invc[geom_id] = invc;
  Ubsys->Gmat->binvc[geom_id] = binvc;
      }
    else /* just set it to non-zero value */
      Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);
  }
  else{ /* free up local arrays */
    if(csize){
      free(invc);
      free(binvc);
    }
  }
}




void Hex::condense(LocMat *m, Bsystem *Ubsys, char pack){

  register int i,j,k;
  int      asize,csize,L,info,n;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id;
  double  **a = m->a, **b = m->b, **c = m->c;
  double  *invc, *binvc;

  /* calc local boundary matrix size */
  asize = Nbmodes;
  csize = Nmodes-Nbmodes;

  m->asize = asize;

  if(csize){

    bw[geom_id] = csize;

    if(csize > bw[geom_id])
      invc  = dvector(0,csize*bw[geom_id]-1);
    else
      invc  = dvector(0,csize*(csize+1)/2-1);

    binvc = dvector(0,asize*csize-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    PackMatrix(c,csize,invc,bw[geom_id]);

    /* calc invc(c) */
    FacMatrix(invc,csize,bw[geom_id]);

    /* calc inv(c)*b^T */
    if(csize > 2*bw[geom_id])
      dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],binvc,csize,info);
    else
      dpptrs('L',csize,asize,invc,binvc,csize,info);

    /* A - b inv(c) b^T */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);
  }

  if(pack == 'y'){
    if(Ubsys->smeth == iterative&&!Ubsys->rslv){
      Ubsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);
      PackMatrix(a,asize,Ubsys->Gmat->a[geom_id],asize);
    }
    else{
      /* sort connectivity issues */
      n = NHex_verts;
      for(i = 0; i < NHex_edges; ++i){
  L = edge[i].l;
  if(edge[i].con){
    for(j = 1; j < L; j+=2){
      dscal(asize,-1.0,a[n+j],1);
      for(k=0; k < asize; ++k)
        a[k][n+j] *= -1.0;
    }
  }
  n += L;
      }
#if 1
      // needs to be fixed
      for(i=0;i<NHex_faces;++i){
  L = face[i].l;
  switch(face[i].con ){
  case 0:
    // nothing to be done
    break;
  case 1: case 5:
    // negate in 'a' direction
    for(k=0;k<L;++k)
      for(j=1;j<L;j+=2){
        dscal(asize,-1.0,a[n+j+L*k],1);
        dscal(asize,-1.0,a[0]+n+j+L*k, asize);
      }
    break;
  case 2: case 6:
    // negate in 'b' direction
    for(k=1;k<L;k+=2)
      for(j=0;j<L;++j){
        dscal(asize,-1.0,a[n+j+L*k],1);
        dscal(asize,-1.0,a[0]+n+j+L*k, asize);
      }
    break;
  case 3: case 7:
    // negate in 'a' direction
    for(k=0;k<L;++k)
      for(j=1;j<L;j+=2){
        dscal(asize,-1.0,a[n+j+L*k],1);
        dscal(asize,-1.0,a[0]+n+j+L*k, asize);
      }
    // negate in 'b' direction
    for(k=1;k<L;k+=2)
      for(j=0;j<L;++j){
        dscal(asize,-1.0,a[n+j+L*k],1);
        dscal(asize,-1.0,a[0]+n+j+L*k, asize);
      }
    break;
  }
  n += L*L;
      }
#endif
    }

    if(csize)
      if(Ubsys->Gmat->invc[geom_id]){
  free(invc);
  free(binvc);
      }
      else{
  Ubsys->Gmat-> invc[geom_id] = invc;
  Ubsys->Gmat->binvc[geom_id] = binvc;
      }
    else /* just set it to non-zero value */
      Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);
  }
  else{ /* free up local arrays */
    if(csize){
      free(invc);
      free(binvc);
    }
  }
}

void Element::condense(LocMat *, Bsystem *, char pack){ERR;}

double Tri_mass_mprod(Element *E, Mode *m, double *wvec);
double Quad_mass_mprod(Element *E, Mode *m, double *wvec);
double Tet_mass_mprod(Element *E, Mode *m, double *wvec);
double Pyr_mass_mprod(Element *E, Mode *m, double *wvec);
double Prism_mass_mprod(Element *E, Mode *m, double *wvec);
double Hex_mass_mprod(Element *E, Mode *m, double *wvec);

/*

Function name: Element::fill_diag_massmat

Function Purpose:

Function Notes:

*/

void Tri::fill_diag_massmat(){
  Basis *b;
  double *wa, *wb;
  int i,j,k;
  double *wvec = dvector(0, qtot-1);

  b = getbasis();

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');

  Mode mw, *m;
  mw.a = wa;  mw.b = wb;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Tri_mass_mprod(this, m, wvec);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Tri_mass_mprod(this, m, wvec);
    }

  for(i=0;i<Nfaces;++i)
    for(j=0;j<face[i].l;++j)
      for(k=0;k<face[i].l-j;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Tri_mass_mprod(this, m, wvec);
      }
  free(wvec);
}




void Quad::fill_diag_massmat(){
  Basis *b;
  double *wa, *wb;
  int i,j,k;
  double *wvec = dvector(0, qtot-1);

  b = getbasis();

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');

  Mode mw, *m;
  mw.a = wa;  mw.b = wb;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Quad_mass_mprod(this, m, wvec);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Quad_mass_mprod(this, m, wvec);
    }

  for(i=0;i<Nfaces;++i)
    for(j=0;j<face[i].l;++j)
      for(k=0;k<face[i].l;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Quad_mass_mprod(this, m, wvec);
      }
  free(wvec);
}




void Tet::fill_diag_massmat(){
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  int i,j,k;
  double *wvec = dvector(0, qtot-1);

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');
  getzw(qc,&wc,&wc,'c');

  Mode mw, *m;
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Tet_mass_mprod(this, m, wvec);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Tet_mass_mprod(this, m, wvec);
    }

  for(i=0;i<Nfaces;++i){
    for(j=0;j<face[i].l;++j){
      for(k=0;k<face[i].l-j;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Tet_mass_mprod(this, m, wvec);
      }
    }
  }

  for(i=0;i<interior_l;++i)
    for(j=0;j<interior_l-i;++j)
      for(k=0;k<interior_l-i-j;++k){
  // fix
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Tet_mass_mprod(this, m, wvec);
      }

  free(wvec);
}




void Pyr::fill_diag_massmat(){
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  int i,j,k,nfv;
  double *wvec = dvector(0, qtot-1);

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  getzw(qc,&wc,&wc,'c');

  Mode mw, *m;
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Pyr_mass_mprod(this, m, wvec);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Pyr_mass_mprod(this, m, wvec);
    }

  for(i=0;i<Nfaces;++i){
    nfv = Nfverts(i);
    for(j=0;j<face[i].l;++j){
      if(nfv == 4)
  for(k=0;k<face[i].l;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Pyr_mass_mprod(this, m, wvec);
  }
      else
  for(k=0;k<face[i].l-j;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Pyr_mass_mprod(this, m, wvec);
  }
    }
  }

  for(i=0;i<interior_l;++i)
    for(j=0;j<interior_l-i;++j)
      for(k=0;k<interior_l-i-j;++k){
  // fix
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Pyr_mass_mprod(this, m, wvec);
      }

  free(wvec);
}




void Prism::fill_diag_massmat(){
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  int i,j,k,nfv;
  double *wvec = dvector(0, qtot-1);

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  getzw(qc,&wc,&wc,'b');

  Mode mw, *m;
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Prism_mass_mprod(this, m, wvec);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Prism_mass_mprod(this, m, wvec);
    }

  for(i=0;i<Nfaces;++i){
    nfv = Nfverts(i);
    for(j=0;j<face[i].l;++j){
      if(nfv == 4)
  for(k=0;k<face[i].l;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Prism_mass_mprod(this, m, wvec);
  }
      else
  for(k=0;k<face[i].l-j;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Prism_mass_mprod(this, m, wvec);
  }
    }
  }

  for(i=0;i<interior_l-1;++i)
    for(j=0;j<interior_l;++j)
      for(k=0;k<interior_l-i-1;++k){
  // fix
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Prism_mass_mprod(this, m, wvec);
      }

  free(wvec);
}


void Hex::fill_diag_massmat(){
  Basis *b=getbasis();
  double *wa, *wb, *wc;
  int i,j,k;
  double *wvec = dvector(0, qtot-1);

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  getzw(qc,&wc,&wc,'a');

  Mode mw, *m;
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Hex_mass_mprod(this, m, wvec);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Hex_mass_mprod(this, m, wvec);
    }

  for(i=0;i<Nfaces;++i)
    for(j=0;j<face[i].l;++j)
      for(k=0;k<face[i].l;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Hex_mass_mprod(this, m, wvec);
      }

  for(i=0;i<interior_l;++i)
    for(j=0;j<interior_l;++j)
      for(k=0;k<interior_l;++k){
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Hex_mass_mprod(this, m, wvec);
      }

  free(wvec);
}




void Element::fill_diag_massmat(){ERR;}



double Tri_mass_mprod(Element *E, Mode *m, double *wvec){
  static double *tmp = 0;

  if(!tmp)
    tmp = dvector(0, QGmax*QGmax-1);

  E->fillvec(m , tmp);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  double d = ddot(E->qtot, tmp, 1, wvec, 1);
  return d;
}


double Quad_mass_mprod(Element *E, Mode *m, double *wvec){
  static double *tmp = 0;

  if(!tmp)
    tmp = dvector(0, QGmax*QGmax-1);

  E->fillvec(m , tmp);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  double d = ddot(E->qtot, tmp, 1, wvec, 1);
  return d;
}



double Tet_mass_mprod(Element *E, Mode *m, double *wvec){
  static double *tmp = 0;

  if(!tmp)
    tmp = dvector(0, QGmax*QGmax*QGmax-1);

  E->fillvec(m , tmp);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  double d = ddot(E->qtot, tmp, 1, wvec, 1);
  return d;
}


double Pyr_mass_mprod(Element *E, Mode *m, double *wvec){
  static double *tmp = 0;

  if(!tmp)
    tmp = dvector(0, QGmax*QGmax*QGmax-1);

  E->fillvec(m , tmp);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  double d = ddot(E->qtot, tmp, 1, wvec, 1);
  return d;
}


double Prism_mass_mprod(Element *E, Mode *m, double *wvec){
  static double *tmp = 0;

  if(!tmp)
    tmp = dvector(0, QGmax*QGmax*QGmax-1);

  E->fillvec(m , tmp);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  double d = ddot(E->qtot, tmp, 1, wvec, 1);
  return d;
}



double Hex_mass_mprod(Element *E, Mode *m, double *wvec){
  static double *tmp = 0;

  if(!tmp)
    tmp = dvector(0, QGmax*QGmax*QGmax-1);

  E->fillvec(m , tmp);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  double d = ddot(E->qtot, tmp, 1, wvec, 1);
  return d;
}




double Tri_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda);
double Quad_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda);
double Tet_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda);
double Pyr_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda);
double Prism_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda);
double Hex_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda);


/*

Function name: Element::fill_diag_helmmat

Function Purpose:

Argument 1: Metric *lambda
Purpose:

Function Notes:

*/

void Tri::fill_diag_helmmat(Metric *lambda){
  int i, j, k;
  Basis *b;
  double *wa, *wb;
  double *wvec = dvector(0, qtot-1);
  Mode mw, *m;

  b = getbasis();

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');
  mw.a = wa;  mw.b = wb;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Tri_helm_mprod(this, m, wvec, lambda);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Tri_helm_mprod(this, m, wvec, lambda);
    }

  for(i=0;i<Nfaces;++i)
    for(j=0;j<face[i].l;++j)
      for(k=0;k<face[i].l-j;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Tri_helm_mprod(this, m, wvec, lambda);
      }

  free(wvec);
}




void Quad::fill_diag_helmmat( Metric *lambda){
  int i, j, k;
  Basis *b;
  double *wa, *wb;
  double *wvec = dvector(0, qtot-1);
  Mode mw, *m;

  b = getbasis();

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  mw.a = wa;  mw.b = wb;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Quad_helm_mprod(this, m, wvec, lambda);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Quad_helm_mprod(this, m, wvec, lambda);
    }

  for(i=0;i<Nfaces;++i)
    for(j=0;j<face[i].l;++j)
      for(k=0;k<face[i].l;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Quad_helm_mprod(this, m, wvec, lambda);
      }

  free(wvec);
}




void Tet::fill_diag_helmmat(Metric *lambda){
  int i, j, k;
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  double *wvec = dvector(0, qtot-1);
  Mode mw, *m;

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');
  getzw(qc,&wc,&wc,'c');
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Tet_helm_mprod(this, m, wvec, lambda);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Tet_helm_mprod(this, m, wvec, lambda);
    }

  for(i=0;i<Nfaces;++i){
    for(j=0;j<face[i].l;++j){
      for(k=0;k<face[i].l-j;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Tet_helm_mprod(this, m, wvec, lambda);
      }
    }
  }

  for(i=0;i<interior_l;++i)
    for(j=0;j<interior_l-i;++j)
      for(k=0;k<interior_l-i-j;++k){
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Tet_helm_mprod(this, m, wvec, lambda);
      }


  free(wvec);
}




void Pyr::fill_diag_helmmat(Metric *lambda){
  int i, j, k, nfv;
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  double *wvec = dvector(0, qtot-1);
  Mode mw, *m;

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  getzw(qc,&wc,&wc,'c');
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Pyr_helm_mprod(this, m, wvec, lambda);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      // fix
      m = b->edge[i]+j;
      edge[i].hj[j] = Pyr_helm_mprod(this, m, wvec, lambda);
    }

  for(i=0;i<Nfaces;++i){
    nfv = Nfverts(i);
    for(j=0;j<face[i].l;++j){
      if(nfv == 4)
  for(k=0;k<face[i].l;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Pyr_helm_mprod(this, m, wvec, lambda);
  }
      else
  for(k=0;k<face[i].l-j;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Pyr_helm_mprod(this, m, wvec, lambda);
  }
    }
  }

  for(i=0;i<interior_l;++i)
    for(j=0;j<interior_l-i;++j)
      for(k=0;k<interior_l-i-j;++k){
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Pyr_helm_mprod(this, m, wvec, lambda);
      }


  free(wvec);
}




void Prism::fill_diag_helmmat(Metric *lambda){
  int i, j, k, nfv;
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  double *wvec = dvector(0, qtot-1);
  Mode mw, *m;

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  getzw(qc,&wc,&wc,'b');
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Prism_helm_mprod(this, m, wvec, lambda);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      // fix
      m = b->edge[i]+j;
      edge[i].hj[j] = Prism_helm_mprod(this, m, wvec, lambda);
    }

  for(i=0;i<Nfaces;++i){
    nfv = Nfverts(i);
    for(j=0;j<face[i].l;++j){
      if(nfv == 4)
  for(k=0;k<face[i].l;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Prism_helm_mprod(this, m, wvec, lambda);
  }
      else
  for(k=0;k<face[i].l-j;++k){
    m = b->face[i][j]+k;
    face[i].hj[j][k] = Prism_helm_mprod(this, m, wvec, lambda);
  }
    }
  }

  for(i=0;i<interior_l-1;++i)
    for(j=0;j<interior_l;++j)
      for(k=0;k<interior_l-i-1;++k){
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Prism_helm_mprod(this, m, wvec, lambda);
      }


  free(wvec);
}




void Hex::fill_diag_helmmat(Metric *lambda){
  int i, j, k;
  Basis *b = getbasis();
  double *wa, *wb, *wc;
  double *wvec = dvector(0, qtot-1);
  Mode mw, *m;

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');
  getzw(qc,&wc,&wc,'a');
  mw.a = wa;  mw.b = wb; mw.c = wc;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  for(i=0;i<Nverts;++i){
    m = b->vert+i;
    vert[i].hj[0] = Hex_helm_mprod(this, m, wvec, lambda);
  }

  for(i=0;i<Nedges;++i)
    for(j=0;j<edge[i].l;++j){
      m = b->edge[i]+j;
      edge[i].hj[j] = Hex_helm_mprod(this, m, wvec, lambda);
    }

  for(i=0;i<Nfaces;++i)
    for(j=0;j<face[i].l;++j)
      for(k=0;k<face[i].l;++k){
  m = b->face[i][j]+k;
  face[i].hj[j][k] = Hex_helm_mprod(this, m, wvec, lambda);
      }

  for(i=0;i<interior_l;++i)
    for(j=0;j<interior_l;++j)
      for(k=0;k<interior_l;++k){
  m = b->intr[i][j]+k;
  hj_3d[i][j][k] = Hex_helm_mprod(this, m, wvec, lambda);
      }


  free(wvec);
}




void Element::fill_diag_helmmat(Metric *){ERR;}




double Tri_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda){
  static double *tmp = 0;
  static double *dx = 0;
  static double *dy = 0;
  double *nul = 0;

  if(!tmp){
    tmp = dvector(0, QGmax*QGmax-1);
     dx = dvector(0, QGmax*QGmax-1);
     dy = dvector(0, QGmax*QGmax-1);
  }

  E->fillvec(m , tmp);
  E->Grad_h(tmp, dx, dy, nul, 'a');
  dvmul (E->qtot, dx, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dy, 1, dy, 1, dx, 1, dx, 1);
  if(lambda && lambda->p)
    dvmul(E->qtot, lambda->p, 1, dx, 1, dx, 1);

  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);
  daxpy (E->qtot, lambda->d, tmp, 1, dx, 1);

  double d = ddot(E->qtot, dx, 1, wvec, 1);
  return d;
}



double Quad_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda){
  static double *tmp = 0;
  static double *dx = 0;
  static double *dy = 0;
  double *nul = 0;

  if(!tmp){
    tmp = dvector(0, QGmax*QGmax-1);
     dx = dvector(0, QGmax*QGmax-1);
     dy = dvector(0, QGmax*QGmax-1);
  }

  E->fillvec(m , tmp);
  E->Grad_h(tmp, dx, dy, nul, 'a');
  dvmul (E->qtot, dx, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dy, 1, dy, 1, dx, 1, dx, 1);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);
  if(lambda && lambda->p)
    dvmul(E->qtot, lambda->p, 1, dx, 1, dx, 1);
  daxpy (E->qtot, lambda->d, tmp, 1, dx, 1);

  double d = ddot(E->qtot, dx, 1, wvec, 1);
  return d;
}

double Tet_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda){
  static double *tmp = 0;
  static double *dx = 0;
  static double *dy = 0;
  static double *dz = 0;

  if(!tmp){
    tmp = dvector(0, QGmax*QGmax*QGmax-1);
     dx = dvector(0, QGmax*QGmax*QGmax-1);
     dy = dvector(0, QGmax*QGmax*QGmax-1);
     dz = dvector(0, QGmax*QGmax*QGmax-1);
  }

  E->fillvec(m , tmp);
  E->Grad_h(tmp, dx, dy, dz, 'a');
  dvmul (E->qtot, dx, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dy, 1, dy, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dz, 1, dz, 1, dx, 1, dx, 1);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);
  if(lambda->p)
    dvvtvp(E->qtot, lambda->p, 1, tmp, 1, dx, 1, dx, 1);
  else
    daxpy (E->qtot, lambda->d, tmp, 1, dx, 1);

  double d = ddot(E->qtot, dx, 1, wvec, 1);
  return d;
}

double Pyr_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda){
  static double *tmp = 0;
  static double *dx = 0;
  static double *dy = 0;
  static double *dz = 0;

  if(!tmp){
    tmp = dvector(0, QGmax*QGmax*QGmax-1);
     dx = dvector(0, QGmax*QGmax*QGmax-1);
     dy = dvector(0, QGmax*QGmax*QGmax-1);
     dz = dvector(0, QGmax*QGmax*QGmax-1);
  }

  E->fillvec(m , tmp);
  E->Grad_h(tmp, dx, dy, dz, 'a');
  dvmul (E->qtot, dx, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dy, 1, dy, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dz, 1, dz, 1, dx, 1, dx, 1);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  if(lambda->p)
    dvvtvp (E->qtot, lambda->p, 1, tmp, 1, dx, 1, dx, 1);
  else
    daxpy (E->qtot, lambda->d, tmp, 1, dx, 1);

  double d = ddot(E->qtot, dx, 1, wvec, 1);
  return d;
}

double Prism_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda){
  static double *tmp = 0;
  static double *dx = 0;
  static double *dy = 0;
  static double *dz = 0;

  if(!tmp){
    tmp = dvector(0, QGmax*QGmax*QGmax-1);
     dx = dvector(0, QGmax*QGmax*QGmax-1);
     dy = dvector(0, QGmax*QGmax*QGmax-1);
     dz = dvector(0, QGmax*QGmax*QGmax-1);
  }

  E->fillvec(m , tmp);
  E->Grad_h(tmp, dx, dy, dz, 'a');
  dvmul (E->qtot, dx, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dy, 1, dy, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dz, 1, dz, 1, dx, 1, dx, 1);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

 if(lambda->p)
    dvvtvp (E->qtot, lambda->p, 1, tmp, 1, dx, 1, dx, 1);
  else
    daxpy (E->qtot, lambda->d, tmp, 1, dx, 1);

  double d = ddot(E->qtot, dx, 1, wvec, 1);
  return d;
}

double Hex_helm_mprod(Element *E, Mode *m, double *wvec, Metric *lambda){
  static double *tmp = 0;
  static double *dx = 0;
  static double *dy = 0;
  static double *dz = 0;

  if(!tmp){
    tmp = dvector(0, QGmax*QGmax*QGmax-1);
     dx = dvector(0, QGmax*QGmax*QGmax-1);
     dy = dvector(0, QGmax*QGmax*QGmax-1);
     dz = dvector(0, QGmax*QGmax*QGmax-1);
  }

  E->fillvec(m , tmp);
  E->Grad_h(tmp, dx, dy, dz, 'a');
  dvmul (E->qtot, dx, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dy, 1, dy, 1, dx, 1, dx, 1);
  dvvtvp(E->qtot, dz, 1, dz, 1, dx, 1, dx, 1);
  dvmul (E->qtot, tmp, 1, tmp, 1, tmp, 1);

  if(lambda->p)
    dvvtvp (E->qtot, lambda->p, 1, tmp, 1, dx, 1, dx, 1);
  else
    daxpy (E->qtot, lambda->d, tmp, 1, dx, 1);

  double d = ddot(E->qtot, dx, 1, wvec, 1);
  return d;
}
