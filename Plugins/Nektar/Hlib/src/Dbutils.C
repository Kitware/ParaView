#include <math.h>
#include <veclib.h>
#include "hotel.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Hlib/src/Dbutils.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 09:59:30 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#ifdef DEBUG
Telapse timetest[_MAX_TIMECALL];

FILE *debug_out = stdout;

/* force compiler to include this file when debugging */
void init_debug(void){
};

void plotdvector(double *mat, int rows, int cols){
  int i,j,count = 0;

  for(i = 0; i < rows ; ++i){
    for(j = 0; j < cols; ++j)
      if(fabs(mat[i*cols+j]) > 10e-12){
  if(mat[i*cols+j] > 0) putchar('+');
  else              putchar('*');
  ++count;
      }
      else
  putchar('-');
    fprintf(debug_out,"\n");
  }

  fprintf(debug_out,"%d values above 10e-12 \n",count);
  return;
}
/*----------------------------------------------------------------------*
 * Plots a '*' if a the absolute value of a matix mat[r][c] is greater  *
 * than 10e-10. The matrix indices run from rl to ru rows by cl to cu   *
 * columns.                                                             *
 *----------------------------------------------------------------------*/

void plotdmatrix(double **mat, int rl, int ru, int cl, int cu){
  int i,j,count = 0;

  for(i = rl; i <= ru; ++i){
    for(j = cl; j <= cu; ++j)
      if(fabs(mat[i][j]) > 10e-12){
  if(mat[i][j] > 0) putchar('+');
  else              putchar('*');
  ++count;
      }
      else
  putchar('-');
    fprintf(debug_out,"\n");
  }

  fprintf(debug_out,"%d values above 10e-12 \n",count);
  return;
}

#define LLEN 7
void showdmatrix(double **d, int rl, int ru, int cl, int cu)
{
  register int i,j,k;
  int nc = (cu-cl)/LLEN+1,l;

  for(i = rl; i <= ru ; ++i){
    fprintf(debug_out,"row %d\n",i);
    for(j = 0,l=cl; j < nc; ++j,l+=LLEN){
      for(k = l; k <= min(l+LLEN-1,cu); ++k)
  fprintf(debug_out,"%#8.6lg ",d[i][k]);
      fprintf(debug_out,"\n");
    }
  }

  return;
}

void showdvector(double *d, int l, int u)
{
  register int i,j;
  int n = (u-l)/LLEN+1;

  for(i = 0; i < n ; ++i,l+=LLEN){
    for(j = l; j <= min(l+LLEN-1,u); ++j)
      fprintf(debug_out,"%#8.6lg ",d[j]);
    fprintf(debug_out,"\n");
  }
  fflush(debug_out);
  return;
}

void showivector(int *d,int l, int u)
{
  int i;

  for(i = l; i <= u ; ++i)
    fprintf(debug_out,"%d ",d[i]);
  fprintf(debug_out,"\n");

  return;
}

void showfield(Element *E){
  register int i,j;
  Vert *v = E->vert;
  Edge *e = E->edge;
  Face *f = E->face;

  fprintf(debug_out,"\n");
  fprintf(debug_out,"field \'%c\' in Element %d. state is \'%c\'",
   E->type,E->id+1,E->state);

  Nek_Facet_Type id = E->identify();

  fprintf(debug_out," Shape = ");

  switch(id){
  case Nek_Tri:
    fprintf(debug_out,"Triangle"); break;
  case Nek_Quad:
    fprintf(debug_out,"Quadrilateral"); break;
  case Nek_Nodal_Quad:
    fprintf(debug_out,"Nodal Quadrilateral"); break;
  case Nek_Nodal_Tri:
    fprintf(debug_out,"Nodal Triangle"); break;
  case Nek_Seg:
    fprintf(debug_out,"Segment"); break;
  }
  fprintf(debug_out,"\n");

  for(i=0;i < E->Nverts;++i)
    fprintf(debug_out,"Vertex %c: data is: %lf\n",i+1,v[i].hj[0]);

    if(E->state == 'p'){
      if(E->dim() == 3){
  for(i = 0; i < E->qc; ++i){
    fprintf(debug_out,"Interior at i = %d\n",i);
    showdmatrix(E->h_3d[i],0,E->qb-1,0,E->qa-1);
    fprintf(debug_out,"\n");
  }
      }
      else
  showdmatrix(E->h,0,E->qb-1,0,E->qa-1);
    }
    else{
      for(i=0;i<E->Nedges;++i){
  if(e[i].l){
    fprintf(debug_out,"Edge %d: data:\n",i+1);
    showdvector(e[i].hj,0,e[i].l-1);
  }
      }
      for(j=0;j<E->Nfaces;++j){
  if(f[j].l){
    fprintf(debug_out,"Face %d: data:\n",j+1);
    switch(id){
    case Nek_Tri:
      for(i = 0; i < f[j].l; ++i)
        showdvector(f[j].hj[i],0,f[j].l-i-1);
      break;
    case Nek_Quad: case Nek_Nodal_Quad: case Nek_Nodal_Tri:
      for(i = 0; i < f[j].l; ++i)
        showdvector(f[j].hj[i],0,f[j].l-1);
      break;
    case Nek_Tet: case Nek_Pyr: case Nek_Prism: case Nek_Hex:
      if(E->Nfverts(j) == 3)
        for(i = 0; i < f[j].l; ++i)
    showdvector(f[j].hj[i],0,f[j].l-i-1);
      else
        for(i = 0; i < f[j].l; ++i)
    showdvector(f[j].hj[i],0,f[j].l-1);
      break;
    }
  }
      }

      if(E->dim() == 3)
  if(E->interior_l)
    switch(id){
    case Nek_Tet:
      for(i = 0; i < E->interior_l; ++i){
        fprintf(debug_out,"Interior at m = %d\n",i);
        for(j = 0; j < E->interior_l-i; ++j)
    showdvector(E->hj_3d[i][j],0,E->interior_l-i-j-1);
        fprintf(debug_out,"\n");
      }
      break;
    }
    }
}

void showbc(Bndry *Ubc){
  int i,nfv;
  Element *E;
  double l;

  Bndry *Ebc = Ubc;

  fprintf(debug_out,"\n");
  fprintf(debug_out,"Bndry   id \t : %d \n",Ebc->id);
  fprintf(debug_out,"Element id \t : %d \n",Ebc->elmt->id+1);
  fprintf(debug_out,"Face       \t : %d \n",Ebc->face+1);
  fprintf(debug_out,"Type       \t :\'%c\'\n",Ebc->type);
  fprintf(debug_out,"String     \t : %s",Ebc->bstring);
  fprintf(debug_out,"\n");

  E = Ebc->elmt;

  nfv = E->Nfverts(Ebc->face);

  for(i = 0; i < nfv; ++i)
    fprintf(debug_out,"Vertex %d   : %lf  \n",i,Ebc->bvert[i]);

  if(E->dim() == 2){
    if(l=Ebc->elmt->edge[Ebc->face].l){
      fprintf(debug_out,"Solution values on edge are : \n");
      showdvector(Ebc->bedge[0],0,(int)l-1);
    }
  }
  else{
    for(i = 0; i < nfv; ++i)
      if(l = E->edge[E->ednum(Ebc->face,i)].l){
  fprintf(debug_out,"Solution values on edge %d are : \n",i);
  showdvector(Ebc->bedge[i],0,(int)l-1);
      }


    if(l = E->face[Ebc->face].l){
      fprintf(debug_out,"Solution values on face (%d) are : \n",i);
      if(nfv == 3)
  for(i = 0; i < l; ++i)
    showdvector(Ebc->bface[i],0,(int)l-i-1);
      else
  for(i = 0; i < l; ++i)
    showdvector(Ebc->bface[i],0,(int)l-1);
    }
  }

  fprintf(debug_out,"\n");
  fprintf(debug_out,"\n");
}

void showbcs(Bndry *Ubc){
  int i,nfv;
  Element *E;
  double l;

  Bndry *Ebc;
  for(Ebc = Ubc; Ebc; Ebc = Ebc->next){
    fprintf(debug_out,"\n");
    fprintf(debug_out,"Bndry   id \t : %d \n",Ebc->id);
    fprintf(debug_out,"Element id \t : %d \n",Ebc->elmt->id+1);
    fprintf(debug_out,"Face       \t : %d \n",Ebc->face+1);
    fprintf(debug_out,"Type       \t :\'%c\'\n",Ebc->type);
    fprintf(debug_out,"String     \t : %s",Ebc->bstring);
    fprintf(debug_out,"\n");

    E = Ebc->elmt;

    nfv = E->Nfverts(Ebc->face);

    for(i = 0; i < nfv; ++i)
      fprintf(debug_out,"Vertex %d   : %lf  \n",i,Ebc->bvert[i]);

    if(E->dim() == 2){
      if(l=Ebc->elmt->edge[Ebc->face].l){
  fprintf(debug_out,"Solution values on edge are : \n");
  showdvector(Ebc->bedge[0],0,(int)l-1);
      }
    }
    else{
      for(i = 0; i < nfv; ++i)
  if(l = E->edge[E->ednum(Ebc->face,i)].l){
    fprintf(debug_out,"Solution values on edge %d are : \n",i);
    showdvector(Ebc->bedge[i],0,(int)l-1);
  }


      if(l = E->face[Ebc->face].l){
  fprintf(debug_out,"Solution values on face (%d) are : \n",i);
  if(nfv == 3)
    for(i = 0; i < l; ++i)
      showdvector(Ebc->bface[i],0,(int)l-i-1);
  else
    for(i = 0; i < l; ++i)
      showdvector(Ebc->bface[i],0,(int)l-1);
      }
    }

    fprintf(debug_out,"\n");
    fprintf(debug_out,"\n");
  }
}

void showbsys(Bsystem *B, Element_List *EL){
  Element *E;

  fprintf(debug_out,"\n");
  fprintf(debug_out,"Bsystem\n");
  fprintf(debug_out,"\n");
  fprintf(debug_out,"Solve method:   %d\n",(int)B->smeth);
  fprintf(debug_out,"Nel:            %d\n",B->nel);
  fprintf(debug_out,"Nvs:            %d\n",B->nv_solve);
  fprintf(debug_out,"Nes:            %d\n",B->ne_solve);
  fprintf(debug_out,"Nfs:            %d\n",B->nf_solve);
  fprintf(debug_out,"Nsolve:         %d\n",B->nsolve);
  fprintf(debug_out,"Nglobal:        %d\n",B->nglobal);
  fprintf(debug_out,"\n");
  fprintf(debug_out,"Edge maps (unknowns):\n");
  showivector(B->edge, 0, B->ne_solve-1);
  fprintf(debug_out,"Face maps (unknowns):\n");
  showivector(B->edge, 0, B->nf_solve-1);
  fprintf(debug_out,"Bmap                :\n");
  for(E=EL->fhead;E;E=E->next){
    fprintf(debug_out,"Element: %d\n", E->id);
    showivector(B->bmap[E->id], 0, E->Nbmodes-1);
  }

  fprintf(debug_out,"Singular:       %d\n",B->singular);
  fprintf(debug_out,"Families:       %d\n",B->families);
  //  fprintf(debug_out,"Lambda:         %lf\n",B->lambda);
}

#if 0
void showcoord(Cmodes *X,int L){
  register int i;

  for(i = 0; i < 3; ++i){ // quick hack
    fprintf(debug_out,"edge %d transformed coords\n",i);
    showdvector(X->Cedge[i],0,L-1);
  }

  if(X->Cface){
    fprintf(debug_out,"face transformed coords\n");
    for(i = 0; i < L-1; ++i)
      showdvector(X->Cface[i],0,L-1-i);
  }
}
#endif
void change_state(Element *U, char state){
  for(;U;U = U->next) U->state = state;
}
/* plot a packed matrix system */
void plotsystem(double *a, int nsolve, int bwidth){
  register int i,j,k;

  if(2*bwidth < nsolve){
    for(i = 0; i < nsolve-bwidth; ++i){
      for(j = 0; j < bwidth; ++j)
  if(fabs(a[i*bwidth + j]) > 1e-12){
      if(a[i*bwidth+j] > 0) putchar('+');
    else                  putchar('*');
        }
  else
    putchar('-');
      fprintf(debug_out,"\n");
    }


    for(i = nsolve-bwidth; i < nsolve; ++i){
      for(j = 0; j < nsolve-i; ++j)
   if(fabs(a[i*bwidth + j]) > 1e-12){
     if(a[i*bwidth+j] > 0) putchar('+');
     else                  putchar('*');
         }
   else
     putchar('-');
       fprintf(debug_out,"\n");
     }
  }
  else
    for(i = 0,k=0; i < nsolve; ++i){
      for(j = i; j < nsolve; ++j,++k)
  if(fabs(a[k]) > 1e-12){
    if(a[k] > 0) putchar('+');
    else                  putchar('*');
  }
  else
    putchar('-');
      fprintf(debug_out,"\n");
    }
}

#if 0
void pmem(void ){
  struct mallinfo minfo = mallinfo();
  fprintf(debug_out," arena                    : %d \n",minfo.arena   );
  fprintf(debug_out," ordinary blocks          : %d \n",minfo.ordblks );
  fprintf(debug_out," small blocks             : %d \n",minfo.smblks  );
  fprintf(debug_out," space in holding blocks  : %d \n",minfo.hblkhd  );
  fprintf(debug_out," # of holding blocks      : %d \n",minfo.hblks   );
  fprintf(debug_out," small blocks in use      : %d \n",minfo.usmblks );
  fprintf(debug_out," free small blocks        : %d \n",minfo.fsmblks );
  fprintf(debug_out," ordinary blocks in use   : %d \n",minfo.uordblks);
}
#endif

void dump_sc(int bsize, int isize, double **bb, double **bi, double **ii){

  int i,j;
  int cnt = 0;
  for(i = 0; i <bsize; ++i)
    for(j = 0; j <bsize; ++j)
      if(fabs(bb[i][j])>1e-10)
  ++cnt;
  for(i = 0; i <bsize; ++i)
    for(j = 0; j <isize; ++j)
      if(fabs(bi[i][j])>1e-10)
  cnt += 2;

 for(i = 0; i <isize; ++i)
    for(j = 0; j <isize; ++j)
      if(fabs(ii[i][j])>1e-10)
  ++cnt;

 fprintf(debug_out, "VARIABLES = i,j,intensity\n");
 fprintf(debug_out, "ZONE F=POINT, I=%d\n",cnt);

  for(i = 0; i <bsize; ++i)
    for(j = 0; j <bsize; ++j)
      if(fabs(bb[i][j])>1e-10)
  fprintf(debug_out, "%lf %lf %lf\n", i*1.0, j*1.0, bb[i][j]);

  for(i = 0; i <bsize; ++i)
    for(j = 0; j <isize; ++j)
      if(fabs(bi[i][j])>1e-10)
  {
    fprintf(debug_out, "%lf %lf %lf\n", i*1.0, (j+bsize)*1.0, bi[i][j]);
    fprintf(debug_out, "%lf %lf %lf\n", (j+bsize)*1.0, i*1.0, bi[i][j]);
  }

  for(i = 0; i <isize; ++i)
    for(j = 0; j <isize; ++j)
      if(fabs(ii[i][j])>1e-10)
  fprintf(debug_out, "%lf %lf %lf\n", (i+bsize)*1.0, (j+bsize)*1.0, ii[i][j]);

  //  exit(-1);
}

void shownormals(Element_List *EL){
  Element *E;
  int i;

  for(E=EL->fhead;E;E=E->next){
    fprintf(debug_out,"\n");
    for(i=0;i<E->Nfaces;++i){
      fprintf(debug_out,"x\n");
      showdvector(E->face[i].n->x, 0, E->face[i].qface*E->face[i].qface-1);
      fprintf(debug_out,"y\n");
      showdvector(E->face[i].n->y, 0, E->face[i].qface*E->face[i].qface-1);
      fprintf(debug_out,"z\n");
      showdvector(E->face[i].n->z, 0, E->face[i].qface*E->face[i].qface-1);
    }
  }
}

void tecmatrix(FILE *fp, double **data, int asize, int bsize){
  int i,j;
  fprintf(fp, "VARIABLES = i,j,intensity\n");
  fprintf(fp, "ZONE F=POINT, I=%d, J=%d\n",asize, bsize);

  for(i = 0; i <asize; ++i)
    for(j = 0; j <bsize; ++j)
      fprintf(fp, "%lf %lf %lf\n", i*1.0, j*1.0, data[i][j]);
}

void shownum(Element_List *EL){

  Element *E;
  int i,j;

  for(E=EL->fhead;E;E=E->next){
    fprintf(debug_out,"\n");
    fprintf(debug_out,"Elmt: %d Numbering:\n", E->id);
    fprintf(debug_out,"\n");
    for(i=0;i<E->Nfaces;++i){
      fprintf(debug_out,"Face: %d vertex gids:",i);
      for(j=0;j<E->Nfverts(i);++j)
  fprintf(debug_out," %d ", E->vert[E->vnum(i,j)].gid);
      fprintf(debug_out,"\n");
    }
    fprintf(debug_out,"\n");

    for(i=0;i<E->Nedges;++i)
      fprintf(debug_out,"Edge: %d con: %d\n",i,E->edge[i].con);

    for(i=0;i<E->Nverts;++i)
      fprintf(debug_out,"Vert: %d solve: %d\n",i,E->vert[i].solve);

    fprintf(debug_out,"\n");
    for(i=0;i<E->Nfaces;++i){
      fprintf(debug_out,"Face: %d edge gids:",i);
      for(j=0;j<E->Nfverts(i);++j)
  fprintf(debug_out," %d ", E->edge[E->ednum(i,j)].gid);
      fprintf(debug_out,"\n");
    }
    fprintf(debug_out,"\n");
    for(i=0;i<E->Nfaces;++i)
      fprintf(debug_out,"Face: %d face gid: %d con: %d\n",
        i,E->face[i].gid,E->face[i].con);

    fprintf(debug_out,"\n");
    for(i=0;i<E->Nfaces;++i){
      if(E->face[i].link)
  fprintf(debug_out,"Elmt: %d Face: %d -> Elmt: %d Face: %d\n",
    E->face[i].eid+1,E->face[i].id+1,
    E->face[i].link->eid+1, E->face[i].link->id+1);
      else
  fprintf(debug_out,"Elmt: %d Face: %d -> b.c.\n",
    E->id+1,i+1);
    }
  }

  fflush(debug_out);
}
#endif
