/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/mlevel.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include "nektar.h"
#include <stdio.h>
#include <string.h>

typedef struct gidinfo {
    int nvert;
    int *vgid;
    int nedge;
    int *egid;
    int nface;
    int *fgid;

} GidInfo;



typedef struct patch {
  int npatch;
  int nel;
  int *xadj;
  int *adjncy;
  int *xelmt;
  int *elmtid;
  int *old2new;
  struct patch *next;
} Patch;


typedef struct mlevinfo {
  int npatch;
  GidInfo *Int;
  GidInfo *Bdy;
  int *old2new;
} Mlevelinfo;


typedef struct vtoe {
  int eid;
  int nvert;
  struct vtoe *next;
} VtoE;

typedef struct v2emap{
  int nvs;
  VtoE *v2e;
} V2emap;

V2emap V2Estore;

static Mlevelinfo *Mlevel_decom(Element_List *E, Bsystem *B, int *nlevel,
        int recur);
static Rsolver *setup_numbering(Mlevelinfo *ml, int nlevel,
        Element_List *E, Bsystem *B);
static void free_Mlevel(Mlevelinfo *m, int nlevel);
#ifdef MLEV_MEMORY
static void memory(Element_List *E, Bsystem *Ubsys);
#endif
static void Set_V2E(Element_List *E, int nv_solve);
static void Get_V2E(VtoE **v2e, int *nvs);
static void free_V2E(void);
#ifdef PRINT_MLEVEL
static void Print_Patch(Element_List *E, Patch *p, FILE *out);
#endif
int eDIM;
void Mlevel_SC_decom(Element_List *E, Bsystem *B){
  int nrecur,nlevel;
  Mlevelinfo *ml;
  int active_handle = get_active_handle();

  eDIM = E->fhead->dim();

  nrecur = option("recursive");

  Set_V2E(E,B->nv_solve);
  ml = Mlevel_decom(E,B,&nlevel,nrecur);


  if(nrecur > nlevel){
    nrecur = nlevel;
    DO_PARALLEL{
      fprintf(stderr,"Only %d recursions were possible on proc %d\n",
        nlevel,pllinfo[active_handle].procid);
    }
    else
      fprintf(stderr,"Only %d recursions were possible\n",nlevel);
  }

  if(nrecur)
    B->rslv  = setup_numbering(ml,nrecur,E,B);

#ifdef MLEV_MEMORY
  memory(E,B);
#endif

  free_V2E();
  free_Mlevel(ml,nlevel);
}

static Fctcon *SCFacetConnect(int nsols, Mlevelinfo *m, int *vmap,
            int *emap, int *fmap);

static Rsolver *setup_numbering(Mlevelinfo *ml, int nlevel,
        Element_List *E, Bsystem *B){
  register int i,j,k;
  int     id,cnt,cnt1,one=1;
  int     nvs  = B->nv_solve;
  int    *ngid = ivector(0,nvs);
  int     nes  = B->ne_solve;
  int    *edge = ivector(0,nes),*edglen, *edgmap;
  Rsolver *rslv;
  Recur   *r;

  int     nfs   = B->nf_solve,l;
  int     *face = ivector(0,nfs),*facelen, *facemap;

  GidInfo  *ginf;
  rslv            = (Rsolver *)calloc(1,sizeof(Rsolver));
  rslv->nrecur    = nlevel;
  r = rslv->rdata = (Recur *)calloc(nlevel,sizeof(Recur));
  Element *U;

  edglen = ivector(0,nes);
  edgmap = ivector(0,nes);
  ifill(nes,-1,edgmap,1);

  /* set up original list of edge lengths */
  for(i = 0; i < B->nel; ++i){
    U = E->flist[i];
    for(j = 0; j < U->Nedges; ++j)
      if((id = U->edge[j].gid) < nes)
  edglen[id] = U->edge[j].l;
  }

  if(eDIM == 3){
    facelen = ivector(0,nfs);
    facemap = ivector(0,nfs);
    ifill(nfs,-1,facemap,1);

    /* set up original list of edge lengths */
    for(i = 0; i < B->nel; ++i){
      U = E->flist[i];
      for(j = 0; j < U->Nfaces; ++j)
  if((id = U->face[j].gid) < nfs) {
    l = U->face[j].l;
    if(U->Nfverts(j) == 3)
      facelen[id] = l*(l+1)/2;
    else
      facelen[id] = l*l;
  }
    }
  }

  /* set up innermost boundary system */

  ifill(nvs,-1,ngid,1);
  ifill(nes,-1,edge,1);
  if(eDIM == 3)
    ifill(nfs,-1,face,1);

  /* set up basic boundary numbering scheme for iterative solver */
  /* put global vertices first */
  cnt = 0;
  for(i = 0; i < ml[nlevel-1].npatch; ++i){
    ginf = ml[nlevel-1].Bdy;
    for(j = 0; j < ginf[i].nvert; ++j){
      if(ngid[ginf[i].vgid[j]] == -1)
  ngid[ginf[i].vgid[j]] = cnt++;
    }
  }
  rslv->Ainfo.nv_solve = cnt;

  for(i = 0,cnt1 = 0; i < ml[nlevel-1].npatch; ++i){
    ginf = ml[nlevel-1].Bdy;
    for(j = 0; j < ginf[i].nedge; ++j){
      if(edge[ginf[i].egid[j]] == -1){
  edge[ginf[i].egid[j]] = cnt;
  cnt += edglen[ginf[i].egid[j]];
  edgmap[ginf[i].egid[j]] = cnt1++; /* keep reverse mapping for block
               precon */
      }
    }
  }

  if(eDIM == 3)
    for(i = 0,cnt1=0; i < ml[nlevel-1].npatch; ++i){
      ginf = ml[nlevel-1].Bdy;
      for(j = 0; j < ginf[i].nface; ++j){
  if(face[ginf[i].fgid[j]] == -1){
    face[ginf[i].fgid[j]] = cnt;
    cnt += facelen[ginf[i].fgid[j]];
    facemap[ginf[i].fgid[j]] =  cnt1++;
  }
      }
    }

  /* reverse cuthill mckee sorting if direct method */
  if(B->smeth == direct){
    Fctcon  *ptcon;
    int     nvert,nedge,nsols;
    int     *bwdm,*newmap;
    int nface;

    if(eDIM == 2)
      bwdm = ivector(0,nes+nvs-1);
    else
      bwdm = ivector(0,nes+nvs+nfs-1);

    /* set up consequative list of unknowns (i.e. vertices and edges)
       as well as the backward mapping                                */

    nvert = 0;
    for(i = 0; i < nvs; ++i)
      if(ngid[i]+1) {
  ngid[i] = nvert;
  bwdm[nvert++] = i;
      }

    nedge = 0;
    for(i = 0; i < nes; ++i)
      if(edge[i]+1){
  edge[i] = nvert + nedge;
  bwdm[nvert + nedge++] = i;
      }
  if(eDIM == 3){
    nface = 0;
    for(i = 0; i < nfs; ++i)
      if(face[i]+1){
  face[i] = nvert + nedge + nface;
  bwdm[nvert + nedge + nface++] = i;
      }
    nsols = nvert + nedge + nface;
  }
  else
    nsols = nvert + nedge;


  newmap = ivector(0,nsols-1);
  ptcon  = SCFacetConnect(nsols,ml + nlevel-1,ngid,edge,face);

  MinOrdering(nsols,ptcon,newmap);

  /* make up list of sorted global gids */
  for(i = 0,cnt = 0; i < nsols; ++i)
    if(newmap[i] < nvert)
      ngid[bwdm[newmap[i]]] = cnt++;
    else if(  eDIM == 2){
      edge[bwdm[newmap[i]]] = cnt;
      cnt += edglen[bwdm[newmap[i]]];
    }
    else if (eDIM == 3 && newmap[i] < nvert + nedge){
      edge[bwdm[newmap[i]]] = cnt;
      cnt += edglen[bwdm[newmap[i]]];
    }
    else if(eDIM == 3){
      face[bwdm[newmap[i]]] = cnt;
      cnt += facelen[bwdm[newmap[i]]];
    }

    free(bwdm); free(newmap);
    free_Fctcon(ptcon,nsols);
  }


  /* go through patches and set up patch information           *
   * do this in reverse order to make the numbering consistent *
   * with the standard ordering                                */

  for(i = nlevel-1; i >= 0; --i){
    r[i].id = i;
    r[i].npatch     = ml[i].npatch;
    r[i].pmap       = ml[i].old2new;
    r[i].patchlen_a = ivector(0,r[i].npatch-1);
    r[i].patchlen_c = ivector(0,r[i].npatch-1);

    /* set up new ordering for interior patches -- putting the
       vertices in the middle surrounded by the edges and then faces */

    for(j = 0; j < r[i].npatch; ++j){
      ginf = ml[i].Int + j;
      cnt1 = cnt;

      if(eDIM == 3)
  for(k = 0; k < ginf->nface/2; ++k){
    face[ginf->fgid[k]] = cnt;
    cnt += facelen[ginf->fgid[k]];
  }

      for(k = 0; k < ginf->nedge/2; ++k){
  edge[ginf->egid[k]] = cnt;
  cnt += edglen[ginf->egid[k]];
      }

      for(k = 0; k < ginf->nvert; ++k)
  ngid[ginf->vgid[k]] = cnt++;

      for(k = ginf->nedge/2; k < ginf->nedge; ++k){
  edge[ginf->egid[k]] = cnt;
  cnt += edglen[ginf->egid[k]];
      }

      if(eDIM == 3)
  for(k = ginf->nface/2; k < ginf->nface; ++k){
    face[ginf->fgid[k]] = cnt;
    cnt += facelen[ginf->fgid[k]];
  }

      r[i].patchlen_c[j] = cnt - cnt1;

      /* calculate the boundary length */
      ginf = ml[i].Bdy + j;
      cnt1 = ginf->nvert;
      for(k =0; k < ginf->nedge; ++k)
  cnt1 += edglen[ginf->egid[k]];
      if(eDIM == 3)
  for(k =0; k < ginf->nface; ++k)
    cnt1 += facelen[ginf->fgid[k]];

      r[i].patchlen_a[j] = cnt1;
      rslv->max_asize = max(rslv->max_asize,cnt1);
    }
  }

  /* set up start location interior systems */
  cnt = B->nsolve;
  for(i = 0; i < nlevel; ++i){
    r[i].cstart = cnt - isum(r[i].npatch,r[i].patchlen_c,1);
    cnt = r[i].cstart;
  }

  /* copy back gid's to vertices */
  for(i = 0; i < B->nel; ++i){
    U = E->flist[i];
    for(j = 0; j < U->Nverts; ++j)
      if((id = U->vert[j].gid) < nvs)
  U->vert[j].gid = ngid[id];
  }

  /* copy back edge id's */
  icopy(nes,edge,1,B->edge,1);

  if(eDIM == 3){
    /* copy back face id's */
    icopy(nfs,face,1,B->face,1);
  }

  /* calculate the local to global boundary mapping */
  for(i = 0; i < nlevel; ++i){
    r[i].map = (int **)malloc(r[i].npatch*sizeof(int *));
    for(j = 0; j < r[i].npatch; ++j)
      if(r[i].patchlen_a[j]){
  r[i].map[j] = ivector(0,r[i].patchlen_a[j]-1);

  ginf = ml[i].Bdy + j;

  cnt = 0;
  /* put vertices first at present */
  for(k = 0; k < ginf->nvert; ++k)
    r[i].map[j][cnt++] = ngid[ginf->vgid[k]];

  /* edges */
  for(k = 0; k < ginf->nedge; ++k){
    iramp(edglen[ginf->egid[k]],edge+ginf->egid[k],
    &one,r[i].map[j]+cnt,1);
    cnt += edglen[ginf->egid[k]];
  }

  if(eDIM == 3){
    /* face */
    for(k = 0; k < ginf->nface; ++k){
      iramp(facelen[ginf->fgid[k]],face+ginf->fgid[k],
      &one,r[i].map[j]+cnt,1);
      cnt += facelen[ginf->fgid[k]];
    }
  }
      }
  }

  /* block preconditioners */
  if((B->smeth == iterative)&&((B->Precon == Pre_Block)||
             (B->Precon == Pre_Block_Stokes))){
    Blockp *Blk;
    double cnt;

    rslv->precon = (Precond *)calloc(1,sizeof(Precond));

    Blk = &(rslv->precon->blk);

    Blk->ngv = ivector(0,r[nlevel-1].npatch-1);

    for(i = 0; i < r[nlevel-1].npatch; ++i)
      Blk->ngv[i] = ml[nlevel-1].Bdy[i].nvert;

    /* add up all global vertices attached to interior of patches */
    cnt = 0;
    for(i = 0; i < nlevel; ++i)
      for(j = 0; j < r[i].npatch; ++j)
  cnt += ml[i].Int[j].nvert;

    Blk->nlgid  = (int) cnt;

    Blk->nle    = ivector(0,r[nlevel-1].npatch-1);
    Blk->edglen = (int **)malloc(r[nlevel-1].npatch*sizeof(int *));
    Blk->edgid  = (int **)malloc(r[nlevel-1].npatch*sizeof(int *));

    if(eDIM == 3){

      Blk->nlf    = ivector(0,r[nlevel-1].npatch-1);
      Blk->faclen = (int **)malloc(r[nlevel-1].npatch*sizeof(int *));
      Blk->facgid = (int **)malloc(r[nlevel-1].npatch*sizeof(int *));
    }

    for(i = 0; i < ml[nlevel-1].npatch; ++i){
      ginf = ml[nlevel-1].Bdy;
      Blk->nle[i] = ginf[i].nedge;
      Blk->edglen[i] = ivector(0,ginf[i].nedge-1);
      Blk->edgid [i] = ivector(0,ginf[i].nedge-1);

      for(j = 0; j < ginf[i].nedge; ++j){
  Blk->edglen[i][j] = edglen[ginf[i].egid[j]];
  Blk->edgid [i][j] = edgmap[ginf[i].egid[j]];
      }
      if(eDIM == 3){
  Blk->nlf[i]    = ginf[i].nface;
  Blk->faclen[i] = ivector(0,ginf[i].nface-1);
  Blk->facgid[i] = ivector(0,ginf[i].nface-1);

  for(j = 0; j < ginf[i].nface; ++j){
    Blk->faclen[i][j] = facelen[ginf[i].fgid[j]];
    Blk->facgid[i][j] = facemap[ginf[i].fgid[j]];
  }
      }
    }

    Blk->nge = 0;
    for(i = 0; i < nes; ++i)
      if(edgmap[i]+1) Blk->nge++;
    if(eDIM == 3){
      Blk->ngf = 0;
      for(i = 0; i < nfs; ++i)
  if(facemap[i]+1) Blk->ngf++;
    }
  }

  free(ngid); free(edglen); free(edge); free(edgmap);
  if(eDIM == 3){
    free(face); free(facelen); free(facemap);
  }

  return rslv;
}

static void free_Mlevel(Mlevelinfo *m, int nlevel){
  register int i,j;

  for(i = 0; i < nlevel; ++i){
    for(j = 0; j < m[i].npatch; ++j){

      free(m[i].Int[j].vgid);
      free(m[i].Bdy[j].vgid);
      free(m[i].Int[j].egid);
      free(m[i].Bdy[j].egid);
      if(eDIM == 3){
  // must fix
  free(m[i].Int[j].fgid);
  free(m[i].Bdy[j].fgid);
      }
    }
    free(m[i].Int);
    free(m[i].Bdy);
  }

  if(nlevel) free(m);
}

/* this routine coarsen's the mesh partition into localised regions
   and returns the 'nlevel' patches via a structure Mlevelinfo */

static Patch *set_init_patch (Element_List *E);
static int    next_patch     (Patch *p);
static void   Fill_Mlevel    (Mlevelinfo *ml, Patch *p,
            Element_List *E, Bsystem *);
static void free_patch_list  (Patch *p);

/* This function is designed to be called from set_pllinfo in
   Parallel.c and is used to design the mesh partitioning based on the
   substructuring. In This routine the function Set_Parallel_Patch is
   also called to set up the local information of the patching so that
   it can be used in Mlevel_decom */

static Patch *pll_p;
void Substruct_Partition(Element_List *E, int nvs, int *npatch, int **elmtid,
       int **xelmt, int **adjncy, int **xadj){
  Patch      *p1;
  int        nlevel, nrecur;

  nrecur = option("recursive");
  Set_V2E(E,nvs);

  pll_p = p1 = set_init_patch(E);

  nlevel = 0;

  while((nlevel < nrecur)&&next_patch(p1)){
    nlevel++;
    p1 = p1->next;
  }

  /* gather information about patch */
  npatch[0] = p1->npatch;
  xelmt[0] = ivector(0,p1->npatch);
  icopy(p1->npatch+1,p1->xelmt,1,xelmt[0],1);

  elmtid[0] = ivector(0,p1->nel);
  icopy(p1->nel,p1->elmtid,1,elmtid[0],1);

  xadj[0] = ivector(0,p1->npatch);
  icopy(p1->npatch+1,p1->xadj,1,xadj[0],1);

  adjncy[0] = ivector(0,p1->xadj[p1->npatch]-1);
  icopy(p1->xadj[p1->npatch],p1->adjncy,1,adjncy[0],1);

  free_V2E();
}

void Set_Parallel_Patch(int *partition){
  register int i,j;
  Patch *p,*p1;
  int *n2o,*o2n,*xelmt,*elmtid,*old2new,cnt,npatch;
  int *revmap;
  int active_handle = get_active_handle();

  revmap = ivector(0,pllinfo[active_handle].gnel-1);
  ifill(pllinfo[active_handle].gnel,-1,revmap,1);
  for(i = 0; i < pllinfo[active_handle].nloop; ++i)
    revmap[pllinfo[active_handle].eloop[i]] = i;

  /* set p to innermost recursion */
  p = pll_p->next;
  while(p->next) p = p->next;

  n2o = ivector(0,p->npatch-1);
  o2n = ivector(0,p->npatch-1);

  /* identify global patch id that are on this processor */
  for(i = 0,cnt=0; i < p->npatch; ++i)
    if(partition[i] == pllinfo[active_handle].procid){
      n2o[cnt] = i;
      o2n[i  ] = cnt++;
    }
    else
      o2n[i] = -1;

  while(p != pll_p){

    npatch = cnt;
    xelmt   = ivector(0,npatch);
    elmtid  = ivector(0,pllinfo[active_handle].nloop-1);

    /* gather together relevant global element id's */
    xelmt[i] = 0;
    for(i = 0; i < npatch; ++i){
      xelmt[i+1] = xelmt[i];
      for(j = p->xelmt[n2o[i]]; j < p->xelmt[n2o[i]+1]; ++j){
  elmtid[xelmt[i+1]] = p->elmtid[j];
  xelmt[i+1]++;
      }
    }

    /* unshuffle elmtid into local eid's */
    for(i = 0; i < pllinfo[active_handle].nloop; ++i){
      if(revmap[elmtid[i]] == -1)
  error_msg("error in sorting substructuring");
      elmtid[i] = revmap[elmtid[i]];
    }

    free(p->elmtid);
    free(p->xelmt);
    p->elmtid = elmtid;
    p->xelmt  = xelmt;
    p->npatch = npatch;
    p->nel    = pllinfo[active_handle].nloop;

    /* find next p down link list*/
    p1 = pll_p;
    while(p1->next != p) p1 = p1->next;

    /* redefine old2new */
    for(i = 0; i < p1->npatch; ++i)
      if(o2n[p->old2new[i]]+1)
  p->old2new[i] = o2n[p->old2new[i]];
      else
  p->old2new[i] = -1;

    /* generate new mappings */
    free(o2n); free(n2o);
    o2n = ivector(0,p1->npatch-1);
    n2o = ivector(0,p1->npatch-1);

    for(i = 0,cnt=0; i < p1->npatch; ++i)
      if(p->old2new[i] + 1){
  n2o[cnt] = i;
  o2n[i  ] = cnt++;
      }
      else
  o2n[i] = -1;

    npatch = cnt;

    for(i = 0; i < npatch; ++i)
      old2new[i] = p->old2new[n2o[i]];
    free(p->old2new);

    p->old2new = old2new;

    p = p1;
  }

  /* perform some sorting checks */

  if(npatch != pllinfo[active_handle].nloop)
    error_msg("error in sorting parallel substructuring");

  for(i = 0; i < npatch; ++i)
    if(revmap[n2o[i]] == -1)
      error_msg("error in ordering parallel substructuring");

  free(o2n); free(n2o); free(revmap);
}

static Mlevelinfo *Mlevel_decom(Element_List *E, Bsystem *B, int *nlevel,
        int nrecur){
  Mlevelinfo *ml;
  Patch      *p,*p1;
  int active_handle = get_active_handle();

  nlevel[0] = 0;
#ifdef RECURPART
  DO_PARALLEL{
#ifdef PRINT_MLEVEL
    p  = pll_p->next;
    while(p){
      int cnt = 1;
      FILE *out;
      char buf[BUFSIZ];

      sprintf(buf,"MlevelP%d_%d.tec",pllinfo[active_handle].procid,cnt++);
      out = fopen(buf,"w");
      Print_Patch(E,p,out);
      fclose(out);
      p = p->next;
    }
#endif
    p = pll_p;
    for(p1 = p->next; p1; p1 = p1->next) nlevel[0]++;
  }
  else
#endif
    {
    p1 = p = set_init_patch(E);

    while((nlevel[0] < nrecur)&&next_patch(p1)){
      nlevel[0]++;
      p1 = p1->next;
#ifdef PRINT_MLEVEL
      {
  FILE *out;
  char buf[BUFSIZ];

  sprintf(buf,"Mlevel_%d.tec",nlevel[0]);
  if(E->fhead->type == 'u'){
    out = fopen(buf,"w");
    Print_Patch(E,p1,out);
    fclose(out);
  }
      }
#endif
    }
  }

  if(nlevel[0]) {
    ml = (Mlevelinfo *)malloc(nlevel[0]*sizeof(Mlevelinfo));

    Fill_Mlevel(ml,p->next,E,B);
  }

  free_patch_list(p);

  return ml;
}

static void Set_V2E(Element_List *E, int nv_solve){
  register int i,j;
  int      gid;
  int      nel = countelements(E->fhead);
  VtoE     *v2e,*v1;
  Element *U;

  /* set up vertex to element connectivity for use later */
  v2e = (VtoE *)calloc(nv_solve,sizeof(VtoE));

  /* set up a link list providing the element id's surrounding any vertex */
  for(i = 0; i < nel; ++i){
    U = E->flist[i];
    for(j = 0; j < U->Nverts; ++j)
      if((gid = U->vert[j].gid) < nv_solve){
  v1 = (VtoE *)malloc(sizeof(VtoE));
  v1->eid = i;
  v1->nvert = j;
  v1->next = v2e[gid].next;
  v2e[gid].next = v1;
      }
  }

  V2Estore.nvs = nv_solve;
  V2Estore.v2e = v2e;
}

static void Get_V2E(VtoE **v2e, int *nvs){
  v2e[0] = V2Estore.v2e;
  nvs[0] = V2Estore.nvs;
}

static void free_V2E(void){
  int i;
  VtoE *v2e,*v1,*v2;

  /* free up vertex list */
  v2e = V2Estore.v2e;
  for(i = 0; i < V2Estore.nvs; ++i){
    v1 = v2e[i].next;
    while(v1){
      v2 = v1->next;
      free(v1);
      v1 = v2;
    }
  }
  free(v2e);
}

static Patch *set_init_patch(Element_List *U){
  register int i,j;
  int      cnt;
  int      nel = countelements(U->fhead), zero = 0, one = 1;
  int      *adjncy,*xadj,medg;
  Patch    *p;
  Element  *E;

  p = (Patch *)calloc(1,sizeof(Patch));

  p->npatch = nel;
  p->nel    = nel;
 /* set up adjacency list with elements that have connecting faces/edges */
  if(eDIM == 2){
    medg =0;
    for(E = U->fhead; E; E = E->next)
      for(j = 0; j < E->Nedges; ++j) if(E->edge[j].base) ++medg;


    xadj   = p->xadj   = ivector(0,p->npatch);
    adjncy = p->adjncy = ivector(0,medg-1);
    izero(nel+1,xadj,1);
    xadj[0] = 0;
    cnt     = 0;
    for(E = U->fhead,i=0; E; E = E->next,++i){
      xadj[i+1] = xadj[i];
      for(j = 0; j < E->Nedges; ++j){
  if(E->edge[j].base){
    if(E->edge[j].link){
      adjncy[cnt++] = E->edge[j].link->eid;
      xadj[i+1]++;
    }
    else{
      adjncy[cnt++] = E->edge[j].base->eid;
      xadj[i+1]++;
    }
  }
      }
    }
  }
  else{
    switch(iparam("InitPartType")){
    case 0: /* use element adjacent to faces */
      medg =0;
      for(i = 0; i < nel; ++i){
  E = U->flist[i];
  for(j = 0; j < E->Nfaces; ++j) if(E->face[j].link) ++medg;
      }

      xadj   = p->xadj   = ivector(0,p->npatch);
      adjncy = p->adjncy = ivector(0,medg-1);
      izero(nel+1,xadj,1);
      xadj[0] = 0;
      cnt     = 0;
      for(i = 0; i < nel; ++i){
  E = U->flist[i];
  xadj[i+1] = xadj[i];
  for(j = 0; j < E->Nfaces; ++j)
    if(E->face[j].link){
      adjncy[cnt++] = E->face[j].link->eid;
      xadj[i+1]++;
    }
      }
      break;
    case 1: /* use elements around edges */
      {
  int *list = ivector(0,nel-1);
  Edge *edg;

  xadj   = p->xadj  = ivector(0,p->npatch);
  izero(nel+1,xadj,1);
  adjncy = (int *)0;
  xadj[0] = 0;

  cnt = 0;
  for(i =0 ; i < nel; ++i){
    izero(nel,list,1);
    E = U->flist[i];
    for(j = 0; j < E->Nedges; ++j)
      for(edg = E->edge[j].base; edg; edg = edg->link)
        list[edg->eid] = 1;
    /* set the element we are looking at to zero */
    list[i] = 0;

    medg = isum(nel,list,1);
    xadj[i+1] = xadj[i]+medg;

    adjncy = (int*) realloc(adjncy,xadj[i+1]*sizeof(int));
    for(j = 0; j < nel; ++j)
      if(list[j]) adjncy[cnt++] = j;
  }

  p->adjncy = adjncy;
  free(list);
      }
    }
  }

  p->xelmt  = ivector(0,nel);
  p->elmtid = ivector(0,nel);

  iramp(nel+1,&zero,&one,p->xelmt,1);
  iramp(nel,&zero,&one,p->elmtid,1);

  return p;
}



static int Edge_connection(Patch *p);
static int Vertex_connection(Patch *p);
static int External_Partitioner(Patch *p);

static int next_patch(Patch *p){
  int npatch;

  switch(iparam("IPartType")){
  case 0:
    npatch = Vertex_connection(p);
    break;
  case 1:
    npatch = Edge_connection(p);
    break;
  case 2:
    npatch = External_Partitioner(p);
    break;
  default:
    error_msg(unknown partition type in next_patch);
  }
  return npatch;
}

/* given a patch defined in terms of a graphs (using metis format) this
   function determines a coarser graph by combining adjacent patches and
   the adds the new graph in a link list fashion. The function returns
   the number of new patches which must be >= 2 */

static int Edge_connection(Patch *p){
  register int i,j,k;
  Patch *p1;
  int npatch = 0, nadj,trip,start,stop;
  int *flag   = ivector(0,p->npatch-1);
  int *elmtid = ivector(0,p->nel-1);
  int *xelmt  = ivector(0,p->npatch);
  int *xadj   = ivector(0,p->npatch);
  int *adjncy = ivector(0,p->xadj[p->npatch]);
  int active_handle = get_active_handle();

  ifill(p->npatch,-1,flag,1);

  xelmt[0] = 0; xadj[0] = 0;

  /* first sweep gather elements according to adjacency */

  for(i = 0; i < p->npatch; ++i){
    /* search through adjacency list to see if any
       connecting patches have been assigned */
    trip = 1;
    for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
      if(flag[p->adjncy[j]] + 1) trip = 0;

    if(trip){ /*  set new patch */

      /* put all element of patch into new elmtid list */
      xelmt[npatch+1] = xelmt[npatch];
      nadj = p->xelmt[i+1] - p->xelmt[i];
      icopy (nadj,p->elmtid + p->xelmt[i],1,elmtid + xelmt[npatch+1],1);
      xelmt[npatch+1] += nadj;
      flag[i] = npatch;

      for(j = p->xadj[i]; j < p->xadj[i+1]; ++j){
  nadj = p->xelmt[p->adjncy[j]+1] - p->xelmt[p->adjncy[j]];
  icopy (nadj,p->elmtid + p->xelmt[p->adjncy[j]],1,
         elmtid + xelmt[npatch+1],1);
  xelmt[npatch+1] += nadj;
  flag[p->adjncy[j]] = npatch;
      }

      /* construct  adjacency using surrounding old patch id's */
      xadj[npatch+1] = xadj[npatch];
      for(j = p->xadj[i]; j < p->xadj[i+1]; ++j){
  start = p->xadj[p->adjncy[j]];
  stop  = p->xadj[p->adjncy[j]+1];
  for(k = start; k < stop; ++k)
    if(flag[p->adjncy[k]] != npatch){
      adjncy[xadj[npatch+1]] = p->adjncy[k];
      xadj[npatch+1]++;
    }
      }
      npatch++;
    }
  }

  /* now go through list checking for unassigned patches */
  for(i = 0; i < p->npatch; ++i)
    if(flag[i] == -1){

      /* check to see if there is a neighbouring unassigned patch */
      trip = 0;
      for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
  if(flag[p->adjncy[j]] == -1){
    trip = 1;
    break;
  }

      if(trip){ /* join unassigned patches into a new patch      */
  /* put all element of patch into new elmtid list */
  xelmt[npatch+1] = xelmt[npatch];
  nadj = p->xelmt[i+1] - p->xelmt[i];
  icopy(nadj,p->elmtid + p->xelmt[i],1,elmtid + xelmt[npatch+1],1);
  xelmt[npatch+1] += nadj;
  flag[i] = npatch;

  for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
    if(flag[p->adjncy[j]] == -1){
      nadj = p->xelmt[p->adjncy[j]+1] - p->xelmt[p->adjncy[j]];
      icopy (nadj,p->elmtid + p->xelmt[p->adjncy[j]],1,
       elmtid + xelmt[npatch+1],1);
      xelmt[npatch+1] += nadj;
      flag[p->adjncy[j]] = npatch;
    }

  xadj[npatch+1] = xadj[npatch];
  /* put in the adjancy of the element first */
  start = p->xadj[i];
  stop  = p->xadj[i+1];
  for(k = start; k < stop; ++k)
    if(flag[p->adjncy[k]] != npatch){
      adjncy[xadj[npatch+1]] = p->adjncy[k];
      xadj[npatch+1]++;
    }

  /* construct  adjacency of surrounding old patch id's */
  for(j = p->xadj[i]; j < p->xadj[i+1]; ++j){
    if(flag[p->adjncy[j]] == npatch){
      start = p->xadj[p->adjncy[j]];
      stop  = p->xadj[p->adjncy[j]+1];
      for(k = start; k < stop; ++k)
        if(flag[p->adjncy[k]] != npatch){
    adjncy[xadj[npatch+1]] = p->adjncy[k];
    xadj[npatch+1]++;
        }
    }
  }
  npatch++;
      }
      else{ /* put patch in neighbouring patch with lowest number of elmts */

  /* find neighbouring patch with lowest number of elements */
  start = p->nel;
  for(j = p->xadj[i]; j < p->xadj[i+1]; ++j){
    nadj = xelmt[flag[p->adjncy[j]]+1] - xelmt[flag[p->adjncy[j]]];
    if(nadj < start){
      start = nadj;
      trip = flag[p->adjncy[j]];
    }
  }

  /* add this patch to patch with smallest number of elements */
  xelmt[npatch+1] = xelmt[npatch];

  nadj = p->xelmt[i+1] - p->xelmt[i];

  /* move all elmtid up by nadj */
  start = xelmt[npatch]-1;
  stop  = xelmt[trip+1];
  for(j = start; j >= stop; --j)
    elmtid[j+ nadj] = elmtid[j];

  /* copy in new elements */
  icopy(nadj,p->elmtid + p->xelmt[i],1,elmtid + xelmt[trip+1],1);

  /* increment xelmt */
  for(j = trip+1; j < npatch+1; ++j)
    xelmt[j] += nadj;

  flag[i] = trip;

  /* construct  adjacency of surround old patch id's */

  /* count number of surrounding patches that don't belong to trip */
  nadj = 0;
  for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
    if(flag[p->adjncy[j]] != npatch) ++nadj;

  /* shift adjacency list up by nadj */
  start = xadj[npatch]-1;
  stop  = xadj[trip+1];
  for(j = start; j >= stop; --j)
    adjncy[j+nadj] = adjncy[j];

  /* put new points in list */
  for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
    if(flag[p->adjncy[j]] != npatch)
      adjncy[stop++] = p->adjncy[j];

  /* increment xadj */
  for(j = trip+1; j < npatch+1; ++j)
    xadj[j] += nadj;
      }
    }

#ifdef PARALLEL
  if((npatch > 1)||((p->npatch > 1)&&(pllinfo[active_handle].nprocs > 1))){
#else
  if((npatch > 1)||(p->npatch > 1)){
#endif
    /* finally construct new patch if npatch > 2. In this process we
       use flag to construct the adjancy interms of the new patch id's */
    /* if a parallel solve always generate a recursion */

    p->next = p1 = (Patch *)calloc(1,sizeof(Patch));
    p1->npatch   = npatch;
    p1->nel      = p->nel;
    p1->elmtid   = elmtid;
    p1->xelmt    = ivector(0,npatch);
    icopy(npatch+1,xelmt,1,p1->xelmt,1);

    p1->old2new  = flag;
    /* set up adjacency list in terms of new patch id's */
    p1->xadj = ivector(0,npatch);
    p1->adjncy = adjncy;

    p1->xadj[0] = 0;
    for(i = 0; i < npatch; ++i){
      ifill(npatch,-1,xelmt,1);
      for(j = xadj[i]; j < xadj[i+1]; ++j)
  xelmt[flag[adjncy[j]]] = flag[adjncy[j]];

      p1->xadj[i+1] = p1->xadj[i];
      for(j = 0; j < npatch; ++j)
  if((xelmt[j] != -1)&&(xelmt[j] != i)){
    p1->adjncy[p1->xadj[i+1]] = xelmt[j];
    p1->xadj[i+1]++;
  }
    }


    free(xelmt); free(xadj);
    return npatch;
  }
  else{
    free(flag); free(elmtid); free(xelmt); free(xadj); free(adjncy);
    return 0;
  }
}


/* use the elements around a global vertex between more than two patches to
   define a new patch  */

static int Vertex_connection(Patch *p){
  if(p->npatch <= 1)
    return 0;
  else{
    register int i,j,k;
    Patch *p1;
    int npatch = 0, nadj,trip,start,stop, ngvert, ptchid;
    int limit_search, unassigned, nvs, *gvert;
    int *e2p    = ivector(0,p->nel-1);
    int *flag   = ivector(0,p->npatch-1);
    int *elmtid = ivector(0,p->nel-1);
    int *xelmt  = ivector(0,p->npatch);
    int *xadj   = ivector(0,p->npatch);
    int *adjncy = ivector(0,p->xadj[p->npatch]);
    VtoE *v2e,*v1;
    int active_handle = get_active_handle();
    xelmt[0] = 0; xadj[0] = 0;

    /* identify which vertices lie between more than one patch */

    Get_V2E(&v2e,&nvs);
    gvert  = ivector(0,nvs-1);

    ifill(nvs,-1,gvert,1);

    /* find the patch id of each element at current patch */
    for(i = 0; i <  p->npatch; ++i)
      for(j = p->xelmt[i]; j < p->xelmt[i+1]; ++j)
  e2p[p->elmtid[j]] = i;

    ngvert = 0;
    for(i = 0; i < nvs; ++i){
      izero(p->npatch,flag,1);
      for(v1 = v2e[i].next; v1; v1 = v1->next)
  flag[e2p[v1->eid]] = 1;
      if(isum(p->npatch,flag,1) > 2)
  gvert[ngvert++] = i;
    }


    ifill(p->npatch,-1,flag,1);


    /* first  gather patches around free global vertex  */
    for(i = 0; i < ngvert; ++i){

      /* search through adjacency list to see if any
       connecting patches have been assigned */
      trip = 1;
      for(v1 = v2e[gvert[i]].next; v1; v1 = v1->next)
  if(flag[e2p[v1->eid]] + 1) trip = 0;

      if(trip){ /*  set new patch */

  /* put all element of patch into new elmtid list */
  xelmt[npatch+1] = xelmt[npatch];
  xadj [npatch+1] = xadj [npatch];

  for(v1 = v2e[gvert[i]].next; v1; v1 = v1->next)
    if(flag[e2p[v1->eid]] == -1){
      ptchid = e2p[v1->eid];
      nadj   = p->xelmt[ptchid+1] - p->xelmt[ptchid];
      icopy (nadj,p->elmtid + p->xelmt[ptchid],1,
     elmtid + xelmt[npatch+1],1);
      xelmt[npatch+1] += nadj;
      flag[ptchid] = npatch;

      /* construct  adjacency of surrounding old patch id's */
      start = p->xadj[ptchid];
      stop  = p->xadj[ptchid+1];
      for(k = start; k < stop; ++k)
        if(flag[p->adjncy[k]] != npatch){
    adjncy[xadj[npatch+1]] = p->adjncy[k];
    xadj[npatch+1]++;
        }
    }
  npatch++;
      }
    }


#if 0
    /* As a second step search through the global vertex list and make
       new patches if more than one patch around a vertex is
       unassigned */


    for(i = 0; i < ngvert; ++i){

      /* search through adjacency list to see how many
   unassigned patches there are  */
      trip = 0;
      for(v1 = v2e[gvert[i]].next; v1; v1 = v1->next)
  if(flag[e2p[v1->eid]] == -1) ++trip;

      if(trip >= 2){ /*  set new patch */

  /* put all element of patch into new elmtid list */
  xelmt[npatch+1] = xelmt[npatch];
  xadj [npatch+1] = xadj [npatch];

  for(v1 = v2e[gvert[i]].next; v1; v1 = v1->next)
    if(flag[e2p[v1->eid]] == -1){
      ptchid = e2p[v1->eid];
      nadj   = p->xelmt[ptchid+1] - p->xelmt[ptchid];
      icopy (nadj,p->elmtid + p->xelmt[ptchid],1,
     elmtid + xelmt[npatch+1],1);
      xelmt[npatch+1] += nadj;
      flag[ptchid] = npatch;

      /* construct  adjacency of surrounding old patch id's */
      start = p->xadj[ptchid];
      stop  = p->xadj[ptchid+1];
      for(k = start; k < stop; ++k)
        if(flag[p->adjncy[k]] != npatch){
    adjncy[xadj[npatch+1]] = p->adjncy[k];
    xadj[npatch+1]++;
        }
    }
  npatch++;
      }
    }
#endif

#if 1
    /* This section will loop through and assign any two unassigned patch
       to be a new patch */
    for(i = 0; i < p->npatch; ++i)
      if(flag[i] == -1){

  /* check to see if there is a neighbouring unassigned patch */
  trip = 0;
  for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
    if(flag[p->adjncy[j]] == -1){
      trip = 1;
      break;
    }

  if(trip){ /* join unassigned patches into a new patch      */
    /* put all element of patch into new elmtid list */
    xelmt[npatch+1] = xelmt[npatch];
    nadj = p->xelmt[i+1] - p->xelmt[i];
    icopy(nadj,p->elmtid + p->xelmt[i],1,elmtid + xelmt[npatch+1],1);
    xelmt[npatch+1] += nadj;
    flag[i] = npatch;

    for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
      if(flag[p->adjncy[j]] == -1){
        nadj = p->xelmt[p->adjncy[j]+1] - p->xelmt[p->adjncy[j]];
        icopy (nadj,p->elmtid + p->xelmt[p->adjncy[j]],1,
         elmtid + xelmt[npatch+1],1);
        xelmt[npatch+1] += nadj;
        flag[p->adjncy[j]] = npatch;
      }

    xadj[npatch+1] = xadj[npatch];
    /* put in the adjncy of the element first */
    start = p->xadj[i];
    stop  = p->xadj[i+1];
    for(k = start; k < stop; ++k)
      if(flag[p->adjncy[k]] != npatch){
        adjncy[xadj[npatch+1]] = p->adjncy[k];
        xadj[npatch+1]++;
      }

    /* construct  adjacency of surround old patch id's */
    for(j = p->xadj[i]; j < p->xadj[i+1]; ++j){
      if(flag[p->adjncy[j]] == npatch){
        start = p->xadj[p->adjncy[j]];
        stop  = p->xadj[p->adjncy[j]+1];
        for(k = start; k < stop; ++k)
    if(flag[p->adjncy[k]] != npatch){
      adjncy[xadj[npatch+1]] = p->adjncy[k];
      xadj[npatch+1]++;
    }
      }
    }
    npatch++;
  }
      }

#endif

    /* Finally this section will assign any unassigned patch to a
       neighbouring patch with lowest number of elmts */
    if(npatch){
      limit_search = 10;

      unassigned = -1;
      for(i = 0; i < p->npatch; ++i)
  if(flag[i] == -1){
    unassigned = i;
    break;
  }

      while((unassigned+1)&&limit_search--){
  for(i = unassigned; i < p->npatch; ++i){
    if(flag[i] == -1){

      /* check to see if there is a neighbouring assigned patch */
      /* and find neighbouring patch with lowest number of elements */
      trip = -1;
      start = p->nel;
      for(j = p->xadj[i]; j < p->xadj[i+1]; ++j){
        if(flag[p->adjncy[j]]+1){
    nadj = xelmt[flag[p->adjncy[j]]+1]-xelmt[flag[p->adjncy[j]]];
    if(nadj < start){
    start = nadj;
    trip = flag[p->adjncy[j]];
        }
        }
      }

      if(trip+1){
        /* add this patch to patch with smallest number of elements */
        nadj = p->xelmt[i+1] - p->xelmt[i];

        /* move all elmtid up by nadj */
        start = xelmt[npatch]-1;
        stop  = xelmt[trip+1];
        for(j = start; j >= stop; --j)
    elmtid[j+ nadj] = elmtid[j];

        /* copy in new elements */
        icopy(nadj,p->elmtid + p->xelmt[i],1,elmtid + xelmt[trip+1],1);

        /* increment xelmt */
        for(j = trip+1; j < npatch+1; ++j)
    xelmt[j] += nadj;

        flag[i] = trip;

        /* construct  adjacency of surround old patch id's */
        /* count number of surrounding patches that don't
     belong to trip */
        nadj = 0;
        for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
    if(flag[p->adjncy[j]] != npatch) ++nadj;

        /* shift adjacency list up by nadj */
        start = xadj[npatch]-1;
        stop  = xadj[trip+1];
        for(j = start; j >= stop; --j)
    adjncy[j+nadj] = adjncy[j];

        /* put new points in list */
        for(j = p->xadj[i]; j < p->xadj[i+1]; ++j)
    if(flag[p->adjncy[j]] != npatch)
      adjncy[stop++] = p->adjncy[j];

        /* increment xadj */
        for(j = trip+1; j < npatch+1; ++j)
    xadj[j] += nadj;
      }
    }
  }
  /* check for any unassigned patches */
  unassigned = -1;
  for(i = 0; i < p->npatch; ++i)
    if(flag[i] == -1){
      unassigned = i;
      break;
    }
    }

      if(limit_search == -1)
  error_msg(Vertex_connection: Failed to assign all patches);
    }

#ifdef PARALLEL
    if((npatch > 1)||(pllinfo[active_handle].nprocs > 1)){
#else
    if((npatch > 1)){
#endif
      /* finally construct new patch if npatch > 2. In this process we
   use flag to construct the adjancy interms of the new patch id's */
      /* if a parallel solve always generate a recursion */
      p->next = p1 = (Patch *)calloc(1,sizeof(Patch));
      p1->npatch   = npatch;
      p1->nel      = p->nel;
      p1->elmtid   = elmtid;
      p1->xelmt    = ivector(0,npatch);
      icopy(npatch+1,xelmt,1,p1->xelmt,1);

      p1->old2new  = flag;

      /* set up adjacency list in terms of new patch id's */
      p1->xadj = ivector(0,npatch);
      p1->adjncy = adjncy;

      p1->xadj[0] = 0;

      for(i = 0; i < npatch; ++i){
  ifill(npatch,-1,xelmt,1);
  for(j = xadj[i]; j < xadj[i+1]; ++j)
    xelmt[flag[adjncy[j]]] = flag[adjncy[j]];

  p1->xadj[i+1] = p1->xadj[i];
  for(j = 0; j < npatch; ++j)
    if((xelmt[j] != -1)&&(xelmt[j] != i)){
      p1->adjncy[p1->xadj[i+1]] = xelmt[j];
      p1->xadj[i+1]++;
    }
      }

      free(gvert); free(e2p);
      free(xelmt); free(xadj);
      return npatch;
    }
    else{
      free(gvert); free(e2p);
      free(flag); free(elmtid);
      free(xelmt); free(xadj);
      free(adjncy);
      return 0;
    }
  }
}


/* given a patch defined in terms of a graphs (using metis format)
   this function determines a coarser graph by using metis and the
   ration of partitions defined in iparam("IPartMetis") such that
   npatch/New_patch = Ipartmetis */

static int External_Partitioner(Patch *p){
  int npatch  = iparam("IPartMetis");

  npatch = p->npatch/npatch;

#ifdef PARALLEL
  if((npatch > 1)||((p->npatch > 1)&&(pllinfo[get_active_handle()].nprocs > 1))){
#else
  if((npatch > 1)||((p->npatch > 1))){
#endif
    register int i,j;
    int nadj;
    int *flag   = ivector(0,p->npatch-1);
    int *elmtid = ivector(0,p->nel-1);
    int *xelmt  = ivector(0,npatch);
    int *xadj   = ivector(0,npatch);
    int *adjncy = ivector(0,p->xadj[p->npatch]-1);
    int *tmp    = ivector(0,npatch);
    Patch *p1;
#if 0
    int opt[5];

    opt[0] = 0;
    opt[1] = max(100,p->npatch);
    opt[2] = 4;
    opt[3] = 1;
    opt[4] = 13;


    pmetis(p->npatch,p->xadj,p->adjncy,0,0,0,npatch,opt,0,&edgecut,flag);
#else
    error_msg(pmetis not called in External_Partitioner);
#endif
    /* sort out element list */
    xelmt[0] = 0;
    for(i = 0; i < npatch; ++i){
      xelmt[i+1] = xelmt[i];
      for(j = 0; j < p->npatch; ++j)
  if(flag[j] == i){
    nadj = p->xelmt[j+1] - p->xelmt[j];
    icopy(nadj,p->elmtid + p->xelmt[j],1,elmtid + xelmt[i+1],1);
    xelmt[i+1] += nadj;
  }
    }

    /* fill adjncy multiply defined  lists */
    xadj[0] = 0;
    for(i = 0; i < npatch; ++i){
      xadj[i+1] = xadj[i];
      for(j = 0; j < p->npatch; ++j)
  if(flag[j] == i){
    nadj = p->xadj[j+1] - p->xadj[j];
    icopy(nadj,p->adjncy + p->xadj[j],1,adjncy + xadj[i+1],1);
    xadj[i+1] += nadj;
  }
    }

    p->next = p1 = (Patch *)calloc(1,sizeof(Patch));
    p1->npatch   = npatch;
    p1->nel      = p->nel;
    p1->elmtid   = elmtid;
    p1->xelmt    = xelmt;
    p1->old2new  = flag;

    /* set up adjacency list in terms of new patch id's */
    p1->xadj     = xadj;
    p1->adjncy   = adjncy;

    /* get rid of multiply defined connectivities */
    p1->xadj[0] = 0;
    for(i = 0; i < npatch; ++i){
      ifill(npatch,-1,tmp,1);
      for(j = xadj[i]; j < xadj[i+1]; ++j)
  tmp[flag[adjncy[j]]] = flag[adjncy[j]];

      p1->xadj[i+1] = p1->xadj[i];
      for(j = 0; j < npatch; ++j)
  if((tmp[j] != -1)&&(tmp[j] != i)){
    p1->adjncy[p1->xadj[i+1]] = tmp[j];
    p1->xadj[i+1]++;
  }
    }

    free(tmp);
    return npatch;
  }
  else{
    return 0;
  }
}



/* given a list of elements stored in patch determine which vertex, edge
   and face gid's are on the interior or boundary of the next patch */
/* Note: this routine assumes that the unknown vertices have global
   id's which start from 0 and end at nv_solve (not true if
   bandwidthopt is called) */

static void Fill_Mlevel(Mlevelinfo *m, Patch *p, Element_List *E, Bsystem *B){
  register int i,j,k;
  int nlevel = 0,  cnt, eid;
  int *patch,gid,*ptchid;
  int nvs = B->nv_solve;
  int *vflag = ivector(0,nvs), *vmask = ivector(0,nvs);
  int nes = B->ne_solve;
  int *eflag = ivector(0,nes), *emask = ivector(0,nes);
  int active_handle = get_active_handle();

  int nfs, *fflag, *fmask;
  if(eDIM == 3){
    nfs = B->nf_solve;
    fflag = ivector(0,nfs);
    fmask = ivector(0,nfs);
  }

  Edge *edg;
  VtoE *v2e,*v1;
  Element *U;

  ifill(nvs,-1,vflag,1);
  ifill(nes,-1,eflag,1);
  if(eDIM == 3)
    ifill(nfs,-1,fflag,1);

  // GAVE UP HERE

  Get_V2E(&v2e,&nvs);

  patch  = ivector(0,p->nel-1);
  ptchid = ivector(0,p->npatch-1);

  /* set up all point along the boundary to have a vflag of -2 so
     that they can be place in the inner most patch */
  DO_PARALLEL{
    if(eDIM == 2){
      for(i = 0; i < pllinfo[active_handle].npface; ++i){
  eid = pllinfo[active_handle].npfeid[i];
  for(j = 0; j < 2; ++j){
    gid = E->flist[eid]->
      vert[E->flist[eid]->vnum(pllinfo[active_handle].npfside[i],j)].gid;
    if(gid < nvs) vflag[gid] = -2;
  }
  eflag[E->flist[eid]->edge[pllinfo[active_handle].npfside[i]].gid] = -2;
      }
    }
    else{
      for(i = 0; i < pllinfo[active_handle].npface; ++i){
  eid = pllinfo[active_handle].npfeid[i];
  for(j = 0; j < 3; ++j){
    gid = E->flist[eid]->
      vert[E->flist[eid]->vnum(pllinfo[active_handle].npfside[i],j)].gid;
    if(gid < nvs) vflag[gid] = -2;
  }
  for(j = 0; j < 3; ++j){
    gid = E->flist[eid]->
      edge[E->flist[eid]->ednum(pllinfo[active_handle].npfside[i],j)].gid;
    if(gid < nes) eflag[gid] = -2;
  }

  fflag[E->flist[eid]->face[pllinfo[active_handle].npfside[i]].gid] = -2;
      }
    }
  }

  while(p){
    m[nlevel].Int = (GidInfo *)calloc(p->npatch,sizeof(GidInfo));
    m[nlevel].Bdy = (GidInfo *)calloc(p->npatch,sizeof(GidInfo));

    m[nlevel].npatch  = p->npatch;
    m[nlevel].old2new = p->old2new;

    /* set up a list of which patch the elements belong to */
    for(i = 0; i < p->npatch; ++i)
      for(j = p->xelmt[i]; j < p->xelmt[i+1]; ++j)
  patch[p->elmtid[j]] = i;

    ifill(nvs,1,vmask,1);
    ifill(nes,1,emask,1);
    if(eDIM == 3)
      ifill(nfs,1,fmask,1);

    for(i = 0; i < p->nel; ++i){
      U = E->flist[i];
      for(j = 0; j < U->Nverts; ++j)
  /* is vertex to be solved for? */
  if((gid = U->vert[j].gid) < nvs){
    /* check: vflag to see if vertex belongs to the interior of a
       previous patch or on the boundary of a parallel patch
       check vmask to stop vertex being visited more than once */
    if(vmask[gid]&&(vflag[gid] < 0)){

      izero(p->npatch,ptchid,1);
      for(v1 = v2e[gid].next;v1; v1 = v1->next){
        ptchid[patch[v1->eid]] = 1;
      }

      /* determine which patches this point is in */
      if(vflag[gid] == -2){ /* point is on a parallel patch
             and should be on boundary */
        cnt = 2;
      }
      else{
        cnt = isum(p->npatch,ptchid,1);
      }

      switch(cnt){
      case 1: /* if only one surrounding patch then point is interior */
        vflag[gid] = nlevel;
        cnt = patch[v2e[gid].next->eid];
        m[nlevel].Int[cnt].nvert++;
        m[nlevel].Int[cnt].vgid = (int *)realloc(m[nlevel].Int[cnt].vgid,
          m[nlevel].Int[cnt].nvert*sizeof(int));
        m[nlevel].Int[cnt].vgid[m[nlevel].Int[cnt].nvert-1] = gid;
        break;
      default: /* else point is on the boundary of patches */
        vmask[gid] = 0;
        for(k = 0; k < p->npatch; ++k)
    if(ptchid[k]){
      m[nlevel].Bdy[k].nvert++;
      m[nlevel].Bdy[k].vgid = (int *)realloc(m[nlevel].Bdy[k].vgid,
            m[nlevel].Bdy[k].nvert*sizeof(int));
      m[nlevel].Bdy[k].vgid[m[nlevel].Bdy[k].nvert-1] = gid;
    }
        break;
      }
    }
      }

      for(j = 0; j < U->Nedges; ++j)
  /* is edge to be solved for? */
  if((gid = U->edge[j].gid) < nes){
    /* check: eflag to see if edge belongs to the interior of a
       previous patch or on the boundary of a parallel patch
       check emask to stop edge being visited more than once */
    if(emask[gid]&&(eflag[gid] < 0)){


      if(eflag[gid] == -2){ /* point is on a parallel patch
             and should be on boundary */
        izero(p->npatch,ptchid,1);
        if(eDIM == 2)
    ptchid[patch[U->edge[j].eid]] = 1;
        else
    for(edg = U->edge[j].base;edg; edg = edg->link){
      ptchid[patch[edg->eid]] = 1;
    }
        cnt = 2;
      }
      else if(!U->edge[j].base){ /* then it is just a single edge */
        cnt = 1;
      }
      else{
        /* determine which patches this point is in */
        izero(p->npatch,ptchid,1);
        for(edg = U->edge[j].base;edg; edg = edg->link){
    ptchid[patch[edg->eid]] = 1;
        }
        cnt = isum(p->npatch,ptchid,1);
      }

      switch(cnt){
      case 1: /* if only one surrounding patch then point is interior */
        eflag[gid] = nlevel;
        cnt = patch[U->edge[j].eid];
        m[nlevel].Int[cnt].nedge++;
        m[nlevel].Int[cnt].egid = (int *)realloc(m[nlevel].Int[cnt].egid,
          m[nlevel].Int[cnt].nedge*sizeof(int));
        m[nlevel].Int[cnt].egid[m[nlevel].Int[cnt].nedge-1] = gid;
        break;
      default: /* else point is on the boundary of patches */
        emask[gid] = 0;
        for(k = 0; k < p->npatch; ++k)
    if(ptchid[k]){
      m[nlevel].Bdy[k].nedge++;
      m[nlevel].Bdy[k].egid = (int *)realloc(m[nlevel].Bdy[k].egid,
            m[nlevel].Bdy[k].nedge*sizeof(int));
      m[nlevel].Bdy[k].egid[m[nlevel].Bdy[k].nedge-1] = gid;
    }
        break;
      }
    }
  }

      if(eDIM == 3){
  for(j = 0; j < U->Nfaces; ++j)
    if((gid = U->face[j].gid) < nfs){ /* is face to be solved for? */
    /* check: fflag to see if face belongs to the interior of a
       previous patch or on the boundary of a parallel patch
       check fmask to stop face being visited more than once */
    if(fmask[gid]&&(fflag[gid] < 0)){
      if(fflag[gid] == -2){ /* point is on a parallel patch
             and should be on boundary */
        cnt = 2;
      }
      else if(!U->face[j].link){ /* then it is just a single edge */
        cnt = 1;
      }
      else {
        /* determine which patches this point is in */
        if(patch[U->face[j].eid] == patch[U->face[j].link->eid])
    cnt = 1;
        else
    cnt = 2;
      }

      switch(cnt){
      case 1: /* if only one surrounding patch then point is interior */
        fflag[gid] = nlevel;
        cnt = patch[U->face[j].eid];
        m[nlevel].Int[cnt].nface++;
        m[nlevel].Int[cnt].fgid = (int*) realloc(m[nlevel].Int[cnt].fgid,
          m[nlevel].Int[cnt].nface*sizeof(int));
        m[nlevel].Int[cnt].fgid[m[nlevel].Int[cnt].nface-1] = gid;
        break;
      default: /* else point is on the boundary of patches */
        fmask[gid] = 0;
        k = patch[i];
        m[nlevel].Bdy[k].nface++;
        m[nlevel].Bdy[k].fgid = (int*)realloc(m[nlevel].Bdy[k].fgid,
              m[nlevel].Bdy[k].nface*sizeof(int));
        m[nlevel].Bdy[k].fgid[m[nlevel].Bdy[k].nface-1] = gid;
        if(U->face[j].link){
    k = patch[U->face[j].link->eid];
    m[nlevel].Bdy[k].nface++;
    m[nlevel].Bdy[k].fgid = (int*)realloc(m[nlevel].Bdy[k].fgid,
                m[nlevel].Bdy[k].nface*sizeof(int));
    m[nlevel].Bdy[k].fgid[m[nlevel].Bdy[k].nface-1] = gid;
        }
        break;
      }
    }
  }
    }
    }
    nlevel++;
    p = p->next;
  }

  free(vflag); free(vmask); free(eflag); free(emask);
  free(patch); free(ptchid);
  if(eDIM == 3){
    free(fflag); free(fmask);
  }

}

static void free_patch_list(Patch *p){
  Patch *p1;

  while(p){
    p1 = p->next;
    free(p->xadj);
    free(p->adjncy);
    free(p->xelmt);
    free(p->elmtid);
    free(p);
    p = p1;
  }
}

/* construct a elemental connectivity list for each element */
static Fctcon *SCFacetConnect(int nsols, Mlevelinfo *ml, int *vmap,
            int *emap, int *fmap)
{
  register int i,k;
  const    int npatch = ml->npatch;
  int        *pts, n;
  Fctcon     *connect;

  connect = (Fctcon *)calloc(nsols,sizeof(Fctcon));

  pts = ivector(0,nsols-1);

  for(k = 0; k < npatch; ++k){
    /* find all facets that are unknowns */
    n = 0;
    for(i = 0; i < ml->Bdy[k].nvert; ++i)
      pts[n++] = vmap[ml->Bdy[k].vgid[i]];

    for(i = 0; i < ml->Bdy[k].nedge; ++i)
      pts[n++] = emap[ml->Bdy[k].egid[i]];

    if(eDIM == 3)
      for(i = 0; i < ml->Bdy[k].nface; ++i)
  pts[n++] = fmap[ml->Bdy[k].fgid[i]];

    addfct(connect,pts,n);
  }

  free(pts);

  return connect;
}

#ifdef MLEV_MEMORY
static void memory(Element_List *E, Bsystem *Ubsys){
  register int i,j;
  int memory = 0, mem1;
  int nrecur = Ubsys->rslv->nrecur;
  Recur *r = Ubsys->rslv->rdata;
  int  nsolve = r[nrecur-1].cstart;

  mem1 = 0;
  for(i = 0 ; i < nrecur; ++i)
    for(j = 0; j < r[i].npatch; ++j)
      mem1 += r[i].patchlen_c[j]*(r[i].patchlen_c[j]+1)/2;
  memory += mem1;

  fprintf(stdout,"Interior systems size: %d \n",mem1);

  mem1 = 0;
  for(i = 0 ; i < nrecur; ++i)
    for(j = 0; j < r[i].npatch; ++j)
      mem1 += r[i].patchlen_c[j]*r[i].patchlen_a[j];

  memory += mem1;
  fprintf(stdout,"Coupling systems size: %d \n",mem1);


  mem1 = 0;
  for(i = 0; i < r[nrecur-1].npatch; ++i)
    mem1 +=r[nrecur-1].patchlen_a[i]*(r[nrecur-1].patchlen_a[i]+1)/2;
  memory += mem1;
  fprintf(stdout,"Boundary systems size: %d (rank=%d : %d)\n",mem1,nsolve,
    nsolve*(nsolve+1)/2);

  fprintf(stdout,"\nTotal doubles (mlevel)= %d\n", memory);
  fprintf(stdout,"\nTotal doubles (orig  )= %d\n",
    Ubsys->nel*E->fhead->Nbmodes*(E->fhead->Nbmodes+1)/2);
}
#endif

#ifdef PRINT_MLEVEL
static void Print_Patch(Element_List *E, Patch *p, FILE *out){
  register int i,j,k;
  int    cnt=0,nadj;
  double Xmax[eDIM], Xmin[eDIM], Xshift[eDIM];
  double Xmesh[eDIM];
  double alpha = dparam("Xalpha"), beta = dparam("Xbeta"),r,rnew;
  Element *U;

  if(!alpha) alpha = 2.0;
  if(!beta)  beta  = 1.0;

  Xmesh[0] = dparam("XC");
  Xmesh[1] = dparam("YC");
  if(eDIM == 3){
    Xmesh[2] = dparam("ZC");
    error_msg(Hybrid code needs sorting out);
    fprintf(out,"VARIABLES = x y z p\n");
  }
  else
    fprintf(out,"VARIABLES = x y p\n");

  for(i = 0; i < p->npatch; ++i){

    dfill(DIM,-100000,Xmax,1);
    dfill(DIM, 100000,Xmin,1);

    /* find max-min x y and z */
    for(j = p->xelmt[i]; j < p->xelmt[i+1]; ++j){
      U = E->flist[p->elmtid[j]];
      for(k = 0; k < U->Nverts; ++k){
  Xmax[0] = max(Xmax[0],U->vert[k].x);
  Xmin[0] = min(Xmin[0],U->vert[k].x);
  Xmax[1] = max(Xmax[1],U->vert[k].y);
  Xmin[1] = min(Xmin[1],U->vert[k].y);
  if(eDIM == 3){
    Xmax[2] = max(Xmax[2],U->vert[k].z);
    Xmin[2] = min(Xmin[2],U->vert[k].z);
  }
      }
    }


    /* find partition radius from mesh centre */
    r = 0.0;
    for(j = 0; j < eDIM; ++j){
      Xshift[j] = 0.5*(Xmax[j] + Xmin[j]) - Xmesh[j];
      r += Xshift[j]*Xshift[j];
    }

    /* scale r */
    rnew = alpha*pow(sqrt(r),beta);

    /* determine Xshift for this patch */
    for(j = 0; j <e DIM; ++j)
      Xshift[j] =(r)? (rnew/r-1.0)*Xshift[j]: 0.0;

    /* write out mesh based on shifted location */
    nadj = p->xelmt[i+1] - p->xelmt[i];
    if(eDIM == 3)
      fprintf(out,"ZONE T=\"zone %d\", N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n",
        cnt++,4*nadj,nad)j;
    else
      fprintf(out,"ZONE T=\"zone %d\", N=%d, E=%d, F=FEPOINT, ET=QUADRILATERAL\n",
        cnt++,E->flist[p->elmtid[j]]->Nverts*nadj,nadj);

    for(j = p->xelmt[i]; j < p->xelmt[i+1]; ++j){
      U = E->flist[p->elmtid[j]];
      if (eDIM == 2){
  for(k = 0; k < U->Nverts; ++k)
    fprintf(out,"%lf %lf %d\n",U->vert[k].x,U->vert[k].y,i+1);
  if(U->Nverts == 3)
    fprintf(out,"%lf %lf %d\n",U->vert[2].x,U->vert[2].y,i+1);
      }
      else{
  fprintf(out,"%lf %lf %lf %d\n",U->vert[0].x+Xshift[0],U->vert[0].y+Xshift[1]
    ,U->vert[0].z+Xshift[2],i+1);
  fprintf(out,"%lf %lf %lf %d\n",U->vert[1].x+Xshift[0],U->vert[1].y+Xshift[1]
    ,U->vert[1].z+Xshift[2],i+1);
  fprintf(out,"%lf %lf %lf %d\n",U->vert[2].x+Xshift[0],U->vert[2].y+Xshift[1]
    ,U->vert[2].z+Xshift[2],i+1);
  fprintf(out,"%lf %lf %lf %d\n",U->vert[3].x+Xshift[0],U->vert[3].y+Xshift[1]
    ,U->vert[3].z+Xshift[2],i+1);
      }
    }

    for(k = p->xelmt[i],j=0; k < p->xelmt[i+1]; ++k,++j){
      if (DIM == 2){
  fprintf(out,"%d %d %d %d\n",4*j+1,4*j+2,4*j+3,4*j+4);
      }
      else{
  fprintf(out,"%d %d %d %d\n",U->Nvert*j+1,U->Nvert*j+2,U->Nvert*j+3,
    U->Nvert+j*4);
      }
    }
  }
}
#endif
