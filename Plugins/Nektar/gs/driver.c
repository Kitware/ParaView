#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#if   defined NXSRC
#ifndef DELTA
#include <nx.h>
#endif

#elif defined MPISRC
#include <mpi.h>

#endif

#include "const.h"
#include "types.h"
#include "comm.h"
#include "ivec.h"
#include "bss_malloc.h"
#include "gs.h"

extern int my_id, num_nodes;


REAL
binary_fct_add(REAL arg1, REAL arg2)
{
  return(arg1+arg2);
}


void
main(int argc, char **argv)
{
  int    *ivec, *work;
  double *dvec;
  int i, j;
  gs_ADT gsh;
  int  gsh_f;
  int  nnp, nn;
  char *op = "e";


#ifdef MPISRC
  MPI_Init(&argc,&argv);
#endif

  comm_init();

  if (!my_id) {printf("%d :: %d\n",my_id,num_nodes);}
  fflush(stdout);
#if   defined NXSRC
  gsync();
#elif defined MPISRC
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  ivec = (int *) bss_malloc(sizeof(int)*(num_nodes+2));
  work = (int *) bss_malloc(sizeof(int)*num_nodes);
  dvec = (double *) bss_malloc(sizeof(double)*(num_nodes+2));

  for (i=0;i<num_nodes;i++)
    {ivec[i]=1;}

  nn = GL_ADD;
  giop(ivec,work,num_nodes,&nn);

  for (i=0;i<num_nodes;i++)
    {
      if (ivec[i]!=num_nodes)
  {
    printf("\ngiop() not right!!!\n");
    exit(0);
  }
    }
  bss_free(work);

  for (i=0;i<num_nodes;i++)
    {ivec[i]=(i+1)*(my_id+1); dvec[i]=(my_id+1);}

  ivec[num_nodes+1] = ivec[num_nodes-1];
  dvec[num_nodes+1] = 0.1;
  ivec[num_nodes] = ivec[0];
  dvec[num_nodes] = 0.01;

#if   defined NXSRC
  gsync();
#elif defined MPISRC
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  nnp = num_nodes+2;
  nn  = num_nodes;

  gsh = gs_init(ivec,num_nodes+2,1);

  /*
  gsh = gs_init(ivec,num_nodes+2,num_nodes);

  gsh_f = gs_init_ (ivec,&nnp,&nn);
  */

  if (!my_id) {printf("done gs_init()\n");}
  fflush(stdout);

#if   defined NXSRC
  gsync();
#elif defined MPISRC
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  /*
  gs_print_template(gsh, 0);
  fflush(stdout);
  printf("%d :: %d :: gsh=%d\n",my_id,num_nodes,gsh);
  fflush(stdout);

#if   defined NXSRC
  gsync();
#elif defined MPISRC
  MPI_Barrier(MPI_COMM_WORLD);
#endif


  for (j=0;j<num_nodes;j++)
    {
      if (my_id==j)
  {
    for (i=0;i<num_nodes;i++)
      {printf("%d :: %d %f\n",my_id,ivec[i],dvec[i]);}
    fflush(stdout);
  }
#if   defined NXSRC
      gsync();
      gsync();
      gsync();
#elif defined MPISRC
      MPI_Barrier(MPI_COMM_WORLD);
#endif
    }
    */


  gs_gop(gsh,dvec,"+");

  /*
  gs_gop_binary(gsh,dvec,binary_fct_add);
  gs_gop(gsh,dvec,"+");


  gs_gop_ (&gsh_f,dvec,op);
  */

  if (!my_id) {printf("done gs_gop()\n");}
  fflush(stdout);

#if   defined NXSRC
  gsync();
#elif defined MPISRC
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  for (j=0;j<num_nodes;j++)
    /*  for (j=0;j<1;j++)  */
    {
      if (my_id==j)
  {
    for (i=0;i<num_nodes+2;i++)
      {printf("%d :: %d %f\n",my_id,ivec[i],dvec[i]);}
    fflush(stdout);
  }
    }

  bss_free(ivec);
  bss_free(dvec);

  /*
  gs_free_(&gsh_f);
  gs_free(gsh);
  */

  gs_free(gsh);



  perm_stats();
  bss_stats();

  fflush(stdout);

#if   defined NXSRC
  gsync();
#elif defined MPISRC
  MPI_Barrier(MPI_COMM_WORLD);
#endif

#if   defined MPISRC
  MPI_Finalize();
#endif
  exit(0);
}
