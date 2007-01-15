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

#define H5O_PACKAGE  /*suppress error about including H5Opkg    */
#define H5S_PACKAGE    /*prevent warning from including H5Spkg.h */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"  /*Free Lists    */
#include "H5Gprivate.h"
#include "H5MMprivate.h"
#include "H5Opkg.h"             /* Object header functions                  */
#include "H5Spkg.h"


/* PRIVATE PROTOTYPES */
static void *H5O_sdspace_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_sdspace_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_sdspace_copy(const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_sdspace_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_sdspace_reset(void *_mesg);
static herr_t H5O_sdspace_free (void *_mesg);
static herr_t H5O_sdspace_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
        FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_SDSPACE[1] = {{
    H5O_SDSPACE_ID,        /* message id number          */
    "simple_dspace",        /* message name for debugging       */
    sizeof(H5S_extent_t),     /* native message size          */
    H5O_sdspace_decode,        /* decode message      */
    H5O_sdspace_encode,        /* encode message      */
    H5O_sdspace_copy,        /* copy the native value    */
    H5O_sdspace_size,        /* size of symbol table entry        */
    H5O_sdspace_reset,        /* default reset method          */
    H5O_sdspace_free,    /* free method        */
    NULL,            /* file delete method    */
    NULL,      /* link method      */
    NULL,          /* get share method      */
    NULL,       /* set share method      */
    H5O_sdspace_debug,          /* debug the message          */
}};

#define H5O_SDSPACE_VERSION  1

/* Declare external the free list for H5S_extent_t's */
H5FL_EXTERN(H5S_extent_t);

/* Declare external the free list for hsize_t arrays */
H5FL_ARR_EXTERN(hsize_t);


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_decode
 PURPOSE
    Decode a simple dimensionality message and return a pointer to a memory
  struct with the decoded information
 USAGE
    void *H5O_sdspace_decode(f, raw_size, p)
  H5F_t *f;    IN: pointer to the HDF5 file struct
  size_t raw_size;  IN: size of the raw information buffer
  const uint8 *p;    IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
  This function decodes the "raw" disk form of a simple dimensionality
    message into a struct in memory native format.  The struct is allocated
    within this function using malloc() and is returned to the caller.

 MODIFICATIONS
  Robb Matzke, 1998-04-09
  The current and maximum dimensions are now H5F_SIZEOF_SIZET bytes
  instead of just four bytes.

    Robb Matzke, 1998-07-20
        Added a version number and reformatted the message for aligment.
--------------------------------------------------------------------------*/
static void *
H5O_sdspace_decode(H5F_t *f, hid_t UNUSED dxpl_id, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5S_extent_t  *sdim = NULL;/* New extent dimensionality structure */
    void    *ret_value;
    unsigned    i;    /* local counting variable */
    unsigned    flags, version;

    FUNC_ENTER_NOAPI_NOINIT(H5O_sdspace_decode);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if ((sdim = H5FL_CALLOC(H5S_extent_t)) != NULL) {
        /* Check version */
        version = *p++;
        if (version!=H5O_SDSPACE_VERSION)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "wrong version number in data space message");

        /* Get rank */
        sdim->rank = *p++;
        if (sdim->rank>H5S_MAX_RANK)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "simple data space dimensionality is too large");

        /* Get dataspace flags for later */
        flags = *p++;

        /* Set the dataspace type to be simple or scalar as appropriate */
        if(sdim->rank>0)
            sdim->type = H5S_SIMPLE;
        else
            sdim->type = H5S_SCALAR;

        p += 5; /*reserved*/

        if (sdim->rank > 0) {
            if (NULL==(sdim->size=H5FL_ARR_MALLOC(hsize_t,sdim->rank)))
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
            for (i = 0; i < sdim->rank; i++)
                H5F_DECODE_LENGTH (f, p, sdim->size[i]);
            if (flags & H5S_VALID_MAX) {
                if (NULL==(sdim->max=H5FL_ARR_MALLOC(hsize_t,sdim->rank)))
                    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
                for (i = 0; i < sdim->rank; i++)
                    H5F_DECODE_LENGTH (f, p, sdim->max[i]);
            }
        }

        /* Compute the number of elements in the extent */
        for(i=0, sdim->nelem=1; i<sdim->rank; i++)
            sdim->nelem*=sdim->size[i];
    }

    /* Set return value */
    ret_value = (void*)sdim;  /*success*/

done:
    if (!ret_value && sdim) {
        H5S_extent_release(sdim);
        H5FL_FREE(H5S_extent_t,sdim);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_encode
 PURPOSE
    Encode a simple dimensionality message
 USAGE
    herr_t H5O_sdspace_encode(f, raw_size, p, mesg)
  H5F_t *f;          IN: pointer to the HDF5 file struct
  size_t raw_size;  IN: size of the raw information buffer
  const uint8 *p;    IN: the raw information buffer
  const void *mesg;  IN: Pointer to the extent dimensionality struct
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
  This function encodes the native memory form of the simple
    dimensionality message in the "raw" disk form.

 MODIFICATIONS
  Robb Matzke, 1998-04-09
  The current and maximum dimensions are now H5F_SIZEOF_SIZET bytes
  instead of just four bytes.

    Robb Matzke, 1998-07-20
        Added a version number and reformatted the message for aligment.
--------------------------------------------------------------------------*/
static herr_t
H5O_sdspace_encode(H5F_t *f, uint8_t *p, const void *mesg)
{
    const H5S_extent_t  *sdim = (const H5S_extent_t *) mesg;
    unsigned    u;  /* Local counting variable */
    unsigned    flags = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_sdspace_encode);

    /* check args */
    assert(f);
    assert(p);
    assert(sdim);

    /* set flags */
    if (sdim->max)
        flags |= H5S_VALID_MAX;

    /* encode */
    *p++ = H5O_SDSPACE_VERSION;
    *p++ = sdim->rank;
    *p++ = flags;
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/

    if (sdim->rank > 0) {
        for (u = 0; u < sdim->rank; u++)
            H5F_ENCODE_LENGTH (f, p, sdim->size[u]);
        if (flags & H5S_VALID_MAX) {
            for (u = 0; u < sdim->rank; u++)
                H5F_ENCODE_LENGTH (f, p, sdim->max[u]);
        }
    }

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_copy
 PURPOSE
    Copies a message from MESG to DEST, allocating DEST if necessary.
 USAGE
    void *H5O_sdspace_copy(mesg, dest)
  const void *mesg;  IN: Pointer to the source extent dimensionality struct
  const void *dest;  IN: Pointer to the destination extent dimensionality struct
 RETURNS
    Pointer to DEST on success, NULL on failure
 DESCRIPTION
  This function copies a native (memory) simple dimensionality message,
    allocating the destination structure if necessary.
 MODIFICATIONS
    Raymond Lu
    April 8, 2004
    Changed operation on H5S_simple_t to H5S_extent_t.

--------------------------------------------------------------------------*/
static void *
H5O_sdspace_copy(const void *mesg, void *dest, unsigned UNUSED update_flags)
{
    const H5S_extent_t     *src = (const H5S_extent_t *) mesg;
    H5S_extent_t     *dst = (H5S_extent_t *) dest;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_sdspace_copy);

    /* check args */
    assert(src);
    if (!dst && NULL==(dst = H5FL_MALLOC(H5S_extent_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy extent information */
    if(H5S_extent_copy(dst,src)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy extent");

    /* Set return value */
    ret_value=dst;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_size
 PURPOSE
    Return the raw message size in bytes
 USAGE
    void *H5O_sdspace_size(f, mesg)
  H5F_t *f;    IN: pointer to the HDF5 file struct
  const void *mesg;  IN: Pointer to the source extent dimensionality struct
 RETURNS
    Size of message on success, zero on failure
 DESCRIPTION
  This function returns the size of the raw simple dimensionality message on
    success.  (Not counting the message type or size fields, only the data
    portion of the message).  It doesn't take into account alignment.

 MODIFICATIONS
  Robb Matzke, 1998-04-09
  The current and maximum dimensions are now H5F_SIZEOF_SIZET bytes
  instead of just four bytes.
--------------------------------------------------------------------------*/
static size_t
H5O_sdspace_size(const H5F_t *f, const void *mesg)
{
    const H5S_extent_t     *space = (const H5S_extent_t *) mesg;

    /*
     * All dimensionality messages are at least 8 bytes long.
     */
    size_t        ret_value = 8;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_sdspace_size);

    /* add in the dimension sizes */
    ret_value += space->rank * H5F_SIZEOF_SIZE (f);

    /* add in the space for the maximum dimensions, if they are present */
    ret_value += space->max ? space->rank * H5F_SIZEOF_SIZE (f) : 0;

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_sdspace_reset
 *
 * Purpose:  Frees the inside of a dataspace message and resets it to some
 *    initial value.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_sdspace_reset(void *_mesg)
{
    H5S_extent_t  *mesg = (H5S_extent_t*)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_sdspace_reset);

    H5S_extent_release(mesg);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_sdsdpace_free
 *
 * Purpose:  Free's the message
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 30, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_sdspace_free (void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_sdspace_free);

    assert (mesg);

    H5FL_FREE(H5S_extent_t,mesg);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_debug
 PURPOSE
    Prints debugging information for a simple dimensionality message
 USAGE
    void *H5O_sdspace_debug(f, mesg, stream, indent, fwidth)
  H5F_t *f;          IN: pointer to the HDF5 file struct
  const void *mesg;  IN: Pointer to the source extent dimensionality struct
  FILE *stream;    IN: Pointer to the stream for output data
  int indent;    IN: Amount to indent information by
  int fwidth;    IN: Field width (?)
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
  This function prints debugging output to the stream passed as a
    parameter.
--------------------------------------------------------------------------*/
static herr_t
H5O_sdspace_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *mesg,
      FILE * stream, int indent, int fwidth)
{
    const H5S_extent_t     *sdim = (const H5S_extent_t *) mesg;
    unsigned        u;  /* local counting variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_sdspace_debug);

    /* check args */
    assert(f);
    assert(sdim);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
      "Rank:",
      (unsigned long) (sdim->rank));

    if(sdim->rank>0) {
        HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Size:");
        for (u = 0; u < sdim->rank; u++)
            HDfprintf (stream, "%s%Hu", u?", ":"", sdim->size[u]);
        HDfprintf (stream, "}\n");

        HDfprintf(stream, "%*s%-*s ", indent, "", fwidth, "Dim Max:");
        if (sdim->max) {
            HDfprintf (stream, "{");
            for (u = 0; u < sdim->rank; u++) {
                if (H5S_UNLIMITED==sdim->max[u]) {
                    HDfprintf (stream, "%sINF", u?", ":"");
                } else {
                    HDfprintf (stream, "%s%Hu", u?", ":"", sdim->max[u]);
                }
            }
            HDfprintf (stream, "}\n");
        } else {
            HDfprintf (stream, "CONSTANT\n");
        }
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED);
}
