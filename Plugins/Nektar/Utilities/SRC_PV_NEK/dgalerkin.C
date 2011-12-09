#include "nektar.h"
#include <stdio.h>
#include <math.h>
#include <veclib.h>
#include <hotel.h>
#include <string.h>


/* set up outward facing normals along faces as well as the edge
   jacobeans divided by the jacobean for the triangle. All points
   are evaluated at the  gauss quadrature points */

void set_sides_geofac(Element_List *EL){
  Element *E;

  for(E=EL->fhead;E;E = E->next)
    E->set_edge_geofac();
}

/*
   This function declares the edge stucture face->h. The size of the
   face is set at present to the minimum of the two order either side
   of the element. They also correspond to Gauss points which are set
   at an order of L as given by face->qedg. Bsoundary faces which have a
   facee->base == NULL are set up to have a dummy edge connected to
   facee->link for the physical values of the boundary conditions
*/

void set_elmt_sides(Element_List *E){
  register int i,j,k,c;
  Face    *f;
  int     ftot;
  double  tot,w;
  Element *U;
  static int set_elmt_sides_init = 1;

  for(k = 0; k < E->nel; ++k)   /* set up number of quadrature points */
    for(i = 0,f=E->flist[k]->face; i < E->flist[k]->Nfaces; ++i){
      f[i].qface = E->flist[f[i].eid]->dgL;
    }

  for(k = 0; k < E->nel; ++k){
    for(i = 0, f=E->flist[k]->face; i < E->flist[k]->Nfaces; ++i){
      /* see if there is an adjacent face */
      if(f[i].link&&(f[i].link != f+i)){ /* last condition needed for parallel */
  f[i].qface = max(f[i].qface,f[i].link->qface);
  if(!(f[i].link->link)) /* set memory in dummy face */
    f[i].link->h = dvector(0,f[i].qface*f[i].qface-1);
      }
      else{ /* set up dummy edges */
  E->flist[k]->face[i].link = f[i].link = (Face *)calloc(1,sizeof(Face));
  memcpy(f[i].link,f+i,sizeof(Face));
  f[i].link->h = dvector(0,f[i].qface*f[i].qface-1);
  f[i].link->link = (Face *)NULL; // flag for dummy face
  f[i].link->con = 0; /* make all dummp faces have a con of 0 */
      }
      /* declare local face memory */
      f[i].h = dvector(0,f[i].qface*f[i].qface-1);
    }
  }

#ifdef PARALLEL
  DO_PARALLEL{  /* if parallel sort out connecting elements info */
    if( set_elmt_sides_init ){
      int      **ibuf,nb,qface;
      int      ncprocs = pllinfo[get_active_handle()].ncprocs,min;
      double   **dbuf;
      ConInfo  *cinfo = pllinfo[get_active_handle()].cinfo;

      /* set up buffers */
      for(i = 0, nb=0; i < ncprocs; ++i)
  nb = max(nb,cinfo[i].nedges);

      // all con are now set to zero.

      ibuf = imatrix(0,ncprocs-1,0,nb-1);

      /* Fill buffer with qedg and swap */
      for(i = 0; i < ncprocs   ; ++i){
  for(j = 0; j < cinfo[i].nedges; ++j)
    ibuf[i][j] = E->flist[cinfo[i].elmtid[j]]->
      face[cinfo[i].edgeid[j]].qface;

  SendRecvRep(ibuf[i],sizeof(int)*(cinfo[i].nedges),cinfo[i].cprocid);
      }

      /* unpack information and set qedg to maximum  */
      for(i = 0; i < ncprocs; ++i)
  for(j = 0; j < cinfo[i].nedges; ++j){
    f = E->flist[cinfo[i].elmtid[j]]->face + cinfo[i].edgeid[j];
    f->link->qface = ibuf[i][j];
    if(f->qface != f->link->qface){
      f->qface = f->link->qface = max(f->qface,f->link->qface);
      free(f->h); f->h = dvector(0,f->qface*f->qface-1);
      free(f->link->h); f->link->h = dvector(0,f->qface*f->qface-1);
    }
  }
      free_imatrix(ibuf,0,0);

      /* finally count up length of all edge contributions */
      for(i = 0; i < ncprocs; ++i){
  cinfo[i].datlen = 0;
  for(j = 0; j < cinfo[i].nedges; ++j){
    qface = E->flist[cinfo[i].elmtid[j]]->
      face[cinfo[i].edgeid[j]].qface;
    cinfo[i].datlen += qface*qface;
  }
      }
    }
    set_elmt_sides_init = 0;
  }
#endif
}

// nodal connectivity for operator splitting
int *Tri_nmap(int l, int con){
  static int **Store_Tri_nmap;

  if(!Store_Tri_nmap){
    register int i,j;

    Store_Tri_nmap = imatrix(0,1,0,QGmax*QGmax-1);

    /* fill with nmaps */
    for(i = 0; i < l; ++i)
      for(j = 0; j < l; ++j)
  Store_Tri_nmap[1][j+i*l] = l-1-j+i*l;  // swap 'a' direction


    for(i = 0; i < l; ++i)
      for(j = 0; j < l; ++j)
  Store_Tri_nmap[0][j+i*l] = j+i*l;
  }

  if(con)
    return Store_Tri_nmap[1];
  else
    return Store_Tri_nmap[0];
}

int *Quad_nmap(int l, int con){
  static int **Store_Quad_nmap;

  if(!Store_Quad_nmap){
    register int i,j;

    Store_Quad_nmap = imatrix(0,7,0,QGmax*QGmax-1);

    // con = 0
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[0][j+i*l] = j+i*l;

    // con = 1
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[1][j+i*l] = l-1-j+i*l;

    // con = 2
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[2][j+i*l] = j+(l-1-i)*l;

    // con = 3
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[3][j+i*l] = l-1-j+(l-1-i)*l;

    // con = 4
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[4][j+i*l] = i+j*l;

    // con = 5
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[5][j+i*l] = l-1-i+j*l;

    // con = 6
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[6][j+i*l] = i+(l-1-j)*l;

    // con = 7
    for(i=0;i<l;++i)
      for(j=0;j<l;++j)
  Store_Quad_nmap[7][j+i*l] = l-1-i+(l-1-j)*l;
  }

  return Store_Quad_nmap[con];
}

void Jtransbwd_Orth(Element_List *EL, Element_List *ELf){
  Element *U, *Uf;
  for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
    Uf->Obwd(U->h_3d[0][0], Uf->h_3d[0][0], Uf->dgL);
}

void EJtransbwd_Orth(Element *U, double *in, double *out){
  U->Obwd(in, out, U->dgL);
}

void InnerProduct_Orth(Element_List *EL, Element_List *ELf){
  Element *U, *Uf;
  for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
    Uf->Ofwd(U->h_3d[0][0], Uf->h_3d[0][0], Uf->dgL);
}

void EInnerProduct_Orth(Element *U, double *in, double *out){
  U->Ofwd(in, out, U->dgL);
}
