/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5HLprivate.h
 *                      Jul 16 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5HLprivate_H
#define _H5HLprivate_H

#include "H5HLpublic.h"

/* Private headers needed by this file. */
#include "H5private.h"
#include "H5Fprivate.h"

/*
 * Feature: Define H5HL_DEBUG on the compiler command line if you want to
 *          diagnostic messages from this layer.
 */
#ifdef NDEBUG
#  undef H5HL_DEBUG
#endif

#define H5HL_MAGIC      "HEAP"          /*heap magic number                  */
#define H5HL_SIZEOF_MAGIC 4

#define H5HL_ALIGN(X)   (((X)+7)&(unsigned)(~0x07)) /*align on 8-byte boundary  */

#define H5HL_SIZEOF_HDR(F)                                                    \
    H5HL_ALIGN(H5HL_SIZEOF_MAGIC +      /*heap signature                */    \
               4 +                      /*reserved                      */    \
               H5F_SIZEOF_SIZE (F) +    /*data size                     */    \
               H5F_SIZEOF_SIZE (F) +    /*free list head                */    \
               H5F_SIZEOF_ADDR (F))     /*data address                  */

#define H5HL_SIZEOF_FREE(F)                                                   \
    H5HL_ALIGN(H5F_SIZEOF_SIZE (F) +    /*ptr to next free block        */    \
               H5F_SIZEOF_SIZE (F))     /*size of this free block       */

/*
 * Library prototypes...
 */
__DLL__ herr_t H5HL_create(H5F_t *f, size_t size_hint, haddr_t *addr/*out*/);
__DLL__ void *H5HL_read(H5F_t *f, haddr_t addr, size_t offset, size_t size,
                        void *buf);
__DLL__ const void *H5HL_peek(H5F_t *f, haddr_t addr, size_t offset);
__DLL__ size_t H5HL_insert(H5F_t *f, haddr_t addr, size_t size,
                           const void *buf);
__DLL__ herr_t H5HL_write(H5F_t *f, haddr_t addr, size_t offset, size_t size,
                          const void *buf);
__DLL__ herr_t H5HL_remove(H5F_t *f, haddr_t addr, size_t offset, size_t size);
__DLL__ herr_t H5HL_debug(H5F_t *f, haddr_t addr, FILE * stream, int indent,
                          int fwidth);
#endif
