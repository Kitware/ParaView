/*
 * Copyright <A9> 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer: Raymond Lu <slu@ncsa.uiuc.edu>
 *             Wednesday, April 12, 2000
 *
 * Purpose:    SRB I/O driver.
 */
#include "H5private.h"          /*library functions                     */
#include "H5Eprivate.h"         /*error handling                        */
#include "H5Fprivate.h"         /*files                                 */
#include "H5FDprivate.h"        /*file driver                           */
#include "H5FDsrb.h"            /*core file driver                      */
#include "H5MMprivate.h"        /*memory allocation                     */
#include "H5Pprivate.h"         /*property lists                        */

/* The driver identification number, initialized at runtime */
static hid_t H5FD_SRB_g = 0;
hid_t GetH5FD_SRB_g()
{ 
  return H5FD_SRB_g;
}

#ifdef H5_HAVE_SRB

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


static H5FD_t *H5FD_srb_open(const char *name, unsigned flags, hid_t fapl_id,
                             haddr_t maxaddr);
static herr_t  H5FD_srb_close(H5FD_t *_file);
static herr_t H5FD_srb_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_srb_get_eoa(H5FD_t *_file);
static herr_t  H5FD_srb_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_srb_get_eof(H5FD_t *_file);
static herr_t  H5FD_srb_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                             hsize_t size, void *buf);
static herr_t  H5FD_srb_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                              hsize_t size, const void *buf);
static herr_t  H5FD_srb_flush(H5FD_t *_file);

/* The description of a file belonging to this driver. */ 
typedef struct H5FD_srb_t {
    H5FD_t      pub;                    /*public stuff, must be first   */
    int         fd;                     /*file descriptor               */
    srbConn     *srb_conn;              /*SRB connection handler        */
    SRB_Info    info;                   /*file information              */
    haddr_t     eoa;                    /*end of allocated region       */
    haddr_t     eof;                    /*end of file; current file size*/
    haddr_t     pos;                    /*current file I/O position     */  
} H5FD_srb_t;

/* SRB-specific file access properties */
typedef struct H5FD_srb_fapl_t {
    srbConn       *srb_conn;            /*SRB connection handler        */
    SRB_Info      info;                 /*file information              */
} H5FD_srb_fapl_t;

/* SRB file driver information */
static const H5FD_class_t H5FD_srb_g = {
    "srb",                                      /*name                  */
    MAXADDR,                                    /*maxaddr               */
    NULL,                                       /*sb_size               */
    NULL,                                       /*sb_encode             */
    NULL,                                       /*sb_decode             */
    sizeof(H5FD_srb_fapl_t),                    /*fapl_size             */
    NULL,                                       /*fapl_get              */
    NULL,                                       /*fapl_copy             */
    NULL,                                       /*fapl_free             */
    0,                                          /*dxpl_size             */
    NULL,                                       /*dxpl_copy             */
    NULL,                                       /*dxpl_free             */
    H5FD_srb_open,                              /*open                  */
    H5FD_srb_close,                             /*close                 */
    NULL,                                       /*cmp                   */
    H5FD_srb_query,                             /*query                 */
    NULL,                                       /*alloc                 */
    NULL,                                       /*free                  */
    H5FD_srb_get_eoa,                           /*get_eoa               */
    H5FD_srb_set_eoa,                           /*set_eoa               */
    H5FD_srb_get_eof,                           /*get_eof               */
    H5FD_srb_read,                              /*read                  */
    H5FD_srb_write,                             /*write                 */
    H5FD_srb_flush,                             /*flush                 */
    H5FD_FLMAP_SINGLE,                          /*fl_map                */
};

/* Interface initialization */
#define PABLO_MASK      H5FD_srb_mask
#define INTERFACE_INIT  H5FD_srb_init
static int interface_initialize_g = 0;

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:        The driver ID for the srb driver.
 *
 *              Failure:        Negative.
 *
 * Programmer:  Raymond Lu
 *              Wednesday, April 12, 2000
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t 
H5FD_srb_init(void)
{
    FUNC_ENTER(H5FD_srb_init, FAIL);

    if(H5I_VFL != H5Iget_type(H5FD_SRB_g))
        H5FD_SRB_g = H5FDregister(&H5FD_srb_g);  

    FUNC_LEAVE(H5FD_SRB_g);
}

/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_srb
 *
 * Purpose:     Store srb connection(client to server) handler SRB_CONN 
 *              after connected and user supplied INFO in the file access 
 *              property list FAPL_ID, which can be used to create or open
 *              file.  
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_srb(hid_t fapl_id, SRB_Info info)
{
    herr_t ret_value = FAIL;
    H5FD_srb_fapl_t fa;
    int srb_status;   

    FUNC_ENTER(H5Pset_fapl_srb, FAIL);

    if(H5P_FILE_ACCESS != H5Pget_class(fapl_id))
        HRETURN_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "Not a fapl");

    /*connect to SRB server */
    fa.srb_conn = clConnect(info.srbHost, info.srbPort, info.srbAuth);
    if((srb_status = clStatus(fa.srb_conn)) != CLI_CONNECTION_OK) {
        fprintf(stderr,"%s",clErrorMessage(fa.srb_conn));
        clFinish(fa.srb_conn);
        /*not sure about first 2 parameters. */
        HRETURN_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, 
                      "Connection to srbMaster failed."); 
    }

    fa.info = info;
    ret_value = H5Pset_driver(fapl_id, H5FD_SRB, &fa);
 
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5Pget_fapl_srb
 *
 * Purpose:     If the file access property list is set to the H5FD_SRB
 *              driver then this function returns the srb file INFO. 
 * 
 * Return:      Success:        File INFO is returned.
 *              Failure:        Negative
 * Programmer:  Raymond Lu
 *              April 12, 2000
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_srb(hid_t fapl_id, SRB_Info *info/*out*/)
{
    H5FD_srb_fapl_t *fa;

    FUNC_ENTER(H5Pget_fapl_srb, FAIL);
    H5TRACE2("e","ix",fapl_id,info);

    if(H5P_FILE_ACCESS != H5Pget_class(fapl_id))
        HRETURN_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a fapl");
    if(H5FD_SRB != H5P_get_driver(fapl_id))
        HRETURN_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");
    if(NULL==(fa=H5Pget_driver_info(fapl_id)))
        HRETURN_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");

    if(info)
        *info = fa->info;

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_open
 *
 * Purpose:     Opens a file with name NAME.  The FLAGS are a bit field with
 *              purpose similar to the second argument of open(2) and which
 *              are defined in H5Fpublic.h. The file access property list
 *              FAPL_ID contains the properties driver properties and MAXADDR
 *              is the largest address which this file will be expected to
 *              access.
 *
 * Return:      Success:        A new file pointer.
 *
 *              Failure:        NULL
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 * Modifications:
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_srb_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    struct srbStat        srb_stat;
    H5FD_srb_fapl_t       *fa=NULL;
    H5FD_srb_fapl_t       _fa;
    H5FD_srb_t            *file;
    int srb_fid;

    FUNC_ENTER(H5FD_srb_open, FAIL);

    /* Check arguments */
    if (!name || !*name)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name");
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HRETURN_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");
    if (ADDR_OVERFLOW(maxaddr))
        HRETURN_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");

    if(H5P_DEFAULT==fapl_id || H5FD_SRB!=H5P_get_driver(fapl_id)) {
        memset((void*)&_fa, 0, sizeof(H5FD_srb_fapl_t));        
        fa = &_fa;
    }
    else {
        fa = H5Pget_driver_info(fapl_id);
        assert(fa);
    }

    /* When I come down here, the possible flag values and the correct
     * responses are given here :-
     *
     * 1. H5F_ACC_CREAT | H5F_ACC_RDWR | H5F_ACC_EXCL : The file is 
     *    a new one. Go ahead and open it with srbObjCreate. 
     * 2. H5F_ACC_CREAT | H5F_ACC_RDWR | H5F_ACC_TRUNC : how do I handle this?
     *    srbObjCreate doesn't support truncate while srbObjOpen doesn't 
     *    support create.  Try to call both srbFileCreate and srbFileOpen!
     * 3. H5F_ACC_RDWR | H5F_ACC_TRUNC : Use O_RDWR | O_TRUNC with srbObjOpen.
     * 4. H5F_ACC_RDWR : Use O_RDWR with srbObjOpen.
     * 5. Flag is not set : Use O_RDONLY with srbObjOpen. 
     *
     * (In srbObjOpen, O_CREATE is not supported.  For file creation, use 
     *  srbObjCreate.)
     */ 

    if((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_RDWR) && 
       (flags & H5F_ACC_EXCL))
        srb_fid = srbFileCreate(fa->srb_conn, fa->info.storSysType, 
            fa->info.srbHost, name, fa->info.mode, fa->info.size);             
    else if((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_RDWR) && 
            (flags & H5F_ACC_TRUNC)) {
        if( (srb_fid = srbFileCreate(fa->srb_conn, fa->info.storSysType, 
            fa->info.srbHost, name, fa->info.mode, fa->info.size)) < 0 ) {
            srb_fid = srbFileOpen(fa->srb_conn, fa->info.storSysType, 
                 fa->info.srbHost, name, O_RDWR|O_TRUNC, fa->info.mode);
        } 
    }
    else if((flags & H5F_ACC_RDWR) && (flags & H5F_ACC_TRUNC))
        srb_fid = srbFileOpen(fa->srb_conn, fa->info.storSysType, 
            fa->info.srbHost, name, O_RDWR|O_TRUNC, fa->info.mode);
    else if(flags & H5F_ACC_RDWR)
        srb_fid = srbFileOpen(fa->srb_conn, fa->info.storSysType, 
            fa->info.srbHost, name, O_RDWR, fa->info.mode);
    else
        srb_fid = srbFileOpen(fa->srb_conn, fa->info.storSysType, 
            fa->info.srbHost, name, O_RDONLY, fa->info.mode);


    if(srb_fid < 0) {
        fprintf(stderr, "cannot open file %s\n", name);
        fprintf(stderr,"%s",clErrorMessage(fa->srb_conn));
        clFinish(fa->srb_conn);       
        HRETURN_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "cannot open file");    
    }

    if(srbFileStat(fa->srb_conn, fa->info.storSysType, fa->info.srbHost, name,
        &srb_stat)!=0) {
        srbFileClose(fa->srb_conn, srb_fid);
        clFinish(fa->srb_conn);    
        HRETURN_ERROR(H5E_IO, H5E_BADFILE, NULL, "SRB file stat failed");
    }

    if (NULL==(file=H5MM_calloc(sizeof(H5FD_srb_t))))
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "can't allocate file struct");    

    file->fd = srb_fid;
    file->eof = srb_stat.st_size;
    file->pos = HADDR_UNDEF;
    file->srb_conn = fa->srb_conn;
    file->info = fa->info;

    FUNC_LEAVE((H5FD_t*)file);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_close
 *
 * Purpose:     Closes a file and srb connection.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Raymond Lu
 * Modification:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_srb_close(H5FD_t *_file)
{
    H5FD_srb_t *file = (H5FD_srb_t *)_file;
    FUNC_ENTER(H5FD_srb_close, FAIL);

    srbFileClose(file->srb_conn, file->fd);
    clFinish(file->srb_conn);

    H5MM_xfree(file);
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_query
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
H5FD_srb_query(const UNUSED H5FD_t *_f, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER(H5FD_srb_query, FAIL);

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags|=H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
    }

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_get_eoa
 *
 * Purpose:     Gets the end-of-address marker for the file. The EOA marker
 *              is the first address past the last byte allocated in the
 *              format address space.
 *
 * Return:      Success:        The end-of-address marker.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_srb_get_eoa(H5FD_t *_file)
{
    H5FD_srb_t *file = (H5FD_srb_t *)_file;
    FUNC_ENTER(H5FD_srb_get_eoa, HADDR_UNDEF);
    FUNC_LEAVE(file->eoa);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_set_eoa
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
 *              Friday, August 6, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_srb_set_eoa(H5FD_t *_file, haddr_t addr)
{
    H5FD_srb_t *file = (H5FD_srb_t *)_file;
    FUNC_ENTER(H5FD_srb_set_eoa, FAIL);
    file->eoa = addr;
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_get_eof
 *
 * Purpose:     Gets the end-of-file marker for the file. The EOF marker
 *              is the real size of the file.
 *
 * Return:      Success:        The end-of-address marker.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_srb_get_eof(H5FD_t *_file)
{
    H5FD_srb_t *file = (H5FD_srb_t *)_file;
    FUNC_ENTER(H5FD_srb_get_eof, HADDR_UNDEF);
    FUNC_LEAVE(file->eof); 
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_read
 *
 * Purpose:     Reads SIZE bytes of data from FILE beginning at address ADDR
 *              into buffer BUF.
 *
 * Return:      Success:        Zero. Result is stored in caller-supplied
 *                              buffer BUF.
 *
 *              Failure:        -1, Contents of buffer BUF are undefined.
 *
 * Programmer:  Raymond Lu
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_srb_read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr, 
              hsize_t size, void *buf)
{
    H5FD_srb_t *file = (H5FD_srb_t*)_file;
    ssize_t    nbytes;

    FUNC_ENTER(H5FD_srb_read, FAIL);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HRETURN_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr+size>file->eoa)
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    if( addr!=file->pos &&
        srbFileSeek(file->srb_conn, (int)file->fd, (int)addr, SEEK_SET)<0 ) {
        file->pos = HADDR_UNDEF;
        srbFileClose(file->srb_conn, file->fd);
        clFinish(file->srb_conn);    
        HRETURN_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "srb file seek failed");
    }

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
    while(size>0) {
        if((nbytes=srbFileRead(file->srb_conn, (int)file->fd, (char*)buf,
                             (int)size))<0) {
            file->pos = HADDR_UNDEF;
            srbFileClose(file->srb_conn, file->fd);
            clFinish(file->srb_conn);    
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, 
                          "srb file write failed");
        }

        if (0==nbytes) {
            /*end of file but not end of format address space*/
            memset(buf, 0, size);
            size = 0;
        }
        size -= (hsize_t)nbytes;
        addr += (haddr_t)nbytes;
        buf = (char*)buf + nbytes;
    }

    /* Update current position */
    file->pos = addr;
  
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_write
 *
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address ADDR
 *              from buffer BUF.
 *
 * Return:      Success:        Zero.
 *
 *              Failure:        -1
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_srb_write(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr, 
               hsize_t size, const void *buf)
{
    H5FD_srb_t *file = (H5FD_srb_t*)_file;
    ssize_t    nbytes;

    FUNC_ENTER(H5FD_srb_write, FAIL);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HRETURN_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr+size>file->eoa)
        HRETURN_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    if( addr!=file->pos &&
        srbFileSeek(file->srb_conn, (int)file->fd, (int)addr, SEEK_SET)<0 ) {
        file->pos = HADDR_UNDEF;
        srbObjClose(file->srb_conn, file->fd);
        clFinish(file->srb_conn);    
        HRETURN_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "srb file seek failed");
    }

    while(size>0) {
        if( (nbytes=srbFileWrite(file->srb_conn, (int)file->fd, (char*)buf, 
                                (int)size)) < 0 ) {
            file->pos = HADDR_UNDEF;
            srbObjClose(file->srb_conn, file->fd);
            clFinish(file->srb_conn);    
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, 
                          "srb file write failed");
        }
      
        size -= (hsize_t)nbytes; 
        addr += (haddr_t)nbytes;
        buf  =  (const char*)buf + nbytes;
    }

    /* Update current position and eof */
    file->pos = addr;
    if(file->pos > file->eof)
         file->eof = file->pos;

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_srb_flush
 *
 * Purpose:     Makes sure that all data is on disk.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_srb_flush(H5FD_t *_file)
{
    H5FD_srb_t *file = (H5FD_srb_t*)_file;

    /*why H5FD_family_flush?*/ 
    FUNC_ENTER(H5FD_family_flush, FAIL);
    if(srbFileSync(file->srb_conn, file->fd) != 0) {
        srbFileClose(file->srb_conn, file->fd);
        clFinish(file->srb_conn);    
        HRETURN_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "srb file sync failed");
    }

    FUNC_LEAVE(SUCCEED);
}

#endif  /* H5_HAVE_SRB */
