/*
 * Copyright (C) 1997 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, October  8, 1997
 *
 * Purpose:     Messages related to data layout.
 */
#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5MMprivate.h"
#include "H5Oprivate.h"

/* PRIVATE PROTOTYPES */
static void *H5O_layout_decode(H5F_t *f, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_layout_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_layout_copy(const void *_mesg, void *_dest);
static size_t H5O_layout_size(H5F_t *f, const void *_mesg);
static herr_t H5O_layout_free (void *_mesg);
static herr_t H5O_layout_debug(H5F_t *f, const void *_mesg, FILE * stream,
                               int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_LAYOUT[1] = {{
    H5O_LAYOUT_ID,              /*message id number             */
    "layout",                   /*message name for debugging    */
    sizeof(H5O_layout_t),       /*native message size           */
    H5O_layout_decode,          /*decode message                */
    H5O_layout_encode,          /*encode message                */
    H5O_layout_copy,            /*copy the native value         */
    H5O_layout_size,            /*size of message on disk       */
    NULL,                       /*reset method                  */
    H5O_layout_free,            /*free the struct         */
    NULL,                       /*get share method              */
    NULL,                       /*set share method              */
    H5O_layout_debug,           /*debug the message             */
}};

#define H5O_LAYOUT_VERSION      1

/* Interface initialization */
#define PABLO_MASK      H5O_layout_mask
static int interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

/* Declare a free list to manage the H5O_layout_t struct */
H5FL_DEFINE(H5O_layout_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_decode
 *
 * Purpose:     Decode an data layout message and return a pointer to a
 *              new one created with malloc().
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Rearranged the message to add a version number at the beginning.
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_decode(H5F_t *f, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5O_layout_t           *mesg = NULL;
    int                    version;
    unsigned                   u;

    FUNC_ENTER(H5O_layout_decode, NULL);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if (NULL==(mesg = H5FL_ALLOC(H5O_layout_t,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    /* Version */
    version = *p++;
    if (version!=H5O_LAYOUT_VERSION) {
        HRETURN_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL,
                      "bad version number for layout message");
    }

    /* Dimensionality */
    mesg->ndims = *p++;
    if (mesg->ndims>H5O_LAYOUT_NDIMS) {
        H5FL_FREE(H5O_layout_t,mesg);
        HRETURN_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL,
                      "dimensionality is too large");
    }

    /* Layout class */
    mesg->type = *p++;
    assert(H5D_CONTIGUOUS == mesg->type || H5D_CHUNKED == mesg->type);

    /* Reserved bytes */
    p += 5;

    /* Address */
    H5F_addr_decode(f, &p, &(mesg->addr));

    /* Read the size */
    for (u = 0; u < mesg->ndims; u++) {
        UINT32DECODE(p, mesg->dim[u]);
    }

    FUNC_LEAVE(mesg);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_layout_encode
 *
 * Purpose:     Encodes a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Rearranged the message to add a version number at the beginning.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned                     u;

    FUNC_ENTER(H5O_layout_encode, FAIL);

    /* check args */
    assert(f);
    assert(mesg);
    assert(mesg->ndims > 0 && mesg->ndims <= H5O_LAYOUT_NDIMS);
    assert(p);

    /* Version */
    *p++ = H5O_LAYOUT_VERSION;

    /* number of dimensions */
    *p++ = mesg->ndims;

    /* layout class */
    *p++ = mesg->type;

    /* reserved bytes should be zero */
    for (u=0; u<5; u++)
        *p++ = 0;

    /* data or B-tree address */
    H5F_addr_encode(f, &p, mesg->addr);

    /* dimension size */
    for (u = 0; u < mesg->ndims; u++) {
        UINT32ENCODE(p, mesg->dim[u]);
    }

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_layout_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_copy(const void *_mesg, void *_dest)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    H5O_layout_t           *dest = (H5O_layout_t *) _dest;

    FUNC_ENTER(H5O_layout_copy, NULL);

    /* check args */
    assert(mesg);
    if (!dest && NULL==(dest=H5FL_ALLOC(H5O_layout_t,0))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    
    /* copy */
    *dest = *mesg;

    FUNC_LEAVE((void *) dest);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_layout_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting the
 *              message type or size fields, but only the data fields.  This
 *              function doesn't take into account message alignment.
 *
 * Return:      Success:        Message data size in bytes
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_layout_size(H5F_t *f, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    size_t                  ret_value = 0;

    FUNC_ENTER(H5O_layout_size, 0);

    /* check args */
    assert(f);
    assert(mesg);
    assert(mesg->ndims > 0 && mesg->ndims <= H5O_LAYOUT_NDIMS);

    ret_value = H5F_SIZEOF_ADDR(f) +    /* B-tree address       */
        1 +                     /* max dimension index  */
        1 +                     /* layout class number  */
        6 +                     /* reserved bytes       */
        mesg->ndims * 4;        /* alignment            */

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_free
 *
 * Purpose:     Free's the message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 11, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_free (void *mesg)
{
    FUNC_ENTER (H5O_layout_free, FAIL);

    assert (mesg);

    H5FL_FREE(H5O_layout_t,mesg);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_debug
 *
 * Purpose:     Prints debugging info for a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_debug(H5F_t UNUSED *f, const void *_mesg, FILE * stream,
                 int indent, int fwidth)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned                    u;

    FUNC_ENTER(H5O_layout_debug, FAIL);

    /* check args */
    assert(f);
    assert(mesg);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              H5D_CHUNKED == mesg->type ? "B-tree address:" : "Data address:",
              mesg->addr);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
              "Number of dimensions:",
              (unsigned long) (mesg->ndims));

    /* Size */
    HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Size:");
    for (u = 0; u < mesg->ndims; u++) {
        HDfprintf(stream, "%s%lu", u ? ", " : "",
                  (unsigned long) (mesg->dim[u]));
    }
    HDfprintf(stream, "}\n");

    FUNC_LEAVE(SUCCEED);
}
