
#include "nektar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <veclib.h>

#ifdef PARALLEL

MPI_Comm get_MPI_COMM();

MPI_Comm MPI_COMM_SPLIT = MPI_COMM_NULL;

MPI_Comm MPI_COMM_BC[10];


void unreduce (double *x, int n);
void reduce   (double *x, int n, double *work);
int get_num_of_subjobs(FILE *pFile);
int get_Ncpu_per_subjob(int *Ncpu_per_subjob, FILE *pFile);
int get_my_color(int mytid, int num_of_subjobs, int *Ncpu_per_subjob);

void GatherBlockMatrices_LowEnergy_Face_Modes2(Element *U, Bsystem *B);
int  test_if_face_is_sheared(int FaceID_global, int *partner, int *location);


static int numproc;
static int my_node;

void init_comm (int *argc, char **argv[])
{
  int info, nprocs,                      /* Number of processors */
      mytid;                             /* My task id */
  int multijob = 0;
  int num_of_subjobs = 1;
  int i;
  int color = 0;

  info = MPI_Init (argc, argv);                 /* Initialize */
  if (info != MPI_SUCCESS) {
    fprintf (stderr, "MPI initialization error\n");
    exit(1);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  /* gsync(); */

  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);         /* Number of processors */
  MPI_Comm_rank(MPI_COMM_WORLD, &mytid);          /* my process id */

  for( i = 0; i < *argc; ++i ){
      std::string s( (*argv)[i] );
      std::string mjb("-mjb");
      if ( s == mjb )
          multijob = 1;
  }

  if (multijob == 1){
    FILE *pFile;
    pFile = fopen(argv[0][argc[0]-1],"r");

    int *Ncpu_per_subjob;
    int num_of_subjobs = get_num_of_subjobs(pFile);

    if (mytid == 0)
      printf("num_of_subjobs = %d \n",num_of_subjobs);

    Ncpu_per_subjob = new int[num_of_subjobs];
    i = get_Ncpu_per_subjob(Ncpu_per_subjob,pFile);

    if (i != nprocs){
      fprintf(stderr,"get_Ncpu_per_subjob: errorin input file, Ncpu_total = %d, MPI_Comm_size = %d \n",i,nprocs);
      exit(1);
    }
    color = get_my_color(mytid,num_of_subjobs,Ncpu_per_subjob);

    fclose(pFile);
  }


  info = MPI_Comm_split ( MPI_COMM_WORLD, color, mytid, &MPI_COMM_SPLIT);
  if (info != MPI_SUCCESS) {
    fprintf (stderr, "MPI split error\n");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_SPLIT, &nprocs);         /* Number of processors */
  MPI_Comm_rank(MPI_COMM_SPLIT, &mytid);          /* my process id */

  pllinfo.nprocs = numproc = nprocs;
  pllinfo.procid = my_node = mytid;

  MPI_Barrier  (MPI_COMM_SPLIT);                  /* sync before work */

  return;
}

void create_comm_BC(int Nout, int *face_counter){

  int my_color,info,n_out;
  my_color = MPI_UNDEFINED;

  for (n_out = 0; n_out < Nout; n_out++){
    if (face_counter[n_out] != 0  || mynode() == 0  )
      my_color = 1;
  }

  info = MPI_Comm_split(get_MPI_COMM(), my_color, mynode(), &MPI_COMM_BC[0]);
  if (info != MPI_SUCCESS) {
      fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
      exit(1);
  }
  if (my_color != 1)
    MPI_COMM_BC[n_out]=MPI_COMM_NULL;

  for (n_out = 1; n_out < 10; n_out++)
     MPI_COMM_BC[n_out]=MPI_COMM_NULL;
}


void exit_comm(){
  MPI_Finalize();
  return;
}

void gsync ()
{
  int info;

  info = MPI_Barrier(MPI_COMM_SPLIT);

  return;
}


int numnodes ()
{
  int np;

  MPI_Comm_size(MPI_COMM_SPLIT, &np);         /* Number of processors */

  return np;
}


int mynode ()
{
  int myid;

  MPI_Comm_rank(MPI_COMM_SPLIT, &myid);          /* my process id */

  return myid;
}


void csend (int type, void *buf, int len, int node, int pid)
{

  MPI_Send (buf, len, MPI_BYTE, node, type, MPI_COMM_SPLIT);

  return;
}

void crecv (int typesel, void *buf, int len)
{
  MPI_Status status;

  MPI_Recv (buf, len, MPI_BYTE, MPI_ANY_SOURCE, typesel, MPI_COMM_SPLIT, &status);

  return;
}


void mpi_dsend (double *buf, int len, int send_to, int tag)
{
  MPI_Send(buf, len, MPI_DOUBLE, send_to, tag, MPI_COMM_SPLIT);
  return;
}

void mpi_isend (int *buf, int len, int send_to, int tag)
{
  MPI_Send (buf, len, MPI_INT, send_to, tag, MPI_COMM_SPLIT);
  return;
}


void mpi_drecv (double *buf, int len, int receive_from, int tag)
{
  MPI_Status status;
  MPI_Recv(buf, len, MPI_DOUBLE, receive_from, tag, MPI_COMM_SPLIT, &status);
  return;
}

void mpi_irecv (int *buf, int len, int receive_from, int tag)
{
  MPI_Status status;
  MPI_Recv(buf, len, MPI_INT, receive_from, tag, MPI_COMM_SPLIT, &status);
  return;
}



void msgwait (MPI_Request *request)
{
  MPI_Status status;

  MPI_Wait (request, &status);

  return;
}

#ifndef __LIBCATAMOUNT__
double dclock(void)
{
  double time;
  time = MPI_Wtime();
  return time;
}
#endif


void gimax (int *x, int n, int *work) {
  register int i;

  MPI_Allreduce (x, work, n, MPI_INT, MPI_MAX, MPI_COMM_SPLIT);

  /* *x = *work; */
  icopy(n,work,1,x,1);

  return;
}

void gdmax (double *x, int n, double *work)
{
  register int i;

  MPI_Allreduce (x, work, n, MPI_DOUBLE, MPI_MAX, MPI_COMM_SPLIT);

  /* *x = *work; */
  dcopy(n,work,1,x,1);

  return;
}


void gdsum (double *x, int n, double *work)
{
  register int i;

  MPI_Allreduce (x, work, n, MPI_DOUBLE, MPI_SUM, MPI_COMM_SPLIT);

  /* *x = *work; */
  dcopy(n,work,1,x,1);

  return;
}

void gisum (int *x, int n, int *work)
{
  register int i;

  MPI_Allreduce (x, work, n, MPI_INT, MPI_SUM, MPI_COMM_SPLIT);

  /* *x = *work; */
  icopy(n,work,1,x,1);

  return;
}

void ifexists(double *in, double *inout, int *n, MPI_Datatype *size);

void BCreduce(double *bc, Bsystem *Ubsys){
#if 1
  gs_gop(Ubsys->pll->known,bc,"A");
#else
  register int i;
  int      ngk = Ubsys->pll->nglobal - Ubsys->pll->nsolve;
  int      nlk = Ubsys->nglobal - Ubsys->nsolve;
  double *n1,*n2;
  static MPI_Op MPI_Ifexists = NULL;

  if(!MPI_Ifexists){
    MPI_Op_create((MPI_User_function *)ifexists, 1, &MPI_Ifexists);
  }

  n1 = dvector(0,ngk-1);
  n2 = dvector(0,ngk-1);

  memset(n1,'\0',ngk*sizeof(double));
  memset(n2,'\0',ngk*sizeof(double));

  /* fill n1 with values from bc  */
  for(i = 0; i < nlk; ++i) n1[Ubsys->pll->knownmap[i]] = bc[i];

  /* receive list from other processors and check against local */
  MPI_Allreduce (n1, n2, ngk, MPI_DOUBLE, MPI_Ifexists, MPI_COMM_SPLIT);
  /* fill bc with values values from  n1 */
  for(i = 0; i < nlk; ++i) bc[i] = n2[Ubsys->pll->knownmap[i]];

  free(n1);  free(n2);
#endif
}

void GatherBlockMatrices(Element *U,Bsystem *B){
  double *edge, *face;

  if(LGmax <=2) return;

  switch(B->Precon){
  case Pre_Block:
    edge  = B->Pmat->info.block.iedge[0];
    face  = B->Pmat->info.block.iface[0];
    break;
  case Pre_LEnergy:
    edge  = B->Pmat->info.lenergy.iedge[0];
    face  = B->Pmat->info.lenergy.iface[0];
    break;
  default:
    error_msg(Unknown preconditioner in GatherBlockMatrices);
    break;
  }
#if 0
  /* for testing only */
  /* STEP 1 */
  FILE  *pFile;
  char  Fname[256];
  static int FLAG_GatherBlockMatrices = 0;
  if (FLAG_GatherBlockMatrices == 0)
    sprintf(Fname,"Aface_PRESSURE_%d.dat",mynode());
  else
    sprintf(Fname,"Aface_VELOCITY_%d.dat",mynode());
  pFile = fopen(Fname,"w");
  int i,j,index = 0;

  for (i = 0; i < B->Pmat->info.lenergy.nface; i++){
     for (j = 0; j < B->Pmat->info.lenergy.Lface[i]; j++){
          fprintf(pFile," %f ",face[index]);
          index++;
     }
    fprintf(pFile," \n");
  }
  fclose(pFile);

  /*   end of  STEP 1 */
#endif


  gs_gop(B->egather,edge,"+");

  free(B->egather);

  //gs_gop(B->fgather,face,"+");
  GatherBlockMatrices_LowEnergy_Face_Modes2(U,B);

#if 0
  /* STEP 2 */
  if (FLAG_GatherBlockMatrices == 0)
    sprintf(Fname,"Bface_PRESSURE_%d.dat",mynode());
  else
    sprintf(Fname,"Bface_VELOCITY_%d.dat",mynode());
  pFile = fopen(Fname,"w");
  index = 0;
  for (i = 0; i < B->Pmat->info.lenergy.nface; i++){
     for (j = 0; j < B->Pmat->info.lenergy.Lface[i]; j++){
          fprintf(pFile," %f ",face[index]);
          index++;
     }
    fprintf(pFile," \n");
  }
  fclose(pFile);
  FLAG_GatherBlockMatrices++;
  /*   end of  STEP 2 */
#endif
}

void GatherBlockMatrices_LowEnergy_Face_Modes2(Element *U, Bsystem *B){

/*   Author: Leopold Grinberg */

  int i_proc, i_face, error_mpi, index, l;
  int partner, location;
  int ElementID_LOC, FaceID_LOC,Face_GID;
  int *counter, *Lface;
  int *message_length;
  double **iface;
  double ***message;
  int ***mapping;

  Face *f;
  Element *E;
  extern  Element_List *Mesh;

  switch(B->Precon){
  case Pre_Block:
    iface  = B->Pmat->info.block.iface;
    Lface  = B->Pmat->info.lenergy.Lface;
    break;
  case Pre_LEnergy:
    iface  = B->Pmat->info.lenergy.iface;
    Lface  = B->Pmat->info.lenergy.Lface;
    break;
  default:
    error_msg(Unknown preconditioner in GatherBlockMatrices_LowEnergy_Face_Modes);
    break;
  }

 /* allocate memory for messages */

  mapping = new int**[pllinfo.ncprocs];
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc)
    mapping[i_proc] = new int*[pllinfo.cinfo[i_proc].nedges];

  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc)
    for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face)
      mapping[i_proc][i_face] = new int[3];

  counter = new int[pllinfo.ncprocs];
  memset(counter,'\0',pllinfo.ncprocs*sizeof(int));
  message_length = new int[pllinfo.ncprocs];
  memset(message_length,'\0',pllinfo.ncprocs*sizeof(int));

  /* identify which faces are sheared, get a list of sheared faces and locations */
  /* compute total length of messages per partner */
  for(E=U;E; E = E->next){
    for(i_face = 0; i_face < E->Nfaces; ++i_face){
      f = E->face + i_face;
      if (f->gid >= B->nf_solve) continue;

      Face_GID = Mesh->flist[pllinfo.eloop[f->eid]]->face[i_face].gid;
      if (test_if_face_is_sheared(Face_GID , &partner, &location)){
        mapping[partner][counter[partner]][0] = f->gid;
        mapping[partner][counter[partner]][1] = Face_GID;
        counter[partner]++;
        l = Lface[f->gid];
        message_length[partner] += l*(l+1)/2;
      }
    }
  }
  /* allocate memory for messages */
  message = new double**[pllinfo.ncprocs];
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    message[i_proc] = new double*[pllinfo.cinfo[i_proc].nedges];
    message[i_proc][0] = new double[message_length[i_proc]];
  }
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
      f = Mesh->flist[pllinfo.cinfo[i_proc].elmtid[i_face]]->face + pllinfo.cinfo[i_proc].edgeid[i_face];
      Face_GID = Mesh->flist[pllinfo.eloop[f->eid]]->face[pllinfo.cinfo[i_proc].edgeid[i_face]].gid;
      for ( index = 0; index < pllinfo.cinfo[i_proc].nedges; ++index){
        if (mapping[i_proc][index][1] == Face_GID){
          mapping[i_proc][i_face][2] = mapping[i_proc][index][0];
          break;
        }
      }
    }
  }
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    for (i_face = 1; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
      l = Lface[mapping[i_proc][i_face-1][2]];
      message[i_proc][i_face] = message[i_proc][i_face-1]+l*(l+1)/2;
    }
  }

  /* fill message and exchange with partner-cpu */
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
      FaceID_LOC = mapping[i_proc][i_face][2];
      l = Lface[FaceID_LOC];
      l = l*(l+1)/2;
      memcpy(message[i_proc][i_face],iface[FaceID_LOC],l*sizeof(double));
    }
    SendRecvRep(message[i_proc][0],message_length[i_proc]*sizeof(double),pllinfo.cinfo[i_proc].cprocid);
  }

  /* update local values */
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
      FaceID_LOC = mapping[i_proc][i_face][2];
      l = Lface[FaceID_LOC];
      l = l*(l+1)/2;
      for (index = 0; index < l; ++index)
        iface[FaceID_LOC][index] += message[i_proc][i_face][index];
    }
  }

  /* clean memory */
  delete[] counter;
  delete[] message_length;
  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
      delete mapping[i_proc][i_face];
    }
   delete mapping[i_proc];
   delete message[i_proc];
  }
  delete[] mapping;
  delete[] message;
}

int test_if_face_is_sheared(int FaceID_global, int *partner, int *location){

  int i_proc, i_face;
  Face *f;
  extern  Element_List *Mesh;
  /* check if Global id is in a list of faces on interfaces */
  /* return one if yes, zero if no. */
  /* pertner - index of partition that has the same global face ID in the list of "partners"  */
  /* location - is index of face in   pllinfo.cinfo[location].nedges */

  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
    for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
      f = Mesh->flist[pllinfo.cinfo[i_proc].elmtid[i_face]]->face + pllinfo.cinfo[i_proc].edgeid[i_face];
      if  (FaceID_global == Mesh->flist[pllinfo.eloop[f->eid]]->face[pllinfo.cinfo[i_proc].edgeid[i_face]].gid){
        partner[0] = i_proc;
        location[0] = i_face;
        return 1;
      }
    }
  }
  return 0;
}


void test_face_on_interface(Element_List *Mesh);
void test_interface_mapping(Element *U, Element_List *Mesh);

void Set_Comm_GatherBlockMatrices(Element *U, Bsystem *B){
  register int i,j;
  int nes = B->ne_solve;
  int nfs = B->nf_solve;
  int nel = B->nel;
  int l, *map, start, one=1, Lskip, *pos, *Ledge, *Lface;
  double *edge, *face;
  Edge   *e;
  Face   *f;
  Element *E;
  extern  Element_List *Mesh;

  if(LGmax <=2) return;

  switch(B->Precon){
  case Pre_Block:
    Ledge = B->Pmat->info.block.Ledge;
    Lface = B->Pmat->info.block.Lface;
    break;
  case Pre_LEnergy:
    Ledge = B->Pmat->info.lenergy.Ledge;
    Lface = B->Pmat->info.lenergy.Lface;
    break;
  default:
    error_msg(Unknown preconditioner in GatherBlockMatrices);
    break;
  }

  pos = ivector(0,max(nes,nfs));

  /* make up numbering list based upon solvemap */
  /* assumed fixed L order */

  pos[0] = 0;
  for(i = 1; i < nes+1; ++i)
    pos[i] = pos[i-1] + Ledge[i-1]*(Ledge[i-1]+1)/2;

  map = ivector(0,pos[nes]);

  Lskip = LGmax-2;
  Lskip = Lskip*(Lskip+1)/2;
  for(E=U; E; E = E->next)
    for(j = 0; j < E->Nedges; ++j){
      e = E->edge + j;
      if(e->gid < nes){
  /* allocate starting location based on global mesh */
  start = Mesh->flist[pllinfo.eloop[e->eid]]->edge[j].gid*Lskip;
  l     = Ledge[e->gid];
  l     = l*(l+1)/2;
  iramp(l,&start,&one,map+pos[e->gid],1);
      }
    }

  B->egather = gs_init(map,pos[nes],option("GSLEVEL"));
  free(map);


#if 0
  pos[0] = 0;
  for(i = 1; i < nfs+1; ++i)
    pos[i] = pos[i-1] +  Lface[i-1]*(Lface[i-1]+1)/2;

  map = ivector(0,pos[nfs]);

  Lskip = (LGmax-2)*(LGmax-2);
  Lskip = Lskip*(Lskip+1)/2;
  for(E=U;E; E = E->next)
    for(j = 0; j < E->Nfaces; ++j){
      f = E->face + j;
      if(f->gid < nfs){
  /* allocate starting location based on global mesh */
  start = Mesh->flist[pllinfo.eloop[f->eid]]->face[j].gid*Lskip;
  l     = Lface[f->gid];
  l     = l*(l+1)/2;
  iramp(l,&start,&one,map+pos[f->gid],1);
      }
    }
  B->fgather = gs_init(map,pos[nfs],option("GSLEVEL"));

  free(map);
#endif

  free(pos);
}


void test_face_on_interface(Element_List *Mesh){
  /* this function works properly for pressure solver only */

  int i_face,i_vert,nv_solve2,solve_mask;
  int nFaces_total = 0, nFaces_interface = 0;
  Element *E;
  int my_rank = mynode();

  for(E=Mesh->fhead;E; E = E->next){
     if (pllinfo.partition[E->id] != my_rank) continue;
     for(i_face = 0; i_face < E->Nfaces; ++i_face){
       nFaces_total++;
       nv_solve2 = 0;
       for (i_vert = 0; i_vert < E->Nfverts(i_face); ++i_vert){
         solve_mask = E->vert[E->vnum(i_face,i_vert)].solve;
         if (solve_mask == 2) nv_solve2++;
       }
       if  (nv_solve2 == E->Nfverts(i_face) )
          nFaces_interface++;
     }
  }

  fprintf(stderr,"rank = %d: nFaces_total = %d  nFaces_interface = %d \n",
                  mynode(),  nFaces_total,      nFaces_interface);
}

void test_interface_mapping(Element *U, Element_List *Mesh){

/* MEMO */
/*
pllinfo.ncprocs is the number of connecting processors to this partition
pllinfo.cinfo[i] is the connecting face information (i is the number of the connecting partition)
pllinfo.cinfo[i].nedges is the number of connecting faces
pllinfo.cinfo[i].elmtid[j] is the local element of data in order of the connection
pllinfo.cinfo[i].edgeid[j] is the local connecting face (it was originally written in 2D so refers to edges).
*/

  int i_proc,i_face,gid;
  int ElementID_LOC, FaceID_LOC;
  Element *E;
  Face *f;

  FILE *pFile;
  char Fname[256];
  static int FLAG_test_interface_mapping = 0;
  if (FLAG_test_interface_mapping == 0)
    sprintf(Fname,"Face_GID_PRESSURE_%d.dat",mynode());
  else
    sprintf(Fname,"Face_GID_VELOCITY_%d.dat",mynode());
  pFile = fopen(Fname,"w");

  for (i_proc = 0; i_proc < pllinfo.ncprocs; ++i_proc){
     fprintf(pFile," Partner proc ID = %d \n", pllinfo.cinfo[i_proc].cprocid);
     fprintf(pFile," Global IDs: \n");
     for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
        f = Mesh->flist[pllinfo.cinfo[i_proc].elmtid[i_face]]->face + pllinfo.cinfo[i_proc].edgeid[i_face];
        gid = Mesh->flist[pllinfo.eloop[f->eid]]->face[pllinfo.cinfo[i_proc].edgeid[i_face]].gid;
        fprintf(pFile," gid = %d f->eid = %d, Eid = %f",gid,f->eid,Mesh->flist[pllinfo.eloop[f->eid]]->id );
     }
     fprintf(pFile,"  \n");
     fprintf(pFile," Local IDs: \n");
     for (i_face = 0; i_face < pllinfo.cinfo[i_proc].nedges; ++i_face){
        f = Mesh->flist[pllinfo.cinfo[i_proc].elmtid[i_face]]->face + pllinfo.cinfo[i_proc].edgeid[i_face];
        ElementID_LOC =  Mesh->flist[pllinfo.cinfo[i_proc].elmtid[i_face]]->id;
        FaceID_LOC    = pllinfo.cinfo[i_proc].edgeid[i_face];

        for(E = U; E; E= E->next){
    if (E->id == ElementID_LOC){
      fprintf(pFile," %d ( %d, %d )  ",f->eid, E->id,Mesh->flist[pllinfo.eloop[f->eid]]->face[FaceID_LOC].gid);
      break;
    }
  }


        //fprintf(pFile," %d (%d, %d)  ",f->gid, E->id,FaceID_LOC);

        //fprintf(pFile," %d (%d)  ",f->gid, U[ElementID_LOC].face[FaceID_LOC].gid);
     }
     fprintf(pFile,"  \n");


  }
  fclose(pFile);
  FLAG_test_interface_mapping = 1;
}


void unreduce (double *x, int n)
{
  int nprocs = numnodes(),
      pid    = mynode(),
      k;

  ROOTONLY
    for (k = 1; k < nprocs; k++)
      csend (MSGTAG + k, x, n*sizeof(double), k, 0);
  else
    crecv (MSGTAG + pid, x, n*sizeof(double));

  return;
}

void reduce (double *x, int n, double *work)
{
  int nprocs = numnodes(),
      pid    = mynode(),
      k, i;

  ROOTONLY {
    for (i = 0; i < n; i++) work[i] = x[i];
    for (k = 1; k < nprocs; k++) {
      crecv (MSGTAG + k, x, n*sizeof(double));
      for (i = 0; i < n; i++) work[i] += x[i];
    }
    for (i = 0; i < n; i++) x[i] = work[i];
  } else
    csend (MSGTAG + pid, x, n*sizeof(double), 0, 0);

  return;
}

void ifexists(double *in, double *inout, int *n, MPI_Datatype *size){
  int i;

  for(i = 0; i < *n; ++i)
    inout[i] = (in[i] != 0.0)? in[i] : inout[i];

}

void parallel_gather(double *w, Bsystem *B){
  gs_gop(B->pll->solve,w,"+");
}



#ifdef METIS /* redefine default partitioner to be metis */


extern "C"
{
  void METIS_PartGraphRecursive(int &, int *, int *, int *, int *, int &,
        int &, int &, int *, int *, int *);
}

static void pmetis(int &nel, int *xadj, int *adjncy, int *vwgt,
       int *ewgt, int& wflag, int& nparts,int *option,
       int &num, int* edgecut,int *partition){

  METIS_PartGraphRecursive(nel,xadj,adjncy,vwgt,ewgt,wflag,
         num, nparts,option,edgecut,partition);
}

void default_partitioner(Element_List *EL, int *partition){
  register int i,j;
  int eDIM  = EL->fhead->dim();
  int nel = EL->nel;
  int medg,edgecut,cnt;
  int *xadj, *adjncy;
  int opt[5];
  Element *E;

  ROOTONLY
    fprintf(stdout,"Partitioner         : using pmetis \n");

  /* count up number of local edges in patch */
  medg =0;
  if(eDIM == 2)
    for(E = EL->fhead; E; E= E->next){
      for(j = 0; j < E->Nedges; ++j)
  if(E->edge[j].base) ++medg;
    }
  else
    for(E = EL->fhead; E; E= E->next){
      for(j = 0; j < E->Nfaces; ++j)
  if(E->face[j].link) ++medg;
    }

  xadj      = ivector(0,nel);
  adjncy    = ivector(0,medg-1);

  izero(nel+1,xadj,1);
  cnt = 0;
  if(eDIM == 2)
    for(i = 0; i < nel; ++i){
      E = EL->flist[i];
      xadj[i+1] = xadj[i];
      for(j = 0; j < E->Nedges; ++j){
  if(E[i].edge[j].base){
    if(E[i].edge[j].link){
      adjncy[cnt++] = E->edge[j].link->eid;
      xadj[i+1]++;
    }
    else{
      adjncy[cnt++] = E->edge[j].base->eid;
      xadj[i+1]++;
    }
  }
      }
    }
  else
    for(i = 0; i < nel; ++i){
      E = EL->flist[i];
      xadj[i+1] = xadj[i];
      for(j = 0; j < E->Nfaces; ++j)
  if(E->face[j].link){
    adjncy[cnt++] = E->face[j].link->eid;
    xadj[i+1]++;
  }
    }

  opt[0] = 0;
  int num, wflag;
  num = wflag = 0;
  pmetis(nel,xadj,adjncy,0,0,wflag,pllinfo.nprocs,opt,num,
   &edgecut,partition);

  free(xadj); free(adjncy);
}
#endif

/* gather edges from other patches */
void exchange_sides(int Nfields, Element_List **Us){
  register int   i,j,k,n;
  int            cnt, qface, qedg, *lid;
  int            ncprocs = pllinfo.ncprocs;
  ConInfo        *cinfo = pllinfo.cinfo;
  static double  **buf;

  if(!buf){
    int lenmax = 0;
    buf = (double **)malloc(ncprocs*sizeof(double *));
    for(i = 0; i < ncprocs; ++i){
      lenmax = max(lenmax,Nfields*cinfo[i].datlen-1);
      buf[i] = dvector(0,Nfields*cinfo[i].datlen-1);
    }
  }

  if(Us[0]->fhead->dim() == 2){
    Edge           *e;
    /* fill up data buffer and send*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
  for(j = 0; j < cinfo[i].nedges; ++j){
    e = Us[n]->flist[cinfo[i].elmtid[j]]->edge + cinfo[i].edgeid[j];
    qedg = e->qedg;
    dcopy(qedg,e->h,1,buf[i] + cnt,1);
    cnt += qedg;
  }
      SendRecvRep(buf[i],Nfields*cinfo[i].datlen*sizeof(double),
      cinfo[i].cprocid);
    }

    /* unpack*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
  for(j = 0; j < cinfo[i].nedges; ++j){
    e = Us[n]->flist[cinfo[i].elmtid[j]]->edge[cinfo[i].edgeid[j]].link;
    qedg = e->qedg;
    dcopy(qedg,buf[i] + cnt,1,e->h,1);
    cnt += qedg;
  }
    }
  }
  else{
    Face  *f;
    /* fill up data buffer and send*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
  for(j = 0; j < cinfo[i].nedges; ++j){
    f = Us[n]->flist[cinfo[i].elmtid[j]]->face + cinfo[i].edgeid[j];

    if(Us[n]->flist[cinfo[i].elmtid[j]]->Nfverts(f->id) == 3)
      lid =  Tri_nmap(f->qface,f->con);
    else
      lid = Quad_nmap(f->qface,f->con);

    qface = f->qface*f->qface;
    for(k=0;k<qface;++k)
      buf[i][cnt+k] = f->h[lid[k]];
    cnt += qface;
  }

      SendRecvRep(buf[i],Nfields*cinfo[i].datlen*sizeof(double),
      cinfo[i].cprocid);
    }

    /* unpack*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
  for(j = 0; j < cinfo[i].nedges; ++j){
    f = Us[n]->flist[cinfo[i].elmtid[j]]->face[cinfo[i].edgeid[j]].link;
    qface = f->qface*f->qface;
    dcopy(qface,buf[i]+cnt,1,f->h,1);
    cnt += qface;
    if(f->con)
      fprintf(stderr,"Face con not zero in elmt %id, face %id\n",
        f->eid,f->id);
  }
    }
  }
}

void SendRecvRep(void *buf, int len, int proc){
  MPI_Status status;

  MPI_Sendrecv_replace(buf, len, MPI_BYTE, proc, MSGTAG+pllinfo.procid, proc,
           MSGTAG+proc, MPI_COMM_SPLIT, &status);
}


MPI_Comm get_MPI_COMM(){
  return MPI_COMM_SPLIT;
}



/* function checks how many jobs to run */
int get_num_of_subjobs(FILE *pFile){

  int Njobs;
  char buf[BUFSIZ];

  rewind(pFile);
  fgets(buf, BUFSIZ, pFile);
  if (sscanf(buf, "%d", &Njobs) != 1){
    fputs("get_num_of_subjobs: can't read # of subjobs", stderr);
    //exit(1);
  }
  rewind(pFile);
  return Njobs;
}

/* functions checks how many CPUs to assign per each job  */
int get_Ncpu_per_subjob(int *Ncpu_per_subjob, FILE *pFile){

   int i,Ncpu,Njobs,Ncpu_total = 0;
   char buf[BUFSIZ];
   rewind(pFile);
   fgets(buf, BUFSIZ, pFile);
   sscanf(buf, "%d", &Njobs);

   for (i = 0; i < Njobs; i++){
    fgets(buf, BUFSIZ, pFile);
    sscanf(buf, "%d", &Ncpu);
    Ncpu_per_subjob[i] = Ncpu;
    Ncpu_total += Ncpu;
   }
   return Ncpu_total;
}

/* function define a color for each job - every job gets unique color */
int get_my_color(int mytid, int num_of_subjobs, int *Ncpu_per_subjob){

   int color_local = -1,i;
   int upperlimit=0,lowerlimit = 0;

   for (i = 0; i < num_of_subjobs; i++){
     upperlimit = lowerlimit+Ncpu_per_subjob[i];
     if (mytid >= lowerlimit && mytid < upperlimit)
        color_local = i;
     lowerlimit = upperlimit;
   }

   return color_local;
}

#endif
