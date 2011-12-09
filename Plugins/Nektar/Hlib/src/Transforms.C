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

static void Hex_map_hj(Element *E, double *d);

static void getlm(int p, int *i, int *j, int l);
static MMinfo *Tet_addmmat2d        (int L, Element *E);
static MMinfo *Pyr_tri_addmmat2d    (int L, Element *E);
static MMinfo *Pyr_quad_addmmat2d   (int L, Element *E);
static MMinfo *Prism_tri_addmmat2d  (int L, Element *E);
static MMinfo *Prism_quad_addmmat2d (int L, Element *E);
static MMinfo *Hex_addmmat2d(int L, Element *E);



  // Transformation routines
void Element::Trans(Element *, Nek_Trans_Type){ERR;} // Transform to Element


void Tri::Trans(Element *e, Nek_Trans_Type trans_type){
  switch(trans_type){
  case J_to_Q:{
    Jbwd(e, e->getbasis());
    break;
  }
  case Q_to_J:{
    Jfwd(e);
    break;
  }
  case F_to_P:{
    fprintf(stderr,"Tri::Trans F_to_P Not implemented yet\n");
    break;
  }
  case P_to_F:{
    fprintf(stderr,"Tri::Trans P_to_F Not implemented yet\n");
    break;
  }
  default:{
    fprintf(stderr,"Tri::Trans  unrecognised transform\n");
    break;
  }
  }
}


void Quad::Trans(Element *e, Nek_Trans_Type trans_type){
  Quad *T=(Quad*)e;
  switch(trans_type){
  case J_to_Q:{
    Jbwd(T, T->getbasis());
    break;
  }
  case Q_to_J:{
    Jfwd(T);
    //    Quad_Massfwd(this, e);
    break;
  }
  case F_to_P:{
    fprintf(stderr,"Quad::Trans F_to_P Not implemented yet\n");
    break;
  }
  case P_to_F:{
    fprintf(stderr,"Quad::Trans P_to_F Not implemented yet\n");
    break;
  }
  default:{
    fprintf(stderr,"Quad::Trans  unrecognised transform\n");
    break;
  }
  }
}


void Tet::Trans(Element *e, Nek_Trans_Type trans_type){
  switch(trans_type){
  case J_to_Q:{
    Jbwd(e, e->getbasis());
    break;
  }
  case Q_to_J:{
    Jfwd(e);
    break;
  }
  case F_to_P:{
    fprintf(stderr,"Tet::Trans F_to_P Not implemented yet\n");
    break;
  }
  case P_to_F:{
    fprintf(stderr,"Tet::Trans P_to_F Not implemented yet\n");
    break;
  }
  default:{
    fprintf(stderr,"Tet::Trans  unrecognised transform\n");
    break;
  }
  }
}


void Pyr::Trans(Element *e, Nek_Trans_Type trans_type){
  switch(trans_type){
  case J_to_Q:{
    Jbwd(e, e->getbasis());
    break;
  }
  case Q_to_J:{
    Jfwd(e);
    break;
  }
  case F_to_P:{
    fprintf(stderr,"Pyr::Trans F_to_P Not implemented yet\n");
    break;
  }
  case P_to_F:{
    fprintf(stderr,"Pyr::Trans P_to_F Not implemented yet\n");
    break;
  }
  default:{
    fprintf(stderr,"Pyr::Trans  unrecognised transform\n");
    break;
  }
  }
}

void Prism::Trans(Element *e, Nek_Trans_Type trans_type){
  switch(trans_type){
  case J_to_Q:{
    Jbwd(e, e->getbasis());
    break;
  }
  case Q_to_J:{
    Jfwd(e);
    break;
  }
  case F_to_P:{
    fprintf(stderr,"Prism::Trans F_to_P Not implemented yet\n");
    break;
  }
  case P_to_F:{
    fprintf(stderr,"Prism::Trans P_to_F Not implemented yet\n");
    break;
  }
  default:{
    fprintf(stderr,"Prism::Trans  unrecognised transform\n");
    break;
  }
  }
}

void Hex::Trans(Element *e, Nek_Trans_Type trans_type){
  switch(trans_type){
  case J_to_Q:{
    Jbwd(e, e->getbasis());
    break;
  }
  case Q_to_J:{
    Jfwd(e);
    break;
  }
  case F_to_P:{
    fprintf(stderr,"Hex::Trans F_to_P Not implemented yet\n");
    break;
  }
  case P_to_F:{
    fprintf(stderr,"Hex::Trans P_to_F Not implemented yet\n");
    break;
  }
  case O_to_Q:{
    fprintf(stderr,"Hex::Trans P_to_F Not implemented yet\n");
    break;
  }
  case Q_to_O:{
    fprintf(stderr,"Hex::Trans P_to_F Not implemented yet\n");
    break;
  }
  default:{
    fprintf(stderr,"Hex::Trans  unrecognised transform\n");
    break;
  }
  }
}


/*

Function name: Element::Jbwd

Function Purpose:

Argument 1: Element *T
Purpose:

Argument 2: Basis *b
Purpose:

Function Notes:

*/

void Tri::Jbwd(Element *T, Basis *b){

  register int i,L;
  const    int Qa = T->qa, Qb = T->qb;
  double   *H = T->h[0];
  Edge    *e = edge;
  Face    *f = face;
  size_t  d  = sizeof(double);

  dzero(lmax*(lmax+1)/2+1, H, 1);

  /* copy field into h for moment */
  *H = vert[2].hj[0]; ++H;
  *H = vert[1].hj[0]; ++H;
  memcpy(H,e[1].hj,e[1].l*d); H += lmax-2;
  *H = vert[2].hj[0]; ++H;
  *H = vert[0].hj[0]; ++H;
  memcpy(H,e[2].hj,e[2].l*d); H += lmax-2;

  for(i = 0, L = 0; i < e[0].l; L += lmax-2-i, ++i)
    H[L] = e[0].hj[i];
  ++H;
  for(i = 0; i < f[0].l; H += lmax-2-i, ++i)
    memcpy(H,f[0].hj[i],(f[0].l-i)*d);

  T->state = 'p';
  H = T->h[0];

  double *wk = Tri_wk.get();

#if 0  /* dgemm and dgemv version */
  dgemv('N',Qb,lmax,1.0,b->vert[2].b,Qb,H,1,0.0,wk,  lmax); H+=lmax;
  dgemv('N',Qb,lmax,1.0,b->vert[2].b,Qb,H,1,0.0,wk+1,lmax); H+=lmax;
  for(i = 0; i < lmax-2; H+=lmax-2-i, ++i)
    dgemv('N',Qb,lmax-2-i,1.0,b->edge[0][i].b,Qb,H,1,0.0,wk+2+i,lmax);
  dgemm('N','N',Qa,Qb,lmax,1.0,b->vert[1].a,Qa,wk,lmax,0.0,T->h[0],Qa);
#else /* mxv and mxm vertions */
  mxva(b->vert[2].b,1,Qb,H,1,wk  ,lmax,Qb,lmax); H+=lmax;
  mxva(b->vert[2].b,1,Qb,H,1,wk+1,lmax,Qb,lmax); H+=lmax;
  for(i = 0; i < lmax-2; H+=lmax-2-i, ++i)
    mxva(b->edge[0][i].b,1,Qb,H,1,wk+2+i,lmax,Qb,lmax-2-i);
  mxm(wk,Qb,b->vert[1].a,lmax,T->h[0],Qa);
#endif
}




void Quad::Jbwd(Element *T, Basis *b){
  double *H, *wk;
  Edge   *e = edge;
  Face   *f = face;
  size_t  d = sizeof(double);
  int i;

  H = Quad_Jbwd_wk;
  dzero(lmax*lmax,H,1);

  /* pack h */
  /* copy field into T->h for moment */
  *H = vert[2].hj[0]; ++H;
  *H = vert[3].hj[0]; ++H;
  memcpy(H,e[2].hj,e[2].l*d); H += lmax-2;
  *H = vert[1].hj[0]; ++H;
  *H = vert[0].hj[0]; ++H;
  memcpy(H,e[0].hj,e[0].l*d); H += lmax-2;

  dcopy(e[1].l, e[1].hj, 1, H, lmax);++H;
  dcopy(e[3].l, e[3].hj, 1, H, lmax);++H;

  for(i = 0; i < f[0].l; ++H, ++i)
    dcopy(f[0].l, f[0].hj[i], 1, H, lmax);

  H  = Quad_Jbwd_wk;
  wk = Quad_wk;

  dgemm('N','N', qa,lmax,lmax,1.0,  b->vert[1].a, qa, H,lmax,0.0,wk, qa);
  dgemm('N','T', qa,  qb,lmax,1.0, wk, qa,  b->vert[2].b,qb, 0.0,*T->h, qa);

  T->state = 'p';
}


void Tet::Jbwd(Element *E, Basis *b){
  register int i,j,k;
  const    int qbc = qb*qc;
  int      lm,ll;
  Vert    *v = vert;
  Edge    *e = edge;
  Face    *f = face;
  double  *f1,*f2,*f2a;

  double *wk =  Tet_Jbwd_wk;

  /* this all needs to be done for variable case */
  lm = max(e[0].l,f[0].l);
  lm = max(lm,f[1].l);
  lm = max(lm,E->interior_l);
  dzero(qbc*(lm+3),wk,1);

  E->state = 'p';

  /* generate fljk */
  /* vertices */

  /* vertex D */
  f1 = wk;  f2 = wk + (lm+3)*qbc;
  dsmul(qc,v[3].hj[0],b->vert[3].c,1,f2,1);
  for(i = 0; i < qc; ++i)
    dsmul(qb,f2[i],b->vert[3].b,1,f1+i*qb,1);

  /* vertex C */
  dsmul(qc,v[2].hj[0],b->vert[2].c,1,f2,1);
  /* edge 6 */
  for(i = 0; i < e[5].l; ++i)
    daxpy(qc,e[5].hj[i],b->edge[5][i].c,1,f2,1);
  for(i = 0; i < qc; ++i)
    daxpy(qb,f2[i],b->vert[2].b,1,f1+i*qb,1);


  /* vertex B */
  dsmul(qc,v[1].hj[0],b->vert[1].c,1,f2,1);
  /* edge 5 */
  for(i = 0; i < e[4].l; ++i)
    daxpy(qc,e[4].hj[i],b->edge[4][i].c,1,f2,1);

  f1 += qbc;
  for(i = 0; i < qc; ++i)
    dsmul(qb,f2[i],b->vert[1].b,1,f1+i*qb,1);

  /* edge 2 */
  /* do variable by checking to see which of the edge/face combination
   has a higher l order */

  if(e[1].l > f[2].l){
    /* edge 2 */
    for(i = 0,f2a = f2; i < e[1].l; ++i,f2a += qc)
      dsmul(qc,e[1].hj[i],b->edge[1][i].c,1,f2a,1);

    /* face 3 */
    for(i = 0,f2a = f2; i < f[2].l; ++i,f2a += qc)
      for(j = 0; j < f[2].l-i; ++j)
  daxpy(qc,f[2].hj[i][j],b->face[2][i][j].c,1,f2a,1);

    for(i = 0,f2a = f2; i < e[1].l; ++i,f2a += qc)
      for(j = 0; j < qc; ++j)
  daxpy(qb,f2a[j],b->edge[1][i].b,1,f1+j*qb,1);
  }
  else{ /* use first line of face 3 to initialise f2 */
    /* face 3 */
    for(i = 0,f2a = f2; i < f[2].l; ++i,f2a += qc)
      dsmul(qc,f[2].hj[i][0],b->face[2][i][0].c,1,f2a,1);

    for(i = 0,f2a = f2; i < f[2].l; ++i,f2a += qc)
      for(j = 1; j < f[2].l-i; ++j)
  daxpy(qc,f[2].hj[i][j],b->face[2][i][j].c,1,f2a,1);

    /* edge 2 */
    for(i = 0,f2a = f2; i < e[1].l;++i,f2a+=qc)
      daxpy(qc,e[1].hj[i],b->edge[1][i].c,1,f2a,1);

    for(i = 0,f2a = f2; i < f[2].l; ++i,f2a += qc)
      for(j = 0; j < qc; ++j)
  daxpy(qb,f2a[j],b->edge[1][i].b,1,f1+j*qb,1);
  }

  /* vertex A */
  dsmul(qc,v[0].hj[0],b->vert[0].c,1,f2,1);
  /* edge 4 */
  for(i = 0; i < e[3].l; ++i)
    daxpy(qc,e[3].hj[i],b->edge[3][i].c,1,f2,1);

  f1 += qbc;
  for(i = 0; i < qc; ++i)
    dsmul(qb,f2[i],b->vert[0].b,1,f1+i*qb,1);

  if(e[2].l > f[3].l){
    /* edge 3 */
    for(i = 0,f2a = f2; i < e[2].l; ++i,f2a += qc)
      dsmul(qc,e[2].hj[i],b->edge[2][i].c,1,f2a,1);

    /* face 4 */
    for(i = 0,f2a = f2; i < f[3].l; ++i,f2a += qc)
      for(j = 0; j < f[3].l-i; ++j)
  daxpy(qc,f[3].hj[i][j],b->face[3][i][j].c,1,f2a,1);

    for(i = 0,f2a=f2; i < e[2].l; ++i,f2a += qc)
      for(j = 0; j < qc; ++j)
  daxpy(qb,f2a[j],b->edge[2][i].b,1,f1+j*qb,1);
  }
  else{ /* use first line of face 4 to initialise f2 */
    /* face 4 */
    for(i = 0,f2a = f2; i < f[3].l; ++i,f2a += qc)
      dsmul(qc,f[3].hj[i][0],b->face[3][i][0].c,1,f2a,1);

    for(i = 0,f2a = f2; i < f[3].l; ++i,f2a += qc)
      for(j = 1; j < f[3].l-i; ++j)
  daxpy(qc,f[3].hj[i][j],b->face[3][i][j].c,1,f2a,1);

    /* edge 3 */
    for(i = 0,f2a = f2; i < e[2].l; ++i,f2a += qc)
      daxpy(qc,e[2].hj[i],b->edge[2][i].c,1,f2a,1);

    for(i = 0,f2a=f2; i < f[3].l; ++i,f2a += qc)
      for(j = 0; j < qc; ++j)
  daxpy(qb,f2a[j],b->edge[2][i].b,1,f1+j*qb,1);
  }

  if(e[0].l > f[1].l){
    /* edge 1 */
    for(i = 0,f2a = f2; i < e[0].l; ++i,f2a += qc)
      dsmul(qc,e[0].hj[i],b->edge[0][i].c,1,f2a,1);

    /* face 2 */
    for(i = 0,f2a = f2; i < f[1].l; ++i, f2a += qc)
      for(j = 0; j < f[1].l -i; ++j)
  daxpy(qc,f[1].hj[i][j],b->face[1][i][j].c,1,f2a,1);

    f1 += qbc;
    for(i = 0,f2a = f2; i < e[0].l; ++i,f1+=qbc,f2a += qc)
      for(j = 0; j < qc; ++j)
  dsmul(qb,f2a[j],b->edge[0][i].b,1,f1+j*qb,1);
  }
  else{ /* use first line of face 4 to initialise f2 */
    /* face 2 */
    for(i = 0,f2a = f2; i < f[1].l; ++i, f2a += qc)
      dsmul(qc,f[1].hj[i][0],b->face[1][i][0].c,1,f2a,1);

    for(i = 0,f2a = f2; i < f[1].l; ++i, f2a += qc)
      for(j = 1; j < f[1].l-i; ++j)
  daxpy(qc,f[1].hj[i][j],b->face[1][i][j].c,1,f2a,1);

    /* edge 1 */
    for(i = 0,f2a = f2; i < e[0].l; ++i,f2a += qc)
      daxpy(qc,e[0].hj[i],b->edge[0][i].c,1,f2a,1);

    f1 += qbc;
    for(i = 0,f2a = f2; i < f[1].l; ++i,f1+=qbc,f2a += qc)
      for(j = 0; j < qc; ++j)
  dsmul(qb,f2a[j],b->edge[0][i].b,1,f1+j*qb,1);
  }

  /* face 1 and interior */
  if(f[0].l > E->interior_l)
    for(i=0,f1 = wk+3*qbc; i < f[0].l; ++i,f1+=qbc){
      for(j = 0,f2a = f2; j < f[0].l-i; ++j,f2a += qc)
  dsmul(qc,f[0].hj[i][j],b->face[0][i][j].c,1,f2a,1);

      for(j = 0,f2a = f2; j < (ll=E->interior_l-i); ++j,f2a +=qc)
  for(k = 0; k < ll-j; ++k)
    daxpy(qc,hj_3d[i][j][k],b->intr[i][j][k].c,1,f2a,1);

      for(j = 0,f2a = f2; j < f[0].l-i; ++j,f2a +=qc)
  for(k = 0; k < qc; ++k)
    daxpy(qb,f2a[k],b->face[0][i][j].b,1,f1+k*qb,1);
    }
  else /* do interior first */
    // l?
    for(i=0,f1 = wk+3*qbc; i < E->interior_l; ++i,f1+=qbc){
      for(j = 0,f2a = f2; j < (ll=E->interior_l-i); ++j,f2a +=qc)
  dsmul(qc,hj_3d[i][j][0],b->intr[i][j][0].c,1,f2a,1);

      for(j = 0,f2a = f2; j < ll; ++j,f2a +=qc)
  for(k = 1; k < ll-j; ++k)
    daxpy(qc,hj_3d[i][j][k],b->intr[i][j][k].c,1,f2a,1);

      for(j = 0,f2a = f2; j < f[0].l-i; ++j,f2a += qc)
  daxpy(qc,f[0].hj[i][j],b->face[0][i][j].c,1,f2a,1);

      // l?
      for(j = 0,f2a = f2; j < E->interior_l-i; ++j,f2a +=qc)
  for(k = 0; k < qc; ++k)
    daxpy(qb,f2a[k],b->face[0][i][j].b,1,f1+k*qb,1);
    }

  /* calculate transformed values */
  dgemm('N','T',qa,qbc,lm+3,1.0,b->vert[2].a,qa,wk,qbc,0.,**E->h_3d,qa);
}




void Pyr::Jbwd(Element *E, Basis *B){
  int i,j,k;
  double *wk = Pyr_Jbwd_wk;
  double *H;

  Mode   *bv = B->vert;
  Mode  **be = B->edge;
  Mode ***bf = B->face;
  Mode ***bi = B->intr;

  Vert   *v  = vert;
  Edge   *e  = edge;
  Face   *f  = face;

  dzero(QGmax*QGmax*QGmax, wk, 1);

  // bottom of layer of data

  H = wk;

  daxpy(qc, v[0].hj[0], bv[0].c, 1, H, lmax*lmax);
  daxpy(qc, v[4].hj[0], bv[4].c, 1, H, lmax*lmax);

  for(i=0;i<e[4].l;++i)
    daxpy(qc, e[4].hj[i], be[4][i].c, 1, H, lmax*lmax);

  H += 1;

  for(i=0;i<e[0].l;++i)
    daxpy(qc, e[0].hj[i], be[0][i].c, 1, H+i, lmax*lmax);

  for(i=0;i<f[1].l;++i)
    for(j=0;j<f[1].l-i;++j)
      daxpy(qc, f[1].hj[i][j], bf[1][i][j].c, 1, H+i, lmax*lmax);

  H += (lmax-2);

  daxpy(qc, v[1].hj[0], bv[1].c, 1, H, lmax*lmax);
  daxpy(qc, v[4].hj[0], bv[4].c, 1, H, lmax*lmax);

  for(i=0;i<e[5].l;++i)
    daxpy(qc, e[5].hj[i], be[5][i].c, 1, H, lmax*lmax);

  // ok so far

  H = wk + lmax;

  for(i=0;i<e[3].l;++i)
    daxpy(qc, e[3].hj[i], be[3][i].c, 1, H+i*lmax, lmax*lmax);

  // could be wrong
  for(i=0;i<f[4].l;++i)
    for(j=0;j<f[4].l-i;++j)
      daxpy(qc, f[4].hj[i][j], bf[4][i][j].c, 1, H+i*lmax, lmax*lmax);

  H += 1;

  int il = interior_l;

  for(i=0;i<il;++i)
    for(j=0;j<il-i;++j)
      for(k=0;k<il-i-j;++k)
  daxpy(qc, hj_3d[i][j][k], bi[i][j][k].c, 1, H+i+j*lmax, lmax*lmax);

  for(i=0;i<f[0].l;++i)
    for(j=0;j<f[0].l;++j)
      daxpy(qc, f[0].hj[i][j], bf[0][i][j].c, 1, H+j+i*lmax, lmax*lmax);

  // more or less ok so far

  H += (lmax-2);

  for(i=0;i<e[1].l;++i)
    daxpy(qc, e[1].hj[i], be[1][i].c, 1, H+i*lmax, lmax*lmax);

  for(i=0;i<f[2].l;++i)
    for(j=0;j<f[2].l-i;++j)
      daxpy(qc, f[2].hj[i][j], bf[2][i][j].c, 1, H+i*lmax, lmax*lmax);

  H = wk + lmax*(lmax-1);  // fill in top layer

  daxpy(qc, v[3].hj[0], bv[3].c, 1, H, lmax*lmax);
  daxpy(qc, v[4].hj[0], bv[4].c, 1, H, lmax*lmax);

  for(i=0;i<e[7].l;++i)
    daxpy(qc, e[7].hj[i], be[7][i].c, 1, H, lmax*lmax);

  H += 1;

  for(i=0;i<e[2].l;++i)
    daxpy(qc, e[2].hj[i], be[2][i].c, 1, H+i, lmax*lmax);

  for(i=0;i<f[3].l;++i)
    for(j=0;j<f[3].l-i;++j)
      daxpy(qc, f[3].hj[i][j], bf[3][i][j].c, 1, H+i, lmax*lmax);

  H += (lmax-2);

  daxpy(qc, v[2].hj[0], bv[2].c, 1, H, lmax*lmax);
  daxpy(qc, v[4].hj[0], bv[4].c, 1, H, lmax*lmax);

  for(i=0;i<e[6].l;++i)
    daxpy(qc, e[6].hj[i], be[6][i].c, 1, H, lmax*lmax);


  dgemm('N','N',qa,lmax*qc,lmax,1.0,bv[0].a,qa,wk,lmax,0.0,**h_3d,qa);

  for(i = 0; i < qc; ++i)
    dgemm('N','T',qa,qb,lmax,1.0,
    (**h_3d)+i*qa*lmax,qa,bv[0].a,qb,0.0,wk+i*qa*qb,qa);

  dcopy(qtot, wk, 1, **E->h_3d, 1);
}




void Prism::Jbwd(Element *E, Basis *B){
  int i,j,k;

  double *H;
  double *wk = Prism_Jbwd_wk;

  Mode   *bv = B->vert;
  Mode  **be = B->edge;
  Mode ***bf = B->face;
  Mode ***bi = B->intr;

  Vert   *v  = vert;
  Edge   *e  = edge;
  Face   *f  = face;

  dzero(QGmax*QGmax*QGmax, wk, 1);

  // bottom of layer of data

  H = wk;

  daxpy(qc, v[0].hj[0], bv[0].c, 1, H, lmax*lmax);
  daxpy(qc, v[4].hj[0], bv[4].c, 1, H, lmax*lmax);

  for(i=0;i<e[4].l;++i)
    daxpy(qc, e[4].hj[i], be[4][i].c, 1, H, lmax*lmax);

  H += 1;

  for(i=0;i<e[0].l;++i)
    daxpy(qc, e[0].hj[i], be[0][i].c, 1, H+i, lmax*lmax);

  for(i=0;i<f[1].l;++i)
    for(j=0;j<f[1].l-i;++j)
      daxpy(qc, f[1].hj[i][j], bf[1][i][j].c, 1, H+i, lmax*lmax);

  H += (lmax-2);

  daxpy(qc, v[1].hj[0], bv[1].c, 1, H, lmax*lmax);
  daxpy(qc, v[4].hj[0], bv[4].c, 1, H, lmax*lmax);

  for(i=0;i<e[5].l;++i)
    daxpy(qc, e[5].hj[i], be[5][i].c, 1, H, lmax*lmax);


  H = wk + lmax;

  for(i=0;i<e[3].l;++i)
    daxpy(qc, e[3].hj[i], be[3][i].c, 1, H+i*lmax, lmax*lmax);

  for(i=0;i<e[8].l;++i)
    daxpy(qc, e[8].hj[i], be[8][i].c, 1, H+i*lmax, lmax*lmax);

  for(i=0;i<f[4].l;++i)
    for(j=0;j<f[4].l;++j)
      daxpy(qc, f[4].hj[i][j], bf[4][i][j].c, 1, H+j*lmax, lmax*lmax);


  H += 1;


  int il = interior_l;

  for(i=0;i<il-1;++i)
    for(j=0;j<il;++j)
      for(k=0;k<il-1-i;++k)
  daxpy(qc, hj_3d[i][j][k], bi[i][j][k].c, 1, H+i+j*lmax, lmax*lmax);

  for(i=0;i<f[0].l;++i)
    for(j=0;j<f[0].l;++j)
      daxpy(qc, f[0].hj[i][j], bf[0][i][j].c, 1, H+j+i*lmax, lmax*lmax);

  H += (lmax-2);

  for(i=0;i<e[1].l;++i)
    daxpy(qc, e[1].hj[i], be[1][i].c, 1, H+i*lmax, lmax*lmax);

  for(i=0;i<e[8].l;++i)
    daxpy(qc, e[8].hj[i], be[8][i].c, 1, H+i*lmax, lmax*lmax);

  for(i=0;i<f[2].l;++i)
    for(j=0;j<f[2].l;++j)
      daxpy(qc, f[2].hj[i][j], bf[2][i][j].c, 1, H+j*lmax, lmax*lmax);

  H = wk + lmax*(lmax-1);  // fill in top layer

  daxpy(qc, v[3].hj[0], bv[3].c, 1, H, lmax*lmax);
  daxpy(qc, v[5].hj[0], bv[5].c, 1, H, lmax*lmax);

  for(i=0;i<e[7].l;++i)
    daxpy(qc, e[7].hj[i], be[7][i].c, 1, H, lmax*lmax);

  H += 1;

  for(i=0;i<e[2].l;++i)
    daxpy(qc, e[2].hj[i], be[2][i].c, 1, H+i, lmax*lmax);

  for(i=0;i<f[3].l;++i)
    for(j=0;j<f[3].l-i;++j)
      daxpy(qc, f[3].hj[i][j], bf[3][i][j].c, 1, H+i, lmax*lmax);

  H += (lmax-2);

  daxpy(qc, v[2].hj[0], bv[2].c, 1, H, lmax*lmax);
  daxpy(qc, v[5].hj[0], bv[5].c, 1, H, lmax*lmax);

  for(i=0;i<e[6].l;++i)
    daxpy(qc, e[6].hj[i], be[6][i].c, 1, H, lmax*lmax);


  dgemm('N','N',qa,lmax*qc,lmax,1.0,bv[0].a,qa,wk,lmax,0.0,**h_3d,qa);

  for(i = 0; i < qc; ++i)
    dgemm('N','T',qa,qb,lmax,1.0,
    (**h_3d)+i*qa*lmax,qa,bv[0].a,qb,0.0,wk+i*qa*qb,qa);

  dcopy(qtot, wk, 1, **E->h_3d, 1);
}



void Hex::Jbwd(Element *E, Basis *B){
  int i;

  double *H = Hex_Jbwd_wk;

  Hex_map_hj(this, H);

  Mode *bv = B->vert;

  /* calculate 'a' direction */
  dgemm('N','N',qa,lmax*lmax,lmax,1.0,bv->a,qa,H,lmax,0.0,**E->h_3d,qa);

  for(i = 0; i < lmax; ++i)
    dgemm('N','T',qa,qb,lmax,1.0,(**E->h_3d)+i*qa*lmax,qa,bv->a,qb,0.0,H+i*qa*qb,qa);

  dgemm('N','T',qa*qb,qc,lmax,1.0,H,qa*qb,bv->a,qc,0.0,**E->h_3d,qa*qb);

}




void Element::Jbwd(Element *, Basis *){ERR;}



static void Hex_map_hj(Element *E, double *d){
  int lm = E->lmax;
  int i,j,k;

  dzero(lm*lm*lm, d, 1);

  d[0]         = E->vert[0].hj[0];
  d[lm-1]      = E->vert[1].hj[0];
  d[lm*lm-1]   = E->vert[2].hj[0];
  d[lm*(lm-1)] = E->vert[3].hj[0];

  d[lm*lm*(lm-1)+0]         = E->vert[4].hj[0];
  d[lm*lm*(lm-1)+lm-1]      = E->vert[5].hj[0];
  d[lm*lm*(lm-1)+lm*lm-1]   = E->vert[6].hj[0];
  d[lm*lm*(lm-1)+lm*(lm-1)] = E->vert[7].hj[0];

  dcopy(E->edge[0].l, E->edge[0].hj, 1, d+1, 1);
  dcopy(E->edge[1].l, E->edge[1].hj, 1, d+lm+lm-1, lm);
  dcopy(E->edge[2].l, E->edge[2].hj, 1, d+lm*(lm-1)+1, 1);
  dcopy(E->edge[3].l, E->edge[3].hj, 1, d+lm, lm);

  dcopy(E->edge[4].l, E->edge[4].hj, 1, d+lm*lm, lm*lm);
  dcopy(E->edge[5].l, E->edge[5].hj, 1, d+lm*lm+lm-1, lm*lm);
  dcopy(E->edge[6].l, E->edge[6].hj, 1, d+lm*lm+lm*lm-1, lm*lm);
  dcopy(E->edge[7].l, E->edge[7].hj, 1, d+lm*lm+lm*(lm-1), lm*lm);

  dcopy(E->edge[8].l, E->edge[8].hj, 1, d+lm*lm*(lm-1)+1, 1);
  dcopy(E->edge[9].l, E->edge[9].hj, 1, d+lm*lm*(lm-1)+lm+lm-1, lm);
  dcopy(E->edge[10].l, E->edge[10].hj, 1, d+lm*lm*(lm-1)+lm*(lm-1)+1, 1);
  dcopy(E->edge[11].l, E->edge[11].hj, 1, d+lm*lm*(lm-1)+lm, lm);

   // faces
  for(i=0;i<E->face[0].l;++i)
    dcopy(E->face[0].l, E->face[0].hj[i], 1, d+lm+1+i*lm, 1);

  for(i=0;i<E->face[1].l;++i)
    dcopy(E->face[1].l, E->face[1].hj[i], 1, d+lm*lm+1+i*lm*lm, 1);

  for(i=0;i<E->face[2].l;++i)
    dcopy(E->face[2].l, E->face[2].hj[i], 1, d+lm*lm+lm+lm-1+i*lm*lm, lm);

  for(i=0;i<E->face[3].l;++i)
    dcopy(E->face[3].l, E->face[3].hj[i], 1, d+lm*lm+lm*(lm-1)+1+i*lm*lm, 1);

  for(i=0;i<E->face[4].l;++i)
    dcopy(E->face[4].l, E->face[4].hj[i], 1, d+lm*lm+lm+i*lm*lm, lm);

  for(i=0;i<E->face[5].l;++i)
    dcopy(E->face[5].l, E->face[5].hj[i], 1, d+lm*lm*(lm-1)+lm+1+i*lm, 1);

  // interior

  for(i=0;i<E->interior_l;++i)
    for(j=0;j<E->interior_l;++j)
      for(k=0;k<E->interior_l;++k)
  d[lm*lm+lm+1+k+j*lm+i*lm*lm] = E->hj_3d[i][j][k];
}


int Tri_compare_L(Element *E);


static LocMat *Tri_Jfwd_mass = (LocMat*)0;
static double *Tri_Jfwd_invm = (double*)0;
static double  Tri_Jfwd_jac  = 0.0;
static int *Tri_mass_L = (int*)0;


/*

Function name: Element::Jfwd

Function Purpose:

Argument 1: Element *E
Purpose:

Function Notes:

*/

void Tri::Jfwd(Element *E){
  register int i,j,n;
  int     asize,csize,N,info;
  double *save = dvector(0,QGmax*QGmax-1);

  asize = Nverts;
  for(i = 0; i < Nedges; ++i)
    asize += E->edge[i].l;

  csize = Nfmodes();
  N = asize + csize;

  if(curvX){
    double *invm = dvector(0,(LGmax*(LGmax+1)/2)*(LGmax*(LGmax+1)/2+1)/2-1);
    LocMat *mass  = mat_mem();

    dcopy(E->qa*E->qb,*E->h,1,save,1);
    MassMatC(mass);
    dcopy(E->qa*E->qb,save,1,*E->h,1);

    for(i = 0,n=0; i <asize; ++i){
      for(j=i;j < asize; ++j,++n)
  invm[n] = mass->a[i][j];
      if(csize)
  dcopy(csize,mass->b[i],1,invm+n,1);
      n += csize;
    }

    for(i = 0; i < csize; ++i)
      for(j = i; j < csize; ++j,++n)
  invm[n] = mass->c[i][j];
    info = 0;
    dpptrf('L',N,invm,info);
    if(info) error_msg(Jtransfwd_Loc: info not zero curved);

    Iprod(E);
    dpptrs('L',N,1,invm,E->vert->hj,N,info);

    free(invm);
    mat_free(mass);
    dcopy(E->qa*E->qb,save,1,*E->h,1);
  }
  else{

    dcopy(E->qa*E->qb,*E->h,1,save,1);

    if(Tri_compare_L(this)){
      Tri_Jfwd_jac  = geom->jac.d;
      Tri_Jfwd_invm = dvector(0,N*N-1);
      Tri_Jfwd_mass = mat_mem();

      MassMat(Tri_Jfwd_mass);
      dcopy(E->qa*E->qb,save,1,*E->h,1);

      for(i = 0,n=0; i <asize; ++i){
  for(j=i;j < asize; ++j,++n)
    Tri_Jfwd_invm[n] = Tri_Jfwd_mass->a[i][j];
  if(csize)
    dcopy(csize,Tri_Jfwd_mass->b[i],1,Tri_Jfwd_invm+n,1);
  n += csize;
      }

      for(i = 0; i < csize; ++i)
  for(j = i; j < csize; ++j,++n)
    Tri_Jfwd_invm[n] = Tri_Jfwd_mass->c[i][j];
      info = 0;
      dpptrf('L',N,Tri_Jfwd_invm,info);
      if(info) error_msg(Jtransfwd_Loc: info not zero straight );

    }

    Iprod(E);
    dpptrs('L',N,1,Tri_Jfwd_invm,E->vert->hj,N,info);

    dscal(N,Tri_Jfwd_jac/E->geom->jac.d,E->vert->hj,1);

    dcopy(E->qa*E->qb,save,1,*E->h,1);
  }
  free(save);

}



void Quad::Jfwd(Element *E){
  register int i,j,n;
  int     asize,csize,L,info=0;
  double *invm = dvector(0,QGmax*QGmax*QGmax*QGmax-1);

  LocMat *mass  = mat_mem();

  double *save = dvector(0,QGmax*QGmax-1);
  dcopy(E->qa*E->qb,*E->h,1,save,1);
  MassMatC(mass);
  dcopy(E->qa*E->qb,save,1,*E->h,1);

  asize = Nverts;
  for(i = 0; i < Nedges; ++i)
    asize += E->edge[i].l;

  csize = E->face->l*E->face->l;

  for(i = 0,n=0; i <asize; ++i){
    for(j=i;j < asize; ++j,++n)
      invm[n] = mass->a[i][j];
    if(csize)
      dcopy(csize,mass->b[i],1,invm+n,1);
    n += csize;
  }

  for(i = 0; i < csize; ++i)
    for(j = i; j < csize; ++j,++n)
      invm[n] = mass->c[i][j];

  L = asize + csize;
  //  dump_sc(asize, csize, mass->a, mass->b, mass->c);

  dpptrf('L',L,invm,info);
  if(info) error_msg(Jtransfwd_Loc: info not zero);

  Iprod(E);
  dpptrs('L',L,1,invm,E->vert->hj,L,info);

  dcopy(E->qa*E->qb,save,1,*E->h,1);

  free(save);
  free(invm);
  mat_free(mass);
}




void Tet::Jfwd(Element *E){
  register int i,j,n;
  int     asize,csize,N,info;

  double *invm = dvector(0,(LGmax*(LGmax+1)*(LGmax+2)/6)
       *(LGmax*(LGmax+1)*(LGmax+2)/6+1)/2-1);
  LocMat *mass  = mat_mem();

  double *save = dvector(0,QGmax*QGmax*QGmax-1);
  dcopy(E->qtot,**E->h_3d,1,save,1);
  MassMatC(mass);
  dcopy(E->qtot,save,1,**E->h_3d,1);

  asize = Nbmodes;

  /* calc local interior matrix size */
  csize = Nmodes-Nbmodes;

  for(i = 0,n=0; i <asize; ++i){
    for(j=i;j < asize; ++j,++n)
      invm[n] = mass->a[i][j];

    if(csize) dcopy(csize,mass->b[i],1,invm+n,1);
    n += csize;
  }

  for(i = 0; i < csize; ++i)
    for(j = i; j < csize; ++j,++n)
      invm[n] = mass->c[i][j];

  N = asize + csize;

  dpptrf('L',N,invm,info);
  if(info) error_msg(Tet::Jfwd info not zero);

  Iprod(E);
  dpptrs('L',N,1,invm,E->vert->hj,N,info);

  free(invm);
  mat_free(mass);

  dcopy(E->qtot,save,1,**E->h_3d,1);
  free(save);

}



static Element *Pyr_Jfwd_E = (Element*)0;
static double  *Pyr_invm   = (double*)0;


void Pyr::Jfwd(Element *E){
  register int i,j,n;
  int     asize,csize,N,info;

  if(!Pyr_invm)
    Pyr_invm = dvector(0,(LGmax*LGmax*LGmax*LGmax*LGmax*LGmax)-1);

  double *invm = Pyr_invm;

  /* calc local interior matrix size */
  asize = Nbmodes;
  csize = Nmodes-Nbmodes;
  N = asize + csize;

  if(Pyr_Jfwd_E != this){
    LocMat *mass  = mat_mem();

    double *save = dvector(0,QGmax*QGmax*QGmax-1);
    dcopy(E->qtot,**E->h_3d,1,save,1);
    MassMatC(mass);
    dcopy(E->qtot,save,1,**E->h_3d,1);
    free(save);

    for(i = 0,n=0; i <asize; ++i){
      for(j=i;j < asize; ++j,++n)
  invm[n] = mass->a[i][j];

      if(csize) dcopy(csize,mass->b[i],1,invm+n,1);
      n += csize;
    }

    for(i = 0; i < csize; ++i)
      for(j = i; j < csize; ++j,++n)
  invm[n] = mass->c[i][j];

    dpptrf('L',N,invm,info);
    if(info) error_msg(Pyr::Jfwd info not zero);

    Pyr_Jfwd_E = this;

    //    free(invm);
    mat_free(mass);
  }

  Iprod(E);
  dpptrs('L',N,1,invm,E->vert->hj,N,info);

}




void Prism::Jfwd(Element *E){
  register int i,j,n;
  int     asize,csize,N,info;

  double *invm = dvector(0,(LGmax*LGmax*LGmax*LGmax*LGmax*LGmax)-1);
  LocMat *mass  = mat_mem();

  double *save = dvector(0,QGmax*QGmax*QGmax-1);
  dcopy(E->qtot,**E->h_3d,1,save,1);
  MassMatC(mass);
  dcopy(E->qtot,save,1,**E->h_3d,1);


  asize = Nbmodes;

  /* calc local interior matrix size */
  csize = Nmodes-Nbmodes;

  for(i = 0,n=0; i <asize; ++i){
    for(j=i;j < asize; ++j,++n)
      invm[n] = mass->a[i][j];

    if(csize) dcopy(csize,mass->b[i],1,invm+n,1);
    n += csize;
  }

  for(i = 0; i < csize; ++i)
    for(j = i; j < csize; ++j,++n)
      invm[n] = mass->c[i][j];

  N = asize + csize;

  dpptrf('L',N,invm,info);
  if(info) error_msg(Prism::Jfwd info not zero);

  Iprod(E);
  dpptrs('L',N,1,invm,E->vert->hj,N,info);

  free(invm);
  mat_free(mass);
  dcopy(E->qtot,save,1,**E->h_3d,1);
  free(save);
}




void Hex::Jfwd(Element *E){
#if 1
  register int i,j,n;
  int     asize,csize,N,info;

  double *invm = dvector(0,(LGmax*LGmax*LGmax*LGmax*LGmax*LGmax)-1);
  LocMat *mass  = mat_mem();

  double *save = dvector(0,QGmax*QGmax*QGmax-1);
  dcopy(E->qtot,**E->h_3d,1,save,1);
  // jacobian->1
  MassMatC(mass);
  // replace jacobian
  dcopy(E->qtot,save,1,**E->h_3d,1);

  asize = Nbmodes;

  /* calc local interior matrix size */
  csize = Nmodes-Nbmodes;

  for(i = 0,n=0; i <asize; ++i){
    for(j=i;j < asize; ++j,++n)
      invm[n] = mass->a[i][j];

    if(csize) dcopy(csize,mass->b[i],1,invm+n,1);
    n += csize;
  }

  for(i = 0; i < csize; ++i)
    for(j = i; j < csize; ++j,++n)
      invm[n] = mass->c[i][j];

  N = asize + csize;

  dpptrf('L',N,invm,info);
  if(info) error_msg(Hex::Jfwd info not zero);

  // BEGIN LOOP
  // jacobian->1
  Iprod(E);
  // replace jacobian
  dpptrs('L',N,1,invm,E->vert->hj,N,info);
  // END LOOP
  free(invm);
  mat_free(mass);
  dcopy(E->qtot,save,1,**E->h_3d,1);
  free(save);
#endif
#if 0
  register int i,j,n;
  int     asize,csize,N,info;

  static double *Hex_invm = NULL;

  double *save = dvector(0,QGmax*QGmax*QGmax-1);
  dcopy(E->qtot,**E->h_3d,1,save,1);

  // jacobian->1
  double *savejac = dvector(0,QGmax*QGmax*QGmax-1);
  dcopy(E->qtot,E->geom->jac.p,1,savejac,1);
  dfill(E->qtot, 1., E->geom->jac.p, 1);

  if(!Hex_invm){
    static LocMat *mass = mat_mem();
    Hex_invm = dvector(0,(LGmax*LGmax*LGmax*LGmax*LGmax*LGmax)-1);

    MassMatC(mass);

    asize = Nbmodes;

    /* calc local interior matrix size */
    csize = Nmodes-Nbmodes;

    for(i = 0,n=0; i <asize; ++i){
      for(j=i;j < asize; ++j,++n)
  invm[n] = mass->a[i][j];

      if(csize) dcopy(csize,mass->b[i],1,invm+n,1);
      n += csize;
    }

    for(i = 0; i < csize; ++i)
      for(j = i; j < csize; ++j,++n)
  invm[n] = mass->c[i][j];

    N = asize + csize;

    dpptrf('L',N,invm,info);
    if(info) error_msg(Hex::Jfwd info not zero);
    mat_free(mass);
  }

  Iprod(E);
  dpptrs('L',N,1,invm,E->vert->hj,N,info);

  free(invm);

  dcopy(E->qtot,save,1,**E->h_3d,1);
  dcopy(E->qtot,savejac,1,E->geom->jac.p,1);
  free(save);
#endif

}




void Element::Jfwd(Element *){ERR;}

int Tri_compare_L(Element *E){
  int i,trip=0;

  if(!Tri_mass_L){
    Tri_mass_L = ivector(0, E->Nedges);
    izero(E->Nedges+1, Tri_mass_L, 1);
    trip = 1;
  }
  else{
    for(i = 0; i < E->Nedges; ++i)
      if(Tri_mass_L[i] != E->edge[i].l)
  trip =1;

    if(Tri_mass_L[i] != E->face[0].l)
      trip = 1;

    if(trip){
      free       (Tri_Jfwd_invm);
      E->mat_free(Tri_Jfwd_mass);
    }
  }

  if(trip){
    for(i = 0; i < E->Nedges; ++i)
      Tri_mass_L[i] = E->edge[i].l;
    Tri_mass_L[i] = E->face[0].l;
  }
  return trip;
}



/*

Function name: Element::Obwd

Function Purpose:

Argument 1: double *in
Purpose:

Argument 2: double *out
Purpose:

Argument 3: int L
Purpose:

Function Notes:

*/

void Tri::Obwd(double *in, double *out, int L){
  register int i;
  double   **ba,***bb;
  int      mode;

  double *wk = Tri_wk.get();

  get_moda_GL (qa, &ba);
  get_modb_GR (qb, &bb);

  mode = 0;
  /* bwd trans w.r.t. b modes */
  for (i= 0; i < L; ++i){
    mxva (bb[i][0],1,qb,in+mode,1,wk+i*qb,1,qb,L-i);
    mode += L-i;
  }

  /* bwd trans w.r.t. a modes */
  dgemm('N','T',qa,qb,L,1.0,*ba,qa,wk,qb,0.0,out,qa);
}




void Quad::Obwd(double *in, double *out, int L){
  double   **ba,**bb;

  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);

  if(L){
   dgemm('N','N', qa, L, L, 1.0,  ba[0], qa,    in, L, 0.0,  Quad_wk, qa);
   dgemm('N','T', qa, qb, L, 1.0,     Quad_wk, qa, bb[0], qb, 0.0, out, qa);
  }
}




void Tet::Obwd(double *in, double *out, int L){

  double *H = Tet_Iprod_wk;
  double *Ha = Tet_Jbwd_wk;
  int i,j;
  double **ba, ***bb, ***bc;

  get_moda_GL (qa, &ba);
  get_modb_GR (qb, &bb);
  get_modc_GR (qc, &bc);

  // do 'c' transform first
  dzero(qtot, H, 1);
  for (j = 0; j < L; ++j)
    for (i= 0; i < L-j; ++i)
      dgemv('N', qc, L-i-j, 1., bc[i+j][0], qc, in+i+j*L, L*L, 0.0, H+i+j*L, L*L);

  dzero(qtot, Ha, 1);
  // do 'b' transform second
  for (j = 0; j < qc; ++j)
    for (i= 0; i < L; ++i)
      dgemv('N', qb, L-i, 1., bb[i][0], qb, H+i+j*L*L, L, 0.0,  Ha+i+j*L*qb, L);

  // do 'a' transform third
  dgemm('N','N', qa,qb*qc,L,1.,ba[0],qa,Ha,L,0.0,out,qa);
}




void Pyr::Obwd(double *, double *, int ){
  fprintf(stderr, "Pyr::Obwd NOT set up yet\n");
  exit(-1);
}




void Prism::Obwd(double *in, double *out, int L){
  double *H  = Prism_Iprod_wk;
  double *Ha = Prism_Jbwd_wk;
  int i,j;
  double **ba, **bb, ***bc;

  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);
  get_modb_GR (qc, &bc);

  // do 'c' transform first
  dzero(qtot, H, 1);
  for (j = 0; j < L; ++j)
    for (i= 0; i < L; ++i)
      dgemv('N', qc, L-i, 1., bc[i][0], qc, in+i+j*L, L*L, 0.0, H+i+j*L, L*L);

  dzero(qtot, Ha, 1);
  // do 'b' transform second
  for (j = 0; j < qc; ++j)
    for (i= 0; i < L; ++i)
      dgemv('N', qb, L, 1., bb[0], qb, H+i+j*L*L, L, 0.0,  Ha+i+j*L*qb, L);

  // do 'a' transform third
  dgemm('N','N', qa,qb*qc,L,1.,ba[0],qa,Ha,L,0.0,out,qa);
}




void Hex::Obwd(double *in, double *out, int L){
  int i;

  double *H = Hex_Jbwd_wk;
  double *Ha = Hex_Iprod_wk;
  double   **ba, **bb, **bc;

  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);
  get_moda_GL (qc, &bc);

#if 1
  /* calculate 'a' direction */
  dgemm('N','N',qa,L*L,L,1.0,ba[0],qa,in,L,0.0,H,qa);

  for(i = 0; i < L; ++i)
    dgemm('N','T',qa,qb,L,1.0,H+i*qa*L,qa,bb[0],qb,0.0,Ha+i*qa*qb,qa);

  dgemm('N','T',qa*qb,qc,L,1.0,Ha,qa*qb,bc[0],qc,0.0,out,qa*qb);
#else
  // do 'c' transform first
  dzero(qtot, H, 1);
  for (j = 0; j < L; ++j)
    for (i= 0; i < L-j; ++i)
      dgemv('N', qc, L-i-j, 1., bc[0], qc, in+i+j*L, L*L, 0.0, H+i+j*L, L*L);

  dzero(qtot, Ha, 1);
  // do 'b' transform second
  for (j = 0; j < qc; ++j)
    for (i= 0; i < L; ++i)
      dgemv('N', qb, L-i, 1., bb[0], qb, H+i+j*L*L, L, 0.0,  Ha+i+j*L*qb, L);

  // do 'a' transform third
  dgemm('N','N', qa,qb*qc,L,1.,ba[0],qa,Ha,L,0.0,out,qa);
#endif
}




void Element::Obwd(double*, double*, int){ERR;}




/*

Function name: Element::Ofwd

Function Purpose:

Argument 1: double *in
Purpose:

Argument 2: double *out
Purpose:

Argument 3: int L
Purpose:

Function Notes:

*/

void Tri::Ofwd(double *in, double *out, int L){
  Ofwd(in,out,L,1);
}

void Tri::Ofwd(double *in, double *out, int L, int invjac){
  register int i;
  double   **ba,***bb,*wa,*wb;
  int      mode;

  double *wk = Tri_wk.get();

  getzw (qa,&wa,&wa,'a');
  getzw (qb,&wb,&wb,'b');
  get_moda_GL (qa, &ba);
  get_modb_GR (qb, &bb);

  if(option("Stokes")||(invjac == 0)){
    /* multiply by jacobian for stokes solver */
    if(curvX)
      dvmul(qa*qb,geom->jac.p,1,in,1,in,1);
    else
      dscal(qa*qb,geom->jac.d,in,1);
  }

  for(i = 0; i < qa; ++i)
    dscal(qb,wa[i],in+i,qa);

  /* Inner product trans w.r.t. a modes */
  dgemm('T','N',qb,L,qa,1.0,in,qa,*ba,qa,0.0,wk,qb);
#ifdef NONORM
  for(i=0;i<L;++i)
    dscal(qb, 0.5*(2.*i+1.), wk+qb*i, 1);

  double fac = 1.;
#endif

  mode = 0;
  for (i= 0; i < L; ++i){ /* Inner product trans w.r.t. b modes */
    dvmul (qb,wb,1,wk+i*qb,1,wk+i*qb,1);
    mxva (bb[i][0],qb,1,wk+i*qb,1,out+mode,1,L-i,qb);
#ifdef NONORM
    for(j = 0; j < L-i; ++j)
      out[mode+j] *= fac*(i+j+1.);
    fac *= 0.25;
#endif
    mode  += L-i;
  }

}

void Quad::Ofwd(double *in, double *out, int L){
  Ofwd(in,out,L,1);
}

void Quad::Ofwd(double *in, double *out, int L, int invjac){
  register int i;
  double   **ba,**bb,*wa,*wb;

  getzw (qa,&wa,&wa,'a');
  getzw (qb,&wb,&wb,'a');
  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);

  /* multiply by jacobian for stokes solver */
  if(option("Stokes")||(invjac == 0))
    dvmul(qa*qb,geom->jac.p,1,in,1,in,1);

  for(i = 0; i < qb; ++i) dvmul(qa, wa, 1, in+qa*i,  1, in+i*qa,  1);
  for(i = 0; i < qa; ++i) dvmul(qb, wb, 1,    in+i, qa,    in+i, qa);

  if(L){
   dgemm('T','N', L, qb, qa, 1.0,   ba[0], qa,    in, qa, 0.0, Quad_wk, L);

   dgemm('N','N', L, L, qb, 1.0, Quad_wk, L, bb[0], qb, 0.0,     out, L);

#ifdef NONORM
  int j,n;
  for(n=0,j=0;j<L;++j)
    for(i=0;i<L;++i, ++n)
  out[n] *= 0.5*(2.0*j+1.0)*0.5*(2.0*i+1.);
#endif
  }
}




void Tet::Ofwd(double *in, double *out, int L){

  double *H = Tet_Iprod_wk;
  int i,j;
  double *wa,*wb,*wc;
  double **ba, ***bb, ***bc;
  // multiply by weights
  getzw(qa,&wa,&wa,'a');  // P^(0,0)_i(a)
  getzw(qb,&wb,&wb,'b');  // [1-b/2]^i * P^(2*i+1,0)_j(b)
  getzw(qc,&wc,&wc,'c');  // [1-c/2]^i * P^(2*i+2,0)_j(c)

  get_moda_GL (qa, &ba);
  get_modb_GR (qb, &bb);
  get_modc_GR (qc, &bc);

  for(i = 0; i < qb*qc; ++i)
    dvmul(qa,  wa,  1,  in+i*qa, 1, H+i*qa, 1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dscal(qa,    wb[j], H+j*qa+i*qa*qb, 1);
  for(i = 0; i < qc; ++i)
    dscal(qa*qb, wc[i], H+i*qa*qb, 1);

  // do 'a' integration first

  dgemm('T','N',L,qb*qc,qa,1.0,ba[0],qa,H,qa,0.0,out,L);

  dzero(qtot, H, 1);
  // do 'b' integration second
  for (j = 0; j < qc; ++j)
    for (i= 0; i < L; ++i)
      dgemv('T', qb, L-i, 1., bb[i][0], qb, out+i+j*L*qb, L, 0.0,  H+i+j*L*L, L);

  dzero(qtot, out, 1);
  // do 'c' integration third
  for (j = 0; j < L; ++j)
    for (i= 0; i < L-j; ++i)
      dgemv('T', qc, L-i-j, 1., bc[i+j][0], qc, H+i+j*L, L*L, 0.0, out+i+j*L, L*L);
#ifdef NONORM
  fprintf(stderr, "Tet:: Ofwd   not set up for NONORM yet\n");
  exit(-1);
#endif
}




void Pyr::Ofwd(double *, double *, int){
  fprintf(stderr, "Pyr::Obwd NOT set up yet\n");
  exit(-1);
}




void Prism::Ofwd(double *in, double *out, int L){

  double *H = Prism_Iprod_wk;
  int i,j;
  double *wa,*wb,*wc;
  double **ba, **bb, ***bc;
  // multiply by weights
  getzw(qa,&wa,&wa,'a');  // P^(0,0)_i(a)
  getzw(qb,&wb,&wb,'a');  // P^(0,0)_j(b)
  getzw(qc,&wc,&wc,'b');  // [1-c/2]^i * P^(2*i+2,0)_j(c)

  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);
  get_modb_GR (qc, &bc);

  for(i = 0; i < qb*qc; ++i)
    dvmul(qa,  wa,  1,  in+i*qa, 1, H+i*qa, 1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dscal(qa,    wb[j], H+j*qa+i*qa*qb, 1);
  for(i = 0; i < qc; ++i)
    dscal(qa*qb, wc[i], H+i*qa*qb, 1);

  // do 'a' integration first

  dgemm('T','N',L,qb*qc,qa,1.0,ba[0],qa,H,qa,0.0,out,L);

  dzero(qtot, H, 1);
  // do 'b' integration second
  for (j = 0; j < qc; ++j)
    for (i= 0; i < L; ++i)
      dgemv('T', qb, L, 1., bb[0], qb, out+i+j*L*qb, L, 0.0,  H+i+j*L*L, L);

  dzero(qtot, out, 1);
  // do 'c' integration third
  for (j = 0; j < L; ++j)
    for (i= 0; i < L; ++i)
      dgemv('T', qc, L-i, 1., bc[i][0], qc, H+i+j*L, L*L, 0.0, out+i+j*L, L*L);

  // set to 1 on Apr 28
  // set to 0 on Apr 29 :)

#if 0
  for(i=0;i<L;++i)
    for(j=0;j<L;++j)
      for(k=L-i-j;k<L;++k)
    out[i*L*L+j*L+k ] = 0.;
#endif
#ifdef NONORM

  double fac;
  for(i=0, fac = 1.;i<L;++i,fac *= 0.25)
    for(j=0;j<L;++j)
      for(k=0;k<L-i;++k)
  out[k*L*L+j*L+i] *= fac*(i+k+1.0)*0.5*(2.0*j+1.)*0.5*(2.0*i+1.);
  //  out[i*L*L+j*L+k] *= 0.5*(2.0*j+1.)*0.5*(2.0*k+1.);
#endif
}




void Hex::Ofwd(double *in, double *out, int L){

  register int i,j;
  double   *wa,*wb,*wc;
  double   **ba, **bb, **bc;
  double *H = Hex_Iprod_wk;

  getzw (qa,&wa,&wa,'a');
  getzw (qb,&wb,&wb,'a');
  getzw (qc,&wc,&wc,'a');
  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);
  get_moda_GL (qc, &bc);

  for(i = 0; i < qb*qc; ++i)
    dvmul(qa,  wa,  1,  in+i*qa, 1, H+i*qa, 1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qb; ++j)
      dscal(qa,    wb[j], H+i*qa*qb+j*qa, 1);
  for(i = 0; i < qc; ++i)
    dscal(qa*qb, wc[i], H+i*qa*qb, 1);

#if 1
  dgemm('T','N',L,qb*qc,qa,1.0,ba[0],qa,H,qa,0.0,out,L);

  for(i=0;i<qc;++i)
    dgemm('N','N',L,L,qb,1.,out+i*L*qb,L, bb[0], qb, 0.0, H+i*L*L,L);

  dgemm('N','N',L*L,L,qc,1.0, H, L*L, bc[0], qc, 0.0, out, L*L);

  // set to 1 on Apr 28
  // set to 0 on Apr 29
#if 0
  for(i=0;i<L;++i)
    for(j=0;j<L;++j)
      for(k=L-i-j;k<L;++k)
    out[i*L*L+j*L+k ] = 0.;
#endif
#ifdef NONORM
  for(i=0;i<L;++i)
    for(j=0;j<L;++j)
      for(k=0;k<L;++k)
    out[i*L*L+j*L+k] =
      out[i*L*L+j*L+k]*0.5*(2.0*i+1.0)*0.5*(2.0*j+1.)*0.5*(2.0*k+1.);
#endif
#else
  // reduced transform

  // do 'a' integration first

  dgemm('T','N',L,qb*qc,qa,1.0,ba[0],qa,H,qa,0.0,out,L);

  dzero(qtot, H, 1);
  // do 'b' integration second
  for (j = 0; j < qc; ++j)
    for (i= 0; i < L; ++i)
      dgemv('T', qb, L-i, 1., bb[0], qb, out+i+j*L*qb, L, 0.0,  H+i+j*L*L, L);

  dzero(qtot, out, 1);
  // do 'c' integration third
  for (j = 0; j < L; ++j)
    for (i= 0; i < L-j; ++i)
      dgemv('T', qc, L-i-j, 1., bc[0], qc, H+i+j*L, L*L, 0.0, out+i+j*L, L*L);
#endif
}




void Element::Ofwd(double*, double*, int, int){ERR;}
void Element::Ofwd(double*, double*, int){ERR;}


/*

Function name: Element::JtransEdge

Function Purpose:

Argument 1: Bndry *B
Purpose:

Argument 2: int edge_id
Purpose:

Argument 3: int loc
Purpose:

Argument 4: double *f
Purpose:

Function Notes:

*/

void Tri::JtransEdge(Bndry *B, int edge_id, int loc, double *f){
  register int i;
  const    int L = edge[edge_id].l;
  int      q,info;
  double  *imat,f1,f2,*z,*w,*s;
  Basis   *Base;
  Mode    *m;

  if(!L) return;
  Base = getbasis();

  f1 = B->bvert[0];
  f2 = B->bvert[1];

  s  = B->bedge[loc];

  /* calculate inner product with basis */
  switch(edge_id){
  case 0:
    q = qa-2;
    getzw(qa,&z,&w,'a');

    /* subtract vertex contributions */
    m = Base->vert;
    daxpy(q,-f1,m[0].a+1,1,f+1,1);
    daxpy(q,-f2,m[1].a+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);

    m = Base->edge[0];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].a+1,1,f+1,1);
    break;
  case 1: case 2:
    q = qb-1;
    getzw(qb,&z,&w,'b');

    /* subtract vertex contributions */
    m = Base->vert+1;
    daxpy(q,-f1,m[0].b+1,1,f+1,1);
    daxpy(q,-f2,m[1].b+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);
    dvdiv(q,f+1,1,m[0].b+1,1,f+1,1);/*correct for (1-b)/2 factor in weights*/

    m = Base->edge[1];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].b+1,1,f+1,1);
    break;
  }
  get_mmat1d(&imat, L);
  if(L>3)
    dpbtrs('L',L,2,1,imat,3,s,L,info);
  else
    dpptrs('L',L,1,imat,s,L,info);
}


void Quad::JtransEdge(Bndry *B, int edge_id, int loc, double *f){
  register int i;
  const    int L = edge[edge_id].l;
  int      q,info;
  double  *imat,f1,f2,*z,*w,*s;
  Basis   *Base;
  Mode    *m;

  loc = loc; // compiler fix

  if(!L) return;
  Base = getbasis();

  f1 = B->bvert[0];
  f2 = B->bvert[1];

  s  = B->bedge[0];

  /* calculate inner product with basis */
  switch(edge_id){
  case 0: case 2:
    q = qa-2;
    getzw(qa,&z,&w,'a');

    /* subtract vertex contributions */
    m = Base->vert;
    daxpy(q,-f1,m[0].a+1,1,f+1,1);
    daxpy(q,-f2,m[1].a+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);

    m = Base->edge[0];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].a+1,1,f+1,1);
    break;
  case 1: case 3:
    q = qb-2;
    getzw(qb,&z,&w,'a');

    /* subtract vertex contributions */
    m = Base->vert+1;
    daxpy(q,-f1,m[0].b+1,1,f+1,1);
    daxpy(q,-f2,m[2].b+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);

    m = Base->edge[1];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].b+1,1,f+1,1);
    break;
  }
  get_mmat1d(&imat, L);
  if(L>3)
    dpbtrs('L',L,2,1,imat,3,s,L,info);
  else
    dpptrs('L',L,1,imat,s,L,info);
}




void Tet::JtransEdge(Bndry *B, int edge_id, int loc, double *f){
  register int i;
  const    int L = edge[edge_id].l;
  int      q,info;
  double  *imat,f1,f2,*z,*w,*s;
  Basis   *Base;
  Mode    *m;

  if(!L) return;
  Base = getbasis();
  f1 = B->bvert[loc%2];
  f2 = (loc == 2)? B->bvert[2]: B->bvert[loc+1];

  s = B->bedge[loc];

  /* calculate inner product with basis */
  switch(edge_id){
  case 0:
    q = qa-2;
    getzw(qa,&z,&w,'a');

    /* subtract vertex conTetbutions */
    m = Base->vert;
    daxpy(q,-f1,m[0].a+1,1,f+1,1);
    daxpy(q,-f2,m[1].a+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);

    m = Base->edge[0];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].a+1,1,f+1,1);
    break;
  case 1: case 2:
    q = qb-1;
    getzw(qb,&z,&w,'b');

    /* subtract vertex conTetbutions */
    m = Base->vert+1;
    daxpy(q,-f1,m[0].b+1,1,f+1,1);
    daxpy(q,-f2,m[1].b+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);
    dvdiv(q,f+1,1,m[0].b+1,1,f+1,1);/*correct for (1-b)/2 factor in weights*/

    m = Base->edge[1];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].b+1,1,f+1,1);
    break;
  case 3: case 4: case 5:
    q = qc-1;
    getzw(qc,&z,&w,'c');

    /* subtract vertex contributions */
    m = Base->vert+2;
    daxpy(q,-f1,m[0].c+1,1,f+1,1);
    daxpy(q,-f2,m[1].c+1,1,f+1,1);

    dvmul(q,f+1,1,w+1,1,f+1,1);
    dvdiv(q,f+1,1,m[0].c+1,1,f+1,1);/*correct for (1-c)^2/2 factor in weights*/
    dvdiv(q,f+1,1,m[0].c+1,1,f+1,1);

    m = Base->edge[3];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].c+1,1,f+1,1);
    break;
  }
  get_mmat1d(&imat, L);
  if(L>3)
    dpbtrs('L',L,2,1,imat,3,s,L,info);
  else
    dpptrs('L',L,1,imat,s,L,info);
}

void Pyr::JtransEdge(Bndry *B, int edge_id, int loc, double *f){
  register int i;
  const    int L = edge[edge_id].l;
  int      q,info;
  double  *imat,*z,*w,*s;
  Basis   *Base;
  Mode    *m;

  if(!L) return;
  Base = getbasis();

  s = B->bedge[loc];

  /* calculate inner product with basis */
  switch(edge_id){
  case 0: case 1: case 2: case 3:
    q = qa-2;
    getzw(qa,&z,&w,'a');

    dvmul(q,f+1,1,w+1,1,f+1,1);

    m = Base->edge[0];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].a+1,1,f+1,1);
    break;
  case 4: case 5: case 6: case 7:
    q = qc-1;
    getzw(qc,&z,&w,'c');

    m = Base->vert;
    dvmul(q,f+1,1,w+1,1,f+1,1);
    dvdiv(q,f+1,1,m[0].c+1,1,f+1,1);/*correct for (1-c)^2/2 factor in weights*/
    dvdiv(q,f+1,1,m[0].c+1,1,f+1,1);

    m = Base->edge[4];
    for(i = 0; i < L; ++i)
      s[i] = ddot(q,m[i].c+1,1,f+1,1);
    break;
  }
  get_mmat1d(&imat, L);

  dpptrs('L',L,1,imat,s,L,info);
}




void Prism::JtransEdge(Bndry *B, int edge_id, int loc, double *f){
  register int i;
  const    int L = edge[edge_id].l;
  int      q,info;
  double  *imat,*z,*w,*s;
  Basis   *Base;
  Mode    *m;

  if(!L) return;
  Base = getbasis();

  s = B->bedge[loc];
  /* calculate inner product with basis */
  q = qa-2;
  getzw(qa,&z,&w,'a');

  dvmul(q,f+1,1,w+1,1,f+1,1);

  m = Base->edge[0];
  for(i = 0; i < L; ++i)
    s[i] = ddot(q,m[i].a+1,1,f+1,1);

  get_mmat1d(&imat, L);

  dpptrs('L',L,1,imat,s,L,info);
}




void Hex::JtransEdge(Bndry *B, int edge_id, int loc, double *f){
  register int i;
  const    int L = edge[edge_id].l;
  int      q,info;
  double  *imat,*z,*w,*s;
  Basis   *Base;
  Mode    *m;

  if(!L) return;
  Base = getbasis();

  s = B->bedge[loc];
  /* calculate inner product with basis */
  q = qa-2;
  getzw(qa,&z,&w,'a');

  dvmul(q,f+1,1,w+1,1,f+1,1);

  m = Base->edge[0];
  for(i = 0; i < L; ++i)
    s[i] = ddot(q,m[i].a+1,1,f+1,1);

  get_mmat1d(&imat, L);

  if(L>3)
    dpbtrs('L',L,2,1,imat,3,s,L,info);
  else
    dpptrs('L',L,1,imat,s,L,info);
}




void Element::JtransEdge(Bndry *, int , int , double *){ERR;}



void Tet_get_mmat2d(double **mat, int L, Element *E);
void Pyr_get_tri_mmat2d(double **mat, int L, Element *E);
void Pyr_get_quad_mmat2d(double **mat, int L, Element *E);
void Hex_get_mmat2d(double **mat, int L, Element *E);

void Prism_Get_Quad_Edge(int qa, int qb, double *from, int fac, double *to);
void Prism_get_tri_mmat2d(double **mat, int L, Element *E);
void Prism_get_quad_mmat2d(double **mat, int L, Element *E);
static void getlm(int p, int *i, int *j, int l);
void Pyr_JtransFace_Tri(Bndry *B, double *f);
void Pyr_JtransFace_Quad(Bndry *B, double *f);
void Prism_JtransFace_Quad(Bndry *B, double *f);
void Prism_JtransFace_Tri(Bndry *B, double *f);
void Hex_GetEdge(int qa, int qb, double *from, int fac, double *to);
void Tet_faceMode(Element *E, int face, Mode *v, double *f);
void Pyr_faceMode(Element *E, int face, Mode *v, double *f);
void Prism_faceMode(Element *E, int face, Mode *v, double *f);
void Hex_faceMode(Element *E, int face, Mode *v, double *f);

/*

Function name: Element::JtransFace

Function Purpose:

Argument 1: Bndry *
Purpose:

Argument 2: double *
Purpose:

Function Notes:

*/


void Tri::JtransFace(Bndry *, double *){
}


void Quad::JtransFace(Bndry *, double *){
}


/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem */

void Tet::JtransFace(Bndry *B, double *f){
  register int i,j;
  const    int fac = B->face;
  const    int L = B->elmt->face[fac].l;
  int      q1,q2,info,ll;
  double   *w1,*w2,*z,*tmp,*tmp1,**s,*imat;
  Basis    *Base;
  Mode     *m,**m1;
  Element  *E = B->elmt;

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax-1);

  switch(fac){
  case 0:
    q1 = E->qa;
    q2 = E->qb;

    /* set vertices (top vertex already set) */
    B->bvert[0] = f[0];
    B->bvert[1] = f[q1-1];

    /* transform sides */
    JtransEdge(B,0,0,f);
    dcopy(q2,f+q1-1,q1,tmp,1);
    JtransEdge(B,1,1,tmp);
    dcopy(q2,f ,q1,tmp,1);
    JtransEdge(B,2,2,tmp);
    break;
  case 1:
    q1 = E->qa;
    q2 = E->qc;

    /* set vertices (top vertex already set) */
    B->bvert[0] = f[0];
    B->bvert[1] = f[q1-1];

    /* transform sides */
    JtransEdge(B,0,0,f);
    dcopy(q2,f+q1-1,q1,tmp,1);
    JtransEdge(B,4,1,tmp);
    dcopy(q2,f ,q1,tmp,1);
    JtransEdge(B,3,2,tmp);

    break;
  case 2:
    q1 = E->qb;
    q2 = E->qc;

    /* set vertices (top vertex already set) */
    B->bvert[0] = f[0];
    B->bvert[1] = f[q1*q2];

    /* transform sides */
    JtransEdge(B,1,0,f);
    JtransEdge(B,5,1,f+q1*q2);
    dcopy(q2,f,q1,tmp,1);
    JtransEdge(B,4,2,tmp);

    break;
  case 3:
    q1 = E->qb;
    q2 = E->qc;

    /* set vertices (top vertex already set) */
    B->bvert[0] = f[0];
    B->bvert[1] = f[q1*q2];

    /* transform sides */
    JtransEdge(B,2,0,f);
    JtransEdge(B,5,1,f+q1*q2);
    dcopy(q2,f,q1,tmp,1);
    JtransEdge(B,3,2,tmp);

    break;
  }

  if(!L){free(tmp); return;}
  Base = E->getbasis();

  /* calculate inner product with basis of face*/
  switch(fac){
  case 0:
    getzw(q1,&z,&w1,'a');
    getzw(q2,&z,&w2,'b');

    s    = B->bface;
    tmp1 = dvector(0,max(q1,q2)-1);

    /* subtract off side 1 contribution from face */
    m  = Base->edge[0];
    ll = E->edge[0].l;
    for(i = 0; i < ll; ++i){
      dsmul(q1-2,B->bedge[0][i],m[i].a+1,1,tmp,1);
      for(j = 1; j < q1-1; ++j)
  daxpy(q2-1,-tmp[j-1],m[i].b+1,1,f+q1+j,q1);
    }

    /* subtract off side 2 contribution from face */
    m = Base->vert+1;
    dsmul(q2-1,B->bvert[1],m[0].b+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].b+1,1,tmp,1);
    m = Base->edge[1];
    ll = E->edge[1].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[1][i],m[i].b+1,1,tmp,1);
    m = Base->vert+1;
    for(i=0;i<q2-1;++i) daxpy(q1-2,-tmp[i],m[0].a+1,1,f+q1*(i+1)+1,1);

    /* subtract off side 3 contribution from face */
    m = Base->vert+1;
    dsmul(q2-1,B->bvert[0],m[0].b+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].b+1,1,tmp,1);
    m = Base->edge[2];
    ll = E->edge[2].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[2][i],m[i].b+1,1,tmp,1);
    m = Base->vert;
    for(i=0;i<q2-1;++i) daxpy(q1-2,-tmp[i],m[0].a+1,1,f+q1*(i+1)+1,1);

    /* finally take inner product */
    m1 = Base->face[0];
    for(i = 0; i < L; ++i){
      dvmul(q1-2,w1+1,1,m1[i][0].a+1,1,tmp,1);
      for(j = 1; j < q2; ++j) tmp1[j-1] = ddot(q1-2,tmp,1,f+q1*j+1,1);

      dvmul(q2-1,w2+1,1,tmp1,1,tmp1,1);
      for(j = 0; j < L-i; ++j)
  s[i][j] = ddot(q2-1,m1[i][j].b+1,1,tmp1,1);
    }
    free(tmp1);
    break;
  case 1:
    getzw(q1,&z,&w1,'a');
    getzw(q2,&z,&w2,'c');

    s    = B->bface;
    tmp1 = dvector(0,QGmax-1);

    /* subtract off side 1 contribution from face */
    m  = Base->edge[0];
    ll = E->edge[0].l;
    for(i = 0; i < ll; ++i){
      dsmul(q1-2,B->bedge[0][i],m[i].a+1,1,tmp,1);
      for(j = 1; j < q1-1; ++j)
  daxpy(q2-1,-tmp[j-1],m[i].c+1,1,f+q1+j,q1);
    }

    /* subtract off side 2 contribution from face */
    m = Base->vert+2;
    dsmul(q2-1,B->bvert[1],m[0].c+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].c+1,1,tmp,1);
    m  = Base->edge[4];
    ll = E->edge[4].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[1][i],m[i].c+1,1,tmp,1);
    m = Base->vert+1;
    for(i=0;i<q2-1;++i) daxpy(q1-2,-tmp[i],m[0].a+1,1,f+q1*(i+1)+1,1);

    /* subtract off side 3 contribution from face */
    m = Base->vert+2;
    dsmul(q2-1,B->bvert[0],m[0].c+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].c+1,1,tmp,1);
    m = Base->edge[3];
    ll = E->edge[3].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[2][i],m[i].c+1,1,tmp,1);
    m = Base->vert;
    for(i=0;i<q2-1;++i) daxpy(q1-2,-tmp[i],m[0].a+1,1,f+q1*(i+1)+1,1);

    /* finally take inner product */
    m1 = Base->face[1];
    m  = Base->vert;
    for(i = 0; i < L; ++i){
      dvmul(q1-2,w1+1,1,m1[i][0].a+1,1,tmp,1);
      for(j = 1; j < q2; ++j) tmp1[j-1] = ddot(q1-2,tmp,1,f+q1*j+1,1);

      dvmul(q2-1,w2+1,1,tmp1,1,tmp1,1);
      dvdiv(q2-1,tmp1,1,m[0].c+1,1,tmp1,1);
      for(j = 0; j < L-i; ++j)
  s[i][j] = ddot(q2-1,m1[i][j].c+1,1,tmp1,1);
    }
    free(tmp1);
    break;
  case 2:
    getzw(q1,&z,&w1,'b');
    getzw(q2,&z,&w2,'c');

    s    = B->bface;
    tmp1 = dvector(0,max(q1,q2)-1);

    /* subtract off side 1 contribution from face */
    m  = Base->edge[1];
    ll = E->edge[1].l;
    for(i = 0; i < ll; ++i){
      dsmul(q1-1,B->bedge[0][i],m[i].b+1,1,tmp,1);
      for(j = 1; j < q1; ++j)
  daxpy(q2-1,-tmp[j-1],m[i].c+1,1,f+q1+j,q1);
    }

    /* subtract off side 2 contribution from face */
    m = Base->vert+2;
    dsmul(q2-1,B->bvert[1],m[0].c+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].c+1,1,tmp,1);
    m  = Base->edge[5];
    ll = E->edge[5].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[1][i],m[i].c+1,1,tmp,1);
    m = Base->vert+2;
    for(i=0;i<q2-1;++i) daxpy(q1-1,-tmp[i],m[0].b+1,1,f+q1*(i+1)+1,1);

    /* subtract off side 3 contribution from face */
    m = Base->vert+2;
    dsmul(q2-1,B->bvert[0],m[0].c+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].c+1,1,tmp,1);
    m = Base->edge[4];
    ll = E->edge[4].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[2][i],m[i].c+1,1,tmp,1);
    m = Base->vert+1;
    for(i=0;i<q2-1;++i) daxpy(q1-1,-tmp[i],m[0].b+1,1,f+q1*(i+1)+1,1);

    /* finally take inner product */
    m1 = Base->face[2];
    m  = Base->vert;
    for(i = 0; i < L; ++i){
      dvmul(q1-1,w1+1,1,m1[i][0].b+1,1,tmp,1);
      dvdiv(q1-1,tmp,1,m[0].b+1,1,tmp,1);
      for(j = 1; j < q2; ++j) tmp1[j-1] = ddot(q1-1,tmp,1,f+q1*j+1,1);

      dvmul(q2-1,w2+1,1,tmp1,1,tmp1,1);
      dvdiv(q2-1,tmp1,1,m[0].c+1,1,tmp1,1);
      for(j = 0; j < L-i; ++j)
  s[i][j] = ddot(q2-1,m1[i][j].c+1,1,tmp1,1);
    }
    free(tmp1);
    break;
  case 3:
    getzw(q1,&z,&w1,'b');
    getzw(q2,&z,&w2,'c');

    s    = B->bface;
    tmp1 = dvector(0,max(q1,q2)-1);

    /* subtract off side 1 contribution from face */
    m  = Base->edge[2];
    ll = E->edge[2].l;
    for(i = 0; i < ll; ++i){
      dsmul(q1-1,B->bedge[0][i],m[i].b+1,1,tmp,1);
      for(j = 1; j < q1; ++j)
  daxpy(q2-1,-tmp[j-1],m[i].c+1,1,f+q1+j,q1);
    }

    /* subtract off side 2 contribution from face */
    m = Base->vert+2;
    dsmul(q2-1,B->bvert[1],m[0].c+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].c+1,1,tmp,1);
    m  = Base->edge[5];
    ll = E->edge[5].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[1][i],m[i].c+1,1,tmp,1);
    m = Base->vert+2;
    for(i=0;i<q2-1;++i) daxpy(q1-1,-tmp[i],m[0].b+1,1,f+q1*(i+1)+1,1);

    /* subtract off side 3 contribution from face */
    m = Base->vert+2;
    dsmul(q2-1,B->bvert[0],m[0].c+1,1,tmp,1);
    daxpy(q2-1,B->bvert[2],m[1].c+1,1,tmp,1);
    m = Base->edge[3];
    ll = E->edge[3].l;
    for(i=0;i<ll;++i) daxpy(q2-1,B->bedge[2][i],m[i].c+1,1,tmp,1);
    m = Base->vert+1;
    for(i=0;i<q2-1;++i) daxpy(q1-1,-tmp[i],m[0].b+1,1,f+q1*(i+1)+1,1);

    /* finally take inner product */
    m1 = Base->face[3];
    m  = Base->vert;
    for(i = 0; i < L; ++i){
      dvmul(q1-1,w1+1,1,m1[i][0].b+1,1,tmp,1);
      dvdiv(q1-1,tmp,1,m[0].b+1,1,tmp,1);
      for(j = 1; j < q2; ++j) tmp1[j-1] = ddot(q1-1,tmp,1,f+q1*j+1,1);

      dvmul(q2-1,w2+1,1,tmp1,1,tmp1,1);
      dvdiv(q2-1,tmp1,1,m[0].c+1,1,tmp1,1);
      for(j = 0; j < L-i; ++j)
  s[i][j] = ddot(q2-1,m1[i][j].c+1,1,tmp1,1);
    }
    free(tmp1);
    break;
  }
  free(tmp);

  Tet_get_mmat2d(&imat,L,E);
  if(L>3)
    dpbtrs('L',L*(L+1)/2,L+(L-1)+1,1,imat,L+(L-1)+2,*s,L*(L+1)/2,info);
  else
    dpptrs('L',L*(L+1)/2,1,imat,*s,L*(L+1)/2,info);
}


/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem */

void Pyr::JtransFace(Bndry *B, double *f){

  if(Nfverts(B->face) == 3)
    Pyr_JtransFace_Tri(B,f);
  else
    Pyr_JtransFace_Quad(B,f);


}

/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem */

void Prism::JtransFace(Bndry *B, double *f){

  if(Nfverts(B->face) == 3)
    Prism_JtransFace_Tri(B,f);
  else
    Prism_JtransFace_Quad(B,f);


}



/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem */

void Hex::JtransFace(Bndry *B, double *f){
  register int i,j,k;
  const    int fac = B->face;
  const    int L = B->elmt->face[fac].l;
  int      q1,q2,info;
  double   *w1,*w2,*z,*tmp,**s,*imat;
  Basis    *Base;
  Element  *E = B->elmt;

  Base = E->getbasis();

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax*QGmax-1);

  q1 = E->qa;
  q2 = E->qb;

  getzw(q1,&z,&w1,'a');
  getzw(q2,&z,&w2,'a');

  /* set vertices (top vertex already set) */
  B->bvert[0] = f[0];
  B->bvert[1] = f[q1-1];
  B->bvert[2] = f[q1*q2-1];
  B->bvert[3] = f[q1*(q2-1)];

  // subtract vertex modes from face
  for(j=0;j<4;++j){
    Hex_faceMode(this, 0, Base->vert+j, tmp);
    daxpy(q1*q2, -B->bvert[j], tmp, 1, f, 1);
  }

  /* transform sides */
  for(j=0;j<4;++j){
    Hex_GetEdge(q1,q2,f, j, tmp);
    JtransEdge(B, j, j, tmp);
  }

  for(j=0;j<4;++j)
    for(i = 0; i < E->edge[ednum(fac,j)].l; ++i){
      Hex_faceMode(this, 0, Base->edge[j]+i, tmp);
      daxpy(q1*q2, -B->bedge[j][i], tmp, 1, f, 1);
    }


  if(L){
    /* calculate inner product with basis of face*/
    s    = B->bface;

    /* finally take inner product */
    for(i = 0; i < L; ++i)
      for(j = 0; j < L; ++j){
  Hex_faceMode(this, 0, Base->face[0][i]+j, tmp);
  for(k = 0; k < q2; ++k) dvmul(q1,w1,1, tmp+k*q1,1, tmp+k*q1,1);
  for(k = 0; k < q2; ++k) dscal(q1,w2[k],tmp+k*q1,1);

  s[i][j] = ddot(q1*q2, tmp, 1, f, 1);
      }
  }

  free(tmp);

  Hex_get_mmat2d(&imat,L,E);
  /*
  if(L>3)
    dpbtrs('L',L*L,L+(L-1)+1,1,imat,L+(L-1)+2,*s,L*L,info);
  else
  */
  if(L)
    dpptrs('L',L*L,1,imat,*s,L*L,info);
}


void Element::JtransFace(Bndry *, double *){ERR;}




static MMinfo *Tet_m2inf,*Tet_m2base;
static MMinfo *Pyr_tri_m2inf,*Pyr_tri_m2base;
static MMinfo *Pyr_quad_m2inf,*Pyr_quad_m2base;
static MMinfo *Prism_tri_m2inf,*Prism_tri_m2base;
static MMinfo *Prism_quad_m2inf,*Prism_quad_m2base;
static MMinfo *Hex_m2inf,*Hex_m2base;


void Tet_get_mmat2d(double **mat, int L, Element *E){

  /* check link list */

  for(Tet_m2inf = Tet_m2base; Tet_m2inf; Tet_m2inf = Tet_m2inf->next)
    if(Tet_m2inf->L == L){
      *mat = Tet_m2inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Tet_m2inf  = Tet_m2base;
  Tet_m2base = Tet_addmmat2d(L,E);
  Tet_m2base->next = Tet_m2inf;

  *mat = Tet_m2base->mat;

  return;
}


void Hex_get_mmat2d(double **mat, int L, Element *E){

  /* check link list */

  for(Hex_m2inf = Hex_m2base; Hex_m2inf; Hex_m2inf = Hex_m2inf->next)
    if(Hex_m2inf->L == L){
      *mat = Hex_m2inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Hex_m2inf  = Hex_m2base;
  Hex_m2base = Hex_addmmat2d(L,E);
  Hex_m2base->next = Hex_m2inf;

  *mat = Hex_m2base->mat;

  return;
}


void Pyr_get_tri_mmat2d(double **mat, int L, Element *E){

  /* check link list */

  for(Pyr_tri_m2inf = Pyr_tri_m2base; Pyr_tri_m2inf; Pyr_tri_m2inf = Pyr_tri_m2inf->next)
    if(Pyr_tri_m2inf->L == L){
      *mat = Pyr_tri_m2inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Pyr_tri_m2inf  = Pyr_tri_m2base;
  Pyr_tri_m2base = Pyr_tri_addmmat2d(L,E);
  Pyr_tri_m2base->next = Pyr_tri_m2inf;

  *mat = Pyr_tri_m2base->mat;

  return;
}



void Pyr_get_quad_mmat2d(double **mat, int L, Element *E){

  /* check link list */

  for(Pyr_quad_m2inf = Pyr_quad_m2base; Pyr_quad_m2inf; Pyr_quad_m2inf = Pyr_quad_m2inf->next)
    if(Pyr_quad_m2inf->L == L){
      *mat = Pyr_quad_m2inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Pyr_quad_m2inf  = Pyr_quad_m2base;
  Pyr_quad_m2base = Pyr_quad_addmmat2d(L,E);
  Pyr_quad_m2base->next = Pyr_quad_m2inf;

  *mat = Pyr_quad_m2base->mat;

  return;
}



void Pyr_Get_Quad_Edge(int qa, int qb, double *from, int fac, double *to){
  switch(fac){
  case 0:
    dcopy(qa,        from, 1,  to, 1);
    break;
  case 1:
    dcopy(qb, from + qa-1, qa, to, 1);
    break;
  case 2:
    dcopy(qa, from + qa*(qb-1), 1, to, 1);
    break;
  case 3:
    dcopy(qb,        from, qa, to, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}



void Prism_Get_Quad_Edge(int qa, int qb, double *from, int fac, double *to){
  switch(fac){
  case 0:
    dcopy(qa,        from, 1,  to, 1);
    break;
  case 1:
    dcopy(qb, from + qa-1, qa, to, 1);
    break;
  case 2:
    dcopy(qa, from + qa*(qb-1), 1, to, 1);
    break;
  case 3:
    dcopy(qb,        from, qa, to, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}


void Prism_get_tri_mmat2d(double **mat, int L, Element *E){

  /* check link list */

  for(Prism_tri_m2inf = Prism_tri_m2base; Prism_tri_m2inf; Prism_tri_m2inf = Prism_tri_m2inf->next)
    if(Prism_tri_m2inf->L == L){
      *mat = Prism_tri_m2inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Prism_tri_m2inf  = Prism_tri_m2base;
  Prism_tri_m2base = Prism_tri_addmmat2d(L,E);
  Prism_tri_m2base->next = Prism_tri_m2inf;

  *mat = Prism_tri_m2base->mat;

  return;
}



void Prism_get_quad_mmat2d(double **mat, int L, Element *E){

  /* check link list */

  for(Prism_quad_m2inf = Prism_quad_m2base; Prism_quad_m2inf;
      Prism_quad_m2inf = Prism_quad_m2inf->next)
    if(Prism_quad_m2inf->L == L){
      *mat = Prism_quad_m2inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Prism_quad_m2inf  = Prism_quad_m2base;
  Prism_quad_m2base = Prism_quad_addmmat2d(L,E);
  Prism_quad_m2base->next = Prism_quad_m2inf;

  *mat = Prism_quad_m2base->mat;

  return;
}


static void getlm(int p, int *i, int *j, int l){
  int cnt = 0;

  i[0]=0;
  while(p-cnt >= l-i[0]){
    cnt += l-i[0];
    i[0]++;
  }
  j[0] = p - i[0]*(2*l-i[0]+1)/2;
}

static MMinfo *Tet_addmmat2d(int L, Element *E){
  register int i,j,k;
  const    int qa = E->qa,qb = E->qb;
  int      info,ll,l,m;
  double  *tmp,*tmp1,*wa,*wb;
  Basis   *B = E->getbasis();
  Mode   **mf;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));

  M->L = L;
  getzw(qa,&tmp,&wa,'a');
  getzw(qb,&tmp,&wb,'b');
  tmp  = dvector(0,qa-1);
  tmp1 = dvector(0,qb-1);
  mf   = B->face[0];
  ll   = L*(L+1)/2;

  if(L>3){ /* banded matrix */
    int bw = L+(L-1)+2;
    M->mat = dvector(0,ll*bw-1);

    for(i = 0; i < ll-bw; ++i){
      getlm(i,&l,&m,L);
      dvmul(qa-2,wa+1,1,mf[l][m].a+1,1,tmp ,1);
      dvmul(qb-1,wb+1,1,mf[l][m].b+1,1,tmp1,1);
      for(j = i; j < i + bw; ++j){
  getlm(j,&l,&m,L);
  M->mat[i*bw+(j-i)] = ddot(qa-2,tmp ,1,mf[l][m].a+1,1)*
    ddot(qb-1,tmp1,1,mf[l][m].b+1,1);
      }
    }
    for(i = ll-bw; i < ll; ++i){
      getlm(i,&l,&m,L);
      dvmul(qa-2,wa+1,1,mf[l][m].a+1,1,tmp ,1);
      dvmul(qb-1,wb+1,1,mf[l][m].b+1,1,tmp1,1);
      for(j = i; j < ll; ++j) {
  getlm(j,&l,&m,L);
  M->mat[i*bw+(j-i)] = ddot(qa-2,tmp ,1,mf[l][m].a+1,1)*
    ddot(qb-1,tmp1,1,mf[l][m].b+1,1);
      }
    }
    dpbtrf('L',ll,bw-1,M->mat,bw,info);
  }
  else { /* symmetric */
    M->mat = dvector(0,ll*(ll+1)/2-1);

    for(i = 0, k=0; i < ll; ++i){
      getlm(i,&l,&m,L);
      dvmul(qa-2,wa+1,1,mf[l][m].a+1,1,tmp ,1);
      dvmul(qb-1,wb+1,1,mf[l][m].b+1,1,tmp1,1);
      for(j = i; j < ll; ++j,++k){
  getlm(j,&l,&m,L);
  M->mat[k] = ddot(qa-2,tmp ,1,mf[l][m].a+1,1)*
    ddot(qb-1,tmp1,1,mf[l][m].b+1,1);
      }
    }
    dpptrf('L',ll,M->mat,info);
  }

  if(info) error_msg(Tet_addmmat2d: info not zero);

  free(tmp); free(tmp1);
  return M;
}

static MMinfo *Pyr_tri_addmmat2d(int L, Element *E){
  register int i,j,k;
  const    int qa = E->qa,qc = E->qc;
  int      info,ll,l;
  double  *tmp,*tmp1,*wa,*wb;
  Basis   *B = E->getbasis();
  Mode   **mf;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));

  M->L = L;
  getzw(qa,&tmp,&wa,'a');
  getzw(qc,&tmp,&wb,'c');

  tmp  = dvector(0,qa-1);
  tmp1 = dvector(0,qc-1);
  mf   = B->face[1];
  ll   = L*(L+1)/2;

  M->mat = dvector(0,ll*(ll+1)/2-1);

  int n = 0;
  int cnt =0, cnt1= 0;

  for(i = 0; i < L; ++i)
    for(j = 0; j < L-i; ++j,++cnt1){
      dvmul(qa,wa,1,mf[i][j].a,1,tmp ,1);
      dvmul(qc,wb,1,mf[i][j].c,1,tmp1,1);

      cnt = 0;
      for(k = 0; k < L; ++k)
        for(l = 0; l < L-k; ++l,++cnt)
          if(cnt >= cnt1){
            M->mat[n]  = ddot(qa,tmp ,1,mf[k][l].a,1);
            M->mat[n] *= ddot(qc,tmp1,1,mf[k][l].c,1);
            ++n;
          }
    }

  dpptrf('L',ll,M->mat,info);

  if(info) error_msg(Pyr_tri_addmmat2d: info not zero);

  free(tmp); free(tmp1);
  return M;
}


static MMinfo *Pyr_quad_addmmat2d(int L, Element *E){
  register int i,j,k;
  const    int qa = E->qa,qb = E->qb;
  int      info,ll,l;
  double  *tmp,*tmp1,*wa,*wb;
  Basis   *B = E->getbasis();
  Mode   **mf;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));

  M->L = L;
  getzw(qa,&tmp,&wa,'a');
  getzw(qb,&tmp,&wb,'a');

  tmp  = dvector(0,qa-1);
  tmp1 = dvector(0,qb-1);
  mf   = B->face[0];
  ll   = L*L;

  M->mat = dvector(0,ll*(ll+1)/2-1);

  int n = 0;
  for(i = 0; i < L; ++i)
    for(j = 0; j < L; ++j){
      dvmul(qa-2,wa+1,1,mf[i][j].a+1,1,tmp ,1);
      dvmul(qb-2,wb+1,1,mf[i][j].b+1,1,tmp1,1);

      for(k = 0; k < L; ++k)
  for(l = 0; l < L; ++l)
    if(k*L+l >= i*L+j){
      M->mat[n]  = ddot(qa-2,tmp ,1,mf[k][l].a+1,1);
      M->mat[n] *= ddot(qb-2,tmp1,1,mf[k][l].b+1,1);
      ++n;
    }
    }

  dpptrf('L',ll,M->mat,info);

  if(info) error_msg(addmmat2d: info not zero);

  free(tmp); free(tmp1);
  return M;
}




/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem
   */

// Assumes data is in standard 'a'x'c' triangle form

void Pyr_JtransFace_Tri(Bndry *B, double *f){
  register int i,j;
  const    int fac = B->face;
  const    int L = B->elmt->face[fac].l;
  int      qa,qc,qt,info,ll;
  double   *wa,*wc,*z,*tmp,**s,*imat;
  Basis    *Base;

  Element  *E = B->elmt;
  Base = E->getbasis();

  tmp  = dvector(0,QGmax*QGmax-1);

  qa = E->qa;
  qc = E->qc;

  qt = qa*qc;

  /* calculate inner product with basis of face*/
  getzw(qa,&z,&wa,'a');
  getzw(qc,&z,&wc,'c');

  // subtract vertices
  Pyr_faceMode(E,1,Base->vert,tmp);
  daxpy(qt, -B->bvert[0], tmp, 1, f, 1);

  Pyr_faceMode(E,1,Base->vert+1,tmp);
  daxpy(qt, -B->bvert[1], tmp, 1, f, 1);

  Pyr_faceMode(E,1,Base->vert+4,tmp);
  daxpy(qt, -B->bvert[2], tmp, 1, f, 1);

  /* transform sides */
  dcopy(qa,f,1,tmp,1);
  E->JtransEdge(B,0,0,tmp);

  // have to interptoedge1
  dcopy(qc,f+qa-1,qa,tmp,1);
  E->JtransEdge(B,E->ednum(fac,1),1,tmp);

  // have to interptoedge1
  dcopy(qc,f ,qa,tmp,1);
  E->JtransEdge(B,E->ednum(fac,1),2,tmp);

  if(!L){free(tmp); return;}

  /* subtract off edge 1 contribution from face */
  ll = E->edge[E->ednum(fac,0)].l;
  for(i = 0; i < ll; ++i){
    Pyr_faceMode(E,1,Base->edge[0]+i,tmp);
    daxpy(qt, -B->bedge[0][i], tmp, 1, f, 1);
  }

  /* subtract off edge 2 contribution from face */
  ll = E->edge[E->ednum(fac,1)].l;
  for(i = 0; i < ll; ++i){
    Pyr_faceMode(E,1,Base->edge[5]+i,tmp);
    daxpy(qt, -B->bedge[1][i], tmp, 1, f, 1);
  }

  /* subtract off edge 3 contribution from face */
  ll = E->edge[E->ednum(fac,2)].l;
  for(i = 0; i < ll; ++i){
    Pyr_faceMode(E,1,Base->edge[4]+i,tmp);
    daxpy(qt, -B->bedge[2][i], tmp, 1, f, 1);
  }

  /* finally take inner product */

  for(i=0;i<qc;++i) dvmul(qa, wa, 1, f+i*qa, 1, f+i*qa, 1);
  for(i=0;i<qa;++i) dvmul(qc, wc, 1, f+i, qa, f+i, qa);

  s    = B->bface;

  for(i = 0; i < L; ++i)
    for(j = 0; j < L-i; ++j){
      Pyr_faceMode(E,1,Base->face[1][i]+j,tmp);
      s[i][j] = ddot(qt,tmp,1,f,1);
  }

  free(tmp);

  Pyr_get_tri_mmat2d(&imat,L,E);

  dpptrs('L',L*(L+1)/2,1,imat,*s,L*(L+1)/2,info);
}


/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem */

// Assumes data is in standard 'a'x'a' quad form

void Pyr_JtransFace_Quad(Bndry *B, double *f){
  register int i,j,k;
  const    int fac = B->face;
  const    int L = B->elmt->face[fac].l;
  int      qa , qb,info;
  double   *w1,*w2,*z,*tmp,**s,*imat;
  Basis    *Base;
  Element  *E = B->elmt;

  Base = E->getbasis();

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax*QGmax-1);

  qa = E->qa;
  qb = E->qb;

  getzw(qa,&z,&w1,'a');
  getzw(qb,&z,&w2,'a');

  // subtract vertex modes from face
  for(j=0;j<4;++j){
    Pyr_faceMode(E, 0, Base->vert+j, tmp);
    daxpy(qa*qb, -B->bvert[j], tmp, 1, f, 1);
  }

  /* transform sides */
  for(j=0;j<4;++j){
    Pyr_Get_Quad_Edge(qa,qb,f, j, tmp);
    E->JtransEdge(B, j, j, tmp);
  }

  for(j=0;j<4;++j)
    for(i = 0; i < E->edge[E->ednum(fac,j)].l; ++i){
      Pyr_faceMode(E, 0, Base->edge[j]+i, tmp);
      daxpy(qa*qb, -B->bedge[j][i], tmp, 1, f, 1);
    }


  if(L){
    /* calculate inner product with basis of face*/

    for(k = 0; k < qb; ++k) dvmul(qa,w1,1, f+k*qa,1, f+k*qa,1);
    for(k = 0; k < qb; ++k) dscal(qa,w2[k],f+k*qa,1);

    s    = B->bface;

    /* finally take inner product */
    for(i = 0; i < L; ++i)
      for(j = 0; j < L; ++j){
  Pyr_faceMode(E, 0, Base->face[0][i]+j, tmp);

  s[i][j] = ddot(qa*qb, tmp, 1, f, 1);
      }
  }

  free(tmp);

  Pyr_get_quad_mmat2d(&imat,L,E);

  dpptrs('L',L*L,1,imat,*s,L*L,info);
}




// Assumes data is in standard 'a'x'a' quad form

void Prism_JtransFace_Quad(Bndry *B, double *f){
  register int i,j,k;
  const    int fac = B->face;
  const    int L = B->elmt->face[fac].l;
  int      qa , qb,info;
  double   *w1,*w2,*z,*tmp,**s,*imat;
  Basis    *Base;

  Element  *E = B->elmt;

  Base = E->getbasis();

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax*QGmax-1);

  qa = E->qa;
  qb = E->qb;

  getzw(qa,&z,&w1,'a');
  getzw(qb,&z,&w2,'a');

  if(f[0] != B->bvert[0])
    fprintf(stderr,"Warning: Prism::JtransFace vertices not set \n");


  // subtract vertex modes from face
  for(j=0;j<4;++j){
    Prism_faceMode(E, 0, Base->vert+j, tmp);
    daxpy(qa*qb, -B->bvert[j], tmp, 1, f, 1);
  }

  /* transform sides */
  for(j=0;j<4;++j){
    Prism_Get_Quad_Edge(qa,qb,f, j, tmp);
    E->JtransEdge(B, j, j, tmp);
  }

  for(j=0;j<4;++j)
    for(i = 0; i < E->edge[E->ednum(fac,j)].l; ++i){
      Prism_faceMode(E, 0, Base->edge[j]+i, tmp);
      daxpy(qa*qb, -B->bedge[j][i], tmp, 1, f, 1);
    }


  if(L){
    /* calculate inner product with basis of face*/

    for(k = 0; k < qb; ++k) dvmul(qa,w1,1, f+k*qa,1, f+k*qa,1);
    for(k = 0; k < qb; ++k) dscal(qa,w2[k],f+k*qa,1);

    s    = B->bface;

    /* finally take inner product */
    for(i = 0; i < L; ++i)
      for(j = 0; j < L; ++j){
  Prism_faceMode(E, 0, Base->face[0][i]+j, tmp);

  s[i][j] = ddot(qa*qb, tmp, 1, f, 1);
      }

    Prism_get_quad_mmat2d(&imat,L,E);
    dpptrs('L',L*L,1,imat,*s,L*L,info);
  }

  free(tmp);

}


/* this function tranforms the function given in f and puts the values
   into B using a H^{1/2} type method. It always assumes that the
   function is expressed in terms of the local co-ordinates and so for
   curved sides is evaluated with respect to the undeformed local
   co-ordinates. This does not appear to give any troubles but could
   be a potential problem
   */

// Assumes data is in standard 'a'x'b' triangle form

void Prism_JtransFace_Tri(Bndry *B, double *f){
  register int i,j;
  const    int fac = B->face;
  const    int L = B->elmt->face[fac].l;
  int      qa,qc,qt,info,ll;
  double   *wa,*wc,*z,*tmp,**s,*imat;
  Basis    *Base;

  Element  *E = B->elmt;
  Base = E->getbasis();

  tmp  = dvector(0,QGmax*QGmax-1);
  double *tmpa = dvector(0, QGmax-1);

  qa = E->qa;
  qc = E->qc;

  qt = qa*qc;

  /* calculate inner product with basis of face*/
  getzw(qa,&z,&wa,'a');
  getzw(qc,&z,&wc,'b');

  if(f[0] != B->bvert[0])
    fprintf(stderr,"Warning: Prism::JtransFace vertices not set \n");

  // subtract vertices
  Prism_faceMode(E,1,Base->vert,tmp);
  daxpy(qt, -B->bvert[0], tmp, 1, f, 1);

  Prism_faceMode(E,1,Base->vert+1,tmp);
  daxpy(qt, -B->bvert[1], tmp, 1, f, 1);

  Prism_faceMode(E,1,Base->vert+4,tmp);
  daxpy(qt, -B->bvert[2], tmp, 1, f, 1);
  //  FIX THIS ??
  /* transform sides */
  dcopy(qa,f,1,tmp,1);
  E->JtransEdge(B,0,0,tmp);

  // RECENT FIX TCEW

  double **im;
  getim(qc,qa,&im,b2a);

  // have to interptoedge1
  dcopy(qc,f+qa-1,qa,tmp,1);

  Interp(*im,tmp,qc,tmpa,qa);

  E->JtransEdge(B,1,1,tmpa);

  // have to interptoedge1
  dcopy(qc,f ,qa,tmp,1);
  Interp(*im,tmp,qc,tmpa,qa);
  E->JtransEdge(B,2,2,tmpa);

  free(tmpa);

  if(!L){free(tmp); return;}

  /* subtract off edge 1 contribution from face */
  ll = E->edge[E->ednum(fac,0)].l;
  for(i = 0; i < ll; ++i){
    Prism_faceMode(E,1,Base->edge[0]+i,tmp);
    daxpy(qt, -B->bedge[0][i], tmp, 1, f, 1);
  }

  /* subtract off edge 2 contribution from face */
  ll = E->edge[E->ednum(fac,1)].l;
  for(i = 0; i < ll; ++i){
    Prism_faceMode(E,1,Base->edge[5]+i,tmp);
    daxpy(qt, -B->bedge[1][i], tmp, 1, f, 1);
  }

  /* subtract off edge 3 contribution from face */
  ll = E->edge[E->ednum(fac,2)].l;
  for(i = 0; i < ll; ++i){
    Prism_faceMode(E,1,Base->edge[4]+i,tmp);
    daxpy(qt, -B->bedge[2][i], tmp, 1, f, 1);
  }

  /* finally take inner product */

  for(i=0;i<qc;++i) dvmul(qa, wa, 1, f+i*qa, 1, f+i*qa, 1);
  for(i=0;i<qa;++i) dvmul(qc, wc, 1, f+i, qa, f+i, qa);

  s    = B->bface;

  for(i = 0; i < L; ++i)
    for(j = 0; j < L-i; ++j){
      Prism_faceMode(E,1,Base->face[1][i]+j,tmp);
      s[i][j] = ddot(qt,tmp,1,f,1);
  }

  free(tmp);

  Prism_get_tri_mmat2d(&imat,L,E);

  dpptrs('L',L*(L+1)/2,1,imat,*s,L*(L+1)/2,info);
}

static MMinfo *Prism_tri_addmmat2d(int L, Element *E){
  register int i,j,k;
  const    int qa = E->qa,qc = E->qc;
  int      info,ll,l;
  double  *tmp,*tmp1,*wa,*wb;
  Basis   *B = E->getbasis();
  Mode   **mf;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));

  M->L = L;
  getzw(qa,&tmp,&wa,'a');
  getzw(qc,&tmp,&wb,'b');

  tmp  = dvector(0,qa-1);
  tmp1 = dvector(0,qc-1);
  mf   = B->face[1];
  ll   = L*(L+1)/2;

  M->mat = dvector(0,ll*(ll+1)/2-1);

  int n = 0;
  int cnt =0, cnt1= 0;

  for(i = 0; i < L; ++i)
    for(j = 0; j < L-i; ++j,++cnt1){
      dvmul(qa,wa,1,mf[i][j].a,1,tmp ,1);
      dvmul(qc,wb,1,mf[i][j].c,1,tmp1,1);

      cnt = 0;
      for(k = 0; k < L; ++k)
  for(l = 0; l < L-k; ++l,++cnt)
    if(cnt >= cnt1){
      M->mat[n]  = ddot(qa,tmp ,1,mf[k][l].a,1);
      M->mat[n] *= ddot(qc,tmp1,1,mf[k][l].c,1);
      ++n;
    }
    }


  dpptrf('L',ll,M->mat,info);

  if(info) error_msg(Prism_tri_addmmat2d: info not zero);

  free(tmp); free(tmp1);
  return M;
}

static MMinfo *Prism_quad_addmmat2d(int L, Element *E){
  register int i,j,k;
  const    int qa = E->qa,qb = E->qb;
  int      info,ll,l;
  double  *tmp,*tmp1,*wa,*wb;
  Basis   *B = E->getbasis();
  Mode   **mf;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));

  M->L = L;
  getzw(qa,&tmp,&wa,'a');
  getzw(qb,&tmp,&wb,'a');

  tmp  = dvector(0,qa-1);
  tmp1 = dvector(0,qb-1);
  mf   = B->face[0];
  ll   = L*L;

  M->mat = dvector(0,ll*(ll+1)/2-1);

  int n = 0;
  for(i = 0; i < L; ++i)
    for(j = 0; j < L; ++j){
      dvmul(qa-2,wa+1,1,mf[i][j].a+1,1,tmp ,1);
      dvmul(qb-2,wb+1,1,mf[i][j].b+1,1,tmp1,1);

      for(k = 0; k < L; ++k)
  for(l = 0; l < L; ++l)
    if(k*L+l >= i*L+j){
      M->mat[n]  = ddot(qa-2,tmp ,1,mf[k][l].a+1,1);
      M->mat[n] *= ddot(qb-2,tmp1,1,mf[k][l].b+1,1);
      ++n;
    }
    }

  dpptrf('L',ll,M->mat,info);

  if(info) error_msg(addmmat2d: info not zero);

  free(tmp); free(tmp1);
  return M;
}



static MMinfo *Hex_addmmat2d(int L, Element *E){
  register int i,j,k;
  const    int qa = E->qa,qb = E->qb;
  int      info,ll,l;
  double  *tmp,*tmp1,*wa,*wb;
  Basis   *B = E->getbasis();
  Mode   **mf;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));

  M->L = L;
  getzw(qa,&tmp,&wa,'a');
  getzw(qb,&tmp,&wb,'a');
  tmp  = dvector(0,qa-1);
  tmp1 = dvector(0,qb-1);
  mf   = B->face[0];
  ll   = L*L;


  M->mat = dvector(0,ll*(ll+1)/2-1);

  int n = 0;
  for(i = 0; i < L; ++i)
    for(j = 0; j < L; ++j){
      dvmul(qa-2,wa+1,1,mf[i][j].a+1,1,tmp ,1);
      dvmul(qb-2,wb+1,1,mf[i][j].b+1,1,tmp1,1);

      for(k = 0; k < L; ++k)
  for(l = 0; l < L; ++l)
    if(k*L+l >= i*L+j){
      M->mat[n]  = ddot(qa-2,tmp ,1,mf[k][l].a+1,1);
      M->mat[n] *= ddot(qb-2,tmp1,1,mf[k][l].b+1,1);
      ++n;
    }
    }

  dpptrf('L',ll,M->mat,info);

  if(info) error_msg(addmmat2d: info not zero);

  free(tmp); free(tmp1);
  return M;
}


void Hex_GetEdge(int qa, int qb, double *from, int fac, double *to){
  switch(fac){
  case 0:
    dcopy(qa,        from, 1,  to, 1);
    break;
  case 1:
    dcopy(qb, from + qa-1, qa, to, 1);
    break;
  case 2:
    dcopy(qa, from + qa*(qb-1), 1, to, 1);
    break;
  case 3:
    dcopy(qb,        from, qa, to, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}


void Tet_faceMode(Element *E, int face, Mode *v, double *f){
  register int i;
  const    int qa = E->qa, qb = E->qb, qc = E->qc;

  switch (face){
  case 0:
    for(i = 0; i < qb; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qb,v->b,1,f+i,qa,f+i,qa);
    break;
  case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qc,v->c,1,f+i,qa,f+i,qa);
    break;
  case 2: case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qb,v->b,1,f+i*qb,1);
    for(i = 0; i < qb; ++i)
      dvmul(qc,v->c,1,f+i,qb,f+i,qb);
    break;
  }
}


void Pyr_faceMode(Element *E, int face, Mode *v, double *f){
  register int i;
  const    int qa = E->qa, qb = E->qb, qc = E->qc;

  switch (face){
  case 0:
    for(i = 0; i < qb; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qb,v->b,1,f+i,qa,f+i,qa);
    break;
  case 1: case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qc,v->c,1,f+i,qa,f+i,qa);
    break;
  case 2: case 4:
    for(i = 0; i < qc; ++i)
      dcopy(qb,v->b,1,f+i*qb,1);
    for(i = 0; i < qb; ++i)
      dvmul(qc,v->c,1,f+i,qb,f+i,qb);
    break;
  }
}

void Prism_faceMode(Element *E, int face, Mode *v, double *f){
  register int i;
  const    int qa = E->qa, qb = E->qb, qc = E->qc;

  switch (face){
  case 0:
    for(i = 0; i < qb; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qb,v->b,1,f+i,qa,f+i,qa);
    break;
  case 1: case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qc,v->c,1,f+i,qa,f+i,qa);
    break;
  case 2: case 4:
    for(i = 0; i < qc; ++i)
      dcopy(qb,v->b,1,f+i*qb,1);
    for(i = 0; i < qb; ++i)
      dvmul(qc,v->c,1,f+i,qb,f+i,qb);
    break;
  }
}



void Hex_faceMode(Element *E, int face, Mode *v, double *f){
  register int i;
  const    int qa = E->qa, qb = E->qb, qc = E->qc;

  switch (face){
  case 5: case 0:
    for(i = 0; i < qb; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qb,v->b,1,f+i,qa,f+i,qa);
    break;
  case 3: case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa,v->a,1,f+i*qa,1);
    for(i = 0; i < qa; ++i)
      dvmul(qc,v->c,1,f+i,qa,f+i,qa);
    break;
  case 4: case 2:
    for(i = 0; i < qc; ++i)
      dcopy(qb,v->b,1,f+i*qb,1);
    for(i = 0; i < qb; ++i)
      dvmul(qc,v->c,1,f+i,qb,f+i,qb);
    break;
  }
}


/*
Function name: Element::Jbwdfac1

Function Purpose: Transform vector in store and return physical values
                   in terms of face 1 points

Argument 1: face
Purpose:

Argument 2: vj
Purpose:

Argument 2: v
Purpose:

Function Notes:
the routine backward transforms the vector vj of face coefficients
which is packed in the standard form based on a fixed lmax and returns
the solution into v
*/

void Tri::Jbwdfac1(int fac, double *vj, double *v){
  int      lm  = edge[fac].l+2;
  Basis    *b = getbasis();
  double   tmp;

  /* need to swap vj ordering  to be consistent with use of b->vert[1].a as
  starting point of backwards trans array  */
  tmp = vj[1];  vj[1] = vj[0]; vj[0] = tmp;

  dgemv('n',qa,lm,1.0,b->vert[1].a,qa,vj,1,0.0,v,1);

  // swap back
  tmp = vj[1];  vj[1] = vj[0]; vj[0] = tmp;
}

void Quad::Jbwdfac1(int fac, double *vj, double *v){
  int      lm  = edge[fac].l+2;
  Basis    *b = getbasis();
  double    tmp;

  /* need to swap vj ordering  to be consistent with use of b->vert[1].a as
  starting point of backwards trans array  */
  tmp = vj[1];  vj[1] = vj[0]; vj[0] = tmp;

  dgemv('n',qa,lm,1.0,b->vert[1].a,qa,vj,1,0.0,v,1);

  // swap back
  tmp = vj[1];  vj[1] = vj[0]; vj[0] = tmp;
}


void Tet::Jbwdfac1(int fac, double *vj, double *v){
  register int i;
  int lm[4];

  for(i = 0; i < 3; ++i)
    lm[i] = edge[ednum(fac,i)].l;
  lm[3] = face[fac].l;

  JbwdTri(this->qa,this->qb,this->lmax,lm,vj,v);
}

void Pyr::Jbwdfac1(int fac, double *vj, double *v){
  register int i;
  int lm[5];

  if(Nfverts(fac) == 3){
    for(i = 0; i < 3; ++i)
      lm[i] = edge[ednum(fac,i)].l;
    lm[3] = face[fac].l;

    JbwdTri(this->qa,this->qc,this->lmax,lm,vj,v);
  }
  else{
    for(i = 0; i < 4; ++i)
      lm[i] = edge[ednum(fac,i)].l;
    lm[4] = face[fac].l;

    JbwdQuad(this->qa,this->qb,this->lmax,lm,vj,v);
  }
}

void Prism::Jbwdfac1(int fac, double *vj, double *v){
  register int i;
  int lm[5];

  if(Nfverts(fac) == 3){
    for(i = 0; i < 3; ++i)
      lm[i] = edge[ednum(fac,i)].l;
    lm[3] = face[i].l;

    JbwdTri(this->qa,this->qc,this->lmax,lm,vj,v);
  }
  else{
    for(i = 0; i < 4; ++i)
      lm[i] = edge[ednum(fac,i)].l;
    lm[4] = face[i].l;

    JbwdQuad(this->qa,this->qb, this->lmax,lm,vj,v);
  }
}

void Hex::Jbwdfac1(int fac, double *vj, double *v){
  register int i;
  int lm[5];

  for(i = 0; i < 4; ++i)
    lm[i] = edge[ednum(fac,i)].l;
  lm[4] = face[fac].l;

  JbwdQuad(this->qa,this->qb,this->lmax,lm,vj,v);
}


void Element::Jbwdfac1(int face1, double *vj, double *v){ERR;}


void JbwdTri(int qa, int qb, int lmax, int *lm, double *vj, double *v){
  register int i,l; // assumes v storage is greater than vj !
  double   *H = v, *Hj, **baseA,***baseB;
  double   *wk = dvector(0,qb*lmax-1);

  get_modA(qa,&baseA);
  get_modB(qb,&baseB);

  dzero((lmax*(lmax+1)/2+1),H,1);

  /* pack vj into  H  */
  *H = vj[2]; ++H;
  *H = vj[1]; ++H;
  dcopy(lm[1],vj+3+lm[0],1,H,1); H += lmax-2;
  *H = vj[2]; ++H;
  *H = vj[0]; ++H;
  dcopy(lm[2],vj+3+lm[0]+lm[1],1,H,1); H += lmax-2;

  for(i = 0, l = 0; i < lm[0]; l += lmax-2-i, ++i)
    H[l] = vj[3+i];
  ++H;

  for(i = 0,Hj = vj+3+lm[0]+lm[1]+lm[2]; i<lm[3]; H+=lmax-2-i,Hj+=lm[3]-i,++i)
    dcopy(lm[3]-i,Hj,1,H,1);

  H = v;

#if 0  /* dgemm and dgemv version */
  dgemv('n',qb,lmax,1.0,baseB[0][0],qb,H,1,0.0,wk,  lmax); H+=lmax;
  dgemv('n',qb,lmax,1.0,baseB[0][0],qb,H,1,0.0,wk+1,lmax); H+=lmax;
  for(i = 0; i < lmax-2; H+=lmax-2-i, ++i)
    dgemv('n',qb,lmax-2-i,1.0,baseB[i+1][0],qb,H,1,0.0,wk+2+i,lmax);
  dgemm('n','n',qa,qb,lmax,1.0,baseA[0],qa,wk,lmax,0.0,v,qa);
#else /* mxv and mxm versions */
  mxva(baseB[0][0],1,qb,H,1,wk  ,lmax,qb,lmax); H+=lmax;
  mxva(baseB[0][0],1,qb,H,1,wk+1,lmax,qb,lmax); H+=lmax;
  for(i = 0; i < lmax-2; H+=lmax-2-i, ++i)
    mxva(baseB[i+1][0],1,qb,H,1,wk+2+i,lmax,qb,lmax-2-i);
  mxm(wk,qb,baseA[0],lmax,v,qa);
#endif

  free(wk);
}


void JbwdQuad(int qa, int qb, int lmax, int *lm, double *vj, double *v){
  double   *H = v;   // assumes v storage is greater than vj !
  register int i;
  double   **baseA, **baseB;
  double   *wk = dvector(0,lmax*qb);

  get_modA(qa,&baseA);
  get_modA(qb,&baseB);

  /* pack vj into  H  */
  dzero(lmax*lmax,H,1);

  /* pack vj into lmax storage */
  *H = vj[2]; ++H;
  *H = vj[3]; ++H;
  dcopy(lm[2],vj+4+lm[0]+lm[1],1,H,1); H += lmax-2;

  *H = vj[1]; ++H;
  *H = vj[0]; ++H;
  dcopy(lm[0],vj+4,1,H,1); H += lmax-2;

  dcopy(lm[1], vj+4+lm[0], 1, H, lmax);++H;
  dcopy(lm[3], vj+4+lm[0]+lm[1]+lm[2], 1, H, lmax);++H;

  //for(i = 0; i < lm[4]; ++H, ++i)
  //  dcopy(lm[4], vj+4+lm[0]+lm[1]+lm[2]+lm[3] +i*lm[4], 1, H, lmax);

  for(i = 0; i < lm[4]; H += lmax, ++i)
    dcopy(lm[4], vj+4+lm[0]+lm[1]+lm[2]+lm[3] +i*lm[4], 1, H, 1);

  H = v;

  dgemm('N','N', qa,lmax,lmax,1.0,  baseA[0], qa, H,lmax,0.0,wk, qa);
  dgemm('N','T', qa,  qb,lmax,1.0, wk, qa,  baseB[0],qb, 0.0, v, qa);

  free(wk);
}
