/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Thursday, January 15, 1998
 *
 * Purpose:     Provides I/O facilities for multi-dimensional arrays of bytes
 *              stored with various layout policies.  If the caller is
 *              interested in arrays of elements >1 byte then add an extra
 *              dimension.  For example, a 10x20 array of int would
 *              translate to a 10x20x4 array of bytes at this level.
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5FDprivate.h"        /*file driver                             */
#include "H5Iprivate.h"
#include "H5MFprivate.h"
#include "H5MMprivate.h"        /*memory management                       */
#include "H5Oprivate.h"
#include "H5Pprivate.h"
#include "H5Vprivate.h"

/* MPIO driver functions are needed for some special checks */
#include "H5FDmpio.h"

/* Interface initialization */
#define PABLO_MASK      H5Farray_mask
#define INTERFACE_INIT  NULL
static int interface_initialize_g = 0;



/*-------------------------------------------------------------------------
 * Function:    H5F_arr_create
 *
 * Purpose:     Creates an array of bytes.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_arr_create (H5F_t *f, struct H5O_layout_t *layout/*in,out*/)
{
    unsigned            u;
    hsize_t             nbytes;
   
    FUNC_ENTER (H5F_arr_create, FAIL);

    /* check args */
    assert (f);
    assert (layout);
    layout->addr = HADDR_UNDEF; /*just in case we fail*/
   
    switch (layout->type) {
        case H5D_CONTIGUOUS:
            /* Reserve space in the file for the entire array */
            for (u=0, nbytes=1; u<layout->ndims; u++)
                nbytes *= layout->dim[u];
            assert (nbytes>0);
            if (HADDR_UNDEF==(layout->addr=H5MF_alloc(f, H5FD_MEM_DRAW, nbytes))) {
                HRETURN_ERROR (H5E_IO, H5E_NOSPACE, FAIL,
                       "unable to reserve file space");
            }
            break;

        case H5D_CHUNKED:
            /* Create the root of the B-tree that describes chunked storage */
            if (H5F_istore_create (f, layout/*out*/)<0) {
                HRETURN_ERROR (H5E_IO, H5E_CANTINIT, FAIL,
                       "unable to initialize chunked storage");
            }
            break;

        default:
            assert ("not implemented yet" && 0);
            HRETURN_ERROR (H5E_IO, H5E_UNSUPPORTED, FAIL,
                   "unsupported storage layout");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_arr_read
 *
 * Purpose:     Reads a hyperslab of a file byte array into a hyperslab of
 *              a byte array in memory.  The data is read from file F and the
 *              array's size and storage information is in LAYOUT.  External
 *              files are described according to the external file list, EFL.
 *              The hyperslab offset is FILE_OFFSET[] in the file and
 *              MEM_OFFSET[] in memory (offsets are relative to the origin of
 *              the array) and the size of the hyperslab is HSLAB_SIZE[]. The
 *              total size of the file array is implied in the LAYOUT
 *              argument and the total size of the memory array is
 *              MEM_SIZE[]. The dimensionality of these vectors is implied by
 *              the LAYOUT argument.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January 16, 1998
 *
 * Modifications:
 *              Albert Cheng, 1998-06-02
 *              Added xfer_mode argument
 *
 *              Robb Matzke, 1998-09-28
 *              Added the `xfer' argument and removed the `xfer_mode'
 *              argument since it's a field of `xfer'.
 *
 *              Robb Matzke, 1999-08-02
 *              Data transfer properties are passed by ID since that's how
 *              the virtual file layer wants them.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_arr_read(H5F_t *f, hid_t dxpl_id, const struct H5O_layout_t *layout,
             const struct H5O_pline_t *pline, const H5O_fill_t *fill,
             const struct H5O_efl_t *efl, const hsize_t _hslab_size[],
             const hsize_t mem_size[], const hssize_t mem_offset[],
             const hssize_t file_offset[], void *_buf/*out*/)
{
    uint8_t     *buf = (uint8_t*)_buf;          /*cast for arithmetic   */
    hssize_t    file_stride[H5O_LAYOUT_NDIMS];  /*strides through file  */
    hssize_t    mem_stride[H5O_LAYOUT_NDIMS];   /*strides through memory*/
    hsize_t     hslab_size[H5O_LAYOUT_NDIMS];   /*hyperslab size        */
    hsize_t     idx[H5O_LAYOUT_NDIMS];          /*multi-dim counter     */
    size_t      mem_start;                      /*byte offset to start  */
    hsize_t     file_start;                     /*byte offset to start  */
    hsize_t     max_data = 0;                   /*bytes in dataset      */
    hsize_t     elmt_size = 1;                  /*bytes per element     */
    size_t      nelmts, z;                      /*number of elements    */
    unsigned    ndims;                          /*stride dimensionality */
    haddr_t     addr;                           /*address in file       */
    int j;                                  /*counters          */
    unsigned    u;                                  /*counters          */
    hbool_t     carray;                         /*carry for subtraction */
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t xfer_mode=H5FD_MPIO_INDEPENDENT;
#endif
#ifdef COALESCE_READS
    H5D_xfer_t *xfer_parms;                     /*transfer property list*/
#endif
   
    FUNC_ENTER(H5F_arr_read, FAIL);

    /* Check args */
    assert(f);
    assert(layout);
    assert(_hslab_size);
    assert(file_offset);
    assert(mem_offset);
    assert(mem_size);
    assert(buf);

    /* Make a local copy of size so we can modify it */
    H5V_vector_cpy(layout->ndims, hslab_size, _hslab_size);

#ifdef H5_HAVE_PARALLEL
    {
        /* Get the transfer mode */
        H5D_xfer_t *dxpl;
        H5FD_mpio_dxpl_t *dx;
        if (H5P_DEFAULT!=dxpl_id && (dxpl=H5I_object(dxpl_id)) &&
            H5FD_MPIO==dxpl->driver_id && (dx=dxpl->driver_info) &&
            H5FD_MPIO_INDEPENDENT!=dx->xfer_mode) {
            xfer_mode = dx->xfer_mode;
        }
    }
#endif
    
#ifdef H5_HAVE_PARALLEL
    /* Collective MPIO access is unsupported for non-contiguous datasets */
    if (H5D_CONTIGUOUS!=layout->type && H5FD_MPIO_COLLECTIVE==xfer_mode) {
        HRETURN_ERROR (H5E_DATASET, H5E_READERROR, FAIL,
                       "collective access on non-contiguous datasets not "
                       "supported yet");
    }
#endif
#ifdef QAK
{
    extern int qak_debug;

    if(qak_debug) {
        printf("%s: layout->ndims=%d\n",FUNC,(int)layout->ndims);
        for(u=0; u<layout->ndims; u++)
            printf("%s: %u: hslab_size=%d, mem_size=%d, mem_offset=%d, file_offset=%d\n",FUNC,u,(int)_hslab_size[u],(int)mem_size[u],(int)mem_offset[u],(int)file_offset[u]);
        printf("%s: *buf=%d, *(buf+1)=%d\n", FUNC,(int)*(const uint16_t *)buf,(int)*((const uint16 *)buf+1));
    }
}
#endif /* QAK */

    switch (layout->type) {
        case H5D_CONTIGUOUS:
            ndims = layout->ndims;
            /*
             * Offsets must not be negative for this type of storage.
             */
            for (u=0; u<ndims; u++) {
                if (mem_offset[u]<0 || file_offset[u]<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                          "negative offsets are not valid");
                }
            }

            /*
             * Filters cannot be used for contiguous data.
             */
            if (pline && pline->nfilters>0) {
                HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                      "filters are not allowed for contiguous data");
            }
            
            /*
             * Calculate the strides needed to walk through the array on disk
             * and memory. Optimize the strides to result in the fewest number of
             * I/O requests.
             */
            mem_start = H5V_hyper_stride(ndims, hslab_size, mem_size,
                             mem_offset, mem_stride/*out*/);
            file_start = H5V_hyper_stride(ndims, hslab_size, layout->dim,
                              file_offset, file_stride/*out*/);
            H5V_stride_optimize2(&ndims, &elmt_size, hslab_size,
                         mem_stride, file_stride);

            /*
             * Initialize loop variables.  The loop is a multi-dimensional loop
             * that counts from SIZE down to zero and IDX is the counter.  Each
             * element of IDX is treated as a digit with IDX[0] being the least
             * significant digit.
             */
            H5V_vector_cpy(ndims, idx, hslab_size);
            nelmts = H5V_vector_reduce_product(ndims, hslab_size);
            if (efl && efl->nused>0) {
                addr = 0;
            } else {
                addr = layout->addr;

                /* Compute the size of the dataset in bytes */
                for(u=0, max_data=1; u<layout->ndims; u++)
                    max_data *= layout->dim[u];

                /* Adjust the maximum size of the data by the offset into it */
                max_data -= file_start;
            }
            addr += file_start;
            buf += mem_start;

            /*
             * Now begin to walk through the array, copying data from disk to
             * memory.
             */
#ifdef H5_HAVE_PARALLEL
            if (H5FD_MPIO_COLLECTIVE==xfer_mode){
                /*
                 * Currently supports same number of collective access. Need to
                 * be changed LATER to combine all reads into one collective MPIO
                 * call.
                 */
                unsigned long max, min, temp;

                temp = nelmts;
                assert(temp==nelmts);   /* verify no overflow */
                MPI_Allreduce(&temp, &max, 1, MPI_UNSIGNED_LONG, MPI_MAX,
                      H5FD_mpio_communicator(f->shared->lf));
                MPI_Allreduce(&temp, &min, 1, MPI_UNSIGNED_LONG, MPI_MIN,
                      H5FD_mpio_communicator(f->shared->lf));
#ifdef AKC
                printf("nelmts=%lu, min=%lu, max=%lu\n", temp, min, max);
#endif
                if (max != min)
                HRETURN_ERROR(H5E_DATASET, H5E_READERROR, FAIL,
                          "collective access with unequal number of "
                          "blocks not supported yet");
            }
#endif

#ifdef COALESCE_READS
                /* Get the dataset transfer property list */
                if (H5P_DEFAULT == dxpl_id) {
                    xfer_parms = &H5D_xfer_dflt;
                } else if (H5P_DATASET_XFER != H5P_get_class (dxpl_id) ||
                       NULL == (xfer_parms = H5I_object (dxpl_id))) {
                    HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms");
                }

            for (z=0, xfer_parms->gather_reads = nelmts - 1;
                     z<nelmts;
                     z++, xfer_parms->gather_reads--) {
#else
#ifdef QAK
        printf("%s: nelmts=%d, addr=%lu, elmt_size=%lu\n",FUNC,(int)nelmts,(unsigned long)addr,(unsigned long)elmt_size);
        printf("%s: sieve_buf=%p, sieve_loc=%lu, sieve_size=%lu, sieve_buf_size=%lu, sieve_dirty=%u\n",FUNC,f->shared->sieve_buf,(unsigned long)f->shared->sieve_loc,(unsigned long)f->shared->sieve_size,(unsigned long)f->shared->sieve_buf_size,(unsigned)f->shared->sieve_dirty);
        printf("%s: feature_flags=%lx\n",FUNC,(unsigned long)f->shared->lf->feature_flags);
#endif /* QAK */
            for (z=0; z<nelmts; z++) {
#endif

                /* Read directly from file if the dataset is in an external file */
                /* Note: We can't use data sieve buffers for datasets in external files
                 *  because the 'addr' of all external files is set to 0 (above) and
                 *  all datasets in external files would alias to the same set of
                 *  file offsets, totally mixing up the data sieve buffer information. -QAK
                 */
                if (efl && efl->nused>0) {
                    if (H5O_efl_read(f, efl, addr, elmt_size, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                              "external data read failed");
                    }
                } else {
                    if (H5F_contig_read(f, max_data, H5FD_MEM_DRAW, addr, elmt_size, dxpl_id, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                                  "block read failed");
                    }
                } /* end else */

                /* Decrement indices and advance pointers */
                for (j=ndims-1, carray=TRUE; j>=0 && carray; --j) {
                    addr += file_stride[j];
                    buf += mem_stride[j];

                    /* Adjust the maximum size of the data by the offset into it */
                    max_data -= file_stride[j];

                    if (--idx[j])
                        carray = FALSE;
                    else
                        idx[j] = hslab_size[j];
                }
            }
            break;

        case H5D_CHUNKED:
            /*
             * This method is unable to access external raw data files or to copy
             * into a proper hyperslab.
             */
            if (efl && efl->nused>0) {
                HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                      "chunking and external files are mutually exclusive");
            }
            for (u=0; u<layout->ndims; u++) {
                if (0!=mem_offset[u] || hslab_size[u]!=mem_size[u]) {
                    HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                          "unable to copy into a proper hyperslab");
                }
            }
            if (H5F_istore_read(f, dxpl_id, layout, pline, fill, file_offset,
                         hslab_size, buf)<0) {
                HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked read failed");
            }
            break;

        default:
            assert("not implemented yet" && 0);
            HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                  "unsupported storage layout");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_arr_write
 *
 * Purpose:     Copies a hyperslab of a memory array to a hyperslab of a
 *              file array.  The data is written to file F and the file
 *              array's size and storage information is implied by LAYOUT.
 *              The data is stored in external files according to the
 *              external file list, EFL. The hyperslab offset is
 *              FILE_OFFSET[] in the file and MEM_OFFSET[] in memory (offsets
 *              are relative to the origin of the array) and the size of the
 *              hyperslab is HSLAB_SIZE[].  The total size of the file array
 *              is implied by the LAYOUT argument and the total size of the
 *              memory array is MEM_SIZE[].  The dimensionality of these
 *              vectors is implied by the LAYOUT argument.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January 16, 1998
 *
 * Modifications:
 *              Albert Cheng, 1998-06-02
 *              Added xfer_mode argument
 *
 *              Robb Matzke, 1998-09-28
 *              Added `xfer' argument, removed `xfer_mode' argument since it
 *              is a member of H5D_xfer_t.
 *
 *              Robb Matzke, 1999-08-02
 *              Data transfer properties are passed by ID since that's how
 *              the virtual file layer wants them.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_arr_write(H5F_t *f, hid_t dxpl_id, const struct H5O_layout_t *layout,
              const struct H5O_pline_t *pline,
              const struct H5O_fill_t *fill, const struct H5O_efl_t *efl,
              const hsize_t _hslab_size[], const hsize_t mem_size[],
              const hssize_t mem_offset[], const hssize_t file_offset[],
              const void *_buf)
{
    const uint8_t *buf = (const uint8_t *)_buf; /*cast for arithmetic   */
    hssize_t    file_stride[H5O_LAYOUT_NDIMS];  /*strides through file  */
    hssize_t    mem_stride[H5O_LAYOUT_NDIMS];   /*strides through memory*/
    hsize_t     hslab_size[H5O_LAYOUT_NDIMS];   /*hyperslab size        */
    hsize_t     idx[H5O_LAYOUT_NDIMS];          /*multi-dim counter     */
    hsize_t     mem_start;                      /*byte offset to start  */
    hsize_t     file_start;                     /*byte offset to start  */
    hsize_t     max_data = 0;                   /*bytes in dataset      */
    hsize_t     elmt_size = 1;                  /*bytes per element     */
    size_t      nelmts, z;                      /*number of elements    */
    unsigned    ndims;                          /*dimensionality        */
    haddr_t     addr;                           /*address in file       */
    int j;                                  /*counters          */
    unsigned    u;                                  /*counters          */
    hbool_t     carray;                         /*carry for subtraction */
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t xfer_mode=H5FD_MPIO_INDEPENDENT;
#endif
   
    FUNC_ENTER(H5F_arr_write, FAIL);

    /* Check args */
    assert(f);
    assert(layout);
    assert(_hslab_size);
    assert(file_offset);
    assert(mem_offset);
    assert(mem_size);
    assert(buf);

    /* Make a local copy of _size so we can modify it */
    H5V_vector_cpy(layout->ndims, hslab_size, _hslab_size);

#ifdef H5_HAVE_PARALLEL
    {
        /* Get the transfer mode */
        H5D_xfer_t *dxpl;
        H5FD_mpio_dxpl_t *dx;
        if (H5P_DEFAULT!=dxpl_id && (dxpl=H5I_object(dxpl_id)) &&
            H5FD_MPIO==dxpl->driver_id && (dx=dxpl->driver_info) &&
            H5FD_MPIO_INDEPENDENT!=dx->xfer_mode) {
            xfer_mode = dx->xfer_mode;
        }
    }
#endif
    
#ifdef H5_HAVE_PARALLEL
    if (H5D_CONTIGUOUS!=layout->type && H5FD_MPIO_COLLECTIVE==xfer_mode) {
        HRETURN_ERROR (H5E_DATASET, H5E_WRITEERROR, FAIL,
                       "collective access on non-contiguous datasets not "
                       "supported yet");
    }
#endif
    
#ifdef QAK
    {
        extern int qak_debug;

        printf("%s: layout->ndims=%d\n",FUNC,(int)layout->ndims);
        for(i=0; i<layout->ndims; i++)
            printf("%s: %d: hslab_size=%d, mem_size=%d, mem_offset=%d, "
                   "file_offset=%d\n", FUNC, i, (int)_hslab_size[i],
                   (int)mem_size[i],(int)mem_offset[i],(int)file_offset[i]);
        if(qak_debug) {
            printf("%s: *buf=%d, *(buf+1)=%d\n", FUNC,
                   (int)*(const uint16_t *)buf, (int)*((const uint16_t *)buf+1));
        }
    }
#endif /* QAK */

    switch (layout->type) {
        case H5D_CONTIGUOUS:
            ndims = layout->ndims;
            /*
             * Offsets must not be negative for this type of storage.
             */
            for (u=0; u<ndims; u++) {
                if (mem_offset[u]<0 || file_offset[u]<0) {
                    HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                          "negative offsets are not valid");
                }
            }

            /*
             * Filters cannot be used for contiguous data
             */
            if (pline && pline->nfilters>0) {
                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                      "filters are not allowed for contiguous data");
            }
            
            /*
             * Calculate the strides needed to walk through the array on disk.
             * Optimize the strides to result in the fewest number of I/O
             * requests.
             */
            mem_start = H5V_hyper_stride(ndims, hslab_size, mem_size,
                             mem_offset, mem_stride/*out*/);
            file_start = H5V_hyper_stride(ndims, hslab_size, layout->dim,
                              file_offset, file_stride/*out*/);
            H5V_stride_optimize2(&ndims, &elmt_size, hslab_size,
                         mem_stride, file_stride);

            /*
             * Initialize loop variables.  The loop is a multi-dimensional loop
             * that counts from SIZE down to zero and IDX is the counter.  Each
             * element of IDX is treated as a digit with IDX[0] being the least
             * significant digit.
             */
            H5V_vector_cpy(ndims, idx, hslab_size);
            nelmts = H5V_vector_reduce_product(ndims, hslab_size);
            if (efl && efl->nused>0) {
                addr = 0;
            } else {
                addr = layout->addr;

                /* Compute the size of the dataset in bytes */
                for(u=0, max_data=1; u<layout->ndims; u++)
                    max_data *= layout->dim[u];

                /* Adjust the maximum size of the data by the offset into it */
                max_data -= file_start;
            }
            addr += file_start;
            buf += mem_start;

            /*
             * Now begin to walk through the array, copying data from memory to
             * disk.
             */
#ifdef H5_HAVE_PARALLEL
            if (H5FD_MPIO_COLLECTIVE==xfer_mode){
                /*
                 * Currently supports same number of collective access. Need to
                 * be changed LATER to combine all writes into one collective
                 * MPIO call.
                 */
                unsigned long max, min, temp;

                temp = nelmts;
                assert(temp==nelmts);   /* verify no overflow */
                MPI_Allreduce(&temp, &max, 1, MPI_UNSIGNED_LONG, MPI_MAX,
                      H5FD_mpio_communicator(f->shared->lf));
                MPI_Allreduce(&temp, &min, 1, MPI_UNSIGNED_LONG, MPI_MIN,
                      H5FD_mpio_communicator(f->shared->lf));
#ifdef AKC
                printf("nelmts=%lu, min=%lu, max=%lu\n", temp, min, max);
#endif
                if (max != min) {
                HRETURN_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL,
                          "collective access with unequal number of "
                          "blocks not supported yet");
                }
            }
#endif

            for (z=0; z<nelmts; z++) {

                /* Write to file */
                if (efl && efl->nused>0) {
                    if (H5O_efl_write(f, efl, addr, elmt_size, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                              "external data write failed");
                    }
                } else {
                    if (H5F_contig_write(f, max_data, H5FD_MEM_DRAW, addr, elmt_size, dxpl_id, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                                  "block write failed");
                    }
                } /* end else */

                /* Decrement indices and advance pointers */
                for (j=ndims-1, carray=TRUE; j>=0 && carray; --j) {
                    addr += file_stride[j];
                    buf += mem_stride[j];
                    
                    /* Adjust the maximum size of the data by the offset into it */
                    max_data -= file_stride[j];
                    
                    if (--idx[j])
                        carray = FALSE;
                    else
                        idx[j] = hslab_size[j];
                }

            }
            break;

        case H5D_CHUNKED:
            /*
             * This method is unable to access external raw data files or to copy
             * from a proper hyperslab.
             */
            if (efl && efl->nused>0) {
                HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                      "chunking and external files are mutually "
                      "exclusive");
            }
            for (u=0; u<layout->ndims; u++) {
                if (0!=mem_offset[u] || hslab_size[u]!=mem_size[u]) {
                    HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                          "unable to copy from a proper hyperslab");
                }
            }
            if (H5F_istore_write(f, dxpl_id, layout, pline, fill, file_offset,
                         hslab_size, buf)<0) {
                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                      "chunked write failed");
            }
            break;

        default:
            assert("not implemented yet" && 0);
            HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                  "unsupported storage layout");
    }

    FUNC_LEAVE (SUCCEED);
}
