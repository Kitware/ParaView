/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, November 10, 1998
 *
 * Purpose:     "None" selection data space I/O functions.
 */

#define H5S_PACKAGE             /*suppress error about including H5Spkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Iprivate.h"
#include "H5Spkg.h"
#include "H5Vprivate.h"
#include "H5Dprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5Snone_mask
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

static herr_t H5S_select_none(H5S_t *space);


/*--------------------------------------------------------------------------
 NAME
    H5S_none_select_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_none_select_serialize(space, buf)
        H5S_t *space;           IN: Dataspace pointer of selection to serialize
        uint8 *buf;             OUT: Buffer to put serialized selection into
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Serializes the current element selection into a buffer.  (Primarily for
    storing on disk).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_none_select_serialize (const H5S_t *space, uint8_t *buf)
{
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_none_select_serialize, FAIL);

    assert(space);

    /* Store the preamble information */
    UINT32ENCODE(buf, (uint32_t)space->select.type);  /* Store the type of selection */
    UINT32ENCODE(buf, (uint32_t)1);  /* Store the version number */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the un-used padding */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the additional information length */

    /* Set success */
    ret_value=SUCCEED;

    FUNC_LEAVE (ret_value);
}   /* H5S_none_select_serialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_none_select_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_none_select_deserialize(space, buf)
        H5S_t *space;           IN/OUT: Dataspace pointer to place selection into
        uint8 *buf;             IN: Buffer to retrieve serialized selection from
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Deserializes the current selection into a buffer.  (Primarily for retrieving
    from disk).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_none_select_deserialize (H5S_t *space, const uint8_t UNUSED *buf)
{
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_none_select_deserialize, FAIL);

    assert(space);

    /* Change to "none" selection */
    if((ret_value=H5S_select_none(space))<0) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");
    } /* end if */

done:
    FUNC_LEAVE (ret_value);
     
    buf = 0;
}   /* H5S_none_select_deserialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_none
 PURPOSE
    Specify that nothing is selected in the extent
 USAGE
    herr_t H5S_select_none(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to modify
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function de-selects the entire extent for a dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t H5S_select_none (H5S_t *space)
{
    herr_t ret_value=SUCCEED;  /* return value */

    FUNC_ENTER (H5S_select_none, FAIL);

    /* Check args */
    assert(space);

    /* Remove current selection first */
    if(H5S_select_release(space)<0) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL,
            "can't release hyperslab");
    } /* end if */

    /* Set selection type */
    space->select.type=H5S_SEL_NONE;

done:
    FUNC_LEAVE (ret_value);
}   /* H5S_select_none() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_none
 PURPOSE
    Specify that nothing is selected in the extent
 USAGE
    herr_t H5Sselect_none(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to modify
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function de-selects the entire extent for a dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t H5Sselect_none (hid_t spaceid)
{
    H5S_t       *space = NULL;  /* Dataspace to modify selection of */
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5Sselect_none, FAIL);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }

    /* Change to "none" selection */
    if((ret_value=H5S_select_none(space))<0) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");
    } /* end if */

done:
    FUNC_LEAVE (ret_value);
}   /* H5Sselect_none() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_select_iterate
 PURPOSE
    Iterate over a none selection, calling a user's function for each
        element. (i.e. the user's function is not called because there are
        zero elements selected)
 USAGE
    herr_t H5S_none_select_iterate(buf, type_id, space, op, operator_data)
        void *buf;      IN/OUT: Buffer containing elements to iterate over
        hid_t type_id;  IN: Datatype ID of BUF array.
        H5S_t *space;   IN: Dataspace object containing selection to iterate over
        H5D_operator_t op; IN: Function pointer to the routine to be
                                called for each element in BUF iterated over.
        void *operator_data;    IN/OUT: Pointer to any user-defined data
                                associated with the operation.
 RETURNS
    Returns success (0).
 DESCRIPTION
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_none_select_iterate(void UNUSED *buf, hid_t UNUSED type_id, H5S_t UNUSED *space, H5D_operator_t UNUSED op,
        void UNUSED *operator_data)
{
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER (H5S_none_select_iterate, FAIL);

    assert(buf);
    assert(space);
    assert(op);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    FUNC_LEAVE (ret_value);

    operator_data = 0;
}   /* H5S_hyper_select_iterate() */
