/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/gridgen.C,v $
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
#include <gen_utils.h>

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "gridgen";
char *usage  = "gridgen:  [options]  input[.rea]\n";

char *author = "";
char *rcsid  = "";
char *help   =
  "-g      ... write out goemetry denoting singular points (3D only)\n"
  "-q      ... quadrature point spacing. Default is even spacing\n"
  "-R      ... range data information. must have mesh file (-m file) \n"
  "-b      ... make body elements specified in mesh file (-m file) \n"
  "-n #    ... Number of mesh points. Default is 15\n"
  "-e #    ... dump element e and surrounding elements\n"
  "-s      ... dump output in single precision (%lf)\n"
  "-S      ... SM format\n";

/* ---------------------------------------------------------------------- */

typedef struct body{
  int N;       /* number of faces    */
  int *elmt;   /* element # of face  */
  int *faceid; /* face if in element */
} Body;

static Body bdy;

typedef struct range{
  double x[2];
  double y[2];
  double z[2];
} Range;

static Range *rnge;

static void setup (FileList *, Element_List **);
static void Get_Body(FILE *fp);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void WriteMesh(Element_List *E, FILE *out);
static void triangle (Element_List *E, Coord *X);
static void circle   (Element_List *E, Coord *X);

static void dump_faces(FILE *out, Element_List *E, Coord X,  int zone );

main (int argc, char *argv[]){
  FileList      f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

#ifdef DEBUG
  init_debug();
#endif

  setup (&f, &master);
  WriteMesh(master,f.out.fp);

  return 0;
}

static void setup (FileList *f, Element_List **U){
  int i,k;
  Curve *curve;

  ReadParams  (f->rea.fp);

  //option_set("FAMOFF", 1);

  if((i=iparam("NORDER-req"))!=UNSET){
    iparam_set("LQUAD",i);
    iparam_set("MQUAD",i);
  }

  *U = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));       /* Generate the list of elements */
  U[0]->Cat_mem();

  if(option("variable"))  ReadOrderFile   (strtok(f->rea.name,"."),*U);

  if(f->mesh.name) Get_Body(f->mesh.fp);

  return;
}

static char *getcolour(void);
static int Check_range(Element *E);

static void WriteMesh(Element_List *U, FILE *out){
  register int i,j;
  int      qa, qb, qc;
  int      qt,zone;
  double   *z,*w;
  Coord    X,Y;
  char     *outformat,*Colour;
  Element *E;
  int      dim = U->fhead->dim();


  if(!option("Qpts")){  /* reset quadrature points */
    reset_bases();

    for(j=2;j<=QGmax;++j){
      getzw(j,&z,&w,'a');
      for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;

      getzw(j,&z,&w,'b');
      for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;

      if(dim == 3){
  getzw(j,&z,&w,'c');
  for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;
      }
    }
  }

  qt = QGmax*QGmax*QGmax;

  X.x = dvector(0,qt-1);
  X.y = dvector(0,qt-1);
  X.z = dvector(0,qt-1);

  if(dim == 3){
    fprintf(out,"VARIABLES = x y z\n");
    outformat = strdup("%lf \t %lf \t %lf\n");

#ifdef ONEELMT
    int elmtid;
    if(elmtid = option("ELMTID")){ /* dump faces of a single element */
      E = U->flist[elmtid-1];

      E->coord(&X);
      fprintf(out,"ZONE T=\"face  %d\", I=%d, J=%d, F=POINT\n",
        1,E->qa,E->qb);
      for(i = 0; i < E->qa*E->qb; ++i)
  fprintf(out, outformat, X.x[i], X.y[i], X.z[i]);

      fprintf(out,"ZONE T=\"face  %d\", I=%d, J=%d, F=POINT\n",
        2,E->qa,E->qc);
      for(j = 0; j < E->qc; ++j)
  for(i = 0; i < E->qa; ++i)
    fprintf(out, outformat, X.x[j*E->qa*E->qb+i], X.y[j*E->qb*E->qb+i],
      X.z[j*E->qa*E->qb+i]);

      fprintf(out,"ZONE T=\"face  %d\", I=%d, J=%d, F=POINT\n",
        3,E->qb,E->qc);
      for(j = 0; j < E->qc; ++j)
  for(i = 0; i < E->qb; ++i)
    fprintf(out, outformat, X.x[j*E->qa*E->qb+i*E->qa+E->qa-1],
      X.y[j*E->qa*E->qb+i*E->qa+E->qa-1],
      X.z[j*E->qa*E->qb+i*E->qa+E->qa-1]);

      switch (E->identify()){
      case  Nek_Tet:
  fprintf(out,"ZONE T=\"face  %d\", I=%d, J=%d, F=POINT\n",
    4,E->qb,E->qc);
  for(j = 0; j < E->qc; ++j)
    for(i = 0; i < E->qb; ++i)
      fprintf(out, outformat, X.x[j*E->qa*E->qb+i*E->qa],
        X.y[j*E->qa*E->qb+i*E->qa],
        X.z[j*E->qa*E->qb+i*E->qa]);
      break;
      case Nek_Prism:
  fprintf(out,"ZONE T=\"face  %d\", I=%d, J=%d, F=POINT\n",
    4,E->qa,E->qc);
  for(j = 0; j < E->qc; ++j)
    for(i = 0; i < E->qa; ++i)
      fprintf(out, outformat, X.x[j*E->qa*E->qb+i+(E->qb-1)*E->qa],
        X.y[j*E->qa*E->qb+i+(E->qb-1)*E->qa],
        X.z[j*E->qa*E->qb+i+(E->qb-1)*E->qa]);

  fprintf(out,"ZONE T=\"face  %d\", I=%d, J=%d, F=POINT\n",
    5,E->qb,E->qc);
  for(j = 0; j < E->qc; ++j)
    for(i = 0; i < E->qb; ++i)
      fprintf(out, outformat, X.x[j*E->qa*E->qb+i*E->qa],
        X.y[j*E->qa*E->qb+i*E->qa],
        X.z[j*E->qa*E->qb+i*E->qa]);
  break;
      default:
  fprintf(stderr,"This element type not configured\n");
  exit(-1);
      break;
      }

      exit(1);

    }
#endif
    for(zone = 0, E = U->fhead; E; E = E->next){
      if(Check_range(E)){
  qt = E->qtot;
  E->coord(&X);
  fprintf(out,"ZONE T=\"Element %d\", I=%d, J=%d, K=%d, F=POINT\n",
    E->id+1,E->qa,E->qb,E->qc);
  for(i = 0; i < qt; ++i)
    fprintf(out, outformat, X.x[i], X.y[i], X.z[i]);
      }
    }
  }
  else{
   if(option("FEstorage")){
     int n,k;
     fprintf(out,"VARIABLES = x y\n");
     fprintf(out,"ZONE N=%d, E=%d, F=FEPOINT, ET=QUADRILATERAL\n", U->nel*4, U->nel);

     for(k = 0,zone=0; k < U->nel; ++k){
       for(i = 0; i < U->flist[k]->Nverts; ++i)
   fprintf(out,"%lg %lg\n", U->flist[k]->vert[i].x, U->flist[k]->vert[i].y);
       if(i == 3)
   fprintf(out,"%lg %lg\n", U->flist[k]->vert[i-1].x, U->flist[k]->vert[i-1].y);
     }

     for(k = 0,n=1; k < U->nel; ++k){
       fprintf(out,"%d %d %d %d\n",n,n+1,n+3,n+2);
       n += 4;
     }
   }
   else{
     if(option("SMformat")){
       for(zone = 0, E=U->fhead; E; E = E->next)
   if(Check_range(E)) ++zone;
       fprintf(out,"%d %d %d %d Nr Ns Nz Nel\n",U->fhead->qa,U->fhead->qb,0,
         zone);
     }
     else{
       fprintf(out,"VARIABLES = x y\n");
     }
     outformat = strdup("%lf \t %lf\n");
     for(zone = 0, E=U->fhead; E; E = E->next){
       if(Check_range(E)){
   qt = E->qtot;
   E->coord(&X);
   if(!option("SMformat"))
   fprintf(out,"ZONE T=\"Element %d\", I=%d, J=%d, F=POINT\n",
     ++zone,E->qa,E->qb);
   for(i = 0; i < qt; ++i)
     fprintf(out, outformat, X.x[i], X.y[i], X.z[i]);
       }
     }
   }
  }

  if(bdy.N) dump_faces(out,U,X,zone);

  free(X.x); free(X.y);  free(X.z);

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

  option_set("SingMesh",0);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'b':
  option_set("Body",1);
  break;
      case 'e':
  if (*++argv[0])
    option_set("ELMTID", atoi(*argv));
  else {
    option_set("ELMTID", atoi(*++argv));
    argc--;
  }
  while(*++argv[0]); /* skip over whole number */

      case 'R':
  option_set("Range",1);
  break;
      case 'g':
  option_set("SingMesh",1);
  break;
      case 's':
  option_set("Spre",1);
  break;
      case 'q':
  option_set("Qpts",1);
  break;
      case 'f':
  option_set("FEstorage",1);
  break;
      case 'S':
  option_set("SMformat",1);
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }
#if DIM ==2
  if(iparam("NORDER-req") == UNSET) iparam_set("NORDER-req",8);
#endif

  /* open the .rea file */

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

static int Check_range(Element *E){
  int elmtid;
  static int init = 1;
  if(rnge){
    register int i;
    int cnt = 0;
    if(E->dim() == 3)
      for(i = 0; i < E->Nverts; ++i){
  if(      (E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
        && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])
        && (E->vert[i].z > rnge->z[0])&&(E->vert[i].z < rnge->z[1])) ++cnt;
      }
    else
      for(i = 0; i < E->Nverts; ++i){
  if(   (E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
     && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])) ++cnt;
      }
    return (cnt == E->Nverts) ? 1:0;
  }
  else if(elmtid = option("ELMTID")){
    static int *eids;

    if(init){
      Edge *e,*e1;
      Element *U;
      int nel = countelements(E),i;
      eids = ivector(0,nel-1);

      izero(nel,eids,1);

      eids[elmtid-1] = 1;
      /* find element */
      for(U = E; U; U = U->next)
  if(U->id == elmtid-1)
    break;

      for(i = 0; i < U->Nedges; ++i)
  for(e = U->edge[i].base; e; e = e->link)
    eids[e->eid] = 1;
      init = 0;
    }

    return eids[E->id];
  } else
    return 1;
}

static void Get_Body(FILE *fp){
  register int i;
  char buf[BUFSIZ],*s;
  int  N;

  if(option("Range")){
    rnge = (Range *)malloc(sizeof(Range));
    rewind(fp);  /* search for range data */
    while(!strstr((s=fgets(buf,BUFSIZ,fp)),"Range"));

    if(s!=NULL){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf %lf",rnge->x,rnge->x+1);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf %lf",rnge->y,rnge->y+1);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf %lf",rnge->z,rnge->z+1);
    }
  }

  if(option("Body")){
    rewind(fp);/* search for body data  */
    while(!strstr((s=fgets(buf,BUFSIZ,fp)),"Body"));

    if(s!=NULL){

      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d",&N);

      bdy.N = N;
      bdy.elmt   = ivector(0,N-1);
      bdy.faceid = ivector(0,N-1);

      for(i = 0; i < N; ++i){
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%d%d",bdy.elmt+i,bdy.faceid+i);
  --bdy.elmt[i];
  --bdy.faceid[i];
      }
    }
  }
}

static void dump_faces(FILE *out,Element_List *U, Coord X, int zone ){
  Element *E;
  int qa, qb, qc;
  register int i,j,k,n;

  for(k = 0; k < bdy.N; ++k){
    E = U->flist[bdy.elmt[k]];
    E->coord(&X);
    qa = E->qa;     qb = E->qb;     qc = E->qc;
    switch(bdy.faceid[k]){
    case 0:
      fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, K=%d, F=POINT\n",
        ++zone,qa,qb,1);

      for(i = 0; i < qa*qb; ++i)
  fprintf(out,"%lg %lg %lg\n", X.x[i], X.y[i], X.z[i]);
      break;
    case 1:
      fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, K=%d, F=POINT\n",
        ++zone,qa,qc,1);

      for(i = 0; i < qc; ++i)
  for(j = 0; j < qa; ++j)
    fprintf(out,"%lg %lg %lg \n", X.x[qa*qb*i+j],
    X.y[qa*qb*i+j], X.z[qa*qb*i+j]);
      break;
    case 2:
      fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, K=%d, F=POINT\n",
        ++zone,qb,qc,1);

      for(i = 0; i < qb*qc; ++i)
  fprintf(out,"%lg %lg %lg\n", X.x[qa-1 + i*qa],
    X.y[qa-1 + i*qa], X.z[qa-1 + i*qa]);
      break;
    case 3:
      fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, K=%d, F=POINT\n",
        ++zone,qb,qc,1);

      for(i = 0; i < qb*qc; ++i)
  fprintf(out,"%lg %lg %lg\n", X.x[i*qa],X.y[i*qa], X.z[i*qa]);
      break;
    }
  }
}
