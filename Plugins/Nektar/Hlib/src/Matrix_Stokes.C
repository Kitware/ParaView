/************************************************************************/
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
#include "hotel.h"
#include "Tri.h"
#include "Quad.h"

static void MemPrecon (Element_List *U, Bsystem *B, Bsystem *BP);
static void FillPrecon(Element_List *U, Bsystem *B);
static void InvtPrecon(Element_List *U, Bsystem *B);
static void Bsystem_mem_stokes (Bsystem *Ubsys, Bsystem *Pbsys, int );
static void invert_a            (Bsystem *bsys);
static void set_pressure_sign(Element_List *U, Bsystem *Ubsys, Bsystem *Pbsys);
static void Calc_PSC_spectrum(Element_List *U, Element_List *P, Bsystem *BV,
            Bsystem *B);

void GenMat_Stokes(Element_List *U, Element_List *P,
       Bsystem *Ubsys, Bsystem *Pbsys, Metric *lambda){

  LocMat     *helm;
  LocMatDiv *div;
  int     eDIM = U->flist[0]->dim();
  Element *E;
  int symm = option("Reflect");

  if(!Ubsys->signchange){
    setup_signchange(U,Ubsys);
    set_pressure_sign(U,Ubsys,Pbsys);
  }

  Bsystem_mem_stokes(Ubsys,Pbsys,eDIM);


  if(Pbsys->rslv) MemRecur(Pbsys,0,'n');
  if(Ubsys->smeth == iterative) MemPrecon(U,Ubsys,Pbsys);

  for(E = U->fhead; E;E=E->next){
    helm = E->mat_mem ();
    div  = E->divmat_mem (P->flist[E->id]);

    E->DivMat(div,P->flist[E->id]);

    if(Ubsys->rslv||(Ubsys->smeth == direct)||
      !Ubsys->Gmat->invc[E->geom->id]){
      if(E->curvX || lambda[E->id].p || Ubsys->lambda->wave)
  E->HelmMatC (helm,lambda+E->id);
      else{
  if(E->identify() == Nek_Tri)
    Tri_HelmMat(E, helm, lambda[E->id].d);
  else
    E->HelmMatC(helm,lambda+E->id);
      }

      if(Ubsys->lambda[E->id].p)
  fprintf(stderr,"Error: variable viscosity not implemented in "
    "stokes solver\n");
      else{ /* multiply by kinvis */
  dscal(helm->asize*helm->asize,Ubsys->lambda[E->id].d,*helm->a,1);

  if(helm->csize){
    dscal(helm->asize*helm->csize,Ubsys->lambda[E->id].d,*helm->b,1);
    dscal(helm->csize*helm->csize,Ubsys->lambda[E->id].d,*helm->c,1);

    if(Ubsys->lambda->wave)
      dscal(helm->asize*helm->csize,Ubsys->lambda[E->id].d,*helm->d,1);

  }
      }

      E->condense_stokes(helm,div,Ubsys,Pbsys,P->flist[E->id]);
    }

    if(Ubsys->smeth == direct || Pbsys->rslv){
      E->project_stokes(Pbsys,eDIM*div->bsize+1,P->flist[E->id]->geom->id);
    }

    E->mat_free (helm);
    E->divmat_free(div);
  }

  if(Pbsys->rslv){ /* recursive solver setup */
    register int i,j;
    double **a;
    Recur  *rdata;

    Condense_Recur(Pbsys,0,'n');

    for(i = 0; i < Pbsys->rslv->nrecur-1; ++i){
      rdata = Pbsys->rslv->rdata+i;
      a     = Pbsys->rslv->A.a;

      MemRecur(Pbsys,i+1,'n');

      for(j = 0; j < rdata->npatch; ++j)
  Project_Recur(j,rdata->patchlen_a[j],a[j],rdata->map[j],i+1,Pbsys);

      Condense_Recur(Pbsys,i+1,'n');

      for(j = 0; j < rdata->npatch; ++j) free(a[j]); free(a);
    }

    if(Pbsys->smeth == direct)
      Rinvert_a(Pbsys,'n');
    else
      Rpack_a  (Pbsys,'u');
  }
  else /* standard statically condensed set up */
    if(Ubsys->smeth == direct)
      invert_a(Pbsys);  /*eDIM*Ubsys->nsolve*/
    else{
      FillPrecon(U,Pbsys);
      InvtPrecon(U,Pbsys);
    }

#ifdef SC_SPEC
    /* calculate spectrum of preconditioned  statically condensed system */
    Calc_PSC_spectrum(U,P,Ubsys,Pbsys);
#endif

  /* zero U so don't start with funny values */
  U->zerofield();
}

static void invert_a(Bsystem *bsys){
  register int k;
  const int bwidth = bsys->Gmat->bwidth_a;
  const int nsolve = bsys->nsolve;
  int info=0, gid;

  if(nsolve){
    if(bsys->lambda->wave){
      if(3*bwidth < nsolve){    /* banded */

  error_msg(banded nonsymmetric solve is not implemented in invert_a);
  /* set first pressure vertex to be a diagonal entry */
  if(bsys->singular){
    gid = bsys->singular-1;
    dzero(3*bwidth-2,bsys->Gmat->inva[gid],1);
    for(k = 0; k < bwidth; ++k){
      bsys->Gmat->inva[2*bwidth-1+gid-k][k] = 0.0;
      bsys->Gmat->inva[gid][2*bwidth-1] = 1.0;
    }
    /* factor */
    dpbtrf('L', nsolve,   bwidth-1, *bsys->Gmat->inva, bwidth, info);
  }
      }
      else{
  double *s;

  /* set first pressure vertex to be a diagonal entry */
  if(bsys->singular){
    gid = bsys->singular-1;
    s = *bsys->Gmat->inva+gid;
    for(k = 0; k < nsolve; ++k, s += nsolve )
      s[0] = 0.0;
    dzero(nsolve,*bsys->Gmat->inva+gid*nsolve,1);
    bsys->Gmat->inva[0][gid*nsolve+gid] = 1.0;
  }
  bsys->Gmat->pivota = ivector(0,nsolve-1);
  dgetrf(nsolve, nsolve, *bsys->Gmat->inva, nsolve,
         bsys->Gmat->pivota, info);
      }
    }
    else{
      if(2*bwidth < nsolve){    /* banded */
  /* set first pressure vertex to be a diagonal entry */

    error_msg(non implemented non-positive definite factorisation);
  if(bsys->singular){
    gid = bsys->singular-1;
    dzero(bwidth,bsys->Gmat->inva[gid],1);
    for(k = 0; k < min(bwidth,gid+1); ++k)
      bsys->Gmat->inva[gid-k][k] = 0.0;
    bsys->Gmat->inva[gid][0] = 1.0;
  }
  /* factor */
  dpbtrf('L', nsolve,   bwidth-1, *bsys->Gmat->inva, bwidth, info);
      }
      else{
  double *s;

  /* set first pressure vertex to be a diagonal entry */
  if(bsys->singular){
    gid = bsys->singular-1;
    s = *bsys->Gmat->inva+gid;
    for(k = 0; k < gid; ++k, s += nsolve-k )
      s[0] = 0.0;
    dzero(nsolve-gid,s,1);
    s[0] = 1.0;
  }
#ifdef DUMP_SC
  writesystem('l',*bsys->Gmat->inva,nsolve,nsolve);
  exit(-1);
#endif
  bsys->Gmat->pivota = ivector(0,nsolve-1);
  dsptrf('L', nsolve,  *bsys->Gmat->inva, bsys->Gmat->pivota, info);
      }
    }
  }
  if(info) fprintf(stderr,"invertA: info not zero\n");
}


static void Bsystem_mem_stokes(Bsystem *Ubsys, Bsystem *Pbsys, int eDIM){
  register int i;
  int nfam = Ubsys->families;
  const int nsolve = Pbsys->nsolve;

  /* pressure system */
  /* reset nsolve for bsystem */
  Pbsys->nsolve = nsolve;

  Pbsys->Gmat->a     = (double **)calloc(nfam,sizeof(double *));

  if(!Pbsys->rslv){
    if(Pbsys->smeth == direct){
      if(nsolve){
  Pbsys->Gmat->bwidth_a = nsolve;
  if(Ubsys->lambda->wave){ /* Oseen Solver */
     if(3*Pbsys->Gmat->bwidth_a  < nsolve){  /* banded solver    */
      Pbsys->Gmat->inva = dmatrix(0,nsolve-1,0,
          3*Pbsys->Gmat->bwidth_a-3);
      dzero(nsolve*(3*Pbsys->Gmat->bwidth_a-2),*Pbsys->Gmat->inva,1);
    }
    else{      /* symmetric solver */
      Pbsys->Gmat->inva  = dmatrix(0,0,0,nsolve*nsolve-1);
      dzero(nsolve*nsolve,*Pbsys->Gmat->inva,1);
    }

  }
  else{
     if(2*Pbsys->Gmat->bwidth_a  < nsolve){  /* banded solver    */
      Pbsys->Gmat->inva = dmatrix(0,nsolve-1,0,Pbsys->Gmat->bwidth_a-1);
      dzero(nsolve*Pbsys->Gmat->bwidth_a,*Pbsys->Gmat->inva,1);
    }
    else{      /* symmetric solver */
      Pbsys->Gmat->inva  = dmatrix(0,0,0,nsolve*(nsolve+1)/2-1);
      dzero(nsolve*(nsolve+1)/2,*Pbsys->Gmat->inva,1);
    }
  }
      }
      else
  Pbsys->Gmat->inva = (double **)malloc(sizeof(double *));
    }
  }

  Pbsys->Gmat->bwidth_c = ivector(0,nfam-1);
  Pbsys->Gmat-> invc    = (double **)calloc(nfam,sizeof(double *));
  Pbsys->Gmat->binvc    = (double **)calloc(nfam,sizeof(double *));

  /* velocity system memory requirements */
  if (!Ubsys->Gmat)
    Ubsys->Gmat = (MatSys  *)calloc(1,sizeof(MatSys));

  Ubsys->Gmat->dbinvc   = (double ***)calloc(eDIM,sizeof(double**));
  for(i = 0; i < eDIM; ++i)
    Ubsys->Gmat->dbinvc[i] = (double **)calloc(nfam,sizeof(double *));

  Ubsys->Gmat->bwidth_c = ivector(0,nfam-1);
  Ubsys->Gmat-> invc    = (double **)calloc(nfam,sizeof(double *));
  Ubsys->Gmat->binvc    = (double **)calloc(nfam,sizeof(double *));

  if(Ubsys->lambda->wave){
    Pbsys->Gmat->cipiv = (int **)calloc(nfam,sizeof(int *));
    Pbsys->Gmat->invcd = (double **)calloc(nfam,sizeof(double *));
    Ubsys->Gmat->cipiv = (int **)calloc(nfam,sizeof(int *));
    Ubsys->Gmat->invcd = (double **)calloc(nfam,sizeof(double *));
  }
  return;
}

/* Matrix preconditioning system */
static void MemPrecon(Element_List *U, Bsystem *B, Bsystem *BP){
  MatPre   *M = BP->Pmat = (MatPre *) malloc(sizeof(MatPre));

  switch(BP->Precon){
  case Pre_Diag:
    M->info.diag.ndiag = BP->nsolve;
    M->info.diag.idiag = dvector(0,BP->nsolve-1);
    dzero(BP->nsolve,M->info.diag.idiag,1);
    break;
  case Pre_Block:{
    register int i;
    int nvs = B->nv_solve;
    int nes = B->ne_solve;
    int eDIM = U->fhead->dim(),gid,ll;
    Element *E;

    M->info.block.nvert = eDIM*nvs;
    M->info.block.iedge = (double **)calloc((nes)?nes:1,sizeof(double *));

    M->info.block.nedge = nes;
    M->info.block.Ledge = ivector(0,(nes)?nes-1:0);
    izero(nes, M->info.block.Ledge, 1);

    /* set edges */
    for(E=U->fhead;E;E=E->next)
      for(i = 0; i < E->Nedges; ++i){
  gid = E->edge[i].gid;
  if(gid < nes)
    M->info.block.Ledge[gid] = eDIM*E->edge[i].l;
      }

    if(eDIM == 3)
      fprintf(stderr,"MemPrecon not set up for 3D solves \n");

    /* declare memory */
#ifdef BVERT
    /* at present use symmetric storage */
    M->info.block.ivert = dvector(0,eDIM*nvs*(eDIM*nvs+1)/2-1);
    dzero(eDIM*nvs*(eDIM*nvs+1)/2,M->info.block.ivert,1);
#else
    M->info.block.ivert = dvector(0,eDIM*nvs-1);
    dzero(eDIM*nvs,M->info.block.ivert,1);
#endif

    for(i = 0; i < nes; ++i){
      ll = M->info.block.Ledge[i];
      ll = ll*(ll+1)/2;
      M->info.block.iedge[i]  = dvector(0,ll-1);
      dzero(ll,M->info.block.iedge[i],1);
    }
  }
    break;
  case Pre_None:
    break;
  case Pre_Diag_Stokes:
    M->info.diagst.ndiag = BP->nsolve-BP->nel;
    M->info.diagst.idiag = dvector(0,BP->nsolve-1);
    dzero(BP->nsolve,M->info.diagst.idiag,1);
    M->info.diagst.nbcb  = BP->nel;
    M->info.diagst.binvcb = dvector(0,BP->nel*(BP->nel+1)/2-1);
    break;
  case Pre_Block_Stokes:
    register int i;
    int nvs = B->nv_solve;
    int nes = B->ne_solve;
    int eDIM = U->fhead->dim(),gid,ll;
    Element *E;

    M->info.blockst.nvert = eDIM*nvs;
    M->info.blockst.iedge = (double **)calloc((nes)?nes:1,sizeof(double *));

    M->info.blockst.nedge = nes;
    M->info.blockst.Ledge = ivector(0,(nes)?nes-1:0);
    izero(nes, M->info.blockst.Ledge, 1);

    /* set edges */
    for(E=U->fhead;E;E=E->next)
      for(i = 0; i < E->Nedges; ++i){
  gid = E->edge[i].gid;
  if(gid < nes)
    M->info.blockst.Ledge[gid] = eDIM*E->edge[i].l;
      }

    if(eDIM == 3)
      fprintf(stderr,"MemPrecon not set up for 3D solves \n");

    /* declare memory */
#ifdef BVERT
    /* at present use symmetric storage */
    M->info.blockst.ivert = dvector(0,eDIM*nvs*(eDIM*nvs+1)/2-1);
    dzero(eDIM*nvs*(eDIM*nvs+1)/2,M->info.blockst.ivert,1);
#else
    M->info.blockst.ivert = dvector(0,eDIM*nvs-1);
    dzero(eDIM*nvs,M->info.blockst.ivert,1);
#endif

    for(i = 0; i < nes; ++i){
      ll = M->info.blockst.Ledge[i];
      ll = ll*(ll+1)/2;
      M->info.blockst.iedge[i]  = dvector(0,ll-1);
      dzero(ll,M->info.blockst.iedge[i],1);
    }

    M->info.blockst.nbcb  = BP->nel;
    M->info.blockst.binvcb = dvector(0,BP->nel*(BP->nel+1)/2-1);
    break;
  }
}

static void FillPrecon(Element_List *U, Bsystem *B){
  register int i,k;
  int      eDIM = U->fhead->dim(),nbl,pos,id;
  int      **bmap = B->bmap;
  double   *A;

  switch(B->Precon){
  case Pre_Diag: case Pre_Diag_Stokes:{
    double *inv;
    if(B->Precon == Pre_Diag)
      inv = B->Pmat->info.diag.idiag;
    else
      inv = B->Pmat->info.diagst.idiag;

    for(k = 0; k < B->nel; ++k){
      A   = B->Gmat->a[U->flist[k]->geom->id];
      nbl = eDIM*U->flist[k]->Nbmodes+1;

      /* A is packed in upper format for this case */
      for(i = 0,pos = 0; i < nbl-1; ++i,pos+=i+1)
  if((id = bmap[k][i]) < B->nsolve) inv[id] += A[pos];
#if 0
      if((id = bmap[k][nbl-1]) < B->nsolve) inv[id] = 1.0;
#else
      if((id = bmap[k][nbl-1]) < B->nsolve)
  inv[id] = U->flist[k]->get_1diag_massmat(0);
#endif
    }
  }
    break;
  case Pre_Block: case Pre_Block_Stokes:{
    register int i,j,d,n;
    int gid,l,Nbmodes,cnt,acnt,sign,sign1,nedge;
    double *s,**iedge,*inv;
    Element *E;

    if(B->Precon == Pre_Block){
      nedge = B->Pmat->info.block.nedge;
      iedge = B->Pmat->info.block.iedge;
      inv   = B->Pmat->info.block.ivert;
    }
    else{
      nedge = B->Pmat->info.blockst.nedge;
      iedge = B->Pmat->info.blockst.iedge;
      inv   = B->Pmat->info.blockst.ivert;
    }

    if(eDIM == 3)
      fprintf(stderr,"ERROR: fillprecon not set up for 3d\n");

    for(E=U->fhead; E; E = E->next){
      A       = B->Gmat->a[E->geom->id];
      Nbmodes = E->Nbmodes;

      pos  = 0;
      acnt = 2;
      for(d = 0; d < eDIM; ++d){
#if BVERT
  for(i = 0; i < E->Nverts; ++i,pos+=acnt++)
    if((id = bmap[E->id][i+d*Nbmodes]) < B->nsolve){
      if(d)
        for(j = 0; j < E->Nverts; ++j)
    if((id1 = bmap[E->id][j]) < B->nsolve)
      if(id1 < id)
        inv[id*(id+1)/2+id1]  += A[pos-acnt+2+j];
      else
        inv[id1*(id1+1)/2+id] += A[pos-acnt+2+j];

      for(j = 0; j <= i; ++j)
        if((id1 = bmap[E->id][j+d*Nbmodes]) < B->nsolve)
    if(id1 < id)
      inv[id*(id+1)/2+id1]  += A[pos - i + j];
    else
      inv[id1*(id1+1)/2+id] += A[pos - i + j];
    }
#else
  for(i = 0; i < E->Nverts; ++i,pos+=acnt++)
    if((id = bmap[E->id][i+d*Nbmodes]) < B->nsolve)
      inv[id] += A[pos];
#endif

  for(i = 0; i < E->Nedges; ++i){
    gid = E->edge[i].gid;
    l   = E->edge[i].l;
    if(gid < nedge){
      s  = iedge[gid];
#if 0
      cnt = d*((eDIM-1)*l*((eDIM-1)*l+1)/2) + d*l;
      if(E->edge[i].con)
        for(j = 0,sign1=1;j<l;++j,cnt+=j+d*l,pos+=acnt++,sign1=-sign1)
    for(n = 0,sign=sign1; n < j+1; ++n,sign=-sign)
      s[cnt+n] += sign*A[pos-j+n];
      else
        for(j = 0; j < l;++j,cnt+=j+d*l,pos+=acnt++)
    dvadd(j+1,A+pos-j,1,s+cnt,1,s+cnt,1);
#else
      /* note this is not set up for variable polynomial order */
      cnt = d*((eDIM-1)*l*((eDIM-1)*l+1)/2);

      if(E->edge[i].con){
        for(j = 0,sign1=1;j<l;++j,cnt+=j+d*l,pos+=acnt++,sign1=-sign1){
    if(d)
      for(n = 0,sign=sign1; n < l; ++n,sign=-sign)
        s[cnt+n] += sign*A[pos-j-(E->Nedges*l+E->Nverts)+n];
    for(n = 0,sign=sign1; n < j+1; ++n,sign=-sign)
      s[cnt+d*l+n] += sign*A[pos-j+n];
        }
      }
      else{
    for(j = 0; j < l;++j,cnt+=j+d*l,pos+=acnt++){
      if(d)
        dvadd(l,A+pos-j-(E->Nedges*l+E->Nverts),1,s+cnt,1,s+cnt,1);
      dvadd(j+1,A+pos-j,1,s+cnt+d*l,1,s+cnt+d*l,1);
    }
        }
#endif
    }
    else
      for(j = 0; j < l; ++j, pos+=acnt++);
  }
      }
    }
  }
    break;
  case Pre_None:
    break;
  }
}

static void InvtPrecon(Element_List *U, Bsystem *B){
  switch(B->Precon){
  case Pre_Diag:
    dvrecp(B->Pmat->info.diag.ndiag,B->Pmat->info.diag.idiag,
     1,B->Pmat->info.diag.idiag,1);
    break;
  case Pre_Block:{
    register int i;
    int eDIM = U->fhead->dim();
    int nedge = B->Pmat->info.block.nedge;
    int info;

#ifdef BVERT
    dpptrf('U',B->Pmat->info.block.nvert,
     B->Pmat->info.block.ivert,info);
    if(info)
      fprintf(stderr,"Error inverting matrix in InvtPrecon (stokes)\n");
#else
    dvrecp(B->Pmat->info.block.nvert,B->Pmat->info.block.ivert,
     1,B->Pmat->info.block.ivert,1);
#endif
    for(i = 0; i < nedge; ++i){
      dpptrf('U',B->Pmat->info.block.Ledge[i],
       B->Pmat->info.block.iedge[i],info);
      if(info)
  fprintf(stderr,"Error inverting matrix in InvtPrecon (stokes)\n");
    }
  }
    break;
  case Pre_None:
    break;
  case Pre_Diag_Stokes:{
    register int i,j,k;
    int    eDIM = U->fhead->dim();
    int    nel = B->nel,Nbmodes,asize,info,cnt;
    int    **bmap = B->bmap;
    double *tmp = dvector(0,B->nglobal-1);
    double **wk,*b,*sc,*sc1, *binvcb;

    if(eDIM == 2)
      wk = dmatrix(0,1,0,8*LGmax);
    else
      wk = dmatrix(0,1,0,18*LGmax*LGmax);

    dvrecp(B->Pmat->info.diagst.ndiag,B->Pmat->info.diagst.idiag,
     1,B->Pmat->info.diagst.idiag,1);

    sc = B->signchange;
    binvcb  = B->Pmat->info.diagst.binvcb;
    for(cnt = 0,k = 0; k < nel; ++k){

      /* gather B from local systems */
      Nbmodes = U->flist[k]->Nbmodes;
      asize   = Nbmodes*eDIM+1;
      b       = B->Gmat->a[U->flist[k]->geom->id] + asize*(asize-1)/2;

      dzero(B->nglobal,tmp,1);
      for(i = 0; i < asize-1; ++i)
  tmp[bmap[k][i]] += sc[i]*b[i];

      dzero(B->nglobal-B->nsolve+nel,tmp+B->nsolve-nel,1);

      /* multiply by invc */
      dvmul(B->Pmat->info.diagst.ndiag,B->Pmat->info.diagst.idiag,1,
      tmp,1,tmp,1);

      /* take inner product with B' */
      sc1 = B->signchange;
      for(j = k; j < nel; ++j,cnt++){
  Nbmodes   = U->flist[j]->Nbmodes;
  asize     = Nbmodes*eDIM+1;

  dzero (asize,wk[0],1);
  dgathr(asize-1, tmp, bmap[j],  wk[0]);
  dvmul (asize-1, sc1, 1, wk[0], 1, wk[0], 1);

  b    = B->Gmat->a[U->flist[j]->geom->id] + asize*(asize-1)/2;
  binvcb[cnt] = ddot(asize-1,b,1,wk[0],1);
  sc1 += asize;
      }
      sc += eDIM*U->flist[k]->Nbmodes+1;
    }
    /* take out first row to deal with singularity ? */
    dzero(nel,binvcb,1);  binvcb[0] = 1.0;
    dpptrf('L', nel, binvcb, info);
    if(info)
      fprintf(stderr,"error in inverting binvcb\n");

    free(tmp); free_dmatrix(wk,0,0);
  }
    break;
  case Pre_Block_Stokes:{
    register int i,j,k;
    int    eDIM   = U->fhead->dim(),l;
    int    nel    = B->nel,Nbmodes,asize,info,cnt,cnt1;
    int    nedge  = B->Pmat->info.blockst.nedge;
    int    **bmap = B->bmap;
    double *tmp   = dvector(0,B->nglobal-1);
    double **wk,*b,*sc,*sc1, *binvcb;

    if(eDIM == 2)
      wk = dmatrix(0,1,0,8*LGmax);
    else
      wk = dmatrix(0,1,0,18*LGmax*LGmax);

    /* invert C system */
#ifdef BVERT
    /* make up diagonal precon for test */
/*    for(i = 0,j=0; i < B->Pmat->info.blockst.nvert; ++i,j+=i)
      dzero(i,B->Pmat->info.blockst.ivert+j,1);*/

    dpptrf('U',B->Pmat->info.blockst.nvert,
     B->Pmat->info.blockst.ivert,info);
    if(info)
      fprintf(stderr,"Error inverting matrix in InvtPrecon (stokes)\n");
#else
    dvrecp(B->Pmat->info.blockst.nvert,B->Pmat->info.blockst.ivert,
     1,B->Pmat->info.blockst.ivert,1);
#endif

    for(i = 0; i < nedge; ++i){
    /* make up diagonal precon for test */
/*      for(j = 0,k=0; j < B->Pmat->info.blockst.Ledge[i]; ++j,k+=j)
  dzero(j,B->Pmat->info.blockst.iedge[i]+k,1);*/

      dpptrf('U',B->Pmat->info.blockst.Ledge[i],
       B->Pmat->info.blockst.iedge[i],info);
      if(info)
  fprintf(stderr,"Error inverting matrix in InvtPrecon (stokes)\n");
    }

    sc = B->signchange;
    binvcb  = B->Pmat->info.blockst.binvcb;
    for(cnt = 0,k = 0; k < nel; ++k){

      /* Gather B from local systems */
      Nbmodes = U->flist[k]->Nbmodes;
      asize   = Nbmodes*eDIM+1;
      b       = B->Gmat->a[U->flist[k]->geom->id] + asize*(asize-1)/2;

      dzero(B->nglobal,tmp,1);
      for(i = 0; i < asize-1; ++i)
  tmp[bmap[k][i]] += sc[i]*b[i];

      dzero(B->nglobal-B->nsolve+nel,tmp+B->nsolve-nel,1);

      /* Multiply by invc */

#ifdef BVERT
      dpptrs('U',B->Pmat->info.blockst.nvert,1,B->Pmat->info.blockst.ivert,
       tmp,B->Pmat->info.blockst.nvert,info);
#else
      dvmul(B->Pmat->info.blockst.nvert,B->Pmat->info.blockst.ivert,1,
      tmp,1,tmp,1);
#endif

      cnt1 = B->Pmat->info.blockst.nvert;
      for(i = 0; i < nedge; ++i){
  l = B->Pmat->info.blockst.Ledge[i];
  dpptrs('U',l,1,B->Pmat->info.blockst.iedge[i],tmp+cnt1,l,info);
  cnt1 += l;
      }

      /* Take inner product with B' */
      sc1 = B->signchange;
      for(j = k; j < nel; ++j,cnt++){
  Nbmodes   = U->flist[j]->Nbmodes;
  asize     = Nbmodes*eDIM+1;

  dzero (asize,wk[0],1);
  dgathr(asize-1, tmp, bmap[j],  wk[0]);
  dvmul (asize-1, sc1, 1, wk[0], 1, wk[0], 1);

  b    = B->Gmat->a[U->flist[j]->geom->id] + asize*(asize-1)/2;
  binvcb[cnt] = ddot(asize-1,b,1,wk[0],1);
  sc1 += asize;
      }
      sc += eDIM*U->flist[k]->Nbmodes+1;
    }
    /* take out first row to deal with singularity ? */
    dzero(nel,binvcb,1);  binvcb[0] = 1.0;
    dpptrf('L', nel, binvcb, info);
    if(info)
      fprintf(stderr,"error in inverting binvcb\n");

    free(tmp); free_dmatrix(wk,0,0);
  }
    break;
  }
}

void PackFullMatrix(double **a, int n, double *b, int bwidth){
  register int i;

  if(n>3*bwidth){ /* banded general form */
    int totw = 3*bwidth-2;
    double *s;


    for(i = 0,s=b; i < bwidth-1; ++i,s+=totw)
      dcopy(bwidth+i,*a+i,n,s+totw-bwidth-i,1);

    for(i = bwidth-1; i < n-bwidth+1; ++i,s+=totw)
      dcopy(2*bwidth-1,a[i+1-bwidth]+i,n,s,1);

    for(i = n-bwidth+1; i < n; ++i,s+=totw)
      dcopy(bwidth+n-1-i,a[i-bwidth]+i,n,s,1);

  }
  else{        /* full matrix trasposed */
    for(i = 0; i < n; ++i)
      dcopy(n, *a+i, n, b+i*n, 1);
  }
}

/* factor positive symmertric and banded matrices */
void FacFullMatrix(double *a, int n, int *ipiv, int bwidth){

  int info;
  if(n>3*bwidth)
    dgbtrf(n,n,bwidth-1,bwidth-1,a,3*bwidth-2,ipiv,info);
  else
    dgetrf(n,n,a,n,ipiv,info);

  if(info) error_msg(FacMatrixP: info not zero);
  return;
}

static void set_pressure_sign(Element_List *U, Bsystem *Ubsys, Bsystem *Pbsys){
  int eDIM = U->fhead->dim();
  int len,Nbmodes;
  double *sc,*sc1;
  Element *E;

  len = 0;
  for(E=U->fhead;E;E = E->next)
    len += E->Nbmodes*eDIM + 1;

  Pbsys->signchange = dvector(0,len-1);

  sc  = Ubsys->signchange;
  sc1 = Pbsys->signchange;
  len = 0;

  for(E=U->fhead;E;E = E->next){
    Nbmodes = E->Nbmodes;
    dcopy(Nbmodes,sc,1,sc1,1);
    sc1 += Nbmodes;
    dcopy(Nbmodes,sc,1,sc1,1);
    sc1 += Nbmodes;
    sc1[0] = 1;
    ++sc1;
    sc += Nbmodes;
  }
}

#ifdef SC_SPEC
static void Calc_PSC_spectrum(Element_List *U, Element_List *P, Bsystem *BV,
            Bsystem *B){
  register int i,j,k;
  double   **A,*aloc;
  double   *sign;
  int      eDIM   = U->fhead->dim();
  int      nbl,cnt,asize;
  int      **bmap = B->bmap;
  int      nsolve = B->nsolve;
  int      nel    = B->nel;

  if(B->smeth != iterative) {
    fputc('\n',stderr);
    error_msg(must run with -i option to call Calc_PSC_spectrum);
  }

  if(!nsolve){
    fprintf(stderr,"Calc_PSC_spectrum: no boundary system, returning\n");
    return;
  }

  /* Generate SC matrix from local contributions */
  A = dmatrix(0,nsolve-1,0,nsolve-1);
  dzero(nsolve*nsolve,*A,1);

  sign = B->signchange;
  for(k = 0; k < nel; ++k){
    nbl   = U->flist[k]->Nbmodes;
    asize = eDIM*nbl+1;

    aloc = B->Gmat->a[U->flist[k]->geom->id];

    /* fill matrix from upper symmetrically packed local matrices */
    cnt = 0;
    for(i = 0; i < asize; ++i){
      if(bmap[k][i] < nsolve){
  for(j = 0; j < i; ++j)
    if(bmap[k][j] < nsolve){
      A[bmap[k][i]][bmap[k][j]] += sign[i]*sign[j]*aloc[cnt];
      A[bmap[k][j]][bmap[k][i]] += sign[i]*sign[j]*aloc[cnt++];
    }
    else
      ++cnt;
  /* do diagonal */
  A[bmap[k][i]][bmap[k][i]] += aloc[cnt++];
      }
      else
  cnt += i+1;
    }
    sign += asize;
  }

#ifdef TEST1
  double *C = dvector(0,nsolve*(nsolve+1)/2-1);
  int    info;
  dzero(nsolve*(nsolve+1)/2,C,1);

  /* pack in velocity part of matrix in lower format */
  for(i = 0, j=0; i < nsolve-nel; j+=nsolve-i++)
    dcopy(nsolve-nel-i,A[i]+i,1,C+j,1);

  /* set singular vertex to 1 */
  C[j] = 1.0;
  j += nsolve-i++;

  /* fill remain of matrix with mass matrix of vertices */
  for(; i < nsolve; j+=nsolve-i++)
    C[j] = P->flist[i-(nsolve-nel)]->get_1diag_massmat(0);

  /* invert C*/
  FacMatrix(C,nsolve,nsolve);

  /* find C^{-1} A */
  dpptrs('L',nsolve,nsolve,C,*A,nsolve,info);
  if(info) error_msg("error inverting C matrix");

  /* get eigenvalues of invM*A */
  fprintf(stdout,"\n Calculating eigenvalues\n"); fflush(stdout);
  FullEigenMatrix(A,nsolve,nsolve,0);

  free_dmatrix(A,0,0);  free(C);
  exit(-1);
#else
#ifdef TEST2
  int    vsize = nsolve-nel;
  double *Bsys = dvector(0,vsize*(vsize+1)/2-1);
  double **Psys = dmatrix(0,nel-1,0,nel-1);
  double **btmp = dmatrix(0,nel-1,0,vsize-1),minv;
  int    info;

#if 0
  /* take out singular pressure modes */
  dzero(nsolve,A[0] + nsolve-nel, nsolve);
  dzero(nsolve,A[nsolve-nel],1);
  A[nsolve-nel][nsolve-nel] = 1.0;
#endif

  dzero(vsize*(vsize+1)/2,Bsys,1);
  dzero(nel*nel,Psys[0],1);

  /* pack in velocity part of matrix in lower format */
  for(i = 0, j=0; i < vsize; j+=vsize-i++)
    dcopy(vsize-i,A[i]+i,1,Bsys+j,1);

  /* invert B*/
  FacMatrix(Bsys,vsize,vsize);

  /* fill b into btmp */
  for(i = 0; i < nel; ++i)
    dcopy(vsize,A[vsize+i],1,btmp[i],1);

  /*calculate inv(B) b */
  dpptrs('L',vsize,nel,Bsys,*btmp,vsize,info);
  if(info) error_msg("error inverting C matrix");

  /* calculate inv(M) b' inv(B) b */
  for(i = 0; i < nel; ++i){
    minv = 1.0/P->flist[i]->get_1diag_massmat(0);
    for(j = 0; j < nel; ++j)
      Psys[i][j] = minv*ddot(vsize,A[vsize+i],1,btmp[j],1);
  }

  /* eliminate singular mode */
/*  dzero(nel,Psys[0],1);
  dzero(nel,Psys[0],nel);
  Psys[0][0] = 1.0; */

  /* get eigenvalues of invM*A */
  fprintf(stdout,"\n Calculating eigenvalues\n"); fflush(stdout);
  FullEigenMatrix(Psys,nel,nel,0);
  exit(-1);
#else

#if 0
  /* take out singular pressure modes */
  dzero(nsolve,A[0] + nsolve-nel, nsolve);
  dzero(nsolve,A[nsolve-nel],1);
  A[nsolve-nel][nsolve-nel] = 1.0;
#endif

  /* operate preconditioner on A[i] */
  for(i = 0; i < nsolve; ++i)
    Precon_Stokes(U,B,A[i],A[i]);

#if 1
  int    vsize = nsolve-nel;
  double **Bsys = dmatrix(0,vsize-1,0,vsize-1);

  /* just take out the velocity space and find eigenvalues */
  for(i = 0; i < vsize; ++i)
    dcopy(vsize,A[i],1,Bsys[i],1);

  /* get eigenvalues of invM*A */
  fprintf(stdout,"\n Calculating eigenvalues\n"); fflush(stdout);
  FullEigenMatrix(Bsys,vsize,vsize,0);
  exit(2);
#endif

#if 0
/* check to see if the product of solver gives the same as the
   matrix system above */
  Element_List *V[3];
  Bsystem      *Bs[3];
  double *p1 = dvector(0,B->nglobal-1);
  double *w = dvector(0,B->nglobal-1);
  double **wk = dmatrix(0,1,0,eDIM*4*LGmax);

  V[0] = U; V[2] = P; Bs[0] = BV; Bs[2] = B;

  for(i = 0; i < nsolve; ++i){
    dzero(B->nglobal,p1,1);
    p1[i] = 1.0;
    A_Stokes(V,Bs,p1,w,wk);
    Precon_Stokes(V[0],Bs[2],w,p1);

    printf("%lf %lf \n",dsum(nsolve,p1,1),dsum(nsolve,A[i],1));
  }
#endif
  /* get eigenvalues of invM*A */
  fprintf(stdout,"\n Calculating eigenvalues\n"); fflush(stdout);
  FullEigenMatrix(A,nsolve,nsolve,0);

  free_dmatrix(A,0,0);
  exit(1);
#endif
#endif

}
#endif


/*

Function name: Element::DivMat

Function Purpose:

Argument 1: LocMatDiv *div
Purpose:

Argument 2: Element *P
Purpose:

Function Notes:

*/

void  Tri::DivMat(LocMatDiv *div, Element *P){
  register int i,j,n;
  const    int nbl = Nbmodes, N = Nmodes - Nbmodes;
  int      L;
  Basis   *b = getbasis();
  double **dxb = div->Dxb,
         **dxi = div->Dxi,
         **dyb = div->Dyb,
         **dyi = div->Dyi;
  char orig_state = state;
  double  *ux,*uy;

  ux = dvector(0,2*QGmax*QGmax-1);
  uy = ux + QGmax*QGmax;

  /* fill boundary systems */
  for(i = 0,n=0; i < Nverts; ++i,++n){
    fillElmt(b->vert+i);
    Grad_d(ux,uy,0,'a');

    dcopy(qtot,ux,1,*P->h,1);
#ifndef PCONTBASE
    P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
    P->Iprod(P);
#endif
    dcopy(P->Nmodes,P->vert->hj,1,*dxb + n,nbl);

    dcopy(qtot,uy,1,*P->h,1);
#ifndef PCONTBASE
    P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
    P->Iprod(P);
#endif
    dcopy(P->Nmodes,P->vert->hj,1,*dyb + n,nbl);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < edge[i].l; ++j, ++n){
      fillElmt(b->edge[i]+j);
      Grad_d(ux,uy,0,'a');

      dcopy(qtot,ux,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dxb + n,nbl);

      dcopy(qtot,uy,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dyb + n,nbl);
    }

  L = face->l;
  for(i = 0,n=0; i < L;++i)
    for(j = 0; j < L-i; ++j,++n){
      fillElmt(b->face[0][i]+j);
      Grad_d(ux,uy,0,'a');

      dcopy(qtot,ux,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dxi + n,N);

      dcopy(qtot,uy,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dyi + n,N);
    }

  state = orig_state;

  /* negate all systems so that the whole operator can be treated
     as positive when condensing */
  dneg(nbl*P->Nmodes,*dxb,1);
  dneg(nbl*P->Nmodes,*dyb,1);
  dneg(N  *P->Nmodes,*dxi,1);
  dneg(N  *P->Nmodes,*dyi,1);

  free(ux);
}




void  Quad::DivMat(LocMatDiv *div, Element *P){
  register int i,j,n;
  const    int nbl = Nbmodes, N = Nmodes - Nbmodes;
  int      L;
  Basis   *b = getbasis();
  double **dxb = div->Dxb,
         **dxi = div->Dxi,
         **dyb = div->Dyb,
         **dyi = div->Dyi;
  char orig_state = state;
  double  *ux,*uy;

  ux = dvector(0,2*QGmax*QGmax-1);
  uy = ux + QGmax*QGmax;

  /* fill boundary systems */
  for(i = 0,n=0; i < Nverts; ++i,++n){
    fillElmt(b->vert+i);
    Grad_d(ux,uy,0,'a');

    dcopy(qtot,ux,1,*P->h,1);
#ifndef PCONTBASE
    P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
    P->Iprod(P);
#endif
    dcopy(P->Nmodes,P->vert->hj,1,*dxb + n,nbl);

    dcopy(qtot,uy,1,*P->h,1);
#ifndef PCONTBASE
    P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
    P->Iprod(P);
#endif
    dcopy(P->Nmodes,P->vert->hj,1,*dyb + n,nbl);
  }

  for(i = 0; i < Nedges; ++i)
    for(j = 0; j < edge[i].l; ++j, ++n){
      fillElmt(b->edge[i]+j);
      Grad_d(ux,uy,0,'a');

      dcopy(qtot,ux,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dxb + n,nbl);

      dcopy(qtot,uy,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dyb + n,nbl);
    }

  L = face->l;
  for(i = 0,n=0; i < L;++i)
    for(j = 0; j < L; ++j,++n){
      fillElmt(b->face[0][i]+j);
      Grad_d(ux,uy,0,'a');

      dcopy(qtot,ux,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dxi + n,N);

      dcopy(qtot,uy,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dyi + n,N);
    }

  state = orig_state;

  /* negate all systems to that the whole operator can be treated
     as positive when condensing */
  dneg(nbl*P->Nmodes,*dxb,1);
  dneg(nbl*P->Nmodes,*dyb,1);
  dneg(N  *P->Nmodes,*dxi,1);
  dneg(N  *P->Nmodes,*dyi,1);

  free(ux);
}

void Element::DivMat  (LocMatDiv *, Element *){ERR;}

/*

Function name: Element::project_stokes

Function Purpose:

Argument 1: Bsystem *Pbsys
Purpose:

Argument 2: int asize
Purpose:

Argument 3: int geom_id
Purpose:

Function Notes:

*/

void Tri::project_stokes(Bsystem *Pbsys, int asize, int geom_id){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int bwidth = Pbsys->Gmat->bwidth_a;
  double   *a = Pbsys->Gmat->a[geom_id], **inva = Pbsys->Gmat->inva;

  list = Pbsys->bmap[id];

  if(Pbsys->rslv){ /* project a to first recurrsion */
    Project_Recur(id, asize, a, list, 0, Pbsys);
    free(a);
  }
  else{           /* normal static condenstion */
    if(nsolve = Pbsys->nsolve){
      if(Pbsys->lambda->wave){

  if(3*bwidth < nsolve){
    /* store as packed banded system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
        inva[pj][2*bwidth-1-(pj-pi)] += a[i*asize+j];
  }
  else{
    /* store as full system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j  = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
        inva[0][pj*nsolve+pi]+=a[i*asize+j];/*pack in fortran form*/
  }
      }
      else{
  if(2*bwidth < nsolve){
    /* store as packed banded system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j  = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
      if(pj >= pi)
        inva[pi][pj-pi] += a[i*asize+j];
  }
  else{
    /* store as symmetric system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j  = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
      if(pj <= pi)
        inva[0][(pj*(pj+1))/2+(nsolve-pj)*pj+(pi-pj)]+=a[i*asize+j];
  }
      }
      free(a);
    }
  }
}




void Quad::project_stokes(Bsystem *Pbsys, int asize, int geom_id){
  register int i,j;
  int      pi,pj,nsolve;
  int      *list;
  const    int bwidth = Pbsys->Gmat->bwidth_a;
  double   *a = Pbsys->Gmat->a[geom_id], **inva = Pbsys->Gmat->inva;

  list = Pbsys->bmap[id];

  if(Pbsys->rslv){ /* project a to first recurrsion */
    Project_Recur(id, asize, a, list, 0, Pbsys);
    free(a);
  }
  else{           /* normal static condenstion */
    if(nsolve = Pbsys->nsolve){
      if(Pbsys->lambda->wave){

  if(3*bwidth < nsolve){
    /* store as packed banded system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
        inva[pj][2*bwidth-1-(pj-pi)] += a[i*asize+j];
  }
  else{
    /* store as full system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j  = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
        inva[0][pj*nsolve+pi]+=a[i*asize+j];/*pack in fortran form*/
  }
      }
      else{
  if(2*bwidth < nsolve){
    /* store as packed banded system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j  = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
      if(pj >= pi)
        inva[pi][pj-pi] += a[i*asize+j];
  }
  else{
    /* store as symmetric system */
    for(i = 0; i < asize; ++i)
      if((pi=list[i]) < nsolve)
        for(j  = 0; j < asize; ++j)
    if((pj=list[j]) < nsolve)
      if(pj <= pi)
        inva[0][(pj*(pj+1))/2+(nsolve-pj)*pj+(pi-pj)]+=a[i*asize+j];
  }
      }
      free(a);
    }
  }
}












void Element::project_stokes(Bsystem *, int, int){ERR;}




/*

Function name: Element::condense_stokes

Function Purpose:

Argument 1: LocMat *m
Purpose:

Argument 2: LocMatDiv *D
Purpose:

Argument 3: Bsystem *Ubsys
Purpose:

Function Notes:

*/

void Tri::condense_stokes(LocMat *m, LocMatDiv *D, Bsystem *Ubsys,
         Bsystem *Pbsys, Element *P){
  register int i,j,k;
  int      asize,csize,rows,L,info,n;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id, *ipiv;
  double  **a = m->a, **b = m->b, **c = m->c, **d = m->d;
  double  **dxb = D->Dxb, **dyb = D->Dyb, **dxi = D->Dxi, **dyi = D->Dyi;
  double  *invc, *binvc, *invcd, *dxinvc, *dyinvc;
  double  **did,*atot;

  /* calc local boundary matrix size */
  if((m->asize != D->bsize)||(m->csize != D->isize))
    fprintf(stderr,"problem in condense_stokes:sizes do not match\n");

  asize = m->asize;
  csize = m->csize;
  rows  = D->rows;

  if(csize){
    // not banded since "curved"
    // just store complete system at present
    bw[geom_id] = csize;

    if(Ubsys->lambda->wave){
      if(csize > 3*bw[geom_id])
  invc = dvector(0,csize*(3*bw[geom_id]-2));
      else
  invc = dvector(0,csize*csize-1);
      ipiv  = ivector(0,csize-1);
      invcd = dvector(0,asize*csize-1);

      for(i = 0; i < asize; ++i)
  dcopy(csize,d[i],1,invcd + i*csize,1);
   }
    else{
      if(csize > 2*bw[geom_id])
  invc = dvector(0,csize*bw[geom_id]-1);
      else
  invc = dvector(0,csize*(csize+1)/2-1);
    }

    binvc  = dvector(0,asize*csize-1);
    dxinvc = dvector(0, rows*csize-1);
    dyinvc = dvector(0, rows*csize-1);

    did = dmatrix(0,rows-1,0,rows-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    for(i = 0; i < rows; ++i){
      dcopy(csize,dxi[i],1,dxinvc + i*csize,1);
      dcopy(csize,dyi[i],1,dyinvc + i*csize,1);
    }

    if(Ubsys->lambda->wave){
      /* calc invc(c) */
      PackFullMatrix(c,csize,invc,bw[geom_id]);
      FacFullMatrix (invc,csize,ipiv,bw[geom_id]);

      /* calc inv(c)^T*b^T */
      if(csize > 3*bw[geom_id])
  dgbtrs('T',csize,bw[geom_id]-1,bw[geom_id]-1,asize,invc,
         3*bw[geom_id]-2,ipiv,binvc,csize,info);
      else
  dgetrs('T',csize,asize,invc,csize,ipiv,binvc,csize,info);

      /* calc inv(c)*d */
      if(csize > 3*bw[geom_id])
  dgbtrs('N',csize,bw[geom_id]-1,bw[geom_id]-1,asize,invc,
         3*bw[geom_id]-2,ipiv,invcd,csize,info);
      else
  dgetrs('N',csize,asize,invc,csize,ipiv,invcd,csize,info);


      /* calc inv(c)^T*dxi^T */
      if(csize > 3*bw[geom_id]){
  dgbtrs('T',csize,bw[geom_id]-1,bw[geom_id]-1,rows,invc,
         3*bw[geom_id]-2,ipiv,dxinvc,csize,info);
  dgbtrs('T',csize,bw[geom_id]-1,bw[geom_id]-1,rows,invc,
         3*bw[geom_id]-2,ipiv,dyinvc,csize,info);
      }
      else{
  dgetrs('T',csize,rows,invc,csize,ipiv,dxinvc,csize,info);
  dgetrs('T',csize,rows,invc,csize,ipiv,dyinvc,csize,info);
      }

    /* A - b inv(c) d */
      for(i = 0; i < asize; ++i)
  for(j = 0; j < asize; ++j)
    a[i][j] -= ddot(csize,binvc+i*csize,1,d[j],1);

      /*  - dxi inv(c) dxi^T - dyi inv(c) dyi^T */
      for(i = 0; i < rows; ++i)
  for(j = 0; j < rows; ++j){
    did[i][j]  = ddot(csize,dxinvc+i*csize,1,dxi[j],1);
    did[i][j] += ddot(csize,dyinvc+i*csize,1,dyi[j],1);
  }

      dneg(rows*rows,did[0],1);
    }
    else{
      /* calc invc(c) */
      PackMatrix(c,csize,invc,bw[geom_id]);
      FacMatrix(invc,csize,bw[geom_id]);

      /* calc inv(c)*b^T */
      if(csize > 2*bw[geom_id])
  dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],
         binvc,csize,info);
      else
  dpptrs('L',csize,asize,invc,binvc,csize,info);

      /* calc inv(c)*dxi^T */
      if(csize > 2*bw[geom_id]){
  dpbtrs('L',csize,bw[geom_id]-1,rows,invc,bw[geom_id],dxinvc,
         csize,info);
  dpbtrs('L',csize,bw[geom_id]-1,rows,invc,bw[geom_id],dyinvc,
         csize,info);
      }
      else{
  dpptrs('L',csize,rows,invc,dxinvc,csize,info);
  dpptrs('L',csize,rows,invc,dyinvc,csize,info);
      }

#ifndef INFSUP
      /* A - b inv(c) b^T */
      for(i = 0; i < asize; ++i)
  for(j = 0; j < asize; ++j)
    a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);

#endif

      /*  - dxi inv(c) dxi^T - dyi inv(c) dyi^T */
      for(i = 0; i < rows; ++i)
  for(j = 0; j < rows; ++j){
    did[i][j]  = ddot(csize,dxi[i],1,dxinvc+j*csize,1);
    did[i][j] += ddot(csize,dyi[i],1,dyinvc+j*csize,1);
  }

      dneg(rows*rows,did[0],1);
    }
  }

  if(csize)
    if(Ubsys->Gmat->invc[geom_id]){
      free(invc);
      free(binvc);
      free(dxinvc);
      free(dyinvc);
      if(Ubsys->lambda->wave){
  free(invcd);
  free(ipiv);
      }
    }
    else{
      Ubsys->Gmat-> invc    [geom_id] = invc;
      Ubsys->Gmat->binvc    [geom_id] = binvc;
      Ubsys->Gmat->dbinvc[0][geom_id] = dxinvc;
      Ubsys->Gmat->dbinvc[1][geom_id] = dyinvc;
      if(Ubsys->lambda->wave){
  Ubsys->Gmat->cipiv  [geom_id] = ipiv;
  Ubsys->Gmat->invcd  [geom_id] = invcd;
      }
    }
  else /* just set it to non-zero value */
    Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);

  /* set up pressure schur decomposition */

  csize = rows - 1;
  asize = 2*asize + 1;

  if(Ubsys->lambda->wave){
    invc = dvector(0,csize*csize-1);
    ipiv = ivector(0,csize-1);
  }
  else
    invc  = dvector(0,csize*(csize+1)/2-1);

  binvc = dvector(0,asize*csize-1);
  invcd = dvector(0,asize*csize-1);
  atot  = dvector(0,asize*asize-1);

  dzero(asize*asize,atot,1);

  /* fill new b system */
  /* Dxb^T - B C^{-1} Dxi^T */
  for(i = 0,n=0; i < D->bsize; ++i)
    for(j = 0; j < csize; ++j)
      binvc[n++] = dxb[j+1][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dxi[j+1],1);

  for(i = 0; i < D->bsize; ++i)
    for(j = 0; j < csize; ++j)
      binvc[n++] = dyb[j+1][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dyi[j+1],1);

  for(j = 0; j < csize; ++j)
    binvc[n++] = did[0][j+1];

  /* fill new d system */
  if(Ubsys->lambda->wave){
    /* Dxb - Dxi C^{-1} D */
    for(i = 0,n=0; i < D->bsize; ++i)
      for(j = 0; j < csize; ++j)
  invcd[n++] = dxb[j+1][i] -
    ddot(m->csize,dxi[j+1],1,Ubsys->Gmat->invcd[geom_id]+i*m->csize,1);

    for(i = 0; i < D->bsize; ++i)
      for(j = 0; j < csize; ++j)
  invcd[n++] = dyb[j+1][i] -
    ddot(m->csize,dyi[j+1],1,Ubsys->Gmat->invcd[geom_id]+i*m->csize,1);

    for(j = 0; j < csize; ++j)
      invcd[n++] = did[j+1][0];
  }

  if(Ubsys->lambda->wave){
    /* fill atot system */
    for(i = 0; i < D->bsize; ++i){
      dcopy(D->bsize,a[i],1,atot + i*asize,1);

      atot[(i+1)*asize-1] = dxb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dxi[0],1);
      atot[(asize-1)*asize+i] =  dxb[0][i] -
  ddot(m->csize,dxi[0],1,Ubsys->Gmat->invcd[geom_id] + i*m->csize,1);

      dcopy(D->bsize,a[i],1,atot + (D->bsize+i)*asize + D->bsize,1);

      atot[(D->bsize+i+1)*asize-1] = dyb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dyi[0],1);
      atot[(asize-1)*asize+D->bsize+i] =  dyb[0][i] -
  ddot(m->csize,dyi[0],1,Ubsys->Gmat->invcd[geom_id] + i*m->csize,1);
    }
  }
  else{
    /* fill atot system */
    for(i = 0; i < D->bsize; ++i){
      dcopy(D->bsize,a[i],1,atot + i*asize,1);
      atot[(i+1)*asize-1] = atot[(asize-1)*asize+i] =  dxb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize, 1,dxi[0],1);

      dcopy(D->bsize,a[i],1,atot + (D->bsize+i)*asize + D->bsize,1);
      atot[(D->bsize+i+1)*asize-1]=atot[(asize-1)*asize+D->bsize+i]=dyb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize, 1,dyi[0],1);
    }
  }

  atot[asize*asize-1] = did[0][0];

#ifndef INFSUP
  if(Ubsys->lambda->wave){
    /* replace dxinvc dyinvc with dxi, dyi for Oseen formulation */
    for(i = 0; i < rows; ++i){
      dcopy(m->csize,dxi[i],1,dxinvc + i*m->csize,1);
      dcopy(m->csize,dyi[i],1,dyinvc + i*m->csize,1);
    }

    ipiv = ivector(0,csize-1);

    /* pack did matrix leaving first point as part of interior */
    /* note have to assemble positive version for a positive system */
    /* but will have to negate all inverses */
    for(i = 0; i < csize; ++i)
      dcopy(csize, did[1]+i+1, rows, invc+i*csize, 1);

    /* calc invc(c) */
    FacFullMatrix(invc,csize,ipiv,csize);

    /* calc inv(c)^T*b^T */
    dgetrs('T',csize,asize,invc,csize,ipiv,binvc,csize,info);

    /* A - b inv(c) d */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  atot[i*asize+j] -= ddot(csize,binvc+i*csize,1,invcd+j*csize,1);

    /* calc inv(c)*d */
    dgetrs('N',csize,asize,invc,csize,ipiv,invcd,csize,info);

  }
  else{
    /* pack did matrix leaving first point as part of interior */
     /* note have to assemble positive version for a positive system */
    /* but will have to negate all inverses */
    for(i = 0,j=0; i < csize; j+=csize-i++)
      dcopy(csize-i, did[i+1]+i+1, 1, invc+j, 1);
    dneg(csize*(csize+1)/2,invc,1);

    /* calc invc(c) */
    FacMatrix(invc,csize,csize);

    dcopy(asize*csize,binvc,1,invcd,1);

    /* calc inv(c)*b^T */
    dpptrs('L',csize,asize,invc,binvc,csize,info);
    /* negate result since invc = -invc */
    dneg(asize*csize,binvc,1);

    /* A - b inv(c) d */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
      atot[i*asize+j] -= ddot(csize,binvc+j*csize,1,invcd+i*csize,1);
  }
#endif

  /* sort sign connectivity issues */
  if(Ubsys->smeth == direct||Ubsys->rslv){
    n = Nverts;
    for(i = 0; i < Nedges; ++i){
      L = edge[i].l;
      if(edge[i].con){
  for(j = 1; j < L; j+=2){
    dscal(asize,-1.0,atot+(n+j)*asize,1);
    for(k = 0; k < asize; ++k)
      atot[k*asize + n+j] *= -1.0;

    dscal(asize,-1.0,atot+(n+j+D->bsize)*asize,1);
    for(k = 0; k < asize; ++k)
      atot[k*asize + n+j+D->bsize] *= -1.0;
  }
      }
      n += L;
    }
  }

  free_dmatrix(did,0,0);

  if(Pbsys->Gmat->invc[geom_id]){
    free(invc);
    free(binvc);
    free(invcd);
    if(Ubsys->lambda->wave){
      free(ipiv);
    }
  }
  else{
    Pbsys->Gmat->invc [geom_id] = invc;
    Pbsys->Gmat->binvc[geom_id] = binvc;
    if(Ubsys->lambda->wave){
      Pbsys->Gmat->cipiv[geom_id] = ipiv;
      Pbsys->Gmat->invcd[geom_id] = invcd;
    }
    else{
      free(invcd);
    }
  }

  /* this option is just here to deal with direct case with sign
     changes -- should really tidy it up and put sign changes
     outside these routines */
  if(Ubsys->smeth == direct||Ubsys->rslv)
    Pbsys->Gmat->a[geom_id] = atot;
  else{
    if(Ubsys->lambda->wave){
      Pbsys->Gmat->a[geom_id] = atot;
  }
    else{
      Pbsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);

      /* for this system store as in upper triangular format
   since we want to use the velocity and pressure subsystems */
      for(i=0, j=0; i < asize; ++i, j+=i)
  dcopy(i+1, atot+i, asize, Pbsys->Gmat->a[geom_id]+j, 1);

      free(atot);
    }
  }
}




void Quad::condense_stokes(LocMat *m, LocMatDiv *D, Bsystem *Ubsys,
         Bsystem *Pbsys, Element *P){
  register int i,j,k;
  int      asize,csize,rows,L,info,n;
  int     *bw = Ubsys->Gmat->bwidth_c, geom_id = geom->id, *ipiv;
  double  **a = m->a, **b = m->b, **c = m->c, **d = m->d;
  double  **dxb = D->Dxb, **dyb = D->Dyb, **dxi = D->Dxi, **dyi = D->Dyi;
  double  *invc, *binvc, *invcd, *dxinvc, *dyinvc;
  double  **did,*atot;

  /* calc local boundary matrix size */
  if((m->asize != D->bsize)||(m->csize != D->isize))
    fprintf(stderr,"problem in condense_stokes:sizes do not match\n");

  asize = m->asize;
  csize = m->csize;
  rows  = D->rows;

  if(csize){
    // not banded since "curved"
    // just store complete system at present
    bw[geom_id] = csize;

    if(Ubsys->lambda->wave){
      if(csize > 3*bw[geom_id])
  invc = dvector(0,csize*(3*bw[geom_id]-2));
      else
  invc = dvector(0,csize*csize-1);
      ipiv  = ivector(0,csize-1);
      invcd = dvector(0,asize*csize-1);

      for(i = 0; i < asize; ++i)
  dcopy(csize,d[i],1,invcd + i*csize,1);
   }
    else{
      if(csize > 2*bw[geom_id])
  invc = dvector(0,csize*bw[geom_id]-1);
      else
  invc = dvector(0,csize*(csize+1)/2-1);
    }

    binvc  = dvector(0,asize*csize-1);
    dxinvc = dvector(0,rows*csize-1);
    dyinvc = dvector(0,rows*csize-1);

    did = dmatrix(0,rows-1,0,rows-1);

    for(i = 0; i < asize; ++i)
      dcopy(csize,b[i],1,binvc + i*csize,1);

    for(i = 0; i < rows; ++i){
      dcopy(csize,dxi[i],1,dxinvc + i*csize,1);
      dcopy(csize,dyi[i],1,dyinvc + i*csize,1);
    }

    if(Ubsys->lambda->wave){
      /* calc invc(c) */
      PackFullMatrix(c,csize,invc,bw[geom_id]);
      FacFullMatrix (invc,csize,ipiv,bw[geom_id]);

      /* calc inv(c)^T*b^T */
      if(csize > 3*bw[geom_id])
  dgbtrs('T',csize,bw[geom_id]-1,bw[geom_id]-1,asize,invc,
         3*bw[geom_id]-2,ipiv,binvc,csize,info);
      else
  dgetrs('T',csize,asize,invc,csize,ipiv,binvc,csize,info);

      /* calc inv(c)*d */
      if(csize > 3*bw[geom_id])
  dgbtrs('N',csize,bw[geom_id]-1,bw[geom_id]-1,asize,invc,
         3*bw[geom_id]-2,ipiv,invcd,csize,info);
      else
  dgetrs('N',csize,asize,invc,csize,ipiv,invcd,csize,info);


      /* calc inv(c)^T*dxi^T */
      if(csize > 3*bw[geom_id]){
  dgbtrs('T',csize,bw[geom_id]-1,bw[geom_id]-1,rows,invc,
         3*bw[geom_id]-2,ipiv,dxinvc,csize,info);
  dgbtrs('T',csize,bw[geom_id]-1,bw[geom_id]-1,rows,invc,
         3*bw[geom_id]-2,ipiv,dyinvc,csize,info);
      }
      else{
  dgetrs('T',csize,rows,invc,csize,ipiv,dxinvc,csize,info);
  dgetrs('T',csize,rows,invc,csize,ipiv,dyinvc,csize,info);
      }

    /* A - b inv(c) d */
      for(i = 0; i < asize; ++i)
  for(j = 0; j < asize; ++j)
    a[i][j] -= ddot(csize,binvc+i*csize,1,d[j],1);

      /*  - dxi inv(c) dxi^T - dyi inv(c) dyi^T */
      for(i = 0; i < rows; ++i)
  for(j = 0; j < rows; ++j){
    did[i][j]  = ddot(csize,dxinvc+i*csize,1,dxi[j],1);
    did[i][j] += ddot(csize,dyinvc+i*csize,1,dyi[j],1);
  }

      dneg(rows*rows,did[0],1);
    }
    else{
      /* calc invc(c) */
      PackMatrix(c,csize,invc,bw[geom_id]);
      FacMatrix(invc,csize,bw[geom_id]);

      /* calc inv(c)*b^T */
      if(csize > 2*bw[geom_id])
  dpbtrs('L',csize,bw[geom_id]-1,asize,invc,bw[geom_id],
         binvc,csize,info);
      else
  dpptrs('L',csize,asize,invc,binvc,csize,info);

      /* calc inv(c)*dxi^T */
      if(csize > 2*bw[geom_id]){
  dpbtrs('L',csize,bw[geom_id]-1,rows,invc,bw[geom_id],dxinvc,
         csize,info);
  dpbtrs('L',csize,bw[geom_id]-1,rows,invc,bw[geom_id],dyinvc,
         csize,info);
      }
      else{
  dpptrs('L',csize,rows,invc,dxinvc,csize,info);
  dpptrs('L',csize,rows,invc,dyinvc,csize,info);
      }

#ifndef INFSUP
    /* A - b inv(c) b^T */
      for(i = 0; i < asize; ++i)
  for(j = 0; j < asize; ++j)
    a[i][j] -= ddot(csize,b[i],1,binvc+j*csize,1);

#endif

      /*  - dxi inv(c) dxi^T - dyi inv(c) dyi^T */
      for(i = 0; i < rows; ++i)
  for(j = 0; j < rows; ++j){
    did[i][j]  = ddot(csize,dxi[i],1,dxinvc+j*csize,1);
    did[i][j] += ddot(csize,dyi[i],1,dyinvc+j*csize,1);
  }

      dneg(rows*rows,did[0],1);
    }
  }

  if(csize)
    if(Ubsys->Gmat->invc[geom_id]){
      free(invc);
      free(binvc);
      free(dxinvc);
      free(dyinvc);
      if(Ubsys->lambda->wave){
  free(invcd);
  free(ipiv);
      }
    }
    else{
      Ubsys->Gmat-> invc    [geom_id] = invc;
      Ubsys->Gmat->binvc    [geom_id] = binvc;
      Ubsys->Gmat->dbinvc[0][geom_id] = dxinvc;
      Ubsys->Gmat->dbinvc[1][geom_id] = dyinvc;
      if(Ubsys->lambda->wave){
  Ubsys->Gmat->cipiv  [geom_id] = ipiv;
  Ubsys->Gmat->invcd  [geom_id] = invcd;
      }
    }
  else /* just set it to non-zero value */
    Ubsys->Gmat->invc[geom_id] = (double *)malloc(1);

  /* set up pressure schur decomposition */

  csize = rows - 1;
  asize = 2*asize + 1;

  if(Ubsys->lambda->wave){
    invc = dvector(0,csize*csize-1);
    ipiv = ivector(0,csize-1);
  }
  else
    invc  = dvector(0,csize*(csize+1)/2-1);

  binvc = dvector(0,asize*csize-1);
  invcd = dvector(0,asize*csize-1);
  atot  = dvector(0,asize*asize-1);

  dzero(asize*asize,atot,1);

  /* fill new b system */
  /* Dxb^T - B C^{-1} Dxi^T */
  for(i = 0,n=0; i < D->bsize; ++i)
    for(j = 0; j < csize; ++j)
      binvc[n++] = dxb[j+1][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dxi[j+1],1);

  for(i = 0; i < D->bsize; ++i)
    for(j = 0; j < csize; ++j)
      binvc[n++] = dyb[j+1][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dyi[j+1],1);

  for(j = 0; j < csize; ++j)
    binvc[n++] = did[0][j+1];

  /* fill new d system */
  if(Ubsys->lambda->wave){
    /* Dxb - Dxi C^{-1} D */
    for(i = 0,n=0; i < D->bsize; ++i)
      for(j = 0; j < csize; ++j)
  invcd[n++] = dxb[j+1][i] -
    ddot(m->csize,dxi[j+1],1,Ubsys->Gmat->invcd[geom_id]+i*m->csize,1);

    for(i = 0; i < D->bsize; ++i)
      for(j = 0; j < csize; ++j)
  invcd[n++] = dyb[j+1][i] -
    ddot(m->csize,dyi[j+1],1,Ubsys->Gmat->invcd[geom_id]+i*m->csize,1);

    for(j = 0; j < csize; ++j)
      invcd[n++] = did[j+1][0];
  }

  if(Ubsys->lambda->wave){
    /* fill atot system */
    for(i = 0; i < D->bsize; ++i){
      dcopy(D->bsize,a[i],1,atot + i*asize,1);

      atot[(i+1)*asize-1] = dxb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dxi[0],1);
      atot[(asize-1)*asize+i] =  dxb[0][i] -
  ddot(m->csize,dxi[0],1,Ubsys->Gmat->invcd[geom_id] + i*m->csize,1);

      dcopy(D->bsize,a[i],1,atot + (D->bsize+i)*asize + D->bsize,1);

      atot[(D->bsize+i+1)*asize-1] = dyb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize,1,dyi[0],1);
      atot[(asize-1)*asize+D->bsize+i] =  dyb[0][i] -
  ddot(m->csize,dyi[0],1,Ubsys->Gmat->invcd[geom_id] + i*m->csize,1);
    }
  }
  else{
    /* fill atot system */
    for(i = 0; i < D->bsize; ++i){
      dcopy(D->bsize,a[i],1,atot + i*asize,1);
      atot[(i+1)*asize-1] = atot[(asize-1)*asize+i] =  dxb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize, 1,dxi[0],1);

      dcopy(D->bsize,a[i],1,atot + (D->bsize+i)*asize + D->bsize,1);
      atot[(D->bsize+i+1)*asize-1]=atot[(asize-1)*asize+D->bsize+i]=dyb[0][i] -
  ddot(m->csize,Ubsys->Gmat->binvc[geom_id]+i*m->csize, 1,dyi[0],1);
    }
  }

  atot[asize*asize-1] = did[0][0];

#ifndef INFSUP
  if(Ubsys->lambda->wave){
    /* replace dxinvc dyinvc with dxi, dyi for Oseen formulation */
    for(i = 0; i < rows; ++i){
      dcopy(m->csize,dxi[i],1,dxinvc + i*m->csize,1);
      dcopy(m->csize,dyi[i],1,dyinvc + i*m->csize,1);
    }

    ipiv = ivector(0,csize-1);

    /* pack did matrix leaving first point as part of interior */
    /* note have to assemble positive version for a positive system */
    /* but will have to negate all inverses */
    for(i = 0; i < csize; ++i)
      dcopy(csize, did[1]+i+1, rows, invc+i*csize, 1);

    /* calc invc(c) */
    FacFullMatrix(invc,csize,ipiv,csize);

    /* calc inv(c)^T*b^T */
    dgetrs('T',csize,asize,invc,csize,ipiv,binvc,csize,info);

    /* A - b inv(c) d */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
  atot[i*asize+j] -= ddot(csize,binvc+i*csize,1,invcd+j*csize,1);

    /* calc inv(c)*d */
    dgetrs('N',csize,asize,invc,csize,ipiv,invcd,csize,info);

  }
  else{
    /* pack did matrix leaving first point as part of interior */
     /* note have to assemble positive version for a positive system */
    /* but will have to negate all inverses */
    for(i = 0,j=0; i < csize; j+=csize-i++)
      dcopy(csize-i, did[i+1]+i+1, 1, invc+j, 1);
    dneg(csize*(csize+1)/2,invc,1);

    /* calc invc(c) */
    FacMatrix(invc,csize,csize);

    dcopy(asize*csize,binvc,1,invcd,1);

    /* calc inv(c)*b^T */
    dpptrs('L',csize,asize,invc,binvc,csize,info);
    /* negate result since invc = -invc */
    dneg(asize*csize,binvc,1);

    /* A - b inv(c) d */
    for(i = 0; i < asize; ++i)
      for(j = 0; j < asize; ++j)
      atot[i*asize+j] -= ddot(csize,binvc+j*csize,1,invcd+i*csize,1);
  }
#endif

  /* sort sign connectivity issues */
  if(Ubsys->smeth == direct||Ubsys->rslv){
    n = Nverts;
    for(i = 0; i < Nedges; ++i){
      L = edge[i].l;
      if(edge[i].con){
  for(j = 1; j < L; j+=2){
    dscal(asize,-1.0,atot+(n+j)*asize,1);
    for(k = 0; k < asize; ++k)
      atot[k*asize + n+j] *= -1.0;

    dscal(asize,-1.0,atot+(n+j+D->bsize)*asize,1);
    for(k = 0; k < asize; ++k)
      atot[k*asize + n+j+D->bsize] *= -1.0;
  }
      }
      n += L;
    }
  }

  free_dmatrix(did,0,0);

  if(Pbsys->Gmat->invc[geom_id]){
    free(invc);
    free(binvc);
    free(invcd);
    if(Ubsys->lambda->wave){
      free(ipiv);
    }
  }
  else{
    Pbsys->Gmat->invc [geom_id] = invc;
    Pbsys->Gmat->binvc[geom_id] = binvc;
    if(Ubsys->lambda->wave){
      Pbsys->Gmat->cipiv[geom_id] = ipiv;
      Pbsys->Gmat->invcd[geom_id] = invcd;
    }
    else{
      free(invcd);
    }
  }

  /* this option is just here to deal with direct case with sign
     changes -- should really tidy it up and put sign changes
     outside these routines */
  if(Ubsys->smeth == direct||Ubsys->rslv)
    Pbsys->Gmat->a[geom_id] = atot;
  else{
    if(Ubsys->lambda->wave){
      Pbsys->Gmat->a[geom_id] = atot;
  }
    else{
      Pbsys->Gmat->a[geom_id] = dvector(0,asize*(asize+1)/2-1);

      /* for this system store as in upper triangular format
   since we want to use the velocity and pressure subsystems */
      for(i=0, j=0; i < asize; ++i, j+=i)
  dcopy(i+1, atot+i, asize, Pbsys->Gmat->a[geom_id]+j, 1);

      free(atot);
    }
  }
}

void Element::condense_stokes(LocMat *, LocMatDiv *,Bsystem *, Bsystem *,
            Element *){ERR;}
