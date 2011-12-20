/***********************************gs.c***************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
************************************gs.c**************************************/

/***********************************gs.c***************************************
File Description:
-----------------

************************************gs.c**************************************/
#include <stdio.h>
#include <strings.h>
#include <math.h>
#include <float.h>
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
#include "bit_mask.h"
#include "error.h"
#include "queue.h"
#include "gs.h"

/* default length of number of items via tree - doubles if exceeded */
#define TREE_BUF_SZ 2048;
#define GS_VEC_SZ   1



/***********************************gs.c***************************************
Type: struct gather_scatter_id
------------------------------

************************************gs.c**************************************/
typedef struct gather_scatter_id {
  int id;
  int nel_min;
  int nel_max;
  int nel_sum;
  int negl;
  int gl_max;
  int gl_min;
  int repeats;
  int ordered;
  int positive;
  REAL *vals;

  /* bit mask info */
  int *my_proc_mask;
  int mask_sz;
  int *ngh_buf;
  int ngh_buf_sz;
  int *nghs;
  int num_nghs;
  int *pw_nghs;
  int num_pw_nghs;
  int *tree_nghs;
  int num_tree_nghs;

  int num_loads;

  /* repeats == true -> local info */
  int nel;         /* number of unique elememts */
  int *elms;       /* of size nel */
  int nel_total;
  int *local_elms; /* of size nel_total */
  int *companion;  /* of size nel_total */

  /* local info */
  int num_local_total;
  int local_strength;
  int num_local;
  int *num_local_reduce;
  int **local_reduce;
  int num_local_gop;
  int *num_gop_local_reduce;
  int **gop_local_reduce;

  /* pairwise info */
  int level;
  int num_pairs;
  int max_pairs;
  int *pair_list;
  int *msg_sizes;
  int **node_list;
  int len_pw_list;
  int *pw_elm_list;
  REAL *pw_vals;

#ifdef MPISRC
  MPI_Request *msg_ids_in;
  MPI_Request *msg_ids_out;

#else
  int *msg_ids_in;
  int *msg_ids_out;

#endif

  REAL *out;
  REAL *in;
  int msg_total;

  /* tree - crystal accumulator info */
  int max_left_over;
  int *pre;
  int *in_num;
  int *out_num;
  int **in_list;
  int **out_list;

  /* new tree work*/
  int  tree_nel;
  int *tree_elms;
  REAL *tree_buf;
  REAL *tree_work;

  int  tree_map_sz;
  int *tree_map_in;
  int *tree_map_out;

  /* current memory status */
  int gl_bss_min;
  int gl_perm_min;

} gs_id;

/* to be made public */
static int  gs_dump_ngh(gs_id *id, int loc_num, int *num, int *ngh_list);

/* PRIVATE - and definitely not exported */
static void gs_print_template(register gs_id* gs, int who);

static gs_id *gsi_check_args(int *elms, int nel, int level);
static void gsi_via_bit_mask(gs_id *gs);
static void gsi_via_int_list(gs_id *gs);
static void get_ngh_buf(gs_id *gs);
static void set_pairwise(gs_id *gs);
static gs_id * gsi_new(void);
static void set_tree(gs_id *gs);

/* same for all but vector flavor */
static void gs_gop_local_out(gs_id *gs, REAL *vals);
/* vector flavor */
static void gs_gop_vec_local_out(gs_id *gs, REAL *vals, int step);

static void gs_gop_vec_plus(gs_id *gs, REAL *in_vals, int step);
static void gs_gop_vec_pairwise_plus(gs_id *gs, REAL *in_vals, int step);
static void gs_gop_vec_local_plus(gs_id *gs, REAL *vals, int step);
static void gs_gop_vec_local_in_plus(gs_id *gs, REAL *vals, int step);
static void gs_gop_vec_tree_plus(gs_id *gs, REAL *vals, int step);


static void gs_gop_plus(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_plus(gs_id *gs, REAL *in_vals);
static void gs_gop_local_plus(gs_id *gs, REAL *vals);
static void gs_gop_local_in_plus(gs_id *gs, REAL *vals);
static void gs_gop_tree_plus(gs_id *gs, REAL *vals);

static void gs_gop_plus_hc(gs_id *gs, REAL *in_vals, int dim);
static void gs_gop_pairwise_plus_hc(gs_id *gs, REAL *in_vals, int dim);
static void gs_gop_tree_plus_hc(gs_id *gs, REAL *vals, int dim);

static void gs_gop_times(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_times(gs_id *gs, REAL *in_vals);
static void gs_gop_local_times(gs_id *gs, REAL *vals);
static void gs_gop_local_in_times(gs_id *gs, REAL *vals);
static void gs_gop_tree_times(gs_id *gs, REAL *vals);

static void gs_gop_min(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_min(gs_id *gs, REAL *in_vals);
static void gs_gop_local_min(gs_id *gs, REAL *vals);
static void gs_gop_local_in_min(gs_id *gs, REAL *vals);
static void gs_gop_tree_min(gs_id *gs, REAL *vals);

static void gs_gop_min_abs(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_min_abs(gs_id *gs, REAL *in_vals);
static void gs_gop_local_min_abs(gs_id *gs, REAL *vals);
static void gs_gop_local_in_min_abs(gs_id *gs, REAL *vals);
static void gs_gop_tree_min_abs(gs_id *gs, REAL *vals);

static void gs_gop_max(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_max(gs_id *gs, REAL *in_vals);
static void gs_gop_local_max(gs_id *gs, REAL *vals);
static void gs_gop_local_in_max(gs_id *gs, REAL *vals);
static void gs_gop_tree_max(gs_id *gs, REAL *vals);

static void gs_gop_max_abs(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_max_abs(gs_id *gs, REAL *in_vals);
static void gs_gop_local_max_abs(gs_id *gs, REAL *vals);
static void gs_gop_local_in_max_abs(gs_id *gs, REAL *vals);
static void gs_gop_tree_max_abs(gs_id *gs, REAL *vals);

static void gs_gop_exists(gs_id *gs, REAL *in_vals);
static void gs_gop_pairwise_exists(gs_id *gs, REAL *in_vals);
static void gs_gop_local_exists(gs_id *gs, REAL *vals);
static void gs_gop_local_in_exists(gs_id *gs, REAL *vals);
static void gs_gop_tree_exists(gs_id *gs, REAL *vals);

static void gs_gop_pairwise_binary(gs_id *gs, REAL *in_vals, rbfp fct);
static void gs_gop_local_binary(gs_id *gs, REAL *vals, rbfp fct);
static void gs_gop_local_in_binary(gs_id *gs, REAL *vals, rbfp fct);
static void gs_gop_tree_binary(gs_id *gs, REAL *vals, rbfp fct);

static int in_sub_tree(int *ptr3, int p_mask_size, int *buf2, int buf_size);


/* global vars */
/* from comm.c module */

/* module state inf and fortran interface */
static int num_gs_ids = 0;

/* should make this dynamic ... later */
static gs_id *gs_handles[MAX_GS_IDS];
static queue_ADT elms_q, mask_q;
static int msg_buf=MAX_MSG_BUF;
static int msg_ch=FALSE;

static int vec_sz=GS_VEC_SZ;
static int vec_ch=FALSE;

static int *tree_buf=NULL;
static int tree_buf_sz=0;
static int ntree=0;



/******************************************************************************
Function: ()

Input :
Output:
Return:
Description:
******************************************************************************/
#if defined UPCASE
void FLUSH_IO  (void)
#else
void flush_io_ (void)
#endif
{
#ifdef DELTA
  fflush(stdout);
#else
  fflush(stdout);
#endif
}



/******************************************************************************
Function: ()

Input :
Output:
Return:
Description:
******************************************************************************/
#if defined UPCASE
void GS_INIT_VEC_SZ(int *size)
#else
void gs_init_vec_sz_(int *size)
#endif
{
  gs_init_vec_sz(*size);
}



/******************************************************************************
Function: gs_init_()

Input :
Output:
Return:
Description:
******************************************************************************/
void gs_init_vec_sz(int size)
{
  vec_ch = TRUE;

  vec_sz = size;
}



/******************************************************************************
Function: ()

Input :
Output:
Return:
Description:
******************************************************************************/
#if defined UPCASE
void GS_INIT_MSG_BUF_SZ (int *buf_size)
#else
void  gs_init_msg_buf_sz_ (int *buf_size)
#endif
{
  gs_init_msg_buf_sz(*buf_size);
}

/******************************************************************************
Function: gs_init_()

Input : 
Output: 
Return: 
Description:  
******************************************************************************/
void gs_init_msg_buf_sz(int buf_size)
{
  msg_ch = TRUE;

  msg_buf = buf_size;
}



/******************************************************************************
Function: gs_init_()

Input : 
Output: 
Return: 
Description:  
******************************************************************************/
#if defined UPCASE
int
GS_INIT  (int *elms, int *nel, int *level)
#else
int
gs_init_ (int *elms, int *nel, int *level)
#endif
{
  gs_id *gsh;

  if (num_gs_ids==MAX_GS_IDS)
    {error_msg_fatal("gs_init_() :: no more than %d gs handles",MAX_GS_IDS);}

  gsh =  gs_init(elms, *nel, *level);

  gs_handles[gsh->id - 1] = gsh;

  return(gsh->id);
}



/******************************************************************************
Function: gs_gop_ ()

Input :

Output:

RETURN:

Description:
******************************************************************************/
#if defined UPCASE
extern void GS_GOP  (int *gs, REAL *vals, char *op)
#else
extern void gs_gop_ (int *gs, REAL *vals, char *op)
#endif
{
  gs_id *gsh;


  gsh = gs_handles[*gs-1];


  gs_gop(gsh,vals,op);
}



/******************************************************************************
Function: gs_gop_vec_ ()

Input :

Output:

RETURN:

Description:
******************************************************************************/
#if defined UPCASE
extern void GS_GOP_VEC  (int *gs, REAL *vals, char *op, int *step)
#else
extern void gs_gop_vec_ (int *gs, REAL *vals, char *op, int *step)
#endif
{
  gs_id *gsh;


  gsh = gs_handles[*gs-1];


  gs_gop_vec(gsh,vals,op,*step);
}



/******************************************************************************
Function: gs_free_ ()

Input :

Output:

RETURN:

Description:
******************************************************************************/
#if defined UPCASE
extern void GS_FREE  (int *gs)
#else
extern void gs_free_ (int *gs)
#endif
{
  gs_id *gsh;


  gsh = gs_handles[*gs-1];

  gs_free(gsh);
}



/******************************************************************************
Function: gs_init()

Input :

Output:

RETURN:

Description:
******************************************************************************/
gs_id *
gs_init(register int *elms, int nel, int level)
{
  register gs_id *gs;


  bss_init();
  perm_init();

#ifdef DEBUG
  error_msg_warning("gs_init() start w/(%d,%d)\n",my_id,num_nodes);
#endif

  /* ensure that communication package has been initialized */
  comm_init();

#ifdef INFO1
  bss_stats();
  perm_stats();
#endif

  /* determines if we have enough dynamic/semi-static memory */
  /* checks input, allocs and sets gd_id template            */
  gs = gsi_check_args(elms,nel,level);

  /* only bit mask version up and working for the moment    */
  /* LATER :: get int list version working for sparse pblms */
  gsi_via_bit_mask(gs);

  /* print out P0's template as well as malloc stats */
#ifdef INFO1
  gs_print_template(gs,3);
  gs_print_template(gs,0);
#endif

#ifdef DEBUG
  error_msg_warning("gs_init() end w/(%d,%d)\n",my_id,num_nodes);
#endif

  bss_stats();
  perm_stats();

  return(gs);
}



/******************************************************************************
Function: gsi_new()

Input :
Output:
Return:
Description:

elm list must >= 0!!!
elm repeats allowed
******************************************************************************/
static
gs_id *
gsi_new(void)
{
  int size=sizeof(gs_id);
  gs_id *gs;


#ifdef DEBUG
  error_msg_warning("gsi_new() :: size=%d\n",size);
#endif

  gs = (gs_id *) perm_malloc(size);

  if (!(size%REAL_LEN))
    {rvec_zero((REAL *)gs,size/REAL_LEN);}
  else if (!(size%INT_LEN))
    {ivec_zero((INT *)gs,size/INT_LEN);}
  else if (!(size%sizeof(char)))
    {memset((char *)gs,0,size/sizeof(char));}
  else
    {error_msg_fatal("gsi_new() :: can't initialize gs template!\n");}

  return(gs);
}



/******************************************************************************
Function: gsi_check_args()

Input :
Output:
Return:
Description:

elm list must >= 0!!!
elm repeats allowed
local working copy of elms is sorted
******************************************************************************/
static
gs_id *
gsi_check_args(int *in_elms, int nel, int level)
{
  register int i, j, k, t2;
  int *companion, *elms, *unique, *iptr;
  int num_local=0, *num_to_reduce, **local_reduce;
  int oprs[] = {NON_UNIFORM,GL_MIN,GL_MAX,GL_ADD,GL_MIN,GL_MAX,GL_MIN,GL_B_AND};
  int vals[sizeof(oprs)/sizeof(oprs[0])-1];
  int work[sizeof(oprs)/sizeof(oprs[0])-1];
  gs_id *gs;


#ifdef DEBUG
  error_msg_warning("gs_check_args() begin w/(%d,%d)\n",my_id,num_nodes);
#endif

#ifdef SAFE
  if (!in_elms)
    {error_msg_fatal("elms point to nothing!!!\n");}

  if (nel<0)
    {error_msg_fatal("can't have fewer than 0 elms!!!\n");}

  if (nel==0)
    {error_msg_warning("I don't have any elements!!!\n");}
#endif

  /* get space for gs template */
  gs = gsi_new();
  gs->id = ++num_gs_ids;

  /* copy over in_elms list and create inverse map */
  elms = (int *) bss_malloc((nel+1)*INT_LEN);
  companion = (int *) bss_malloc(nel*INT_LEN);
  ivec_c_index(companion,nel);
  ivec_copy(elms,in_elms,nel);

#ifdef SAFE
  /* pre-pass ... check to see if sorted */
  elms[nel] = INT_MAX;
  iptr = elms;
  unique = elms+1;
  j=0;
  while (*iptr!=INT_MAX)
    {
      if (*iptr++>*unique++)
  {j=1; break;}
    }

  /* set up inverse map */
  if (j)
    {
      error_msg_warning("gsi_check_args() :: elm list *not* sorted!\n");
      SMI_sort((void *)elms, (void *)companion, nel, SORT_INTEGER);
    }
  else
    {error_msg_warning("gsi_check_args() :: elm list sorted!\n");}
#else
  SMI_sort((void *)elms, (void *)companion, nel, SORT_INTEGER);
#endif
  elms[nel] = INT_MIN;

  /* first pass */
  /* determine number of unique elements, check pd */
  for (i=k=0;i<nel;i+=j)
    {
      t2 = elms[i];
      j=++i;

      /* clump 'em for now */
      while (elms[j]==t2) {j++;}

      /* how many together and num local */
      if (j-=i)
  {num_local++; k+=j;}
    }

  /* how many unique elements? */
  gs->repeats=k;
  gs->nel = nel-k;


  /* number of repeats? */
  gs->num_local = num_local;

  /* two extras for sentinal and partition ... */
  num_local+=2;
  gs->local_reduce=local_reduce=(int **)perm_malloc(num_local*INT_PTR_LEN);
  gs->num_local_reduce=num_to_reduce=(int *) perm_malloc(num_local*INT_LEN);

  unique = (int *) bss_malloc((gs->nel+1)*INT_LEN);
  gs->elms = unique;
  gs->nel_total = nel;
  gs->local_elms = elms;
  gs->companion = companion;

  /* compess map as well as keep track of local ops */
  for (num_local=i=j=0;i<gs->nel;i++)
    {
      k=j;
      t2 = unique[i] = elms[j];
      companion[i] = companion[j];

      while (elms[j]==t2) {j++;}

      if ((t2=(j-k))>1)
  {
    /* number together */
    num_to_reduce[num_local] = t2++;
    iptr = local_reduce[num_local++] = (int *)perm_malloc(t2*INT_LEN);

    /* to use binary searching don't remap until we check intersection */
    *iptr++ = i;

    /* note that we're skipping the first one */
    while (++k<j)
      {*(iptr++) = companion[k];}
    *iptr = -1;
  }
    }

  /* sentinel for ngh_buf */
  unique[gs->nel]=INT_MAX;

#ifdef DEBUG
  if (num_local!=gs->num_local)
    {error_msg_fatal("compression of maps wrong!!!\n");}
#endif

  /* for two partition sort hack */
  num_to_reduce[num_local] = 0;
  local_reduce[num_local] = NULL;
  num_to_reduce[++num_local] = 0;
  local_reduce[num_local] = NULL;

  /* load 'em up */
  /* note one extra to hold NON_UNIFORM flag!!! */
  vals[2] = vals[1] = vals[0] = nel;
  if (gs->nel>0)
    {
       vals[3] = unique[0];           /* ivec_lb(elms,nel); */
       vals[4] = unique[gs->nel-1];   /* ivec_ub(elms,nel); */
    }
  else
    {
       vals[3] = INT_MAX;             /* ivec_lb(elms,nel); */
       vals[4] = INT_MIN;             /* ivec_ub(elms,nel); */
    }
  vals[5] = level;
  vals[6] = num_gs_ids;

  /* GLOBAL: send 'em out */
  giop(vals,work,sizeof(oprs)/sizeof(oprs[0])-1,oprs);

  /* must be semi-pos def - only pairwise depends on this */
  /* LATER - remove this restriction */
  if (vals[3]<0)
    {error_msg_fatal("gsi_check_args() :: system not semi-pos def ::%d\n",vals[3]);}

  if (vals[4]==INT_MAX)
    {error_msg_fatal("gsi_check_args() :: system ub too large ::%d!\n",vals[4]);}

#ifdef DEBUG
  /* check gs template count */
  if (vals[6] != num_gs_ids)
    {error_msg_fatal("num_gs_ids mismatch!!!");}

  /* check all have same level threshold */
  if (level != vals[5])
    {error_msg_fatal("gsi_check_args() :: level not uniform across nodes!!!\n");}
#endif

  gs->nel_min = vals[0];
  gs->nel_max = vals[1];
  gs->nel_sum = vals[2];
  gs->gl_min  = vals[3];
  gs->gl_max  = vals[4];
  gs->negl    = vals[4]-vals[3]+1;

#ifdef DEBUG
      printf("nel(unique)=%d\n", gs->nel);
      printf("nel_max=%d\n",     gs->nel_max);
      printf("nel_min=%d\n",     gs->nel_min);
      printf("nel_sum=%d\n",     gs->nel_sum);
      printf("negl=%d\n",        gs->negl);
      printf("gl_max=%d\n",      gs->gl_max);
      printf("gl_min=%d\n",      gs->gl_min);
      printf("elms ordered=%d\n",gs->ordered);
      printf("repeats=%d\n",     gs->repeats);
      printf("positive=%d\n",    gs->positive);
      printf("level=%d\n",       gs->level);
#endif

  if (gs->negl<=0)
    {error_msg_fatal("gsi_check_args() :: system empty or neg :: %d\n",gs->negl);}

  /* LATER :: add level == -1 -> program selects level */
  if (vals[5]<0)
    {vals[5]=0;}
  else if (vals[5]>num_nodes)
    {vals[5]=num_nodes;}
  gs->level = vals[5];

#ifdef DEBUG
  error_msg_warning("gs_check_args() :: end w/(%d,%d)+level=%d\n",
        my_id,num_nodes,vals[5]);
#endif

  return(gs);
}


/******************************************************************************
Function: gsi_via_bit_mask()

Input :
Output:
Return:
Description:


******************************************************************************/
static
void
gsi_via_bit_mask(gs_id *gs)
{
  register int i, nel, *elms;
  int t1,t2,op;
  int **reduce;
  int *map;

#ifdef DEBUG
  error_msg_warning("gsi_via_bit_mask() begin w/%d :: %d\n",my_id,num_nodes);
#endif

  /* totally local removes ... ct_bits == 0 */
  get_ngh_buf(gs);

  if (gs->level)
    {set_pairwise(gs);}

  if (gs->max_left_over)
    {set_tree(gs);}

  /* intersection local and pairwise/tree? */
  gs->num_local_total = gs->num_local;
  gs->gop_local_reduce = gs->local_reduce;
  gs->num_gop_local_reduce = gs->num_local_reduce;

  map = gs->companion;

  /* is there any local compression */
  if (gs->num_local == 0)
    {
      gs->local_strength = NONE;
      gs->num_local_gop = 0;
    }
  else
    {
      /* ok find intersection */
      map = gs->companion;
      reduce = gs->local_reduce;
      for (i=0, t1=0; i<gs->num_local; i++, reduce++)
  {
    if ((ivec_binary_search(**reduce,gs->pw_elm_list,gs->len_pw_list)>=0)
        ||
        ivec_binary_search(**reduce,gs->tree_map_in,gs->tree_map_sz)>=0)
      {
        /* printf("C%d :: i=%d, **reduce=%d\n",my_id,i,**reduce); */
        t1++;
        if (gs->num_local_reduce[i]<=0)
    {error_msg_fatal("nobody in list?");}
        gs->num_local_reduce[i] *= -1;
      }
     **reduce=map[**reduce];
  }

      /* intersection is empty */
      if (!t1)
  {
#ifdef DEBUG
    error_msg_warning("gsi_check_args() :: local gs_gop w/o intersection!");
#endif
    gs->local_strength = FULL;
    gs->num_local_gop = 0;
  }
      /* intersection not empty */
      else
  {
#ifdef DEBUG
    error_msg_warning("gsi_check_args() :: local gs_gop w/intersection!");
#endif
    gs->local_strength = PARTIAL;
    SMI_sort((void *)gs->num_local_reduce, (void *)gs->local_reduce,
       gs->num_local + 1, SORT_INT_PTR);

    gs->num_local_gop = t1;
    gs->num_local_total =  gs->num_local;
    gs->num_local    -= t1;
    gs->gop_local_reduce = gs->local_reduce;
    gs->num_gop_local_reduce = gs->num_local_reduce;

    for (i=0; i<t1; i++)
      {
        if (gs->num_gop_local_reduce[i]>=0)
    {error_msg_fatal("they aren't negative?");}
        gs->num_gop_local_reduce[i] *= -1;
        gs->local_reduce++;
        gs->num_local_reduce++;
      }
    gs->local_reduce++;
    gs->num_local_reduce++;
  }
    }

  elms = gs->pw_elm_list;
  nel  = gs->len_pw_list;
  for (i=0; i<nel; i++)
    {elms[i] = map[elms[i]];}

  elms = gs->tree_map_in;
  nel  = gs->tree_map_sz;
  for (i=0; i<nel; i++)
    {elms[i] = map[elms[i]];}

  /* clean up */
  bss_free((void *) gs->local_elms);
  bss_free((void *) gs->companion);
  bss_free((void *) gs->elms);
  bss_free((void *) gs->ngh_buf);
  gs->local_elms = gs->companion = gs->elms = gs->ngh_buf = NULL;

#ifdef DEBUG
  error_msg_warning("gsi_via_bit_mask() end w/%d :: %d\n",my_id,num_nodes);
#endif
}



/******************************************************************************
Function: place_in_tree()

Input :
Output:
Return:
Description:


******************************************************************************/
static
void
place_in_tree(register int elm)
{
  register int *tp, n;


  if (ntree==tree_buf_sz)
    {
      if (tree_buf_sz)
  {
    tp = tree_buf;
    n = tree_buf_sz;
    tree_buf_sz<<=1;
    tree_buf = (int *)bss_malloc(tree_buf_sz*INT_LEN);
    ivec_copy(tree_buf,tp,n);
    bss_free(tp);
  }
      else
  {
    tree_buf_sz = TREE_BUF_SZ;
    tree_buf = (int *)bss_malloc(tree_buf_sz*INT_LEN);
  }
    }

  tree_buf[ntree++] = elm;
}



/******************************************************************************
Function: get_ngh_buf()

Input :
Output:
Return:
Description:


******************************************************************************/
static
void
get_ngh_buf(gs_id *gs)
{
  register int i, j, npw=0, ntree_map=0;
  int p_mask_size, ngh_buf_size, buf_size;
  int *p_mask, *sh_proc_mask, *pw_sh_proc_mask;
  int *ngh_buf, *buf1, *buf2;
  int offset, per_load, num_loads, or_ct, start, end;
  int *ptr1, *ptr2, i_start, negl, nel, *elms;
  int oper=GL_B_OR;
  int *ptr3, *t_mask, *in_elm, level, ct1, ct2, *tp;


#ifdef DEBUG
  error_msg_warning("get_ngh_buf() begin w/%d :: %d\n",my_id,num_nodes);
#endif

  /* to make life easier */
  nel   = gs->nel;
  elms  = gs->elms;
  level = gs->level;

  /* det #bytes needed for processor bit masks and init w/mask cor. to my_id */
  p_mask = (int *) bss_malloc(p_mask_size=len_bit_mask(num_nodes));
  set_bit_mask(p_mask,p_mask_size,my_id);

  /* allocate space for masks and info bufs */
  gs->nghs = sh_proc_mask = (int *) perm_malloc(p_mask_size);
  gs->pw_nghs = pw_sh_proc_mask = (int *) perm_malloc(p_mask_size);
  gs->ngh_buf_sz = ngh_buf_size = p_mask_size*nel;
  t_mask = (int *) bss_malloc(p_mask_size);
  gs->ngh_buf = ngh_buf = (int *) bss_malloc(ngh_buf_size);

  /* comm buffer size ... memory usage bounded by ~2*msg_buf */
  /* had thought I could exploit rendezvous threshold */

  /* default is one pass */
  per_load = negl  = gs->negl;
  gs->num_loads = num_loads = 1;
  i=p_mask_size*negl;
  long int ii;
  ii = ((long int) p_mask_size) * ((long int) negl);
  if ( ((long int) msg_buf) < ii )
    buf_size = msg_buf;
  else
    buf_size = ii;
/*
  buf_size = MIN(msg_buf,i);
*/

  /* can we do it? */
  if (p_mask_size>buf_size)
    {error_msg_fatal("get_ngh_bug() :: buf<pms :: %d>%d\n",p_mask_size,buf_size);}

  /* get giop buf space ... make *only* one malloc */
  buf1 = (int *) bss_malloc(buf_size<<1);

  /* more than one gior exchange needed? */
  if (buf_size!=i)
    {
      per_load = buf_size/p_mask_size;
      buf_size = per_load*p_mask_size;
      gs->num_loads = num_loads = negl/per_load + (negl%per_load>0);
    }

#ifdef DEBUG
  /* dump some basic info */
  error_msg_warning("n_lds=%d,pms=%d,buf_sz=%d\n",num_loads,p_mask_size,buf_size);
#endif

  /* convert buf sizes from #bytes to #ints - 32 bit only! */
#ifdef SAFE
  p_mask_size/=INT_LEN; ngh_buf_size/=INT_LEN; buf_size/=INT_LEN;
#else
  p_mask_size>>=2; ngh_buf_size>>=2; buf_size>>=2;
#endif

  /* find giop work space */
  buf2 = buf1+buf_size;

  /* hold #ints needed for processor masks */
  gs->mask_sz=p_mask_size;

  /* init buffers */
  ivec_zero(sh_proc_mask,p_mask_size);
  ivec_zero(pw_sh_proc_mask,p_mask_size);
  ivec_zero(ngh_buf,ngh_buf_size);

  /* HACK reset tree info */
  tree_buf=NULL;
  tree_buf_sz=ntree=0;

  /* queue the tree elements for now */
  /* elms_q = new_queue(); */

  /* can also queue tree info for pruned or forest implememtation */
  /*  mask_q = new_queue(); */

  /* ok do it */
  for (ptr1=ngh_buf,ptr2=elms,end=gs->gl_min,or_ct=i=0; or_ct<num_loads; or_ct++)
    {
      /* identity for bitwise or is 000...000 */
      ivec_zero(buf1,buf_size);

      /* load msg buffer */
      for (start=end,end+=per_load,i_start=i; (offset=*ptr2)<end; i++, ptr2++)
  {
    offset = (offset-start)*p_mask_size;
    ivec_copy(buf1+offset,p_mask,p_mask_size);
  }

      /* GLOBAL: pass buffer */
      giop(buf1,buf2,buf_size,&oper);

      /* unload buffer into ngh_buf */
      ptr2=(elms+i_start);
      for(ptr3=buf1,j=start; j<end; ptr3+=p_mask_size,j++)
  {
    /* I own it ... may have to pairwise it */
    if (j==*ptr2)
      {
        /* do i share it w/anyone? */
#ifdef SAFE
        ct1 = ct_bits((char *)ptr3,p_mask_size*INT_LEN);
#else
        ct1 = ct_bits((char *)ptr3,p_mask_size<<2);
#endif
        /* guess not */
        if (ct1<2)
    {ptr2++; ptr1+=p_mask_size; continue;}

        /* i do ... so keep info and turn off my bit */
        ivec_copy(ptr1,ptr3,p_mask_size);
        ivec_xor(ptr1,p_mask,p_mask_size);
        ivec_or(sh_proc_mask,ptr1,p_mask_size);

        /* is it to be done pairwise? */
        if (--ct1<=level)
    {
      npw++;

      /* turn on high bit to indicate pw need to process */
      *ptr2++ |= TOP_BIT;
      ivec_or(pw_sh_proc_mask,ptr1,p_mask_size);
      ptr1+=p_mask_size;
      continue;
    }

        /* get set for next and note that I have a tree contribution */
        /* could save exact elm index for tree here -> save a search */
        ptr2++; ptr1+=p_mask_size; ntree_map++;
      }
    /* i don't but still might be involved in tree */
    else
      {

        /* shared by how many? */
#ifdef SAFE
        ct1 = ct_bits((char *)ptr3,p_mask_size*INT_LEN);
#else
        ct1 = ct_bits((char *)ptr3,p_mask_size<<2);
#endif

        /* none! */
        if (ct1<2)
    {continue;}

        /* is it going to be done pairwise? but not by me of course!*/
        if (--ct1<=level)
    {continue;}
      }
    /* LATER we're going to have to process it NOW */
    /* nope ... tree it */
    place_in_tree(j);
  }
    }

  bss_free((void *)t_mask);
  bss_free((void *)buf1);

  gs->len_pw_list=npw;

  gs->num_nghs = ct_bits((char *)sh_proc_mask,p_mask_size*INT_LEN);
  gs->num_pw_nghs = ct_bits((char *)pw_sh_proc_mask,p_mask_size*INT_LEN);


  gs->tree_map_sz  = ntree_map;
  gs->max_left_over=ntree;

  bss_free((void *)p_mask);

#ifdef DEBUG
  error_msg_warning("get_ngh_buf() end w/%d :: %d\n",my_id,num_nodes);
#endif
}





/******************************************************************************
Function: pairwise_init()

Input :
Output:
Return:
Description:

if an element is shared by fewer that level# of nodes do pairwise exch
******************************************************************************/
static
void
set_pairwise(gs_id *gs)
{
  register int i, j;
  int p_mask_size;
  int *p_mask, *sh_proc_mask, *tmp_proc_mask;
  int *ngh_buf, *buf2;
  int offset;
  int *msg_list, *msg_size, **msg_nodes, nprs;
  int *pairwise_elm_list, len_pair_list=0;
  int *iptr, t1, i_start, nel, *elms;
  int ct, level;


#ifdef DEBUG
  error_msg_warning("set_pairwise() begin w/%d :: %d\n",my_id,num_nodes);
#endif

  /* to make life easier */
  nel  = gs->nel;
  elms = gs->elms;
  ngh_buf = gs->ngh_buf;
  sh_proc_mask  = gs->pw_nghs;
  level = gs->level;

  /* need a few temp masks */
  p_mask_size   = len_bit_mask(num_nodes);
  p_mask        = (int *) bss_malloc(p_mask_size);
  tmp_proc_mask = (int *) bss_malloc(p_mask_size);

  /* set mask to my my_id's bit mask */
  set_bit_mask(p_mask,p_mask_size,my_id);

#ifdef SAFE
  p_mask_size /= INT_LEN;
#else
  p_mask_size >>= 2;
#endif

  len_pair_list=gs->len_pw_list;
  gs->pw_elm_list=pairwise_elm_list=perm_malloc((len_pair_list+1)*INT_LEN);

  /* how many processors (nghs) do we have to exchange with? */
  nprs=gs->num_pairs=ct_bits((char *)sh_proc_mask,p_mask_size*INT_LEN);


  /* allocate space for gs_gop() info */
  gs->pair_list = msg_list = (int *)  perm_malloc(INT_LEN*nprs);
  gs->msg_sizes = msg_size  = (int *)  perm_malloc(INT_LEN*nprs);
  gs->node_list = msg_nodes = (int **) perm_malloc(INT_PTR_LEN*(nprs+1));

  /* init msg_size list */
  ivec_zero(msg_size,nprs);

  /* expand from bit mask list to int list */
  bm_to_proc((char *)sh_proc_mask,p_mask_size*INT_LEN,msg_list);

  /* keep list of elements being handled pairwise */
  for (i=j=0;i<nel;i++)
    {
      if (elms[i] & TOP_BIT)
  {elms[i] ^= TOP_BIT; pairwise_elm_list[j++] = i;}
    }
  pairwise_elm_list[j] = -1;

#ifdef DEBUG
  if (j!=len_pair_list)
    {error_msg_fatal("oops ... bad paiwise list in set_pairwise!");}
#endif

#if MPISRC
  gs->msg_ids_out = (MPI_Request *)  perm_malloc(sizeof(MPI_Request)*(nprs+1));
  gs->msg_ids_out[nprs] = MPI_REQUEST_NULL;
  gs->msg_ids_in = (MPI_Request *)  perm_malloc(sizeof(MPI_Request)*(nprs+1));
  gs->msg_ids_in[nprs] = MPI_REQUEST_NULL;
  gs->pw_vals = (REAL *) perm_malloc(REAL_LEN*len_pair_list*vec_sz);
#else
  gs->msg_ids_out = (int *)  perm_malloc(INT_LEN*(nprs+1));
  ivec_zero(gs->msg_ids_out,nprs);
  gs->msg_ids_out[nprs] = -1;
  gs->msg_ids_in = (int *)  perm_malloc(INT_LEN*(nprs+1));
  ivec_zero(gs->msg_ids_in,nprs);
  gs->msg_ids_in[nprs] = -1;
  gs->pw_vals = (REAL *) perm_malloc(REAL_LEN*len_pair_list*vec_sz);
#endif

  /* find who goes to each processor */
  for (i_start=i=0;i<nprs;i++)
    {
      /* processor i's mask */
      set_bit_mask(p_mask,p_mask_size*INT_LEN,msg_list[i]);

      /* det # going to processor i */
      for (ct=j=0;j<len_pair_list;j++)
  {
    buf2 = ngh_buf+(pairwise_elm_list[j]*p_mask_size);
    ivec_and3(tmp_proc_mask,p_mask,buf2,p_mask_size);
    if (ct_bits((char *)tmp_proc_mask,p_mask_size*INT_LEN))
      {ct++;}
  }
      msg_size[i] = ct;
      i_start = MAX(i_start,ct);

      /*space to hold nodes in message to first neighbor */
      msg_nodes[i] = iptr = (int *) perm_malloc(INT_LEN*(ct+1));

      for (j=0;j<len_pair_list;j++)
  {
    buf2 = ngh_buf+(pairwise_elm_list[j]*p_mask_size);
    ivec_and3(tmp_proc_mask,p_mask,buf2,p_mask_size);
    if (ct_bits((char *)tmp_proc_mask,p_mask_size*INT_LEN))
      {*iptr++ = j;}
  }
      *iptr = -1;
    }
  msg_nodes[nprs] = NULL;

#ifdef INFO1
  t1 = GL_MAX;
  giop(&i_start,&offset,1,&t1);
  gs->max_pairs = i_start;
#else
  gs->max_pairs = -1;
#endif

  /* remap pairwise in tail of gsi_via_bit_mask() */
  gs->msg_total = ivec_sum(gs->msg_sizes,nprs);
  gs->out = (REAL *) perm_malloc(REAL_LEN*gs->msg_total*vec_sz);
  gs->in  = (REAL *) perm_malloc(REAL_LEN*gs->msg_total*vec_sz);

  /* reset malloc pool */
  bss_free((void *)p_mask);
  bss_free((void *)tmp_proc_mask);

#ifdef DEBUG
  error_msg_warning("set_pairwise() end w/%d :: %d\n",my_id,num_nodes);
#endif
}



/******************************************************************************
Function: set_tree()

Input :
Output:
Return:
Description:

to do pruned tree just save ngh buf copy for each one and decode here!
******************************************************************************/
static
void
set_tree(gs_id *gs)
{
  register int i, j, n, nel;
  register int *iptr_in, *iptr_out, *tree_elms, *elms, *map;


#ifdef DEBUG
  error_msg_warning("set_tree() :: begin\n");
#endif

  /* local work ptrs */
  elms = gs->elms;
  nel     = gs->nel;
  map = gs->companion;

  /* how many via tree */
  gs->tree_nel  = n = ntree;
  gs->tree_elms = tree_elms = iptr_in = tree_buf;
  gs->tree_buf  = (REAL *) bss_malloc(REAL_LEN*n*vec_sz);
  gs->tree_work = (REAL *) bss_malloc(REAL_LEN*n*vec_sz);
  j=gs->tree_map_sz;
  gs->tree_map_in = iptr_in  = (int *) bss_malloc(INT_LEN*(j+1));
  gs->tree_map_out = iptr_out = (int *) bss_malloc(INT_LEN*(j+1));

#ifdef DEBUG
  error_msg_warning("num on tree=%d,%d",gs->max_left_over,gs->tree_nel);
#endif

  /* search the longer of the two lists */
  /* note ... could save this info in get_ngh_buf and save searches */
  if (n<=nel)
    {
      /* bijective fct w/remap - search elm list */
      for (i=0; i<n; i++)
  {
    if ((j=ivec_binary_search(*tree_elms++,elms,nel))>=0)
      {*iptr_in++ = j; *iptr_out++ = i;}
  }
    }
  else
    {
      for (i=0; i<nel; i++)
  {
    if ((j=ivec_binary_search(*elms++,tree_elms,n))>=0)
      {*iptr_in++ = i; *iptr_out++ = j;}
  }
    }

  /* sentinel */
  *iptr_in = *iptr_out = -1;

#ifdef DEBUG
  error_msg_warning("set_tree() :: end\n");
#endif
}


/******************************************************************************
Function: gsi_via_int_list()

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gsi_via_int_list(gs_id *gs)
{

  /* LATER: for P large the bit masks -> too many passes */
  /* LATER: strategy: do gsum w/1 in position i in negl if owner */
  /* LATER: then sum of entire vector 1 ... negl determines min buf len */
  /* LATER: So choose min from this or mask method */
}


static
int
root_sub_tree(int *proc_list, int num)
{
  register int i, j, p_or, p_and;
  register int root, mask;


  /* ceiling(log2(num_nodes)) - 1 */
  j = i_log2_num_nodes;
  if (num_nodes==floor_num_nodes)
    {j--;}

  /* set mask to msb */
  for(mask=1,i=0; i<j; i++)
    {mask<<=1;}

  p_or  = ivec_reduce_or(proc_list,num);
  p_and = ivec_reduce_and(proc_list,num);
  for(root=i=0; i<j; i++,mask>>=1)
    {
      /* (msb-i)'th bits on ==> root in right 1/2 tree */
      if (mask & p_and)
  {root |= mask;}

      /* (msb-i)'th bits differ ==> root found */
      else if (mask & p_or)
  {break;}

      /* (msb-i)'th bits off ==>root in left 1/2 tree */
    }

#ifdef DEBUG
  if ((root<0) || (root>num_nodes))
    {error_msg_fatal("root_sub_tree() :: bad root!");}

  if (!my_id)
    {
      printf("num_nodes=%d, j=%d, root=%d\n",num_nodes,j,root);
      printf("procs: ");
      for(i=0;i<num;i++)
  {printf("%d ",proc_list[i]);}
      printf("\n");
    }
#endif

  return(root);
}



static int
in_sub_tree(int *mask, int mask_size, int *work, int nw)
{
  int i, j, k, ct, nb;
  int root, *sh_mask;

  /* mask size in bytes */
  nb = mask_size<<2;

  /* shared amoungst how many? */
  ct = ct_bits((char *)mask,nb);

  /* enough space? */
  if (nw<ct)
    {error_msg_fatal("in_sub_tree() :: not enough space to expand bit mask!");}

  /* expand */
  bm_to_proc((char *)mask,nb,work);

  /* find tree root */
  root = root_sub_tree(work,ct);

  /* am i in any of the paths? */

  return(TRUE);

  /*
  sh_mask = (int *)bss_malloc(nb);
  bss_free(sh_mask);
  */
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_out(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      /* wall */
      if (*num == 2)
  {
    num ++;
    vals[map[1]] = vals[map[0]];
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++;
    vals[map[2]] = vals[map[1]] = vals[map[0]];
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++;
    vals[map[3]] = vals[map[2]] = vals[map[1]] = vals[map[0]];
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num++;
    tmp = *(vals + *map++);
    while (*map >= 0)
      {*(vals + *map++) = tmp;}
  }
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
void
gs_gop_binary(gs_ADT gs, REAL *vals, rbfp fct)
{
#ifdef DEBUG
  if (!gs)  {error_msg_fatal("gs_gop() :: passed NULL gs handle!!!");}
  if (!fct) {error_msg_fatal("gs_gop() :: passed NULL bin fct handle!!!");}
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_binary(gs,vals,fct);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_binary(gs,vals,fct);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_binary(gs,vals,fct);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_binary(gs,vals,fct);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_binary(gs,vals,fct);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_binary(gs,vals,fct);}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_binary(register gs_id *gs, register REAL *vals, register rbfp fct)
{
  register int *num, *map, **reduce;
  REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif
  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      num ++;
      (*fct)(&tmp,NULL,1);
      /* tmp = 0.0; */
      while (*map >= 0)
  {(*fct)(&tmp,(vals + *map),1); map++;}
  /*        {tmp = (*fct)(tmp,*(vals + *map)); map++;} */

      map = *reduce++;
      while (*map >= 0)
        {*(vals + *map++) = tmp;}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_binary(register gs_id *gs, register REAL *vals, register rbfp fct)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;

  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      num++;
      base = vals + *map++;
      while (*map >= 0)
  {(*fct)(base,(vals + *map),1); map++;}
  /*        {*base = (*fct)(*base,*(vals + *map)); map++;} */
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_binary(register gs_id *gs, register REAL *in_vals,
                       register rbfp fct)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;

#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
        {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,
           *(msg_size++)*REAL_LEN,*msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_binary(gs,in_vals,fct);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {(*fct)((dptr1 + *iptr),in2,1); iptr++; in2++;}
  /* {*(dptr1 + *iptr) = (*fct)(*(dptr1 + *iptr),*in2); iptr++; in2++;} */
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
         second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
                MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
        {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
                MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  if (gs->max_left_over)
    {gs_gop_tree_binary(gs,in_vals,fct);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {(*fct)((dptr1 + *iptr),in2,1); iptr++; in2++;}
      /* {*(dptr1 + *iptr) = (*fct)(*(dptr1 + *iptr),*in2); iptr++; in2++;} */
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif


}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_binary(gs_id *gs, REAL *vals, register rbfp fct)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
#ifdef MPISRC
  MPI_Op op;
#endif


#ifdef DEBUG
  error_msg_warning("gs_gop_tree_binary() :: start\n");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

  /* load vals vector w/identity */
  (*fct)(buf,NULL,size);

  /* load my contribution into val vector */
  while (*in >= 0)
    {(*fct)((buf + *out++),(vals + *in++),-1);}
/*    {*(buf + *out++) = *(vals + *in++);} */

  gop(buf,work,size,fct,REAL_TYPE,0);

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  while (*in >= 0)
    {(*fct)((vals + *in++),(buf + *out++),-1);}
    /*    {*(vals + *in++) = *(buf + *out++);} */


#ifdef DEBUG
  error_msg_warning("gs_gop_tree_binary() :: end\n");
#endif

}




/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
void
gs_gop(register gs_id *gs, register REAL *vals, register char *op)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop()\n");
  if (!gs) {error_msg_fatal("gs_gop() :: passed NULL gs handle!!!");}
  if (!op) {error_msg_fatal("gs_gop() :: passed NULL operation!!!");}
#endif

  switch (*op) {
  case '+':
    gs_gop_plus(gs,vals);
    break;
  case '*':
    gs_gop_times(gs,vals);
    break;
  case 'a':
    gs_gop_min_abs(gs,vals);
    break;
  case 'A':
    gs_gop_max_abs(gs,vals);
    break;
  case 'e':
    gs_gop_exists(gs,vals);
    break;
  case 'm':
    gs_gop_min(gs,vals);
    break;
  case 'M':
    gs_gop_max(gs,vals); break;
    /*
    if (*(op+1)=='\0')
      {gs_gop_max(gs,vals); break;}
    else if (*(op+1)=='X')
      {gs_gop_max_abs(gs,vals); break;}
    else if (*(op+1)=='N')
      {gs_gop_min_abs(gs,vals); break;}
    */
  default:
    error_msg_warning("gs_gop() :: %c is not a valid op",op[0]);
    error_msg_warning("gs_gop() :: default :: plus");
    gs_gop_plus(gs,vals);
    break;
  }
#ifdef DEBUG
  error_msg_warning("end gs_gop()\n");
#endif
}


/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_exists(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_exists(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_exists(gs,vals);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_exists(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_exists(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_exists(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_exists(gs,vals);}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_exists(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      num ++;
      tmp = 0.0;
      while (*map >= 0)
  {tmp = EXISTS(tmp,*(vals + *map)); map++;}

      map = *reduce++;
      while (*map >= 0)
  {*(vals + *map++) = tmp;}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_exists(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      num++;
      base = vals + *map++;
      while (*map >= 0)
  {*base = EXISTS(*base,*(vals + *map)); map++;}
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_exists(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_exists(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = EXISTS(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  if (gs->max_left_over)
    {gs_gop_tree_exists(gs,in_vals);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = EXISTS(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_exists(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_EXISTS,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_exists()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = 0.0;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 0.0;
  scopy(size,work,0,buf,1);
#else
  rvec_zero(buf,size);
#endif

  while (*in >= 0)
    {
      /*
      printf("%d :: out=%d\n",my_id,*out);
      printf("%d :: in=%d\n",my_id,*in);
      */
      *(buf + *out++) = *(vals + *in++);
    }

  grop(buf,work,size,op);

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;

  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}

#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_exists()");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_max_abs(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_max_abs(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_max_abs(gs,vals);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_max_abs(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_max_abs(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_max_abs(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_max_abs(gs,vals);}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_max_abs(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      num ++;
      tmp = 0.0;
      while (*map >= 0)
  {tmp = MAX_FABS(tmp,*(vals + *map)); map++;}

      map = *reduce++;
      while (*map >= 0)
  {*(vals + *map++) = tmp;}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_max_abs(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      num++;
      base = vals + *map++;
      while (*map >= 0)
  {*base = MAX_FABS(*base,*(vals + *map)); map++;}
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_max_abs(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;



#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_max_abs(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MAX_FABS(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  if (gs->max_left_over)
    {gs_gop_tree_max_abs(gs,in_vals);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MAX_FABS(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_max_abs(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_MAX_ABS,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_max_abs()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = 0.0;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 0.0;
  scopy(size,work,0,buf,1);
#else
  rvec_zero(buf,size);
#endif

  while (*in >= 0)
    {
      /*
      printf("%d :: out=%d\n",my_id,*out);
      printf("%d :: in=%d\n",my_id,*in);
      */
      *(buf + *out++) = *(vals + *in++);
    }

  grop(buf,work,size,op);

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;

  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}

#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_max_abs()");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_max(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif


  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_max(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_max(gs,vals);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_max(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_max(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_max(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_max(gs,vals);}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_max(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      num ++;
      tmp = -REAL_MAX;
      while (*map >= 0)
  {tmp = MAX(tmp,*(vals + *map)); map++;}

      map = *reduce++;
      while (*map >= 0)
  {*(vals + *map++) = tmp;}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_max(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      num++;
      base = vals + *map++;
      while (*map >= 0)
  {*base = MAX(*base,*(vals + *map)); map++;}
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_max(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_max(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MAX(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  if (gs->max_left_over)
    {gs_gop_tree_max(gs,in_vals);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MAX(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_max(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_MAX,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_max()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = -REAL_MAX;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 1.0;
  scopy(size,work,0,buf,1);
#else
  rvec_set(buf,-REAL_MAX,size);
#endif

  while (*in >= 0)
    {*(buf + *out++) = *(vals + *in++);}

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
#if   NXSRC&&r8
  gdhigh(buf,size,work);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#elif MPISRC
  MPI_Allreduce(buf,work,size,REAL_TYPE,MPI_MAX,MPI_COMM_WORLD);
  while (*in >= 0)
    {*(vals + *in++) = *(work + *out++);}
#else
  grop(buf,work,size,op);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#endif

#ifdef DEBUG
  error_msg_warning("end gs_gop_tree_max()");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_min_abs(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_min_abs(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_min_abs(gs,vals);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_min_abs(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_min_abs(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_min_abs(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_min_abs(gs,vals);}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_min_abs(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      num ++;
      tmp = REAL_MAX;
      while (*map >= 0)
  {tmp = MIN_FABS(tmp,*(vals + *map)); map++;}

      map = *reduce++;
      while (*map >= 0)
  {*(vals + *map++) = tmp;}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_min_abs(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;

#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      num++;
      base = vals + *map++;
      while (*map >= 0)
  {*base = MIN_FABS(*base,*(vals + *map)); map++;}
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_min_abs(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_min_abs(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MIN_FABS(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  if (gs->max_left_over)
    {gs_gop_tree_min_abs(gs,in_vals);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MIN_FABS(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_min_abs(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_MIN_ABS,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_min_abs()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = REAL_MAX;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 1.0;
  scopy(size,work,0,buf,1);
#else
  rvec_set(buf,REAL_MAX,size);
#endif

  while (*in >= 0)
    {*(buf + *out++) = *(vals + *in++);}

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  grop(buf,work,size,op);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}

#ifdef DEBUG
  error_msg_warning("end gs_gop_tree_min_abs()");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_min(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_min(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_min(gs,vals);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_min(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_min(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_min(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_min(gs,vals);}
    }
#ifdef DEBUG
  error_msg_warning("end gs_gop_xxx()\n");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_min(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      num ++;
      tmp = REAL_MAX;
      while (*map >= 0)
  {tmp = MIN(tmp,*(vals + *map)); map++;}

      map = *reduce++;
      while (*map >= 0)
  {*(vals + *map++) = tmp;}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_min(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      num++;
      base = vals + *map++;
      while (*map >= 0)
  {*base = MIN(*base,*(vals + *map)); map++;}
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_min(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_min(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MIN(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  /* process the received data */
  if (gs->max_left_over)
    {gs_gop_tree_min(gs,in_vals);}

  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr) = MIN(*(dptr1 + *iptr),*in2); iptr++; in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_min(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_MIN,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_min()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = REAL_MAX;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 1.0;
  scopy(size,work,0,buf,1);
#else
  rvec_set(buf,REAL_MAX,size);
#endif

  while (*in >= 0)
    {*(buf + *out++) = *(vals + *in++);}

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
#if   NXSRC&&r8
  gdlow(buf,size,work);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#elif MPISRC
  MPI_Allreduce(buf,work,size,REAL_TYPE,MPI_MIN,MPI_COMM_WORLD);
  while (*in >= 0)
    {*(vals + *in++) = *(work + *out++);}
#else
  grop(buf,work,size,op);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#endif

#ifdef DEBUG
  error_msg_warning("end gs_gop_tree_min()");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_times(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_times()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_times(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_times(gs,vals);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_times(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_times(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_pairwise_times(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_times(gs,vals);}
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_times(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      /* wall */
      if (*num == 2)
  {
    num ++; reduce++;
    vals[map[1]] = vals[map[0]] *= vals[map[1]];
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++; reduce++;
    vals[map[2]]=vals[map[1]]=vals[map[0]]*=(vals[map[1]]*vals[map[2]]);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++; reduce++;
    vals[map[1]]=vals[map[2]]=vals[map[3]]=vals[map[0]] *=
                           (vals[map[1]] * vals[map[2]] * vals[map[3]]);
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num ++;
     tmp = 1.0;
    while (*map >= 0)
      {tmp *= *(vals + *map++);}

    map = *reduce++;
    while (*map >= 0)
      {*(vals + *map++) = tmp;}
  }
    }
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_times(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      /* wall */
      if (*num == 2)
  {
    num ++;
    vals[map[0]] *= vals[map[1]];
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++;
    vals[map[0]] *= (vals[map[1]] * vals[map[2]]);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++;
    vals[map[0]] *= (vals[map[1]] * vals[map[2]] * vals[map[3]]);
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num++;
    base = vals + *map++;
    while (*map >= 0)
      {*base *= *(vals + *map++);}
  }
    }
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_times(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;



#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/
  if (gs->max_left_over)
    {gs_gop_tree_times(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr++) *= *in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  if (gs->max_left_over)
    {gs_gop_tree_times(gs,in_vals);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr++) *= *in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}
#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_times(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_MULT,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_times()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = 1.0;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 1.0;
  scopy(size,work,0,buf,1);
#else
  rvec_one(buf,size);
#endif

  while (*in >= 0)
    {*(buf + *out++) = *(vals + *in++);}

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
#if  NXSRC&&r8
  gdprod(buf,size,work);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#elif MPISRC
  MPI_Allreduce(buf,work,size,REAL_TYPE,MPI_PROD,MPI_COMM_WORLD);
  while (*in >= 0)
    {*(vals + *in++) = *(work + *out++);}
#else
  grop(buf,work,size,op);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#endif

#ifdef DEBUG
  error_msg_warning("end gs_gop_tree_times()");
#endif
}



/******************************************************************************
Function: gather_scatter


Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_plus(register gs_id *gs, register REAL *vals)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_plus()\n");
#endif

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_plus(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_plus(gs,vals);

      /* pairwise will do tree inside ... */
      if (gs->num_pairs)
  {gs_gop_pairwise_plus(gs,vals);}

      /* tree only */
      else if (gs->max_left_over)
  {gs_gop_tree_plus(gs,vals);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise will do tree inside */
      if (gs->num_pairs)
  {gs_gop_pairwise_plus(gs,vals);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_plus(gs,vals);}
    }

#ifdef DEBUG
  error_msg_warning("end gs_gop_plus()\n");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_plus(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL tmp;


#ifdef DEBUG
  error_msg_warning("begin gs_gop_local_plus()\n");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      /* wall */
      if (*num == 2)
  {
    num ++; reduce++;
    vals[map[1]] = vals[map[0]] += vals[map[1]];
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++; reduce++;
    vals[map[2]]=vals[map[1]]=vals[map[0]]+=(vals[map[1]]+vals[map[2]]);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++; reduce++;
    vals[map[1]]=vals[map[2]]=vals[map[3]]=vals[map[0]] +=
                           (vals[map[1]] + vals[map[2]] + vals[map[3]]);
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num ++;
     tmp = 0.0;
    while (*map >= 0)
      {tmp += *(vals + *map++);}

    map = *reduce++;
    while (*map >= 0)
      {*(vals + *map++) = tmp;}
  }
    }
#ifdef DEBUG
  error_msg_warning("end gs_gop_local_plus()\n");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_local_in_plus(register gs_id *gs, register REAL *vals)
{
  register int *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("begin gs_gop_local_in_plus()\n");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      /* wall */
      if (*num == 2)
  {
    num ++;
    vals[map[0]] += vals[map[1]];
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++;
    vals[map[0]] += (vals[map[1]] + vals[map[2]]);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++;
    vals[map[0]] += (vals[map[1]] + vals[map[2]] + vals[map[3]]);
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num++;
    base = vals + *map++;
    while (*map >= 0)
      {*base += *(vals + *map++);}
  }
    }
#ifdef DEBUG
  error_msg_warning("end gs_gop_local_in_plus()\n");
#endif
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_plus(register gs_id *gs, register REAL *in_vals)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;


#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_plus() start\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
      in1 += *size++;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,*(msg_size++)*REAL_LEN,
           *msg_list++,0);
    }

  /* post the receives ... was here*/

  /* do the tree while we're waiting */
  if (gs->max_left_over)
    {gs_gop_tree_plus(gs,in_vals);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {*(dptr1 + *iptr++) += *in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_plus() end\n");
#endif

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_plus() start\n");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  /* do the tree while we're waiting */
  if (gs->max_left_over)
    {gs_gop_tree_plus(gs,in_vals);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr++) += *in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}

#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_plus() end\n");
#endif

#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_plus(gs_id *gs, REAL *vals)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_ADD,0};


#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_plus()\n");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = 0.0;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 0.0;
  scopy(size,work,0,buf,1);
#else
  rvec_zero(buf,size);
#endif

  while (*in >= 0)
    {*(buf + *out++) = *(vals + *in++);}

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
#if NXSRC&&r8
  gdsum(buf,size,work);

  /* grop(buf,work,size,op);  */
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#elif MPISRC
  MPI_Allreduce(buf,work,size,REAL_TYPE,MPI_SUM,MPI_COMM_WORLD);
  while (*in >= 0)
    {*(vals + *in++) = *(work + *out++);}
#else
  grop(buf,work,size,op);
  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}
#endif

#ifdef DEBUG
  error_msg_warning("end gs_gop_tree_plus()\n");
#endif
}



/******************************************************************************
Function: level_best_guess()

Input :
Output:
Return:
Description:
******************************************************************************/
static
int
level_best_guess(void)
{
  /* full pairwise for now */
  return(num_nodes);
}



/******************************************************************************
Function: gs_print_template()

Input :

Output:

Return:

Description:
******************************************************************************/
static
void
gs_print_template(register gs_id* gs, int who)
{
  register int j, k, *iptr, *iptr2;


  if ((my_id == who) && (num_gs_ids))
    {
      printf("\n\nP#%d's GS#%d template:\n", my_id, gs->id);
      printf("id=%d\n",          gs->id);
      printf("nel(unique)=%d\n", gs->nel);
      printf("nel_max=%d\n",     gs->nel_max);
      printf("nel_min=%d\n",     gs->nel_min);
      printf("nel_sum=%d\n",     gs->nel_sum);
      printf("negl=%d\n",        gs->negl);
      printf("gl_max=%d\n",      gs->gl_max);
      printf("gl_min=%d\n",      gs->gl_min);
      printf("elms ordered=%d\n",gs->ordered);
      printf("repeats=%d\n",     gs->repeats);
      printf("positive=%d\n",    gs->positive);
      printf("elms=%ld\n",        (PTRINT) gs->elms);
      printf("elms(total)=%ld\n", (PTRINT) gs->local_elms);
      printf("vals=%ld\n",        (PTRINT) gs->vals);
      printf("gl_bss_min=%d\n",  gs->gl_bss_min);
      printf("gl_perm_min=%d\n", gs->gl_perm_min);
      printf("level=%d\n",       gs->level);
      printf("proc_mask_sz=%d\n",gs->mask_sz);
      printf("sh_proc_mask=%ld\n",(PTRINT) gs->nghs);
      printf("ngh_buf_size=%d\n",gs->ngh_buf_sz);
      printf("ngh_buf=%ld\n",     (PTRINT) gs->ngh_buf);
      printf("num_nghs=%d\n",    gs->num_nghs);

      /* pairwise exchange information */
      printf("\nPaiwise Info:\n");
      printf("num_pairs=%d\n",   gs->num_pairs);
      printf("pair_list=%ld\n",   (PTRINT) gs->pair_list);
      printf("msg_sizes=%ld\n",   (PTRINT) gs->msg_sizes);
      printf("node_list=%ld\n",   (PTRINT) gs->node_list);
      printf("max_pairs=%d\n",   gs->max_pairs);
      printf("len_pw_list=%d\n", gs->len_pw_list);
      printf("pw_elm_list=%ld\n", (PTRINT) gs->pw_elm_list);

      printf("pw_elm_list: ");
      if (iptr = gs->pw_elm_list)
  {
    for (j=0;j<gs->len_pw_list;j++)
      {printf("%d ", *iptr); iptr++;}
  }
      printf("\n");


      printf("processor_list: ");
      if (iptr = gs->pair_list)
  {
    for (j=0;j<gs->num_pairs;j++)
      {printf("%d ", *iptr); iptr++;}
  }
      printf("\n");

      printf("size_list: ");
      if (iptr = gs->msg_sizes)
  {
    for (j=0;j<gs->num_pairs;j++)
      {printf("%d ", *iptr); iptr++;}
  }
      printf("\n");

      if (iptr = gs->pair_list)
  {
    for (j=0;j<gs->num_pairs;j++)
      {
        printf("node_list %d: ", *iptr);
        if (iptr2 = (gs->node_list)[j])
    {
      for (k=0;k<(gs->msg_sizes)[j];k++)
        {printf("%d ", *iptr2); iptr2++;}
    }
        iptr++;
        printf("\n");
      }
  }
      printf("\n");

      printf("elm_list(U): ");
      if (iptr = gs->elms)
  {
    for (j=0;j<gs->nel;j++)
      {printf("%d ", *iptr); iptr++;}
  }
      printf("\n");
      printf("\n");

      printf("elm_list(T): ");
      if (iptr = gs->local_elms)
  {
    for (j=0;j<gs->nel_total;j++)
      {printf("%d ", *iptr); iptr++;}
  }
      printf("\n");
      printf("\n");

      printf("map_list(T): ");
      if (iptr = gs->companion)
  {
    for (j=0;j<gs->nel;j++)
      {printf("%d ", *iptr); iptr++;}
  }
      printf("\n");
      printf("\n");


      /* local exchange information */
      printf("\nLocal Info:\n");
      printf("local_strength=%d\n",   gs->local_strength);
      printf("num_local_total=%d\n",  gs->num_local_total);
      printf("num_local=%d\n",        gs->num_local);
      printf("num_local_gop=%d\n",    gs->num_local_gop);
      printf("num_local_reduce=%ld\n", (PTRINT) gs->num_local_reduce);
      printf("local_reduce=%ld\n",     (PTRINT) gs->local_reduce);
      printf("num_gop_local_reduce=%ld\n", (PTRINT) gs->num_gop_local_reduce);
      printf("gop_local_reduce=%ld\n",     (PTRINT) gs->gop_local_reduce);
      printf("\n");

      for (j=0;j<gs->num_local;j++)
  {
    printf("local reduce_list %d: ", j);
    if (iptr2 = (gs->local_reduce)[j])
      {
        if ((gs->num_local_reduce)[j] <= 0)
    {printf("oops");}

        for (k=0;k<(gs->num_local_reduce)[j];k++)
    {printf("%d ", *iptr2); iptr2++;}
      }
    printf("\n");
  }

      printf("\n");
      printf("\n");

      for (j=0;j<gs->num_local_gop;j++)
  {
    printf("gop reduce_list %d: ", j);
    iptr2 = (gs->gop_local_reduce)[j];

    if ((gs->num_gop_local_reduce)[j] <= 0)
      {printf("oops");}


    for (k=0;k<(gs->num_gop_local_reduce)[j];k++)
      {printf("%d ", *iptr2); iptr2++;}
    printf("\n");
  }
      printf("\n");
      printf("\n");

      /* crystal router information */
      printf("\n\n");
      printf("Tree Info:\n");
      printf("max_left_over=%d\n",   gs->max_left_over);
      printf("num_in_list=%ld\n",    (PTRINT) gs->in_num);
      printf("in_list=%ld\n",        (PTRINT) gs->in_list);
      printf("num_out_list=%ld\n",   (PTRINT) gs->out_num);
      printf("out_list=%ld\n",       (PTRINT) gs->out_list);

      printf("\n\n");
    }
  fflush(stdout);
}



/******************************************************************************
Function: gs_free()

Input :

Output:

Return:

Description:
  if (gs->sss) {perm_free((void*) gs->sss);}
******************************************************************************/
void
gs_free(register gs_id *gs)
{
  register int i;


#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
  if (!gs) {error_msg_warning("NULL ptr passed to gs_free()"); return;}
#endif

  if (gs->nghs) {perm_free((void*) gs->nghs);}
  if (gs->pw_nghs) {perm_free((void*) gs->pw_nghs);}

  /* tree */
  if (gs->max_left_over)
    {
      if (gs->tree_elms) {bss_free((void*) gs->tree_elms);}
      if (gs->tree_buf) {bss_free((void*) gs->tree_buf);}
      if (gs->tree_work) {bss_free((void*) gs->tree_work);}
      if (gs->tree_map_in) {bss_free((void*) gs->tree_map_in);}
      if (gs->tree_map_out) {bss_free((void*) gs->tree_map_out);}
    }

  /* pairwise info */
  if (gs->num_pairs)
    {
      /* should be NULL already */
      if (gs->ngh_buf) {bss_free((void*) gs->ngh_buf);}
      if (gs->elms) {bss_free((void*) gs->elms);}
      if (gs->local_elms) {bss_free((void*) gs->local_elms);}
      if (gs->companion) {bss_free((void*) gs->companion);}

      /* only set if pairwise */
      if (gs->vals) {perm_free((void*) gs->vals);}
      if (gs->in) {perm_free((void*) gs->in);}
      if (gs->out) {perm_free((void*) gs->out);}
      if (gs->msg_ids_in) {perm_free((void*) gs->msg_ids_in);}
      if (gs->msg_ids_out) {perm_free((void*) gs->msg_ids_out);}
      if (gs->pw_vals) {perm_free((void*) gs->pw_vals);}
      if (gs->pw_elm_list) {perm_free((void*) gs->pw_elm_list);}
      if (gs->node_list)
  {
    for (i=0;i<gs->num_pairs;i++)
      {if (gs->node_list[i]) {perm_free((void*) gs->node_list[i]);}}
    perm_free((void*) gs->node_list);
  }
      if (gs->msg_sizes) {perm_free((void*) gs->msg_sizes);}
      if (gs->pair_list) {perm_free((void*) gs->pair_list);}
    }

  /* local info */
  if (gs->num_local_total>=0)
    {
      for (i=0;i<gs->num_local_total+1;i++)
  /*      for (i=0;i<gs->num_local_total;i++) */
  {
    if (gs->num_gop_local_reduce[i])
      {perm_free((void*) gs->gop_local_reduce[i]);}
  }
    }

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->gop_local_reduce) {perm_free((void*) gs->gop_local_reduce);}
  if (gs->num_gop_local_reduce) {perm_free((void*) gs->num_gop_local_reduce);}

  perm_free((void *) gs);
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:

NOT FUNCTIONAL - FROM OLD VERSION!!!
***********************************************************************/
static int
gs_dump_ngh(gs_id *id, int loc_num, int *num, int *ngh_list)
{
  register int size, *ngh_buf;

#ifdef DEBUG
  error_msg_warning("start gs_gop_xxx()\n");
#endif


  return(0);
  /*
  size = id->mask_sz;
  ngh_buf = id->ngh_buf;
  ngh_buf += (size*loc_num);

  size *= INT_LEN;
  *num = ct_bits((char *) ngh_buf, size);
  bm_to_proc((char *) ngh_buf, size, ngh_list);
  */
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
void
gs_gop_vec(register gs_id *gs, register REAL *vals, register char *op,
     register int step)
{
#ifdef DEBUG
  error_msg_warning("gs_gop_vec() start");
  if (!gs) {error_msg_fatal("gs_gop_vec() :: passed NULL gs handle!!!");}
  if (!op) {error_msg_fatal("gs_gop_vec() :: passed NULL operation!!!");}
#endif

  switch (*op) {
  case '+':
    gs_gop_vec_plus(gs,vals,step);
    break;
#ifdef NOT_YET
  case '*':
    gs_gop_times(gs,vals);
    break;
  case 'a':
    gs_gop_min_abs(gs,vals);
    break;
  case 'A':
    gs_gop_max_abs(gs,vals);
    break;
  case 'e':
    gs_gop_exists(gs,vals);
    break;
  case 'm':
    gs_gop_min(gs,vals);
    break;
  case 'M':
    gs_gop_max(gs,vals); break;
    /*
    if (*(op+1)=='\0')
      {gs_gop_max(gs,vals); break;}
    else if (*(op+1)=='X')
      {gs_gop_max_abs(gs,vals); break;}
    else if (*(op+1)=='N')
      {gs_gop_min_abs(gs,vals); break;}
    */
#endif
  default:
    error_msg_warning("gs_gop_vec() :: %c is not a valid op",op[0]);
    error_msg_warning("gs_gop_vec() :: default :: plus");
    gs_gop_vec_plus(gs,vals,step);
    break;
  }
#ifdef DEBUG
  error_msg_warning("gs_gop_vec() end");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_vec_plus(register gs_id *gs, register REAL *vals, register int step)
{
#ifdef DEBUG
  error_msg_warning("gs_gop_vec_plus() start");
#endif

  if (!gs) {error_msg_fatal("gs_gop_vec() passed NULL gs handle!!!");}

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_vec_local_plus(gs,vals,step);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_vec_local_in_plus(gs,vals,step);

      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_vec_pairwise_plus(gs,vals,step);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_vec_tree_plus(gs,vals,step);}

      gs_gop_vec_local_out(gs,vals,step);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise */
      if (gs->num_pairs)
  {gs_gop_vec_pairwise_plus(gs,vals,step);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_vec_tree_plus(gs,vals,step);}
    }
#ifdef DEBUG
  error_msg_warning("gs_gop_vec_plus() end");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_vec_local_plus(register gs_id *gs, register REAL *vals,
          register int step)
{
  register int i, *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("gs_gop_vec_local_plus() start");
#endif

  num    = gs->num_local_reduce;
  reduce = gs->local_reduce;
  while (map = *reduce)
    {
      base = vals + map[0] * step;

      /* wall */
      if (*num == 2)
  {
    num++; reduce++;
    rvec_add (base,vals+map[1]*step,step);
    rvec_copy(vals+map[1]*step,base,step);
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num++; reduce++;
    rvec_add (base,vals+map[1]*step,step);
    rvec_add (base,vals+map[2]*step,step);
    rvec_copy(vals+map[2]*step,base,step);
    rvec_copy(vals+map[1]*step,base,step);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num++; reduce++;
    rvec_add (base,vals+map[1]*step,step);
    rvec_add (base,vals+map[2]*step,step);
    rvec_add (base,vals+map[3]*step,step);
    rvec_copy(vals+map[3]*step,base,step);
    rvec_copy(vals+map[2]*step,base,step);
    rvec_copy(vals+map[1]*step,base,step);
  }
      /* general case ... odd geoms ... 3D */
      else
  {
    num++;
    while (*++map >= 0)
      {rvec_add (base,vals+*map*step,step);}

    map = *reduce;
    while (*++map >= 0)
      {rvec_copy(vals+*map*step,base,step);}

    reduce++;
  }
    }
#ifdef DEBUG
  error_msg_warning("gs_gop_vec_local_plus() end");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_vec_local_in_plus(register gs_id *gs, register REAL *vals,
       register int step)
{
  register int i, *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("gs_gop_vec_locel_in_plus() start");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      base = vals + map[0] * step;

      /* wall */
      if (*num == 2)
  {
    num ++;
    rvec_add(base,vals+map[1]*step,step);
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++;
    rvec_add(base,vals+map[1]*step,step);
    rvec_add(base,vals+map[2]*step,step);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++;
    rvec_add(base,vals+map[1]*step,step);
    rvec_add(base,vals+map[2]*step,step);
    rvec_add(base,vals+map[3]*step,step);
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num++;
    while (*++map >= 0)
      {rvec_add(base,vals+*map*step,step);}
  }
    }
#ifdef DEBUG
  error_msg_warning("gs_gop_vec_local_in_plus() end");
#endif
}


/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_vec_local_out(register gs_id *gs, register REAL *vals,
         register int step)
{
  register int i, *num, *map, **reduce;
  register REAL *base;


#ifdef DEBUG
  error_msg_warning("gs_gop_vec_local_out() start");
#endif

  num    = gs->num_gop_local_reduce;
  reduce = gs->gop_local_reduce;
  while (map = *reduce++)
    {
      base = vals + map[0] * step;

      /* wall */
      if (*num == 2)
  {
    num ++;
    rvec_copy(vals+map[1]*step,base,step);
  }
      /* corner shared by three elements */
      else if (*num == 3)
  {
    num ++;
    rvec_copy(vals+map[1]*step,base,step);
    rvec_copy(vals+map[2]*step,base,step);
  }
      /* corner shared by four elements */
      else if (*num == 4)
  {
    num ++;
    rvec_copy(vals+map[1]*step,base,step);
    rvec_copy(vals+map[2]*step,base,step);
    rvec_copy(vals+map[3]*step,base,step);
  }
      /* general case ... odd geoms ... 3D*/
      else
  {
    num++;
    while (*++map >= 0)
      {rvec_copy(vals+*map*step,base,step);}
  }
    }
#ifdef DEBUG
  error_msg_warning("gs_gop_vec_local_out() end");
#endif
}



/******************************************************************************
Function: gather_scatter

VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_vec_pairwise_plus(register gs_id *gs, register REAL *in_vals,
       register int step)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  register int i;

#ifdef DEBUG
  error_msg_warning("gs_gop_vec_pairwise_plus() start");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN*step);
      in1 += *size++ * step;
    }
  while (*msg_ids_in >= 0);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {
      rvec_copy(dptr3,in_vals + *iptr*step,step);
      dptr3+=step;
      iptr++;
    }

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {
    rvec_copy(dptr2,dptr1 + *iptr*step,step);
    dptr2+=step;
    iptr++;
  }
      *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,
           *(msg_size++)*REAL_LEN*step,*msg_list++,0);
    }

  /* post the receives ... was here*/
  /* tree */
  if (gs->max_left_over)
    {gs_gop_vec_tree_plus(gs,in_vals,step);}

  /* process the received data */
  while (iptr = *nodes++)
    {
      msgwait(*ids_in++);
      while (*iptr >= 0)
  {
#if BLAS&&r8
    daxpy(step,1.0,in2,1,dptr1 + *iptr*step,1);
#elif BLAS
    saxpy(step,1.0,in2,1,dptr1 + *iptr*step,1);
#else
    rvec_add(dptr1 + *iptr*step,in2,step);
#endif
    in2+=step;
    iptr++;
  }
    }

  /* replace vals */
  while (*pw >= 0)
    {
      rvec_copy(in_vals + *pw*step,dptr1,step);
      dptr1+=step;
      pw++;
    }

  /* clear isend message handles */
  while (*ids_out >= 0)
    {msgwait(*ids_out++);}

#ifdef DEBUG
  error_msg_warning("gs_gop_vec_pairwise_plus() end");
#endif

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;


#ifdef DEBUG
  error_msg_warning("gs_gop_vec_pairwise_plus() start");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
    MPI_COMM_WORLD, msg_ids_in++);
      in1 += *size++;
    }
  while (*++msg_nodes);
  msg_nodes=nodes;

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  while (iptr = *msg_nodes++)
    {
      dptr3 = dptr2;
      while (*iptr >= 0)
  {*dptr2++ = *(dptr1 + *iptr++);}
      /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
      /* is msg_ids_out++ correct? */
      MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *msg_list++,
    MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
    }

  /* tree */
  if (gs->max_left_over)
    {gs_gop_vec_tree_plus(gs,in_vals,step);}

  /* process the received data */
  msg_nodes=nodes;
  while (iptr = *nodes++)
    {
      /* Should I check the return value of MPI_Wait() or status? */
      /* Can this loop be replaced by a call to MPI_Waitall()? */
      MPI_Wait(ids_in++, &status);
      while (*iptr >= 0)
  {*(dptr1 + *iptr++) += *in2++;}
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    {MPI_Wait(ids_out++, &status);}

#ifdef DEBUG
  error_msg_warning("gs_gop_vec_pairwise_plus() end");
#endif

#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_vec_tree_plus(register gs_id *gs, register REAL *vals, register int step)
{
  register int size, *in, *out;
  register REAL *buf, *work;
  int op[] = {GL_ADD,0};

#ifdef DEBUG
  error_msg_warning("start gs_gop_vec_tree_plus()");
#endif

  /* copy over to local variables */
  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel*step;

  /* zero out collection buffer */
#if   BLAS&&r8
  *work = 0.0;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 0.0;
  scopy(size,work,0,buf,1);
#else
  rvec_zero(buf,size);
#endif


  /* copy over my contributions */
  while (*in >= 0)
    {
#if   BLAS&&r8
      dcopy(step,vals + *in++*step,1,buf + *out++*step,1);
#elif BLAS
      scopy(step,vals + *in++*step,1,buf + *out++*step,1);
#else
      rvec_copy(buf + *out++*step,vals + *in++*step,step);
#endif
    }

  /* perform fan in/out on full buffer */
  /* must change grop to handle the blas */
  grop(buf,work,size,op);

  /* reset */
  in   = gs->tree_map_in;
  out  = gs->tree_map_out;

  /* get the portion of the results I need */
  while (*in >= 0)
    {
#if   BLAS&&r8
      dcopy(step,buf + *out++*step,1,vals + *in++*step,1);
#elif BLAS
      scopy(step,buf + *out++*step,1,vals + *in++*step,1);
#else
      rvec_copy(vals + *in++*step,buf + *out++*step,step);
#endif
    }

#ifdef DEBUG
  error_msg_warning("start gs_gop_vec_tree_plus()");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
void
gs_gop_hc(register gs_id *gs, register REAL *vals, register char *op,
     register int dim)
{
#ifdef DEBUG
  error_msg_warning("gs_gop_hc() start");
  if (!gs) {error_msg_fatal("gs_gop_vec() :: passed NULL gs handle!!!");}
  if (!op) {error_msg_fatal("gs_gop_vec() :: passed NULL operation!!!");}
#endif

  switch (*op) {
  case '+':
    gs_gop_plus_hc(gs,vals,dim);
    break;
#ifdef NOT_YET
  case '*':
    gs_gop_times(gs,vals);
    break;
  case 'a':
    gs_gop_min_abs(gs,vals);
    break;
  case 'A':
    gs_gop_max_abs(gs,vals);
    break;
  case 'e':
    gs_gop_exists(gs,vals);
    break;
  case 'm':
    gs_gop_min(gs,vals);
    break;
  case 'M':
    gs_gop_max(gs,vals); break;
    /*
    if (*(op+1)=='\0')
      {gs_gop_max(gs,vals); break;}
    else if (*(op+1)=='X')
      {gs_gop_max_abs(gs,vals); break;}
    else if (*(op+1)=='N')
      {gs_gop_min_abs(gs,vals); break;}
    */
#endif
  default:
    error_msg_warning("gs_gop_hc() :: %c is not a valid op",op[0]);
    error_msg_warning("gs_gop_hc() :: default :: plus");
    gs_gop_plus_hc(gs,vals,dim);
    break;
  }
#ifdef DEBUG
  error_msg_warning("gs_gop_hc() end");
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static void
gs_gop_plus_hc(register gs_id *gs, register REAL *vals, int dim)
{
#ifdef DEBUG
  error_msg_warning("start gs_gop_hc()");
  if (!gs) {error_msg_fatal("gs_gop_hc() passed NULL gs handle!!!");}
#endif

  /* if there's nothing to do return */
  if (dim<=0)
    {return;}

  /* can't do more dimensions then exist */
  dim = MIN(dim,i_log2_num_nodes);

  /* local only operations!!! */
  if (gs->num_local)
    {gs_gop_local_plus(gs,vals);}

  /* if intersection tree/pairwise and local isn't empty */
  if (gs->num_local_gop)
    {
      gs_gop_local_in_plus(gs,vals);

      /* pairwise will do tree inside ... */
      if (gs->num_pairs)
  {gs_gop_pairwise_plus_hc(gs,vals,dim);}

      /* tree only */
      else if (gs->max_left_over)
  {gs_gop_tree_plus_hc(gs,vals,dim);}

      gs_gop_local_out(gs,vals);
    }
  /* if intersection tree/pairwise and local is empty */
  else
    {
      /* pairwise will do tree inside */
      if (gs->num_pairs)
  {gs_gop_pairwise_plus_hc(gs,vals,dim);}

      /* tree */
      else if (gs->max_left_over)
  {gs_gop_tree_plus_hc(gs,vals,dim);}
    }

#ifdef DEBUG
  error_msg_warning("end gs_gop_hc()");
#endif
}


/******************************************************************************
VERSION 3 ::

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_pairwise_plus_hc(register gs_id *gs, register REAL *in_vals, int dim)
{
#if   defined NXSRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  register int *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  int i, mask=1;

  for (i=1; i<dim; i++)
    {mask<<=1; mask++;}


#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_hc() start");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  do
    {
      if ((my_id|mask)==(*list|mask))
  {
    *msg_ids_in++ = (int) irecv(MSGTAG1 + *list++,(char *)in1,*size*REAL_LEN);
    in1 += *size++;
  }
      else
  {list++; size++;}
    }
  while (*++msg_nodes);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  list = msg_list;
  msg_nodes=nodes;
  while (iptr = *msg_nodes++)
    {
      if ((my_id|mask)==(*list|mask))
  {
    dptr3 = dptr2;
    while (*iptr >= 0)
      {*dptr2++ = *(dptr1 + *iptr++);}
    *msg_ids_out++ = (int) isend(MSGTAG1+my_id,(char *)dptr3,
               *(msg_size++)*REAL_LEN,*list++,0);
  }
      else
  {msg_size++; list++;}
    }
  /* post the receives ... was here*/

  /* do the tree while we're waiting */
  if (gs->max_left_over)
    {gs_gop_tree_plus_hc(gs,in_vals,dim);}

  /* process the received data */
  list = msg_list;
  msg_nodes=nodes;
  while (iptr = *msg_nodes++)
    {
      if ((my_id|mask)==(*list|mask))
  {
    msgwait(*ids_in++);
    while (*iptr >= 0)
      {*(dptr1 + *iptr++) += *in2++;}
  }
      list++;
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  while (iptr = *nodes++)
    {
      if ((my_id|mask)==(*msg_list|mask))
  {msgwait(*ids_out++);}
      msg_list++;
    }

#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_hc() end");
#endif

#elif defined MPISRC
  register REAL *dptr1, *dptr2, *dptr3, *in1, *in2;
  register int *iptr, *msg_list, *msg_size, **msg_nodes;
  register int *pw, *list, *size, **nodes;
  MPI_Request *msg_ids_in, *msg_ids_out, *ids_in, *ids_out;
  MPI_Status status;
  int i, mask=1;

  for (i=1; i<dim; i++)
    {mask<<=1; mask++;}


#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_hc() start");
#endif

  /* strip and load registers */
  msg_list =list         = gs->pair_list;
  msg_size =size         = gs->msg_sizes;
  msg_nodes=nodes        = gs->node_list;
  iptr=pw                = gs->pw_elm_list;
  dptr1=dptr3            = gs->pw_vals;
  msg_ids_in  = ids_in   = gs->msg_ids_in;
  msg_ids_out = ids_out  = gs->msg_ids_out;
  dptr2                  = gs->out;
  in1=in2                = gs->in;

  /* post the receives */
  /*  msg_nodes=nodes; */
  do
    {
      /* Should MPI_ANY_SOURCE be replaced by *list ? In that case do the
   second one *list and do list++ afterwards */
      if ((my_id|mask)==(*list|mask))
  {
    MPI_Irecv(in1, *size, REAL_TYPE, MPI_ANY_SOURCE, MSGTAG1 + *list++,
        MPI_COMM_WORLD, msg_ids_in++);
    in1 += *size++;
  }
      else
  {list++; size++;}
    }
  while (*++msg_nodes);

  /* load gs values into in out gs buffers */
  while (*iptr >= 0)
    {*dptr3++ = *(in_vals + *iptr++);}

  /* load out buffers and post the sends */
  msg_nodes=nodes;
  list = msg_list;
  while (iptr = *msg_nodes++)
    {
      if ((my_id|mask)==(*list|mask))
  {
    dptr3 = dptr2;
    while (*iptr >= 0)
      {*dptr2++ = *(dptr1 + *iptr++);}
    /* CHECK PERSISTENT COMMS MODE FOR ALL THIS STUFF */
    /* is msg_ids_out++ correct? */
    MPI_Isend(dptr3, *msg_size++, REAL_TYPE, *list++,
        MSGTAG1+my_id, MPI_COMM_WORLD, msg_ids_out++);
  }
      else
  {list++; msg_size++;}
    }

  /* do the tree while we're waiting */
  if (gs->max_left_over)
    {gs_gop_tree_plus_hc(gs,in_vals,dim);}

  /* process the received data */
  msg_nodes=nodes;
  list = msg_list;
  while (iptr = *nodes++)
    {
      if ((my_id|mask)==(*list|mask))
  {
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    MPI_Wait(ids_in++, &status);
    while (*iptr >= 0)
      {*(dptr1 + *iptr++) += *in2++;}
  }
      list++;
    }

  /* replace vals */
  while (*pw >= 0)
    {*(in_vals + *pw++) = *dptr1++;}

  /* clear isend message handles */
  /* This changed for clarity though it could be the same */
  while (*msg_nodes++)
    {
      if ((my_id|mask)==(*msg_list|mask))
  {
    /* Should I check the return value of MPI_Wait() or status? */
    /* Can this loop be replaced by a call to MPI_Waitall()? */
    MPI_Wait(ids_out++, &status);
  }
      msg_list++;
    }

#ifdef DEBUG
  error_msg_warning("gs_gop_pairwise_hc() end");
#endif

#else
  return;
#endif
}



/******************************************************************************
Function: gather_scatter

Input :
Output:
Return:
Description:
******************************************************************************/
static
void
gs_gop_tree_plus_hc(gs_id *gs, REAL *vals, int dim)
{
  int size;
  int *in, *out;
  REAL *buf, *work;
  int op[] = {GL_ADD,0};

#ifdef DEBUG
  error_msg_warning("start gs_gop_tree_plus_hc()");
#endif

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;
  buf  = gs->tree_buf;
  work = gs->tree_work;
  size = gs->tree_nel;

#if   BLAS&&r8
  *work = 0.0;
  dcopy(size,work,0,buf,1);
#elif BLAS
  *work = 0.0;
  scopy(size,work,0,buf,1);
#else
  rvec_zero(buf,size);
#endif

  while (*in >= 0)
    {*(buf + *out++) = *(vals + *in++);}

  in   = gs->tree_map_in;
  out  = gs->tree_map_out;

  grop_hc(buf,work,size,op,dim);

  while (*in >= 0)
    {*(vals + *in++) = *(buf + *out++);}

#ifdef DEBUG
  error_msg_warning("end gs_gop_tree_plus_hc()");
#endif
}
