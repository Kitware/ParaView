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

#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */

/* Private header files */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Ppkg.h"		/* Property lists		  	*/

/* Pablo mask */
#define PABLO_MASK	H5Pdxpl_mask

/* Interface initialization */
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

/* Local datatypes */

/* Static function prototypes */

#ifdef H5_WANT_H5_V1_4_COMPAT

/*-------------------------------------------------------------------------
 * Function:	H5Pset_buffer
 *
 * Purpose:	Given a dataset transfer property list, set the maximum size
 *		for the type conversion buffer and background buffer and
 *		optionally supply pointers to application-allocated buffers.
 *		If the buffer size is smaller than the entire amount of data
 *		being transfered between application and file, and a type
 *		conversion buffer or background buffer is required then
 *		strip mining will be used.
 *
 *		If TCONV and/or BKG are null pointers then buffers will be
 *		allocated and freed during the data transfer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, March 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_buffer(hid_t plist_id, hsize_t _size, void *tconv, void *bkg)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t size=(size_t)_size;  /* Work around size difference */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_buffer, FAIL);
    H5TRACE4("e","ihxx",plist_id,_size,tconv,bkg);

    /* Check arguments */
    if (size<=0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "buffer size must not be zero");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_MAX_TEMP_BUF_NAME, &size)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "Can't set transfer buffer size");
    if(H5P_set(plist, H5D_XFER_TCONV_BUF_NAME, &tconv)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "Can't set transfer type conversion buffer");
    if(H5P_set(plist, H5D_XFER_BKGR_BUF_NAME, &bkg)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "Can't set background type conversion buffer");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_buffer
 *
 * Purpose:	Reads values previously set with H5Pset_buffer().
 *
 * Return:	Success:	Buffer size.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *              Monday, March 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5Pget_buffer(hid_t plist_id, void **tconv/*out*/, void **bkg/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t size;                /* Type conversion buffer size */
    hsize_t ret_value;          /* Return value */

    FUNC_ENTER_API(H5Pget_buffer, 0);
    H5TRACE3("h","ixx",plist_id,tconv,bkg);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, 0, "can't find object for ID");

    /* Return values */
    if (tconv)
        if(H5P_get(plist, H5D_XFER_TCONV_BUF_NAME, tconv)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, 0, "Can't get transfer type conversion buffer");
    if (bkg)
        if(H5P_get(plist, H5D_XFER_BKGR_BUF_NAME, bkg)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, 0, "Can't get background type conversion buffer");

    /* Get the size */
    if(H5P_get(plist, H5D_XFER_MAX_TEMP_BUF_NAME, &size)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, 0, "Can't set transfer buffer size");

    /* Set the return value */
    ret_value=(hsize_t)size;

done:
    FUNC_LEAVE_API(ret_value);
}

#else /* H5_WANT_H5_V1_4_COMPAT */

/*-------------------------------------------------------------------------
 * Function:	H5Pset_buffer
 *
 * Purpose:	Given a dataset transfer property list, set the maximum size
 *		for the type conversion buffer and background buffer and
 *		optionally supply pointers to application-allocated buffers.
 *		If the buffer size is smaller than the entire amount of data
 *		being transfered between application and file, and a type
 *		conversion buffer or background buffer is required then
 *		strip mining will be used.
 *
 *		If TCONV and/or BKG are null pointers then buffers will be
 *		allocated and freed during the data transfer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, March 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_buffer(hid_t plist_id, size_t size, void *tconv, void *bkg)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_buffer, FAIL);
    H5TRACE4("e","izxx",plist_id,size,tconv,bkg);

    /* Check arguments */
    if (size<=0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "buffer size must not be zero");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_MAX_TEMP_BUF_NAME, &size)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "Can't set transfer buffer size");
    if(H5P_set(plist, H5D_XFER_TCONV_BUF_NAME, &tconv)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "Can't set transfer type conversion buffer");
    if(H5P_set(plist, H5D_XFER_BKGR_BUF_NAME, &bkg)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, FAIL, "Can't set background type conversion buffer");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_buffer
 *
 * Purpose:	Reads values previously set with H5Pset_buffer().
 *
 * Return:	Success:	Buffer size.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *              Monday, March 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Pget_buffer(hid_t plist_id, void **tconv/*out*/, void **bkg/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t size;                /* Type conversion buffer size */
    size_t ret_value;           /* Return value */

    FUNC_ENTER_API(H5Pget_buffer, 0);
    H5TRACE3("z","ixx",plist_id,tconv,bkg);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, 0, "can't find object for ID");

    /* Return values */
    if (tconv)
        if(H5P_get(plist, H5D_XFER_TCONV_BUF_NAME, tconv)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, 0, "Can't get transfer type conversion buffer");
    if (bkg)
        if(H5P_get(plist, H5D_XFER_BKGR_BUF_NAME, bkg)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTGET, 0, "Can't get background type conversion buffer");

    /* Get the size */
    if(H5P_get(plist, H5D_XFER_MAX_TEMP_BUF_NAME, &size)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTSET, 0, "Can't set transfer buffer size");

    /* Set the return value */
    ret_value=size;

done:
    FUNC_LEAVE_API(ret_value);
}

#endif /* H5_WANT_H5_V1_4_COMPAT */

#ifdef H5_WANT_H5_V1_4_COMPAT

/*-------------------------------------------------------------------------
 * Function:	H5Pset_hyper_cache
 *
 * Purpose:	Given a dataset transfer property list, indicate whether to
 *		cache the hyperslab blocks during the I/O (which speeds
 *		things up) and the maximum size of the hyperslab block to
 *		cache.  If a block is smaller than to limit, it may still not
 *		be cached if no memory is available. Setting the limit to 0
 *		indicates no limitation on the size of block to attempt to
 *		cache.
 *
 *		The default is to cache blocks with no limit on block size
 *		for serial I/O and to not cache blocks for parallel I/O
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_hyper_cache(hid_t plist_id, unsigned cache, unsigned limit)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_API(H5Pset_hyper_cache, FAIL);
    H5TRACE3("e","iIuIu",plist_id,cache,limit);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    cache = (cache>0) ? 1 : 0;
    if (H5P_set(plist,H5D_XFER_HYPER_CACHE_NAME,&cache)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to set value");
    if (H5P_set(plist,H5D_XFER_HYPER_CACHE_LIM_NAME,&limit)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to set value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_hyper_cache
 *
 * Purpose:	Reads values previously set with H5Pset_hyper_cache().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_hyper_cache(hid_t plist_id, unsigned *cache/*out*/,
		   unsigned *limit/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_hyper_cache, FAIL);
    H5TRACE3("e","ixx",plist_id,cache,limit);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Return values */
    if (cache)
        if (H5P_get(plist,H5D_XFER_HYPER_CACHE_NAME,cache)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");
    if (limit)
        if (H5P_get(plist,H5D_XFER_HYPER_CACHE_LIM_NAME,limit)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");

done:
    FUNC_LEAVE_API(ret_value);
}
#endif /* H5_WANT_H5_V1_4_COMPAT */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_preserve
 *
 * Purpose:	When reading or writing compound data types and the
 *		destination is partially initialized and the read/write is
 *		intended to initialize the other members, one must set this
 *		property to TRUE.  Otherwise the I/O pipeline treats the
 *		destination datapoints as completely uninitialized.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_preserve(hid_t plist_id, hbool_t status)
{
    H5T_bkg_t need_bkg;         /* Value for background buffer type */
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */
    
    FUNC_ENTER_API(H5Pset_preserve, FAIL);
    H5TRACE2("e","ib",plist_id,status);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    need_bkg = status ? H5T_BKG_YES : H5T_BKG_NO;
    if (H5P_set(plist,H5D_XFER_BKGR_BUF_TYPE_NAME,&need_bkg)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_preserve
 *
 * Purpose:	The inverse of H5Pset_preserve()
 *
 * Return:	Success:	TRUE or FALSE
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_preserve(hid_t plist_id)
{
    H5T_bkg_t need_bkg;         /* Background value */
    H5P_genplist_t *plist;      /* Property list pointer */
    int ret_value;              /* return value */
    
    FUNC_ENTER_API(H5Pget_preserve, FAIL);
    H5TRACE1("Is","i",plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if (H5P_get(plist,H5D_XFER_BKGR_BUF_NAME,&need_bkg)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");

    /* Set return value */
    ret_value= need_bkg ? TRUE : FALSE;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_edc_check
 *
 * Purpose:     Enable or disable error-detecting for a dataset reading 
 *              process.  This error-detecting algorithm is whichever 
 *              user chooses earlier.  This function cannot control 
 *              writing process.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Jan 3, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_edc_check(hid_t plist_id, H5Z_EDC_t check)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */
    
    FUNC_ENTER_API(H5Pset_edc_check, FAIL);
    H5TRACE2("e","iZe",plist_id,check);

    /* Check argument */
    if (check != H5Z_ENABLE_EDC && check != H5Z_DISABLE_EDC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a valid value");
        
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    if (H5P_set(plist,H5D_XFER_EDC_NAME,&check)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");
 
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_edc_check
 *
 * Purpose:     Enable or disable error-detecting for a dataset reading 
 *              process.  This error-detecting algorithm is whichever 
 *              user chooses earlier.  This function cannot control 
 *              writing process.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Jan 3, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5Z_EDC_t
H5Pget_edc_check(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5Z_EDC_t      ret_value;   /* return value */
    
    FUNC_ENTER_API(H5Pget_edc_check, H5Z_ERROR_EDC);
    H5TRACE1("Ze","i",plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5Z_ERROR_EDC, "can't find object for ID");

    /* Update property list */
    if (H5P_get(plist,H5D_XFER_EDC_NAME,&ret_value)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, H5Z_ERROR_EDC, "unable to set value");

    /* check valid value */
    if (ret_value != H5Z_ENABLE_EDC && ret_value != H5Z_DISABLE_EDC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5Z_ERROR_EDC, "not a valid value");

done:
    FUNC_LEAVE_API(ret_value);
}
 

/*-------------------------------------------------------------------------
 * Function:	H5Pset_filter_callback
 *
 * Purpose:     Sets user's callback function for dataset transfer property
 *              list.  This callback function defines what user wants to do
 *              if certain filter fails.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Jan 14, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_filter_callback(hid_t plist_id, H5Z_filter_func_t func, void *op_data)
{
    H5P_genplist_t      *plist;      /* Property list pointer */
    herr_t              ret_value=SUCCEED;   /* return value */
    H5Z_cb_t            cb_struct;
    
    FUNC_ENTER_API(H5Pset_filter_callback, FAIL);
    H5TRACE3("e","ixx",plist_id,func,op_data);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    cb_struct.func = func;
    cb_struct.op_data = op_data;
    
    if (H5P_set(plist,H5D_XFER_FILTER_CB_NAME,&cb_struct)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_btree_ratios
 *
 * Purpose:	Queries B-tree split ratios.  See H5Pset_btree_ratios().
 *
 * Return:	Success:	Non-negative with split ratios returned through
 *				the non-null arguments.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, September 28, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_btree_ratios(hid_t plist_id, double *left/*out*/, double *middle/*out*/,
		    double *right/*out*/)
{
    double btree_split_ratio[3];        /* B-tree node split ratios */
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_btree_ratios, FAIL);
    H5TRACE4("e","ixxx",plist_id,left,middle,right);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get the split ratios */
    if (H5P_get(plist,H5D_XFER_BTREE_SPLIT_RATIO_NAME,&btree_split_ratio)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");

    /* Get values */
    if (left)
        *left = btree_split_ratio[0];
    if (middle)
        *middle = btree_split_ratio[1];
    if (right)
        *right = btree_split_ratio[2];

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_btree_ratios
 *
 * Purpose:	Sets B-tree split ratios for a dataset transfer property
 *		list. The split ratios determine what percent of children go
 *		in the first node when a node splits.  The LEFT ratio is
 *		used when the splitting node is the left-most node at its
 *		level in the tree; the RIGHT ratio is when the splitting node
 *		is the right-most node at its level; and the MIDDLE ratio for
 *		all other cases.  A node which is the only node at its level
 *		in the tree uses the RIGHT ratio when it splits.  All ratios
 *		are real numbers between 0 and 1, inclusive.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, September 28, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_btree_ratios(hid_t plist_id, double left, double middle,
		    double right)
{
    double split_ratio[3];        /* B-tree node split ratios */
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_btree_ratios, FAIL);
    H5TRACE4("e","iddd",plist_id,left,middle,right);

    /* Check arguments */
    if (left<0.0 || left>1.0 || middle<0.0 || middle>1.0 ||
            right<0.0 || right>1.0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "split ratio must satisfy 0.0<=X<=1.0");
    
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    split_ratio[0] = left;
    split_ratio[1] = middle;
    split_ratio[2] = right;

    /* Set the split ratios */
    if (H5P_set(plist,H5D_XFER_BTREE_SPLIT_RATIO_NAME,&split_ratio)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5P_set_vlen_mem_manager
 *
 * Purpose:	Sets the memory allocate/free pair for VL datatypes.  The
 *		allocation routine is called when data is read into a new
 *		array and the free routine is called when H5Dvlen_reclaim is
 *		called.  The alloc_info and free_info are user parameters
 *		which are passed to the allocation and freeing functions
 *		respectively.  To reset the allocate/free functions to the
 *		default setting of using the system's malloc/free functions,
 *		call this routine with alloc_func and free_func set to NULL.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 1, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_set_vlen_mem_manager(H5P_genplist_t *plist, H5MM_allocate_t alloc_func,
        void *alloc_info, H5MM_free_t free_func, void *free_info)
{
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_NOAPI(H5P_set_vlen_mem_manager, FAIL);

    assert(plist);

    /* Update property list */
    if (H5P_set(plist,H5D_XFER_VLEN_ALLOC_NAME,&alloc_func)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");
    if (H5P_set(plist,H5D_XFER_VLEN_ALLOC_INFO_NAME,&alloc_info)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");
    if (H5P_set(plist,H5D_XFER_VLEN_FREE_NAME,&free_func)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");
    if (H5P_set(plist,H5D_XFER_VLEN_FREE_INFO_NAME,&free_info)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_set_vlen_mem_manager() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_vlen_mem_manager
 *
 * Purpose:	Sets the memory allocate/free pair for VL datatypes.  The
 *		allocation routine is called when data is read into a new
 *		array and the free routine is called when H5Dvlen_reclaim is
 *		called.  The alloc_info and free_info are user parameters
 *		which are passed to the allocation and freeing functions
 *		respectively.  To reset the allocate/free functions to the
 *		default setting of using the system's malloc/free functions,
 *		call this routine with alloc_func and free_func set to NULL.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 1, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_vlen_mem_manager(hid_t plist_id, H5MM_allocate_t alloc_func,
        void *alloc_info, H5MM_free_t free_func, void *free_info)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_vlen_mem_manager, FAIL);
    H5TRACE5("e","ixxxx",plist_id,alloc_func,alloc_info,free_func,free_info);

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list");

    /* Update property list */
    if (H5P_set_vlen_mem_manager(plist,alloc_func,alloc_info,free_func,free_info)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set values");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_vlen_mem_manager() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_vlen_mem_manager
 *
 * Purpose:	The inverse of H5Pset_vlen_mem_manager()
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 1, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_vlen_mem_manager(hid_t plist_id, H5MM_allocate_t *alloc_func/*out*/,
			void **alloc_info/*out*/,
			H5MM_free_t *free_func/*out*/,
			void **free_info/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_vlen_mem_manager, FAIL);
    H5TRACE5("e","ixxxx",plist_id,alloc_func,alloc_info,free_func,free_info);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if(alloc_func!=NULL)
        if (H5P_get(plist,H5D_XFER_VLEN_ALLOC_NAME,alloc_func)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");
    if(alloc_info!=NULL)
        if (H5P_get(plist,H5D_XFER_VLEN_ALLOC_INFO_NAME,alloc_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");
    if(free_func!=NULL)
        if (H5P_get(plist,H5D_XFER_VLEN_FREE_NAME,free_func)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");
    if(free_info!=NULL)
        if (H5P_get(plist,H5D_XFER_VLEN_FREE_INFO_NAME,free_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_hyper_vector_size
 *
 * Purpose:	Given a dataset transfer property list, set the number of
 *              "I/O vectors" (offset and length pairs) which are to be
 *              accumulated in memory before being issued to the lower levels
 *              of the library for reading or writing the actual data.
 *              Increasing the number should give better performance, but use
 *              more memory during hyperslab I/O.  The vector size must be
 *              greater than 1.
 *
 *		The default is to use 1024 vectors for I/O during hyperslab
 *              reading/writing.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 9, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_hyper_vector_size(hid_t plist_id, size_t vector_size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_hyper_vector_size, FAIL);
    H5TRACE2("e","iz",plist_id,vector_size);

    /* Check arguments */
    if (vector_size<1)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "vector size too small");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Update property list */
    if (H5P_set(plist,H5D_XFER_HYPER_VECTOR_SIZE_NAME,&vector_size)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_hyper_vector_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_hyper_vector_size
 *
 * Purpose:	Reads values previously set with H5Pset_hyper_vector_size().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 9, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_hyper_vector_size(hid_t plist_id, size_t *vector_size/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_hyper_vector_size, FAIL);
    H5TRACE2("e","ix",plist_id,vector_size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Return values */
    if (vector_size)
        if (H5P_get(plist,H5D_XFER_HYPER_VECTOR_SIZE_NAME,vector_size)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_hyper_vector_size() */

