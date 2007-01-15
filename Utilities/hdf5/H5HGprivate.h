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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Friday, March 27, 1998
 */
#ifndef _H5HGprivate_H
#define _H5HGprivate_H

/* Include package's public header */
#include "H5HGpublic.h"

/* Private headers needed by this file. */
#include "H5Fprivate.h"    /* File access        */

/*
 * Each collection has a magic number for some redundancy.
 */
#define H5HG_MAGIC  "GCOL"
#define H5HG_SIZEOF_MAGIC 4

/* Information to locate object in global heap */
typedef struct H5HG_t {
    haddr_t    addr;    /*address of collection    */
    size_t    idx;    /*object ID within collection  */
} H5HG_t;

/* Typedef for heap in memory (defined in H5HGpkg.h) */
typedef struct H5HG_heap_t H5HG_heap_t;

H5_DLL herr_t H5HG_insert(H5F_t *f, hid_t dxpl_id, size_t size, void *obj,
         H5HG_t *hobj/*out*/);
H5_DLL void *H5HG_read(H5F_t *f, hid_t dxpl_id, H5HG_t *hobj, void *object);
H5_DLL int H5HG_link(H5F_t *f, hid_t dxpl_id, const H5HG_t *hobj, int adjust);
H5_DLL herr_t H5HG_remove(H5F_t *f, hid_t dxpl_id, H5HG_t *hobj);

/* Debugging functions */
H5_DLL herr_t H5HG_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent,
        int fwidth);

#endif
