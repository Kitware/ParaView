/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, April 15, 1998
 *
 * Purpose:     Data filter pipeline message.
 */
#include "H5private.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5MMprivate.h"
#include "H5Oprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5O_pline_mask
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

#define H5O_PLINE_VERSION       1

static herr_t H5O_pline_encode (H5F_t *f, uint8_t *p, const void *mesg);
static void *H5O_pline_decode (H5F_t *f, const uint8_t *p, H5O_shared_t *sh);
static void *H5O_pline_copy (const void *_mesg, void *_dest);
static size_t H5O_pline_size (H5F_t *f, const void *_mesg);
static herr_t H5O_pline_reset (void *_mesg);
static herr_t H5O_pline_free (void *_mesg);
static herr_t H5O_pline_debug (H5F_t *f, const void *_mesg,
                               FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_PLINE[1] = {{
    H5O_PLINE_ID,               /* message id number            */
    "filter pipeline",          /* message name for debugging   */
    sizeof(H5O_pline_t),        /* native message size          */
    H5O_pline_decode,           /* decode message               */
    H5O_pline_encode,           /* encode message               */
    H5O_pline_copy,             /* copy the native value        */
    H5O_pline_size,             /* size of raw message          */
    H5O_pline_reset,            /* reset method                 */
    H5O_pline_free,             /* free method                  */
    NULL,                       /* get share method             */
    NULL,                       /* set share method             */
    H5O_pline_debug,            /* debug the message            */
}};


/* Declare a free list to manage the H5O_pline_t struct */
H5FL_DEFINE(H5O_pline_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_decode
 *
 * Purpose:     Decodes a filter pipeline message.
 *
 * Return:      Success:        Ptr to the native message.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_pline_decode(H5F_t UNUSED *f, const uint8_t *p,
                H5O_shared_t UNUSED *sh)
{
    H5O_pline_t         *pline = NULL;
    void                *ret_value = NULL;
    unsigned            version;
    size_t              i, j, n, name_length;

    FUNC_ENTER(H5O_pline_decode, NULL);

    /* check args */
    assert(p);

    /* Decode */
    if (NULL==(pline = H5FL_ALLOC(H5O_pline_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
    }
    version = *p++;
    if (version!=H5O_PLINE_VERSION) {
        HGOTO_ERROR(H5E_PLINE, H5E_CANTLOAD, NULL,
                    "bad version number for filter pipeline message");
    }
    pline->nfilters = *p++;
    if (pline->nfilters>32) {
        HGOTO_ERROR(H5E_PLINE, H5E_CANTLOAD, NULL,
                    "filter pipeline message has too many filters");
    }
    p += 6;     /*reserved*/
    pline->nalloc = pline->nfilters;
    pline->filter = H5MM_calloc(pline->nalloc*sizeof(pline->filter[0]));
    if (NULL==pline->filter) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                    "memory allocation failed");
    }
    for (i=0; i<pline->nfilters; i++) {
        UINT16DECODE(p, pline->filter[i].id);
        UINT16DECODE(p, name_length);
        if (name_length % 8) {
            HGOTO_ERROR(H5E_PLINE, H5E_CANTLOAD, NULL,
                        "filter name length is not a multiple of eight");
        }
        UINT16DECODE(p, pline->filter[i].flags);
        UINT16DECODE(p, pline->filter[i].cd_nelmts);
        if (name_length) {
            /*
             * Get the name, allocating an extra byte for an extra null
             * terminator just in case there isn't one in the file (there
             * should be, but to be safe...)
             */
            pline->filter[i].name = H5MM_malloc(name_length+1);
            HDmemcpy(pline->filter[i].name, p, name_length);
            pline->filter[i].name[name_length] = '\0';
            p += name_length;
        }
        if ((n=pline->filter[i].cd_nelmts)) {
            /*
             * Read the client data values and the padding
             */
            pline->filter[i].cd_values = H5MM_malloc(n*sizeof(unsigned));
            if (NULL==pline->filter[i].cd_values) {
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                            "memory allocation failed for client data");
            }
            for (j=0; j<pline->filter[i].cd_nelmts; j++) {
                UINT32DECODE(p, pline->filter[i].cd_values[j]);
            }
            if (pline->filter[i].cd_nelmts % 2) {
                p += 4; /*padding*/
            }
        }
    }
    ret_value = pline;

 done:
    if (NULL==ret_value && pline) {
        if (pline->filter) {
            for (i=0; i<pline->nfilters; i++) {
                H5MM_xfree(pline->filter[i].name);
                H5MM_xfree(pline->filter[i].cd_values);
            }
            H5MM_xfree(pline->filter);
        }
        H5FL_FREE(H5O_pline_t,pline);
    }
    FUNC_LEAVE(ret_value);

    f = 0;
    sh = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_encode
 *
 * Purpose:     Encodes message MESG into buffer P.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_encode (H5F_t UNUSED *f, uint8_t *p/*out*/, const void *mesg)
{
    const H5O_pline_t   *pline = (const H5O_pline_t*)mesg;
    size_t              i, j, name_length;
    const char          *name=NULL;
    H5Z_class_t         *cls=NULL;
    
    FUNC_ENTER (H5O_pline_encode, FAIL);

    /* Check args */
    assert (p);
    assert (mesg);

    *p++ = H5O_PLINE_VERSION;
    *p++ = (uint8_t)(pline->nfilters);
    *p++ = 0;   /*reserved 1*/
    *p++ = 0;   /*reserved 2*/
    *p++ = 0;   /*reserved 3*/
    *p++ = 0;   /*reserved 4*/
    *p++ = 0;   /*reserved 5*/
    *p++ = 0;   /*reserved 6*/

    for (i=0; i<pline->nfilters; i++) {
        /*
         * Get the filter name.  If the pipeline message has a name in it then
         * use that one.  Otherwise try to look up the filter and get the name
         * as it was registered.
         */
        if (NULL==(name=pline->filter[i].name) &&
            (cls=H5Z_find(pline->filter[i].id))) {
            name = cls->name;
        }
        name_length = name ? HDstrlen(name)+1 : 0;

        /* Encode the filter */
        UINT16ENCODE(p, pline->filter[i].id);
        UINT16ENCODE(p, H5O_ALIGN(name_length));
        UINT16ENCODE(p, pline->filter[i].flags);
        UINT16ENCODE(p, pline->filter[i].cd_nelmts);
        if (name_length>0) {
            HDmemcpy(p, name, name_length);
            p += name_length;
            while (name_length++ % 8) *p++ = 0;
        }
        for (j=0; j<pline->filter[i].cd_nelmts; j++) {
            UINT32ENCODE(p, pline->filter[i].cd_values[j]);
        }
        if (pline->filter[i].cd_nelmts % 2) {
            UINT32ENCODE(p, 0);
        }
    }

    FUNC_LEAVE (SUCCEED);

    f = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_copy
 *
 * Purpose:     Copies a filter pipeline message from SRC to DST allocating
 *              DST if necessary.  If DST is already allocated then we assume
 *              that it isn't initialized.
 *
 * Return:      Success:        Ptr to DST or allocated result.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_pline_copy (const void *_src, void *_dst/*out*/)
{
    const H5O_pline_t   *src = (const H5O_pline_t *)_src;
    H5O_pline_t         *dst = (H5O_pline_t *)_dst;
    size_t              i;
    H5O_pline_t         *ret_value = NULL;
    
    FUNC_ENTER (H5O_pline_copy, NULL);

    if (!dst && NULL==(dst = H5FL_ALLOC (H5O_pline_t,0))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    *dst = *src;
    dst->nalloc = dst->nfilters;
    if (dst->nalloc>0) {
        dst->filter = H5MM_calloc(dst->nalloc * sizeof(dst->filter[0]));
        if (NULL==dst->filter) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                        "memory allocation failed");
        }
    } else {
        dst->filter = NULL;
    }
    
    for (i=0; i<src->nfilters; i++) {
        dst->filter[i] = src->filter[i];
        if (src->filter[i].name) {
            dst->filter[i].name = H5MM_xstrdup(src->filter[i].name);
        }
        if (src->filter[i].cd_nelmts>0) {
            dst->filter[i].cd_values = H5MM_malloc(src->filter[i].cd_nelmts*
                                                   sizeof(unsigned));
            if (NULL==dst->filter[i].cd_values) {
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                               "memory allocation failed");
            }
            HDmemcpy (dst->filter[i].cd_values, src->filter[i].cd_values,
                      src->filter[i].cd_nelmts * sizeof(unsigned));
        }
    }
    ret_value = dst;

 done:
    if (!ret_value && dst) {
        if (dst->filter) {
            for (i=0; i<dst->nfilters; i++) {
                H5MM_xfree(dst->filter[i].name);
                H5MM_xfree(dst->filter[i].cd_values);
            }
            H5MM_xfree(dst->filter);
        }
        if (!_dst) H5FL_FREE(H5O_pline_t,dst);
    }

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_size
 *
 * Purpose:     Determines the size of a raw filter pipeline message.
 *
 * Return:      Success:        Size of message.
 *
 *              Failure:        zero
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_pline_size (H5F_t UNUSED *f, const void *mesg)
{
    const H5O_pline_t   *pline = (const H5O_pline_t*)mesg;
    size_t              i, size, name_len;
    const char          *name = NULL;
    H5Z_class_t         *cls = NULL;
        
    FUNC_ENTER (H5O_pline_size, 0);

    /* Message header */
    size = 1 +                          /*version                       */
           1 +                          /*number of filters             */
           6;                           /*reserved                      */

    for (i=0; i<pline->nfilters; i++) {
        /* Get the name of the filter, same as done with H5O_pline_encode() */
        if (NULL==(name=pline->filter[i].name) &&
            (cls=H5Z_find(pline->filter[i].id))) {
            name = cls->name;
        }
        name_len = name ? HDstrlen(name)+1 : 0;
        

        size += 2 +                     /*filter identification number  */
                2 +                     /*name length                   */
                2 +                     /*flags                         */
                2 +                     /*number of client data values  */
                H5O_ALIGN(name_len);    /*length of the filter name     */
        
        size += pline->filter[i].cd_nelmts * 4;
        if (pline->filter[i].cd_nelmts % 2) size += 4;
    }

    FUNC_LEAVE (size);

    f = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_reset
 *
 * Purpose:     Resets a filter pipeline message by clearing all filters.
 *              The MESG buffer is not freed.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_reset (void *mesg)
{
    H5O_pline_t *pline = (H5O_pline_t*)mesg;
    size_t      i;
    
    FUNC_ENTER (H5O_pline_reset, FAIL);

    assert (pline);
    for (i=0; i<pline->nfilters; i++) {
        H5MM_xfree(pline->filter[i].name);
        H5MM_xfree(pline->filter[i].cd_values);
    }
    H5MM_xfree(pline->filter);
    HDmemset(pline, 0, sizeof *pline);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_free
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
H5O_pline_free (void *mesg)
{
    FUNC_ENTER (H5O_pline_free, FAIL);

    assert (mesg);

    H5FL_FREE(H5O_pline_t,mesg);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_debug
 *
 * Purpose:     Prints debugging information for filter pipeline message MESG
 *              on output stream STREAM.  Each line is indented INDENT
 *              characters and the field name takes up FWIDTH characters.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_debug (H5F_t UNUSED *f, const void *mesg, FILE *stream,
                int indent, int fwidth)
{
    const H5O_pline_t   *pline = (const H5O_pline_t *)mesg;
    size_t              i, j;

    FUNC_ENTER(H5O_pline_debug, FAIL);

    /* check args */
    assert(f);
    assert(pline);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    fprintf(stream, "%*s%-*s %lu/%lu\n", indent, "", fwidth,
            "Number of filters:",
            (unsigned long)(pline->nfilters),
            (unsigned long)(pline->nalloc));

    for (i=0; i<pline->nfilters; i++) {
        char            name[32];
        sprintf(name, "Filter at position %lu", (unsigned long)i);
        fprintf(stream, "%*s%-*s\n", indent, "", fwidth, name);
        fprintf(stream, "%*s%-*s 0x%04x\n", indent+3, "", MAX(0, fwidth-3),
                "Filter identification:",
                (unsigned)(pline->filter[i].id));
        if (pline->filter[i].name) {
            fprintf(stream, "%*s%-*s \"%s\"\n", indent+3, "", MAX(0, fwidth-3),
                    "Filter name:",
                    pline->filter[i].name);
        } else {
            fprintf(stream, "%*s%-*s NONE\n", indent+3, "", MAX(0, fwidth-3),
                    "Filter name:");
        }
        fprintf(stream, "%*s%-*s 0x%04x\n", indent+3, "", MAX(0, fwidth-3),
                "Flags:",
                (unsigned)(pline->filter[i].flags));
        fprintf(stream, "%*s%-*s %lu\n", indent+3, "", MAX(0, fwidth-3),
                "Num CD values:",
                (unsigned long)(pline->filter[i].cd_nelmts));
        for (j=0; j<pline->filter[i].cd_nelmts; j++) {
            char        field_name[32];
            sprintf(field_name, "CD value %lu", (unsigned long)j);
            fprintf(stream, "%*s%-*s %lu\n", indent+6, "", MAX(0, fwidth-6),
                    field_name,
                    (unsigned long)(pline->filter[i].cd_values[j]));
        }
    }
    
    FUNC_LEAVE(SUCCEED);
}
