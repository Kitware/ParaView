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
H5_DLL hid_t   H5Acreate(hid_t loc_id, const char *name, hid_t type_id,
	 		 hid_t space_id, hid_t plist_id);
H5_DLL hid_t   H5Aopen_name(hid_t loc_id, const char *name);
H5_DLL hid_t   H5Aopen_idx(hid_t loc_id, unsigned idx);
H5_DLL herr_t  H5Awrite(hid_t attr_id, hid_t type_id, const void *buf);
H5_DLL herr_t  H5Aread(hid_t attr_id, hid_t type_id, void *buf);
H5_DLL herr_t  H5Aclose(hid_t attr_id);
H5_DLL hid_t   H5Aget_space(hid_t attr_id);
H5_DLL hid_t   H5Aget_type(hid_t attr_id);
H5_DLL ssize_t H5Aget_name(hid_t attr_id, size_t buf_size, char *buf);
H5_DLL hsize_t H5Aget_storage_size(hid_t attr_id);
H5_DLL int     H5Aget_num_attrs(hid_t loc_id);
H5_DLL herr_t  H5Arename(hid_t loc_id, const char *old_name, const char *new_name);
H5_DLL herr_t  H5Aiterate(hid_t loc_id, unsigned *attr_num, H5A_operator_t op,
	  	          void *op_data);
H5_DLL herr_t  H5Adelete(hid_t loc_id, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _H5Apublic_H */
