/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/*
 * This file contains public declarations for the H5D module.
 */
#ifndef _H5Dpublic_H
#define _H5Dpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/* Values for the H5D_LAYOUT property */
typedef enum H5D_layout_t {
    H5D_LAYOUT_ERROR    = -1,

    H5D_COMPACT         = 0,    /*raw data is very small                     */
    H5D_CONTIGUOUS      = 1,    /*the default                                */
    H5D_CHUNKED         = 2,    /*slow and fancy                             */

    H5D_NLAYOUTS        = 3     /*this one must be last!                     */
} H5D_layout_t;

#if defined(WANT_H5_V1_2_COMPAT) || defined(H5_WANT_H5_V1_2_COMPAT)
/* Values for the data transfer property */
typedef enum H5D_transfer_t {
    H5D_XFER_INDEPENDENT,       /*Independent data transfer                  */
    H5D_XFER_COLLECTIVE,        /*Collective data transfer                   */
    H5D_XFER_DFLT               /*default data transfer mode                 */
} H5D_transfer_t;
#endif /* WANT_H5_V1_2_COMPAT */

/* Define the operator function pointer for H5Diterate() */
typedef herr_t (*H5D_operator_t)(void *elem, hid_t type_id, hsize_t ndim,
                                 hssize_t *point, void *operator_data);

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ hid_t H5Dcreate (hid_t file_id, const char *name, hid_t type_id,
                         hid_t space_id, hid_t plist_id);
__DLL__ hid_t H5Dopen (hid_t file_id, const char *name);
__DLL__ herr_t H5Dclose (hid_t dset_id);
__DLL__ hid_t H5Dget_space (hid_t dset_id);
__DLL__ hid_t H5Dget_type (hid_t dset_id);
__DLL__ hid_t H5Dget_create_plist (hid_t dset_id);
__DLL__ hsize_t H5Dget_storage_size(hid_t dset_id);
__DLL__ herr_t H5Dread (hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
                        hid_t file_space_id, hid_t plist_id, void *buf/*out*/);
__DLL__ herr_t H5Dwrite (hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
                         hid_t file_space_id, hid_t plist_id, const void *buf);
__DLL__ herr_t H5Dextend (hid_t dset_id, const hsize_t *size);
__DLL__ herr_t H5Diterate(void *buf, hid_t type_id, hid_t space_id,
            H5D_operator_t op, void *operator_data);
__DLL__ herr_t H5Dvlen_reclaim(hid_t type_id, hid_t space_id, hid_t plist_id, void *buf);
__DLL__ herr_t H5Dvlen_get_buf_size(hid_t dataset_id, hid_t type_id, hid_t space_id, hsize_t *size);
__DLL__ herr_t H5Ddebug(hid_t dset_id, unsigned int flags);

#ifdef __cplusplus
}
#endif
#endif
