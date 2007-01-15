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
 * Purpose:  Global Heap object debugging functions.
 */
#define H5HG_PACKAGE    /*suppress error about including H5HGpkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5ACprivate.h"  /* Metadata cache      */
#include "H5Eprivate.h"    /* Error handling            */
#include "H5HGpkg.h"    /* Global heaps        */
#include "H5Iprivate.h"    /* ID Functions                    */


/*-------------------------------------------------------------------------
 * Function:  H5HG_debug
 *
 * Purpose:  Prints debugging information about a global heap collection.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    matzke@llnl.gov
 *    Mar 27, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *
 *              Robb Matzke, LLNL, 2003-06-05
 *              The size does not include the object header, just the data.
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent,
    int fwidth)
{
    unsigned    u, nused, maxobj;
    unsigned    j, k;
    H5HG_heap_t    *h = NULL;
    char    buf[64];
    uint8_t    *p = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HG_debug, FAIL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined (addr));
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    if (NULL == (h = H5AC_protect(f, dxpl_id, H5AC_GHEAP, addr, NULL, NULL, H5AC_READ)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to load global heap collection");

    fprintf(stream, "%*sGlobal Heap Collection...\n", indent, "");
    fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
      "Dirty:",
      (int)(h->cache_info.is_dirty));
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
      "Total collection size in file:",
      (unsigned long)(h->size));

    for (u=1, nused=0, maxobj=0; u<h->nused; u++) {
  if (h->obj[u].begin) {
      nused++;
      if (u>maxobj) maxobj = u;
  }
    }
    fprintf (stream, "%*s%-*s %u/%lu/", indent, "", fwidth,
       "Objects defined/allocated/max:",
       nused, (unsigned long)h->nalloc);
    fprintf (stream, nused ? "%u\n": "NA\n", maxobj);

    fprintf (stream, "%*s%-*s %lu\n", indent, "", fwidth,
       "Free space:",
       (unsigned long)(h->obj[0].size));

    for (u=1; u<h->nused; u++) {
  if (h->obj[u].begin) {
      sprintf (buf, "Object %u", u);
      fprintf (stream, "%*s%s\n", indent, "", buf);
      fprintf (stream, "%*s%-*s %d\n", indent+3, "", MIN(fwidth-3, 0),
         "Reference count:",
         h->obj[u].nrefs);
      fprintf (stream, "%*s%-*s %lu/%lu\n", indent+3, "",
         MIN(fwidth-3, 0),
         "Size of object body:",
         (unsigned long)(h->obj[u].size),
         (unsigned long)H5HG_ALIGN(h->obj[u].size));
      p = h->obj[u].begin + H5HG_SIZEOF_OBJHDR (f);
      for (j=0; j<h->obj[u].size; j+=16) {
    fprintf (stream, "%*s%04d: ", indent+6, "", j);
    for (k=0; k<16; k++) {
        if (8==k) fprintf (stream, " ");
        if (j+k<h->obj[u].size) {
      fprintf (stream, "%02x ", p[j+k]);
        } else {
      HDfputs("   ", stream);
        }
    }
    for (k=0; k<16 && j+k<h->obj[u].size; k++) {
        if (8==k) fprintf (stream, " ");
        HDfputc(p[j+k]>' ' && p[j+k]<='~' ? p[j+k] : '.', stream);
    }
    fprintf (stream, "\n");
      }
  }
    }

done:
    if (h && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, addr, h, FALSE) != SUCCEED)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to release object header");

    FUNC_LEAVE_NOAPI(ret_value);
}
