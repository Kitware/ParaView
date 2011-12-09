/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/nek2tec.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/01/16 15:49:01 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <ctype.h>
#include <math.h>
#include <veclib.h>
#include <nektar.h>
#include <gen_utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <sys/stat.h>
#include <sys/types.h>

int mkdir(const char *pathname, mode_t mode);


extern "C"
{
  //   dsyevd_ ('V',     'L',      N,        A,       LDA,        W,       WORK,       LWORK,    IWORK,  LIWORK,    INFO);
  //void dsyevd_ (char *, char *, const int&, double *, const int&, double *, double *, const int&, int *, const int&,  int&);

    void dsyevd_( char *, char *, int *,       double *, int *lda, double *w, double *work, int *lwork,  int *iwork, int *liwork,    int *info );
    void dsyev_( char *jobz, char *uplo, int *n, double *a, int *lda,
        double *w, double *work, int *lwork, int *info );
}

int DIM;

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "nek2tec";
char *usage  = "nek2tec:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-q      ... quadrature point spacing. Default is even spacing\n"
  "-R      ... range data information. must have mesh file specified\n"
  "-b      ... make body elements specified in mesh file\n"
  "-p      ... project vorticity by inverting mass matrix\n"
  "-d      ... dump output to a field file (implied -p)\n"
  "-f      ... dump in equispace finite element data format \n"
  "-c      ... dump in continuous equispace finite element data format \n"
  "-surf  ... Take input as a surface field file\n"
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

static int  setup (FileList *f, Element_List **U, int *nftot, int Nsnapshots, Bndry *Ubc);
static void ReadCopyField (FileList *f, Element_List **U);
static void ReadAppendField (FileList *f, Element_List **U,  int start_fieled_index);
static void Calc_Vort (FileList *f, Element_List **U, int nfields, int Snapshot_index);
static void Calc_Write_WSS(Element_List **E, FileList f, Bndry *Ubc, int Snapshot_index);
static void Get_Body(FILE *fp);
static void dump_faces(FILE *out, Element_List **E, Coord X, int nel, int zone,
           int nfields);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void WriteS(Element_List **E, FILE *out, int nfields,int Snapshot_index);
static void WriteC(Element_List **E, FILE *out, int nfields);
static void Write(Element_List **E, FILE *out, int nfields);


void compute_Corr_matrix(double **C, Element_List **E, int field_index, int Nsnapshots);
int MY_DSYEVD(int nfields, double **K, double *EIG_VAL);
int compute_VA(double **V, Element_List **E, int field_index, int nfields);
int reconstruct_field(double **V, Element_List **E, int field_index, int nfields, int Nmodes);


void solve(Element_List *U, Element_List *Uf,
           Bndry *Ubc, Bsystem *Ubsys,SolveType Stype)
{
  SetBCs (U,Ubc,Ubsys);
  Solve  (U,Uf,Ubc,Ubsys,Stype);
}


main (int argc, char *argv[])
{
  register int  i,j,k;
  int       dump=0,nfields,nftot;

  FileList       f;
  Element_List **master;
  Bndry         *Ubc;
  char file_name[BUFSIZ];

  init_comm(&argc, &argv);

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  int fieldID[6];
  fieldID[0] = 74;
  fieldID[1] = 134;
  fieldID[2] = 207;
  fieldID[3] = 268;
  fieldID[4] = 316;
  fieldID[5] = 544;

  int Nsnapshots  = 594;
  int N_POD_MODES = Nsnapshots;
  master  = (Element_List **) malloc(_MAX_FIELDS*sizeof(Element_List *));
  nfields = setup (&f, master, &nftot, Nsnapshots,Ubc);


  ReadCopyField(&f,master);

  for (i = 1; i < Nsnapshots; ++i){
    sprintf(f.in.name,"RST_DATA/surf_test14_%d.rst",1+i);
    f.in.fp = fopen(f.in.name,"r");
    if (!f.in.fp) error_msg(Restart: no dumps read from restart file);
    ReadAppendField(&f,master,nfields*i);
   }

  DIM = master[0]->fhead->dim();
  for(i = 0; i < Nsnapshots*nfields; ++i)
    master[i]->Trans(master[i],J_to_Q);


#if 1


  double ***C;
  double **C_tmp = dmatrix(0,Nsnapshots-1,0,Nsnapshots-1);

  C = new double**[3];
  for (i = 0; i < 3; ++i){
    C[i] = dmatrix(0,Nsnapshots-1,0,Nsnapshots-1);
    memset(C[i][0],'\0',Nsnapshots*Nsnapshots*sizeof(double));
    compute_Corr_matrix(C[i],master,i,Nsnapshots);
    memset(C_tmp[0],'\0',Nsnapshots*Nsnapshots*sizeof(double));
    gdsum(C[i][0],Nsnapshots*Nsnapshots,C_tmp[0]);
    MY_DSYEVD(Nsnapshots, C[i], C_tmp[0]);
    compute_VA(C[i], master, i, Nsnapshots);
    reconstruct_field(C[i], master,i,Nsnapshots, N_POD_MODES);
  }


  for (i = 0; i < 3; ++i)
    free_dmatrix(C[i],0,0);
  free_dmatrix(C_tmp,0,0);


  /* create directories for *dat files */
  ROOTONLY{
    char dir_name[BUFSIZ];
    int j;
    for (j = 0; j < 6; ++j){
      sprintf(dir_name,"VORT_DATA_%d",fieldID[j]);
      i = mkdir(dir_name,0700);
      if (i != 0 )
         fprintf(stderr,"Error in creating new directory  \n ");
/*
      sprintf(dir_name,"WSS_DATA_%d",fieldID[j]);
      i = mkdir(dir_name,0700);
      if (i != 0 )
         fprintf(stderr,"Error in creating new directory  \n ");
*/
    }
  }
  gsync();


  /* COMPUTE WSS */
/*
  for (i = 0; i < 6; ++i){
    sprintf(file_name,"WSS_DATA_%d/surf_test14_%d.dat",fieldID[i],mynode());
    f.out.fp = fopen(file_name,"w");
    Calc_Write_WSS(master, f, Ubc,fieldID[i]);
    fclose(f.out.fp);
  }
*/

  for (i = 0; i < 6; ++i)
    Calc_Vort(&f,master,nfields, fieldID[i]-1);

  if(option("FEstorage")){

    for (i = 0; i < 6; ++i){

      sprintf(file_name,"VORT_DATA_%d/surf_test14_%d.dat",fieldID[i],mynode());
      f.out.fp = fopen(file_name,"w");
      WriteS(master,f.out.fp, 4, fieldID[i]-1);
      fclose(f.out.fp);
    }
  }
  else if(option("Continuous"))
    WriteC(master,f.out.fp, nftot);
  else
    Write(master,f.out.fp, nftot);


#endif

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

static void Average_edges_div(Element_List *Us, Element_List *Uf,
            char operation){
  register int i,j,k,n;
  int     qedg,nel = Us->nel;
  double  w1,w2;
  Edge    *e;

  for(k = 0; k < nel; ++k)
    for(i = 0 ; i < Us->flist[k]->Nedges; ++i){
      if((e = Us->flist[k]->edge+i)->link){
  qedg = e->qedg;
  w1   = e->weight;
  w2   = e->link->weight;
  if(e->con == e->link->con)
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->link->h[j]*w2;
  else
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->link->h[qedg-1-j]*w2;
      }
      else{
  qedg = e->qedg;
  w1   = e->weight;
  w2   = e->base->weight;
  if(e->con == e->base->con)
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->base->h[j]*w2;
  else
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->base->h[qedg-1-j]*w2;
      }
    }
}

Gmap *gmap;

int  setup (FileList *f, Element_List **U, int *nft, int Nsnapshots, Bndry *Ubc){
  int    i,shuff;
  int    nfields,nftot;
  Field  fld;
  extern Element_List *Mesh;

  memset(&fld, '\0', sizeof (Field));
  readHeader(f->in.fp,&fld,&shuff);
  rewind(f->in.fp);

  nfields = strlen(fld.type);
  nftot   = nfields*Nsnapshots;

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

  DO_PARALLEL{ // recall global numbering to put partition vertices first
    free_gmap(gmap);
    Reflect_Global_Velocity    (Mesh, (Bndry *)NULL, 0);
    gmap  = GlobalNumScheme    (Mesh, (Bndry *)NULL);
    Replace_Numbering          (U[0], Mesh);
  }

  init_ortho_basis();

  if(f->mesh.name) Get_Body(f->mesh.fp);

  for(i = 1; i < nfields; ++i){
    U[i] = U[0]->gen_aux_field('u');
    U[i]->fhead->type = fld.type[i];
  }

  for(i = nfields; i < nftot; ++i){
    U[i] = U[0]->gen_aux_field('u');
    U[i]->fhead->type = fld.type[nfields % i];
  }

  freeField(&fld);

  nft[0] = nftot;
  return nfields;
}

static  void ReadAppendField (FileList *f, Element_List **U,  int start_field_index){
  int i,dump;
  int nfields;
  Field fld;

  memset(&fld, '\0', sizeof (Field));

  dump = readField (f->in.fp, &fld);
  if (!dump) error_msg(Restart: no dumps read from restart file);
  rewind (f->in.fp);

  nfields = strlen(fld.type);

  for(i = 0; i < nfields; ++i)
    copyfield(&fld,i,U[i+start_field_index]->fhead);


  freeField(&fld);
  fclose(f->in.fp);

}


static  void ReadCopyField (FileList *f, Element_List **U){
  int i,dump;
  int nfields;
  Field fld;

  memset(&fld, '\0', sizeof (Field));

  if(option("oldhybrid"))
    set_nfacet_list(U[0]);

  dump = readField (f->in.fp, &fld);
  if (!dump) error_msg(Restart: no dumps read from restart file);
  rewind (f->in.fp);

  nfields = strlen(fld.type);


  if(option("Surface_Fld")){
    Bndry   *Ubc, *B;
    int     nbcs=0;

    /* set up boundary structure */
    Ubc = ReadMeshBCs(f->rea.fp,U[0]);
    for (B=Ubc;B;B=B->next) nbcs++;
    Ubc = bsort(Ubc,nbcs);

    for(i = 0; i < nfields; ++i)
      copysurffield(&fld,i,Ubc,U[i]);
  }
  else
    for(i = 0; i < nfields; ++i)
      copyfield(&fld,i,U[i]->fhead);


  freeField(&fld);
}

static void Calc_Vort (FileList *f, Element_List **U, int nfields, int Snapshot_index){
  int i,j,k,qt;
  double *z, *w;

  Snapshot_index *= nfields;

  for(i=0;i<nfields;++i)
    U[i+Snapshot_index]->Trans(U[i+Snapshot_index],J_to_Q);

  if(U[0]->fhead->dim() == 3){

    double  **d = dmatrix(0,8,0,QGmax*QGmax*QGmax-1);

    double  *Ux=d[0], *Uy=d[1], *Uz=d[2];
    double  *Vx=d[3], *Vy=d[4], *Vz=d[5];
    double  *Wx=d[6], *Wy=d[7], *Wz=d[8];

    for(k = 0; k < U[0]->nel; ++k){
      qt = U[0+Snapshot_index]->flist[k]->qtot;

      U[0+Snapshot_index]->flist[k]->Grad_d(Ux, Uy, Uz, 'a');
      U[1+Snapshot_index]->flist[k]->Grad_d(Vx, Vy, Vz, 'a');
      U[2+Snapshot_index]->flist[k]->Grad_d(Wx, Wy, Wz, 'a');

      dvsub(qt, Wy, 1, Vz, 1, **U[0+Snapshot_index]->flist[k]->h_3d, 1);
      dvsub(qt, Uz, 1, Wx, 1, **U[1+Snapshot_index]->flist[k]->h_3d, 1);
      dvsub(qt, Vx, 1, Uy, 1, **U[2+Snapshot_index]->flist[k]->h_3d, 1);

      dvadd(qt, Ux, 1, Vy, 1, **U[3+Snapshot_index]->flist[k]->h_3d, 1);
      dvadd(qt, Wz, 1, **U[3+Snapshot_index]->flist[k]->h_3d, 1,
            **U[3+Snapshot_index]->flist[k]->h_3d, 1);
    }

    free_dmatrix(d,0,0);

    if(option("PROJECT")){
      Element_List *T  = U[0+Snapshot_index]->gen_aux_field('T');
      Element_List *Tf = T->gen_aux_field('T');
      Bsystem *Bsys;
      option_set("recursive",1);

      Bsys = gen_bsystem(T, gmap);

      ROOTONLY
        fprintf(stderr,"Projecting Vorticity -- Generating Matrix [");

      Bsys->lambda = (Metric*) calloc(U[0+Snapshot_index]->nel, sizeof(Metric));
      GenMat(T, NULL, Bsys, Bsys->lambda, Mass);
      ROOTONLY fprintf(stderr," ]\n");


      /********** PROJECTION  OF VORTICITY ON Co FIELD ******************/

      /* project Wx */

      dcopy(U[0+Snapshot_index]->htot, U[0+Snapshot_index]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      ROOTONLY fprintf(stderr,"Projecting Wx\n");
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(U[0+Snapshot_index], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, U[0+Snapshot_index]->base_hj, 1);
      U[0+Snapshot_index]->fhead->type = 'a';


      /* project Wy */

      dcopy(U[1+Snapshot_index]->htot, U[1+Snapshot_index]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      ROOTONLY fprintf(stderr,"Projecting Wy\n");
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(U[1+Snapshot_index], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, U[1+Snapshot_index]->base_hj, 1);
      U[1+Snapshot_index]->fhead->type = 'b';


     /* project Wz */

      dcopy(U[2+Snapshot_index]->htot, U[2+Snapshot_index]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      ROOTONLY fprintf(stderr,"Projecting Wz\n");
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(U[2+Snapshot_index], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, U[2+Snapshot_index]->base_hj, 1);
      U[2+Snapshot_index]->fhead->type = 'c';


     /********** PROJECTION DONE ******************/

      if(option("DUMPFILE")){
        char out[BUFSIZ],out1[BUFSIZ];
        FILE *fp[2];

        ROOTONLY fprintf(stderr,"Writing output\n");
        sprintf(out,"%s_vort.fld",f->rea.name);

        DO_PARALLEL{
          char out1[BUFSIZ];

          sprintf(out1,"%s.fld.hdr.%d",out,pllinfo.procid);
          fp[0] = fopen(out1,"w");
          sprintf(out1,"%s.fld.dat.%d",out,pllinfo.procid);
          fp[1] = fopen(out1,"w");
        }
        else{
          sprintf(out1,"%s.fld",out);
          fp[0] = fp[1] = fopen(out1,"w");
        }

        U[nfields]->Set_state('t');
        U[nfields+1]->Set_state('t');
        U[nfields+2]->Set_state('t');

        Writefld(fp, out, 0 ,0 , 3, U + nfields);

        DO_PARALLEL{
          fclose(fp[0]);
          fclose(fp[1]);
        }
        else
          fclose(fp[0]);
        DO_PARALLEL
          exit_comm();
        exit(1);
      }
    }

  }
  else{

    double  **d = dmatrix(0,3,0,QGmax*QGmax-1);
    double  *Ux = d[0], *Uy = d[1];
    double  *Vx = d[2], *Vy = d[3];
    {
      for(k = 0; k < U[0]->nel; ++k){

        qt = U[0]->flist[k]->qtot;

        U[0]->flist[k]->Grad_d(Ux, Uy, 0, 'a');
        U[1]->flist[k]->Grad_d(Vx, Vy, 0, 'a');

        dvsub(qt, Vx, 1, Uy, 1, *U[nfields]->flist[k]->h, 1);
        dvadd(qt, Ux, 1, Vy, 1, *U[nfields+1]->flist[k]->h, 1);
      }
    }

    if(option("PROJECT")){
      Element_List *T  = U[0]->gen_aux_field('T');
      Bsystem *Bsys;
      Element_List *Tf = T->gen_aux_field('T');

      option_set("recursive",1);

      Bsys = gen_bsystem(T, gmap);

      ROOTONLY fprintf(stderr,"Projecting Vorticity -- Generating Matrix [");

      Bsys->lambda = (Metric*) calloc(U[0]->nel, sizeof(Metric));
      GenMat(T, NULL, Bsys, Bsys->lambda, Mass);
      ROOTONLY fprintf(stderr," ]\n");

      dcopy(U[nfields]->htot, U[nfields]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(U[nfields], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, U[nfields]->base_hj, 1);
      U[nfields]->fhead->type = 'a';

      dcopy(U[nfields+1]->htot, U[nfields+1]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(U[nfields+1], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, U[nfields+1]->base_hj, 1);
      U[nfields+1]->fhead->type = 'b';

      if(option("DUMPFILE")){
        char out[BUFSIZ],out1[BUFSIZ];
        FILE *fp[2];

        ROOTONLY fprintf(stderr,"Writing output\n");
        sprintf(out,"%s_vort.fld",f->rea.name);

        DO_PARALLEL{
          char out1[BUFSIZ];

          sprintf(out1,"%s.fld.hdr.%d",out,pllinfo.procid);
          fp[0] = fopen(out1,"w");
          sprintf(out1,"%s.fld.dat.%d",out,pllinfo.procid);
          fp[1] = fopen(out1,"w");
        }
        else{
          sprintf(out1,"%s.hdr",out);
          fp[0] = fp[1] = fopen(out,"w");
        }

        U[nfields]->Set_state('t');
        U[nfields+1]->Set_state('t');

        Writefld(fp, out, 0, 0, 2, U + nfields);

        DO_PARALLEL{
          fclose(fp[0]);
          fclose(fp[1]);
        }
        else
          fclose(fp[0]);
        exit(1);
      }
    }
    free_dmatrix(d,0,0);
  }
}







static int Check_range(Element *E);

static void WriteS(Element_List **E, FILE *out, int nfields, int Snapshot_index){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;

  Snapshot_index *= nfields;

  interior = ivector(0,E[0+Snapshot_index]->nel-1);

  if(E[0+Snapshot_index]->fhead->dim() == 3){
    double ***num;

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);


    // ROOTONLY{
      fprintf(out,"VARIABLES = x y z");

      for(i = 0; i < nfields; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fputc('\n',out);
      //}

    for(k = 0,n=i=0; k < E[0+Snapshot_index]->nel; ++k){
      F  = E[0+Snapshot_index]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  i += qa*(qa+1)*(qa+2)/6;
  n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    switch(F->identify()){
    case Nek_Tet:
      fprintf(out,"ZONE  N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n",i,n);
      break;
    default:
      fprintf(stderr,"WriteS is not set up for this element type \n");
      exit(1);
      break;
    }

    /* dump data */
    for(k = 0; k < E[0+Snapshot_index]->nel; ++k){
      F  = E[0+Snapshot_index]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);
  ntot = Interp_symmpts(F,F->qa,X.x,X.x,'p');
  ntot = Interp_symmpts(F,F->qa,X.y,X.y,'p');
  ntot = Interp_symmpts(F,F->qa,X.z,X.z,'p');
  for(n = 0; n < nfields; ++n){
    F = E[n+Snapshot_index]->flist[k];
    Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'p');
  }
  for(i = 0; i < ntot; ++i){
    fprintf(out,"%lg %lg %lg", X.x[i], X.y[i], X.z[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n+Snapshot_index]->flist[k]->h_3d[0][0][i]);
    fputc('\n',out);
  }
      }
    }


    /* numbering array */
    num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

    for(cnt = 1, k = 0; k < qa; ++k)
      for(j = 0; j < qa-k; ++j)
  for(i = 0; i < qa-k-j; ++i, ++cnt)
    num[i][j][k] = cnt;

    for(e = 0,n=0; e < E[0]->nel; ++e){
      F  = E[0]->flist[e];
      if(Check_range(F)){
  qa = F->qa;
  /* dump connectivity */
  switch(F->identify()){
  case Nek_Tet:
    for(k=0; k < qa-1; ++k)
      for(j = 0; j < qa-1-k; ++j){
        for(i = 0; i < qa-2-k-j; ++i){
    fprintf(out,"%d %d %d %d\n", n+(int) num[i][j][k],
      n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k], n+(int) num[i][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i][j][k+1], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k+1],
      n+(int) num[i][j][k+1],
      n+(int) num[i][j+1][k+1], n+(int) num[i][j+1][k]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k+1], n+(int) num[i][j+1][k+1]);
    if(i < qa-3-k-j)
      fprintf(out,"%d %d %d %d\n", n+(int) num[i][j+1][k+1],
        n+(int) num[i+1][j+1][k+1],
        n+(int) num[i+1][j][k+1],
        n+(int) num[i+1][j+1][k]);
        }
        fprintf(out,"%d %d %d %d\n", n+(int) num[qa-2-k-j][j][k],
          n+(int) num[qa-1-k-j][j][k],
          n+(int) num[qa-2-k-j][j+1][k],
          n+(int) num[qa-2-k-j][j][k+1]);
      }
    n += qa*(qa+1)*(qa+2)/6;
    break;
  default:
    fprintf(stderr,"WriteS is not set up for this element type \n");
    exit(1);
    break;
  }
      }
    }

    free(X.x); free(X.y); free(X.z);
    free_dtarray(num,0,0,0);
  }
  else{

    ROOTONLY{
      fprintf(out,"VARIABLES = x y");

      for(i = 0; i < nfields; ++i)
  fprintf(out," %c", E[i]->fhead->type);
      fputc('\n',out);
    }

    /* set up global number scheme for this partition */
    nelmts = 0;
    for(k = 0,n=0; k < E[0]->nel; ++k){
      F= E[0]->flist[k];
      qa = F->qa;

      if(F->identify() == Nek_Tri){
  nelmts += (qa-1)*(qa-1);
  n += qa*(qa+1)/2;
      }
      else{
  nelmts += 2*(qa-1)*(qa-1);
  n += qa*qa;
      }
    }

    /* treat all elements as tri's for hybrid code in tecplot */
    fprintf(out,"ZONE, N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n",n, nelmts);

    X.x = dvector(0,QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax-1);

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);

  ntot = Interp_symmpts(F,qa,X.x,X.x,'p');
  ntot = Interp_symmpts(F,qa,X.y,X.y,'p');
  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,qa,F->h[0],F->h[0],'p');
  }

  for(i = 0; i < ntot; ++i){
    fprintf(out,"%lg %lg", X.x[i], X.y[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h[0][i]);
    fputc('\n',out);
  }
      }
    }

    /* dump connectivity */
    for(k = 0,n=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  if(F->identify() == Nek_Tri){
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-2-j; ++i){
        fprintf(out,"%d %d %d\n",n+cnt+i+1,n+cnt+i+2,n+cnt+qa-j+i+1);
        fprintf(out,"%d %d %d\n",n+cnt+qa-j+i+2,n+cnt+qa-j+i+1,n+cnt+i+2);
      }
      fprintf(out,"%d %d %d\n",n+cnt+qa-1-j,n+cnt+qa-j,
        n+cnt+2*qa-2*j-1);
      cnt += qa-j;
    }
    n += qa*(qa+1)/2;
  }
  else{
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-1; ++i){
        fprintf(out,"%d %d %d\n",cnt+i+1,cnt+i+2,cnt+qa+i+1);
        fprintf(out,"%d %d %d\n",cnt+qa+i+2,cnt+qa+i+1,cnt+i+2);
      }
      cnt += qa;
    }
    n += 2*(qa-1)*(qa-1);
  }
      }
    }
    free(X.x); free(X.y);
  }
}

static void WriteC(Element_List **E, FILE *out, int nfields){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;


  interior = ivector(0,E[0]->nel-1);

  if(E[0]->fhead->dim() == 3){
    double ***num;

    fprintf(stderr,"CONTINUOUS dump scheme not set up yet in 3d \n");
    exit(1);

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);

    ROOTONLY{
      fprintf(out,"VARIABLES = x y z");

      for(i = 0; i < nfields; ++i)
  fprintf(out," %c", E[i]->fhead->type);
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

    switch(F->identify()){
    case Nek_Tet:
      fprintf(out,"ZONE  N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n",i,n);
      break;
    default:
      fprintf(stderr,"WriteS is not set up for this element type \n");
      exit(1);
      break;
    }

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);
  ntot = Interp_symmpts(F,F->qa,X.x,X.x,'m');
  ntot = Interp_symmpts(F,F->qa,X.y,X.y,'m');
  ntot = Interp_symmpts(F,F->qa,X.z,X.z,'m');
  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'m');
  }
  for(i = 0; i < ntot; ++i){
    fprintf(out,"%lg %lg %lg", X.x[i], X.y[i], X.z[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h_3d[0][0][i]);
    fputc('\n',out);
  }
      }
    }


    /* numbering array */
    num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

    for(cnt = 1, k = 0; k < qa; ++k)
      for(j = 0; j < qa-k; ++j)
  for(i = 0; i < qa-k-j; ++i, ++cnt)
    num[i][j][k] = cnt;

    for(e = 0,n=0; e < E[0]->nel; ++e){
      F  = E[0]->flist[e];
      if(Check_range(F)){
  qa = F->qa;
  /* dump connectivity */
  switch(F->identify()){
  case Nek_Tet:
    for(k=0; k < qa-1; ++k)
      for(j = 0; j < qa-1-k; ++j){
        for(i = 0; i < qa-2-k-j; ++i){
    fprintf(out,"%d %d %d %d\n", n+(int) num[i][j][k],
      n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k], n+(int) num[i][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i][j][k+1], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k+1],
      n+(int) num[i][j][k+1],
      n+(int) num[i][j+1][k+1], n+(int) num[i][j+1][k]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k+1], n+(int) num[i][j+1][k+1]);
    if(i < qa-3-k-j)
      fprintf(out,"%d %d %d %d\n", n+(int) num[i][j+1][k+1],
        n+(int) num[i+1][j+1][k+1],
        n+(int) num[i+1][j][k+1],
        n+(int) num[i+1][j+1][k]);
        }
        fprintf(out,"%d %d %d %d\n", n+(int) num[qa-2-k-j][j][k],
          n+(int) num[qa-1-k-j][j][k],
          n+(int) num[qa-2-k-j][j+1][k],
          n+(int) num[qa-2-k-j][j][k+1]);
      }
    n += qa*(qa+1)*(qa+2)/6;
    break;
  default:
    fprintf(stderr,"WriteS is not set up for this element type \n");
    exit(1);
    break;
  }
      }
    }

    free(X.x); free(X.y); free(X.z);
    free_dtarray(num,0,0,0);
  }
  else{
    ROOTONLY{
      fprintf(out,"VARIABLES = x y");

      for(i = 0; i < nfields; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fputc('\n',out);
    }

    nelmts = 0;
    /* set up global number scheme for this partition */
    for(k = 0,n=1; k < E[0]->nel; ++k){
      F= E[0]->flist[k];
      qa = F->qa;
      for(i = 0; i < F->Nverts; ++i)
  if(F->vert[0].base){
    if(F->vert[i].base == F->vert+i)
      F->vert[i].gid = n++;
  }
  else
    F->vert[i].gid = n++;

      for(i = 0; i < F->Nedges; ++i)
  if(F->edge[i].base){
    if(F->edge[i].base == F->edge+i){
      F->edge[i].gid = n;
      n += qa-2;
    }

    if(F->edge[i].base->con){ /* make sure base con is 0 */
      F->edge[i].base->con = 0;
      F->edge[i].link->con = 1;
    }
  }
  else{
    F->edge[i].gid = n;
    n+=qa-2;
  }

      interior[k] = n;
      if(F->identify() == Nek_Tri){
  nelmts += (qa-1)*(qa-1);
  n += (qa-3)*(qa-2)/2;
      }
      else{
  nelmts += 2*(qa-1)*(qa-1);
  n += (qa-2)*(qa-2);
      }
    }

    /* look over and update link lists */
    for(k = 0; k < E[0]->nel; ++k){
      F= E[0]->flist[k];
      qa = F->qa;
      for(i = 0; i < F->Nverts; ++i)
  if(F->vert[i].base)
    if(F->vert[i].base != F->vert+i)
      F->vert[i].gid = F->vert[i].base->gid;

      for(i = 0; i < F->Nedges; ++i)
  if(F->edge[i].base)
    if(F->edge[i].base != F->edge+i)
      F->edge[i].gid = F->edge[i].base->gid;
    }

    /* treat all elements as tri's for hybrid code in tecplot */
    fprintf(out,"ZONE, N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n",n-1, nelmts);

    X.x = dvector(0,QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax-1);

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);

  ntot = Interp_symmpts(F,qa,X.x,X.x,'m');
  ntot = Interp_symmpts(F,qa,X.y,X.y,'m');

  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,qa,F->h[0],F->h[0],'m');
  }

  F = E[0]->flist[k];
  cnt = 0;
  for(i = 0; i < F->Nverts; ++i,++cnt)
    if((!F->vert[i].base)||(F->vert[i].base == F->vert+i)){
      fprintf(out,"%lg %lg", X.x[cnt], X.y[cnt]);
      for(n = 0; n < nfields; ++n)
        fprintf(out," %lg",E[n]->flist[k]->h[0][cnt]);
      fputc('\n',out);
    }

  for(i = 0; i < F->Nedges; ++i,cnt += qa-2)
    if((!F->edge[i].base)||(F->edge[i].base == F->edge+i))
      for(j = 0; j < qa-2; ++j){
        fprintf(out,"%lg %lg", X.x[cnt+j], X.y[cnt+j]);
        for(n = 0; n < nfields; ++n)
    fprintf(out," %lg",E[n]->flist[k]->h[0][cnt+j]);
        fputc('\n',out);
      }

  for(j = cnt; j < ntot; ++j){
    fprintf(out,"%lg %lg", X.x[j], X.y[j]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h[0][j]);
    fputc('\n',out);
  }
      }
    }


    int **num = imatrix(0,QGmax-1,0,QGmax-1);

    /* dump connectivity */
    for(k = 0,n=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;

  if(F->identify() == Nek_Tri){
    /* set up numbering */
    num[0][0]    = F->vert[0].gid;
    num[0][qa-1] = F->vert[1].gid;
    num[qa-1][0] = F->vert[2].gid;

    if(F->edge[0].con)
      for(i = 0; i < qa-2; ++i)
        num[0][i+1] = F->edge[0].gid+qa-3-i;
    else
      for(i = 0; i < qa-2; ++i)
        num[0][i+1] = F->edge[0].gid+i;

    if(F->edge[1].con)
      for(i = 0; i < qa-2; ++i)
        num[i+1][qa-2-i] = F->edge[1].gid+qa-3-i;
    else
      for(i = 0; i < qa-2; ++i)
        num[i+1][qa-2-i] = F->edge[1].gid+i;

    if(F->edge[2].con)
      for(i = 0; i < qa-2; ++i)
        num[i+1][0] = F->edge[2].gid+qa-3-i;
    else
      for(i = 0; i < qa-2; ++i)
        num[i+1][0] = F->edge[2].gid+i;

    for(cnt = 0,j = 0; j < qa-3; ++j)
      for(i = 0; i < qa-3-j; ++i,++cnt)
        num[j+1][i+1] = interior[k] + cnt;

    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-2-j; ++i){
        fprintf(out,"%d %d %d\n",num[j][i],num[j][i+1],num[j+1][i]);
        fprintf(out,"%d %d %d\n",num[j+1][i+1],num[j+1][i],
          num[j][i+1]);
      }
      fprintf(out,"%d %d %d\n",num[j][qa-2-j],num[j][qa-1-j],
        num[j+1][qa-2-j]);
    }

  }
  else{
    fprintf(stderr,"Continuous output not set up for quads: WriteC\n");
    exit(1);
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-1; ++i){
        fprintf(out,"%d %d %d\n",cnt+i+1,cnt+i+2,cnt+qa+i+1);
        fprintf(out,"%d %d %d\n",cnt+qa+i+2,cnt+qa+i+1,cnt+i+2);
      }
      cnt += qa;
    }
    n += 2*(qa-1)*(qa-1);
  }
      }

    }
    free_imatrix(num,0,0);
    free(X.x); free(X.y);
  }
}

static void Write(Element_List **E, FILE *out, int nfields){
  register int i,j,k,n;
  int      qa,qb,qc,qt,zone;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim();
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;

  if(!option("Qpts")){
    /* transform to equispaced */
    for(i=0;i<nfields;++i)
      OrthoInnerProduct(E[i],E[i]);
    reset_bases();
    init_ortho_basis(); /* this is a lazy way of reseting ortho basis */

    for(j=2;j<QGmax+2;++j){
      getzw(j,&z,&w,'a');
      for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;

      getzw(j,&z,&w,'b');
      for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;
    }
    if(dim == 3)
      for(j=2;j<QGmax+2;++j){
  getzw(j,&z,&w,'c');
  for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;
      }

    for(i=0;i<nfields;++i)
      OrthoJTransBwd(E[i],E[i]);
  }


  if(E[0]->fhead->dim() == 3){
    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);

    ROOTONLY{
      fprintf(out,"VARIABLES = x y z");

      for(i = 0; i < nfields; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fputc('\n',out);
    }

    for(k = 0,zone=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  F->coord(&X);
  fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, K=%d, F=POINT\n",
    ++zone,F->qa,F->qb,F->qc);
  for(i = 0; i < F->qtot; ++i){
    fprintf(out,"%lg %lg %lg", X.x[i], X.y[i], X.z[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h_3d[0][0][i]);
    fputc('\n',out);
  }
      }
    }
    if(bdy.N) dump_faces(out,E,X,k,zone,nfields);
    free(X.x); free(X.y); free(X.z);
  }
  else{
    ROOTONLY{
      fprintf(out,"VARIABLES = x y");

      for(i = 0; i < nfields; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fputc('\n',out);
    }

    X.x = dvector(0,QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax-1);

    for(k = 0,zone=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  F->coord(&X);

  fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, F=POINT\n",
    ++zone,F->qa,F->qb);
  for(i = 0; i < F->qtot; ++i){
    fprintf(out,"%lg %lg", X.x[i], X.y[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h[0][i]);
    fputc('\n',out);
  }
      }
    }
    free(X.x); free(X.y);
  }
}


static void Calc_Write_WSS(Element_List **E, FileList f, Bndry *Ubc, int Snapshot_index){

  register int i,j,k,n;
  int       qa,qb,qc,nfs,qt,size_start,eid,cnt;
  double   *z,*w,*wk,**Dm,**D,*p,**s,**s1;
  Coord     X;
  Element  *F;
  FILE     *out = f.out.fp;
  double   kinvis = dparam("KINVIS");
  Tri      T;
  int symm = option("Equispaced");
  symm = 1;

  double *xyz2vw = dvector(0,4);
  double *vw = xyz2vw+3;
  int surf_marker;

  ROOTONLY fprintf(stderr," symm = %d \n",symm);

  /* required for WSS calculation */
  Ubc  = ReadBCs (f.rea.fp,E[0]->fhead);


  Bndry *B;

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


  /* calculate derivatives */
  for(cnt=0,B = Ubc;B; B = B->next)
    if((B->type == 'W'))
      for(i = 0; i < 3; ++i){
        F = B->elmt;
        eid = F->id;
        F = E[i+Snapshot_index]->flist[eid];
        F->Grad_d(D[0],D[1],D[2],'a');

        /* Extract faces and interp to face 1 */
        for(j = 0; j < 3; ++j){
          F->GetFace      (D[j],B->face,wk);
          F->InterpToFace1(B->face,wk,Dm[3*i+j]+B->id*qt);
        }
      }

  F  = E[0]->fhead;

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
    fprintf(out, "VARIABLES = x, y, z, p, se, sxe, sye, sze, m, c1, c2\n");

  for(B = Ubc;B; B = B->next)
    if(B->type == 'W'){
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
        fprintf(out,"ZONE T =\"Elmt %d Face %d\", N=%d, E=%d, F=FEPOINT,"
                "ET=TRIANGLE \n",F->id+1,B->face+1,QGmax*(QGmax+1)/2,
                (QGmax-1)*(QGmax-1));

        for(i = 0; i < QGmax*(QGmax+1)/2; ++i){
          fprintf(out,"%lg %lg %lg ", X.x[i], X.y[i], X.z[i]);
          fprintf(out,"%lg %lg ", p[B->id*qt+i],
                  sqrt(s[0][i]*s[0][i]+s[1][i]*s[1][i]+s[2][i]*s[2][i]));
          if(option("cylindrical")) // project vectors
            fprintf(out,"%lg %lg %lg %lg %lg\n", s[0][i], s[1][i], s[2][i],
                    s1[0][i],s1[1][i]);
          else
            fprintf(out,"%lg %lg %lg \n", s[0][i], s[1][i], s[2][i]);

          if ( F->curve){
            xyz2vw[0] = X.x[i];
            xyz2vw[1] = X.y[i];
            xyz2vw[2] = X.z[i];
            F->curve->info.free.get_vw_safe(&xyz2vw[0],&vw[0]);
            surf_marker = F->curve->info.free.nvc;
          }
          else{
           vw[0] = 0.0;
            vw[1] = 0.0;
            surf_marker = -1;
          }
          fprintf(out,"%d %lg %lg ", surf_marker,  vw[0], vw[1]);
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
        fprintf(out,"ZONE T=\"Elmt %d Face %d\",I=%d, J=%d, F=POINT\n",
                F->id+1,B->face+1,QGmax,QGmax);
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

  free_dmatrix(Dm,0,0);  free_dmatrix(D,0,0);
  free(p);               free(wk);
  free(xyz2vw);
}

#if 0
static void Calc_Write_WSS(Element_List *E, FileList f, Field fld, int Snapshot_index){
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
        fprintf(out,"ZONE T =\"Elmt %d Face %d\", N=%d, E=%d, F=FEPOINT,"
                "ET=TRIANGLE \n",F->id+1,B->face+1,QGmax*(QGmax+1)/2,
                (QGmax-1)*(QGmax-1));

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
        fprintf(out,"ZONE T=\"Elmt %d Face %d\",I=%d, J=%d, F=POINT\n",
                F->id+1,B->face+1,QGmax,QGmax);
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

#endif






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
    if (strcmp (*argv+1, "old") == 0) {
      option_set("oldhybrid",1);
      continue;
    }
    else if (strcmp (*argv+1, "surf") == 0) {
      option_set("Surface_Fld",1);
      continue;
    }

    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'b':
  option_set("Body",1);
  break;
      case 'c':
  option_set("Continuous",1);
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
      case 'd':
  option_set("DUMPFILE",1);
      case 'p':
  option_set("PROJECT",1);
  break;
      case 'i':
  option_set("iterative",1);
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





void compute_Corr_matrix(double **C, Element_List **E, int field_index, int nfields){


  int i,j,k,n,qa,qb,qc, field_index_I, field_index_J, time_index_I,time_index_J;
  int qab, qbc, qt;
  double *z,*wa,*wb,*wc,*val,*tmp;
  double sum;
  Element *F;

  val = dvector(0,QGmax*QGmax*QGmax-1);
  tmp = dvector(0,QGmax*QGmax*QGmax-1);


  for (time_index_I = 0; time_index_I < nfields; ++time_index_I){

    for(k = 0; k < E[0]->nel; ++k){
      field_index_I = time_index_I*4+field_index;
      F = E[field_index_I]->flist[k];
      qa = F->qa;
      qb = F->qb;
      qc = F->qc;
      qab = qa*qb;
      qbc = qb*qc;
      qt = qab*qc;
      memcpy(val,F->h_3d[0][0],qt*sizeof(double));
      getzw(qa,&z,&wa,'a');
      getzw(qb,&z,&wb,'b');
      getzw(qc,&z,&wc,'c');

      for(i = 0; i < qbc; ++i)
         dcopy(qa,wa,1,tmp+i*qa,1);
      for(i = 0; i < qc; ++i)
        for(j = 0; j < qa; ++j)
          dvmul(qb,wb,1,tmp+i*qab+j,qa,tmp+i*qab+j,qa);

      for(i = 0; i < qab; ++i)
        dvmul(qc,wc,1,tmp+i,qab,tmp+i,qab);

      if(F->curvX)
  dvmul(qt, F->geom->jac.p, 1, tmp, 1, tmp, 1);
      else
  dsmul(qt, F->geom->jac.d, tmp, 1, tmp, 1);

      dvmul(qt, val, 1, tmp, 1, val, 1);

      /* now loop over this element for different times
   do dot product and add result into C */


      for (time_index_J = time_index_I; time_index_J < nfields; ++time_index_J){
  field_index_J = time_index_J*4+field_index;
  C[time_index_I][time_index_J] += ddot(qt,val,1,E[field_index_J]->flist[k]->h_3d[0][0],1);
      }
    }
  }

  free(val);
  free(tmp);

}


int MY_DSYEVD(int nfields, double **K, double *EIG_VAL){

        double  *work=NULL, *w;
        int n,lwork,info,i;

        n = nfields;

        w = EIG_VAL;

        work = new double[1];
        lwork=-1;
        dsyev_( "V", "L", &n, K[0], &n, w, work, &lwork, &info );
        lwork = (int) work[0];
        delete[] work;
        work = new double[lwork];

        ROOTONLY
          fprintf(stdout,"INFO = %d lwork = %d \n",info, lwork );

        dsyev_( "V", "L", &n, K[0], &n, w, work, &lwork, &info );

        ROOTONLY
          fprintf(stdout,"INFO = %d \n",info);


        ROOTONLY{
          for (i = 0;  i < n; ++i)
            fprintf(stdout,"e[%d]=%f\n",i,w[i]);
        }

/*
        for (i = 0;  i < n; ++i){
          for (info = 0;  info < n; ++info)
            fprintf(stdout," %f ",i,K[i][info]);
          fprintf(stdout,"  \n");
        }
*/
        delete[] work;

        return 0;
}


int compute_VA(double **V, Element_List **E, int field_index, int nfields){


  int i,j,k, field_index_I;
  int qt;
  double *val,*tmp;
  Element *F;

  val = dvector(0,nfields-1);
  tmp = dvector(0,nfields-1);

  for(k = 0; k < E[0]->nel; ++k){

    F = E[0]->flist[k];
    qt = F->qa*F->qb*F->qc;

    for (i = 0; i < qt; ++i){
      j = 0;
      for (field_index_I = 0; field_index_I < nfields; ++field_index_I)
        val[field_index_I] =  E[field_index_I*4+field_index]->flist[k]->h_3d[0][0][i];

      for (field_index_I = 0; field_index_I < nfields; ++field_index_I)
        tmp[field_index_I] = ddot(nfields,V[field_index_I],1,val,1);

      j = 0;
      for (field_index_I = 0; field_index_I < nfields; ++field_index_I)
         E[field_index_I*4+field_index]->flist[k]->h_3d[0][0][i] = tmp[field_index_I];

    }
  }

  free(val);
  free(tmp);
  return 0;

}

int reconstruct_field(double **V, Element_List **E, int field_index, int nfields, int Nmodes){

  int i,j,k, field_index_I;
  int qt;
  register double sum;
  double *val,*tmp;
  Element *F;

  val = dvector(0,nfields-1);
  tmp = dvector(0,nfields-1);

  j = nfields - Nmodes;
  for (field_index_I = 0; field_index_I < j; ++field_index_I)
    memset(V[field_index_I],'\0',nfields*sizeof(double));


  for(k = 0; k < E[0]->nel; ++k){

    F = E[0]->flist[k];
    qt = F->qa*F->qb*F->qc;

    for (i = 0; i < qt; ++i){

      for (field_index_I = 0; field_index_I < nfields; ++field_index_I)
        val[field_index_I] =  E[field_index_I*4+field_index]->flist[k]->h_3d[0][0][i];

      for (field_index_I = 0; field_index_I < nfields; ++field_index_I){
  sum = 0;
  for (j = 0; j < nfields; ++j)
    sum += V[j][field_index_I]*val[j];

  tmp[field_index_I] = sum;
      }

      for (field_index_I = 0; field_index_I < nfields; ++field_index_I)
          E[field_index_I*4+field_index]->flist[k]->h_3d[0][0][i] = tmp[field_index_I];
    }
  }
  free(val);
  free(tmp);
  return 0;
}
