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

Function name: Element::GetFace

Function Purpose:

Argument 1: int fac
Purpose:

Argument 2: Coord *X
Purpose:

Function Notes:

*/


void Tri::GetFace(double *from, int fac, double *to){
  switch(fac){
  case 0:
    dcopy(qa, from, 1, to, 1);
    break;
  case 1:
    dcopy(qb, from + qa-1, qa, to, 1);
    break;
  case 2:
    dcopy(qb, from       , qa, to, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}


void Quad::GetFace(double *from, int fac, double *to){
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

void Tet::GetFace(double *from, int fac, double *to){
  register int i;

  switch(fac){
  case 0:
    dcopy(qa*qb,from ,1 ,to ,1);
    break;
  case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb, 1, to + i*qa, 1);
    break;
  case 2:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + qa-1 + i*qa*qb, qa, to + i*qb, 1);
    break;
  case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + i*qa*qb, qa, to + i*qb, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}


// needs to be fixed
void Pyr::GetFace(double *from, int fac, double *to){
  register int i;

  switch(fac){
  case 0:
    dcopy(qa*qb,from ,1 ,to ,1);
    break;
  case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb, 1, to + i*qa, 1);
    break;
  case 2:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + qa-1 + i*qa*qb, qa, to + i*qb, 1);
    break;
  case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb+qa*(qb-1), 1, to + i*qa, 1);
    break;
  case 4:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + i*qa*qb, qa, to + i*qb, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}


// needs to be fixed
void Prism::GetFace(double *from, int fac, double *to){
  register int i;

  switch(fac){
  case 0:
    dcopy(qa*qb,from ,1 ,to ,1);
    break;
  case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb, 1, to + i*qa, 1);
    break;
  case 2:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + qa-1 + i*qa*qb, qa, to + i*qb, 1);
    break;
  case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb+qa*(qb-1), 1, to + i*qa, 1);
    break;
  case 4:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + i*qa*qb, qa, to + i*qb, 1);
    break;
  default:
    error_msg(GetFace -- unknown face);
    break;
  }
}


// needs to be fixed
void Hex::GetFace(double *from, int fac, double *to){
  register int i;

  switch(fac){
  case 0:
    dcopy(qa*qb,from ,1 ,to ,1);
    break;
  case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb, 1, to + i*qa, 1);
    break;
  case 2:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + qa-1 + i*qa*qb, qa, to + i*qb, 1);
    break;
  case 3:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa*qb+qa*(qb-1), 1, to + i*qa, 1);
    break;
  case 4:
    for(i = 0; i < qc; ++i)
      dcopy(qb, from + i*qa*qb, qa, to + i*qb, 1);
    break;
  case 5:
    dcopy(qa*qb,from +qa*qb*(qc-1),1 ,to ,1);
    break;
  default:
    fprintf(stderr,"Hex: %d ", fac);
    error_msg(GetFace -- unknown face);
    break;
  }
}

void Element::GetFace(double *, int, double*){ERR;}
/*

Function name: Element::PutFace

Function Purpose:

Argument 1: double *from
Purpose:

Argument 2: int fac
Purpose:

Function Notes:

*/

void Tri::PutFace(double *from, int fac){
  if(from)
    switch(fac){
    case 0:
      dcopy(qa, from, 1, h[0], 1);
      break;
    case 1:
      dcopy(qb, from, 1, h[0]+qa-1, qa);
      break;
    case 2:
      dcopy(qb, from, 1, h[0], qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  else
    switch(fac){
    case 0:
      dzero(qa, h[0], 1);
      break;
    case 1:
      dzero(qb, h[0]+qa-1, qa);
      break;
    case 2:
      dzero(qb, h[0], qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
}




void Quad::PutFace(double *from, int fac){

  if(from)
    switch(fac){
    case 0:
      dcopy(qa, from, 1, h[0], 1);
      break;
    case 1:
      dcopy(qb, from, 1, h[0]+qa-1, qa);
      break;
    case 2:
      dcopy(qa, from, 1, h[0]+qa*(qb-1), 1);
      break;
    case 3:
      dcopy(qb, from, 1, h[0], qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  else
    switch(fac){
    case 0:
      dzero(qa, h[0], 1);
      break;
    case 1:
      dzero(qb, h[0]+qa-1, qa);
      break;
    case 2:
      dzero(qa, h[0]+qa*(qb-1), 1);
      break;
    case 3:
      dzero(qb, h[0], qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
}




void Tet::PutFace(double *from , int fac){
  int i;
  if(from){
    switch(fac){
    case 0:
      dcopy(qa*qb, from ,1 ,**h_3d ,1);
      break;
    case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa, 1, **h_3d + i*qa*qb, 1);
    break;
    case 2:
      for(i = 0; i < qc; ++i)
  dcopy(qb, from+i*qb, 1, **h_3d + qa-1 + i*qa*qb, qa);
      break;
    case 3:
      for(i = 0; i < qc; ++i)
  dcopy(qb, from + i*qb, 1, **h_3d + i*qa*qb, qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  }
  else{
    switch(fac){
    case 0:
      dzero(qa*qb, **h_3d ,1);
      break;
    case 1:
      for(i = 0; i < qc; ++i)
  dzero(qa, **h_3d + i*qa*qb, 1);
      break;
    case 2:
      for(i = 0; i < qc; ++i)
  dzero(qb, **h_3d + qa-1 + i*qa*qb, qa);
      break;
    case 3:
      for(i = 0; i < qc; ++i)
  dzero(qb, **h_3d + i*qa*qb, qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  }
}




void Pyr::PutFace(double *, int ){
}




void Prism::PutFace(double *from, int fac ){

  int i;
  if(from){
    switch(fac){
    case 0:
      dcopy(qa*qb, from ,1 ,**h_3d ,1);
      break;
    case 1:
    for(i = 0; i < qc; ++i)
      dcopy(qa, from + i*qa, 1, **h_3d + i*qa*qb, 1);
    break;
    case 2:
      for(i = 0; i < qc; ++i)
  dcopy(qb, from+i*qb, 1, **h_3d + qa-1 + i*qa*qb, qa);
      break;
    case 3:
      for(i = 0; i < qc; ++i)
  dcopy(qa, from+i*qa, 1, **h_3d + qa*(qb-1) + i*qa*qb, 1);
      break;

    case 4:
      for(i = 0; i < qc; ++i)
  dcopy(qb, from + i*qb, 1, **h_3d + i*qa*qb, qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  }
  else{
    switch(fac){
    case 0:
      dzero(qa*qb, **h_3d ,1);
      break;
    case 1:
      for(i = 0; i < qc; ++i)
  dzero(qa, **h_3d + i*qa*qb, 1);
      break;
    case 2:
      for(i = 0; i < qc; ++i)
  dzero(qb, **h_3d + qa-1 + i*qa*qb, qa);
      break;
    case 3:
      for(i = 0; i < qc; ++i)
  dzero(qa, **h_3d + qa*(qb-1) + i*qa*qb, 1);
      break;
    case 4:
      for(i = 0; i < qc; ++i)
  dzero(qb, **h_3d + i*qa*qb, qa);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  }
}




void Hex::PutFace(double *from, int fac){

  int i;
  if(from){
    switch(fac){
    case 0:
      dcopy(qa*qb, from ,1 ,**h_3d ,1);
      break;
    case 1:
      for(i = 0; i < qc; ++i)
  dcopy(qa, from + i*qa, 1, **h_3d + i*qa*qb, 1);
      break;
    case 2:
      for(i = 0; i < qc; ++i)
  dcopy(qb, from+i*qb, 1, **h_3d + qa-1 + i*qa*qb, qa);
      break;
    case 3:
      for(i = 0; i < qc; ++i)
  dcopy(qa, from+i*qa, 1, **h_3d + qa*(qb-1) + i*qa*qb, 1);
      break;

    case 4:
      for(i = 0; i < qc; ++i)
  dcopy(qb, from + i*qb, 1, **h_3d + i*qa*qb, qa);
      break;
    case 5:
      dcopy(qa*qb, from ,1 ,**h_3d + (qc-1)*qa*qb ,1);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  }
  else{
    switch(fac){
    case 0:
      dzero(qa*qb, **h_3d ,1);
      break;
    case 1:
      for(i = 0; i < qc; ++i)
  dzero(qa, **h_3d + i*qa*qb, 1);
      break;
    case 2:
      for(i = 0; i < qc; ++i)
  dzero(qb, **h_3d + qa-1 + i*qa*qb, qa);
      break;
    case 3:
      for(i = 0; i < qc; ++i)
  dzero(qa, **h_3d + qa*(qb-1) + i*qa*qb, 1);
      break;
    case 4:
      for(i = 0; i < qc; ++i)
  dzero(qb, **h_3d + i*qa*qb, qa);
      break;
    case 5:
      dzero(qa*qb, **h_3d + (qc-1)*qa*qb ,1);
      break;
    default:
      error_msg(GetFace -- unknown face);
      break;
    }
  }
}




void Element::PutFace(double*, int){ERR;}




/*

Function name: Element::Set_field

Function Purpose:

Argument 1: char *string
Purpose:

Function Notes:

*/

void Tri::Set_field(char *string){
  register int i;
  int   qt;
  Coord X;
  Vert  *v;

  /* find max quadrature points*/
  qt = QGmax*QGmax;
  vector_def("x y",string);

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);

  qt = qa*qb;
  v  = vert;
  this->coord(&X);

  for(i = 0;i < Nverts;++i)
    vector_set(1,&v[i].x,&v[i].y,v[i].hj);
  vector_set(qt,X.x,X.y,h[0]);
  state = 'p';

  free(X.x); free(X.y);
}




void Quad::Set_field(char *string){
  register int i;
  int   qt;
  double *s;
  Coord X;
  Vert  *v;

  /* find max quadrature points*/
  qt = QGmax*QGmax;
  vector_def("x y",string);

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);

  qt = qa*qb;
  s  = h[0];
  v  = vert;
  this->coord(&X);

  for(i = 0;i < Nverts;++i)
    vector_set(1,&v[i].x,&v[i].y,v[i].hj);
  vector_set(qt,X.x,X.y,s);
  state = 'p';

  free(X.x); free(X.y);
}


void Tet::Set_field(char *string){
  register int i;
  int   qt;
  Coord X;
  Vert  *v;

  /* find max quadrature points*/
  qt = QGmax*QGmax*QGmax;
  vector_def("x y z",string);

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  qt = qa*qb*qc;
  v  = vert;
  this->coord(&X);

  for(i = 0;i < Nverts;++i)
    vector_set(1,&v[i].x,&v[i].y,&v[i].z,v[i].hj);
  vector_set(qt,X.x,X.y,X.z, h_3d[0][0]);
  state = 'p';

  free(X.x); free(X.y); free(X.z);
}




void Pyr::Set_field(char *string){
  register int i;
  int   qt;
  Coord X;
  Vert  *v;

  /* find max quadrature points*/
  qt = QGmax*QGmax*QGmax;
  vector_def("x y z",string);

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  qt = qa*qb*qc;
  v  = vert;
  this->coord(&X);

  for(i = 0;i < NPyr_verts;++i)
    vector_set(1,&v[i].x,&v[i].y,&v[i].z,v[i].hj);
  vector_set(qt,X.x,X.y,X.z, h_3d[0][0]);
  state = 'p';

  free(X.x); free(X.y); free(X.z);
}




void Prism::Set_field(char *string){
  register int i;
  int   qt;
  Coord X;
  Vert  *v;

  /* find max quadrature points*/
  qt = QGmax*QGmax*QGmax;
  vector_def("x y z",string);

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  qt = qa*qb*qc;
  v  = vert;
  this->coord(&X);

  for(i = 0;i < NPrism_verts;++i)
    vector_set(1,&v[i].x,&v[i].y,&v[i].z,v[i].hj);
  vector_set(qt,X.x,X.y,X.z, h_3d[0][0]);
  state = 'p';

  free(X.x); free(X.y); free(X.z);
}




void Hex::Set_field(char *string){
  register int i;
  int   qt;
  Coord X;
  Vert  *v;

  /* find max quadrature points*/
  qt = QGmax*QGmax*QGmax;
  vector_def("x y z",string);

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  qt = qa*qb*qc;
  v  = vert;
  this->coord(&X);

  for(i = 0;i < NHex_verts;++i)
    vector_set(1,&v[i].x,&v[i].y,&v[i].z,v[i].hj);
  vector_set(qt,X.x,X.y,X.z, h_3d[0][0]);
  state = 'p';

  free(X.x); free(X.y); free(X.z);
}




void Element::Set_field(char *){ERR;}         // set field to function




/*

Function name: Element::dump_mesh

Function Purpose:

Argument 1: FILE *name
Purpose:

Function Notes:

*/

void Tri::dump_mesh(FILE *name){
  Coord X;
  X.x = dvector(0,QGmax*QGmax-1);
  X.y = dvector(0,QGmax*QGmax-1);

  this->coord(&X);

  fprintf(name,"VARIABLES = x y z\n");
  fprintf(name,"ZONE T=\"Element %d\", I=%d, J=%d, F=POINT\n",
    id,qa,qb);

  for(int i=0;i<qa*qb;++i)
    fprintf(name,"%lf %lf %lf\n", X.x[i], X.y[i], h[0][i]);
  free(X.x);
  free(X.y);
}




void Quad::dump_mesh(FILE *name){
  Coord X;
  X.x = dvector(0,QGmax*QGmax-1);
  X.y = dvector(0,QGmax*QGmax-1);

  this->coord(&X);

  fprintf(name,"VARIABLES = x y z\n");
  fprintf(name,"ZONE T=\"Element %d\", I=%d, J=%d, F=POINT\n",
    id,qa,qb);

  for(int i=0;i<qa;++i)
    for(int j=0;j<qb;++j)
      fprintf(name,"%lf %lf %lf \n", X.x[i+j*qa], X.y[i+j*qa], h[j][i]);
  free(X.x);
  free(X.y);

}




void Tet::dump_mesh(FILE *name){
  Coord X;
  X.x = dvector(0,QGmax*QGmax*QGmax-1);
  X.y = dvector(0,QGmax*QGmax*QGmax-1);
  X.z = dvector(0,QGmax*QGmax*QGmax-1);

  coord(&X);

  if(!option("NOHEADER"))
    fprintf(name,"VARIABLES = x y z u\n");
  fprintf(name,"ZONE T=\"Element %d\", I=%d, J=%d, K=%d,F=POINT\n",
    0,qa,qb,qc);

  for(int i=0;i<qtot;++i)
    fprintf(name,"%lf %lf %lf %lf\n", X.x[i], X.y[i], X.z[i], h_3d[0][0][i]);
  free(X.x);
  free(X.y);
  free(X.z);
}

void Pyr::dump_mesh(FILE *name){
  Coord X;
  X.x = dvector(0,QGmax*QGmax*QGmax-1);
  X.y = dvector(0,QGmax*QGmax*QGmax-1);
  X.z = dvector(0,QGmax*QGmax*QGmax-1);

  this->coord(&X);

  if(!option("NOHEADER"))
    fprintf(name,"VARIABLES = x y z u\n");
  fprintf(name,"ZONE T=\"Element %d\", I=%d, J=%d, K=%d,F=POINT\n",
    0,qa,qb,qc);

  for(int i=0;i<qtot;++i)
    fprintf(name,"%lf %lf %lf %lf\n", X.x[i], X.y[i], X.z[i], h_3d[0][0][i]);
  free(X.x);
  free(X.y);
  free(X.z);
}



void Prism::dump_mesh(FILE *name){
  Coord X;
  X.x = dvector(0,QGmax*QGmax*QGmax-1);
  X.y = dvector(0,QGmax*QGmax*QGmax-1);
  X.z = dvector(0,QGmax*QGmax*QGmax-1);

  this->coord(&X);

  if(!option("NOHEADER"))
    fprintf(name,"VARIABLES = x y z u\n");
  fprintf(name,"ZONE T=\"Element %d\", I=%d, J=%d, K=%d,F=POINT\n",
    0,qa,qb,qc);

  for(int i=0;i<qtot;++i)
    fprintf(name,"%lf %lf %lf %lf\n", X.x[i], X.y[i], X.z[i], h_3d[0][0][i]);
  free(X.x);
  free(X.y);
  free(X.z);
}



void Hex::dump_mesh(FILE *name){
  Coord X;
  X.x = dvector(0,QGmax*QGmax*QGmax-1);
  X.y = dvector(0,QGmax*QGmax*QGmax-1);
  X.z = dvector(0,QGmax*QGmax*QGmax-1);

  this->coord(&X);

  if(!option("NOHEADER"))
    fprintf(name,"VARIABLES = x y z u\n");
  fprintf(name,"ZONE T=\"Element %d\", I=%d, J=%d, K=%d,F=POINT\n",
    0,qa,qb,qc);

  for(int i=0;i<qtot;++i)
    fprintf(name,"%lf %lf %lf %lf\n", X.x[i], X.y[i], X.z[i], h_3d[0][0][i]);
  free(X.x);
  free(X.y);
  free(X.z);
}



void Element::dump_mesh(FILE *){ERR;}




/*

Function name: Element::fill_column

Function Purpose:

Argument 1: double **Mat
Purpose:

Argument 2: int loc
Purpose:

Argument 3: Bsystem *B
Purpose:

Argument 4: int nm
Purpose:

Argument 5: int offset
Purpose:

Function Notes:

*/

void Tri::fill_column(double **Mat, int loc, Bsystem *B, int nm, int offset){
  int       N,ll,tid,gid;
  const int nvs = B->nv_solve, nes = B->ne_solve;
  int      *e = B->edge;
  register int j;

  N = Nfmodes();

  for(j = 0; j < Nverts; ++j)
    if(vert[j].gid < nvs)
      Mat[vert[j].gid][loc] += vert[j].hj[0];

  for(j = 0; j < Nedges; ++j)
    if((gid = edge[j].gid) < nes){
      ll = edge[j].l;
      dvadd(ll,edge[j].hj,1,Mat[e[gid]]+loc,nm,Mat[e[gid]]+loc,nm);
    }

  tid = B->nsolve + offset;
  if(N) dvadd(N,*face->hj,1,Mat[tid]+loc,nm,Mat[tid]+loc,nm);
}




void Quad::fill_column(double **Mat, int loc, Bsystem *B, int nm, int offset){
  int       N,ll,tid,gid;
  const int nvs = B->nv_solve, nes = B->ne_solve;
  int      *e = B->edge;
  register int j;

  N = face->l;
  N = N*N;

  for(j = 0; j < Nverts; ++j)
    if(vert[j].gid < nvs)
      Mat[vert[j].gid][loc] += vert[j].hj[0];

  for(j = 0; j < Nedges; ++j)
    if((gid = edge[j].gid) < nes){
      ll = edge[j].l;
      dvadd(ll,edge[j].hj,1,Mat[e[gid]]+loc,nm,Mat[e[gid]]+loc,nm);
    }

  tid = B->nsolve + offset;
  if(N) dvadd(N,*face->hj,1,Mat[tid]+loc,nm,Mat[tid]+loc,nm);
}




void Tet::fill_column(double **Mat, int loc, Bsystem *B, int nm, int offset){
  int       N,ll,tid,gid;
  const int nvs = B->nv_solve, nes = B->ne_solve;
  int      *e = B->edge;
  register int j;

  N = Nmodes-Nbmodes;

  for(j = 0; j < Nverts; ++j)
    if(vert[j].gid < nvs)
      Mat[vert[j].gid][loc] += vert[j].hj[0];

  for(j = 0; j < Nedges; ++j)
    if((gid = edge[j].gid) < nes){
      ll = edge[j].l;
      dvadd(ll,edge[j].hj,1,Mat[e[gid]]+loc,nm,Mat[e[gid]]+loc,nm);
    }

  tid = B->nsolve + offset;
  if(N) dvadd(N,*face->hj,1,Mat[tid]+loc,nm,Mat[tid]+loc,nm);
}




void Pyr::fill_column(double **, int , Bsystem *, int , int ){

}




void Prism::fill_column(double **, int , Bsystem *, int , int ){

}




void Hex::fill_column(double **Mat, int loc, Bsystem *B, int nm, int offset){
  int       N,ll,tid,gid;
  const int nvs = B->nv_solve, nes = B->ne_solve;
  int      *e = B->edge;
  register int j;

  N = Nmodes-Nbmodes;

  for(j = 0; j < NHex_verts; ++j)
    if(vert[j].gid < nvs)
      Mat[vert[j].gid][loc] += vert[j].hj[0];

  for(j = 0; j < NHex_edges; ++j)
    if((gid = edge[j].gid) < nes){
      ll = edge[j].l;
      dvadd(ll,edge[j].hj,1,Mat[e[gid]]+loc,nm,Mat[e[gid]]+loc,nm);
    }

  tid = B->nsolve + offset;
  if(N) dvadd(N,*face->hj,1,Mat[tid]+loc,nm,Mat[tid]+loc,nm);
}




void Element::fill_column(double **, int , Bsystem *, int, int ){ERR;}




/*

Function name: Element::GetZW

Function Purpose:

Argument 1: double **za
Purpose:

Argument 2: double **wa
Purpose:

Argument 3: double **zb
Purpose:

Argument 4: double **wb
Purpose:

Argument 5: double **zc
Purpose:

Argument 6: double **wc
Purpose:

Function Notes:

*/

void Tri::GetZW(double **za, double **wa, double **zb, double **wb, double **zc, double **wc){

  getzw(qa, za, wa, 'a');
  getzw(qb, zb, wb, 'b');
}




void Quad::GetZW(double **za, double **wa, double **zb, double **wb, double **, double **){

  getzw(qa, za, wa, 'a');
  getzw(qb, zb, wb, 'a');
}




void Tet::GetZW(double **za, double **wa, double **zb, double **wb, double **zc, double **wc){

  getzw(qa, za, wa, 'a');
  getzw(qb, zb, wb, 'b');
  getzw(qc, zc, wc, 'c');
}




void Pyr::GetZW(double **za, double **wa, double **zb, double **wb, double **zc, double **wc){

  getzw(qa, za, wa, 'a');
  getzw(qb, zb, wb, 'a');
  getzw(qc, zc, wc, 'c');
}




void Prism::GetZW(double **za, double **wa, double **zb, double **wb, double **zc, double **wc){

  getzw(qa, za, wa, 'a');
  getzw(qb, zb, wb, 'a');
  getzw(qc, zc, wc, 'b');
}




void Hex::GetZW(double **za, double **wa, double **zb, double **wb, double **zc, double **wc){

  getzw(qa, za, wa, 'a');
  getzw(qb, zb, wb, 'a');
  getzw(qc, zc, wc, 'a');
}




void Element::GetZW(double **, double **, double **, double **, double **, double **){ERR;}




/*

Function name: Element::fillvec

Function Purpose:

Argument 1: Mode *v
Purpose:

Argument 2: double *f
Purpose:

Function Notes:

*/

void Tri::fillvec(Mode *v, double *f){
  register int i;

  for(i = 0; i < qb; ++i)
    dcopy(qa,v->a,1,f+i*qa,1);
  for(i = 0; i < qa; ++i)
    dvmul(qb,v->b,1,f+i,qa,f+i,qa);
}




void Quad::fillvec(Mode *v, double *f){
  register int i;

  for(i = 0; i < qb; ++i)
    dcopy(qa,v->a,1,f+i*qa,1);
  for(i = 0; i < qa; ++i)
    dvmul(qb,v->b,1,f+i,qa,f+i,qa);
}




void Tet::fillvec(Mode *v, double *f){
  register int i,j;

 for(i = 0; i < qb*qc; ++i)
   dcopy(qa,v->a,1,f+i*qa,1);
 for(i = 0; i < qc; ++i)
   dsmul(qa*qb,v->c[i],f+i*qa*qb,1,f+i*qa*qb,1);
 for(i = 0; i < qc; ++i)
   for(j = 0; j < qb; ++j,f+=qa)
     dsmul(qa,v->b[j],f,1,f,1);

}




void Pyr::fillvec(Mode *v, double *f){
  register int i,j;

 for(i = 0; i < qb*qc; ++i)
   dcopy(qa,v->a,1,f+i*qa,1);
 for(i = 0; i < qc; ++i)
   dsmul(qa*qb,v->c[i],f+i*qa*qb,1,f+i*qa*qb,1);
 for(i = 0; i < qc; ++i)
   for(j = 0; j < qb; ++j,f+=qa)
     dsmul(qa,v->b[j],f,1,f,1);

}




void Prism::fillvec(Mode *v, double *f){
  register int i,j;

 for(i = 0; i < qb*qc; ++i)
   dcopy(qa,v->a,1,f+i*qa,1);
 for(i = 0; i < qc; ++i)
   dsmul(qa*qb,v->c[i],f+i*qa*qb,1,f+i*qa*qb,1);
 for(i = 0; i < qc; ++i)
   for(j = 0; j < qb; ++j,f+=qa)
     dsmul(qa,v->b[j],f,1,f,1);

}




void Hex::fillvec(Mode *v, double *f){
  register int i,j;

 for(i = 0; i < qb*qc; ++i)
   dcopy(qa,v->a,1,f+i*qa,1);
 for(i = 0; i < qc; ++i)
   dsmul(qa*qb,v->c[i],f+i*qa*qb,1,f+i*qa*qb,1);
 for(i = 0; i < qc; ++i)
   for(j = 0; j < qb; ++j,f+=qa)
     dsmul(qa,v->b[j],f,1,f,1);

}




void Element::fillvec(Mode *, double *){ERR;}     //




/*

Function name: Element::get_1diag_massmat

Function Purpose:

Argument 1: int ID
Purpose:

Function Notes:

*/

double Tri::get_1diag_massmat(int ID){
  double *wa, *wb;
  double *wvec = dvector(0, qtot-1), vmmat;
  Mode mw,*m;

#ifndef PCONTBASE
  double **ba, **bb;
  Mode m1;
  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);
  m1.a = ba[ID];
  m1.b = bb[ID];
  m = &m1;
#else
  Basis *b = getbasis();
  m = b->vert+ID;
#endif

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');

  mw.a = wa;  mw.b = wb;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  vmmat = Tri_mass_mprod(this, m, wvec);

  free(wvec);

  return vmmat;
}




double Quad::get_1diag_massmat(int ID){
  double *wa, *wb;
  double *wvec = dvector(0, qtot-1), vmmat;
  Mode mw,*m;

#ifndef PCONTBASE
  double **ba, **bb;
  Mode m1;
  get_moda_GL (qa, &ba);
  get_moda_GL (qb, &bb);
  m1.a = ba[ID];
  m1.b = bb[ID];
  m = &m1;
#else
  Basis *b = getbasis();
  m = b->vert+ID;
#endif

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'a');

  mw.a = wa;  mw.b = wb;

  fillvec(&mw, wvec);

  if(curvX)
    dvmul(qtot, wvec, 1, geom->jac.p, 1, wvec, 1);
  else
    dscal(qtot, geom->jac.d, wvec, 1);

  vmmat = Quad_mass_mprod(this, m, wvec);

  free(wvec);

  return vmmat;
}












double Element::get_1diag_massmat(int ){return 0.0;}
