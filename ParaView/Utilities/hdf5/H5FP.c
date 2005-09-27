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

/* Private header files */
#include "H5private.h"          /* Generic Functions                    */
#include "H5Eprivate.h"         /* Error Handling                       */
#include "H5Oprivate.h"         /* Object Headers                       */
#include "H5TBprivate.h"        /* Threaded, Balanced, Binary Trees     */

#ifdef H5_HAVE_FPHDF5

#include "H5FPprivate.h"        /* Flexible Parallel Functions          */

/* Pablo mask */
#define PABLO_MASK          H5FP_mask

/* Interface initialization */
#define INTERFACE_INIT      NULL
static int interface_initialize_g = 0;

MPI_Datatype H5FP_request;      /* MPI datatype for the H5FP_request_t type */
MPI_Datatype H5FP_reply;        /* MPI datatype for the H5FP_reply_t type   */
MPI_Datatype H5FP_read;         /* MPI datatype for the H5FP_read_t type    */
MPI_Datatype H5FP_alloc;        /* MPI datatype for the H5FP_alloc_t type   */

/* SAP specific variables */
MPI_Comm H5FP_SAP_COMM;         /* Comm we use: Supplied by user        */
MPI_Comm H5FP_SAP_BARRIER_COMM; /* Comm if you want to do a barrier     */

unsigned H5FP_sap_rank;         /* The rank of the SAP: Supplied by user*/
unsigned H5FP_capt_rank;        /* The rank which tells SAP of opens    */
unsigned H5FP_capt_barrier_rank;/* Rank of captain in barrier comm      */

/* local functions */
static herr_t H5FP_commit_sap_datatypes(void);
static herr_t H5FP_request_sap_stop(void);

/*
 *===----------------------------------------------------------------------===
 *                          Public (API) Functions
 *===----------------------------------------------------------------------===
 */

/*
 * Function:    H5FPinit
 * Purpose:     Initialize the SAP environment: duplicate the COMM the user
 *              supplies to us, set aside the SAP_RANK as the SAP.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling, 26. July, 2002
 * Modifications:
 */
herr_t
H5FPinit(MPI_Comm comm, int sap_rank, MPI_Comm *sap_comm, MPI_Comm *sap_barrier_comm)
{
    MPI_Group sap_group = MPI_GROUP_NULL, sap_barrier_group = MPI_GROUP_NULL;
    int mrc, comm_size, my_rank;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(H5FPinit, FAIL);
    H5TRACE4("e","McIs*Mc*Mc",comm,sap_rank,sap_comm,sap_barrier_comm);

    /* initialize to NULL so that we can release if an error occurs */
    H5FP_request = MPI_DATATYPE_NULL;
    H5FP_reply = MPI_DATATYPE_NULL;
    H5FP_read = MPI_DATATYPE_NULL;
    H5FP_alloc = MPI_DATATYPE_NULL;

    *sap_comm = H5FP_SAP_COMM = MPI_COMM_NULL;
    *sap_barrier_comm = H5FP_SAP_BARRIER_COMM = MPI_COMM_NULL;

    /* Set the global variable to track the SAP's rank */
    H5FP_sap_rank = sap_rank;

    /* Make a private copy of the communicator passed to us */
    if (MPI_Comm_dup(comm, &H5FP_SAP_COMM) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_dup failed");

    *sap_comm = H5FP_SAP_COMM;

    if (MPI_Comm_group(H5FP_SAP_COMM, &sap_group) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_group failed");

    /* Exclude the SAP from the barrier group group */
    if (MPI_Group_excl(sap_group, 1, (int *)&H5FP_sap_rank,
                       &sap_barrier_group) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Group_excl failed");

    /* Create communicator for barrier group (all processes except the SAP) */
    if (MPI_Comm_create(H5FP_SAP_COMM, sap_barrier_group,
                        &H5FP_SAP_BARRIER_COMM) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_create failed");

    *sap_barrier_comm = H5FP_SAP_BARRIER_COMM;

    /* Get the size of all the processes (including the SAP) */
    if (MPI_Comm_size(H5FP_SAP_COMM, &comm_size) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_size failed");

    /*
     * We assign the process right after the sap_rank as the one which
     * will tell the SAP that files have been opened or closed. We mod
     * it so that we don't go over the size of the communicator.
     */
    H5FP_capt_rank = (H5FP_sap_rank + 1) % comm_size;

    /* Get this processes rank */
    if ((mrc = MPI_Comm_rank(H5FP_SAP_COMM, (int *)&my_rank)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mrc);

    /* Get the rank of the captain in the barrier Comm */
    if (H5FP_capt_rank == (unsigned)my_rank)
        if ((mrc = MPI_Comm_rank(H5FP_SAP_BARRIER_COMM,
                                 (int *)&H5FP_capt_barrier_rank)) != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mrc);

    /* Broadcast the captain's barrier rank */
    if ((mrc = MPI_Bcast(&H5FP_capt_barrier_rank, 1, MPI_UNSIGNED,
                         (int)H5FP_capt_rank,
                         H5FP_SAP_COMM)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mrc);

    /* Create the MPI types used for communicating with the SAP */
    if (H5FP_commit_sap_datatypes() != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "H5FP_commit_sap_datatypes failed");

    /* Go loop, if we are the SAP */
    if ((unsigned)my_rank == H5FP_sap_rank)
        H5FP_sap_receive_loop();

    /* Fall through and return to user, if not SAP */

done:
    if (ret_value == FAIL) {
        /* we've encountered an error...clean up */
        if (H5FP_request != MPI_DATATYPE_NULL)
            if (MPI_Type_free(&H5FP_request) != MPI_SUCCESS)
                HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

        if (H5FP_reply != MPI_DATATYPE_NULL)
            if (MPI_Type_free(&H5FP_reply) != MPI_SUCCESS)
                HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

        if (H5FP_read != MPI_DATATYPE_NULL)
            if (MPI_Type_free(&H5FP_read) != MPI_SUCCESS)
                HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

        if (H5FP_alloc != MPI_DATATYPE_NULL)
            if (MPI_Type_free(&H5FP_alloc) != MPI_SUCCESS)
                HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

        if (H5FP_SAP_BARRIER_COMM != MPI_COMM_NULL)
            /* this comm will be NULL for the SAP */
            if (MPI_Comm_free(&H5FP_SAP_BARRIER_COMM) != MPI_SUCCESS)
                HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_free failed");

        if (H5FP_SAP_COMM != MPI_COMM_NULL)
            if (MPI_Comm_free(&H5FP_SAP_COMM) != MPI_SUCCESS)
                HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_free failed");
    }

    if (sap_group != MPI_GROUP_NULL)
        MPI_Group_free(&sap_group);

    if (sap_barrier_group != MPI_GROUP_NULL)
        MPI_Group_free(&sap_barrier_group);

    FUNC_LEAVE_API(ret_value);
}

/*
 * Function:    H5FPfinalize
 * Purpose:     Get rid of the initilized environment we setup with H5FPinit.
 *              Mostly just freeing the duplicated COMM object and committed
 *              datatypes.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling, 26. July, 2002
 * Modifications:
 */
herr_t
H5FPfinalize(void)
{
    int mrc, my_rank;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(H5FPfinalize, FAIL);
    H5TRACE0("e","");

    /* Get this processes rank */
    if ((mrc = MPI_Comm_rank(H5FP_SAP_COMM, (int *)&my_rank)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mrc);

    /* Stop the SAP */
    if ((unsigned)my_rank != H5FP_sap_rank)
        if (H5FP_request_sap_stop() < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Error stopping the SAP");

    /* Release the MPI types we created */
    if (MPI_Type_free(&H5FP_request) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

    if (MPI_Type_free(&H5FP_reply) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

    if (MPI_Type_free(&H5FP_read) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

    if (MPI_Type_free(&H5FP_alloc) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_free failed");

    /* Release the barrier communicator */
    if (H5FP_SAP_BARRIER_COMM != MPI_COMM_NULL)
        /* this comm will be NULL for the SAP */
        if (MPI_Comm_free(&H5FP_SAP_BARRIER_COMM) != MPI_SUCCESS)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_free failed");

    /* Release the FPH5 communicator */
    if (MPI_Comm_free(&H5FP_SAP_COMM) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Comm_free failed");

done:
    FUNC_LEAVE_API(ret_value);
}

/*
 *===----------------------------------------------------------------------===
 *                    Public Library (non-API) Functions
 *===----------------------------------------------------------------------===
 */

/*
 * Function:    H5FP_send_metadata
 * Purpose:     Send a string of metadata to a process.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling, 30. July, 2002
 * Modifications:
 */
herr_t
H5FP_send_metadata(const char *mdata, int len, int to)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FP_send_metadata, FAIL);

    assert(mdata);
    assert(len);

    /* casts the CONST away: Okay */
    if (MPI_Send((void *)mdata, len, MPI_BYTE, to, H5FP_TAG_METADATA, H5FP_SAP_COMM)
            != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Send failed");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*
 * Function:    H5FP_read_metadata
 * Purpose:     Read a string of metadata from process FROM.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling, 31. January, 2003
 * Modifications:
 */
herr_t
H5FP_read_metadata(char **mdata, int len, int from)
{
    MPI_Status status;
    herr_t ret_value = SUCCEED;
    int mrc;

    FUNC_ENTER_NOAPI(H5FP_read_metadata, FAIL);

    /* check args */
    assert(mdata);

    /*
     * There is metadata associated with this request. Get it as a
     * string (requires another read).
     */
    if ((*mdata = (char *)H5MM_malloc((size_t)len + 1)) == NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "out of memory");

    HDmemset(*mdata, 0, (size_t)len + 1);

    if ((mrc = MPI_Recv(*mdata, len + 1, MPI_BYTE, from, H5FP_TAG_METADATA,
                        H5FP_SAP_COMM, &status)) != MPI_SUCCESS) {
        HDfree(*mdata);
        *mdata = NULL;
        HMPI_GOTO_ERROR(FAIL, "MPI_Recv failed", mrc);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*
 *===----------------------------------------------------------------------===
 *                            Private Functions
 *===----------------------------------------------------------------------===
 */

/*
 * Function:    H5FP_commit_sap_datatypes
 * Purpose:     Commit the H5FP_request, H5FP_reply, H5FP_read, and
 *              H5FP_alloc structure datatypes to MPI.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling, 26. July, 2002
 * Modifications:
 */
static herr_t
H5FP_commit_sap_datatypes(void)
{
    int             block_length[5];
    int             i;
    MPI_Aint        displs[5];
    MPI_Datatype    old_types[5];
    H5FP_request_t  sap_req;
    H5FP_reply_t    sap_reply;
    H5FP_read_t     sap_read;
    H5FP_alloc_t    sap_alloc;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FP_commit_sap_datatypes, FAIL);

    /* Commit the H5FP_request datatype */
    block_length[0] = 8;
    block_length[1] = 1;
    block_length[2] = 4;
    block_length[3] = 1;
    block_length[4] = sizeof(sap_req.oid);
    old_types[0] = MPI_UNSIGNED;
    old_types[1] = MPI_UNSIGNED_LONG;
    old_types[2] = MPI_LONG_LONG_INT;
    old_types[3] = HADDR_AS_MPI_TYPE;
    old_types[4] = MPI_UNSIGNED_CHAR;
    MPI_Address(&sap_req.req_id, &displs[0]);
    MPI_Address(&sap_req.feature_flags, &displs[1]);
    MPI_Address(&sap_req.meta_block_size, &displs[2]);
    MPI_Address(&sap_req.addr, &displs[3]);
    MPI_Address(&sap_req.oid, &displs[4]);

    /* Calculate the displacements */
    for (i = 4; i >= 0; --i)
        displs[i] -= displs[0];

    if (MPI_Type_struct(5, block_length, displs, old_types, &H5FP_request) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_struct failed");

    if (MPI_Type_commit(&H5FP_request) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_commit failed");

    /* Commit the H5FP_reply datatype */
    block_length[0] = 4;
    old_types[0] = MPI_INT;
    MPI_Address(&sap_reply.req_id, &displs[0]);

    /* Calculate the displacements */
    for (i = 0; i >= 0; --i)
        displs[i] -= displs[0];

    if (MPI_Type_struct(1, block_length, displs, old_types, &H5FP_reply) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_struct failed");

    if (MPI_Type_commit(&H5FP_reply) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_commit failed");

    /* Commit the H5FP_read datatype */
    block_length[0] = 5;
    block_length[1] = 1;
    old_types[0] = MPI_UNSIGNED;
    old_types[1] = HADDR_AS_MPI_TYPE;
    MPI_Address(&sap_read.req_id, &displs[0]);
    MPI_Address(&sap_read.addr, &displs[1]);

    /* Calculate the displacements */
    for (i = 1; i >= 0; --i)
        displs[i] -= displs[0];

    if (MPI_Type_struct(2, block_length, displs, old_types, &H5FP_read) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_struct failed");

    if (MPI_Type_commit(&H5FP_read) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_commit failed");

    /* Commit the H5FP_alloc datatype */
    block_length[0] = 4;
    block_length[1] = 2;
    old_types[0] = MPI_UNSIGNED;
    old_types[1] = HADDR_AS_MPI_TYPE;
    MPI_Address(&sap_alloc.req_id, &displs[0]);
    MPI_Address(&sap_alloc.addr, &displs[1]);

    /* Calculate the displacements */
    for (i = 1; i >= 0; --i)
        displs[i] -= displs[0];

    if (MPI_Type_struct(2, block_length, displs, old_types, &H5FP_alloc) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_struct failed");

    if (MPI_Type_commit(&H5FP_alloc) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Type_commit failed");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*
 * Function:    H5FP_request_sap_stop
 * Purpose:     Request that the SAP stop it's loop processing. Each
 *              process should send this to the SAP.
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 * Programmer:  Bill Wendling, 02. August, 2002
 * Modifications:
 */
static herr_t
H5FP_request_sap_stop(void)
{
    H5FP_request_t req;
    int mrc, my_rank;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FP_request_sap_stop, FAIL);

    /* Get this processes rank */
    if ((mrc = MPI_Comm_rank(H5FP_SAP_COMM, (int *)&my_rank)) != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mrc);

    HDmemset(&req, 0, sizeof(req));
    req.req_type = H5FP_REQ_STOP;
    req.req_id = 0;
    req.proc_rank = my_rank;

    if (MPI_Send(&req, 1, H5FP_request, (int)H5FP_sap_rank,
                 H5FP_TAG_REQUEST, H5FP_SAP_COMM) != MPI_SUCCESS)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Send failed");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

#endif  /* H5_HAVE_FPHDF5 */
