/*
 * Copyright © 1999-2001 NCSA
 *                       All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, July 26, 1999
 *
 * Purpose:     The Virtual File Layer as described in documentation. This is
 *              the greatest common denominator for all types of storage
 *              access whether a file, memory, network, etc. This layer
 *              usually just dispatches the request to an actual file driver
 *              layer.
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

/* Packages needed by this file */
#include "H5private.h"          /*library functions                     */
#include "H5Eprivate.h"         /*error handling                        */
#include "H5Fpkg.h"             /*files                                 */
#include "H5FDprivate.h"        /*virtual file driver                   */
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5Iprivate.h"         /*interface abstraction layer           */
#include "H5MMprivate.h"        /*memory management                     */
#include "H5Pprivate.h"         /*property lists                        */

/* Interface initialization */
#define PABLO_MASK      H5FD_mask
#define INTERFACE_INIT  H5FD_init_interface
static int interface_initialize_g = 0;

/* static prototypes */
static herr_t H5FD_init_interface(void);
static herr_t H5FD_free_cls(H5FD_class_t *cls);
static haddr_t H5FD_real_alloc(H5FD_t *file, H5FD_mem_t type, hsize_t size);

/* Declare a PQ free list to manage the metadata accumulator buffer */
H5FL_BLK_DEFINE_STATIC(meta_accum);


/*-------------------------------------------------------------------------
 * Function:    H5FD_init_interface
 *
 * Purpose:     Initialize the virtual file layer.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
    FUNC_ENTER(H5FD_init_interface, FAIL);

    if (H5I_init_group(H5I_VFL, H5I_VFL_HASHSIZE, 0,
                       (H5I_free_t)H5FD_free_cls)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "unable to initialize interface");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_term_interface
 *
 * Purpose:     Terminate this interface: free all memory and reset global
 *              variables to their initial values.  Release all ID groups
 *              associated with this interface.
 *
 * Return:      Success:        Positive if anything was done that might
 *                              have affected other interfaces; zero
 *                              otherwise.
 *
 *              Failure:        Never fails.
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
    int n = 0;

    if (interface_initialize_g) {
        if ((n=H5I_nmembers(H5I_VFL))) {
            H5I_clear_group(H5I_VFL, FALSE);
        } else {
            H5I_destroy_group(H5I_VFL);
            interface_initialize_g = 0;
            n = 1; /*H5I*/
        }
    }
    return n;
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_free_cls
 *
 * Purpose:     Frees a file driver class struct and returns an indication of
 *              success. This function is used as the free callback for the
 *              virtual file layer object identifiers (cf H5FD_init_interface).
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
    FUNC_ENTER(H5FD_free_cls, FAIL);
    H5MM_xfree(cls);
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDregister
 *
 * Purpose:     Registers a new file driver as a member of the virtual file
 *              driver class.  Certain fields of the class struct are
 *              required and that is checked here so it doesn't have to be
 *              checked every time the field is accessed.
 *
 * Return:      Success:        A file driver ID which is good until the
 *                              library is closed or the driver is
 *                              unregistered.
 *
 *              Failure:        A negative value.
 *
 * Programmer:  Robb Matzke
 *              Monday, July 26, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FDregister(const H5FD_class_t *cls)
{
    hid_t               retval;
    H5FD_class_t        *saved;
    H5FD_mem_t          type;

    FUNC_ENTER(H5FDregister, FAIL);
    H5TRACE1("i","x",cls);

    /* Check arguments */
    if (!cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL,
                      "null class pointer is disallowed");
    }

    if (!cls->open || !cls->close) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL,
                      "`open' and/or `close' methods are not defined");
    }

    if (!cls->get_eoa || !cls->set_eoa) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL,
                      "`get_eoa' and/or `set_eoa' methods are not defined");
    }

    if (!cls->get_eof) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL,
                      "`get_eof' method is not defined");
    }
    if (!cls->read || !cls->write) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL,
                      "`read' and/or `write' method is not defined");
    }
    for (type=H5FD_MEM_DEFAULT; type<H5FD_MEM_NTYPES; type++) {
        if (cls->fl_map[type]<H5FD_MEM_NOLIST ||
            cls->fl_map[type]>=H5FD_MEM_NTYPES) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                          "invalid free-list mapping");
        }
    }

    /* Copy the class structure so the caller can reuse or free it */
    if (NULL==(saved=H5MM_malloc(sizeof(H5FD_class_t)))) {
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                      "memory allocation failed for file driver class struct");
    }
    *saved = *cls;

    /* Create the new class ID */
    if ((retval=H5I_register(H5I_VFL, saved))<0) {
        H5MM_xfree(saved);
        HRETURN_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                      "unable to register file driver ID");
    }

    FUNC_LEAVE(retval);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDunregister
 *
 * Purpose:     Removes a driver ID from the library. This in no way affects
 *              file access property lists which have been defined to use
 *              this driver or files which are already opened under this
 *              driver.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
    FUNC_ENTER(H5FDunregister, FAIL);
    H5TRACE1("e","i",driver_id);

    /* Check arguments */
    if (H5I_VFL!=H5I_get_type(driver_id) ||
        NULL==H5I_object(driver_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not a file driver");
    }

    /* The H5FD_class_t struct will be freed by this function */
    if (H5I_dec_ref(driver_id)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "unable to unregister file driver");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_get_class
 *
 * Purpose:     Optains a pointer to the driver struct containing all the
 *              callback pointers, etc. The PLIST_ID argument can be a file
 *              access property list, a data transfer property list, or a
 *              file driver identifier.
 *
 * Return:      Success:        Ptr to the driver information. The pointer is
 *                              only valid as long as the driver remains
 *                              registered or some file or property list
 *                              exists which references the driver.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, August 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5FD_class_t *
H5FD_get_class(hid_t id)
{
    H5FD_class_t        *ret_value=NULL;
    H5F_access_t        *fapl=NULL;
    H5D_xfer_t          *dxpl=NULL;
    
    FUNC_ENTER(H5FD_get_class, NULL);

    if (H5P_DEFAULT==id) {
        ret_value = H5FD_get_class(H5F_access_dflt.driver_id);
    } else if (H5I_VFL==H5I_get_type(id)) {
        ret_value = H5I_object(id);
    } else {
        switch (H5P_get_class(id)) {
        case H5P_FILE_ACCESS:
            if (NULL==(fapl=H5I_object(id))) {
                HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL,
                              "not a file access property list");
            }
            ret_value = H5FD_get_class(fapl->driver_id);
            break;

        case H5P_DATASET_XFER:
            if (NULL==(dxpl=H5I_object(id))) {
                HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL,
                              "not a data transfer property list");
            }
            ret_value = H5FD_get_class(dxpl->driver_id);
            break;

        default:
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL,
                          "not a driver id, file access property list or "
                          "data transfer property list");
        }
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sb_size
 *
 * Purpose:     Obtains the number of bytes required to store the driver file
 *              access data in the HDF5 superblock.
 *
 * Return:      Success:        Number of bytes required.
 *
 *              Failure:        0 if an error occurs or if the driver has no
 *                              data to store in the superblock.
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
    hsize_t     ret_value=0;
    
    FUNC_ENTER(H5FD_sb_size, 0);

    assert(file && file->cls);
    if (file->cls->sb_size) {
        ret_value = (file->cls->sb_size)(file);
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sb_encode
 *
 * Purpose:     Encode driver-specific data into the output arguments. The
 *              NAME is a nine-byte buffer which should get an
 *              eight-character driver name and/or version followed by a null
 *              terminator. The BUF argument is a buffer to receive the
 *              encoded driver-specific data. The size of the BUF array is
 *              the size returned by the H5FD_sb_size() call.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
    FUNC_ENTER(H5FD_sb_encode, FAIL);

    assert(file && file->cls);
    if (file->cls->sb_encode &&
        (file->cls->sb_encode)(file, name/*out*/, buf/*out*/)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "driver sb_encode request failed");
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sb_decode
 *
 * Purpose:     Decodes the driver information block.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
    FUNC_ENTER(H5FD_sb_decode, FAIL);

    assert(file && file->cls);
    if (file->cls->sb_decode &&
        (file->cls->sb_decode)(file, name, buf)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "driver sb_decode request failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fapl_get
 *
 * Purpose:     Gets the file access property list associated with a file.
 *              Usually the file will copy what it needs from the original
 *              file access property list when the file is created. The
 *              purpose of this function is to create a new file access
 *              property list based on the settings in the file, which may
 *              have been modified from the original file access property
 *              list.
 *
 * Return:      Success:        Pointer to a new file access property list
 *                              with all members copied.  If the file is
 *                              closed then this property list lives on, and
 *                              vice versa.
 *
 *              Failure:        NULL, including when the file has no
 *                              properties.
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
    void        *ret_value=NULL;
    
    FUNC_ENTER(H5FD_fapl_get, NULL);
    assert(file);

    if (file->cls->fapl_get) {
        ret_value = (file->cls->fapl_get)(file);
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fapl_copy
 *
 * Purpose:     Copies the driver-specific part of the file access property
 *              list.
 *
 * Return:      Success:        Pointer to new driver-specific file access
 *                              properties.
 *
 *              Failure:        NULL, but also returns null with no error
 *                              pushed onto the error stack if the OLD_FAPL
 *                              is null.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5FD_fapl_copy(hid_t driver_id, const void *old_fapl)
{
    void                *new_fapl = NULL;
    H5FD_class_t        *driver=NULL;
    
    FUNC_ENTER(H5FD_fapl_copy, NULL);

    /* Check args */
    if (H5I_VFL!=H5I_get_type(driver_id) ||
        NULL==(driver=H5I_object(driver_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a driver ID");
    }
    if (!old_fapl) HRETURN(NULL); /*but no error*/

    /* Allow the driver to copy or do it ourselves */
    if (driver->fapl_copy) {
        new_fapl = (driver->fapl_copy)(old_fapl);
    } else if (driver->fapl_size>0) {
        new_fapl = H5MM_malloc(driver->fapl_size);
        HDmemcpy(new_fapl, old_fapl, driver->fapl_size);
    } else {
        HRETURN_ERROR(H5E_VFL, H5E_UNSUPPORTED, NULL,
                      "no way to copy driver file access property list");
    }

    FUNC_LEAVE(new_fapl);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fapl_free
 *
 * Purpose:     Frees the driver-specific file access property list.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fapl_free(hid_t driver_id, void *fapl)
{
    H5FD_class_t        *driver=NULL;

    FUNC_ENTER(H5FD_fapl_free, FAIL);
    H5TRACE2("e","ix",driver_id,fapl);

    /* Check args */
    if (H5I_VFL!=H5I_get_type(driver_id) ||
        NULL==(driver=H5I_object(driver_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID");
    }
    
    /* Allow driver to free or do it ourselves */
    if (fapl && driver->fapl_free) {
        if ((driver->fapl_free)(fapl)<0) {
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                          "driver fapl_free request failed");
        }
    } else {
        H5MM_xfree(fapl);
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dxpl_copy
 *
 * Purpose:     Copies the driver-specific part of the data transfer property
 *              list.
 *
 * Return:      Success:        Pointer to new driver-specific data transfer
 *                              properties.
 *
 *              Failure:        NULL, but also returns null with no error
 *                              pushed onto the error stack if the OLD_DXPL
 *                              is null.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5FD_dxpl_copy(hid_t driver_id, const void *old_dxpl)
{
    void                *new_dxpl = NULL;
    H5FD_class_t        *driver=NULL;
    
    FUNC_ENTER(H5FD_dxpl_copy, NULL);

    /* Check args */
    if (H5I_VFL!=H5I_get_type(driver_id) ||
        NULL==(driver=H5I_object(driver_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL,
                      "not a driver ID");
    }
    if (!old_dxpl) HRETURN(NULL); /*but no error*/

    /* Allow the driver to copy or do it ourselves */
    if (driver->dxpl_copy) {
        new_dxpl = (driver->dxpl_copy)(old_dxpl);
    } else if (driver->dxpl_size>0) {
        new_dxpl = H5MM_malloc(driver->dxpl_size);
        HDmemcpy(new_dxpl, old_dxpl, driver->dxpl_size);
    } else {
        HRETURN_ERROR(H5E_VFL, H5E_UNSUPPORTED, NULL,
                      "no way to copy driver file access property list");
    }

    FUNC_LEAVE(new_dxpl);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dxpl_free
 *
 * Purpose:     Frees the driver-specific data transfer property list.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_dxpl_free(hid_t driver_id, void *dxpl)
{
    H5FD_class_t        *driver=NULL;

    FUNC_ENTER(H5FD_dxpl_free, FAIL);
    H5TRACE2("e","ix",driver_id,dxpl);

    /* Check args */
    if (H5I_VFL!=H5I_get_type(driver_id) ||
        NULL==(driver=H5I_object(driver_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID");
    }
    
    /* Allow driver to free or do it ourselves */
    if (dxpl && driver->dxpl_free) {
        if ((driver->dxpl_free)(dxpl)<0) {
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                          "driver dxpl_free request failed");
        }
    } else {
        H5MM_xfree(dxpl);
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDopen
 *
 * Purpose:     Opens a file named NAME for the type(s) of access described
 *              by the bit vector FLAGS according to a file access property
 *              list FAPL_ID (which may be the constant H5P_DEFAULT). The
 *              file should expect to handle format addresses in the range [0,
 *              MAXADDR] (if MAXADDR is the undefined address then the caller
 *              doesn't care about the address range).
 *
 *              Possible values for the FLAGS bits are:
 *
 *              H5F_ACC_RDWR:   Open the file for read and write access. If
 *                              this bit is not set then open the file for
 *                              read only access. It is permissible to open a
 *                              file for read and write access when only read
 *                              access is requested by the library (the
 *                              library will never attempt to write to a file
 *                              which it opened with only read access).
 *
 *              H5F_ACC_CREATE: Create the file if it doesn't already exist.
 *                              However, see H5F_ACC_EXCL below.
 *
 *              H5F_ACC_TRUNC:  Truncate the file if it already exists. This
 *                              is equivalent to deleting the file and then
 *                              creating a new empty file.
 *
 *              H5F_ACC_EXCL:   When used with H5F_ACC_CREATE, if the file
 *                              already exists then the open should fail.
 *                              Note that this is unsupported/broken with
 *                              some file drivers (e.g., sec2 across nfs) and
 *                              will contain a race condition when used to
 *                              perform file locking.
 *
 *              The MAXADDR is the maximum address which will be requested by
 *              the library during an allocation operation. Usually this is
 *              the same value as the MAXADDR field of the class structure,
 *              but it can be smaller if the driver is being used under some
 *              other driver.
 *
 *              Note that when the driver `open' callback gets control that
 *              the public part of the file struct (the H5FD_t part) will be
 *              incomplete and will be filled in after that callback returns.
 *
 * Return:      Success:        Pointer to a new file driver struct.
 *
 *              Failure:        NULL
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
    H5FD_t      *ret_value=NULL;

    FUNC_ENTER(H5FDopen, NULL);

    if (NULL==(ret_value=H5FD_open(name, flags, fapl_id, maxaddr))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, NULL,
                      "unable to open file");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_open
 *
 * Purpose:     Private version of H5FDopen()
 *
 * Return:      Success:        Pointer to a new file driver struct
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5FD_t *
H5FD_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    const H5F_access_t  *fapl=NULL;
    H5FD_class_t        *driver;
    H5FD_t              *file=NULL;
    
    FUNC_ENTER(H5FD_open, NULL);

    /* Check arguments */
    if (H5P_DEFAULT==fapl_id) {
        fapl = &H5F_access_dflt;
    } else if (H5P_FILE_ACCESS != H5P_get_class(fapl_id) ||
               NULL == (fapl = H5I_object(fapl_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL,
                      "not a file access property list");
    }
    if (0==maxaddr) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                      "zero format address range");
    }

    /* Get driver info */
    if (H5I_VFL!=H5I_get_type(fapl->driver_id) ||
        NULL==(driver=H5I_object(fapl->driver_id))) {
        HRETURN_ERROR(H5E_VFL, H5E_BADVALUE, NULL,
                      "invalid driver ID in file access property list");
    }
    if (NULL==driver->open) {
        HRETURN_ERROR(H5E_VFL, H5E_UNSUPPORTED, NULL,
                      "file driver has no `open' method");
    }
    
    /* Dispatch to file driver */
    if (HADDR_UNDEF==maxaddr) maxaddr = driver->maxaddr;
    if (NULL==(file=(driver->open)(name, flags, fapl_id, maxaddr))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "open failed");
    }

    /*
     * Fill in public fields. We must increment the reference count on the
     * driver ID to prevent it from being freed while this file is open.
     */
    file->driver_id = fapl->driver_id;
    H5I_inc_ref(file->driver_id);
    file->cls = driver;
    file->maxaddr = maxaddr;
    HDmemset(file->fl, 0, sizeof(file->fl));
    file->def_meta_block_size = fapl->meta_block_size;
    file->accum_loc = HADDR_UNDEF;
    file->threshold = fapl->threshold;
    file->alignment = fapl->alignment;
    
    /* Retrieve the VFL driver feature flags */
    if (H5FD_query(file, &(file->feature_flags))<0)
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to query file driver");

    FUNC_LEAVE(file);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDclose
 *
 * Purpose:     Closes the file by calling the driver `close' callback, which
 *              should free all driver-private data and free the file struct.
 *              Note that the public part of the file struct (the H5FD_t part)
 *              will be all zero during the driver close callback like during
 *              the `open' callback.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
    FUNC_ENTER(H5FDclose, FAIL);
    H5TRACE1("e","x",file);

    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer");
    }

    if (H5FD_close(file)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "unable to close file");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_close
 *
 * Purpose:     Private version of H5FDclose()
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *              Robb Matzke, 2000-11-10
 *              Removed a call to set *file to all zero because the struct
 *              has already been freed by the close method. This fixes a write
 *              to freed memory.
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_close(H5FD_t *file)
{
    const H5FD_class_t  *driver;
    H5FD_free_t         *cur, *next;
    H5FD_mem_t          i;
#ifdef H5F_DEBUG
    unsigned            nblocks=0;
    hsize_t             nbytes=0;
#endif
    
    FUNC_ENTER(H5FD_close, FAIL);
    assert(file && file->cls);

    /* Free all free-lists, leaking any memory thus described. Also leaks
     * file space allocated but not used when metadata aggregation is
     * turned on. */
    for (i=H5FD_MEM_DEFAULT; i<H5FD_MEM_NTYPES; i++) {
        for (cur=file->fl[i]; cur; cur=next) {
#ifdef H5F_DEBUG
            nblocks++;
            nbytes += cur->size;
#endif
            next = cur->next;
            H5MM_xfree(cur);
        }
        file->fl[i]=NULL;
    }
#ifdef H5F_DEBUG
    if (nblocks && H5DEBUG(F)) {
        fprintf(H5DEBUG(F),
                "H5F: leaked %lu bytes of file memory in %u blocks\n",
                (unsigned long)nbytes, nblocks);
    }
#endif

    /* Check if we need to reset the metadata accumulator information */
    if(file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) {
        /* Free the buffer */
        if(file->meta_accum!=NULL)
            file->meta_accum=H5FL_BLK_FREE(meta_accum,file->meta_accum);
        
        /* Reset the buffer sizes & location */
        file->accum_buf_size=file->accum_size=0;
        file->accum_loc=HADDR_UNDEF;
        file->accum_dirty=0;
    } /* end if */

    /* Prepare to close file by clearing all public fields */
    driver = file->cls;
    H5I_dec_ref(file->driver_id);

    /*
     * Dispatch to the driver for actual close. If the driver fails to
     * close the file then the file will be in an unusable state.
     */
    assert(driver->close);
    if ((driver->close)(file)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "close failed");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDcmp
 *
 * Purpose:     Compare the keys of two files using the file driver callback
 *              if the files belong to the same driver, otherwise sort the
 *              files by driver class pointer value.
 *
 * Return:      Success:        A value like strcmp()
 *
 *              Failure:        Must never fail. If both file handles are
 *                              invalid then they compare equal. If one file
 *                              handle is invalid then it compares less than
 *                              the other.  If both files belong to the same
 *                              driver and the driver doesn't provide a
 *                              comparison callback then the file pointers
 *                              themselves are compared.
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
    int ret_value;
    
    FUNC_ENTER(H5FDcmp, -1); /*return value is arbitrary*/
    H5TRACE2("Is","xx",f1,f2);
    
    ret_value = H5FD_cmp(f1, f2);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_cmp
 *
 * Purpose:     Private version of H5FDcmp()
 *
 * Return:      Success:        A value like strcmp()
 *
 *              Failure:        Must never fail.
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
    int ret_value;

    FUNC_ENTER(H5FD_cmp, -1); /*return value is arbitrary*/

    if ((!f1 || !f1->cls) && (!f2 || !f2->cls)) HRETURN(0);
    if (!f1 || !f1->cls) HRETURN(-1);
    if (!f2 || !f2->cls) HRETURN(1);
    if (f1->cls < f2->cls) HRETURN(-1);
    if (f1->cls > f2->cls) HRETURN(1);

    /* Files are same driver; no cmp callback */
    if (!f1->cls->cmp) {
        if (f1<f2) HRETURN(-1);
        if (f1>f2) HRETURN(1);
        HRETURN(0);
    }

    ret_value = (f1->cls->cmp)(f1, f2);

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDquery
 *
 * Purpose:     Query a VFL driver for its feature flags. (listed in H5FDpublic.h)
 *
 * Return:      Success:    non-negative
 *
 *              Failure:        negative
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
    int ret_value;

    FUNC_ENTER(H5FDquery, FAIL);
    H5TRACE2("Is","xx",f,flags);
    
    assert(f);
    assert(flags);
    
    ret_value = H5FD_query(f, flags);

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_query
 *
 * Purpose:     Private version of H5FDquery()
 *
 * Return:      Success:    non-negative
 *
 *              Failure:        negative
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
    int ret_value=0;

    FUNC_ENTER(H5FD_query, FAIL);

    assert(f);
    assert(flags);
    
    /* Check for query driver and call it */
    if (f->cls->query)
        ret_value = (f->cls->query)(f, flags);
    else
        *flags=0;

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDalloc
 *
 * Purpose:     Allocates SIZE bytes of memory from the FILE. The memory will
 *              be used according to the allocation class TYPE. First we try
 *              to satisfy the request from one of the free lists, according
 *              to the free list map provided by the driver. The free list
 *              array has one entry for each request type and the value of
 *              that array element can be one of four possibilities:
 *
 *                    It can be the constant H5FD_MEM_DEFAULT (or zero) which
 *                    indicates that the identity mapping is used. In other
 *                    words, the request type maps to its own free list.
 *
 *                    It can be the request type itself, which has the same
 *                    effect as the H5FD_MEM_DEFAULT value above.
 *
 *                    It can be the ID for another request type, which
 *                    indicates that the free list for the specified type
 *                    should be used instead.
 *
 *                    It can be the constant H5FD_MEM_NOLIST which means that
 *                    no free list should be used for this type of request.
 *
 *              If the request cannot be satisfied from a free list then
 *              either the driver's `alloc' callback is invoked (if one was
 *              supplied) or the end-of-address marker is extended. The
 *              `alloc' callback is always called with the same arguments as
 *              the H5FDalloc().
 *
 * Return:      Success:        The format address of the new file memory.
 *
 *              Failure:        The undefined address HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 27, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDalloc(H5FD_t *file, H5FD_mem_t type, hsize_t size)
{
    haddr_t     ret_value = HADDR_UNDEF;
    
    FUNC_ENTER(H5FDalloc, HADDR_UNDEF);
    H5TRACE3("a","xMth",file,type,size);

    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF,
                      "invalid file pointer");
    }
    if (type<0 || type>=H5FD_MEM_NTYPES) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF,
                      "invalid request type");
    }
    if (size<=0) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF,
                      "zero-size request");
    }

    /* Do the real work */
    if (HADDR_UNDEF==(ret_value=H5FD_alloc(file, type, size))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF,
                      "unable to allocate file memory");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_alloc
 *
 * Purpose:     Private version of H5FDalloc()
 *
 * Return:      Success:        The format address of the new file memory.
 *
 *              Failure:        The undefined address HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *      Albert Cheng, 2001/05/01
 *      Implement the allocation by alignment/threshold.
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_alloc(H5FD_t *file, H5FD_mem_t type, hsize_t size)
{
    haddr_t     ret_value = HADDR_UNDEF;
    H5FD_mem_t  mapped_type;

    FUNC_ENTER(H5FD_alloc, HADDR_UNDEF);

    /* Check args */
    assert(file && file->cls);
    assert(type>=0 && type<H5FD_MEM_NTYPES);
    assert(size>0);
    
#ifdef H5F_DEBUG
    if (H5DEBUG(F)) {
        fprintf(H5DEBUG(F), "%s: alignment=%ld, threshold=%ld, size=%ld\n",
            FUNC, file->alignment, file->threshold, size);
    }
#endif
    /* Map the allocation request to a free list */
    if (H5FD_MEM_DEFAULT==file->cls->fl_map[type]) {
        mapped_type = type;
    } else {
        mapped_type = file->cls->fl_map[type];
    }

    /*
     * Try to satisfy the request from the free list.  Only perform the search
     * if the free list has the potential of satisfying the request.
     * Here, aligned requests are requests that are >= threshold and
     * alignment > 1.
     * For non-aligned request, first try to find an exact match, otherwise
     * use the best match which is the smallest size that meets the requested
     * size.
     * For aligned address request, find a block in the following order
     * of preferences:
     *   1. block address is aligned and exact match in size;
     *   2. block address is aligned with smallest size > requested size;
     *   3. block address is not aligned with smallest size >= requested size.
     */
    if (mapped_type>=0 && (0==file->maxsize || size<=file->maxsize)) {
        H5FD_free_t *prev=NULL, *best=NULL;
        H5FD_free_t *cur = file->fl[mapped_type];
        int     found_aligned = 0;
        int     need_aligned;
        hsize_t head;

        need_aligned = file->alignment > 1 && size >= file->threshold;
        while (cur) {
            file->maxsize = MAX(file->maxsize, cur->size);
            if (need_aligned) {
                if ((head = cur->addr % file->alignment) == 0) {
                    /* got aligned address*/
                    if (cur->size==size) {
                        /* exact match */
                        ret_value = cur->addr;

                        /*
                         * Make certain we don't hand out a block of raw data
                         * from the free list which overlaps with the metadata
                         * aggregation buffer (if it's turned on)
                         */
                        if(type==H5FD_MEM_DRAW &&
                                (file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) &&
                                H5F_addr_overlap(ret_value,size,file->accum_loc,file->accum_size)) {
                            ret_value=HADDR_UNDEF;
                        } /* end if */
                        else {
                            if (prev)
                                prev->next = cur->next;
                            else
                                file->fl[mapped_type] = cur->next;
                            H5MM_xfree(cur);
                            if (size==file->maxsize)
                                file->maxsize=0; /*unknown*/
                            HGOTO_DONE(ret_value);
                        } /* end else */
                    }
                    if (cur->size>size) {
                        if (!best || !found_aligned || cur->size<best->size) {
                            best = cur;
                            found_aligned = 1;
                        } /* end if */
                    } /* end if */
                } /* end if */
                else {
                    /* non-aligned address.
                     * check to see if this block is big enough to skip
                     * to the next aligned address and is still big enough
                     * for the requested size.
                     * the extra cur->size>head is for preventing unsigned
                     * underflow.
                     * (this can be improved by checking for an exact match
                     * after excluding the head. Such match is as good as
                     * the found_aligned case above.)
                     */
                    head = file->alignment - head;      /* actual head size */
                    if (!found_aligned &&
                            (cur->size > head && cur->size-head >= size) &&
                            (!best || cur->size < best->size)) {
                        best =cur;
                    } /* end if */
                } /* end if */
            } /* end if */
            else {
                /* !need_aligned */
                if (cur->size==size) {
                    ret_value = cur->addr;

                    /*
                     * Make certain we don't hand out a block of raw data
                     * from the free list which overlaps with the metadata
                     * aggregation buffer (if it's turned on)
                     */
                    if(type==H5FD_MEM_DRAW &&
                            (file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) &&
                            H5F_addr_overlap(ret_value,size,file->accum_loc,file->accum_size)) {
                        ret_value=HADDR_UNDEF;
                    } /* end if */
                    else {
                        if (prev)
                            prev->next = cur->next;
                        else
                            file->fl[mapped_type] = cur->next;
                        H5MM_xfree(cur);
                        if (size==file->maxsize)
                            file->maxsize=0; /*unknown*/
                        HGOTO_DONE(ret_value);
                    } /* end else */
                } /* end if */
                else
                    if (cur->size>size && (!best || cur->size<best->size)) {
                        best = cur;
                    } /* end if */
            } /* end else */
            prev = cur;
            cur = cur->next;
        } /* end while */

        /* Couldn't find exact match, use best fitting piece found */
        if (best) {
            if (best->size==file->maxsize)
                file->maxsize=0; /*unknown*/
            if (!need_aligned || found_aligned) {
                /* free only tail */
                ret_value = best->addr;

                /*
                 * Make certain we don't hand out a block of raw data
                 * from the free list which overlaps with the metadata
                 * aggregation buffer (if it's turned on)
                 */
                if(type==H5FD_MEM_DRAW &&
                        (file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) &&
                        H5F_addr_overlap(ret_value,size,file->accum_loc,file->accum_size)) {
                    ret_value=HADDR_UNDEF;
                } /* end if */
                else {
                    best->addr += size;     /* Reduce size of block on free list */
                    best->size -= size;
                    HGOTO_DONE(ret_value);
                } /* end else */
            } /* end if */
            else {
                /* Split into 3 pieces. */
                /* Keep the the head and tail in the freelist. */
                H5FD_free_t *tmp = NULL;

                head = file->alignment - (best->addr % file->alignment);
                ret_value = best->addr + head;

                /*
                 * Make certain we don't hand out a block of raw data
                 * from the free list which overlaps with the metadata
                 * aggregation buffer (if it's turned on)
                 */
                if(type==H5FD_MEM_DRAW &&
                        (file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) &&
                        H5F_addr_overlap(ret_value,size,file->accum_loc,file->accum_size)) {
                    ret_value=HADDR_UNDEF;
                } /* end if */
                else {
                    /* Attempt to allocate memory for temporary node */ 
                    tmp = H5MM_malloc(sizeof(H5FD_free_t));
#ifdef H5F_DEBUG
                    if (H5DEBUG(F)) {
                        HDfprintf(H5DEBUG(F),
                            "%s: 3 pieces, begin best->addr=%a, best->size=%Hd, "
                            "head=%Hd, size=%Hd\n",
                            FUNC, best->addr, best->size, head, size);
                    }
#endif
                    assert(tmp);                /* bark in debug mode */
                    if (tmp) {
                        if ((tmp->size = (best->size - head - size))) {
                            tmp->addr = best->addr + head + size;
                            tmp->next = best->next;
                            best->next = tmp;
                        } /* end if */
                        else {
                            /* no tail piece */
                            H5MM_xfree(tmp);
                        } /* end else */
                    } /* end if */
                    else {
                        /* Cannot keep the tail piece.  Leak file memory. */
                        /* (Only happens if memory allocation fails) */
                    } /* end else */
                    best->size = head;
                    HGOTO_DONE(ret_value);
                } /* end else */
            } /* end else */
        } /* end if */
    } /* end if */

#ifdef H5F_DEBUG
    if (H5DEBUG(F)) {
        fprintf(H5DEBUG(F), "%s: Could not allocate from freelists\n", FUNC);
    }
#endif

    /*
     * If the metadata aggregation feature is enabled for this VFL driver,
     * allocate "generic" metadata space and sub-allocate out of that, if
     * possible.  Otherwise just allocate through H5FD_real_alloc()
     */
    /* Allocate all types of metadata out of the metadata block */
    if((file->feature_flags&H5FD_FEAT_AGGREGATE_METADATA) && type!=H5FD_MEM_DRAW) {
        /* Check if the space requested is larger than the space left in the block */
        if(size>file->cur_meta_block_size) {
            /* Check if the block asked for is too large for a metadata block */
            if(size>=file->def_meta_block_size) {
                /* Allocate just enough room for this new block the regular way */
                ret_value=H5FD_real_alloc(file,type,size);
            } /* end if */
            else {
                /*
                 * Instead of just dropping the remainder of the block on the
                 * floor and leaving the space in the file unused, we should
                 * return this small piece of unused space to the free list
                 * management. - QAK
                 */

                /* Allocate another metadata block */
                file->eoma=H5FD_real_alloc(file,H5FD_MEM_DEFAULT,file->def_meta_block_size);
                file->cur_meta_block_size=file->def_meta_block_size;

                /* Allocate space out of the metadata block */
                ret_value=file->eoma;
                file->cur_meta_block_size-=size;
                file->eoma+=size;
            } /* end else */
        } /* end if */
        else {
            /* Allocate space out of the metadata block */
            ret_value=file->eoma;
            file->cur_meta_block_size-=size;
            file->eoma+=size;
        } /* end else */
    } /* end if */
    else { /* Allocate data the regular way */
        ret_value=H5FD_real_alloc(file,type,size);
    } /* end else */

done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_real_alloc
 *
 * Purpose:     Double private version of H5FDalloc() :-)
 *
 * Return:      Success:        The format address of the new file memory.
 *
 *              Failure:        The undefined address HADDR_UNDEF
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *      Albert Cheng, 2001/05/01
 *      Implement the allocation by alignment/threshold.
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_real_alloc(H5FD_t *file, H5FD_mem_t type, hsize_t size)
{
    haddr_t     ret_value = HADDR_UNDEF;

    FUNC_ENTER(H5FD_real_alloc, HADDR_UNDEF);

    /* Check args */
    assert(file && file->cls);
    assert(type>=0 && type<H5FD_MEM_NTYPES);
    assert(size>0);
    
    /*
     * Dispatch to driver `alloc' callback or extend the end-of-address
     * marker
     */
    if (file->cls->alloc) {
        ret_value = (file->cls->alloc)(file, type, size);
        if (HADDR_UNDEF==ret_value) {
            HRETURN_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF,
                  "driver allocation request failed");
        }
    } else {
        hsize_t wasted;
        haddr_t oldeoa=0;
        haddr_t eoa = (file->cls->get_eoa)(file);

#ifdef H5F_DEBUG
        if (file->alignment * file->threshold != 1 && H5DEBUG(F)) {
            fprintf(H5DEBUG(F),
                "%s: alignment=%ld, threshold=%ld, size=%ld, Begin eoa=%ld\n",
                FUNC, file->alignment, file->threshold, size, eoa);
        }
#endif
        /* wasted is 0 if not exceeding threshold or eoa happens to be aligned*/
        wasted = (size>=file->threshold) ? (eoa % file->alignment) : 0;
        if (wasted){
            wasted = file->alignment - wasted;  /* actual waste */
            oldeoa = eoa;                       /* save it for later freeing */
            /* advance eoa to the next alignment by allocating the wasted */
            if (H5F_addr_overflow(eoa, wasted) || eoa+wasted>file->maxaddr) {
                HRETURN_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF,
                      "file allocation request failed");
            }
            eoa += wasted;
            if ((file->cls->set_eoa)(file, eoa)<0) {
                HRETURN_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF,
                      "file allocation request failed");
            }
        }

        /* allocate the aligned memory */
        if (H5F_addr_overflow(eoa, size) || eoa+size>file->maxaddr) {
            HRETURN_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF,
                  "file allocation request failed");
        }
        ret_value = eoa;
        eoa += size;
        if ((file->cls->set_eoa)(file, eoa)<0) {
            HRETURN_ERROR(H5E_VFL, H5E_NOSPACE, HADDR_UNDEF,
                  "file allocation request failed");
        }

        /* Free the wasted memory */
        if (wasted)
            H5FDfree(file, type, oldeoa, wasted);

#ifdef H5F_DEBUG
        if (file->alignment * file->threshold != 1 && H5DEBUG(F)) {
            fprintf(H5DEBUG(F),
                "%s: ret_value=%ld, wasted=%ld, Ended eoa=%ld\n",
                FUNC, ret_value, wasted, eoa);
        }
#endif
    }

    FUNC_LEAVE(ret_value);
} /* end H5FD_real_alloc() */



/*-------------------------------------------------------------------------
 * Function:    H5FDfree
 *
 * Purpose:     Frees format addresses starting with ADDR and continuing for
 *              SIZE bytes in the file FILE. The type of space being freed is
 *              specified by TYPE, which is mapped to a free list as
 *              described for the H5FDalloc() function above.  If the request
 *              doesn't map to a free list then either the application `free'
 *              callback is invoked (if defined) or the memory is leaked.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, July 28, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDfree(H5FD_t *file, H5FD_mem_t type, haddr_t addr, hsize_t size)
{
    FUNC_ENTER(H5FDfree, FAIL);
    H5TRACE4("e","xMtah",file,type,addr,size);
    
    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer");
    }
    if (type<0 || type>=H5FD_MEM_NTYPES) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid request type");
    }

    /* Do the real work */
    if (H5FD_free(file, type, addr, size)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "file deallocation request failed");
    }

    FUNC_LEAVE(SUCCEED);
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
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_free(H5FD_t *file, H5FD_mem_t type, haddr_t addr, hsize_t size)
{
    H5FD_mem_t          mapped_type;
    
    FUNC_ENTER(H5FD_free, FAIL);

    /* Check args */
    assert(file && file->cls);
    assert(type>=0 && type<H5FD_MEM_NTYPES);
    if (!H5F_addr_defined(addr) || addr>file->maxaddr || 0==size ||
            H5F_addr_overflow(addr, size) || addr+size>file->maxaddr) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid region");
    }

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
    if (mapped_type>=0) {
        H5FD_free_t *cur = H5MM_malloc(sizeof(H5FD_free_t));

        cur->addr = addr;
        cur->size = size;
        cur->next = file->fl[mapped_type];
        file->fl[mapped_type] = cur;
        file->maxsize = MAX(file->maxsize, size);
    } else if (file->cls->free) {
        if ((file->cls->free)(file, type, addr, size)<0) {
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                  "driver free request failed");
        }
    } else {
        /* leak memory */
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDrealloc
 *
 * Purpose:     Changes the size of an allocated chunk of memory, possibly
 *              also changing its location in the file.
 *
 * Return:      Success:        New address of the block of memory, not
 *                              necessarily the same as the original address.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  3, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDrealloc(H5FD_t *file, H5FD_mem_t type, haddr_t old_addr, hsize_t old_size,
            hsize_t new_size)
{
    haddr_t     ret_value=HADDR_UNDEF;

    FUNC_ENTER(H5FDrealloc, HADDR_UNDEF);
    H5TRACE5("a","xMtahh",file,type,old_addr,old_size,new_size);

    if (HADDR_UNDEF==(ret_value=H5FD_realloc(file, type, old_addr, old_size,
                                             new_size))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF,
                      "file reallocation request failed");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_realloc
 *
 * Purpose:     Private version of H5FDrealloc()
 *
 * Return:      Success:        New address of the block of memory, not
 *                              necessarily the same as the original address.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_realloc(H5FD_t *file, H5FD_mem_t type, haddr_t old_addr, hsize_t old_size,
             hsize_t new_size)
{
    haddr_t     new_addr=old_addr;
    uint8_t     _buf[8192];
    uint8_t     *buf=_buf;
    
    FUNC_ENTER(H5FD_realloc, HADDR_UNDEF);

    if (new_size==old_size) {
        /*nothing to do*/
        
    } else if (0==old_size) {
        /* allocate memory */
        assert(!H5F_addr_defined(old_addr));
        if (HADDR_UNDEF==(new_addr=H5FDalloc(file, type, new_size))) {
            HRETURN_ERROR(H5E_FILE, H5E_NOSPACE, HADDR_UNDEF,
                  "file allocation failed");
        }
    } else if (0==new_size) {
        /* free memory */
        assert(H5F_addr_defined(old_addr));
        H5FDfree(file, type, old_addr, old_size);
        new_addr = HADDR_UNDEF;
        
    } else if (new_size<old_size) {
        /* free the end of the block */
        H5FDfree(file, type, old_addr+old_size, old_size-new_size);
    } else {
        /* move memory to new location */
        if (HADDR_UNDEF==(new_addr=H5FDalloc(file, type, new_size))) {
            HRETURN_ERROR(H5E_FILE, H5E_NOSPACE, HADDR_UNDEF,
                  "file allocation failed");
        }
        assert(old_size==(hsize_t)((size_t)old_size)); /*check for overflow*/
        if (old_size>sizeof(_buf) && NULL==(buf=H5MM_malloc((size_t)old_size))) {
            H5FDfree(file, type, new_addr, new_size);
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF,
                  "memory allocation failed");
        }
        if (H5FDread(file, type, H5P_DEFAULT, old_addr, old_size, buf)<0 ||
                H5FDwrite(file, type, H5P_DEFAULT, new_addr, old_size, buf)<0) {
            H5FDfree(file, type, new_addr, new_size);
            H5MM_xfree(buf);
            HRETURN_ERROR(H5E_FILE, H5E_READERROR, HADDR_UNDEF,
                  "unable to move file block");
        }
        
        if (buf!=_buf)
            H5MM_xfree(buf);
        H5FDfree(file, type, old_addr, old_size);
    }

    FUNC_LEAVE(new_addr);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDget_eoa
 *
 * Purpose:     Returns the address of the first byte after the last
 *              allocated memory in the file.
 *
 * Return:      Success:        First byte after allocated memory.
 *
 *              Failure:        HADDR_UNDEF
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
    haddr_t     addr;

    FUNC_ENTER(H5FDget_eoa, HADDR_UNDEF);
    H5TRACE1("a","x",file);

    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF,
                      "invalid file pointer");
    }

    /* The real work */
    if (HADDR_UNDEF==(addr=H5FD_get_eoa(file))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF,
                      "file get eoa request failed");
    }

    FUNC_LEAVE(addr);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_get_eoa
 *
 * Purpose:     Private version of H5FDget_eoa()
 *
 * Return:      Success:        First byte after allocated memory.
 *
 *              Failure:        HADDR_UNDEF
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
    haddr_t     addr;
    
    FUNC_ENTER(H5FD_get_eoa, HADDR_UNDEF);
    assert(file && file->cls);
    
    /* Dispatch to driver */
    if (HADDR_UNDEF==(addr=(file->cls->get_eoa)(file))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF,
                      "driver get_eoa request failed");
    }

    FUNC_LEAVE(addr);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDset_eoa
 *
 * Purpose:     Set the end-of-address marker for the file. The ADDR is the
 *              address of the first byte past the last allocated byte of the
 *              file. This function is called from two places:
 *
 *                  It is called after an existing file is opened in order to
 *                  "allocate" enough space to read the superblock and then
 *                  to "allocate" the entire hdf5 file based on the contents
 *                  of the superblock.
 *
 *                  It is called during file memory allocation if the
 *                  allocation request cannot be satisfied from the free list
 *                  and the driver didn't supply an allocation callback.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative, no side effect
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
    FUNC_ENTER(H5FDset_eoa, FAIL);
    H5TRACE2("e","xa",file,addr);

    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer");
    }
    if (!H5F_addr_defined(addr) || addr>file->maxaddr) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "invalid end-of-address value");
    }

    /* The real work */
    if (H5FD_set_eoa(file, addr)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "file set eoa request failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_set_eoa
 *
 * Purpose:     Private version of H5FDset_eoa()
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative, no side effect
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
    FUNC_ENTER(H5FD_set_eoa, FAIL);
    assert(file && file->cls);
    assert(H5F_addr_defined(addr) && addr<=file->maxaddr);
    
    /* Dispatch to driver */
    if ((file->cls->set_eoa)(file, addr)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "driver set_eoa request failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDget_eof
 *
 * Purpose:     Returns the end-of-file address, which is the greater of the
 *              end-of-format address and the actual EOF marker. This
 *              function is called after an existing file is opened in order
 *              for the library to learn the true size of the underlying file
 *              and to determine whether the hdf5 data has been truncated.
 *
 *              It is also used when a file is first opened to learn whether
 *              the file is empty or not.
 *
 *              It is permissible for the driver to return the maximum address
 *              for the file size if the file is not empty.
 *
 * Return:      Success:        The EOF address.
 *
 *              Failure:        HADDR_UNDEF
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
    haddr_t     addr;
    
    FUNC_ENTER(H5FDget_eof, HADDR_UNDEF);
    H5TRACE1("a","x",file);

    /* Check arguments */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF,
                      "invalid file pointer");
    }

    /* The real work */
    if (HADDR_UNDEF==(addr=H5FD_get_eof(file))) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF,
                      "file get eof request failed");
    }

    FUNC_LEAVE(addr);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_get_eof
 *
 * Purpose:     Private version of H5FDget_eof()
 *
 * Return:      Success:        The EOF address.
 *
 *              Failure:        HADDR_UNDEF
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
    haddr_t     addr=HADDR_UNDEF;

    FUNC_ENTER(H5FD_get_eof, HADDR_UNDEF);
    assert(file && file->cls);
    
    /* Dispatch to driver */
    if (file->cls->get_eof) {
        if (HADDR_UNDEF==(addr=(file->cls->get_eof)(file))) {
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF,
                          "driver get_eof request failed");
        }
    } else {
        addr = file->maxaddr;
    }

    FUNC_LEAVE(addr);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDread
 *
 * Purpose:     Reads SIZE bytes from FILE beginning at address ADDR
 *              according to the data transfer property list DXPL_ID (which may
 *              be the constant H5P_DEFAULT). The result is written into the
 *              buffer BUF.
 *
 * Return:      Success:        Non-negative. The read result is written into
 *                              the BUF buffer which should be allocated by
 *                              the caller.
 *
 *              Failure:        Negative. The contents of BUF is undefined.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDread(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size,
         void *buf/*out*/)
{
    FUNC_ENTER(H5FDread, FAIL);
    H5TRACE6("e","xMtiahx",file,type,dxpl_id,addr,size,buf);

    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer");
    }
    if (H5P_DEFAULT!=dxpl_id &&
        (H5P_DATASET_XFER!=H5P_get_class(dxpl_id) ||
         NULL==H5I_object(dxpl_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not a data transfer property list");
    }
    if (!buf) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null result buffer");
    }

    /* Do the real work */
    if (H5FD_read(file, type, dxpl_id, addr, size, buf)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "file read request failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_read
 *
 * Purpose:     Private version of H5FDread()
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *      Albert Cheng, 2000-11-21
 *      Disable the code that does early return when size==0 for
 *      Parallel mode since a collective call would require the process
 *      to continue on with "nothing" to transfer.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_read(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size,
          void *buf/*out*/)
{
    FUNC_ENTER(H5FD_read, FAIL);
    assert(file && file->cls);
    assert(H5P_DEFAULT==dxpl_id ||
           (H5P_DATASET_XFER==H5P_get_class(dxpl_id) || H5I_object(dxpl_id)));
    assert(buf);

#ifndef H5_HAVE_PARALLEL
    /* Do not return early for Parallel mode since the I/O could be a */
    /* collective transfer. */
    /* The no-op case */
    if (0==size) HRETURN(SUCCEED);
#endif

    /* Check if this information is in the metadata accumulator */
    if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) &&
        H5F_addr_overlap(addr,size,file->accum_loc,file->accum_size)) {

        unsigned char *read_buf=(unsigned char *)buf; /* Pointer to the buffer being read in */
        hsize_t amount_read;        /* Amount to read at a time */
        hsize_t read_off;           /* Offset to read from */

        /* Double check that we aren't reading raw data */
        assert(type!=H5FD_MEM_DRAW);

        /* Read the part before the metadata accumulator */
        if(addr<file->accum_loc) {
            /* Set the amount to read */
            amount_read=file->accum_loc-addr;

            /* Dispatch to driver */
            if ((file->cls->read)(file, type, dxpl_id, addr, amount_read, read_buf)<0)
                HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver read request failed");

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
            amount_read=MIN((file->accum_size-read_off),size);

            /* Copy the data out of the buffer */
            assert(amount_read==(hsize_t)((size_t)amount_read)); /*check for overflow*/
            HDmemcpy(read_buf,file->meta_accum+read_off,(size_t)amount_read);

            /* Adjust the buffer, address & size */
            read_buf+=amount_read;
            addr+=amount_read;
            size-=amount_read;
        } /* end if */

        /* Read the part after the metadata accumulator */
        if(size>0 && addr>=(file->accum_loc+file->accum_size)) {
            /* Dispatch to driver */
            if ((file->cls->read)(file, type, dxpl_id, addr, size, read_buf)<0)
                HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver read request failed");

            /* Adjust the buffer, address & size */
            read_buf+=size;
            addr+=size;
            size-=size;
        } /* end if */

        /* Make certain we've read it all */
        assert(size==0);
    } /* end if */
    else {
        /* Dispatch to driver */
        if ((file->cls->read)(file, type, dxpl_id, addr, size, buf)<0)
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver read request failed");
    } /* end else */

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDwrite
 *
 * Purpose:     Writes SIZE bytes to FILE beginning at address ADDR according
 *              to the data transfer property list DXPL_ID (which may be the
 *              constant H5P_DEFAULT). The bytes to be written come from the
 *              buffer BUF.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDwrite(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size,
          const void *buf)
{
    FUNC_ENTER(H5FDwrite, FAIL);
    H5TRACE6("e","xMtiahx",file,type,dxpl_id,addr,size,buf);

    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer");
    }
    if (H5P_DEFAULT!=dxpl_id &&
        (H5P_DATASET_XFER!=H5P_get_class(dxpl_id) ||
         NULL==H5I_object(dxpl_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not a data transfer property list");
    }
    if (!buf) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null buffer");
    }

    /* The real work */
    if (H5FD_write(file, type, dxpl_id, addr, size, buf)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "file write request failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_write
 *
 * Purpose:     Private version of H5FDwrite()
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *      Albert Cheng, 2000-11-21
 *      Disable the code that does early return when size==0 for
 *      Parallel mode since a collective call would require the process
 *      to continue on with "nothing" to transfer.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_write(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size,
           const void *buf)
{
    hsize_t new_size;    /* New size of the accumulator buffer */
    size_t old_offset;  /* Offset of old data within the accumulator buffer */

    FUNC_ENTER(H5FD_write, FAIL);
    assert(file && file->cls);
    assert(H5P_DEFAULT==dxpl_id ||
           (H5P_DATASET_XFER==H5P_get_class(dxpl_id) && H5I_object(dxpl_id)));
    assert(buf);
    
#ifndef H5_HAVE_PARALLEL
    /* Do not return early for Parallel mode since the I/O could be a */
    /* collective transfer. */
    /* The no-op case */
    if (0==size) HRETURN(SUCCEED);
#endif

    /* Check for accumulating metadata */
    if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) && type!=H5FD_MEM_DRAW) {
        /* Check if there is already metadata in the accumulator */
        if(file->accum_size>0) {
            /* Check if the piece of metadata being written adjoins or is inside the metadata accumulator */
            if((addr>=file->accum_loc && addr <(file->accum_loc+file->accum_size))
                || ((addr+size)>file->accum_loc && (addr+size)<=(file->accum_loc+file->accum_size))
                || (addr<file->accum_loc && (addr+size)>file->accum_loc)) {

                /* Check if the new metadata adjoins the beginning of the current accumulator */
                if((addr+size)==file->accum_loc) {
                    /* Check if we need more buffer space */
                    if((size+file->accum_size)>file->accum_buf_size) {
                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size+file->accum_size))==NULL)
                            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer");

                        /* Note the new buffer size */
                        file->accum_buf_size=size+file->accum_size;
                    } /* end if */

                    /* Move the existing metadata to the proper location */
                    assert(file->accum_size==(hsize_t)((size_t)file->accum_size)); /*check for overflow*/
                    HDmemmove(file->meta_accum+size,file->meta_accum,(size_t)file->accum_size);

                    /* Copy the new metadata at the front */
                    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                    HDmemcpy(file->meta_accum,buf,(size_t)size);

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
                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size+file->accum_size))==NULL)
                            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer");

                        /* Note the new buffer size */
                        file->accum_buf_size=size+file->accum_size;
                    } /* end if */

                    /* Copy the new metadata to the end */
                    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                    HDmemcpy(file->meta_accum+file->accum_size,buf,(size_t)size);

                    /* Set the new size of the metadata accumulator */
                    file->accum_size=file->accum_size+size;

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata is entirely within the current accumulator */
                else if(addr>=file->accum_loc && (addr+size)<=(file->accum_loc+file->accum_size)) {
                    /* Copy the new metadata to the proper location within the accumulator */
                    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                    HDmemcpy(file->meta_accum+(addr-file->accum_loc),buf,(size_t)size);

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata overlaps the beginning of the current accumulator */
                else if(addr<file->accum_loc && (addr+size)<=(file->accum_loc+file->accum_size)) {
                    /* Calculate the new accumulator size, based on the amount of overlap */
                    new_size=(file->accum_loc-addr)+file->accum_size;

                    /* Check if we need more buffer space */
                    if(new_size>file->accum_buf_size) {
                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,new_size))==NULL)
                            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer");

                        /* Note the new buffer size */
                        file->accum_buf_size=new_size;
                    } /* end if */

                    /* Calculate the proper offset of the existing metadata */
                    old_offset=(addr+size)-file->accum_loc;

                    /* Move the existing metadata to the proper location */
                    assert((file->accum_size-old_offset)==(hsize_t)((size_t)(file->accum_size-old_offset))); /*check for overflow*/
                    HDmemmove(file->meta_accum+size,file->meta_accum+old_offset,(size_t)(file->accum_size-old_offset));

                    /* Copy the new metadata at the front */
                    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                    HDmemcpy(file->meta_accum,buf,(size_t)size);

                    /* Set the new size & location of the metadata accumulator */
                    file->accum_loc=addr;
                    file->accum_size=new_size;

                    /* Mark it as written to */
                    file->accum_dirty=TRUE;
                } /* end if */
                /* Check if the new metadata overlaps the end of the current accumulator */
                else if(addr>=file->accum_loc && (addr+size)>(file->accum_loc+file->accum_size)) {
                    /* Calculate the new accumulator size, based on the amount of overlap */
                    new_size=(addr-file->accum_loc)+size;

                    /* Check if we need more buffer space */
                    if(new_size>file->accum_buf_size) {
                        /* Reallocate the metadata accumulator buffer */
                        if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,new_size))==NULL)
                            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer");

                        /* Note the new buffer size */
                        file->accum_buf_size=new_size;
                    } /* end if */

                    /* Copy the new metadata to the end */
                    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                    HDmemcpy(file->meta_accum+(addr-file->accum_loc),buf,(size_t)size);

                    /* Set the new size & location of the metadata accumulator */
                    file->accum_loc=addr;
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
                if ((file->cls->write)(file, H5FD_MEM_DEFAULT, dxpl_id, file->accum_loc, file->accum_size, file->meta_accum)<0)
                    HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver write request failed");

                /* Cache the new piece of metadata */
                /* Check if we need to reallocate the buffer */
                if(size>file->accum_buf_size) {
                    /* Reallocate the metadata accumulator buffer */
                    if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size))==NULL)
                        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer");

                    /* Note the new buffer size */
                    file->accum_buf_size=size;
                } /* end if */

                /* Store the piece of metadata in the accumulator */
                file->accum_loc=addr;
                file->accum_size=size;
                file->accum_dirty=TRUE;
                assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                HDmemcpy(file->meta_accum,buf,(size_t)size);
            } /* end else */
        } /* end if */
        /* No metadata in the accumulator, grab this piece and keep it */
        else {
            /* Check if we need to reallocate the buffer */
            if(size>file->accum_buf_size) {
                /* Reallocate the metadata accumulator buffer */
                if ((file->meta_accum=H5FL_BLK_REALLOC(meta_accum,file->meta_accum,size))==NULL)
                    HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer");

                /* Note the new buffer size */
                file->accum_buf_size=size;
            } /* end if */

            /* Store the piece of metadata in the accumulator */
            file->accum_loc=addr;
            file->accum_size=size;
            file->accum_dirty=TRUE;
            assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
            HDmemcpy(file->meta_accum,buf,(size_t)size);
        } /* end else */
    } /* end if */
    else {
        /* Dispatch to driver */
        if ((file->cls->write)(file, type, dxpl_id, addr, size, buf)<0)
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver write request failed");
    } /* end else */

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FDflush
 *
 * Purpose:     Notify driver to flush all cached data.  If the driver has no
 *              flush method then nothing happens.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDflush(H5FD_t *file)
{
    FUNC_ENTER(H5FDflush, FAIL);
    H5TRACE1("e","x",file);

    /* Check args */
    if (!file || !file->cls) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file pointer");
    }

    /* Do the real work */
    if (H5FD_flush(file)<0) {
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                      "file flush request failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_flush
 *
 * Purpose:     Private version of H5FDflush()
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_flush(H5FD_t *file)
{
    FUNC_ENTER(H5FD_flush, FAIL);
    assert(file && file->cls);

    /* Check if we need to flush out the metadata accumulator */
    if((file->feature_flags&H5FD_FEAT_ACCUMULATE_METADATA) && file->accum_dirty && file->accum_size>0) {
        /* Flush the metadata contents */
        /* Not certain if the type and dxpl should be the way they are... -QAK */
        if ((file->cls->write)(file, H5FD_MEM_DEFAULT, H5P_DEFAULT, file->accum_loc, file->accum_size, file->meta_accum)<0)
            HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver write request failed");

        /* Reset the dirty flag */
        file->accum_dirty=FALSE;
    } /* end if */

    if (file->cls->flush && (file->cls->flush)(file)<0)
        HRETURN_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver flush request failed");

    FUNC_LEAVE(SUCCEED);
}
