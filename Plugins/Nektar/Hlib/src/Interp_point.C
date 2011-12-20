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
#include "polylib.h"
#include "hotel.h"
#include <smart_ptr.hpp>

#include <stdio.h>

using namespace polylib;

static int Find_elmt_coords_2d(Element_List *U,
          double xo, double yo,
          int *eid, double *a, double *b, char trip);
static int Find_elmt_coords_3d(Element_List *U,
          double xo, double yo, double zo,
          int *eid, double *a, double *b, double *c, char trip);


void Tri_get_point_shape(Element *E, double a, double b,
       double *hr, double *hs){
  double *za, *zb, *zc;
  double *wa, *wb, *wc;

  int i;

  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  for (i = 0; i < E->qa; i++)
    hr[i] = hgll(i, a, za, E->qa);

  if(LZero)
    for (i = 0; i < E->qb; i++)
      hs[i] = hgrj(i, b, zb, E->qb, 0., 0.);
  else
    for (i = 0; i < E->qb; i++)
      hs[i] = hgrj(i, b, zb, E->qb, 1., 0.);
}


void Quad_get_point_shape(Element *E, double a, double b,
       double *hr, double *hs){
  double *za, *zb, *zc;
  double *wa, *wb, *wc;

  int i;

  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  for (i = 0; i < E->qa; i++)
    hr[i] = hgll(i, a, za, E->qa);

  for (i = 0; i < E->qb; i++)
    hs[i] = hgll(i, b, zb, E->qb);
}


void Tet_get_point_shape(Element *E, double a, double b, double c,
       double *hr, double *hs, double *ht){
  double *za, *zb, *zc;
  double *wa, *wb, *wc;

  int i;

  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  for (i = 0; i < E->qa; i++)
    hr[i] = hgll(i, a, za, E->qa);

  if(LZero){
    for (i = 0; i < E->qb; i++)
      hs[i] = hgrj(i, b, zb, E->qb, 0., 0.);

    for (i = 0; i < E->qc; i++)
      ht[i] = hgrj(i, c, zc, E->qc, 0., 0.);
  }
  else{
    for (i = 0; i < E->qb; i++)
      hs[i] = hgrj(i, b, zb, E->qb, 1., 0.);

    for (i = 0; i < E->qc; i++)
      ht[i] = hgrj(i, c, zc, E->qc, 2., 0.);
  }

}


void Prism_get_point_shape(Element *E, double a, double b, double c,
       double *hr, double *hs, double *ht){
  double *za, *zb, *zc;
  double *wa, *wb, *wc;

  int i;

  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  for (i = 0; i < E->qa; i++)
    hr[i] = hgll(i, a, za, E->qa);

  for (i = 0; i < E->qb; i++)
    hs[i] = hgll(i, b, zb, E->qb);

  if(LZero)
    for (i = 0; i < E->qc; i++)
      ht[i] = hgrj(i, c, zc, E->qc, 0., 0.);
  else
    for (i = 0; i < E->qc; i++)
      ht[i] = hgrj(i, c, zc, E->qc, 1., 0.);

}




void Hex_get_point_shape(Element *E, double a, double b, double c,
       double *hr, double *hs, double *ht){
  double *za, *zb, *zc;
  double *wa, *wb, *wc;

  int i;

  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  for (i = 0; i < E->qa; i++)
    hr[i] = hgll(i, a, za, E->qa);

  for (i = 0; i < E->qb; i++)
    hs[i] = hgll(i, b, zb, E->qb);

  for (i = 0; i < E->qc; i++)
    ht[i] = hgll(i, c, zc, E->qc);
}


void get_point_shape_3d(Element *E, double a, double b, double c,
         double *hr, double *hs, double *ht){
  switch (E->identify()){
  case Nek_Tet:
    Tet_get_point_shape(E,a,b,c,hr,hs,ht);
    break;
  case Nek_Prism:
    Prism_get_point_shape(E,a,b,c,hr,hs,ht);
    break;
  case Nek_Hex:
    Hex_get_point_shape(E,a,b,c,hr,hs,ht);
    break;
  }
}


void get_point_shape_2d(Element *E, double a, double b,
         double *hr, double *hs){
  switch (E->identify()){
  case Nek_Tri:
    Tri_get_point_shape(E,a,b,hr,hs);
    break;
  case Nek_Quad:
    Quad_get_point_shape(E,a,b,hr,hs);
    break;
  }
}

// this function should be written in c++
void get_point_shape(Element *E, Coord A, double **h){
  switch (E->identify()){
  case Nek_Tri:
    Tri_get_point_shape(E,A.x[0],A.y[0],h[0],h[1]);
    break;
  case Nek_Quad:
    Quad_get_point_shape(E,A.x[0],A.y[0],h[0],h[1]);
    break;
  case Nek_Tet:
    Tet_get_point_shape(E,A.x[0],A.y[0],A.z[0],h[0],h[1],h[2]);
    break;
  case Nek_Prism:
    Prism_get_point_shape(E,A.x[0],A.y[0],A.z[0],h[0],h[1],h[2]);
    break;
  case Nek_Hex:
    Hex_get_point_shape(E,A.x[0],A.y[0],A.z[0],h[0],h[1],h[2]);
    break;
  }
}


double eval_field_at_pt_2d(int qa, int qb, double *field, double *hr, double *hs){
  int i;
  double d=0.;

  for (i = 0; i < qb; i++)
    d += hs[i]*ddot(qa, hr, 1, field+i*qa, 1);
  return d;

}


double eval_field_at_pt_3d(int qa, int qb, int qc, double *field, double *hr, double *hs, double *ht){
  int i,j;
  double d=0.;

  for(j = 0; j < qc; ++j)
    for (i = 0; i < qb; i++)
      d += ht[j]*hs[i]*ddot(qa, hr, 1, field+i*qa+j*qa*qb, 1);
  return d;

}



class BoundBox {
public:
  int nel;
  int dim;
  double *lx;
  double *ly;
  double *lz;

  double *ux;
  double *uy;
  double *uz;
};

namespace {

struct DestroyBoundBox {
  void operator()(BoundBox * Bbox)
  {
    free(Bbox->lx);
    free(Bbox->ly);
    free(Bbox->ux);
    free(Bbox->uy);
    if (Bbox->dim == 3) {
      free(Bbox->lz);
      free(Bbox->uz);
    }
    free (Bbox);
    Bbox = 0;
  }
};

} // namespace anon

nektar::scoped_ptr<BoundBox, DestroyBoundBox> Bbox;

#define TOLBOX 1.1  // scaling factor of box

void setup_elmt_boxes(Element_List *U){
  Element *E;
  double mean;
  int i,k;

  if(U->fhead->dim() == 3){

    Bbox.reset((BoundBox*) calloc(1, sizeof(BoundBox)));

    Bbox->dim = U->fhead->dim();
    Bbox->nel = U->nel;
    Bbox->lx = dvector(0, U->nel-1);
    Bbox->ly = dvector(0, U->nel-1);
    Bbox->lz = dvector(0, U->nel-1);

    Bbox->ux = dvector(0, U->nel-1);
    Bbox->uy = dvector(0, U->nel-1);
    Bbox->uz = dvector(0, U->nel-1);

    Coord X;
    X.x = dvector(0, QGmax*QGmax*QGmax-1);
    X.y = dvector(0, QGmax*QGmax*QGmax-1);
    X.z = dvector(0, QGmax*QGmax*QGmax-1);

    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      E->coord(&X);
      Bbox->lx[k] = X.x[idmin(E->qtot, X.x, 1)];
      Bbox->ly[k] = X.y[idmin(E->qtot, X.y, 1)];
      Bbox->lz[k] = X.z[idmin(E->qtot, X.z, 1)];

      Bbox->ux[k] = X.x[idmax(E->qtot, X.x, 1)];
      Bbox->uy[k] = X.y[idmax(E->qtot, X.y, 1)];
      Bbox->uz[k] = X.z[idmax(E->qtot, X.z, 1)];

      /* check singular vertices */
      for(i = 2; i < min(E->Nverts,6); ++i){
  Bbox->lx[k] = min(Bbox->lx[k],E->vert[i].x);
  Bbox->ly[k] = min(Bbox->ly[k],E->vert[i].y);
  Bbox->lz[k] = min(Bbox->lz[k],E->vert[i].z);

  Bbox->ux[k] = max(Bbox->ux[k],E->vert[i].x);
  Bbox->uy[k] = max(Bbox->uy[k],E->vert[i].y);
  Bbox->uz[k] = max(Bbox->uz[k],E->vert[i].z);
      }

      /* add in a safety tolerance */
      mean = (Bbox->lx[k] + Bbox->ux[k])*0.5;
      Bbox->lx[k] = mean + TOLBOX*(Bbox->lx[k]-mean);
      Bbox->ux[k] = mean + TOLBOX*(Bbox->ux[k]-mean);
      mean = (Bbox->ly[k] + Bbox->uy[k])*0.5;
      Bbox->ly[k] = mean + TOLBOX*(Bbox->ly[k]-mean);
      Bbox->uy[k] = mean + TOLBOX*(Bbox->uy[k]-mean);
      mean = (Bbox->lz[k] + Bbox->uz[k])*0.5;
      Bbox->lz[k] = mean + TOLBOX*(Bbox->lz[k]-mean);
      Bbox->uz[k] = mean + TOLBOX*(Bbox->uz[k]-mean);
    }

    free(X.x);
    free(X.y);
    free(X.z);
  }
  else{

    Bbox.reset((BoundBox*) calloc(1, sizeof(BoundBox)));

    Bbox->dim = U->fhead->dim();
    Bbox->nel = U->nel;
    Bbox->lx = dvector(0, U->nel-1);
    Bbox->ly = dvector(0, U->nel-1);

    Bbox->ux = dvector(0, U->nel-1);
    Bbox->uy = dvector(0, U->nel-1);

    Coord X;
    X.x = dvector(0, QGmax*QGmax-1);
    X.y = dvector(0, QGmax*QGmax-1);

    for(k=0;k<U->nel;++k){
      E = U->flist[k];
      E->coord(&X);
      Bbox->lx[k] = X.x[idmin(E->qtot, X.x, 1)];
      Bbox->ly[k] = X.y[idmin(E->qtot, X.y, 1)];

      Bbox->ux[k] = X.x[idmax(E->qtot, X.x, 1)];
      Bbox->uy[k] = X.y[idmax(E->qtot, X.y, 1)];

      /* check top vertex for triangular element */
      Bbox->lx[k] = min(Bbox->lx[k],E->vert[2].x);
      Bbox->ly[k] = min(Bbox->ly[k],E->vert[2].y);

      Bbox->ux[k] = max(Bbox->ux[k],E->vert[2].x);
      Bbox->uy[k] = max(Bbox->uy[k],E->vert[2].y);

      /* add in a safety tolerance */
      mean = (Bbox->lx[k] + Bbox->ux[k])*0.5;
      Bbox->lx[k] = mean + TOLBOX*(Bbox->lx[k]-mean);
      Bbox->ux[k] = mean + TOLBOX*(Bbox->ux[k]-mean);
      mean = (Bbox->ly[k] + Bbox->uy[k])*0.5;
      Bbox->ly[k] = mean + TOLBOX*(Bbox->ly[k]-mean);
      Bbox->uy[k] = mean + TOLBOX*(Bbox->uy[k]-mean);
    }
    free(X.x);
    free(X.y);
  }
}

int point_in_box_2d(double x, double y, int boxid){

  if( x <= Bbox->ux[boxid] && x >= Bbox->lx[boxid] &&
      y <= Bbox->uy[boxid] && y >= Bbox->ly[boxid])
    return 1;

  return 0;
}

int point_in_box_3d(double x, double y, double z, int boxid){

  if((boxid >= Bbox->nel)||(boxid < 0))
    fprintf(stderr,"call to point_in_box_3d with illegal boxid number \n");

  if( x <= Bbox->ux[boxid] && x >= Bbox->lx[boxid] &&
      y <= Bbox->uy[boxid] && y >= Bbox->ly[boxid] &&
      z <= Bbox->uz[boxid] && z >= Bbox->lz[boxid])
    return 1;

  return 0;
}

#define MAXIT    101         /* maximum iterations for convergence   */
#define EPSILON  1.e-8       /* |x-xp| < EPSILON  error tolerance    */
#define GETRSDIV 15.0        /* |r,s|  > GETRSDIV stop the search    */

int     meshX_nel;
int     meshX_dim;

namespace {

struct DestroyMeshX {
  void operator()(Coord ** ptr)
  {
    for(int i = 0; i < meshX_nel; ++i) {
      free(ptr[i]->x);
      free(ptr[i]->y);
      if(meshX_dim == 3)
  free(ptr[i]->z);
      free(ptr[i]);
    }
    free(ptr);
    ptr = 0;
  }
};

} // namespace anon

nektar::scoped_ptr<Coord *, DestroyMeshX> meshX;

void set_mesh_coord(Element_List *EL){
  Element *E;

  meshX.reset((Coord**) calloc(EL->nel, sizeof(Coord*)));

  for(E=EL->fhead;E;E=E->next){
    meshX[E->id] = (Coord*) calloc(1, sizeof(Coord));
    meshX[E->id]->x = dvector(0, E->qtot-1);
    meshX[E->id]->y = dvector(0, E->qtot-1);
    if(E->dim() == 3)
      meshX[E->id]->z = dvector(0, E->qtot-1);
    E->coord(meshX[E->id]);
  }
  meshX_nel = EL->nel;
  meshX_dim = EL->fhead->dim();
}

int Find_coords_2d(Element *E, double xo, double yo,
    double *a, double *b, char trip){


  int fl = 0;
  int ic = 0, i;

  int qa = E->qa;
  int qb = E->qb;

  double xp, yp, drdx, drdy, dsdx, dsdy;
  double rr, ss, aa, bb;
  double dx, dy, tmp;

  double *za, *wa, *zb, *wb, *zc, *wc;
  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  static double *hr    = NULL;
  static double *hs    = NULL;
  //  static double *gridx = NULL;
  //  static double *gridy = NULL;
  //  static Coord gX;
  Coord *gX;
  double *gridx;
  double *gridy;

  if(!hr){
    hr    = dvector (0, QGmax-1);
    hs    = dvector (0, QGmax-1);
  }

  gX = meshX[E->id];
  gridx = gX->x;
  gridy = gX->y;

  if(trip == 'i'){
    rr = aa = a[0];
    ss = bb = b[0];
  }
  else if(E->curvX){
    /* make initial guess based on nearest point */
    tmp = 1e10;
    for(i = 0; i < qa*qb; ++i){
      rr = (xo-gridx[i])*(xo-gridx[i]) + (yo-gridy[i])*(yo-gridy[i]);
      if(rr < tmp){
  tmp = rr;
  ic = i;
      }
    }
    rr = aa = za[ic%qa];
    ss = bb = zb[ic/qa];
  }

  ic = 0;

  /* .... Start the iteration .... */
  if(E->identify() == Nek_Quad){

    while (!fl && (ic++ <= MAXIT)) {

      Quad_get_point_shape(E, rr, ss, hr, hs);

      xp = eval_field_at_pt_2d(qa, qb, gridx, hr, hs);
      yp = eval_field_at_pt_2d(qa, qb, gridy, hr, hs);

      drdx = eval_field_at_pt_2d(qa, qb, E->geom->rx.p, hr, hs);
      drdy = eval_field_at_pt_2d(qa, qb, E->geom->ry.p, hr, hs);

      dsdx = eval_field_at_pt_2d(qa, qb, E->geom->sx.p, hr, hs);
      dsdy = eval_field_at_pt_2d(qa, qb, E->geom->sy.p, hr, hs);

      dx    = xo - xp;                   /* Distance from the point (x,y) */
      dy    = yo - yp;

      rr   += (drdx * dx + drdy * dy);  /* Corrections to the guess */
      ss   += (dsdx * dx + dsdy * dy);

      /* Convergence test */

      if (sqrt(dx*dx + dy*dy) < EPSILON){
  fl = 1;
  break;
      }
      if (fabs(rr) > GETRSDIV || fabs(ss) > GETRSDIV){
  fl = -1;
  break;
      }
    }

    aa = rr;
    bb = ss;

  }
  else{

    if(E->curvX){
      ss = bb;
      rr = (aa+1)*(1.-ss)*0.5 - 1.0;

      fl = 0;

      while (!fl && (ic++ <= MAXIT)) {

  Tri_get_point_shape(E, aa, bb, hr, hs);

  xp = eval_field_at_pt_2d(qa, qb, gridx, hr, hs);
  yp = eval_field_at_pt_2d(qa, qb, gridy, hr, hs);

  drdx = eval_field_at_pt_2d(qa, qb, E->geom->rx.p, hr, hs);
  drdy = eval_field_at_pt_2d(qa, qb, E->geom->ry.p, hr, hs);

  dsdx = eval_field_at_pt_2d(qa, qb, E->geom->sx.p, hr, hs);
  dsdy = eval_field_at_pt_2d(qa, qb, E->geom->sy.p, hr, hs);

        dx    = xo - xp;                   /* Distance from the point (x,y) */
  dy    = yo - yp;

  rr   += (drdx * dx + drdy * dy);  /* Corrections to the guess */
  ss   += (dsdx * dx + dsdy * dy);

  aa = (2.*(1.+rr)/(1.-ss))-1.;
  bb = ss;

  /* Convergence test */

  if (sqrt(dx*dx + dy*dy) < EPSILON){
    fl = 1;
    break;
  }

  if(aa < -GETRSDIV || aa > GETRSDIV || bb < -GETRSDIV || bb > GETRSDIV){
    fl = -1;
    break;
  }
      }
    }
    else {// straight sided so use analytic form
      Vert *v[3];

      v[0] = E->vert;
      v[1] = E->vert+1;
      v[2] = E->vert+2;

      rr = ((2*xo - v[1]->x - v[2]->x)*(v[2]->y - v[0]->y) +
      (2*yo - v[1]->y - v[2]->y)*(v[0]->x - v[2]->x))/
        ((v[1]->x - v[0]->x)*(v[2]->y - v[0]->y)
         +  (v[1]->y - v[0]->y)*(v[0]->x - v[2]->x));

      ss = ((2*xo - v[1]->x - v[2]->x)*(v[1]->y - v[0]->y) +
      (2*yo - v[1]->y - v[2]->y)*(v[0]->x - v[1]->x))/
        ((v[2]->x - v[0]->x)*(v[1]->y - v[0]->y)
         +  (v[2]->y - v[0]->y)*(v[0]->x - v[1]->x));

      // if ss = 1 then check to see if r=-1 otherwise return value of 2
      aa = (1-ss)? (2.*(1.+rr)/(1.-ss))-1.:(1+rr)? 2:rr;
      bb = ss;
      fl = 1;
    }
  }

  *a = aa;
  *b = bb;

  if(ic > MAXIT){
    return 0;
  }
  if (fl == 1){
    if( aa < -1. || aa > 1. || bb < -1. || bb > 1.)
      return 0;
    else
      return 1;
  }

  return 0;
}


// note this is the find_coords_3d function with a trip added

int Find_coords_3d(Element *E, double xo, double yo,  double zo,
       double *a, double *b, double *c, char trip){
  int fl = 0;
  int ic = 0, i;
  int qa = E->qa;
  int qb = E->qb;
  int qc = E->qc;
  double xp, yp, zp;
  double drdx, drdy, drdz;
  double dsdx, dsdy, dsdz;
  double dtdx, dtdy, dtdz;
  double rr, ss, tt;
  double aa, bb, cc;
  double dx, dy, dz, tmp;
  double *za, *wa, *zb, *wb, *zc, *wc;
  static double *hr    = NULL;
  static double *hs    = NULL;
  static double *ht    = NULL;
  double *gridx = NULL;
  double *gridy = NULL;
  double *gridz = NULL;
  Coord *gX;

  E->GetZW(&za, &wa, &zb, &wb, &zc, &wc);

  if(!hr){
    hr    = dvector (0, QGmax-1);
    hs    = dvector (0, QGmax-1);
    ht    = dvector (0, QGmax-1);
  }

  gX = meshX[E->id];
  gridx = gX->x;
  gridy = gX->y;
  gridz = gX->z;

  if(trip == 'i'){
    rr = aa = a[0];
    ss = bb = b[0];
    tt = cc = c[0];
  }
  else if(E->curvX){
    /* make initial guess based on nearest point */
    tmp = 1e10;
    for(i = 0; i < qa*qb*qc; ++i){
      rr = (xo-gridx[i])*(xo-gridx[i]) + (yo-gridy[i])*(yo-gridy[i])
  + (zo-gridz[i])*(zo-gridz[i]);
      if(rr < tmp){
  tmp = rr;
  ic = i;
      }
    }
    rr = aa = za[(ic%(qa*qb))%qa];
    ss = bb = zb[(ic%(qa*qb))/qa];
    tt = cc = zc[(ic/(qa*qb))];
  }

  ic = 0;

  /* .... Start the iteration .... */
  switch (E->identify()){
  case Nek_Hex:

    while (!fl && (ic++ <= MAXIT)) {

      Hex_get_point_shape(E, rr, ss, tt, hr, hs, ht);

      xp   = eval_field_at_pt_3d(qa,qb,qc, gridx, hr, hs, ht);
      yp   = eval_field_at_pt_3d(qa,qb,qc, gridy, hr, hs, ht);
      zp   = eval_field_at_pt_3d(qa,qb,qc, gridz, hr, hs, ht);

      drdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->rx.p, hr, hs, ht);
      drdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->ry.p, hr, hs, ht);
      drdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->rz.p, hr, hs, ht);

      dsdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->sx.p, hr, hs, ht);
      dsdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->sy.p, hr, hs, ht);
      dsdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->sz.p, hr, hs, ht);

      dtdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->tx.p, hr, hs, ht);
      dtdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->ty.p, hr, hs, ht);
      dtdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->tz.p, hr, hs, ht);

      dx    = xo - xp;                   /* Distance from the point (x,y) */
      dy    = yo - yp;
      dz    = zo - zp;

      /* Corrections to the guess */
      rr   += (drdx * dx + drdy * dy + drdz * dz);
      ss   += (dsdx * dx + dsdy * dy + dsdz * dz);
      tt   += (dtdx * dx + dtdy * dy + dtdz * dz);

      /* Convergence test */

      if (sqrt(dx*dx + dy*dy + dz*dz) < EPSILON){
  fl = 1;
  break;
      }
      if (fabs(rr) > GETRSDIV || fabs(ss) > GETRSDIV ||
    fabs(tt) > GETRSDIV) {
  fl = -1;
  break;
      }
    }

    aa = rr;
    bb = ss;
    cc = tt;

    break;
  case Nek_Tet:

    if(E->curvX){
      fl = 0;

      tt = cc;
      ss = (bb+1)*(1-tt)*0.5-1.0;
      rr = (aa+1)*(-ss-tt)*0.5-1.0;

      while (!fl && (ic++ <= MAXIT)) {

  Tet_get_point_shape(E, aa, bb, cc, hr, hs, ht);

  xp   = eval_field_at_pt_3d(qa,qb,qc, gridx, hr, hs, ht);
  yp   = eval_field_at_pt_3d(qa,qb,qc, gridy, hr, hs, ht);
  zp   = eval_field_at_pt_3d(qa,qb,qc, gridz, hr, hs, ht);

  drdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->rx.p, hr, hs, ht);
  drdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->ry.p, hr, hs, ht);
  drdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->rz.p, hr, hs, ht);

  dsdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->sx.p, hr, hs, ht);
  dsdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->sy.p, hr, hs, ht);
  dsdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->sz.p, hr, hs, ht);

  dtdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->tx.p, hr, hs, ht);
  dtdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->ty.p, hr, hs, ht);
  dtdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->tz.p, hr, hs, ht);

  dx    = xo - xp;   /* Distance from the point (x,y) */
  dy    = yo - yp;
  dz    = zo - zp;

  /* Corrections to the guess */
  rr   += (drdx * dx + drdy * dy + drdz * dz);
  ss   += (dsdx * dx + dsdy * dy + dsdz * dz);
  tt   += (dtdx * dx + dtdy * dy + dtdz * dz);


  aa = (ss+tt)? (-2.*(1.+rr)/(ss+tt)) - 1.:0.0;
  bb = (1.-tt)? ( 2.*(1.+ss)/(1.-tt)) - 1.: 0.0;
  cc = tt;

  /* Convergence test */

  if (sqrt(dx*dx + dy*dy + dz*dz) < EPSILON){
    fl = 1;
    break;
  }

  if( aa < -GETRSDIV || aa > GETRSDIV ||
     bb < -GETRSDIV || bb > GETRSDIV ||
     cc < -GETRSDIV || cc > GETRSDIV){
    fl = -1;
    break;
  }
      }
    }
    else{ // use analytic mapping

      double n[3],X[3];;
      Vert *v[4];

      v[0] = E->vert;
      v[1] = E->vert+1;
      v[2] = E->vert+2;
      v[3] = E->vert+3;

      X[0] = 2*xo - v[1]->x - v[2]->x - v[3]->x + v[0]->x;
      X[1] = 2*yo - v[1]->y - v[2]->y - v[3]->y + v[0]->y;
      X[2] = 2*zo - v[1]->z - v[2]->z - v[3]->z + v[0]->z;

      // define normal to face 0
      n[0] = (v[1]->y - v[0]->y)*(v[2]->z - v[0]->z) -
  (v[1]->z - v[0]->z)*(v[2]->y - v[0]->y);
      n[1] = (v[1]->z - v[0]->z)*(v[2]->x - v[0]->x) -
  (v[1]->x - v[0]->x)*(v[2]->z - v[0]->z);
      n[2] = (v[1]->x - v[0]->x)*(v[2]->y - v[0]->y) -
  (v[1]->y - v[0]->y)*(v[2]->x - v[0]->x);

      tt = (X[0]*n[0] +  X[1]*n[1] + X[2]*n[2])/
  ((v[3]->x - v[0]->x)*n[0] + (v[3]->y - v[0]->y)*n[1] +
   (v[3]->z - v[0]->z)*n[2]);

      // define normal to face 1
      n[0] = (v[1]->y - v[0]->y)*(v[3]->z - v[0]->z) -
  (v[1]->z - v[0]->z)*(v[3]->y - v[0]->y);
      n[1] = (v[1]->z - v[0]->z)*(v[3]->x - v[0]->x) -
  (v[1]->x - v[0]->x)*(v[3]->z - v[0]->z);
      n[2] = (v[1]->x - v[0]->x)*(v[3]->y - v[0]->y) -
  (v[1]->y - v[0]->y)*(v[3]->x - v[0]->x);

      ss = (X[0]*n[0] +  X[1]*n[1] + X[2]*n[2])/
  ((v[2]->x - v[0]->x)*n[0] + (v[2]->y - v[0]->y)*n[1] +
   (v[2]->z - v[0]->z)*n[2]);

      // define normal to face 3
      n[0] = (v[2]->y - v[0]->y)*(v[3]->z - v[0]->z) -
  (v[2]->z - v[0]->z)*(v[3]->y - v[0]->y);
      n[1] = (v[2]->z - v[0]->z)*(v[3]->x - v[0]->x) -
  (v[2]->x - v[0]->x)*(v[3]->z - v[0]->z);
      n[2] = (v[2]->x - v[0]->x)*(v[3]->y - v[0]->y) -
  (v[2]->y - v[0]->y)*(v[3]->x - v[0]->x);

      rr = (X[0]*n[0] +  X[1]*n[1] + X[2]*n[2])/
  ((v[1]->x - v[0]->x)*n[0] + (v[1]->y - v[0]->y)*n[1] +
   (v[1]->z - v[0]->z)*n[2]);


      aa = (ss+tt)? (-2.*(1.+rr)/(ss+tt)) - 1.: rr;
      bb = (1.-tt)? ( 2.*(1.+ss)/(1.-tt)) - 1.: ss;
      cc = tt;

      // check to see if point is in element based on r,s,t
      if((rr < -1)||(ss < -1)||(tt < -1)||(rr+ss+tt > 0))
  fl = -1;
      else
  fl = 1; // assume have the correct point
    }
    break;
  case Nek_Prism:

    fl = 0;

    tt = cc;
    ss = bb;
    rr = (aa+1)*(1-tt)/2.0 - 1.0;

    while (!fl && (ic++ <= MAXIT)) {

      Prism_get_point_shape(E, aa, bb, cc, hr, hs, ht);

      xp   = eval_field_at_pt_3d(qa,qb,qc, gridx, hr, hs, ht);
      yp   = eval_field_at_pt_3d(qa,qb,qc, gridy, hr, hs, ht);
      zp   = eval_field_at_pt_3d(qa,qb,qc, gridz, hr, hs, ht);

      drdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->rx.p, hr, hs, ht);
      drdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->ry.p, hr, hs, ht);
      drdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->rz.p, hr, hs, ht);

      dsdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->sx.p, hr, hs, ht);
      dsdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->sy.p, hr, hs, ht);
      dsdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->sz.p, hr, hs, ht);

      dtdx = eval_field_at_pt_3d(qa,qb,qc, E->geom->tx.p, hr, hs, ht);
      dtdy = eval_field_at_pt_3d(qa,qb,qc, E->geom->ty.p, hr, hs, ht);
      dtdz = eval_field_at_pt_3d(qa,qb,qc, E->geom->tz.p, hr, hs, ht);

      dx    = xo - xp;                   /* Distance from the point (x,y) */
      dy    = yo - yp;
      dz    = zo - zp;

      /* Corrections to the guess */
      rr   += (drdx * dx + drdy * dy + drdz * dz);
      ss   += (dsdx * dx + dsdy * dy + dsdz * dz);
      tt   += (dtdx * dx + dtdy * dy + dtdz * dz);

      aa = (1.-tt)?( 2.*(1.+rr)/(1.-tt)) - 1.:0.0;
      bb = ss;
      cc = tt;

      /* Convergence test */

      if (sqrt(dx*dx + dy*dy + dz*dz) < EPSILON){
  fl = 1;
  break;
      }

      if( aa < -GETRSDIV || aa > GETRSDIV ||
    bb < -GETRSDIV || bb > GETRSDIV ||
    cc < -GETRSDIV || cc > GETRSDIV){
  fl = -1;
  break;
      }
    }
    break;
  }

  *a = aa;
  *b = bb;
  *c = cc;

  if(ic > MAXIT){
    return 0;
  }

  if (fl == 1){
    if( aa < -1. || aa > 1. ||
  bb < -1. || bb > 1. ||
  cc < -1. || cc > 1.)
      return 0;

    return 1;
  }

  return 0;
}

static int old_eid = 0;

void reset_starteid(int eid){
  old_eid = max(eid,0);
}

int find_elmt_coords(Element_List *U, Coord X,int *eid, Coord *A, char trip){
  if(U->fhead->dim() == 2)
    return Find_elmt_coords_2d(U,*(X.x),*(X.y),eid,A->x,A->y,trip);
  else
    return Find_elmt_coords_3d(U,*(X.x),*(X.y),*(X.z),eid,A->x,A->y,A->z,trip);
}


/* Find local (a,b,(c)) coordinates of a list of coordinates */
/* Eids[..] == -1  => that the node is outside any element   */
void Prt_find_local_coords(Element_List *U, Coord X,
         int *Eid, Coord *A, char status){
  int i;
  int dim = U->fhead->dim();

  if(!Bbox.get()){
    setup_elmt_boxes(U);
    set_mesh_coord(U);
  }

  if(status != 'P')
    reset_starteid(*Eid);

  if(dim == 2)
    find_elmt_coords_2d(U,*(X.x),*(X.y),Eid,A->x,A->y);
  else
    find_elmt_coords_3d(U,*(X.x),*(X.y),*(X.z),Eid,A->x,A->y,A->z);

}


int  Find_elmt_coords_2d(Element_List *U,
          double xo, double yo,
          int *eid, double *a, double *b){

  reset_starteid(*eid); // use value of eid to start search
  return find_elmt_coords_2d(U,xo,yo,eid,a,b);
}


int find_elmt_coords_2d(Element_List *U,
          double xo, double yo,
          int *eid, double *a, double *b){
  return Find_elmt_coords_2d(U,xo,yo,eid,a,b,'n');
}

static int Find_elmt_coords_2d(Element_List *U,
          double xo, double yo,
          int *eid, double *a, double *b, char trip){

  int fl = 0;
  int k,cnt = 0;
  double dist = .01;
  double d,sav_a,sav_b;

  /* Check all element "boxes" that contain (xo,yo,zo) */
  for(k=old_eid;k<U->nel;++k){
    if(point_in_box_2d(xo,yo,k)){
      ++cnt;
      if(Find_coords_2d(U->flist[k],xo,yo,a,b,trip)){
  *eid = k;
  old_eid = k;
  return cnt;
      }
      else{
  d = 0.;
  if((*a) > 1)
    d = ((*a)-1.)*((*a)-1.);
  else if( (*a) < -1)
    d = ((*a)+1.)*((*a)+1.);

  if((*b) > 1)
    d = max(d,((*b)-1.)*((*b)-1.));
  else if((*b) < -1)
    d = max(d,((*b)+1.)*((*b)+1.));

  if(d < dist){
    dist = d;
    fl = k+1;
    sav_a = a[0];
    sav_b = b[0];
  }
      }
    }
  }

  for(k=0;k<old_eid;++k){
    if(point_in_box_2d(xo,yo,k)){
      ++cnt;
      if(Find_coords_2d(U->flist[k],xo,yo,a,b,trip)){
  *eid = k;
  old_eid = k;
  return cnt;
      }
      else{
  d = 0.;
  if((*a) > 1)
    d = ((*a)-1.)*((*a)-1.);
  else if( (*a) < -1)
    d = ((*a)+1.)*((*a)+1.);

  if((*b) > 1)
    d = max(d,((*b)-1.)*((*b)-1.));
  else if((*b) < -1)
    d = max(d,((*b)+1.)*((*b)+1.));

  if(d < dist){
    dist = d;
    fl = k+1;
    sav_a = a[0];
    sav_b = b[0];
  }
      }
    }
  }


  /* Can put in more fancy check here */

  if(fl){
    *eid = fl-1;
    old_eid = fl-1;
    a[0] = sav_a;
    b[0] = sav_b;
    return cnt;
  }

  *eid = -1;
  return cnt;
}

int Find_elmt_coords_3d(Element_List *U,
          double xo, double yo, double zo,
          int *eid, double *a, double *b, double *c){
  reset_starteid(*eid);
  return find_elmt_coords_3d(U,xo,yo,zo,eid,a,b,c);
}


/* Find local (a,b,(c)) coordinates of a list of coordinates */

int find_elmt_coords_3d(Element_List *U,
          double xo, double yo, double zo,
      int *eid, double *a, double *b, double *c){
  return Find_elmt_coords_3d(U,xo,yo,zo,eid,a,b,c,'n');
}

static int Find_elmt_coords_3d(Element_List *U,
          double xo, double yo, double zo,
          int *eid, double *a, double *b, double *c, char trip){

  int fl = 0;
  int k,cnt = 0;
  double dist = .01;
  double d,sav_a,sav_b,sav_c;

  /* Check all element "boxes" that contain (xo,yo,zo) */
  for(k=old_eid;k<U->nel;++k){
    if(point_in_box_3d(xo,yo,zo,k)){
      ++cnt;
      if(Find_coords_3d(U->flist[k],xo,yo,zo,a,b,c,trip)){
  *eid = k;
  old_eid = k;
  return cnt;
      }
      else{
  /* if point_in_box_3d if TRUE and find_coords_3d is FALSE
     a,b,c are undefined, so it will never verify any of
     the IF statement below, for this reason

     d = 0.

     has been set.
     The same applies to the 2D routines  */

        d = 0.;
  if((*a) > 1)
    d = ((*a)-1.)*((*a)-1.);
  else if( (*a) < -1)
    d = ((*a)+1.)*((*a)+1.);

  if((*b) > 1)
    d = max(d,((*b)-1.)*((*b)-1.));
  else if((*b) < -1)
    d = max(d,((*b)+1.)*((*b)+1.));

  if((*c) > 1)
    d = max(d,((*c)-1.)*((*c)-1.));
  else if((*c) < -1)
    d = max(d,((*c)+1.)*((*c)+1.));


  if(d < dist){
    dist = d;
    fl = k+1;
    sav_a = a[0];
    sav_b = b[0];
    sav_c = c[0];
  }
      }
    }
  }

  for(k=0;k<old_eid;++k){
    if(point_in_box_3d(xo,yo,zo,k)){
      ++cnt;
      if(Find_coords_3d(U->flist[k],xo,yo,zo,a,b,c,trip)){
  *eid = k;
  old_eid = k;
  return cnt;
      }
      else{
  d = 0.;

  if((*a) > 1)
    d = ((*a)-1.)*((*a)-1.);
  else if( (*a) < -1)
    d = ((*a)+1.)*((*a)+1.);

  if((*b) > 1)
    d = max(d,((*b)-1.)*((*b)-1.));
  else if((*b) < -1)
    d = max(d,((*b)+1.)*((*b)+1.));

  if((*c) > 1)
    d = max(d,((*c)-1.)*((*c)-1.));
  else if((*c) < -1)
    d = max(d,((*c)+1.)*((*c)+1.));

  if(d < dist){
    dist = d;
    fl = k+1;
    sav_a = a[0];
    sav_b = b[0];
    sav_c = c[0];
  }
      }
    }
  }

  /* Can put in more fancy check here */

  if(fl){
    *eid = fl-1;
    old_eid = fl-1;
    a[0] = sav_a;
    b[0] = sav_b;
    c[0] = sav_c;
    return cnt;
  }

  *eid = -1;
  return cnt;
}




/* Find local (a,b,(c)) coordinates of a list of coordinates */
/* Eids[..] == -1  => that the node is outside any element   */
void Find_local_coords(Element_List *U, Coord *X, int Npts,
           int **Eids, Coord **A){

  int i;
  int dim = U->fhead->dim();

  if(!Bbox.get()){
    setup_elmt_boxes(U);
    set_mesh_coord(U);
  }

  (*Eids) = ivector(0, Npts-1);

  (*A) = (Coord*) calloc(1,sizeof(Coord));

  (*A)->x = dvector(0, Npts-1);
  (*A)->y = dvector(0, Npts-1);

  if(dim == 2){
    for(i=0;i<Npts;++i){
      if(!(i%100))
  fprintf(stderr, ".");
      find_elmt_coords_2d(U, X->x[i], X->y[i],
         (*Eids)+i, (*A)->x+i, (*A)->y+i);
      if(Eids[0][i] == -1)
  fprintf(stderr, "-");
    }

    fprintf(stderr, "\n");
  }
  else{
    (*A)->z = dvector(0, Npts-1);
    for(i=0;i<Npts;++i)
      find_elmt_coords_3d(U, X->x[i], X->y[i], X->z[i],
        (*Eids)+i, (*A)->x+i, (*A)->y+i, (*A)->z+i);
  }
}

/* Find local (a,b,(c)) coordinates of a list of coordinates */
/* Eids[..] == -1  => that the node is outside any element   */
void find_local_coords(Element_List *U, Coord *X, int Npts,
           int *Eids, Coord *A){

  int i;
  int dim = U->fhead->dim();

  if(!Bbox.get()){
    setup_elmt_boxes(U);
    set_mesh_coord(U);
  }

  if(dim == 2){
    for(i=0;i<Npts;++i){
      if(!(i%100))
  fprintf(stderr, ".");
      find_elmt_coords_2d(U, X[i].x[0], X[i].y[0], Eids+i, A[i].x, A[i].y);
      if(Eids[i] == -1)
  fprintf(stderr, "-");
    }

    fprintf(stderr, "\n");
  }
  else{
    for(i=0;i<Npts;++i)
      find_elmt_coords_3d(U, X[i].x[0], X[i].y[0], X[i].z[0],
        Eids+i, A[i].x, A[i].y, A[i].z);
  }
}

void Interpolate_field_values(Element_List **U, int Nfields, int Npts, int *Eids, Coord *A, double ***data){
  int i,j,k;
  int qa, qb;
  Element *E;
  double *za, *wa, *zb, *wb, *zc, *wc;
  double aa, bb, cc;
  double *hr = dvector(0, QGmax-1);
  double *hs = dvector(0, QGmax-1);
  double *ht = dvector(0, QGmax-1);

  *data = dmatrix(0, Nfields-1, 0, Npts-1);
  dzero(Nfields*Npts, data[0][0], 1);

  if(U[0]->fhead->dim() == 2){
    for(k=0;k<Npts;++k){
      if(Eids[k] != -1){
  E = U[0]->flist[Eids[k]];

  aa = A->x[k];
  bb = A->y[k];

  get_point_shape_2d(E, aa, bb, hr, hs);
  for(j=0;j<Nfields;++j)
    (*data)[j][k] =
      eval_field_at_pt_2d(E->qa,E->qb, U[j]->flist[E->id]->h[0], hr, hs);
      }
      else{
#if 1
  for(j=0;j<Nfields;++j)
    (*data)[j][k] = -100; // can customize
#endif
      }
    }
  }
  else{
    for(k=0;k<Npts;++k){
      if(Eids[k] != -1){
  E = U[0]->flist[Eids[k]];

  aa = A->x[k];
  bb = A->y[k];
  cc = A->z[k];

  get_point_shape_3d(E, aa, bb, cc, hr, hs, ht);
  for(j=0;j<Nfields;++j)
    (*data)[j][k] = eval_field_at_pt_3d(E->qa,E->qb,E->qc,
                U[j]->flist[E->id]->h_3d[0][0],
                hr, hs, ht);
      }
      else{
  for(j=0;j<Nfields;++j)
    (*data)[j][k] = -100; // can customize
      }
    }
  }

  free(hr);
  free(hs);
  free(ht);


}
