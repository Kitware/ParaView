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
 *      the datatype string padding for the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5Tpkg.h"		/*data-type functions			  */

#define PABLO_MASK	H5Tstrpad_mask

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5T_init_strpad_interface
static herr_t H5T_init_strpad_interface(void);


/*--------------------------------------------------------------------------
NAME
   H5T_init_strpad_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_strpad_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_strpad_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_strpad_interface);

    FUNC_LEAVE_NOAPI(H5T_init());
} /* H5T_init_strpad_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_strpad
 *
 * Purpose:	The method used to store character strings differs with the
 *		programming language: C usually null terminates strings while
 *		Fortran left-justifies and space-pads strings.	This property
 *		defines the storage mechanism for the string.
 *		
 * Return:	Success:	The character set of a string type.
 *
 *		Failure:	H5T_STR_ERROR (Negative)
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_str_t
H5Tget_strpad(hid_t type_id)
{
    H5T_t	*dt = NULL;
    H5T_str_t	ret_value;

    FUNC_ENTER_API(H5Tget_strpad, H5T_STR_ERROR);
    H5TRACE1("Tz","i",type_id);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_STR_ERROR, "not a data type");
    while (dt->parent && !H5T_IS_STRING(dt))
        dt = dt->parent;  /*defer to parent*/
    if (!H5T_IS_STRING(dt))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_STR_ERROR, "operation not defined for data type class");
    
    /* result */
    if(H5T_STRING == dt->type)
        ret_value = dt->u.atomic.u.s.pad;
    else if(H5T_VLEN == dt->type && H5T_VLEN_STRING == dt->u.vlen.type)
        ret_value = dt->u.vlen.pad;
    else
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, H5T_STR_ERROR, "can't get strpad info");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tset_strpad
 *
 * Purpose:	The method used to store character strings differs with the
 *		programming language: C usually null terminates strings while
 *		Fortran left-justifies and space-pads strings.	This property
 *		defines the storage mechanism for the string.
 *
 *		When converting from a long string to a short string if the
 *		short string is H5T_STR_NULLPAD or H5T_STR_SPACEPAD then the
 *		string is simply truncated; otherwise if the short string is
 *		H5T_STR_NULLTERM it will be truncated and a null terminator
 *		is appended.
 *
 *		When converting from a short string to a long string, the
 *		long string is padded on the end by appending nulls or
 *		spaces.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_strpad(hid_t type_id, H5T_str_t strpad)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tset_strpad, FAIL);
    H5TRACE2("e","iTz",type_id,strpad);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (H5T_STATE_TRANSIENT!=dt->state)
	HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    if (strpad < 0 || strpad >= H5T_NSTR)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal string pad type");
    while (dt->parent && !H5T_IS_STRING(dt))
        dt = dt->parent;  /*defer to parent*/
    if (!H5T_IS_STRING(dt))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not defined for data type class");
    
    /* Commit */
    if(H5T_STRING == dt->type)
        dt->u.atomic.u.s.pad = strpad;
    else if(H5T_VLEN == dt->type && H5T_VLEN_STRING == dt->u.vlen.type)
        dt->u.vlen.pad = strpad;
    else
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, H5T_STR_ERROR, "can't set strpad info");

done:
    FUNC_LEAVE_API(ret_value);
}

