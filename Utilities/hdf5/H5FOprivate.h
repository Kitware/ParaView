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
 * This file contains library private information about the H5FO module
 */
#ifndef _H5FOprivate_H
#define _H5FOprivate_H

#ifdef LATER
#include "H5FOpublic.h"
#endif /* LATER */

/* Private headers needed by this file */
#include "H5private.h"
#include "H5TBprivate.h"	/* TBBTs	  		        */

/* Typedefs */

/* Typedef for open object cache */
typedef H5TB_TREE H5FO_t;       /* Currently, all open objects are stored in TBBT */

/* Macros */

/* Private routines */
H5_DLL herr_t H5FO_create(H5F_t *f);
H5_DLL hid_t H5FO_opened(const H5F_t *f, haddr_t addr);
H5_DLL herr_t H5FO_insert(H5F_t *f, haddr_t addr, hid_t id);
H5_DLL herr_t H5FO_delete(H5F_t *f, hid_t dxpl_id, haddr_t addr);
H5_DLL herr_t H5FO_mark(const H5F_t *f, haddr_t addr);
H5_DLL herr_t H5FO_dest(H5F_t *f);

#endif /* _H5FOprivate_H */

