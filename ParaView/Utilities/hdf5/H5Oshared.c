/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, April  1, 1998
 *
 * Purpose:     Functions that operate on a shared message.  The shared
 *              message doesn't ever actually appear in the object header as
 *              a normal message.  Instead, if a message is shared, the
 *              H5O_FLAG_SHARED bit is set and the message body is that
 *              defined here for H5O_SHARED.  The message ID is the ID of the
 *              pointed-to message and the pointed-to message is stored in
 *              the global heap.
 */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"

static void *H5O_shared_decode (H5F_t*, const uint8_t*, H5O_shared_t *sh);
static herr_t H5O_shared_encode (H5F_t*, uint8_t*, const void*);
static size_t H5O_shared_size (H5F_t*, const void*);
static herr_t H5O_shared_debug (H5F_t*, const void*, FILE*, int, int);

/* This message derives from H5O */
const H5O_class_t H5O_SHARED[1] = {{
    H5O_SHARED_ID,              /*message id number                     */
    "shared",                   /*message name for debugging            */
    sizeof(H5O_shared_t),       /*native message size                   */
    H5O_shared_decode,          /*decode method                         */
    H5O_shared_encode,          /*encode method                         */
    NULL,                       /*no copy method                        */
    H5O_shared_size,            /*size method                           */
    NULL,                       /*no reset method                       */
    NULL,                       /*no free method                        */
    NULL,                       /*get share method                      */
    NULL,                       /*set share method                      */
    H5O_shared_debug,           /*debug method                          */
}};

#define H5O_SHARED_VERSION      1

/* Interface initialization */
#define PABLO_MASK      H5O_shared_mask
static int interface_initialize_g = 0;
#define INTERFACE_INIT  NULL


/*-------------------------------------------------------------------------
 * Function:    H5O_shared_decode
 *
 * Purpose:     Decodes a shared object message and returns it.
 *
 * Return:      Success:        Ptr to a new shared object message.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Added a version number to the beginning of the message.
 *-------------------------------------------------------------------------
 */
static void *
H5O_shared_decode (H5F_t *f, const uint8_t *buf, H5O_shared_t UNUSED *sh)
{
    H5O_shared_t        *mesg;
    unsigned            flags, version;
    
    FUNC_ENTER (H5O_shared_decode, NULL);

    /* Check args */
    assert (f);
    assert (buf);
    assert (!sh);

    /* Decode */
    if (NULL==(mesg = H5MM_calloc (sizeof *mesg))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    /* Version */
    version = *buf++;
    if (version!=H5O_SHARED_VERSION) {
        HRETURN_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL,
                      "bad version number for shared object message");
    }

    /* Flags */
    flags = *buf++;
    mesg->in_gh = (flags & 0x01);

    /* Reserved */
    buf += 6;

    /* Body */
    if (mesg->in_gh) {
        H5F_addr_decode (f, &buf, &(mesg->u.gh.addr));
        INT32DECODE (buf, mesg->u.gh.idx);
    } else {
        H5G_ent_decode (f, &buf, &(mesg->u.ent));
    }

    FUNC_LEAVE (mesg);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_shared_encode
 *
 * Purpose:     Encodes message _MESG into buffer BUF.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Added a version number to the beginning of the message.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_shared_encode (H5F_t *f, uint8_t *buf/*out*/, const void *_mesg)
{
    const H5O_shared_t  *mesg = (const H5O_shared_t *)_mesg;
    unsigned            flags;
    
    FUNC_ENTER (H5O_shared_encode, FAIL);

    /* Check args */
    assert (f);
    assert (buf);
    assert (mesg);

    /* Encode */
    *buf++ = H5O_SHARED_VERSION;
    flags = mesg->in_gh ? 0x01 : 0x00;
    *buf++ = flags;
    *buf++ = 0; /*reserved 1*/
    *buf++ = 0; /*reserved 2*/
    *buf++ = 0; /*reserved 3*/
    *buf++ = 0; /*reserved 4*/
    *buf++ = 0; /*reserved 5*/
    *buf++ = 0; /*reserved 6*/

    if (mesg->in_gh) {
        H5F_addr_encode (f, &buf, mesg->u.gh.addr);
        INT32ENCODE (buf, mesg->u.gh.idx);
    } else {
        H5G_ent_encode (f, &buf, &(mesg->u.ent));
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_shared_size
 *
 * Purpose:     Returns the length of a shared object message.
 *
 * Return:      Success:        Length
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_shared_size (H5F_t *f, const void UNUSED *_mesg)
{
    size_t      size;
    
    FUNC_ENTER (H5O_shared_size, 0);

    size = 1 +                          /*the flags field               */
           7 +                          /*reserved                      */
           MAX (H5F_SIZEOF_ADDR(f)+4,   /*sharing via global heap       */
                H5G_SIZEOF_ENTRY(f));   /*sharing by another obj hdr    */

    FUNC_LEAVE (size);
    _mesg = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5O_shared_debug
 *
 * Purpose:     Prints debugging info for the message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_shared_debug (H5F_t UNUSED *f, const void *_mesg,
                  FILE *stream, int indent, int fwidth)
{
    const H5O_shared_t  *mesg = (const H5O_shared_t *)_mesg;

    FUNC_ENTER (H5O_shared_debug, FAIL);

    /* Check args */
    assert (f);
    assert (mesg);
    assert (stream);
    assert (indent>=0);
    assert (fwidth>=0);

    if (mesg->in_gh) {
        HDfprintf (stream, "%*s%-*s %s\n", indent, "", fwidth,
                   "Sharing method",
                   "Global heap");
        HDfprintf (stream, "%*s%-*s %a\n", indent, "", fwidth,
                   "Collection address:",
                   mesg->u.gh.addr);
        HDfprintf (stream, "%*s%-*s %d\n", indent, "", fwidth,
                   "Object ID within collection:",
                   mesg->u.gh.idx);
    } else {
        HDfprintf (stream, "%*s%-*s %s\n", indent, "", fwidth,
                   "Sharing method",
                   "Obj Hdr");
        H5G_ent_debug (f, &(mesg->u.ent), stream, indent, fwidth,
                       HADDR_UNDEF);
    }
    
    FUNC_LEAVE (SUCCEED);
}
