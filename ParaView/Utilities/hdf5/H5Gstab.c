/*
 * Copyright (C) 1997 National Center for Supercomputing Applications
 *                    All rights reserved.
 *
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *             Friday, September 19, 1997
 *
 */
#define H5G_PACKAGE
#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"         /*file access                             */
#include "H5Gpkg.h"
#include "H5HLprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"

#define PABLO_MASK      H5G_stab_mask
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

/*-------------------------------------------------------------------------
 * Function:    H5G_stab_create
 *
 * Purpose:     Creates a new empty symbol table (object header, name heap,
 *              and B-tree).  The caller can specify an initial size for the
 *              name heap.  The object header of the group is opened for
 *              write access.
 *
 *              In order for the B-tree to operate correctly, the first
 *              item in the heap is the empty string, and must appear at
 *              heap offset zero.
 *
 * Errors:
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  1 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_create(H5F_t *f, size_t init, H5G_entry_t *self/*out*/)
{
    size_t                  name;       /*offset of "" name     */
    H5O_stab_t              stab;       /*symbol table message  */

    FUNC_ENTER(H5G_stab_create, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(self);
    init = MAX(init, H5HL_SIZEOF_FREE(f) + 2);

    /* Create symbol table private heap */
    if (H5HL_create(f, init, &(stab.heap_addr)/*out*/)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create heap");
    }
    name = H5HL_insert(f, stab.heap_addr, 1, "");
    if ((size_t)(-1)==name) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't initialize heap");
    }

    /*
     * B-tree's won't work if the first name isn't at the beginning
     * of the heap.
     */
    assert(0 == name);

    /* Create the B-tree */
    if (H5B_create(f, H5B_SNODE, NULL, &(stab.btree_addr)/*out*/) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create B-tree");
    }

    /*
     * Create symbol table object header.  It has a zero link count
     * since nothing refers to it yet.  The link count will be
     * incremented if the object is added to the group directed graph.
     */
    if (H5O_create(f, 4 + 2 * H5F_SIZEOF_ADDR(f), self/*out*/) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create header");
    }

    /*
     * Insert the symbol table message into the object header and the symbol
     * table entry.
     */
    if (H5O_modify(self, H5O_STAB, H5O_NEW_MESG, H5O_FLAG_CONSTANT, &stab)<0) {
        H5O_close(self);
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create message");
    }
    self->cache.stab.btree_addr = stab.btree_addr;
    self->cache.stab.heap_addr = stab.heap_addr;
    self->type = H5G_CACHED_STAB;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_stab_find
 *
 * Purpose:     Finds a symbol named NAME in the symbol table whose
 *              description is stored in GRP_ENT in file F and returns its
 *              symbol table entry through OBJ_ENT (which is optional).
 *
 * Errors:
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  1 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_find(H5G_entry_t *grp_ent, const char *name,
              H5G_entry_t *obj_ent/*out*/)
{
    H5G_bt_ud1_t        udata;          /*data to pass through B-tree   */
    H5O_stab_t          stab;           /*symbol table message          */

    FUNC_ENTER(H5G_stab_find, FAIL);

    /* Check arguments */
    assert(grp_ent);
    assert(grp_ent->file);
    assert(name && *name);

    /* set up the udata */
    if (NULL == H5O_read(grp_ent, H5O_STAB, 0, &stab)) {
        HRETURN_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "can't read message");
    }
    udata.operation = H5G_OPER_FIND;
    udata.name = name;
    udata.heap_addr = stab.heap_addr;

    /* search the B-tree */
    if (H5B_find(grp_ent->file, H5B_SNODE, stab.btree_addr, &udata) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "not found");
    }
    if (obj_ent) *obj_ent = udata.ent;
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5G_stab_insert
 *
 * Purpose:     Insert a new symbol into the table described by GRP_ENT in
 *              file F.  The name of the new symbol is NAME and its symbol
 *              table entry is OBJ_ENT.
 *
 * Errors:
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  1 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_insert(H5G_entry_t *grp_ent, const char *name, H5G_entry_t *obj_ent)
{
    H5O_stab_t          stab;           /*symbol table message          */
    H5G_bt_ud1_t        udata;          /*data to pass through B-tree   */
    static double       split_ratios[3] = {0.1, 0.5, 0.9};

    FUNC_ENTER(H5G_stab_insert, FAIL);

    /* check arguments */
    assert(grp_ent && grp_ent->file);
    assert(name && *name);
    assert(obj_ent && obj_ent->file);
    if (grp_ent->file->shared != obj_ent->file->shared) {
        HRETURN_ERROR(H5E_SYM, H5E_LINK, FAIL,
                      "interfile hard links are not allowed");
    }

    /* initialize data to pass through B-tree */
    if (NULL == H5O_read(grp_ent, H5O_STAB, 0, &stab)) {
        HRETURN_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table");
    }
    udata.operation = H5G_OPER_INSERT;
    udata.name = name;
    udata.heap_addr = stab.heap_addr;
    udata.ent = *obj_ent;

    /* insert */
    if (H5B_insert(grp_ent->file, H5B_SNODE, stab.btree_addr, split_ratios,
                   &udata) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "unable to insert entry");
    }
    
    /* update the name offset in the entry */
    obj_ent->name_off = udata.ent.name_off;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_stab_remove
 *
 * Purpose:     Remove NAME from a symbol table.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, September 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_remove(H5G_entry_t *grp_ent, const char *name)
{
    H5O_stab_t          stab;           /*symbol table message          */
    H5G_bt_ud1_t        udata;          /*data to pass through B-tree   */
    
    FUNC_ENTER(H5G_stab_remove, FAIL);
    assert(grp_ent && grp_ent->file);
    assert(name && *name);

    /* initialize data to pass through B-tree */
    if (NULL==H5O_read(grp_ent, H5O_STAB, 0, &stab)) {
        HRETURN_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table");
    }
    udata.operation = H5G_OPER_REMOVE;
    udata.name = name;
    udata.heap_addr = stab.heap_addr;
    HDmemset(&(udata.ent), 0, sizeof(udata.ent));

    /* remove */
    if (H5B_remove(grp_ent->file, H5B_SNODE, stab.btree_addr, &udata)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to remove entry");
    }

    FUNC_LEAVE(SUCCEED);
}
