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

#define H5D_PACKAGE    /*suppress error about including H5Dpkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5D_init_interface

#include "H5private.h"    /* Generic Functions      */
#include "H5Dpkg.h"    /* Datasets         */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free Lists                           */
#include "H5FOprivate.h"        /* File objects                         */
#include "H5HLprivate.h"  /* Local heaps        */
#include "H5Iprivate.h"    /* IDs            */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Sprivate.h"    /* Dataspaces         */
#include "H5Vprivate.h"    /* Vectors and arrays       */

/*#define H5D_DEBUG*/

/* Local functions */
static herr_t H5D_init_storage(H5D_t *dataset, hbool_t full_overwrite, hid_t dxpl_id);
static H5D_shared_t * H5D_new(hid_t dcpl_id, hbool_t creating, hbool_t vl_type);
static H5D_t * H5D_create(H5G_entry_t *loc, const char *name, hid_t type_id,
           const H5S_t *space, hid_t dcpl_id, hid_t dxpl_id);
static H5D_t * H5D_open_oid(const H5G_entry_t *ent, hid_t dxpl_id);
static herr_t H5D_get_space_status(H5D_t *dset, H5D_space_status_t *allocation, hid_t dxpl_id);
static hsize_t H5D_get_storage_size(H5D_t *dset, hid_t dxpl_id);
static haddr_t H5D_get_offset(const H5D_t *dset);
static herr_t H5D_extend(H5D_t *dataset, const hsize_t *size, hid_t dxpl_id);
static herr_t H5D_set_extent(H5D_t *dataset, const hsize_t *size, hid_t dxpl_id);
static herr_t H5D_init_type(H5F_t *file, const H5D_t *dset, hid_t type_id, const H5T_t *type);
static int H5D_crt_fill_value_cmp(const void *value1, const void *value2, size_t size);
static int H5D_crt_ext_file_list_cmp(const void *value1, const void *value2, size_t size);
static int H5D_crt_data_pipeline_cmp(const void *value1, const void *value2, size_t size);

/* Internal data structure for computing variable-length dataset's total size */
typedef struct {
    hid_t dataset_id;   /* ID of the dataset we are working on */
    hid_t fspace_id;    /* ID of the file dataset's dataspace we are working on */
    hid_t mspace_id;    /* ID of the memory dataset's dataspace we are working on */
    void *fl_tbuf;      /* Ptr to the temporary buffer we are using for fixed-length data */
    void *vl_tbuf;      /* Ptr to the temporary buffer we are using for VL data */
    hid_t xfer_pid;     /* ID of the dataset xfer property list */
    hsize_t size;       /* Accumulated number of bytes for the selection */
} H5D_vlen_bufsize_t;

/* Declare a free list to manage the H5D_t and H5D_shared_t structs */
H5FL_DEFINE_STATIC(H5D_t);
H5FL_DEFINE_STATIC(H5D_shared_t);

/* Declare a free list to manage blocks of VL data */
H5FL_BLK_DEFINE_STATIC(vlen_vl_buf);

/* Declare a free list to manage other blocks of VL data */
H5FL_BLK_DEFINE_STATIC(vlen_fl_buf);

/* Declare the external PQ free list for the sieve buffer information */
H5FL_BLK_EXTERN(sieve_buf);

/* Define a static "default" dataset structure to use to initialize new datasets */
static H5D_shared_t H5D_def_dset;

/* Define a "default" dataset transfer property list cache structure to use for default DXPLs */
H5D_dxpl_cache_t H5D_def_dxpl_cache;


/*-------------------------------------------------------------------------
 * Function:  H5D_init
 *
 * Purpose:  Initialize the interface from some other layer.
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 4, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_init(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_init, FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*--------------------------------------------------------------------------
NAME
   H5D_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5D_init_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.
NOTES
    Care must be taken when using the H5P functions, since they can cause
    a deadlock in the library when the library is attempting to terminate -QAK

--------------------------------------------------------------------------*/
static herr_t
H5D_init_interface(void)
{
    /* Dataset Transfer property class variables.  In sequence, they are,
     * - Transfer Property list class to modify
     * - Default value for maximum temp buffer size
     * - Default value for type conversion buffer
     * - Default value for background buffer
     * - Default value for B-tree node split ratios
     * - Default value for hyperslab caching
     * - Default value for hyperslab cache limit
     * - Default value for vlen allocation function
     * - Default value for vlen allocation information
     * - Default value for vlen free function
     * - Default value for vlen free information
     * - Default value for file driver ID
     * - Default value for file driver info
     * - Default value for 'gather reads' property
     * - Default value for vector size
     * - Default value for I/O transfer mode
     * - Default value for EDC property
     * - Default value for filter callback
     */
    H5P_genclass_t  *xfer_pclass;
    size_t          def_max_temp_buf         = H5D_XFER_MAX_TEMP_BUF_DEF;
    void            *def_tconv_buf           = H5D_XFER_TCONV_BUF_DEF;
    void            *def_bkgr_buf            = H5D_XFER_BKGR_BUF_DEF;
    H5T_bkg_t       def_bkgr_buf_type        = H5D_XFER_BKGR_BUF_TYPE_DEF;
    double          def_btree_split_ratio[3] = H5D_XFER_BTREE_SPLIT_RATIO_DEF;
#ifdef H5_WANT_H5_V1_4_COMPAT
    unsigned        def_hyper_cache          = H5D_XFER_HYPER_CACHE_DEF;
    unsigned        def_hyper_cache_lim      = H5D_XFER_HYPER_CACHE_LIM_DEF;
#endif /* H5_WANT_H5_V1_4_COMPAT */
    H5MM_allocate_t def_vlen_alloc           = H5D_XFER_VLEN_ALLOC_DEF;
    void            *def_vlen_alloc_info     = H5D_XFER_VLEN_ALLOC_INFO_DEF;
    H5MM_free_t     def_vlen_free            = H5D_XFER_VLEN_FREE_DEF;
    void            *def_vlen_free_info      = H5D_XFER_VLEN_FREE_INFO_DEF;
    hid_t           def_vfl_id               = H5D_XFER_VFL_ID_DEF;
    void            *def_vfl_info            = H5D_XFER_VFL_INFO_DEF;
    size_t          def_hyp_vec_size         = H5D_XFER_HYPER_VECTOR_SIZE_DEF;
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t def_io_xfer_mode        = H5D_XFER_IO_XFER_MODE_DEF;
#endif /* H5_HAVE_PARALLEL */
    H5Z_EDC_t       enable_edc               = H5D_XFER_EDC_DEF;
    H5Z_cb_t        filter_cb                = H5D_XFER_FILTER_CB_DEF;

    /* Dataset creation property class variables.  In sequence, they are,
     * - Creation property list class to modify
     * - Default value for storage layout property
     * - Default value for chunk dimensionality property
     * - Default value for chunk size
     * - Default value for fill value
     * - Default value for external file list
     * - Default value for data filter pipeline
     */
    H5P_genclass_t  *crt_pclass;
    H5D_layout_t    layout                   = H5D_CRT_LAYOUT_DEF;
    unsigned        chunk_ndims              = H5D_CRT_CHUNK_DIM_DEF;
    size_t          chunk_size[H5O_LAYOUT_NDIMS] = H5D_CRT_CHUNK_SIZE_DEF;
    H5O_fill_t      fill                     = H5D_CRT_FILL_VALUE_DEF;
    H5D_alloc_time_t    alloc_time           = H5D_CRT_ALLOC_TIME_DEF;
    unsigned        alloc_time_state         = H5D_CRT_ALLOC_TIME_STATE_DEF;
    H5D_fill_time_t     fill_time            = H5D_CRT_FILL_TIME_DEF;
    H5O_efl_t       efl                      = H5D_CRT_EXT_FILE_LIST_DEF;
    H5O_pline_t     pline                    = H5D_CRT_DATA_PIPELINE_DEF;

    H5P_genplist_t *def_dcpl;               /* Default Dataset Creation Property list */
    size_t          nprops;                 /* Number of properties */
    herr_t          ret_value                = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_init_interface)

    /* Initialize the atom group for the dataset IDs */
    if (H5I_init_group(H5I_DATASET, H5I_DATASETID_HASHSIZE, H5D_RESERVED_ATOMS, (H5I_free_t)H5D_close)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* =========Dataset Transfer Property Class Initialization========= */
    /* Register the default dataset transfer properties */
    assert(H5P_CLS_DATASET_XFER_g!=(-1));

    /* Get the pointer to the dataset transfer class */
    if (NULL == (xfer_pclass = H5I_object(H5P_CLS_DATASET_XFER_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class")

    /* Get the number of properties in the class */
    if(H5P_get_nprops_pclass(xfer_pclass,&nprops)<0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't query number of properties")

    /* Assume that if there are properties in the class, they are the default ones */
    if(nprops==0) {
        /* Register the max. temp buffer size property */
        if(H5P_register(xfer_pclass,H5D_XFER_MAX_TEMP_BUF_NAME,H5D_XFER_MAX_TEMP_BUF_SIZE,&def_max_temp_buf,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the type conversion buffer property */
        if(H5P_register(xfer_pclass,H5D_XFER_TCONV_BUF_NAME,H5D_XFER_TCONV_BUF_SIZE,&def_tconv_buf,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the background buffer property */
        if(H5P_register(xfer_pclass,H5D_XFER_BKGR_BUF_NAME,H5D_XFER_BKGR_BUF_SIZE,&def_bkgr_buf,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the background buffer type property */
        if(H5P_register(xfer_pclass,H5D_XFER_BKGR_BUF_TYPE_NAME,H5D_XFER_BKGR_BUF_TYPE_SIZE,&def_bkgr_buf_type,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the B-Tree node splitting ratios property */
        if(H5P_register(xfer_pclass,H5D_XFER_BTREE_SPLIT_RATIO_NAME,H5D_XFER_BTREE_SPLIT_RATIO_SIZE,def_btree_split_ratio,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

#ifdef H5_WANT_H5_V1_4_COMPAT
        /* Register the hyperslab caching property */
        if(H5P_register(xfer_pclass,H5D_XFER_HYPER_CACHE_NAME,H5D_XFER_HYPER_CACHE_SIZE,&def_hyper_cache,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the hyperslab cache limit property */
        if(H5P_register(xfer_pclass,H5D_XFER_HYPER_CACHE_LIM_NAME,H5D_XFER_HYPER_CACHE_LIM_SIZE,&def_hyper_cache_lim,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
#endif /* H5_WANT_H5_V1_4_COMPAT */

        /* Register the vlen allocation function property */
        if(H5P_register(xfer_pclass,H5D_XFER_VLEN_ALLOC_NAME,H5D_XFER_VLEN_ALLOC_SIZE,&def_vlen_alloc,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the vlen allocation information property */
        if(H5P_register(xfer_pclass,H5D_XFER_VLEN_ALLOC_INFO_NAME,H5D_XFER_VLEN_ALLOC_INFO_SIZE,&def_vlen_alloc_info,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the vlen free function property */
        if(H5P_register(xfer_pclass,H5D_XFER_VLEN_FREE_NAME,H5D_XFER_VLEN_FREE_SIZE,&def_vlen_free,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the vlen free information property */
        if(H5P_register(xfer_pclass,H5D_XFER_VLEN_FREE_INFO_NAME,H5D_XFER_VLEN_FREE_INFO_SIZE,&def_vlen_free_info,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the file driver ID property */
        if(H5P_register(xfer_pclass,H5D_XFER_VFL_ID_NAME,H5D_XFER_VFL_ID_SIZE,&def_vfl_id,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the file driver info property */
        if(H5P_register(xfer_pclass,H5D_XFER_VFL_INFO_NAME,H5D_XFER_VFL_INFO_SIZE,&def_vfl_info,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the vector size property */
        if(H5P_register(xfer_pclass,H5D_XFER_HYPER_VECTOR_SIZE_NAME,H5D_XFER_HYPER_VECTOR_SIZE_SIZE,&def_hyp_vec_size,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

#ifdef H5_HAVE_PARALLEL
        /* Register the I/O transfer mode property */
        if(H5P_register(xfer_pclass,H5D_XFER_IO_XFER_MODE_NAME,H5D_XFER_IO_XFER_MODE_SIZE,&def_io_xfer_mode,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
#endif /* H5_HAVE_PARALLEL */

        /* Register the EDC property */
        if(H5P_register(xfer_pclass,H5D_XFER_EDC_NAME,H5D_XFER_EDC_SIZE,&enable_edc,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the filter callback property */
        if(H5P_register(xfer_pclass,H5D_XFER_FILTER_CB_NAME,H5D_XFER_FILTER_CB_SIZE,&filter_cb,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    } /* end if */

    /* Only register the default property list if it hasn't been created yet */
    if(H5P_LST_DATASET_XFER_g==(-1)) {
        /* Register the default data transfer property list */
        if ((H5P_LST_DATASET_XFER_g = H5P_create_id (xfer_pclass))<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register default property list")
    } /* end if */

    /* =========Dataset Creation Property Class Initialization========== */
    /* Register the default dataset creation properties */
    assert(H5P_CLS_DATASET_CREATE_g != -1);

    /* Get the pointer to the dataset creation class */
    if(NULL == (crt_pclass = H5I_object(H5P_CLS_DATASET_CREATE_g)))
       HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class")

    /* Get the number of properties in the class */
    if(H5P_get_nprops_pclass(crt_pclass,&nprops)<0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't query number of properties")

    /* Assume that if there are properties in the class, they are the default ones */
    if(nprops==0) {
        /* Register the storage layout property */
        if(H5P_register(crt_pclass, H5D_CRT_LAYOUT_NAME, H5D_CRT_LAYOUT_SIZE, &layout, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
           HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the chunking dimensionality property */
        if(H5P_register(crt_pclass, H5D_CRT_CHUNK_DIM_NAME, H5D_CRT_CHUNK_DIM_SIZE, &chunk_ndims, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
           HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the chunking size property */
        if(H5P_register(crt_pclass, H5D_CRT_CHUNK_SIZE_NAME, H5D_CRT_CHUNK_SIZE_SIZE, chunk_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
           HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the fill value property */
        if(H5P_register(crt_pclass, H5D_CRT_FILL_VALUE_NAME, H5D_CRT_FILL_VALUE_SIZE, &fill, NULL, NULL, NULL, NULL, NULL, H5D_CRT_FILL_VALUE_CMP, NULL) < 0)
           HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the space allocation time property */
        if(H5P_register(crt_pclass, H5D_CRT_ALLOC_TIME_NAME, H5D_CRT_ALLOC_TIME_SIZE, &alloc_time, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the space allocation time state property */
        if(H5P_register(crt_pclass, H5D_CRT_ALLOC_TIME_STATE_NAME, H5D_CRT_ALLOC_TIME_STATE_SIZE, &alloc_time_state, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the fill value writing time property */
        if(H5P_register(crt_pclass, H5D_CRT_FILL_TIME_NAME, H5D_CRT_FILL_TIME_SIZE, &fill_time, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the external file list property */
        if(H5P_register(crt_pclass, H5D_CRT_EXT_FILE_LIST_NAME, H5D_CRT_EXT_FILE_LIST_SIZE, &efl, NULL, NULL, NULL, NULL, NULL, H5D_CRT_EXT_FILE_LIST_CMP, NULL) < 0)
           HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

        /* Register the data pipeline property */
        if(H5P_register(crt_pclass, H5D_CRT_DATA_PIPELINE_NAME, H5D_CRT_DATA_PIPELINE_SIZE, &pline, NULL, NULL, NULL, NULL, NULL, H5D_CRT_DATA_PIPELINE_CMP, NULL) < 0)
           HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    } /* end if */

    /* Only register the default property list if it hasn't been created yet */
    if(H5P_LST_DATASET_CREATE_g==(-1)) {
        /* Register the default data transfer property list */
        if ((H5P_LST_DATASET_CREATE_g = H5P_create_id (crt_pclass))<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register default property list")
    } /* end if */

    /* Reset the "default dataset" information */
    HDmemset(&H5D_def_dset,0,sizeof(H5D_shared_t));

    /* Get the default dataset cretion property list values and initialize the
     * default dataset with them.
     */
    if (NULL == (def_dcpl = H5I_object(H5P_LST_DATASET_CREATE_g)))
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "can't get default dataset creation property list")

    /* Set up the default allocation time information */
    if(H5P_get(def_dcpl, H5D_CRT_ALLOC_TIME_NAME, &H5D_def_dset.alloc_time) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve space allocation time")

    /* Get the default external file list information */
    if(H5P_get(def_dcpl, H5D_CRT_EXT_FILE_LIST_NAME, &H5D_def_dset.efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve external file list")

    /* Get the default data storage method */
    if(H5P_get(def_dcpl, H5D_CRT_LAYOUT_NAME, &H5D_def_dset.layout.type) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve layout")

    /* Get the default fill value time */
    if (H5P_get(def_dcpl, H5D_CRT_FILL_TIME_NAME, &H5D_def_dset.fill_time) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve fill time")

    /* Get the default fill value */
    if (H5P_get(def_dcpl, H5D_CRT_FILL_VALUE_NAME, &H5D_def_dset.fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve fill value")

    /* Reset the "default DXPL cache" information */
    HDmemset(&H5D_def_dxpl_cache,0,sizeof(H5D_dxpl_cache_t));

    /* Get the default DXPL cache information */
    if (H5D_get_dxpl_cache_real(H5P_DATASET_XFER_DEFAULT, &H5D_def_dxpl_cache) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve default DXPL info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_term_interface
 *
 * Purpose:  Terminate this interface.
 *
 * Return:  Success:  Positive if anything was done that might
 *        affect other interfaces; zero otherwise.
 *
 *     Failure:  Negative.
 *
 * Programmer:  Robb Matzke
 *              Friday, November 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5D_term_interface(void)
{
    int    n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_term_interface)

    if (H5_interface_initialize_g) {
  if ((n=H5I_nmembers(H5I_DATASET))>0) {
            /* The dataset API uses the "force" flag set to true because it
             * is using the "file objects" (H5FO) API functions to track open
             * objects in the file.  Using the H5FO code means that dataset
             * IDs can have reference counts >1, when an existing dataset is
             * opened more than once.  However, the H5I code does not attempt
             * to close objects with reference counts>1 unless the "force" flag
             * is set to true.
             *
             * At some point (probably after the group and datatypes use the
             * the H5FO code), the H5FO code might need to be switched around
             * to storing pointers to the objects being tracked (H5D_t, H5G_t,
             * etc) and reference count those itself instead of relying on the
             * reference counting in the H5I layer.  Then, the "force" flag can
             * be put back to false.
             *
             * Setting the "force" flag to true for all the interfaces won't
             * work because the "file driver" (H5FD) APIs use the H5I reference
             * counting to avoid closing a file driver out from underneath an
             * open file...
             *
             * QAK - 5/13/03
             */
      H5I_clear_group(H5I_DATASET, TRUE);
  } else {
      H5I_destroy_group(H5I_DATASET);
      H5_interface_initialize_g = 0;
      n = 1; /*H5I*/
  }
    }
    FUNC_LEAVE_NOAPI(n)
}


/*-------------------------------------------------------------------------
 * Function:       H5D_crt_copy
 *
 * Purpose:        Callback routine which is called whenever any dataset
 *                 creation property list is copied.  This routine copies
 *                 the properties from the old list to the new list.
 *
 * Return:         Success:        Non-negative
 *
 *                 Failure:        Negative
 *
 * Programmer:     Raymond Lu
 *                 Tuesday, October 2, 2001
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
herr_t
H5D_crt_copy(hid_t new_plist_id, hid_t old_plist_id, void UNUSED *copy_data)
{
    H5O_fill_t     src_fill, dst_fill;
    H5O_efl_t      src_efl, dst_efl;
    H5O_pline_t    src_pline, dst_pline;
    H5P_genplist_t *old_plist;
    H5P_genplist_t *new_plist;
    herr_t         ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5D_crt_copy, FAIL)

    /* Verify property list ID */
    if (NULL == (new_plist = H5I_object(new_plist_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")
    if (NULL == (old_plist = H5I_object(old_plist_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get the fill value, external file list, and data pipeline properties
     * from the old property list */
    if(H5P_get(old_plist, H5D_CRT_FILL_VALUE_NAME, &src_fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")
    if(H5P_get(old_plist, H5D_CRT_EXT_FILE_LIST_NAME, &src_efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")
    if(H5P_get(old_plist, H5D_CRT_DATA_PIPELINE_NAME, &src_pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline")

    /* Make copies of fill value, external file list, and data pipeline */
    if(src_fill.buf) {
        if(NULL==H5O_copy(H5O_FILL_ID, &src_fill, &dst_fill))
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't copy fill value")
    } /* end if */
    else {
  dst_fill.type = dst_fill.buf = NULL;
  dst_fill.size = src_fill.size;
    }
    HDmemset(&dst_efl,0,sizeof(H5O_efl_t));
    if(NULL==H5O_copy(H5O_EFL_ID, &src_efl, &dst_efl))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't copy external file list")
    if(NULL==H5O_copy(H5O_PLINE_ID, &src_pline, &dst_pline))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't copy data pipeline")

    /* Set the fill value, external file list, and data pipeline property
     * for the new property list */
    if(H5P_set(new_plist, H5D_CRT_FILL_VALUE_NAME, &dst_fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill value")
    if(H5P_set(new_plist, H5D_CRT_EXT_FILE_LIST_NAME, &dst_efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set external file list")
    if(H5P_set(new_plist, H5D_CRT_DATA_PIPELINE_NAME, &dst_pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set pipeline")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_crt_close
 *
 * Purpose:  Callback routine which is called whenever any dataset create
 *              property list is closed.  This routine performs any generic
 *              cleanup needed on the properties the library put into the list.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, July 11, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
herr_t
H5D_crt_close(hid_t dcpl_id, void UNUSED *close_data)
{
    H5O_fill_t     fill;
    H5O_efl_t      efl;
    H5O_pline_t    pline;
    H5P_genplist_t *plist;      /* Property list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_crt_close, FAIL)

    /* Check arguments */
    if (NULL == (plist = H5I_object(dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get the fill value, external file list, and data pipeline properties
     * from the old property list */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline")

    /* Clean up any values set for the fill-value, external file-list and
     * data pipeline */
    if(H5O_reset(H5O_FILL_ID, &fill)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release fill info")
    if(H5O_reset(H5O_EFL_ID, &efl)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release external file list info")
    if(H5O_reset(H5O_PLINE_ID, &pline)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release pipeline info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_crt_close() */


/*-------------------------------------------------------------------------
 * Function:       H5D_crt_fill_value_cmp
 *
 * Purpose:        Callback routine which is called whenever the fill value
 *                 property in the dataset creation property list is compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Wednesday, January 7, 2004
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_crt_fill_value_cmp(const void *value1, const void *value2, size_t UNUSED size)
{
    const H5O_fill_t     *fill1=(const H5O_fill_t *)value1,     /* Create local aliases for values */
        *fill2=(const H5O_fill_t *)value2;
    int cmp_value;              /* Value from comparison */
    herr_t         ret_value=0; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_crt_fill_value_cmp)

    /* Sanity check */
    assert(fill1);
    assert(fill2);
    assert(size==sizeof(H5O_fill_t));

    /* Check the size of fill values */
    if(fill1->size < fill2->size) HGOTO_DONE(-1);
    if(fill1->size > fill2->size) HGOTO_DONE(1);

    /* Check the types of the fill values */
    if(fill1->type==NULL && fill2->type!=NULL) HGOTO_DONE(-1);
    if(fill1->type!=NULL && fill2->type==NULL) HGOTO_DONE(1);
    if(fill1->type!=NULL)
        if((cmp_value=H5T_cmp(fill1->type,fill2->type, FALSE))!=0)
            HGOTO_DONE(cmp_value);

    /* Check the fill values in the buffers */
    if(fill1->buf==NULL && fill2->buf!=NULL) HGOTO_DONE(-1);
    if(fill1->buf!=NULL && fill2->buf==NULL) HGOTO_DONE(1);
    if(fill1->buf!=NULL)
        if((cmp_value=HDmemcmp(fill1->buf,fill2->buf,fill1->size))!=0)
            HGOTO_DONE(cmp_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_crt_fill_value_cmp() */


/*-------------------------------------------------------------------------
 * Function:       H5D_crt_ext_file_list_cmp
 *
 * Purpose:        Callback routine which is called whenever the external file
 *                 list property in the dataset creation property list is
 *                 compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Wednesday, January 7, 2004
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_crt_ext_file_list_cmp(const void *value1, const void *value2, size_t UNUSED size)
{
    const H5O_efl_t     *efl1=(const H5O_efl_t *)value1,     /* Create local aliases for values */
        *efl2=(const H5O_efl_t *)value2;
    int cmp_value;              /* Value from comparison */
    herr_t         ret_value=0; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_crt_ext_file_list_cmp)

    /* Sanity check */
    assert(efl1);
    assert(efl2);
    assert(size==sizeof(H5O_efl_t));

    /* Check the heap address of external file lists */
    if(H5F_addr_defined(efl1->heap_addr) || H5F_addr_defined(efl2->heap_addr)) {
        if(!H5F_addr_defined(efl1->heap_addr) && H5F_addr_defined(efl2->heap_addr)) HGOTO_DONE(-1);
        if(H5F_addr_defined(efl1->heap_addr) && !H5F_addr_defined(efl2->heap_addr)) HGOTO_DONE(1);
        if((cmp_value=H5F_addr_cmp(efl1->heap_addr,efl2->heap_addr))!=0)
            HGOTO_DONE(cmp_value);
    } /* end if */

    /* Check the number of allocated efl entries */
    if(efl1->nalloc < efl2->nalloc) HGOTO_DONE(-1);
    if(efl1->nalloc > efl2->nalloc) HGOTO_DONE(1);

    /* Check the number of used efl entries */
    if(efl1->nused < efl2->nused) HGOTO_DONE(-1);
    if(efl1->nused > efl2->nused) HGOTO_DONE(1);

    /* Check the efl entry information */
    if(efl1->slot==NULL && efl2->slot!=NULL) HGOTO_DONE(-1);
    if(efl1->slot!=NULL && efl2->slot==NULL) HGOTO_DONE(1);
    if(efl1->slot!=NULL && efl1->nused>0) {
        size_t u;       /* Local index variable */

        /* Loop through all entries, comparing them */
        for(u=0; u<efl1->nused; u++) {
            /* Check the name offset of the efl entry */
            if(efl1->slot[u].name_offset < efl2->slot[u].name_offset) HGOTO_DONE(-1);
            if(efl1->slot[u].name_offset > efl2->slot[u].name_offset) HGOTO_DONE(1);

            /* Check the name of the efl entry */
            if(efl1->slot[u].name==NULL && efl2->slot[u].name!=NULL) HGOTO_DONE(-1);
            if(efl1->slot[u].name!=NULL && efl2->slot[u].name==NULL) HGOTO_DONE(1);
            if(efl1->slot[u].name!=NULL)
                if((cmp_value=HDstrcmp(efl1->slot[u].name,efl2->slot[u].name))!=0)
                    HGOTO_DONE(cmp_value);

            /* Check the file offset of the efl entry */
            if(efl1->slot[u].offset < efl2->slot[u].offset) HGOTO_DONE(-1);
            if(efl1->slot[u].offset > efl2->slot[u].offset) HGOTO_DONE(1);

            /* Check the file size of the efl entry */
            if(efl1->slot[u].size < efl2->slot[u].size) HGOTO_DONE(-1);
            if(efl1->slot[u].size > efl2->slot[u].size) HGOTO_DONE(1);
        } /* end for */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_crt_ext_file_list_cmp() */


/*-------------------------------------------------------------------------
 * Function:       H5D_crt_data_pipeline_cmp
 *
 * Purpose:        Callback routine which is called whenever the filter pipeline
 *                 property in the dataset creation property list is compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Wednesday, January 7, 2004
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_crt_data_pipeline_cmp(const void *value1, const void *value2, size_t UNUSED size)
{
    const H5O_pline_t     *pline1=(const H5O_pline_t *)value1,     /* Create local aliases for values */
        *pline2=(const H5O_pline_t *)value2;
    int cmp_value;              /* Value from comparison */
    herr_t         ret_value=0; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_crt_data_pipeline_cmp)

    /* Sanity check */
    assert(pline1);
    assert(pline2);
    assert(size==sizeof(H5O_pline_t));

    /* Check the number of allocated pipeline entries */
    if(pline1->nalloc < pline2->nalloc) HGOTO_DONE(-1);
    if(pline1->nalloc > pline2->nalloc) HGOTO_DONE(1);

    /* Check the number of used pipeline entries */
    if(pline1->nused < pline2->nused) HGOTO_DONE(-1);
    if(pline1->nused > pline2->nused) HGOTO_DONE(1);

    /* Check the filter entry information */
    if(pline1->filter==NULL && pline2->filter!=NULL) HGOTO_DONE(-1);
    if(pline1->filter!=NULL && pline2->filter==NULL) HGOTO_DONE(1);
    if(pline1->filter!=NULL && pline1->nused>0) {
        size_t u;       /* Local index variable */

        /* Loop through all filters, comparing them */
        for(u=0; u<pline1->nused; u++) {
            /* Check the ID of the filter */
            if(pline1->filter[u].id < pline2->filter[u].id) HGOTO_DONE(-1);
            if(pline1->filter[u].id > pline2->filter[u].id) HGOTO_DONE(1);

            /* Check the flags for the filter */
            if(pline1->filter[u].flags < pline2->filter[u].flags) HGOTO_DONE(-1);
            if(pline1->filter[u].flags > pline2->filter[u].flags) HGOTO_DONE(1);

            /* Check the name of the filter */
            if(pline1->filter[u].name==NULL && pline2->filter[u].name!=NULL) HGOTO_DONE(-1);
            if(pline1->filter[u].name!=NULL && pline2->filter[u].name==NULL) HGOTO_DONE(1);
            if(pline1->filter[u].name!=NULL)
                if((cmp_value=HDstrcmp(pline1->filter[u].name,pline2->filter[u].name))!=0)
                    HGOTO_DONE(cmp_value);

            /* Check the number of parameters for the filter */
            if(pline1->filter[u].cd_nelmts < pline2->filter[u].cd_nelmts) HGOTO_DONE(-1);
            if(pline1->filter[u].cd_nelmts > pline2->filter[u].cd_nelmts) HGOTO_DONE(1);

            /* Check the filter parameter information */
            if(pline1->filter[u].cd_values==NULL && pline2->filter[u].cd_values!=NULL) HGOTO_DONE(-1);
            if(pline1->filter[u].cd_values!=NULL && pline2->filter[u].cd_values==NULL) HGOTO_DONE(1);
            if(pline1->filter[u].cd_values!=NULL && pline1->filter[u].cd_nelmts>0) {
                size_t v;       /* Local index variable */

                /* Loop through all parameters, comparing them */
                for(v=0; v<pline1->filter[u].cd_nelmts; v++) {
                    /* Check each parameter for the filter */
                    if(pline1->filter[u].cd_values[v] < pline2->filter[u].cd_values[v]) HGOTO_DONE(-1);
                    if(pline1->filter[u].cd_values[v] > pline2->filter[u].cd_values[v]) HGOTO_DONE(1);
                } /* end for */
            } /* end if */
        } /* end for */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_crt_data_pipeline_cmp() */


/*-------------------------------------------------------------------------
 * Function:  H5D_xfer_create
 *
 * Purpose:  Callback routine which is called whenever any dataset transfer
 *              property list is created.  This routine performs any generic
 *              initialization needed on the properties the library put into
 *              the list.
 *              Right now, it's just allocating the driver-specific dataset
 *              transfer information.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, August 2, 2001
 *
 * Notes:       This same routine is currently used for the 'copy' callback.
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
herr_t
H5D_xfer_create(hid_t dxpl_id, void UNUSED *create_data)
{
    hid_t driver_id;            /* VFL driver ID */
    void *driver_info;          /* VFL driver info */
    H5P_genplist_t *plist;      /* Property list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_xfer_create, FAIL)

    /* Check arguments */
    if (NULL == (plist = H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

    /* Get the driver information */
    if(H5P_get(plist, H5D_XFER_VFL_ID_NAME, &driver_id)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve VFL driver ID")
    if(H5P_get(plist, H5D_XFER_VFL_INFO_NAME, &driver_info)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve VFL driver info")

    /* Check if we have a valid driver ID */
    if(driver_id>0) {
        /* Set the driver for the property list */
        if(H5FD_dxpl_open(plist, driver_id, driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_xfer_create() */


/*-------------------------------------------------------------------------
 * Function:       H5D_xfer_copy
 *
 * Purpose:        Callback routine which is called whenever any dataset
 *                 transfer property list is copied.  This routine copies
 *                 the properties from the old list to the new list.
 *
 * Return:         Success:        Non-negative
 *
 *                 Failure:        Negative
 *
 * Programmer:     Raymond Lu
 *                 Tuesday, October 2, 2001
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
herr_t
H5D_xfer_copy(hid_t new_dxpl_id, hid_t old_dxpl_id, void UNUSED *copy_data)
{
    hid_t          driver_id;
    void*          driver_info;
    H5P_genplist_t *new_plist;              /* New property list */
    H5P_genplist_t *old_plist;              /* Old property list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_xfer_copy, FAIL)

    if(NULL == (new_plist = H5I_object(new_dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")
    if(NULL == (old_plist = H5I_object(old_dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Get values from old property list */
    if(H5P_get(old_plist, H5D_XFER_VFL_ID_NAME, &driver_id)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve VFL driver ID")
    if(H5P_get(old_plist, H5D_XFER_VFL_INFO_NAME, &driver_info)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get drver info")

    if(driver_id > 0) {
        /* Set the driver for the property list */
        if(H5FD_dxpl_open(new_plist, driver_id, driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set driver")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_xfer_close
 *
 * Purpose:  Callback routine which is called whenever any dataset transfer
 *              property list is closed.  This routine performs any generic
 *              cleanup needed on the properties the library put into the list.
 *              Right now, it's just freeing the driver-specific dataset
 *              transfer information.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, July 11, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
herr_t
H5D_xfer_close(hid_t dxpl_id, void UNUSED *close_data)
{
    hid_t driver_id;            /* VFL driver ID */
    void *driver_info;          /* VFL driver info */
    H5P_genplist_t *plist;      /* Property list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_xfer_close, FAIL)

    /* Check arguments */
    if (NULL == (plist = H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

    if(H5P_get(plist, H5D_XFER_VFL_ID_NAME, &driver_id)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve VFL driver ID")
    if(H5P_get(plist, H5D_XFER_VFL_INFO_NAME, &driver_info)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve VFL driver info")
    if(driver_id>0) {
        /* Close the driver for the property list */
        if(H5FD_dxpl_close(driver_id, driver_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't reset driver")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_xfer_close() */


/*--------------------------------------------------------------------------
 NAME
    H5D_get_dcpl_cache
 PURPOSE
    Get all the values for the DCPL cache.
 USAGE
    herr_t H5D_get_dcpl_cache(dcpl_id, cache)
        hid_t dcpl_id;          IN: DCPL to query
        H5D_dcpl_cache_t *cache;IN/OUT: DCPL cache to fill with values
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Query all the values from a DCPL that are needed by internal routines
    within the library.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5D_get_dcpl_cache(hid_t dcpl_id, H5D_dcpl_cache_t *cache)
{
    H5P_genplist_t *dc_plist;   /* Data transfer property list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_get_dcpl_cache)

    /* Check args */
    assert(cache);

    /* Get the dataset transfer property list */
    if (NULL == (dc_plist = H5I_object(dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get I/O pipeline info */
    if(H5P_get(dc_plist, H5D_CRT_DATA_PIPELINE_NAME, &cache->pline)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve I/O pipeline info")

    /* Get fill value info */
    if(H5P_get(dc_plist, H5D_CRT_FILL_VALUE_NAME, &cache->fill)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve fill value info")

    /* Get fill time info */
    if(H5P_get(dc_plist, H5D_CRT_FILL_TIME_NAME, &cache->fill_time)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve fill time")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5D_get_dcpl_cache() */


/*-------------------------------------------------------------------------
 * Function:  H5Dcreate
 *
 * Purpose:  Creates a new dataset named NAME at LOC_ID, opens the
 *    dataset for access, and associates with that dataset constant
 *    and initial persistent properties including the type of each
 *    datapoint as stored in the file (TYPE_ID), the size of the
 *    dataset (SPACE_ID), and other initial miscellaneous
 *    properties (DCPL_ID).
 *
 *    All arguments are copied into the dataset, so the caller is
 *    allowed to derive new types, data spaces, and creation
 *    parameters from the old ones and reuse them in calls to
 *    create other datasets.
 *
 * Return:  Success:  The object ID of the new dataset.  At this
 *        point, the dataset is ready to receive its
 *        raw data.  Attempting to read raw data from
 *        the dataset will probably return the fill
 *        value.  The dataset should be closed when
 *        the caller is no longer interested in it.
 *
 *    Failure:  FAIL
 *
 * Errors:
 *    ARGS    BADTYPE  Not a data space.
 *    ARGS    BADTYPE  Not a dataset creation plist.
 *    ARGS    BADTYPE  Not a file.
 *    ARGS    BADTYPE  Not a type.
 *    ARGS    BADVALUE  No name.
 *    DATASET    CANTINIT  Can't create dataset.
 *    DATASET    CANTREGISTER  Can't register dataset.
 *
 * Programmer:  Robb Matzke
 *    Wednesday, December  3, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dcreate(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id,
    hid_t dcpl_id)
{
    H5G_entry_t     *loc = NULL;         /* Entry for group to insert dataset into */
    H5D_t     *new_dset = NULL;    /* New dataset's info */
    const H5S_t    *space;              /* Dataspace for dataset */
    hid_t      ret_value;          /* Return value */

    FUNC_ENTER_API(H5Dcreate, FAIL)
    H5TRACE5("i","isiii",loc_id,name,type_id,space_id,dcpl_id);

    /* Check arguments */
    if (NULL == (loc = H5G_loc(loc_id)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location ID")
    if (!name || !*name)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if (H5I_DATATYPE != H5I_get_type(type_id))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype ID")
    if (NULL == (space = H5I_object_verify(space_id,H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace ID")
    if(H5P_DEFAULT == dcpl_id)
        dcpl_id = H5P_DATASET_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dcpl_id, H5P_DATASET_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset create property list ID")

    /* build and open the new dataset */
    if (NULL == (new_dset = H5D_create(loc, name, type_id, space, dcpl_id, H5AC_dxpl_id)))
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset")

    /* Register the new dataset to get an ID for it */
    if ((ret_value = H5I_register(H5I_DATASET, new_dset)) < 0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register dataset")

done:
    if(ret_value<0) {
        if(new_dset!=NULL) {
            if(H5D_close(new_dset)<0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")
        } /* end if */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dopen
 *
 * Purpose:  Finds a dataset named NAME at LOC_ID, opens it, and returns
 *    its ID.   The dataset should be close when the caller is no
 *    longer interested in it.
 *
 * Return:  Success:  A new dataset ID
 *
 *    Failure:  FAIL
 *
 * Errors:
 *
 * Programmer:  Robb Matzke
 *    Thursday, December  4, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dopen(hid_t loc_id, const char *name)
{
    H5D_t       *dset = NULL;
    H5G_entry_t  *loc = NULL;    /*location holding the dataset  */
    H5G_entry_t  ent;              /*dataset symbol table entry  */
    hbool_t      ent_found = FALSE;     /* Entry at 'name' found */
    hid_t        dxpl_id = H5AC_dxpl_id;    /* dxpl to use to open datset */
    hid_t        ret_value;

    FUNC_ENTER_API(H5Dopen, FAIL)
    H5TRACE2("i","is",loc_id,name);

    /* Check args */
    if (NULL == (loc = H5G_loc(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Find the dataset object */
    if (H5G_find(loc, name, &ent, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, FAIL, "not found")
    ent_found = TRUE;

    /* Check that the object found is the correct type */
    if (H5G_get_type(&ent, dxpl_id) != H5G_DATASET)
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "not a dataset")

    /* Open the dataset */
    if ((dset = H5D_open(&ent, dxpl_id))==NULL)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't open dataset")

    /* Register an atom for the dataset */
    if((ret_value=H5I_register(H5I_DATASET, dset)) <0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "can't register dataset atom")

done:
    if(ret_value < 0) {
        if(dset != NULL) {
            if(H5D_close(dset) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")
        } /* end if */
        else {
            if(ent_found && ent.header)
                H5G_free_ent_name(&ent);
        } /* end else */
    } /* end if */
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dclose
 *
 * Purpose:  Closes access to a dataset (DATASET_ID) and releases
 *    resources used by it. It is illegal to subsequently use that
 *    same dataset ID in calls to other dataset functions.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Errors:
 *    ARGS    BADTYPE  Not a dataset.
 *    DATASET    CANTINIT  Can't free.
 *
 * Programmer:  Robb Matzke
 *    Thursday, December  4, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dclose(hid_t dset_id)
{
    H5D_t  *dset = NULL;          /* Dataset object to release */
    herr_t       ret_value=SUCCEED;     /* Return value */

    FUNC_ENTER_API(H5Dclose, FAIL)
    H5TRACE1("e","i",dset_id);

    /* Check args */
    if (NULL == (dset = H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if (NULL == dset->ent.file)
  HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "not a dataset")

    /*
     * Decrement the counter on the dataset.  It will be freed if the count
     * reaches zero.
     */
    if (H5I_dec_ref(dset_id) < 0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't free")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dget_space
 *
 * Purpose:  Returns a copy of the file data space for a dataset.
 *
 * Return:  Success:  ID for a copy of the data space.  The data
 *        space should be released by calling
 *        H5Sclose().
 *
 *    Failure:  FAIL
 *
 * Programmer:  Robb Matzke
 *    Wednesday, January 28, 1998
 *
 * Modifications:
 *  Robb Matzke, 9 Jun 1998
 *  The data space is not constant and is no longer cached by the dataset
 *  struct.
 *-------------------------------------------------------------------------
 */
hid_t
H5Dget_space(hid_t dset_id)
{
    H5D_t  *dset = NULL;
    H5S_t  *space = NULL;
    hid_t  ret_value;

    FUNC_ENTER_API(H5Dget_space, FAIL)
    H5TRACE1("i","i",dset_id);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Read the data space message and return a data space object */
    if (NULL==(space=H5S_copy (dset->shared->space, FALSE)))
  HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get data space")

    /* Create an atom */
    if ((ret_value=H5I_register (H5I_DATASPACE, space))<0)
  HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data space")

done:
    if(ret_value<0) {
        if(space!=NULL) {
            if(H5S_close(space)<0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
        } /* end if */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dget_space_status
 *
 * Purpose:  Returns the status of data space allocation.
 *
 * Return:
 *    Success:  Non-negative
 *
 *    Failture:  Negative
 *
 * Programmer:  Raymond Lu
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_space_status(hid_t dset_id, H5D_space_status_t *allocation)
{
    H5D_t   *dset = NULL;
    herr_t   ret_value = SUCCEED;

    FUNC_ENTER_API(H5Dget_space_status, FAIL)
    H5TRACE2("e","i*Ds",dset_id,allocation);

    /* Check arguments */
    if(NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Read data space address and return */
    if(FAIL==(ret_value=H5D_get_space_status(dset, allocation, H5AC_ind_dxpl_id)))
        HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get space status")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5D_get_space_status
 *
 * Purpose:     Returns the status of data space allocation.
 *
 * Return:
 *              Success:        Non-negative
 *
 *              Failture:       Negative
 *
 * Programmer:  Raymond Lu
 *
 * Modification:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_get_space_status(H5D_t *dset, H5D_space_status_t *allocation, hid_t dxpl_id)
{
    H5S_t      *space;              /* Dataset's dataspace */
    hsize_t     space_allocated;    /* The number of bytes allocated for chunks */
    hssize_t    total_elem;         /* The total number of elements in dataspace */
    size_t      type_size;          /* The size of the datatype for the dataset */
    hsize_t     full_size;          /* The number of bytes in the dataset when fully populated */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_get_space_status)

    assert(dset);

    /* Get the dataset's dataspace */
    space=dset->shared->space;
    assert(space);

    /* Get the total number of elements in dataset's dataspace */
    if((total_elem=H5S_GET_EXTENT_NPOINTS(space))<0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTCOUNT, FAIL, "unable to get # of dataspace elements")

    /* Get the size of the dataset's datatype */
    if((type_size=H5T_get_size(dset->shared->type))==0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTCOUNT, FAIL, "unable to get size of datatype")

    /* Compute the maximum size of the dataset in bytes */
    H5_CHECK_OVERFLOW(total_elem,hssize_t,hsize_t);
    full_size=((hsize_t)total_elem)*type_size;

    /* Difficult to error check, since the error value is 0 and 0 is a valid value... :-/ */
    space_allocated=H5D_get_storage_size(dset,dxpl_id);

    /* Decide on how much of the space is allocated */
    if(space_allocated==0)
        *allocation = H5D_SPACE_STATUS_NOT_ALLOCATED;
    else if(space_allocated==full_size)
        *allocation = H5D_SPACE_STATUS_ALLOCATED;
    else {
        /* Should only happen for chunked datasets currently */
        assert(dset->shared->layout.type==H5D_CHUNKED);

        *allocation = H5D_SPACE_STATUS_PART_ALLOCATED;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dget_type
 *
 * Purpose:  Returns a copy of the file data type for a dataset.
 *
 * Return:  Success:  ID for a copy of the data type.   The data
 *        type should be released by calling
 *        H5Tclose().
 *
 *    Failure:  FAIL
 *
 * Programmer:  Robb Matzke
 *    Tuesday, February  3, 1998
 *
 * Modifications:
 *
 *   Robb Matzke, 1 Jun 1998
 *  If the dataset has a named data type then a handle to the opened data
 *  type is returned.  Otherwise the returned data type is read-only.  If
 *  atomization of the data type fails then the data type is closed.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dget_type(hid_t dset_id)
{

    H5D_t  *dset = NULL;
    H5T_t  *copied_type = NULL;
    hid_t  ret_value = FAIL;

    FUNC_ENTER_API(H5Dget_type, FAIL)
    H5TRACE1("i","i",dset_id);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Copy the data type and mark it read-only */
    if (NULL==(copied_type=H5T_copy (dset->shared->type, H5T_COPY_REOPEN)))
  HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy the data type")

    /* Mark any VL datatypes as being in memory now */
    if (H5T_vlen_mark(copied_type, NULL, H5T_VLEN_MEMORY)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");

    /* Unlock copied type */
    if (H5T_lock (copied_type, FALSE)<0)
  HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to lock transient data type")

    /* Create an atom */
    if ((ret_value=H5I_register (H5I_DATATYPE, copied_type))<0)
  HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data type")

done:
    if(ret_value<0) {
        if(copied_type!=NULL) {
            if(H5T_close (copied_type)<0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release datatype")
        } /* end if */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dget_create_plist
 *
 * Purpose:  Returns a copy of the dataset creation property list.
 *
 * Return:  Success:  ID for a copy of the dataset creation
 *        property list.  The template should be
 *        released by calling H5P_close().
 *
 *    Failure:  FAIL
 *
 * Programmer:  Robb Matzke
 *    Tuesday, February  3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              The way to retrieve and set property is changed for the
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dget_create_plist(hid_t dset_id)
{
    H5D_t    *dset = NULL;
    H5O_fill_t          copied_fill={NULL,0,NULL};
    H5P_genplist_t      *dcpl_plist;
    H5P_genplist_t      *new_plist;
    hid_t    new_dcpl_id = FAIL;
    hid_t    ret_value = FAIL;

    FUNC_ENTER_API(H5Dget_create_plist, FAIL)
    H5TRACE1("i","i",dset_id);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if (NULL == (dcpl_plist = H5I_object(dset->shared->dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Copy the creation property list */
    if((new_dcpl_id = H5P_copy_plist(dcpl_plist)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to copy the creation property list")
    if (NULL == (new_plist = H5I_object(new_dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Get the fill value property */
    if(H5P_get(new_plist, H5D_CRT_FILL_VALUE_NAME, &copied_fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

    /* Copy the dataset type into the fill value message */
    if(copied_fill.type==NULL)
        if(NULL==(copied_fill.type=H5T_copy(dset->shared->type, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy dataset data type for fill value")

    /* Set back the fill value property to property list */
    if(H5P_set(new_plist, H5D_CRT_FILL_VALUE_NAME, &copied_fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property list fill value")

    /* Set the return value */
    ret_value=new_dcpl_id;

done:
    if(ret_value<0) {
        if(new_dcpl_id>0)
            (void)H5I_dec_ref(new_dcpl_id);
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dextend
 *
 * Purpose:  This function makes sure that the dataset is at least of size
 *    SIZE. The dimensionality of SIZE is the same as the data
 *    space of the dataset being changed.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Friday, January 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dextend(hid_t dset_id, const hsize_t *size)
{
    H5D_t  *dset = NULL;
    herr_t       ret_value=SUCCEED;  /* Return value */

    FUNC_ENTER_API(H5Dextend, FAIL)
    H5TRACE2("e","i*h",dset_id,size);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if (!size)
  HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no size specified")

    /* Increase size */
    if (H5D_extend (dset, size, H5AC_dxpl_id)<0)
  HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to extend dataset")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_new
 *
 * Purpose:  Creates a new, empty dataset structure
 *
 * Return:  Success:  Pointer to a new dataset descriptor.
 *
 *    Failure:  NULL
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Monday, October 12, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to query and inialization for generic
 *              property list.
 *
 *-------------------------------------------------------------------------
 */
static H5D_shared_t *
H5D_new(hid_t dcpl_id, hbool_t creating, hbool_t vl_type)
{
    H5P_genplist_t  *plist;             /* Property list created */
    H5D_shared_t    *new_dset = NULL;   /* New dataset object */
    H5D_shared_t    *ret_value;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_new)

    if (NULL==(new_dset = H5FL_MALLOC(H5D_shared_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* If we are using the default dataset creation property list, during creation
     * don't bother to copy it, just increment the reference count
     */
    if(!vl_type && creating && dcpl_id == H5P_DATASET_CREATE_DEFAULT) {
        /* Copy the default dataset information */
        HDmemcpy(new_dset,&H5D_def_dset,sizeof(H5D_shared_t));

        if(H5I_inc_ref(dcpl_id)<0)
            HGOTO_ERROR (H5E_DATASET, H5E_CANTINC, NULL, "Can't increment default DCPL ID")
        new_dset->dcpl_id = dcpl_id;
    } /* end if */
    else {
        /* Reset the dataset information */
        HDmemset(new_dset,0,sizeof(H5D_shared_t));

        /* Get the property list */
        if (NULL == (plist = H5I_object(dcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a property list")

        new_dset->dcpl_id = H5P_copy_plist(plist);
    } /* end else */

    /* Set return value */
    ret_value=new_dset;

done:
    if(ret_value==NULL) {
        if(new_dset!=NULL) {
            if(new_dset->dcpl_id!=0)
                (void)H5I_dec_ref(new_dset->dcpl_id);
            H5FL_FREE(H5D_shared_t,new_dset);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_init_type
 *
 * Purpose:  Copy a datatype for a dataset's use, performing all the
 *              necessary adjustments, etc.
 *
 * Return:  Success:    SUCCEED
 *    Failure:    FAIL
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Thursday, June 24, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_init_type(H5F_t *file, const H5D_t *dset, hid_t type_id, const H5T_t *type)
{
    htri_t relocatable;                 /* Flag whether the type is relocatable */
    htri_t immutable;                   /* Flag whether the type is immutable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_init_type, FAIL)

    /* Sanity checking */
    assert(file);
    assert(dset);
    assert(type);

    /* Check whether the datatype is relocatable */
    if((relocatable=H5T_is_relocatable(type))<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check datatype?")

    /* Check whether the datatype is immutable */
    if((immutable=H5T_is_immutable(type))<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check datatype?")

    /* Copy the datatype if it's a custom datatype or if it'll change when it's location is changed */
    if(!immutable || relocatable) {
        /* Copy datatype for dataset */
        if((dset->shared->type = H5T_copy(type, H5T_COPY_ALL))==NULL)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "can't copy datatype")

        /* Mark any datatypes as being on disk now */
        if(H5T_vlen_mark(dset->shared->type, file, H5T_VLEN_DISK)<0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype location")

        /* Get a datatype ID for the dataset's datatype */
  if((dset->shared->type_id = H5I_register(H5I_DATATYPE, dset->shared->type))<0)
      HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register type")
    } /* end if */
    /* Not a custom datatype, just use it directly */
    else {
        if(H5I_inc_ref(type_id)<0)
            HGOTO_ERROR (H5E_DATASET, H5E_CANTINC, FAIL, "Can't increment datatype ID")

        /* Use existing datatype */
        dset->shared->type_id = type_id;
        dset->shared->type = (H5T_t *)type; /* (Cast away const OK - QAK) */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_init_type() */


/*-------------------------------------------------------------------------
 * Function:  H5D_update_entry_info
 *
 * Purpose:  Create and fill an H5G_entry_t object for insertion into
 *              the group LOC.
 *
 *              This code was originally found at the end of H5D_create()
 *              but was placed here for general use.
 *
 * Return:  Success:    SUCCEED
 *    Failure:    FAIL
 *
 * Errors:
 *
 * Programmer:  Bill Wendling
 *    Thursday, October 31, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_update_entry_info(H5F_t *file, hid_t dxpl_id, H5D_t *dset, H5P_genplist_t *plist)
{
    size_t              ohdr_size = H5D_MINHDR_SIZE;    /* Size of dataset's object header */
    H5G_entry_t        *ent=NULL;    /* Dataset's group entry */
    H5O_layout_t       *layout; /* Dataset's layout information */
    H5T_t              *type;   /* Dataset's datatype */
    H5S_t              *space;  /* Dataset's dataspace */
    H5D_alloc_time_t alloc_time;/* Dataset's allocation time */
    H5O_efl_t          *efl;    /* Dataset's external file list */

    /* fill value variables */
    H5D_fill_time_t  fill_time;
    H5O_fill_t    *fill_prop;     /* Pointer to dataset's fill value information */
    H5O_fill_new_t      fill = { NULL, 0, NULL, H5D_ALLOC_TIME_LATE, H5D_FILL_TIME_ALLOC, TRUE };
    H5D_fill_value_t  fill_status;

    struct H5O_t       *oh=NULL;        /* Pointer to dataset's object header */

    /* return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5D_update_entry_info, FAIL)

    /* Sanity checking */
    assert(file);
    assert(dset);

    /* Pick up former parameters */
    ent=&dset->ent;
    layout=&dset->shared->layout;
    type=dset->shared->type;
    space=dset->shared->space;
    alloc_time=dset->shared->alloc_time;
    efl=&dset->shared->efl;

    /* Add the dataset's raw data size to the size of the header, if the raw data will be stored as compact */
    if (layout->type == H5D_COMPACT)
        ohdr_size += layout->u.compact.size;

    /* Create (open for write access) an object header */
    if (H5O_create(file, dxpl_id, ohdr_size, ent) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset object header")

    /* Get a pointer to the object header itself */
    if((oh=H5O_protect(ent, dxpl_id))==NULL)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to protect dataset object header")

    /* Point at dataset's copy, to cache it for later */
    fill_prop=&dset->shared->fill;
    fill_time=dset->shared->fill_time;

    /* Check if dataset has non-default creation property list */
    if(dset->shared->dcpl_id!=H5P_DATASET_CREATE_DEFAULT) {
        /*
         * Retrieve properties of fill value and others. Copy them into new fill
         * value struct.
         */
        if (H5P_get(plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve fill time")
        dset->shared->fill_time=fill_time;    /* Cache this for later */

        /* Get the fill value information from the property list */
        if (H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, fill_prop) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve fill value")
    } /* end if */

    if (H5P_is_fill_value_defined(fill_prop, &fill_status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

    /* Special case handling for variable-length types */
    if(H5T_detect_class(type, H5T_VLEN)) {
        /* If the default fill value is chosen for variable-length types, always write it */
        if(fill_time==H5D_FILL_TIME_IFSET && fill_status==H5D_FILL_VALUE_DEFAULT) {
            dset->shared->fill_time=fill_time=H5D_FILL_TIME_ALLOC;

            /* Update dataset creation property */
            assert(dset->shared->dcpl_id!=H5P_DATASET_CREATE_DEFAULT);
            if (H5P_set(plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill time")
        } /* end if */

        /* Don't allow never writing fill values with variable-length types */
        if(fill_time==H5D_FILL_TIME_NEVER)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "Dataset doesn't support VL datatype when fill value is not defined")
    } /* end if */

    if (fill_status == H5D_FILL_VALUE_DEFAULT || fill_status == H5D_FILL_VALUE_USER_DEFINED) {
        if (H5O_copy(H5O_FILL_ID, fill_prop, &fill) == NULL)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT,FAIL, "unable to copy fill value")

        if (fill_prop->buf && fill_prop->size > 0 && H5O_fill_convert(&fill, type, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to convert fill value to dataset type")

  fill.fill_defined = TRUE;
    } else if (fill_status == H5D_FILL_VALUE_UNDEFINED) {
  fill.size = -1;
   fill.type = fill.buf = NULL;
   fill.fill_defined = FALSE;
    } else
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to determine if fill value is defined")

    fill.alloc_time = alloc_time;
    fill.fill_time = fill_time;

    if (fill.fill_defined == FALSE && fill_time == H5D_FILL_TIME_ALLOC)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT,FAIL, "unable to create dataset")

    /* Write new fill value message */
    if (H5O_append(file, dxpl_id, oh, H5O_FILL_NEW_ID, H5O_FLAG_CONSTANT, &fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update fill value header message")

    /* If there is valid information for the old fill value struct, update it */
    if (fill.buf) {
        /* Clear any previous values */
        if(H5O_reset(H5O_FILL_ID, fill_prop)<0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release fill info")

        /* Copy new fill value information to old fill value struct */
        if(H5O_copy(H5O_FILL_ID, &fill, fill_prop) == NULL)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT,FAIL,"unable to copy fill value")

        /* Write old fill value */
        if (fill_prop->buf && H5O_append(file, dxpl_id, oh, H5O_FILL_ID, H5O_FLAG_CONSTANT, fill_prop) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update fill value header message")

        /* Update dataset creation property */
        assert(dset->shared->dcpl_id!=H5P_DATASET_CREATE_DEFAULT);
        if (H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, fill_prop) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill value")
    } /* end if */

    /* Update the type and space header messages */
    if (H5O_append(file, dxpl_id, oh, H5O_DTYPE_ID, H5O_FLAG_CONSTANT | H5O_FLAG_SHARED, type) < 0 ||
            H5S_append(file, dxpl_id, oh, space) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update type or space header messages")

    /* Update the filters message, if this is a chunked dataset */
    if(layout->type==H5D_CHUNKED) {
        H5O_pline_t         pline;      /* Chunked data I/O pipeline info */

        if (H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve pipeline filter")

        if (pline.nused > 0 && H5O_append(file, dxpl_id, oh, H5O_PLINE_ID, H5O_FLAG_CONSTANT, &pline) < 0)
            HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update filter header message")
    } /* end if */

    /*
     * Allocate storage if space allocate time is early; otherwise delay
     * allocation until later.
     */
    if (alloc_time == H5D_ALLOC_TIME_EARLY)
        if (H5D_alloc_storage(file, dxpl_id, dset, H5D_ALLOC_CREATE, FALSE, FALSE) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize storage")

    /* Update external storage message */
    if (efl->nused > 0) {
        size_t heap_size = H5HL_ALIGN(1);
        size_t u;

        for (u = 0; u < efl->nused; ++u)
            heap_size += H5HL_ALIGN(HDstrlen(efl->slot[u].name) + 1);

        if (H5HL_create(file, dxpl_id, heap_size, &efl->heap_addr/*out*/) < 0 ||
                H5HL_insert(file, dxpl_id, efl->heap_addr, 1, "") == (size_t)(-1))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create external file list name heap")

        for (u = 0; u < efl->nused; ++u) {
            size_t offset = H5HL_insert(file, dxpl_id, efl->heap_addr,
                        HDstrlen(efl->slot[u].name) + 1, efl->slot[u].name);

            assert(0 == efl->slot[u].name_offset);

            if (offset == (size_t)(-1))
                HGOTO_ERROR(H5E_EFL, H5E_CANTINIT, FAIL, "unable to insert URL into name heap")

            efl->slot[u].name_offset = offset;
        }

        if (H5O_append(file, dxpl_id, oh, H5O_EFL_ID, H5O_FLAG_CONSTANT, efl) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update external file list message")
    }

    /* Update layout message */
    /* (Don't make layout message constant unless allocation time is early, since space may not be allocated) */
    /* Note: this is relying on H5D_alloc_storage not calling H5O_modify during dataset creation */
    if (H5D_COMPACT != layout->type && H5O_append(file, dxpl_id, oh, H5O_LAYOUT_ID, (alloc_time == H5D_ALLOC_TIME_EARLY ? H5O_FLAG_CONSTANT : 0), layout) < 0)
         HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update layout")

#ifdef H5O_ENABLE_BOGUS
    /*
     * Add a "bogus" message.
     */
    if (H5O_bogus_oh(file, dxpl_id, oh))<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to update 'bogus' message")
#endif /* H5O_ENABLE_BOGUS */

    /* Add a modification time message. */
    if (H5O_touch_oh(file, oh, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update modification time message")

done:
    /* Release fill value information */
    if (H5O_reset(H5O_FILL_ID, &fill) <0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to release fill-value info")

    /* Release pointer to object header itself */
    if(ent!=NULL && oh!=NULL)
        if(H5O_unprotect(ent,oh, dxpl_id)<0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to unprotect dataset object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_update_entry_info() */


/*-------------------------------------------------------------------------
 * Function:  H5D_create
 *
 * Purpose:  Creates a new dataset with name NAME in file F and associates
 *    with it a datatype TYPE for each element as stored in the
 *    file, dimensionality information or dataspace SPACE, and
 *    other miscellaneous properties CREATE_PARMS.  All arguments
 *    are deep-copied before being associated with the new dataset,
 *    so the caller is free to subsequently modify them without
 *    affecting the dataset.
 *
 * Return:  Success:  Pointer to a new dataset
 *
 *    Failure:  NULL
 *
 * Errors:
 *    DATASET    CANTINIT  Can't update dataset header.
 *    DATASET    CANTINIT  Problem with the dataset name.
 *    DATASET    CANTINIT  Fail in file space allocation for
 *          chunks
 *
 * Programmer:  Robb Matzke
 *    Thursday, December  4, 1997
 *
 * Modifications:
 *  Robb Matzke, 9 Jun 1998
 *  The data space message is no longer cached in the dataset struct.
 *
 *   Robb Matzke, 27 Jul 1998
 *  Added the MTIME message to the dataset object header.
 *
 *   Robb Matzke, 1999-10-14
 *  The names for the external file list are entered into the heap hear
 *  instead of when the efl message is encoded, preventing a possible
 *  infinite recursion situation.
 *
 *      Raymond Lu
 *      Tuesday, October 2, 2001
 *      Changed the way to retrieve and set property for generic property
 *      list.
 *
 *  Raymond Lu, 26 Feb 2002
 *  A new fill value message is added.  Two properties, space allocation
 *  time and fill value writing time, govern space allocation and fill
 *      value writing.
 *
 *      Bill Wendling, 1. November 2002
 *      Removed the cache updating mechanism. This was done so that it
 *      can be called separately from the H5D_create function. There were
 *      two of these mechanisms: one to create and insert into the parent
 *      group the H5G_entry_t object and the other to update based upon
 *      whether we're working with an external file or not. Between the
 *      two, there is a conditional call to allocate space which isn't
 *      part of updating the cache.
 *
 *      Nat Furrer and James Laird
 *      June 7, 2004
 *      Added checked_filters flag
 *
 *-------------------------------------------------------------------------
 */
static H5D_t *
H5D_create(H5G_entry_t *loc, const char *name, hid_t type_id, const H5S_t *space,
    hid_t dcpl_id, hid_t dxpl_id)
{
    const H5T_t         *type;                  /* Datatype for dataset */
    H5D_t    *new_dset = NULL;
    int            i, ndims;
    unsigned    u;
    H5F_t    *file=NULL;
    unsigned            chunk_ndims = 0;        /* Dimensionality of chunk */
    H5P_genplist_t   *dc_plist=NULL;         /* New Property list */
    hbool_t             has_vl_type=FALSE;      /* Flag to indicate a VL-type for dataset */
    hbool_t             chunk_init=FALSE;       /* Flag to indicate that chunk information was initialized */
    H5D_t    *ret_value;             /* Return value */

    FUNC_ENTER_NOAPI(H5D_create, NULL)

    /* check args */
    assert (loc);
    assert (name && *name);
    assert (H5I_DATATYPE==H5I_get_type(type_id));
    assert (space);
    assert (H5I_GENPROP_LST==H5I_get_type(dcpl_id));
    assert (H5I_GENPROP_LST==H5I_get_type(dxpl_id));

    /* Get the dataset's datatype */
    if (NULL == (type = H5I_object(type_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a datatype")

    /* Check if the datatype is "sensible" for use in a dataset */
    if(H5T_is_sensible(type)!=TRUE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "datatype is not sensible")

    /* Check if the datatype is/contains a VL-type */
    if(H5T_detect_class(type, H5T_VLEN))
        has_vl_type=TRUE;

    /* Check if the dataspace has an extent set (or is NULL) */
    if( !(H5S_has_extent(space)) )
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "dataspace extent has not been set.")

    /* Initialize the dataset object */
    if (NULL==(new_dset = H5FL_CALLOC(H5D_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    H5G_ent_reset(&(new_dset->ent));

    /* Initialize the shared dataset space */
    if(NULL == (new_dset->shared = H5D_new(dcpl_id,TRUE,has_vl_type)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* What file is the dataset being added to? */
    if(NULL==(file=H5G_insertion_file(loc, name, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to locate insertion point")

    /* Copy datatype for dataset */
    if(H5D_init_type(file, new_dset, type_id, type)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy datatype")

    /* Check if the filters in the DCPL can be applied to this dataset */
    if(H5Z_can_apply(new_dset->shared->dcpl_id,new_dset->shared->type_id)<0)
        HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, NULL, "I/O filters can't operate on this dataset")

    /* Set the dataset's checked_filters flag to enable writing */
    new_dset->shared->checked_filters = TRUE;

    /* Copy dataspace for dataset */
    if((new_dset->shared->space = H5S_copy(space, FALSE))==NULL)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy dataspace")

    /* Set the dataset's dataspace to 'all' selection */
    if(H5S_select_all(new_dset->shared->space,1)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTSET, NULL, "unable to set all selection")

    /* Make the "set local" filter callbacks for this dataset */
    if(H5Z_set_local(new_dset->shared->dcpl_id,new_dset->shared->type_id)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to set local filter parameters")

    /* Check if the dataset has a non-default DCPL & get important values, if so */
    if(new_dset->shared->dcpl_id!=H5P_DATASET_CREATE_DEFAULT) {
        H5D_layout_t    dcpl_layout;    /* Dataset's layout information */
        H5O_pline_t     dcpl_pline;     /* Dataset's I/O pipeline information */
        H5D_alloc_time_t alloc_time;    /* Dataset's allocation time */

        /* Get new dataset's property list object */
        if (NULL == (dc_plist = H5I_object(new_dset->shared->dcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "can't get dataset creation property list")

        if(H5P_get(dc_plist, H5D_CRT_DATA_PIPELINE_NAME, &dcpl_pline) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't retrieve pipeline filter")
        if(H5P_get(dc_plist, H5D_CRT_LAYOUT_NAME, &dcpl_layout) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't retrieve layout")
        if(dcpl_pline.nused > 0 && H5D_CHUNKED != dcpl_layout)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "filters can only be used with chunked layout")
        if(H5P_get(dc_plist, H5D_CRT_ALLOC_TIME_NAME, &alloc_time) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't retrieve space allocation time")

        /* Check if the alloc_time is the default and error out */
        if(alloc_time==H5D_ALLOC_TIME_DEFAULT)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "invalid space allocation state")

        /* Don't allow compact datasets to allocate space later */
        if(dcpl_layout==H5D_COMPACT && alloc_time!=H5D_ALLOC_TIME_EARLY)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "compact dataset doesn't support late space allocation")

        /* Set the alloc_time for the dataset, in case the default was used */
        new_dset->shared->alloc_time=alloc_time;

        /* If MPI VFD is used, no filter support yet. */
        if(IS_H5FD_MPI(file) && dcpl_pline.nused > 0)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, NULL, "Parallel I/O does not support filters yet")

        /* Chunked datasets are non-default, so retrieve their info here */
        if(H5P_get(dc_plist, H5D_CRT_CHUNK_DIM_NAME, &chunk_ndims) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't retrieve chunk dimensions")

        /* Get the dataset's external file list information */
        if(H5P_get(dc_plist, H5D_CRT_EXT_FILE_LIST_NAME, &new_dset->shared->efl) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't retrieve external file list")

        /* Set the dataset's data storage method */
        new_dset->shared->layout.type=dcpl_layout;
    } /* end if */

    /* Check if this dataset is going into a parallel file and set space allocation time */
    if(IS_H5FD_MPI(file))
        new_dset->shared->alloc_time=H5D_ALLOC_TIME_EARLY;

    /* Set up layout information */
    new_dset->shared->layout.unused.ndims = H5S_GET_EXTENT_NDIMS(new_dset->shared->space) + 1;
    assert((unsigned)(new_dset->shared->layout.unused.ndims) <= NELMTS(new_dset->shared->layout.unused.dim));
    if (H5S_get_simple_extent_dims(new_dset->shared->space, new_dset->shared->layout.unused.dim, NULL)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to initialize storage info")
    new_dset->shared->layout.unused.dim[new_dset->shared->layout.unused.ndims-1] = H5T_get_size(new_dset->shared->type);

    switch (new_dset->shared->layout.type) {
        case H5D_CONTIGUOUS:
            {
                hssize_t tmp_size;                      /* Temporary holder for raw data size */
                hsize_t  max_dim[H5O_LAYOUT_NDIMS];      /* Maximum size of data in elements */

                /*
                 * The maximum size of the dataset cannot exceed the storage size.
                 * Also, only the slowest varying dimension of a simple data space
                 * can be extendible (currently only for external data storage).
                 */
                new_dset->shared->layout.u.contig.addr = HADDR_UNDEF;        /* Initialize to no address */

                if ((ndims=H5S_get_simple_extent_dims(new_dset->shared->space, NULL, max_dim))<0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to initialize contiguous storage")
                for (i=1; i<ndims; i++)
                    if (max_dim[i]>new_dset->shared->layout.unused.dim[i])
                        HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "only the first dimension can be extendible")
                if (new_dset->shared->efl.nused>0) {
                    hsize_t max_points = H5S_get_npoints_max (new_dset->shared->space);
                    hsize_t max_storage = H5O_efl_total_size (&new_dset->shared->efl);

                    if (H5S_UNLIMITED==max_points) {
                        if (H5O_EFL_UNLIMITED!=max_storage)
                            HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "unlimited data space but finite storage")
                    } else if (max_points * H5T_get_size (type) < max_points) {
                        HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "data space * type size overflowed")
                    } else if (max_points * H5T_get_size (type) > max_storage) {
                        HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "data space size exceeds external storage size")
                    }

                    /* Set the I/O functions for this layout type */
                    new_dset->shared->io_ops.readvv=H5D_efl_readvv;
                    new_dset->shared->io_ops.writevv=H5D_efl_writevv;
                } /* end if */
                else {
                    if (ndims>0 && max_dim[0]>new_dset->shared->layout.unused.dim[0])
                        HGOTO_ERROR (H5E_DATASET, H5E_UNSUPPORTED, NULL, "extendible contiguous non-external dataset")

                    /* Set the I/O functions for this layout type */
                    new_dset->shared->io_ops.readvv=H5D_contig_readvv;
                    new_dset->shared->io_ops.writevv=H5D_contig_writevv;
                } /* end else */

                /* Compute the total size of a chunk */
                tmp_size = H5S_GET_EXTENT_NPOINTS(new_dset->shared->space) *
                                        H5T_get_size(new_dset->shared->type);
                H5_ASSIGN_OVERFLOW(new_dset->shared->layout.u.contig.size,tmp_size,hssize_t,hsize_t);

                /* Get the sieve buffer size for this dataset */
                new_dset->shared->cache.contig.sieve_buf_size = H5F_SIEVE_BUF_SIZE(loc->file);
            } /* end case */
            break;

        case H5D_CHUNKED:
            {
                hsize_t  max_dim[H5O_LAYOUT_NDIMS];      /* Maximum size of data in elements */

                /* Set up layout information */
                if((ndims=H5S_GET_EXTENT_NDIMS(new_dset->shared->space))<0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "unable to get rank")
                new_dset->shared->layout.u.chunk.ndims = (unsigned)ndims + 1;
                assert((unsigned)(new_dset->shared->layout.u.chunk.ndims) <= NELMTS(new_dset->shared->layout.u.chunk.dim));

                new_dset->shared->layout.u.chunk.addr = HADDR_UNDEF;        /* Initialize to no address */

                /*
                 * Chunked storage allows any type of data space extension, so we
                 * don't even bother checking.
                 */
                if(chunk_ndims != (unsigned)ndims)
                    HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "dimensionality of chunks doesn't match the data space")
                if (new_dset->shared->efl.nused>0)
                    HGOTO_ERROR (H5E_DATASET, H5E_BADVALUE, NULL, "external storage not supported with chunked layout")

                /*
                 * The chunk size of a dimension with a fixed size cannot exceed
                 * the maximum dimension size
                 */
                if(H5P_get(dc_plist, H5D_CRT_CHUNK_SIZE_NAME, new_dset->shared->layout.u.chunk.dim) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't retrieve chunk size")
                new_dset->shared->layout.u.chunk.dim[new_dset->shared->layout.u.chunk.ndims-1] = H5T_get_size(new_dset->shared->type);

                if (H5S_get_simple_extent_dims(new_dset->shared->space, NULL, max_dim)<0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to query maximum dimensions")
                for (u=0; u<new_dset->shared->layout.u.chunk.ndims-1; u++)
                    if(max_dim[u] != H5S_UNLIMITED && max_dim[u] < new_dset->shared->layout.u.chunk.dim[u])
                        HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "chunk size must be <= maximum dimension size for fixed-sized dimensions")

                /* Compute the total size of a chunk */
                for (u=1, new_dset->shared->layout.u.chunk.size=new_dset->shared->layout.u.chunk.dim[0]; u<new_dset->shared->layout.u.chunk.ndims; u++)
                    new_dset->shared->layout.u.chunk.size *= new_dset->shared->layout.u.chunk.dim[u];

                /* Set the I/O functions for this layout type */
                new_dset->shared->io_ops.readvv=H5D_istore_readvv;
                new_dset->shared->io_ops.writevv=H5D_istore_writevv;

                /* Initialize the chunk cache for the dataset */
                if(H5D_istore_init(file,new_dset)<0)
                    HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "can't initialize chunk cache")

                /* Indicate that the chunk information was initialized */
                chunk_init=TRUE;
            } /* end case */
            break;

        case H5D_COMPACT:
            {
                hssize_t tmp_size;      /* Temporary holder for raw data size */
                hsize_t  comp_data_size;

                /*
                 * Compact dataset is stored in dataset object header message of
                 * layout.
                 */
                tmp_size = H5S_GET_EXTENT_NPOINTS(space) *
                                        H5T_get_size(new_dset->shared->type);
                H5_ASSIGN_OVERFLOW(new_dset->shared->layout.u.compact.size,tmp_size,hssize_t,size_t);

                /* Verify data size is smaller than maximum header message size
                 * (64KB) minus other layout message fields.
                 */
                comp_data_size=H5O_MAX_SIZE-H5O_layout_meta_size(file, &(new_dset->shared->layout));
                if(new_dset->shared->layout.u.compact.size > comp_data_size)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "compact dataset size is bigger than header message maximum size")

                /* Set the I/O functions for this layout type */
                new_dset->shared->io_ops.readvv=H5D_compact_readvv;
                new_dset->shared->io_ops.writevv=H5D_compact_writevv;
            } /* end case */
            break;

        default:
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, NULL, "not implemented yet")
    } /* end switch */ /*lint !e788 All appropriate cases are covered */

    /* Update the dataset's entry info. */
    if (H5D_update_entry_info(file, dxpl_id, new_dset, dc_plist) != SUCCEED)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't update the metadata cache")

    /* Get the dataset's DCPL cache info */
    if (H5D_get_dcpl_cache(new_dset->shared->dcpl_id,&new_dset->shared->dcpl_cache)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't fill DCPL cache")

    /*
     * Give the dataset a name.  That is, create and add a new object to the
     * group this dataset is being initially created in.
     */
    if (H5G_insert(loc, name, &new_dset->ent, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to name dataset")

    /* Add the dataset to the list of opened objects in the file */
    if(H5FO_top_incr(new_dset->ent.file, new_dset->ent.header)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't incr object ref. count")
    if(H5FO_insert(new_dset->ent.file,new_dset->ent.header,new_dset->shared)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, NULL, "can't insert dataset into list of open objects")

    new_dset->shared->fo_count=1;

    /* Success */
    ret_value = new_dset;

done:
    if (!ret_value && new_dset && new_dset->shared) {
        if( new_dset->shared) {
            if(new_dset->shared->layout.type==H5D_CHUNKED && chunk_init) {
                if(H5D_istore_dest(new_dset,H5AC_dxpl_id)<0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, NULL, "unable to destroy chunk cache")
            } /* end if */
            if (new_dset->shared->space) {
                if(H5S_close(new_dset->shared->space)<0)
                    HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release dataspace")
            } /* end if */
            if (new_dset->shared->type) {
                if(H5I_dec_ref(new_dset->shared->type_id)<0)
                    HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release datatype")
            } /* end if */
            if (H5F_addr_defined(new_dset->ent.header)) {
                if(H5O_close(&(new_dset->ent))<0)
                    HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release object header")
                if(file) {
                    if(H5O_delete(file, dxpl_id,new_dset->ent.header)<0)
                        HDONE_ERROR(H5E_DATASET, H5E_CANTDELETE, NULL, "unable to delete object header")
                } /* end if */
            } /* end if */
            if(new_dset->shared->dcpl_id!=0) {
                if(H5I_dec_ref(new_dset->shared->dcpl_id)<0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, NULL, "unable to decrement ref count on property list")
            } /* end if */
            H5FL_FREE(H5D_shared_t,new_dset->shared);
        } /* end if */
        new_dset->ent.file = NULL;
        H5FL_FREE(H5D_t, new_dset);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_create() */


/*-------------------------------------------------------------------------
 * Function:  H5D_isa
 *
 * Purpose:  Determines if an object has the requisite messages for being
 *    a dataset.
 *
 * Return:  Success:  TRUE if the required dataset messages are
 *        present; FALSE otherwise.
 *
 *    Failure:  FAIL if the existence of certain messages
 *        cannot be determined.
 *
 * Programmer:  Robb Matzke
 *              Monday, November  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5D_isa(H5G_entry_t *ent, hid_t dxpl_id)
{
    htri_t  exists;
    htri_t  ret_value=TRUE;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_isa, FAIL)

    assert(ent);

    /* Data type */
    if ((exists=H5O_exists(ent, H5O_DTYPE_ID, 0, dxpl_id))<0) {
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to read object header")
    } else if (!exists) {
  HGOTO_DONE(FALSE)
    }

    /* Layout */
    if ((exists=H5O_exists(ent, H5O_LAYOUT_ID, 0, dxpl_id))<0) {
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to read object header")
    } else if (!exists) {
  HGOTO_DONE(FALSE)
    }


done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*
 *-------------------------------------------------------------------------
 * Function:  H5D_open
 *
 * Purpose:  Checks if dataset is already open, or opens a dataset for
 *              access.
 *
 * Return:  Success:  Dataset ID
 *    Failure:  FAIL
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Friday, December 20, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5D_t*
H5D_open(const H5G_entry_t *ent, hid_t dxpl_id)
{
    H5D_shared_t    *shared_fo=NULL;
    H5D_t           *dataset=NULL;
    H5D_t           *ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5D_open, NULL)

    /* check args */
    assert (ent);

    /* Check if dataset was already open */
    if((shared_fo=H5FO_opened(ent->file,ent->header))==NULL) {
        /* Clear any errors from H5FO_opened() */
        H5E_clear();

        /* Open the dataset object */
        if ((dataset=H5D_open_oid(ent, dxpl_id)) ==NULL)
            HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, NULL, "not found")

        /* Add the dataset to the list of opened objects in the file */
        if(H5FO_insert(dataset->ent.file,dataset->ent.header,dataset->shared)<0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, NULL, "can't insert dataset into list of open objects")

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(dataset->ent.file, dataset->ent.header) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't increment object count")

        dataset->shared->fo_count = 1;
    } /* end if */
    else {
        if(NULL == (dataset = H5FL_CALLOC(H5D_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "can't allocate space for dataset")

        /* Shallow copy (take ownership) of the group entry object */
        if(H5G_ent_copy(&(dataset->ent),ent,H5G_COPY_SHALLOW)<0)
            HGOTO_ERROR (H5E_DATASET, H5E_CANTCOPY, NULL, "can't copy group entry")

        dataset->shared=shared_fo;

        shared_fo->fo_count++;

        /* Check if the object has been opened through the top file yet */
        if(H5FO_top_count(dataset->ent.file, dataset->ent.header) == 0) {
            /* Open the object through this top file */
            if(H5O_open(&(dataset->ent)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, NULL, "unable to open object header")
        } /* end if */

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(dataset->ent.file, dataset->ent.header) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't increment object count")
    } /* end else */

    ret_value = dataset;

done:
    if(ret_value==NULL) {
        if(dataset) {
            if(shared_fo==NULL)   /* Need to free shared fo */
                H5FL_FREE(H5D_shared_t, dataset->shared);
            H5FL_FREE(H5D_t, dataset);
        }
        if(shared_fo)
            shared_fo->fo_count--;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_open_oid
 *
 * Purpose:  Opens a dataset for access.
 *
 * Return:  Dataset pointer on success, NULL on failure
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Monday, October 12, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to set property for generic property list.
 *
 *    Raymond Lu
 *    Feb 26, 2002
 *    A new fill value message and two new properties are added.
 *
 *              Pedro Vicente, <pvn@ncsa.uiuc.edu> 22 Aug 2002
 *              Added a deep copy of the symbol table entry
 *
 *-------------------------------------------------------------------------
 */
static H5D_t *
H5D_open_oid(const H5G_entry_t *ent, hid_t dxpl_id)
{
    H5D_t   *dataset = NULL;  /*new dataset struct     */
    H5O_fill_new_t  fill = {NULL, 0, NULL, H5D_ALLOC_TIME_LATE, H5D_CRT_FILL_TIME_DEF, TRUE};
    unsigned    alloc_time_state;       /* Allocation time state */
    H5O_fill_t     *fill_prop;          /* Pointer to dataset's fill value area */
    H5O_pline_t  pline;                 /* I/O pipeline information */
    H5P_genplist_t *plist;              /* Property list */
    H5D_t   *ret_value = NULL;  /*return value      */

    FUNC_ENTER_NOAPI(H5D_open_oid, NULL)

    /* check args */
    assert (ent);

    /* Allocate the dataset structure */
    if(NULL==(dataset=H5FL_CALLOC(H5D_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* (Set the 'vl_type' parameter to FALSE since it doesn't matter from here) */
    if(NULL==(dataset->shared = H5D_new(H5P_DATASET_CREATE_DEFAULT,FALSE,FALSE)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Shallow copy (take ownership) of the group entry object */
    if(H5G_ent_copy(&(dataset->ent),ent,H5G_COPY_SHALLOW)<0)
        HGOTO_ERROR (H5E_DATASET, H5E_CANTCOPY, NULL, "can't copy group entry")

    /* Find the dataset object */
    if (H5O_open(&(dataset->ent)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, NULL, "unable to open")

    /* Get the type and space */
    if (NULL==(dataset->shared->type=H5O_read(&(dataset->ent), H5O_DTYPE_ID, 0, NULL, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to load type info from dataset header")
    /* Get a datatype ID for the dataset's datatype */
    if((dataset->shared->type_id = H5I_register(H5I_DATATYPE, dataset->shared->type))<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, NULL, "unable to register type")

    if (NULL==(dataset->shared->space=H5S_read(&(dataset->ent),dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to load space info from dataset header")

    /* Get dataset creation property list object */
    if (NULL == (plist = H5I_object(dataset->shared->dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "can't get dataset creation property list")

    /* Get the optional filters message */
    if(NULL == H5O_read(&(dataset->ent), H5O_PLINE_ID, 0, &pline, dxpl_id)) {
        H5E_clear();
        HDmemset(&pline, 0, sizeof(pline));
    }
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set pipeline")

    /*
     * Get the raw data layout info.  It's actually stored in two locations:
     * the storage message of the dataset (dataset->storage) and certain
     * values are copied to the dataset create plist so the user can query
     * them.
     */
    if (NULL==H5O_read(&(dataset->ent), H5O_LAYOUT_ID, 0, &(dataset->shared->layout), dxpl_id))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to read data layout message")
    if(H5P_set(plist, H5D_CRT_LAYOUT_NAME, &dataset->shared->layout.type) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set layout")
    switch (dataset->shared->layout.type) {
        case H5D_CONTIGUOUS:
            /* Compute the size of the contiguous storage for versions of the
             * layout message less than version 3 because versions 1 & 2 would
             * truncate the dimension sizes to 32-bits of information. - QAK 5/26/04
             */
            if(dataset->shared->layout.version<3) {
                hssize_t tmp_size;                      /* Temporary holder for raw data size */

                tmp_size = H5S_GET_EXTENT_NPOINTS(dataset->shared->space) *
                                        H5T_get_size(dataset->shared->type);
                H5_ASSIGN_OVERFLOW(dataset->shared->layout.u.contig.size,tmp_size,hssize_t,hsize_t);
            } /* end if */

            /* Set the I/O functions for this layout type */
            dataset->shared->io_ops.readvv=H5D_contig_readvv;
            dataset->shared->io_ops.writevv=H5D_contig_writevv;

            /* Get the sieve buffer size for this dataset */
            dataset->shared->cache.contig.sieve_buf_size = H5F_SIEVE_BUF_SIZE(dataset->ent.file);
            break;

        case H5D_CHUNKED:
            /*
             * Chunked storage.  The creation plist's dimension is one less than
             * the chunk dimension because the chunk includes a dimension for the
             * individual bytes of the data type.
             */
            {
                unsigned     chunk_ndims;           /* Dimensionality of chunk */

                chunk_ndims  = dataset->shared->layout.u.chunk.ndims - 1;

                if(H5P_set(plist, H5D_CRT_CHUNK_DIM_NAME, &chunk_ndims) < 0)
                     HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set chunk dimensions")
                if(H5P_set(plist, H5D_CRT_CHUNK_SIZE_NAME, dataset->shared->layout.u.chunk.dim) < 0)
                     HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set chunk size")

                /* Initialize the chunk cache for the dataset */
                if(H5D_istore_init(dataset->ent.file,dataset)<0)
                    HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, NULL, "can't initialize chunk cache")
            }

            /* Set the I/O functions for this layout type */
            dataset->shared->io_ops.readvv=H5D_istore_readvv;
            dataset->shared->io_ops.writevv=H5D_istore_writevv;
            break;

        case H5D_COMPACT:
            /* Set the I/O functions for this layout type */
            dataset->shared->io_ops.readvv=H5D_compact_readvv;
            dataset->shared->io_ops.writevv=H5D_compact_writevv;
            break;

        default:
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, NULL, "not implemented yet")
    } /* end switch */ /*lint !e788 All appropriate cases are covered */

    /* Point at dataset's copy, to cache it for later */
    fill_prop=&dataset->shared->fill;

    /* Retrieve & release the previous fill-value settings */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, fill_prop) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't get fill value")
    if(H5O_reset(H5O_FILL_ID, fill_prop)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, NULL, "can't release fill info")

    /* Get the new fill value message */
    if(NULL == H5O_read(&(dataset->ent), H5O_FILL_NEW_ID, 0, &fill, dxpl_id)) {
        H5E_clear();
        HDmemset(&fill, 0, sizeof(fill));

        /* Set the space allocation time appropriately, based on the type of dataset storage */
        switch (dataset->shared->layout.type) {
            case H5D_COMPACT:
                fill.alloc_time=H5D_ALLOC_TIME_EARLY;
                break;

            case H5D_CONTIGUOUS:
                fill.alloc_time=H5D_ALLOC_TIME_LATE;
                break;

            case H5D_CHUNKED:
                fill.alloc_time=H5D_ALLOC_TIME_INCR;
                break;

            default:
                HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, NULL, "not implemented yet")
        } /* end switch */ /*lint !e788 All appropriate cases are covered */

        /* Set the default fill time */
        fill.fill_time=H5D_CRT_FILL_TIME_DEF;
    } /* end if */
    if(fill.fill_defined) {
        if(NULL==H5O_copy(H5O_FILL_ID, &fill, fill_prop))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't copy fill value")
    } else {
  /* For compatibility with v1.4.  Retrieve the old fill value message.
    * If size is 0, make it -1 for undefined. */
        if(NULL == H5O_read(&(dataset->ent), H5O_FILL_ID, 0, fill_prop, dxpl_id)) {
            H5E_clear();
            HDmemset(fill_prop, 0, sizeof(H5O_fill_t));
        }
        if(fill_prop->size == 0) {
      fill_prop->type = fill_prop->buf = NULL;
      fill_prop->size = (size_t)-1;
  }
    } /* end else */
    alloc_time_state=0;
    if( (dataset->shared->layout.type==H5D_COMPACT && fill.alloc_time==H5D_ALLOC_TIME_EARLY)
            || (dataset->shared->layout.type==H5D_CONTIGUOUS && fill.alloc_time==H5D_ALLOC_TIME_LATE)
            || (dataset->shared->layout.type==H5D_CHUNKED && fill.alloc_time==H5D_ALLOC_TIME_INCR))
        alloc_time_state=1;

    /* Set revised fill value properties */
    if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, fill_prop) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set fill value")
    dataset->shared->alloc_time=fill.alloc_time;        /* Cache this for later */
    if(H5P_set(plist, H5D_CRT_ALLOC_TIME_NAME, &fill.alloc_time) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set allocation time")
    if(H5P_set(plist, H5D_CRT_ALLOC_TIME_STATE_NAME, &alloc_time_state) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set allocation time state")
    dataset->shared->fill_time=fill.fill_time;          /* Cache this for later */
    if(H5P_set(plist, H5D_CRT_FILL_TIME_NAME, &fill.fill_time) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set fill time")

    /* Get the external file list message, which might not exist.  Space is
     * also undefined when space allocate time is H5D_ALLOC_TIME_LATE. */
    if((dataset->shared->layout.type==H5D_CONTIGUOUS && !H5F_addr_defined(dataset->shared->layout.u.contig.addr))
            || (dataset->shared->layout.type==H5D_CHUNKED && !H5F_addr_defined(dataset->shared->layout.u.chunk.addr))) {
        HDmemset(&dataset->shared->efl,0,sizeof(H5O_efl_t));
        if(NULL != H5O_read(&(dataset->ent), H5O_EFL_ID, 0, &dataset->shared->efl, dxpl_id)) {
            if(H5P_set(plist, H5D_CRT_EXT_FILE_LIST_NAME, &dataset->shared->efl) < 0)
              HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set external file list")

            /* Override the I/O functions for this layout type */
            dataset->shared->io_ops.readvv=H5D_efl_readvv;
            dataset->shared->io_ops.writevv=H5D_efl_writevv;
        } /* end if */
    } /* end if */

    /*
     * Make sure all storage is properly initialized.
     * This is important only for parallel I/O where the space must
     * be fully allocated before I/O can happen.
     */
    if ((H5F_get_intent(dataset->ent.file) & H5F_ACC_RDWR)
            && ((dataset->shared->layout.type==H5D_CONTIGUOUS && !H5F_addr_defined(dataset->shared->layout.u.contig.addr))
                || (dataset->shared->layout.type==H5D_CHUNKED && !H5F_addr_defined(dataset->shared->layout.u.chunk.addr)))
            && IS_H5FD_MPI(dataset->ent.file)) {
        if (H5D_alloc_storage(dataset->ent.file, dxpl_id, dataset,H5D_ALLOC_OPEN, TRUE, FALSE)<0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to initialize file storage")
    }

    /* Get the dataset's DCPL cache info */
    if (H5D_get_dcpl_cache(dataset->shared->dcpl_id,&dataset->shared->dcpl_cache)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't fill DCPL cache")

    /* Success */
    ret_value = dataset;

done:
    /* Release fill value information */
    if (H5O_reset(H5O_FILL_ID, &fill) <0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, NULL, "unable to release fill-value info")

    if (ret_value==NULL && dataset) {
        if (H5F_addr_defined(dataset->ent.header)) {
            if(H5O_close(&(dataset->ent))<0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release object header")
        } /* end if */
        if (dataset->shared->space) {
            if(H5S_close(dataset->shared->space)<0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release dataspace")
        } /* end if */
        if (dataset->shared->type) {
            if(H5I_dec_ref(dataset->shared->type_id)<0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release datatype")
        } /* end if */
        dataset->ent.file = NULL;
        H5FL_FREE(H5D_t,dataset);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_open_oid() */


/*-------------------------------------------------------------------------
 * Function:  H5D_close
 *
 * Purpose:  Insures that all data has been saved to the file, closes the
 *    dataset object header, and frees all resources used by the
 *    descriptor.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Errors:
 *    DATASET    CANTINIT  Couldn't free the type or space,
 *          but the dataset was freed anyway.
 *
 * Programmer:  Robb Matzke
 *    Thursday, December  4, 1997
 *
 * Modifications:
 *  Robb Matzke, 9 Jun 1998
 *  The data space message is no longer cached in the dataset struct.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_close(H5D_t *dataset)
{
    unsigned                free_failed=FALSE;
    herr_t                  ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5D_close, FAIL)

    /* check args */
    assert(dataset && dataset->ent.file && dataset->shared);
    assert(dataset->shared->fo_count >0);

    /* Dump debugging info */
#ifdef H5D_ISTORE_DEBUG
    H5D_istore_stats(dataset, FALSE);
#endif /* H5F_ISTORE_DEBUG */

    dataset->shared->fo_count--;
    if(dataset->shared->fo_count ==0)
    {
        /* Free the data sieve buffer, if it's been allocated */
        if(dataset->shared->cache.contig.sieve_buf) {
            assert(dataset->shared->layout.type!=H5D_COMPACT);      /* We should never have a sieve buffer for compact storage */

            /* Flush the raw data buffer, if its dirty */
            if (dataset->shared->cache.contig.sieve_dirty) {
                /* Write dirty data sieve buffer to file */
                if (H5F_block_write(dataset->ent.file, H5FD_MEM_DRAW, dataset->shared->cache.contig.sieve_loc,
                        dataset->shared->cache.contig.sieve_size, H5AC_dxpl_id, dataset->shared->cache.contig.sieve_buf) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "block write failed")

                /* Reset sieve buffer dirty flag */
                dataset->shared->cache.contig.sieve_dirty=0;
            } /* end if */

            dataset->shared->cache.contig.sieve_buf = H5FL_BLK_FREE (sieve_buf,dataset->shared->cache.contig.sieve_buf);
        } /* end if */

        /* Free cached information for each kind of dataset */
        switch(dataset->shared->layout.type) {
            case H5D_CONTIGUOUS:
                break;

            case H5D_CHUNKED:
                /* Flush and destroy chunks in the cache */
                if(H5D_istore_dest(dataset,H5AC_dxpl_id)<0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to destroy chunk cache")
                break;

            case H5D_COMPACT:
                /* Update header message of layout for compact dataset. */
                if(dataset->shared->layout.u.compact.dirty) {
                    if(H5O_modify(&(dataset->ent), H5O_LAYOUT_ID, 0, 0, H5O_UPDATE_TIME, &(dataset->shared->layout), H5AC_dxpl_id)<0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to update layout message")
                    dataset->shared->layout.u.compact.dirty = FALSE;
                } /* end if */

                /* Free the buffer for the raw data for compact datasets */
                dataset->shared->layout.u.compact.buf=H5MM_xfree(dataset->shared->layout.u.compact.buf);
                break;

            default:
                assert ("not implemented yet" && 0);
#ifdef NDEBUG
                HGOTO_ERROR (H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
        } /* end switch */ /*lint !e788 All appropriate cases are covered */

        /*
        * Release datatype, dataspace and creation property list -- there isn't
        * much we can do if one of these fails, so we just continue.
        */
        free_failed=(H5I_dec_ref(dataset->shared->type_id)<0 || H5S_close(dataset->shared->space)<0 ||
                          H5I_dec_ref(dataset->shared->dcpl_id) < 0);

        /* Remove the dataset from the list of opened objects in the file */
        if(H5FO_top_decr(dataset->ent.file, dataset->ent.header) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't decrement count for object")
        if(H5FO_delete(dataset->ent.file, H5AC_dxpl_id, dataset->ent.header)<0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't remove dataset from list of open objects")

        /* Close the dataset object */
        /* (This closes the file, if this is the last object open) */
        if(H5O_close(&(dataset->ent))<0)
            HGOTO_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release object header");

        /*
        * Free memory.  Before freeing the memory set the file pointer to NULL.
        * We always check for a null file pointer in other H5D functions to be
        * sure we're not accessing an already freed dataset (see the assert()
        * above).
        */
        dataset->ent.file = NULL;

        H5FL_FREE(H5D_shared_t,dataset->shared);
    } /* end if */
    else
    {
        /* Decrement the ref. count for this object in the top file */
        if(H5FO_top_decr(dataset->ent.file, dataset->ent.header) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't decrement count for object")

        /* Check reference count for this object in the top file */
        if(H5FO_top_count(dataset->ent.file, dataset->ent.header) == 0)
            if(H5O_close(&(dataset->ent)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to close")

       if(H5G_free_ent_name(&dataset->ent)<0)
           free_failed=TRUE;
    } /* end else */

    H5FL_FREE(H5D_t,dataset);

    if (free_failed)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "couldn't free the type or creation property list, but the dataset was freed anyway.")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_extend
 *
 * Purpose:  Increases the size of a dataset.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Friday, January 30, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to retrieve property for generic property
 *              list.
 *
 *              Nat Furrer and James Laird
 *              June 17, 2004
 *              Added check for filter encode capability
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_extend (H5D_t *dataset, const hsize_t *size, hid_t dxpl_id)
{
    int  changed;                        /* Flag to indicate that the dataspace was successfully extended */
    H5S_t  *space = NULL;          /* Dataset's dataspace */
    H5D_fill_value_t fill_status;
    H5D_fill_time_t  fill_time;
    herr_t  ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5D_extend, FAIL)

    /* Check args */
    assert (dataset);
    assert (size);

    /* Check if the filters in the DCPL will need to encode, and if so, can they?
     * Filters need encoding if fill value is defined and a fill policy is set that requires
     * writing on an extend.
     */
    if(! dataset->shared->checked_filters)
    {
        if(H5P_is_fill_value_defined(&(dataset->shared->fill), &fill_status) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Couldn't retrieve fill value from dataset.");

        if(fill_status == H5D_FILL_VALUE_DEFAULT || fill_status == H5D_FILL_VALUE_USER_DEFINED)
        {
            if( H5Pget_fill_time(dataset->shared->dcpl_id, &fill_time) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Couldn't retrieve fill time from dataset.");

            if(fill_time == H5D_FILL_TIME_ALLOC ||
                    (fill_time == H5D_FILL_TIME_IFSET && fill_status == H5D_FILL_VALUE_USER_DEFINED) )
            {
                /* Filters must have encoding enabled. Ensure that all filters can be applied */
                if(H5Z_can_apply(dataset->shared->dcpl_id, dataset->shared->type_id) <0)
                    HGOTO_ERROR(H5E_PLINE, H5E_CANAPPLY, FAIL, "can't apply filters")

                dataset->shared->checked_filters = TRUE;
            }
        }
    }

    /*
     * NOTE: Restrictions on extensions were checked when the dataset was
     *       created.  All extensions are allowed here since none should be
     *       able to muck things up.
     */

    /* Increase the size of the data space */
    space=dataset->shared->space;
    if ((changed=H5S_extend (space, size))<0)
  HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to increase size of data space")

    if (changed>0){
  /* Save the new dataspace in the file if necessary */
  if (H5S_modify (&(dataset->ent), space, TRUE, dxpl_id)<0)
      HGOTO_ERROR (H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update file with new dataspace")

        /* Update the index values for the cached chunks for this dataset */
        if(H5D_CHUNKED == dataset->shared->layout.type)
            if(H5D_istore_update_cache(dataset, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update cached chunk indices")

  /* Allocate space for the new parts of the dataset, if appropriate */
        if(dataset->shared->alloc_time==H5D_ALLOC_TIME_EARLY)
            if (H5D_alloc_storage(dataset->ent.file, dxpl_id, dataset, H5D_ALLOC_EXTEND, TRUE, FALSE)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize dataset with fill value")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_entof
 *
 * Purpose:  Returns a pointer to the entry for a dataset.
 *
 * Return:  Success:  Ptr to entry
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, April 24, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_entry_t *
H5D_entof (H5D_t *dataset)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_entof)

    FUNC_LEAVE_NOAPI( dataset ? &(dataset->ent) : (H5G_entry_t *)NULL)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_typeof
 *
 * Purpose:  Returns a pointer to the dataset's data type.  The data type
 *    is not copied.
 *
 * Return:  Success:  Ptr to the dataset's data type, uncopied.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, June  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5D_typeof (const H5D_t *dset)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_typeof)

    assert (dset);
    assert (dset->shared->type);

    FUNC_LEAVE_NOAPI(dset->shared->type)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_get_file
 *
 * Purpose:   Returns the dataset's file pointer.
 *
 * Return:    Success:        Ptr to the dataset's file pointer.
 *
 *            Failure:        NULL
 *
 * Programmer:        Quincey Koziol
 *              Thursday, October 22, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5F_t *
H5D_get_file (const H5D_t *dset)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_get_file)

    assert (dset);
    assert (dset->ent.file);

    FUNC_LEAVE_NOAPI(dset->ent.file)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_alloc_storage
 *
 * Purpose:  Allocate storage for the raw data of a dataset.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January 16, 1998
 *
 * Modifications:
 *              Quincey Koziol
 *              Thursday, August 22, 2002
 *              Moved here from H5F_arr_create and moved more logic into
 *              this function from places where it was being called.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_alloc_storage (H5F_t *f, hid_t dxpl_id, H5D_t *dset/*in,out*/, H5D_time_alloc_t time_alloc,
    hbool_t update_time, hbool_t full_overwrite)
{
    struct H5O_layout_t *layout;        /* The dataset's layout information */
    unsigned init_space=0;              /* Flag to indicate that space should be initialized */
    unsigned addr_set=0;                /* Flag to indicate that the dataset's storage address was set */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_alloc_storage)

    /* check args */
    assert (f);
    assert (dset);

    /* If the data is stored in external files, don't set an address for the layout
     * We assume that external storage is already
     * allocated by the caller, or at least will be before I/O is performed.
     */
    if(dset->shared->efl.nused==0) {
        /* Get a pointer to the dataset's layout information */
        layout=&(dset->shared->layout);

        switch (layout->type) {
            case H5D_CONTIGUOUS:
                if(layout->u.contig.addr==HADDR_UNDEF) {
                    /* Reserve space in the file for the entire array */
                    if (H5D_contig_create (f, dxpl_id, layout/*out*/)<0)
                        HGOTO_ERROR (H5E_IO, H5E_CANTINIT, FAIL, "unable to initialize contiguous storage")

                    /* Indicate that we set the storage addr */
                    addr_set=1;

                    /* Indicate that we should initialize storage space */
                    init_space=1;
                } /* end if */
                break;

            case H5D_CHUNKED:
                if(layout->u.chunk.addr==HADDR_UNDEF) {
                    /* Create the root of the B-tree that describes chunked storage */
                    if (H5D_istore_create (f, dxpl_id, layout/*out*/)<0)
                        HGOTO_ERROR (H5E_IO, H5E_CANTINIT, FAIL, "unable to initialize chunked storage")

                    /* Indicate that we set the storage addr */
                    addr_set=1;

                    /* Indicate that we should initialize storage space */
                    init_space=1;
                } /* end if */

                /* If space allocation is set to 'early' and we are extending
                 *  the dataset, indicate that space should be allocated, so the
                 *  B-tree gets expanded. -QAK
                 */
                if(dset->shared->alloc_time==H5D_ALLOC_TIME_EARLY && time_alloc==H5D_ALLOC_EXTEND)
                    init_space=1;

                break;

            case H5D_COMPACT:
                /* Check if space is already allocated */
                if(layout->u.compact.buf==NULL) {
                    /* Reserve space in layout header message for the entire array. */
                    assert(layout->u.compact.size>0);
                    if (NULL==(layout->u.compact.buf=H5MM_malloc(layout->u.compact.size)))
                        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory for compact dataset")
                    layout->u.compact.dirty = TRUE;

                    /* Indicate that we set the storage addr */
                    addr_set=1;

                    /* Indicate that we should initialize storage space */
                    init_space=1;
                } /* end if */
                break;

            default:
                assert ("not implemented yet" && 0);
#ifdef NDEBUG
                HGOTO_ERROR (H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
        } /* end switch */ /*lint !e788 All appropriate cases are covered */

        /* Check if we need to initialize the space */
        if(init_space) {
            if (layout->type==H5D_CHUNKED) {
                /* If we are doing incremental allocation and the B-tree got
                 * created during a H5Dwrite call, don't initialize the storage
                 * now, wait for the actual writes to each block and let the
                 * low-level chunking routines handle initialize the fill-values.
                 * Otherwise, pass along the space initialization call and let
                 * the low-level chunking routines sort out whether to write
                 * fill values to the chunks they allocate space for.  Yes,
                 * this is icky. -QAK
                 */
                if(!(dset->shared->alloc_time==H5D_ALLOC_TIME_INCR && time_alloc==H5D_ALLOC_WRITE)) {
                    if(H5D_init_storage(dset, full_overwrite, dxpl_id) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize dataset with fill value")
                } /* end if */
            } /* end if */
            else {
                H5D_fill_value_t  fill_status;    /* The fill value status */

                /* Check the dataset's fill-value status */
                if (H5P_is_fill_value_defined(&(dset->shared->fill), &fill_status) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

                /* If we are filling the dataset on allocation or "if set" and
                 * the fill value _is_ set, do that now */
                if(dset->shared->fill_time==H5D_FILL_TIME_ALLOC ||
                        (dset->shared->fill_time==H5D_FILL_TIME_IFSET && fill_status==H5D_FILL_VALUE_USER_DEFINED)) {
                    if(H5D_init_storage(dset, full_overwrite, dxpl_id) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize dataset with fill value")
                } /* end if */
            } /* end else */
        } /* end if */

        /* Also update header message for layout with new address, if we
         * set the address.  (this is improves forward compatibility).
         */
        if(time_alloc!=H5D_ALLOC_CREATE && addr_set)
            if (H5O_modify (&(dset->ent), H5O_LAYOUT_ID, 0, H5O_FLAG_CONSTANT, update_time, &(dset->shared->layout), dxpl_id) < 0)
                HGOTO_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update layout message")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_alloc_storage() */


/*-------------------------------------------------------------------------
 * Function:  H5D_init_storage
 *
 * Purpose:  Initialize the data for a new dataset.  If a selection is
 *    defined for SPACE then initialize only that part of the
 *    dataset.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, October  5, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to retrieve property for generic property
 *              list.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_init_storage(H5D_t *dset, hbool_t full_overwrite, hid_t dxpl_id)
{
    herr_t    ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_init_storage)

    assert(dset);

    switch (dset->shared->layout.type) {
        case H5D_COMPACT:
            {
                hssize_t            snpoints;       /* Number of points in space (for error checking) */
                size_t              npoints;        /* Number of points in space */

                /* Get the number of elements in the dataset's dataspace */
                snpoints = H5S_GET_EXTENT_NPOINTS(dset->shared->space);
                assert(snpoints>=0);
                H5_ASSIGN_OVERFLOW(npoints,snpoints,hssize_t,size_t);

                /* If we will be immediately overwriting the values, don't bother to clear them */
                if(!full_overwrite) {
                    /* If the fill value is defined, initialize the data buffer with it */
                    if(dset->shared->fill.buf)
                        /* Initialize the cached data buffer with the fill value */
                        H5V_array_fill(dset->shared->layout.u.compact.buf, dset->shared->fill.buf, dset->shared->fill.size, npoints);
                    else /* If the fill value is default, zero set data buf. */
                        HDmemset(dset->shared->layout.u.compact.buf, 0, dset->shared->layout.u.compact.size);
                } /* end if */
            }
            break;

        case H5D_CONTIGUOUS:
            /* Don't write default fill values to external files */
            /* If we will be immediately overwriting the values, don't bother to clear them */
            if((dset->shared->efl.nused==0 || dset->shared->fill.buf) && !full_overwrite) {
                if (H5D_contig_fill(dset, dxpl_id)<0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to allocate all chunks of dataset")
            } /* end if */
            break;

        case H5D_CHUNKED:
            /*
             * Allocate file space
             * for all chunks now and initialize each chunk with the fill value.
             */
            if (H5D_istore_allocate(dset, dxpl_id, full_overwrite)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to allocate all chunks of dataset")
            break;

        default:
            assert ("not implemented yet" && 0);
#ifdef NDEBUG
            HGOTO_ERROR (H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
    } /* end switch */ /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_init_storage() */


/*-------------------------------------------------------------------------
 * Function:  H5Dget_storage_size
 *
 * Purpose:  Returns the amount of storage that is required for the
 *    dataset. For chunked datasets this is the number of allocated
 *    chunks times the chunk size.
 *
 * Return:  Success:  The amount of storage space allocated for the
 *        dataset, not counting meta data. The return
 *        value may be zero if no data has been stored.
 *
 *    Failure:  Zero
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5Dget_storage_size(hid_t dset_id)
{
    H5D_t  *dset=NULL;
    hsize_t  ret_value;      /* Return value */

    FUNC_ENTER_API(H5Dget_storage_size, 0)
    H5TRACE1("h","i",dset_id);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a dataset")

    /* Set return value */
    ret_value = H5D_get_storage_size(dset,H5AC_ind_dxpl_id);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_get_storage_size
 *
 * Purpose:  Determines how much space has been reserved to store the raw
 *    data of a dataset.
 *
 * Return:  Success:  Number of bytes reserved to hold raw data.
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5D_get_storage_size(H5D_t *dset, hid_t dxpl_id)
{
    hsize_t  ret_value;

    FUNC_ENTER_NOAPI(H5D_get_storage_size, 0)

    switch(dset->shared->layout.type) {
        case H5D_CHUNKED:
            if(dset->shared->layout.u.chunk.addr == HADDR_UNDEF)
                ret_value=0;
            else
                ret_value = H5D_istore_allocated(dset, dxpl_id);
            break;

        case H5D_CONTIGUOUS:
            /* Datasets which are not allocated yet are using no space on disk */
            if(dset->shared->layout.u.contig.addr == HADDR_UNDEF)
                ret_value=0;
            else
                 ret_value=dset->shared->layout.u.contig.size;
            break;

        case H5D_COMPACT:
            ret_value = dset->shared->layout.u.compact.size;
            break;

        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a dataset type")
    } /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Dget_offset
 *
 * Purpose:  Returns the address of dataset in file.
 *
 * Return:  Success:        the address of dataset
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Raymond Lu
 *              November 6, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5Dget_offset(hid_t dset_id)
{
    H5D_t  *dset=NULL;
    haddr_t  ret_value;      /* Return value */

    FUNC_ENTER_API(H5Dget_offset, HADDR_UNDEF)
    H5TRACE1("a","i",dset_id);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "not a dataset")

    /* Set return value */
    ret_value = H5D_get_offset(dset);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_get_offset
 *
 * Purpose:  Private function for H5D_get_offset.  Returns the address
 *              of dataset in file.
 *
 * Return:  Success:        the address of dataset
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Raymond Lu
 *              November 6, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5D_get_offset(const H5D_t *dset)
{
    haddr_t  ret_value=HADDR_UNDEF;
    haddr_t     base_addr;
    H5F_t       *f;

    FUNC_ENTER_NOAPI(H5D_get_offset, HADDR_UNDEF)

    assert(dset);

    switch(dset->shared->layout.type) {
        case H5D_CHUNKED:
        case H5D_COMPACT:
            break;

        case H5D_CONTIGUOUS:
            /* If dataspace hasn't been allocated or dataset is stored in
             * an external file, the value will be HADDR_UNDEF. */
            f =  H5D_get_file(dset);
            base_addr = H5F_get_base_addr(f);

            /* If there's user block in file, returns the absolute dataset offset
             * from the beginning of file. */
            if(base_addr!=HADDR_UNDEF)
                ret_value = dset->shared->layout.u.contig.addr + base_addr;
            else
                ret_value = dset->shared->layout.u.contig.addr;
            break;

        default:
            assert ("not implemented yet" && 0);
#ifdef NDEBUG
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "unknown dataset layout type")
#endif /* NDEBUG */
    } /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Diterate
 *
 * Purpose:  This routine iterates over all the elements selected in a memory
 *      buffer.  The callback function is called once for each element selected
 *      in the dataspace.  The selection in the dataspace is modified so
 *      that any elements already iterated over are removed from the selection
 *      if the iteration is interrupted (by the H5D_operator_t function
 *      returning non-zero) in the "middle" of the iteration and may be
 *      re-started by the user where it left off.
 *
 *      NOTE: Until "subtracting" elements from a selection is implemented,
 *          the selection is not modified.
 *
 * Parameters:
 *      void *buf;          IN/OUT: Pointer to the buffer in memory containing
 *                              the elements to iterate over.
 *      hid_t type_id;      IN: Datatype ID for the elements stored in BUF.
 *      hid_t space_id;     IN: Dataspace ID for BUF, also contains the
 *                              selection to iterate over.
 *      H5D_operator_t op; IN: Function pointer to the routine to be
 *                              called for each element in BUF iterated over.
 *      void *operator_data;    IN/OUT: Pointer to any user-defined data
 *                              associated with the operation.
 *
 * Operation information:
 *      H5D_operator_t is defined as:
 *          typedef herr_t (*H5D_operator_t)(void *elem, hid_t type_id,
 *              unsigned ndim, const hsize_t *point, void *operator_data);
 *
 *      H5D_operator_t parameters:
 *          void *elem;         IN/OUT: Pointer to the element in memory containing
 *                                  the current point.
 *          hid_t type_id;      IN: Datatype ID for the elements stored in ELEM.
 *          unsigned ndim;       IN: Number of dimensions for POINT array
 *          const hsize_t *point; IN: Array containing the location of the element
 *                                  within the original dataspace.
 *          void *operator_data;    IN/OUT: Pointer to any user-defined data
 *                                  associated with the operation.
 *
 *      The return values from an operator are:
 *          Zero causes the iterator to continue, returning zero when all
 *              elements have been processed.
 *          Positive causes the iterator to immediately return that positive
 *              value, indicating short-circuit success.  The iterator can be
 *              restarted at the next element.
 *          Negative causes the iterator to immediately return that value,
 *              indicating failure. The iterator can be restarted at the next
 *              element.
 *
 * Return:  Returns the return value of the last operator if it was non-zero,
 *          or zero if all elements were processed. Otherwise returns a
 *          negative value.
 *
 * Programmer:  Quincey Koziol
 *              Friday, June 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Diterate(void *buf, hid_t type_id, hid_t space_id, H5D_operator_t op,
        void *operator_data)
{
    H5S_t       *space = NULL;
    herr_t ret_value;

    FUNC_ENTER_API(H5Diterate, FAIL)
    H5TRACE5("e","xiixx",buf,type_id,space_id,op,operator_data);

    /* Check args */
    if (NULL==op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid operator")
    if (buf==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer")
    if (H5I_DATATYPE != H5I_get_type(type_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid datatype")
    if (NULL == (space = H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if( !(H5S_has_extent(space)) )
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    ret_value=H5S_select_iterate(buf,type_id,space,op,operator_data);

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Diterate() */


/*-------------------------------------------------------------------------
 * Function:  H5Dvlen_reclaim
 *
 * Purpose:  Frees the buffers allocated for storing variable-length data
 *      in memory.  Only frees the VL data in the selection defined in the
 *      dataspace.  The dataset transfer property list is required to find the
 *      correct allocation/free methods for the VL data in the buffer.
 *
 * Return:  Non-negative on success, negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, June 10, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dvlen_reclaim(hid_t type_id, hid_t space_id, hid_t plist_id, void *buf)
{
    H5T_vlen_alloc_info_t _vl_alloc_info;       /* VL allocation info buffer */
    H5T_vlen_alloc_info_t *vl_alloc_info=&_vl_alloc_info;   /* VL allocation info */
    herr_t ret_value;

    FUNC_ENTER_API(H5Dvlen_reclaim, FAIL)
    H5TRACE4("e","iiix",type_id,space_id,plist_id,buf);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type_id) || H5I_DATASPACE!=H5I_get_type(space_id) ||
            buf==NULL)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == plist_id)
        plist_id= H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE!=H5P_isa_class(plist_id,H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Get the allocation info */
    if(H5T_vlen_get_alloc_info(plist_id,&vl_alloc_info)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "unable to retrieve VL allocation info");

    /* Call H5Diterate with args, etc. */
    ret_value=H5Diterate(buf,type_id,space_id,H5T_vlen_reclaim,vl_alloc_info);

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Dvlen_reclaim() */


/*-------------------------------------------------------------------------
 * Function:  H5D_vlen_get_buf_size_alloc
 *
 * Purpose:  This routine makes certain there is enough space in the temporary
 *      buffer for the new data to read in.  All the VL data read in is actually
 *      placed in this buffer, overwriting the previous data.  Needless to say,
 *      this data is not actually usable.
 *
 * Return:  Non-negative on success, negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, August 17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5D_vlen_get_buf_size_alloc(size_t size, void *info)
{
    H5D_vlen_bufsize_t *vlen_bufsize=(H5D_vlen_bufsize_t *)info;
    void *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(H5D_vlen_get_buf_size_alloc, NULL)

    /* Get a temporary pointer to space for the VL data */
    if ((vlen_bufsize->vl_tbuf=H5FL_BLK_REALLOC(vlen_vl_buf,vlen_bufsize->vl_tbuf,size))!=NULL)
        vlen_bufsize->size+=size;

    /* Set return value */
    ret_value=vlen_bufsize->vl_tbuf;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_vlen_get_buf_size_alloc() */


/*-------------------------------------------------------------------------
 * Function:  H5D_vlen_get_buf_size
 *
 * Purpose:  This routine checks the number of bytes required to store a single
 *      element from a dataset in memory, creating a selection with just the
 *      single element selected to read in the element and using a custom memory
 *      allocator for any VL data encountered.
 *          The *size value is modified according to how many bytes are
 *      required to store the element in memory.
 *
 * Implementation: This routine actually performs the read with a custom
 *      memory manager which basically just counts the bytes requested and
 *      uses a temporary memory buffer (through the H5FL API) to make certain
 *      enough space is available to perform the read.  Then the temporary
 *      buffer is released and the number of bytes allocated is returned.
 *      Kinda kludgy, but easier than the other method of trying to figure out
 *      the sizes without actually reading the data in... - QAK
 *
 * Return:  Non-negative on success, negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, August 17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5D_vlen_get_buf_size(void UNUSED *elem, hid_t type_id, unsigned UNUSED ndim, const hsize_t *point, void *op_data)
{
    H5D_vlen_bufsize_t *vlen_bufsize=(H5D_vlen_bufsize_t *)op_data;
    H5T_t  *dt = NULL;
    herr_t ret_value=0;         /* The correct return value, if this function succeeds */

    FUNC_ENTER_NOAPI(H5D_vlen_get_buf_size, FAIL)

    assert(op_data);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    /* Check args */
    if (NULL==(dt=H5I_object(type_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")

    /* Make certain there is enough fixed-length buffer available */
    if ((vlen_bufsize->fl_tbuf=H5FL_BLK_REALLOC(vlen_fl_buf,vlen_bufsize->fl_tbuf,H5T_get_size(dt)))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't resize tbuf")

    /* Select point to read in */
    if (H5Sselect_elements(vlen_bufsize->fspace_id,H5S_SELECT_SET,1,(const hsize_t **)point)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't select point")

    /* Read in the point (with the custom VL memory allocator) */
    if(H5Dread(vlen_bufsize->dataset_id,type_id,vlen_bufsize->mspace_id,vlen_bufsize->fspace_id,vlen_bufsize->xfer_pid,vlen_bufsize->fl_tbuf)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read point")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D_vlen_get_buf_size() */


/*-------------------------------------------------------------------------
 * Function:  H5Dvlen_get_buf_size
 *
 * Purpose:  This routine checks the number of bytes required to store the VL
 *      data from the dataset, using the space_id for the selection in the
 *      dataset on disk and the type_id for the memory representation of the
 *      VL data, in memory.  The *size value is modified according to how many
 *      bytes are required to store the VL data in memory.
 *
 * Implementation: This routine actually performs the read with a custom
 *      memory manager which basically just counts the bytes requested and
 *      uses a temporary memory buffer (through the H5FL API) to make certain
 *      enough space is available to perform the read.  Then the temporary
 *      buffer is released and the number of bytes allocated is returned.
 *      Kinda kludgy, but easier than the other method of trying to figure out
 *      the sizes without actually reading the data in... - QAK
 *
 * Return:  Non-negative on success, negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, August 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dvlen_get_buf_size(hid_t dataset_id, hid_t type_id, hid_t space_id,
        hsize_t *size)
{
    H5D_vlen_bufsize_t vlen_bufsize = {0, 0, 0, 0, 0, 0, 0};
    char bogus;         /* bogus value to pass to H5Diterate() */
    H5P_genclass_t  *pclass;    /* Property class */
    H5P_genplist_t  *plist;     /* Property list */
    herr_t ret_value=FAIL;

    FUNC_ENTER_API(H5Dvlen_get_buf_size, FAIL)
    H5TRACE4("e","iii*h",dataset_id,type_id,space_id,size);

    /* Check args */
    if (H5I_DATASET!=H5I_get_type(dataset_id) ||
            H5I_DATATYPE!=H5I_get_type(type_id) ||
            H5I_DATASPACE!=H5I_get_type(space_id) || size==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")

    /* Save the dataset ID */
    vlen_bufsize.dataset_id=dataset_id;

    /* Get a copy of the dataspace ID */
    if((vlen_bufsize.fspace_id=H5Dget_space(dataset_id))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy dataspace")

    /* Create a scalar for the memory dataspace */
    if((vlen_bufsize.mspace_id=H5Screate(H5S_SCALAR))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't create dataspace")

    /* Grab the temporary buffers required */
    if((vlen_bufsize.fl_tbuf=H5FL_BLK_MALLOC(vlen_fl_buf,1))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "no temporary buffers available")
    if((vlen_bufsize.vl_tbuf=H5FL_BLK_MALLOC(vlen_vl_buf,1))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "no temporary buffers available")

    /* Get the pointer to the dataset transfer class */
    if (NULL == (pclass = H5I_object(H5P_CLS_DATASET_XFER_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class")

    /* Change to the custom memory allocation routines for reading VL data */
    if((vlen_bufsize.xfer_pid=H5P_create_id(pclass))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "no dataset xfer plists available")

    /* Get the property list struct */
    if (NULL == (plist = H5I_object(vlen_bufsize.xfer_pid)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

    /* Set the memory manager to the special allocation routine */
    if(H5P_set_vlen_mem_manager(plist,H5D_vlen_get_buf_size_alloc,&vlen_bufsize,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set VL data allocation routine")

    /* Set the initial number of bytes required */
    vlen_bufsize.size=0;

    /* Call H5Diterate with args, etc. */
    ret_value=H5Diterate(&bogus,type_id,space_id,H5D_vlen_get_buf_size,&vlen_bufsize);

    /* Get the size if we succeeded */
    if(ret_value>=0)
        *size=vlen_bufsize.size;

done:
    if(vlen_bufsize.fspace_id>0) {
        if(H5I_dec_ref(vlen_bufsize.fspace_id)<0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    } /* end if */
    if(vlen_bufsize.mspace_id>0) {
        if(H5I_dec_ref(vlen_bufsize.mspace_id)<0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    } /* end if */
    if(vlen_bufsize.fl_tbuf!=NULL)
        H5FL_BLK_FREE(vlen_fl_buf,vlen_bufsize.fl_tbuf);
    if(vlen_bufsize.vl_tbuf!=NULL)
        H5FL_BLK_FREE(vlen_vl_buf,vlen_bufsize.vl_tbuf);
    if(vlen_bufsize.xfer_pid>0) {
        if(H5I_dec_ref(vlen_bufsize.xfer_pid)<0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to decrement ref count on property list")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}   /* end H5Dvlen_get_buf_size() */


/*-------------------------------------------------------------------------
 * Function: H5Dset_extent
 *
 * Purpose: Modifies the dimensions of a dataset, based on H5Dextend.
 *  Can change to a lower dimension.
 *
 * Return: Success: SUCCEED, Failure: FAIL
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *             Robb Matzke
 *
 * Date: April 9, 2002
 *
 * Comments: Public function, calls private H5D_set_extent
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dset_extent(hid_t dset_id, const hsize_t *size)
{
    H5D_t                  *dset = NULL;
    herr_t                  ret_value=SUCCEED;  /* Return value */

    FUNC_ENTER_API(H5Dset_extent, FAIL)
    H5TRACE2("e","i*h",dset_id,size);

    /* Check args */
    if(NULL == (dset = H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(!size)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no size specified")

    /* Private function */
    if(H5D_set_extent(dset, size, H5AC_dxpl_id) < 0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set extend dataset")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function: H5D_set_extent
 *
 * Purpose: Based in H5D_extend, allows change to a lower dimension,
 *  calls H5S_set_extent and H5D_istore_prune_by_extent instead
 *
 * Return: Success: SUCCEED, Failure: FAIL
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *             Robb Matzke
 *
 * Date: April 9, 2002
 *
 * Comments: Private function
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_set_extent(H5D_t *dset, const hsize_t *size, hid_t dxpl_id)
{
    hsize_t                 curr_dims[H5O_LAYOUT_NDIMS];  /* Current dimension sizes */
    int                     rank;  /* Dataspace # of dimensions */
    herr_t                  ret_value = SUCCEED;        /* Return value */
    H5S_t                  *space = NULL;
    int                     u;
    unsigned                shrink = 0;         /* Flag to indicate a dimension has shrank */
    unsigned                expand = 0;         /* Flag to indicate a dimension has grown */
    int                     changed = 0;

    FUNC_ENTER_NOAPI(H5D_set_extent, FAIL)

    /* Check args */
    assert(dset);
    assert(size);

 /*-------------------------------------------------------------------------
  * Get the data space
  *-------------------------------------------------------------------------
  */
    space = dset->shared->space;

 /*-------------------------------------------------------------------------
  * Check if we are shrinking or expanding any of the dimensions
  *-------------------------------------------------------------------------
  */
    if((rank = H5S_get_simple_extent_dims(space, curr_dims, NULL)) < 0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataset dimensions")

    for(u = 0; u < rank; u++) {
  if(size[u] < curr_dims[u])
      shrink = 1;
  if(size[u] > curr_dims[u])
      expand = 1;
    }

 /*-------------------------------------------------------------------------
  * Modify the size of the data space
  *-------------------------------------------------------------------------
  */
    if((changed=H5S_set_extent(space, size)) < 0)
  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to modify size of data space")

    /* Don't bother updating things, unless they've changed */
    if(changed) {
     /*-------------------------------------------------------------------------
      * Modify the dataset storage
      *-------------------------------------------------------------------------
      */
        /* Save the new dataspace in the file if necessary */
        if(H5S_modify(&(dset->ent), space, TRUE, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update file with new dataspace")

        /* Update the index values for the cached chunks for this dataset */
        if(H5D_CHUNKED == dset->shared->layout.type)
            if(H5D_istore_update_cache(dset, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update cached chunk indices")

  /* Allocate space for the new parts of the dataset, if appropriate */
        if(expand && dset->shared->alloc_time==H5D_ALLOC_TIME_EARLY)
            if(H5D_alloc_storage(dset->ent.file, dxpl_id, dset, H5D_ALLOC_EXTEND, TRUE, FALSE) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize dataset storage")


     /*-------------------------------------------------------------------------
      * Remove chunk information in the case of chunked datasets
      * This removal takes place only in case we are shrinking the dateset
      *-------------------------------------------------------------------------
      */
        if(shrink && H5D_CHUNKED == dset->shared->layout.type) {
            H5D_io_info_t io_info;              /* Dataset I/O info */
            H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
            H5D_dxpl_cache_t *dxpl_cache=&_dxpl_cache;   /* Data transfer property cache */

            /* Fill the DXPL cache values for later use */
            if (H5D_get_dxpl_cache(dxpl_id,&dxpl_cache)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

            /* Construct dataset I/O info */
            H5D_BUILD_IO_INFO(&io_info,dset,dxpl_cache,dxpl_id,NULL);

            /* Remove excess chunks */
            if(H5D_istore_prune_by_extent(&io_info) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to remove chunks ")

            /* Reset the elements outsize the new dimensions, but in existing chunks */
            if(H5D_istore_initialize_by_extent(&io_info) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to initialize chunks ")
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5D_flush
 *
 * Purpose:     Flush any dataset information cached in memory
 *
 * Return:  Success:  Non-negative
 *    Failure:  Negative
 *
 *
 * Programmer:  Ray Lu
 *
 * Date:        August 14, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_flush(const H5F_t *f, hid_t dxpl_id, unsigned flags)
{
    unsigned    num_dsets;      /* Number of datasets in file   */
    hid_t       *id_list=NULL;  /* list of dataset IDs          */
    H5D_t       *dataset=NULL;  /* Dataset pointer              */
    unsigned  u;              /* Index variable */
    herr_t      ret_value = SUCCEED;        /* Return value     */

    FUNC_ENTER_NOAPI(H5D_flush, FAIL)

    /* Check args */
    assert(f);

    /* Update layout message for compact dataset */
    num_dsets=H5F_get_obj_count(f, H5F_OBJ_DATASET);

    /* Check for something to do */
    if(num_dsets>0) {
        H5_CHECK_OVERFLOW(num_dsets,unsigned,size_t);
        if(NULL==(id_list=H5MM_malloc((size_t)num_dsets*sizeof(hid_t))))
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to allocate memory for ID list")
        if(H5F_get_obj_ids(f, H5F_OBJ_DATASET, -1, id_list) != num_dsets)
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to get dataset ID list")
        for(u = 0; u < num_dsets; u++) {
            if(NULL==(dataset=H5I_object_verify(id_list[u], H5I_DATASET)))
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to get dataset object")

            /* Flush the raw data buffer, if we have a dirty one */
            if (dataset->shared->cache.contig.sieve_buf && dataset->shared->cache.contig.sieve_dirty) {
                assert(dataset->shared->layout.type!=H5D_COMPACT);      /* We should never have a sieve buffer for compact storage */

                /* Write dirty data sieve buffer to file */
                if (H5F_block_write(f, H5FD_MEM_DRAW, dataset->shared->cache.contig.sieve_loc,
                        dataset->shared->cache.contig.sieve_size, dxpl_id, dataset->shared->cache.contig.sieve_buf) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "block write failed")

                /* Reset sieve buffer dirty flag */
                dataset->shared->cache.contig.sieve_dirty=0;
            } /* end if */

            /* Flush cached information for each kind of dataset */
            switch(dataset->shared->layout.type) {
                case H5D_CONTIGUOUS:
                    break;

                case H5D_CHUNKED:
                    /* Flush the raw data cache */
                    if (H5D_istore_flush(dataset, dxpl_id, flags & (H5F_FLUSH_INVALIDATE | H5F_FLUSH_CLEAR_ONLY)) < 0)
                        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to flush raw data cache")
                    break;

                case H5D_COMPACT:
                    if(dataset->shared->layout.u.compact.dirty) {
                        if(H5O_modify(&(dataset->ent), H5O_LAYOUT_ID, 0, 0, H5O_UPDATE_TIME, &(dataset->shared->layout), dxpl_id)<0)
                            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to update layout message")
                        dataset->shared->layout.u.compact.dirty = FALSE;
                    } /* end if */
                    break;

                default:
                    assert ("not implemented yet" && 0);
#ifdef NDEBUG
                    HGOTO_ERROR (H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
            } /* end switch */ /*lint !e788 All appropriate cases are covered */

        }
    } /* end if */

done:
    if(id_list!=NULL)
        H5MM_xfree(id_list);
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_flush() */


/*-------------------------------------------------------------------------
 * Function:  H5Ddebug
 *
 * Purpose:  Prints various information about a dataset.  This function is
 *    not to be documented in the API at this time.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 28, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ddebug(hid_t dset_id)
{
    H5D_t  *dset=NULL;
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_API(H5Ddebug, FAIL)
    H5TRACE1("e","i",dset_id);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(dset_id, H5I_DATASET)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Print B-tree information */
    if (H5D_CHUNKED==dset->shared->layout.type) {
  (void)H5D_istore_dump_btree(dset->ent.file, H5AC_dxpl_id, stdout, dset->shared->layout.u.chunk.ndims, dset->shared->layout.u.chunk.addr);
    } else if (H5D_CONTIGUOUS==dset->shared->layout.type) {
  HDfprintf(stdout, "    %-10s %a\n", "Address:", dset->shared->layout.u.contig.addr);
    }

done:
    FUNC_LEAVE_API(ret_value)
}
