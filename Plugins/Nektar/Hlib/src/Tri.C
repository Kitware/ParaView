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

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <polylib.h>
#include "veclib.h"
#include "hotel.h"
#include "nekstruct.h"

#include <stdio.h>

#define Tri_DIM 2


Tri::Tri(){
  Nverts = 3;
  Nedges = 3;
  Nfaces = 1;
}


Tri::Tri(Element *E){

  if(!Tri_wk.get())
    Tri_work();
  id      = E->id;
  type    = E->type;
  state   = 'p';
  Nverts = 3;
  Nedges = 3;
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

  qtot    = E->qtot;

  memcpy(vert,E->vert,Nverts*sizeof(Vert));
  memcpy(edge,E->edge,Nedges*sizeof(Edge));
  memcpy(face,E->face,Nfaces*sizeof(Face));

  /* set memory */
  vert[0].hj = (double*)  0;
  face[0].hj = (double**) 0;
  h          = (double**) 0;
  hj_3d = (double***)0;
  h_3d  = (double***)0;

  curve  = E->curve;
  curvX  = E->curvX;
  geom   = E->geom;
  dgL    = E->dgL;

}


Tri::Tri(int i_d, char ty, int L, int Qa, int Qb, int Qc, Coord *X){
  int i;

  if(!Tri_wk.get())
    Tri_work();

  id = i_d;
  type = ty;
  state = 'p';
  Nverts = 3;
  Nedges = 3;
  Nfaces = 1;

  vert = (Vert *)calloc(Nverts,sizeof(Vert));
  edge = (Edge *)calloc(Nedges,sizeof(Edge));
  face = (Face *)calloc(Nfaces,sizeof(Face));
  lmax = L;
  interior_l    = 0;
  Nmodes  = L*(L+1)/2;
  Nbmodes = Nmodes - (L-3)*(L-2)/2;
  qa      = Qa;
  qb      = Qb;
  qc      = Qc;

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
    face[i].l   = max(0,L-3);
  }

  vert[0].hj = (double*)  0;
  face[0].hj = (double**) 0;
  h          = (double**) 0;
  hj_3d = (double***)0;
  h_3d  = (double***)0;

  curve  = (Curve*)NULL;
  curvX  = (Cmodes*)NULL;
}

Tri::Tri(int , char , int *, int *, Coord *){
}


static void CheckVertLoc(Element *U, Element *E, int fac);

/* make sure that any vertex touching 'face' in element E has the
   same 'xy' co-ordinates */

static void CheckVertLoc(Element *U, Element *E, int face){
  int     gid1,gid2,i;
  double  x1,x2,y1,y2;

  gid1 = E->vert[face].gid;
  x1   = E->vert[face].x;
  y1   = E->vert[face].y;
  gid2 = E->vert[(face+1)%E->Nverts].gid;
  x2   = E->vert[(face+1)%E->Nverts].x;
  y2   = E->vert[(face+1)%E->Nverts].y;

  for(;U; U=U->next)
    for(i = 0; i < U->Nverts; ++i){
      if(U->vert[i].gid == gid1){
  U->vert[i].x = x1;
  U->vert[i].y = y1;
      }
      if(U->vert[i].gid == gid2){
  U->vert[i].x = x2;
  U->vert[i].y = y2;
      }
    }
}
#if 1
static double ***Tri_iplap = (double***)0;
static int Tri_ipmodes = 0;

// Assume fixed order
int mode_pos(Element *T, int m){
#if 1
  if(m < T->Nverts+T->edge[0].l)
    return m;

  if(m < T->Nverts+T->edge[0].l+T->edge[1].l)
    return m-T->edge[0].l+LGmax-2;

  if(m < T->Nbmodes)
    return m-T->edge[0].l-T->edge[1].l + 2*(LGmax-2);

  int j,n;
  for(j=0,n=T->Nbmodes+T->face[0].l;j<T->face[0].l;++j,n+=T->face[0].l-j)
    if(m < n)
      return
  m-T->Nbmodes+T->Nverts+(LGmax-2)*T->Nedges+j*(LGmax-3-T->face[0].l);

  return -100;
#else
  return m;
#endif
}

double Tri_iprodhelm(Element *T, int i, int j, double lambda){
  double d;
  double jac = T->geom->jac.d;
  double rx  = T->geom->rx.d, sx = T->geom->sx.d;
  double ry  = T->geom->ry.d, sy = T->geom->sy.d;
  int    pa  = mode_pos(T,i);
  int    pb  = mode_pos(T,j);

  d  = (rx*rx+ry*ry)*Tri_iplap[0][pa][pb];
  d += (rx*sx+ry*sy)*Tri_iplap[1][pa][pb];
  d += (sx*sx+sy*sy)*Tri_iplap[2][pa][pb];
  d +=        lambda*Tri_iplap[3][pa][pb];
  d *= jac;

  return d;
}
void  Tri_setup_iprodlap();

void  Tri_HelmMat(Element *T, LocMat *helm, double lambda){

  if(!Tri_iplap)
    Tri_setup_iprodlap();

  double **a = helm->a,
         **b = helm->b,
         **c = helm->c;
  int    nbl    = T->Nbmodes;
  int    Nmodes = T->Nmodes;
  int    i,j;

  /* `A' matrix */
  for(i = 0; i < nbl; ++i)
    for(j = i; j < nbl; ++j)
      a[i][j] = a[j][i] = Tri_iprodhelm(T, i, j, lambda);

  /* `B' matrix */
  for(i = 0; i < nbl; ++i)
    for(j = nbl; j < Nmodes; ++j)
      b[i][j-nbl] = Tri_iprodhelm(T, i, j, lambda);

  /* 'C' matrix */
  for(i = nbl; i < Nmodes; ++i)
    for(j = nbl; j < Nmodes; ++j)
      c[i-nbl][j-nbl] = c[j-nbl][i-nbl] = Tri_iprodhelm(T, i, j, lambda);

}

void Tri_setup_iprodlap(){

  int qa = QGmax;
  int qb;

  if(LZero) // use extra points if have Legendre zeros in 'b' direction
    qb = QGmax;
  else
    qb = QGmax-1;

  int qc = 0;
  int L  = LGmax;

  // Set up dummy element with maximum quadrature/edge order

  Coord X;
  X.x = dvector(0,NTri_verts-1);
  X.y = dvector(0,NTri_verts-1);

  X.x[0] = 0.0;  X.x[1] = 1.0;  X.x[2] = 0.0;
  X.y[0] = 0.0;  X.y[1] = 0.0;  X.y[2] = 1.0;

  Tri *T = (Tri*) new Tri(0,'T', L, qa, qb, qc, &X);

  free(X.x);  free(X.y);

  int i,j,k,n;
  int facs = Tri_DIM*Tri_DIM;
  Tri_ipmodes = T->Nmodes;
  Tri_iplap = (double***) calloc(facs,sizeof(double**));

  for(i = 0; i < facs; ++i){
    Tri_iplap[i] = dmatrix(0, T->Nmodes-1, 0, T->Nmodes-1);
    dzero(T->Nmodes*T->Nmodes, Tri_iplap[i][0], 1);
  }

  // Set up gradient basis

  Basis   *B,*DB;
  Mode    *w,*m,*m1,*md,*md1,**gb,**gb1,*fac;
  double  *z;

  B      = T->getbasis();
  DB     = T->derbasis();

  fac    = B->vert;
  w      = mvector(0,0);
  gb     = (Mode **) malloc(T->Nmodes*sizeof(Mode *));
  gb[0]  = mvecset(0,Tri_DIM*T->Nmodes,qa, qb, qc);
  gb1    = (Mode **) malloc(T->Nmodes*sizeof(Mode *));
  gb1[0] = mvecset(0,Tri_DIM*T->Nmodes,qa, qb, qc);

  for(i = 1; i < T->Nmodes; ++i) gb[i]  = gb[i-1]+Tri_DIM;
  for(i = 1; i < T->Nmodes; ++i) gb1[i] = gb1[i-1]+Tri_DIM;

  getzw(qa,&z,&w[0].a,'a');
  getzw(qb,&z,&w[0].b,'b');

  /* fill gb with basis info for laplacian calculation */

  // vertex modes
  m  =  B->vert;
  md = DB->vert;
  for(i = 0,n=0; i < T->Nverts; ++i,++n)
    T->fill_gradbase(gb[n],m+i,md+i,fac);

  // edge modes
  for(i = 0; i < T->Nedges; ++i){
    m1  = B ->edge[i];
    md1 = DB->edge[i];
    for(j = 0; j < T->edge[i].l; ++j,++n)
      T->fill_gradbase(gb[n],m1+j,md1+j,fac);
  }

  // face modes
  for(i = 0; i < T->Nfaces; ++i)
    for(j = 0; j < T->face[0].l; ++j){
      m1  = B ->face[i][j];
      md1 = DB->face[i][j];
      for(k = 0; k < T->face[0].l-j; ++k,++n)
  T->fill_gradbase(gb[n],m1+k,md1+k,fac);
    }

  /* multiply by weights */
  for(i = 0; i < T->Nmodes; ++i){
    Tri_mvmul2d(qa,qb,qc,gb[i]  ,w,gb1[i]);
    Tri_mvmul2d(qa,qb,qc,gb[i]+1,w,gb1[i]+1);
  }

  // Calculate Laplacian inner products

  double s1, s2, s3, s4, s5, s6, s7, s8;
  double *tmp = dvector(0, QGmax-1);

  fac = B->vert+1;

  for(i = 0; i < T->Nmodes; ++i)
    for(j = 0; j < T->Nmodes; ++j){

      s1  = ddot(qa,gb[i][0].a,1,gb1[j][0].a,1);
      s1 *= ddot(qb,gb[i][0].b,1,gb1[j][0].b,1);

      dvmul(qa, fac->a, 1, gb[i][0].a, 1, tmp, 1);

      s2  = ddot(qa,       tmp,1,gb1[j][0].a,1);
      s2 *= ddot(qb,gb[i][0].b,1,gb1[j][0].b,1);

      s3  = ddot(qa,       tmp,1,gb1[j][1].a,1);
      s3 *= ddot(qb,gb[i][0].b,1,gb1[j][1].b,1);

      dvmul(qa, fac->a, 1, tmp, 1, tmp, 1);

      s4  = ddot(qa,       tmp,1,gb1[j][0].a,1);
      s4 *= ddot(qb,gb[i][0].b,1,gb1[j][0].b,1);

      s5  = ddot(qa,gb[i][0].a,1,gb1[j][1].a,1);
      s5 *= ddot(qb,gb[i][0].b,1,gb1[j][1].b,1);

      s6  = ddot(qa,gb[i][1].a,1,gb1[j][0].a,1);
      s6 *= ddot(qb,gb[i][1].b,1,gb1[j][0].b,1);

      dvmul(qa, fac->a, 1, gb[i][1].a, 1, tmp, 1);

      s7  = ddot(qa,       tmp,1,gb1[j][0].a,1);
      s7 *= ddot(qb,gb[i][1].b,1,gb1[j][0].b,1);

      s8  = ddot(qa,gb[i][1].a,1,gb1[j][1].a,1);
      s8 *= ddot(qb,gb[i][1].b,1,gb1[j][1].b,1);

      Tri_iplap[0][i][j] = s1;
      Tri_iplap[1][i][j] = s5 + 2*s2 + s6;
      Tri_iplap[2][i][j] = s3 + s4 + s7 + s8;
    }

  /* fill gb with basis info for mass matrix calculation */
  // vertex modes
  m  = B->vert;
  for(i = 0,n=0; i < T->Nverts; ++i,++n){
    dcopy(qa, m[i].a, 1, gb[n]->a, 1);
    dcopy(qb, m[i].b, 1, gb[n]->b, 1);
  }

  // edge modes
  for(i = 0; i < T->Nedges; ++i){
    m1 = B ->edge[i];
    for(j = 0; j < T->edge[i].l; ++j,++n){
      dcopy(qa, m1[j].a, 1, gb[n]->a, 1);
      dcopy(qb, m1[j].b, 1, gb[n]->b, 1);
    }
  }

  // face modes
  for(i = 0; i < T->Nfaces; ++i)
    for(j = 0; j < T->face[i].l; ++j){
      m1  = B ->face[i][j];
      for(k = 0; k < T->face[i].l-j; ++k,++n){
  dcopy(qa, m1[k].a, 1, gb[n]->a, 1);
  dcopy(qb, m1[k].b, 1, gb[n]->b, 1);
      }
    }

  /* multiply by weights */
  for(i = 0; i < T->Nmodes; ++i)
    Tri_mvmul2d(qa,qb,qc,gb[i]  ,w,gb1[i]);

  for(i = 0; i < T->Nmodes; ++i)
    for(j = 0; j < T->Nmodes; ++j){

      s1  = ddot(qa,gb[i][0].a,1,gb1[j][0].a,1);
      s1 *= ddot(qb,gb[i][0].b,1,gb1[j][0].b,1);

      Tri_iplap[3][i][j] = s1;
    }

  free_mvec(gb[0]) ; free((char *) gb);
  free_mvec(gb1[0]); free((char *) gb1);
  free((char*)w);

  free(tmp);

  delete (T);
}
#endif

// ============================================================================
void Tri::PSE_Mat(Element *E, Metric *lambda, LocMat *pse, double *DU){

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

}
// ============================================================================
void Tri::PSE_Mat(Element *E, LocMat *pse, double *DU){

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

}
// ============================================================================
void Tri::BET_Mat(Element *P, LocMatDiv *bet, double *beta, double *sigma){
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
    for(j = 0; j < L-i; ++j,++n){
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
