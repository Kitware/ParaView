/*
 * Copyright (C) 2000 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Thursday, September 28, 2000
 *
 * Purpose:     Contiguous dataset I/O functions.  These routines are similar
 *      to the H5F_istore_* routines and really only abstract away dealing
 *      with the data sieve buffer from the H5F_arr_read/write and
 *      H5F_seg_read/write.
 *
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5FDprivate.h"        /*file driver                             */
#include "H5MMprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5Fcontig_mask
static int              interface_initialize_g = 0;
#define INTERFACE_INIT NULL


/*-------------------------------------------------------------------------
 * Function:    H5F_contig_read
 *
 * Purpose:     Reads some data from a dataset into a buffer.
 *              The data is contiguous.  The address is relative to the base
 *              address for the file.
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
H5F_contig_read(H5F_t *f, hsize_t max_data, H5FD_mem_t type, haddr_t addr, hsize_t size, hid_t dxpl_id,
               void *buf/*out*/)
{
    haddr_t abs_eoa;                    /* Absolute end of file address         */
    haddr_t rel_eoa;                    /* Relative end of file address         */
   
    FUNC_ENTER(H5F_contig_read, FAIL);

    /* Check args */
    assert(f);
    assert(size<SIZET_MAX);
    assert(buf);

    /* Check if data sieving is enabled */
    if(f->shared->lf->feature_flags&H5FD_FEAT_DATA_SIEVE) {
        /* Try reading from the data sieve buffer */
        if(f->shared->sieve_buf) {
            haddr_t sieve_start, sieve_end;     /* Start & end locations of sieve buffer */
            haddr_t contig_end;         /* End locations of block to write */
            hsize_t sieve_size;                 /* size of sieve buffer */

            /* Stash local copies of these value */
            sieve_start=f->shared->sieve_loc;
            sieve_size=f->shared->sieve_size;
            sieve_end=sieve_start+sieve_size;
            contig_end=addr+size-1;
            
            /* If entire read is within the sieve buffer, read it from the buffer */
            if(addr>=sieve_start && contig_end<sieve_end) {
                /* Grab the data out of the buffer */
                assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                HDmemcpy(buf,f->shared->sieve_buf+(addr-sieve_start),(size_t)size);
            } /* end if */
            /* Entire request is not within this data sieve buffer */
            else {
                /* Check if we can actually hold the I/O request in the sieve buffer */
                if(size>f->shared->sieve_buf_size) {
                    /* Check for any overlap with the current sieve buffer */
                    if((sieve_start>=addr && sieve_start<(contig_end+1))
                            || ((sieve_end-1)>=addr && (sieve_end-1)<(contig_end+1))) {
                        /* Flush the sieve buffer, if it's dirty */
                        if(f->shared->sieve_dirty) {
                            /* Write to file */
                            if (H5F_block_write(f, H5FD_MEM_DRAW, sieve_start, sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                                  "block write failed");
                            }

                            /* Reset sieve buffer dirty flag */
                            f->shared->sieve_dirty=0;
                        } /* end if */
                    } /* end if */

                    /* Read directly into the user's buffer */
                    if (H5F_block_read(f, type, addr, size, dxpl_id, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                                  "block read failed");
                    }
                } /* end if */
                /* Element size fits within the buffer size */
                else {
                    /* Flush the sieve buffer if it's dirty */
                    if(f->shared->sieve_dirty) {
                        /* Write to file */
                        if (H5F_block_write(f, H5FD_MEM_DRAW, sieve_start, sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                              "block write failed");
                        }

                        /* Reset sieve buffer dirty flag */
                        f->shared->sieve_dirty=0;
                    } /* end if */

                    /* Determine the new sieve buffer size & location */
                    f->shared->sieve_loc=addr;

                    /* Make certain we don't read off the end of the file */
                    if (HADDR_UNDEF==(abs_eoa=H5FD_get_eoa(f->shared->lf))) {
                        HRETURN_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL,
                            "unable to determine file size");
                    }

                    /* Adjust absolute EOA address to relative EOA address */
                    rel_eoa=abs_eoa-f->shared->base_addr;

                    /* Compute the size of the sieve buffer */
                    f->shared->sieve_size=MIN(rel_eoa-addr,MIN(max_data,f->shared->sieve_buf_size));

                    /* Read the new sieve buffer */
                    if (H5F_block_read(f, type, f->shared->sieve_loc, f->shared->sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                                  "block read failed");
                    }

                    /* Reset sieve buffer dirty flag */
                    f->shared->sieve_dirty=0;

                    /* Grab the data out of the buffer (must be first piece of data in buffer ) */
                    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                    HDmemcpy(buf,f->shared->sieve_buf,(size_t)size);
                } /* end else */
            } /* end else */
        } /* end if */
        /* No data sieve buffer yet, go allocate one */
        else {
            /* Check if we can actually hold the I/O request in the sieve buffer */
            if(size>f->shared->sieve_buf_size) {
                if (H5F_block_read(f, type, addr, size, dxpl_id, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                              "block read failed");
                }
            } /* end if */
            else {
                /* Allocate room for the data sieve buffer */
                assert(f->shared->sieve_buf_size==(hsize_t)((size_t)f->shared->sieve_buf_size)); /*check for overflow*/
                if (NULL==(f->shared->sieve_buf=H5MM_malloc((size_t)f->shared->sieve_buf_size))) {
                    HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                          "memory allocation failed");
                }

                /* Determine the new sieve buffer size & location */
                f->shared->sieve_loc=addr;

                /* Make certain we don't read off the end of the file */
                if (HADDR_UNDEF==(abs_eoa=H5FD_get_eoa(f->shared->lf))) {
                    HRETURN_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL,
                        "unable to determine file size");
                }

                /* Adjust absolute EOA address to relative EOA address */
                rel_eoa=abs_eoa-f->shared->base_addr;

                /* Compute the size of the sieve buffer */
                f->shared->sieve_size=MIN(rel_eoa-addr,MIN(max_data,f->shared->sieve_buf_size));

                /* Read the new sieve buffer */
                if (H5F_block_read(f, type, f->shared->sieve_loc, f->shared->sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                              "block read failed");
                }

                /* Reset sieve buffer dirty flag */
                f->shared->sieve_dirty=0;

                /* Grab the data out of the buffer (must be first piece of data in buffer ) */
                assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                HDmemcpy(buf,f->shared->sieve_buf,(size_t)size);
            } /* end else */
        } /* end else */
    } /* end if */
    else {
        if (H5F_block_read(f, type, addr, size, dxpl_id, buf)<0) {
            HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                      "block read failed");
        }
    } /* end else */

    FUNC_LEAVE(SUCCEED);
}   /* End H5F_contig_read() */


/*-------------------------------------------------------------------------
 * Function:    H5F_contig_write
 *
 * Purpose:     Writes some data from a dataset into a buffer.
 *              The data is contiguous.  The address is relative to the base
 *              address for the file.
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
H5F_contig_write(H5F_t *f, hsize_t max_data, H5FD_mem_t type, haddr_t addr, hsize_t size,
        hid_t dxpl_id, const void *buf)
{
    haddr_t abs_eoa;                    /* Absolute end of file address         */
    haddr_t rel_eoa;                    /* Relative end of file address         */

    FUNC_ENTER(H5F_contig_write, FAIL);

    assert (f);
    assert (size<SIZET_MAX);
    assert (buf);

    /* Check if data sieving is enabled */
    if(f->shared->lf->feature_flags&H5FD_FEAT_DATA_SIEVE) {
        /* Try writing to the data sieve buffer */
        if(f->shared->sieve_buf) {
            haddr_t sieve_start, sieve_end;     /* Start & end locations of sieve buffer */
            haddr_t contig_end;         /* End locations of block to write */
            hsize_t sieve_size;                 /* size of sieve buffer */

            /* Stash local copies of these value */
            sieve_start=f->shared->sieve_loc;
            sieve_size=f->shared->sieve_size;
            sieve_end=sieve_start+sieve_size;
            contig_end=addr+size-1;
            
            /* If entire write is within the sieve buffer, write it to the buffer */
            if(addr>=sieve_start && contig_end<sieve_end) {
                /* Grab the data out of the buffer */
                assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                HDmemcpy(f->shared->sieve_buf+(addr-sieve_start),buf,(size_t)size);

                /* Set sieve buffer dirty flag */
                f->shared->sieve_dirty=1;

            } /* end if */
            /* Entire request is not within this data sieve buffer */
            else {
                /* Check if we can actually hold the I/O request in the sieve buffer */
                if(size>f->shared->sieve_buf_size) {
                    /* Check for any overlap with the current sieve buffer */
                    if((sieve_start>=addr && sieve_start<(contig_end+1))
                            || ((sieve_end-1)>=addr && (sieve_end-1)<(contig_end+1))) {
                        /* Flush the sieve buffer, if it's dirty */
                        if(f->shared->sieve_dirty) {
                            /* Write to file */
                            if (H5F_block_write(f, H5FD_MEM_DRAW, sieve_start, sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                                  "block write failed");
                            }

                            /* Reset sieve buffer dirty flag */
                            f->shared->sieve_dirty=0;
                        } /* end if */

                        /* Force the sieve buffer to be re-read the next time */
                        f->shared->sieve_loc=HADDR_UNDEF;
                        f->shared->sieve_size=0;
                    } /* end if */

                    /* Write directly to the user's buffer */
                    if (H5F_block_write(f, type, addr, size, dxpl_id, buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                                  "block write failed");
                    }
                } /* end if */
                /* Element size fits within the buffer size */
                else {
                    /* Check if it is possible to (exactly) prepend or append to existing (dirty) sieve buffer */
                    if(((addr+size)==sieve_start || addr==sieve_end) &&
                            (size+sieve_size)<=f->shared->sieve_buf_size &&
                            f->shared->sieve_dirty) {
                        /* Prepend to existing sieve buffer */
                        if((addr+size)==sieve_start) {
                            /* Move existing sieve information to correct location */
                            assert(sieve_size==(hsize_t)((size_t)sieve_size)); /*check for overflow*/
                            HDmemmove(f->shared->sieve_buf+size,f->shared->sieve_buf,(size_t)sieve_size);

                            /* Copy in new information (must be first in sieve buffer) */
                            assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                            HDmemcpy(f->shared->sieve_buf,buf,(size_t)size);

                            /* Adjust sieve location */
                            f->shared->sieve_loc=addr;
                            
                        } /* end if */
                        /* Append to existing sieve buffer */
                        else {
                            /* Copy in new information */
                            assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                            HDmemcpy(f->shared->sieve_buf+sieve_size,buf,(size_t)size);
                        } /* end else */

                        /* Adjust sieve size */
                        f->shared->sieve_size += size;
                        
                    } /* end if */
                    /* Can't add the new data onto the existing sieve buffer */
                    else {
                        /* Flush the sieve buffer if it's dirty */
                        if(f->shared->sieve_dirty) {
                            /* Write to file */
                            if (H5F_block_write(f, H5FD_MEM_DRAW, sieve_start, sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                                HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                                  "block write failed");
                            }

                            /* Reset sieve buffer dirty flag */
                            f->shared->sieve_dirty=0;
                        } /* end if */

                        /* Determine the new sieve buffer size & location */
                        f->shared->sieve_loc=addr;

                        /* Make certain we don't read off the end of the file */
                        if (HADDR_UNDEF==(abs_eoa=H5FD_get_eoa(f->shared->lf))) {
                            HRETURN_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL,
                                "unable to determine file size");
                        }

                        /* Adjust absolute EOA address to relative EOA address */
                        rel_eoa=abs_eoa-f->shared->base_addr;

                        /* Compute the size of the sieve buffer */
                        f->shared->sieve_size=MIN(rel_eoa-addr,MIN(max_data,f->shared->sieve_buf_size));

                        /* Check if there is any point in reading the data from the file */
                        if(f->shared->sieve_size>size) {
                            /* Read the new sieve buffer */
                            if (H5F_block_read(f, type, f->shared->sieve_loc, f->shared->sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                                HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                                          "block read failed");
                            }
                        } /* end if */

                        /* Grab the data out of the buffer (must be first piece of data in buffer) */
                        assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                        HDmemcpy(f->shared->sieve_buf,buf,(size_t)size);

                        /* Set sieve buffer dirty flag */
                        f->shared->sieve_dirty=1;

                    } /* end else */
                } /* end else */
            } /* end else */
        } /* end if */
        /* No data sieve buffer yet, go allocate one */
        else {
            /* Check if we can actually hold the I/O request in the sieve buffer */
            if(size>f->shared->sieve_buf_size) {
                if (H5F_block_write(f, type, addr, size, dxpl_id, buf)<0) {
                    HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                              "block write failed");
                }
            } /* end if */
            else {
                /* Allocate room for the data sieve buffer */
                assert(f->shared->sieve_buf_size==(hsize_t)((size_t)f->shared->sieve_buf_size)); /*check for overflow*/
                if (NULL==(f->shared->sieve_buf=H5MM_malloc((size_t)f->shared->sieve_buf_size))) {
                    HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                          "memory allocation failed");
                }

                /* Determine the new sieve buffer size & location */
                f->shared->sieve_loc=addr;

                /* Make certain we don't read off the end of the file */
                if (HADDR_UNDEF==(abs_eoa=H5FD_get_eoa(f->shared->lf))) {
                    HRETURN_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL,
                        "unable to determine file size");
                }

                /* Adjust absolute EOA address to relative EOA address */
                rel_eoa=abs_eoa-f->shared->base_addr;

                /* Compute the size of the sieve buffer */
                f->shared->sieve_size=MIN(rel_eoa-addr,MIN(max_data,f->shared->sieve_buf_size));

                /* Check if there is any point in reading the data from the file */
                if(f->shared->sieve_size>size) {
                    /* Read the new sieve buffer */
                    if (H5F_block_read(f, type, f->shared->sieve_loc, f->shared->sieve_size, dxpl_id, f->shared->sieve_buf)<0) {
                        HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                                  "block read failed");
                    }
                } /* end if */

                /* Grab the data out of the buffer (must be first piece of data in buffer) */
                assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
                HDmemcpy(f->shared->sieve_buf,buf,(size_t)size);

                /* Set sieve buffer dirty flag */
                f->shared->sieve_dirty=1;
            } /* end else */
        } /* end else */
    } /* end if */
    else {
        if (H5F_block_write(f, type, addr, size, dxpl_id, buf)<0) {
            HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                      "block write failed");
        }
    } /* end else */

    FUNC_LEAVE(SUCCEED);
}   /* End H5F_contig_write() */
