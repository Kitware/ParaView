/****************************************************************************
* NCSA HDF                                                                 *
* Software Development Group                                               *
* National Center for Supercomputing Applications                          *
* University of Illinois at Urbana-Champaign                               *
* 605 E. Springfield, Champaign IL 61820                                   *
*                                                                          *
* For conditions of distribution and use, see the accompanying             *
* hdf/COPYING file.                                                        *
*
* MODIFICATIONS
*       Robb Matzke, 30 Aug 1997
*       Added `ERRORS' fields to function prologues.
*
****************************************************************************/

/* Id */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

/* Predefined file drivers */
#include "H5FDcore.h"           /*temporary in-memory files               */
#include "H5FDfamily.h"         /*family of files                         */
#include "H5FDmpio.h"           /*MPI-2 I/O                               */
#include "H5FDgass.h"           /*GASS I/O                                */
#include "H5FDstream.h"         /*in-memory files streamed via sockets    */
#include "H5FDsrb.h"            /*SRB I/O                                 */
#include "H5FDmulti.h"          /*multiple files partitioned by mem usage */
#include "H5FDsec2.h"           /*Posix unbuffered I/O                    */
#include "H5FDstdio.h"          /*Standard C buffered I/O                 */
#include "H5FDlog.h"            /*sec2 driver with logging, for debugging */

/* Packages needed by this file... */
#include "H5private.h"          /*library functions                       */
#include "H5Aprivate.h"         /*attributes                              */
#include "H5Dprivate.h"         /*datasets                                */
#include "H5FLprivate.h"        /*Free Lists                              */
#include "H5Iprivate.h"         /*object IDs                              */
#include "H5ACprivate.h"        /*cache                                   */
#include "H5Eprivate.h"         /*error handling                          */
#include "H5Fpkg.h"             /*file access                             */
#include "H5FDprivate.h"        /*file driver                             */
#include "H5Gprivate.h"         /*symbol tables                           */
#include "H5MMprivate.h"        /*core memory management                  */
#include "H5Pprivate.h"         /*property lists                          */
#include "H5Tprivate.h"         /*data types                              */

#define PABLO_MASK      H5F_mask

/*
 * Define the default file creation property list.
 */
const H5F_create_t      H5F_create_dflt = {
    0,                          /* Default user-block size */
    4,                          /* Default 1/2 rank for symtab leaf nodes */
    {                           /* Default 1/2 rank for btree intern nodes */
        16,                     /* Symbol table internal nodes             */
        32,                     /* Indexed storage internal nodes          */
        0,                      /* unused                                  */
        0,                      /* unused                                  */
        0,                      /* unused                                  */
        0,                      /* unused                                  */
        0,                      /* unused                                  */
        0,                      /* unused                                  */
    },
    sizeof(haddr_t),            /* Default offset size                     */
    sizeof(hsize_t),            /* Default length size                     */
    HDF5_BOOTBLOCK_VERSION,     /* Current Boot-Block version #            */
    HDF5_FREESPACE_VERSION,     /* Current Free-Space info version #       */
    HDF5_OBJECTDIR_VERSION,     /* Current Object Directory info version # */
    HDF5_SHAREDHEADER_VERSION,  /* Current Shared-Header format version #  */
};

/*
 * Define the default file access property list.  The template is initialized
 * by H5F_init_interface().
 */
H5F_access_t H5F_access_dflt;

/*
 * Define the default mount property list.
 */
const H5F_mprop_t       H5F_mount_dflt = {
    FALSE,                      /* Absolute symlinks are wrt mount root    */
};

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5F_init_interface
static herr_t H5F_init_interface(void);

/* PRIVATE PROTOTYPES */
static H5F_t *H5F_new(H5F_file_t *shared, hid_t fcpl_id, hid_t fapl_id);
static herr_t H5F_dest(H5F_t *f);
static herr_t H5F_flush(H5F_t *f, H5F_scope_t scope, hbool_t invalidate,
                        hbool_t alloc_only);
static haddr_t H5F_locate_signature(H5FD_t *file);
static int H5F_flush_all_cb(H5F_t *f, const void *_invalidate);

/* Declare a free list to manage the H5F_t struct */
H5FL_DEFINE_STATIC(H5F_t);

/* Declare a free list to manage the H5F_file_t struct */
H5FL_DEFINE_STATIC(H5F_file_t);

/* Declare the external free list for the H5G_t struct */
H5FL_EXTERN(H5G_t);


/*-------------------------------------------------------------------------
 * Function:    H5F_init
 *
 * Purpose:     Initialize the interface from some other layer.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_init(void)
{
    FUNC_ENTER(H5F_init, FAIL);
    /* FUNC_ENTER() does all the work */
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_init_interface
 *
 * Purpose:     Initialize interface-specific information.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Friday, November 20, 1998
 *
 * Modifications:
 *      Robb Matzke, 4 Aug 1997
 *      Changed pablo mask from H5_mask to H5F_mask for the FUNC_LEAVE call.
 *      It was already H5F_mask for the PABLO_TRACE_ON call.
 *
 *      Kim Yates, 1998-08-16
 *      Added .disp, .btype, .ftype to H5F_access_t.
 *
 *      Robb Matzke, 1999-02-19
 *      Added initialization for the H5I_FILE_CLOSING ID group.
 *
 *      Raymond Lu, April 10, 2000
 *      Put SRB into the 'Register predefined file drivers' list.
 *
 *      Thomas Radke, 2000-09-12
 *      Put Stream VFD into the 'Register predefined file drivers' list.
 *-------------------------------------------------------------------------
 */
static herr_t 
H5F_init_interface(void)
{
    herr_t      ret_value = SUCCEED;
    herr_t      status;
    
    FUNC_ENTER(H5F_init_interface, FAIL);

#ifdef H5_HAVE_PARALLEL
    {
        /* Allow MPI buf-and-file-type optimizations? */
        const char *s = HDgetenv ("HDF5_MPI_1_METAWRITE");
        if (s && HDisdigit(*s)) {
            H5_mpi_1_metawrite_g = (int)HDstrtol (s, NULL, 0);
        }
    }
#endif

    /*
     * Initialize the atom group for the file IDs. There are two groups:
     * the H5I_FILE group contains all the ID's for files which are currently
     * open at the public API level while the H5I_FILE_CLOSING group contains
     * ID's for files for which the application has called H5Fclose() but
     * which are pending completion because there are object headers still
     * open within the file.
     */
    if (H5I_init_group(H5I_FILE, H5I_FILEID_HASHSIZE, 0, (H5I_free_t)H5F_close)<0 ||
        H5I_init_group(H5I_FILE_CLOSING, H5I_FILEID_HASHSIZE, 0, (H5I_free_t)H5F_close)<0) {
            HRETURN_ERROR (H5E_FILE, H5E_CANTINIT, FAIL,
                       "unable to initialize interface");
    }

/* Register the default file creation & access properties */

    /* Register predefined file drivers */
    H5E_BEGIN_TRY {
        if ((status=H5FD_SEC2)<0) goto end_registration;
        if ((status=H5FD_STDIO)<0) goto end_registration;
        if ((status=H5FD_FAMILY)<0) goto end_registration;
#ifdef H5_HAVE_GASS
        if ((status=H5FD_GASS)<0) goto end_registration;
#endif
#ifdef H5_HAVE_SRB
        if ((status=H5FD_SRB)<0) goto end_registration;
#endif
        if ((status=H5FD_CORE)<0) goto end_registration;
        if ((status=H5FD_MULTI)<0) goto end_registration;
#ifdef H5_HAVE_PARALLEL
        if ((status=H5FD_MPIO)<0) goto end_registration;
#endif
#ifdef H5_HAVE_STREAM
        if ((status=H5FD_STREAM)<0) goto end_registration;
#endif
    end_registration: ;
        } H5E_END_TRY;

    if (status<0) {
        HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                      "file driver registration failed");
    }
    
    /* Initialize the default file access property list */
    H5F_access_dflt.mdc_nelmts = H5AC_NSLOTS;
    H5F_access_dflt.rdcc_nelmts = 521;
    H5F_access_dflt.rdcc_nbytes = 1024*1024; /*1MB*/
    H5F_access_dflt.rdcc_w0 = 0.75; /*preempt fully read chunks*/
    H5F_access_dflt.threshold = 1; /*alignment applies to everything*/
    H5F_access_dflt.alignment = 1; /*no alignment*/
    H5F_access_dflt.gc_ref = 0; /*don't garbage-collect references*/
    H5F_access_dflt.meta_block_size = 2048; /* set metadata block allocations to 2KB */
    H5F_access_dflt.sieve_buf_size = 64*1024; /* set sieve buffer allocation to 64KB */
    H5F_access_dflt.driver_id = H5FD_SEC2; /*default driver*/
    H5F_access_dflt.driver_info = NULL; /*driver file access properties*/

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_term_interface
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
H5F_term_interface(void)
{
    int n = 0;

    if (interface_initialize_g) {
        if ((n=H5I_nmembers(H5I_FILE))) {
            H5F_close_all();
        } else if (0==(n=H5I_nmembers(H5I_FILE_CLOSING))) {
            H5I_destroy_group(H5I_FILE);
            H5I_destroy_group(H5I_FILE_CLOSING);
            interface_initialize_g = 0;
            n = 1; /*H5I*/
        }
    }
    return n;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_flush_all_cb
 *
 * Purpose:     Callback function for H5F_flush_all().
 *
 * Return:      Always returns zero.
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_flush_all_cb(H5F_t *f, const void *_invalidate)
{
    hbool_t     invalidate = *((const hbool_t*)_invalidate);
    H5F_flush(f, H5F_SCOPE_LOCAL, invalidate, FALSE);
    return 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_flush_all
 *
 * Purpose:     Flush all open files. If INVALIDATE is true then also remove
 *              everything from the cache.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 18, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_flush_all(hbool_t invalidate)
{
    FUNC_ENTER(H5F_flush_all, FAIL);
    H5I_search(H5I_FILE, (H5I_search_func_t)H5F_flush_all_cb,
               (void*)&invalidate);
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_close_all
 *
 * Purpose:     Close all open files. Any file which has open object headers
 *              will be moved from the H5I_FILE group to the H5I_FILE_CLOSING
 *              group.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_close_all(void)
{
    FUNC_ENTER(H5F_close_all, FAIL);
    if (H5I_clear_group(H5I_FILE, FALSE)<0) {
        HRETURN_ERROR(H5E_FILE, H5E_CLOSEERROR, FAIL,
                      "unable to close one or more files");
    }
    FUNC_LEAVE(SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
       H5F_encode_length_unusual -- encode an unusual length size
 USAGE
       void H5F_encode_length_unusual(f, p, l)
       const H5F_t *f;             IN: pointer to the file record
       uint8_t **p;             IN: pointer to buffer pointer to encode length in
       uint8_t *l;              IN: pointer to length to encode

 ERRORS

 RETURNS
    none
 DESCRIPTION
    Encode non-standard (i.e. not 2, 4 or 8-byte) lengths in file meta-data.
--------------------------------------------------------------------------*/
void 
H5F_encode_length_unusual(const H5F_t *f, uint8_t **p, uint8_t *l)
{
    int             i = (int)H5F_SIZEOF_SIZE(f)-1;

#ifdef WORDS_BIGENDIAN
    /*
     * For non-little-endian platforms, encode each byte in memory backwards.
     */
    for (/*void*/; i>=0; i--, (*p)++)*(*p) = *(l+i);
#else
    /* platform has little-endian integers */
    HDmemcpy(*p,l,(size_t)(i+1));
    *p+=(i+1);
#endif

}


/*-------------------------------------------------------------------------
 * Function:    H5Fget_create_plist
 *
 * Purpose:     Get an atom for a copy of the file-creation property list for
 *              this file. This function returns an atom with a copy of the
 *              properties used to create a file.
 *
 * Return:      Success:        template ID
 *
 *              Failure:        FAIL
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *
 *      Robb Matzke, 18 Feb 1998
 *      Calls H5P_copy() to copy the property list and H5P_close() to free
 *      that property list if an error occurs.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fget_create_plist(hid_t file_id)
{
    H5F_t               *file = NULL;
    hid_t               ret_value = FAIL;
    H5P_t           *plist = NULL;

    FUNC_ENTER(H5Fget_create_plist, FAIL);
    H5TRACE1("i","i",file_id);

    /* check args */
    if (H5I_FILE!=H5I_get_type(file_id) || NULL==(file=H5I_object(file_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file");
    }
    
    /* Create the property list object to return */
    if (NULL==(plist=H5P_copy(H5P_FILE_CREATE, file->shared->fcpl))) {
        HRETURN_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL,
                      "unable to copy file creation properties");
    }

    /* Create an atom */
    if ((ret_value = H5P_create(H5P_FILE_CREATE, plist)) < 0) {
        H5P_close(plist);
        HRETURN_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                      "unable to register property list");
    }
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fget_access_plist
 *
 * Purpose:     Returns a copy of the file access property list of the
 *              specified file.
 *
 * Return:      Success:        Object ID for a copy of the file access
 *                              property list.
 *
 *              Failure:        FAIL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, February 18, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fget_access_plist(hid_t file_id)
{
    H5F_t               *f = NULL;
    H5F_access_t        _fapl;
    H5P_t           *plist=NULL;
    hid_t               ret_value = FAIL;
    
    FUNC_ENTER(H5Fget_access_plist, FAIL);
    H5TRACE1("i","i",file_id);

    /* Check args */
    if (H5I_FILE!=H5I_get_type(file_id) || NULL==(f=H5I_object(file_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file");
    }

    /* Initialize the property list */
    HDmemset(&_fapl, 0, sizeof _fapl);
    _fapl.mdc_nelmts = f->shared->mdc_nelmts;
    _fapl.rdcc_nelmts = f->shared->rdcc_nelmts;
    _fapl.rdcc_nbytes = f->shared->rdcc_nbytes;
    _fapl.rdcc_w0 = f->shared->rdcc_w0;
    _fapl.threshold = f->shared->threshold;
    _fapl.alignment = f->shared->alignment;
    _fapl.gc_ref = f->shared->gc_ref;
    _fapl.meta_block_size = f->shared->lf->def_meta_block_size;
    _fapl.sieve_buf_size = f->shared->sieve_buf_size;
    _fapl.driver_id = f->shared->lf->driver_id;
    _fapl.driver_info = NULL; /*just for now */

    /* Copy properties */
    if (NULL==(plist=H5P_copy(H5P_FILE_ACCESS, &_fapl))) {
        HRETURN_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL,
                      "unable to copy file access properties");
    }

    /* Get the properties for the file driver */
    plist->u.faccess.driver_info = H5FD_fapl_get(f->shared->lf);

    /* Create an atom */
    if ((ret_value = H5P_create(H5P_FILE_ACCESS, plist))<0) {
        H5P_close(plist);
        HRETURN_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                      "unable to register property list");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_equal
 *
 * Purpose:     Compares NEEDLE to a file from the HAYSTACK.
 *
 * Return:      Success:        Returns positive if two files are equal,
 *                              zero otherwise.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, August  2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_equal(void *_haystack, const void *_needle)
{
    H5F_t               *haystack = (H5F_t*)_haystack;
    const H5FD_t        *needle = (const H5FD_t*)_needle;
    int         retval;
    
    FUNC_ENTER(H5F_equal, FAIL);
    retval = (0==H5FD_cmp(haystack->shared->lf, needle));
    FUNC_LEAVE(retval);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_locate_signature
 *
 * Purpose:     Finds the HDF5 boot block signature in a file.  The signature
 *              can appear at address 0, or any power of two beginning with
 *              512.
 *
 * Return:      Success:        The absolute format address of the signature.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Friday, November  7, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              Rewritten to use the virtual file layer.
 *-------------------------------------------------------------------------
 */
static haddr_t
H5F_locate_signature(H5FD_t *file)
{
    haddr_t         addr, eoa;
    uint8_t         buf[H5F_SIGNATURE_LEN];
    unsigned        n, maxpow;

    FUNC_ENTER(H5F_locate_signature, HADDR_UNDEF);

    /* Find the least N such that 2^N is larger than the file size */
    if (HADDR_UNDEF==(addr=H5FD_get_eof(file)) ||
        HADDR_UNDEF==(eoa=H5FD_get_eoa(file))) {
        HRETURN_ERROR(H5E_IO, H5E_CANTINIT, HADDR_UNDEF,
                      "unable to obtain EOF/EOA value");
    }
    for (maxpow=0; addr; maxpow++) addr>>=1;
    maxpow = MAX(maxpow, 9);

    /*
     * Search for the file signature at format address zero followed by
     * powers of two larger than 9.
     */
    for (n=8; n<maxpow; n++) {
        addr = (8==n) ? 0 : (haddr_t)1 << n;
        if (H5FD_set_eoa(file, addr+H5F_SIGNATURE_LEN)<0) {
            HRETURN_ERROR(H5E_IO, H5E_CANTINIT, HADDR_UNDEF,
                          "unable to set EOA value for file signature");
        }
        if (H5FD_read(file, H5FD_MEM_SUPER, H5P_DEFAULT, addr, (hsize_t)H5F_SIGNATURE_LEN, buf)<0) {
            HRETURN_ERROR(H5E_IO, H5E_CANTINIT, HADDR_UNDEF,
                          "unable to read file signature");
        }
        if (!HDmemcmp(buf, H5F_SIGNATURE, H5F_SIGNATURE_LEN)) break;
    }

    /*
     * If the signature was not found then reset the EOA value and return
     * failure.
     */
    if (n>=maxpow) {
        H5FD_set_eoa(file, eoa);
        HRETURN_ERROR(H5E_IO, H5E_CANTINIT, HADDR_UNDEF,
                      "unable to find a valid file signature");
    }

    /* Success */
    FUNC_LEAVE(addr);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fis_hdf5
 *
 * Purpose:     Check the file signature to detect an HDF5 file.
 *
 * Bugs:        This function is not robust: it only uses the default file
 *              driver when attempting to open the file when in fact it
 *              should use all known file drivers.
 *
 * Return:      Success:        TRUE/FALSE
 *
 *              Failure:        Negative
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              Rewritten to use the virtual file layer.
 *-------------------------------------------------------------------------
 */
htri_t
H5Fis_hdf5(const char *name)
{
    H5FD_t      *file = NULL;
    htri_t      ret_value = FAIL;

    FUNC_ENTER(H5Fis_hdf5, FAIL);
    H5TRACE1("b","s",name);

    /* Check args and all the boring stuff. */
    if (!name || !*name) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "no file name specified");
    }

    /* Open the file at the virtual file layer */
    if (NULL==(file=H5FD_open(name, H5F_ACC_RDONLY, H5P_DEFAULT,
                              HADDR_UNDEF))) {
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to open file");
    }

    /* The file is an hdf5 file if the hdf5 file signature can be found */
    ret_value = (HADDR_UNDEF!=H5F_locate_signature(file));

 done:
    /* Close the file */
    if (file) H5FD_close(file);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_new
 *
 * Purpose:     Creates a new file object and initializes it.  The
 *              H5Fopen and H5Fcreate functions then fill in various
 *              fields.  If SHARED is a non-null pointer then the shared info
 *              to which it points has the reference count incremented.
 *              Otherwise a new, empty shared info struct is created and
 *              initialized with the specified file access property list.
 *
 * Errors:
 *
 * Return:      Success:        Ptr to a new file struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5F_t *
H5F_new(H5F_file_t *shared, hid_t fcpl_id, hid_t fapl_id)
{
    H5F_t               *f=NULL, *ret_value=NULL;
    int         n;
    const H5F_create_t  *fcpl=NULL;
    const H5F_access_t  *fapl=NULL;
    
    FUNC_ENTER(H5F_new, NULL);

    if (NULL==(f=H5FL_ALLOC(H5F_t,1))) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                    "memory allocation failed");
    }

    if (shared) {
        f->shared = shared;
    } else {
        f->shared = H5FL_ALLOC(H5F_file_t,1);
        f->shared->boot_addr = HADDR_UNDEF;
        f->shared->base_addr = HADDR_UNDEF;
        f->shared->freespace_addr = HADDR_UNDEF;
        f->shared->driver_addr = HADDR_UNDEF;
    
        /*
         * Copy the file creation and file access property lists into the
         * new file handle.  We do this early because some values might need
         * to change as the file is being opened.
         */
        fcpl = (H5P_DEFAULT==fcpl_id)? &H5F_create_dflt : (const H5F_create_t *)H5I_object(fcpl_id);
        if (NULL==(f->shared->fcpl=H5P_copy(H5P_FILE_CREATE, fcpl))) {
            HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, NULL,
                          "unable to copy file creation property list");
        }

        fapl = (H5P_DEFAULT==fapl_id)? &H5F_access_dflt : (const H5F_access_t *)H5I_object(fapl_id);
        f->shared->mdc_nelmts = fapl->mdc_nelmts;
        f->shared->rdcc_nelmts = fapl->rdcc_nelmts;
        f->shared->rdcc_nbytes = fapl->rdcc_nbytes;
        f->shared->rdcc_w0 = fapl->rdcc_w0;
        f->shared->threshold = fapl->threshold;
        f->shared->alignment = fapl->alignment;
        f->shared->gc_ref = fapl->gc_ref;
        f->shared->sieve_buf_size = fapl->sieve_buf_size;

#ifdef H5_HAVE_PARALLEL
        /*
         * Disable cache if file is open using MPIO driver.  Parallel
         * does not permit caching.  (maybe able to relax it for
         * read only open.)
         */
        if (H5FD_MPIO==fapl->driver_id){
            f->shared->rdcc_nbytes = 0;
            f->shared->mdc_nelmts = 0;
        }
#endif

        /*
         * Create a meta data cache with the specified number of elements.
         * The cache might be created with a different number of elements and
         * the access property list should be updated to reflect that.
         */
        if ((n=H5AC_create(f, f->shared->mdc_nelmts))<0) {
            HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, NULL,
                          "unable to create meta data cache");
        }
        f->shared->mdc_nelmts = n;
        
        /* Create the chunk cache */
        H5F_istore_init(f);
    }
    
    f->shared->nrefs++;
    f->nrefs = 1;
    ret_value = f;

 done:
    if (!ret_value && f) {
        if (!shared) H5FL_FREE(H5F_file_t,f->shared);
        H5FL_FREE(H5F_t,f);
    }
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_dest
 *
 * Purpose:     Destroys a file structure.  This function flushes the cache
 *              but doesn't do any other cleanup other than freeing memory
 *              for the file struct.  The shared info for the file is freed
 *              only when its reference count reaches zero.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 1998-10-14
 *      Nothing happens unless the reference count for the H5F_t goes to
 *      zero.  The reference counts are decremented here.
 *
 *      Robb Matzke, 1999-02-19
 *      More careful about decrementing reference counts so they don't go
 *      negative or wrap around to some huge value.  Nothing happens if a
 *      reference count is already zero.
 *
 *      Robb Matzke, 2000-10-31
 *      H5FL_FREE() aborts if called with a null pointer (unlike the
 *      original H5MM_free()).
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_dest(H5F_t *f)
{
    herr_t      ret_value = SUCCEED;
    
    FUNC_ENTER(H5F_dest, FAIL);

    if (f && 1==f->nrefs) {
        if (1==f->shared->nrefs) {
            /*
             * Do not close the root group since we didn't count it, but free
             * the memory associated with it.
             */
            if (f->shared->root_grp) {
                H5FL_FREE(H5G_t,f->shared->root_grp);
                f->shared->root_grp=NULL;
            }
            if (H5AC_dest(f)) {
                HERROR(H5E_FILE, H5E_CANTINIT, "problems closing file");
                ret_value = FAIL; /*but keep going*/
            }
            if (H5F_istore_dest (f)<0) {
                HERROR(H5E_FILE, H5E_CANTINIT, "problems closing file");
                ret_value = FAIL; /*but keep going*/
            }
            f->shared->cwfs = H5MM_xfree (f->shared->cwfs);

        /* Free the data sieve buffer, if it's been allocated */
        if(f->shared->sieve_buf) {
            assert(f->shared->sieve_dirty==0);    /* The buffer had better be flushed... */
            f->shared->sieve_buf = H5MM_xfree (f->shared->sieve_buf);
        } /* end if */

            /* Destroy file creation properties */
            H5P_close(f->shared->fcpl);

            /* Destroy shared file struct */
            if (H5FD_close(f->shared->lf)<0) {
                HERROR(H5E_FILE, H5E_CANTINIT, "problems closing file");
                ret_value = FAIL; /*but keep going*/
            }
            f->shared = H5FL_FREE(H5F_file_t,f->shared);
            
        } else if (f->shared->nrefs>0) {
            /*
             * There are other references to the shared part of the file.
             * Only decrement the reference count.
             */
            --f->shared->nrefs;
        }

        /* Free the non-shared part of the file */
        f->name = H5MM_xfree(f->name);
        f->mtab.child = H5MM_xfree(f->mtab.child);
        f->mtab.nalloc = 0;
        H5FL_FREE(H5F_t,f);
    } else if (f && f->nrefs>0) {
        /*
         * There are other references to this file. Only decrement the
         * reference count.
         */
        --f->nrefs;
    }
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_open
 *
 * Purpose:     Opens (or creates) a file.  This function understands the
 *              following flags which are similar in nature to the Posix
 *              open(2) flags.
 *
 *              H5F_ACC_RDWR:   Open with read/write access. If the file is
 *                              currently open for read-only access then it
 *                              will be reopened. Absence of this flag
 *                              implies read-only access.
 *
 *              H5F_ACC_CREAT:  Create a new file if it doesn't exist yet.
 *                              The permissions are 0666 bit-wise AND with
 *                              the current umask.  H5F_ACC_WRITE must also
 *                              be specified.
 *
 *              H5F_ACC_EXCL:   This flag causes H5F_open() to fail if the
 *                              file already exists.
 *
 *              H5F_ACC_TRUNC:  The file is truncated and a new HDF5 superblock
 *                              is written.  This operation will fail if the
 *                              file is already open.
 *
 *              Unlinking the file name from the group directed graph while
 *              the file is opened causes the file to continue to exist but
 *              one will not be able to upgrade the file from read-only
 *              access to read-write access by reopening it. Disk resources
 *              for the file are released when all handles to the file are
 *              closed. NOTE: This paragraph probably only applies to Unix;
 *              deleting the file name in other OS's has undefined results.
 *
 *              The CREATE_PARMS argument is optional.  A null pointer will
 *              cause the default file creation parameters to be used.
 *
 *              The ACCESS_PARMS argument is optional.  A null pointer will
 *              cause the default file access parameters to be used.
 *
 * Return:      Success:        A new file pointer.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, September 23, 1997
 *
 * Modifications:
 *              Albert Cheng, 1998-02-05
 *              Added the access_parms argument to pass down access template
 *              information.
 *
 *              Robb Matzke, 1998-02-18
 *              The H5F_access_t changed to allow more generality.  The low
 *              level driver is part of the file access template so the TYPE
 *              argument has been removed.
 *
 *              Robb Matzke, 1999-08-02
 *              Rewritten to use the virtual file layer.
 *
 *              Robb Matzke, 1999-08-16
 *              Added decoding of file driver information block, which uses a
 *              formerly reserved address slot in the boot block in order to
 *              be compatible with previous versions of the file format.
 *
 *              Robb Matzke, 1999-08-20
 *              Optimizations for opening a file. If the driver can't
 *              determine when two file handles refer to the same file then
 *              we open the file in one step.  Otherwise if the first attempt
 *              to open the file fails then we skip the second attempt if the
 *              arguments would be the same.
 *-------------------------------------------------------------------------
 */
H5F_t *
H5F_open(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
{
    H5F_t               *file=NULL;     /*the success return value      */
    H5F_t               *ret_value=NULL;/*actual return value           */
    H5F_file_t          *shared=NULL;   /*shared part of `file'         */
    H5FD_t              *lf=NULL;       /*file driver part of `shared'  */
    uint8_t             buf[256];       /*temporary I/O buffer          */
    const uint8_t       *p;             /*ptr into temp I/O buffer      */
    hsize_t             fixed_size=24;  /*fixed sizeof superblock       */
    hsize_t             variable_size;  /*variable sizeof superblock    */
    hsize_t             driver_size;    /*size of driver info block     */
    H5G_entry_t         root_ent;       /*root symbol table entry       */
    haddr_t             eof;            /*end of file address           */
    haddr_t             stored_eoa;     /*relative end-of-addr in file  */
    unsigned            tent_flags;     /*tentative flags               */
    char                driver_name[9]; /*file driver name/version      */
    hbool_t             driver_has_cmp; /*`cmp' callback defined?       */
    
    FUNC_ENTER(H5F_open, NULL);

    /*
     * If the driver has a `cmp' method then the driver is capable of
     * determining when two file handles refer to the same file and the
     * library can insure that when the application opens a file twice that
     * the two handles coordinate their operations appropriately. Otherwise
     * it is the application's responsibility to never open the same file
     * more than once at a time.
     */
    driver_has_cmp = H5FD_has_cmp(fapl_id);

    /*
     * Opening a file is a two step process. First we try to open the file in
     * a way which doesn't affect its state (like not truncating or creating
     * it) so we can compare it with files that are already open. If that
     * fails then we try again with the full set of flags (only if they're
     * different than the original failed attempt). However, if the file
     * driver can't distinquish between files then there's no reason to open
     * the file tentatively because it's the application's responsibility to
     * prevent this situation (there's no way for us to detect it here
     * anyway).
     */
    if (driver_has_cmp) {
        tent_flags = flags & ~(H5F_ACC_CREAT|H5F_ACC_TRUNC|H5F_ACC_EXCL);
    } else {
        tent_flags = flags;
    }
    if (NULL==(lf=H5FD_open(name, tent_flags, fapl_id, HADDR_UNDEF))) {
        if (tent_flags == flags) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to open file");
        }
        H5E_clear();
        tent_flags = flags;
        if (NULL==(lf=H5FD_open(name, tent_flags, fapl_id, HADDR_UNDEF))) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to open file");
        }
    }

    /* Is the file already open? */
    if ((file=H5I_search(H5I_FILE, H5F_equal, lf)) ||
        (file=H5I_search(H5I_FILE_CLOSING, H5F_equal, lf))) {
        /*
         * The file is already open, so use that one instead of the one we
         * just opened. We only one one H5FD_t* per file so one doesn't
         * confuse the other.  But fail if this request was to truncate the
         * file (since we can't do that while the file is open), or if the
         * request was to create a non-existent file (since the file already
         * exists), or if the new request adds write access (since the
         * readers don't expect the file to change under them).
         */
        H5FD_close(lf);
        if (flags & H5F_ACC_TRUNC) {
            file = NULL; /*to prevent destruction of wrong file*/
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to truncate a file which is already open");
        }
        if (flags & H5F_ACC_EXCL) {
            file = NULL; /*to prevent destruction of wrong file*/
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "file exists");
        }
        if ((flags & H5F_ACC_RDWR) && 0==(file->intent & H5F_ACC_RDWR)) {
            file = NULL; /*to prevent destruction of wrong file*/
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "file is already open for read-only");
        }
        file = H5F_new(file->shared, fcpl_id, fapl_id);
        lf = file->shared->lf;
    } else if (flags!=tent_flags) {
        /*
         * This file is not yet open by the library and the flags we used to
         * open it are different than the desired flags. Close the tentative
         * file and open it for real.
         */
        H5FD_close(lf);
        if (NULL==(lf=H5FD_open(name, flags, fapl_id, HADDR_UNDEF))) {
            file = NULL; /*to prevent destruction of wrong file*/
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to open file");
        }
        if (NULL==(file = H5F_new(NULL, fcpl_id, fapl_id)))
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to create new file object");
        file->shared->flags = flags;
        file->shared->lf = lf;
    } else {
        /*
         * This file is not yet open by the library and our tentative opening
         * above is good enough.
         */
        if (NULL==(file = H5F_new(NULL, fcpl_id, fapl_id)))
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to create new file object");
        file->shared->flags = flags;
        file->shared->lf = lf;
    }

    /* Short cuts */
    shared = file->shared;
    lf = shared->lf;

    /*
     * The intent at the top level file struct are not necessarily the same as
     * the flags at the bottom.  The top level describes how the file can be
     * accessed through the HDF5 library.  The bottom level describes how the
     * file can be accessed through the C library.
     */
    file->intent = flags;
    file->name = H5MM_xstrdup(name);

    /*
     * Read or write the file superblock, depending on whether the file is
     * empty or not.
     */
    if (0==H5FD_get_eof(lf) && (flags & H5F_ACC_RDWR)) {
        /*
         * The superblock starts immediately after the user-defined header,
         * which we have already insured is a proper size.  The base address
         * is set to the same thing as the superblock for now.
         */
        shared->boot_addr = shared->fcpl->userblock_size;
        shared->base_addr = shared->boot_addr;
        shared->consist_flags = 0x03;
        if (H5F_flush(file, H5F_SCOPE_LOCAL, FALSE, TRUE)<0) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, NULL,
                        "unable to write file superblock");
        }

        /* Create and open the root group */
        if (H5G_mkroot(file, NULL)<0) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, NULL,
                        "unable to create/open root group");
        }
        
    } else if (1==shared->nrefs) {
        /* Read the superblock if it hasn't been read before. */
        if (HADDR_UNDEF==(shared->boot_addr=H5F_locate_signature(lf))) {
            HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, NULL,
                        "unable to find file signature");
        }
        if (H5FD_set_eoa(lf, shared->boot_addr+fixed_size)<0 ||
            H5FD_read(lf, H5FD_MEM_SUPER, H5P_DEFAULT, shared->boot_addr, fixed_size, buf)<0) {
            HGOTO_ERROR(H5E_FILE, H5E_READERROR, NULL,
                        "unable to read superblock");
        }

        /* Signature, already checked */
        p = buf + H5F_SIGNATURE_LEN;

        /* Superblock version */
        shared->fcpl->bootblock_ver = *p++;
        if (HDF5_BOOTBLOCK_VERSION!=shared->fcpl->bootblock_ver) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad superblock version number");
        }

        /* Freespace version */
        shared->fcpl->freespace_ver = *p++;
        if (HDF5_FREESPACE_VERSION!=shared->fcpl->freespace_ver) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad free space version number");
        }

        /* Root group version number */
        shared->fcpl->objectdir_ver = *p++;
        if (HDF5_OBJECTDIR_VERSION!=shared->fcpl->objectdir_ver) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad root group version number");
        }

        /* reserved */
        p++;

        /* Shared header version number */
        shared->fcpl->sharedheader_ver = *p++;
        if (HDF5_SHAREDHEADER_VERSION!=shared->fcpl->sharedheader_ver) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad shared header version number");
        }

        /* Size of file addresses */
        shared->fcpl->sizeof_addr = *p++;
        if (shared->fcpl->sizeof_addr != 2 &&
            shared->fcpl->sizeof_addr != 4 &&
            shared->fcpl->sizeof_addr != 8 &&
            shared->fcpl->sizeof_addr != 16 &&
            shared->fcpl->sizeof_addr != 32) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad file sizeof(address)");
        }

        /* Size of file sizes */
        shared->fcpl->sizeof_size = *p++;
        if (shared->fcpl->sizeof_size != 2 &&
            shared->fcpl->sizeof_size != 4 &&
            shared->fcpl->sizeof_size != 8 &&
            shared->fcpl->sizeof_size != 16 &&
            shared->fcpl->sizeof_size != 32) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad file sizeof(size)");
        }
        
        /* Reserved byte */
        p++;

        /* Various B-tree sizes */
        UINT16DECODE(p, shared->fcpl->sym_leaf_k);
        if (shared->fcpl->sym_leaf_k < 1) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad symbol table leaf node 1/2 rank");
        }
        UINT16DECODE(p, shared->fcpl->btree_k[H5B_SNODE_ID]);
        if (shared->fcpl->btree_k[H5B_SNODE_ID] < 1) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "bad symbol table internal node 1/2 rank");
        }

        /* File consistency flags. Not really used yet */
        UINT32DECODE(p, shared->consist_flags);
        assert((hsize_t)(p-buf) == fixed_size);

        /* Decode the variable-length part of the superblock... */
        variable_size = H5F_SIZEOF_ADDR(file) +         /*base addr*/
                        H5F_SIZEOF_ADDR(file) +         /*global free list*/
                        H5F_SIZEOF_ADDR(file) +         /*end-of-address*/
                        H5F_SIZEOF_ADDR(file) +         /*reserved address*/
                        H5G_SIZEOF_ENTRY(file);         /*root group ptr*/
        assert(variable_size<=sizeof(buf));
        if (H5FD_set_eoa(lf, shared->boot_addr+fixed_size+variable_size)<0 ||
            H5FD_read(lf, H5FD_MEM_SUPER, H5P_DEFAULT, shared->boot_addr+fixed_size,
                      variable_size, buf)<0) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to read superblock");
        }
        p = buf;
        H5F_addr_decode(file, &p, &(shared->base_addr)/*out*/);
        H5F_addr_decode(file, &p, &(shared->freespace_addr)/*out*/);
        H5F_addr_decode(file, &p, &stored_eoa/*out*/);
        H5F_addr_decode(file, &p, &(shared->driver_addr)/*out*/);
        if (H5G_ent_decode(file, &p, &root_ent/*out*/)<0) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to read root symbol entry");
        }

        /* Decode the optional driver information block */
        if (H5F_addr_defined(shared->driver_addr)) {
            haddr_t drv_addr = shared->base_addr + shared->driver_addr;
            if (H5FD_set_eoa(lf, drv_addr+16)<0 ||
                H5FD_read(lf, H5FD_MEM_SUPER, H5P_DEFAULT, drv_addr, (hsize_t)16, buf)<0) {
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                            "unable to read driver information block");
            }
            p = buf;

            /* Version number */
            if (HDF5_DRIVERINFO_VERSION!=*p++) {
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                            "bad driver information block version number");
            }

            /* Reserved */
            p += 3;

            /* Driver info size */
            UINT32DECODE(p, driver_size);

            /* Driver name and/or version */
            strncpy(driver_name, (const char *)p, 8);
            driver_name[8] = '\0';

            /* Read driver information and decode */
            if (H5FD_set_eoa(lf, drv_addr+16+driver_size)<0 ||
                H5FD_read(lf, H5FD_MEM_SUPER, H5P_DEFAULT, drv_addr+16, driver_size, buf)<0) {
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                            "unable to read file driver information");
            }
            if (H5FD_sb_decode(lf, driver_name, buf)<0) {
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                            "unable to decode driver information");
            }
        }
        
        /* Make sure we can open the root group */
        if (H5G_mkroot(file, &root_ent)<0) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to read root group");
        }

        /*
         * The user-defined data is the area of the file before the base
         * address.
         */
        shared->fcpl->userblock_size = shared->base_addr;

        /*
         * Make sure that the data is not truncated. One case where this is
         * possible is if the first file of a family of files was opened
         * individually.
         */
        if (HADDR_UNDEF==(eof=H5FD_get_eof(lf))) {
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                        "unable to determine file size");
        }
        if (eof<stored_eoa) {
            HGOTO_ERROR(H5E_FILE, H5E_TRUNCATED, NULL, "truncated file");
        }
        
        /*
         * Tell the file driver how much address space has already been
         * allocated so that it knows how to allocated additional memory.
         */
        if (H5FD_set_eoa(lf, stored_eoa)<0) {
            HRETURN_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL,
                          "unable to set end-of-address marker for file");
        }
    }

    /* Success */
    ret_value = file;

 done:
    if (!ret_value && file) H5F_dest(file);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fcreate
 *
 * Purpose:     This is the primary function for creating HDF5 files . The
 *              flags parameter determines whether an existing file will be
 *              overwritten or not.  All newly created files are opened for
 *              both reading and writing.  All flags may be combined with the
 *              bit-wise OR operator (`|') to change the behavior of the file
 *              create call.
 *
 *              The more complex behaviors of a file's creation and access
 *              are controlled through the file-creation and file-access
 *              property lists.  The value of H5P_DEFAULT for a template
 *              value indicates that the library should use the default
 *              values for the appropriate template.
 *
 * See also:    H5Fpublic.h for the list of supported flags. H5Ppublic.h for
 *              the list of file creation and file access properties.
 *
 * Return:      Success:        A file ID
 *
 *              Failure:        FAIL
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *              Robb Matzke, 1997-07-18
 *              File struct creation and destruction is through H5F_new() and
 *              H5F_dest(). Writing the root symbol table entry is done with
 *              H5G_encode().
 *      
 *              Robb Matzke, 1997-08-29
 *              Moved creation of the boot block to H5F_flush().
 *      
 *              Robb Matzke, 1997-09-23
 *              Most of the work is now done by H5F_open() since H5Fcreate()
 *              and H5Fopen() originally contained almost identical code.
 *
 *              Robb Matzke, 1998-02-18
 *              Better error checking for the creation and access property
 *              lists. It used to be possible to swap the two and core the
 *              library.  Also, zero is no longer valid as a default property
 *              list; one must use H5P_DEFAULT instead.
 *
 *              Robb Matzke, 1999-08-02
 *              The file creation and file access property lists are passed
 *              to the H5F_open() as object IDs.
 *-------------------------------------------------------------------------
 */
hid_t
H5Fcreate(const char *filename, unsigned flags, hid_t fcpl_id,
          hid_t fapl_id)
{
    
    H5F_t       *new_file = NULL;       /*file struct for new file      */
    hid_t       ret_value = FAIL;       /*return value                  */

    FUNC_ENTER(H5Fcreate, FAIL);
    H5TRACE4("i","sIuii",filename,flags,fcpl_id,fapl_id);

    /* Check/fix arguments */
    if (!filename || !*filename) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file name");
    }
    if (flags & ~(H5F_ACC_EXCL|H5F_ACC_TRUNC|H5F_ACC_DEBUG)) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid flags");
    }
    if ((flags & H5F_ACC_EXCL) && (flags & H5F_ACC_TRUNC)) {
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                     "mutually exclusive flags for file creation");
    }
    if (H5P_DEFAULT!=fcpl_id &&
        (H5P_FILE_CREATE!=H5P_get_class(fcpl_id) ||
         NULL==H5I_object(fcpl_id))) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                    "not a file creation property list");
    }
    if (H5P_DEFAULT!=fapl_id &&
        (H5P_FILE_ACCESS!=H5P_get_class(fapl_id) ||
         NULL==H5I_object(fapl_id))) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                    "not a file access property list");
    }

    /*
     * Adjust bit flags by turning on the creation bit and making sure that
     * the EXCL or TRUNC bit is set.  All newly-created files are opened for
     * reading and writing.
     */
    if (0==(flags & (H5F_ACC_EXCL|H5F_ACC_TRUNC))) {
        flags |= H5F_ACC_EXCL;   /*default*/
    }
    flags |= H5F_ACC_RDWR | H5F_ACC_CREAT;

    /*
     * Create a new file or truncate an existing file.
     */
    if (NULL==(new_file=H5F_open(filename, flags, fcpl_id, fapl_id))) {
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to create file");
    }
    
    /* Get an atom for the file */
    if ((ret_value = H5I_register(H5I_FILE, new_file))<0) {
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                    "unable to atomize file");
    }

  done:
    if (ret_value<0 && new_file) H5F_close(new_file);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fopen
 *
 * Purpose:     This is the primary function for accessing existing HDF5
 *              files.  The FLAGS argument determines whether writing to an
 *              existing file will be allowed or not.  All flags may be
 *              combined with the bit-wise OR operator (`|') to change the
 *              behavior of the file open call.  The more complex behaviors
 *              of a file's access are controlled through the file-access
 *              property list.
 *
 * See Also:    H5Fpublic.h for a list of possible values for FLAGS.
 *
 * Return:      Success:        A file ID
 *
 *              Failure:        FAIL
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *              Robb Matzke, 1997-07-18
 *              File struct creation and destruction is through H5F_new() and
 *              H5F_dest(). Reading the root symbol table entry is done with
 *              H5G_decode().
 *      
 *              Robb Matzke, 1997-09-23
 *              Most of the work is now done by H5F_open() since H5Fcreate()
 *              and H5Fopen() originally contained almost identical code.
 *
 *              Robb Matzke, 1998-02-18
 *              Added better error checking for the flags and the file access
 *              property list.  It used to be possible to make the library
 *              dump core by passing an object ID that was not a file access
 *              property list.
 *
 *              Robb Matzke, 1999-08-02
 *              The file access property list is passed to the H5F_open() as
 *              object IDs.
 *-------------------------------------------------------------------------
 */
hid_t
H5Fopen(const char *filename, unsigned flags, hid_t fapl_id)
{
    H5F_t       *new_file = NULL;       /*file struct for new file      */
    hid_t       ret_value = FAIL;       /*return value                  */

    FUNC_ENTER(H5Fopen, FAIL);
    H5TRACE3("i","sIui",filename,flags,fapl_id);

    /* Check/fix arguments. */
    if (!filename || !*filename) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file name");
    }
    if ((flags & ~H5F_ACC_PUBLIC_FLAGS) ||
        (flags & H5F_ACC_TRUNC) || (flags & H5F_ACC_EXCL)) {
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file open flags");
    }
    if (H5P_DEFAULT!=fapl_id &&
        (H5P_FILE_ACCESS!=H5P_get_class(fapl_id) ||
         NULL==H5I_object(fapl_id))) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                    "not a file access property list");
    }

    /* Open the file */
    if (NULL==(new_file=H5F_open(filename, flags, H5P_DEFAULT, fapl_id))) {
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to open file");
    }

    /* Get an atom for the file */
    if ((ret_value = H5I_register(H5I_FILE, new_file))<0) {
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                    "unable to atomize file handle");
    }

 done:
    if (ret_value<0 && new_file) H5F_close(new_file);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fflush
 *
 * Purpose:     Flushes all outstanding buffers of a file to disk but does
 *              not remove them from the cache.  The OBJECT_ID can be a file,
 *              dataset, group, attribute, or named data type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, August  6, 1998
 *
 * Modifications:
 *
 *              Robb Matzke, 1998-10-16
 *              Added the `scope' argument.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fflush(hid_t object_id, H5F_scope_t scope)
{
    H5F_t       *f = NULL;
    H5G_t       *grp = NULL;
    H5T_t       *type = NULL;
    H5D_t       *dset = NULL;
    H5A_t       *attr = NULL;
    H5G_entry_t *ent = NULL;
    
    FUNC_ENTER(H5Fflush, FAIL);
    H5TRACE2("e","iFs",object_id,scope);

    switch (H5I_get_type(object_id)) {
    case H5I_FILE:
        if (NULL==(f=H5I_object(object_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                          "invalid file identifier");
        }
        break;

    case H5I_GROUP:
        if (NULL==(grp=H5I_object(object_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                          "invalid group identifier");
        }
        ent = H5G_entof(grp);
        break;

    case H5I_DATATYPE:
        if (NULL==(type=H5I_object(object_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                          "invalid type identifier");
        }
        ent = H5T_entof(type);
        break;

    case H5I_DATASET:
        if (NULL==(dset=H5I_object(object_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                          "invalid dataset identifier");
        }
        ent = H5D_entof(dset);
        break;

    case H5I_ATTR:
        if (NULL==(attr=H5I_object(object_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                          "invalid attribute identifier");
        }
        ent = H5A_entof(attr);
        break;

    default:
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not a file or file object");
    }

    if (!f) {
        if (!ent) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                          "object is not assocated with a file");
        }
        f = ent->file;
    }
    if (!f) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "object is not associated with a file");
    }

    /* Flush the file */
    if (H5F_flush(f, scope, FALSE, FALSE)<0) {
        HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                      "flush failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_flush
 *
 * Purpose:     Flushes (and optionally invalidates) cached data plus the
 *              file boot block.  If the logical file size field is zero
 *              then it is updated to be the length of the boot block.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 29 1997
 *
 * Modifications:
 *              rky 1998-08-28
 *              Only p0 writes metadata to disk.
 *
 *              Robb Matzke, 1998-10-16
 *              Added the `scope' argument to indicate what should be
 *              flushed. If the value is H5F_SCOPE_GLOBAL then the entire
 *              virtual file is flushed; a value of H5F_SCOPE_LOCAL means
 *              that only the specified file is flushed.  A value of
 *              H5F_SCOPE_DOWN means flush the specified file and all
 *              children.
 *
 *              Robb Matzke, 1999-08-02
 *              If ALLOC_ONLY is non-zero then all this function does is
 *              allocate space for the userblock and superblock. Also
 *              rewritten to use the virtual file layer.
 *
 *              Robb Matzke, 1999-08-16
 *              The driver information block is encoded and either allocated
 *              or written to disk.
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_flush(H5F_t *f, H5F_scope_t scope, hbool_t invalidate,
          hbool_t alloc_only)
{
    uint8_t             sbuf[2048], dbuf[2048], *p=NULL;
    unsigned            nerrors=0, i;
    hsize_t             superblock_size, driver_size;
    char                driver_name[9];
    
    FUNC_ENTER(H5F_flush, FAIL);
        
    /*
     * Nothing to do if the file is read only.  This determination is made at
     * the shared open(2) flags level, implying that opening a file twice,
     * once for read-only and once for read-write, and then calling
     * H5F_flush() with the read-only handle, still causes data to be flushed.
     */
    if (0 == (H5F_ACC_RDWR & f->shared->flags)) {
        HRETURN(SUCCEED);
    }

    /* Flush other stuff depending on scope */
    if (H5F_SCOPE_GLOBAL==scope) {
        while (f->mtab.parent) f = f->mtab.parent;
        scope = H5F_SCOPE_DOWN;
    }
    if (H5F_SCOPE_DOWN==scope) {
        for (i=0; i<f->mtab.nmounts; i++) {
            if (H5F_flush(f->mtab.child[i].file, scope, invalidate, FALSE)<0) {
                nerrors++;
            }
        }
    }

    /* flush the data sieve buffer, if we have a dirty one */
    if(!alloc_only && f->shared->sieve_buf && f->shared->sieve_dirty) {
        /* Write dirty data sieve buffer to file */
        if (H5F_block_write(f, H5FD_MEM_DRAW, f->shared->sieve_loc, f->shared->sieve_size, H5P_DEFAULT, f->shared->sieve_buf)<0) {
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
              "block write failed");
        }

        /* Reset sieve buffer dirty flag */
        f->shared->sieve_dirty=0;
    } /* end if */

    /* flush the entire raw data cache */
    if (!alloc_only && H5F_istore_flush (f, invalidate)<0) {
        HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                      "unable to flush raw data cache");
    }
    
    /* flush (and invalidate) the entire meta data cache */
    if (!alloc_only && H5AC_flush(f, NULL, HADDR_UNDEF, invalidate)<0) {
        HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                      "unable to flush meta data cache");
    }

    /* encode the file boot block */
    p = sbuf;
    HDmemcpy(p, H5F_SIGNATURE, H5F_SIGNATURE_LEN);
    p += H5F_SIGNATURE_LEN;
    *p++ = f->shared->fcpl->bootblock_ver;
    *p++ = f->shared->fcpl->freespace_ver;
    *p++ = f->shared->fcpl->objectdir_ver;
    *p++ = 0;                   /*reserved*/
    *p++ = f->shared->fcpl->sharedheader_ver;
    assert (H5F_SIZEOF_ADDR(f)<=255);
    *p++ = (uint8_t)H5F_SIZEOF_ADDR(f);
    assert (H5F_SIZEOF_SIZE(f)<=255);
    *p++ = (uint8_t)H5F_SIZEOF_SIZE(f);
    *p++ = 0;                   /*reserved */
    UINT16ENCODE(p, f->shared->fcpl->sym_leaf_k);
    UINT16ENCODE(p, f->shared->fcpl->btree_k[H5B_SNODE_ID]);
    UINT32ENCODE(p, f->shared->consist_flags);
    H5F_addr_encode(f, &p, f->shared->base_addr);
    H5F_addr_encode(f, &p, f->shared->freespace_addr);
    H5F_addr_encode(f, &p, H5FD_get_eoa(f->shared->lf));
    H5F_addr_encode(f, &p, f->shared->driver_addr);
    H5G_ent_encode(f, &p, H5G_entof(f->shared->root_grp));
    superblock_size = p-sbuf;

    /*
     * Encode the driver information block.
     */
    if ((driver_size=H5FD_sb_size(f->shared->lf))) {
        driver_size += 16; /*driver block header */
        assert(driver_size<=sizeof(dbuf));
        p = dbuf;

        /* Version */
        *p++ = HDF5_DRIVERINFO_VERSION;

        /* Reserved*/
        p += 3;

        /* Driver info size, excluding header */
        UINT32ENCODE(p, driver_size-16);

        /* Encode driver-specific data */
        if (H5FD_sb_encode(f->shared->lf, driver_name, dbuf+16)<0) {
            HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                          "unable to encode driver information");
        }

        /* Driver name */
        HDmemcpy(dbuf+8, driver_name, 8);
    }

    if (alloc_only) {
        /*
         * Allocate space for the userblock, superblock, and driver info
         * block. We do it with one allocation request because the userblock
         * and superblock need to be at the beginning of the file and only
         * the first allocation request is required to return memory at
         * format address zero.
         */
        haddr_t addr = H5FD_alloc(f->shared->lf, H5FD_MEM_SUPER,
                                  (f->shared->base_addr +
                                   superblock_size +
                                   driver_size));
        if (HADDR_UNDEF==addr) {
            HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                          "unable to allocate file space for userblock "
                          "and/or superblock");
        }
        if (0!=addr) {
            HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                          "file driver failed to allocate userblock "
                          "and/or superblock at address zero");
        }
        
        /*
         * The file driver information block begins immediately after the
         * superblock.
         */
        if (driver_size>0) {
            f->shared->driver_addr = superblock_size;
        }
        
    } else {
        /* Write superblock */
#ifdef H5_HAVE_PARALLEL
        if (IS_H5FD_MPIO(f))
            H5FD_mpio_tas_allsame(f->shared->lf, TRUE); /*only p0 will write*/
#endif
        if (H5FD_write(f->shared->lf, H5FD_MEM_SUPER, H5P_DEFAULT, f->shared->boot_addr,
                       superblock_size, sbuf)<0) {
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                          "unable to write superblock");
        }

        /* Write driver information block */
        if (HADDR_UNDEF!=f->shared->driver_addr) {
#ifdef H5_HAVE_PARALLEL
            if (IS_H5FD_MPIO(f))
                H5FD_mpio_tas_allsame(f->shared->lf, TRUE); /*only p0 will write*/
#endif
            if (H5FD_write(f->shared->lf, H5FD_MEM_SUPER, H5P_DEFAULT,
                           f->shared->base_addr+superblock_size, driver_size,
                           dbuf)<0) {
                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                              "unable to write driver information block");
            }
        }
    }

    /* Flush file buffers to disk */
    if (!alloc_only && H5FD_flush(f->shared->lf)<0) {
        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "low level flush failed");
    }

    /* Check flush errors for children - errors are already on the stack */
    if (nerrors) HRETURN(FAIL);
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_close
 *
 * Purpose:     Closes a file or causes the close operation to be pended.
 *              This function is called two ways: from the API it gets called
 *              by H5Fclose->H5I_dec_ref->H5F_close when H5I_dec_ref()
 *              decrements the file ID reference count to zero.  The file ID
 *              is removed from the H5I_FILE group by H5I_dec_ref() just
 *              before H5F_close() is called. If there are open object
 *              headers then the close is pended by moving the file to the
 *              H5I_FILE_CLOSING ID group (the f->closing contains the ID
 *              assigned to file).
 *
 *              This function is also called directly from H5O_close() when
 *              the last object header is closed for the file and the file
 *              has a pending close.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, September 23, 1997
 *
 * Modifications:
 *              Robb Matzke, 1998-10-14
 *              Nothing happens unless the H5F_t reference count is one (the
 *              file is flushed anyway).  The reference count is decremented
 *              by H5F_dest().
 *
 *              Robb Matzke, 1999-08-02
 *              Modified to use the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_close(H5F_t *f)
{
    unsigned    i;

    FUNC_ENTER(H5F_close, FAIL);
    assert(f->nrefs>0);

    /*
     * If this file is referenced more than once then just decrement the
     * count, flush the file, and return.
     */
    if (f->nrefs>1) {
        if (H5F_flush(f, H5F_SCOPE_LOCAL, FALSE, FALSE)<0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush cache");
        }
        H5F_dest(f); /*decrement reference counts*/
        HRETURN(SUCCEED);
    }
    
    /*
     * Unmount and close each child before closing the current file.
     */
    assert(NULL==f->mtab.parent);
    for (i=0; i<f->mtab.nmounts; i++) {
        f->mtab.child[i].file->mtab.parent = NULL;
        H5G_close(f->mtab.child[i].group);
        H5F_close(f->mtab.child[i].file);
    }
    f->mtab.nmounts = 0;

    /*
     * If object headers are still open then delay deletion of resources until
     * they have all been closed.  Flush all caches and update the object
     * header anyway so that failing to close all objects isn't a major
     * problem. If the file is on the H5I_FILE list then move it to the
     * H5I_FILE_CLOSING list instead.
     */
    if (f->nopen_objs>0) {
        if (H5F_flush(f, H5F_SCOPE_LOCAL, FALSE, FALSE)<0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush cache");
        }
#ifdef H5F_DEBUG
        if (H5DEBUG(F)) {
            fprintf(H5DEBUG(F), "H5F: H5F_close(%s): %u object header%s still "
                    "open (file close will complete when %s closed)\n",
                    f->name,
                    f->nopen_objs,
                    1 == f->nopen_objs?" is":"s are",
                    1 == f->nopen_objs?"that header is":"those headers are");
        }
#endif
        if (!f->closing) {
            f->closing  = H5I_register(H5I_FILE_CLOSING, f);
        }
        HRETURN(SUCCEED);
    } else if (f->closing) {
#ifdef H5F_DEBUG
        if (H5DEBUG(F)) {
            fprintf(H5DEBUG(F), "H5F: H5F_close: operation completing\n");
        }
#endif
    }

    /*
     * If this is the last reference to the shared part of the file then
     * close it also.
     */
    assert(1==f->nrefs);
    if (1==f->shared->nrefs) {
        /* Flush and destroy all caches */
        if (H5F_flush(f, H5F_SCOPE_LOCAL, TRUE, FALSE)<0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush cache");
        }

        /* Dump debugging info */
        H5AC_debug(f);
        H5F_istore_stats(f, FALSE);
    } else {
        /*
         * Flush all caches but do not destroy. As long as all handles for
         * this file are closed the flush isn't really necessary, but lets
         * just be safe.
         */
        if (H5F_flush(f, H5F_SCOPE_LOCAL, TRUE, FALSE)<0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush cache");
        }
    }

    /*
     * Destroy the H5F_t struct and decrement the reference count for the
     * shared H5F_file_t struct. If the reference count for the H5F_file_t
     * struct reaches zero then destroy it also.
     */
    if (H5F_dest(f)<0) {
        HRETURN_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "problems closing file");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fclose
 *
 * Purpose:     This function closes the file specified by FILE_ID by
 *              flushing all data to storage, and terminating access to the
 *              file through FILE_ID.  If objects (e.g., datasets, groups,
 *              etc.) are open in the file then the underlying storage is not
 *              closed until those objects are closed; however, all data for
 *              the file and the open objects is flushed.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Saturday, February 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclose(hid_t file_id)
{
    
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER(H5Fclose, FAIL);
    H5TRACE1("e","i",file_id);

    /* Check/fix arguments. */
    if (H5I_FILE != H5I_get_type(file_id)) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file atom");
    }
    if (NULL == H5I_object(file_id)) {
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "unable to unatomize file");
    }

    /*
     * Decrement reference count on atom.  When it reaches zero the file will
     * be closed.
     */
    if (H5I_dec_ref (file_id)<0) {
        HGOTO_ERROR (H5E_ATOM, H5E_CANTINIT, FAIL, "problems closing file");
    }
    
done:
    FUNC_LEAVE(ret_value < 0 ? FAIL : SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_mount
 *
 * Purpose:     Mount file CHILD onto the group specified by LOC and NAME,
 *              using mount properties in PLIST.  CHILD must not already be
 *              mouted and must not be a mount ancestor of the mount-point.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October  6, 1998
 *
 * Modifications:
 *
 *      Robb Matzke, 1998-10-14
 *      The reference count for the mounted H5F_t is incremented.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_mount(H5G_entry_t *loc, const char *name, H5F_t *child,
          const H5F_mprop_t UNUSED *plist)
{
    H5G_t       *mount_point = NULL;    /*mount point group             */
    H5G_entry_t *mp_ent = NULL;         /*mount point symbol table entry*/
    H5F_t       *ancestor = NULL;       /*ancestor files                */
    H5F_t       *parent = NULL;         /*file containing mount point   */
    int lt, rt, md, cmp;        /*binary search indices         */
    H5G_entry_t *ent = NULL;            /*temporary symbol table entry  */
    herr_t      ret_value = FAIL;       /*return value                  */
    
    FUNC_ENTER(H5F_mount, FAIL);
    assert(loc);
    assert(name && *name);
    assert(child);
    assert(plist);

    /*
     * Check that the child isn't mounted, that the mount point exists, and
     * that the mount wouldn't introduce a cycle in the mount tree.
     */
    if (child->mtab.parent) {
        HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL, "file is already mounted");
    }
    if (NULL==(mount_point=H5G_open(loc, name))) {
        HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL, "mount point not found");
    }
    parent = H5G_fileof(mount_point);
    mp_ent = H5G_entof(mount_point);
    for (ancestor=parent; ancestor; ancestor=ancestor->mtab.parent) {
        if (ancestor==child) {
            HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL,
                        "mount would introduce a cycle");
        }
    }
    
    /*
     * Use a binary search to locate the position that the child should be
     * inserted into the parent mount table.  At the end of this paragraph
     * `md' will be the index where the child should be inserted.
     */
    lt = md = 0;
    rt = parent->mtab.nmounts;
    cmp = -1;
    while (lt<rt && cmp) {
        md = (lt+rt)/2;
        ent = H5G_entof(parent->mtab.child[md].group);
        cmp = H5F_addr_cmp(mp_ent->header, ent->header);
        if (cmp<0) {
            rt = md;
        } else if (cmp>0) {
            lt = md+1;
        }
    }
    if (cmp>0) md++;
    if (!cmp) {
        HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL,
                    "mount point is already in use");
    }
    
    /* Make room in the table */
    if (parent->mtab.nmounts>=parent->mtab.nalloc) {
        unsigned n = MAX(16, 2*parent->mtab.nalloc);
        H5F_mount_t *x = H5MM_realloc(parent->mtab.child,
                                      n*sizeof(parent->mtab.child[0]));
        if (!x) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                        "memory allocation failed for mount table");
        }
        parent->mtab.child = x;
        parent->mtab.nalloc = n;
    }

    /* Insert into table */
    HDmemmove(parent->mtab.child+md+1,
              parent->mtab.child+md,
              (parent->mtab.nmounts-md)*sizeof(parent->mtab.child[0]));
    parent->mtab.nmounts++;
    parent->mtab.child[md].group = mount_point;
    parent->mtab.child[md].file = child;
    child->mtab.parent = parent;
    child->nrefs++;
    ret_value = SUCCEED;

 done:
    if (ret_value<0 && mount_point) {
        H5G_close(mount_point);
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_unmount
 *
 * Purpose:     Unmount the child which is mounted at the group specified by
 *              LOC and NAME or fail if nothing is mounted there.  Neither
 *              file is closed.
 *
 *              Because the mount point is specified by name and opened as a
 *              group, the H5G_namei() will resolve it to the root of the
 *              mounted file, not the group where the file is mounted.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October  6, 1998
 *
 * Modifications:
 *
 *      Robb Matzke, 1998-10-14
 *      The ref count for the child is decremented by calling H5F_close().
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_unmount(H5G_entry_t *loc, const char *name)
{
    H5G_t       *mounted = NULL;        /*mount point group             */
    H5G_entry_t *mnt_ent = NULL;        /*mounted symbol table entry    */
    H5F_t       *child = NULL;          /*mounted file                  */
    H5F_t       *parent = NULL;         /*file where mounted            */
    H5G_entry_t *ent = NULL;            /*temporary symbol table entry  */
    herr_t      ret_value = FAIL;       /*return value                  */
    unsigned    i;                      /*coutners                      */
    int lt, rt, md=(-1), cmp;   /*binary search indices         */
    
    FUNC_ENTER(H5F_unmount, FAIL);
    assert(loc);
    assert(name && *name);

    /*
     * Get the mount point, or more precisely the root of the mounted file.
     * If we get the root group and the file has a parent in the mount tree,
     * then we must have found the mount point.
     */
    if (NULL==(mounted=H5G_open(loc, name))) {
        HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL, "mount point not found");
    }
    child = H5G_fileof(mounted);
    mnt_ent = H5G_entof(mounted);
    ent = H5G_entof(child->shared->root_grp);

    if (child->mtab.parent &&
        H5F_addr_eq(mnt_ent->header, ent->header)) {
        /*
         * We've been given the root group of the child.  We do a reverse
         * lookup in the parent's mount table to find the correct entry.
         */
        parent = child->mtab.parent;
        for (i=0; i<parent->mtab.nmounts; i++) {
            if (parent->mtab.child[i].file==child) {
                /* Unmount the child */
                parent->mtab.nmounts -= 1;
                H5G_close(parent->mtab.child[i].group);
                child->mtab.parent = NULL;
                H5F_close(child);
                HDmemmove(parent->mtab.child+i,
                          parent->mtab.child+i+1,
                          ((parent->mtab.nmounts-i)*
                           sizeof(parent->mtab.child[0])));
                ret_value = SUCCEED;
            }
        }
        assert(ret_value>=0);
        
    } else {
        /*
         * We've been given the mount point in the parent.  We use a binary
         * search in the parent to locate the mounted file, if any.
         */
        parent = child; /*we guessed wrong*/
        lt = 0;
        rt = parent->mtab.nmounts;
        cmp = -1;
        while (lt<rt && cmp) {
            md = (lt+rt)/2;
            ent = H5G_entof(parent->mtab.child[md].group);
            cmp = H5F_addr_cmp(mnt_ent->header, ent->header);
            if (cmp<0) {
                rt = md;
            } else {
                lt = md+1;
            }
        }
        if (cmp) {
            HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL, "not a mount point");
        }

        /* Unmount the child */
        parent->mtab.nmounts -= 1;
        H5G_close(parent->mtab.child[md].group);
        parent->mtab.child[md].file->mtab.parent = NULL;
        H5F_close(parent->mtab.child[md].file);
        HDmemmove(parent->mtab.child+md,
                  parent->mtab.child+md+1,
                  (parent->mtab.nmounts-md)*sizeof(parent->mtab.child[0]));
        ret_value = SUCCEED;
    }
    
 done:
    if (mounted) H5G_close(mounted);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_mountpoint
 *
 * Purpose:     If ENT is a mount point then copy the entry for the root
 *              group of the mounted file into ENT.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_mountpoint(H5G_entry_t *find/*in,out*/)
{
    H5F_t       *parent = find->file;
    int lt, rt, md=(-1), cmp;
    H5G_entry_t *ent = NULL;
    
    FUNC_ENTER(H5F_mountpoint, FAIL);
    assert(find);

    /*
     * The loop is necessary because we might have file1 mounted at the root
     * of file2, which is mounted somewhere in file3.
     */
    do {
        /*
         * Use a binary search to find the potential mount point in the mount
         * table for the parent
         */
        lt = 0;
        rt = parent->mtab.nmounts;
        cmp = -1;
        while (lt<rt && cmp) {
            md = (lt+rt)/2;
            ent = H5G_entof(parent->mtab.child[md].group);
            cmp = H5F_addr_cmp(find->header, ent->header);
            if (cmp<0) {
                rt = md;
            } else {
                lt = md+1;
            }
        }

        /* Copy root info over to ENT */
        if (0==cmp) {
            ent = H5G_entof(parent->mtab.child[md].file->shared->root_grp);
            *find = *ent;
            parent = ent->file;
        }
    } while (!cmp);
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Fmount
 *
 * Purpose:     Mount file CHILD_ID onto the group specified by LOC_ID and
 *              NAME using mount properties PLIST_ID.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fmount(hid_t loc_id, const char *name, hid_t child_id, hid_t plist_id)
{
    H5G_entry_t         *loc = NULL;
    const H5F_mprop_t   *plist = NULL;
    H5F_t               *child = NULL;
    
    FUNC_ENTER(H5Fmount, FAIL);
    H5TRACE4("e","isii",loc_id,name,child_id,plist_id);

    /* Check arguments */
    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }
    if (H5I_FILE!=H5I_get_type(child_id) ||
        NULL==(child=H5I_object(child_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file");
    }
    if (H5P_DEFAULT==plist_id) {
        plist = &H5F_mount_dflt;
    } else if (H5P_MOUNT!=H5P_get_class(plist_id) ||
               NULL==(plist=H5I_object(plist_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not a mount property list");
    }

    /* Do the mount */
    if (H5F_mount(loc, name, child, plist)<0) {
        HRETURN_ERROR(H5E_FILE, H5E_MOUNT, FAIL,
                      "unable to mount file");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Funmount
 *
 * Purpose:     Given a mount point, dissassociate the mount point's file
 *              from the file mounted there.  Do not close either file.
 *
 *              The mount point can either be the group in the parent or the
 *              root group of the mounted file (both groups have the same
 *              name).  If the mount point was opened before the mount then
 *              it's the group in the parent, but if it was opened after the
 *              mount then it's the root group of the child.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Funmount(hid_t loc_id, const char *name)
{
    H5G_entry_t         *loc = NULL;
    
    FUNC_ENTER(H5Funmount, FAIL);
    H5TRACE2("e","is",loc_id,name);

    /* Check args */
    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }

    /* Unmount */
    if (H5F_unmount(loc, name)<0) {
        HRETURN_ERROR(H5E_FILE, H5E_MOUNT, FAIL,
                      "unable to unmount file");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Freopen
 *
 * Purpose:     Reopen a file.  The new file handle which is returned points
 *              to the same file as the specified file handle.  Both handles
 *              share caches and other information.  The only difference
 *              between the handles is that the new handle is not mounted
 *              anywhere and no files are mounted on it.
 *
 * Return:      Success:        New file ID
 *
 *              Failure:        FAIL
 *
 * Programmer:  Robb Matzke
 *              Friday, October 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Freopen(hid_t file_id)
{
    H5F_t       *old_file=NULL;
    H5F_t       *new_file=NULL;
    hid_t       ret_value = -1;
    

    FUNC_ENTER(H5Freopen, FAIL);
    H5TRACE1("i","i",file_id);

    if (H5I_FILE!=H5I_get_type(file_id) ||
        NULL==(old_file=H5I_object(file_id))) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file");
    }
    if (NULL==(new_file=H5F_new(old_file->shared, H5P_DEFAULT, H5P_DEFAULT))) {
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to reopen file");
    }
    if ((ret_value=H5I_register(H5I_FILE, new_file))<0) {
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                    "unable to atomize file handle");
    }

 done:
    if (ret_value<0 && new_file) {
        H5F_close(new_file);
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_get_intent
 *
 * Purpose:     Quick and dirty routine to retrieve the file's 'intent' flags
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:      'intent' on success/abort on failure (shouldn't fail)
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              September 29, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5F_get_intent(H5F_t *f)
{
    FUNC_ENTER(H5F_get_intent, 0);

    assert(f);

    FUNC_LEAVE(f->intent);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_sizeof_addr
 *
 * Purpose:     Quick and dirty routine to retrieve the size of the file's size_t
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:      'sizeof_addr' on success/abort on failure (shouldn't fail)
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              September 29, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5F_sizeof_addr(H5F_t *f)
{
    FUNC_ENTER(H5F_sizeof_addr, 0);

    assert(f);

    FUNC_LEAVE((f)->shared->fcpl->sizeof_addr)
}


/*-------------------------------------------------------------------------
 * Function:    H5F_sizeof_size
 *
 * Purpose:     Quick and dirty routine to retrieve the size of the file's off_t
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:      'sizeof_size' on success/abort on failure (shouldn't fail)
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              September 29, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5F_sizeof_size(H5F_t *f)
{
    FUNC_ENTER(H5F_sizeof_size, 0);

    assert(f);

    FUNC_LEAVE((f)->shared->fcpl->sizeof_addr)
}


/*-------------------------------------------------------------------------
 * Function:    H5F_get_intent
 *
 * Purpose:     Quick and dirty routine to retrieve the file's 'driver_id' value
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:      'driver_id' on success/abort on failure (shouldn't fail)
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              October 10, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5F_get_driver_id(H5F_t *f)
{
    FUNC_ENTER(H5F_get_driver_id, 0);

    assert(f);

    FUNC_LEAVE(f->shared->lf->driver_id);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_block_read
 *
 * Purpose:     Reads some data from a file/server/etc into a buffer.
 *              The data is contiguous.  The address is relative to the base
 *              address for the file.
 *
 * Errors:
 *              IO        READERROR     Low-level read failed. 
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *
 * Modifications:
 *              Albert Cheng, 1998-06-02
 *              Added XFER_MODE argument
 *
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *
 *              Robb Matzke, 1999-08-02
 *              Modified to use the virtual file layer. The data transfer
 *              property list is passed in by object ID since that's how the
 *              virtual file layer needs it.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_block_read(H5F_t *f, H5FD_mem_t type, haddr_t addr, hsize_t size, hid_t dxpl_id,
               void *buf/*out*/)
{
    haddr_t                 abs_addr;

    FUNC_ENTER(H5F_block_read, FAIL);

    assert(size<SIZET_MAX);

    /* convert the relative address to an absolute address */
    abs_addr = f->shared->base_addr + addr;

    /* Read the data */
    if (H5FD_read(f->shared->lf, type, dxpl_id, abs_addr, size, buf)<0) {
        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed");
    }
    
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5F_block_write
 *
 * Purpose:     Writes some data from memory to a file/server/etc.  The
 *              data is contiguous.  The address is relative to the base
 *              address.
 *
 * Errors:
 *              IO        WRITEERROR    Low-level write failed. 
 *              IO        WRITEERROR    No write intent. 
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *
 * Modifications:
 *              Albert Cheng, 1998-06-02
 *              Added XFER_MODE argument
 *
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *
 *              Robb Matzke, 1999-08-02
 *              Modified to use the virtual file layer. The data transfer
 *              property list is passed in by object ID since that's how the
 *              virtual file layer needs it.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_block_write(H5F_t *f, H5FD_mem_t type, haddr_t addr, hsize_t size,
        hid_t dxpl_id, const void *buf)
{
    haddr_t                 abs_addr;

    FUNC_ENTER(H5F_block_write, FAIL);

    assert (size<SIZET_MAX);

    if (0==(f->intent & H5F_ACC_RDWR)) {
        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "no write intent");
    }

    /* Convert the relative address to an absolute address */
    abs_addr = f->shared->base_addr + addr;

    /* Write the data */
    if (H5FD_write(f->shared->lf, type, dxpl_id, abs_addr, size, buf)) {
        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_addr_encode
 *
 * Purpose:     Encodes an address into the buffer pointed to by *PP and
 *              then increments the pointer to the first byte after the
 *              address.  An undefined value is stored as all 1's.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Friday, November  7, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
void
H5F_addr_encode(H5F_t *f, uint8_t **pp/*in,out*/, haddr_t addr)
{
    unsigned                i;
    haddr_t                 tmp;

    assert(f);
    assert(pp && *pp);

    if (H5F_addr_defined(addr)) {
        tmp = addr;
        for (i=0; i<H5F_SIZEOF_ADDR(f); i++) {
            *(*pp)++ = (uint8_t)(tmp & 0xff);
            tmp >>= 8;
        }
        assert("overflow" && 0 == tmp);

    } else {
        for (i=0; i<H5F_SIZEOF_ADDR(f); i++) {
            *(*pp)++ = 0xff;
        }
    }
}


/*-------------------------------------------------------------------------
 * Function:    H5F_addr_decode
 *
 * Purpose:     Decodes an address from the buffer pointed to by *PP and
 *              updates the pointer to point to the next byte after the
 *              address.
 *
 *              If the value read is all 1's then the address is returned
 *              with an undefined value.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Friday, November  7, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5F_addr_decode(H5F_t *f, const uint8_t **pp/*in,out*/, haddr_t *addr_p/*out*/)
{
    unsigned                i;
    haddr_t                 tmp;
    uint8_t                 c;
    hbool_t                 all_zero = TRUE;

    assert(f);
    assert(pp && *pp);
    assert(addr_p);

    *addr_p = 0;

    for (i=0; i<H5F_SIZEOF_ADDR(f); i++) {
        c = *(*pp)++;
        if (c != 0xff) all_zero = FALSE;

        if (i<sizeof(*addr_p)) {
            tmp = c;
            tmp <<= i * 8;      /*use tmp to get casting right */
            *addr_p |= tmp;
        } else if (!all_zero) {
            assert(0 == **pp);  /*overflow */
        }
    }
    if (all_zero) *addr_p = HADDR_UNDEF;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_addr_pack
 *
 * Purpose:     Converts a long[2] array (usually returned from
 *              H5G_get_objinfo) back into a haddr_t
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, October  23, 1998
 *
 * Modifications:
 *              Albert Cheng, 1999-02-18
 *              Changed objno to unsigned long type to be consistent with
 *              addr->offset and how it is being called.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_addr_pack(H5F_t UNUSED *f, haddr_t *addr_p/*out*/,
              const unsigned long objno[2])
{
    assert(f);
    assert(objno);
    assert(addr_p);

    *addr_p = objno[0];
#if SIZEOF_LONG<SIZEOF_UINT64_T
    *addr_p |= ((uint64_t)objno[1]) << (8*sizeof(long));
#endif
    
    return(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_debug
 *
 * Purpose:     Prints a file header to the specified stream.  Each line
 *              is indented and the field name occupies the specified width
 *              number of characters.
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
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_debug(H5F_t *f, haddr_t UNUSED addr, FILE * stream, int indent,
          int fwidth)
{
    FUNC_ENTER(H5F_debug, FAIL);

    /* check args */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    /* debug */
    HDfprintf(stream, "%*sFile Boot Block...\n", indent, "");

    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
              "File name:",
              f->name);
    HDfprintf(stream, "%*s%-*s 0x%08x\n", indent, "", fwidth,
              "Flags",
              (unsigned) (f->shared->flags));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Reference count:",
              (unsigned) (f->shared->nrefs));
    HDfprintf(stream, "%*s%-*s 0x%08lx\n", indent, "", fwidth,
              "Consistency flags:",
              (unsigned long) (f->shared->consist_flags));
    HDfprintf(stream, "%*s%-*s %a (abs)\n", indent, "", fwidth,
              "Address of boot block:", f->shared->boot_addr);
    HDfprintf(stream, "%*s%-*s %a (abs)\n", indent, "", fwidth,
              "Base address:", f->shared->base_addr);
    HDfprintf(stream, "%*s%-*s %a (rel)\n", indent, "", fwidth,
              "Free list address:", f->shared->freespace_addr);
    HDfprintf(stream, "%*s%-*s %a (rel)\n", indent, "", fwidth,
              "Driver information block:", f->shared->driver_addr);
    HDfprintf(stream, "%*s%-*s %lu bytes\n", indent, "", fwidth,
              "Size of user block:",
              (unsigned long) (f->shared->fcpl->userblock_size));
    HDfprintf(stream, "%*s%-*s %u bytes\n", indent, "", fwidth,
              "Size of file size_t type:",
              (unsigned) (f->shared->fcpl->sizeof_size));
    HDfprintf(stream, "%*s%-*s %u bytes\n", indent, "", fwidth,
              "Size of file haddr_t type:",
              (unsigned) (f->shared->fcpl->sizeof_addr));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Symbol table leaf node 1/2 rank:",
              (unsigned) (f->shared->fcpl->sym_leaf_k));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Symbol table internal node 1/2 rank:",
              (unsigned) (f->shared->fcpl->btree_k[H5B_SNODE_ID]));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Boot block version number:",
              (unsigned) (f->shared->fcpl->bootblock_ver));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Free list version number:",
              (unsigned) (f->shared->fcpl->freespace_ver));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Object directory version number:",
              (unsigned) (f->shared->fcpl->objectdir_ver));
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Shared header version number:",
              (unsigned) (f->shared->fcpl->sharedheader_ver));

    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
              "Root group symbol table entry:",
              f->shared->root_grp ? "" : "(none)");
    if (f->shared->root_grp) {
        H5G_ent_debug(f, H5G_entof(f->shared->root_grp), stream,
                      indent+3, MAX(0, fwidth-3), HADDR_UNDEF);
    }
    FUNC_LEAVE(SUCCEED);
}
