/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5MMprivate.h
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Private header for memory management.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5MMprivate_H
#define _H5MMprivate_h

#include "H5MMpublic.h"

/* Private headers needed by this file */
#include "H5private.h"

#define H5MM_malloc(Z)  HDmalloc(MAX(1,Z))
#define H5MM_calloc(Z)  HDcalloc(1,MAX(1,Z))

/*
 * Library prototypes...
 */
__DLL__ void *H5MM_realloc(void *mem, size_t size);
__DLL__ char *H5MM_xstrdup(const char *s);
__DLL__ char *H5MM_strdup(const char *s);
__DLL__ void *H5MM_xfree(void *mem);

#endif
