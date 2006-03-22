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

#ifndef H5FPPRIVATE_H__
#define H5FPPRIVATE_H__ 0

#ifdef H5_HAVE_FPHDF5

#include "H5ACpublic.h"         /* Metadata Cache                       */
#include "H5FPpublic.h"         /* Flexible Parallel HDF5               */
#include "H5Oprivate.h"         /* Object Headers                       */
#include "H5Rprivate.h"         /* References                           */

/*===----------------------------------------------------------------------===
 *                             Request Types
 *===----------------------------------------------------------------------===
 *
 * The H5FP_REQ_LOCK_END and H5FP_REQ_RELEASE_END are used to lock and
 * release a collection of objects at the same time. The last object you
 * need to lock is sent with the H5FP_REQ_LOCK_END request type (this can
 * also be a null message - where you're just sending a message that has
 * H5FP_REQ_LOCK_END as the request type with no data associated with it.
 * In that case, the md_size *MUST* be set to 0). The SAP will then try
 * to lock all of the objects you've requested to lock at once. If it
 * cannot do so, then it will fail and you have to try again at a later
 * time.
 *
 * Releasing locks is done in the exact same way, except that the action
 * will always release locks (i.e., not fail) if there is a vaild lock
 * for that object.
 */
typedef enum {
    /* Metadata Requests */
    H5FP_REQ_OPEN,              /* Open a file (or eventually an object)    */
    H5FP_REQ_LOCK,              /* Lock an object (in a sequence)           */
    H5FP_REQ_LOCK_END,          /* Last lock request in lock sequence       */
    H5FP_REQ_RELEASE,           /* Unlock an object (in a sequence)         */
    H5FP_REQ_RELEASE_END,       /* Last unlock request in unlock sequence   */
    H5FP_REQ_WRITE,             /* Writing a piece of metadata              */
    H5FP_REQ_READ,              /* Reading a piece of metadata              */
    H5FP_REQ_FLUSH,             /* Flush the metadata out to disk           */
    H5FP_REQ_CLOSE,             /* Close a file (or eventually an object)   */
    H5FP_REQ_STOP,              /* Stop SAP                                 */

    /* Allocation Requests */
    H5FP_REQ_ALLOC,             /* Allocate a region of metadata            */
    H5FP_REQ_FREE               /* Free a region of metadata                */
} H5FP_req_t;

/*===----------------------------------------------------------------------===
 *                              Lock Types
 *===----------------------------------------------------------------------===
 *
 * A ``read'' lock indicates that the process is busy reading the
 * metadata of that object. It's non-exclusive, so any number of
 * processes can have any number of locks on a given object. However, you
 * cannot have a write and a read lock on an object.
 *
 * A ``write'' lock indicates that the process is busy writing to the
 * metadata of that object. It's exclusive, so only one process can have
 * a write lock on an object at any one time. However, that object can
 * have any number of write locks on that object.
 *
 * It's up to the program to release all of the locks it has on a given
 * object.
 */
typedef enum {
    H5FP_LOCK_READ,
    H5FP_LOCK_WRITE
} H5FP_lock_t;

/*===----------------------------------------------------------------------===
 *                             Object Types
 *===----------------------------------------------------------------------===
 *
 * The various types of objects we're able to get a lock on or which we
 * want to modify/read.
 */
typedef enum {
    H5FP_OBJ_FILE,
    H5FP_OBJ_GROUP,
    H5FP_OBJ_DATASET,
    H5FP_OBJ_DATATYPE,
    H5FP_OBJ_ATTRIBUTE,
    H5FP_OBJ_MEMORY
} H5FP_obj_t;

/*===----------------------------------------------------------------------===
 *                            MPI Message Tags
 *===----------------------------------------------------------------------===
 *
 * Special tag numbers for requests, replies, and string passing
 * messages.
 *
 * Certain actions (Open, Change, and Close) require a pathname to the
 * object. This pathname is sent in a separate message and the SAP will
 * search for it after getting the appropriate request.
 */
enum {
    H5FP_TAG_REQUEST,
    H5FP_TAG_REPLY,
    H5FP_TAG_READ,
    H5FP_TAG_METADATA,
    H5FP_TAG_ALLOC,
    H5FP_TAG_DUMP,
    H5FP_TAG_FILE_ID
};

/*===----------------------------------------------------------------------===
 *                                Status
 *===----------------------------------------------------------------------===
 *
 * The status returned by the SAP. If the process receives an
 * H5FP_STATUS_CATASTROPHIC status, then something *REALLY* bad happened
 * on the set-aside process. The state of the program is then
 * indeterminant and the only real course of action is for the program to
 * abort operation.
 */
typedef enum sap_status {
    H5FP_STATUS_OK,

    /* For locking */
    H5FP_STATUS_LOCK_ACQUIRED,
    H5FP_STATUS_LOCK_FAILED,

    /* For releasing locks */
    H5FP_STATUS_LOCK_RELEASED,
    H5FP_STATUS_LOCK_RELEASE_FAILED,
    H5FP_STATUS_BAD_LOCK,       /* Process doesn't own a lock on the OID    */

    /* For change requests */
    H5FP_STATUS_FILE_CLOSING,
    H5FP_STATUS_NO_LOCK,

    /* For read requests */
    H5FP_STATUS_MDATA_NOT_CACHED,

    /* For dumping data to client */
    H5FP_STATUS_DUMPING,
    H5FP_STATUS_DUMPING_FINISHED,
    H5FP_STATUS_DUMPING_FAILED,

    /* Out of memory error */
    H5FP_STATUS_OOM,

    /* Bad file ID */
    H5FP_STATUS_BAD_FILE_ID,

    /* Can't do memory things */
    H5FP_STATUS_CANT_ALLOC,
    H5FP_STATUS_CANT_FREE,

    /* Reserved for completely disasterous failures which require an abort */
    H5FP_STATUS_CATASTROPHIC
} H5FP_status_t;

/*===----------------------------------------------------------------------===
 *                              H5FP_request
 *===----------------------------------------------------------------------===
 *
 * The structure sent to the SAP which holds all of the requested action
 */
typedef struct {
    unsigned        req_id;     /* ID for request set by sending process    */
    H5FP_req_t      req_type;   /* Request type                             */
    unsigned        proc_rank;  /* Rank of sending process                  */
    unsigned        file_id;    /* SAP's file ID for the specific file      */
    H5FP_obj_t      obj_type;   /* Type of the object                       */
    H5FP_lock_t     rw_lock;    /* Indicates read or write lock             */
    H5FD_mem_t      mem_type;   /* Type of memory updated, if req'd         */
    unsigned        md_size;    /* Size of the metadata sent in next msg    */
    unsigned long   feature_flags; /* Feature flags for the file driver     */
    hsize_t         meta_block_size; /* Metadata block size                 */
    hsize_t         sdata_block_size; /* Small data block size              */
    hsize_t         threshold;  /* Alignment threshold                      */
    hsize_t         alignment;  /* Alignment (really!)                      */
    haddr_t         addr;       /* Address of the metadata                  */
    unsigned char   oid[H5R_OBJ_REF_BUF_SIZE]; /* Buffer to store OID of object */
} H5FP_request_t;

extern MPI_Datatype H5FP_request; /* MPI datatype for the H5FP_request obj  */

/*===----------------------------------------------------------------------===
 *                               H5FP_reply
 *===----------------------------------------------------------------------===
 *
 * Reply from the SAP on an H5FP_request send
 */
typedef struct {
    unsigned        req_id;     /* Request ID copied from the SAP_request   */
    unsigned        file_id;    /* File ID assigned to an open file         */
    H5FP_status_t   status;     /* Status of the request                    */
    unsigned        md_size;    /* Size of the metadata sent in next msg    */
} H5FP_reply_t;

extern MPI_Datatype H5FP_reply; /* MPI datatype for the H5FP_reply obj      */

/*===----------------------------------------------------------------------===
 *                               H5FP_read
 *===----------------------------------------------------------------------===
 *
 * The reply message from the SAP on an H5FP_request H5FP_REQ_READ send
 */
typedef struct {
    unsigned        req_id;     /* Request ID copied from the SAP_request   */
    unsigned        file_id;    /* SAP's file ID for the specific file      */
    H5FP_status_t   status;     /* Status of the request                    */
    H5FD_mem_t      mem_type;   /* Type of memory updated, if req'd         */
    unsigned        md_size;    /* Size of the metadata sent in next msg    */
    MPI_Offset      addr;       /* Address of the metadata                  */
} H5FP_read_t;

extern MPI_Datatype H5FP_read;  /* MPI datatype for the H5FP_read obj       */

/*===----------------------------------------------------------------------===
 *                              H5FP_alloc
 *===----------------------------------------------------------------------===
 *
 * The reply message from the SAP on an H5FP_alloc H5FP_REQ_ALLOC send.
 */
typedef struct {
    unsigned        req_id;     /* Request ID copied from the SAP_request   */
    unsigned        file_id;    /* SAP's file ID for the specific file      */
    H5FP_status_t   status;     /* Status of the request                    */
    H5FD_mem_t      mem_type;   /* Type of memory updated, if req'd         */
    haddr_t         addr;       /* Address of the metadata                  */
    haddr_t         eoa;        /* End of address space                     */
} H5FP_alloc_t;

extern MPI_Datatype H5FP_alloc; /* MPI datatype for the H5FP_alloc obj      */

/* Handy #define for copying OIDs */
#define H5FP_COPY_OID(dst, src)     HDmemcpy((dst), (src), H5R_OBJ_REF_BUF_SIZE)

/* SAP specific variables */
extern MPI_Comm H5FP_SAP_COMM;  /* Comm we use: Supplied by user            */
extern MPI_Comm H5FP_SAP_BARRIER_COMM; /* Comm if you want to do a barrier  */

extern unsigned H5FP_sap_rank;  /* The rank of the SAP: Supplied by user    */
extern unsigned H5FP_capt_rank; /* The rank which tells SAP of opens        */
extern unsigned H5FP_capt_barrier_rank;/* Rank of captain in barrier comm   */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Start the SAP */
extern herr_t H5FP_sap_receive_loop(void);

/* Use these functions to communicate with the SAP */
extern herr_t H5FP_request_open(H5FP_obj_t obj_type,
                                haddr_t maxaddr,
                                unsigned long feature_flags,
                                hsize_t meta_block_size,
                                hsize_t sdata_block_size,
                                hsize_t threshold,
                                hsize_t alignment,
                                unsigned *file_id,
                                unsigned *req_id);
extern herr_t H5FP_request_lock(unsigned sap_file_id, unsigned char *mdata,
                                H5FP_lock_t rw_lock, int last, unsigned *req_id,
                                H5FP_status_t *status);
extern herr_t H5FP_request_release_lock(unsigned sap_file_id, unsigned char *mdata,
                                        int last, unsigned *req_id,
                                        H5FP_status_t *status);
extern herr_t H5FP_request_read_metadata(H5FD_t *file, unsigned sap_file_id, hid_t dxpl_id,
                                         H5FD_mem_t mem_type, MPI_Offset addr,
                                         size_t size, uint8_t **buf, int *bytes_read,
                                         unsigned *req_id, H5FP_status_t *status);
extern herr_t H5FP_request_write_metadata(H5FD_t *file, unsigned file_id, hid_t dxpl_id,
                                          H5FD_mem_t mem_type, MPI_Offset addr,
                                          int mdata_size, const char *mdata,
                                          unsigned *req_id, H5FP_status_t *status);
extern herr_t H5FP_request_flush_metadata(H5FD_t *file, unsigned file_id,
                                          hid_t dxpl_id, unsigned *req_id,
                                          H5FP_status_t *status);
extern herr_t H5FP_request_close(H5FD_t *file, unsigned sap_file_id,
                                 unsigned *req_id, H5FP_status_t *status);
extern herr_t H5FP_request_allocate(H5FD_t *file, H5FD_mem_t mem_type,
                                    hsize_t size, haddr_t *addr,
                                    haddr_t *eoa, unsigned *req_id,
                                    H5FP_status_t *status);
extern herr_t H5FP_request_free(H5FD_t *file, unsigned *req_id, H5FP_status_t *status);

/* NOTE: Don't use these functions outside of the H5FP* modules! */
extern herr_t H5FP_send_metadata(const char *mdata, int len, int to);
extern herr_t H5FP_read_metadata(char **mdata, int len, int from);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* H5_HAVE_FPHDF5 */

#endif  /* H5FPPRIVATE_H__ */
