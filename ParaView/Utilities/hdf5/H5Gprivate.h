/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Gprivate.h
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Library-visible declarations.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Gprivate_H
#define _H5Gprivate_H

#include "H5Gpublic.h"

/* Private headers needed by this file */
#include "H5private.h"
#include "H5Bprivate.h"
#include "H5Fprivate.h"

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5G_DEBUG
#endif
#define H5G_NODE_MAGIC  "SNOD"          /*symbol table node magic number     */
#define H5G_NODE_SIZEOF_MAGIC 4         /*sizeof symbol node magic number    */
#define H5G_NO_CHANGE   (-1)            /*see H5G_ent_modified()             */
#define H5G_NLINKS      16              /*max symlinks to follow per lookup  */

/*
 * The disk size for a symbol table entry...
 */
#define H5G_SIZEOF_SCRATCH      16
#define H5G_SIZEOF_ENTRY(F)                                                   \
   (H5F_SIZEOF_SIZE(F) +        /*offset of name into heap              */    \
    H5F_SIZEOF_ADDR(F) +        /*address of object header              */    \
    4 +                         /*entry type                            */    \
    4 +                         /*reserved                              */    \
    H5G_SIZEOF_SCRATCH)         /*scratch pad space                     */

/*
 * Various types of object header information can be cached in a symbol
 * table entry (it's normal home is the object header to which the entry
 * points).  This datatype determines what (if anything) is cached in the
 * symbol table entry.
 */
typedef enum H5G_type_t {
    H5G_CACHED_ERROR    = -1,   /*force enum to be signed                    */
    H5G_NOTHING_CACHED  = 0,    /*nothing is cached, must be 0               */
    H5G_CACHED_STAB     = 1,    /*symbol table, `stab'                       */
    H5G_CACHED_SLINK    = 2,    /*symbolic link                              */

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
        size_t  lval_offset;            /*link value offset                  */
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
} H5G_entry_t;
typedef struct H5G_t H5G_t;

/*
 * This table contains a list of object types, descriptions, and the
 * functions that determine if some object is a particular type.  The table
 * is allocated dynamically.
 */
typedef struct H5G_typeinfo_t {
    int type;                   /*one of the public H5G_* types      */
    htri_t      (*isa)(H5G_entry_t*);   /*function to determine type         */
    char        *desc;                  /*description of object type         */
} H5G_typeinfo_t;

/*
 * Library prototypes...  These are the ones that other packages routinely
 * call.
 */
__DLL__ herr_t H5G_register_type(int type, htri_t(*isa)(H5G_entry_t*),
                                 const char *desc);
__DLL__ H5G_entry_t *H5G_loc(hid_t loc_id);
__DLL__ herr_t H5G_mkroot(H5F_t *f, H5G_entry_t *root_entry);
__DLL__ H5G_entry_t *H5G_entof(H5G_t *grp);
__DLL__ H5F_t *H5G_fileof(H5G_t *grp);
__DLL__ H5G_t *H5G_create(H5G_entry_t *loc, const char *name,
                          size_t size_hint);
__DLL__ H5G_t *H5G_open(H5G_entry_t *loc, const char *name);
__DLL__ H5G_t *H5G_open_oid(H5G_entry_t *ent);
__DLL__ H5G_t *H5G_reopen(H5G_t *grp);
__DLL__ herr_t H5G_close(H5G_t *grp);
__DLL__ H5G_t *H5G_rootof(H5F_t *f);
__DLL__ htri_t H5G_isa(H5G_entry_t *ent);
__DLL__ herr_t H5G_link(H5G_entry_t *loc, H5G_link_t type,
                        const char *cur_name, const char *new_name,
                        unsigned namei_flags);
__DLL__ int H5G_get_type(H5G_entry_t *ent);
__DLL__ herr_t H5G_get_objinfo(H5G_entry_t *loc, const char *name,
                               hbool_t follow_link,
                               H5G_stat_t *statbuf/*out*/);
__DLL__ herr_t H5G_linkval(H5G_entry_t *loc, const char *name, size_t size,
                           char *buf/*out*/);
__DLL__ herr_t H5G_set_comment(H5G_entry_t *loc, const char *name,
                               const char *buf);
__DLL__ int H5G_get_comment(H5G_entry_t *loc, const char *name,
                             size_t bufsize, char *buf);
__DLL__ herr_t H5G_insert(H5G_entry_t *loc, const char *name,
                          H5G_entry_t *ent);
__DLL__ herr_t H5G_move(H5G_entry_t *loc, const char *src_name,
                        const char *dst_name);
__DLL__ herr_t H5G_unlink(H5G_entry_t *loc, const char *name);
__DLL__ herr_t H5G_find(H5G_entry_t *loc, const char *name,
                        H5G_entry_t *grp_ent/*out*/, H5G_entry_t *ent/*out*/);
__DLL__ H5F_t *H5G_insertion_file(H5G_entry_t *loc, const char *name);
__DLL__ herr_t H5G_traverse_slink(H5G_entry_t *grp_ent/*in,out*/,
                                  H5G_entry_t *obj_ent/*in,out*/,
                                  int *nlinks/*in,out*/);
__DLL__ herr_t H5G_ent_encode(H5F_t *f, uint8_t **pp, const H5G_entry_t *ent);
__DLL__ herr_t H5G_ent_decode(H5F_t *f, const uint8_t **pp,
                              H5G_entry_t *ent/*out*/);

/*
 * These functions operate on symbol table nodes.
 */
__DLL__ herr_t H5G_node_debug(H5F_t *f, haddr_t addr, FILE *stream,
                              int indent, int fwidth, haddr_t heap);

/*
 * These functions operate on symbol table entries.  They're used primarily
 * in the H5O package where header messages are cached in symbol table
 * entries.  The subclasses of H5O probably don't need them though.
 */
__DLL__ H5G_cache_t *H5G_ent_cache(H5G_entry_t *ent, H5G_type_t *cache_type);
__DLL__ herr_t H5G_ent_modified(H5G_entry_t *ent, H5G_type_t cache_type);
__DLL__ herr_t H5G_ent_debug(H5F_t *f, const H5G_entry_t *ent, FILE * stream,
                             int indent, int fwidth, haddr_t heap);
#endif
