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

/* Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, October  8, 1997
 *
 * Purpose:     Messages related to data layout.
 */

#define H5O_PACKAGE  /*suppress error about including H5Opkg    */


#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"  /*Free Lists    */
#include "H5MFprivate.h"  /* File space management    */
#include "H5MMprivate.h"
#include "H5Opkg.h"             /* Object header functions                  */

/* PRIVATE PROTOTYPES */
static void *H5O_layout_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_layout_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_layout_copy(const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_layout_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_layout_reset(void *_mesg);
static herr_t H5O_layout_free(void *_mesg);
static herr_t H5O_layout_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg, hbool_t adj_link);
static herr_t H5O_layout_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE * stream,
             int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_LAYOUT[1] = {{
    H5O_LAYOUT_ID,            /*message id number             */
    "layout",                 /*message name for debugging    */
    sizeof(H5O_layout_t),     /*native message size           */
    H5O_layout_decode,        /*decode message                */
    H5O_layout_encode,        /*encode message                */
    H5O_layout_copy,          /*copy the native value         */
    H5O_layout_size,          /*size of message on disk       */
    H5O_layout_reset,           /*reset method                  */
    H5O_layout_free,          /*free the struct    */
    H5O_layout_delete,          /* file delete method    */
    NULL,      /* link method      */
    NULL,          /*get share method    */
    NULL,      /*set share method    */
    H5O_layout_debug,         /*debug the message             */
}};

/* For forward and backward compatibility.  Version is 1 when space is
 * allocated; 2 when space is delayed for allocation; 3
 * is revised to just store information needed for each storage type. */
#define H5O_LAYOUT_VERSION_1  1
#define H5O_LAYOUT_VERSION_2  2
#define H5O_LAYOUT_VERSION_3  3

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
 *   Robb Matzke, 1998-07-20
 *  Rearranged the message to add a version number at the beginning.
 *
 *      Raymond Lu, 2002-2-26
 *      Added version number 2 case depends on if space has been allocated
 *      at the moment when layout header message is updated.
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_decode(H5F_t *f, hid_t UNUSED dxpl_id, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5O_layout_t           *mesg = NULL;
    unsigned               u;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_layout_decode);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if (NULL==(mesg = H5FL_CALLOC(H5O_layout_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Version. 1 when space allocated; 2 when space allocation is delayed */
    mesg->version = *p++;
    if (mesg->version<H5O_LAYOUT_VERSION_1 || mesg->version>H5O_LAYOUT_VERSION_3)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for layout message");

    if(mesg->version < H5O_LAYOUT_VERSION_3) {
        unsigned  ndims;      /* Num dimensions in chunk           */

        /* Dimensionality */
        ndims = *p++;
        if (ndims>H5O_LAYOUT_NDIMS)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is too large");

        /* Layout class */
        mesg->type = (H5D_layout_t)*p++;
        assert(H5D_CONTIGUOUS == mesg->type || H5D_CHUNKED == mesg->type || H5D_COMPACT == mesg->type);

        /* Reserved bytes */
        p += 5;

        /* Address */
        if(mesg->type==H5D_CONTIGUOUS)
            H5F_addr_decode(f, &p, &(mesg->u.contig.addr));
        else if(mesg->type==H5D_CHUNKED)
            H5F_addr_decode(f, &p, &(mesg->u.chunk.addr));

        /* Read the size */
        if(mesg->type!=H5D_CHUNKED) {
      mesg->unused.ndims=ndims;

            for (u = 0; u < ndims; u++)
                UINT32DECODE(p, mesg->unused.dim[u]);

            /* Don't compute size of contiguous storage here, due to possible
             * truncation of the dimension sizes when they were stored in this
             * version of the layout message.  Compute the contiguous storage
             * size in the dataset code, where we've got the dataspace
             * information available also.  - QAK 5/26/04
             */
        } /* end if */
        else {
            mesg->u.chunk.ndims=ndims;
            for (u = 0; u < ndims; u++)
                UINT32DECODE(p, mesg->u.chunk.dim[u]);

            /* Compute chunk size */
            for (u=1, mesg->u.chunk.size=mesg->u.chunk.dim[0]; u<ndims; u++)
                mesg->u.chunk.size *= mesg->u.chunk.dim[u];
        } /* end if */

        if(mesg->type == H5D_COMPACT) {
            UINT32DECODE(p, mesg->u.compact.size);
            if(mesg->u.compact.size > 0) {
                if(NULL==(mesg->u.compact.buf=H5MM_malloc(mesg->u.compact.size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for compact data buffer");
                HDmemcpy(mesg->u.compact.buf, p, mesg->u.compact.size);
                p += mesg->u.compact.size;
            }
        }
    } /* end if */
    else {
        /* Layout class */
        mesg->type = (H5D_layout_t)*p++;

        /* Interpret the rest of the message according to the layout class */
        switch(mesg->type) {
            case H5D_CONTIGUOUS:
                H5F_addr_decode(f, &p, &(mesg->u.contig.addr));
                H5F_DECODE_LENGTH(f, p, mesg->u.contig.size);
                break;

            case H5D_CHUNKED:
                /* Dimensionality */
                mesg->u.chunk.ndims = *p++;
                if (mesg->u.chunk.ndims>H5O_LAYOUT_NDIMS)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is too large");

                /* B-tree address */
                H5F_addr_decode(f, &p, &(mesg->u.chunk.addr));

                /* Chunk dimensions */
                for (u = 0; u < mesg->u.chunk.ndims; u++)
                    UINT32DECODE(p, mesg->u.chunk.dim[u]);

                /* Compute chunk size */
                for (u=1, mesg->u.chunk.size=mesg->u.chunk.dim[0]; u<mesg->u.chunk.ndims; u++)
                    mesg->u.chunk.size *= mesg->u.chunk.dim[u];
                break;

            case H5D_COMPACT:
                UINT16DECODE(p, mesg->u.compact.size);
                if(mesg->u.compact.size > 0) {
                    if(NULL==(mesg->u.compact.buf=H5MM_malloc(mesg->u.compact.size)))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for compact data buffer");
                    HDmemcpy(mesg->u.compact.buf, p, mesg->u.compact.size);
                    p += mesg->u.compact.size;
                } /* end if */
                break;

            default:
                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "Invalid layout class");
        } /* end switch */
    } /* end else */

    /* Set return value */
    ret_value=mesg;

done:
    if(ret_value==NULL) {
        if(mesg)
            H5FL_FREE(H5O_layout_t,mesg);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
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
 *   Robb Matzke, 1998-07-20
 *  Rearranged the message to add a version number at the beginning.
 *
 *  Raymond Lu, 2002-2-26
 *  Added version number 2 case depends on if space has been allocated
 *  at the moment when layout header message is updated.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned               u;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_layout_encode);

    /* check args */
    assert(f);
    assert(mesg);
    assert(mesg->version>0);
    assert(p);

    /* Version */
    *p++ = mesg->version;

    /* Check for which information to write */
    if(mesg->version<3) {
        /* number of dimensions */
        if(mesg->type!=H5D_CHUNKED) {
            assert(mesg->unused.ndims > 0 && mesg->unused.ndims <= H5O_LAYOUT_NDIMS);
            *p++ = mesg->unused.ndims;
        } /* end if */
        else {
            assert(mesg->u.chunk.ndims > 0 && mesg->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
            *p++ = mesg->u.chunk.ndims;
        } /* end else */

        /* layout class */
        *p++ = mesg->type;

        /* reserved bytes should be zero */
        for (u=0; u<5; u++)
            *p++ = 0;

        /* data or B-tree address */
        if(mesg->type==H5D_CONTIGUOUS)
            H5F_addr_encode(f, &p, mesg->u.contig.addr);
        else if(mesg->type==H5D_CHUNKED)
            H5F_addr_encode(f, &p, mesg->u.chunk.addr);

        /* dimension size */
        if(mesg->type!=H5D_CHUNKED)
            for (u = 0; u < mesg->unused.ndims; u++)
                UINT32ENCODE(p, mesg->unused.dim[u])
        else
            for (u = 0; u < mesg->u.chunk.ndims; u++)
                UINT32ENCODE(p, mesg->u.chunk.dim[u]);

        if(mesg->type==H5D_COMPACT) {
            UINT32ENCODE(p, mesg->u.compact.size);
            if(mesg->u.compact.size>0 && mesg->u.compact.buf) {
                HDmemcpy(p, mesg->u.compact.buf, mesg->u.compact.size);
                p += mesg->u.compact.size;
            }
        }
    } /* end if */
    else {
        /* Layout class */
        *p++ = mesg->type;

        /* Write out layout class specific information */
        switch(mesg->type) {
            case H5D_CONTIGUOUS:
                H5F_addr_encode(f, &p, mesg->u.contig.addr);
                H5F_ENCODE_LENGTH(f, p, mesg->u.contig.size);
                break;

            case H5D_CHUNKED:
                /* Number of dimensions */
                assert(mesg->u.chunk.ndims > 0 && mesg->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
                *p++ = mesg->u.chunk.ndims;

                /* B-tree address */
                H5F_addr_encode(f, &p, mesg->u.chunk.addr);

                /* Dimension sizes */
                for (u = 0; u < mesg->u.chunk.ndims; u++)
                    UINT32ENCODE(p, mesg->u.chunk.dim[u]);
                break;

            case H5D_COMPACT:
                /* Size of raw data */
                UINT16ENCODE(p, mesg->u.compact.size);

                /* Raw data */
                if(mesg->u.compact.size>0 && mesg->u.compact.buf) {
                    HDmemcpy(p, mesg->u.compact.buf, mesg->u.compact.size);
                    p += mesg->u.compact.size;
                } /* end if */
                break;

            default:
                HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "Invalid layout class");
        } /* end switch */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
H5O_layout_copy(const void *_mesg, void *_dest, unsigned UNUSED update_flags)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    H5O_layout_t           *dest = (H5O_layout_t *) _dest;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_layout_copy);

    /* check args */
    assert(mesg);
    if (!dest && NULL==(dest=H5FL_MALLOC(H5O_layout_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* copy */
    *dest = *mesg;

    /* Deep copy the buffer for compact datasets also */
    if(mesg->type==H5D_COMPACT) {
        /* Allocate memory for the raw data */
        if (NULL==(dest->u.compact.buf=H5MM_malloc(dest->u.compact.size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate memory for compact dataset");

        /* Copy over the raw data */
        HDmemcpy(dest->u.compact.buf,mesg->u.compact.buf,dest->u.compact.size);
    } /* end if */

    /* Set return value */
    ret_value=dest;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_meta_size
 *
 * Purpose:     Returns the size of the raw message in bytes except raw data
 *              part for compact dataset.  This function doesn't take into
 *              account message alignment.
 *
 * Return:      Success:        Message data size in bytes(except raw data
 *                              for compact dataset)
 *              Failure:        0
 *
 * Programmer:  Raymond Lu
 *              August 14, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5O_layout_meta_size(const H5F_t *f, const void *_mesg)
{
    /* Casting away const OK - QAK */
    H5O_layout_t      *mesg = (H5O_layout_t *) _mesg;
    size_t                  ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5O_layout_meta_size);

    /* check args */
    assert(f);
    assert(mesg);

    /* Check version information for new datasets */
    if(mesg->version==0) {
        unsigned               u;

        /* Check for dimension that would be truncated */
        assert(mesg->unused.ndims > 0 && mesg->unused.ndims <= H5O_LAYOUT_NDIMS);
        for (u = 0; u < mesg->unused.ndims; u++)
            if(mesg->unused.dim[u]!=(0xffffffff&mesg->unused.dim[u])) {
                /* Make certain the message is encoded with the new version */
                mesg->version=3;
                break;
            } /* end if */

        /* If the message doesn't _have_ to be encoded with the new version */
        if(mesg->version==0) {
            /* Version: 1 when space allocated; 2 when space allocation is delayed */
            if(mesg->type==H5D_CONTIGUOUS) {
                if(mesg->u.contig.addr==HADDR_UNDEF)
                    mesg->version = H5O_LAYOUT_VERSION_2;
                else
                    mesg->version = H5O_LAYOUT_VERSION_1;
            } else if(mesg->type==H5D_COMPACT) {
                mesg->version = H5O_LAYOUT_VERSION_2;
            } else
                mesg->version = H5O_LAYOUT_VERSION_1;
        } /* end if */
    } /* end if */
    assert(mesg->version>0);

    if(mesg->version<H5O_LAYOUT_VERSION_3) {
        ret_value = 1 +                     /* Version number                       */
                    1 +                     /* layout class type                    */
                    1 +                     /* dimensionality                       */
                    5 +                     /* reserved bytes                       */
                    mesg->unused.ndims * 4;        /* size of each dimension               */

        if(mesg->type==H5D_COMPACT)
            ret_value += 4;        /* size field for compact dataset       */
        else
            ret_value += H5F_SIZEOF_ADDR(f); /* file address of data or B-tree for chunked dataset */
    } /* end if */
    else {
        ret_value = 1 +                     /* Version number                       */
                    1;                      /* layout class type                    */

        switch(mesg->type) {
            case H5D_CONTIGUOUS:
                ret_value += H5F_SIZEOF_ADDR(f);    /* Address of data */
                ret_value += H5F_SIZEOF_SIZE(f);    /* Length of data */
                break;

            case H5D_CHUNKED:
                /* Number of dimensions (1 byte) */
                assert(mesg->u.chunk.ndims > 0 && mesg->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
                ret_value++;

                /* B-tree address */
                ret_value += H5F_SIZEOF_ADDR(f);    /* Address of data */

                /* Dimension sizes */
                ret_value += mesg->u.chunk.ndims*4;
                break;

            case H5D_COMPACT:
                /* Size of raw data */
                ret_value+=2;
                break;

            default:
                HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, 0, "Invalid layout class");
        } /* end switch */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_size
 *
 * Purpose:     Returns the size of the raw message in bytes.  If it's
 *              compact dataset, the data part is also included.
 *              This function doesn't take into account message alignment.
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
H5O_layout_size(const H5F_t *f, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    size_t                  ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_layout_size);

    /* check args */
    assert(f);
    assert(mesg);

    ret_value = H5O_layout_meta_size(f, mesg);
    if(mesg->type==H5D_COMPACT)
        ret_value += mesg->u.compact.size;/* data for compact dataset             */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_layout_reset
 *
 * Purpose:  Frees resources within a data type message, but doesn't free
 *    the message itself.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 13, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_reset (void *_mesg)
{
    H5O_layout_t     *mesg = (H5O_layout_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_layout_reset);

    if(mesg) {
        /* Free the compact storage buffer */
        if(mesg->type==H5D_COMPACT)
            mesg->u.compact.buf=H5MM_xfree(mesg->u.compact.buf);

        /* Reset the message */
        mesg->type=H5D_CONTIGUOUS;
        mesg->version=0;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_layout_free
 *
 * Purpose:  Free's the message
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 11, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_free (void *_mesg)
{
    H5O_layout_t     *mesg = (H5O_layout_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_layout_free);

    assert (mesg);

    /* Free the compact storage buffer */
    if(mesg->type==H5D_COMPACT)
        mesg->u.compact.buf=H5MM_xfree(mesg->u.compact.buf);

    H5FL_FREE(H5O_layout_t,mesg);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, March 19, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg, hbool_t UNUSED adj_link)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_layout_delete);

    /* check args */
    assert(f);
    assert(mesg);

    /* Perform different actions, depending on the type of storage */
    switch(mesg->type) {
        case H5D_COMPACT:       /* Compact data storage */
            /* Nothing required */
            break;

        case H5D_CONTIGUOUS:    /* Contiguous block on disk */
            /* Free the file space for the raw data */
            if (H5D_contig_delete(f, dxpl_id, mesg)<0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data");
            break;

        case H5D_CHUNKED:       /* Chunked blocks on disk */
            /* Free the file space for the raw data */
            if (H5D_istore_delete(f, dxpl_id, mesg)<0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data");
            break;

        default:
            HGOTO_ERROR (H5E_OHDR, H5E_BADTYPE, FAIL, "not valid storage type");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_layout_delete() */


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
H5O_layout_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg, FILE * stream,
     int indent, int fwidth)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned                    u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_layout_debug);

    /* check args */
    assert(f);
    assert(mesg);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    if(mesg->type==H5D_CHUNKED) {
        HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                  "B-tree address:", mesg->u.chunk.addr);
        HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
                  "Number of dimensions:",
                  (unsigned long) (mesg->u.chunk.ndims));
        /* Size */
        HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Size:");
        for (u = 0; u < mesg->u.chunk.ndims; u++) {
            HDfprintf(stream, "%s%lu", u ? ", " : "",
                      (unsigned long) (mesg->u.chunk.dim[u]));
        }
        HDfprintf(stream, "}\n");
    } /* end if */
    else if(mesg->type==H5D_CONTIGUOUS) {
        HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                  "Data address:", mesg->u.contig.addr);
        HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
                  "Data Size:", mesg->u.contig.size);
    } /* end if */
    else {
        HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
                  "Data Size:", mesg->u.compact.size);
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED);
}
