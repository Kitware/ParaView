/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/fro2surf.C,v $
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

char *prog   = "surface";
char *usage  = "surface:  [options]  file[.rea]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-q     ... dump at quadrature points (can not be used with -e)  \n"
  "-e     ... dump as a single zone at equispaced points in FEdata \n"
  "-n #   ... Number of mesh points.\n";

/* ---------------------------------------------------------------------- */

typedef struct body{
  int N;       /* number of faces    */
  int *elmt;   /* element # of face  */
  int *faceid; /* face if in element */
} Body;

static Body bdy;


static void setup (FileList *f, Element_List **U);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void WriteSurfPts (FileList *f);

main (int argc, char *argv[]){
  int       dump=0,nfields;

  FileList  f;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  if(!f.in.fp){
    fputs (usage, stderr);
    exit  (1);
  }

  WriteSurfPts (&f);

  return 0;
}

static void WriteSurfPts (FileList *f){
  int i,j,q,n,cnt,cnt1;
  int nfface, nfvert;
  Curve *curve;
  Coord X,V;
  double *x, *y, *z,*wk;
  double *tmp,j2d;
  Element *E;
  FILE    *infile;
  char    buf[BUFSIZ];
  FILE    *out = f->out.fp;
  Tri      T;
  int symm = option("Equispaced");
  int quad = option("Quadrature");
  Element_List *U;
  Bndry    *B;

  if(!(infile = fopen(f->in.name,"r"))){
    sprintf(buf,"%s.fro",strtok(f->in.name,"."));
    if(!(infile = fopen(buf,"r"))){
      fprintf(stderr,"unable to open file %s or %s\n",f->in.name,buf);
      exit(-1);
    }
  }

  fgets(buf,BUFSIZ,infile);
  sscanf(buf,"%d%d",&nfface,&nfvert);
  fclose(infile);

  V.x = dvector(0,5);
  V.y = dvector(0,5);
  V.z = dvector(0,5);


  // set up vertices as standard element
  V.x[0] = -1; V.x[1] =  1; V.x[2] =  1; V.x[3] = -1; V.x[4] = -1; V.x[5] = -1;
  V.y[0] = -1; V.y[1] = -1; V.y[2] =  1; V.y[3] =  1; V.y[4] = -1; V.y[5] =  1;
  V.z[0] = -1; V.z[1] = -1; V.z[2] = -1; V.z[3] = -1; V.z[4] =  1; V.z[5] =  1;

  // set default order to 5
  if((q=iparam("NORDER.REQ")) == UNSET) q = 5;

  // load up vertices and define linear element and curve structure

  QGmax = q;
  LGmax = q-1;
  E = new Prism(0,'u', q-1,q,q-1,q-1, &V);
  U = (Element_List*) new Element_List(&E, 1);
  U->Cat_mem();
  T.qa = q; // set up dummy T element for Interp_symmpts
  T.qb = q-1;


  E->curve = (Curve *)calloc(1,sizeof(Curve));
  E->curve->info.file.name = strdup(f->in.name);
  E->curve->type = T_File;
  E->curve->face = 1; // face 2 on prism should be curved

  // memory for surface points
  X.x = x = dvector(0, QGmax*QGmax*QGmax-1);
  X.y = y = dvector(0, QGmax*QGmax*QGmax-1);
  X.z = z = dvector(0, QGmax*QGmax*QGmax-1);
  wk = dvector(0,QGmax*QGmax-1);
  // set up felisa information
  Load_Felisa_Surface(f->in.name);

  fprintf(out, "VARIABLES = x, y, z, j2d\n");
  if(symm){
    fprintf(out,"ZONE, N=%d, E=%d, F=FEPOINT, ET=TRIANGLE \n",
      nfface*q*(q+1)/2,nfface*(q-1)*(q-1));
  }
  else{
    if(quad)
      fprintf(out,"ZONE N=%d, E=%d, F=FEPOINT, ET=QUADRILATERAL\n",
        nfface*q*(q-1),nfface*(q-1)*(q-2));
    else
      fprintf(out,"ZONE N=%d, E=%d, F=FEPOINT, ET=QUADRILATERAL\n",
        nfface*q*q,nfface*(q-1)*(q-1));

  }


  for(n = 0; n < nfface; ++n){

    // set up vertices and curve information with current felisa data
    Felisa_fillElmt(E,1,n);

    // generate felisa points
    E->set_curved_elmt(U);

    if(n) free(E->geom->rx.p);
    E->geom = (Geom*)0;
    E->set_geofac();
    B = E->gen_bndry('F',1,0.0);

    if(E->curvX){
      j2d = B->sjac.p[idmin(q*(q-1),B->sjac.p,1)]/
  B->sjac.p[idamax(q*(q-1),B->sjac.p,1)];
      free(B->nx.p);
      free(B->ny.p);
      free(B->sjac.p);
      free(B->K.p);
    }
    else
      j2d = 1;

    for(i = 0; i < 3; ++i) free(B->bedge[i]);
    if(E->face[1].l) free(B->bface[0]);
    free(B->bface);
    free(B->bvert);
    free(B->bstring);
    free(B);

    E->coord(&X);
    E->GetFace(X.x,E->curve->face,wk);
    E->InterpToFace1(E->curve->face,wk,X.x);
    E->GetFace(X.y,E->curve->face,wk);
    E->InterpToFace1(E->curve->face,wk,X.y);
    E->GetFace(X.z,E->curve->face,wk);
    E->InterpToFace1(E->curve->face,wk,X.z);

    if(symm){
      Interp_symmpts(&T,q,X.x,wk,'n');
      dcopy(q*q,wk,1,X.x,1);
      Interp_symmpts(&T,q,X.y,wk,'n');
      dcopy(q*q,wk,1,X.y,1);
      Interp_symmpts(&T,q,X.z,wk,'n');
      dcopy(q*q,wk,1,X.z,1);
    }
    else
      if(!quad){
  InterpToEqui(3,q,q-1,X.x,X.x);
  InterpToEqui(3,q,q-1,X.y,X.y);
  InterpToEqui(3,q,q-1,X.z,X.z);
      }

    if(symm)
      for(i = 0; i < q*(q+1)/2; ++i)
  fprintf(out,"%lg %lg %lg %lg\n", X.x[i], X.y[i], X.z[i],j2d);
    else
      if(quad)
  for(i = 0; i < q*(q-1); ++i)
    fprintf(out,"%lg %lg %lg %lg\n", X.x[i], X.y[i], X.z[i],j2d);
      else
  for(i = 0; i < q*q; ++i)
    fprintf(out,"%lg %lg %lg %lg\n", X.x[i], X.y[i], X.z[i],j2d);
  }

  // dump connectivity
  if(symm){
    for(cnt = n = 0; n < nfface; ++n){
      for(cnt1 = cnt,j = 0; j < q-1; ++j){
  for(i = 0; i < q-2-j; ++i){
    fprintf(out,"%d %d %d\n",cnt1+i+1, cnt1+i+2,cnt1+q-j+i+1);
    fprintf(out,"%d %d %d\n",cnt1+q-j+i+2,cnt1+q-j+i+1,cnt1+i+2);
  }
  fprintf(out,"%d %d %d\n",cnt1+q-1-j,cnt1+q-j,cnt1+2*q-2*j-1);
  cnt1 += q-j;
      }
      cnt += q*(q+1)/2;
    }
  }
  else
    if(quad)
      for(cnt = n = 0; n < nfface; ++n){
  for(i = 0; i < q-2; ++i)
    for(j = 0; j < q-1; ++j)
      fprintf(out,"%d %d %d %d\n",j+i*q+cnt+1, j+i*q+cnt+2,
        j+(i+1)*q+cnt+2, j+(i+1)*q+cnt+1);
  cnt += q*(q-1);
      }
    else
      for(cnt = n = 0; n < nfface; ++n){
  for(i = 0; i < q-1; ++i)
    for(j = 0; j < q-1; ++j)
      fprintf(out,"%d %d %d %d\n",j+i*q+cnt+1, j+i*q+cnt+2,
        j+(i+1)*q+cnt+2, j+(i+1)*q+cnt+1);
  cnt += q*q;
      }

  free(X.x); free(X.y); free(X.z);
  free(V.x); free(V.y); free(V.z);
  Free_Felisa_data();

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
  option_set("Quadrature",1);
  break;
      case 'e':
  option_set("Equispaced",1);
  break;
      case 'f':
  option_set("FEstorage",1);
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
    if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.fro", strtok(*argv,"."));
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
