/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Friday, August 27, 1999
 */
#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"
#include "H5Zprivate.h"

#ifdef H5_HAVE_ZLIB_H
#   include <zlib.h>
#else
/* Make sure compression is disabled too. */
#undef H5_HAVE_COMPRESS2
#endif

/* Interface initialization */
#define PABLO_MASK      H5Z_deflate_mask
#define INTERFACE_INIT  NULL
static int interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:    H5Z_filter_deflate
 *
 * Purpose:     
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
#if defined(H5_HAVE_COMPRESS2)
H5Z_filter_deflate (unsigned flags, size_t cd_nelmts,
                    const unsigned cd_values[], size_t nbytes,
                    size_t *buf_size, void **buf)
#else
H5Z_filter_deflate (unsigned UNUSED flags, size_t cd_nelmts,
                    const unsigned cd_values[], size_t UNUSED nbytes,
                    size_t * UNUSED buf_size, void ** UNUSED buf)
#endif
{
    size_t      ret_value = 0;
    void        *outbuf = NULL;
#if defined(H5_HAVE_COMPRESS2)
    int         aggression = 6;
    int         status;
#endif
    
    FUNC_ENTER (H5Z_filter_deflate, 0);

    /* Check arguments */
    if (cd_nelmts!=1 || cd_values[0]>9) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, 0,
                    "invalid deflate aggression level");
    }

#if defined(H5_HAVE_COMPRESS2)
    aggression = cd_values[0];
    if (flags & H5Z_FLAG_REVERSE) {
        /* Input; uncompress */
        z_stream        z_strm;
        size_t          nalloc = *buf_size;

        if (NULL==(outbuf = H5F_istore_chunk_alloc(nalloc))) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0,
                        "memory allocation failed for deflate uncompression");
        }
        HDmemset(&z_strm, 0, sizeof(z_strm));
        z_strm.next_in = *buf;
        z_strm.avail_in = nbytes;
        z_strm.next_out = outbuf;
        z_strm.avail_out = nalloc;
        if (Z_OK!=inflateInit(&z_strm)) {
            HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "inflateInit() failed");
        }
        while (1) {
            status = inflate(&z_strm, Z_SYNC_FLUSH);
            if (Z_STREAM_END==status) break;    /*done*/
            if (Z_OK!=status) {
                inflateEnd(&z_strm);
                HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "inflate() failed");
            }
            if (Z_OK==status && 0==z_strm.avail_out) {
                nalloc *= 2;
                if (NULL==(outbuf = H5F_istore_chunk_realloc(outbuf, nalloc))) {
                    inflateEnd(&z_strm);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0,
                                "memory allocation failed for deflate "
                                "uncompression");
                }
                z_strm.next_out = (unsigned char*)outbuf + z_strm.total_out;
                z_strm.avail_out = nalloc - z_strm.total_out;
            }
        }
        
        H5F_istore_chunk_free(*buf);
        *buf = outbuf;
        outbuf = NULL;
        *buf_size = nalloc;
        ret_value = z_strm.total_out;
        inflateEnd(&z_strm);
        
    } else {
        /*
         * Output; compress but fail if the result would be larger than the
         * input.  The library doesn't provide in-place compression, so we
         * must allocate a separate buffer for the result.
         */
        const Bytef     *z_src = (const Bytef*)(*buf);
        Bytef           *z_dst;         /*destination buffer            */
        uLongf          z_dst_nbytes = (uLongf)nbytes;
        uLong           z_src_nbytes = (uLong)nbytes;

        if (NULL==(z_dst=outbuf=H5F_istore_chunk_alloc(nbytes))) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0,
                        "unable to allocate deflate destination buffer");
        }
        status = compress2 (z_dst, &z_dst_nbytes, z_src, z_src_nbytes,
                            aggression);
        if (Z_BUF_ERROR==status) {
            HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "overflow");
        } else if (Z_MEM_ERROR==status) {
            HGOTO_ERROR (H5E_PLINE, H5E_CANTINIT, 0, "deflate memory error");
        } else if (Z_OK!=status) {
            HGOTO_ERROR (H5E_PLINE, H5E_CANTINIT, 0, "deflate error");
        } else {
            H5F_istore_chunk_free(*buf);
            *buf = outbuf;
            outbuf = NULL;
            *buf_size = nbytes;
            ret_value = z_dst_nbytes;
        }
    }
#else
    HGOTO_ERROR (H5E_PLINE, H5E_UNSUPPORTED, 0,
                 "hdf5 was not compiled with zlib-1.0.2 or better");
#endif

done:
    if(outbuf)
        H5F_istore_chunk_free(outbuf);
    FUNC_LEAVE (ret_value);
    flags = 0;
    nbytes = 0;
    buf_size = 0;
    buf = 0;
}
