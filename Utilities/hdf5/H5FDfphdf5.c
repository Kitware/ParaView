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

#include "H5private.h"      /* Library functions                            */
#include "H5ACprivate.h"    /* Metadata cache                               */
#include "H5Dprivate.h"     /* Dataset functions                            */
#include "H5Eprivate.h"     /* Error handling                               */
#include "H5Fprivate.h"     /* Files                                        */
#include "H5FDprivate.h"    /* File driver                                  */
#include "H5FDfphdf5.h"     /* Flexible PHDF5 I/O file driver               */
#include "H5FDmpio.h"       /* MPI I/O file driver                          */
#include "H5FDprivate.h"    /* File drivers                                 */
#include "H5Iprivate.h"     /* Object IDs                                   */
#include "H5MMprivate.h"    /* Memory allocation                            */
#include "H5Pprivate.h"     /* Property lists                               */

#ifdef H5_HAVE_FPHDF5

#include "H5FPprivate.h"    /* Flexible PHDF5                               */

/*
 * The driver identification number, initialized at runtime if
 * H5_HAVE_FPHDF5 is defined. This allows applications to still have
 * the H5FD_FPHDF5 "constants" in their source code (it also makes this
 * file strictly ANSI compliant when H5_HAVE_FPHDF5 isn't defined)
 */
static hid_t H5FD_FPHDF5_g = 0;

/*
 * Prototypes
 */
static haddr_t  H5FD_fphdf5_MPIOff_to_haddr(MPI_Offset mpi_off);
static herr_t   H5FD_fphdf5_haddr_to_MPIOff(haddr_t addr, MPI_Offset *mpi_off);

/*
 * Callbacks
 */
static void    *H5FD_fphdf5_fapl_get(H5FD_t *_file);
static H5FD_t  *H5FD_fphdf5_open(const char *name, unsigned flags,
                                 hid_t fapl_id, haddr_t maxaddr);
static herr_t   H5FD_fphdf5_close(H5FD_t *_file);
static herr_t   H5FD_fphdf5_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t  H5FD_fphdf5_get_eoa(H5FD_t *_file);
static herr_t   H5FD_fphdf5_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t  H5FD_fphdf5_get_eof(H5FD_t *_file);
static herr_t   H5FD_fphdf5_get_handle(H5FD_t *_file, hid_t fapl,
                                     void **file_handle);
static herr_t   H5FD_fphdf5_read(H5FD_t *_file, H5FD_mem_t mem_type, hid_t dxpl_id,
                                 haddr_t addr, size_t size, void *buf);
static herr_t   H5FD_fphdf5_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id,
                                  haddr_t addr, size_t size, const void *buf);
static herr_t   H5FD_fphdf5_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);

/*
 * FPHDF5-specific file access properties
 */
typedef struct H5FD_fphdf5_fapl_t {
    MPI_Comm    comm;           /*communicator          */
    MPI_Comm    barrier_comm;   /*barrier communicator  */
    MPI_Info    info;           /*file information      */
    unsigned    sap_rank;       /*SAP's rank            */
    unsigned    capt_rank;      /*captain rank          */
} H5FD_fphdf5_fapl_t;

/*
 * The FPHDF5 file driver information
 */
const H5FD_class_t H5FD_fphdf5_g = {
    "fphdf5",                                   /*name                  */
    HADDR_MAX,                                  /*maxaddr               */
    H5F_CLOSE_SEMI,                             /*fc_degree             */
    NULL,                                       /*sb_size               */
    NULL,                                       /*sb_encode             */
    NULL,                                       /*sb_decode             */
    sizeof(H5FD_fphdf5_fapl_t),                 /*fapl_size             */
    H5FD_fphdf5_fapl_get,                       /*fapl_get              */
    NULL,                                       /*fapl_copy             */
    NULL,                                       /*fapl_free             */
    0,                                          /*dxpl_size             */
    NULL,                                       /*dxpl_copy             */
    NULL,                                       /*dxpl_free             */
    H5FD_fphdf5_open,                           /*open                  */
    H5FD_fphdf5_close,                          /*close                 */
    NULL,                                       /*cmp                   */
    H5FD_fphdf5_query,                          /*query                 */
    NULL,                                       /*alloc                 */
    NULL,                                       /*free                  */
    H5FD_fphdf5_get_eoa,                        /*get_eoa               */
    H5FD_fphdf5_set_eoa,                        /*set_eoa               */
    H5FD_fphdf5_get_eof,                        /*get_eof               */
    H5FD_fphdf5_get_handle,                     /*get_handle            */
    H5FD_fphdf5_read,                           /*read                  */
    H5FD_fphdf5_write,                          /*write                 */
    H5FD_fphdf5_flush,                          /*flush                 */
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE                           /*fl_map                */
};

/* Interface initialization */
#define PABLO_MASK          H5FD_fphdf5_mask
#define INTERFACE_INIT      H5FD_fphdf5_init

static int interface_initialize_g = 0;

/*
 * The view is set to this value
 */
static char H5FD_mpio_native[] = "native";

/* ======== Temporary, Local data transfer properties ======== */
/*
 * Definitions for memory MPI type property
 */
#define H5FD_FPHDF5_XFER_MEM_MPI_TYPE_NAME      "H5FD_fphdf5_mem_mpi_type"
#define H5FD_FPHDF5_XFER_MEM_MPI_TYPE_SIZE      sizeof(MPI_Datatype)

/*
 * Definitions for file MPI type property
 */
#define H5FD_FPHDF5_XFER_FILE_MPI_TYPE_NAME     "H5FD_fphdf5_file_mpi_type"
#define H5FD_FPHDF5_XFER_FILE_MPI_TYPE_SIZE     sizeof(MPI_Datatype)

/*
 * Definitions for whether to use MPI types property
 */
#define H5FD_FPHDF5_XFER_USE_VIEW_NAME          "H5FD_fphdf5_use_view"
#define H5FD_FPHDF5_XFER_USE_VIEW_SIZE          sizeof(unsigned)


/*-------------------------------------------------------------------------
 * Function:	H5FD_fphdf5_init
 * Purpose:	Initialize this driver by registering the driver with the
 *		library.
 * Return:	Success:    The driver ID for the FPHDF5 driver.
 *		Failure:    Doesn't fail.
 * Programmer:	Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_fphdf5_init(void)
{
    hid_t   ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_init, FAIL);

    if (H5Iget_type(H5FD_FPHDF5_g) != H5I_VFL)
        H5FD_FPHDF5_g = H5FDregister(&H5FD_fphdf5_g);

    /* Set return value */
    ret_value = H5FD_FPHDF5_g;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fapl_fphdf5
 * Purpose:	Store the user supplied MPIO communicator COMM and INFO
 *              in the file access property list FAPL_ID which can then
 *              be used to create and/or open the file. This function is
 *              available only in the parallel HDF5 library and is not
 *              collective.
 *
 *		COMM is the MPI communicator to be used for file open as
 *		defined in MPI_FILE_OPEN of MPI-2. This function does not
 *		make a duplicated communicator. Any modification to COMM
 *		after this function call returns may have an indeterminate
 *		effect on the access property list. Users should not
 *		modify the communicator while it is defined in a property
 *		list.
 *
 *		INFO is the MPI info object to be used for file open as
 *		defined in MPI_FILE_OPEN of MPI-2. This function does not
 *		make a duplicated info. Any modification to info after
 *		this function call returns may have an indeterminate effect
 *		on the access property list. Users should not modify the
 *		info while it is defined in a property list.
 * Return:	Success:    SUCCEED
 * 		Failure:    FAIL
 * Programmer:	Bill Wendling
 *		30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_fphdf5(hid_t fapl_id, MPI_Comm comm, MPI_Comm barrier_comm,
                   MPI_Info info, unsigned sap_rank)
{
    H5FD_fphdf5_fapl_t  fa;
    H5P_genplist_t     *plist;
    int                 mrc, comm_size;
    herr_t              ret_value;
    
    FUNC_ENTER_API(H5Pset_fapl_fphdf5, FAIL);
    H5TRACE5("e","iMcMcMiIu",fapl_id,comm,barrier_comm,info,sap_rank);

    if (fapl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL,
                    "can't set values in default property list");

    /* Check arguments */
    if ((plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a file access list");

    if ((mrc = MPI_Comm_size(comm, &comm_size)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mrc);

    /* Initialize driver specific properties */
    fa.comm = comm;
    fa.barrier_comm = barrier_comm;
    fa.info = info;
    fa.sap_rank = sap_rank;
    fa.capt_rank = (sap_rank + 1) % comm_size;

    ret_value = H5P_set_driver(plist, H5FD_FPHDF5, &fa);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_fapl_fphdf5
 * Purpose:     If the file access property list is set to the
 *              H5FD_FPHDF5 driver then this function returns the MPI
 *              communicator and information through the COMM and INFO
 *              pointers.
 * Return:      Success:    SUCCEED with the communicator and information
 *                          returned through the COMM and INFO arguments
 *                          if non-null. Neither piece of information is
 *                          copied and they are therefore valid only
 *                          until the file access property list is
 *                          modified or closed.
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_fphdf5(hid_t fapl_id, MPI_Comm *comm, MPI_Comm *barrier_comm,
                   MPI_Info *info, unsigned *sap_rank, unsigned *capt_rank)
{
    H5FD_fphdf5_fapl_t *fa;
    H5P_genplist_t     *plist;
    herr_t              ret_value = SUCCEED;
    
    FUNC_ENTER_API(H5Pget_fapl_fphdf5, FAIL);
    H5TRACE6("e","i*Mc*Mc*Mi*Iu*Iu",fapl_id,comm,barrier_comm,info,sap_rank,
             capt_rank);

    if ((plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a file access list");

    if (H5P_get_driver(plist) != H5FD_FPHDF5)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");

    if ((fa = H5P_get_driver_info(plist)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");

    if (comm)
        *comm = fa->comm;

    if (barrier_comm)
        *barrier_comm = fa->barrier_comm;

    if (info)
        *info = fa->info;

    if (sap_rank)
        *sap_rank = fa->sap_rank;

    if (capt_rank)
        *capt_rank = fa->capt_rank;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_communicator
 * Purpose:     Returns the MPI communicator for the file.
 * Return:      Success:    The communicator
 *              Failure:    NULL
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
MPI_Comm
H5FD_fphdf5_communicator(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    MPI_Comm        ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_communicator, MPI_COMM_NULL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->comm;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_barrier_communicator
 * Purpose:     Returns the MPI communicator for the file that can be
 *              used in an MPI_Barrier() statement for the client
 *              processes.
 * Return:      Success:    The barrier communicator
 *              Failure:    NULL
 * Programmer:  Bill Wendling
 *              10. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
MPI_Comm
H5FD_fphdf5_barrier_communicator(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    MPI_Comm        ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_communicator, MPI_COMM_NULL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->barrier_comm;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_mpi_rank
 * Purpose:     Returns the MPI rank for a process
 * Return:      Success:    MPI rank
 *              Failure:    Doesn't fail
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
int
H5FD_fphdf5_mpi_rank(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    int             ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_mpi_rank, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->mpi_rank;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_mpi_size
 * Purpose:     Returns the number of MPI processes
 * Return:      Success:    Number of MPI processes
 *              Failure:    Doesn't fail
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
int
H5FD_fphdf5_mpi_size(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    int             ret_value; 

    FUNC_ENTER_NOAPI(H5FD_fphdf5_mpi_size, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->mpi_size;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_file_id
 * Purpose:     Returns the file ID for the file.
 * Return:      Success:    File ID
 *              Failure:    Doesn't fail
 * Programmer:  Bill Wendling
 *              19. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
unsigned
H5FD_fphdf5_file_id(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    unsigned        ret_value; 

    FUNC_ENTER_NOAPI(H5FD_fphdf5_file_id, 0);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->file_id;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_is_sap
 * Purpose:     Asks the question "Is this process the SAP?".
 * Return:      Success:    Non-zero if it is, 0 otherwise
 *              Failure:    Doesn't fails
 * Programmer:  Bill Wendling
 *              19. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
hbool_t
H5FD_fphdf5_is_sap(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    hbool_t         ret_value = FALSE;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_is_sap, FALSE);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = ((unsigned)file->mpi_rank == H5FP_sap_rank);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_is_captain
 * Purpose:     Asks the question "Is this process the captain?".
 * Return:      Success:    Non-zero if it is, 0 otherwise
 *              Failure:    Doesn't fails
 * Programmer:  Bill Wendling
 *              19. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
hbool_t
H5FD_fphdf5_is_captain(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    hbool_t         ret_value = FALSE;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_is_sap, FALSE);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = ((unsigned)file->mpi_rank == H5FP_capt_rank);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_is_fphdf5_driver
 * Purpose:     Asks the question "Is this an FPHDF5 driver?".
 * Return:      Success:    Non-zero if it is, 0 otherwise
 *              Failure:    Doesn't fails
 * Programmer:  Bill Wendling
 *              26. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
hbool_t
H5FD_is_fphdf5_driver(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    hbool_t         ret_value = FALSE;

    FUNC_ENTER_NOAPI(H5FD_is_fphdf5_driver, FALSE);

    /* check args */
    assert(file);

    /* Set return value */
    ret_value = file->pub.driver_id == H5FD_FPHDF5;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_setup
 * Purpose:     Set the buffer type BTYPE, file type FTYPE for a data
 *              transfer. Also request an MPI type transfer.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fphdf5_setup(hid_t dxpl_id, MPI_Datatype btype,
                  MPI_Datatype ftype, unsigned use_view)
{
    H5P_genplist_t *plist;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_setup, FAIL);

    /* Check arguments */
    if ((plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dataset transfer list");

    /* Set buffer MPI type */
    if (H5P_insert(plist, H5FD_FPHDF5_XFER_MEM_MPI_TYPE_NAME,
                   H5FD_FPHDF5_XFER_MEM_MPI_TYPE_SIZE, &btype,
                   NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert MPI-I/O property");

    /* Set file MPI type */
    if (H5P_insert(plist, H5FD_FPHDF5_XFER_FILE_MPI_TYPE_NAME,
                   H5FD_FPHDF5_XFER_FILE_MPI_TYPE_SIZE, &ftype,
                   NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert MPI-I/O property");

    /* Set 'use view' property */
    if (H5P_insert(plist, H5FD_FPHDF5_XFER_USE_VIEW_NAME,
                   H5FD_FPHDF5_XFER_USE_VIEW_SIZE, &use_view,
                   NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert MPI-I/O property");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_teardown
 * Purpose:     Remove the temporary MPI-I/O properties from dxpl.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fphdf5_teardown(hid_t dxpl_id)
{
    H5P_genplist_t *plist;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_teardown, FAIL);

    /* Check arguments */
    if ((plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dataset transfer list");

    /* Remove buffer MPI type */
    if (H5P_remove(dxpl_id, plist, H5FD_FPHDF5_XFER_MEM_MPI_TYPE_NAME) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "can't remove MPI-I/O property");

    /* Remove file MPI type */
    if (H5P_remove(dxpl_id, plist, H5FD_FPHDF5_XFER_FILE_MPI_TYPE_NAME) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "can't remove MPI-I/O property");

    /* Remove 'use view' property */
    if (H5P_remove(dxpl_id, plist, H5FD_FPHDF5_XFER_USE_VIEW_NAME) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "can't remove MPI-I/O property");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_dxpl_fphdf5
 * Purpose:     Set the data transfer property list DXPL_ID to use
 *              transfer mode XFER_MODE. The property list can then be
 *              used to control the I/O transfer mode during data I/O
 *              operations. The valid transfer modes are:
 *
 *              H5FD_MPIO_INDEPENDENT:
 *                  Use independent I/O access (the default).
 *
 *              H5FD_MPIO_COLLECTIVE:
 *                  Use collective I/O access.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              10. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_fphdf5(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode)
{
    H5P_genplist_t *plist;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_API(H5Pset_dxpl_fphdf5, FAIL);
    H5TRACE2("e","iDt",dxpl_id,xfer_mode);
    
    if (dxpl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL,
                    "can't set values in default property list");

    /* Check arguments */
    if ((plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl");

    if (xfer_mode != H5FD_MPIO_INDEPENDENT && xfer_mode != H5FD_MPIO_COLLECTIVE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "incorrect xfer_mode");

    /* Set the transfer mode */
    if (H5P_set(plist, H5D_XFER_IO_XFER_MODE_NAME, &xfer_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

    /* Initialize driver-specific properties */
    ret_value = H5P_set_driver(plist, H5FD_MPIO, NULL);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_dxpl_fphdf5
 * Purpose:     Queries the transfer mode current set in the data
 *              transfer property list DXPL_ID. This is not collective.
 * Return:      Success:    SUCCEED - with the transfer mode returned
 *                                    through the XFER_MODE argument if
 *                                    it is non-null.
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              10. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_dxpl_fphdf5(hid_t dxpl_id, H5FD_mpio_xfer_t *xfer_mode)
{
    H5P_genplist_t *plist;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_API(H5Pget_dxpl_fphdf5, FAIL);
    H5TRACE2("e","i*Dt",dxpl_id,xfer_mode);

    if ((plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl");

    if (H5P_get_driver(plist) != H5FD_FPHDF5)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");

    /* Get the transfer mode */
    if (xfer_mode)
        if (H5P_get(plist, H5D_XFER_IO_XFER_MODE_NAME, xfer_mode) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to get value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_fapl_get
 * Purpose:     Returns a file access property list which could be used
 *              to create another file the same as this one.
 * Return:      Success:    Ptr to new file access property list with all
 *                          fields copied from the file pointer.
 *              Failure:    NULL
 * Programmer:  Bill Wendling
 *              07. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static void *
H5FD_fphdf5_fapl_get(H5FD_t *_file)
{
    H5FD_fphdf5_t      *file = (H5FD_fphdf5_t*)_file;
    H5FD_fphdf5_fapl_t *fa = NULL;
    void               *ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_fapl_get, NULL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    if ((fa = H5MM_calloc(sizeof(H5FD_fphdf5_fapl_t))) == NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* These should both be copied. --rpm, 1999-08-13 */
    fa->comm = file->comm;
    fa->barrier_comm = file->barrier_comm;
    fa->info = file->info;

    /* Set return value */
    ret_value = fa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_open
 * Purpose:     Opens a file with name NAME. The FLAGS are a bit field with
 *              purpose similar to the second argument of open(2) and
 *              which are defined in H5Fpublic.h. The file access
 *              property list FAPL_ID contains the properties driver
 *              properties and MAXADDR is the largest address which this
 *              file will be expected to access.  This is collective.
 * Return:      Success:    A new file pointer.
 *              Failure:    NULL
 * Programmer:  Bill Wendling
 *              05. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_fphdf5_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_fphdf5_t              *file = NULL;
    MPI_File                    fh;
    int                         mpi_amode;
    int                         mpi_rank;
    int                         mpi_size;
    int                         mrc;
    MPI_Offset                  size;
    H5FD_fphdf5_fapl_t          _fa;
    const H5FD_fphdf5_fapl_t   *fa = NULL;
    H5P_genplist_t             *plist;
    unsigned long               feature_flags;
    hsize_t                     meta_block_size = 0;
    hsize_t                     sdata_block_size = 0;
    hsize_t                     threshold;
    hsize_t                     alignment;
    unsigned                    file_id;
    unsigned                    req_id;
    MPI_Status                  status;
    H5FD_t                     *ret_value = NULL;

    /* Flag to indicate that the file was successfully opened */
    unsigned                    file_opened = FALSE;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_open, NULL);

    /* check args */
    assert(name);

    /* Obtain a pointer to fphdf5-specific file access properties */
    if ((plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)) == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");

    if (fapl_id == H5P_FILE_ACCESS_DEFAULT || H5P_get_driver(plist) != H5FD_FPHDF5) {
        _fa.comm = MPI_COMM_SELF;           /*default*/
        _fa.barrier_comm = MPI_COMM_SELF;   /*default*/
        _fa.info = MPI_INFO_NULL;           /*default*/
        fa = &_fa;
    } else {
        fa = H5P_get_driver_info(plist);
        assert(fa);
    }

    /*
     * Convert HDF5 flags to MPI-IO flags. Some combinations are illegal;
     * let MPI-IO figure it out
     */
    mpi_amode = (flags & H5F_ACC_RDWR) ? MPI_MODE_RDWR : MPI_MODE_RDONLY;

    if (flags & H5F_ACC_CREAT)  mpi_amode |= MPI_MODE_CREATE;
    if (flags & H5F_ACC_EXCL)   mpi_amode |= MPI_MODE_EXCL;

    /* OKAY: CAST DISCARDS CONST */
    if ((mrc = MPI_File_open(H5FP_SAP_BARRIER_COMM, (char *)name, mpi_amode,
                             fa->info, &fh)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(NULL, "MPI_File_open failed", mrc);

    file_opened = TRUE;

    if (H5P_get(plist, H5F_ACS_META_BLOCK_SIZE_NAME, &meta_block_size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get meta data block size");

    if (H5P_get(plist, H5F_ACS_SDATA_BLOCK_SIZE_NAME, &sdata_block_size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get 'small data' block size");

    if (H5P_get(plist, H5F_ACS_ALIGN_THRHD_NAME, &threshold) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get alignment threshold");

    if (H5P_get(plist, H5F_ACS_ALIGN_NAME, &alignment) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get alignment");    

    /* Retrieve the VFL driver feature flags */
    H5FD_fphdf5_query(NULL, &feature_flags);    /* doesn't fail */

    /* Inform the SAP that the file was opened */
    if (H5FP_request_open(H5FP_OBJ_FILE, maxaddr, feature_flags,
                          meta_block_size, sdata_block_size, threshold,
                          alignment, &file_id, &req_id) == FAIL)
        HGOTO_ERROR(H5E_FPHDF5, H5E_CANTOPENFILE, NULL,
                    "can't inform SAP of file open");

    /* Grab the rank of this process */
    if ((mrc = MPI_Comm_rank(H5FP_SAP_COMM, &mpi_rank)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(NULL, "MPI_Comm_rank failed", mrc);

    HDmemset(&status, 0, sizeof(status));

    /* Broadcast the file ID */
    if ((mrc = MPI_Bcast(&file_id, 1, MPI_UNSIGNED,
                         (int)H5FP_capt_barrier_rank,
                         H5FP_SAP_BARRIER_COMM)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(NULL, "MPI_Bcast failed", mrc);

    /* The captain rank will get the filesize and broadcast it. */
    if ((unsigned)mpi_rank == H5FP_capt_rank)
        /* Get current file size */
        if ((mrc = MPI_File_get_size(fh, &size)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(NULL, "MPI_File_get_size failed", mrc);

    /* Broadcast file size */
    if ((mrc = MPI_Bcast(&size, sizeof(MPI_Offset), MPI_BYTE,
                         (int)H5FP_capt_barrier_rank,
                         H5FP_SAP_BARRIER_COMM)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(NULL, "MPI_Bcast failed", mrc);

    /* Only if size > 0, truncate the file - if requested */
    if (size && (flags & H5F_ACC_TRUNC)) {
        if ((mrc = MPI_File_set_size(fh, (MPI_Offset)0)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(NULL, "MPI_File_set_size (file truncation) failed", mrc);

        /* Don't let any proc return until all have truncated the file. */
        if ((mrc = MPI_Barrier(H5FP_SAP_BARRIER_COMM)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(NULL, "MPI_Barrier failed", mrc);

        size = 0;
    }

    /* Grab the size of the communicator */
    if ((mrc = MPI_Comm_size(H5FP_SAP_COMM, &mpi_size)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(NULL, "MPI_Comm_size failed", mrc);

    /* Build the return value and initialize it */
    if ((file = H5MM_calloc(sizeof(H5FD_fphdf5_t))) == NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    file->file_id = file_id;
    file->f = fh;
    file->comm = fa->comm;
    file->info = fa->info;
    file->mpi_rank = mpi_rank;
    file->mpi_size = mpi_size;
    file->eof = H5FD_fphdf5_MPIOff_to_haddr(size);

    /* Set return value */
    ret_value = (H5FD_t *)file;

done:
    if (!ret_value && file_opened)
        MPI_File_close(&fh);

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_close
 * Purpose:     Closes a file. This is collective.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              07. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_close(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t *)_file;
    H5FP_status_t   status;
    unsigned        req_id;
    int             mrc;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_close, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* MPI_File_close sets argument to MPI_FILE_NULL */
    if ((mrc = MPI_File_close(&file->f)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_File_close failed", mrc);

    if (H5FP_request_close(_file, file->file_id, &req_id, &status) == FAIL)
        HGOTO_ERROR(H5E_IO, H5E_CANTCLOSEFILE, FAIL,
                    "can't inform SAP of file close");

    /* Clean up other stuff */
    H5MM_xfree(file);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_query
 * Purpose:     Set the flags that this VFL driver is capable of
 *              supporting. (listed in H5FDpublic.h)
 * Return:      Success:    SUCCEED
 *              Failure:    Doesn't fail.
 * Programmer:  Bill Wendling
 *              07. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_query(const H5FD_t UNUSED *_file, unsigned long *flags /* out */)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_query, FAIL);

    /* check args */
    assert(flags);

    /* Set the VFL feature flags that this driver supports */
    if (flags) {
        *flags = 0;

        /* OK to aggregate metadata allocations */
        *flags |= H5FD_FEAT_AGGREGATE_METADATA;

        /*
         * Distinguish between updating the metadata accumulator on
         * writes and reads. This is particularly (perhaps only, even)
         * important for MPI-I/O where we guarantee that writes are
         * collective, but reads may not be. If we were to allow the
         * metadata accumulator to be written during a read operation,
         * the application would hang.
         */

#if 0
        /*
         * FIXME: For now, having metadata accumulate causes problems for
         * the SAP when it goes to allocate data (oddly enough, an
         * allocation can result in a call to H5FD_free...which can
         * result in a call to H5FD_write...which needs a data xfer
         * property list...but only when metadata accumulation is turned
         * on...go figure). Turn it off for now. -- BW 02/19/2003
         */

        /* OK to accumulate metadata for faster writes */
        *flags |= H5FD_FEAT_ACCUMULATE_METADATA_WRITE;
#endif  /* 0 */

        /* OK to aggregate "small" raw data allocations */
        *flags |= H5FD_FEAT_AGGREGATE_SMALLDATA;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_get_eoa
 * Purpose:     Gets the end-of-address marker for the file. The EOA
 *              marker is the first address past the last byte allocated
 *              in the format address space.
 * Return:      Success:    The end-of-address marker.
 *              Failure:    HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              07. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_fphdf5_get_eoa(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t *)_file;
    haddr_t         ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_get_eoa, HADDR_UNDEF);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_set_eoa
 * Purpose:     Set the end-of-address marker for the file. This function
 *              is called shortly after an existing HDF5 file is opened
 *              in order to tell the driver where the end of the HDF5
 *              data is located.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              06. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_set_eoa(H5FD_t *_file, haddr_t addr)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t *)_file;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_set_eoa, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    file->eoa = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_get_eof
 * Purpose:     Gets the end-of-file marker for the file. The EOF marker
 *              is the real size of the file.
 *
 *              The FPHDF5 driver doesn't bother keeping this field updated
 *              since that's a relatively expensive operation.
 *              Fortunately the library only needs the EOF just after the
 *              file is opened in order to determine whether the file is
 *              empty, truncated, or okay. Therefore, any MPIO I/O
 *              function will set its value to HADDR_UNDEF which is the
 *              error return value of this function.
 * Return:      Success:    The end-of-address marker
 *              Failure:    HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              06. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_fphdf5_get_eof(H5FD_t *_file)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    haddr_t         ret_value;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_get_eof, HADDR_UNDEF);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /* Set return value */
    ret_value = file->eof;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_get_handle
 * Purpose:     Returns the file handle of MPIO file driver.
 * Returns:     Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              06. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
*/
static herr_t  
H5FD_fphdf5_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{   
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t *)_file;
    herr_t          ret_value = SUCCEED;
                            
    FUNC_ENTER_NOAPI(H5FD_fphdf5_get_handle, FAIL);

    /* check args */
    assert(file);
                                    
    if (!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid");

    *file_handle = &file->f;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_read
 * Purpose:	Reads SIZE bytes of data from FILE beginning at address
 *              ADDR into buffer BUF according to data transfer
 *              properties in DXPL_ID using potentially complex file and
 *              buffer types to effect the transfer.
 *
 *              Reading past the end of the MPI file returns zeros
 *              instead of failing. MPI is able to coalesce requests
 *              from different processes (collective or independent).
 * Return:      Success:    SUCCEED - Result is stored in caller-supplied
 *                                    buffer BUF
 *              Failure:    FAIL - Contents of buffer BUF are undefined
 * Programmer:  Bill Wendling
 *              10. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_read(H5FD_t *_file, H5FD_mem_t mem_type, hid_t dxpl_id,
                 haddr_t addr, size_t size, void *buf)
{
    H5FD_fphdf5_t      *file = (H5FD_fphdf5_t*)_file;
    MPI_Offset          mpi_off;
    MPI_Offset          mpi_disp;
    MPI_Status          status;
    int                 mrc;
    MPI_Datatype        buf_type;
    MPI_Datatype        file_type;
    int                 size_i;
    int                 bytes_read;
    int                 n;
    unsigned            use_view_this_time = 0;
    H5P_genplist_t     *plist;
    H5FD_mpio_xfer_t    xfer_mode = H5FD_MPIO_INDEPENDENT;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_read, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);
    assert(buf);

    /* make certain we have the correct type of property list */
    assert(H5I_get_type(dxpl_id) == H5I_GENPROP_LST);
    assert(H5P_isa_class(dxpl_id, H5P_DATASET_XFER) == TRUE);

    /* Portably initialize MPI status variable */
    HDmemset(&status, 0, sizeof(MPI_Status));

    /* Some numeric conversions */
    if (H5FD_fphdf5_haddr_to_MPIOff(addr, &mpi_off) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL,
                    "can't convert from haddr_t to MPI offset");

    size_i = (int)size;

    if ((hsize_t)size_i != size)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from size_t to int");

    /* Obtain the data transfer properties */
    if ((plist = H5I_object(dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");

    if (H5P_get_driver(plist) == H5FD_FPHDF5)
        /* Get the transfer mode */
        xfer_mode = H5P_peek_unsigned(plist, H5D_XFER_IO_XFER_MODE_NAME);

    /*
     * Set up for a fancy xfer using complex types, or single byte block.
     * We wouldn't need to rely on the use_view field if MPI semantics
     * allowed us to test that btype == ftype == MPI_BYTE (or even
     * MPI_TYPE_NULL, which could mean "use MPI_BYTE" by convention).
     */
    if (H5P_exist_plist(plist, H5FD_FPHDF5_XFER_USE_VIEW_NAME) > 0)
        if (H5P_get(plist, H5FD_FPHDF5_XFER_USE_VIEW_NAME, &use_view_this_time) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

    if (use_view_this_time) {
        /* Prepare for a full-blown xfer using btype, ftype, and disp */
        if (H5P_get(plist, H5FD_FPHDF5_XFER_MEM_MPI_TYPE_NAME, &buf_type) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

        if (H5P_get(plist, H5FD_FPHDF5_XFER_FILE_MPI_TYPE_NAME, &file_type) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

        /*
         * When using types, use the address as the displacement for
         * MPI_File_set_view and reset the address for the read to zero
         */
        mpi_disp = mpi_off;
        mpi_off = 0;

        /* Set the file view when we are using MPI derived types */
        if ((mrc = MPI_File_set_view(file->f, (MPI_Offset)mpi_disp, MPI_BYTE,
                                     file_type, H5FD_mpio_native,
                                     file->info)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mrc);
    } else {
        /*
         * Prepare for a simple xfer of a contiguous block of bytes. The
         * btype, ftype, and disp fields are not used.
         */
        buf_type = MPI_BYTE;
        file_type = MPI_BYTE;
        mpi_disp = 0;   /* mpi_off is already set */
    }

    /* If metadata, check the metadata cache first */
    if (mem_type != H5FD_MEM_DRAW) {
        /*
         * This is metadata - we want to try to read it from the SAP
         * first.
         */
        H5FP_status_t   sap_status;
        unsigned        req_id;

        if (H5FP_request_read_metadata(_file, file->file_id, dxpl_id, mem_type,
                                       mpi_off, size, (uint8_t**)&buf,
                                       &bytes_read, &req_id, &sap_status) != SUCCEED) {
            /* FIXME: The read failed, for some reason */
HDfprintf(stderr, "%s:%d: Metadata cache read failed!\n", FUNC, __LINE__);
        }

        if (sap_status == H5FP_STATUS_OK) {
            /* WAH-HOO! We've found it! We can leave now */
            goto finished_read;
        } else if (sap_status != H5FP_STATUS_MDATA_NOT_CACHED) {
            /* FIXME: something bad happened */
HDfprintf(stderr, "%s:%d: Metadata cache read failed!\n", FUNC, __LINE__);
        }
    }

    /* Read the data. */
    assert(xfer_mode == H5FD_MPIO_INDEPENDENT || xfer_mode == H5FD_MPIO_COLLECTIVE);

    if (xfer_mode == H5FD_MPIO_INDEPENDENT) {
        if ((mrc = MPI_File_read_at(file->f, mpi_off, buf, size_i,
                                    buf_type, &status)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at failed", mrc);
    } else {
        if ((mrc = MPI_File_read_at_all(file->f, mpi_off, buf, size_i,
                                        buf_type, &status )) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at_all failed", mrc);
    }

    /*
     * KLUDGE, Robb Matzke, 2000-12-29
     * The LAM implementation of MPI_Get_count() says
     *
     *    MPI_Get_count: invalid argument (rank 0, MPI_COMM_WORLD)
     *
     * So I'm commenting this out until it can be investigated. The
     * returned `bytes_written' isn't used anyway because of Kim's kludge
     * to avoid bytes_written < 0. Likewise in H5FD_fphdf5_write().
     */

#ifdef H5_HAVE_MPI_GET_COUNT /* Bill and Albert's kludge*/
    /*
     * Yet Another KLUDGE, Albert Cheng & Bill Wendling, 2001-05-11.
     * Many systems don't support MPI_Get_count so we need to do a
     * configure thingy to fix this.
     */

    /*
     * Calling MPI_Get_count with "MPI_BYTE" is only valid when we
     * actually had the 'buf_type' set to MPI_BYTE -QAK
     */
    if (use_view_this_time) {
        /*
         * Figure out the mapping from the MPI 'buf_type' to bytes,
         * someday... If this gets fixed (and MPI_Get_count() is
         * reliable), the kludge below where the 'bytes_read' value from
         * MPI_Get_count() is overwritten with the 'size_i' parameter can
         * be removed. -QAK
         */
    } else {
        /* How many bytes were actually read? */
        if ((mrc = MPI_Get_count(&status, MPI_BYTE, &bytes_read)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_Get_count failed", mrc);
    }
#endif /* H5_HAVE_MPI_GET_COUNT */

    /*
     * KLUGE rky 1998-02-02
     * MPI_Get_count incorrectly returns negative count; fake a complete
     * read.
     */
    bytes_read = size_i;

    /*
     * Reset the file view when we used MPI derived types
     */
    if (use_view_this_time)
        if ((mrc = MPI_File_set_view(file->f, (MPI_Offset)0, MPI_BYTE, MPI_BYTE,
                                     H5FD_mpio_native,  file->info)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mrc);

finished_read:
    /* Check for read failure */
    if (bytes_read < 0 || bytes_read > size_i)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed");

    /*
     * This gives us zeroes beyond end of physical MPI file. What about
     * reading past logical end of HDF5 file???
     */
    n = size_i - bytes_read;

    if (n > 0) {
        if (use_view_this_time)
            /*
             * INCOMPLETE rky 1998-09-18
             * Haven't implemented reading zeros beyond EOF. What to do???
             */
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "eof file read failed");

        memset((char*)buf + bytes_read, 0, (size_t)n);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_write
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address
 *              ADDR from buffer BUF according to data transfer
 *              properties in DXPL_ID using potentially complex file and
 *              buffer types to effect the transfer.
 *
 *              MPI is able to coalesce requests from different processes
 *              (collective and independent).
 * Return:      Success:    SUCCEED - USE_TYPES and OLD_USE_TYPES in the
 *                                    access params are altered.
 *              Failure:    FAIL - USE_TYPES and OLD_USE_TYPES in the
 *                                 access params may be altered.
 * Programmer:  Bill Wendling
 *              10. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_write(H5FD_t *_file, H5FD_mem_t mem_type, hid_t dxpl_id,
                  haddr_t addr, size_t size, const void *buf)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    MPI_Offset      mpi_off;
    int             size_i;
    unsigned        dumping = 0;
    H5P_genplist_t *plist;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_write, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);
    assert(buf);

    /* Make certain we have the correct type of property list */
    assert(H5I_get_type(dxpl_id) == H5I_GENPROP_LST);
    assert(H5P_isa_class(dxpl_id, H5P_DATASET_XFER) == TRUE);

    /* some numeric conversions */
    if (H5FD_fphdf5_haddr_to_MPIOff(addr, &mpi_off) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL,
                    "can't convert from haddr to MPI off");

    size_i = (int)size;

    if ((hsize_t)size_i != size)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from size to size_i");

    /* Obtain the data transfer properties */
    if ((plist = H5I_object(dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");

#if 0
    /* Metadata specific actions */
    if (mem_type != H5FD_MEM_DRAW) {
        unsigned block_before_meta_write = 0;
        int mrc;

        /*
         * Check if we need to syncronize all processes before attempting
         * metadata write (Prevents race condition where the process
         * writing the metadata goes ahead and writes the metadata to the
         * file before all the processes have read the data,
         * "transmitting" data from the "future" to the reading process.
         * -QAK )
         */
        if (H5P_exist_plist(plist, H5AC_BLOCK_BEFORE_META_WRITE_NAME) > 0)
            if (H5P_get(plist, H5AC_BLOCK_BEFORE_META_WRITE_NAME,
                        &block_before_meta_write) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get H5AC property");

        if (block_before_meta_write)
            if ((mrc = MPI_Barrier(file->barrier_comm)) != MPI_SUCCESS)
                HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mrc);
    }
#endif  /* 0 */

    if (H5P_exist_plist(plist, H5FD_FPHDF5_XFER_DUMPING_METADATA) > 0)
        if (H5P_get(plist, H5FD_FPHDF5_XFER_DUMPING_METADATA, &dumping) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

    /*
     * If metadata, write to the metadata cache but only if we're not
     * dumping the data from the SAP...
     */
    if (mem_type != H5FD_MEM_DRAW && !dumping) {
        unsigned req_id;
        H5FP_status_t sap_status;

        if (H5FP_request_write_metadata(_file, file->file_id, dxpl_id, mem_type,
                                        mpi_off, size_i, buf, &req_id,
                                        &sap_status) != SUCCEED) {
            /* FIXME: Couldn't write metadata. This is bad... */
HDfprintf(stderr, "%s:%d: Couldn't write metadata to SAP (%d)\n",
          FUNC, __LINE__, sap_status);
        }

        switch (sap_status) {
        case H5FP_STATUS_OK:
            /* WAH-HOO! We've written it! We can leave now */
            /* Forget the EOF value (see H5FD_fphdf5_get_eof()) */
            file->eof = HADDR_UNDEF;
            HGOTO_DONE(ret_value);
        case H5FP_STATUS_FILE_CLOSING:
            HGOTO_DONE(ret_value);
        case H5FP_STATUS_DUMPING_FAILED:
        case H5FP_STATUS_OOM:
        case H5FP_STATUS_BAD_FILE_ID:
        default:
            /* FIXME: Something bad happened */
HDfprintf(stderr, "%s: Couldn't write metadata to SAP (%d)\n",
          FUNC, sap_status);
            break;
        }
    }

    /* FIXME: Should I check this return value or just pass it on out? */
    ret_value = H5FD_fphdf5_write_real(_file, dxpl_id, mpi_off, size_i, buf);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_write_real
 * Purpose:     Do the actual writing to a file. Split apart from the
 *              H5FD_fphdf5_write call since I need to write things
 *              directly if the SAP is dumping data to me.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              12. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_fphdf5_write_real(H5FD_t *_file, hid_t dxpl_id, MPI_Offset mpi_off, int size,
                       const void *buf)
{
    H5FD_fphdf5_t      *file = (H5FD_fphdf5_t*)_file;
    MPI_Status          status;
    MPI_Datatype        buf_type;
    MPI_Datatype        file_type;
    MPI_Offset          mpi_disp;
    int                 mrc;
    int                 bytes_written;
    unsigned            use_view_this_time = 0;
    H5P_genplist_t     *plist;
    H5FD_mpio_xfer_t    xfer_mode = H5FD_MPIO_INDEPENDENT;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_write_real, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);
    assert(buf);

    /* Make certain we have the correct type of property list */
    assert(H5I_get_type(dxpl_id) == H5I_GENPROP_LST);
    assert(H5P_isa_class(dxpl_id, H5P_DATASET_XFER) == TRUE);

    /* Portably initialize MPI status variable */
    HDmemset(&status, 0, sizeof(MPI_Status));

    /* Obtain the data transfer properties */
    if ((plist = H5I_object(dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");

    /*
     * Set up for a fancy xfer using complex types, or single byte block.
     * We wouldn't need to rely on the use_view field if MPI semantics
     * allowed us to test that btype == ftype == MPI_BYTE (or even
     * MPI_TYPE_NULL, which could mean "use MPI_BYTE" by convention).
     */
    if (H5P_exist_plist(plist, H5FD_FPHDF5_XFER_USE_VIEW_NAME) > 0)
        if (H5P_get(plist, H5FD_FPHDF5_XFER_USE_VIEW_NAME, &use_view_this_time) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

    if (use_view_this_time) {
        /* Prepare for a full-blown xfer using btype, ftype, and disp */
        if (H5P_get(plist, H5FD_FPHDF5_XFER_MEM_MPI_TYPE_NAME, &buf_type) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

        if (H5P_get(plist, H5FD_FPHDF5_XFER_FILE_MPI_TYPE_NAME, &file_type) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI-I/O type property");

        /*
         * When using types, use the address as the displacement for
         * MPI_File_set_view and reset the address for the read to zero
         */
        mpi_disp = mpi_off;
        mpi_off = 0;

        /* Set the file view when we are using MPI derived types */
        if ((mrc = MPI_File_set_view(file->f, mpi_disp, MPI_BYTE,
                                     file_type, H5FD_mpio_native,
                                     file->info)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mrc);
    } else {
        /*
         * Prepare for a simple xfer of a contiguous block of bytes. The
         * btype, ftype, and disp fields are not used.
         */
        buf_type = MPI_BYTE;
        file_type = MPI_BYTE;
        mpi_disp = 0;   /* mpi_off is already set */
    }


    if (H5P_get_driver(plist) == H5FD_FPHDF5)
        /* Get the transfer mode */
        xfer_mode = H5P_peek_unsigned(plist, H5D_XFER_IO_XFER_MODE_NAME);
 
    /* Write the data. */
    assert(xfer_mode == H5FD_MPIO_INDEPENDENT || xfer_mode == H5FD_MPIO_COLLECTIVE);

    if (xfer_mode == H5FD_MPIO_INDEPENDENT) {
        /*OKAY: CAST DISCARDS CONST QUALIFIER*/
        if ((mrc = MPI_File_write_at(file->f, mpi_off, (void*)buf,
                                     size, buf_type, &status)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at failed", mrc);
    } else {
        /*OKAY: CAST DISCARDS CONST QUALIFIER*/
        if ((mrc = MPI_File_write_at_all(file->f, mpi_off, (void*)buf,
                                         size, buf_type, &status)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at_all failed", mrc);
    }

    /*
     * KLUDGE, Robb Matzke, 2000-12-29
     * The LAM implementation of MPI_Get_count() says
     *
     *    MPI_Get_count: invalid argument (rank 0, MPI_COMM_WORLD)
     *
     * So I'm commenting this out until it can be investigated. The
     * returned `bytes_written' isn't used anyway because of Kim's kludge
     * to avoid bytes_written<0. Likewise in H5FD_fphdf5_read().
     */

#ifdef H5_HAVE_MPI_GET_COUNT /* Bill and Albert's kludge*/
    /*
     * Yet Another KLUDGE, Albert Cheng & Bill Wendling, 2001-05-11.
     * Many systems don't support MPI_Get_count so we need to do a
     * configure thingy to fix this.
     */

    /*
     * Calling MPI_Get_count with "MPI_BYTE" is only valid when we
     * actually had the 'buf_type' set to MPI_BYTE -QAK
     */
    if (use_view_this_time) {
        /*
         * Figure out the mapping from the MPI 'buf_type' to bytes,
         * someday... If this gets fixed (and MPI_Get_count() is
         * reliable), the kludge below where the 'bytes_written' value
         * from MPI_Get_count() is overwritten with the 'size'
         * parameter can be removed. -QAK
         */
    } else {
        /* How many bytes were actually written? */
        if ((mrc = MPI_Get_count(&status, MPI_BYTE, &bytes_written)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_Get_count failed", mrc);
    }
#endif /* H5_HAVE_MPI_GET_COUNT */

    /* Reset the file view when we used MPI derived types */
    if (use_view_this_time)
        if ((mrc = MPI_File_set_view(file->f, (MPI_Offset)0, MPI_BYTE, MPI_BYTE,
                                     H5FD_mpio_native, file->info)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mrc);

    /*
     * MPI_Get_count incorrectly returns negative count; fake a complete
     * write (use size for both parameters).
     */
    bytes_written = size;

    /* Check for write failure */
    if (bytes_written < 0 || bytes_written > size)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file write failed");

    /* Forget the EOF value (see H5FD_fphdf5_get_eof()) */
    file->eof = HADDR_UNDEF;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_flush
 * Purpose:     Makes sure that all data is on disk. This is collective.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling
 *              12. February 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing)
{
    H5FD_fphdf5_t  *file = (H5FD_fphdf5_t*)_file;
    MPI_Offset      mpi_off;
    int	            mrc;
    unsigned        req_id;
    H5FP_status_t   status;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_fphdf5_flush, FAIL);

    /* check args */
    assert(file);
    assert(file->pub.driver_id == H5FD_FPHDF5);

    /*
     * Extend the file to make sure it's large enough, then sync.
     * Unfortunately, keeping track of EOF is an expensive operation, so
     * we can't just check whether EOF<EOA like with other drivers.
     * Therefore we'll just read the byte at EOA-1 and then write it
     * back.
     */
    if (file->eoa > file->last_eoa) {
        if (H5FD_fphdf5_haddr_to_MPIOff(file->eoa, &mpi_off) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL,
                        "cannot convert from haddr_t to MPI_Offset");

        /* Extend the file's size */
        if ((mrc = MPI_File_set_size(file->f, mpi_off)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_size failed", mrc);

        /*
         * Don't let any proc return until all have extended the file.
         * (Prevents race condition where some processes go ahead and
         * write more data to the file before all the processes have
         * finished making it the shorter length, potentially truncating
         * the file and dropping the new data written)
         */
        if ((mrc = MPI_Barrier(H5FP_SAP_BARRIER_COMM)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mrc);

        /* Update the 'last' eoa value */
        file->last_eoa = file->eoa;
    }

    /* Only the captain process needs to flush the metadata. */
    if (H5FD_fphdf5_is_captain(_file)) {
        if (H5FP_request_flush_metadata(_file, file->file_id, dxpl_id,
                                        &req_id, &status) != SUCCEED) {
            /* FIXME: This failed */
HDfprintf(stderr, "%s:%d: Flush failed (%d)\n", FUNC, __LINE__, status);
        }

        /* Only sync the file if we are not going to immediately close it */
        if (!closing)
            if ((mrc = MPI_File_sync(file->f)) != MPI_SUCCESS)
                HMPI_GOTO_ERROR(FAIL, "MPI_File_sync failed", mrc);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_MPIOff_to_haddr
 * Purpose:     Convert an MPI_Offset value to haddr_t.
 * Return:      Success:    The haddr_t equivalent of the MPI_OFF argument.
 *              Failure:    HADDR_UNDEF
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *------------------------------------------------------------------------- 
 */
static haddr_t
H5FD_fphdf5_MPIOff_to_haddr(MPI_Offset mpi_off)
{
    haddr_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5FD_fphdf5_MPIOff_to_haddr);
    ret_value = (mpi_off != (MPI_Offset)(haddr_t)mpi_off ? HADDR_UNDEF : (haddr_t)mpi_off);
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_fphdf5_haddr_to_MPIOff
 * Purpose:     Convert an haddr_t value to MPI_Offset.
 * Return:      Success:    Non-negative, the MPI_OFF argument contains
 *                          the converted value.
 *              Failure:    FAIL, MPI_OFF is undefined.
 * Programmer:  Bill Wendling
 *              30. January 2003
 * Modifications:
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_fphdf5_haddr_to_MPIOff(haddr_t addr, MPI_Offset *mpi_off)
{
    herr_t ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT(H5FD_fphdf5_haddr_to_MPIOff);

    if (mpi_off)
        *mpi_off = (MPI_Offset)addr;

    ret_value = (addr != (haddr_t)(MPI_Offset)addr ? FAIL : SUCCEED);
    FUNC_LEAVE_NOAPI(ret_value);
}

#endif  /* H5_HAVE_FPHDF5 */
