/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Thomas Radke <tradke@aei-potsdam.mpg.de>
 *              Monday, October 11, 1999
 *
 * Purpose:     This is the Distributed Parallel Storage System I/O driver.
 *
 */

#include "hdf5.h"
#include "H5Eprivate.h"         /*error handling                          */
#include "H5FDprivate.h"        /*file driver                             */
#include "H5FDdpss.h"
#include "H5MMprivate.h"        /*memory management                       */

#ifdef COALESCE_READS
/* Packages needed by this file.*/
#include "H5Fprivate.h"
#include "H5Iprivate.h"
#endif


/* uncomment this to get some debugging output from this driver */
#if 1
#define DEBUG 1
#endif

/* The driver identification number, initialized at runtime */
static hid_t H5FD_DPSS_g = 0;
hid_t GetH5FD_DPSS_g()
{
  return H5FD_DPSS_g;
}


/* compile this only if HDF5 was configured to use the Grid Storage I/O driver */
#ifdef H5_HAVE_GRIDSTORAGE

/* include the Storage Client header */
#include <grid_storage_file.h>

/*
 * The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying Unix file).
 * When opening a file the `eof' will be set to the current file size,
 * `eoa' will be set * to zero.
 */
typedef struct H5FD_dpss_t {
    H5FD_t                pub;            /* public stuff, must be first    */
    grid_storage_handle_t handle;         /* the DPSS file handle           */
    haddr_t               eoa;            /* end of allocated region        */
    haddr_t               eof;            /* end of file; current file size */
} H5FD_dpss_t;



/*
 * This driver supports systems that have the lseek64() function by defining
 * some macros here so we don't have to have conditional compilations later
 * throughout the code.
 *
 * file_offset_t:       The datatype for file offsets, the second argument of
 *                      the lseek() or lseek64() call.
 *
 * file_seek:           The function which adjusts the current file position,
 *                      either lseek() or lseek64().
 */
#ifdef H5_HAVE_LSEEK64
#   define file_offset_t        off64_t
#   define file_seek            lseek64
#else
#   define file_offset_t        off_t
#   define file_seek            lseek
#endif

/*
 * These macros check for overflow of various quantities.  These macros
 * assume that file_offset_t is signed and haddr_t and size_t are unsigned.
 * 
 * ADDR_OVERFLOW:    Checks whether a file address of type `haddr_t'
 *            is too large to be represented by the second argument
 *            of the file seek function.
 *
 * SIZE_OVERFLOW:    Checks whether a buffer size of type `hsize_t' is too
 *            large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW:    Checks whether an address and size pair describe data
 *            which can be addressed entirely by the second
 *            argument of the file seek function.
 */
#define MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define ADDR_OVERFLOW(A)    (HADDR_UNDEF==(A) ||                  \
                 ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)    ((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)    (ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
                 sizeof(file_offset_t)<sizeof(size_t) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||              \
                 (file_offset_t)((A)+(Z))<(file_offset_t)(A))

#define PRINT_GLOBUS_ERROR_MSG(globus_result)                                 \
        {                                                                     \
            char *globus_error_msg;                                           \
                                                                              \
            globus_error_msg = globus_object_printable_to_string (            \
                                   globus_error_get (globus_result));         \
            if (globus_error_msg) {                                           \
                /*** FIXME: put appropriate minor error code in here ***/     \
                HERROR (H5E_IO, -1, globus_error_msg);                        \
                globus_free (globus_error_msg);                               \
            }                                                                 \
        }

/* Grid Storage driver function prototypes */
static H5FD_t *H5FD_dpss_open (const char *name, unsigned flags,
                               hid_t UNUSED fapl_id, haddr_t maxaddr);
static herr_t H5FD_dpss_close (H5FD_t *_file);
static herr_t H5FD_dpss_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_dpss_get_eoa (H5FD_t *_file);
static herr_t H5FD_dpss_set_eoa (H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_dpss_get_eof (H5FD_t *_file);
static herr_t H5FD_dpss_read (H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                              hsize_t size, void *buf);
static herr_t H5FD_dpss_write (H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED fapl_id,haddr_t addr,
                               hsize_t size, const void *buf);

/* The Grid Storage I/O driver information */
static const H5FD_class_t H5FD_dpss_g = {
    "dpss",                                        /* name           */
    MAXADDR,                                       /* maxaddr        */
    NULL,                                          /* sb_size        */
    NULL,                                          /* sb_encode      */
    NULL,                                          /* sb_decode      */
    0,                                             /* fapl_size      */
    NULL,                                          /* fapl_get       */
    NULL,                                          /* fapl_copy      */
    NULL,                                          /* fapl_free      */
    0,                                             /* dxpl_size      */
    NULL,                                          /* dxpl_copy      */
    NULL,                                          /* dxpl_free      */
    H5FD_dpss_open,                                /* open           */
    H5FD_dpss_close,                               /* close          */
    NULL,                                          /* cmp            */
    H5FD_dpss_query,                               /* query          */
    NULL,                                          /* alloc          */
    NULL,                                          /* free           */
    H5FD_dpss_get_eoa,                             /* get_eoa        */
    H5FD_dpss_set_eoa,                             /* set_eoa        */
    H5FD_dpss_get_eof,                             /* get_eof        */
    H5FD_dpss_read,                                /* read           */
    H5FD_dpss_write,                               /* write          */
    NULL,                                          /* flush          */
    H5FD_FLMAP_SINGLE,                             /* fl_map         */
};

/* Interface initialization */
#define PABLO_MASK      H5FD_dpss_mask
#define INTERFACE_INIT  H5FD_dpss_init
static intn interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:    The driver ID for the DPSS driver.
 *
 *              Failure:    Negative.
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_dpss_init (void)
{
    int globus_result;           /* NOTE: globus_module_activate() doesn't
                                          return a globus_result_t type */

    FUNC_ENTER (H5FD_dpss_init, FAIL);

    /* register the DPSS driver if it wasn't already */
    if (! H5FD_DPSS_g)
        H5FD_DPSS_g = H5FDregister (&H5FD_dpss_g);

    /* active the DPSS storage client module which in turn activates
       all other Globus modules that are needed */
    if (H5FD_DPSS_g >= 0) {
        globus_result = globus_module_activate (GRID_STORAGE_FILE_MODULE);
        if (GLOBUS_SUCCESS != globus_result) {
            H5FDunregister (H5FD_DPSS_g);
            H5FD_DPSS_g = -1;
            HRETURN_ERROR (H5E_IO, H5E_CANTINIT, FAIL,
                           "Cannot activate Globus Grid Storage File module");
        }
    }

    FUNC_LEAVE (H5FD_DPSS_g);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_dpss
 *
 * Purpose:     Modify the file application list to use the DPSS I/O
 *              driver defined in this source file.
 *
 * Return:      Success:    Non-negative
 *
 *              Failure:    Negative
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_dpss(hid_t fapl_id)
{
    herr_t ret_value=FAIL;

    FUNC_ENTER (H5Pset_fapl_dpss, FAIL);
    H5TRACE1("e","i",fapl_id);

    /* Check arguments */
    if (H5P_FILE_ACCESS != H5Pget_class (fapl_id))
        HRETURN_ERROR (H5E_PLIST, H5E_BADTYPE, FAIL, "not a fapl");

    ret_value = H5Pset_driver (fapl_id, H5FD_DPSS, NULL);

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_open
 *
 * Purpose:     Creates/opens a DPSS file instance identified by its URL
 *              as an HDF5 file.
 *
 * Return:      Success:    A pointer to a new file data structure. The
 *                          public fields will be initialized by the
 *                          caller, which is always H5FD_open().
 *
 *              Failure:    NULL
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_dpss_open (const char *name, unsigned flags, hid_t UNUSED fapl_id,
                haddr_t maxaddr)
{
    int                      i;
    globus_result_t          globus_result;
    grid_storage_url_t       *url = (grid_storage_url_t *) name;
    grid_storage_handle_t    handle;
    H5FD_dpss_t              *file;
    unsigned int             o_flags;
    char                     int_attribute_value [10];
    int64_t                  file_size;
    char                     **property_names,
                             **property_values;
    grid_storage_attribute_t attributes = GRID_STORAGE_ATTRIBUTE_INITIALIZER;


    FUNC_ENTER (H5FD_dpss_open, NULL);

#ifdef DEBUG
    fprintf (stdout, "H5FD_dpss_open: name='%s', flags=0x%x\n", name, flags);
#endif

    /* Check arguments */
    if (!name || !*name)
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name");
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HRETURN_ERROR (H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");
    if (ADDR_OVERFLOW (maxaddr))
        HRETURN_ERROR (H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");

    /* Build the open flags */
    o_flags = (H5F_ACC_RDWR & flags) ? O_RDWR : O_RDONLY;
    if (H5F_ACC_TRUNC & flags) o_flags |= O_TRUNC;
    if (H5F_ACC_CREAT & flags) o_flags |= O_CREAT;
    if (H5F_ACC_EXCL  & flags) o_flags |= O_EXCL;
    sprintf (int_attribute_value, "%d", o_flags);

    /* set the 'open flags' attribute */
    globus_result =
        grid_storage_add_attribute (GRID_STORAGE_ATTRIBUTE_OPEN_FLAGS,
                                    int_attribute_value, &attributes);
    if (GLOBUS_SUCCESS != globus_result) {
        PRINT_GLOBUS_ERROR_MSG (globus_result);
        HRETURN_ERROR (H5E_IO, H5E_CANTOPENFILE, NULL,
                       "Can't set attributes for file instance");
    }

    /* open the file instance */
    globus_result = grid_storage_open ((grid_storage_url_t *) url, &handle,
                                       &attributes);
    if (GLOBUS_SUCCESS != globus_result) {
        PRINT_GLOBUS_ERROR_MSG (globus_result);
        HRETURN_ERROR (H5E_IO, H5E_CANTOPENFILE, NULL,
                       "Can't open file instance");
    }

    /* Get the size of the file instance.
       Read all the instance properties ... */
    globus_result = grid_storage_list_properties (url, NULL, &property_names,
                                                  &property_values);
    if (GLOBUS_SUCCESS != globus_result) {
        PRINT_GLOBUS_ERROR_MSG (globus_result);
        grid_storage_close (&handle, NULL);
        HRETURN_ERROR (H5E_IO, H5E_CANTOPENFILE, NULL,
                       "Can't get properties of file instance");
    }
    /* ... now sort out the 'size' property */
    file_size = -1;
    for (i = 0; property_names [i]; i++) {
        if (strcmp (property_names [i], GRID_STORAGE_PROPERTY_SIZE) == 0) {
            if (property_values [i])
                sscanf (property_values [i], "%"PRINTF_LL_WIDTH"u", &file_size);
        }
        /* don't forget to free the allocated property strings */
        globus_libc_free (property_names [i]);
        globus_libc_free (property_values [i]);
    }
    globus_libc_free (property_names);
    globus_libc_free (property_values);

    /* Did we find a size ? */
    if (file_size < 0) {
        grid_storage_close (&handle, NULL);
        HRETURN_ERROR (H5E_IO, H5E_CANTOPENFILE, NULL,
                       "Can't get size of file instance");
    }

    /* Create the new file struct */
    if (NULL == (file = (H5FD_dpss_t *) H5MM_calloc (sizeof (H5FD_dpss_t))))
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "Can't allocate file struct");
    file->handle = handle;
    file->eof = (haddr_t) file_size;

    FUNC_LEAVE ((H5FD_t *) file);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_close
*
 * Purpose:     Closes a previously opened DPSS file instance.
 *
 * Return:      Success:    0
 *
 *              Failure:    -1, file not closed.
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dpss_close (H5FD_t *_file)
{
    globus_result_t globus_result;
    H5FD_dpss_t *file = (H5FD_dpss_t *) _file;

    FUNC_ENTER (H5FD_dpss_close, FAIL);

    if (NULL == file)
      HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file handle");
    
    globus_result = grid_storage_close (&file->handle, NULL);
    if (GLOBUS_SUCCESS != globus_result) {
        PRINT_GLOBUS_ERROR_MSG (globus_result);
        HRETURN_ERROR (H5E_IO, H5E_CANTCLOSEFILE, FAIL, "Failed to close file instance");
    }

    H5MM_xfree (file);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_query
 *
 * Purpose:     Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, September 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dpss_query(const UNUSED H5FD_t *_f, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER(H5FD_dpss_query, FAIL);

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags|=H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_get_eoa
 *
 * Purpose:     Gets the end-of-address marker for the file. The EOA marker
 *              is the first address past the last byte allocated in the
 *              format address space.
 *
 * Return:      Success:    The end-of-address marker.
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_dpss_get_eoa (H5FD_t *_file)
{
    H5FD_dpss_t *file = (H5FD_dpss_t *) _file;

    FUNC_ENTER (H5FD_dpss_get_eoa, HADDR_UNDEF);

    FUNC_LEAVE (file->eoa);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_set_eoa
 *
 * Purpose:     Set the end-of-address marker for the file. This function is
 *              called shortly after an existing HDF5 file is opened in order
 *              to tell the driver where the end of the HDF5 data is located.
 *
 * Return:      Success:        0
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dpss_set_eoa (H5FD_t *_file, haddr_t addr)
{
    H5FD_dpss_t *file = (H5FD_dpss_t *) _file;

    FUNC_ENTER (H5FD_dpss_set_eoa, FAIL);

    file->eoa = addr;

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_get_eof
 *
 * Purpose:     Returns the end-of-file marker, which is the greater of
 *              either the Unix end-of-file or the HDF5 end-of-address
 *              markers.
 *
 * Return:      Success:        End of file address, the first address past
 *                              the end of the "file", either the DPSS
 *                              file or the HDF5 file.
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_dpss_get_eof (H5FD_t *_file)
{
    H5FD_dpss_t *file = (H5FD_dpss_t*) _file;

    FUNC_ENTER (H5FD_dpss_get_eof, HADDR_UNDEF);

    FUNC_LEAVE (file->eof > file->eoa ? file->eof : file->eoa);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_read
 *
 * Purpose:     Reads SIZE bytes of data from FILE beginning at address ADDR
 *              into buffer BUF.
 *
 * Return:      Success:        Zero. Result is stored in caller-supplied
 *                              buffer BUF.
 *
 *              Failure:        -1, Contents of buffer BUF are undefined.
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dpss_read (H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t dxpl_id, haddr_t addr,
                hsize_t size, void *buf/*out*/)
{
    H5FD_dpss_t *file = (H5FD_dpss_t *) _file;
    globus_result_t  globus_result;
#ifdef COALESCE_READS
    static int count = 0;                 /* counter for single reads */
    H5F_xfer_t *xfer_parms;               /*transfer property list*/
#endif

    FUNC_ENTER (H5FD_dpss_read, FAIL);

#ifdef DEBUG
    fprintf (stdout, "H5FD_dpss_read: addr 0x%lx, size %ld\n",
             (unsigned long int) addr, (unsigned long int) size);
#endif

    /* Check parameters */
    if (! file)
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file handle");
    if (! buf)
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer pointer");

    /* Check for overflow conditions */
    if (HADDR_UNDEF == addr)
        HRETURN_ERROR (H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW (addr, size))
        HRETURN_ERROR (H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr + size > file->eoa)
        HRETURN_ERROR (H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

#ifdef COALESCE_READS
    /* Get the dataset transfer property list */
    if (H5P_DEFAULT == dxpl_id) {
        xfer_parms = &H5F_xfer_dflt;
    } else if (H5P_DATASET_XFER != H5P_get_class (dxpl_id) ||
               NULL == (xfer_parms = H5I_object (dxpl_id))) {
        HRETURN_ERROR (H5E_PLIST, H5E_BADTYPE, FAIL, "not a xfer");
    }

    if (xfer_parms->gather_reads) {
        if (! count)
            count = xfer_parms->gather_reads;
#ifdef DEBUG
        fprintf (stdout, "H5FD_dpss_read: request would be queued\n");
#endif
    } else {
#ifdef DEBUG
        fprintf (stdout, "H5FD_dpss_read: transaction would be performed "
                         "with %d individual reads\n", count + 1);
#endif
        count = 0;
    }
#endif

    /* do the (synchronuous) write operation */
    globus_result = grid_storage_read (&file->handle, (unsigned char *) buf,
                                       (size_t) addr, (size_t) size, NULL);
    if (GLOBUS_SUCCESS != globus_result) {
        PRINT_GLOBUS_ERROR_MSG (globus_result);
        HRETURN_ERROR (H5E_IO, H5E_READERROR, FAIL,
                       "Failed to read file instance");
    }

    FUNC_LEAVE (SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_dpss_write
 *
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address ADDR
 *              from buffer BUF.
 *
 * Return:      Success:        Zero
 *
 *              Failure:        -1
 *
 * Programmer:  Thomas Radke
 *              Monday, October 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dpss_write (H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, haddr_t addr,
                 hsize_t size, const void *buf)
{
    H5FD_dpss_t  *file = (H5FD_dpss_t *) _file;
    globus_result_t  globus_result;

    FUNC_ENTER (H5FD_dpss_write, FAIL);

#ifdef DEBUG
    fprintf (stdout, "H5FD_dpss_write: addr 0x%lx, size %ld\n",
             (unsigned long int) addr, (unsigned long int) size);
#endif

    /* Check parameters */
    if (! file)
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file handle");
    if (! buf)
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer pointer");

    /* Check for overflow conditions */
    if (HADDR_UNDEF == addr)
        HRETURN_ERROR (H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW (addr, size))
        HRETURN_ERROR (H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr + size > file->eoa)
        HRETURN_ERROR (H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    /* do the (synchronuous) write operation */
    globus_result = grid_storage_write (&file->handle, (unsigned char *) buf,
                                        (size_t) addr, (size_t) size, NULL);
    if (GLOBUS_SUCCESS != globus_result) {
        PRINT_GLOBUS_ERROR_MSG (globus_result);
        HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "Failed to write file instance");
    }

    /* expand end-of-file marker if necessary */
    if (addr > file->eof)
        file->eof = addr;

    FUNC_LEAVE (SUCCEED);
}

#endif /* DPSS */
  
