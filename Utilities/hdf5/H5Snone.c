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
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, November 10, 1998
 *
 * Purpose:	"None" selection data space I/O functions.
 */

#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK      H5Snone_mask

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Iprivate.h"
#include "H5Spkg.h"
#include "H5Vprivate.h"
#include "H5Dprivate.h"

/* Interface initialization */
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

/* Static function prototypes */
static herr_t H5S_none_iter_coords(const H5S_sel_iter_t *iter, hssize_t *coords);
static herr_t H5S_none_iter_block(const H5S_sel_iter_t *iter, hssize_t *start, hssize_t *end);
static hsize_t H5S_none_iter_nelmts(const H5S_sel_iter_t *iter);
static htri_t H5S_none_iter_has_next_block(const H5S_sel_iter_t *iter);
static herr_t H5S_none_iter_next(H5S_sel_iter_t *sel_iter, size_t nelem);
static herr_t H5S_none_iter_next_block(H5S_sel_iter_t *sel_iter);
static herr_t H5S_none_iter_release(H5S_sel_iter_t *sel_iter);


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_init
 *
 * Purpose:	Initializes iteration information for "none" selection.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_none_iter_init (H5S_sel_iter_t *iter, const H5S_t UNUSED *space, size_t UNUSED elmt_size)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_iter_init, FAIL);

    /* Check args */
    assert (space && H5S_SEL_NONE==space->select.type);
    assert (iter);

    /* Initialize methods for selection iterator */
    iter->iter_coords=H5S_none_iter_coords;
    iter->iter_block=H5S_none_iter_block;
    iter->iter_nelmts=H5S_none_iter_nelmts;
    iter->iter_has_next_block=H5S_none_iter_has_next_block;
    iter->iter_next=H5S_none_iter_next;
    iter->iter_next_block=H5S_none_iter_next_block;
    iter->iter_release=H5S_none_iter_release;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_iter_init() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_coords
 *
 * Purpose:	Retrieve the current coordinates of iterator for current
 *              selection
 *
 * Return:	non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, April 22, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_none_iter_coords (const H5S_sel_iter_t UNUSED *iter, hssize_t UNUSED *coords)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_coords);

    /* Check args */
    assert (iter);
    assert (coords);

    FUNC_LEAVE_NOAPI(FAIL);
}   /* H5S_none_iter_coords() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_block
 *
 * Purpose:	Retrieve the current block of iterator for current
 *              selection
 *
 * Return:	non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, June 2, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_none_iter_block (const H5S_sel_iter_t UNUSED *iter, hssize_t UNUSED *start, hssize_t UNUSED *end)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_block);

    /* Check args */
    assert (iter);
    assert (start);
    assert (end);

    FUNC_LEAVE_NOAPI(FAIL);
}   /* H5S_none_iter_block() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_nelmts
 *
 * Purpose:	Return number of elements left to process in iterator
 *
 * Return:	non-negative number of elements on success, zero on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_none_iter_nelmts (const H5S_sel_iter_t UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_nelmts);

    /* Check args */
    assert (iter);

    FUNC_LEAVE_NOAPI(0);
}   /* H5S_none_iter_nelmts() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_has_next_block
 PURPOSE
    Check if there is another block left in the current iterator
 USAGE
    htri_t H5S_none_iter_has_next_block(iter)
        const H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative (TRUE/FALSE) on success/Negative on failure
 DESCRIPTION
    Check if there is another block available in the selection iterator.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_none_iter_has_next_block(const H5S_sel_iter_t UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_has_next_block);

    /* Check args */
    assert (iter);

    FUNC_LEAVE_NOAPI(FAIL);
}   /* H5S_none_iter_has_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_next
 PURPOSE
    Increment selection iterator
 USAGE
    herr_t H5S_none_iter_next(iter, nelem)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
        size_t nelem;               IN: Number of elements to advance by
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Advance selection iterator to the NELEM'th next element in the selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_none_iter_next(H5S_sel_iter_t UNUSED *iter, size_t UNUSED nelem)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_next);

    /* Check args */
    assert (iter);
    assert (nelem>0);

    FUNC_LEAVE_NOAPI(SUCCEED);
}   /* H5S_none_iter_next() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_next_block
 PURPOSE
    Increment selection iterator to next block
 USAGE
    herr_t H5S_none_iter_next(iter)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Advance selection iterator to the next block in the selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_none_iter_next_block(H5S_sel_iter_t UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_next);

    /* Check args */
    assert (iter);

    FUNC_LEAVE_NOAPI(FAIL);
}   /* H5S_none_iter_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_release
 PURPOSE
    Release "none" selection iterator information for a dataspace
 USAGE
    herr_t H5S_none_iter_release(iter)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all information for a dataspace "none" selection iterator
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_none_iter_release (H5S_sel_iter_t UNUSED * iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_none_iter_release);

    /* Check args */
    assert (iter);

    FUNC_LEAVE_NOAPI(SUCCEED);
}   /* H5S_none_iter_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_release
 PURPOSE
    Release none selection information for a dataspace
 USAGE
    herr_t H5S_none_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases "none" selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_none_release (H5S_t UNUSED * space)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_release, FAIL);

    /* Check args */
    assert (space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_npoints
 PURPOSE
    Compute number of elements in current selection
 USAGE
    hsize_t H5S_none_npoints(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Compute number of elements in current selection.  For "none" selections,
    this is always 0.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hsize_t
H5S_none_npoints (const H5S_t UNUSED *space)
{
    hsize_t ret_value=0;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_npoints, 0);

    /* Check args */
    assert (space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_npoints() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_none_is_valid(space);
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offet fits within the
    extent for the dataspace.  Offset is irrelevant for this type of selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_none_is_valid (const H5S_t UNUSED *space)
{
    htri_t ret_value=TRUE;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_is_valid, FAIL);

    assert(space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_none_is_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_serial_size
 PURPOSE
    Determine the number of bytes needed to store the serialized "none"
        selection information.
 USAGE
    hssize_t H5S_none_serial_size(space)
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize an "none"
    selection for storage on disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5S_none_serial_size (const H5S_t UNUSED *space)
{
    hssize_t ret_value;    /* return value */

    FUNC_ENTER_NOAPI(H5S_none_serial_size, FAIL);

    assert(space);

    /* Basic number of bytes required to serialize point selection:
     *  <type (4 bytes)> + <version (4 bytes)> + <padding (4 bytes)> + 
     *      <length (4 bytes)> = 16 bytes
     */
    ret_value=16;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_none_serial_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_none_serialize(space, buf)
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
H5S_none_serialize (const H5S_t *space, uint8_t *buf)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_serialize, FAIL);

    assert(space);

    /* Store the preamble information */
    UINT32ENCODE(buf, (uint32_t)space->select.type);  /* Store the type of selection */
    UINT32ENCODE(buf, (uint32_t)1);  /* Store the version number */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the un-used padding */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the additional information length */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_serialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_none_deserialize(space, buf)
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
H5S_none_deserialize (H5S_t *space, const uint8_t UNUSED *buf)
{
    herr_t ret_value;  /* return value */

    FUNC_ENTER_NOAPI(H5S_none_deserialize, FAIL);

    assert(space);

    /* Change to "none" selection */
    if((ret_value=H5S_select_none(space))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_deserialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_none_bounds(space, start, end)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hssize_t *start;         OUT: Starting coordinate of bounding box
        hssize_t *end;           OUT: Opposite coordinate of bounding box
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Retrieves the bounding box containing the current selection and places
    it into the user's buffers.  The start and end buffers must be large
    enough to hold the dataspace rank number of coordinates.  The bounding box
    exactly contains the selection, ie. if a 2-D element selection is currently
    defined with the following points: (4,5), (6,8) (10,7), the bounding box
    with be (4, 5), (10, 8).  Calling this function on a "none" selection
    returns fail.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_none_bounds(const H5S_t UNUSED *space, hssize_t UNUSED *start, hssize_t UNUSED *end)
{
    herr_t ret_value=FAIL;      /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_bounds, FAIL);

    assert(space);
    assert(start);
    assert(end);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5Sget_none_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_contiguous
 PURPOSE
    Check if a "none" selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_all_is_contiguous(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is contiguous.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_none_is_contiguous(const H5S_t UNUSED *space)
{
    htri_t ret_value=FALSE;      /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_is_contiguous, FAIL);

    assert(space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_is_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_single
 PURPOSE
    Check if a "none" selection is a single block within the dataspace extent.
 USAGE
    htri_t H5S_none_is_single(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is a single block.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_none_is_single(const H5S_t UNUSED *space)
{
    htri_t ret_value=FALSE;      /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_is_single, FAIL);

    assert(space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_is_single() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_regular
 PURPOSE
    Check if a "none" selection is "regular"
 USAGE
    htri_t H5S_none_is_regular(space)
        const H5S_t *space;     IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in a dataspace is the a regular
    pattern.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_none_is_regular(const H5S_t UNUSED *space)
{
    htri_t ret_value=TRUE;      /* Return value */

    FUNC_ENTER_NOAPI(H5S_none_is_regular, FAIL);

    /* Check args */
    assert(space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_none_is_regular() */


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
herr_t H5S_select_none (H5S_t *space)
{
    herr_t ret_value=SUCCEED;  /* return value */

    FUNC_ENTER_NOAPI(H5S_select_none, FAIL);

    /* Check args */
    assert(space);

    /* Remove current selection first */
    if((*space->select.release)(space)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't release hyperslab");

    /* Set selection type */
    space->select.type=H5S_SEL_NONE;

    /* Set selection methods */
    space->select.get_seq_list=H5S_none_get_seq_list;
    space->select.get_npoints=H5S_none_npoints;
    space->select.release=H5S_none_release;
    space->select.is_valid=H5S_none_is_valid;
    space->select.serial_size=H5S_none_serial_size;
    space->select.serialize=H5S_none_serialize;
    space->select.bounds=H5S_none_bounds;
    space->select.is_contiguous=H5S_none_is_contiguous;
    space->select.is_single=H5S_none_is_single;
    space->select.is_regular=H5S_none_is_regular;
    space->select.iter_init=H5S_none_iter_init;

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
    H5S_t	*space = NULL;  /* Dataspace to modify selection of */
    herr_t ret_value;  /* return value */

    FUNC_ENTER_API(H5Sselect_none, FAIL);

    /* Check args */
    if (NULL == (space=H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    /* Change to "none" selection */
    if((ret_value=H5S_select_none(space))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Sselect_none() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_get_seq_list
 PURPOSE
    Create a list of offsets & lengths for a selection
 USAGE
    herr_t H5S_all_get_seq_list(space,flags,iter,elem_size,maxseq,maxbytes,nseq,nbytes,off,len)
        H5S_t *space;           IN: Dataspace containing selection to use.
        unsigned flags;         IN: Flags for extra information about operation
        H5S_sel_iter_t *iter;   IN/OUT: Selection iterator describing last
                                    position of interest in selection.
        size_t elem_size;       IN: Size of an element
        size_t maxseq;          IN: Maximum number of sequences to generate
        size_t maxbytes;        IN: Maximum number of bytes to include in the
                                    generated sequences
        size_t *nseq;           OUT: Actual number of sequences generated
        size_t *nbytes;         OUT: Actual number of bytes in sequences generated
        hsize_t *off;           OUT: Array of offsets
        size_t *len;            OUT: Array of lengths
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Use the selection in the dataspace to generate a list of byte offsets and
    lengths for the region(s) selected.  Start/Restart from the position in the
    ITER parameter.  The number of sequences generated is limited by the MAXSEQ
    parameter and the number of sequences actually generated is stored in the
    NSEQ parameter.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_none_get_seq_list(const H5S_t UNUSED *space, unsigned UNUSED flags, H5S_sel_iter_t UNUSED *iter,
    size_t UNUSED elem_size, size_t UNUSED maxseq, size_t UNUSED maxbytes, size_t *nseq, size_t *nbytes,
    hsize_t UNUSED *off, size_t UNUSED *len)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI (H5S_none_get_seq_list, FAIL);

    /* Check args */
    assert(space);
    assert(iter);
    assert(elem_size>0);
    assert(maxseq>0);
    assert(maxbytes>0);
    assert(nseq);
    assert(nbytes);
    assert(off);
    assert(len);

    /* "none" selection don't generate sequences of bytes */
    *nseq=0;

    /* The don't use any bytes, either */
    *nbytes=0;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_all_get_seq_list() */
