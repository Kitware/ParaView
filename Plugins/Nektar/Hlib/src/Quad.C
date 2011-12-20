/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission f the author.                     //
//                                                                        //
/**************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <polylib.h>
#include "veclib.h"
#include "hotel.h"
#include "nekstruct.h"

#include <stdio.h>

static double Quad_Penalty_Fac = 1.;

Quad::Quad() : Element(){
  Nverts = 4;
  Nedges = 4;
  Nfaces = 1;
}

Quad::Quad(Element *E){

  if(dparam("DPENFAC"))
    Quad_Penalty_Fac = dparam("DPENFAC");

  if(!Quad_wk)
    Quad_work();

  id      = E->id;
  type    = E->type;
  state   = 'p';
  Nverts = 4;
  Nedges = 4;
  Nfaces = 1;

  vert    = (Vert *)calloc(Nverts,sizeof(Vert));
  edge    = (Edge *)calloc(Nedges,sizeof(Edge));
  face    = (Face *)calloc(Nfaces,sizeof(Face));
  lmax    = E->lmax;
  interior_l       = E->interior_l;
  Nmodes  = E->Nmodes;
  Nbmodes = E->Nbmodes;
  qa      = E->qa;
  qb      = E->qb;
  qc      = 0;

  qtot    = qa*qb;

  memcpy(vert,E->vert,Nverts*sizeof(Vert));
  memcpy(edge,E->edge,Nedges*sizeof(Edge));
  memcpy(face,E->face,Nfaces*sizeof(Face));

  /* set memory */
  vert[0].hj = (double*)  0;
  face[0].hj = (double**) 0;
  h          = (double**) 0;

  curve  = E->curve;
  curvX  = E->curvX;
  geom   = E->geom;
  dgL    = E->dgL;
}


Quad::Quad(int i_d, char ty, int L, int Qa, int Qb, int Qc, Coord *X){
  int i;

  if(!Quad_wk)
    Quad_work();

  Qc = Qc; // compiler fix
  id = i_d;
  type = ty;  state = 'p';
  Nverts = 4;
  Nedges = 4;
  Nfaces = 1;

  vert = (Vert *)calloc(Nverts,sizeof(Vert));
  edge = (Edge *)calloc(Nedges,sizeof(Edge));
  face = (Face *)calloc(Nfaces,sizeof(Face));
  lmax = L;
  interior_l    = 0;
  Nmodes  = Nverts + Nedges*(L-2) + Nfaces*(L-2)*(L-2);
  Nbmodes = Nmodes - (L-2)*(L-2);
  qa      = Qa;
  qb      = Qb;
  qc      = 0;

  qtot    = qa*qb;

  /* set vertex solve mask to 1 by default */
  for(i = 0; i < Nverts; ++i){
    vert[i].id    = i;
    vert[i].eid   = id;
    vert[i].solve = 1;
    vert[i].x     = X->x[i];
    vert[i].y     = X->y[i];
  }

  /* construct edge system */
  for(i = 0; i < Nedges; ++i){
    edge[i].id  = i;
    edge[i].eid = id;
    edge[i].l   = L-2;
  }

  /* construct face system */
  for(i = 0; i < Nfaces; ++i){
    face[i].id  = i;
    face[i].eid = id;
    face[i].l   = max(0,L-2);
  }

  /* set memory */
  vert[0].hj = (double*)  0;
  face[0].hj = (double**) 0;
  h          = (double**) 0;

  curve  = (Curve*) calloc(1,sizeof(Curve));
  curve->face = -1;
  curve->type = T_Straight;
  curvX  = (Cmodes*)0;
}

Quad::Quad(int , char , int *, int *, Coord *){
}



// ============================================================================
#if 0
void Quad::PSE_Mat(Element *E, Metric *lambda, LocMat *pse, double *DU){

  double *save = dvector(0, qtot+Nmodes-1);
  dcopy(qtot, h[0], 1, save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save+qtot, 1);
  char orig_state = state;

  register int i,j,n;
  int nbl, L, N, Nm, qt, asize = pse->asize, csize = pse->csize;
  Basis   *B = E->getbasis();
  double *Eh;
  double **a = pse->a;
  double **b = pse->b;
  double **c = pse->c;
  double **d = pse->d;

  // This routine is placed within the for loop for each element
  // at around line 30 in the code. Therefore, this is within an element

  nbl = E->Nbmodes;
  N   = E->Nmodes - nbl;
  Nm  = E->Nmodes;
  qt  = E->qa*E->qb;

  Eh  = dvector(0, qt - 1);    // Temporary storage for E->h[0] ------
  dcopy(qt, E->h[0], 1, Eh, 1);

  // Fill A and D ----------------------------------------------------

  for(i = 0, n = 0; i < E->Nverts; ++i, ++n){
    E->fillElmt(B->vert + i);

    // ROUTINE THAT DOES INNERPRODUCT AND EVALUATES TERM--------------
    dvmul(qt, DU, 1, E->h[0], 1, E->h[0], 1);
    E->Iprod(E);
    // ---------------------------------------------------------------

    dcopy(nbl, E->vert->hj, 1, *a + n, asize);
    dcopy(N,   E->vert->hj + nbl, 1, d[n], 1);
  }

  dcopy(qt, Eh, 1, E->h[0], 1);

  for(i = 0; i < E->Nedges; ++i){
    for(j = 0; j < (L = E->edge[i].l); ++j, ++n){
      E->fillElmt(B->edge[i] + j);

      // ROUTINE THAT DOES INNERPRODUCT AND EVALUATES TERM------------
      dvmul(qt, DU, 1, E->h[0], 1, E->h[0], 1);
      E->Iprod(E);
      // -------------------------------------------------------------

      dcopy(nbl, E->vert->hj, 1, *a + n, asize);
      dcopy(N,   E->vert->hj + nbl, 1, d[n], 1);
    }
  }

  dcopy(qt, Eh, 1, E->h[0], 1);

  // Fill B and C ----------------------------------------------------

  L = E->face->l;
  for(i = 0, n = 0; i < L; ++i){
    for(j = 0; j < L - i; ++j, ++n){
      E->fillElmt(B->face[0][i]+j);

      // ROUTINE THAT DOES INNERPRODUCT AND EVALUATES TERM-----------
      dvmul(qt, DU, 1, E->h[0], 1, E->h[0], 1);
      E->Iprod(E);
      // ------------------------------------------------------------

      dcopy(nbl, E->vert->hj    , 1, *b + n, csize);
      dcopy(N, E->vert->hj + nbl, 1, *c + n, csize);
    }
  }

  // -----------------------------------------------------------------

  free(Eh);

  state = orig_state;

  dcopy(qtot, save, 1, h[0], 1);
  dcopy(Nmodes, save+qtot, 1, vert[0].hj, 1);
  free(save);

}
#endif

// ============================================================================
void Quad::PSE_Mat(Element *E, LocMat *pse, double *DU){

  double *save = dvector(0, qtot+Nmodes-1);
  dcopy(qtot, h[0], 1, save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save+qtot, 1);
  char orig_state = state;

  register int i,j,n;
  int nbl, L, N, Nm, qt, asize = pse->asize, csize = pse->csize;
  Basis   *B = E->getbasis();
  double *Eh;
  double **a = pse->a;
  double **b = pse->b;
  double **c = pse->c;
  double **d = pse->d;

  // This routine is placed within the for loop for each element
  // at around line 30 in the code. Therefore, this is within an element

  nbl = E->Nbmodes;
  N   = E->Nmodes - nbl;
  Nm  = E->Nmodes;
  qt  = E->qa*E->qb;

  Eh  = dvector(0, qt - 1);    // Temporary storage for E->h[0] ------
  dcopy(qt, E->h[0], 1, Eh, 1);

  // Fill A and D ----------------------------------------------------

  for(i = 0, n = 0; i < E->Nverts; ++i, ++n){
    E->fillElmt(B->vert + i);

    // ROUTINE THAT DOES INNERPRODUCT AND EVALUATES TERM--------------
    dvmul(qt, DU, 1, E->h[0], 1, E->h[0], 1);
    E->Iprod(E);
    // ---------------------------------------------------------------

    dcopy(nbl, E->vert->hj, 1, *a + n, asize);
    dcopy(N,   E->vert->hj + nbl, 1, d[n], 1);
  }

  dcopy(qt, Eh, 1, E->h[0], 1);

  for(i = 0; i < E->Nedges; ++i){
    for(j = 0; j < (L = E->edge[i].l); ++j, ++n){
      E->fillElmt(B->edge[i] + j);

      // ROUTINE THAT DOES INNERPRODUCT AND EVALUATES TERM------------
      dvmul(qt, DU, 1, E->h[0], 1, E->h[0], 1);
      E->Iprod(E);
      // -------------------------------------------------------------

      dcopy(nbl, E->vert->hj, 1, *a + n, asize);
      dcopy(N,   E->vert->hj + nbl, 1, d[n], 1);
    }
  }

  dcopy(qt, Eh, 1, E->h[0], 1);

  // Fill B and C ----------------------------------------------------

  L = E->face->l;
  for(i = 0, n = 0; i < L; ++i){
    for(j = 0; j < L - i; ++j, ++n){
      E->fillElmt(B->face[0][i]+j);

      // ROUTINE THAT DOES INNERPRODUCT AND EVALUATES TERM-----------
      dvmul(qt, DU, 1, E->h[0], 1, E->h[0], 1);
      E->Iprod(E);
      // ------------------------------------------------------------

      dcopy(nbl, E->vert->hj    , 1, *b + n, csize);
      dcopy(N, E->vert->hj + nbl, 1, *c + n, csize);
    }
  }

  // -----------------------------------------------------------------

  free(Eh);

  state = orig_state;

  dcopy(qtot, save, 1, h[0], 1);
  dcopy(Nmodes, save+qtot, 1, vert[0].hj, 1);
  free(save);
}

// ============================================================================
void Quad::BET_Mat(Element *P, LocMatDiv *bet, double *beta, double *sigma){
  register int i,j,n;
  const    int nbl = Nbmodes, N = Nmodes - Nbmodes;
  int      L;
  Basis   *b = getbasis();
  double **dxb = bet->Dxb,   // MSB: dx corresponds to bar(beta)
         **dxi = bet->Dxi,
         **dyb = bet->Dyb,   // MSB: dy corresponds to sigma
         **dyi = bet->Dyi;
  char orig_state = state;

  /* fill boundary systems */
  for(i = 0,n=0; i < Nverts; ++i,++n){
    fillElmt(b->vert+i);

    dvmul(qtot,beta,1,*h,1,*P->h,1);
#ifndef PCONTBASE
    P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
    P->Iprod(P);
#endif
    dcopy(P->Nmodes,P->vert->hj,1,*dxb + n,nbl);

    dvmul(qtot,sigma,1,*h,1,*P->h,1);
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

      dvmul(qtot,beta,1,*h,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dxb + n,nbl);

      dvmul(qtot,sigma,1,*h,1,*P->h,1);
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

      dvmul(qtot,beta,1,*h,1,*P->h,1);
#ifndef PCONTBASE
      P->Ofwd(*P->h,P->vert->hj,P->dgL);
#else
      P->Iprod(P);
#endif
      dcopy(P->Nmodes,P->vert->hj,1,*dxi + n,N);

      dvmul(qtot,sigma,1,*h,1,*P->h,1);
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
  /*
  dneg(nbl*P->Nmodes,*dxb,1);
  dneg(nbl*P->Nmodes,*dyb,1);
  dneg(N  *P->Nmodes,*dxi,1);
  dneg(N  *P->Nmodes,*dyi,1);
  */
}

// ============================================================================
