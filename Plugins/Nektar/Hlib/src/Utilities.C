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

#include <stdio.h>

using namespace polylib;

/* local functions */
static void set_normals (Element_List *E);
static int X2A         (Element *E, Coord *X, Coord *A);
static int GetRS       (Element *U, Coord *X, Coord *A);
static int GetAB (Element *U, Coord *X, Coord *A);
static int  find_eid    (Element_List *E, Coord *X);
static int EID=0;
static int LAST_EID=0;

int Query_element_id(){
  return LAST_EID;
}

/* Interpolate the nfields fields stored in U to points described by X
   and return values in ui. */

void Interp_point(Element_List **U, int nfields, Coord *X, double *ui){
  register int i,k,n;
  int      eid, qa, qb;
  Coord    A;
  double   *ha,*hb,*z,*w, *tmp;
  Element  *E;

  // needs to be fixed for 3d
  A.x = dvector(0,U[0]->fhead->dim()-1);
  A.y = A.x+1;
  A.x[0] = 0.;
  A.y[0] = 0.;

  set_normals(*U);

  /* find element that contains points */
  eid = find_eid(*U,X);
  if(eid == -1){dzero(nfields,ui,1); LAST_EID=-1;return;}

  E = U[0]->flist[eid];

  if(!X2A(E,X,&A)){
    for(k=0;k<U[0]->nel;++k){
      E = U[0]->flist[k];
      if(X2A(E,X,&A)){
  eid = E->id;
  break;
      }
    }
    if(k == U[0]->nel)
      {dzero(nfields,ui,1); fprintf(stderr, "#");return;}
  }

  E = U[0]->flist[eid];

  LAST_EID = E->id;

  qa = E->qa;
  qb = E->qb;

  ha = dvector(0,qa-1);
  getzw(qa,&z,&w,'a');
  for(i = 0; i < qa; ++i)
    ha[i] = hgll(i,A.x[0],z,qa);


  hb = dvector(0,qb-1);
  if(E->identify() == Nek_Tri || E->identify() == Nek_Nodal_Tri){
    getzw(qb,&z,&w,'b');
    if(LZero) // use Legendre weights
      for(i = 0; i < qb; ++i)
  hb[i] = hgrj(i,A.x[1],z,qb,0.0,0.0);
    else
      for(i = 0; i < qb; ++i)
  hb[i] = hgrj(i,A.x[1],z,qb,1.0,0.0);
  }
  else{
    getzw(qb,&z,&w,'a');
    for(i = 0; i < qb; ++i)
      hb[i] = hgll(i,A.x[1],z,qb);
  }

  tmp = dvector(0,qb-1);
  /* Interpolate to point using lagrange interpolation in the physical space */
  for(n = 0; n < nfields; ++n){
    E = U[n]->flist[eid];
    if(E->state == 't'){
      E->Trans(E,J_to_Q);
    }

    /* interpolation in the `a' direction */
    for(i = 0; i < qb; ++i)
      tmp[i] = ddot(qa,ha,1,E->h[i],1);

    /* interpolation in the `b' direction */
    ui[n] = ddot(qb,hb,1,tmp,1);
  }

  free(ha); free(hb); free(tmp); free(A.x);
}

typedef struct nmls{
  double n[4][2];
} Nmls;

static Nmls *Nmals = (Nmls*)0;

/* function to set up inward normals (not normalised) for straight
   sided elements. Needs to be set up for most of these functions  */

static void set_normals(Element_List *U){
  if(!Nmals){
    register int i,k;
    const    int nel = U->nel;
    Element  *E;

    Nmals = (Nmls *)malloc(nel*sizeof(Nmls));

    for(k = 0; k < nel; ++k){
      E = U->flist[k];

      for(i=0;i<E->Nedges;++i){
  Nmals[k].n[i][0] = -(E->vert[(i+1)%E->Nverts].y - E->vert[i].y);
  Nmals[k].n[i][1] =   E->vert[(i+1)%E->Nverts].x - E->vert[i].x;
      }
    }
  }
}

void free_normals(void){
  free(Nmals);
}

/* Given a points (x,y) stored in X->x[0] Y->y[0] find the points
   (a.b) and return in A->x[0] A->y[0]. The derivation of this algorithm
   involves describing the point in vector as:

   X - X1 = 2(1+r)/2 (X2 - X1) + 2(1+s)/2 (X3 - X1)

   where X1,X2 and X3 are the (x,y) points of the vertices. Then r and
   s are evaluated by taking the inner product of this equation with the
   normals to (X2-x1) and (X3-X1). A similar approach can be used in 3D
   except you need to use the normals of the faces.

   Note: this function assumes that the element is straight sides
   (Curved sided has no explicit formulae) and that the normals have
   been set up already by either calling `set_normals' or `get_eid'. */

static int X2A(Element *E, Coord *X, Coord *A){
  if(E->identify() == Nek_Quad)
    return GetRS(E, X, A);

  /*
  if(E->curve)
    return GetAB(E,X,A);
    */

  const  int k = E->id;
  double r,s;
  double x = X->x[0], y = X->y[0];
  double x1 = E->vert[0].x, y1 = E->vert[0].y;
  double x2 = E->vert[1].x, y2 = E->vert[1].y;
  double x3 = E->vert[2].x, y3 = E->vert[2].y;

  /* first calculate r and s */

  r  = 2*((x  - x1)*Nmals[k].n[2][0] + (y  - y1)*Nmals[k].n[2][1]);
  r /=    (x2 - x1)*Nmals[k].n[2][0] + (y2 - y1)*Nmals[k].n[2][1];
  r -= 1.0;

  s  = 2*((x  - x1)*Nmals[k].n[0][0] + (y  - y1)*Nmals[k].n[0][1]);
  s /=    (x3 - x1)*Nmals[k].n[0][0] + (y3 - y1)*Nmals[k].n[0][1];
  s -= 1.0;


  A->x[0] = (1.0-s)? 2*(1.0+r)/(1.0-s) - 1.0: 1.0;
  A->y[0] = s;

  if(A->x[0] > 1. || A->x[0] < -1. || A->y[0] > 1. || A->y[0] < -1.)
    return 0;

  A->x[0] = clamp(A->x[0], -1., 1.);
  A->y[0] = clamp(A->y[0], -1., 1.);

  return 1;

}


#if 1
/* ---------------------------------------------------------------------- *
 * GetRS() -- Locate an (x,y)-point within an element                     *
 *                                                                        *
 * This function executes a Newton iteration to compute the local (r,s)-  *
 * coordinates of a given point (x,y) within an element.                  *
 *                                                                        *
 * Return value: 0 if the input is invalid                                *
 *               n if converged after n-iterations                        *
 *              -n if diverged after n-iterations                         *
 * ---------------------------------------------------------------------- */

                             /* ............ Tolerances ............ */
#define MAXIT    101         /* maximum iterations for convergence   */
#define EPSILON  1.e-9       /* |x-xp| < EPSILON  error tolerance    */
#define GETRSTOL 1.001       /* |r,s|  < GETRSTOL inside an element? */
#define GETRSDIV 1.5         /* |r,s|  > GETRSDIV stop the search    */

//int GetRS (Element *U, double x, double y, double *rg, double *sg)

int GetRS (Element *U, Coord *X, Coord *A)
{
  double  *za, *zb, *ha, *hb, *hr, *hs,
           drdx, drdy, dsdx, dsdy, dx, dy, xp, yp, rr, ss;
  double   rg, sg, x, y;
  int      ok = 0,
           ic = 0;
  register int i;

  rg = A->x[0];    sg = A->y[0];
  x  = X->x[0];    y  = X->y[0];

  if (!U) return 0;

  const  int  qa = U->qa, qb = U->qb;

  dx = 1.;
  dy = 1.;

  getzw (qa, &za, &ha, 'a');   /* Get GLL points */
  getzw (qb, &zb, &hb, 'a');

  hr = dvector (0, QGmax);
  hs = dvector (0, QGmax);

  rr = clamp (rg, -1., 1.);   /* Use the given values as a guess if they */
  ss = clamp (sg, -1., 1.);   /* are valid, otherwise clamp them.        */

  double **gridx = dmatrix(0, qb-1, 0, qa-1);
  double **gridy = dmatrix(0, qb-1, 0, qa-1);
  Coord gX;

  gX.x = gridx[0];
  gX.y = gridy[0];
  U->coord(&gX);

  /* .... Start the iteration .... */

  while (!ok && ic++ <= MAXIT) {

    xp   = 0.;        yp   = 0.;
    drdx = 0.;        drdy = 0.;
    dsdx = 0.;        dsdy = 0.;

    for (i = 0; i < qa; i++)
      hr[i] = hgll(i, rr, za, qa);

    for (i = 0; i < qb; i++)
      hs[i] = hgll(i, ss, zb, qb);

    for (i = 0; i < qb; i++) {
      xp   += hs[i] * ddot (qa, hr, 1, gridx[i], 1);
      yp   += hs[i] * ddot (qa, hr, 1, gridy[i], 1);

      drdx += hs[i] * ddot (qa, hr, 1, U->geom->rx.p + i*qa, 1);
      dsdy += hs[i] * ddot (qa, hr, 1, U->geom->sy.p + i*qa, 1);

      drdy += hs[i] * ddot (qa, hr, 1, U->geom->ry.p + i*qa, 1);
      dsdx += hs[i] * ddot (qa, hr, 1, U->geom->sx.p + i*qa, 1);
    }

    dx    = x - xp;                   /* Distance from the point (x,y) */
    dy    = y - yp;

    rr   += (drdx * dx + drdy * dy);  /* Corrections to the guess */
    ss   += (dsdx * dx + dsdy * dy);

    /* Convergence test */

    if (sqrt(dx*dx + dy*dy) < EPSILON)
  ok =  ic;
    if (fabs(rr) > GETRSDIV || fabs(ss) > GETRSDIV)
      ok = -ic;
  }

  free (hr);     free (hs);
  free_dmatrix (gridx,0,0);  free_dmatrix (gridy,0,0);

  if(ic > MAXIT){
    //    fprintf(stderr, "GetAB: did not converge\n");
    return 0;
  }
  if (fabs(rr) > GETRSTOL || fabs(ss) > GETRSTOL )
    return 0;

  A->x[0] = rr;
  A->y[0] = ss;

  return 1;
}

int GetAB (Element *U, Coord *X, Coord *A)
{
  double  *za, *zb, *ha, *hb, *hr, *hs,
           drdx, drdy, dsdx, dsdy, dx, dy, xp, yp, rr, ss, aa, bb;

  double   rg, sg, x, y;
  int      ok = 0,
           ic = 0;
  register int i;

  rg = A->x[0];    sg = A->y[0];
  x  = X->x[0];    y  = X->y[0];

  if (!U) return 0;

  const  int  qa = U->qa, qb = U->qb;

  dx = 1.;
  dy = 1.;

  getzw (qa, &za, &ha, 'a');   /* Get GLL points */
  getzw (qb, &zb, &hb, 'b');   /* Get GRL points */

  hr = dvector (0, QGmax);
  hs = dvector (0, QGmax);

  rr = clamp (rg, -1., 1.);   /* Use the given values as a guess if they */
  ss = clamp (sg, -1., 1.);   /* are valid, otherwise clamp them.        */

  double *gridx = dvector(0, qa*qb-1);
  double *gridy = dvector(0, qa*qb-1);
  Coord gX;

  gX.x = gridx;
  gX.y = gridy;
  U->coord(&gX);

  /* .... Start the iteration .... */

  while (!ok && ic++ <= MAXIT) {

    xp   = 0.;        yp   = 0.;
    drdx = 0.;        drdy = 0.;
    dsdx = 0.;        dsdy = 0.;

    aa = (rr+1.)*2./(1.-ss) -1.;
    bb = ss;

    for (i = 0; i < qa; i++)
      hr[i] = hgll(i, aa, za, qa);

    for (i = 0; i < qb; i++)
    if(LZero) // use Legendre weights
      hs[i] = hgrj(i, bb, zb, qb, 0., 0.);
    else
      hs[i] = hgrj(i, bb, zb, qb, 1., 0.);

    for (i = 0; i < qb; i++) {
      xp   += hs[i] * ddot (qa, hr, 1, gridx+i*qa, 1);
      yp   += hs[i] * ddot (qa, hr, 1, gridy+i*qa, 1);

      drdx += hs[i] * ddot (qa, hr, 1, U->geom->rx.p + i*qa, 1);
      drdy += hs[i] * ddot (qa, hr, 1, U->geom->ry.p + i*qa, 1);
      dsdx += hs[i] * ddot (qa, hr, 1, U->geom->sx.p + i*qa, 1);
      dsdy += hs[i] * ddot (qa, hr, 1, U->geom->sy.p + i*qa, 1);
    }

    dx    = x - xp;                   /* Distance from the point (x,y) */
    dy    = y - yp;

    rr   += (drdx * dx + drdy * dy);  /* Corrections to the guess */
    ss   += (dsdx * dx + dsdy * dy);

    /* Convergence test */

    if (sqrt(dx*dx + dy*dy) < EPSILON)
  ok =  ic;
    if (fabs(rr) > GETRSDIV || fabs(ss) > GETRSDIV)
      ok = -ic;
  }

  if(ic > MAXIT){
    //    fprintf(stderr, "GetAB: did not converge\n");
    free (hr);  free (hs);
    free (gridx);  free(gridy);
    return 0;
  }
  if (fabs(rr) > GETRSTOL || fabs(ss) > GETRSTOL) ok = -ic;

  A->x[0] = 2.*(rr+1.)/(1.-ss)-1.;
  A->y[0] = ss;

  if(A->x[0] > 1. || A->x[0] < -1. ||
     A->y[0] > 1. || A->y[0] < -1. ||
     ok < 0 || ok > 0){
    free (hr);  free (hs);
    free (gridx);  free(gridy);
    return 0;
  }

  free (hr);  free (hs);
  free (gridx);  free(gridy);
  return 1;
}


#endif


/* Find and return the element id of which contains the points (x,y,z)
   stored in (X->x[0],X->y[0],Z->z[0]). This is achieved by checking the
   sign of the inner product between the inner normal and a vector from
   the point to a positionn on the edge. */



static int find_eid(Element_List *E, Coord *X){
  register int k,j,cnt;
  Element *F;

  set_normals(E);

  for(k = EID; k < E->nel; ++k){
    F = E->flist[k];
    for(cnt = 0, j = 0; j < F->Nedges; ++j){
      if(((X->x[0] - F->vert[j].x)*Nmals[k].n[j][0] +
    (X->y[0] - F->vert[j].y)*Nmals[k].n[j][1]) >= 0) ++cnt;
    }
    if(cnt == F->Nedges){EID = k;  return k;}
  }

  for(k = 0; k < EID; ++k){
    F = E->flist[k];
    for(cnt = 0, j = 0; j < F->Nedges; ++j){
      if(((X->x[0] - F->vert[j].x)*Nmals[k].n[j][0] +
    (X->y[0] - F->vert[j].y)*Nmals[k].n[j][1]) >= 0) ++cnt;
    }
    if(cnt == F->Nedges){EID = k;  return k;}
  }

  return -1;
}


double interp_abc(double *vec, int qa, char type, double a){
  int i;
  double *z, *w, res;

  getzw(qa,&z,&w,type);
  for(res=0.0, i = 0; i < qa; ++i)
    res += vec[i]*hgll(i,a,z,qa);

  return res;
}

// 2d only
void calc_edge_centers(Element *E, Coord *X){
  int i;
  if(!E->curve || E->curve->face == -1)
    for(i=0;i<E->Nedges;++i){
      X->x[i] = 0.5*(E->vert[i].x +E->vert[(i+1)%E->Nedges].x);
      X->y[i] = 0.5*(E->vert[i].y +E->vert[(i+1)%E->Nedges].y);
    }
  else{
    Coord cX;
    double *tmp = dvector(0, QGmax-1);
    cX.x = dvector(0, QGmax-1);
    cX.y = dvector(0, QGmax-1);
    for(i=0;i<E->Nedges;++i){
      E->GetFaceCoord(i, &cX);
      E->InterpToFace1(i, cX.x, tmp);
      X->x[i] = interp_abc(tmp, E->qa, 'a', 0.0);
      E->InterpToFace1(i, cX.y, tmp);
      X->y[i] = interp_abc(tmp, E->qa, 'a', 0.0);
    }
    free(cX.x);    free(cX.y);   free(tmp);
  }
}

void nomem_set_curved(Element_List *EL, Element *E){

  double *h  = dvector(0, QGmax*QGmax-1);
  double *hj = dvector(0, QGmax*QGmax-1);

  E->Mem_shift(h, hj, 'n');
  E->curvX = (Cmodes*)0;
  E->set_curved_elmt(EL);

  free(E->h);
  free(E->face[0].hj);
  free(h);  free(hj);
}

double eval_field_at_pt(int qa, int qb, int qc, double *field, double *hr, double *hs, double *ht){
  int i,j;
  double d=0.;

  for(j = 0; j < qc; ++j)
    for (i = 0; i < qb; i++)
      d += ht[j]*hs[i]*ddot(qa, hr, 1, field+i*qa+j*qa*qb, 1);
  return d;

}


int GetABC (Element *U, Coord *X, Coord *A)
{
  double  *za, *zb, *zc;
  double  *ha, *hb, *hc;
  double  *hr, *hs, *ht;
  double  drdx, drdy, drdz;
  double  dsdx, dsdy, dsdz;
  double  dtdx, dtdy, dtdz;

  double dx, dy, dz;
  double xp, yp, zp;
  double rr, ss, tt;
  double aa, bb, cc;

  double rg, sg, tg;
  double x, y, z;

  int      ok = 0,
           ic = 0;
  register int i;

  rg = A->x[0];    sg = A->y[0];    tg = A->z[0];
  x  = X->x[0];    y  = X->y[0];    z  = X->z[0];

  if (!U) return 0;

  const  int  qa = U->qa, qb = U->qb, qc = U->qc;

  dx = 1.;
  dy = 1.;
  dz = 1.;

  // do for hex right now
  getzw (qa, &za, &ha, 'a');   /* Get GLL points */
  getzw (qb, &zb, &hb, 'a');   /* Get GRL points */
  getzw (qc, &zc, &hc, 'a');   /* Get GRL points */

  hr = dvector (0, QGmax);
  hs = dvector (0, QGmax);
  ht = dvector (0, QGmax);

  rr = clamp (rg, -1., 1.);   /* Use the given values as a guess if they */
  ss = clamp (sg, -1., 1.);   /* are valid, otherwise clamp them.        */
  tt = clamp (tg, -1., 1.);   /* are valid, otherwise clamp them.        */

  double *gridx = dvector(0, qa*qb*qc-1);
  double *gridy = dvector(0, qa*qb*qc-1);
  double *gridz = dvector(0, qa*qb*qc-1);
  Coord gX;

  gX.x = gridx;
  gX.y = gridy;
  gX.z = gridz;
  U->coord(&gX);

  /* .... Start the iteration .... */

  while (!ok && ic++ <= MAXIT) {

    xp   = 0.;        yp   = 0.;         zp   = 0.;
    drdx = 0.;        drdy = 0.;         drdz = 0.;
    dsdx = 0.;        dsdy = 0.;         dsdz = 0.;
    dtdx = 0.;        dtdy = 0.;         dtdz = 0.;

    aa = rr;
    bb = ss;
    cc = tt;

    for (i = 0; i < qa; i++)
      hr[i] = hgll(i, aa, za, qa);

    for (i = 0; i < qb; i++)
      hs[i] = hgll(i, bb, zb, qb);

    for (i = 0; i < qc; i++)
      ht[i] = hgll(i, cc, zc, qc);

    xp   = eval_field_at_pt(qa,qb,qc, gridx, hr, hs, ht);
    yp   = eval_field_at_pt(qa,qb,qc, gridy, hr, hs, ht);
    zp   = eval_field_at_pt(qa,qb,qc, gridz, hr, hs, ht);

    drdx = eval_field_at_pt(qa,qb,qc, U->geom->rx.p, hr, hs, ht);
    drdy = eval_field_at_pt(qa,qb,qc, U->geom->ry.p, hr, hs, ht);
    drdz = eval_field_at_pt(qa,qb,qc, U->geom->rz.p, hr, hs, ht);

    dsdx = eval_field_at_pt(qa,qb,qc, U->geom->sx.p, hr, hs, ht);
    dsdy = eval_field_at_pt(qa,qb,qc, U->geom->sy.p, hr, hs, ht);
    dsdz = eval_field_at_pt(qa,qb,qc, U->geom->sz.p, hr, hs, ht);

    dtdx = eval_field_at_pt(qa,qb,qc, U->geom->tx.p, hr, hs, ht);
    dtdy = eval_field_at_pt(qa,qb,qc, U->geom->ty.p, hr, hs, ht);
    dtdz = eval_field_at_pt(qa,qb,qc, U->geom->tz.p, hr, hs, ht);

    dx    = x - xp;                   /* Distance from the point (x,y) */
    dy    = y - yp;
    dz    = z - zp;

    rr   += (drdx * dx + drdy * dy + drdz * dz);  /* Corrections to the guess */
    ss   += (dsdx * dx + dsdy * dy + dsdz * dz);
    tt   += (dtdx * dx + dtdy * dy + dtdz * dz);

    /* Convergence test */

    if (sqrt(dx*dx + dy*dy + dz*dz) < EPSILON)
  ok =  ic;
    if (fabs(rr) > GETRSDIV || fabs(ss) > GETRSDIV || fabs(tt) > GETRSDIV)
      ok = -ic;
  }

  free (hr);     free (hs);     free (ht);
  free (gridx);  free (gridy);  free (gridz);

  if(ic > MAXIT){
    //    fprintf(stderr, "GetAB: did not converge\n");
    return 0;
  }
  if (fabs(rr) > GETRSTOL || fabs(ss) > GETRSTOL || fabs(tt) > GETRSDIV )
    return -ic;

  A->x[0] = rr;
  A->y[0] = ss;
  A->z[0] = tt;

  return 1;
}

static int point_in_elmt(Element *E, Coord *X){
  // only fixed for reg hexes
  if(X->x[0] >= E->vert[0].x && X->x[0] <= E->vert[1].x &&
     X->y[0] >= E->vert[0].y && X->y[0] <= E->vert[3].y &&
     X->z[0] >= E->vert[0].z && X->z[0] <= E->vert[4].z)
    return 1;
  else
    return 0;
}



static int ELMT_ID=0;

int find_elmt(Element_List *EL, Coord *X){
  int k;

  for(k=ELMT_ID;k<EL->nel;++k)
    if(point_in_elmt(EL->flist[k], X)){
      ELMT_ID = k;
      return k;
    }
  for(k=0;k<ELMT_ID;++k)
    if(point_in_elmt(EL->flist[k], X)){
      ELMT_ID = k;
      return k;
    }
  return -1;
}





void Interp_point_3d(Element_List **U, int nfields, Coord *X, double *ui){
  register int i,k,n;
  int      eid, qa, qb, qc;
  Coord    A;
  double   *ha,*hb,*hc,*z,*w;
  Element  *E;

  // needs to be fixed for 3d
  A.x = dvector(0,U[0]->fhead->dim()-1);
  A.y = A.x+1;
  A.z = A.x+2;
  A.x[0] = 0.;
  A.y[0] = 0.;
  A.z[0] = 0.;

  /* find element that contains points */
  eid = find_elmt(*U,X);
  if(eid == -1){dzero(nfields,ui,1); return;}

  E = U[0]->flist[eid];
  if(!GetABC(E,X,&A)){
    for(k=0;k<U[0]->nel;++k){
      E = U[0]->flist[k];
      if(GetABC(E,X,&A))
  break;
    }
    if(k == U[0]->nel)
      {dzero(nfields,ui,1); fprintf(stderr, "#");return;}
  }

  qa = E->qa;
  qb = E->qb;
  qc = E->qc;

  ha = dvector(0,qa-1);
  getzw(qa,&z,&w,'a');
  for(i = 0; i < qa; ++i)
    ha[i] = hgll(i,A.x[0],z,qa);

  hb = dvector(0,qb-1);
  getzw(qb,&z,&w,'a');
  for(i = 0; i < qb; ++i)
    hb[i] = hgll(i,A.x[1],z,qb);

  hc = dvector(0,qc-1);
  getzw(qc,&z,&w,'a');
  for(i = 0; i < qc; ++i)
    hc[i] = hgll(i,A.x[2],z,qc);

  /* Interpolate to point using lagrange interpolation in the physical space */
  for(n = 0; n < nfields; ++n){
    E = U[n]->flist[eid];
    if(E->state == 't'){
      E->Trans(E,J_to_Q);
      E->state = 'p';
    }

    ui[n] = eval_field_at_pt(qa,qb,qc, E->h_3d[0][0], ha, hb, hc);
  }

  free(ha); free(hb); free(hc); free(A.x);
}


void cross_products(Coord *A, Coord *B, Coord *C, int nq){
  int i;
  for(i=0;i<nq;++i){
    C->x[i] = A->y[i]*B->z[i] - A->z[i]*B->y[i];
    C->y[i] = A->z[i]*B->x[i] - A->x[i]*B->z[i];
    C->z[i] = A->x[i]*B->y[i] - A->y[i]*B->x[i];
  }
}

void normalise(Coord *A, int nq){
  int i;
  double fac;
  for(i=0;i<nq;++i){
    fac = sqrt(A->x[i]*A->x[i] + A->y[i]*A->y[i] + A->z[i]*A->z[i]);
    A->x[i] /= fac;
    A->y[i] /= fac;
    A->z[i] /= fac;
  }
}


double *Interp2d_wk = (double*)0;

void  Interp2d(double *ima, double *imb,  double *from, int qa, int qb, double *to, int nqa, int nqb){

  if(!Interp2d_wk)
    Interp2d_wk = dvector(0, max(qa,max(nqa,QGmax))*max(qa,max(nqa,QGmax))-1);

  dgemm('n','n', qa,nqb,qb,1.,from,qa,        imb,qb,0.,Interp2d_wk, qa);
  dgemm('t','n',nqa,nqb,qa,1., ima,qa,Interp2d_wk,qa,0.,         to,nqa);
}



/* given a list of coefficients stored in uj of order qa*qb at 2d surface
   quadrature points in 'a' & 'b' this function evaluates the function
   at a set of rotationally symmetic points based on the value of 'n'
   and puts them in *ui. */

int  Interp_symmpts(Element *E, int n, double *uj, double *ui, char storage){
  static int nstore,qstore1,qstore2,qstore3;
  static double **mat;
  static Nek_Facet_Type  NekTypeStore;
  int qa,qb,qc;
  int ntot, qtot;
  int dim = E->dim();
  double *utmp;

  qa = qb = qc = 0;

  switch(E->identify()){
  case Nek_Tri:
    ntot = n*(n+1)/2;
    qa = E->qa;
    qb = E->qb;
    qtot = qa*qb;
    break;
  case Nek_Quad:
    ntot = n*n;
    qa = E->qa;
    qb = E->qb;
    qtot = qa*qb;
    break;
  case Nek_Tet:
    ntot = n*(n+1)*(n+2)/6;
    qa = E->qa;
    qb = E->qb;
    qc = E->qc;
    qtot = qa*qb*qc;
    break;
  case Nek_Prism:
    ntot = n*n*(n+1)/2;
    qa = E->qa;
    qb = E->qb;
    qc = E->qc;
    qtot = qa*qb*qc;
    break;
  default:
    fprintf(stderr,"Element is not setup in Interp_symmpts\n");
    exit(-1);
    break;
  }

  /* generate transformation matrix */
  if(nstore != n || qstore1 != qa || qstore2 != qb || qstore3 != qc
     || E->identify() != NekTypeStore ){
    register int i,j,k,cnt,i1,j1,k1;
    double r[3],a[3];
    double *ha,*hb,*hc;
    int  *order;

    if(mat)
      free_dmatrix(mat,0,0);

    mat = dmatrix(0,ntot-1,0,qtot-1);

    order = ivector(0,ntot-1);

    switch(E->identify()){
    case Nek_Tri:
      ha = dvector(0,QGmax-1);
      hb = dvector(0,QGmax-1);
      if(storage == 'm'){ /* use modal storage to order output */
  cnt = 0;
  order[0] = cnt++;
  order[qa-1] = cnt++;
  order[qa*(qa+1)/2-1] = cnt++;
  for(i = 0; i < qa-2; ++i)
    order[i+1] = cnt++;
  for(i = 0,i1=2*qa-2; i < qa-2; ++i,i1+=qa-1-i)
    order[i1] = cnt++;
  for(i = 0,i1=qa; i < qa-2; ++i,i1+=qa-i)
    order[i1] = cnt++;
  for(i = 0,i1=qa+1; i < qa-3; ++i,i1+=qa-i)
    for(j = 0; j < qa-3-i; ++j)
      order[i1+j] = cnt++;
      }
      else /* use row  storage */
  for(i = 0; i < ntot; ++i)
    order[i] = i;


      for(j = 0,cnt = 0; j < n; ++j)
  for(i = 0; i < n-j; ++i,++cnt){

    r[0] = 2*i/(double)(n-1)-1;
    r[1] = 2*j/(double)(n-1)-1;

    if(j != n-1)
      a[0] = 2*(1+r[0])/(1-r[1])-1;
    else
      a[0] = -1.0;
    a[1] = r[1];

    get_point_shape_2d(E,a[0],a[1],ha,hb);
    for(k = 0; k < qb; ++k)
      dsmul(qa,hb[k],ha,1,mat[order[cnt]]+k*qa,1);
  }
      free(ha);
      free(hb);
      break;
    case Nek_Quad:
      ha = dvector(0,QGmax-1);
      hb = dvector(0,QGmax-1);

      if(storage == 'm'){
  fprintf(stderr,"Interp_symm not set up for modal storage in tets \n");
  exit(-1);
      }

      for(j = 0,cnt=0; j < n; ++j)
  for(i = 0; i < n; ++i,++cnt){

    r[0] = 2*i/(double)(n-1)-1;
    r[1] = 2*j/(double)(n-1)-1;

    a[0] = r[0];
    a[1] = r[1];

    get_point_shape_2d(E,a[0],a[1],ha,hb);

    for(k = 0; k < qb; ++k)
      dsmul(qa,hb[k],ha,1,mat[cnt]+k*qa,1);

  }
      free(ha);
      free(hb);
      break;
    case Nek_Prism:
      ha = dvector(0,QGmax-1);
      hb = dvector(0,QGmax-1);
      hc = dvector(0,QGmax-1);

      if(storage == 'm'){
  fprintf(stderr,"Interp_symm not set up for modal storage in tets \n");
  exit(-1);
      }

      for(k = 0,cnt = 0; k < n; ++k)
  for(j = 0; j < n; ++j)
    for(i = 0; i < n-k; ++i,++cnt){

      r[0] = 2*i/(double)(n-1)-1;
      r[1] = 2*j/(double)(n-1)-1;
      r[2] = 2*k/(double)(n-1)-1;

      if(k != n-1)
        a[0] = 2*(1+r[0])/(1-r[2])-1;
      else
        a[0] = -1.0;
      a[1] = r[1];
      a[2] = r[2];

      get_point_shape_3d(E,a[0],a[1],a[2],ha,hb,hc);
      for(j1 = 0,k1=0; j1 < qc; ++j1)
        for(i1 = 0; i1 < qb; ++i1,++k1)
    dsmul(qa,hc[j1]*hb[i1],ha,1,mat[cnt]+k1*qa,1);
    }
      free(ha);
      free(hb);
      free(hc);
      break;
    case Nek_Tet:
      ha = dvector(0,QGmax-1);
      hb = dvector(0,QGmax-1);
      hc = dvector(0,QGmax-1);

      if(storage == 'm'){
  fprintf(stderr,"Interp_symm not set up for modal storage in tets \n");
  exit(-1);
      }

      for(k = 0,cnt = 0; k < n; ++k)
  for(j = 0; j < n-k; ++j)
    for(i = 0; i < n-k-j; ++i,++cnt){

      r[0] = 2*i/(double)(n-1)-1;
      r[1] = 2*j/(double)(n-1)-1;
      r[2] = 2*k/(double)(n-1)-1;

      if(j+k != n-1)
        a[0] = 2*(1+r[0])/(-r[1]-r[2])-1;
      else
        a[0] = -1.0;

      if(k != n-1)
        a[1] = 2*(1+r[1])/(1-r[2])-1;
      else
        a[1] = -1.0;

      a[2] = r[2];

      get_point_shape_3d(E,a[0],a[1],a[2],ha,hb,hc);
      for(j1 = 0,k1=0; j1 < qc; ++j1)
        for(i1 = 0; i1 < qb; ++i1,++k1)
    dsmul(qa,hc[j1]*hb[i1],ha,1,mat[cnt]+k1*qa,1);
    }
      free(ha);
      free(hb);
      free(hc);
      break;
    default:
      fprintf(stderr,"Element is not setup in Interp_symmpts\n");
      exit(-1);
      break;
    }

    nstore  = n;
    qstore1 = qa;
    qstore2 = qb;
    qstore3 = qc;
    NekTypeStore = E->identify();

    free(order);

  }

  utmp = dvector(0,ntot-1); // use extra array so ui and uj can be same vector
  dgemv('T',qtot,ntot,1.0,*mat,qtot,uj,1,0.0,utmp,1);
  dcopy(ntot,utmp,1,ui,1);
  free(utmp);

  return ntot;
}
