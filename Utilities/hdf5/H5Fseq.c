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
 * Programmer: 	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *	       	Thursday, September 28, 2000
 *
 * Purpose:	Provides I/O facilities for sequences of bytes stored with various 
 *      layout policies.  These routines are similar to the H5Farray.c routines,
 *      these deal in terms of byte offsets and lengths, not coordinates and
 *      hyperslab sizes.
 *
 */

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK	H5Fseq_mask

#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5FDprivate.h"	/*file driver				  */
#include "H5Iprivate.h"
#include "H5MFprivate.h"
#include "H5MMprivate.h"	/*memory management			  */
#include "H5Oprivate.h"
#include "H5Pprivate.h"
#include "H5Vprivate.h"

/* MPIO & MPIPOSIX driver functions are needed for some special checks */
#include "H5FDmpio.h"
#include "H5FDmpiposix.h"

/* Interface initialization */
#define INTERFACE_INIT	NULL
static int interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:	H5F_seq_read
 *
 * Purpose:	Reads a sequence of bytes from a file dataset into a buffer in
 *      in memory.  The data is read from file F and the array's size and
 *      storage information is in LAYOUT.  External files are described
 *      according to the external file list, EFL.  The sequence offset is 
 *      DSET_OFFSET in the dataset (offsets are in terms of bytes) and the 
 *      size of the hyperslab is SEQ_LEN. The total size of the file array 
 *      is implied in the LAYOUT argument.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, September 28, 2000
 *
 * Modifications:
 *              Re-written to use new vector I/O call - QAK, 7/7/01
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_seq_read(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
    H5P_genplist_t *dc_plist, const H5D_storage_t *store, 
    size_t seq_len, hsize_t dset_offset, void *buf/*out*/)
{
    hsize_t mem_off=0;                  /* Offset in memory */
    size_t mem_len=seq_len;             /* Length in memory */
    size_t mem_curr_seq=0;              /* "Current sequence" in memory */
    size_t dset_curr_seq=0;             /* "Current sequence" in dataset */
    herr_t     ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_seq_read, FAIL);

    /* Check args */
    assert(f);
    assert(layout);
    assert(buf);
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));

    if (H5F_seq_readvv(f, dxpl_id, layout, dc_plist, store, 1, &dset_curr_seq, &seq_len, &dset_offset, 1, &mem_curr_seq, &mem_len, &mem_off, buf)<0)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "vector read failed");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5F_seq_read() */


/*-------------------------------------------------------------------------
 * Function:	H5F_seq_write
 *
 * Purpose:	Writes a sequence of bytes to a file dataset from a buffer in
 *      in memory.  The data is written to file F and the array's size and
 *      storage information is in LAYOUT.  External files are described
 *      according to the external file list, EFL.  The sequence offset is 
 *      DSET_OFFSET in the dataset (offsets are in terms of bytes) and the 
 *      size of the hyperslab is SEQ_LEN. The total size of the file array 
 *      is implied in the LAYOUT argument.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 9, 2000
 *
 * Modifications:
 *              Re-written to use new vector I/O routine - QAK, 7/7/01
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_seq_write(H5F_t *f, hid_t dxpl_id, H5O_layout_t *layout,
    H5P_genplist_t *dc_plist, const H5D_storage_t *store, 
    size_t seq_len, hsize_t dset_offset, const void *buf)
{
    hsize_t mem_off=0;                  /* Offset in memory */
    size_t mem_len=seq_len;             /* Length in memory */
    size_t mem_curr_seq=0;              /* "Current sequence" in memory */
    size_t dset_curr_seq=0;             /* "Current sequence" in dataset */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5F_seq_write, FAIL);

    /* Check args */
    assert(f);
    assert(layout);
    assert(buf);
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));

    if (H5F_seq_writevv(f, dxpl_id, layout, dc_plist, store, 1, &dset_curr_seq, &seq_len, &dset_offset, 1, &mem_curr_seq, &mem_len, &mem_off, buf)<0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "vector write failed");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5F_seq_write() */


/*-------------------------------------------------------------------------
 * Function:	H5F_seq_readvv
 *
 * Purpose:	Reads in a vector of byte sequences from a file dataset into a
 *      buffer in in memory.  The data is read from file F and the array's size
 *      and storage information is in LAYOUT.  External files are described
 *      according to the external file list, EFL.  The vector of byte sequences
 *      offsets is in the DSET_OFFSET array into the dataset (offsets are in
 *      terms of bytes) and the size of each sequence is in the SEQ_LEN array.
 *      The total size of the file array is implied in the LAYOUT argument.
 *      Bytes read into BUF are sequentially stored in the buffer, each sequence
 *      from the vector stored directly after the previous.  The number of
 *      sequences is NSEQ.
 * Purpose:	Reads a vector of byte sequences from a vector of byte
 *      sequences in a file dataset into a buffer in memory.  The data is
 *      read from file F and the array's size and storage information is in
 *      LAYOUT.  External files and chunks are described according to the
 *      storage information, STORE.  The vector of byte sequences offsets for
 *      the file is in the DSET_OFFSET_ARR array into the dataset (offsets are
 *      in terms of bytes) and the size of each sequence is in the DSET_LEN_ARR
 *      array.  The vector of byte sequences offsets for memory is in the
 *      MEM_OFFSET_ARR array into the dataset (offsets are in terms of bytes)
 *      and the size of each sequence is in the MEM_LEN_ARR array.  The total
 *      size of the file array is implied in the LAYOUT argument.  The maximum
 *      number of sequences in the file dataset and the memory buffer are
 *      DSET_MAX_NSEQ & MEM_MAX_NSEQ respectively.  The current sequence being
 *      operated on in the file dataset and the memory buffer are DSET_CURR_SEQ
 *      & MEM_CURR_SEQ respectively.  The current sequence being operated on
 *      will be updated as a result of the operation, as will the offsets and
 *      lengths of the file dataset and memory buffer sequences.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, May 7, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5F_seq_readvv(H5F_t *f, hid_t dxpl_id, const struct H5O_layout_t *layout,
    struct H5P_genplist_t *dc_plist, const H5D_storage_t *store, 
    size_t dset_max_nseq, size_t *dset_curr_seq,  size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[],
    void *buf/*out*/)
{
    ssize_t ret_value;            /* Return value */
   
    FUNC_ENTER_NOAPI(H5F_seq_readvv, FAIL);

    /* Check args */
    assert(f);
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER)); /* Make certain we have the correct type of property list */
    assert(layout);
    assert(dc_plist);
    assert(dset_curr_seq);
    assert(*dset_curr_seq<dset_max_nseq);
    assert(dset_len_arr);
    assert(dset_offset_arr);
    assert(mem_curr_seq);
    assert(*mem_curr_seq<mem_max_nseq);
    assert(mem_len_arr);
    assert(mem_offset_arr);
    assert(buf);

    switch (layout->type) {
        case H5D_CONTIGUOUS:
            /* Read directly from file if the dataset is in an external file */
            if (store && store->efl.nused>0) {
                /* Note: We can't use data sieve buffers for datasets in external files
                 *  because the 'addr' of all external files is set to 0 (above) and
                 *  all datasets in external files would alias to the same set of
                 *  file offsets, totally mixing up the data sieve buffer information. -QAK
                 */
                if((ret_value=H5O_efl_readvv(&(store->efl),
                        dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                        mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                        buf))<0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "external data read failed");
            } else {
                hsize_t	max_data;    			/*bytes in dataset	*/
                unsigned	u;				/*counters		*/

                /* Compute the size of the dataset in bytes */
                for(u=1, max_data=layout->dim[0]; u<layout->ndims; u++)
                    max_data *= layout->dim[u];

                /* Pass along the vector of sequences to read */
                if((ret_value=H5F_contig_readvv(f, max_data, layout->addr,
                        dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                        mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                        dxpl_id, buf))<0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "block read failed");
            } /* end else */
            break;

        case H5D_CHUNKED:
            assert(store);
            if((ret_value=H5F_istore_readvv(f, dxpl_id, layout, dc_plist, store->chunk_coords,
                    dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                    mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                    buf))<0)
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "istore read failed");
            break;

        case H5D_COMPACT:
            /* Pass along the vector of sequences to read */
            if((ret_value=H5F_compact_readvv(f, layout,
                    dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                    mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                    dxpl_id, buf))<0)
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "compact read failed");
            break;
                                                        
        default:
            assert("not implemented yet" && 0);
            HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout");
    }   /* end switch() */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5F_seq_readvv() */


/*-------------------------------------------------------------------------
 * Function:	H5F_seq_writevv
 *
 * Purpose:	Writes a vector of byte sequences from a buffer in memory into
 *      a vector of byte sequences in a file dataset.  The data is written to
 *      file F and the array's size and storage information is in LAYOUT.
 *      External files and chunks are described according to the storage
 *      information, STORE.  The vector of byte sequences offsets for the file
 *      is in the DSET_OFFSET_ARR array into the dataset (offsets are in
 *      terms of bytes) and the size of each sequence is in the DSET_LEN_ARR
 *      array.  The vector of byte sequences offsets for memory is in the
 *      MEM_OFFSET_ARR array into the dataset (offsets are in terms of bytes)
 *      and the size of each sequence is in the MEM_LEN_ARR array.  The total
 *      size of the file array is implied in the LAYOUT argument.  The maximum
 *      number of sequences in the file dataset and the memory buffer are
 *      DSET_MAX_NSEQ & MEM_MAX_NSEQ respectively.  The current sequence being
 *      operated on in the file dataset and the memory buffer are DSET_CURR_SEQ
 *      & MEM_CURR_SEQ respectively.  The current sequence being operated on
 *      will be updated as a result of the operation, as will the offsets and
 *      lengths of the file dataset and memory buffer sequences.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 2, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5F_seq_writevv(H5F_t *f, hid_t dxpl_id, struct H5O_layout_t *layout,
    struct H5P_genplist_t *dc_plist, const H5D_storage_t *store, 
    size_t dset_max_nseq, size_t *dset_curr_seq,  size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[],
    const void *buf)
{
    ssize_t     ret_value;              /* Return value */
   
    FUNC_ENTER_NOAPI(H5F_seq_writevv, FAIL);

    /* Check args */
    assert(f);
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER)); /* Make certain we have the correct type of property list */
    assert(layout);
    assert(dc_plist);
    assert(dset_curr_seq);
    assert(*dset_curr_seq<dset_max_nseq);
    assert(dset_len_arr);
    assert(dset_offset_arr);
    assert(mem_curr_seq);
    assert(*mem_curr_seq<mem_max_nseq);
    assert(mem_len_arr);
    assert(mem_offset_arr);
    assert(buf);

    switch (layout->type) {
        case H5D_CONTIGUOUS:
            /* Write directly to file if the dataset is in an external file */
            if (store && store->efl.nused>0) {
                /* Note: We can't use data sieve buffers for datasets in external files
                 *  because the 'addr' of all external files is set to 0 (above) and
                 *  all datasets in external files would alias to the same set of
                 *  file offsets, totally mixing up the data sieve buffer information. -QAK
                 */
                if ((ret_value=H5O_efl_writevv(&(store->efl),
                        dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                        mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                        buf))<0)
                    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "external data write failed");
            } else {
                hsize_t	max_data;    	/* Bytes in dataset */
                unsigned u;		/* Local index variable */

                /* Compute the size of the dataset in bytes */
                for(u=1, max_data=layout->dim[0]; u<layout->ndims; u++)
                    max_data *= layout->dim[u];

                /* Pass along the vector of sequences to write */
                if ((ret_value=H5F_contig_writevv(f, max_data, layout->addr,
                        dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                        mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                        dxpl_id, buf))<0)
                    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "block write failed");
            } /* end else */
            break;

        case H5D_CHUNKED:
            assert(store);
            if((ret_value=H5F_istore_writevv(f, dxpl_id, layout, dc_plist, store->chunk_coords,
                    dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                    mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                    buf))<0)
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "istore write failed");
            break;

        case H5D_COMPACT:       
            /* Pass along the vector of sequences to write */
            if((ret_value=H5F_compact_writevv(f, layout,
                    dset_max_nseq, dset_curr_seq, dset_len_arr, dset_offset_arr,
                    mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr,
                    dxpl_id, buf))<0)
                 HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "compact write failed");
            break;

        default:
            assert("not implemented yet" && 0);
            HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout");
    }   /* end switch() */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5F_seq_writevv() */
