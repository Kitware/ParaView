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
 *    Thursday, May 15, 2003
 *
 * Purpose:  This file contains declarations which are visible only within
 *    the H5B package.  Source files outside the H5B package should
 *    include H5Bprivate.h instead.
 */
#ifndef H5B_PACKAGE
#error "Do not include this file outside the H5B package!"
#endif

#ifndef _H5Bpkg_H
#define _H5Bpkg_H

/* Get package's private header */
#include "H5Bprivate.h"

/* Other private headers needed by this file */
#include "H5RCprivate.h"  /* Reference counted object functions    */

/**************************/
/* Package Private Macros */
/**************************/

/****************************/
/* Package Private Typedefs */
/****************************/

/*
 * The B-tree node as stored in memory...
 */
struct H5B_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    H5RC_t    *rc_shared;  /*ref-counted shared info       */
    unsigned    level;    /*node level           */
    unsigned    nchildren;  /*number of child pointers       */
    haddr_t    left;    /*address of left sibling       */
    haddr_t    right;    /*address of right sibling       */
    uint8_t    *native;  /*array of keys in native format     */
    haddr_t    *child;    /*2k child pointers         */
};

/******************************/
/* Package Private Prototypes */
/******************************/

#endif /*_H5Bpkg_H*/
