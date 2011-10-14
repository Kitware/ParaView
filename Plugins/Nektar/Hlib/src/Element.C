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

int QGmax,LGmax;

/* zero physical and transformed storage */
void zerofield(Element *U){
  while(U){
    dzero(U->qa*U->qb, *U->h, 1);   /* zero physical space */
    dzero(U->Nmodes,U->vert->hj,1); /* zero jacobi space   */
    U = U->next;
  }
}

#define ERR fprintf(stderr,"Accessing virtual function\n");






Element::Element(){
  id     = -1;
  type   = '-';
  state  = '-';
  Nverts = Nedges = Nfaces = 0;
  interior_l  = lmax   = 0;
  Nmodes = Nbmodes= 0;
  qa     = qb     = qc     = 0;
  vert   = (Vert*) NULL;
  edge   = (Edge*) NULL;
  face   = (Face*) NULL;
  curve  = (Curve*)NULL;
  curvX  = (Cmodes*)NULL;
  geom   = (Geom*)NULL;
  next   = (Element*)NULL;
}

Element::Element(Element *E){
  (*this) = (*E);
  return;
}


Element::Element(const Element &E){
  id     = E.id;
  type   = E.type;
  state  = E.state;
  Nverts = E.Nverts;
  Nedges = E.Nedges;
  Nfaces = E.Nfaces;
  vert = (Vert *)calloc(Nverts,sizeof(Vert));
  memcpy (vert,E.vert,Nverts*sizeof(Vert));
  edge = (Edge *)calloc(Nedges,sizeof(Edge));
  memcpy (edge,E.edge,Nedges*sizeof(Edge));
  face = (Face *)calloc(Nfaces,sizeof(Face));
  memcpy (face,E.face,Nfaces*sizeof(Face));
}


Element::~Element()
{
  Curve *cur;

  free(vert);
  free(edge);
  free(face);

  while(curve){
    cur = curve->next;
    free(curve);
    curve = cur;
  }

  if(curvX)
    free(curvX);



  // Looks to me like curve, curvX and geom should be stored by a
  // boost::shared_ptr, as multiple elements reference the same block of memory.
  // Angus
  //    double  **h;
  //    double  ***h_3d;
  //    double  ***hj_3d;
  //    Curve   *curve;
  //    Cmodes  *curvX;
  //    Geom    *geom;
  //    Element *next;
}

void Element::EdgeJbwd(double *d, int edg){
  Basis *b = getbasis();
  int   va = vnum(edg,0);
  int   vb = vnum(edg,1), i;
  dsmul(qa, vert[va].hj[0], b->vert[0].a, 1, d, 1);
  daxpy(qa, vert[vb].hj[0], b->vert[1].a, 1, d, 1);

  for(i = 0; i < edge[edg].l;++i)
    daxpy(qa, edge[edg].hj[i], b->edge[0][i].a, 1, d, 1);
}


void Element::set_curved(Curve*){ERR;}              // fix curve sides

// ============================================================================
void Element::PSE_Mat(Element *E, Metric *lambda, LocMat *pse, double *DU){
  ERR;}
