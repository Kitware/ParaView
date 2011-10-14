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

using namespace polylib;


/*

Function name: Element::Grad

Function Purpose:
Wrapper around the routines to calculate spatial derivatives on the element.

Argument 1: Element *d_dx
Purpose:
Element to be used to store the 'x' derivative output.

Argument 2: Element *d_dy
Purpose:
Element to be used to store the 'y' derivative output.

Argument 3: Element *d_dz
Purpose:
(3d) Element to be used to store the 'x' derivative output.

Argument 4: char Trip
Purpose:
Flag to denote which derivatives are to be calculated:

Options:

Function Notes:

*/

void Tri::Grad(Element *d_dx, Element *d_dy, Element *d_dz, char Trip){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h[0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h[0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h[0]: (double*)NULL;

  Grad_d(ux,uy,uz, Trip);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
}

void Quad::Grad(Element *d_dx, Element *d_dy, Element *d_dz, char Trip){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h[0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h[0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h[0]: (double*)NULL;

  Grad_d(ux,uy,uz, Trip);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
}

void Tet::Grad(Element *d_dx, Element *d_dy, Element *d_dz, char trip){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h_3d[0][0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h_3d[0][0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h_3d[0][0]: (double*)NULL;

  Grad_d(ux,uy,uz, trip);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
  if(d_dz) d_dz->state = 'p';
}




void Pyr::Grad(Element *d_dx, Element *d_dy, Element *d_dz, char trip){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h_3d[0][0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h_3d[0][0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h_3d[0][0]: (double*)NULL;

  Grad_d(ux,uy,uz, trip);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
  if(d_dz) d_dz->state = 'p';
}




void Prism::Grad(Element *d_dx, Element *d_dy, Element *d_dz, char trip){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h_3d[0][0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h_3d[0][0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h_3d[0][0]: (double*)NULL;

  Grad_d(ux,uy,uz, trip);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
  if(d_dz) d_dz->state = 'p';
}




void Hex::Grad(Element *d_dx, Element *d_dy, Element *d_dz, char trip){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h_3d[0][0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h_3d[0][0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h_3d[0][0]: (double*)NULL;

  Grad_d(ux,uy,uz, trip);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
  if(d_dz) d_dz->state = 'p';
}




void Element::Grad (Element *, Element *, Element *, char ){ERR;}//grad


void Tri::GradT(Element *d_dx, Element *d_dy, Element *d_dz, char Trip){
    GradT(d_dx,d_dy,d_dz,0);
}

void Tri::GradT(Element *d_dx, Element *d_dy, Element *d_dz, char Trip, bool invW){
  double *ux,*uy,*uz;
  ux = (d_dx) ? d_dx->h[0]: (double*)NULL;
  uy = (d_dy) ? d_dy->h[0]: (double*)NULL;
  uz = (d_dz) ? d_dz->h[0]: (double*)NULL;

  GradT_h(h[0],ux,uy,uz, Trip,invW);
  if(d_dx) d_dx->state = 'p';
  if(d_dy) d_dy->state = 'p';
}

void Quad::GradT(Element *d_dx, Element *d_dy, Element *d_dz, char Trip){
    GradT(d_dx,d_dy,d_dz,Trip,0);
}

void Quad::GradT(Element *d_dx, Element *d_dy, Element *d_dz, char Trip, bool invW){
    double *ux,*uy,*uz;
    ux = (d_dx) ? d_dx->h[0]: (double*)NULL;
    uy = (d_dy) ? d_dy->h[0]: (double*)NULL;
    uz = (d_dz) ? d_dz->h[0]: (double*)NULL;

    GradT_h(h[0],ux,uy,uz, Trip,invW);
    if(d_dx) d_dx->state = 'p';
    if(d_dy) d_dy->state = 'p';
}



/*

Function name: Element::Grad_d

Function Purpose:
Wrapper around Grad_h. Output will be the derivatives of the field stored in this element.

Argument 1: double *ux
Purpose:
Array for storing the 'x' derivative of the field held in the physical storage ('h' (2d) or 'h_3d' (3d)) in this element.

Argument 2: double *uy
Purpose:
Array for storing the 'x' derivative of the field held in the physical storage ('h' (2d) or 'h_3d' (3d)) in this element.

Argument 3: double *uz
Purpose:
Array for storing the 'x' derivative of the field held in the physical storage ('h' (2d) or 'h_3d' (3d)) in this element.

Argument 4: char trip
Purpose:
Flag which determines which derivatives are to be calculated.
 Options:

Function Notes:

*/

void Tri::Grad_d(double *ux, double *uy, double *uz, char trip){
  Grad_h(h[0],ux,uy,uz,trip);
}

void Quad::Grad_d(double *ux, double *uy, double *uz, char trip){
  Grad_h(h[0],ux,uy,uz,trip);
}

void Tet::Grad_d(double *ux, double *uy, double *uz, char trip){
  Grad_h(h_3d[0][0],ux,uy,uz,trip);
}

void Pyr::Grad_d(double *ux, double *uy, double *uz, char trip){
  Grad_h(h_3d[0][0],ux,uy,uz,trip);
}

void Prism::Grad_d(double *ux, double *uy, double *uz, char trip){
  Grad_h(h_3d[0][0],ux,uy,uz,trip);
}

void Hex::Grad_d(double *ux, double *uy, double *uz, char trip){
  Grad_h(h_3d[0][0],ux,uy,uz,trip);
}

void Element::Grad_d (double *, double *, double *, char ){ERR;} //grad

/*

Function name: Element::Grad_h

Function Purpose:
Output will be the derivatives of the field stored in the vector H.

Argument 1: double *H
Purpose:
Field (assumed to be given in normal quadrature storage format) to be differentiated.

Argument 2: double *ux
Purpose:
Array for storing the 'x' derivative of the field held in H.

Argument 3: double *uy
Purpose:
Array for storing the 'y' derivative of the field held in H.

Argument 4: double *uz
Purpose:
Array for storing the 'z' derivative of the field held in H.

Argument 5: char trip
Purpose:
Flag which determines which derivatives are to be calculated.
 Options:

Function Notes:

*/

void Tri::Grad_h(double *H, double *ux, double *uy, double *uz, char trip){
  register  int i;
  const     int qt = qa*qb;
  double    **da,**dat,**db,**dbt;
  Geom      *G = geom;
  Mode      *v = getbasis()->vert;
  double    *u = H; // fix ??
//  double    *wk = dvector(0,QGmax*QGmax-1);
  double    *ur = Tri_wk.get();

  uz=uz; // compiler fix

  getD(&da,&dat,&db,&dbt,NULL,NULL);

   /*-------------------------------------------------------------------*
   * note du/dx = du/dr * dr/dx + du/ds * ds/dx + du/dt * dt/dx        *
   *      du/dy = du/dr * dr/dy + du/ds * ds/dy + du/dt * dt/dy        *
   *      du/dz = du/dr * dr/dz + du/ds * ds/dz + du/dt * dt/dz        *
   *                                                                   *
   * In 2D:                                                            *
   *      du/dr =   2.0/(1-b) du/da |_b                                *
   *      du/ds = (1+a)/(1-b) du/da |_b  +  d /db |_a                  *
   *                                                                   *
   * Note: there is a slight economy in calculating all  derivatives   *
   * rather than each one seperately                                   *
   *-------------------------------------------------------------------*/

  /* calculate du/dr */
  mxm(u,qb,*dat,qa,ur,qa);

  for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],ur+i*qa,1);

  if(!curvX){
    double rx,ry,sx,sy;

    rx  =  G->rx.d;
    ry  =  G->ry.d;
    sx  =  G->sx.d;
    sy  =  G->sy.d;

    switch(trip){
    case 'a':
      if(sx||sy){
  mxm(*db,qb,u,qb,ux,qa);
  for(i = 0; i < qb; ++i)
    dvvtvp(qa,v[1].a,1,ur+i*qa,1,ux+i*qa,1,ux+i*qa,1);
      }

      if(sy){
  dsmul(qt,sy,ux,1,uy,1);
  if(ry) daxpy(qt,ry,ur,1,uy,1);
      }
      else
  dsmul(qt,ry,ur,1,uy,1);

      if(sx){
  dscal(qt,sx,ux,1);
  if(rx) daxpy(qt,rx,ur,1,ux,1);
      }
      else
  dsmul(qt,rx,ur,1,ux,1);
      break;
    case 'x':
      if(sx){
  mxm(*db,qb,u,qb,ux,qa);
  for(i = 0; i < qb; ++i)
    dvvtvp(qa,v[1].a,1,ur+i*qa,1,ux+i*qa,1,ux+i*qa,1);
  dscal(qt,sx,ux,1);
  if(rx) daxpy(qt,rx,ur,1,ux,1);
      }
      else
  dsmul(qt,rx,ur,1,ux,1);
      break;
    case 'y':
      if(sy){
  mxm(*db,qb,u,qb,uy,qa);
  for(i = 0; i < qb; ++i)
    dvvtvp(qa,v[1].a,1,ur+i*qa,1,uy+i*qa,1,uy+i*qa,1);
  dscal(qt,sy,uy,1);

  if(ry) daxpy(qt,ry,ur,1,uy,1);
      }
      else
  dsmul(qt,ry,ur,1,uy,1);
      break;
    }
  }
  else{
    double *rx,*ry,*sx,*sy;

    rx  =  G->rx.p;
    ry  =  G->ry.p;
    sx  =  G->sx.p;
    sy  =  G->sy.p;

    switch(trip){
    case 'a':
      mxm(*db,qb,u,qb,ux,qa);
      for(i = 0; i < qb; ++i)
  dvvtvp(qa,v[1].a,1,ur+i*qa,1,ux+i*qa,1,ux+i*qa,1);
      dvmul  (qt,sy,1,ux ,1,uy,1);
      dvvtvp (qt,ry,1,ur,1,uy,1,uy,1);
      dvmul  (qt,sx,1,ux ,1,ux,1);
      dvvtvp (qt,rx,1,ur,1,ux,1,ux,1);

      break;
    case 'x':
      mxm(*db,qb,u,qb,ux,qa);
      for(i = 0; i < qb; ++i)
  dvvtvp(qa,v[1].a,1,ur+i*qa,1,ux+i*qa,1,ux+i*qa,1);
      dvmul  (qt,sx,1,ux, 1,ux,1);
      dvvtvp (qt,rx,1,ur,1,ux,1,ux,1);
      break;
    case 'y':
      mxm(*db,qb,u,qb,uy,qa);
      for(i = 0; i < qb; ++i)
  dvvtvp(qa,v[1].a,1,ur+i*qa,1,uy+i*qa,1,uy+i*qa,1);
      dvmul  (qt,sy,1,uy ,1,uy,1);
      dvvtvp (qt,ry,1,ur,1,uy,1,uy,1);
      break;
    }
  }
//  free(wk);
  return;
}


void Quad::Grad_h(double *H, double *ux, double *uy, double *uz, char trip){
  const     int qt = qa*qb;
  double    **da,**dat,**db,**dbt;
  Geom      *G = geom;
  double    *u = H; // fix ??
  double    *ur = Quad_Grad_wk;
  double    *us = Quad_Grad_wk +QGmax*QGmax;

  uz=uz; // compiler fix

  getD(&da,&dat,&db,&dbt,NULL,NULL);

  /*-------------------------------------------------------------------*
   * note du/dx = du/dr * dr/dx + du/ds * ds/dx                        *
   *      du/dy = du/dr * dr/dy + du/ds * ds/dy                        *
   *                                                                   *
   * In 2D:                                                            *
   *      du/dr =   du/da |_b                                          *
   *      du/ds =   du/db |_a                                          *
   *                                                                   *
   * Note: there is a slight economy in calculating all  derivatives   *
   * rather than each one seperately                                   *
   *-------------------------------------------------------------------*/
#if 0

  /* calculate du/da */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,u,qa,0.0,ur,qa);

  // calculate du/db
  dgemm('N','N',qa,qb,qb,1.0,u,qa,*db,qb,0.0,us,qa);
#else
  /* calculate du/dr */
  mxm(u,qb,*dat,qa,ur,qa);
  /* calculate du/ds */
  mxm(*db,qb,u,qb,us,qa);
#endif
  double *rx,*ry,*sx,*sy;

  rx  =  G->rx.p;
  ry  =  G->ry.p;
  sx  =  G->sx.p;
  sy  =  G->sy.p;

  switch(trip){
  case 'a':
    dvmul  (qt,sy,1,us ,1, uy,1);
    dvvtvp (qt,ry,1,ur ,1, uy,1,uy,1);
    dvmul  (qt,sx,1,us ,1, ux,1);
    dvvtvp (qt,rx,1,ur ,1, ux,1,ux,1);
    break;
  case 'x':
    dvmul  (qt,sx,1,us,1,ux,1);
    dvvtvp (qt,rx,1,ur,1,ux,1,ux,1);
    break;
  case 'y':
    dvmul  (qt,sy,1,us,1,uy,1);
    dvvtvp (qt,ry,1,ur,1,uy,1,uy,1);
    break;
  }
  return;
}


void Tet::Grad_h(double *u, double *ux, double *uy, double *uz, char trip){
  register  int i,j;
  const     int qt = qtot;
  double    **da,**dat,**db,**dc,**dt,*ub,*s,*s1,*fac;
  Geom      *G = geom;
  Mode      *v = getbasis()->vert;
  double    *ua = Tet_Grad_wk;


  getD(&da,&dat,&db,&dt,&dc,&dt);

  /* work vectors */
  ub = ua + qt; fac = ub + qt;

  /*-------------------------------------------------------------------*
   * note du/dx = du/dr * dr/dx + du/ds * ds/dx + du/dt * dt/dx        *
   *      du/dy = du/dr * dr/dy + du/ds * ds/dy + du/dt * dt/dy        *
   *      du/dz = du/dr * dr/dz + du/ds * ds/dz + du/dt * dt/dz        *
   *                                                                   *
   * In 3D:                                                            *
   *      du/dr = 4.0/[(1-b)(1-c)] du/da |_bc                          *
   *      du/ds = 2.0 (1+a)/[(1-b)(1-c)] du/da |_bc                    *
   *            + 2.0/(1-c) du/db |_ac                                 *
   *      du/dt = 2.0 (1+a)/[(1-b)(1-c)] du/da |_bc                    *
   *            +     (1+b)/(1-c) du/db |_ac  + du/dc |_ab             *
   *                                                                   *
   * note there is a slight economy in calculating all  derivatives    *
   * rather than each one seperately                                   *
   *-------------------------------------------------------------------*/

  /* calculate du/da */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,u,qa,0.0,ua,qa);

  /* multiply by 4/[(1-b)(1-c)]*/

  dvrecp(qb,v->b,1,fac,1);

  for(i = 0,s=ua; i < qc; ++i){
    for(j = 0; j < qb; ++j,s+=qa)
      dsmul(qa,fac[j],s,1,s,1);
    dsmul(qa*qb,1.0/v->c[i],ua+i*qa*qb,1,ua+i*qa*qb,1);
  }

  if(!curvX){
    switch(trip){
    case 'a':
      /* calc du/db/(1-c)/2 */
      for(i = 0; i < qc; ++i)
  dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
        0.0,ub+i*qa*qb,qa);

      dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,uz,qa*qb);

      dsmul(qa,G->sx.d+G->tx.d,v[1].a,1,fac,1);
      dsadd(qa,G->rx.d,fac,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,fac,1,ua+i*qa,1,ux+i*qa,1);

      if(G->tx.d){
  dsmul(qb,G->tx.d,v[2].b,1,fac,1);
  dsadd(qb,G->sx.d,fac,1,fac,1);
  for(i = 0, s=ub, s1 = ux; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
      daxpy(qa,fac[j],s,1,s1,1);

  daxpy(qt,G->tx.d,uz,1,ux,1);
      }
      else if (G->sx.d)
  daxpy(qt,G->sx.d,ub,1,ux,1);

      dsmul(qa,G->sy.d+G->ty.d,v[1].a,1,fac,1);
      dsadd(qa,G->ry.d,fac,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,fac,1,ua+i*qa,1,uy+i*qa,1);

      if(G->ty.d){
  dsmul(qb,G->ty.d,v[2].b,1,fac,1);
  dsadd(qb,G->sy.d,fac,1,fac,1);
  for(i = 0, s=ub, s1 = uy; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa, s1 +=qa)
      daxpy(qa,fac[j],s,1,s1,1);

  daxpy(qt,G->ty.d,uz,1,uy,1);
      }
      else if (G->sy.d)
  daxpy(qt,G->sy.d,ub,1,uy,1);

      dsmul(qa,G->sz.d+G->tz.d,v[1].a,1,fac,1);
      dsadd(qa,G->rz.d,fac,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,fac,1,ua+i*qa,1,ua+i*qa,1);

      if(G->tz.d){
  dsvtvp(qt,G->tz.d,uz,1,ua,1,uz,1);

  dsmul(qb,G->tz.d,v[2].b,1,fac,1);
  dsadd(qb,G->sz.d,fac,1,fac,1);
  for(i = 0, s=ub, s1 = uz; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa, s1+=qa)
      daxpy(qa,fac[j],s,1,s1,1);
      }
      else if (G->sz.d)
  dsvtvp(qt,G->sz.d,ub,1,ua,1,uz,1);
      else
  dcopy(qt,ua,1,uz,1);

      break;
    case 'x':
      dsmul(qa,G->sx.d+G->tx.d,v[1].a,1,fac,1);
      dsadd(qa,G->rx.d,fac,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,fac,1,ua+i*qa,1,ux+i*qa,1);

      if(G->tx.d){
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);
  dgemm('N','N',qa*qb,qc,qc,G->tx.d,u,qa*qb,*dc,qc,1.0,ux,qa*qb);

  dsmul(qb,G->tx.d,v[2].b,1,fac,1);
  dsadd(qb,G->sx.d,fac,1,fac,1);
  for(i = 0, s=ub, s1 = ux; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
      daxpy(qa,fac[j],s,1,s1,1);
      }
      else if (G->sx.d){
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
        0.0,ub+i*qa*qb,qa);
  daxpy(qt,G->sx.d,ub,1,ux,1);
      }
      break;
    case 'y':
      dsmul(qa,G->sy.d+G->ty.d,v[1].a,1,fac,1);
      dsadd(qa,G->ry.d,fac,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,fac,1,ua+i*qa,1,uy+i*qa,1);

      if(G->ty.d){
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);
  dgemm('N','N',qa*qb,qc,qc,G->ty.d,u,qa*qb,*dc,qc,1.0,uy,qa*qb);

  dsmul(qb,G->ty.d,v[2].b,1,fac,1);
  dsadd(qb,G->sy.d,fac,1,fac,1);
  for(i = 0, s=ub, s1 = uy; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
    daxpy(qa,fac[j],s,1,s1,1);
      }
      else if (G->sy.d){
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);
  daxpy(qt,G->sy.d,ub,1,uy,1);
      }
      break;
    case 'z':
      dsmul(qa,G->sz.d+G->tz.d,v[1].a,1,fac,1);
      dsadd(qa,G->rz.d,fac,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,fac,1,ua+i*qa,1,uz+i*qa,1);

      if(G->tz.d){
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);
  dgemm('N','N',qa*qb,qc,qc,G->tz.d,u,qa*qb,*dc,qc,1.0,uz,qa*qb);

  dsmul(qb,G->tz.d,v[2].b,1,fac,1);
  dsadd(qb,G->sz.d,fac,1,fac,1);
  for(i = 0, s=ub, s1 = uz; i < qc; ++i)
    for(j = 0; j < qb; ++j,s += qa,s1+=qa)
    daxpy(qa,fac[j],s,1,s1,1);
      }
      else if (G->sz.d){
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);
  daxpy(qt,G->sz.d,ub,1,uz,1);
      }

      break;
    }
  }
  else{ /* curved face version */
    for(i = 0; i < qc; ++i)
      dgemm('N','N',qa,qb,qb,1.0/v->c[i],u+i*qa*qb,qa,*db,qb,
      0.0,ub+i*qa*qb,qa);

    switch(trip){
    case 'a':
      dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,uz,qa*qb);

      dvadd (qt,G->sx.p,1,G->tx.p,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,v[1].a,1,fac+i*qa,1,fac+i*qa,1);
      dvadd (qt,G->rx.p,1,fac,1,fac,1);
      dvmul (qt,fac,1,ua,1,ux,1);
      for(i = 0, s=G->tx.p, s1 = fac; i < qc; ++i)
  for(j = 0; j < qb; ++j,s += qa,s1+=qa)
    dsmul(qa,v[2].b[j],s,1,s1,1);
      dvadd (qt,G->sx.p,1,fac,1,fac,1);
      dvvtvp(qt,fac,    1,ub,1,ux,1,ux,1);
      dvvtvp(qt,G->tx.p,1,uz,1,ux,1,ux,1);

      dvadd(qt,G->sy.p,1,G->ty.p,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,v[1].a,1,fac+i*qa,1,fac+i*qa,1);
      dvadd(qt,G->ry.p,1,fac,1,fac,1);
      dvmul(qt,fac,1,ua,1,uy,1);
      for(i = 0, s=G->ty.p, s1 = fac; i < qc; ++i)
  for(j = 0; j < qb; ++j,s += qa, s1 +=qa)
    dsmul(qa,v[2].b[j],s,1,s1,1);
      dvadd (qt,G->sy.p,1,fac,1,fac,1);
      dvvtvp(qt,fac    ,1,ub,1,uy,1,uy,1);
      dvvtvp(qt,G->ty.p,1,uz,1,uy,1,uy,1);

      dvadd(qt,G->sz.p,1,G->tz.p,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,v[1].a,1,fac+i*qa,1,fac+i*qa,1);
      dvadd (qt,G->rz.p,1,fac,1,fac,1);
      dvmul (qt,fac,1,ua,1,ua,1);
      dvvtvp(qt,G->tz.p,1,uz,1,ua,1,uz,1);
      for(i = 0, s=G->tz.p, s1 = fac; i < qc; ++i)
  for(j = 0; j < qb; ++j,s += qa, s1+=qa)
    dsmul(qa,v[2].b[j],s,1,s1,1);
      dvadd (qt,G->sz.p,1,fac,1,fac,1);
      dvvtvp(qt,fac,1,ub,1,uz,1,uz,1);
      break;
    case 'x':
      dvadd (qt,G->sx.p,1,G->tx.p,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,v[1].a,1,fac+i*qa,1,fac+i*qa,1);
      dvadd (qt,G->rx.p,1,fac    ,1,fac,1);
      dvmul (qt,fac,1,ua,1,ux,1);
      for(i = 0, s=G->tx.p, s1 = fac; i < qc; ++i)
  for(j = 0; j < qb; ++j,s += qa,s1+=qa)
    dsmul(qa,v[2].b[j],s,1,s1,1);
      dvadd (qt,G->sx.p,1,fac,1,fac,1);
      dvvtvp(qt,fac, 1,ub,1,ux,1,ux,1);
      dgemm ('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,fac,qa*qb);
      dvvtvp(qt,G->tx.p,1,fac,1,ux,1,ux,1);
      break;
    case 'y':
      dvadd(qt,G->sy.p,1,G->ty.p,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,v[1].a,1,fac+i*qa,1,fac+i*qa,1);
      dvadd(qt,G->ry.p,1,fac,1,fac,1);
      dvmul(qt,fac,1,ua,1,uy,1);
      for(i = 0, s=G->ty.p, s1 = fac; i < qc; ++i)
  for(j = 0; j < qb; ++j,s += qa, s1 +=qa)
    dsmul(qa,v[2].b[j],s,1,s1,1);
      dvadd (qt,G->sy.p,1,fac,1,fac,1);
      dvvtvp(qt,fac    ,1,ub,1,uy,1,uy,1);
      dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,fac,qa*qb);
      dvvtvp(qt,G->ty.p,1,fac,1,uy,1,uy,1);
      break;
    case 'z':
      dvadd(qt,G->sz.p,1,G->tz.p,1,fac,1);
      for(i = 0; i < qb*qc; ++i) dvmul(qa,v[1].a,1,fac+i*qa,1,fac+i*qa,1);
      dvadd (qt,G->rz.p,1,fac,1,fac,1);
      dvmul (qt,fac,1,ua,1,uz,1);
      for(i = 0, s=G->tz.p, s1 = fac; i < qc; ++i)
  for(j = 0; j < qb; ++j,s += qa, s1+=qa)
    dsmul(qa,v[2].b[j],s,1,s1,1);
      dvadd (qt,G->sz.p,1,fac,1,fac,1);
      dvvtvp(qt,fac,1,ub,1,uz,1,uz,1);
      dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,fac,qa*qb);
      dvvtvp(qt,G->tz.p,1,fac,1,uz,1,uz,1);
      break;
    }
  }
  return;
}




void Pyr::Grad_h(double *u, double *ux, double *uy, double *uz, char trip){
  Basis     *B = getbasis();
  double    **da,**dat,**db,**dbt,**dc,**dct;
  Geom      *G  = geom;
  double    *ua = Pyr_Grad_wk;
  double    *ub = ua+QGmax*QGmax*QGmax;
  double    *uc = ua+2*QGmax*QGmax*QGmax;
  int i,j;

  getD(&da,&dat,&db,&dbt,&dc,&dct);

  /*-------------------------------------------------------------------*
   * note du/dx = du/dr * dr/dx + du/ds * ds/dx + du/dt * dt/dx        *
   *      du/dy = du/dr * dr/dy + du/ds * ds/dy + du/dt * dt/dy        *
   *      du/dz = du/dr * dr/dz + du/ds * ds/dz + du/dt * dt/dz        *
   *                                                                   *
   * In 2D:                                                            *
   *      du/dr =   (2/1-c) du/da                                      *
   *      du/ds =   (2/1-c) du/db                                      *
   *      du/dt =   (1+a/1-c) du_da + (1+b/1-c) du_db + du/dc          *
   *-------------------------------------------------------------------*/

  /* calculate du/dr */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,u,qa,0.0,ua,qa);

  for(i=0;i<qc;++i)
    dscal(qa*qb, 1./B->vert[0].c[i], ua+i*qa*qb, 1);

  // calculate du/ds
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0,u+i*qa*qb,qa,*db,qb,0.0,ub+i*qa*qb,qa);

  for(i=0;i<qc;++i)
    dscal(qa*qb, 1./B->vert[0].c[i], ub+i*qa*qb, 1);

  // calculate du/dt
  dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,uc,qa*qb);

  for(i=0;i<qb*qc;++i)
    dvvtvp(qa, B->vert[1].a, 1, ua+i*qa, 1, uc+i*qa, 1, uc+i*qa, 1);

  for(i=0;i<qc;++i)
    for(j=0;j<qb;++j)
      daxpy(qa, B->vert[1].a[j], ub+j*qa+i*qa*qb, 1, uc+j*qa+i*qa*qb,1);

  double *rx  =  G->rx.p,  *ry  =  G->ry.p, *rz = G->rz.p;
  double *sx  =  G->sx.p,  *sy  =  G->sy.p, *sz = G->sz.p;
  double *tx  =  G->tx.p,  *ty  =  G->ty.p, *tz = G->tz.p;

  switch(trip){
  case 'a':

    dvmul  (qtot,rx,1,ua,1,ux,1);
    dvvtvp (qtot,sx,1,ub,1,ux,1,ux,1);
    dvvtvp (qtot,tx,1,uc,1,ux,1,ux,1);

    dvmul  (qtot,ry,1,ua,1,uy,1);
    dvvtvp (qtot,sy,1,ub,1,uy,1,uy,1);
    dvvtvp (qtot,ty,1,uc,1,uy,1,uy,1);

    dvmul  (qtot,rz,1,ua,1,uz,1);
    dvvtvp (qtot,sz,1,ub,1,uz,1,uz,1);
    dvvtvp (qtot,tz,1,uc,1,uz,1,uz,1);
    break;
  case 'x':

    dvmul  (qtot,rx,1,ua,1,ux,1);
    dvvtvp (qtot,sx,1,ub,1,ux,1,ux,1);
    dvvtvp (qtot,tx,1,uc,1,ux,1,ux,1);

    break;
  case 'y':

    dvmul  (qtot,ry,1,ua,1,uy,1);
    dvvtvp (qtot,sy,1,ub,1,uy,1,uy,1);
    dvvtvp (qtot,ty,1,uc,1,uy,1,uy,1);
    break;
  case 'z':

    dvmul  (qtot,rz,1,ua,1,uz,1);
    dvvtvp (qtot,sz,1,ub,1,uz,1,uz,1);
    dvvtvp (qtot,tz,1,uc,1,uz,1,uz,1);
    break;

  }

  return;
}




void Prism::Grad_h(double *u, double *ux, double *uy, double *uz, char trip){
  Basis     *B = getbasis();
  double    **da,**dat,**db,**dbt,**dc,**dct;
  Geom      *G  = geom;
  double    *ua = Prism_Grad_wk;
  double    *ub = ua+QGmax*QGmax*QGmax;
  double    *uc = ua+2*QGmax*QGmax*QGmax;
  int i;

  getD(&da,&dat,&db,&dbt,&dc,&dct);

  /*-------------------------------------------------------------------*
   * note du/dx = du/dr * dr/dx + du/ds * ds/dx + du/dt * dt/dx        *
   *      du/dy = du/dr * dr/dy + du/ds * ds/dy + du/dt * dt/dy        *
   *      du/dz = du/dr * dr/dz + du/ds * ds/dz + du/dt * dt/dz        *
   *                                                                   *
   * In 2D:                                                            *
   *      du/dr =   (2/1-c) du/da                                      *
   *      du/ds =   du/db                                              *
   *      du/dt =   (1+a/1-c) du_da + du/dc                            *
   *-------------------------------------------------------------------*/

  /* calculate du/dr */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,u,qa,0.0,ua,qa);

  for(i=0;i<qc;++i)
    dscal(qa*qb, 1./B->vert[0].c[i], ua+i*qa*qb, 1);

  // calculate du/ds
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0,u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);

  // calculate du/dt
  dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,uc,qa*qb);

  for(i=0;i<qb*qc;++i)
    dvvtvp(qa, B->vert[1].a, 1, ua+i*qa, 1, uc+i*qa, 1, uc+i*qa, 1);

  double *rx  =  G->rx.p,  *ry  =  G->ry.p, *rz = G->rz.p;
  double *sx  =  G->sx.p,  *sy  =  G->sy.p, *sz = G->sz.p;
  double *tx  =  G->tx.p,  *ty  =  G->ty.p, *tz = G->tz.p;

  switch(trip){
  case 'a':

    dvmul  (qtot,rx,1,ua,1,ux,1);
    dvvtvp (qtot,sx,1,ub,1,ux,1,ux,1);
    dvvtvp (qtot,tx,1,uc,1,ux,1,ux,1);

    dvmul  (qtot,ry,1,ua,1,uy,1);
    dvvtvp (qtot,sy,1,ub,1,uy,1,uy,1);
    dvvtvp (qtot,ty,1,uc,1,uy,1,uy,1);

    dvmul  (qtot,rz,1,ua,1,uz,1);
    dvvtvp (qtot,sz,1,ub,1,uz,1,uz,1);
    dvvtvp (qtot,tz,1,uc,1,uz,1,uz,1);
    break;
  case 'x':

    dvmul  (qtot,rx,1,ua,1,ux,1);
    dvvtvp (qtot,sx,1,ub,1,ux,1,ux,1);
    dvvtvp (qtot,tx,1,uc,1,ux,1,ux,1);

    break;
  case 'y':

    dvmul  (qtot,ry,1,ua,1,uy,1);
    dvvtvp (qtot,sy,1,ub,1,uy,1,uy,1);
    dvvtvp (qtot,ty,1,uc,1,uy,1,uy,1);
    break;
  case 'z':

    dvmul  (qtot,rz,1,ua,1,uz,1);
    dvvtvp (qtot,sz,1,ub,1,uz,1,uz,1);
    dvvtvp (qtot,tz,1,uc,1,uz,1,uz,1);
    break;

  }

  return;
}




void Hex::Grad_h(double *u, double *ux, double *uy, double *uz, char trip){
  double    **da,**dat,**db,**dbt,**dc,**dct;
  Geom      *G  = geom;
  double    *ua = Hex_Grad_wk;
  double    *ub = ua+QGmax*QGmax*QGmax;
  double    *uc = ua+2*QGmax*QGmax*QGmax;
  int i;

  getD(&da,&dat,&db,&dbt,&dc,&dct);

  /*-------------------------------------------------------------------*
   * note du/dx = du/dr * dr/dx + du/ds * ds/dx + du/dt * dt/dx        *
   *      du/dy = du/dr * dr/dy + du/ds * ds/dy + du/dt * dt/dy        *
   *      du/dz = du/dr * dr/dz + du/ds * ds/dz + du/dt * dt/dz        *
   *                                                                   *
   * In 2D:                                                            *
   *      du/dr =   du/da |_bc                                         *
   *      du/ds =   du/db |_ac                                         *
   *      du/dt =   du/dc |_ab                                         *
   *-------------------------------------------------------------------*/

  /* calculate du/da */
  dgemm('T','N',qa,qb*qc,qa,1.0,*da,qa,u,qa,0.0,ua,qa);

  // calculate du/db
  for(i = 0; i < qc; ++i)
    dgemm('N','N',qa,qb,qb,1.0,u+i*qa*qb,qa,*db,qb,
    0.0,ub+i*qa*qb,qa);

  // calculate du/dc
  dgemm('N','N',qa*qb,qc,qc,1.0,u,qa*qb,*dc,qc,0.0,uc,qa*qb);

  double *rx  =  G->rx.p,  *ry  =  G->ry.p, *rz = G->rz.p;
  double *sx  =  G->sx.p,  *sy  =  G->sy.p, *sz = G->sz.p;
  double *tx  =  G->tx.p,  *ty  =  G->ty.p, *tz = G->tz.p;

  switch(trip){
  case 'a':

    dvmul  (qtot,rx,1,ua,1,ux,1);
    dvvtvp (qtot,sx,1,ub,1,ux,1,ux,1);
    dvvtvp (qtot,tx,1,uc,1,ux,1,ux,1);

    dvmul  (qtot,ry,1,ua,1,uy,1);
    dvvtvp (qtot,sy,1,ub,1,uy,1,uy,1);
    dvvtvp (qtot,ty,1,uc,1,uy,1,uy,1);

    dvmul  (qtot,rz,1,ua,1,uz,1);
    dvvtvp (qtot,sz,1,ub,1,uz,1,uz,1);
    dvvtvp (qtot,tz,1,uc,1,uz,1,uz,1);
    break;
  case 'x':

    dvmul  (qtot,rx,1,ua,1,ux,1);
    dvvtvp (qtot,sx,1,ub,1,ux,1,ux,1);
    dvvtvp (qtot,tx,1,uc,1,ux,1,ux,1);

    break;
  case 'y':

    dvmul  (qtot,ry,1,ua,1,uy,1);
    dvvtvp (qtot,sy,1,ub,1,uy,1,uy,1);
    dvvtvp (qtot,ty,1,uc,1,uy,1,uy,1);
    break;
  case 'z':

    dvmul  (qtot,rz,1,ua,1,uz,1);
    dvvtvp (qtot,sz,1,ub,1,uz,1,uz,1);
    dvvtvp (qtot,tz,1,uc,1,uz,1,uz,1);
    break;

  }

  return;
}


void Element::Grad_h (double *,double *, double *, double *, char ){ERR;}


/*

Function name: Element::GradT_h

Function Purpose:
Output will be the transpose derivatives (in a Galerkin projection sense)
of the field stored in the vector H.

Argument 1: double *H
Purpose:
Field (assumed to be given in normal quadrature storage format) to be differentiated.

Argument 2: double *ux
Purpose:
Array for storing the 'x' derivative of the field held in H.

Argument 3: double *uy
Purpose:
Array for storing the 'y' derivative of the field held in H.

Argument 4: double *uz
Purpose:
Array for storing the 'z' derivative of the field held in H.

Argument 5: char trip
Purpose:
Flag which determines which derivatives are to be calculated.
 Options:

Function Notes:

Calculates

The Galerkin projection against the derivative of the Lagrange basis
W^-1 D^T W

*/

void Tri::GradT_h(double *H, double *ux, double *uy, double *uz, char trip){
    GradT_h(H,ux,uy,uz,0);
}

void Tri::GradT_h(double *H, double *ux, double *uy, double *uz, char trip,
      bool invW){
  register  int i;
  const     int qt = qa*qb;
  double    **da,**dat,**db,**dbt,*z,*wa,*wb;
  Geom      *G = geom;
  Mode      *v = getbasis()->vert;
  double    *u = H;
  double    *ua = Tri_wk.get();
  double    *ub = ua + QGmax*QGmax;
  double    *W  = ub + QGmax*QGmax;

  uz=uz; // compiler fix

  getD(&da,&dat,&db,&dbt,NULL,NULL);

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'b');

  /*-------------------------------------------------------------------*
   * note du/dx^T = d/dr^T ( dr/dx * u )+ d/ds^T ( ds/dx * u)          *
   *      du/dy^T = d/dr^T ( dr/dy * u )+ d/ds^T ( ds/dy * u)          *
   *                                                                   *
   *      d/dr^T =  d/da^T |_b 2.0/(1-b)                               *
   *      d/ds^T =  d/da^T |_b (1+a)/(1-b) +  d /db^T |_a              *
   *                                                                   *
   *-------------------------------------------------------------------*/

  if(!curvX){
    double rx,ry,sx,sy;

    rx  =  G->rx.d;
    ry  =  G->ry.d;
    sx  =  G->sx.d;
    sy  =  G->sy.d;

    // set up W
    for(i = 0; i < qb; ++i)
      dsmul(qa,wb[i],wa,1,W+i*qa,1);
    dscal(qt,G->jac.d,W,1);

    switch(trip){
    case 'a': case 'x':
      if(sx){
  dsmul (qt,sx,u,1,ub,1);

  for(i = 0; i < qb; ++i)
    dvmul(qa,v[1].a,1,ub+i*qa,1,ua+i*qa,1);

  dvmul (qt,W,1,ub,1,ub,1);
  mxm   (*dbt,qb,ub,qb,ux,qa);

  if(rx)
    daxpy(qt,rx,u,1,ua,1);

  dvmul(qt,W,1,ua,1,ua,1);
      }
      else{
  dzero(qt,ux,1);
  dsmul(qt,rx,u,1,ua,1);
  dvmul(qt,W,1,ua,1,ua,1);
      }

      for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],ua+i*qa,1);

      /* calculate d/da  and add to ux*/
      mxm(ua,qb,*da,qa,ub,qa);
      dvadd(qt,ub,1,ux,1,ux,1);

      if(trip == 'x') break;

    case 'y':
      if(sy){
   dsmul (qt,sy,u,1,ub,1);

  for(i = 0; i < qb; ++i)
    dvmul(qa,v[1].a,1,ub+i*qa,1,ua+i*qa,1);

  dvmul (qt,W,1,ub,1,ub,1);
  mxm   (*dbt,qb,ub,qb,uy,qa);

  if(ry)
    daxpy(qt,ry,u,1,ua,1);

  dvmul(qt,W,1,ua,1,ua,1);
      }
      else{
  dzero(qt,uy,1);
  dsmul(qt,ry,u,1,ua,1);
  dvmul(qt,W,1,ua,1,ua,1);
      }

      for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],ua+i*qa,1);

      /* calculate d/da  and add to uy*/
      mxm(ua,qb,*da,qa,ub,qa);
      dvadd(qt,ub,1,uy,1,uy,1);
      break;
    }
  }
  else{
    double *rx,*ry,*sx,*sy;

    rx  =  G->rx.p;
    ry  =  G->ry.p;
    sx  =  G->sx.p;
    sy  =  G->sy.p;

    // set up W
    for(i = 0; i < qb; ++i)
      dsmul(qa,wb[i],wa,1,W+i*qa,1);
    dvmul(qt,G->jac.p,1,W,1,W,1);

    switch(trip){
    case 'a': case 'x':
      dvmul(qt,sx,1,u,1,ub,1);

      for(i = 0; i < qb; ++i)
  dvmul(qa,v[1].a,1,ub+i*qa,1,ua+i*qa,1);

      dvmul(qt,W,1,ub,1,ub,1);
      mxm(*dbt,qb,ub,qb,ux,qa);


      dvvtvp(qt,rx,1,u,1,ua,1,ua,1);

      dvmul(qt,W,1,ua,1,ua,1);

      for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],ua+i*qa,1);

      /* calculate d/da  and add to ux*/
      mxm(ua,qb,*da,qa,ub,qa);
      dvadd(qt,ub,1,ux,1,ux,1);

      if(trip == 'x') break;

    case 'y':
      dvmul(qt,sy,1,u,1,ub,1);

      for(i = 0; i < qb; ++i)
  dvmul(qa,v[1].a,1,ub+i*qa,1,ua+i*qa,1);

      dvmul(qt,W,1,ub,1,ub,1);
      mxm(*dbt,qb,ub,qb,uy,qa);

      dvvtvp(qt,ry,1,u,1,ua,1,ua,1);

      dvmul(qt,W,1,ua,1,ua,1);

      for(i = 0; i < qb; ++i)  dscal(qa,1/v->b[i],ua+i*qa,1);

      /* calculate d/da  and add to uy*/
      mxm(ua,qb,*da,qa,ub,qa);
      dvadd(qt,ub,1,uy,1,uy,1);
      break;
    }
  }

  if(invW)
  {
      // Multiply by  W^{-1}
      dvrecp (qt,W,1,W,1);
      switch(trip){
      case 'a':
    dvmul  (qt,W,1,ux,1,ux,1);
    dvmul  (qt,W,1,uy,1,uy,1);
    break;
      case 'x':
    dvmul  (qt,W,1,ux,1,ux,1);
    break;
      case 'y':
    dvmul  (qt,W,1,uy,1,uy,1);
    break;
      }
  }
  return;
}

void Quad::GradT_h(double *H, double *ux, double *uy, double *uz, char trip){
      GradT_h(H,ux,uy,uz,0);
}

void Quad::GradT_h(double *H, double *ux, double *uy, double *uz, char trip,
       bool invW){
    const     int qt = qa*qb;
    int       i;
    double    **da,**dat,**db,**dbt,*z,*wa,*wb;
    Geom      *G = geom;
    double    *u = H;
    double    *ua = Quad_Grad_wk;
    double    *ub = ua + QGmax*QGmax;
    double    *W  = ub + QGmax*QGmax;

    uz=uz; // compiler fix

    getD(&da,&dat,&db,&dbt,NULL,NULL);
    getzw(qa,&z,&wa,'a');
    getzw(qb,&z,&wb,'a');

    /*-------------------------------------------------------------------*
     * note du/dx^T = d/dr^T ( dr/dx * u )+ d/ds^T ( ds/dx * u)          *
     *      du/dy^T = d/dr^T ( dr/dy * u )+ d/ds^T ( ds/dy * u)          *
     *                                                                   *
     *      d/dr^T =  d/da^T |_b                                         *
     *      d/ds^T =  d/db^T |_a                                        *
     *-------------------------------------------------------------------*/

    double *rx,*ry,*sx,*sy;

    rx  =  G->rx.p;
    ry  =  G->ry.p;
    sx  =  G->sx.p;
    sy  =  G->sy.p;

    // set up W
    for(i = 0; i < qb; ++i)
  dsmul(qa,wb[i],wa,1,W+i*qa,1);
    dvmul(qt,G->jac.p,1,W,1,W,1);

    switch(trip){
    case 'a': case 'x':
  dvmul(qt,sx,1,u,1,ub,1);
  dvmul(qt,W,1,ub,1,ub,1);
  mxm  (*dbt,qb,ub,qb,ux,qa);

  dvmul(qt,rx,1,u,1,ua,1);
  dvmul(qt,W,1,ua,1,ua,1);
  mxm  (ua,qb,*da,qa,ub,qa);

  dvadd(qt,ub,1,ux,1,ux,1);

  if(trip == 'x') break;

    case 'y':
  dvmul(qt,sy,1,u,1,ub,1);
  dvmul(qt,W,1,ub,1,ub,1);
  mxm(*dbt,qb,ub,qb,uy,qa);

  dvmul(qt,ry,1,u,1,ua,1);
  dvmul(qt,W,1,ua,1,ua,1);
  mxm(ua,qb,*da,qa,ub,qa);

  dvadd(qt,ub,1,uy,1,uy,1);
  break;
    }

    if(invW){
  // Multiply by  W^{-1}
  dvrecp (qt,W,1,W,1);
  switch(trip){
  case 'a':
      dvmul  (qt,W,1,ux,1,ux,1);
      dvmul  (qt,W,1,uy,1,uy,1);
      break;
  case 'x':
      dvmul  (qt,W,1,ux,1,ux,1);
      break;
  case 'y':
      dvmul  (qt,W,1,uy,1,uy,1);
      break;
  }
    }

  return;
}


  static Basis *Tri_add_dbase   (int L, int qa, int qb, int qc);
static Basis *Quad_add_dbase  (int L, int qa, int qb, int qc);
static Basis *Tet_add_dbase   (int L, int qa, int qb, int qc);
static Basis *Pyr_add_dbase   (int L, int qa, int qb, int qc);
static Basis *Prism_add_dbase (int L, int qa, int qb, int qc);
static Basis *Hex_add_dbase   (int L, int qa, int qb, int qc);

static void   Tri_diff_base(Basis *b, int L, int qa, int qb, int qc);
static void  Quad_diff_base(Basis *b, int L, int qa, int qb, int qc);
static void   Tet_diff_base(Basis *b, int L, int qa, int qb, int qc);
static void   Pyr_diff_base(Basis *b, int L, int qa, int qb, int qc);
static void Prism_diff_base(Basis *b, int L, int qa, int qb, int qc);
static void   Hex_diff_base(Basis *b, int L, int qa, int qb, int qc);

static Basis  *Tri_dbinf=NULL,*Tri_dbase=NULL;
static Basis ****Quad_dbase=NULL;
static Basis  *Tet_dbinf,*Tet_dbase;
static Basis  *Pyr_dbinf,*Pyr_dbase;
static Basis  *Prism_dbinf,*Prism_dbase;
static Basis  *Hex_dbinf,*Hex_dbase;


/*

Function name: Element::derbasis

Function Purpose:

Function Notes:

*/

Basis *Tri::derbasis(){
  /* check link list */

  for(Tri_dbinf = Tri_dbase; Tri_dbinf; Tri_dbinf = Tri_dbinf->next)
    if(Tri_dbinf->id == lmax &&
       Tri_dbinf->qa == qa &&
       Tri_dbinf->qb == qb){
      return Tri_dbinf;
    }

  /* else add new zeros and weights */
  Tri_dbinf = Tri_dbase;
  Tri_dbase = Tri_add_dbase(lmax,qa,qb,qc);
  Tri_dbase->next = Tri_dbinf;

  return Tri_dbase;
}




Basis *Quad::derbasis(){

  int i,j;
  if(!Quad_dbase){
    Quad_dbase = (Basis****) calloc(LGmax+1, sizeof(Basis***));
    for(i=0;i<LGmax+1;++i){
      Quad_dbase[i] = (Basis***) calloc(QGmax+1, sizeof(Basis**));
      for(j=0;j<QGmax+1;++j)
  Quad_dbase[i][j] = (Basis**) calloc(QGmax+1, sizeof(Basis*));
    }
  }
  if(!Quad_dbase[lmax][qa][qb])
    Quad_dbase[lmax][qa][qb] = Quad_add_dbase(lmax,qa,qb,qc);
  return Quad_dbase[lmax][qa][qb];
}




Basis *Tet::derbasis(){
  /* check link list */

  for(Tet_dbinf = Tet_dbase; Tet_dbinf; Tet_dbinf = Tet_dbinf->next)
    if(Tet_dbinf->id == lmax){
      return Tet_dbinf;
    }

  /* else add new zeros and weights */
  Tet_dbinf = Tet_dbase;
  Tet_dbase = Tet_add_dbase(lmax,qa,qb,qc);
  Tet_dbase->next = Tet_dbinf;

  return Tet_dbase;
}




Basis *Pyr::derbasis(){
  /* check link list */

  for(Pyr_dbinf = Pyr_dbase; Pyr_dbinf; Pyr_dbinf = Pyr_dbinf->next)
    if(Pyr_dbinf->id == lmax){
      return Pyr_dbinf;
    }

  /* else add new zeros and weights */
  Pyr_dbinf = Pyr_dbase;
  Pyr_dbase = Pyr_add_dbase(lmax,qa,qb,qc);
  Pyr_dbase->next = Pyr_dbinf;

  return Pyr_dbase;
}




Basis *Prism::derbasis(){
  /* check link list */

  for(Prism_dbinf = Prism_dbase; Prism_dbinf; Prism_dbinf = Prism_dbinf->next)
    if(Prism_dbinf->id == lmax){
      return Prism_dbinf;
    }

  /* else add new zeros and weights */
  Prism_dbinf = Prism_dbase;
  Prism_dbase = Prism_add_dbase(lmax,qa,qb,qc);
  Prism_dbase->next = Prism_dbinf;

  return Prism_dbase;
}




Basis *Hex::derbasis(){
  /* check link list */

  for(Hex_dbinf = Hex_dbase; Hex_dbinf; Hex_dbinf = Hex_dbinf->next)
    if(Hex_dbinf->id == lmax){
      return Hex_dbinf;
    }

  /* else add new zeros and weights */
  Hex_dbinf = Hex_dbase;
  Hex_dbase = Hex_add_dbase(lmax,qa,qb,qc);
  Hex_dbase->next = Hex_dbinf;

  return Hex_dbase;
}




Basis *Element::derbasis(){return (Basis*)NULL;}



static Basis *Tri_add_dbase(int L, int qa, int qb, int qc){
  Basis   *b = Tri_mem_base(L,qa,qb,qc);

  /* get complete base for a given L with independent storage */
  /* set up storage */
  Tri_mem_modes(b);

  /* set up vertices */
  Tri_set_vertices(b->vert,qa,'a');
  Tri_set_vertices(b->vert,qb,'b');

  if(L>2){
    Tri_set_edges(b,qa,'a');
    Tri_set_edges(b,qb,'b');
  }

  if(L>3)
    Tri_set_faces(b,qb,'b');

  /* differentiate with respect to local co-ordintate independent modes */
  Tri_diff_base(b,L,qa,qb,qc);

  return b;
}

static void Tri_diff_base(Basis *b, int L, int qa, int qb, int qc){
  register int i;
  Mode *m;
  double *za,*zb,*w,**da,**dat,**db,**dbt;
  double *tmp;

  tmp = dvector(0,max(qa,max(qb,qc))-1);

  da  = dmatrix(0,qa-1,0,qa-1);
  dat = dmatrix(0,qa-1,0,qa-1);
  db  = dmatrix(0,qb-1,0,qb-1);
  dbt = dmatrix(0,qb-1,0,qb-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'b');

  dglj(da,dat,za,qa,0.0,0.0);
  if(LZero)
    dgrj(db,dbt,zb,qb,0.0,0.0);
  else
    dgrj(db,dbt,zb,qb,1.0,0.0);

  m = b->vert;
  /* Vertices */
  for(i = 0; i < 3; ++i){
    dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
    dcopy(qa,tmp,1,m[i].a,1);
  }

  for(i = 1; i < 3; ++i){
    dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
    dcopy(qb,tmp,1,m[i].b,1);
  }

  /* edges */
  if(L>2){
    m = b->edge[0];
    for(i = 0; i < L-2; ++i){
      dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
      dcopy(qa,tmp,1,m[i].a,1);

      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);
    }

    m = b->edge[1];
    for(i = 0; i < L-2; ++i){
      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);
    }
  }
  if(L>3){
    /* faces */
    m = b->face[0][0];
    for(i = 0; i < (L-3)*(L-2)/2; ++i){
      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);
    }
  }
  free(tmp);
  free_dmatrix(da,0,0); free_dmatrix(dat,0,0);
  free_dmatrix(db,0,0); free_dmatrix(dbt,0,0);
}


static Basis *Quad_add_dbase(int L, int qa, int qb, int qc){
  Basis   *b = Quad_mem_base(L,qa,qb,qc);

  /* get complete base for a given L with independent storage */
  /* set up storage */
  Quad_mem_modes(b);

  /* set up vertices */
  Quad_set_vertices(b,qa,'a');
  Quad_set_vertices(b,qb,'b');

  if(L>2){
    Quad_set_edges(b,qa,'a');
    Quad_set_edges(b,qb,'b');
  }

  if(L>2)
    Quad_set_faces(b,qb,'b');

  /* differentiate with respect to local co-ordintate independent modes */
  Quad_diff_base(b,L,qa,qb,qc);

  return b;
}

// DONE

static void Quad_diff_base(Basis *b, int L, int qa, int qb, int qc){

  register int i;
  Mode *m;
  double *za,*zb,*w,**da,**dat,**db,**dbt,*tmp;

  qc = qc; // compiler fix

  tmp = dvector(0,QGmax-1);

  da  = dmatrix(0,qa-1,0,qa-1);
  dat = dmatrix(0,qa-1,0,qa-1);
  db  = dmatrix(0,qb-1,0,qb-1);
  dbt = dmatrix(0,qb-1,0,qb-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'a');

  dglj(da,dat,za,qa,0.0,0.0);
  dglj(db,dbt,zb,qb,0.0,0.0);

  /* Vertices */
  m = b->vert;
  for(i = 0; i < 2; ++i){
    dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
    dcopy(qa,tmp,1,m[i].a,1);
  }

  for(i = 1; i < 3; ++i){
    dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
    dcopy(qb,tmp,1,m[i].b,1);
  }

  /* edges */
  if(L>2){
    for(i = 0; i < L-2; ++i){
      m = b->edge[0];
      dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
      dcopy(qa,tmp,1,m[i].a,1);

      m = b->edge[1];
      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);
    }
  }

  free(tmp);
  free_dmatrix(da,0,0); free_dmatrix(dat,0,0);
  free_dmatrix(db,0,0); free_dmatrix(dbt,0,0);
}



static Basis *Tet_add_dbase(int L, int qa, int qb, int qc){
  Basis   *b = Tet_mem_base(L,qa,qb,qc);

  /* get complete base for a given L with independent storage */
  /* set up storage */
  Tet_mem_modes(b);

  /* set up vertices */
  Tet_set_vertices(b->vert,qa,'a');
  Tet_set_vertices(b->vert,qb,'b');
  Tet_set_vertices(b->vert,qc,'c');

  if(L>2){
    Tet_set_edges(b,qa,'a');
    Tet_set_edges(b,qb,'b');
    Tet_set_edges(b,qc,'c');
  }

  if(L>3){
    Tet_set_faces(b,qa,'a');
    Tet_set_faces(b,qb,'b');
    Tet_set_faces(b,qc,'c');
  }

  /* differentiate with respect to local co-ordintate independent modes */
  Tet_diff_base(b,L,qa,qb,qc);

  return b;
}

static void Tet_diff_base(Basis *b, int L, int qa, int qb, int qc){
  register int i;
  Mode *m;
  double *za,*zb,*w,**da,**dat,**db,**dbt,*tmp;
  double *zc,**dc,**dct;

  tmp = dvector(0,max(qa,max(qb,qc))-1);

  da  = dmatrix(0,qa-1,0,qa-1);
  dat = dmatrix(0,qa-1,0,qa-1);
  db  = dmatrix(0,qb-1,0,qb-1);
  dbt = dmatrix(0,qb-1,0,qb-1);
  dc  = dmatrix(0,qc-1,0,qc-1);
  dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'b');
  getzw(qc,&zc,&w,'c');
  if(LZero){
    dglj(da,dat,za,qa,0.0,0.0);
    dgrj(db,dbt,zb,qb,0.0,0.0);
    dgrj(dc,dct,zc,qc,0.0,0.0);
  }
  else{
    dglj(da,dat,za,qa,0.0,0.0);
    dgrj(db,dbt,zb,qb,1.0,0.0);
    dgrj(dc,dct,zc,qc,2.0,0.0);
  }
  m = b->vert;
  /* Vertices */
  for(i = 0; i < 3; ++i){
    dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
    dcopy(qa,tmp,1,m[i].a,1);
  }

  for(i = 1; i < 4; ++i){
    dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
    dcopy(qb,tmp,1,m[i].b,1);
  }
  for(i = 2; i < 4; ++i){
    dgemv('T',qc,qc,1.0,*dc,qc,m[i].c,1,0.0,tmp,1);
    dcopy(qc,tmp,1,m[i].c,1);
  }

  /* edges */
  if(L>2){
    m = b->edge[0];
    for(i = 0; i < L-2; ++i){
      dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
      dcopy(qa,tmp,1,m[i].a,1);

      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);

      dgemv('T',qc,qc,1.0,*dc,qc,m[i].c,1,0.0,tmp,1);
      dcopy(qc,tmp,1,m[i].c,1);
    }

    m = b->edge[1];
    for(i = 0; i < L-2; ++i){
      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);
    }

    m = b->edge[3];
    for(i = 0; i < L-2; ++i){
      dgemv('T',qc,qc,1.0,*dc,qc,m[i].c,1,0.0,tmp,1);
      dcopy(qc,tmp,1,m[i].c,1);
    }
  }

  if(L>3){
    /* faces */
    m = b->face[0][0];
    for(i = 0; i < (L-3)*(L-2)/2; ++i){
      dgemv('T',qb,qb,1.0,*db,qb,m[i].b,1,0.0,tmp,1);
      dcopy(qb,tmp,1,m[i].b,1);
    }
    m = b->face[1][0];
    for(i = 0; i < (L-3)*(L-2)/2; ++i){
      dgemv('T',qc,qc,1.0,*dc,qc,m[i].c,1,0.0,tmp,1);
      dcopy(qc,tmp,1,m[i].c,1);
    }
  }
  free(tmp);
  free_dmatrix(da,0,0); free_dmatrix(dat,0,0);
  free_dmatrix(db,0,0); free_dmatrix(dbt,0,0);
  free_dmatrix(dc,0,0); free_dmatrix(dct,0,0);
}


static Basis *Pyr_add_dbase(int L, int qa, int qb, int qc){
  Basis   *b = Pyr_mem_base(L,qa,qb,qc);

  /* set up storage */
  Pyr_mem_modes(b);

  /* set up vertices */
  Pyr_set_vertices(b->vert,qa,'a');
  Pyr_set_vertices(b->vert,qb,'b');
  Pyr_set_vertices(b->vert,qc,'c');

  if(L>2){
    Pyr_set_edges(b,qa,'a');
    Pyr_set_edges(b,qa,'b');
    Pyr_set_edges(b,qc,'c');
  }

  if(L>3){
    Pyr_set_faces(b,qa,'a');
    Pyr_set_faces(b,qa,'b');
    Pyr_set_faces(b,qc,'c');
  }

  /* differentiate with respect to local co-ordintate independent modes */
  Pyr_diff_base(b,L,qa,qb,qc);

  return b;
}

static void Pyr_diff_base(Basis *b, int L, int qa, int qb, int qc){
  register int i;
  double *za,*w,**da,**dat,*tmp;
  double *zc,**dc,**dct;

  tmp = dvector(0,QGmax-1);

  da  = dmatrix(0,qa-1,0,qa-1);
  dat = dmatrix(0,qa-1,0,qa-1);
  dc  = dmatrix(0,qc-1,0,qc-1);
  dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qc,&zc,&w,'c');

  dglj(da,dat,za,qa,0.0,0.0);
  if(LZero)
    dgrj(dc,dct,zc,qc,0.0,0.0);
  else
    dgrj(dc,dct,zc,qc,2.0,0.0);

  int Namodes = L+1;

  for(i = 0; i < Namodes; ++i){
    dgemv('T',qa,qa,1.0,*da,qa,b->vert[4].a+i*qa,1,0.0,tmp,1);
    dcopy(qa,tmp,1,b->vert[4].a + i*qa,1);
  }

  int Ncmodes = L+L-2+(L-2)*(L-2)+(L-3)*(L-2)/2+
            (L-3)*(L-2)*(L-1)/6; // see Pyr_mem_modes

  for(i = 0; i < Ncmodes; ++i){
    dgemv('T',qc,qc,1.0,*dc,qc,b->vert[0].c+qc*i,1,0.0,tmp,1);
    dcopy(qc,tmp,1,b->vert[0].c +qc*i,1);
  }

  free(tmp);
  free_dmatrix(da,0,0); free_dmatrix(dat,0,0);
  free_dmatrix(dc,0,0); free_dmatrix(dct,0,0);
}



static Basis *Prism_add_dbase(int L, int qa, int qb, int qc){
  Basis   *b = Prism_mem_base(L,qa,qb,qc);

  /* set up storage */
  Prism_mem_modes(b);

  /* set up vertices */
  Prism_set_vertices(b->vert,qa,'a');
  Prism_set_vertices(b->vert,qb,'b');
  Prism_set_vertices(b->vert,qc,'c');

  if(L>2){
    Prism_set_edges(b,qa,'a');
    Prism_set_edges(b,qa,'b');
    Prism_set_edges(b,qc,'c');
  }

  if(L>3){
    Prism_set_faces(b,qa,'a');
    Prism_set_faces(b,qa,'b');
    Prism_set_faces(b,qc,'c');
  }

  /* differentiate with respect to local co-ordintate independent modes */
  Prism_diff_base(b,L,qa,qb,qc);

  return b;
}

static void Prism_diff_base(Basis *b, int L, int qa, int qb, int qc){
  register int i;

  double *za,*w,**da,**dat,*tmp;
  double *zc,**dc,**dct;

  tmp = dvector(0,max(qa,max(qb,qc))-1);

  da  = dmatrix(0,qa-1,0,qa-1);
  dat = dmatrix(0,qa-1,0,qa-1);
  dc  = dmatrix(0,qc-1,0,qc-1);
  dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qc,&zc,&w,'b');

  dglj(da,dat,za,qa,0.0,0.0);
  if(LZero)
    dgrj(dc,dct,zc,qc,0.0,0.0);
  else
    dgrj(dc,dct,zc,qc,1.0,0.0);

  int Namodes = L+1;

  // differentiate basis

  for(i = 0; i < Namodes; ++i){
    dgemv('T',qa,qa,1.0,*da,qa,b->vert[5].a+i*qa,1,0.0,tmp,1);
    dcopy(qa,tmp,1,b->vert[5].a + i*qa,1);
  }

  int Ncmodes = 2+ (L-2) + (L-2) + (L-3)*(L-2)/2;

  for(i = 0; i < Ncmodes; ++i){
    dgemv('T',qc,qc,1.0,*dc,qc,b->vert[0].c+qc*i,1,0.0,tmp,1);
    dcopy(qc,tmp,1,b->vert[0].c+qc*i,1);
  }

  free(tmp);
  free_dmatrix(da,0,0); free_dmatrix(dat,0,0);
  free_dmatrix(dc,0,0); free_dmatrix(dct,0,0);
}



static Basis *Hex_add_dbase(int L, int qa, int qb, int qc){
  Basis   *b = Hex_mem_base(L,qa,qb,qc);

  /* get complete base for a given L with independent storage */
  /* set up storage */
  Hex_mem_modes(b);

  /* set up vertices */
  Hex_set_vertices(b->vert,qa,'a');

  if(L>2)
    Hex_set_edges(b,qa,'a');

  if(L>2)
    Hex_set_faces(b,qb,'a');

  /* differentiate with respect to local co-ordintate independent modes */
  Hex_diff_base(b,L,qa,qb,qc);

  return b;
}

static void Hex_diff_base(Basis *b, int L, int qa, int qb, int qc){
  register int i;
  Mode *m;
  double *za,*w,**da,**dat,*tmp;

  tmp = dvector(0,max(qa,max(qb,qc))-1);

  da  = dmatrix(0,qa-1,0,qa-1);
  dat = dmatrix(0,qa-1,0,qa-1);

  getzw(qa,&za,&w,'a');

  dglj(da,dat,za,qa,0.0,0.0);

  m = b->vert;
  /* Vertices */
  for(i = 0; i < 2; ++i){
    dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
    dcopy(qa,tmp,1,m[i].a,1);
  }

  /* edges */
  if(L>2){
    m = b->edge[0];
    for(i = 0; i < L-2; ++i){
      dgemv('T',qa,qa,1.0,*da,qa,m[i].a,1,0.0,tmp,1);
      dcopy(qa,tmp,1,m[i].a,1);
    }
  }

  free(tmp);
  free_dmatrix(da,0,0); free_dmatrix(dat,0,0);

}

typedef struct tridinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Da;      /* Differential matrix for a direction                  */
  double **Dat;     /* Differential matrix for a direction (transposed)     */
  double **Db;      /* Differential matrix for b direction                  */
  double **Dbt;     /* Differential matrix for b direction (transposed)     */
  struct tridinfo *next;
} Tri_Dinfo;


typedef struct quaddinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Da;      /* Differential matrix for a direction                  */
  double **Dat;     /* Differential matrix for a direction (transposed)     */
  double **Db;      /* Differential matrix for b direction                  */
  double **Dbt;     /* Differential matrix for b direction (transposed)     */
  struct quaddinfo *next;
} Quad_Dinfo;

typedef struct tetdinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Da;      /* Differential matrix for a direction                  */
  double **Dat;     /* Differential matrix for a direction (transposed)     */
  double **Db;      /* Differential matrix for b direction                  */
  double **Dbt;     /* Differential matrix for b direction (transposed)     */
  double **Dc;      /* Differential matrix for b direction                  */
  double **Dct;     /* Differential matrix for b direction (transposed)     */
  struct tetdinfo *next;
} Tet_Dinfo;

typedef struct pyrdinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Da;      /* Differential matrix for a direction                  */
  double **Dat;     /* Differential matrix for a direction (transposed)     */
  double **Db;      /* Differential matrix for b direction                  */
  double **Dbt;     /* Differential matrix for b direction (transposed)     */
  double **Dc;      /* Differential matrix for b direction                  */
  double **Dct;     /* Differential matrix for b direction (transposed)     */
  struct pyrdinfo *next;
} Pyr_Dinfo;


typedef struct prismdinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Da;      /* Differential matrix for a direction                  */
  double **Dat;     /* Differential matrix for a direction (transposed)     */
  double **Db;      /* Differential matrix for b direction                  */
  double **Dbt;     /* Differential matrix for b direction (transposed)     */
  double **Dc;      /* Differential matrix for b direction                  */
  double **Dct;     /* Differential matrix for b direction (transposed)     */
  struct prismdinfo *next;
} Prism_Dinfo;

typedef struct hexdinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Da;      /* Differential matrix for a direction                  */
  double **Dat;     /* Differential matrix for a direction (transposed)     */
  double **Db;      /* Differential matrix for b direction                  */
  double **Dbt;     /* Differential matrix for b direction (transposed)     */
  double **Dc;      /* Differential matrix for b direction                  */
  double **Dct;     /* Differential matrix for b direction (transposed)     */
  struct hexdinfo *next;
} Hex_Dinfo;



static Tri_Dinfo *Tri_addD(Tri *U);
static Quad_Dinfo *Quad_addD(Quad *U);
static Tet_Dinfo *Tet_addD(Tet *U);
static Pyr_Dinfo *Pyr_addD(Pyr *U);
static Prism_Dinfo *Prism_addD(Prism *U);
static Hex_Dinfo *Hex_addD(Hex *U);

static Tri_Dinfo  **Tri_getD_base = NULL;
static Quad_Dinfo  **Quad_getD_base = NULL;
static Tet_Dinfo *Tet_Dinf,*Tet_Dbase;
static Pyr_Dinfo *Pyr_Dinf,*Pyr_Dbase;
static Prism_Dinfo *Prism_Dinf,*Prism_Dbase;
static Hex_Dinfo *Hex_Dinf,*Hex_Dbase;





/*

Function name: Element::getD

Function Purpose:

Argument 1: double ***da
Purpose:

Argument 2: double ***dat
Purpose:

Argument 3: double ***db
Purpose:

Argument 4: double ***dbt
Purpose:

Function Notes:

*/

void Tri::getD(double ***da, double ***dat,double ***db,double ***dbt,
         double ***, double ***){
  /* check link list */

  if(!Tri_getD_base)
    Tri_getD_base  = (Tri_Dinfo**) calloc(QGmax+1,sizeof(Tri_Dinfo*));

  if(!Tri_getD_base[qa])
    Tri_getD_base[qa] = Tri_addD(this);

  *da  = Tri_getD_base[qa]->Da;
  *dat = Tri_getD_base[qa]->Dat;

  *db  = Tri_getD_base[qa]->Db;
  *dbt = Tri_getD_base[qa]->Dbt;
  return;
}




void Quad::getD(double ***da, double ***dat,double ***db,double ***dbt,
         double ***, double ***){
  /* check link list */

  if(!Quad_getD_base)
    Quad_getD_base  = (Quad_Dinfo**) calloc(QGmax+1,sizeof(Quad_Dinfo*));

  if(!Quad_getD_base[qa])
    Quad_getD_base[qa] = Quad_addD(this);

  *da  = Quad_getD_base[qa]->Da;
  *dat = Quad_getD_base[qa]->Dat;

  *db  = Quad_getD_base[qa]->Db;
  *dbt = Quad_getD_base[qa]->Dbt;
  return;
}




void Tet::getD(double ***da, double ***dat,double ***db,double ***dbt,
         double ***dc, double ***dct){
  /* check link list */

  for(Tet_Dinf = Tet_Dbase; Tet_Dinf; Tet_Dinf = Tet_Dinf->next)
    if(Tet_Dinf->qa == qa){  /* note using qa as flad */
      *da = Tet_Dinf->Da;  *dat = Tet_Dinf->Dat;
      *db = Tet_Dinf->Db;  *dbt = Tet_Dinf->Dbt;
      *dc = Tet_Dinf->Dc;  *dct = Tet_Dinf->Dct;
      return;
    }

  /* else add Differential maTetces */

  Tet_Dinf  = Tet_Dbase;
  Tet_Dbase = Tet_addD(this);
  Tet_Dbase->next = Tet_Dinf;

  *da = Tet_Dbase->Da;  *dat = Tet_Dbase->Dat;
  *db = Tet_Dbase->Db;  *dbt = Tet_Dbase->Dbt;
  *dc = Tet_Dbase->Dc;  *dct = Tet_Dbase->Dct;
  return;
}




void Pyr::getD(double ***da, double ***dat,double ***db,double ***dbt,
         double ***dc, double ***dct){

  /* check link list */

  for(Pyr_Dinf = Pyr_Dbase; Pyr_Dinf; Pyr_Dinf = Pyr_Dinf->next)
    if(Pyr_Dinf->qa == qa){  /* note using qa as flad */
      *da = Pyr_Dinf->Da;  *dat = Pyr_Dinf->Dat;
      *db = Pyr_Dinf->Db;  *dbt = Pyr_Dinf->Dbt;
      *dc = Pyr_Dinf->Dc;  *dct = Pyr_Dinf->Dct;
      return;
    }

  /* else add Differential maPyrces */

  Pyr_Dinf  = Pyr_Dbase;
  Pyr_Dbase = Pyr_addD(this);
  Pyr_Dbase->next = Pyr_Dinf;

  *da = Pyr_Dbase->Da;  *dat = Pyr_Dbase->Dat;
  *db = Pyr_Dbase->Db;  *dbt = Pyr_Dbase->Dbt;
  *dc = Pyr_Dbase->Dc;  *dct = Pyr_Dbase->Dct;
  return;
}




void Prism::getD(double ***da, double ***dat,double ***db,double ***dbt,
     double ***dc, double ***dct){

  /* check link list */

  for(Prism_Dinf = Prism_Dbase; Prism_Dinf; Prism_Dinf = Prism_Dinf->next)
    if(Prism_Dinf->qa == qa){  /* note using qa as flad */
      *da = Prism_Dinf->Da;  *dat = Prism_Dinf->Dat;
      *db = Prism_Dinf->Db;  *dbt = Prism_Dinf->Dbt;
      *dc = Prism_Dinf->Dc;  *dct = Prism_Dinf->Dct;
      return;
    }

  /* else add Differential maPrismces */

  Prism_Dinf  = Prism_Dbase;
  Prism_Dbase = Prism_addD(this);
  Prism_Dbase->next = Prism_Dinf;

  *da = Prism_Dbase->Da;  *dat = Prism_Dbase->Dat;
  *db = Prism_Dbase->Db;  *dbt = Prism_Dbase->Dbt;
  *dc = Prism_Dbase->Dc;  *dct = Prism_Dbase->Dct;
  return;
}




void Hex::getD(double ***da, double ***dat,double ***db,double ***dbt,
         double ***dc, double ***dct){

  /* check link list */

  for(Hex_Dinf = Hex_Dbase; Hex_Dinf; Hex_Dinf = Hex_Dinf->next)
    if(Hex_Dinf->qa == qa){  /* note using qa as flad */
      *da = Hex_Dinf->Da;  *dat = Hex_Dinf->Dat;
      *db = Hex_Dinf->Db;  *dbt = Hex_Dinf->Dbt;
      *dc = Hex_Dinf->Dc;  *dct = Hex_Dinf->Dct;
      return;
    }

  /* else add Differential maHexces */

  Hex_Dinf  = Hex_Dbase;
  Hex_Dbase = Hex_addD(this);
  Hex_Dbase->next = Hex_Dinf;

  *da = Hex_Dbase->Da;  *dat = Hex_Dbase->Dat;
  *db = Hex_Dbase->Db;  *dbt = Hex_Dbase->Dbt;
  *dc = Hex_Dbase->Dc;  *dct = Hex_Dbase->Dct;
  return;
}




void Element::getD(double ***,double ***,
       double ***,double ***,
       double ***,double ***){ERR;}






/*--------------------------------------------------------------------------*
 * Dz is the first order differential matrix operating on the qt quadrature *
 * points. So that the product of D and a vector U(z_q) gives the partial  *
 * of U with respect to z. i.e.:                                            *
 *                                                                          *
 *              du                                                          *
 *              -- [i]  =  sum  Dz[i,j] U[j]   =   Dz U                     *
 *              dz          j                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

Tri_Dinfo *Tri_addD(Tri *U)
{
  const  int qa = U->qa, qb = U->qb;
  double *za,*zb,*w;
  Tri_Dinfo *Tri_D = (Tri_Dinfo *)calloc(1,sizeof(Tri_Dinfo));

  Tri_D->qa  = U->qa;

  Tri_D->Da  = dmatrix(0,qa-1,0,qa-1);
  Tri_D->Dat = dmatrix(0,qa-1,0,qa-1);
  Tri_D->Db  = dmatrix(0,qb-1,0,qb-1);
  Tri_D->Dbt = dmatrix(0,qb-1,0,qb-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'b');

  dgll(Tri_D->Da,Tri_D->Dat,za,qa);
  if(LZero)
    dgrj(Tri_D->Db,Tri_D->Dbt,zb,qb,0.0,0.0);
  else
    dgrj(Tri_D->Db,Tri_D->Dbt,zb,qb,1.0,0.0);

  return Tri_D;
}



/*--------------------------------------------------------------------------*
 * Dz is the first order differential matrix operating on the qt quadrature *
 * points. So that the product of D and a vector U(z_q) gives the partial  *
 * of U with respect to z. i.e.:                                            *
 *                                                                          *
 *              du                                                          *
 *              -- [i]  =  sum  Dz[i,j] U[j]   =   Dz U                     *
 *              dz          j                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

Quad_Dinfo *Quad_addD(Quad *U)
{
  const  int qa = U->qa, qb = U->qb;
  double *za,*zb,*w;
  Quad_Dinfo *Quad_D = (Quad_Dinfo *)malloc(sizeof(Quad_Dinfo));

  Quad_D->qa  = U->qa;

  Quad_D->Da  = dmatrix(0,qa-1,0,qa-1);
  Quad_D->Dat = dmatrix(0,qa-1,0,qa-1);
  Quad_D->Db  = dmatrix(0,qb-1,0,qb-1);
  Quad_D->Dbt = dmatrix(0,qb-1,0,qb-1);

  getzw(qa,&za,&w,'a');
  // b->a
  getzw(qb,&zb,&w,'a');

  dgll(Quad_D->Da,Quad_D->Dat,za,qa);
  dgll(Quad_D->Db,Quad_D->Dbt,zb,qb);

  return Quad_D;
}



/*--------------------------------------------------------------------------*
 * Dz is the first order differential matrix operating on the qt quadrature *
 * points. So that the product of D and a vector U(z_q) gives the partial  *
 * of U with respect to z. i.e.:                                            *
 *                                                                          *
 *              du                                                          *
 *              -- [i]  =  sum  Dz[i,j] U[j]   =   Dz U                     *
 *              dz          j                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

Tet_Dinfo *Tet_addD(Tet *U)
{
  const  int qa = U->qa, qb = U->qb, qc = U->qc;
  double *za,*zb,*zc,*w;
  Tet_Dinfo *Tet_D = (Tet_Dinfo *)malloc(sizeof(Tet_Dinfo));

  Tet_D->qa  = U->qa;

  Tet_D->Da  = dmatrix(0,qa-1,0,qa-1);
  Tet_D->Dat = dmatrix(0,qa-1,0,qa-1);
  Tet_D->Db  = dmatrix(0,qb-1,0,qb-1);
  Tet_D->Dbt = dmatrix(0,qb-1,0,qb-1);
  Tet_D->Dc  = dmatrix(0,qc-1,0,qc-1);
  Tet_D->Dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'b');
  getzw(qc,&zc,&w,'c');

  dgll(Tet_D->Da,Tet_D->Dat,za,qa);

  if(LZero){
    dgrj(Tet_D->Db,Tet_D->Dbt,zb,qb,0.0,0.0);
    dgrj(Tet_D->Dc,Tet_D->Dct,zc,qc,0.0,0.0);
  }
  else{
    dgrj(Tet_D->Db,Tet_D->Dbt,zb,qb,1.0,0.0);
    dgrj(Tet_D->Dc,Tet_D->Dct,zc,qc,2.0,0.0);
  }

  return Tet_D;
}



/*--------------------------------------------------------------------------*
 * Dz is the first order differential matrix operating on the qt quadrature *
 * points. So that the product of D and a vector U(z_q) gives the partial  *
 * of U with respect to z. i.e.:                                            *
 *                                                                          *
 *              du                                                          *
 *              -- [i]  =  sum  Dz[i,j] U[j]   =   Dz U                     *
 *              dz          j                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

Pyr_Dinfo *Pyr_addD(Pyr *U)
{
  const  int qa = U->qa, qb = U->qb, qc = U->qc;
  double *za,*zb,*zc,*w;
  Pyr_Dinfo *Pyr_D = (Pyr_Dinfo *)malloc(sizeof(Pyr_Dinfo));

  Pyr_D->qa  = U->qa;

  Pyr_D->Da  = dmatrix(0,qa-1,0,qa-1);
  Pyr_D->Dat = dmatrix(0,qa-1,0,qa-1);
  Pyr_D->Db  = dmatrix(0,qb-1,0,qb-1);
  Pyr_D->Dbt = dmatrix(0,qb-1,0,qb-1);
  Pyr_D->Dc  = dmatrix(0,qc-1,0,qc-1);
  Pyr_D->Dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'a');
  getzw(qc,&zc,&w,'c'); // changed

  dgll(Pyr_D->Da,Pyr_D->Dat,za,qa);
  dgll(Pyr_D->Db,Pyr_D->Dbt,zb,qb);
  dgrj(Pyr_D->Dc,Pyr_D->Dct,zc,qc,2.,0.);  // changed

  return Pyr_D;
}



/*--------------------------------------------------------------------------*
 * Dz is the first order differential matrix operating on the qt quadrature *
 * points. So that the product of D and a vector U(z_q) gives the partial  *
 * of U with respect to z. i.e.:                                            *
 *                                                                          *
 *              du                                                          *
 *              -- [i]  =  sum  Dz[i,j] U[j]   =   Dz U                     *
 *              dz          j                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

Prism_Dinfo *Prism_addD(Prism *U)
{
  const  int qa = U->qa, qb = U->qb, qc = U->qc;
  double *za,*zb,*zc,*w;
  Prism_Dinfo *Prism_D = (Prism_Dinfo *)calloc(1,sizeof(Prism_Dinfo));

  Prism_D->qa  = U->qa;

  Prism_D->Da  = dmatrix(0,qa-1,0,qa-1);
  Prism_D->Dat = dmatrix(0,qa-1,0,qa-1);
  Prism_D->Db  = dmatrix(0,qb-1,0,qb-1);
  Prism_D->Dbt = dmatrix(0,qb-1,0,qb-1);
  Prism_D->Dc  = dmatrix(0,qc-1,0,qc-1);
  Prism_D->Dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'a');
  getzw(qc,&zc,&w,'b');

  dgll(Prism_D->Da,Prism_D->Dat,za,qa);
  if(LZero){
    dgll(Prism_D->Db,Prism_D->Dbt,zb,qb);
    dgrj(Prism_D->Dc,Prism_D->Dct,zc,qc,0.,0.);
  }
  else{
    dgll(Prism_D->Db,Prism_D->Dbt,zb,qb);
    dgrj(Prism_D->Dc,Prism_D->Dct,zc,qc,1.,0.);
  }

  return Prism_D;
}




/*--------------------------------------------------------------------------*
 * Dz is the first order differential matrix operating on the qt quadrature *
 * points. So that the product of D and a vector U(z_q) gives the partial  *
 * of U with respect to z. i.e.:                                            *
 *                                                                          *
 *              du                                                          *
 *              -- [i]  =  sum  Dz[i,j] U[j]   =   Dz U                     *
 *              dz          j                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

Hex_Dinfo *Hex_addD(Hex *U)
{
  const  int qa = U->qa, qb = U->qb, qc = U->qc;
  double *za,*zb,*zc,*w;
  Hex_Dinfo *Hex_D = (Hex_Dinfo *)malloc(sizeof(Hex_Dinfo));

  Hex_D->qa  = U->qa;

  Hex_D->Da  = dmatrix(0,qa-1,0,qa-1);
  Hex_D->Dat = dmatrix(0,qa-1,0,qa-1);
  Hex_D->Db  = dmatrix(0,qb-1,0,qb-1);
  Hex_D->Dbt = dmatrix(0,qb-1,0,qb-1);
  Hex_D->Dc  = dmatrix(0,qc-1,0,qc-1);
  Hex_D->Dct = dmatrix(0,qc-1,0,qc-1);

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'a');
  getzw(qc,&zc,&w,'a');

  dgll(Hex_D->Da,Hex_D->Dat,za,qa);
  dgll(Hex_D->Db,Hex_D->Dbt,zb,qb);
  dgll(Hex_D->Dc,Hex_D->Dct,zc,qc);

  return Hex_D;
}



/*

Function name: Element::WeakDiff

Function Purpose:

Argument 1: Mode *m
Purpose:

Argument 2: double *ux
Purpose:

Argument 3: double *uy
Purpose:

Argument 4: double *
Purpose:

Argument 5: int con
Purpose:

Function Notes:

*/

void Tri::WeakDiff(Mode *m, double *ux, double *uy, double *, int con){
  register int i;

  double  a = -dparam("WAVEX"), b = -dparam("WAVEY");

  /* fill mode */
  for(i = 0; i < qb; ++i)
    dcopy(qa,m->a,1,h[i],1);
  for(i = 0; i < qa; ++i)
    dvmul(qb,m->b,1,*h+i,qa,*h+i,qa);

  if(con) dvneg(qa*qb,*h,1,*h,1);

  if(!option("WaveSpeed")){
    Grad_d(ux,uy,(double*) 0,'a');

    dsmul(qa*qb,a,ux,1,*h,1);
    daxpy(qa*qb,b,uy,1,*h,1);
  }
  else{
    double *uX = dvector(0,qa*qb-1);
    double *uY = dvector(0,qa*qb-1);

    Grad_d(uX,uY,(double*) 0,'a');

    dvmul (qa*qb,ux,1,uX,1,*h,1);
    dvvtvp(qa*qb,uy,1,uY,1,*h,1,*h,1);

    free(uX);  free(uY);
  }

  Iprod(this);

  for(i = 0; i < Nedges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2,-1.0,edge[i].hj+1,2);

}




void Quad::WeakDiff(Mode *m, double *ux, double *uy,double *, int con){
  register int i;
  double  a = -dparam("WAVEX"), b = -dparam("WAVEY");

  /* fill mode */
  for(i = 0; i < qb; ++i)
    dcopy(qa,m->a,1,h[i],1);
  for(i = 0; i < qa; ++i)
    dvmul(qb,m->b,1,*h+i,qa,*h+i,qa);

  if(con) dvneg(qa*qb,*h,1,*h,1);

  if(!option("WaveSpeed")){
    Grad_d(ux,uy,(double*) 0,'a');

    dsmul(qa*qb,a,ux,1,*h,1);
    daxpy(qa*qb,b,uy,1,*h,1);
  }
  else{
    double *uX = dvector(0,qa*qb-1);
    double *uY = dvector(0,qa*qb-1);

    Grad_d(uX,uY,(double*) 0,'a');

    dvmul (qa*qb,ux,1,uX,1,*h,1);
    dvvtvp(qa*qb,uy,1,uY,1,*h,1,*h,1);

    free(uX);  free(uY);
  }

  Iprod(this);

  for(i = 0; i < Nedges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2,-1.0,edge[i].hj+1,2);
}




void Tet::WeakDiff(Mode *m, double *ux, double *uy, double *, int con){
  register int i;

  double  a = -dparam("WAVEX"), b = -dparam("WAVEY");

  /* fill mode */
  for(i = 0; i < qb; ++i)
    dcopy(qa,m->a,1,h[i],1);
  for(i = 0; i < qa; ++i)
    dvmul(qb,m->b,1,*h+i,qa,*h+i,qa);

  if(con) dvneg(qa*qb,*h,1,*h,1);

  if(!option("WaveSpeed")){
    Grad_d(ux,uy,(double*) 0,'a');

    dsmul(qa*qb,a,ux,1,*h,1);
    daxpy(qa*qb,b,uy,1,*h,1);
  }
  else{
    double *uX = dvector(0,qa*qb-1);
    double *uY = dvector(0,qa*qb-1);

    Grad_d(uX,uY,(double*) 0,'a');

    dvmul (qa*qb,ux,1,uX,1,*h,1);
    dvvtvp(qa*qb,uy,1,uY,1,*h,1,*h,1);

    free(uX);  free(uY);
  }

  Iprod(this);

  for(i = 0; i < Nedges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2,-1.0,edge[i].hj+1,2);

}




void Pyr::WeakDiff(Mode *, double *, double *, double *, int ){

}




void Prism::WeakDiff(Mode *, double *, double *, double *, int ){

}




void Hex::WeakDiff(Mode *m, double *ux, double *uy, double *, int con){
  register int i;

  double  a = -dparam("WAVEX"), b = -dparam("WAVEY");

  /* fill mode */
  for(i = 0; i < qb; ++i)
    dcopy(qa,m->a,1,h[i],1);
  for(i = 0; i < qa; ++i)
    dvmul(qb,m->b,1,*h+i,qa,*h+i,qa);

  if(con) dvneg(qa*qb,*h,1,*h,1);

  if(!option("WaveSpeed")){
    Grad_d(ux,uy,(double*) 0,'a');

    dsmul(qa*qb,a,ux,1,*h,1);
    daxpy(qa*qb,b,uy,1,*h,1);
  }
  else{
    double *uX = dvector(0,qa*qb-1);
    double *uY = dvector(0,qa*qb-1);

    Grad_d(uX,uY,(double*) 0,'a');

    dvmul (qa*qb,ux,1,uX,1,*h,1);
    dvvtvp(qa*qb,uy,1,uY,1,*h,1,*h,1);

    free(uX);  free(uY);
  }

  Iprod(this);

  for(i = 0; i < NHex_edges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2,-1.0,edge[i].hj+1,2);

}




void Element::WeakDiff(Mode *, double *, double *,double *, int ){ERR;}




/*

Function name: Element::fill_gradbase

Function Purpose:

Argument 1: Mode *gb
Purpose:

Argument 2: Mode *m
Purpose:

Argument 3: Mode *mb
Purpose:

Argument 4: Mode *fac
Purpose:

Function Notes:

*/

void Tri::fill_gradbase(Mode *gb, Mode *m, Mode *mb, Mode *fac){

  /* d/dr term */
  dcopy(qa,mb->a,1,gb[0].a,1);
  dvdiv(qb,m->b,1,fac->b,1,gb[0].b,1);

  /* extra d/ds term in d/ds  */
  dcopy(qa,m ->a,1,gb[1].a,1);
  dcopy(qb,mb->b,1,gb[1].b,1);
}




void Quad::fill_gradbase(Mode *gb, Mode *m, Mode *mb, Mode *fac){

  /* d/dr term */
  dcopy(qa,mb->a,1,gb[0].a,1);
  dvdiv(qb,m->b,1,fac->b,1,gb[0].b,1);

  /* extra d/ds term in d/ds  */
  dcopy(qa,m ->a,1,gb[1].a,1);
  dcopy(qb,mb->b,1,gb[1].b,1);
}




void Tet::fill_gradbase(Mode *gb, Mode *m, Mode *mb, Mode *fac){

  dvdiv(qc,m->c,1,fac->c,1,gb[0].c,1);

  /* d/dr term */
  dcopy(qa,mb->a,1,gb[0].a,1);
  dvdiv(qb,m->b,1,fac->b,1,gb[0].b,1);

  /* extra d/ds term in d/ds  */
  dcopy(qa,m ->a,1,gb[1].a,1);
  dcopy(qb,mb->b,1,gb[1].b,1);
  dvdiv(qc,m->c,1,fac->c,1,gb[2].c,1);

  /* extra d/dt term in d/dt  */
  dcopy(qa,m-> a,1,gb[2].a,1);
  dcopy(qb,m-> b,1,gb[2].b,1);
  dcopy(qc,mb->c,1,gb[2].c,1);

}




void Pyr::fill_gradbase(Mode *, Mode *, Mode *, Mode *){

  fprintf(stderr,"Pyr::fill_gradbase not implemented\n");
}




void Prism::fill_gradbase(Mode *, Mode *, Mode *, Mode *){

  fprintf(stderr,"Prism::fill_gradbase not implemented\n");
}




void Hex::fill_gradbase(Mode *gb, Mode *m, Mode *mb, Mode *fac){

  fprintf(stderr,"Hex::fill_gradbase not implemented\n");

}




void Element::fill_gradbase(Mode *, Mode *, Mode *, Mode *){ERR;}
