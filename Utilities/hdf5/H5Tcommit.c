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
 * Module Info: This module contains the functionality for committing datatypes
 *      to a file for the H5T interface.
 */

#define H5T_PACKAGE    /*suppress error about including H5Tpkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5T_init_commit_interface


#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling      */
#include "H5FOprivate.h"  /* File objects        */
#include "H5Iprivate.h"    /* IDs          */
#include "H5Oprivate.h"    /* Object headers      */
#include "H5Tpkg.h"    /* Datatypes        */

/* Static local functions */
static herr_t H5T_commit(H5G_entry_t *loc, const char *name, H5T_t *type, hid_t dxpl_id);


/*--------------------------------------------------------------------------
NAME
   H5T_init_commit_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_commit_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_commit_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_commit_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_commit_interface() */


/*-------------------------------------------------------------------------
 * Function:  H5Tcommit
 *
 * Purpose:  Save a transient datatype to a file and turn the type handle
 *    into a named, immutable type.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, June  1, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tcommit(hid_t loc_id, const char *name, hid_t type_id)
{
    H5G_entry_t  *loc = NULL;
    H5T_t  *type = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tcommit, FAIL)
    H5TRACE3("e","isi",loc_id,name,type_id);

    /* Check arguments */
    if (NULL==(loc=H5G_loc (loc_id)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if (!name || !*name)
  HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if (NULL==(type=H5I_object_verify(type_id, H5I_DATATYPE)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Commit the type */
    if (H5T_commit(loc, name, type, H5AC_dxpl_id)<0)
  HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to commit datatype")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5T_commit
 *
 * Purpose:  Commit a type, giving it a name and causing it to become
 *    immutable.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, June  1, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_commit (H5G_entry_t *loc, const char *name, H5T_t *type, hid_t dxpl_id)
{
    H5F_t  *file = NULL;
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_commit)

    HDassert (loc);
    HDassert (name && *name);
    HDassert (type);

    /*
     * Check arguments.  We cannot commit an immutable type because H5Tclose()
     * normally fails on such types (try H5Tclose(H5T_NATIVE_INT)) but closing
     * a named type should always succeed.
     */
    if (H5T_STATE_NAMED==type->shared->state || H5T_STATE_OPEN==type->shared->state)
  HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "datatype is already committed")
    if (H5T_STATE_IMMUTABLE==type->shared->state)
  HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "datatype is immutable")

    /* Find the insertion file */
    if (NULL==(file=H5G_insertion_file(loc, name, dxpl_id)))
  HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to find insertion point")

    /* Check for a "sensible" datatype to store on disk */
    if(H5T_is_sensible(type)<=0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "datatype is not sensible")

    /* Mark datatype as being on disk now.  This step changes the size of datatype as
     * stored on disk. */
    if(H5T_vlen_mark(type, file, H5T_VLEN_DISK)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");

    /*
     * Create the object header and open it for write access. Insert the data
     * type message and then give the object header a name.
     */
    if (H5O_create (file, dxpl_id, 64, &(type->ent))<0)
  HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to create datatype object header")
    if (H5O_modify (&(type->ent), H5O_DTYPE_ID, 0, H5O_FLAG_CONSTANT, H5O_UPDATE_TIME, type, dxpl_id)<0)
  HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to update type header message")

    /*
     * Give the datatype a name.  That is, create and add a new object to the
     * group this datatype is being initially created in.
     */
    if (H5G_insert (loc, name, &(type->ent), dxpl_id)<0)
  HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to name datatype")
    type->shared->state = H5T_STATE_OPEN;
    type->shared->fo_count=1;

    /* Add datatype to the list of open objects in the file */
    if(H5FO_top_incr(type->ent.file, type->ent.header)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINC, FAIL, "can't incr object ref. count")
    if(H5FO_insert(type->ent.file, type->ent.header, type->shared)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "can't insert datatype into list of open objects")

    /* Mark datatype as being on memory now.  Since this datatype may still be used in memory
     * after committed to disk, change its size back as in memory. */
    if (H5T_vlen_mark(type, NULL, H5T_VLEN_MEMORY)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "cannot mark datatype in memory")

done:
    if (ret_value<0) {
  if ((type->shared->state==H5T_STATE_TRANSIENT || type->shared->state==H5T_STATE_RDONLY) && H5F_addr_defined(type->ent.header)) {
      if(H5O_close(&(type->ent))<0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "unable to release object header")
            if(H5O_delete(file, dxpl_id,type->ent.header)<0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CANTDELETE, FAIL, "unable to delete object header")
      type->ent.header = HADDR_UNDEF;
  }
    }

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Tcommitted
 *
 * Purpose:  Determines if a datatype is committed or not.
 *
 * Return:  Success:  TRUE if committed, FALSE otherwise.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, June  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tcommitted(hid_t type_id)
{
    H5T_t  *type = NULL;
    htri_t      ret_value;       /* Return value */

    FUNC_ENTER_API(H5Tcommitted, FAIL)
    H5TRACE1("t","i",type_id);

    /* Check arguments */
    if (NULL==(type=H5I_object_verify(type_id,H5I_DATATYPE)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Set return value */
    ret_value= H5T_committed(type);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5T_committed
 *
 * Purpose:  Determines if a datatype is committed or not.
 *
 * Return:  Success:  TRUE if committed, FALSE otherwise.
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, September 24, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_committed(const H5T_t *type)
{
    /* Use no-init for efficiency */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_committed)

    assert (type);

    FUNC_LEAVE_NOAPI(H5T_STATE_OPEN==type->shared->state || H5T_STATE_NAMED==type->shared->state)
} /* end H5T_committed() */


/*-------------------------------------------------------------------------
 * Function:  H5T_link
 *
 * Purpose:  Adjust the link count for an object header by adding
 *    ADJUST to the link count.
 *
 * Return:  Success:  New link count
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 26, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5T_link(const H5T_t *type, int adjust, hid_t dxpl_id)
{
    int ret_value;      /* Return value */

    /* Use no-init for efficiency */
    FUNC_ENTER_NOAPI(H5T_link,FAIL)

    assert (type);

    /* Adjust the link count on the named datatype */
    if((ret_value=H5O_link(&(type->ent),adjust,dxpl_id))<0)
        HGOTO_ERROR (H5E_DATATYPE, H5E_LINK, FAIL, "unable to adjust named datatype link count")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_link() */

