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
 * Programmer:  rky 980813
 *
 * Purpose:	Functions to read/write directly between app buffer and file.
 *
 * 		Beware of the ifdef'ed print statements.
 *		I didn't make them portable.
 */

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */

#include "H5private.h"          /* Internal types, etc. */
#include "H5Eprivate.h"         /* Error reporting */
#include "H5Fpkg.h"             /* Ugly, but necessary for the MPIO I/O accesses */
#include "H5FDmpio.h"		/* MPIO file driver			*/
#include "H5FDprivate.h"        /* Necessary for the H5FD_write & H5FD_read prototypes.. */
#include "H5Iprivate.h"		/* Object IDs */
#include "H5Pprivate.h"		/* Property Lists */
#include "H5Spkg.h"             /* Dataspaces */

#ifndef H5_HAVE_PARALLEL
/* 
 * The H5S_mpio_xxxx functions are for parallel I/O only and are
 * valid only when H5_HAVE_PARALLEL is #defined.  This empty #ifndef
 * body is used to allow this source file be included in the serial
 * distribution.
 * Some compilers/linkers may complain about "empty" object file.
 * If that happens, uncomment the following statement to pacify
 * them.
 */
/* const hbool_t H5S_mpio_avail = FALSE; */
#else /* H5_HAVE_PARALLEL */
/* Interface initialization */
#define PABLO_MASK      H5Sall_mask
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

static herr_t
H5S_mpio_all_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );
static herr_t
H5S_mpio_none_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );
static herr_t
H5S_mpio_hyper_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );
static herr_t
H5S_mpio_space_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );
static herr_t
H5S_mpio_spaces_xfer(H5F_t *f, const H5O_layout_t *layout, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     hid_t dxpl_id, void *buf/*out*/, hbool_t do_write);


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_all_type
 *
 * Purpose:	Translate an HDF5 "all" selection into an MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 *      Quincey Koziol, June 18, 2002
 *      Added 'extra_offset' parameter
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_all_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    hsize_t	total_bytes;
    unsigned		u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_mpio_all_type);

    /* Check args */
    assert (space);

    /* Just treat the entire extent as a block of bytes */
    total_bytes = (hsize_t)elmt_size;
    for (u=0; u<space->extent.u.simple.rank; ++u)
        total_bytes *= space->extent.u.simple.size[u];

    /* fill in the return values */
    *new_type = MPI_BYTE;
    H5_ASSIGN_OVERFLOW(*count, total_bytes, hsize_t, size_t);
    *extra_offset = 0;
    *is_derived_type = 0;

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* H5S_mpio_all_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_none_type
 *
 * Purpose:	Translate an HDF5 "none" selection into an MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	Quincey Koziol, October 29, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_none_type( const H5S_t UNUSED *space, size_t UNUSED elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_mpio_none_type);

    /* fill in the return values */
    *new_type = MPI_BYTE;
    *count = 0;
    *extra_offset = 0;
    *is_derived_type = 0;

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* H5S_mpio_none_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_hyper_type
 *
 * Purpose:	Translate an HDF5 hyperslab selection into an MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 * Modifications:  ppw 990401
 *		rky, ppw 2000-09-26 Freed old type after creating struct type.
 *		rky 2000-10-05 Changed displacements to be MPI_Aint.
 *		rky 2000-10-06 Added code for cases of empty hyperslab.
 *		akc, rky 2000-11-16 Replaced hard coded dimension size with
 *		    H5S_MAX_RANK.
 *
 *      Quincey Koziol, June 18, 2002
 *      Added 'extra_offset' parameter.  Also accomodate
 *      selection offset in MPI type built.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_hyper_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    H5S_sel_iter_t sel_iter;    /* Selection iteration info */
    hbool_t sel_iter_init=0;    /* Selection iteration info has been initialized */

    struct dim {	/* less hassle than malloc/free & ilk */
        hssize_t start;
        hsize_t strid;
        hsize_t block;
        hsize_t xtent;
        hsize_t count;
    } d[H5S_MAX_RANK];

    int			i;
    int			offset[H5S_MAX_RANK];
    int			max_xtent[H5S_MAX_RANK];
    H5S_hyper_dim_t	*diminfo;		/* [rank] */
    int		rank;
    int			block_length[2];
    MPI_Datatype	inner_type, outer_type, old_type[2];
    MPI_Aint            extent_len, displacement[2];
    int                 mpi_code;               /* MPI return code */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_hyper_type);

    /* Check args */
    assert (space);
    assert(sizeof(MPI_Aint) >= sizeof(elmt_size));
    if (0==elmt_size)
        goto empty;

    /* Initialize selection iterator */
    if (H5S_select_iter_init(&sel_iter, space, elmt_size)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    sel_iter_init=1;	/* Selection iteration info has been initialized */

    /* Abbreviate args */
    diminfo=sel_iter.u.hyp.diminfo;
    assert (diminfo);

    /* make a local copy of the dimension info so we can operate with them */

    /* Check if this is a "flattened" regular hyperslab selection */
    if(sel_iter.u.hyp.iter_rank!=0 && sel_iter.u.hyp.iter_rank<space->extent.u.simple.rank) {
        /* Flattened selection */
        rank=sel_iter.u.hyp.iter_rank;
        assert (rank >= 0 && rank<=H5S_MAX_RANK);	/* within array bounds */
        if (0==rank)
            goto empty;

#ifdef H5Smpi_DEBUG
            HDfprintf(stderr, "%s: Flattened selection\n",FUNC);
#endif
        for ( i=0; i<rank; ++i) {
            d[i].start = diminfo[i].start+sel_iter.u.hyp.sel_off[i];
            d[i].strid = diminfo[i].stride;
            d[i].block = diminfo[i].block;
            d[i].count = diminfo[i].count;
            d[i].xtent = sel_iter.u.hyp.size[i];
#ifdef H5Smpi_DEBUG
            HDfprintf(stderr, "%s: start=%Hd  stride=%Hu  count=%Hu  block=%Hu  xtent=%Hu",
                FUNC, d[i].start, d[i].strid, d[i].count, d[i].block, d[i].xtent );
            if (i==0)
                HDfprintf(stderr, "  rank=%d\n", rank );
            else
                HDfprintf(stderr, "\n" );
#endif
            if (0==d[i].block)
                goto empty;
            if (0==d[i].count)
                goto empty;
            if (0==d[i].xtent)
                goto empty;
        }
    } /* end if */
    else {
        /* Non-flattened selection */
        rank = space->extent.u.simple.rank;
        assert (rank >= 0 && rank<=H5S_MAX_RANK);	/* within array bounds */
        if (0==rank)
            goto empty;

#ifdef H5Smpi_DEBUG
            HDfprintf(stderr, "%s: Non-flattened selection\n",FUNC);
#endif
        for ( i=0; i<rank; ++i) {
            d[i].start = diminfo[i].start+space->select.offset[i];
            d[i].strid = diminfo[i].stride;
            d[i].block = diminfo[i].block;
            d[i].count = diminfo[i].count;
            d[i].xtent = space->extent.u.simple.size[i];
#ifdef H5Smpi_DEBUG
            HDfprintf(stderr, "%s: start=%Hd  stride=%Hu  count=%Hu  block=%Hu  xtent=%Hu",
                FUNC, d[i].start, d[i].strid, d[i].count, d[i].block, d[i].xtent );
            if (i==0)
                HDfprintf(stderr, "  rank=%d\n", rank );
            else
                HDfprintf(stderr, "\n" );
#endif
            if (0==d[i].block)
                goto empty;
            if (0==d[i].count)
                goto empty;
            if (0==d[i].xtent)
                goto empty;
        }
    } /* end else */
    
/**********************************************************************
    Compute array "offset[rank]" which gives the offsets for a multi-
    dimensional array with dimensions "d[i].xtent" (i=0,1,...,rank-1). 
**********************************************************************/
    offset[rank-1] = 1;
    max_xtent[rank-1] = d[rank-1].xtent;
#ifdef H5Smpi_DEBUG
    i=rank-1;
    HDfprintf(stderr, " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
#endif
    for (i=rank-2; i>=0; --i) {
        offset[i] = offset[i+1]*d[i+1].xtent;
        max_xtent[i] = max_xtent[i+1]*d[i].xtent;
#ifdef H5Smpi_DEBUG
        HDfprintf(stderr, " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
#endif
    }

    /*  Create a type covering the selected hyperslab.
     *  Multidimensional dataspaces are stored in row-major order.
     *  The type is built from the inside out, going from the
     *  fastest-changing (i.e., inner) dimension * to the slowest (outer). */

/*******************************************************
*  Construct contig type for inner contig dims:
*******************************************************/
#ifdef H5Smpi_DEBUG
    HDfprintf(stderr, "%s: Making contig type %d MPI_BYTEs\n", FUNC, elmt_size );
    for (i=rank-1; i>=0; --i)
        HDfprintf(stderr, "d[%d].xtent=%Hu \n", i, d[i].xtent);
#endif
    if (MPI_SUCCESS != (mpi_code= MPI_Type_contiguous( (int)elmt_size, MPI_BYTE, &inner_type )))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code);

/*******************************************************
*  Construct the type by walking the hyperslab dims
*  from the inside out:
*******************************************************/
    for ( i=rank-1; i>=0; --i) {
#ifdef H5Smpi_DEBUG
        HDfprintf(stderr, "%s: Dimension i=%d \n"
            "count=%Hu block=%Hu stride=%Hu\n",
            FUNC, i, d[i].count, d[i].block, d[i].strid );
#endif

#ifdef H5Smpi_DEBUG
        HDfprintf(stderr, "%s: i=%d  Making vector-type \n", FUNC, i);
#endif
       /****************************************
       *  Build vector in current dimension:
       ****************************************/
        mpi_code =MPI_Type_vector((int)(d[i].count),        /* count */
				  (int)(d[i].block),        /* blocklength */
				  (int)(d[i].strid),   	    /* stride */
				  inner_type,	            /* old type */
				  &outer_type );            /* new type */

        MPI_Type_free( &inner_type );
        if (mpi_code!=MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "couldn't create MPI vector type", mpi_code);

        displacement[1] = (MPI_Aint)elmt_size * max_xtent[i];
        if(MPI_SUCCESS != (mpi_code = MPI_Type_extent(outer_type, &extent_len)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_extent failed", mpi_code);

       /*************************************************
       *  Restructure this datatype ("outer_type")
       *  so that it still starts at 0, but its extent
       *  is the full extent in this dimension.
       *************************************************/
        if ((int)extent_len < displacement[1]) {

#ifdef H5Smpi_DEBUG
            HDfprintf(stderr, "%s: i=%d Extending struct type\n"
                "***displacements: 0, %d\n", FUNC, i, displacement[1]);
#endif

#ifdef H5_HAVE_MPI2  /*  have MPI-2  (this function is not included in MPICH) */
            mpi_code = MPI_Type_create_resized
                                  ( outer_type,        /* old type  */
                                    0,                 /* blocklengths */
                                    displacement[1],   /* displacements */
                                    &inner_type);      /* new type */
#else        /*  do not have MPI-2  */
            block_length[0] = 1;
            block_length[1] = 1;
  
            displacement[0] = 0;
  
            old_type[0] = outer_type;
            old_type[1] = MPI_UB;
            mpi_code = MPI_Type_struct ( 2,               /* count */
                                    block_length,    /* blocklengths */
                                    displacement,    /* displacements */
                                    old_type,        /* old types */
                                    &inner_type);    /* new type */
#endif
  
            MPI_Type_free (&outer_type);
    	    if (mpi_code!=MPI_SUCCESS)
                HMPI_GOTO_ERROR(FAIL, "couldn't resize MPI vector type", mpi_code);
        }
        else {
            inner_type = outer_type;
        }
    } /* end for */
/***************************
*  End of loop, walking 
*  thru dimensions.
***************************/


    /* At this point inner_type is actually the outermost type, even for 0-trip loop */

/***************************************************************
*  Final task: create a struct which is a "clone" of the
*  current struct, but displaced according to the d[i].start
*  values given in the hyperslab description:
***************************************************************/
    displacement[0] = 0;
    for (i=rank-1; i>=0; i--)
        displacement[0] += d[i].start * offset[i];

    if (displacement[0] > 0) {
        displacement[0] *= elmt_size;
        block_length[0] = 1;
        old_type[0] = inner_type;

#ifdef H5Smpi_DEBUG
        HDfprintf(stderr, "%s:  Making final struct\n***count=1:\n", FUNC);
        HDfprintf(stderr, "\tblocklength[0]=%d; displacement[0]=%d\n",
                     block_length[0], displacement[0]);
#endif

  
        if (MPI_SUCCESS != (mpi_code= MPI_Type_struct( 1,                  /* count */
                     block_length,       /* blocklengths */
                     displacement,       /* displacements */
                     old_type,	         /* old type */
                     new_type ))         /* new type */
                )
            HMPI_GOTO_ERROR(FAIL, "couldn't create MPI struct type", mpi_code);

        if (MPI_SUCCESS != (mpi_code= MPI_Type_free (&old_type[0])))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_free failed", mpi_code);
    }
    else {
        *new_type = inner_type;
    }

    if (MPI_SUCCESS != (mpi_code= MPI_Type_commit( new_type )))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code);

    /* fill in the remaining return values */
    *count = 1;			/* only have to move one of these suckers! */
    *extra_offset = 0;
    *is_derived_type = 1;
    HGOTO_DONE(SUCCEED);

empty:
    /* special case: empty hyperslab */
    *new_type = MPI_BYTE;
    *count = 0;
    *extra_offset = 0;
    *is_derived_type = 0;

done:
    /* Release selection iterator */
    if(sel_iter_init) {
        if (H5S_select_iter_release(&sel_iter)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

#ifdef H5Smpi_DEBUG
    HDfprintf(stderr, "Leave %s, count=%Hu  is_derived_type=%d\n",
		FUNC, *count, *is_derived_type );
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_space_type
 *
 * Purpose:	Translate an HDF5 dataspace selection into an MPI type.
 *		Currently handle only hyperslab and "all" selections.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 *      Quincey Koziol, June 18, 2002
 *      Added 'extra_offset' parameter
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_space_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    herr_t	ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_space_type);

    /* Check args */
    assert (space);

    /* Creat MPI type based on the kind of selection */
    switch (space->extent.type) {
        case H5S_SCALAR:
        case H5S_SIMPLE:
            switch(space->select.type) {
                case H5S_SEL_NONE:
                    if ( H5S_mpio_none_type( space, elmt_size,
                        /* out: */ new_type, count, extra_offset, is_derived_type ) <0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    break;

                case H5S_SEL_ALL:
                    if ( H5S_mpio_all_type( space, elmt_size,
                        /* out: */ new_type, count, extra_offset, is_derived_type ) <0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    break;

                case H5S_SEL_POINTS:
                    /* not yet implemented */
                    ret_value = FAIL;
                    break;

                case H5S_SEL_HYPERSLABS:
                    if(H5S_mpio_hyper_type( space, elmt_size,
                            /* out: */ new_type, count, extra_offset, is_derived_type )<0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    break;

                default:
                    assert("unknown selection type" && 0);
                    break;
            } /* end switch */
            break;

        case H5S_COMPLEX:
            /* not yet implemented */
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "complex data spaces are not supported yet");
            break;

        default:
            assert("unknown data space type" && 0);
            break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_spaces_xfer
 *
 * Purpose:	Use MPI-IO to transfer data efficiently
 *		directly between app buffer and file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	rky 980813
 *
 * Notes:
 *      For collective data transfer only since this would eventually call
 *      H5FD_mpio_setup to do setup to eveually call MPI_File_set_view in
 *      H5FD_mpio_read or H5FD_mpio_write.  MPI_File_set_view is a collective
 *      call.  Letting independent data transfer use this route would result in
 *      hanging.
 *
 *      The preconditions for calling this routine are located in the
 *      H5S_mpio_opt_possible() routine, which determines whether this routine
 *      can be called for a given dataset transfer.
 *
 * Modifications:
 *	rky 980918
 *	Added must_convert parameter to let caller know we can't optimize
 *	the xfer.
 *
 *	Albert Cheng, 001123
 *	Include the MPI_type freeing as part of cleanup code.
 *
 *      QAK - 2002/04/02
 *      Removed the must_convert parameter and move preconditions to
 *      H5S_mpio_opt_possible() routine
 *
 *      QAK - 2002/06/17
 *      Removed 'disp' parameter from H5FD_mpio_setup routine and use the
 *      address of the dataset in MPI_File_set_view() calls, as necessary.
 *
 *      QAK - 2002/06/18
 *      Removed 'dc_plist' parameter, since it was not used.  Also, switch to
 *      getting the 'extra_offset' setting for each selection.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_spaces_xfer(H5F_t *f, const H5O_layout_t *layout, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
		     hid_t dxpl_id, void *_buf /*out*/,
		     hbool_t do_write )
{
    haddr_t	 addr;                  /* Address of dataset (or selection) within file */
    size_t	 mpi_buf_count, mpi_file_count;       /* Number of "objects" to transfer */
    hsize_t	 mpi_buf_offset, mpi_file_offset;       /* Offset within dataset where selection (ie. MPI type) begins */
    MPI_Datatype mpi_buf_type, mpi_file_type;   /* MPI types for buffer (memory) and file */
    hbool_t	 mbt_is_derived=0,      /* Whether the buffer (memory) type is derived and needs to be free'd */
		 mft_is_derived=0;      /* Whether the file type is derived and needs to be free'd */
    hbool_t	 plist_is_setup=0;      /* Whether the dxpl has been customized */
    uint8_t	*buf=(uint8_t *)_buf;   /* Alias for pointer arithmetic */
    int         mpi_code;               /* MPI return code */
    herr_t	 ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_spaces_xfer);

    /* Check args */
    assert (f);
    assert (layout);
    assert (file_space);
    assert (mem_space);
    assert (buf);
    assert (IS_H5FD_MPIO(f));
    /* Make certain we have the correct type of property list */
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));

    /* create the MPI buffer type */
    if (H5S_mpio_space_type( mem_space, elmt_size,
			       /* out: */
			       &mpi_buf_type,
			       &mpi_buf_count,
			       &mpi_buf_offset,
			       &mbt_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI buf type");

    /* create the MPI file type */
    if ( H5S_mpio_space_type( file_space, elmt_size,
			       /* out: */
			       &mpi_file_type,
			       &mpi_file_count,
			       &mpi_file_offset,
			       &mft_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI file type");

    /* Use the absolute base address of the dataset (or chunk, eventually) as
     * the address to read from.  This should be used as the diplacement for
     * a call to MPI_File_set_view() in the read or write call.
     */
    addr = f->shared->base_addr + layout->addr + mpi_file_offset;
#ifdef H5Smpi_DEBUG
    HDfprintf(stderr, "spaces_xfer: addr=%a\n", addr );
#endif

    /*
     * Pass buf type, file type to the file driver. Request an MPI type
     * transfer (instead of an elementary byteblock transfer).
     */
    if(H5FD_mpio_setup(dxpl_id, mpi_buf_type, mpi_file_type)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O properties");
    plist_is_setup=1;

    /* Adjust the buffer pointer to the beginning of the selection */
    buf+=mpi_buf_offset;

    /* transfer the data */
    if (do_write) {
    	if (H5FD_write(f->shared->lf, H5FD_MEM_DRAW, dxpl_id, addr, mpi_buf_count, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,"MPI write failed");
    } else {
    	if ( H5FD_read (f->shared->lf, H5FD_MEM_DRAW, dxpl_id, addr, mpi_buf_count, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL,"MPI read failed");
    }

done:
    /* Reset the dxpl settings */
    if(plist_is_setup) {
        if(H5FD_mpio_teardown(dxpl_id)<0)
    	    HDONE_ERROR(H5E_DATASPACE, H5E_CANTFREE, FAIL, "unable to reset dxpl values");
    } /* end if */

    /* free the MPI buf and file types */
    if (mbt_is_derived) {
	if (MPI_SUCCESS != (mpi_code= MPI_Type_free( &mpi_buf_type )))
            HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code);
    }
    if (mft_is_derived) {
	if (MPI_SUCCESS != (mpi_code= MPI_Type_free( &mpi_file_type )))
            HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code);
    }

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_spaces_read
 *
 * Purpose:	MPI-IO function to read directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 * rky 980918
 * Added must_convert parameter to let caller know we can't optimize the xfer.
 *
 *      QAK - 2002/04/02
 *      Removed the must_convert parameter and move preconditions to
 *      H5S_mpio_opt_possible() routine
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_spaces_read(H5F_t *f, const H5O_layout_t *layout,
    H5P_genplist_t UNUSED *dc_plist, const H5D_storage_t UNUSED *store, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space, hid_t dxpl_id,
    void *buf/*out*/)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI(H5S_mpio_spaces_read, FAIL);

    ret_value = H5S_mpio_spaces_xfer(f, layout, elmt_size,
				     file_space, mem_space, dxpl_id,
				     buf, 0/*read*/);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_spaces_write
 *
 * Purpose:	MPI-IO function to write directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 * rky 980918
 * Added must_convert parameter to let caller know we can't optimize the xfer.
 *
 *      QAK - 2002/04/02
 *      Removed the must_convert parameter and move preconditions to
 *      H5S_mpio_opt_possible() routine
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_spaces_write(H5F_t *f, H5O_layout_t *layout,
    H5P_genplist_t UNUSED *dc_plist, const H5D_storage_t UNUSED *store, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space, hid_t dxpl_id,
    const void *buf)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI(H5S_mpio_spaces_write, FAIL);

    /*OKAY: CAST DISCARDS CONST QUALIFIER*/
    ret_value = H5S_mpio_spaces_xfer(f, layout, elmt_size,
				     file_space, mem_space, dxpl_id,
				     (void*)buf, 1/*write*/);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_opt_possible
 *
 * Purpose:	Checks if an direct I/O transfer is possible between memory and
 *                  the file.
 *
 * Return:	Success:        Non-negative: TRUE or FALSE
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, April 3, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5S_mpio_opt_possible( const H5S_t *mem_space, const H5S_t *file_space, const unsigned flags)
{
    htri_t c1,c2;               /* Flags whether a selection is optimizable */
    htri_t ret_value=TRUE;

    FUNC_ENTER_NOAPI(H5S_mpio_opt_possible, FAIL);

    /* Check args */
    assert(mem_space);
    assert(file_space);

    /* Check whether these are both simple or scalar dataspaces */
    if (!((H5S_SIMPLE==mem_space->extent.type || H5S_SCALAR==mem_space->extent.type)
         && (H5S_SIMPLE==file_space->extent.type || H5S_SCALAR==file_space->extent.type)))
        HGOTO_DONE(FALSE);

    /* Check whether both selections are "regular" */
    c1=(*file_space->select.is_regular)(file_space);
    c2=(*mem_space->select.is_regular)(mem_space);
    if(c1==FAIL || c2==FAIL)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "invalid check for single selection blocks");
    if(c1==FALSE || c2==FALSE)
        HGOTO_DONE(FALSE);

    /* Can't currently handle point selections */
    if (H5S_SEL_POINTS==mem_space->select.type || H5S_SEL_POINTS==file_space->select.type)
        HGOTO_DONE(FALSE);

    /* Dataset storage must be contiguous currently */
    if ((flags&H5S_CONV_STORAGE_MASK)!=H5S_CONV_STORAGE_CONTIGUOUS)
        HGOTO_DONE(FALSE);

    /* Parallel I/O conversion flag must be set */
    if(!(flags&H5S_CONV_PAR_IO_POSSIBLE))
        HGOTO_DONE(FALSE);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5S_mpio_opt_possible() */

#endif  /* H5_HAVE_PARALLEL */
