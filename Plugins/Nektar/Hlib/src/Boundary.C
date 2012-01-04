

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

using namespace polylib;

/*

Function name: Element::gen_bndry

Function Purpose:

Argument 1: char bc
Purpose:

Argument 2: int fac
Purpose:

Argument 3: ...
Purpose:

Function Notes:

*/

static void Tri_Fill_Phys_Bound (Bndry *Ubc, double *f);
Bndry *Tri::gen_bndry(char bc, int fac, ...){
  register int i;
  int     qt,Je;
  char   *bf;
  double  bv,*f;
  Bndry  *nbdry;
  Coord  X;
  va_list ap;
  int OpSplit = option("OpSplit");

  X.x = (double*) 0;
  X.y = (double*) 0;
  X.z = (double*) 0;

  if(fac == 0)
    qt = qa;
  else
    qt = qb;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  nbdry = (Bndry *)calloc(1,sizeof(Bndry));

  Je = (option("tvarying"))? iparam("INTYPE")+1:1;

  MemBndry(nbdry,fac,Je);

  va_start(ap, fac);
  if(islower(bc)){

    if((bf = (strchr(va_arg(ap,char *),'='))) == (char *) NULL)
      error_msg(gen_bndry: Illegal B.C. function definition);

    while(isspace(*++bf));

    nbdry->bstring = strdup(bf);

    vector_def("x y",bf);
    /* set top vertex */
    vector_set(1,&(vert[vnum(fac,1)].x),&(vert[vnum(fac,1)].y),
         nbdry->bvert+1);
    GetFaceCoord(fac,&X);
    vector_set(qt,X.x,X.y,f);
  }
  else{
    char buf[BUFSIZ];

    bv = va_arg(ap,double);

    dfill(qt,bv,f,1);
    sprintf(buf,"%lf",bv);
    nbdry->bstring = strdup(buf);
  }

  va_end(ap);

  nbdry->type = toupper(bc);
  nbdry->face = fac;
  nbdry->elmt = this;

  if(curve)
    Surface_geofac(nbdry);


#ifdef COMPRESS
  nbdry->phys_val   = dvector(0,QGmax-1);
  nbdry->phys_val_g = dvector(0,QGmax-1);
  switch(bc){
  case 'V':
    dfill(qt, bv, f, 1);
  case 'v':
    Tri_Fill_Phys_Bound (nbdry,f);
    break;
  case 'W':
    break;
  case 'O':
    dzero(qt,nbdry->phys_val_g,1);
    Tri_Fill_Phys_Bound (nbdry,f);
    break;
  case 'F':
    dcopy(qt,&bv,0,f,1);
  case 'f': case's':
    Tri_Fill_Phys_Bound (nbdry,f);
    break;
  }
#endif

  if(OpSplit&&(bc != 'X')){
    nbdry->phys_val_g = dvector(0,QGmax-1);
    Tri_Fill_Phys_Bound (nbdry,f);
  }

  switch(bc){
  case 'v':
    nbdry->bvert[0] = f[0];
    JtransEdge(nbdry,fac,0,f);
    break;
  case 'V':
    for(i = 0; i < Tri_DIM; ++i) nbdry->bvert[i] = bv;
    break;
  case 'F': case 'R':
    dfill(qt,bv,f,1);
  case 'f': case 'r':
    Surface_geofac(nbdry);
    MakeFlux(nbdry,fac,f);
    break;
  default:
    break;
  }

  free(X.x);  free(X.y); free(f);

  return nbdry;
}

static void Quad_Fill_Phys_Bound (Bndry *Ubc, double *f);

Bndry *Quad::gen_bndry(char bc, int fac, ...){
  register int i;
  int     qt,Je;
  char   *bf;
  double  bv,*f;
  Bndry  *nbdry;
  Coord   X;
  va_list ap;
  int OpSplit = option("OpSplit");

  if(fac == 0 || fac ==2)
    qt = qa;
  else
    qt = qb;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  nbdry = (Bndry *)calloc(1,sizeof(Bndry));
  Je = (option("tvarying"))? iparam("INTYPE")+1:1;

  MemBndry(nbdry,fac,Je);

  va_start(ap, fac);
  if(islower(bc)){

    if((bf = (strchr(va_arg(ap,char *),'='))) == (char *) NULL)
      error_msg(gen_bndry: Illegal B.C. function definition);

    while(isspace(*++bf));

    nbdry->bstring = strdup(bf);

    vector_def("x y",bf);
    /* set top vertex */
    vector_set(1,&(vert[vnum(fac,1)].x),&(vert[vnum(fac,1)].y),
         nbdry->bvert+1);
    GetFaceCoord(fac,&X);

    vector_set(qt,X.x,X.y,f);
  }
  else{
    char buf[BUFSIZ];

    bv = va_arg(ap,double);

    dfill(qt,bv,f,1);
    sprintf(buf,"%lf",bv);
    nbdry->bstring = strdup(buf);
  }
  va_end(ap);

  nbdry->type = toupper(bc);
  nbdry->face = fac;
  nbdry->elmt = this;

#ifdef COMPRESS
  nbdry->phys_val   = dvector(0,QGmax-1);
  nbdry->phys_val_g = dvector(0,QGmax-1);
  switch(bc){
  case 'V':
    dfill(qt, bv, f, 1);
  case 'v':
    Quad_Fill_Phys_Bound (nbdry,f);
    break;
  case 'W':
    break;
  case 'O':
    dzero(qt,nbdry->phys_val_g,1);
    Quad_Fill_Phys_Bound (nbdry,f);
    break;
  case 'F':
    dcopy(qt,&bv,0,f,1);
  case 'f': case's':
    Quad_Fill_Phys_Bound (nbdry,f);
    break;
  }
#endif

  if(OpSplit&&(bc != 'X')){ // don't use this when defining boundaries
    nbdry->phys_val_g = dvector(0,QGmax-1);
    Quad_Fill_Phys_Bound (nbdry,f);
  }

  switch(bc){
  case 'v':
    nbdry->bvert[0] = f[0];
    JtransEdge(nbdry,fac,0,f);
    break;
  case 'V':
    for(i = 0; i < 2; ++i) nbdry->bvert[i] = bv;
    break;
  case 'F': case 'R':
    dfill(qt,bv,f,1);
  case 'f': case 'r':
    Surface_geofac(nbdry);
    MakeFlux(nbdry,fac,f);
    break;
  default:
    break;
  }

  free(X.x);  free(X.y);  free(X.z); free(f);

  return nbdry;
}



static void Tet_Fill_Phys_Bound (Bndry *Ubc, double *f);

Bndry *Tet::gen_bndry(char bc, int fac, ...){
  int     i,qt,Je;
  char   *bf;
  double  bv,*f;
  Bndry  *nbdry;
  Coord   X;
  va_list ap;
  int OpSplit = option("OpSplit");

  double *tmp = dvector(0, QGmax*QGmax-1);

  if(fac == 0)
    qt = qa*qb;
  else
    if(fac == 1)
      qt = qa*qc;
    else
      qt = (qb+1)*qc;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  nbdry = (Bndry *)calloc(1,sizeof(Bndry));
  Je = (option("tvarying"))? iparam("INTYPE")+1:1;
  MemBndry(nbdry,fac,Je);

  va_start(ap, fac);
  if(islower(bc)){

    if((bf = (strchr(va_arg(ap,char *),'='))) == (char *) NULL)
      error_msg(gen_bndry: Illegal B.C. function definition);

    while(isspace(*++bf));

    nbdry->bstring = strdup(bf);
    vector_def("x y z",bf);

    /* set singular point */
    vector_set(1,&(vert[vnum(fac,2)].x),&(vert[vnum(fac,2)].y),
         &(vert[vnum(fac,2)].z),nbdry->bvert+2);

    GetFaceCoord(fac,&X);
    vector_set(qt,X.x,X.y,X.z,f);
  }
  else{
    char buf[BUFSIZ];

    bv = va_arg(ap,double);

    sprintf(buf,"%lf",bv);
    dfill(qt,bv,f,1);
    nbdry->bstring = strdup(buf);
  }

  va_end(ap);

  nbdry->type = toupper(bc);
  nbdry->face = fac;
  nbdry->elmt = this;

#ifdef COMPRESS
  nbdry->phys_val   = dvector(0,qt-1);
  nbdry->phys_val_g = dvector(0,qt-1);
  switch(bc){
  case 'v':
    Tet_Fill_Phys_Bound (nbdry,f);
    break;
  case 'W':
    break;
  case 'f': case's':
    Tet_Fill_Phys_Bound (nbdry,f);
    break;
  }
#endif

  if(OpSplit&&(bc != 'X')){ // don't use this when defining boundaries
    nbdry->phys_val_g = dvector(0,qt-1);
    Tet_Fill_Phys_Bound (nbdry,f);
  }

  switch(bc){
  case 'v':
    Tet::JtransFace(nbdry,f);
    break;
  case 'V':
    for(i = 0; i < Nverts-1; ++i) nbdry->bvert[i] = bv;
    break;
  case 'F': case 'R':
    dfill(qt,bv,f,1);
  case 'f': case 'r':
    Surface_geofac(nbdry);
    InterpToFace1(fac, f, tmp);
    MakeFlux(nbdry,fac, tmp);
    break;
  default:
    break;
  }

  free(tmp); free(f);
  free(X.x);  free(X.y);  free(X.z);

  return nbdry;
}

Bndry *Pyr::gen_bndry(char bc, int fac, ...){
  register int i;
  int     qt,q1,q2,Je;
  char   *bf;
  double  bv,*f;
  Bndry  *nbdry;
  Coord   X;
  va_list ap;
  double *tmp = dvector(0, QGmax*QGmax-1);
  int OpSplit = option("OpSplit");

  if(fac == 0){
    q1 = qa; q2 = qb;
  }
  else if(fac == 1 || fac == 3){
    q1 = qa; q2 = qc;
  }
  else{
    q1 = qb; q2 = qc;
  }
  qt = q1*q2;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  nbdry = (Bndry *)calloc(1,sizeof(Bndry));
  Je = (option("tvarying"))? iparam("INTYPE")+1:1;
  MemBndry(nbdry,fac,Je);

  va_start(ap, fac);
  if(islower(bc)){

    if((bf = (strchr(va_arg(ap,char *),'='))) == (char *) NULL)
      error_msg(gen_bndry: Illegal B.C. function definition);

    while(isspace(*++bf));

    nbdry->bstring = strdup(bf);

    vector_def("x y z",bf);

    for(i=0;i<Nfverts(fac);++i)
      vector_set(1,
     &(vert[vnum(fac,i)].x),&(vert[vnum(fac,i)].y),
     &(vert[vnum(fac,i)].z),nbdry->bvert+i);

    GetFaceCoord(fac,&X);
    vector_set(qt,X.x,X.y,X.z,f);
  }
  else{
    char buf[BUFSIZ];

    bv = va_arg(ap,double);

    sprintf(buf,"%lf",bv);
    dfill(qt,bv,f,1);
    nbdry->bstring = strdup(buf);
  }

  va_end(ap);

  nbdry->type = toupper(bc);
  nbdry->face = fac;
  nbdry->elmt = this;

  if(OpSplit&&(bc != 'X')){ // don't use this when defining boundaries
    nbdry->phys_val_g = dvector(0,qt-1);
    fprintf(stderr,"Pyr_Fill_Phys_Bound needs definiing\n");
  }

  switch(bc){
  case 'v':
    InterpToFace1(fac,f,tmp);
    Pyr::JtransFace(nbdry,tmp);
    break;
  case 'V':
    for(i = 0; i < Nfverts(fac); ++i) nbdry->bvert[i] = bv;
    break;
  case 'F': case 'R':
    dfill(qt,bv,f,1);
  case 'f': case 'r':
    Surface_geofac(nbdry);
    InterpToFace1(fac, f, tmp);
    MakeFlux(nbdry,fac,tmp);
    break;
  default:
    break;
  }

 free(tmp); free(X.x); free(X.y); free(X.z); free(f);

  return nbdry;
}


static void Prism_Fill_Phys_Bound (Bndry *Ubc, double *f);
Bndry *Prism::gen_bndry(char bc, int fac, ...){
  register int i;
  int     qt,Je;
  char   *bf;
  double  bv,*f;
  Bndry  *nbdry;
  Coord   X;
  va_list ap;
  double *tmp = dvector(0, QGmax*QGmax-1);
  int OpSplit = option("OpSplit");

  if(fac == 0)
    qt = qa*qb;
  else if(fac == 1 || fac == 3)
    qt = qa*qc;
  else
    qt = qb*qc;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  nbdry = (Bndry *)calloc(1,sizeof(Bndry));
  Je = (option("tvarying"))? iparam("INTYPE")+1:1;
  MemBndry(nbdry,fac,Je);

  va_start(ap, fac);
  if(islower(bc)){

    if((bf = (strchr(va_arg(ap,char *),'='))) == (char *) NULL)
      error_msg(gen_bndry: Illegal B.C. function definition);

    while(isspace(*++bf));

    nbdry->bstring = strdup(bf);

    vector_def("x y z",bf);

    for(i=0;i<Nfverts(fac);++i)
      vector_set(1,
     &(vert[vnum(fac,i)].x),&(vert[vnum(fac,i)].y),
     &(vert[vnum(fac,i)].z),nbdry->bvert+i);

    GetFaceCoord(fac,&X);
    vector_set(qt,X.x,X.y,X.z,f);
  }
  else{
    char buf[BUFSIZ];

    bv = va_arg(ap,double);

    sprintf(buf,"%lf",bv);
    dfill(qt,bv,f,1);
    nbdry->bstring = strdup(buf);
  }

  va_end(ap);

  nbdry->type = toupper(bc);
  nbdry->face = fac;
  nbdry->elmt = this;

#ifdef COMPRESS
  nbdry->phys_val   = dvector(0,qt-1);
  nbdry->phys_val_g = dvector(0,qt-1);
  switch(bc){
  case 'v':
    Prism_Fill_Phys_Bound (nbdry,f);
    break;
  case 'W':
    break;
  case 'f': case's':
    Prism_Fill_Phys_Bound (nbdry,f);
    break;
  }
#endif

  if(OpSplit&&(bc != 'X')){ // don't use this when defining boundaries
    nbdry->phys_val_g = dvector(0,qt-1);
    Prism_Fill_Phys_Bound (nbdry,f);
  }

  switch(bc){
  case 'v':
    InterpToFace1(fac,f,tmp);
    Prism::JtransFace(nbdry,tmp);
    break;
  case 'V':
    for(i = 0; i < Nfverts(fac); ++i) nbdry->bvert[i] = bv;
    break;
  case 'F': case 'R':
    dfill(qt,bv,f,1);
  case 'f': case 'r':
    Surface_geofac(nbdry);
    InterpToFace1(fac,f,tmp);
    MakeFlux(nbdry,fac,tmp);
    break;
  default:
    break;
  }

  free(tmp); free(X.x); free(X.y); free(X.z); free(f);

  return nbdry;
}

static void Hex_Fill_Phys_Bound (Bndry *Ubc, double *f);

Bndry *Hex::gen_bndry(char bc, int fac, ...){

  int     qt,Je;
  char   *bf;
  double  bv,*f;
  Bndry  *nbdry;
  Coord   X;
  va_list ap;
  int OpSplit = option("OpSplit");

  double *tmp = dvector(0, QGmax*QGmax-1);

  qt = qa*qb;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  nbdry = (Bndry *)calloc(1,sizeof(Bndry));
  Je = (option("tvarying"))? iparam("INTYPE")+1:1;
  MemBndry(nbdry,fac,Je);

  va_start(ap, fac);
  if(islower(bc)){

    if((bf = (strchr(va_arg(ap,char *),'='))) == (char *) NULL)
      error_msg(gen_bndry: Illegal B.C. function definition);

    while(isspace(*++bf));

    nbdry->bstring = strdup(bf);

    vector_def("x y z",bf);

    GetFaceCoord(fac,&X);
    //    straight_face(&X,fac,0);
    vector_set(qt,X.x,X.y,X.z,f);
  }
  else{
    char buf[BUFSIZ];

    bv = va_arg(ap,double);

    sprintf(buf,"%lf",bv);
    dfill(qt,bv,f,1);
    nbdry->bstring = strdup(buf);
  }

  va_end(ap);

  nbdry->type = toupper(bc);
  nbdry->face = fac;
  nbdry->elmt = this;

#ifdef COMPRESS
  nbdry->phys_val   = dvector(0,QGmax*QGmax-1);
  nbdry->phys_val_g = dvector(0,QGmax*QGmax-1);
  switch(bc){
  case 'V':
    dfill(qt, bv, f, 1);
  case 'v':
    Hex_Fill_Phys_Bound (nbdry,f);
    break;
  case 'W':
    break;
  case 'O':
    dzero(qt,nbdry->phys_val_g,1);
    Hex_Fill_Phys_Bound (nbdry,f);
    break;
  case 'F':
    dcopy(qt,&bv,0,f,1);
  case 'f': case's':
    Hex_Fill_Phys_Bound (nbdry,f);
    break;
  }
#else

  if(OpSplit&&(bc != 'X')){ // don't use this when defining boundaries
    nbdry->phys_val_g = dvector(0,qt-1);
    Hex_Fill_Phys_Bound (nbdry,f);
  }

  register int i;
  switch(bc){
  case 'v': case 'm':
    Hex::JtransFace(nbdry,f);
    break;
  case 'V':
    for(i = 0; i < Nfverts(fac); ++i) nbdry->bvert[i] = bv;
    break;
  case 'F': case 'R':
    dfill(qt,bv,f,1);
  case 'f': case 'r':
    Surface_geofac(nbdry);
    InterpToFace1(fac, f, tmp);
    MakeFlux(nbdry,fac, tmp);
    break;
  default:
    break;
  }
#endif
  // leak
  free(tmp); free(X.x); free(X.y); free(X.z); free(f);
  return nbdry;
}




Bndry *Element::gen_bndry(char , int , ...){ERR;return (Bndry*)NULL;}


static void Tri_Fill_Phys_Bound (Bndry *Ubc, double *f){
  int qt;
  double **im;
  int qedg = Ubc->elmt->edge[Ubc->face].qedg;

  switch (Ubc->face) {
  case 0:
    qt=Ubc->elmt->qa;
    getim(qt,qedg,&im,a2g);
    break;
  case 1:  case 2:
    qt=Ubc->elmt->qb;
    getim(qt,qedg,&im,b2g);
    break;
  }

  if(!option("OpSplit")){
    /* store quadrature points for viscous part */
    dcopy(qt,f,1,Ubc->phys_val,1);
  }

  /* store gaussian distribution for euler part */
  Interp(*im,f,qt,Ubc->phys_val_g,qedg);

  return;
}


static void Quad_Fill_Phys_Bound (Bndry *Ubc, double *f){
  int qt;
  double **im;
  int qedg = Ubc->elmt->edge[Ubc->face].qedg;

  qt = (Ubc->face == 0 || Ubc->face == 2) ? Ubc->elmt->qa: Ubc->elmt->qb;
  getim(qt,qedg,&im,a2g);// ok

  if(!option("OpSplit")){
    /* store quadrature points for viscous part */
    dcopy(qt,f,1,Ubc->phys_val,1);
  }

  /* store gaussian distribution for euler part */
  Interp(*im,f,qt,Ubc->phys_val_g,qedg);

  return;
}




static void Tet_Fill_Phys_Bound (Bndry *Ubc, double *f){
  int      q1,q2;
  double **ima, **imb;
  Element *E = Ubc->elmt;
  int      qface = E->face[Ubc->face].qface;

  switch (Ubc->face) {
  case 0:
    q1 = E->qa;
    q2 = E->qb;
    getim(q1,qface,&ima,a2g); //ok
    getim(q2,qface,&imb,b2g); //ok
    break;
  case 1:
    q1 = E->qa;
    q2 = E->qc;
    getim(q1,qface,&ima,a2g); //ok
    getim(q2,qface,&imb,c2g); //ok
    break;
  case 2: case 3:
    q1 = E->qb;
    q2 = E->qc;
    getim(q1,qface,&ima,b2g); //ok
    getim(q2,qface,&imb,c2g); //ok
    break;
  }

  /* store quadrature points for viscous part */
  if(!option("OpSplit"))
    dcopy(q1*q2,f,1,Ubc->phys_val,1);

  /* store gaussian distribution for euler part */
  Interp2d(*ima,*imb,f,q1,q2, Ubc->phys_val_g,qface,qface);

  return;
}


static void Prism_Fill_Phys_Bound (Bndry *Ubc, double *f){
  int      q1,q2;
  double **ima, **imb;
  Element *E = Ubc->elmt;
  int      qface = E->face[Ubc->face].qface;

  switch (Ubc->face) {
  case 0:
    q1 = E->qa;
    q2 = E->qb;
    getim(q1,qface,&ima,a2g); //ok
    getim(q2,qface,&imb,a2g); //ok
    break;
  case 1: case 3:
    q1 = E->qa;
    q2 = E->qc;
    getim(q1,qface,&ima,a2g); //ok
    getim(q2,qface,&imb,b2g); //ok
    break;
  case 2: case 4:
    q1 = E->qb;
    q2 = E->qc;
    getim(q1,qface,&ima,a2g); //ok
    getim(q2,qface,&imb,b2g); //ok
    break;
  }

  /* store quadrature points for viscous part */
  if(!option("OpSplit"))
    dcopy(q1*q2,f,1,Ubc->phys_val,1);

  /* store gaussian distribution for euler part */
  Interp2d(*ima,*imb,f,q1,q2, Ubc->phys_val_g,qface,qface);

  return;
}


static void Hex_Fill_Phys_Bound (Bndry *Ubc, double *f){
  int qt,q1,q2;
  double **ima, **imb;
  Element *E = Ubc->elmt;
  int qedg = E->face[Ubc->face].qface;

  switch (Ubc->face) {
  case 0:  case 5:
    q1 = E->qa;
    q2 = E->qb;
    getim(q1,qedg,&ima,a2g); //ok
    getim(q2,qedg,&imb,a2g); //ok
    break;
  case 1: case 3:
    q1 = E->qa;
    q2 = E->qc;
    getim(q1,qedg,&ima,a2g); //ok
    getim(q2,qedg,&imb,a2g); //ok
    break;
  case 2: case 4:
    q1 = E->qb;
    q2 = E->qc;
    getim(q1,qedg,&ima,a2g); //ok
    getim(q2,qedg,&imb,a2g); //ok
    break;
  }
  qt = q1*q2;

  /* store quadrature points for viscous part */
  if(!option("OpSplit"))
    dcopy(qt,f,1,Ubc->phys_val,1);

  /* store gaussian distribution for euler part */
  Interp2d(*ima,*imb,f,q1,q2,
     Ubc->phys_val_g,E->face[Ubc->face].qface,E->face[Ubc->face].qface);

  return;
}


/*

Function name: Element::update_bndry

Function Purpose:
  Update the coefficients in the Bc storage.

Argument 1: Bndry *Bc
Purpose:
 Storage for boundary condition data.

Function Notes:

*/

void Tri::update_bndry(Bndry *Bc, int save){
  int     qt;
  double  *f;
  Coord   X;

  if(Bc->face == 0)
    qt = qa;
  else
    qt = qb;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  if(save){
    static int Je = iparam("INTYPE");
    int l = Bc->elmt->edge[Bc->face].l;
    int i;

    for(i = Je; i > 0; --i){
      Bc->bvert[2*i  ] = Bc->bvert[2*(i-1)];
      Bc->bvert[2*i+1] = Bc->bvert[2*(i-1)+1];
      dcopy(l,Bc->bedge[0]+(i-1)*l,1,Bc->bedge[0]+i*l,1);
    }
  }

  vector_def("x y",Bc->bstring);
  /* set top vertex */
  vector_set(1,&(vert[vnum(Bc->face,1)].x),&(vert[vnum(Bc->face,1)].y),
       Bc->bvert+1);
  GetFaceCoord(Bc->face,&X);
  vector_set(qt,X.x,X.y,f);

  switch(Bc->type){
  case 'V': case 'M':
    Bc->bvert[0] = f[0];
    JtransEdge(Bc,Bc->face,0,f);
    break;
  case 'W':
    break;
  case 'B':
    if(type == 'u')
      break;
  case 'F': case 'R':
    MakeFlux(Bc,Bc->face,f);
    break;
  }
  free(X.x);  free(X.y);  free(f);

  return;
}


void Quad::update_bndry(Bndry *Bc,int save){
  int     qt;
  double  *f;
  Coord   X;

  if(Bc->face == 0 || Bc->face ==2)
    qt = qa;
  else
    qt = qb;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);

  f   = dvector(0,qt-1);

 if(save){
    static int Je = iparam("INTYPE");
    int l = Bc->elmt->edge[Bc->face].l;
    int i;

    for(i = Je; i > 0; --i){
      Bc->bvert[2*i  ] = Bc->bvert[2*(i-1)];
      Bc->bvert[2*i+1] = Bc->bvert[2*(i-1)+1];
      dcopy(l,Bc->bedge[0]+(i-1)*l,1,Bc->bedge[0]+i*l,1);
    }
  }

  vector_def("x y",Bc->bstring);
  /* set top vertex */
  vector_set(1,&(vert[vnum(Bc->face,1)].x),&(vert[vnum(Bc->face,1)].y),
       Bc->bvert+1);
  GetFaceCoord(Bc->face,&X);
  vector_set(qt,X.x,X.y,f);

  switch(Bc->type){
  case 'V': case 'M':
    Bc->bvert[0] = f[0];
    JtransEdge(Bc,Bc->face,0,f);
    break;
  case 'W':
    break;
  case 'B':
    if(Bc->elmt->type == 'u')
      break;
  case 'F': case 'R':
    MakeFlux(Bc,Bc->face,f);
    break;
  }

  free(X.x);  free(X.y);  free(f);

  return;
}

void Tet::update_bndry(Bndry *Bc, int save){
  int     qt;
  double  *f;
  Coord   X;
  int fac = Bc->face;

  if(fac == 0)
    qt = qa*qb;
  else
    if(fac == 1)
      qt = qa*qc;
    else
      qt = (qb+1)*qc;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  vector_def("x y z",Bc->bstring);

  GetFaceCoord(Bc->face,&X);
  vector_set(qt,X.x,X.y,X.z,f);

  if(save){
    static int Je = iparam("INTYPE");
    int i,j,nfv,l;
    Element *E = Bc->elmt;

    nfv = E->Nfverts(Bc->face);
    for(j = Je; j > 0; --j){
      for(i = 0; i < nfv; ++i)
  Bc->bvert[j*nfv+i] = Bc->bvert[(j-1)*nfv+i];

      for(i = 0; i < nfv; ++i){
  l = E->edge[E->ednum(Bc->face,i)].l;
  dcopy(l,Bc->bedge[i]+(j-1)*l,1,Bc->bedge[i]+j*l,1);
      }

      l  =  Bc->elmt->face[Bc->face].l;
      l = l*(l+1)/2;
      dcopy(l,Bc->bface[0]+(j-1)*l,1,Bc->bface[0]+j*l,1);
    }
  }

  /* set singular point */
  vector_set(1,&(vert[vnum(fac,2)].x),&(vert[vnum(fac,2)].y),
         &(vert[vnum(fac,2)].z),Bc->bvert+2);

  Tet::JtransFace(Bc,f);

  free(f);  free(X.x);  free(X.y);  free(X.z);

  return;
}

void Pyr::update_bndry(Bndry *Bc,int save){
  register int i;
  int     qt;

  double  *f;
  Coord   X;
  double *tmp = dvector(0, QGmax*QGmax-1);

  if(Bc->face == 0)
    qt = qa*qb;
  else if(Bc->face == 1 || Bc->face == 3)
    qt = qa*qc;
  else
    qt = qb*qc;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  if(save){
    static int Je = iparam("INTYPE");
    int i,j,nfv,l;
    Element *E = Bc->elmt;

    nfv = E->Nfverts(Bc->face);
    for(j = Je; j > 0; --j){
      for(i = 0; i < nfv; ++i)
  Bc->bvert[j*nfv+i] = Bc->bvert[(j-1)*nfv+i];

      for(i = 0; i < nfv; ++i){
  l = E->edge[E->ednum(Bc->face,i)].l;
  dcopy(l,Bc->bedge[i]+(j-1)*l,1,Bc->bedge[i]+j*l,1);
      }

      l  =  Bc->elmt->face[Bc->face].l;

    l  =  Bc->elmt->face[Bc->face].l;
    if(nfv == 3)
      l = l*(l+1)/2;
    else
      l = l*l;
    dcopy(l,Bc->bface[0]+(j-1)*l,1,Bc->bface[0]+j*l,1);
    }
  }

  vector_def("x y z",Bc->bstring);

  for(i=0;i<Nfverts(Bc->face);++i)
    vector_set(1,&(vert[vnum(Bc->face,i)].x),
           &(vert[vnum(Bc->face,i)].y),
           &(vert[vnum(Bc->face,i)].z),Bc->bvert+i);

  GetFaceCoord(Bc->face,&X);
  vector_set(qt,X.x,X.y,X.z,f);


  InterpToFace1(Bc->face,f,tmp);
  Pyr::JtransFace(Bc,tmp);

  free(tmp); free(X.x);free(X.y);free(X.z);free(f);

  return;
}


void Prism::update_bndry(Bndry *Bc, int save){
  register int i;
  int     qt;

  double  *f;
  Coord   X;
  double *tmp = dvector(0, QGmax*QGmax-1);

  if(Bc->face == 0)
    qt = qa*qb;
  else if(Bc->face == 1 || Bc->face == 3)
    qt = qa*qc;
  else
    qt = qb*qc;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  if(save){
    static int Je = iparam("INTYPE");
    int i,j,nfv,l;
    Element *E = Bc->elmt;

    nfv = E->Nfverts(Bc->face);
    for(j = Je; j > 0; --j){
      for(i = 0; i < nfv; ++i)
  Bc->bvert[j*nfv+i] = Bc->bvert[(j-1)*nfv+i];

      for(i = 0; i < nfv; ++i){
  l = E->edge[E->ednum(Bc->face,i)].l;
  dcopy(l,Bc->bedge[i]+(j-1)*l,1,Bc->bedge[i]+j*l,1);
      }

      l  =  Bc->elmt->face[Bc->face].l;

    l  =  Bc->elmt->face[Bc->face].l;
    if(nfv == 3)
      l = l*(l+1)/2;
    else
      l = l*l;
    dcopy(l,Bc->bface[0]+(j-1)*l,1,Bc->bface[0]+j*l,1);
    }
  }

  vector_def("x y z",Bc->bstring);

  for(i=0;i<Nfverts(Bc->face);++i)
    vector_set(1,&(vert[vnum(Bc->face,i)].x),
         &(vert[vnum(Bc->face,i)].y),
         &(vert[vnum(Bc->face,i)].z),Bc->bvert+i);

  GetFaceCoord(Bc->face,&X);
  vector_set(qt,X.x,X.y,X.z,f);


  InterpToFace1(Bc->face,f,tmp);
  Prism::JtransFace(Bc,tmp);

  free(tmp); free(X.x);free(X.y);free(X.z); free(f);

  return;
}

void Hex::update_bndry(Bndry *Bc, int save){

  int     qt;
  double  *f;
  Coord   X;

  if(Bc->face == 0 || Bc->face == 5)
    qt = qa*qb;
  else if(Bc->face == 1 || Bc->face == 3)
    qt = qa*qc;
  else
    qt = qb*qc;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);
  f   = dvector(0,qt-1);

  vector_def("x y z",Bc->bstring);

  GetFaceCoord(Bc->face,&X);
  vector_set(qt,X.x,X.y,X.z,f);

  if(save){
    int nfv,l,i;
    Element *E = Bc->elmt;

    nfv = E->Nfverts(Bc->face);
    for(i = 0; i < nfv; ++i)
      Bc->bvert[nfv+i] = Bc->bvert[i];

    for(i = 0; i < nfv; ++i){
      l = E->edge[E->ednum(Bc->face,i)].l;
      dcopy(l,Bc->bedge[i],1,Bc->bedge[i]+l,1);
    }
    l  =  Bc->elmt->face[Bc->face].l;
    l = l*l;

    dcopy(l,Bc->bface[0],1,Bc->bface[0]+l,1);
  }

  Hex::JtransFace(Bc,f);

  free(X.x);free(X.y);free(X.z);free(f);

  return;
}

void Element::update_bndry(Bndry *, int){ERR;}

/*

Function name: Element::setbcs

Function Purpose:

Argument 1: Bndry *Ubc
Purpose:

Argument 2: double *bc
Purpose:

Function Notes:

*/

void Tri::setbcs(Bndry *Ubc,double *bc){
  if(Ubc->type == 'W' || Ubc->type == 'V' || Ubc->type == 'M' ||
     Ubc->type == 'v' || Ubc->type == 'o' || Ubc->type == 'm' ||
     Ubc->type == 's' || (Ubc->type == 'B' && type == 'u')){

    double scal = (Ubc->type != 'o' && Ubc->type != 's')
      ? dparam("BNDTIMEFCE"): 1.0;
    int     fac = Ubc->face;
    for(int i = 0; i < 2; ++i){
      if(!vert[vnum(fac,i)].solve)
  bc[vert[vnum(fac,i)].gid] = scal*Ubc->bvert[i];
    }
    if(edge[fac].l)
      dsmul(edge[fac].l,scal,Ubc->bedge[0],1,edge[fac].hj,1);
  }
}




void Quad::setbcs(Bndry *Ubc,double *bc){
  if(Ubc->type == 'W' || Ubc->type == 'V' || Ubc->type == 'M' ||
     Ubc->type == 'v' || Ubc->type == 'o' || Ubc->type == 'm' ||
     Ubc->type == 's' || (Ubc->type == 'B' && type == 'u')){

    double scal = (Ubc->type != 'o' && Ubc->type != 's')
      ? dparam("BNDTIMEFCE"): 1.0;
    int     fac = Ubc->face;
    for(int i = 0; i < 2; ++i){
      if(!vert[vnum(fac,i)].solve)
  bc[vert[vnum(fac,i)].gid] = scal*Ubc->bvert[i];
    }
    if(edge[fac].l)
      dsmul(edge[fac].l,scal,Ubc->bedge[0],1,edge[fac].hj,1);
  }
}




void Tet::setbcs(Bndry *Ubc,double *bc){
  if(Ubc->type == 'W' || Ubc->type == 'V' || Ubc->type == 'M' ||
     Ubc->type == 'v' || Ubc->type == 'o' || Ubc->type == 'm' ||
     Ubc->type == 's' || (Ubc->type == 'B' && type == 'u')){

    double scal = (Ubc->type != 'o')? dparam("BNDTIMEFCE"): 1.0;
    int    fac  = Ubc->face, i,j;
    Vert *v;    Edge *e;    Face *f;
    int    con;

    for(i = 0; i < Nverts-1; ++i){
      v = vert + vnum(fac,i);
      if(!v->solve)
  bc[v->gid] = scal*Ubc->bvert[i];
    }

    for(i = 0; i < Nfverts(fac); ++i){
      e = edge+ednum(fac,i);
      dcopy(e->l,Ubc->bedge[i],1,e->hj,1); /* need for single boundary */
      con = e->con;
      for(e = e->base;e;e = e->link)
  if(e->l){
    dsmul(e->l,scal,Ubc->bedge[i],1,e->hj,1);
    if(e->con!=con) for(j=1;j<e->l;j+=2) e->hj[j] *= -1;
  }
    }
    f = face + fac;
    if(f->l) dsmul(f->l*(f->l+1)/2, scal, *Ubc->bface, 1, *f->hj ,1);
  }
}




void Pyr::setbcs(Bndry *Ubc,double *bc){
  if(Ubc->type == 'W' || Ubc->type == 'V' || Ubc->type == 'M' ||
     Ubc->type == 'v' || Ubc->type == 'o' || Ubc->type == 'm' ||
     Ubc->type == 's' || (Ubc->type == 'B' && type == 'u')){

    double scal = (Ubc->type != 'o')? dparam("BNDTIMEFCE"): 1.0;
    int    fac  = Ubc->face, i,j;
    Vert *v;    Edge *e;    Face *f;
    int    con;

    for(i = 0; i < Nfverts(fac); ++i){
      v = vert + vnum(fac,i);
      if(!v->solve)
  bc[v->gid] = scal*Ubc->bvert[i];
    }

    for(i = 0; i < Nfverts(fac); ++i){
      e = edge+ednum(fac,i);
      dcopy(e->l,Ubc->bedge[i],1,e->hj,1); /* need for single boundary */
      con = e->con;
      for(e = e->base;e;e = e->link)
  if(e->l){
    dsmul(e->l,scal,Ubc->bedge[i],1,e->hj,1);
    if(e->con!=con) for(j=1;j<e->l;j+=2) e->hj[j] *= -1.0;
  }
    }
    f = face + fac;
    if(f->l)
      if(Nfverts(fac) == 4)
  dsmul(f->l*f->l, scal, *Ubc->bface, 1, *f->hj ,1);
      else
  dsmul(f->l*(f->l+1)/2, scal, *Ubc->bface, 1, *f->hj ,1);
  }
}




void Prism::setbcs(Bndry *Ubc,double *bc){
  if(Ubc->type == 'W' || Ubc->type == 'V' || Ubc->type == 'M' ||
     Ubc->type == 'v' || Ubc->type == 'o' || Ubc->type == 'm' ||
     Ubc->type == 's' || (Ubc->type == 'B' && type == 'u')){

    double scal = (Ubc->type != 'o')? dparam("BNDTIMEFCE"): 1.0;
    int    fac  = Ubc->face, i,j;
    Vert *v;    Edge *e;    Face *f;
    int    con;

    for(i = 0; i < Nfverts(fac); ++i){
      v = vert + vnum(fac,i);
      if(!v->solve)
  bc[v->gid] = scal*Ubc->bvert[i];
    }

    for(i = 0; i < Nfverts(fac); ++i){
      e = edge+ednum(fac,i);
      dcopy(e->l,Ubc->bedge[i],1,e->hj,1); /* need for single boundary */
      con = e->con;
      for(e = e->base;e;e = e->link)
  if(e->l){
    dsmul(e->l,scal,Ubc->bedge[i],1,e->hj,1);
    if(e->con!=con) for(j=1;j<e->l;j+=2) e->hj[j] *= -1.0;
  }
    }
    f = face + fac;
    if(f->l)
      if(Nfverts(fac) == 4)
  dsmul(f->l*f->l, scal, *Ubc->bface, 1, *f->hj ,1);
      else
  dsmul(f->l*(f->l+1)/2, scal, *Ubc->bface, 1, *f->hj ,1);
  }
}




void Hex::setbcs(Bndry *Ubc,double *bc){
  if(Ubc->type == 'W' || Ubc->type == 'V' || Ubc->type == 'M' ||
     Ubc->type == 'v' || Ubc->type == 'o' || Ubc->type == 'm' ||
     Ubc->type == 's' || (Ubc->type == 'B' && type == 'u')){

    double scal = (Ubc->type != 'o')? dparam("BNDTIMEFCE"): 1.0;
    int    fac  = Ubc->face, i,j;
    Vert *v;    Edge *e;    Face *f;
    int    con;

    for(i = 0; i < Nfverts(fac); ++i){
      v = vert + vnum(fac,i);
      if(!v->solve)
  bc[v->gid] = scal*Ubc->bvert[i];
    }

    for(i = 0; i < Nfverts(fac); ++i){
      e = edge+ednum(fac,i);
      dcopy(e->l,Ubc->bedge[i],1,e->hj,1); /* need for single boundary */
      con = e->con;
      for(e = e->base;e;e = e->link)
  if(e->l){
    dsmul(e->l,scal,Ubc->bedge[i],1,e->hj,1);
    if(e->con!=con) for(j=1;j<e->l;j+=2) e->hj[j] *= -1.0;
  }
    }
    f = face + fac;
    if(f->l) dsmul(f->l*f->l, scal, *Ubc->bface, 1, *f->hj ,1);
  }
}




void Element::setbcs(Bndry *,double *){ERR;}




/*

Function name: Element::Add_flux_terms

Function Purpose:

Argument 1: Bndry *Ebc
Purpose:

Function Notes:

*/

void Tri::Add_flux_terms(Bndry *Ebc){
  if(Ebc->type == 'F' || Ebc->type == 'R' || Ebc->type == 'S' ||
     (Ebc->type == 'B' && type == 'v')){
    int fac = Ebc->face;
    for(int i = 0; i < Tri_DIM; ++i)
      vert[vnum(fac,i)].hj[0] += Ebc->bvert[i];

    dvadd(edge[fac].l, Ebc->bedge[0], 1, edge[fac].hj, 1, edge[fac].hj, 1);
  }
}




void Quad::Add_flux_terms(Bndry *Ebc){
  if(Ebc->type == 'F' || Ebc->type == 'R' || Ebc->type == 'S' ||
     (Ebc->type == 'B' && type == 'v')){
    int fac = Ebc->face;
    for(int i = 0; i < Quad_DIM; ++i)
      vert[vnum(fac,i)].hj[0] += Ebc->bvert[i];

    dvadd(edge[fac].l, Ebc->bedge[0], 1, edge[fac].hj, 1, edge[fac].hj, 1);
  }
}




void Tet::Add_flux_terms(Bndry *Ebc){
  int i;
  Edge *e;
  Face *f;

  if(Ebc->type == 'F' || Ebc->type == 'R' || Ebc->type == 'S' ||
     (Ebc->type == 'B' && type == 'v')){
    int fac = Ebc->face;
    for(i = 0; i < Nfverts(fac) ; ++i)
      vert[vnum(fac,i)].hj[0] += Ebc->bvert[i];

    for(i = 0; i < Nfverts(fac); ++i){
      e   = edge+ednum(fac,i);
      dvadd(e->l, Ebc->bedge[i], 1, e->hj, 1, e->hj, 1);
    }
    f = face + fac;
    if(f->l) dvadd(f->l*(f->l+1)/2, *Ebc->bface, 1, *f->hj, 1, *f->hj, 1);
  }
}




void Pyr::Add_flux_terms(Bndry *Ebc){
  Edge *e;
  Face *f;

  if(Ebc->type == 'F' || Ebc->type == 'R'){
    int fac = Ebc->face;
    int nfv = Nfverts(Ebc->face), i;

    for(i = 0; i < nfv ; ++i)
      vert[vnum(fac,i)].hj[0] += Ebc->bvert[i];

    for(i = 0; i < nfv; ++i){
      e   = edge+ednum(fac,i);
      dvadd(e->l, Ebc->bedge[i], 1, e->hj, 1, e->hj, 1);
    }

    f = face + fac;
    if(f->l)
      if(nfv == 3)
  dvadd(f->l*(f->l+1)/2, *Ebc->bface, 1, *f->hj, 1, *f->hj, 1);
      else
        dvadd(f->l*f->l,       *Ebc->bface, 1, *f->hj, 1, *f->hj, 1);
  }
}




void Prism::Add_flux_terms(Bndry *Ebc){
  if(Ebc->type == 'F' || Ebc->type == 'R'){
    Edge *e;
    Face *f;

    int fac = Ebc->face;
    int nfv = Nfverts(fac), i;

    for(i = 0; i < nfv ; ++i)
      vert[vnum(fac,i)].hj[0] += Ebc->bvert[i];

    for(i = 0; i < nfv; ++i){
      e   = edge+ednum(fac,i);
      dvadd(e->l, Ebc->bedge[i], 1, e->hj, 1, e->hj, 1);
    }

    f = face + fac;
    i = (nfv == 3) ? f->l*(f->l+1)/2 : f->l*f->l;
    if(i)
      dvadd(i, *Ebc->bface, 1, *f->hj, 1, *f->hj, 1);
  }
}




void Hex::Add_flux_terms(Bndry *Ebc){
  Edge *e;
  Face *f;

  if(Ebc->type == 'F' || Ebc->type == 'R'){
    int fac = Ebc->face;
    int nfv = Nfverts(fac), i;

    for(i = 0; i < nfv ; ++i)
      vert[vnum(fac,i)].hj[0] += Ebc->bvert[i];

    for(i = 0; i < nfv; ++i){
      e   = edge+ednum(fac,i);
      dvadd(e->l, Ebc->bedge[i], 1, e->hj, 1, e->hj, 1);
    }

    f = face + fac;
    if(f->l)
      dvadd(f->l*f->l, *Ebc->bface, 1, *f->hj, 1, *f->hj, 1);
  }
}




void Element::Add_flux_terms(Bndry *){ERR;}




/*

Function name: Element::MakeFlux

Function Purpose:

Argument 1: Bndry *B
Purpose:

Argument 2: int iface
Purpose:

Argument 3: double *f
Purpose:

Function Notes:

*/

void Tri::MakeFlux(Bndry *B, int iface, double *f){
  register int i;
  const    int fac = B->face;
  const    int L = edge[fac].l;
  double   *wa,*fi;
  double   *tmp;
  Mode     *m;
  Basis    *Base = getbasis();

  tmp = dvector(0,qa-1);
  fi  = dvector(0,qa-1);
  getzw(qa,&wa,&wa,'a');

  m   = Base->edge[0];

  InterpToFace1(iface, f, fi);

  if(curve)
    dvmul(qa,B->sjac.p,1,fi,1,fi,1);
  else
    dscal(qa,B->sjac.d,fi,1);

  /* calculate inner product over surface */
  dvmul(qa,Base->vert[0].a,1,wa,1,tmp,1);
  B->bvert[0] = ddot(qa,tmp,1,fi,1);
  dvmul(qa,Base->vert[1].a,1,wa,1,tmp,1);
  B->bvert[1] = ddot(qa,tmp,1,fi,1);

  for(i = 0; i < L; ++i){
    dvmul(qa,m[i].a,1,wa,1,tmp,1);
    B->bedge[0][i] = ddot(qa,tmp,1,fi,1);
  }

  free(tmp);
  free(fi);
}




void Quad::MakeFlux(Bndry *B, int iface, double *f){
  register int i;
  const    int fac = B->face;
  const    int L = edge[fac].l;
  double   *wa,*tmp,*fi;
  Mode     *m;
  Basis    *Base = getbasis();

  tmp = dvector(0,qa-1);
  fi  = dvector(0,qa-1);
  getzw(qa,&wa,&wa,'a');

  m   = Base->edge[0];

  InterpToFace1(iface, f, fi);

  dvmul(qa,B->sjac.p,1,fi,1,fi,1);

  /* calculate inner product over surface */
  dvmul(qa,Base->vert[0].a,1,wa,1,tmp,1);
  B->bvert[0] = ddot(qa,tmp,1,fi,1);
  dvmul(qa,Base->vert[1].a,1,wa,1,tmp,1);
  B->bvert[1] = ddot(qa,tmp,1,fi,1);

  for(i = 0; i < L; ++i){
    dvmul(qa,m[i].a,1,wa,1,tmp,1);
    B->bedge[0][i] = ddot(qa,tmp,1,fi,1);
  }

  free(tmp); free(fi);
}




void Tet::MakeFlux(Bndry *B, int iface, double *f){
  register  int i,j,k;
  const     int fac = B->face;
  int       l;
  double   *wa,*wb,*tmp,*tmp1,**s;
  Basis    *Base = getbasis();
  Mode     *m,**m1;

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax-1);
  tmp1 = dvector(0,QGmax-1);

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');


  if(curvX)
    dvmul(qa*qb,B->sjac.p,1,f,1,f,1);
  else
    dsmul(qa*qb,B->sjac.d,f,1,f,1);

  /* Weak flux of vertices */
  for(i = 0; i < Nfverts(fac); ++i){
    m = Base->vert+i;
    dvmul(qa,wa,1,m->a,1,tmp,1);
    for(j = 0; j < qb; ++j) tmp1[j] = ddot(qa,tmp,1,f+qa*j,1);
    dvmul(qb,wb,1,tmp1,1,tmp1,1);
    B->bvert[i] = ddot(qb,m->b,1,tmp1,1);
  }

  /* Weak flux of edges */
  for(i = 0; i < Nfverts(fac); ++i){
    l = B->elmt->edge[ednum(fac,i)].l;
    m = Base->edge[i];
    for(j = 0; j < l; ++j){
      dvmul(qa,wa,1,m[j].a,1,tmp,1);
      for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,tmp,1,f+k*qa,1);
      dvmul(qb,wb,1,tmp1,1,tmp1,1);
      B->bedge[i][j] = ddot(qb,m[j].b,1,tmp1,1);
    }
  }

  /* Weak flux of interior face modes */
  l  = B->elmt->face[fac].l;
  s  = B->bface;
  m1 = Base->face[0];
  for(i = 0; i < l; ++i){
    dvmul(qa-2,wa+1,1,m1[i][0].a+1,1,tmp,1);
    for(j = 1; j < qb; ++j) tmp1[j-1] = ddot(qa-2,tmp,1,f+qa*j+1,1);

    dvmul(qb-1,wb+1,1,tmp1,1,tmp1,1);
    for(j = 0; j < l-i; ++j)
      s[i][j] = ddot(qb-1,m1[i][j].b+1,1,tmp1,1);
  }

  free(tmp1); free(tmp); // free(f);
}




void Pyr::MakeFlux(Bndry *B, int iface, double *f){
  register  int i,j,k;
  const     int fac = B->face;
  int       l;
  double   *wa,*wb,*wc,*tmp,*tmp1,**s;
  Basis    *Base = getbasis();
  Mode     *m,**m1;

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax-1);
  tmp1 = dvector(0,QGmax-1);
  //   fi   = dvector(0,QGmax*QGmax-1);

  //  InterpToFace1(iface, f, fi);

  if(Nfverts(fac) == 4){
    getzw(qa,&wa,&wa,'a');
    getzw(qb,&wb,&wb,'a');

    dvmul(qa*qb,B->sjac.p,1,f,1,f,1);

    for(j = 0; j < qb; ++j) dvmul(qa, wa, 1, f+j*qa, 1, f+j*qa, 1);
    for(j = 0; j < qb; ++j) dscal(qa, wb[j], f+j*qa, 1);

    /* Weak flux of vertices */
    for(i = 0; i < Nfverts(fac); ++i){
      m = Base->vert+i;

      for(j = 0; j < qb; ++j) tmp1[j] = ddot(qa,m->a,1,f+qa*j,1);
      B->bvert[i] = ddot(qb,m->b,1,tmp1,1);
    }

    /* Weak flux of edges */
    for(i = 0; i < Nfverts(fac); ++i){
      l = B->elmt->edge[ednum(fac,i)].l;
      m = Base->edge[i];
      for(j = 0; j < l; ++j){
  for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,m[j].a,1,f+qa*k,1);
  B->bedge[i][j] = ddot(qb,m[j].b,1,tmp1,1);
      }
    }

    /* Weak flux of interior face modes */
    l  = B->elmt->face[fac].l;
    s  = B->bface;
    m1 = Base->face[0];
    for(i = 0; i < l; ++i)
      for(j = 0; j < l; ++j){
  for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,m1[i][j].a,1,f+qa*k,1);
  s[i][j] = ddot(qb,m1[i][j].b,1,tmp1,1);
      }
  }
  else{
    getzw(qa,&wa,&wa,'a');
    getzw(qc,&wc,&wc,'c');

    dvmul(qa*qc,B->sjac.p,1,f,1,f,1);

    for(j = 0; j < qc; ++j) dvmul(qa, wa, 1, f+j*qa, 1, f+j*qa, 1);
    for(j = 0; j < qc; ++j) dscal(qa, wc[j], f+j*qa, 1);

    /* Weak flux of vertices */
    for(i = 0; i < Nfverts(fac); ++i){
      m = Base->vert+vnum(fac,i);

      for(j = 0; j < qc; ++j) tmp1[j] = ddot(qa,m->a,1,f+qa*j,1);
      B->bvert[i] = ddot(qc,m->c,1,tmp1,1);
    }

    /* Weak flux of edges */
    for(i = 0; i < Nfverts(fac); ++i){
      l = B->elmt->edge[ednum(fac,i)].l;
      m = Base->edge[ednum(fac,i)];
      for(j = 0; j < l; ++j){
  for(k = 0; k < qc; ++k) tmp1[k] = ddot(qa,m[j].a,1,f+k*qa,1);
  B->bedge[i][j] = ddot(qc,m[j].c,1,tmp1,1);
      }
    }

    /* Weak flux of interior face modes */
    l  = B->elmt->face[fac].l;
    s  = B->bface;
    m1 = Base->face[1];
    for(i = 0; i < l; ++i)
      for(j = 0; j < l-i; ++j){
  for(k = 0; k < qc; ++k) tmp1[k] = ddot(qa,m1[i][j].a,1,f+qa*k,1);
  s[i][j] = ddot(qc,m1[i][j].c,1,tmp1,1);
      }
  }

  free(tmp1); free(tmp); // free(fi);
}




void Prism::MakeFlux(Bndry *B, int iface, double *f){
  register  int i,j,k;
  const     int fac = B->face;
  int       l, nfv;
  double   *wa,*wb,*wc,*tmp1,**s;
  Basis    *Base = getbasis();
  Mode     *m,**m1;

  /* sort vertices and edges*/
  tmp1 = dvector(0,QGmax-1);
  nfv = Nfverts(fac);

  if(nfv == 4){
    getzw(qa,&wa,&wa,'a');
    getzw(qb,&wb,&wb,'a');

    dvmul(qa*qb,B->sjac.p,1,f,1,f,1);

    for(j = 0; j < qb; ++j) dvmul(qa, wa, 1, f+j*qa, 1, f+j*qa, 1);
    for(j = 0; j < qb; ++j) dscal(qa, wb[j], f+j*qa, 1);

    /* Weak flux of vertices */
    for(i = 0; i < Nfverts(fac); ++i){
      m = Base->vert+i;

      for(j = 0; j < qb; ++j) tmp1[j] = ddot(qa,m->a,1,f+qa*j,1);
      B->bvert[i] = ddot(qb,m->b,1,tmp1,1);
    }

    /* Weak flux of edges */
    for(i = 0; i < Nfverts(fac); ++i){
      l = B->elmt->edge[ednum(fac,i)].l;
      m = Base->edge[i];
      for(j = 0; j < l; ++j){
  for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,m[j].a,1,f+qa*k,1);
  B->bedge[i][j] = ddot(qb,m[j].b,1,tmp1,1);
      }
    }

    /* Weak flux of interior face modes */
    l  = B->elmt->face[fac].l;
    s  = B->bface;
    m1 = Base->face[0];
    for(i = 0; i < l; ++i)
      for(j = 0; j < l; ++j){
  for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,m1[i][j].a,1,f+qa*k,1);
  s[i][j] = ddot(qb,m1[i][j].b,1,tmp1,1);
      }
  }
  else{
    getzw(qa,&wa,&wa,'a');
    getzw(qc,&wc,&wc,'b');

    dvmul(qa*qc,B->sjac.p,1,f,1,f,1);

    for(j = 0; j < qc; ++j) dvmul(qa, wa, 1, f+j*qa, 1, f+j*qa, 1);
    for(j = 0; j < qc; ++j) dscal(qa, wc[j], f+j*qa, 1);

    /* Weak flux of vertices */
    for(i = 0; i < nfv; ++i){
      m = Base->vert+vnum(fac,i);

      for(j = 0; j < qc; ++j) tmp1[j] = ddot(qa,m->a,1,f+qa*j,1);
      B->bvert[i] = ddot(qc,m->c,1,tmp1,1);
    }

    /* Weak flux of edges */
    for(i = 0; i < nfv; ++i){
      l = B->elmt->edge[ednum(fac,i)].l;
      m = Base->edge[ednum(fac,i)];
      for(j = 0; j < l; ++j){
  for(k = 0; k < qc; ++k) tmp1[k] = ddot(qa,m[j].a,1,f+k*qa,1);
  B->bedge[i][j] = ddot(qc,m[j].c,1,tmp1,1);
      }
    }

    /* Weak flux of interior face modes */
    l  = B->elmt->face[fac].l;
    s  = B->bface;
    m1 = Base->face[1];
    for(i = 0; i < l; ++i)
      for(j = 0; j < l-i; ++j){
  for(k = 0; k < qc; ++k) tmp1[k] = ddot(qa,m1[i][j].a,1,f+qa*k,1);
  s[i][j] = ddot(qc,m1[i][j].c,1,tmp1,1);
      }
  }

  free(tmp1);
}




void Hex::MakeFlux(Bndry *B, int iface, double *f){
  register  int i,j,k;
  const     int fac = B->face;
  int       l;
  double   *wa,*wb,*tmp,*tmp1,**s;
  Basis    *Base = getbasis();
  Mode     *m,**m1;

  /* sort vertices and edges*/
  tmp  = dvector(0,QGmax-1);
  tmp1 = dvector(0,QGmax-1);
  //  fi   = dvector(0,QGmax*QGmax-1);

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');

  //  InterpToFace1(iface, f, fi);

  dvmul(qa*qb,B->sjac.p,1,f,1,f,1);

  for(j = 0; j < qb; ++j) dvmul(qa, wa, 1, f+j*qa, 1, f+j*qa, 1);
  for(j = 0; j < qb; ++j) dscal(qa, wb[j], f+j*qa, 1);

  /* Weak flux of vertices */
  for(i = 0; i < Nfverts(fac); ++i){
    m = Base->vert+i;

    for(j = 0; j < qb; ++j) tmp1[j] = ddot(qa,m->a,1,f+qa*j,1);
    B->bvert[i] = ddot(qb,m->b,1,tmp1,1);
  }

  /* Weak flux of edges */
  for(i = 0; i < Nfverts(fac); ++i){
    l = B->elmt->edge[ednum(fac,i)].l;
    m = Base->edge[i];
    for(j = 0; j < l; ++j){
      for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,m[j].a,1,f+qa*k,1);
      B->bedge[i][j] = ddot(qb,m[j].b,1,tmp1,1);
    }
  }

  /* Weak flux of interior face modes */
  l  = B->elmt->face[fac].l;
  s  = B->bface;
  m1 = Base->face[0];
  for(i = 0; i < l; ++i)
    for(j = 0; j < l; ++j){
      for(k = 0; k < qb; ++k) tmp1[k] = ddot(qa,m1[i][j].a,1,f+qa*k,1);
      s[i][j] = ddot(qb,m1[i][j].b,1,tmp1,1);
    }

  free(tmp1); free(tmp);  // free(fi);
}




void Element::MakeFlux(Bndry *, int , double *){ERR;}



/*

Function name: Element::Add_Surface_Contrib

Function Purpose:

Argument 1: Element *Ef
Purpose:

Argument 2: double *in
Purpose:

Argument 3: char dir
Purpose:

Function Notes:

*/

void Tri::Add_Surface_Contrib(Element *Ef, double *in, char dir){
  Tri::Add_Surface_Contrib(Ef,in,dir,-1,1);
}


void Tri::Add_Surface_Contrib(Element *Ef, double *in, char dir, int edg){
  Tri::Add_Surface_Contrib(Ef,in,dir,-1,1);
}

void Tri::Add_Surface_Contrib(Element *Ef, double *in, char dir, int edg,
            int invjac){
  int      qedga,qedgb,i;
  Element *E = this;
  double   *za,*zb,*wa,*wb;
  double   **ima, **imb;
  double one = 1.0e0 , two = 2.0e0;
  double *f  = Tri_wk.get();
  double *fi = f+QGmax;

  getzw(qa,&za,&wa,'a');
  getzw(qb,&zb,&wb,'b');

  if((edg == 0)|| (edg == -1)){
    /* add surface contribition of edge 1 */
    qedga = edge[0].qedg;
    getim(qedga,qa,&ima,g2a);

    /* gather edge 1 and multiply by edge Jacobian/face Jacobian */
    dvsub(qedga,Ef->edge[0].h,1,E->edge[0].h,1,f,1);
    dvmul(qedga,E->edge[0].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*ima,f,qedga,fi,qa);

    /* divide by wb[0] */
    dscal(qa,1.0/wb[0],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p,1,fi,1);
      else
  dscal(qa,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qa,fi,1,in,1,in,1);
  }

  if((edg == 1|| edg == -1)){
    /* add surface contribition of edge 2 */
    qedgb = E->edge[1].qedg;
    getim(qedgb,qb,&imb,g2b);

    /* gather edge 2 and multiply by jacobian */
    dvsub(qedgb,Ef->edge[1].h,1,E->edge[1].h,1,f,1);
    dvmul(qedgb,E->edge[1].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->y[i];

    /* interpolate to guass-radau points */
    Interp(*imb,f,qedgb,fi,qb);

    for(i = 0; i < qb; ++i)
      f[i] = fi[i]*two/(wa[qa-1]*(one-zb[i]));

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,f,1,geom->jac.p+qa-1,qa,f,1);
      else
  dscal(qb,1/geom->jac.d,f,1);

    /* add to Ef physical mode storage */
    dvadd(qb,f,1,in + qa-1,qa,in + qa-1,qa);
  }


  if((edg == 2|| edg == -1)){
    /* add surface contribition of edge 3 */
    qedgb = E->edge[2].qedg;
    getim(qedgb,qb,&imb,g2b);

    /* gather edge 3 and multiply by jacobian */
    dvsub(qedgb,Ef->edge[2].h,1,E->edge[2].h,1,f,1);
    dvmul(qedgb,E->edge[2].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[2].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[2].norm->y[i];

    /* interpolate to guass-radau points */
    Interp(*imb,f,qedgb,fi,qb);


    for(i = 0; i < qb; ++i)
      f[i] = fi[i]*two/(wa[0]*(one-zb[i]));

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,f,1,geom->jac.p,qa,f,1);
      else
  dscal(qb,1/geom->jac.d,f,1);

    /* add to Ef physical mode storage */
    dvadd(qb,f,1,in,qa,in,qa);
  }

  return;
}


/* Add surface value stored in edge[edg].h into vector in so that
   on taking the inner product you calculate both the function and its
   flux

   if edg = -1 then all edges are evaluated
*/

void Tri::Add_Surface_Contrib(double *in, char dir, int edg){
  Add_Surface_Contrib(in,dir, edg,1);
}

void Tri::Add_Surface_Contrib(double *in, char dir, int edg, int invjac){
  int      qedga,qedgb,i;
  Element *E = this;
  double   *za,*zb,*wa,*wb;
  double   **ima, **imb;
  double one = 1.0e0 , two = 2.0e0;
  double *f  = Tri_wk.get();
  double *fi = f+QGmax;

  getzw(qa,&za,&wa,'a');
  getzw(qb,&zb,&wb,'b');

  if((edg == 0)|| (edg == -1)){
    /* add surface contribition of edge 1 */
    qedga = edge[0].qedg;
    getim(qedga,qa,&ima,g2a);

    /* gather edge 1 and ultiply by edge Jacobian/face Jacobian */
    dcopy(qedga,E->edge[0].h,1,f,1);
    dvmul(qedga,E->edge[0].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*ima,f,qedga,fi,qa);

    /* divide by wb[0] */
    dscal(qa,1.0/wb[0],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p,1,fi,1);
      else
  dscal(qa,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qa,fi,1,in,1,in,1);
  }

  if((edg == 1|| edg == -1)){
    /* add surface contribition of edge 2 */
    qedgb = E->edge[1].qedg;
    getim(qedgb,qb,&imb,g2b);

    /* gather edge 2 and multiply by jacobian */
    dcopy(qedgb,E->edge[1].h,1,f,1);
    dvmul(qedgb,E->edge[1].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->y[i];

    /* interpolate to guass-radau points */
    Interp(*imb,f,qedgb,fi,qb);

    for(i = 0; i < qb; ++i)
      f[i] = fi[i]*two/(wa[qa-1]*(one-zb[i]));

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,f,1,geom->jac.p+qa-1,qa,f,1);
      else
  dscal(qb,1/geom->jac.d,f,1);

    /* add to Ef physical mode storage */
    dvadd(qb,f,1,in + qa-1,qa,in + qa-1,qa);
  }


  if((edg == 2|| edg == -1)){
    /* add surface contribition of edge 3 */
    qedgb = E->edge[2].qedg;
    getim(qedgb,qb,&imb,g2b);

    /* gather edge 3 and multiply by jacobian */
    dcopy(qedgb,E->edge[2].h,1,f,1);
    dvmul(qedgb,E->edge[2].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[2].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[2].norm->y[i];

    /* interpolate to guass-radau points */
    Interp(*imb,f,qedgb,fi,qb);

    for(i = 0; i < qb; ++i)
      f[i] = fi[i]*two/(wa[0]*(one-zb[i]));

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,f,1,geom->jac.p,qa,f,1);
      else
  dscal(qb,1/geom->jac.d,f,1);

    /* add to Ef physical mode storage */
    dvadd(qb,f,1,in,qa,in,qa);
  }
  return;
}


static double Quad_Penalty_Fac = 1.;

void Quad::Add_Surface_Contrib(Element *Ef, double *in, char dir){
  Add_Surface_Contrib(Ef,in,dir,-1,1);
}

void Quad::Add_Surface_Contrib(Element *Ef, double *in, char dir, int edg){
  Add_Surface_Contrib(Ef,in,dir,-1,1);
}

void Quad::Add_Surface_Contrib(Element *Ef, double *in, char dir, int edg,
             int invjac){
  int      qedga,qedgb,i;
  Element *E = this;
  double   *za,*zb,*wa,*wb;
  double   **ima, **imb;

  double *f  = Quad_wk;
  double *fi = Quad_wk+QGmax;

  getzw(qa,&za,&wa,'a'); // ok
  getzw(qb,&zb,&wb,'a'); // ok

  if((edg == 0|| edg == -1)){
    /* add surface contribition of edge 1 */
    qedga = E->edge[0].qedg;
    getim(qedga,qa,&ima,g2a);

    /* gather edge 1 and multiply by jacobian */
    dvsub(qedga,Ef->edge[0].h,1,E->edge[0].h,1,f,1);
    dvmul(qedga,E->edge[0].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*ima,f,qedga,fi,qa);

    /* divide by wb[0] */
    //  dscal(qa,1.0/wb[0],fi,1);
    dscal(qa,Quad_Penalty_Fac/wb[0],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p,1,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qa,fi,1,in,1,in,1);
  }

  if((edg == 2|| edg == -1)){
    /* add surface contribition of edge 3 */
    qedga = E->edge[2].qedg;
    getim(qedga,qa,&ima,g2a);

    /* gather edge 3 and multiply by jacobian */
    dvsub(qedga,Ef->edge[2].h,1,E->edge[2].h,1,f,1);
    dvmul(qedga,E->edge[2].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[2].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedga; ++i)
      f[i] *= E->edge[2].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*ima,f,qedga,fi,qa);

    /* divide by wb[0] */
    //  dscal(qa,1.0/wb[qb-1],fi,1);
    dscal(qa,Quad_Penalty_Fac/wb[qb-1],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p+qa*(qb-1),1,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qa,fi,1,in+qa*(qb-1),1,in+qa*(qb-1),1);
  }
    // ---------------------------------------------

  if((edg == 1|| edg == -1)){
    /* add surface contribition of edge 2 */
    qedgb = E->edge[1].qedg;
    getim(qedgb,qb,&imb,g2a);

    /* gather edge 2 and multiply by jacobian */
    dvsub(qedgb,Ef->edge[1].h,1,E->edge[1].h,1,f,1);
    dvmul(qedgb,E->edge[1].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*imb,f,qedgb,fi,qb);

    /* divide by wb[0] */
    //  dscal(qb,1.0/wa[qa-1],fi,1);
    dscal(qb,Quad_Penalty_Fac/wa[qa-1],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p+qa-1,qa,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qb,fi,1,in+qa-1,qa,in+qa-1,qa);
  }

  if((edg == 3|| edg == -1)){
    /* add surface contribition of edge 4 */
    qedgb = E->edge[3].qedg;
    getim(qedgb,qb,&imb,g2a);

    /* gather edge 4 and multiply by jacobian */
    dvsub(qedgb,Ef->edge[3].h,1,E->edge[3].h,1,f,1);
    dvmul(qedgb,E ->edge[3].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[3].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[3].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*imb,f,qedgb,fi,qb);

    /* divide by wb[0] */
    //  dscal(qb,1.0/wa[0],fi,1);
    dscal(qb,Quad_Penalty_Fac/wa[0],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p,qa,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qb,fi,1,in,qa,in,qa);
  }

  return;
}

void Quad::Add_Surface_Contrib(double *in, char dir, int edg){
  Add_Surface_Contrib(in,dir,edg, 1);
}

void Quad::Add_Surface_Contrib(double *in, char dir, int edg, int invjac){
  int      qedga,qedgb,i;
  Element *E = this;
  double   *za,*zb,*wa,*wb;
  double   **ima, **imb;

  double *f  = Quad_wk;
  double *fi = Quad_wk+QGmax;

  getzw(qa,&za,&wa,'a'); // ok
  getzw(qb,&zb,&wb,'a'); // ok

  if((edg == 0|| edg == -1)){
    /* add surface contribition of edge 1 */
    qedga = E->edge[0].qedg;
    getim(qedga,qa,&ima,g2a);

    /* gather edge 1 and multiply by jacobian */
    dcopy(qedga,E->edge[0].h,1,f,1);
    dvmul(qedga,E->edge[0].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[0].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*ima,f,qedga,fi,qa);

    /* divide by wb[0] */
    //  dscal(qa,1.0/wb[0],fi,1);
    dscal(qa,Quad_Penalty_Fac/wb[0],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p,1,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qa,fi,1,in,1,in,1);
  }

  if((edg == 2|| edg == -1)){
    /* add surface contribition of edge 3 */
    qedga = E->edge[2].qedg;
    getim(qedga,qa,&ima,g2a);

    /* gather edge 3 and multiply by jacobian */
    dcopy(qedga,E->edge[2].h,1,f,1);
    dvmul(qedga,E->edge[2].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedga; ++i)
  f[i] *= E->edge[2].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedga; ++i)
      f[i] *= E->edge[2].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*ima,f,qedga,fi,qa);

    /* divide by wb[0] */
    //  dscal(qa,1.0/wb[qb-1],fi,1);
    dscal(qa,Quad_Penalty_Fac/wb[qb-1],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p+qa*(qb-1),1,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qa,fi,1,in+qa*(qb-1),1,in+qa*(qb-1),1);
  }
    // ---------------------------------------------

  if((edg == 1|| edg == -1)){
    /* add surface contribition of edge 2 */
    qedgb = E->edge[1].qedg;
    getim(qedgb,qb,&imb,g2a);

    /* gather edge 2 and multiply by jacobian */
    dcopy(qedgb,E->edge[1].h,1,f,1);
    dvmul(qedgb,E->edge[1].jac,1,f,1,f,1);

    /* multiply by appropriate component of normal */
    if(dir == 'x')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->x[i];
    else if(dir == 'y')
      for(i =0; i < qedgb; ++i)
  f[i] *= E->edge[1].norm->y[i];

    /* interpolate to guass lobatto points */
    Interp(*imb,f,qedgb,fi,qb);

    /* divide by wa[qa-1] */
    //  dscal(qb,1.0/wa[qa-1],fi,1);
    dscal(qb,Quad_Penalty_Fac/wa[qa-1],fi,1);

    if(invjac == 0) // divide by area Jacobian
      if(curvX)
  dvdiv(qa,fi,1,geom->jac.p+qa-1,qa,fi,1);
      else
  dscal(qb,1/geom->jac.d,fi,1);

    /* add to Ef physical mode storage */
    dvadd(qb,fi,1,in+qa-1,qa,in+qa-1,qa);
  }

    if((edg == 3|| edg == -1)){
      /* add surface contribition of edge 4 */
      qedgb = E->edge[3].qedg;
      getim(qedgb,qb,&imb,g2a);

      /* gather edge 4 and multiply by jacobian */
      dcopy(qedgb,E->edge[3].h,1,f,1);
      dvmul(qedgb,E->edge[3].jac,1,f,1,f,1);

      /* multiply by appropriate component of normal */
      if(dir == 'x')
  for(i =0; i < qedgb; ++i)
    f[i] *= E->edge[3].norm->x[i];
      else if(dir == 'y')
  for(i =0; i < qedgb; ++i)
    f[i] *= E->edge[3].norm->y[i];

      /* interpolate to guass lobatto points */
      Interp(*imb,f,qedgb,fi,qb);

      /* divide by wb[0] */
      //  dscal(qb,1.0/wa[0],fi,1);
      dscal(qb,Quad_Penalty_Fac/wa[0],fi,1);

      if(invjac == 0) // divide by area Jacobian
  if(curvX)
  dvdiv(qa,fi,1,geom->jac.p,qa,fi,1);
  else
    dscal(qb,1/geom->jac.d,fi,1);

      /* add to Ef physical mode storage */
      dvadd(qb,fi,1,in,qa,in,qa);
    }

  return;
}


void Tet::Add_Surface_Contrib(Element *Ef, double *out, char dir, int edge ){
  fprintf(stderr,"Error:Add_Surface_Contrib\n");
  exit(-1);
 }

void Tet::Add_Surface_Contrib(double *out, char dir, int edge ){
  fprintf(stderr,"Error:Add_Surface_Contrib\n");
  exit(-1);
 }

void Tet::Add_Surface_Contrib(Element *Ef, double *out, char dir){
  double *za, *zb, *zc, *wa, *wb, *wc, **ima, **imb;
  double *f  = Tet_wk;
  double *fi = Tet_wk+QGmax*QGmax;
  int fq, qftot, i;
  Element *E = this;

  getzw(qa,&za,&wa,'a'); // ok
  getzw(qb,&zb,&wb,'b'); // ok
  getzw(qc,&zc,&wc,'c'); // ok

  /* add surface contribition of face 1 */
  fq = face[0].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qb,&imb,g2b);

  /* gather face 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[0].h,1,E->face[0].h,1,f,1);
  dvmul(qftot,E->face[0].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[0].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[0].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[0].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);

  /* add to Ef physical mode storage */
  daxpy(qa*qb,1./wc[0], fi,1, out, 1);

  /* ********************************* */

  /* add surface contribition of face 2 */
  fq = face[1].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qc,&imb,g2c);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[1].h,1,E->face[1].h,1,f,1);
  dvmul(qftot,E->face[1].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[1].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[1].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[1].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);

  /* divide by wb[0] */
  dscal(qa*qc,1.0/wb[0],fi,1);

  /* add to Ef physical mode storage */
  for(i=0;i<qc;++i)
    daxpy(qa,2./(1.-zc[i]),fi+i*qa,1, out+i*qa*qb,1);


  /* ********************************* */
  /* add surface contribition of face 3 */
  fq = face[2].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2b);
  getim(fq,qc,&imb,g2c);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[2].h,1,E->face[2].h,1,f,1);
  dvmul(qftot,E->face[2].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[2].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[2].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[2].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);

  dscal(qb*qc,1.0/wa[qa-1],fi,1);

  /* add to Ef physical mode storage */
  for(i=0;i<qb;++i)
    dscal(qc, 2./(1.-zb[i]), fi+i, qb);

  for(i=0;i<qc;++i)
    daxpy(qb,2./(1.-zc[i]),fi+i*qb,1, out+i*qa*qb+qa-1,qa);

  /* ********************************** */
  /* add surface contribition of face 4 */
  fq = face[3].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2b);
  getim(fq,qc,&imb,g2c);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[3].h,1,E->face[3].h,1,f,1);
  dvmul(qftot,E->face[3].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[3].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[3].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[3].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);

  // FIX THESE
  dscal(qb*qc,1.0/wa[0],fi,1);

  /* add to Ef physical mode storage */

  for(i=0;i<qb;++i)
    dscal(qc, 2./(1.-zb[i]), fi+i, qb);

  for(i=0;i<qc;++i)
    daxpy(qb, 2./(1.-zc[i]),fi+i*qb,1, out+i*qa*qb,qa);

  return;
}


 void Pyr::Add_Surface_Contrib(Element *Ef, double *out, char dir, int edge){
  fprintf(stderr,"Error:Add_Surface_Contrib\n");
  exit(-1);
 }

 void Pyr::Add_Surface_Contrib(double *out, char dir, int edge){
  fprintf(stderr,"Error:Add_Surface_Contrib\n");
  exit(-1);
 }


 void Pyr::Add_Surface_Contrib(Element *Ef, double *in, char dir){
   fprintf(stderr,"Error:Add_Surface_Contrib\n");
   exit(-1);
   return;
 }



 void Prism::Add_Surface_Contrib(Element *Ef, double *out, char dir, int edge){
  fprintf(stderr,"Add_Surface_Contrib\n");
  exit(-1);
}

void Prism::Add_Surface_Contrib(double *out, char dir, int edge){
  fprintf(stderr,"Add_Surface_Contrib\n");
  exit(-1);
}

void Prism::Add_Surface_Contrib(Element *Ef, double *out, char dir){
  double *za, *zb, *zc, *wa, *wb, *wc, **ima, **imb;
  double *f  = Prism_wk;
  double *fi = Prism_wk+QGmax*QGmax;
  int fq, qftot, i;
  Element *E = this;

  getzw(qa,&za,&wa,'a'); // ok
  getzw(qb,&zb,&wb,'a'); // ok
  getzw(qc,&zc,&wc,'b'); // ok

  /* add surface contribition of face 1 */
  fq = face[0].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qb,&imb,g2a);

  /* gather face 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[0].h,1,E->face[0].h,1,f,1);
  dvmul(qftot,E->face[0].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[0].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[0].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[0].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);

  /* add to Ef physical mode storage */
  daxpy(qa*qb,1./wc[0],fi,1,out,1);


  /* ********************************* */
  /* add surface contribition of face 2 */
  fq = face[1].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qc,&imb,g2b);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[1].h,1,E->face[1].h,1,f,1);
  dvmul(qftot,E->face[1].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[1].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[1].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[1].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);

  /* divide by wb[0] */
  for(i=0;i<qc;++i)
    daxpy(qa,1.0/wb[0],fi+i*qa, 1,out +i*qa*qb,1);


  /* ********************************* */
  /* add surface contribition of face 3 */
  fq = face[2].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2a);
  getim(fq,qc,&imb,g2b);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[2].h,1,E->face[2].h,1,f,1);
  dvmul(qftot,E->face[2].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[2].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[2].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[2].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);

  /* add to Ef physical mode storage */

  for(i=0;i<qc;++i)
    daxpy(qb,2./(wa[qa-1]*(1.-zc[i])),fi+i*qb,1, out+i*qa*qb+qa-1,qa);


  /* ********************************** */
  /* add surface contribition of face 4 */
  fq = face[3].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2a);
  getim(fq,qc,&imb,g2b);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[3].h,1,E->face[3].h,1,f,1);
  dvmul(qftot,E->face[3].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[3].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[3].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[3].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);

  /* add to Ef physical mode storage */

  for(i=0;i<qc;++i)
    daxpy(qa,1./wb[qb-1],fi+i*qa,1, out+qa*(qb-1)+i*qa*qb,1);


  /* ********************************** */
  /* add surface contribition of face 5 */
  fq = face[4].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2a);
  getim(fq,qc,&imb,g2b);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[4].h,1,E->face[4].h,1,f,1);
  dvmul(qftot,E->face[4].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[4].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[4].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[4].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);

  /* add to Ef physical mode storage */

  for(i=0;i<qc;++i)
    daxpy(qb,2./(wa[0]*(1.-zc[i])),fi+i*qb,1, out+i*qa*qb,qa);

  return;
}


 void Hex::Add_Surface_Contrib(double *out, char dir, int edge){
  fprintf(stderr,"Add_Surface_Contrib\n");
  exit(-1);
}


 void Hex::Add_Surface_Contrib(Element *Ef, double *out, char dir, int edge){
  fprintf(stderr,"Add_Surface_Contrib\n");
  exit(-1);
}

void Hex::Add_Surface_Contrib(Element *Ef, double *out, char dir){
  double *za, *zb, *zc, *wa, *wb, *wc, **ima, **imb;
  double *f  = Hex_wk;
  double *fi = Hex_wk+QGmax*QGmax;
  int fq, qftot, i;
  Element *E = this;

  getzw(qa,&za,&wa,'a'); // ok
  getzw(qb,&zb,&wb,'a'); // ok
  getzw(qc,&zc,&wc,'a'); // ok

  /* add surface contribition of face 1 */
  fq = face[0].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qb,&imb,g2a);

  /* gather face 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[0].h,1,E->face[0].h,1,f,1);
  dvmul(qftot,E->face[0].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[0].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[0].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[0].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);

  /* add to Ef physical mode storage */
  daxpy(qa*qb,1./wc[0],fi,1,out,1);


  /* ********************************* */
  /* add surface contribition of face 2 */
  fq = face[1].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qc,&imb,g2a);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[1].h,1,E->face[1].h,1,f,1);
  dvmul(qftot,E->face[1].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[1].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[1].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[1].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);

  /* divide by wb[0] */
  for(i=0;i<qc;++i)
    daxpy(qa,1.0/wb[0],fi+i*qa, 1,out +i*qa*qb,1);

  /* ********************************* */
  /* add surface contribition of face 3 */
  fq = face[2].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2a);
  getim(fq,qc,&imb,g2a);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[2].h,1,E->face[2].h,1,f,1);
  dvmul(qftot,E->face[2].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[2].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[2].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[2].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);

  /* add to Ef physical mode storage */

  for(i=0;i<qc;++i)
    daxpy(qb,1./wa[qa-1],fi+i*qb,1, out+i*qa*qb+qa-1,qa);


  /* ********************************** */
  /* add surface contribition of face 4 */
  fq = face[3].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2a);
  getim(fq,qc,&imb,g2a);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[3].h,1,E->face[3].h,1,f,1);
  dvmul(qftot,E->face[3].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[3].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[3].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[3].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);

  /* add to Ef physical mode storage */

  for(i=0;i<qc;++i)
    daxpy(qa,1./wb[qb-1],fi+i*qa,1, out+qa*(qb-1)+i*qa*qb,1);


  /* ********************************** */
  /* add surface contribition of face 5 */
  fq = face[4].qface;
  qftot = fq*fq;

  getim(fq,qb,&ima,g2a);
  getim(fq,qc,&imb,g2a);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[4].h,1,E->face[4].h,1,f,1);
  dvmul(qftot,E->face[4].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[4].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[4].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[4].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);

  /* add to Ef physical mode storage */

  for(i=0;i<qc;++i)
    daxpy(qb,1./wa[0],fi+i*qb,1, out+i*qa*qb,qa);


  /* ********************************** */
  /* add surface contribition of face 6 */
  fq = face[5].qface;
  qftot = fq*fq;

  getim(fq,qa,&ima,g2a);
  getim(fq,qb,&imb,g2a);

  /* gather edge 1 and multiply by jacobian */
  dvsub(qftot,Ef->face[5].h,1,E->face[5].h,1,f,1);
  dvmul(qftot,E->face[5].jac,1,f,1,f,1);

  /* multiply by appropriate component of normal */
  switch(dir){
  case 'x':
    dvmul(qftot, E->face[5].n->x, 1, f, 1, f, 1);
    break;
  case 'y':
    dvmul(qftot, E->face[5].n->y, 1, f, 1, f, 1);
    break;
  case 'z':
    dvmul(qftot, E->face[5].n->z, 1, f, 1, f, 1);
    break;
  }

  /* interpolate to guass lobatto points */
  Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);

  /* Add to Ef physical mode storage */

  daxpy(qa*qb,1./wc[qc-1],fi,1, out+qa*qb*(qc-1),1);

  return;
}

void Element::Add_Surface_Contrib(Element *, double *, char, int, int){ERR;}
void Element::Add_Surface_Contrib(Element *, double *, char, int){ERR;}
void Element::Add_Surface_Contrib( double *, char, int){ERR;}
void Element::Add_Surface_Contrib( double *, char, int, int){ERR;}
void Element::Add_Surface_Contrib(Element *, double *, char ){ERR;}

/*
Function name: Element::fill_edges

Function Purpose:

Argument 1: double *ux
Purpose:

Argument 2: double *uy
Purpose:

Argument 3: double *
Purpose:

Function Notes:

*/

void Tri::fill_edges(double *ux, double *uy, double *){
  double **im;

  // one field case
  double * wk = Tri_wk.get();

  if(!uy){
    getim(qa,edge[0].qedg,&im,a2g);
    GetFace(ux,0,wk);
    Interp(*im,wk,qa,edge[0].h,edge[0].qedg);

    getim(qb,edge[1].qedg,&im,b2g);
    GetFace(ux,1,wk);
    Interp(*im,wk,qb,edge[1].h,edge[1].qedg);

    getim(qb,edge[2].qedg,&im,b2g);
    GetFace(ux,2,wk);
    Interp(*im,wk,qb,edge[2].h,edge[2].qedg);
  }
  else{
    double *wka = wk+QGmax;
    double *wkb = wk+2*QGmax;
    int qedg;
    Edge   *e;
    e    = edge;
    qedg = e->qedg;

    getim(qa,qedg,&im,a2g);
    GetFace(ux,0,wk);
    Interp(*im,wk,qa,wka,qedg);

    GetFace(uy,0,wk);
    Interp(*im,wk,qa,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);

    e = edge+1;
    qedg = e->qedg;
    getim(qb,qedg,&im,b2g);
    GetFace(ux,1,wk);
    Interp(*im,wk,qb,wka,qedg);

    GetFace(uy,1,wk);
    Interp(*im,wk,qb,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);

    e = edge+2;
    qedg = e->qedg;
    getim(qb,qedg,&im,b2g);
    GetFace(ux,2,wk);
    Interp(*im,wk,qb,wka,qedg);

    GetFace(uy,2,wk);
    Interp(*im,wk,qb,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);
  }
}




void Quad::fill_edges(double *ux, double *uy, double *){
  double **im;
  double *wk = Quad_wk;
  // one field case
  if(!uy){
    getim(qa,edge[0].qedg,&im,a2g);
    GetFace(ux,0,wk);
    Interp(*im,wk,qa,edge[0].h,edge[0].qedg);

    getim(qb,edge[1].qedg,&im,a2g);
    GetFace(ux,1,wk);
    Interp(*im,wk,qb,edge[1].h,edge[1].qedg);

    getim(qa,edge[2].qedg,&im,a2g);
    GetFace(ux,2,wk);
    Interp(*im,wk,qa,edge[2].h,edge[2].qedg);

    getim(qb,edge[3].qedg,&im,a2g);
    GetFace(ux,3,wk);
    Interp(*im,wk,qb,edge[3].h,edge[3].qedg);
  }
  else{
    double *wka = wk+QGmax;
    double *wkb = wk+2*QGmax;
    Edge   *e;
    int    qedg;

    e = edge;
    qedg = e->qedg;

    getim(qa,qedg,&im,a2g);
    GetFace(ux,0,wk);
    Interp(*im,wk,qa,wka,qedg);

    GetFace(uy,0,wk);
    Interp(*im,wk,qa,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);

    e = edge+1;
    qedg = e->qedg;
    getim(qb,qedg,&im,a2g);
    GetFace(ux,1,wk);
    Interp(*im,wk,qb,wka,qedg);

    GetFace(uy,1,wk);
    Interp(*im,wk,qb,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);

    e = edge+2;
    qedg = e->qedg;
    getim(qa,qedg,&im,a2g);
    GetFace(ux,2,wk);
    Interp(*im,wk,qa,wka,qedg);

    GetFace(uy,2,wk);
    Interp(*im,wk,qa,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);

    e = edge+3;
    qedg = e->qedg;
    getim(qb,qedg,&im,a2g);
    GetFace(ux,3,wk);
    Interp(*im,wk,qb,wka,qedg);

    GetFace(uy,3,wk);
    Interp(*im,wk,qb,wkb,qedg);

    dvmul (qedg, e->norm->x, 1, wka, 1, e->h, 1);
    dvvtvp(qedg, e->norm->y, 1, wkb, 1, e->h, 1, e->h, 1);
  }
}




void Tet::fill_edges(double *ux, double *uy, double *uz){
  int i;
  // one field case

  if(!uy){
    for(i=0;i<NTet_faces;++i){
      GetFace(ux,i,Tet_wk);
      InterpToGaussFace(i, Tet_wk, face[i].qface, face[i].qface, face[i].h);
    }
  }
  else{
    double *wka = Tet_wk;
    double *wkb = Tet_wk+QGmax*QGmax;

    int qface;
    Face   *f;

    for(i=0;i<NTet_faces;++i){
      f    = face+i;
      qface = f->qface;

      GetFace(ux,i,wka);
      InterpToGaussFace(i, wka, qface, qface, wkb);
      dvmul (qface*qface, f->n->x, 1, wkb, 1, f->h, 1);

      GetFace(uy,i,wka);
      InterpToGaussFace(i, wka, qface, qface, wkb);
      dvvtvp(qface*qface, f->n->y, 1, wkb, 1, f->h, 1, f->h, 1);

      GetFace(uz,i,wka);
      InterpToGaussFace(i, wka, qface, qface, wkb);
      dvvtvp(qface*qface, f->n->z, 1, wkb, 1, f->h, 1, f->h, 1);
    }
  }
}




void Pyr::fill_edges(double *ux, double *uy, double *uz){
}




void Prism::fill_edges(double *ux, double *uy, double *uz){
  double **ima, **imb;

  // one field case

  if(!uy){
    getim(qa,face[0].qface,&ima,a2g);
    getim(qb,face[0].qface,&imb,a2g);
    GetFace(ux,0,Prism_wk);
    Interp2d(*ima,*imb,Prism_wk,qa,qb,face[0].h,face[0].qface,face[0].qface);

    getim(qa,face[1].qface,&ima,a2g);
    getim(qc,face[1].qface,&imb,b2g);
    GetFace(ux,1,Prism_wk);
    Interp2d(*ima,*imb,Prism_wk,qa,qc,face[1].h,face[1].qface,face[1].qface);

    getim(qb,face[2].qface,&ima,a2g);
    getim(qc,face[2].qface,&imb,b2g);
    GetFace(ux,2,Prism_wk);
    Interp2d(*ima,*imb,Prism_wk,qb,qc,face[2].h,face[2].qface,face[2].qface);

    getim(qb,face[3].qface,&ima,a2g);
    getim(qc,face[3].qface,&imb,b2g);
    GetFace(ux,3,Prism_wk);
    Interp2d(*ima,*imb,Prism_wk,qa,qc,face[3].h,face[3].qface,face[3].qface);

    getim(qb,face[4].qface,&ima,a2g);
    getim(qc,face[4].qface,&imb,b2g);
    GetFace(ux,4,Prism_wk);
    Interp2d(*ima,*imb,Prism_wk,qb,qc,face[4].h,face[4].qface,face[4].qface);
  }
  else{
    double *wka = Prism_wk;
    double *wkb = Prism_wk+QGmax*QGmax;

    int qedg, i;
    Face   *f;

    for(i=0;i<NPrism_faces;++i){
      f    = face+i;
      qedg = f->qface;

      GetFace(ux,i,wka);
      InterpToGaussFace(i, wka, qedg, qedg, wkb);
      dvmul (qedg*qedg, f->n->x, 1, wkb, 1, f->h, 1);

      GetFace(uy,i,wka);
      InterpToGaussFace(i, wka, qedg, qedg, wkb);
      dvvtvp(qedg*qedg, f->n->y, 1, wkb, 1, f->h, 1, f->h, 1);

      GetFace(uz,i,wka);
      InterpToGaussFace(i, wka, qedg, qedg, wkb);
      dvvtvp(qedg*qedg, f->n->z, 1, wkb, 1, f->h, 1, f->h, 1);
    }
  }
}




void Hex::fill_edges(double *ux, double *uy, double *uz){

  double **ima;
  int i;
  // one field case

  if(!uy){
    for(i=0;i<NHex_faces;++i){
      getim(qa,face[i].qface,&ima,a2g);   // assume

      GetFace(ux,i,Hex_wk);
      Interp2d(*ima,*ima,Hex_wk,qa,qb,face[i].h,face[i].qface,face[i].qface);
    }
  }
  else{
    double *wka = Hex_wk;
    double *wkb = Hex_wk+QGmax*QGmax;

    int qedg;
    Face   *f;

    for(i=0;i<NHex_faces;++i){
      f    = face+i;
      qedg = f->qface;

      GetFace(ux,i,wka);
      InterpToGaussFace(i, wka, qedg, qedg, wkb);
      dvmul (qedg*qedg, f->n->x, 1, wkb, 1, f->h, 1);

      GetFace(uy,i,wka);
      InterpToGaussFace(i, wka, qedg, qedg, wkb);
      dvvtvp(qedg*qedg, f->n->y, 1, wkb, 1, f->h, 1, f->h, 1);


      GetFace(uz,i,wka);
      InterpToGaussFace(i, wka, qedg, qedg, wkb);
      dvvtvp(qedg*qedg, f->n->z, 1, wkb, 1, f->h, 1, f->h, 1);
    }
  }
}




void Element::fill_edges(double *, double *, double *){ERR;}




/*

Function name: Element::Sign_Change

Function Purpose:

Function Notes:

*/

void Tri::Sign_Change(){
  int i;

  for(i = 0; i < Nedges; ++i)
    if(edge[i].con && edge[i].l)
      dscal(edge[i].l/2, -1., edge[i].hj+1, 2);

}

void Quad::Sign_Change(){
  int i;

  for(i = 0; i < Nedges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2, -1., edge[i].hj+1, 2);

}

void Tet::Sign_Change(){
  register int i,j;
  int      L;

  for(i = 0; i < Nedges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2, -1.0, edge[i].hj+1, 2);

  for(i = 0; i < Nfaces; ++i)
    if(face[i].con)
      for(j = 1; j < (L=face[i].l); j = j + 2)
  dsmul(L-j, -1, face[i].hj[j], 1, face[i].hj[j], 1);

}




void Pyr::Sign_Change(){
  register int i,j;
  int      L;

  for(i = 0; i < NPyr_edges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2, -1.0, edge[i].hj+1, 2);

  // fix for triangle faces
  for(i = 0; i < NPyr_faces; ++i){
    if(Nfverts(i) == 4)
      switch(face[i].con ){
      case 0:
  break;
      case 1: case 5:
  for(j = 0; j < (L=face[i].l); ++j)
    dscal(L/2, -1., face[i].hj[j]+1, 2);
  break;
      case 2: case 6:
  for(j = 1; j < (L=face[i].l); j = j + 2)
    dscal(L, -1., face[i].hj[j], 1);
  break;
      case 3: case 7:
  for(j = 0; j < (L=face[i].l); ++j)
    dscal(L/2, -1., face[i].hj[j]+1, 2);

  for(j = 1; j < (L=face[i].l); j += 2)
    dscal(L, -1., face[i].hj[j], 1);
  break;
      }
    else
      if(face[i].con)
  for(j = 1; j < (L=face[i].l); j = j + 2)
    dsmul(L-j, -1, face[i].hj[j], 1, face[i].hj[j], 1);
  }
}




void Prism::Sign_Change(){
  register int i,j;
  int      L;

  for(i = 0; i < NPrism_edges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2, -1.0, edge[i].hj+1, 2);

  // fix for triangle faces
  for(i = 0; i < NPrism_faces; ++i){
    L = face[i].l;
    if(Nfverts(i) == 4)
      switch(face[i].con ){
      case 0:
  break;
      case 1: case 5:
  for(j = 0; j < L; ++j)
    dscal(L/2, -1., face[i].hj[j]+1, 2);
  break;
      case 2: case 6:
  for(j = 1; j < L; j += 2)
    dscal(L, -1., face[i].hj[j], 1);
  break;
      case 3: case 7:
  for(j = 0; j < L; ++j)
    dscal(L/2, -1., face[i].hj[j]+1, 2);

  for(j = 1; j < L; j += 2)
    dscal(L, -1., face[i].hj[j], 1);
  break;
      }
    else
      if(face[i].con)
  for(j = 1; j < L; j += 2)
    dsmul(L-j, -1, face[i].hj[j], 1, face[i].hj[j], 1);
  }
}




void Hex::Sign_Change(){
  register int i,j;
  int      L;

  for(i = 0; i < NHex_edges; ++i)
    if(edge[i].con)
      dscal(edge[i].l/2, -1.0, edge[i].hj+1, 2);
  for(i = 0; i < NHex_faces; ++i){
    switch(face[i].con ){
    case 0:
      break;
    case 1: case 5:
      for(j = 0; j < (L=face[i].l); ++j)
  dscal(L/2, -1., face[i].hj[j]+1, 2);
      break;
    case 2: case 6:
      for(j = 1; j < (L=face[i].l); j = j + 2)
  dscal(L, -1., face[i].hj[j], 1);
      break;
    case 3: case 7:
      for(j = 0; j < (L=face[i].l); ++j)
  dscal(L/2, -1., face[i].hj[j]+1, 2);

      for(j=1;j<(L=face[i].l);++j)
  dscal(L, -1., face[i].hj[j], 1);
      break;
    }
  }
}




void Element::Sign_Change(){ERR;}




static MMinfo *Tri_m1inf=NULL,*Tri_m1base=NULL;
static MMinfo *Quad_m1inf,*Quad_m1base;
static MMinfo *Tet_m1inf,*Tet_m1base;
static MMinfo *Pyr_m1inf,*Pyr_m1base;
static MMinfo *Prism_m1inf,*Prism_m1base;
static MMinfo *Hex_m1inf,*Hex_m1base;

static MMinfo   *Tri_addmmat1d(int L,   Tri *E);
static MMinfo  *Quad_addmmat1d(int L,  Quad *E);
static MMinfo   *Tet_addmmat1d(int L,   Tet *E);
static MMinfo   *Pyr_addmmat1d(int L,   Pyr *E);
static MMinfo *Prism_addmmat1d(int L, Prism *E);
static MMinfo   *Hex_addmmat1d(int L,   Hex *E);

/*

Function name: Element::get_mmat1d

Function Purpose:

Argument 1: double **mat
Purpose:

Argument 2: int L
Purpose:

Function Notes:

*/

void Tri::get_mmat1d(double **mat, int L){

  /* check link list */
  for(Tri_m1inf = Tri_m1base; Tri_m1inf; Tri_m1inf = Tri_m1inf->next)
    if(Tri_m1inf->L == L){
      *mat = Tri_m1inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Tri_m1inf  = Tri_m1base;
  Tri_m1base = Tri_addmmat1d(L,this);
  Tri_m1base->next = Tri_m1inf;

  *mat = Tri_m1base->mat;

  return;
}




void Quad::get_mmat1d(double **mat, int L){

  /* check link list */
  for(Quad_m1inf = Quad_m1base; Quad_m1inf; Quad_m1inf = Quad_m1inf->next)
    if(Quad_m1inf->L == L){
      *mat = Quad_m1inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Quad_m1inf  = Quad_m1base;
  Quad_m1base = Quad_addmmat1d(L,this);
  Quad_m1base->next = Quad_m1inf;

  *mat = Quad_m1base->mat;

  return;
}




void Tet::get_mmat1d(double **mat, int L){

  /* check link list */
  for(Tet_m1inf = Tet_m1base; Tet_m1inf; Tet_m1inf = Tet_m1inf->next)
    if(Tet_m1inf->L == L){
      *mat = Tet_m1inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Tet_m1inf  = Tet_m1base;
  Tet_m1base = Tet_addmmat1d(L,this);
  Tet_m1base->next = Tet_m1inf;

  *mat = Tet_m1base->mat;

  return;
}




void Pyr::get_mmat1d(double **mat, int L){

  /* check link list */
  for(Pyr_m1inf = Pyr_m1base; Pyr_m1inf; Pyr_m1inf = Pyr_m1inf->next)
    if(Pyr_m1inf->L == L){
      *mat = Pyr_m1inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Pyr_m1inf  = Pyr_m1base;
  Pyr_m1base = Pyr_addmmat1d(L,this);
  Pyr_m1base->next = Pyr_m1inf;

  *mat = Pyr_m1base->mat;

  return;
}




void Prism::get_mmat1d(double **mat, int L){

  /* check link list */
  for(Prism_m1inf = Prism_m1base; Prism_m1inf; Prism_m1inf = Prism_m1inf->next)
    if(Prism_m1inf->L == L){
      *mat = Prism_m1inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Prism_m1inf  = Prism_m1base;
  Prism_m1base = Prism_addmmat1d(L,this);
  Prism_m1base->next = Prism_m1inf;

  *mat = Prism_m1base->mat;

  return;
}




void Hex::get_mmat1d(double **mat, int L){

  /* check link list */
  for(Hex_m1inf = Hex_m1base; Hex_m1inf; Hex_m1inf = Hex_m1inf->next)
    if(Hex_m1inf->L == L){
      *mat = Hex_m1inf->mat;
      return;
    }

    /* else add new zeros and weights */

  Hex_m1inf  = Hex_m1base;
  Hex_m1base = Hex_addmmat1d(L,this);
  Hex_m1base->next = Hex_m1inf;

  *mat = Hex_m1base->mat;

  return;
}




void Element::get_mmat1d(double **, int){ERR;}



static MMinfo *Tri_addmmat1d(int L, Tri *E){
  register int i,j,k;
  int      info,qa=E->qa,trip=0;
  double  *tmp;
  double   *z,*w;
  MMinfo  *M = (MMinfo*)calloc(1,sizeof(MMinfo));
  Basis   *b = E->getbasis();

  M->L = L;

  if(!L) return M;

  /* If qa is less than L+3 release base definition and regenerate. */
  /* This usually won't be necessary but arises in Utilities.       */
  getzw(qa,&z,&w,'a');
  tmp =  dvector(0,qa-1);

  if(L>3){ /* banded matrix */
    M->mat = dvector(0,L*3-1);

    for(i = 0; i < L-3; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < i + 3; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    for(i = L-3; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpbtrf('L',L,2,M->mat,3,info);
  }
  else { /* symmetric */
    M->mat = dvector(0,L*(L+1)/2-1);

    for(i = 0, k=0; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j,++k)
  M->mat[k] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpptrf('L',L,M->mat,info);
  }
  if(info) error_msg(Tri addmmat1d: info not zero);

  if(trip){
    E->qa = trip;
    Tri_reset_basis();
  }

  free(tmp);
  return M;
}




// tcew -- not sure about this
static MMinfo *Quad_addmmat1d(int L, Quad *E){
  register int i,j,k;
  int      info,qa=E->qa,trip=0;
  double  *tmp,*z,*w;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));
  Basis   *b = E->getbasis();

  M->L = L;

  if(!L) return M;

  /* If qa is less than L+3 release base definition and regenerate. */
  /* This usually won't be necessary but arises in Utilities.       */
#if 0
  if(qa < L+3){
    Quad_reset_basis();
    trip = E->qa;
    E->qa = qa = L+3;
    b = E->getbasis();
  }
#endif
  getzw(qa,&z,&w,'a');
  tmp = dvector(0,qa-1);

  if(L>3){ /* banded matrix */
    M->mat =  dvector(0,L*3-1);

    for(i = 0; i < L-3; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < i + 3; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    for(i = L-3; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpbtrf('L',L,2,M->mat,3,info);
  }
  else
    { /* symmetric */
      // tcew

      M->mat = dvector(0, L*(L+1)/2-1);

      for(i = 0, k=0; i < L; ++i){
  dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
  for(j = i; j < L; ++j,++k)
    M->mat[k] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
      }
      dpptrf('L',L,M->mat,info);
    }
  if(info) error_msg(Quad addmmat1d: info not zero);

  if(trip){
    E->qa = trip;
    Quad_reset_basis();
  }

  free(tmp);
  return M;
}



static MMinfo *Tet_addmmat1d(int L, Tet *E){
  register int i,j,k;
  int      info,qa=E->qa;
  double  *tmp;
  double   *z,*w;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));
  Basis   *b = E->getbasis();

  M->L = L;

  if(!L) return M;

  /* If qa is less than L+3 release base definition and regenerate. */
  /* This usually won't be necessary but arises in Utilities.       */
#if 0
  if(qa < L+3){
    Tet_reset_basis();
    trip = E->qa;
    E->qa = qa = L+3;
    b = E->getbasis();
  }
#endif
  getzw(qa,&z,&w,'a');
  tmp =  dvector(0,qa-1);

  if(L>3){ /* banded matrix */
    M->mat = dvector(0,L*3-1);

    for(i = 0; i < L-3; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < i + 3; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    for(i = L-3; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpbtrf('L',L,2,M->mat,3,info);
  }
  else { /* symmeTetc */
    M->mat = dvector(0,L*(L+1)/2-1);

    for(i = 0, k=0; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j,++k)
  M->mat[k] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpptrf('L',L,M->mat,info);
  }
  if(info) error_msg(addmmat1d: info not zero);
#if 0
  if(trip){
    E->qa = trip;
    Tet_reset_basis();
  }
#endif
  free(tmp);
  return M;
}




static MMinfo *Pyr_addmmat1d(int L, Pyr *E){
  register int i,j,k;
  int      info,qa=E->qa;
  double  *tmp;
  double   *z,*w;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));
  Basis   *b = E->getbasis();

  M->L = L;

  if(!L) return M;

  /* If qa is less than L+3 release base definition and regenerate. */
  /* This usually won't be necessary but arises in Utilities.       */

  getzw(qa,&z,&w,'a');
  tmp =  dvector(0,qa-1);

  M->mat = dvector(0,L*(L+1)/2-1);

  for(i = 0, k=0; i < L; ++i){
    dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
    for(j = i; j < L; ++j,++k)
      M->mat[k] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
  }
  dpptrf('L',L,M->mat,info);

  if(info) error_msg(addmmat1d: info not zero);

  free(tmp);
  return M;
}




static MMinfo *Prism_addmmat1d(int L, Prism *E){
  register int i,j,k;
  int      info,qa=E->qa;
  double  *tmp;
  double   *z,*w;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));
  Basis   *b = E->getbasis();

  M->L = L;

  if(!L) return M;

  /* If qa is less than L+3 release base definition and regenerate. */
  /* This usually won't be necessary but arises in Utilities.       */

  getzw(qa,&z,&w,'a');
  tmp =  dvector(0,qa-1);

  M->mat = dvector(0,L*(L+1)/2-1);

  for(i = 0, k=0; i < L; ++i){
    dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
    for(j = i; j < L; ++j,++k)
      M->mat[k] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
  }
  dpptrf('L',L,M->mat,info);

  if(info) error_msg(addmmat1d: info not zero);

  free(tmp);
  return M;
}



static MMinfo *Hex_addmmat1d(int L, Hex *E){
  register int i,j,k;
  int      info,qa=E->qa;
  double  *tmp;
  double   *z,*w;
  MMinfo  *M = (MMinfo*)malloc(sizeof(MMinfo));
  Basis   *b = E->getbasis();

  M->L = L;

  if(!L) return M;

  /* If qa is less than L+3 release base definition and regenerate. */
  /* This usually won't be necessary but arises in Utilities.       */

  getzw(qa,&z,&w,'a');
  tmp =  dvector(0,qa-1);

  if(L>3){ /* banded matrix */
    M->mat = dvector(0,L*3-1);

    for(i = 0; i < L-3; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < i + 3; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    for(i = L-3; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j)
  M->mat[i*3+(j-i)] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpbtrf('L',L,2,M->mat,3,info);
  }
  else { /* symmeHexc */
    M->mat = dvector(0,L*(L+1)/2-1);

    for(i = 0, k=0; i < L; ++i){
      dvmul(qa-2,w+1,1,b->edge[0][i].a+1,1,tmp,1);
      for(j = i; j < L; ++j,++k)
  M->mat[k] = ddot(qa-2,tmp,1,b->edge[0][j].a+1,1);
    }
    dpptrf('L',L,M->mat,info);
  }
  if(info) error_msg(addmmat1d: info not zero);


  free(tmp);

  return M;
}


/*

Function name: Element::set_solve

Function Purpose:

Argument 1: int fac
Purpose:

Argument 2: int mask
Purpose:

Function Notes:

*/

void Tri::set_solve(int fac, int mask){
  for(int i = 0; i < 2; ++i)
    vert[vnum(fac,i)].solve &= mask;
}




void Quad::set_solve(int fac, int mask){
  for(int i = 0; i < 2; ++i)
    vert[vnum(fac,i)].solve &= mask;
}




void Tet::set_solve(int fac, int mask){
  for(int i = 0; i < Nfverts(fac); ++i)
    vert[fnum(fac,i)].solve &= mask;
}




void Pyr::set_solve(int fac, int mask){
  for(int i = 0; i < Nfverts(fac); ++i)
    vert[vnum(fac,i)].solve &= mask;
}




void Prism::set_solve(int fac, int mask){
  for(int i = 0; i < Nfverts(fac); ++i)
    vert[vnum(fac,i)].solve &= mask;
}




void Hex::set_solve(int fac, int mask){
  for(int i = 0; i < Nfverts(fac); ++i)
    vert[vnum(fac,i)].solve &= mask;
}




void Element::set_solve(int , int ){ERR;}


// needs to be process into Element_list structure
/*
   This function interploltes the value in in->[Bc->id].link at the
   quadrature points multiplied by beta and the subtracts this value
   from the current field U multiplied by alpha and put the value into
   field "out"

   out = alpha * U - beta U[Bc->eid].link */

void Element::SubtractBC(double alpha, double  beta, int Bface,  Element *out)
{ERR;}

void Tri::SubtractBC(double alpha, double  beta, int Bface,  Element *out){
  int     qedg;
  double *f, *fi, *u, *uf,**im;
  double **ima, **imb;
  Edge   *e;

  fi = Tri_wk.get();

  /* set up boundary conditions  */
  e    = edge[Bface].link;
  qedg = e->qedg;
  f    = e->h;
  u    = this->h[0];
  uf   = out->h[0];

  switch(Bface){
  case 0:
    getim (qedg,qa,&im,g2a);
    Interp(*im,f,qedg,fi,qa);
    dscal  (qa,beta,fi,1);
    dsvtvm (qa,alpha,u,1,fi,1,uf,1);
    break;
  case 1:
    getim  (qedg,qb,&im,g2b);
    Interp (*im,f,qedg,fi,qb);
    dscal  (qb,beta,fi,1);
    dsvtvm (qb,alpha,u+qa-1,qa,fi,1,uf+qa-1,qa);
    break;
  case 2:
    getim  (qedg,qb,&im,g2b);
    Interp (*im,f,qedg,fi,qb);
    dscal  (qb,beta,fi,1);
    dsvtvm (qb,alpha,u,qa,fi,1,uf,qa);

  }
}

void Quad::SubtractBC(double alpha, double  beta, int Bface,  Element *out){
  int     qedg;
  double *f, *fi, *u, *uf,**im;
  double **ima, **imb;
  Edge   *e;

  fi = Quad_wk;

  /* set up boundary conditions  */
  e    = edge[Bface].link;
  qedg = e->qedg;
  f    = e->h;
  u    = this->h[0];
  uf   = out->h[0];

  switch(Bface){
  case 0:
    getim (qedg,qa,&im,g2a);
    Interp(*im,f,qedg,fi,qa);
    dscal  (qa,beta,fi,1);
    dsvtvm (qa,alpha,u,1,fi,1,uf,1);
    break;
  case 1:
    getim  (qedg,qb,&im,g2a);
    Interp (*im,f,qedg,fi,qb);
    dscal  (qb,beta,fi,1);
    dsvtvm (qb,alpha,u+qa-1,qa,fi,1,uf+qa-1,qa);
    break;
  case 2:
    getim  (qedg,qa,&im,g2a);
    Interp (*im,f,qedg,fi,qa);
    dscal  (qa,beta,fi,1);
    dsvtvm (qa,alpha,u+qa*(qb-1),1,fi,1,uf+qa*(qb-1),1);
    break;
  case 3:
    getim  (qedg,qb,&im,g2a);
    Interp (*im,f,qedg,fi,qb);
    dscal  (qb,beta,fi,1);
    dsvtvm (qb,alpha,u,qa,fi,1,uf,qa);
    break;
  }
}

void Tet::SubtractBC(double alpha, double  beta, int Bface,  Element *out){
  int     qedg,fq,qftot,i;
  double *f, *fi, *u, *uf,**im;
  double **ima, **imb;

  fi = Tet_wk;

  /* set up boundary conditions  */
  fq   = face[Bface].link->qface;
  qftot = fq*fq;
  f    = face[Bface].link->h;
  u    = this->h_3d[0][0];
  uf   = out->h_3d[0][0];

  switch(Bface){
  case 0:
    getim   (fq,qa,&ima,g2a);
    getim   (fq,qb,&imb,g2b);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);
    dscal   (qa*qb,beta,fi,1);
    dsvtvm  (qa*qb,alpha,u,1,fi,1,uf,1);
    break;
  case 1:
    getim   (fq,qa,&ima,g2a);
    getim   (fq,qc,&imb,g2c);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);
    dscal   (qa*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm  (qa,alpha,u+i*qa*qb,1,fi+i*qa,1,uf+i*qa*qb,1);
    break;
  case 2:
    getim(fq,qb,&ima,g2b);
    getim(fq,qc,&imb,g2c);
    Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);
    dscal  (qb*qc,beta,fi,1);
    for(i=0;i<qc;++i)
  dsvtvm (qb,alpha,u+i*qa*qb+qa-1,qa,fi+i*qb,1,uf+qa*qb+qa-1,qa);
    break;
  case 3:
    getim(fq,qb,&ima,g2b);
    getim(fq,qc,&imb,g2c);
    Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);
    dscal  (qb*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qb,alpha,u+i*qa*qb,qa,fi+i*qb,1,uf+i*qa*qb,qa);
    break;
  }
}

void Pyr::SubtractBC(double alpha, double  beta, int Bface,  Element *out){
  fprintf(stderr,"SubtractBC no implemented in Boundary.C\n");
  return;
}

void Prism::SubtractBC(double alpha, double  beta, int Bface,  Element *out){
  int     qedg,fq,qftot,i;
  double  *f, *fi, *u, *uf,**im;
  double  **ima, **imb;

  fi = Prism_wk;

  /* set up boundary conditions  */
  fq   = face[Bface].link->qface;
  qftot = fq*fq;
  f    = face[Bface].link->h;
  u    = this->h_3d[0][0];
  uf   = out->h_3d[0][0];

  switch(Bface){
  case 0:
    getim(fq,qa,&ima,g2a);
    getim(fq,qb,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);
    dscal  (qa*qb,beta,fi,1);
    dsvtvm (qa*qb,alpha,u,1,fi,1,uf,1);
    break;
  case 1:
    getim(fq,qa,&ima,g2a);
    getim(fq,qc,&imb,g2b);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);
    dscal  (qa*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qa,alpha,u+i*qa*qb,1,fi+i*qa,1,uf+i*qa*qb,1);
    break;
  case 2:
    getim(fq,qb,&ima,g2a);
    getim(fq,qc,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);
    dscal  (qb*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qb,alpha,u+i*qa*qb+qa-1,qa,fi+i*qb,1,uf+qa*qb+qa-1,qa);
    break;
  case 3:
    getim(fq,qa,&ima,g2a);
    getim(fq,qc,&imb,g2b);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);
    dscal  (qa*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qa,alpha,u+qa*(qb-1)+i*qa*qb,1,fi+i*qa,1,
        uf+qa*(qb-1)+i*qa*qb,1);
    break;
  case 4:
    getim(fq,qb,&ima,g2a);
    getim(fq,qc,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);
    dscal  (qb*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qb,alpha,u+i*qa*qb,qa,fi+i*qb,1,uf+i*qa*qb,qa);
  }
}

void Hex::SubtractBC(double alpha, double  beta, int Bface,  Element *out){
  int     qedg,fq,qftot,i;
  double *f, *fi, *u, *uf,**im;
  double **ima, **imb;

  fi = Hex_wk;

  /* set up boundary conditions  */
  fq = face[Bface].link->qface;
  qftot = fq*fq;
  f    = face[Bface].link->h;
  u    = this->h_3d[0][0];
  uf   = out->h_3d[0][0];

  switch(Bface){
  case 0:
    getim(fq,qa,&ima,g2a);
    getim(fq,qb,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);
    dscal  (qa*qb,beta,fi,1);
    dsvtvm (qa*qb,alpha,u,1,fi,1,uf,1);
    break;
  case 1:
    getim(fq,qa,&ima,g2a);
    getim(fq,qc,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);
    dscal  (qa*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qa,alpha,u+i*qa*qb,1,fi+i*qa,1,uf+i*qa*qb,1);
    break;
  case 2:
    getim(fq,qb,&ima,g2a);
    getim(fq,qc,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);
    dscal  (qb*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qb,alpha,u+i*qa*qb+qa-1,qa,fi+i*qb,1,uf+qa*qb+qa-1,qa);
    break;
  case 3:
    getim(fq,qa,&ima,g2a);
    getim(fq,qc,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qc);
    dscal  (qa*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qa,alpha,u+qa*(qb-1)+i*qa*qb,1,fi+i*qa,1,
        uf+qa*(qb-1)+i*qa*qb,1);
    break;
  case 4:
    getim(fq,qb,&ima,g2a);
    getim(fq,qc,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qb,qc);
    dscal  (qb*qc,beta,fi,1);
    for(i=0;i<qc;++i)
      dsvtvm (qb,alpha,u+i*qa*qb,qa,fi+i*qb,1,uf+i*qa*qb,qa);
  case 5:
    getim(fq,qa,&ima,g2a);
    getim(fq,qb,&imb,g2a);
    Interp2d(*ima,*imb, f,fq,fq,fi,qa,qb);
    dscal  (qa*qb,beta,fi,1);
    dsvtvm (qa*qb,alpha,u+i*qa*qb*(qc-1),1,fi,1,uf+i*qa*qb*(qc-1),1);
  }
}
