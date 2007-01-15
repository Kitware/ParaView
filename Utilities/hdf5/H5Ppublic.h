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
 * This file contains function prototypes for each exported function in the
 * H5P module.
 */
#ifndef _H5Ppublic_H
#define _H5Ppublic_H

/* Default Template for creation, access, etc. templates */
#define H5P_DEFAULT     0

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"
#include "H5Dpublic.h"
#include "H5Fpublic.h"
#include "H5FDpublic.h"
#include "H5MMpublic.h"
#include "H5Zpublic.h"

/* Metroworks <sys/types.h> doesn't define off_t. */
#ifdef __MWERKS__
typedef long off_t;
/* Metroworks does not define EINTR in <errno.h> */
# define EINTR 4
#endif
/*__MWERKS__*/

#ifdef H5_WANT_H5_V1_4_COMPAT
/* Backward compatibility typedef... */
typedef hid_t H5P_class_t;      /* Alias H5P_class_t to hid_t */

/* H5P_DATASET_XFER was the name from the beginning through 1.2.  It was
 * changed to H5P_DATA_XFER on v1.3.0.  Then it was changed back to
 * H5P_DATASET_XFER right before the release of v1.4.0-beta2.
 * Define an alias here to help applications that had ported to v1.3.
 * Should be removed in later version.
 */
#define H5P_DATA_XFER H5P_DATASET_XFER
#endif /* H5_WANT_H5_V1_4_COMPAT */

#ifdef __cplusplus
extern "C" {
#endif

/* Define property list class callback function pointer types */
typedef herr_t (*H5P_cls_create_func_t)(hid_t prop_id, void *create_data);
typedef herr_t (*H5P_cls_copy_func_t)(hid_t new_prop_id, hid_t old_prop_id,
                                      void *copy_data);
typedef herr_t (*H5P_cls_close_func_t)(hid_t prop_id, void *close_data);

/* Define property list callback function pointer types */
typedef herr_t (*H5P_prp_cb1_t)(const char *name, size_t size, void *value);
typedef herr_t (*H5P_prp_cb2_t)(hid_t prop_id, const char *name, size_t size, void *value);
typedef H5P_prp_cb1_t H5P_prp_create_func_t;
typedef H5P_prp_cb2_t H5P_prp_set_func_t;
typedef H5P_prp_cb2_t H5P_prp_get_func_t;
typedef H5P_prp_cb2_t H5P_prp_delete_func_t;
typedef H5P_prp_cb1_t H5P_prp_copy_func_t;
typedef int (*H5P_prp_compare_func_t)(const void *value1, const void *value2, size_t size);
typedef H5P_prp_cb1_t H5P_prp_close_func_t;

/* Define property list iteration function type */
typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);

/*
 * The library created property list classes
 *
 * NOTE: When adding H5P_* macros, remember to redefine them in H5Pprivate.h
 *
 */

/* When this header is included from a private header, don't make calls to H5open() */
#undef H5OPEN
#ifndef _H5private_H
#define H5OPEN        H5open(),
#else   /* _H5private_H */
#define H5OPEN
#endif  /* _H5private_H */

#define H5P_NO_CLASS         (H5OPEN H5P_CLS_NO_CLASS_g)
#define H5P_FILE_CREATE     (H5OPEN H5P_CLS_FILE_CREATE_g)
#define H5P_FILE_ACCESS     (H5OPEN H5P_CLS_FILE_ACCESS_g)
#define H5P_DATASET_CREATE         (H5OPEN H5P_CLS_DATASET_CREATE_g)
#define H5P_DATASET_XFER           (H5OPEN H5P_CLS_DATASET_XFER_g)
#define H5P_MOUNT           (H5OPEN H5P_CLS_MOUNT_g)
H5_DLLVAR hid_t H5P_CLS_NO_CLASS_g;
H5_DLLVAR hid_t H5P_CLS_FILE_CREATE_g;
H5_DLLVAR hid_t H5P_CLS_FILE_ACCESS_g;
H5_DLLVAR hid_t H5P_CLS_DATASET_CREATE_g;
H5_DLLVAR hid_t H5P_CLS_DATASET_XFER_g;
H5_DLLVAR hid_t H5P_CLS_MOUNT_g;

/*
 * The library created default property lists
 *
 * NOTE: When adding H5P_* macros, remember to redefine them in H5Pprivate.h
 *
 */
#define H5P_NO_CLASS_DEFAULT       (H5OPEN H5P_LST_NO_CLASS_g)
#define H5P_FILE_CREATE_DEFAULT    (H5OPEN H5P_LST_FILE_CREATE_g)
#define H5P_FILE_ACCESS_DEFAULT   (H5OPEN H5P_LST_FILE_ACCESS_g)
#define H5P_DATASET_CREATE_DEFAULT    (H5OPEN H5P_LST_DATASET_CREATE_g)
#define H5P_DATASET_XFER_DEFAULT     (H5OPEN H5P_LST_DATASET_XFER_g)
#define H5P_MOUNT_DEFAULT         (H5OPEN H5P_LST_MOUNT_g)
H5_DLLVAR hid_t H5P_LST_NO_CLASS_g;
H5_DLLVAR hid_t H5P_LST_FILE_CREATE_g;
H5_DLLVAR hid_t H5P_LST_FILE_ACCESS_g;
H5_DLLVAR hid_t H5P_LST_DATASET_CREATE_g;
H5_DLLVAR hid_t H5P_LST_DATASET_XFER_g;
H5_DLLVAR hid_t H5P_LST_MOUNT_g;

/* Public functions */
H5_DLL hid_t H5Pcreate_class(hid_t parent, const char *name,
            H5P_cls_create_func_t cls_create, void *create_data,
            H5P_cls_copy_func_t cls_copy, void *copy_data,
            H5P_cls_close_func_t cls_close, void *close_data);
H5_DLL char *H5Pget_class_name(hid_t pclass_id);
H5_DLL hid_t H5Pcreate(hid_t cls_id);
H5_DLL herr_t H5Pregister(hid_t cls_id, const char *name, size_t size,
            void *def_value, H5P_prp_create_func_t prp_create,
            H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
            H5P_prp_delete_func_t prp_del,
            H5P_prp_copy_func_t prp_copy,
            H5P_prp_close_func_t prp_close);
H5_DLL herr_t H5Pinsert(hid_t plist_id, const char *name, size_t size,
            void *value, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
            H5P_prp_delete_func_t prp_delete,
            H5P_prp_copy_func_t prp_copy,
            H5P_prp_close_func_t prp_close);
H5_DLL herr_t H5Pset(hid_t plist_id, const char *name, void *value);
H5_DLL htri_t H5Pexist(hid_t plist_id, const char *name);
H5_DLL herr_t H5Pget_size(hid_t id, const char *name, size_t *size);
H5_DLL herr_t H5Pget_nprops(hid_t id, size_t *nprops);
H5_DLL hid_t H5Pget_class(hid_t plist_id);
H5_DLL hid_t H5Pget_class_parent(hid_t pclass_id);
H5_DLL herr_t H5Pget(hid_t plist_id, const char *name, void * value);
H5_DLL htri_t H5Pequal(hid_t id1, hid_t id2);
H5_DLL htri_t H5Pisa_class(hid_t plist_id, hid_t pclass_id);
H5_DLL int H5Piterate(hid_t id, int *idx, H5P_iterate_t iter_func,
            void *iter_data);
H5_DLL herr_t H5Pcopy_prop(hid_t dst_id, hid_t src_id, const char *name);
H5_DLL herr_t H5Premove(hid_t plist_id, const char *name);
H5_DLL herr_t H5Punregister(hid_t pclass_id, const char *name);
H5_DLL herr_t H5Pclose_class(hid_t plist_id);
H5_DLL herr_t H5Pclose(hid_t plist_id);
H5_DLL hid_t H5Pcopy(hid_t plist_id);

H5_DLL herr_t H5Pget_version(hid_t plist_id, unsigned *boot/*out*/,
         unsigned *freelist/*out*/, unsigned *stab/*out*/,
         unsigned *shhdr/*out*/);
H5_DLL herr_t H5Pset_userblock(hid_t plist_id, hsize_t size);
H5_DLL herr_t H5Pget_userblock(hid_t plist_id, hsize_t *size);
H5_DLL herr_t H5Pset_alignment(hid_t fapl_id, hsize_t threshold,
    hsize_t alignment);
H5_DLL herr_t H5Pget_alignment(hid_t fapl_id, hsize_t *threshold/*out*/,
    hsize_t *alignment/*out*/);
H5_DLL herr_t H5Pset_sizes(hid_t plist_id, size_t sizeof_addr,
       size_t sizeof_size);
H5_DLL herr_t H5Pget_sizes(hid_t plist_id, size_t *sizeof_addr/*out*/,
       size_t *sizeof_size/*out*/);
#ifdef H5_WANT_H5_V1_4_COMPAT
H5_DLL herr_t H5Pset_sym_k(hid_t plist_id, int ik, int lk);
H5_DLL herr_t H5Pget_sym_k(hid_t plist_id, int *ik/*out*/, int *lk/*out*/);
#else /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_sym_k(hid_t plist_id, unsigned ik, unsigned lk);
H5_DLL herr_t H5Pget_sym_k(hid_t plist_id, unsigned *ik/*out*/, unsigned *lk/*out*/);
#endif /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_istore_k(hid_t plist_id, unsigned ik);
H5_DLL herr_t H5Pget_istore_k(hid_t plist_id, unsigned *ik/*out*/);
H5_DLL herr_t H5Pset_layout(hid_t plist_id, H5D_layout_t layout);
H5_DLL H5D_layout_t H5Pget_layout(hid_t plist_id);
H5_DLL herr_t H5Pset_chunk(hid_t plist_id, int ndims, const hsize_t dim[]);
H5_DLL int H5Pget_chunk(hid_t plist_id, int max_ndims, hsize_t dim[]/*out*/);
H5_DLL herr_t H5Pset_external(hid_t plist_id, const char *name, off_t offset,
          hsize_t size);
H5_DLL int H5Pget_external_count(hid_t plist_id);
H5_DLL herr_t H5Pget_external(hid_t plist_id, unsigned idx, size_t name_size,
          char *name/*out*/, off_t *offset/*out*/,
          hsize_t *size/*out*/);
H5_DLL herr_t H5Pset_driver(hid_t plist_id, hid_t driver_id,
        const void *driver_info);
H5_DLL hid_t H5Pget_driver(hid_t plist_id);
H5_DLL void *H5Pget_driver_info(hid_t plist_id);
H5_DLL herr_t H5Pset_family_offset(hid_t fapl_id, hsize_t offset);
H5_DLL herr_t H5Pget_family_offset(hid_t fapl_id, hsize_t *offset);
H5_DLL herr_t H5Pset_multi_type(hid_t fapl_id, H5FD_mem_t type);
H5_DLL herr_t H5Pget_multi_type(hid_t fapl_id, H5FD_mem_t *type);
#ifdef H5_WANT_H5_V1_4_COMPAT
H5_DLL herr_t H5Pset_buffer(hid_t plist_id, hsize_t size, void *tconv,
        void *bkg);
H5_DLL hsize_t H5Pget_buffer(hid_t plist_id, void **tconv/*out*/,
        void **bkg/*out*/);
#else /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_buffer(hid_t plist_id, size_t size, void *tconv,
        void *bkg);
H5_DLL size_t H5Pget_buffer(hid_t plist_id, void **tconv/*out*/,
        void **bkg/*out*/);
#endif /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_preserve(hid_t plist_id, hbool_t status);
H5_DLL int H5Pget_preserve(hid_t plist_id);
H5_DLL herr_t H5Pmodify_filter(hid_t plist_id, H5Z_filter_t filter,
        unsigned int flags, size_t cd_nelmts,
        const unsigned int cd_values[/*cd_nelmts*/]);
H5_DLL herr_t H5Pset_filter(hid_t plist_id, H5Z_filter_t filter,
        unsigned int flags, size_t cd_nelmts,
        const unsigned int c_values[]);
H5_DLL int H5Pget_nfilters(hid_t plist_id);
H5_DLL H5Z_filter_t H5Pget_filter(hid_t plist_id, unsigned filter,
       unsigned int *flags/*out*/,
       size_t *cd_nelmts/*out*/,
       unsigned cd_values[]/*out*/,
       size_t namelen, char name[]);
H5_DLL H5Z_filter_t H5Pget_filter_by_id(hid_t plist_id, H5Z_filter_t id,
       unsigned int *flags/*out*/,
       size_t *cd_nelmts/*out*/,
       unsigned cd_values[]/*out*/,
       size_t namelen, char name[]);
H5_DLL htri_t H5Pall_filters_avail(hid_t plist_id);
H5_DLL herr_t H5Pset_deflate(hid_t plist_id, unsigned aggression);
H5_DLL herr_t H5Pset_szip(hid_t plist_id, unsigned options_mask, unsigned pixels_per_block);
H5_DLL herr_t H5Pset_shuffle(hid_t plist_id);
H5_DLL herr_t H5Pset_fletcher32(hid_t plist_id);
H5_DLL herr_t H5Pset_edc_check(hid_t plist_id, H5Z_EDC_t check);
H5_DLL H5Z_EDC_t H5Pget_edc_check(hid_t plist_id);
H5_DLL herr_t H5Pset_filter_callback(hid_t plist_id, H5Z_filter_func_t func,
                                     void* op_data);
#ifdef H5_WANT_H5_V1_4_COMPAT
H5_DLL herr_t H5Pset_cache(hid_t plist_id, int mdc_nelmts, int rdcc_nelmts,
       size_t rdcc_nbytes, double rdcc_w0);
H5_DLL herr_t H5Pget_cache(hid_t plist_id, int *mdc_nelmts/*out*/,
       int *rdcc_nelmts/*out*/,
       size_t *rdcc_nbytes/*out*/, double *rdcc_w0);
#else /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_cache(hid_t plist_id, int mdc_nelmts, size_t rdcc_nelmts,
       size_t rdcc_nbytes, double rdcc_w0);
H5_DLL herr_t H5Pget_cache(hid_t plist_id, int *mdc_nelmts/*out*/,
       size_t *rdcc_nelmts/*out*/,
       size_t *rdcc_nbytes/*out*/, double *rdcc_w0);
#endif /* H5_WANT_H5_V1_4_COMPAT */
#ifdef H5_WANT_H5_V1_4_COMPAT
H5_DLL herr_t H5Pset_hyper_cache(hid_t plist_id, unsigned cache,
      unsigned limit);
H5_DLL herr_t H5Pget_hyper_cache(hid_t plist_id, unsigned *cache,
      unsigned *limit);
#endif /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_btree_ratios(hid_t plist_id, double left, double middle,
       double right);
H5_DLL herr_t H5Pget_btree_ratios(hid_t plist_id, double *left/*out*/,
       double *middle/*out*/,
       double *right/*out*/);
H5_DLL herr_t H5Pset_fill_value(hid_t plist_id, hid_t type_id,
     const void *value);
H5_DLL herr_t H5Pget_fill_value(hid_t plist_id, hid_t type_id,
     void *value/*out*/);
H5_DLL herr_t H5Pfill_value_defined(hid_t plist, H5D_fill_value_t *status);
H5_DLL herr_t H5Pset_alloc_time(hid_t plist_id, H5D_alloc_time_t
  alloc_time);
H5_DLL herr_t H5Pget_alloc_time(hid_t plist_id, H5D_alloc_time_t
  *alloc_time/*out*/);
H5_DLL herr_t H5Pset_fill_time(hid_t plist_id, H5D_fill_time_t fill_time);
H5_DLL herr_t H5Pget_fill_time(hid_t plist_id, H5D_fill_time_t
  *fill_time/*out*/);
H5_DLL herr_t H5Pset_gc_references(hid_t fapl_id, unsigned gc_ref);
H5_DLL herr_t H5Pget_gc_references(hid_t fapl_id, unsigned *gc_ref/*out*/);
H5_DLL herr_t H5Pset_fclose_degree(hid_t fapl_id, H5F_close_degree_t degree);
H5_DLL herr_t H5Pget_fclose_degree(hid_t fapl_id, H5F_close_degree_t *degree);
H5_DLL herr_t H5Pset_vlen_mem_manager(hid_t plist_id,
                                       H5MM_allocate_t alloc_func,
                                       void *alloc_info, H5MM_free_t free_func,
                                       void *free_info);
H5_DLL herr_t H5Pget_vlen_mem_manager(hid_t plist_id,
                                       H5MM_allocate_t *alloc_func,
                                       void **alloc_info,
                                       H5MM_free_t *free_func,
                                       void **free_info);
H5_DLL herr_t H5Pset_meta_block_size(hid_t fapl_id, hsize_t size);
H5_DLL herr_t H5Pget_meta_block_size(hid_t fapl_id, hsize_t *size/*out*/);
#ifdef H5_WANT_H5_V1_4_COMPAT
H5_DLL herr_t H5Pset_sieve_buf_size(hid_t fapl_id, hsize_t size);
H5_DLL herr_t H5Pget_sieve_buf_size(hid_t fapl_id, hsize_t *size/*out*/);
#else /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_sieve_buf_size(hid_t fapl_id, size_t size);
H5_DLL herr_t H5Pget_sieve_buf_size(hid_t fapl_id, size_t *size/*out*/);
#endif /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL herr_t H5Pset_hyper_vector_size(hid_t fapl_id, size_t size);
H5_DLL herr_t H5Pget_hyper_vector_size(hid_t fapl_id, size_t *size/*out*/);
H5_DLL herr_t H5Pset_small_data_block_size(hid_t fapl_id, hsize_t size);
H5_DLL herr_t H5Pget_small_data_block_size(hid_t fapl_id, hsize_t *size/*out*/);
H5_DLL herr_t H5Premove_filter(hid_t plist_id, H5Z_filter_t filter);

#ifdef __cplusplus
}
#endif
#endif
