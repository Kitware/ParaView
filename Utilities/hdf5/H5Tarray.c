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
 * Module Info: This module contains the functionality for array datatypes in
 *      the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK	H5Tarray_mask

#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5FLprivate.h"	/*Free Lists	  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5Tpkg.h"		/*data-type functions			  */

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5T_init_array_interface
static herr_t H5T_init_array_interface(void);

/* Declare extern the free list for H5T_t's */
H5FL_EXTERN(H5T_t);


/*--------------------------------------------------------------------------
NAME
   H5T_init_array_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_array_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_array_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_array_interface);

    FUNC_LEAVE_NOAPI(H5T_init());
} /* H5T_init_array_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tarray_create
 *
 * Purpose:	Create a new array data type based on the specified BASE_TYPE.
 *		The type is an array with NDIMS dimensionality and the size of the
 *      array is DIMS. The total member size should be relatively small.
 *      PERM is currently unimplemented and unused, but is designed to contain
 *      the dimension permutation from C order.
 *      Array datatypes are currently limited to H5S_MAX_RANK number of
 *      dimensions and must have the number of dimensions set greater than
 *      0. (i.e. 0 > ndims <= H5S_MAX_RANK)  All dimensions sizes must be greater
 *      than 0 also.
 *
 * Return:	Success:	ID of new array data type
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Oct 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tarray_create(hid_t base_id, int ndims, const hsize_t dim[/* ndims */],
    const int perm[/* ndims */])
{
    H5T_t	*base = NULL;		/* base data type	*/
    H5T_t	*dt = NULL;		    /* new array data type	*/
    int    i;                  /* local index variable */
    hid_t	ret_value;	/* return value			*/
    
    FUNC_ENTER_API(H5Tarray_create, FAIL);
    H5TRACE4("i","iIs*h*Is",base_id,ndims,dim,perm);

    /* Check args */
    if (ndims<1 || ndims>H5S_MAX_RANK)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid dimensionality");
    if (ndims>0 && !dim)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no dimensions specified");
    for(i=0; i<ndims; i++)
        if(!(dim[i]>0))
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "zero-sized dimension specified");
    if (NULL==(base=H5I_object_verify(base_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype");

    /* Create the actual array datatype */
    if ((dt=H5T_array_create(base,ndims,dim,perm))==NULL)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to create datatype");

    /* Atomize the type */
    if ((ret_value=H5I_register(H5I_DATATYPE, dt))<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register datatype");

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Tarray_create */


/*-------------------------------------------------------------------------
 * Function:	H5T_array_create
 *
 * Purpose:	Internal routine to create a new array data type based on the
 *      specified BASE_TYPE.  The type is an array with NDIMS dimensionality
 *      and the size of the array is DIMS.  PERM is currently unimplemented
 *      and unused, but is designed to contain the dimension permutation from
 *      C order.  Array datatypes are currently limited to H5S_MAX_RANK number
 *      of *      dimensions.
 *
 * Return:	Success:	ID of new array data type
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Oct 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_array_create(H5T_t *base, int ndims, const hsize_t dim[/* ndims */],
    const int perm[/* ndims */])
{
    H5T_t	*ret_value = NULL;		/*new array data type	*/
    int    i;                  /* local index variable */

    FUNC_ENTER_NOAPI(H5T_array_create, NULL);

    assert(base);
    assert(ndims>0 && ndims<=H5S_MAX_RANK);
    assert(dim);

    /* Build new type */
    if (NULL==(ret_value = H5FL_CALLOC(H5T_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    ret_value->ent.header = HADDR_UNDEF;
    ret_value->type = H5T_ARRAY;

    /* Copy the base type of the array */
    ret_value->parent = H5T_copy(base, H5T_COPY_ALL);

    /* Set the array parameters */
    ret_value->u.array.ndims = ndims;

    /* Copy the array dimensions & compute the # of elements in the array */
    for(i=0, ret_value->u.array.nelem=1; i<ndims; i++) {
        H5_ASSIGN_OVERFLOW(ret_value->u.array.dim[i],dim[i],hsize_t,size_t);
        ret_value->u.array.nelem *= (size_t)dim[i];
    } /* end for */

    /* Copy the dimension permutations */
    for(i=0; i<ndims; i++)
        ret_value->u.array.perm[i] = perm ? perm[i] : i;

    /* Set the array's size (number of elements * element datatype's size) */
    ret_value->size = ret_value->parent->size * ret_value->u.array.nelem;

    /*
     * Set the "force conversion" flag if the base datatype indicates
     */
    if(base->force_conv==TRUE)
        ret_value->force_conv=TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5T_array_create */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_array_ndims
 *
 * Purpose:	Query the number of dimensions for an array datatype.
 *
 * Return:	Success:	Number of dimensions of the array datatype
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 6, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Tget_array_ndims(hid_t type_id)
{
    H5T_t	*dt = NULL;		    /* pointer to array data type	*/
    int	ret_value;	    /* return value			*/
    
    FUNC_ENTER_API(H5Tget_array_ndims, FAIL);
    H5TRACE1("Is","i",type_id);

    /* Check args */
    if (NULL==(dt=H5I_object_verify(type_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object");
    if(dt->type!=H5T_ARRAY)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an array datatype");

    /* Retrieve the number of dimensions */
    ret_value = H5T_get_array_ndims(dt);

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Tget_array_ndims */


/*-------------------------------------------------------------------------
 * Function:	H5T_get_array_ndims
 *
 * Purpose:	Private function for H5T_get_array_ndims.  Query the number 
 *              of dimensions for an array datatype.
 *
 * Return:	Success:	Number of dimensions of the array datatype
 *		Failure:	Negative
 *
 * Programmer:	Raymond Lu
 *              October 10, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5T_get_array_ndims(H5T_t *dt)
{
    int	ret_value;	    /* return value			*/
    
    FUNC_ENTER_NOAPI(H5T_get_array_ndims, FAIL);

    assert(dt);
    assert(dt->type==H5T_ARRAY);

    /* Retrieve the number of dimensions */
    ret_value=dt->u.array.ndims;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5T_get_array_ndims */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_array_dims
 *
 * Purpose:	Query the sizes of dimensions for an array datatype.
 *
 * Return:	Success:	Number of dimensions of the array type
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 6, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Tget_array_dims(hid_t type_id, hsize_t dims[], int perm[])
{
    H5T_t	*dt = NULL;		    /* pointer to array data type	*/
    int	ret_value;	/* return value			*/
    
    FUNC_ENTER_API(H5Tget_array_dims, FAIL);
    H5TRACE3("Is","i*h*Is",type_id,dims,perm);

    /* Check args */
    if (NULL==(dt=H5I_object_verify(type_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object");
    if(dt->type!=H5T_ARRAY)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an array datatype");

    /* Retrieve the sizes of the dimensions */
    if((ret_value=H5T_get_array_dims(dt, dims, perm))<0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "unable to get dimension sizes");
done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Tget_array_dims */


/*-------------------------------------------------------------------------
 * Function:	H5T_get_array_dims
 *
 * Purpose:	Private function for H5T_get_array_dims.  Query the sizes 
 *              of dimensions for an array datatype.
 *
 * Return:	Success:	Number of dimensions of the array type
 *		Failure:	Negative
 *
 * Programmer:  Raymond Lu
 *              October 10, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5T_get_array_dims(H5T_t *dt, hsize_t dims[], int perm[])
{
    int	ret_value;	/* return value			*/
    int    i;                  /* Local index variable */
    
    FUNC_ENTER_NOAPI(H5T_get_array_dims, FAIL);

    assert(dt);
    assert(dt->type==H5T_ARRAY);

    /* Retrieve the sizes of the dimensions */
    if(dims) 
        for(i=0; i<dt->u.array.ndims; i++)
            dims[i]=dt->u.array.dim[i];

    /* Retrieve the dimension permutations */
    if(perm) 
        for(i=0; i<dt->u.array.ndims; i++)
            perm[i]=dt->u.array.perm[i];

    /* Pass along the array rank as the return value */
    ret_value=dt->u.array.ndims;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5T_get_array_dims */

