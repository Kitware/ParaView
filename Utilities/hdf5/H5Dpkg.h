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
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		Monday, April 14, 2003
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5D package.  Source files outside the H5D package should
 *		include H5Dprivate.h instead.
 */
#ifndef H5D_PACKAGE
#error "Do not include this file outside the H5D package!"
#endif

#ifndef _H5Dpkg_H
#define _H5Dpkg_H

/* Get package's private header */
#include "H5Dprivate.h"

/* Other private headers needed by this file */
#include "H5Gprivate.h"		/* Groups 			  	*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Sprivate.h"		/* Dataspace functions			*/
#include "H5Tprivate.h"		/* Datatype functions			*/

/**************************/
/* Package Private Macros */
/**************************/

/* The number of reserved IDs in dataset ID group */
#define H5D_RESERVED_ATOMS  0

/* Set the minimum object header size to create objects with */
#define H5D_MINHDR_SIZE 256

/****************************/
/* Package Private Typedefs */
/****************************/

/*
 * A dataset is the following struct.
 */
struct H5D_t {
    H5G_entry_t         ent;            /* cached object header stuff   */
    H5T_t              *type;           /* datatype of this dataset     */
    H5S_t              *space;          /* dataspace of this dataset    */
    hid_t               dcpl_id;        /* dataset creation property id */
    H5O_layout_t        layout;         /* data layout                  */
    /* Cache some frequently accessed values from the DCPL */
    H5O_efl_t           efl;            /* External file list information */
    H5D_alloc_time_t    alloc_time;     /* Dataset allocation time      */
    H5D_fill_time_t	fill_time;	/* Dataset fill value writing time */
    H5O_fill_t          fill;           /* Dataset fill value information */
};

/* Enumerated type for allocating dataset's storage */
typedef enum {
    H5D_ALLOC_CREATE,           /* Dataset is being created */
    H5D_ALLOC_OPEN,             /* Dataset is being opened */
    H5D_ALLOC_EXTEND,           /* Dataset's dataspace is being extended */
    H5D_ALLOC_WRITE             /* Dataset is being extended */
} H5D_time_alloc_t;

/******************************/
/* Package Private Prototypes */
/******************************/

H5_DLL herr_t H5D_alloc_storage (H5F_t *f, hid_t dxpl_id, H5D_t *dset, H5D_time_alloc_t time_alloc,
                        hbool_t update_time, hbool_t full_overwrite);
#endif /*_H5Dpkg_H*/
