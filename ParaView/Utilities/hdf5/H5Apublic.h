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
 * This file contains public declarations for the H5A module.
 */
#ifndef _H5Apublic_H
#define _H5Apublic_H

/* Public headers needed by this file */
#include "H5Ipublic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef herr_t (*H5A_operator_t)(hid_t location_id/*in*/,
    const char *attr_name/*in*/, void *operator_data/*in,out*/);

/* Public function prototypes */
__DLL__ hid_t H5Acreate(hid_t loc_id, const char *name, hid_t type_id,
                        hid_t space_id, hid_t plist_id);
__DLL__ hid_t H5Aopen_name(hid_t loc_id, const char *name);
__DLL__ hid_t H5Aopen_idx(hid_t loc_id, unsigned idx);
__DLL__ herr_t H5Awrite(hid_t attr_id, hid_t type_id, const void *buf);
__DLL__ herr_t H5Aread(hid_t attr_id, hid_t type_id, void *buf);
__DLL__ herr_t H5Aclose(hid_t attr_id);
__DLL__ hid_t H5Aget_space(hid_t attr_id);
__DLL__ hid_t H5Aget_type(hid_t attr_id);
__DLL__ ssize_t H5Aget_name(hid_t attr_id, size_t buf_size, char *buf);
__DLL__ int H5Aget_num_attrs(hid_t loc_id);
__DLL__ herr_t H5Aiterate(hid_t loc_id, unsigned *attr_num, H5A_operator_t op,
                       void *op_data);
__DLL__ herr_t H5Adelete(hid_t loc_id, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _H5Apublic_H */
