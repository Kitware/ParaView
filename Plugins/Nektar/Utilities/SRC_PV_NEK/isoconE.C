
/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/isoconE.C,v $
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

int DIM;

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "isocont";
char *usage  = "isocont:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-l #    ... Value of contour to be extracted (defaut 0) \n"
  "-f #    ... location of field to be extracted \n"
#if DIM == 2
  "-n #    ... Number of mesh points. Default is 15\n";
#else
  "-n #    ... Number of mesh points.\n";
#endif

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

static int  setup (FileList *f, Element_List **U, int *nftot);
static void ReadCopyField (FileList *f, Element_List **U);
static void Get_Body(FILE *fp);
static void dump_faces(FILE *out, Element_List **E, Coord X, int nel, int zone,
           int nfields);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Isocontour(Element_List **E, FILE *out, int nfields);

void solve(Element_List *U, Element_List *Uf,
     Bndry *Ubc, Bsystem *Ubsys,SolveType Stype)
{
  SetBCs (U,Ubc,Ubsys);
  Solve  (U,Uf,Ubc,Ubsys,Stype);
}


main (int argc, char *argv[])
{
  register int  i,k;
  int       dump=0,nfields,nftot;
  FileList  f;
  Element_List **master;

  init_comm(&argc, &argv);

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  master  = (Element_List **) malloc(_MAX_FIELDS*sizeof(Element_List *));
  nfields = setup (&f, master, &nftot);

  ReadCopyField(&f,master);

  DIM = master[0]->fhead->dim();

  Isocontour(master,f.out.fp, nfields);

  exit_comm();

  return 0;
}

/* Inner project w.r.t. orthogonal modes */

void OrthoInnerProduct(Element_List *EL, Element_List *ELf){
  Element *U, *Uf;
  if(EL->fhead->dim() == 2)
    for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
      Uf->Ofwd(U->h[0], Uf->h[0], Uf->lmax);
  else
    for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
      Uf->Ofwd(U->h_3d[0][0], Uf->h_3d[0][0], Uf->lmax);
}

void OrthoJTransBwd(Element_List *EL, Element_List *ELf){
  Element *U, *Uf;

 if(EL->fhead->dim() == 2)
   for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
     Uf->Obwd(U->h[0], Uf->h[0], Uf->lmax);
 else
   for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
     Uf->Obwd(U->h_3d[0][0], Uf->h_3d[0][0], Uf->lmax);
}


void init_ortho_basis(void);

Gmap *gmap;

int  setup (FileList *f, Element_List **U, int *nft){
  int    i,shuff;
  int    nfields,nftot;
  Field  fld;
  extern Element_List *Mesh;

  memset(&fld, '\0', sizeof (Field));
  readHeader(f->in.fp,&fld,&shuff);
  rewind(f->in.fp);

  nfields = strlen(fld.type);
  nftot   = nfields;
  nftot  += (fld.dim == 3) ? 4:2;

  ReadParams  (f->rea.fp);
  if((i=iparam("NORDER.REQ"))!=UNSET){
    iparam_set("LQUAD",i+1);
    iparam_set("MQUAD",i+1);
  }
  else if(option("Qpts")){
    iparam_set("LQUAD",fld.lmax+1);
    iparam_set("MQUAD",fld.lmax+1);
  }
  else{
    iparam_set("LQUAD",fld.lmax+1);
    iparam_set("MQUAD",fld.lmax+1);
  }

  iparam_set("MODES",iparam("LQUAD")-1);

  /* Generate the list of elements */
  Mesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  gmap = GlobalNumScheme(Mesh, (Bndry *)NULL);
  U[0] = LocalMesh(Mesh,strtok(f->rea.name,"."));

  init_ortho_basis();

  if(f->mesh.name) Get_Body(f->mesh.fp);

  for(i = 1; i < nfields; ++i){
    U[i] = U[0]->gen_aux_field('u');
    U[i]->fhead->type = fld.type[i];
  }

  for(i = nfields; i < nftot; ++i){
    U[i] = U[0]->gen_aux_field('u');
    U[i]->fhead->type = 'W';
  }

  freeField(&fld);

  nft[0] = nftot;
  return nfields;
}


static  void ReadCopyField (FileList *f, Element_List **U){
  int i,dump;
  int nfields;
  Field fld;

  memset(&fld, '\0', sizeof (Field));

  dump = readField (f->in.fp, &fld);
  if (!dump) error_msg(Restart: no dumps read from restart file);
  rewind (f->in.fp);

  nfields = strlen(fld.type);

  for(i = 0; i < nfields; ++i)
    copyfield(&fld,i,U[i]->fhead);

  freeField(&fld);
}

static int Check_range(Element *E);





static void Dump_Contour(int nnodes, double *x, double *y, double *z, double *c,
                         int nelmts, int **tets, double val, int zoneNum);




static void Isocontour(Element_List **E, FILE *out, int nfields){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt,nnodes,sum, *interior, **tet;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  int      fieldid;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;

  fieldid = iparam("FIELDID");

  if(fieldid >= nfields){
    fprintf(stderr,"field id was greater than number of fields setting to "
      "nfields-1\n");
    fieldid = nfields-1;
  }

  for(i=0;i<nfields;++i)
    E[i]->Trans(E[i],J_to_Q);

  interior = ivector(0,E[0]->nel-1);

  if(E[0]->fhead->dim() == 3){
    double ***num;

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);

    ROOTONLY{

      fprintf(stdout,"TITLE = \"Field: %c ; Level: %10.6f\"\n",
              E[fieldid]->fhead->type,dparam("CONTOURVAL"));
      fprintf(stdout,"VARIABLES = \"x\" \"y\" \"z\"");
      fprintf(stdout," \"%c\"", E[fieldid]->fhead->type);
      fputc('\n',out);
    }

    for(k = 0,n=i=0; k < E[0]->nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
        qa = F->qa;
        i += qa*(qa+1)*(qa+2)/6;
        n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    fprintf(stdout,"qa is %d\n",qa);


    switch(F->identify()){
    case Nek_Tet:
      /* numbering array */

      tet = imatrix(0,(qa-1)*(qa-1)*(qa-1)-1,0,3);
      num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

      fprintf(stdout,"welcome to tet\n");
      nnodes = ((qa*(qa+1)*(qa+2))/6);

      for(cnt = 1, k = 0; k < qa; ++k)
        for(j = 0; j < qa-k; ++j)
          for(i = 0; i < qa-k-j; ++i, ++cnt)
            num[i][j][k] = cnt;


      for(k=0,cnt=0; k < qa-1; ++k)
        for(j = 0; j < qa-1-k; ++j){
          for(i = 0; i < qa-2-k-j; ++i){
            tet[cnt  ][0] = num[i][j][k];
            tet[cnt  ][1] = num[i+1][j][k];
            tet[cnt  ][2] = num[i][j+1][k];
            tet[cnt++][3] = num[i][j][k+1];

            tet[cnt  ][0] = num[i+1][j][k];
            tet[cnt  ][1] = num[i][j+1][k];
            tet[cnt  ][2] = num[i][j][k+1];
            tet[cnt++][3] = num[i+1][j][k+1];

            tet[cnt  ][0] = num[i+1][j][k+1];
            tet[cnt  ][1] = num[i][j][k+1];
            tet[cnt  ][2] = num[i][j+1][k+1];
            tet[cnt++][3] = num[i][j+1][k];

            tet[cnt  ][0] = num[i+1][j+1][k];
            tet[cnt  ][1] = num[i][j+1][k];
            tet[cnt  ][2] = num[i+1][j][k];
            tet[cnt++][3] = num[i+1][j][k+1];

            tet[cnt  ][0] = num[i+1][j+1][k];
            tet[cnt  ][1] = num[i][j+1][k];
            tet[cnt  ][2] = num[i+1][j][k+1];
            tet[cnt++][3] = num[i][j+1][k+1];

            if(i < qa-3-k-j){
              tet[cnt  ][0] = num[i][j+1][k+1];
              tet[cnt  ][1] = num[i+1][j+1][k+1];
              tet[cnt  ][2] = num[i+1][j][k+1];
              tet[cnt++][3] = num[i+1][j+1][k];
            }
          }
          tet[cnt  ][0] = num[qa-2-k-j][j][k];
          tet[cnt  ][1] = num[qa-1-k-j][j][k];
          tet[cnt  ][2] = num[qa-2-k-j][j+1][k];
          tet[cnt++][3] = num[qa-2-k-j][j][k+1];
        }
      break;


    case Nek_Prism:

      tet = imatrix(0,(((qa*qa)-(2*qa)+1)*(qa-1)*3)-1,0,3);
      num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

      fprintf (stdout,"welcome to prism\n");
      sum = 0;
      for (i=qa;i>0;i--){
        sum += i;
      }
      nnodes = qa*sum;

      for (cnt=0, k=0; k<qa; k++){
        for (j=0; j<qa; j++){
          for (i=0; i<qa-k; i++){
            num[i][j][k] = ++cnt;
          }
        }
      }

      for (cnt=0, k=0; k<qa-1; k++){
        for (j=0; j<qa-1; j++){
          for (i=0; i<qa-k-1; i++){
            tet[cnt  ][0]=num[i]  [j]  [k]  ;
            tet[cnt  ][1]=num[i+1][j]  [k]  ;
            tet[cnt  ][2]=num[i]  [j]  [k+1];
            tet[cnt++][3]=num[i]  [j+1][k]  ;

            tet[cnt  ][0]=num[i+1][j]  [k]  ;
            tet[cnt  ][1]=num[i]  [j]  [k+1];
            tet[cnt  ][2]=num[i]  [j+1][k]  ;
            tet[cnt++][3]=num[i+1][j+1][k]  ;

            tet[cnt  ][0]=num[i]  [j]  [k+1];
            tet[cnt  ][1]=num[i]  [j+1][k]  ;
            tet[cnt  ][2]=num[i+1][j+1][k]  ;
            tet[cnt++][3]=num[i]  [j+1][k+1];

            if (i<qa-k-2){
              tet[cnt  ][0]=num[i+1][j]  [k]  ;
              tet[cnt  ][1]=num[i+1][j]  [k+1];
              tet[cnt  ][2]=num[i]  [j]  [k+1];
              tet[cnt++][3]=num[i+1][j+1][k]  ;

              tet[cnt  ][0]=num[i+1][j]  [k+1];
              tet[cnt  ][1]=num[i]  [j]  [k+1];
              tet[cnt  ][2]=num[i+1][j+1][k]  ;
              tet[cnt++][3]=num[i+1][j+1][k+1];

              tet[cnt  ][0]=num[i]  [j]  [k+1];
              tet[cnt  ][1]=num[i+1][j+1][k]  ;
              tet[cnt  ][2]=num[i+1][j+1][k+1];
              tet[cnt++][3]=num[i]  [j+1][k+1];
            }
          }
        }
      }
      break;

    default:
      fprintf(stderr,"Isocontour is not set up for this element type \n");
      exit(1);
      break;
    }

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
        qa = F->qa;
        F->coord(&X);
        ntot = Interp_symmpts(F,F->qa,X.x,X.x,'p');
        ntot = Interp_symmpts(F,F->qa,X.y,X.y,'p');
        ntot = Interp_symmpts(F,F->qa,X.z,X.z,'p');
        F = E[fieldid]->flist[k];
        Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'p');

        switch(F->identify()){
        case Nek_Tet:
          Dump_Contour(nnodes,
                       X.x,X.y,X.z,F->h_3d[0][0],(qa-1)*(qa-1)*(qa-1),tet,
                       dparam("CONTOURVAL"),(k+1));
          break;
        case Nek_Prism:
          Dump_Contour(nnodes,
                       X.x,X.y,X.z,F->h_3d[0][0],(((qa*qa)-(2*qa)+1)*(qa-1)*3),tet,
                       dparam("CONTOURVAL"),(k+1));
          break;
        }
      }
    }

    free(X.x); free(X.y); free(X.z);
    free_dtarray(num,0,0,0);
    free_imatrix(tet,0,0);
  }
  else{
    fprintf(stderr,"isocon not set up in 2d\n");
  }

}





static void Dump_Contour(int nnodes, double *x, double *y, double *z, double *c,
       int nelmts, int **tets, double val, int zoneNum)
{
  int i;

  fprintf(stdout,"\nZONE T=\"ZONE %d (%4.2f)\"\n",zoneNum, val);
  fprintf(stdout,"N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n", nnodes, nelmts);

  for (i=0;i<nnodes;i++){
    fprintf(stdout,"%f %f %f %f\n",x[i],y[i],z[i],c[i]);
  }

  for (i=0;i<nelmts;i++){
    fprintf(stdout,"%3d %3d %3d %3d\n",tets[i][0],tets[i][1],tets[i][2],tets[i][3]);
  }
}






#ifdef EXCLUDE
static int Check_range(Element *E){
  if(rnge){
    register int i;

    for(i = 0; i < Nvert; ++i){
      if((E->vert[i].x < rnge->x[0])||(E->vert[i].x > rnge->x[1])) return 0;
      if((E->vert[i].y < rnge->y[0])||(E->vert[i].y > rnge->y[1])) return 0;
      if(DIM == 3)
  if((E->vert[i].z < rnge->z[0])||(E->vert[i].z > rnge->z[1])) return 0;
    }
  }
  return 1;
}
#else
static int Check_range(Element *E){
  if(rnge){
    register int i;

    for(i = 0; i < E->Nverts; ++i){
      if(DIM == 3){
  if((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
     && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])
     && (E->vert[i].z > rnge->z[0])&&(E->vert[i].z < rnge->z[1])) return 1;
      }
      else
  if((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
     && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])) return 1;
    }
    return 0;
  }
  else
    return 1;
}
#endif

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

  iparam_set("Nout", UNSET);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'l':
  if (*++argv[0])
    dparam_set("CONTOURVAL", atof(*argv));
  else {
    dparam_set("CONTOURVAL", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'f':
  if (*++argv[0])
    iparam_set("FIELDID", atoi(*argv));
  else {
    iparam_set("FIELDID", atoi(*++argv));
    argc--;
  }
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

static void Get_Body(FILE *fp){
  register int i;
  char buf[BUFSIZ],*s;
  int  N;

  if(option("Range")){
    rnge = (Range *)malloc(sizeof(Range));
    rewind(fp);  /* search for range data */
    while(s && !strstr((s=fgets(buf,BUFSIZ,fp)),"Range"));

    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->x,rnge->x+1);
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->y,rnge->y+1);
    if(DIM == 3){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf",rnge->z,rnge->z+1);
    }
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

static void dump_faces(FILE *out, Element_List **U, Coord X, int nel,
           int zone, int nfields){
  Element *F;
  int qa, qb, qc,fac, cnt;
  register int i,j,k,l,n;
  double **tmp;
  int data_skip = 0;
  int size_skip = 0;

  tmp  = dmatrix(0, nfields, 0, QGmax*QGmax*QGmax-1);

  k = 0;
  for(l=0;l<bdy.N;++l){
    F = U[0]->flist[bdy.elmt[l]];

    fac = bdy.faceid[l];

    if(F->identify() == Nek_Tet)
      switch(fac){
      case 0:
  qa = F->qa;      qb = F->qb;
  break;
      case 1:
  qa = F->qa;      qb = F->qc;
  break;
      case 2: case 3:
  qa = F->qb;      qb = F->qc;
  break;
      }
    else if(F->identify() == Nek_Hex){
      switch(fac){
      case 0: case 5:
  qa = F->qa;      qb = F->qb;
  break;
      case 1: case 3:
  qa = F->qa;      qb = F->qc;
  break;
      case 2: case 4:
  qa = F->qb;      qb = F->qc;
  break;
      }
    }
    else if(F->identify() == Nek_Prism){
      switch(fac){
      case 0:
  qa = F->qa;      qb = F->qb;
  break;
      case 1: case 3:
  qa = F->qa;      qb = F->qc;
  break;
      case 2: case 4:
  qa = F->qb;      qb = F->qc;
  break;
      }
    }

    cnt = F->qa*F->qb;

    F->GetFaceCoord(fac, &X);

    fprintf(out,"ZONE T=\"ELEMENT %d\", I=%d, J=%d, K=%d, F=POINT\n",
      ++zone,qa,qb,1);

    for(n = 0; n < nfields; ++n)
      U[n]->flist[F->id]->GetFace(**U[n]->flist[F->id]->h_3d, fac, tmp[n]);

    for(i = 0; i < cnt; ++i){
      fprintf(out,"%lg %lg %lg ",X.x[i], X.y[i], X.z[i]);
      for(n = 0; n < nfields; ++n)
  fprintf(out,"%lg ",tmp[n][i]);
      fputc('\n',out);
    }
    ++k;
  }
  free_dmatrix(tmp,0,0);
}


void  set_elmt_edges(Element_List *EL){
  register int i,j,k,c;
  Edge    *e;
  int     nel = EL->nel,edgtot;
  double  tot,*wa,*wb,*wk,w;
  Element *E;

  wk = dvector(0,QGmax-1);


  for(k = 0; k < nel; ++k)   /* set up number of quadrature points */
    for(i = 0,e=EL->flist[k]->edge; i < EL->flist[k]->Nedges; ++i)
      e[i].qedg = EL->flist[e[i].eid]->lmax;

  edgtot = 0;
  for(k = 0; k < nel; ++k){
    E = EL->flist[k];
    getzw(E->qa,&wa,&wa,'a'); // ok

    if(E->identify() == Nek_Tri)
      getzw(E->qb,&wb,&wb,'b'); // ok
    else
      getzw(E->qb,&wb,&wb,'a'); // ok

    for(i = 0,e=E->edge; i < E->Nedges; ++i){

      /* set up weights for based on edge area */
      if(E->curvX){
  for(j = 0; j < E->qb; ++j)
    wk[j] = ddot(E->qa,wa,1,E->geom->jac.p+j*E->qa,1);
  e[i].weight = ddot(E->qb,wb,1,wk,1);
      }
      else{
  e[i].weight = E->geom->jac.d;
      }

      /* loop through and reset to minimum */
      if(e[i].base){
  /* see if there is an adjacent edge */
  if(e[i].link)
    e[i].qedg = max(e[i].qedg,e[i].link->qedg);
  else
    e[i].qedg = max(e[i].qedg,e[i].base->qedg);
  edgtot += e[i].qedg;
      }
      else{
  e[i].link = (Edge *)calloc(1,sizeof(Edge));
  //e[i].link[0] = e[i];
  memcpy(e[i].link,e+i,sizeof(Edge));
  e[i].link->base = e+i;
  edgtot += 2*e[i].qedg;
      }
    }
  }

  /* declare memory for edges at once to keep it together */
  EL->flist[0]->edge->h = dvector(0,edgtot-1);
  dzero(edgtot,EL->flist[0]->edge->h,1);
  c = 0;
  for(k = 0; k < nel; ++k) {
    E = EL->flist[k];
    for(i = 0,e=E->edge; i < E->Nedges; ++i){
      e[i].h = EL->flist[0]->edge->h + c;
      c += e[i].qedg;
      if(!e[i].base){
  e[i].link->h = EL->flist[0]->edge->h + c;
  c += e[i].qedg;
      }
    }
  }

  // viscous weights
  /* loop through edges and calculate edge weights as ratio of the local
     area of adjacent elements - otherwise set it to be 1 */
  for(E=EL->fhead;E;E = E->next){
    for(i = 0; i < E->Nedges; ++i)
      if(E->edge[i].base){
  if(E->edge[i].link){
    tot = E->edge[i].weight + E->edge[i].link->weight;

    w = E->edge[i].weight/tot;
    E->edge[i].weight =  E->edge[i].link->weight/tot;
    E->edge[i].link->weight = w;
  }
      }
      else{
  E->edge[i].weight = 1;
  E->edge[i].link->weight = 0;
      }
  }
  free(wk);
}

/* set up outward facing normals along faces as well as the edge
   jacobeans divided by the jacobean for the triangle. All points
   are evaluated at the  gauss quadrature points */

void set_edge_geofac(Element_List *EL){
  Element *E;
  int i,j,k;

  for(E=EL->fhead;E;E = E->next)
    E->set_edge_geofac();

}
