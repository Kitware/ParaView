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

/*-------------------------------------------------------------------------
 *
 * Created:             H5MF.c
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             File memory management functions.
 *
 * Modifications:
 *      Robb Matzke, 5 Aug 1997
 *      Added calls to H5E.
 *
 *   Robb Matzke, 8 Jun 1998
 *  Implemented a very simple free list which is not persistent and which
 *  is lossy.
 *
 *-------------------------------------------------------------------------
 */
#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */


#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5FDprivate.h"
#include "H5MFprivate.h"


/*-------------------------------------------------------------------------
 * Function:    H5MF_alloc
 *
 * Purpose:     Allocate SIZE bytes of file memory and return the relative
 *    address where that contiguous chunk of file memory exists.
 *    The TYPE argument describes the purpose for which the storage
 *    is being requested.
 *
 * Return:      Success:        The file address of new chunk.
 *
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 11 1997
 *
 * Modifications:
 *    Robb Matzke, 1999-08-04
 *    Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
haddr_t
H5MF_alloc(H5F_t *f, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t  ret_value;

    FUNC_ENTER_NOAPI(H5MF_alloc, HADDR_UNDEF);

    /* check arguments */
    assert(f);
    assert(size > 0);

    /* Fail if we don't have write access */
    if (0==(f->intent & H5F_ACC_RDWR))
  HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, HADDR_UNDEF, "file is read-only");

    /* Check that the file can address the new space */
    if( H5MF_alloc_overflow(f, size) )
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "not enough address space in file");

    /* Allocate space from the virtual file layer */
    if (HADDR_UNDEF==(ret_value=H5FD_alloc(f->shared->lf, type, dxpl_id, size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "file allocation failed");

    /* Convert absolute file address to relative file address */
    assert(ret_value>=f->shared->base_addr);

    /* Set return value */
    ret_value -= f->shared->base_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5MF_xfree
 *
 * Purpose:     Frees part of a file, making that part of the file
 *              available for reuse.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 17 1997
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value
 *
 *     Robb Matzke, 1999-08-03
 *    Modified to use the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_xfree(H5F_t *f, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5MF_xfree);

    /* check arguments */
    assert(f);
    if (!H5F_addr_defined(addr) || 0 == size)
        HGOTO_DONE(SUCCEED);
    assert(addr!=0);

    /* Convert relative address to absolute address */
    addr += f->shared->base_addr;

    /* Allow virtual file layer to free block */
    if (H5FD_free(f->shared->lf, type, dxpl_id, addr, size)<0) {
#ifdef H5MF_DEBUG
  if (H5DEBUG(MF)) {
      fprintf(H5DEBUG(MF),
        "H5MF_free: lost %lu bytes of file storage\n",
        (unsigned long)size);
  }
#endif
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5MF_realloc
 *
 * Purpose:  Changes the size of an allocated chunk, possibly moving it to
 *    a new address.  The chunk to change is at address OLD_ADDR
 *    and is exactly OLD_SIZE bytes (if these are H5F_ADDR_UNDEF
 *    and zero then this function acts like H5MF_alloc).  The new
 *    size will be NEW_SIZE and its address is the return value (if
 *    NEW_SIZE is zero then this function acts like H5MF_free and
 *    an undefined address is returned).
 *
 *    If the new size is less than the old size then the new
 *    address will be the same as the old address (except for the
 *    special case where the new size is zero).
 *
 *    If the new size is more than the old size then most likely a
 *    new address will be returned.  However, under certain
 *    circumstances the library may return the same address.
 *
 * Return:  Success:  The relative file address of the new block.
 *
 *     Failure:  HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ORIG_ADDR is passed by value. The name of NEW_ADDR has
 *    been changed to NEW_ADDR_P
 *
 *     Robb Matzke, 1999-08-04
 *    Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
haddr_t
H5MF_realloc(H5F_t *f, H5FD_mem_t type, hid_t dxpl_id, haddr_t old_addr, hsize_t old_size,
       hsize_t new_size)
{
    haddr_t  ret_value;

    FUNC_ENTER_NOAPI(H5MF_realloc, HADDR_UNDEF);

    /* Convert old relative address to absolute address */
    old_addr += f->shared->base_addr;

    /* Check that the file can address the new space. */
    /* In the worst case, this means adding new_size bytes to the end of the file. */
    if( H5MF_alloc_overflow(f, new_size) )
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "not enough address space in file");

    /* Reallocate memory from the virtual file layer */
    ret_value = H5FD_realloc(f->shared->lf, type, dxpl_id, old_addr, old_size,
           new_size);
    if (HADDR_UNDEF==ret_value)
  HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "unable to allocate new file memory");

    /* Convert return value to relative address */
    assert(ret_value>=f->shared->base_addr);

    /* Set return value */
    ret_value -= f->shared->base_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:  H5MF_reserve
 *
 * Purpose:  Sets aside file space that has not yet been allocated, but will
 *    be (or might be in the worst case).  This number is used to
 *    ensure that there is room in the file when it is flushed to disk.
 *
 *    Nothing changes (and no error is generated) if the file is opened
 *    as read-only.
 *
 * Return:  Success:  0
 *
 *     Failure:  negative
 *
 * Programmer:  James Laird
 *    Nat Furrer
 *              Thursday, May 27, 2004
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_reserve(H5F_t *f, hsize_t size)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5MF_reserve, FAIL);

    /* Check arguments */
    assert(f);

    /* Check that there is room in the file to reserve this space */
    if( H5MF_alloc_overflow( f, size ) )
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "not enough address space in file");

    f->shared->lf->reserved_alloc += size;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:  H5MF_free_reserved
 *
 * Purpose:  Releases the file space set aside by H5MF_reserve.  This should
 *      be called immediately before allocating the file space for which
 *      the space was reserved.
 *
 * Return:  None
 *
 * Programmer:  James Laird
 *        Nat Furrer
 *              Thursday, May 27, 2004
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_free_reserved(H5F_t *f, hsize_t size)
{
    FUNC_ENTER_NOAPI_NOFUNC(H5MF_free_reserved)

    /* Check arguments */
    assert(f);

    /* If this assert breaks, it means that HDF5 is trying to free file space
     * that was never reserved.
     */
    assert(size <= f->shared->lf->reserved_alloc);

    f->shared->lf->reserved_alloc -= size;

    FUNC_LEAVE_NOAPI(SUCCEED)
}

/*-------------------------------------------------------------------------
 * Function:  H5MF_alloc_overflow
 *
 * Purpose:  Checks if an allocation of file space would cause an overflow.
 *          F is the file whose space is being allocated, SIZE is the amount
 *          of space needed.
 *
 * Return:  0 if no overflow would result
 *          1 if overflow would result (the allocation should not be allowed)
 *
 * Programmer:  James Laird
 *        Nat Furrer
 *              Tuesday, June 1, 2004
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
hbool_t
H5MF_alloc_overflow(H5F_t *f, hsize_t size)
{
    hsize_t space_needed;       /* Accumulator variable */
    size_t c;                   /* Local index variable */
    hbool_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5MF_alloc_overflow)

    /* Start with the current end of the file's address. */
    space_needed = (hsize_t)H5F_get_eoa(f);
    HDassert(H5F_addr_defined(space_needed));

    /* Subtract the file's base address to get the actual amount of
     * space being used:
     * (end of allocated space - beginning of allocated space)
     */
    HDassert(H5F_BASE_ADDR(f) < space_needed);
    space_needed -= (hsize_t)H5F_BASE_ADDR(f);

    /* Add the amount of space requested for this allocation */
    space_needed += size;

    /* Also add space that is "reserved" for data to be flushed
     * to disk (e.g., for object headers and the heap).
     * This is the total amount of file space that will be
     * allocated.
     */
    space_needed += f->shared->lf->reserved_alloc;

    /* Ensure that this final number is less than the file's
     * address space.  We do this by shifting in multiples
     * of 16 bits because some systems will do nothing if
     * we shift by the size of a long long (64 bits) all at
     * once (<cough> Linux <cough>).  Thus, we break one shift
     * into several smaller shifts.
     */
    for(c=0; c < H5F_SIZEOF_ADDR(f); c += 2)
        space_needed = space_needed >> 16;

    if(space_needed != 0)
        ret_value=TRUE;
    else
        ret_value=FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5MF_can_extend
 *
 * Purpose:  Check if a block in the file can be extended.
 *
 *    This is a simple check currently, which only checks for the
 *              block being at the end of the file.  A more sophisticated check
 *              would also use the free space list to see if there is a block
 *              appropriately placed to accomodate the space requested.
 *
 * Return:  Success:  TRUE(1)/FALSE(0)
 *
 *     Failure:  FAIL
 *
 * Programmer:  Quincey Koziol
 *              Friday, June 11, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_can_extend(H5F_t *f, H5FD_mem_t type, haddr_t addr, hsize_t size, hsize_t extra_requested)
{
    htri_t  ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5MF_can_extend, FAIL);

    /* Convert old relative address to absolute address */
    addr += H5F_BASE_ADDR(f);

    /* Pass the request down to the virtual file layer */
    if((ret_value=H5FD_can_extend(f->shared->lf, type, addr, size, extra_requested))<0)
  HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate new file memory");

    /* Make sure there is enough addressable space to satisfy the request */
    if (ret_value == TRUE)
        ret_value = !H5MF_alloc_overflow(f, extra_requested);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5MF_can_extend() */


/*-------------------------------------------------------------------------
 * Function:  H5MF_extend
 *
 * Purpose:  Extend a block in the file.
 *
 * Return:  Success:  TRUE(1)/FALSE(0)
 *
 *     Failure:  FAIL
 *
 * Programmer:  Quincey Koziol
 *              Saturday, June 12, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_extend(H5F_t *f, H5FD_mem_t type, haddr_t addr, hsize_t size, hsize_t extra_requested)
{
    htri_t  ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5MF_extend, FAIL);

    /* Make sure there is enough addressable space to satisfy the request */
    if ( H5MF_alloc_overflow(f, extra_requested) )
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate new file memory: out of address space");

    /* Convert old relative address to absolute address */
    addr += H5F_BASE_ADDR(f);

    /* Pass the request down to the virtual file layer */
    if((ret_value=H5FD_extend(f->shared->lf, type, addr, size, extra_requested))<0)
  HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate new file memory");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5MF_extend() */
