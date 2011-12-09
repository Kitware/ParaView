
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

Geom *Tri_gen_geofac   (Tri *E, int id);
Geom *Quad_gen_geofac  (Quad *E, int id);
Geom *Tet_gen_geofac   (Element *E, int id);
Geom *Pyr_gen_geofac   (Element *E, int id);
Geom *Prism_gen_geofac (Element *E, int id);
Geom *Hex_gen_geofac   (Element *E, int id);

Geom *Tri_gen_curved_geofac   (Tri *E, int id);
Geom *Quad_gen_curved_geofac  (Quad *E, int id);
Geom *Tet_gen_curved_geofac   (Element *E, int id);
Geom *Pyr_gen_curved_geofac   (Element *E, int id);
Geom *Prism_gen_curved_geofac (Element *E, int id);
Geom *Hex_gen_curved_geofac   (Element *E, int id);



static void Tri_DerX(Tri *E, double **DR);
static void Quad_DerX(Quad *E, double **DR);
static void Tet_DerX(Element *E, double **DR);
static void Pyr_DerX(Element *E, double **DR);
static void Prism_DerX(Element *E, double **DR);
static void Hex_DerX(Element *E, double **DR);

static int Tri_check_L(Tri *E, Geom *G);
static int Quad_check_L(Quad *E, Geom *G);
static int Tet_check_L(Element *E, Geom *G);
static int Pyr_check_L(Element *E, Geom *G);
static int Prism_check_L(Element *E, Geom *G);
static int Hex_check_L(Element *E, Geom *G);


static int same(double a, double b){
  double tol = 1e-7;
  if(a-b > tol)
    return 0;
  if(b-a > tol)
    return 0;
  return 1;
}


/*

Function name: Element::set_edge_geofac

Function Purpose:

Function Notes:

*/

void Tri::set_edge_geofac(){
  set_edge_geofac(1);
}

// if invJac = 1 then make edge weights with respect to invJac
void Tri::set_edge_geofac(int invjac){
  register int i,j;
  int      q,qedg;
  Edge     *e;
  Vert     *v;
  double   **mat,*wk1,**im,nx,ny,fac;

  double *wk = Tri_wk.get();

  wk1 = wk+QGmax;

  v = vert;

  for(i = 0; i < Nedges; ++i){
    e    = edge+i;
    qedg = e->qedg;
    e->norm = (Coord *)malloc(sizeof(Coord));
    e->norm->x = dvector(0, qedg-1);
    e->norm->y = dvector(0, qedg-1);
    e->jac  = dvector(0,qedg-1);

    /* sort out normals and put inverse of triangular jacobean in e->jac*/
    switch(i){
    case 0:
      nx =  (v[1].y - v[0].y);
      ny = -(v[1].x - v[0].x);
      if(!curvX){ /* straight sided case */
  for(j = 0; j < qedg; ++j){
    e->norm->x[j] = nx;
    e->norm->y[j] = ny;
    if(invjac)
      e->jac[j] = (0.5/geom->jac.d)*sqrt(((v[1].x-v[0].x)*(v[1].x-v[0].x)
           + (v[1].y-v[0].y)*(v[1].y-v[0].y)));
    else
      e->jac[j] = 0.5*sqrt(((v[1].x-v[0].x)*(v[1].x-v[0].x)+
          (v[1].y-v[0].y)*(v[1].y-v[0].y)));
  }
      }
      else{
  q = qa;
  getim(q,qedg,&mat,a2g);

  dcopy(q,geom->sx.p,1,wk,1);
  dvneg(q,wk,1,wk,1);
  Interp(*mat,wk,q,wk1,qedg);
  for(j = 0; j < qedg; ++j)
    e->norm->x[j] = wk1[j];

  dcopy(q,geom->sy.p,1,wk,1);
  dvneg(q,wk,1,wk,1);
  Interp(*mat,wk,q,wk1,qedg);
  for(j = 0; j < qedg; ++j)
    e->norm->y[j] = wk1[j];

  if(invjac){
    dcopy (q,geom->jac.p,1,wk,1);
    dvrecp(q,wk,1,wk,1);
    Interp(*mat,wk,q,e->jac,qedg);
  }
  else
    dfill(qedg,1,e->jac,1);
      }
      break;
    case 1:
      nx =  (v[2].y - v[1].y);
      ny = -(v[2].x - v[1].x);
      if(!curvX){
  for(j = 0; j < qedg; ++j){
    e->norm->x[j] = nx;
    e->norm->y[j] = ny;
    if(invjac)
      e->jac[j] = (0.5/geom->jac.d)*sqrt(((v[2].x-v[1].x)*(v[2].x-v[1].x)
         +   (v[2].y-v[1].y)*(v[2].y-v[1].y)));
    else
      e->jac[j] = 0.5*sqrt(((v[2].x-v[1].x)*(v[2].x-v[1].x)
          +   (v[2].y-v[1].y)*(v[2].y-v[1].y)));
  }
      }
      else{
  q = qb;
  getim(q,qedg,&im,b2g);

  dcopy(q,geom->rx.p+qa-1,qa,wk,1);
  dvadd(q,geom->sx.p+qa-1,qa,wk,1,wk,1);
  Interp(*im,wk,q,wk1,qedg);
  for(j = 0; j < qedg; ++j)
    e->norm->x[j] = wk1[j];

  dcopy(q,geom->ry.p+qa-1,qa,wk,1);
  dvadd(q,geom->sy.p+qa-1,qa,wk,1,wk,1);
  Interp(*im,wk,q,wk1,qedg);
  for(j = 0; j < qedg; ++j)
    e->norm->y[j] = wk1[j];

  if(invjac){
    dcopy (q,geom->jac.p+qa-1,qa,wk,1);
    dvrecp(q,wk,1,wk,1);
    Interp(*im,wk,q,e->jac,qedg);
  }
  else
    dfill(qedg,1,e->jac,1);
      }
      break;
    case 2:
      nx = -(v[2].y - v[0].y);
      ny =  (v[2].x - v[0].x);
      if(!curvX){
  for(j = 0; j < qedg; ++j){
    e->norm->x[j] = nx;
    e->norm->y[j] = ny;
    if(invjac)
      e->jac[j] = (0.5/geom->jac.d)*sqrt(((v[2].x-v[0].x)*(v[2].x-v[0].x)
          +  (v[2].y-v[0].y)*(v[2].y-v[0].y)));
    else
      e->jac[j] = 0.5*sqrt(((v[2].x-v[0].x)*(v[2].x-v[0].x)+
          (v[2].y-v[0].y)*(v[2].y-v[0].y)));
  }
      }
      else{
  q = qb;
  getim(q,qedg,&im,b2g);

  dcopy(q,geom->rx.p,qa,wk,1);
  dvneg(q,wk,1,wk,1);
  Interp(*im,wk,q,wk1,qedg);
  for(j = 0; j < qedg; ++j)
    e->norm->x[j] = wk1[j];

  dcopy(q,geom->ry.p,qa,wk,1);
  dvneg(q,wk,1,wk,1);
  Interp(*im,wk,q,wk1,qedg);
  for(j = 0; j < qedg; ++j)
    e->norm->y[j] = wk1[j];

  if(invjac){
    dcopy(q,geom->jac.p,qa,wk,1);
    dvrecp(q,wk,1,wk,1);
    Interp(*im,wk,q,e->jac,qedg);
  }
  else
    dfill(qedg,1,e->jac,1);
      }
      break;
    }


    /* normalise normals */
    for(j = 0; j < qedg; ++j){
      fac = sqrt(e->norm->x[j]*e->norm->x[j] + e->norm->y[j]*e->norm->y[j]);
      e->norm->x[j] /= fac;
      e->norm->y[j] /= fac;
    }

    if(curvX){
      double  *x, *y;
      double **da,**dt;
      Coord  X;

      getD(&da,&dt,&dt,&dt, &dt, &dt);

      /* get coordinates along edge and interp to edge1 */
      X.x = x = dvector(0,QGmax-1);
      X.y = y = dvector(0,QGmax-1);

      GetFaceCoord(i,&X);
      InterpToFace1(i,x,wk);
      dcopy(qa,wk,1,x,1);
      InterpToFace1(i,y,wk);
      dcopy(qa,wk,1,y,1);

      /* calculate the surface jacobian as
   sjac = sqrt((dx/da)^2 + (dy/da)^2) */
      mxva(*da,qa,1,x,1,wk,1,qa,qa);
      dcopy(qa,wk,1,x,1);
      mxva(*da,qa,1,y,1,wk,1,qa,qa);
      dcopy(qa,wk,1,y,1);

      dvmul(qa,x,1,x,1,x,1);
      dvvtvp(qa,y,1,y,1,x,1,x,1);

      dvsqrt(qa,x,1,x,1);

      /* interp to gauss points */
      getim(qa,qedg,&mat,a2g);
      Interp(*mat,x,qa,wk,qedg);

      for(j = 0; j < qedg; ++j)
  e->jac[j] *= wk[j];

      free(x); free(y);
    }
  }
}


void Quad::set_edge_geofac(){
  set_edge_geofac(1);
}

void Quad::set_edge_geofac(int invjac){
  register int i,j;
  int      q,qedg;
  Edge     *e;
  double   **mat,fac, *wk;

  wk = Quad_wk;

  for(i = 0; i < Nedges; ++i){
    e    = edge+i;
    qedg = e->qedg;
    e->norm = (Coord *)calloc(1,sizeof(Coord));
    e->norm->x = dvector(0, qedg-1);
    e->norm->y = dvector(0, qedg-1);
    e->jac     = dvector(0, qedg-1);

    q = (i == 0 || i == 2) ? qa:qb;

    getim(q,qedg,&mat,a2g);

    /* sort out normals and put inverse of triangular jacobean in e->jac*/
    switch(i){
    case 0:

      GetFace(geom->sx.p, 0, wk);
      Interp(*mat,wk,q,e->norm->x,qedg);
      dvneg(qedg,e->norm->x,1,e->norm->x,1);

      GetFace(geom->sy.p, 0, wk);
      Interp(*mat,wk,q,e->norm->y,qedg);
      dvneg(qedg,e->norm->y,1,e->norm->y,1);

      break;
    case 1:

      GetFace(geom->rx.p, 1, wk);
      Interp(*mat,wk,q,e->norm->x,qedg);

      GetFace(geom->ry.p, 1, wk);
      Interp(*mat,wk,q,e->norm->y,qedg);

      break;
    case 2:

      GetFace(geom->sx.p, 2, wk);
      Interp(*mat,wk,q,e->norm->x,qedg);

      GetFace(geom->sy.p, 2, wk);
      Interp(*mat,wk,q,e->norm->y,qedg);

      break;
    case 3:

      GetFace(geom->rx.p, 3, wk);
      Interp(*mat,wk,q,e->norm->x,qedg);
      dvneg(qedg,e->norm->x,1,e->norm->x,1);

      GetFace(geom->ry.p, 3, wk);
      Interp(*mat,wk,q,e->norm->y,qedg);
      dvneg(qedg,e->norm->y,1,e->norm->y,1);

      break;
    }

    if(invjac){
      GetFace(geom->jac.p, i, wk);
      dvrecp(qa,wk,1,wk,1);
      Interp(*mat,wk,q,e->jac,qedg);
    }
    else
      dfill(qedg,1,e->jac,1);


    /* normalise normals */
    for(j = 0; j < qedg; ++j){
      fac = sqrt(e->norm->x[j]*e->norm->x[j] + e->norm->y[j]*e->norm->y[j]);
      e->norm->x[j] /= fac;
      e->norm->y[j] /= fac;
    }

    double  *x, *y;
    double **da,**dt;
    Coord  X;

    getD(&da,&dt,&dt,&dt, &dt, &dt);

    /* get coordinates along edge and interp to edge1 */
    X.x = x = dvector(0,QGmax-1);
    X.y = y = dvector(0,QGmax-1);

    GetFaceCoord(i,&X);
    InterpToFace1(i,x,wk);
    dcopy(qa,wk,1,x,1);
    InterpToFace1(i,y,wk);
    dcopy(qa,wk,1,y,1);

    /* calculate the surface jacobian as
       sjac = sqrt((dx/da)^2 + (dy/da)^2) */
    mxva(*da,qa,1,x,1,wk,1,qa,qa);
    dcopy(qa,wk,1,x,1);
    mxva(*da,qa,1,y,1,wk,1,qa,qa);
    dcopy(qa,wk,1,y,1);

    dvmul(qa,x,1,x,1,x,1);
    dvvtvp(qa,y,1,y,1,x,1,x,1);
    dvsqrt(qa,x,1,x,1);

    /* interp to gauss points */
    getim(qa,qedg,&mat,a2g);
    Interp(*mat,x,qa,wk,qedg);

    for(j = 0; j < qedg; ++j)
      e->jac[j] *= wk[j];

    free(x); free(y);
  }
}


void Tet::set_edge_geofac(int invjac){
  fprintf(stderr,"set_edge_geoface with invjac not set up\n");
  exit(1);
}

void Tet::set_edge_geofac(){
  int i, qftot;
  double **ima, **imb;
  Bndry *Bc;
  Face *f;
  double *tmp = dvector(0, QGmax*QGmax-1);
  double *tmp1= dvector(0, QGmax*QGmax-1);
  double *tmp2= dvector(0, QGmax*QGmax-1);

  Coord dedge;
  dedge.x = dvector(0, QGmax*QGmax-1);
  dedge.y = dvector(0, QGmax*QGmax-1);
  dedge.z = dvector(0, QGmax*QGmax-1);

  // face 1
  for(i=0;i<Nfaces;++i){
    Bc = (Bndry *)calloc(1,sizeof(Bndry));
    Bc->elmt = this;
    Bc->type = 'V';
    Bc->face = i;

    Surface_geofac(Bc);

    f = face+i;
    qftot = f->qface*f->qface;
    f->n    = (Coord *)malloc(sizeof(Coord));
    f->n->x = dvector(0, qftot-1);
    f->n->y = dvector(0, qftot-1);
    f->n->z = dvector(0, qftot-1);

    f->t    = (Coord *)malloc(sizeof(Coord));
    f->t->x = dvector(0, qftot-1);
    f->t->y = dvector(0, qftot-1);
    f->t->z = dvector(0, qftot-1);

    f->b    = (Coord *)malloc(sizeof(Coord));
    f->b->x = dvector(0, qftot-1);
    f->b->y = dvector(0, qftot-1);
    f->b->z = dvector(0, qftot-1);

    f->jac    = dvector(0, qftot-1);
    // f->jac     = dvector(0, qa*qb-1);

    if(curvX){
      getim(qa,f->qface, &ima, a2g);
      getim(qb,f->qface, &imb, b2g);

      Interp2d(*ima, *imb,   Bc->nx.p, qa, qb, f->n->x, f->qface, f->qface);
      Interp2d(*ima, *imb,   Bc->ny.p, qa, qb, f->n->y, f->qface, f->qface);
      Interp2d(*ima, *imb,   Bc->nz.p, qa, qb, f->n->z, f->qface, f->qface);
      Interp2d(*ima, *imb, Bc->sjac.p, qa, qb,    tmp2, f->qface, f->qface);

      GetFace(geom->jac.p, i, tmp);
      dvrecp (qa*qb, tmp, 1, tmp1, 1);  // assumes qa=qb=1
      InterpToGaussFace(i,tmp1,f->qface,f->qface,tmp);

      dvmul(f->qface*f->qface, tmp2, 1, tmp, 1, f->jac, 1);

      free(Bc->nx.p);
      free(Bc->ny.p);
      free(Bc->nz.p);
      free(Bc->sjac.p);
    }
    else{
      dfill(qftot, Bc->nx.d, f->n->x, 1);
      dfill(qftot, Bc->ny.d, f->n->y, 1);
      dfill(qftot, Bc->nz.d, f->n->z, 1);
      dfill(qftot, Bc->sjac.d/geom->jac.d, f->jac, 1);
    }
    // Warning: This relies on the fact that the surface is no where normal
    //          to the linear edge 1
    // now calculate tangent as cross product of normal and edge 1 of face
    dfill(qftot, vert[vnum(i,1)].x - vert[vnum(i,0)].x, dedge.x, 1);
    dfill(qftot, vert[vnum(i,1)].y - vert[vnum(i,0)].y, dedge.y, 1);
    dfill(qftot, vert[vnum(i,1)].z - vert[vnum(i,0)].z, dedge.z, 1);
    normalise(f->n, qftot);
    cross_products(f->n, &dedge, f->b, qftot);
    normalise(f->b, qftot);

    cross_products(f->b, f->n, f->t, qftot);
    normalise(f->t, qftot);

    free(Bc);
  }
  free(dedge.x);
  free(dedge.y);
  free(dedge.z);
  free(tmp);  free(tmp1); free(tmp2);
}


void Pyr::set_edge_geofac(int invjac){
  fprintf(stderr,"set_edge_geofac with innjac not set up\n");
  exit(1);
}

void Pyr::set_edge_geofac(){
  fprintf(stderr,"set_edge_geofac not set up\n");
  exit(1);
}



void Prism::set_edge_geofac(int invjac){
  fprintf(stderr,"set_edge_geofac with innjac not set up\n");
  exit(1);
}


void Prism::set_edge_geofac(){
 int i, qftot;
  double **ima, **imb;
  Bndry *Bc;
  Face *f;
  double *tmp = dvector(0, QGmax*QGmax-1);
  double *tmp1 = dvector(0, QGmax*QGmax-1);
  double *tmp2= dvector(0, QGmax*QGmax-1);

  Coord dedge;
  dedge.x = dvector(0, QGmax*QGmax-1);
  dedge.y = dvector(0, QGmax*QGmax-1);
  dedge.z = dvector(0, QGmax*QGmax-1);

  // face 1
  for(i=0;i<Nfaces;++i){
    Bc = (Bndry *)calloc(1,sizeof(Bndry));
    Bc->elmt = this;
    Bc->type = 'V';
    Bc->face = i;

    Surface_geofac(Bc);

    f = face+i;
    qftot = f->qface*f->qface;
    f->n    = (Coord *)malloc(sizeof(Coord));
    f->n->x = dvector(0, qftot-1);
    f->n->y = dvector(0, qftot-1);
    f->n->z = dvector(0, qftot-1);

    f->t    = (Coord *)malloc(sizeof(Coord));
    f->t->x = dvector(0, qftot-1);
    f->t->y = dvector(0, qftot-1);
    f->t->z = dvector(0, qftot-1);

    f->b    = (Coord *)malloc(sizeof(Coord));
    f->b->x = dvector(0, qftot-1);
    f->b->y = dvector(0, qftot-1);
    f->b->z = dvector(0, qftot-1);

    f->jac     = dvector(0, qftot-1);

    if(curvX){
      if(i==0 || i==2 || i==4){
  getim(qa,f->qface, &ima, a2g);
  getim(qb,f->qface, &imb, a2g);
      }
      else{
  getim(qa,f->qface, &ima, a2g);
  getim(qb,f->qface, &imb, b2g);
      }

      Interp2d(*ima, *imb,   Bc->nx.p, qa, qb, f->n->x, f->qface, f->qface);
      Interp2d(*ima, *imb,   Bc->ny.p, qa, qb, f->n->y, f->qface, f->qface);
      Interp2d(*ima, *imb,   Bc->nz.p, qa, qb, f->n->z, f->qface, f->qface);
      Interp2d(*ima, *imb, Bc->sjac.p, qa, qb,    tmp2, f->qface, f->qface);

      GetFace(geom->jac.p, i, tmp);
      dvrecp (qa*qb, tmp, 1, tmp1, 1);  // assumes qa=qb=1
      InterpToGaussFace(i,tmp1,f->qface,f->qface,tmp);

      dvmul(f->qface*f->qface, tmp2, 1, tmp, 1, f->jac, 1);

      free(Bc->nx.p);
      free(Bc->ny.p);
      free(Bc->nz.p);
      free(Bc->sjac.p);
    }
    else{
      dfill(qftot, Bc->nx.d, f->n->x, 1);
      dfill(qftot, Bc->ny.d, f->n->y, 1);
      dfill(qftot, Bc->nz.d, f->n->z, 1);
      dfill(qftot, Bc->sjac.d/geom->jac.d, f->jac, 1);
    }
    // Warning: This relies on the face that the surface is nowhere normal
    //          to the linear edge 1
    // now calculate tangent as cross product of normal and edge 1 of face
    dfill(qftot, vert[vnum(i,1)].x - vert[vnum(i,0)].x, dedge.x, 1);
    dfill(qftot, vert[vnum(i,1)].y - vert[vnum(i,0)].y, dedge.y, 1);
    dfill(qftot, vert[vnum(i,1)].z - vert[vnum(i,0)].z, dedge.z, 1);

    normalise(f->n, qftot);
    cross_products(f->n, &dedge, f->b, qftot);
    normalise(f->b, qftot);

    cross_products(f->b, f->n, f->t, qftot);
    normalise(f->t, qftot);
    free(Bc);
  }
  free(dedge.x);
  free(dedge.y);
  free(dedge.z);
  free(tmp);
  free(tmp1);
  free(tmp2);
}


void Hex::set_edge_geofac( int invjac){
  fprintf(stderr,"set_edge_geofac with innjac not set up\n");
  exit(1);
}

void Hex::set_edge_geofac(){

   int i, qftot;
  double **ima, **imb;
  Bndry *Bc;
  Face *f;
  double *tmp = dvector(0, QGmax*QGmax-1);

  Coord dedge;
  dedge.x = dvector(0, QGmax*QGmax-1);
  dedge.y = dvector(0, QGmax*QGmax-1);
  dedge.z = dvector(0, QGmax*QGmax-1);

  // face 1
  for(i=0;i<Nfaces;++i){
    Bc = (Bndry *)calloc(1,sizeof(Bndry));
    Bc->elmt = this;
    Bc->type = 'V';
    Bc->face = i;

    Surface_geofac(Bc);

    f = face+i;
    qftot = f->qface*f->qface;
    f->n    = (Coord *)malloc(sizeof(Coord));
    f->n->x = dvector(0, qftot-1);
    f->n->y = dvector(0, qftot-1);
    f->n->z = dvector(0, qftot-1);

    f->t    = (Coord *)malloc(sizeof(Coord));
    f->t->x = dvector(0, qftot-1);
    f->t->y = dvector(0, qftot-1);
    f->t->z = dvector(0, qftot-1);

    f->b    = (Coord *)malloc(sizeof(Coord));
    f->b->x = dvector(0, qftot-1);
    f->b->y = dvector(0, qftot-1);
    f->b->z = dvector(0, qftot-1);

    f->jac     = dvector(0, qftot-1);

    if(curvX){
      getim(qa,f->qface, &ima, a2g);
      getim(qb,f->qface, &imb, a2g);

      Interp2d(*ima, *imb,  Bc->nx.p, qa, qb, f->n->x, f->qface, f->qface);
      Interp2d(*ima, *imb,  Bc->ny.p, qa, qb, f->n->y, f->qface, f->qface);
      Interp2d(*ima, *imb,  Bc->nz.p, qa, qb, f->n->z, f->qface, f->qface);

      GetFace(geom->jac.p, i, tmp);
      dvdiv  (qa*qb, Bc->sjac.p, 1, tmp, 1, tmp, 1);
      Interp2d(*ima, *imb, tmp, qa, qb, f->jac, f->qface, f->qface);

      free(Bc->nx.p);
      free(Bc->ny.p);
      free(Bc->nz.p);
      free(Bc->sjac.p);
    }
    else{
      dfill(qftot, Bc->nx.d, f->n->x, 1);
      dfill(qftot, Bc->ny.d, f->n->y, 1);
      dfill(qftot, Bc->nz.d, f->n->z, 1);
      dfill(qftot, Bc->sjac.d/geom->jac.d, f->jac, 1);
    }
    // Warning: This relies on the face that the surface is nowhere normal
    //          to the linear edge 1
    // now calculate tangent as cross product of normal and edge 1 of face
    dfill(qftot, vert[vnum(i,1)].x - vert[vnum(i,0)].x, dedge.x, 1);
    dfill(qftot, vert[vnum(i,1)].y - vert[vnum(i,0)].y, dedge.y, 1);
    dfill(qftot, vert[vnum(i,1)].z - vert[vnum(i,0)].z, dedge.z, 1);
    normalise(f->n, qftot);
    cross_products(f->n, &dedge, f->b, qftot);
    normalise(f->b, qftot);

    cross_products(f->b, f->n, f->t, qftot);
    normalise(f->t, qftot);
    free(Bc);
  }
  free(dedge.x);
  free(dedge.y);
  free(dedge.z);
}




void Element::set_edge_geofac(){ERR;}
void Element::set_edge_geofac(int invjac){ERR;}


/*

Function name: Element::set_geofac

Function Purpose:

Function Notes:

*/

static Geom    *Tri_Ginf=NULL,   *Tri_Gbase=NULL;
static Geom   *Quad_Ginf=NULL,  *Quad_Gbase=NULL;
static Geom    *Tet_Ginf=NULL,   *Tet_Gbase=NULL;
static Geom    *Pyr_Ginf=NULL,   *Pyr_Gbase=NULL;
static Geom  *Prism_Ginf=NULL, *Prism_Gbase=NULL;
static Geom    *Hex_Ginf=NULL,   *Hex_Gbase=NULL;

void Tri::set_geofac(){
  register int i;
  int      idg = iparam("FAMILIES");
  Vert    *v;

  if(option("FAMOFF")){
    if(geom){
      idg = geom->id;
      if(curvX){ /* set curved element */
  // possible leak here
  if(geom->sy.p)
    free(geom->sy.p);
  free(geom);
  geom = Tri_gen_curved_geofac(this,idg);
      }
      else{
  free(geom);
  geom = Tri_gen_geofac(this,idg);
      }
    }
    else{
      /* set curved element */
      if(curvX)
  geom = Tri_gen_curved_geofac(this,idg++);
      else
  geom = Tri_gen_geofac(this,idg++);
      iparam_set("FAMILIES",idg);
    }
    return;
  }

  if(curvX) /* set curved element */
    geom = Tri_gen_curved_geofac(this,idg++);
  else{           /* check link list */
    v = vert;
    for(Tri_Ginf = Tri_Gbase; Tri_Ginf; Tri_Ginf = Tri_Ginf->next){
      for(i = 1; i < Nverts; ++i){
  if(((v[i].x - v[0].x) != Tri_Ginf->dx[i-1])||
     ((v[i].y - v[0].y) != Tri_Ginf->dy[i-1]))
    break;
      }
      if((i == Nverts)&&(Tri_check_L(this,Tri_Ginf))){
  geom = Tri_Ginf;
  goto End;
      }
    }
    /* Else add Differential matrices */
    Tri_Ginf  = Tri_Gbase;
    Tri_Gbase = Tri_gen_geofac(this,idg++);
    Tri_Gbase->next = Tri_Ginf;
    geom = Tri_Gbase;
  End:;
  }

  iparam_set("FAMILIES",idg);
  return;
}




void Quad::set_geofac(){
  register int i;
  int      idg = iparam("FAMILIES");
  Vert    *v;

  if(option("FAMOFF")){
    if(geom){
      idg = geom->id;
      free(geom->sy.p);
      free(geom);
      geom = Quad_gen_curved_geofac(this,idg);
    }
    else{
      geom = Quad_gen_curved_geofac(this,idg++);
      iparam_set("FAMILIES",idg);
    }
    return;
  }

  if(curve  && curve->type!=T_Straight) /* set curved element */
    geom = Quad_gen_curved_geofac(this,idg++);
  else{           /* check link list */
    v = vert;
    for(Quad_Ginf = Quad_Gbase; Quad_Ginf; Quad_Ginf = Quad_Ginf->next){
      for(i = 1; i < Nverts; ++i){
  //  if(((v[i].x - v[0].x) != Quad_Ginf->dx[i-1])||
  //     ((v[i].y - v[0].y) != Quad_Ginf->dy[i-1]))
  if(!same(v[i].x - v[0].x,Quad_Ginf->dx[i-1])||
     !same(v[i].y - v[0].y,Quad_Ginf->dy[i-1]))
    break;
      }
      if((i == Nverts)&&(Quad_check_L(this,Quad_Ginf))){
  geom = Quad_Ginf;
  goto End;
      }
    }
    /* Else add Differential matrices */
    Quad_Ginf  = Quad_Gbase;
    Quad_Gbase = Quad_gen_geofac(this,idg++);
    Quad_Gbase->next = Quad_Ginf;
    geom = Quad_Gbase;
  End:;
  }

  iparam_set("FAMILIES",idg);
  return;
}




void Tet::set_geofac(){
  register int i;
  int      idg = iparam("FAMILIES");
  Vert    *v;

  if(option("FAMOFF")){
    if(geom){
      idg = geom->id;
      if(curvX){ /* set curved element */
  // possible leak here
  if(geom->rx.p)
    free(geom->rx.p);
  free(geom);
  geom = Tet_gen_curved_geofac(this,idg);
      }
      else{
  free(geom);
  geom = Tet_gen_geofac(this,idg);
      }
    }
    else{
      /* set curved element */
      if(curvX)
  geom = Tet_gen_curved_geofac(this,idg++);
      else
  geom = Tet_gen_geofac(this,idg++);
      iparam_set("FAMILIES",idg);
    }
    return;
  }

  if(curvX) /* set curved element */
    geom = Tet_gen_curved_geofac(this,idg++);
  else{           /* check link list */
    v = vert;
    for(Tet_Ginf = Tet_Gbase; Tet_Ginf; Tet_Ginf = Tet_Ginf->next){
      for(i = 1; i < Nverts; ++i){
  if(((v[i].x - v[0].x) != Tet_Ginf->dx[i-1])||
     ((v[i].y - v[0].y) != Tet_Ginf->dy[i-1])||
     ((v[i].z - v[0].z) != Tet_Ginf->dz[i-1]))
    break;
      }
      if((i == Nverts)&&(Tet_check_L(this,Tet_Ginf))){
  geom = Tet_Ginf;
  goto End;
      }
    }
    /* Else add Differential maTetces */
    Tet_Ginf  = Tet_Gbase;
    Tet_Gbase = Tet_gen_geofac(this,idg++);
    Tet_Gbase->next = Tet_Ginf;
    geom = Tet_Gbase;
  End:;
  }

  iparam_set("FAMILIES",idg);
  return;
}


void Pyr::set_geofac(){
  register int i;
  int      idg = iparam("FAMILIES");
  Vert    *v;


  if(option("FAMOFF")){
    if(geom){
      idg = geom->id;
      free_geofac();
      // possible leak here
      if(curvX) /* set curved element */
  geom = Pyr_gen_curved_geofac(this,idg);
      else
  geom = Pyr_gen_curved_geofac(this,idg);
    }
    else{
      /* set curved element */
      if(curvX)
  geom = Pyr_gen_curved_geofac(this,idg++);
      else
  geom = Pyr_gen_curved_geofac(this,idg++);
      iparam_set("FAMILIES",idg);
    }
    return;
  }


  if(curvX) /* set curved element */
    geom = Pyr_gen_curved_geofac(this,idg++);
  else{           /* check link list */
    v = vert;
    for(Pyr_Ginf = Pyr_Gbase; Pyr_Ginf; Pyr_Ginf = Pyr_Ginf->next){
      for(i = 1; i < NPyr_verts; ++i){
  if(((v[i].x - v[0].x) != Pyr_Ginf->dx[i-1])||
     ((v[i].y - v[0].y) != Pyr_Ginf->dy[i-1])||
     ((v[i].z - v[0].z) != Pyr_Ginf->dz[i-1]))
    break;
      }
      if((i == NPyr_verts)&&(Pyr_check_L(this,Pyr_Ginf))){
  geom = Pyr_Ginf;
  goto End;
      }
    }
    /* Else add Differential maPyrces */
    Pyr_Ginf  = Pyr_Gbase;
    Pyr_Gbase = Pyr_gen_geofac(this,idg++);
    Pyr_Gbase->next = Pyr_Ginf;
    geom = Pyr_Gbase;
  End:;
  }

  iparam_set("FAMILIES",idg);
  return;
}




void Prism::set_geofac(){
  register int i;
  int      idg = iparam("FAMILIES");
  Vert    *v;

 if(option("FAMOFF")){
    if(geom){
      idg = geom->id;
      free_geofac();
      // possible leak here
      if(curvX) /* set curved element */
  geom = Prism_gen_curved_geofac(this,idg);
      else
  geom = Prism_gen_geofac(this,idg);
    }
    else{
      /* set curved element */
      if(curvX)
  geom = Prism_gen_curved_geofac(this,idg++);
      else
  geom = Prism_gen_geofac(this,idg++);
      iparam_set("FAMILIES",idg);
    }
    return;
  }


  if(curvX) /* set curved element */
    geom = Prism_gen_curved_geofac(this,idg++);
  else{           /* check link list */
    v = vert;
    for(Prism_Ginf = Prism_Gbase; Prism_Ginf; Prism_Ginf = Prism_Ginf->next){
      for(i = 1; i < NPrism_verts; ++i){
  if(((v[i].x - v[0].x) != Prism_Ginf->dx[i-1])||
     ((v[i].y - v[0].y) != Prism_Ginf->dy[i-1])||
     ((v[i].z - v[0].z) != Prism_Ginf->dz[i-1]))
    break;
      }
      if((i == NPrism_verts)&&(Prism_check_L(this,Prism_Ginf))){
  geom = Prism_Ginf;
  goto End;
      }
    }
    /* Else add Differential maPrismces */
    Prism_Ginf  = Prism_Gbase;
    Prism_Gbase = Prism_gen_geofac(this,idg++);
    Prism_Gbase->next = Prism_Ginf;
    geom = Prism_Gbase;
  End:;
  }

  iparam_set("FAMILIES",idg);
  return;
}




void Hex::set_geofac(){
  register int i;
  int      idg = iparam("FAMILIES");
  Vert    *v;

  if(option("FAMOFF")){
    if(geom){
      idg = geom->id;
      free_geofac();
      // possible leak here
      if(curvX) /* set curved element */
  geom = Hex_gen_curved_geofac(this,idg);
      else
  geom = Hex_gen_curved_geofac(this,idg);
    }
    else{
      /* set curved element */
      if(curvX)
  geom = Hex_gen_curved_geofac(this,idg++);
      else
  geom = Hex_gen_curved_geofac(this,idg++);
      iparam_set("FAMILIES",idg);
    }
    return;
  }
  // MUST FIX
  if(curvX && curve->type!=T_Straight) /* set curved element */
    geom = Hex_gen_curved_geofac(this,idg++);
  else{           /* check link list */
    v = vert;
    for(Hex_Ginf = Hex_Gbase; Hex_Ginf; Hex_Ginf = Hex_Ginf->next){
      for(i = 1; i < NHex_verts; ++i){
  if(((v[i].x - v[0].x) != Hex_Ginf->dx[i-1])||
     ((v[i].y - v[0].y) != Hex_Ginf->dy[i-1])||
     ((v[i].z - v[0].z) != Hex_Ginf->dz[i-1]))
    break;
      }
      if((i == NHex_verts)&&(Hex_check_L(this,Hex_Ginf))){
  geom = Hex_Ginf;
  goto End;
      }
    }
    /* Else add Differential maHexces */
    Hex_Ginf  = Hex_Gbase;
    Hex_Gbase = Hex_gen_geofac(this,idg++);
    Hex_Gbase->next = Hex_Ginf;
    geom = Hex_Gbase;
  End:;
  }

  iparam_set("FAMILIES",idg);
  return;
}




void Element::set_geofac(){ERR;}







Geom *Tri_gen_geofac(Tri *E, int id){
  register int i;
  double   xr,xs,yr,ys;
  Geom    *G = (Geom *)calloc(1,sizeof(Geom));
  Vert    *v = E->vert;

  G->id = id;
  for(i = 1; i < E->Nverts; ++i){
    G->dx[i-1] = v[i].x - v[0].x;
    G->dy[i-1] = v[i].y - v[0].y;
  }
  G->elmt = E;

  xr = (v[1].x - v[0].x)/2.0;
  xs = (v[2].x - v[0].x)/2.0;
  yr = (v[1].y - v[0].y)/2.0;
  ys = (v[2].y - v[0].y)/2.0;

  //  G->jac.d =  fabs(xr*ys - xs*yr);

  G->jac.d = xr*ys - xs*yr;
  if(G->jac.d < 0.){
    G->singular = 1;
    fprintf(stderr, "Tri_gen_geofac: Elmt: %d has a -ve Jacobian\n",
      E->id+1);
  }
  else
    G->singular = 0;
  G->jac.d = fabs(G->jac.d);

  G->rx.d  =  ys/G->jac.d;
  G->ry.d  = -xs/G->jac.d;
  G->sx.d  = -yr/G->jac.d;
  G->sy.d  =  xr/G->jac.d;

/*  fprintf(stderr, "Elmt: %d rx: %lf ry: %lf sx: %lf sy: %lf jac: %lf\n",
    id, G->rx.d, G->ry.d, G->sx.d, G->sy.d, G->jac.d);*/

  return G;
}



Geom *Quad_gen_geofac(Quad *E, int id){
  double   xr,xs,yr,ys;
  Geom    *G = (Geom *)malloc(sizeof(Geom));
  Vert    *v = E->vert;

  if(E->curve->type == T_Straight)
    return Quad_gen_curved_geofac(E,id);

  G->id = id;
  for(int i = 1; i < E->Nverts; ++i){
    G->dx[i-1] = v[i].x - v[0].x;
    G->dy[i-1] = v[i].y - v[0].y;
  }
  G->elmt = E;

  xr = (v[1].x - v[0].x)/2.0;
  xs = (v[2].x - v[0].x)/2.0;
  yr = (v[1].y - v[0].y)/2.0;
  ys = (v[2].y - v[0].y)/2.0;

  G->jac.d = xr*ys - xs*yr;
  if(G->jac.d < 0.) {
    fprintf(stderr, "Quad: gen_geofac -ve Jacobianin Elmt: %d\n",
      E->id);
      G->singular = 1;
  }
  else
    G->singular = 0;
  G->jac.d =  fabs(G->jac.d); // fabs(xr*ys - xs*yr);

  G->rx.d  =  ys/G->jac.d;
  G->ry.d  = -xs/G->jac.d;
  G->sx.d  = -yr/G->jac.d;
  G->sy.d  =  xr/G->jac.d;

  return G;
}



Geom *Tet_gen_geofac(Element *E, int id){
  register int i;
  double   xr,xs,yr,ys;
  Geom    *G = (Geom *)malloc(sizeof(Geom));
  Vert    *v = E->vert;
  double   xt,yt,zr,zs,zt;

  G->id = id;
  for(i = 1; i < NTet_verts; ++i){
    G->dx[i-1] = v[i].x - v[0].x;
    G->dy[i-1] = v[i].y - v[0].y;
    G->dz[i-1] = v[i].z - v[0].z;
  }
  G->elmt = E;

  xr = (v[1].x - v[0].x)/2.0;
  xs = (v[2].x - v[0].x)/2.0;
  yr = (v[1].y - v[0].y)/2.0;
  ys = (v[2].y - v[0].y)/2.0;
  zr = (v[1].z - v[0].z)/2.0;
  zs = (v[2].z - v[0].z)/2.0;

  xt = (v[3].x - v[0].x)/2.0;
  yt = (v[3].y - v[0].y)/2.0;
  zt = (v[3].z - v[0].z)/2.0;

  G->jac.d = xr*(ys*zt-zs*yt) - yr*(xs*zt-zs*xt) + zr*(xs*yt-ys*xt);

  if(G->jac.d < 0.){
    fprintf(stderr, "Tet_gen_geofac: -ve Jacobian in Elmt:%d\n", E->id+1);
    G->singular = 1;
  }
  else
    G->singular = 0;

  G->jac.d = fabs(G->jac.d);
  //  G->jac.d = fabs(xr*(ys*zt-zs*yt) - yr*(xs*zt-zs*xt) + zr*(xs*yt-ys*xt));

  G->rx.d  =  (ys*zt - zs*yt)/G->jac.d;
  G->ry.d  = -(xs*zt - zs*xt)/G->jac.d;
  G->rz.d  =  (xs*yt - ys*xt)/G->jac.d;

  G->sx.d  = -(yr*zt - zr*yt)/G->jac.d;
  G->sy.d  =  (xr*zt - zr*xt)/G->jac.d;
  G->sz.d  = -(xr*yt - yr*xt)/G->jac.d;

  G->tx.d  =  (yr*zs - zr*ys)/G->jac.d;
  G->ty.d  = -(xr*zs - zr*xs)/G->jac.d;
  G->tz.d  =  (xr*ys - yr*xs)/G->jac.d;

  return G;
}





Geom *Pyr_gen_geofac(Element *E, int id){
  register int i;
  double   xr,xs,yr,ys;
  Geom    *G = (Geom *)malloc(sizeof(Geom));
  Vert    *v = E->vert;

  double   xt,yt,zr,zs,zt;

  G->id = id;
  for(i = 1; i < NPyr_verts; ++i){
    G->dx[i-1] = v[i].x - v[0].x;
    G->dy[i-1] = v[i].y - v[0].y;
    G->dz[i-1] = v[i].z - v[0].z;
  }
  G->elmt = E;

  xr = (v[1].x - v[0].x)/2.0;
  xs = (v[2].x - v[0].x)/2.0;
  yr = (v[1].y - v[0].y)/2.0;
  ys = (v[2].y - v[0].y)/2.0;
  zr = (v[1].z - v[0].z)/2.0;
  zs = (v[2].z - v[0].z)/2.0;

  xt = (v[3].x - v[0].x)/2.0;
  yt = (v[3].y - v[0].y)/2.0;
  zt = (v[3].z - v[0].z)/2.0;

  G->jac.d = fabs(xr*(ys*zt-zs*yt) - yr*(xs*zt-zs*xt) + zr*(xs*yt-ys*xt));

  G->rx.d  =  (ys*zt - zs*yt)/G->jac.d;
  G->ry.d  = -(xs*zt - zs*xt)/G->jac.d;
  G->rz.d  =  (xs*yt - ys*xt)/G->jac.d;

  G->sx.d  = -(yr*zt - zr*yt)/G->jac.d;
  G->sy.d  =  (xr*zt - zr*xt)/G->jac.d;
  G->sz.d  = -(xr*yt - yr*xt)/G->jac.d;

  G->tx.d  =  (yr*zs - zr*ys)/G->jac.d;
  G->ty.d  = -(xr*zs - zr*xs)/G->jac.d;
  G->tz.d  =  (xr*ys - yr*xs)/G->jac.d;

  return G;
}



Geom *Prism_gen_geofac(Element *E, int id){
  register int i;
  double   xr,xs,yr,ys;
  Geom    *G = (Geom *)calloc(1,sizeof(Geom));
  Vert    *v = E->vert;

  double   xt,yt,zr,zs,zt;

  G->id = id;
  for(i = 1; i < NPrism_verts; ++i){
    G->dx[i-1] = v[i].x - v[0].x;
    G->dy[i-1] = v[i].y - v[0].y;
    G->dz[i-1] = v[i].z - v[0].z;
  }
  G->elmt = E;

  xr = (v[1].x - v[0].x)/2.0;
  xs = (v[2].x - v[0].x)/2.0;
  yr = (v[1].y - v[0].y)/2.0;
  ys = (v[2].y - v[0].y)/2.0;
  zr = (v[1].z - v[0].z)/2.0;
  zs = (v[2].z - v[0].z)/2.0;

  xt = (v[3].x - v[0].x)/2.0;
  yt = (v[3].y - v[0].y)/2.0;
  zt = (v[3].z - v[0].z)/2.0;

  G->jac.d = fabs(xr*(ys*zt-zs*yt) - yr*(xs*zt-zs*xt) + zr*(xs*yt-ys*xt));

  if(G->jac.d < 0.){
    fprintf(stderr, "Prism_gen_geofac: -ve Jacobian in Elmt:%d\n", E->id+1);
    G->singular = 1;
  }
  else
    G->singular = 0;



  G->rx.d  =  (ys*zt - zs*yt)/G->jac.d;
  G->ry.d  = -(xs*zt - zs*xt)/G->jac.d;
  G->rz.d  =  (xs*yt - ys*xt)/G->jac.d;

  G->sx.d  = -(yr*zt - zr*yt)/G->jac.d;
  G->sy.d  =  (xr*zt - zr*xt)/G->jac.d;
  G->sz.d  = -(xr*yt - yr*xt)/G->jac.d;

  G->tx.d  =  (yr*zs - zr*ys)/G->jac.d;
  G->ty.d  = -(xr*zs - zr*xs)/G->jac.d;
  G->tz.d  =  (xr*ys - yr*xs)/G->jac.d;

  return G;
}




Geom *Hex_gen_geofac(Element *E, int id){
  register int i;
  Vert    *v = E->vert;

  Geom *G = Hex_gen_curved_geofac(E,id);

  for(i = 1; i < NHex_verts; ++i){
    G->dx[i-1] = v[i].x - v[0].x;
    G->dy[i-1] = v[i].y - v[0].y;
    G->dz[i-1] = v[i].z - v[0].z;
  }


  return G;
}



/*

Function name: Element_gen_curved_geofac

Function Purpose:




Function Notes:

*/






Geom *Tri_gen_curved_geofac(Tri *E, int id){
  int      qt,sing;
  const    int qa = E->qa, qb = E->qb;
  double   **DR;
  Geom     *G = (Geom *)calloc(1,sizeof(Geom));

  G->id     = id;
  G->elmt   = E;

  qt  = qa*qb;

  double *d = dvector(0, 5*qt-1);
  DR = (double**) calloc(5,sizeof(double*));
  DR[0] = d; DR[1] = d+qt; DR[2] = d+2*qt; DR[3] = d+3*qt; DR[4] = d+4*qt;

  //  DR  = dmatrix(0,4,0,qt-1);

  Tri_DerX(E,DR); /* calc dX/d(r/s/t) */

  /* calculate Jacobean */
  dvmul  (qt, DR[1], 1, DR[2], 1, DR[4], 1);
  dvvtvm (qt, DR[0], 1, DR[3], 1, DR[4], 1, DR[4], 1);

  if(DR[4][sing = idmin(qt,DR[4],1)] < 0.0){
    G->singular = sing+1;
  }
  else
    G->singular = 0;

  dvabs  (qt, DR[4], 1, DR[4], 1);

  /* calculate reverse geometric derivatives */
  dvdiv  (qt, DR[0], 1, DR[4], 1, DR[0], 1);
  dvdiv  (qt, DR[1], 1, DR[4], 1, DR[1], 1);
  dvdiv  (qt, DR[2], 1, DR[4], 1, DR[2], 1);
  dvdiv  (qt, DR[3], 1, DR[4], 1, DR[3], 1);

  dvneg  (qt, DR[1], 1, DR[1], 1);
  dvneg  (qt, DR[2], 1, DR[2], 1);

  G->jac.p  = DR[4];

  G->rx.p   = DR[3];
  G->ry.p   = DR[1];
  G->sx.p   = DR[2];
  G->sy.p   = DR[0];

  free(DR);
  return G;
}




Geom *Quad_gen_curved_geofac(Quad *E, int id){
  int      qt,sing;
  const    int qa = E->qa, qb = E->qb;
  double   **DR;
  Geom     *G = (Geom *)malloc(sizeof(Geom));

  G->id     = id;
  G->elmt   = E;

  for(int i = 1; i < E->Nverts; ++i){
    G->dx[i-1] = E->vert[i].x - E->vert[0].x;
    G->dy[i-1] = E->vert[i].y - E->vert[0].y;
  }

  qt  = qa*qb;
  DR  = dmatrix(0,4,0,qt-1);

  Quad_DerX(E,DR); /* calc dX/d(r/s/t) */

  /* calculate Jacobean */
  dvmul  (qt, DR[1], 1, DR[2], 1, DR[4], 1);
  dvvtvm (qt, DR[0], 1, DR[3], 1, DR[4], 1, DR[4], 1);

  if(DR[4][sing = idmin(qt,DR[4],1)] < 0.0){
    G->singular = sing+1;
  }
  else
    G->singular = 0;
  dvabs  (qt, DR[4], 1, DR[4], 1);

  /* calculate reverse geometric derivatives */
  dvdiv  (qt, DR[0], 1, DR[4], 1, DR[0], 1);
  dvdiv  (qt, DR[1], 1, DR[4], 1, DR[1], 1);
  dvdiv  (qt, DR[2], 1, DR[4], 1, DR[2], 1);
  dvdiv  (qt, DR[3], 1, DR[4], 1, DR[3], 1);

  dvneg  (qt, DR[1], 1, DR[1], 1);
  dvneg  (qt, DR[2], 1, DR[2], 1);

  G->jac.p  = DR[4];

  G->rx.p   = DR[3];
  G->ry.p   = DR[1];
  G->sx.p   = DR[2];
  G->sy.p   = DR[0];

  free(DR);
  return G;
}



Geom *Tet_gen_curved_geofac(Element *E, int id){
 int      qt,sing;
  const    int qa = E->qa, qb = E->qb;
  double   **DR;
  Geom     *G = (Geom *)calloc(1,sizeof(Geom));

  register int i;
  const    int qc = E->qc;
  double   **DX;

  G->id     = id;
  G->elmt   = E;

  qt  = qa*qb*qc;
  DR  = dmatrix(0,8,0,qt-1); /* work space in 3d */
  DX  = dmatrix(0,9,0,qt-1);

  Tet_DerX(E,DR); /* calc dX/d(r/s/t) */

   /* follows same order as scalar case in add_geofac */
  dvmul  (qt, DR[7], 1, DR[5], 1, DX[0], 1);
  dvvtvm (qt, DR[4], 1, DR[8], 1, DX[0], 1, DX[0], 1);
  dvmul  (qt, DR[7], 1, DR[2], 1, DX[1], 1);
  dvvtvm (qt, DR[1], 1, DR[8], 1, DX[1], 1, DX[1], 1);
  dvmul  (qt, DR[4], 1, DR[2], 1, DX[2], 1);
  dvvtvm (qt, DR[1], 1, DR[5], 1, DX[2], 1, DX[2], 1);
  dvneg  (qt, DX[1], 1, DX[1], 1);

  dvmul  (qt, DR[6], 1, DR[5], 1, DX[3], 1);
  dvvtvm (qt, DR[3], 1, DR[8], 1, DX[3], 1, DX[3], 1);
  dvmul  (qt, DR[6], 1, DR[2], 1, DX[4], 1);
  dvvtvm (qt, DR[0], 1, DR[8], 1, DX[4], 1, DX[4], 1);
  dvmul  (qt, DR[3], 1, DR[2], 1, DX[5], 1);
  dvvtvm (qt, DR[0], 1, DR[5], 1, DX[5], 1, DX[5], 1);
  dvneg  (qt, DX[3], 1, DX[3], 1);
  dvneg  (qt, DX[5], 1, DX[5], 1);

  dvmul  (qt, DR[6], 1, DR[4], 1, DX[6], 1);
  dvvtvm (qt, DR[3], 1, DR[7], 1, DX[6], 1, DX[6], 1);
  dvmul  (qt, DR[6], 1, DR[1], 1, DX[7], 1);
  dvvtvm (qt, DR[0], 1, DR[7], 1, DX[7], 1, DX[7], 1);
  dvmul  (qt, DR[3], 1, DR[1], 1, DX[8], 1);
  dvvtvm (qt, DR[0], 1, DR[4], 1, DX[8], 1, DX[8], 1);
  dvneg  (qt, DX[7], 1, DX[7], 1);

  /* compute Jacobian */
  dvmul  (qt, DR[0], 1, DX[0], 1, DX[9], 1);
  dvvtvp (qt, DR[3], 1, DX[1], 1, DX[9], 1, DX[9], 1);
  dvvtvp (qt, DR[6], 1, DX[2], 1, DX[9], 1, DX[9], 1);

  /* divide factors by Jacobian */
  for(i = 0; i < 9; ++i) dvdiv(qt, DX[i], 1, DX[9], 1, DX[i], 1);

  if(DX[9][sing = idmin(qt,DX[9],1)] < 0.0){
    G->singular = sing+1;
  }
  else{
    G->singular = 0;
  }
  /* take abs. value of Jacobian */
  dvabs  (qt, DX[9], 1, DX[9], 1);

  /* set pointers */
  G->rx.p  = DX[0];
  G->ry.p  = DX[1];
  G->rz.p  = DX[2];

  G->sx.p  = DX[3];
  G->sy.p  = DX[4];
  G->sz.p  = DX[5];

  G->tx.p  = DX[6];
  G->ty.p  = DX[7];
  G->tz.p  = DX[8];

  G->jac.p = DX[9];

  free_dmatrix(DR,0,0);  free(DX);

  return G;
}



Geom *Pyr_gen_curved_geofac(Element *E, int id){
  int      qt,sing;
  const    int qa = E->qa, qb = E->qb;
  double   **DR;
  Geom     *G = (Geom *)calloc(1,sizeof(Geom));

  register int i;
  const    int qc = E->qc;
  double   **DX;

  G->id     = id;
  G->elmt   = E;

  qt  = qa*qb*qc;
  DR  = dmatrix(0,8,0,qt-1); /* work space in 3d */
  DX  = dmatrix(0,9,0,qt-1);

  Pyr_DerX(E,DR); /* calc dX/d(r/s/t) */

   /* follows same order as scalar case in add_geofac */
  dvmul  (qt, DR[7], 1, DR[5], 1, DX[0], 1);
  dvvtvm (qt, DR[4], 1, DR[8], 1, DX[0], 1, DX[0], 1);
  dvmul  (qt, DR[7], 1, DR[2], 1, DX[1], 1);
  dvvtvm (qt, DR[1], 1, DR[8], 1, DX[1], 1, DX[1], 1);
  dvmul  (qt, DR[4], 1, DR[2], 1, DX[2], 1);
  dvvtvm (qt, DR[1], 1, DR[5], 1, DX[2], 1, DX[2], 1);
  dvneg  (qt, DX[1], 1, DX[1], 1);

  dvmul  (qt, DR[6], 1, DR[5], 1, DX[3], 1);
  dvvtvm (qt, DR[3], 1, DR[8], 1, DX[3], 1, DX[3], 1);
  dvmul  (qt, DR[6], 1, DR[2], 1, DX[4], 1);
  dvvtvm (qt, DR[0], 1, DR[8], 1, DX[4], 1, DX[4], 1);
  dvmul  (qt, DR[3], 1, DR[2], 1, DX[5], 1);
  dvvtvm (qt, DR[0], 1, DR[5], 1, DX[5], 1, DX[5], 1);
  dvneg  (qt, DX[3], 1, DX[3], 1);
  dvneg  (qt, DX[5], 1, DX[5], 1);

  dvmul  (qt, DR[6], 1, DR[4], 1, DX[6], 1);
  dvvtvm (qt, DR[3], 1, DR[7], 1, DX[6], 1, DX[6], 1);
  dvmul  (qt, DR[6], 1, DR[1], 1, DX[7], 1);
  dvvtvm (qt, DR[0], 1, DR[7], 1, DX[7], 1, DX[7], 1);
  dvmul  (qt, DR[3], 1, DR[1], 1, DX[8], 1);
  dvvtvm (qt, DR[0], 1, DR[4], 1, DX[8], 1, DX[8], 1);
  dvneg  (qt, DX[7], 1, DX[7], 1);

  /* compute Jacobian */
  dvmul  (qt, DR[0], 1, DX[0], 1, DX[9], 1);
  dvvtvp (qt, DR[3], 1, DX[1], 1, DX[9], 1, DX[9], 1);
  dvvtvp (qt, DR[6], 1, DX[2], 1, DX[9], 1, DX[9], 1);

  /* divide factors by Jacobian */
  for(i = 0; i < 9; ++i) dvdiv(qt, DX[i], 1, DX[9], 1, DX[i], 1);

  if(DX[9][sing = idmin(qt,DX[9],1)] < 0.0)
    G->singular = sing+1;
  else
    G->singular = 0;

  /* take abs. value of Jacobian */
  dvabs  (qt, DX[9], 1, DX[9], 1);

  /* set pointers */
  G->rx.p  = DX[0];
  G->ry.p  = DX[1];
  G->rz.p  = DX[2];

  G->sx.p  = DX[3];
  G->sy.p  = DX[4];
  G->sz.p  = DX[5];

  G->tx.p  = DX[6];
  G->ty.p  = DX[7];
  G->tz.p  = DX[8];

  G->jac.p = DX[9];

  free_dmatrix(DR,0,0);  free(DX);

  return G;
}




Geom *Prism_gen_curved_geofac(Element *E, int id){
  int      qt,sing;
  const    int qa = E->qa, qb = E->qb;
  double   **DR;
  Geom     *G = (Geom *)calloc(1,sizeof(Geom));

  register int i;
  const    int qc = E->qc;
  double   **DX;

  G->id     = id;
  G->elmt   = E;

  qt  = qa*qb*qc;
  DR  = dmatrix(0,8,0,qt-1); /* work space in 3d */
  DX  = dmatrix(0,9,0,qt-1);

  Prism_DerX(E,DR); /* calc dX/d(r/s/t) */

   /* follows same order as scalar case in add_geofac */
  dvmul  (qt, DR[7], 1, DR[5], 1, DX[0], 1);
  dvvtvm (qt, DR[4], 1, DR[8], 1, DX[0], 1, DX[0], 1);
  dvmul  (qt, DR[7], 1, DR[2], 1, DX[1], 1);
  dvvtvm (qt, DR[1], 1, DR[8], 1, DX[1], 1, DX[1], 1);
  dvmul  (qt, DR[4], 1, DR[2], 1, DX[2], 1);
  dvvtvm (qt, DR[1], 1, DR[5], 1, DX[2], 1, DX[2], 1);
  dvneg  (qt, DX[1], 1, DX[1], 1);

  dvmul  (qt, DR[6], 1, DR[5], 1, DX[3], 1);
  dvvtvm (qt, DR[3], 1, DR[8], 1, DX[3], 1, DX[3], 1);
  dvmul  (qt, DR[6], 1, DR[2], 1, DX[4], 1);
  dvvtvm (qt, DR[0], 1, DR[8], 1, DX[4], 1, DX[4], 1);
  dvmul  (qt, DR[3], 1, DR[2], 1, DX[5], 1);
  dvvtvm (qt, DR[0], 1, DR[5], 1, DX[5], 1, DX[5], 1);
  dvneg  (qt, DX[3], 1, DX[3], 1);
  dvneg  (qt, DX[5], 1, DX[5], 1);

  dvmul  (qt, DR[6], 1, DR[4], 1, DX[6], 1);
  dvvtvm (qt, DR[3], 1, DR[7], 1, DX[6], 1, DX[6], 1);
  dvmul  (qt, DR[6], 1, DR[1], 1, DX[7], 1);
  dvvtvm (qt, DR[0], 1, DR[7], 1, DX[7], 1, DX[7], 1);
  dvmul  (qt, DR[3], 1, DR[1], 1, DX[8], 1);
  dvvtvm (qt, DR[0], 1, DR[4], 1, DX[8], 1, DX[8], 1);
  dvneg  (qt, DX[7], 1, DX[7], 1);

  /* compute Jacobian */
  dvmul  (qt, DR[0], 1, DX[0], 1, DX[9], 1);
  dvvtvp (qt, DR[3], 1, DX[1], 1, DX[9], 1, DX[9], 1);
  dvvtvp (qt, DR[6], 1, DX[2], 1, DX[9], 1, DX[9], 1);

  /* divide factors by Jacobian */
  for(i = 0; i < 9; ++i) dvdiv(qt, DX[i], 1, DX[9], 1, DX[i], 1);


  if(DX[9][sing = idmin(qt,DX[9],1)] < 0.0){
    G->singular = sing+1;
  }
  else
    G->singular = 0;

  /* take abs. value of Jacobian */
  dvabs  (qt, DX[9], 1, DX[9], 1);

  /* set pointers */
  G->rx.p  = DX[0];
  G->ry.p  = DX[1];
  G->rz.p  = DX[2];

  G->sx.p  = DX[3];
  G->sy.p  = DX[4];
  G->sz.p  = DX[5];

  G->tx.p  = DX[6];
  G->ty.p  = DX[7];
  G->tz.p  = DX[8];

  G->jac.p = DX[9];

  free_dmatrix(DR,0,0);  free(DX);

  return G;
}




Geom *Hex_gen_curved_geofac(Element *E, int id){
  int      qt,sing;
  const    int qa = E->qa, qb = E->qb;
  double   **DR;
  Geom     *G = (Geom *)calloc(1,sizeof(Geom));

  register int i;
  const    int qc = E->qc;
  double   **DX;

  G->id     = id;
  G->elmt   = E;

  qt  = qa*qb*qc;
  DR  = dmatrix(0,8,0,qt-1); /* work space in 3d */
  DX  = dmatrix(0,9,0,qt-1);

  Hex_DerX(E,DR); /* calc dX/d(r/s/t) */

   /* follows same order as scalar case in add_geofac */
  dvmul  (qt, DR[7], 1, DR[5], 1, DX[0], 1);
  dvvtvm (qt, DR[4], 1, DR[8], 1, DX[0], 1, DX[0], 1);
  dvmul  (qt, DR[7], 1, DR[2], 1, DX[1], 1);
  dvvtvm (qt, DR[1], 1, DR[8], 1, DX[1], 1, DX[1], 1);
  dvmul  (qt, DR[4], 1, DR[2], 1, DX[2], 1);
  dvvtvm (qt, DR[1], 1, DR[5], 1, DX[2], 1, DX[2], 1);
  dvneg  (qt, DX[1], 1, DX[1], 1);

  dvmul  (qt, DR[6], 1, DR[5], 1, DX[3], 1);
  dvvtvm (qt, DR[3], 1, DR[8], 1, DX[3], 1, DX[3], 1);
  dvmul  (qt, DR[6], 1, DR[2], 1, DX[4], 1);
  dvvtvm (qt, DR[0], 1, DR[8], 1, DX[4], 1, DX[4], 1);
  dvmul  (qt, DR[3], 1, DR[2], 1, DX[5], 1);
  dvvtvm (qt, DR[0], 1, DR[5], 1, DX[5], 1, DX[5], 1);
  dvneg  (qt, DX[3], 1, DX[3], 1);
  dvneg  (qt, DX[5], 1, DX[5], 1);

  dvmul  (qt, DR[6], 1, DR[4], 1, DX[6], 1);
  dvvtvm (qt, DR[3], 1, DR[7], 1, DX[6], 1, DX[6], 1);
  dvmul  (qt, DR[6], 1, DR[1], 1, DX[7], 1);
  dvvtvm (qt, DR[0], 1, DR[7], 1, DX[7], 1, DX[7], 1);
  dvmul  (qt, DR[3], 1, DR[1], 1, DX[8], 1);
  dvvtvm (qt, DR[0], 1, DR[4], 1, DX[8], 1, DX[8], 1);
  dvneg  (qt, DX[7], 1, DX[7], 1);

  /* compute Jacobian */
  dvmul  (qt, DR[0], 1, DX[0], 1, DX[9], 1);
  dvvtvp (qt, DR[3], 1, DX[1], 1, DX[9], 1, DX[9], 1);
  dvvtvp (qt, DR[6], 1, DX[2], 1, DX[9], 1, DX[9], 1);

  /* divide factors by Jacobian */
  for(i = 0; i < 9; ++i) dvdiv(qt, DX[i], 1, DX[9], 1, DX[i], 1);

  if(DX[9][sing = idmin(qt,DX[9],1)] < 0.0)
    G->singular = sing + 1;
  else
    G->singular = 0;
  /* take abs. value of Jacobian */
  dvabs  (qt, DX[9], 1, DX[9], 1);

  /* set pointers */
  G->rx.p  = DX[0];
  G->ry.p  = DX[1];
  G->rz.p  = DX[2];

  G->sx.p  = DX[3];
  G->sy.p  = DX[4];
  G->sz.p  = DX[5];

  G->tx.p  = DX[6];
  G->ty.p  = DX[7];
  G->tz.p  = DX[8];

  G->jac.p = DX[9];

  free_dmatrix(DR,0,0);  free(DX);

  return G;
}


/*

Function name: Element_DerX

Function Purpose:

Function Notes:

*/

static void Tri_DerX(Tri *E, double **DR){
  register int i;
  const    int qa = E->qa, qb = E->qb;
  int    qt;
  Coord  X;
  Mode   *v = E->getbasis()->vert;
  double **da,**db,**dt;

  qt  = qa*qb;
  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  E->getD(&da,&dt,&db,&dt,NULL,NULL);

  E->coord(&X);


  /*-------------------------------------------------------------------*
   *                                                                   *
   * In 2D:                                                            *
   *      dX/dr =   2.0/(1-b) du/da |_b                                *
   *      dX/ds = (1+a)/(1-b) du/da |_b  +  d /db |_a                  *
   *                                                                   *
   *-------------------------------------------------------------------*/

  /* calculate dx/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,X.x,qa,0.0,DR[0],qa);
  for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],DR[0]+i*qa,1);

  /* calculate dx/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,DR[0]+i*qa,1,DR[1]+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,X.x,qa,*db,qb,1.0,DR[1],qa);

  /* calculate dy/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,X.y,qa,0.0,DR[2],qa);
  for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],DR[2]+i*qa,1);

  /* calculate dy/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,DR[2]+i*qa,1,DR[3]+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,X.y,qa,*db,qb,1.0,DR[3],qa);

  free(X.x);  free(X.y);

}


static void Quad_DerX(Quad *E, double **DR){
  const    int qa = E->qa, qb = E->qb;
  int    qt;
  Coord  X;
  double **da,**db,**dt;

  qt  = qa*qb;
  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  E->getD(&da,&dt,&db,&dt,NULL,NULL);
  E->coord(&X);

  /*-------------------------------------------------------------------*
   *                                                                   *
   * In 2D:                                                            *
   *      dX/dr =   dX/da |_b                                          *
   *      dX/ds =   dX/db |_a                                          *
   *                                                                   *
   *-------------------------------------------------------------------*/

  /* calculate dx/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,X.x,qa,0.0,DR[0],qa);

  /* calculate dx/ds */
  dgemm('N','N',qa,qb,qb,1.0,X.x,qa,*db,qb,0.0,DR[1],qa);

  /* calculate dy/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,X.y,qa,0.0,DR[2],qa);

  /* calculate dy/ds */
  dgemm('N','N',qa,qb,qb,1.0,X.y,qa,*db,qb,0.0,DR[3],qa);

  free(X.x); free(X.y);

}



static void Tet_DerX(Element *E, double **DR){
  register int i;
  const    int qa = E->qa, qb = E->qb;
  int    qt;
  Coord  X;
  Mode   *v = E->getbasis()->vert;
  double **da,**db,**dt;

  register int j;
  const    int qc = E->qc;
  double   *fac,*s,*s1,**dc;

  qt  = E->qtot;
  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  fac = dvector(0,qt-1);
  E->getD(&da,&dt,&db,&dt,&dc,&dt);

  E->coord(&X);

  /*-------------------------------------------------------------------*
   *                                                                   *
   * In 3D:                                                            *
   *      dX/dr = 4.0/[(1-b)(1-c)] dX/da |_bc                          *
   *      dX/ds = 2.0 (1+a)/[(1-b)(1-c)] dX/da |_bc                    *
   *            + 2.0/(1-c) dX/db |_ac                                 *
   *      dX/dt = 2.0 (1+a)/[(1-b)(1-c)] dX/da |_bc                    *
   *            +     (1+b)/(1-c) dX/db |_ac  + dX/dc |_ab             *
   *                                                                   *
   *-------------------------------------------------------------------*/

  /* calculate dx/dr */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.x,qa,0.0,DR[0],qa);
  /* multiply by 4/[(1-b)(1-c)] */
  dvrecp(qb,v->b,1,fac,1);
  for(i = 0,s=DR[0]; i < qc; ++i){
    for(j = 0; j < qb; ++j,s+=qa)
      dsmul(qa,fac[j],s,1,s,1);
    dsmul(qa*qb,1.0/v->c[i],DR[0]+i*qa*qb,1,DR[0]+i*qa*qb,1);
  }
  /* calc dx/db/(1-c)/2 */
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],X.x+i*qa*qb,qa,*db,qb,
      0.0,DR[1]+i*qa*qb,qa);
  /* calc dx/dc */
  dgemm('N','N',qa*qb,qc,qc,1.0,X.x,qa*qb,*dc,qc,0.0,DR[2],qa*qb);
  /* add dx/db*(1+b)/2 to dx/dc */
  for(i = 0, s=DR[1], s1 = DR[2]; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
      daxpy(qa,v[2].b[j],s,1,s1,1);
  /* calc dx/dr*(1+a)/2 */
  for(i = 0; i < qb*qc; ++i)  dvmul(qa,v[1].a,1,DR[0]+i*qa,1,fac+i*qa,1);
  /* add fac to dx/db and dx/dc  to get dx/ds and dx/ds*/
  dvadd(qt,fac,1,DR[1],1,DR[1],1);
  dvadd(qt,fac,1,DR[2],1,DR[2],1);

  /* calculate dy/dr */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.y,qa,0.0,DR[3],qa);
  /* multiply by 4/[(1-b)(1-c)] */
  dvrecp(qb,v->b,1,fac,1);
  for(i = 0,s=DR[3]; i < qc; ++i){
    for(j = 0; j < qb; ++j,s+=qa)
      dsmul(qa,fac[j],s,1,s,1);
    dsmul(qa*qb,1.0/v->c[i],DR[3]+i*qa*qb,1,DR[3]+i*qa*qb,1);
  }
  /* calc dy/db/(1-c)/2 */
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],X.y+i*qa*qb,qa,*db,qb,
      0.0,DR[4]+i*qa*qb,qa);
  /* calc dy/dc */
  dgemm('N','N',qa*qb,qc,qc,1.0,X.y,qa*qb,*dc,qc,0.0,DR[5],qa*qb);
  /* add dy/db*(1+b)/2 to dy/dc */
  for(i = 0, s=DR[4], s1 = DR[5]; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
      daxpy(qa,v[2].b[j],s,1,s1,1);
  /* calc dy/dr*(1+a)/2 */
  for(i = 0; i < qb*qc; ++i)  dvmul(qa,v[1].a,1,DR[3]+i*qa,1,fac+i*qa,1);
  /* add fac to dy/db and dy/dc  to get dy/ds and dy/ds*/
  dvadd(qt,fac,1,DR[4],1,DR[4],1);
  dvadd(qt,fac,1,DR[5],1,DR[5],1);

  /* calculate dz/dr */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.z,qa,0.0,DR[6],qa);
  /* multiply by 4/[(1-b)(1-c)] */
  dvrecp(qb,v->b,1,fac,1);
  for(i = 0,s=DR[6]; i < qc; ++i){
    for(j = 0; j < qb; ++j,s+=qa)
      dsmul(qa,fac[j],s,1,s,1);
    dsmul(qa*qb,1.0/v->c[i],DR[6]+i*qa*qb,1,DR[6]+i*qa*qb,1);
  }
  /* calc dz/db/(1-c)/2 */
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],X.z+i*qa*qb,qa,*db,qb,
      0.0,DR[7]+i*qa*qb,qa);
  /* calc dz/dc */
  dgemm('N','N',qa*qb,qc,qc,1.0,X.z,qa*qb,*dc,qc,0.0,DR[8],qa*qb);
  /* add dz/db*(1+b)/2 to dz/dc */
  for(i = 0, s=DR[7], s1 = DR[8]; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
      daxpy(qa,v[2].b[j],s,1,s1,1);
  /* calc dz/dr*(1+a)/2 */
  for(i = 0; i < qb*qc; ++i)  dvmul(qa,v[1].a,1,DR[6]+i*qa,1,fac+i*qa,1);
  /* add fac to dz/db and dz/dc  to get dz/ds and dz/ds*/
  dvadd(qt,fac,1,DR[7],1,DR[7],1);
  dvadd(qt,fac,1,DR[8],1,DR[8],1);

  free(X.x); free(X.y); free(X.z); free(fac);
}


static void Pyr_DerX(Element *E, double **DR){
  register int i,j,k;
  int    qt;
  Coord  X;
  Mode   *v = E->getbasis()->vert;
  double **da,**db,**dt,**dc;
  int qa = E->qa, qb = E->qb, qc = E->qc;

  qt  = E->qtot;
  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  E->getD(&da,&dt,&db,&dt,&dc,&dt);
  E->coord(&X);


  /*--------------------------------------------------------------------*
   *                                                                    *
   * In 3D:                                                             *
   *                                                                    *
   * dX/dr =   2/(1-c). dX/da |_bc                                      *
   * dX/ds =   2/(1-c). dX/db |_ac                                      *
   * dX/dt =   (1+a)/(1-c). dX/da |_bc + (1+b)/(1-c). dX/db +dX/dc |_ab *
   *                                                                    *
   *--------------------------------------------------------------------*/

  /* calculate dx/da */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.x,qa,0.0,DR[0],qa);
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.y,qa,0.0,DR[3],qa);
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.z,qa,0.0,DR[6],qa);

  /* calculate dx/db */
  for(i = 0; i < qc; ++i){
    dgemm('N','N',qa,qb,qb,1.0,X.x+i*qa*qb,qa,*db,qb,0.0,DR[1]+i*qa*qb,qa);
    dgemm('N','N',qa,qb,qb,1.0,X.y+i*qa*qb,qa,*db,qb,0.0,DR[4]+i*qa*qb,qa);
    dgemm('N','N',qa,qb,qb,1.0,X.z+i*qa*qb,qa,*db,qb,0.0,DR[7]+i*qa*qb,qa);
  }

  /* calculate dx/dc */
  dgemm('N','N',qa*qb,qc,qc,1.0,X.x,qa*qb,*dc,qc,0.0,DR[2],qa*qb);
  dgemm('N','N',qa*qb,qc,qc,1.0,X.y,qa*qb,*dc,qc,0.0,DR[5],qa*qb);
  dgemm('N','N',qa*qb,qc,qc,1.0,X.z,qa*qb,*dc,qc,0.0,DR[8],qa*qb);

  // dX/dr
  for(i=0;i<qc;++i){
    dscal(qa*qb, 1./v[0].c[i], DR[0]+i*qa*qb, 1);
    dscal(qa*qb, 1./v[0].c[i], DR[3]+i*qa*qb, 1);
    dscal(qa*qb, 1./v[0].c[i], DR[6]+i*qa*qb, 1);
  }

  // dX/ds
  for(i=0;i<qc;++i){
    dscal(qa*qb, 1./v[0].c[i], DR[1]+i*qa*qb, 1);
    dscal(qa*qb, 1./v[0].c[i], DR[4]+i*qa*qb, 1);
    dscal(qa*qb, 1./v[0].c[i], DR[7]+i*qa*qb, 1);
  }

  // dX/dt
  for(i=0;i<qc*qb;++i){
    dvvtvp(qa, v[1].a, 1, DR[0]+i*qa, 1, DR[2]+i*qa, 1, DR[2]+i*qa, 1);
    dvvtvp(qa, v[1].a, 1, DR[3]+i*qa, 1, DR[5]+i*qa, 1, DR[5]+i*qa, 1);
    dvvtvp(qa, v[1].a, 1, DR[6]+i*qa, 1, DR[8]+i*qa, 1, DR[8]+i*qa, 1);
  }

  for(i=0;i<qc;++i)
    for(j=0;j<qb;++j){
      k = i*qb*qa+j*qa;
      daxpy(qa, v[1].a[j], DR[1]+k, 1, DR[2]+k, 1);
      daxpy(qa, v[1].a[j], DR[4]+k, 1, DR[5]+k, 1);
      daxpy(qa, v[1].a[j], DR[7]+k, 1, DR[8]+k, 1);
    }

  free(X.x); free(X.y); free(X.z);

}




static void Prism_DerX(Element *E, double **DR){
  register int i;
  int    qt;
  Coord  X;
  Mode   *v = E->getbasis()->vert;
  double **da,**db,**dt,**dc;
  int qa = E->qa, qb = E->qb, qc = E->qc;

  qt  = E->qtot;
  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  E->getD(&da,&dt,&db,&dt,&dc,&dt);
  E->coord(&X);


  /*-------------------------------------------------------------------*
   *                                                                   *
   * In 3D:                                                            *
   *      dX/dr =   2/(1-c). dX/da |_bc
   *      dX/ds =   dX/db |_ac                                         *
   *      dX/dt =   (1+a)/(1-c). dX/da |_bc  +dX/dc |_ab               *
   *                                                                   *
   *-------------------------------------------------------------------*/

  /* calculate dx/da */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.x,qa,0.0,DR[0],qa);
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.y,qa,0.0,DR[3],qa);
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.z,qa,0.0,DR[6],qa);

  /* calculate dx/db */
  for(i = 0; i < qc; ++i){
    dgemm('N','N',qa,qb,qb,1.0,X.x+i*qa*qb,qa,*db,qb,0.0,DR[1]+i*qa*qb,qa);
    dgemm('N','N',qa,qb,qb,1.0,X.y+i*qa*qb,qa,*db,qb,0.0,DR[4]+i*qa*qb,qa);
    dgemm('N','N',qa,qb,qb,1.0,X.z+i*qa*qb,qa,*db,qb,0.0,DR[7]+i*qa*qb,qa);
  }

  /* calculate dx/dc */
  dgemm('N','N',qa*qb,qc,qc,1.0,X.x,qa*qb,*dc,qc,0.0,DR[2],qa*qb);
  dgemm('N','N',qa*qb,qc,qc,1.0,X.y,qa*qb,*dc,qc,0.0,DR[5],qa*qb);
  dgemm('N','N',qa*qb,qc,qc,1.0,X.z,qa*qb,*dc,qc,0.0,DR[8],qa*qb);

  for(i=0;i<qc;++i){
    dscal(qa*qb, 1./v[0].c[i], DR[0]+i*qa*qb, 1);
    dscal(qa*qb, 1./v[0].c[i], DR[3]+i*qa*qb, 1);
    dscal(qa*qb, 1./v[0].c[i], DR[6]+i*qa*qb, 1);
  }

  for(i=0;i<qc*qb;++i){
    dvvtvp(qa, v[1].a, 1, DR[0]+i*qa, 1, DR[2]+i*qa, 1, DR[2]+i*qa, 1);
    dvvtvp(qa, v[1].a, 1, DR[3]+i*qa, 1, DR[5]+i*qa, 1, DR[5]+i*qa, 1);
    dvvtvp(qa, v[1].a, 1, DR[6]+i*qa, 1, DR[8]+i*qa, 1, DR[8]+i*qa, 1);
  }

  free(X.x); free(X.y); free(X.z);

}


static void Hex_DerX(Element *E, double **DR){
  register int i;
  int    qt;
  Coord  X;
  Mode   *v = E->getbasis()->vert;
  double **da,**db,**dt;
  int qa = E->qa, qb = E->qb, qc = E->qc;
  double   **dc;

  qt  = E->qtot;
  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  E->getD(&da,&dt,&db,&dt,&dc,&dt);
  E->coord(&X);


  /*-------------------------------------------------------------------*
   *                                                                   *
   * In 3D:                                                            *
   *      dX/dr =   dX/da |_bc                                         *
   *      dX/ds =   dX/db |_ac                                         *
   *      dX/dt =   dX/dc |_ab                                         *
   *                                                                   *
   *-------------------------------------------------------------------*/

  /* calculate dx/dr */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.x,qa,0.0,DR[0],qa);
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.y,qa,0.0,DR[3],qa);
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,X.z,qa,0.0,DR[6],qa);

  /* calculate dx/ds */
  for(i = 0; i < qc; ++i){
    dgemm('N','N',qa,qb,qb,1.0,X.x+i*qa*qb,qa,*db,qb,0.0,DR[1]+i*qa*qb,qa);
    dgemm('N','N',qa,qb,qb,1.0,X.y+i*qa*qb,qa,*db,qb,0.0,DR[4]+i*qa*qb,qa);
    dgemm('N','N',qa,qb,qb,1.0,X.z+i*qa*qb,qa,*db,qb,0.0,DR[7]+i*qa*qb,qa);
  }


  /* calculate dx/dt */
  dgemm('N','N',qa*qb,qc,qc,1.0,X.x,qa*qb,*dc,qc,0.0,DR[2],qa*qb);
  dgemm('N','N',qa*qb,qc,qc,1.0,X.y,qa*qb,*dc,qc,0.0,DR[5],qa*qb);
  dgemm('N','N',qa*qb,qc,qc,1.0,X.z,qa*qb,*dc,qc,0.0,DR[8],qa*qb);

  free(X.x); free(X.y); free(X.z);
}








static int Tri_check_L(Tri *E, Geom *G){
  register int i;
  int      trip = 1;
  Tri *U = (Tri*) (G->elmt);

  for(i = 0; i < E->Nedges; ++i)
    if(E->edge[i].l != U->edge[i].l) trip = 0;

  if(E->face->l != U->face->l) trip = 0;

  if(U->qa != E->qa) trip = 0;
  if(U->qb != E->qb) trip = 0;

  if(trip && U->lmax != E->lmax)
    fprintf(stdout, "Everything but lmax agrees\n");

  return trip;
}





static int Quad_check_L(Quad *E, Geom *G){
  register int i;
  int      trip = 1;
  Quad *U = (Quad*) G->elmt;

  for(i = 0; i < E->Nedges; ++i)
    if(E->edge[i].l != U->edge[i].l) trip = 0;
  if(E->face->l != U->face->l) trip = 0;

  if(U->qa != E->qa) trip = 0;
  if(U->qb != E->qb) trip = 0;

  if(trip && U->lmax != E->lmax)
    fprintf(stdout, "Everything but lmax agrees\n");

  return trip;
}




static int Tet_check_L(Element *E, Geom *G){
  register int i;
  int      trip = 1;
  Element *U = G->elmt;

  for(i = 0; i < NTet_edges; ++i)
    if(E->edge[i].l != U->edge[i].l) trip = 0;

  for(i = 0; i < NTet_faces; ++i)
    if(E->face[i].l != U->face[i].l) trip = 0;

  if(E->interior_l != U->interior_l) trip = 0;

  return trip;
}



static int Pyr_check_L(Element *E, Geom *G){
  register int i;
  int      trip = 1;
  Element *U = G->elmt;

  for(i = 0; i < NPyr_edges; ++i)
    if(E->edge[i].l != U->edge[i].l) trip = 0;

  for(i = 0; i < NPyr_faces; ++i)
    if(E->face[i].l != U->face[i].l) trip = 0;

  if(E->interior_l != U->interior_l) trip = 0;

  return trip;
}



static int Prism_check_L(Element *E, Geom *G){
  register int i;
  int      trip = 1;
  Element *U = G->elmt;

  for(i = 0; i < NPrism_edges; ++i)
    if(E->edge[i].l != U->edge[i].l) trip = 0;

  for(i = 0; i < NPrism_faces; ++i)
    if(E->face[i].l != U->face[i].l) trip = 0;

  if(E->interior_l != U->interior_l) trip = 0;

  return trip;
}


static int Hex_check_L(Element *E, Geom *G){
  register int i;
  int      trip = 1;
  Element *U = G->elmt;

  for(i = 0; i < NHex_edges; ++i)
    if(E->edge[i].l != U->edge[i].l) trip = 0;

  for(i = 0; i < NHex_faces; ++i)
    if(E->face[i].l != U->face[i].l) trip = 0;

  if(E->interior_l != U->interior_l) trip = 0;

  return trip;
}
