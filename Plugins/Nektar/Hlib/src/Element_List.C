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

Element *el,*fl,*gl,*hl;

#define WARN fprintf(stderr,"Call to void Element function\n");
#define Eloop(E) for(el=E;el;el=el->next)
#define EFloop(E,F) for(el=E, fl=F;el&&fl;el=el->next,fl=fl->next)
#define EFGloop(E,F,G) for(el=E, fl=F, gl=G;el&&fl&&gl;el=el->next,fl=fl->next,gl=gl->next)
#define EFGHloop(E,F,G,H) for(el=E,fl=F,gl=G,hl=H;el&&fl&&gl&&hl;el=el->next,fl=fl->next,gl=gl->next,hl=hl->next)

int countelements(Element *U){
  int nel = 0;
  for(;U;U = U->next, ++nel);
  return nel;
}

Element_List::Element_List(){
  fhead   = (Element*)NULL;
  nel     = 0;
  hjtot   = 0;
  htot    = 0;
  base_h  = (double *)0;
  base_hj = (double *)0;

  nz      = 1;
  nztot   = 1;
  flevels = (Element_List**)NULL;
}

Element_List::Element_List(Element **hea, int n){
  flist  = hea;
  fhead  = *hea;
  nel    = n; // countelements(*hea);
#ifdef DEBUG
  if(n!=nel)
    fprintf(stderr,"Element_List::Element_List error list incomplete\n");
#endif
  hjtot  = 0;
  htot   = 0;
  base_h = (double *)0;
  base_hj= (double *)0;

  nz      = 1;
  nztot   = 1;
  flevels = (Element_List**)NULL;
}

Element_List::~Element_List()
{
  for (int i = 0; i < nel; ++i) {
    delete flist[i];
  }
  free(flist);

  if (base_h)
    free(base_h);
  if (base_hj)
    free(base_hj);
}


Element *Element_List::operator()(int i){
#ifdef DEBUG
  if(i>nel-1)
    fprintf(stderr,"Element_List::operator() bounds error\n");
#endif
  return flist[i];
}

Element *Element_List::operator[](int i){
#ifdef DEBUG
  if(i>nel-1)
    fprintf(stderr,"Element_List::operator() bounds error\n");
#endif
  return flist[i];
}

void Element_List::Cat_mem(){
  htot = 0;
  hjtot = 0;
  Eloop(fhead){
    htot  += el->qtot;
    hjtot += el->Nmodes;
  }

  base_h  = dvector(0, nz*htot-1);
  base_hj = dvector(0, nz*hjtot-1);

  double *tmp_base_h  = base_h;
  double *tmp_base_hj = base_hj;

  dzero(nz*htot,   base_h, 1);
  dzero(nz*hjtot, base_hj, 1);

  Eloop(fhead){
    el->Mem_shift(tmp_base_h, tmp_base_hj, 'n');
    tmp_base_h  += el->qtot;
    tmp_base_hj += el->Nmodes;
  }

}

void Element_List::Trans(Element_List *EL, Nek_Trans_Type ntt){
  // Treat FFT as a global operation
  if(ntt == F_to_P || ntt == P_to_F){
    fprintf(stderr,"Element_List::FFT should have called Fourier_List\n");
    return;
  }
  else
    EFloop(fhead,EL->fhead)
      el->Trans(fl, ntt);
}

int *Tri_get_global_mapping(Element_List *E){
  int lmax = E->fhead->lmax;
  int nel  = E->nel;
  int *map = ivector(0,nel*(lmax*(lmax+1)/2+1)-1);
  int cnt  = 0,start,i,j,k,c;

  start=0;
  for(k = 0; k < nel; start+=E->flist[k]->Nmodes,++k){
    map[cnt++] = start+2; /* vertex 3 */
    map[cnt++] = start+1; /* vertex 2 */
    for(i = 0; i < lmax-2; ++i) /* edge 2 */
      map[cnt++] = start+ lmax+1 + i;
  }

  start=0;
  for(k = 0; k < nel; start+=E->flist[k]->Nmodes,++k){
    map[cnt++] = start + 2; /* vertex 3 */
    map[cnt++] = start + 0; /* vertex 2 */
    for(i = 0; i < lmax-2; ++i) /* edge 3 */
      map[cnt++] = start + 2*lmax-1 + i;
  }

  for(i = 0, c=E->fhead->Nbmodes; i < lmax-2; c+=lmax-3-i,++i){
    start = 0;
    for(k = 0; k < nel; start+=E->flist[k]->Nmodes,++k){
      map[cnt++] = start + 3 + i; /* edge 1 */
      for(j = 0; j < lmax-3-i;++j)
  map[cnt++] = start + c+j; /* interior */
    }
  }
  return map;
}

// FIX !!!!!!!!!!!!!!

int *Quad_get_global_mapping(Element_List *E){
  int lmax = E->fhead->lmax;
  int nel  = E->nel;
  int *map = ivector(0,nel*(lmax*(lmax+1)/2+1)-1);
  int cnt  = 0,start,i,j,k,c;
  // counting edges/verts from 0
  start=0;
  for(k = 0; k < nel; start+=E->flist[k]->Nmodes,++k){
    if(E->flist[k]->identify() == Nek_Quad){
      map[cnt++] = start+2;       /* vertex 2 */
      map[cnt++] = start+3;       /* vertex 3 */
      for(i = 0; i < lmax-2; ++i) /* edge 2   */
  map[cnt++] = start+ E->flist[k]->Nverts + 2*(lmax-2)+ i;
    }
  }

  start=0;
  for(k = 0; k < nel; start+=E->flist[k]->Nmodes,++k){
    if(E->flist[k]->identify() == Nek_Quad){
      map[cnt++] = start + 1;     /* vertex 1 */
      map[cnt++] = start + 0;     /* vertex 0 */
      for(i = 0; i < lmax-2; ++i) /* edge 1   */
  map[cnt++] = start + E->flist[k]->Nverts + i;
    }
  }

  for(i = 0, c=E->fhead->Nbmodes; i < lmax-2; c+=lmax-2,++i){
    start = 0;
    for(k = 0; k < nel; start+=E->flist[k]->Nmodes,++k){
      map[cnt++] = start + 3 + i; /* edge 1 */
      for(j = 0; j < lmax-3-i;++j)
  map[cnt++] = start + c+j; /* interior */
    }
  }
  return map;
}

/* assumes that quadrature storage is larger than modal storage */
static void InnerPB_Tri(Element_List *E, Element_List *Ef,
      Basis *ba, Basis *bb){
  register int  k;
  int      nel  = E->nel;
  int      lmax;
  //  Basis    *b;
  int       qa = E->fhead->qa, qb = E->fhead->qb;
  double   **wk,*h,*wa,*wb;
  static int *Tri_mapping = (int*)0;

  lmax = E->fhead->lmax;

  if(!Tri_mapping) Tri_mapping = Tri_get_global_mapping(E);

  wk = dmatrix(0,max(qa,lmax)-1,0,qb*nel-1);

  E->Set_state('t');

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');

  /* fill wk with weights */
  for(k = 0; k < qb; ++k)
    dsmul(qa,wb[k],wa,1,*wk+k*qa,1);

  /* multiply field by weights and jacobean*/
  for(k = 0; k < nel; ++k){
    dvmul(qa*qb,*wk,1,*Ef->flist[k]->h,1,*E->flist[k]->h,1);
    if(E->flist[k]->curvX)
      dvmul(qa*qb,E->flist[k]->geom->jac.p,1,*E->flist[k]->h,1,*E->flist[k]->h,1);
    else
      dscal(qa*qb,E->flist[k]->geom->jac.d,*E->flist[k]->h,1);
  }

  h = E->base_h;

  /* inner product w.r.t. 'a' basis */
  dgemm('T','N',qb*nel,lmax,qa,1.0,h,qa,ba->vert[1].a,qa,0.0,*wk,qb*nel);

  /* inner product w.r.t. 'b' basis */
  /* vertices and edges 2,3 */
  dgemm('T','N',lmax,2*nel,qb,1.0,bb->vert[2].b,qb,*wk,qb,0.0,h,lmax);

  /* add degenerate vertex together */
  for(k = 0; k < nel; ++k){
    h[k*lmax] += h[(k+nel)*lmax];
    h[(k+nel)*lmax]  = h[k*lmax];
  }

  h += 2*lmax*nel;

  /* edge 1 and interior */
  for(k = 0; k < lmax-2; h+=nel*(lmax-2-k), ++k)
    dgemm('T','N',lmax-2-k,nel,qb,1.0,bb->edge[0][k].b,qb,
    wk[k+2],qb,0.0,h,lmax-2-k);

  /* scatter all modes into h */
  dscatr((lmax*(lmax+1)/2+1)*nel,E->base_h,Tri_mapping,E->base_hj);

  free_dmatrix(wk,0,0);
}
// FIX !!!!!!!!!!!!!!

/* assumes that quadrature storage is larger than modal storage */
static void InnerPB_Quad(Element_List *E, Element_List *Ef,
      Basis *ba, Basis *bb){
  register int  k;
  int      nel  = E->nel;
  int      lmax;
  //  Basis    *b;
  int       qa = E->fhead->qa, qb = E->fhead->qb;
  double   **wk,*h,*wa,*wb;
  static int *Quad_mapping;

  lmax = E->fhead->lmax;

  if(!Quad_mapping) Quad_mapping = Quad_get_global_mapping(E);

  wk = dmatrix(0,lmax-1,0,qb*nel-1);

  E->Set_state('t');

  getzw(qa,&wa,&wa,'a');
  getzw(qb,&wb,&wb,'b');

  /* fill wk with weights */
  for(k = 0; k < qb; ++k)
    dsmul(qa,wb[k],wa,1,*wk+k*qa,1);

  /* multiply field by weights and jacobean*/
  for(k = 0; k < nel; ++k){
    dvmul(qa*qb,*wk,1,*Ef->flist[k]->h,1,*E->flist[k]->h,1);
    if(E->flist[k]->curvX)
      dvmul(qa*qb,E->flist[k]->geom->jac.p,1,*E->flist[k]->h,1,*E->flist[k]->h,1);
    else
      dscal(qa*qb,E->flist[k]->geom->jac.d,*E->flist[k]->h,1);
  }

  h = E->base_h;

  /* inner product w.r.t. 'a' basis */
  dgemm('T','N',qb*nel,lmax,qa,1.0,h,qa,ba->vert[1].a,qa,0.0,*wk,qb*nel);

  /* inner product w.r.t. 'b' basis */
  /* vertices and edges 2,3 */
  dgemm('T','N',lmax,2*nel,qb,1.0,bb->vert[2].b,qb,*wk,qb,0.0,h,lmax);

  /* add degenerate vertex together */
  for(k = 0; k < nel; ++k){
    h[k*lmax] += h[(k+nel)*lmax];
    h[(k+nel)*lmax]  = h[k*lmax];
  }

  h += 2*lmax*nel;
  /* edge 1 and interior */
  for(k = 0; k < lmax-2; h+=nel*(lmax-2-k), ++k)
    dgemm('T','N',lmax-2-k,nel,qb,1.0,bb->edge[0][k].b,qb,
    wk[k+2],qb,0.0,h,lmax-2-k);

  /* scatter all modes into h */
  dscatr((lmax*(lmax+1)/2+1)*nel,E->base_h,Quad_mapping,E->base_hj);

  free_dmatrix(wk,0,0);
}



void Element_List::Iprod(Element_List *EL){
#if 1
    EFloop(fhead,EL->fhead)
      el->Iprod(fl);
#else
  if(!option("variable")&&(fhead->dim() == 2)){ /* fixed element version */
    Basis *b = EL->fhead->getbasis();
    InnerPB_Tri (this,EL,b,b);
    InnerPB_Quad(this,EL,b,b);
  }
  else
    EFloop(fhead,EL->fhead)
      el->Iprod(fl);
#endif
}

void Element_List::Grad(Element_List *AL, Element_List *BL, Element_List *CL,
      char Trip){
  Element *A,*B,*C;
  Element *nul = NULL;

  A = (AL && AL->fhead) ? AL->fhead : (Element*)NULL;
  B = (BL && BL->fhead) ? BL->fhead : (Element*)NULL;
  C = (CL && CL->fhead) ? CL->fhead : (Element*)NULL;

  if(nz > 1 && C && (Trip == 'a' || Trip ==  'z')){
    fprintf(stderr,"Element_List::FFT should have called Fourier_List\n");
    return;
  }

  if(A && B && C)
    EFGHloop(fhead,A,B,C)
      el->Grad(fl,gl,hl,Trip);
  else if(A && B)
    EFGloop(fhead, A, B)
      el->Grad(fl,gl, nul, 'a');
  else if(A)
    EFloop(fhead,A)
      el->Grad(fl,nul,nul,'x');
  else if(B)
    EFloop(fhead,B)
      el->Grad(nul,fl,nul,'y');
  else if(C)
    EFloop(fhead,C)
      el->Grad(nul,nul,fl,'z');
}


void Element_List::GradT(Element_List *AL, Element_List *BL, Element_List *CL,
      char Trip){
    GradT(AL,BL,CL,Trip,0);
}

void Element_List::GradT(Element_List *AL, Element_List *BL, Element_List *CL,
       char Trip, bool invW){
  Element *A,*B,*C;
  Element *nul = NULL;

  A = (AL && AL->fhead) ? AL->fhead : (Element*)NULL;
  B = (BL && BL->fhead) ? BL->fhead : (Element*)NULL;
  C = (CL && CL->fhead) ? CL->fhead : (Element*)NULL;

  if(nz > 1 && C && (Trip == 'a' || Trip ==  'z')){
    fprintf(stderr,"Element_List::FFT should have called Fourier_List\n");
    return;
  }

  if(A && B && C)
    EFGHloop(fhead,A,B,C)
      el->GradT(fl,gl,hl,Trip,invW);
  else if(A && B)
    EFGloop(fhead, A, B)
      el->GradT(fl,gl, nul, 'a',invW);
  else if(A)
    EFloop(fhead,A)
      el->GradT(fl,nul,nul,'x',invW);
  else if(B)
    EFloop(fhead,B)
      el->GradT(nul,fl,nul,'y',invW);
  else if(C)
    EFloop(fhead,C)
      el->GradT(nul,nul,fl,'z',invW);
}


void Element_List::Grad_d(double *AL, double *BL, double *CL, char Trip){
  Element *A;
  double  *nul = (double*)0;

  if(AL && BL && CL)
    for(A=fhead;A;AL+=A->qtot, BL+=A->qtot, CL+= A->qtot, A=A->next)
      A->Grad_d(AL,BL,CL,'a');
  else if(AL && BL)
    for(A=fhead;A;AL+=A->qtot, BL+=A->qtot, A=A->next)
      A->Grad_d(AL,BL,nul,'a');
  else if(AL)
    for(A=fhead;A;AL+=A->qtot,A=A->next)
      A->Grad_d(AL,nul,nul,'x');
  else if(BL)
    for(A=fhead;A;BL+=A->qtot,A=A->next)
      A->Grad_d(nul,BL,nul,'y');
  else if(CL)
    for(A=fhead;A;CL+=A->qtot,A=A->next)
      A->Grad_d(nul,nul,CL,'z');
}


void Element_List::Grad_h(double *HL, double *AL, double *BL, double *CL, char Trip){
  Element *A;
  double  *nul = (double*)0;

  if(AL && BL && CL)
    for(A=fhead;A;AL+=A->qtot, BL+=A->qtot, CL+= A->qtot, HL += A->qtot, A=A->next)
      A->Grad_h(HL,AL,BL,CL,'a');
  else if(AL && BL)
    for(A=fhead;A;AL+=A->qtot, BL+=A->qtot, HL += A->qtot, A=A->next)
      A->Grad_h(HL,AL,BL,nul,'a');
  else if(AL)
    for(A=fhead;A;AL+=A->qtot, HL += A->qtot,A=A->next)
      A->Grad_h(HL,AL,nul,nul,'x');
  else if(BL)
    for(A=fhead;A;BL+=A->qtot, HL += A->qtot,A=A->next)
      A->Grad_h(HL,nul,BL,nul,'y');
  else if(CL)
    for(A=fhead;A;CL+=A->qtot, HL += A->qtot,A=A->next)
      A->Grad_h(HL,nul,nul,CL,'z');
}


void Element_List::GradT_h(double *HL, double *AL, double *BL, double *CL, char Trip){
    GradT_h(HL,AL,BL,CL,Trip,0);
}
void Element_List::GradT_h(double *HL, double *AL, double *BL, double *CL, char Trip, bool invW){
  Element *A;
  double  *nul = (double*)0;

  if(AL && BL && CL)
    for(A=fhead;A;AL+=A->qtot, BL+=A->qtot, CL+= A->qtot,
    HL += A->qtot, A=A->next)
      A->GradT_h(HL,AL,BL,CL,'a',invW);
  else if(AL && BL)
    for(A=fhead;A;AL+=A->qtot, BL+=A->qtot, HL += A->qtot, A=A->next)
      A->GradT_h(HL,AL,BL,nul,'a',invW);
  else if(AL)
    for(A=fhead;A;AL+=A->qtot, HL += A->qtot,A=A->next)
      A->GradT_h(HL,AL,nul,nul,'x',invW);
  else if(BL)
    for(A=fhead;A;BL+=A->qtot, HL += A->qtot,A=A->next)
      A->GradT_h(HL,nul,BL,nul,'y',invW);
  else if(CL)
    for(A=fhead;A;CL+=A->qtot, HL += A->qtot,A=A->next)
      A->GradT_h(HL,nul,nul,CL,'z',invW);
}


void Element_List::HelmHoltz(Metric *lambda){
  Eloop(fhead)
    el->HelmHoltz(lambda+el->id);
}

void Element_List::Set_field(char *string){
  Eloop(fhead)
    el->Set_field(string);
}

void Element_List::Set_state(char st){
  set_state(fhead, st);
}

void Element_List::zerofield(){

  dzero(htot, base_h, 1);
  dzero(hjtot, base_hj, 1);
}

Element_List* Element_List::gen_aux_field(char ty){
  int           eid, id, i,k;
  Edge         *edg;
  Element_List *EL = (Element_List*) new Element_List();
  Element**  new_E = (Element**) malloc(nel*sizeof(Element*));
#if 1
  for(i=0;i<nel;++i){
    switch(flist[i]->identify()){
    case Nek_Tri:
      new_E[i] =  new Tri(flist[i]);
      break;
    case Nek_Quad:
      new_E[i] =  new Quad(flist[i]);
      break;
    case Nek_Tet:
      new_E[i] =  new Tet(flist[i]);
      break;
    case Nek_Hex:
      new_E[i] =  new Hex(flist[i]);
      break;
    case Nek_Prism:
      new_E[i] =  new Prism(flist[i]);
      break;
    case Nek_Pyr:
      new_E[i] =  new Pyr(flist[i]);
      break;
    case Nek_Element:
      fprintf(stderr,"Element_List::gen_aux_field copying from Element\n");
      break;
    }
    new_E[i]->type = ty;
  }
#else
  Element *temp = (Element*) calloc(nel, sizeof(Element));
  for(i=0;i<nel;++i){
    new_E[i] = temp+i;
    memcpy(new_E[i], flist[i], sizeof(Element));

    new_E[i]->vert = (Vert *)calloc(flist[i]->Nverts,sizeof(Vert));
    new_E[i]->edge = (Edge *)calloc(flist[i]->Nedges,sizeof(Edge));
    new_E[i]->face = (Face *)calloc(flist[i]->Nfaces,sizeof(Face));

    memcpy( new_E[i]->vert,flist[i]->vert,flist[i]->Nverts*sizeof(Vert));
    memcpy( new_E[i]->edge,flist[i]->edge,flist[i]->Nedges*sizeof(Edge));
    memcpy( new_E[i]->face,flist[i]->face,flist[i]->Nfaces*sizeof(Face));
  }
#endif
  for(i=0;i<nel-1;++i)
    new_E[i]->next = new_E[i+1];
  new_E[i]->next = (Element*)NULL;

  /* set up edge and face links */
  /* first zero each base or link structure */
  for(k = 0; k < nel ; ++k){
    for(i = 0; i < new_E[k]->Nedges; ++i)
      new_E[k]->edge[i].base = (Edge *)NULL;

    for(i = 0; i < new_E[k]->Nfaces; ++i)
      new_E[k]->face[i].link = (Face *)NULL;
  }

  for(k = 0; k < nel; ++k){
#ifdef COMPRESS
    if(new_E[0]->dim() == 2)
#endif
    for(i = 0; i < new_E[k]->Nedges; ++i)
      if((!new_E[k]->edge[i].base)&&flist[k]->edge[i].base){
  eid = flist[k]->edge[i].base->eid;
  id  = flist[k]->edge[i].base->id;
  for(edg=flist[k]->edge[i].base; edg->link; edg = edg->link){
    new_E[edg->eid]->edge[edg->id].base = new_E[eid]->edge+id;
    new_E[edg->eid]->edge[edg->id].link =
      new_E[edg->link->eid]->edge+edg->link->id;
  }
  new_E[edg->eid]->edge[edg->id].base = new_E[eid]->edge+id;
      }
    for(i = 0; i < new_E[k]->Nfaces; ++i)
      if(!new_E[k]->face[i].link&&flist[k]->face[i].link){
  if(flist[k]->face[i].link->link){
    eid = flist[k]->face[i].link->eid;
    id  = flist[k]->face[i].link->id;
    new_E[k  ]->face[i ].link = new_E[eid]->face+id;
    new_E[eid]->face[id].link = new_E[k]->face+i;
  }
  else{ // if link back does not exist then treat as dummy face
    new_E[k  ]->face[i ].link = (Face *) calloc(1,sizeof(Face));
    memcpy(new_E[k]->face[i].link,flist[k]->face[i].link,sizeof(Face));
  }
      }
  }

  EL->flist = new_E;
  EL->fhead = *new_E;
  EL->nel   = nel;
  EL->fhead->type = ty;

  EL->nz      = nz;
  EL->nztot   = nztot;
  EL->flevels = (Element_List**) NULL;

  if(option("NZ")==1)
    EL->Cat_mem();

  return EL;
}

void Element_List::Terror(char *string){

  int   qt,trip=0;
  double li=0.0,l2=0.0,h1=0.0,areat = 0.0,l2a,h1a=0.0,area,store;
  double *s,*utmp;
  Coord *X = (Coord *)malloc(sizeof(Coord));
  Element *E;

  if(fhead->dim() == 2){
    if(string)
       vector_def("x y",string);

    X->x = dvector(0,QGmax*QGmax-1);
    X->y = dvector(0,QGmax*QGmax-1);
    utmp = dvector(0,QGmax*QGmax-1);

    for(E=fhead;E;E = E->next){
      qt = E->qtot;
      s  = E->h[0];

      if(E->state == 't'){ E->Trans(E,J_to_Q); trip = 1;}
      dcopy(qt,s,1,utmp,1);

      if(string){
  E->coord(X);
#ifdef MAP
  /* ce107 changes begin */
  double orig;
  if ((orig = dparam("ORIGINX")) != 0.0)
    dsadd(qt, orig, X->x, 1, X->x, 1);
  if ((orig = dparam("ORIGINY")) != 0.0)
    dsadd(qt, orig, X->y, 1, X->y, 1);
#endif
  /* ce107 changes end */
  vector_set(qt,X->x,X->y,s);
  dvsub(qt,s,1,utmp,1,s,1);
      }

      li = ((store=E->Norm_li())>li)? store:li;
      E->Norm_l2m(&l2a,&area);
      E->Norm_h1m(&h1a,&area);

      l2    += l2a;
      h1    += h1a;
      areat += area;

      if(trip){ E->state = 't'; trip = 0;}
      else     dcopy(qt,utmp,1,s,1);
    }
  }
  else{
    if(string)
      vector_def("x y z",string);

    X->x = dvector(0,QGmax*QGmax*QGmax-1);
    X->y = dvector(0,QGmax*QGmax*QGmax-1);
    X->z = dvector(0,QGmax*QGmax*QGmax-1);
    utmp = dvector(0,QGmax*QGmax*QGmax-1);

    for(E=fhead;E;E = E->next){
      qt = E->qtot;
      s  = E->h_3d[0][0];

      if(E->state == 't'){ E->Trans(E,J_to_Q); trip = 1;}
      dcopy(qt,s,1,utmp,1);

      if(string){
  E->coord(X);
  vector_set(qt,X->x,X->y,X->z,s);
  dvsub(qt,s,1,utmp,1,s,1);
      }

      li = ((store=E->Norm_li())>li)? store:li;
      E->Norm_l2m(&l2a,&area);
      E->Norm_h1m(&h1a,&area);

      l2    += l2a;
      h1    += h1a;
      areat += area;

      if(trip){ E->state = 't'; trip = 0;}
      else     dcopy(qt,utmp,1,s,1);
    }
    free(X->z);
  }


  ROOTONLY
    fprintf(stdout, "Field: %c ", fhead->type);

  DO_PARALLEL{
    gdmax(&li,1,&l2a);
    gdsum(&l2,1,&l2a);
    gdsum(&h1,1,&h1a);
    gdsum(&areat,1,&area);
  }

  ROOTONLY
    fprintf(stdout,"%12.6lg %12.6lg %12.6lg (Linf  L2  H1)\n",
      li,sqrt(l2/areat),sqrt(h1/areat));

  free(X->x); free(X->y); free(utmp); free((char *)X);
}

// not set up for 3d
void DirBCs(Element_List *U, Bndry *Ubc, Bsystem *Ubsys, SolveType Stype){
  /* set up static product of boundary dirichlet points for RHS */
  /* don't want to do for iterative or pressure solve */
  if(Ubsys->smeth == direct){
    register int k;
    int      nbl,N;
    double  *tmp   = dvector(0,Ubsys->nglobal-1);
    double **binvc = Ubsys->Gmat->binvc;
    Bndry   *Ebc;
    //    Element *E;

    dzero(Ubsys->nglobal, tmp, 1);

    if(!Ubc||!Ubsys->nsolve) return;

    Ubc->DirRHS = dvector(0,Ubsys->nsolve-1);
    dzero(Ubsys->nsolve, Ubc->DirRHS, 1);

    for(Ebc = Ubc; Ebc; Ebc = Ebc->next) Ebc->DirRHS = Ubc->DirRHS;

    for(k = 0; k < Ubsys->nel; ++k)
      dzero(U->flist[k]->Nmodes, U->flist[k]->vert->hj, 1);

    SetBCs(U,Ubc,Ubsys);

    if(Stype == Helm){
      U->Trans(U, J_to_Q);
      U->HelmHoltz(Ubsys->lambda);
    }
    else{
      U->Trans(U, J_to_Q);
      U->Iprod(U);
    }

    for(k = 0; k < Ubsys->nel; ++k){
      nbl = U->flist[k]->Nbmodes;
      N   = U->flist[k]->Nmodes - nbl;
      if(N) dgemv('T',N, nbl, -1., binvc[U->flist[k]->geom->id], N,
      U->flist[k]->vert->hj+nbl, 1, 1.0, U->flist[k]->vert->hj,1);
    }

    // SignChange(U);
    GathrBndry(U,tmp,Ubsys);
    dcopy(Ubsys->nsolve,tmp,1,Ubc->DirRHS,1);
    free(tmp);

    /* zero U so don't start with funny values */
    U->zerofield();
  }
}

void DirBCs_Stokes(Element_List **V, Bndry **Vbc, Bsystem **Vbsys){
  /* set up static product of boundary dirichlet points for RHS */
  /* don't want to do for iterative or time dependent solves  */
  if(Vbsys[0]->smeth == direct){
    register int i,k;
    int      nbl,N,info;
    int      eDIM  = V[0]->fhead->dim();
    Bsystem   *B = Vbsys[0], *PB = Vbsys[eDIM];
    int      nel = B->nel;
    double  *tmp;
    int      **ipiv    = B->Gmat->cipiv;
    double   **binvc   = B->Gmat->binvc;
    double   **invc    = B->Gmat->invc;
    double   ***dbinvc = B->Gmat->dbinvc;
    double   **p_binvc  = PB->Gmat->binvc;
    Element  *E, *E1;

    if(!PB||!PB->nsolve) return;

    tmp = dvector(0,PB->nglobal-1);
    dzero(PB->nglobal, tmp, 1);

    Vbc[0]->DirRHS = dvector(0,PB->nsolve-1);
    dzero(PB->nsolve, Vbc[0]->DirRHS, 1);

    for(i = 0; i < eDIM; ++i){
      dzero(V[i]->hjtot,V[i]->base_hj,1);
      SetBCs(V[i],Vbc[i],Vbsys[i]);

      /* inner product of divergence for pressure forcing */
      V[i]->Trans(V[i], J_to_Q);
    }

    V[0]->Grad(V[eDIM],0,0,'x');
    V[1]->Grad(0,V[0],0,'y');
    dvadd(V[1]->htot,V[eDIM]->base_h,1,V[0]->base_h,1,V[eDIM]->base_h,1);

    if(eDIM == 3){
      V[2]->Grad(0,V[0],0,'z');
      dvadd(V[2]->htot,V[eDIM]->base_h,1,V[0]->base_h,1,V[eDIM]->base_h,1);
    }
    dneg(V[eDIM]->htot,V[eDIM]->base_h,1);

#ifndef PCONTBASE
    for(k = 0; k < nel; ++k)
      V[eDIM]->flist[k]->Ofwd(*V[eDIM]->flist[k]->h,
            V[eDIM]->flist[k]->vert->hj,
            V[eDIM]->flist[k]->dgL);
#else
    V[eDIM]->Iprod(V[eDIM]);
#endif
    /* put boundary solution back into V[0] */
    V[0]->Trans(V[0], J_to_Q);

    for(i = 0; i < eDIM; ++i){
      for(k = 0; k < nel; ++k){
  E   = V[i]->flist[k];
  nbl = E->Nbmodes;
  N   = E->Nmodes - nbl;

  E->HelmHoltz(PB->lambda+k);

  dscal(E->Nmodes, B->lambda[k].d, E->vert->hj, 1);

  if(N) {
    /* condense out interior terms in velocity */
    dgemv('T', N, nbl, -1., binvc[E->geom->id], N,
    E->vert->hj+nbl, 1, 1., E->vert->hj,1);

    /* condense out interior terms in pressure*/
    E1 = V[eDIM]->flist[k];
    if(B->lambda->wave){
      dcopy(N,E->vert->hj+nbl,1,tmp,1);
      dgetrs('N',N,1,invc[E->geom->id],N,ipiv[E->geom->id],tmp,N,info);
      dgemv('T', N, E1->Nmodes, -1., dbinvc[i][E->geom->id], N,
      tmp, 1, 1., E1->vert->hj,1);
    }
    else{
      dgemv('T', N, E1->Nmodes, -1., dbinvc[i][E->geom->id], N,
      E->vert->hj+nbl, 1, 1., E1->vert->hj,1);
    }
  }
      }
    }

    /* second level of factorisation to orthogonalise basis to  p */
    /* p - vel */
    for(E=V[eDIM]->fhead;E;E=E->next){
      E1 = V[0]->flist[E->id];

      nbl = eDIM*E1->Nbmodes + 1;
      N   = E->Nmodes-1;

      dgemv('T', N, nbl, -1.0, p_binvc[E->geom->id], N,
      E->vert->hj+1, 1, 0.0, tmp,1);

      for(i = 0; i < eDIM; ++i){
  E1 = V[i]->flist[E->id];
  dvadd(E1->Nbmodes,tmp+i*E1->Nbmodes,1,E1->vert->hj,1,E1->vert->hj,1);
      }

      E->vert->hj[0] += tmp[nbl-1];
    }

    GathrBndry_Stokes(V,tmp,Vbsys);
    dcopy(PB->nsolve,tmp,1,Vbc[0]->DirRHS,1);
    free(tmp);

    /* zero V so don't start with funny values */
    for(i = 0 ; i <= eDIM; ++i)
      V[i]->zerofield();
  }
}

void Element_List::FFT(Element_List *, Nek_Trans_Type ){
  fprintf(stderr, "Element_List::FFT  -- not compiled for fourier modes\n");
}

void Element_List::Grad_z(Element_List *){
  fprintf(stderr, "Element_List::Grad_z  -- not compiled for fourier modes\n");
}

void Element_List::Mem_shift(double *new_h, double *new_hj){
  base_h  = new_h;
  base_hj = new_hj;

  Eloop(fhead){
    el->Mem_shift(new_h, new_hj, 't');
    new_h  += el->qtot;
    new_hj += el->Nmodes;
  }
}

int check_connect(Element_List *EL, int element1, int element2);

void Element_List::H_Refine(int * to_split, int nfields, Bndry **Ubc, int *flag)
{
  // if flag[0] == 1, then we do a pure split (ie triangles -> triangles)
  // otherwise, we allow hybrid splitting.

  int i;
  int NVerts;
  int *link_eid;

  int * edges_to_split;
  int orignel = nel;
  int elements_to_split = 0;
  int * unconnected;
  Element * E;

  for(i = 0; i < orignel; i++)
    if(to_split[i])
      elements_to_split++;



  //this array is used to accelerate the time to reconnect the new mesh
  int num_in_unconnected = orignel + 4*elements_to_split;
  unconnected = new int[num_in_unconnected];
  izero(num_in_unconnected, unconnected, 1);


  for(i=0; i < orignel; i++)
    {
      if(to_split[i])
  {
    E = flist[i];
    NVerts = E->Nverts;
    link_eid = ivector(0,NVerts-1);
    edges_to_split = new int[NVerts+1];
    edges_to_split[0] = 0;
    for(int k=0; k < NVerts; k++)
      {
        if(E->edge[k].base)
    if(E->edge[k].link){
      link_eid[k] = E->edge[k].link->eid;
    }
    else{
      link_eid[k] = E->edge[k].base->eid;
    }
        else{
    link_eid[k] = -1;
        }//end if

        if(link_eid[k] == -1) {
          edges_to_split[0]++;
    edges_to_split[edges_to_split[0]] = k;
        }
        else if(to_split[link_eid[k]])
    {
      edges_to_split[0]++;
      edges_to_split[edges_to_split[0]] = k;
    }//end if
      }//end for

    if(flag[0]==1)
      E->close_split(this, Ubc, nfields, edges_to_split);
    else
      E->split_element(this, Ubc, nfields, edges_to_split);

    //upon returning, edges_to_split contains the number of unconnected element and their element ids
    for(int j = 0; j < (edges_to_split[0]*2); j+=2)
        unconnected[edges_to_split[j+1]] = edges_to_split[j+2];

    free(link_eid);
    delete [] edges_to_split;
  }//endif
    }//end for

  for(i = 0; i < num_in_unconnected; i++)
    {
      if(unconnected[i]>0)
  {
    for(int j = i+1; j < num_in_unconnected; j++)
      {
        if(unconnected[j]>0)
    if(check_connect(this, i, j))
         {
           unconnected[i]--;
           unconnected[j]--;
         }//end check_connect if

        }//end of j for loop
    }//end of if
      }//end of for i

  delete[] unconnected;
  return;
} //end H_Refine member function

int check_connect(Element_List *EL, int element1, int element2)
{
   Element *E1 = EL->flist[element1];
   Element *E2 = EL->flist[element2];

   double ax, ay;
   double bx, by;
   for(int i=0; i < E1->Nedges; ++i)
     {
       if(E1->edge[i].base == NULL)
   {
     ax = (E1->vert[i].x+E1->vert[(i+1)%E1->Nverts].x);
     ay = (E1->vert[i].y+E1->vert[(i+1)%E1->Nverts].y);
     for(int j=0; j < E2->Nedges; ++j)
       {
         bx = (E2->vert[j].x+E2->vert[(j+1)%E2->Nverts].x);
         by = (E2->vert[j].y+E2->vert[(j+1)%E2->Nverts].y);
         if(((ax-bx)*(ax-bx) + (ay-by)*(ay-by)) < 1e-14)
     {
       E1->edge[i].base = E1->edge+i;
       E1->edge[i].link = E2->edge+j;
       E2->edge[j].base = E1->edge+i;
       E2->edge[j].link = NULL;
       return (1);
     }//end if
       }//end for j
   }//end if
     }//end for i
    return(0);
}//end check_connect function

ParaInfo pllinfo[51];
ParaInfo pBCinfo[51];

void set_pllinfo(Element_List *EL, char *name);
void set_pBCinfo(Element_List *EL, Bndry *Ubc, char *name);
void set_Pllinfo(Element_List *EL, Bndry *Ubc, char *name);


Element_List *Part_elmt_list(Element_List *EL, char trip);

/* given an element E this routine returns a sorted element list
   with a reduced number of elements in the parallel version */

Element_List *LocalMesh(Element_List *EL, char *name){
  Element_List *new_EL;

  set_pllinfo(EL,name);


  if(option("OLDHYBRID"))
  {
      //fprintf(stderr, "LocalMesh: calling: set_nfacet_list(EL)\n");
    set_nfacet_list(EL);
    //fprintf(stderr, "LocalMesh: Done calling: set_nfacet_list(EL)\n");
  }

  //fprintf(stderr, "LocalMesh: calling: Part_elmt_list\n");
  new_EL = Part_elmt_list(EL, 'b');
  //fprintf(stderr, "LocalMesh: Done calling: Part_elmt_list\n");


  return new_EL;
}



Element_List *LocalMesh(Element_List *EL, Bndry *Ubc, char *name){
  Element_List *new_EL;


  set_Pllinfo(EL,Ubc,name);

  if(option("OLDHYBRID"))
    set_nfacet_list(EL);

  new_EL = Part_elmt_list(EL, 'b');

  return new_EL;
}



void set_con_info(Element_List *U, int nel, int *partition){
  register int i,j,k,n,cnt;
  int *elmtid, *edgeid, *edgegid;
  int eid, edid, nedges, *lelmtid, *ledgeid;
  Element *E;
  int active_handle = get_active_handle();

  ConInfo *cinfo,*c;

  elmtid  = ivector(0,Max_Nedges*pllinfo[active_handle].nloop-1);
  edgeid  = ivector(0,Max_Nedges*pllinfo[active_handle].nloop-1);
  edgegid = ivector(0,Max_Nedges*pllinfo[active_handle].nloop-1);

  for(n = 0; n < pllinfo[active_handle].nprocs; ++n){
    if(n == pllinfo[active_handle].procid) continue;

    ifill(Max_Nedges*pllinfo[active_handle].nloop,-1,elmtid,1);
    ifill(Max_Nedges*pllinfo[active_handle].nloop,-1,edgeid,1);
    ifill(Max_Nedges*pllinfo[active_handle].nloop,-1,edgegid,1);

    /* gather together information about edges on this process
       connecting with processor n */
    if(U->fhead->dim() == 2){
      for(i = 0,nedges=0; i < pllinfo[active_handle].nloop; ++i){
  E = U->flist[pllinfo[active_handle].eloop[i]];
  for(j = 0; j < E->Nedges; ++j)
    if(E->edge[j].base){
      if(E->edge[j].link){
        if(partition[E->edge[j].link->eid] == n){
    elmtid [nedges] = i;
    edgeid [nedges] = j;
    edgegid[nedges] = E->edge[j].gid;
    nedges++;
        }
      }
      else{
        if(partition[E->edge[j].base->eid] == n){
    elmtid [nedges] = i;
    edgeid [nedges] = j;
    edgegid[nedges] = E->edge[j].gid;
    nedges++;
        }
      }
    }
      }
    }
    else{
      for(i = 0,nedges=0; i < pllinfo[active_handle].nloop; ++i){
  E = U->flist[pllinfo[active_handle].eloop[i]];
  for(j = 0; j < E->Nfaces; ++j)
    if(E->face[j].link){
      if(partition[E->face[j].link->eid] == n){
        elmtid [nedges] = i;
        edgeid [nedges] = j;
        edgegid[nedges] = E->face[j].gid;
        nedges++;
      }
    }
      }
    }

    if(nedges){
      ++pllinfo[active_handle].ncprocs;
      cinfo         = (ConInfo *)calloc(1,sizeof(ConInfo));
      cinfo->next   = pllinfo[active_handle].cinfo;
      pllinfo[active_handle].cinfo = cinfo;

      /* set up cinfo structures */
      cinfo->nedges = nedges;
      cinfo->cprocid = n;
      cinfo->elmtid = lelmtid = ivector(0,nedges-1);
      cinfo->edgeid = ledgeid = ivector(0,nedges-1);

      /* to ensure that the global list of data is in the same order
   between two processors we should sort data according to edge
   gid's since this preserves the same ordering as the edge gid's
   of the global mesh. */
      icopy(nedges,elmtid,1,lelmtid,1);
      icopy(nedges,edgeid,1,ledgeid,1);

      /* reorder elmtid and edgeid according to the value of edgegid */
      for(j = 0; j < nedges; ++j){
  cnt = j;
  for(k = j+1; k < nedges; ++k)
    cnt = (edgegid[k] < edgegid[cnt])? k: cnt;
  eid  = lelmtid[cnt];
  edid = ledgeid[cnt];

  /*
    icopy(cnt-j,edgegid +j,1,edgegid +j+1,1);
    icopy(cnt-j,lelmtid+j,1,lelmtid+j+1,1);
    icopy(cnt-j,ledgeid+j,1,ledgeid+j+1,1);
  */
        for(k=cnt-j-1;k>=0;--k){
          edgegid[k+j+1] = edgegid[k+j];
          lelmtid[k+j+1] = lelmtid[k+j];
          ledgeid[k+j+1] = ledgeid[k+j];

        }

  lelmtid[j] = eid;
  ledgeid[j] = edid;
      }
    }
  }

  /*finally make cinfo list contiguous */
  cinfo = (ConInfo *)malloc(pllinfo[active_handle].ncprocs*sizeof(ConInfo));
  for(cnt=0,c = pllinfo[active_handle].cinfo; c; c = c->next, ++cnt)
    memcpy(cinfo + cnt, c,sizeof(ConInfo));

  c = pllinfo[active_handle].cinfo;
  pllinfo[active_handle].cinfo = cinfo;

  for(cnt=0; cnt < pllinfo[active_handle].ncprocs-1; ++cnt)
    pllinfo[active_handle].cinfo[cnt].next = pllinfo[active_handle].cinfo + cnt+1;

  /* free up link list */
  while(c){
    cinfo = c;
    c = cinfo->next;
    free(cinfo);
  }

  free(edgegid);
  free(elmtid);
  free(edgeid);
}

void set_pllinfo(Element_List *EL, char *name){
  register int i,j;
  int      id,eid,nel = EL->nel;
  char     fname[FILENAME_MAX];
  FILE     *fp;
  int active_handle = get_active_handle();

  //fprintf(stderr, "Entering: set_pllinfo\n");

  pllinfo[active_handle].nprocs = numnodes();
  pllinfo[active_handle].procid = mynode();
  pllinfo[active_handle].gnel   = nel;

  /* Domain decomposition input file   */
  DO_PARALLEL{
    int *partition      = ivector(0, nel);

    sprintf(fname, "%s.part.%d",name,pllinfo[active_handle].nprocs);
    //gsync();
    //fprintf(stderr, "set_pllinfo: DO_PARALLEL: fname: %s\n", fname);

    if ((fp = fopen(fname,"r")) == (FILE *) NULL)
    {
  //gsync();
  //fprintf(stderr, "set_pllinfo: calling default_partitioner()\n");
      default_partitioner(EL,partition);
      //gsync();
      //fprintf(stderr, "set_pllinfo: Done calling default_partitioner()\n");
    }
    else {
      int part;

      ROOTONLY
  fprintf(stderr,"Partitioner         : Using file %s\n",fname);

      for (i = 0; i < nel; i++)
  fscanf(fp,"%d",partition+i);
    }

#ifdef PARALLEL
    //gsync();
    //fprintf(stderr, "set_pllinfo: call pllinfo.AdjacentPartitions\n");
    pllinfo[active_handle].AdjacentPartitions = test_partition_connectivity(EL,partition,&i);
    pllinfo[active_handle].NAdjacentPartitions = i;
#endif

    /* extract data on current partition */
    //gsync();
    //fprintf(stderr, "set_pllinfo: before First loop\n");

    pllinfo[active_handle].nloop = 0;
    for(i = 0; i < nel; ++i)
      if(partition[i] == pllinfo[active_handle].procid) pllinfo[active_handle].nloop++;

    //gsync();
    //fprintf(stderr, "set_pllinfo: before pllinfo.eloop = ivector(0,pllinfo.nloop-1)\n");
    pllinfo[active_handle].eloop = ivector(0,pllinfo[active_handle].nloop-1);

    //gsync();
    //fprintf(stderr, "set_pllinfo: before Second loop\n");
    int cnt = 0;
    for(i = 0; i < nel; ++i)
      if(partition[i] == pllinfo[active_handle].procid) pllinfo[active_handle].eloop[cnt++] = i;

    /* sort element list so that the numbers of global elements are
       consequative with lowest global id first */
    /* could probably use a better sort */;

    //gsync();
    //fprintf(stderr, "set_pllinfo: before Third (sort) loop\n");

    for(i = 0; i < pllinfo[active_handle].nloop; ++i){
      for(id = j = i; j < pllinfo[active_handle].nloop; ++j)
  id = (pllinfo[active_handle].eloop[j] < pllinfo[active_handle].eloop[id])? j:id;
      eid = pllinfo[active_handle].eloop[id];
      for(j = id; j > i; --j)
  pllinfo[active_handle].eloop[j] = pllinfo[active_handle].eloop[j-1];
      pllinfo[active_handle].eloop[i] = eid;
    }

    //gsync();
    //fprintf(stderr, "set_pllinfo: before ivectors\n");

    //  pllinfo.etypes  = ivector(0, nel-1);
    pllinfo[active_handle].efacets = ivector(0, nel-1);
    pllinfo[active_handle].eedges  = ivector(0, nel-1);
    pllinfo[active_handle].efaces  = ivector(0, nel-1);

    //gsync();
    //fprintf(stderr, "set_pllinfo: before Fourth (edge/face) loop\n");

    for(i=0;i<nel;++i){
      //    pllinfo.etypes[i]  = (int) EL->flist[i]->identify();
      pllinfo[active_handle].eedges[i] = EL->flist[i]->Nedges;
      pllinfo[active_handle].efaces[i] = EL->flist[i]->Nfaces;

      pllinfo[active_handle].efacets[i] = EL->flist[i]->Nedges + EL->flist[i]->Nfaces;
      if(EL->flist[i]->dim() == 3) ++pllinfo[active_handle].efacets[i]; /* interior */
    }

    // cumfacets[i] gives the offset from zero of the first
    // facet of element i
    pllinfo[active_handle].cumfacets = ivector(0, nel);
    pllinfo[active_handle].cumfacets[0] = 0;
    for(i=1;i<nel+1;++i)
      pllinfo[active_handle].cumfacets[i] = pllinfo[active_handle].cumfacets[i-1]
                                                 +pllinfo[active_handle].efacets[i-1];

    //gsync();
    //fprintf(stderr, "set_pllinfo: before set_con_info\n");

    set_con_info(EL,EL->nel,partition);

    //gsync();
    //fprintf(stderr, "set_pllinfo: after set_con_info\n");

    // reset vertex solve mask to have a value of 2 if it is on a partition
    // this is necessary for the vertex block precondiitioning required by
    // LE precon.

    if(EL->fhead->dim() == 3){
      Element *E;
      for(E = EL->fhead; E; E = E->next)
  for(i = 0; i < E->Nfaces; ++i)
    if(E->face[i].link)
      if(partition[i] != partition[E->face[i].link->eid])
        for(j = 0; j < E->Nfverts(i); ++j)
    E->vert[E->vnum(i,j)].solve *= 2;
    }

    pllinfo[active_handle].partition = partition; // save for later usage (Particulary LE).
  }
  else{
    /* set up standard ordering  */
    pllinfo[active_handle].nloop = nel;
    pllinfo[active_handle].eloop = ivector(0,nel-1);
    for(i=0; i < nel; ++i)
      pllinfo[active_handle].eloop[i] = i;
  }

  //fprintf(stderr, "Exiting: set_pllinfo\n");

}

// new function for sectional forces to run in parallel

void set_pBCinfo(Element_List *EL,Bndry *Ubc, char *name){
  register int i,j;
  Bndry *Bc;
  int eid;
  int n_elem=0;
  int nel = EL->nel;
  int active_handle = get_active_handle();
  char     fname[FILENAME_MAX];
  FILE     *fp;

  //  fprintf(stderr, "Entering: set_pBCinfo\n");

  for (Bc=Ubc;Bc;Bc=Bc->next){
    if (Bc->type == 'W'){
      n_elem++;
    }
  }

  pBCinfo[active_handle].nprocs = numnodes();
  pBCinfo[active_handle].procid = mynode();
  pBCinfo[active_handle].gnel   = n_elem;

  DO_PARALLEL{
      int *part=ivector(0,nel-1);

      ifill(nel,-1,part,1);

      for(i=0; i<pllinfo[active_handle].nloop;i++){
        part[pllinfo[active_handle].eloop[i]]=-2;
      }

      int id=0;
      pBCinfo[active_handle].nloop=0;

        for (Bc=Ubc;Bc;Bc=Bc->next){
          if (Bc->type =='W'){

           eid=Bc->elmt->id;

           if (part[eid]+1){
             part[eid]=id;
             pBCinfo[active_handle].nloop++;

          }


           id++;
          }
        }


    //     if (pBCinfo[get_active_handle()].nloop==0)
    //   return;

    id=0;
    pBCinfo[active_handle].eloop = ivector(0, pBCinfo[active_handle].nloop);

    for(i = 0; i < nel; ++i){
       if(part[i]>-1)
       pBCinfo[active_handle].eloop[id++]=part[i];
    }

  }
  else{
    /* set up standard ordering  */
    pBCinfo[active_handle].nloop = n_elem;
    pBCinfo[active_handle].eloop = ivector(0,n_elem-1);
    for(i=0; i < n_elem; ++i)
      pBCinfo[active_handle].eloop[i] = i;
  }
}

void set_Pllinfo(Element_List *EL, Bndry *Ubc, char *name){
     set_pllinfo(EL,name);
     set_pBCinfo(EL,Ubc,name);
}


static void  add_pf(int elmt, int side);

Element_List *Part_elmt_list(Element_List *GEL, char trip){
  int           i,k;
  int           lnel = pllinfo[get_active_handle()].nloop, ig;
  int active_handle = get_active_handle();

  Element**  new_E = (Element**) malloc(lnel*sizeof(Element*));
  Element *Eg, *E;

  //  fprintf(stderr, "Entering: Part_elmt_list\n");

  for(i=0;i<lnel;++i){
    ig = pllinfo[active_handle].eloop[i];
    Eg = GEL->flist[ig];

    switch(Eg->identify()){
    case Nek_Tri:
      new_E[i] =  new Tri(Eg);
      break;
    case Nek_Quad:
      new_E[i] =  new Quad(Eg);
      break;
    case Nek_Tet:
      new_E[i] =  new Tet(Eg);
      break;
    case Nek_Hex:
      new_E[i] =  new Hex(Eg);
      break;
    case Nek_Prism:
      new_E[i] =  new Prism(Eg);
      break;
    case Nek_Pyr:
      new_E[i] =  new Pyr(Eg);
      break;
    case Nek_Element:
      fprintf(stderr,"Element_List::gen_aux_field copying from Element\n");
      break;
    }
    new_E[i]->type = Eg->type;

    if(Eg->curvX){
      new_E[i]->curvX = (Cmodes*) calloc(Eg->dim(), sizeof(Cmodes));
      memcpy(new_E[i]->curvX, Eg->curvX, Eg->dim()*sizeof(Cmodes));
    }
    else
      new_E[i]->curvX = NULL;

    if(Eg->curve){
      new_E[i]->curve = (Curve*) calloc(1, sizeof(Curve));
      memcpy(new_E[i]->curve, Eg->curve, sizeof(Curve));
    }
    else
      new_E[i]->curve = NULL;
  }

  for(i=0;i<lnel-1;++i)
    new_E[i]->next = new_E[i+1];
  new_E[i]->next = (Element*)NULL;

  for(k = 0; k < lnel; ++k){
    E = new_E[k];
    E->id   = k;
    for(i = 0; i < E->Nedges; ++i) E->edge[i].eid = k;
    for(i = 0; i < E->Nfaces; ++i) E->face[i].eid = k;
  }

  Vert *vl, *vb;
  Edge *edg, *link, *base;
  Face *face;

  /* Need to reset link list in edge and face structures so that
     they only point to elements in the local region                */
  /* needed for recursive solver */

  int *inc = ivector(0,GEL->nel-1);

  ifill(GEL->nel,-1,inc,1);
  for(i = 0; i < lnel; ++i)
    inc[pllinfo[active_handle].eloop[i]] = i;

  /* At present all edges and links point to U structure */
  if(GEL->fhead->dim() == 2){
    for(k = 0; k < lnel ; ++k){

      for(i = 0; i < new_E[k]->Nverts; ++i){
  if(vb = GEL->flist[pllinfo[active_handle].eloop[k]]->vert[i].base){
    /* find first edge in list which is in
       local region and set it to base */
    while(vb){
      if(inc[vb->eid]+1){
        new_E[k]->vert[i].base = new_E[inc[vb->eid]]->vert +
    vb->id;
        break;
      }
      else
        vb = vb->link;
    }

    /* point link to the link list of GEL structure */
    vl = vb->link;

    /* reset base to point to local beginning of real list */
    vb = new_E[k]->vert[i].base;

    while(vl){
      /* find next linked edge which is in patch */
      while(vl&&!(inc[vl->eid]+1))
        vl = vl->link;

      if(vl){
        /* reset base to point to new link */
        vb->link = new_E[inc[vl->eid]]->vert + vl->id;
        vb = vb->link;

        vl = vl->link;
      }
      else
        vb->link = (Vert*) NULL;
    }
  }
      }

      for(i = 0; i < new_E[k]->Nedges; ++i){
  if((edg = new_E[k]->edge + i)->base){
    if(edg->link)
      if(inc[edg->link->eid]+1){
        new_E[k]->edge[i].base = new_E[k]->edge + i;
        new_E[k]->edge[i].link = new_E[inc[edg->link->eid]]->edge +
    edg->link->id;
      }
      else{
        new_E[k]->edge[i].base = (Edge *)NULL;
        /* normally the link is not defined if base not defined
     so this identifies this edge as a part of the global
     solution domain                                      */
        new_E[k]->edge[i].link = new_E[k]->edge + i;
#ifndef COMPRESS
        /* new methods is to add list to pllinfo - old info kept
     until recurrSC.c completely removed */
        add_pf(k,i);
#endif
      }
    else
      if(inc[edg->base->eid]+1){
        new_E[k]->edge[i].base = new_E[inc[edg->base->eid]]->edge +
    edg->base->id;
        new_E[k]->edge[i].link = (Edge *)NULL;
      }
      else{
        new_E[k]->edge[i].base = (Edge *)NULL;
        new_E[k]->edge[i].link = new_E[k]->edge + i;
#ifndef COMPRESS
        /* new methods is to add list to pllinfo - old info kept
     until recurrSC.c completely removed */
        add_pf(k,i);
#endif
      }
  }
      }
    }
  }
  else{
    for(k = 0; k < lnel ; ++k){
      for(i = 0; i < new_E[k]->Nverts; ++i){
  if(vb = GEL->flist[pllinfo[active_handle].eloop[k]]->vert[i].base){
    /* find first edge in list which is in
       local region and set it to base */
    while(vb){
      if(inc[vb->eid]+1){
        new_E[k]->vert[i].base = new_E[inc[vb->eid]]->vert +
    vb->id;
        break;
      }
      else
        vb = vb->link;
    }

    /* point link to the link list of GEL structure */
    vl = vb->link;

    /* reset base to point to local beginning of real list */
    vb = new_E[k]->vert[i].base;

    while(vl){
      /* find next linked edge which is in patch */
      while(vl&&!(inc[vl->eid]+1))
        vl = vl->link;

      if(vl){
        /* reset base to point to new link */ // possible mistake
        vb->link = new_E[inc[vl->eid]]->vert + vl->id;
        vb = vb->link;

        vl = vl->link;
      }
      else
        vb->link = (Vert*) NULL;
    }
  }
      }

      for(i = 0; i < new_E[k]->Nedges; ++i){
  if(base = GEL->flist[pllinfo[active_handle].eloop[k]]->edge[i].base){
    /* find first edge in list which is in local region and set it
       to base */
    while(base){
      if(inc[base->eid]+1){
        new_E[k]->edge[i].base = new_E[inc[base->eid]]->edge +
    base->id;
        break;
      }
      else
        base = base->link;
    }

    /* point link to the link list of U structure */
    link = base->link;

    /* reset base to point to local beginning of real list */
    base = new_E[k]->edge[i].base;

    while(link){
      /* find next linked edge which is in patch */
      while(link&&!(inc[link->eid]+1))
        link = link->link;

      if(link){
        /* reset base to point to new link */
        base->link = new_E[inc[link->eid]]->edge + link->id;
        base = base->link;

        link = link->link;
      }
      else
        base->link = (Edge*) NULL;
    }
  }
      }

      for(i = 0; i < new_E[k]->Nfaces; ++i)
  if((face = new_E[k]->face+i)->link){
    if(inc[face->link->eid]+1)
      new_E[k]->face[i].link = new_E[inc[face->link->eid]]->face +
        face->link->id;
    else{
      new_E[k]->face[i].link = new_E[k]->face + i; // reflexive link
      add_pf(k,i); // mlevel data .
    }
  }
    }
  }
  free(inc);

  Element_List * EL = (Element_List*) new Element_List(new_E, lnel);

  EL->Cat_mem();
  for(E=EL->fhead;E;E=E->next){
    E->geom = (Geom*)0;
    E->set_geofac();
    if(E->geom->singular){
      double xo,yo,zo;
      xo = yo = zo = 0.0;
      for (int iv = 0; iv < E->Nverts; ++iv){
        xo += E->vert[iv].x;
        yo += E->vert[iv].y;
        zo += E->vert[iv].z;
      }
      xo /= E->Nverts;
      yo /= E->Nverts;
      zo /= E->Nverts;
      fprintf(stderr,"Jacobian is negative in element %d (xyz: %f %f %f )\n",E->id+1,xo,yo,zo);
    }
  }

  return EL;
}

static void  add_pf(int elmt, int side){
  register int i;
  int flag = 1;
  int active_handle = get_active_handle();


  for(i = 0; i < pllinfo[active_handle].npface; ++i)
    if((elmt == pllinfo[active_handle].npfeid[i])&&(side == pllinfo[active_handle].npfside[i]))
      flag = 0;

  if(flag){
    pllinfo[active_handle].npface++;
    pllinfo[active_handle].npfeid  = (int *)realloc(pllinfo[active_handle].npfeid,
             pllinfo[active_handle].npface*sizeof(int));
    pllinfo[active_handle].npfeid[pllinfo[active_handle].npface-1] = elmt;
    pllinfo[active_handle].npfside = (int *) realloc(pllinfo[active_handle].npfside,
              pllinfo[active_handle].npface*sizeof(int));
    pllinfo[active_handle].npfside[pllinfo[active_handle].npface-1] = side;
  }
}

/* these routines are for reading the old hybrid format field files which
   need to know the number of facets in each element befor being able
   to read the field files. The new format stores this information directly
   into the files */

static int *nfacetlist;
void set_nfacet_list(Element_List *U){

  if(!nfacetlist){
    int i;
    int nel = countelements(U->fhead);
    Element *E;

    nfacetlist = ivector(0,nel-1);

    for(E=U->fhead,i=0; E; E = E->next,++i)
      nfacetlist[i] = E->Nedges+E->Nfaces;

    if(U->fhead->dim() == 3) /* add interior facet */
      for(i = 0; i < nel; ++i)
  ++nfacetlist[i];
  }
}

void get_facet(int nel,int *nfacet,int *emap, int shuffled){

  if(!nfacetlist){
    fprintf(stderr,"To read Old Hybrid format fieldfile -old must be "
      "specified in command line (or run old2new utility) \n");
    exit(-1);
  }

  if(shuffled){
    register int i;

    for(i = 0; i < nel; ++i)
      nfacet[i] = nfacetlist[emap[i]];
  }
  else
    icopy(nel,nfacetlist,1,nfacet,1);
}
