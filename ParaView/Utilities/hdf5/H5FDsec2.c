/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Thursday, July 29, 1999
 *
 * Purpose:     The POSIX unbuffered file driver using only the HDF5 public
 *              API and with a few optimizations: the lseek() call is made
 *              only when the current file position is unknown or needs to be
 *              changed based on previous I/O through this driver (don't mix
 *              I/O from this driver with I/O from other parts of the
 *              application to the same file).
 */
#include "H5private.h"          /*library functions                     */
#include "H5Eprivate.h"         /*error handling                        */
#include "H5Fprivate.h"         /*files                                 */
#include "H5FDprivate.h"        /*file driver                           */
#include "H5FDsec2.h"           /*sec2 file driver                      */
#include "H5FLprivate.h"        /*free lists                            */
#include "H5MMprivate.h"        /*memory allocation                     */
#include "H5Pprivate.h"         /*property lists                        */

#ifdef MAX
#undef MAX
#define MAX(X,Y)        ((X)>(Y)?(X):(Y))
#endif /* MAX */

/* The driver identification number, initialized at runtime */
static hid_t H5FD_SEC2_g = 0;

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
typedef struct H5FD_sec2_t {
    H5FD_t      pub;                    /*public stuff, must be first   */
    int         fd;                     /*the unix file                 */
    haddr_t     eoa;                    /*end of allocated region       */
    haddr_t     eof;                    /*end of file; current file size*/
    haddr_t     pos;                    /*current file I/O position     */
    int         op;                     /*last operation                */
#ifndef WIN32
    /*
     * On most systems the combination of device and i-node number uniquely
     * identify a file.
     */
    dev_t       device;                 /*file device number            */
    ino_t       inode;                  /*file i-node number            */
#else
    /*
     * On WIN32 the low-order word of a unique identifier associated with the
     * file and the volume serial number uniquely identify a file. This number
     * (which, both? -rpm) may change when the system is restarted or when the
     * file is opened. After a process opens a file, the identifier is
     * constant until the file is closed. An application can use this
     * identifier and the volume serial number to determine whether two
     * handles refer to the same file.
     */
    int fileindexlo;
    int fileindexhi;
#endif
} H5FD_sec2_t;

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
/* pvn: added __MWERKS__ support. */

#ifdef __CYGWIN__
#   define file_offset_t        off_t
#   define file_seek            lseek
#else
#ifdef H5_HAVE_LSEEK64
#   define file_offset_t        off64_t
#   define file_seek            lseek64
#elif defined (WIN32)
# ifdef __MWERKS__
#   define file_offset_t off_t
#   define file_seek lseek
# else /*MSVC*/
#   define file_offset_t __int64
#   define file_seek _lseeki64
# endif
#else
#   define file_offset_t        off_t
#   define file_seek            lseek
#endif
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
static H5FD_t *H5FD_sec2_open(const char *name, unsigned flags, hid_t fapl_id,
                              haddr_t maxaddr);
static herr_t H5FD_sec2_close(H5FD_t *_file);
static int H5FD_sec2_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t H5FD_sec2_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_sec2_get_eoa(H5FD_t *_file);
static herr_t H5FD_sec2_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_sec2_get_eof(H5FD_t *_file);
static herr_t H5FD_sec2_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                             hsize_t size, void *buf);
static herr_t H5FD_sec2_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                              hsize_t size, const void *buf);
static herr_t H5FD_sec2_flush(H5FD_t *_file);

static const H5FD_class_t H5FD_sec2_g = {
    "sec2",                                     /*name                  */
    MAXADDR,                                    /*maxaddr               */
    NULL,                                       /*sb_size               */
    NULL,                                       /*sb_encode             */
    NULL,                                       /*sb_decode             */
    0,                                          /*fapl_size             */
    NULL,                                       /*fapl_get              */
    NULL,                                       /*fapl_copy             */
    NULL,                                       /*fapl_free             */
    0,                                          /*dxpl_size             */
    NULL,                                       /*dxpl_copy             */
    NULL,                                       /*dxpl_free             */
    H5FD_sec2_open,                             /*open                  */
    H5FD_sec2_close,                            /*close                 */
    H5FD_sec2_cmp,                              /*cmp                   */
    H5FD_sec2_query,                            /*query                 */
    NULL,                                       /*alloc                 */
    NULL,                                       /*free                  */
    H5FD_sec2_get_eoa,                          /*get_eoa               */
    H5FD_sec2_set_eoa,                          /*set_eoa               */
    H5FD_sec2_get_eof,                          /*get_eof               */
    H5FD_sec2_read,                             /*read                  */
    H5FD_sec2_write,                            /*write                 */
    H5FD_sec2_flush,                            /*flush                 */
    H5FD_FLMAP_SINGLE,                          /*fl_map                */
};

/* Interface initialization */
#define PABLO_MASK      H5FD_sec2_mask
#define INTERFACE_INIT  H5FD_sec2_init
static int interface_initialize_g = 0;

/* Declare a free list to manage the H5FD_sec2_t struct */
H5FL_DEFINE_STATIC(H5FD_sec2_t);


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:        The driver ID for the sec2 driver.
 *
 *              Failure:        Negative.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_sec2_init(void)
{
    FUNC_ENTER(H5FD_sec2_init, FAIL);

    if (H5I_VFL!=H5Iget_type(H5FD_SEC2_g))
        H5FD_SEC2_g = H5FDregister(&H5FD_sec2_g);

    FUNC_LEAVE(H5FD_SEC2_g);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_sec2
 *
 * Purpose:     Modify the file access property list to use the H5FD_SEC2
 *              driver defined in this source file.  There are no driver
 *              specific properties.
 *              
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 19, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_sec2(hid_t fapl_id)
{
    herr_t ret_value=FAIL;

    FUNC_ENTER(H5Pset_fapl_sec2, FAIL);
    H5TRACE1("e","i",fapl_id);
    
    if (H5P_FILE_ACCESS!=H5Pget_class(fapl_id))
        HRETURN_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a fapl");

    ret_value= H5Pset_driver(fapl_id, H5FD_SEC2, NULL);

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_open
 *
 * Purpose:     Create and/or opens a Unix file as an HDF5 file.
 *
 * Return:      Success:        A pointer to a new file data structure. The
 *                              public fields will be initialized by the
 *                              caller, which is always H5FD_open().
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_sec2_open(const char *name, unsigned flags, hid_t UNUSED fapl_id,
               haddr_t maxaddr)
{
    int o_flags;
    int         fd;
    struct stat sb;
    H5FD_sec2_t *file=NULL;
#ifdef WIN32
        HFILE filehandle;
        struct _BY_HANDLE_FILE_INFORMATION fileinfo;
        int results;   
#endif

    FUNC_ENTER(H5FD_sec2_open, NULL);

    /* Check arguments */
    if (!name || !*name)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name");
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HRETURN_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");
    if (ADDR_OVERFLOW(maxaddr))
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, NULL, "bogus maxaddr");

    /* Build the open flags */
    o_flags = (H5F_ACC_RDWR & flags) ? O_RDWR : O_RDONLY;
    if (H5F_ACC_TRUNC & flags) o_flags |= O_TRUNC;
    if (H5F_ACC_CREAT & flags) o_flags |= O_CREAT;
    if (H5F_ACC_EXCL & flags) o_flags |= O_EXCL;

    /* Open the file */
    if ((fd=HDopen(name, o_flags, 0666))<0)
        HRETURN_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to open file");
    if (fstat(fd, &sb)<0) {
        close(fd);
        HRETURN_ERROR(H5E_FILE, H5E_BADFILE, NULL, "unable to fstat file");
    }

    /* Create the new file struct */
    if (NULL==(file=H5FL_ALLOC(H5FD_sec2_t,1)))
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "unable to allocate file struct");
    file->fd = fd;
    file->eof = sb.st_size;
    file->pos = HADDR_UNDEF;
    file->op = OP_UNKNOWN;
#ifdef WIN32
    filehandle = _get_osfhandle(fd);
    results = GetFileInformationByHandle((HANDLE)filehandle, &fileinfo);
    file->fileindexhi = fileinfo.nFileIndexHigh;
    file->fileindexlo = fileinfo.nFileIndexLow;
#else
    file->device = sb.st_dev;
    file->inode = sb.st_ino;
#endif
    FUNC_LEAVE((H5FD_t*)file);
    fapl_id = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_close
 *
 * Purpose:     Closes a Unix file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, file not closed.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_sec2_close(H5FD_t *_file)
{
    H5FD_sec2_t *file = (H5FD_sec2_t*)_file;

    FUNC_ENTER(H5FD_sec2_close, FAIL);

    if (H5FD_sec2_flush(_file)<0)
        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to flush file");
    if (close(file->fd)<0)
        HRETURN_ERROR(H5E_IO, H5E_CANTCLOSEFILE, FAIL, "unable to close file");

    H5FL_FREE(H5FD_sec2_t,file);

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_cmp
 *
 * Purpose:     Compares two files belonging to this driver using an
 *              arbitrary (but consistent) ordering.
 *
 * Return:      Success:        A value like strcmp()
 *
 *              Failure:        never fails (arguments were checked by the
 *                              caller).
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_sec2_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_sec2_t   *f1 = (const H5FD_sec2_t*)_f1;
    const H5FD_sec2_t   *f2 = (const H5FD_sec2_t*)_f2;
    int ret_value=0;

    FUNC_ENTER(H5FD_sec2_cmp, H5FD_VFD_DEFAULT);

#ifdef WIN32
    if (f1->fileindexhi < f2->fileindexhi) ret_value= -1;
    if (f1->fileindexhi > f2->fileindexhi) ret_value= 1;

    if (f1->fileindexlo < f2->fileindexlo) ret_value= -1;
    if (f1->fileindexlo > f2->fileindexlo) ret_value= 1;

#else
    if (f1->device < f2->device) ret_value= -1;
    if (f1->device > f2->device) ret_value= 1;

    if (f1->inode < f2->inode) ret_value= -1;
    if (f1->inode > f2->inode) ret_value= 1;
#endif

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_query
 *
 * Purpose:     Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:      Success:        non-negative
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
static herr_t
H5FD_sec2_query(const H5FD_t UNUSED *_f, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER(H5FD_sec2_query, FAIL);

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags|=H5FD_FEAT_AGGREGATE_METADATA; /* OK to aggregate metadata allocations */
        *flags|=H5FD_FEAT_ACCUMULATE_METADATA; /* OK to accumulate metadata for faster writes */
        *flags|=H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
    }

    FUNC_LEAVE(ret_value);
    _f = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_get_eoa
 *
 * Purpose:     Gets the end-of-address marker for the file. The EOA marker
 *              is the first address past the last byte allocated in the
 *              format address space.
 *
 * Return:      Success:        The end-of-address marker.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Monday, August  2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_sec2_get_eoa(H5FD_t *_file)
{
    H5FD_sec2_t *file = (H5FD_sec2_t*)_file;

    FUNC_ENTER(H5FD_sec2_get_eoa, HADDR_UNDEF);

    FUNC_LEAVE(file->eoa);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_set_eoa
 *
 * Purpose:     Set the end-of-address marker for the file. This function is
 *              called shortly after an existing HDF5 file is opened in order
 *              to tell the driver where the end of the HDF5 data is located.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_sec2_set_eoa(H5FD_t *_file, haddr_t addr)
{
    H5FD_sec2_t *file = (H5FD_sec2_t*)_file;

    FUNC_ENTER(H5FD_sec2_set_eoa, FAIL);

    file->eoa = addr;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_get_eof
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
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_sec2_get_eof(H5FD_t *_file)
{
    H5FD_sec2_t *file = (H5FD_sec2_t*)_file;

    FUNC_ENTER(H5FD_get_get_eof, HADDR_UNDEF);

    FUNC_LEAVE(MAX(file->eof, file->eoa));
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_read
 *
 * Purpose:     Reads SIZE bytes of data from FILE beginning at address ADDR
 *              into buffer BUF according to data transfer properties in
 *              DXPL_ID.
 *
 * Return:      Success:        Zero. Result is stored in caller-supplied
 *                              buffer BUF.
 *
 *              Failure:        -1, Contents of buffer BUF are undefined.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_sec2_read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr,
               hsize_t size, void *buf/*out*/)
{
    H5FD_sec2_t         *file = (H5FD_sec2_t*)_file;
    ssize_t             nbytes;
    
    FUNC_ENTER(H5FD_sec2_read, FAIL);

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow");
    if (addr+size>file->eoa)
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow");

    /* Seek to the correct location */
    if ((addr!=file->pos || OP_READ!=file->op) &&
            file_seek(file->fd, (file_offset_t)addr, SEEK_SET)<0) {
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
        HRETURN_ERROR(H5E_IO, H5E_SEEKERROR, FAIL,
                      "unable to seek to proper position");
    }

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
    while (size>0) {
        do {
            assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
            nbytes = HDread(file->fd, buf, (size_t)size);
        } while (-1==nbytes && EINTR==errno);
        if (-1==nbytes) {
            /* error */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed");
        }
        if (0==nbytes) {
            /* end of file but not end of format address space */
            assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
            HDmemset(buf, 0, (size_t)size);
            size = 0;
        }
        assert(nbytes>=0);
        assert((hsize_t)nbytes<=size);
        size -= (hsize_t)nbytes;
        addr += (haddr_t)nbytes;
        buf = (char*)buf + nbytes;
    }
    
    /* Update current position */
    file->pos = addr;
    file->op = OP_READ;
    FUNC_LEAVE(SUCCEED);
    type = 0;
    dxpl_id = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_write
 *
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address ADDR
 *              from buffer BUF according to data transfer properties in
 *              DXPL_ID.
 *
 * Return:      Success:        Zero
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_sec2_write(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr,
                hsize_t size, const void *buf)
{
    H5FD_sec2_t         *file = (H5FD_sec2_t*)_file;
    ssize_t             nbytes;
    
    FUNC_ENTER(H5FD_sec2_write, FAIL);

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr) 
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow");
    if (addr+size>file->eoa)
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow");
    
    /* Seek to the correct location */
    if ((addr!=file->pos || OP_WRITE!=file->op) &&
            file_seek(file->fd, (file_offset_t)addr, SEEK_SET)<0) {
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
        HRETURN_ERROR(H5E_IO, H5E_SEEKERROR, FAIL,
                      "unable to seek to proper position");
    }

    /*
     * Write the data, being careful of interrupted system calls and partial
     * results
     */
    while (size>0) {
        do {
            assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
            nbytes = HDwrite(file->fd, buf, (size_t)size);
        } while (-1==nbytes && EINTR==errno);
        if (-1==nbytes) {
            /* error */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed");
        }
        assert(nbytes>0);
        assert((hsize_t)nbytes<=size);
        size -= (hsize_t)nbytes;
        addr += (haddr_t)nbytes;
        buf = (const char*)buf + nbytes;
    }
    
    /* Update current position and eof */
    file->pos = addr;
    file->op = OP_WRITE;
    if (file->pos>file->eof)
        file->eof = file->pos;

    FUNC_LEAVE(SUCCEED);
    type = 0;
    dxpl_id = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_sec2_flush
 *
 * Purpose:     Makes sure that the true file size is the same (or larger)
 *              than the end-of-address.
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
static herr_t
H5FD_sec2_flush(H5FD_t *_file)
{
    H5FD_sec2_t *file = (H5FD_sec2_t*)_file;

    FUNC_ENTER(H5FD_sec2_seek, FAIL);

    if (file->eoa>file->eof) {
        if (-1==file_seek(file->fd, (file_offset_t)(file->eoa-1), SEEK_SET))
            HRETURN_ERROR(H5E_IO, H5E_SEEKERROR, FAIL,
                          "unable to seek to proper position");
        if (write(file->fd, "", 1)!=1)
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed");
        file->eof = file->eoa;
        file->pos = file->eoa;
        file->op = OP_WRITE;
    }

    FUNC_LEAVE(SUCCEED);
}
