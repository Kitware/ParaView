/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/extractsurf.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/01/16 15:49:01 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <veclib.h>
#include <nektar.h>
#include "Quad.h"
#include "Tri.h"
#include <gen_utils.h>

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "extract";
char *usage  = "extract: [options] -m file[.mesh] -r file[.rea] input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
#if DIM == 2
  "-n #   ... Number of mesh points. Default is 15\n";
#else
  "-n #   ... Number of mesh points.";
#endif

/* ---------------------------------------------------------------------- */

static void setup (FileList *f, Element_List **U, Field *fld);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Write(Element_List *E, FileList *f, int nfields, Field *fld);

main (int argc, char *argv[]){
  int       dump=0,nfields;
  Field     fld;
  FileList  f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  memset(&fld, '\0', sizeof (Field));

  dump = readField(f.in.fp, &fld);

  if (!dump ) error_msg(Restart: no dumps read from restart file);
  fprintf(stderr, "Dimension %d file\n", fld.dim);

  nfields = strlen(fld.type);
  setup (&f, &master,&fld);

  Write(master,&f,nfields,&fld);

  return 0;
}

Element_List *GMesh;
Gmap *gmap;

static void setup (FileList *f, Element_List **U, Field *fld){
  int i,k;
  int nfields = strlen(fld->type);
  Curve *curve;

  ReadParams  (f->rea.fp);

  if(!iparam("LQUAD") || !iparam("MQUAD")){
    if((i=iparam("NORDER.REQ"))!=UNSET){
      iparam_set("LQUAD",i);
      iparam_set("MQUAD",i-1);
    }
    else if(option("Qpts")){
      iparam_set("LQUAD",fld->lmax+1);
      iparam_set("MQUAD",fld->lmax);
    }
    else {
      iparam_set("LQUAD",fld->lmax+1);
      iparam_set("MQUAD",fld->lmax);
    }
  }

  iparam_set("MODES",fld->lmax);
  option_set("FAMOFF",fld->lmax);

  /* Generate the list of elements */
  GMesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  gmap  = GlobalNumScheme(GMesh, (Bndry *)NULL);
  U[0]  = LocalMesh(GMesh,strtok(f->rea.name,"."));

  init_ortho_basis();

  fprintf(stderr,"Finished Reading Mesh\n");

  U[0]->fhead->type = fld->type[0];

  return;
}

static void Write(Element_List *U, FileList *f, int nfields, Field *fld){
  register int i,j,k;
  int      dim = U->fhead->dim(),*eids,npts;
  char     buf[BUFSIZ];
  Coord    Xe,*A;
  FILE     *out = f->out.fp,*mesh = f->mesh.fp;
  Element *E;
  double aa, bb, cc, **data;
  double *hr = dvector(0, QGmax-1);
  double *hs = dvector(0, QGmax-1);
  double *ht = dvector(0, QGmax-1);
  Bndry  *Ubc, *B;
  int    nbcs=0;

  /* set up boundary structure */
  Ubc = ReadMeshBCs(f->rea.fp,U);
  for (B=Ubc;B;B=B->next) nbcs++;
  Ubc = bsort(Ubc,nbcs);
  for(B=Ubc;B;B=B->next)
     B->elmt->Surface_geofac(B);

  /* read mesh file */
   fgets(buf,BUFSIZ,mesh);
   sscanf(buf,"%d",&npts);

   data = dmatrix(0,nfields-1,0,npts-1);

   if(dim == 2){
     Xe.x = dvector(0,npts-1);
     Xe.y = dvector(0,npts-1);

     for(i = 0; i < npts; ++i)
       fscanf(mesh,"%lf%lf\n",Xe.x+i,Xe.y+i);
   }
   else{
     Xe.x = dvector(0,npts-1);
     Xe.y = dvector(0,npts-1);
     Xe.z = dvector(0,npts-1);

     for(i = 0; i < npts; ++i){
       fscanf(mesh,"%lf%lf%lf\n",Xe.x+i,Xe.y+i,Xe.z+i);
     }
   }

   Find_local_coords(U,&Xe,npts,&eids,&A);

   for(j = 0; j < nfields; ++j){
     copysurffield(fld,j,Ubc,U);
     U->Trans(U, J_to_Q);

     for(k = 0; k < npts; ++k){
       if(eids[k] != -1){
   E = U->flist[eids[k]];

   if(dim == 2){
     get_point_shape_2d(E, A->x[k], A->y[k], hr, hs);

     data[j][k] =
      eval_field_at_pt_2d(E->qa,E->qb, U->flist[E->id]->h[0], hr, hs);
   }
   else{
     get_point_shape_3d(E, A->x[k], A->y[k], A->z[k], hr, hs, ht);
     data[j][k] = eval_field_at_pt_3d
       (E->qa,E->qb,E->qc,U->flist[E->id]->h_3d[0][0],hr, hs, ht);
   }
       }
       else{
   data[j][k] = dparam("EXT_DEF"); // can customize
       }
     }
   }

   double *normal_x=dvector(0,npts-1);
   double *normal_y=dvector(0,npts-1);
   double *normal_z=dvector(0,npts-1);

   for(k=0;k<npts;++k)
     if(eids[k] != -1){
       E = U->flist[eids[k]];

       get_point_shape_3d(E, A->x[k], A->y[k], A->z[k], hr, hs, ht);

       for(j=0;j<nbcs;j++){
   if (Ubc[j].elmt->id==E->id){
     if(E->curvX){
       normal_x[k]=eval_field_at_pt_2d
         (E->qa,E->qc,Ubc[j].nx.p,hr, ht);
       normal_y[k]=eval_field_at_pt_2d
         (E->qa,E->qc,Ubc[j].ny.p,hr,ht);
       normal_z[k]=eval_field_at_pt_2d
         (E->qa,E->qc,Ubc[j].nz.p,hr,ht);
     }
     else{
       normal_x[k]= Ubc[j].nx.d;
       normal_y[k]= Ubc[j].ny.d;
       normal_z[k]= Ubc[j].nz.d;
     }
   }
       }
     }

   /* write out extracted points */
   fprintf(out,"%d  # number of data points\n",npts);
   for(i = 0; i < npts; ++i){

    if(dim == 2)
      fprintf(out,"%lf %lf",Xe.x[i],Xe.y[i]);
    else
      fprintf(out,"%lf %lf %lf %lf %lf %lf %lf",fld->time,Xe.x[i],
        Xe.y[i],Xe.z[i],normal_x[i],normal_y[i],normal_z[i]);
    for(j = 0; j < nfields; ++j)
      fprintf(out," %lf",data[j][i]);
    fputc('\n',out);
   }

   free(Xe.x); free(Xe.y); if(U->fhead->dim() == 3) free(Xe.z);
   free(A->x); free(A->y); if(U->fhead->dim() == 3) free(A->z);
   free(A); free(eids); free_dmatrix(data,0,0);
}

/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f)
{
  char  c;
  int   i;
  char  fname[FILENAME_MAX];

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }
  iparam_set("Porder",0);
  dparam_set("theta",0.3);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'q':
  option_set("Qpts",1);
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }
#if DIM == 2
  if(iparam("NORDER.REQ") == UNSET) iparam_set("NORDER-req",15);
#endif
  /* open input file */

  if ((*argv)[0] == '-') {
    f->in.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.fld", *argv);
      if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
  fprintf(stderr, "%s: unable to open the input file -- %s or %s\n",
    prog, *argv, fname);
    exit(1);
      }
    }
    f->in.name = strdup(fname);
  }

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
       f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
       f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}
