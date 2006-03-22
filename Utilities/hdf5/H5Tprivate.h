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
 * This file contains private information about the H5T module
 */
#ifndef _H5Tprivate_H
#define _H5Tprivate_H

#include "H5Tpublic.h"

/* Private headers needed by this file */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Gprivate.h"		/* Groups 			  	*/
#include "H5Rprivate.h"		/* References				*/

/* Forward references of package typedefs */
typedef struct H5T_t H5T_t;
typedef struct H5T_stats_t H5T_stats_t;
typedef struct H5T_path_t H5T_path_t;

/* How to copy a data type */
typedef enum H5T_copy_t {
    H5T_COPY_TRANSIENT,
    H5T_COPY_ALL,
    H5T_COPY_REOPEN
} H5T_copy_t;

/* Location of VL information */
typedef enum {
    H5T_VLEN_BADLOC =   0,  /* invalid VL Type */
    H5T_VLEN_MEMORY,        /* VL data stored in memory */
    H5T_VLEN_DISK,          /* VL data stored on disk */
    H5T_VLEN_MAXLOC         /* highest type (Invalid as true type) */
} H5T_vlen_loc_t;

/* Private functions */
H5_DLL herr_t H5TN_init_interface(void);
H5_DLL herr_t H5T_init(void);
H5_DLL htri_t H5T_isa(H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL H5T_t *H5T_open_oid(H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL H5T_t *H5T_copy(const H5T_t *old_dt, H5T_copy_t method);
H5_DLL herr_t H5T_lock(H5T_t *dt, hbool_t immutable);
H5_DLL herr_t H5T_close(H5T_t *dt);
H5_DLL H5T_class_t H5T_get_class(const H5T_t *dt);
H5_DLL htri_t H5T_detect_class (const H5T_t *dt, H5T_class_t cls);
H5_DLL size_t H5T_get_size(const H5T_t *dt);
H5_DLL int    H5T_cmp(const H5T_t *dt1, const H5T_t *dt2);
H5_DLL herr_t H5T_debug(const H5T_t *dt, FILE * stream);
H5_DLL H5G_entry_t *H5T_entof(H5T_t *dt);
H5_DLL htri_t H5T_is_immutable(H5T_t *dt);
H5_DLL htri_t H5T_is_named(H5T_t *dt);
H5_DLL H5T_path_t *H5T_path_find(const H5T_t *src, const H5T_t *dst,
				  const char *name, H5T_conv_t func, hid_t dxpl_id);
H5_DLL hbool_t H5T_path_noop(const H5T_path_t *p);
H5_DLL H5T_bkg_t H5T_path_bkg(const H5T_path_t *p);
H5_DLL herr_t H5T_convert(H5T_path_t *tpath, hid_t src_id, hid_t dst_id,
			   hsize_t nelmts, size_t buf_stride, size_t bkg_stride,
                           void *buf, void *bkg, hid_t dset_xfer_plist);
H5_DLL herr_t H5T_vlen_reclaim(void *elem, hid_t type_id, hsize_t ndim, hssize_t *point, void *_op_data);
H5_DLL htri_t H5T_vlen_mark(H5T_t *dt, H5F_t *f, H5T_vlen_loc_t loc);
H5_DLL htri_t H5T_is_sensible(const H5T_t *dt);
H5_DLL htri_t H5T_committed(H5T_t *type);
H5_DLL int H5T_link(const H5T_t *type, int adjust, hid_t dxpl_id);

/* Reference specific functions */
H5_DLL H5R_type_t H5T_get_ref_type(const H5T_t *dt);

#endif
