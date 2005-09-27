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

#ifndef H5FDFPHDF5_H__
#define H5FDFPHDF5_H__

#include "H5FDmpio.h"
#include "H5FDpublic.h"         /* for the H5FD_t structure         */

#ifdef H5_HAVE_FPHDF5
#   define H5FD_FPHDF5      (H5FD_fphdf5_init())
#else
#   define H5FD_FPHDF5      (-1)
#endif  /* H5_HAVE_FPHDF5 */

/* Macros */

#ifndef H5_HAVE_FPHDF5

/* If FPHDF5 isn't specified, make this a "FALSE" value */
#define IS_H5FD_FPHDF5(f)   (0)

#else

#define IS_H5FD_FPHDF5(f)   (H5F_get_driver_id(f) == H5FD_FPHDF5)

/* Turn on H5FDfphdf5_debug if H5F_DEBUG is on */
#if defined(H5F_DEBUG) && !defined(H5FDfphdf5_DEBUG)
#   define H5FDfphdf5_DEBUG
#endif  /* H5F_DEBUG && ! H5FDfphdf5_DEBUG */

#define H5FD_FPHDF5_XFER_DUMPING_METADATA   "H5FD_fphdf5_dumping_metadata"
#define H5FD_FPHDF5_XFER_DUMPING_SIZE       sizeof(unsigned)

/*
 * For specifying that only the captain is allowed to allocate things at
 * this time.
 */
#define H5FD_FPHDF5_CAPTN_ALLOC_ONLY        "Only_Captain_Alloc"
#define H5FD_FPHDF5_CAPTN_ALLOC_SIZE        sizeof(unsigned)

/*
 * The description of a file belonging to this driver.
 *
 * The FILE_ID field is an SAP defined value. When reading/writing to the
 * SAP, this value should be sent.
 *
 * The EOF field is only used just after the file is opened in order for
 * the library to determine whether the file is empty, truncated, or
 * okay. The FPHDF5 driver doesn't bother to keep it updated since it's
 * an expensive operation.
 */
typedef struct H5FD_fphdf5_t {
    H5FD_t      pub;            /*Public stuff, must be first (ick!)    */
    unsigned    file_id;        /*ID used by the SAP                    */
    MPI_File    f;              /*MPIO file handle                      */
    MPI_Comm    comm;           /*Communicator                          */
    MPI_Comm    barrier_comm;   /*Barrier communicator                  */
    MPI_Info    info;           /*File information                      */
    int         mpi_rank;       /*This process's rank                   */
    int         mpi_size;       /*Total number of processes             */
    haddr_t     eof;            /*End-of-file marker                    */
    haddr_t     eoa;            /*End-of-address marker                 */
    haddr_t     last_eoa;       /*Last known end-of-address marker      */
} H5FD_fphdf5_t;

extern const H5FD_class_t H5FD_fphdf5_g;

/* Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 *==--------------------------------------------------------------------------==
 *                               API Functions
 *==--------------------------------------------------------------------------==
 */
H5_DLL herr_t   H5Pset_dxpl_fphdf5(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode);
H5_DLL herr_t   H5Pget_dxpl_fphdf5(hid_t dxpl_id, H5FD_mpio_xfer_t *xfer_mode);
H5_DLL herr_t   H5Pset_fapl_fphdf5(hid_t fapl_id, MPI_Comm comm,
                                   MPI_Comm barrier_comm, MPI_Info info,
                                   unsigned sap_rank);
H5_DLL herr_t   H5Pget_fapl_fphdf5(hid_t fapl_id, MPI_Comm *comm,
                                   MPI_Comm *barrier_comm, MPI_Info *info,
                                   unsigned *sap_rank, unsigned *capt_rank);

/*
 *==--------------------------------------------------------------------------==
 *                         Private Library Functions
 *==--------------------------------------------------------------------------==
 */
H5_DLL hid_t    H5FD_fphdf5_init(void);
H5_DLL MPI_Comm H5FD_fphdf5_communicator(H5FD_t *_file);
H5_DLL MPI_Comm H5FD_fphdf5_barrier_communicator(H5FD_t *_file);
H5_DLL herr_t   H5FD_fphdf5_setup(hid_t dxpl_id, MPI_Datatype btype,
                                  MPI_Datatype ftype, unsigned use_view);
H5_DLL herr_t   H5FD_fphdf5_teardown(hid_t dxpl_id);
H5_DLL int      H5FD_fphdf5_mpi_rank(H5FD_t *_file);
H5_DLL int      H5FD_fphdf5_mpi_size(H5FD_t *_file);
H5_DLL unsigned H5FD_fphdf5_file_id(H5FD_t *_file);
H5_DLL hbool_t  H5FD_fphdf5_is_sap(H5FD_t *_file);
H5_DLL hbool_t  H5FD_fphdf5_is_captain(H5FD_t *_file);
H5_DLL hbool_t  H5FD_is_fphdf5_driver(H5FD_t *_file);

H5_DLL herr_t   H5FD_fphdf5_write_real(H5FD_t *_file, hid_t dxpl_id,
                                       MPI_Offset mpi_off, int size,
                                       const void *buf);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* H5_HAVE_PARALLEL */

#endif  /* H5FDFPHDF5_H__ */
