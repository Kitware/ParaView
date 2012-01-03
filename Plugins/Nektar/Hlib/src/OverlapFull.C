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
void setup_signchangeOverlap(Element_List *U, Bsystem *B);

void GenPatchesFull(Element_List *U, Element_List *Uf, Bsystem *B);
void PreconFullOverlap(Bsystem *B, double *pin, double *zout);

void ReadPatchesFull(char *fname, Element_List *U, Element_List *Uf,
           Bsystem *B){

  Element *E,*F;
  int  i,j,k,l,N;
  Vert *vb;
  int skip = 0;

  setup_signchange(U,B);

  int cnt=0;

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

      //  DONE
      // FILE PARTITION
      for(k=0;k<U->nel;++k){
  fgets(buf, BUFSIZ, fin);
  sscanf(buf, "%d" , partition+k);
      }

      ++OL->npatches;

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

      OL->solveflags = dmatrix(0, OL->npatches-1, 0, OL->Ntotal-1);
      dfill(OL->npatches*OL->Ntotal, 1.0, OL->solveflags[0], 1);
      for(i=0;i<OL->npatches-1;++i){
  cnt = B->nglobal;
  for(k=0;k<U->nel;++k){
    if(OL->maskflags[i][k] == 0){
      for(j=0;j<U->flist[k]->Nbmodes;++j)
        OL->solveflags[i][B->bmap[k][j]] = 0.;
      dzero(U->flist[k]->Nmodes-U->flist[k]->Nbmodes,
      OL->solveflags[i]+cnt, 1);
    }
    cnt += U->flist[k]->Nmodes-U->flist[k]->Nbmodes;
  }
  dzero(B->nglobal-B->nsolve, OL->solveflags[i]+B->nsolve, 1);
      }

      dzero(OL->Ntotal, OL->solveflags[i], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    OL->solveflags[i][B->bmap[k][j]] = 1.;
      dzero(B->nglobal-B->nsolve, OL->solveflags[i]+B->nsolve, 1);

      free(partition);
      fclose(fin);
      break;
    }
  case VertexPatch:
    {
      // DONE
      // PATCH AROUND VERTEX + FILL INS + VERTEX PRECONDITIONER

      int extrapatches = 0, id;

      OL->npatches = B->nv_solve +  1;
      if(U->fhead->dim() == 3)
  OL->npatches += B->ne_solve;

      OL->maskflags = imatrix(0, OL->npatches-1, 0, U->nel-1);
      izero(OL->npatches*U->nel, OL->maskflags[0], 1);

#if 0
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

      if(U->fhead->dim() == 3)
  exit(-1);
      else
  for(k=0;k<U->nel;++k){
    E = U->flist[k];

    if(E->dim() == 3){
      for(j=0;j<E->Nfaces;++j)
        if(!E->face[j].link)
    ++cnt;
      if(cnt == E->Nfaces-1)
        for(j=0;j<E->Nfaces;++j){

    if(E->face[j].link){
      id = E->face[j].link->eid;
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
    else{
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
#endif
      if(U->fhead->dim() == 2){

  OL->npatches = B->nv_solve +  1;

  OL->maskflags = imatrix(0, OL->npatches-1, 0, U->nel-1);
  izero(OL->npatches*U->nel, OL->maskflags[0], 1);

  // setup vertex
  cnt = 0;
  for(i=0;i<B->nsolve;++i){
    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      for(j=0;j<E->Nverts;++j)
        if(E->vert[j].gid == i){
    OL->maskflags[i][E->id] = 1;
        }
    }
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

      OL->solveflags = dmatrix(0, OL->npatches-1, 0, OL->Ntotal-1);

      // find dofs in this patch
      dfill(OL->npatches*OL->Ntotal, 1.0, OL->solveflags[0], 1);
      for(i=0;i<OL->npatches-1;++i){
  // do boundary flags
  for(k=0;k<U->nel;++k)
    if(OL->maskflags[i][k] == 0){
      for(j=0;j<U->flist[k]->Nbmodes;++j)
        if(B->bmap[k][j] < B->nsolve)
    OL->solveflags[i][B->bmap[k][j]] = 0.;
    }
  // zero known dof flags
  dzero(B->nglobal-B->nsolve, OL->solveflags[i]+B->nsolve, 1);
  cnt = B->nglobal;
  // zero interior dof not in this patch
  for(k=0;k<U->nel;++k){
    N = U->flist[k]->Nmodes-U->flist[k]->Nbmodes;
    if(OL->maskflags[i][k] == 0)
      dzero(N,OL->solveflags[i]+cnt, 1);
    cnt += N;
  }
      }

      // vertex space
      dzero(OL->Ntotal, OL->solveflags[i], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    if(B->bmap[k][j] < B->nsolve )
      OL->solveflags[i][B->bmap[k][j]] = 1.;
      break;
    }
  case ElementPatch:
    {
      // DONE
      // ELEMENT PATCHES + GLOBAL VERTEX PRECON

      int extrapatches = 0, id;

      OL->npatches = U->nel + 1;

      OL->solveflags = dmatrix(0, OL->npatches-1, 0, OL->Ntotal-1);

      dfill(OL->npatches*OL->Ntotal, 0.0, OL->solveflags[0], 1);

      cnt = B->nglobal;
      for(k=0;k<U->nel;++k){
  E = U->flist[k];
  for(j=0;j<E->Nbmodes;++j)
    if(B->bmap[k][j] < B->nsolve)
      OL->solveflags[k][B->bmap[k][j]] = 1.;

  dfill(E->Nmodes-E->Nbmodes, 1., OL->solveflags[k]+cnt, 1);
  cnt += E->Nmodes-E->Nbmodes;
      }

      dzero(OL->Ntotal, OL->solveflags[U->nel], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    if(B->bmap[k][j] < B->nsolve )
      OL->solveflags[U->nel][B->bmap[k][j]] = 1.;
      break;
    }
#if 0
  case Coarse:
    {
      // GLOBAL VERTEX PRECON

      int extrapatches = 0, id;

      OL->npatches = 1;

      OL->solveflags = dmatrix(0, OL->npatches-1, 0, B->nsolve-1);

      dzero(B->nsolve, OL->solveflags[0], 1);
      for(k=0;k<U->nel;++k)
  for(j=0;j<U->flist[k]->Nverts;++j)
    if(B->bmap[k][j] < B->nsolve )
      OL->solveflags[0][B->bmap[k][j]] = 1.;
      break;
    }
#endif
  }

  fprintf(stderr, "Overlap->Npatches = %d\n", OL->npatches);

  double **save = dmatrix(0, 3, 0, max(U->htot,U->hjtot)-1);
  char ustate = U->fhead->state;
  char ufstate = Uf->fhead->state;
  dcopy(U->htot, U->base_h,  1, save[0], 1);
  dcopy(U->htot, Uf->base_h,  1, save[1], 1);
  dcopy(U->hjtot, U->base_hj, 1, save[2], 1);
  dcopy(U->hjtot, Uf->base_hj, 1, save[3], 1);

  GenPatchesFull(U, Uf, B);
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


static void gather_modes(Element_List *U, double *u0, Bsystem *B){
  register int  i;
  Element  *E = U->fhead;
  double  *sc = B->signchange;
  Overlap *OL = B->overlap;

  dzero (OL->Ntotal, u0, 1);
  for(;E;E = E->next){
    for(i = 0; i < E->Nbmodes; ++i)
      u0[B->bmap[E->id][i]] += sc[i]*E->vert[0].hj[i];
    sc += E->Nbmodes;
  }

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
  Overlap *OL = B->overlap;

  dzero(U->hjtot, U->base_hj, 1);

  for(;E;E = E->next){
    dgathr(E->Nbmodes, p, B->bmap[E->id], E->vert[0].hj);
    dvmul (E->Nbmodes, sc, 1, E->vert[0].hj, 1, E->vert[0].hj, 1);
    sc += E->Nbmodes;

    dcopy(E->Nmodes-E->Nbmodes, p+cnt, 1, E->vert[0].hj+E->Nbmodes, 1);
    cnt += E->Nmodes-E->Nbmodes;
  }
}

void A_Full_Column(Element_List *U, Element_List *Uf, Bsystem *B,
       int colid, double *col, SolveType Stype){
  Overlap *OL = B->overlap;
  double d;

  dzero(OL->Ntotal, col, 1);
  col[colid] = 1.;

  scatter_modes(U, col, B);

  Element *E, *F;

  for(E=U->fhead;E;E=E->next){
    d = E->vert[0].hj[idamax(E->Nmodes, E->vert[0].hj, 1)];
    if(d){
      E->Trans(E,J_to_Q);
      if(Stype == Helm)
  E->HelmHoltz   (B->lambda+E->id);
      else
  E->Iprod(E);
    }
    else{
      E->type = 't';
      dzero(E->Nmodes, E->vert[0].hj, 1);
    }
  }

  gather_modes(U, col, B);
  dzero(B->nglobal-B->nsolve, col+B->nsolve, 1);
}

void GenPatchesFull(Element_List *U, Element_List *Uf, Bsystem *B){
  int i,j,k,L;
  int cnt = 0;
  int skip;
  int maxlen = 0;
  Overlap *OL = B->overlap;

  OL->patchmaps    = (int **) calloc(OL->npatches, sizeof(int*));
  OL->patchlengths = ivector(0, OL->npatches-1);

  for(i=0;i<OL->npatches;++i){
    cnt = 0;
    for(j=0;j<OL->Ntotal;++j)
      cnt += (int) OL->solveflags[i][j];

    OL->patchlengths[i] = cnt;
    OL->patchmaps[i] = ivector(0, cnt-1);

    maxlen = max(maxlen, cnt);
    skip = 0;
    for(j=0;j<OL->Ntotal;++j)
      if(OL->solveflags[i][j]){
  OL->patchmaps[i][skip] = j;
  ++skip;
      }
  }

  fprintf(stderr, "Max local A dimension=%d\n", maxlen);

  int *pskip  = ivector(0, OL->npatches-1);
  izero(OL->npatches, pskip, 1);

  double ***A = 0;

  A = (double***) calloc(OL->npatches, sizeof(double**));
  for(j=0;j<OL->npatches;++j)
    if(OL->patchlengths[j])
      A[j] = dmatrix(0, OL->patchlengths[j]-1, 0, OL->patchlengths[j]-1);

  double *col = dvector(0, OL->Ntotal-1);
  for(i=0;i<OL->Ntotal;++i){
    if(!(i % 10))
      fprintf(stdout, ".");
    A_Full_Column(U,Uf,B,i,col,Helm);
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
      free_dmatrix(A[j], 0, 0);
    }
  }


  fprintf(stderr, "Done local A factorizations\n");

}

void PreconFullOverlap(Bsystem *B, double *pin, double *zout){
  Overlap  *OL = B->overlap;
  double *tmp = dvector(0, OL->Ntotal-1);
  int i, j, k, L, *map, info;
  double *x = dvector(0, OL->Ntotal-1);
  dzero(OL->Ntotal, zout, 1);


  for(i=0;i<OL->npatches;++i){
    L   = OL->patchlengths[i];

    if(L){
      map = OL->patchmaps[i];
      for(j=0;j<L;++j)
  tmp[j] = pin[map[j]];

      if(L>2*OL->patchbw[i])
  dpbtrs('L',L,OL->patchbw[i]-1,1,OL->patchinvA[i],
         OL->patchbw[i],tmp,L,info);
      else
  dpptrs('L', L, 1, OL->patchinvA[i], tmp, L, info);

      for(j=0;j<L;++j)
  zout[map[j]] += tmp[j];
    }
  }

  free(x);
  free(tmp);
}
