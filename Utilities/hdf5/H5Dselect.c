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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.ued>
 *              Thursday, September 30, 2004
 *
 * Purpose:  Dataspace I/O functions.
 */

#define H5D_PACKAGE    /*suppress error about including H5Dpkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5Dpkg.h"    /* Datasets        */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free Lists                           */

/* Declare a free list to manage sequences of size_t */
H5FL_SEQ_DEFINE_STATIC(size_t);

/* Declare a free list to manage sequences of hsize_t */
H5FL_SEQ_DEFINE_STATIC(hsize_t);


/*-------------------------------------------------------------------------
 * Function:  H5D_select_fscat
 *
 * Purpose:  Scatters dataset elements from the type conversion buffer BUF
 *    to the file F where the data points are arranged according to
 *    the file dataspace FILE_SPACE and stored according to
 *    LAYOUT and EFL. Each element is ELMT_SIZE bytes.
 *    The caller is requesting that NELMTS elements are copied.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, June 20, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_select_fscat (H5D_io_info_t *io_info,
    const H5S_t *space, H5S_sel_iter_t *iter, size_t nelmts,
    const void *_buf)
{
    const uint8_t *buf=_buf;       /* Alias for pointer arithmetic */
    hsize_t _off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];             /* Array to store sequence offsets */
    hsize_t *off=NULL;             /* Pointer to sequence offsets */
    hsize_t mem_off;               /* Offset in memory */
    size_t mem_curr_seq;           /* "Current sequence" in memory */
    size_t dset_curr_seq;          /* "Current sequence" in dataset */
    size_t _len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];              /* Array to store sequence lengths */
    size_t *len=NULL;              /* Array to store sequence lengths */
    size_t orig_mem_len, mem_len;  /* Length of sequence in memory */
    size_t  nseq;                  /* Number of sequences generated */
    size_t  nelem;                 /* Number of elements used in sequences */
    herr_t  ret_value=SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_fscat, FAIL);

    /* Check args */
    assert (io_info);
    assert (space);
    assert (iter);
    assert (nelmts>0);
    assert (_buf);
    assert(TRUE==H5P_isa_class(io_info->dxpl_id,H5P_DATASET_XFER));

    /* Allocate the vector I/O arrays */
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if((len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array");
        if((off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array");
    } /* end if */
    else {
        len=_len;
        off=_off;
    } /* end else */

    /* Loop until all elements are written */
    while(nelmts>0) {
        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space,H5S_GET_SEQ_LIST_SORTED,iter,io_info->dxpl_cache->vec_size,nelmts,&nseq,&nelem,off,len)<0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed");

        /* Reset the current sequence information */
        mem_curr_seq=dset_curr_seq=0;
        orig_mem_len=mem_len=nelem*iter->elmt_size;
        mem_off=0;

        /* Write sequence list out */
        if ((*io_info->ops.writevv)(io_info, nseq, &dset_curr_seq, len, off, 1, &mem_curr_seq, &mem_len, &mem_off, buf)<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error");

        /* Update buffer */
        buf += orig_mem_len;

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if(len!=NULL)
            H5FL_SEQ_FREE(size_t,len);
        if(off!=NULL)
            H5FL_SEQ_FREE(hsize_t,off);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5D_select_fscat() */


/*-------------------------------------------------------------------------
 * Function:  H5D_select_fgath
 *
 * Purpose:  Gathers data points from file F and accumulates them in the
 *    type conversion buffer BUF.  The LAYOUT argument describes
 *    how the data is stored on disk and EFL describes how the data
 *    is organized in external files.  ELMT_SIZE is the size in
 *    bytes of a datum which this function treats as opaque.
 *    FILE_SPACE describes the dataspace of the dataset on disk
 *    and the elements that have been selected for reading (via
 *    hyperslab, etc).  This function will copy at most NELMTS
 *    elements.
 *
 * Return:  Success:  Number of elements copied.
 *    Failure:  0
 *
 * Programmer:  Quincey Koziol
 *              Monday, June 24, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5D_select_fgath (H5D_io_info_t *io_info,
    const H5S_t *space, H5S_sel_iter_t *iter, size_t nelmts,
    void *_buf/*out*/)
{
    uint8_t *buf=_buf;          /* Alias for pointer arithmetic */
    hsize_t _off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];          /* Array to store sequence offsets */
    hsize_t *off=NULL;          /* Pointer to sequence offsets */
    hsize_t mem_off;            /* Offset in memory */
    size_t mem_curr_seq;        /* "Current sequence" in memory */
    size_t dset_curr_seq;       /* "Current sequence" in dataset */
    size_t _len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];           /* Array to store sequence lengths */
    size_t *len=NULL;           /* Pointer to sequence lengths */
    size_t orig_mem_len, mem_len;       /* Length of sequence in memory */
    size_t nseq;                /* Number of sequences generated */
    size_t nelem;               /* Number of elements used in sequences */
    size_t ret_value=nelmts;    /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_fgath, 0);

    /* Check args */
    assert (io_info);
    assert (io_info->dset);
    assert (io_info->store);
    assert (space);
    assert (iter);
    assert (nelmts>0);
    assert (_buf);

    /* Allocate the vector I/O arrays */
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if((len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O length vector array");
        if((off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O offset vector array");
    } /* end if */
    else {
        len=_len;
        off=_off;
    } /* end else */

    /* Loop until all elements are read */
    while(nelmts>0) {
        /* Get list of sequences for selection to read */
        if(H5S_SELECT_GET_SEQ_LIST(space,H5S_GET_SEQ_LIST_SORTED,iter,io_info->dxpl_cache->vec_size,nelmts,&nseq,&nelem,off,len)<0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed");

        /* Reset the current sequence information */
        mem_curr_seq=dset_curr_seq=0;
        orig_mem_len=mem_len=nelem*iter->elmt_size;
        mem_off=0;

        /* Read sequence list in */
        if ((*io_info->ops.readvv)(io_info, nseq, &dset_curr_seq, len, off, 1, &mem_curr_seq, &mem_len, &mem_off, buf)<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, 0, "read error");

        /* Update buffer */
        buf += orig_mem_len;

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if(len!=NULL)
            H5FL_SEQ_FREE(size_t,len);
        if(off!=NULL)
            H5FL_SEQ_FREE(hsize_t,off);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5D_select_fgath() */


/*-------------------------------------------------------------------------
 * Function:  H5D_select_mscat
 *
 * Purpose:  Scatters NELMTS data points from the scatter buffer
 *    TSCAT_BUF to the application buffer BUF.  Each element is
 *    ELMT_SIZE bytes and they are organized in application memory
 *    according to SPACE.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, July 8, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_select_mscat (const void *_tscat_buf, const H5S_t *space,
    H5S_sel_iter_t *iter, size_t nelmts, const H5D_dxpl_cache_t *dxpl_cache,
    void *_buf/*out*/)
{
    uint8_t *buf=(uint8_t *)_buf;   /* Get local copies for address arithmetic */
    const uint8_t *tscat_buf=(const uint8_t *)_tscat_buf;
    hsize_t _off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];          /* Array to store sequence offsets */
    hsize_t *off=NULL;          /* Pointer to sequence offsets */
    size_t _len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];           /* Array to store sequence lengths */
    size_t *len=NULL;           /* Pointer to sequence lengths */
    size_t curr_len;            /* Length of bytes left to process in sequence */
    size_t nseq;                /* Number of sequences generated */
    size_t curr_seq;            /* Current sequence being processed */
    size_t nelem;               /* Number of elements used in sequences */
    herr_t ret_value=SUCCEED;   /* Number of elements scattered */

    FUNC_ENTER_NOAPI(H5D_select_mscat, FAIL);

    /* Check args */
    assert (tscat_buf);
    assert (space);
    assert (iter);
    assert (nelmts>0);
    assert (buf);

    /* Allocate the vector I/O arrays */
    if(dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if((len = H5FL_SEQ_MALLOC(size_t,dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array");
        if((off = H5FL_SEQ_MALLOC(hsize_t,dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array");
    } /* end if */
    else {
        len=_len;
        off=_off;
    } /* end else */

    /* Loop until all elements are written */
    while(nelmts>0) {
        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space,0,iter,dxpl_cache->vec_size,nelmts,&nseq,&nelem,off,len)<0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed");

        /* Loop, while sequences left to process */
        for(curr_seq=0; curr_seq<nseq; curr_seq++) {
            /* Get the number of bytes in sequence */
            curr_len=len[curr_seq];

            HDmemcpy(buf+off[curr_seq],tscat_buf,curr_len);

            /* Advance offset in destination buffer */
            tscat_buf+=curr_len;
        } /* end for */

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    if(dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if(len!=NULL)
            H5FL_SEQ_FREE(size_t,len);
        if(off!=NULL)
            H5FL_SEQ_FREE(hsize_t,off);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5D_select_mscat() */


/*-------------------------------------------------------------------------
 * Function:  H5D_select_mgath
 *
 * Purpose:  Gathers dataset elements from application memory BUF and
 *    copies them into the gather buffer TGATH_BUF.
 *    Each element is ELMT_SIZE bytes and arranged in application
 *    memory according to SPACE.
 *    The caller is requesting that at most NELMTS be gathered.
 *
 * Return:  Success:  Number of elements copied.
 *    Failure:  0
 *
 * Programmer:  Quincey Koziol
 *              Monday, June 24, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5D_select_mgath (const void *_buf, const H5S_t *space,
    H5S_sel_iter_t *iter, size_t nelmts, const H5D_dxpl_cache_t *dxpl_cache,
    void *_tgath_buf/*out*/)
{
    const uint8_t *buf=(const uint8_t *)_buf;   /* Get local copies for address arithmetic */
    uint8_t *tgath_buf=(uint8_t *)_tgath_buf;
    hsize_t _off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];          /* Array to store sequence offsets */
    hsize_t *off=NULL;          /* Pointer to sequence offsets */
    size_t _len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];           /* Array to store sequence lengths */
    size_t *len=NULL;           /* Pointer to sequence lengths */
    size_t curr_len;            /* Length of bytes left to process in sequence */
    size_t nseq;                /* Number of sequences generated */
    size_t curr_seq;            /* Current sequence being processed */
    size_t nelem;               /* Number of elements used in sequences */
    size_t ret_value=nelmts;    /* Number of elements gathered */

    FUNC_ENTER_NOAPI(H5D_select_mgath, 0);

    /* Check args */
    assert (buf);
    assert (space);
    assert (iter);
    assert (nelmts>0);
    assert (tgath_buf);

    /* Allocate the vector I/O arrays */
    if(dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if((len = H5FL_SEQ_MALLOC(size_t,dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O length vector array");
        if((off = H5FL_SEQ_MALLOC(hsize_t,dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O offset vector array");
    } /* end if */
    else {
        len=_len;
        off=_off;
    } /* end else */

    /* Loop until all elements are written */
    while(nelmts>0) {
        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space,0,iter,dxpl_cache->vec_size,nelmts,&nseq,&nelem,off,len)<0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed");

        /* Loop, while sequences left to process */
        for(curr_seq=0; curr_seq<nseq; curr_seq++) {
            /* Get the number of bytes in sequence */
            curr_len=len[curr_seq];

            HDmemcpy(tgath_buf,buf+off[curr_seq],curr_len);

            /* Advance offset in gather buffer */
            tgath_buf+=curr_len;
        } /* end for */

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    if(dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if(len!=NULL)
            H5FL_SEQ_FREE(size_t,len);
        if(off!=NULL)
            H5FL_SEQ_FREE(hsize_t,off);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5D_select_mgath() */


/*-------------------------------------------------------------------------
 * Function:  H5D_select_read
 *
 * Purpose:  Reads directly from file into application memory.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 23, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_select_read(H5D_io_info_t *io_info,
    size_t nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    void *buf/*out*/)
{
    H5S_sel_iter_t mem_iter;    /* Memory selection iteration info */
    hbool_t mem_iter_init=0;    /* Memory selection iteration info has been initialized */
    H5S_sel_iter_t file_iter;   /* File selection iteration info */
    hbool_t file_iter_init=0;  /* File selection iteration info has been initialized */
    hsize_t _mem_off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];      /* Array to store sequence offsets in memory */
    hsize_t *mem_off=NULL;      /* Pointer to sequence offsets in memory */
    hsize_t _file_off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];     /* Array to store sequence offsets in the file */
    hsize_t *file_off=NULL;     /* Pointer to sequence offsets in the file */
    size_t _mem_len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];       /* Array to store sequence lengths in memory */
    size_t *mem_len=NULL;       /* Pointer to sequence lengths in memory */
    size_t _file_len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];      /* Array to store sequence lengths in the file */
    size_t *file_len=NULL;      /* Pointer to sequence lengths in the file */
    size_t mem_nseq;            /* Number of sequences generated in the file */
    size_t file_nseq;           /* Number of sequences generated in memory */
    size_t mem_nelem;           /* Number of elements used in memory sequences */
    size_t file_nelem;          /* Number of elements used in file sequences */
    size_t curr_mem_seq;        /* Current memory sequence to operate on */
    size_t curr_file_seq;       /* Current file sequence to operate on */
    ssize_t tmp_file_len;       /* Temporary number of bytes in file sequence */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_read, FAIL);

    /* Check args */
    assert(io_info);
    assert(io_info->dset);
    assert(io_info->dxpl_cache);
    assert(io_info->store);
    assert(buf);
    assert(TRUE==H5P_isa_class(io_info->dxpl_id,H5P_DATASET_XFER));

    /* Initialize file iterator */
    if (H5S_select_iter_init(&file_iter, file_space, elmt_size)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    file_iter_init=1;  /* File selection iteration info has been initialized */

    /* Initialize memory iterator */
    if (H5S_select_iter_init(&mem_iter, mem_space, elmt_size)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    mem_iter_init=1;  /* Memory selection iteration info has been initialized */

    /* Allocate the vector I/O arrays */
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if((mem_len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array");
        if((mem_off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array");
        if((file_len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array");
        if((file_off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array");
    } /* end if */
    else {
        mem_len=_mem_len;
        mem_off=_mem_off;
        file_len=_file_len;
        file_off=_file_off;
    } /* end else */

    /* Initialize sequence counts */
    curr_mem_seq=curr_file_seq=0;
    mem_nseq=file_nseq=0;

    /* Loop, until all bytes are processed */
    while(nelmts>0) {
        /* Check if more file sequences are needed */
        if(curr_file_seq>=file_nseq) {
            /* Get sequences for file selection */
            if(H5S_SELECT_GET_SEQ_LIST(file_space,H5S_GET_SEQ_LIST_SORTED,&file_iter,io_info->dxpl_cache->vec_size,nelmts,&file_nseq,&file_nelem,file_off,file_len)<0)
                HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed");

            /* Start at the beginning of the sequences again */
            curr_file_seq=0;
        } /* end if */

        /* Check if more memory sequences are needed */
        if(curr_mem_seq>=mem_nseq) {
            /* Get sequences for memory selection */
            if(H5S_SELECT_GET_SEQ_LIST(mem_space,0,&mem_iter,io_info->dxpl_cache->vec_size,nelmts,&mem_nseq,&mem_nelem,mem_off,mem_len)<0)
                HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed");

            /* Start at the beginning of the sequences again */
            curr_mem_seq=0;
        } /* end if */

        /* Read file sequences into current memory sequence */
        if ((tmp_file_len=(*io_info->ops.readvv)(io_info,
                file_nseq, &curr_file_seq, file_len, file_off,
                mem_nseq, &curr_mem_seq, mem_len, mem_off,
                buf))<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, FAIL, "read error");

        /* Decrement number of elements left to process */
        assert((tmp_file_len%elmt_size)==0);
        nelmts-=(tmp_file_len/elmt_size);
    } /* end while */

done:
    /* Release file selection iterator */
    if(file_iter_init) {
        if (H5S_SELECT_ITER_RELEASE(&file_iter)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

    /* Release memory selection iterator */
    if(mem_iter_init) {
        if (H5S_SELECT_ITER_RELEASE(&mem_iter)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

    /* Free vector arrays */
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if(file_len!=NULL)
            H5FL_SEQ_FREE(size_t,file_len);
        if(file_off!=NULL)
            H5FL_SEQ_FREE(hsize_t,file_off);
        if(mem_len!=NULL)
            H5FL_SEQ_FREE(size_t,mem_len);
        if(mem_off!=NULL)
            H5FL_SEQ_FREE(hsize_t,mem_off);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_select_read() */


/*-------------------------------------------------------------------------
 * Function:  H5D_select_write
 *
 * Purpose:  Writes directly from application memory into a file
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 23, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_select_write(H5D_io_info_t *io_info,
    size_t nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    const void *buf/*out*/)
{
    H5S_sel_iter_t mem_iter;    /* Memory selection iteration info */
    hbool_t mem_iter_init=0;    /* Memory selection iteration info has been initialized */
    H5S_sel_iter_t file_iter;   /* File selection iteration info */
    hbool_t file_iter_init=0;  /* File selection iteration info has been initialized */
    hsize_t _mem_off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];      /* Array to store sequence offsets in memory */
    hsize_t *mem_off=NULL;      /* Pointer to sequence offsets in memory */
    hsize_t _file_off[H5D_XFER_HYPER_VECTOR_SIZE_DEF];     /* Array to store sequence offsets in the file */
    hsize_t *file_off=NULL;     /* Pointer to sequence offsets in the file */
    size_t _mem_len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];       /* Array to store sequence lengths in memory */
    size_t *mem_len=NULL;       /* Pointer to sequence lengths in memory */
    size_t _file_len[H5D_XFER_HYPER_VECTOR_SIZE_DEF];      /* Array to store sequence lengths in the file */
    size_t *file_len=NULL;      /* Pointer to sequence lengths in the file */
    size_t mem_nseq;            /* Number of sequences generated in the file */
    size_t file_nseq;           /* Number of sequences generated in memory */
    size_t mem_nelem;           /* Number of elements used in memory sequences */
    size_t file_nelem;          /* Number of elements used in file sequences */
    size_t curr_mem_seq;        /* Current memory sequence to operate on */
    size_t curr_file_seq;       /* Current file sequence to operate on */
    ssize_t tmp_file_len;       /* Temporary number of bytes in file sequence */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_write, FAIL);

    /* Check args */
    assert(io_info);
    assert(io_info->dset);
    assert(io_info->store);
    assert(TRUE==H5P_isa_class(io_info->dxpl_id,H5P_DATASET_XFER));
    assert(buf);

    /* Allocate the vector I/O arrays */
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if((mem_len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array");
        if((mem_off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array");
        if((file_len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array");
        if((file_off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array");
    } /* end if */
    else {
        mem_len=_mem_len;
        mem_off=_mem_off;
        file_len=_file_len;
        file_off=_file_off;
    } /* end else */

    /* Initialize file iterator */
    if (H5S_select_iter_init(&file_iter, file_space, elmt_size)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    file_iter_init=1;  /* File selection iteration info has been initialized */

    /* Initialize memory iterator */
    if (H5S_select_iter_init(&mem_iter, mem_space, elmt_size)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    mem_iter_init=1;  /* Memory selection iteration info has been initialized */

    /* Initialize sequence counts */
    curr_mem_seq=curr_file_seq=0;
    mem_nseq=file_nseq=0;

    /* Loop, until all bytes are processed */
    while(nelmts>0) {
        /* Check if more file sequences are needed */
        if(curr_file_seq>=file_nseq) {
            /* Get sequences for file selection */
            if(H5S_SELECT_GET_SEQ_LIST(file_space,H5S_GET_SEQ_LIST_SORTED,&file_iter,io_info->dxpl_cache->vec_size,nelmts,&file_nseq,&file_nelem,file_off,file_len)<0)
                HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed");

            /* Start at the beginning of the sequences again */
            curr_file_seq=0;
        } /* end if */

        /* Check if more memory sequences are needed */
        if(curr_mem_seq>=mem_nseq) {
            /* Get sequences for memory selection */
            if(H5S_SELECT_GET_SEQ_LIST(mem_space,0,&mem_iter,io_info->dxpl_cache->vec_size,nelmts,&mem_nseq,&mem_nelem,mem_off,mem_len)<0)
                HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed");

            /* Start at the beginning of the sequences again */
            curr_mem_seq=0;
        } /* end if */

        /* Write memory sequences into file sequences */
        if ((tmp_file_len=(*io_info->ops.writevv)(io_info,
                file_nseq, &curr_file_seq, file_len, file_off,
                mem_nseq, &curr_mem_seq, mem_len, mem_off,
                buf))<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error");

        /* Decrement number of elements left to process */
        assert((tmp_file_len%elmt_size)==0);
        nelmts-=(tmp_file_len/elmt_size);
    } /* end while */

done:
    /* Release file selection iterator */
    if(file_iter_init) {
        if (H5S_SELECT_ITER_RELEASE(&file_iter)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

    /* Release memory selection iterator */
    if(mem_iter_init) {
        if (H5S_SELECT_ITER_RELEASE(&mem_iter)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

    /* Free vector arrays */
    if(io_info->dxpl_cache->vec_size!=H5D_XFER_HYPER_VECTOR_SIZE_DEF) {
        if(file_len!=NULL)
            H5FL_SEQ_FREE(size_t,file_len);
        if(file_off!=NULL)
            H5FL_SEQ_FREE(hsize_t,file_off);
        if(mem_len!=NULL)
            H5FL_SEQ_FREE(size_t,mem_len);
        if(mem_off!=NULL)
            H5FL_SEQ_FREE(hsize_t,mem_off);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_select_write() */

