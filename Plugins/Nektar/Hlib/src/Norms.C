
/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //

//  The code was updated by Leopold Grinberg                             //
//  for  BlueGene                                                         //
//  Data   :    02/07/2006
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

/*

Function name: Element::Error

Function Purpose:

Argument 1: char *string
Purpose:

Function Notes:

*/

void Tri::Error(char *string){
  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)calloc(1,sizeof(Coord));

  vector_def("x y",string);
  X->x = dvector(0,QGmax*QGmax-1);
  X->y = dvector(0,QGmax*QGmax-1);
  utmp = dvector(0,QGmax*QGmax-1);

  fprintf(stdout, "%c ",type);

  qt = qa*qb;
  s  = h[0];

  if(state == 't'){ Trans(this, J_to_Q); trip = 1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);
  dvsub(qt,s,1,utmp,1,s,1);

  li = ((store=Norm_li())>li)? store:li;
  Norm_l2m(&l2a,&area);
  Norm_h1m(&h1a,&area);

  l2    += l2a;
  h1    += h1a;
  areat += area;

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(utmp); free((char *)X);
}




void Quad::Error(char *string){
  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  vector_def("x y",string);
  X->x = dvector(0,QGmax*QGmax-1);
  X->y = dvector(0,QGmax*QGmax-1);
  utmp = dvector(0,QGmax*QGmax-1);

  fprintf(stdout, "%c ",type);

  qt = qa*qb;
  s  = h[0];

  if(state == 't'){ Trans(this, J_to_Q); trip = 1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);
  dvsub(qt,s,1,utmp,1,s,1);

  li = ((store=Norm_li())>li)? store:li;
  Norm_l2m(&l2a,&area);
  Norm_h1m(&h1a,&area);

  l2    += l2a;
  h1    += h1a;
  areat += area;

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);
  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(utmp); free((char *)X);
}




void Tet::Error(char *string){
  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a=0.,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  vector_def("x y z",string);
  X->x = dvector(0,QGmax*QGmax*QGmax-1);
  X->y = dvector(0,QGmax*QGmax*QGmax-1);
  X->z = dvector(0,QGmax*QGmax*QGmax-1);
  utmp = dvector(0,QGmax*QGmax*QGmax-1);

  qt = qtot;
  s  = h_3d[0][0];

  if(state == 't'){ Trans(this,J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,X->z,s);

  dvsub(qt,s,1,utmp,1,s,1);

  li = ((store=Norm_li())>li)? store:li;
  Norm_l2m(&l2a,&area);
  Norm_h1m(&h1a,&area);

  l2    += l2a;
  h1    += h1a;
  areat += area;

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);
  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(X->z);
  free(utmp); free((char *)X);
}




void Pyr::Error(char *string){
  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  vector_def("x y z",string);
  X->x = dvector(0,QGmax*QGmax*QGmax-1);
  X->y = dvector(0,QGmax*QGmax*QGmax-1);
  X->z = dvector(0,QGmax*QGmax*QGmax-1);
  utmp = dvector(0,QGmax*QGmax*QGmax-1);

  qt = qtot;
  s  = h_3d[0][0];

  if(state == 't'){ Trans(this,J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,X->z,s);

  dvsub(qt,s,1,utmp,1,s,1);

  li = ((store=Norm_li())>li)? store:li;
  Norm_l2m(&l2a,&area);
  Norm_h1m(&h1a,&area);

  l2    += l2a;
  h1    += h1a;
  areat += area;

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);
  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(X->z);
  free(utmp); free((char *)X);
}




void Prism::Error(char *string){
  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)calloc(1,sizeof(Coord));

  vector_def("x y z",string);
  X->x = dvector(0,QGmax*QGmax*QGmax-1);
  X->y = dvector(0,QGmax*QGmax*QGmax-1);
  X->z = dvector(0,QGmax*QGmax*QGmax-1);
  utmp = dvector(0,QGmax*QGmax*QGmax-1);

  qt = qtot;
  s  = h_3d[0][0];

  if(state == 't'){ Trans(this,J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,X->z,s);

  dvsub(qt,s,1,utmp,1,s,1);

  li = ((store=Norm_li())>li)? store:li;
  Norm_l2m(&l2a,&area);
  Norm_h1m(&h1a,&area);

  l2    += l2a;
  h1    += h1a;
  areat += area;

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(X->z);
  free(utmp); free((char *)X);
}




void Hex::Error(char *string){
  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  vector_def("x y z",string);
  X->x = dvector(0,QGmax*QGmax*QGmax-1);
  X->y = dvector(0,QGmax*QGmax*QGmax-1);
  X->z = dvector(0,QGmax*QGmax*QGmax-1);
  utmp = dvector(0,QGmax*QGmax*QGmax-1);

  qt = qtot;
  s  = h_3d[0][0];

  if(state == 't'){ Trans(this,J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,X->z,s);

  dvsub(qt,s,1,utmp,1,s,1);

  li = ((store=Norm_li())>li)? store:li;
  Norm_l2m(&l2a,&area);
  Norm_h1m(&h1a,&area);

  l2    += l2a;
  h1    += h1a;
  areat += area;

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(X->z);
  free(utmp); free((char *)X);
}




void Element::Error(char *){ERR;}             // compare with function




/*

Function name: Element::Verror

Function Purpose:

Argument 1: double *u
Purpose:

Argument 2: char *string
Purpose:

Function Notes:

*/

void  Tri::Verror(double *u, char *string){
  int     qt;
  double *s,*utmp;
  Coord *X = (Coord *)calloc(1,sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);

  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,u,1,s,1);

  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    Norm_li(),Norm_l2(),Norm_h1());

  if(state == 'p') dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);
  return;
}




void  Quad::Verror(double *u, char *string){
  int   qt;
  double *s,*utmp;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);

  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,u,1,s,1);

  fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf L2 H1)\n",
    Norm_li(),Norm_l2(),Norm_h1());

  if(state == 'p') dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);
  return;
}




void  Tet::Verror(double *, char *){
  return;
}




void  Pyr::Verror(double *, char *){
  return;
}




void  Prism::Verror(double *, char *){
  return;
}




void  Hex::Verror(double *, char *){
  return;
}




void Element::Verror(double *, char *){ERR;}




/*

Function name: Element::Norm_li

Function Purpose:

Function Notes:

*/

double Tri::Norm_li(){
  const int qt = qa*qb;
  double *u=h[0];

#if defined  (__blrts__) || defined (__bg__)
  return fabs(u[idamax(qt,u,1)-1]);
#else
  return fabs(u[idamax(qt,u,1)]);
#endif
}




double Quad::Norm_li(){
  const int qt = qa*qb;
  double *u=h[0];
#if defined  (__blrts__) || defined (__bg__)
  return fabs(u[idamax(qt,u,1)-1]);
#else
  return fabs(u[idamax(qt,u,1)]);
#endif
}




double Tet::Norm_li(){

  double *u=h_3d[0][0];
#if defined  (__blrts__) || defined (__bg__)
  return fabs(u[idamax(qtot,u,1)-1]);
#else
  return fabs(u[idamax(qtot,u,1)]);
#endif
}




double Pyr::Norm_li(){

  double *u=h_3d[0][0];
#if defined  (__blrts__) || defined (__bg__)
    return fabs(u[idamax(qtot,u,1)-1]);
#else
  return fabs(u[idamax(qtot,u,1)]);
#endif
}




double Prism::Norm_li(){

  double *u=h_3d[0][0];

#if defined  (__blrts__) || defined (__bg__)
  return fabs(u[idamax(qtot,u,1)-1]);
#else
  return fabs(u[idamax(qtot,u,1)]);
#endif
}




double Hex::Norm_li(){

  double *u=h_3d[0][0];
#if defined  (__blrts__) || defined (__bg__)
  return fabs(u[idamax(qtot,u,1)-1]);
#else
  return fabs(u[idamax(qtot,u,1)]);
#endif
}




double Element::Norm_li(){return 0.0;}




/*

Function name: Element::Norm_l2

Function Purpose:

Function Notes:

*/

double Tri::Norm_l2(){
  const   int  qbc = qb, qt = qa*qb;
  double  *H   = h[0];
  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
          area,l2,*z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'b');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
      dvmul(qb,wb,1,b+j,qa,b+j,qa);

  if(curvX)
    dvmul(qt, geom->jac.p, 1, b, 1, b, 1);
  else
    dscal(qt, geom->jac.d, b, 1);
  dvmul(qt, H, 1, H, 1, u, 1);

  l2   = ddot(qt, b, 1, u, 1);
  area = dsum(qt, b, 1);

  free(u); free(b);

  return sqrt(l2/area);
}




double Quad::Norm_l2(){
  const   int Qc   = 1, qab = qa, qbc = qb, qt = qa*qb;
  double  *H   = h[0];
  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
          area,l2,*z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(i = 0; i < Qc; ++i)
    for(j = 0; j < qa; ++j)
      dvmul(qb,wb,1,b+i*qab+j,qa,b+i*qab+j,qa);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);

  l2   = ddot(qt, b, 1, u, 1);
  area = dsum(qt, b, 1);

  free(u); free(b);

  return sqrt(l2/area);
}




double Tet::Norm_l2(){
  return -1.;
}




double Pyr::Norm_l2(){
  return -1.;
}




double Prism::Norm_l2(){
  return -1.;
}




double Hex::Norm_l2(){
  return -1.;
}




double Element::Norm_l2(){return 0.0;}




/*

Function name: Element::Norm_l2m

Function Purpose:

Argument 1: double *l2
Purpose:

Argument 2: double *area
Purpose:

Function Notes:

*/

void Tri::Norm_l2m(double *l2, double *area){
  const    int qt = qa*qb;
  double  *H   = h[0];

  double *u    =dvector(0,qt-1),
         *b    =dvector(0,qt-1),
         *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'b');

  for(i = 0; i < qb; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  if(curvX)
    dvmul(qt, geom->jac.p, 1, b, 1, b, 1);
  else
    dscal(qt, geom->jac.d, b, 1);
  dvmul(qt, H, 1, H, 1, u, 1);

  l2  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(b);

  return;
}




void Quad::Norm_l2m(double *l2, double *area){
  const   int qt = qa*qb;
  double  *H   = h[0];

  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
         *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');

  for(i = 0; i < qb; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);

  l2  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(b);

  return;
}




void Tet::Norm_l2m(double *l2, double *area){

  const   int qab = qa*qb, qbc = qb*qc, qt = qa*qb*qc;
  double  *H   = h_3d[0][0],*wc;

  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
          *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'b');
  getzw(qc,&z,&wc,'c');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qa; ++j)
      dvmul(qb,wb,1,b+i*qab+j,qa,b+i*qab+j,qa);

  for(i = 0; i < qab; ++i)
    dvmul(qc,wc,1,b+i,qab,b+i,qab);

  if(curvX)
    dvmul(qt, geom->jac.p, 1, b, 1, b, 1);
  else
    dsmul(qt, geom->jac.d, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);

  l2  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(b);

  return;
}




void Pyr::Norm_l2m(double *l2, double *area){

  const   int qab = qa*qb, qbc = qb*qc, qt = qa*qb*qc;
  double  *H   = h_3d[0][0],*wc;

  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
          *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');
  getzw(qc,&z,&wc,'c');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qa; ++j)
      dvmul(qb,wb,1,b+i*qab+j,qa,b+i*qab+j,qa);

  for(i = 0; i < qab; ++i)
    dvmul(qc,wc,1,b+i,qab,b+i,qab);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);

  l2  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(b);

  return;
}




void Prism::Norm_l2m(double *l2, double *area){

  const   int qab = qa*qb, qbc = qb*qc, qt = qa*qb*qc;
  double  *H   = h_3d[0][0],*wc;

  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
          *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');
  getzw(qc,&z,&wc,'b');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qa; ++j)
      dvmul(qb,wb,1,b+i*qab+j,qa,b+i*qab+j,qa);

  for(i = 0; i < qab; ++i)
    dvmul(qc,wc,1,b+i,qab,b+i,qab);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);

  l2  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(b);

  return;
}




void Hex::Norm_l2m(double *l2, double *area){

  const   int qab = qa*qb, qbc = qb*qc, qt = qa*qb*qc;
  double  *H   = h_3d[0][0],*wc;

  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
          *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');
  getzw(qc,&z,&wc,'a');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(i = 0; i < qc; ++i)
    for(j = 0; j < qa; ++j)
      dvmul(qb,wb,1,b+i*qab+j,qa,b+i*qab+j,qa);

  for(i = 0; i < qab; ++i)
    dvmul(qc,wc,1,b+i,qab,b+i,qab);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);

  l2  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(b);

  return;
}




void Element::Norm_l2m(double *, double *){ERR;}




/*

Function name: Element::Norm_h1

Function Purpose:

Function Notes:

*/

double Tri::Norm_h1(){
  const   int qbc = qb, qt = qa*qb;
  double  *H   = h[0];

  double  *u   = dvector(0,qt-1),
          *ux  = dvector(0,qt-1),
          *uy  = dvector(0,qt-1),

          *b   = dvector(0,qt-1),
          area,h1,*z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'b');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  Grad_d(ux,uy,(double*)NULL,'a');
  dvmul(qt,ux,1,ux,1,ux,1);
  dvmul(qt,uy,1,uy,1,uy,1);
  dvadd(qt,ux,1,uy,1,ux,1);

  if(curvX)
    dvmul(qt, geom->jac.p, 1, b, 1, b, 1);
  else
    dscal(qt, geom->jac.d, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);
#ifdef ANORM
  daxpy(qt,1.0/(dparam("LAMBDA")*dparam("LAMBDA")),ux,1,u,1);
#else
  dvadd(qt,ux, 1, u, 1, u, 1);
#endif

  h1   = ddot(qt, b, 1, u, 1);
  area = dsum(qt, b, 1);

  free(u); free(ux); free(uy); free(b);

  return sqrt(h1/area);
}




double Quad::Norm_h1(){
  const   int qt = qa*qb;
  double  *H   = h[0];

  double  *u   = dvector(0,qt-1),
          *ux  = dvector(0,qt-1),
          *uy  = dvector(0,qt-1),

          *b   = dvector(0,qt-1),
          area,h1,*z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');

  for(i = 0; i < qb; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  Grad_d(ux,uy,(double*)NULL,'a');
  dvmul(qt,ux,1,ux,1,ux,1);
  dvmul(qt,uy,1,uy,1,uy,1);
  dvadd(qt,ux,1,uy,1,ux,1);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);
#ifdef ANORM
  daxpy(qt,1.0/(dparam("LAMBDA")*dparam("LAMBDA")),ux,1,u,1);
#else
  dvadd(qt,ux, 1, u, 1, u, 1);
#endif

  h1   = ddot(qt, b, 1, u, 1);
  area = dsum(qt, b, 1);

  free(u); free(ux); free(uy); free(b);

  return sqrt(h1/area);
}




double Tet::Norm_h1(){
  return -1.;
}




double Pyr::Norm_h1(){
  return -1.;
}




double Prism::Norm_h1(){
  return -1.;
}




double Hex::Norm_h1(){
  return -1.;
}




double Element::Norm_h1(){return 0.0;}




/*

Function name: Element::Norm_h1m

Function Purpose:

Argument 1: double *h1
Purpose:

Argument 2: double  *area
Purpose:

Function Notes:

*/

void Tri::Norm_h1m(double *h1, double  *area){
  const   int qbc = qb, qt = qa*qb;
  double  *H   = h[0];
  double  *u   = dvector(0,qt-1),
          *ux  = dvector(0,qt-1),
          *uy  = dvector(0,qt-1),
          *b   =  dvector(0,qt-1),
  *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'b');

  for(i = 0; i < qbc; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  Grad_d(ux,uy,NULL,'a');
  dvmul(qt,ux,1,ux,1,ux,1);
  dvmul(qt,uy,1,uy,1,uy,1);
  dvadd(qt,ux,1,uy,1,ux,1);

  if(curvX)
    dvmul(qt, geom->jac.p, 1, b, 1, b, 1);
  else
    dscal(qt, geom->jac.d, b, 1);
  dvmul(qt, H, 1, H, 1, u, 1);
  dvadd(qt,ux, 1, u, 1, u, 1);

  h1  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(ux); free(uy); free(b);

  return;
}




void Quad::Norm_h1m(double *h1, double  *area){
  const   int qt = qa*qb;
  double  *H   = h[0];
  double  *u   = dvector(0,qt-1),
          *ux  = dvector(0,qt-1),
          *uy  = dvector(0,qt-1),
          *b   = dvector(0,qt-1),
  *z,*wa,*wb;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');

  for(i = 0; i < qb; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  Grad_d(ux,uy,NULL,'a');
  dvmul(qt,ux,1,ux,1,ux,1);
  dvmul(qt,uy,1,uy,1,uy,1);
  dvadd(qt,ux,1,uy,1,ux,1);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  dvmul(qt, H, 1, H, 1, u, 1);
  dvadd(qt,ux, 1, u, 1, u, 1);

  h1  [0] = ddot(qt, b, 1, u, 1);
  area[0] = dsum(qt, b, 1);

  free(u); free(ux); free(uy); free(b);

  return;
}




void Tet::Norm_h1m(double *, double  *){
  return ;
}




void Pyr::Norm_h1m(double *, double  *){
  return ;
}




void Prism::Norm_h1m(double *, double  *){
  return ;
}




void Hex::Norm_h1m(double *, double  *){
  return ;
}




void Element::Norm_h1m(double *, double  *){ERR;}




/*

Function name: Element::L2_error_elmt

Function Purpose:

Argument 1: char *string
Purpose:

Function Notes:

*/

double Tri::L2_error_elmt(char *string){
  int    qt,trip=0;
  double *s,*utmp,l2;
  Coord  *X = (Coord *)calloc(1,sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);

  if(state == 't'){Trans(this, J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,utmp,1,s,1);
  l2 = Norm_l2();

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);

  return l2;
}




double Quad::L2_error_elmt(char *string){
  int    qt,trip=0;
  double *s,*utmp,l2;
  Coord  *X = (Coord *)malloc(sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);

  if(state == 't'){Trans(this, J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,utmp,1,s,1);
  l2 = Norm_l2();

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);

  return l2;
}




double Tet::L2_error_elmt(char *){

  return -1.;
}




double Pyr::L2_error_elmt(char *){

  return -1.;
}




double Prism::L2_error_elmt(char *){

  return -1.;
}




double Hex::L2_error_elmt(char *){

  return -1.;
}




double Element::L2_error_elmt(char *){return 0.0;}




/*

Function name: Element::Int_error

Function Purpose:

Argument 1: char *string
Purpose:

Function Notes:

*/

double Tri::Int_error( char *string){
  int   qt,trip=0;
  double *s,*utmp,l2,area;
  Coord *X = (Coord *)calloc(1,sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp =  dvector(0,qt-1);

  if(state == 't'){Trans(this, J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,utmp,1,s,1);
  Norm_l2m(&l2,&area);

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);

  return l2;
}




double Quad::Int_error( char *string){
  int   qt,trip=0;
  double *s,*utmp,l2,area;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);

  if(state == 't'){Trans(this, J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,utmp,1,s,1);
  Norm_l2m(&l2,&area);

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);

  return l2;
}




double Tet::Int_error( char *){
  return -1.;
}




double Pyr::Int_error( char *){
  return -1.;
}




double Prism::Int_error( char *){
  return -1.;
}




double Hex::Int_error( char *){
  return -1.;
}




double Element::Int_error( char *){return 0.0;}




/*

Function name: Element::H1_error_elmt

Function Purpose:

Argument 1: char *string
Purpose:

Function Notes:

*/

double Tri::H1_error_elmt(char *string){
  int   qt,trip=0;
  double *s,*utmp,h1;
  Coord *X = (Coord *)calloc(1,sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);


  if(state == 't'){Trans(this, J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,utmp,1,s,1);
  h1 = Norm_h1();

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);

  return h1;
}




double Quad::H1_error_elmt(char *string){
  int   qt,trip=0;
  double *s,*utmp,h1;
  Coord *X = (Coord *)malloc(sizeof(Coord));

  qt = qa*qb;
  s  = h[0];
  vector_def("x y",string);

  X->x = dvector(0,qt-1);
  X->y = dvector(0,qt-1);
  utmp = dvector(0,qt-1);

  if(state == 't'){Trans(this, J_to_Q); trip =1;}
  dcopy(qt,s,1,utmp,1);

  coord(X);
  vector_set(qt,X->x,X->y,s);

  dvsub(qt,s,1,utmp,1,s,1);
  h1 = Norm_h1();

  if(trip){ state = 't'; trip = 0;}
  else     dcopy(qt,utmp,1,s,1);

  free(X->x); free(X->y); free(utmp); free((char *)X);

  return h1;
}




double Tet::H1_error_elmt(char *){

  return -1.;
}




double Pyr::H1_error_elmt(char *){

  return -1.;
}




double Prism::H1_error_elmt(char *){

  return -1.;
}




double Hex::H1_error_elmt(char *){

  return -1.;
}




double Element::H1_error_elmt(char *){return 0.0;}


/* ------------------------------------------------------------------------ */
double Quad::Norm_beta(){
  const   int qt = qa*qb;
  double  *H   = h[0];

  double *u    = dvector(0,qt-1),
         *b    = dvector(0,qt-1),
         *z,*wa,*wb, sum;
  register int i,j;

  getzw(qa,&z,&wa,'a');
  getzw(qb,&z,&wb,'a');

  for(i = 0; i < qb; ++i)
    dcopy(qa,wa,1,b+i*qa,1);
  for(j = 0; j < qa; ++j)
    dvmul(qb,wb,1,b+j,qa,b+j,qa);

  dvmul(qt, geom->jac.p, 1, b, 1, b, 1);

  sum = ddot(qt, b, 1, H, 1);

  free(u); free(b);

  return sum;
}

double Element::Norm_beta(){ ERR;}
/* ------------------------------------------------------------------------ */
