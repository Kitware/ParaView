/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/bwoptim.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <math.h>
#include <veclib.h>
#include "nektar.h"


static void    RenumberElmts (Element *E, Bsystem *Bsys, int *newmap,
            char trip);
static Fctcon *FacetConnect  (Bsystem *B, Element *E, char trip);


/* this routine optimises the ordering for the unknown degrees of freedom
   it assume that the knowns have already been stored.
   Note: asumes that the unknown vertices are have gid's 0 < gid < nvs
   Note: asumes that the unknown edges    are have gid's 0 < gid < nes
   Note: asumes that the unknown facess   are have gid's 0 < gid < nfs */

void bandwidthopt(Element *E, Bsystem *Bsys, char trip){
  int     nsols, *newmap;
  Fctcon *ptcon;

 if(trip == 'v') /* just do vertex space */
    nsols = Bsys->nv_solve;
  else {
    if(E->dim() == 2)
      nsols = Bsys->nv_solve + Bsys->ne_solve;
    else
      nsols = Bsys->nv_solve + Bsys->ne_solve + Bsys->nf_solve;
  }

  if(nsols < 2) return;

  newmap   = ivector(0,nsols-1);

  /* Initially set up "point" to "point" connections (a point/facet is
     a vertex, edge or face) */
  ptcon = FacetConnect(Bsys,E,trip);

  /* Given a list of "nsols" points whose interconnectivity is given in
     ptcon this routine returns a new mapping in newmap which contains
     the order in which the unknowns should be stored                   */
  MinOrdering(nsols,ptcon,newmap);

  /* Sort out the numbering according to new ordering in newmap */
  RenumberElmts(E, Bsys, newmap, trip);

  free_Fctcon(ptcon,nsols);

  free(newmap);
}

static int Cuthill(Fctcon *ptcon, int *newmap,
       int *initmap, int nsols, int initpt);
void MinOrdering(int nsols, Fctcon *ptcon, int *newmap){
  register int i,j,k,n;
  int      firstv, *initmap;

  /********** Cuthill-McKee optimisation *********/

  initmap  = ivector(0, nsols-1);
  for(i = 0; i < nsols; ++i) initmap[i] = i;

  /* to optimise search try algorithm from all points that have
     a minimum number of connecting points */

  /* order list so that the points with least degree are first */
  for(i = 0; i < nsols; ++i){
    k = ptcon[initmap[0]].ncon;
    n = 0;
    for(j = 1; j < nsols-i; ++j)
      if(ptcon[initmap[j]].ncon > k){
  n = j;
  k = ptcon[initmap[j]].ncon;
      }
    k = initmap[n];
    initmap[n] = initmap[nsols-i-1];
    initmap[nsols-i-1] = k;
  }

  /* perform reorganisation for each peripheral vertex */

  /* find number of points with minimum connectivity */
  for(n = 0; n < nsols; ++n)
    if(ptcon[initmap[n]].ncon != ptcon[initmap[0]].ncon)
      break;

  /* find Cuthill with mimimum bandwidth using points with min. connect */
  for(firstv = 0, k = nsols, i = 0; i < n; ++i){
    j = Cuthill(ptcon,newmap,initmap,nsols,i);
    if(j < k){
      k = j; firstv = i;
    }
  }

  /* finally perform optimal reorganisation */
  Cuthill(ptcon,newmap,initmap,nsols,firstv);

  free(initmap);
}


static int Cuthill(Fctcon *ptcon, int *newmap, int *initmap, int nsols,
       int initpt){
  register int i,j,k;
  int      bw=0,start=0,end=1;
  int      cnt,len,jtmp,beg;
  int      next,cand,*mark;
  Facet    *f;

  if(nsols <= 1) return bw;

  mark = ivector(0,nsols);

  ifill(nsols,1,mark,1);
  izero(nsols,newmap,1);

  cnt = 1;
  newmap[0] = initmap[initpt];
  mark[initmap[initpt]]=0;

  while(cnt < nsols){
    for(i = start; i < end; ++i){
      next = newmap[i];
      beg = cnt;
      for(f = ptcon[next].f; f; f = f->next){
  cand = f->id;
  if(mark[cand]){
    mark[cand]   = 0;
    newmap[cnt] = cand;
    ++cnt;
  }
      }
      for(j = beg; j < cnt; ++j){
  len  = ptcon[newmap[beg]].ncon;
  jtmp = beg;
  for(k = beg; k < cnt+beg-j; ++k){
    if(ptcon[newmap[k]].ncon > len){
      len  = ptcon[newmap[k]].ncon;
      jtmp = k;
    }
  }
  k  = newmap[jtmp];
  newmap[jtmp] = newmap[cnt+beg-j-1];
  newmap[cnt+beg-j-1] = k;
      }
      if((cnt-beg) > bw) bw = cnt-beg;
    }
    start= end;
    end  = cnt;

    /* check to see if the tree leaves have been reached
       and if so start a new tree */
    if(start == end){
      i = 0;
      while(!mark[initmap[i]]) ++i;
      newmap[cnt] = initmap[i];
      mark[initmap[i]]=0;
      start = cnt;
      end   = start+1;
      ++cnt;
    }
  }
  free(mark);

  return bw;
}

static void   RenumberElmts(Element *E, Bsystem *Bsys, int *newmap, char trip){
  register int i,k,n;
  int      *gbmap, *length, *sort;
  int      nsols;
  Element  *F;

  if(trip == 'v'){
    const    int nel = Bsys->nel, nvs = Bsys->nv_solve;

    nsols = nvs ;
    gbmap = ivector(0,nsols-1);

    /* need to unshuffles data to store */
    for(i = 0; i < nsols; ++i)
      gbmap[newmap[i]] = i;

    /* replace new vertex gid's */
    for(F=E; F; F = F->next)
      for(i = 0; i < F->Nverts; ++i)
  if((n = F->vert[i].gid) < nvs)
    F->vert[i].gid = gbmap[n];

    free(gbmap);
  }
  else{
    const    int nel = Bsys->nel, nvs = Bsys->nv_solve, nes = Bsys->ne_solve;
    const    int nfs = Bsys->nf_solve;

    nsols = nvs + nes;

    if(E->dim() == 3)
      nsols += nfs;

    length   = ivector(0,nsols-1);
    gbmap    = ivector(0,nsols-1);
    sort     = ivector(0,nsols-1);

    /* work out global positions and copy into permanent storage */
    ifill(nvs,1,length,1);

    for(F=E;F;F=F->next){
      for(i = 0; i < F->Nedges; ++i)
  if((n = F->edge[i].gid) < nes)
    length[nvs+n] = F->edge[i].l;

      // needs to be fixed
      // look for face[.].l
      if(F->dim() == 3)
  for(i = 0; i < F->Nfaces; ++i)
    if((n = F->face[i].gid)< nfs)
      length[nvs+nes+n] = (F->Nfverts(i) == 3) ? F->face[i].l*(1+F->face[i].l)/2 : F->face[i].l*F->face[i].l;

    }

    for(i = 0, n = 0; i < nsols; ++i){
      sort[i] = n;
      n += length[newmap[i]];
    }

    /* need to unshuffles data to store */
    for(i = 0; i < nsols; ++i)
      gbmap[newmap[i]] = sort[i];


    /* replace new vertex gid's */
    for(F=E;F;F=F->next)
      for(i = 0; i < F->Nverts; ++i)
  if((n = F->vert[i].gid) < nvs)
    F->vert[i].gid = gbmap[n];

    /* replace new edge locations */
    icopy(nes, gbmap + nvs, 1, Bsys->edge, 1);

    if(E->dim() == 3)
      /* replace new face locations */
      icopy(nfs, gbmap + nvs+nes, 1, Bsys->face, 1);

    free(length); free(sort); free(gbmap);
  }
}

/* setup list of connectivity of all connecting points */

static Fctcon *FacetConnect(Bsystem *B, Element *E, char trip){
  register int i,k;
  int  nel = B->nel,*pts, n;
  Fctcon    *connect;
  Element  *F;

  if(trip == 'v'){
    const int  nvs = B->nv_solve;

    pts = ivector(0,Max_Nverts-1);

    connect = (Fctcon *)calloc(nvs,sizeof(Fctcon));

    for(F=E;F;F=F->next){
      /* find all facets that are unknowns */
      n = 0;

      for(i = 0; i < F->Nverts; ++i)
  if(F->vert[i].gid < nvs)
    pts[n++] = F->vert[i].gid;

      addfct(connect,pts,n);
    }
  }
  else{
    int nvs = B->nv_solve, nes = B->ne_solve, nfs = B->nf_solve;
    int nsols = nvs + nes;

    if(E->dim() == 3)
      nsols += nfs;

    pts = ivector(0,Max_Nverts + Max_Nedges + Max_Nfaces - 1);

    connect = (Fctcon *)calloc(nsols,sizeof(Fctcon));

    for(F=E;F;F=F->next){
      /* find all facets that are unknowns */
      n = 0;
      for(i = 0; i < F->Nverts; ++i)
  if(F->vert[i].gid < nvs)
    pts[n++] = F->vert[i].gid;

      for(i = 0; i < F->Nedges; ++i)
  if(F->edge[i].gid < nes)
    pts[n++] = F->edge[i].gid + nvs;

      if(F->dim() == 3)
  for(i = 0; i < F->Nfaces; ++i)
    if(F->face[i].gid < nfs)
      pts[n++] = F->face[i].gid + nvs + nes;

      addfct(connect,pts,n);
    }
  }
  free(pts);
  return connect;
}


/* assemble connectivity for all points in list pts and put in connect */
void addfct(Fctcon *con, int *pts, int n){
  register int i,j;
  int      trip;
  Facet    *f,*f1;

  for(i = 0; i < n; ++i){
    f = con[pts[i]].f;
    for(j = 0; j < n; ++j){
      if(i == j) continue;

      /* check to see if this points is already included */
      for(trip = 1,f1 = f; f1; f1 = f1->next)
  if(f1->id == pts[j]) trip = 0;

      /* if this point isn't already in list then add it */
      if(trip){
  f1 = (Facet *)malloc(sizeof(Facet));
  f1->id = pts[j];
  f1->next = con[pts[i]].f;
  con[pts[i]].f = f1;
  con[pts[i]].ncon++;
      }
    }
  }
}

void  free_Fctcon(Fctcon *con, int n){
  register int i;
  Facet *f,*f1;

  for(i = 0; i < n; ++i)
    f = con[i].f;
    while(f){
      f1 = f->next;
      free(f);
      f = f1;
    }

  free(con);
}
