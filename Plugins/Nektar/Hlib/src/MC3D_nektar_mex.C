/*  author: Leopold Grinberg
            Brown University
            Division of Applied Mathematics
            year 2010 
*/



#include "nektar_mex.h"
#include <math.h>

#ifdef TEST_OMP
#include <omp.h>
#endif

//#define MEX_REPORT

static void sort_ascending(int Npartners, int *dep, int *ref, int *pivot);
static int in_list(int n,int *list, int value);
static int compare (const void * a, const void * b);

#if (defined (__bg__) || defined (__blrts__) )
extern "C"{
void get_rank_coordinates(int *my_coord);
}

static void get_partners_coordinates(int Npartners, int *partner_list, int *my_coord, int *partners_coordinates, MPI_Comm comm);
static void reorder_partner_list_2(int Npartners,int *partners_coordinates, int *my_coord, int *partner_list, int *ref, int *pivot);
#endif

void NEKTAR_MEX::MEX_clean(){

  ;
/*
  int i;

  free(request_recv);
  free(request_send);
  free(partner_list);
  free(message_size);

  for (i = 0; i < Npartners; ++i){
     free(message_send_map[i]);
     free(message_recv_map[i]);
     free(send_buffer[i]);
     free(recv_buffer[i]); 
  }  
  free(message_send_map);
  free(message_recv_map);
  free(send_buffer);
  free(recv_buffer);
*/
}


void NEKTAR_MEX::MEX_init (int *map, int n, int *AdjacentPartitions, int NAdjacentPartitions, MPI_Comm comm_in){

/*  
n   - [INPUT] integer - length of array "map", number of degrees of freedom processed in this partition
map - [INPUT] - array of integers - global IDs of degrees of fredom processed in this partition
AdjacentPartitions - [INPUT] - array of integers -  list of possible adjacent partitions
NAdjacentPartitions - [INPUT] integer - number of possible adjacent partitions
comm - [INPUT] - communicator
*/

/* at the beginning we assume that NAdjacentPartitions can be greater or equall to the 
    actuall number of partition to communicate with */



  int *partner_map_size;
  int **partners_map;
  int *shared_dof;
  int i,j,k,ii,jj,partner;

  comm = comm_in;
  MPI_Comm_rank(comm,&my_rank);

#ifdef MEX_REPORT
  static int FLAG_INIT = 0;
#endif

  
  MPI_Request *request_recv_tmp, *request_send_tmp;
  //fprintf(stderr,"my_rank = %d, NAdjacentPartitions = %d\n",my_rank,NAdjacentPartitions);

  request_recv_tmp = new MPI_Request[NAdjacentPartitions];
  request_send_tmp = new MPI_Request[NAdjacentPartitions];

  partner_map_size  = new int[NAdjacentPartitions];
  shared_dof = new int[NAdjacentPartitions];

  
  for (i = 0; i < NAdjacentPartitions; ++i)
	  MPI_Irecv(&partner_map_size[i],1,MPI_INT,AdjacentPartitions[i],AdjacentPartitions[i],comm,&request_recv_tmp[i]);

  for (i = 0; i < NAdjacentPartitions; ++i)
	  MPI_Isend(&n,1,MPI_INT,AdjacentPartitions[i],my_rank,comm,&request_send_tmp[i]);


  MPI_Waitall(NAdjacentPartitions,request_recv_tmp,MPI_STATUS_IGNORE);
  MPI_Waitall(NAdjacentPartitions,request_send_tmp,MPI_STATUS_IGNORE);

 //allocate memory for incomming messages


  partners_map  = new int*[NAdjacentPartitions];
  for (i = 0; i < NAdjacentPartitions; i++)
      partners_map[i] = new int[partner_map_size[i]];


  //get partners map
  for (i = 0; i < NAdjacentPartitions; ++i)
	  MPI_Irecv(partners_map[i],partner_map_size[i],MPI_INT,AdjacentPartitions[i],AdjacentPartitions[i],comm,&request_recv_tmp[i]);

  //send local map to partners
  for (i = 0; i < NAdjacentPartitions; ++i)
	  MPI_Isend(map,n,MPI_INT,AdjacentPartitions[i],my_rank,comm,&request_send_tmp[i]);
  
  MPI_Waitall(NAdjacentPartitions,request_recv_tmp,MPI_STATUS_IGNORE);
  MPI_Waitall(NAdjacentPartitions,request_send_tmp,MPI_STATUS_IGNORE);


  // compare local map and partners map
  
  for (partner = 0; partner < NAdjacentPartitions; ++partner){
      shared_dof[partner] = 0;
      for (i = 0; i < n; ++i){
	  for (j = 0; j < partner_map_size[partner]; ++j){
	      if (map[i] == partners_map[partner][j]){
		  shared_dof[partner]++;
		  break;
	      }
	  }
      }
  }

  
  /* calculate the number of partitions to communicate with */
  for (partner = 0, Npartners = 0; partner < NAdjacentPartitions; ++partner){
    if (shared_dof[partner] > 0) 
      Npartners++;
  }

  delete[] request_recv_tmp;
  delete[] request_send_tmp;

  request_recv = (MPI_Request *) malloc(Npartners*sizeof(MPI_Request));
  request_send = (MPI_Request *) malloc(Npartners*sizeof(MPI_Request));

  for (i = 0; i < Npartners; ++i){
    request_recv[i] = MPI_REQUEST_NULL;
    request_send[i] = MPI_REQUEST_NULL;
  }

#if (defined (__bg__) || defined (__blrts__) )
  posix_memalign((void**)&partner_list,16, Npartners*sizeof(int));   
  posix_memalign((void**)&message_size,16, Npartners*sizeof(int));   
#else
  partner_list = (int *) malloc(Npartners*sizeof(int));
  message_size = (int *) malloc(Npartners*sizeof(int));
#endif

  for (partner = 0, i = 0; partner < NAdjacentPartitions; ++partner){
     if (shared_dof[partner] > 0){
         partner_list[i] = AdjacentPartitions[partner];
         message_size[i] = shared_dof[partner];
         i++;
     }
   }

   /* 
     the "partner_list" now can be sorted with respect to topology and message size
  */
  
  int *pivot, *message_size_tmp, *partner_map_tmp;

#if (defined (__bg__) || defined (__blrts__) )
  posix_memalign((void**)&my_coord,16, 4*sizeof(int));
  posix_memalign((void**)&partners_coordinates,16, Npartners*4*sizeof(int));
  get_rank_coordinates(my_coord); 
  get_partners_coordinates(Npartners,partner_list,my_coord,partners_coordinates,comm);
  posix_memalign((void**)&pivot,16, Npartners*sizeof(int));
  posix_memalign((void**)&message_size_tmp,16, Npartners*sizeof(int));
  posix_memalign((void**)&partner_map_tmp,16, Npartners*sizeof(int));
#else
  pivot            = (int *) malloc( Npartners*sizeof(int));
  message_size_tmp = (int *) malloc( Npartners*sizeof(int)); 
  partner_map_tmp  = (int *) malloc( Npartners*sizeof(int));
#endif  


   /* initialize pivot to default values */
   for (i = 0; i < Npartners;  ++i)
       pivot[i] = i;

#if (defined (__bg__) || defined (__blrts__) )
   reorder_partner_list_2(Npartners,partners_coordinates,my_coord,partner_list,message_size,pivot);
   free(partners_coordinates); 
#else
    sort_ascending(Npartners,partner_list,message_size,pivot);
#endif


   
   MPI_Barrier(comm);
   //if (my_rank == 0)
   //    fprintf(stderr,"MEX: sort_ascending - done\n");
      

   for (i = 0; i < Npartners;  ++i)
     message_size_tmp[pivot[i]] = message_size[i];

   memcpy(message_size,message_size_tmp,Npartners*sizeof(int));
  

   /* 
   map partner_list to AdjacentPartitions 
   partner_map_tmp  will store the index of partner_list[i] in AdjacentPartitions 
   */

   for (i = 0; i < Npartners;  ++i){
     for (j = 0; j < NAdjacentPartitions;  ++j){
        if (AdjacentPartitions[j] == partner_list[i]){
            partner_map_tmp[i] = j;
            break;
        }
     }
   }
 
   free(pivot);
   free(message_size_tmp);

   MPI_Barrier(comm);
   //if (my_rank == 0)
   //    fprintf(stderr,"MEX: partners reordering - done\n");

#if (defined (__bg__) || defined (__blrts__) )
   posix_memalign((void**)&message_send_map,16, Npartners*sizeof(int*));   
   for (i = 0; i < Npartners; ++i){
     posix_memalign((void**)&message_send_map[i],16, message_size[i]*sizeof(int));   
     memset(message_send_map[i],'\0',message_size[i]*sizeof(int));
   }

   posix_memalign((void**)&message_recv_map,16, Npartners*sizeof(int*));   
   for (i = 0; i < Npartners; ++i){
     posix_memalign((void**)&message_recv_map[i],16, message_size[i]*sizeof(int));   
     memset(message_recv_map[i],'\0',message_size[i]*sizeof(int));
   }

   posix_memalign((void**)&send_buffer,16, Npartners*sizeof(double*));
   for (i = 0; i < Npartners; ++i){
     posix_memalign((void**)&send_buffer[i],16, message_size[i]*sizeof(double));
     memset(send_buffer[i],'\0',message_size[i]*sizeof(double));
   }

   posix_memalign((void**)&recv_buffer,16, Npartners*sizeof(double*));
   for (i = 0; i < Npartners; ++i){
     posix_memalign((void**)&recv_buffer[i],16, message_size[i]*sizeof(double));
     memset(recv_buffer[i],'\0',message_size[i]*sizeof(double));
   }
#else
   
   message_send_map = (int **) malloc(Npartners*sizeof(int*));
   for (i = 0; i < Npartners; ++i){
     message_send_map[i] = (int *) malloc(message_size[i]*sizeof(int));
     memset(message_send_map[i],'\0',message_size[i]*sizeof(int));
   }

   message_recv_map = (int **) malloc(Npartners*sizeof(int*));
   for (i = 0; i < Npartners; ++i){
     message_recv_map[i]  = (int *) malloc(message_size[i]*sizeof(int)); 
     memset(message_recv_map[i],'\0',message_size[i]*sizeof(int));
   }

   send_buffer = (double **) malloc(Npartners*sizeof(double*));
   for (i = 0; i < Npartners; ++i){
     send_buffer[i] = (double *) malloc(message_size[i]*sizeof(double));
     memset(send_buffer[i],'\0',message_size[i]*sizeof(double));
   }
   recv_buffer = (double **) malloc(Npartners*sizeof(double*));
   for (i = 0; i < Npartners; ++i){
     recv_buffer[i] =  (double *) malloc(message_size[i]*sizeof(double));
     memset(recv_buffer[i],'\0',message_size[i]*sizeof(double));
   }
#endif

   /* to support unsorted list of degrees of freedom 
      two maps are created 
      it is possible to check if the two maps are identical 
      so only one will be kept and pointers 
      message_send_map[k]    and    message_recv_map[k]
      will be the same - this will help to save some memory
   */

   MPI_Barrier(comm);
   //if (my_rank == 0)
   //    fprintf(stderr,"MEX: file = %s, line = %d\n",__FILE__,__LINE__);

  double map_time_start = MPI_Wtime();

#ifdef TEST_OMP
#pragma omp parallel private(partner, i, ii, j, jj)
{  
   #pragma omp for schedule(dynamic)
   for (k = 0; k < Npartners; ++k){

     partner = partner_map_tmp[k];

     for (i = 0, ii=0; i < n; ++i){
           for (j = 0; j < partner_map_size[partner]; ++j){
             if (map[i] == partners_map[partner][j]){
                 message_send_map[k][ii] = i;
                 ii++;
                 break;
             }
           }
         }
        for (j = 0, jj = 0; j < partner_map_size[partner]; ++j){
          for (i = 0; i < n; ++i){
             if (map[i] == partners_map[partner][j]){
                message_recv_map[k][jj] = i;
                jj++;
                break;
             }
          }
        }
   }
}
#else
   for (k = 0; k < Npartners; ++k){ 
     partner = partner_map_tmp[k];
        for (i = 0, ii=0; i < n; ++i){
           for (j = 0; j < partner_map_size[partner]; ++j){
             if (map[i] == partners_map[partner][j]){
                 message_send_map[k][ii] = i;
                 ii++;
                 break;
             }
           }
        }
        for (j = 0, jj = 0; j < partner_map_size[partner]; ++j){
          for (i = 0; i < n; ++i){
             if (map[i] == partners_map[partner][j]){
                message_recv_map[k][jj] = i;
                jj++;
                break;
             }
          }
        }
   }
#endif 

   MPI_Barrier(comm);
   //if (my_rank == 0)
   //    fprintf(stderr,"MEX: file = %s, line = %d map_time = %f\n",__FILE__,__LINE__,MPI_Wtime() - map_time_start);
 
   free(partner_map_tmp);

/*

   for (partner = 0, k = 0; partner < NAdjacentPartitions; ++partner){
      
     if (shared_dof[partner] == 0) continue;

     for (i = 0, ii=0; i < n; ++i){
	   for (j = 0; j < partner_map_size[partner]; ++j){
 	     if (map[i] == partners_map[partner][j]){
		 message_send_map[k][ii] = i;
		 ii++;
		 break;
	     }
	   }
	 }

        for (j = 0, jj = 0; j < partner_map_size[partner]; ++j){
          for (i = 0; i < n; ++i){
             if (map[i] == partners_map[partner][j]){
                message_recv_map[k][jj] = i;
		jj++;
                break;
	     }
	  }
	}
        k++;
   }
*/

#ifdef MEX_REPORT
 //print report to file
  FILE *pFile;
  char fname[128];
  sprintf(fname,"report_gs_init.%d.%d",FLAG_INIT,my_rank);
  pFile = fopen(fname,"w");
  for (i = 0; i < NAdjacentPartitions; ++i){
	  if (shared_dof[i] > 0)
		  fprintf(pFile,"%d  %d\n",AdjacentPartitions[i],shared_dof[i]);
  }
  fprintf(pFile,"-1 -1 \n");
  
  fprintf(pFile,"**\n");
  for (partner = 0; partner < Npartners; ++partner){
	  fprintf(pFile,"will send to partner %d array of size %d\n",partner_list[partner],message_size[partner]);
	  for (i = 0; i < message_size[partner]; ++i)
           fprintf(pFile,"%d  ", message_send_map[partner][i]);
      fprintf(pFile,"\n");
  }
  fprintf(pFile,"**\n");
  for (partner = 0; partner < Npartners; ++partner){
	  fprintf(pFile,"will recv from partner %d array of size %d\n",partner_list[partner],message_size[partner]);
	  for (i = 0; i < message_size[partner]; ++i)
           fprintf(pFile,"%d  ", message_recv_map[partner][i]);
      fprintf(pFile,"\n");
  }
  fprintf(pFile,"**\n");
  fprintf(pFile,"My d.o.f are:\n");
  for (i = 0; i < n; ++i)
    fprintf(pFile,"%d  ",map[i]);
  fprintf(pFile,"\n");


  fclose(pFile);
  FLAG_INIT++;
#endif

  delete[] shared_dof;
  delete[] partner_map_size;
  for (i = 0; i < NAdjacentPartitions; i++)
	  delete partners_map[i];
  delete[] partners_map;
}


void NEKTAR_MEX::MEX_post_recv(){
  for (int i = 0; i < Npartners; ++i)
    MPI_Irecv(recv_buffer[i],message_size[i],MPI_DOUBLE,partner_list[i],partner_list[i]+2999,comm,&request_recv[i]);
}

void NEKTAR_MEX::MEX_post_send(){
  for (int i = 0; i < Npartners; ++i)
    MPI_Isend(send_buffer[i],message_size[i],MPI_DOUBLE,partner_list[i],my_rank+2999,comm,&request_send[i]);
}


void NEKTAR_MEX::MEX_plus(double *val){

  double *dp;
  int *map;
  int i,j,partner,index;

#ifdef MEX_TIMING
  double time_start,time_end,time_MAX, time_MIN;  
  time_start = MPI_Wtime();
#endif

  MEX_post_recv();

  for (partner = 0; partner < Npartners; ++partner){
    dp = send_buffer[partner];
    map = message_send_map[partner];
    for (i = 0; i < message_size[partner]; ++i)
       dp[i] = val[map[i]]; 
  }


  MEX_post_send();


  for (i = 0; i < Npartners; i++){
     MPI_Waitany(Npartners,request_recv,&index,MPI_STATUS_IGNORE);
     dp = recv_buffer[index];
     map = message_recv_map[index];
     for (j = 0; j < message_size[index]; ++j)
        val[map[j]] += dp[j];
  } 
  MPI_Waitall(Npartners,request_send,MPI_STATUS_IGNORE);
 
#ifdef MEX_TIMING  
  time_end = MPI_Wtime();
  MPI_Barrier(comm);
  time_end = time_end - time_start;
  MPI_Reduce ( &time_end, &time_MAX, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
  MPI_Reduce ( &time_end, &time_MIN, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
  if (my_rank == 0)
    fprintf(stdout,"MEX_plus: time_MAX = %e, time_MIN = %e\n",time_MAX, time_MIN);
#endif


}

void NEKTAR_MEX::MEX_max_fabs(double *val){

  double a,b;
  double *dp;
  int *map;
  int i, j,partner,index;

  MEX_post_recv();


  for (partner = 0; partner < Npartners; ++partner){
    dp = send_buffer[partner];
    map = message_send_map[partner];
    for (i = 0; i < message_size[partner]; ++i)
       dp[i] = val[map[i]];
  }

  MEX_post_send();


  for (partner = 0; partner < Npartners; partner++){
     MPI_Waitany(Npartners,request_recv,&index,MPI_STATUS_IGNORE);
     dp = recv_buffer[index];
     map = message_recv_map[index];
     for (i = 0; i < message_size[index]; ++i){
        j = map[i];
        a = fabs(val[j]);
        b = fabs(dp[i]);
        if (b>a)
           val[j] = dp[i];
     }
  }
  MPI_Waitall(Npartners,request_send,MPI_STATUS_IGNORE);
}

static void sort_ascending(int Npartners, int *dep, int *ref, int *pivot){

  int i,j,k;
  int *dep_tmp,*ref_tmp,*list;
  dep_tmp = (int *) malloc(Npartners*sizeof(int));
  ref_tmp = (int *) malloc(Npartners*sizeof(int));
  list = (int *) malloc(Npartners*sizeof(int));

  memcpy(dep_tmp,dep,Npartners*sizeof(int));
  memcpy(ref_tmp,ref,Npartners*sizeof(int));

  for (i = 0; i < Npartners; i++)
     list[i] = -1;

  qsort(ref,Npartners,sizeof(int),compare);

  k = 0;
  for (i = 0; i < Npartners; i++){
    for (j = 0; j < Npartners; j++){
      if (ref[i] == ref_tmp[j]){
        if (in_list(Npartners,list,j) != 1){
          dep[i] = dep_tmp[j];
          list[k] = j;
          k++;
          break;
        }
      }
    }
  }
  free(list);

  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

/*
  for (i = 0; i < Npartners; i++)
    fprintf(stderr,"my_rank = %d::: [%d  %d]  [%d  %d] \n",my_rank,dep_tmp[i],ref_tmp[i],dep[i],ref[i]);
*/

   /* set up pivot table */
   for (i = 0; i < Npartners; i++)
     for (j = 0; j < Npartners; j++){
       if (dep_tmp[i] == dep[j]){
         pivot[i] = j;
         break;
       }
     }

  memcpy(ref,ref_tmp,Npartners*sizeof(int));

  free(dep_tmp);
  free(ref_tmp);
}


static int in_list(int n,int *list, int value){

  int i;
  for (i = 0; i < n; ++i){
    if (value == list[i])
      return 1;
  }
  return 0;
}

static int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

#if (defined (__bg__) || defined (__blrts__) )

static void reorder_partner_list_2(int Npartners,int *partners_coordinates, int *my_coord, int *partner_list, int *ref, int *pivot){


   int i,j,k,Npartners_within_node,Xpartner,Ypartner,Zpartner,XYZpartner,my_rank;
   int *partner_list_tmp,*Xpartner_list,*Ypartner_list,*Zpartner_list,*XYZpartner_list;
   int *Xpartner_message_size,*Ypartner_message_size,*Zpartner_message_size,*XYZpartner_message_size;
   int *dep_tmp,*ref_tmp,*list;
   int ranks_in_my_node[3],ranks_in_my_node_message_size[3];

   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

   posix_memalign((void**)&partner_list_tmp,16, Npartners*sizeof(int));

   posix_memalign((void**)&Xpartner_list,16, Npartners*sizeof(int));
   posix_memalign((void**)&Ypartner_list,16, Npartners*sizeof(int));
   posix_memalign((void**)&Zpartner_list,16, Npartners*sizeof(int));
   posix_memalign((void**)&XYZpartner_list,16, Npartners*sizeof(int));
   posix_memalign((void**)&Xpartner_message_size,16, Npartners*sizeof(int));
   posix_memalign((void**)&Ypartner_message_size,16, Npartners*sizeof(int));
   posix_memalign((void**)&Zpartner_message_size,16, Npartners*sizeof(int));
   posix_memalign((void**)&XYZpartner_message_size,16, Npartners*sizeof(int));
   posix_memalign((void**)&dep_tmp,16, Npartners*sizeof(int));
   posix_memalign((void**)&ref_tmp,16, Npartners*sizeof(int));


//   __alignx(16,partner_list);
//   __alignx(16,partner_list_tmp);

   for (i = 0; i < Npartners; i++)
     partner_list_tmp[i] = partner_list[i];

   //search for partners with the sama Y and Z coordinate and different X coordinate
   for (Xpartner = 0, Ypartner =0, Zpartner = 0, XYZpartner = 0, Npartners_within_node = 0, i = 0; i < Npartners; i++){
     if ( (my_coord[0] != partners_coordinates[i*4]) && (my_coord[1] == partners_coordinates[i*4+1]) && (my_coord[2] == partners_coordinates[i*4+2])){
       Xpartner_list[Xpartner] = partner_list[i];
       Xpartner_message_size[Xpartner] = ref[i];
       Xpartner++;
       continue;
     }
     if ( (my_coord[0] == partners_coordinates[i*4]) && (my_coord[1] != partners_coordinates[i*4+1]) && (my_coord[2] == partners_coordinates[i*4+2])){
       Ypartner_list[Ypartner] = partner_list[i];
       Ypartner_message_size[Ypartner] = ref[i];
       Ypartner++;
       continue;
     }
     if ( (my_coord[0] == partners_coordinates[i*4]) && (my_coord[1] == partners_coordinates[i*4+1]) && (my_coord[2] != partners_coordinates[i*4+2])){
       Zpartner_list[Zpartner] = partner_list[i];
       Zpartner_message_size[Zpartner] = ref[i];
       Zpartner++;
       continue;
     }
     if ( (my_coord[0] == partners_coordinates[i*4]) && (my_coord[1] == partners_coordinates[i*4+1]) && (my_coord[2] == partners_coordinates[i*4+2])){
        ranks_in_my_node[Npartners_within_node] = partner_list[i];
        ranks_in_my_node_message_size[Npartners_within_node] = ref[i];
        Npartners_within_node++;
        continue;
     }
     XYZpartner_list[XYZpartner] = partner_list[i];
     XYZpartner_message_size[XYZpartner] = ref[i];
     XYZpartner++;
   }

   /*  sort partners in each category by the message size */

   list = (int *) malloc(Npartners*sizeof(int));

   memcpy(dep_tmp,ranks_in_my_node,Npartners_within_node*sizeof(int));
   memcpy(ref_tmp,ranks_in_my_node_message_size,Npartners_within_node*sizeof(int));

   for (i = 0; i < Npartners_within_node; i++)
     list[i] = -1;

   qsort(ranks_in_my_node_message_size,Npartners_within_node,sizeof(int),compare);

   k = 0;
   for (i = 0; i < Npartners_within_node; i++){
      for (j = 0; j < Npartners_within_node; j++){
        if (ranks_in_my_node_message_size[i] == ref_tmp[j]){
          if (in_list(Npartners_within_node,list,j) != 1){
            ranks_in_my_node[i] = dep_tmp[j];
            list[k] = j;
            k++;
            break;
          }
        }
      }
   }

   memcpy(dep_tmp,Xpartner_list,Xpartner*sizeof(int));
   memcpy(ref_tmp,Xpartner_message_size,Xpartner*sizeof(int));

   for (i = 0; i < Xpartner; i++)
     list[i] = -1;

   qsort(Xpartner_message_size,Xpartner,sizeof(int),compare);

   k = 0;
   for (i = 0; i < Xpartner; i++){
      for (j = 0; j < Xpartner; j++){
        if (Xpartner_message_size[i] == ref_tmp[j]){
          if (in_list(Xpartner,list,j) != 1){
            Xpartner_list[i] = dep_tmp[j];
            list[k] = j;
            k++;
            break;
          }
        }
      }
   }

   memcpy(dep_tmp,Ypartner_list,Ypartner*sizeof(int));
   memcpy(ref_tmp,Ypartner_message_size,Ypartner*sizeof(int));

   for (i = 0; i < Ypartner; i++)
     list[i] = -1;

   qsort(Ypartner_message_size,Ypartner,sizeof(int),compare);

   k = 0;
   for (i = 0; i < Ypartner; i++){
      for (j = 0; j < Ypartner; j++){
        if (Ypartner_message_size[i] == ref_tmp[j]){
          if (in_list(Ypartner,list,j) != 1){
            Ypartner_list[i] = dep_tmp[j];
            list[k] = j;
            k++;
            break;
          }
        }
      }
   }

   memcpy(dep_tmp,Zpartner_list,Zpartner*sizeof(int));
   memcpy(ref_tmp,Zpartner_message_size,Zpartner*sizeof(int));

   for (i = 0; i < Zpartner; i++)
     list[i] = -1;

   qsort(Zpartner_message_size,Zpartner,sizeof(int),compare);

   k = 0;
   for (i = 0; i < Zpartner; i++){
      for (j = 0; j < Zpartner; j++){
        if (Zpartner_message_size[i] == ref_tmp[j]){
          if (in_list(Zpartner,list,j) != 1){
            Zpartner_list[i] = dep_tmp[j];
            list[k] = j;
            k++;
            break;
          }
        }
      }
   }

   memcpy(dep_tmp,XYZpartner_list,XYZpartner*sizeof(int));
   memcpy(ref_tmp,XYZpartner_message_size,XYZpartner*sizeof(int));

   for (i = 0; i < XYZpartner; i++)
     list[i] = -1;

   qsort(XYZpartner_message_size,XYZpartner,sizeof(int),compare);

   k = 0;
   for (i = 0; i < XYZpartner; i++){
      for (j = 0; j < XYZpartner; j++){
        if (XYZpartner_message_size[i] == ref_tmp[j]){
          if (in_list(XYZpartner,list,j) != 1){
            XYZpartner_list[i] = dep_tmp[j];
            list[k] = j;
            k++;
            break;
          }
        }
      }
   }


  /* reorder partner_list
      put ranks_in_my_node first continue list with attempt
      sending a first message  X then to Y then to Z and
      finaly to arbitrary direction
      and start over with X direction */

  for (i = 0; i < Npartners_within_node; ++i)
     partner_list[i] = ranks_in_my_node[i];



   i = Npartners_within_node;
   int ix,iy,iz,ixyz;
   ix = iy = iz = ixyz = 0;

   while (i < Npartners){

     if (ix < Xpartner){
       partner_list[i] = Xpartner_list[ix];
       ix++;
       i++;
     }
     if (iy < Ypartner){
       partner_list[i] = Ypartner_list[iy];
       iy++;
       i++;
     }
     if (iz < Zpartner){
       partner_list[i] = Zpartner_list[iz];
       iz++;
       i++;
     }
     if (ixyz < XYZpartner){
       partner_list[i] = XYZpartner_list[ixyz];
       ixyz++;
       i++;
     }
   }


   /* set up pivot table - maping between the original sequence of partners to new sequence */
   for (i = 0; i < Npartners; i++)
     for (j = 0; j < Npartners; j++){
       if (partner_list_tmp[i] == partner_list[j]){
         pivot[i] = j;
         break;
       }
     }

#ifdef MEX_REPORT
   static int FLAG3 = 0;

   FILE *pFile;
   char fname[128];
   sprintf(fname,"partner_list2.%d.txt.%d",FLAG3,my_rank);
   FLAG3++;

   pFile = fopen(fname,"w");

   fprintf(pFile,"Rank = %d: [%d %d %d %d] Npartners_within_node = %d  Xpartner = %d Ypartner = %d Zpartner = %d, XYZpartner = %d \n",
                    my_rank,my_coord[0],my_coord[1],my_coord[2],my_coord[3],Npartners_within_node,Xpartner,Ypartner,Zpartner,XYZpartner);



   for (i = 0; i < Npartners; i++)
       ref_tmp[pivot[i]] = ref[i];




   fprintf(pFile,"\n old and new list and pivot: \n");
   for (i = 0; i < Npartners; i++)
      fprintf(pFile,"%3d [%d %d %d %d]  \t %3d  \t %3d, \t %3d \n",partner_list_tmp[i],
                                                                            partners_coordinates[i*4],
                                                                            partners_coordinates[i*4+1],
                                                                            partners_coordinates[i*4+2],
                                                                            partners_coordinates[i*4+3],
                                                                            partner_list[i],
                                                                            pivot[i],
                                                                            ref_tmp[i]);

   fclose(pFile);
#endif


   free(partner_list_tmp);
   free(Xpartner_list);
   free(Ypartner_list);
   free(Zpartner_list);
   free(XYZpartner_list);
   free(Xpartner_message_size);
   free(Ypartner_message_size);
   free(Zpartner_message_size);
   free(XYZpartner_message_size);
   free(dep_tmp);
   free(ref_tmp);



 
}


void get_partners_coordinates(int Npartners, int *partner_list, int *my_coord, int *partners_coordinates, MPI_Comm comm){


   int i,isend,irecv,my_rank;


   MPI_Request *request_recv,*request_send;
   request_recv = (MPI_Request *)malloc(Npartners*2*sizeof(MPI_Request));
   request_send = request_recv+Npartners;


   MPI_Comm_rank(comm, &my_rank);

   for (i = 0; i < Npartners; i++)
     MPI_Irecv( &partners_coordinates[4*i], 4, MPI_INT, partner_list[i],  my_rank, comm, &request_recv[i] );


   for (i = 0; i < Npartners; i++)
     MPI_Isend( my_coord, 4, MPI_INT, partner_list[i], partner_list[i], comm, &request_send[i]);

   MPI_Waitall(Npartners*2,request_recv,MPI_STATUSES_IGNORE);


#ifdef MEX_REPORT
   for (i = 0; i < Npartners; i++)
     fprintf(stdout,"My_rank = %d: partners_coordinates[%d] = %d %d %d %d\n",my_rank,i,partners_coordinates[4*i],
                                                                                       partners_coordinates[4*i+1],
                                                                                       partners_coordinates[4*i+2],
                                                                                       partners_coordinates[4*i+3]);
#endif

   free(request_recv);

}





#endif
