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
 * Programmer: Raymond Lu <slu@ncsa.uiuc.edu>
 *             Wednesday, April 12, 2000
 *
 * Purpose:    SRB I/O driver.
 */
#include "H5private.h"		/*library functions			*/
#include "H5Eprivate.h"		/*error handling			*/
#include "H5Fprivate.h"		/*files					*/
#include "H5FDprivate.h"	/*file driver				  */
#include "H5FDsrb.h"            /* Core file driver                     */
#include "H5Iprivate.h"		/*object IDs				  */
#include "H5MMprivate.h"        /* Memory allocation                    */
#include "H5Pprivate.h"		/*property lists			*/


#ifdef H5_HAVE_SRB

/* The driver identification number, initialized at runtime */
static hid_t H5FD_SRB_g = 0;

/*
 * This driver supports systems that have the lseek64() function by defining
 * some macros here so we don't have to have conditional compilations later
 * throughout the code.
 *
 * file_offset_t:	The datatype for file offsets, the second argument of
 *			the lseek() or lseek64() call.
 *
 * file_seek:		The function which adjusts the current file position,
 *			either lseek() or lseek64().
 */
/* adding for windows NT file system support. */

#ifdef H5_HAVE_LSEEK64
#   define file_offset_t	off64_t
#   define file_seek		lseek64
#elif defined (WIN32) && !defined(__CYGWIN__)
#   ifdef __MWERKS__
#       define file_offset_t off_t
#       define file_seek lseek
#   else /*MSVC*/
#       define file_offset_t __int64
#       define file_seek _lseeki64
#   endif
#else
#   define file_offset_t	off_t
#   define file_seek		lseek
#endif


/*
 * These macros check for overflow of various quantities.  These macros
 * assume that file_offset_t is signed and haddr_t and size_t are unsigned.
 * 
 * ADDR_OVERFLOW:	Checks whether a file address of type `haddr_t'
 *			is too large to be represented by the second argument
 *			of the file seek function.
 *
 * SIZE_OVERFLOW:	Checks whether a buffer size of type `hsize_t' is too
 *			large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW:	Checks whether an address and size pair describe data
 *			which can be addressed entirely by the second
 *			argument of the file seek function.
 */
#define MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define ADDR_OVERFLOW(A)	(HADDR_UNDEF==(A) ||			      \
				 ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)	((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)	(ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
				 sizeof(file_offset_t)<sizeof(size_t) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||		      \
				 (file_offset_t)((A)+(Z))<(file_offset_t)(A))


static H5FD_t *H5FD_srb_open(const char *name, unsigned flags, hid_t fapl_id,
	       	             haddr_t maxaddr);
static herr_t  H5FD_srb_close(H5FD_t *_file);
static herr_t H5FD_srb_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_srb_get_eoa(H5FD_t *_file);
static herr_t  H5FD_srb_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_srb_get_eof(H5FD_t *_file);
static herr_t  H5FD_srb_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t  H5FD_srb_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
			     size_t size, void *buf);
static herr_t  H5FD_srb_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
			      size_t size, const void *buf);
static herr_t  H5FD_srb_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);

/* The description of a file belonging to this driver. */ 
typedef struct H5FD_srb_t {
    H5FD_t	pub;			/*public stuff, must be first	*/
    int		fd;			/*file descriptor    		*/
    srbConn     *srb_conn;              /*SRB connection handler        */
    SRB_Info    info;                   /*file information              */
    haddr_t	eoa;			/*end of allocated region	*/
    haddr_t	eof;			/*end of file; current file size*/
    haddr_t	pos;			/*current file I/O position	*/  
} H5FD_srb_t;

/* SRB-specific file access properties */
typedef struct H5FD_srb_fapl_t {
    srbConn       *srb_conn;            /*SRB connection handler        */
    SRB_Info      info;                 /*file information              */
} H5FD_srb_fapl_t;

/* SRB file driver information */
static const H5FD_class_t H5FD_srb_g = {
    "srb",					/*name			*/
    MAXADDR,					/*maxaddr		*/
    H5F_CLOSE_WEAK,				/* fc_degree		*/
    NULL,					/*sb_size		*/
    NULL,					/*sb_encode		*/
    NULL,					/*sb_decode		*/
    sizeof(H5FD_srb_fapl_t), 			/*fapl_size		*/
    NULL,					/*fapl_get		*/
    NULL,					/*fapl_copy		*/
    NULL, 					/*fapl_free		*/
    0,						/*dxpl_size		*/
    NULL,					/*dxpl_copy		*/
    NULL,					/*dxpl_free		*/
    H5FD_srb_open,         	 		/*open			*/
    H5FD_srb_close,		        	/*close			*/
    NULL,				        /*cmp			*/
    H5FD_srb_query,				/*query			*/
    NULL,					/*alloc			*/
    NULL,					/*free			*/
    H5FD_srb_get_eoa,           		/*get_eoa		*/
    H5FD_srb_set_eoa, 		                /*set_eoa		*/
    H5FD_srb_get_eof,				/*get_eof		*/
    H5FD_srb_get_handle,                        /*get_handle            */
    H5FD_srb_read,				/*read			*/
    H5FD_srb_write,				/*write			*/
    H5FD_srb_flush,				/*flush			*/
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE 				/*fl_map		*/
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
    hid_t ret_value=H5FD_SRB_g; /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_init, FAIL);

    if(H5I_VFL != H5Iget_type(H5FD_SRB_g))
        H5FD_SRB_g = H5FDregister(&H5FD_srb_g);  

    /* Set return value */
    ret_value=H5FD_SRB_g;

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
 *
 * Modifications:
 *
 *		Raymond Lu, 2001-10-25
 *		Use the new generic property list for argument checking.
 *	
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_srb(hid_t fapl_id, SRB_Info info)
{
    H5FD_srb_fapl_t fa;
    int srb_status;   
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value;

    FUNC_ENTER_API(H5Pset_fapl_srb, FAIL);
    /*NO TRACE*/

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");

    /*connect to SRB server */
    fa.srb_conn = clConnect(info.srbHost, info.srbPort, info.srbAuth);
    if((srb_status = clStatus(fa.srb_conn)) != CLI_CONNECTION_OK) {
        fprintf(stderr,"%s",clErrorMessage(fa.srb_conn));
        clFinish(fa.srb_conn);

        /*not sure about first 2 parameters. */
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "Connection to srbMaster failed."); 
    }

    fa.info = info;
    ret_value = H5P_set_driver(plist, H5FD_SRB, &fa);
 
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_fapl_srb
 *
 * Purpose:     If the file access property list is set to the H5FD_SRB
 *              driver then this function returns the srb file INFO. 
 * 
 * Return:      Success:        File INFO is returned.
 *              Failure:        Negative
 *
 * Programmer:  Raymond Lu
 *              April 12, 2000
 *
 * Modifications:
 *
 *		Raymond Lu, 2001-10-25
 *		Use the new generic property list for checking property list
 *		ID.
 *	
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_srb(hid_t fapl_id, SRB_Info *info/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5FD_srb_fapl_t *fa;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pget_fapl_srb, FAIL);
    H5TRACE2("e","ix",fapl_id,info);

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    if(H5FD_SRB != H5P_get_driver(plist))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");
    if(NULL==(fa=H5P_get_driver_info(plist)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");

    if(info)
        *info = fa->info;

done:
    FUNC_LEAVE_API(ret_value);
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
    H5P_genplist_t *plist;      /* Property list pointer */
    H5FD_t            *ret_value;

    FUNC_ENTER_NOAPI(H5FD_srb_open, FAIL);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name");
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");
    if (ADDR_OVERFLOW(maxaddr))
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
    if(H5P_FILE_ACCESS_DEFAULT==fapl_id || H5FD_SRB!=H5P_get_driver(plist)) {
        HDmemset((void*)&_fa, 0, sizeof(H5FD_srb_fapl_t));        
        fa = &_fa;
    }
    else {
        fa = H5P_get_driver_info(plist);
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
#ifdef OLD_WAY
        fprintf(stderr, "cannot open file %s\n", name);
        fprintf(stderr,"%s",clErrorMessage(fa->srb_conn));
        clFinish(fa->srb_conn);       
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, NULL, "cannot open file");    
#else /* OLD_WAY */
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, NULL, clErrorMessage(fa->srb_conn));
#endif /* OLD_WAY */
    }

    if(srbFileStat(fa->srb_conn, fa->info.storSysType, fa->info.srbHost, name,
            &srb_stat)!=0) {
#ifdef OLD_WAY
        srbFileClose(fa->srb_conn, srb_fid);
        clFinish(fa->srb_conn);    
        HGOTO_ERROR(H5E_IO, H5E_BADFILE, NULL, "SRB file stat failed");
#else /* OLD_WAY */
        HGOTO_ERROR(H5E_IO, H5E_BADFILE, NULL, "SRB file stat failed");
#endif /* OLD_WAY */
    }

    if (NULL==(file=H5MM_calloc(sizeof(H5FD_srb_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "can't allocate file struct");    

    file->fd = srb_fid;
    file->eof = srb_stat.st_size;
    file->pos = HADDR_UNDEF;
    file->srb_conn = fa->srb_conn;
    file->info = fa->info;

    /* Set return value */
    ret_value=(H5FD_t*)file;

done:
    if(ret_value==NULL) {
        if(fa!=NULL)
            clFinish(fa->srb_conn);       
        if(srb_fid>=0)
            srbFileClose(fa->srb_conn, srb_fid);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_close, FAIL);

    srbFileClose(file->srb_conn, file->fd);
    clFinish(file->srb_conn);

    H5MM_xfree(file);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_srb_query
 *
 * Purpose:	Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, September 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_srb_query(const UNUSED H5FD_t *_f, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_query, FAIL);

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
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_get_eoa, HADDR_UNDEF);

    /* Set return value */
    ret_value=file->eoa; 

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_set_eoa, FAIL);

    file->eoa = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_get_eof, HADDR_UNDEF);

    /* Set return value */
    ret_value=file->eof; 

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:       H5FD_srb_get_handle
 * 
 * Purpose:        Returns the file handle of SRB file driver.
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
H5FD_srb_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{   
    H5FD_srb_t          *file = (H5FD_srb_t *)_file;
    herr_t              ret_value = SUCCEED;
                            
    FUNC_ENTER_NOAPI(H5FD_srb_get_eof, FAIL);

    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid");

    *file_handle = &(file->fd);

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
              size_t size, void *buf)
{
    H5FD_srb_t *file = (H5FD_srb_t*)_file;
    ssize_t    nbytes;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_read, FAIL);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr+size>file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    if( addr!=file->pos &&
            srbFileSeek(file->srb_conn, (int)file->fd, (int)addr, SEEK_SET)<0 )
        HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "srb file seek failed");

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
    while(size>0) {
        if((nbytes=srbFileRead(file->srb_conn, (int)file->fd, (char*)buf, size))<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "srb file write failed");

        if (0==nbytes) {
            /*end of file but not end of format address space*/
            HDmemset(buf, 0, size);
            size = 0;
        }
        size -= nbytes;
        addr += (haddr_t)nbytes;
        buf = (char*)buf + nbytes;
    }

    /* Update current position */
    file->pos = addr;
  
done:
    if(ret_value<0) {
        /* Reset file position */
        file->pos = HADDR_UNDEF;

        /* Close connection, etc. */
        srbFileClose(file->srb_conn, file->fd);
        clFinish(file->srb_conn);    
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
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
               size_t size, const void *buf)
{
    H5FD_srb_t *file = (H5FD_srb_t*)_file;
    ssize_t    nbytes;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_write, FAIL);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "addr undefined");
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");
    if (addr+size>file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr too large");

    if( addr!=file->pos &&
            srbFileSeek(file->srb_conn, (int)file->fd, (int)addr, SEEK_SET)<0 )
        HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "srb file seek failed");

    while(size>0) {
        if( (nbytes=srbFileWrite(file->srb_conn, (int)file->fd, (char*)buf, size)) < 0 )
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "srb file write failed");
      
        size -= nbytes; 
        addr += (haddr_t)nbytes;
        buf  =  (const char*)buf + nbytes;
    }

    /* Update current position and eof */
    file->pos = addr;
    if(file->pos > file->eof)
         file->eof = file->pos;

done:
    if(ret_value<0) {
        /* Reset file position */
        file->pos = HADDR_UNDEF;

        /* Close connection, etc. */
        srbFileClose(file->srb_conn, file->fd);
        clFinish(file->srb_conn);    
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
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
H5FD_srb_flush(H5FD_t *_file, hid_t dxpl_id, unsigned UNUSED closing)
{
    H5FD_srb_t *file = (H5FD_srb_t*)_file;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_srb_flush, FAIL);

    if(srbFileSync(file->srb_conn, file->fd) != 0)
        HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "srb file sync failed");

done:
    if(ret_value<0) {
        srbFileClose(file->srb_conn, file->fd);
        clFinish(file->srb_conn);    
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}

#endif  /* H5_HAVE_SRB */
