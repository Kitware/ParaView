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

#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"         /* File access                          */
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Ppublic.h"		/* Property lists			*/
#include "H5Tpublic.h"		/* Datatype functions			*/
#include "H5Zpkg.h"		/* Data filters				*/

#ifdef H5_HAVE_FILTER_SZIP

#ifdef H5_HAVE_SZLIB_H
#   include "szlib.h"
#endif

/* Interface initialization */
#define PABLO_MASK	H5Z_szip_mask
#define INTERFACE_INIT	NULL
static int interface_initialize_g = 0;

/* Local function prototypes */
static herr_t H5Z_can_apply_szip(hid_t dcpl_id, hid_t type_id, hid_t space_id);
static herr_t H5Z_set_local_szip(hid_t dcpl_id, hid_t type_id, hid_t space_id);
static size_t H5Z_filter_szip (unsigned flags, size_t cd_nelmts,
    const unsigned cd_values[], size_t nbytes, size_t *buf_size, void **buf);

/* This message derives from H5Z */
const H5Z_class_t H5Z_SZIP[1] = {{
    H5Z_FILTER_SZIP,		/* Filter id number		*/
    "szip",			/* Filter name for debugging	*/
    H5Z_can_apply_szip,		/* The "can apply" callback     */
    H5Z_set_local_szip,         /* The "set local" callback     */
    H5Z_filter_szip,		/* The actual filter function	*/
}};

/* Local macros */
#define H5Z_SZIP_USER_NPARMS    2       /* Number of parameters that users can set */
#define H5Z_SZIP_TOTAL_NPARMS   4       /* Total number of parameters for filter */
#define H5Z_SZIP_PARM_MASK      0       /* "User" parameter for option mask */
#define H5Z_SZIP_PARM_PPB       1       /* "User" parameter for pixels-per-block */
#define H5Z_SZIP_PARM_BPP       2       /* "Local" parameter for bits-per-pixel */
#define H5Z_SZIP_PARM_PPS       3       /* "Local" parameter for pixels-per-scanline */


/*-------------------------------------------------------------------------
 * Function:	H5Z_can_apply_szip
 *
 * Purpose:	Check the parameters for szip compression for validity and
 *              whether they fit a particular dataset.
 *
 * Note:        This function currently range-checks for datatypes with
 *              8-bit boundaries (8, 16, 24, etc.).  It appears that the szip
 *              library can actually handle 1-24, 32 & 64 bit samples.  If
 *              this becomes important, we should make the checks below more
 *              sophisticated and have them check for n-bit datatypes of the
 *              correct size, etc. - QAK
 *
 * Return:	Success: Non-negative
 *		Failure: Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, April  7, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5Z_can_apply_szip(hid_t dcpl_id, hid_t type_id, hid_t space_id)
{
    unsigned flags;         /* Filter flags */
    size_t cd_nelmts=H5Z_SZIP_USER_NPARMS;     /* Number of filter parameters */
    unsigned cd_values[H5Z_SZIP_TOTAL_NPARMS];  /* Filter parameters */
    hsize_t dims[H5O_LAYOUT_NDIMS];     /* Dataspace (i.e. chunk) dimensions */
    int ndims;                          /* Number of (chunk) dimensions */
    int dtype_size;                     /* Datatype's size (in bits) */
    H5T_order_t dtype_order;            /* Datatype's endianness order */
    hsize_t scanline;                   /* Size of dataspace's fastest changing dimension */
    herr_t ret_value=TRUE;              /* Return value */

    FUNC_ENTER_NOAPI(H5Z_can_apply_szip, FAIL);

    /* Get the filter's current parameters */
    if(H5Pget_filter_by_id(dcpl_id,H5Z_FILTER_SZIP,&flags,&cd_nelmts, cd_values,0,NULL)<0)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTGET, FAIL, "can't get szip parameters");

    /* Get datatype's size, for checking the "bits-per-pixel" */
    if((dtype_size=(sizeof(unsigned char)*H5Tget_size(type_id)))==0)
	HGOTO_ERROR(H5E_PLINE, H5E_BADTYPE, FAIL, "bad datatype size");

    /* Range check datatype's size */
    if(dtype_size>32 && dtype_size!=64)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FALSE, "invalid datatype size");

    /* Get datatype's endianness order */
    if((dtype_order=H5Tget_order(type_id))==H5T_ORDER_ERROR)
	HGOTO_ERROR(H5E_PLINE, H5E_BADTYPE, FAIL, "can't retrieve datatype endianness order");

    /* Range check datatype's endianness order */
    /* (Note: this may not handle non-atomic datatypes well) */
    if(dtype_order != H5T_ORDER_LE && dtype_order != H5T_ORDER_BE)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FALSE, "invalid datatype endianness order");

    /* Get dimensions for dataspace */
    if ((ndims=H5Sget_simple_extent_dims(space_id, dims, NULL))<0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTGET, FAIL, "unable to get dataspace dimensions");

    /* Get "local" parameter for this dataset's "pixels-per-scanline" */
    /* (Use the chunk's fastest changing dimension size) */
    scanline=dims[ndims-1];

    /* Range check the scanline's size */
    if(scanline > SZ_MAX_PIXELS_PER_SCANLINE)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FALSE, "invalid scanline size");

    /* Range check the pixels per block against the 'scanline' size */
    if(scanline<cd_values[H5Z_SZIP_PARM_PPB])
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FALSE, "pixels per block greater than scanline");

    /* Range check the scanline's number of blocks */
    if((scanline/cd_values[H5Z_SZIP_PARM_PPB]) > SZ_MAX_BLOCKS_PER_SCANLINE)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FALSE, "invalid number of blocks per scanline");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5Z_can_apply_szip() */


/*-------------------------------------------------------------------------
 * Function:	H5Z_set_local_szip
 *
 * Purpose:	Set the "local" dataset parameters for szip compression.
 *
 * Return:	Success: Non-negative
 *		Failure: Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, April  7, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5Z_set_local_szip(hid_t dcpl_id, hid_t type_id, hid_t space_id)
{
    unsigned flags;         /* Filter flags */
    size_t cd_nelmts=H5Z_SZIP_USER_NPARMS;     /* Number of filter parameters */
    unsigned cd_values[H5Z_SZIP_TOTAL_NPARMS];  /* Filter parameters */
    hsize_t dims[H5O_LAYOUT_NDIMS];             /* Dataspace (i.e. chunk) dimensions */
    int ndims;                  /* Number of (chunk) dimensions */
    H5T_order_t dtype_order;    /* Datatype's endianness order */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5Z_set_local_szip, FAIL);

    /* Get the filter's current parameters */
    if(H5Pget_filter_by_id(dcpl_id,H5Z_FILTER_SZIP,&flags,&cd_nelmts, cd_values,0,NULL)<0)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTGET, FAIL, "can't get szip parameters");

    /* Get dimensions for dataspace */
    if ((ndims=H5Sget_simple_extent_dims(space_id, dims, NULL))<0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTGET, FAIL, "unable to get dataspace dimensions");

    /* Set "local" parameter for this dataset's "bits-per-pixel" */
    if((cd_values[H5Z_SZIP_PARM_BPP]=(8*sizeof(unsigned char)*H5Tget_size(type_id)))==0)
	HGOTO_ERROR(H5E_PLINE, H5E_BADTYPE, FAIL, "bad datatype size");

    /* Set "local" parameter for this dataset's "pixels-per-scanline" */
    /* (Use the chunk's fastest changing dimension size) */
    cd_values[H5Z_SZIP_PARM_PPS]=dims[ndims-1];

    /* Get datatype's endianness order */
    if((dtype_order=H5Tget_order(type_id))==H5T_ORDER_ERROR)
	HGOTO_ERROR(H5E_PLINE, H5E_BADTYPE, FAIL, "bad datatype endianness order");

    /* Set the correct endianness flag for szip */
    /* (Note: this may not handle non-atomic datatypes well) */
    cd_values[H5Z_SZIP_PARM_MASK] &= ~(SZ_LSB_OPTION_MASK|SZ_MSB_OPTION_MASK);
    switch(dtype_order) {
        case H5T_ORDER_LE:      /* Little-endian byte order */
            cd_values[H5Z_SZIP_PARM_MASK] |= SZ_LSB_OPTION_MASK;
            break;

        case H5T_ORDER_BE:      /* Big-endian byte order */
            cd_values[H5Z_SZIP_PARM_MASK] |= SZ_MSB_OPTION_MASK;
            break;

        default:
            HGOTO_ERROR(H5E_PLINE, H5E_BADTYPE, FAIL, "bad datatype endianness order");
    } /* end switch */

    /* Modify the filter's parameters for this dataset */
    if(H5Pmodify_filter(dcpl_id, H5Z_FILTER_SZIP, flags, H5Z_SZIP_TOTAL_NPARMS, cd_values)<0)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTSET, FAIL, "can't set local szip parameters");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5Z_set_local_szip() */


/*-------------------------------------------------------------------------
 * Function:	H5Z_filter_szip
 *
 * Purpose:	Implement an I/O filter around the 'rice' algorithm in
 *              libsz
 *
 * Return:	Success: Size of buffer filtered
 *		Failure: 0	
 *
 * Programmer:	Kent Yang
 *              Tuesday, April 1, 2003
 *
 * Modifications:
 *              Quincey Koziol, April 2, 2003
 *              Cleaned up code.
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5Z_filter_szip (unsigned flags, size_t cd_nelmts, const unsigned cd_values[], 
    size_t nbytes, size_t *buf_size, void **buf)
{
    size_t ret_value = 0;       /* Return value */
    size_t size_out  = 0;       /* Size of output buffer */
    unsigned char *outbuf = NULL;    /* Pointer to new output buffer */
    unsigned char *newbuf = NULL;    /* Pointer to input buffer */
    SZ_com_t sz_param;          /* szip parameter block */

    FUNC_ENTER_NOAPI(H5Z_filter_szip, 0);

    /* Sanity check to make certain that we haven't drifted out of date with
     * the mask options from the szlib.h header */
    assert(H5_SZIP_ALLOW_K13_OPTION_MASK==SZ_ALLOW_K13_OPTION_MASK);
    assert(H5_SZIP_CHIP_OPTION_MASK==SZ_CHIP_OPTION_MASK);
    assert(H5_SZIP_EC_OPTION_MASK==SZ_EC_OPTION_MASK);
    assert(H5_SZIP_LSB_OPTION_MASK==SZ_LSB_OPTION_MASK);
    assert(H5_SZIP_MSB_OPTION_MASK==SZ_MSB_OPTION_MASK);
    assert(H5_SZIP_NN_OPTION_MASK==SZ_NN_OPTION_MASK);
    assert(H5_SZIP_RAW_OPTION_MASK==SZ_RAW_OPTION_MASK);

    /* Check arguments */
    if (cd_nelmts!=4)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, 0, "invalid deflate aggression level");

    /* Copy the filter parameters into the szip parameter block */
    sz_param.options_mask        = cd_values[H5Z_SZIP_PARM_MASK];
    sz_param.bits_per_pixel      = cd_values[H5Z_SZIP_PARM_BPP];
    sz_param.pixels_per_block    = cd_values[H5Z_SZIP_PARM_PPB];
    sz_param.pixels_per_scanline = cd_values[H5Z_SZIP_PARM_PPS];

    /* Input; uncompress */
    if (flags & H5Z_FLAG_REVERSE) {
        uint32_t stored_nalloc;  /* Number of bytes the compressed block will expand into */
        size_t nalloc;  /* Number of bytes the compressed block will expand into */

        /* Get the size of the uncompressed buffer */
        newbuf = *buf;
        UINT32DECODE(newbuf,stored_nalloc);
        H5_ASSIGN_OVERFLOW(nalloc,stored_nalloc,uint32_t,size_t);
        
        /* Allocate space for the uncompressed buffer */
        if(NULL==(outbuf = H5MM_malloc(nalloc)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed for szip decompression");

        /* Decompress the buffer */
        size_out=nalloc;
        if(SZ_BufftoBuffDecompress(outbuf, &size_out, newbuf, nbytes-4, &sz_param) != SZ_OK)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "szip_filter: decompression failed");
        assert(size_out==nalloc);

        /* Free the input buffer */
        H5MM_xfree(*buf);

        /* Set return values */
        *buf = outbuf;
        outbuf = NULL;
        *buf_size = nalloc;
        ret_value = nalloc;
    }
    /* Output; compress */
    else {
        unsigned char *dst = NULL;    /* Temporary pointer to new output buffer */

        /* Allocate space for the compressed buffer & header (assume data won't get bigger) */
        if(NULL==(dst=outbuf = H5MM_malloc(nbytes+4)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "unable to allocate szip destination buffer");

        /* Encode the uncompressed length */
        H5_CHECK_OVERFLOW(nbytes,size_t,uint32_t);
        UINT32ENCODE(dst,nbytes);

        /* Compress the buffer */
        size_out = nbytes;
        if(SZ_OK!= SZ_BufftoBuffCompress(dst, &size_out, *buf, nbytes, &sz_param))
	    HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "overflow");
        assert(size_out<=nbytes);

        /* Free the input buffer */
        H5MM_xfree(*buf);

        /* Set return values */
        *buf = outbuf;
        outbuf = NULL;
        *buf_size = size_out+4;
        ret_value = size_out+4;
    }

done: 
    if(outbuf)
        H5MM_xfree(outbuf);
    FUNC_LEAVE_NOAPI(ret_value);
}
#endif /* H5_HAVE_FILTER_SZIP */

