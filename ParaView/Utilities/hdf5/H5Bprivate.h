/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Bprivate.h
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Private non-prototype header.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Bprivate_H
#define _H5Bprivate_H

#include "H5Bpublic.h"          /*API prototypes                             */

/* Private headers needed by this file */
#include "H5private.h"
#include "H5Fprivate.h"
#include "H5ACprivate.h"        /*cache                                      */

/*
 * Feature: Define this constant if you want to check B-tree consistency
 *          after each B-tree operation.  Note that this slows down the
 *          library considerably! Debugging the B-tree depends on assert()
 *          being enabled.
 */
#ifdef NDEBUG
#  undef H5B_DEBUG
#endif
#define H5B_MAGIC       "TREE"          /*tree node magic number             */
#define H5B_SIZEOF_MAGIC 4              /*size of magic number               */
#define H5B_SIZEOF_HDR(F)                                                     \
   (H5B_SIZEOF_MAGIC +          /*magic number                            */  \
    4 +                         /*type, level, num entries                */  \
    2*H5F_SIZEOF_ADDR(F))       /*left and right sibling addresses        */
     
#define H5B_K(F,TYPE)           /*K value given file and Btree subclass   */  \
   ((F)->shared->fcpl->btree_k[(TYPE)->id])

typedef enum H5B_ins_t {
    H5B_INS_ERROR        = -1,  /*error return value                         */
    H5B_INS_NOOP         = 0,   /*insert made no changes                     */
    H5B_INS_LEFT         = 1,   /*insert new node to left of cur node        */
    H5B_INS_RIGHT        = 2,   /*insert new node to right of cur node       */
    H5B_INS_CHANGE       = 3,   /*change child address for cur node          */
    H5B_INS_FIRST        = 4,   /*insert first node in (sub)tree             */
    H5B_INS_REMOVE       = 5    /*remove current node                        */
} H5B_ins_t;

typedef enum H5B_subid_t {
    H5B_SNODE_ID         = 0,   /*B-tree is for symbol table nodes           */
    H5B_ISTORE_ID        = 1    /*B-tree is for indexed object storage       */
} H5B_subid_t;

/*
 * Each class of object that can be pointed to by a B-link tree has a
 * variable of this type that contains class variables and methods.  Each
 * tree has a K (1/2 rank) value on a per-file basis.  The file_create_parms
 * has an array of K values indexed by the `id' class field below.  The
 * array is initialized with the HDF5_BTREE_K_DEFAULT macro.
 */
struct H5B_t;                           /*forward decl                       */

typedef struct H5B_class_t {
    H5B_subid_t id;                                     /*id as found in file*/
    size_t      sizeof_nkey;                    /*size of native (memory) key*/
    size_t      (*get_sizeof_rkey)(H5F_t*, const void*);    /*raw key size   */
    herr_t      (*new_node)(H5F_t*, H5B_ins_t, void*, void*, void*, haddr_t*);
    int (*cmp2)(H5F_t*, void*, void*, void*);       /*compare 2 keys */
    int (*cmp3)(H5F_t*, void*, void*, void*);       /*compare 3 keys */
    herr_t      (*found)(H5F_t*, haddr_t, const void*, void*, const void*);
    
    /* insert new data */
    H5B_ins_t   (*insert)(H5F_t*, haddr_t, void*, hbool_t*, void*, void*,
                          void*, hbool_t*, haddr_t*);

    /* min insert uses min leaf, not new(), similarily for max insert */
    hbool_t     follow_min;
    hbool_t     follow_max;
    
    /* remove existing data */
    H5B_ins_t   (*remove)(H5F_t*, haddr_t, void*, hbool_t*, void*, void*,
                          hbool_t*);

    /* iterate through the leaf nodes */
    herr_t      (*list)(H5F_t*, void*, haddr_t, void*, void*);

    /* encode, decode, debug key values */
    herr_t      (*decode)(H5F_t*, struct H5B_t*, uint8_t*, void*);
    herr_t      (*encode)(H5F_t*, struct H5B_t*, uint8_t*, void*);
    herr_t      (*debug_key)(FILE*, int, int, const void*, const void*);
} H5B_class_t;

/*
 * The B-tree node as stored in memory...
 */
typedef struct H5B_key_t {
    hbool_t     dirty;  /*native key is more recent than raw key             */
    uint8_t     *rkey;  /*ptr into node->page for raw key                    */
    void        *nkey;  /*null or ptr into node->native for key              */
} H5B_key_t;

typedef struct H5B_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    const H5B_class_t   *type;          /*type of tree                       */
    size_t              sizeof_rkey;    /*size of raw (disk) key             */
    hbool_t             dirty;          /*something in the tree is dirty     */
    int         ndirty;         /*num child ptrs to emit             */
    int         level;          /*node level                         */
    haddr_t             left;           /*address of left sibling            */
    haddr_t             right;          /*address of right sibling           */
    int         nchildren;      /*number of child pointers           */
    uint8_t             *page;          /*disk page                          */
    uint8_t             *native;        /*array of keys in native format     */
    H5B_key_t           *key;           /*2k+1 key entries                   */
    haddr_t             *child;         /*2k child pointers                  */
} H5B_t;

/*
 * Library prototypes.
 */
__DLL__ herr_t H5B_debug (H5F_t *f, haddr_t addr, FILE * stream,
                          int indent, int fwidth, const H5B_class_t *type,
                          void *udata);
__DLL__ herr_t H5B_create (H5F_t *f, const H5B_class_t *type, void *udata,
                           haddr_t *addr_p/*out*/);
__DLL__ herr_t H5B_find (H5F_t *f, const H5B_class_t *type, haddr_t addr,
                         void *udata);
__DLL__ herr_t H5B_insert (H5F_t *f, const H5B_class_t *type, haddr_t addr,
                           const double split_ratios[], void *udata);
__DLL__ herr_t H5B_remove(H5F_t *f, const H5B_class_t *type, haddr_t addr,
                          void *udata);
__DLL__ herr_t H5B_iterate (H5F_t *f, const H5B_class_t *type, haddr_t addr,
                            void *udata);
#endif
