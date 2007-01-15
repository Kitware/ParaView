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

#define H5P_PACKAGE    /*suppress error about including H5Ppkg    */


/* Private header files */
#include "H5private.h"    /* Generic Functions      */
#include "H5Dprivate.h"    /* Datasets        */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fprivate.h"    /* Files        */
#include "H5FDprivate.h"  /* File drivers        */
#include "H5Iprivate.h"    /* IDs            */
#include "H5Ppkg.h"    /* Property lists        */

/* Default file driver - see H5Pget_driver() */
#include "H5FDsec2.h"    /* Posix unbuffered I/O  file driver  */

/* Local datatypes */

/* Static function prototypes */
static herr_t H5P_set_family_offset(H5P_genplist_t *plist, hsize_t offset);
static herr_t H5P_get_family_offset(H5P_genplist_t *plist, hsize_t *offset);
static herr_t H5P_set_multi_type(H5P_genplist_t *plist, H5FD_mem_t type);
static herr_t H5P_get_multi_type(H5P_genplist_t *plist, H5FD_mem_t *type);


/*-------------------------------------------------------------------------
 * Function:  H5Pset_alignment
 *
 * Purpose:  Sets the alignment properties of a file access property list
 *    so that any file object >= THRESHOLD bytes will be aligned on
 *    an address which is a multiple of ALIGNMENT.  The addresses
 *    are relative to the end of the user block; the alignment is
 *    calculated by subtracting the user block size from the
 *    absolute file address and then adjusting the address to be a
 *    multiple of ALIGNMENT.
 *
 *    Default values for THRESHOLD and ALIGNMENT are one, implying
 *    no alignment.  Generally the default values will result in
 *    the best performance for single-process access to the file.
 *    For MPI-IO and other parallel systems, choose an alignment
 *    which is a multiple of the disk block size.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, June  9, 1998
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed file access property list mechanism to the new
 *    generic property list.
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_alignment(hid_t fapl_id, hsize_t threshold, hsize_t alignment)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_alignment, FAIL);
    H5TRACE3("e","ihh",fapl_id,threshold,alignment);

    /* Check args */
    if (alignment<1)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "alignment must be positive");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_ACS_ALIGN_THRHD_NAME, &threshold) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set threshold");
    if(H5P_set(plist, H5F_ACS_ALIGN_NAME, &alignment) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set alignment");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pget_alignment
 *
 * Purpose:  Returns the current settings for alignment properties from a
 *    file access property list.  The THRESHOLD and/or ALIGNMENT
 *    pointers may be null pointers.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, June  9, 1998
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list design to the new generic
 *    property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_alignment(hid_t fapl_id, hsize_t *threshold/*out*/,
      hsize_t *alignment/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pget_alignment, FAIL);
    H5TRACE3("e","ixx",fapl_id,threshold,alignment);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (threshold)
        if(H5P_get(plist, H5F_ACS_ALIGN_THRHD_NAME, threshold) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get threshold");
    if (alignment)
        if(H5P_get(plist, H5F_ACS_ALIGN_NAME, alignment) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get alignment");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5P_set_driver
 *
 * Purpose:  Set the file driver (DRIVER_ID) for a file access or data
 *    transfer property list (PLIST_ID) and supply an optional
 *    struct containing the driver-specific properites
 *    (DRIVER_INFO).  The driver properties will be copied into the
 *    property list and the reference count on the driver will be
 *    incremented, allowing the caller to close the driver ID but
 *    still use the property list.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *     Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list design to the new generic
 *    property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_set_driver(H5P_genplist_t *plist, hid_t new_driver_id, const void *new_driver_info)
{
    hid_t driver_id;            /* VFL driver ID */
    void *driver_info;          /* VFL driver info */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5P_set_driver, FAIL);

    if (NULL==H5I_object_verify(new_driver_id, H5I_VFL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file driver ID");

    if( TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
        /* Get the current driver information */
        if(H5P_get(plist, H5F_ACS_FILE_DRV_ID_NAME, &driver_id) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get driver ID");
        if(H5P_get(plist, H5F_ACS_FILE_DRV_INFO_NAME, &driver_info) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL,"can't get driver info");

        /* Close the driver for the property list */
        if(H5FD_fapl_close(driver_id, driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't reset driver")

        /* Set the driver for the property list */
        if(H5FD_fapl_open(plist, new_driver_id, new_driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver")
    } else if( TRUE == H5P_isa_class(plist->plist_id, H5P_DATASET_XFER) ) {
        /* Get the current driver information */
        if(H5P_get(plist, H5D_XFER_VFL_ID_NAME, &driver_id)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve VFL driver ID");
        if(H5P_get(plist, H5D_XFER_VFL_INFO_NAME, &driver_info)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve VFL driver info");

        /* Close the driver for the property list */
        if(H5FD_dxpl_close(driver_id, driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't reset driver")

        /* Set the driver for the property list */
        if(H5FD_dxpl_open(plist, new_driver_id, new_driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver")
    } else {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access or data transfer property list");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_set_driver() */


/*-------------------------------------------------------------------------
 * Function:  H5Pset_driver
 *
 * Purpose:  Set the file driver (DRIVER_ID) for a file access or data
 *    transfer property list (PLIST_ID) and supply an optional
 *    struct containing the driver-specific properites
 *    (DRIVER_INFO).  The driver properties will be copied into the
 *    property list and the reference count on the driver will be
 *    incremented, allowing the caller to close the driver ID but
 *    still use the property list.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *     Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list design to the new generic
 *    property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_driver(hid_t plist_id, hid_t new_driver_id, const void *new_driver_info)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Pset_driver, FAIL);
    H5TRACE3("e","iix",plist_id,new_driver_id,new_driver_info);

    /* Check arguments */
    if(NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (NULL==H5I_object_verify(new_driver_id, H5I_VFL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file driver ID");

    /* Set the driver */
    if(H5P_set_driver(plist,new_driver_id,new_driver_info)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver info");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_driver() */


/*-------------------------------------------------------------------------
 * Function:  H5P_get_driver
 *
 * Purpose:  Return the ID of the low-level file driver.  PLIST_ID should
 *    be a file access property list or data transfer propert list.
 *
 * Return:  Success:  A low-level driver ID which is the same ID
 *        used when the driver was set for the property
 *        list. The driver ID is only valid as long as
 *        the file driver remains registered.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Thursday, February 26, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-08-03
 *    Rewritten to use the virtual file layer.
 *
 *     Robb Matzke, 1999-08-05
 *    If the driver ID is H5FD_VFD_DEFAULT then substitute the
 *              current value of H5FD_SEC2.
 *
 *     Quincey Koziol 2000-11-28
 *    Added internal function..
 *
 *    Raymond Lu, 2001-10-23
 *    Changed the file access list design to the new generic
 *    property list.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5P_get_driver(H5P_genplist_t *plist)
{
    hid_t  ret_value=FAIL;         /* Return value */

    FUNC_ENTER_NOAPI(H5P_get_driver, FAIL);

    /* Get the current driver ID */
    if(TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
        if(H5P_get(plist, H5F_ACS_FILE_DRV_ID_NAME, &ret_value) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get driver ID");
    } else if( TRUE == H5P_isa_class(plist->plist_id, H5P_DATASET_XFER) ) {
        if(H5P_get(plist, H5D_XFER_VFL_ID_NAME, &ret_value)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve VFL driver ID");
    } else {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access or data transfer property list");
    }

    if (H5FD_VFD_DEFAULT==ret_value)
        ret_value = H5FD_SEC2;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pget_driver
 *
 * Purpose:  Return the ID of the low-level file driver.  PLIST_ID should
 *    be a file access property list or data transfer propert list.
 *
 * Return:  Success:  A low-level driver ID which is the same ID
 *        used when the driver was set for the property
 *        list. The driver ID is only valid as long as
 *        the file driver remains registered.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Thursday, February 26, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-08-03
 *    Rewritten to use the virtual file layer.
 *
 *     Robb Matzke, 1999-08-05
 *    If the driver ID is H5FD_VFD_DEFAULT then substitute the current value of
 *    H5FD_SEC2.
 *
 *     Quincey Koziol 2000-11-28
 *    Added internal function..
 *-------------------------------------------------------------------------
 */
hid_t
H5Pget_driver(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    hid_t  ret_value;      /* Return value */

    FUNC_ENTER_API(H5Pget_driver, FAIL);
    H5TRACE1("i","i",plist_id);

    if(NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");

    ret_value = H5P_get_driver(plist);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5P_get_driver_info
 *
 * Purpose:  Returns a pointer directly to the file driver-specific
 *    information of a file access or data transfer property list.
 *
 * Return:  Success:  Ptr to *uncopied* driver specific data
 *        structure if any.
 *
 *    Failure:  NULL. Null is also returned if the driver has
 *        not registered any driver-specific properties
 *        although no error is pushed on the stack in
 *        this case.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list design to the new generic
 *    property list.
 *
 *-------------------------------------------------------------------------
 */
void *
H5P_get_driver_info(H5P_genplist_t *plist)
{
    void  *ret_value=NULL;

    FUNC_ENTER_NOAPI(H5P_get_driver_info, NULL);

    /* Get the current driver info */
    if( TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
        if(H5P_get(plist, H5F_ACS_FILE_DRV_INFO_NAME, &ret_value) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,NULL,"can't get driver info");
    } else if( TRUE == H5P_isa_class(plist->plist_id, H5P_DATASET_XFER) ) {
        if(H5P_get(plist, H5D_XFER_VFL_INFO_NAME, &ret_value)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, NULL, "Can't retrieve VFL driver ID");
    } else {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access or data transfer property list");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_get_driver_info() */


/*-------------------------------------------------------------------------
 * Function:  H5Pget_driver_info
 *
 * Purpose:  Returns a pointer directly to the file driver-specific
 *    information of a file access or data transfer property list.
 *
 * Return:  Success:  Ptr to *uncopied* driver specific data
 *        structure if any.
 *
 *    Failure:  NULL. Null is also returned if the driver has
 *        not registered any driver-specific properties
 *        although no error is pushed on the stack in
 *        this case.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list design to the new generic
 *    property list.
 *
 *-------------------------------------------------------------------------
 */
void *
H5Pget_driver_info(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    void  *ret_value;     /* Return value */

    FUNC_ENTER_API(H5Pget_driver_info, NULL);

    if(NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a property list");

    if((ret_value=H5P_get_driver_info(plist))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTGET,NULL,"can't get driver info");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_driver_info() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_family_offset
 *
 * Purpose:     Set offset for family driver.  This file access property
 *              list will be passed to H5Fget_vfd_handle or H5FDget_vfd_handle
 *              to retrieve VFD file handle.
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
*/
herr_t
H5Pset_family_offset(hid_t fapl_id, hsize_t offset)
{
    H5P_genplist_t      *plist;                 /* Property list pointer */
    herr_t              ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pset_family_offset, FAIL);
    H5TRACE2("e","ih",fapl_id,offset);

    /* Get the plist structure */
    if(H5P_DEFAULT == fapl_id)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't modify default property list");
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
         HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");
    /* Set values */
    if((ret_value=H5P_set_family_offset(plist, offset)) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set family offset");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5P_set_family_offset
 *
 * Purpose:     Set offset for family driver.  Private function for
 *              H5Pset_family_offset
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_set_family_offset(H5P_genplist_t *plist, hsize_t offset)
{
    herr_t      ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5P_set_family_offset, FAIL);

    if( TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
         if(H5P_set(plist, H5F_ACS_FAMILY_OFFSET_NAME, &offset) < 0)
              HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL,"can't set offset for family file");
    } else {
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access or data transfer property list");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_family_offset
 *
 * Purpose:     Get offset for family driver.  This file access property
 *              list will be passed to H5Fget_vfd_handle or H5FDget_vfd_handle
 *              to retrieve VFD file handle.
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_family_offset(hid_t fapl_id, hsize_t *offset)
{
    H5P_genplist_t      *plist;                 /* Property list pointer */
    herr_t              ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pget_family_offset, FAIL);
    H5TRACE2("e","i*h",fapl_id,offset);

    /* Get the plist structure */
    if(H5P_DEFAULT == fapl_id)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't modify default property list");
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
         HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");
    /* Set values */
    if((ret_value=H5P_get_family_offset(plist, offset)) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't get family offset");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5P_get_family_offset
 *
 * Purpose:     Get offset for family driver.  Private function for
 *              H5Pget_family_offset
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_get_family_offset(H5P_genplist_t *plist, hsize_t *offset)
{
    herr_t      ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5P_get_family_offset, FAIL);

    if( TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
        if(H5P_get(plist, H5F_ACS_FAMILY_OFFSET_NAME, offset) < 0)
             HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL,"can't set offset for family file");
    } else {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access or data transfer property list");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_multi_type
 *
 * Purpose:     Set data type for multi driver.  This file access property
 *              list will be passed to H5Fget_vfd_handle or H5FDget_vfd_handle
 *              to retrieve VFD file handle.
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_multi_type(hid_t fapl_id, H5FD_mem_t type)
{
    H5P_genplist_t      *plist;                 /* Property list pointer */
    herr_t              ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pset_multi_type, FAIL);
    H5TRACE2("e","iMt",fapl_id,type);

    /* Get the plist structure */
    if(H5P_DEFAULT == fapl_id)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't modify default property list");
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
         HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");
    /* Set values */
    if((ret_value=H5P_set_multi_type(plist, type)) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set data type for multi driver");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5P_set_multi_type
 *
 * Purpose:     Set data type for multi file driver.  Private function for
 *              H5Pset_multi_type.
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_set_multi_type(H5P_genplist_t *plist, H5FD_mem_t type)
{
    herr_t      ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5P_set_multi_type, FAIL);

    if( TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
         if(H5P_set(plist, H5F_ACS_MULTI_TYPE_NAME, &type) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL,"can't set type for multi driver");
    } else {
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access or data transfer property list");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_multi_type
 *
 * Purpose:     Get data type for multi driver.  This file access property
 *              list will be passed to H5Fget_vfd_handle or H5FDget_vfd_handle
 *              to retrieve VFD file handle.
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_multi_type(hid_t fapl_id, H5FD_mem_t *type)
{
    H5P_genplist_t      *plist;                 /* Property list pointer */
    herr_t              ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pget_multi_type, FAIL);
    H5TRACE2("e","i*Mt",fapl_id,type);

    /* Get the plist structure */
    if(H5P_DEFAULT == fapl_id)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't modify default property list");
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
         HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");
    /* Set values */
    if((ret_value=H5P_get_multi_type(plist, type)) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't get data type for multi driver");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5P_get_multi_type
 *
 * Purpose:     Get data type for multi file driver.  Private function for
 *              H5Pget_multi_type.
 *
 * Return:      Success:        Non-negative value.
 *
 *              Failure:        Negative value.
 *
 * Programmer:  Raymond Lu
 *              Sep 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_get_multi_type(H5P_genplist_t *plist, H5FD_mem_t *type)
{
    herr_t      ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5P_get_multi_type, FAIL);

    if( TRUE == H5P_isa_class(plist->plist_id, H5P_FILE_ACCESS) ) {
         if(H5P_get(plist, H5F_ACS_MULTI_TYPE_NAME, type) < 0)
             HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL,"can't get type for multi driver");
    } else {
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


#ifdef H5_WANT_H5_V1_4_COMPAT
/*-------------------------------------------------------------------------
 * Function:  H5Pset_cache
 *
 * Purpose:  Set the number of objects in the meta data cache and the
 *    maximum number of chunks and bytes in the raw data chunk
 *    cache.
 *
 *     The RDCC_W0 value should be between 0 and 1 inclusive and
 *    indicates how much chunks that have been fully read or fully
 *    written are favored for preemption.  A value of zero means
 *    fully read or written chunks are treated no differently than
 *    other chunks (the preemption is strictly LRU) while a value
 *    of one means fully read chunks are always preempted before
 *    other chunks.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, May 19, 1998
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_cache(hid_t plist_id, int mdc_nelmts,
       int _rdcc_nelmts, size_t rdcc_nbytes, double rdcc_w0)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t rdcc_nelmts=(size_t)_rdcc_nelmts;    /* Work around variable changing size */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_cache, FAIL);
    H5TRACE5("e","iIsIszd",plist_id,mdc_nelmts,_rdcc_nelmts,rdcc_nbytes,
             rdcc_w0);

    /* Check arguments */
    if (mdc_nelmts<0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "meta data cache size must be non-negative");
    if (rdcc_w0<0.0 || rdcc_w0>1.0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "raw data cache w0 value must be between 0.0 and 1.0 inclusive");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set sizes */
    if(H5P_set(plist, H5F_ACS_META_CACHE_SIZE_NAME, &mdc_nelmts) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set meta data cache size");
    if(H5P_set(plist, H5F_ACS_DATA_CACHE_ELMT_SIZE_NAME, &rdcc_nelmts) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set data cache element size");
    if(H5P_set(plist, H5F_ACS_DATA_CACHE_BYTE_SIZE_NAME, &rdcc_nbytes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set data cache byte size");
    if(H5P_set(plist, H5F_ACS_PREEMPT_READ_CHUNKS_NAME, &rdcc_w0) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set preempt read chunks");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pget_cache
 *
 * Purpose:  Retrieves the maximum possible number of elements in the meta
 *    data cache and the maximum possible number of elements and
 *    bytes and the RDCC_W0 value in the raw data chunk cache.  Any
 *    (or all) arguments may be null pointers in which case the
 *    corresponding datum is not returned.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, May 19, 1998
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_cache(hid_t plist_id, int *mdc_nelmts,
       int *_rdcc_nelmts, size_t *rdcc_nbytes, double *rdcc_w0)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t rdcc_nelmts;         /* Work around variable changing size */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_cache, FAIL);
    H5TRACE5("e","i*Is*Is*z*d",plist_id,mdc_nelmts,_rdcc_nelmts,rdcc_nbytes,
             rdcc_w0);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get sizes */
    if (mdc_nelmts)
        if(H5P_get(plist, H5F_ACS_META_CACHE_SIZE_NAME, mdc_nelmts) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get meta data cache size");
    if (_rdcc_nelmts) {
        if(H5P_get(plist, H5F_ACS_DATA_CACHE_ELMT_SIZE_NAME, &rdcc_nelmts) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache element size");
        *_rdcc_nelmts=rdcc_nelmts;
    } /* end if */
    if (rdcc_nbytes)
        if(H5P_get(plist, H5F_ACS_DATA_CACHE_BYTE_SIZE_NAME, rdcc_nbytes) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache byte size");
    if (rdcc_w0)
        if(H5P_get(plist, H5F_ACS_PREEMPT_READ_CHUNKS_NAME, rdcc_w0) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get preempt read chunks");

done:
    FUNC_LEAVE_API(ret_value);
}

#else /* H5_WANT_H5_V1_4_COMPAT */

/*-------------------------------------------------------------------------
 * Function:  H5Pset_cache
 *
 * Purpose:  Set the number of objects in the meta data cache and the
 *    maximum number of chunks and bytes in the raw data chunk
 *    cache.
 *
 *     The RDCC_W0 value should be between 0 and 1 inclusive and
 *    indicates how much chunks that have been fully read or fully
 *    written are favored for preemption.  A value of zero means
 *    fully read or written chunks are treated no differently than
 *    other chunks (the preemption is strictly LRU) while a value
 *    of one means fully read chunks are always preempted before
 *    other chunks.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, May 19, 1998
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_cache(hid_t plist_id, int mdc_nelmts,
       size_t rdcc_nelmts, size_t rdcc_nbytes, double rdcc_w0)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_cache, FAIL);
    H5TRACE5("e","iIszzd",plist_id,mdc_nelmts,rdcc_nelmts,rdcc_nbytes,rdcc_w0);

    /* Check arguments */
    if (mdc_nelmts<0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "meta data cache size must be non-negative");
    if (rdcc_w0<0.0 || rdcc_w0>1.0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "raw data cache w0 value must be between 0.0 and 1.0 inclusive");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set sizes */
    if(H5P_set(plist, H5F_ACS_META_CACHE_SIZE_NAME, &mdc_nelmts) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set meta data cache size");
    if(H5P_set(plist, H5F_ACS_DATA_CACHE_ELMT_SIZE_NAME, &rdcc_nelmts) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set data cache element size");
    if(H5P_set(plist, H5F_ACS_DATA_CACHE_BYTE_SIZE_NAME, &rdcc_nbytes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set data cache byte size");
    if(H5P_set(plist, H5F_ACS_PREEMPT_READ_CHUNKS_NAME, &rdcc_w0) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET,FAIL, "can't set preempt read chunks");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pget_cache
 *
 * Purpose:  Retrieves the maximum possible number of elements in the meta
 *    data cache and the maximum possible number of elements and
 *    bytes and the RDCC_W0 value in the raw data chunk cache.  Any
 *    (or all) arguments may be null pointers in which case the
 *    corresponding datum is not returned.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, May 19, 1998
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_cache(hid_t plist_id, int *mdc_nelmts,
       size_t *rdcc_nelmts, size_t *rdcc_nbytes, double *rdcc_w0)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_cache, FAIL);
    H5TRACE5("e","i*Is*z*z*d",plist_id,mdc_nelmts,rdcc_nelmts,rdcc_nbytes,
             rdcc_w0);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get sizes */
    if (mdc_nelmts)
        if(H5P_get(plist, H5F_ACS_META_CACHE_SIZE_NAME, mdc_nelmts) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get meta data cache size");
    if (rdcc_nelmts)
        if(H5P_get(plist, H5F_ACS_DATA_CACHE_ELMT_SIZE_NAME, rdcc_nelmts) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache element size");
    if (rdcc_nbytes)
        if(H5P_get(plist, H5F_ACS_DATA_CACHE_BYTE_SIZE_NAME, rdcc_nbytes) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache byte size");
    if (rdcc_w0)
        if(H5P_get(plist, H5F_ACS_PREEMPT_READ_CHUNKS_NAME, rdcc_w0) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get preempt read chunks");

done:
    FUNC_LEAVE_API(ret_value);
}
#endif /* H5_WANT_H5_V1_4_COMPAT */


/*-------------------------------------------------------------------------
 * Function:  H5Pset_gc_references
 *
 * Purpose:  Sets the flag for garbage collecting references for the file.
 *    Dataset region references (and other reference types
 *    probably) use space in the file heap.  If garbage collection
 *    is on and the user passes in an uninitialized value in a
 *    reference structure, the heap might get corrupted.  When
 *    garbage collection is off however and the user re-uses a
 *    reference, the previous heap block will be orphaned and not
 *    returned to the free heap space.  When garbage collection is
 *    on, the user must initialize the reference structures to 0 or
 *    risk heap corruption.
 *
 *    Default value for garbage collecting references is off, just
 *    to be on the safe side.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    June, 1999
 *
 * Modifications:
 *
 *    Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_gc_references(hid_t plist_id, unsigned gc_ref)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_gc_references, FAIL);
    H5TRACE2("e","iIu",plist_id,gc_ref);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_ACS_GARBG_COLCT_REF_NAME, &gc_ref) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set garbage collect reference");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pget_gc_references
 *
 * Purpose:  Returns the current setting for the garbage collection
 *    references property from a file access property list.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              June, 1999
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_gc_references(hid_t plist_id, unsigned *gc_ref/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_gc_references, FAIL);
    H5TRACE2("e","ix",plist_id,gc_ref);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (gc_ref)
        if(H5P_get(plist, H5F_ACS_GARBG_COLCT_REF_NAME, gc_ref) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get garbage collect reference");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fclose_degree
 *
 * Purpose:     Sets the degree for the file close behavior.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              November, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fclose_degree(hid_t plist_id, H5F_close_degree_t degree)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_fclose_degree, FAIL);
    H5TRACE2("e","iFd",plist_id,degree);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_CLOSE_DEGREE_NAME, &degree) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set file close degree");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_fclose_degree
 *
 * Purpose:     Returns the degree for the file close behavior.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              November, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5Pget_fclose_degree(hid_t plist_id, H5F_close_degree_t *degree)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_fclose_degree, FAIL);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if( degree && (H5P_get(plist, H5F_CLOSE_DEGREE_NAME, degree) < 0) )
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get file close degree");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pset_meta_block_size
 *
 * Purpose:  Sets the minimum size of metadata block allocations when
 *      the H5FD_FEAT_AGGREGATE_METADATA is set by a VFL driver.
 *      Each "raw" metadata block is allocated to be this size and then
 *      specific pieces of metadata (object headers, local heaps, B-trees, etc)
 *      are sub-allocated from this block.
 *
 *    The default value is set to 2048 (bytes), indicating that metadata
 *      will be attempted to be bunched together in (at least) 2K blocks in
 *      the file.  Setting the value to 0 with this API function will
 *      turn off the metadata aggregation, even if the VFL driver attempts to
 *      use that strategy.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_meta_block_size(hid_t plist_id, hsize_t size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_meta_block_size, FAIL);
    H5TRACE2("e","ih",plist_id,size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_ACS_META_BLOCK_SIZE_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set meta data block size");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Pget_meta_block_size
 *
 * Purpose:  Returns the current settings for the metadata block allocation
 *      property from a file access property list.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 29, 2000
 *
 * Modifications:
 *
 *    Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_meta_block_size(hid_t plist_id, hsize_t *size/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_meta_block_size, FAIL);
    H5TRACE2("e","ix",plist_id,size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (size) {
        if(H5P_get(plist, H5F_ACS_META_BLOCK_SIZE_NAME, size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get meta data block size");
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value);
}

#ifdef H5_WANT_H5_V1_4_COMPAT

/*-------------------------------------------------------------------------
 * Function:  H5Pset_sieve_buf_size
 *
 * Purpose:  Sets the maximum size of the data seive buffer used for file
 *      drivers which are capable of using data sieving.  The data sieve
 *      buffer is used when performing I/O on datasets in the file.  Using a
 *      buffer which is large anough to hold several pieces of the dataset
 *      being read in for hyperslab selections boosts performance by quite a
 *      bit.
 *
 *    The default value is set to 64KB, indicating that file I/O for raw data
 *      reads and writes will occur in at least 64KB blocks.
 *      Setting the value to 0 with this API function will turn off the
 *      data sieving, even if the VFL driver attempts to use that strategy.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, September 21, 2000
 *
 * Modifications:
 *
 *    Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_sieve_buf_size(hid_t plist_id, hsize_t _size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t size=(size_t)_size;  /* Work around size difference */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_sieve_buf_size, FAIL);
    H5TRACE2("e","ih",plist_id,_size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_ACS_SIEVE_BUF_SIZE_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set sieve buffer size");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_sieve_buf_size() */


/*-------------------------------------------------------------------------
 * Function:  H5Pget_sieve_buf_size
 *
 * Purpose:  Returns the current settings for the data sieve buffer size
 *      property from a file access property list.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, September 21, 2000
 *
 * Modifications:
 *
 *    Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_sieve_buf_size(hid_t plist_id, hsize_t *_size/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t size;                /* Work around size difference */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_sieve_buf_size, FAIL);
    H5TRACE2("e","ix",plist_id,_size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (_size) {
        if(H5P_get(plist, H5F_ACS_SIEVE_BUF_SIZE_NAME, &size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get sieve buffer size");
        *_size=size;
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_sieve_buf_size() */
#else /* H5_WANT_H5_V1_4_COMPAT */

/*-------------------------------------------------------------------------
 * Function:  H5Pset_sieve_buf_size
 *
 * Purpose:  Sets the maximum size of the data seive buffer used for file
 *      drivers which are capable of using data sieving.  The data sieve
 *      buffer is used when performing I/O on datasets in the file.  Using a
 *      buffer which is large anough to hold several pieces of the dataset
 *      being read in for hyperslab selections boosts performance by quite a
 *      bit.
 *
 *    The default value is set to 64KB, indicating that file I/O for raw data
 *      reads and writes will occur in at least 64KB blocks.
 *      Setting the value to 0 with this API function will turn off the
 *      data sieving, even if the VFL driver attempts to use that strategy.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, September 21, 2000
 *
 * Modifications:
 *
 *    Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_sieve_buf_size(hid_t plist_id, size_t size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_sieve_buf_size, FAIL);
    H5TRACE2("e","iz",plist_id,size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_ACS_SIEVE_BUF_SIZE_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set sieve buffer size");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_sieve_buf_size() */


/*-------------------------------------------------------------------------
 * Function:  H5Pget_sieve_buf_size
 *
 * Purpose:  Returns the current settings for the data sieve buffer size
 *      property from a file access property list.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, September 21, 2000
 *
 * Modifications:
 *
 *    Raymond Lu
 *     Tuesday, Oct 23, 2001
 *    Changed the file access list to the new generic property
 *    list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_sieve_buf_size(hid_t plist_id, size_t *size/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_sieve_buf_size, FAIL);
    H5TRACE2("e","ix",plist_id,size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (size)
        if(H5P_get(plist, H5F_ACS_SIEVE_BUF_SIZE_NAME, size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get sieve buffer size");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_sieve_buf_size() */
#endif /* H5_WANT_H5_V1_4_COMPAT */


/*-------------------------------------------------------------------------
 * Function:  H5Pset_small_data_block_size
 *
 * Purpose:  Sets the minimum size of "small" raw data block allocations
 *      when the H5FD_FEAT_AGGREGATE_SMALLDATA is set by a VFL driver.
 *      Each "small" raw data block is allocated to be this size and then
 *      pieces of raw data which are small enough to fit are sub-allocated from
 *      this block.
 *
 *  The default value is set to 2048 (bytes), indicating that raw data
 *      smaller than this value will be attempted to be bunched together in (at
 *      least) 2K blocks in the file.  Setting the value to 0 with this API
 *      function will turn off the "small" raw data aggregation, even if the
 *      VFL driver attempts to use that strategy.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 5, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_small_data_block_size(hid_t plist_id, hsize_t size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_small_data_block_size, FAIL);
    H5TRACE2("e","ih",plist_id,size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5F_ACS_SDATA_BLOCK_SIZE_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set 'small data' block size");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_small_data_block_size() */


/*-------------------------------------------------------------------------
 * Function:  H5Pget_small_data_block_size
 *
 * Purpose:  Returns the current settings for the "small" raw data block
 *      allocation property from a file access property list.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 5, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_small_data_block_size(hid_t plist_id, hsize_t *size/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_small_data_block_size, FAIL);
    H5TRACE2("e","ix",plist_id,size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (size) {
        if(H5P_get(plist, H5F_ACS_META_BLOCK_SIZE_NAME, size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get 'small data' block size");
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_small_data_block_size() */

