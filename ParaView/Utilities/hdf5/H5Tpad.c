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
 * Module Info: This module contains the functionality for setting & querying
 *      the datatype padding for the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5Tpkg.h"		/*data-type functions			  */

#define PABLO_MASK	H5Tpad_mask

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5T_init_pad_interface
static herr_t H5T_init_pad_interface(void);


/*--------------------------------------------------------------------------
NAME
   H5T_init_pad_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_pad_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_pad_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_pad_interface);

    FUNC_LEAVE_NOAPI(H5T_init());
} /* H5T_init_pad_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_pad
 *
 * Purpose:	Gets the least significant pad type and the most significant
 *		pad type and returns their values through the LSB and MSB
 *		arguments, either of which may be the null pointer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tget_pad(hid_t type_id, H5T_pad_t *lsb/*out*/, H5T_pad_t *msb/*out*/)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tget_pad, FAIL);
    H5TRACE3("e","ixx",type_id,lsb,msb);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    while (dt->parent)
        dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not defined for specified data type");
    
    /* Get values */
    assert(H5T_is_atomic(dt));
    if (lsb)
        *lsb = dt->u.atomic.lsb_pad;
    if (msb)
        *msb = dt->u.atomic.msb_pad;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tset_pad
 *
 * Purpose:	Sets the LSB and MSB pad types.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 *	Robb Matzke, 22 Dec 1998
 *	Also works with derived data types.
 *	
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_pad(hid_t type_id, H5T_pad_t lsb, H5T_pad_t msb)
{
    H5T_t *dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tset_pad, FAIL);
    H5TRACE3("e","iTpTp",type_id,lsb,msb);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (H5T_STATE_TRANSIENT!=dt->state)
	HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    if (lsb < 0 || lsb >= H5T_NPAD || msb < 0 || msb >= H5T_NPAD)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pad type");
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not allowed after members are defined");
    while (dt->parent)
        dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not defined for specified data type");

    /* Commit */
    assert(H5T_is_atomic(dt));
    dt->u.atomic.lsb_pad = lsb;
    dt->u.atomic.msb_pad = msb;

done:
    FUNC_LEAVE_API(ret_value);
}

