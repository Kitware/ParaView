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
#include "H5ACprivate.h"	/* Metadata cache			  */

/*
 * A symbol table node is a collection of symbol table entries.  It can
 * be thought of as the lowest level of the B-link tree that points to
 * a collection of symbol table entries that belong to a specific symbol
 * table or group.
 */
typedef struct H5G_node_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    int         nsyms;                  /*number of symbols                  */
    H5G_entry_t *entry;                 /*array of symbol table entries      */
} H5G_node_t;

/*
 * A group handle passed around through layers of the library within and
 * above the H5G layer.
 */
struct H5G_t {
    int         nref;                   /*open reference count               */
    H5G_entry_t ent;                    /*info about the group               */
};

/*
 * These operations can be passed down from the H5G_stab layer to the
 * H5G_node layer through the B-tree layer.
 */
typedef enum H5G_oper_t {
    H5G_OPER_FIND       = 0,   	/*find a symbol                              */
    H5G_OPER_INSERT     = 1,	/*insert a new symbol                        */
    H5G_OPER_REMOVE	= 2	/*remove existing symbol		     */
} H5G_oper_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points.
 */
typedef struct H5G_bt_ud1_t {

    /* downward */
    H5G_oper_t  operation;              /*what operation to perform          */
    const char  *name;                  /*points to temporary memory         */
    haddr_t     heap_addr;              /*symbol table heap address          */

    /* downward for INSERT, upward for FIND */
    H5G_entry_t ent;                    /*entry to insert into table         */

} H5G_bt_ud1_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_ud2_t {
    /* downward */
    hid_t	group_id;	/*group id to pass to iteration operator     */
    H5G_entry_t *ent;           /*the entry to which group_id points         */
    int		skip;		/*initial entries to skip		     */
    H5G_iterate_t op;		/*iteration operator			     */
    void	*op_data;	/*user-defined operator data		     */

    /* upward */
    int		final_ent;	/*final entry looked at                      */
    
} H5G_bt_ud2_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_ud3_t {
    /* downward */
    H5G_entry_t *ent;           /*the entry of group being queried           */
    hsize_t      idx;           /*index of group member to be querried       */
    hsize_t      num_objs;      /*the number of objects having been traversed*/

    /* upward */
    char         *name;         /*member name to be returned                 */
    int          type;          /*member type to be returned                 */
} H5G_bt_ud3_t;

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
			       H5G_entry_t *obj_ent, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_delete(H5F_t *f, hid_t dxpl_id, haddr_t btree_addr, haddr_t heap_addr);
H5_DLL herr_t H5G_stab_remove(H5G_entry_t *grp_ent, const char *name, hid_t dxpl_id);

/*
 * Functions that understand symbol table entries.
 */
H5_DLL herr_t H5G_ent_decode_vec(H5F_t *f, const uint8_t **pp,
				  H5G_entry_t *ent, int n);
H5_DLL herr_t H5G_ent_encode_vec(H5F_t *f, uint8_t **pp,
				  const H5G_entry_t *ent, int n);

/* Functions that understand symbol table nodes */
H5_DLL int H5G_node_iterate (H5F_t *f, hid_t dxpl_id, void *_lt_key, haddr_t addr,
		     void *_rt_key, void *_udata);
H5_DLL int H5G_node_sumup(H5F_t *f, hid_t dxpl_id, void *_lt_key, haddr_t addr,
		     void *_rt_key, void *_udata);
H5_DLL int H5G_node_name(H5F_t *f, hid_t dxpl_id, void *_lt_key, haddr_t addr,
		     void *_rt_key, void *_udata);
H5_DLL int H5G_node_type(H5F_t *f, hid_t dxpl_id, void *_lt_key, haddr_t addr,
		     void *_rt_key, void *_udata);
#endif
