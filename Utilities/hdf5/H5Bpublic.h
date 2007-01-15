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
 * Created:             H5Bproto.h
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Public declarations for the H5B package.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Bpublic_H
#define _H5Bpublic_H

/* Public headers needed by this file */
#include "H5public.h"

/* B-tree IDs for various internal things. */
/* Not really a "public" symbol, but that should be OK -QAK */
/* Note - if more of these are added, any 'K' values (for internal or leaf
 * nodes) they use will need to be stored in the file somewhere. -QAK
 */
typedef enum H5B_subid_t {
    H5B_SNODE_ID   = 0,  /*B-tree is for symbol table nodes       */
    H5B_ISTORE_ID   = 1,  /*B-tree is for indexed object storage       */
    H5B_NUM_BTREE_ID            /* Number of B-tree key IDs (must be last)   */
} H5B_subid_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
