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
 * Created:             H5MFprivate.h
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Private header file for file memory management.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5MFprivate_H
#define _H5MFprivate_H

/* Private headers needed by this file */
#include "H5private.h"
#include "H5Fprivate.h"
#include "H5FDprivate.h"	/*file driver				  */

/*
 * Feature: Define H5MF_DEBUG on the compiler command line if you want to
 *	    see diagnostics from this layer.
 */
#ifdef NDEBUG
#  undef H5MF_DEBUG
#endif

/*
 * Library prototypes...
 */
H5_DLL haddr_t H5MF_alloc(H5F_t *f, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
H5_DLL herr_t H5MF_xfree(H5F_t *f, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
			  hsize_t size);
H5_DLL haddr_t H5MF_realloc(H5F_t *f, H5FD_mem_t type, hid_t dxpl_id, haddr_t old_addr,
			     hsize_t old_size, hsize_t new_size);

#endif
