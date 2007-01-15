/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:    H5FLprivate.h
 *      Mar 23 2000
 *      Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:    Private non-prototype header.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5FLprivate_H
#define _H5FLprivate_H

/* Public headers needed by this file */
#ifdef LATER
#include "H5FLpublic.h"    /*API prototypes           */
#endif /* LATER */

/* Private headers needed by this file */

/* Macros for turning off free lists in the library */
/* #define H5_NO_FREE_LISTS */
#if defined H5_NO_FREE_LISTS || defined H5_USING_PURIFY
#define H5_NO_REG_FREE_LISTS
#define H5_NO_ARR_FREE_LISTS
#define H5_NO_SEQ_FREE_LISTS
#define H5_NO_BLK_FREE_LISTS
#endif /* H5_NO_FREE_LISTS */

/*
 * Private datatypes.
 */

/* Data structure to store each block in free list */
typedef struct H5FL_reg_node_t {
    struct H5FL_reg_node_t *next;   /* Pointer to next block in free list */
} H5FL_reg_node_t;

/* Data structure for free list of blocks */
typedef struct H5FL_reg_head_t {
    unsigned init;         /* Whether the free list has been initialized */
    unsigned allocated;    /* Number of blocks allocated */
    unsigned onlist;       /* Number of blocks on free list */
    size_t list_mem;    /* Amount of memory on free list */
    const char *name;   /* Name of the type */
    size_t size;        /* Size of the blocks in the list */
    H5FL_reg_node_t *list;  /* List of free blocks */
} H5FL_reg_head_t;

/*
 * Macros for defining & using free lists for a type
 */
#define H5FL_REG_NAME(t)        H5_##t##_reg_free_list
#ifndef H5_NO_REG_FREE_LISTS
/* Common macros for H5FL_DEFINE & H5FL_DEFINE_STATIC */
#define H5FL_DEFINE_COMMON(t) H5FL_reg_head_t H5FL_REG_NAME(t)={0,0,0,0,#t,sizeof(t),NULL}

/* Declare a free list to manage objects of type 't' */
#define H5FL_DEFINE(t) H5_DLL H5FL_DEFINE_COMMON(t)

/* Reference a free list for type 't' defined in another file */
#define H5FL_EXTERN(t)  extern H5_DLL H5FL_reg_head_t H5FL_REG_NAME(t)

/* Declare a static free list to manage objects of type 't' */
#define H5FL_DEFINE_STATIC(t)  static H5FL_DEFINE_COMMON(t)

/* Allocate an object of type 't' */
#define H5FL_MALLOC(t) H5FL_reg_malloc(&(H5FL_REG_NAME(t)))

/* Allocate an object of type 't' and clear it to all zeros */
#define H5FL_CALLOC(t) H5FL_reg_calloc(&(H5FL_REG_NAME(t)))

/* Free an object of type 't' */
#define H5FL_FREE(t,obj) H5FL_reg_free(&(H5FL_REG_NAME(t)),obj)

/* Re-allocating an object of type 't' is not defined, because these free-lists
 * only support fixed sized types, like structs, etc..
 */

#else /* H5_NO_REG_FREE_LISTS */
#include "H5MMprivate.h"
/* Common macro for H5FL_DEFINE & H5FL_DEFINE_STATIC */
#define H5FL_DEFINE_COMMON(t) int H5FL_REG_NAME(t)

#define H5FL_DEFINE(t)  H5_DLL H5FL_DEFINE_COMMON(t)
#define H5FL_EXTERN(t)  extern H5_DLL int H5FL_REG_NAME(t)
#define H5FL_DEFINE_STATIC(t)  static H5FL_DEFINE_COMMON(t)
#define H5FL_MALLOC(t) H5MM_malloc(sizeof(t))
#define H5FL_CALLOC(t) H5MM_calloc(sizeof(t))
#define H5FL_FREE(t,obj) H5MM_xfree(obj)
#endif /* H5_NO_REG_FREE_LISTS */

/* Data structure to store information about each block allocated */
typedef union H5FL_blk_list_t {
    size_t size;                /* Size of the page */
    union H5FL_blk_list_t *next;   /* Pointer to next block in free list */
    double unused1;         /* Unused normally, just here for aligment */
    haddr_t unused2;        /* Unused normally, just here for aligment */
} H5FL_blk_list_t;

/* Data structure for priority queue node of block free lists */
typedef struct H5FL_blk_node_t {
    size_t size;                /* Size of the blocks in the list */
    H5FL_blk_list_t *list;      /* List of free blocks */
    struct H5FL_blk_node_t *next;    /* Pointer to next free list in queue */
    struct H5FL_blk_node_t *prev;    /* Pointer to previous free list in queue */
} H5FL_blk_node_t;

/* Data structure for priority queue of native block free lists */
typedef struct H5FL_blk_head_t {
    unsigned init;         /* Whether the free list has been initialized */
    unsigned allocated;    /* Number of blocks allocated */
    unsigned onlist;       /* Number of blocks on free list */
    size_t list_mem;    /* Amount of memory in block on free list */
    const char *name;   /* Name of the type */
    H5FL_blk_node_t *head;  /* Pointer to first free list in queue */
} H5FL_blk_head_t;

/*
 * Macros for defining & using priority queues
 */
#define H5FL_BLK_NAME(t)        H5_##t##_blk_free_list
#ifndef H5_NO_BLK_FREE_LISTS
/* Common macro for H5FL_BLK_DEFINE & H5FL_BLK_DEFINE_STATIC */
#define H5FL_BLK_DEFINE_COMMON(t) H5FL_blk_head_t H5FL_BLK_NAME(t)={0,0,0,0,#t"_blk",NULL}

/* Declare a free list to manage objects of type 't' */
#define H5FL_BLK_DEFINE(t)  H5_DLL H5FL_BLK_DEFINE_COMMON(t)

/* Reference a free list for type 't' defined in another file */
#define H5FL_BLK_EXTERN(t)  extern H5_DLL H5FL_blk_head_t H5FL_BLK_NAME(t)

/* Declare a static free list to manage objects of type 't' */
#define H5FL_BLK_DEFINE_STATIC(t)  static H5FL_BLK_DEFINE_COMMON(t)

/* Allocate an block of type 't' */
#define H5FL_BLK_MALLOC(t,size) H5FL_blk_malloc(&(H5FL_BLK_NAME(t)),size)

/* Allocate an block of type 't' and clear it to zeros */
#define H5FL_BLK_CALLOC(t,size) H5FL_blk_calloc(&(H5FL_BLK_NAME(t)),size)

/* Free a block of type 't' */
#define H5FL_BLK_FREE(t,blk) H5FL_blk_free(&(H5FL_BLK_NAME(t)),blk)

/* Re-allocate a block of type 't' */
#define H5FL_BLK_REALLOC(t,blk,new_size) H5FL_blk_realloc(&(H5FL_BLK_NAME(t)),blk,new_size)

/* Check if there is a free block available to re-use */
#define H5FL_BLK_AVAIL(t,size)  H5FL_blk_free_block_avail(&(H5FL_BLK_NAME(t)),size)

#else /* H5_NO_BLK_FREE_LISTS */
/* Common macro for H5FL_BLK_DEFINE & H5FL_BLK_DEFINE_STATIC */
#define H5FL_BLK_DEFINE_COMMON(t) int H5FL_BLK_NAME(t)

#define H5FL_BLK_DEFINE(t)      H5_DLL H5FL_BLK_DEFINE_COMMON(t)
#define H5FL_BLK_EXTERN(t)      extern H5_DLL int H5FL_BLK_NAME(t)
#define H5FL_BLK_DEFINE_STATIC(t)  static H5FL_BLK_DEFINE_COMMON(t)
#define H5FL_BLK_MALLOC(t,size) H5MM_malloc(size)
#define H5FL_BLK_CALLOC(t,size) H5MM_calloc(size)
#define H5FL_BLK_FREE(t,blk) H5MM_xfree(blk)
#define H5FL_BLK_REALLOC(t,blk,new_size) H5MM_realloc(blk,new_size)
#define H5FL_BLK_AVAIL(t,size)  (FALSE)
#endif /* H5_NO_BLK_FREE_LISTS */

/* Data structure to store each array in free list */
typedef union H5FL_arr_list_t {
    union H5FL_arr_list_t *next;   /* Pointer to next block in free list */
    size_t nelem;               /* Number of elements in this array */
    double unused1;             /* Unused normally, just here for aligment */
    haddr_t unused2;            /* Unused normally, just here for aligment */
} H5FL_arr_list_t;

/* Data structure for each size of array element */
typedef struct H5FL_arr_node_t {
    size_t size;                /* Size of the blocks in the list */
    unsigned onlist;            /* Number of blocks on free list */
    H5FL_arr_list_t *list;      /* List of free blocks */
} H5FL_arr_node_t;

/* Data structure for free list of array blocks */
typedef struct H5FL_arr_head_t {
    unsigned init;         /* Whether the free list has been initialized */
    unsigned allocated;    /* Number of blocks allocated */
    size_t list_mem;       /* Amount of memory in block on free list */
    const char *name;      /* Name of the type */
    int  maxelem;          /* Maximum number of elements in an array */
    size_t base_size;      /* Size of the "base" object in the list */
    size_t elem_size;      /* Size of the array elements in the list */
    H5FL_arr_node_t *list_arr;  /* Array of lists of free blocks */
} H5FL_arr_head_t;

/*
 * Macros for defining & using free lists for an array of a type
 */
#define H5FL_ARR_NAME(t)        H5_##t##_arr_free_list
#ifndef H5_NO_ARR_FREE_LISTS
/* Common macro for H5FL_BLK_DEFINE & H5FL_BLK_DEFINE_STATIC */
#define H5FL_ARR_DEFINE_COMMON(b,t,m) H5FL_arr_head_t H5FL_ARR_NAME(t)={0,0,0,#t"_arr",m+1,b,sizeof(t),NULL}

/* Declare a free list to manage arrays of type 't' */
#define H5FL_ARR_DEFINE(t,m)  H5_DLL H5FL_ARR_DEFINE_COMMON(0,t,m)

/* Declare a free list to manage base 'b' + arrays of type 't' */
#define H5FL_BARR_DEFINE(b,t,m)  H5_DLL H5FL_ARR_DEFINE_COMMON(sizeof(b),t,m)

/* Reference a free list for arrays of type 't' defined in another file */
#define H5FL_ARR_EXTERN(t)  extern H5_DLL H5FL_arr_head_t H5FL_ARR_NAME(t)

/* Declare a static free list to manage arrays of type 't' */
#define H5FL_ARR_DEFINE_STATIC(t,m)  static H5FL_ARR_DEFINE_COMMON(0,t,m)

/* Declare a static free list to manage base 'b' + arrays of type 't' */
#define H5FL_BARR_DEFINE_STATIC(b,t,m)  static H5FL_ARR_DEFINE_COMMON(sizeof(b),t,m)

/* Allocate an array of type 't' */
#define H5FL_ARR_MALLOC(t,elem) H5FL_arr_malloc(&(H5FL_ARR_NAME(t)),elem)

/* Allocate an array of type 't' and clear it to all zeros */
#define H5FL_ARR_CALLOC(t,elem) H5FL_arr_calloc(&(H5FL_ARR_NAME(t)),elem)

/* Free an array of type 't' */
#define H5FL_ARR_FREE(t,obj) H5FL_arr_free(&(H5FL_ARR_NAME(t)),obj)

/* Re-allocate an array of type 't' */
#define H5FL_ARR_REALLOC(t,obj,new_elem) H5FL_arr_realloc(&(H5FL_ARR_NAME(t)),obj,new_elem)

#else /* H5_NO_ARR_FREE_LISTS */
/* Common macro for H5FL_ARR_DEFINE & H5FL_ARR_DEFINE_STATIC */
#define H5FL_ARR_DEFINE_COMMON(t,m) size_t H5FL_ARR_NAME(t)

#define H5FL_ARR_DEFINE(t,m)    H5_DLL H5FL_ARR_DEFINE_COMMON(t,m) = 0
#define H5FL_BARR_DEFINE(b,t,m) H5_DLL H5FL_ARR_DEFINE_COMMON(t,m) = sizeof(b)
#define H5FL_ARR_EXTERN(t)      extern H5_DLL H5FL_ARR_DEFINE_COMMON(t,m)
#define H5FL_ARR_DEFINE_STATIC(t,m)  static H5FL_ARR_DEFINE_COMMON(t,m) = 0
#define H5FL_BARR_DEFINE_STATIC(b,t,m)  static H5FL_ARR_DEFINE_COMMON(t,m) = sizeof(b)
#define H5FL_ARR_MALLOC(t,elem) H5MM_malloc(H5FL_ARR_NAME(t) + ((elem)*sizeof(t)))
#define H5FL_ARR_CALLOC(t,elem) H5MM_calloc(H5FL_ARR_NAME(t) + ((elem)*sizeof(t)))
#define H5FL_ARR_FREE(t,obj) H5MM_xfree(obj)
#define H5FL_ARR_REALLOC(t,obj,new_elem) H5MM_realloc(obj,H5FL_ARR_NAME(t) + ((new_elem)*sizeof(t)))
#endif /* H5_NO_ARR_FREE_LISTS */

/* Data structure for free list of sequence blocks */
typedef struct H5FL_seq_head_t {
    H5FL_blk_head_t queue;      /* Priority queue of sequence blocks */
    size_t size;                /* Size of the sequence elements in the list */
} H5FL_seq_head_t;

/*
 * Macros for defining & using free lists for a sequence of a type
 *
 * Sequences are like arrays, except they have no upper limit.
 *
 */
#define H5FL_SEQ_NAME(t)        H5_##t##_seq_free_list
#ifndef H5_NO_SEQ_FREE_LISTS
/* Common macro for H5FL_SEQ_DEFINE & H5FL_SEQ_DEFINE_STATIC */
#define H5FL_SEQ_DEFINE_COMMON(t) H5FL_seq_head_t H5FL_SEQ_NAME(t)={{0,0,0,0,#t"_seq",NULL},sizeof(t)}

/* Declare a free list to manage sequences of type 't' */
#define H5FL_SEQ_DEFINE(t)  H5_DLL H5FL_SEQ_DEFINE_COMMON(t)

/* Reference a free list for sequences of type 't' defined in another file */
#define H5FL_SEQ_EXTERN(t)  extern H5_DLL H5FL_seq_head_t H5FL_SEQ_NAME(t)

/* Declare a static free list to manage sequences of type 't' */
#define H5FL_SEQ_DEFINE_STATIC(t)  static H5FL_SEQ_DEFINE_COMMON(t)

/* Allocate a sequence of type 't' */
#define H5FL_SEQ_MALLOC(t,elem) H5FL_seq_malloc(&(H5FL_SEQ_NAME(t)),elem)

/* Allocate a sequence of type 't' and clear it to all zeros */
#define H5FL_SEQ_CALLOC(t,elem) H5FL_seq_calloc(&(H5FL_SEQ_NAME(t)),elem)

/* Free a sequence of type 't' */
#define H5FL_SEQ_FREE(t,obj) H5FL_seq_free(&(H5FL_SEQ_NAME(t)),obj)

/* Re-allocate a sequence of type 't' */
#define H5FL_SEQ_REALLOC(t,obj,new_elem) H5FL_seq_realloc(&(H5FL_SEQ_NAME(t)),obj,new_elem)

#else /* H5_NO_SEQ_FREE_LISTS */
/* Common macro for H5FL_BLK_DEFINE & H5FL_BLK_DEFINE_STATIC */
#define H5FL_SEQ_DEFINE_COMMON(t) int H5FL_SEQ_NAME(t)

#define H5FL_SEQ_DEFINE(t)      H5_DLL H5FL_SEQ_DEFINE_COMMON(t)
#define H5FL_SEQ_EXTERN(t)      extern H5_DLL int H5FL_SEQ_NAME(t)
#define H5FL_SEQ_DEFINE_STATIC(t)  static H5FL_SEQ_DEFINE_COMMON(t)
#define H5FL_SEQ_MALLOC(t,elem) H5MM_malloc((elem)*sizeof(t))
#define H5FL_SEQ_CALLOC(t,elem) H5MM_calloc((elem)*sizeof(t))
#define H5FL_SEQ_FREE(t,obj) H5MM_xfree(obj)
#define H5FL_SEQ_REALLOC(t,obj,new_elem) H5MM_realloc(obj,(new_elem)*sizeof(t))
#endif /* H5_NO_SEQ_FREE_LISTS */

/*
 * Library prototypes.
 */
H5_DLL void * H5FL_blk_malloc(H5FL_blk_head_t *head, size_t size);
H5_DLL void * H5FL_blk_calloc(H5FL_blk_head_t *head, size_t size);
H5_DLL void * H5FL_blk_free(H5FL_blk_head_t *head, void *block);
H5_DLL void * H5FL_blk_realloc(H5FL_blk_head_t *head, void *block, size_t new_size);
H5_DLL htri_t H5FL_blk_free_block_avail(H5FL_blk_head_t *head, size_t size);
H5_DLL void * H5FL_reg_malloc(H5FL_reg_head_t *head);
H5_DLL void * H5FL_reg_calloc(H5FL_reg_head_t *head);
H5_DLL void * H5FL_reg_free(H5FL_reg_head_t *head, void *obj);
H5_DLL void * H5FL_arr_malloc(H5FL_arr_head_t *head, size_t elem);
H5_DLL void * H5FL_arr_calloc(H5FL_arr_head_t *head, size_t elem);
H5_DLL void * H5FL_arr_free(H5FL_arr_head_t *head, void *obj);
H5_DLL void * H5FL_arr_realloc(H5FL_arr_head_t *head, void *obj, size_t new_elem);
H5_DLL void * H5FL_seq_malloc(H5FL_seq_head_t *head, size_t elem);
H5_DLL void * H5FL_seq_calloc(H5FL_seq_head_t *head, size_t elem);
H5_DLL void * H5FL_seq_free(H5FL_seq_head_t *head, void *obj);
H5_DLL void * H5FL_seq_realloc(H5FL_seq_head_t *head, void *obj, size_t new_elem);
H5_DLL herr_t H5FL_garbage_coll(void);
H5_DLL herr_t H5FL_set_free_list_limits(int reg_global_lim, int reg_list_lim,
    int arr_global_lim, int arr_list_lim, int blk_global_lim, int blk_list_lim);
H5_DLL int   H5FL_term_interface(void);

#endif
