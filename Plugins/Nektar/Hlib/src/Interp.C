

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

/*

Function name: Element::InterpToFace1

Function Purpose:

Argument 1: int from_face
Purpose:

Argument 2: double *f
Purpose:

Argument 3: double *fi
Purpose:

Function Notes:

*/

void Tri::InterpToFace1(int from_face, double *f, double *fi){

  /* interpolate to side quadrature points */
  if((from_face == 1)||(from_face == 2)){
    double **im;
    getim(qb,qa,&im,b2a);

    if(qb == qa-1){ /* just need to do end point */
      dcopy(qa-1,f,1,fi,1);
      fi[qa-1] = ddot(qb,im[qa-1],1,fi,1);
    }
    else /* do all interior points */
      dgemv('T',qb,qa,1.0,*im,qb,f,1,0.0,fi,1);
  }
  else
    dcopy(qa,f,1,fi,1);
}




void Quad::InterpToFace1(int from_face, double *f, double *fi){

  /* interpolate to side quadrature points */

  if((from_face == 1)||(from_face == 3)){
    double **im;
    getim(qb,qa,&im,a2a);

    dgemv('T',qb,qa,1.0,*im,qb,f,1,0.0,fi,1);
  }
  else
    dcopy(qa,f,1,fi,1);
}




void Tet::InterpToFace1(int from_face, double *f, double *fi){
  register int i;

  /* interpolate onto face 1 co-ordinate system */
  switch(from_face){
  case 0:
    dcopy(qa*qb,f,1,fi,1);
    break;
  case 1:
    {
      double **im;
      /* interpolate qc to qb */
      getim(qc,qb,&im,c2b);
      for(i = 0; i < qa; ++i)
  dgemv('T',qc,qb,1.0,*im,qc,f+i,qa,0.0,fi+i,qa);
    }
    break;
  case 2: case 3:
    {
      double **ima, **imb;
      getim(qb,qa,&ima,b2a);
      getim(qc,qb,&imb,c2b);
      Interp2d(*ima,*imb,f,qb,qc,fi,qa,qb);
    }
    break;
  }
}

void Pyr::InterpToFace1(int from_face, double *f, double *fi){

  switch(from_face){
  case 0:
    dcopy(qa*qb,f,1,fi,1);
    break;
  case 1: case 3:
    dcopy(qa*qc,f,1,fi,1);
    break;
  case 2: case 4:
    dcopy(qb*qc,f,1,fi,1);
    if(qa != qb)
      fprintf(stderr,"Warning:Pyr:InterpToFace1 - actually "
        "interpolating to qb x qc rather than qa x qc");
    break;
  }
}

void Prism::InterpToFace1(int from_face, double *f, double *fi){
  register int i;

  switch(from_face){
  case 0:
    dcopy(qa*qb,f,1,fi,1);
    break;
  case 1: case 3:
    dcopy(qa*qc,f,1,fi,1);
    break;
  case 2: case 4:
    double **im;
      getim(qc,qb,&im,b2a);
      for(i=0;i<qb;++i)
  dgemv('T',qc,qb,1.0,*im,qc,f+i,qb,0.0,fi+i,qb);

    if(qa != qb)
      fprintf(stderr,"Warning:Prism:InterpToFace1 - actually "
        "interpolating to qb x qb rather than qa x qb");
    break;
  }
}




void Hex::InterpToFace1(int from_face, double *f, double *fi){
  dcopy(qa*qb,f,1,fi,1);

  if((qa != qb)||(qb != qc))
     fprintf(stderr,"Warning:Hex:InterpToFace1 - not actually "
       "to  qa x qc");
}


void Element::InterpToFace1(int , double *, double *){ERR;}




/*

Function name: Element::InterpToGaussFace

Function Purpose:

Argument 1: int
Purpose:

Argument 2: double *
Purpose:

Argument 3: int
Purpose:

Argument 4: int
Purpose:

Argument 5: double *
Purpose:

Function Notes:

*/

void Tri::InterpToGaussFace(int , double *, int , int , double *){
  // Nothing here
}




void Quad::InterpToGaussFace(int , double *, int , int , double *){
  // Nothing here
}




void Tet::InterpToGaussFace(int from_face, double *from, int qaf, int qbf, double *to){

  double **ima, **imb;
  int q1, q2;
  /* interpolate onto face with Gauss quadrature points */
  switch(from_face){
  case 0:
    q1 = qa; q2 = qb;
    getim(qa,qaf,&ima,a2g);
    getim(qb,qbf,&imb,b2g);

    break;
  case 1:
    q1 = qa; q2 = qc;
    getim(qa,qaf,&ima,a2g);
    getim(qc,qbf,&imb,c2g);

    break;
  case 2: case 3:
    q1 = qb; q2 = qc;
    getim(qb,qaf,&ima,b2g);
    getim(qc,qbf,&imb,c2g);

    break;
  }

  Interp2d(*ima, *imb, from, q1, q2, to, qaf, qbf);

  return;
}




void Pyr::InterpToGaussFace(int from_face, double *from, int qaf, int qbf, double *to){
  double *wk = Pyr_InterpToGaussFace_wk;

  double **im;
  int i;
  /* interpolate onto face with Gauss quadrature points */
  switch(from_face){
  case 0:
    /* interpolate qa to qaf */
    getim(qa,qaf,&im,a2g);
    for(i = 0; i < qb; ++i)
      dgemv('T',qa,qaf,1.0,*im,qa,from+i*qa,1,0.0,wk+i*qaf,1);
    /* interpolate qb to qbf */
    getim(qb,qbf,&im,a2g);
    for(i = 0; i < qaf; ++i)
      dgemv('T',qb,qaf,1.0,*im,qb,wk+i,qaf,0.0,to+i,qaf);
    break;
  case 1: case 3:
    /* interpolate qa to qaf */
    getim(qa,qaf,&im,a2g);
    for(i = 0; i < qc; ++i)
      dgemv('T',qa,qaf,1.0,*im,qa,from+i*qa,1,0.0,wk+i*qaf,1);
    /* interpolate qc to qbf */
    getim(qc,qbf,&im,c2h);
    for(i = 0; i < qaf; ++i)
      dgemv('T',qc,qbf,1.0,*im,qc,wk+i,qaf,0.0,to+i,qaf);
    break;
  case 2: case 4:
    /* interpolate qb to qaf */
    getim(qb,qaf,&im,a2g);
    for(i = 0; i < qc; ++i)
      dgemv('T',qb,qaf,1.0,*im,qb,from+i*qb,1,0.0,wk+i*qaf,1);
    /* interpolate qc to qbf */
    getim(qc,qbf,&im,c2h);
    for(i = 0; i < qaf; ++i)
      dgemv('T',qc,qbf,1.0,*im,qc,wk+i,qaf,0.0,to+i,qaf);
    break;
  }
  return;
}




void Prism::InterpToGaussFace(int from_face, double *from, int qaf, int qbf, double *to){
  double **ima, **imb;
  int  q1, q2;
  /* interpolate onto face with Gauss quadrature points */
  switch(from_face){
  case 0:
    q1 = qa; q2 = qb;
    getim(qa,qaf,&ima,a2g);
    getim(qb,qbf,&imb,a2g);

    break;
  case 1: case 3:
    q1 = qa; q2 = qc;
    getim(qa,qaf,&ima,a2g);
    getim(qc,qbf,&imb,b2g);

    break;
  case 2: case 4:
    q1 = qb; q2 = qc;
    getim(qb,qaf,&ima,a2g);
    getim(qc,qbf,&imb,b2g);

    break;
  }

  Interp2d(*ima, *imb, from, q1, q2, to, qaf, qbf);

  return;
}




void Hex::InterpToGaussFace(int from_face, double *from, int qag, int qbg, double *to){

  double **ima, **imb;
  int q1, q2;

  switch(from_face){
  case 0: case 5:
    q1 = qa; q2 = qb; break;
  case 1: case 3:
    q1 = qa; q2 = qc; break;
  case 2: case 4:
    q1 = qb; q2 = qc; break;
  default:
    break;
  }

  /* interpolate q1 to qaf */
  getim(q1,qag,&ima,a2g);
  getim(q2,qbg,&imb,a2g);
  Interp2d(*ima,*imb,from,q1,q2,to,qag,qbg);
}




void Element::InterpToGaussFace(int,double*,int,int,double*){ERR;}
