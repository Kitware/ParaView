/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Ocont.c
 *                      Aug  6 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             The object header continuation message.  This
 *                      message is only generated and read from within
 *                      the H5O package.  Therefore, do not change
 *                      any definitions in this file!
 *
 * Modifications:       
 *
 *-------------------------------------------------------------------------
 */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"

#define PABLO_MASK      H5O_cont_mask

/* PRIVATE PROTOTYPES */
static void *H5O_cont_decode(H5F_t *f, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_cont_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static herr_t H5O_cont_debug(H5F_t *f, const void *_mesg, FILE * stream,
                             int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_CONT[1] = {{
    H5O_CONT_ID,                /*message id number             */
    "hdr continuation",         /*message name for debugging    */
    sizeof(H5O_cont_t),         /*native message size           */
    H5O_cont_decode,            /*decode message                */
    H5O_cont_encode,            /*encode message                */
    NULL,                       /*no copy method                */
    NULL,                       /*no size method                */
    NULL,                       /*default reset method          */
    NULL,                           /* default free method                      */
    NULL,                       /*get share method              */
    NULL,                       /*set share method              */
    H5O_cont_debug,             /*debugging                     */
}};

/* Interface initialization */
static int             interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

/*-------------------------------------------------------------------------
 * Function:    H5O_cont_decode
 *
 * Purpose:     Decode the raw header continuation message.
 *
 * Return:      Success:        Ptr to the new native message
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_cont_decode(H5F_t *f, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5O_cont_t             *cont = NULL;

    FUNC_ENTER(H5O_cont_decode, NULL);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if (NULL==(cont = H5MM_calloc(sizeof(H5O_cont_t)))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    H5F_addr_decode(f, &p, &(cont->addr));
    H5F_DECODE_LENGTH(f, p, cont->size);

    FUNC_LEAVE((void *) cont);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_cont_encode
 *
 * Purpose:     Encodes a continuation message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  7 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_cont_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5O_cont_t       *cont = (const H5O_cont_t *) _mesg;

    FUNC_ENTER(H5O_cont_encode, FAIL);

    /* check args */
    assert(f);
    assert(p);
    assert(cont);

    /* encode */
    H5F_addr_encode(f, &p, cont->addr);
    H5F_ENCODE_LENGTH(f, p, cont->size);

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_cont_debug
 *
 * Purpose:     Prints debugging info.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_cont_debug(H5F_t UNUSED *f, const void *_mesg, FILE * stream,
               int indent, int fwidth)
{
    const H5O_cont_t       *cont = (const H5O_cont_t *) _mesg;

    FUNC_ENTER(H5O_cont_debug, FAIL);

    /* check args */
    assert(f);
    assert(cont);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              "Continuation address:", cont->addr);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
              "Continuation size in bytes:",
              (unsigned long) (cont->size));
    HDfprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
              "Points to chunk number:",
              (int) (cont->chunkno));

    FUNC_LEAVE(SUCCEED);
}
