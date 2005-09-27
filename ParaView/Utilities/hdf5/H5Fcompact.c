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
 * Programmer:  Raymond Lu <slu@ncsa.uiuc.edu> 
 *              August 5, 2002 
 *
 * Purpose:     Compact dataset I/O functions.  These routines are similar
 *              H5F_contig_* and H5F_istore_*.
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK	H5Fcompact_mask

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5Oprivate.h"
#include "H5FDprivate.h"        /*file driver                             */
#include "H5FLprivate.h"        /*Free Lists                              */
#include "H5Vprivate.h"		/* Vector and array functions		*/

/* Interface initialization */
static int              interface_initialize_g = 0;
#define INTERFACE_INIT NULL


/*-------------------------------------------------------------------------
 * Function:    H5F_compact_readvv
 * 
 * Purpose:     Reads some data vectors from a dataset into a buffer.
 *              The data is in compact dataset.  The address is relative 
 *              to the beginning address of the dataset.  The offsets and
 *              sequence lengths are in bytes.
 *              
 * Return:      Non-negative on success/Negative on failure
 * 
 * Programmer:  Quincey Koziol
 *              May 7, 2003
 *              
 * Notes:       
 *              Offsets in the sequences must be monotonically increasing
 *              
 * Modifications:
 * 
 *-------------------------------------------------------------------------
 */
ssize_t
H5F_compact_readvv(H5F_t UNUSED *f, const H5O_layout_t *layout,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_size_arr[], hsize_t dset_offset_arr[], 
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_size_arr[], hsize_t mem_offset_arr[], 
    hid_t UNUSED dxpl_id, void *buf)
{
    ssize_t ret_value;          /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_compact_readvv, FAIL);

    /* Use the vectorized memory copy routine to do actual work */
    if((ret_value=H5V_memcpyvv(buf,mem_max_nseq,mem_curr_seq,mem_size_arr,mem_offset_arr,layout->buf,dset_max_nseq,dset_curr_seq,dset_size_arr,dset_offset_arr))<0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "vectorized memcpy failed");

done:   
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5F_compact_readvv() */


/*-------------------------------------------------------------------------
 * Function:    H5F_compact_writevv
 * 
 * Purpose:     Writes some data vectors from a dataset into a buffer.
 *              The data is in compact dataset.  The address is relative 
 *              to the beginning address for the file.  The offsets and
 *              sequence lengths are in bytes.  This function only copies
 *              data into the buffer in the LAYOUT struct and mark it 
 *              as DIRTY.  Later in H5D_close, the data is copied into 
 *              header message in memory.  
 *              
 * Return:      Non-negative on success/Negative on failure
 * 
 * Programmer:  Quincey Koziol
 *              May 2, 2003
 *              
 * Notes:       
 *              Offsets in the sequences must be monotonically increasing
 *              
 * Modifications:
 * 
 *-------------------------------------------------------------------------
 */
ssize_t
H5F_compact_writevv(H5F_t UNUSED *f, H5O_layout_t *layout,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_size_arr[], hsize_t dset_offset_arr[], 
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_size_arr[], hsize_t mem_offset_arr[], 
    hid_t UNUSED dxpl_id, const void *buf)
{
    ssize_t ret_value;          /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_compact_writevv, FAIL);

    /* Use the vectorized memory copy routine to do actual work */
    if((ret_value=H5V_memcpyvv(layout->buf,dset_max_nseq,dset_curr_seq,dset_size_arr,dset_offset_arr,buf,mem_max_nseq,mem_curr_seq,mem_size_arr,mem_offset_arr))<0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "vectorized memcpy failed");

    layout->dirty = TRUE;

done:   
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5F_compact_writevv() */

