/*
 * Copyright (C) 2000 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Thursday, September 28, 2000
 *
 * Purpose:     Provides I/O facilities for sequences of bytes stored with various 
 *      layout policies.  These routines are similar to the H5Farray.c routines,
 *      these deal in terms of byte offsets and lengths, not coordinates and
 *      hyperslab sizes.
 *
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
#define PABLO_MASK      H5Fseq_mask
#define INTERFACE_INIT  NULL
static int interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:    H5F_seq_read
 *
 * Purpose:     Reads a sequence of bytes from a file dataset into a buffer in
 *      in memory.  The data is read from file F and the array's size and
 *      storage information is in LAYOUT.  External files are described
 *      according to the external file list, EFL.  The sequence offset is 
 *      FILE_OFFSET in the file (offsets are
 *      in terms of bytes) and the size of the hyperslab is SEQ_LEN. The
 *              total size of the file array is implied in the LAYOUT argument.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, September 28, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_seq_read(H5F_t *f, hid_t dxpl_id, const struct H5O_layout_t *layout,
             const struct H5O_pline_t *pline, const H5O_fill_t *fill,
             const struct H5O_efl_t *efl, const H5S_t *file_space, size_t elmt_size,
         hsize_t seq_len, hsize_t file_offset, void *buf/*out*/)
{
    hsize_t     dset_dims[H5O_LAYOUT_NDIMS];    /* dataspace dimensions */
    hssize_t    coords[H5O_LAYOUT_NDIMS];       /* offset of hyperslab in dataspace */
    hsize_t     hslab_size[H5O_LAYOUT_NDIMS];   /* hyperslab size in dataspace*/
    hsize_t     down_size[H5O_LAYOUT_NDIMS];    /* Cumulative yperslab sizes (in elements) */
    hsize_t     acc;    /* Accumulator for hyperslab sizes (in elements) */
    int ndims;
    hsize_t     max_data = 0;                   /*bytes in dataset      */
    haddr_t     addr=0;                         /*address in file       */
    unsigned    u;                              /*counters              */
    int i,j;                            /*counters              */
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t xfer_mode=H5FD_MPIO_INDEPENDENT;
#endif
   
    FUNC_ENTER(H5F_seq_read, FAIL);

    /* Check args */
    assert(f);
    assert(layout);
    assert(buf);

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

    /* Collective MPIO access is unsupported for non-contiguous datasets */
    if (H5D_CONTIGUOUS!=layout->type && H5FD_MPIO_COLLECTIVE==xfer_mode) {
        HRETURN_ERROR (H5E_DATASET, H5E_READERROR, FAIL,
           "collective access on non-contiguous datasets not supported yet");
    }
#endif

    switch (layout->type) {
        case H5D_CONTIGUOUS:
            /* Filters cannot be used for contiguous data. */
            if (pline && pline->nfilters>0) {
                HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                      "filters are not allowed for contiguous data");
            }
            
            /*
             * Initialize loop variables.  The loop is a multi-dimensional loop
             * that counts from SIZE down to zero and IDX is the counter.  Each
             * element of IDX is treated as a digit with IDX[0] being the least
             * significant digit.
             */
            if (efl && efl->nused>0) {
                addr = 0;
            } else {
                addr = layout->addr;

                /* Compute the size of the dataset in bytes */
                for(u=0, max_data=1; u<layout->ndims; u++)
                    max_data *= layout->dim[u];

                /* Adjust the maximum size of the data by the offset into it */
                max_data -= file_offset;
            }
            addr += file_offset;

            /*
             * Now begin to walk through the array, copying data from disk to
             * memory.
             */
#ifdef H5_HAVE_PARALLEL
            if (H5FD_MPIO_COLLECTIVE==xfer_mode) {
                /*
                 * Currently supports same number of collective access. Need to
                 * be changed LATER to combine all reads into one collective MPIO
                 * call.
                 */
                unsigned long max, min, temp;

                temp = seq_len;
                assert(temp==seq_len);  /* verify no overflow */
                MPI_Allreduce(&temp, &max, 1, MPI_UNSIGNED_LONG, MPI_MAX,
                      H5FD_mpio_communicator(f->shared->lf));
                MPI_Allreduce(&temp, &min, 1, MPI_UNSIGNED_LONG, MPI_MIN,
                      H5FD_mpio_communicator(f->shared->lf));
#ifdef AKC
                printf("seq_len=%lu, min=%lu, max=%lu\n", temp, min, max);
#endif
                if (max != min)
                    HRETURN_ERROR(H5E_DATASET, H5E_READERROR, FAIL,
                      "collective access with unequal number of blocks not supported yet");
            }
#endif

            /* Read directly from file if the dataset is in an external file */
            /* Note: We can't use data sieve buffers for datasets in external files
             *  because the 'addr' of all external files is set to 0 (above) and
             *  all datasets in external files would alias to the same set of
             *  file offsets, totally mixing up the data sieve buffer information. -QAK
             */
            if (efl && efl->nused>0) {
                if (H5O_efl_read(f, efl, addr, seq_len, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                          "external data read failed");
                }
            } else {
                if (H5F_contig_read(f, max_data, H5FD_MEM_DRAW, addr, seq_len, dxpl_id, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                              "block read failed");
                }
            } /* end else */
            break;

        case H5D_CHUNKED:
        {
            unsigned       leading_partials;       /* Flag set if there are leading partial hyperslabs to take care of */

            /*
             * This method is unable to access external raw data files 
             */
            if (efl && efl->nused>0) {
                HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                      "chunking and external files are mutually exclusive");
            }
            /* Compute the file offset coordinates and hyperslab size */
            if((ndims=H5S_get_simple_extent_dims(file_space,dset_dims,NULL))<0)
                HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unable to retrieve dataspace dimensions");
            
#ifdef QAK
            /* The library shouldn't be reading partial elements currently */
            assert(seq_len%elmt_size!=0);
            assert(addr%elmt_size!=0);
#endif /* QAK */

#ifdef QAK
/* Print out the file offsets & hyperslab sizes */
{
    static int count=0;

    if(count<1000000) {
        printf("%s: elmt_size=%d, addr=%d, seq_len=%d\n",FUNC,(int)elmt_size,(int)addr,(int)seq_len);
        printf("%s: file_offset=%d\n",FUNC,(int)file_offset);
        count++;
    }
}
#endif /* QAK */
            /* Set location in dataset from the file_offset */
            addr=file_offset;

            /* Convert the bytes into elements */
            seq_len/=elmt_size;
            addr/=elmt_size;

            /* Build the array of cumulative hyperslab sizes */
            for(acc=1, i=(ndims-1); i>=0; i--) {
                down_size[i]=acc;
                acc*=dset_dims[i];
#ifdef QAK
printf("%s: acc=%ld, down_size[%d]=%ld\n",FUNC,(long)acc,i,(long)down_size[i]);
#endif /* QAK */
            } /* end for */

            /* Compute the hyperslab offset from the address given */
            leading_partials=0;
            for(i=ndims-1; i>=0; i--) {
                coords[i]=addr%dset_dims[i];
                addr/=dset_dims[i];
                if(i>0 && coords[i]>0)
                    leading_partials=1;
#ifdef QAK
printf("%s: addr=%lu, coords[%d]=%ld\n",FUNC,(unsigned long)addr,i,(long)coords[i]);
#endif /* QAK */
            } /* end for */
            coords[ndims]=0;   /* No offset for element info */
#ifdef QAK
printf("%s: addr=%lu, coords[%d]=%ld\n",FUNC,(unsigned long)addr,ndims,(long)coords[ndims]);
printf("%s: leading_partials=%u\n",FUNC,leading_partials);
#endif /* QAK */

            /*
             * Peel off initial partial hyperslabs until we've got a hyperslab which starts
             *      at coord[n]==0 for dimensions 1->(ndims-1)  (i.e. starting at coordinate
             *      zero for all dimensions except the slowest changing one
             */
            for(i=ndims-1; i>0 && seq_len>=down_size[i]; i--) {
                hsize_t partial_size;       /* Size of the partial hyperslab in bytes */

                /* Check if we have a partial hyperslab in this lower dimension */
                if(coords[i]>0) {
#ifdef QAK
printf("%s: Need to get hyperslab, seq_len=%ld, coords[%d]=%ld\n",FUNC,(long)seq_len,i,(long)coords[i]);
#endif /* QAK */
                    /* Reset the partial hyperslab size */
                    partial_size=1;

                    /* Build the partial hyperslab information */
                    for(j=0; j<ndims; j++) {
                        if(i==j)
                            hslab_size[j]=MIN(seq_len/down_size[i],dset_dims[i]-coords[i]);
                        else
                            if(j>i)
                                hslab_size[j]=dset_dims[j];
                            else
                                hslab_size[j]=1;
                        partial_size*=hslab_size[j];
#ifdef QAK
printf("%s: partial_size=%lu, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,j,(long)hslab_size[j]);
#endif /* QAK */
                    } /* end for */
                    hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */
#ifdef QAK
printf("%s: partial_size=%lu, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,ndims,(long)hslab_size[ndims]);
#endif /* QAK */

                    /* Read in the partial hyperslab */
                    if (H5F_istore_read(f, dxpl_id, layout, pline, fill, coords,
                                 hslab_size, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked read failed");
                    }

                    /* Increment the buffer offset */
                    buf=(unsigned char *)buf+(partial_size*elmt_size);

                    /* Decrement the length of the sequence to read */
                    seq_len-=partial_size;

                    /* Correct the coords array */
                    coords[i]=0;
                    coords[i-1]++;

                    /* Carry the coord array correction up the array, if the dimension is finished */
                    while(i>0 && coords[i-1]==(hssize_t)dset_dims[i-1]) {
                        i--;
                        coords[i]=0;
                        if(i>0) {
                            coords[i-1]++;
                            assert(coords[i-1]<=(hssize_t)dset_dims[i-1]);
                        } /* end if */
                    } /* end while */
                } /* end if */
            } /* end for */
#ifdef QAK
printf("%s: after reading initial partial hyperslabs, seq_len=%lu\n",FUNC,(unsigned long)seq_len);
#endif /* QAK */

            /* Check if there is more than just a partial hyperslab to read */
            if(seq_len>=down_size[0]) {
                hsize_t tmp_seq_len;    /* Temp. size of the sequence in elements */
                hsize_t full_size;      /* Size of the full hyperslab in bytes */

                /* Get the sequence length for computing the hyperslab sizes */
                tmp_seq_len=seq_len;

                /* Reset the size of the hyperslab read in */
                full_size=1;

                /* Compute the hyperslab size from the length given */
                for(i=ndims-1; i>=0; i--) {
                    /* Check if the hyperslab is wider than the width of the dimension */
                    if(tmp_seq_len>dset_dims[i]) {
                        assert(0==coords[i]);
                        hslab_size[i]=dset_dims[i];
                    } /* end if */
                    else 
                        hslab_size[i]=tmp_seq_len;

                    /* compute the number of elements read in */
                    full_size*=hslab_size[i];

                    /* Fold the length into the length in the next highest dimension */
                    tmp_seq_len/=dset_dims[i];
#ifdef QAK
printf("%s: tmp_seq_len=%lu, hslab_size[%d]=%ld\n",FUNC,(unsigned long)tmp_seq_len,i,(long)hslab_size[i]);
#endif /* QAK */

                    /* Make certain the hyperslab sizes don't go less than 1 for dimensions less than 0*/
                    assert(tmp_seq_len>=1 || i==0);
                } /* end for */
                hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */

#ifdef QAK
/* Print out the file offsets & hyperslab sizes */
{
    static int count=0;

    if(count<1000000) {
        printf("%s: elmt_size=%d, addr=%d, full_size=%ld, tmp_seq_len=%ld seq_len=%ld\n",FUNC,(int)elmt_size,(int)addr,(long)full_size,(long)tmp_seq_len,(long)seq_len);
        for(i=0; i<ndims; i++)
            printf("%s: dset_dims[%d]=%d\n",FUNC,i,(int)dset_dims[i]);
        for(i=0; i<=ndims; i++)
            printf("%s: coords[%d]=%d, hslab_size[%d]=%d\n",FUNC,i,(int)coords[i],(int)i,(int)hslab_size[i]);
        count++;
    }
}
#endif /* QAK */

                /* Read the full hyperslab in */
                if (H5F_istore_read(f, dxpl_id, layout, pline, fill, coords,
                             hslab_size, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked read failed");
                }

                /* Increment the buffer offset */
                buf=(unsigned char *)buf+(full_size*elmt_size);

                /* Decrement the sequence length left */
                seq_len-=full_size;

                /* Increment coordinate of slowest changing dimension */
                coords[0]+=hslab_size[0];

            } /* end if */
#ifdef QAK
printf("%s: after reading 'middle' full hyperslabs, seq_len=%lu\n",FUNC,(unsigned long)seq_len);
#endif /* QAK */

            /*
             * Peel off final partial hyperslabs until we've finished reading all the data
             */
            if(seq_len>0) {
                hsize_t partial_size;       /* Size of the partial hyperslab in bytes */

                /*
                 * Peel off remaining partial hyperslabs, from the next-slowest dimension
                 *  on down to the next-to-fastest changing dimension
                 */
                for(i=1; i<(ndims-1); i++) {
                    /* Check if there are enough elements to read in a row in this dimension */
                    if(seq_len>=down_size[i]) {
#ifdef QAK
printf("%s: seq_len=%ld, down_size[%d]=%ld\n",FUNC,(long)seq_len,i+1,(long)down_size[i+1]);
#endif /* QAK */
                        /* Reset the partial hyperslab size */
                        partial_size=1;

                        /* Build the partial hyperslab information */
                        for(j=0; j<ndims; j++) {
                            if(j<i)
                                hslab_size[j]=1;
                            else
                                if(j==i)
                                    hslab_size[j]=seq_len/down_size[j];
                                else
                                    hslab_size[j]=dset_dims[j];

                            partial_size*=hslab_size[j];
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,j,(long)coords[j],j,(long)hslab_size[j]);
#endif /* QAK */
                        } /* end for */
                        hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,ndims,(long)coords[ndims],ndims,(long)hslab_size[ndims]);
#endif /* QAK */

                        /* Read in the partial hyperslab */
                        if (H5F_istore_read(f, dxpl_id, layout, pline, fill, coords,
                                     hslab_size, buf)<0) {
                            HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked read failed");
                        }

                        /* Increment the buffer offset */
                        buf=(unsigned char *)buf+(partial_size*elmt_size);

                        /* Decrement the length of the sequence to read */
                        seq_len-=partial_size;

                        /* Correct the coords array */
                        coords[i]=hslab_size[i];
                    } /* end if */
                } /* end for */
#ifdef QAK
printf("%s: after reading trailing hyperslabs for all but the last dimension, seq_len=%ld\n",FUNC,(long)seq_len);
#endif /* QAK */

                /* Handle fastest changing dimension if there are any elements left */
                if(seq_len>0) {
#ifdef QAK
printf("%s: i=%d, seq_len=%ld\n",FUNC,ndims-1,(long)seq_len);
#endif /* QAK */
                    assert(seq_len<dset_dims[ndims-1]);

                    /* Reset the partial hyperslab size */
                    partial_size=1;

                    /* Build the partial hyperslab information */
                    for(j=0; j<ndims; j++) {
                        if(j==(ndims-1))
                            hslab_size[j]=seq_len;
                        else
                            hslab_size[j]=1;

                        partial_size*=hslab_size[j];
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,j,(long)coords[j],j,(long)hslab_size[j]);
#endif /* QAK */
                    } /* end for */
                    hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,ndims,(long)coords[ndims],ndims,(long)hslab_size[ndims]);
#endif /* QAK */

                    /* Read in the partial hyperslab */
                    if (H5F_istore_read(f, dxpl_id, layout, pline, fill, coords,
                                 hslab_size, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked read failed");
                    }

                    /* Double-check the amount read in */
                    assert(seq_len==partial_size);
                } /* end if */
            } /* end if */
        }
            break;

        default:
            assert("not implemented yet" && 0);
            HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout");
    }   /* end switch() */

    FUNC_LEAVE(SUCCEED);
}   /* H5F_seq_read() */


/*-------------------------------------------------------------------------
 * Function:    H5F_seq_write
 *
 * Purpose:     Writes a sequence of bytes to a file dataset from a buffer in
 *      in memory.  The data is written to file F and the array's size and
 *      storage information is in LAYOUT.  External files are described
 *      according to the external file list, EFL.  The sequence offset is 
 *      FILE_OFFSET in the file (offsets are
 *      in terms of bytes) and the size of the hyperslab is SEQ_LEN. The
 *              total size of the file array is implied in the LAYOUT argument.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, October 9, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_seq_write(H5F_t *f, hid_t dxpl_id, const struct H5O_layout_t *layout,
             const struct H5O_pline_t *pline, const H5O_fill_t *fill,
             const struct H5O_efl_t *efl, const H5S_t *file_space, size_t elmt_size,
         hsize_t seq_len, hsize_t file_offset, const void *buf)
{
    hsize_t     dset_dims[H5O_LAYOUT_NDIMS];    /* dataspace dimensions */
    hssize_t    coords[H5O_LAYOUT_NDIMS];       /* offset of hyperslab in dataspace */
    hsize_t     hslab_size[H5O_LAYOUT_NDIMS];   /* hyperslab size in dataspace*/
    hsize_t     down_size[H5O_LAYOUT_NDIMS];    /* Cumulative hyperslab sizes (in elements) */
    hsize_t     acc;            /* Accumulator for hyperslab sizes (in elements) */
    int ndims;
    hsize_t     max_data = 0;                   /*bytes in dataset      */
    haddr_t     addr;                           /*address in file       */
    unsigned    u;                              /*counters              */
    int i,j;                            /*counters              */
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t xfer_mode=H5FD_MPIO_INDEPENDENT;
#endif
   
    FUNC_ENTER(H5F_seq_write, FAIL);

    /* Check args */
    assert(f);
    assert(layout);
    assert(buf);

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

    /* Collective MPIO access is unsupported for non-contiguous datasets */
    if (H5D_CONTIGUOUS!=layout->type && H5FD_MPIO_COLLECTIVE==xfer_mode) {
        HRETURN_ERROR (H5E_DATASET, H5E_WRITEERROR, FAIL,
           "collective access on non-contiguous datasets not supported yet");
    }
#endif

    switch (layout->type) {
        case H5D_CONTIGUOUS:
            /* Filters cannot be used for contiguous data. */
            if (pline && pline->nfilters>0) {
                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                      "filters are not allowed for contiguous data");
            }
            
            /*
             * Initialize loop variables.  The loop is a multi-dimensional loop
             * that counts from SIZE down to zero and IDX is the counter.  Each
             * element of IDX is treated as a digit with IDX[0] being the least
             * significant digit.
             */
            if (efl && efl->nused>0) {
                addr = 0;
            } else {
                addr = layout->addr;

                /* Compute the size of the dataset in bytes */
                for(u=0, max_data=1; u<layout->ndims; u++)
                    max_data *= layout->dim[u];

                /* Adjust the maximum size of the data by the offset into it */
                max_data -= file_offset;
            }
            addr += file_offset;

            /*
             * Now begin to walk through the array, copying data from disk to
             * memory.
             */
#ifdef H5_HAVE_PARALLEL
            if (H5FD_MPIO_COLLECTIVE==xfer_mode) {
                /*
                 * Currently supports same number of collective access. Need to
                 * be changed LATER to combine all reads into one collective MPIO
                 * call.
                 */
                unsigned long max, min, temp;

                temp = seq_len;
                assert(temp==seq_len);  /* verify no overflow */
                MPI_Allreduce(&temp, &max, 1, MPI_UNSIGNED_LONG, MPI_MAX,
                      H5FD_mpio_communicator(f->shared->lf));
                MPI_Allreduce(&temp, &min, 1, MPI_UNSIGNED_LONG, MPI_MIN,
                      H5FD_mpio_communicator(f->shared->lf));
#ifdef AKC
                printf("seq_len=%lu, min=%lu, max=%lu\n", temp, min, max);
#endif
                if (max != min)
                    HRETURN_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL,
                      "collective access with unequal number of blocks not supported yet");
            }
#endif

            /* Write directly to file if the dataset is in an external file */
            /* Note: We can't use data sieve buffers for datasets in external files
             *  because the 'addr' of all external files is set to 0 (above) and
             *  all datasets in external files would alias to the same set of
             *  file offsets, totally mixing up the data sieve buffer information. -QAK
             */
            if (efl && efl->nused>0) {
                if (H5O_efl_write(f, efl, addr, seq_len, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                          "external data write failed");
                }
            } else {
                if (H5F_contig_write(f, max_data, H5FD_MEM_DRAW, addr, seq_len, dxpl_id, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                              "block write failed");
                }
            } /* end else */
            break;

        case H5D_CHUNKED:
        {
            unsigned       leading_partials;       /* Flag set if there are leading partial hyperslabs to take care of */

            /*
             * This method is unable to access external raw data files 
             */
            if (efl && efl->nused>0) {
                HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL,
                      "chunking and external files are mutually exclusive");
            }
            /* Compute the file offset coordinates and hyperslab size */
            if((ndims=H5S_get_simple_extent_dims(file_space,dset_dims,NULL))<0)
                HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unable to retrieve dataspace dimensions");
            
#ifdef QAK
/* Print out the file offsets & hyperslab sizes */
{
    static int count=0;

    if(count<1000000) {
        printf("%s: elmt_size=%d, addr=%d, seq_len=%lu\n",FUNC,(int)elmt_size,(int)addr,(unsigned long)seq_len);
        printf("%s: file_offset=%d\n",FUNC,(int)file_offset);
        count++;
    }
}
#endif /* QAK */
#ifdef QAK
            /* The library shouldn't be reading partial elements currently */
            assert((seq_len%elmt_size)!=0);
            assert((addr%elmt_size)!=0);
#endif /* QAK */

            /* Set location in dataset from the file_offset */
            addr=file_offset;

            /* Convert the bytes into elements */
            seq_len/=elmt_size;
            addr/=elmt_size;

            /* Build the array of cumulative hyperslab sizes */
            for(acc=1, i=(ndims-1); i>=0; i--) {
                down_size[i]=acc;
                acc*=dset_dims[i];
#ifdef QAK
printf("%s: acc=%ld, down_size[%d]=%ld\n",FUNC,(long)acc,i,(long)down_size[i]);
#endif /* QAK */
            } /* end for */

            /* Compute the hyperslab offset from the address given */
            leading_partials=0;
            for(i=ndims-1; i>=0; i--) {
                coords[i]=addr%dset_dims[i];
                addr/=dset_dims[i];
                if(i>0 && coords[i]>0)
                    leading_partials=1;
#ifdef QAK
printf("%s: addr=%lu, dset_dims[%d]=%ld, coords[%d]=%ld\n",FUNC,(unsigned long)addr,i,(long)dset_dims[i],i,(long)coords[i]);
#endif /* QAK */
            } /* end for */
            coords[ndims]=0;   /* No offset for element info */
#ifdef QAK
printf("%s: addr=%lu, coords[%d]=%ld\n",FUNC,(unsigned long)addr,ndims,(long)coords[ndims]);
printf("%s: leading_partials=%u\n",FUNC,leading_partials);
#endif /* QAK */

            /*
             * Peel off initial partial hyperslabs until we've got a hyperslab which starts
             *      at coord[n]==0 for dimensions 1->(ndims-1)  (i.e. starting at coordinate
             *      zero for all dimensions except the slowest changing one
             */
            for(i=ndims-1; i>0 && seq_len>=down_size[i]; i--) {
                hsize_t partial_size;       /* Size of the partial hyperslab in bytes */

                /* Check if we have a partial hyperslab in this lower dimension */
                if(coords[i]>0) {
#ifdef QAK
printf("%s: Need to get hyperslab, seq_len=%ld, coords[%d]=%ld\n",FUNC,(long)seq_len,i,(long)coords[i]);
#endif /* QAK */
                    /* Reset the partial hyperslab size */
                    partial_size=1;

                    /* Build the partial hyperslab information */
                    for(j=0; j<ndims; j++) {
                        if(i==j)
                            hslab_size[j]=MIN(seq_len/down_size[i],dset_dims[i]-coords[i]);
                        else
                            if(j>i)
                                hslab_size[j]=dset_dims[j];
                            else
                                hslab_size[j]=1;
                        partial_size*=hslab_size[j];
#ifdef QAK
printf("%s: partial_size=%lu, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,j,(long)hslab_size[j]);
#endif /* QAK */
                    } /* end for */
                    hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */
#ifdef QAK
printf("%s: partial_size=%lu, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,ndims,(long)hslab_size[ndims]);
#endif /* QAK */

                    /* Write out the partial hyperslab */
                    if (H5F_istore_write(f, dxpl_id, layout, pline, fill, coords,
                                 hslab_size, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked write failed");
                    }

                    /* Increment the buffer offset */
                    buf=(const unsigned char *)buf+(partial_size*elmt_size);

                    /* Decrement the length of the sequence to read */
                    seq_len-=partial_size;

                    /* Correct the coords array */
                    coords[i]=0;
                    coords[i-1]++;

                    /* Carry the coord array correction up the array, if the dimension is finished */
                    while(i>0 && coords[i-1]==(hssize_t)dset_dims[i-1]) {
                        i--;
                        coords[i]=0;
                        if(i>0) {
                            coords[i-1]++;
                            assert(coords[i-1]<=(hssize_t)dset_dims[i-1]);
                        } /* end if */
                    } /* end while */
                } /* end if */
            } /* end for */
#ifdef QAK
printf("%s: seq_len=%lu\n",FUNC,(unsigned long)seq_len);
#endif /* QAK */

            /* Check if there is more than just a partial hyperslab to read */
            if(seq_len>=down_size[0]) {
                hsize_t tmp_seq_len;    /* Temp. size of the sequence in elements */
                hsize_t full_size;      /* Size of the full hyperslab in bytes */

                /* Get the sequence length for computing the hyperslab sizes */
                tmp_seq_len=seq_len;

                /* Reset the size of the hyperslab read in */
                full_size=1;

                /* Compute the hyperslab size from the length given */
                for(i=ndims-1; i>=0; i--) {
                    /* Check if the hyperslab is wider than the width of the dimension */
                    if(tmp_seq_len>dset_dims[i]) {
                        assert(0==coords[i]);
                        hslab_size[i]=dset_dims[i];
                    } /* end if */
                    else 
                        hslab_size[i]=tmp_seq_len;

                    /* compute the number of elements read in */
                    full_size*=hslab_size[i];

                    /* Fold the length into the length in the next highest dimension */
                    tmp_seq_len/=dset_dims[i];
#ifdef QAK
printf("%s: tmp_seq_len=%lu, hslab_size[%d]=%ld\n",FUNC,(unsigned long)tmp_seq_len,i,(long)hslab_size[i]);
#endif /* QAK */

                    /* Make certain the hyperslab sizes don't go less than 1 for dimensions less than 0*/
                    assert(tmp_seq_len>=1 || i==0);
                } /* end for */
                hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */

#ifdef QAK
/* Print out the file offsets & hyperslab sizes */
{
    static int count=0;

    if(count<1000000) {
        printf("%s: elmt_size=%d, addr=%d, full_size=%ld, tmp_seq_len=%ld seq_len=%ld\n",FUNC,(int)elmt_size,(int)addr,(long)full_size,(long)tmp_seq_len,(long)seq_len);
        for(i=0; i<ndims; i++)
            printf("%s: dset_dims[%d]=%d\n",FUNC,i,(int)dset_dims[i]);
        for(i=0; i<=ndims; i++)
            printf("%s: coords[%d]=%d, hslab_size[%d]=%d\n",FUNC,i,(int)coords[i],(int)i,(int)hslab_size[i]);
        count++;
    }
}
#endif /* QAK */

                /* Write the full hyperslab in */
                if (H5F_istore_write(f, dxpl_id, layout, pline, fill, coords,
                             hslab_size, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked write failed");
                }

                /* Increment the buffer offset */
                buf=(const unsigned char *)buf+(full_size*elmt_size);

                /* Decrement the sequence length left */
                seq_len-=full_size;

                /* Increment coordinate of slowest changing dimension */
                coords[0]+=hslab_size[0];

            } /* end if */
#ifdef QAK
printf("%s: seq_len=%lu\n",FUNC,(unsigned long)seq_len);
#endif /* QAK */

            /*
             * Peel off final partial hyperslabs until we've finished reading all the data
             */
            if(seq_len>0) {
                hsize_t partial_size;       /* Size of the partial hyperslab in bytes */

                /*
                 * Peel off remaining partial hyperslabs, from the next-slowest dimension
                 *  on down to the next-to-fastest changing dimension
                 */
                for(i=1; i<(ndims-1); i++) {
                    /* Check if there are enough elements to read in a row in this dimension */
                    if(seq_len>=down_size[i]) {
#ifdef QAK
printf("%s: seq_len=%ld, down_size[%d]=%ld\n",FUNC,(long)seq_len,i+1,(long)down_size[i+1]);
#endif /* QAK */
                        /* Reset the partial hyperslab size */
                        partial_size=1;

                        /* Build the partial hyperslab information */
                        for(j=0; j<ndims; j++) {
                            if(j<i)
                                hslab_size[j]=1;
                            else
                                if(j==i)
                                    hslab_size[j]=seq_len/down_size[j];
                                else
                                    hslab_size[j]=dset_dims[j];

                            partial_size*=hslab_size[j];
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,j,(long)coords[j],j,(long)hslab_size[j]);
#endif /* QAK */
                        } /* end for */
                        hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,ndims,(long)coords[ndims],ndims,(long)hslab_size[ndims]);
#endif /* QAK */

                        /* Write out the partial hyperslab */
                        if (H5F_istore_write(f, dxpl_id, layout, pline, fill, coords,
                                     hslab_size, buf)<0) {
                            HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked write failed");
                        }

                        /* Increment the buffer offset */
                        buf=(const unsigned char *)buf+(partial_size*elmt_size);

                        /* Decrement the length of the sequence to read */
                        seq_len-=partial_size;

                        /* Correct the coords array */
                        coords[i]=hslab_size[i];
                    } /* end if */
                } /* end for */

                /* Handle fastest changing dimension if there are any elements left */
                if(seq_len>0) {
#ifdef QAK
printf("%s: i=%d, seq_len=%ld\n",FUNC,ndims-1,(long)seq_len);
#endif /* QAK */
                    assert(seq_len<dset_dims[ndims-1]);

                    /* Reset the partial hyperslab size */
                    partial_size=1;

                    /* Build the partial hyperslab information */
                    for(j=0; j<ndims; j++) {
                        if(j==(ndims-1))
                            hslab_size[j]=seq_len;
                        else
                            hslab_size[j]=1;

                        partial_size*=hslab_size[j];
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,j,(long)coords[j],j,(long)hslab_size[j]);
#endif /* QAK */
                    } /* end for */
                    hslab_size[ndims]=elmt_size;   /* basic hyperslab size is the element */
#ifdef QAK
printf("%s: partial_size=%lu, coords[%d]=%ld, hslab_size[%d]=%ld\n",FUNC,(unsigned long)partial_size,ndims,(long)coords[ndims],ndims,(long)hslab_size[ndims]);
#endif /* QAK */

                    /* Write in the final partial hyperslab */
                    if (H5F_istore_write(f, dxpl_id, layout, pline, fill, coords,
                                 hslab_size, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL, "chunked write failed");
                    }

                    /* Double-check the amount read in */
                    assert(seq_len==partial_size);
                } /* end if */
            } /* end if */
        }
            break;

        default:
            assert("not implemented yet" && 0);
            HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout");
    }   /* end switch() */

    FUNC_LEAVE(SUCCEED);
}   /* H5F_seq_write() */

