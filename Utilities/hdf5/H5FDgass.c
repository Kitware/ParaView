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
 * Programmer:  Saurabh Bagchi (bagchi@uiuc.edu)
 *              Thursday, August 12 -Tuesday, August 17, 1999
 *
 * Purpose:     This is the GASS I/O driver.
 *
 */
#include "H5private.h"          /*library functions                     */
#include "H5Eprivate.h"         /*error handling                        */
#include "H5Fprivate.h"         /*files                                 */
#include "H5FDprivate.h"        /*file driver                             */
#include "H5FDgass.h"           /* Core file driver */
#include "H5Iprivate.h"         /*object IDs                              */
#include "H5MMprivate.h"        /* Memory allocation */
#include "H5Pprivate.h"         /*property lists                        */

#ifdef H5_HAVE_GASS

/* The driver identification number, initialized at runtime */
static hid_t H5FD_GASS_g = 0;

/* File operations */
#define OP_UNKNOWN      0
#define OP_READ         1
#define OP_WRITE        2

/*
 * The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying Unix file). The `pos'
 * value is used to eliminate file position updates when they would be a
 * no-op. Unfortunately we've found systems that use separate file position
 * indicators for reading and writing so the lseek can only be eliminated if
 * the current operation is the same as the previous operation.  When opening
 * a file the `eof' will be set to the current file size, `eoa' will be set
 * to zero, `pos' will be set to H5F_ADDR_UNDEF (as it is when an error
 * occurs), and `op' will be set to H5F_OP_UNKNOWN.
 */
typedef struct H5FD_gass_t {
    H5FD_t      pub;                    /*public stuff, must be first   */
    int         fd;                     /*the unix file                 */
    GASS_Info   info;                   /*file information */
    haddr_t     eoa;                    /*end of allocated region       */
    haddr_t     eof;                    /*end of file; current file size*/
    haddr_t     pos;                    /*current file I/O position     */
    int         op;                     /*last operation                */
    
  
} H5FD_gass_t;

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
/* adding for windows NT file system support. */

#ifdef H5_HAVE_LSEEK64
#   define file_offset_t        off64_t
#   define file_seek            lseek64
#elif defined (WIN32) && !defined(__CYGWIN__)
#   ifdef __MWERKS__
#       define file_offset_t off_t
#       define file_seek lseek
#   else /*MSVC*/
#       define file_offset_t __int64
#       define file_seek _lseeki64
#   endif
#else
#   define file_offset_t        off_t
#   define file_seek            lseek
#endif



/*
 * These macros check for overflow of various quantities.  These macros
 * assume that file_offset_t is signed and haddr_t and size_t are unsigned.
 * 
 * ADDR_OVERFLOW:       Checks whether a file address of type `haddr_t'
 *                      is too large to be represented by the second argument
 *                      of the file seek function.
 *
 * SIZE_OVERFLOW:       Checks whether a buffer size of type `hsize_t' is too
 *                      large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW:     Checks whether an address and size pair describe data
 *                      which can be addressed entirely by the second
 *                      argument of the file seek function.
 */
#define MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define ADDR_OVERFLOW(A)        (HADDR_UNDEF==(A) ||                          \
                                 ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)        ((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)    (ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
                                 sizeof(file_offset_t)<sizeof(size_t) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||                      \
                                 (file_offset_t)((A)+(Z))<(file_offset_t)(A))

/* Prototypes */
static H5FD_t *H5FD_gass_open(const char *name, unsigned flags, hid_t fapl_id,
                              haddr_t maxaddr);
static herr_t H5FD_gass_close(H5FD_t *_file);
static herr_t H5FD_gass_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_gass_get_eoa(H5FD_t *_file);
static herr_t H5FD_gass_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_gass_get_eof(H5FD_t *_file);
static herr_t  H5FD_gass_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_gass_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                             size_t size, void *buf);
static herr_t H5FD_gass_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                              size_t size, const void *buf);

/* GASS I/O-specific file access properties */
typedef struct H5FD_gass_fapl_t {
  GASS_Info            info;      /* access property parameters */
} H5FD_gass_fapl_t;

/* The GASS IO driver information */
static const H5FD_class_t H5FD_gass_g = {
    "gass",                                     /*name                  */
    MAXADDR,                                    /*maxaddr               */
    H5F_CLOSE_WEAK,                             /* fc_degree            */
    NULL,                                       /*sb_size               */
    NULL,                                       /*sb_encode             */
    NULL,                                       /*sb_decode             */
    sizeof(H5FD_gass_fapl_t),                   /*fapl_size             */
    NULL,                                       /*fapl_get              */
    NULL,                                       /*fapl_copy             */
    NULL,                                       /*fapl_free             */
    0,                                          /*dxpl_size             */
    NULL,                                       /*dxpl_copy             */
    NULL,                                       /*dxpl_free             */
    H5FD_gass_open,                             /*open                  */
    H5FD_gass_close,                            /*close                 */
    NULL,                                       /*cmp                   */
    H5FD_gass_query,                            /*query                 */
    NULL,                                       /*alloc                 */
    NULL,                                       /*free                  */
    H5FD_gass_get_eoa,                          /*get_eoa               */
    H5FD_gass_set_eoa,                          /*set_eoa               */
    H5FD_gass_get_eof,                          /*get_eof               */
    H5FD_gass_get_handle,                       /*get_handle            */
    H5FD_gass_read,                             /*read                  */
    H5FD_gass_write,                            /*write                 */
    NULL,                                       /*flush                 */
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE                           /*fl_map                */
};

/* Interface initialization */
#define PABLO_MASK      H5FD_gass_mask
#define INTERFACE_INIT  H5FD_gass_init
static int interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:        The driver ID for the gass driver.
 *
 *              Failure:        Negative.
 *
 * Programmer:  Saurabh Bagchi
 *              Friday, August 13, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_gass_init(void)
{
    hid_t ret_value=H5FD_GASS_g;        /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_init, FAIL);

    if (!H5FD_GASS_g)
        H5FD_GASS_g = H5FDregister(&H5FD_gass_g);

    globus_module_activate (GLOBUS_COMMON_MODULE);
    globus_module_activate (GLOBUS_GASS_FILE_MODULE);

    /* Set return value */
    ret_value=H5FD_GASS_g;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_gass
 *
 * Purpose:     Store the user supplied GASS INFO in
 *              the file access property list FAPL_ID which can then be used
 *              to create and/or open the file. 
 *
 *              GASS info object to be used for file open using GASS.
 *              Any modification to info after
 *              this function call returns may have undetermined effect
 *              to the access property list.  Users should call this
 *              function again to setup the property list.
 *
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Saurabh Bagchi
 *              Friday, August 13, 1999
 *
 * Modifications:
 *
 *              Raymond Lu, 2001-10-25
 *              Changed the file access list to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_gass(hid_t fapl_id, GASS_Info info)
{
    H5FD_gass_fapl_t    fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value;
    
    FUNC_ENTER_API(H5Pset_fapl_gass, FAIL);
    /*NO TRACE*/
    
    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");   

#ifdef LATER
int We_need_to_verify_that_INFO_contain_sensible_information;
#endif

    /* Initialize driver specific properties */
    fa.info = info;

    ret_value= H5P_set_driver(plist, H5FD_GASS, &fa);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_fapl_gass
 *
 * Purpose:     If the file access property list is set to the H5FD_GASS
 *              driver then this function returns the GASS info object
 *              through the INFO pointer.
 *
 * Return:      Success:        Non-negative with the info object returned 
 *                              through the INFO arguments if non-null. 
 *                              The information is copied and it is therefore
 *                              valid only until the file access property
 *                              list is modified or closed.
 *
 *              Failure:        Negative
 *
 * Programmer:  Saurabh Bagchi
 *              Friday, August 13, 1999
 *
 * Modifications:
 *
 *              Raymond Lu, 2001-10-25
 *              Changed the file access list to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_gass(hid_t fapl_id, GASS_Info *info/*out*/)
{
    H5FD_gass_fapl_t    *fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_API(H5Pget_fapl_gass, FAIL);
    H5TRACE2("e","ix",fapl_id,info);

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    if (H5FD_GASS!=H5P_get_driver(plist))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");
    if (NULL==(fa=H5P_get_driver_info(plist)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");

    if (info)
        *info = fa->info;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_open
 *
 * Purpose:     Opens a file with name NAME.  The FLAGS are a bit field with
 *              purpose similar to the second argument of open(2) and which
 *              are defined in H5Fpublic.h. The file access property list
 *              FAPL_ID contains the driver properties and MAXADDR
 *              is the largest address which this file will be expected to
 *              access.
 *
 * Return:      Success:        A pointer to a new file data structure. The
 *                              public fields will be initialized by the
 *                              caller, which is always H5FD_open().
 *
 *              Failure:        NULL
 *
 * Programmer:  Saurabh Bagchi
 *              Friday, August 13, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_gass_open(const char *name, unsigned flags, hid_t fapl_id,
               haddr_t maxaddr)
{
    int         fd;
    H5FD_gass_t *file=NULL;
    const H5FD_gass_fapl_t      *fa=NULL;
    H5FD_gass_fapl_t            _fa;
    char *filename = (char *) H5MM_malloc(80 * sizeof(char));
    H5P_genplist_t *plist;      /* Property list pointer */
    h5_stat_t sb;
    H5FD_t      *ret_value;
    
    FUNC_ENTER_NOAPI(H5FD_gass_open, NULL);

    /* fprintf(stdout, "Entering H5FD_gass_open name=%s flags=0x%x\n", name, flags); */

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name");
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");
    if (ADDR_OVERFLOW(maxaddr))
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");

    strcpy (filename, name);
    
    /* Obtain a pointer to gass-specific file access properties */
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
    if (H5P_FILE_ACCESS_DEFAULT==fapl_id || H5FD_GASS!=H5P_get_driver(plist)) {
        GASS_INFO_NULL (_fa.info);
        /* _fa.info = GASS_INFO_NULL; */
        /* _fa.info = {0,0}; */ /*default*/
        fa = &_fa;
    } else {
        fa = H5P_get_driver_info(plist);
        assert(fa);
    }
    
    /* When I come down here, the possible flag values and the correct
       responses are given here :-

       1. H5F_ACC_CREAT | H5F_ACC_RDWR | H5F_ACC_EXCL : The file is 
          a new one. Go ahead and open it in O_RDWR. 

       2. H5F_ACC_CREAT | H5F_ACC_RDWR | H5F_ACC_TRUNC : Use
          O_RDWR | O_TRUNC with gass_open.

       3. H5F_ACC_RDWR | H5F_ACC_TRUNC : File already exists. Truncate it.
          
       4. H5F_ACC_RDWR : Use O_RDWR with gass_open

       5. H5F_ACC_RDWR is not set : Use O_RDONLY with gass_open

       One reason why we cannot simply pass on the flags to gass_open
       is that gass_open does not support many of them (e.g., O_CREAT)
    */

    
    if ((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_RDWR) && (flags & H5F_ACC_EXCL)) {
        if ((fd = globus_gass_open (filename, O_RDWR|O_TRUNC)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTOPENFILE, NULL, "open failed");
    }
    else if ((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_RDWR) && (flags & H5F_ACC_TRUNC)) {
        if ((fd = globus_gass_open (filename, O_RDWR|O_TRUNC)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTOPENFILE, NULL, "open failed");
      
    }
    else if ((flags & H5F_ACC_RDWR) && (flags & H5F_ACC_TRUNC)) {
        if ((fd = globus_gass_open (filename, O_RDWR|O_TRUNC)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTOPENFILE, NULL, "open failed");
    
    }
    else if (flags & H5F_ACC_RDWR) {
        if ((fd = globus_gass_open (filename, O_RDWR)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTOPENFILE, NULL, "open failed");
      
    }
    else { /* This is case where H5F_ACC_RDWR is not set */
        if ((fd = globus_gass_open (filename, O_RDONLY)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTOPENFILE, NULL, "open failed");
      
    }
   
    if (HDfstat(fd, &sb)<0) {
        close(fd);
        HGOTO_ERROR(H5E_IO, H5E_BADFILE, NULL, "fstat failed");
    }

    /* Create the new file struct */
    if (NULL==(file=H5MM_calloc(sizeof(H5FD_gass_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "can't allocate file struct");
    file->fd = fd;
    file->eof = sb.st_size;
    file->pos = HADDR_UNDEF;
    file->op = OP_UNKNOWN;
    file->info = fa->info;

    /* Set return value */
    ret_value=(H5FD_t*)file;
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_close
 *
 * Purpose:     Closes a GASS file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, file not closed.
 *
 * Programmer:  Saurabh Bagchi
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_gass_close (H5FD_t *_file)
{
    H5FD_gass_t *file = (H5FD_gass_t *)_file;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_close, FAIL);

    if (file == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file handle");
    
    if (globus_gass_close (file->fd) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTCLOSEFILE, FAIL, "can't close GASS file");

    H5MM_xfree(file);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_query
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
H5FD_gass_query(const UNUSED H5FD_t *_f, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_gass_query, FAIL);

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags|=H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
        *flags|=H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_get_eoa
 *
 * Purpose:     Gets the end-of-address marker for the file. The EOA marker
 *              is the first address past the last byte allocated in the
 *              format address space.
 *
 * Return:      Success:        The end-of-address marker.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Saurabh Bagchi
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_gass_get_eoa(H5FD_t *_file)
{
    H5FD_gass_t *file = (H5FD_gass_t *)_file;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_get_eoa, HADDR_UNDEF);

    /* Set return value */
    ret_value=file->eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_set_eoa
 *
 * Purpose:     Set the end-of-address marker for the file. This function is
 *              called shortly after an existing HDF5 file is opened in order
 *              to tell the driver where the end of the HDF5 data is located.
 *
 * Return:      Success:        0
 *
 * Programmer:  Saurabh Bagchi
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_gass_set_eoa(H5FD_t *_file, haddr_t addr)
{
    H5FD_gass_t *file = (H5FD_gass_t *)_file;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_set_eoa, FAIL);

    file->eoa = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_get_eof
 *
 * Purpose:     Returns the end-of-file marker, which is the greater of
 *              either the Unix end-of-file or the HDF5 end-of-address
 *              markers.
 *
 * Return:      Success:        End of file address, the first address past
 *                              the end of the "file", either the Unix file
 *                              or the HDF5 file.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Saurabh Bagchi
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_gass_get_eof(H5FD_t *_file)
{
    H5FD_gass_t *file = (H5FD_gass_t*)_file;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_get_eof, HADDR_UNDEF);

    /* Set return value */
    ret_value=MAX(file->eof, file->eoa);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:       H5FD_gass_get_handle
 * 
 * Purpose:        Returns the file handle of GASS file driver.
 * 
 * Returns:        Non-negative if succeed or negative if fails.
 * 
 * Programmer:     Raymond Lu
 *                 Sept. 16, 2002
 *                 
 * Modifications:  
 * 
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_gass_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{   
    H5FD_gass_t         *file = (H5FD_gass_t *)_file;
    herr_t              ret_value = SUCCEED;
                                                   
    FUNC_ENTER_NOAPI(H5FD_gass_get_handle, FAIL);

    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid");

    *file_handle = &(file->fd);

done:   
    FUNC_LEAVE_NOAPI(ret_value);
}   
            

/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_read
 *
 * Purpose:     Reads SIZE bytes of data from FILE beginning at address ADDR
 *              into buffer BUF.
 *
 * Return:      Success:        Zero. Result is stored in caller-supplied
 *                              buffer BUF.
 *
 *              Failure:        -1, Contents of buffer BUF are undefined.
 *
 * Programmer:  Saurabh Bagchi
 *              Monday, August 16, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_gass_read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t dxpl_id/*unused*/, haddr_t addr,
               size_t size, void *buf/*out*/)
{
    H5FD_gass_t         *file = (H5FD_gass_t*)_file;
    ssize_t             nbytes;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_read, FAIL);

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr+size>file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    /* Seek to the correct location */
    if ((addr!=file->pos || OP_READ!=file->op) &&
            file_seek(file->fd, (file_offset_t)addr, SEEK_SET)<0) {
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
        HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "gass file seek failed");
    }

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
    while (size>0) {
        do 
            nbytes = read(file->fd, buf, size);
        while (-1==nbytes && EINTR==errno);

        if (-1==nbytes) {
            /* error */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "gass file read failed");
        }
        if (0==nbytes) {
            /* end of file but not end of format address space */
            memset(buf, 0, size);
            size = 0;
        }
        assert(nbytes>=0);
        assert(nbytes<=size);
        size -= (hsize_t)nbytes;
        addr += (haddr_t)nbytes;
        buf = (char*)buf + nbytes;
    }

    /* Update current position */
    file->pos = addr;
    file->op = OP_READ;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_gass_write
 *
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address ADDR
 *              from buffer BUF.
 *
 * Return:      Success:        Zero
 *
 *              Failure:        -1
 *
 * Programmer:  Saurabh Bagchi
 *              Tuesday, August 17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_gass_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id/*unused*/, haddr_t addr,
                size_t size, const void *buf)
{
    H5FD_gass_t         *file = (H5FD_gass_t*)_file;
    ssize_t             nbytes;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_gass_write, FAIL);

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr+size>file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    /* Seek to the correct location */
    if ((addr!=file->pos || OP_WRITE!=file->op) &&
            file_seek(file->fd, (file_offset_t)addr, SEEK_SET)<0) {
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
        HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "gass file seek failed");
    }

    /*
     * Write the data, being careful of interrupted system calls and partial
     * results
     */
    while (size>0) {
        do
            nbytes = write(file->fd, buf, size);
        while (-1==nbytes && EINTR==errno);

        if (-1==nbytes) {
            /* error */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "gass file write failed");
        }
        assert(nbytes>0);
        assert(nbytes<=size);
        size -= (hsize_t)nbytes;
        addr += (haddr_t)nbytes;
        buf = (const char*)buf + nbytes;
    }

    /* Update current position and eof */
    file->pos = addr;
    file->op = OP_WRITE;
    if (file->pos>file->eof)
        file->eof = file->pos;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

#endif  /* H5_HAVE_GASS */
