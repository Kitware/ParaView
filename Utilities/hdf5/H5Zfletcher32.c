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

/*
 * Programmer:  Raymond Lu<slu@ncsa.uiuc.edu>
 *              Jan 3, 2003
 */

#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"         /* File access                          */
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Zpkg.h"		/* Data filters				*/

#ifdef H5_HAVE_FILTER_FLETCHER32

/* Interface initialization */
#define PABLO_MASK	H5Z_fletcher32_mask
#define INTERFACE_INIT	NULL
static int interface_initialize_g = 0;

/* Local function prototypes */
static size_t H5Z_filter_fletcher32 (unsigned flags, size_t cd_nelmts,
    const unsigned cd_values[], size_t nbytes, size_t *buf_size, void **buf);

/* This message derives from H5Z */
const H5Z_class_t H5Z_FLETCHER32[1] = {{
    H5Z_FILTER_FLETCHER32,	/* Filter id number		*/
    "fletcher32",		/* Filter name for debugging	*/
    NULL,                       /* The "can apply" callback     */
    NULL,                       /* The "set local" callback     */
    H5Z_filter_fletcher32,	/* The actual filter function	*/
}};

#define FLETCHER_LEN       4


/*-------------------------------------------------------------------------
 * Function:	H5Z_filter_fletcher32_compute
 *
 * Purpose:	Implement an Fletcher32 Checksum using 1's complement.
 *
 * Return:	Success: Fletcher32 value	
 *
 *		Failure: Can't fail
 *
 * Programmer:	Raymond Lu
 *              Jan 3, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static uint32_t
H5Z_filter_fletcher32_compute(void *_src, size_t len)
{
#if H5_SIZEOF_UINT16_T==2
    uint16_t *src=(uint16_t *)_src;
#else /* H5_SIZEOF_UINT16_T */
    /*To handle unusual platforms like Cray*/
    unsigned char *src=(unsigned char *)_src;
    unsigned short tmp_src;
#endif /* H5_SIZEOF_UINT16_T */
    size_t count = len;         /* Number of bytes left to checksum */
    uint32_t s1 = 0, s2 = 0;    /* Temporary partial checksums */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5Z_filter_fletcher32_compute);

    /* Compute checksum */
    while(count > 1) {
#if H5_SIZEOF_UINT16_T==2
        /*For normal platforms*/
        s1 += *src++;
#else /* H5_SIZEOF_UINT16_T */
        /*To handle unusual platforms like Cray*/
        tmp_src = (((unsigned short)src[0])<<8) | ((unsigned short)src[1]);
        src +=2;
        s1 += tmp_src;
#endif /* H5_SIZEOF_UINT16_T */
        if(s1 & 0xFFFF0000) { /*Wrap around carry if occurred*/
            s1 &= 0xFFFF;
            s1++;
        }
        s2 += s1;
        if(s2 & 0xFFFF0000) { /*Wrap around carry if occurred*/
            s2 &= 0xFFFF;
            s2++;
        }
        count -= 2;
    }

    if(count==1) {
        s1 += *(unsigned char*)src;
        if(s1 & 0xFFFF0000) { /*Wrap around carry if occurred*/
            s1 &= 0xFFFF;
            s1++;
        }
        s2 += s1;
        if(s2 & 0xFFFF0000) { /*Wrap around carry if occurred*/
            s2 &= 0xFFFF;
            s2++;
        }
    }

    FUNC_LEAVE_NOAPI((s2 << 16) + s1);
}


/*-------------------------------------------------------------------------
 * Function:	H5Z_filter_fletcher32
 *
 * Purpose:	Implement an I/O filter of Fletcher32 Checksum
 *
 * Return:	Success: Size of buffer filtered
 *		Failure: 0	
 *
 * Programmer:	Raymond Lu
 *              Jan 3, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5Z_filter_fletcher32 (unsigned flags, size_t UNUSED cd_nelmts, const unsigned UNUSED cd_values[], 
                     size_t nbytes, size_t *buf_size, void **buf)
{
    void    *outbuf = NULL;     /* Pointer to new buffer */
    unsigned char *src = (unsigned char*)(*buf);
    uint32_t fletcher;          /* Checksum value */
    size_t   ret_value;         /* Return value */
    
    FUNC_ENTER_NOAPI(H5Z_filter_fletcher32, 0);

    assert(sizeof(uint32_t)>=4);
   
    if (flags & H5Z_FLAG_REVERSE) { /* Read */
        /* Do checksum if it's enabled for read; otherwise skip it
         * to save performance. */
        if (!(flags & H5Z_FLAG_SKIP_EDC)) {
            unsigned char *tmp_src;             /* Pointer to checksum in buffer */
            size_t  src_nbytes = nbytes;        /* Original number of bytes */
            uint32_t stored_fletcher;           /* Stored checksum value */

            /* Get the stored checksum */
            src_nbytes -= FLETCHER_LEN;
            tmp_src=src+src_nbytes;
            UINT32DECODE(tmp_src, stored_fletcher);

            /* Compute checksum (can't fail) */
            fletcher = H5Z_filter_fletcher32_compute((unsigned short*)src,src_nbytes);

            /* Verify computed checksum matches stored checksum */
            if(stored_fletcher != fletcher)
	        HGOTO_ERROR(H5E_STORAGE, H5E_READERROR, 0, "data error detected by Fletcher32 checksum");
        }
        
        /* Set return values */
        /* (Re-use the input buffer, just note that the size is smaller by the size of the checksum) */
        ret_value = nbytes-FLETCHER_LEN;
    } else { /* Write */
        unsigned char *dst;     /* Temporary pointer to destination buffer */

        /* Compute checksum (can't fail) */
        fletcher = H5Z_filter_fletcher32_compute((unsigned short*)src,nbytes);
        
	if (NULL==(dst=outbuf=H5MM_malloc(nbytes+FLETCHER_LEN)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "unable to allocate Fletcher32 checksum destination buffer");
        
        /* Copy raw data */
        HDmemcpy((void*)dst, (void*)(*buf), nbytes);

        /* Append checksum to raw data for storage */
        dst += nbytes;
        UINT32ENCODE(dst, fletcher);

        /* Free input buffer */
 	H5MM_xfree(*buf);

        /* Set return values */
        *buf_size = nbytes + FLETCHER_LEN;
	*buf = outbuf;
	outbuf = NULL;
	ret_value = *buf_size;           
    }

done:
    if(outbuf)
        H5MM_xfree(outbuf);
    FUNC_LEAVE_NOAPI(ret_value);
}
#endif /* H5_HAVE_FILTER_FLETCHER32 */

