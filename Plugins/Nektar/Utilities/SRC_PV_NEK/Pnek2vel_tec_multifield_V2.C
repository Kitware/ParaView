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
//#include <string.h>


#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
using std::vector;
using std::ostringstream;
using std::string;
using std::copy;
using std::fill_n;



#include <sys/stat.h>
#include <sys/types.h>


/* function form comm_split.C*/
MPI_Comm get_MPI_COMM();


int mkdir(const char *pathname, mode_t mode);
int write_vtk_file_XML(float *XYZ, float *UVWP, int *vert_ID_array, int Nverts, int Nelements, int vert_ID_array_len, int Elemnt_type, char *file_name, int FLAG_DUMP_INDEX, int FLAG_GEOM);
void write_pvd_file(int Ndumps, double *Time_all_steps, int *fieldID, char *file_name);
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

static int  setup (FileList *f, Element_List **U, int *nftot, int Nsnapshots);
static void ReadCopyField (FileList *f, Element_List **U);
static void ReadAppendField (FileList *f, Element_List **U,  int start_fieled_index);
static void Substruct_mean ( Element_List **U, int nfields, int Nsnapshot);
static void Calc_Vort (FileList *f, Element_List **U, int nfields, int Snapshot_index);
static void Calc_Write_WSS(Element_List **E, FileList f, Bndry *Ubc, int Snapshot_index);
static void Get_Body(FILE *fp);
static void dump_faces(FILE *out, Element_List **E, Coord X, int nel, int zone,
           int nfields);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void WriteS(Element_List **E, FILE *out, int nfields,int Snapshot_index);
static void WriteC(Element_List **E, FILE *out, int nfields);
static void Write(Element_List **E, FILE *out, int nfields);
static void WriteVTK(Element_List **E, char *fname_vtk, int nfields, int Snapshot_index, int FLAG_GEOM);
static void WriteTEC(Element_List **E, char *file_name, int nfields, int FLAG_DUMP_INDEX);

int read_number_of_snapshots(FILE *pFile);
int read_number_of_snapshots(FILE *pFile, int *index_file);
void read_field_name_in(FILE *pFile, char *name);
void read_field_name_out(FILE *pFile, char *name);
void read_time_all_steps ( int Nsnapshots, double *Time_all_steps, int *fieldID,  char *file_name_in);


void compute_Corr_matrix(double **C, Element_List **E, int field_index, int Nsnapshots);
int MY_DSYEVD(int nfields, double **K, double *EIG_VAL);
int compute_VA(double **V, Element_List **E, int field_index, int nfields);
int reconstruct_field(double **V, Element_List **E, int field_index, int nfields, int Nmodes);

static int Check_range_sub_cyl(Element *E);


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
  char file_name_in[BUFSIZ], file_name_out[BUFSIZ];

  init_comm(&argc, &argv);

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  int Nsnapshots;
  int *fieldID;
  double *Time_all_steps;

  /* read input file */
  FILE *pFile_params;
  char buf[BUFSIZ];
  pFile_params = fopen("PARAMETERS.txt","r");

  /* read number of fields */

  Nsnapshots = read_number_of_snapshots(pFile_params);
  fieldID = new int[Nsnapshots];
  read_number_of_snapshots(pFile_params,fieldID);
  read_field_name_in(pFile_params, file_name_in);
  read_field_name_out(pFile_params, file_name_out);

  fclose(pFile_params);

  /*  read Time  */
  Time_all_steps = new double[Nsnapshots];
  read_time_all_steps(Nsnapshots, Time_all_steps, fieldID,file_name_in);


  ROOTONLY{
     fprintf(stderr,"Nsnapshots = %d ",Nsnapshots);
     write_pvd_file(Nsnapshots, Time_all_steps, fieldID, file_name_out);
  }

  master  = (Element_List **) malloc((2)*4*sizeof(Element_List *));
  nfields = setup (&f, master, &nftot, 1);


  /* read the first field */
  ReadCopyField(&f,master);


  /* write fields */

  /* at the first call to WriteVTK create geometry file */

  int  FLAG_GEOM = 1;

  for (j = 0; j < Nsnapshots; ++j){

      if (j) {
        sprintf(f.in.name,"%s_%d.rst",file_name_in,fieldID[j]);
        f.in.fp = fopen(f.in.name,"r");
        if (!f.in.fp) error_msg(Restart: no dumps read from restart file);
        ReadAppendField(&f,master,0);
      }

     /* transform field into physical space */
     DIM = master[0]->fhead->dim();
     for(i = 0; i < nfields; ++i)
       master[i]->Trans(master[i],J_to_Q);

     if(option("FEstorage")){
        sprintf(file_name,"VEL_DATA_%d/%s_%d.dat",fieldID[j],file_name_out,mynode());
        WriteTEC(master, file_name_out, 4, fieldID[j]);
     }
     else{
       fprintf(stderr,"writing fields in FEstorage ONLY - rerun with option -f  \n ");
     }
  }


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


int read_number_of_snapshots(FILE *pFile){

  char buf[BUFSIZ];
  char *p;
  int index_start, index_end;
  int Nsnapshots = 0;
  rewind(pFile);

  /* read number of fields */

  while (p = fgets (buf, BUFSIZ, pFile)){
     if (strstr (p, "RANGE")){
         fgets(buf,BUFSIZ,pFile);
         sscanf(buf,"%d %d",&index_start,&index_end);
         Nsnapshots = index_end-index_start+1;
         break;
      }
      if (strstr (p, "SELECTION")){
         fgets(buf,BUFSIZ,pFile);
         sscanf(buf,"%d",&Nsnapshots);
         break;
     }
  }
  return Nsnapshots;
}



int read_number_of_snapshots(FILE *pFile, int *index_file){

  char buf[BUFSIZ];
  char *p;
  int i, index_start, index_end;
  int Nsnapshots = 0;

  rewind(pFile);

  /* read number of fields */

  while (p = fgets (buf, BUFSIZ, pFile)){
     if (strstr (p, "RANGE")){
         fgets(buf,BUFSIZ,pFile);
         sscanf(buf,"%d %d",&index_start,&index_end);
         Nsnapshots = index_end-index_start+1;
         for (i = 0; i < Nsnapshots; ++i)
           index_file[i] = index_start+i;
         break;
      }
      if (strstr (p, "SELECTION")){
         fgets(buf,BUFSIZ,pFile);
         sscanf(buf,"%d",&Nsnapshots);
         for (i = 0; i < Nsnapshots; ++i){
            fgets(buf,BUFSIZ,pFile);
            sscanf(buf,"%d",index_file+i);
         }
         break;
     }
  }
  return Nsnapshots;
}


void read_field_name_in(FILE *pFile, char *name){

  char buf[BUFSIZ];
  char *p;
  int  FLAG = 0;
  rewind(pFile);

  /* read number of fields */

  while (p = fgets (buf, BUFSIZ, pFile)){
     if (strstr (p, "NAMEIN")){
         fgets(buf,BUFSIZ,pFile);
         sscanf(buf,"%s ",name);
         FLAG = 1;
         break;
     }
  }
  if (FLAG == 0)
    sprintf(name,"data");
}

void read_field_name_out(FILE *pFile, char *name){

  char buf[BUFSIZ];
  char *p;
  int  FLAG = 0;
  rewind(pFile);

  /* read number of fields */

  while (p = fgets (buf, BUFSIZ, pFile)){
     if (strstr (p, "NAMEOUT")){
         fgets(buf,BUFSIZ,pFile);
         sscanf(buf,"%s ",name);
         FLAG = 1;
         break;
     }
  }
  if (FLAG == 0)
    sprintf(name,"data");
}


Gmap *gmap;

int  setup (FileList *f, Element_List **U, int *nft, int Nsnapshots){
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

void read_time_all_steps ( int Nsnapshots, double *Time_all_steps, int *fieldID,  char *file_name_in){

  int i,j;
  char file_name[BUFSIZ];
  FILE *pFile;
  char *p;
  char buf[BUFSIZ];

  for (i = 0; i < Nsnapshots; ++i){
    sprintf(file_name,"%s_%d.rst",file_name_in,fieldID[i]);
    pFile = fopen(file_name,"r");
    if (!pFile) error_msg(Restart: no dumps read from restart file);

    for (j = 0; j < 6; ++j)
      p = fgets (buf, BUFSIZ, pFile);

    sscanf(buf,"%lf",Time_all_steps+i);
    ROOTONLY
      fprintf(stderr,"Time_all_steps[%d] = %f \n", fieldID[i],Time_all_steps[i]);
    fclose(pFile);
  }
}


static void Substruct_mean ( Element_List **U, int nfields, int Nsnapshot){

  int i,field,k,qt;
  double mean_val;
  double *sum_val;

  for(k = 0; k < U[0]->nel; ++k){
    qt = U[0]->flist[k]->qtot;
    sum_val = new double[qt];

    for (field = 0; field < nfields; ++field){
      /* compute the average */
      memset(sum_val,'\0',qt*sizeof(double));
      for (i = 0; i < Nsnapshot; ++i)
        dvadd(qt, **U[i*nfields+field]->flist[k]->h_3d, 1, sum_val, 1, sum_val, 1);

      dscal(qt, 1.0/Nsnapshot, sum_val, 1);

     /* substruct the average */
      for (i = 0; i < Nsnapshot; ++i)
        memcpy(**U[i*nfields+field]->flist[k]->h_3d,sum_val,qt*sizeof(double));

#if 0
      /* substruct the average */
      for (i = 0; i < Nsnapshot; ++i)
        dvadd(qt, **U[i*nfields+field]->flist[k]->h_3d, 1, sum_val, 1, **U[i*nfields+field]->flist[k]->h_3d, 1);
#endif


    }
    delete[] sum_val;
  }
}


static void Calc_Vort (FileList *f, Element_List **U, int nfields, int Snapshot_index){
  int i,j,k,qt;
  double *z, *w;
  int active_handle = get_active_handle();


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

          sprintf(out1,"%s.fld.hdr.%d",out,pllinfo[active_handle].procid);
          fp[0] = fopen(out1,"w");
          sprintf(out1,"%s.fld.dat.%d",out,pllinfo[active_handle].procid);
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

          sprintf(out1,"%s.fld.hdr.%d",out,pllinfo[active_handle].procid);
          fp[0] = fopen(out1,"w");
          sprintf(out1,"%s.fld.dat.%d",out,pllinfo[active_handle].procid);
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

static void WriteVTK(Element_List **E, char *fname_vtk, int nfields, int Snapshot_index, int FLAG_GEOM){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;

  interior = ivector(0,E[0]->nel-1);


  float *XYZ;
  float *UVWP, *Pres;
  int *vert_ID_array;

  int Nvert_total = 0;
  int Nelements_total = 0;
  int index = 0;
  int vert_ID_array_length = 0;
  int Nel = E[0]->nel;

  if(E[0]->fhead->dim() == 3){
    double ***num;

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);

    qa = E[0]->flist[0]->qa;

    for(k = 0,n=i=0; k < Nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range_sub_cyl(F)){
        //if(Check_range(F)){
        qa = F->qa;
        i += qa*(qa+1)*(qa+2)/6;
        n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    Nvert_total = i;
    Nelements_total = n;


    UVWP = new float[Nvert_total*4];
    Pres = &UVWP[Nvert_total*3];


    if (FLAG_GEOM){
      XYZ = new float[Nvert_total*3];

      /* fill XYZ and UVWP arrays */
      index = 0;
      for(k = 0; k < Nel; ++k){
        F  = E[0]->flist[k];
        if(Check_range_sub_cyl(F)){
          //if(Check_range(F)){
          qa = F->qa;
          F->coord(&X);
          ntot = Interp_symmpts(F,F->qa,X.x,X.x,'p');
          ntot = Interp_symmpts(F,F->qa,X.y,X.y,'p');
          ntot = Interp_symmpts(F,F->qa,X.z,X.z,'p');
          for(n = 0; n < nfields; ++n){
            F = E[n]->flist[k];
            Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'p');
          }

          for(i = 0; i < ntot; ++i){
            XYZ[index*3]   = X.x[i];
            XYZ[index*3+1] = X.y[i];
            XYZ[index*3+2] = X.z[i];

            for(n = 0; n < 3;  ++n)
              UVWP[index*3+n] = E[n]->flist[k]->h_3d[0][0][i];
            Pres[index] = E[3]->flist[k]->h_3d[0][0][i];
            index ++;
          }
        }
      }
    }
    else{
     /* fill  UVWP arrays */
      index = 0;
      for(k = 0; k < Nel; ++k){
        F  = E[0]->flist[k];
        if(Check_range_sub_cyl(F)){
          //if(Check_range(F)){
          qa = F->qa;
          F->coord(&X);
         // ntot = Interp_symmpts(F,F->qa,X.x,X.x,'p');
          for(n = 0; n < nfields; ++n){
            F = E[n]->flist[k];
            ntot = Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'p');
          }

          for(i = 0; i < ntot; ++i){
            for(n = 0; n < 3;  ++n)
              UVWP[index*3+n] = E[n]->flist[k]->h_3d[0][0][i];
            Pres[index] = E[3]->flist[k]->h_3d[0][0][i];
            index ++;
          }
        }
      }
    }

    if (FLAG_GEOM){
      /* numbering array */
      for(e = 0,n=0; e < Nel; ++e){
        F  = E[0]->flist[e];
        if(Check_range_sub_cyl(F)){
          //if(Check_range(F)){
          qa = F->qa;
          /* dump connectivity */
          switch(F->identify()){
          case Nek_Tet:
            for(k=0; k < qa-1; ++k)
              for(j = 0; j < qa-1-k; ++j){
                for(i = 0; i < qa-2-k-j; ++i){
                   vert_ID_array_length += 20;
                   if(i < qa-3-k-j)
                     vert_ID_array_length += 4;
                }
                vert_ID_array_length += 4;
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
      vert_ID_array = new int[vert_ID_array_length];
      fprintf(stderr,"rank = %d: vert_ID_array_length = %d\n",mynode(),vert_ID_array_length);



      num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);
      for(cnt = 1, k = 0; k < qa; ++k)
        for(j = 0; j < qa-k; ++j)
          for(i = 0; i < qa-k-j; ++i, ++cnt)
            num[i][j][k] = cnt;

      fprintf(stderr,"rank = %d: cnt*Nel = %d\n",mynode(),cnt*Nel);
      gsync();

      index = 0;
      for(e = 0,n=0; e < Nel; ++e){
        F  = E[0]->flist[e];
        if(Check_range_sub_cyl(F)){
          //if(Check_range(F)){
          qa = F->qa;
          /* dump connectivity */
          switch(F->identify()){
          case Nek_Tet:
            for(k=0; k < qa-1; ++k)
              for(j = 0; j < qa-1-k; ++j){
                for(i = 0; i < qa-2-k-j; ++i){

                  vert_ID_array[index++] = n+(int) num[i][j][k];
                  vert_ID_array[index++] = n+(int) num[i+1][j][k];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k];
                  vert_ID_array[index++] = n+(int) num[i][j][k+1];

                  vert_ID_array[index++] = n+(int) num[i+1][j][k];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k];
                  vert_ID_array[index++] = n+(int) num[i][j][k+1];
                  vert_ID_array[index++] = n+(int) num[i+1][j][k+1];

                  vert_ID_array[index++] = n+(int) num[i+1][j][k+1];
                  vert_ID_array[index++] = n+(int) num[i][j][k+1];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k+1];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k];

                  vert_ID_array[index++] = n+(int) num[i+1][j+1][k];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k];
                  vert_ID_array[index++] = n+(int) num[i+1][j][k];
                  vert_ID_array[index++] = n+(int) num[i+1][j][k+1];

                  vert_ID_array[index++] = n+(int) num[i+1][j+1][k];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k];
                  vert_ID_array[index++] = n+(int) num[i+1][j][k+1];
                  vert_ID_array[index++] = n+(int) num[i][j+1][k+1];

                  if(i < qa-3-k-j){
                    vert_ID_array[index++] = n+(int) num[i][j+1][k+1];
                    vert_ID_array[index++] = n+(int) num[i+1][j+1][k+1];
                    vert_ID_array[index++] = n+(int) num[i+1][j][k+1];
                    vert_ID_array[index++] = n+(int) num[i+1][j+1][k];
                  }

                }
                vert_ID_array[index++] = n+(int) num[qa-2-k-j][j][k];
                vert_ID_array[index++] = n+(int) num[qa-1-k-j][j][k];
                vert_ID_array[index++] = n+(int) num[qa-2-k-j][j+1][k];
                vert_ID_array[index++] = n+(int) num[qa-2-k-j][j][k+1];
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

      for (k = 0; k < vert_ID_array_length; ++k)
        vert_ID_array[k] -= 1;

    free_dtarray(num,0,0,0);
    }//end "if (FLAG_GEOM)"

    free(X.x); free(X.y); free(X.z);

    static FLAG_DUMP_INDEX = 0;
    write_vtk_file_XML(XYZ,UVWP,vert_ID_array, Nvert_total, Nelements_total, vert_ID_array_length, 10, fname_vtk, Snapshot_index, FLAG_GEOM);
    FLAG_DUMP_INDEX++;

    if (FLAG_GEOM){
      delete[] XYZ;
      delete[] vert_ID_array;
    }
    delete[] UVWP;


  }
  else{
      fprintf(stderr,"2D case is not implemented");
     ;
  }
}


static void WriteTEC(Element_List **E, char *file_name, int nfields, int FLAG_DUMP_INDEX){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;

  FILE *TEC_FILE;

  interior = ivector(0,E[0]->nel-1);


  ROOTONLY{
    char path_name[BUFSIZ];
    sprintf(path_name,"%s_time%d",file_name,FLAG_DUMP_INDEX);
    i = mkdir(path_name,0700);
      if (i != 0 )
        fprintf(stderr,"Error in creating new directory \"%s\" \n ",path_name);
  }


  float *XYZUVWP,*XYZUVWP_all;
  int *vert_ID_array;

  int Nvert_total = 0;
  int Nelements_total = 0;
  int Nelements_global = 0;
  int index = 0;
  int vert_ID_array_length = 0;

  if(E[0]->fhead->dim() == 3){
    double ***num;

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);


    qa = E[0]->flist[0]->qa;

    int Nel = E[0]->nel;

    for(k = 0,n=i=0; k < Nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range_sub_cyl(F)){
  //if(Check_range(F)){
  qa = F->qa;
  i += qa*(qa+1)*(qa+2)/6;
  n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    Nvert_total = i;
    Nelements_total = n;

    XYZUVWP = new float[Nvert_total*(3+nfields)];

    /* fill XYZ and UVWP arrays */

    index = 0;
    for(k = 0; k < Nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range_sub_cyl(F)){
        //if(Check_range(F)){
        qa = F->qa;
        F->coord(&X);
        ntot = Interp_symmpts(F,F->qa,X.x,X.x,'p');
        ntot = Interp_symmpts(F,F->qa,X.y,X.y,'p');
        ntot = Interp_symmpts(F,F->qa,X.z,X.z,'p');
        for(n = 0; n < nfields; ++n){
          F = E[n]->flist[k];
          Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'p');
        }

        for(i = 0; i < ntot; ++i){
    XYZUVWP[index*(3+nfields)]   = X.x[i];
          XYZUVWP[index*(3+nfields)+1] = X.y[i];
          XYZUVWP[index*(3+nfields)+2] = X.z[i];

          for(n = 0; n < nfields;  ++n)
            XYZUVWP[index*(3+nfields)+3+n] = E[n]->flist[k]->h_3d[0][0][i];
          index ++;
        }
      }
    }

    free(X.x); free(X.y); free(X.z);

    /* gather data from all partiotions and dump it */
    int Nvert_global,Nelements_global,Ncores,MyRank;
    int *Nvert_per_core,*displs,*recvcnt;
    Ncores = numnodes();
    MyRank = mynode();
    displs = new int[Ncores*3];
    recvcnt = displs+Ncores;
    Nvert_per_core = displs+2*Ncores;

    int VIZ_ROOT = 0;

    MPI_Allgather(&Nvert_total, 1,MPI_INT,Nvert_per_core,1,MPI_INT,get_MPI_COMM());
    MPI_Reduce(&Nelements_total,&Nelements_global,1,MPI_INT,MPI_SUM,VIZ_ROOT,get_MPI_COMM());

    Nvert_global = 0;
    //compute offsets and total number of vertices.
    if (MyRank == VIZ_ROOT){
      displs[0] = 0;
      recvcnt[0] = Nvert_per_core[0]*(3+nfields);
      Nvert_global += Nvert_per_core[0];
      for (i = 1; i < Ncores; ++i){
  displs[i] = displs[i-1]+recvcnt[i-1];
  recvcnt[i] = Nvert_per_core[i]*(3+nfields);
        Nvert_global += Nvert_per_core[i];
      }
      XYZUVWP_all = new float[Nvert_global*(3+nfields)];
    }


    /* gather data to VIZ_ROOT */
    MPI_Gatherv (XYZUVWP, Nvert_total*(3+nfields), MPI_FLOAT,
     XYZUVWP_all, recvcnt, displs,
     MPI_FLOAT, VIZ_ROOT, get_MPI_COMM() );

    delete[] XYZUVWP;


    /* write header */
    if (MyRank == VIZ_ROOT){


     /* set name for *dat file */
     char TEC_FILE_NAME[BUFSIZ];
     sprintf(TEC_FILE_NAME,"%s_time%d/%s_%d.dat",file_name,FLAG_DUMP_INDEX,file_name,mynode());
     TEC_FILE = fopen(TEC_FILE_NAME,"w");

      fprintf(TEC_FILE,"VARIABLES = x y z");
      for(i = 0; i < nfields; ++i)
        fprintf(TEC_FILE," %c", E[i]->fhead->type);
      fputc('\n',TEC_FILE);

      switch(F->identify()){
        case Nek_Tet:
          fprintf(TEC_FILE,"ZONE  N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n",Nvert_global,Nelements_global);
        break;
        default:
          fprintf(stderr,"WriteS is not set up for this element type \n");
          exit(1);
        break;
      }
    }

    if (MyRank == VIZ_ROOT){
      for (index = 0; index < Nvert_global; ++index){
        i = index*(3+nfields);
        fprintf(TEC_FILE,"%f %f %f", XYZUVWP_all[i],XYZUVWP_all[i+1],XYZUVWP_all[i+2]);
        for(n = 0; n < nfields;  ++n)
    fprintf(TEC_FILE," %f", XYZUVWP_all[i+3+n]);
        fputc('\n',TEC_FILE);
      }
      delete[] XYZUVWP_all;
    }


    /* numbering array */

    for(e = 0,n=0; e < Nel; ++e){
      F  = E[0]->flist[e];
      if(Check_range_sub_cyl(F)){
        //if(Check_range(F)){
        qa = F->qa;
        /* dump connectivity */
        switch(F->identify()){
        case Nek_Tet:
          for(k=0; k < qa-1; ++k)
            for(j = 0; j < qa-1-k; ++j){
              for(i = 0; i < qa-2-k-j; ++i){
                 vert_ID_array_length += 20;
                 if(i < qa-3-k-j)
                   vert_ID_array_length += 4;
              }
              vert_ID_array_length += 4;
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
  //  vert_ID_array_length = n;
    vert_ID_array = new int[vert_ID_array_length];
    fprintf(stderr,"rank = %d: vert_ID_array_length = %d\n",mynode(),vert_ID_array_length);


    num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

    for(cnt = 1, k = 0; k < qa; ++k)
      for(j = 0; j < qa-k; ++j)
  for(i = 0; i < qa-k-j; ++i, ++cnt)
    num[i][j][k] = cnt;

    fprintf(stderr,"rank = %d: cnt*Nel = %d\n",mynode(),cnt*Nel);
    gsync();

    index = 0;
    for(e = 0,n=0; e < Nel; ++e){
      F  = E[0]->flist[e];
      if(Check_range_sub_cyl(F)){
  //if(Check_range(F)){
  qa = F->qa;
  /* dump connectivity */
  switch(F->identify()){
  case Nek_Tet:
    for(k=0; k < qa-1; ++k)
      for(j = 0; j < qa-1-k; ++j){
        for(i = 0; i < qa-2-k-j; ++i){

                vert_ID_array[index++] = n+(int) num[i][j][k];
                vert_ID_array[index++] = n+(int) num[i+1][j][k];
                vert_ID_array[index++] = n+(int) num[i][j+1][k];
                vert_ID_array[index++] = n+(int) num[i][j][k+1];

                vert_ID_array[index++] = n+(int) num[i+1][j][k];
                vert_ID_array[index++] = n+(int) num[i][j+1][k];
                vert_ID_array[index++] = n+(int) num[i][j][k+1];
                vert_ID_array[index++] = n+(int) num[i+1][j][k+1];

                vert_ID_array[index++] = n+(int) num[i+1][j][k+1];
                vert_ID_array[index++] = n+(int) num[i][j][k+1];
                vert_ID_array[index++] = n+(int) num[i][j+1][k+1];
                vert_ID_array[index++] = n+(int) num[i][j+1][k];

                vert_ID_array[index++] = n+(int) num[i+1][j+1][k];
                vert_ID_array[index++] = n+(int) num[i][j+1][k];
                vert_ID_array[index++] = n+(int) num[i+1][j][k];
                vert_ID_array[index++] = n+(int) num[i+1][j][k+1];

                vert_ID_array[index++] = n+(int) num[i+1][j+1][k];
                vert_ID_array[index++] = n+(int) num[i][j+1][k];
                vert_ID_array[index++] = n+(int) num[i+1][j][k+1];
                vert_ID_array[index++] = n+(int) num[i][j+1][k+1];

                if(i < qa-3-k-j){
                  vert_ID_array[index++] = n+(int) num[i][j+1][k+1];
                  vert_ID_array[index++] = n+(int) num[i+1][j+1][k+1];
                  vert_ID_array[index++] = n+(int) num[i+1][j][k+1];
                  vert_ID_array[index++] = n+(int) num[i+1][j+1][k];
                }
#if 0
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
#endif
        }

                vert_ID_array[index++] = n+(int) num[qa-2-k-j][j][k];
                vert_ID_array[index++] = n+(int) num[qa-1-k-j][j][k];
                vert_ID_array[index++] = n+(int) num[qa-2-k-j][j+1][k];
                vert_ID_array[index++] = n+(int) num[qa-2-k-j][j][k+1];
#if 0
        fprintf(out,"%d %d %d %d\n", n+(int) num[qa-2-k-j][j][k],
                                  n+(int) num[qa-1-k-j][j][k],
                               n+(int) num[qa-2-k-j][j+1][k],
                               n+(int) num[qa-2-k-j][j][k+1]);
#endif
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
    gsync();

    free_dtarray(num,0,0,0);

    /* gather vert_ID_array to VIZ_ROOT */
    int vert_ID_array_length_all = 0;
    int *vert_ID_array_len_per_core;
    int *vert_ID_array_all;


    vert_ID_array_len_per_core = new int[Ncores];

    MPI_Gather (&vert_ID_array_length,      1, MPI_INT,
            vert_ID_array_len_per_core, 1, MPI_INT,
          VIZ_ROOT, get_MPI_COMM());

    if (MyRank == VIZ_ROOT){
      for (i = 0; i < Ncores; ++i)
  vert_ID_array_length_all += vert_ID_array_len_per_core[i];

      vert_ID_array_all = new int[vert_ID_array_length_all];

      /* compute offsets and recv. countes */
      displs[0] = 0;
      recvcnt[0] = vert_ID_array_len_per_core[0];
      for (i = 1; i < Ncores; ++i){
  displs[i] = displs[i-1]+recvcnt[i-1];
  recvcnt[i] = vert_ID_array_len_per_core[i];
      }
    }

     /*      shift values of  the vert_IDs    */
     /*      compute offset */
     j = 0;
     for (i = 0; i < MyRank; ++i)
       j += Nvert_per_core[i];

     /*      shift vert. IDs  */
     for (i = 0; i < vert_ID_array_length; ++i)
       vert_ID_array[i] += j;

      MPI_Gatherv ( vert_ID_array, vert_ID_array_length, MPI_INT,
        vert_ID_array_all, recvcnt, displs,  MPI_INT,
        VIZ_ROOT, get_MPI_COMM() );

    delete[] vert_ID_array;

    /* write connectivity */

    if (MyRank == VIZ_ROOT){
       for (i = 0; i < vert_ID_array_length_all/4; ++i)
         fprintf(TEC_FILE,"%d %d %d %d\n",vert_ID_array_all[i*4],  vert_ID_array_all[i*4+1],
                                     vert_ID_array_all[i*4+2],vert_ID_array_all[i*4+3]);
      fclose(TEC_FILE);
      delete[] vert_ID_array_all;
    }
    delete[] displs;

  }
  else{

    ROOTONLY{
      fprintf(TEC_FILE,"VARIABLES = x y");

      for(i = 0; i < nfields; ++i)
  fprintf(TEC_FILE," %c", E[i]->fhead->type);
      fputc('\n',TEC_FILE);
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
    fprintf(TEC_FILE,"ZONE, N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n",n, nelmts);

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
    fprintf(TEC_FILE,"%lg %lg", X.x[i], X.y[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(TEC_FILE," %lg",E[n]->flist[k]->h[0][i]);
    fputc('\n',TEC_FILE);
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
        fprintf(TEC_FILE,"%d %d %d\n",n+cnt+i+1,n+cnt+i+2,n+cnt+qa-j+i+1);
        fprintf(TEC_FILE,"%d %d %d\n",n+cnt+qa-j+i+2,n+cnt+qa-j+i+1,n+cnt+i+2);
      }
      fprintf(TEC_FILE,"%d %d %d\n",n+cnt+qa-1-j,n+cnt+qa-j,
        n+cnt+2*qa-2*j-1);
      cnt += qa-j;
    }
    n += qa*(qa+1)/2;
  }
  else{
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-1; ++i){
        fprintf(TEC_FILE,"%d %d %d\n",cnt+i+1,cnt+i+2,cnt+qa+i+1);
        fprintf(TEC_FILE,"%d %d %d\n",cnt+qa+i+2,cnt+qa+i+1,cnt+i+2);
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

static int Check_range_sub_cyl(Element *E){

  return 1;

  int i;
  double xo,yo,zo;
  xo=yo=zo=0;
  /* compute center of the element */
  for(i = 0; i < E->Nverts; ++i){
      xo += E->vert[i].x;
      yo += E->vert[i].y;
      zo += E->vert[i].z;
  }
  double R = 1.0/E->Nverts;
  xo *= R;
  yo *= R;
  zo *= R;


//  if (zo < 55)
//    return 0;
//  else
//    return 1;



  if (zo < 26.0)
     return 0;

  if (zo > 95.0)
     return 0;


#if 0
/* sub Hup */
//  if (zo >  51.45)
//    return 0;

/* sub GtoH */
//  if ((zo <=  51.45) || (zo > 64.49))
//    return 0;

/* sub FtoG */
  if ((zo <=  64.49) || (zo > 73.01))
    return 0;


/* sub H */
//  if (zo >  57.96)
//    return 0;

/* sub G */
//  if ((zo <=  57.96) || (zo > 68.75)  )
//    return 0;

/* sub F */
//  if ((zo <=  68.75) || (zo > 77.3)  )
//    return 0;




/* sub ICA */
//  if (zo > 100.0)
//    return 0;

#endif

  double Ox = 8.7692;//11.62866;//    68.60218;
  double Oy = -0.77;//4.784833; //57.52709;
  double Oz = 50.574;//86.13319; //83.89227;
  double v[3];
  double nx,ny,nz;

  if (zo >= 50.574 ){
    nx = 0.079090982706979;   //-0.04171497;
    ny = 0.16250236948079;//-0.17585484;
    nz = 0.98353322077476;//-0.98353191;
    R = 2.3;
  }
  else{
    nx = 0.0;
    ny = 0.0;
    nz = 1.0;
    R = 2.5;
  }

  v[0] = xo-Ox;
  v[1] = yo-Oy;
  v[2] = zo-Oz;

  xo = ny*v[2]-nz*v[1];
  yo = nz*v[0]-nx*v[2];
  zo = nx*v[1]-ny*v[0];

  if ( (xo*xo+yo*yo+zo*zo)  < (R*R))
    return 1;
  else
    return 0;

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

      F = E[field_index_I]->flist[k];


#if 1
      /* if the element is inside the region of interest  process it */

      qa = Check_range_sub_cyl(F);

      //for(i = 0; i < F->Nverts; ++i){
      //  if((F->vert[i].x < rnge->x[0])||(F->vert[i].x > rnge->x[1])) return 0;
      //  if((F->vert[i].y < rnge->y[0])||(F->vert[i].y > rnge->y[1])) return 0;
      //  if(DIM == 3)
      //   if( F->vert[i].z > 68.75){
      //   if( (F->vert[i].z > 77.3) || (F->vert[i].z < 68.75) )  {
      //      qa = 0;
      //      break;
      //   }
      //}
      if (qa == 0) continue;
#endif

      field_index_I = time_index_I*4+field_index;
      //F = E[field_index_I]->flist[k];
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
        static int FLAG_INDEX = 0;

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
            fprintf(stdout,"e(%d,%d)=%f\n",FLAG_INDEX+1,i+1,w[i]);
        }


        FILE *pFile;
        char fname[256];
        sprintf(fname,"EIG_VEC_%d.dat",FLAG_INDEX);
        pFile = fopen(fname,"w");


        for (i = 0;  i < n; ++i){
          for (info = 0;  info < n; ++info)
            fprintf(pFile," %f ",K[i][info]);
          fprintf(pFile,"  \n");
        }
        fclose(pFile);

        delete[] work;
        FLAG_INDEX++;

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


#if 0
  j = 4;
  for (field_index_I = 0; field_index_I < j; ++field_index_I)
    memset(V[field_index_I],'\0',nfields*sizeof(double));

  j = nfields-4;
  for (field_index_I = j; field_index_I < nfields; ++field_index_I)
    memset(V[field_index_I],'\0',nfields*sizeof(double));
#endif


#if 1
  j = nfields - Nmodes;
  for (field_index_I = 0; field_index_I < j; ++field_index_I)
    memset(V[field_index_I],'\0',nfields*sizeof(double));

  for (field_index_I = j+1; field_index_I < nfields; ++field_index_I)
    memset(V[field_index_I],'\0',nfields*sizeof(double));
#endif

 // memset(V[nfields-1],'\0',nfields*sizeof(double));


/*
  j = nfields - Nmodes;
  for (field_index_I = 0; field_index_I < j; ++field_index_I)
    memset(V[field_index_I],'\0',nfields*sizeof(double));
*/



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



/* write file in XML format. unstructured grid. Tetarahedral elements. binary output */
int write_vtk_file_XML(float *XYZ, float *UVWP, int *vert_ID_array, int Nverts, int Nelements,
                       int vert_ID_array_len, int Elemnt_type, char *file_name, int FLAG_DUMP_INDEX,
                       int FLAG_GEOM){


    int i,j,k,iel,Nvert_total,vert_ID_array_length, Nelements_total;


    /*  gather data on ROOT, then write  */

    int Ncores = numnodes();
    int MyRank = mynode();
    int VIZ_ROOT = 0;
    int *Nverts_per_core, *vert_ID_array_len_per_core;
/*
     if (FLAG_GEOM){
       FILE *pFile;
       char fname[256];
       sprintf(fname, "test_vert_array_ID.dat.%d",MyRank);
       pFile = fopen(fname,"w");

       for (i = 0; i < vert_ID_array_len; ++i)
   fprintf(pFile," %d \n", vert_ID_array[i]);

       fclose(pFile);
     }

*/
    Nverts_per_core            = new int[Ncores];
    vert_ID_array_len_per_core = new int[Ncores];

    MPI_Allgather (&Nverts,         1, MPI_INT,
       Nverts_per_core, 1, MPI_INT,
       get_MPI_COMM());

    MPI_Allgather (&vert_ID_array_len,         1, MPI_INT,
       vert_ID_array_len_per_core, 1, MPI_INT,
       get_MPI_COMM());

    MPI_Reduce(&Nelements, &Nelements_total,1,MPI_INT, MPI_SUM,
               VIZ_ROOT, get_MPI_COMM());

    MPI_Reduce(&Nverts, &Nvert_total,1,MPI_INT, MPI_SUM,
               VIZ_ROOT, get_MPI_COMM());

    float *XYZ_all,*UVW_all, *Pres_all;
    int *vert_ID_array_all;
    int *displs, *recvcnt;

    displs  = new int[Ncores];
    recvcnt = new int[Ncores];



    if (FLAG_GEOM){
      if (MyRank == VIZ_ROOT){
  XYZ_all = new float[Nvert_total*3];

        /* compute displasments and recv countes */
        displs[0] = 0;
        recvcnt[0] = 3*Nverts_per_core[0];
        for (i = 1; i < Ncores; ++i){
    displs[i] = displs[i-1]+recvcnt[i-1];
    recvcnt[i] = 3*Nverts_per_core[i];
  }
      }


      MPI_Gatherv ( XYZ, 3*Nverts, MPI_FLOAT,
        XYZ_all, recvcnt, displs,
        MPI_FLOAT,
        VIZ_ROOT, get_MPI_COMM() );


        /* gather connectivity */
      if (MyRank == VIZ_ROOT){

  vert_ID_array_length = vert_ID_array_len;
  for (i = 1; i < Ncores; ++i)
    vert_ID_array_length +=  vert_ID_array_len_per_core[i];

  vert_ID_array_all = new int[vert_ID_array_length];

        /* compute offsets and recv. countes */
        displs[0] = 0;
        recvcnt[0] = vert_ID_array_len_per_core[0];
        for (i = 1; i < Ncores; ++i){
    displs[i] = displs[i-1]+recvcnt[i-1];
    recvcnt[i] = vert_ID_array_len_per_core[i];
  }
      }

      /*      shift values of  the vert_IDs    */
      /*      compute offset */
      j = 0;
      for (i = 0; i < MyRank; ++i)
  j += Nverts_per_core[i];

      /*      shift vert. IDs  */
      for (i = 0; i < vert_ID_array_len; ++i)
  vert_ID_array[i] += j;

      MPI_Gatherv ( vert_ID_array, vert_ID_array_len, MPI_INT,
        vert_ID_array_all, recvcnt, displs,
        MPI_INT,
        VIZ_ROOT, get_MPI_COMM() );

    }


    if (MyRank == VIZ_ROOT){
      UVW_all  = new float[Nvert_total*3];
      /* compute displasments and recv countes */
      displs[0] = 0;
      recvcnt[0] = 3*Nverts_per_core[0];
      for (i = 1; i < Ncores; ++i){
  displs[i] = displs[i-1]+recvcnt[i-1];
  recvcnt[i] = 3*Nverts_per_core[i];
      }
    }
    /* gather velocity vector */
    MPI_Gatherv ( UVWP, 3*Nverts, MPI_FLOAT,
      UVW_all, recvcnt, displs,
      MPI_FLOAT,
      VIZ_ROOT, get_MPI_COMM() );




    if (MyRank == VIZ_ROOT){
      Pres_all = new float[Nvert_total];
      displs[0] = 0;
      recvcnt[0] = Nverts_per_core[0];
      for (i = 1; i < Ncores; ++i){
  displs[i] = displs[i-1]+recvcnt[i-1];
  recvcnt[i] = Nverts_per_core[i];
      }
    }
    /* gather pressure */
    MPI_Gatherv ( UVWP+3*Nverts, Nverts, MPI_FLOAT,
      Pres_all, recvcnt, displs,
      MPI_FLOAT,
      VIZ_ROOT, get_MPI_COMM() );


    delete[] displs;
    delete[] recvcnt;
    delete[] vert_ID_array_len_per_core;
    delete[] Nverts_per_core;

    if (MyRank != VIZ_ROOT)
      return;



    FILE *vert_id_file;
    char path_name[BUFSIZ];
    char vert_id_file_name[BUFSIZ];
    char* c;

     /* create directories for *vtu files */
     ROOTONLY{
        sprintf(path_name,"%s_time%d",file_name,FLAG_DUMP_INDEX);
        i = mkdir(path_name,0700);
        if (i != 0 )
          fprintf(stderr,"Error in creating new directory \"%s\" \n ",path_name);

        if (FLAG_GEOM){
          memset(path_name,'\0',BUFSIZ*sizeof(char));
          sprintf(path_name,"%s_geom",file_name);
          i = mkdir(path_name,0700);
          if (i != 0 )
            fprintf(stderr,"Error in creating new directory \"%s\" \n ",path_name);
        }

     }

/*
     if (FLAG_GEOM){
       FILE *pFile;
       pFile = fopen("test_vert_array_ID_all.dat","w");

       for (i = 0; i < vert_ID_array_length; ++i)
   fprintf(pFile," %d \n", vert_ID_array_all[i]);

       fclose(pFile);
     }
*/


     if (FLAG_GEOM){
       ostringstream xml_geom;
       /*  write header */
       xml_geom << string("<?xml version=\"1.0\"?>\n")
                << string("<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n")
                << string("<UnstructuredGrid>\n")
                << string("<Piece NumberOfPoints=\"")
                << Nvert_total << string("\" NumberOfCells=\"")
                << Nelements_total << string("\">\n")
       /*  write geometry data - coordinates */
                << string("<Points>\n")
                << string("<DataArray type=\"Float32\" Name=\"xyz\" NumberOfComponents=\"3\" format=\"appended\" offset=\"")
                << (sizeof(float)*Nvert_total*0+sizeof(int)*0) << string("\"/>\n")
                << string("</Points>\n")
       /*  map cells */
                << string("<Cells>\n")
                << string("<DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"appended\" offset=\"")
                << (sizeof(float)*Nvert_total*3+sizeof(int)*1) << string("\"/>\n")

                << string("<DataArray type=\"Int32\" Name=\"offsets\" format=\"appended\" offset=\"")
                << (sizeof(float)*Nvert_total*3+sizeof(int)*vert_ID_array_length+sizeof(int)*2) << string("\"/>\n")

                << string("<DataArray type=\"UInt8\" Name=\"types\"  format=\"appended\" offset=\"")
                << (sizeof(float)*Nvert_total*3+sizeof(int)*(vert_ID_array_length+Nelements_total)+sizeof(int)*3)
                << string("\"/>\n")
                << string("</Cells>\n")
                << string("</Piece>\n")
                << string("</UnstructuredGrid>\n")
                << string("<AppendedData encoding=\"raw\">\n_");

       string xml_start_geom(xml_geom.str());
       int xml_start_length_geom = xml_start_geom.length();
       string xml_end_geom("</AppendedData>\n</VTKFile>\n");
       int xml_end_length_geom = xml_end_geom.length();

       // the total size of the binary section is the sum of the values of
       // "size" in the comment blocks preceding the section of code
       int binary_total_size_geom = 4 + sizeof(float)*Nvert_total*3 +
                                  4 + sizeof(int)*vert_ID_array_length +
                                  4 + sizeof(int)*Nelements_total +
                                  4 + sizeof(unsigned char)*Nelements_total;

       // the total length of the file in bytes
       int total_length_geom = xml_start_length_geom + binary_total_size_geom + xml_end_length_geom;

       // the xml file
       vector<char> buffer_geom(total_length_geom);
       // copy the strings first - the header and the tail as well
       vector<char>::iterator iter_geom = copy( xml_start_geom.begin(), xml_start_geom.end(), buffer_geom.begin() );
       copy( xml_end_geom.begin(), xml_end_geom.end(), buffer_geom.begin() + xml_start_length_geom + binary_total_size_geom );

       j = sizeof(float)*Nvert_total*3;
       c = reinterpret_cast<char*>(&j);
       iter_geom = copy( c, c + 4, iter_geom );
       c = reinterpret_cast<char*>(XYZ_all);
       iter_geom = copy( c, c + sizeof(float)*Nvert_total*3, iter_geom );

       j = sizeof(int)*vert_ID_array_length;
       c = reinterpret_cast<char*>(&j);
       iter_geom = copy( c, c + 4, iter_geom );


       /* connectivity */
       k = 0;
       for(iel = 0; iel < Nelements_total; iel++){
         for (i = 0; i < 4; i++){
           j = vert_ID_array_all[k];
           k++;
           c = reinterpret_cast<char*>(&j);
           iter_geom = copy( c, c + 4, iter_geom);
         }
       }

       j = sizeof(int)*Nelements_total;
       c = reinterpret_cast<char*>(&j);
       iter_geom = copy( c, c + 4, iter_geom );

       for(iel = 0; iel < Nelements_total; iel++){
         j = 4*(iel+1);
         c = reinterpret_cast<char*>(&j);
         iter_geom = copy( c, c + 4, iter_geom );
       }

       j = sizeof(unsigned char)*Nelements_total;
       c = reinterpret_cast<char*>(&j);
       iter_geom = copy( c, c + 4, iter_geom);

       // define a type of elements - for Tets use type=10
       char cj = 10;
       fill_n( iter_geom, Nelements_total, cj );


       ////////////////////////
       // Write geometry file
       ////////////////////////

       /* set name for *vtu file */

       sprintf(vert_id_file_name,"%s_geom/%s_%d.vtu",file_name, file_name,mynode());

       vert_id_file = fopen(vert_id_file_name,"wb");
       fwrite( &buffer_geom[0], 1, buffer_geom.size(), vert_id_file );
       fclose(vert_id_file);

       //////////////////////////////////////
       //// geometry is written /////////////
       //////////////////////////////////////

   }//end of "if (FLAG_GEOM) "



   /* set name for *vtu file */
   sprintf(vert_id_file_name,"%s_time%d/%s_%d.vtu",file_name,FLAG_DUMP_INDEX,file_name,mynode());

  /*  write header */

  ostringstream xml;

  xml << string("<?xml version=\"1.0\"?>\n")
      << string("<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\" geometry=\"static\"> \n")
      << string("<UnstructuredGrid>\n")
      << string("<Piece NumberOfPoints=\"")
      << Nvert_total << string("\" NumberOfCells=\"")
      << Nelements_total << string("\">\n")

  /* start section data at points  */
      << string("<PointData Scalars=\"Pressure\" Vector=\"Velocity\">\n ")

  /* write scalar data - Pressure */
      << string("<DataArray type=\"Float32\" Name=\"Pressure\" NumberOfComponents=\"1\" format=\"appended\" offset=\"0\"/>\n")

  /* write Vector data - Velocity */
      << string("<DataArray type=\"Float32\" Name=\"Velocity\" NumberOfComponents=\"3\" format=\"appended\" offset=\"")
      << (sizeof(float)*Nvert_total+sizeof(int)) << string("\"/>\n")

  /* end section data at points  */
      << string("</PointData>\n")
      << string("</Piece>\n")
      << string("</UnstructuredGrid>\n")
  /* write data */
  /* format: 1. write length of DataArray in bytes (j) 2. DataArray   */

      << string("<AppendedData encoding=\"raw\">\n_");

   string xml_start(xml.str());
   int xml_start_length = xml_start.length();

        //////////////////////////////////////
        // handle the end of the xml file
        //////////////////////////////////////

   string xml_end("</AppendedData>\n</VTKFile>\n");
   int xml_end_length = xml_end.length();

  //////////////////////////////////////////////////////////////////////
  // handle the "raw" binary section of the xml file
  // (note: techinically a valid xml document is NOT allowed to have
  // unencoded ie raw binary data)
  //////////////////////////////////////////////////////////////////////

  // the total size of the binary section is the sum of the values of
  // "size" in the comment blocks preceding the section of code
  int binary_total_size = 4 + sizeof(float)*Nvert_total +
                          4 + sizeof(float)*Nvert_total*3;

  // the total length of the file in bytes
  int total_length = xml_start_length + binary_total_size + xml_end_length;

  // the xml file
  vector<char> buffer(total_length);

  // copy the strings first - the header and the tail as well
  vector<char>::iterator iter = copy( xml_start.begin(), xml_start.end(), buffer.begin() );
  copy( xml_end.begin(), xml_end.end(), buffer.begin() + xml_start_length + binary_total_size );

  // size = 4 + sizeof(float)*Nvert_total
  // j is an int and sizeof(int) == 4 even for x86-64 systems
  j = sizeof(float)*Nvert_total;
  c = reinterpret_cast<char*>(&j);
  iter = copy( c, c + 4, iter );

  // write Scalar (pressure)
  c = reinterpret_cast<char*>(Pres_all);
  iter = copy( c, c + sizeof(float)*Nvert_total, iter );


  // size = 4 + sizeof(float)*Nvert_total*3
  j = sizeof(float)*Nvert_total*3;
  c = reinterpret_cast<char*>(&j);
  iter = copy( c, c + 4, iter );


  c = reinterpret_cast<char*>(UVW_all);
  iter = copy( c, c + sizeof(float)*Nvert_total*3, iter );

  vert_id_file = fopen(vert_id_file_name,"wb");
  fwrite( &buffer[0], 1, buffer.size(), vert_id_file );
  fclose(vert_id_file);

  if (FLAG_GEOM)
    delete[] XYZ_all;
  delete[] UVW_all;
  delete[] Pres_all;

}




#ifdef PDIO
void write_pvd_file(int Ndumps, Domain *omega, pdio_info_t info) {
#else
//void write_pvd_file(int Ndumps, Domain *omega){
void write_pvd_file(int Ndumps, double *Time_all_steps, int *fieldID, char *file_name){
#endif

  int i,timestep, Ncpu;//, VTKSTEPS = iparam("VTKSTEPS");
  double timevalue = 0;

  int Ncores = 1;

  FILE *pvd_file;
  char pvd_file_name[BUFSIZ];

  sprintf(pvd_file_name,"%s.pvd",file_name);

  pvd_file = fopen(pvd_file_name,"w");

  ostringstream xml;
  int xml_start_length,xml_end_length,xml_core_length,xml_total_length;

  /*  create header */
  xml << string("<?xml version=\"1.0\"?>\n")
      << string("<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\" geometry=\"static\"> \n")
      << string("  <Collection> \n");

  string xml_start(xml.str());
  xml_start_length = xml_start.length();

  /* create tail */
  string xml_end("  </Collection>\n</VTKFile>\n");
  xml_end_length = xml_end.length();

  /* create core */
  string *xml_core;
  xml_core = new string[(Ndumps+1)*numnodes()];

  i=0;
  xml_core_length = 0;

  char *pch;
  char temp_fname[BUFSIZ];
  char fname_short[BUFSIZ];
  sprintf(temp_fname,"%s",file_name);
  pch = strtok(temp_fname,"/");
  while ( pch != NULL){
    memset(fname_short,'\0',256*sizeof(char));
    sprintf(fname_short,"%s",pch);
    pch = strtok(NULL,"/");
  }


  for (Ncpu = 0; Ncpu < Ncores; Ncpu++){
    ostringstream xml_temp;
    xml_temp    << string("    <DataSet part=\"")
                << Ncpu        << string("\" file=\"")
                << file_name << string("_geom/")
                << fname_short << string("_")
                << Ncpu        << string(".vtu\"/> \n");

    xml_core[i] = xml_temp.str();
    xml_core_length += xml_core[i].length();

    i++;
  }


  for (timestep = 0; timestep < Ndumps; timestep++){

    timevalue = Time_all_steps[timestep];

    for (Ncpu = 0; Ncpu < Ncores; Ncpu++){

      ostringstream xml_temp;
#if 1
      xml_temp    << string("    <DataSet timestep=\"")
                  << timestep    << string("\" timevalue=\"")
                  << timevalue   << string("\" part=\"")
                  << Ncpu        << string("\" file=\"")
                  << file_name << string("_time")
                  << fieldID[timestep]    << string("/")
                  << fname_short << string("_")
                  << Ncpu        << string(".vtu\"/> \n");
#else
      xml_temp    << string("    <DataSet timestep=\"")
                  << timestep    << string("\" timevalue=\"")
                  << timevalue   << string("\" part=\"")
                  << Ncpu        << string("\" file=\"")
                  << file_name << string("_time")
                  << timestep    << string("/file_")
                  << Ncpu        << string(".vtu\"/> \n");
#endif

      xml_core[i] = xml_temp.str();
      xml_core_length += xml_core[i].length();

      i++;
    }
  }

  /* combine header, core and tail  */

  xml_total_length = xml_start_length+xml_end_length+xml_core_length;

  vector<char> buffer(xml_total_length);
  copy( xml_start.begin(), xml_start.end(), buffer.begin());

  int offset = xml_start_length;
  for (i = 0; i < (Ndumps+1)*numnodes(); i++){
     copy( xml_core[i].begin(), xml_core[i].end(), buffer.begin()+offset);
     offset += xml_core[i].length();
  }
  copy( xml_end.begin(), xml_end.end(), buffer.begin()+offset);

  /* output */
  fwrite( &buffer[0], 1, buffer.size(), pvd_file );
  fclose(pvd_file);


#ifdef PDIO
  if (iparam("RMTHST")) {
    int      rank = 1;
    ssize_t  rc;
    offset = 0;
    snprintf(info.rfile_name, MAXPATHLEN, "%s", pvd_file_name);
    rc = pdio_write(&buffer[0], buffer.size(), offset, &info);
    if (rc != buffer.size()) {
      fprintf(stderr, "%d: pdio_write failed (%ld)\n", rank, rc);
      delete[] xml_core;
      return;
    }
  }
#endif

  delete[] xml_core;

}
