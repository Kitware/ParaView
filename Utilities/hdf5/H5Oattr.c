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

#define H5A_PACKAGE             /*prevent warning from including H5Apkg   */
#define H5O_PACKAGE    /*suppress error about including H5Opkg    */
#define H5S_PACKAGE          /*suppress error about including H5Spkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5Apkg.h"    /* Attributes        */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free Lists        */
#include "H5Gprivate.h"    /* Groups        */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Opkg.h"             /* Object headers      */
#include "H5Spkg.h"    /* Dataspaces        */

/* PRIVATE PROTOTYPES */
static herr_t H5O_attr_encode (H5F_t *f, uint8_t *p, const void *mesg);
static void *H5O_attr_decode (H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static void *H5O_attr_copy (const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_attr_size (const H5F_t *f, const void *_mesg);
static herr_t H5O_attr_reset (void *_mesg);
static herr_t H5O_attr_free (void *mesg);
static herr_t H5O_attr_delete (H5F_t *f, hid_t dxpl_id, const void *_mesg, hbool_t adj_link);
static herr_t H5O_attr_link(H5F_t *f, hid_t dxpl_id, const void *_mesg);
static herr_t H5O_attr_debug (H5F_t *f, hid_t dxpl_id, const void *_mesg,
            FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_ATTR[1] = {{
    H5O_ATTR_ID,    /* message id number            */
    "attribute",    /* message name for debugging   */
    sizeof(H5A_t),    /* native message size          */
    H5O_attr_decode,    /* decode message               */
    H5O_attr_encode,    /* encode message               */
    H5O_attr_copy,    /* copy the native value        */
    H5O_attr_size,    /* size of raw message          */
    H5O_attr_reset,    /* reset method                 */
    H5O_attr_free,          /* free method      */
    H5O_attr_delete,    /* file delete method    */
    H5O_attr_link,    /* link method      */
    NULL,      /* get share method    */
    NULL,      /* set share method    */
    H5O_attr_debug,    /* debug the message            */
}};

/* This is the initial version, which does not have support for shared datatypes */
#define H5O_ATTR_VERSION  1

/* This version allows support for shared datatypes */
#define H5O_ATTR_VERSION_NEW  2

/* Flags for attribute flag encoding */
#define H5O_ATTR_FLAG_TYPE_SHARED       0x01

/* Declare extern the free list for H5A_t's */
H5FL_EXTERN(H5A_t);

/* Declare extern the free list for attribute data buffers */
H5FL_BLK_EXTERN(attr_buf);

/* Declare external the free list for H5S_t's */
H5FL_EXTERN(H5S_t);

/* Declare external the free list for H5S_extent_t's */
H5FL_EXTERN(H5S_extent_t);

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
 *   Robb Matzke, 17 Jul 1998
 *  Added padding for alignment.
 *
 *   Robb Matzke, 20 Jul 1998
 *  Added a version number at the beginning.
 *
 *  Raymond Lu, 8 April 2004
 *  Changed Dataspace operation on H5S_simple_t to H5S_extent_t.
 *
--------------------------------------------------------------------------*/
static void *
H5O_attr_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5A_t    *attr = NULL;
    H5S_extent_t  *extent;  /*extent dimensionality information  */
    size_t    name_len;     /*attribute name length */
    int            version;  /*message version number*/
    unsigned            flags=0;        /* Attribute flags */
    H5A_t    *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_attr_decode);

    /* check args */
    assert(f);
    assert(p);

    if (NULL==(attr = H5FL_CALLOC(H5A_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Version number */
    version = *p++;
    if (version!=H5O_ATTR_VERSION && version!=H5O_ATTR_VERSION_NEW)
  HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for attribute message");

    /* Get the flags byte if we have a later version of the attribute */
    if(version>H5O_ATTR_VERSION)
        flags = *p++;
    else
        p++;    /* Byte is unused when version<2 */

    /*
     * Decode the sizes of the parts of the attribute.  The sizes stored in
     * the file are exact but the parts are aligned on 8-byte boundaries.
     */
    UINT16DECODE(p, name_len); /*including null*/
    UINT16DECODE(p, attr->dt_size);
    UINT16DECODE(p, attr->ds_size);

    /* Decode and store the name */
    if (NULL==(attr->name=H5MM_strdup((const char *)p)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    if(version < H5O_ATTR_VERSION_NEW)
        p += H5O_ALIGN(name_len);    /* advance the memory pointer */
    else
        p += name_len;    /* advance the memory pointer */

    /* decode the attribute datatype */
    if (flags & H5O_ATTR_FLAG_TYPE_SHARED) {
  H5O_shared_t *shared;   /* Shared information */

        /* Get the shared information */
  if (NULL == (shared = (H5O_SHARED->decode) (f, dxpl_id, p, NULL)))
      HGOTO_ERROR(H5E_OHDR, H5E_CANTDECODE, NULL, "unable to decode shared message");

        /* Get the actual datatype information */
        if((attr->dt= H5O_shared_read(f, dxpl_id, shared, H5O_DTYPE, NULL))==NULL)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTDECODE, NULL, "can't decode attribute datatype");

        /* Free the shared information */
        H5O_free_real(H5O_SHARED, shared);
    } /* end if */
    else {
        if((attr->dt=(H5O_DTYPE->decode)(f,dxpl_id,p,NULL))==NULL)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTDECODE, NULL, "can't decode attribute datatype");
    } /* end else */
    if(version < H5O_ATTR_VERSION_NEW)
        p += H5O_ALIGN(attr->dt_size);
    else
        p += attr->dt_size;

    /* decode the attribute dataspace */
    if (NULL==(attr->ds = H5FL_CALLOC(H5S_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    if((extent=(H5O_SDSPACE->decode)(f,dxpl_id,p,NULL))==NULL)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTDECODE, NULL, "can't decode attribute dataspace");

    /* Copy the extent information */
    HDmemcpy(&(attr->ds->extent),extent, sizeof(H5S_extent_t));

    /* Release temporary extent information */
    H5FL_FREE(H5S_extent_t,extent);

    /* Default to entire dataspace being selected */
    if(H5S_select_all(attr->ds,0)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTSET, NULL, "unable to set all selection");

    if(version < H5O_ATTR_VERSION_NEW)
        p += H5O_ALIGN(attr->ds_size);
    else
        p += attr->ds_size;

    /* Compute the size of the data */
    H5_ASSIGN_OVERFLOW(attr->data_size,H5S_GET_EXTENT_NPOINTS(attr->ds)*H5T_get_size(attr->dt),hsize_t,size_t);

    /* Go get the data */
    if(attr->data_size) {
        if (NULL==(attr->data = H5FL_BLK_MALLOC(attr_buf, attr->data_size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
        HDmemcpy(attr->data,p,attr->data_size);
    }

    /* Indicate that the fill values aren't to be written out */
    attr->initialized=1;

    /* Set return value */
    ret_value=attr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
 *   Robb Matzke, 17 Jul 1998
 *  Added padding for alignment.
 *
 *   Robb Matzke, 20 Jul 1998
 *  Added a version number at the beginning.
 *
 *  Raymond Lu, 8 April 2004
 *  For data space, changed the operation on H5S_simple_t to
 *  H5S_extent_t
 *
--------------------------------------------------------------------------*/
static herr_t
H5O_attr_encode(H5F_t *f, uint8_t *p, const void *mesg)
{
    const H5A_t *attr = (const H5A_t *) mesg;
    size_t      name_len;   /* Attribute name length */
    unsigned    version;        /* Attribute version */
    hbool_t     type_shared;    /* Flag to indicate that a shared datatype is used for this attribute */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_attr_encode);

    /* check args */
    assert(f);
    assert(p);
    assert(attr);

    /* Check whether datatype is shared */
    if(H5T_committed(attr->dt))
        type_shared = TRUE;
    else
        type_shared = FALSE;

    /* Check which version to write out */
    if(type_shared)
        version = H5O_ATTR_VERSION_NEW;  /* Write out new version if shared datatype */
    else
        version = H5O_ATTR_VERSION;

    /* Encode Version */
    *p++ = version;

    /* Set attribute flags if version >1 */
    if(version>H5O_ATTR_VERSION)
        *p++ = (type_shared ? H5O_ATTR_FLAG_TYPE_SHARED : 0 );    /* Set flags for attribute */
    else
        *p++ = 0; /* Reserved, for version <2 */

    /*
     * Encode the lengths of the various parts of the attribute message. The
     * encoded lengths are exact but we pad each part except the data to be a
     * multiple of eight bytes (in the first version).
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
    if(version < H5O_ATTR_VERSION_NEW)
        p += H5O_ALIGN(name_len);
    else
        p += name_len;

    /* encode the attribute datatype */
    if(type_shared) {
        H5O_shared_t  sh_mesg;

        /* Reset shared message information */
        HDmemset(&sh_mesg,0,sizeof(H5O_shared_t));

        /* Get shared message information from datatype */
        if ((H5O_DTYPE->get_share)(f, attr->dt, &sh_mesg/*out*/)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "can't encode shared attribute datatype");

        /* Encode shared message information for datatype */
        if((H5O_SHARED->encode)(f,p,&sh_mesg)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "can't encode shared attribute datatype");
    } /* end if */
    else {
        /* Encode datatype information */
        if((H5O_DTYPE->encode)(f,p,attr->dt)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "can't encode attribute datatype");
    } /* end else */
    if(version < H5O_ATTR_VERSION_NEW) {
        HDmemset(p+attr->dt_size, 0, H5O_ALIGN(attr->dt_size)-attr->dt_size);
        p += H5O_ALIGN(attr->dt_size);
    } /* end if */
    else
        p += attr->dt_size;

    /* encode the attribute dataspace */
    if((H5O_SDSPACE->encode)(f,p,&(attr->ds->extent))<0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "can't encode attribute dataspace");
    if(version < H5O_ATTR_VERSION_NEW) {
        HDmemset(p+attr->ds_size, 0, H5O_ALIGN(attr->ds_size)-attr->ds_size);
        p += H5O_ALIGN(attr->ds_size);
    } /* end if */
    else
        p += attr->ds_size;

    /* Store attribute data */
    if(attr->data)
        HDmemcpy(p,attr->data,attr->data_size);
    else
        HDmemset(p,0,attr->data_size);

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
static void *
H5O_attr_copy(const void *_src, void *_dst, unsigned update_flags)
{
    const H5A_t            *src = (const H5A_t *) _src;
    void                   *ret_value;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_attr_copy);

    /* check args */
    assert(src);

    /* copy */
    if (NULL == (ret_value = H5A_copy(_dst,src,update_flags)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, NULL, "can't copy attribute");

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
 *   Robb Matzke, 17 Jul 1998
 *  Added padding between message parts for alignment.
--------------------------------------------------------------------------*/
static size_t
H5O_attr_size(const H5F_t UNUSED *f, const void *_mesg)
{
    const H5A_t         *attr = (const H5A_t *)_mesg;
    size_t    name_len;
    unsigned            version;        /* Attribute version */
    hbool_t             type_shared;    /* Flag to indicate that a shared datatype is used for this attribute */
    size_t    ret_value = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_attr_size);

    assert(attr);

    name_len = HDstrlen(attr->name)+1;

    /* Check whether datatype is shared */
    if(H5T_committed(attr->dt))
        type_shared = TRUE;
    else
        type_shared = FALSE;

    /* Check which version to write out */
    if(type_shared)
        version = H5O_ATTR_VERSION_NEW;  /* Write out new version if shared datatype */
    else
        version = H5O_ATTR_VERSION;

    if(version < H5O_ATTR_VERSION_NEW)
        ret_value = 1 +        /*version               */
                    1 +        /*reserved    */
                    2 +        /*name size inc. null  */
                    2 +        /*type size    */
                    2 +        /*space size    */
                    H5O_ALIGN(name_len)  +  /*attribute name  */
                    H5O_ALIGN(attr->dt_size) +  /*data type    */
                    H5O_ALIGN(attr->ds_size) +  /*data space    */
                    attr->data_size;    /*the data itself  */
    else
        ret_value = 1 +        /*version               */
                    1 +        /*flags      */
                    2 +        /*name size inc. null  */
                    2 +        /*type size    */
                    2 +        /*space size    */
                    name_len  +    /*attribute name  */
                    attr->dt_size +    /*data type    */
                    attr->ds_size +    /*data space    */
                    attr->data_size;    /*the data itself  */

    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_attr_reset);

    if (attr)
        H5A_free(attr);

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_attr_free
 *
 * Purpose:  Free's the message
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, November 18, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_attr_free (void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_attr_free);

    assert (mesg);

    H5FL_FREE(H5A_t,mesg);

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* end H5O_attr_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O_attr_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 26, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_attr_delete(H5F_t UNUSED *f, hid_t dxpl_id, const void *_mesg, hbool_t adj_link)
{
    const H5A_t            *attr = (const H5A_t *) _mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_attr_delete);

    /* check args */
    assert(f);
    assert(attr);

    /* Check whether datatype is shared */
    if(H5T_committed(attr->dt)) {
        /* Decrement the reference count on the shared datatype, if requested */
        if(adj_link)
            if(H5T_link(attr->dt, -1, dxpl_id)<0)
                HGOTO_ERROR (H5E_OHDR, H5E_LINK, FAIL, "unable to adjust shared datatype link count")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_attr_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O_attr_link
 *
 * Purpose:     Increment reference count on any objects referenced by
 *              message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 26, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_attr_link(H5F_t UNUSED *f, hid_t dxpl_id, const void *_mesg)
{
    const H5A_t            *attr = (const H5A_t *) _mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_attr_link);

    /* check args */
    assert(f);
    assert(attr);

    /* Check whether datatype is shared */
    if(H5T_committed(attr->dt)) {
        /* Increment the reference count on the shared datatype */
        if(H5T_link(attr->dt,1,dxpl_id)<0)
            HGOTO_ERROR (H5E_OHDR, H5E_LINK, FAIL, "unable to adjust shared datatype link count");
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_attr_link() */


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
H5O_attr_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE * stream, int indent,
         int fwidth)
{
    const H5A_t *mesg = (const H5A_t *)_mesg;
    H5O_shared_t  sh_mesg;        /* Shared message information */
    void *dt_mesg;                      /* Pointer to datatype message to dump */
    herr_t      (*debug)(H5F_t*, hid_t, const void*, FILE*, int, int)=NULL;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_attr_debug);

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
    H5G_ent_debug(f, dxpl_id, &(mesg->ent), stream, indent+3, MAX(0, fwidth-3),
      HADDR_UNDEF);

    fprintf(stream, "%*sData type...\n", indent, "");
    fprintf(stream, "%*s%-*s %lu\n", indent+3, "", MAX(0,fwidth-3),
      "Size:",
      (unsigned long)(mesg->dt_size));
    fprintf (stream, "%*s%-*s %s\n", indent+3, "", MAX(0,fwidth-3),
               "Shared:",
               (H5T_committed(mesg->dt) ? "Yes" : "No")
               );
    if(H5T_committed(mesg->dt)) {
        /* Reset shared message information */
        HDmemset(&sh_mesg,0,sizeof(H5O_shared_t));

        /* Get shared message information from datatype */
        if ((H5O_DTYPE->get_share)(f, mesg->dt, &sh_mesg/*out*/)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "can't retrieve shared message information");

        debug=H5O_SHARED->debug;
        dt_mesg=&sh_mesg;
    } /* end if */
    else {
        debug=H5O_DTYPE->debug;
        dt_mesg=mesg->dt;
    } /* end else */
    if(debug)
        (debug)(f, dxpl_id, dt_mesg, stream, indent+3, MAX(0, fwidth-3));
    else
        fprintf(stream, "%*s<No info for this message>\n", indent + 6, "");

    fprintf(stream, "%*sData space...\n", indent, "");
    fprintf(stream, "%*s%-*s %lu\n", indent+3, "", MAX(0,fwidth-3),
      "Size:",
      (unsigned long)(mesg->ds_size));
    H5S_debug(f, dxpl_id, mesg->ds, stream, indent+3, MAX(0, fwidth-3));

done:
    FUNC_LEAVE_NOAPI(ret_value);
}
