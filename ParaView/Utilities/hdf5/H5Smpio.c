/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  rky 980813
 *
 * Purpose:     Functions to read/write directly between app buffer and file.
 *
 *              Beware of the ifdef'ed print statements.
 *              I didn't make them portable.
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */
#define H5S_PACKAGE             /*suppress error about including H5Spkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"         /* Ugly, but necessary for the MPIO I/O accesses */
#include "H5Spkg.h"

#include "H5FDmpio.h"           /*the MPIO file driver                  */

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
H5S_mpio_all_type( const H5S_t *space, const size_t elmt_size,
                     /* out: */
                     MPI_Datatype *new_type,
                     hsize_t *count,
                     hbool_t *is_derived_type );
static herr_t
H5S_mpio_hyper_type( const H5S_t *space, const size_t elmt_size,
                     /* out: */
                     MPI_Datatype *new_type,
                     hsize_t *count,
                     hbool_t *is_derived_type );
static herr_t
H5S_mpio_space_type( const H5S_t *space, const size_t elmt_size,
                     /* out: */
                     MPI_Datatype *new_type,
                     hsize_t *count,
                     hbool_t *is_derived_type );
static herr_t
H5S_mpio_spaces_xfer(H5F_t *f, const struct H5O_layout_t *layout,
                     const struct H5O_pline_t UNUSED *pline,
                     const struct H5O_fill_t UNUSED *fill,
                     const struct H5O_efl_t UNUSED *efl, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     hid_t dxpl_id, void *buf/*out*/,
                     hbool_t *must_convert/*out*/, const hbool_t do_write);

/*-------------------------------------------------------------------------
 * Function:    H5S_mpio_all_type
 *
 * Purpose:     Translate an HDF5 "all" selection into an MPI type.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Outputs:     *new_type         the MPI type corresponding to the selection
 *              *count            how many objects of the new_type in selection
 *                                (useful if this is the buffer type for xfer)
 *              *is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:  rky 980813
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_all_type( const H5S_t *space, const size_t elmt_size,
                     /* out: */
                     MPI_Datatype *new_type,
                     hsize_t *count,
                     hbool_t *is_derived_type )
{
    hsize_t     total_bytes;
    int         i;

    FUNC_ENTER (H5S_mpio_all_type, FAIL);

    /* Check args */
    assert (space);

    /* Just treat the entire extent as a block of bytes */
    total_bytes = (hsize_t)elmt_size;
    for (i=0; i<space->extent.u.simple.rank; ++i) {
        total_bytes *= space->extent.u.simple.size[i];
    }

    /* fill in the return values */
    *new_type = MPI_BYTE;
    *count = total_bytes;
    *is_derived_type = 0;

#ifdef H5Smpi_DEBUG
    fprintf(stdout, "Leave %s total_bytes=%Hu\n", FUNC, total_bytes );
#endif
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_mpio_hyper_type
 *
 * Purpose:     Translate an HDF5 hyperslab selection into an MPI type.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Outputs:     *new_type         the MPI type corresponding to the selection
 *              *count            how many objects of the new_type in selection
 *                                (useful if this is the buffer type for xfer)
 *              *is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:  rky 980813
 *
 * Modifications:  ppw 990401
 *              rky, ppw 2000-09-26 Freed old type after creating struct type.
 *              rky 2000-10-05 Changed displacements to be MPI_Aint.
 *              rky 2000-10-06 Added code for cases of empty hyperslab.
 *              akc, rky 2000-11-16 Replaced hard coded dimension size with
 *                  H5S_MAX_RANK.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_hyper_type( const H5S_t *space, const size_t elmt_size,
                     /* out: */
                     MPI_Datatype *new_type,
                     hsize_t *count,
                     hbool_t *is_derived_type )
{
    struct dim {        /* less hassle than malloc/free & ilk */
        hssize_t start;
        hsize_t strid;
        hsize_t block;
        hsize_t xtent;
        hsize_t count;
    } d[H5S_MAX_RANK];

    int                 i, err, new_rank, num_to_collapse;
    int                 offset[H5S_MAX_RANK];
    int                 max_xtent[H5S_MAX_RANK];
    H5S_hyper_dim_t     *diminfo;               /* [rank] */
    int         rank;
    int                 block_length[2];
    MPI_Datatype        inner_type, outer_type, old_type[2];
    MPI_Aint            extent_len, displacement[2];

    FUNC_ENTER (H5S_mpio_hyper_type, FAIL);

    /* Check and abbreviate args */
    assert (space);
    assert(sizeof(MPI_Aint) >= sizeof(elmt_size));
    diminfo = space->select.sel_info.hslab.diminfo;
    assert (diminfo);
    rank = space->extent.u.simple.rank;
    assert (rank >= 0);
    if (0==rank) goto empty;
    if (0==elmt_size) goto empty;

    /* make a local copy of the dimension info so we can transform them */
    assert(rank<=H5S_MAX_RANK); /* within array bounds */
    for ( i=0; i<rank; ++i) {
        d[i].start = diminfo[i].start;
        d[i].strid = diminfo[i].stride;
        d[i].block = diminfo[i].block;
        d[i].count = diminfo[i].count;
        d[i].xtent = space->extent.u.simple.size[i];
#ifdef H5Smpi_DEBUG
        HDfprintf(stdout, "hyper_type: start=%Hd  stride=%Hu  count=%Hu  "
                "block=%Hu  xtent=%Hu",
                d[i].start, d[i].strid, d[i].count, d[i].block, d[i].xtent );
        if (i==0) fprintf(stdout, "  rank=%d\n", rank );
        else      fprintf(stdout, "\n" );
#endif
        if (0==d[i].block) goto empty;
        if (0==d[i].count) goto empty;
        if (0==d[i].xtent) goto empty;
    }
    
/**********************************************************************
    Compute array "offset[rank]" which gives the offsets for a multi-
    dimensional array with dimensions "d[i].xtent" (i=0,1,...,rank-1). 
**********************************************************************/
    offset[rank-1] = 1;
    max_xtent[rank-1] = d[rank-1].xtent;
#ifdef H5Smpi_DEBUG
          i=rank-1;
          fprintf(stdout, " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
#endif
    for (i=rank-2; i>=0; --i) {
        offset[i] = offset[i+1]*d[i+1].xtent;
        max_xtent[i] = max_xtent[i+1]*d[i].xtent;
#ifdef H5Smpi_DEBUG
          fprintf(stdout, " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
#endif
    }

    /*  Create a type covering the selected hyperslab.
     *  Multidimensional dataspaces are stored in row-major order.
     *  The type is built from the inside out, going from the
     *  fastest-changing (i.e., inner) dimension * to the slowest (outer). */

    /*  Optimization: check for contiguous inner dimensions.
     *  Supposing the dimensions were numbered from 1 to rank, we find that
     *
     *  dim d=rank is contiguous if:
     *                  stride[d] = block[d]
     *   and  count[d] * block[d] = extent.u.simple.size[d]
     *
     *  (i.e., there's no overlap or gaps and the entire extent is filled.)
     *
     * dim d (1<=d<rank) is contiguous if:
     *                    dim d+1 is contiguous
     *   and            stride[d] = block[d]
     *   and  count[d] * block[d] = extent.u.simple.size[d]
     *
     *  There is also a weak sense in which the first noncollapsible dim
     *  is contiguous if it consists of a single unbroken range,
     *  and we also take advantage of that.
     */

    /* figure out how many dimensions we can eliminate */
    /* This loop examines contiguity from the inside out. */
    for ( i=0; i<rank; ++i) {
        if ((d[rank-i-1].strid != d[rank-i-1].block)
                ||
            (d[rank-i-1].count*d[rank-i-1].block) != space->extent.u.simple.size[rank-i-1]) {
            break;
        }
    } /* end for */
    num_to_collapse = i;
    if (num_to_collapse == rank) num_to_collapse--;

    assert(0<=num_to_collapse && num_to_collapse<rank);
    new_rank = rank - num_to_collapse;
#ifdef H5Smpi_DEBUG
    fprintf(stdout, "num_to_collapse=%d\n", num_to_collapse);
    fprintf(stdout, "hyper_type: new_rank=%d\n", new_rank );
#endif

    /* To collapse dims, just transform dimension info (from inner to outer) */
    /* must multiply r.h.s. by "count" ---  ppw 03/11/99  */
    for (i=rank-1; i>=new_rank; --i) {
        d[i-1].block *= d[i].count * d[i].block;
        d[i-1].strid *= d[i].count * d[i].block;
        d[i-1].xtent *= d[i].count * d[i].block;
        assert( d[i].start == 0 );
        /* d[i-1].start stays unchanged */
        /* d[i-1].count stays unchanged */
    }

    /* check for possibility to coalesce blocks of the uncoalesced dimensions */
    for (i=0; i<new_rank; ++i) {
        if (d[i].strid == d[i].block) {
            /* transform smaller blocks to 1 larger block of combined size */
            d[i].strid = d[i].block *= d[i].count;
            d[i].count = 1;
        }
    }

/*******************************************************
*  Construct contig type for inner contig dims:
*******************************************************/
#ifdef H5Smpi_DEBUG
    fprintf(stdout, "hyper_type: Making contig type %d MPI_BYTEs\n", elmt_size );
    for (i=new_rank-1; i>=0; --i) {
      HDfprintf(stdout, "d[%d].xtent=%Hu \n", i, d[i].xtent);
    }
#endif
    err = MPI_Type_contiguous( (int)elmt_size, MPI_BYTE, &inner_type );
    if (err) {
        HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,"couldn't create MPI contiguous type");
    }

/*******************************************************
*  Construct the type by walking the hyperslab dims
*  from the inside out:
*******************************************************/
    for ( i=new_rank-1; i>=0; --i) {
#ifdef H5Smpi_DEBUG
        HDfprintf(stdout, "hyper_type: Dimension i=%d \n"
                "count=%Hu block=%Hu stride=%Hu\n",
                i, d[i].count, d[i].block, d[i].strid );
#endif

#ifdef H5Smpi_DEBUG
        HDfprintf(stdout, "hyper_type: i=%d  Making vector-type \n", i);
#endif
       /****************************************
       *  Build vector in current dimension:
       ****************************************/
          err = MPI_Type_vector ( (int)(d[i].count),        /* count */
                                  (int)(d[i].block),        /* blocklength */
                                  (int)(d[i].strid),        /* stride */
                                  inner_type,               /* old type */
                                  &outer_type );            /* new type */

          MPI_Type_free( &inner_type );
          if (err) {
              HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,"couldn't create MPI vector type");
          }

          displacement[1] = (MPI_Aint)elmt_size * max_xtent[i];
          err = MPI_Type_extent(outer_type, &extent_len);

       /*************************************************
       *  Restructure this datatype ("outer_type")
       *  so that it still starts at 0, but its extent
       *  is the full extent in this dimension.
       *************************************************/
          if ((int)extent_len < displacement[1]) {

#ifdef H5Smpi_DEBUG
        HDfprintf(stdout, "hyper_type: i=%d Extending struct type\n"
                "***displacements: 0, %d\n", i, displacement[1]);
#endif

#ifdef H5_HAVE_MPI2  /*  have MPI-2  */
            err = MPI_Type_create_resized
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
#ifdef H5Smpi_DEBUG
        HDfprintf(stdout, "hyper_type: i=%d Extending struct type\n"
                "***displacements: 0, %d\n", i, displacement[1]);
#endif
            err = MPI_Type_struct ( 2,               /* count */
                                    block_length,    /* blocklengths */
                                    displacement,    /* displacements */
                                    old_type,        /* old types */
                                    &inner_type);    /* new type */
#endif
  
            MPI_Type_free (&outer_type);
            if (err) {
                HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,"couldn't resize MPI vector type");
            }
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
    for (i=new_rank-1; i>=0; i--)
       displacement[0] += d[i].start * offset[i];

    if (displacement[0] > 0) {
      displacement[0] *= elmt_size;
      block_length[0] = 1;
      old_type[0] = inner_type;

#ifdef H5Smpi_DEBUG
        HDfprintf(stdout, "hyper_type:  Making final struct\n"
                "***count=1:\n");
        HDfprintf(stdout, "\tblocklength[0]=%d; displacement[0]=%d\n",
                     block_length[0], displacement[0]);
#endif

      err = MPI_Type_struct( 1,                  /* count */
                             block_length,       /* blocklengths */
                             displacement,       /* displacements */
                             old_type,           /* old type */
                             new_type );         /* new type */
  
      if (err) {
         HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,"couldn't create MPI struct type");
      }
      MPI_Type_free (&old_type[0]);
      if (err) {
            HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,"couldn't resize MPI vector type");
      }
    }
    else {
      *new_type = inner_type;
    }

    err = MPI_Type_commit( new_type );
    if (err) {
        HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,"couldn't commit MPI vector type");
    }

    /* fill in the remaining return values */
    *count = 1;                 /* only have to move one of these suckers! */
    *is_derived_type = 1;
    goto done;

empty:
    /* special case: empty hyperslab */
    *new_type = MPI_BYTE;
    *count = 0;
    *is_derived_type = 0;

done:
#ifdef H5Smpi_DEBUG
    HDfprintf(stdout, "Leave %s, count=%Hu  is_derived_type=%d\n",
                FUNC, *count, *is_derived_type );
#endif
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_mpio_space_type
 *
 * Purpose:     Translate an HDF5 dataspace selection into an MPI type.
 *              Currently handle only hyperslab and "all" selections.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Outputs:     *new_type         the MPI type corresponding to the selection
 *              *count            how many objects of the new_type in selection
 *                                (useful if this is the buffer type for xfer)
 *              *is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:  rky 980813
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_space_type( const H5S_t *space, const size_t elmt_size,
                     /* out: */
                     MPI_Datatype *new_type,
                     hsize_t *count,
                     hbool_t *is_derived_type )
{
    int         err;
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER (H5S_mpio_space_type, FAIL);

    /* Check args */
    assert (space);

    /* Creat MPI type based on the kind of selection */
    switch (space->extent.type) {
        case H5S_SCALAR:
            /* not yet implemented */
            ret_value = FAIL;
            break;

        case H5S_SIMPLE:
            switch(space->select.type) {
                case H5S_SEL_NONE:
                case H5S_SEL_ALL:
                    err = H5S_mpio_all_type( space, elmt_size,
                                /* out: */ new_type, count, is_derived_type );
                    if (err<0) {
                        HRETURN_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    }
                    break;

                case H5S_SEL_POINTS:
                    /* not yet implemented */
                    ret_value = FAIL;
                    break;

                case H5S_SEL_HYPERSLABS:
                    err = H5S_mpio_hyper_type( space, elmt_size,
                                /* out: */ new_type, count, is_derived_type );
                    if (err) {
                        HRETURN_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    }
                    break;

                default:
                    assert("unknown selection type" && 0);
                    break;
            } /* end switch */
            break;

        case H5S_COMPLEX:
            /* not yet implemented */
            ret_value = FAIL;
            break;

        default:
            assert("unknown data space type" && 0);
            break;
    }

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_mpio_spaces_xfer
 *
 * Purpose:     Use MPI-IO to transfer data efficiently
 *              directly between app buffer and file.
 *
 * Return:      non-negative on success, negative on failure.
 *              If MPI-IO is indeed used, must_convert is set to 0;
 *              otherwise it is set to 1 with return code succeed (so that the
 *              calling routine may try other means.)
 *
 * Programmer:  rky 980813
 *
 * Modifications:
 *      rky 980918
 *      Added must_convert parameter to let caller know we can't optimize
 *      the xfer.
 *
 *      Albert Cheng, 001123
 *      Include the MPI_type freeing as part of cleanup code.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_spaces_xfer(H5F_t *f, const struct H5O_layout_t *layout,
                     const struct H5O_pline_t UNUSED *pline,
                     const struct H5O_fill_t UNUSED *fill,
                     const struct H5O_efl_t UNUSED *efl, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     hid_t dxpl_id, void *buf /*out*/,
                     hbool_t *must_convert /*out*/,
                     const hbool_t do_write )
{
    herr_t       ret_value = SUCCEED;
    int          err;
    haddr_t      disp, addr;
    hsize_t      mpi_count;
    hsize_t      mpi_buf_count, mpi_unused_count;
    MPI_Datatype mpi_buf_type, mpi_file_type;
    hbool_t      mbt_is_derived=0,
                 mft_is_derived=0;

    FUNC_ENTER (H5S_mpio_spaces_xfer, FAIL);

    *must_convert = 0;  /* means we at least tried to do optimized xfer */

    /* Check args */
    assert (f);
    assert (layout);
    assert (file_space);
    assert (mem_space);
    assert (buf);
    assert (IS_H5FD_MPIO(f));

    /* INCOMPLETE!!!  rky 980816 */
    /* Currently can only handle H5D_CONTIGUOUS layout */
    if (layout->type != H5D_CONTIGUOUS) {
#ifdef H5S_DEBUG
        if (H5DEBUG(S)) {
            fprintf (H5DEBUG(S), "H5S: can only create MPI datatype for hyperslab when layout is contiguous.\n" );
        }
#endif
        *must_convert = 1;      /* can't do optimized xfer; do the old way */
        HGOTO_DONE(SUCCEED);
    }

    /*
     * For collective data transfer only since this would eventually
     * call H5FD_mpio_setup to do setup to eveually call MPI_File_set_view
     * in H5FD_mpio_read or H5FD_mpio_write.  MPI_File_set_view is a
     * collective call.  Letting independent data transfer use this
     * route would result in hanging.
     */
#if 0
    /* For now, the checking is being done in
     * H5D_write and H5D_read before it is called because
     * the following block of code, though with the right idea, is not
     * correct yet.
     */
    {   /* Get the transfer mode */
        H5D_xfer_t *dxpl;
        H5FD_mpio_dxpl_t *dx;

        if (H5P_DEFAULT!=dxpl_id && (dxpl=H5I_object(dxpl_id)) &&
                H5FD_MPIO==dxpl->driver_id && (dx=dxpl->driver_info) &&
                H5FD_MPIO_COLLECTIVE==dx->xfer_mode) {
            /* let it fall through */
        }else{
            *must_convert = 1;  /* can't do optimized xfer; do the old way */
            HGOTO_DONE(SUCCEED);
        }
    }
#endif

    /* create the MPI buffer type */
    err = H5S_mpio_space_type( mem_space, elmt_size,
                               /* out: */
                               &mpi_buf_type,
                               &mpi_buf_count,
                               &mbt_is_derived );
    if (MPI_SUCCESS != err)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI buf type");

    /* create the MPI file type */
    err = H5S_mpio_space_type( file_space, elmt_size,
                               /* out: */
                               &mpi_file_type,
                               &mpi_unused_count,
                               &mft_is_derived );
    if (MPI_SUCCESS != err)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI file type");

    /* calculate the absolute base addr (i.e., the file view disp) */
    disp = f->shared->base_addr + layout->addr;
#ifdef H5Smpi_DEBUG
    HDfprintf(stdout, "spaces_xfer: disp=%Hu\n", disp );
#endif

    /* Effective address determined by base addr and the MPI file type */
    addr = 0;

    /*
     * Pass buf type, file type, and absolute base address (i.e., the file
     * view disp) to the file driver. Request a dataspace transfer (instead
     * of an elementary byteblock transfer).
     */
    H5FD_mpio_setup(f->shared->lf, mpi_buf_type, mpi_file_type, disp, 1);

    /* transfer the data */
    mpi_count = (size_t)mpi_buf_count;
    if (mpi_count != mpi_buf_count) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,
                      "transfer size overflows size_t");
    }
    if (do_write) {
        err = H5FD_write(f->shared->lf, H5FD_MEM_DRAW, dxpl_id, addr, mpi_count, buf);
        if (err) {
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,"MPI write failed");
        }
    } else {
        err = H5FD_read (f->shared->lf, H5FD_MEM_DRAW, dxpl_id, addr, mpi_count, buf);
        if (err) {
            HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,"MPI read failed");
        }
    }

done:
    /* free the MPI buf and file types */
    if (mbt_is_derived) {
        err = MPI_Type_free( &mpi_buf_type );
        if (MPI_SUCCESS != err) {
            HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,
                          "unable to free MPI file type");
        }
    }
    if (mft_is_derived) {
        err = MPI_Type_free( &mpi_file_type );
        if (MPI_SUCCESS != err) {
            HRETURN_ERROR(H5E_DATASPACE, H5E_MPI, FAIL,
                          "unable to free MPI file type");
        }
    }

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_mpio_spaces_read
 *
 * Purpose:     MPI-IO function to read directly from app buffer to file.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Programmer:  rky 980813
 *
 * Modifications:
 *
 * rky 980918
 * Added must_convert parameter to let caller know we can't optimize the xfer.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_spaces_read(H5F_t *f, const struct H5O_layout_t *layout,
                     const struct H5O_pline_t *pline,
                     const struct H5O_fill_t *fill,
                     const struct H5O_efl_t *efl, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     hid_t dxpl_id, void *buf/*out*/,
                     hbool_t *must_convert/*out*/)
{
    herr_t ret_value = FAIL;

    FUNC_ENTER (H5S_mpio_spaces_read, FAIL);

    ret_value = H5S_mpio_spaces_xfer(f, layout, pline, fill, efl, elmt_size,
                                     file_space, mem_space, dxpl_id,
                                     buf, must_convert/*out*/, 0/*read*/);

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_mpio_spaces_write
 *
 * Purpose:     MPI-IO function to write directly from app buffer to file.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Programmer:  rky 980813
 *
 * Modifications:
 *
 * rky 980918
 * Added must_convert parameter to let caller know we can't optimize the xfer.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_spaces_write(H5F_t *f, const struct H5O_layout_t *layout,
                      const struct H5O_pline_t *pline,
                      const struct H5O_fill_t *fill,
                      const struct H5O_efl_t *efl, size_t elmt_size,
                      const H5S_t *file_space, const H5S_t *mem_space,
                      hid_t dxpl_id, const void *buf,
                      hbool_t *must_convert/*out*/)
{
    herr_t ret_value = FAIL;

    FUNC_ENTER (H5S_mpio_spaces_write, FAIL);

    ret_value = H5S_mpio_spaces_xfer(f, layout, pline, fill, efl, elmt_size,
                                     file_space, mem_space, dxpl_id,
                                     (void*)buf, must_convert/*out*/,
                                     1/*write*/);

    FUNC_LEAVE (ret_value);
}

#endif  /* H5_HAVE_PARALLEL */
