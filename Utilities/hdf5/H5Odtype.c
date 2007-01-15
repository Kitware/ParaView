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

#define H5O_PACKAGE    /*suppress error about including H5Opkg    */
#define H5T_PACKAGE    /*prevent warning from including H5Tpkg   */

#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free Lists        */
#include "H5Gprivate.h"    /* Groups        */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Opkg.h"             /* Object headers      */
#include "H5Tpkg.h"    /* Datatypes        */


/* PRIVATE PROTOTYPES */
static herr_t H5O_dtype_encode (H5F_t *f, uint8_t *p, const void *mesg);
static void *H5O_dtype_decode (H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static void *H5O_dtype_copy (const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_dtype_size (const H5F_t *f, const void *_mesg);
static herr_t H5O_dtype_reset (void *_mesg);
static herr_t H5O_dtype_free (void *_mesg);
static herr_t H5O_dtype_get_share (H5F_t *f, const void *_mesg,
           H5O_shared_t *sh);
static herr_t H5O_dtype_set_share (H5F_t *f, void *_mesg,
           const H5O_shared_t *sh);
static herr_t H5O_dtype_debug (H5F_t *f, hid_t dxpl_id, const void *_mesg,
             FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_DTYPE[1] = {{
    H5O_DTYPE_ID,    /* message id number    */
    "data_type",    /* message name for debugging  */
    sizeof(H5T_t),    /* native message size    */
    H5O_dtype_decode,    /* decode message    */
    H5O_dtype_encode,    /* encode message    */
    H5O_dtype_copy,    /* copy the native value  */
    H5O_dtype_size,    /* size of raw message    */
    H5O_dtype_reset,    /* reset method      */
    H5O_dtype_free,    /* free method      */
    NULL,            /* file delete method    */
    NULL,      /* link method      */
    H5O_dtype_get_share,  /* get share method    */
    H5O_dtype_set_share,  /* set share method    */
    H5O_dtype_debug,    /* debug the message    */
}};

/* This is the correct version to create all datatypes which don't contain
 * array datatypes (atomic types, compound datatypes without array fields,
 * vlen sequences of objects which aren't arrays, etc.) */
#define H5O_DTYPE_VERSION_COMPAT  1

/* This is the correct version to create all datatypes which contain H5T_ARRAY
 * class objects (array definitely, potentially compound & vlen sequences also) */
#define H5O_DTYPE_VERSION_UPDATED  2

/* Declare external the free list for H5T_t's */
H5FL_EXTERN(H5T_t);
H5FL_EXTERN(H5T_shared_t);


/*-------------------------------------------------------------------------
 * Function:  H5O_dtype_decode_helper
 *
 * Purpose:  Decodes a datatype
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Monday, December  8, 1997
 *
 * Modifications:
 *    Robb Matzke, Thursday, May 20, 1999
 *    Added support for bitfields and opaque datatypes.
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_decode_helper(H5F_t *f, const uint8_t **pp, H5T_t *dt)
{
    unsigned    flags, version;
    unsigned    i, j;
    size_t    z;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dtype_decode_helper);

    /* check args */
    assert(pp && *pp);
    assert(dt && dt->shared);

    /* decode */
    UINT32DECODE(*pp, flags);
    version = (flags>>4) & 0x0f;
    if (version!=H5O_DTYPE_VERSION_COMPAT && version!=H5O_DTYPE_VERSION_UPDATED)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTLOAD, FAIL, "bad version number for datatype message");
    dt->shared->type = (H5T_class_t)(flags & 0x0f);
    flags >>= 8;
    UINT32DECODE(*pp, dt->shared->size);

    switch (dt->shared->type) {
        case H5T_INTEGER:
            /*
             * Integer types...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.u.i.sign = (flags & 0x8) ? H5T_SGN_2 : H5T_SGN_NONE;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_BITFIELD:
            /*
             * Bit fields...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_OPAQUE:
            /*
             * Opaque types...
             */
            z = flags & (H5T_OPAQUE_TAG_MAX - 1);
            assert(0==(z&0x7)); /*must be aligned*/
            if (NULL==(dt->shared->u.opaque.tag=H5MM_malloc(z+1)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
            HDmemcpy(dt->shared->u.opaque.tag, *pp, z);
            dt->shared->u.opaque.tag[z] = '\0';
            *pp += z;
            break;

        case H5T_FLOAT:
            /*
             * Floating-point types...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.u.f.pad = (flags & 0x8) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            switch ((flags >> 4) & 0x03) {
                case 0:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_NONE;
                    break;
                case 1:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_MSBSET;
                    break;
                case 2:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown floating-point normalization");
            }
            dt->shared->u.atomic.u.f.sign = (flags >> 8) & 0xff;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            dt->shared->u.atomic.u.f.epos = *(*pp)++;
            dt->shared->u.atomic.u.f.esize = *(*pp)++;
            assert(dt->shared->u.atomic.u.f.esize > 0);
            dt->shared->u.atomic.u.f.mpos = *(*pp)++;
            dt->shared->u.atomic.u.f.msize = *(*pp)++;
            assert(dt->shared->u.atomic.u.f.msize > 0);
            UINT32DECODE(*pp, dt->shared->u.atomic.u.f.ebias);
            break;

        case H5T_COMPOUND:
            /*
             * Compound datatypes...
             */
            dt->shared->u.compnd.nmembs = flags & 0xffff;
            assert(dt->shared->u.compnd.nmembs > 0);
            dt->shared->u.compnd.packed = TRUE; /* Start off packed */
            dt->shared->u.compnd.nalloc = dt->shared->u.compnd.nmembs;
            dt->shared->u.compnd.memb = H5MM_calloc(dt->shared->u.compnd.nalloc*
                            sizeof(H5T_cmemb_t));
            if (NULL==dt->shared->u.compnd.memb)
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
            for (i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                unsigned ndims=0;     /* Number of dimensions of the array field */
                hsize_t dim[H5O_LAYOUT_NDIMS];  /* Dimensions of the array */
                int perm[H5O_LAYOUT_NDIMS];     /* Dimension permutations */
                unsigned perm_word=0;    /* Dimension permutation information */
                H5T_t *array_dt;    /* Temporary pointer to the array datatype */
                H5T_t *temp_type;   /* Temporary pointer to the field's datatype */

                /* Decode the field name */
                dt->shared->u.compnd.memb[i].name = H5MM_xstrdup((const char *)*pp);
                /*multiple of 8 w/ null terminator */
                *pp += ((HDstrlen((const char *)*pp) + 8) / 8) * 8;

                /* Decode the field offset */
                UINT32DECODE(*pp, dt->shared->u.compnd.memb[i].offset);

                /* Older versions of the library allowed a field to have
                 * intrinsic 'arrayness'.  Newer versions of the library
                 * use the separate array datatypes. */
                if(version==H5O_DTYPE_VERSION_COMPAT) {
                    /* Decode the number of dimensions */
                    ndims = *(*pp)++;
                    assert(ndims <= 4);
                    *pp += 3;    /*reserved bytes */

                    /* Decode dimension permutation (unused currently) */
                    UINT32DECODE(*pp, perm_word);

                    /* Skip reserved bytes */
                    *pp += 4;

                    /* Decode array dimension sizes */
                    for (j=0; j<4; j++)
                        UINT32DECODE(*pp, dim[j]);
                } /* end if */

                /* Allocate space for the field's datatype */
                if(NULL == (temp_type = H5T_alloc()))
                    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

                /* Decode the field's datatype information */
                if (H5O_dtype_decode_helper(f, pp, temp_type)<0) {
                    for (j=0; j<=i; j++)
                        H5MM_xfree(dt->shared->u.compnd.memb[j].name);
                    H5MM_xfree(dt->shared->u.compnd.memb);
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode member type");
                }

                /* Go create the array datatype now, for older versions of the datatype message */
                if(version==H5O_DTYPE_VERSION_COMPAT) {
                    /* Check if this member is an array field */
                    if(ndims>0) {
                        /* Set up the permutation vector for the array create */
                        for (j=0; j<ndims; j++)
                            perm[j]=(perm_word>>(j*8))&0xff;

                        /* Create the array datatype for the field */
                        if ((array_dt=H5T_array_create(temp_type,(int)ndims,dim,perm))==NULL) {
                            for (j=0; j<=i; j++)
                                H5MM_xfree(dt->shared->u.compnd.memb[j].name);
                            H5MM_xfree(dt->shared->u.compnd.memb);
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to create array datatype");
                        }

                        /* Close the base type for the array */
                        H5T_close(temp_type);

                        /* Make the array type the type that is set for the field */
                        temp_type=array_dt;
                    } /* end if */
                } /* end if */

                /*
                 * Set the "force conversion" flag if VL datatype fields exist in this
                 * type or any component types
                 */
                if(temp_type->shared->force_conv==TRUE)
                    dt->shared->force_conv=TRUE;

                /* Member size */
                dt->shared->u.compnd.memb[i].size = temp_type->shared->size;

                /* Set the field datatype (finally :-) */
                dt->shared->u.compnd.memb[i].type=temp_type;

                /* Check if the datatype stayed packed */
                if(dt->shared->u.compnd.packed) {
                    /* Check if the member type is packed */
                    if(H5T_is_packed(temp_type)>0) {
                        if(i==0) {
                            /* If the is the first member, the datatype is not packed
                             * if the first member isn't at offset 0
                             */
                            if(dt->shared->u.compnd.memb[i].offset>0)
                                dt->shared->u.compnd.packed=FALSE;
                        } /* end if */
                        else {
                            /* If the is not the first member, the datatype is not
                             * packed if the new member isn't adjoining the previous member
                             */
                            if(dt->shared->u.compnd.memb[i].offset!=(dt->shared->u.compnd.memb[i-1].offset+dt->shared->u.compnd.memb[i-1].size))
                                dt->shared->u.compnd.packed=FALSE;
                        } /* end else */
                    } /* end if */
                    else
                        dt->shared->u.compnd.packed=FALSE;
                } /* end if */
            }
            break;

        case H5T_ENUM:
            /*
             * Enumeration datatypes...
             */
            dt->shared->u.enumer.nmembs = dt->shared->u.enumer.nalloc = flags & 0xffff;
            if(NULL == (dt->shared->parent = H5T_alloc()))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if (H5O_dtype_decode_helper(f, pp, dt->shared->parent)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode parent datatype");
            if (NULL==(dt->shared->u.enumer.name=H5MM_calloc(dt->shared->u.enumer.nalloc * sizeof(char*))) ||
                    NULL==(dt->shared->u.enumer.value=H5MM_calloc(dt->shared->u.enumer.nalloc *
                    dt->shared->parent->shared->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

            /* Names, each a multiple of 8 with null termination */
            for (i=0; i<dt->shared->u.enumer.nmembs; i++) {
                dt->shared->u.enumer.name[i] = H5MM_xstrdup((const char*)*pp);
                *pp += ((HDstrlen((const char*)*pp)+8)/8)*8;
            }

            /* Values */
            HDmemcpy(dt->shared->u.enumer.value, *pp,
                 dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size);
            *pp += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_REFERENCE: /* Reference datatypes...  */
            dt->shared->u.atomic.order = H5T_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;
            dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;

            /* Set reference type */
            dt->shared->u.atomic.u.r.rtype = (H5R_type_t)(flags & 0x0f);
            break;

        case H5T_STRING:
            /*
             * Character string types...
             */
            dt->shared->u.atomic.order = H5T_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;
            dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;

            dt->shared->u.atomic.u.s.pad = (H5T_str_t)(flags & 0x0f);
            dt->shared->u.atomic.u.s.cset = (H5T_cset_t)((flags>>4) & 0x0f);
            break;

        case H5T_VLEN:  /* Variable length datatypes...  */
            /* Set the type of VL information, either sequence or string */
            dt->shared->u.vlen.type = (H5T_vlen_type_t)(flags & 0x0f);
            if(dt->shared->u.vlen.type == H5T_VLEN_STRING) {
                dt->shared->u.vlen.pad  = (H5T_str_t)((flags>>4) & 0x0f);
                dt->shared->u.vlen.cset = (H5T_cset_t)((flags>>8) & 0x0f);
            } /* end if */

            /* Decode base type of VL information */
            if(NULL == (dt->shared->parent = H5T_alloc()))
                HGOTO_ERROR (H5E_DATATYPE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if (H5O_dtype_decode_helper(f, pp, dt->shared->parent)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode VL parent type");

            dt->shared->force_conv=TRUE;
            /* Mark this type as on disk */
            if (H5T_vlen_mark(dt, f, H5T_VLEN_DISK)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");
            break;

        case H5T_TIME:  /* Time datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_ARRAY:  /* Array datatypes...  */
            /* Decode the number of dimensions */
            dt->shared->u.array.ndims = *(*pp)++;

            /* Double-check the number of dimensions */
            assert(dt->shared->u.array.ndims <= H5S_MAX_RANK);

            /* Skip reserved bytes */
            *pp += 3;

            /* Decode array dimension sizes & compute number of elements */
            for (j=0, dt->shared->u.array.nelem=1; j<(unsigned)dt->shared->u.array.ndims; j++) {
                UINT32DECODE(*pp, dt->shared->u.array.dim[j]);
                dt->shared->u.array.nelem *= dt->shared->u.array.dim[j];
            } /* end for */

            /* Decode array dimension permutations (even though they are unused currently) */
            for (j=0; j<(unsigned)dt->shared->u.array.ndims; j++)
                UINT32DECODE(*pp, dt->shared->u.array.perm[j]);

            /* Decode base type of array */
            if(NULL == (dt->shared->parent = H5T_alloc()))
                HGOTO_ERROR (H5E_DATATYPE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if (H5O_dtype_decode_helper(f, pp, dt->shared->parent)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode VL parent type");

            /*
             * Set the "force conversion" flag if a VL base datatype is used or
             * or if any components of the base datatype are VL types.
             */
            if(dt->shared->parent->shared->force_conv==TRUE)
                dt->shared->force_conv=TRUE;
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown datatype class found");
    }

done:
    if(ret_value <0)
    {
        if(dt != NULL) {
            if(dt->shared != NULL)
                H5FL_FREE(H5T_shared_t, dt->shared);
            H5FL_FREE(H5T_t, dt);
        } /* end if */
    }
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_dtype_encode_helper
 *
 * Purpose:  Encodes a datatype.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Monday, December  8, 1997
 *
 * Modifications:
 *    Robb Matzke, Thursday, May 20, 1999
 *    Added support for bitfields and opaque types.
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_encode_helper(uint8_t **pp, const H5T_t *dt)
{
    htri_t has_array=FALSE;       /* Whether a compound datatype has an array inside it */
    unsigned    flags = 0;
    char    *hdr = (char *)*pp;
    unsigned    i, j;
    size_t    n, z, aligned;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dtype_encode_helper);

    /* check args */
    assert(pp && *pp);
    assert(dt);

    /* skip the type and class bit-field for now */
    *pp += 4;
    UINT32ENCODE(*pp, dt->shared->size);

    switch (dt->shared->type) {
        case H5T_INTEGER:
            /*
             * Integer datatypes...
             */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;    /*nothing */
                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.lsb_pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x02;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.msb_pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x04;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.u.i.sign) {
                case H5T_SGN_NONE:
                    break;    /*nothing */
                case H5T_SGN_2:
                    flags |= 0x08;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "sign scheme is not supported in file format yet");
            }

            UINT16ENCODE(*pp, dt->shared->u.atomic.offset);
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_BITFIELD:
            /*
             * Bitfield datatypes...
             */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;    /*nothing */
                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.lsb_pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x02;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.msb_pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x04;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            UINT16ENCODE(*pp, dt->shared->u.atomic.offset);
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_OPAQUE:
            /*
             * Opaque datatypes...  The tag is stored in a field which is a
             * multiple of eight characters and null padded (not necessarily
             * null terminated).
             */
            z = HDstrlen(dt->shared->u.opaque.tag);
            aligned = (z+7) & (H5T_OPAQUE_TAG_MAX - 8);
            flags |= aligned;
            HDmemcpy(*pp, dt->shared->u.opaque.tag, MIN(z,aligned));
            for (n=MIN(z,aligned); n<aligned; n++) (*pp)[n] = 0;
            *pp += aligned;
            break;

        case H5T_FLOAT:
            /*
             * Floating-point types...
             */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;    /*nothing */
                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.lsb_pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x02;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.msb_pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x04;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.u.f.pad) {
                case H5T_PAD_ZERO:
                    break;    /*nothing */
                case H5T_PAD_ONE:
                    flags |= 0x08;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet");
            }

            switch (dt->shared->u.atomic.u.f.norm) {
                case H5T_NORM_NONE:
                    break;    /*nothing */
                case H5T_NORM_MSBSET:
                    flags |= 0x10;
                    break;
                case H5T_NORM_IMPLIED:
                    flags |= 0x20;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "normalization scheme is not supported in file format yet");
            }

            flags |= (dt->shared->u.atomic.u.f.sign << 8) & 0xff00;
            UINT16ENCODE(*pp, dt->shared->u.atomic.offset);
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            assert (dt->shared->u.atomic.u.f.epos<=255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.epos);
            assert (dt->shared->u.atomic.u.f.esize<=255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.esize);
            assert (dt->shared->u.atomic.u.f.mpos<=255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.mpos);
            assert (dt->shared->u.atomic.u.f.msize<=255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.msize);
            UINT32ENCODE(*pp, dt->shared->u.atomic.u.f.ebias);
            break;

        case H5T_COMPOUND:
            /* Check for an array datatype somewhere within the compound type */
            if((has_array=H5T_detect_class(dt,H5T_ARRAY))<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "can't detect array class");

            /*
             * Compound datatypes...
             */
            flags = dt->shared->u.compnd.nmembs & 0xffff;
            for (i=0; i<dt->shared->u.compnd.nmembs; i++) {

                /* Name, multiple of eight bytes */
                HDstrcpy((char*)(*pp), dt->shared->u.compnd.memb[i].name);
                n = HDstrlen(dt->shared->u.compnd.memb[i].name);
                for (z=n+1; z%8; z++)
                    (*pp)[z] = '\0';
                *pp += z;

                /* Member offset */
                UINT32ENCODE(*pp, dt->shared->u.compnd.memb[i].offset);

                /* If we don't have any array fields, write out the old style
                 * member information, for better backward compatibility
                 * Write out all zeros for the array information, though...
                 */
                if(!has_array) {
                    /* Dimensionality */
                    *(*pp)++ = 0;

                    /* Reserved */
                    *(*pp)++ = 0;
                    *(*pp)++ = 0;
                    *(*pp)++ = 0;

                    /* Dimension permutation */
                    UINT32ENCODE(*pp, 0);

                    /* Reserved */
                    UINT32ENCODE(*pp, 0);

                    /* Dimensions */
                    for (j=0; j<4; j++)
                        UINT32ENCODE(*pp, 0);
                } /* end if */

                /* Subtype */
                if (H5O_dtype_encode_helper(pp, dt->shared->u.compnd.memb[i].type)<0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode member type");
            }
            break;

        case H5T_ENUM:
            /*
             * Enumeration datatypes...
             */
            flags = dt->shared->u.enumer.nmembs & 0xffff;

            /* Parent type */
            if (H5O_dtype_encode_helper(pp, dt->shared->parent)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode parent datatype");

            /* Names, each a multiple of eight bytes */
            for (i=0; i<dt->shared->u.enumer.nmembs; i++) {
                HDstrcpy((char*)(*pp), dt->shared->u.enumer.name[i]);
                n = HDstrlen(dt->shared->u.enumer.name[i]);
                for (z=n+1; z%8; z++)
                    (*pp)[z] = '\0';
                *pp += z;
            }

            /* Values */
            HDmemcpy(*pp, dt->shared->u.enumer.value, dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size);
            *pp += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_REFERENCE:
            flags |= (dt->shared->u.atomic.u.r.rtype & 0x0f);
            break;

        case H5T_STRING:
            /*
             * Character string types... (not fully implemented)
             */
            assert (dt->shared->u.atomic.order == H5T_ORDER_NONE);
            assert (dt->shared->u.atomic.prec == 8 * dt->shared->size);
            assert (dt->shared->u.atomic.offset == 0);
            assert (dt->shared->u.atomic.lsb_pad == H5T_PAD_ZERO);
            assert (dt->shared->u.atomic.msb_pad == H5T_PAD_ZERO);

            flags |= (dt->shared->u.atomic.u.s.pad & 0x0f);
            flags |= (dt->shared->u.atomic.u.s.cset & 0x0f) << 4;
            break;

        case H5T_VLEN:  /* Variable length datatypes...  */
            flags |= (dt->shared->u.vlen.type & 0x0f);
            if(dt->shared->u.vlen.type == H5T_VLEN_STRING) {
                flags |= (dt->shared->u.vlen.pad   & 0x0f) << 4;
                flags |= (dt->shared->u.vlen.cset  & 0x0f) << 8;
            } /* end if */

            /* Encode base type of VL information */
            if (H5O_dtype_encode_helper(pp, dt->shared->parent)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode VL parent type");
            break;

        case H5T_TIME:  /* Time datatypes...  */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;    /*nothing */
                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet");
            }
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_ARRAY:  /* Array datatypes...  */
            /* Double-check the number of dimensions */
            assert(dt->shared->u.array.ndims <= H5S_MAX_RANK);

            /* Encode the number of dimensions */
            *(*pp)++ = dt->shared->u.array.ndims;

            /* Reserved */
            *(*pp)++ = '\0';
            *(*pp)++ = '\0';
            *(*pp)++ = '\0';

            /* Encode array dimensions */
            for (j=0; j<(unsigned)dt->shared->u.array.ndims; j++)
                UINT32ENCODE(*pp, dt->shared->u.array.dim[j]);

            /* Encode array dimension permutations */
            for (j=0; j<(unsigned)dt->shared->u.array.ndims; j++)
                UINT32ENCODE(*pp, dt->shared->u.array.perm[j]);

            /* Encode base type of array's information */
            if (H5O_dtype_encode_helper(pp, dt->shared->parent)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode VL parent type");
            break;

        default:
            /*nothing */
            break;
    }

    /* Encode the type's class, version and bit field */
    *hdr++ = ((unsigned)(dt->shared->type) & 0x0f) | (((dt->shared->type==H5T_COMPOUND && has_array) ? H5O_DTYPE_VERSION_UPDATED : H5O_DTYPE_VERSION_COMPAT )<<4);
    *hdr++ = (flags >> 0) & 0xff;
    *hdr++ = (flags >> 8) & 0xff;
    *hdr++ = (flags >> 16) & 0xff;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_decode
 PURPOSE
    Decode a message and return a pointer to a memory struct
  with the decoded information
 USAGE
    void *H5O_dtype_decode(f, raw_size, p)
  H5F_t *f;    IN: pointer to the HDF5 file struct
  size_t raw_size;  IN: size of the raw information buffer
  const uint8 *p;    IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
  This function decodes the "raw" disk form of a simple datatype message
    into a struct in memory native format.  The struct is allocated within this
    function using malloc() and is returned to the caller.
--------------------------------------------------------------------------*/
static void *
H5O_dtype_decode(H5F_t *f, hid_t UNUSED dxpl_id, const uint8_t *p,
     H5O_shared_t UNUSED *sh)
{
    H5T_t       *dt = NULL;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dtype_decode);

    /* check args */
    assert(p);

    if(NULL == (dt = H5T_alloc()))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    if (H5O_dtype_decode_helper(f, &p, dt) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, NULL, "can't decode type");

    /* Set return value */
    ret_value=dt;

done:
    if(ret_value==NULL) {
        if(dt != NULL) {
            if(dt->shared != NULL)
                H5FL_FREE(H5T_shared_t, dt->shared);
            H5FL_FREE(H5T_t, dt);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_encode
 PURPOSE
    Encode a simple datatype message
 USAGE
    herr_t H5O_dtype_encode(f, raw_size, p, mesg)
  H5F_t *f;    IN: pointer to the HDF5 file struct
  size_t raw_size;  IN: size of the raw information buffer
  const uint8 *p;    IN: the raw information buffer
  const void *mesg;  IN: Pointer to the simple datatype struct
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
  This function encodes the native memory form of the simple datatype
    message in the "raw" disk form.
--------------------------------------------------------------------------*/
static herr_t
H5O_dtype_encode(H5F_t UNUSED *f, uint8_t *p, const void *mesg)
{
    const H5T_t       *dt = (const H5T_t *) mesg;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dtype_encode);

    /* check args */
    assert(f);
    assert(p);
    assert(dt);

    /* encode */
    if (H5O_dtype_encode_helper(&p, dt) < 0)
  HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "can't encode type");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_copy
 PURPOSE
    Copies a message from MESG to DEST, allocating DEST if necessary.
 USAGE
    void *H5O_dtype_copy(mesg, dest)
  const void *mesg;  IN: Pointer to the source simple datatype
            struct
  const void *dest;  IN: Pointer to the destination simple
            datatype struct
 RETURNS
    Pointer to DEST on success, NULL on failure
 DESCRIPTION
  This function copies a native (memory) simple datatype message,
    allocating the destination structure if necessary.
--------------------------------------------------------------------------*/
static void *
H5O_dtype_copy(const void *_src, void *_dst, unsigned UNUSED update_flags)
{
    const H5T_t       *src = (const H5T_t *) _src;
    H5T_t       *dst = NULL;
    void        *ret_value;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dtype_copy);

    /* check args */
    assert(src);

    /* copy */
    if (NULL == (dst = H5T_copy(src, H5T_COPY_ALL)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "can't copy type");

    /* was result already allocated? */
    if (_dst) {
        *((H5T_t *) _dst) = *dst;
        H5FL_FREE(H5T_t,dst);
        dst = (H5T_t *) _dst;
    }

    /* Set return value */
    ret_value=dst;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_size
 PURPOSE
    Return the raw message size in bytes
 USAGE
    void *H5O_dtype_size(f, mesg)
  H5F_t *f;    IN: pointer to the HDF5 file struct
  const void *mesg;     IN: Pointer to the source simple datatype struct
 RETURNS
    Size of message on success, 0 on failure
 DESCRIPTION
  This function returns the size of the raw simple datatype message on
    success.  (Not counting the message type or size fields, only the data
    portion of the message).  It doesn't take into account alignment.
 NOTES
        All datatype messages have a common 8 byte header, plus a variable-
    sized "properties" field.
--------------------------------------------------------------------------*/
static size_t
H5O_dtype_size(const H5F_t *f, const void *mesg)
{
    unsigned        i;
    size_t        ret_value = 8;
    const H5T_t       *dt = (const H5T_t *) mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_dtype_size);

    assert(mesg);

    /* Add in the property field length for each datatype class */
    switch (dt->shared->type) {
        case H5T_INTEGER:
            ret_value += 4;
            break;

        case H5T_BITFIELD:
            ret_value += 4;
            break;

        case H5T_OPAQUE:
            ret_value += (HDstrlen(dt->shared->u.opaque.tag)+7) & (H5T_OPAQUE_TAG_MAX - 8);
            break;

        case H5T_FLOAT:
            ret_value += 12;
            break;

        case H5T_COMPOUND:
            for (i=0; i<dt->shared->u.compnd.nmembs; i++) {
                ret_value += ((HDstrlen(dt->shared->u.compnd.memb[i].name) + 8) / 8) * 8;
                ret_value += 4 +    /*member offset*/
                     1 +    /*dimensionality*/
                     3 +    /*reserved*/
                     4 +    /*permutation*/
                     4 +    /*reserved*/
                     16;    /*dimensions*/
                ret_value += H5O_dtype_size(f, dt->shared->u.compnd.memb[i].type);
            }
            break;

        case H5T_ENUM:
            ret_value += H5O_dtype_size(f, dt->shared->parent);
            for (i=0; i<dt->shared->u.enumer.nmembs; i++)
                ret_value += ((HDstrlen(dt->shared->u.enumer.name[i])+8)/8)*8;
            ret_value += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_VLEN:
            ret_value += H5O_dtype_size(f, dt->shared->parent);
            break;

        case H5T_TIME:
            ret_value += 2;
            break;

        case H5T_ARRAY:
            ret_value += 4; /* ndims & reserved bytes*/
            ret_value += 4*dt->shared->u.array.ndims; /* dimensions */
            ret_value += 4*dt->shared->u.array.ndims; /* dimension permutations */
            ret_value += H5O_dtype_size(f, dt->shared->parent);
            break;

        default:
            /*no properties */
            break;
    }

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_dtype_reset
 *
 * Purpose:  Frees resources within a message, but doesn't free
 *    the message itself.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_reset(void *_mesg)
{
    H5T_t       *dt = (H5T_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_dtype_reset);

    if (dt)
        H5T_free(dt);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_dtype_free
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
H5O_dtype_free (void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_dtype_free);

    assert (mesg);

    H5FL_FREE(H5T_shared_t, ((H5T_t *) mesg)->shared);
    H5FL_FREE(H5T_t,mesg);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_dtype_get_share
 *
 * Purpose:  Returns information about where the shared message is located
 *    by filling in the SH shared message struct.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Monday, June  1, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_get_share(H5F_t UNUSED *f, const void *_mesg,
        H5O_shared_t *sh/*out*/)
{
    const H5T_t  *dt = (const H5T_t *)_mesg;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dtype_get_share);

    assert (dt);
    assert (sh);

    if (H5F_addr_defined (dt->ent.header)) {
        /* If the address is defined, this had better be a named datatype */
  HDassert (H5T_STATE_NAMED==dt->shared->state || H5T_STATE_OPEN==dt->shared->state);

  sh->in_gh = FALSE;
  sh->u.ent = dt->ent;
    } else
  HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL, "datatype is not sharable");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_dtype_set_share
 *
 * Purpose:  Copies sharing information from SH into the message.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Thursday, June  4, 1998
 *
 * Modifications:
 *
 *      Pedro Vicente, <pvn@ncsa.uiuc.edu> 22 Aug 2002
 *      Added `id to name' support.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_set_share (H5F_t UNUSED *f, void *_mesg/*in,out*/,
         const H5O_shared_t *sh)
{
    H5T_t  *dt = (H5T_t *)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_dtype_set_share);

    assert (dt);
    assert (sh);
    assert (!sh->in_gh);

    /* NULL copy here, names not appropriate */
    H5G_ent_copy(&(dt->ent),&(sh->u.ent),H5G_COPY_NULL);

    /* Note that the datatype is a named datatype */
    dt->shared->state = H5T_STATE_NAMED;

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_debug
 PURPOSE
    Prints debugging information for a message
 USAGE
    void *H5O_dtype_debug(f, mesg, stream, indent, fwidth)
  H5F_t *f;    IN: pointer to the HDF5 file struct
  const void *mesg;  IN: Pointer to the source simple datatype
            struct
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
H5O_dtype_debug(H5F_t *f, hid_t dxpl_id, const void *mesg, FILE *stream,
    int indent, int fwidth)
{
    const H5T_t    *dt = (const H5T_t*)mesg;
    const char    *s;
    char    buf[256];
    unsigned    i;
    size_t    k;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_dtype_debug);

    /* check args */
    assert(f);
    assert(dt);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    switch (dt->shared->type) {
        case H5T_INTEGER:
            s = "integer";
            break;
        case H5T_FLOAT:
            s = "floating-point";
            break;
        case H5T_TIME:
            s = "date and time";
            break;
        case H5T_STRING:
            s = "text string";
            break;
        case H5T_BITFIELD:
            s = "bit field";
            break;
        case H5T_OPAQUE:
            s = "opaque";
            break;
        case H5T_COMPOUND:
            s = "compound";
            break;
        case H5T_REFERENCE:
            s = "reference";
            break;
        case H5T_ENUM:
            s = "enum";
            break;
        case H5T_ARRAY:
            s = "array";
            break;
        case H5T_VLEN:
            s = "vlen";
            break;
        default:
            sprintf(buf, "H5T_CLASS_%d", (int) (dt->shared->type));
            s = buf;
            break;
    }
    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
      "Type class:",
      s);

    fprintf(stream, "%*s%-*s %lu byte%s\n", indent, "", fwidth,
      "Size:",
      (unsigned long)(dt->shared->size), 1==dt->shared->size?"":"s");

    if (H5T_COMPOUND == dt->shared->type) {
  fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
    "Number of members:",
    dt->shared->u.compnd.nmembs);
  for (i=0; i<dt->shared->u.compnd.nmembs; i++) {
      sprintf(buf, "Member %d:", i);
      fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        buf,
        dt->shared->u.compnd.memb[i].name);
      fprintf(stream, "%*s%-*s %lu\n", indent+3, "", MAX(0, fwidth-3),
        "Byte offset:",
        (unsigned long) (dt->shared->u.compnd.memb[i].offset));
      H5O_dtype_debug(f, dxpl_id, dt->shared->u.compnd.memb[i].type, stream,
          indent+3, MAX(0, fwidth - 3));
  }
    } else if (H5T_ENUM==dt->shared->type) {
  fprintf(stream, "%*s%s\n", indent, "", "Base type:");
  H5O_dtype_debug(f, dxpl_id, dt->shared->parent, stream, indent+3, MAX(0, fwidth-3));
  fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
    "Number of members:",
    dt->shared->u.enumer.nmembs);
  for (i=0; i<dt->shared->u.enumer.nmembs; i++) {
      sprintf(buf, "Member %d:", i);
      fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        buf,
        dt->shared->u.enumer.name[i]);
      fprintf(stream, "%*s%-*s 0x", indent, "", fwidth,
        "Raw bytes of value:");
      for (k=0; k<dt->shared->parent->shared->size; k++) {
    fprintf(stream, "%02x",
      dt->shared->u.enumer.value[i*dt->shared->parent->shared->size+k]);
      }
      fprintf(stream, "\n");
  }

    } else if (H5T_OPAQUE==dt->shared->type) {
  fprintf(stream, "%*s%-*s \"%s\"\n", indent, "", fwidth,
    "Tag:", dt->shared->u.opaque.tag);
    } else if (H5T_REFERENCE==dt->shared->type) {
  fprintf(stream, "%*s%-*s\n", indent, "", fwidth,
    "Fix dumping reference types!");
    } else if (H5T_VLEN==dt->shared->type) {
        switch (dt->shared->u.vlen.type) {
            case H5T_VLEN_SEQUENCE:
                s = "sequence";
                break;
            case H5T_VLEN_STRING:
                s = "string";
                break;
            default:
                sprintf(buf, "H5T_VLEN_%d", dt->shared->u.vlen.type);
                s = buf;
                break;
        }
        fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                "Vlen type:", s);

        switch (dt->shared->u.vlen.loc) {
            case H5T_VLEN_MEMORY:
                s = "memory";
                break;
            case H5T_VLEN_DISK:
                s = "disk";
                break;
            default:
                sprintf(buf, "H5T_VLEN_%d", dt->shared->u.vlen.loc);
                s = buf;
                break;
        }
        fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                "Location:", s);
    } else if (H5T_ARRAY==dt->shared->type) {
  fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
    "Rank:",
    dt->shared->u.array.ndims);
    fprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Size:");
  for (i=0; i<(unsigned)dt->shared->u.array.ndims; i++) {
        fprintf (stream, "%s%u", i?", ":"", (unsigned)dt->shared->u.array.dim[i]);
    }
    fprintf (stream, "}\n");
    fprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Permutation:");
  for (i=0; i<(unsigned)dt->shared->u.array.ndims; i++) {
        fprintf (stream, "%s%d", i?", ":"", dt->shared->u.array.perm[i]);
    }
    fprintf (stream, "}\n");
  fprintf(stream, "%*s%s\n", indent, "", "Base type:");
  H5O_dtype_debug(f, dxpl_id, dt->shared->parent, stream, indent+3, MAX(0, fwidth-3));
    } else {
  switch (dt->shared->u.atomic.order) {
            case H5T_ORDER_LE:
                s = "little endian";
                break;
            case H5T_ORDER_BE:
                s = "big endian";
                break;
            case H5T_ORDER_VAX:
                s = "VAX";
                break;
            case H5T_ORDER_NONE:
                s = "none";
                break;
            default:
                sprintf(buf, "H5T_ORDER_%d", dt->shared->u.atomic.order);
                s = buf;
                break;
  }
  fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
    "Byte order:",
    s);

  fprintf(stream, "%*s%-*s %lu bit%s\n", indent, "", fwidth,
    "Precision:",
    (unsigned long)(dt->shared->u.atomic.prec),
    1==dt->shared->u.atomic.prec?"":"s");

  fprintf(stream, "%*s%-*s %lu bit%s\n", indent, "", fwidth,
    "Offset:",
    (unsigned long)(dt->shared->u.atomic.offset),
    1==dt->shared->u.atomic.offset?"":"s");

  switch (dt->shared->u.atomic.lsb_pad) {
            case H5T_PAD_ZERO:
                s = "zero";
                break;
            case H5T_PAD_ONE:
                s = "one";
                break;
            default:
                s = "pad?";
                break;
  }
  fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
    "Low pad type:", s);

  switch (dt->shared->u.atomic.msb_pad) {
            case H5T_PAD_ZERO:
                s = "zero";
                break;
            case H5T_PAD_ONE:
                s = "one";
                break;
            default:
                s = "pad?";
                break;
  }
  fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
    "High pad type:", s);

  if (H5T_FLOAT == dt->shared->type) {
      switch (dt->shared->u.atomic.u.f.pad) {
                case H5T_PAD_ZERO:
                    s = "zero";
                    break;
                case H5T_PAD_ONE:
                    s = "one";
                    break;
                default:
                    if (dt->shared->u.atomic.u.f.pad < 0) {
                        sprintf(buf, "H5T_PAD_%d", -(dt->shared->u.atomic.u.f.pad));
                    } else {
                        sprintf(buf, "bit-%d", dt->shared->u.atomic.u.f.pad);
                    }
                    s = buf;
                    break;
      }
      fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Internal pad type:", s);

      switch (dt->shared->u.atomic.u.f.norm) {
                case H5T_NORM_IMPLIED:
                    s = "implied";
                    break;
                case H5T_NORM_MSBSET:
                    s = "msb set";
                    break;
                case H5T_NORM_NONE:
                    s = "none";
                    break;
                default:
                    sprintf(buf, "H5T_NORM_%d", (int) (dt->shared->u.atomic.u.f.norm));
                    s = buf;
      }
      fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Normalization:", s);

      fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
        "Sign bit location:",
        (unsigned long) (dt->shared->u.atomic.u.f.sign));

      fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
        "Exponent location:",
        (unsigned long) (dt->shared->u.atomic.u.f.epos));

      fprintf(stream, "%*s%-*s 0x%08lx\n", indent, "", fwidth,
        "Exponent bias:",
        (unsigned long) (dt->shared->u.atomic.u.f.ebias));

      fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
        "Exponent size:",
        (unsigned long) (dt->shared->u.atomic.u.f.esize));

      fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
        "Mantissa location:",
        (unsigned long) (dt->shared->u.atomic.u.f.mpos));

      fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
        "Mantissa size:",
        (unsigned long) (dt->shared->u.atomic.u.f.msize));

  } else if (H5T_INTEGER == dt->shared->type) {
      switch (dt->shared->u.atomic.u.i.sign) {
                case H5T_SGN_NONE:
                    s = "none";
                    break;
                case H5T_SGN_2:
                    s = "2's comp";
                    break;
                default:
                    sprintf(buf, "H5T_SGN_%d", (int) (dt->shared->u.atomic.u.i.sign));
                    s = buf;
                    break;
      }
      fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Sign scheme:", s);

  }
    }

    FUNC_LEAVE_NOAPI(SUCCEED);
}
