/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
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
#include "H5FDprivate.h"        /*file driver                             */

/*
 * Feature: Define H5MF_DEBUG on the compiler command line if you want to
 *          see diagnostics from this layer.
 */
#ifdef NDEBUG
#  undef H5MF_DEBUG
#endif

/*
 * Library prototypes...
 */
__DLL__ haddr_t H5MF_alloc(H5F_t *f, H5FD_mem_t type, hsize_t size);
__DLL__ herr_t H5MF_xfree(H5F_t *f, H5FD_mem_t type, haddr_t addr,
                          hsize_t size);
__DLL__ haddr_t H5MF_realloc(H5F_t *f, H5FD_mem_t type, haddr_t old_addr,
                             hsize_t old_size, hsize_t new_size);

#endif
