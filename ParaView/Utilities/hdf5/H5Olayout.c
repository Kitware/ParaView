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

#define H5O_PACKAGE	/*suppress error about including H5Opkg	  */

#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"	/*Free Lists	  */
#include "H5MFprivate.h"	/* File space management		*/
#include "H5MMprivate.h"
#include "H5Opkg.h"             /* Object header functions                  */

/* PRIVATE PROTOTYPES */
static void *H5O_layout_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_layout_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_layout_copy(const void *_mesg, void *_dest);
static size_t H5O_layout_size(H5F_t *f, const void *_mesg);
static herr_t H5O_layout_reset (void *_mesg);
static herr_t H5O_layout_free (void *_mesg);
static herr_t H5O_layout_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg);
static herr_t H5O_layout_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE * stream,
			       int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_LAYOUT[1] = {{
    H5O_LAYOUT_ID,          	/*message id number             */
    "layout",               	/*message name for debugging    */
    sizeof(H5O_layout_t),   	/*native message size           */
    H5O_layout_decode,      	/*decode message                */
    H5O_layout_encode,      	/*encode message                */
    H5O_layout_copy,        	/*copy the native value         */
    H5O_layout_size,        	/*size of message on disk       */
    H5O_layout_reset,           /*reset method                  */
    H5O_layout_free,        	/*free the struct		*/
    H5O_layout_delete,	        /* file delete method		*/
    NULL,			/* link method			*/
    NULL,		    	/*get share method		*/
    NULL,			/*set share method		*/
    H5O_layout_debug,       	/*debug the message             */
}};

/* For forward and backward compatibility.  Version is 1 when space is 
 * allocated; 2 when space is delayed for allocation. */
#define H5O_LAYOUT_VERSION_1	1
#define H5O_LAYOUT_VERSION_2	2

/* Interface initialization */
#define PABLO_MASK      H5O_layout_mask
static int interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

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
 * 	Robb Matzke, 1998-07-20
 *	Rearranged the message to add a version number at the beginning.
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
    int                    version;
    unsigned               u;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_decode, NULL);

    /* check args */
    assert(f);
    assert(p);
    assert (!sh);

    /* decode */
    if (NULL==(mesg = H5FL_CALLOC(H5O_layout_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Version. 1 when space allocated; 2 when space allocation is delayed */
    version = *p++;
    if (version!=H5O_LAYOUT_VERSION_1 && version!=H5O_LAYOUT_VERSION_2)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for layout message");

    /* Dimensionality */
    mesg->ndims = *p++;
    if (mesg->ndims>H5O_LAYOUT_NDIMS)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is too large");

    /* Layout class */
    mesg->type = *p++;
    assert(H5D_CONTIGUOUS == mesg->type || H5D_CHUNKED == mesg->type || H5D_COMPACT == mesg->type);

    /* Reserved bytes */
    p += 5;

    /* Address */
    if(mesg->type!=H5D_COMPACT)
        H5F_addr_decode(f, &p, &(mesg->addr));

    /* Read the size */
    for (u = 0; u < mesg->ndims; u++)
        UINT32DECODE(p, mesg->dim[u]);

    if(mesg->type == H5D_COMPACT) {
        UINT32DECODE(p, mesg->size);
        if(mesg->size > 0) {
            if(NULL==(mesg->buf=H5MM_malloc(mesg->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value");
            HDmemcpy(mesg->buf, p, mesg->size);
            p += mesg->size;
        }
    }

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
 * 	Robb Matzke, 1998-07-20
 *	Rearranged the message to add a version number at the beginning.
 *
 *	Raymond Lu, 2002-2-26
 *	Added version number 2 case depends on if space has been allocated
 *	at the moment when layout header message is updated.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned               u;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_encode, FAIL);

    /* check args */
    assert(f);
    assert(mesg);
    assert(mesg->ndims > 0 && mesg->ndims <= H5O_LAYOUT_NDIMS);
    assert(p);

    /* Version: 1 when space allocated; 2 when space allocation is delayed */
    if(mesg->type==H5D_CONTIGUOUS) {
    	if(mesg->addr==HADDR_UNDEF)
            *p++ = H5O_LAYOUT_VERSION_2;
    	else 
	    *p++ = H5O_LAYOUT_VERSION_1;
    } else if(mesg->type==H5D_COMPACT) {
        *p++ = H5O_LAYOUT_VERSION_2;
    } else 
	*p++ = H5O_LAYOUT_VERSION_1;

    /* number of dimensions */
    *p++ = mesg->ndims;

    /* layout class */
    *p++ = mesg->type;

    /* reserved bytes should be zero */
    for (u=0; u<5; u++)
        *p++ = 0;

    /* data or B-tree address */
    if(mesg->type!=H5D_COMPACT)
        H5F_addr_encode(f, &p, mesg->addr);

    /* dimension size */
    for (u = 0; u < mesg->ndims; u++)
        UINT32ENCODE(p, mesg->dim[u]);

    if(mesg->type==H5D_COMPACT) {
        UINT32ENCODE(p, mesg->size);
        if(mesg->size>0 && mesg->buf) {
            HDmemcpy(p, mesg->buf, mesg->size);
            p += mesg->size;
        }
    }
    
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
H5O_layout_copy(const void *_mesg, void *_dest)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    H5O_layout_t           *dest = (H5O_layout_t *) _dest;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_copy, NULL);

    /* check args */
    assert(mesg);
    if (!dest && NULL==(dest=H5FL_MALLOC(H5O_layout_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    
    /* copy */
    *dest = *mesg;

    /* Deep copy the buffer for compact datasets also */
    if(mesg->type==H5D_COMPACT) {
        /* Allocate memory for the raw data */
        if (NULL==(dest->buf=H5MM_malloc(dest->size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate memory for compact dataset");

        /* Copy over the raw data */
        HDmemcpy(dest->buf,mesg->buf,dest->size);
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
H5O_layout_meta_size(H5F_t *f, const void *_mesg)
{
    const H5O_layout_t      *mesg = (const H5O_layout_t *) _mesg;
    size_t                  ret_value;
     
    FUNC_ENTER_NOAPI(H5O_layout_meta_size, 0);
                                
    /* check args */
    assert(f);
    assert(mesg);
    assert(mesg->ndims > 0 && mesg->ndims <= H5O_LAYOUT_NDIMS);
                     
    ret_value = 1 +                     /* Version number                       */
                1 +                     /* layout class type                    */
                1 +                     /* dimensionality                       */
                5 +                     /* reserved bytes                       */
                mesg->ndims * 4;        /* size of each dimension               */

    if(mesg->type==H5D_COMPACT)
        ret_value += 4;        /* size field for compact dataset       */
    else
        ret_value += H5F_SIZEOF_ADDR(f); /* file address of data or B-tree for chunked dataset */ 

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
H5O_layout_size(H5F_t *f, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    size_t                  ret_value;

    FUNC_ENTER_NOAPI(H5O_layout_size, 0);

    /* check args */
    assert(f);
    assert(mesg);
    assert(mesg->ndims > 0 && mesg->ndims <= H5O_LAYOUT_NDIMS);

    ret_value = H5O_layout_meta_size(f, mesg);
    if(mesg->type==H5D_COMPACT)
        ret_value += mesg->size;/* data for compact dataset             */
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5O_layout_reset
 *
 * Purpose:	Frees resources within a data type message, but doesn't free
 *		the message itself.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
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
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_reset, FAIL);

    if(mesg) {
        /* Free the compact storage buffer */
        if(mesg->type==H5D_COMPACT)
            mesg->buf=H5MM_xfree(mesg->buf);

        /* Reset the message */
        mesg->type=H5D_CONTIGUOUS;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5O_layout_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
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
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_free, FAIL);

    assert (mesg);

    /* Free the compact storage buffer */
    if(mesg->type==H5D_COMPACT)
        mesg->buf=H5MM_xfree(mesg->buf);

    H5FL_FREE(H5O_layout_t,mesg);

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
H5O_layout_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_delete, FAIL);

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
            if (H5F_contig_delete(f, dxpl_id, mesg)<0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data");
            break;

        case H5D_CHUNKED:       /* Chunked blocks on disk */
            /* Free the file space for the raw data */
            if (H5F_istore_delete(f, dxpl_id, mesg)<0)
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
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5O_layout_debug, FAIL);

    /* check args */
    assert(f);
    assert(mesg);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      H5D_CHUNKED == mesg->type ? "B-tree address:" : "Data address:",
	      mesg->addr);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
	      "Number of dimensions:",
	      (unsigned long) (mesg->ndims));

    /* Size */
    HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Size:");
    for (u = 0; u < mesg->ndims; u++) {
        HDfprintf(stream, "%s%lu", u ? ", " : "",
		  (unsigned long) (mesg->dim[u]));
    }
    HDfprintf(stream, "}\n");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}
