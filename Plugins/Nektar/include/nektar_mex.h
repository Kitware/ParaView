#ifndef NEKTAR_MEX_H
#define NEKTAR_MEX_H

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


class NEKTAR_MEX{

  private:
   
  int Npartners,my_rank;
  int *partner_list, 
      *message_size;
  int **message_send_map,
      **message_recv_map;
  double **send_buffer,
         **recv_buffer;

  MPI_Request *request_send, 
              *request_recv; 

  MPI_Comm  comm;

#if (defined (__bg__) || defined (__blrts__) )
  int *my_coord, *partners_coordinates;
#endif



  public:

    void MEX_init(int *map, int n, int *AdjacentPartitions, int NAdjacentPartitions, MPI_Comm comm);

    void MEX_clean();

    void MEX_post_recv();
    void MEX_post_send();

    void MEX_plus(double *val);
    void MEX_max_fabs(double *val);
};

#endif
