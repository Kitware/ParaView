/****************************************************************************
* NCSA HDF                                                                 *
* Software Development Group                                               *
* National Center for Supercomputing Applications                          *
* University of Illinois at Urbana-Champaign                               *
* 605 E. Springfield, Champaign IL 61820                                   *
*                                                                          *
* For conditions of distribution and use, see the accompanying             *
* hdf/COPYING file.                                                        *
*                                                                          *
****************************************************************************/

/* Id */

#define H5S_PACKAGE             /*prevent warning from including H5Spkg.h */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5Gprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"
#include "H5Spkg.h"

#define PABLO_MASK      H5O_sdspace_mask

/* PRIVATE PROTOTYPES */
static void *H5O_sdspace_decode(H5F_t *f, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_sdspace_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_sdspace_copy(const void *_mesg, void *_dest);
static size_t H5O_sdspace_size(H5F_t *f, const void *_mesg);
static herr_t H5O_sdspace_reset(void *_mesg);
static herr_t H5O_sdspace_free (void *_mesg);
static herr_t H5O_sdspace_debug(H5F_t *f, const void *_mesg,
                                FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_SDSPACE[1] = {{
    H5O_SDSPACE_ID,             /* message id number                    */
    "simple_dspace",            /* message name for debugging           */
    sizeof(H5S_simple_t),       /* native message size                  */
    H5O_sdspace_decode,         /* decode message                       */
    H5O_sdspace_encode,         /* encode message                       */
    H5O_sdspace_copy,           /* copy the native value                */
    H5O_sdspace_size,           /* size of symbol table entry           */
    H5O_sdspace_reset,          /* default reset method                 */
    H5O_sdspace_free,               /* free method                      */
    NULL,                       /* get share method                     */
    NULL,                       /* set share method                     */
    H5O_sdspace_debug,          /* debug the message                    */
}};

#define H5O_SDSPACE_VERSION     1

/* Is the interface initialized? */
static int interface_initialize_g = 0;
#define INTERFACE_INIT NULL

/* Declare external the free list for H5S_simple_t's */
H5FL_EXTERN(H5S_simple_t);

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
        H5F_t *f;         IN: pointer to the HDF5 file struct
        size_t raw_size;        IN: size of the raw information buffer
        const uint8 *p;         IN: the raw information buffer
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
H5O_sdspace_decode(H5F_t *f, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5S_simple_t        *sdim = NULL;/* New simple dimensionality structure */
    void                *ret_value = NULL;
    unsigned            u;              /* local counting variable */
    unsigned            flags, version;
    
    FUNC_ENTER(H5O_sdspace_decode, NULL);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if ((sdim = H5FL_ALLOC(H5S_simple_t,1)) != NULL) {
        version = *p++;
        if (version!=H5O_SDSPACE_VERSION) {
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL,
                "wrong version number in data space message");
        }
        sdim->rank = *p++;
        if (sdim->rank>H5S_MAX_RANK) {
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL,
                "simple data space dimensionality is too large");
        }
        flags = *p++;
        p += 5; /*reserved*/

        if (sdim->rank > 0) {
            if (NULL==(sdim->size=H5FL_ARR_ALLOC(hsize_t,(hsize_t)sdim->rank,0))) {
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
            }
            for (u = 0; u < sdim->rank; u++) {
                H5F_DECODE_LENGTH (f, p, sdim->size[u]);
            }
            if (flags & H5S_VALID_MAX) {
                if (NULL==(sdim->max=H5FL_ARR_ALLOC(hsize_t,(hsize_t)sdim->rank,0))) {
                    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                         "memory allocation failed");
                }
                for (u = 0; u < sdim->rank; u++) {
                    H5F_DECODE_LENGTH (f, p, sdim->max[u]);
                }
            }
#ifdef LATER
            if (flags & H5S_VALID_PERM) {
                if (NULL==(sdim->perm=H5FL_ARR_ALLOC(hsize_t,sdim->rank,0))) {
                    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
                }
                for (u = 0; u < sdim->rank; u++)
                    UINT32DECODE(p, sdim->perm[u]);
            }
#endif
        }
    }
    ret_value = (void*)sdim;    /*success*/
    
  done:
    if (!ret_value) H5FL_FREE(H5S_simple_t,sdim);
    FUNC_LEAVE(ret_value);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_encode
 PURPOSE
    Encode a simple dimensionality message 
 USAGE
    herr_t H5O_sdspace_encode(f, raw_size, p, mesg)
        H5F_t *f;         IN: pointer to the HDF5 file struct
        size_t raw_size;        IN: size of the raw information buffer
        const uint8 *p;         IN: the raw information buffer
        const void *mesg;       IN: Pointer to the simple dimensionality struct
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
    const H5S_simple_t  *sdim = (const H5S_simple_t *) mesg;
    unsigned            u;  /* Local counting variable */
    unsigned            flags = 0;

    FUNC_ENTER(H5O_sdspace_encode, FAIL);

    /* check args */
    assert(f);
    assert(p);
    assert(sdim);

    /* set flags */
    if (sdim->max)
        flags |= H5S_VALID_MAX;
#ifdef LATER
    if (sdim->perm)
        flags |= H5S_VALID_PERM;
#endif

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
#ifdef LATER
        if (flags & H5S_VALID_PERM) {
            for (u = 0; u < sdim->rank; u++)
                UINT32ENCODE(p, sdim->perm[u]);
        }
#endif
    }
    FUNC_LEAVE(SUCCEED);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_copy
 PURPOSE
    Copies a message from MESG to DEST, allocating DEST if necessary.
 USAGE
    void *H5O_sdspace_copy(mesg, dest)
        const void *mesg;       IN: Pointer to the source simple dimensionality struct
        const void *dest;       IN: Pointer to the destination simple dimensionality struct
 RETURNS
    Pointer to DEST on success, NULL on failure
 DESCRIPTION
        This function copies a native (memory) simple dimensionality message,
    allocating the destination structure if necessary.
--------------------------------------------------------------------------*/
static void *
H5O_sdspace_copy(const void *mesg, void *dest)
{
    const H5S_simple_t     *src = (const H5S_simple_t *) mesg;
    H5S_simple_t           *dst = (H5S_simple_t *) dest;

    FUNC_ENTER(H5O_sdspace_copy, NULL);

    /* check args */
    assert(src);
    if (!dst && NULL==(dst = H5FL_ALLOC(H5S_simple_t,0))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    /* deep copy -- pointed-to values are copied also */
    HDmemcpy(dst, src, sizeof(H5S_simple_t));
    
    if (src->size) {
        if (NULL==(dst->size = H5FL_ARR_ALLOC(hsize_t,(hsize_t)src->rank,0))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                           "memory allocation failed");
        }
        HDmemcpy (dst->size, src->size, src->rank*sizeof(src->size[0]));
    }
    if (src->max) {
        if (NULL==(dst->max=H5FL_ARR_ALLOC(hsize_t,(hsize_t)src->rank,0))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                           "memory allocation failed");
        }
        HDmemcpy (dst->max, src->max, src->rank*sizeof(src->max[0]));
    }
#ifdef LATER
    if (src->perm) {
        if (NULL==(dst->perm=H5FL_ARR_ALLOC(hsize_t,src->rank,0))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                           "memory allocation failed");
        }
        HDmemcpy (dst->perm, src->perm, src->rank*sizeof(src->perm[0]));
    }
#endif

    FUNC_LEAVE((void *) dst);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_size
 PURPOSE
    Return the raw message size in bytes
 USAGE
    void *H5O_sdspace_copy(f, mesg)
        H5F_t *f;         IN: pointer to the HDF5 file struct
        const void *mesg;       IN: Pointer to the source simple dimensionality struct
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
H5O_sdspace_size(H5F_t *f, const void *mesg)
{
    const H5S_simple_t     *space = (const H5S_simple_t *) mesg;
    
    /*
     * All dimensionality messages are at least 8 bytes long.
     */
    size_t                  ret_value = 8;

    FUNC_ENTER(H5O_sdspace_size, 0);

    /* add in the dimension sizes */
    ret_value += space->rank * H5F_SIZEOF_SIZE (f);

    /* add in the space for the maximum dimensions, if they are present */
    ret_value += space->max ? space->rank * H5F_SIZEOF_SIZE (f) : 0;

#ifdef LATER
    /* add in the space for the dimension permutations, if they are present */
    ret_value += space->perm ? space->rank * 4 : 0;
#endif

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_sdspace_reset
 *
 * Purpose:     Frees the inside of a dataspace message and resets it to some
 *              initial value.
 *
 * Return:      Non-negative on success/Negative on failure
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
    H5S_simple_t        *mesg = (H5S_simple_t*)_mesg;
    
    FUNC_ENTER (H5O_sdspace_reset, FAIL);

    H5S_release_simple(mesg);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_sdsdpace_free
 *
 * Purpose:     Free's the message
 *
 * Return:      Non-negative on success/Negative on failure
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
    FUNC_ENTER (H5O_sdspace_free, FAIL);

    assert (mesg);

    H5FL_FREE(H5S_simple_t,mesg);

    FUNC_LEAVE (SUCCEED);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_debug
 PURPOSE
    Prints debugging information for a simple dimensionality message
 USAGE
    void *H5O_sdspace_debug(f, mesg, stream, indent, fwidth)
        H5F_t *f;         IN: pointer to the HDF5 file struct
        const void *mesg;       IN: Pointer to the source simple dimensionality struct
        FILE *stream;           IN: Pointer to the stream for output data
        int indent;             IN: Amount to indent information by
        int fwidth;             IN: Field width (?)
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
        This function prints debugging output to the stream passed as a 
    parameter.
--------------------------------------------------------------------------*/
static herr_t
H5O_sdspace_debug(H5F_t UNUSED *f, const void *mesg,
                  FILE * stream, int indent, int fwidth)
{
    const H5S_simple_t     *sdim = (const H5S_simple_t *) mesg;
    unsigned                u;  /* local counting variable */

    FUNC_ENTER(H5O_sdspace_debug, FAIL);

    /* check args */
    assert(f);
    assert(sdim);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Rank:",
            (unsigned long) (sdim->rank));
    
    HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Size:");
    for (u = 0; u < sdim->rank; u++) {
        HDfprintf (stream, "%s%Hu", u?", ":"", sdim->size[u]);
    }
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

#ifdef LATER
    if (sdim->perm) {
        HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Perm:");
        for (u = 0; u < sdim->rank; u++) {
            HDfprintf (stream, "%s%lu", u?", ":"",
                 (unsigned long) (sdim->perm[u]));
        }
    }
#endif

    FUNC_LEAVE(SUCCEED);
}
