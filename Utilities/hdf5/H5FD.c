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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, July 26, 1999
 *
 * Purpose:  The Virtual File Layer as described in documentation.
 *              This is the greatest common denominator for all types of
 *              storage access whether a file, memory, network, etc. This
 *              layer usually just dispatches the request to an actual
 *              file driver layer.
 */

#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5FD_init_interface


/* Packages needed by this file */
#include "H5private.h"    /* Generic Functions      */
#include "H5Dprivate.h"    /* Datasets        */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fpkg.h"             /* File access        */
#include "H5FDprivate.h"  /* File drivers        */
#include "H5FDcore.h"    /* Files stored entirely in memory  */
#include "H5FDfamily.h"    /* File families       */
#include "H5FDgass.h"    /* Remote files using GASS I/O    */
#include "H5FDlog.h"          /* sec2 driver with I/O logging (for debugging) */
#include "H5FDmpi.h"            /* MPI-based file drivers    */
#include "H5FDmulti.h"    /* Usage-partitioned file family  */
#include "H5FDsec2.h"    /* POSIX unbuffered file I/O    */
#include "H5FDsrb.h"          /* Remote access using SRB              */
#include "H5FDstdio.h"    /* Standard C buffered I/O    */
#include "H5FDstream.h"       /* In-memory files streamed via sockets */
#include "H5FLprivate.h"  /* Free lists                           */
#include "H5Iprivate.h"    /* IDs            */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Pprivate.h"    /* Property lists      */

/* static prototypes */
static herr_t H5FD_pl_copy(void *(*copy_func)(const void *), size_t pl_size,
    const void *old_pl, void **copied_pl);
static herr_t H5FD_pl_close(hid_t driver_id, herr_t (*free_func)(void *),
    void *pl);
static herr_t H5FD_free_cls(H5FD_class_t *cls);
static haddr_t H5FD_alloc_from_free_list(H5FD_t *file, H5FD_mem_t type,
                                         H5FD_mem_t mapped_type, hsize_t size);
static haddr_t H5FD_alloc_metadata(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                                   hsize_t size);
static haddr_t H5FD_alloc_raw(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                              hsize_t size);
static haddr_t H5FD_real_alloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
static haddr_t H5FD_update_eoa(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);

/* Declare a free list to manage the H5FD_free_t struct */
H5FL_DEFINE(H5FD_free_t);

/* Declare a PQ free list to manage the metadata accumulator buffer */
H5FL_BLK_DEFINE_STATIC(meta_accum);

/* Local macro definitions */
#define H5FD_ACCUM_THROTTLE     8
#define H5FD_ACCUM_THRESHOLD    2048

/* Static local variables */

/*
 * Global count of the number of H5FD_t's handed out.  This is used as a
 * "serial number" for files that are currently open and is used for the
 * 'fileno[2]' field in H5G_stat_t.  However, if a VFL driver is not able
 * to detect whether two files are the same, a file that has been opened
 * by H5Fopen more than once with that VFL driver will have two different
 * serial numbers.  :-/
 *
 * Also, if a file is opened, the 'fileno[2]' field is retrieved for an
 * object and the file is closed and re-opened, the 'fileno[2]' value will
 * be different.
 */
static unsigned long file_serial_no[2];


/*-------------------------------------------------------------------------
 * Function:  H5FD_init_interface
 *
 * Purpose:  Initialize the virtual file layer.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, July 26, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_init_interface(void)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FD_init_interface)

    if (H5I_init_group(H5I_VFL, H5I_VFL_HASHSIZE, 0, (H5I_free_t)H5FD_free_cls)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Reset the file serial numbers */
    HDmemset(file_serial_no,0,sizeof(file_serial_no));

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_term_interface
 *
 * Purpose:  Terminate this interface: free all memory and reset global
 *    variables to their initial values.  Release all ID groups
 *    associated with this interface.
 *
 * Return:  Success:  Positive if anything was done that might
 *        have affected other interfaces; zero
 *        otherwise.
 *
 *    Failure:        Never fails.
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_term_interface(void)
{
    int  n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_term_interface)

    if (H5_interface_initialize_g) {
  if ((n=H5I_nmembers(H5I_VFL))!=0) {
      H5I_clear_group(H5I_VFL, FALSE);

            /* Reset the VFL drivers, if they've been closed */
            if(H5I_nmembers(H5I_VFL)==0) {
                H5FD_sec2_term();
                H5FD_log_term();
                H5FD_stdio_term();
                H5FD_family_term();
#ifdef H5_HAVE_GASS
                H5FD_gass_term();
#endif
#ifdef H5_HAVE_SRB
                H5FD_srb_term();
#endif
                H5FD_core_term();
                H5FD_multi_term();
#ifdef H5_HAVE_PARALLEL
                H5FD_mpio_term();
                H5FD_mpiposix_term();
#endif /* H5_HAVE_PARALLEL */
#ifdef H5_HAVE_STREAM
                H5FD_stream_term();
#endif
            } /* end if */
  } else {
      H5I_destroy_group(H5I_VFL);
      H5_interface_initialize_g = 0;
      n = 1; /*H5I*/
  }
    }
    FUNC_LEAVE_NOAPI(n)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_free_cls
 *
 * Purpose:  Frees a file driver class struct and returns an indication of
 *    success. This function is used as the free callback for the
 *    virtual file layer object identifiers (cf H5FD_init_interface).
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, July 26, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_free_cls(H5FD_class_t *cls)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_free_cls)

    H5MM_xfree(cls);

    FUNC_LEAVE_NOAPI(SUCCEED)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDregister
 *
 * Purpose:  Registers a new file driver as a member of the virtual file
 *    driver class.  Certain fields of the class struct are
 *    required and that is checked here so it doesn't have to be
 *    checked every time the field is accessed.
 *
 * Return:  Success:  A file driver ID which is good until the
 *        library is closed or the driver is
 *        unregistered.
 *
 *    Failure:  A negative value.
 *
 * Programmer:  Robb Matzke
 *              Monday, July 26, 1999
 *
 * Modifications:
 *              Copied guts of function info H5FD_register
 *              Quincey Koziol
 *              Friday, January 30, 2004
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FDregister(const H5FD_class_t *cls)
{
    hid_t    ret_value;
    H5FD_mem_t    type;

    FUNC_ENTER_API(H5FDregister, FAIL)
    H5TRACE1("i","x",cls);

    /* Check arguments */
    if (!cls)
  HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL, "null class pointer is disallowed")
    if (!cls->open || !cls->close)
  HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL, "`open' and/or `close' methods are not defined")
    if (!cls->get_eoa || !cls->set_eoa)
  HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL, "`get_eoa' and/or `set_eoa' methods are not defined")
    if (!cls->get_eof)
  HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL, "`get_eof' method is not defined")
    if (!cls->read || !cls->write)
  HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL, "`read' and/or `write' method is not defined")
    for (type=H5FD_MEM_DEFAULT; type<H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t,type))
  if (cls->fl_map[type]<H5FD_MEM_NOLIST || cls->fl_map[type]>=H5FD_MEM_NTYPES)
      HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid free-list mapping")

    /* Create the new class ID */
    if ((ret_value=H5FD_register(cls, sizeof(H5FD_class_t)))<0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register file driver ID")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDregister() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_register
 *
 * Purpose:  Registers a new file driver as a member of the virtual file
 *    driver class.  Certain fields of the class struct are
 *    required and that is checked here so it doesn't have to be
 *    checked every time the field is accessed.
 *
 * Return:  Success:  A file driver ID which is good until the
 *        library is closed or the driver is
 *        unregistered.
 *
 *    Failure:  A negative value.
 *
 * Programmer:  Robb Matzke
 *              Monday, July 26, 1999
 *
 * Modifications:
 *              Broke into public and internal routines & added 'size'
 *              parameter to internal routine, which allows us to create
 *              sub-classes of H5FD_class_t for internal support (see the
 *              MPI drivers, etc.)
 *              Quincey Koziol
 *              January 30, 2004
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_register(const void *_cls, size_t size)
{
    hid_t    ret_value;
    const H5FD_class_t  *cls=(const H5FD_class_t *)_cls;
    H5FD_class_t  *saved=NULL;
    H5FD_mem_t    type;

    FUNC_ENTER_NOAPI(H5FD_register, FAIL)

    /* Check arguments */
    assert(cls);
    assert(cls->open && cls->close);
    assert(cls->get_eoa && cls->set_eoa);
    assert(cls->get_eof);
    assert(cls->read && cls->write);
    for (type=H5FD_MEM_DEFAULT; type<H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t,type))
        assert(cls->fl_map[type]>=H5FD_MEM_NOLIST && cls->fl_map[type]<H5FD_MEM_NTYPES);

    /* Copy the class structure so the caller can reuse or free it */
    if (NULL==(saved=H5MM_malloc(size)))
  HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for file driver class struct")
    HDmemcpy(saved,cls,size);

    /* Create the new class ID */
    if ((ret_value=H5I_register(H5I_VFL, saved))<0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register file driver ID")

done:
    if(ret_value<0)
        if(saved)
            H5MM_xfree(saved);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_register() */


/*-------------------------------------------------------------------------
 * Function:  H5FDunregister
 *
 * Purpose:  Removes a driver ID from the library. This in no way affects
 *    file access property lists which have been defined to use
 *    this driver or files which are already opened under this
 *    driver.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, July 26, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDunregister(hid_t driver_id)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDunregister, FAIL)
    H5TRACE1("e","i",driver_id);

    /* Check arguments */
    if (NULL==H5I_object_verify(driver_id,H5I_VFL))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file driver")

    /* The H5FD_class_t struct will be freed by this function */
    if (H5I_dec_ref(driver_id)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "unable to unregister file driver")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_get_class
 *
 * Purpose:  Optains a pointer to the driver struct containing all the
 *    callback pointers, etc. The PLIST_ID argument can be a file
 *    access property list, a data transfer property list, or a
 *    file driver identifier.
 *
 * Return:  Success:  Ptr to the driver information. The pointer is
 *        only valid as long as the driver remains
 *        registered or some file or property list
 *        exists which references the driver.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, August 20, 1999
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
H5FD_class_t *
H5FD_get_class(hid_t id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5FD_class_t  *ret_value=NULL;
    hid_t               driver_id = -1;

    FUNC_ENTER_NOAPI(H5FD_get_class, NULL)

    if (H5I_VFL==H5I_get_type(id)) {
  ret_value = H5I_object(id);
    } else {
        /* Get the plist structure */
        if(NULL == (plist = H5I_object(id)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, NULL, "can't find object for ID")

        if (TRUE==H5P_isa_class(id,H5P_FILE_ACCESS)) {
            if(H5P_get(plist, H5F_ACS_FILE_DRV_ID_NAME, &driver_id) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get driver ID")
            ret_value = H5FD_get_class(driver_id);
        } else if (TRUE==H5P_isa_class(id,H5P_DATASET_XFER)) {
            if(H5P_get(plist, H5D_XFER_VFL_ID_NAME, &driver_id) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get driver ID")
            ret_value = H5FD_get_class(driver_id);
        } else {
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a driver id, file access property list or data transfer property list")
        }
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_sb_size
 *
 * Purpose:  Obtains the number of bytes required to store the driver file
 *    access data in the HDF5 superblock.
 *
 * Return:  Success:  Number of bytes required.
 *
 *    Failure:  0 if an error occurs or if the driver has no
 *        data to store in the superblock.
 *
 * Programmer:  Robb Matzke
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5FD_sb_size(H5FD_t *file)
{
    hsize_t  ret_value=0;

    FUNC_ENTER_NOAPI(H5FD_sb_size, 0)

    assert(file && file->cls);

    if (file->cls->sb_size)
  ret_value = (file->cls->sb_size)(file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_sb_encode
 *
 * Purpose:  Encode driver-specific data into the output arguments. The
 *    NAME is a nine-byte buffer which should get an
 *    eight-character driver name and/or version followed by a null
 *    terminator. The BUF argument is a buffer to receive the
 *    encoded driver-specific data. The size of the BUF array is
 *    the size returned by the H5FD_sb_size() call.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_sb_encode(H5FD_t *file, char *name/*out*/, uint8_t *buf)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_sb_encode, FAIL)

    assert(file && file->cls);
    if (file->cls->sb_encode &&
            (file->cls->sb_encode)(file, name/*out*/, buf/*out*/)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver sb_encode request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_sb_decode
 *
 * Purpose:  Decodes the driver information block.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_sb_decode(H5FD_t *file, const char *name, const uint8_t *buf)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_sb_decode, FAIL)

    assert(file && file->cls);
    if (file->cls->sb_decode &&
            (file->cls->sb_decode)(file, name, buf)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver sb_decode request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_pl_copy
 *
 * Purpose:  Copies the driver-specific part of the a property list.
 *              This is common code, used by both the dataset transfer and
 *              file access property list routines.
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, October 23, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_pl_copy(void *(*copy_func)(const void *), size_t pl_size, const void *old_pl, void **copied_pl)
{
    void *new_pl = NULL;        /* Copy of property list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FD_pl_copy)

    /* Copy old pl, if one exists */
    if (old_pl) {
        /* Allow the driver to copy or do it ourselves */
        if (copy_func) {
            new_pl = (copy_func)(old_pl);
        } else if (pl_size>0) {
            if((new_pl = H5MM_malloc(pl_size))==NULL)
                HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "property list allocation failed")
            HDmemcpy(new_pl, old_pl, pl_size);
        } else
            HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL, "no way to copy driver property list")
    } /* end if */

    /* Set copied value */
    *copied_pl=new_pl;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_pl_copy() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_pl_close
 *
 * Purpose:  Closes a driver for a property list
 *              This is common code, used by both the dataset transfer and
 *              file access property list routines.
 *
 * Return:  Success:  non-negative
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, October 23, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_pl_close(hid_t driver_id, herr_t (*free_func)(void *), void *pl)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FD_pl_close)

    /* Allow driver to free or do it ourselves */
    if (pl && free_func) {
  if ((free_func)(pl)<0)
      HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver free request failed")
    } else
  H5MM_xfree(pl);

    /* Decrement reference count for driver */
    if(H5I_dec_ref(driver_id)<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTDEC, FAIL, "can't decrement reference count for driver")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_pl_close() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_fapl_get
 *
 * Purpose:  Gets the file access property list associated with a file.
 *    Usually the file will copy what it needs from the original
 *    file access property list when the file is created. The
 *    purpose of this function is to create a new file access
 *    property list based on the settings in the file, which may
 *    have been modified from the original file access property
 *    list.
 *
 * Return:  Success:  Pointer to a new file access property list
 *        with all members copied.  If the file is
 *        closed then this property list lives on, and
 *        vice versa.
 *
 *    Failure:  NULL, including when the file has no
 *        properties.
 *
 * Programmer:  Robb Matzke
 *              Friday, August 13, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5FD_fapl_get(H5FD_t *file)
{
    void  *ret_value=NULL;

    FUNC_ENTER_NOAPI(H5FD_fapl_get, NULL)

    assert(file);

    if (file->cls->fapl_get)
  ret_value = (file->cls->fapl_get)(file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_fapl_get() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_fapl_open
 *
 * Purpose:  Mark a driver as used by a file access property list
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, October 23, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fapl_open(H5P_genplist_t *plist, hid_t driver_id, const void *driver_info)
{
    void *copied_driver_info;           /* Temporary VFL driver info */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_fapl_open, FAIL)

    /* Increment the reference count on driver and copy driver info */
    if(H5I_inc_ref(driver_id)<0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINC, FAIL, "unable to increment ref count on VFL driver")
    if(H5FD_fapl_copy(driver_id, driver_info, &copied_driver_info)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy VFL driver info")

    /* Set the driver properties for the list */
    if(H5P_set(plist, H5F_ACS_FILE_DRV_ID_NAME, &driver_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver ID")
    if(H5P_set(plist, H5F_ACS_FILE_DRV_INFO_NAME, &copied_driver_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_fapl_open() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_fapl_copy
 *
 * Purpose:  Copies the driver-specific part of the file access property
 *    list.
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fapl_copy(hid_t driver_id, const void *old_fapl, void **copied_fapl)
{
    H5FD_class_t  *driver=NULL;
    herr_t ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_fapl_copy, FAIL)

    /* Check args */
    if (NULL==(driver=H5I_object(driver_id)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID")

    /* Copy the file access property list */
    if(H5FD_pl_copy(driver->fapl_copy,driver->fapl_size,old_fapl,copied_fapl)<0)
        HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL, "can't copy driver file access property list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_fapl_close
 *
 * Purpose:  Closes a driver for a dataset transfer property list
 *
 * Return:  Success:  non-negative
 *    Failure:  negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fapl_close(hid_t driver_id, void *fapl)
{
    H5FD_class_t  *driver=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_fapl_close, FAIL)

    /* Check args */
    if(driver_id>0) {
        if (NULL==(driver=H5I_object(driver_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID")

        /* Close the driver for the property list */
        if(H5FD_pl_close(driver_id,driver->fapl_free,fapl)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver fapl_free request failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_fapl_close() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_dxpl_open
 *
 * Purpose:  Mark a driver as used by a data transfer property list
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, October 23, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_dxpl_open(H5P_genplist_t *plist, hid_t driver_id, const void *driver_info)
{
    void *copied_driver_info;           /* Temporary VFL driver info */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_dxpl_open, FAIL)

    /* Increment the reference count on the driver and copy the driver info */
    if(H5I_inc_ref(driver_id)<0)
        HGOTO_ERROR (H5E_DATASET, H5E_CANTINC, FAIL, "can't increment VFL driver ID")
    if(H5FD_dxpl_copy(driver_id, driver_info, &copied_driver_info)<0)
        HGOTO_ERROR (H5E_DATASET, H5E_CANTCOPY, FAIL, "can't copy VFL driver")

    /* Set the driver information for the new property list */
    if(H5P_set(plist, H5D_XFER_VFL_ID_NAME, &driver_id)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "can't set VFL driver ID")
    if(H5P_set(plist, H5D_XFER_VFL_INFO_NAME, &copied_driver_info)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "can't set VFL driver info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_dxpl_open() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_dxpl_copy
 *
 * Purpose:  Copies the driver-specific part of the data transfer property
 *    list.
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_dxpl_copy(hid_t driver_id, const void *old_dxpl, void **copied_dxpl)
{
    H5FD_class_t  *driver=NULL;
    herr_t ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_dxpl_copy, FAIL)

    /* Check args */
    if (NULL==(driver=H5I_object(driver_id)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID")

    /* Copy the file access property list */
    if(H5FD_pl_copy(driver->dxpl_copy,driver->dxpl_size,old_dxpl,copied_dxpl)<0)
        HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL, "can't copy driver data transfer property list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_dxpl_copy() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_dxpl_close
 *
 * Purpose:  Closes a driver for a dataset transfer property list
 *
 * Return:  Success:  non-negative
 *    Failure:  negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_dxpl_close(hid_t driver_id, void *dxpl)
{
    H5FD_class_t  *driver=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_dxpl_close, FAIL)

    /* Check args */
    if(driver_id>0) {
        if (NULL==(driver=H5I_object(driver_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID")

        /* Close the driver for the property list */
        if(H5FD_pl_close(driver_id,driver->dxpl_free,dxpl)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver fapl_free request failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_dxpl_close() */


/*-------------------------------------------------------------------------
 * Function:  H5FDopen
 *
 * Purpose:  Opens a file named NAME for the type(s) of access described
 *    by the bit vector FLAGS according to a file access property
 *    list FAPL_ID (which may be the constant H5P_DEFAULT). The
 *    file should expect to handle format addresses in the range [0,
 *    MAXADDR] (if MAXADDR is the undefined address then the caller
 *    doesn't care about the address range).
 *
 *     Possible values for the FLAGS bits are:
 *
 *    H5F_ACC_RDWR:  Open the file for read and write access. If
 *        this bit is not set then open the file for
 *        read only access. It is permissible to open a
 *        file for read and write access when only read
 *        access is requested by the library (the
 *        library will never attempt to write to a file
 *        which it opened with only read access).
 *
 *    H5F_ACC_CREATE:  Create the file if it doesn't already exist.
 *        However, see H5F_ACC_EXCL below.
 *
 *    H5F_ACC_TRUNC:  Truncate the file if it already exists. This
 *        is equivalent to deleting the file and then
 *        creating a new empty file.
 *
 *    H5F_ACC_EXCL:  When used with H5F_ACC_CREATE, if the file
 *        already exists then the open should fail.
 *        Note that this is unsupported/broken with
 *        some file drivers (e.g., sec2 across nfs) and
 *        will contain a race condition when used to
 *        perform file locking.
 *
 *    The MAXADDR is the maximum address which will be requested by
 *    the library during an allocation operation. Usually this is
 *    the same value as the MAXADDR field of the class structure,
 *    but it can be smaller if the driver is being used under some
 *    other driver.
 *
 *    Note that when the driver `open' callback gets control that
 *    the public part of the file struct (the H5FD_t part) will be
 *    incomplete and will be filled in after that callback returns.
 *
 * Return:  Success:  Pointer to a new file driver struct.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 27, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5FD_t *
H5FDopen(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_t  *ret_value=NULL;

    FUNC_ENTER_API(H5FDopen, NULL)

    /* Check arguments */
    if(H5P_DEFAULT == fapl_id)
        fapl_id = H5P_FILE_ACCESS_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(fapl_id,H5P_FILE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")

    if (NULL==(ret_value=H5FD_open(name, flags, fapl_id, maxaddr)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to open file")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_open
 *
 * Purpose:  Private version of H5FDopen()
 *
 * Return:  Success:  Pointer to a new file driver struct
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
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
H5FD_t *
H5FD_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_class_t  *driver;
    H5FD_t    *file=NULL;
    hid_t               driver_id = -1;
    hsize_t             meta_block_size=0;
    hsize_t             sdata_block_size=0;
    H5P_genplist_t *plist;      /* Property list pointer */
    H5FD_t    *ret_value;

    FUNC_ENTER_NOAPI(H5FD_open, NULL)

    /* Get file access property list */
    if(NULL == (plist = H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")

    if (0==maxaddr)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "zero format address range")

    if(H5P_get(plist, H5F_ACS_FILE_DRV_ID_NAME, &driver_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get driver ID")

    /* Get driver info */
    if (NULL==(driver=H5I_object(driver_id)))
  HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "invalid driver ID in file access property list")
    if (NULL==driver->open)
  HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, NULL, "file driver has no `open' method")

    /* Dispatch to file driver */
    if (HADDR_UNDEF==maxaddr)
        maxaddr = driver->maxaddr;
    if (NULL==(file=(driver->open)(name, flags, fapl_id, maxaddr)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "open failed")

    /*
     * Fill in public fields. We must increment the reference count on the
     * driver ID to prevent it from being freed while this file is open.
     */
    file->driver_id = driver_id;
    if(H5I_inc_ref(file->driver_id)<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINC, NULL, "unable to increment ref count on VFL driver")
    file->cls = driver;
    file->maxaddr = maxaddr;
    file->reserved_alloc = 0;
    HDmemset(file->fl, 0, sizeof(file->fl));
    if(H5P_get(plist, H5F_ACS_META_BLOCK_SIZE_NAME, &(meta_block_size)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get meta data block size")
    file->def_meta_block_size = meta_block_size;
    if(H5P_get(plist, H5F_ACS_SDATA_BLOCK_SIZE_NAME, &(sdata_block_size)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get 'small data' block size")
    file->def_sdata_block_size = sdata_block_size;
    file->accum_loc = HADDR_UNDEF;
    if(H5P_get(plist, H5F_ACS_ALIGN_THRHD_NAME, &(file->threshold)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get alignment threshold")
    if(H5P_get(plist, H5F_ACS_ALIGN_NAME, &(file->alignment)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get alignment")

    /* Retrieve the VFL driver feature flags */
    if (H5FD_query(file, &(file->feature_flags))<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to query file driver")

    /* Increment the global serial number & assign it to this H5FD_t object */
    if(++file_serial_no[0]==0) {
        /* (Just error out if we wrap both numbers around for now...) */
        if(++file_serial_no[1]==0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to get file serial number")
    } /* end if */
    HDmemcpy(file->fileno,file_serial_no,sizeof(file_serial_no));

    /* Set return value */
    ret_value=file;

done:
    /* Can't cleanup 'file' information, since we don't know what type it is */
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDclose
 *
 * Purpose:     Closes the file by calling the driver `close' callback, which
 *    should free all driver-private data and free the file struct.
 *    Note that the public part of the file struct (the H5FD_t part)
 *    will be all zero during the driver close callback like during
 *    the `open' callback.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 27, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDclose(H5FD_t *file)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDclose, FAIL)
    H5TRACE1("e","x",file);

    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer")

    if (H5FD_close(file)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "unable to close file")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_close
 *
 * Purpose:  Private version of H5FDclose()
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *              Robb Matzke, 2000-11-10
 *              Removed a call to set *file to all zero because the struct
 *              has already been freed by the close method. This fixes a write
 *              to freed memory.
 *
 *              Bill Wendling, 2003-02-17
 *              Split out the freeing of the freelist from this function
 *              so that the Flexible PHDF5 stuff can call it without
 *              having to call H5FD_close().
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_close(H5FD_t *file)
{
    const H5FD_class_t *driver;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_close, FAIL)

    /* check args */
    assert(file && file->cls);

    /* Free the freelist (this call never fails) */
    (void)H5FD_free_freelist(file);

    /* Prepare to close file by clearing all public fields */
    driver = file->cls;
    if(H5I_dec_ref(file->driver_id)<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTDEC, FAIL, "can't close driver ID")

    /*
     * Dispatch to the driver for actual close. If the driver fails to
     * close the file then the file will be in an unusable state.
     */
    assert(driver->close);

    if ((driver->close)(file) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_free_freelist
 * Purpose:     Split off from H5FD_close(). Free the elements in the
 *              free list for this file driver.
 * Return:      Success:    SUCCEED
 *              Failure:    Never fails
 * Programmer:  Bill Wendling
 *              17. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_free_freelist(H5FD_t *file)
{
    H5FD_mem_t  i;
#ifdef H5F_DEBUG
    unsigned    nblocks = 0;
    hsize_t     nbytes = 0;
#endif  /* H5F_DEBUG */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_free_freelist, FAIL)

    /* check args */
    assert(file && file->cls);

    /*
     * Free all free-lists, leaking any memory thus described. Also leaks
     * file space allocated but not used when metadata aggregation is
     * turned on.
     */
    for (i = H5FD_MEM_DEFAULT; i < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, i)) {
        H5FD_free_t *cur, *next;

        for (cur = file->fl[i]; cur; cur = next) {
#ifdef H5F_DEBUG
            ++nblocks;
            nbytes += cur->size;
#endif  /* H5F_DEBUG */
            next = cur->next;
            H5FL_FREE(H5FD_free_t, cur);
        }

        file->fl[i] = NULL;
    }

#ifdef H5F_DEBUG
    if (nblocks && H5DEBUG(F))
        HDfprintf(H5DEBUG(F),
                  "H5F: leaked %Hu bytes of file memory in %u blocks\n",
                  nbytes, nblocks);
#endif  /* H5F_DEBUG */

    /* Check if we need to reset the metadata accumulator information */
    if (file->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) {
        /* Free the buffer */
        if (file->meta_accum)
            file->meta_accum = H5FL_BLK_FREE(meta_accum, file->meta_accum);

        /* Reset the buffer sizes & location */
        file->accum_buf_size = file->accum_size = 0;
        file->accum_loc = HADDR_UNDEF;
        file->accum_dirty = 0;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDcmp
 *
 * Purpose:  Compare the keys of two files using the file driver callback
 *    if the files belong to the same driver, otherwise sort the
 *    files by driver class pointer value.
 *
 * Return:  Success:  A value like strcmp()
 *
 *    Failure:  Must never fail. If both file handles are
 *        invalid then they compare equal. If one file
 *        handle is invalid then it compares less than
 *        the other.  If both files belong to the same
 *        driver and the driver doesn't provide a
 *        comparison callback then the file pointers
 *        themselves are compared.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 27, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FDcmp(const H5FD_t *f1, const H5FD_t *f2)
{
    int  ret_value;

    FUNC_ENTER_API(H5FDcmp, -1) /*return value is arbitrary*/
    H5TRACE2("Is","xx",f1,f2);

    ret_value = H5FD_cmp(f1, f2);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_cmp
 *
 * Purpose:  Private version of H5FDcmp()
 *
 * Return:  Success:  A value like strcmp()
 *
 *    Failure:  Must never fail.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_cmp(const H5FD_t *f1, const H5FD_t *f2)
{
    int  ret_value;

    FUNC_ENTER_NOAPI(H5FD_cmp, -1) /*return value is arbitrary*/

    if ((!f1 || !f1->cls) && (!f2 || !f2->cls))
        HGOTO_DONE(0)
    if (!f1 || !f1->cls)
        HGOTO_DONE(-1)
    if (!f2 || !f2->cls)
        HGOTO_DONE(1)
    if (f1->cls < f2->cls)
        HGOTO_DONE(-1)
    if (f1->cls > f2->cls)
        HGOTO_DONE(1)

    /* Files are same driver; no cmp callback */
    if (!f1->cls->cmp) {
  if (f1<f2)
            HGOTO_DONE(-1)
  if (f1>f2)
            HGOTO_DONE(1)
  HGOTO_DONE(0)
    }

    ret_value = (f1->cls->cmp)(f1, f2);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDquery
 *
 * Purpose:  Query a VFL driver for its feature flags. (listed in H5FDpublic.h)
 *
 * Return:  Success:    non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FDquery(const H5FD_t *f, unsigned long *flags/*out*/)
{
    int  ret_value;

    FUNC_ENTER_API(H5FDquery, FAIL)
    H5TRACE2("Is","xx",f,flags);

    assert(f);
    assert(flags);

    ret_value = H5FD_query(f, flags);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_query
 *
 * Purpose:  Private version of H5FDquery()
 *
 * Return:  Success:    non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_query(const H5FD_t *f, unsigned long *flags/*out*/)
{
    int  ret_value=0;

    FUNC_ENTER_NOAPI(H5FD_query, FAIL)

    assert(f);
    assert(flags);

    /* Check for query driver and call it */
    if (f->cls->query)
        ret_value = (f->cls->query)(f, flags);
    else
        *flags=0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDalloc
 *
 * Purpose:  Allocates SIZE bytes of memory from the FILE. The memory will
 *    be used according to the allocation class TYPE. First we try
 *    to satisfy the request from one of the free lists, according
 *    to the free list map provided by the driver. The free list
 *    array has one entry for each request type and the value of
 *    that array element can be one of four possibilities:
 *
 *          It can be the constant H5FD_MEM_DEFAULT (or zero) which
 *          indicates that the identity mapping is used. In other
 *          words, the request type maps to its own free list.
 *
 *          It can be the request type itself, which has the same
 *          effect as the H5FD_MEM_DEFAULT value above.
 *
 *          It can be the ID for another request type, which
 *          indicates that the free list for the specified type
 *          should be used instead.
 *
 *          It can be the constant H5FD_MEM_NOLIST which means that
 *          no free list should be used for this type of request.
 *
 *    If the request cannot be satisfied from a free list then
 *    either the driver's `alloc' callback is invoked (if one was
 *    supplied) or the end-of-address marker is extended. The
 *    `alloc' callback is always called with the same arguments as
 *     the H5FDalloc().
 *
 * Return:  Success:  The format address of the new file memory.
 *
 *    Failure:  The undefined address HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 27, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDalloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t  ret_value = HADDR_UNDEF;

    FUNC_ENTER_API(H5FDalloc, HADDR_UNDEF)
    H5TRACE4("a","xMtih",file,type,dxpl_id,size);

    /* Check args */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "invalid file pointer")
    if (type<H5FD_MEM_DEFAULT || type>=H5FD_MEM_NTYPES)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "invalid request type")
    if (size==0)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "zero-size request")
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(dxpl_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "not a data transfer property list")

    /* Do the real work */
    if (HADDR_UNDEF==(ret_value=H5FD_alloc(file, type, dxpl_id, size)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "unable to allocate file memory")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_alloc
 * Purpose:     Private version of H5FDalloc().
 *
 * Return:      Success:    The format address of the new file memory.
 *              Failure:    The undefined address HADDR_UNDEF
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 * Modifications:
 *  Albert Cheng, 2001/05/01
 *  Implement the allocation by alignment/threshold.
 *
 *      Bill Wendling, 2002/12/02
 *      Split apart into subfunctions for each separate task.
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_alloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    H5FD_mem_t  mapped_type;
    haddr_t     ret_value = HADDR_UNDEF;

    FUNC_ENTER_NOAPI(H5FD_alloc, HADDR_UNDEF)

    /* check args */
    assert(file);
    assert(file->cls);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    assert(size > 0);


#ifdef H5F_DEBUG
    if (H5DEBUG(F))
  HDfprintf(H5DEBUG(F), "%s: alignment=%Hd, threshold=%Hd, size=%Hd\n",
                  FUNC, file->alignment, file->threshold, size);
#endif  /* H5F_DEBUG */

    /* Map the allocation request to a free list */
    if (H5FD_MEM_DEFAULT == file->cls->fl_map[type])
        mapped_type = type;
    else
        mapped_type = file->cls->fl_map[type];

    /* Try to allocate from the free list first */
    if ((ret_value = H5FD_alloc_from_free_list(file, type,
                                               mapped_type, size)) != HADDR_UNDEF)
        HGOTO_DONE(ret_value)

#ifdef H5F_DEBUG
    if (H5DEBUG(F))
  HDfprintf(H5DEBUG(F), "%s: Could not allocate from freelists\n", FUNC);
#endif  /* H5F_DEBUG */

    if (type != H5FD_MEM_DRAW) {
        /* Handle metadata differently from "raw" data */
        if ((ret_value = H5FD_alloc_metadata(file, type, dxpl_id, size)) == HADDR_UNDEF)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "can't allocate for metadata")
    } else {
        /* Allocate "raw" data */
        if ((ret_value = H5FD_alloc_raw(file, type, dxpl_id, size)) == HADDR_UNDEF)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "can't allocate for raw data")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_alloc_from_free_list
 * Purpose:     Try to allocate SIZE bytes of memory from the free list
 *              if possible.
 *
 *              This is split from H5FD_alloc().
 * Return:      Success:    The format address of the new file memory.
 *              Failure:    The undefined address HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              02. December, 2002
 * Modifications:
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_alloc_from_free_list(H5FD_t *file, H5FD_mem_t type,
                          H5FD_mem_t mapped_type, hsize_t size)
{
    haddr_t ret_value = HADDR_UNDEF;

    FUNC_ENTER_NOAPI(H5FD_alloc_from_free_list, HADDR_UNDEF)

    assert(file);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    assert(size > 0);

    /*
     * Try to satisfy the request from the free list. Only perform the
     * search if the free list has the potential of satisfying the
     * request.
     *
     * Here, aligned requests are requests that are >= threshold and
     * alignment > 1.
     *
     * For non-aligned request, first try to find an exact match,
     * otherwise use the best match which is the smallest size that meets
     * the requested size.
     *
     * For aligned address request, find a block in the following order
     * of preferences:
     *
     *   1. block address is aligned and exact match in size;
     *   2. block address is aligned with smallest size > requested size;
     *   3. block address is not aligned with smallest size >= requested size.
     */
    if (mapped_type >= H5FD_MEM_DEFAULT && (file->maxsize == 0 || size <= file->maxsize)) {
        H5FD_free_t    *prev = NULL, *best = NULL;
        H5FD_free_t    *cur = file->fl[mapped_type];
        int             found_aligned = 0;
        int             need_aligned;
        hsize_t         head;

        need_aligned = file->alignment > 1 && size >= file->threshold;

        while (cur) {
            file->maxsize = MAX(file->maxsize, cur->size);

            if (need_aligned) {
                if ((head = cur->addr % file->alignment) == 0) {
                    /*
                     * Aligned address
                     */
                    if (cur->size == size) {
                        /* exact match */
                        ret_value = cur->addr;

                        /*
                         * Make certain we don't hand out a block of raw data
                         * from the free list which overlaps with the metadata
                         * aggregation buffer (if it's turned on)
                         */
                        if (type == H5FD_MEM_DRAW &&
                                (file->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) &&
                                H5F_addr_overlap(ret_value, size,
                                                 file->accum_loc, file->accum_size)) {
                            ret_value = HADDR_UNDEF;
                        } else {
                            if (prev)
                                prev->next = cur->next;
                            else
                                file->fl[mapped_type] = cur->next;

                            H5FL_FREE(H5FD_free_t, cur);

                            if (size == file->maxsize)
                                file->maxsize = 0;  /*unknown*/

                            HGOTO_DONE(ret_value)
                        }
                    }

                    if (cur->size > size)
                        if (!best || !found_aligned || cur->size < best->size) {
                            best = cur;
                            found_aligned = 1;
                        }
                } else {
                    /*
                     * Non-aligned address
                     *
                     * Check to see if this block is big enough to skip
                     * to the next aligned address and is still big
                     * enough for the requested size.  the extra
                     * cur->size>head is for preventing unsigned
                     * underflow. (this can be improved by checking for
                     * an exact match after excluding the head. Such
                     * match is as good as the found_aligned case above.)
                     */
                    head = file->alignment - head;  /* actual head size */

                    if (!found_aligned && cur->size > head && cur->size-head >= size &&
                            (!best || cur->size < best->size))
                        best = cur;
                }
            } else {
                /* !need_aligned */
                if (cur->size == size) {
                    /* exact match */
                    ret_value = cur->addr;

                    /*
                     * Make certain we don't hand out a block of raw data
                     * from the free list which overlaps with the metadata
                     * aggregation buffer (if it's turned on)
                     */
                    if (type == H5FD_MEM_DRAW &&
                            (file->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) &&
                            H5F_addr_overlap(ret_value, size, file->accum_loc,
                                             file->accum_size)) {
                        ret_value = HADDR_UNDEF;
                    } else {
                        if (prev)
                            prev->next = cur->next;
                        else
                            file->fl[mapped_type] = cur->next;

                        H5FL_FREE(H5FD_free_t, cur);

                        if (size == file->maxsize)
                            file->maxsize = 0;  /*unknown*/

                        HGOTO_DONE(ret_value)
                    }
                } else {
                    if (cur->size > size && (!best || cur->size < best->size))
                        best = cur;
                }
            }

            prev = cur;
            cur = cur->next;
        }

        /* Couldn't find exact match, use best fitting piece found */
        if (best) {
            if (best->size == file->maxsize)
                file->maxsize = 0;  /*unknown*/

            if (!need_aligned || found_aligned) {
                /* free only tail */
                ret_value = best->addr;

                /*
                 * Make certain we don't hand out a block of raw data
                 * from the free list which overlaps with the metadata
                 * aggregation buffer (if it's turned on)
                 */
                if (type == H5FD_MEM_DRAW &&
                        (file->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) &&
                        H5F_addr_overlap(ret_value, size, file->accum_loc,
                                         file->accum_size)) {
                    ret_value = HADDR_UNDEF;
                } else {
                    best->addr += size;     /* Reduce size of block on free list */
                    best->size -= size;
                    HGOTO_DONE(ret_value)
                }
            } else {
                /*
                 * Split into 3 pieces. Keep the the head and tail in the
                 * freelist.
                 */
                H5FD_free_t *tmp = NULL;

                head = file->alignment - (best->addr % file->alignment);
                ret_value = best->addr + head;

                /*
                 * Make certain we don't hand out a block of raw data
                 * from the free list which overlaps with the metadata
                 * aggregation buffer (if it's turned on)
                 */
                if (type == H5FD_MEM_DRAW &&
                        (file->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) &&
                        H5F_addr_overlap(ret_value, size, file->accum_loc, file->accum_size)) {
                    ret_value = HADDR_UNDEF;
                } else {
                    /* Attempt to allocate memory for temporary node */
                    if((tmp = H5FL_MALLOC(H5FD_free_t))==NULL)
                        HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "free block allocation failed")

                    if ((tmp->size = (best->size - (head + size)))!=0) {
                        tmp->addr = best->addr + (head + size);
                        tmp->next = best->next;
                        best->next = tmp;
                    } else {
                        /* no tail piece */
                        H5FL_FREE(H5FD_free_t,tmp);
                    }

                    best->size = head;
                    HGOTO_DONE(ret_value)
                } /* end else */
            } /* end else */
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_alloc_metadata
 * Purpose:     Try to allocate SIZE bytes of memory from the metadata
 *              block if possible.
 *
 *              This is split from H5FD_alloc().
 * Return:      Success:    The format address of the new file memory.
 *              Failure:    The undefined address HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              2. December, 2002
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_alloc_metadata(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t ret_value = HADDR_UNDEF;

    FUNC_ENTER_NOAPI(H5FD_alloc_metadata, HADDR_UNDEF)

    /* check args */
    assert(file);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    assert(size > 0);

    /*
     * If the metadata aggregation feature is enabled for this VFL
     * driver, allocate "generic" metadata space and sub-allocate out of
     * that, if possible. Otherwise just allocate through
     * H5FD_real_alloc()
     */

    /*
     * Allocate all types of metadata out of the metadata block
     */
    if (file->feature_flags & H5FD_FEAT_AGGREGATE_METADATA) {
        /*
         * Check if the space requested is larger than the space left in
         * the block
         */
        if (size > file->cur_meta_block_size) {
            haddr_t new_meta;   /* Address for new metadata */

            /*
             * Check if the block asked for is too large for a metadata
             * block
             */
            if (size >= file->def_meta_block_size) {
                /* Allocate more room for this new block the regular way */
                new_meta = H5FD_real_alloc(file, type, dxpl_id, size);

                /*
                 * Check if the new metadata is at the end of the current
                 * metadata block
                 */
                if (file->eoma + file->cur_meta_block_size == new_meta) {
                    /*
                     * Treat the allocation request as if the current
                     * metadata block grew by the amount allocated and
                     * just update the eoma address. Don't bother
                     * updating the cur_meta_block_size since it will
                     * just grow and shrink by the same amount.
                     */
                    ret_value = file->eoma;
                    file->eoma += size;
                } else {
                    /* Use the new metadata block for the space allocated */
                    ret_value = new_meta;
                }
            } else {
                /* Allocate another metadata block */
                new_meta = H5FD_real_alloc(file, H5FD_MEM_DEFAULT, dxpl_id,
                                           file->def_meta_block_size);

                /*
                 * Check if the new metadata is at the end of the current
                 * metadata block
                 */
                if (file->eoma + file->cur_meta_block_size == new_meta) {
                    file->cur_meta_block_size += file->def_meta_block_size;
                } else {
                    /*
                     * Return the unused portion of the metadata block to
                     * a free list
                     */
                    if (file->eoma != 0)
                        if (H5FD_free(file, H5FD_MEM_DEFAULT, dxpl_id, file->eoma,
                                      file->cur_meta_block_size) < 0)
                            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "can't free metadata block")

                    /* Point the metadata block at the newly allocated block */
                    file->eoma = new_meta;
                    file->cur_meta_block_size = file->def_meta_block_size;
                }

                /* Allocate space out of the metadata block */
                ret_value = file->eoma;
                file->cur_meta_block_size -= size;
                file->eoma += size;
            }
        } else {
            /* Allocate space out of the metadata block */
            ret_value = file->eoma;
            file->cur_meta_block_size -= size;
            file->eoma += size;
        }
    } else {
        /* Allocate data the regular way */
        ret_value = H5FD_real_alloc(file, type, dxpl_id, size);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_alloc_raw
 * Purpose:     Try to allocate SIZE bytes of raw data.
 *
 *              This is split from H5FD_alloc().
 * Return:      Success:    The format address of the new file memory.
 *              Failure:    The undefined address HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              2. December, 2002
 * Modifications:
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_alloc_raw(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t ret_value = HADDR_UNDEF;

    FUNC_ENTER_NOAPI(H5FD_alloc_raw, HADDR_UNDEF)

    /* check args */
    assert(file);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    assert(size > 0);

    /*
     * If the "small data" aggregation feature is enabled for this VFL driver,
     * allocate "small data" space and sub-allocate out of that, if
     * possible. Otherwise just allocate through H5FD_real_alloc()
     */
    if (file->feature_flags & H5FD_FEAT_AGGREGATE_SMALLDATA) {
        /*
         * Check if the space requested is larger than the space left in
         * the block
         */
        if (size > file->cur_sdata_block_size) {
            haddr_t new_data;       /* Address for new raw data block */

            /* Check if the block asked for is too large for the "small data" block */
            if (size >= file->def_sdata_block_size) {
                /* Allocate more room for this new block the regular way */
                new_data = H5FD_real_alloc(file, type, dxpl_id, size);

                /*
                 * Check if the new raw data is at the end of the current
                 * "small data" block
                 */
                if (file->eosda + file->cur_sdata_block_size == new_data) {
                    /*
                     * Treat the allocation request as if the current
                     * "small data" block grew by the amount allocated
                     * and just update the eosda address. Don't bother
                     * updating the cur_sdata_block_size since it will
                     * just grow and shrink by the same amount.
                     */
                    ret_value = file->eosda;
                    file->eosda += size;
                } else {
                    /* Use the new "small data" block for the space allocated */
                    ret_value = new_data;
                }
            } else {
                /* Allocate another "small data" block */
                new_data = H5FD_real_alloc(file, type, dxpl_id,
                                           file->def_sdata_block_size);

                /*
                 * Check if the new raw data is at the end of the current
                 * "small data" block
                 */
                if (file->eosda + file->cur_sdata_block_size == new_data) {
                    file->cur_sdata_block_size += file->def_sdata_block_size;
                } else {
                    /*
                     * Return the unused portion of the "small data"
                     * block to a free list
                     */
                    if (file->eosda != 0)
                        if (H5FD_free(file, H5FD_MEM_DRAW, dxpl_id, file->eosda,
                                      file->cur_sdata_block_size) < 0)
                            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "can't free 'small data' block")

                    /*
                     * Point the "small data" block at the newly
                     * allocated block
                     */
                    file->eosda = new_data;
                    file->cur_sdata_block_size = file->def_sdata_block_size;
                }

                /* Allocate space out of the "small data" block */
                ret_value = file->eosda;
                file->cur_sdata_block_size -= size;
                file->eosda += size;
            }
        } else {
            /* Allocate space out of the "small data" block */
            ret_value = file->eosda;
            file->cur_sdata_block_size -= size;
            file->eosda += size;
        }
    } else {
        /* Allocate data the regular way */
        ret_value = H5FD_real_alloc(file, type, dxpl_id, size);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_real_alloc
 *
 * Purpose:  Double private version of H5FDalloc() :-)
 *
 * Return:  Success:  The format address of the new file memory.
 *
 *    Failure:  The undefined address HADDR_UNDEF
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *  Albert Cheng, 2001/05/01
 *  Implement the allocation by alignment/threshold.
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_real_alloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t ret_value = HADDR_UNDEF;

    FUNC_ENTER_NOAPI_NOINIT(H5FD_real_alloc)

    /* check args */
    assert(file);
    assert(file->cls);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    assert(size > 0);

    /*
     * Dispatch to driver `alloc' callback or extend the end-of-address
     * marker
     */
    if (file->cls->alloc) {
        if ((ret_value = (file->cls->alloc)(file, type, dxpl_id, size)) == HADDR_UNDEF)
            HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "driver allocation request failed")
    } else {
        if ((ret_value = H5FD_update_eoa(file, type, dxpl_id, size)) == HADDR_UNDEF)
            HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "driver eoa update request failed")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_real_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_update_eoa
 * Purpose:     Update the EOA field of the file's memory.
 *
 *              This was split off from the H5FD_real_alloc function to
 *              make life easier for all.
 * Return:      Success:    The format address of the new file memory.
 *              Failure:    The undefined address HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              Wednesday, 04. December, 2002
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_update_eoa(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t eoa, oldeoa = 0;
    hsize_t wasted;
    haddr_t ret_value = HADDR_UNDEF;

    FUNC_ENTER_NOAPI_NOINIT(H5FD_update_eoa)

    /* check args */
    assert(file);
    assert(file->cls);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    assert(size > 0);

    eoa = file->cls->get_eoa(file);

#ifdef H5F_DEBUG
    if (file->alignment * file->threshold != 1 && H5DEBUG(F))
        HDfprintf(H5DEBUG(F),
                  "%s: alignment=%Hd, threshold=%Hd, size=%Hd, Begin eoa=%a\n",
                  FUNC, file->alignment, file->threshold, size, eoa);
#endif  /* H5F_DEBUG */

    /*
     * Wasted is 0 if not exceeding threshold or eoa happens to be
     * aligned
     */
    wasted = (size >= file->threshold) ? (eoa % file->alignment) : 0;

    if (wasted) {
        wasted = file->alignment - wasted;      /* actual waste                 */
        oldeoa = eoa;                           /* save it for later freeing    */

      /* Advance eoa to the next alignment by allocating the wasted */
        if (H5F_addr_overflow(eoa, size) || eoa + wasted > file->maxaddr)
            HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "file allocation request failed")

        eoa += wasted;

        if (file->cls->set_eoa(file, eoa) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "file allocation request failed")
    }

    /* allocate the aligned memory */
    if (H5F_addr_overflow(eoa, size) || eoa + size > file->maxaddr)
        HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "file allocation request failed")

    ret_value = eoa;
    eoa += size;

    if (file->cls->set_eoa(file, eoa) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF, "file allocation request failed")

    /* Free the wasted memory */
    if (wasted) {
        if(H5FD_free(file, type, dxpl_id, oldeoa, wasted)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "file deallocation request failed")
    } /* end if */

#ifdef H5F_DEBUG
    if (file->alignment * file->threshold != 1 && H5DEBUG(F))
        HDfprintf(H5DEBUG(F),
                  "%s: ret_value=%a, wasted=%Hd, Ended eoa=%a\n",
                  FUNC, ret_value, wasted, eoa);
#endif  /* H5F_DEBUG */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDfree
 *
 * Purpose:  Frees format addresses starting with ADDR and continuing for
 *    SIZE bytes in the file FILE. The type of space being freed is
 *    specified by TYPE, which is mapped to a free list as
 *    described for the H5FDalloc() function above.  If the request
 *    doesn't map to a free list then either the application `free'
 *    callback is invoked (if defined) or the memory is leaked.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, July 28, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDfree(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDfree, FAIL)
    H5TRACE5("e","xMtiah",file,type,dxpl_id,addr,size);

    /* Check args */
    if (!file || !file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer")
    if (type<H5FD_MEM_DEFAULT || type>=H5FD_MEM_NTYPES)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid request type")
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(dxpl_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Do the real work */
    if (H5FD_free(file, type, dxpl_id, addr, size)<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "file deallocation request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_free
 *
 * Purpose:     Private version of H5FDfree()
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *          Bill Wendling, February 20, 2003
 *          Added support for Flexible PHDF5. If the process is the
 *          Set-Aside-Process, then we execute this function. Clients
 *          don't.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_free(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size)
{
    H5FD_mem_t    mapped_type;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_free, FAIL)

    /* Check args */
    assert(file);
    assert(file->cls);
    assert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);

    if (!H5F_addr_defined(addr) || addr>file->maxaddr ||
            H5F_addr_overflow(addr, size) || addr+size>file->maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid region")

    /* Allow 0-sized free's to occur without penalty */
    if(0==size)
        HGOTO_DONE(SUCCEED)

    /* Map request type to free list */
    if (H5FD_MEM_DEFAULT==file->cls->fl_map[type]) {
        mapped_type = type;
    } else {
        mapped_type = file->cls->fl_map[type];
    }

    /*
     * If the request maps to a free list then add memory to the free list
     * without ever telling the driver that it was freed.  Otherwise let the
     * driver deallocate the memory.
     */
    if (mapped_type>=H5FD_MEM_DEFAULT) {
        H5FD_free_t *last;          /* Last merged node */
        H5FD_free_t *last_prev=NULL;/* Pointer to node before merged node */
        H5FD_free_t *curr;          /* Current free block being inspected */
        H5FD_free_t *prev;          /* Previous free block being inspected */

        /* Adjust the metadata accumulator to remove the freed block, if it overlaps */
        if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA)
                && H5F_addr_overlap(addr,size,file->accum_loc,file->accum_size)) {
            size_t overlap_size;        /* Size of overlap with accumulator */

            /* Check for overlapping the beginning of the accumulator */
            if(H5F_addr_le(addr,file->accum_loc)) {
                /* Check for completely overlapping the accumulator */
                if(H5F_addr_ge(addr+size,file->accum_loc+file->accum_size)) {
                    /* Reset the entire accumulator */
                    file->accum_loc=HADDR_UNDEF;
                    file->accum_size=FALSE;
                    file->accum_dirty=FALSE;
                } /* end if */
                /* Block to free must end within the accumulator */
                else {
                    size_t new_accum_size;      /* Size of new accumulator buffer */

                    /* Calculate the size of the overlap with the accumulator, etc. */
                    H5_ASSIGN_OVERFLOW(overlap_size,(addr+size)-file->accum_loc,haddr_t,size_t);
                    new_accum_size=file->accum_size-overlap_size;

                    /* Move the accumulator buffer information to eliminate the freed block */
                    HDmemmove(file->meta_accum,file->meta_accum+overlap_size,new_accum_size);

                    /* Adjust the accumulator information */
                    file->accum_loc+=overlap_size;
                    file->accum_size=new_accum_size;
                } /* end else */
            } /* end if */
            /* Block to free must start within the accumulator */
            else {
                /* Calculate the size of the overlap with the accumulator */
                H5_ASSIGN_OVERFLOW(overlap_size,(file->accum_loc+file->accum_size)-addr,haddr_t,size_t);

                /* Block to free is in the middle of the accumulator */
                if(H5F_addr_lt(addr,file->accum_loc+file->accum_size)) {
                    haddr_t tail_addr;
                    size_t tail_size;

                    /* Calculate the address & size of the tail to write */
                    tail_addr=addr+size;
                    H5_ASSIGN_OVERFLOW(tail_size,(file->accum_loc+file->accum_size)-tail_addr,haddr_t,size_t);

                    /* Write out the part of the accumulator after the block to free */
                    /* (Use the driver's write call directly - to avoid looping back and writing to metadata accumulator) */
                    if ((file->cls->write)(file, H5FD_MEM_DEFAULT, dxpl_id, tail_addr, tail_size, file->meta_accum+(tail_addr-file->accum_loc))<0)
                        HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "file write request failed")
                } /* end if */

                /* Adjust the accumulator information */
                file->accum_size=file->accum_size-overlap_size;
            } /* end else */
        } /* end if */

        /* Scan through the existing blocks for the mapped type to see if we can extend one */
        curr=file->fl[mapped_type];
        last=prev=NULL;
        while(curr!=NULL) {
            /* Check if the block to free adjoins the start of the current block */
            if((addr+size)==curr->addr) {
                /* If we previously found & merged a node, eliminate it from the list & free it */
                if(last!=NULL) {
                    /* Check if there was a previous block in the list */
                    if(last_prev!=NULL)
                        /* Eliminate the merged block from the list */
                        last_prev->next=last->next;
                    /* No previous block, this must be the head of the list */
                    else
                        /* Eliminate the merged block from the list */
                        file->fl[mapped_type] = last->next;

                    /* Check for eliminating the block before the 'current' one */
                    if(last==prev)
                        prev=last_prev;

                    /* Free the memory for the merged block */
                    H5FL_FREE(H5FD_free_t,last);
                } /* end if */

                /* Adjust the address and size of the block found */
                curr->addr=addr;
                curr->size+=size;

                /* Adjust the information about to memory block to include the merged block */
                addr=curr->addr;
                size=curr->size;

                /* Update the information about the merged node */
                last=curr;
                last_prev=prev;
            } /* end if */
            else {
                /* Check if the block to free adjoins the end of the current block */
                if((curr->addr+curr->size)==addr) {
                    /* If we previously found & merged a node, eliminate it from the list & free it */
                    if(last!=NULL) {
                        /* Check if there was a previous block in the list */
                        if(last_prev!=NULL)
                            /* Eliminate the merged block from the list */
                            last_prev->next=last->next;
                        /* No previous block, this must be the head of the list */
                        else
                            /* Eliminate the merged block from the list */
                            file->fl[mapped_type] = last->next;

                        /* Check for eliminating the block before the 'current' one */
                        if(last==prev)
                            prev=last_prev;

                        /* Free the memory for the merged block */
                        H5FL_FREE(H5FD_free_t,last);
                    } /* end if */

                    /* Adjust the size of the block found */
                    curr->size+=size;

                    /* Adjust the information about to memory block to include the merged block */
                    addr=curr->addr;
                    size=curr->size;

                    /* Update the information about the merged node */
                    last=curr;
                    last_prev=prev;
                } /* end if */
            } /* end else */

            /* Advance to next node in list */
            prev=curr;
            curr=curr->next;
        } /* end while */

        /* Check if we adjusted an existing block */
        if(last!=NULL) {
            /* Move the node found to the front, if it wasn't already there */
            if(last_prev!=NULL) {
                last_prev->next=last->next;
                last->next = file->fl[mapped_type];
                file->fl[mapped_type] = last;
            } /* end if */
        } /* end if */
        else {
            /* Allocate a new node to hold the free block's information */
            if(NULL==(last = H5FL_MALLOC(H5FD_free_t)))
                HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, FAIL, "can't allocate node for free space info")

            last->addr = addr;
            last->size = size;
            last->next = file->fl[mapped_type];
            file->fl[mapped_type] = last;
        } /* end else */

        /* Check if we increased the size of the largest block on the list */
        file->maxsize = MAX(file->maxsize, last->size);

        /* Check if this free block is at the end of file allocated space.
         * Truncate it if this is true. */
        if(file->cls->get_eoa) {
            haddr_t     eoa;

            eoa = file->cls->get_eoa(file);
            if(eoa == (last->addr+last->size)) {
                if(file->cls->set_eoa(file, last->addr) < 0)
                    HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "set end of space allocation request failed")

                /* Remove this free block from the list */
                file->fl[mapped_type] = last->next;
                if(file->maxsize==last->size)
                    file->maxsize=0; /*unknown*/
                H5FL_FREE(H5FD_free_t, last);
            }
        }
    } else if (file->cls->free) {
        if ((file->cls->free)(file, type, dxpl_id, addr, size)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver free request failed")
    } else {
        /* leak memory */
#ifdef H5F_DEBUG
HDfprintf(stderr, "%s: LEAKED MEMORY!!!!!!\n", FUNC);
#endif /* H5F_DEBUG */
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDrealloc
 *
 * Purpose:  Changes the size of an allocated chunk of memory, possibly
 *    also changing its location in the file.
 *
 * Return:  Success:  New address of the block of memory, not
 *        necessarily the same as the original address.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDrealloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t old_addr, hsize_t old_size,
      hsize_t new_size)
{
    haddr_t  ret_value=HADDR_UNDEF;

    FUNC_ENTER_API(H5FDrealloc, HADDR_UNDEF)
    H5TRACE6("a","xMtiahh",file,type,dxpl_id,old_addr,old_size,new_size);

    /* Check args */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(dxpl_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "not a data transfer property list")

    if (HADDR_UNDEF==(ret_value=H5FD_realloc(file, type, dxpl_id, old_addr, old_size, new_size)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "file reallocation request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_realloc
 *
 * Purpose:  Private version of H5FDrealloc()
 *
 * Return:  Success:  New address of the block of memory, not
 *        necessarily the same as the original address.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_realloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t old_addr, hsize_t old_size,
       hsize_t new_size)
{
    haddr_t  new_addr=old_addr;
    uint8_t  _buf[8192];
    uint8_t  *buf=_buf;
    haddr_t      ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_realloc, HADDR_UNDEF)

    if (new_size==old_size) {
        /*nothing to do*/

    } else if (0==old_size) {
        /* allocate memory */
        assert(!H5F_addr_defined(old_addr));
        if (HADDR_UNDEF==(new_addr=H5FD_alloc(file, type, dxpl_id, new_size)))
            HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, HADDR_UNDEF, "file allocation failed")
    } else if (0==new_size) {
        /* free memory */
        assert(H5F_addr_defined(old_addr));
        if(H5FD_free(file, type, dxpl_id, old_addr, old_size)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "file deallocation request failed")
        new_addr = HADDR_UNDEF;

    } else if (new_size<old_size) {
        /* free the end of the block */
        if(H5FD_free(file, type, dxpl_id, old_addr+old_size, old_size-new_size)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "file deallocation request failed")
    } else {
        /* move memory to new location */
        /* Note!  This may fail if sizeof(hsize_t)>sizeof(size_t) and the
         * object on disk is too large to read into a memory buffer all at one
         * time.  This chunk of code would have to be re-written using a loop
         * to move pieces of the realloced data through a fixed size buffer, etc.
         * -QAK, 6/20/01
         */
        if (HADDR_UNDEF==(new_addr=H5FD_alloc(file, type, dxpl_id, new_size)))
            HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, HADDR_UNDEF, "file allocation failed")
        H5_CHECK_OVERFLOW(old_size,hsize_t,size_t);
        if (old_size>sizeof(_buf) && NULL==(buf=H5MM_malloc((size_t)old_size))) {
            (void)H5FD_free(file, type, dxpl_id, new_addr, new_size);
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "memory allocation failed")
        }
        if (H5FD_read(file, type, dxpl_id, old_addr, (size_t)old_size, buf)<0 ||
                H5FD_write(file, type, dxpl_id, new_addr, (size_t)old_size, buf)<0) {
            (void)H5FD_free(file, type, dxpl_id, new_addr, new_size);
            if (buf!=_buf)
                H5MM_xfree(buf);
            HGOTO_ERROR(H5E_FILE, H5E_READERROR, HADDR_UNDEF, "unable to move file block")
        }

        if (buf!=_buf)
            H5MM_xfree(buf);
        if(H5FD_free(file, type, dxpl_id, old_addr, old_size)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, HADDR_UNDEF, "file deallocation request failed")
    }

    /* Set return value */
    ret_value=new_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_can_extend
 *
 * Purpose:  Check if a block in the file can be extended.
 *
 *    This is a simple check currently, which only checks for the
 *              block being at the end of the file.  A more sophisticated check
 *              would also use the free space list to see if there is a block
 *              appropriately placed to accomodate the space requested.
 *
 * Return:  Success:  TRUE(1)/FALSE(0)
 *
 *     Failure:  FAIL
 *
 * Programmer:  Quincey Koziol
 *              Friday, June 11, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5FD_can_extend(H5FD_t *file, H5FD_mem_t type, haddr_t addr, hsize_t size, hsize_t UNUSED extra_requested)
{
    haddr_t     eoa;                    /* End of address space in the file */
    htri_t      ret_value=FALSE;        /* Return value */

    FUNC_ENTER_NOAPI(H5FD_can_extend, FAIL)

    /* Retrieve the end of the address space */
    if (HADDR_UNDEF==(eoa=H5FD_get_eoa(file)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver get_eoa request failed")

    /* Check if the block is exactly at the end of the file */
    if((addr+size)==eoa)
        HGOTO_DONE(TRUE)
    /* Check if block is inside the metadata or small data accumulator */
    else {
        if(type!=H5FD_MEM_DRAW) {
            if (file->feature_flags & H5FD_FEAT_AGGREGATE_METADATA) {
                /* If the metadata block is at the end of the file, and
                 * the block to test adjoins the beginning of the metadata
                 * block, then it's extendable
                 */
                if (file->eoma + file->cur_meta_block_size == eoa &&
                        (addr+size)==file->eoma)
                    HGOTO_DONE(TRUE)
            } /* end if */
        } /* end if */
        else {
            if (file->feature_flags & H5FD_FEAT_AGGREGATE_SMALLDATA) {
                /* If the small data block is at the end of the file, and
                 * the block to test adjoins the beginning of the small data
                 * block, then it's extendable
                 */
                if (file->eosda + file->cur_sdata_block_size == eoa &&
                        (addr+size)==file->eosda)
                    HGOTO_DONE(TRUE)
            } /* end if */
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_can_extend() */


/*-------------------------------------------------------------------------
 * Function:  H5FD_extend
 *
 * Purpose:  Extend a block in the file.
 *
 *    This is simple code currently, which only checks for the
 *              block being at the end of the file.  A more sophisticated check
 *              would also use the free space list to see if there is a block
 *              appropriately placed to accomodate the space requested.
 *
 * Return:  Success:  TRUE(1)/FALSE(0)
 *
 *     Failure:  FAIL
 *
 * Programmer:  Quincey Koziol
 *              Saturday, June 12, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_extend(H5FD_t *file, H5FD_mem_t type, haddr_t addr, hsize_t size, hsize_t extra_requested)
{
    haddr_t     eoa;                    /* End of address space in the file */
    hbool_t     update_eoma=FALSE;      /* Whether we need to update the eoma */
    hbool_t     update_eosda=FALSE;     /* Whether we need to update the eosda */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5FD_extend, FAIL)

    /* Retrieve the end of the address space */
    if (HADDR_UNDEF==(eoa=H5FD_get_eoa(file)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver get_eoa request failed")

    /* Check if the block is exactly at the end of the file */
    /* (Check if block is inside the metadata or small data accumulator) */
    if((addr+size)!=eoa) {
        if(type!=H5FD_MEM_DRAW) {
            if (file->feature_flags & H5FD_FEAT_AGGREGATE_METADATA) {
                /* If the metadata block is at the end of the file, and
                 * the block to test adjoins the beginning of the metadata
                 * block, then it's extendable
                 */
                if (file->eoma + file->cur_meta_block_size == eoa &&
                        (addr+size)==file->eoma)
                    update_eoma=TRUE;
                else
                    HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "can't extend block")
            } /* end if */
            else
                HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "can't extend block")
        } /* end if */
        else {
            if (file->feature_flags & H5FD_FEAT_AGGREGATE_SMALLDATA) {
                /* If the small data block is at the end of the file, and
                 * the block to test adjoins the beginning of the small data
                 * block, then it's extendable
                 */
                if (file->eosda + file->cur_sdata_block_size == eoa &&
                        (addr+size)==file->eosda)
                    update_eosda=TRUE;
                else
                    HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "can't extend block")
            } /* end if */
            else
                HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "can't extend block")
        } /* end else */
    } /* end else */

    /* Check for overflowing the file */
    if (H5F_addr_overflow(eoa, extra_requested) || eoa + extra_requested > file->maxaddr)
        HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "file allocation request failed")

    /* Extend the file */
    eoa += extra_requested;
    if (file->cls->set_eoa(file, eoa) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_NOSPACE, FAIL, "file allocation request failed")

    /* Update the metadata and/or small data block */
    assert(!(update_eoma && update_eosda));
    if(update_eoma)
        file->eoma+=extra_requested;
    if(update_eosda)
        file->eosda+=extra_requested;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_extend() */


/*-------------------------------------------------------------------------
 * Function:  H5FDget_eoa
 *
 * Purpose:  Returns the address of the first byte after the last
 *    allocated memory in the file.
 *
 * Return:  Success:  First byte after allocated memory.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Friday, July 30, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDget_eoa(H5FD_t *file)
{
    haddr_t  ret_value;

    FUNC_ENTER_API(H5FDget_eoa, HADDR_UNDEF)
    H5TRACE1("a","x",file);

    /* Check args */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "invalid file pointer")

    /* The real work */
    if (HADDR_UNDEF==(ret_value=H5FD_get_eoa(file)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "file get eoa request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_get_eoa
 *
 * Purpose:  Private version of H5FDget_eoa()
 *
 * Return:  Success:  First byte after allocated memory.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_get_eoa(H5FD_t *file)
{
    haddr_t  ret_value;

    FUNC_ENTER_NOAPI(H5FD_get_eoa, HADDR_UNDEF)
    assert(file && file->cls);

    /* Dispatch to driver */
    if (HADDR_UNDEF==(ret_value=(file->cls->get_eoa)(file)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "driver get_eoa request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDset_eoa
 *
 * Purpose:  Set the end-of-address marker for the file. The ADDR is the
 *    address of the first byte past the last allocated byte of the
 *    file. This function is called from two places:
 *
 *        It is called after an existing file is opened in order to
 *        "allocate" enough space to read the superblock and then
 *        to "allocate" the entire hdf5 file based on the contents
 *        of the superblock.
 *
 *        It is called during file memory allocation if the
 *        allocation request cannot be satisfied from the free list
 *        and the driver didn't supply an allocation callback.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative, no side effect
 *
 * Programmer:  Robb Matzke
 *              Friday, July 30, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDset_eoa(H5FD_t *file, haddr_t addr)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDset_eoa, FAIL)
    H5TRACE2("e","xa",file,addr);

    /* Check args */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer")
    if (!H5F_addr_defined(addr) || addr>file->maxaddr)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid end-of-address value")

    /* The real work */
    if (H5FD_set_eoa(file, addr)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "file set eoa request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_set_eoa
 *
 * Purpose:  Private version of H5FDset_eoa()
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative, no side effect
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_set_eoa(H5FD_t *file, haddr_t addr)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_set_eoa, FAIL)

    assert(file && file->cls);
    assert(H5F_addr_defined(addr) && addr<=file->maxaddr);

    /* Dispatch to driver */
    if ((file->cls->set_eoa)(file, addr)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver set_eoa request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDget_eof
 *
 * Purpose:  Returns the end-of-file address, which is the greater of the
 *    end-of-format address and the actual EOF marker. This
 *    function is called after an existing file is opened in order
 *    for the library to learn the true size of the underlying file
 *    and to determine whether the hdf5 data has been truncated.
 *
 *    It is also used when a file is first opened to learn whether
 *    the file is empty or not.
 *
 *     It is permissible for the driver to return the maximum address
 *    for the file size if the file is not empty.
 *
 * Return:  Success:  The EOF address.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDget_eof(H5FD_t *file)
{
    haddr_t  ret_value;

    FUNC_ENTER_API(H5FDget_eof, HADDR_UNDEF)
    H5TRACE1("a","x",file);

    /* Check arguments */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "invalid file pointer")

    /* The real work */
    if (HADDR_UNDEF==(ret_value=H5FD_get_eof(file)))
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "file get eof request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_get_eof
 *
 * Purpose:  Private version of H5FDget_eof()
 *
 * Return:  Success:  The EOF address.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_get_eof(H5FD_t *file)
{
    haddr_t  ret_value;

    FUNC_ENTER_NOAPI(H5FD_get_eof, HADDR_UNDEF)

    assert(file && file->cls);

    /* Dispatch to driver */
    if (file->cls->get_eof) {
  if (HADDR_UNDEF==(ret_value=(file->cls->get_eof)(file)))
      HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "driver get_eof request failed")
    } else {
  ret_value = file->maxaddr;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDread
 *
 * Purpose:  Reads SIZE bytes from FILE beginning at address ADDR
 *    according to the data transfer property list DXPL_ID (which may
 *    be the constant H5P_DEFAULT). The result is written into the
 *    buffer BUF.
 *
 * Return:  Success:  Non-negative. The read result is written into
 *        the BUF buffer which should be allocated by
 *        the caller.
 *
 *    Failure:  Negative. The contents of BUF is undefined.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDread(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
   void *buf/*out*/)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDread, FAIL)
    H5TRACE6("e","xMtiazx",file,type,dxpl_id,addr,size,buf);

    /* Check args */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(dxpl_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")
    if (!buf)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null result buffer")

    /* Do the real work */
    if (H5FD_read(file, type, dxpl_id, addr, size, buf)<0)
  HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "file read request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_read
 *
 * Purpose:  Private version of H5FDread()
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *  Albert Cheng, 2000-11-21
 *  Disable the code that does early return when size==0 for
 *  Parallel mode since a collective call would require the process
 *  to continue on with "nothing" to transfer.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_read(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
    void *buf/*out*/)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_read, FAIL)

    assert(file && file->cls);
    assert(H5I_GENPROP_LST==H5I_get_type(dxpl_id));
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
    assert(buf);

#ifndef H5_HAVE_PARALLEL
    /* Do not return early for Parallel mode since the I/O could be a */
    /* collective transfer. */
    /* The no-op case */
    if (0==size)
        HGOTO_DONE(SUCCEED)
#endif /* H5_HAVE_PARALLEL */

    /* Check if this information is in the metadata accumulator */
    if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) && type!=H5FD_MEM_DRAW) {
        /* Current read overlaps with metadata accumulator */
        if(H5F_addr_overlap(addr,size,file->accum_loc,file->accum_size)) {
            unsigned char *read_buf=(unsigned char *)buf; /* Pointer to the buffer being read in */
            size_t amount_read;         /* Amount to read at a time */
#ifndef NDEBUG
            hsize_t tempamount_read;         /* Amount to read at a time */
#endif /* NDEBUG */
            hsize_t read_off;           /* Offset to read from */

            /* Double check that we aren't reading raw data */
            assert(type!=H5FD_MEM_DRAW);

            /* Read the part before the metadata accumulator */
            if(addr<file->accum_loc) {
                /* Set the amount to read */
                H5_ASSIGN_OVERFLOW(amount_read,file->accum_loc-addr,hsize_t,size_t);

                /* Dispatch to driver */
                if ((file->cls->read)(file, type, dxpl_id, addr, amount_read, read_buf)<0)
                    HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "driver read request failed")

                /* Adjust the buffer, address & size */
                read_buf+=amount_read;
                addr+=amount_read;
                size-=amount_read;
            } /* end if */

            /* Copy the part overlapping the metadata accumulator */
            if(size>0 && (addr>=file->accum_loc && addr<(file->accum_loc+file->accum_size))) {
                /* Set the offset to "read" from */
                read_off=addr-file->accum_loc;

                /* Set the amount to "read" */
#ifndef NDEBUG
                tempamount_read = file->accum_size-read_off;
                H5_CHECK_OVERFLOW(tempamount_read,hsize_t,size_t);
                amount_read = MIN(size, (size_t)tempamount_read);
#else /* NDEBUG */
                amount_read = MIN(size, (size_t)(file->accum_size-read_off));
#endif /* NDEBUG */

                /* Copy the data out of the buffer */
                HDmemcpy(read_buf,file->meta_accum+read_off,amount_read);

                /* Adjust the buffer, address & size */
                read_buf+=amount_read;
                addr+=amount_read;
                size-=amount_read;
            } /* end if */

            /* Read the part after the metadata accumulator */
            if(size>0 && addr>=(file->accum_loc+file->accum_size)) {
                /* Dispatch to driver */
                if ((file->cls->read)(file, type, dxpl_id, addr, size, read_buf)<0)
                    HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "driver read request failed")

                /* Adjust the buffer, address & size */
                read_buf+=size;
                addr+=size;
                size-=size;
            } /* end if */

            /* Make certain we've read it all */
            assert(size==0);
        } /* end if */
        /* Current read doesn't overlap with metadata accumulator, read it into accumulator */
        else {
            /* Only update the metadata accumulator if it is not dirty or if
             * we are allowed to write the accumulator out during reads (when
             * it is dirty)
             */
            if(file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA_READ || !file->accum_dirty) {
                /* Flush current contents, if dirty */
                if(file->accum_dirty) {
                    if ((file->cls->write)(file, H5FD_MEM_DEFAULT, dxpl_id, file->accum_loc, file->accum_size, file->meta_accum)<0)
                        HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "driver write request failed")

                    /* Reset accumulator dirty flag */
                    file->accum_dirty=FALSE;
                } /* end if */

                /* Cache the new piece of metadata */
                /* Check if we need to resize the buffer */
                if(size>file->accum_buf_size) {
                    /* Grow the metadata accumulator buffer */
                    if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size))==NULL)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                    /* Note the new buffer size */
                    file->accum_buf_size=size;
                } /* end if */
                else {
                    /* Check if we should shrink the accumulator buffer */
                    if(size<(file->accum_buf_size/H5FD_ACCUM_THROTTLE) &&
                            file->accum_buf_size>H5FD_ACCUM_THRESHOLD) {
                        size_t new_size=(file->accum_buf_size/H5FD_ACCUM_THROTTLE); /* New size of accumulator buffer */

                        /* Shrink the accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,new_size))==NULL)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                        /* Note the new buffer size */
                        file->accum_buf_size=new_size;
                    } /* end if */
                } /* end else */

                /* Update accumulator information */
                file->accum_loc=addr;
                file->accum_size=size;
                file->accum_dirty=FALSE;

                /* Read into accumulator */
                if ((file->cls->read)(file, H5FD_MEM_DEFAULT, dxpl_id, file->accum_loc, file->accum_size, file->meta_accum)<0)
                    HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "driver read request failed")

                /* Copy into buffer */
                HDmemcpy(buf,file->meta_accum,size);
            } /* end if */
            else {
                /* Dispatch to driver */
                if ((file->cls->read)(file, type, dxpl_id, addr, size, buf)<0)
                    HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "driver read request failed")
            } /* end else */
        } /* end else */
    } /* end if */
    else {
        /* Dispatch to driver */
        if ((file->cls->read)(file, type, dxpl_id, addr, size, buf)<0)
            HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "driver read request failed")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDwrite
 *
 * Purpose:  Writes SIZE bytes to FILE beginning at address ADDR according
 *    to the data transfer property list DXPL_ID (which may be the
 *    constant H5P_DEFAULT). The bytes to be written come from the
 *    buffer BUF.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDwrite(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
    const void *buf)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDwrite, FAIL)
    H5TRACE6("e","xMtiazx",file,type,dxpl_id,addr,size,buf);

    /* Check args */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer")
    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(dxpl_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")
    if (!buf)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null buffer")

    /* The real work */
    if (H5FD_write(file, type, dxpl_id, addr, size, buf)<0)
  HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "file write request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_write
 *
 * Purpose:  Private version of H5FDwrite()
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *  Albert Cheng, 2000-11-21
 *  Disable the code that does early return when size==0 for
 *  Parallel mode since a collective call would require the process
 *  to continue on with "nothing" to transfer.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_write(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
     const void *buf)
{
    size_t new_size;    /* New size of the accumulator buffer */
    size_t old_offset;  /* Offset of old data within the accumulator buffer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_write, FAIL)

    assert(file && file->cls);
    assert(H5I_GENPROP_LST==H5I_get_type(dxpl_id));
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
    assert(buf);

#ifndef H5_HAVE_PARALLEL
    /* Do not return early for Parallel mode since the I/O could be a */
    /* collective transfer. */
    /* The no-op case */
    if (0==size)
        HGOTO_DONE(SUCCEED)
#endif /* H5_HAVE_PARALLEL */

    /* Check for accumulating metadata */
    if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) && type!=H5FD_MEM_DRAW) {
        /* Check if there is already metadata in the accumulator */
        if(file->accum_size>0) {
            /* Check if the piece of metadata being written adjoins or is inside the metadata accumulator */
            if((addr>=file->accum_loc && addr<=(file->accum_loc+file->accum_size))
                || ((addr+size)>file->accum_loc && (addr+size)<=(file->accum_loc+file->accum_size))
                || (addr<file->accum_loc && (addr+size)>=file->accum_loc)) {

                /* Check if the new metadata adjoins the beginning of the current accumulator */
                if((addr+size)==file->accum_loc) {
                    /* Check if we need more buffer space */
                    if((size+file->accum_size)>file->accum_buf_size) {
                        /* Adjust the buffer size, by doubling it */
                        file->accum_buf_size = MAX(file->accum_buf_size*2,size+file->accum_size);

                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,file->accum_buf_size))==NULL)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")
#ifdef H5_USING_PURIFY
HDmemset(file->meta_accum+file->accum_size,0,(file->accum_buf_size-file->accum_size));
#endif /* H5_USING_PURIFY */
                    } /* end if */

                    /* Move the existing metadata to the proper location */
                    HDmemmove(file->meta_accum+size,file->meta_accum,file->accum_size);

                    /* Copy the new metadata at the front */
                    HDmemcpy(file->meta_accum,buf,size);

                    /* Set the new size & location of the metadata accumulator */
                    file->accum_loc=addr;
                    file->accum_size=file->accum_size+size;

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata adjoins the end of the current accumulator */
                else if(addr==(file->accum_loc+file->accum_size)) {
                    /* Check if we need more buffer space */
                    if((size+file->accum_size)>file->accum_buf_size) {
                        /* Adjust the buffer size, by doubling it */
                        file->accum_buf_size = MAX(file->accum_buf_size*2,size+file->accum_size);

                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,file->accum_buf_size))==NULL)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")
#ifdef H5_USING_PURIFY
HDmemset(file->meta_accum+file->accum_size,0,(file->accum_buf_size-file->accum_size));
#endif /* H5_USING_PURIFY */
                    } /* end if */

                    /* Copy the new metadata to the end */
                    HDmemcpy(file->meta_accum+file->accum_size,buf,size);

                    /* Set the new size of the metadata accumulator */
                    file->accum_size=file->accum_size+size;

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata is entirely within the current accumulator */
                else if(addr>=file->accum_loc && (addr+size)<=(file->accum_loc+file->accum_size)) {
                    /* Copy the new metadata to the proper location within the accumulator */
                    HDmemcpy(file->meta_accum+(addr-file->accum_loc),buf,size);

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata overlaps the beginning of the current accumulator */
                else if(addr<file->accum_loc && (addr+size)<=(file->accum_loc+file->accum_size)) {
                    /* Calculate the new accumulator size, based on the amount of overlap */
                    H5_ASSIGN_OVERFLOW(new_size,(file->accum_loc-addr)+file->accum_size,hsize_t,size_t);

                    /* Check if we need more buffer space */
                    if(new_size>file->accum_buf_size) {
                        /* Adjust the buffer size, by doubling it */
                        file->accum_buf_size = MAX(file->accum_buf_size*2,new_size);

                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,file->accum_buf_size))==NULL)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")
#ifdef H5_USING_PURIFY
HDmemset(file->meta_accum+file->accum_size,0,(file->accum_buf_size-file->accum_size));
#endif /* H5_USING_PURIFY */
                    } /* end if */

                    /* Calculate the proper offset of the existing metadata */
                    H5_ASSIGN_OVERFLOW(old_offset,(addr+size)-file->accum_loc,hsize_t,size_t);

                    /* Move the existing metadata to the proper location */
                    HDmemmove(file->meta_accum+size,file->meta_accum+old_offset,(file->accum_size-old_offset));

                    /* Copy the new metadata at the front */
                    HDmemcpy(file->meta_accum,buf,size);

                    /* Set the new size & location of the metadata accumulator */
                    file->accum_loc=addr;
                    file->accum_size=new_size;

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata overlaps the end of the current accumulator */
                else if(addr>=file->accum_loc && (addr+size)>(file->accum_loc+file->accum_size)) {
                    /* Calculate the new accumulator size, based on the amount of overlap */
                    H5_ASSIGN_OVERFLOW(new_size,(addr-file->accum_loc)+size,hsize_t,size_t);

                    /* Check if we need more buffer space */
                    if(new_size>file->accum_buf_size) {
                        /* Adjust the buffer size, by doubling it */
                        file->accum_buf_size = MAX(file->accum_buf_size*2,new_size);

                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,file->accum_buf_size))==NULL)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")
#ifdef H5_USING_PURIFY
HDmemset(file->meta_accum+file->accum_size,0,(file->accum_buf_size-file->accum_size));
#endif /* H5_USING_PURIFY */
                    } /* end if */

                    /* Copy the new metadata to the end */
                    HDmemcpy(file->meta_accum+(addr-file->accum_loc),buf,size);

                    /* Set the new size & location of the metadata accumulator */
                    file->accum_size=new_size;

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                else {
                    assert(0 && "New metadata overlapped both beginning and end of existing metadata accumulator!");
                } /* end else */
            } /* end if */
            /* New piece of metadata doesn't adjoin or overlap the existing accumulator */
            else {
                /* Write out the existing metadata accumulator, with dispatch to driver */
                if(file->accum_dirty) {
                    if ((file->cls->write)(file, H5FD_MEM_DEFAULT, dxpl_id, file->accum_loc, file->accum_size, file->meta_accum)<0)
                        HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "driver write request failed")
                    /* Reset accumulator dirty flag */
                    file->accum_dirty=FALSE;
                } /* end if */

                /* Cache the new piece of metadata */
                /* Check if we need to resize the buffer */
                if(size>file->accum_buf_size) {
                    /* Grow the metadata accumulator buffer */
                    if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size))==NULL)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                    /* Note the new buffer size */
                    file->accum_buf_size=size;
#ifdef H5_USING_PURIFY
HDmemset(file->meta_accum+file->accum_size,0,(file->accum_buf_size-file->accum_size));
#endif /* H5_USING_PURIFY */
                } /* end if */
                else {
                    /* Check if we should shrink the accumulator buffer */
                    if(size<(file->accum_buf_size/H5FD_ACCUM_THROTTLE) &&
                            file->accum_buf_size>H5FD_ACCUM_THRESHOLD) {
                        size_t tmp_size=(file->accum_buf_size/H5FD_ACCUM_THROTTLE); /* New size of accumulator buffer */

                        /* Shrink the accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,tmp_size))==NULL)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                        /* Note the new buffer size */
                        file->accum_buf_size=tmp_size;
                    } /* end if */
                } /* end else */

                /* Update the metadata accumulator information */
                file->accum_loc=addr;
                file->accum_size=size;
                file->accum_dirty=TRUE;

                /* Store the piece of metadata in the accumulator */
                HDmemcpy(file->meta_accum,buf,size);
            } /* end else */
        } /* end if */
        /* No metadata in the accumulator, grab this piece and keep it */
        else {
            /* Check if we need to reallocate the buffer */
            if(size>file->accum_buf_size) {
                /* Reallocate the metadata accumulator buffer */
                if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size))==NULL)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                /* Note the new buffer size */
                file->accum_buf_size=size;
            } /* end if */

            /* Update the metadata accumulator information */
            file->accum_loc=addr;
            file->accum_size=size;
            file->accum_dirty=TRUE;

            /* Store the piece of metadata in the accumulator */
            HDmemcpy(file->meta_accum,buf,size);
        } /* end else */
    } /* end if */
    else {
        /* Dispatch to driver */
        if ((file->cls->write)(file, type, dxpl_id, addr, size, buf)<0)
            HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "driver write request failed")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FDflush
 *
 * Purpose:  Notify driver to flush all cached data.  If the driver has no
 *    flush method then nothing happens.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *              Quincey Koziol, May 20, 2002
 *              Added 'closing' parameter
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDflush(H5FD_t *file, hid_t dxpl_id, unsigned closing)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5FDflush, FAIL)
    H5TRACE3("e","xiIu",file,dxpl_id,closing);

    /* Check args */
    if (!file || !file->cls)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer")
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(dxpl_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Do the real work */
    if (H5FD_flush(file,dxpl_id,closing)<0)
  HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "file flush request failed")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_flush
 *
 * Purpose:  Private version of H5FDflush()
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *              Quincey Koziol, May 20, 2002
 *              Added 'closing' parameter
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_flush(H5FD_t *file, hid_t dxpl_id, unsigned closing)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_flush, FAIL)

    assert(file && file->cls);

    /* Check if we need to flush out the metadata accumulator */
    if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) && file->accum_dirty && file->accum_size>0) {
        /* Flush the metadata contents */
        /* Not certain if the type and dxpl should be the way they are... -QAK */
        if ((file->cls->write)(file, H5FD_MEM_DEFAULT, dxpl_id, file->accum_loc, file->accum_size, file->meta_accum)<0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver write request failed")

        /* Reset the dirty flag */
        file->accum_dirty=FALSE;
    } /* end if */

    if (file->cls->flush && (file->cls->flush)(file,dxpl_id,closing)<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver flush request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5FD_get_fileno
 *
 * Purpose:  Quick and dirty routine to retrieve the file's 'fileno' value
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5FD_t data structure)
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *    March 27, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_get_fileno(const H5FD_t *file, unsigned long *filenum)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_get_fileno, FAIL)

    assert(file);
    assert(filenum);

    /* Retrieve the file's serial number */
    HDmemcpy(filenum,file->fileno,sizeof(file->fileno));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_fileno() */


/*--------------------------------------------------------------------------
 * Function:    H5FDget_vfd_handle
 *
 * Purpose:     Returns a pointer to the file handle of low-level virtual
 *              file driver.
 *
 * Return:      Non-negative if succeed; negative otherwise.
 *
 * Programmer:  Raymond Lu
 *              Sep. 16, 2002
 *
 * Modifications:
 *
 *--------------------------------------------------------------------------
 */
herr_t H5FDget_vfd_handle(H5FD_t *file, hid_t fapl, void** file_handle)
{
    herr_t              ret_value;

    FUNC_ENTER_API(H5FDget_vfd_handle, FAIL)

    /* Check arguments */
    assert(file);
    assert(file_handle);
    ret_value=H5FD_get_vfd_handle(file, fapl, file_handle);

done:
    FUNC_LEAVE_API(ret_value)
}


/*--------------------------------------------------------------------------
 * Function:    H5FD_get_vfd_handle
 *
 * Purpose:     Retrieve the file handle for file driver.
 *
 * Return:      Non-negative if succeed; negative if fails.
 *
 * Programmer:  Raymond Lu
 *              Sep. 16, 2002
 *
 * Modifications:
 *
 *--------------------------------------------------------------------------
 */
herr_t H5FD_get_vfd_handle(H5FD_t *file, hid_t fapl, void** file_handle)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_get_vfd_handle, FAIL)

    assert(file_handle);
    if(file->cls->get_handle && ((ret_value=file->cls->get_handle(file, fapl, file_handle)) < 0))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get file handle for file driver")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_get_freespace
 *
 * Purpose:     Retrieve the amount of free space in a file.
 *
 * Return:      Success:        Amount of free space in file
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, October  6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5FD_get_freespace(const H5FD_t *file)
{
    H5FD_free_t *free_node;     /* Pointer to node on free list */
    H5FD_mem_t type;            /* Type of memory */
    hssize_t ret_value;         /* Return value */

    FUNC_ENTER_NOAPI(H5FD_get_freespace, FAIL)

    /* check args */
    assert(file);
    assert(file->cls);

    /* Initialize return value */
    ret_value=0;

    /* Iterate over all the types of memory, to retrieve amount of free space for each */
    for (type=H5FD_MEM_DEFAULT; type<H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t,type)) {
        /* Iterate through the free list, accumulating the amount of free space for this type */
        free_node = file->fl[type];
        while(free_node) {
            ret_value+=(hssize_t)free_node->size;
            free_node=free_node->next;
        } /* end while */
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_freespace() */

