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

#define H5A_PACKAGE         /*prevent warning from including H5Tpkg.h */
#define H5S_PACKAGE         /*suppress error about including H5Spkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"    /*free lists                              */
#include "H5Gprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"
#include "H5Apkg.h"
#include "H5Spkg.h"         /*data spaces                             */

#define PABLO_MASK      H5O_attr_mask

/* PRIVATE PROTOTYPES */
static herr_t H5O_attr_encode (H5F_t *f, uint8_t *p, const void *mesg);
static void *H5O_attr_decode (H5F_t *f, const uint8_t *p, H5O_shared_t *sh);
static void *H5O_attr_copy (const void *_mesg, void *_dest);
static size_t H5O_attr_size (H5F_t *f, const void *_mesg);
static herr_t H5O_attr_reset (void *_mesg);
static herr_t H5O_attr_debug (H5F_t *f, const void *_mesg,
                              FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_ATTR[1] = {{
    H5O_ATTR_ID,                /* message id number            */
    "attribute",                /* message name for debugging   */
    sizeof(H5A_t),              /* native message size          */
    H5O_attr_decode,            /* decode message               */
    H5O_attr_encode,            /* encode message               */
    H5O_attr_copy,              /* copy the native value        */
    H5O_attr_size,              /* size of raw message          */
    H5O_attr_reset,             /* reset method                 */
    NULL,                       /* default free method                  */
    NULL,                       /* get share method             */
    NULL,                       /* set share method             */
    H5O_attr_debug,             /* debug the message            */
}};

#define H5O_ATTR_VERSION        1

/* Interface initialization */
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

/* Declare external the free list for H5S_t's */
H5FL_EXTERN(H5S_t);

/* Declare external the free list for H5S_simple_t's */
H5FL_EXTERN(H5S_simple_t);

/*--------------------------------------------------------------------------
 NAME
    H5O_attr_decode
 PURPOSE
    Decode a attribute message and return a pointer to a memory struct
        with the decoded information
 USAGE
    void *H5O_attr_decode(f, raw_size, p)
        H5F_t *f;               IN: pointer to the HDF5 file struct
        size_t raw_size;        IN: size of the raw information buffer
        const uint8_t *p;         IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
        This function decodes the "raw" disk form of a attribute message
    into a struct in memory native format.  The struct is allocated within this
    function using malloc() and is returned to the caller.
 *
 * Modifications:
 *      Robb Matzke, 17 Jul 1998
 *      Added padding for alignment.
 *
 *      Robb Matzke, 20 Jul 1998
 *      Added a version number at the beginning.
--------------------------------------------------------------------------*/
static void *
H5O_attr_decode(H5F_t *f, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5A_t               *attr = NULL;
    H5S_simple_t        *simple;        /*simple dimensionality information  */
    size_t              name_len;       /*attribute name length */
    int         version;        /*message version number*/

    FUNC_ENTER(H5O_attr_decode, NULL);

    /* check args */
    assert(f);
    assert(p);

    if (NULL==(attr = H5MM_calloc(sizeof(H5A_t)))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    /* Version number */
    version = *p++;
    if (version!=H5O_ATTR_VERSION) {
        HRETURN_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL,
                      "bad version number for attribute message");
    }

    /* Reserved */
    p++;

    /*
     * Decode the sizes of the parts of the attribute.  The sizes stored in
     * the file are exact but the parts are aligned on 8-byte boundaries.
     */
    UINT16DECODE(p, name_len); /*including null*/
    UINT16DECODE(p, attr->dt_size);
    UINT16DECODE(p, attr->ds_size);
    
    /* Decode and store the name */
    if (NULL==(attr->name=H5MM_malloc(name_len))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    HDmemcpy(attr->name,p,name_len);
    p += H5O_ALIGN(name_len);    /* advance the memory pointer */

    /* decode the attribute datatype */
    if((attr->dt=(H5O_DTYPE->decode)(f,p,NULL))==NULL) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTDECODE, NULL,
                      "can't decode attribute datatype");
    }
    p += H5O_ALIGN(attr->dt_size);

    /* decode the attribute dataspace */
    if (NULL==(attr->ds = H5FL_ALLOC(H5S_t,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    if((simple=(H5O_SDSPACE->decode)(f,p,NULL))!=NULL) {
        attr->ds->extent.type = H5S_SIMPLE;
        HDmemcpy(&(attr->ds->extent.u.simple),simple, sizeof(H5S_simple_t));
        H5FL_FREE(H5S_simple_t,simple);
    } else {
        attr->ds->extent.type = H5S_SCALAR;
    }
    p += H5O_ALIGN(attr->ds_size);

    /* Compute the size of the data */
    attr->data_size=H5S_get_simple_extent_npoints(attr->ds)*H5T_get_size(attr->dt);

    /* Go get the data */
    if (NULL==(attr->data = H5MM_malloc(attr->data_size))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    HDmemcpy(attr->data,p,attr->data_size);

    /* Indicate that the fill values aren't to be written out */
    attr->initialized=1;
    
#ifdef LOTSLATER
    if (hobj) {
        attr->sh_heap = *hobj;
        attr->sh_file = f;
    }
#endif 

    FUNC_LEAVE(attr);
    sh = 0;
}

/*--------------------------------------------------------------------------
 NAME
    H5O_attr_encode
 PURPOSE
    Encode a simple attribute message 
 USAGE
    herr_t H5O_attr_encode(f, raw_size, p, mesg)
        H5F_t *f;         IN: pointer to the HDF5 file struct
        const uint8 *p;         IN: the raw information buffer
        const void *mesg;       IN: Pointer to the simple datatype struct
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
        This function encodes the native memory form of the attribute
    message in the "raw" disk form.
 *
 * Modifications:
 *      Robb Matzke, 17 Jul 1998
 *      Added padding for alignment.
 *
 *      Robb Matzke, 20 Jul 1998
 *      Added a version number at the beginning.
--------------------------------------------------------------------------*/
static herr_t
H5O_attr_encode(H5F_t *f, uint8_t *p, const void *mesg)
{
    const H5A_t    *attr = (const H5A_t *) mesg;
    size_t          name_len;   /* Attribute name length */

    FUNC_ENTER(H5O_attr_encode, FAIL);

    /* check args */
    assert(f);
    assert(p);
    assert(attr);

    /* Version */
    *p++ = H5O_ATTR_VERSION;

    /* Reserved */
    *p++ = 0;

    /*
     * Encode the lengths of the various parts of the attribute message. The
     * encoded lengths are exact but we pad each part except the data to be a
     * multiple of eight bytes.
     */
    name_len = HDstrlen(attr->name)+1;
    UINT16ENCODE(p, name_len);
    UINT16ENCODE(p, attr->dt_size);
    UINT16ENCODE(p, attr->ds_size);

    /*
     * Write the name including null terminator padded to the correct number
     * of bytes.
     */
    HDmemcpy(p, attr->name, name_len);
    HDmemset(p+name_len, 0, H5O_ALIGN(name_len)-name_len);
    p += H5O_ALIGN(name_len);

    /* encode the attribute datatype */
    if((H5O_DTYPE->encode)(f,p,attr->dt)<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL,
                      "can't encode attribute datatype");
    }
    HDmemset(p+attr->dt_size, 0, H5O_ALIGN(attr->dt_size)-attr->dt_size);
    p += H5O_ALIGN(attr->dt_size);

    /* encode the attribute dataspace */
    if((H5O_SDSPACE->encode)(f,p,&(attr->ds->extent.u.simple))<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL,
                      "can't encode attribute dataspace");
    }
    HDmemset(p+attr->ds_size, 0, H5O_ALIGN(attr->ds_size)-attr->ds_size);
    p += H5O_ALIGN(attr->ds_size);
    
    /* Store attribute data */
    HDmemcpy(p,attr->data,attr->data_size);

    FUNC_LEAVE(SUCCEED);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_attr_copy
 PURPOSE
    Copies a message from MESG to DEST, allocating DEST if necessary.
 USAGE
    void *H5O_attr_copy(mesg, dest)
        const void *mesg;       IN: Pointer to the source attribute struct 
        const void *dest;       IN: Pointer to the destination attribute struct 
 RETURNS
    Pointer to DEST on success, NULL on failure
 DESCRIPTION
        This function copies a native (memory) attribute message,
    allocating the destination structure if necessary.
--------------------------------------------------------------------------*/
static void            *
H5O_attr_copy(const void *_src, void *_dst)
{
    const H5A_t            *src = (const H5A_t *) _src;
    H5A_t                  *dst = NULL;

    FUNC_ENTER(H5O_attr_copy, NULL);

    /* check args */
    assert(src);

    /* copy */
    if (NULL == (dst = H5A_copy(src))) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTINIT, NULL, "can't copy attribute");
    }
    /* was result already allocated? */
    if (_dst) {
        *((H5A_t *) _dst) = *dst;
        H5MM_xfree(dst);
        dst = (H5A_t *) _dst;
    }
    FUNC_LEAVE((void *) dst);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_attr_size
 PURPOSE
    Return the raw message size in bytes
 USAGE
    size_t H5O_attr_size(f, mesg)
        H5F_t *f;         IN: pointer to the HDF5 file struct
        const void *mesg;     IN: Pointer to the source attribute struct
 RETURNS
    Size of message on success, 0 on failure
 DESCRIPTION
        This function returns the size of the raw attribute message on
    success.  (Not counting the message type or size fields, only the data
    portion of the message).  It doesn't take into account alignment.
 *
 * Modified:
 *      Robb Matzke, 17 Jul 1998
 *      Added padding between message parts for alignment.
--------------------------------------------------------------------------*/
static size_t
H5O_attr_size(H5F_t UNUSED *f, const void *mesg)
{
    size_t              ret_value = 0;
    size_t              name_len;
    const H5A_t         *attr = (const H5A_t *) mesg;

    FUNC_ENTER(H5O_attr_size, 0);

    assert(attr);

    name_len = HDstrlen(attr->name)+1;

    ret_value = 2 +                             /*name size inc. null   */
                2 +                             /*type size             */
                2 +                             /*space size            */
                2 +                             /*reserved              */
                H5O_ALIGN(name_len)      +      /*attribute name        */
                H5O_ALIGN(attr->dt_size) +      /*data type             */
                H5O_ALIGN(attr->ds_size) +      /*data space            */
                attr->data_size;                /*the data itself       */

    FUNC_LEAVE(ret_value);

    f = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5O_attr_reset
 *
 * Purpose:     Frees resources within a attribute message, but doesn't free
 *              the message itself.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_attr_reset(void *_mesg)
{
    H5A_t                  *attr = (H5A_t *) _mesg;
    H5A_t                  *tmp = NULL;

    FUNC_ENTER(H5O_attr_reset, FAIL);

    if (attr) {
        if (NULL==(tmp = H5MM_malloc(sizeof(H5A_t)))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                           "memory allocation failed");
        }
        HDmemcpy(tmp,attr,sizeof(H5A_t));
        H5A_close(tmp);
        HDmemset(attr, 0, sizeof(H5A_t));
    }
    FUNC_LEAVE(SUCCEED);
}

/*--------------------------------------------------------------------------
 NAME
    H5O_attr_debug
 PURPOSE
    Prints debugging information for an attribute message
 USAGE
    void *H5O_attr_debug(f, mesg, stream, indent, fwidth)
        H5F_t *f;               IN: pointer to the HDF5 file struct
        const void *mesg;       IN: Pointer to the source attribute struct
        FILE *stream;           IN: Pointer to the stream for output data
        int indent;            IN: Amount to indent information by
        int fwidth;            IN: Field width (?)
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
        This function prints debugging output to the stream passed as a 
    parameter.
--------------------------------------------------------------------------*/
static herr_t
H5O_attr_debug(H5F_t *f, const void *_mesg, FILE * stream, int indent,
               int fwidth)
{
    const H5A_t *mesg = (const H5A_t *)_mesg;

    FUNC_ENTER(H5O_attr_debug, FAIL);

    /* check args */
    assert(f);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    fprintf(stream, "%*s%-*s \"%s\"\n", indent, "", fwidth,
            "Name:",
            mesg->name);
    fprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
            "Initialized:",
            (unsigned int)mesg->initialized);
    fprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
            "Opened:",
            (unsigned int)mesg->ent_opened);
    fprintf(stream, "%*sSymbol table entry...\n", indent, "");
    H5G_ent_debug(f, &(mesg->ent), stream, indent+3, MAX(0, fwidth-3),
                  HADDR_UNDEF);
    
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Data type size:",
            (unsigned long)(mesg->dt_size));
    fprintf(stream, "%*sData type...\n", indent, "");
    (H5O_DTYPE->debug)(f, mesg->dt, stream, indent+3, MAX(0, fwidth-3));

    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Data space size:",
            (unsigned long)(mesg->ds_size));
    fprintf(stream, "%*sData space...\n", indent, "");
    H5S_debug(f, mesg->ds, stream, indent+3, MAX(0, fwidth-3));

    FUNC_LEAVE(SUCCEED);
}
