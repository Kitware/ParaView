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
 * Created:             H5Gprivate.h
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Library-visible declarations.
 *
 * Modifications:       Aug 22, 2002
 *                      Pedro Vicente <pvn@ncsa.uiuc.edu>
 *                      Added 'names' field to H5G_entry_t
 *                      Added H5G_replace_name
 *
 *-------------------------------------------------------------------------
 */

#ifndef _H5Gprivate_H
#define _H5Gprivate_H

/* Include package's public header */
#include "H5Gpublic.h"

/* Private headers needed by this file */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Bprivate.h"		/* B-trees				*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5RSprivate.h"        /* Reference-counted strings            */

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5G_DEBUG
#endif

#define H5G_NODE_MAGIC  "SNOD"          /*symbol table node magic number     */
#define H5G_NODE_SIZEOF_MAGIC 4         /*sizeof symbol node magic number    */
#define H5G_NO_CHANGE   (-1)            /*see H5G_ent_modified()             */

/*
 * The disk size for a symbol table entry...
 */
#define H5G_SIZEOF_SCRATCH      16
#define H5G_SIZEOF_ENTRY(F)                                                   \
   (H5F_SIZEOF_SIZE(F) +        /*offset of name into heap              */    \
    H5F_SIZEOF_ADDR(F) +        /*address of object header              */    \
    4 +                         /*entry type                            */    \
    4 +				/*reserved				*/    \
    H5G_SIZEOF_SCRATCH)         /*scratch pad space                     */

/*
 * Various types of object header information can be cached in a symbol
 * table entry (it's normal home is the object header to which the entry
 * points).  This datatype determines what (if anything) is cached in the
 * symbol table entry.
 */
typedef enum H5G_type_t {
    H5G_CACHED_ERROR	= -1, 	/*force enum to be signed		     */
    H5G_NOTHING_CACHED  = 0,    /*nothing is cached, must be 0               */
    H5G_CACHED_STAB     = 1,    /*symbol table, `stab'                       */
    H5G_CACHED_SLINK	= 2, 	/*symbolic link				     */

    H5G_NCACHED         = 3     /*THIS MUST BE LAST                          */
} H5G_type_t;

/*
 * A symbol table entry caches these parameters from object header
 * messages...  The values are entered into the symbol table when an object
 * header is created (by hand) and are extracted from the symbol table with a
 * callback function registered in H5O_init_interface().  Be sure to update
 * H5G_ent_decode(), H5G_ent_encode(), and H5G_ent_debug() as well.
 */
typedef union H5G_cache_t {
    struct {
        haddr_t btree_addr;             /*file address of symbol table B-tree*/
        haddr_t heap_addr;              /*file address of stab name heap     */
    } stab;

    struct {
	size_t	lval_offset;		/*link value offset		     */
    } slink;
} H5G_cache_t;

/*
 * A symbol table entry.  The two important fields are `name_off' and
 * `header'.  The remaining fields are used for caching information that
 * also appears in the object header to which this symbol table entry
 * points.
 */
typedef struct H5G_entry_t {
    hbool_t     dirty;                  /*entry out-of-date?                 */
    size_t      name_off;               /*offset of name within name heap    */
    haddr_t     header;                 /*file address of object header      */
    H5G_type_t  type;                   /*type of information cached         */
    H5G_cache_t cache;                  /*cached data from object header     */
    H5F_t       *file;                  /*file to which this obj hdr belongs */
    H5RS_str_t  *user_path_r;           /* Path to object, as opened by user */
    H5RS_str_t  *canon_path_r;          /* Path to object, as found in file  */
    unsigned    user_path_hidden;       /* Whether the user's path is valid  */
} H5G_entry_t;

typedef struct H5G_t H5G_t;

/* Type of operation being performed for call to H5G_replace_name() */
typedef enum {
    OP_MOVE = 0,        /* H5*move call    */
    OP_UNLINK,          /* H5Gunlink call  */
    OP_MOUNT,           /* H5Fmount call   */
    OP_UNMOUNT          /* H5Funmount call */
} H5G_names_op_t;

/* Depth of group entry copy */
typedef enum {
    H5G_COPY_NULL,      /* Null destination names */
    H5G_COPY_LIMITED,   /* Limited copy from source to destination, omitting name & old name fields */
    H5G_COPY_SHALLOW,   /* Copy from source to destination, including name & old name fields */
    H5G_COPY_DEEP       /* Deep copy from source to destination, including duplicating name & old name fields */
} H5G_ent_copy_depth_t;

/*
 * Library prototypes...  These are the ones that other packages routinely
 * call.
 */
H5_DLL H5G_entry_t *H5G_loc(hid_t loc_id);
H5_DLL herr_t H5G_mkroot(H5F_t *f, hid_t dxpl_id, H5G_entry_t *root_entry);
H5_DLL H5G_entry_t *H5G_entof(H5G_t *grp);
H5_DLL H5F_t *H5G_fileof(H5G_t *grp);
H5_DLL H5G_t *H5G_open(H5G_entry_t *loc, const char *name, hid_t dxpl_id);
H5_DLL H5G_t *H5G_open_oid(H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL herr_t H5G_close(H5G_t *grp);
H5_DLL int H5G_get_type(H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL herr_t H5G_get_objinfo(H5G_entry_t *loc, const char *name,
			       hbool_t follow_link,
			       H5G_stat_t *statbuf/*out*/, hid_t dxpl_id);
H5_DLL herr_t H5G_insert(H5G_entry_t *loc, const char *name,
			  H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL herr_t H5G_find(H5G_entry_t *loc, const char *name,
			H5G_entry_t *grp_ent/*out*/, H5G_entry_t *ent/*out*/, hid_t dxpl_id);
H5_DLL H5F_t *H5G_insertion_file(H5G_entry_t *loc, const char *name, hid_t dxpl_id);
H5_DLL  herr_t H5G_replace_name(int type, H5G_entry_t *loc,
        H5RS_str_t *src_name, H5G_entry_t *src_loc,
        H5RS_str_t *dst_name, H5G_entry_t *dst_loc, H5G_names_op_t op);
H5_DLL  herr_t H5G_free_grp_name(H5G_t *grp);

/*
 * These functions operate on symbol table nodes.
 */
H5_DLL herr_t H5G_node_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream,
			      int indent, int fwidth, haddr_t heap);

/*
 * These functions operate on symbol table entries.  They're used primarily
 * in the H5O package where header messages are cached in symbol table
 * entries.  The subclasses of H5O probably don't need them though.
 */
H5_DLL herr_t H5G_ent_encode(H5F_t *f, uint8_t **pp, const H5G_entry_t *ent);
H5_DLL herr_t H5G_ent_decode(H5F_t *f, const uint8_t **pp,
			      H5G_entry_t *ent/*out*/);
H5_DLL H5G_cache_t *H5G_ent_cache(H5G_entry_t *ent, H5G_type_t *cache_type);
H5_DLL  herr_t H5G_ent_copy(H5G_entry_t *dst, const H5G_entry_t *src,
            H5G_ent_copy_depth_t depth);
H5_DLL  herr_t H5G_free_ent_name(H5G_entry_t *ent);
H5_DLL herr_t H5G_ent_debug(H5F_t *f, hid_t dxpl_id, const H5G_entry_t *ent, FILE * stream,
			     int indent, int fwidth, haddr_t heap);
#endif
