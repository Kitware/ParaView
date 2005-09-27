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
 *              Thursday, July 29, 1999
 *
 * Purpose:	This is the MPI-2 I/O driver.
 *
 */
#include "H5private.h"		/*library functions			*/
#include "H5ACprivate.h"        /* Metadata cache */
#include "H5Dprivate.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/*error handling			*/
#include "H5Fprivate.h"		/*files					*/
#include "H5FDprivate.h"	/*file driver				  */
#include "H5FDmpio.h"           /* MPI I/O file driver */
#include "H5Iprivate.h"		/*object IDs				  */
#include "H5MMprivate.h"        /* Memory allocation */
#include "H5Pprivate.h"		/*property lists			*/

#ifdef H5_HAVE_PARALLEL

/*
 * The driver identification number, initialized at runtime if H5_HAVE_PARALLEL
 * is defined. This allows applications to still have the H5FD_MPIO
 * "constants" in their source code.
 */
static hid_t H5FD_MPIO_g = 0;

/*
 * The description of a file belonging to this driver.
 * The EOF value is only used just after the file is opened in order for the
 * library to determine whether the file is empty, truncated, or okay. The MPIO
 * driver doesn't bother to keep it updated since it's an expensive operation.
 */
typedef struct H5FD_mpio_t {
    H5FD_t	pub;		/*public stuff, must be first		*/
    MPI_File	f;		/*MPIO file handle			*/
    MPI_Comm	comm;		/*communicator				*/
    MPI_Info	info;		/*file information			*/
    int         mpi_rank;       /* This process's rank                  */
    int         mpi_size;       /* Total number of processes            */
    haddr_t	eof;		/*end-of-file marker			*/
    haddr_t	eoa;		/*end-of-address marker			*/
    haddr_t	last_eoa;	/* Last known end-of-address marker	*/
} H5FD_mpio_t;

/* Prototypes */
static haddr_t H5FD_mpio_MPIOff_to_haddr(MPI_Offset mpi_off);
static herr_t H5FD_mpio_haddr_to_MPIOff(haddr_t addr, MPI_Offset *mpi_off/*out*/);

/* Callbacks */
static void *H5FD_mpio_fapl_get(H5FD_t *_file);
static void *H5FD_mpio_fapl_copy(const void *_old_fa);
static herr_t H5FD_mpio_fapl_free(void *_fa);
static H5FD_t *H5FD_mpio_open(const char *name, unsigned flags, hid_t fapl_id,
			      haddr_t maxaddr);
static herr_t H5FD_mpio_close(H5FD_t *_file);
static herr_t H5FD_mpio_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_mpio_get_eoa(H5FD_t *_file);
static herr_t H5FD_mpio_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_mpio_get_eof(H5FD_t *_file);
static herr_t  H5FD_mpio_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_mpio_read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
            size_t size, void *buf);
static herr_t H5FD_mpio_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
            size_t size, const void *buf);
static herr_t H5FD_mpio_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);
static herr_t H5FD_mpio_comm_info_dup(MPI_Comm comm, MPI_Info info,
				MPI_Comm *comm_new, MPI_Info *info_new);
static herr_t H5FD_mpio_comm_info_free(MPI_Comm *comm, MPI_Info *info);

/* MPIO-specific file access properties */
typedef struct H5FD_mpio_fapl_t {
    MPI_Comm		comm;		/*communicator			*/
    MPI_Info		info;		/*file information		*/
} H5FD_mpio_fapl_t;

/* The MPIO file driver information */
static const H5FD_class_t H5FD_mpio_g = {
    "mpio",					/*name			*/
    HADDR_MAX,					/*maxaddr		*/
    H5F_CLOSE_SEMI,				/* fc_degree		*/
    NULL,					/*sb_size		*/
    NULL,					/*sb_encode		*/
    NULL,					/*sb_decode		*/
    sizeof(H5FD_mpio_fapl_t),			/*fapl_size		*/
    H5FD_mpio_fapl_get,				/*fapl_get		*/
    H5FD_mpio_fapl_copy,			/*fapl_copy		*/
    H5FD_mpio_fapl_free, 			/*fapl_free		*/
    0,		                		/*dxpl_size		*/
    NULL,					/*dxpl_copy		*/
    NULL,					/*dxpl_free		*/
    H5FD_mpio_open,				/*open			*/
    H5FD_mpio_close,				/*close			*/
    NULL,					/*cmp			*/
    H5FD_mpio_query,		                /*query			*/
    NULL,					/*alloc			*/
    NULL,					/*free			*/
    H5FD_mpio_get_eoa,				/*get_eoa		*/
    H5FD_mpio_set_eoa, 				/*set_eoa		*/
    H5FD_mpio_get_eof,				/*get_eof		*/
    H5FD_mpio_get_handle,                       /*get_handle            */
    H5FD_mpio_read,				/*read			*/
    H5FD_mpio_write,				/*write			*/
    H5FD_mpio_flush,				/*flush			*/
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE                           /*fl_map                */
};

#ifdef H5FDmpio_DEBUG
/* Flags to control debug actions in H5Fmpio.
 * Meant to be indexed by characters.
 *
 * 'c' show result of MPI_Get_count after read
 * 'r' show read offset and size
 * 't' trace function entry and exit
 * 'w' show write offset and size
 */
static int H5FD_mpio_Debug[256] =
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
#endif

/* Interface initialization */
#define PABLO_MASK	H5FD_mpio_mask
#define INTERFACE_INIT	H5FD_mpio_init
static int interface_initialize_g = 0;

/* ======== Temporary, Local data transfer properties ======== */
/* Definitions for memory MPI type property */
#define H5FD_MPIO_XFER_MEM_MPI_TYPE_NAME        "H5FD_mpio_mem_mpi_type"
#define H5FD_MPIO_XFER_MEM_MPI_TYPE_SIZE        sizeof(MPI_Datatype)
/* Definitions for file MPI type property */
#define H5FD_MPIO_XFER_FILE_MPI_TYPE_NAME       "H5FD_mpio_file_mpi_type"
#define H5FD_MPIO_XFER_FILE_MPI_TYPE_SIZE       sizeof(MPI_Datatype)


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_init
 *
 * Purpose:	Initialize this driver by registering the driver with the
 *		library.
 *
 * Return:	Success:	The driver ID for the mpio driver.
 *
 *		Failure:	Negative.
 *
 * Programmer:	Robb Matzke
 *              Thursday, August 5, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_mpio_init(void)
{
#ifdef H5FDmpio_DEBUG
    static int H5FD_mpio_Debug_inited=0;
#endif /* H5FDmpio_DEBUG */
    hid_t ret_value;        	/* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_init, FAIL);

    if (H5I_VFL!=H5Iget_type(H5FD_MPIO_g))
        H5FD_MPIO_g = H5FDregister(&H5FD_mpio_g);

#ifdef H5FDmpio_DEBUG
    if (!H5FD_mpio_Debug_inited)
    {
	/* set debug mask */
	/* Should this be done in H5F global initialization instead of here? */
        const char *s = HDgetenv ("H5FD_mpio_Debug");
        if (s) {
	    while (*s){
		H5FD_mpio_Debug[(int)*s]++;
		s++;
	    }
        }
	H5FD_mpio_Debug_inited++;
    }
#endif /* H5FDmpio_DEBUG */
    
    /* Set return value */
    ret_value=H5FD_MPIO_g;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fapl_mpio
 *
 * Purpose:	Store the user supplied MPIO communicator comm and info in
 *		the file access property list FAPL_ID which can then be used
 *		to create and/or open the file.  This function is available
 *		only in the parallel HDF5 library and is not collective.
 *
 *		comm is the MPI communicator to be used for file open as
 *		defined in MPI_FILE_OPEN of MPI-2. This function makes a
 *		duplicate of comm. Any modification to comm after this function
 *		call returns has no effect on the access property list.
 *
 *		info is the MPI Info object to be used for file open as
 *		defined in MPI_FILE_OPEN of MPI-2. This function makes a
 *		duplicate of info. Any modification to info after this
 *		function call returns has no effect on the access property
 *		list.
 *
 *              If fapl_id has previously set comm and info values, they
 *              will be replaced and the old communicator and Info object
 *              are freed.
 *
 * Return:	Success:	Non-negative
 *
 * 		Failure:	Negative
 *
 * Programmer:	Albert Cheng
 *		Feb 3, 1998
 *
 * Modifications:
 *		Robb Matzke, 1998-02-18
 *		Check all arguments before the property list is updated so we
 *		don't leave the property list in a bad state if something
 *		goes wrong.  Also, the property list data type changed to
 *		allow more generality so all the mpi-related stuff is in the
 *		`u.mpi' member.  The `access_mode' will contain only
 * 		mpi-related flags defined in H5Fpublic.h.
 *
 *		Albert Cheng, 1998-04-16
 *		Removed the ACCESS_MODE argument.  The access mode is changed
 *		to be controlled by data transfer property list during data
 *		read/write calls.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *
 *		Raymond Lu, 2001-10-23
 *		Changed the file access list to the new generic property 
 *		list.
 *
 *		Albert Cheng, 2003-04-17
 *		Modified the description of the function that it now stores
 *		a duplicate of the communicator and INFO object.  Free the
 *		old duplicates if previously set.  (Work is actually done
 *		by H5P_set_driver.)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_mpio(hid_t fapl_id, MPI_Comm comm, MPI_Info info)
{
    H5FD_mpio_fapl_t	fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value;
    
    FUNC_ENTER_API(H5Pset_fapl_mpio, FAIL);
    H5TRACE3("e","iMcMi",fapl_id,comm,info);

    if(fapl_id==H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list");

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a file access list");
    if (MPI_COMM_NULL == comm)
	HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a valid communicator");

    /* Initialize driver specific properties */
    fa.comm = comm;
    fa.info = info;

    /* duplication is done during driver setting. */
    ret_value= H5P_set_driver(plist, H5FD_MPIO, &fa);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_fapl_mpio
 *
 * Purpose:	If the file access property list is set to the H5FD_MPIO
 *		driver then this function returns duplicates of the MPI
 *		communicator and Info object stored through the comm and
 *		info pointers.  It is the responsibility of the application
 *		to free the returned communicator and Info object.
 *
 * Return:	Success:	Non-negative with the communicator and
 *				Info object returned through the comm and
 *				info arguments if non-null. Since they are
 *				duplicates of the stored objects, future
 *				modifications to the access property list do
 *				not affect them and it is the responsibility
 *				of the application to free them.
 *
 * 		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Thursday, February 26, 1998
 *
 * Modifications:
 *
 *	        Albert Cheng, Apr 16, 1998
 *	        Removed the access_mode argument.  The access_mode is changed
 *	        to be controlled by data transfer property list during data
 *	        read/write calls.
 *
 *		Raymond Lu, 2001-10-23
 *		Changed the file access list to the new generic property 
 *		list.
 *
 *		Albert Cheng, 2003-04-17
 *		Return duplicates of the stored communicator and Info object.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_mpio(hid_t fapl_id, MPI_Comm *comm/*out*/, MPI_Info *info/*out*/)
{
    H5FD_mpio_fapl_t	*fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    MPI_Comm	comm_tmp=MPI_COMM_NULL;
    int		mpi_code;		/* mpi return code */
    herr_t      ret_value=SUCCEED;      /* Return value */
    
    FUNC_ENTER_API(H5Pget_fapl_mpio, FAIL);
    H5TRACE3("e","ixx",fapl_id,comm,info);

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a file access list");
    if (H5FD_MPIO!=H5P_get_driver(plist))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");
    if (NULL==(fa=H5P_get_driver_info(plist)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");

    /* Store the duplicated communicator in a temporary variable for error */
    /* recovery in case the INFO duplication fails.  We cannot attempt to */
    /* the value into *comm yet since if MPI_Comm_dup fails, we will end  */
    /* up freeing whatever *comm holds and that could be invalid. */
    if (comm){
	if (MPI_SUCCESS != (mpi_code=MPI_Comm_dup(fa->comm, &comm_tmp)))
	    HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code);
    }
    
    if (info){
	if (MPI_INFO_NULL != fa->info){
	    if (MPI_SUCCESS != (mpi_code=MPI_Info_dup(fa->info, info)))
		HMPI_GOTO_ERROR(FAIL, "MPI_Info_dup failed", mpi_code);
	}else{
	    /* do not dup it */
	    *info = MPI_INFO_NULL;
	}
    }

    if (comm)
        *comm = comm_tmp;

done:
    if (FAIL==ret_value){
	/* need to free anything created here */
	if (comm_tmp != MPI_COMM_NULL)
	    MPI_Comm_free(&comm_tmp);
    }
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_dxpl_mpio
 *
 * Purpose:	Set the data transfer property list DXPL_ID to use transfer
 *		mode XFER_MODE. The property list can then be used to control
 *		the I/O transfer mode during data I/O operations. The valid
 *		transfer modes are:
 *
 * 		H5FD_MPIO_INDEPENDENT:
 *			Use independent I/O access (the default).
 *
 * 		H5FD_MPIO_COLLECTIVE:
 *			Use collective I/O access.
 *			
 * Return:	Success:	Non-negative
 *
 * 		Failure:	Negative
 *
 * Programmer:	Albert Cheng
 *		April 2, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_mpio(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value;

    FUNC_ENTER_API(H5Pset_dxpl_mpio, FAIL);
    H5TRACE2("e","iDt",dxpl_id,xfer_mode);
    
    if(dxpl_id==H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list");

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl");
    if (H5FD_MPIO_INDEPENDENT!=xfer_mode && H5FD_MPIO_COLLECTIVE!=xfer_mode)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "incorrect xfer_mode");

    /* Set the transfer mode */
    if (H5P_set(plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

    /* Initialize driver-specific properties */
    ret_value= H5P_set_driver(plist, H5FD_MPIO, NULL);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_dxpl_mpio
 *
 * Purpose:	Queries the transfer mode current set in the data transfer
 *		property list DXPL_ID. This is not collective.
 *
 * Return:	Success:	Non-negative, with the transfer mode returned
 *				through the XFER_MODE argument if it is
 *				non-null.
 *
 * 		Failure:	Negative
 *
 * Programmer:	Albert Cheng
 *		April 2, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_dxpl_mpio(hid_t dxpl_id, H5FD_mpio_xfer_t *xfer_mode/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pget_dxpl_mpio, FAIL);
    H5TRACE2("e","ix",dxpl_id,xfer_mode);

    if(NULL == (plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl");
    if (H5FD_MPIO!=H5P_get_driver(plist))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");

    /* Get the transfer mode */
    if (xfer_mode)
        if (H5P_get(plist,H5D_XFER_IO_XFER_MODE_NAME,xfer_mode)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to get value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_communicator
 *
 * Purpose:	Returns the MPI communicator for the file.
 *
 * Return:	Success:	The communicator
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Monday, August  9, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
MPI_Comm
H5FD_mpio_communicator(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    MPI_Comm ret_value;         /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_communicator, MPI_COMM_NULL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Set return value */
    ret_value=file->comm;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_mpi_rank
 *
 * Purpose:	Returns the MPI rank for a process
 *
 * Return:	Success: non-negative
 *		Failure: negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 16, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_mpio_mpi_rank(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_mpi_rank, FAIL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Set return value */
    ret_value=file->mpi_rank;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5FD_mpio_mpi_rank() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_mpi_size
 *
 * Purpose:	Returns the number of MPI processes
 *
 * Return:	Success: non-negative
 *		Failure: negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 16, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_mpio_mpi_size(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_mpi_size, FAIL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Set return value */
    ret_value=file->mpi_size;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5FD_mpio_mpi_size() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_setup
 *
 * Purpose:	Set the buffer type BTYPE, file type FTYPE for a data
 *		transfer. Also request a MPI type transfer.
 *
 * Return:	Success:	0
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Monday, August  9, 1999
 *
 * Modifications:
 *
 *              Quincey Koziol - 2002/06/17
 *              Removed 'disp' parameter, read & write routines will use
 *              the address of the dataset in MPI_File_set_view() calls, as
 *              necessary.
 *
 *              Quincey Koziol - 2002/06/17
 *              Changed to set temporary properties in a dxpl, instead of
 *              flags in the file struct, which will make this more threadsafe.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpio_setup(hid_t dxpl_id, MPI_Datatype btype, MPI_Datatype ftype)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_setup, FAIL);

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dataset transfer list");

    /* Set buffer MPI type */
    if(H5P_insert(plist,H5FD_MPIO_XFER_MEM_MPI_TYPE_NAME,H5FD_MPIO_XFER_MEM_MPI_TYPE_SIZE,&btype,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert MPI-I/O property");

    /* Set file MPI type */
    if(H5P_insert(plist,H5FD_MPIO_XFER_FILE_MPI_TYPE_NAME,H5FD_MPIO_XFER_FILE_MPI_TYPE_SIZE,&ftype,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert MPI-I/O property");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_teardown
 *
 * Purpose:	Remove the temporary MPI-I/O properties from dxpl.
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, June 17, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpio_teardown(hid_t dxpl_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_teardown, FAIL);

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dataset transfer list");

    /* Remove buffer MPI type */
    if(H5P_remove(dxpl_id,plist,H5FD_MPIO_XFER_MEM_MPI_TYPE_NAME)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "can't remove MPI-I/O property");

    /* Remove file MPI type */
    if(H5P_remove(dxpl_id,plist,H5FD_MPIO_XFER_FILE_MPI_TYPE_NAME)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "can't remove MPI-I/O property");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_wait_for_left_neighbor
 *
 * Purpose:	Blocks until (empty) msg is received from immediately
 *		lower-rank neighbor. In conjunction with
 *		H5FD_mpio_signal_right_neighbor, useful for enforcing
 *		1-process-at-at-time access to critical regions to avoid race
 *		conditions (though it is overkill to require that the
 *		processes be allowed to proceed strictly in order of their
 *		rank).
 *
 * Note:	This routine doesn't read or write any file, just performs
 *		interprocess coordination. It really should reside in a
 *		separate package of such routines.
 *
 * Return:	Success:	0
 *		Failure:	-1
 *
 * Programmer:	rky
 *              19981207
 *
 * Modifications:
 *		Robb Matzke, 1999-08-09
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpio_wait_for_left_neighbor(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    char msgbuf[1];
    MPI_Status rcvstat;
    int		mpi_code;		/* mpi return code */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_wait_for_left_neighbor, FAIL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Portably initialize MPI status variable */
    HDmemset(&rcvstat,0,sizeof(MPI_Status));

    /* p0 has no left neighbor; all other procs wait for msg */
    if (file->mpi_rank != 0) {
        if (MPI_SUCCESS != (mpi_code=MPI_Recv( &msgbuf, 1, MPI_CHAR,
			file->mpi_rank-1, MPI_ANY_TAG, file->comm, &rcvstat )))
            HMPI_GOTO_ERROR(FAIL, "MPI_Recv failed", mpi_code);
    }
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_signal_right_neighbor
 *
 * Purpose:	Blocks until (empty) msg is received from immediately
 *		lower-rank neighbor. In conjunction with
 *		H5FD_mpio_wait_for_left_neighbor, useful for enforcing
 *		1-process-at-at-time access to critical regions to avoid race
 *		conditions (though it is overkill to require that the
 *		processes be allowed to proceed strictly in order of their
 *		rank).
 *
 * Note: 	This routine doesn't read or write any file, just performs
 *		interprocess coordination. It really should reside in a
 *		separate package of such routines.
 *
 * Return:	Success:	0
 *		Failure:	-1
 *
 * Programmer:	rky
 *              19981207
 *
 * Modifications:
 *		Robb Matzke, 1999-08-09
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpio_signal_right_neighbor(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    char msgbuf[1];
    int		mpi_code;		/* mpi return code */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_signal_right_neighbor, FAIL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    if (file->mpi_rank != (file->mpi_size-1)) {
        if (MPI_SUCCESS != (mpi_code=MPI_Send(&msgbuf, 0/*empty msg*/, MPI_CHAR,
			file->mpi_rank+1, 0, file->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Send failed", mpi_code);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_fapl_get
 *
 * Purpose:	Returns a file access property list which could be used to
 *		create another file the same as this one.
 *
 * Return:	Success:	Ptr to new file access property list with all
 *				fields copied from the file pointer.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Friday, August 13, 1999
 *
 * Modifications:
 * 		Albert Cheng, 2003-04-17
 * 		Duplicate the communicator and Info object so that the new
 * 		property list is insulated from the old one.
 *-------------------------------------------------------------------------
 */
static void *
H5FD_mpio_fapl_get(H5FD_t *_file)
{
    H5FD_mpio_t		*file = (H5FD_mpio_t*)_file;
    H5FD_mpio_fapl_t	*fa = NULL;
    void      *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_fapl_get, NULL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    if (NULL==(fa=H5MM_calloc(sizeof(H5FD_mpio_fapl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Duplicate communicator and Info object. */
    if (FAIL==H5FD_mpio_comm_info_dup(file->comm, file->info,
					&fa->comm, &fa->info))
	HGOTO_ERROR(H5E_INTERNAL, H5E_CANTCOPY, NULL,
		"Communicator/Info duplicate failed");

    /* Set return value */
    ret_value=fa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_fapl_copy
 *
 * Purpose:	Copies the mpio-specific file access properties.
 *
 * Return:	Success:	Ptr to a new property list
 *
 *		Failure:	NULL
 *
 * Programmer:	Albert Cheng
 *              Jan  8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_mpio_fapl_copy(const void *_old_fa)
{
    void		*ret_value = NULL;
    const H5FD_mpio_fapl_t *old_fa = (const H5FD_mpio_fapl_t*)_old_fa;
    H5FD_mpio_fapl_t	*new_fa = NULL;
    
    FUNC_ENTER_NOAPI(H5FD_mpio_fapl_copy, NULL);
#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "enter H5FD_mpio_fapl_copy\n");
#endif

    if (NULL==(new_fa=H5MM_malloc(sizeof(H5FD_mpio_fapl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy the general information */
    HDmemcpy(new_fa, old_fa, sizeof(H5FD_mpio_fapl_t));

    /* Duplicate communicator and Info object. */
    if (FAIL==H5FD_mpio_comm_info_dup(old_fa->comm, old_fa->info,
					&new_fa->comm, &new_fa->info))
	HGOTO_ERROR(H5E_INTERNAL, H5E_CANTCOPY, NULL,
		"Communicator/Info duplicate failed");
    ret_value = new_fa;

done:
    if (NULL == ret_value){
	/* cleanup */
	if (new_fa)
	    H5MM_xfree(new_fa);
    }

#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "leaving H5FD_mpio_fapl_copy\n");
#endif
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5FD_mpio_fapl_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_fapl_free
 *
 * Purpose:	Frees the mpio-specific file access properties.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1
 *
 * Programmer:	Albert Cheng
 *              Jan  8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_fapl_free(void *_fa)
{
    herr_t		ret_value = SUCCEED;
    H5FD_mpio_fapl_t	*fa = (H5FD_mpio_fapl_t*)_fa;

    FUNC_ENTER_NOAPI(H5FD_mpio_fapl_free, FAIL);
#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "in H5FD_mpio_fapl_free\n");
#endif
    assert(fa);

    /* Free the internal communicator and INFO object */
    assert(MPI_COMM_NULL!=fa->comm);
    H5FD_mpio_comm_info_free(&fa->comm, &fa->info);
    H5MM_xfree(fa);

done:
#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "leaving H5FD_mpio_fapl_free\n");
#endif
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5FD_mpio_fapl_free() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_open
 *
 * Purpose:     Opens a file with name NAME.  The FLAGS are a bit field with
 *		purpose similar to the second argument of open(2) and which
 *		are defined in H5Fpublic.h. The file access property list
 *		FAPL_ID contains the properties driver properties and MAXADDR
 *		is the largest address which this file will be expected to
 *		access.  This is collective.
 *
 * Return:      Success:        A new file pointer.
 *
 *              Failure:        NULL
 *
 * Programmer:  
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1998-02-18
 *		Added the ACCESS_PARMS argument.  Moved some error checking
 *		here from elsewhere.
 *
 *      	rky, 1998-01-11
 *      	Added H5FD_mpio_Debug debug flags controlled by MPI_Info.
 *
 *      	rky, 1998-08-28
 *		Init flag controlling redundant metadata writes to disk.
 *
 *      	rky, 1998-12-07
 *		Added barrier after MPI_File_set_size to prevent race
 *		condition -- subsequent writes were being truncated, causing
 *		holes in file.
 *		
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *
 *      	rky & ppw, 1999-11-07
 *		Modified "H5FD_mpio_open" so that file-truncation is
 *              avoided for brand-new files (with zero filesize).
 *
 * 		Albert Cheng, 2003-04-17
 * 		Duplicate the communicator and Info object so that file is
 * 		insulated from the old one.
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_mpio_open(const char *name, unsigned flags, hid_t fapl_id,
	       haddr_t UNUSED maxaddr)
{
    H5FD_mpio_t			*file=NULL;
    MPI_File			fh;
    unsigned                    file_opened=0;  /* Flag to indicate that the file was successfully opened */
    int				mpi_amode;
    int				mpi_rank;       /* MPI rank of this process */
    int				mpi_size;       /* Total number of MPI processes */
    int				mpi_code;	/* mpi return code */
    MPI_Offset			size;
    const H5FD_mpio_fapl_t	*fa=NULL;
    H5FD_mpio_fapl_t		_fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    MPI_Comm                    comm_dup=MPI_COMM_NULL;
    MPI_Info                    info_dup=MPI_INFO_NULL;
    H5FD_t			*ret_value;     /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_open, NULL);

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t']) {
    	fprintf(stdout, "Entering H5FD_mpio_open(name=\"%s\", flags=0x%x, "
		"fapl_id=%d, maxaddr=%lu)\n", name, flags, (int)fapl_id, (unsigned long)maxaddr);
    }
#endif

    /* Obtain a pointer to mpio-specific file access properties */
    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
    if (H5P_FILE_ACCESS_DEFAULT==fapl_id || H5FD_MPIO!=H5P_get_driver(plist)) {
	_fa.comm = MPI_COMM_SELF; /*default*/
	_fa.info = MPI_INFO_NULL; /*default*/
	fa = &_fa;
    } else {
	fa = H5P_get_driver_info(plist);
	assert(fa);
    }

    /* Duplicate communicator and Info object for use by this file. */
    if (FAIL==H5FD_mpio_comm_info_dup(fa->comm, fa->info, &comm_dup, &info_dup))
	HGOTO_ERROR(H5E_INTERNAL, H5E_CANTCOPY, NULL,
		"Communicator/Info duplicate failed");

    /* convert HDF5 flags to MPI-IO flags */
    /* some combinations are illegal; let MPI-IO figure it out */
    mpi_amode  = (flags&H5F_ACC_RDWR) ? MPI_MODE_RDWR : MPI_MODE_RDONLY;
    if (flags&H5F_ACC_CREAT)	mpi_amode |= MPI_MODE_CREATE;
    if (flags&H5F_ACC_EXCL)	mpi_amode |= MPI_MODE_EXCL;

#ifdef H5FDmpio_DEBUG
    /* Check for debug commands in the info parameter */
    {
	char debug_str[128];
        int flag, i;
        if (MPI_INFO_NULL != info_dup) {
            MPI_Info_get(fa->info, H5F_MPIO_DEBUG_KEY, 127, debug_str, &flag);
            if (flag) {
                fprintf(stdout, "H5FD_mpio debug flags=%s\n", debug_str );
                for (i=0;
                     debug_str[i]/*end of string*/ && i<128/*just in case*/;
                     ++i) {
                    H5FD_mpio_Debug[(int)debug_str[i]] = 1;
                }
            }
        }
    }
#endif

    /*OKAY: CAST DISCARDS CONST*/
    if (MPI_SUCCESS != (mpi_code=MPI_File_open(comm_dup, (char*)name, mpi_amode, info_dup, &fh)))
        HMPI_GOTO_ERROR(NULL, "MPI_File_open failed", mpi_code);
    file_opened=1;

    /* Get the MPI rank of this process and the total number of processes */
    if (MPI_SUCCESS != (mpi_code=MPI_Comm_rank (comm_dup, &mpi_rank)))
        HMPI_GOTO_ERROR(NULL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code=MPI_Comm_size (comm_dup, &mpi_size)))
        HMPI_GOTO_ERROR(NULL, "MPI_Comm_size failed", mpi_code);

    /* Build the return value and initialize it */
    if (NULL==(file=H5MM_calloc(sizeof(H5FD_mpio_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    file->f = fh;
    file->comm = comm_dup;
    file->info = info_dup;
    file->mpi_rank = mpi_rank;
    file->mpi_size = mpi_size;

    /* Only processor p0 will get the filesize and broadcast it. */
    if (mpi_rank == 0) {
        /* Get current file size */
        if (MPI_SUCCESS != (mpi_code=MPI_File_get_size(fh, &size)))
            HMPI_GOTO_ERROR(NULL, "MPI_File_get_size failed", mpi_code)
    } /* end if */

    /* Broadcast file-size */
    if (MPI_SUCCESS != (mpi_code=MPI_Bcast(&size, sizeof(MPI_Offset), MPI_BYTE, 0, comm_dup)))
        HMPI_GOTO_ERROR(NULL, "MPI_Bcast failed", mpi_code)

    /* Determine if the file should be truncated */
    if(size && (flags & H5F_ACC_TRUNC)) {
        if (MPI_SUCCESS != (mpi_code=MPI_File_set_size(fh, (MPI_Offset)0)))
            HMPI_GOTO_ERROR(NULL, "MPI_File_set_size failed", mpi_code)

        /* Don't let any proc return until all have truncated the file. */
        if (MPI_SUCCESS!= (mpi_code=MPI_Barrier(comm_dup)))
            HMPI_GOTO_ERROR(NULL, "MPI_Barrier failed", mpi_code)

        /* File is zero size now */
        size = 0;
    } /* end if */

    /* Set the size of the file (from library's perspective) */
    file->eof = H5FD_mpio_MPIOff_to_haddr(size);

    /* Set return value */
    ret_value=(H5FD_t*)file;

done:
    if(ret_value==NULL) {
        if(file_opened)
            MPI_File_close(&fh);
	if (MPI_COMM_NULL != comm_dup)
	    MPI_Comm_free(&comm_dup);
	if (MPI_INFO_NULL != info_dup)
	    MPI_Info_free(&info_dup);
	if (file)
	    H5MM_xfree(file);
    } /* end if */

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
        fprintf(stdout, "Leaving H5FD_mpio_open\n" );
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_close
 *
 * Purpose:     Closes a file.  This is collective.
 *
 * Return:      Success:	Non-negative
 *
 * 		Failure:	Negative
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1998-02-18
 *		Added the ACCESS_PARMS argument.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *
 * 		Albert Cheng, 2003-04-17
 *		Free the communicator stored.
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_close(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    int		mpi_code;	        /* MPI return code */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_close, FAIL);

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Entering H5FD_mpio_close\n");
#endif
    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* MPI_File_close sets argument to MPI_FILE_NULL */
    if (MPI_SUCCESS != (mpi_code=MPI_File_close(&(file->f)/*in,out*/)))
        HMPI_GOTO_ERROR(FAIL, "MPI_File_close failed", mpi_code);

    /* Clean up other stuff */
    H5FD_mpio_comm_info_free(&file->comm, &file->info);
    H5MM_xfree(file);

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Leaving H5FD_mpio_close\n");
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_query
 *
 * Purpose:	Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_query(const H5FD_t UNUSED *_file, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_mpio_query, FAIL);

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags=0;
        *flags|=H5FD_FEAT_AGGREGATE_METADATA; /* OK to aggregate metadata allocations */

        /* Distinguish between updating the metadata accumulator on writes and
         * reads.  This is particularly (perhaps only, even) important for MPI-I/O
         * where we guarantee that writes are collective, but reads may not be.
         * If we were to allow the metadata accumulator to be written during a
         * read operation, the application would hang.
         */
        *flags|=H5FD_FEAT_ACCUMULATE_METADATA_WRITE; /* OK to accumulate metadata for faster writes */

        *flags|=H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_get_eoa
 *
 * Purpose:	Gets the end-of-address marker for the file. The EOA marker
 *		is the first address past the last byte allocated in the
 *		format address space.
 *
 * Return:	Success:	The end-of-address marker.
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Robb Matzke
 *              Friday, August  6, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_mpio_get_eoa(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_get_eoa, HADDR_UNDEF);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Set return value */
    ret_value=file->eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_set_eoa
 *
 * Purpose:	Set the end-of-address marker for the file. This function is
 *		called shortly after an existing HDF5 file is opened in order
 *		to tell the driver where the end of the HDF5 data is located.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Friday, August 6, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_set_eoa(H5FD_t *_file, haddr_t addr)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_set_eoa, FAIL);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    file->eoa = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_get_eof
 *
 * Purpose:	Gets the end-of-file marker for the file. The EOF marker
 *		is the real size of the file.
 *
 *		The MPIO driver doesn't bother keeping this field updated
 *		since that's a relatively expensive operation. Fortunately
 *		the library only needs the EOF just after the file is opened
 *		in order to determine whether the file is empty, truncated,
 *		or okay.  Therefore, any MPIO I/O function will set its value
 *		to HADDR_UNDEF which is the error return value of this
 *		function.
 *
 * Return:	Success:	The end-of-address marker.
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Robb Matzke
 *              Friday, August  6, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_mpio_get_eof(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_mpio_get_eof, HADDR_UNDEF);

    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Set return value */
    ret_value=file->eof;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:       H5FD_mpio_get_handle
 * 
 * Purpose:        Returns the file handle of MPIO file driver.
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
H5FD_mpio_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{   
    H5FD_mpio_t         *file = (H5FD_mpio_t *)_file;
    herr_t              ret_value = SUCCEED;
                            
    FUNC_ENTER_NOAPI(H5FD_mpio_get_handle, FAIL);
                                    
    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid");

    *file_handle = &(file->f);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}
             

/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_read
 *
 * Purpose:	Reads SIZE bytes of data from FILE beginning at address ADDR
 *		into buffer BUF according to data transfer properties in
 *		DXPL_ID using potentially complex file and buffer types to
 *		effect the transfer.
 *
 *		Reading past the end of the MPI file returns zeros instead of
 *		failing.  MPI is able to coalesce requests from different
 *		processes (collective or independent).
 *
 * Return:	Success:	Zero. Result is stored in caller-supplied
 *				buffer BUF.
 *
 *		Failure:	-1, Contents of buffer BUF are undefined.
 *
 * Programmer:	rky, 1998-01-30
 *
 * Modifications:
 * 		Robb Matzke, 1998-02-18
 *		Added the ACCESS_PARMS argument.
 *
 * 		rky, 1998-04-10
 *		Call independent or collective MPI read, based on
 *		ACCESS_PARMS.
 *
 * 		Albert Cheng, 1998-06-01
 *		Added XFER_MODE to control independent or collective MPI
 *		read.
 *
 * 		rky, 1998-08-16
 *		Use BTYPE, FTYPE, and DISP from access parms. The guts of
 *		H5FD_mpio_read and H5FD_mpio_write should be replaced by a
 *		single dual-purpose routine.
 *
 * 		Robb Matzke, 1999-04-21
 *		Changed XFER_MODE to XFER_PARMS for all H5F_*_read()
 *		callbacks.
 *
 * 		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *		
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *
 *		Quincey Koziol,  2002-05-14
 *		Only call MPI_Get_count if we can use MPI_BYTE for the MPI type
 *              for the I/O transfer.  Someday we might include code to decode
 *              the MPI type used for more complicated transfers and call
 *              MPI_Get_count all the time.
 *
 *              Quincey Koziol - 2002/06/17
 *              Removed 'disp' parameter from H5FD_mpio_setup routine and use
 *              the address of the dataset in MPI_File_set_view() calls, as
 *              necessary.
 *
 *              Quincey Koziol - 2002/06/24
 *              Removed "lazy" MPI_File_set_view() calls, since they would fail
 *              if the first I/O was a collective I/O using MPI derived types
 *              and the next I/O was an independent I/O.
 *
 *              Quincey Koziol - 2003/10/22-31
 *              Restructured code massively, straightening out logic and finally
 *              getting the bytes_read stuff working.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t dxpl_id, haddr_t addr, size_t size,
	       void *buf/*out*/)
{
    H5FD_mpio_t			*file = (H5FD_mpio_t*)_file;
    MPI_Offset			mpi_off;
    MPI_Status  		mpi_stat;
    int				mpi_code;	/* mpi return code */
    MPI_Datatype		buf_type=MPI_BYTE;      /* MPI description of the selection in memory */
    int         		size_i;         /* Integer copy of 'size' to read */
    int         		bytes_read;     /* Number of bytes read in */
    int         		n;
    int                         type_size;      /* MPI datatype used for I/O's size */
    int                         io_size;        /* Actual number of bytes requested */
    unsigned			use_view_this_time=0;
    herr_t              	ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_mpio_read, FAIL);

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Entering H5FD_mpio_read\n" );
#endif
    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);
    /* Make certain we have the correct type of property list */
    assert(H5I_GENPROP_LST==H5I_get_type(dxpl_id));
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
    assert(buf);

    /* Portably initialize MPI status variable */
    HDmemset(&mpi_stat,0,sizeof(MPI_Status));

    /* some numeric conversions */
    if (H5FD_mpio_haddr_to_MPIOff(addr, &mpi_off/*out*/)<0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from haddr to MPI off");
    size_i = (int)size;
    if ((hsize_t)size_i != size)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from size to size_i");

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'r'])
        fprintf(stdout, "in H5FD_mpio_read  mpi_off=%ld  size_i=%d\n",
		(long)mpi_off, size_i );
#endif

    /* Only look for MPI views for raw data transfers */
    if(type==H5FD_MEM_DRAW) {
        H5P_genplist_t              *plist;      /* Property list pointer */
        H5FD_mpio_xfer_t            xfer_mode;   /* I/O tranfer mode */

        /* Obtain the data transfer properties */
        if(NULL == (plist = H5I_object(dxpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
        xfer_mode=(H5FD_mpio_xfer_t)H5P_peek_unsigned(plist, H5D_XFER_IO_XFER_MODE_NAME);
    
        /*
         * Set up for a fancy xfer using complex types, or single byte block. We
         * wouldn't need to rely on the use_view field if MPI semantics allowed
         * us to test that btype=ftype=MPI_BYTE (or even MPI_TYPE_NULL, which
         * could mean "use MPI_BYTE" by convention).
         */
        if(xfer_mode==H5FD_MPIO_COLLECTIVE) {
            MPI_Datatype		file_type;

            /* Remember that views are used */
            use_view_this_time=TRUE;

            /* prepare for a full-blown xfer using btype, ftype, and disp */
            if(H5P_get(plist,H5FD_MPIO_XFER_MEM_MPI_TYPE_NAME,&buf_type)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");
            if(H5P_get(plist,H5FD_MPIO_XFER_FILE_MPI_TYPE_NAME,&file_type)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

            /*
             * Set the file view when we are using MPI derived types
             */
            /*OKAY: CAST DISCARDS CONST QUALIFIER*/
            if (MPI_SUCCESS != (mpi_code=MPI_File_set_view(file->f, mpi_off, MPI_BYTE, file_type, (char*)"native",  file->info)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)

            /* When using types, use the address as the displacement for
             * MPI_File_set_view and reset the address for the read to zero
             */
            mpi_off=0;
        } /* end if */
    } /* end if */

    /* Read the data. */
    if (use_view_this_time) {
#ifdef H5FDmpio_DEBUG
	if (H5FD_mpio_Debug[(int)'t'])
	    fprintf(stdout, "H5FD_mpio_read: using MPIO collective mode\n");
#endif
        if (MPI_SUCCESS!= (mpi_code=MPI_File_read_at_all(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat )))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at_all failed", mpi_code);

        /*
         * Reset the file view when we used MPI derived types
         */
        /*OKAY: CAST DISCARDS CONST QUALIFIER*/
        if (MPI_SUCCESS != (mpi_code=MPI_File_set_view(file->f, 0, MPI_BYTE, MPI_BYTE, (char*)"native",  file->info)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)
    } else {
        if (MPI_SUCCESS!= (mpi_code=MPI_File_read_at(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at failed", mpi_code);
    }

    /* How many bytes were actually read? */
    /* [This works because the "basic elements" we use for all our MPI derived
     *  types are MPI_BYTE - QAK]
     */
    if (MPI_SUCCESS != (mpi_code=MPI_Get_elements(&mpi_stat, MPI_BYTE, &bytes_read)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Get_elements failed", mpi_code)

    /* Get the type's size */
    if (MPI_SUCCESS != (mpi_code=MPI_Type_size(buf_type,&type_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_size failed", mpi_code)

    /* Compute the actual number of bytes requested */
    io_size=type_size*size_i;

    /* Check for read failure */
    if (bytes_read<0 || bytes_read>io_size)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed")

    /*
     * This gives us zeroes beyond end of physical MPI file.
     */
    if ((n=(io_size-bytes_read)) > 0)
        HDmemset((char*)buf+bytes_read, 0, (size_t)n);

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Leaving H5FD_mpio_read\n" );
#endif

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_write
 *
 * Purpose:	Writes SIZE bytes of data to FILE beginning at address ADDR
 *		from buffer BUF according to data transfer properties in
 *		DXPL_ID using potentially complex file and buffer types to
 *		effect the transfer.
 *
 *		MPI is able to coalesce requests from different processes
 *		(collective and independent).
 *
 * Return:	Success:	Zero. USE_TYPES and OLD_USE_TYPES in the
 *				access params are altered.
 *
 *		Failure:	-1, USE_TYPES and OLD_USE_TYPES in the
 *				access params may be altered.
 *
 * Programmer:	Unknown
 *              January 30, 1998
 *
 * Modifications:
 *		rky, 1998-08-28
 *		If the file->allsame flag is set, we assume that all the
 *		procs in the relevant MPI communicator will write identical
 *		data at identical offsets in the file, so only proc 0 will
 *		write, and all other procs will wait for p0 to finish. This
 *		is useful for writing metadata, for example. Note that we
 *		don't _check_ that the data is identical. Also, the mechanism
 *		we use to eliminate the redundant writes is by requiring a
 *		call to H5FD_mpio_tas_allsame before the write, which is
 *		rather klugey. Would it be better to pass a parameter to
 *		low-level writes like H5F_block_write and H5F_low_write,
 *		instead?  Or...??? Also, when I created this mechanism I
 *		wanted to minimize the difference in behavior between the old
 *		way of doing things (i.e., all procs write) and the new way,
 *		so the writes are eliminated at the very lowest level, here
 *		in H5FD_mpio_write. It may be better to rethink that, and
 *		short-circuit the writes at a higher level (e.g., at the
 *		points in the code where H5FD_mpio_tas_allsame is called).
 *
 *
 * 		Robb Matzke, 1998-02-18
 *		Added the ACCESS_PARMS argument.
 *
 * 		rky, 1998-04-10
 *		Call independent or collective MPI write, based on
 *		ACCESS_PARMS.
 *
 * 		rky, 1998-04-24
 *		Removed redundant write from H5FD_mpio_write.
 *
 * 		Albert Cheng, 1998-06-01
 *		Added XFER_MODE to control independent or collective MPI
 *		write.
 *
 * 		rky, 1998-08-16
 *		Use BTYPE, FTYPE, and DISP from access parms. The guts of
 *		H5FD_mpio_read and H5FD_mpio_write should be replaced by a
 *		single dual-purpose routine.
 *
 * 		rky, 1998-08-28
 *		Added ALLSAME parameter to make all but proc 0 skip the
 *		actual write.
 *
 * 		Robb Matzke, 1999-04-21
 *		Changed XFER_MODE to XFER_PARMS for all H5FD_*_write()
 *		callbacks.
 *
 * 		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *
 *		Albert Cheng, 1999-12-19
 *		When only-p0-write-allsame-data, p0 Bcasts the
 *		ret_value to other processes.  This prevents
 *		a racing condition (that other processes try to
 *		read the file before p0 finishes writing) and also
 *		allows all processes to report the same ret_value.
 *
 *		Kim Yates, Pat Weidhaas,  2000-09-26
 *		Move block of coding where only p0 writes after the
 *              MPI_File_set_view call. 
 *
 *		Quincey Koziol,  2002-05-10
 *		Instead of always writing metadata from process 0, spread the
 *              burden among all the processes by using a round-robin rotation
 *              scheme.
 *
 *		Quincey Koziol,  2002-05-10
 *		Removed allsame code, keying off the type parameter instead.
 *
 *		Quincey Koziol,  2002-05-14
 *		Only call MPI_Get_count if we can use MPI_BYTE for the MPI type
 *              for the I/O transfer.  Someday we might include code to decode
 *              the MPI type used for more complicated transfers and call
 *              MPI_Get_count all the time.
 *
 *              Quincey Koziol - 2002/06/17
 *              Removed 'disp' parameter from H5FD_mpio_setup routine and use
 *              the address of the dataset in MPI_File_set_view() calls, as
 *              necessary.
 *
 *              Quincey Koziol - 2002/06/24
 *              Removed "lazy" MPI_File_set_view() calls, since they would fail
 *              if the first I/O was a collective I/O using MPI derived types
 *              and the next I/O was an independent I/O.
 *
 *              Quincey Koziol - 2002/07/18
 *              Added "block_before_meta_write" dataset transfer flag, which
 *              is set during writes from a metadata cache flush and indicates
 *              that all the processes must sync up before (one of them)
 *              writing metadata.
 *
 *              Quincey Koziol - 2003/10/22-31
 *              Restructured code massively, straightening out logic and finally
 *              getting the bytes_written stuff working.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
		size_t size, const void *buf)
{
    H5FD_mpio_t			*file = (H5FD_mpio_t*)_file;
    MPI_Offset 		 	mpi_off;
    MPI_Status			mpi_stat;
    MPI_Datatype		buf_type=MPI_BYTE;      /* MPI description of the selection in memory */
    int			        mpi_code;	/* MPI return code */
    int         		size_i, bytes_written;
    int                         type_size;      /* MPI datatype used for I/O's size */
    int                         io_size;        /* Actual number of bytes requested */
    unsigned			use_view_this_time=0;
    H5P_genplist_t              *plist;                 /* Property list pointer */
    herr_t              	ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_mpio_write, FAIL);

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Entering H5FD_mpio_write\n" );
#endif
    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);
    /* Make certain we have the correct type of property list */
    assert(H5I_GENPROP_LST==H5I_get_type(dxpl_id));
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
    assert(buf);

    /* Portably initialize MPI status variable */
    HDmemset(&mpi_stat,0,sizeof(MPI_Status));

    /* some numeric conversions */
    if (H5FD_mpio_haddr_to_MPIOff(addr, &mpi_off)<0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from haddr to MPI off");
    size_i = (int)size;
    if ((hsize_t)size_i != size)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from size to size_i");

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'w'])
        fprintf(stdout, "in H5FD_mpio_write  mpi_off=%ld  size_i=%d\n",
                (long)mpi_off, size_i);
#endif

    /* Obtain the data transfer properties */
    if(NULL == (plist = H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    
    if(type==H5FD_MEM_DRAW) {
        H5FD_mpio_xfer_t            xfer_mode;   /* I/O tranfer mode */

        /* Obtain the data transfer properties */
        xfer_mode = H5P_peek_unsigned(plist, H5D_XFER_IO_XFER_MODE_NAME);

        /*
         * Set up for a fancy xfer using complex types, or single byte block. We
         * wouldn't need to rely on the use_view field if MPI semantics allowed
         * us to test that btype=ftype=MPI_BYTE (or even MPI_TYPE_NULL, which
         * could mean "use MPI_BYTE" by convention).
         */
        if(xfer_mode==H5FD_MPIO_COLLECTIVE) {
            MPI_Datatype		file_type;

            /* Remember that views are used */
            use_view_this_time=TRUE;

            /* prepare for a full-blown xfer using btype, ftype, and disp */
            if(H5P_get(plist,H5FD_MPIO_XFER_MEM_MPI_TYPE_NAME,&buf_type)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");
            if(H5P_get(plist,H5FD_MPIO_XFER_FILE_MPI_TYPE_NAME,&file_type)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

            /*
             * Set the file view when we are using MPI derived types
             */
            /*OKAY: CAST DISCARDS CONST QUALIFIER*/
            if (MPI_SUCCESS != (mpi_code=MPI_File_set_view(file->f, mpi_off, MPI_BYTE, file_type, (char*)"native", file->info)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)

            /* When using types, use the address as the displacement for
             * MPI_File_set_view and reset the address for the read to zero
             */
            mpi_off=0;
        } /* end if */
    } /* end if */
    else {
        unsigned		        block_before_meta_write=0;      /* Whether to block before a metadata write */

        /* Check if we need to syncronize all processes before attempting metadata write
         * (Prevents race condition where the process writing the metadata goes ahead
         * and writes the metadata to the file before all the processes have
         * read the data, "transmitting" data from the "future" to the reading
         * process. -QAK )
         *
         * The only time we don't want to block before a metadata write is when
         * we are flushing out a bunch of metadata.  Then, we block before the
         * first write and don't block for further writes in the sequence.
         */
        if(H5P_exist_plist(plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME)>0)
            if(H5P_get(plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,&block_before_meta_write)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get H5AC property");

        if(block_before_meta_write)
            if (MPI_SUCCESS!= (mpi_code=MPI_Barrier(file->comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code);

        /* Only one process will do the actual write if all procs in comm write same metadata */
        if (file->mpi_rank != H5_PAR_META_WRITE) {
#ifdef H5FDmpio_DEBUG
            if (H5FD_mpio_Debug[(int)'w']) {
                fprintf(stdout,
                    "  proc %d: in H5FD_mpio_write (write omitted)\n",
                    file->mpi_rank );
            }
#endif
            HGOTO_DONE(SUCCEED) /* skip the actual write */
        }
    } /* end if */

    /* Write the data. */
    if (use_view_this_time) {
#ifdef H5FDmpio_DEBUG
        if (H5FD_mpio_Debug[(int)'t'])
            fprintf(stdout, "H5FD_mpio_write: using MPIO collective mode\n");
#endif
        /*OKAY: CAST DISCARDS CONST QUALIFIER*/
        if (MPI_SUCCESS != (mpi_code=MPI_File_write_at_all(file->f, mpi_off, (void*)buf, size_i, buf_type, &mpi_stat)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at_all failed", mpi_code);

        /*
         * Reset the file view when we used MPI derived types
         */
        /*OKAY: CAST DISCARDS CONST QUALIFIER*/
        if (MPI_SUCCESS != (mpi_code=MPI_File_set_view(file->f, 0, MPI_BYTE, MPI_BYTE, (char*)"native",  file->info)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)
    } else {
        /*OKAY: CAST DISCARDS CONST QUALIFIER*/
        if (MPI_SUCCESS != (mpi_code=MPI_File_write_at(file->f, mpi_off, (void*)buf, size_i, buf_type, &mpi_stat)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at failed", mpi_code);
    }

    /* How many bytes were actually written? */
    /* [This works because the "basic elements" we use for all our MPI derived
     *  types are MPI_BYTE - QAK]
     */
    if (MPI_SUCCESS != (mpi_code=MPI_Get_elements(&mpi_stat, MPI_BYTE, &bytes_written)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Get_elements failed", mpi_code)

    /* Get the type's size */
    if (MPI_SUCCESS != (mpi_code=MPI_Type_size(buf_type,&type_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_size failed", mpi_code)

    /* Compute the actual number of bytes requested */
    io_size=type_size*size_i;

    /* Check for write failure */
    if (bytes_written != io_size)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")

    /* Forget the EOF value (see H5FD_mpio_get_eof()) --rpm 1999-08-06 */
    file->eof = HADDR_UNDEF;
    
done:
    /* if only one process writes, need to broadcast the ret_value to other processes */
    if (type!=H5FD_MEM_DRAW) {
	if (MPI_SUCCESS != (mpi_code=MPI_Bcast(&ret_value, sizeof(ret_value), MPI_BYTE, H5_PAR_META_WRITE, file->comm)))
	    HMPI_DONE_ERROR(FAIL, "MPI_Bcast failed", mpi_code)
    } /* end if */

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "proc %d: Leaving H5FD_mpio_write with ret_value=%d\n",
	    file->mpi_rank, ret_value );
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_flush
 *
 * Purpose:     Makes sure that all data is on disk.  This is collective.
 *
 * Return:      Success:	Non-negative
 *
 * 		Failure:	Negative
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1998-02-18
 *		Added the ACCESS_PARMS argument.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *
 *              Robb Matzke, 2000-12-29
 *              Make sure file size is at least as large as the last
 *              allocated byte.
 *
 *              Quincey Koziol, 2002-06-??
 *              Changed file extension method to use MPI_File_set_size instead 
 *              read->write method.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_flush(H5FD_t *_file, hid_t UNUSED dxpl_id, unsigned closing)
{
    H5FD_mpio_t		*file = (H5FD_mpio_t*)_file;
    int			mpi_code;	/* mpi return code */
    MPI_Offset          mpi_off;
    herr_t              ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_mpio_flush, FAIL);

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Entering H5FD_mpio_flush\n" );
#endif
    assert(file);
    assert(H5FD_MPIO==file->pub.driver_id);

    /* Extend the file to make sure it's large enough, then sync.
     * Unfortunately, keeping track of EOF is an expensive operation, so
     * we can't just check whether EOF<EOA like with other drivers.
     * Therefore we'll just read the byte at EOA-1 and then write it back. */
    if(file->eoa>file->last_eoa) {
#ifdef H5_MPI_FILE_SET_SIZE_BIG
        if (H5FD_mpio_haddr_to_MPIOff(file->eoa, &mpi_off)<0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "cannot convert from haddr_t to MPI_Offset");

        /* Extend the file's size */
        if (MPI_SUCCESS != (mpi_code=MPI_File_set_size(file->f, mpi_off)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_size failed", mpi_code);
#else /* H5_MPI_FILE_SET_SIZE_BIG */
        if (0==file->mpi_rank) {
            uint8_t             byte=0;
            MPI_Status          mpi_stat;

            /* Portably initialize MPI status variable */
            HDmemset(&mpi_stat,0,sizeof(MPI_Status));

            if (H5FD_mpio_haddr_to_MPIOff(file->eoa-1, &mpi_off)<0)
                HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "cannot convert from haddr_t to MPI_Offset");
            if (MPI_SUCCESS != (mpi_code=MPI_File_read_at(file->f, mpi_off, &byte, 1, MPI_BYTE, &mpi_stat)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at failed", mpi_code);
            if (MPI_SUCCESS != (mpi_code=MPI_File_write_at(file->f, mpi_off, &byte, 1, MPI_BYTE, &mpi_stat)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at failed", mpi_code);
        } /* end if */
#endif /* H5_MPI_FILE_SET_SIZE_BIG */

	/* Don't let any proc return until all have extended the file.
         * (Prevents race condition where some processes go ahead and write
         * more data to the file before all the processes have finished making
         * it the shorter length, potentially truncating the file and dropping
         * the new data written)
         */
        if (MPI_SUCCESS!= (mpi_code=MPI_Barrier(file->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code);

        /* Update the 'last' eoa value */
        file->last_eoa=file->eoa;
    } /* end if */

    /* Only sync the file if we are not going to immediately close it */
    if(!closing) {
        if (MPI_SUCCESS != (mpi_code=MPI_File_sync(file->f)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_sync failed", mpi_code);
    } /* end if */

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_Debug[(int)'t'])
    	fprintf(stdout, "Leaving H5FD_mpio_flush\n" );
#endif

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_MPIOff_to_haddr
 *
 * Purpose:     Convert an MPI_Offset value to haddr_t.
 *
 * Return:      Success:	The haddr_t equivalent of the MPI_OFF
 *				argument.
 *				
 *              Failure:	HADDR_UNDEF
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-04-23
 *		An error is reported for address overflows. The ADDR output
 *		argument is optional.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *------------------------------------------------------------------------- 
 */
static haddr_t
H5FD_mpio_MPIOff_to_haddr(MPI_Offset mpi_off)
{
    haddr_t ret_value=HADDR_UNDEF;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_mpio_MPIOff_to_haddr);

    if (mpi_off != (MPI_Offset)(haddr_t)mpi_off)
        ret_value=HADDR_UNDEF;
    else
        ret_value=(haddr_t)mpi_off;

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_haddr_to_MPIOff
 *
 * Purpose:     Convert an haddr_t value to MPI_Offset.
 *
 * Return:      Success:	Non-negative, the MPI_OFF argument contains
 *				the converted value.
 *
 * 		Failure:	Negative, MPI_OFF is undefined.
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-04-23
 *		An error is reported for address overflows. The ADDR output
 *		argument is optional.
 *
 * 		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_haddr_to_MPIOff(haddr_t addr, MPI_Offset *mpi_off/*out*/)
{
    herr_t ret_value=FAIL;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_mpio_haddr_to_MPIOff);

    if (mpi_off)
        *mpi_off = (MPI_Offset)addr;
    if (addr != (haddr_t)(MPI_Offset)addr)
        ret_value=FAIL;
    else
        ret_value=SUCCEED;

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_comm_info_dup
 *
 * Purpose:     Make duplicates of communicator and Info object.
 * 		If the Info object is in fact MPI_INFO_NULL, no duplicate
 * 		is made but the same value assigned to the new Info object
 * 		handle.
 *
 * Return:      Success:	Non-negative.  The new communicator and Info
 * 				object handles are returned via comm_new and
 * 				info_new pointers.
 *
 * 		Failure:	Negative.
 *
 * Programmer:  Albert Cheng
 *              Jan  8, 2003
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_comm_info_dup(MPI_Comm comm, MPI_Info info, MPI_Comm *comm_new, MPI_Info *info_new)
{
    herr_t	ret_value=SUCCEED;
    MPI_Comm	comm_dup=MPI_COMM_NULL;
    MPI_Info	info_dup=MPI_INFO_NULL;
    int		mpi_code;
    
    FUNC_ENTER_NOAPI(H5FD_mpio_comm_info_dup, FAIL);

#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "In H5FD_mpio_comm_info_dup: argument comm/info = %d/%ld\n", comm, (long)info);
#endif
    /* Check arguments */
    if (MPI_COMM_NULL == comm)
	HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "not a valid argument");
    if (!comm_new || !info_new)
	HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "bad pointers");

    /* Dup them.  Using temporary variables for error recovery cleanup. */
    if (MPI_SUCCESS != (mpi_code=MPI_Comm_dup(comm, &comm_dup)))
	HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code);
    if (MPI_INFO_NULL != info){
	if (MPI_SUCCESS != (mpi_code=MPI_Info_dup(info, &info_dup)))
	    HMPI_GOTO_ERROR(FAIL, "MPI_Info_dup failed", mpi_code);
    }else{
	/* No dup, just copy it. */
	info_dup = info;
    }

    /* copy them to the return arguments */
    *comm_new = comm_dup;
    *info_new = info_dup;

done:
    if (FAIL == ret_value){
	/* need to free anything created here */
	if (MPI_COMM_NULL != comm_dup)
	    MPI_Comm_free(&comm_dup);
	if (MPI_INFO_NULL != info_dup)
	    MPI_Info_free(&info_dup);
    }

#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "Leaving H5FD_mpio_comm_info_dup\n");
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_comm_info_free
 *
 * Purpose:     Free the communicator and Info object.
 * 		If comm or info is in fact MPI_COMM_NULL or MPI_INFO_NULL
 * 		respectively, no action occurs to it. 
 *
 * Return:      Success:	Non-negative.  The values the pointers refer
 * 				to will be set to the corresponding NULL
 * 				handles.
 *
 * 		Failure:	Negative.
 *
 * Programmer:  Albert Cheng
 *              Jan  8, 2003
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_mpio_comm_info_free(MPI_Comm *comm, MPI_Info *info)
{
    herr_t      ret_value=SUCCEED;
    FUNC_ENTER_NOAPI(H5FD_mpio_comm_info_free, FAIL);

#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "in H5FD_mpio_comm_info_free\n");
#endif
    /* Check arguments */
    if (!comm || !info)
	HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "not a valid argument");

    if (MPI_COMM_NULL != *comm)
	MPI_Comm_free(comm);
    if (MPI_INFO_NULL != *info)
	MPI_Info_free(info);

done:
#ifdef H5FDmpio_DEBUG
if (H5FD_mpio_Debug[(int)'t'])
fprintf(stderr, "Leaving H5FD_mpio_comm_info_free\n");
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}
#endif /* H5_HAVE_PARALLEL */
