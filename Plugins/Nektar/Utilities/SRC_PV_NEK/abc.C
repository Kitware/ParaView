/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/abc.C,v $
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
#include <polylib.h>
#include <gen_utils.h>

int Nfields;

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "abc3d";
char *usage  = "abc3d:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-A      ... calculate wall surface area \n"
  "-s      ... smooth surface shear stress using local edges averages \n"
  "-e      ... dump as a single zone at equispaced points in FEdata \n"
  "-R      ... range data information. must have mesh file specified \n"
  "-V      ... write out variable viscosity \n"
  "-S #    ... only dump information from Felisa surface # \n"
  "-x #    ... move mesh in x direction by #\n"
  "-d #    ... read dump # (1 - no. dumps) from [.fld] file\n"
  "-n #    ... Number of mesh points.\n";

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

static void setup (FileList *f, Element_List **U, Field *fld);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Get_Body(FILE *fp);
static int  Check_range(Bndry *B, FileList f);
static void RunCoupled(Element_List **E, FileList f, int nfields);
static void GetNdForces(int npts, Bndry *Ubc, Element_List **EL, double **D,
      int **gval, double kinvis, double *xc, double
      *yc, double *zc, double **fg);
static void Write(FILE *out, int nel, int qa, Bndry *Ubc, int gpts,
      double *xc, double *yc, double *zc, double **fg, int *gval);
static void MakeAbqFile(FILE *out, Bndry *Ubc, int qa, int nel, int
      gpts, int **gval, int step, double t[], double
      *xc, double *yc, double *zc, double **fg, double
      **fg0, int *b1, int *b2, int *b3, int *b4);
static void InterpDisp(int qa, int nwf, int **gval, double *dxc, double
           *dyc, double *dzc, double **disp);
static void InterpPres(int qa, int qb, double *p, double *fr, double
           *area, double *xc, double *yc, double *zc);
static void NcrdTecOut(int qa, int nwf, int **gval, char job[], double **disp,
           double *xc, double *yc, double *zc);

main (int argc, char *argv[]){
  register       int i,k;
  int            dump=0,nfields;
  Field          fld;
  FileList       f;
  Element_List   **master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  memset(&fld, '\0', sizeof (Field));
  while (readField (f.in.fp, &fld)){
    dump++;
    if(iparam("Dump") == dump) break;
  }
  if (!dump) error_msg( no dumps read from fld file);

  if(f.mesh.name) Get_Body(f.mesh.fp);

  master = (Element_List **) malloc(((nfields = strlen(fld.type)))
            *sizeof(Element_List *));
  setup (&f, master, &fld);

  dparam_set("KINVIS", fld.kinvis);

  RunCoupled(master, f, nfields);

  return 0;
}

Gmap *gmap;

static void setup (FileList *f, Element_List **U, Field *fld){
  int i,j,k;
  int nfields = strlen(fld->type);
  extern Element_List *Mesh;

  ReadParams  (f->rea.fp);

  if((i=iparam("NORDER.REQ"))!=UNSET){
    iparam_set("LQUAD",i);
    iparam_set("MQUAD",i);
  }

  iparam_set("MODES",fld->lmax);

  /* Generate the list of elements */
  Mesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  U[0] = LocalMesh(Mesh,strtok(f->rea.name,"."));


  for(i = 1; i < nfields; ++i)
    U[i] = U[0]->gen_aux_field ('u');

  init_ortho_basis();

  for(i = 0; i < nfields; ++i){
    U[i]->fhead->type = fld->type[i];
    copyfield(fld,i,U[i]->fhead);
  }

  return;
}

static void RunCoupled(Element_List **EL, FileList f, int nfields){
  register int i,j,k,n;
  int      qa, qb, qc,zone=0;
  int      face,eid,cnt,nel,nwf,nfs,npts;
  Basis    *b;
  Bndry    *B,*Ubc;
  int      *gid,**gval,cnt1,l,gpts,id;
  double   wallarea = 0, *w = dvector(0,QGmax*QGmax-1);
  double   kinvis = dparam("KINVIS");
  double   **D = dmatrix(0,11,0,QGmax*QGmax*QGmax-1), **vis;
  Element  *U = EL[0]->fhead, *V = EL[1]->fhead, *W = EL[2]->fhead,
    *P = EL[3]->fhead, *E;
  double   **fg, **fg0, *xc, *yc, *zc, *dxc, *dyc, *dzc;
  int      *b1, *b2, *b3, *b4;
  Gmap     *gmap;
  FILE     *afp;
  int      step, Nstep;
  double   t[3];
  char     buf[BUFSIZ], fname[FILENAME_MAX], commline[FILENAME_MAX], job[FILENAME_MAX], c;

  sprintf(job, "%s", strtok(f.rea.name, "."));

  /* set up boundary structure */
  Ubc = ReadMeshBCs(f.rea.fp,*EL);

  gmap = GlobalNumScheme(EL[0],Ubc);

  gid = ivector(0,gmap->nvg + gmap->neg + gmap->nfg-1);
  ifill(gmap->nvg + gmap->neg + gmap->nfg,-1,gid,1);

  /* loop over elements and redefine edge and face mapping to
     start after each other as well as define vertex global numbering */

  npts = QGmax;
  cnt = 0;
  nwf = 0;
  nel = 0;
  for(B = Ubc; B; B = B->next)
    if(B->type == 'W'){
      face = B->face;
      E    = B->elmt;
      nfs  = E->Nfverts(B->face);

      for(i = 0; i < nfs; ++i)
  if(!(gid[E->vert[E->vnum(face,i)].gid]+1)){
    /* fprintf(stderr,"%d %d %d\n",E->vnum(face,i),E->vert[E->vnum(face,i)].gid,gid[E->vert[E->vnum(face,i)].gid]);*/
    gid[E->vert[E->vnum(face,i)].gid] = cnt++;
    }

      for(i = 0; i < nfs; ++i){
  E->edge[E->ednum(face,i)].gid += gmap->nvg;

  if(!(gid[E->edge[E->ednum(face,i)].gid]+1)){
    /*          fprintf(stderr,"%d %d %d\n",E->ednum(face,i),E->edge[E->ednum(face,i)].gid,gid[E->edge[E->ednum(face,i)].gid]);*/
    gid[E->edge[E->ednum(face,i)].gid]  = cnt;
    cnt += npts-2;
  }
      }

      E->face[face].gid += gmap->nvg + gmap->neg;
      if(!(gid[E->face[face].gid]+1)){
  /*  fprintf(stderr,"%d %d %d\n",face,E->face[face].gid,gid[E->face[face].gid]);*/

  gid[E->face[face].gid] = cnt;
  if(nfs == 3)
    cnt += (npts-3)*(npts-2)/2;
  else
    cnt += (npts-2)*(npts-2);
      }

      nwf++;

      if(nfs == 3)
  nel += (npts-1)*(npts-1);
      else
  nel += 2*(npts-1)*(npts-1);
    }

  gpts = cnt;
  gval = (int **)malloc(nwf*sizeof(int *));

  fg = dmatrix(0,2,0,gpts-1);
  dzero(gpts*3,fg[0],1);
  fg0 = dmatrix(0,2,0,gpts-1);
  dzero(gpts*3,fg0[0],1);

  xc   = dvector(0,gpts-1);
  yc   = dvector(0,gpts-1);
  zc   = dvector(0,gpts-1);
  dxc  = dvector(0,gpts-1);
  dyc  = dvector(0,gpts-1);
  dzc  = dvector(0,gpts-1);
  b1   = ivector(0,gpts-1);
  b2   = ivector(0,gpts-1);
  b3   = ivector(0,gpts-1);
  b4   = ivector(0,gpts-1);

  /* loop over edges and faces to define global number */
  n = 0;
  for(B = Ubc;B; B = B->next)
    if(B->type == 'W'){
      face = B->face;
      E    = B->elmt;
      nfs  = E->Nfverts(B->face);
      if(nfs == 3){
  gval[n] = ivector(0,npts*(npts+1)/2-1);

  /* number vertices */
  l = gid[E->vert[E->vnum(face,0)].gid];
  gval[n][0] = l;
  /* fprintf(stderr,"%d %d %d\n",n ,0, gval[n][0]);*/
  l = gid[E->vert[E->vnum(face,1)].gid];
  gval[n][npts-1] = l;
  /* fprintf(stderr,"%d %d %d\n",n,npts-1, gval[n][npts-1]);*/
  l = gid[E->vert[E->vnum(face,2)].gid];
  gval[n][npts*(npts+1)/2-1] = l;
  /* fprintf(stderr,"%d %d %d\n",n,npts*(npts+1)/2-1, gval[n][npts*(npts+1)/2-1]);*/

  /* number edges  */
  l = gid[E->edge[E->ednum(face,0)].gid];
  if(E->edge[E->ednum(face,0)].con) /* reverse ordering */
    for(i = 0; i < npts-2; ++i)
      gval[n][i+1] = l + npts-3 - i;
  else
    for(i = 0; i < npts-2; ++i)
      gval[n][i+1] = l + i;

  l = gid[E->edge[E->ednum(face,1)].gid];
  if(E->edge[E->ednum(face,1)].con) /* reverse ordering */
    for(i = 0,cnt = 2*npts-2; i < npts-2;++i,cnt+=npts-i-1)
      gval[n][cnt] = l + npts-3 - i;
  else
    for(i = 0,cnt = 2*npts-2; i < npts-2;++i,cnt+=npts-i-1)
      gval[n][cnt] = l + i;


  l = gid[E->edge[E->ednum(face,2)].gid];
  if(E->edge[E->ednum(face,2)].con) /* reverse ordering */
    for(i = 0,cnt = npts; i < npts-2;++i,cnt+=npts-i)
      gval[n][cnt] = l + npts-3 - i;
  else
    for(i = 0,cnt = npts; i < npts-2;++i,cnt+=npts-i)
      gval[n][cnt] = l + i;

  /* turn on all faces */
  l = gid[E->face[face].gid];
  for(i = 0,cnt = npts+1,cnt1=0; i < npts-3; ++i, cnt+=npts-i)
    for(j = 0; j < npts-3-i; ++j,++cnt1)
      gval[n][cnt + j] = l+cnt1;
      }
      else{
  gval[n] = ivector(0,npts*npts-1);
  fprintf(stderr,"Routine is not set up for square faces \n");
  exit(-1);
      }
      n++;
    }

  free(gid);

  /*  for  (i = 0; i < nwf; ++i){
    for (j = 0; j < npts*(npts+1)/2; ++j){
      fprintf(stderr,"%d\n" , gval[i][j]);
    }
  }*/

  Nstep=1;
  t[0] = 1/Nstep;
  t[2] = 0;

  for(step = 1; step <= Nstep; ++step){
    t[1] = t[2];
    t[2] = t[1] + t[0];
    fg0=fg;

    /* run fluid problem... */

    /* transform pressure field to nodal forces */
    GetNdForces(npts, Ubc, EL, D, gval, kinvis, &*xc, &*yc, &*zc, &*fg);

    /* make abaqus input file */
    sprintf(fname, "%s%d.inp", job, step);
    afp = fopen(fname, "w");
    MakeAbqFile(afp, Ubc, npts, nel, gpts, gval, step, t, xc, yc, zc, fg,
    fg0, &*b1, &*b2, &*b3, &*b4);
    fclose(afp);


  }


  free_dmatrix(D,0,0);/* free_dmatrix(fg,0,0);*/ free_dmatrix(fg0,0,0);

}

/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f){
  char  c;
  int   i;
  char  fname[FILENAME_MAX];

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }

  iparam_set("Nout", UNSET);
  iparam_set("Dump", UNSET);

  dparam_set("Xmove",0.0);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'A':
  option_set("WALLAREA",1);
  break;
      case 'd':
  if (*++argv[0])
    iparam_set("Dump", atoi(*argv));
  else {
    iparam_set("Dump", atoi(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'e':
  option_set("Equispaced",1);
  break;
      case 's':
  option_set("Smooth",1);
  break;
      case 'S':
  if (*++argv[0])
    option_set("Surfaceid", atoi(*argv));
  else {
    option_set("Surfaceid", atoi(*++argv));
    argc--;
  }
  break;
      case 'R':
  option_set("Range",1);
  break;
      case 'x':
  if (*++argv[0])
    dparam_set("Xmove", atof(*argv));
  else {
    dparam_set("Xmove", atof(*++argv));
    argc--;
  }
  if(option("verbose"))
    fprintf(stdout,"Mesh will be moved by %lf\n",dparam("Xmove"));
  (*argv)[1] = '\0';
  break;
      case 'V':
  option_set("Viscosity",1);
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



#ifdef EXCLUDE
static int Check_range(Bndry *B, FileList f){
  Element *E = B->elmt;
  if(rnge){
    register int i;

    for(i = 0; i < E->Nverts; ++i){
      if((E->vert[i].x < rnge->x[0])||(E->vert[i].x > rnge->x[1])) return 0;
      if((E->vert[i].y < rnge->y[0])||(E->vert[i].y > rnge->y[1])) return 0;
#if DIM == 3
      if((E->vert[i].z < rnge->z[0])||(E->vert[i].z > rnge->z[1])) return 0;
#endif
    }
  }
  return 1;
}
#else
static int Check_range(Bndry *B, FileList f){
  Element *E = B->elmt;
  if(rnge){
    register int i;

    for(i = 0; i < E->Nverts; ++i){
#if DIM == 3
      if((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
   && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])
         && (E->vert[i].z > rnge->z[0])&&(E->vert[i].z < rnge->z[1])) return 1;
#else
      if((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
   && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])) return 1;
#endif
    }
    return 0;
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

    if(b2fac[B->id] == option("Surfaceid"))
      return 1;
    else
      return 0;
  }
  else
    return 1;
}
#endif


static void Get_Body(FILE *fp){
  register int i;
  char buf[BUFSIZ],*s = (char *)NULL;
  int  N;

  if(option("Range")){
    rnge = (Range *)malloc(sizeof(Range));
    rewind(fp); /* search for range data */
    while(s && !strstr((s=fgets(buf,BUFSIZ,fp)),"Range"));

    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->x,rnge->x+1);
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->y,rnge->y+1);
#if DIM == 3
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->z,rnge->z+1);
#endif
  }

  if(option("Body")){
    rewind(fp); /* search for body data  */
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

static void GetNdForces(int  npts, Bndry *Ubc, Element_List **EL, double **D,
      int **gval, double kinvis, double *xc, double
      *yc, double *zc, double **fg){
  register int i, j, n;
  int      cnt, cnt1, id,nfs;
  Basis    *b;
  Coord    X;
  Bndry    *B;
  int      qa, qb;
  double   *area, x1, y1, z1, x2, y2, z2, f1, f2, f3;
  int      face, eid, tr;
  double   *wk = dvector(0,3*QGmax*QGmax*QGmax-1);
  double   *p = dvector(0,QGmax*QGmax-1);
  double   *fx, *fy, *fz;
  double   *ux = D[0], *uy = D[1], *uz = D[2];
  double   *vx = D[3], *vy = D[4], *vz = D[5];
  double   *wx = D[6], *wy = D[7], *wz = D[8];
  Element_List  *U = EL[0], *V = EL[1], *W = EL[2], *P = EL[3];
  Element  *E;
  Tri      T;

  area = dvector(0, (npts-1)*(npts-1)-1);

  X.x = D[9];
  X.y = D[10];
  X.z = D[11];

  n = 0;
  for(id = 1, B = Ubc; B ; B = B->next)
    if(B->type == 'W'){
      B->elmt->Surface_geofac(B);

      face = B->face;
      E    = B->elmt;
      eid  = B->elmt->id;

      qa = E->qa;
      qb = E->qb;

      nfs  = E->Nfverts(B->face);

      if(U->flist[eid]->state != 'p'){
  U->flist[eid]->Trans(U->flist[eid],J_to_Q);
  V->flist[eid]->Trans(V->flist[eid],J_to_Q);
  W->flist[eid]->Trans(W->flist[eid],J_to_Q);
      }

      U->flist[eid]->Grad_d(ux, uy, uz, 'a');
      V->flist[eid]->Grad_d(vx, vy, vz, 'a');
      W->flist[eid]->Grad_d(wx, wy, wz, 'a');

      /* get appropriate face values from D */
      for(i = 0; i < 9; ++i){
  E->GetFace(D[i],face,wk);
  E->InterpToFace1(face,wk,D[i]);
      }

      /* get pressure */
      P->flist[eid]->Trans(P->flist[eid],J_to_Q);
      E->GetFace(P->flist[eid]->h_3d[0][0],face,wk);
      E->InterpToFace1(face,wk,p);

      fx = U->flist[eid]->h_3d[0][0];
      fy = V->flist[eid]->h_3d[0][0];
      fz = W->flist[eid]->h_3d[0][0];

      if(E->curvX)
  for(i = 0; i < qa*qb; ++i){
    fx[i] = p[i]*B->nx.p[i] + kinvis*(2.0*ux[i]*B->nx.p[i] +
     (uy[i] + vx[i])*B->ny.p[i] + (uz[i] + wx[i])*B->nz.p[i]);
    fy[i] = p[i]*B->ny.p[i] + kinvis*(2.0*vy[i]*B->ny.p[i] +
     (uy[i] + vx[i])*B->nx.p[i] + (vz[i] + wy[i])*B->nz.p[i]);
    fz[i] = p[i]*B->nz.p[i] + kinvis*(2.0*wz[i]*B->nz.p[i] +
                 (uz[i] + wx[i])*B->nx.p[i] + (vz[i] + wy[i])*B->ny.p[i]);
  }
      else
  for(i = 0; i < qa*qb; ++i){
    fx[i] = p[i]*B->nx.d + kinvis*(2.0*ux[i]*B->nx.d +
      (uy[i] + vx[i])*B->ny.d + (uz[i] + wx[i])*B->nz.d);
    fy[i] = p[i]*B->ny.d + kinvis*(2.0*vy[i]*B->ny.d +
                  (uy[i] + vx[i])*B->nx.d + (vz[i] + wy[i])*B->nz.d);
    fz[i] = p[i]*B->nz.d + kinvis*(2.0*wz[i]*B->nz.d +
      (uz[i] + wx[i])*B->nx.d + (vz[i] + wy[i])*B->ny.d);
  }

      /*
      if(U[eid].curvX)
  for(i = 0; i < qa*qb; ++i){
          p[i] = 5.0E+03 / (1.0e+03 * 1.2E-01*1.2E-01);
    fx[i] = p[i]*B->nx.p[i];
    fy[i] = p[i]*B->ny.p[i];
    fz[i] = p[i]*B->nz.p[i];
  }
      else
  for(i = 0; i < qa*qb; ++i){
          p[i] = 5.0E+03 / (1.0e+03 * 1.2E-01*1.2E-01);
    fx[i] = p[i]*B->nx.d;
    fy[i] = p[i]*B->ny.d;
    fz[i] = p[i]*B->nz.d;
  }
      */

      E->coord(&X);

      T.qa = qa; // set up dummy T element for Interp_symmpts
      T.qb = qb;
      for(i = 9; i < 12; ++i){ /* interpolate coordinate to equispaced */
  E->GetFace       (D[i],face,wk);
  E->InterpToFace1 (face,wk,D[3]);
  if(nfs == 3)
    Interp_symmpts(&T,npts,D[3],D[i],'n');
  else
    fprintf(stderr,"Not set up for quad faces \n");

  /*InterpToEqui  (nfs,npts,npts,D[3],D[i]); changed second npts to npts-1 as other InterpToEqui call with (nfs,qa,qb... )*/
      }

      /* determine local areas and global coordinates */
      tr = 0;
      for(cnt = 0,j = 0; j < npts-1; ++j){
  for(i = 0; i < npts-2-j; ++i){
    x1 = X.x[cnt+i+1]    - X.x[cnt+i];
    x2 = X.x[cnt+npts-j+i] - X.x[cnt+i];
    y1 = X.y[cnt+i+1]    - X.y[cnt+i];
    y2 = X.y[cnt+npts-j+i] - X.y[cnt+i];
    z1 = X.z[cnt+i+1]    - X.z[cnt+i];
    z2 = X.z[cnt+npts-j+i] - X.z[cnt+i];

    area[tr] = 0.5*sqrt((y1*z2 - z1*y2)*(y1*z2 - z1*y2) +
            (z1*x2 - x1*z2)*(z1*x2 - x1*z2) +
            (x1*y2 - y1*x2)*(x1*y2 - y1*x2));
    ++tr;

    /* generate global coordinates */
    xc[gval[n][cnt+i]]        = X.x[cnt+i];
    xc[gval[n][cnt+i+1]]      = X.x[cnt+i+1];
    xc[gval[n][cnt+npts-j+i]] = X.x[cnt+npts-j+i];
    yc[gval[n][cnt+i]]        = X.y[cnt+i];
    yc[gval[n][cnt+i+1]]      = X.y[cnt+i+1];
    yc[gval[n][cnt+npts-j+i]] = X.y[cnt+npts-j+i];
    zc[gval[n][cnt+i]]        = X.z[cnt+i];
    zc[gval[n][cnt+i+1]]      = X.z[cnt+i+1];
    zc[gval[n][cnt+npts-j+i]] = X.z[cnt+npts-j+i];

    /* upper triangle */
    x1 = X.x[cnt+npts-j+i] - X.x[cnt+npts-j+i+1];
    x2 = X.x[cnt+i+1]    - X.x[cnt+npts-j+i+1];
    y1 = X.y[cnt+npts-j+i] - X.y[cnt+npts-j+i+1];
    y2 = X.y[cnt+i+1]    - X.y[cnt+npts-j+i+1];
    z1 = X.z[cnt+npts-j+i] - X.z[cnt+npts-j+i+1];
    z2 = X.z[cnt+i+1]    - X.z[cnt+npts-j+i+1];

    area[tr] = 0.5*sqrt((y1*z2 - z1*y2)*(y1*z2 - z1*y2) +
            (z1*x2 - x1*z2)*(z1*x2 - x1*z2) +
            (x1*y2 - y1*x2)*(x1*y2 - y1*x2));
    ++tr;

    /* generate global coordinates */
    xc[gval[n][cnt+npts-j+i+1]] = X.x[cnt+npts-j+i+1];
    xc[gval[n][cnt+npts-j+i]]   = X.x[cnt+npts-j+i];
    xc[gval[n][cnt+i+1]]      = X.x[cnt+i+1];
    yc[gval[n][cnt+npts-j+i+1]] = X.y[cnt+npts-j+i+1];
    yc[gval[n][cnt+npts-j+i]]   = X.y[cnt+npts-j+i];
    yc[gval[n][cnt+i+1]]      = X.y[cnt+i+1];
    zc[gval[n][cnt+npts-j+i+1]] = X.z[cnt+npts-j+i+1];
    zc[gval[n][cnt+npts-j+i]]   = X.z[cnt+npts-j+i];
    zc[gval[n][cnt+i+1]]      = X.z[cnt+i+1];
  }

  x1 = X.x[cnt+npts-j-1]     - X.x[cnt+npts-2-j];
  x2 = X.x[cnt+2*npts-2*j-2] - X.x[cnt+npts-2-j];
  y1 = X.y[cnt+npts-j-1]     - X.y[cnt+npts-2-j];
  y2 = X.y[cnt+2*npts-2*j-2] - X.y[cnt+npts-2-j];
  z1 = X.z[cnt+npts-j-1]     - X.z[cnt+npts-2-j];
  z2 = X.z[cnt+2*npts-2*j-2] - X.z[cnt+npts-2-j];

  area[tr] = 0.5*sqrt((y1*z2 - z1*y2)*(y1*z2 - z1*y2) +
          (z1*x2 - x1*z2)*(z1*x2 - x1*z2) +
          (x1*y2 - y1*x2)*(x1*y2 - y1*x2));
        ++tr;

  /* generate global coordinates */
  xc[gval[n][cnt+npts-2-j]]     = X.x[cnt+npts-2-j];
  xc[gval[n][cnt+npts-j-1]]     = X.x[cnt+npts-j-1];
  xc[gval[n][cnt+2*npts-2*j-2]] = X.x[cnt+2*npts-2*j-2];
  yc[gval[n][cnt+npts-2-j]]     = X.y[cnt+npts-2-j];
  yc[gval[n][cnt+npts-j-1]]     = X.y[cnt+npts-j-1];
  yc[gval[n][cnt+2*npts-2*j-2]] = X.y[cnt+2*npts-2*j-2];
  zc[gval[n][cnt+npts-2-j]]     = X.z[cnt+npts-2-j];
  zc[gval[n][cnt+npts-j-1]]     = X.z[cnt+npts-j-1];
  zc[gval[n][cnt+2*npts-2*j-2]] = X.z[cnt+2*npts-2*j-2];

  cnt += npts-j;
      }

      /* generate nodal forces */
      InterpPres(qa, qb, fx, D[0], area, xc, yc, zc);
      InterpPres(qa, qb, fy, D[1], area, xc, yc, zc);
      InterpPres(qa, qb, fz, D[2], area, xc, yc, zc);

      for(i = 0; i < npts*(npts+1)/2; ++i){
  fg[0][gval[n][i]] += D[0][i];
  fg[1][gval[n][i]] += D[1][i];
  fg[2][gval[n][i]] += D[2][i];
      }

      n++;
    }
  free(wk); free(p);
}

static void Write(FILE *out, int nel, int qa, Bndry *Ubc, int gpts,
      double *xc, double *yc, double *zc, double **fg, int
      *gval){
  register int i, j, n;
  Bndry    *B;
  int      cnt, id;

#ifdef TEC
  fprintf(out,"VARIABLES = x y z fx fy fz\n");
  fprintf(out,"ZONE N=%d, E=%d, F=FEPOINT, ET=TRIANGLE \n",gpts,nel);
#else
#ifdef ABQ
  fprintf(out,"**\n");
  fprintf(out,"** NODE DEFINITIONS\n");
  fprintf(out,"**\n");
#else
  fprintf(out,"*\n");
  fprintf(out,"*-------------------------- NODE DEFINITIONS --------------------------*\n");
  fprintf(out,"*\n");
#endif
#endif
  for(id=1,i = 0; i < gpts; ++i){
#ifdef TEC
    /* dump data */
    fprintf(out, "%l2g %l2g %l2g",xc[i],yc[i],zc[i]);
    for(j = 0; j < 3; ++j)
      fprintf(out," %l2g",fg[j][i]);
    fputc('\n',out);
#else
    /* dump coordinates */
#ifdef ABQ
    fprintf(out,"%8d, %20.13lE, %20.13lE, %20.13lE\n",id++,xc[i],yc[i],zc[i]);
#else
    fprintf(out,"%8d%5.1lf%20.13lE%20.13lE%20.13lE%5.1lf\n",id++,0.0,xc[i],yc[i],zc[i],0.0);
#endif
#endif
  }

#ifndef TEC
#ifndef ABQ
  fprintf(out,"*\n");
  fprintf(out,"*---------------- ELEMENT CARDS FOR SHELL ELEMENTS -----------------*\n");
  fprintf(out,"*\n");
#else
  fprintf(out,"**\n");
  fprintf(out,"** ELEMENT CARDS FOR SHELL ELEMENTS\n");
  fprintf(out,"**\n");
#endif
#endif
  /* dump connectivity */
  for(id=1,B = Ubc,n=0;B; B = B->next)
    if(B->type == 'W')
      if(B->face == 1 || B->face == 2){
        for(cnt = 0,j = 0; j < qa-1; ++j){
    for(i = 0; i < qa-2-j; ++i){
#ifdef TEC
      fprintf(out,"%d %d %d\n",gval[n+cnt+i]+1,
              gval[n+cnt+qa-j+i]+1, gval[n+cnt+i+1]+1);
#else
#ifdef ABQ
            fprintf(out,"%6d, %6d, %6d, %6d\n",id++,
                    gval[n+cnt+i]+1,gval[n+cnt+qa-j+i]+1,gval[n+cnt+i+1]+1);
#else
            fprintf(out,"  %6d    1  %6d  %6d  %6d  %6d\n",id++,gval[n+cnt+i]+1,
              gval[n+cnt+qa-j+i]+1,gval[n+cnt+i+1]+1,gval[n+cnt+i+1]+1);
      fprintf(out," 0.000E+00 0.000E+00 0.000E+00 0.000E+00\n");
#endif
#endif
#ifdef TEC
      fprintf(out,"%d %d %d\n",gval[n+cnt+qa-j+i+1]+1,
        gval[n+cnt+i+1]+1,gval[n+cnt+qa-j+i]+1);
#else
#ifdef ABQ
            fprintf(out,"%6d, %6d, %6d, %6d\n",id++,
                    gval[n+cnt+qa-j+i+1]+1,gval[n+cnt+i+1]+1,gval[n+cnt+qa-j+i]+1);
#else
            fprintf(out,"  %6d    1  %6d  %6d  %6d  %6d\n",id++,
              gval[n+cnt+qa-j+i+1]+1,
              gval[n+cnt+i+1]+1,gval[n+cnt+qa-j+i]+1,gval[n+cnt+qa-j+i]+1);
      fprintf(out," 0.000E+00 0.000E+00 0.000E+00 0.000E+00\n");
#endif
#endif
    }
#ifdef TEC
    fprintf(out,"%d %d %d\n",gval[n+cnt+qa-2-j]+1,
      gval[n+cnt+2*qa-2*j-2]+1,gval[n+cnt+qa-j-1]+1);
#else
#ifdef ABQ
          fprintf(out,"%6d, %6d, %6d, %6d\n",id++,
                  gval[n+cnt+qa-2-j]+1,gval[n+cnt+2*qa-2*j-2]+1,gval[n+cnt+qa-j-1]+1);
#else
          fprintf(out,"  %6d    1  %6d  %6d  %6d  %6d\n",id++,
            gval[n+cnt+qa-2-j]+1,
            gval[n+cnt+2*qa-2*j-2]+1,gval[n+cnt+qa-j-1]+1,gval[n+cnt+qa-j-1]+1);
    fprintf(out," 0.000E+00 0.000E+00 0.000E+00 0.000E+00\n");
#endif
#endif
    cnt += qa-j;
        }
        n += qa*(qa+1)/2;
      }
      else{
        for(cnt = 0,j = 0; j < qa-1; ++j){
    for(i = 0; i < qa-2-j; ++i){
#ifdef TEC
      fprintf(out,"%d %d %d\n",gval[n+cnt+i]+1, gval[n+cnt+i+1]+1,
              gval[n+cnt+qa-j+i]+1);
#else
#ifdef ABQ
            fprintf(out,"%6d, %6d, %6d, %6d\n",id++,
                    gval[n+cnt+i]+1,gval[n+cnt+i+1]+1,gval[n+cnt+qa-j+i]+1);
#else
            fprintf(out,"  %6d    1  %6d  %6d  %6d  %6d\n",id++,gval[n+cnt+i]+1,
              gval[n+cnt+i+1]+1,gval[n+cnt+qa-j+i]+1,gval[n+cnt+qa-j+i]+1);
      fprintf(out," 0.000E+00 0.000E+00 0.000E+00 0.000E+00\n");
#endif
#endif
#ifdef TEC
      fprintf(out,"%d %d %d\n",gval[n+cnt+qa-j+i+1]+1,
        gval[n+cnt+qa-j+i]+1,gval[n+cnt+i+1]+1);
#else
#ifdef ABQ
            fprintf(out,"%6d, %6d, %6d, %6d\n",id++,
                    gval[n+cnt+qa-j+i+1]+1,gval[n+cnt+qa-j+i]+1,gval[n+cnt+i+1]+1);
#else
            fprintf(out,"  %6d    1  %6d  %6d  %6d  %6d\n",id++,
              gval[n+cnt+qa-j+i+1]+1,gval[n+cnt+qa-j+i]+1,
              gval[n+cnt+i+1]+1,gval[n+cnt+i+1]+1);
      fprintf(out," 0.000E+00 0.000E+00 0.000E+00 0.000E+00\n");
#endif
#endif
    }
#ifdef TEC
    fprintf(out,"%d %d %d\n",gval[n+cnt+qa-2-j]+1,gval[n+cnt+qa-j-1]+1,
      gval[n+cnt+2*qa-2*j-2]+1);
#else
#ifdef ABQ
          fprintf(out,"%6d, %6d, %6d, %6d\n",id++,
                  gval[n+cnt+qa-2-j]+1,gval[n+cnt+qa-j-1]+1,gval[n+cnt+2*qa-2*j-2]+1);
#else
          fprintf(out,"  %6d    1  %6d  %6d  %6d  %6d\n",id++,
            gval[n+cnt+qa-2-j]+1,gval[n+cnt+qa-j-1]+1,
            gval[n+cnt+2*qa-2*j-2]+1,gval[n+cnt+2*qa-2*j-2]+1);
    fprintf(out," 0.000E+00 0.000E+00 0.000E+00 0.000E+00\n");
#endif
#endif
    cnt += qa-j;
        }
        n += qa*(qa+1)/2;
      }

#ifndef TEC
#ifndef ABQ
  fprintf(out,"*\n");
  fprintf(out,"*--------------------------NODAL LOADS-----------------------------------\n");
  fprintf(out,"*\n");

  for(id=1,i = 0; i < gpts; ++i){
    /* dump coordinates */
    for(j = 0; j < 3; ++j)
      fprintf(out,"%8d%5d%5d%10.3lE\n",id,j+1,1,fg[j][i]);
    id++;
  }
#else
  fprintf(out,"**\n");
  fprintf(out,"** NODAL LOADS\n");
  fprintf(out,"**\n");

  for(id=1,i = 0; i < gpts; ++i){
    /* dump coordinates */
    for(j = 0; j < 3; ++j)
      fprintf(out,"%8d, %5d, %10.3lE\n",id,j+1,fg[j][i]);
    id++;
  }
#endif
#endif
}

static void MakeAbqFile(FILE *out, Bndry *Ubc, int qa, int nel, int
      gpts, int **gval, int step, double t[], double
      *xc, double *yc, double *zc, double **fg, double
      **fg0, int *b1, int *b2, int *b3, int *b4){
  register int i, j, n;
  int      cnt, id, nb1, nb2, nb3, nb4;
  double   a, b;
  Bndry    *B;
  FILE     *mfp;
  char     fname[FILENAME_MAX], buf[BUFSIZ], *p;



  if(step == 1){
    nb1 = nb2 = nb3 = nb4 = 0;
    for(id = 1, i = 0; i < gpts; ++i){
#ifdef POIS
      if (fabs(zc[i] - 0) <= 1E-4){
        b1[nb1] = id;
        ++nb1;
      }
      if (fabs(zc[i] - 1) <= 1E-4){
        b2[nb2] = id;
        ++nb2;
      }
      if (fabs(xc[i] - -0.5) <= 1E-4 || fabs(xc[i] - 0.5) <= 1E-4){
        b3[nb3] = id;
        ++nb3;
      }
      else if (fabs(yc[i] - -0.5) <= 1E-4 || fabs(yc[i] - 0.5) <= 1E-4){
        b4[nb4] = id;
        ++nb4;
      }
#endif
#ifdef ANST
      if (fabs(xc[i] + yc[i] - sqrt(50)) <= 1E-4){
        b1[nb1] = id;
        ++nb1;
      }
      if (fabs(xc[i] - 3) <= 1E-4 && yc[i] <= 0.5){
        b2[nb2] = id;
        ++nb2;
      }
      if (fabs(xc[i] - -9) <= 1E-4){
        b3[nb3] = id;
        ++nb3;
      }
#endif
      ++id;
    }
    sprintf(fname, "Abaqstar");
  }
  else
    sprintf(fname, "Abaqrest");

  mfp = fopen(fname,"r");

if (mfp == NULL)
  {
  printf("can't open file\n");
  exit(1);
  }

  while(p=fgets(buf,BUFSIZ,mfp)){
    if(strstr(p,"NODE DEFINITIONS")){
      fprintf(out, "*NODE, NSET=NALL\n");
      for(id = 1, i = 0; i < gpts; ++i)
        fprintf(out,"%5d,%20.13lE,%20.13lE,%20.13lE\n",id++,xc[i],yc[i],zc[i]);
#ifdef POIS
      fprintf(out, "*NSET, NSET=INFLOW\n");
      for (i = 0; i < nb1; ++i)
        fprintf(out, "%5d,\n", b1[i]);
      fprintf(out, "*NSET, NSET=OUTFLOW\n");
      for (i = 0; i < nb2; ++i)
        fprintf(out, "%5d,\n", b2[i]);
      fprintf(out, "*NSET, NSET=HORIZONTAL\n");
      for (i = 0; i < nb3; ++i)
        fprintf(out, "%5d,\n", b3[i]);
      fprintf(out, "*NSET, NSET=VERTICAL\n");
      for (i = 0; i < nb4; ++i)
        fprintf(out, "%5d,\n", b4[i]);
#endif
#ifdef ANST
      fprintf(out, "*NSET, NSET=INFLOW\n");
      for (i = 0; i < nb1; ++i)
        fprintf(out, "%5d,\n", b1[i]);
      fprintf(out, "*NSET, NSET=OCCLUSION\n");
      for (i = 0; i < nb2; ++i)
        fprintf(out, "%5d,\n", b2[i]);
      fprintf(out, "*NSET, NSET=OUTFLOW\n");
      for (i = 0; i < nb3; ++i)
        fprintf(out, "%5d,\n", b3[i]);
      /* define local coordinate system for XSYMM boundary condition at inflow */
      fprintf(out, "*TRANSFORM, NSET=INFLOW\n");
      fprintf(out, "%20.13lE,%20.13lE,%20.13lE,%20.13lE,%20.13lE,%20.13lE\n",
              1/sqrt(2), 1/sqrt(2), 0E+00, -1/sqrt(2), 1/sqrt(2), 0E+00);
#endif
    }
    else if(strstr(p,"ELEMENT DEFINITIONS")){
      p=fgets(buf,BUFSIZ,mfp);
      fprintf(out, "*ELEMENT, %s", p);
      for(id = 1, B = Ubc, n = 0; B ; B = B->next)
        if(B->type == 'W')
          if(B->face == 1 || B->face == 2){
            for(cnt = 0, j = 0; j < qa-1; ++j){
        for(i = 0; i < qa-2-j; ++i){
                fprintf(out,"%5d,%5d,%5d,%5d\n",id++,
                        gval[n][cnt+i]+1,gval[n][cnt+qa-j+i]+1,
      gval[n][cnt+i+1]+1);
                fprintf(out,"%5d,%5d,%5d,%5d\n",id++,
                        gval[n][cnt+qa-j+i+1]+1,gval[n][cnt+i+1]+1,
      gval[n][cnt+qa-j+i]+1);
        }
              fprintf(out,"%5d,%5d,%5d,%5d\n",id++,
                      gval[n][cnt+qa-2-j]+1,gval[n][cnt+2*qa-2*j-2]+1,
          gval[n][cnt+qa-j-1]+1);
        cnt += qa-j;
            }
            n++;
          }
          else{
            for(cnt = 0, j = 0; j < qa-1; ++j){
        for(i = 0; i < qa-2-j; ++i){
                fprintf(out,"%5d,%5d,%5d,%5d\n",id++,
                        gval[n][cnt+i]+1,gval[n][cnt+i+1]+1,
      gval[n][cnt+qa-j+i]+1);
                fprintf(out,"%5d,%5d,%5d,%5d\n",id++,
                        gval[n][cnt+qa-j+i+1]+1,gval[n][cnt+qa-j+i]+1,
      gval[n][cnt+i+1]+1);
        }
              fprintf(out,"%5d,%5d,%5d,%5d\n",id++,
                      gval[n][cnt+qa-2-j]+1,gval[n][cnt+qa-j-1]+1,
          gval[n][cnt+2*qa-2*j-2]+1);
        cnt += qa-j;
            }
            n ++;
          }
    }
    else if(strstr(p,"BOUNDARY CONDITIONS")){
      fprintf(out, "*BOUNDARY\n");
#ifdef POIS
      fprintf(out, "  HORIZONTAL, YSYMM\n");
      fprintf(out, "  VERTICAL,   XSYMM\n");
      fprintf(out, "  INFLOW,     ZSYMM\n");
      fprintf(out, "  OUTFLOW,    ZSYMM\n");
#endif
#ifdef ANST
      fprintf(out, "  INFLOW,    XSYMM\n");
      fprintf(out, "  OCCLUSION, ENCASTRE\n");
      fprintf(out, "  OUTFLOW,   XSYMM\n");
#endif
    }
    else if(strstr(p,"STATIC")){
      fprintf(out, p);
      fprintf(out,"%7.5f, %7.5f, %7.5f, %7.5f\n", t[0], t[0], 0.001*t[0], t[0]);
    }
    else if(strstr(p,"NODAL LOADS")){
#ifdef ANST
      /* evaluate forces at inflow nodes in local coordinate system */
      for(i = 0; i < gpts; ++i)
        for(j = 0; j < nb1; ++j)
          if(i == b1[j]){
            a = fg[0][i];
            b = fg[1][i];
            fg[0][i] = 1/sqrt(2)*(a + b);
            fg[1][i] = 1/sqrt(2)*(b - a);
          }
#endif

      fprintf(out,"*AMPLITUDE, NAME=AMP%d, TIME=TOTAL TIME\n", step);
      fprintf(out,"%7.5f,%10.3lE,%7.5f,%10.3lE\n", t[1], 0.0, t[2], 1.0);
      fprintf(out,"*CLOAD, AMPLITUDE=AMP%d\n", step);
      for(id = 1, i = 0; i < gpts; ++i){
        for(j = 0; j < 3; ++j){
          fprintf(out,"%5d,%5d,%10.3lE\n", id, j+1, fg[j][i]);
  }
        id++;
      }

      /*
      for(id = 1, i = 0; i < gpts; ++i){
        for(j = 0; j < 3; ++j){
          fprintf(out,"*AMPLITUDE, NAME=AMP%d%d, TIME=TOTAL TIME\n", j+1, id);
          fprintf(out,"%7.5f,%10.3lE,%7.5f,%10.3lE\n", t[1], fg0[j][i], t[2], fg[j][i]);
  }
        id++;
      }
      for(id = 1, i = 0; i < gpts; ++i){
        for(j = 0; j < 3; ++j){
          fprintf(out,"*CLOAD, AMPLITUDE=AMP%d%d\n", j+1, id);
          fprintf(out,"%5d,%5d, 1\n", id, j+1);
  }
        id++;
      }
      */

    }
    else
      fprintf(out, p);
  }
  fclose(mfp);
}

static void InterpDisp(int qa, int nwf, int **gval, double *dxc, double
           *dyc, double *dzc, double **disp){
  register int i,j;
  double   *z, *wa, **equind, **gausnd, **dist, **locc, dtemp;
  int      id1, id2, n, cnt, **trnd, **trng, itemp;

  equind = dmatrix(0,qa*(qa+1)/2-1,0,1);
  gausnd = dmatrix(0,qa*qa-1,0,1);
  dist   = dmatrix(0,qa*qa-1,0,(qa-1)*(qa-1)-1);
  locc   = dmatrix(0,qa*qa-1,0,1);
  trng   = imatrix(0,qa*qa-1,0,(qa-1)*(qa-1)-1);
  trnd   = imatrix(0,(qa-1)*(qa-1)-1,0,2);

  getzw(qa,&z,&wa,'a');

  for(id1 = 0, j = 0; j < qa; ++j)
    for(i = 0; i < qa; ++i, ++id1){
      gausnd[id1][0] = (1.0 + z[i]) * (1.0 - z[j]) / 2.0 - 1.0;
      gausnd[id1][1] = z[j];
    }

  for(id2 = 0, j = 0; j < qa; ++j)
    for(i = 0; i < qa-j; ++i, ++id2){
      equind[id2][0] = -1.0 + (double)i*2.0/(double)(qa-1);
      equind[id2][1] = -1.0 + (double)j*2.0/(double)(qa-1);
    }

  for(id1 = 0; id1 < qa*qa; ++id1){
    for(id2 = 0, cnt = 0, j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-2-j; ++i){
        dist[id1][id2] = pow(equind[cnt+i][0]      - gausnd[id1][0], 2) +
                         pow(equind[cnt+i][1]      - gausnd[id1][1], 2) +
                         pow(equind[cnt+i+1][0]    - gausnd[id1][0], 2) +
                         pow(equind[cnt+i+1][1]    - gausnd[id1][1], 2) +
                         pow(equind[cnt+qa-j+i][0] - gausnd[id1][0], 2) +
                         pow(equind[cnt+qa-j+i][1] - gausnd[id1][1], 2);
        trng[id1][id2] = id2;
        trnd[id2][0] = cnt+i;
        trnd[id2][1] = cnt+i+1;
        trnd[id2][2] = cnt+qa-j+i;
        ++id2;
        dist[id1][id2] = pow(equind[cnt+qa-j+i+1][0] - gausnd[id1][0], 2) +
                         pow(equind[cnt+qa-j+i+1][1] - gausnd[id1][1], 2) +
                         pow(equind[cnt+qa-j+i][0]   - gausnd[id1][0], 2) +
                         pow(equind[cnt+qa-j+i][1]   - gausnd[id1][1], 2) +
                         pow(equind[cnt+i+1][0]      - gausnd[id1][0], 2) +
                         pow(equind[cnt+i+1][1]      - gausnd[id1][1], 2);
        trng[id1][id2] = id2;
        trnd[id2][0] = cnt+qa-j+i+1;
        trnd[id2][1] = cnt+qa-j+i;
        trnd[id2][2] = cnt+i+1;
        ++id2;
      }
      dist[id1][id2] = pow(equind[cnt+qa-2-j][0]     - gausnd[id1][0], 2) +
                       pow(equind[cnt+qa-2-j][1]     - gausnd[id1][1], 2) +
                       pow(equind[cnt+qa-j-1][0]     - gausnd[id1][0], 2) +
                       pow(equind[cnt+qa-j-1][1]     - gausnd[id1][1], 2) +
                       pow(equind[cnt+2*qa-2*j-2][0] - gausnd[id1][0], 2) +
                       pow(equind[cnt+2*qa-2*j-2][1] - gausnd[id1][1], 2);
      trng[id1][id2] = id2;
      trnd[id2][0] = cnt+qa-2-j;
      trnd[id2][1] = cnt+qa-j-1;
      trnd[id2][2] = cnt+2*qa-2*j-2;
      ++id2;

      cnt += qa-j;
    }
    for(j = 0; j < (qa-1)*(qa-1)-1; ++j)
      for(i = (qa-1)*(qa-1)-1; i > j; --i)
        if(dist[id1][i-1] > dist[id1][i]){
          dtemp = dist[id1][i-1];
          dist[id1][i-1] = dist[id1][i];
          dist[id1][i] = dtemp;
          itemp = trng[id1][i-1];
          trng[id1][i-1] = trng[id1][i];
          trng[id1][i] = itemp;
        }
    locc[id1][0] = fabs(gausnd[id1][0] - equind[trnd[trng[id1][0]][0]][0]) * (double)(qa-1) - 1.0;
    locc[id1][1] = fabs(gausnd[id1][1] - equind[trnd[trng[id1][0]][0]][1]) * (double)(qa-1) - 1.0;
  }

  for(i = 0, n = 0; i < nwf*qa*qa; i += qa*qa, ++n)
    for(id1 = 0; id1 < qa*qa; ++id1){
      disp[i + id1][0] =
      dxc[gval[n][trnd[trng[id1][0]][0]]] * -(locc[id1][1] + locc[id1][0])/2 +
      dxc[gval[n][trnd[trng[id1][0]][1]]] * (1 + locc[id1][0])/2 +
      dxc[gval[n][trnd[trng[id1][0]][2]]] * (1 + locc[id1][1])/2;
      disp[i + id1][1] =
      dyc[gval[n][trnd[trng[id1][0]][0]]] * -(locc[id1][1] + locc[id1][0])/2 +
      dyc[gval[n][trnd[trng[id1][0]][1]]] * (1 + locc[id1][0])/2 +
      dyc[gval[n][trnd[trng[id1][0]][2]]] * (1 + locc[id1][1])/2;
      disp[i + id1][2] =
      dzc[gval[n][trnd[trng[id1][0]][0]]] * -(locc[id1][1] + locc[id1][0])/2 +
      dzc[gval[n][trnd[trng[id1][0]][1]]] * (1 + locc[id1][0])/2 +
      dzc[gval[n][trnd[trng[id1][0]][2]]] * (1 + locc[id1][1])/2;
    }

  free_dmatrix(equind,0,0); free_dmatrix(gausnd,0,0);
  free_dmatrix(dist,0,0); free_dmatrix(locc,0,0); free(trng);
  free(trnd);
}

static void NcrdTecOut(int qa, int nwf, int **gval, char job[], double **disp,
           double *xc, double *yc, double *zc){
  register int i, j;
  int      id, cnt;
  double   **ocrd, **ncrd;
  FILE     *tfp;
  char     fname[FILENAME_MAX];

  sprintf(fname, "%s.plt", job);
  tfp = fopen(fname, "w");

  ocrd = dmatrix(0,nwf*qa*qa-1,0,2);
  ncrd = dmatrix(0,nwf*qa*qa-1,0,2);
  InterpDisp(qa, nwf, gval, xc, yc, zc, &*ocrd);

  for(i = 0; i < nwf*qa*qa; ++i){
    ncrd[i][0] = ocrd[i][0] + disp[i][0];
    ncrd[i][1] = ocrd[i][1] + disp[i][1];
    ncrd[i][2] = ocrd[i][2] + disp[i][2];
  }

  fprintf(tfp, "VARIABLES = x y z\n");
  fprintf(tfp, "ZONE N=%d, E=%d, F=FEPOINT, ET=QUADRILATERAL\n",
    nwf*qa*qa, nwf*(qa-1)*(qa-1));
  for(i = 0; i < nwf*qa*qa; i += qa*qa){
    for(id = 0; id < qa*qa; ++id)
      fprintf(tfp, "%l2g %l2g %l2g\n",
             ncrd[i + id][0], ncrd[i + id][1], ncrd[i +id][2]);
  }
  for(cnt = 0; cnt < nwf*qa*qa; cnt += qa*qa)
    for(j = 0; j < qa*(qa-1); j += qa)
      for(i = 1; i <= qa-1; ++i)
        fprintf(tfp, "%d %d %d %d\n", cnt+j+i, cnt+j+i+1,
    cnt+j+i+qa+1, cnt+j+i+qa);

  free_dmatrix(ocrd,0,0); free_dmatrix(ncrd,0,0);
}

static void InterpPres(int qa, int qb, double *p, double *fr, double
           *area, double *xc, double *yc, double *zc){
  register int i, j, k, l;
  double   *za, *zb, *wa, *wb, *zl, *wal, *wbl, **gausndl, **wghtndl,
           **mi, **ms, *pims, ni, nk, n1, n2, n3, area_fl, a[2], b[2], *r,
           **mat, *intp, hb, **equind, di;
  int      id1, cnt, **trnd, m, n, info, qal, qbl, tr, ntr;

  m      = qa*(qa+1)/2;
  ntr    = (qa-1)*(qa-1);
  mi     = dmatrix(0,2,0,2);
  ms     = dmatrix(0,m-1,0,m-1);
  equind = dmatrix(0,m-1,0,1);
  trnd   = imatrix(0,ntr-1,0,2);
  r      = dvector(0,m-1);

  /* set up solid mass matrix (linear interpolation) */
  qal     = 3;
  qbl     = 2;
  gausndl = dmatrix(0,qal*qbl-1,0,1);
  wghtndl = dmatrix(0,qal*qbl-1,0,1);

  getzw(qbl,&zl,&wbl,'b');
  getzw(qal,&zl,&wal,'a');

  for(id1 = 0, j = 0; j < qbl; ++j)
    for(i = 0; i < qal; ++i, ++id1){
      gausndl[id1][0] = (1.0 + zl[i]) * (1.0 - zl[j]) / 2.0 - 1.0;
      gausndl[id1][1] = zl[j];
      wghtndl[id1][0] = wal[i];
      wghtndl[id1][1] = wbl[j];
    }

  for(k = 0; k < 3; ++k){
    for(i = 0; i < 3; ++i){
      mi[k][i] = 0;
      for(id1 = 0; id1 < qal*qbl; ++id1){
  if(i == 0)
    ni = -(gausndl[id1][1] + gausndl[id1][0])/2;
  else if(i == 1)
    ni = (1 + gausndl[id1][0])/2;
  else if(i == 2)
    ni = (1 + gausndl[id1][1])/2;
        if(k == 0)
          nk = -(gausndl[id1][1] + gausndl[id1][0])/2;
        else if(k == 1)
          nk = (1 + gausndl[id1][0])/2;
        else if(k == 2)
          nk = (1 + gausndl[id1][1])/2;
        mi[k][i] += ni * nk * wghtndl[id1][0] * wghtndl[id1][1];
      }
    }
  }

  for(tr = 0, cnt = 0, j = 0; j < qa-1; ++j){
    for(i = 0; i < qa-2-j; ++i){
      trnd[tr][0] = cnt+i;
      trnd[tr][1] = cnt+i+1;
      trnd[tr][2] = cnt+qa-j+i;
      ++tr;
      trnd[tr][0] = cnt+qa-j+i+1;
      trnd[tr][1] = cnt+qa-j+i;
      trnd[tr][2] = cnt+i+1;
      ++tr;
    }
    trnd[tr][0] = cnt+qa-2-j;
    trnd[tr][1] = cnt+qa-j-1;
    trnd[tr][2] = cnt+2*qa-2*j-2;
    ++tr;
    cnt += qa-j;
  }

  for(k = 0; k < m; ++k)
    for(i = 0; i < m; ++i)
      ms[k][i] = 0;

  for(tr = 0; tr < ntr; ++tr)
    for(k = 0; k < 3; ++k)
      for(i = 0; i < 3; ++i)
  ms[trnd[tr][k]][trnd[tr][i]] += mi[k][i] * area[tr]/2;

  for(id1 = 0, j = 0; j < qa; ++j)
    for(i = 0; i < qa-j; ++i, ++id1){
      equind[id1][0] = -1.0 + (double)i*2.0/(double)(qa-1);
      equind[id1][1] = -1.0 + (double)j*2.0/(double)(qa-1);
    }

  mat = dmatrix(0,tr*qal*qbl-1,0,qa*qb-1);
  intp = dvector(0,tr*qal*qbl-1);

  getzw(qa,&za,&wa,'a');
  getzw(qb,&zb,&wb,'b');

  for(id1 = 0, tr = 0; tr < ntr; ++tr)
    for(j = 0; j < qbl; ++j)
      for(i = 0; i < qal; ++i, ++id1){

  if(equind[trnd[tr][1]][0] > equind[trnd[tr][0]][0]){
    a[0] = equind[trnd[tr][0]][0] + (1.0 + zl[i]) * (1.0 - zl[j])
      / 2 / (double)(qa - 1);
    a[1] = equind[trnd[tr][0]][1] + (zl[j] + 1) / (double)(qa - 1);
  }
  else if(equind[trnd[tr][1]][0] < equind[trnd[tr][0]][0]){
    a[0] = equind[trnd[tr][0]][0] - (1.0 + zl[i]) * (1.0 - zl[j])
      / 2 / (double)(qa - 1);
    a[1] = equind[trnd[tr][0]][1] - (zl[j] + 1) / (double)(qa - 1);
  }

        if(a[1] != 1)
          b[0] = 2*(1+a[0])/(1-a[1])-1;
  else
    b[0] = -1.0;
        b[1] = a[1];

  /* fillqbasis */
  for(k = 0; k < qb; ++k){
    hb = hgrj(k,b[1],zb,qb,1.0,0.0);
    for(l = 0; l < qa; ++l)
      mat[id1][k*qa + l] = hb*hglj(l,b[0],za,qa,0.0,0.0);
  }
      }

  dgemv('T',qa*qb,tr*qal*qbl,1.0,*mat,qa*qb,p,1,0.0,intp,1);

  for(i = 0; i < m; ++i){
    r[i] = 0;
    for(tr = 0; tr < ntr; ++tr){
      if(i == trnd[tr][0])
        for(id1 = 0; id1 < qal*qbl; ++id1){
    ni = -(gausndl[id1][1] + gausndl[id1][0])/2;
    r[i] += ni * wghtndl[id1][0] * wghtndl[id1][1] * area[tr]
      / 2 * intp[tr*qal*qbl+id1];
  }
      else if(i == trnd[tr][1])
        for(id1 = 0; id1 < qal*qbl; ++id1){
    ni = (1 + gausndl[id1][0])/2;
    r[i] += ni * wghtndl[id1][0] * wghtndl[id1][1] * area[tr]
      / 2 * intp[tr*qal*qbl+id1];
  }
      else if(i == trnd[tr][2])
        for(id1 = 0; id1 < qal*qbl; ++id1){
    ni = (1 + gausndl[id1][1])/2;
    r[i] += ni * wghtndl[id1][0] * wghtndl[id1][1] * area[tr]
      / 2 * intp[tr*qal*qbl+id1];
  }
      else r[i] += 0;
    }
  }

  pims = dvector(0,m*(m+1)/2-1);
  PackMatrix(ms,m,pims,m);

  dpptrf('L', m, pims, info);
  if(info) fprintf(stderr,"solving M p = r: dpptrf info not zero (%d)\n",info);
  dpptrs('L', m, 1, pims, r, m, info);
  if(info) fprintf(stderr,"solving M p = r: dpptrs info not zero (%d)\n",info);

  /* calculate statically equivalent forces */
  for(i = 0; i < m; ++i)
    fr[i] = 0;

  for(tr = 0; tr < ntr; ++tr){
    for(id1 = 0; id1 < qal*qbl; ++id1){
      n1 = -(gausndl[id1][1] + gausndl[id1][0])/2;
      n2 = (1 + gausndl[id1][0])/2;
      n3 = (1 + gausndl[id1][1])/2;
      fr[trnd[tr][0]] += n1 * (n1 * r[trnd[tr][0]] + n2 * r[trnd[tr][1]] +
             n3 * r[trnd[tr][2]]) *
                   wghtndl[id1][0] * wghtndl[id1][1] * area[tr] / 2 ;
      fr[trnd[tr][1]] += n2 * (n1 * r[trnd[tr][0]] + n2 * r[trnd[tr][1]] +
             n3 * r[trnd[tr][2]]) *
                   wghtndl[id1][0] * wghtndl[id1][1] * area[tr] / 2 ;
      fr[trnd[tr][2]] += n3 * (n1 * r[trnd[tr][0]] + n2 * r[trnd[tr][1]] +
             n3 * r[trnd[tr][2]]) *
                   wghtndl[id1][0] * wghtndl[id1][1] * area[tr] / 2 ;
    }
  }

  free_dmatrix(mi,0,0);
  free_dmatrix(ms,0,0);
  free_dmatrix(equind,0,0);
  free_imatrix(trnd,0,0);
  free_dmatrix(gausndl,0,0);
  free_dmatrix(wghtndl,0,0);
  free_dmatrix(mat,0,0);
  free_dvector(intp,0);
  free_dvector(pims,0);
}
