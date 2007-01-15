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
 *      the datatype byte order for the H5T interface.
 */

#define H5T_PACKAGE    /*suppress error about including H5Tpkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5T_init_order_interface


#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Iprivate.h"    /* IDs            */
#include "H5Tpkg.h"    /* Datatypes        */


/*--------------------------------------------------------------------------
NAME
   H5T_init_order_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_order_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_order_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_order_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_order_interface() */


/*-------------------------------------------------------------------------
 * Function:  H5Tget_order
 *
 * Purpose:  Returns the byte order of a data type.
 *
 * Return:  Success:  A byte order constant
 *
 *    Failure:  H5T_ORDER_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *    Wednesday, January  7, 1998
 *
 * Modifications:
 *   Robb Matzke, 22 Dec 1998
 *  Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_order_t
H5Tget_order(hid_t type_id)
{
    H5T_t    *dt = NULL;
    H5T_order_t    ret_value;

    FUNC_ENTER_API(H5Tget_order, H5T_ORDER_ERROR)
    H5TRACE1("To","i",type_id);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_ORDER_ERROR, "not a data type")
    while (dt->shared->parent)
        dt = dt->shared->parent; /*defer to parent*/
    if (!H5T_IS_ATOMIC(dt->shared))
  HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, H5T_ORDER_ERROR, "operation not defined for specified data type")

    /* Order */
    ret_value = dt->shared->u.atomic.order;

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Tset_order
 *
 * Purpose:  Sets the byte order for a data type.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Wednesday, January  7, 1998
 *
 * Modifications:
 *   Robb Matzke, 22 Dec 1998
 *  Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_order(hid_t type_id, H5T_order_t order)
{
    H5T_t  *dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tset_order, FAIL)
    H5TRACE2("e","iTo",type_id,order);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
  HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")
    if (H5T_STATE_TRANSIENT!=dt->shared->state)
  HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only")
    if (order < H5T_ORDER_LE || order > H5T_ORDER_NONE)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal byte order")
    if (H5T_ENUM==dt->shared->type && dt->shared->u.enumer.nmembs>0)
  HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not allowed after members are defined")
    while (dt->shared->parent)
        dt = dt->shared->parent; /*defer to parent*/
    if (!H5T_IS_ATOMIC(dt->shared))
  HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "operation not defined for specified data type")

    /* Commit */
    dt->shared->u.atomic.order = order;

done:
    FUNC_LEAVE_API(ret_value)
}

