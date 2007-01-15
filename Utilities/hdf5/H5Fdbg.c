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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Wednesday, July 9, 2003
 *
 * Purpose:  File object debugging functions.
 */

#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling            */
#include "H5Fpkg.h"             /* File access        */
#include "H5Iprivate.h"    /* ID Functions                    */
#include "H5Pprivate.h"    /* Property lists      */


/*-------------------------------------------------------------------------
 * Function:  H5F_debug
 *
 * Purpose:  Prints a file header to the specified stream.  Each line
 *    is indented and the field name occupies the specified width
 *    number of characters.
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
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *
 *    Raymond Lu, 2001-10-14
 *     Changed to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_debug(H5F_t *f, hid_t dxpl_id, FILE * stream, int indent, int fwidth)
{
    hsize_t userblock_size;
    int     super_vers, freespace_vers, obj_dir_vers, share_head_vers;
    H5P_genplist_t *plist;              /* Property list */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_debug, FAIL)

    /* check args */
    assert(f);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    /* Get property list */
    if(NULL == (plist = H5I_object(f->shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    if(H5P_get(plist, H5F_CRT_USER_BLOCK_NAME, &userblock_size)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get user block size")
    if(H5P_get(plist, H5F_CRT_SUPER_VERS_NAME, &super_vers)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get super block version")
    if(H5P_get(plist, H5F_CRT_FREESPACE_VERS_NAME, &freespace_vers)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get super block version")
    if(H5P_get(plist, H5F_CRT_OBJ_DIR_VERS_NAME, &obj_dir_vers)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get object directory version")
    if(H5P_get(plist, H5F_CRT_SHARE_HEAD_VERS_NAME, &share_head_vers)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get shared-header format version")

    /* debug */
    HDfprintf(stream, "%*sFile Super Block...\n", indent, "");

    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "File name:",
        f->name);
    HDfprintf(stream, "%*s%-*s 0x%08x\n", indent, "", fwidth,
        "File access flags",
        (unsigned) (f->shared->flags));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "File open reference count:",
        (unsigned) (f->shared->nrefs));
    HDfprintf(stream, "%*s%-*s %a (abs)\n", indent, "", fwidth,
        "Address of super block:", f->shared->super_addr);
    HDfprintf(stream, "%*s%-*s %lu bytes\n", indent, "", fwidth,
        "Size of user block:", (unsigned long) userblock_size);

    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Super block version number:", (unsigned) super_vers);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Free list version number:", (unsigned) freespace_vers);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Root group symbol table entry version number:", (unsigned) obj_dir_vers);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Shared header version number:", (unsigned) share_head_vers);
    HDfprintf(stream, "%*s%-*s %u bytes\n", indent, "", fwidth,
        "Size of file offsets (haddr_t type):", (unsigned) f->shared->sizeof_addr);
    HDfprintf(stream, "%*s%-*s %u bytes\n", indent, "", fwidth,
        "Size of file lengths (hsize_t type):", (unsigned) f->shared->sizeof_size);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Symbol table leaf node 1/2 rank:", f->shared->sym_leaf_k);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Symbol table internal node 1/2 rank:",
              f->shared->btree_k[H5B_SNODE_ID]);
    HDfprintf(stream, "%*s%-*s 0x%08lx\n", indent, "", fwidth,
        "File consistency flags:",
        (unsigned long) (f->shared->consist_flags));
    HDfprintf(stream, "%*s%-*s %a (abs)\n", indent, "", fwidth,
        "Base address:", f->shared->base_addr);
    HDfprintf(stream, "%*s%-*s %a (rel)\n", indent, "", fwidth,
        "Free list address:", f->shared->freespace_addr);

    HDfprintf(stream, "%*s%-*s %a (rel)\n", indent, "", fwidth,
        "Address of driver information block:", f->shared->driver_addr);

    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Root group symbol table entry:",
        f->shared->root_grp ? "" : "(none)");
    if (f->shared->root_grp) {
  H5G_ent_debug(f, dxpl_id, H5G_entof(f->shared->root_grp), stream,
          indent+3, MAX(0, fwidth-3), HADDR_UNDEF);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

