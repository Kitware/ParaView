/*
 * Copyright (C) 1998-2001 NCSA
 *                         All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Thursday, June 18, 1998
 *
 * Purpose:     Hyperslab selection data space I/O functions.
 */

#define H5S_PACKAGE             /*suppress error about including H5Spkg   */

#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5Fprivate.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5Iprivate.h"
#include "H5MMprivate.h"
#include "H5Pprivate.h"
#include "H5Spkg.h"
#include "H5Vprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5Shyper_mask
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

/* Local datatypes */
/* Parameter block for H5S_hyper_fread, H5S_hyper_fwrite, H5S_hyper_mread & H5S_hyper_mwrite */
typedef struct {
    H5F_t *f;
    const struct H5O_layout_t *layout;
    const struct H5O_pline_t *pline;
    const struct H5O_fill_t *fill;
    const struct H5O_efl_t *efl;
    size_t elmt_size;
    const H5S_t *space;
    H5S_sel_iter_t *iter;
        hsize_t nelmts;
    hid_t dxpl_id;
    const void *src;
    void *dst;
    hsize_t     mem_size[H5O_LAYOUT_NDIMS];
    hssize_t offset[H5O_LAYOUT_NDIMS];
    hsize_t     hsize[H5O_LAYOUT_NDIMS];
} H5S_hyper_io_info_t;

/* Parameter block for H5S_hyper_select_iter_mem */
typedef struct {
    hid_t dt;
    size_t elem_size;
    const H5S_t *space;
    H5S_sel_iter_t *iter;
    void *src;
    hsize_t     mem_size[H5O_LAYOUT_NDIMS];
    hssize_t mem_offset[H5O_LAYOUT_NDIMS];
    H5D_operator_t op;
    void * op_data;
} H5S_hyper_iter_info_t;

/* Static function prototypes */
static H5S_hyper_region_t * H5S_hyper_get_regions (size_t *num_regions,
               unsigned rank, unsigned dim, size_t bound_count,
               H5S_hyper_bound_t **lo_bounds, hssize_t *pos, hssize_t *offset);
static hsize_t H5S_hyper_fread (int dim, H5S_hyper_io_info_t *io_info);
static hsize_t H5S_hyper_fread_opt (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline, const struct H5O_fill_t *fill,
                 const struct H5O_efl_t *efl, size_t elmt_size,
                 const H5S_t *file_space, H5S_sel_iter_t *file_iter,
                 hsize_t nelmts, hid_t dxpl_id, void *_buf/*out*/);
static hsize_t H5S_hyper_fwrite (int dim,
                                H5S_hyper_io_info_t *io_info);
static hsize_t H5S_hyper_fwrite_opt (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline, const struct H5O_fill_t *fill,
                 const struct H5O_efl_t *efl, size_t elmt_size,
                 const H5S_t *file_space, H5S_sel_iter_t *file_iter,
                 hsize_t nelmts, hid_t dxpl_id, const void *_buf);
static herr_t H5S_hyper_init (const struct H5O_layout_t *layout,
                              const H5S_t *space, H5S_sel_iter_t *iter);
static hsize_t H5S_hyper_favail (const H5S_t *space, const H5S_sel_iter_t *iter,
                                hsize_t max);
static hsize_t H5S_hyper_fgath (H5F_t *f, const struct H5O_layout_t *layout,
                               const struct H5O_pline_t *pline,
                               const struct H5O_fill_t *fill,
                               const struct H5O_efl_t *efl, size_t elmt_size,
                               const H5S_t *file_space,
                               H5S_sel_iter_t *file_iter, hsize_t nelmts,
                               hid_t dxpl_id, void *buf/*out*/);
static herr_t H5S_hyper_fscat (H5F_t *f, const struct H5O_layout_t *layout,
                               const struct H5O_pline_t *pline,
                               const struct H5O_fill_t *fill,
                               const struct H5O_efl_t *efl, size_t elmt_size,
                               const H5S_t *file_space,
                               H5S_sel_iter_t *file_iter, hsize_t nelmts,
                               hid_t dxpl_id, const void *buf);
static hsize_t H5S_hyper_mread (int dim, H5S_hyper_io_info_t *io_info);
static hsize_t H5S_hyper_mread_opt (const void *_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_tconv_buf/*out*/);
static hsize_t H5S_hyper_mgath (const void *_buf, size_t elmt_size,
                               const H5S_t *mem_space,
                               H5S_sel_iter_t *mem_iter, hsize_t nelmts,
                               void *_tconv_buf/*out*/);
static size_t H5S_hyper_mwrite (int dim, H5S_hyper_io_info_t *io_info);
static hsize_t H5S_hyper_mwrite_opt (const void *_tconv_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_buf/*out*/);
static herr_t H5S_hyper_mscat (const void *_tconv_buf, size_t elmt_size,
                               const H5S_t *mem_space,
                               H5S_sel_iter_t *mem_iter, hsize_t nelmts,
                               void *_buf/*out*/);

const H5S_fconv_t       H5S_HYPER_FCONV[1] = {{
    "hslab",                                    /*name                  */
    H5S_SEL_HYPERSLABS,                         /*selection type        */
    H5S_hyper_init,                             /*initialize            */
    H5S_hyper_favail,                           /*available             */
    H5S_hyper_fgath,                            /*gather                */
    H5S_hyper_fscat,                            /*scatter               */
}};

const H5S_mconv_t       H5S_HYPER_MCONV[1] = {{
    "hslab",                                    /*name                  */
    H5S_SEL_HYPERSLABS,                         /*selection type        */
    H5S_hyper_init,                             /*initialize            */
    H5S_hyper_mgath,                            /*gather                */
    H5S_hyper_mscat,                            /*scatter               */
}};

/* Array for use with I/O algorithms which frequently need array of zeros */
static const hssize_t   zero[H5O_LAYOUT_NDIMS]={0};             /* Array of zeros */

/* Declare a free list to manage the H5S_hyper_node_t struct */
H5FL_DEFINE_STATIC(H5S_hyper_node_t);

/* Declare a free list to manage the H5S_hyper_list_t struct */
H5FL_DEFINE_STATIC(H5S_hyper_list_t);

/* Declare a free list to manage arrays of hsize_t */
H5FL_ARR_DEFINE_STATIC(hsize_t,H5S_MAX_RANK);

typedef H5S_hyper_bound_t *H5S_hyper_bound_ptr_t;
/* Declare a free list to manage arrays of H5S_hyper_bound_ptr_t */
H5FL_ARR_DEFINE_STATIC(H5S_hyper_bound_ptr_t,H5S_MAX_RANK);

/* Declare a free list to manage arrays of H5S_hyper_dim_t */
H5FL_ARR_DEFINE_STATIC(H5S_hyper_dim_t,H5S_MAX_RANK);

/* Declare a free list to manage arrays of H5S_hyper_bound_t */
H5FL_ARR_DEFINE_STATIC(H5S_hyper_bound_t,-1);

/* Declare a free list to manage arrays of H5S_hyper_region_t */
H5FL_ARR_DEFINE_STATIC(H5S_hyper_region_t,-1);

/* Declare a free list to manage blocks of hyperslab data */
H5FL_BLK_DEFINE_STATIC(hyper_block);


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_init
 *
 * Purpose:     Initializes iteration information for hyperslab selection.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_init (const struct H5O_layout_t UNUSED *layout,
               const H5S_t *space, H5S_sel_iter_t *sel_iter)
{
    FUNC_ENTER (H5S_hyper_init, FAIL);

    /* Check args */
    assert (space && H5S_SEL_HYPERSLABS==space->select.type);
    assert (sel_iter);

    /* Initialize the number of points to iterate over */
    sel_iter->hyp.elmt_left=space->select.num_elem;

    /* Allocate the position & initialize to invalid location */
    sel_iter->hyp.pos = H5FL_ARR_ALLOC(hsize_t,(hsize_t)space->extent.u.simple.rank,0);
    sel_iter->hyp.pos[0]=(-1);
    H5V_array_fill(sel_iter->hyp.pos, sel_iter->hyp.pos, sizeof(hssize_t),
                   space->extent.u.simple.rank);
    
    FUNC_LEAVE (SUCCEED);

    layout = 0;
}   /* H5S_hyper_init() */

/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_favail
 *
 * Purpose:     Figure out the optimal number of elements to transfer to/from the file
 *
 * Return:      non-negative number of elements on success, zero on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_favail (const H5S_t UNUSED *space,
                  const H5S_sel_iter_t *sel_iter, hsize_t max)
{
    FUNC_ENTER (H5S_hyper_favail, 0);

    /* Check args */
    assert (space && H5S_SEL_HYPERSLABS==space->select.type);
    assert (sel_iter);

#ifdef QAK
    printf("%s: sel_iter->hyp.elmt_left=%u, max=%u\n",FUNC,(unsigned)sel_iter->hyp.elmt_left,(unsigned)max);
#endif /* QAK */
    FUNC_LEAVE (MIN(sel_iter->hyp.elmt_left,max));
}   /* H5S_hyper_favail() */

/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_compare_regions
 *
 * Purpose:     Compares two regions for equality (regions must not overlap!)
 *
 * Return:      an integer less than, equal to, or greater than zero if the
 *              first region is considered to be respectively less than,
 *              equal to, or greater than the second
 *
 * Programmer:  Quincey Koziol
 *              Friday, July 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_hyper_compare_regions (const void *r1, const void *r2)
{
    if (((const H5S_hyper_region_t *)r1)->start < ((const H5S_hyper_region_t *)r2)->start)
        return(-1);
    else if (((const H5S_hyper_region_t *)r1)->start > ((const H5S_hyper_region_t *)r2)->start)
        return(1);
    else
        return(0);
}   /* end H5S_hyper_compare_regions */

/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_get_regions
 *
 * Purpose:     Builds a sorted array of the overlaps in a dimension
 *
 * Return:      Success:        Pointer to valid array (num_regions parameter
 *                              set to array size)
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Monday, June 29, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5S_hyper_region_t *
H5S_hyper_get_regions (size_t *num_regions, unsigned rank, unsigned dim,
    size_t bound_count, H5S_hyper_bound_t **lo_bounds, hssize_t *pos,
   hssize_t *offset)
{
    H5S_hyper_region_t *ret_value=NULL; /* Pointer to array of regions to return */
    H5S_hyper_region_t *reg=NULL;           /* Pointer to array of regions */
    H5S_hyper_bound_t *lo_bound_dim;    /* Pointer to the boundary nodes for a given dimension */
    H5S_hyper_node_t *node;             /* Region node for a given boundary */
    hssize_t *node_start,*node_end;     /* Extra pointers to node's start & end arrays */
    hssize_t *tmp_pos,*tmp_off;         /* Extra pointers into the position and offset arrays */
    hssize_t pos_dim,off_dim;           /* The position & offset in the dimension passed in */
    size_t num_reg=0;                   /* Number of regions in array */
    int curr_reg=-1;                   /* The current region we are working with */
    int temp_dim;                      /* Temporary dim. holder */
    size_t i;                           /* Counters */

    FUNC_ENTER (H5S_hyper_get_regions, NULL);
    
    assert(num_regions);
    assert(lo_bounds);
    assert(pos);

#ifdef QAK
    printf("%s: check 1.0, rank=%u, dim=%d\n",FUNC,rank,dim);
    for(i=0; i<rank; i++)
        printf("%s: %d - pos=%d, offset=%d\n",FUNC,i,(int)pos[i],offset!=NULL ? (int)offset[i] : 0);
#endif /* QAK */

    /* Iterate over the blocks which fit the position, or all of the blocks, if pos[dim]==-1 */
    lo_bound_dim=lo_bounds[dim];
    pos_dim=pos[dim];
    off_dim=offset[dim];
#ifdef QAK
    printf("%s: check 1.1, bound_count=%d, pos_dim=%d\n",FUNC,bound_count,(int)pos_dim);
#endif /* QAK */

    for(i=0; i<bound_count; i++,lo_bound_dim++) {
#ifdef QAK
    printf("%s: check 1.2, i=%d, num_reg=%d, curr_reg=%d\n",FUNC,(int)i,(int)num_reg,(int)curr_reg);
    printf("%s: check 1.2.1, lo_bound_dim->bound=%d\n",FUNC,(int)lo_bound_dim->bound);
#endif /* QAK */
        /* Check if each boundary overlaps in the higher dimensions */
        node=lo_bound_dim->node;
        if(pos_dim<0 || (node->end[dim]+off_dim)>=pos_dim) {
            temp_dim=(dim-1);
            if(temp_dim>=0) {
                node_start=node->start+temp_dim;
                node_end=node->end+temp_dim;
                tmp_pos=pos+temp_dim;
                tmp_off=offset+temp_dim;
                while(temp_dim>=0 && *tmp_pos>=(*node_start+*tmp_off) && *tmp_pos<=(*node_end+*tmp_off)) {
                    temp_dim--;
                    node_start--;
                    node_end--;
                    tmp_pos--;
                    tmp_off--;
                } /* end while */
            } /* end if */

#ifdef QAK
        printf("%s: check 1.3, i=%d, temp_dim=%d\n",FUNC,(int)i,(int)temp_dim);
#endif /* QAK */
            /* Yes, all previous positions match, this is a valid region */
            if(temp_dim<0) {
#ifdef QAK
        printf("%s: check 1.4, node->start[%d]=%d, node->end[%d]=%d\n",FUNC,(int)dim,(int)node->start[dim],(int)dim,(int)node->end[dim]);
#endif /* QAK */
                /* Check if we've allocated the array yet */
                if(num_reg==0) {
                    /* Allocate temporary buffer, big enough for worst case size */
                    reg=H5FL_ARR_ALLOC(H5S_hyper_region_t,(hsize_t)bound_count,0);

                    /* Initialize with first region */
                    reg[num_reg].start=MAX(node->start[dim],pos[dim])+offset[dim];
                    reg[num_reg].end=node->end[dim]+offset[dim];
                    reg[num_reg].node=node;

                    /* Increment the number of regions */
                    num_reg++;
                    curr_reg++;
                } else {
                    /* Try to merge regions together in all dimensions, except the final one */
                    if(dim<(rank-1) && (node->start[dim]+offset[dim])<=(reg[curr_reg].end+1)) {
#ifdef QAK
        printf("%s: check 1.4.1\n",FUNC);
#endif /* QAK */
                        reg[curr_reg].end=MAX(node->end[dim],reg[curr_reg].end)+offset[dim];
                    } else { /* no overlap with previous region, add new region */
#ifdef QAK
        printf("%s: check 1.4.2\n",FUNC);
#endif /* QAK */
                        /* Initialize with new region */
                        reg[num_reg].start=node->start[dim]+offset[dim];
                        reg[num_reg].end=node->end[dim]+offset[dim];
                        reg[num_reg].node=node;

                        /*
                         * Increment the number of regions & the current
                         * region.
                         */
                        num_reg++;
                        curr_reg++;
                    } /* end else */
                } /* end else */
            } /* end if */
        } /* end if */
    } /* end for */

    /* Save the number of regions we generated */
    *num_regions=num_reg;

    /* Set return value */
    ret_value=reg;

#ifdef QAK
    printf("%s: check 10.0, reg=%p, num_reg=%d\n",
           FUNC,reg,num_reg);
    for(i=0; i<num_reg; i++)
        printf("%s: start[%d]=%d, end[%d]=%d\n",
               FUNC,i,(int)reg[i].start,i,(int)reg[i].end);
#endif /* QAK */

    FUNC_LEAVE (ret_value);
} /* end H5S_hyper_get_regions() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_block_cache
 *
 * Purpose:     Cache a hyperslab block for reading or writing.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, September 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_block_cache (H5S_hyper_node_t *node,
                       H5S_hyper_io_info_t *io_info, unsigned block_read)
{
    hssize_t    file_offset[H5O_LAYOUT_NDIMS];  /*offset of slab in file*/
    hsize_t     hsize[H5O_LAYOUT_NDIMS];        /*size of hyperslab     */
    unsigned u;                   /* Counters */

    FUNC_ENTER (H5S_hyper_block_cache, SUCCEED);

    assert(node);
    assert(io_info);

    /* Allocate temporary buffer of proper size */
    if((node->cinfo.block=H5FL_BLK_ALLOC(hyper_block,(hsize_t)(node->cinfo.size*io_info->elmt_size),0))==NULL)
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
            "can't allocate hyperslab cache block");

    /* Read in block, if we are read caching */
    if(block_read) {
        /* Copy the location of the region in the file */
        HDmemcpy(file_offset,node->start,(io_info->space->extent.u.simple.rank * sizeof(hssize_t)));
        file_offset[io_info->space->extent.u.simple.rank]=0;

        /* Set the hyperslab size to read */
        for(u=0; u<io_info->space->extent.u.simple.rank; u++)
            hsize[u]=(node->end[u]-node->start[u])+1;
        hsize[io_info->space->extent.u.simple.rank]=io_info->elmt_size;

        if (H5F_arr_read(io_info->f, io_info->dxpl_id,
                         io_info->layout, io_info->pline,
                         io_info->fill, io_info->efl, hsize, hsize,
                         zero, file_offset, node->cinfo.block/*out*/)<0)
            HRETURN_ERROR (H5E_DATASPACE, H5E_READERROR, FAIL, "read error");
    } /* end if */
    else {
/* keep information for writing block later? */
    } /* end else */
    
    /* Set up parameters for accessing block (starting the read and write information at the same point) */
    node->cinfo.wleft=node->cinfo.rleft=(unsigned)node->cinfo.size;
    node->cinfo.wpos=node->cinfo.rpos=node->cinfo.block;

    /* Set cached flag */
    node->cinfo.cached=1;

    FUNC_LEAVE (SUCCEED);
}   /* H5S_hyper_block_cache() */

/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_block_read
 *
 * Purpose:     Read in data from a cached hyperslab block
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, September 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_block_read (H5S_hyper_node_t *node, H5S_hyper_io_info_t *io_info, hsize_t region_size)
{
    FUNC_ENTER (H5S_hyper_block_read, SUCCEED);

    assert(node && node->cinfo.cached);
    assert(io_info);

    /* Copy the elements into the user's buffer */
    /*
        !! NOTE !! This will need to be changed for different dimension
            permutations from the standard 'C' ordering!
    */
#ifdef QAK
        printf("%s: check 1.0, io_info->dst=%p, node->cinfo.rpos=%p, region_size=%lu, io_info->elmt_size=%lu\n",FUNC,io_info->dst,node->cinfo.rpos,(unsigned long)region_size,(unsigned long)io_info->elmt_size);
#endif /* QAK */
    HDmemcpy(io_info->dst, node->cinfo.rpos, (size_t)(region_size*io_info->elmt_size));

    /*
     * Decrement the number of elements left in block to read & move the
     * offset
     */
    node->cinfo.rpos+=region_size*io_info->elmt_size;
    node->cinfo.rleft-=region_size;

    /* If we've read in all the elements from the block, throw it away */
    if(node->cinfo.rleft==0 && (node->cinfo.wleft==0 || node->cinfo.wleft==node->cinfo.size)) {
        /* Release the temporary buffer */
        H5FL_BLK_FREE(hyper_block,node->cinfo.block);

        /* Reset the caching flag for next time */
        node->cinfo.cached=0;
    } /* end if */

    FUNC_LEAVE (SUCCEED);
}   /* H5S_hyper_block_read() */

/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_block_write
 *
 * Purpose:     Write out data to a cached hyperslab block
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, September 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_block_write (H5S_hyper_node_t *node,
                       H5S_hyper_io_info_t *io_info,
                       hsize_t region_size)
{
    hssize_t    file_offset[H5O_LAYOUT_NDIMS];  /*offset of slab in file*/
    hsize_t     hsize[H5O_LAYOUT_NDIMS];        /*size of hyperslab     */
    unsigned u;                   /* Counters */

    FUNC_ENTER (H5S_hyper_block_write, SUCCEED);

    assert(node && node->cinfo.cached);
    assert(io_info);

    /* Copy the elements into the user's buffer */
    /*
        !! NOTE !! This will need to be changed for different dimension
            permutations from the standard 'C' ordering!
    */
    HDmemcpy(node->cinfo.wpos, io_info->src, (size_t)(region_size*io_info->elmt_size));

    /*
     * Decrement the number of elements left in block to read & move the
     * offset
     */
    node->cinfo.wpos+=region_size*io_info->elmt_size;
    node->cinfo.wleft-=region_size;

    /* If we've read in all the elements from the block, throw it away */
    if(node->cinfo.wleft==0 && (node->cinfo.rleft==0 || node->cinfo.rleft==node->cinfo.size)) {
        /* Copy the location of the region in the file */
        HDmemcpy(file_offset, node->start, (io_info->space->extent.u.simple.rank * sizeof(hssize_t)));
        file_offset[io_info->space->extent.u.simple.rank]=0;

        /* Set the hyperslab size to write */
        for(u=0; u<io_info->space->extent.u.simple.rank; u++)
            hsize[u]=(node->end[u]-node->start[u])+1;
        hsize[io_info->space->extent.u.simple.rank]=io_info->elmt_size;

        if (H5F_arr_write(io_info->f, io_info->dxpl_id, io_info->layout,
                          io_info->pline, io_info->fill, io_info->efl, hsize,
                          hsize, zero, file_offset,
                          node->cinfo.block/*out*/)<0)
            HRETURN_ERROR (H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error");

        /* Release the temporary buffer */
        H5FL_BLK_FREE(hyper_block,node->cinfo.block);

        /* Reset the caching flag for next time */
        node->cinfo.cached=0;
    } /* end if */

    FUNC_LEAVE (SUCCEED);
}   /* H5S_hyper_block_write() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_fread
 *
 * Purpose:     Recursively gathers data points from a file using the
 *              parameters passed to H5S_hyper_fgath.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_fread (int dim, H5S_hyper_io_info_t *io_info)
{
    hsize_t region_size;                /* Size of lowest region */
    unsigned parm_init=0;          /* Whether one-shot parameters set up */
    H5S_hyper_region_t *regions;  /* Pointer to array of hyperslab nodes overlapped */
    size_t num_regions;         /* number of regions overlapped */
    size_t i;                   /* Counters */
    int j;
#ifdef QAK
    unsigned u;
#endif /* QAK */
    hsize_t num_read=0;          /* Number of elements read */
    const H5D_xfer_t *xfer_parms;/* Data transfer property list */

    FUNC_ENTER (H5S_hyper_fread, 0);

    assert(io_info);
    if (H5P_DEFAULT==io_info->dxpl_id) {
        xfer_parms = &H5D_xfer_dflt;
    } else {
        xfer_parms = H5I_object(io_info->dxpl_id);
        assert(xfer_parms);
    }

#ifdef QAK
    printf("%s: check 1.0, dim=%d\n",FUNC,dim);
#endif /* QAK */

    /* Get a sorted list (in the next dimension down) of the regions which */
    /*  overlap the current index in this dim */
    if((regions=H5S_hyper_get_regions(&num_regions,io_info->space->extent.u.simple.rank,
            (unsigned)(dim+1),
            io_info->space->select.sel_info.hslab.hyper_lst->count,
            io_info->space->select.sel_info.hslab.hyper_lst->lo_bounds,
            io_info->iter->hyp.pos,io_info->space->select.offset))!=NULL) {

        /*
         * Check if this is the second to last dimension in dataset (Which
         * means that we've got a list of the regions in the fastest changing
         * dimension and should input those regions).
         */
#ifdef QAK
    if(dim>=0) {
        printf("%s: check 2.0, rank=%d, ",
               FUNC,(int)io_info->space->extent.u.simple.rank);
        printf("%s: pos={",FUNC);
        for(u=0; u<io_info->space->extent.u.simple.rank; u++) {
            printf("%d",(int)io_info->iter->hyp.pos[u]);
            if(u<io_info->space->extent.u.simple.rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
    } /* end if */
    else
        printf("%s: check 2.0, rank=%d\n",
               FUNC,(int)io_info->space->extent.u.simple.rank);
    for(i=0; i<num_regions; i++)
        printf("%s: check 2.1, region #%d: start=%d, end=%d\n",
                   FUNC,i,(int)regions[i].start,(int)regions[i].end);
#endif /* QAK */
        if((unsigned)(dim+2)==io_info->space->extent.u.simple.rank) {
#ifdef QAK
        printf("%s: check 2.1.1, num_regions=%d\n",FUNC,(int)num_regions);
#endif /* QAK */
            /* perform I/O on data from regions */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                /* Compute the size of the region to read */
                H5_CHECK_OVERFLOW(io_info->nelmts,hsize_t,hssize_t);
                region_size=MIN((hssize_t)io_info->nelmts, (regions[i].end-regions[i].start)+1);

#ifdef QAK
        printf("%s: check 2.1.2, region=%d, region_size=%d, num_read=%lu\n",FUNC,(int)i,(int)region_size,(unsigned long)num_read);
#endif /* QAK */
                /* Check if this hyperslab block is cached or could be cached */
                if(!regions[i].node->cinfo.cached &&
                   (xfer_parms->cache_hyper &&
                    (xfer_parms->block_limit==0 ||
                     xfer_parms->block_limit>=(regions[i].node->cinfo.size*io_info->elmt_size)))) {
                    /* if we aren't cached, attempt to cache the block */
#ifdef QAK
        printf("%s: check 2.1.3, caching block\n",FUNC);
#endif /* QAK */
                    H5S_hyper_block_cache(regions[i].node,io_info,1);
                } /* end if */

                /* Read information from the cached block */
                if(regions[i].node->cinfo.cached) {
#ifdef QAK
        printf("%s: check 2.1.4, reading block from cache\n",FUNC);
#endif /* QAK */
                    if(H5S_hyper_block_read(regions[i].node,io_info,region_size)<0)
                        HRETURN_ERROR (H5E_DATASPACE, H5E_READERROR, 0, "read error");
                }
                else {
#ifdef QAK
        printf("%s: check 2.1.5, reading block from file, parm_init=%d\n",FUNC,(int)parm_init);
#endif /* QAK */
                    /* Set up hyperslab I/O parameters which apply to all regions */
                    if(!parm_init) {
                        /* Copy the location of the region in the file */
                        HDmemcpy(io_info->offset,io_info->iter->hyp.pos,(io_info->space->extent.u.simple.rank * sizeof(hssize_t)));
                        io_info->offset[io_info->space->extent.u.simple.rank]=0;

                        /* Set flag */
                        parm_init=1;
                    } /* end if */

#ifdef QAK
    printf("%s: check 2.2, i=%d, region_size=%d\n",FUNC,(int)i,(int)region_size);
#endif /* QAK */
                    /* Fill in the region specific parts of the I/O request */
                    io_info->hsize[io_info->space->extent.u.simple.rank-1]=region_size;
                    io_info->offset[io_info->space->extent.u.simple.rank-1]=regions[i].start;

                    /*
                     * Gather from file.
                     */
                    if (H5F_arr_read(io_info->f, io_info->dxpl_id,
                                     io_info->layout, io_info->pline,
                                     io_info->fill, io_info->efl,
                                     io_info->hsize, io_info->hsize,
                                     zero, io_info->offset,
                                     io_info->dst/*out*/)<0) {
                        HRETURN_ERROR (H5E_DATASPACE, H5E_READERROR, 0,
                                       "read error");
                    }
                } /* end else */
#ifdef QAK
    printf("%s: check 2.3, region #%d\n",FUNC,(int)i);
    printf("pos={");
    for(u=0; u<io_info->space->extent.u.simple.rank; u++) {
        printf("%d",(int)io_info->iter->hyp.pos[u]);
        if(u<io_info->space->extent.u.simple.rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
#endif /* QAK */

                /* Advance the pointer in the buffer */
                io_info->dst = ((uint8_t *)io_info->dst) + region_size*io_info->elmt_size;

                /* Increment the number of elements read */
                num_read+=region_size;

                /* Decrement the buffer left */
                io_info->nelmts-=region_size;

                /* Set the next position to start at */
                if(region_size==(hsize_t)((regions[i].end-regions[i].start)+1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim+1]=(-1);
                else
                    io_info->iter->hyp.pos[dim+1] = regions[i].start + region_size;
#ifdef QAK
    printf("%s: check 2.3.5, region #%d\n",FUNC,(int)i);
    printf("pos={");
    for(j=0; j<io_info->space->extent.u.simple.rank; j++) {
        printf("%d",(int)io_info->iter->hyp.pos[j]);
        if(j<io_info->space->extent.u.simple.rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
#endif /* QAK */

                /* Decrement the iterator count */
                io_info->iter->hyp.elmt_left-=region_size;
            } /* end for */
        } else { /* recurse on each region to next dimension down */
#ifdef QAK
    printf("%s: check 3.0, num_regions=%d\n",FUNC,(int)num_regions);
    for(i=0; i<num_regions; i++)
        printf("%s: region %d={%d, %d}\n", FUNC,i,(int)regions[i].start,(int)regions[i].end);
#endif /* QAK */

            /* Increment the dimension we are working with */
            dim++;

            /* Step through each region in this dimension */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
#ifdef QAK
    printf("%s: check 3.5, dim=%d, region #%d={%d, %d}\n",FUNC,(int)dim,(int)i,(int)regions[i].start,(int)regions[i].end);
{
    int k;

    printf("pos={");
    for(k=0; k<io_info->space->extent.u.simple.rank; k++) {
        printf("%d",(int)io_info->iter->hyp.pos[k]);
        if(k<io_info->space->extent.u.simple.rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
}
#endif /* QAK */
                /* Step through each location in each region */
                for(j=MAX(io_info->iter->hyp.pos[dim],regions[i].start); j<=regions[i].end && io_info->nelmts>0; j++) {
#ifdef QAK
    printf("%s: check 4.0, dim=%d, j=%d, num_read=%lu\n",FUNC,dim,j,(unsigned long)num_read);
#endif /* QAK */

                    /* Set the correct position we are working on */
                    io_info->iter->hyp.pos[dim]=j;

                    /* Go get the regions in the next lower dimension */
                    num_read+=H5S_hyper_fread(dim, io_info);

                    /* Advance to the next row if we got the whole region */
                    if(io_info->iter->hyp.pos[dim+1]==(-1))
                        io_info->iter->hyp.pos[dim]=j+1;
                } /* end for */
#ifdef QAK
{
    printf("%s: check 5.0, dim=%d, j=%d, region #%d={%d, %d}\n",FUNC,(int)dim,(int)j,(int)i,(int)regions[i].start,(int)regions[i].end);
    printf("%s: pos={",FUNC);
    for(u=0; u<io_info->space->extent.u.simple.rank; u++) {
        printf("%d",(int)io_info->iter->hyp.pos[u]);
        if(u<io_info->space->extent.u.simple.rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
}
#endif /* QAK */
                if(j>regions[i].end && io_info->iter->hyp.pos[dim+1]==(-1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim]=(-1);
            } /* end for */
#ifdef QAK
{
    int k;

    printf("%s: check 6.0, dim=%d\n",FUNC,(int)dim);
    printf("pos={");
    for(k=0; k<io_info->space->extent.u.simple.rank; k++) {
        printf("%d",(int)io_info->iter->hyp.pos[k]);
        if(k<io_info->space->extent.u.simple.rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
}
#endif /* QAK */
        } /* end else */

        /* Release the temporary buffer */
        H5FL_ARR_FREE(H5S_hyper_region_t,regions);
    } /* end if */

    FUNC_LEAVE (num_read);
}   /* H5S_hyper_fread() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_iter_next
 *
 * Purpose:     Moves a hyperslab iterator to the beginning of the next sequence
 *      of elements to read.  Handles walking off the end in all dimensions.
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 8, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5S_hyper_iter_next (const H5S_t *file_space, H5S_sel_iter_t *file_iter)
{
    hsize_t iter_offset[H5O_LAYOUT_NDIMS];
    hsize_t iter_count[H5O_LAYOUT_NDIMS];
    int fast_dim;  /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;  /* Temporary rank holder */
    unsigned i;        /* Counters */
    unsigned ndims;    /* Number of dimensions of dataset */

    FUNC_ENTER (H5S_hyper_iter_next, FAIL);

    /* Set some useful rank information */
    fast_dim=file_space->extent.u.simple.rank-1;
    ndims=file_space->extent.u.simple.rank;

    /* Calculate the offset and block count for each dimension */
    for(i=0; i<ndims; i++) {
        iter_offset[i]=(file_iter->hyp.pos[i]-file_space->select.sel_info.hslab.diminfo[i].start)%file_space->select.sel_info.hslab.diminfo[i].stride;
        iter_count[i]=(file_iter->hyp.pos[i]-file_space->select.sel_info.hslab.diminfo[i].start)/file_space->select.sel_info.hslab.diminfo[i].stride;
    } /* end for */

    /* Start with the fastest changing dimension */
    temp_dim=fast_dim;
    while(temp_dim>=0) {
        if(temp_dim==fast_dim) {
            /* Move to the next block in the current dimension */
            iter_offset[temp_dim]=0;    /* reset the offset in the fastest dimension */
            iter_count[temp_dim]++;

            /* If this block is still in the range of blocks to output for the dimension, break out of loop */
            if(iter_count[temp_dim]<file_space->select.sel_info.hslab.diminfo[temp_dim].count)
                break;
            else
                iter_count[temp_dim]=0; /* reset back to the beginning of the line */
        } /* end if */
        else {
            /* Move to the next row in the curent dimension */
            iter_offset[temp_dim]++;

            /* If this block is still in the range of blocks to output for the dimension, break out of loop */
            if(iter_offset[temp_dim]<file_space->select.sel_info.hslab.diminfo[temp_dim].block)
                break;
            else {
                /* Move to the next block in the current dimension */
                iter_offset[temp_dim]=0;
                iter_count[temp_dim]++;

                /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                if(iter_count[temp_dim]<file_space->select.sel_info.hslab.diminfo[temp_dim].count)
                    break;
                else
                    iter_count[temp_dim]=0; /* reset back to the beginning of the line */
            } /* end else */
        } /* end else */

        /* Decrement dimension count */
        temp_dim--;
    } /* end while */

    /* Translate current iter_offset and iter_count into iterator position */
    for(i=0; i<ndims; i++)
        file_iter->hyp.pos[i]=file_space->select.sel_info.hslab.diminfo[i].start+(file_space->select.sel_info.hslab.diminfo[i].stride*iter_count[i])+iter_offset[i];

    FUNC_LEAVE (SUCCEED);
} /* H5S_hyper_iter_next() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_fread_opt
 *
 * Purpose:     Performs an optimized gather from the file, based on a regular
 *      hyperslab (i.e. one which was generated from just one call to
 *      H5Sselect_hyperslab).
 *
 * Return:      Success:        Number of elements copied.
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 8, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_fread_opt (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline,
                 const struct H5O_fill_t *fill,
                 const struct H5O_efl_t *efl, size_t elmt_size,
                 const H5S_t *file_space, H5S_sel_iter_t *file_iter,
                 hsize_t nelmts, hid_t dxpl_id, void *_buf/*out*/)
{
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset on disk */
    hsize_t     slab[H5O_LAYOUT_NDIMS];         /* Hyperslab size */
    hsize_t     tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary block count */
    hsize_t     tmp_block[H5O_LAYOUT_NDIMS];    /* Temporary block offset */
    uint8_t     *buf=(uint8_t *)_buf;   /* Alias for pointer arithmetic */
    const H5S_hyper_dim_t *tdiminfo;    /* Local pointer to diminfo information */
    hssize_t fast_dim_start,            /* Local copies of fastest changing dimension info */
        fast_dim_offset;
    hsize_t fast_dim_stride,            /* Local copies of fastest changing dimension info */
        fast_dim_block,
        fast_dim_count,
        fast_dim_buf_off;
    int fast_dim;  /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;  /* Temporary rank holder */
    hsize_t     acc;    /* Accumulator */
    hsize_t     buf_off;        /* Current buffer offset for copying memory */
    hsize_t     last_buf_off;   /* Last buffer offset for copying memory */
    hsize_t buf_size;       /* Current size of the buffer to write */
    int i;         /* Counters */
    unsigned u;        /* Counters */
    int         ndims;      /* Number of dimensions of dataset */
    hsize_t actual_read;    /* The actual number of elements to read in */
    hsize_t actual_bytes;   /* The actual number of bytes to copy */
    hsize_t num_read=0;     /* Number of elements read */

    FUNC_ENTER (H5S_hyper_fread_opt, 0);

#ifdef QAK
printf("%s: Called!\n",FUNC);
#endif /* QAK */
    /* Check if this is the first element read in from the hyperslab */
    if(file_iter->hyp.pos[0]==(-1)) {
        for(u=0; u<file_space->extent.u.simple.rank; u++)
            file_iter->hyp.pos[u]=file_space->select.sel_info.hslab.diminfo[u].start;
    } /* end if */

#ifdef QAK
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: file_file->hyp.pos[%d]=%d\n",FUNC,(int)i,(int)file_iter->hyp.pos[i]);
#endif /* QAK */

    /* Set the rank of the fastest changing dimension */
    fast_dim=file_space->extent.u.simple.rank-1;
    ndims=file_space->extent.u.simple.rank;

    /* initialize row sizes for each dimension */
    for(i=(ndims-1),acc=1; i>=0; i--) {
        slab[i]=acc*elmt_size;
        acc*=file_space->extent.u.simple.size[i];
    } /* end for */

#ifdef QAK
    printf("%s: fast_dim=%d\n",FUNC,(int)fast_dim);
    printf("%s: file_space->select.sel_info.hslab.diminfo[%d].start=%d\n",FUNC,(int)fast_dim,(int)file_space->select.sel_info.hslab.diminfo[fast_dim].start);
    printf("%s: file_space->select.sel_info.hslab.diminfo[%d].stride=%d\n",FUNC,(int)fast_dim,(int)file_space->select.sel_info.hslab.diminfo[fast_dim].stride);
#endif /* QAK */
    /* Check if we stopped in the middle of a sequence of elements */
    if((file_iter->hyp.pos[fast_dim]-file_space->select.sel_info.hslab.diminfo[fast_dim].start)%file_space->select.sel_info.hslab.diminfo[fast_dim].stride!=0 ||
        ((file_iter->hyp.pos[fast_dim]!=file_space->select.sel_info.hslab.diminfo[fast_dim].start) && file_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)) {
        hsize_t leftover;  /* The number of elements left over from the last sequence */

#ifdef QAK
printf("%s: Check 1.0\n",FUNC);
#endif /* QAK */
        /* Calculate the number of elements left in the sequence */
        if(file_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)
            leftover=file_space->select.sel_info.hslab.diminfo[fast_dim].block-(file_iter->hyp.pos[fast_dim]-file_space->select.sel_info.hslab.diminfo[fast_dim].start);
        else
            leftover=file_space->select.sel_info.hslab.diminfo[fast_dim].block-((file_iter->hyp.pos[fast_dim]-file_space->select.sel_info.hslab.diminfo[fast_dim].start)%file_space->select.sel_info.hslab.diminfo[fast_dim].stride);

        /* Make certain that we don't read too many */
        actual_read=MIN(leftover,nelmts);
        actual_bytes=actual_read*elmt_size;

        /* Copy the location of the point to get */
        HDmemcpy(offset, file_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += file_space->select.offset[i];
#ifdef QAK
for(i=0; i<ndims+1; i++)
    printf("%s: offset[%d]=%d\n",FUNC,(int)i,(int)offset[i]);
#endif /* QAK */

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];
#ifdef QAK
printf("%s: buf_off=%ld, actual_read=%d, actual_bytes=%d\n",FUNC,(long)buf_off,(int)actual_read,(int)actual_bytes);
#endif /* QAK */

        /* Read in the rest of the sequence */
        if (H5F_seq_read(f, dxpl_id, layout, pline, fill, efl, file_space,
            elmt_size, actual_bytes, buf_off, buf/*out*/)<0) {
            HRETURN_ERROR(H5E_DATASPACE, H5E_READERROR, 0, "read error");
        }

        /* Increment the offset of the buffer */
        buf+=elmt_size*actual_read;

        /* Increment the count read */
        num_read+=actual_read;

        /* Advance the point iterator */
        /* If we had enough buffer space to read in the rest of the sequence
         * in the fastest changing dimension, move the iterator offset to
         * the beginning of the next block to read.  Otherwise, just advance
         * the iterator in the fastest changing dimension.
         */
        if(actual_read==leftover) {
            /* Move iterator offset to beginning of next sequence in the fastest changing dimension */
            H5S_hyper_iter_next(file_space,file_iter);
        } /* end if */
        else {
            file_iter->hyp.pos[fast_dim]+=actual_read; /* whole sequence not read in, just advance fastest dimension offset */
        } /* end if */
    } /* end if */

    /* Now that we've cleared the "remainder" of the previous fastest dimension
     * sequence, we must be at the beginning of a sequence, so use the fancy
     * algorithm to compute the offsets and run through as many as possible,
     * until the buffer fills up.
     */
    if(num_read<nelmts) { /* Just in case the "remainder" above filled the buffer */
#ifdef QAK
printf("%s: Check 2.0, ndims=%d, num_read=%d, nelmts=%d\n",FUNC,(int)ndims,(int)num_read,(int)nelmts);
#endif /* QAK */
        /* Compute the arrays to perform I/O on */
        /* Copy the location of the point to get */
        HDmemcpy(offset, file_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;
#ifdef QAK
for(i=0; i<ndims+1; i++)
    printf("%s: offset[%d]=%d\n",FUNC,(int)i,(int)offset[i]);
#endif /* QAK */

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += file_space->select.offset[i];

        /* Compute the current "counts" for this location */
        for(i=0; i<ndims; i++) {
            tmp_count[i] = (file_iter->hyp.pos[i]-file_space->select.sel_info.hslab.diminfo[i].start)%file_space->select.sel_info.hslab.diminfo[i].stride;
            tmp_block[i] = (file_iter->hyp.pos[i]-file_space->select.sel_info.hslab.diminfo[i].start)/file_space->select.sel_info.hslab.diminfo[i].stride;
        } /* end for */
#ifdef QAK
for(i=0; i<ndims; i++) {
    printf("%s: tmp_count[%d]=%d, tmp_block[%d]=%d\n",FUNC,(int)i,(int)tmp_count[i],(int)i,(int)tmp_block[i]);
    printf("%s: slab[%d]=%d\n",FUNC,(int)i,(int)slab[i]);
}
#endif /* QAK */

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        /* Set the number of elements to read each time */
        actual_read=file_space->select.sel_info.hslab.diminfo[fast_dim].block;

        /* Set the number of actual bytes */
        actual_bytes=actual_read*elmt_size;
#ifdef QAK
printf("%s: buf_off=%ld, actual_read=%d, actual_bytes=%d\n",FUNC,(long)buf_off,(int)actual_read,(int)actual_bytes);
#endif /* QAK */

#ifdef QAK
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: diminfo: start[%d]=%d, stride[%d]=%d, block[%d]=%d, count[%d]=%d\n",FUNC,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].start,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].stride,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].block,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].count);
#endif /* QAK */

        /* Set the last location & length to invalid numbers */
        last_buf_off=(hsize_t)-1;
        buf_size=0;

        /* Set the local copy of the diminfo pointer */
        tdiminfo=file_space->select.sel_info.hslab.diminfo;

        /* Set local copies of information for the fastest changing dimension */
        fast_dim_start=tdiminfo[fast_dim].start;
        fast_dim_stride=tdiminfo[fast_dim].stride;
        fast_dim_block=tdiminfo[fast_dim].block;
        fast_dim_count=tdiminfo[fast_dim].count;
        fast_dim_buf_off=slab[fast_dim]*fast_dim_stride;
        fast_dim_offset=fast_dim_start+file_space->select.offset[fast_dim];

        /* Read in data until an entire sequence can't be read in any longer */
        while(num_read<nelmts) {
            /* Check if we are running out of room in the buffer */
            if((actual_read+num_read)>nelmts) {
                actual_read=nelmts-num_read;
                actual_bytes=actual_read*elmt_size;
            } /* end if */

#ifdef QAK
printf("%s: num_read=%d\n",FUNC,(int)num_read);
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: tmp_count[%d]=%d, offset[%d]=%d\n",FUNC,(int)i,(int)tmp_count[i],(int)i,(int)offset[i]);
#endif /* QAK */

            /* check for the first read */
            if(last_buf_off==(hsize_t)-1) {
                last_buf_off=buf_off;
                buf_size=actual_bytes;
            } /* end if */
            else {
                /* Check if we are extending the buffer to read */
                if((last_buf_off+buf_size)==buf_off) {
                    buf_size+=actual_bytes;
                } /* end if */
                /*
                 * We've moved to another section of the dataset, read in the
                 *  previous piece and change the last position and length to
                 *  the current position and length
                 */
                else {
                    /* Read in the sequence */
                    if (H5F_seq_read(f, dxpl_id, layout, pline, fill, efl, file_space,
                        elmt_size, buf_size, last_buf_off, buf/*out*/)<0) {
                        HRETURN_ERROR(H5E_DATASPACE, H5E_READERROR, 0, "read error");
                    } /* end if */

                    /* Increment the offset of the buffer */
                    buf+=buf_size;

                    /* Updated the last position and length */
                    last_buf_off=buf_off;
                    buf_size=actual_bytes;

                } /* end else */
            } /* end else */

            /* Increment the count read */
            num_read+=actual_read;

            /* Increment the offset and count for the fastest changing dimension */

            /* Move to the next block in the current dimension */
            /* Check for partial block read! */
            if(actual_read<fast_dim_block) {
                offset[fast_dim]+=actual_read;
                buf_off+=actual_bytes;
                continue;   /* don't bother checking slower dimensions */
            } /* end if */
            else {
                offset[fast_dim]+=fast_dim_stride;    /* reset the offset in the fastest dimension */
                buf_off+=fast_dim_buf_off;
                tmp_count[fast_dim]++;
            } /* end else */

            /* If this block is still in the range of blocks to output for the dimension, break out of loop */
            if(tmp_count[fast_dim]<fast_dim_count)
                continue;   /* don't bother checking slower dimensions */
            else {
                tmp_count[fast_dim]=0; /* reset back to the beginning of the line */
                offset[fast_dim]=fast_dim_offset;

                /* Re-compute the initial buffer offset */
                for(i=0,buf_off=0; i<ndims; i++)
                    buf_off+=offset[i]*slab[i];
            } /* end else */

            /* Increment the offset and count for the other dimensions */
            temp_dim=fast_dim-1;
            while(temp_dim>=0) {
                /* Move to the next row in the curent dimension */
                offset[temp_dim]++;
                buf_off+=slab[temp_dim];
                tmp_block[temp_dim]++;

                /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                if(tmp_block[temp_dim]<tdiminfo[temp_dim].block)
                    break;
                else {
                    /* Move to the next block in the current dimension */
                    offset[temp_dim]+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block);
                    buf_off+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block)*slab[temp_dim];
                    tmp_block[temp_dim]=0;
                    tmp_count[temp_dim]++;

                    /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                    if(tmp_count[temp_dim]<tdiminfo[temp_dim].count)
                        break;
                    else {
                        tmp_count[temp_dim]=0; /* reset back to the beginning of the line */
                        tmp_block[temp_dim]=0;
                        offset[temp_dim]=tdiminfo[temp_dim].start+file_space->select.offset[temp_dim];

                        /* Re-compute the initial buffer offset */
                        for(i=0,buf_off=0; i<ndims; i++)
                            buf_off+=offset[i]*slab[i];
                    }
                } /* end else */

                /* Decrement dimension count */
                temp_dim--;
            } /* end while */
        } /* end while */

        /* check for the last read */
        if(last_buf_off!=(hsize_t)-1) {
            /* Read in the sequence */
            if (H5F_seq_read(f, dxpl_id, layout, pline, fill, efl, file_space,
                elmt_size, buf_size, last_buf_off, buf/*out*/)<0) {
                HRETURN_ERROR(H5E_DATASPACE, H5E_READERROR, 0, "read error");
            } /* end if */
        } /* end if */

        /* Update the iterator with the location we stopped */
        HDmemcpy(file_iter->hyp.pos, offset, ndims*sizeof(hssize_t));
    } /* end if */

    /* Decrement the number of elements left in selection */
    file_iter->hyp.elmt_left-=num_read;

    FUNC_LEAVE (num_read);
} /* H5S_hyper_fread_opt() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_fgath
 *
 * Purpose:     Gathers data points from file F and accumulates them in the
 *              type conversion buffer BUF.  The LAYOUT argument describes
 *              how the data is stored on disk and EFL describes how the data
 *              is organized in external files.  ELMT_SIZE is the size in
 *              bytes of a datum which this function treats as opaque.
 *              FILE_SPACE describes the data space of the dataset on disk
 *              and the elements that have been selected for reading (via
 *              hyperslab, etc).  This function will copy at most NELMTS elements.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-08-03
 *              The data transfer properties are passed by ID since that's
 *              what the virtual file layer needs.
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_fgath (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline,
                 const struct H5O_fill_t *fill,
                 const struct H5O_efl_t *efl, size_t elmt_size,
                 const H5S_t *file_space, H5S_sel_iter_t *file_iter,
                 hsize_t nelmts, hid_t dxpl_id, void *_buf/*out*/)
{
    H5S_hyper_io_info_t io_info;  /* Block of parameters to pass into recursive calls */
    hsize_t  num_read=0;       /* number of elements read into buffer */
    herr_t  ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_fgath, 0);

    /* Check args */
    assert (f);
    assert (layout);
    assert (elmt_size>0);
    assert (file_space);
    assert (file_iter);
    assert (nelmts>0);
    assert (_buf);

#ifdef QAK
    printf("%s: check 1.0\n", FUNC);
#endif /* QAK */

    /* Check for the special case of just one H5Sselect_hyperslab call made */
    if(file_space->select.sel_info.hslab.diminfo!=NULL) {
        /* Use optimized call to read in regular hyperslab */
        num_read=H5S_hyper_fread_opt(f,layout,pline,fill,efl,elmt_size,file_space,file_iter,nelmts,dxpl_id,_buf);
    } /* end if */
    /* Perform generic hyperslab operation */
    else {
        /* Initialize parameter block for recursive calls */
        io_info.f=f;
        io_info.layout=layout;
        io_info.pline=pline;
        io_info.fill=fill;
        io_info.efl=efl;
        io_info.elmt_size=elmt_size;
        io_info.space=file_space;
        io_info.iter=file_iter;
        io_info.nelmts=nelmts;
        io_info.dxpl_id = dxpl_id;
        io_info.src=NULL;
        io_info.dst=_buf;

        /* Set the hyperslab size to copy */
        io_info.hsize[0]=1;
        H5V_array_fill(io_info.hsize,io_info.hsize,sizeof(io_info.hsize[0]),file_space->extent.u.simple.rank);
        io_info.hsize[file_space->extent.u.simple.rank]=elmt_size;

        /* Recursively input the hyperslabs currently defined */
        /* starting with the slowest changing dimension */
        num_read=H5S_hyper_fread(-1,&io_info);
#ifdef QAK
        printf("%s: check 5.0, num_read=%d\n",FUNC,(int)num_read);
#endif /* QAK */
    } /* end else */

    FUNC_LEAVE (ret_value==SUCCEED ? num_read : 0);
} /* H5S_hyper_fgath() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_fwrite
 *
 * Purpose:     Recursively scatters data points to a file using the parameters
 *      passed to H5S_hyper_fscat.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_fwrite (int dim, H5S_hyper_io_info_t *io_info)
{
    hsize_t region_size;                /* Size of lowest region */
    unsigned parm_init=0;          /* Whether one-shot parameters set up */
    H5S_hyper_region_t *regions;  /* Pointer to array of hyperslab nodes overlapped */
    size_t num_regions;         /* number of regions overlapped */
    size_t i;                   /* Counters */
    int j;
    hsize_t num_written=0;          /* Number of elements read */
    const H5D_xfer_t *xfer_parms;       /* Data transfer properties */

    FUNC_ENTER (H5S_hyper_fwrite, 0);

    assert(io_info);
    if (H5P_DEFAULT==io_info->dxpl_id) {
        xfer_parms = &H5D_xfer_dflt;
    } else {
        xfer_parms = H5I_object(io_info->dxpl_id);
        assert(xfer_parms);
    }

#ifdef QAK
    printf("%s: check 1.0\n", FUNC);
#endif /* QAK */
    /* Get a sorted list (in the next dimension down) of the regions which */
    /*  overlap the current index in this dim */
    if((regions=H5S_hyper_get_regions(&num_regions,io_info->space->extent.u.simple.rank,
            (unsigned)(dim+1),
            io_info->space->select.sel_info.hslab.hyper_lst->count,
            io_info->space->select.sel_info.hslab.hyper_lst->lo_bounds,
            io_info->iter->hyp.pos,io_info->space->select.offset))!=NULL) {

#ifdef QAK
    printf("%s: check 1.1, regions=%p\n", FUNC,regions);
        printf("%s: check 1.2, rank=%d\n",
               FUNC,(int)io_info->space->extent.u.simple.rank);
        for(i=0; i<num_regions; i++)
            printf("%s: check 2.1, region #%d: start=%d, end=%d\n",
                   FUNC,i,(int)regions[i].start,(int)regions[i].end);
#endif /* QAK */

        /* Check if this is the second to last dimension in dataset */
        /*  (Which means that we've got a list of the regions in the fastest */
        /*   changing dimension and should input those regions) */
        if((unsigned)(dim+2)==io_info->space->extent.u.simple.rank) {

            /* perform I/O on data from regions */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                /* Compute the size of the region to read */
                H5_CHECK_OVERFLOW(io_info->nelmts,hsize_t,hssize_t);
                region_size=MIN((hssize_t)io_info->nelmts, (regions[i].end-regions[i].start)+1);

                /* Check if this hyperslab block is cached or could be cached */
                if(!regions[i].node->cinfo.cached && (xfer_parms->cache_hyper && (xfer_parms->block_limit==0 || xfer_parms->block_limit>=(regions[i].node->cinfo.size*io_info->elmt_size)))) {
                    /* if we aren't cached, attempt to cache the block */
                    H5S_hyper_block_cache(regions[i].node,io_info,0);
                } /* end if */

                /* Write information to the cached block */
                if(regions[i].node->cinfo.cached) {
                    if(H5S_hyper_block_write(regions[i].node,io_info,region_size)<0)
                        HRETURN_ERROR (H5E_DATASPACE, H5E_WRITEERROR, 0, "write error");
                }
                else {
                    /* Set up hyperslab I/O parameters which apply to all regions */
                    if(!parm_init) {
                        /* Copy the location of the region in the file */
                        HDmemcpy(io_info->offset, io_info->iter->hyp.pos, (io_info->space->extent.u.simple.rank * sizeof(hssize_t)));
                        io_info->offset[io_info->space->extent.u.simple.rank]=0;

                        /* Set flag */
                        parm_init=1;
                    } /* end if */

                    io_info->hsize[io_info->space->extent.u.simple.rank-1]=region_size;
                    io_info->offset[io_info->space->extent.u.simple.rank-1]=regions[i].start;

                    /*
                     * Scatter to file.
                     */
                    if (H5F_arr_write(io_info->f, io_info->dxpl_id,
                                      io_info->layout, io_info->pline,
                                      io_info->fill, io_info->efl,
                                      io_info->hsize, io_info->hsize, zero,
                                      io_info->offset, io_info->src)<0) {
                        HRETURN_ERROR (H5E_DATASPACE, H5E_WRITEERROR, 0, "write error");
                    }
                } /* end else */

                /* Advance the pointer in the buffer */
                io_info->src = ((const uint8_t *)io_info->src) +
                                   region_size*io_info->elmt_size;

                /* Increment the number of elements read */
                num_written+=region_size;

                /* Decrement the buffer left */
                io_info->nelmts-=region_size;

                /* Set the next position to start at */
                if(region_size==(hsize_t)((regions[i].end-regions[i].start)+1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim+1]=(-1);
                else
                    io_info->iter->hyp.pos[dim+1] = regions[i].start +
                                                        region_size;

                /* Decrement the iterator count */
                io_info->iter->hyp.elmt_left-=region_size;
            } /* end for */
        } else { /* recurse on each region to next dimension down */

            /* Increment the dimension we are working with */
            dim++;

            /* Step through each region in this dimension */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                /* Step through each location in each region */
                for(j=MAX(io_info->iter->hyp.pos[dim],regions[i].start); j<=regions[i].end && io_info->nelmts>0; j++) {
                    /* Set the correct position we are working on */
                    io_info->iter->hyp.pos[dim]=j;

                    /* Go get the regions in the next lower dimension */
                    num_written+=H5S_hyper_fwrite(dim, io_info);

                    /* Advance to the next row if we got the whole region */
                    if(io_info->iter->hyp.pos[dim+1]==(-1))
                        io_info->iter->hyp.pos[dim]=j+1;
                } /* end for */
                if(j>regions[i].end && io_info->iter->hyp.pos[dim+1]==(-1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim]=(-1);
            } /* end for */
        } /* end else */

        /* Release the temporary buffer */
        H5FL_ARR_FREE(H5S_hyper_region_t,regions);
    } /* end if */

#ifdef QAK
    printf("%s: check 2.0\n", FUNC);
#endif /* QAK */
    FUNC_LEAVE (num_written);
}   /* H5S_hyper_fwrite() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_fwrite_opt
 *
 * Purpose:     Performs an optimized scatter to the file, based on a regular
 *      hyperslab (i.e. one which was generated from just one call to
 *      H5Sselect_hyperslab).
 *
 * Return:      Success:        Number of elements copied.
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_fwrite_opt (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline,
                 const struct H5O_fill_t *fill,
                 const struct H5O_efl_t *efl, size_t elmt_size,
                 const H5S_t *file_space, H5S_sel_iter_t *file_iter,
                 hsize_t nelmts, hid_t dxpl_id, const void *_buf)
{
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset on disk */
    hsize_t     slab[H5O_LAYOUT_NDIMS];         /* Hyperslab size */
    hsize_t     tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary block count */
    hsize_t     tmp_block[H5O_LAYOUT_NDIMS];    /* Temporary block offset */
    const uint8_t       *buf=(const uint8_t *)_buf;   /* Alias for pointer arithmetic */
    const H5S_hyper_dim_t *tdiminfo;      /* Temporary pointer to diminfo information */
    hssize_t fast_dim_start,            /* Local copies of fastest changing dimension info */
        fast_dim_offset;
    hsize_t fast_dim_stride,            /* Local copies of fastest changing dimension info */
        fast_dim_block,
        fast_dim_count,
        fast_dim_buf_off;
    int fast_dim;  /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;  /* Temporary rank holder */
    hsize_t     acc;    /* Accumulator */
    hsize_t     buf_off;        /* Buffer offset for copying memory */
    hsize_t     last_buf_off;   /* Last buffer offset for copying memory */
    hsize_t buf_size;       /* Current size of the buffer to write */
    int i;         /* Counters */
    unsigned u;         /* Counters */
    int         ndims;      /* Number of dimensions of dataset */
    hsize_t actual_write;     /* The actual number of elements to read in */
    hsize_t actual_bytes;     /* The actual number of bytes to copy */
    hsize_t num_write=0;     /* Number of elements read */

    FUNC_ENTER (H5S_hyper_fwrite_opt, 0);

#ifdef QAK
printf("%s: Called!\n",FUNC);
#endif /* QAK */
    /* Check if this is the first element read in from the hyperslab */
    if(file_iter->hyp.pos[0]==(-1)) {
        for(u=0; u<file_space->extent.u.simple.rank; u++)
            file_iter->hyp.pos[u]=file_space->select.sel_info.hslab.diminfo[u].start;
    } /* end if */

#ifdef QAK
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: file_file->hyp.pos[%d]=%d\n",FUNC,(int)i,(int)file_iter->hyp.pos[i]);
#endif /* QAK */

    /* Set the rank of the fastest changing dimension */
    fast_dim=file_space->extent.u.simple.rank-1;
    ndims=file_space->extent.u.simple.rank;

    /* initialize row sizes for each dimension */
    for(i=(ndims-1),acc=1; i>=0; i--) {
        slab[i]=acc*elmt_size;
        acc*=file_space->extent.u.simple.size[i];
    } /* end for */

    /* Check if we stopped in the middle of a sequence of elements */
    if((file_iter->hyp.pos[fast_dim]-file_space->select.sel_info.hslab.diminfo[fast_dim].start)%file_space->select.sel_info.hslab.diminfo[fast_dim].stride!=0 ||
        ((file_iter->hyp.pos[fast_dim]!=file_space->select.sel_info.hslab.diminfo[fast_dim].start) && file_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)) {
        hsize_t leftover;  /* The number of elements left over from the last sequence */

#ifdef QAK
printf("%s: Check 1.0\n",FUNC);
#endif /* QAK */
        /* Calculate the number of elements left in the sequence */
        if(file_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)
            leftover=file_space->select.sel_info.hslab.diminfo[fast_dim].block-(file_iter->hyp.pos[fast_dim]-file_space->select.sel_info.hslab.diminfo[fast_dim].start);
        else
            leftover=file_space->select.sel_info.hslab.diminfo[fast_dim].block-((file_iter->hyp.pos[fast_dim]-file_space->select.sel_info.hslab.diminfo[fast_dim].start)%file_space->select.sel_info.hslab.diminfo[fast_dim].stride);

        /* Make certain that we don't write too many */
        actual_write=MIN(leftover,nelmts);
        actual_bytes=actual_write*elmt_size;

        /* Copy the location of the point to get */
        HDmemcpy(offset, file_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += file_space->select.offset[i];

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        /* Read in the rest of the sequence */
        if (H5F_seq_write(f, dxpl_id, layout, pline, fill, efl, file_space,
            elmt_size, actual_bytes, buf_off, buf)<0) {
            HRETURN_ERROR(H5E_DATASPACE, H5E_WRITEERROR, 0, "write error");
        }

        /* Increment the offset of the buffer */
        buf+=elmt_size*actual_write;

        /* Increment the count write */
        num_write+=actual_write;

        /* Advance the point iterator */
        /* If we had enough buffer space to write out the rest of the sequence
         * in the fastest changing dimension, move the iterator offset to
         * the beginning of the next block to write.  Otherwise, just advance
         * the iterator in the fastest changing dimension.
         */
        if(actual_write==leftover) {
            /* Move iterator offset to beginning of next sequence in the fastest changing dimension */
            H5S_hyper_iter_next(file_space,file_iter);
        } /* end if */
        else {
            file_iter->hyp.pos[fast_dim]+=actual_write; /* whole sequence not written out, just advance fastest dimension offset */
        } /* end if */
    } /* end if */

    /* Now that we've cleared the "remainder" of the previous fastest dimension
     * sequence, we must be at the beginning of a sequence, so use the fancy
     * algorithm to compute the offsets and run through as many as possible,
     * until the buffer fills up.
     */
    if(num_write<nelmts) { /* Just in case the "remainder" above filled the buffer */
#ifdef QAK
printf("%s: Check 2.0\n",FUNC);
#endif /* QAK */
        /* Compute the arrays to perform I/O on */
        /* Copy the location of the point to get */
        HDmemcpy(offset, file_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += file_space->select.offset[i];

        /* Compute the current "counts" for this location */
        for(i=0; i<ndims; i++) {
            tmp_count[i] = (file_iter->hyp.pos[i]-file_space->select.sel_info.hslab.diminfo[i].start)%file_space->select.sel_info.hslab.diminfo[i].stride;
            tmp_block[i] = (file_iter->hyp.pos[i]-file_space->select.sel_info.hslab.diminfo[i].start)/file_space->select.sel_info.hslab.diminfo[i].stride;
        } /* end for */

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        /* Set the number of elements to write each time */
        actual_write=file_space->select.sel_info.hslab.diminfo[fast_dim].block;

        /* Set the number of actual bytes */
        actual_bytes=actual_write*elmt_size;
#ifdef QAK
printf("%s: actual_write=%d\n",FUNC,(int)actual_write);
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: diminfo: start[%d]=%d, stride[%d]=%d, block[%d]=%d, count[%d]=%d\n",FUNC,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].start,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].stride,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].block,
        (int)i,(int)file_space->select.sel_info.hslab.diminfo[i].count);
#endif /* QAK */

        /* Set the last location & length to invalid numbers */
        last_buf_off=(hsize_t)-1;
        buf_size=0;

        /* Set the local copy of the diminfo pointer */
        tdiminfo=file_space->select.sel_info.hslab.diminfo;

        /* Set local copies of information for the fastest changing dimension */
        fast_dim_start=tdiminfo[fast_dim].start;
        fast_dim_stride=tdiminfo[fast_dim].stride;
        fast_dim_block=tdiminfo[fast_dim].block;
        fast_dim_count=tdiminfo[fast_dim].count;
        fast_dim_buf_off=slab[fast_dim]*fast_dim_stride;
        fast_dim_offset=fast_dim_start+file_space->select.offset[fast_dim];

        /* Read in data until an entire sequence can't be written out any longer */
        while(num_write<nelmts) {
            /* Check if we are running out of room in the buffer */
            if((actual_write+num_write)>nelmts) {
                actual_write=nelmts-num_write;
                actual_bytes=actual_write*elmt_size;
            } /* end if */

#ifdef QAK
printf("%s: num_write=%d\n",FUNC,(int)num_write);
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: tmp_count[%d]=%d, offset[%d]=%d\n",FUNC,(int)i,(int)tmp_count[i],(int)i,(int)offset[i]);
#endif /* QAK */

            /* check for the first write */
            if(last_buf_off==(hsize_t)-1) {
                last_buf_off=buf_off;
                buf_size=actual_bytes;
            } /* end if */
            else {
                /* Check if we are extending the buffer to write */
                if((last_buf_off+buf_size)==buf_off) {
                    buf_size+=actual_bytes;
                } /* end if */
                /*
                 * We've moved to another section of the dataset, write out the
                 *  previous piece and change the last position and length to
                 *  the current position and length
                 */
                else {
                    /* Write out the sequence */
                    if (H5F_seq_write(f, dxpl_id, layout, pline, fill, efl, file_space,
                        elmt_size, buf_size, last_buf_off, buf)<0) {
                        HRETURN_ERROR(H5E_DATASPACE, H5E_WRITEERROR, 0, "write error");
                    } /* end if */

                    /* Increment the offset of the buffer */
                    buf+=buf_size;

                    /* Updated the last position and length */
                    last_buf_off=buf_off;
                    buf_size=actual_bytes;

                } /* end else */
            } /* end else */

            /* Increment the count write */
            num_write+=actual_write;

            /* Increment the offset and count for the fastest changing dimension */

            /* Move to the next block in the current dimension */
            /* Check for partial block write! */
            if(actual_write<fast_dim_block) {
                offset[fast_dim]+=actual_write;
                buf_off+=actual_bytes;
                continue;   /* don't bother checking slower dimensions */
            } /* end if */
            else {
                offset[fast_dim]+=fast_dim_stride;    /* reset the offset in the fastest dimension */
                buf_off+=fast_dim_buf_off;
                tmp_count[fast_dim]++;
            } /* end else */

            /* If this block is still in the range of blocks to output for the dimension, break out of loop */
            if(tmp_count[fast_dim]<fast_dim_count)
                continue;   /* don't bother checking slower dimensions */
            else {
                tmp_count[fast_dim]=0; /* reset back to the beginning of the line */
                offset[fast_dim]=fast_dim_offset;

                /* Re-compute the initial buffer offset */
                for(i=0,buf_off=0; i<ndims; i++)
                    buf_off+=offset[i]*slab[i];
            } /* end else */

            /* Increment the offset and count for the other dimensions */
            temp_dim=fast_dim-1;
            while(temp_dim>=0) {
                /* Move to the next row in the curent dimension */
                offset[temp_dim]++;
                buf_off+=slab[temp_dim];
                tmp_block[temp_dim]++;

                /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                if(tmp_block[temp_dim]<tdiminfo[temp_dim].block)
                    break;
                else {
                    /* Move to the next block in the current dimension */
                    offset[temp_dim]+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block);
                    buf_off+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block)*slab[temp_dim];
                    tmp_block[temp_dim]=0;
                    tmp_count[temp_dim]++;

                    /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                    if(tmp_count[temp_dim]<tdiminfo[temp_dim].count)
                        break;
                    else {
                        tmp_count[temp_dim]=0; /* reset back to the beginning of the line */
                        tmp_block[temp_dim]=0;
                        offset[temp_dim]=tdiminfo[temp_dim].start+file_space->select.offset[temp_dim];

                        /* Re-compute the initial buffer offset */
                        for(i=0,buf_off=0; i<ndims; i++)
                            buf_off+=offset[i]*slab[i];
                    }
                } /* end else */

                /* Decrement dimension count */
                temp_dim--;
            } /* end while */
        } /* end while */

        /* check for the last write */
        if(last_buf_off!=(hsize_t)-1) {
            /* Write out the sequence */
            if (H5F_seq_write(f, dxpl_id, layout, pline, fill, efl, file_space,
                elmt_size, buf_size, last_buf_off, buf)<0) {
                HRETURN_ERROR(H5E_DATASPACE, H5E_WRITEERROR, 0, "write error");
            } /* end if */
        } /* end if */

        /* Update the iterator with the location we stopped */
        HDmemcpy(file_iter->hyp.pos, offset, ndims*sizeof(hssize_t));
    } /* end if */

    /* Decrement the number of elements left in selection */
    file_iter->hyp.elmt_left-=num_write;

    FUNC_LEAVE (num_write);
} /* H5S_hyper_fwrite_opt() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_fscat
 *
 * Purpose:     Scatters dataset elements from the type conversion buffer BUF
 *              to the file F where the data points are arranged according to
 *              the file data space FILE_SPACE and stored according to
 *              LAYOUT and EFL. Each element is ELMT_SIZE bytes.
 *              The caller is requesting that NELMTS elements are copied.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-08-03
 *              The data transfer properties are passed by ID since that's
 *              what the virtual file layer needs.
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_fscat (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline,
                 const struct H5O_fill_t *fill,
                 const struct H5O_efl_t *efl, size_t elmt_size,
                 const H5S_t *file_space, H5S_sel_iter_t *file_iter,
                 hsize_t nelmts, hid_t dxpl_id, const void *_buf)
{
    H5S_hyper_io_info_t io_info;  /* Block of parameters to pass into recursive calls */
    hsize_t  num_written=0;       /* number of elements read into buffer */
    herr_t  ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_fscat, 0);

    /* Check args */
    assert (f);
    assert (layout);
    assert (elmt_size>0);
    assert (file_space);
    assert (file_iter);
    assert (nelmts>0);
    assert (_buf);

#ifdef QAK
    printf("%s: check 1.0\n", FUNC);
#endif /* QAK */

    /* Check for the special case of just one H5Sselect_hyperslab call made */
    if(file_space->select.sel_info.hslab.diminfo!=NULL) {
        /* Use optimized call to write out regular hyperslab */
        num_written=H5S_hyper_fwrite_opt(f,layout,pline,fill,efl,elmt_size,file_space,file_iter,nelmts,dxpl_id,_buf);
    }
    else {
        /* Initialize parameter block for recursive calls */
        io_info.f=f;
        io_info.layout=layout;
        io_info.pline=pline;
        io_info.fill=fill;
        io_info.efl=efl;
        io_info.elmt_size=elmt_size;
        io_info.space=file_space;
        io_info.iter=file_iter;
        io_info.nelmts=nelmts;
        io_info.dxpl_id = dxpl_id;
        io_info.src=_buf;
        io_info.dst=NULL;

        /* Set the hyperslab size to copy */
        io_info.hsize[0]=1;
        H5V_array_fill(io_info.hsize,io_info.hsize,sizeof(io_info.hsize[0]),file_space->extent.u.simple.rank);
        io_info.hsize[file_space->extent.u.simple.rank]=elmt_size;

        /* Recursively input the hyperslabs currently defined */
        /* starting with the slowest changing dimension */
        num_written=H5S_hyper_fwrite(-1,&io_info);
#ifdef QAK
        printf("%s: check 2.0\n", FUNC);
#endif /* QAK */
    } /* end else */

    FUNC_LEAVE (ret_value==FAIL ? ret_value : (num_written >0) ? SUCCEED : FAIL);
} /* H5S_hyper_fscat() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_mread
 *
 * Purpose:     Recursively gathers data points from memory using the
 *              parameters passed to H5S_hyper_mgath.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_mread (int dim, H5S_hyper_io_info_t *io_info)
{
    hsize_t region_size;                /* Size of lowest region */
    H5S_hyper_region_t *regions;  /* Pointer to array of hyperslab nodes overlapped */
    size_t num_regions;         /* number of regions overlapped */
    size_t i;                   /* Counters */
    int j;
    hsize_t num_read=0;          /* Number of elements read */

    FUNC_ENTER (H5S_hyper_mread, 0);

    assert(io_info);

#ifdef QAK
    printf("%s: check 1.0, dim=%d\n",FUNC,dim);
#endif /* QAK */

    /* Get a sorted list (in the next dimension down) of the regions which */
    /*  overlap the current index in this dim */
    if((regions=H5S_hyper_get_regions(&num_regions,io_info->space->extent.u.simple.rank,
            (unsigned)(dim+1),
            io_info->space->select.sel_info.hslab.hyper_lst->count,
            io_info->space->select.sel_info.hslab.hyper_lst->lo_bounds,
            io_info->iter->hyp.pos,io_info->space->select.offset))!=NULL) {

        /* Check if this is the second to last dimension in dataset */
        /*  (Which means that we've got a list of the regions in the fastest */
        /*   changing dimension and should input those regions) */
#ifdef QAK
        printf("%s: check 2.0, rank=%d, num_regions=%d\n",
               FUNC, (int)io_info->space->extent.u.simple.rank,
               (int)num_regions);
        for(i=0; i<num_regions; i++)
            printf("%s: check 2.1, region #%d: start=%d, end=%d\n",
                   FUNC,i,(int)regions[i].start,(int)regions[i].end);
#endif /* QAK */

        if((unsigned)(dim+2)==io_info->space->extent.u.simple.rank) {

            /* Set up hyperslab I/O parameters which apply to all regions */

            /* Copy the location of the region in the file */
            HDmemcpy(io_info->offset, io_info->iter->hyp.pos, (io_info->space->extent.u.simple.rank * sizeof(hssize_t)));
            io_info->offset[io_info->space->extent.u.simple.rank]=0;

            /* perform I/O on data from regions */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                H5_CHECK_OVERFLOW(io_info->nelmts,hsize_t,hssize_t);
                region_size=MIN((hssize_t)io_info->nelmts,(regions[i].end-regions[i].start)+1);
                io_info->hsize[io_info->space->extent.u.simple.rank-1]=region_size;
                io_info->offset[io_info->space->extent.u.simple.rank-1]=regions[i].start;
#ifdef QAK
                printf("%s: check 2.1, i=%d, region_size=%d\n",
                       FUNC,(int)i,(int)region_size);
#endif /* QAK */

                /*
                 * Gather from memory.
                 */
                if (H5V_hyper_copy (io_info->space->extent.u.simple.rank+1,
                        io_info->hsize, io_info->hsize, zero, io_info->dst,
                        io_info->mem_size, io_info->offset, io_info->src)<0) {
                    HRETURN_ERROR (H5E_DATASPACE, H5E_READERROR, 0,
                                   "unable to gather data from memory");
                }

                /* Advance the pointer in the buffer */
                io_info->dst = ((uint8_t *)io_info->dst) + region_size*io_info->elmt_size;

                /* Increment the number of elements read */
                num_read+=region_size;

                /* Decrement the buffer left */
                io_info->nelmts-=region_size;

                /* Set the next position to start at */
                if(region_size==(hsize_t)((regions[i].end-regions[i].start)+1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim+1]=(-1);
                else
                    io_info->iter->hyp.pos[dim+1] =regions[i].start +
                                                       region_size;

                /* Decrement the iterator count */
                io_info->iter->hyp.elmt_left-=region_size;
            } /* end for */
        } else { /* recurse on each region to next dimension down */
#ifdef QAK
            printf("%s: check 3.0, num_regions=%d\n",FUNC,(int)num_regions);
#endif /* QAK */

            /* Increment the dimension we are working with */
            dim++;

            /* Step through each region in this dimension */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                /* Step through each location in each region */
                for(j=MAX(io_info->iter->hyp.pos[dim],regions[i].start); j<=regions[i].end && io_info->nelmts>0; j++) {
#ifdef QAK
                    printf("%s: check 4.0, dim=%d, location=%d\n",FUNC,dim,j);
#endif /* QAK */

                    /* Set the correct position we are working on */
                    io_info->iter->hyp.pos[dim]=j;

                    /* Go get the regions in the next lower dimension */
                    num_read+=H5S_hyper_mread(dim, io_info);

                    /* Advance to the next row if we got the whole region */
                    if(io_info->iter->hyp.pos[dim+1]==(-1))
                        io_info->iter->hyp.pos[dim]=j+1;
                } /* end for */
                if(j>regions[i].end && io_info->iter->hyp.pos[dim+1]==(-1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim]=(-1);
            } /* end for */
        } /* end else */

        /* Release the temporary buffer */
        H5FL_ARR_FREE(H5S_hyper_region_t,regions);
    } /* end if */

    FUNC_LEAVE (num_read);
}   /* H5S_hyper_mread() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_mread_opt
 *
 * Purpose:     Performs an optimized gather from a memory buffer, based on a
 *      regular hyperslab (i.e. one which was generated from just one call to
 *      H5Sselect_hyperslab).
 *
 * Return:      Success:        Number of elements copied.
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_mread_opt (const void *_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_tconv_buf/*out*/)
{
    hsize_t     mem_size[H5O_LAYOUT_NDIMS];     /* Size of the source buffer */
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset on disk */
    hsize_t     slab[H5O_LAYOUT_NDIMS];         /* Hyperslab size */
    hsize_t     tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary block count */
    hsize_t     tmp_block[H5O_LAYOUT_NDIMS];    /* Temporary block offset */
    const uint8_t       *src=(const uint8_t *)_buf;   /* Alias for pointer arithmetic */
    uint8_t     *dst=(uint8_t *)_tconv_buf;   /* Alias for pointer arithmetic */
    const H5S_hyper_dim_t *tdiminfo;      /* Temporary pointer to diminfo information */
    hssize_t fast_dim_start,            /* Local copies of fastest changing dimension info */
        fast_dim_offset;
    hsize_t fast_dim_stride,            /* Local copies of fastest changing dimension info */
        fast_dim_block,
        fast_dim_count,
        fast_dim_buf_off;
    int fast_dim;  /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;  /* Temporary rank holder */
    hsize_t     acc;    /* Accumulator */
    size_t      buf_off;  /* Buffer offset for copying memory */
    int i;         /* Counters */
    unsigned u;        /* Counters */
    int         ndims;      /* Number of dimensions of dataset */
    hsize_t actual_read;       /* The actual number of elements to read in */
    hsize_t actual_bytes;     /* The actual number of bytes to copy */
    hsize_t num_read=0;      /* Number of elements read */

    FUNC_ENTER (H5S_hyper_mread_opt, 0);

#ifdef QAK
printf("%s: Called!\n",FUNC);
#endif /* QAK */
    /* Check if this is the first element read in from the hyperslab */
    if(mem_iter->hyp.pos[0]==(-1)) {
        for(u=0; u<mem_space->extent.u.simple.rank; u++)
            mem_iter->hyp.pos[u]=mem_space->select.sel_info.hslab.diminfo[u].start;
    } /* end if */

#ifdef QAK
for(u=0; u<mem_space->extent.u.simple.rank; u++)
    printf("%s: mem_file->hyp.pos[%u]=%d\n",FUNC,(unsigned)u,(int)mem_iter->hyp.pos[u]);
#endif /* QAK */

    /* Set the aliases for dimension information */
    fast_dim=mem_space->extent.u.simple.rank-1;
    ndims=mem_space->extent.u.simple.rank;

    /* Set up the size of the memory space */
    HDmemcpy(mem_size, mem_space->extent.u.simple.size,mem_space->extent.u.simple.rank*sizeof(hsize_t));
    mem_size[mem_space->extent.u.simple.rank]=elmt_size;

    /* initialize row sizes for each dimension */
    for(i=(ndims-1),acc=1; i>=0; i--) {
        slab[i]=acc*elmt_size;
        acc*=mem_size[i];
    } /* end for */
#ifdef QAK
for(i=0; i<ndims; i++)
    printf("%s: mem_size[%d]=%d, slab[%d]=%d\n",FUNC,(int)i,(int)mem_size[i],(int)i,(int)slab[i]);
#endif /* QAK */

    /* Check if we stopped in the middle of a sequence of elements */
    if((mem_iter->hyp.pos[fast_dim]-mem_space->select.sel_info.hslab.diminfo[fast_dim].start)%mem_space->select.sel_info.hslab.diminfo[fast_dim].stride!=0 ||
        ((mem_iter->hyp.pos[fast_dim]!=mem_space->select.sel_info.hslab.diminfo[fast_dim].start) && mem_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)) {
        size_t leftover;  /* The number of elements left over from the last sequence */

#ifdef QAK
printf("%s: Check 1.0\n",FUNC);
#endif /* QAK */
        /* Calculate the number of elements left in the sequence */
        if(mem_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)
            leftover=mem_space->select.sel_info.hslab.diminfo[fast_dim].block-(mem_iter->hyp.pos[fast_dim]-mem_space->select.sel_info.hslab.diminfo[fast_dim].start);
        else
            leftover=mem_space->select.sel_info.hslab.diminfo[fast_dim].block-((mem_iter->hyp.pos[fast_dim]-mem_space->select.sel_info.hslab.diminfo[fast_dim].start)%mem_space->select.sel_info.hslab.diminfo[fast_dim].stride);

        /* Make certain that we don't read too many */
        actual_read=MIN(leftover,nelmts);
        actual_bytes=actual_read*elmt_size;

        /* Copy the location of the point to get */
        HDmemcpy(offset, mem_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += mem_space->select.offset[i];

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        assert(actual_bytes==(hsize_t)((size_t)actual_bytes)); /*check for overflow*/
        HDmemcpy(dst,src+buf_off,(size_t)actual_bytes);

        /* Increment the offset of the buffer */
        dst+=actual_bytes;

        /* Increment the count read */
        num_read+=actual_read;

        /* Advance the point iterator */
        /* If we had enough buffer space to read in the rest of the sequence
         * in the fastest changing dimension, move the iterator offset to
         * the beginning of the next block to read.  Otherwise, just advance
         * the iterator in the fastest changing dimension.
         */
        if(actual_read==leftover) {
            /* Move iterator offset to beginning of next sequence in the fastest changing dimension */
            H5S_hyper_iter_next(mem_space,mem_iter);
        } /* end if */
        else {
            mem_iter->hyp.pos[fast_dim]+=actual_read; /* whole sequence not read in, just advance fastest dimension offset */
        } /* end if */
    } /* end if */

    /* Now that we've cleared the "remainder" of the previous fastest dimension
     * sequence, we must be at the beginning of a sequence, so use the fancy
     * algorithm to compute the offsets and run through as many as possible,
     * until the buffer fills up.
     */
    if(num_read<nelmts) { /* Just in case the "remainder" above filled the buffer */
#ifdef QAK
printf("%s: Check 2.0\n",FUNC);
#endif /* QAK */
        /* Compute the arrays to perform I/O on */
        /* Copy the location of the point to get */
        HDmemcpy(offset, mem_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += mem_space->select.offset[i];

        /* Compute the current "counts" for this location */
        for(i=0; i<ndims; i++) {
            tmp_count[i] = (mem_iter->hyp.pos[i]-mem_space->select.sel_info.hslab.diminfo[i].start)%mem_space->select.sel_info.hslab.diminfo[i].stride;
            tmp_block[i] = (mem_iter->hyp.pos[i]-mem_space->select.sel_info.hslab.diminfo[i].start)/mem_space->select.sel_info.hslab.diminfo[i].stride;
        } /* end for */

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        /* Set the number of elements to read each time */
        actual_read=mem_space->select.sel_info.hslab.diminfo[fast_dim].block;

        /* Set the number of actual bytes */
        actual_bytes=actual_read*elmt_size;
#ifdef QAK
printf("%s: buf_off=%u, actual_bytes=%u\n",FUNC,(unsigned)buf_off,(int)actual_bytes);
#endif /* QAK */

#ifdef QAK
printf("%s: actual_read=%d\n",FUNC,(int)actual_read);
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: diminfo: start[%d]=%d, stride[%d]=%d, block[%d]=%d, count[%d]=%d\n",FUNC,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].start,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].stride,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].block,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].count);
#endif /* QAK */

        /* Set the local copy of the diminfo pointer */
        tdiminfo=mem_space->select.sel_info.hslab.diminfo;

        /* Set local copies of information for the fastest changing dimension */
        fast_dim_start=tdiminfo[fast_dim].start;
        fast_dim_stride=tdiminfo[fast_dim].stride;
        fast_dim_block=tdiminfo[fast_dim].block;
        fast_dim_count=tdiminfo[fast_dim].count;
        fast_dim_buf_off=slab[fast_dim]*fast_dim_stride;
        fast_dim_offset=fast_dim_start+mem_space->select.offset[fast_dim];

        /* Read in data until an entire sequence can't be written out any longer */
        while(num_read<nelmts) {
            /* Check if we are running out of room in the buffer */
            if((actual_read+num_read)>nelmts) {
                actual_read=nelmts-num_read;
                actual_bytes=actual_read*elmt_size;
            } /* end if */

#ifdef QAK
printf("%s: num_read=%d\n",FUNC,(int)num_read);
for(i=0; i<mem_space->extent.u.simple.rank; i++)
    printf("%s: tmp_count[%d]=%d, offset[%d]=%d\n",FUNC,(int)i,(int)tmp_count[i],(int)i,(int)offset[i]);
#endif /* QAK */

            /* Scatter out the rest of the sequence */
            assert(actual_bytes==(hsize_t)((size_t)actual_bytes)); /*check for overflow*/
            HDmemcpy(dst,src+buf_off,(size_t)actual_bytes);

            /* Increment the offset of the buffer */
            dst+=actual_bytes;

            /* Increment the count read */
            num_read+=actual_read;

            /* Increment the offset and count for the fastest changing dimension */

            /* Move to the next block in the current dimension */
            /* Check for partial block read! */
            if(actual_read<fast_dim_block) {
                offset[fast_dim]+=actual_read;
                buf_off+=actual_bytes;
                continue;   /* don't bother checking slower dimensions */
            } /* end if */
            else {
                offset[fast_dim]+=fast_dim_stride;    /* reset the offset in the fastest dimension */
                buf_off+=fast_dim_buf_off;
                tmp_count[fast_dim]++;
            } /* end else */

            /* If this block is still in the range of blocks to output for the dimension, break out of loop */
            if(tmp_count[fast_dim]<fast_dim_count)
                continue;   /* don't bother checking slower dimensions */
            else {
                tmp_count[fast_dim]=0; /* reset back to the beginning of the line */
                offset[fast_dim]=fast_dim_offset;

                /* Re-compute the initial buffer offset */
                for(i=0,buf_off=0; i<ndims; i++)
                    buf_off+=offset[i]*slab[i];
            } /* end else */

            /* Increment the offset and count for the other dimensions */
            temp_dim=fast_dim-1;
            while(temp_dim>=0) {
                /* Move to the next row in the curent dimension */
                offset[temp_dim]++;
                buf_off+=slab[temp_dim];
                tmp_block[temp_dim]++;

                /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                if(tmp_block[temp_dim]<tdiminfo[temp_dim].block)
                    break;
                else {
                    /* Move to the next block in the current dimension */
                    offset[temp_dim]+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block);
                    buf_off+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block)*slab[temp_dim];
                    tmp_block[temp_dim]=0;
                    tmp_count[temp_dim]++;

                    /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                    if(tmp_count[temp_dim]<tdiminfo[temp_dim].count)
                        break;
                    else {
                        tmp_count[temp_dim]=0; /* reset back to the beginning of the line */
                        tmp_block[temp_dim]=0;
                        offset[temp_dim]=tdiminfo[temp_dim].start+mem_space->select.offset[temp_dim];

                        /* Re-compute the initial buffer offset */
                        for(i=0,buf_off=0; i<ndims; i++)
                            buf_off+=offset[i]*slab[i];
                    }
                } /* end else */

                /* Decrement dimension count */
                temp_dim--;
            } /* end while */
        } /* end while */

        /* Update the iterator with the location we stopped */
        HDmemcpy(mem_iter->hyp.pos, offset, ndims*sizeof(hssize_t));
    } /* end if */

    /* Decrement the number of elements left in selection */
    mem_iter->hyp.elmt_left-=num_read;

    FUNC_LEAVE (num_read);
} /* end H5S_hyper_mread_opt() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_mgath
 *
 * Purpose:     Gathers dataset elements from application memory BUF and
 *              copies them into the data type conversion buffer TCONV_BUF.
 *              Each element is ELMT_SIZE bytes and arranged in application
 *              memory according to MEM_SPACE.  
 *              The caller is requesting that at most NELMTS be gathered.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_mgath (const void *_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_tconv_buf/*out*/)
{
    H5S_hyper_io_info_t io_info;  /* Block of parameters to pass into recursive calls */
    hsize_t  num_read;       /* number of elements read into buffer */

    FUNC_ENTER (H5S_hyper_mgath, 0);

#ifdef QAK
    printf("%s: check 1.0, elmt_size=%d, mem_space=%p\n",
           FUNC,(int)elmt_size,mem_space);
    printf("%s: check 1.0, mem_iter=%p, nelmts=%d\n",FUNC,mem_iter,nelmts);
    printf("%s: check 1.0, _buf=%p, _tconv_buf=%p\n",FUNC,_buf,_tconv_buf);
#endif /* QAK */

    /* Check args */
    assert (elmt_size>0);
    assert (mem_space);
    assert (mem_iter);
    assert (nelmts>0);
    assert (_buf);
    assert (_tconv_buf);

    /* Check for the special case of just one H5Sselect_hyperslab call made */
    if(mem_space->select.sel_info.hslab.diminfo!=NULL) {
        /* Use optimized call to read in regular hyperslab */
        num_read=H5S_hyper_mread_opt(_buf,elmt_size,mem_space,mem_iter,nelmts,_tconv_buf);
    }
    else {
        /* Initialize parameter block for recursive calls */
        io_info.elmt_size=elmt_size;
        io_info.space=mem_space;
        io_info.iter=mem_iter;
        io_info.nelmts=nelmts;
        io_info.src=_buf;
        io_info.dst=_tconv_buf;

        /* Set up the size of the memory space */
        HDmemcpy(io_info.mem_size, mem_space->extent.u.simple.size,mem_space->extent.u.simple.rank*sizeof(hsize_t));
        io_info.mem_size[mem_space->extent.u.simple.rank]=elmt_size;

        /* Set the hyperslab size to copy */
        io_info.hsize[0]=1;
        H5V_array_fill(io_info.hsize, io_info.hsize, sizeof(io_info.hsize[0]),mem_space->extent.u.simple.rank);
        io_info.hsize[mem_space->extent.u.simple.rank]=elmt_size;

        /* Recursively input the hyperslabs currently defined */
        /* starting with the slowest changing dimension */
        num_read=H5S_hyper_mread(-1,&io_info);
#ifdef QAK
        printf("%s: check 5.0, num_read=%d\n",FUNC,(int)num_read);
#endif /* QAK */
    } /* end else */

    FUNC_LEAVE (num_read);
}   /* H5S_hyper_mgath() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_mwrite
 *
 * Purpose:     Recursively scatters data points from memory using the parameters
 *      passed to H5S_hyper_mscat.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5S_hyper_mwrite (int dim, H5S_hyper_io_info_t *io_info)
{
    hsize_t region_size;        /* Size of lowest region */
    H5S_hyper_region_t *regions;  /* Pointer to array of hyperslab nodes overlapped */
    size_t num_regions;         /* number of regions overlapped */
    size_t i;                   /* Counters */
    int j;
    hsize_t num_write=0;         /* Number of elements written */

    FUNC_ENTER (H5S_hyper_mwrite, 0);

    assert(io_info);
#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */

    /* Get a sorted list (in the next dimension down) of the regions which */
    /*  overlap the current index in this dim */
    if((regions=H5S_hyper_get_regions(&num_regions,io_info->space->extent.u.simple.rank,
            (unsigned)(dim+1),
            io_info->space->select.sel_info.hslab.hyper_lst->count,
            io_info->space->select.sel_info.hslab.hyper_lst->lo_bounds,
            io_info->iter->hyp.pos,io_info->space->select.offset))!=NULL) {

#ifdef QAK
        printf("%s: check 2.0, rank=%d\n",
               FUNC,(int)io_info->space->extent.u.simple.rank);
        for(i=0; i<num_regions; i++)
            printf("%s: check 2.1, region #%d: start=%d, end=%d\n",
                   FUNC,i,(int)regions[i].start,(int)regions[i].end);
#endif /* QAK */
        /* Check if this is the second to last dimension in dataset */
        /*  (Which means that we've got a list of the regions in the fastest */
        /*   changing dimension and should input those regions) */
        if((unsigned)(dim+2)==io_info->space->extent.u.simple.rank) {

            /* Set up hyperslab I/O parameters which apply to all regions */

            /* Copy the location of the region in the file */
            HDmemcpy(io_info->offset, io_info->iter->hyp.pos, (io_info->space->extent.u.simple.rank* sizeof(hssize_t)));
            io_info->offset[io_info->space->extent.u.simple.rank]=0;

#ifdef QAK
            printf("%s: check 3.0\n",FUNC);
#endif /* QAK */
            /* perform I/O on data from regions */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                H5_CHECK_OVERFLOW(io_info->nelmts,hsize_t,hssize_t);
                region_size=MIN((hssize_t)io_info->nelmts, (regions[i].end-regions[i].start)+1);
                io_info->hsize[io_info->space->extent.u.simple.rank-1]=region_size;
                io_info->offset[io_info->space->extent.u.simple.rank-1]=regions[i].start;

                /*
                 * Scatter to memory
                 */
                if (H5V_hyper_copy (io_info->space->extent.u.simple.rank+1,
                                    io_info->hsize, io_info->mem_size, io_info->offset,
                                    io_info->dst, io_info->hsize, zero,
                                    io_info->src)<0) {
                    HRETURN_ERROR (H5E_DATASPACE, H5E_READERROR, 0, "unable to gather data from memory");
                }

                /* Advance the pointer in the buffer */
                io_info->src = ((const uint8_t *)io_info->src) +
                                   region_size*io_info->elmt_size;

                /* Increment the number of elements read */
                num_write+=region_size;

                /* Decrement the buffer left */
                io_info->nelmts-=region_size;

                /* Set the next position to start at */
                if(region_size==(hsize_t)((regions[i].end-regions[i].start)+1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim+1]=(-1);
                else
                    io_info->iter->hyp.pos[dim+1] = regions[i].start +
                                                        region_size;

                /* Decrement the iterator count */
                io_info->iter->hyp.elmt_left-=region_size;
            } /* end for */
        } else { /* recurse on each region to next dimension down */

            /* Increment the dimension we are working with */
            dim++;

#ifdef QAK
            printf("%s: check 6.0, num_regions=%d\n",FUNC,(int)num_regions);
#endif /* QAK */
            /* Step through each region in this dimension */
            for(i=0; i<num_regions && io_info->nelmts>0; i++) {
                /* Step through each location in each region */
#ifdef QAK
                printf("%s: check 7.0, start[%d]=%d, end[%d]=%d, nelmts=%d\n",
                       FUNC, i, (int)regions[i].start, i,
                       (int)regions[i].end, (int)io_info->nelmts);
#endif /* QAK */
                for(j=MAX(io_info->iter->hyp.pos[dim],regions[i].start); j<=regions[i].end && io_info->nelmts>0; j++) {

                    /* Set the correct position we are working on */
                    io_info->iter->hyp.pos[dim]=j;

                    /* Go get the regions in the next lower dimension */
                    num_write+=H5S_hyper_mwrite(dim, io_info);

                    /* Advance to the next row if we got the whole region */
                    if(io_info->iter->hyp.pos[dim+1]==(-1))
                        io_info->iter->hyp.pos[dim]=j+1;
                } /* end for */
                if(j>regions[i].end && io_info->iter->hyp.pos[dim+1]==(-1)
                        && i==(num_regions-1))
                    io_info->iter->hyp.pos[dim]=(-1);
            } /* end for */
        } /* end else */

        /* Release the temporary buffer */
        H5FL_ARR_FREE(H5S_hyper_region_t,regions);
    } /* end if */

    FUNC_LEAVE (num_write);
}   /* H5S_hyper_mwrite() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_mwrite_opt
 *
 * Purpose:     Performs an optimized scatter to a memory buffer, based on a
 *      regular hyperslab (i.e. one which was generated from just one call to
 *      H5Sselect_hyperslab).
 *
 * Return:      Success:        Number of elements copied.
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_hyper_mwrite_opt (const void *_tconv_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_buf/*out*/)
{
    hsize_t     mem_size[H5O_LAYOUT_NDIMS];     /* Size of the source buffer */
    hsize_t     slab[H5O_LAYOUT_NDIMS];         /* Hyperslab size */
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset on disk */
    hsize_t     tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary block count */
    hsize_t     tmp_block[H5O_LAYOUT_NDIMS];    /* Temporary block offset */
    const uint8_t       *src=(const uint8_t *)_tconv_buf;   /* Alias for pointer arithmetic */
    uint8_t     *dst=(uint8_t *)_buf;   /* Alias for pointer arithmetic */
    const H5S_hyper_dim_t *tdiminfo;      /* Temporary pointer to diminfo information */
    hssize_t fast_dim_start,            /* Local copies of fastest changing dimension info */
        fast_dim_offset;
    hsize_t fast_dim_stride,            /* Local copies of fastest changing dimension info */
        fast_dim_block,
        fast_dim_count,
        fast_dim_buf_off;
    int fast_dim;  /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;  /* Temporary rank holder */
    hsize_t     acc;    /* Accumulator */
    size_t      buf_off;  /* Buffer offset for copying memory */
    int i;         /* Counters */
    unsigned u;         /* Counters */
    int         ndims;      /* Number of dimensions of dataset */
    hsize_t actual_write;       /* The actual number of elements to read in */
    hsize_t actual_bytes;     /* The actual number of bytes to copy */
    hsize_t num_write=0;      /* Number of elements read */

    FUNC_ENTER (H5S_hyper_fwrite_opt, 0);

#ifdef QAK
printf("%s: Called!, elmt_size=%d\n",FUNC,(int)elmt_size);
#endif /* QAK */
    /* Check if this is the first element read in from the hyperslab */
    if(mem_iter->hyp.pos[0]==(-1)) {
        for(u=0; u<mem_space->extent.u.simple.rank; u++)
            mem_iter->hyp.pos[u]=mem_space->select.sel_info.hslab.diminfo[u].start;
    } /* end if */

#ifdef QAK
for(u=0; u<mem_space->extent.u.simple.rank; u++)
    printf("%s: mem_file->hyp.pos[%u]=%d\n",FUNC,(unsigned)u,(int)mem_iter->hyp.pos[u]);
#endif /* QAK */

    /* Set the aliases for a few important dimension ranks */
    fast_dim=mem_space->extent.u.simple.rank-1;
    ndims=mem_space->extent.u.simple.rank;

    /* Set up the size of the memory space */
    HDmemcpy(mem_size, mem_space->extent.u.simple.size,mem_space->extent.u.simple.rank*sizeof(hsize_t));
    mem_size[mem_space->extent.u.simple.rank]=elmt_size;

    /* initialize row sizes for each dimension */
    for(i=(ndims-1),acc=1; i>=0; i--) {
        slab[i]=acc*elmt_size;
        acc*=mem_size[i];
    } /* end for */
#ifdef QAK
for(i=0; i<ndims; i++)
    printf("%s: mem_size[%d]=%d, slab[%d]=%d\n",FUNC,(int)i,(int)mem_size[i],(int)i,(int)slab[i]);
#endif /* QAK */

    /* Check if we stopped in the middle of a sequence of elements */
    if((mem_iter->hyp.pos[fast_dim]-mem_space->select.sel_info.hslab.diminfo[fast_dim].start)%mem_space->select.sel_info.hslab.diminfo[fast_dim].stride!=0 ||
        ((mem_iter->hyp.pos[fast_dim]!=mem_space->select.sel_info.hslab.diminfo[fast_dim].start) && mem_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)) {
        unsigned leftover;  /* The number of elements left over from the last sequence */

#ifdef QAK
printf("%s: Check 1.0\n",FUNC);
#endif /* QAK */
        /* Calculate the number of elements left in the sequence */
        if(mem_space->select.sel_info.hslab.diminfo[fast_dim].stride==1)
            leftover=mem_space->select.sel_info.hslab.diminfo[fast_dim].block-(mem_iter->hyp.pos[fast_dim]-mem_space->select.sel_info.hslab.diminfo[fast_dim].start);
        else
            leftover=mem_space->select.sel_info.hslab.diminfo[fast_dim].block-((mem_iter->hyp.pos[fast_dim]-mem_space->select.sel_info.hslab.diminfo[fast_dim].start)%mem_space->select.sel_info.hslab.diminfo[fast_dim].stride);

        /* Make certain that we don't write too many */
        actual_write=MIN(leftover,nelmts);
        actual_bytes=actual_write*elmt_size;

        /* Copy the location of the point to get */
        HDmemcpy(offset, mem_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += mem_space->select.offset[i];

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        /* Scatter out the rest of the sequence */
        assert(actual_bytes==(hsize_t)((size_t)actual_bytes)); /*check for overflow*/
        HDmemcpy(dst+buf_off,src,(size_t)actual_bytes);

        /* Increment the offset of the buffer */
        src+=actual_bytes;

        /* Increment the count write */
        num_write+=actual_write;

        /* Advance the point iterator */
        /* If we had enough buffer space to write out the rest of the sequence
         * in the fastest changing dimension, move the iterator offset to
         * the beginning of the next block to write.  Otherwise, just advance
         * the iterator in the fastest changing dimension.
         */
        if(actual_write==leftover) {
            /* Move iterator offset to beginning of next sequence in the fastest changing dimension */
            H5S_hyper_iter_next(mem_space,mem_iter);
        } /* end if */
        else {
            mem_iter->hyp.pos[fast_dim]+=actual_write; /* whole sequence not written out, just advance fastest dimension offset */
        } /* end if */
    } /* end if */

    /* Now that we've cleared the "remainder" of the previous fastest dimension
     * sequence, we must be at the beginning of a sequence, so use the fancy
     * algorithm to compute the offsets and run through as many as possible,
     * until the buffer fills up.
     */
    if(num_write<nelmts) { /* Just in case the "remainder" above filled the buffer */
#ifdef QAK
printf("%s: Check 2.0\n",FUNC);
#endif /* QAK */
        /* Compute the arrays to perform I/O on */
        /* Copy the location of the point to get */
        HDmemcpy(offset, mem_iter->hyp.pos,ndims*sizeof(hssize_t));
        offset[ndims] = 0;

        /* Add in the selection offset */
        for(i=0; i<ndims; i++)
            offset[i] += mem_space->select.offset[i];

        /* Compute the current "counts" for this location */
        for(i=0; i<ndims; i++) {
            tmp_count[i] = (mem_iter->hyp.pos[i]-mem_space->select.sel_info.hslab.diminfo[i].start)%mem_space->select.sel_info.hslab.diminfo[i].stride;
            tmp_block[i] = (mem_iter->hyp.pos[i]-mem_space->select.sel_info.hslab.diminfo[i].start)/mem_space->select.sel_info.hslab.diminfo[i].stride;
        } /* end for */

        /* Compute the initial buffer offset */
        for(i=0,buf_off=0; i<ndims; i++)
            buf_off+=offset[i]*slab[i];

        /* Set the number of elements to write each time */
        actual_write=mem_space->select.sel_info.hslab.diminfo[fast_dim].block;

        /* Set the number of actual bytes */
        actual_bytes=actual_write*elmt_size;
#ifdef QAK
printf("%s: buf_off=%u, actual_bytes=%u\n",FUNC,(unsigned)buf_off,(int)actual_bytes);
#endif /* QAK */

#ifdef QAK
printf("%s: actual_write=%d\n",FUNC,(int)actual_write);
for(i=0; i<file_space->extent.u.simple.rank; i++)
    printf("%s: diminfo: start[%d]=%d, stride[%d]=%d, block[%d]=%d, count[%d]=%d\n",FUNC,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].start,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].stride,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].block,
        (int)i,(int)mem_space->select.sel_info.hslab.diminfo[i].count);
#endif /* QAK */

        /* Set the local copy of the diminfo pointer */
        tdiminfo=mem_space->select.sel_info.hslab.diminfo;

        /* Set local copies of information for the fastest changing dimension */
        fast_dim_start=tdiminfo[fast_dim].start;
        fast_dim_stride=tdiminfo[fast_dim].stride;
        fast_dim_block=tdiminfo[fast_dim].block;
        fast_dim_count=tdiminfo[fast_dim].count;
        fast_dim_buf_off=slab[fast_dim]*fast_dim_stride;
        fast_dim_offset=fast_dim_start+mem_space->select.offset[fast_dim];

        /* Read in data until an entire sequence can't be written out any longer */
        while(num_write<nelmts) {
            /* Check if we are running out of room in the buffer */
            if((actual_write+num_write)>nelmts) {
                actual_write=nelmts-num_write;
                actual_bytes=actual_write*elmt_size;
            } /* end if */

#ifdef QAK
printf("%s: num_write=%d\n",FUNC,(int)num_write);
for(i=0; i<mem_space->extent.u.simple.rank; i++)
    printf("%s: tmp_count[%d]=%d, offset[%d]=%d\n",FUNC,(int)i,(int)tmp_count[i],(int)i,(int)offset[i]);
#endif /* QAK */

            /* Scatter out the rest of the sequence */
            assert(actual_bytes==(hsize_t)((size_t)actual_bytes)); /*check for overflow*/
            HDmemcpy(dst+buf_off,src,(size_t)actual_bytes);

#ifdef QAK
printf("%s: buf_off=%u, actual_bytes=%u\n",FUNC,(unsigned)buf_off,(int)actual_bytes);
#endif /* QAK */

            /* Increment the offset of the buffer */
            src+=actual_bytes;

            /* Increment the count write */
            num_write+=actual_write;

            /* Increment the offset and count for the fastest changing dimension */

            /* Move to the next block in the current dimension */
            /* Check for partial block write! */
            if(actual_write<fast_dim_block) {
                offset[fast_dim]+=actual_write;
                buf_off+=actual_bytes;
                continue;   /* don't bother checking slower dimensions */
            } /* end if */
            else {
                offset[fast_dim]+=fast_dim_stride;    /* reset the offset in the fastest dimension */
                buf_off+=fast_dim_buf_off;
                tmp_count[fast_dim]++;
            } /* end else */

            /* If this block is still in the range of blocks to output for the dimension, break out of loop */
            if(tmp_count[fast_dim]<fast_dim_count)
                continue;   /* don't bother checking slower dimensions */
            else {
                tmp_count[fast_dim]=0; /* reset back to the beginning of the line */
                offset[fast_dim]=fast_dim_offset;

                /* Re-compute the initial buffer offset */
                for(i=0,buf_off=0; i<ndims; i++)
                    buf_off+=offset[i]*slab[i];
            } /* end else */

            /* Increment the offset and count for the other dimensions */
            temp_dim=fast_dim-1;
            while(temp_dim>=0) {
                /* Move to the next row in the curent dimension */
                offset[temp_dim]++;
                buf_off+=slab[temp_dim];
                tmp_block[temp_dim]++;

                /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                if(tmp_block[temp_dim]<tdiminfo[temp_dim].block)
                    break;
                else {
                    /* Move to the next block in the current dimension */
                    offset[temp_dim]+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block);
                    buf_off+=(tdiminfo[temp_dim].stride-tdiminfo[temp_dim].block)*slab[temp_dim];
                    tmp_block[temp_dim]=0;
                    tmp_count[temp_dim]++;

                    /* If this block is still in the range of blocks to output for the dimension, break out of loop */
                    if(tmp_count[temp_dim]<tdiminfo[temp_dim].count)
                        break;
                    else {
                        tmp_count[temp_dim]=0; /* reset back to the beginning of the line */
                        tmp_block[temp_dim]=0;
                        offset[temp_dim]=tdiminfo[temp_dim].start+mem_space->select.offset[temp_dim];

                        /* Re-compute the initial buffer offset */
                        for(i=0,buf_off=0; i<ndims; i++)
                            buf_off+=offset[i]*slab[i];
                    }
                } /* end else */

                /* Decrement dimension count */
                temp_dim--;
            } /* end while */
        } /* end while */

        /* Update the iterator with the location we stopped */
        HDmemcpy(mem_iter->hyp.pos, offset, ndims*sizeof(hssize_t));
    } /* end if */

    /* Decrement the number of elements left in selection */
    mem_iter->hyp.elmt_left-=num_write;

    FUNC_LEAVE (num_write);
} /* end H5S_hyper_mwrite_opt() */


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_mscat
 *
 * Purpose:     Scatters NELMTS data points from the type conversion buffer
 *              TCONV_BUF to the application buffer BUF.  Each element is
 *              ELMT_SIZE bytes and they are organized in application memory
 *              according to MEM_SPACE.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_mscat (const void *_tconv_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_buf/*out*/)
{
    H5S_hyper_io_info_t io_info;    /* Block of parameters to pass into recursive calls */
    hsize_t  num_written;            /* number of elements written into buffer */

    FUNC_ENTER (H5S_hyper_mscat, 0);

    /* Check args */
    assert (elmt_size>0);
    assert (mem_space);
    assert (mem_iter);
    assert (nelmts>0);
    assert (_buf);
    assert (_tconv_buf);

    /* Check for the special case of just one H5Sselect_hyperslab call made */
    if(mem_space->select.sel_info.hslab.diminfo!=NULL) {
        /* Use optimized call to write out regular hyperslab */
        num_written=H5S_hyper_mwrite_opt(_tconv_buf,elmt_size,mem_space,mem_iter,nelmts,_buf);
    }
    else {
        /* Initialize parameter block for recursive calls */
        io_info.elmt_size=elmt_size;
        io_info.space=mem_space;
        io_info.iter=mem_iter;
        io_info.nelmts=nelmts;
        io_info.src=_tconv_buf;
        io_info.dst=_buf;

        /* Set up the size of the memory space */
        HDmemcpy(io_info.mem_size, mem_space->extent.u.simple.size,mem_space->extent.u.simple.rank*sizeof(hsize_t));
        io_info.mem_size[mem_space->extent.u.simple.rank]=elmt_size;

        /* Set the hyperslab size to copy */
        io_info.hsize[0]=1;
        H5V_array_fill(io_info.hsize, io_info.hsize, sizeof(io_info.hsize[0]), mem_space->extent.u.simple.rank);
        io_info.hsize[mem_space->extent.u.simple.rank]=elmt_size;

        /* Recursively input the hyperslabs currently defined */
        /* starting with the slowest changing dimension */
#ifdef QAK
        printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
        num_written=H5S_hyper_mwrite(-1,&io_info);
#ifdef QAK
        printf("%s: check 2.0\n",FUNC);
#endif /* QAK */
    } /* end else */

    FUNC_LEAVE (num_written>0 ? SUCCEED : FAIL);
}   /* H5S_hyper_mscat() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_bound_comp
 PURPOSE
    Compare two hyperslab boundary elements (for qsort)
 USAGE
    herr_t H5S_hyper_bound_comp(b1,b2)
        const H5S_hyper_bound_t *b1;  IN: Pointer to the first boundary element
        const H5S_hyper_bound_t *b2;  IN: Pointer to the first boundary element
 RETURNS
    <0 if b1 compares less than b2
    0 if b1 compares equal to b2
    >0 if b1 compares greater than b2
 DESCRIPTION
    Callback routine for qsort to compary boundary elements.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5S_hyper_bound_comp(const void *_b1, const void *_b2)
{
    const H5S_hyper_bound_t *b1=(const H5S_hyper_bound_t *)_b1; /* Ptr to first boundary element */
    const H5S_hyper_bound_t *b2=(const H5S_hyper_bound_t *)_b2; /* Ptr to second boundary element */

#ifdef LATER
    FUNC_ENTER (H5S_hyper_bsearch, FAIL);
#endif /* LATER */

    assert(b1);
    assert(b2);

    if(b1->bound<b2->bound)
        return(-1);
    if(b1->bound>b2->bound)
        return(1);
    return(0);

#ifdef LATER
    FUNC_LEAVE (ret_value);
#endif /* LATER */
}   /* H5S_hyper_bsearch() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_node_add
 PURPOSE
    Add a new node to a list of hyperslab nodes
 USAGE
    herr_t H5S_hyper_node_add(head, start, size)
        H5S_hyper_node_t *head;   IN: Pointer to head of hyperslab list
        int endflag;             IN: "size" array actually contains "end" array
        unsigned rank;                IN: # of dimensions of the node
        const hssize_t *start;    IN: Offset of block
        const hsize_t *size;      IN: Size of block
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Adds a new hyperslab node to a list of them.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_node_add (H5S_hyper_node_t **head, int endflag, unsigned rank, const hssize_t *start, const hsize_t *size)
{
    H5S_hyper_node_t *slab;     /* New hyperslab node to add */
    unsigned u;     /* Counters */
    herr_t ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_node_add, FAIL);

    /* Check args */
    assert (head);
    assert (start);
    assert (size);

#ifdef QAK
    printf("%s: check 1.0, head=%p, *head=%p, rank=%u, endflag=%d\n",FUNC,head,*head,rank,endflag);
#endif /* QAK */
    /* Create new hyperslab node to insert */
    if((slab = H5FL_ALLOC(H5S_hyper_node_t,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab node");
    if((slab->start = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab start boundary");
    if((slab->end = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab end boundary");

#ifdef QAK
    printf("%s: check 2.0, slab=%p, slab->start=%p, slab->end=%p\n",FUNC,slab,slab->start,slab->end);
#endif /* QAK */
    /* Set boundary on new node */
    for(u=0; u<rank; u++) {
        slab->start[u]=start[u];
        if(endflag)
            slab->end[u]=size[u];
        else
            slab->end[u]=start[u]+size[u]-1;
    } /* end for */

    /* Prepend on list of hyperslabs for this selection */
    slab->next=*head;
    *head=slab;

done:
    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_node_add() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_node_prepend
 PURPOSE
    Prepend an existing node to an existing list of hyperslab nodes
 USAGE
    herr_t H5S_hyper_node_prepend(head, node)
        H5S_hyper_node_t **head;  IN: Pointer to pointer to head of hyperslab list
        H5S_hyper_node_t *node;   IN: Pointer to node to prepend
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Prepends an existing hyperslab node to a list of them.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_hyper_node_prepend (H5S_hyper_node_t **head, H5S_hyper_node_t *node)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_node_prepend, FAIL);

    /* Check args */
    assert (head);
    assert (node);

    /* Prepend on list of hyperslabs for this selection */
    node->next=*head;
    *head=node;

    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_node_prepend() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_node_release
 PURPOSE
    Free the memory for a hyperslab node
 USAGE
    herr_t H5S_hyper_node_release(node)
        H5S_hyper_node_t *node;   IN: Pointer to node to free
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Frees a hyperslab node.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_hyper_node_release (H5S_hyper_node_t *node)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_node_release, FAIL);

    /* Check args */
    assert (node);

    /* Free the hyperslab node */
    H5FL_ARR_FREE(hsize_t,node->start);
    H5FL_ARR_FREE(hsize_t,node->end);
    H5FL_FREE(H5S_hyper_node_t,node);

    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_node_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_add
 PURPOSE
    Add a block to hyperslab selection
 USAGE
    herr_t H5S_hyper_add(space, start, size)
        H5S_t *space;             IN: Pointer to dataspace
        const hssize_t *start;    IN: Offset of block
        const hsize_t *end;       IN: Offset of end of block
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Adds a block to an existing hyperslab selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_hyper_add (H5S_t *space, H5S_hyper_node_t *piece_lst)
{
    H5S_hyper_node_t *slab;     /* New hyperslab node to insert */
    H5S_hyper_node_t *tmp_slab; /* Temporary hyperslab node */
    H5S_hyper_bound_t *tmp;     /* Temporary pointer to an hyperslab bound array */
    size_t elem_count;          /* Number of elements in hyperslab selection */
    unsigned piece_count;          /* Number of hyperslab pieces being added */
    unsigned u;     /* Counters */
    herr_t ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_add, FAIL);

    /* Check args */
    assert (space);

    /* Count the number of hyperslab pieces to add to the selection */
    piece_count=0;
    tmp_slab=piece_lst;
    while(tmp_slab!=NULL) {
        piece_count++;
        tmp_slab=tmp_slab->next;
    } /* end while */
    
#ifdef QAK
    printf("%s: check 1.0, piece_count=%u, lo_bounds=%p\n",
           FUNC, (unsigned)piece_count,space->select.sel_info.hslab.hyper_lst->lo_bounds);
#endif /* QAK */
    /* Increase size of boundary arrays for dataspace's selection by piece_count */
    for(u=0; u<space->extent.u.simple.rank; u++) {
        tmp=space->select.sel_info.hslab.hyper_lst->lo_bounds[u];
#ifdef QAK
        printf("%s: check 1.1, u=%u, space->sel_info.count=%d, tmp=%p\n",FUNC,(unsigned)u, space->select.sel_info.hslab.hyper_lst->count,tmp);
#endif /* QAK */
        if((space->select.sel_info.hslab.hyper_lst->lo_bounds[u]=H5FL_ARR_REALLOC(H5S_hyper_bound_t,tmp,(hsize_t)(space->select.sel_info.hslab.hyper_lst->count+piece_count)))==NULL) {
            space->select.sel_info.hslab.hyper_lst->lo_bounds[u]=tmp;
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate hyperslab lo boundary array");
        } /* end if */
    } /* end for */

    while(piece_lst!=NULL) {
#ifdef QAK
        printf("%s: check 2.0\n",FUNC);
#endif /* QAK */
        /* Re-use the current H5S_hyper_node_t */
        slab=piece_lst;

        /* Don't loose place in list of nodes to add.. */
        piece_lst=piece_lst->next;

#ifdef QAK
        printf("%s: check 3.0\n",FUNC);
#endif /* QAK */
        /* Set boundary on new node */
        for(u=0,elem_count=1; u<space->extent.u.simple.rank; u++) {
#ifdef QAK
            printf("%s: check 3.1, %u: start=%d, end=%d, elem_count=%d\n",
               FUNC,(unsigned)u,(int)start[u],(int)end[u],(int)elem_count);
#endif /* QAK */
            elem_count*=(slab->end[u]-slab->start[u])+1;
        } /* end for */

        /* Initialize caching parameters */
        slab->cinfo.cached=0;
        slab->cinfo.size=elem_count;
        slab->cinfo.wleft=slab->cinfo.rleft=0;
        slab->cinfo.block=slab->cinfo.wpos=slab->cinfo.rpos=NULL;

#ifdef QAK
        printf("%s: check 4.0\n",FUNC);
        {
            unsigned v;
            
            for(u=0; u<space->extent.u.simple.rank; u++) {
                for(v=0; v<space->select.sel_info.hslab.hyper_lst->count; v++) {
                    printf("%s: lo_bound[%u][%u]=%d(%p)\n", FUNC,
                        u,v,(int)space->select.sel_info.hslab.hyper_lst->lo_bounds[u][v].bound,
                            space->select.sel_info.hslab.hyper_lst->lo_bounds[u][v].node);
                }
            }
        }
#endif /* QAK */
        /* Insert each boundary of the hyperslab into the sorted lists of bounds */
        for(u=0; u<space->extent.u.simple.rank; u++) {
#ifdef QAK
            printf("%s: check 4.1, start[%u]=%d, end[%u]=%d\n",
               FUNC, u, (int)slab->start[u],u,(int)slab->end[u]);
            printf("%s: check 4.1,.hslab.hyper_lst->count=%d\n",
               FUNC,(int)space->select.sel_info.hslab.hyper_lst->count);
#endif /* QAK */
            space->select.sel_info.hslab.hyper_lst->lo_bounds[u][space->select.sel_info.hslab.hyper_lst->count].bound=slab->start[u];
            space->select.sel_info.hslab.hyper_lst->lo_bounds[u][space->select.sel_info.hslab.hyper_lst->count].node=slab;
        } /* end for */

        /* Increment the number of bounds in the array */
        space->select.sel_info.hslab.hyper_lst->count++;
#ifdef QAK
        printf("%s: check 5.0, count=%d\n",FUNC,(int)space->select.sel_info.hslab.hyper_lst->count);
#endif /* QAK */
        
        /* Prepend on list of hyperslabs for this selection */
        slab->next=space->select.sel_info.hslab.hyper_lst->head;
        space->select.sel_info.hslab.hyper_lst->head=slab;

        /* Increment the number of elements in the hyperslab selection */
        space->select.num_elem+=elem_count;
#ifdef QAK
        printf("%s: check 6.0, elem_count=%d\n",FUNC,(int)elem_count);
        {
            unsigned v;
            
            for(u=0; u<space->extent.u.simple.rank; u++) {
                for(v=0; v<space->select.sel_info.hslab.hyper_lst->count; v++) {
                    printf("%s: lo_bound[%u][%u]=%d(%p)\n", FUNC,
                        u,v,(int)space->select.sel_info.hslab.hyper_lst->lo_bounds[u][v].bound,
                            space->select.sel_info.hslab.hyper_lst->lo_bounds[u][v].node);
                }
            }
        }
#endif /* QAK */
    } /* end while */

    /* Sort each dimension's array of bounds, now that they are all in the array */
    for(u=0; u<space->extent.u.simple.rank; u++)
        HDqsort(space->select.sel_info.hslab.hyper_lst->lo_bounds[u],space->select.sel_info.hslab.hyper_lst->count,sizeof(H5S_hyper_bound_t),H5S_hyper_bound_comp);

done:
    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_add() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_clip
 PURPOSE
    Clip a list of nodes against the current selection
 USAGE
    herr_t H5S_hyper_clip(space, nodes, uniq, overlap)
        H5S_t *space;             IN: Pointer to dataspace
        H5S_hyper_node_t *nodes;  IN: Pointer to list of nodes
        H5S_hyper_node_t **uniq;  IN: Handle to list of non-overlapping nodes
        H5S_hyper_node_t **overlap;  IN: Handle to list of overlapping nodes
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Clips a list of hyperslab nodes against the current hyperslab selection.
    The list of non-overlapping and overlapping nodes which are generated from
    this operation are returned in the 'uniq' and 'overlap' pointers.  If
    either of those lists are not needed, they may be set to NULL and the
    list will be released.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Clipping a multi-dimensional space against another multi-dimensional
    space generates at most 1 overlapping region and 2*<rank> non-overlapping
    regions, falling into the following categories in each dimension:
        Case 1 - A overlaps B on both sides:
            node            <----AAAAAAAA--->
                clipped against:
            existing        <-----BBBBB----->
                generates:
            overlapping     <-----CCCCC----->
            non-overlapping <----D---------->
            non-overlapping <----------EE--->

        Case 2 - A overlaps B on one side: (need to check both sides!)
            Case 2a:
                node            <------AAAAAA--->
                    clipped against:
                existing        <-----BBBBB----->
                    generates:
                overlapping     <------CCCC----->
                non-overlapping <----------EE--->
            Case 2b:
                node            <---AAAAA------->
                    clipped against:
                existing        <-----BBBBB----->
                    generates:
                overlapping     <-----CCC------->
                non-overlapping <---EE---------->

        Case 3 - A is entirely within B:
            node            <------AA------->
                clipped against:
            existing        <-----BBBBB----->
                generates:
            overlapping     <------CC------->

        Case 4 - A is entirely outside B: (doesn't matter which side)
            node            <-----------AAA->
                clipped against:
            existing        <-----BBBBB----->
                generates:
            non-overlapping <-----------AAA->

    This algorithm could be sped up by keeping track of the last (existing)
    region the new node was compared against when it was split and resume
    comparing against the region following that one when it's returned to
    later (for non-overlapping blocks).

    Another optimization is to build a n-tree (not certain about how many
    times each dimension should be cut, but at least once) for the dataspace
    and build a list of existing blocks which overlap each "n"-tant and only
    compare the new nodes against existing node in the region of the n-tree
    which the are located in.

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_clip (H5S_t *space, H5S_hyper_node_t *nodes, H5S_hyper_node_t **uniq,
        H5S_hyper_node_t **overlap)
{
    H5S_hyper_node_t *region,   /* Temp. hyperslab selection region pointer */
        *node,                  /* Temp. hyperslab node pointer */
        *next_node,             /* Pointer to next node in node list */
        *new_nodes=NULL;        /* List of new nodes added */
    hssize_t *start=NULL;       /* Temporary arrays of start & sizes (for splitting nodes) */
    hsize_t *end=NULL;          /* Temporary arrays of start & sizes (for splitting nodes) */
    unsigned rank;                 /* Cached copy of the rank of the dataspace */
    int overlapped;            /* Flag for overlapping nodes */
    int non_intersect;         /* Flag for non-intersecting nodes */
    unsigned u;     /* Counters */
    enum               /* Cases for edge overlaps */
        {OVERLAP_BOTH,OVERLAP_LOWER,OVERLAP_UPPER,WITHIN,NO_OVERLAP} clip_case;
    herr_t ret_value=SUCCEED;

    FUNC_ENTER (H5S_hyper_clip, FAIL);

    /* Check args */
    assert (space);
    assert (nodes);
    assert (uniq || overlap);

    /* Allocate space for the temporary starts & sizes */
    if((start = H5FL_ARR_ALLOC(hsize_t,(hsize_t)space->extent.u.simple.rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab start array");
    if((end = H5FL_ARR_ALLOC(hsize_t,(hsize_t)space->extent.u.simple.rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab size array");

    /* Set up local variables */
    rank=space->extent.u.simple.rank;
#ifdef QAK
    printf("%s: check 1.0, start=%p, end=%p\n",FUNC,start,end);
#endif /* QAK */

    /*
     * Cycle through all the hyperslab nodes, clipping them against the 
     * existing hyperslab selection.
     */
    node=nodes;
    while(node!=NULL) {
#ifdef QAK
    printf("%s: check 2.0, node=%p, nodes=%p\n",FUNC,node,nodes);
#endif /* QAK */
        /* Remove current node from head of list to evaulate it */
        next_node=node->next;   /* retain next node in list */
        node->next=NULL;    /* just to be safe */
#ifdef QAK
    printf("%s: check 2.1, node=%p, next_node=%p\n",FUNC,node,next_node);
    printf("node->start={",FUNC);
    for(u=0; u<rank; u++) {
        printf("%d",(int)node->start[u]);
        if(u<rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
    printf("node->end={",FUNC);
    for(u=0; u<rank; u++) {
        printf("%d",(int)node->end[u]);
        if(u<rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
    region=new_nodes;
    while(region!=NULL) {
        printf("new_nodes=%p, new_nodes->next=%p\n",region,region->next);
        printf("\tstart={",FUNC);
        for(u=0; u<rank; u++) {
            printf("%d",(int)region->start[u]);
            if(u<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        printf("\tend={",FUNC);
        for(u=0; u<rank; u++) {
            printf("%d",(int)region->end[u]);
            if(u<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        region=region->next;
    } /* end while */

    region=space->select.sel_info.hslab.hyper_lst->head;
    while(region!=NULL) {
        printf("region=%p, region->next=%p\n",region,region->next);
        printf("\tstart={",FUNC);
        for(u=0; u<rank; u++) {
            printf("%d",(int)region->start[u]);
            if(u<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        printf("\tend={",FUNC);
        for(u=0; u<rank; u++) {
            printf("%d",(int)region->end[u]);
            if(u<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        region=region->next;
    } /* end while */
#endif /* QAK */

        overlapped=0;       /* Reset overlapped flag */
        region=space->select.sel_info.hslab.hyper_lst->head;
        while(region!=NULL && overlapped==0) {
#ifdef QAK
    printf("%s: check 3.0, new_nodes=%p, region=%p, head=%p, overlapped=%d\n",FUNC,new_nodes,region,space->select.sel_info.hslab.hyper_lst->head,overlapped);
    printf("region->start={",FUNC);
    for(u=0; u<rank; u++) {
        printf("%d",(int)region->start[u]);
        if(u<rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
    printf("region->end={",FUNC);
    for(u=0; u<rank; u++) {
        printf("%d",(int)region->end[u]);
        if(u<rank-1)
            printf(", ");
    } /* end for */
    printf("}\n");
#endif /* QAK */
            /* Check for intersection */
            for(u=0, non_intersect=0; u<rank && non_intersect==0; u++) {
                if(node->end[u]<region->start[u] || node->start[u]>region->end[u])
                    non_intersect=1;
            } /* end for */

#ifdef QAK
    printf("%s: check 3.0.1, new_nodes=%p, region=%p, head=%p, non_intersect=%d\n",FUNC,new_nodes,region,space->select.sel_info.hslab.hyper_lst->head,non_intersect);
#endif /* QAK */
            /* Only compare node with regions that actually intersect */
            if(non_intersect==0) {
                /* Compare the boundaries of the two objects in each dimension */
                for(u=0; u<rank && overlapped==0; u++) {
                    /* Find overlap case we are in */

                    /* True if case 1, 4 or 2b */
                    if(node->start[u]<region->start[u]) {
#ifdef QAK
    printf("%s: check 3.1, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                        /* Test for case 4 */
                        /* NO_OVERLAP cases could be taken out, but are left in for clarity */
                        if(node->end[u]<region->start[u]) {
#ifdef QAK
    printf("%s: check 3.1.1, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                            clip_case=NO_OVERLAP;
                            assert("invalid clipping case" && 0);
                        } /* end if */
                        else {
#ifdef QAK
    printf("%s: check 3.1.2, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                            /* Test for case 2b */
                            if(node->end[u]<=region->end[u]) {
#ifdef QAK
    printf("%s: check 3.1.2.1, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                                clip_case=OVERLAP_LOWER;
                            } /* end if */
                            /* Must be case 1 */
                            else {
#ifdef QAK
    printf("%s: check 3.1.2.2, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                                clip_case=OVERLAP_BOTH;
                            } /* end else */
                        } /* end else */
                    } /* end if */
                    /* Case 2a, 3 or 4 (on the other side)*/
                    else {
#ifdef QAK
    printf("%s: check 3.2, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                        /* Test for case 4 */
                        if(node->start[u]>region->end[u]) {
#ifdef QAK
    printf("%s: check 3.2.1, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                            clip_case=NO_OVERLAP;
                            assert("invalid clipping case" && 0);
                        } /* end if */
                        /* Case 2a or 3 */
                        else {
#ifdef QAK
    printf("%s: check 3.2.2, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                            /* Test for case 2a */
                            if(node->end[u]>region->end[u]) {
#ifdef QAK
    printf("%s: check 3.2.2.1, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                                clip_case=OVERLAP_UPPER;
                            } /* end if */
                            /* Must be case 3 */
                            else {
#ifdef QAK
    printf("%s: check 3.2.2.2, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                                clip_case=WITHIN;
                            } /* end else */
                        } /* end else */
                    } /* end else */
                    
                    if(clip_case!=WITHIN) {
#ifdef QAK
    printf("%s: check 3.3, new_nodes=%p\n",FUNC,new_nodes);
#endif /* QAK */
                        /* Copy all the dimensions start & end points */
                        HDmemcpy(start,node->start,rank*sizeof(hssize_t));
                        HDmemcpy(end,node->end,rank*sizeof(hssize_t));
                    } /* end if */

                    /* Work on upper overlapping block */
                    if(clip_case==OVERLAP_BOTH || clip_case==OVERLAP_LOWER) {
#ifdef QAK
    printf("%s: check 3.4, new_nodes=%p\n",FUNC,new_nodes);
#endif /* QAK */
                        /* Modify the end point in the current dimension of the overlap */
                        end[u]=region->start[u]-1;
                        /* Clip the existing non-overlapped portion off the current node */
                        node->start[u]=region->start[u];
                        /* Add the non-overlapping portion to the list of new nodes */
                        if(H5S_hyper_node_add(&new_nodes,1,rank,(const hssize_t *)start,(const hsize_t *)end)<0)
                            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslab");
#ifdef QAK
    printf("%s: check 3.4.1, new_nodes=%p\n",FUNC,new_nodes);
#ifdef QAK
{
    H5S_hyper_node_t *tmp_reg;   /* Temp. hyperslab selection region pointer */
    unsigned v;

    tmp_reg=space->select.sel_info.hslab.hyper_lst->head;
    while(tmp_reg!=NULL) {
        printf("tmp_reg=%p\n",tmp_reg);
        printf("\tstart={",FUNC);
        for(v=0; v<rank; v++) {
            printf("%d",(int)tmp_reg->start[v]);
            if(v<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        printf("\tend={",FUNC);
        for(v=0; v<rank; v++) {
            printf("%d",(int)tmp_reg->end[v]);
            if(v<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        tmp_reg=tmp_reg->next;
    } /* end while */
}
#endif /* QAK */
#endif /* QAK */
                    } /* end if */

#ifdef QAK
    printf("%s: check 3.4.5, new_nodes=%p\n",FUNC,new_nodes);
#endif /* QAK */
                    /* Work on lower overlapping block */
                    if(clip_case==OVERLAP_BOTH || clip_case==OVERLAP_UPPER) {
                        /* Modify the start & end point in the current dimension of the overlap */
                        start[u]=region->end[u]+1;
                        end[u]=node->end[u];
                        /* Clip the existing non-overlapped portion off the current node */
                        node->end[u]=region->end[u];
                        /* Add the non-overlapping portion to the list of new nodes */
#ifdef QAK
    printf("%s: check 3.5, &new_nodes=%p, new_nodes=%p\n",FUNC,&new_nodes,new_nodes);
#endif /* QAK */
                        if(H5S_hyper_node_add(&new_nodes,1,rank,(const hssize_t *)start,(const hsize_t *)end)<0)
                            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslab");
#ifdef QAK
    printf("%s: check 3.5.1, &new_nodes=%p, new_nodes=%p\n",FUNC,&new_nodes,new_nodes);
#ifdef QAK
{
    H5S_hyper_node_t *tmp_reg;   /* Temp. hyperslab selection region pointer */
    unsigned v;

    tmp_reg=space->select.sel_info.hslab.hyper_lst->head;
    while(tmp_reg!=NULL) {
        printf("tmp_reg=%p\n",tmp_reg);
        printf("\tstart={",FUNC);
        for(v=0; v<rank; v++) {
            printf("%d",(int)tmp_reg->start[v]);
            if(v<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        printf("\tend={",FUNC);
        for(v=0; v<rank; v++) {
            printf("%d",(int)tmp_reg->end[v]);
            if(v<rank-1)
                printf(", ");
        } /* end for */
        printf("}\n");
        tmp_reg=tmp_reg->next;
    } /* end while */
}
#endif /* QAK */
#endif /* QAK */
                    } /* end if */

#ifdef QAK
    printf("%s: check 3.5.5, new_nodes=%p\n",FUNC,new_nodes);
#endif /* QAK */
                    /* Check if this is the last dimension */
                    /* Add the block to the "overlapped" list, if so */
                    /* Allow the algorithm to proceed to the next dimension otherwise */
                    if(u==(rank-1)) {   
#ifdef QAK
    printf("%s: check 3.6, overlapped=%d\n",FUNC,overlapped);
#endif /* QAK */
                        if(overlap!=NULL) {
                            if(H5S_hyper_node_prepend(overlap,node)<0)
                                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslab");
                        }
                        else {  /* Free the node if we aren't going to keep it */
#ifdef QAK
    printf("%s: check 3.6.1, node=%p\n",FUNC,node);
#endif /* QAK */
                            H5S_hyper_node_release(node);
                        } /* end else */
                        overlapped=1;   /* stop the algorithm for this block */
                    } /* end if */
                } /* end for */
            } /* end if */

            /* Advance to next hyperslab region */
            region=region->next;
        } /* end while */

        /* Check whether we should add the node to the non-overlapping list */
        if(!overlapped) {
#ifdef QAK
    printf("%s: check 3.7, node=%p\n",FUNC,node);
#endif /* QAK */
            if(uniq!=NULL) {
                if(H5S_hyper_node_prepend(uniq,node)<0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslab");
            }
            else {  /* Free the node if we aren't going to keep it */
#ifdef QAK
    printf("%s: check 3.7.1\n",FUNC);
#endif /* QAK */
                H5S_hyper_node_release(node);
            } /* end else */
        } /* end if */

        /* Advance to next hyperslab node */
        node=next_node;

        /* Check if we've added more nodes from splitting to the list */
        if(node==NULL && new_nodes!=NULL) {
            node=new_nodes;
            new_nodes=NULL;
        } /* end if */
    } /* end while */

done:
    if(start!=NULL)
        H5FL_ARR_FREE(hsize_t,start);
    if(end!=NULL)
        H5FL_ARR_FREE(hsize_t,end);

    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_clip() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_release
 PURPOSE
    Release hyperslab selection information for a dataspace
 USAGE
    herr_t H5S_hyper_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all hyperslab selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
 *      Robb Matzke, 1998-08-25
 *      The fields which are freed are set to NULL to prevent them from being
 *      freed again later.  This fixes some allocation problems where
 *      changing the hyperslab selection of one data space causes a core dump
 *      when closing some other data space.
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_release (H5S_t *space)
{
    H5S_hyper_node_t *curr,*next;   /* Pointer to hyperslab nodes */
    unsigned u;     /* Counters */

    FUNC_ENTER (H5S_hyper_release, FAIL);

    /* Check args */
    assert (space && H5S_SEL_HYPERSLABS==space->select.type);
#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */

    /* Reset the number of points selected */
    space->select.num_elem=0;

    /* Release the regular selection info */
    if(space->select.sel_info.hslab.diminfo!=NULL) {
        H5FL_ARR_FREE(H5S_hyper_dim_t,space->select.sel_info.hslab.diminfo);
        space->select.sel_info.hslab.diminfo = NULL;
        H5FL_ARR_FREE(H5S_hyper_dim_t,space->select.sel_info.hslab.app_diminfo);
        space->select.sel_info.hslab.app_diminfo = NULL;
    } /* end if */

    /* Release irregular hyperslab information */
    if(space->select.sel_info.hslab.hyper_lst!=NULL) {
        /* Release hi and lo boundary information */
        if(space->select.sel_info.hslab.hyper_lst->lo_bounds!=NULL) {
            for(u=0; u<space->extent.u.simple.rank; u++) {
                H5FL_ARR_FREE(H5S_hyper_bound_t,space->select.sel_info.hslab.hyper_lst->lo_bounds[u]);
                space->select.sel_info.hslab.hyper_lst->lo_bounds[u] = NULL;
            } /* end for */
            H5FL_ARR_FREE(H5S_hyper_bound_ptr_t,space->select.sel_info.hslab.hyper_lst->lo_bounds);
            space->select.sel_info.hslab.hyper_lst->lo_bounds = NULL;
        } /* end if */

        /* Release list of selected regions */
        curr=space->select.sel_info.hslab.hyper_lst->head;
        while(curr!=NULL) {
            next=curr->next;
            H5S_hyper_node_release(curr);
            curr=next;
        } /* end while */

        /* Release hyperslab selection node itself */
        H5FL_FREE(H5S_hyper_list_t,space->select.sel_info.hslab.hyper_lst);
        space->select.sel_info.hslab.hyper_lst=NULL;
    } /* end if */

#ifdef QAK
    printf("%s: check 2.0\n",FUNC);
#endif /* QAK */

    FUNC_LEAVE (SUCCEED);
}   /* H5S_hyper_release() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_npoints
 PURPOSE
    Compute number of elements in current selection
 USAGE
    hsize_t H5S_hyper_npoints(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Compute number of elements in current selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hsize_t
H5S_hyper_npoints (const H5S_t *space)
{
    FUNC_ENTER (H5S_hyper_npoints, 0);

    /* Check args */
    assert (space);

    FUNC_LEAVE (space->select.num_elem);
}   /* H5S_hyper_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_sel_iter_release
 PURPOSE
    Release hyperslab selection iterator information for a dataspace
 USAGE
    herr_t H5S_hyper_sel_iter_release(sel_iter)
        H5S_t *space;                   IN: Pointer to dataspace iterator is for
        H5S_sel_iter_t *sel_iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all information for a dataspace hyperslab selection iterator
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_sel_iter_release (H5S_sel_iter_t *sel_iter)
{
    FUNC_ENTER (H5S_hyper_sel_iter_release, FAIL);

    /* Check args */
    assert (sel_iter);

    if(sel_iter->hyp.pos!=NULL)
        H5FL_ARR_FREE(hsize_t,sel_iter->hyp.pos);

    FUNC_LEAVE (SUCCEED);
}   /* H5S_hyper_sel_iter_release() */

/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_compare_bounds
 *
 * Purpose:     Compares two bounds for equality
 *
 * Return:      an integer less than, equal to, or greater than zero if the first
 *          region is considered to be respectively less than, equal to, or
 *          greater than the second
 *
 * Programmer:  Quincey Koziol
 *              Friday, July 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_hyper_compare_bounds (const void *r1, const void *r2)
{
    if(((const H5S_hyper_bound_t *)r1)->bound<((const H5S_hyper_bound_t *)r2)->bound)
        return(-1);
    else
        if(((const H5S_hyper_bound_t *)r1)->bound>((const H5S_hyper_bound_t *)r2)->bound)
            return(1);
        else
            return(0);
}   /* end H5S_hyper_compare_bounds */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_hyper_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies all the hyperslab selection information from the source
    dataspace to the destination dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_copy (H5S_t *dst, const H5S_t *src)
{
    H5S_hyper_list_t *new_hyper=NULL;    /* New hyperslab selection */
    H5S_hyper_node_t *curr, *new, *new_head;    /* Hyperslab information nodes */
    H5S_hyper_dim_t *new_diminfo=NULL;  /* New per-dimension info array[rank] */
    unsigned u;                    /* Counters */
    size_t v;                   /* Counters */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER (H5S_hyper_copy, FAIL);

    assert(src);
    assert(dst);

#ifdef QAK
    printf("%s: check 3.0\n", FUNC);
#endif /* QAK */
    /* Check if there is regular hyperslab information to copy */
    if(src->select.sel_info.hslab.diminfo!=NULL) {
        /* Create the per-dimension selection info */
        if((new_diminfo = H5FL_ARR_ALLOC(H5S_hyper_dim_t,(hsize_t)src->extent.u.simple.rank,0))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate per-dimension array");

        /* Copy the per-dimension selection info */
        for(u=0; u<src->extent.u.simple.rank; u++) {
            new_diminfo[u].start = src->select.sel_info.hslab.diminfo[u].start;
            new_diminfo[u].stride = src->select.sel_info.hslab.diminfo[u].stride;
            new_diminfo[u].count = src->select.sel_info.hslab.diminfo[u].count;
            new_diminfo[u].block = src->select.sel_info.hslab.diminfo[u].block;
        } /* end for */
        dst->select.sel_info.hslab.diminfo = new_diminfo;

        /* Create the per-dimension selection info */
        if((new_diminfo = H5FL_ARR_ALLOC(H5S_hyper_dim_t,(hsize_t)src->extent.u.simple.rank,0))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate per-dimension array");

        /* Copy the per-dimension selection info */
        for(u=0; u<src->extent.u.simple.rank; u++) {
            new_diminfo[u].start = src->select.sel_info.hslab.app_diminfo[u].start;
            new_diminfo[u].stride = src->select.sel_info.hslab.app_diminfo[u].stride;
            new_diminfo[u].count = src->select.sel_info.hslab.app_diminfo[u].count;
            new_diminfo[u].block = src->select.sel_info.hslab.app_diminfo[u].block;
        } /* end for */
        dst->select.sel_info.hslab.app_diminfo = new_diminfo;
    } /* end if */
    else {
        dst->select.sel_info.hslab.diminfo = new_diminfo;
        dst->select.sel_info.hslab.app_diminfo = new_diminfo;
    } /* end else */

    /* Check if there is irregular hyperslab information to copy */
    if(src->select.sel_info.hslab.hyper_lst!=NULL) {
        /* Create the new hyperslab information node */
        if((new_hyper = H5FL_ALLOC(H5S_hyper_list_t,0))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate point node");

        /* Copy the basic hyperslab selection information */
        *new_hyper=*(src->select.sel_info.hslab.hyper_lst);

#ifdef QAK
        printf("%s: check 4.0\n", FUNC);
#endif /* QAK */
        /* Allocate space for the low & high bound arrays */
        if((new_hyper->lo_bounds = H5FL_ARR_ALLOC(H5S_hyper_bound_ptr_t,(hsize_t)src->extent.u.simple.rank,0))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate boundary node");
        for(u=0; u<src->extent.u.simple.rank; u++) {
            if((new_hyper->lo_bounds[u] = H5FL_ARR_ALLOC(H5S_hyper_bound_t,(hsize_t)src->select.sel_info.hslab.hyper_lst->count,0))==NULL)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "can't allocate boundary list");
        } /* end for */

#ifdef QAK
        printf("%s: check 5.0\n", FUNC);
#endif /* QAK */
        /* Copy the hyperslab selection nodes, adding them to the lo & hi bound arrays also */
        curr=src->select.sel_info.hslab.hyper_lst->head;
        new_head=NULL;
        v=0;
        while(curr!=NULL) {
#ifdef QAK
        printf("%s: check 5.1\n", FUNC);
#endif /* QAK */
            /* Create each point */
            if((new = H5FL_ALLOC(H5S_hyper_node_t,0))==NULL)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "can't allocate point node");
            HDmemcpy(new,curr,sizeof(H5S_hyper_node_t));    /* copy caching information */
            if((new->start = H5FL_ARR_ALLOC(hsize_t,(hsize_t)src->extent.u.simple.rank,0))==NULL)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "can't allocate coordinate information");
            if((new->end = H5FL_ARR_ALLOC(hsize_t,(hsize_t)src->extent.u.simple.rank,0))==NULL)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "can't allocate coordinate information");
            HDmemcpy(new->start,curr->start,(src->extent.u.simple.rank*sizeof(hssize_t)));
            HDmemcpy(new->end,curr->end,(src->extent.u.simple.rank*sizeof(hssize_t)));
            new->next=NULL;

            /* Insert into low & high bound arrays */
            for(u=0; u<src->extent.u.simple.rank; u++) {
                new_hyper->lo_bounds[u][v].bound=new->start[u];
                new_hyper->lo_bounds[u][v].node=new;
            } /* end for */
            v++;    /* Increment the location of the next node in the boundary arrays */

            /* Keep the order the same when copying */
            if(new_head==NULL)
                new_head=new_hyper->head=new;
            else {
                new_head->next=new;
                new_head=new;
            } /* end else */

            curr=curr->next;
        } /* end while */
#ifdef QAK
        printf("%s: check 6.0\n", FUNC);
#endif /* QAK */

        /* Sort the boundary array */
        for(u=0; u<src->extent.u.simple.rank; u++)
            HDqsort(new_hyper->lo_bounds[u], new_hyper->count, sizeof(H5S_hyper_bound_t), H5S_hyper_compare_bounds);
#ifdef QAK
        printf("%s: check 7.0\n", FUNC);
#endif /* QAK */
    } /* end if */

    /* Attach the hyperslab information to the destination dataspace */
    dst->select.sel_info.hslab.hyper_lst=new_hyper;

done:
    FUNC_LEAVE (ret_value);
} /* end H5S_hyper_copy() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_hyper_select_valid(space);
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offet fits within the
    extent for the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_hyper_select_valid (const H5S_t *space)
{
    H5S_hyper_node_t *curr;     /* Hyperslab information nodes */
    unsigned u;                    /* Counter */
    htri_t ret_value=TRUE;      /* return value */

    FUNC_ENTER (H5S_hyper_select_valid, FAIL);

    assert(space);

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo != NULL) {
        const H5S_hyper_dim_t *diminfo=space->select.sel_info.hslab.diminfo; /* local alias for diminfo */
        hssize_t end;      /* The high bound of a region in a dimension */

        /* Check each dimension */
        for(u=0; u<space->extent.u.simple.rank; u++) {
            /* if block or count is zero, then can skip the test since */
            /* no data point is chosen */
            if (diminfo[u].count*diminfo[u].block != 0) {
                /* Bounds check the start point in this dimension */
                if((diminfo[u].start+space->select.offset[u])<0 ||
                    (diminfo[u].start+space->select.offset[u])>=(hssize_t)space->extent.u.simple.size[u]) {
                    ret_value=FALSE;
                    break;
                } /* end if */

                /* Compute the largest location in this dimension */
                end=diminfo[u].start+diminfo[u].stride*(diminfo[u].count-1)+(diminfo[u].block-1)+space->select.offset[u];

                /* Bounds check the end point in this dimension */
                if(end<0 || end>=(hssize_t)space->extent.u.simple.size[u]) {
                    ret_value=FALSE;
                    break;
                } /* end if */
            }
        } /* end for */
    } /* end if */
    else {
        /* Check each point to determine whether selection+offset is within extent */
        curr=space->select.sel_info.hslab.hyper_lst->head;
        while(curr!=NULL && ret_value==TRUE) {
            /* Check each dimension */
            for(u=0; u<space->extent.u.simple.rank; u++) {
                /* Check if an offset has been defined */
                /* Bounds check the selected point + offset against the extent */
                if(((curr->start[u]+space->select.offset[u])>=(hssize_t)space->extent.u.simple.size[u])
                        || ((curr->start[u]+space->select.offset[u])<0)
                        || ((curr->end[u]+space->select.offset[u])>=(hssize_t)space->extent.u.simple.size[u])
                        || ((curr->end[u]+space->select.offset[u])<0)) {
                    ret_value=FALSE;
                    break;
                } /* end if */
            } /* end for */

            curr=curr->next;
        } /* end while */
    } /* end while */

    FUNC_LEAVE (ret_value);
} /* end H5S_hyper_select_valid() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_serial_size
 PURPOSE
    Determine the number of bytes needed to store the serialized hyperslab
        selection information.
 USAGE
    hssize_t H5S_hyper_select_serial_size(space)
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize the current hyperslab
    selection information for storage on disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5S_hyper_select_serial_size (const H5S_t *space)
{
    H5S_hyper_node_t *curr;     /* Hyperslab information nodes */
    unsigned u;                    /* Counter */
    hssize_t block_count;       /* block counter for regular hyperslabs */
    hssize_t ret_value=FAIL;    /* return value */

    FUNC_ENTER (H5S_hyper_select_serial_size, FAIL);

    assert(space);

    /* Basic number of bytes required to serialize point selection:
     *  <type (4 bytes)> + <version (4 bytes)> + <padding (4 bytes)> + 
     *      <length (4 bytes)> + <rank (4 bytes)> + <# of blocks (4 bytes)> = 24 bytes
     */
    ret_value=24;

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo != NULL) {
        /* Check each dimension */
        for(block_count=1,u=0; u<space->extent.u.simple.rank; u++)
            block_count*=space->select.sel_info.hslab.diminfo[u].count;
        ret_value+=8*block_count*space->extent.u.simple.rank;
    } /* end if */
    else {
        /* Spin through hyperslabs to total the space needed to store them */
        curr=space->select.sel_info.hslab.hyper_lst->head;
        while(curr!=NULL) {
            /* Add 8 bytes times the rank for each element selected */
            ret_value+=8*space->extent.u.simple.rank;
            curr=curr->next;
        } /* end while */
    } /* end else */

    FUNC_LEAVE (ret_value);
} /* end H5S_hyper_select_serial_size() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_hyper_select_serialize(space, buf)
        H5S_t *space;           IN: Dataspace pointer of selection to serialize
        uint8 *buf;             OUT: Buffer to put serialized selection into
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Serializes the current element selection into a buffer.  (Primarily for
    storing on disk).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_select_serialize (const H5S_t *space, uint8_t *buf)
{
    H5S_hyper_dim_t *diminfo;               /* Alias for dataspace's diminfo information */
    hsize_t tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary hyperslab counts */
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset of element in dataspace */
    hssize_t temp_off;            /* Offset in a given dimension */
    H5S_hyper_node_t *curr;     /* Hyperslab information nodes */
    uint8_t *lenp;          /* pointer to length location for later storage */
    uint32_t len=0;         /* number of bytes used */
    int i;                 /* local counting variable */
    unsigned u;                /* local counting variable */
    hssize_t block_count;       /* block counter for regular hyperslabs */
    int fast_dim;      /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;      /* Temporary rank holder */
    int ndims;         /* Rank of the dataspace */
    int done;          /* Whether we are done with the iteration */
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_point_select_serialize, FAIL);

    assert(space);

    /* Store the preamble information */
    UINT32ENCODE(buf, (uint32_t)space->select.type);  /* Store the type of selection */
    UINT32ENCODE(buf, (uint32_t)1);  /* Store the version number */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the un-used padding */
    lenp=buf;           /* keep the pointer to the length location for later */
    buf+=4;             /* skip over space for length */

    /* Encode number of dimensions */
    UINT32ENCODE(buf, (uint32_t)space->extent.u.simple.rank);
    len+=4;

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo != NULL) {
        /* Set some convienence values */
        ndims=space->extent.u.simple.rank;
        fast_dim=ndims-1;
        diminfo=space->select.sel_info.hslab.diminfo;

#ifdef QAK
    printf("%s: Serializing regular selection\n",FUNC);
    for(i=0; i<ndims; i++)
        printf("%s: (%d) start=%d, stride=%d, count=%d, block=%d\n",FUNC,i,(int)diminfo[i].start,(int)diminfo[i].stride,(int)diminfo[i].count,(int)diminfo[i].block);
#endif /*QAK */
        /* Check each dimension */
        for(block_count=1,i=0; i<ndims; i++)
            block_count*=diminfo[i].count;
#ifdef QAK
printf("%s: block_count=%d\n",FUNC,(int)block_count);
#endif /*QAK */

        /* Encode number of hyperslabs */
        UINT32ENCODE(buf, (uint32_t)block_count);
        len+=4;

        /* Now serialize the information for the regular hyperslab */

        /* Build the tables of count sizes as well as the initial offset */
        for(i=0; i<ndims; i++) {
            tmp_count[i]=diminfo[i].count;
            offset[i]=diminfo[i].start;
        } /* end for */

        /* We're not done with the iteration */
        done=0;

        /* Go iterate over the hyperslabs */
        while(done==0) {
            /* Iterate over the blocks in the fastest dimension */
            while(tmp_count[fast_dim]>0) {
                /* Add 8 bytes times the rank for each hyperslab selected */
                len+=8*ndims;

#ifdef QAK
for(i=0; i<ndims; i++)
    printf("%s: offset(%d)=%d\n",FUNC,i,(int)offset[i]);
#endif /*QAK */
                /* Encode hyperslab starting location */
                for(i=0; i<ndims; i++)
                    UINT32ENCODE(buf, (uint32_t)offset[i]);

#ifdef QAK
for(i=0; i<ndims; i++)
    printf("%s: offset+block-1(%d)=%d\n",FUNC,i,(int)(offset[i]+(diminfo[i].block-1)));
#endif /*QAK */
                /* Encode hyperslab ending location */
                for(i=0; i<ndims; i++)
                    UINT32ENCODE(buf, (uint32_t)(offset[i]+(diminfo[i].block-1)));

                /* Move the offset to the next sequence to start */
                offset[fast_dim]+=diminfo[fast_dim].stride;

                /* Decrement the block count */
                tmp_count[fast_dim]--;
            } /* end while */

            /* Work on other dimensions if necessary */
            if(fast_dim>0) {
                /* Reset the block counts */
                tmp_count[fast_dim]=diminfo[fast_dim].count;

                /* Bubble up the decrement to the slower changing dimensions */
                temp_dim=fast_dim-1;
                while(temp_dim>=0 && done==0) {
                    /* Decrement the block count */
                    tmp_count[temp_dim]--;

                    /* Check if we have more blocks left */
                    if(tmp_count[temp_dim]>0)
                        break;

                    /* Check for getting out of iterator */
                    if(temp_dim==0)
                        done=1;

                    /* Reset the block count in this dimension */
                    tmp_count[temp_dim]=diminfo[temp_dim].count;
                
                    /* Wrapped a dimension, go up to next dimension */
                    temp_dim--;
                } /* end while */
            } /* end if */

            /* Re-compute offset array */
            for(i=0; i<ndims; i++) {
                temp_off=diminfo[i].start
                    +diminfo[i].stride*(diminfo[i].count-tmp_count[i]);
                offset[i]=temp_off;
            } /* end for */
        } /* end while */
    } /* end if */
    else {
        /* Encode number of hyperslabs */
        UINT32ENCODE(buf, (uint32_t)space->select.sel_info.hslab.hyper_lst->count);
        len+=4;

        /* Encode each hyperslab in selection */
        curr=space->select.sel_info.hslab.hyper_lst->head;
        while(curr!=NULL) {
            /* Add 8 bytes times the rank for each hyperslab selected */
            len+=8*space->extent.u.simple.rank;

            /* Encode starting point */
            for(u=0; u<space->extent.u.simple.rank; u++)
                UINT32ENCODE(buf, (uint32_t)curr->start[u]);

            /* Encode ending point */
            for(u=0; u<space->extent.u.simple.rank; u++)
                UINT32ENCODE(buf, (uint32_t)curr->end[u]);

            curr=curr->next;
        } /* end while */
    } /* end else */

    /* Encode length */
    UINT32ENCODE(lenp, (uint32_t)len);  /* Store the length of the extra information */
    
    /* Set success */
    ret_value=SUCCEED;

    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_select_serialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_hyper_select_deserialize(space, buf)
        H5S_t *space;           IN/OUT: Dataspace pointer to place selection into
        uint8 *buf;             IN: Buffer to retrieve serialized selection from
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Deserializes the current selection into a buffer.  (Primarily for retrieving
    from disk).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_select_deserialize (H5S_t *space, const uint8_t *buf)
{
    uint32_t rank;              /* rank of points */
    size_t num_elem=0;          /* number of elements in selection */
    hssize_t *start=NULL;       /* hyperslab start information */
    hssize_t *end=NULL;     /* hyperslab end information */
    hsize_t *count=NULL;        /* hyperslab count information */
    hsize_t *block=NULL;        /* hyperslab block information */
    hssize_t *tstart=NULL;      /* temporary hyperslab pointers */
    hssize_t *tend=NULL;        /* temporary hyperslab pointers */
    hsize_t *tcount=NULL;       /* temporary hyperslab pointers */
    hsize_t *tblock=NULL;       /* temporary hyperslab pointers */
    unsigned i,j;               /* local counting variables */
    herr_t ret_value=FAIL;      /* return value */

    FUNC_ENTER (H5S_hyper_select_deserialize, FAIL);

    /* Check args */
    assert(space);
    assert(buf);

    /* Deserialize slabs to select */
    buf+=16;    /* Skip over selection header */
    UINT32DECODE(buf,rank);  /* decode the rank of the point selection */
    if(rank!=space->extent.u.simple.rank)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "rank of pointer does not match dataspace");
    UINT32DECODE(buf,num_elem);  /* decode the number of points */

    /* Allocate space for the coordinates */
    if((start = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab information");
    if((end = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab information");
    if((block = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab information");
    if((count = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab information");
    
    /* Set the count for all blocks */
    for(tcount=count,j=0; j<rank; j++,tcount++)
        *tcount=1;

    /* Retrieve the coordinates from the buffer */
    for(i=0; i<num_elem; i++) {
        /* Decode the starting points */
        for(tstart=start,j=0; j<rank; j++,tstart++)
            UINT32DECODE(buf, *tstart);

        /* Decode the ending points */
        for(tend=end,j=0; j<rank; j++,tend++)
            UINT32DECODE(buf, *tend);

        /* Change the ending points into blocks */
        for(tblock=block,tstart=start,tend=end,j=0; j<(unsigned)rank; j++,tstart++,tend++,tblock++)
            *tblock=(*tend-*tstart)+1;

        /* Select or add the hyperslab to the current selection */
        if((ret_value=H5S_select_hyperslab(space,(i==0 ? H5S_SELECT_SET : H5S_SELECT_OR),start,NULL,count,block))<0) {
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");
        } /* end if */
    } /* end for */

    /* Free temporary buffers */
    H5FL_ARR_FREE(hsize_t,start);
    H5FL_ARR_FREE(hsize_t,end);
    H5FL_ARR_FREE(hsize_t,count);
    H5FL_ARR_FREE(hsize_t,block);

done:
    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_select_deserialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_hyper_bounds(space, hsize_t *start, hsize_t *end)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hsize_t *start;         OUT: Starting coordinate of bounding box
        hsize_t *end;           OUT: Opposite coordinate of bounding box
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Retrieves the bounding box containing the current selection and places
    it into the user's buffers.  The start and end buffers must be large
    enough to hold the dataspace rank number of coordinates.  The bounding box
    exactly contains the selection, ie. if a 2-D element selection is currently
    defined with the following points: (4,5), (6,8) (10,7), the bounding box
    with be (4, 5), (10, 8).
        The bounding box calculations _does_ include the current offset of the
    selection within the dataspace extent.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_bounds(H5S_t *space, hsize_t *start, hsize_t *end)
{
    H5S_hyper_node_t *node;     /* Hyperslab node */
    int rank;                  /* Dataspace rank */
    int i;                     /* index variable */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER (H5S_hyper_bounds, FAIL);

    assert(space);
    assert(start);
    assert(end);

    /* Get the dataspace extent rank */
    rank=space->extent.u.simple.rank;

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo!=NULL) {
        const H5S_hyper_dim_t *diminfo=space->select.sel_info.hslab.diminfo; /* local alias for diminfo */

        /* Check each dimension */
        for(i=0; i<rank; i++) {
            /* Compute the smallest location in this dimension */
            start[i]=diminfo[i].start+space->select.offset[i];

            /* Compute the largest location in this dimension */
            end[i]=diminfo[i].start+diminfo[i].stride*(diminfo[i].count-1)+(diminfo[i].block-1)+space->select.offset[i];
        } /* end for */
    } /* end if */
    else {
        /* Iterate through the node, copying each hyperslab's information */
        node=space->select.sel_info.hslab.hyper_lst->head;
        while(node!=NULL) {
            for(i=0; i<rank; i++) {
                if(start[i]>(hsize_t)(node->start[i]+space->select.offset[i]))
                    start[i]=node->start[i]+space->select.offset[i];
                if(end[i]<(hsize_t)(node->end[i]+space->select.offset[i]))
                    end[i]=node->end[i]+space->select.offset[i];
            } /* end for */
            node=node->next;
          } /* end while */
    } /* end if */

    FUNC_LEAVE (ret_value);
}   /* H5Sget_hyper_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_contiguous
 PURPOSE
    Check if a hyperslab selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_select_contiguous(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is contiguous.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_hyper_select_contiguous(const H5S_t *space)
{
    htri_t ret_value=FAIL;  /* return value */
    H5S_hyper_node_t *node;     /* Hyperslab node */
    unsigned rank;                 /* Dataspace rank */
    unsigned u;                    /* index variable */

    FUNC_ENTER (H5S_hyper_select_contiguous, FAIL);

    assert(space);

    /* Check for a "regular" hyperslab selection */
    if(space->select.sel_info.hslab.diminfo != NULL) {
        /*
         * For a regular hyperslab to be contiguous, it must have only one
         * block (i.e. count==1 in all dimensions) and the block size must be
         * the same as the dataspace extent's in all but the slowest changing
         * dimension.
         */
        ret_value=TRUE; /* assume true and reset if the dimensions don't match */
        for(u=1; u<space->extent.u.simple.rank; u++) {
            if(space->select.sel_info.hslab.diminfo[u].count>1 || space->select.sel_info.hslab.diminfo[u].block!=space->extent.u.simple.size[u]) {
                ret_value=FALSE;
                break;
            } /* end if */
        } /* end for */
    } /* end if */
    else {
        /* If there is more than one hyperslab in the selection, they are not contiguous */
        if(space->select.sel_info.hslab.hyper_lst->count>1)
            ret_value=FALSE;
        else {  /* If there is one hyperslab, then it might be contiguous */
            /* Get the dataspace extent rank */
            rank=space->extent.u.simple.rank;

            /* Get the hyperslab node */
            node=space->select.sel_info.hslab.hyper_lst->head;

            /*
             * For a hyperslab to be contiguous, it's size must be the same as the
             * dataspace extent's in all but the slowest changing dimension
             */
            ret_value=TRUE;     /* assume true and reset if the dimensions don't match */
            for(u=1; u<rank; u++) {
                if(((node->end[u]-node->start[u])+1)!=(hssize_t)space->extent.u.simple.size[u]) {
                    ret_value=FALSE;
                    break;
                } /* end if */
            } /* end for */
        } /* end else */
    } /* end else */
    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_select_contiguous() */


/*-------------------------------------------------------------------------
 * Function:    H5S_generate_hyperlab
 *
 * Purpose:     Generate hyperslab information from H5S_select_hyperslab()
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol (split from HS_select_hyperslab()).
 *              Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_generate_hyperslab (H5S_t *space, H5S_seloper_t op,
                      const hssize_t start[/*space_id*/],
                      const hsize_t stride[/*space_id*/],
                      const hsize_t count[/*space_id*/],
                      const hsize_t block[/*space_id*/])
{
    hssize_t slab[H5O_LAYOUT_NDIMS]; /* Location of the block to add for strided selections */
    size_t slice[H5O_LAYOUT_NDIMS];      /* Size of preceding dimension's slice */
    H5S_hyper_node_t *add=NULL, /* List of hyperslab nodes to add */
        *uniq=NULL;         /* List of unique hyperslab nodes */
    unsigned acc;                /* Accumulator for building slices */
    unsigned contig;             /* whether selection is contiguous or not */
    int i;                   /* Counters */
    unsigned u,v;                /* Counters */
    herr_t ret_value=FAIL;    /* return value */

    FUNC_ENTER (H5S_generate_hyperslab, FAIL);

    /* Check args */
    assert(block);
    assert(stride);
    assert(space);
    assert(start);
    assert(count);
    assert(op>H5S_SELECT_NOOP && op<H5S_SELECT_INVALID);
    
    /* Determine if selection is contiguous */
    /* assume hyperslab is contiguous, until proven otherwise */
    contig=1;
    for(u=0; u<space->extent.u.simple.rank; u++) {
        /* contiguous hyperslabs have the block size equal to the stride */
        if(stride[u]!=block[u]) {
            contig=0;   /* hyperslab isn't contiguous */
            break;      /* no use looking further */
        } /* end if */
    } /* end for */

#ifdef QAK
    printf("%s: check 1.0, contig=%d, op=%s\n",FUNC,(int)contig,(op==H5S_SELECT_SET? "H5S_SELECT_SET" : (op==H5S_SELECT_OR ? "H5S_SELECT_OR" : "Unknown")));
#endif /* QAK */

#ifdef QAK
    printf("%s: check 2.0, rank=%d\n",FUNC,(int)space->extent.u.simple.rank);
#endif /* QAK */
    /* Allocate space for the hyperslab selection information if necessary */
    if(space->select.type!=H5S_SEL_HYPERSLABS || space->select.sel_info.hslab.hyper_lst==NULL) {
        if((space->select.sel_info.hslab.hyper_lst = H5FL_ALLOC(H5S_hyper_list_t,0))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab information");

        /* Set the fields for the hyperslab list */
        space->select.sel_info.hslab.hyper_lst->count=0;
        space->select.sel_info.hslab.hyper_lst->head=NULL;
        if((space->select.sel_info.hslab.hyper_lst->lo_bounds = H5FL_ARR_ALLOC(H5S_hyper_bound_ptr_t,(hsize_t)space->extent.u.simple.rank,1))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate hyperslab lo bound information");
    } /* end if */

#ifdef QAK
    printf("%s: check 3.0\n",FUNC);
#endif /* QAK */
    /* Generate list of blocks to add/remove based on selection operation */
    switch(op) {
        case H5S_SELECT_SET:
        case H5S_SELECT_OR:
#ifdef QAK
    printf("%s: check 4.0\n",FUNC);
#endif /* QAK */
            /* Generate list of blocks to add to selection */
            if(contig) { /* Check for trivial case */
#ifdef QAK
    printf("%s: check 4.1\n",FUNC);
#endif /* QAK */

                /* Account for strides & blocks being equal, but larger than one */
                /* (Why someone would torture us this way, I don't know... -QAK :-) */
                for(u=0; u<space->extent.u.simple.rank; u++)
                    slab[u]=count[u]*stride[u];
#ifdef QAK
    printf("%s: check 4.2\n",FUNC);
    printf("%s: start = {",FUNC);
    for(u=0; u<space->extent.u.simple.rank; u++) {
        printf("%d",(int)start[u]);
        if(u<(space->extent.u.simple.rank-1))
            printf(", ");
        else
            printf("}\n");
    }
    printf("%s: slab = {",FUNC);
    for(u=0; u<space->extent.u.simple.rank; u++) {
        printf("%d",(int)slab[u]);
        if(u<(space->extent.u.simple.rank-1))
            printf(", ");
        else
            printf("}\n");
    }
#endif /* QAK */

                /* Add the contiguous hyperslab to the selection */
                if(H5S_hyper_node_add(&add,0,space->extent.u.simple.rank,start,(const hsize_t *)slab)<0) {
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslab");
                }
            } else {
#ifdef QAK
    printf("%s: check 4.3\n",FUNC);
#endif /* QAK */
                /* Build the slice sizes for each dimension */
                for(u=0, acc=1; u<space->extent.u.simple.rank; u++) {
                    slice[u]=acc;
                    acc*=count[u];
                } /* end for */

                /* Step through all the blocks to add */
                /* (reuse the count in ACC above) */
                /* Adding the blocks in reverse order reduces the time spent moving memory around in H5S_hyper_add() */
                for(i=(int)acc-1; i>=0; i--) {
                    /* Build the location of the block */
                    for(v=0; v<space->extent.u.simple.rank; v++)
                        slab[v]=start[v]+((i/slice[v])%count[v])*stride[v];
                    
                    /* Add the block to the list of hyperslab selections */
                    if(H5S_hyper_node_add(&add,0,space->extent.u.simple.rank,(const hssize_t *)slab, (const hsize_t *)block)<0) {
                        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslab");
                    } /* end if */
                } /* end for */
            } /* end else */

#ifdef QAK
    printf("%s: check 4.5\n",FUNC);
#endif /* QAK */
            /* Clip list of new blocks to add against current selection */
            if(op==H5S_SELECT_OR) {
#ifdef QAK
    printf("%s: check 4.5.1\n",FUNC);
#endif /* QAK */
                H5S_hyper_clip(space,add,&uniq,NULL);
                add=uniq;
            } /* end if */
#ifdef QAK
    printf("%s: check 4.5.5\n",FUNC);
#endif /* QAK */
            break;

        default:
            HRETURN_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "invalid selection operation");
    } /* end switch */

#ifdef QAK
    printf("%s: check 5.0\n",FUNC);
#endif /* QAK */
    /* Add new blocks to current selection */
    if(H5S_hyper_add(space,add)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert hyperslabs");

    /* Merge blocks for better I/O performance */
    /* Regenerate lo/hi bounds arrays? */

#ifdef QAK
    printf("%s: check 6.0\n",FUNC);
#endif /* QAK */

    /* Set return value */
    ret_value=SUCCEED;

done:
    FUNC_LEAVE (ret_value);
} /* end H5S_generate_hyperslab() */


/*-------------------------------------------------------------------------
 * Function:    H5S_select_hyperslab
 *
 * Purpose:     Internal version of H5Sselect_hyperslab().
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke (split from HSselect_hyperslab()).
 *              Tuesday, August 25, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_select_hyperslab (H5S_t *space, H5S_seloper_t op,
                      const hssize_t start[/*space_id*/],
                      const hsize_t stride[/*space_id*/],
                      const hsize_t count[/*space_id*/],
                      const hsize_t block[/*space_id*/])
{
    hsize_t *_stride=NULL;      /* Stride array */
    hsize_t *_block=NULL;       /* Block size array */
    unsigned u;                    /* Counters */
    H5S_hyper_dim_t *diminfo; /* per-dimension info for the selection */
    herr_t ret_value=FAIL;    /* return value */

    FUNC_ENTER (H5S_select_hyperslab, FAIL);

    /* Check args */
    assert(space);
    assert(start);
    assert(count);
    assert(op>H5S_SELECT_NOOP && op<H5S_SELECT_INVALID);
    
    /* Fill in the correct stride values */
    if(stride==NULL) {
        hssize_t fill=1;

        /* Allocate temporary buffer */
        if ((_stride=H5FL_ARR_ALLOC(hsize_t,(hsize_t)space->extent.u.simple.rank,0))==NULL)
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed for stride buffer");
        H5V_array_fill(_stride,&fill,sizeof(hssize_t),space->extent.u.simple.rank);
        stride = _stride;
    }

    /* Fill in the correct block values */
    if(block==NULL) {
        hssize_t fill=1;

        /* Allocate temporary buffer */
        if ((_block=H5FL_ARR_ALLOC(hsize_t,(hsize_t)space->extent.u.simple.rank,0))==NULL)
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed for stride buffer");
        H5V_array_fill(_block,&fill,sizeof(hssize_t),space->extent.u.simple.rank);
        block = _block;
    }

    /* Fixup operation if selection is 'none' and operation is an OR */
    /* (Allows for 'or'ing a sequence of hyperslab into a 'none' selection to */
    /* have same affect as setting the first hyperslab in the sequence to have */
    /* the 'set' operation and the rest of the hyperslab sequence to be 'or'ed */
    /* after that */
    if(space->select.type==H5S_SEL_NONE && op==H5S_SELECT_OR)
        op=H5S_SELECT_SET;

#ifdef QAK
    printf("%s: check 1.0, op=%s\n",FUNC,(op==H5S_SELECT_SET? "H5S_SELECT_SET" : (op==H5S_SELECT_OR ? "H5S_SELECT_OR" : "Unknown")));
#endif /* QAK */
    if(op==H5S_SELECT_SET) {
        /*
         * Check for overlapping hyperslab blocks in new selection
         *  (remove when real block-merging algorithm is in place? -QAK).
         */
#ifdef QAK
for(u=0; u<space->extent.u.simple.rank; u++)
    printf("%s: (%u) start=%d, stride=%d, count=%d, block=%d\n",FUNC,u,(int)start[u],(int)stride[u],(int)count[u],(int)block[u]);
#endif /* QAK */
        for(u=0; u<space->extent.u.simple.rank; u++) {
            if(count[u]>1 && stride[u]<block[u]) {
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "hyperslab blocks overlap");
            } /* end if */
        } /* end for */

        /* If we are setting a new selection, remove current selection first */
        if(H5S_select_release(space)<0) {
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL,
                "can't release hyperslab");
        } /* end if */

        /* Copy all the application per-dimension selection info into the space descriptor */
        if((diminfo = H5FL_ARR_ALLOC(H5S_hyper_dim_t,(hsize_t)space->extent.u.simple.rank,0))==NULL) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate per-dimension vector");
        } /* end if */
        for(u=0; u<space->extent.u.simple.rank; u++) {
            diminfo[u].start = start[u];
            diminfo[u].stride = stride[u];
            diminfo[u].count = count[u];
            diminfo[u].block = block[u];
        } /* end for */
        space->select.sel_info.hslab.app_diminfo = diminfo;

        /* Allocate room for the optimized per-dimension selection info */
        if((diminfo = H5FL_ARR_ALLOC(H5S_hyper_dim_t,(hsize_t)space->extent.u.simple.rank,0))==NULL) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate per-dimension vector");
        } /* end if */

        /* Optimize the hyperslab selection to detect contiguously selected block/stride information */
        /* Modify the stride, block & count for contiguous hyperslab selections */
        for(u=0; u<space->extent.u.simple.rank; u++) {
            /* Starting location doesn't get optimized */
            diminfo[u].start = start[u];

            /* contiguous hyperslabs have the block size equal to the stride */
            if(stride[u]==block[u]) {
                diminfo[u].stride=1;
                diminfo[u].count=1;
                diminfo[u].block=count[u]*block[u];
            } /* end if */
            else {
                diminfo[u].stride=stride[u];
                diminfo[u].count=count[u];
                diminfo[u].block=block[u];
            } /* end else */
        } /* end for */
        space->select.sel_info.hslab.diminfo = diminfo;

        /* Set the number of elements in the hyperslab selection */
        for(space->select.num_elem=1,u=0; u<space->extent.u.simple.rank; u++)
            space->select.num_elem*=block[u]*count[u];
    } /* end if */
    else if(op==H5S_SELECT_OR) {
        switch(space->select.type) {
            case H5S_SEL_ALL:
                /* break out now, 'or'ing with an all selection leaves the all selection */
                HGOTO_DONE(SUCCEED);

            case H5S_SEL_HYPERSLABS:
                /* Is this the first 'or' operation? */
                if(space->select.sel_info.hslab.diminfo != NULL) {
                    /* yes, a "regular" hyperslab is selected currently */

                    hssize_t tmp_start[H5O_LAYOUT_NDIMS];
                    hsize_t tmp_stride[H5O_LAYOUT_NDIMS];
                    hsize_t tmp_count[H5O_LAYOUT_NDIMS];
                    hsize_t tmp_block[H5O_LAYOUT_NDIMS];

                    /* Generate the hyperslab information for the regular hyperslab */

                    /* Copy over the 'diminfo' information */
                    for(u=0; u<space->extent.u.simple.rank; u++) {
                        tmp_start[u]=space->select.sel_info.hslab.diminfo[u].start;
                        tmp_stride[u]=space->select.sel_info.hslab.diminfo[u].stride;
                        tmp_count[u]=space->select.sel_info.hslab.diminfo[u].count;
                        tmp_block[u]=space->select.sel_info.hslab.diminfo[u].block;
                    } /* end for */

                    /* Reset the number of selection elements */
                    space->select.num_elem=0;

                    /* Build the hyperslab information */
                    H5S_generate_hyperslab (space, H5S_SELECT_SET, tmp_start, tmp_stride, tmp_count, tmp_block);

                    /* Remove the 'diminfo' information, since we're adding to it */
                    H5FL_ARR_FREE(H5S_hyper_dim_t,space->select.sel_info.hslab.diminfo);
                    space->select.sel_info.hslab.diminfo = NULL;

                    /* Remove the 'app_diminfo' information also, since we're adding to it */
                    H5FL_ARR_FREE(H5S_hyper_dim_t,space->select.sel_info.hslab.app_diminfo);
                    space->select.sel_info.hslab.app_diminfo = NULL;

                    /* Add in the new hyperslab information */
                    H5S_generate_hyperslab (space, op, start, stride, count, block);
                } /* end if */
                else {
                    /* nope, an "irregular" hyperslab is selected currently */
                    /* Add in the new hyperslab information */
                    H5S_generate_hyperslab (space, op, start, stride, count, block);
                } /* end else */
                break;

            default:
                HRETURN_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "invalid selection operation");
        } /* end switch() */
    } /* end if */
    else {
        HRETURN_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "invalid selection operation");
    } /* end else */

    /* Set selection type */
    space->select.type=H5S_SEL_HYPERSLABS;

    ret_value=SUCCEED;

done:
    if(_stride!=NULL)
        H5FL_ARR_FREE(hsize_t,_stride);
    if(_block!=NULL)
        H5FL_ARR_FREE(hsize_t,_block);
    FUNC_LEAVE (ret_value);
}   /* end H5S_select_hyperslab() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_hyperslab
 PURPOSE
    Specify a hyperslab to combine with the current hyperslab selection
 USAGE
    herr_t H5Sselect_hyperslab(dsid, op, start, stride, count, block)
        hid_t dsid;             IN: Dataspace ID of selection to modify
        H5S_seloper_t op;       IN: Operation to perform on current selection
        const hssize_t *start;        IN: Offset of start of hyperslab
        const hssize_t *stride;       IN: Hyperslab stride
        const hssize_t *count;        IN: Number of blocks included in hyperslab
        const hssize_t *block;        IN: Size of block in hyperslab
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Combines a hyperslab selection with the current selection for a dataspace.
    If the current selection is not a hyperslab, it is freed and the hyperslab
    parameters passed in are combined with the H5S_SEL_ALL hyperslab (ie. a
    selection composing the entire current extent).  Currently, only the
    H5S_SELECT_SET & H5S_SELECT_OR operations are supported.  If STRIDE or
    BLOCK is NULL, they are assumed to be set to all '1'.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sselect_hyperslab(hid_t space_id, H5S_seloper_t op,
                     const hssize_t start[/*space_id*/],
                     const hsize_t _stride[/*space_id*/],
                     const hsize_t count[/*space_id*/],
                     const hsize_t _block[/*space_id*/])
{
    H5S_t       *space = NULL;  /* Dataspace to modify selection of */

    FUNC_ENTER (H5Sselect_hyperslab, FAIL);
    H5TRACE6("e","iSs*[a0]Hs*[a0]h*[a0]h*[a0]h",space_id,op,start,_stride,
             count,_block);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) ||
            NULL == (space=H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if(start==NULL || count==NULL) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "hyperslab not specified");
    } /* end if */

    if(!(op>H5S_SELECT_NOOP && op<H5S_SELECT_INVALID)) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "invalid selection operation");
    } /* end if */

    if (H5S_select_hyperslab(space, op, start, _stride, count, _block)<0) {
        HRETURN_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL,
                      "unable to set hyperslab selection");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_hyper_select_iterate_mem
 *
 * Purpose:     Recursively iterates over data points in memory using the parameters
 *      passed to H5S_hyper_select_iterate.
 *
 * Return:      Success:        Number of elements copied.
 *
 *              Failure:        0
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, June 22, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_hyper_select_iterate_mem (int dim, H5S_hyper_iter_info_t *iter_info)
{
    hsize_t offset;             /* offset of region in buffer */
    void *tmp_buf;              /* temporary location of the element in the buffer */
    H5S_hyper_region_t *regions;  /* Pointer to array of hyperslab nodes overlapped */
    size_t num_regions;         /* number of regions overlapped */
    herr_t user_ret=0;          /* User's return value */
    size_t i;                   /* Counters */
    int j;

    FUNC_ENTER (H5S_hyper_select_iterate_mem, 0);

    assert(iter_info);

    /* Get a sorted list (in the next dimension down) of the regions which */
    /*  overlap the current index in this dim */
    if((regions=H5S_hyper_get_regions(&num_regions,iter_info->space->extent.u.simple.rank,
            (unsigned)(dim+1),
            iter_info->space->select.sel_info.hslab.hyper_lst->count,
            iter_info->space->select.sel_info.hslab.hyper_lst->lo_bounds,
            iter_info->iter->hyp.pos,iter_info->space->select.offset))!=NULL) {

        /* Check if this is the second to last dimension in dataset */
        /*  (Which means that we've got a list of the regions in the fastest */
        /*   changing dimension and should input those regions) */
        if((unsigned)(dim+2)==iter_info->space->extent.u.simple.rank) {
            HDmemcpy(iter_info->mem_offset, iter_info->iter->hyp.pos,(iter_info->space->extent.u.simple.rank*sizeof(hssize_t)));
            iter_info->mem_offset[iter_info->space->extent.u.simple.rank]=0;

            /* Iterate over data from regions */
            for(i=0; i<num_regions && user_ret==0; i++) {
                /* Set the location of the current hyperslab */
                iter_info->mem_offset[iter_info->space->extent.u.simple.rank-1]=regions[i].start;

                /* Get the offset in the memory buffer */
                offset=H5V_array_offset(iter_info->space->extent.u.simple.rank+1,
                    iter_info->mem_size,iter_info->mem_offset);
                tmp_buf=((char *)iter_info->src+offset);

                /* Iterate over each element in the current region */
                for(j=regions[i].start; j<=regions[i].end && user_ret==0; j++) {
                    /* Call the user's function */
                    user_ret=(*(iter_info->op))(tmp_buf,iter_info->dt,(hsize_t)iter_info->space->extent.u.simple.rank,iter_info->mem_offset,iter_info->op_data);

                    /* Subtract the element from the selected region (not implemented yet) */

                    /* Increment the coordinate offset */
                    iter_info->mem_offset[iter_info->space->extent.u.simple.rank-1]=j;

                    /* Advance the pointer in the buffer */
                    tmp_buf=((char *)tmp_buf+iter_info->elem_size);
                } /* end for */

                /* Decrement the iterator count */
                iter_info->iter->hyp.elmt_left-=((regions[i].end-regions[i].start)+1);
            } /* end for */

            /* Set the next position to start at */
            iter_info->iter->hyp.pos[dim+1]=(-1);
        } else { /* recurse on each region to next dimension down */

            /* Increment the dimension we are working with */
            dim++;

            /* Step through each region in this dimension */
            for(i=0; i<num_regions && user_ret==0; i++) {
                /* Step through each location in each region */
                for(j=regions[i].start; j<=regions[i].end && user_ret==0; j++) {

                    /*
                     * If we are moving to a new position in this dim, reset
                     * the next lower dim. location.
                     */
                    if(iter_info->iter->hyp.pos[dim]!=j)
                        iter_info->iter->hyp.pos[dim+1]=(-1);

                    /* Set the correct position we are working on */
                    iter_info->iter->hyp.pos[dim]=j;

                    /* Go get the regions in the next lower dimension */
                    user_ret=H5S_hyper_select_iterate_mem(dim, iter_info);
                } /* end for */
            } /* end for */
        } /* end else */

        /* Release the temporary buffer */
        H5FL_ARR_FREE(H5S_hyper_region_t,regions);
    } /* end if */

    FUNC_LEAVE (user_ret);
}   /* H5S_hyper_select_iterate_mem() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_iterate_mem_opt
 PURPOSE
    Iterate over the data points in a regular hyperslab selection, calling a
    user's function for each element.
 USAGE
    herr_t H5S_hyper_select_iterate_mem_opt(buf, type_id, space, op, operator_data)
        H5S_sel_iter_t *iter;   IN/OUT: Selection iterator
        void *buf;      IN/OUT: Buffer containing elements to iterate over
        hid_t type_id;  IN: Datatype ID of BUF array.
        H5S_t *space;   IN: Dataspace object containing selection to iterate over
        H5D_operator_t op; IN: Function pointer to the routine to be
                                called for each element in BUF iterated over.
        void *op_data;  IN/OUT: Pointer to any user-defined data associated
                                with the operation.
 RETURNS
    Returns the return value of the last operator if it was non-zero, or zero
    if all elements were processed. Otherwise returns a negative value.
 DESCRIPTION
    Iterates over the selected elements in a memory buffer, calling the user's
    callback function for each element.  The selection in the dataspace is
    modified so that any elements already iterated over are removed from the
    selection if the iteration is interrupted (by the H5D_operator_t function
    returning non-zero) in the "middle" of the iteration and may be re-started
    by the user where it left off.

    NOTE: Until "subtracting" elements from a selection is implemented,
        the selection is not modified.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_hyper_select_iterate_mem_opt(H5S_sel_iter_t UNUSED *iter, void *buf, hid_t type_id, H5S_t *space, H5D_operator_t op,
        void *op_data)
{
    H5S_hyper_dim_t *diminfo;               /* Alias for dataspace's diminfo information */
    hsize_t tmp_count[H5O_LAYOUT_NDIMS];    /* Temporary hyperslab counts */
    hsize_t tmp_block[H5O_LAYOUT_NDIMS];    /* Temporary hyperslab blocks */
    hssize_t offset[H5O_LAYOUT_NDIMS];      /* Offset of element in dataspace */
    hsize_t slab[H5O_LAYOUT_NDIMS];         /* Size of objects in buffer */
    size_t elem_size;           /* Size of data element in buffer */
    hssize_t temp_off;            /* Offset in a given dimension */
    uint8_t *loc;               /* Current element location */
    int i;                     /* Counter */
    unsigned u;                    /* Counter */
    int fast_dim;      /* Rank of the fastest changing dimension for the dataspace */
    int temp_dim;      /* Temporary rank holder */
    unsigned ndims;        /* Rank of the dataspace */
    herr_t user_ret=0;          /* User's return value */

    FUNC_ENTER (H5S_hyper_select_iterate_mem_opt, FAIL);

    /* Set some convienence values */
    ndims=space->extent.u.simple.rank;
    fast_dim=ndims-1;
    diminfo=space->select.sel_info.hslab.diminfo;

    /* Get the data element size */
    elem_size=H5Tget_size(type_id);

    /* Elements in the fastest dimension are 'elem_size' */
    slab[ndims-1]=elem_size;

    /* If we have two or more dimensions, build the other dimension's element sizes */
    if(ndims>=2) {
        /* Build the table of next-dimension down 'element' sizes */
        for(i=ndims-2; i>=0; i--)
            slab[i]=slab[i+1]*space->extent.u.simple.size[i+1];
    } /* end if */

    /* Build the tables of count & block sizes as well as the initial offset */
    for(u=0; u<ndims; u++) {
        tmp_count[u]=diminfo[u].count;
        tmp_block[u]=diminfo[u].block;
        offset[u]=diminfo[u].start;
    } /* end for */

    /* Initialize the starting location */
    for(loc=buf,u=0; u<ndims; u++)
        loc+=diminfo[u].start*slab[u];

    /* Go iterate over the hyperslabs */
    while(user_ret==0) {
        /* Iterate over the blocks in the fastest dimension */
        while(tmp_count[fast_dim]>0 && user_ret==0) {

            /* Iterate over the elements in the fastest dimension */
            while(tmp_block[fast_dim]>0 && user_ret==0) {
                user_ret=(*op)(loc,type_id,(hsize_t)ndims,offset,op_data);

                /* Increment the buffer location */
                loc+=slab[fast_dim];

                /* Increment the offset in the dataspace */
                offset[fast_dim]++;

                /* Decrement the sequence count */
                tmp_block[fast_dim]--;
            } /* end while */

            /* Reset the sequence count */
            tmp_block[fast_dim]=diminfo[fast_dim].block;

            /* Move the location to the next sequence to start */
            loc+=(diminfo[fast_dim].stride-diminfo[fast_dim].block)*slab[fast_dim];
             
            /* Move the offset to the next sequence to start */
            offset[fast_dim]+=(diminfo[fast_dim].stride-diminfo[fast_dim].block);

            /* Decrement the block count */
            tmp_count[fast_dim]--;
        } /* end while */

        /* Check for getting out of iterator, we're done in the 1-D case */
        if(ndims==1)
            goto done; /* Yes, an evil goto.. :-) -QAK */

        /* Work on other dimensions if necessary */
        if(fast_dim>0 && user_ret==0) {
            /* Reset the sequence and block counts */
            tmp_block[fast_dim]=diminfo[fast_dim].block;
            tmp_count[fast_dim]=diminfo[fast_dim].count;

            /* Bubble up the decrement to the slower changing dimensions */
            temp_dim=fast_dim-1;
            while(temp_dim>=0) {
                /* Decrement the sequence count in this dimension */
                tmp_block[temp_dim]--;

                /* Check if we are still in the sequence */
                if(tmp_block[temp_dim]>0)
                    break;

                /* Reset the sequence count in this dimension */
                tmp_block[temp_dim]=diminfo[temp_dim].block;

                /* Decrement the block count */
                tmp_count[temp_dim]--;

                /* Check if we have more blocks left */
                if(tmp_count[temp_dim]>0)
                    break;

                /* Check for getting out of iterator */
                if(temp_dim==0)
                    goto done; /* Yes, an evil goto.. :-) -QAK */

                /* Reset the block count in this dimension */
                tmp_count[temp_dim]=diminfo[temp_dim].count;
            
                /* Wrapped a dimension, go up to next dimension */
                temp_dim--;
            } /* end while */
        } /* end if */

        /* Re-compute buffer location & offset array */
        for(loc=buf,u=0; u<ndims; u++) {
            temp_off=diminfo[u].start
                +diminfo[u].stride*(diminfo[u].count-tmp_count[u])
                    +(diminfo[u].block-tmp_block[u]);
            loc+=temp_off*slab[u];
            offset[u]=temp_off;
        } /* end for */
    } /* end while */

done:
    FUNC_LEAVE (user_ret);

    iter = 0;
} /* end H5S_hyper_select_iterate_mem_opt() */


/*--------------------------------------------------------------------------
 NAME
    H5S_hyper_select_iterate
 PURPOSE
    Iterate over a hyperslab selection, calling a user's function for each
        element.
 USAGE
    herr_t H5S_hyper_select_iterate(buf, type_id, space, op, operator_data)
        void *buf;      IN/OUT: Buffer containing elements to iterate over
        hid_t type_id;  IN: Datatype ID of BUF array.
        H5S_t *space;   IN: Dataspace object containing selection to iterate over
        H5D_operator_t op; IN: Function pointer to the routine to be
                                called for each element in BUF iterated over.
        void *operator_data;    IN/OUT: Pointer to any user-defined data
                                associated with the operation.
 RETURNS
    Returns the return value of the last operator if it was non-zero, or zero
    if all elements were processed. Otherwise returns a negative value.
 DESCRIPTION
    Iterates over the selected elements in a memory buffer, calling the user's
    callback function for each element.  The selection in the dataspace is
    modified so that any elements already iterated over are removed from the
    selection if the iteration is interrupted (by the H5D_operator_t function
    returning non-zero) in the "middle" of the iteration and may be re-started
    by the user where it left off.

    NOTE: Until "subtracting" elements from a selection is implemented,
        the selection is not modified.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_hyper_select_iterate(void *buf, hid_t type_id, H5S_t *space, H5D_operator_t op,
        void *operator_data)
{
    H5S_hyper_iter_info_t iter_info;  /* Block of parameters to pass into recursive calls */
    H5S_sel_iter_t      iter;   /* selection iteration info*/
    herr_t ret_value=FAIL;      /* return value */

    FUNC_ENTER (H5S_hyper_select_iterate, FAIL);

    assert(buf);
    assert(space);
    assert(op);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    /* Initialize the selection iterator */
    if (H5S_hyper_init(NULL, space, &iter)<0) {
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL,
                     "unable to initialize selection information");
    } 

    /* Check for the special case of just one H5Sselect_hyperslab call made */
    if(space->select.sel_info.hslab.diminfo!=NULL) {
        /* Use optimized call to iterate over regular hyperslab */
        ret_value=H5S_hyper_select_iterate_mem_opt(&iter,buf,type_id,space,op,operator_data);
    }
    else {
        /* Initialize parameter block for recursive calls */
        iter_info.dt=type_id;
        iter_info.elem_size=H5Tget_size(type_id);
        iter_info.space=space;
        iter_info.iter=&iter;
        iter_info.src=buf;

        /* Set up the size of the memory space */
        HDmemcpy(iter_info.mem_size, space->extent.u.simple.size, space->extent.u.simple.rank*sizeof(hsize_t));
        iter_info.mem_size[space->extent.u.simple.rank]=iter_info.elem_size;

        /* Copy the location of the region in the file */
        iter_info.op=op;
        iter_info.op_data=operator_data;

        /* Recursively input the hyperslabs currently defined */
        /* starting with the slowest changing dimension */
        ret_value=H5S_hyper_select_iterate_mem(-1,&iter_info);
    } /* end else */

    /* Release selection iterator */
    H5S_sel_iter_release(space,&iter);

done:
    FUNC_LEAVE (ret_value);
}   /* H5S_hyper_select_iterate() */

