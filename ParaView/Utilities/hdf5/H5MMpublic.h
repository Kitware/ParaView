/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5MMproto.h
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Public declarations for the H5MM (memory management)
 *                      package.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5MMpublic_H
#define _H5MMpublic_H

/* Public headers needed by this file */
#include "H5public.h"

/* These typedefs are currently used for VL datatype allocation/freeing */
typedef void *(* H5MM_allocate_t)(size_t size,void *info);
typedef void (* H5MM_free_t)(void *mem, void *free_info);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
