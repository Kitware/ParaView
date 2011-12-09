/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/shear.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <veclib.h>
#include <nekscal.h>
#include "Quad.h"
#include "Tri.h"
#include <gen_utils.h>

#include <stdio.h>
#include <stdlib.h>



/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "shear";
char *usage  = "shear:  [options]  -r file[.rea]  [input[.fld]]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-q     ... quadrature point spacing. Default is even spacing\n"
  "-R     ... range data information. must have mesh file specified\n"
  "-a     ... dump all faces on boundary \n"
  "-c     ... only dump surface which is defined as a cylinder \n"
  "-C     ... dump cylindrical 2d shear vector based upon [AX,AY,AZ]\n"
  "-e      ... dump as a single zone at equispaced points in FEdata \n"
  "-S #    ... only dump information from Felisa surface # \n"
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

typedef struct range{
  double x[2];
  double y[2];
  double z[2];
} Range;

static Range *rnge;

static void setup (FileList *f, Element_List **U, int lmax);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Write(Element_List *E, FileList f, Field fld);
static int Check_surface(Bndry *B, FileList f);
static void Get_Body(FILE *fp);

main (int argc, char *argv[])
{
  int       dump=0,nfields;
  Field     fld;
  FileList  f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  if(!f.rea.fp){
    fputs (usage, stderr);
    exit  (1);
  }

  if(f.mesh.name) Get_Body(f.mesh.fp);

  memset(&fld, '\0', sizeof (Field));
  dump = readField(f.in.fp, &fld);
  if (!dump         ) error_msg(Restart: no dumps read from restart file);
  //  if (fld.dim != DIM) error_msg(Restart: file if wrong dimension);
  fprintf(stderr, "Dimension %d file\n", fld.dim);

  setup (&f, &master,fld.lmax);

  Write(master,f,fld);

  return 0;
}


Element_List *GMesh;
Gmap *gmap;

static void setup (FileList *f, Element_List **U,int lmax){
  int i,k;
  Curve *curve;

  ReadParams  (f->rea.fp);

  if((i=iparam("NORDER.REQ"))!=UNSET){
    iparam_set("LQUAD",i+1);
    iparam_set("MQUAD",i+1);
    iparam_set("MODES",i);
  }
  else
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
  int       qa,qb,qc,nfs,qt,size_start,*data_skip,eid,cnt;
  double   *z,*w,*wk,**Dm,**D,*p,**s,**s1;
  Coord     X;
  Element  *F;
  Bndry    *Ubc, *B;
  FILE     *out = f.out.fp;
  double   kinvis = dparam("KINVIS");
  Tri      T;
  int symm = option("Equispaced");

  /* set up bounday structure */
  Ubc = ReadMeshBCs(f.rea.fp,E);

  /* count the number of relevant element reset counters, set geofac */
  for(cnt=0,B = Ubc;B; B = B->next) {
    B->id = cnt++;
    if(B->type == 'W') B->elmt->Surface_geofac(B);
  }

  qt = QGmax*QGmax;
  Dm = dmatrix(0,8,0,cnt*qt-1);
  dzero(9*cnt*qt,Dm[0],1);
  p = dvector(0,cnt*qt-1);
  dzero(cnt*qt,p,1);

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

  /* calculate derivatives */
  for(cnt=0,B = Ubc;B; B = B->next)
    if((B->type == 'W')&&(Check_surface(B,f)))
      for(i = 0; i < 3; ++i){
  F = B->elmt;
  eid = F->id;
  size_start = isum(eid,fld.nfacet,1);
  F->Copy_field(fld.data[i]+data_skip[eid],fld.size+size_start);
  F->Trans(F,J_to_Q);
  F->Grad_d(D[0],D[1],D[2],'a');

  /* Extract faces and interp to face 1 */
  for(j = 0; j < 3; ++j){
    F->GetFace      (D[j],B->face,wk);
    F->InterpToFace1(B->face,wk,Dm[3*i+j]+B->id*qt);
  }
      }

  /* fill pressure */
  for(cnt=0,B = Ubc;B; B = B->next)
    if((B->type == 'W')&&(Check_surface(B,f))){
      F   = B->elmt;
      eid = F->id;
      size_start = isum(eid,fld.nfacet,1);
      F->Copy_field    (fld.data[3]+data_skip[eid],fld.size+size_start);
      F->Trans         (F,J_to_Q);
      F->GetFace       (F->h_3d[0][0],B->face,wk);
      F->InterpToFace1 (B->face,wk,p+B->id*qt);
    }

  F  = E->fhead;

  X.x = D[0];
  X.y = D[1];
  X.z = D[2];

  s = dmatrix(0,2,0,qt-1);
  if(option("cylindrical")){ // project vectors
    s = dmatrix(0,4,0,qt-1);
    s1 = s + 3;
  }
  else
    s = dmatrix(0,2,0,qt-1);


  if(option("cylindrical"))
    fprintf(out, "VARIABLES = x, y, z, p, s, sx, sy, sz, c1, c2\n");
  else
    fprintf(out, "VARIABLES = x, y, z, p, s, sx, sy, sz\n");

  int counter = 1;
  for(B = Ubc;B; B = B->next)
    if((B->type == 'W')&&(Check_surface(B,f))){
      F  = B->elmt;
      nfs = F->Nfverts(B->face);
      if(nfs == 3){
  qa = F->qa;
  qb = F->qc; /* fix for prisms */
      }
      else{
  qa = F->qa;
  qb = F->qb;
      }
      T.qa = qa; // set up dummy T element for Interp_symmpts
      T.qb = qb;

      if((symm)&&(nfs != 3))
  fprintf(stderr,"Not set up for quad faces \n");

      /* calculate shear stress */
      if(F->curvX)
  for(i = B->id*qt; i < B->id*qt+qa*qb; ++i){
    s[0][i-B->id*qt] = kinvis*(2.0*Dm[0][i]*B->nx.p[i-B->id*qt] +
            (Dm[1][i] + Dm[3][i])*B->ny.p[i-B->id*qt] +
            (Dm[2][i] + Dm[6][i])*B->nz.p[i-B->id*qt]);
    s[1][i-B->id*qt] = kinvis*(2.0*Dm[4][i]*B->ny.p[i-B->id*qt] +
            (Dm[1][i] + Dm[3][i])*B->nx.p[i-B->id*qt] +
                  (Dm[5][i] + Dm[7][i])*B->nz.p[i-B->id*qt]);
    s[2][i-B->id*qt] = kinvis*(2.0*Dm[8][i]*B->nz.p[i-B->id*qt] +
                  (Dm[2][i] + Dm[6][i])*B->nx.p[i-B->id*qt] +
                  (Dm[5][i] + Dm[7][i])*B->ny.p[i-B->id*qt]);
  }
      else
  for(i = B->id*qt; i < B->id*qt+qa*qb; ++i){
    s[0][i-B->id*qt] = kinvis*(2.0*Dm[0][i]*B->nx.d +
                  (Dm[1][i] + Dm[3][i])*B->ny.d +
                  (Dm[2][i] + Dm[6][i])*B->nz.d);
    s[1][i-B->id*qt] = kinvis*(2.0*Dm[4][i]*B->ny.d +
                  (Dm[1][i] + Dm[3][i])*B->nx.d +
                  (Dm[5][i] + Dm[7][i])*B->nz.d);
    s[2][i-B->id*qt] = kinvis*(2.0*Dm[8][i]*B->nz.d +
                  (Dm[2][i] + Dm[6][i])*B->nx.d +
                  (Dm[5][i] + Dm[7][i])*B->ny.d);
  }

      if(option("cylindrical")){ // project vectors
  double ax = dparam("AX");
  double ay = dparam("AY");
  double az = dparam("AZ");
  double bx,by,bz;
  double mag;

  mag = sqrt(ax*ax + ay*ay + az*az);
  if(!mag){
    fprintf(stderr,"Error values of [AX,AY,AZ] == 0\n");
    exit(1);
  }
  ax /= mag;
  ay /= mag;
  az /= mag;

  for(i = 0; i < qa*qb; ++i)
    s1[0][i] = s[0][i]*ax + s[1][i]*ay + s[2][i]*az;

  if(F->curvX){
    for(i = 0; i < qa*qb; ++i){
      bx = ay*B->nz.p[i] - az*B->ny.p[i];
      by = az*B->nx.p[i] - ax*B->nz.p[i];
      bz = ax*B->ny.p[i] - ay*B->nx.p[i];
      s1[1][i] = s[0][i]*bx + s[1][i]*by + s[2][i]*bz;
    }
  }
  else{
    bx = ay*B->nz.d - az*B->ny.d;
    by = az*B->nx.d - ax*B->nz.d;
    bz = ax*B->ny.d - ay*B->nx.d;
    for(i = 0; i < qa*qb; ++i)
      s1[1][i] = s[0][i]*bx + s[1][i]*by + s[2][i]*bz;
  }

  if(symm){
    Interp_symmpts(&T,QGmax,s1[0],wk,'n');
    dcopy(qt,wk,1,s1[0],1);
    Interp_symmpts(&T,QGmax,s1[1],wk,'n');
    dcopy(qt,wk,1,s1[1],1);
  }
  else{
    InterpToEqui(nfs,qa,qb,s1[0],wk);
    dcopy(qt,wk,1,s1[0],1);
    InterpToEqui(nfs,qa,qb,s1[1],wk);
    dcopy(qt,wk,1,s1[1],1);
  }
      }


      F->coord(&X);

      if(symm){

  /* interplate s and p to equispaced points */
  Interp_symmpts(&T,QGmax,s[0],wk,'n');
  dcopy(qt,wk,1,s[0],1);
  Interp_symmpts(&T,QGmax,s[1],wk,'n');
  dcopy(qt,wk,1,s[1],1);
  Interp_symmpts(&T,QGmax,s[2],wk,'n');
  dcopy(qt,wk,1,s[2],1);

  Interp_symmpts(&T,QGmax,p+B->id*qt,wk,'n');
  dcopy(qt,wk,1,p+B->id*qt,1);

  F->GetFace(X.x,B->face,wk);
  F->InterpToFace1(B->face,wk,X.x);
  Interp_symmpts(&T,QGmax,X.x,wk,'n');
  dcopy(qt,wk,1,X.x,1);
  F->GetFace(X.y,B->face,wk);
  F->InterpToFace1(B->face,wk,X.y);
  Interp_symmpts(&T,QGmax,X.y,wk,'n');
  dcopy(qt,wk,1,X.y,1);
  F->GetFace(X.z,B->face,wk);
  F->InterpToFace1(B->face,wk,X.z);
  Interp_symmpts(&T,QGmax,X.z,wk,'n');
  dcopy(qt,wk,1,X.z,1);
      }
      else{
  /* interplate s and p to equispaced points */
  InterpToEqui(nfs,qa,qb,s[0],wk);
  dcopy(qt,wk,1,s[0],1);
  InterpToEqui(nfs,qa,qb,s[1],wk);
  dcopy(qt,wk,1,s[1],1);
  InterpToEqui(nfs,qa,qb,s[2],wk);
  dcopy(qt,wk,1,s[2],1);

  InterpToEqui(nfs,qa,qb,p+B->id*qt,wk);
  dcopy(qt,wk,1,p+B->id*qt,1);

  F->GetFace(X.x,B->face,wk);
  F->InterpToFace1(B->face,wk,X.x);
  InterpToEqui(nfs,qa,qb,X.x,X.x);
  F->GetFace(X.y,B->face,wk);
  F->InterpToFace1(B->face,wk,X.y);
  InterpToEqui(nfs,qa,qb,X.y,X.y);
  F->GetFace(X.z,B->face,wk);
  F->InterpToFace1(B->face,wk,X.z);
  InterpToEqui(nfs,qa,qb,X.z,X.z);
      }


      if(symm){
        fprintf(out,"ZONE T =\" %d \", N=%d, E=%d, F=FEPOINT,"
                "ET=TRIANGLE \n",counter,QGmax*(QGmax+1)/2,(QGmax-1)*(QGmax-1));
        counter++;
/*
  fprintf(out,"ZONE T =\"Elmt %d Face %d\", N=%d, E=%d, F=FEPOINT,"
    "ET=TRIANGLE \n",F->id+1,B->face+1,QGmax*(QGmax+1)/2,
    (QGmax-1)*(QGmax-1));
*/
  for(i = 0; i < QGmax*(QGmax+1)/2; ++i){
    fprintf(out,"%lg %lg %lg ", X.x[i], X.y[i], X.z[i]);
    fprintf(out,"%lg %lg ", p[B->id*qt+i],
      sqrt(s[0][i]*s[0][i]+s[1][i]*s[1][i]+s[2][i]*s[2][i]));
    if(option("cylindrical")) // project vectors
      fprintf(out,"%lg %lg %lg %lg %lg\n", s[0][i], s[1][i], s[2][i],
        s1[0][i],s1[1][i]);
    else
      fprintf(out,"%lg %lg %lg \n", s[0][i], s[1][i], s[2][i]);
  }
  // dump connectivity
  for(cnt = 0,j = 0; j < QGmax-1; ++j){
    for(i = 0; i < QGmax-2-j; ++i){
      fprintf(out,"%d %d %d\n",cnt+i+1, cnt+i+2,cnt+qa-j+i+1);
      fprintf(out,"%d %d %d\n",cnt+qa-j+i+2,cnt+qa-j+i+1,cnt+i+2);
    }
    fprintf(out,"%d %d %d\n",cnt+qa-1-j,cnt+qa-j,cnt+2*qa-2*j-1);
    cnt += qa-j;
  }
      }
      else{
        fprintf(out,"ZONE T=\"%d\" ,I=%d, J=%d, F=POINT\n",counter,QGmax,QGmax);
        counter++;
/*  fprintf(out,"ZONE T=\"Elmt %d Face %d\",I=%d, J=%d, F=POINT\n",
    F->id+1,B->face+1,QGmax,QGmax);  */
  for(i = 0; i < QGmax*QGmax; ++i){
    fprintf(out,"%lg %lg %lg ", X.x[i], X.y[i], X.z[i]);
    fprintf(out,"%lg %lg ", p[B->id*qt+i],
      sqrt(s[0][i]*s[0][i]+s[1][i]*s[1][i]+s[2][i]*s[2][i]));
    if(option("cylindrical")) // project vectors
      fprintf(out,"%lg %lg %lg %lg %lg\n", s[0][i], s[1][i], s[2][i],
        s1[0][i],s1[1][i]);
    else
      fprintf(out,"%lg %lg %lg \n", s[0][i], s[1][i], s[2][i]);
  }
      }
    }

    free_dmatrix(Dm,0,0);  free_dmatrix(D,0,0);  free_dmatrix(s,0,0);
  free(wk);
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
      case 'a':
  option_set("Dump_all",1);
  break;
      case 'b':
  option_set("Body",1);
  break;
      case 'C':
  option_set("cylindrical",1);
  break;
      case 'c':
  option_set("Cylinder",1);
  break;
      case 'e':
  option_set("Equispaced",1);
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
      case 'S':
  if (*++argv[0])
    option_set("Surfaceid", atoi(*argv));
  else {
    option_set("Surfaceid", atoi(*++argv));
    argc--;
  }
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


static int Check_surface(Bndry *B, FileList f){
  Element *E = B->elmt;
  int val = 1;

  if(option("Cylinder"))
    if(B->elmt->curve)
      if(B->elmt->curve->type != T_Cylinder)
  val = 0;


  if(rnge){
    register int i;

    for(i = 0; i < E->Nverts; ++i){
      if(E->dim() == 3)
  if(!((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
       && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])
       && (E->vert[i].z > rnge->z[0])&&(E->vert[i].z < rnge->z[1])))
    val = 0;
  else
    if(!((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
         && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])))
      val = 0;
    }

  }
  else if(option("Surfaceid")){
    static int trip,*b2fac;
    /* check with .fro file to see if this surface is in designated region */


    if(!trip){
      register int i,n;
      int nfac, nvert,nbdy;
      int **faceid;
      FILE *fp;
      char buf[BUFSIZ];
      Bndry *B1;
      Curve *cur;

      sprintf(buf,"%s.fro",strtok(f.rea.name,"."));
      if(!(fp = fopen(buf,"r"))){
  fprintf(stderr,"Can not open %s\n",buf);
  exit(1);
      }

      /* load up data file with face numbers from felisa file */
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d%d",&nfac,&nvert);
      /* skip over vertices */
      for(i = 0; i < nvert; ++i)
  fgets(buf,BUFSIZ,fp);

      faceid = imatrix(0,nfac-1,0,3);
      /* read in face's with vertex id's */
      for(i = 0; i < nfac; ++i){
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%*d%d%d%d%d",
         faceid[i],faceid[i]+1,faceid[i]+2,faceid[i]+3);
  faceid[i][0]--;
  faceid[i][1]--;
  faceid[i][2]--;
      }

      /* count # of boundaries assuming this functions is called from
         first boudary (or at least the lowest one called) */
      /* also need to set id's since not previously set in this prog */
      for(nbdy=0,B1 =B; B1; B1 = B1->next)
  B1->id = nbdy++;
      b2fac = ivector(0,nbdy-1);
      izero(nbdy,b2fac,1);

      /*for each boundary find the felisa surface index */
      for(B1 = B; B1; B1 = B1->next){
  /* check though list to find first matching vertex */
  if(cur = B1->elmt->curve)
    for(; cur; cur = cur->next)
      if(B1->face == cur->face) break;
  if(cur)
    for(n = 0; n < nfac; ++n){
      for(i = 0; i < 3; ++i)
        if(cur->info.file.vert[0] == faceid[n][i])
    break;
      if(i < 3){
        if((cur->info.file.vert[1] == faceid[n][(i+1)%3])&&
     (cur->info.file.vert[2] == faceid[n][(i+2)%3])){
    b2fac[B1->id] = faceid[n][3];
    break;
        }

        if((cur->info.file.vert[1] == faceid[n][(i+2)%3])&&
     (cur->info.file.vert[2] == faceid[n][(i+1)%3])){
    b2fac[B1->id] = faceid[n][3];
    break;
        }
      }
    }
      }
      free_imatrix(faceid,0,0);
      trip = 1;
    }

    if(b2fac[B->id] != option("Surfaceid"))
      val = 0;
  }


  return val;
}

static void Get_Body(FILE *fp){
  register int i;
  char buf[BUFSIZ],*s = (char *)NULL;
  int  N;

  if(option("Range")){
    rnge = (Range *)malloc(sizeof(Range));
    rewind(fp);  /* search for range data */
    while(!strstr((s=fgets(buf,BUFSIZ,fp)),"Range"));

    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->x,rnge->x+1);
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->y,rnge->y+1);
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->z,rnge->z+1);
  }

  if(option("Body")){
    rewind(fp);/* search for body data  */
    while(s && !strstr((s=fgets(buf,BUFSIZ,fp)),"Body"));

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
