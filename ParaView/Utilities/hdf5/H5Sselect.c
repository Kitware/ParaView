/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.ued>
 *              Friday, May 29, 1998
 *
 * Purpose:     Dataspace functions.
 */

#define H5S_PACKAGE             /*suppress error about including H5Spkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5Iprivate.h"
#include "H5MMprivate.h"
#include "H5Spkg.h"
#include "H5Vprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5Sselect_mask
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

static hssize_t H5S_get_select_hyper_nblocks(H5S_t *space);
static hssize_t H5S_get_select_elem_npoints(H5S_t *space);
static herr_t H5S_get_select_hyper_blocklist(H5S_t *space, hsize_t startblock, hsize_t numblocks, hsize_t *buf);
static herr_t H5S_get_select_elem_pointlist(H5S_t *space, hsize_t startpoint, hsize_t numpoints, hsize_t *buf);
static herr_t H5S_get_select_bounds(H5S_t *space, hsize_t *start, hsize_t *end);

/* Declare external the free list for hssize_t arrays */
H5FL_ARR_EXTERN(hssize_t);


/*--------------------------------------------------------------------------
 NAME
    H5S_select_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_select_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies all the selection information (include offset) from the source
    dataspace to the destination dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_copy (H5S_t *dst, const H5S_t *src)
{
    herr_t ret_value=SUCCEED;     /* return value */

    FUNC_ENTER (H5S_select_copy, FAIL);

    /* Check args */
    assert(dst);
    assert(src);

    /* Copy regular fields */
    dst->select=src->select;

/* Need to copy order information still */

    /* Copy offset information */
    if (NULL==(dst->select.offset = H5FL_ARR_ALLOC(hssize_t,(hsize_t)src->extent.u.simple.rank,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                       "memory allocation failed");
    }
    if(src->select.offset!=NULL)
        HDmemcpy(dst->select.offset,src->select.offset,(src->extent.u.simple.rank*sizeof(hssize_t)));

    /* Perform correct type of copy based on the type of selection */
    switch (src->extent.type) {
        case H5S_SCALAR:
            /*nothing needed */
            break;

        case H5S_SIMPLE:
            /* Deep copy extra stuff */
            switch(src->select.type) {
                case H5S_SEL_NONE:
                case H5S_SEL_ALL:
                    /*nothing needed */
                    break;

                case H5S_SEL_POINTS:
                    ret_value=H5S_point_copy(dst,src);
                    break;

                case H5S_SEL_HYPERSLABS:
                    ret_value=H5S_hyper_copy(dst,src);
                    break;

                default:
                    assert("unknown selection type" && 0);
                    break;
            } /* end switch */
            break;

        case H5S_COMPLEX:
            /*void */
            break;

        default:
            assert("unknown data space type" && 0);
            break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_select_copy() */

/*--------------------------------------------------------------------------
 NAME
    H5S_select_release
 PURPOSE
    Release selection information for a dataspace
 USAGE
    herr_t H5S_select_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_release (H5S_t *space)
{
    herr_t ret_value=SUCCEED;     /* return value */

    FUNC_ENTER (H5S_select_release, FAIL);

    /* Check args */
    assert (space);

    switch(space->select.type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=H5S_point_release(space);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_release(space);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=H5S_all_release(space);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            break;

        case H5S_SEL_ERROR:
        case H5S_SEL_N:
            break;
    }

    /* Reset type of selection to "all" */
    space->select.type=H5S_SEL_ALL;

    FUNC_LEAVE (ret_value);
}   /* H5S_select_release() */


/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_npoints
 PURPOSE
    Get the number of elements in current selection
 USAGE
    hssize_t H5Sget_select_npoints(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to query
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Returns the number of elements in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5Sget_select_npoints(hid_t spaceid)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    hssize_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5Sget_select_npoints, 0);
    H5TRACE1("Hs","i",spaceid);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a data space");
    }

    ret_value = H5S_get_select_npoints(space);

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_npoints
 PURPOSE
    Get the number of elements in current selection
 USAGE
    herr_t H5Sselect_hyperslab(ds)
        H5S_t *ds;             IN: Dataspace pointer
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Returns the number of elements in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5S_get_select_npoints (const H5S_t *space)
{
    hssize_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_get_select_npoints, FAIL);

    assert(space);

    switch(space->select.type) {
    case H5S_SEL_POINTS:         /* Sequence of points selected */
        ret_value=H5S_point_npoints(space);
        break;

    case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
        ret_value=H5S_hyper_npoints(space);
        break;

    case H5S_SEL_ALL:            /* Entire extent selected */
        ret_value=H5S_all_npoints(space);
        break;

    case H5S_SEL_NONE:           /* Nothing selected */
        ret_value=0;
        break;

    case H5S_SEL_ERROR:
    case H5S_SEL_N:
        break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_get_select_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5S_sel_iter_release
 PURPOSE
    Release selection iterator information for a dataspace
 USAGE
    herr_t H5S_sel_iter_release(sel_iter)
        const H5S_t *space;             IN: Pointer to dataspace iterator is for
        H5S_sel_iter_t *sel_iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all information for a dataspace selection iterator
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_sel_iter_release (const H5S_t *space, H5S_sel_iter_t *sel_iter)
{
    herr_t ret_value=SUCCEED;     /* Return value */

    FUNC_ENTER (H5S_sel_iter_release, FAIL);

    /* Check args */
    assert (sel_iter);

    switch(space->select.type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
        case H5S_SEL_ALL:            /* Entire extent selected */
            /* no action needed */
            ret_value=SUCCEED;
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_sel_iter_release(sel_iter);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            break;

        case H5S_SEL_ERROR:
        case H5S_SEL_N:
            break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_sel_iter_release() */

/*--------------------------------------------------------------------------
 NAME
    H5Sselect_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5Sselect_void(dsid)
        hid_t dsid;             IN: Dataspace ID to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offet fits within the
    extent for the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5Sselect_valid(hid_t spaceid)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    htri_t ret_value=FAIL;     /* return value */

    FUNC_ENTER (H5Sselect_valid, 0);
    H5TRACE1("b","i",spaceid);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a data space");
    }

    ret_value = H5S_select_valid(space);

    FUNC_LEAVE (ret_value);
}   /* H5Sselect_valid() */

/*--------------------------------------------------------------------------
 NAME
    H5S_select_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5Sselect_void(space)
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offet fits within the
    extent for the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_select_valid (const H5S_t *space)
{
    htri_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_select_valid, FAIL);

    assert(space);

    switch(space->select.type) {
    case H5S_SEL_POINTS:         /* Sequence of points selected */
        ret_value=H5S_point_select_valid(space);
        break;

    case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
        ret_value=H5S_hyper_select_valid(space);
        break;

    case H5S_SEL_ALL:            /* Entire extent selected */
    case H5S_SEL_NONE:           /* Nothing selected */
        ret_value=TRUE;
        break;

    case H5S_SEL_ERROR:
    case H5S_SEL_N:
        break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_select_valid() */

/*--------------------------------------------------------------------------
 NAME
    H5S_select_serial_size
 PURPOSE
    Determine the number of bytes needed to serialize the current selection
    offset defined.
 USAGE
    hssize_t H5S_select_serial_size(space)
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize the current selection
    information for storage on disk.  This routine just hands off to the
    appropriate routine for each type of selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5S_select_serial_size (const H5S_t *space)
{
    hssize_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_select_serial_size, FAIL);

    assert(space);

    switch(space->select.type) {
    case H5S_SEL_POINTS:         /* Sequence of points selected */
        ret_value=H5S_point_select_serial_size(space);
        break;

    case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
        ret_value=H5S_hyper_select_serial_size(space);
        break;

    case H5S_SEL_ALL:            /* Entire extent selected */
    case H5S_SEL_NONE:           /* Nothing selected */
        ret_value=16;            /* replace with real function call at some point */
        break;

    case H5S_SEL_ERROR:
    case H5S_SEL_N:
        break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_select_serial_size() */

/*--------------------------------------------------------------------------
 NAME
    H5S_select_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_select_serialize(space, buf)
        H5S_t *space;           IN: Dataspace pointer of selection to serialize
        uint8 *buf;             OUT: Buffer to put serialized selection into
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Serializes the current selection into a buffer.  (Primarily for storing
    on disk).  This routine just hands off to the appropriate routine for each
    type of selection.
    The serialized information for all types of selections follows this format:
        <type of selection> = uint32
        <version #>         = uint32
        <padding, not-used> = 4 bytes
        <length of selection specific information> = uint32
        <selection specific information> = ? bytes (depends on selection type)
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_serialize (const H5S_t *space, uint8_t *buf)
{
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_select_serialize, FAIL);

    assert(space);

    switch(space->select.type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=H5S_point_select_serialize(space,buf);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_select_serialize(space,buf);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=H5S_all_select_serialize(space,buf);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            ret_value=H5S_none_select_serialize(space,buf);
            break;

        case H5S_SEL_ERROR:
        case H5S_SEL_N:
            break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_select_serialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_select_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer into a real
        selection in the dataspace.
 USAGE
    herr_t H5S_select_deserialize(space, buf)
        H5S_t *space;           IN/OUT: Dataspace pointer to place selection into
        uint8 *buf;             IN: Buffer to retrieve serialized selection from
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Deserializes the current selection into a buffer.  (Primarily for retrieving
    from disk).  This routine just hands off to the appropriate routine for each
    type of selection.  The format of the serialized information is shown in
    the H5S_select_serialize() header.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_deserialize (H5S_t *space, const uint8_t *buf)
{
    const uint8_t *tbuf;    /* Temporary pointer to the selection type */
    uint32_t sel_type;       /* Pointer to the selection type */
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_select_deserialize, FAIL);

    assert(space);

    tbuf=buf;
    UINT32DECODE(tbuf, sel_type);
    switch(sel_type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=H5S_point_select_deserialize(space,buf);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_select_deserialize(space,buf);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=H5S_all_select_deserialize(space,buf);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            ret_value=H5S_none_select_deserialize(space,buf);
            break;

        default:
            break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_select_deserialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_hyper_nblocks
 PURPOSE
    Get the number of hyperslab blocks in current hyperslab selection
 USAGE
    hssize_t H5S_get_select_hyper_nblocks(space)
        H5S_t *space;             IN: Dataspace ptr of selection to query
 RETURNS
    The number of hyperslab blocks in selection on success, negative on failure
 DESCRIPTION
    Returns the number of hyperslab blocks in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static hssize_t
H5S_get_select_hyper_nblocks(H5S_t *space)
{
    hssize_t ret_value=FAIL;        /* return value */
    unsigned u;                    /* Counter */

    FUNC_ENTER (H5S_get_select_hyper_nblocks, FAIL);

    assert(space);

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo != NULL) {

#ifdef QAK
{
H5S_hyper_dim_t *diminfo=space->select.sel_info.hslab.diminfo;
for(u=0; u<space->extent.u.simple.rank; u++)
    printf("%s: (%u) start=%d, stride=%d, count=%d, block=%d\n",FUNC,u,(int)diminfo[u].start,(int)diminfo[u].stride,(int)diminfo[u].count,(int)diminfo[u].block);
}
#endif /*QAK */
        /* Check each dimension */
        for(ret_value=1,u=0; u<space->extent.u.simple.rank; u++)
            ret_value*=space->select.sel_info.hslab.app_diminfo[u].count;
    } /* end if */
    else 
        ret_value = (hssize_t)space->select.sel_info.hslab.hyper_lst->count;

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_hyper_nblocks() */

/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_hyper_nblocks
 PURPOSE
    Get the number of hyperslab blocks in current hyperslab selection
 USAGE
    hssize_t H5Sget_select_hyper_nblocks(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to query
 RETURNS
    The number of hyperslab blocks in selection on success, negative on failure
 DESCRIPTION
    Returns the number of hyperslab blocks in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5Sget_select_hyper_nblocks(hid_t spaceid)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    hssize_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5Sget_select_hyper_nblocks, FAIL);
    H5TRACE1("Hs","i",spaceid);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if(space->select.type!=H5S_SEL_HYPERSLABS)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a hyperslab selection");

    ret_value = H5S_get_select_hyper_nblocks(space);

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_hyper_nblocks() */

/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_elem_npoints
 PURPOSE
    Get the number of points in current element selection
 USAGE
    hssize_t H5S_get_select_elem_npoints(space)
        H5S_t *space;             IN: Dataspace ptr of selection to query
 RETURNS
    The number of element points in selection on success, negative on failure
 DESCRIPTION
    Returns the number of element points in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static hssize_t
H5S_get_select_elem_npoints(H5S_t *space)
{
    hssize_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5S_get_select_elem_npoints, FAIL);

    assert(space);

    ret_value = space->select.num_elem;

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_elem_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_elem_npoints
 PURPOSE
    Get the number of points in current element selection
 USAGE
    hssize_t H5Sget_select_elem_npoints(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to query
 RETURNS
    The number of element points in selection on success, negative on failure
 DESCRIPTION
    Returns the number of element points in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5Sget_select_elem_npoints(hid_t spaceid)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    hssize_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5Sget_select_elem_npoints, FAIL);
    H5TRACE1("Hs","i",spaceid);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if(space->select.type!=H5S_SEL_POINTS)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an element selection");

    ret_value = H5S_get_select_elem_npoints(space);

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_elem_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_hyper_blocklist
 PURPOSE
    Get the list of hyperslab blocks currently selected
 USAGE
    herr_t H5S_get_select_hyper_blocklist(space, hsize_t *buf)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hsize_t startblock;     IN: Hyperslab block to start with
        hsize_t numblocks;      IN: Number of hyperslab blocks to get
        hsize_t *buf;           OUT: List of hyperslab blocks selected
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
        Puts a list of the hyperslab blocks into the user's buffer.  The blocks
    start with the 'startblock'th block in the list of blocks and put
    'numblocks' number of blocks into the user's buffer (or until the end of
    the list of blocks, whichever happen first)
        The block coordinates have the same dimensionality (rank) as the
    dataspace they are located within.  The list of blocks is formatted as
    follows: <"start" coordinate> immediately followed by <"opposite" corner
    coordinate>, followed by the next "start" and "opposite" coordinate, etc.
    until all the block information requested has been put into the user's
    buffer.
        No guarantee of any order of the blocks is implied.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_get_select_hyper_blocklist(H5S_t *space, hsize_t startblock, hsize_t numblocks, hsize_t *buf)
{
    H5S_hyper_dim_t *diminfo;               /* Alias for dataspace's diminfo information */
    hsize_t tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary hyperslab counts */
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset of element in dataspace */
    hssize_t temp_off;            /* Offset in a given dimension */
    H5S_hyper_node_t *node;     /* Hyperslab node */
    int rank;                  /* Dataspace rank */
    int i;                     /* Counter */
    int fast_dim;      /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;      /* Temporary rank holder */
    int ndims;         /* Rank of the dataspace */
    int done;          /* Whether we are done with the iteration */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER (H5S_get_select_hyper_blocklist, FAIL);

    assert(space);
    assert(buf);

    /* Get the dataspace extent rank */
    rank=space->extent.u.simple.rank;

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo != NULL) {
        /* Set some convienence values */
        ndims=space->extent.u.simple.rank;
        fast_dim=ndims-1;
        /*
         * Use the "application dimension information" to pass back to the user
         * the blocks they set, not the optimized, internal information.
         */
        diminfo=space->select.sel_info.hslab.app_diminfo;

        /* Build the tables of count sizes as well as the initial offset */
        for(i=0; i<ndims; i++) {
            tmp_count[i]=diminfo[i].count;
            offset[i]=diminfo[i].start;
        } /* end for */

        /* We're not done with the iteration */
        done=0;

        /* Go iterate over the hyperslabs */
        while(done==0 && numblocks>0) {
            /* Iterate over the blocks in the fastest dimension */
            while(tmp_count[fast_dim]>0 && numblocks>0) {

                /* Check if we should copy this block information */
                if(startblock==0) {
                    /* Copy the starting location */
                    HDmemcpy(buf,offset,sizeof(hsize_t)*ndims);
                    buf+=ndims;

                    /* Compute the ending location */
                    HDmemcpy(buf,offset,sizeof(hsize_t)*ndims);
                    for(i=0; i<ndims; i++)
                        buf[i]+=(diminfo[i].block-1);
                    buf+=ndims;

                    /* Decrement the number of blocks to retrieve */
                    numblocks--;
                } /* end if */
                else
                    startblock--;

                /* Move the offset to the next sequence to start */
                offset[fast_dim]+=diminfo[fast_dim].stride;

                /* Decrement the block count */
                tmp_count[fast_dim]--;
            } /* end while */

            /* Work on other dimensions if necessary */
            if(fast_dim>0 && numblocks>0) {
                /* Reset the block counts */
                tmp_count[fast_dim]=diminfo[fast_dim].count;

                /* Bubble up the decrement to the slower changing dimensions */
                temp_dim=fast_dim-1;
                while(temp_dim>=0 && done==0) {
                    /* Decrement the block count */
                    tmp_count[temp_dim]--;

                    /* Check if we have more blocks left */
                    if(tmp_count[temp_dim]>0)
                        break;

                    /* Check for getting out of iterator */
                    if(temp_dim==0)
                        done=1;

                    /* Reset the block count in this dimension */
                    tmp_count[temp_dim]=diminfo[temp_dim].count;
                
                    /* Wrapped a dimension, go up to next dimension */
                    temp_dim--;
                } /* end while */
            } /* end if */

            /* Re-compute offset array */
            for(i=0; i<ndims; i++) {
                temp_off=diminfo[i].start
                    +diminfo[i].stride*(diminfo[i].count-tmp_count[i]);
                offset[i]=temp_off;
            } /* end for */
        } /* end while */
    } /* end if */
    else {
        /* Get the head of the hyperslab list */
        node=space->select.sel_info.hslab.hyper_lst->head;

        /* Get to the correct first node to give back to the user */
        while(node!=NULL && startblock>0) {
            startblock--;
            node=node->next;
          } /* end while */
        
        /* Iterate through the node, copying each hyperslab's information */
        while(node!=NULL && numblocks>0) {
            HDmemcpy(buf,node->start,sizeof(hsize_t)*rank);
            buf+=rank;
            HDmemcpy(buf,node->end,sizeof(hsize_t)*rank);
            buf+=rank;
            numblocks--;
            node=node->next;
          } /* end while */
    } /* end else */

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_hyper_blocklist() */

/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_hyper_blocklist
 PURPOSE
    Get the list of hyperslab blocks currently selected
 USAGE
    herr_t H5Sget_select_hyper_blocklist(dsid, hsize_t *buf)
        hid_t dsid;             IN: Dataspace ID of selection to query
        hsize_t startblock;     IN: Hyperslab block to start with
        hsize_t numblocks;      IN: Number of hyperslab blocks to get
        hsize_t *buf;           OUT: List of hyperslab blocks selected
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
        Puts a list of the hyperslab blocks into the user's buffer.  The blocks
    start with the 'startblock'th block in the list of blocks and put
    'numblocks' number of blocks into the user's buffer (or until the end of
    the list of blocks, whichever happen first)
        The block coordinates have the same dimensionality (rank) as the
    dataspace they are located within.  The list of blocks is formatted as
    follows: <"start" coordinate> immediately followed by <"opposite" corner
    coordinate>, followed by the next "start" and "opposite" coordinate, etc.
    until all the block information requested has been put into the user's
    buffer.
        No guarantee of any order of the blocks is implied.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sget_select_hyper_blocklist(hid_t spaceid, hsize_t startblock, hsize_t numblocks, hsize_t *buf)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    herr_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5Sget_select_hyper_blocklist, FAIL);
    H5TRACE4("e","ihh*h",spaceid,startblock,numblocks,buf);

    /* Check args */
    if(buf==NULL)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pointer");
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if(space->select.type!=H5S_SEL_HYPERSLABS)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a hyperslab selection");

    ret_value = H5S_get_select_hyper_blocklist(space,startblock,numblocks,buf);

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_hyper_blocklist() */

/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_elem_pointlist
 PURPOSE
    Get the list of element points currently selected
 USAGE
    herr_t H5S_get_select_elem_pointlist(space, hsize_t *buf)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hsize_t startpoint;     IN: Element point to start with
        hsize_t numpoints;      IN: Number of element points to get
        hsize_t *buf;           OUT: List of element points selected
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
        Puts a list of the element points into the user's buffer.  The points
    start with the 'startpoint'th block in the list of points and put
    'numpoints' number of points into the user's buffer (or until the end of
    the list of points, whichever happen first)
        The point coordinates have the same dimensionality (rank) as the
    dataspace they are located within.  The list of points is formatted as
    follows: <coordinate> followed by the next coordinate, etc. until all the
    point information in the selection have been put into the user's buffer.
        The points are returned in the order they will be interated through
    when a selection is read/written from/to disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_get_select_elem_pointlist(H5S_t *space, hsize_t startpoint, hsize_t numpoints, hsize_t *buf)
{
    H5S_pnt_node_t *node;       /* Point node */
    int rank;                  /* Dataspace rank */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER (H5S_get_select_elem_pointlist, FAIL);

    assert(space);
    assert(buf);

    /* Get the dataspace extent rank */
    rank=space->extent.u.simple.rank;

    /* Get the head of the point list */
    node=space->select.sel_info.pnt_lst->head;

    /* Iterate to the first point to return */
    while(node!=NULL && startpoint>0) {
        startpoint--;
        node=node->next;
      } /* end while */

    /* Iterate through the node, copying each hyperslab's information */
    while(node!=NULL && numpoints>0) {
        HDmemcpy(buf,node->pnt,sizeof(hsize_t)*rank);
        buf+=rank;
        numpoints--;
        node=node->next;
      } /* end while */

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_elem_pointlist() */

/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_elem_pointlist
 PURPOSE
    Get the list of element points currently selected
 USAGE
    herr_t H5Sget_select_elem_pointlist(dsid, hsize_t *buf)
        hid_t dsid;             IN: Dataspace ID of selection to query
        hsize_t startpoint;     IN: Element point to start with
        hsize_t numpoints;      IN: Number of element points to get
        hsize_t *buf;           OUT: List of element points selected
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
        Puts a list of the element points into the user's buffer.  The points
    start with the 'startpoint'th block in the list of points and put
    'numpoints' number of points into the user's buffer (or until the end of
    the list of points, whichever happen first)
        The point coordinates have the same dimensionality (rank) as the
    dataspace they are located within.  The list of points is formatted as
    follows: <coordinate> followed by the next coordinate, etc. until all the
    point information in the selection have been put into the user's buffer.
        The points are returned in the order they will be interated through
    when a selection is read/written from/to disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sget_select_elem_pointlist(hid_t spaceid, hsize_t startpoint, hsize_t numpoints, hsize_t *buf)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    herr_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5Sget_select_elem_pointlist, FAIL);
    H5TRACE4("e","ihh*h",spaceid,startpoint,numpoints,buf);

    /* Check args */
    if(buf==NULL)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pointer");
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if(space->select.type!=H5S_SEL_POINTS)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a point selection");

    ret_value = H5S_get_select_elem_pointlist(space,startpoint,numpoints,buf);

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_elem_pointlist() */

/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_get_select_bounds(space, hsize_t *start, hsize_t *end)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hsize_t *start;         OUT: Starting coordinate of bounding box
        hsize_t *end;           OUT: Opposite coordinate of bounding box
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
        The bounding box calculations _does_ include the current offset of the
    selection within the dataspace extent.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_get_select_bounds(H5S_t *space, hsize_t *start, hsize_t *end)
{
    int rank;                  /* Dataspace rank */
    int i;                     /* index variable */
    herr_t ret_value=FAIL;   /* return value */

    FUNC_ENTER (H5S_get_select_bounds, FAIL);

    assert(space);
    assert(start);
    assert(end);

    /* Set all the start and end arrays up */
    rank=space->extent.u.simple.rank;
    for(i=0; i<rank; i++) {
        start[i]=UINT_MAX;
        end[i]=0;
    } /* end for */

    switch(space->select.type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=H5S_point_bounds(space,start,end);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_bounds(space,start,end);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=H5S_all_bounds(space,start,end);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
        case H5S_SEL_ERROR:
        case H5S_SEL_N:
            break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_get_select_bounds() */

/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_get_select_bounds(space, start, end)
        hid_t dsid;             IN: Dataspace ID of selection to query
        hsize_t *start;         OUT: Starting coordinate of bounding box
        hsize_t *end;           OUT: Opposite coordinate of bounding box
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
        The bounding box calculations _does_ include the current offset of the
    selection within the dataspace extent.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sget_select_bounds(hid_t spaceid, hsize_t *start, hsize_t *end)
{
    H5S_t       *space = NULL;      /* Dataspace to modify selection of */
    herr_t ret_value=FAIL;        /* return value */

    FUNC_ENTER (H5Sget_select_bounds, FAIL);
    H5TRACE3("e","i*h*h",spaceid,start,end);

    /* Check args */
    if(start==NULL || end==NULL)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pointer");
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }

    ret_value = H5S_get_select_bounds(space,start,end);

    FUNC_LEAVE (ret_value);
}   /* H5Sget_select_bounds() */

/*--------------------------------------------------------------------------
 NAME
    H5S_select_contiguous
 PURPOSE
    Check if the selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_select_contiguous(space)
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
H5S_select_contiguous(const H5S_t *space)
{
    htri_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_select_contiguous, FAIL);

    assert(space);

    switch(space->select.type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=H5S_point_select_contiguous(space);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_select_contiguous(space);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=TRUE;
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            ret_value=FALSE;
            break;

        case H5S_SEL_ERROR:
        case H5S_SEL_N:
            break;
    }

    FUNC_LEAVE (ret_value);
}   /* H5S_select_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_iterate
 PURPOSE
    Iterate over the selected elements in a memory buffer.
 USAGE
    herr_t H5S_select_iterate(buf, type_id, space, operator, operator_data)
        void *buf;      IN/OUT: Buffer containing elements to iterate over
        hid_t type_id;  IN: Datatype ID of BUF array.
        H5S_t *space;   IN: Dataspace object containing selection to iterate over
        H5D_operator_t op; IN: Function pointer to the routine to be
                                called for each element in BUF iterated over.
        void *operator_data;    IN/OUT: Pointer to any user-defined data
                                associated with the operation.
 RETURNS
    Returns the return value of the last operator if it was non-zero, or zero
    if all elements were processed. Otherwise returns a negative value.
 DESCRIPTION
    Iterates over the selected elements in a memory buffer, calling the user's
    callback function for each element.  The selection in the dataspace is
    modified so that any elements already iterated over are removed from the
    selection if the iteration is interrupted (by the H5D_operator_t function
    returning non-zero) in the "middle" of the iteration and may be re-started
    by the user where it left off.

    NOTE: Until "subtracting" elements from a selection is implemented,
        the selection is not modified.
--------------------------------------------------------------------------*/
herr_t
H5S_select_iterate(void *buf, hid_t type_id, H5S_t *space, H5D_operator_t op,
        void *operator_data)
{
    herr_t ret_value=FAIL;

    FUNC_ENTER(H5S_select_iterate, FAIL);

    /* Check args */
    assert(buf);
    assert(space);
    assert(op);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    switch(space->select.type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=H5S_point_select_iterate(buf,type_id,space,op,operator_data);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=H5S_hyper_select_iterate(buf,type_id,space,op,operator_data);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=H5S_all_select_iterate(buf,type_id,space,op,operator_data);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            ret_value=H5S_none_select_iterate(buf,type_id,space,op,operator_data);
            break;

        case H5S_SEL_ERROR:
        case H5S_SEL_N:
            break;
    }

    FUNC_LEAVE(ret_value);
}   /* end H5S_select_iterate() */

