/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permiseion of the author.                     //
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

// done

#define TET_DIM  3

Tet::Tet(){
  Nverts = 4;
  Nedges = 6;
  Nfaces = 4;
}


Tet::Tet(Element *E){
  int i;

  if(!Tet_wk)
    Tet_work();

  id      = E->id;
  type    = E->type;
  state   = 'p';
  Nverts = 4;
  Nedges = 6;
  Nfaces = 4;

  vert    = (Vert *)calloc(Nverts,sizeof(Vert));
  edge    = (Edge *)calloc(Nedges,sizeof(Edge));
  face    = (Face *)calloc(Nfaces,sizeof(Face));

  lmax    = E->lmax;
  interior_l       = E->interior_l;
  Nmodes  = E->Nmodes;
  Nbmodes = E->Nbmodes;
  qa      = E->qa;
  qb      = E->qb;
  qc      = E->qc;

  qtot    = qa*qb*qc;

  memcpy(vert,E->vert,Nverts*sizeof(Vert));
  memcpy(edge,E->edge,Nedges*sizeof(Edge));
  memcpy(face,E->face,Nfaces*sizeof(Face));

  /* set memory */
  for(i=0;i<Nverts;++i)
    vert[i].hj = (double*)  0;
  for(i=0;i<Nedges;++i)
    edge[i].hj = (double*)  0;
  for(i=0;i<Nfaces;++i)
    face[i].hj = (double**) 0;

  h        = (double**)  0;
  h_3d     = (double***) 0;
  hj_3d    = (double***) 0;

  curve  = E->curve;
  curvX  = E->curvX;
  geom   = E->geom;
  dgL    = E->dgL;

}

Tet::Tet(int i_d, char ty, int L, int Qa, int Qb, int Qc, Coord *X){
  int i;

  if(!Tet_wk)
    Tet_work();

  id = i_d;
  type = ty;
  state = 'p';
  Nverts = 4;
  Nedges = 6;
  Nfaces = 4;

  vert = (Vert *)calloc(Nverts,sizeof(Vert));
  edge = (Edge *)calloc(Nedges,sizeof(Edge));
  face = (Face *)calloc(Nfaces,sizeof(Face));

  lmax = L;
  interior_l = max(0,L-4);

  Nmodes  = L*(L+1)*(L+2)/6;
  Nbmodes = Nmodes - (L-4)*(L-3)*(L-2)/6;
  qa      = Qa;
  qb      = Qb;
  qc      = Qc;

  qtot    = qa*qb*qc;

  /* set vertex solve mask to 1 by default */
  for(i = 0; i < Nverts; ++i){
    vert[i].id    = i;
    vert[i].eid   = id;
    vert[i].solve = 1;
    vert[i].x     = X->x[i];
    vert[i].y     = X->y[i];
    vert[i].z     = X->z[i];
    vert[i].hj    = (double*)0;
  }
  /* construct edge system */
  for(i = 0; i < Nedges; ++i){
    edge[i].id  = i;
    edge[i].eid = id;
    edge[i].l   = L-2;
    edge[i].hj    = (double*)0;
  }

  /* construct face system */
  for(i = 0; i < Nfaces; ++i){
    face[i].id  = i;
    face[i].eid = id;
    face[i].l   = max(0,L-3);
    face[i].hj  = (double**)0;
  }

  h     = (double**)0;
  h_3d  = (double***)0;
  hj_3d = (double***)0;

  curve  = (Curve*)NULL;
  curvX  = (Cmodes*)NULL;
}

Tet::Tet(int , char , int *, int *, Coord *){
}
