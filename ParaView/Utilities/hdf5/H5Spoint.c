/*
 * Copyright (C) 1998-2001 NCSA
 *                         All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, June 16, 1998
 *
 * Purpose:     Point selection data space I/O functions.
 */

#define H5S_PACKAGE             /*suppress error about including H5Spkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Iprivate.h"
#include "H5MMprivate.h"
#include "H5Spkg.h"
#include "H5Vprivate.h"
#include "H5Dprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5Spoint_mask
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

static herr_t H5S_point_init (const struct H5O_layout_t *layout,
                              const H5S_t *space, H5S_sel_iter_t *iter);
static hsize_t H5S_point_favail (const H5S_t *space, const H5S_sel_iter_t *iter,
                                hsize_t max);
static hsize_t H5S_point_fgath (H5F_t *f, const struct H5O_layout_t *layout,
                               const struct H5O_pline_t *pline,
                               const struct H5O_fill_t *fill,
                               const struct H5O_efl_t *efl, size_t elmt_size,
                               const H5S_t *file_space,
                               H5S_sel_iter_t *file_iter, hsize_t nelmts,
                               hid_t dxpl_id, void *buf/*out*/);
static herr_t H5S_point_fscat (H5F_t *f, const struct H5O_layout_t *layout,
                               const struct H5O_pline_t *pline,
                               const struct H5O_fill_t *fill,
                               const struct H5O_efl_t *efl, size_t elmt_size,
                               const H5S_t *file_space,
                               H5S_sel_iter_t *file_iter, hsize_t nelmts,
                               hid_t dxpl_id, const void *buf);
static hsize_t H5S_point_mgath (const void *_buf, size_t elmt_size,
                               const H5S_t *mem_space,
                               H5S_sel_iter_t *mem_iter, hsize_t nelmts,
                               void *_tconv_buf/*out*/);
static herr_t H5S_point_mscat (const void *_tconv_buf, size_t elmt_size,
                               const H5S_t *mem_space,
                               H5S_sel_iter_t *mem_iter, hsize_t nelmts,
                               void *_buf/*out*/);
static herr_t H5S_select_elements(H5S_t *space, H5S_seloper_t op,
                                   size_t num_elem, const hssize_t **coord);

const H5S_fconv_t       H5S_POINT_FCONV[1] = {{
    "point",                            /*name                          */
    H5S_SEL_POINTS,                     /*selection type                */
    H5S_point_init,                     /*initialize                    */
    H5S_point_favail,                   /*available                     */
    H5S_point_fgath,                    /*gather                        */
    H5S_point_fscat,                    /*scatter                       */
}};

const H5S_mconv_t       H5S_POINT_MCONV[1] = {{
    "point",                            /*name                          */
    H5S_SEL_POINTS,                     /*selection type                */
    H5S_point_init,                     /*initialize                    */
    H5S_point_mgath,                    /*gather                        */
    H5S_point_mscat,                    /*scatter                       */
}};

                                              

/*-------------------------------------------------------------------------
 * Function:    H5S_point_init
 *
 * Purpose:     Initializes iteration information for point selection.
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
H5S_point_init (const struct H5O_layout_t UNUSED *layout,
                const H5S_t *space, H5S_sel_iter_t *sel_iter)
{
    FUNC_ENTER (H5S_point_init, FAIL);

    /* Check args */
    assert (layout);
    assert (space && H5S_SEL_POINTS==space->select.type);
    assert (sel_iter);

#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    /* Initialize the number of points to iterate over */
    sel_iter->pnt.elmt_left=space->select.num_elem;

    /* Start at the head of the list of points */
    sel_iter->pnt.curr=space->select.sel_info.pnt_lst->head;
    
    FUNC_LEAVE (SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
    H5S_point_add
 PURPOSE
    Add a series of elements to a point selection
 USAGE
    herr_t H5S_point_add(space, num_elem, coord)
        H5S_t *space;           IN: Dataspace of selection to modify
        size_t num_elem;        IN: Number of elements in COORD array.
        const hssize_t *coord[];    IN: The location of each element selected
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function adds elements to the current point selection for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t H5S_point_add (H5S_t *space, H5S_seloper_t op, size_t num_elem, const hssize_t **_coord)
{
    H5S_pnt_node_t *top, *curr, *new; /* Point selection nodes */
    const hssize_t *coord=(const hssize_t *)_coord;     /* Pointer to the actual coordinates */
    unsigned i;                 /* Counter */
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_point_add, FAIL);

    assert(space);
    assert(num_elem>0);
    assert(coord);
    assert(op==H5S_SELECT_SET || op==H5S_SELECT_APPEND || op==H5S_SELECT_PREPEND);

#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    top=curr=NULL;
    for(i=0; i<num_elem; i++) {
        /* Allocate space for the new node */
        if((new = H5MM_malloc(sizeof(H5S_pnt_node_t)))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate point node");

#ifdef QAK
        printf("%s: check 1.1, rank=%d\n",
               FUNC,(int)space->extent.u.simple.rank);
#endif /* QAK */
        if((new->pnt = H5MM_malloc(space->extent.u.simple.rank*sizeof(hssize_t)))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate coordinate information");
#ifdef QAK
        printf("%s: check 1.2\n",FUNC);
#endif /* QAK */

        /* Copy over the coordinates */
        HDmemcpy(new->pnt,coord+(i*space->extent.u.simple.rank),(space->extent.u.simple.rank*sizeof(hssize_t)));
#ifdef QAK
        printf("%s: check 1.3\n",FUNC);
        {
            int j;

            for(j=0; j<space->extent.u.simple.rank; j++) {
                printf("%s: pnt[%d]=%d\n",FUNC,(int)j,(int)new->pnt[j]);
                printf("%s: coord[%d][%d]=%d\n",
                       FUNC, (int)i, (int)j,
                       (int)*(coord+(i*space->extent.u.simple.rank)+j));
            }
        }
#endif /* QAK */

        /* Link into list */
        new->next=NULL;
        if(top==NULL)
            top=new;
        else
            curr->next=new;
        curr=new;
    } /* end for */
#ifdef QAK
    printf("%s: check 2.0\n",FUNC);
#endif /* QAK */

    /* Insert the list of points selected in the proper place */
    if(op==H5S_SELECT_SET || op==H5S_SELECT_PREPEND) {
        /* Append current list, if there is one */
        if(space->select.sel_info.pnt_lst->head!=NULL)
            curr->next=space->select.sel_info.pnt_lst->head;

        /* Put new list in point selection */
        space->select.sel_info.pnt_lst->head=top;
    }
    else {  /* op==H5S_SELECT_APPEND */
        new=space->select.sel_info.pnt_lst->head;
        if(new!=NULL) {
            while(new->next!=NULL)
                new=new->next;

            /* Append new list to point selection */
            new->next=top;
        } /* end if */
        else 
            space->select.sel_info.pnt_lst->head=top;
    }

    /* Add the number of elements in the new selection */
    space->select.num_elem+=num_elem;

    ret_value=SUCCEED;
#ifdef QAK
    printf("%s: check 3.0\n",FUNC);
#endif /* QAK */
    
done:
    FUNC_LEAVE (ret_value);
}   /* H5S_point_add() */

/*-------------------------------------------------------------------------
 * Function:    H5S_point_favail
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
H5S_point_favail (const H5S_t UNUSED *space,
                  const H5S_sel_iter_t *sel_iter, hsize_t max)
{
    FUNC_ENTER (H5S_point_favail, 0);

    /* Check args */
    assert (space && H5S_SEL_POINTS==space->select.type);
    assert (sel_iter);

#ifdef QAK
    printf("%s: check 1.0, ret=%d\n", FUNC,(int)MIN(sel_iter->pnt.elmt_left,max));
#endif /* QAK */
    FUNC_LEAVE (MIN(sel_iter->pnt.elmt_left,max));
}   /* H5S_point_favail() */

/*-------------------------------------------------------------------------
 * Function:    H5S_point_fgath
 *
 * Purpose:     Gathers data points from file F and accumulates them in the
 *              type conversion buffer BUF.  The LAYOUT argument describes
 *              how the data is stored on disk and EFL describes how the data
 *              is organized in external files.  ELMT_SIZE is the size in
 *              bytes of a datum which this function treats as opaque.
 *              FILE_SPACE describes the data space of the dataset on disk
 *              and the elements that have been selected for reading (via
 *              hyperslab, etc).  This function will copy at most NELMTS
 *              elements.
 *
 *  Notes: This could be optimized by gathering selected elements near (how
 *      near?) each other into one I/O request and then moving the correct
 *      elements into the return buffer
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
H5S_point_fgath (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline,
                 const struct H5O_fill_t *fill, const struct H5O_efl_t *efl,
                 size_t elmt_size, const H5S_t *file_space,
                 H5S_sel_iter_t *file_iter, hsize_t nelmts, hid_t dxpl_id,
                 void *_buf/*out*/)
{
    hssize_t    file_offset[H5O_LAYOUT_NDIMS];  /*offset of slab in file*/
    hsize_t     hsize[H5O_LAYOUT_NDIMS];        /*size of hyperslab     */
    hssize_t    zero[H5O_LAYOUT_NDIMS];         /*zero                  */
    uint8_t     *buf=(uint8_t *)_buf;   /* Alias for pointer arithmetic */
    unsigned    ndims;          /* Number of dimensions of dataset */
    unsigned        u;                          /*counters              */
    hsize_t     num_read;       /* number of elements read into buffer */

    FUNC_ENTER (H5S_point_fgath, 0);

    /* Check args */
    assert (f);
    assert (layout);
    assert (elmt_size>0);
    assert (file_space);
    assert (file_iter);
    assert (nelmts>0);
    assert (buf);

#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    ndims=file_space->extent.u.simple.rank;
    /* initialize hyperslab size and offset in memory buffer */
    for(u=0; u<ndims+1; u++) {
        hsize[u]=1;     /* hyperslab size is 1, except for last element */
        zero[u]=0;      /* memory offset is 0 */
    } /* end for */
    hsize[ndims] = elmt_size;

    /*
     * Walk though and request each element we need and put it into the
     * buffer.
     */
    num_read=0;
    while(num_read<nelmts) {
        if(file_iter->pnt.elmt_left>0) {
            /* Copy the location of the point to get */
            HDmemcpy(file_offset, file_iter->pnt.curr->pnt, ndims*sizeof(hssize_t));
            file_offset[ndims] = 0;

            /* Add in the offset */
            for(u=0; u<file_space->extent.u.simple.rank; u++)
                file_offset[u] += file_space->select.offset[u];

            /* Go read the point */
            if (H5F_arr_read(f, dxpl_id, layout, pline, fill, efl, hsize, hsize, zero, file_offset, buf/*out*/)<0) {
                HRETURN_ERROR(H5E_DATASPACE, H5E_READERROR, 0, "read error");
            }

#ifdef QAK
            printf("%s: check 3.0\n",FUNC);
                for(u=0; u<ndims; u++) {
                    printf("%s: %u - pnt=%d\n", FUNC, (unsigned)u, (int)file_iter->pnt.curr->pnt[u]);
                    printf("%s: %u - file_offset=%d\n", FUNC, (unsigned)u, (int)file_offset[u]);
                }
                printf("%s: *buf=%u\n",FUNC,(unsigned)*buf);
#endif /* QAK */
            /* Increment the offset of the buffer */
            buf+=elmt_size;

            /* Increment the count read */
            num_read++;

            /* Advance the point iterator */
            file_iter->pnt.elmt_left--;
            file_iter->pnt.curr=file_iter->pnt.curr->next;
        } else {
            break;      /* out of elements in the selection */
        } /* end else */
    } /* end while */
    
    FUNC_LEAVE (num_read);
} /* H5S_point_fgath() */

/*-------------------------------------------------------------------------
 * Function:    H5S_point_fscat
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
H5S_point_fscat (H5F_t *f, const struct H5O_layout_t *layout,
                 const struct H5O_pline_t *pline,
                 const struct H5O_fill_t *fill, const struct H5O_efl_t *efl,
                 size_t elmt_size, const H5S_t *file_space,
                 H5S_sel_iter_t *file_iter, hsize_t nelmts, hid_t dxpl_id,
                 const void *_buf)
{
    hssize_t    file_offset[H5O_LAYOUT_NDIMS];  /*offset of hyperslab   */
    hsize_t     hsize[H5O_LAYOUT_NDIMS];        /*size of hyperslab     */
    hssize_t    zero[H5O_LAYOUT_NDIMS];         /*zero vector           */
    const uint8_t *buf=(const uint8_t *)_buf;   /* Alias for pointer arithmetic */
    unsigned   ndims;          /* Number of dimensions of dataset */
    unsigned    u;                              /*counters              */
    hsize_t  num_written;    /* number of elements written from buffer */

    FUNC_ENTER (H5S_point_fscat, FAIL);

    /* Check args */
    assert (f);
    assert (layout);
    assert (elmt_size>0);
    assert (file_space);
    assert (file_iter);
    assert (nelmts>0);
    assert (buf);

#ifdef QAK
    printf("%s: check 1.0, layout->ndims=%d\n",FUNC,(int)layout->ndims);
#endif /* QAK */
    ndims=file_space->extent.u.simple.rank;
    /* initialize hyperslab size and offset in memory buffer */
    for(u=0; u<ndims+1; u++) {
        hsize[u]=1;     /* hyperslab size is 1, except for last element */
        zero[u]=0;      /* memory offset is 0 */
    } /* end for */
    hsize[ndims] = elmt_size;

    /*
     * Walk though and request each element we need and put it into the
     * buffer.
     */
    num_written=0;
    while(num_written<nelmts && file_iter->pnt.elmt_left>0) {
#ifdef QAK
        printf("%s: check 2.0\n",FUNC);
        {
            for(u=0; u<ndims; u++) {
            printf("%s: %u - pnt=%d\n", FUNC, (unsigned)u, (int)file_iter->pnt.curr->pnt[u]);
            }
        }
#endif /* QAK */
        /* Copy the location of the point to get */
        HDmemcpy(file_offset,file_iter->pnt.curr->pnt,ndims*sizeof(hssize_t));
        file_offset[ndims] = 0;

        /* Add in the offset, if there is one */
        for(u=0; u<file_space->extent.u.simple.rank; u++)
            file_offset[u] += file_space->select.offset[u];

#ifdef QAK
        printf("%s: check 3.0\n",FUNC);
    for(u=0; u<ndims; u++) {
        printf("%s: %u - pnt=%d\n", FUNC,(unsigned)u,(int)file_iter->pnt.curr->pnt[u]);
        printf("%s: %u - file_offset=%d\n", FUNC,(unsigned)u,(int)file_offset[u]);
    }
    printf("%s: *buf=%u\n",FUNC,(unsigned)*buf);
#endif /* QAK */
        /* Go write the point */
        if (H5F_arr_write(f, dxpl_id, layout, pline, fill, efl, hsize, hsize, zero, file_offset, buf)<0) {
            HRETURN_ERROR(H5E_DATASPACE, H5E_WRITEERROR, 0, "write error");
        }

        /* Increment the offset of the buffer */
        buf+=elmt_size;

        /* Increment the count read */
        num_written++;

        /* Advance the point iterator */
        file_iter->pnt.elmt_left--;
        file_iter->pnt.curr=file_iter->pnt.curr->next;
#ifdef QAK
        printf("%s: check 5.0, file_iter->pnt.curr=%p\n", FUNC,file_iter->pnt.curr);
#endif
    } /* end while */

    FUNC_LEAVE (num_written>0 ? SUCCEED : FAIL);
}   /* H5S_point_fscat() */

/*-------------------------------------------------------------------------
 * Function:    H5S_point_mgath
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
H5S_point_mgath (const void *_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_tconv_buf/*out*/)
{
    hsize_t     mem_size[H5O_LAYOUT_NDIMS];     /*total size of app buf */
    const uint8_t *buf=(const uint8_t *)_buf;   /* Get local copies for address arithmetic */
    uint8_t *tconv_buf=(uint8_t *)_tconv_buf;
    hsize_t     acc;                            /* coordinate accumulator */
    hsize_t     off;                            /* coordinate offset */
    int space_ndims;                    /*dimensionality of space*/
    int i;                              /*counters              */
    hsize_t num_gath;        /* number of elements gathered */

    FUNC_ENTER (H5S_point_mgath, 0);

    /* Check args */
    assert (buf);
    assert (elmt_size>0);
    assert (mem_space && H5S_SEL_POINTS==mem_space->select.type);
    assert (nelmts>0);
    assert (tconv_buf);

#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    if ((space_ndims=H5S_get_simple_extent_dims (mem_space, mem_size, NULL))<0) {
        HRETURN_ERROR (H5E_DATASPACE, H5E_CANTINIT, 0,
                       "unable to retrieve data space dimensions");
    }

    for(num_gath=0; num_gath<nelmts; num_gath++) {
        if(mem_iter->pnt.elmt_left>0) {
            /* Compute the location of the point to get */
            for(i=space_ndims-1,acc=elmt_size,off=0; i>=0; i--) {
                off+=(mem_iter->pnt.curr->pnt[i]+mem_space->select.offset[i])*acc;
                acc*=mem_size[i];
            } /* end for */

#ifdef QAK
            printf("%s: check 2.0, acc=%d, off=%d\n",FUNC,(int)acc,(int)off);
#endif /* QAK */
            /* Copy the elements into the type conversion buffer */
            HDmemcpy(tconv_buf,buf+off,elmt_size);

            /* Increment the offset of the buffers */
            tconv_buf+=elmt_size;

            /* Advance the point iterator */
            mem_iter->pnt.elmt_left--;
            mem_iter->pnt.curr=mem_iter->pnt.curr->next;
        } else {
            break;      /* out of elements in the selection */
        } /* end else */
    } /* end for */

    FUNC_LEAVE (num_gath);
}   /* H5S_point_mgath() */

/*-------------------------------------------------------------------------
 * Function:    H5S_point_mscat
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
H5S_point_mscat (const void *_tconv_buf, size_t elmt_size,
                 const H5S_t *mem_space, H5S_sel_iter_t *mem_iter,
                 hsize_t nelmts, void *_buf/*out*/)
{
    hsize_t     mem_size[H5O_LAYOUT_NDIMS];     /*total size of app buf */
    uint8_t *buf=(uint8_t *)_buf;   /* Get local copies for address arithmetic */
    const uint8_t *tconv_buf=(const uint8_t *)_tconv_buf;
    hsize_t     acc;                            /* coordinate accumulator */
    hsize_t     off;                            /* coordinate offset */
    int space_ndims;            /*dimensionality of space*/
    int i;                              /*counters              */
    hsize_t num_scat;        /* Number of elements scattered */

    FUNC_ENTER (H5S_point_mscat, FAIL);

    /* Check args */
    assert (tconv_buf);
    assert (elmt_size>0);
    assert (mem_space && H5S_SEL_POINTS==mem_space->select.type);
    assert (nelmts>0);
    assert (buf);

#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    /*
     * Retrieve hyperslab information to determine what elements are being
     * selected (there might be other selection methods in the future).  We
     * only handle hyperslabs with unit sample because there's currently no
     * way to pass sample information to H5V_hyper_copy().
     */
    if ((space_ndims=H5S_get_simple_extent_dims (mem_space, mem_size, NULL))<0) {
        HRETURN_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL,
                       "unable to retrieve data space dimensions");
    }

    for(num_scat=0; num_scat<nelmts; num_scat++) {
        if(mem_iter->pnt.elmt_left>0) {
            /* Compute the location of the point to get */
            for(i=space_ndims-1,acc=elmt_size,off=0; i>=0; i--) {
                off+=(mem_iter->pnt.curr->pnt[i]+mem_space->select.offset[i])*acc;
                acc*=mem_size[i];
            } /* end for */

            /* Copy the elements into the type conversion buffer */
            HDmemcpy(buf+off,tconv_buf,elmt_size);

            /* Increment the offset of the buffers */
            tconv_buf+=elmt_size;

            /* Advance the point iterator */
            mem_iter->pnt.elmt_left--;
            mem_iter->pnt.curr=mem_iter->pnt.curr->next;
        } else {
            break;      /* out of elements in the selection */
        } /* end else */
    } /* end for */

    FUNC_LEAVE (SUCCEED);
}   /* H5S_point_mscat() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_release
 PURPOSE
    Release point selection information for a dataspace
 USAGE
    herr_t H5S_point_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all point selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_point_release (H5S_t *space)
{
    H5S_pnt_node_t *curr, *next;        /* Point selection nodes */
    FUNC_ENTER (H5S_point_release, FAIL);

    /* Check args */
    assert (space);

    /* Delete all the nodes from the list */
    curr=space->select.sel_info.pnt_lst->head;
    while(curr!=NULL) {
        next=curr->next;
        H5MM_xfree(curr->pnt);
        H5MM_xfree(curr);
        curr=next;
    } /* end while */
    
    /* Free & reset the point list header */
    H5MM_xfree(space->select.sel_info.pnt_lst);
    space->select.sel_info.pnt_lst=NULL;

    /* Reset the number of elements in the selection */
    space->select.num_elem=0;
    
    FUNC_LEAVE (SUCCEED);
}   /* H5S_point_release() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_npoints
 PURPOSE
    Compute number of elements in current selection
 USAGE
    hsize_t H5S_point_npoints(space)
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
H5S_point_npoints (const H5S_t *space)
{
    FUNC_ENTER (H5S_point_npoints, 0);

    /* Check args */
    assert (space);

#ifdef QAK
    printf("%s: check 1.0, nelmts=%d\n",FUNC,(int)space->select.num_elem);
#endif /* QAK */
    FUNC_LEAVE (space->select.num_elem);
}   /* H5S_point_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_point_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies all the point selection information from the source
    dataspace to the destination dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_point_copy (H5S_t *dst, const H5S_t *src)
{
    H5S_pnt_node_t *curr, *new, *new_head;    /* Point information nodes */
    herr_t ret_value=SUCCEED;  /* return value */

    FUNC_ENTER (H5S_point_copy, FAIL);

    assert(src);
    assert(dst);

#ifdef QAK
 printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    /* Allocate room for the head of the point list */
    if((dst->select.sel_info.pnt_lst=H5MM_malloc(sizeof(H5S_pnt_list_t)))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
            "can't allocate point node");

    curr=src->select.sel_info.pnt_lst->head;
    new_head=NULL;
    while(curr!=NULL) {
        /* Create each point */
        if((new=H5MM_malloc(sizeof(H5S_pnt_node_t)))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate point node");
        if((new->pnt = H5MM_malloc(src->extent.u.simple.rank*sizeof(hssize_t)))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate coordinate information");
        HDmemcpy(new->pnt,curr->pnt,(src->extent.u.simple.rank*sizeof(hssize_t)));
        new->next=NULL;

#ifdef QAK
 printf("%s: check 5.0\n",FUNC);
    {
        int i;
        for(i=0; i<src->extent.u.simple.rank; i++)
            printf("%s: check 5.1, new->pnt[%d]=%d\n",FUNC,i,(int)new->pnt[i]);
    }
#endif /* QAK */

        /* Keep the order the same when copying */
        if(new_head==NULL)
            new_head=dst->select.sel_info.pnt_lst->head=new;
        else {
            new_head->next=new;
            new_head=new;
        } /* end else */

        curr=curr->next;
    } /* end while */
#ifdef QAK
 printf("%s: check 10.0 src->select.sel_info.pnt_lst=%p, dst->select.sel_info.pnt_lst=%p\n",FUNC,src->select.sel_info.pnt_lst,dst->select.sel_info.pnt_lst);
 printf("%s: check 10.0 src->select.sel_info.pnt_lst->head=%p, dst->select.sel_info.pnt_lst->head=%p\n",FUNC,src->select.sel_info.pnt_lst->head,dst->select.sel_info.pnt_lst->head);
#endif /* QAK */

done:
    FUNC_LEAVE (ret_value);
} /* end H5S_point_copy() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_select_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_point_select_valid(space);
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
H5S_point_select_valid (const H5S_t *space)
{
    H5S_pnt_node_t *curr;      /* Point information nodes */
    unsigned u;                   /* Counter */
    htri_t ret_value=TRUE;     /* return value */

    FUNC_ENTER (H5S_point_select_valid, FAIL);

    assert(space);

#ifdef QAK
printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    /* Check each point to determine whether selection+offset is within extent */
    curr=space->select.sel_info.pnt_lst->head;
    while(curr!=NULL) {
        /* Check each dimension */
        for(u=0; u<space->extent.u.simple.rank; u++) {
#ifdef QAK
printf("%s: check 2.0\n",FUNC);
printf("%s: curr->pnt[%u]=%d\n",FUNC,(unsigned)u,(int)curr->pnt[u]);
printf("%s: space->select.offset[%u]=%d\n",FUNC,(unsigned)u,(int)space->select.offset[u]);
printf("%s: space->extent.u.simple.size[%u]=%d\n",FUNC,(unsigned)u,(int)space->extent.u.simple.size[u]);
#endif /* QAK */
            /* Check if an offset has been defined */
            /* Bounds check the selected point + offset against the extent */
            if(((curr->pnt[u]+space->select.offset[u])>(hssize_t)space->extent.u.simple.size[u])
                    || ((curr->pnt[u]+space->select.offset[u])<0)) {
                ret_value=FALSE;
                break;
            } /* end if */
        } /* end for */

        curr=curr->next;
    } /* end while */
#ifdef QAK
printf("%s: check 3.0\n",FUNC);
#endif /* QAK */

    FUNC_LEAVE (ret_value);
} /* end H5S_point_select_valid() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_select_serial_size
 PURPOSE
    Determine the number of bytes needed to store the serialized point selection
    information.
 USAGE
    hssize_t H5S_point_select_serial_size(space)
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize the current point
    selection information for storage on disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5S_point_select_serial_size (const H5S_t *space)
{
    H5S_pnt_node_t *curr;       /* Point information nodes */
    hssize_t ret_value=FAIL;    /* return value */

    FUNC_ENTER (H5S_point_select_serial_size, FAIL);

    assert(space);

    /* Basic number of bytes required to serialize point selection:
     *  <type (4 bytes)> + <version (4 bytes)> + <padding (4 bytes)> + 
     *      <length (4 bytes)> + <rank (4 bytes)> + <# of points (4 bytes)> = 24 bytes
     */
    ret_value=24;

    /* Count points in selection */
    curr=space->select.sel_info.pnt_lst->head;
    while(curr!=NULL) {
        /* Add 4 bytes times the rank for each element selected */
        ret_value+=4*space->extent.u.simple.rank;
        curr=curr->next;
    } /* end while */

    FUNC_LEAVE (ret_value);
} /* end H5S_point_select_serial_size() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_select_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_point_select_serialize(space, buf)
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
H5S_point_select_serialize (const H5S_t *space, uint8_t *buf)
{
    H5S_pnt_node_t *curr;   /* Point information nodes */
    uint8_t *lenp;          /* pointer to length location for later storage */
    uint32_t len=0;         /* number of bytes used */
    unsigned u;                /* local counting variable */
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

    /* Encode number of elements */
    UINT32ENCODE(buf, (uint32_t)space->select.num_elem);
    len+=4;

    /* Encode each point in selection */
    curr=space->select.sel_info.pnt_lst->head;
    while(curr!=NULL) {
        /* Add 4 bytes times the rank for each element selected */
        len+=4*space->extent.u.simple.rank;

        /* Encode each point */
        for(u=0; u<space->extent.u.simple.rank; u++)
            UINT32ENCODE(buf, (uint32_t)curr->pnt[u]);

        curr=curr->next;
    } /* end while */

    /* Encode length */
    UINT32ENCODE(lenp, (uint32_t)len);  /* Store the length of the extra information */
    
    /* Set success */
    ret_value=SUCCEED;

    FUNC_LEAVE (ret_value);
}   /* H5S_point_select_serialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_select_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_point_select_deserialize(space, buf)
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
H5S_point_select_deserialize (H5S_t *space, const uint8_t *buf)
{
    H5S_seloper_t op=H5S_SELECT_SET;    /* Selection operation */
    uint32_t rank;           /* Rank of points */
    size_t num_elem=0;      /* Number of elements in selection */
    hssize_t *coord=NULL, *tcoord;   /* Pointer to array of elements */
    unsigned i,j;              /* local counting variables */
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_point_select_deserialize, FAIL);

    /* Check args */
    assert(space);
    assert(buf);

    /* Deserialize points to select */
    buf+=16;    /* Skip over selection header */
    UINT32DECODE(buf,rank);  /* decode the rank of the point selection */
    if(rank!=space->extent.u.simple.rank)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "rank of pointer does not match dataspace");
    UINT32DECODE(buf,num_elem);  /* decode the number of points */

    /* Allocate space for the coordinates */
    if((coord = H5MM_malloc(num_elem*rank*sizeof(hssize_t)))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate coordinate information");
    
    /* Retrieve the coordinates from the buffer */
    for(tcoord=coord,i=0; i<num_elem; i++)
        for(j=0; j<(unsigned)rank; j++,tcoord++)
            UINT32DECODE(buf, *tcoord);

    /* Select points */
    if((ret_value=H5S_select_elements(space,op,num_elem,(const hssize_t **)coord))<0) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");
    } /* end if */

done:
    /* Free the coordinate array if necessary */
    if(coord!=NULL)
        H5MM_xfree(coord);

    FUNC_LEAVE (ret_value);
}   /* H5S_point_select_deserialize() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_point_bounds(space, hsize_t *start, hsize_t *end)
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
H5S_point_bounds(H5S_t *space, hsize_t *start, hsize_t *end)
{
    H5S_pnt_node_t *node;       /* Point node */
    int rank;                  /* Dataspace rank */
    int i;                     /* index variable */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER (H5S_point_bounds, FAIL);

    assert(space);
    assert(start);
    assert(end);

    /* Get the dataspace extent rank */
    rank=space->extent.u.simple.rank;

    /* Iterate through the node, checking the bounds on each element */
    node=space->select.sel_info.pnt_lst->head;
    while(node!=NULL) {
        for(i=0; i<rank; i++) {
            if(start[i]>(hsize_t)(node->pnt[i]+space->select.offset[i]))
                start[i]=node->pnt[i]+space->select.offset[i];
            if(end[i]<(hsize_t)(node->pnt[i]+space->select.offset[i]))
                end[i]=node->pnt[i]+space->select.offset[i];
        } /* end for */
        node=node->next;
      } /* end while */

    FUNC_LEAVE (ret_value);
}   /* H5Sget_point_bounds() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_select_contiguous
 PURPOSE
    Check if a point selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_point_select_contiguous(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is contiguous.
    This is primarily used for reading the entire selection in one swoop.
    This code currently doesn't properly check for contiguousness when there is
    more than one point, as that would take a lot of extra coding that we
    don't need now.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_point_select_contiguous(const H5S_t *space)
{
    htri_t ret_value=FAIL;  /* return value */

    FUNC_ENTER (H5S_point_select_contiguous, FAIL);

    assert(space);

    /* One point is definitely contiguous */
    if(space->select.num_elem==1)
        ret_value=TRUE;
    else        /* More than one point might be contiguous, but it's complex to check and we don't need it right now */
        ret_value=FALSE;

    FUNC_LEAVE (ret_value);
}   /* H5S_point_select_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_elements
 PURPOSE
    Specify a series of elements in the dataspace to select
 USAGE
    herr_t H5S_select_elements(dsid, op, num_elem, coord)
        hid_t dsid;             IN: Dataspace ID of selection to modify
        H5S_seloper_t op;       IN: Operation to perform on current selection
        size_t num_elem;        IN: Number of elements in COORD array.
        const hssize_t **coord; IN: The location of each element selected
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function selects array elements to be included in the selection for
    the dataspace.  The COORD array is a 2-D array of size <dataspace rank>
    by NUM_ELEM (ie. a list of coordinates in the dataspace).  The order of
    the element coordinates in the COORD array specifies the order that the
    array elements are iterated through when I/O is performed.  Duplicate
    coordinates are not checked for.  The selection operator, OP, determines
    how the new selection is to be combined with the existing selection for
    the dataspace.  Currently, only H5S_SELECT_SET is supported, which replaces
    the existing selection with the one defined in this call.  When operators
    other than H5S_SELECT_SET are used to combine a new selection with an
    existing selection, the selection ordering is reset to 'C' array ordering.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t H5S_select_elements (H5S_t *space, H5S_seloper_t op, size_t num_elem,
    const hssize_t **coord)
{
    herr_t ret_value=SUCCEED;  /* return value */

    FUNC_ENTER (H5S_select_elements, FAIL);

    /* Check args */
    assert(space);
    assert(num_elem);
    assert(coord);
    assert(op==H5S_SELECT_SET || op==H5S_SELECT_APPEND || op==H5S_SELECT_PREPEND);

#ifdef QAK
    printf("%s: check 1.0\n",FUNC);
#endif /* QAK */
    /* If we are setting a new selection, remove current selection first */
    if(op==H5S_SELECT_SET) {
        if(H5S_select_release(space)<0) {
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL,
                "can't release hyperslab");
        } /* end if */
    } /* end if */

#ifdef QAK
    printf("%s: check 2.0\n",FUNC);
#endif /* QAK */
    /* Allocate space for the point selection information if necessary */
    if(space->select.type!=H5S_SEL_POINTS || space->select.sel_info.pnt_lst==NULL) {
        if((space->select.sel_info.pnt_lst = H5MM_calloc(sizeof(H5S_pnt_list_t)))==NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                "can't allocate element information");
    } /* end if */

#ifdef QAK
    printf("%s: check 3.0\n",FUNC);
#endif /* QAK */
    /* Add points to selection */
    if(H5S_point_add(space,op,num_elem,coord)<0) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL,
            "can't insert elements");
    }

    /* Set selection type */
    space->select.type=H5S_SEL_POINTS;
#ifdef QAK
    printf("%s: check 4.0\n",FUNC);
#endif /* QAK */

done:
    FUNC_LEAVE (ret_value);
}   /* H5S_select_elements() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_elements
 PURPOSE
    Specify a series of elements in the dataspace to select
 USAGE
    herr_t H5Sselect_elements(dsid, op, num_elem, coord)
        hid_t dsid;             IN: Dataspace ID of selection to modify
        H5S_seloper_t op;       IN: Operation to perform on current selection
        size_t num_elem;        IN: Number of elements in COORD array.
        const hssize_t **coord; IN: The location of each element selected
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function selects array elements to be included in the selection for
    the dataspace.  The COORD array is a 2-D array of size <dataspace rank>
    by NUM_ELEM (ie. a list of coordinates in the dataspace).  The order of
    the element coordinates in the COORD array specifies the order that the
    array elements are iterated through when I/O is performed.  Duplicate
    coordinates are not checked for.  The selection operator, OP, determines
    how the new selection is to be combined with the existing selection for
    the dataspace.  Currently, only H5S_SELECT_SET is supported, which replaces
    the existing selection with the one defined in this call.  When operators
    other than H5S_SELECT_SET are used to combine a new selection with an
    existing selection, the selection ordering is reset to 'C' array ordering.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t H5Sselect_elements (hid_t spaceid, H5S_seloper_t op, size_t num_elem,
    const hssize_t **coord)
{
    H5S_t       *space = NULL;  /* Dataspace to modify selection of */
    herr_t ret_value=SUCCEED;  /* return value */

    FUNC_ENTER (H5Sselect_elements, FAIL);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(spaceid) ||
            NULL == (space=H5I_object(spaceid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if(coord==NULL || num_elem==0) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "elements not specified");
    } /* end if */
    if(!(op==H5S_SELECT_SET || op==H5S_SELECT_APPEND || op==H5S_SELECT_PREPEND)) {
        HRETURN_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL,
            "operations other than H5S_SELECT_SET not supported currently");
    } /* end if */

    /* Call the real element selection routine */
    if((ret_value=H5S_select_elements(space,op,num_elem,coord))<0) {
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't select elements");
    } /* end if */

done:
    FUNC_LEAVE (ret_value);
}   /* H5Sselect_elements() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_select_iterate
 PURPOSE
    Iterate over a point selection, calling a user's function for each
        element.
 USAGE
    herr_t H5S_point_select_iterate(buf, type_id, space, op, operator_data)
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
H5S_point_select_iterate(void *buf, hid_t type_id, H5S_t *space, H5D_operator_t op,
        void *operator_data)
{
    hsize_t     mem_size[H5O_LAYOUT_NDIMS]; /* Dataspace size */
    hssize_t    mem_offset[H5O_LAYOUT_NDIMS];   /* Point offset */
    hsize_t offset;             /* offset of region in buffer */
    void *tmp_buf;              /* temporary location of the element in the buffer */
    H5S_pnt_node_t *node;   /* Point node */
    unsigned rank;             /* Dataspace rank */
    herr_t ret_value=0;     /* return value */

    FUNC_ENTER (H5S_point_select_iterate, 0);

    assert(buf);
    assert(space);
    assert(op);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    /* Get the dataspace extent rank */
    rank=space->extent.u.simple.rank;

    /* Set up the size of the memory space */
    HDmemcpy(mem_size, space->extent.u.simple.size, rank*sizeof(hsize_t));
    mem_size[rank]=H5Tget_size(type_id);

    /* Iterate through the node, checking the bounds on each element */
    node=space->select.sel_info.pnt_lst->head;
    while(node!=NULL && ret_value==0) {
        /* Set up the location of the point */
        HDmemcpy(mem_offset, node->pnt, rank*sizeof(hssize_t));
        mem_offset[rank]=0;

        /* Get the offset in the memory buffer */
        offset=H5V_array_offset(rank+1,mem_size,(const hssize_t *)mem_offset);
        tmp_buf=((char *)buf+offset);

        ret_value=(*op)(tmp_buf,type_id,(hsize_t)rank,node->pnt,operator_data);

        node=node->next;
      } /* end while */

    FUNC_LEAVE (ret_value);
}   /* H5S_point_select_iterate() */
