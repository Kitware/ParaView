/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/surfvel.C,v $
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

char *prog   = "surfvel";
char *usage  = "surfvel:  [options]  -r file[.rea]  [input[.fld]]\n";
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


static void setup (FileList *f, Element_List **U, int lmax);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Write(Element_List *E, FileList f, Field fld);

main (int argc, char *argv[]){
  int       dump=0,nfields;
  Field     fld;
  FileList  f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  if(!f.rea.fp){
    fputs (usage, stderr);
    exit  (1);
  }


  memset(&fld, '\0', sizeof (Field));
  dump = readField(f.in.fp, &fld);
  if (!dump         ) error_msg(Restart: no dumps read from restart file);
  //  if (fld.dim != DIM) error_msg(Restart: file if wrong dimension);
  fprintf(stderr, "Dimension %d file\n", fld.dim);

  setup (&f, &master,fld.lmax);

  Write(master,f,fld);

  return NULL;
}


Element_List *GMesh;
Gmap *gmap;

static void setup (FileList *f, Element_List **U,int lmax){
  int i,k;
  Curve *curve;

  ReadParams  (f->rea.fp);

  iparam_set("MODES",lmax);

  /* Generate the list of elements */
  fprintf(stderr,"Reading Mesh\n");
  /* Generate the list of elements */
  GMesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  //gmap = GlobalNumScheme(GMesh, (Bndry *)NULL);
  U[0] = LocalMesh(GMesh,strtok(f->rea.name,"."));
  fprintf(stderr,"Finished Reading Mesh\n");

  init_ortho_basis();

  U[0]->fhead->type = 'u';

  return;
}

static void Write(Element_List *E, FileList f, Field fld){
  register int i,j,k,n;
  int       qa, qb, qc,nfs;
  int       qt,size_start,*data_skip,eid,cnt;
  double   *z,*w, *wk,**Dm,**D,*p;
  Coord     X;
  Element  *F;
  Bndry    *Ubc, *B;
  FILE     *out = f.out.fp;
  int      Dump_All = option("Dump_all");
  /* set up bounday structure */
  Ubc = ReadMeshBCs(f.rea.fp,E);

  /* count the number of relevant element reset counters, set geofac */
  for(cnt=0,B = Ubc;B; B = B->next)
    if(B->type == 'W'){
      B->id = cnt++;
      B->elmt->Surface_geofac(B);
    }

  if(!cnt){
    fprintf(stderr,"No walls detected in .rea file\n");
    exit(-1);
  }

  qt = QGmax*QGmax;
  Dm = dmatrix(0,3,0,qt-1);
  dzero(qt,Dm[0],1);
  p = dvector(0,qt-1);
  dzero(qt,p,1);

  D  = dmatrix(0,2,0,QGmax*QGmax*QGmax-1);
  wk = dvector(0, QGmax*QGmax-1);

  /* make a list of data skips */
  data_skip    = ivector(0,E->nel-1);
  data_skip[0] = 0;
  size_start   = 0;
  for(i = 1; i < E->nel; ++i){
    data_skip[i]= data_skip[i-1] +
      E->flist[i-1]->data_len(fld.size+size_start);
    size_start += fld.nfacet[i-1];
  }

  X.x = D[0];
  X.y = D[1];
  X.z = D[2];

  fprintf(out, "VARIABLES = x, y, z, u, v, w, p\n");

  for(cnt=0,B = Ubc;B; B = B->next)
    if(B->type == 'W'||(B->type == 'V')||(Dump_All)){
      F = B->elmt;
      eid = F->id;
      size_start = isum(eid,fld.nfacet,1);
      nfs = F->Nfverts(B->face);
      if(nfs == 3){
  qa = F->qa;
  qb = F->qc; /* fix for prisms */
      }
      else{
  qa = F->qa;
  qb = F->qb;
      }

      for(i = 0; i < 3; ++i){
  F->Copy_field(fld.data[i]+data_skip[eid],fld.size+size_start);
  F->Trans(F,J_to_Q);

  F->GetFace(F->h_3d[0][0],B->face,wk);
  F->InterpToFace1(B->face,wk,Dm[i]);
  InterpToEqui(nfs,qa,qb,Dm[i],Dm[i]);
      }

      F->Copy_field(fld.data[3]+data_skip[eid],fld.size+size_start);
      F->Trans(F,J_to_Q);
      F->GetFace (F->h_3d[0][0],B->face,wk);
      F->InterpToFace1(B->face,wk,p);
      InterpToEqui(nfs,qa,qb,p,p);

      F->coord(&X);

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
      for(i = 0; i < QGmax*QGmax; ++i){
  fprintf(out,"%lg %lg %lg ", X.x[i], X.y[i], X.z[i]);
  fprintf(out,"%lg %lg %lg %lg\n", Dm[0][i],Dm[1][i],Dm[2][i],p[i]);
      }
    }

  free_dmatrix(Dm,0,0); free_dmatrix(D,0,0); free(wk);
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
