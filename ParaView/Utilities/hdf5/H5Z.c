/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Thursday, April 16, 1998
 *
 * Purpose:     Functions for data filters.
 */
#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"
#include "H5Zprivate.h"

/* Interface initialization */
#define PABLO_MASK      H5Z_mask
#define INTERFACE_INIT H5Z_init_interface
static int interface_initialize_g = 0;
static herr_t H5Z_init_interface (void);

static size_t           H5Z_table_alloc_g = 0;
static size_t           H5Z_table_used_g = 0;
static H5Z_class_t      *H5Z_table_g = NULL;



/*-------------------------------------------------------------------------
 * Function:    H5Z_init_interface
 *
 * Purpose:     Initializes the data filter layer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5Z_init_interface (void)
{
    FUNC_ENTER (H5Z_init_interface, FAIL);

    H5Z_register (H5Z_FILTER_DEFLATE, "deflate", H5Z_filter_deflate);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_term_interface
 *
 * Purpose:     Terminate the H5Z layer.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Z_term_interface (void)
{
    size_t      i;
#ifdef H5Z_DEBUG
    int         dir, nprint=0;
    char        comment[16], bandwidth[32];
#endif

    if (interface_initialize_g) {
#ifdef H5Z_DEBUG
        if (H5DEBUG(Z)) {
            for (i=0; i<H5Z_table_used_g; i++) {
                for (dir=0; dir<2; dir++) {
                    if (0==H5Z_table_g[i].stats[dir].total) continue;

                    if (0==nprint++) {
                        /* Print column headers */
                        HDfprintf (H5DEBUG(Z), "H5Z: filter statistics "
                                   "accumulated over life of library:\n");
                        HDfprintf (H5DEBUG(Z),
                                   "   %-16s %10s %10s %8s %8s %8s %10s\n",
                                   "Filter", "Total", "Errors", "User",
                                   "System", "Elapsed", "Bandwidth");
                        HDfprintf (H5DEBUG(Z),
                                   "   %-16s %10s %10s %8s %8s %8s %10s\n",
                                   "------", "-----", "------", "----",
                                   "------", "-------", "---------");
                    }

                    /* Truncate the comment to fit in the field */
                    HDstrncpy(comment, H5Z_table_g[i].name,
                              sizeof comment);
                    comment[sizeof(comment)-1] = '\0';

                    /*
                     * Format bandwidth to have four significant digits and
                     * units of `B/s', `kB/s', `MB/s', `GB/s', or `TB/s' or
                     * the word `Inf' if the elapsed time is zero.
                     */
                    H5_bandwidth(bandwidth,
                                 (double)(H5Z_table_g[i].stats[dir].total),
                                 H5Z_table_g[i].stats[dir].timer.etime);

                    /* Print the statistics */
                    HDfprintf (H5DEBUG(Z),
                               "   %s%-15s %10Hd %10Hd %8.2f %8.2f %8.2f "
                               "%10s\n", dir?"<":">", comment, 
                               H5Z_table_g[i].stats[dir].total,
                               H5Z_table_g[i].stats[dir].errors,
                               H5Z_table_g[i].stats[dir].timer.utime,
                               H5Z_table_g[i].stats[dir].timer.stime,
                               H5Z_table_g[i].stats[dir].timer.etime,
                               bandwidth);
                }
            }
        }
#endif
        /* Free the table */
        for (i=0; i<H5Z_table_used_g; i++) {
            H5MM_xfree(H5Z_table_g[i].name);
        }
        H5Z_table_g = H5MM_xfree(H5Z_table_g);
        H5Z_table_used_g = H5Z_table_alloc_g = 0;
        interface_initialize_g = 0;
    }
    return 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5Zregister
 *
 * Purpose:     This function registers new filter. The COMMENT argument is
 *              used for debugging and may be the null pointer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Zregister(H5Z_filter_t id, const char *comment, H5Z_func_t func)
{
    FUNC_ENTER (H5Zregister, FAIL);
    H5TRACE3("e","Zfsx",id,comment,func);

    /* Check args */
    if (id<0 || id>H5Z_FILTER_MAX) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "invalid filter identification number");
    }
    if (id<256) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "unable to modify predefined filters");
    }
    if (!func) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "no function specified");
    }

    /* Do it */
    if (H5Z_register (id, comment, func)<0) {
        HRETURN_ERROR (H5E_PLINE, H5E_CANTINIT, FAIL,
                       "unable to register filter");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_register
 *
 * Purpose:     Same as the public version except this one allows filters
 *              to be set for predefined method numbers <256
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Z_register (H5Z_filter_t id, const char *comment, H5Z_func_t func)
{
    size_t              i;
    
    FUNC_ENTER (H5Z_register, FAIL);

    assert (id>=0 && id<=H5Z_FILTER_MAX);

    /* Is the filter already registered? */
    for (i=0; i<H5Z_table_used_g; i++) {
        if (H5Z_table_g[i].id==id) break;
    }
    if (i>=H5Z_table_used_g) {
        if (H5Z_table_used_g>=H5Z_table_alloc_g) {
            size_t n = MAX(32, 2*H5Z_table_alloc_g);
            H5Z_class_t *table = H5MM_realloc(H5Z_table_g,
                                              n*sizeof(H5Z_class_t));
            if (!table) {
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                              "unable to extend filter table");
            }
            H5Z_table_g = table;
            H5Z_table_alloc_g = n;
        }

        /* Initialize */
        i = H5Z_table_used_g++;
        HDmemset(H5Z_table_g+i, 0, sizeof(H5Z_class_t));
        H5Z_table_g[i].id = id;
        H5Z_table_g[i].name = H5MM_xstrdup(comment);
        H5Z_table_g[i].func = func;
    } else {
        /* Replace old contents */
        H5MM_xfree(H5Z_table_g[i].name);
        H5Z_table_g[i].name = H5MM_xstrdup(comment);
        H5Z_table_g[i].func = func;
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_append
 *
 * Purpose:     Append another filter to the specified pipeline.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Z_append(H5O_pline_t *pline, H5Z_filter_t filter, unsigned flags,
           size_t cd_nelmts, const unsigned int cd_values[/*cd_nelmts*/])
{
    size_t      idx, i;
    
    FUNC_ENTER(H5Z_append, FAIL);
    assert(pline);
    assert(filter>=0 && filter<=H5Z_FILTER_MAX);
    assert(0==(flags & ~((unsigned)H5Z_FLAG_DEFMASK)));
    assert(0==cd_nelmts || cd_values);

    /*
     * Check filter limit.  We do it here for early warnings although we may
     * decide to relax this restriction in the future.
     */
    if (pline->nfilters>=32) {
        HRETURN_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL,
                      "too many filters in pipeline");
    }

    /* Allocate additional space in the pipeline if it's full */
    if (pline->nfilters>=pline->nalloc) {
        H5O_pline_t x;
        x.nalloc = MAX(32, 2*pline->nalloc);
        x.filter = H5MM_realloc(pline->filter, x.nalloc*sizeof(x.filter[0]));
        if (NULL==x.filter) {
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                          "memory allocation failed for filter pipeline");
        }
        pline->nalloc = x.nalloc;
        pline->filter = x.filter;
    }
    
    /* Add the new filter to the pipeline */
    idx = pline->nfilters;
    pline->filter[idx].id = filter;
    pline->filter[idx].flags = flags;
    pline->filter[idx].name = NULL; /*we'll pick it up later*/
    pline->filter[idx].cd_nelmts = cd_nelmts;
    if (cd_nelmts>0) {
        pline->filter[idx].cd_values = H5MM_malloc(cd_nelmts*sizeof(unsigned));
        if (NULL==pline->filter[idx].cd_values) {
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                          "memory allocation failed for filter");
        }
        for (i=0; i<cd_nelmts; i++) {
            pline->filter[idx].cd_values[i] = cd_values[i];
        }
    } else {
       pline->filter[idx].cd_values = NULL;
    }
    pline->nfilters++;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_find
 *
 * Purpose:     Given a filter ID return a pointer to a global struct that
 *              defines the filter.
 *
 * Return:      Success:        Ptr to entry in global filter table.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5Z_class_t *
H5Z_find(H5Z_filter_t id)
{
    size_t      i;

    FUNC_ENTER(H5Z_find, NULL);

    for (i=0; i<H5Z_table_used_g; i++) {
        if (H5Z_table_g[i].id == id) {
            HRETURN(H5Z_table_g+i);
        }
    }

    FUNC_LEAVE(NULL);
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_pipeline
 *
 * Purpose:     Process data through the filter pipeline.  The FLAGS argument
 *              is the filter invocation flags (definition flags come from
 *              the PLINE->filter[].flags).  The filters are processed in
 *              definition order unless the H5Z_FLAG_REVERSE is set.  The
 *              FILTER_MASK is a bit-mask to indicate which filters to skip
 *              and on exit will indicate which filters failed.  Each
 *              filter has an index number in the pipeline and that index
 *              number is the filter's bit in the FILTER_MASK.  NBYTES is the
 *              number of bytes of data to filter and on exit should be the
 *              number of resulting bytes while BUF_SIZE holds the total
 *              allocated size of the buffer, which is pointed to BUF.
 *
 *              If the buffer must grow during processing of the pipeline
 *              then the pipeline function should free the original buffer
 *              and return a fresh buffer, adjusting BUF_SIZE accordingly.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Z_pipeline(H5F_t UNUSED *f, const H5O_pline_t *pline, unsigned flags,
             unsigned *filter_mask/*in,out*/, size_t *nbytes/*in,out*/,
             size_t *buf_size/*in,out*/, void **buf/*in,out*/)
{
    size_t      i, idx, new_nbytes;
    H5Z_class_t *fclass=NULL;
    unsigned    failed = 0;
#ifdef H5Z_DEBUG
    H5_timer_t  timer;
#endif
    
    FUNC_ENTER(H5Z_pipeline, FAIL);
    
    assert(f);
    assert(0==(flags & ~((unsigned)H5Z_FLAG_INVMASK)));
    assert(filter_mask);
    assert(nbytes && *nbytes>0);
    assert(buf_size && *buf_size>0);
    assert(buf && *buf);
    assert(!pline || pline->nfilters<32);

    if (pline && (flags & H5Z_FLAG_REVERSE)) {
        for (i=pline->nfilters; i>0; --i) {
            idx = i-1;
            
            if (*filter_mask & ((unsigned)1<<idx)) {
                failed |= (unsigned)1 << idx;
                continue;/*filter excluded*/
            }
            if (NULL==(fclass=H5Z_find(pline->filter[idx].id))) {
                failed |= (unsigned)1 << idx;
                HRETURN_ERROR(H5E_PLINE, H5E_READERROR, FAIL,
                              "required filter is not registered");
            }
#ifdef H5Z_DEBUG
            H5_timer_begin(&timer);
#endif
            new_nbytes = (fclass->func)(flags|(pline->filter[idx].flags),
                                        pline->filter[idx].cd_nelmts,
                                        pline->filter[idx].cd_values,
                                        *nbytes, buf_size, buf);
#ifdef H5Z_DEBUG
            H5_timer_end(&(fclass->stats[1].timer), &timer);
            fclass->stats[1].total += MAX(*nbytes, new_nbytes);
            if (0==new_nbytes) fclass->stats[1].errors += *nbytes;
#endif
            if (0==new_nbytes) {
                failed |= (unsigned)1 << idx;
                HRETURN_ERROR(H5E_PLINE, H5E_READERROR, FAIL,
                              "filter returned failure");
            }
            *nbytes = new_nbytes;
        }
    } else if (pline) {
        for (idx=0; idx<pline->nfilters; idx++) {
            if (*filter_mask & ((unsigned)1<<idx)) {
                failed |= (unsigned)1 << idx;
                continue; /*filter excluded*/
            }
            if (NULL==(fclass=H5Z_find(pline->filter[idx].id))) {
                failed |= (unsigned)1 << idx;
                if (pline->filter[idx].flags & H5Z_FLAG_OPTIONAL) {
                    H5E_clear();
                    continue;
                } else {
                    HRETURN_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL,
                                  "required filter is not registered");
                }
            }
#ifdef H5Z_DEBUG
            H5_timer_begin(&timer);
#endif
            new_nbytes = (fclass->func)(flags|(pline->filter[idx].flags),
                                        pline->filter[idx].cd_nelmts,
                                        pline->filter[idx].cd_values,
                                        *nbytes, buf_size, buf);
#ifdef H5Z_DEBUG
            H5_timer_end(&(fclass->stats[0].timer), &timer);
            fclass->stats[0].total += MAX(*nbytes, new_nbytes);
            if (0==new_nbytes) fclass->stats[0].errors += *nbytes;
#endif
            if (0==new_nbytes) {
                failed |= (unsigned)1 << idx;
                if (0==(pline->filter[idx].flags & H5Z_FLAG_OPTIONAL)) {
                    HRETURN_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL,
                                  "filter returned failure");
                } else {
                    H5E_clear();
                }
            } else {
                *nbytes = new_nbytes;
            }
        }
    }

    *filter_mask = failed;
    FUNC_LEAVE(SUCCEED);
}

