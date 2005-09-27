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
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Ppkg.h"		/* Property lists		  	*/
#include "H5Zprivate.h"		/* Data filters				*/

/* Pablo mask */
#define PABLO_MASK	H5Pdcpl_mask

/* Interface initialization */
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

/* Local datatypes */

/* Static function prototypes */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_layout
 *
 * Purpose:	Sets the layout of raw data in the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_layout(hid_t plist_id, H5D_layout_t layout)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_layout, FAIL);
    H5TRACE2("e","iDl",plist_id,layout);

    /* Check arguments */
    if (layout < 0 || layout >= H5D_NLAYOUTS)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "raw data layout method is not valid");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set value */
    if(H5P_set(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set layout");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_layout
 *
 * Purpose:	Retrieves layout type of a dataset creation property list.
 *
 * Return:	Success:	The layout type
 *
 *		Failure:	H5D_LAYOUT_ERROR (negative)
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and get property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
H5D_layout_t
H5Pget_layout(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5D_layout_t   ret_value=H5D_LAYOUT_ERROR;

    FUNC_ENTER_API(H5Pget_layout, H5D_LAYOUT_ERROR);
    H5TRACE1("Dl","i",plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5D_LAYOUT_ERROR, "can't find object for ID");

    /* Get value */
    if(H5P_get(plist, H5D_CRT_LAYOUT_NAME, &ret_value) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, H5D_LAYOUT_ERROR, "can't get layout");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_chunk
 *
 * Purpose:	Sets the number of dimensions and the size of each chunk to
 *		the values specified.  The dimensionality of the chunk should
 *		match the dimensionality of the data space.
 *
 *		As a side effect, the layout method is changed to
 *		H5D_CHUNKED.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_chunk(hid_t plist_id, int ndims, const hsize_t dim[/*ndims*/])
{
    int			    i;
    hsize_t real_dims[H5O_LAYOUT_NDIMS]; /* Full-sized array to hold chunk dims */
    H5D_layout_t           layout;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_chunk, FAIL);
    H5TRACE3("e","iIs*[a1]h",plist_id,ndims,dim);

    /* Check arguments */
    if (ndims <= 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "chunk dimensionality must be positive");
    if (ndims > H5S_MAX_RANK)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "chunk dimensionality is too large");
    if (!dim)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no chunk dimensions specified");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Initialize chunk dims to 0s */
    HDmemset(real_dims,0,H5O_LAYOUT_NDIMS*sizeof(hsize_t));
    for (i=0; i<ndims; i++) {
        if (dim[i] <= 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "all chunk dimensions must be positive");
        real_dims[i]=dim[i]; /* Store user's chunk dimensions */
    }

    layout = H5D_CHUNKED;
    if(H5P_set(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set layout");
    if(H5P_set(plist, H5D_CRT_CHUNK_DIM_NAME, &ndims) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set chunk dimensionanlity");
    if(H5P_set(plist, H5D_CRT_CHUNK_SIZE_NAME, real_dims) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set chunk size");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_chunk
 *
 * Purpose:	Retrieves the chunk size of chunked layout.  The chunk
 *		dimensionality is returned and the chunk size in each
 *		dimension is returned through the DIM argument.	 At most
 *		MAX_NDIMS elements of DIM will be initialized.
 *
 * Return:	Success:	Positive Chunk dimensionality.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_chunk(hid_t plist_id, int max_ndims, hsize_t dim[]/*out*/)
{
    int			i;
    int                 ndims;
    H5D_layout_t        layout;
    hsize_t             chunk_size[H5O_LAYOUT_NDIMS];
    H5P_genplist_t *plist;      /* Property list pointer */
    int                 ret_value;

    FUNC_ENTER_API(H5Pget_chunk, FAIL);
    H5TRACE3("Is","iIsx",plist_id,max_ndims,dim);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if(H5P_get(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't get layout"); 
    if(H5D_CHUNKED != layout) 
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a chunked storage layout");
   
    if(H5P_get(plist, H5D_CRT_CHUNK_SIZE_NAME, chunk_size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get chunk size");
    if(H5P_get(plist, H5D_CRT_CHUNK_DIM_NAME, &ndims) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get chunk dimensionality");

    /* Get the dimension sizes */
    for (i=0; i<ndims && i<max_ndims && dim; i++)
        dim[i] = chunk_size[i];

    /* Set the return value */
    ret_value=ndims;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_external
 *
 * Purpose:	Adds an external file to the list of external files. PLIST_ID
 *		should be an object ID for a dataset creation property list.
 *		NAME is the name of an external file, OFFSET is the location
 *		where the data starts in that file, and SIZE is the number of
 *		bytes reserved in the file for the data.
 *
 *		If a dataset is split across multiple files then the files
 *		should be defined in order. The total size of the dataset is
 *		the sum of the SIZE arguments for all the external files.  If
 *		the total size is larger than the size of a dataset then the
 *		dataset can be extended (provided the data space also allows
 *		the extending).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, March	3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_external(hid_t plist_id, const char *name, off_t offset, hsize_t size)
{
    int			idx;
    hsize_t		total, tmp;
    H5O_efl_t           efl;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_external, FAIL);
    H5TRACE4("e","isoh",plist_id,name,offset,size);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name given");
    if (offset<0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "negative external file offset");
    if (size<=0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "zero size");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list");
    if(efl.nused > 0 && H5O_EFL_UNLIMITED==efl.slot[efl.nused-1].size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "previous file size is unlimited");

    if (H5O_EFL_UNLIMITED!=size) {
        for (idx=0, total=size; idx<efl.nused; idx++, total=tmp) {
            tmp = total + efl.slot[idx].size;
            if (tmp <= total)
                HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, FAIL, "total external data size overflowed");
        } /* end for */
    } /* end if */
    

    /* Add to the list */
    if (efl.nused >= efl.nalloc) {
        int na = efl.nalloc + H5O_EFL_ALLOC;
        H5O_efl_entry_t *x = H5MM_realloc (efl.slot, na*sizeof(H5O_efl_entry_t));

        if (!x)
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
        efl.nalloc = na;
        efl.slot = x;
    }
    idx = efl.nused;
    efl.slot[idx].name_offset = 0; /*not entered into heap yet*/
    efl.slot[idx].name = H5MM_xstrdup (name);
    efl.slot[idx].offset = offset;
    efl.slot[idx].size = size;
    efl.nused++;
    
    if(H5P_set(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set external file list");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_external_count
 *
 * Purpose:	Returns the number of external files for this dataset.
 *
 * Return:	Success:	Number of external files
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March  3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_external_count(hid_t plist_id)
{
    H5O_efl_t           efl;
    H5P_genplist_t *plist;      /* Property list pointer */
    int ret_value;              /* return value */

    FUNC_ENTER_API(H5Pget_external_count, FAIL);
    H5TRACE1("Is","i",plist_id);
    
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list");
    
    /* Set return value */
    ret_value=efl.nused;
    
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_external
 *
 * Purpose:	Returns information about an external file.  External files
 *		are numbered from zero to N-1 where N is the value returned
 *		by H5Pget_external_count().  At most NAME_SIZE characters are
 *		copied into the NAME array.  If the external file name is
 *		longer than NAME_SIZE with the null terminator, then the
 *		return value is not null terminated (similar to strncpy()).
 *
 *		If NAME_SIZE is zero or NAME is the null pointer then the
 *		external file name is not returned.  If OFFSET or SIZE are
 *		null pointers then the corresponding information is not
 *		returned.
 *
 * See Also:	H5Pset_external()
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March  3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and get property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_external(hid_t plist_id, int idx, size_t name_size, char *name/*out*/,
		 off_t *offset/*out*/, hsize_t *size/*out*/)
{
    H5O_efl_t           efl;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_external, FAIL);
    H5TRACE6("e","iIszxxx",plist_id,idx,name_size,name,offset,size);
    
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list");
    
    if (idx<0 || idx>=efl.nused)
        HGOTO_ERROR (H5E_ARGS, H5E_BADRANGE, FAIL, "external file index is out of range");

    /* Return values */
    if (name_size>0 && name)
        HDstrncpy (name, efl.slot[idx].name, name_size);
    if (offset)
        *offset = efl.slot[idx].offset;
    if (size)
        *size = efl.slot[idx].size;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pmodify_filter
 *
 * Purpose:	Modifies the specified FILTER in the
 *		transient or permanent output filter pipeline
 *		depending on whether PLIST is a dataset creation or dataset
 *		transfer property list.  The FLAGS argument specifies certain
 *		general properties of the filter and is documented below.
 *		The CD_VALUES is an array of CD_NELMTS integers which are
 *		auxiliary data for the filter.  The integer vlues will be
 *		stored in the dataset object header as part of the filter
 *		information.
 *
 * 		The FLAGS argument is a bit vector of the following fields:
 *
 * 		H5Z_FLAG_OPTIONAL(0x0001)
 *		If this bit is set then the filter is optional.  If the
 *		filter fails during an H5Dwrite() operation then the filter
 *		is just excluded from the pipeline for the chunk for which it
 *		failed; the filter will not participate in the pipeline
 *		during an H5Dread() of the chunk.  If this bit is clear and
 *		the filter fails then the entire I/O operation fails.
 *
 * Note:	This function currently supports only the permanent filter
 *		pipeline.  That is, PLIST_ID must be a dataset creation
 *		property list.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, April  5, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pmodify_filter(hid_t plist_id, H5Z_filter_t filter, unsigned int flags,
	       size_t cd_nelmts, const unsigned int cd_values[/*cd_nelmts*/])
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pmodify_filter, FAIL);
    H5TRACE5("e","iZfIuz*[a3]Iu",plist_id,filter,flags,cd_nelmts,cd_values);

    /* Check args */
    if (filter<0 || filter>H5Z_FILTER_MAX)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid filter identifier");
    if (flags & ~((unsigned)H5Z_FLAG_DEFMASK))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid flags");
    if (cd_nelmts>0 && !cd_values)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no client data values supplied");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get the pipeline property to append to */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");

    /* Modify the filter parameters of the I/O pipeline */
    if(H5Z_modify(&pline, filter, flags, cd_nelmts, cd_values) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add filter to pipeline");

    /* Put the I/O pipeline information back into the property list */
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set pipeline");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pmodify_filter() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_filter
 *
 * Purpose:	Adds the specified FILTER and corresponding properties to the
 *		end of the transient or permanent output filter pipeline
 *		depending on whether PLIST is a dataset creation or dataset
 *		transfer property list.  The FLAGS argument specifies certain
 *		general properties of the filter and is documented below.
 *		The CD_VALUES is an array of CD_NELMTS integers which are
 *		auxiliary data for the filter.  The integer vlues will be
 *		stored in the dataset object header as part of the filter
 *		information.
 *
 * 		The FLAGS argument is a bit vector of the following fields:
 *
 * 		H5Z_FLAG_OPTIONAL(0x0001)
 *		If this bit is set then the filter is optional.  If the
 *		filter fails during an H5Dwrite() operation then the filter
 *		is just excluded from the pipeline for the chunk for which it
 *		failed; the filter will not participate in the pipeline
 *		during an H5Dread() of the chunk.  If this bit is clear and
 *		the filter fails then the entire I/O operation fails.
 *
 * Note:	This function currently supports only the permanent filter
 *		pipeline.  That is, PLIST_ID must be a dataset creation
 *		property list.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for  
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_filter(hid_t plist_id, H5Z_filter_t filter, unsigned int flags,
	       size_t cd_nelmts, const unsigned int cd_values[/*cd_nelmts*/])
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_filter, FAIL);
    H5TRACE5("e","iZfIuz*[a3]Iu",plist_id,filter,flags,cd_nelmts,cd_values);

    /* Check args */
    if (filter<0 || filter>H5Z_FILTER_MAX)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid filter identifier");
    if (flags & ~((unsigned)H5Z_FLAG_DEFMASK))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid flags");
    if (cd_nelmts>0 && !cd_values)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no client data values supplied");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get the pipeline property to append to */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");

    /* Add the filter to the I/O pipeline */
    if(H5Z_append(&pline, filter, flags, cd_nelmts, cd_values) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add filter to pipeline");

    /* Put the I/O pipeline information back into the property list */
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set pipeline");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_nfilters
 *
 * Purpose:	Returns the number of filters in the permanent or transient
 *		pipeline depending on whether PLIST_ID is a dataset creation
 *		or dataset transfer property list.  In each pipeline the
 *		filters are numbered from zero through N-1 where N is the
 *		value returned by this function.  During output to the file
 *		the filters of a pipeline are applied in increasing order
 *		(the inverse is true for input).
 *
 * Note:	Only permanent filters are supported at this time.
 *
 * Return:	Success:	Number of filters or zero if there are none.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, August  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_nfilters(hid_t plist_id)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    int ret_value;              /* return value */

    FUNC_ENTER_API(H5Pget_nfilters, FAIL);
    H5TRACE1("Is","i",plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");

    /* Set return value */
    ret_value=(int)(pline.nfilters);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_filter
 *
 * Purpose:	This is the query counterpart of H5Pset_filter() and returns
 *		information about a particular filter number in a permanent
 *		or transient pipeline depending on whether PLIST_ID is a
 *		dataset creation or transfer property list.  On input,
 *		CD_NELMTS indicates the number of entries in the CD_VALUES
 *		array allocated by the caller while on exit it contains the
 *		number of values defined by the filter.  The IDX should be a
 *		value between zero and N-1 as described for H5Pget_nfilters()
 *		and the function will return failure if the filter number is
 *		out or range.
 * 
 * Return:	Success:	Filter identification number.
 *
 *		Failure:	H5Z_FILTER_ERROR (Negative)
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check paramter and set property for 
 *              generic property list. 
 *
 *-------------------------------------------------------------------------
 */
H5Z_filter_t
H5Pget_filter(hid_t plist_id, int idx, unsigned int *flags/*out*/,
	       size_t *cd_nelmts/*in_out*/, unsigned cd_values[]/*out*/,
	       size_t namelen, char name[]/*out*/)
{
    H5O_pline_t         pline;  /* Filter pipeline */
    H5Z_filter_info_t *filter;  /* Pointer to filter information */
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t		i;      /* Local index variable */
    H5Z_filter_t ret_value;     /* return value */
    
    FUNC_ENTER_API(H5Pget_filter, H5Z_FILTER_ERROR);
    H5TRACE7("Zf","iIsx*zxzx",plist_id,idx,flags,cd_nelmts,cd_values,namelen,
             name);
    
    /* Check args */
    if (cd_nelmts || cd_values) {
        if (cd_nelmts && *cd_nelmts>256)
            /*
             * It's likely that users forget to initialize this on input, so
             * we'll check that it has a reasonable value.  The actual number
             * is unimportant because the H5O layer will detect when a message
             * is too large.
             */
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5Z_FILTER_ERROR, "probable uninitialized *cd_nelmts argument");
        if (cd_nelmts && *cd_nelmts>0 && !cd_values)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5Z_FILTER_ERROR, "client data values not supplied");

        /*
         * If cd_nelmts is null but cd_values is non-null then just ignore
         * cd_values
         */
        if (!cd_nelmts)
            cd_values = NULL;
    }

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5Z_FILTER_ERROR, "can't find object for ID");

    /* Get pipeline info */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, H5Z_FILTER_ERROR, "can't get pipeline");

    /* Check more args */
    if (idx<0 || (size_t)idx>=pline.nfilters)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5Z_FILTER_ERROR, "filter number is invalid");

    /* Set pointer to particular filter to query */
    filter=&pline.filter[idx];

    if (flags)
        *flags = filter->flags;
    if (cd_values) {
        for (i=0; i<filter->cd_nelmts && i<*cd_nelmts; i++)
            cd_values[i] = filter->cd_values[i];
    }
    if (cd_nelmts)
        *cd_nelmts = filter->cd_nelmts;

    if (namelen>0 && name) {
        const char *s = filter->name;

        if (!s) {
            H5Z_class_t *cls = H5Z_find(filter->id);

            if (cls)
                s = cls->name;
        }
        if (s)
            HDstrncpy(name, s, namelen);
        else
            name[0] = '\0';
    }
    
    /* Set return value */
    ret_value=filter->id;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_filter_by_id
 *
 * Purpose:	This is an additional query counterpart of H5Pset_filter() and
 *              returns information about a particular filter in a permanent
 *		or transient pipeline depending on whether PLIST_ID is a
 *		dataset creation or transfer property list.  On input,
 *		CD_NELMTS indicates the number of entries in the CD_VALUES
 *		array allocated by the caller while on exit it contains the
 *		number of values defined by the filter.  The ID should be the
 *              filter ID to retrieve the parameters for.  If the filter is not
 *              set for the property list, an error will be returned.
 * 
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, April  5, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_filter_by_id(hid_t plist_id, H5Z_filter_t id, unsigned int *flags/*out*/,
	       size_t *cd_nelmts/*in_out*/, unsigned cd_values[]/*out*/,
	       size_t namelen, char name[]/*out*/)
{
    H5O_pline_t         pline;  /* Filter pipeline */
    H5Z_filter_info_t *filter;  /* Pointer to filter information */
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t		i;      /* Local index variable */
    herr_t ret_value=SUCCEED;   /* Return value */
    
    FUNC_ENTER_API(H5Pget_filter_by_id, FAIL);
    H5TRACE7("e","iZfx*zxzx",plist_id,id,flags,cd_nelmts,cd_values,namelen,
             name);
    
    /* Check args */
    if (cd_nelmts || cd_values) {
        if (cd_nelmts && *cd_nelmts>256)
            /*
             * It's likely that users forget to initialize this on input, so
             * we'll check that it has a reasonable value.  The actual number
             * is unimportant because the H5O layer will detect when a message
             * is too large.
             */
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "probable uninitialized *cd_nelmts argument");
        if (cd_nelmts && *cd_nelmts>0 && !cd_values)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "client data values not supplied");

        /*
         * If cd_nelmts is null but cd_values is non-null then just ignore
         * cd_values
         */
        if (!cd_nelmts)
            cd_values = NULL;
    }

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get pipeline info */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");

    /* Get pointer to filter in pipeline */
    if ((filter=H5Z_filter_info(&pline,id))==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "filter ID is invalid");

    /* Copy filter information into user's parameters */
    if (flags)
        *flags = filter->flags;
    if (cd_values) {
        for (i=0; i<filter->cd_nelmts && i<*cd_nelmts; i++)
            cd_values[i] = filter->cd_values[i];
    }
    if (cd_nelmts)
        *cd_nelmts = filter->cd_nelmts;
    if (namelen>0 && name) {
        const char *s = filter->name;

        if (!s) {
            H5Z_class_t *cls = H5Z_find(filter->id);

            if (cls)
                s = cls->name;
        }
        if (s)
            HDstrncpy(name, s, namelen);
        else
            name[0] = '\0';
    }
    
done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_filter_by_id() */


/*-------------------------------------------------------------------------
 * Function:	H5Pall_filters_avail
 *
 * Purpose:	This is a query routine to verify that all the filters set
 *              in the dataset creation property list are available currently.
 * 
 * Return:	Success:	TRUE if all filters available, FALSE if one or
 *                              more filters not currently available.
 *		Failure:	FAIL on error
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, April  8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Pall_filters_avail(hid_t plist_id)
{
    H5O_pline_t         pline;  /* Filter pipeline */
    H5P_genplist_t *plist;      /* Property list pointer */
    hbool_t ret_value=TRUE;     /* return value */
    
    FUNC_ENTER_API(H5Pall_filters_avail, UFAIL);
    H5TRACE1("b","i",plist_id);
    
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, UFAIL, "can't find object for ID");

    /* Get pipeline info */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, UFAIL, "can't get pipeline");

    /* Set return value */
    if((ret_value=H5Z_all_filters_avail(&pline))==UFAIL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, UFAIL, "can't check pipeline information");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pall_filters_avail() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_deflate
 *
 * Purpose:	Sets the compression method for a permanent or transient
 *		filter pipeline (depending on whether PLIST_ID is a dataset
 *		creation or transfer property list) to H5Z_FILTER_DEFLATE
 *		and the compression level to LEVEL which should be a value
 *		between zero and nine, inclusive.  Lower compression levels
 *		are faster but result in less compression.  This is the same
 *		algorithm as used by the GNU gzip program.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list. 
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_deflate(hid_t plist_id, unsigned level)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */
    
    FUNC_ENTER_API(H5Pset_deflate, FAIL);
    H5TRACE2("e","iIu",plist_id,level);

    /* Check arguments */
    if (level>9)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid deflate level");
   
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Add the filter */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");
    if(H5Z_append(&pline, H5Z_FILTER_DEFLATE, H5Z_FLAG_OPTIONAL, 1, &level)<0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add deflate filter to pipeline");
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_deflate() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_szip
 *
 * Purpose:	Sets the compression method for a permanent or transient
 *		filter pipeline (depending on whether PLIST_ID is a dataset
 *		creation or transfer property list) to H5Z_FILTER_SZIP
 *		Szip is a special compression package that is said to be good
 *              for scientific data.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Kent Yang
 *              Tuesday, April 1, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_szip(hid_t plist_id, unsigned options_mask, unsigned pixels_per_block)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    unsigned cd_values[2];      /* Filter parameters */
    herr_t ret_value=SUCCEED;   /* return value */
    
    FUNC_ENTER_API(H5Pset_szip, FAIL);
    H5TRACE3("e","iIuIu",plist_id,options_mask,pixels_per_block);
    
    /* Check arguments */
    if ((pixels_per_block%2)==1)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "pixels_per_block is not even");
    if (pixels_per_block>H5_SZIP_MAX_PIXELS_PER_BLOCK)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "pixels_per_block is too large");
   
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Always set "raw" (no szip header) flag for data */
    options_mask |= H5_SZIP_RAW_OPTION_MASK;

    /* Mask off the LSB and MSB options, if they were given */
    /* (The HDF5 library sets them internally, as needed) */
    options_mask &= ~(H5_SZIP_LSB_OPTION_MASK|H5_SZIP_MSB_OPTION_MASK);

    /* Set the parameters for the filter */
    cd_values[0]=options_mask;
    cd_values[1]=pixels_per_block;

    /* Add the filter */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");
    if(H5Z_append(&pline, H5Z_FILTER_SZIP, H5Z_FLAG_OPTIONAL, 2, cd_values)<0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add szip filter to pipeline");
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_szip() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_shuffle
 *
 * Purpose:	Sets the shuffling method for a permanent 
 *		filter to H5Z_FILTER_SHUFFLE
 *		and bytes of the datatype of the array to be shuffled 
 *              
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Kent Yang
 *              Wednesday, November 13, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_shuffle(hid_t plist_id)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */
    
    FUNC_ENTER_API(H5Pset_shuffle, FAIL);
    H5TRACE1("e","i",plist_id);

    /* Check arguments */
    if(TRUE != H5P_isa_class(plist_id, H5P_DATASET_CREATE))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list");
   
    /* Get the plist structure */
    if(NULL == (plist = H5I_object(plist_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Add the filter */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");
    if(H5Z_append(&pline, H5Z_FILTER_SHUFFLE, H5Z_FLAG_OPTIONAL, 0, NULL)<0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to shuffle the data");
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_shuffle() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fletcher32
 *
 * Purpose:	Sets Fletcher32 checksum of EDC for a dataset creation 
 *              property list.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Dec 19, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fletcher32(hid_t plist_id)
{
    H5O_pline_t         pline;
    H5P_genplist_t      *plist;      /* Property list pointer */
    herr_t              ret_value=SUCCEED;   /* return value */
    
    FUNC_ENTER_API(H5Pset_fletcher32, FAIL);
    H5TRACE1("e","i",plist_id);
   
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Add the Fletcher32 checksum as a filter */
    if(H5P_get(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline");
    if(H5Z_append(&pline, H5Z_FILTER_FLETCHER32, H5Z_FLAG_MANDATORY, 0, NULL)<0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add deflate filter to pipeline");
    if(H5P_set(plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_fletcher32() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fill_value
 *
 * Purpose:	Set the fill value for a dataset creation property list. The
 *		VALUE is interpretted as being of type TYPE, which need not
 *		be the same type as the dataset but the library must be able
 *		to convert VALUE to the dataset type when the dataset is
 *		created.  If VALUE is NULL, it will be interpreted as 
 *		undefining fill value.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for 
 *              generic property list. 
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fill_value(hid_t plist_id, hid_t type_id, const void *value)
{
    H5O_fill_t	        fill;
    H5T_t		*type = NULL;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */
   
    FUNC_ENTER_API(H5Pset_fill_value, FAIL);
    H5TRACE3("e","iix",plist_id,type_id,value);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get the "basic" fill value structure */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value");

    /* Reset the fill structure */
    if(H5O_reset(H5O_FILL_ID, &fill)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't reset fill value");

    if(value) {
	if (NULL==(type=H5I_object_verify(type_id, H5I_DATATYPE)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");

	/* Set the fill value */
        if (NULL==(fill.type=H5T_copy(type, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy data type");
        fill.size = H5T_get_size(type);
        if (NULL==(fill.buf=H5MM_malloc(fill.size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "memory allocation failed for fill value");
        HDmemcpy(fill.buf, value, fill.size);
    } else {
	fill.type = fill.buf = NULL;
	fill.size = (size_t)-1;
    }

    if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set fill value");
  	     
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_fill_value
 *
 * Purpose:	Queries the fill value property of a dataset creation
 *		property list.  The fill value is returned through the VALUE
 *		pointer and the memory is allocated by the caller.  The fill
 *		value will be converted from its current data type to the
 *		specified TYPE.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and get property for 
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fill_value(hid_t plist_id, hid_t type_id, void *value/*out*/)
{
    H5O_fill_t          fill;
    H5T_t		*type = NULL;		/*data type		*/
    H5T_path_t		*tpath = NULL;		/*type conversion info	*/
    void		*buf = NULL;		/*conversion buffer	*/
    void		*bkg = NULL;		/*conversion buffer	*/
    hid_t		src_id = -1;		/*source data type id	*/
    herr_t              ret_value=SUCCEED;      /* Return value */
    H5P_genplist_t *plist;      /* Property list pointer */
    
    FUNC_ENTER_API(H5Pget_fill_value, FAIL);
    H5TRACE3("e","iix",plist_id,type_id,value);

    /* Check arguments */
    if (NULL==(type=H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (!value)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,"no fill value output buffer");
 
    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /*
     * If no fill value is defined then return an error.  We can't even
     * return zero because we don't know the data type of the dataset and
     * data type conversion might not have resulted in zero.  If fill value
     * is undefined, also return error.
     */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value"); 
    if(fill.size == (size_t)-1)
	HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "fill value is undefined");

    if(fill.size == 0) {
	HDmemset(value, 0, H5T_get_size(type));
   	HGOTO_DONE(SUCCEED);
    } 	    
     /*
     * Can we convert between the source and destination data types?
     */
    if(NULL==(tpath=H5T_path_find(fill.type, type, NULL, NULL, H5AC_dxpl_id)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between src and dst data types");
    src_id = H5I_register(H5I_DATATYPE, H5T_copy (fill.type, H5T_COPY_TRANSIENT));
    if (src_id<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy/register data type");

    /*
     * Data type conversions are always done in place, so we need a buffer
     * other than the fill value buffer that is large enough for both source
     * and destination.  The app-supplied buffer might do okay.
     */
    if (H5T_get_size(type)>=H5T_get_size(fill.type)) {    
        buf = value;
        if (H5T_path_bkg(tpath) && NULL==(bkg=H5MM_malloc(H5T_get_size(type))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion");
    } else {
        if (NULL==(buf=H5MM_malloc(H5T_get_size(fill.type))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion");
        if (H5T_path_bkg(tpath))
            bkg = value;
    }
    HDmemcpy(buf, fill.buf, H5T_get_size(fill.type));
        
    /* Do the conversion */
    if (H5T_convert(tpath, src_id, type_id, (hsize_t)1, 0, 0, buf, bkg, H5AC_dxpl_id)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");
    if (buf!=value)
        HDmemcpy(value, buf, H5T_get_size(type));

done:
    if (buf!=value)
        H5MM_xfree(buf);
    if (bkg!=value)
        H5MM_xfree(bkg);
    if (src_id>=0)
        H5I_dec_ref(src_id);
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5P_is_fill_value_defined
 *
 * Purpose:	Check if fill value is defined.  Internal version of function
 * 
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *              Extracted from H5P_fill_value_defined, QAK, Dec. 13, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_is_fill_value_defined(const struct H5O_fill_t *fill, H5D_fill_value_t *status)
{
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5P_is_fill_value_defined, FAIL);

    assert(fill);
    assert(status);

    /* Check if the fill value was never set */
    if(fill->size == (size_t)-1 && !fill->buf) {
	*status = H5D_FILL_VALUE_UNDEFINED;
    }
    /* Check if the fill value was set to the default fill value by the library */
    else if(fill->size == 0 && !fill->buf) {
	*status = H5D_FILL_VALUE_DEFAULT;
    }
    /* Check if the fill value was set by the application */
    else if(fill->size > 0 && fill->buf) {
	*status = H5D_FILL_VALUE_USER_DEFINED;
    }
    else {
	*status = H5D_FILL_VALUE_ERROR;
        HGOTO_ERROR(H5E_PLIST, H5E_BADRANGE, FAIL, "invalid combination of fill-value info"); 
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_is_fill_value_defined() */


/*-------------------------------------------------------------------------
 * Function:    H5P_fill_value_defined
 *
 * Purpose:	Check if fill value is defined.  Internal version of function
 * 
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *              
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_fill_value_defined(H5P_genplist_t *plist, H5D_fill_value_t *status)
{
    herr_t		ret_value = SUCCEED;
    H5O_fill_t		fill;

    FUNC_ENTER_NOAPI(H5P_fill_value_defined, FAIL);

    assert(plist);
    assert(status);

    /* Get the fill value struct */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value"); 

    /* Get the fill-value status */
    if(H5P_is_fill_value_defined(&fill, status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't check fill value status"); 

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_fill_value_defined() */


/*-------------------------------------------------------------------------
 * Function:    H5Pfill_value_defined
 *
 * Purpose:	Check if fill value is defined.
 * 
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *              
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pfill_value_defined(hid_t plist_id, H5D_fill_value_t *status)
{
    H5P_genplist_t 	*plist;
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_API(H5Pfill_value_defined, FAIL);
    H5TRACE2("e","i*DF",plist_id,status);

    assert(status);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Call the internal function */
    if(H5P_fill_value_defined(plist, status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value info"); 

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pfill_value_defined() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_alloc_time
 *
 * Purpose:     Set space allocation time for dataset during creation.
 *		Valid values are H5D_ALLOC_TIME_DEFAULT, H5D_ALLOC_TIME_EARLY,
 *			H5D_ALLOC_TIME_LATE, H5D_ALLOC_TIME_INCR
 * 
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 * 		Wednesday, January 16, 2002
 *
 * Modifications:
 *		
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_alloc_time(hid_t plist_id, H5D_alloc_time_t alloc_time)
{
    H5P_genplist_t *plist; 	/* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value 	 */

    FUNC_ENTER_API(H5Pset_alloc_time, FAIL);
    H5TRACE2("e","iDa",plist_id,alloc_time);

    /* Get the property list structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5D_CRT_ALLOC_TIME_NAME, &alloc_time) < 0)
	HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set space allocation time");

done:
    FUNC_LEAVE_API(ret_value);  
}


/*-------------------------------------------------------------------------
 * Function:    H5Pget_alloc_time
 *
 * Purpose:     Get space allocation time for dataset creation.
 *		Valid values are H5D_ALLOC_TIME_DEFAULT, H5D_ALLOC_TIME_EARLY,
 *			H5D_ALLOC_TIME_LATE, H5D_ALLOC_TIME_INCR
 * 
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *              
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_alloc_time(hid_t plist_id, H5D_alloc_time_t *alloc_time/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value          */

    FUNC_ENTER_API(H5Pget_alloc_time, FAIL);
    H5TRACE2("e","ix",plist_id,alloc_time);

    /* Get the property list structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if(!alloc_time || H5P_get(plist, H5D_CRT_ALLOC_TIME_NAME, alloc_time) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get space allocation time");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fill_time
 *
 * Purpose:	Set fill value writing time for dataset.  Valid values are
 *		H5D_FILL_TIME_ALLOC and H5D_FILL_TIME_NEVER.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fill_time(hid_t plist_id, H5D_fill_time_t fill_time)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value          */

    FUNC_ENTER_API(H5Pset_fill_time, FAIL);
    H5TRACE2("e","iDf",plist_id,fill_time);

    /* Check arguments */
    if(fill_time<H5D_FILL_TIME_ALLOC || fill_time>H5D_FILL_TIME_IFSET)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fill time setting");

    /* Get the property list structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(H5P_set(plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set space allocation time");

done:
    FUNC_LEAVE_API(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5Pget_fill_time
 *
 * Purpose:	Get fill value writing time.  Valid values are H5D_NEVER
 *		and H5D_ALLOC.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fill_time(hid_t plist_id, H5D_fill_time_t *fill_time/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value          */

    FUNC_ENTER_API(H5Pget_fill_time, FAIL);
    H5TRACE2("e","ix",plist_id,fill_time);

    /* Get the property list structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if(!fill_time || H5P_get(plist, H5D_CRT_FILL_TIME_NAME, fill_time) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set space allocation time");

done:
    FUNC_LEAVE_API(ret_value);
}

