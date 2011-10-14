#include "nektar.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

/* function form comm_split.C*/
MPI_Comm get_MPI_COMM();


MPI_Comm MPI_COMM_COLUMN_NEW = MPI_COMM_NULL;
MPI_Comm MPI_COMM_ROW_NEW = MPI_COMM_NULL;

   /*  Functions associated with BLACS  */
extern "C"
{
void sl_init_( int& , const int&, const int &);

void blacs_pinfo_( int&, int&);
void blacs_get_( const int& , const int& , const int&);
void blacs_gridinit_( int&, char* , const int&, const int&);
void blacs_gridinfo_( const int&, const int &, const int &, int&, int&);
int  numroc_(const int&, const int&, const int&, const int& , const int& );
void descinit_(int*, const int&, const int&, const int&, const int&, const int&, const int&,
              const int& , const int&, const int&);

int ilcm_ (const int&,const int&);

void pdgetrf_(const int&, const int&,  double* , const int&,const int&, int*, int*, int&);
void pdgetrs_( const char&, const int&, const int&, double*, const int&, const int&, int*, int*, double*, const int&, const int&, int* , int&);
void pdgetri_(const int&, double*, const int&, const int&, int*, int*, double*, const int&, int*, const int&, int&);
void pdgemv_ (const char&, const int&, const int&, const double&, double*, const int&, const int&, int*, double*,
              const int&, const int&, int*, const int&, const double&, double*, const int&, const int&, int*, const int&);

void dgsum2d_( const int&, const char*, const char*, const int&, const int&, double*, const int&, const int&, const int& );
void dtrbs2d_( const int&, const char*, const char*, const int&, const int&, double*, const int&);
void dtrbr2d_( const int&, const char*, const char*, const int&, const int&, double*, const int&, const int&, const int&);
void blacs_barrier_( const int&, const char*);
}


void InitBLAC(Bsystem *Bsys){

  int i,j;
  int *BLACS_PARAMS;
  static int INIT_FLAG_COMM = 0;

  Bsys->Pmat->info.lenergy.DESC_ivert   = ivector(0,8);
  Bsys->Pmat->info.lenergy.DESC_rhs     = ivector(0,8);
  Bsys->Pmat->info.lenergy.BLACS_PARAMS = ivector(0,14);

  BLACS_PARAMS = Bsys->Pmat->info.lenergy.BLACS_PARAMS;

  BLACS_PARAMS[7] = Bsys->pll->nv_gpsolve;
  BLACS_PARAMS[8] = Bsys->pll->nv_gpsolve;

  /* determinate dimension of grid of CPUs  */
  get_proc_grid(pllinfo[get_active_handle()].nprocs, &i, &j);
  BLACS_PARAMS[3] = i;
  BLACS_PARAMS[4] = j;

  /* determinate size of basic block in global boundary operator */
  BLACS_PARAMS[9]  = get_block_size(BLACS_PARAMS[7], BLACS_PARAMS[3]);
  BLACS_PARAMS[10] = get_block_size(BLACS_PARAMS[8], BLACS_PARAMS[4]);

  if (BLACS_PARAMS[9] < BLACS_PARAMS[10])
     BLACS_PARAMS[10] = BLACS_PARAMS[9];
  else
     BLACS_PARAMS[9] = BLACS_PARAMS[10];

  blacs_gridinit_nektar(BLACS_PARAMS, Bsys->Pmat->info.lenergy.DESC_ivert, Bsys->Pmat->info.lenergy.DESC_rhs);

  /* allocate memory for local partition of ivert */
  /* since we use ScaLAPACK (fortran)- use transpose of  inva_LOC  */

  Bsys->Pmat->info.lenergy.ivert_local  = dmatrix(0,BLACS_PARAMS[12]-1,
                 0,BLACS_PARAMS[11]-1);

  memset(Bsys->Pmat->info.lenergy.ivert_local[0],'\0',BLACS_PARAMS[11]*
   BLACS_PARAMS[12]*sizeof(double));


  Bsys->Pmat->info.lenergy.ivert_ipvt = ivector(0,BLACS_PARAMS[7]-1);

  Bsys->Pmat->info.lenergy.map_row    = ivector(0,BLACS_PARAMS[11]-1);
  get_gather_map(BLACS_PARAMS,'r',Bsys->Pmat->info.lenergy.map_row);

  Bsys->Pmat->info.lenergy.col_displs = ivector(0,BLACS_PARAMS[3]-1);
  Bsys->Pmat->info.lenergy.col_rcvcnt = ivector(0,BLACS_PARAMS[3]-1);
  memset(Bsys->Pmat->info.lenergy.col_displs,'\0',BLACS_PARAMS[3]*sizeof(int));
  memset(Bsys->Pmat->info.lenergy.col_rcvcnt,'\0',BLACS_PARAMS[3]*sizeof(int));

   /* create row and column communicators by splitting */
   /* all processors construct 2D grid */
   if (INIT_FLAG_COMM == 0){
     int info;

     info = MPI_Comm_split(get_MPI_COMM(), BLACS_PARAMS[6], BLACS_PARAMS[5], &MPI_COMM_COLUMN_NEW);
     if (info != MPI_SUCCESS){
       fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
       exit(1);
     }
     info = MPI_Comm_split(get_MPI_COMM(), BLACS_PARAMS[5], BLACS_PARAMS[6], &MPI_COMM_ROW_NEW);
      if (info != MPI_SUCCESS) {
        fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
        exit(1);
     }
     INIT_FLAG_COMM = 1;
   }


  /* summary */
  for (i = 0; i < 13; i++)
     if (pllinfo[get_active_handle()].procid == 0)
       printf(" Ubsys->Gmat->BLACS_PARAMS[%d] = %d \n ",i,Bsys->Pmat->info.lenergy.BLACS_PARAMS[i]);

}


void  GatherScatterIvert(Bsystem *Bsys){

  int i,j;
  int *BLACS_PARAMS;
  double **ivert_local;
  SMatrix *SM,SM_cont;

  double start_time_GatherScatterIvert, end_time_GatherScatterIvert;
    //start_time_GatherScatterIvert_tmp, end_time_GatherScatterIvert_tmp;

  BLACS_PARAMS = Bsys->Pmat->info.lenergy.BLACS_PARAMS;
  ivert_local = Bsys->Pmat->info.lenergy.ivert_local;
  SM = Bsys->Pmat->info.lenergy.SM_local;


  /*  copy values from  SM_local  to invert_local  */
  update_inva_LOC(SM, BLACS_PARAMS, ivert_local, 'L');


  /***************************/
  /* innitialize ring pattern communication */
  /* create two send buffers - one for indeces and one for values*/
  /* communicate over all CPUs to get the "nnz" value */
  /* copy values from sparse matrix to this buffers and send them */


  int *nnz_per_rank,*temp_nnz_per_rank;
  nnz_per_rank = ivector(0,pllinfo[get_active_handle()].nprocs-1);
  izero(pllinfo[get_active_handle()].nprocs,nnz_per_rank,1);
  nnz_per_rank[pllinfo[get_active_handle()].procid] = SM[0].nz;

  temp_nnz_per_rank = ivector(0,pllinfo[get_active_handle()].nprocs-1);
  izero(pllinfo[get_active_handle()].nprocs,temp_nnz_per_rank,1);

  gisum (nnz_per_rank,pllinfo[get_active_handle()].nprocs,temp_nnz_per_rank);
  free(temp_nnz_per_rank);


  int *send_buf_Index,*send_buf_Jndex;
  double *send_buf_value;
  int index,partner_to_send_data,partner_to_receive_data;

  start_time_GatherScatterIvert = dclock();

  SM_cont.allocate_SMatrix_cont(SM[0].nz,1,1);
  memcpy(SM_cont.AA,SM[0].AA,SM[0].nz*sizeof(double));
  memcpy(SM_cont.IA,SM[0].IA,SM[0].nz*sizeof(int));
  memcpy(SM_cont.JA,SM[0].JA,SM[0].nz*sizeof(int));
  SM[0].deallocate_SMatrix();

  for (i = 1; i < pllinfo[get_active_handle()].nprocs; i++){
    send_buf_Index = ivector(0,SM_cont.nz*2-1);
    send_buf_Jndex = send_buf_Index + SM_cont.nz;
    send_buf_value = dvector(0,SM_cont.nz-1);

    memcpy (send_buf_Index,SM_cont.IA,SM_cont.nz*2*sizeof(int));
    memcpy (send_buf_value,SM_cont.AA,SM_cont.nz*sizeof(double));

    partner_to_send_data = pllinfo[get_active_handle()].procid+1;
    partner_to_receive_data =  pllinfo[get_active_handle()].procid-1;
    if (partner_to_send_data == pllinfo[get_active_handle()].nprocs)
      partner_to_send_data = 0;
    if (partner_to_receive_data == -1)
      partner_to_receive_data = pllinfo[get_active_handle()].nprocs-1;

    /* trace number of SM transfers in order to get nz value right */
    index = pllinfo[get_active_handle()].procid-i;
    if (index < 0)
      index = index+pllinfo[get_active_handle()].nprocs;

    j = SM_cont.nz; //length of massege to be sent

    SM_cont.deallocate_SMatrix_cont();
    SM_cont.allocate_SMatrix_cont(nnz_per_rank[index],1,1);

    if (pllinfo[get_active_handle()].procid == 0){
      mpi_isend (send_buf_Index,j*2,partner_to_send_data ,0);
      mpi_irecv (SM_cont.IA,SM_cont.nz*2,partner_to_receive_data,0);
    }
    else{
      mpi_irecv (SM_cont.IA,SM_cont.nz*2,partner_to_receive_data,0);
      mpi_isend (send_buf_Index,j*2,partner_to_send_data ,0);
    }

    if (pllinfo[get_active_handle()].procid == 0){
      mpi_dsend (send_buf_value,j,partner_to_send_data ,2);
      mpi_drecv (SM_cont.AA,SM_cont.nz,partner_to_receive_data,2);
    }
    else{
      mpi_drecv (SM_cont.AA,SM_cont.nz,partner_to_receive_data,2);
      mpi_dsend (send_buf_value,j,partner_to_send_data ,2);
    }


    /*  copy values from  SM_local  to invert_local  */
    update_inva_LOC(&SM_cont, BLACS_PARAMS, ivert_local, 'L');

    //if (pllinfo[get_active_handle()].procid == 0)
    //    printf(" pass No. %d completed ! \n", i);

    free(send_buf_Index);free(send_buf_value);

  }
  free(nnz_per_rank);
  SM_cont.deallocate_SMatrix_cont();

  end_time_GatherScatterIvert = dclock();
  ROOTONLY
     fprintf(stderr,"ring communication was done in : %f sec \n", end_time_GatherScatterIvert-start_time_GatherScatterIvert);


#if 0
            static int FLAG_INDEX_IVERT_M = 0;
            int itmp, jtmp;
            FILE *Fivert_local;
            char fname_ivert_local[128];
            sprintf(fname_ivert_local,"ivert_localM_%d_%d.dat",FLAG_INDEX_IVERT_M,mynode());
            Fivert_local = fopen(fname_ivert_local,"w");
            for (itmp = 0; itmp < BLACS_PARAMS[12]; itmp++){
              for (jtmp = 0; jtmp < BLACS_PARAMS[11]; jtmp++)
                fprintf(Fivert_local," %2.16f ",ivert_local[itmp][jtmp]);
              fprintf(Fivert_local," \n");
            }
            fclose(Fivert_local);


#endif

  /* compute LU decompisition and pivot vector,
     LU stored in "ivert_local", pivot stored in "ivert_ipvt  */

  start_time_GatherScatterIvert = dclock();

  blacs_pdgetrf_nektar(BLACS_PARAMS,
           Bsys->Pmat->info.lenergy.DESC_ivert,
           Bsys->Pmat->info.lenergy.ivert_ipvt,
                       ivert_local);

  end_time_GatherScatterIvert = dclock();

  ROOTONLY
     fprintf(stderr,"Parallel LU was done in : %f sec \n", end_time_GatherScatterIvert-start_time_GatherScatterIvert);



#if 1

/* invert operator  */

   start_time_GatherScatterIvert = dclock();
   blacs_pdgetri_nektar(BLACS_PARAMS,
                       Bsys->Pmat->info.lenergy.DESC_ivert,
                       Bsys->Pmat->info.lenergy.ivert_ipvt,
                       Bsys->Pmat->info.lenergy.ivert_local);

   end_time_GatherScatterIvert = dclock();

   ROOTONLY
     fprintf(stderr,"Operator Inversion was done in : %f sec \n", end_time_GatherScatterIvert-start_time_GatherScatterIvert);

#endif


#if 0
            sprintf(fname_ivert_local,"ivert_localIM_%d_%d.dat",FLAG_INDEX_IVERT_M,mynode());
            Fivert_local = fopen(fname_ivert_local,"w");
            for (itmp = 0; itmp < BLACS_PARAMS[12]; itmp++){
              for (jtmp = 0; jtmp < BLACS_PARAMS[11]; jtmp++)
                fprintf(Fivert_local," %2.16f ",Bsys->Pmat->info.lenergy.invA_local[itmp][jtmp]);
              fprintf(Fivert_local," \n");
            }
            fclose(Fivert_local);

            FLAG_INDEX_IVERT_M++;
#endif

}


void blacs_gridinit_nektar(int *BLACS_PARAMS, int *DESCA, int *DESCB){
  int i,j,k;

  /*
   BLACS_PARAMS:
   [0]  = ictxt;
   [1]  = my_proc;
   [2]  = total_procs;
   [3]  = Nproc_row;
   [4]  = Nproc_col;
   [5]  = my_row;
   [6]  = my_col;
   [7]  = Global_rows;
   [8]  = Global_columns;
   [9]  = Block_Size_row;
   [10] = Block_Size_col;
   [11] = LOC_rows;
   [12] = LOC_columns;
   */


   blacs_pinfo_( i, j);
   BLACS_PARAMS[1] = i;
   BLACS_PARAMS[2] = j;

   blacs_get_( -1, 0,  k);   // <- LG check the arguments of this call
   BLACS_PARAMS[0] = k;

   i = BLACS_PARAMS[3];
   j = BLACS_PARAMS[4];
   blacs_gridinit_( BLACS_PARAMS[0], "Row", i, j);

   blacs_gridinfo_( BLACS_PARAMS[0] ,BLACS_PARAMS[3], BLACS_PARAMS[4], i, j);
   BLACS_PARAMS[5] = i;
   BLACS_PARAMS[6] = j;

   i = 0;
   BLACS_PARAMS[11] = numroc_(BLACS_PARAMS[7],BLACS_PARAMS[9], BLACS_PARAMS[5],i,BLACS_PARAMS[3]);
   BLACS_PARAMS[12] = numroc_(BLACS_PARAMS[8],BLACS_PARAMS[10],BLACS_PARAMS[6],i,BLACS_PARAMS[4]);

   i = 0;
   j = 0;
   descinit_(DESCA, BLACS_PARAMS[7], BLACS_PARAMS[8],
                    BLACS_PARAMS[9], BLACS_PARAMS[10],
                    i, j,
                    BLACS_PARAMS[0],BLACS_PARAMS[11],
                    k);
   if (k != 0)
     fprintf(stderr,"blacs_gridinit_nektar: ERROR, descinit(info) = %d \n",k);

   descinit_(DESCB, BLACS_PARAMS[7], 1,
                    BLACS_PARAMS[9], 1,
                    i, j,
                    BLACS_PARAMS[0],BLACS_PARAMS[11],
                    k);
  if (k != 0)
     fprintf(stderr,"blacs_gridinit_nektar: ERROR, descinit(info) = %d \n",k);


}

void blacs_pdgetrf_nektar(int *BLACS_PARAMS, int *DESCA, int *ipvt, double **inva_LOC){
  int row_start = 1, col_start = 1;
  int info;
  pdgetrf_(BLACS_PARAMS[7],BLACS_PARAMS[8],*inva_LOC,
             row_start,col_start,DESCA,ipvt,info);

  if (info != 0)
    fprintf(stderr,"blacs_pdgetrf_nektar: ERROR - info = %d \n",info);
}

void blacs_pdgetri_nektar(int *BLACS_PARAMS, int *DESCA, int *ipvt, double **inva_LOC){
  int row_start = 1, col_start = 1;
  int lwork,liwork,info = 0;
  double *work;
  int *iwork;
  int M;

  M = BLACS_PARAMS[7] + ((row_start-1) % BLACS_PARAMS[10]);
  lwork = BLACS_PARAMS[9]*numroc_( M, BLACS_PARAMS[10], BLACS_PARAMS[5], row_start, BLACS_PARAMS[3]);
  liwork = BLACS_PARAMS[7];

  work  = dvector(0,lwork-1);
  iwork = ivector(0,liwork-1);

  int i = -1, j = -1;
  pdgetri_(BLACS_PARAMS[7],*inva_LOC,
             row_start,col_start,DESCA,ipvt,
             work,i,
             iwork,j,
             info);

  if (info != 0)
    fprintf(stderr,"blacs_pdgetri_nektar: ERROR - info = %d \n",info);


  if ( ((int) work[0]) > lwork){
    lwork = (int) work[0];
    free(work);
    work = dvector(0,lwork);
  }


  if ( iwork[0] > lwork){
    liwork = iwork[0];
    free(iwork);
    iwork = ivector(0,liwork);
  }

  pdgetri_(BLACS_PARAMS[7],*inva_LOC,
             row_start,col_start,DESCA,ipvt,
             work,lwork,
             iwork,liwork,
             info);


  if (info != 0)
    fprintf(stderr,"blacs_pdgetri_nektar: ERROR - info = %d \n",info);


  free(work);
  free(iwork);

}


void blacs_pdgetrs_nektar(int *BLACS_PARAMS, int *DESCA, int *DESCB, double **A, int *ipvt, double *RHS){
  int row_start = 1, col_start = 1;
  char transa = 'N';
  int info;
  pdgetrs_(transa,BLACS_PARAMS[7],1,*A,row_start,col_start,DESCA,ipvt,RHS,row_start,col_start,DESCB,info);

  if (info != 0)
    fprintf(stderr,"blacs_pdgetrs_nektar: ERROR - info = %d \n",info);
}


void pdgemv_nektar(int *BLACS_PARAMS, int *DESCA, int *DESCB, double **A, int *ipvt, double *RHS){

  int row_start = 1, col_start = 1;
  char transa = 'N';
  double alpha = 1.0, beta = 0.0;
  int ix = 1, jx = 1, incx = 1;
  static int FLAG_INIT = 0;
  static double *result = dvector(0,1);
  static int size = 0;

  if (FLAG_INIT == 0 || size < BLACS_PARAMS[11]){
     free(result);
     size = BLACS_PARAMS[11];
     result = dvector(0,size-1);
     FLAG_INIT = 1;
  }

  memset(result,'\0',size*sizeof(double));

  pdgemv_(transa, BLACS_PARAMS[7], BLACS_PARAMS[8], alpha, *A, row_start, col_start, DESCA,
          RHS, ix, jx, DESCB, incx,
          beta, result, row_start, col_start, DESCB, incx);

  memcpy(RHS,result,BLACS_PARAMS[11]*sizeof(double));

}

void blacs_dgather_rhs_nektar(int *BLACS_PARAMS, double *A){

  int RDEST = BLACS_PARAMS[5];
  int CDEST = 0;
  int M = BLACS_PARAMS[7];
  int N = 1;
  int LDA = BLACS_PARAMS[7];
  int ICONTXT = BLACS_PARAMS[0];

  char scop_row[]    = "Row";
  char scop_column[] = "Column";
  //  char TOP[]         = "D";
  char TOP_I_tree[]    = "1-tree";

  CDEST = 0;
  /* sum within rows of proc. grid result -> first proc. in each row*/
  dgsum2d_( ICONTXT, scop_row,TOP_I_tree, M, N, A, LDA, RDEST, CDEST);

  /* sum the first column, result -> to first proc. in each row */

  RDEST = -1;
  CDEST = 0;
  if ( BLACS_PARAMS[6] == 0)
    dgsum2d_( ICONTXT, scop_column,TOP_I_tree, M, N, A, LDA, RDEST, CDEST);

}

void blacs_dscather_rhs_nektar(int *BLACS_PARAMS, double *A){


  /* 1. sum data on my_col = 0, ALL 2 ALL */
  /* 2. broadcast from my_col = 0 to the rest in the scop="ROW" */

  int ICONTXT = BLACS_PARAMS[0];
  int M = BLACS_PARAMS[7];
  int N = 1;
  int LDA = BLACS_PARAMS[7];

  char scop_row[]    = "Row";
  char scop_column[] = "Column";
  char TOP_I[]         = "I";
  char TOP_I_tree[]    = "1-tree";


  if ( BLACS_PARAMS[6] == 0 ){
    /* sum over my_col = 0 */
    int CDEST = 0;
    int RDEST = -1;
    //char scop_C[] = "C";

    dgsum2d_( ICONTXT, scop_column, TOP_I, M, N, A, LDA, RDEST, CDEST);
    dtrbs2d_( ICONTXT, scop_row, TOP_I_tree, M, N, A, LDA );

  }
  else{
    int RSRC = BLACS_PARAMS[5];
    int CSRC = 0;
    dtrbr2d_( ICONTXT, scop_row, TOP_I_tree,  M, N, A, LDA, RSRC, CSRC );
  }


}


/* internal functions */


int get_block_size(int length, int Nproc){


  int i, blocksize = 1;
  int *BS;

  BS = new int[9];

  BS[0] = 1;   BS[1] = 2;
  BS[2] = 4;   BS[3] = 8;
  BS[4] = 16;  BS[5] = 32;
  BS[6] = 64;  BS[7] = 80;
  BS[8] = 110; BS[9] = 200;

  for (i = 2; i >= 0; i--){
    if ( (int) (length/Nproc) >= BS[i]){
      blocksize = BS[i];
      break;
    }
  }

  delete[] BS;
  return blocksize;
}


void get_proc_grid(int Nprocs, int *nrow, int *ncol){

   nrow[0] = (int) sqrt( (double) Nprocs);
   while (nrow[0] >= 1){
     ncol[0] = Nprocs/nrow[0];
     if  ( (nrow[0]*ncol[0]) == Nprocs )
        break;
     nrow[0]--;
   }
}


void  update_inva_LOC( SMatrix *SM ,int *BLACS_PARAMS, double **inva_LOC, char storage_type){

  /* storage types:
     F - SM stores all non zero entries
     L - SM stores non zero entries of Low diagonal matrix including diagonal
     U - SM stores non zero entries of Upper diagonal matrix including diagonal - NOT IMPLEMENTED yet !!!
  */



  int i,i_loc,j_loc,lr,lc,Pr,Pc;
  int *IA,*JA;
  double *AA;

  IA = SM[0].IA;
  JA = SM[0].JA;
  AA = SM[0].AA;


  for (i = 0; i < SM[0].nz; i++){

     Pr = (IA[i] / BLACS_PARAMS[9] ) % BLACS_PARAMS[3] ;
     if (Pr == BLACS_PARAMS[5] ){

       Pc = (JA[i] / BLACS_PARAMS[10] ) % BLACS_PARAMS[4] ;
       if (Pc == BLACS_PARAMS[6] ){

           i_loc = IA[i] % BLACS_PARAMS[9] ;
           j_loc = JA[i] % BLACS_PARAMS[10];

           lr = IA[i]/(BLACS_PARAMS[3]*BLACS_PARAMS[9]);
           lc = JA[i]/(BLACS_PARAMS[4]*BLACS_PARAMS[10]);
           /* transpose form       */
           inva_LOC[lc*BLACS_PARAMS[10]+j_loc][lr*BLACS_PARAMS[9]+i_loc] += AA[i];
           /* not transposed form  */
           //inva_LOC[lr*BLACS_PARAMS[9]+i_loc][lc*BLACS_PARAMS[10]+j_loc] += AA[i];
        }
     }
  }

  if (storage_type == 'F')
    return;

  if (storage_type == 'L'){

    /* transpose  */
    JA = SM[0].IA;
    IA = SM[0].JA;

    for (i = 0; i < SM[0].nz; i++){

      if (JA[i] == IA[i] ) continue; /* diagonal elements are already stored */

      Pr = (IA[i] / BLACS_PARAMS[9] ) % BLACS_PARAMS[3] ;
      if (Pr == BLACS_PARAMS[5] ){

        Pc = (JA[i] / BLACS_PARAMS[10] ) % BLACS_PARAMS[4] ;
        if (Pc == BLACS_PARAMS[6] ){

           i_loc = IA[i] % BLACS_PARAMS[9] ;
           j_loc = JA[i] % BLACS_PARAMS[10];

           lr = IA[i]/(BLACS_PARAMS[3]*BLACS_PARAMS[9]);
           lc = JA[i]/(BLACS_PARAMS[4]*BLACS_PARAMS[10]);
           /* transpose form       */
           inva_LOC[lc*BLACS_PARAMS[10]+j_loc][lr*BLACS_PARAMS[9]+i_loc] += AA[i];
           /* not transposed form  */
           //inva_LOC[lr*BLACS_PARAMS[9]+i_loc][lc*BLACS_PARAMS[10]+j_loc] += AA[i];
        }
      }
    }
  return;
  }
}

void get_gather_map(int *BLACS_PARAMS, char dir, int *map){

/* functions returns (in map) correspondence between local
   and global indices  X[index_local] = X[map[index_local]] */


  if (dir == 'r') {

    int row, Pr, row_loc, lr;

    for (row = 0; row < BLACS_PARAMS[7] ; row++){
      Pr = (row / BLACS_PARAMS[9] ) % BLACS_PARAMS[3] ;
      if (Pr == BLACS_PARAMS[5] ){
         row_loc = row % BLACS_PARAMS[9] ;
         lr = row/(BLACS_PARAMS[3]*BLACS_PARAMS[9]);
         map[lr*BLACS_PARAMS[9]+row_loc] = row;
      }
    }
    return;
  }
  if (dir == 'c'){

    int col, Pc, col_loc, lc;

    for (col = 0; col < BLACS_PARAMS[8]; col++){
      Pc = (col / BLACS_PARAMS[10] ) % BLACS_PARAMS[4] ;
      if (Pc == BLACS_PARAMS[6] ){
         col_loc = col % BLACS_PARAMS[10];
         lc = col/(BLACS_PARAMS[4]*BLACS_PARAMS[10]);
         map[lc*BLACS_PARAMS[10]+col_loc] = col;
      }
    }
    return;
  }
  fprintf(stderr,"get_gather_map: ERROR, unknown direction - dir = %s \n",dir);
}

void scatter_topology_nektar(int *BLACS_PARAMS, double *V, double *work){

  /* proccessors with column=0  do MPI_Allgather      */
  /* then scatter results from proc with MYCOL=0
     to the rest of proccessors with the same MYROW   */

   int my_color,info;
   static int INIT_FLAG = 0;
   static MPI_Comm MPI_COMM_ZERO_COLUMN = MPI_COMM_NULL;
   //static MPI_Comm MPI_COMM_ROW = MPI_COMM_NULL;

   if (INIT_FLAG == 0){
      /* create communicators by splitting */

      if (BLACS_PARAMS[6] == 0)
        my_color = 1;
      else
        my_color = MPI_UNDEFINED;

      info = MPI_Comm_split(get_MPI_COMM(), my_color, BLACS_PARAMS[5], &MPI_COMM_ZERO_COLUMN);
      if (info != MPI_SUCCESS) {
        fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
        exit(1);
     }
     INIT_FLAG = 1;
   }

  memset(work,'\0',BLACS_PARAMS[7]);

  if (BLACS_PARAMS[6] == 0)
    MPI_Allreduce (V, work ,BLACS_PARAMS[7], MPI_DOUBLE, MPI_SUM, MPI_COMM_ZERO_COLUMN);

  MPI_Bcast ( work, BLACS_PARAMS[7], MPI_DOUBLE, 0, MPI_COMM_ROW_NEW);

  memcpy(V,work,BLACS_PARAMS[7]*sizeof(double));
}


void gather_topology_nektar(int *BLACS_PARAMS, double *V, double *work){

  /* proccessors with column=0  do MPI_Allgather      */
  /* then scatter results from proc with MYCOL=0
     to the rest of proccessors with the same MYROW   */

   int my_color,info;
   static int INIT_FLAG = 0;
   static MPI_Comm MPI_COMM_ZERO_COLUMN = MPI_COMM_NULL;

   if (INIT_FLAG == 0){
      /* create communicators by splitting */

      if (BLACS_PARAMS[6] == 0)
        my_color = 1;
      else
        my_color = MPI_UNDEFINED;

      info = MPI_Comm_split(get_MPI_COMM(), my_color, BLACS_PARAMS[5], &MPI_COMM_ZERO_COLUMN);
      if (info != MPI_SUCCESS) {
        fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
        exit(1);
     }
     INIT_FLAG = 1;
   }

  MPI_Reduce(V, work, BLACS_PARAMS[7], MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROW_NEW);

  if (BLACS_PARAMS[6] == 0)
    MPI_Allreduce (work, V ,BLACS_PARAMS[7], MPI_DOUBLE, MPI_SUM, MPI_COMM_ZERO_COLUMN);

}


void matrix_2DcyclicToNormal(int *BLACS_PARAMS, double **A_2D, int *first_row_col){

/* reminder:  A_2D is stored in transposed form                      */
/* this function reorder lines and columns of array A_2D             */
/*  2D cyclic block destributed matrix -> block destributed matrix   */

  int *map;
  double *sendbuf,*recvbuf;
  int row,col,shift;
  int *shift_index;

  /*  STEP 1: reorder rows */
  map = new int[BLACS_PARAMS[11]];
  sendbuf = new double[BLACS_PARAMS[7]];
  recvbuf = new double[BLACS_PARAMS[7]];


  /* need to evaluate shift_row index */
  shift_index = new int[BLACS_PARAMS[3]];
  MPI_Allgather(&BLACS_PARAMS[11],1,MPI_INT, shift_index,1,MPI_INT,  MPI_COMM_COLUMN_NEW);
  shift = 0;
  for (row = 1; row <= BLACS_PARAMS[5]; row++)
    shift += shift_index[row-1];

  first_row_col[0] = shift;
  get_gather_map(BLACS_PARAMS,'r',map);

  for (col = 0; col < BLACS_PARAMS[12]; col++){
    memset(sendbuf,'\0',BLACS_PARAMS[7]*sizeof(double));

    /* copy values from A_2D row into work */
    for (row = 0; row < BLACS_PARAMS[11]; row++)
        sendbuf[map[row]] = A_2D[col][row];

    MPI_Allreduce (sendbuf, recvbuf,BLACS_PARAMS[7], MPI_DOUBLE, MPI_SUM, MPI_COMM_COLUMN_NEW);

    /* copy values back to A_2D */
    for (row = 0; row < BLACS_PARAMS[11]; row++)
       A_2D[col][row] = recvbuf[row+shift];
  }


  delete[] map;
  delete[] sendbuf;
  delete[] recvbuf;
  delete[] shift_index;

 /*  STEP 2: reorder columns */
  map = new int[BLACS_PARAMS[12]];
  sendbuf = new double[BLACS_PARAMS[8]];
  recvbuf = new double[BLACS_PARAMS[8]];

  /* need to evaluate shift index */
  shift_index = new int[BLACS_PARAMS[4]];
  MPI_Allgather(&BLACS_PARAMS[12],1,MPI_INT, shift_index,1,MPI_INT, MPI_COMM_ROW_NEW);

  shift = 0;
  for (col = 1; col <= BLACS_PARAMS[6]; col++)
    shift += shift_index[col-1];

  first_row_col[1] = shift;
  get_gather_map(BLACS_PARAMS,'c',map);

  for (row = 0; row < BLACS_PARAMS[11]; row++){

    memset(sendbuf,'\0',BLACS_PARAMS[8]*sizeof(double));

    /* copy values from A_2D columns into work */
    for (col = 0; col < BLACS_PARAMS[12]; col++){
        sendbuf[map[col]] = A_2D[col][row];
    }
    MPI_Allreduce (sendbuf, recvbuf,BLACS_PARAMS[8], MPI_DOUBLE, MPI_SUM, MPI_COMM_ROW_NEW);

    /* copy values back to A_2D */
    for (col = 0; col < BLACS_PARAMS[12]; col++)
       A_2D[col][row] = recvbuf[col+shift];
  }

  delete[] map;
  delete[] sendbuf;
  delete[] recvbuf;
  delete[] shift_index;
}

int parallel_dgemv(int *BLACS_PARAMS, double **M_local,double *V, double *ANS, int first_row, int first_col,
                   int *displs, int *rcvcnt){

  double *V_loc, *ANS_loc;

  V_loc   = &V[first_col];
  ANS_loc = &ANS[first_row];

  dgemv('N',BLACS_PARAMS[11],BLACS_PARAMS[12],1.0,*M_local,BLACS_PARAMS[11],V_loc,1,0.0,ANS_loc,1);

  //from this point V is not needed - use it as temporary array
  V_loc = &V[first_row]; //for temorary use
  MPI_Allreduce(ANS_loc,V_loc,BLACS_PARAMS[11],MPI_DOUBLE,MPI_SUM,MPI_COMM_ROW_NEW);


  return 0;
}



void parallel_dgemv_params(int *BLACS_PARAMS, int  *displs, int *rcvcnt){

/* the output is displacement and recvcounter vectors required by parallel_dgemv */

  int row;

  MPI_Allgather(&BLACS_PARAMS[11],1,MPI_INT,rcvcnt,1,MPI_INT,MPI_COMM_COLUMN_NEW);

  displs[0] = 0;
  for (row = 1; row < BLACS_PARAMS[3]; row++)
     displs[row] = displs[row-1]+rcvcnt[row-1];
}


void set_mapping_parallel_dgemv(Bsystem *Bsys){

  int *BLACS_PARAMS = Bsys->Pmat->info.lenergy.BLACS_PARAMS;
  int n_val_local = Bsys->pll->nv_lpsolve;
  int *solvemap_local = Bsys->pll->solvemap;

  int *n_val_local_per_cpu;
  int **solvemap_partner;
  int i,j,k;
  int my_range_min = Bsys->Pmat->info.lenergy.first_row;
  int my_range_max = Bsys->Pmat->info.lenergy.first_row + BLACS_PARAMS[11]-1;


  Bsys->Pmat->info.lenergy.GS_col_sendcntr = new int[BLACS_PARAMS[3]];
  Bsys->Pmat->info.lenergy.GS_col_recvcntr = new int[BLACS_PARAMS[3]];
  Bsys->Pmat->info.lenergy.GS_row_sendcntr = new int[BLACS_PARAMS[4]];
  Bsys->Pmat->info.lenergy.GS_row_recvcntr = new int[BLACS_PARAMS[4]];

  int *GS_col_sendcntr = Bsys->Pmat->info.lenergy.GS_col_sendcntr;
  int *GS_col_recvcntr = Bsys->Pmat->info.lenergy.GS_col_recvcntr;
  int *GS_row_sendcntr = Bsys->Pmat->info.lenergy.GS_row_sendcntr;
  int *GS_row_recvcntr = Bsys->Pmat->info.lenergy.GS_row_recvcntr;


  /* create row column communicator */

  MPI_Comm MPI_COMM_COLUMN = MPI_COMM_NULL;
  i = MPI_Comm_split(get_MPI_COMM(), BLACS_PARAMS[6],
                        BLACS_PARAMS[5],&MPI_COMM_COLUMN);
  if (i != MPI_SUCCESS) {
     fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
     exit(1);
  }


  /*  distribute length of solvemap_local to all procs */

  n_val_local_per_cpu = new int[BLACS_PARAMS[3]];

  MPI_Allgather ( &n_val_local,        1, MPI_INT,
                  n_val_local_per_cpu, 1, MPI_INT, MPI_COMM_COLUMN);


  solvemap_partner = new int*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
    solvemap_partner[i] = new int[n_val_local_per_cpu[i]];


  /*  exchange solvemap_local between procs in each column */
  memcpy(solvemap_partner[BLACS_PARAMS[5]],solvemap_local,n_val_local*sizeof(int));

  /* should MPI_Barrier be used here ? */
  for (i = 0; i < BLACS_PARAMS[3]; i++){
    MPI_Bcast (solvemap_partner[i], n_val_local_per_cpu[i], MPI_INT, i, MPI_COMM_COLUMN);
    MPI_Barrier(MPI_COMM_COLUMN);
  }

  /* count number of variables to be sent to each partner processor.
     and distributed locally as well */

  for (i = 0; i < BLACS_PARAMS[3]; i++){
     GS_col_sendcntr[i] = 0;
     for (j = 0; j < n_val_local_per_cpu[i]; j++){
    if ( (solvemap_partner[i][j] >= my_range_min) &&
    (solvemap_partner[i][j] <= my_range_max))
              GS_col_sendcntr[i]++;
     }
  }

  /* create a list of indices of variables that should be sent to partners */
  /* this lists has LOCAL indices of variables to be sent                  */
  /* global indeces can be computed from                                   */
  /* global_index = local_index+my_range_min                               */
  /* message to partner i is composed from elements
     array_name[index_list[i][k]], where k = 0,...,GS_col_sendcntr[i]     */


  Bsys->Pmat->info.lenergy.GS_col_index_list_send = new int*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
     Bsys->Pmat->info.lenergy.GS_col_index_list_send[i] = new int[GS_col_sendcntr[i]];

  int **GS_col_index_list_send = Bsys->Pmat->info.lenergy.GS_col_index_list_send;


  for (i = 0; i < BLACS_PARAMS[3]; i++){
      k = 0;
      for (j = 0; j < n_val_local_per_cpu[i]; j++){
         if ((solvemap_partner[i][j] >= my_range_min) && (solvemap_partner[i][j] <= my_range_max)){
      GS_col_index_list_send[i][k] = solvemap_partner[i][j]-my_range_min;
      k++;
   }
      }
  }

  /* at this point we know what data we should send to partner */
  /* we also need to know how to locally distribute data obtained from partner */
  int partner_range_min;
  int partner_range_max;


  /*  first we figure out what is the length of a message to be received from each partner */

  for (i = 0; i < BLACS_PARAMS[3]; i++){
    /* define partners range of indices */
    partner_range_min = Bsys->Pmat->info.lenergy.col_displs[i];
    partner_range_max = Bsys->Pmat->info.lenergy.col_displs[i]+Bsys->Pmat->info.lenergy.col_rcvcnt[i]-1;
    GS_col_recvcntr[i] = 0;
    for (j = 0; j < n_val_local; j++){
      if ((solvemap_local[j] >= partner_range_min) && (solvemap_local[j] <= partner_range_max)){
        GS_col_recvcntr[i]++;
      }
    }
  }


  Bsys->Pmat->info.lenergy.GS_col_index_list_recv = new int*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
     Bsys->Pmat->info.lenergy.GS_col_index_list_recv[i] = new int[GS_col_recvcntr[i]];

  int **GS_col_index_list_recv = Bsys->Pmat->info.lenergy.GS_col_index_list_recv;


  for (i = 0; i < BLACS_PARAMS[3]; i++){
    /* define partners range of indices */
    partner_range_min = Bsys->Pmat->info.lenergy.col_displs[i];
    partner_range_max = Bsys->Pmat->info.lenergy.col_displs[i]+Bsys->Pmat->info.lenergy.col_rcvcnt[i]-1;
    k = 0;
    for (j = 0; j < n_val_local; j++){
      if ((solvemap_local[j] >= partner_range_min) && (solvemap_local[j] <= partner_range_max)){
  GS_col_index_list_recv[i][k] = j;
        k++;
      }
    }
  }

   /* create a list of partners who will get more then 0bytes of date */
  k = 0;
  for (i = 0; i < BLACS_PARAMS[3]; i++)
       if ( (GS_col_sendcntr[i] > 0) && (i != BLACS_PARAMS[5]) ) k++;
  Bsys->Pmat->info.lenergy.GS_col_partner_list_send = new int[k];
  Bsys->Pmat->info.lenergy.Npartners_send = k;


  k = 0;
  for (i = 0; i < BLACS_PARAMS[3]; i++)
       if ( (GS_col_recvcntr[i] > 0) && (i != BLACS_PARAMS[5]) ) k++;
  Bsys->Pmat->info.lenergy.GS_col_partner_list_recv = new int[k];
  Bsys->Pmat->info.lenergy.Npartners_recv = k;

  /* determinate who are the partners */
  k = 0;
  for (i = 0; i < BLACS_PARAMS[3]; i++){
     if ((GS_col_sendcntr[i] > 0) && (i != BLACS_PARAMS[5]) ){
       Bsys->Pmat->info.lenergy.GS_col_partner_list_send[k] = i;
       k++;
     }
  }
  k = 0;
  for (i = 0; i < BLACS_PARAMS[3]; i++){
     if ((GS_col_recvcntr[i] > 0) && (i != BLACS_PARAMS[5]) ){
       Bsys->Pmat->info.lenergy.GS_col_partner_list_recv[k] = i;
       k++;
    }
  }

  Bsys->Pmat->info.lenergy.GS_col_sendbuf = new double*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
     Bsys->Pmat->info.lenergy.GS_col_sendbuf[i] = new double[GS_col_sendcntr[i]];

  Bsys->Pmat->info.lenergy.GS_col_recvbuf = new double*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
     Bsys->Pmat->info.lenergy.GS_col_recvbuf[i] = new double[GS_col_recvcntr[i]];


  /************** do the same for communication in row ************************/
  delete[] n_val_local_per_cpu;
  for (i = 0; i < BLACS_PARAMS[3]; i++)
    delete solvemap_partner[i];
  delete[] solvemap_partner;


  my_range_min = Bsys->Pmat->info.lenergy.first_col;
  my_range_max = Bsys->Pmat->info.lenergy.first_col + BLACS_PARAMS[12]-1;


  n_val_local_per_cpu = new int[BLACS_PARAMS[4]];

  /*  distribute length of solvemap_local to all procs */

  MPI_Allgather ( &n_val_local,        1, MPI_INT,
                  n_val_local_per_cpu, 1, MPI_INT, MPI_COMM_ROW_NEW);


  solvemap_partner = new int*[BLACS_PARAMS[4]];
  for (i = 0; i < BLACS_PARAMS[4]; i++)
    solvemap_partner[i] = new int[n_val_local_per_cpu[i]];

  /*  exchange solvemap_local between procs in each column */
  memcpy(solvemap_partner[BLACS_PARAMS[6]],solvemap_local,n_val_local*sizeof(int));

  for (i = 0; i < BLACS_PARAMS[4]; i++){
    MPI_Bcast (solvemap_partner[i], n_val_local_per_cpu[i], MPI_INT, i, MPI_COMM_ROW_NEW);
    MPI_Barrier(MPI_COMM_ROW_NEW);
  }


  /* localy check how many values this rank will receive from each partner including itself */
  for (i = 0; i < BLACS_PARAMS[4]; i++){
     GS_row_recvcntr[i] = 0;
     for (j = 0; j < n_val_local_per_cpu[i]; j++){
          if ( (solvemap_partner[i][j] >= my_range_min) &&
                (solvemap_partner[i][j] <= my_range_max))
              GS_row_recvcntr[i]++;
     }
     //fprintf(stderr," my rank = %d GS_row_recvcntr[%d] = %d \n",mynode(),i,GS_row_recvcntr[i]);
  }

  /* allocate memory for recvbuf  */
  Bsys->Pmat->info.lenergy.GS_row_index_list_recv = new int*[BLACS_PARAMS[4]];
  for (i = 0; i < BLACS_PARAMS[4]; ++i)
     Bsys->Pmat->info.lenergy.GS_row_index_list_recv[i] = new int[GS_row_recvcntr[i]];

  int **GS_row_index_list_recv = Bsys->Pmat->info.lenergy.GS_row_index_list_recv;

  /* save indices of values to be received from each partner */
  for (i = 0; i < BLACS_PARAMS[4]; i++){
     k = 0;
     for (j = 0; j < n_val_local_per_cpu[i]; j++){
          if ( (solvemap_partner[i][j] >= my_range_min) &&
                (solvemap_partner[i][j] <= my_range_max)){
            GS_row_index_list_recv[i][k] = solvemap_partner[i][j];
            k++;
          }
     }
  }

  /* get partners range */

  int *partner_range_min_max;
  partner_range_min_max = new int[2*BLACS_PARAMS[4]];
  MPI_Allgather ( &my_range_min, 1, MPI_INT, partner_range_min_max, 1, MPI_INT, MPI_COMM_ROW_NEW);
  MPI_Allgather ( &my_range_max, 1, MPI_INT, partner_range_min_max+BLACS_PARAMS[4], 1, MPI_INT, MPI_COMM_ROW_NEW);


  /* localy check how many values this rank will receive from each partner including itself */
  for (i = 0; i < BLACS_PARAMS[4]; i++){
     GS_row_sendcntr[i] = 0;
     for (j = 0; j < n_val_local; j++){
          if ( (solvemap_local[j] >= partner_range_min_max[i]) &&
                (solvemap_local[j] <= partner_range_min_max[i+BLACS_PARAMS[4]]))
              GS_row_sendcntr[i]++;
     }
     //fprintf(stderr," my rank = %d GS_row_sendcntr[%d] = %d \n",mynode(),i,GS_row_recvcntr[i]);
  }

  /* allocate memory for GS_row_index_list_send  */
  Bsys->Pmat->info.lenergy.GS_row_index_list_send = new int*[BLACS_PARAMS[4]];
  for (i = 0; i < BLACS_PARAMS[4]; i++)
     Bsys->Pmat->info.lenergy.GS_row_index_list_send[i] = new int[GS_row_sendcntr[i]];

  int **GS_row_index_list_send = Bsys->Pmat->info.lenergy.GS_row_index_list_send;

  /* localy create a list of values locations which will be send to partners */
  for (i = 0; i < BLACS_PARAMS[4]; i++){
     k = 0;
     for (j = 0; j < n_val_local; j++){
          if ( (solvemap_local[j] >= partner_range_min_max[i]) &&
                (solvemap_local[j] <= partner_range_min_max[i+BLACS_PARAMS[4]])){
              GS_row_index_list_send[i][k] = solvemap_local[j];
              k++;
          }
     }
  }

  /*  allocate memory for GS_row_sendbuf and GS_row_recvdbuf */
  Bsys->Pmat->info.lenergy.GS_row_sendbuf = new double*[BLACS_PARAMS[4]];
  for (i = 0; i < BLACS_PARAMS[4]; i++)
    Bsys->Pmat->info.lenergy.GS_row_sendbuf[i] = new double[GS_row_sendcntr[i]];

  Bsys->Pmat->info.lenergy.GS_row_recvbuf = new double*[BLACS_PARAMS[4]];
  for (i = 0; i < BLACS_PARAMS[4]; i++)
    Bsys->Pmat->info.lenergy.GS_row_recvbuf[i] = new double[GS_row_recvcntr[i]];

#if 0
  k = 0;
  for(i = 0; i <  BLACS_PARAMS[4];i++)
      k += GS_row_sendcntr[i];
  k = k-GS_row_sendcntr[BLACS_PARAMS[6]];
  Bsys->Pmat->info.lenergy.GS_row_sendbuf_new = new double[k];

  k = 0;
  for(i = 0; i < BLACS_PARAMS[4];i++)
      k += GS_row_recvcntr[i];
  k = k-GS_row_recvcntr[BLACS_PARAMS[6]];
  Bsys->Pmat->info.lenergy.GS_row_recvbuf_new = new double[k];

  GS_row_sendcntr[BLACS_PARAMS[6]] = GS_row_recvcntr[BLACS_PARAMS[6]] = 0;
#endif


#if 0
 /* for testing  save GS_col_index_list_send and GS_col_index_list_recv into file */

  static int FLAG = 0;
  FILE *pFile;
  char Fname[256];


  if (FLAG == 0)
    sprintf(Fname,"index_list_send_PRESSURE_%d.dat",mynode());
  else
    sprintf(Fname,"index_list_send_VELOCITY_%d.dat",mynode());

 pFile = fopen(Fname,"w");
  fprintf(pFile,"my_range_min = %d my_range_max = %d \n",my_range_min,my_range_max);
  for (i = 0; i < BLACS_PARAMS[4]; i++){
    fprintf(pFile,"will send %d doubles \n",GS_row_sendcntr[i]);
    for (j = 0; j < GS_row_sendcntr[i]; j++)
      fprintf(pFile,"%d ",GS_row_index_list_send[i][j]);
    fprintf(pFile," \n");
  }
  fclose(pFile);

  if (FLAG == 0)
    sprintf(Fname,"index_list_recv_PRESSURE_%d.dat",mynode());
  else
    sprintf(Fname,"index_list_recv_VELOCITY_%d.dat",mynode());

  pFile = fopen(Fname,"w");

  fprintf(pFile,"my_range_min = %d my_range_max = %d \n",my_range_min,my_range_max);
  for (i = 0; i < BLACS_PARAMS[4]; i++){
    fprintf(pFile,"will receive %d doubles \n ",GS_row_recvcntr[i]);
    for (j = 0; j < GS_row_recvcntr[i]; j++)
      fprintf(pFile,"%d ",GS_row_index_list_recv[i][j]);
    fprintf(pFile," \n");
  }
  fclose(pFile);


  if (FLAG == 0)
    sprintf(Fname,"index_list_part_PRESSURE_%d.dat",mynode());
  else
    sprintf(Fname,"index_list_part_VELOCITY_%d.dat",mynode());

  pFile = fopen(Fname,"w");

  fprintf(pFile,"my_range_min = %d my_range_max = %d \n",my_range_min,my_range_max);
  for (i = 0; i < BLACS_PARAMS[4]; i++){
    for (j = 0; j < n_val_local_per_cpu[i]; j++)
      fprintf(pFile,"%d ",solvemap_partner[i][j]);
    fprintf(pFile," \n");
  }
  fclose(pFile);

  FLAG = 1;

#endif

  delete[] n_val_local_per_cpu;
  for (i = 0; i < BLACS_PARAMS[4]; i++)
    delete solvemap_partner[i];
  delete[] solvemap_partner;
  delete[] partner_range_min_max;

}

void scatter_vector(Bsystem *Bsys, double *v_block, double *v_local){

  int *GS_col_sendcntr = Bsys->Pmat->info.lenergy.GS_col_sendcntr;
  int *GS_col_recvcntr = Bsys->Pmat->info.lenergy.GS_col_recvcntr;

  int **GS_col_index_list_send = Bsys->Pmat->info.lenergy.GS_col_index_list_send;
  int **GS_col_index_list_recv = Bsys->Pmat->info.lenergy.GS_col_index_list_recv;

  int *BLACS_PARAMS = Bsys->Pmat->info.lenergy.BLACS_PARAMS;

  double **sendbuf, **recvbuf;
  int i,j,info;

  MPI_Request **send_request, **recv_request;

  sendbuf = Bsys->Pmat->info.lenergy.GS_col_sendbuf;
  recvbuf = Bsys->Pmat->info.lenergy.GS_col_recvbuf;

  send_request = new MPI_Request*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
     send_request[i] = new MPI_Request[1];

  recv_request = new MPI_Request*[BLACS_PARAMS[3]];
  for (i = 0; i < BLACS_PARAMS[3]; i++)
     recv_request[i] = new MPI_Request[1];


  /*  recive data from partners */

   for (i = 0; i < BLACS_PARAMS[5]; i++){
     if (GS_col_recvcntr[i] == 0) continue;
     info = MPI_Irecv(recvbuf[i], GS_col_recvcntr[i], MPI_DOUBLE,
                       i, i, MPI_COMM_COLUMN_NEW, recv_request[i]);
      if (info != MPI_SUCCESS) fprintf (stderr, "rank %d: MPI_Irecv failed info = %d \n",info);
   }
   for (i = BLACS_PARAMS[5]+1; i < BLACS_PARAMS[3]; i++){
     if (GS_col_recvcntr[i] == 0) continue;
     info = MPI_Irecv(recvbuf[i], GS_col_recvcntr[i], MPI_DOUBLE,
                       i, i, MPI_COMM_COLUMN_NEW, recv_request[i]);
      if (info != MPI_SUCCESS) fprintf (stderr, "rank %d: MPI_Irecv failed info = %d \n",info);
   }


   /* send data to partners */
   for (i = 0; i < BLACS_PARAMS[5]; i++){
     if (GS_col_sendcntr[i] == 0) continue;
     for (j = 0; j <  GS_col_sendcntr[i]; j++)
        sendbuf[i][j] = v_block[GS_col_index_list_send[i][j]];
     MPI_Isend( sendbuf[i], GS_col_sendcntr[i] , MPI_DOUBLE,
                i, BLACS_PARAMS[5],  MPI_COMM_COLUMN_NEW, send_request[i]);
   }
   for (i = BLACS_PARAMS[5]+1; i < BLACS_PARAMS[3]; i++){
     if (GS_col_sendcntr[i] == 0) continue;
     for (j = 0; j <  GS_col_sendcntr[i]; j++)
        sendbuf[i][j] = v_block[GS_col_index_list_send[i][j]];
     MPI_Isend( sendbuf[i], GS_col_sendcntr[i] , MPI_DOUBLE,
                i, BLACS_PARAMS[5],  MPI_COMM_COLUMN_NEW, send_request[i]);
   }


   /* take care of  data located locally (let the communication to proceed) */
   i = BLACS_PARAMS[5];
   memset(v_local,'\0', Bsys->pll->nv_lpsolve*sizeof(double));
   for (j = 0; j < GS_col_sendcntr[i]; j++)
      v_local[GS_col_index_list_recv[i][j]] += v_block[GS_col_index_list_send[i][j]];


   /* test if MPI_Irecv has completed, if yes copy data into v_local */
    for (i = 0; i < BLACS_PARAMS[5]; i++){
      if (GS_col_recvcntr[i] == 0) continue;
       MPI_Wait(recv_request[i],MPI_STATUS_IGNORE);
       for (j = 0; j < GS_col_recvcntr[i]; j++)
           v_local[GS_col_index_list_recv[i][j]] += recvbuf[i][j];
    }
    for (i = BLACS_PARAMS[5]+1; i < BLACS_PARAMS[3]; i++){
      if (GS_col_recvcntr[i] == 0) continue;
       MPI_Wait(recv_request[i],MPI_STATUS_IGNORE);
       for (j = 0; j < GS_col_recvcntr[i]; j++)
           v_local[GS_col_index_list_recv[i][j]] += recvbuf[i][j];
    }

    /* free untested  but initialized MPI_Request */

    for (i = 0; i < BLACS_PARAMS[5]; ++i){
      if (GS_col_sendcntr[i] == 0) continue;
      MPI_Wait(send_request[i],MPI_STATUS_IGNORE);
    }
    for (i = BLACS_PARAMS[5]+1; i < BLACS_PARAMS[3]; ++i){
     if (GS_col_sendcntr[i] == 0) continue;
     MPI_Wait(send_request[i],MPI_STATUS_IGNORE);
    }

    for (i = 0; i < BLACS_PARAMS[3]; i++){
       delete send_request[i];
       delete recv_request[i];
    }
    delete[] send_request;
    delete[] recv_request;
  }

void gather_vector(Bsystem *Bsys, double *v_global, double *work){

#if 0
  int *GS_row_sendcntr = Bsys->Pmat->info.lenergy.GS_row_sendcntr;
  int *GS_row_recvcntr = Bsys->Pmat->info.lenergy.GS_row_recvcntr;

  int **GS_row_index_list_send = Bsys->Pmat->info.lenergy.GS_row_index_list_send;
  int **GS_row_index_list_recv = Bsys->Pmat->info.lenergy.GS_row_index_list_recv;

  int *BLACS_PARAMS = Bsys->Pmat->info.lenergy.BLACS_PARAMS;

  double *sendbuf, *recvbuf;
  int i,j,info,n;
  int nrecv = 0,nsend = 0;;

  double *dptr;
  int *iptr,*sdispl,*rdispl;

  sdispl = new int[BLACS_PARAMS[4]+1];
  rdispl = new int[BLACS_PARAMS[4]+1];

  sendbuf = Bsys->Pmat->info.lenergy.GS_row_sendbuf_new;
  recvbuf = Bsys->Pmat->info.lenergy.GS_row_recvbuf_new;

  dptr = sendbuf;
  sdispl[0] = 0;
  rdispl[0] = 0;
  for(i=0;i <  BLACS_PARAMS[4];i++) {
    sdispl[i+1] = sdispl[i] + GS_row_sendcntr[i];
    rdispl[i+1] = rdispl[i] + GS_row_recvcntr[i];
    if(i != BLACS_PARAMS[6]) {
     iptr = GS_row_index_list_send[i];
     for (j = 0; j <  GS_row_sendcntr[i]; j++)
       *dptr++ = *(v_global + *iptr++);
    }
  }
  MPI_Alltoallv(sendbuf,GS_row_sendcntr,sdispl,MPI_DOUBLE,recvbuf,GS_row_recvcntr,rdispl,MPI_DOUBLE,MPI_COMM_ROW_NEW);

  dptr = recvbuf;
  for(i=0;i <  BLACS_PARAMS[4];i++)
    if(i != BLACS_PARAMS[6]) {
     iptr = GS_row_index_list_recv[i];
     for (j = 0; j < GS_row_recvcntr[i]; j++)
       *(v_global + *iptr++) += *dptr++;
    }

   i = Bsys->Pmat->info.lenergy.first_col;
   MPI_Allreduce (v_global+i, work+i, BLACS_PARAMS[12],
                  MPI_DOUBLE, MPI_SUM, MPI_COMM_COLUMN_NEW);

   memcpy(v_global+i,work+i,BLACS_PARAMS[12]*sizeof(double));
   delete [] sdispl;
   delete [] rdispl;

#else

/*
  MPI_Allreduce(v_global,work,Bsys->Pmat->info.lenergy.BLACS_PARAMS[8],MPI_DOUBLE,MPI_SUM,MPI_COMM_ROW_NEW);
  MPI_Allreduce(work+Bsys->Pmat->info.lenergy.first_col,v_global+Bsys->Pmat->info.lenergy.first_col,Bsys->Pmat->info.lenergy.BLACS_PARAMS[12],MPI_DOUBLE,MPI_SUM,MPI_COMM_COLUMN_NEW);

  return;
*/


  int *GS_row_sendcntr = Bsys->Pmat->info.lenergy.GS_row_sendcntr;
  int *GS_row_recvcntr = Bsys->Pmat->info.lenergy.GS_row_recvcntr;

  int **GS_row_index_list_send = Bsys->Pmat->info.lenergy.GS_row_index_list_send;
  int **GS_row_index_list_recv = Bsys->Pmat->info.lenergy.GS_row_index_list_recv;

  int *BLACS_PARAMS = Bsys->Pmat->info.lenergy.BLACS_PARAMS;

  double **sendbuf, **recvbuf;
  int i,j,info;
  int nrecv = 0,nsend = 0;;

  double *dptr;
  int *iptr;


  MPI_Request *send_request, *recv_request;

  sendbuf = Bsys->Pmat->info.lenergy.GS_row_sendbuf;
  recvbuf = Bsys->Pmat->info.lenergy.GS_row_recvbuf;

  send_request = new MPI_Request[BLACS_PARAMS[4]];
  recv_request = new MPI_Request[BLACS_PARAMS[4]];


   /*  recive data from partners */
   for (i = 0; i < BLACS_PARAMS[6]; i++){

     if (GS_row_recvcntr[i] == 0){
       recv_request[i] = MPI_REQUEST_NULL;
       continue;
     }
     nrecv++;
     info = MPI_Irecv(recvbuf[i], GS_row_recvcntr[i], MPI_DOUBLE,
                       i, i, MPI_COMM_ROW_NEW, &recv_request[i]);
      if (info != MPI_SUCCESS) fprintf (stderr, "rank %d: MPI_Irecv failed info = %d \n",info);
   }

   recv_request[BLACS_PARAMS[6]] = MPI_REQUEST_NULL;

   for (i = BLACS_PARAMS[6]+1; i < BLACS_PARAMS[4]; i++){
     if (GS_row_recvcntr[i] == 0){
       recv_request[i] = MPI_REQUEST_NULL;
       continue;
     }
     nrecv++;
     info = MPI_Irecv(recvbuf[i], GS_row_recvcntr[i], MPI_DOUBLE,
                       i, i, MPI_COMM_ROW_NEW, &recv_request[i]);
      if (info != MPI_SUCCESS) fprintf (stderr, "rank %d: MPI_Irecv failed info = %d \n",info);
   }


   /* send data to partners */
   for (i = 0; i < BLACS_PARAMS[6]; i++){
     if (GS_row_sendcntr[i] == 0) {
       send_request[i] = MPI_REQUEST_NULL;
       continue;
     }
     nsend++;
     dptr = sendbuf[i];
     iptr = GS_row_index_list_send[i];
     for (j = 0; j <  GS_row_sendcntr[i]; j++)
       *dptr++ = *(v_global + *iptr++);

     MPI_Isend( sendbuf[i], GS_row_sendcntr[i] , MPI_DOUBLE,
                i, BLACS_PARAMS[6],  MPI_COMM_ROW_NEW, &send_request[i]);
   }

   send_request[BLACS_PARAMS[6]] = MPI_REQUEST_NULL;

   for (i = BLACS_PARAMS[6]+1; i < BLACS_PARAMS[4]; i++){
     if (GS_row_sendcntr[i] == 0){
       send_request[i] = MPI_REQUEST_NULL;
       continue;
     }
     nsend++;
     dptr = sendbuf[i];
     iptr = GS_row_index_list_send[i];
     for (j = 0; j <  GS_row_sendcntr[i]; j++)
       *dptr++ = *(v_global + *iptr++);

     MPI_Isend( sendbuf[i], GS_row_sendcntr[i] , MPI_DOUBLE,
                i, BLACS_PARAMS[6],  MPI_COMM_ROW_NEW, &send_request[i]);
   }

#if 0
   /* take care of the data locally */
   info = BLACS_PARAMS[6];
   iptr = GS_row_index_list_recv[info];
   for (j = 0; j < GS_row_recvcntr[info]; j++){
     *(work+*iptr) = *(v_global+*iptr);
     iptr++;
   }


  for (i = 0; i < nrecv; i++){
     MPI_Waitany(BLACS_PARAMS[4], recv_request, &info, MPI_STATUS_IGNORE);
     dptr = recvbuf[info];
     iptr = GS_row_index_list_recv[info];
     for (j = 0; j < GS_row_recvcntr[info]; j++)
       *(work + *iptr++) += *dptr++;
   }

   i = Bsys->Pmat->info.lenergy.first_col;
   MPI_Allreduce (work+i, v_global+i, BLACS_PARAMS[12],
                  MPI_DOUBLE, MPI_SUM, MPI_COMM_COLUMN_NEW);
#else

   for (i = 0; i < nrecv; i++){
     MPI_Waitany(BLACS_PARAMS[4], recv_request, &info, MPI_STATUS_IGNORE);
     dptr = recvbuf[info];
     iptr = GS_row_index_list_recv[info];
     for (j = 0; j < GS_row_recvcntr[info]; j++)
       *(v_global + *iptr++) += *dptr++;
   }

   i = Bsys->Pmat->info.lenergy.first_col;
   MPI_Allreduce (v_global+i, work+i, BLACS_PARAMS[12],
                  MPI_DOUBLE, MPI_SUM, MPI_COMM_COLUMN_NEW);

   memcpy(v_global+i,work+i,BLACS_PARAMS[12]*sizeof(double));
#endif


  /* free untested  but initialized MPI_Request */

  for(i=0; i < nsend;i++)
    MPI_Waitany(BLACS_PARAMS[4],send_request,&j,MPI_STATUS_IGNORE);

    delete[] send_request;
    delete[] recv_request;
#endif

}
