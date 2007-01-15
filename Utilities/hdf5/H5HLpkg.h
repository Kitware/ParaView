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
 * Programmer: Quincey Koziol <koziol@ncsa.uiuc.edu>
 *             Wednesday, July 9, 2003
 *
 * Purpose:     This file contains declarations which are visible
 *              only within the H5HL package. Source files outside the
 *              H5HL package should include H5HLprivate.h instead.
 */
#ifndef H5HL_PACKAGE
#error "Do not include this file outside the H5HL package!"
#endif

#ifndef _H5HLpkg_H
#define _H5HLpkg_H

/* Get package's private header */
#include "H5HLprivate.h"

/* Other private headers needed by this file */

/*****************************/
/* Package Private Variables */
/*****************************/

/* The cache subclass */
H5_DLLVAR const H5AC_class_t H5AC_LHEAP[1];

/**************************/
/* Package Private Macros */
/**************************/

#define H5HL_SIZEOF_HDR(F)                  \
    H5HL_ALIGN(H5HL_SIZEOF_MAGIC +  /*heap signature    */    \
         4 +      /*reserved      */    \
         H5F_SIZEOF_SIZE (F) +  /*data size      */    \
         H5F_SIZEOF_SIZE (F) +  /*free list head    */    \
         H5F_SIZEOF_ADDR (F))  /*data address      */

/****************************/
/* Package Private Typedefs */
/****************************/

typedef struct H5HL_free_t {
    size_t    offset;    /*offset of free block    */
    size_t    size;    /*size of free block    */
    struct H5HL_free_t  *prev;    /*previous entry in free list  */
    struct H5HL_free_t  *next;    /*next entry in free list  */
} H5HL_free_t;

struct H5HL_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    haddr_t        addr;  /*address of data    */
    size_t        disk_alloc;  /*data bytes allocated on disk  */
    size_t        mem_alloc;  /*data bytes allocated in mem  */
    size_t        disk_resrv; /*data bytes "reserved" on disk */
    uint8_t       *chunk;  /*the chunk, including header  */
    H5HL_free_t       *freelist;  /*the free list      */
};

/******************************/
/* Package Private Prototypes */
/******************************/

#endif


