/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Oname.c
 *                      Aug 12 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Object name message.
 *
 * Modifications:       
 *
 *-------------------------------------------------------------------------
 */
#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"

#define PABLO_MASK      H5O_name_mask

/* PRIVATE PROTOTYPES */
static void *H5O_name_decode(H5F_t *f, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_name_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_name_copy(const void *_mesg, void *_dest);
static size_t H5O_name_size(H5F_t *f, const void *_mesg);
static herr_t H5O_name_reset(void *_mesg);
static herr_t H5O_name_debug(H5F_t *f, const void *_mesg, FILE * stream,
                             int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_NAME[1] = {{
    H5O_NAME_ID,                /*message id number             */
    "name",                     /*message name for debugging    */
    sizeof(H5O_name_t),         /*native message size           */
    H5O_name_decode,            /*decode message                */
    H5O_name_encode,            /*encode message                */
    H5O_name_copy,              /*copy the native value         */
    H5O_name_size,              /*raw message size              */
    H5O_name_reset,             /*free internal memory          */
    NULL,                       /*free method                   */
    NULL,                       /*get share method              */
    NULL,                       /*set share method              */
    H5O_name_debug,             /*debug the message             */
}};

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT  NULL


/*-------------------------------------------------------------------------
 * Function:    H5O_name_decode
 *
 * Purpose:     Decode a name message and return a pointer to a new
 *              native message struct.
 *
 * Return:      Success:        Ptr to new message in native struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_name_decode(H5F_t UNUSED *f, const uint8_t *p,
                H5O_shared_t UNUSED *sh)
{
    H5O_name_t             *mesg;

    FUNC_ENTER(H5O_name_decode, NULL);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if (NULL==(mesg = H5MM_calloc(sizeof(H5O_name_t))) ||
        NULL==(mesg->s = H5MM_malloc (HDstrlen((const char*)p)+1))) {
        H5MM_xfree (mesg);
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    HDstrcpy(mesg->s, (const char*)p);

    FUNC_LEAVE(mesg);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_name_encode
 *
 * Purpose:     Encodes a name message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_name_encode(H5F_t UNUSED *f, uint8_t *p, const void *_mesg)
{
    const H5O_name_t       *mesg = (const H5O_name_t *) _mesg;

    FUNC_ENTER(H5O_name_encode, FAIL);

    /* check args */
    assert(f);
    assert(p);
    assert(mesg && mesg->s);

    /* encode */
    HDstrcpy((char*)p, mesg->s);
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_name_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_name_copy(const void *_mesg, void *_dest)
{
    const H5O_name_t       *mesg = (const H5O_name_t *) _mesg;
    H5O_name_t             *dest = (H5O_name_t *) _dest;

    FUNC_ENTER(H5O_name_copy, NULL);

    /* check args */
    assert(mesg);
    if (!dest && NULL==(dest = H5MM_calloc(sizeof(H5O_name_t)))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    
    /* copy */
    *dest = *mesg;
    dest->s = H5MM_xstrdup(mesg->s);

    FUNC_LEAVE((void *) dest);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_name_size
 *
 * Purpose:     Returns the size of the raw message in bytes not
 *              counting the message typ or size fields, but only the data
 *              fields.  This function doesn't take into account
 *              alignment.
 *
 * Return:      Success:        Message data size in bytes w/o alignment.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_name_size(H5F_t UNUSED *f, const void *_mesg)
{
    const H5O_name_t       *mesg = (const H5O_name_t *) _mesg;
    size_t                  size;

    FUNC_ENTER(H5O_name_size, 0);

    /* check args */
    assert(f);
    assert(mesg);

    size = mesg->s ? HDstrlen(mesg->s) + 1 : 0;
    FUNC_LEAVE(size);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_name_reset
 *
 * Purpose:     Frees internal pointers and resets the message to an
 *              initial state.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_name_reset(void *_mesg)
{
    H5O_name_t             *mesg = (H5O_name_t *) _mesg;

    FUNC_ENTER(H5O_name_reset, FAIL);

    /* check args */
    assert(mesg);

    /* reset */
    mesg->s = H5MM_xfree(mesg->s);

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_name_debug
 *
 * Purpose:     Prints debugging info for the message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_name_debug(H5F_t UNUSED *f, const void *_mesg, FILE *stream,
               int indent, int fwidth)
{
    const H5O_name_t    *mesg = (const H5O_name_t *)_mesg;

    FUNC_ENTER(H5O_name_debug, FAIL);

    /* check args */
    assert(f);
    assert(mesg);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    fprintf(stream, "%*s%-*s `%s'\n", indent, "", fwidth,
            "Name:",
            mesg->s);

    FUNC_LEAVE(SUCCEED);
}
