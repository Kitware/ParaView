#include <mpi.h>
#include <nektar.h>
#include <gen_utils.h>
#include <string.h>

#ifdef PARALLEL
int  do_main (int argc, char *argv[], MPI_Comm my_comm);
void init_comm(int *argc, char ***argv);
#else
int do_main (int argc, char *argv[]);
#endif

int  setup (FileList *f, Element_List **U, int *nftot, int Nsnapshots);
void ReadCopyField (FileList *f, Element_List **U);
void WriteVTK(Element_List **E, char *fname_vtk, int nfields, int Snapshot_index, int FLAG_GEOM);


#ifdef PARALLEL
/* function form comm_split.C*/
MPI_Comm get_MPI_COMM();
#endif


//local functions
static void parse_util_args (int npoint_per_edge);


int main(int argc, char *argv[]){

#ifdef PARALLEL
  init_comm(&argc, &argv);
  Element_List **master;
  int nfields,points_per_edge;
  FileList       f;
  char FNAME[BUFSIZ];


  sprintf(FNAME,"%s","KOVAZ_PZ_AB");

  // mesh file
  // expecting a name without extension
  // assumption is that the extension is ".rea"

  fprintf(stderr,"FNAME = %s\n",FNAME);


  memset (&f, '\0', sizeof(FileList));

  f.in.fp    =  stdin;
  f.out.fp   =  stdout;
  f.mesh.fp  =  stdin;

  f.rea.name = new char[BUFSIZ];
  f.in.name = new char[BUFSIZ];

  sprintf(f.rea.name,"%s.rea",FNAME);


  f.rea.fp = fopen(f.rea.name,"r");
  if (!f.rea.fp) error_msg(Restart: no REA file read);


  // data file
  // may need to provide additional path to the data file
  // for example if such is stored in a subdirectory
  sprintf(f.in.name,"%s_%d.rst",FNAME,39);


  f.in.fp = fopen(f.in.name,"r");
  if (!f.in.fp) error_msg(Restart: no dumps read from restart file);


  points_per_edge = 3;
  nfields = 4;

  //set up some parameters
  parse_util_args(points_per_edge);

  //read mesh file and header of the  data file
  master  = (Element_List **) malloc((2)*nfields*sizeof(Element_List *));
  nfields = setup (&f, master, &nfields, 1);

  /* read the field data */
  ReadCopyField(&f,master);


  /* transform field into physical space */
   for(int i = 0; i < nfields; ++i)
     master[i]->Trans(master[i],J_to_Q);

   int FLAG_GEOM = 1;
  // sprintf(FNAME,"VEL_DATA_%d/%s_%d.dat",39,f.in.name,0);
   //f.out.fp = fopen(file_name,"w");
   WriteVTK(master, f.in.name, 4, 39, FLAG_GEOM);
   FLAG_GEOM = 0;
   //fclose(f.out.fp);





  fprintf(stderr,"start do_main\n");
//  do_main (argc, argv,get_MPI_COMM());

  MPI_Finalize();

#else
  do_main (argc, argv);
#endif

  return 0;

}

static void parse_util_args (int npoint_per_edge)
{

   manager_init();       /* initialize the symbol table manager */

   option_set("FEstorage",1);
   option_set("iterative",1);
   iparam_set("NORDER.REQ",npoint_per_edge);
}

#if 0
static int  setup (FileList *f, Element_List **U, int *nft, int Nsnapshots){
  int    i,shuff;
  int    nfields,nftot;
  Field  fld;
  extern Element_List *Mesh;

  Gmap *gmap;

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
#endif
