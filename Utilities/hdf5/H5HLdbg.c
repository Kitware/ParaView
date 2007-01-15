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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Wednesday, July 9, 2003
 *
 * Purpose:  Local Heap object debugging functions.
 */
#define H5HL_PACKAGE    /* Suppress error about including H5HLpkg */


#include "H5private.h"    /* Generic Functions      */
#include "H5ACprivate.h"  /* Metadata cache      */
#include "H5Eprivate.h"    /* Error handling            */
#include "H5HLpkg.h"    /* Local heaps        */
#include "H5Iprivate.h"    /* ID Functions                    */
#include "H5MMprivate.h"  /* Memory management      */


/*-------------------------------------------------------------------------
 * Function:  H5HL_debug
 *
 * Purpose:  Prints debugging information about a heap.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    matzke@llnl.gov
 *    Aug  1 1997
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE * stream, int indent, int fwidth)
{
    H5HL_t    *h = NULL;
    int      i, j, overlap, free_block;
    uint8_t    c;
    H5HL_free_t    *freelist = NULL;
    uint8_t    *marker = NULL;
    size_t    amount_free = 0;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HL_debug, FAIL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    if (NULL == (h = H5AC_protect(f, dxpl_id, H5AC_LHEAP, addr, NULL, NULL, H5AC_READ)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to load heap");

    fprintf(stream, "%*sLocal Heap...\n", indent, "");
    fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
      "Dirty:",
      (int) (h->cache_info.is_dirty));
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
      "Header size (in bytes):",
      (unsigned long) H5HL_SIZEOF_HDR(f));
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
        "Address of heap data:",
        h->addr);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
      "Data bytes allocated on disk:",
            h->disk_alloc);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
      "Data bytes allocated in core:",
            h->mem_alloc);

    /*
     * Traverse the free list and check that all free blocks fall within
     * the heap and that no two free blocks point to the same region of
     * the heap.  */
    if (NULL==(marker = H5MM_calloc(h->mem_alloc)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

    fprintf(stream, "%*sFree Blocks (offset, size):\n", indent, "");

    for (free_block=0, freelist = h->freelist; freelist; freelist = freelist->next, free_block++) {
        char temp_str[32];

        sprintf(temp_str,"Block #%d:",free_block);
  HDfprintf(stream, "%*s%-*s %8Zu, %8Zu\n", indent+3, "", MAX(0,fwidth-9),
    temp_str,
    freelist->offset, freelist->size);
  if (freelist->offset + freelist->size > h->mem_alloc) {
      fprintf(stream, "***THAT FREE BLOCK IS OUT OF BOUNDS!\n");
  } else {
      for (i=overlap=0; i<(int)(freelist->size); i++) {
    if (marker[freelist->offset + i])
        overlap++;
    marker[freelist->offset + i] = 1;
      }
      if (overlap) {
    fprintf(stream, "***THAT FREE BLOCK OVERLAPPED A PREVIOUS "
      "ONE!\n");
      } else {
    amount_free += freelist->size;
      }
  }
    }

    if (h->mem_alloc) {
  fprintf(stream, "%*s%-*s %.2f%%\n", indent, "", fwidth,
    "Percent of heap used:",
    (100.0 * (double)(h->mem_alloc - amount_free) / (double)h->mem_alloc));
    }
    /*
     * Print the data in a VMS-style octal dump.
     */
    fprintf(stream, "%*sData follows (`__' indicates free region)...\n",
      indent, "");
    for (i=0; i<(int)(h->disk_alloc); i+=16) {
  fprintf(stream, "%*s %8d: ", indent, "", i);
  for (j = 0; j < 16; j++) {
      if (i+j<(int)(h->disk_alloc)) {
    if (marker[i + j]) {
        fprintf(stream, "__ ");
    } else {
        c = h->chunk[H5HL_SIZEOF_HDR(f) + i + j];
        fprintf(stream, "%02x ", c);
    }
      } else {
    fprintf(stream, "   ");
      }
      if (7 == j)
    HDfputc(' ', stream);
  }

  for (j = 0; j < 16; j++) {
      if (i+j < (int)(h->disk_alloc)) {
    if (marker[i + j]) {
        HDfputc(' ', stream);
    } else {
        c = h->chunk[H5HL_SIZEOF_HDR(f) + i + j];
        if (c > ' ' && c < '~')
      HDfputc(c, stream);
        else
      HDfputc('.', stream);
    }
      }
  }

  HDfputc('\n', stream);
    }

done:
    if (h && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP, addr, h, FALSE) != SUCCEED)
  HDONE_ERROR(H5E_OHDR, H5E_PROTECT, FAIL, "unable to release object header");
    H5MM_xfree(marker);

    FUNC_LEAVE_NOAPI(ret_value);
}
