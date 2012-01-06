/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/fld2fld.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
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

char *prog   = "fld2fld";
char *usage  = "fld2fld: [options] -m newmesh[.rea] -r oldfile[.rea] oldfile[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "option list:";

/* ---------------------------------------------------------------------- */

static void setup (FileList *f, Element_List **U, Field *fld);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void InterpField(Element_List *E, FileList *f, int nfields, Field *fld);

main (int argc, char *argv[]){
  int       dump=0,nfields;
  Field     fld;
  FileList  f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  memset(&fld, '\0', sizeof (Field));

  dump = readField(f.in.fp, &fld);
  if (!dump         ) error_msg(Restart: no dumps read from restart file);
  fprintf(stderr, "Dimension %d file\n", fld.dim);

  nfields = strlen(fld.type);
  setup (&f, &master,&fld);

  InterpField(master,&f,nfields,&fld);

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

  iparam_set("MODES", fld->lmax);
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

static void InterpField(Element_List *U, FileList *f,
       int nfields, Field *fld){
  register int i,j,k;
  int      dim = U->fhead->dim(),*eids,npts,cnt;
  char     buf[BUFSIZ];
  Coord    Xe,*A;
  FILE     *out = f->out.fp,*mesh = f->mesh.fp;
  Element *E, *NE;
  Element_List *NewMesh;
  double aa, bb, cc, **data;
  double *hr = dvector(0, QGmax-1);
  double *hs = dvector(0, QGmax-1);
  double *ht = dvector(0, QGmax-1);


  /* read mesh file */
  NewMesh = ReadMesh(mesh, strtok(f->mesh.name,"."));
  NewMesh->Cat_mem();
  for(E = NewMesh->fhead;E;E = E->next){
    E->geom = (Geom*)0;
    E->set_geofac();
  }


  if(dim == 2){
    Xe.x = dvector(0,QGmax*QGmax-1);
    Xe.y = dvector(0,QGmax*QGmax-1);
  }
  else{
    Xe.x = dvector(0,QGmax*QGmax*QGmax-1);
    Xe.y = dvector(0,QGmax*QGmax*QGmax-1);
    Xe.z = dvector(0,QGmax*QGmax*QGmax-1);
  }

  data = dmatrix(0,nfields-1,0,NewMesh->htot-1);

  for(NE = NewMesh->fhead,cnt=0; NE; NE = NE->next,cnt+=npts){
    NE->coord(&Xe);

    fprintf(stderr,"Interpolating element %d:",NE->id+1);
    if(dim == 2)
      npts = NE->qa*NE->qb;
    else
      npts = NE->qa*NE->qb*NE->qc;

    Find_local_coords(U,&Xe,npts,&eids,&A);

    for(j = 0; j < nfields; ++j){
      copyfield(fld,j,U->fhead);
      U->Trans(U, J_to_Q);

      for(k = 0; k < npts; ++k){
  if(eids[k] != -1){
    E = U->flist[eids[k]];

    if(dim == 2){
      get_point_shape_2d(E, A->x[k], A->y[k], hr, hs);

      data[j][cnt+k] =
        eval_field_at_pt_2d(E->qa,E->qb, U->flist[E->id]->h[0], hr, hs);
    }
    else{
      get_point_shape_3d(E, A->x[k], A->y[k], A->z[k], hr, hs, ht);
      data[j][cnt+k] = eval_field_at_pt_3d
        (E->qa,E->qb,E->qc,U->flist[E->id]->h_3d[0][0],hr, hs, ht);
    }
  }
  else{
    data[j][cnt+k] = dparam("EXT_DEF"); // can customize
  }
      }
    }
  }

  /*forward tranform new data */
  for(i = 0; i < nfields; ++i){
    dcopy(NewMesh->htot,data[i],1,NewMesh->base_h,1);
    set_state(NewMesh->fhead,'p');
    NewMesh->Trans(NewMesh,Q_to_J);
    dcopy(NewMesh->hjtot,NewMesh->base_hj,1,data[i],1);
  }

  char *type = (char *) calloc(nfields,sizeof(char));
  FILE *outfile[2];
  outfile[0] = outfile[1] = out;

  for(i = 0; i < nfields; ++i)   type[i] = fld->type[i];

  /* write out new field */
  Writefld(outfile, strtok(f->rea.name,"."),fld->step,fld->time,
     nfields,NewMesh,data,type);

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
