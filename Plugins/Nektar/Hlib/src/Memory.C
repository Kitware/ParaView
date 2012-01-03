
#define WORK 1

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
#include <smart_ptr.hpp>

#include <stdio.h>

nektar::scoped_c_ptr<double> Tri_wk;

 double *Quad_wk = (double*)0;
 double *Quad_Jbwd_wk = (double*)0;
 double *Quad_Iprod_wk = (double*)0;
 double *Quad_Grad_wk = (double*)0;
 double *Quad_HelmHoltz_wk = (double*)0;

 double *Tet_wk = (double*)0;
 double *Tet_form_diprod_wk = (double*)0;
 double *Tet_Jbwd_wk = (double*)0;
 double *Tet_Iprod_wk = (double*)0;
 double *Tet_Grad_wk = (double*)0;
 double *Tet_HelmHoltz_wk = (double*)0;

 double *Pyr_wk = (double*)0;
 double *Pyr_form_diprod_wk = (double*)0;
 double *Pyr_Jbwd_wk = (double*)0;
 double *Pyr_Iprod_wk = (double*)0;
 double *Pyr_Grad_wk = (double*)0;
 double *Pyr_HelmHoltz_wk = (double*)0;
 double *Pyr_InterpToGaussFace_wk = (double*)0;

 double *Prism_wk = (double*)0;
 double *Prism_form_diprod_wk = (double*)0;
 double *Prism_Jbwd_wk = (double*)0;
 double *Prism_Iprod_wk = (double*)0;
 double *Prism_Grad_wk = (double*)0;
 double *Prism_HelmHoltz_wk = (double*)0;
 double *Prism_InterpToGaussFace_wk = (double*)0;

 double *Hex_wk = (double*)0;
 double *Hex_form_diprod_wk = (double*)0;
 double *Hex_Jbwd_wk = (double*)0;
 double *Hex_Iprod_wk = (double*)0;
 double *Hex_Grad_wk = (double*)0;
 double *Hex_HelmHoltz_wk = (double*)0;
 double *Hex_InterpToGaussFace_wk = (double*)0;


void Tri_work(){
  Tri_wk.reset(dvector(0,5*QGmax*QGmax-1));
}

void Quad_work(){

  if(Quad_wk){
    free(Quad_wk);
    free(Quad_Jbwd_wk);
    free(Quad_Iprod_wk);
    free(Quad_Grad_wk);
    free(Quad_HelmHoltz_wk);
  }
  int nmax = max(QGmax+2, LGmax+2);

  Quad_wk             = dvector(0, 2*nmax*nmax-1);
  Quad_Jbwd_wk        = dvector(0, 2*nmax*nmax-1);
  Quad_Iprod_wk       = dvector(0, 2*nmax*nmax-1);
  Quad_Grad_wk        = dvector(0, 2*nmax*nmax-1);
  Quad_HelmHoltz_wk   = dvector(0, 2*nmax*nmax-1);
}


void Tet_work(){

  if(Tet_wk){
    free(Tet_wk);
    free(Tet_form_diprod_wk);
    free(Tet_Jbwd_wk);
    free(Tet_Iprod_wk);
    free(Tet_Grad_wk);
    free(Tet_HelmHoltz_wk);
  }

  Tet_wk             = dvector(0, QGmax*QGmax*QGmax-1);
  Tet_form_diprod_wk = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Tet_Jbwd_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Tet_Iprod_wk       = dvector(0, 2*QGmax*QGmax*QGmax-1);
  Tet_Grad_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Tet_HelmHoltz_wk   = dvector(0, 4*QGmax*QGmax*QGmax-1);
}

void Pyr_work(){

  if(Pyr_wk){
    free(Pyr_wk);
    free(Pyr_form_diprod_wk);
    free(Pyr_Jbwd_wk);
    free(Pyr_Iprod_wk);
    free(Pyr_Grad_wk);
    free(Pyr_HelmHoltz_wk);
    free(Pyr_InterpToGaussFace_wk);
  }

  Pyr_wk             = dvector(0, QGmax*QGmax*QGmax-1);
  Pyr_form_diprod_wk = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Pyr_Jbwd_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Pyr_Iprod_wk       = dvector(0, QGmax*QGmax*QGmax-1);
  Pyr_Grad_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Pyr_HelmHoltz_wk   = dvector(0, 4*QGmax*QGmax*QGmax-1);
  Pyr_InterpToGaussFace_wk   = dvector(0, QGmax*QGmax-1);
}

void Prism_work(){

  if(Prism_wk){
    free(Prism_wk);
    free(Prism_form_diprod_wk);
    free(Prism_Jbwd_wk);
    free(Prism_Iprod_wk);
    free(Prism_Grad_wk);
    free(Prism_HelmHoltz_wk);
    free(Prism_InterpToGaussFace_wk);
  }

  Prism_wk             = dvector(0, QGmax*QGmax*QGmax-1);
  Prism_form_diprod_wk = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Prism_Jbwd_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Prism_Iprod_wk       = dvector(0, QGmax*QGmax*QGmax-1);
  Prism_Grad_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Prism_HelmHoltz_wk   = dvector(0, 4*QGmax*QGmax*QGmax-1);
  Prism_InterpToGaussFace_wk   = dvector(0, QGmax*QGmax-1);
}

void Hex_work(){

  if(Hex_wk){
    free(Hex_wk);
    free(Hex_form_diprod_wk);
    free(Hex_Jbwd_wk);
    free(Hex_Iprod_wk);
    free(Hex_Grad_wk);
    free(Hex_HelmHoltz_wk);
    free(Hex_InterpToGaussFace_wk);
  }

  Hex_wk             = dvector(0, QGmax*QGmax*QGmax-1);
  Hex_form_diprod_wk = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Hex_Jbwd_wk        = dvector(0, QGmax*QGmax*QGmax-1);
  Hex_Iprod_wk       = dvector(0, QGmax*QGmax*QGmax-1);
  Hex_Grad_wk        = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Hex_HelmHoltz_wk   = dvector(0, 4*QGmax*QGmax*QGmax-1);
  Hex_InterpToGaussFace_wk   = dvector(0, QGmax*QGmax-1);
}


/*

Function name: Element::Mem_Q

Function Purpose:

Function Notes:

*/

void Tri::Mem_Q(){

  if(qa != lmax+1){
    if(LZero)
      qa =  qb = lmax+1;
    else{
      qa = lmax+1;
      qb = lmax;
    }
  }

  qtot = qa*qb;

  if(qa>QGmax)
    QGmax=qa;
  if(qb>QGmax)
    QGmax=qb;

}




void Quad::Mem_Q(){

  if(qa != lmax+1){
    qa = lmax+1; qb = lmax+1;
  }
  qtot = qa*qb;

 if(qa>QGmax)
    QGmax=qa;
  if(qb>QGmax)
    QGmax=qb;
}




void Tet::Mem_Q(){
  int lm;

  if(qa != (lm=lmax+1)){
    if(LZero){
      qa = qb = qc = lm;
    }
    else{
      qa = lm; qb = lm-1; qc = lm-1;
    }
    qtot = qa*qb*qc;
  }

  if(qa>QGmax)
    QGmax=qa;
  if(qb>QGmax)
    QGmax=qb;
  if(qc>QGmax)
    QGmax=qc;

}




void Pyr::Mem_Q(){
 qb = qa = lmax+1;
 qc = lmax;
 QGmax = max(qa, QGmax);
}

void Prism::Mem_Q(){

  if(LZero){
    qa = qb =  qc = lmax+1;
  }
  else{
    qb = qa = lmax+1;
    qc = lmax;
  }

  qtot = qa*qb*qc;
  QGmax = max(qa, QGmax);
}


void Hex::Mem_Q(){
  qc = qb = qa = lmax+1;
  QGmax = max(qa, QGmax);
}


void Element::Mem_Q(){ERR;}




/*

Function name: Element::Mem_J

Function Purpose:

Argument 1: int *list
Purpose:

Argument 2: char
Purpose:

Function Notes:

*/

void Tri::Mem_J(int *list, char )
{
  register int i;
  int      ndata,nbl;

  /* check to see if any 'l' values are different */
  /* also recalculate new vector length */
  nbl = Nverts;
  for(i = 0; i < Nedges; ++i)
    nbl += list[i];

  ndata = nbl;
  ndata += list[i]*(list[i]+1)/2;

  Nmodes  = ndata;
  Nbmodes = nbl;

  for(i = 0; i < Nedges; ++i)
    edge[i].l  = list[i];

  face[0].l = list[i];

  lmax = 0;
  //  if(interior_l) lmax = interior_l+4;
  for(i = 0; i < Nedges; ++i) lmax = max(lmax,edge[i].l+2);
  for(i = 0; i < Nfaces; ++i)
    if(face[i].l) lmax = max(lmax,face[i].l+3);
  LGmax= (lmax > LGmax) ? lmax:LGmax;

}




void Quad::Mem_J(int *list, char )
{
  register int i;
  int      ndata,nbl;

  /* check to see if any 'l' values are different */
  /* also recalculate new vector length */
  nbl = Nverts;
  for(i = 0; i < Nedges; ++i)
    nbl += list[i];

  ndata = nbl;
  ndata += list[i]*list[i];

  Nmodes  = ndata;
  Nbmodes = nbl;

  for(i = 0; i < Nedges; ++i)
    edge[i].l  = list[i];

  face[0].l = list[i];

  lmax = 0;
  //  if(interior_l) lmax = interior_l+4;
  for(i = 0; i < Nedges; ++i) lmax = max(lmax,edge[i].l+2);
  for(i = 0; i < Nfaces; ++i)
    if(face[i].l) lmax = max(lmax,face[i].l+2);

   LGmax = (lmax>LGmax) ? lmax:LGmax;
}




void Tet::Mem_J(int *list, char ){
 int i, cnt=0, l,nfv;
  Nbmodes = Nverts;
  for(i=0;i<Nedges;++i,++cnt){
    l = list[cnt];
    edge[i].l = l;
    Nbmodes += l;
    lmax = max(lmax, l+2);
  }

  for(i=0;i<Nfaces;++i,++cnt){
    l = list[cnt];
    face[i].l = l;
    if(l){
      nfv = Nfverts(i);
      Nbmodes += (nfv == 3) ? l*(l+1)/2:l*l;
      lmax = max(lmax, (nfv == 3) ? l+3:l+2);
    }
  }

  l = list[cnt];
  if(l)
    lmax = max(lmax, l+3);
  interior_l = l;
  // needs to be fixed
  Nmodes = Nbmodes + l*(l+1)*(l+2)/6;

  LGmax= max(lmax, LGmax);
}




void Pyr::Mem_J(int *list, char ){
  int i, cnt=0, l,nfv;
  Nbmodes = NPyr_verts;
  for(i=0;i<NPyr_edges;++i,++cnt){
    l = list[cnt];
    edge[i].l = l;
    Nbmodes += l;
    lmax = max(lmax, l+2);
  }

  for(i=0;i<NPyr_faces;++i,++cnt){
    l = list[cnt];
    face[i].l = l;
    if(l){
      nfv = Nfverts(i);
      Nbmodes += (nfv == 3) ? l*(l+1)/2:l*l;
      lmax = max(lmax, (nfv == 3) ? l+3:l+2);
    }
  }

  l = list[cnt];
  if(l)
    lmax = max(lmax, l+3);
  interior_l = l;
  // needs to be fixed
  Nmodes = Nbmodes + l*(l+1)*(l+2)/6;

  LGmax= max(lmax, LGmax);
}




void Prism::Mem_J(int *list, char ){
  int i, cnt=0, l,nfv;
  Nbmodes = NPrism_verts;
  for(i=0;i<NPrism_edges;++i,++cnt){
    l = list[cnt];
    edge[i].l = l;
    Nbmodes += l;
    lmax = max(lmax, l+2);
  }

  for(i=0;i<NPrism_faces;++i,++cnt){
    l = list[cnt];
    face[i].l = l;
    if(l){
      nfv = Nfverts(i);
      Nbmodes += (nfv == 3) ? l*(l+1)/2:l*l;
      lmax = max(lmax, (nfv == 3) ? l+3:l+2);
    }
  }

  l = list[cnt];
  if(l)
    lmax = max(lmax, l+2);
  interior_l = l;
  // needs to be fixed
  Nmodes = Nbmodes + (l-1)*l*l/2;

  LGmax= max(lmax, LGmax);
}




void Hex::Mem_J(int *list, char ){
  int i, cnt=0, l;
  Nbmodes = NHex_verts;
  for(i=0;i<NHex_edges;++i,++cnt){
    l = list[cnt];
    edge[i].l = l;
    Nbmodes += l;
    lmax = max(lmax, l+2);
  }

  for(i=0;i<NHex_faces;++i,++cnt){
    l = list[cnt];
    face[i].l = l;
    if(l){
      Nbmodes += l*l;
      lmax = max(lmax, l+2);
    }
  }

  l = list[cnt];
  if(l)
    lmax = max(lmax, l+2);
  interior_l = l;
  Nmodes = Nbmodes + l*l*l;

  LGmax= max(lmax,LGmax);
}




void Element::Mem_J(int *, char){ERR;}




/*

Function name: Element::Mem_free

Function Purpose:

Function Notes:

*/

void Tri::Mem_free(){
  if(h)
    free_dmatrix(h,0,0);
  if(vert[0].hj)
    free(vert[0].hj);
}




void Quad::Mem_free(){
  if(h)
    free(*h);
  if(vert[0].hj)
    free(vert[0].hj);
}




void Tet::Mem_free(){
  if(h)
    free_dmatrix(h,0,0);
  if(h_3d)
    free_dtarray(h_3d,0,0,0);
  if(vert[0].hj)
    free(vert[0].hj);
}




void Pyr::Mem_free(){
  if(h)
    free_dmatrix(h,0,0);
  if(h_3d)
    free_dtarray(h_3d,0,0,0);
  if(vert[0].hj)
    free(vert[0].hj);
}




void Prism::Mem_free(){
  if(h)
    free_dmatrix(h,0,0);
  if(h_3d)
    free_dtarray(h_3d,0,0,0);
  if(vert[0].hj)
    free(vert[0].hj);
}




void Hex::Mem_free(){
  if(h)
    free_dmatrix(h,0,0);
  if(h_3d)
    free_dtarray(h_3d,0,0,0);
  if(vert[0].hj)
    free(vert[0].hj);
}




void Element::Mem_free(){ERR;}




/*

Function name: Element::Mem_shift

Function Purpose:

Argument 1: double *new_h
Purpose:

Argument 2: double *new_hj
Purpose:

Argument 3: char Trip
Purpose:

Function Notes:

*/

void Tri::Mem_shift(double *new_h, double *new_hj, char Trip){

  if (h)
    free(h);
  h = (double**) calloc(qb,sizeof(double*));

  if (face[0].hj)
    free(face[0].hj);
  face[0].hj = (double**) calloc(face[0].l,sizeof(double*));

  // Physical memory
  int i;
  for(i = 0; i < qb; ++i)
    h[i] = new_h+i*qa;

  // Modal Memory
  for(i = 0; i < Nverts; ++i, ++new_hj)
    vert[i].hj = new_hj;

  for(i = 0; i < Nedges; ++i){
    edge[i].hj = new_hj;
    new_hj += edge[i].l;
  }
  for(i = 0; i < face[0].l; ++i){
    face[0].hj[i] = new_hj;
    new_hj += face[0].l-i;
  }
}




void Quad::Mem_shift(double *new_h, double *new_hj, char ){

  h = (double**) malloc(qb*sizeof(double*));
  face[0].hj = (double**) malloc(face[0].l*sizeof(double*));

  // Physical memory
  int i;
  for(i = 0; i < qb; ++i)
    h[i] = new_h+i*qa;
  for(i = 0; i < Nverts; ++i, ++new_hj)
    vert[i].hj = new_hj;
  for(i = 0; i < Nedges; ++i){
    edge[i].hj = new_hj;
    new_hj += edge[i].l;
  }
  for(i = 0; i < face[0].l; ++i){
    face[0].hj[i] = new_hj;
    new_hj += face[0].l;
  }
}




void Tet::Mem_shift(double *new_h, double *new_hj, char trip){

  // Physical memory
  register int i,j,ll;
  int skipa,skipb;

  h_3d       = (double ***) malloc(qc*sizeof(double **));
  h_3d[0]    = (double **)  malloc(qb*qc*sizeof(double *));

  h_3d[0][0] = new_h;

  skipa = skipb = 0;

  for(i = 0; i < qc; ++i, skipb += qb){
    h_3d[i] = h_3d[0] + skipb;
    for(j = 0; j < qb; ++j, skipa += qa)
      h_3d[i][j] = h_3d[0][0] + skipa;
  }

  // Modal memory

  for(i = 0; i < Nverts; ++i, ++new_hj)
    vert[i].hj = new_hj;
  for(i = 0; i < Nedges; ++i){
    edge[i].hj = new_hj;
    new_hj += edge[i].l;
  }

  double *p   = new_hj;

  for(i = 0; i < Nfaces; ++i)
    if (ll=face[i].l){
      face[i].hj = (double **)malloc(ll*sizeof(double *));
      for(j = 0; j < ll; p+=ll-j, ++j)
  face[i].hj[j] = p;
    }
    else face[i].hj = (double **)malloc(sizeof(double *));

    // interior modes
  if(interior_l){
    hj_3d = (double ***)malloc(interior_l*sizeof(double**));
    double **p1;
    p1 = (double **)malloc((interior_l*(interior_l+1)/2)*sizeof(double *));
    for(i = 0; i < interior_l;p1+=interior_l-i,++i){
      hj_3d[i] = p1;
      for(j = 0; j < interior_l-i;p+=interior_l-i-j,++j)
  hj_3d[i][j] = p;
      }
  }
}




void Pyr::Mem_shift(double *new_h, double *new_hj, char trip){

  // Physical memory
  register int i,j,ll;
  int skipa,skipb;

  h_3d       = (double ***) malloc(qc*sizeof(double **));
  h_3d[0]    = (double **)  malloc(qc*qb*sizeof(double *));
  h_3d[0][0] = new_h;

  skipa = skipb = 0;

  for(i = 0; i < qc; ++i, skipb += qb){
    h_3d[i] = h_3d[0] + skipb;
    for(j = 0; j < qb; ++j, skipa += qa)
      h_3d[i][j] = h_3d[0][0] + skipa;
  }

  // Modal memory

  for(i = 0; i < NPyr_verts; ++i, ++new_hj)
    vert[i].hj = new_hj;
  for(i = 0; i < NPyr_edges; ++i){
    edge[i].hj = new_hj;
    new_hj += edge[i].l;
  }

  double *p   = new_hj;

  for(i = 0; i < NPyr_faces; ++i)
    if (ll=face[i].l){
      face[i].hj = (double **)malloc(ll*sizeof(double *));
      if(Nfverts(i) == 4)
  for(j = 0; j < ll; p+=ll, ++j)
    face[i].hj[j] = p;
      else
  for(j = 0; j < ll; p+=ll-j, ++j)
    face[i].hj[j] = p;
    }
    else face[i].hj = (double **)malloc(sizeof(double *));

  // interior modes
  if(interior_l){
    hj_3d = (double ***)malloc(interior_l*sizeof(double**));
    double **p1;
    p1 = (double **)malloc(interior_l*interior_l*sizeof(double *));
    for(i = 0; i < interior_l;p1+=interior_l-i,++i){
      hj_3d[i] = p1;
      for(j = 0; j < interior_l-i;  p += interior_l-i-j,++j)
  hj_3d[i][j] = p;
    }
  }
}




void Prism::Mem_shift(double *new_h, double *new_hj, char trip){

  // Physical memory
  register int i,j,ll;
  int skipa,skipb;

  h_3d       = (double ***) malloc(qc*sizeof(double **));
  h_3d[0]    = (double **)  malloc(qc*qb*sizeof(double *));

  h_3d[0][0] = new_h;

  skipa = skipb = 0;

  for(i = 0; i < qc; ++i, skipb += qb){
    h_3d[i] = h_3d[0] + skipb;
    for(j = 0; j < qb; ++j, skipa += qa)
      h_3d[i][j] = h_3d[0][0] + skipa;
  }

  // Modal memory

  for(i = 0; i < NPrism_verts; ++i, ++new_hj)
    vert[i].hj = new_hj;
  for(i = 0; i < NPrism_edges; ++i){
    edge[i].hj = new_hj;
    new_hj += edge[i].l;
  }

  double *p   = new_hj;

  for(i = 0; i < NPrism_faces; ++i)
    if (ll=face[i].l){
     face[i].hj = (double **)malloc(ll*sizeof(double *));
     if(Nfverts(i) == 4)
       for(j = 0; j < ll; p+=ll, ++j)
   face[i].hj[j] = p;
     else
       for(j = 0; j < ll; p+=ll-j, ++j)
   face[i].hj[j] = p;
    }
    else face[i].hj = (double **)malloc(sizeof(double *));

  // interior modes
  if(interior_l>1){ /* when interior_l==1 the triangular face
           means there are no interior modes */
    hj_3d = (double ***)malloc((interior_l-1)*sizeof(double**));
    double **p1;
    p1 = (double **)malloc(interior_l*(interior_l)*sizeof(double *));
    for(i = 0; i < interior_l-1;p1+=interior_l,++i){
      hj_3d[i] = p1;
      for(j = 0; j < interior_l;p+=interior_l-1-i,++j)
  hj_3d[i][j] = p;
    }
  }
}


void Hex::Mem_shift(double *new_h, double *new_hj, char trip){

    // Physical memory
    register int i,j,ll;
    int skipa,skipb;

    h_3d       = (double ***) malloc(qc*sizeof(double **));
    h_3d[0]    = (double **)  malloc(qb*qc*sizeof(double *));


    h_3d[0][0] = new_h;

    skipa = skipb = 0;

    for(i = 0; i < qc; ++i, skipb += qb){
      h_3d[i] = h_3d[0] + skipb;
      for(j = 0; j < qb; ++j, skipa += qa)
  h_3d[i][j] = h_3d[0][0] + skipa;
    }

    // Modal memory

    for(i = 0; i < NHex_verts; ++i, ++new_hj)
      vert[i].hj = new_hj;
    for(i = 0; i < NHex_edges; ++i){
      edge[i].hj = new_hj;
      new_hj += edge[i].l;
    }

    double *p   = new_hj;

    for(i = 0; i < NHex_faces; ++i)
      if (ll=face[i].l){
  face[i].hj = (double **)malloc(ll*sizeof(double *));

  for(j = 0; j < ll; p+=ll, ++j)
    face[i].hj[j] = p;
      }
      else face[i].hj = (double **)malloc(sizeof(double *));

    // interior modes
    if(interior_l){
      hj_3d = (double ***)malloc(interior_l*sizeof(double**));
      double **p1;
      p1 = (double **)malloc((interior_l*interior_l)*sizeof(double *));
      for(i = 0; i < interior_l;p1+=interior_l,++i){
  hj_3d[i] = p1;
  for(j = 0; j < interior_l;p+=interior_l,++j)
    hj_3d[i][j] = p;
      }
    }
}




void Element::Mem_shift(double *, double *, char){ERR;}




/*

Function name: Element::mat_mem

Function Purpose:

Function Notes:

*/

LocMat *Tri::mat_mem(){
  LocMat *m = (LocMat *)calloc(1,sizeof(LocMat));
  int    bsize,isize,i;

  bsize = Nverts;
  for(i=0;i<Nedges;++i)
    bsize += edge[i].l;

  isize = 0;
  for(i=0;i<Nfaces;++i)
    isize += Nfmodes();

  m->asize = bsize;
  m->csize = isize;

  m->list = ivector(0,bsize-1);
  m->a    = dmatrix(0,bsize-1,0,bsize-1);
  dzero(bsize*bsize, m->a[0], 1);

  if(isize){
    m->b = dmatrix(0,bsize-1,0,isize-1);
    m->c = dmatrix(0,isize-1,0,isize-1);
    dzero(bsize*isize, m->b[0], 1);
    dzero(isize*isize, m->c[0], 1);
    if(option("Oseen")){
      m->d = dmatrix(0,bsize-1,0,isize-1);
      dzero(isize*bsize, m->d[0],1);
    }
    else
      m->d = (double **)NULL;
  }


  return m;
}




LocMat *Quad::mat_mem(){
  LocMat *m = (LocMat *)malloc(sizeof(LocMat));
  int    bsize,isize,i;

  bsize = Nverts;
  for(i=0;i<Nedges;++i)
    bsize += edge[i].l;

  isize = 0;
  for(i=0;i<Nfaces;++i)
    isize += face[i].l*face[i].l;

  m->asize = bsize;
  m->csize = isize;

  m->list = ivector(0,bsize-1);
  m->a    = dmatrix(0,bsize-1,0,bsize-1);
  dzero(bsize*bsize, m->a[0], 1);

  if(isize){
    m->b = dmatrix(0,bsize-1,0,isize-1);
    m->c = dmatrix(0,isize-1,0,isize-1);
    dzero(bsize*isize, m->b[0], 1);
    dzero(isize*isize, m->c[0], 1);
    if(option("Oseen")){
      m->d = dmatrix(0,bsize-1,0,isize-1);
      dzero(isize*bsize, m->d[0],1);
    }
    else
      m->d = (double **)NULL;
  }

  return m;
}




LocMat *Tet::mat_mem(){
  LocMat *m = (LocMat *)malloc(sizeof(LocMat));
  int    bsize,isize;

  bsize = Nbmodes;
  isize = Nmodes - Nbmodes;

  m->asize = bsize;
  m->csize = isize;

  m->list = ivector(0,bsize-1);
  m->a    = dmatrix(0,bsize-1,0,bsize-1);
  dzero(bsize*bsize, m->a[0], 1);

  if(isize){
    m->b = dmatrix(0,bsize-1,0,isize-1);
    m->c = dmatrix(0,isize-1,0,isize-1);
    dzero(bsize*isize, m->b[0], 1);
    dzero(isize*isize, m->c[0], 1);
  }


  return m;
}




LocMat *Pyr::mat_mem(){
  LocMat *m = (LocMat *)malloc(sizeof(LocMat));
  int    bsize,isize;

  bsize = Nbmodes;
  isize = Nmodes - Nbmodes;

  m->asize = bsize;
  m->csize = isize;

  m->list = ivector(0,bsize-1);
  m->a    = dmatrix(0,bsize-1,0,bsize-1);
  dzero(bsize*bsize, m->a[0], 1);

  if(isize){
    m->b = dmatrix(0,bsize-1,0,isize-1);
    m->c = dmatrix(0,isize-1,0,isize-1);
    dzero(bsize*isize, m->b[0], 1);
    dzero(isize*isize, m->c[0], 1);
  }


  return m;
}




LocMat *Prism::mat_mem(){
  LocMat *m = (LocMat *)malloc(sizeof(LocMat));
  int    bsize,isize;

  bsize = Nbmodes;
  isize = Nmodes - Nbmodes;

  m->asize = bsize;
  m->csize = isize;

  m->list = ivector(0,bsize-1);
  m->a    = dmatrix(0,bsize-1,0,bsize-1);
  dzero(bsize*bsize, m->a[0], 1);

  if(isize){
    m->b = dmatrix(0,bsize-1,0,isize-1);
    m->c = dmatrix(0,isize-1,0,isize-1);
    dzero(bsize*isize, m->b[0], 1);
    dzero(isize*isize, m->c[0], 1);
  }


  return m;
}




LocMat *Hex::mat_mem(){
  LocMat *m = (LocMat *)malloc(sizeof(LocMat));
  int    bsize,isize;

  bsize = Nbmodes;
  isize = Nmodes - Nbmodes;

  m->asize = bsize;
  m->csize = isize;

  m->list = ivector(0,bsize-1);
  m->a    = dmatrix(0,bsize-1,0,bsize-1);
  dzero(bsize*bsize, m->a[0], 1);

  if(isize){
    m->b = dmatrix(0,bsize-1,0,isize-1);
    m->c = dmatrix(0,isize-1,0,isize-1);
    dzero(bsize*isize, m->b[0], 1);
    dzero(isize*isize, m->c[0], 1);
  }


  return m;
}




LocMat *Element::mat_mem(){ERR;return (LocMat*)NULL;}




/*

Function name: Element::mat_free

Function Purpose:

Argument 1: LocMat *m
Purpose:

Function Notes:

*/

void  Tri::mat_free(LocMat *m){

  free(m->list);
  free_dmatrix(m->a,0,0);
  if(m->csize){
    free_dmatrix(m->b,0,0);
    free_dmatrix(m->c,0,0);
    if(m->d)
      free_dmatrix(m->d,0,0);
  }
  free(m);
}




void  Quad::mat_free(LocMat *m){
  free(m->list);
  free_dmatrix(m->a,0,0);
  if(m->csize){
    free_dmatrix(m->b,0,0);
    free_dmatrix(m->c,0,0);
    if(m->d)
      free_dmatrix(m->d,0,0);
  }
  free(m);
}




void  Tet::mat_free(LocMat *m){

  free(m->list);
  free_dmatrix(m->a,0,0);
  if(m->csize){
    free_dmatrix(m->b,0,0);
    free_dmatrix(m->c,0,0);
  }
  free(m);
}




void  Pyr::mat_free(LocMat *m){

  free(m->list);
  free_dmatrix(m->a,0,0);
  if(m->csize){
    free_dmatrix(m->b,0,0);
    free_dmatrix(m->c,0,0);
  }
  free(m);
}




void  Prism::mat_free(LocMat *m){

  free(m->list);
  free_dmatrix(m->a,0,0);
  if(m->csize){
    free_dmatrix(m->b,0,0);
    free_dmatrix(m->c,0,0);
  }
  free(m);
}




void  Hex::mat_free(LocMat *m){

  free(m->list);
  free_dmatrix(m->a,0,0);
  if(m->csize){
    free_dmatrix(m->b,0,0);
    free_dmatrix(m->c,0,0);
  }
  free(m);
}




void Element::mat_free(LocMat *){ERR;}




/*

Function name: Element::divmat_mem

Function Purpose:

Argument 1: Element *P
Purpose:

Function Notes:

*/

LocMatDiv *Tri::divmat_mem(Element *P){
  LocMatDiv *m = (LocMatDiv *)malloc(sizeof(LocMatDiv));
  int    bsize,isize,psize,i;

  psize = Nverts;
  bsize = Nverts;
  for(i=0;i<Nedges;++i){
    bsize += edge[i].l;
    psize += P->edge[i].l;
  }

  isize = 0;
  for(i=0;i<Nfaces;++i){
    isize += face[i].l*(face[i].l+1)/2;
    psize += P->face[i].l*(P->face[i].l+1)/2;
  }

  m->bsize = bsize;
  m->isize = isize;
  m->rows  = psize;

  m->Atot  = dmatrix(0,2*bsize,0,2*bsize);
  m->Dxb   = dmatrix(0,psize-1,0,bsize-1);
  dzero(psize*bsize, m->Dxb[0], 1);
  m->Dyb   = dmatrix(0,psize-1,0,bsize-1);
  dzero(psize*bsize, m->Dyb[0], 1);

  if(isize){
    m->Dxi = dmatrix(0,psize-1,0,isize-1);
    dzero(psize*isize, m->Dxi[0], 1);
    m->Dyi = dmatrix(0,psize-1,0,isize-1);
    dzero(psize*isize, m->Dyi[0], 1);
  }

  return m;
}




LocMatDiv *Quad::divmat_mem(Element *P){
  LocMatDiv *m = (LocMatDiv *)malloc(sizeof(LocMatDiv));
  int    bsize,isize,psize,i;

  psize = Nverts;
  bsize = Nverts;
  for(i=0;i<Nedges;++i){
    bsize += edge[i].l;
    psize += P->edge[i].l;
  }

  isize = 0;
  for(i=0;i<Nfaces;++i){
    isize += face[i].l*face[i].l;
    psize += P->face[i].l*P->face[i].l;
  }

  m->bsize = bsize;
  m->isize = isize;
  m->rows  = psize;

  m->Atot  = dmatrix(0,2*bsize,0,2*bsize);
  m->Dxb   = dmatrix(0,psize-1,0,bsize-1);
  dzero(psize*bsize, m->Dxb[0], 1);
  m->Dyb   = dmatrix(0,psize-1,0,bsize-1);
  dzero(psize*bsize, m->Dyb[0], 1);

  if(isize){
    m->Dxi = dmatrix(0,psize-1,0,isize-1);
    dzero(psize*isize, m->Dxi[0], 1);
    m->Dyi = dmatrix(0,psize-1,0,isize-1);
    dzero(psize*isize, m->Dyi[0], 1);
  }

  return m;
}


LocMatDiv *Element::divmat_mem(Element *){ERR;return NULL;}




/*

Function name: Element::divmat_free

Function Purpose:

Argument 1: LocMatDiv *m
Purpose:

Function Notes:

*/

void  Tri::divmat_free(LocMatDiv *m){

  free_dmatrix(m->Atot,0,0);
  free_dmatrix(m->Dxb,0,0);
  free_dmatrix(m->Dyb,0,0);
  if(m->isize){
    free_dmatrix(m->Dxi,0,0);
    free_dmatrix(m->Dyi,0,0);
  }
  free(m);
}




void  Quad::divmat_free(LocMatDiv *m){

  free_dmatrix(m->Atot,0,0);
  free_dmatrix(m->Dxb,0,0);
  free_dmatrix(m->Dyb,0,0);
  if(m->isize){
    free_dmatrix(m->Dxi,0,0);
    free_dmatrix(m->Dyi,0,0);
  }
  free(m);
}

void Element::divmat_free(LocMatDiv *){ERR;}




/*

Function name: Element::MemBndry

Function Purpose:

Argument 1: Bndry *B
Purpose:

Argument 2: int fac
Purpose:

Argument 3: int Je
Purpose:

Function Notes:

*/

void Tri::MemBndry(Bndry *B, int fac, int Je){
  int L;
  B->bvert = dvector(0,Je*Tri_DIM-1);
  dzero(Je*Tri_DIM,B->bvert,1);

  if(L = edge[fac].l){
    B->bedge[0] =  dvector(0,Je*L-1);
    dzero(Je*L,B->bedge[0],1);
  }
}




void Quad::MemBndry(Bndry *B, int fac, int Je){
  int L;
  B->bvert = (double *)calloc(Je*Quad_DIM,sizeof(double));

  if(L = edge[fac].l)
    B->bedge[0] =  (double *)calloc(Je*L,sizeof(double));
}




void Tet::MemBndry(Bndry *B, int fac, int Je){
  int L;
  register int i;

  B->bvert = (double *)calloc(Je*Tet_DIM ,sizeof(double));

  for(i = 0; i < Nfverts(fac); ++i)
    if(L = edge[ednum(fac,i)].l)
      B->bedge[i] =  (double *)calloc(Je*L,sizeof(double));

  if(L = face[fac].l){
    B->bface = (double **)malloc(L*sizeof(double*));
    B->bface[0] = dvector(0,Je*(L*(L+1)/2)-1);
    for(i = 1; i < L; ++i)  B->bface[i] = B->bface[i-1] + L+1-i;

    dzero(Je*(L*(L+1)/2),B->bface[0],1);
  }
  else
    B->bface = (double **)malloc(sizeof(double *));
}

void Pyr::MemBndry(Bndry *B, int fac, int Je){
  int L;
  register int i;

  B->bvert = (double *)calloc(Je*Nfverts(fac) ,sizeof(double));

  for(i = 0; i < Nfverts(fac); ++i)
    if(L = edge[ednum(fac,i)].l)
      B->bedge[i] =  (double *)calloc(Je*L,sizeof(double));

  if(L = face[fac].l){
    B->bface = (double **)malloc(L*sizeof(double*));
    if(Nfverts(fac) == 4){
      B->bface[0] = dvector(0,Je*L*L-1);
      for(i = 1; i < L; ++i)  B->bface[i] = B->bface[i-1] + L;
      dzero(Je*L*L,B->bface[0],1);
    }
    else{
      B->bface[0] = dvector(0,Je*L*(L+1)/2-1);
      for(i = 1; i < L; ++i)  B->bface[i] = B->bface[i-1] + L+1-i;
      dzero(Je*L*(L+1)/2,B->bface[0],1);
    }
  }
  else
    B->bface = (double **)malloc(sizeof(double *));
}




void Prism::MemBndry(Bndry *B, int fac, int Je){
  int L;
  register int i;

  B->bvert = (double *)calloc(Je*Nfverts(fac) ,sizeof(double));

  for(i = 0; i < Nfverts(fac); ++i)
    if(L = edge[ednum(fac,i)].l)
      B->bedge[i] =  (double *)calloc(Je*L,sizeof(double));

  if(L = face[fac].l){
    B->bface = (double **)malloc(L*sizeof(double*));
    if(Nfverts(fac) == 4){
      B->bface[0] = dvector(0,Je*L*L-1);
      for(i = 1; i < L; ++i)  B->bface[i] = B->bface[i-1] + L;
      dzero(Je*L*L,B->bface[0],1);
    }
    else{
      B->bface[0] = dvector(0,Je*(L*(L+1)/2)-1);
      for(i = 1; i < L; ++i)  B->bface[i] = B->bface[i-1] + L+1-i;
      dzero(Je*(L*(L+1)/2),B->bface[0],1);
    }
  }
  else
    B->bface = (double **)calloc(1,sizeof(double *));
}




void Hex::MemBndry(Bndry *B, int fac, int Je){
  int L;
  register int i;

  B->bvert = (double *)calloc(Je*Nfverts(fac) ,sizeof(double));

  for(i = 0; i < Nfverts(fac); ++i)
    if(L = edge[ednum(fac,i)].l)
      B->bedge[i] =  (double *)calloc(Je*L,sizeof(double));

  if(L = face[fac].l){
    B->bface = (double **)malloc(L*sizeof(double*));
    B->bface[0] = dvector(0,Je*L*L-1);
    for(i = 1; i < L; ++i)  B->bface[i] = B->bface[i-1] + L;

    dzero(Je*L*L,B->bface[0],1);
  }
  else
    B->bface = (double **)calloc(1,sizeof(double *));
}




void Element::MemBndry(Bndry *, int , int ){ERR;}
