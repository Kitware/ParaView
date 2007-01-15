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

/*
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *             Thursday, September 18, 1997
 *
 * Purpose:     This file contains declarations which are visible
 *              only within the H5G package. Source files outside the
 *              H5G package should include H5Gprivate.h instead.
 */
#ifndef H5G_PACKAGE
#error "Do not include this file outside the H5G package!"
#endif

#ifndef _H5Gpkg_H
#define _H5Gpkg_H

/* Get package's private header */
#include "H5Gprivate.h"

/* Other private headers needed by this file */
#include "H5ACprivate.h"  /* Metadata cache        */
#include "H5Oprivate.h"    /* Object headers        */

/*
 * A symbol table node is a collection of symbol table entries.  It can
 * be thought of as the lowest level of the B-link tree that points to
 * a collection of symbol table entries that belong to a specific symbol
 * table or group.
 */
typedef struct H5G_node_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    unsigned nsyms;                     /*number of symbols                  */
    H5G_entry_t *entry;                 /*array of symbol table entries      */
} H5G_node_t;

/*
 * Shared information for all open group objects
 */
struct H5G_shared_t {
    int fo_count;                   /* open file object count */
    hbool_t mounted;                /* Group is mount point */
};

/*
 * A group handle passed around through layers of the library within and
 * above the H5G layer.
 */
struct H5G_t {
    H5G_shared_t* shared;               /*shared file object data */
    H5G_entry_t ent;                    /*info about the group    */
};

/*
 * Common data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points.
 */
typedef struct H5G_bt_ud_common_t {
    /* downward */
    const char  *name;                  /*points to temporary memory         */
    haddr_t     heap_addr;              /*symbol table heap address          */
} H5G_bt_ud_common_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points for operations which require no
 * additional information.
 *
 * (Just an alias for the "common" info).
 */
typedef H5G_bt_ud_common_t H5G_bt_ud0_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points.
 */
typedef struct H5G_bt_ud1_t {
    /* downward */
    H5G_bt_ud_common_t common;          /* Common info for B-tree user data (must be first) */

    /* downward for INSERT, upward for FIND */
    H5G_entry_t *ent;                   /*entry to insert into table         */
} H5G_bt_ud1_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points.
 */
typedef struct H5G_bt_ud2_t {
    /* downward */
    H5G_bt_ud_common_t common;          /* Common info for B-tree user data (must be first) */
    hbool_t adj_link;                   /* Whether to adjust the link count on objects */

    /* upward */
} H5G_bt_ud2_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_it_ud1_t {
    /* downward */
    hid_t  group_id;  /*group id to pass to iteration operator     */
    haddr_t     heap_addr;      /*symbol table heap address                  */
    H5G_iterate_t op;    /*iteration operator           */
    void  *op_data;  /*user-defined operator data         */
    int    skip;    /*initial entries to skip         */

    /* upward */
    int    final_ent;  /*final entry looked at                      */
} H5G_bt_it_ud1_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_it_ud2_t {
    /* downward */
    haddr_t     heap_addr;      /*symbol table heap address                  */
    hsize_t     idx;            /*index of group member to be queried        */
    hsize_t     num_objs;       /*the number of objects having been traversed*/

    /* upward */
    char         *name;         /*member name to be returned                 */
} H5G_bt_it_ud2_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_it_ud3_t {
    /* downward */
    hsize_t     idx;            /*index of group member to be queried        */
    hsize_t     num_objs;       /*the number of objects having been traversed*/

    /* upward */
#ifdef H5_WANT_H5_V1_4_COMPAT
    int    type;                /*member type to be returned                 */
#else /* H5_WANT_H5_V1_4_COMPAT */
    H5G_obj_t    type;          /*member type to be returned                 */
#endif /* H5_WANT_H5_V1_4_COMPAT */
} H5G_bt_it_ud3_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_it_ud4_t {
    /* downward */
    haddr_t     heap_addr;      /*symbol table heap address                  */
    H5G_entry_t *grp_ent;       /*entry of group                             */

    /* upward */
} H5G_bt_it_ud4_t;

/*
 * This is the class identifier to give to the B-tree functions.
 */
H5_DLLVAR H5B_class_t H5B_SNODE[1];

/* The cache subclass */
H5_DLLVAR const H5AC_class_t H5AC_SNODE[1];

/*
 * Functions that understand symbol tables but not names.  The
 * functions that understand names are exported to the rest of
 * the library and appear in H5Gprivate.h.
 */
H5_DLL herr_t H5G_stab_create(H5F_t *f, hid_t dxpl_id, size_t size_hint,
             H5G_entry_t *ent/*out*/);
H5_DLL herr_t H5G_stab_find(H5G_entry_t *grp_ent, const char *name,
           H5G_entry_t *obj_ent/*out*/, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_insert(H5G_entry_t *grp_ent, const char *name,
             H5G_entry_t *obj_ent, hbool_t inc_link, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_delete(H5F_t *f, hid_t dxpl_id, const H5O_stab_t *stab, hbool_t adj_link);
H5_DLL herr_t H5G_stab_remove(H5G_entry_t *grp_ent, const char *name, hid_t dxpl_id);

/*
 * Functions that understand symbol table entries.
 */
H5_DLL herr_t H5G_ent_decode_vec(H5F_t *f, const uint8_t **pp,
          H5G_entry_t *ent, unsigned n);
H5_DLL herr_t H5G_ent_encode_vec(H5F_t *f, uint8_t **pp,
          const H5G_entry_t *ent, unsigned n);
H5_DLL herr_t H5G_ent_set_name(H5G_entry_t *loc, H5G_entry_t *obj, const char *name);

/* Functions that understand symbol table nodes */
H5_DLL int H5G_node_iterate (H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
         const void *_rt_key, void *_udata);
H5_DLL int H5G_node_sumup(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
         const void *_rt_key, void *_udata);
H5_DLL int H5G_node_name(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
         const void *_rt_key, void *_udata);
H5_DLL int H5G_node_type(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
         const void *_rt_key, void *_udata);
#endif
