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

#include <stdio.h>

Hex::Hex(){
  Nverts = NHex_verts;
  Nedges = NHex_edges;
  Nfaces = NHex_faces;
}

Hex::Hex(Element *E){
  int i;

  if(!Hex_wk)
    Hex_work();

  id      = E->id;
  type    = E->type;
  state   = 'p';
  Nverts = NHex_verts;
  Nedges = NHex_edges;
  Nfaces = NHex_faces;

  vert    = (Vert *)calloc(NHex_verts,sizeof(Vert));
  edge    = (Edge *)calloc(NHex_edges,sizeof(Edge));
  face    = (Face *)calloc(NHex_faces,sizeof(Face));

  lmax    = E->lmax;
  interior_l       = E->interior_l;
  Nmodes  = E->Nmodes;
  Nbmodes = E->Nbmodes;
  qa      = E->qa;
  qb      = E->qb;
  qc      = E->qc;

  qtot    = qa*qb*qc;

  memcpy(vert,E->vert,NHex_verts*sizeof(Vert));
  memcpy(edge,E->edge,NHex_edges*sizeof(Edge));
  memcpy(face,E->face,NHex_faces*sizeof(Face));

  /* set memory */
  for(i=0;i<NHex_verts;++i)
    vert[i].hj = (double*)  0;
  for(i=0;i<NHex_edges;++i)
    edge[i].hj = (double*)  0;
  for(i=0;i<NHex_faces;++i)
    face[i].hj = (double**) 0;

  h        = (double**)  0;
  h_3d     = (double***) 0;
  hj_3d    = (double***) 0;

  curve  = E->curve;
  curvX  = E->curvX;
  geom   = E->geom;
  dgL    = E->dgL;

}

Hex::Hex(int i_d, char ty, int L, int Qa, int Qb, int Qc, Coord *X){
  int i;
  if(!Hex_wk)
    Hex_work();

  id = i_d;
  type = ty;
  state = 'p';
  Nverts = NHex_verts;
  Nedges = NHex_edges;
  Nfaces = NHex_faces;

  vert = (Vert *)calloc(NHex_verts,sizeof(Vert));
  edge = (Edge *)calloc(NHex_edges,sizeof(Edge));
  face = (Face *)calloc(NHex_faces,sizeof(Face));

  lmax = L;
  interior_l = max(0,L-2);

  Nmodes  = L*L*L;
  Nbmodes = Nmodes - (L-2)*(L-2)*(L-2);
  qa      = Qa;
  qb      = Qb;
  qc      = Qc;

  qtot    = qa*qb*qc;

  /* set vertex solve mask to 1 by default */
  for(i = 0; i < NHex_verts; ++i){
    vert[i].id    = i;
    vert[i].eid   = id;
    vert[i].solve = 1;
    vert[i].x     = X->x[i];
    vert[i].y     = X->y[i];
    vert[i].z     = X->z[i];
    vert[i].hj    = (double*)0;
  }
  /* construct edge system */
  for(i = 0; i < NHex_edges; ++i){
    edge[i].id  = i;
    edge[i].eid = id;
    edge[i].l   = L-2;
    edge[i].hj    = (double*)0;
  }

  /* construct face system */
  for(i = 0; i < NHex_faces; ++i){
    face[i].id  = i;
    face[i].eid = id;
    face[i].l   = max(0,L-2);
    face[i].hj  = (double**)0;
  }

  h     = (double**)0;
  h_3d  = (double***)0;
  hj_3d = (double***)0;

  curve  = (Curve*) calloc(1,sizeof(Curve));
  curve->face = -1;
  curve->type = T_Straight;
  curvX  = (Cmodes*)0;

  // curve  = (Curve*)NULL;
  // curvX  = (Cmodes*)NULL;
}

Hex::Hex(int , char , int *, int *, Coord *){
}
