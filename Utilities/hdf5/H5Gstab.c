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

/* Programmer: Robb Matzke <matzke@llnl.gov>
 *         Friday, September 19, 1997
 *
 */
#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */
#define H5G_PACKAGE    /*suppress error about including H5Gpkg    */


/* Packages needed by this file... */
#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fpkg.h"    /* File access        */
#include "H5Gpkg.h"    /* Groups          */
#include "H5HLprivate.h"  /* Local Heaps        */
#include "H5MMprivate.h"  /* Memory management      */

/* Private prototypes */


/*-------------------------------------------------------------------------
 * Function:  H5G_stab_create
 *
 * Purpose:  Creates a new empty symbol table (object header, name heap,
 *    and B-tree).  The caller can specify an initial size for the
 *    name heap.  The object header of the group is opened for
 *    write access.
 *
 *    In order for the B-tree to operate correctly, the first
 *    item in the heap is the empty string, and must appear at
 *    heap offset zero.
 *
 * Errors:
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    matzke@llnl.gov
 *    Aug  1 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_create(H5F_t *f, hid_t dxpl_id, size_t init, H5G_entry_t *self/*out*/)
{
    size_t        name;  /*offset of "" name  */
    H5O_stab_t        stab;  /*symbol table message  */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_create, FAIL)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(self);
    init = MAX(init, H5HL_SIZEOF_FREE(f) + 2);

    /* Create symbol table private heap */
    if (H5HL_create(f, dxpl_id, init, &(stab.heap_addr)/*out*/)<0)
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create heap")
    name = H5HL_insert(f, dxpl_id, stab.heap_addr, 1, "");
    if ((size_t)(-1)==name)
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't initialize heap")

    /*
     * B-tree's won't work if the first name isn't at the beginning
     * of the heap.
     */
    assert(0 == name);

    /* Create the B-tree */
    if (H5B_create(f, dxpl_id, H5B_SNODE, NULL, &(stab.btree_addr)/*out*/) < 0)
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create B-tree")

    /*
     * Create symbol table object header.  It has a zero link count
     * since nothing refers to it yet.  The link count will be
     * incremented if the object is added to the group directed graph.
     */
    if (H5O_create(f, dxpl_id, 4 + 2 * H5F_SIZEOF_ADDR(f), self/*out*/) < 0)
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create header")

    /*
     * Insert the symbol table message into the object header and the symbol
     * table entry.
     */
    if (H5O_modify(self, H5O_STAB_ID, H5O_NEW_MESG, H5O_FLAG_CONSTANT, H5O_UPDATE_TIME, &stab, dxpl_id)<0) {
  H5O_close(self);
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create message")
    }
    self->cache.stab.btree_addr = stab.btree_addr;
    self->cache.stab.heap_addr = stab.heap_addr;
    self->type = H5G_CACHED_STAB;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_create() */


/*-------------------------------------------------------------------------
 * Function:  H5G_stab_find
 *
 * Purpose:  Finds a symbol named NAME in the symbol table whose
 *    description is stored in GRP_ENT in file F and returns its
 *    symbol table entry through OBJ_ENT (which is optional).
 *
 * Errors:
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    matzke@llnl.gov
 *    Aug  1 1997
 *
 * Modifications:
 *
 *      Pedro Vicente, <pvn@ncsa.uiuc.edu> 22 Aug 2002
 *      Added `id to name' support.
 *      Added a deep copy of the symbol table entry
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_find(H5G_entry_t *grp_ent, const char *name,
        H5G_entry_t *obj_ent/*out*/, hid_t dxpl_id)
{
    H5G_bt_ud1_t  udata;    /*data to pass through B-tree  */
    H5O_stab_t    stab;    /*symbol table message    */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_find, FAIL)

    /* Check arguments */
    assert(grp_ent);
    assert(grp_ent->file);
    assert(obj_ent);
    assert(name && *name);

    /* set up the udata */
    if (NULL == H5O_read(grp_ent, H5O_STAB_ID, 0, &stab, dxpl_id))
  HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "can't read message")
    udata.common.name = name;
    udata.common.heap_addr = stab.heap_addr;
    udata.ent = obj_ent;

    /* search the B-tree */
    if (H5B_find(grp_ent->file, dxpl_id, H5B_SNODE, stab.btree_addr, &udata) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "not found")

    /* Set the name for the symbol entry OBJ_ENT */
    if (H5G_ent_set_name( grp_ent, obj_ent, name ) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot insert name")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_find() */


/*-------------------------------------------------------------------------
 * Function:  H5G_stab_insert
 *
 * Purpose:  Insert a new symbol into the table described by GRP_ENT in
 *    file F.   The name of the new symbol is NAME and its symbol
 *    table entry is OBJ_ENT.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    matzke@llnl.gov
 *    Aug  1 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_insert(H5G_entry_t *grp_ent, const char *name, H5G_entry_t *obj_ent,
    hbool_t inc_link, hid_t dxpl_id)
{
    H5O_stab_t    stab;    /*symbol table message    */
    H5G_bt_ud1_t  udata;    /*data to pass through B-tree  */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_insert, FAIL)

    /* check arguments */
    assert(grp_ent && grp_ent->file);
    assert(name && *name);
    assert(obj_ent && obj_ent->file);
    if (grp_ent->file->shared != obj_ent->file->shared)
  HGOTO_ERROR(H5E_SYM, H5E_LINK, FAIL, "interfile hard links are not allowed")

    /* Set the name for the symbol entry OBJ_ENT */
    if (H5G_ent_set_name( grp_ent, obj_ent, name ) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot insert name")

    /* initialize data to pass through B-tree */
    if (NULL == H5O_read(grp_ent, H5O_STAB_ID, 0, &stab, dxpl_id))
  HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table")

    udata.common.name = name;
    udata.common.heap_addr = stab.heap_addr;
    udata.ent = obj_ent;

    /* insert */
    if (H5B_insert(grp_ent->file, dxpl_id, H5B_SNODE, stab.btree_addr, &udata) < 0)
  HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "unable to insert entry")

    /* Increment link count on object, if appropriate */
    if(inc_link)
        if (H5O_link(obj_ent, 1, dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_LINK, FAIL, "unable to increment hard link count")

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5G_stab_remove
 *
 * Purpose:  Remove NAME from a symbol table.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, September 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_remove(H5G_entry_t *grp_ent, const char *name, hid_t dxpl_id)
{
    H5O_stab_t    stab;    /*symbol table message    */
    H5G_bt_ud2_t  udata;    /*data to pass through B-tree  */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_remove, FAIL)

    assert(grp_ent && grp_ent->file);
    assert(name && *name);

    /* initialize data to pass through B-tree */
    if (NULL==H5O_read(grp_ent, H5O_STAB_ID, 0, &stab, dxpl_id))
  HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table")
    udata.common.name = name;
    udata.common.heap_addr = stab.heap_addr;
    udata.adj_link = TRUE;

    /* remove */
    if (H5B_remove(grp_ent->file, dxpl_id, H5B_SNODE, stab.btree_addr, &udata)<0)
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to remove entry")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5G_stab_delete
 *
 * Purpose:  Delete entire symbol table information from file
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 20, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_delete(H5F_t *f, hid_t dxpl_id, const H5O_stab_t *stab, hbool_t adj_link)
{
    H5G_bt_ud2_t  udata;    /*data to pass through B-tree  */
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5G_stab_delete, FAIL);

    assert(f);
    assert(stab);
    assert(H5F_addr_defined(stab->btree_addr));
    assert(H5F_addr_defined(stab->heap_addr));

    /* Set up user data for B-tree deletion */
    udata.common.name = NULL;
    udata.common.heap_addr = stab->heap_addr;
    udata.adj_link = adj_link;

    /* Delete entire B-tree */
    if(H5B_delete(f, dxpl_id, H5B_SNODE, stab->btree_addr, &udata)<0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete symbol table B-tree");

    /* Delete local heap for names */
    if(H5HL_delete(f, dxpl_id, stab->heap_addr)<0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete symbol table heap");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5G_stab_delete() */
