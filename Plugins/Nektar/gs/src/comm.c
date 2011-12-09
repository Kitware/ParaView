/***********************************comm.c*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
11.21.97
***********************************comm.c*************************************/

/***********************************comm.c*************************************
File Description:
-----------------

***********************************comm.c*************************************/
#include <stdio.h>

#if   defined NXSRC
#ifndef DELTA
#include <nx.h>
#endif

#elif defined MPISRC
#include <mpi.h>

#endif


#include "const.h"
#include "types.h"
#include "error.h"
#include "ivec.h"
#include "bss_malloc.h"
#include "comm.h"


/* global program control variables - explicitly exported */
int my_id = 0;
int num_nodes = 1;
int floor_num_nodes;
int i_log2_num_nodes;

/* global program control variables */
static int p_init = 0;
static int modfl_num_nodes;
static int edge_not_pow_2;

static unsigned int edge_node[INT_LEN*32];

static void sgl_iadd(int    *vals, int level);
static void sgl_radd(double *vals, int level);
static void hmt_concat(REAL *vals, REAL *work, int size);





/***********************************comm.c*************************************
Function: c_init_ ()

Input :
Output:
Return:
Description: legacy ...
***********************************comm.c*************************************/
#if defined UPCASE
static
void
C_INIT (void)
#else
static
void
c_init_ (void)
#endif
{
  comm_init();
}

/***********************************comm.c*************************************
Function: c_init ()

Input :
Output:
Return:
Description: legacy ...
***********************************comm.c*************************************/
static
void
c_init (void)
{
  comm_init();
}



/***********************************comm.c*************************************
Function: comm_init_ ()

Input :
Output:
Return:
Description: legacy ...
***********************************comm.c*************************************/
#if defined UPCASE
void
COMM_INIT (void)
#else
void
comm_init_ (void)
#endif
{
  comm_init();
}



/***********************************comm.c*************************************
Function: giop()

Input :
Output:
Return:
Description:
***********************************comm.c*************************************/
void
comm_init (void)
{
  REAL tmp=0.0;
  int *iptr,i;


#ifdef DEBUG
  error_msg_warning("c_init() :: start\n");
#endif

  if (p_init++) return;

#ifdef NXSRC
  if (sizeof(int) != sizeof(long))
    {error_msg_fatal("Int/Long Incompatible!");}
#endif

  if (NULL!=0)
    {error_msg_fatal("NULL != 0!");}

  if (sizeof(int) != 4)
    {error_msg_warning("Int != 4 Bytes!");}

#ifdef r8
  iptr= (int *) &tmp;
  for(i=0;i<sizeof(REAL)/sizeof(int);i++)
  {
     if (iptr[i]!=0)
    {error_msg_fatal("type double doesn't conform to IEEE 754 std. for 64 bit!");}
  }
#else
  iptr= (int *) &tmp;
  if (iptr[0])
    {error_msg_fatal("type float doesn't conform to IEEE 754 std. for 32 bit!");}
#endif


#if  defined MPISRC
  MPI_Comm_size(MPI_COMM_WORLD,&num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

#elif defined NXSRC
  my_id = (int)mynode();
  num_nodes = (int)numnodes();

#else
  my_id = 0;
  num_nodes = 1;

#endif

  if (!num_nodes)
    {error_msg_fatal("Can't have no nodes!?!");}

  if (num_nodes> (INT_MAX >> 1))
  {error_msg_fatal("Can't have more then MAX_INT/2 nodes!!!");}

  ivec_zero((int *)edge_node,INT_LEN*32);

  floor_num_nodes = 1;
  i_log2_num_nodes = modfl_num_nodes = 0;
  while (floor_num_nodes <= num_nodes)
    {
      edge_node[i_log2_num_nodes] = my_id ^ floor_num_nodes;
      floor_num_nodes <<= 1;
      i_log2_num_nodes++;
    }

  i_log2_num_nodes--;
  floor_num_nodes >>= 1;
  modfl_num_nodes = (num_nodes - floor_num_nodes);

  if ((my_id > 0) && (my_id <= modfl_num_nodes))
    {edge_not_pow_2=((my_id|floor_num_nodes)-1);}
  else if (my_id >= floor_num_nodes)
    {edge_not_pow_2=((my_id^floor_num_nodes)+1);
    }
  else
    {edge_not_pow_2 = 0;}

#ifdef DEBUG
  error_msg_warning("c_init() done :: my_id=%d, num_nodes=%d",my_id,num_nodes);
#endif
}



/***********************************comm.c*************************************
Function: giop_ ()

Input :
Output:
Return:
Description: fan-in/out version (Fortran)
***********************************comm.c*************************************/
#if defined UPCASE
void
GIOP (int *vals, int *work, int *n, int *oprs)
#else
void
giop_ (int *vals, int *work, int *n, int *oprs)
#endif
{
  giop(vals, work, *n, oprs);
}



/***********************************comm.c*************************************
Function: giop()

Input :
Output:
Return:
Description: fan-in/out version
***********************************comm.c*************************************/
void
giop(int *vals, int *work, int n, int *oprs)
{
  register int mask, edge;
  int type, dest;
  vfp fp;
#if defined MPISRC
  MPI_Status  status;
#elif defined NXSRC
  int len;
#endif


#ifdef SAFE
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs)
    {error_msg_fatal("giop() :: vals=%d, work=%d, oprs=%d",vals,work,oprs);}

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2))
    {error_msg_fatal("giop() :: non_uniform and n=0,1?");}

  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}
#endif

  /* if there's nothing to do return */
  if ((num_nodes<2)||(!n))
    {
#ifdef DEBUG
      error_msg_warning("giop() :: n=%d num_nodes=%d",n,num_nodes);
#endif
      return;
    }

  /* a negative number if items to send ==> fatal */
  if (n<0)
    {error_msg_fatal("giop() :: n=%d<0?",n);}

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM)
    {oprs++;}

  /* major league hack */
  if ((fp = (vfp) ivec_fct_addr(type)) == NULL)
    {
      error_msg_warning("giop() :: hope you passed in a rbfp!\n");
      fp = (vfp) oprs;
    }

#if  defined NXSRC
  /* all msgs will be of the same length */
  len = n*INT_LEN;

  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {csend(MSGTAG0+my_id,(char *)vals,len,edge_not_pow_2,0);}
      else
  {crecv(MSGTAG0+edge_not_pow_2,(char *)work,len); (*fp)(vals,work,n,oprs);}
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {csend(MSGTAG2+my_id,(char *)vals,len,dest,0); break;}
    else
      {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals, work, n, oprs);}
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {csend(MSGTAG4+my_id,(char *)vals,len,dest,0);}
    else
      {crecv(MSGTAG4+dest,(char *)vals,len);}
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {crecv(MSGTAG5+edge_not_pow_2,(char *)vals,len);}
      else
  {csend(MSGTAG5+my_id,(char *)vals,len,edge_not_pow_2,0);}
    }

#elif defined MPISRC
  /* all msgs will be of the same length */
  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {MPI_Send(vals,n,MPI_INT,edge_not_pow_2,MSGTAG0+my_id,MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG0+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
    (*fp)(vals,work,n,oprs);
  }
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {MPI_Send(vals,n,MPI_INT,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(work,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG2+dest,
           MPI_COMM_WORLD, &status);
        (*fp)(vals, work, n, oprs);
      }
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {MPI_Send(vals,n,MPI_INT,dest,MSGTAG4+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(vals,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG4+dest,
           MPI_COMM_WORLD, &status);
      }
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {
    MPI_Recv(vals,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG5+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
  }
      else
  {MPI_Send(vals,n,MPI_INT,edge_not_pow_2,MSGTAG5+my_id,MPI_COMM_WORLD);}
    }
#else
  return;
#endif
}



/***********************************comm.c*************************************
Function: grop_ ()

Input :
Output:
Return:
Description: fan-in/out version (Fortran)
***********************************comm.c*************************************/
#if defined UPCASE
void
GROP (REAL *vals, REAL *work, int *n, int *oprs)
#else
void
grop_ (REAL *vals, REAL *work, int *n, int *oprs)
#endif
{
  grop(vals, work, *n, oprs);
}



/***********************************comm.c*************************************
Function: grop()

Input :
Output:
Return:
Description: fan-in/out version
***********************************comm.c*************************************/
void
grop(REAL *vals, REAL *work, int n, int *oprs)
{
  register int mask, edge;
  int type, dest;
  vfp fp;
#if defined MPISRC
  MPI_Status  status;
#elif defined NXSRC
  int len;
#endif


#ifdef SAFE
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs)
    {error_msg_fatal("grop() :: vals=%d, work=%d, oprs=%d",vals,work,oprs);}

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2))
    {error_msg_fatal("grop() :: non_uniform and n=0,1?");}

  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}
#endif

  /* if there's nothing to do return */
  if ((num_nodes<2)||(!n))
    {return;}

  /* a negative number of items to send ==> fatal */
  if (n<0)
    {error_msg_fatal("gdop() :: n=%d<0?",n);}

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM)
    {oprs++;}

  if ((fp = (vfp) rvec_fct_addr(type)) == NULL)
    {
      error_msg_warning("grop() :: hope you passed in a rbfp!\n");
      fp = (vfp) oprs;
    }

#if  defined NXSRC
  /* all msgs will be of the same length */
  len = n*REAL_LEN;

  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {csend(MSGTAG0+my_id,(char *)vals,len,edge_not_pow_2,0);}
      else
  {crecv(MSGTAG0+edge_not_pow_2,(char *)work,len); (*fp)(vals,work,n,oprs);}
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {csend(MSGTAG2+my_id,(char *)vals,len,dest,0); break;}
    else
      {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals, work, n, oprs);}
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {csend(MSGTAG4+my_id,(char *)vals,len,dest,0);}
    else
      {crecv(MSGTAG4+dest,(char *)vals,len);}
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {crecv(MSGTAG5+edge_not_pow_2,(char *)vals,len);}
      else
  {csend(MSGTAG5+my_id,(char *)vals,len,edge_not_pow_2,0);}
    }

#elif defined MPISRC
  /* all msgs will be of the same length */
  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {MPI_Send(vals,n,REAL_TYPE,edge_not_pow_2,MSGTAG0+my_id,
      MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG0+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
    (*fp)(vals,work,n,oprs);
  }
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {MPI_Send(vals,n,REAL_TYPE,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(work,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG2+dest,
           MPI_COMM_WORLD, &status);
        (*fp)(vals, work, n, oprs);
      }
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {MPI_Send(vals,n,REAL_TYPE,dest,MSGTAG4+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(vals,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG4+dest,
           MPI_COMM_WORLD, &status);
      }
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {
    MPI_Recv(vals,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG5+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
  }
      else
  {MPI_Send(vals,n,REAL_TYPE,edge_not_pow_2,MSGTAG5+my_id,
      MPI_COMM_WORLD);}
    }
#else
  return;
#endif
}



/***********************************comm.c*************************************
Function: grop_hc_ ()

Input :
Output:
Return:
Description: fan-in/out version (Fortran)
***********************************comm.c*************************************/
#if defined UPCASE
void
GROP_HC (REAL *vals, REAL *work, int *n, int *oprs, int *dim)
#else
void
grop_hc_ (REAL *vals, REAL *work, int *n, int *oprs, int *dim)
#endif
{
  grop_hc(vals, work, *n, oprs, *dim);
}



/***********************************comm.c*************************************
Function: grop()

Input :
Output:
Return:
Description: fan-in/out version

note good only for num_nodes=2^k!!!

***********************************comm.c*************************************/
void
grop_hc(REAL *vals, REAL *work, int n, int *oprs, int dim)
{
  register int mask, edge;
  int type, dest;
  vfp fp;
#if defined MPISRC
  MPI_Status  status;
#elif defined NXSRC
  int len;
#endif


#ifdef SAFE
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs)
    {error_msg_fatal("grop_hc() :: vals=%d, work=%d, oprs=%d",vals,work,oprs);}

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2))
    {error_msg_fatal("grop_hc() :: non_uniform and n=0,1?");}

  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}
#endif

  /* if there's nothing to do return */
  if ((num_nodes<2)||(!n)||(dim<=0))
    {return;}

  /* the error msg says it all!!! */
  if (modfl_num_nodes)
    {error_msg_fatal("grop_hc() :: num_nodes not a power of 2!?!");}

  /* a negative number of items to send ==> fatal */
  if (n<0)
    {error_msg_fatal("grop_hc() :: n=%d<0?",n);}

  /* can't do more dimensions then exist */
  dim = MIN(dim,i_log2_num_nodes);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM)
    {oprs++;}

  if ((fp = (vfp) rvec_fct_addr(type)) == NULL)
    {
      error_msg_warning("grop_hc() :: hope you passed in a rbfp!\n");
      fp = (vfp) oprs;
    }

#if  defined NXSRC
  /* all msgs will be of the same length */
  len = n*REAL_LEN;

  /* implement the mesh fan in/out exchange algorithm */
  for (mask=1,edge=0; edge<dim; edge++,mask<<=1)
    {
      dest = my_id^mask;
      if (my_id > dest)
  {csend(MSGTAG2+my_id,(char *)vals,len,dest,0); break;}
      else
  {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals, work, n, oprs);}
    }

  /* for (mask=1, edge=1; edge<dim; edge++,mask<<=1) {;} */
  if (edge==dim)
    {mask>>=1;}
  else
    {while (++edge<dim) {mask<<=1;}}

  for (edge=0; edge<dim; edge++,mask>>=1)
    {
      if (my_id%mask)
  {continue;}

      dest = my_id^mask;
      if (my_id < dest)
  {csend(MSGTAG4+my_id,(char *)vals,len,dest,0);}
      else
  {crecv(MSGTAG4+dest,(char *)vals,len);}
    }

#elif defined MPISRC
  for (mask=1,edge=0; edge<dim; edge++,mask<<=1)
    {
      dest = my_id^mask;
      if (my_id > dest)
  {MPI_Send(vals,n,REAL_TYPE,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG2+dest,MPI_COMM_WORLD,
       &status);
    (*fp)(vals, work, n, oprs);
  }
    }

  if (edge==dim)
    {mask>>=1;}
  else
    {while (++edge<dim) {mask<<=1;}}

  for (edge=0; edge<dim; edge++,mask>>=1)
    {
      if (my_id%mask)
  {continue;}

      dest = my_id^mask;
      if (my_id < dest)
  {MPI_Send(vals,n,REAL_TYPE,dest,MSGTAG4+my_id,MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(vals,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG4+dest,MPI_COMM_WORLD,
       &status);
  }
    }
#else
  return;
#endif
}



/***********************************comm.c*************************************
Function: grop_hc_ ()

Input :
Output:
Return:
Description: fan-in/out version (Fortran)
***********************************comm.c*************************************/
#if defined UPCASE
void
GROP_HC_VVL  (REAL *vals, REAL *work, int *len_vec, int *oprs, int *dim)
#else
void
grop_hc_vvl_ (REAL *vals, REAL *work, int *len_vec, int *oprs, int *dim)
#endif
{
  grop_hc_vvl(vals, work, len_vec, oprs, *dim);
}



/***********************************comm.c*************************************
Function: gop()

Input :
Output:
Return:
Description: fan-in/out version
***********************************comm.c*************************************/
void
gop(void *vals, void *work, int n, vbfp fp, DATA_TYPE dt, int comm_type)
{
  register int mask, edge;
  int type, dest;
#if defined MPISRC
  MPI_Status  status;
  MPI_Op op;
#elif defined NXSRC
  int len;
#endif


#ifdef SAFE
  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}

  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!fp)
    {error_msg_fatal("gop() :: v=%d, w=%d, f=%d",vals,work,fp);}
#endif

  /* if there's nothing to do return */
  if ((num_nodes<2)||(!n))
    {return;}

  /* a negative number of items to send ==> fatal */
  if (n<0)
    {error_msg_fatal("gop() :: n=%d<0?",n);}

#if  defined NXSRC
  switch (dt) {
  case REAL_TYPE:
    len = n*REAL_LEN;
    break;
  case INT_TYPE:
    len = n*INT_LEN;
    break;
  default:
    error_msg_fatal("gop() :: unrecognized datatype!!!\n");
  }

  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {csend(MSGTAG0+my_id,(char *)vals,len,edge_not_pow_2,0);}
      else
  {crecv(MSGTAG0+edge_not_pow_2,(char *)work,len); (*fp)(vals,work,n,dt);}
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {csend(MSGTAG2+my_id,(char *)vals,len,dest,0); break;}
    else
      {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals,work,n,dt);}
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {csend(MSGTAG4+my_id,(char *)vals,len,dest,0);}
    else
      {crecv(MSGTAG4+dest,(char *)vals,len);}
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {crecv(MSGTAG5+edge_not_pow_2,(char *)vals,len);}
      else
  {csend(MSGTAG5+my_id,(char *)vals,len,edge_not_pow_2,0);}
    }

#elif defined MPISRC

  if (comm_type==MPI)
    {
      MPI_Op_create(fp,TRUE,&op);
      MPI_Allreduce (vals, work, n, dt, op, MPI_COMM_WORLD);
      MPI_Op_free(&op);
      return;
    }

  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {MPI_Send(vals,n,dt,edge_not_pow_2,MSGTAG0+my_id,
      MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,dt,MPI_ANY_SOURCE,MSGTAG0+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
    (*fp)(vals,work,&n,&dt);
  }
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {MPI_Send(vals,n,dt,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(work,n,dt,MPI_ANY_SOURCE,MSGTAG2+dest,
           MPI_COMM_WORLD, &status);
        (*fp)(vals, work, &n, &dt);
      }
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {MPI_Send(vals,n,dt,dest,MSGTAG4+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(vals,n,dt,MPI_ANY_SOURCE,MSGTAG4+dest,
           MPI_COMM_WORLD, &status);
      }
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {
    MPI_Recv(vals,n,dt,MPI_ANY_SOURCE,MSGTAG5+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
  }
      else
  {MPI_Send(vals,n,dt,edge_not_pow_2,MSGTAG5+my_id,
      MPI_COMM_WORLD);}
    }
#else
  return;
#endif
}






/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:

ii+1 entries in seg :: 0 .. ii

******************************************************************************/
void
ssgl_radd(register REAL *vals, register REAL *work, register int level,
    register int *segs)
{
  register int edge, type, dest, source, mask;
  register int stage_n;


#ifdef DEBUG
  if (level > i_log2_num_nodes)
    {error_msg_fatal("sgl_add() :: level > log_2(P)!!!");}
#endif


#ifdef NXSRC
  /* all msgs are *NOT* the same length */
  /* implement the mesh fan in/out exchange algorithm */
  for (mask=0, edge=0; edge<level; edge++, mask++)
    {
      stage_n = (segs[level] - segs[edge]);
      if (stage_n && !(my_id & mask))
  {
    dest = edge_node[edge];
    type = MSGTAG3 + my_id + (num_nodes*edge);
    if (my_id>dest)
      {csend(type, vals+segs[edge],stage_n*REAL_LEN,dest,0);}
    else
      {
        type =  type - my_id + dest;
        crecv(type,work,stage_n*REAL_LEN);
        rvec_add(vals+segs[edge], work, stage_n);
/*            daxpy(vals+segs[edge], work, stage_n); */
      }
  }
      mask <<= 1;
    }
  mask>>=1;
  for (edge=0; edge<level; edge++)
    {
      stage_n = (segs[level] - segs[level-1-edge]);
      if (stage_n && !(my_id & mask))
  {
    dest = edge_node[level-edge-1];
    type = MSGTAG6 + my_id + (num_nodes*edge);
    if (my_id<dest)
      {csend(type,vals+segs[level-1-edge],stage_n*REAL_LEN,dest,0);}
    else
      {
        type =  type - my_id + dest;
        crecv(type,vals+segs[level-1-edge],stage_n*REAL_LEN);
      }
  }
      mask >>= 1;
    }
#endif
}



/***********************************comm.c*************************************
Function: grop_hc_vvl()

Input :
Output:
Return:
Description: fan-in/out version

note good only for num_nodes=2^k!!!

***********************************comm.c*************************************/
void
grop_hc_vvl(REAL *vals, REAL *work, int *segs, int *oprs, int dim)
{
  register int mask, edge, n;
  int type, dest;
  vfp fp;
#if defined MPISRC
  MPI_Status  status;
#elif defined NXSRC
  int len;
#endif


  error_msg_fatal("grop_hc_vvl() :: is not working!\n");

#ifdef SAFE
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs||!segs)
    {error_msg_fatal("grop_hc() :: vals=%d, work=%d, oprs=%d",vals,work,oprs);}

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2))
    {error_msg_fatal("grop_hc() :: non_uniform and n=0,1?");}

  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}
#endif

  /* if there's nothing to do return */
  if ((num_nodes<2)||(dim<=0))
    {return;}

  /* the error msg says it all!!! */
  if (modfl_num_nodes)
    {error_msg_fatal("grop_hc() :: num_nodes not a power of 2!?!");}

  /* can't do more dimensions then exist */
  dim = MIN(dim,i_log2_num_nodes);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM)
    {oprs++;}

  if ((fp = (vfp) rvec_fct_addr(type)) == NULL)
    {
      error_msg_warning("grop_hc() :: hope you passed in a rbfp!\n");
      fp = (vfp) oprs;
    }

#if  defined NXSRC
  /* all msgs are *NOT* the same length */
  /* implement the mesh fan in/out exchange algorithm */
  for (mask=1,edge=0; edge<dim; edge++,mask<<=1)
    {
      n = segs[dim]-segs[edge];
      /*
      error_msg_warning("n=%d, segs[%d]=%d, segs[%d]=%d\n",n,dim,segs[dim],
      edge,segs[edge]);
      */
      len = n*REAL_LEN;
      dest = my_id^mask;
      if (my_id > dest)
  {csend(MSGTAG2+my_id,(char *)vals+segs[edge],len,dest,0); break;}
      else
  {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals+segs[edge], work, n, oprs);}
    }

  if (edge==dim)
    {mask>>=1;}
  else
    {while (++edge<dim) {mask<<=1;}}

  for (edge=0; edge<dim; edge++,mask>>=1)
    {
      if (my_id%mask)
  {continue;}
      len = (segs[dim]-segs[dim-1-edge])*REAL_LEN;
      /*
      error_msg_warning("n=%d, segs[%d]=%d, segs[%d]=%d\n",n,dim,segs[dim],
                         dim-1-edge,segs[dim-1-edge]);
      */
      dest = my_id^mask;
      if (my_id < dest)
  {csend(MSGTAG4+my_id,(char *)vals+segs[dim-1-edge],len,dest,0);}
      else
  {crecv(MSGTAG4+dest,(char *)vals+segs[dim-1-edge],len);}
    }

#elif defined MPISRC
  for (mask=1,edge=0; edge<dim; edge++,mask<<=1)
    {
      n = segs[dim]-segs[edge];
      dest = my_id^mask;
      if (my_id > dest)
  {MPI_Send(vals+segs[edge],n,REAL_TYPE,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG2+dest,
       MPI_COMM_WORLD, &status);
    (*fp)(vals+segs[edge], work, n, oprs);
  }
    }

  if (edge==dim)
    {mask>>=1;}
  else
    {while (++edge<dim) {mask<<=1;}}

  for (edge=0; edge<dim; edge++,mask>>=1)
    {
      if (my_id%mask)
  {continue;}

      n = (segs[dim]-segs[dim-1-edge]);

      dest = my_id^mask;
      if (my_id < dest)
  {MPI_Send(vals+segs[dim-1-edge],n,REAL_TYPE,dest,MSGTAG4+my_id,
      MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(vals+segs[dim-1-edge],n,REAL_TYPE,MPI_ANY_SOURCE,
       MSGTAG4+dest,MPI_COMM_WORLD, &status);
  }
    }
#else
  return;
#endif
}


#ifdef INPROG

#if defined UPCASE
extern void PROC_SYNC  ();
#else
extern void proc_sync_ ();
#endif


/***********************************comm.c*************************************
Function: proc_sync_ ()

Input :
Output:
Return:
Description: fan-in/out version (Fortran)
***********************************comm.c*************************************/
#if defined UPCASE
void
PROC_SYNC  ()
#else
void
proc_sync_ ()
#endif
{
  proc_sync()
}



/***********************************comm.c*************************************
Function: proc_sync()

Input :
Output:
Return:
Description: hc bassed version
***********************************comm.c*************************************/
void
proc_sync()
{
  register int mask, edge;
  int type, dest;
#if defined MPISRC
  MPI_Status  status;
#endif


#ifdef DEBUG
  error_msg_warning("begin proc_sync()\n");
#endif

#ifdef SAFE
  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}
#endif

#if  defined NXSRC
  /* all msgs will be of zero length */

  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {csend(MSGTAG0+my_id,(char *)vals,len,edge_not_pow_2,0);}
      else
  {crecv(MSGTAG0+edge_not_pow_2,(char *)work,len); (*fp)(vals,work,n,oprs);}
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {csend(MSGTAG2+my_id,(char *)vals,len,dest,0); break;}
    else
      {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals, work, n, oprs);}
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {csend(MSGTAG4+my_id,(char *)vals,len,dest,0);}
    else
      {crecv(MSGTAG4+dest,(char *)vals,len);}
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {crecv(MSGTAG5+edge_not_pow_2,(char *)vals,len);}
      else
  {csend(MSGTAG5+my_id,(char *)vals,len,edge_not_pow_2,0);}
    }

#elif defined MPISRC
  /* all msgs will be of the same length */
  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {MPI_Send(vals,n,MPI_INT,edge_not_pow_2,MSGTAG0+my_id,MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG0+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
    (*fp)(vals,work,n,oprs);
  }
    }

  /* implement the mesh fan in/out exchange algorithm */
  if (my_id<floor_num_nodes)
    {
      for (mask=1,edge=0; edge<i_log2_num_nodes; edge++,mask<<=1)
  {
    dest = my_id^mask;
    if (my_id > dest)
      {MPI_Send(vals,n,MPI_INT,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(work,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG2+dest,
           MPI_COMM_WORLD, &status);
        (*fp)(vals, work, n, oprs);
      }
  }

      mask=floor_num_nodes>>1;
      for (edge=0; edge<i_log2_num_nodes; edge++,mask>>=1)
  {
    if (my_id%mask)
      {continue;}

    dest = my_id^mask;
    if (my_id < dest)
      {MPI_Send(vals,n,MPI_INT,dest,MSGTAG4+my_id,MPI_COMM_WORLD);}
    else
      {
        MPI_Recv(vals,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG4+dest,
           MPI_COMM_WORLD, &status);
      }
  }
    }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2)
    {
      if (my_id >= floor_num_nodes)
  {
    MPI_Recv(vals,n,MPI_INT,MPI_ANY_SOURCE,MSGTAG5+edge_not_pow_2,
       MPI_COMM_WORLD,&status);
  }
      else
  {MPI_Send(vals,n,MPI_INT,edge_not_pow_2,MSGTAG5+my_id,MPI_COMM_WORLD);}
    }
#else
  return;
#endif
}
#endif


/* hmt hack */
int
hmt_xor_ (register int *i1, register int *i2)
{
  return(*i1^*i2);
}


/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:

ii+1 entries in seg :: 0 .. ii

******************************************************************************/
void
staged_gs_ (register REAL *vals, register REAL *work, register int *level,
      register int *segs)
{
  ssgl_radd(vals, work, *level, segs);
}

/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:
******************************************************************************/
void
staged_iadd_ (int *gl_num, int *level)
{
  sgl_iadd(gl_num,*level);
}



/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:
******************************************************************************/
static void
sgl_iadd(int *vals, int level)
{
  int edge, type, dest, source, len, mask, ceil;
  int tmp, *work;


  /* all msgs will be of the same length */
  work = &tmp;
  len = INT_LEN;

  if (level > i_log2_num_nodes)
    {error_msg_fatal("sgl_add() :: level too big?");}

  if (level<=0)
    {return;}

#if defined NXSRC
  {
    long msg_id;
    /* implement the mesh fan in/out exchange algorithm */
    if (my_id<floor_num_nodes)
      {
  mask = 0;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[edge];
    type = MSGTAG1 + my_id + (num_nodes*edge);
    if (my_id > dest)
      {
        msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id);
      }
    else
      {
        type =  type - my_id + source;
        msg_id = irecv(type,work,len);
        msgwait(msg_id);
        vals[0] += work[0];
      }
        }
      mask <<= 1;
      mask += 1;
    }
      }

    if (my_id<floor_num_nodes)
      {
  mask >>= 1;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[level-edge-1];
    type = MSGTAG1 + my_id + (num_nodes*edge);
    if (my_id < dest)
      {
        msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id);
      }
    else
      {
        type =  type - my_id + source;
        msg_id = irecv(type,work,len);
        msgwait(msg_id);
        vals[0] = work[0];
      }
        }
      mask >>= 1;
    }
      }
  }
#elif defined MPISRC
  {
    MPI_Request msg_id;
    MPI_Status status;

    /* implement the mesh fan in/out exchange algorithm */
    if (my_id<floor_num_nodes)
      {
  mask = 0;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[edge];
    type = MSGTAG1 + my_id + (num_nodes*edge);
    if (my_id > dest)
      {
        MPI_Isend(vals,len,INT_TYPE,dest,type,MPI_COMM_WORLD,
            &msg_id);
        MPI_Wait(&msg_id,&status);
        /* msg_id = isend(type,vals,len,dest,0);
           msgwait(msg_id); */
      }
    else
      {
        type =  type - my_id + source;
        MPI_Irecv(work,len,INT_TYPE,MPI_ANY_SOURCE,
            type,MPI_COMM_WORLD,&msg_id);
        MPI_Wait(&msg_id,&status);
        /* msg_id = irecv(type,work,len);
           msgwait(msg_id); */
        vals[0] += work[0];
      }
        }
      mask <<= 1;
      mask += 1;
    }
      }

    if (my_id<floor_num_nodes)
      {
  mask >>= 1;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[level-edge-1];
    type = MSGTAG1 + my_id + (num_nodes*edge);
    if (my_id < dest)
      {
        MPI_Isend(vals,len,INT_TYPE,dest,type,MPI_COMM_WORLD,
            &msg_id);
        MPI_Wait(&msg_id,&status);
        /* msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id);*/
      }
    else
      {
        type =  type - my_id + source;
        MPI_Irecv(work,len,INT_TYPE,MPI_ANY_SOURCE,
            type,MPI_COMM_WORLD,&msg_id);
        MPI_Wait(&msg_id,&status);
        /* msg_id = irecv(type,work,len);
        msgwait(msg_id); */
        vals[0] = work[0];
      }
        }
      mask >>= 1;
    }
      }
  }
#else
  return;
#endif

}



/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:
******************************************************************************/
void
staged_radd_ (double *gl_num, int *level)
{
  sgl_radd(gl_num,*level);
}

/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:
******************************************************************************/
static void
sgl_radd(double *vals, int level)
{
  int edge, type, dest, source, len, mask, ceil;
  double tmp, *work;

#if defined NXSRC
  {
    long msg_id;

    /* all msgs will be of the same length */
    work = &tmp;
    len = REAL_LEN;

    if (level > i_log2_num_nodes)
      {error_msg_fatal("sgl_add() :: level too big?");}

    if (level<=0)
      {return;}

    /* implement the mesh fan in/out exchange algorithm */
    if (my_id<floor_num_nodes)
      {
  mask = 0;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[edge];
    type = MSGTAG3 + my_id + (num_nodes*edge);
    if (my_id > dest)
      {
        msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id);
      }
    else
      {
        type =  type - my_id + source;
        msg_id = irecv(type,work,len);
        msgwait(msg_id);
        vals[0] += work[0];
      }
        }
      mask <<= 1;
      mask += 1;
    }
      }

    if (my_id<floor_num_nodes)
      {
  mask >>= 1;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[level-edge-1];
    type = MSGTAG3 + my_id + (num_nodes*edge);
    if (my_id < dest)
      {
        msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id);
      }
    else
      {
        type =  type - my_id + source;
        msg_id = irecv(type,work,len);
        msgwait(msg_id);
        vals[0] = work[0];
      }
        }
      mask >>= 1;
    }
      }
  }
#elif defined MRISRC
  {
    MPI_Request msg_id;
    MPI_Status status;

    /* all msgs will be of the same length */
    work = &tmp;
    len = REAL_LEN;

    if (level > i_log2_num_nodes)
      {error_msg_fatal("sgl_add() :: level too big?");}

    if (level<=0)
      {return;}

    /* implement the mesh fan in/out exchange algorithm */
    if (my_id<floor_num_nodes)
      {
  mask = 0;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[edge];
    type = MSGTAG3 + my_id + (num_nodes*edge);
    if (my_id > dest)
      {
        MPI_Isend(vals,len,INT_TYPE,dest,type,MPI_COMM_WORLD,
            &msg_id);
        MPI_Wait(&msg_id,&status);
        /*msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id);*/
      }
    else
      {
        type =  type - my_id + source;
        MPI_Irecv(work,len,INT_TYPE,MPI_ANY_SOURCE,
            type,MPI_COMM_WORLD,&msg_id);
        MPI_Wait(&msg_id,&status);
        /*msg_id = irecv(type,work,len);
        msgwait(msg_id); */
        vals[0] += work[0];
      }
        }
      mask <<= 1;
      mask += 1;
    }
      }

    if (my_id<floor_num_nodes)
      {
  mask >>= 1;
  for (edge = 0; edge < level; edge++)
    {
      if (!(my_id & mask))
        {
    source = dest = edge_node[level-edge-1];
    type = MSGTAG3 + my_id + (num_nodes*edge);
    if (my_id < dest)
      {
        MPI_Isend(vals,len,INT_TYPE,dest,type,MPI_COMM_WORLD,
            &msg_id);
        MPI_Wait(&msg_id,&status);
        /* msg_id = isend(type,vals,len,dest,0);
        msgwait(msg_id); */
      }
    else
      {
        type =  type - my_id + source;
        MPI_Irecv(work,len,INT_TYPE,MPI_ANY_SOURCE,
            type,MPI_COMM_WORLD,&msg_id);
        MPI_Wait(&msg_id,&status);
        /*msg_id = irecv(type,work,len);
        msgwait(msg_id); */
        vals[0] = work[0];
      }
        }
      mask >>= 1;
    }
      }
  }
#else
  return;
#endif
}



/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:

ii+1 entries in seg :: 0 .. ii
******************************************************************************/
void
hmt_concat_ (REAL *vals, REAL *work, int *size)
{
  hmt_concat(vals, work, *size);
}



/******************************************************************************
Function: giop()

Input :
Output:
Return:
Description:

ii+1 entries in seg :: 0 .. ii

******************************************************************************/
static void
hmt_concat(REAL *vals, REAL *work, int size)
{
  int edge, type, dest, source, len, mask, ceil;
  int offset, stage_n;
  double *dptr;

  /* all msgs are *NOT* the same length */
  /* implement the mesh fan in/out exchange algorithm */
  mask = 0;
  stage_n = size;
  rvec_copy(work,vals,size);

#if defined NXSRC
  {
    long msg_id;

    dptr  = work+size;
    for (edge = 0; edge < i_log2_num_nodes; edge++)
      {
  len = stage_n * REAL_LEN;

  if (!(my_id & mask))
    {
      source = dest = edge_node[edge];
      type = MSGTAG3 + my_id + (num_nodes*edge);
      if (my_id > dest)
        {
    msg_id = isend(type, work, len,dest,0);
    msgwait(msg_id);
        }
      else
        {
    type =  type - my_id + source;
    msg_id = irecv(type, dptr,len);
    msgwait(msg_id);
        }
    }

#ifdef DEBUG_1
  ierror_msg_warning_n0("stage_n = ",stage_n);
#endif

  dptr += stage_n;
  stage_n <<=1;
  mask <<= 1;
  mask += 1;
      }

    size = stage_n;
    stage_n >>=1;
    dptr -= stage_n;

    mask >>= 1;

    for (edge = 0; edge < i_log2_num_nodes; edge++)
      {
  len = (size-stage_n) * REAL_LEN;

  if (!(my_id & mask) && stage_n)
    {
      source = dest = edge_node[i_log2_num_nodes-edge-1];
      type = MSGTAG6 + my_id + (num_nodes*edge);
      if (my_id < dest)
        {
    msg_id = isend(type,dptr,len,dest,0);
    msgwait(msg_id);
        }
      else
        {
    type =  type - my_id + source;
    msg_id = irecv(type,dptr,len);
    msgwait(msg_id);
        }
    }

#ifdef DEBUG_1
  ierror_msg_warning_n0("size-stage_n = ",size-stage_n);
#endif

  stage_n >>= 1;
  dptr -= stage_n;
  mask >>= 1;
      }
  }
#elif defined MRISRC
  {
    MPI_Request msg_id;
    MPI_Status status;

    dptr  = work+size;
    for (edge = 0; edge < i_log2_num_nodes; edge++)
      {
  len = stage_n * REAL_LEN;

  if (!(my_id & mask))
    {
      source = dest = edge_node[edge];
      type = MSGTAG3 + my_id + (num_nodes*edge);
      if (my_id > dest)
        {
    MPI_Isend(vals,len,INT_TYPE,dest,type,MPI_COMM_WORLD,
        &msg_id);
    MPI_Wait(&msg_id,&status);
    /*msg_id = isend(type, work, len,dest,0);
      msgwait(msg_id);*/
        }
      else
        {
    type =  type - my_id + source;
    MPI_Irecv(dptr,len,INT_TYPE,MPI_ANY_SOURCE,
        type,MPI_COMM_WORLD,&msg_id);
    MPI_Wait(&msg_id,&status);
    /* msg_id = irecv(type, dptr,len);
    msgwait(msg_id);*/
        }
    }

#ifdef DEBUG_1
  ierror_msg_warning_n0("stage_n = ",stage_n);
#endif

  dptr += stage_n;
  stage_n <<=1;
  mask <<= 1;
  mask += 1;
      }

    size = stage_n;
    stage_n >>=1;
    dptr -= stage_n;

    mask >>= 1;

    for (edge = 0; edge < i_log2_num_nodes; edge++)
      {
  len = (size-stage_n) * REAL_LEN;

  if (!(my_id & mask) && stage_n)
    {
      source = dest = edge_node[i_log2_num_nodes-edge-1];
      type = MSGTAG6 + my_id + (num_nodes*edge);
      if (my_id < dest)
        {
    MPI_Isend(vals,len,INT_TYPE,dest,type,MPI_COMM_WORLD,
        &msg_id);
    MPI_Wait(&msg_id,&status);
    /*msg_id = isend(type, work, len,dest,0);
    msg_id = isend(type,dptr,len,dest,0); */
    msgwait(msg_id);
        }
      else
        {
    type =  type - my_id + source;
    MPI_Irecv(dptr,len,INT_TYPE,MPI_ANY_SOURCE,
        type,MPI_COMM_WORLD,&msg_id);
    MPI_Wait(&msg_id,&status);
    /*msg_id = irecv(type,dptr,len);
    msgwait(msg_id);*/
        }
    }

#ifdef DEBUG_1
  ierror_msg_warning_n0("size-stage_n = ",size-stage_n);
#endif

  stage_n >>= 1;
  dptr -= stage_n;
  mask >>= 1;
      }
  }
#else
  return;
#endif
}
