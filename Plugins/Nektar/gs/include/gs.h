/***********************************gs.h***************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
************************************gs.h**************************************/

/***********************************gs.h***************************************
File Description:
-----------------

************************************gs.h**************************************/
#ifndef _gs_h
#define _gs_h

/***********************************gs.h***************************************
Type: gs_ADT
------------

************************************gs.h**************************************/

typedef struct gather_scatter_id *gs_ADT;


/***********************************gs.h***************************************
Function:

Input :
Output:
Return:
Description:
Usage:
************************************gs.h**************************************/
extern gs_ADT gs_init(int *elms, int nel, int level);
extern void   gs_gop(gs_ADT gs_handle, REAL *vals, char *op);
extern void   gs_gop_vec(gs_ADT gs_handle, REAL *vals, char *op, int step);
extern void   gs_gop_binary(gs_ADT gs, REAL *vals, rbfp fct);
extern void   gs_gop_hc(gs_ADT gs_handle, REAL *vals, char *op, int dim);

/*
extern void   gs_gop_gen(gs_ADT gs, void *vals, void *id, DATA_TYPE *dt, bfp fct);
*/

extern void   gs_free(gs_ADT gs_handle);
/* extern void   gs_print_template(gs_ADT gs_handle, int who); */
extern void   gs_init_msg_buf_sz(int buf_size);
extern void   gs_init_vec_sz(int size);

#if defined UPCASE
extern int GS_INIT  (int *elms, int *nel, int *level);
#else
extern int gs_init_ (int *elms, int *nel, int *level);
#endif

#if defined UPCASE
extern void GS_GOP  (int *gs, REAL *vals, char *op);
#else
extern void gs_gop_ (int *gs, REAL *vals, char *op);
#endif

#if defined UPCASE
extern void GS_GOP_VEC  (int *gs, REAL *vals, char *op, int *step);
#else
extern void gs_gop_vec_ (int *gs, REAL *vals, char *op, int *step);
#endif

#if defined UPCASE
extern void GS_FREE  (int *gs);
#else
extern void gs_free_ (int *gs);
#endif


#if defined UPCASE
extern void GS_INIT_MSG_BUF_SZ (int *buf_size);
#else
extern void gs_init_msg_buf_sz_ (int *buf_size);
#endif

#if defined UPCASE
extern void GS_INIT_VEC_SZ  (int *size);
#else
extern void gs_init_vec_sz_ (int *size);
#endif



#endif
