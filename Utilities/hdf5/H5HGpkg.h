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
 *              only within the H5HG package. Source files outside the
 *              H5HG package should include H5HGprivate.h instead.
 */
#ifndef H5HG_PACKAGE
#error "Do not include this file outside the H5HG package!"
#endif

#ifndef _H5HGpkg_H
#define _H5HGpkg_H

/* Get package's private header */
#include "H5HGprivate.h"

/* Other private headers needed by this file */

/*****************************/
/* Package Private Variables */
/*****************************/

/* The cache subclass */
H5_DLLVAR const H5AC_class_t H5AC_GHEAP[1];

/**************************/
/* Package Private Macros */
/**************************/

/*
 * Pad all global heap messages to a multiple of eight bytes so we can load
 * the entire collection into memory and operate on it there.  Eight should
 * be sufficient for machines that have alignment constraints because our
 * largest data type is eight bytes.
 */
#define H5HG_ALIGNMENT  8
#define H5HG_ALIGN(X)  (H5HG_ALIGNMENT*(((X)+H5HG_ALIGNMENT-1)/        \
           H5HG_ALIGNMENT))
#define H5HG_ISALIGNED(X) ((X)==H5HG_ALIGN(X))

/*
 * The overhead associated with each object in the heap, always a multiple of
 * the alignment so that the stuff that follows the header is aligned.
 */
#define H5HG_SIZEOF_OBJHDR(f)                  \
    H5HG_ALIGN(2 +      /*object id number  */        \
         2 +      /*reference count  */        \
         4 +      /*reserved    */        \
         H5F_SIZEOF_SIZE(f))  /*object data size  */

/****************************/
/* Package Private Typedefs */
/****************************/

typedef struct H5HG_obj_t {
    int    nrefs;    /*reference count    */
    size_t    size;    /*total size of object    */
    uint8_t    *begin;    /*ptr to object into heap->chunk*/
} H5HG_obj_t;

struct H5HG_heap_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    haddr_t    addr;    /*collection address    */
    size_t    size;    /*total size of collection  */
    uint8_t    *chunk;    /*the collection, incl. header  */
    size_t    nalloc;    /*numb object slots allocated  */
    size_t    nused;    /*number of slots used    */
                                        /* If this value is >65535 then all indices */
                                        /* have been used at some time and the */
                                        /* correct new index should be searched for */
    H5HG_obj_t  *obj;    /*array of object descriptions  */
};

/******************************/
/* Package Private Prototypes */
/******************************/

#endif

