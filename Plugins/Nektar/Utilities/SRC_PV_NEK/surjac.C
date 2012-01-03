/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/surjac.C,v $
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
#include <nekscal.h>
#include "Quad.h"
#include "Tri.h"
#include <gen_utils.h>


/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "surjac";
char *usage  = "surjac:  [options]  file[.rea]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-q     ... quadrature point spacing. Default is even spacing\n"
  "-R     ... range data information. must have mesh file specified\n"
  "-a     ... dump all faces on boundary \n"
#if DIM == 2
  "-n #   ... Number of mesh points. Default is 15\n";
#else
  "-n #   ... Number of mesh points.\n";
#endif

/* ---------------------------------------------------------------------- */

typedef struct body{
  int N;       /* number of faces    */
  int *elmt;   /* element # of face  */
  int *faceid; /* face if in element */
} Body;

static Body bdy;


static void setup (FileList *f, Element_List **U);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Write(Element_List *E, FileList f);

main (int argc, char *argv[])
{
  int       dump=0,nfields;

  FileList  f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  if(!f.rea.fp){
    fputs (usage, stderr);
    exit  (1);
  }

  setup (&f, &master);

  Write(master,f);

  return 0;
}

static void setup (FileList *f, Element_List **U)
{
  int i,k;
  Curve *curve;

  ReadParams  (f->rea.fp);

  if(iparam("NORDER.REQ") == UNSET)
    iparam_set("MODES",5);
  else
    iparam_set("MODES",iparam("NORDER.REQ"));

  /* Generate the list of elements */
  fprintf(stderr,"Reading Mesh\n");
  U[0] = ReadMesh(f->rea.fp,strtok(f->rea.name,"."));
  U[0]->Cat_mem();
  fprintf(stderr,"Finished Reading Mesh\n");

  U[0]->fhead->type = 'u';

  return;
}

static void Write(Element_List *E, FileList f){
  register int i,j,k,n;
  int      qa,qb,qc,qt,zone,nfs;
  double   *z,*w, *wk;
  Coord    X;
  char     *outformat;
  Element  *F;
  Bndry    *Ubc, *B;
  FILE     *out = f.out.fp;
  double   dump_all = (double) option("Dump_all"),j3d,j2d;

  /* set up bounday structure */
  Ubc = ReadMeshBCs(f.rea.fp,E);

  for(B = Ubc;B; B = B->next) /* make sure geofac and surface geofac are set */
    if((B->type == 'W')||(B->type == 'V')||dump_all){
      F = B->elmt;
      if(!F->geom){
  F->set_geofac();
  if(F->geom->singular)
    fprintf(stderr,"Singular element: %d\n",F->id+1);
      }

      F->Surface_geofac(B);
    }

  F  = E->fhead;
  qt = F->qtot;

  X.x = dvector(0, QGmax*QGmax*QGmax-1);
  X.y = dvector(0, QGmax*QGmax*QGmax-1);
  X.z = dvector(0, QGmax*QGmax*QGmax-1);
  wk  = dvector(0, QGmax*QGmax-1);

  fprintf(out, "VARIABLES = x, y, z, singular, Jac_ratio3d, Jac_ratio2d\n");

  for(B = Ubc;B; B = B->next)
    if((B->type == 'W')||(B->type == 'V')||dump_all){
      F = B->elmt;
      nfs = F->Nfverts(B->face);
      if(F->Nverts == 6){  /* fix for prisms */
  qa = F->qa;
  qb = F->qc;
      }
      else{
  qa = F->qa;
  qb = F->qb;
      }

      F->coord(&X);

      if(F->curvX)
  j3d = F->geom->jac.p[idmin(F->qa*F->qb*F->qc,F->geom->jac.p,1)]/
    F->geom->jac.p[idamax(F->qa*F->qb*F->qc,F->geom->jac.p,1)];
      else
  j3d  = 1;

      if(F->curvX)
  j2d = B->sjac.p[idmin(qa*qb,B->sjac.p,1)]/
    B->sjac.p[idamax(qa*qb,B->sjac.p,1)];
      else
  j2d = 1;

      F->GetFace(X.x,B->face,wk);
      F->InterpToFace1(B->face,wk,X.x);
      InterpToEqui(nfs,qa,qb,X.x,X.x);
      F->GetFace(X.y,B->face,wk);
      F->InterpToFace1(B->face,wk,X.y);
      InterpToEqui(nfs,qa,qb,X.y,X.y);
      F->GetFace(X.z,B->face,wk);
      F->InterpToFace1(B->face,wk,X.z);
      InterpToEqui(nfs,qa,qb,X.z,X.z);

      fprintf(out,"ZONE T=\"Elmt %d Face %d\",I=%d, J=%d, F=POINT\n",
        F->id+1,B->face+1,QGmax,QGmax);
      for(i = 0; i < QGmax*QGmax; ++i)
  fprintf(out,"%lg %lg %lg %d %lg %lg\n", X.x[i], X.y[i],
    X.z[i],(F->geom->singular)?1:0,j3d,j2d);
    }
  free(X.x); free(X.y); free(X.z); free(wk);

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
      case 'b':
  option_set("Body",1);
  break;
      case 'f':
  option_set("FEstorage",1);
  break;
      case 'R':
  option_set("Range",1);
  break;
      case 'q':
  option_set("Qpts",1);
  break;
      case 't':
  if (*++argv[0])
    dparam_set("theta", atof(*argv));
  else {
    dparam_set("theta", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'a':
  option_set("Dump_all",1);
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }

  /* open input file */
  if ((*argv)[0] == '-') {
    f->rea.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->rea.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.rea", *argv);
      if ((f->rea.fp = fopen(fname, "r")) == (FILE*) NULL) {
  fprintf(stderr, "%s: unable to open the input file -- %s or %s\n",
    prog, *argv, fname);
    exit(1);
      }
    }
    f->rea.name = strdup(fname);
  }

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
       f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
       f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}
