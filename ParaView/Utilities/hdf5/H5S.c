/****************************************************************************
* NCSA HDF                                                                 *
* Software Development Group                                               *
* National Center for Supercomputing Applications                          *
* University of Illinois at Urbana-Champaign                               *
* 605 E. Springfield, Champaign IL 61820                                   *
*                                                                          *
* For conditions of distribution and use, see the accompanying             *
* hdf/COPYING file.                                                        *
*                                                                          *
****************************************************************************/

/* Id */

#define H5S_PACKAGE             /*suppress error about including H5Spkg   */

#define _H5S_IN_H5S_C
#include "H5private.h"          /* Generic Functions                      */
#include "H5Iprivate.h"         /* ID Functions                           */
#include "H5Eprivate.h"         /* Error handling                         */
#include "H5FLprivate.h"        /* Free Lists                             */
#include "H5MMprivate.h"        /* Memory Management functions            */
#include "H5Oprivate.h"         /* object headers                         */
#include "H5Spkg.h"             /* Data-space functions                   */

/* Interface initialization */
#define PABLO_MASK      H5S_mask
#define INTERFACE_INIT  H5S_init_interface
static int              interface_initialize_g = 0;
static herr_t           H5S_init_interface(void);

/* Tables of file and memory conversion information */
static const H5S_fconv_t        *H5S_fconv_g[H5S_SEL_N];
static const H5S_mconv_t        *H5S_mconv_g[H5S_SEL_N];

/* The path table, variable length */
static H5S_conv_t               **H5S_conv_g = NULL;
static size_t                   H5S_aconv_g = 0;        /*entries allocated*/
static size_t                   H5S_nconv_g = 0;        /*entries used*/

#ifdef H5_HAVE_PARALLEL
/* Global var whose value comes from environment variable */
hbool_t         H5_mpi_opt_types_g = FALSE;
#endif

/* Declare a free list to manage the H5S_simple_t struct */
H5FL_DEFINE(H5S_simple_t);

/* Declare a free list to manage the H5S_t struct */
H5FL_DEFINE(H5S_t);

/* Declare a free list to manage the array's of hsize_t's */
H5FL_ARR_DEFINE(hsize_t,H5S_MAX_RANK);

/* Declare a free list to manage the array's of hssize_t's */
H5FL_ARR_DEFINE(hssize_t,H5S_MAX_RANK);


/*--------------------------------------------------------------------------
NAME
   H5S_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5S_init_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5S_init_interface(void)
{
    FUNC_ENTER(H5S_init_interface, FAIL);

    /* Initialize the atom group for the file IDs */
    if (H5I_init_group(H5I_DATASPACE, H5I_DATASPACEID_HASHSIZE,
                       H5S_RESERVED_ATOMS, (H5I_free_t)H5S_close)<0) {
        HRETURN_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL,
                       "unable to initialize interface");
    }

    /* Register space conversion functions */
    if (H5S_register(H5S_SEL_POINTS, H5S_POINT_FCONV, H5S_POINT_MCONV)<0 ||
        H5S_register(H5S_SEL_ALL, H5S_ALL_FCONV, H5S_ALL_MCONV) <0 ||
        H5S_register(H5S_SEL_HYPERSLABS, H5S_HYPER_FCONV, H5S_HYPER_MCONV)<0) {
        HRETURN_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL,
                      "unable to register one or more conversion functions");
    }

#ifdef H5_HAVE_PARALLEL
    {
        /* Allow MPI buf-and-file-type optimizations? */
        const char *s = HDgetenv ("HDF5_MPI_OPT_TYPES");
#ifdef H5FDmpio_DEBUG
        hbool_t         oldtmp = H5_mpi_opt_types_g ;
#endif
        if (s && HDisdigit(*s)) {
            H5_mpi_opt_types_g = (int)HDstrtol (s, NULL, 0);
        }
#ifdef H5FDmpio_DEBUG
        fprintf(stdout, "H5_mpi_opt_types_g was %ld became %ld\n",
                    oldtmp, H5_mpi_opt_types_g);
#endif
    }
#endif

    FUNC_LEAVE(SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
    H5S_term_interface
 PURPOSE
    Terminate various H5S objects
 USAGE
    void H5S_term_interface()
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5S_term_interface(void)
{
    size_t      i;
    int n=0;
    
#ifdef H5S_DEBUG
    int         j, nprints=0;
    H5S_conv_t  *path=NULL;
    char        buf[256];
#endif

    if (interface_initialize_g) {
        if ((n=H5I_nmembers(H5I_DATASPACE))) {
            H5I_clear_group(H5I_DATASPACE, FALSE);
        } else {
#ifdef H5S_DEBUG
            /*
             * Print statistics about each conversion path.
             */
            if (H5DEBUG(S)) {
                for (i=0; i<H5S_nconv_g; i++) {
                    path = H5S_conv_g[i];
                    for (j=0; j<2; j++) {
                        if (0==path->stats[j].gath_ncalls &&
                            0==path->stats[j].scat_ncalls &&
                            0==path->stats[j].bkg_ncalls &&
                            0==path->stats[j].read_ncalls &&
                            0==path->stats[j].write_ncalls) {
                            continue;
                        }
                        if (0==nprints++) {
                            fprintf(H5DEBUG(S), "H5S: data space conversion "
                                    "statistics:\n");
                            fprintf(H5DEBUG(S),
                                    "   %-16s %10s %10s %8s %8s %8s %10s\n",
                                    "Memory <> File", "Bytes", "Calls",
                                    "User", "System", "Elapsed", "Bandwidth");
                            fprintf(H5DEBUG(S),
                                    "   %-16s %10s %10s %8s %8s %8s %10s\n",
                                    "--------------", "-----", "-----",
                                    "----", "------", "-------", "---------");
                        }

                        /* Summary */
                        sprintf(buf, "%s %c %s",
                                path->m->name, 0==j?'>':'<', path->f->name);
                        fprintf(H5DEBUG(S), "   %-16s\n", buf);

                        /* Gather */
                        if (path->stats[j].gath_ncalls) {
                            H5_bandwidth(buf,
                                         (double)(path->stats[j].gath_nbytes),
                                         path->stats[j].gath_timer.etime);
                            HDfprintf(H5DEBUG(S),
                                      "   %16s %10Hu %10Hu %8.2f %8.2f %8.2f "
                                      "%10s\n", "gather",
                                      path->stats[j].gath_nbytes,
                                      path->stats[j].gath_ncalls,
                                      path->stats[j].gath_timer.utime, 
                                      path->stats[j].gath_timer.stime, 
                                      path->stats[j].gath_timer.etime,
                                      buf);
                        }

                        /* Scatter */
                        if (path->stats[j].scat_ncalls) {
                            H5_bandwidth(buf,
                                         (double)(path->stats[j].scat_nbytes),
                                         path->stats[j].scat_timer.etime);
                            HDfprintf(H5DEBUG(S),
                                      "   %16s %10Hu %10Hu %8.2f %8.2f %8.2f "
                                      "%10s\n", "scatter",
                                      path->stats[j].scat_nbytes,
                                      path->stats[j].scat_ncalls,
                                      path->stats[j].scat_timer.utime, 
                                      path->stats[j].scat_timer.stime, 
                                      path->stats[j].scat_timer.etime,
                                      buf);
                        }

                        /* Background */
                        if (path->stats[j].bkg_ncalls) {
                            H5_bandwidth(buf,
                                         (double)(path->stats[j].bkg_nbytes),
                                         path->stats[j].bkg_timer.etime);
                            HDfprintf(H5DEBUG(S),
                                      "   %16s %10Hu %10Hu %8.2f %8.2f %8.2f "
                                      "%10s\n", "background",
                                      path->stats[j].bkg_nbytes,
                                      path->stats[j].bkg_ncalls,
                                      path->stats[j].bkg_timer.utime, 
                                      path->stats[j].bkg_timer.stime, 
                                      path->stats[j].bkg_timer.etime,
                                      buf);
                        }

                        /* Read */
                        if (path->stats[j].read_ncalls) {
                            H5_bandwidth(buf,
                                         (double)(path->stats[j].read_nbytes),
                                         path->stats[j].read_timer.etime);
                            HDfprintf(H5DEBUG(S),
                                      "   %16s %10Hu %10Hu %8.2f %8.2f %8.2f "
                                      "%10s\n", "read",
                                      path->stats[j].read_nbytes,
                                      path->stats[j].read_ncalls,
                                      path->stats[j].read_timer.utime,
                                      path->stats[j].read_timer.stime,
                                      path->stats[j].read_timer.etime,
                                      buf);
                        }
                        
                        /* Write */
                        if (path->stats[j].write_ncalls) {
                            H5_bandwidth(buf,
                                         (double)(path->stats[j].write_nbytes),
                                         path->stats[j].write_timer.etime);
                            HDfprintf(H5DEBUG(S),
                                      "   %16s %10Hu %10Hu %8.2f %8.2f %8.2f "
                                      "%10s\n", "write",
                                      path->stats[j].write_nbytes,
                                      path->stats[j].write_ncalls,
                                      path->stats[j].write_timer.utime,
                                      path->stats[j].write_timer.stime,
                                      path->stats[j].write_timer.etime,
                                      buf);
                        }
                    }
                }
            }
#endif

            /* Free data types */
            H5I_destroy_group(H5I_DATASPACE);

            /* Clear/free conversion table */
            HDmemset(H5S_fconv_g, 0, sizeof(H5S_fconv_g));
            HDmemset(H5S_mconv_g, 0, sizeof(H5S_mconv_g));
            for (i=0; i<H5S_nconv_g; i++) H5MM_xfree(H5S_conv_g[i]);
            H5S_conv_g = H5MM_xfree(H5S_conv_g);
            H5S_nconv_g = H5S_aconv_g = 0;

            /* Shut down interface */
            interface_initialize_g = 0;
            n = 1; /*H5I*/
        }
    }
    
    return n;
}


/*-------------------------------------------------------------------------
 * Function:    H5S_register
 *
 * Purpose:     Adds information about a data space conversion to the space
 *              conversion table.  A space conversion has two halves: the
 *              half that copies data points between application memory and
 *              the type conversion array, and the half that copies points
 *              between the type conversion array and the file.  Both halves
 *              are required.
 *
 * Note:        The conversion table will contain pointers to the file and
 *              memory conversion info.  The FCONV and MCONV arguments are
 *              not copied.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August 11, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_register(H5S_sel_type cls, const H5S_fconv_t *fconv,
             const H5S_mconv_t *mconv)
{
    FUNC_ENTER(H5S_register, FAIL);

    assert(cls>=0 && cls<H5S_SEL_N);
    assert(fconv);
    assert(mconv);

    H5S_fconv_g[cls] = fconv;
    H5S_mconv_g[cls] = mconv;

    FUNC_LEAVE(SUCCEED);
}


/*--------------------------------------------------------------------------
 NAME
    H5S_create
 PURPOSE
    Create empty, typed dataspace
 USAGE
   H5S_t *H5S_create(type)
    H5S_type_t  type;           IN: Dataspace type to create
 RETURNS
    Pointer to dataspace on success, NULL on failure
 DESCRIPTION
    Creates a new dataspace of a given type.  The extent & selection are
    undefined
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5S_t *
H5S_create(H5S_class_t type)
{
    H5S_t *ret_value = NULL;

    FUNC_ENTER(H5S_create, NULL);

    /* Create a new data space */
    if((ret_value = H5FL_ALLOC(H5S_t,1))!=NULL)
    {
        ret_value->extent.type = type;
        ret_value->select.type = H5S_SEL_ALL;  /* Entire extent selected by default */
    }

#ifdef LATER
done:
#endif
    FUNC_LEAVE(ret_value);
} /* end H5S_create() */

/*--------------------------------------------------------------------------
 NAME
    H5Screate
 PURPOSE
    Create empty, typed dataspace
 USAGE
   hid_t  H5Screate(type)
    H5S_type_t  type;           IN: Dataspace type to create
 RETURNS
    Valid dataspace ID on success, negative on failure
 DESCRIPTION
    Creates a new dataspace of a given type.  The extent & selection are
    undefined
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Screate(H5S_class_t type)
{
    H5S_t  *new_ds=NULL;
    hid_t       ret_value = FAIL;

    FUNC_ENTER(H5Screate, FAIL);
    H5TRACE1("i","Sc",type);

    /* Check args */
    if(type<=H5S_NO_CLASS || type> H5S_SIMPLE)  /* don't allow complex dataspace yet */
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                   "invalid dataspace type");

    if (NULL==(new_ds=H5S_create(type))) {
        HRETURN_ERROR (H5E_DATASPACE, H5E_CANTCREATE, FAIL, "unable to create dataspace");
    }

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, new_ds))<0) {
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data space atom");
    }

done:
    if (ret_value < 0) {
    }
    FUNC_LEAVE(ret_value);
} /* end H5Screate() */

/*-------------------------------------------------------------------------
 * Function:    H5S_extent_release
 *
 * Purpose:     Releases all memory associated with a dataspace extent.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_extent_release(H5S_t *ds)
{
    FUNC_ENTER(H5S_extent_release, FAIL);

    assert(ds);

    /* release extent */
    switch (ds->extent.type) {
        case H5S_NO_CLASS:
            /*nothing needed */
            break;

        case H5S_SCALAR:
            /*nothing needed */
            break;

        case H5S_SIMPLE:
            H5S_release_simple(&(ds->extent.u.simple));
            break;

        case H5S_COMPLEX:
            /* nothing yet */
            break;

        default:
            assert("unknown dataspace (extent) type" && 0);
            break;
    }
    FUNC_LEAVE(SUCCEED);
}   /* end H5S_extent_release() */

/*-------------------------------------------------------------------------
 * Function:    H5S_close
 *
 * Purpose:     Releases all memory associated with a data space.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_close(H5S_t *ds)
{
    FUNC_ENTER(H5S_close, FAIL);

    assert(ds);

    /* If there was a previous offset for the selection, release it */
    if(ds->select.offset!=NULL)
        ds->select.offset=H5FL_ARR_FREE(hssize_t,ds->select.offset);

    /* Release selection (this should come before the extent release) */
    H5S_select_release(ds);

    /* Release extent */
    H5S_extent_release(ds);

    /* Release the main structure */
    H5FL_FREE(H5S_t,ds);

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5Sclose
 *
 * Purpose:     Release access to a data space object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Errors:
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Sclose(hid_t space_id)
{
    FUNC_ENTER(H5Sclose, FAIL);
    H5TRACE1("e","i",space_id);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) ||
        NULL == H5I_object(space_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    /* When the reference count reaches zero the resources are freed */
    if (H5I_dec_ref(space_id) < 0) {
        HRETURN_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "problem freeing id");
    }
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_release_simple
 *
 * Purpose:     Releases all memory associated with a simple data space.
 *          (but doesn't free the simple space itself)
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, April  17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_release_simple(H5S_simple_t *simple)
{
    FUNC_ENTER(H5S_release_simple, FAIL);

    assert(simple);

    if(simple->size)
        H5FL_ARR_FREE(hsize_t,simple->size);
    if(simple->max)
        H5FL_ARR_FREE(hsize_t,simple->max);
#ifdef LATER
    if(simple->perm)
        H5FL_ARR_FREE(hsize_t,simple->perm);
#endif /* LATER */

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5Scopy
 *
 * Purpose:     Copies a dataspace.
 *
 * Return:      Success:        ID of the new dataspace
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, January 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Scopy(hid_t space_id)
{
    H5S_t       *src = NULL;
    H5S_t       *dst = NULL;
    hid_t       ret_value = FAIL;
    
    FUNC_ENTER (H5Scopy, FAIL);
    H5TRACE1("i","i",space_id);

    /* Check args */
    if (H5I_DATASPACE!=H5I_get_type (space_id) || NULL==(src=H5I_object (space_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }

    /* Copy */
    if (NULL==(dst=H5S_copy (src))) {
        HRETURN_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to copy data space");
    }

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, dst))<0) {
        HRETURN_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data space atom");
    }

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Sextent_copy
 *
 * Purpose:     Copies a dataspace extent.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Sextent_copy(hid_t dst_id,hid_t src_id)
{
    H5S_t       *src = NULL;
    H5S_t       *dst = NULL;
    hid_t       ret_value = SUCCEED;
    
    FUNC_ENTER (H5Scopy, FAIL);
    H5TRACE2("e","ii",dst_id,src_id);

    /* Check args */
    if (H5I_DATASPACE!=H5I_get_type (src_id) || NULL==(src=H5I_object (src_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if (H5I_DATASPACE!=H5I_get_type (dst_id) || NULL==(dst=H5I_object (dst_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }

    /* Copy */
    if (H5S_extent_copy(&(dst->extent),&(src->extent))<0)
        HRETURN_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy extent");

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_extent_copy
 *
 * Purpose:     Copies a dataspace extent
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June  3, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_extent_copy(H5S_extent_t *dst, const H5S_extent_t *src)
{
    unsigned u;

    FUNC_ENTER(H5S_extent_copy, FAIL);

    /* Copy the regular fields */
    *dst=*src;

    switch (src->type) {
        case H5S_SCALAR:
            /*nothing needed */
            break;

        case H5S_SIMPLE:
            if (src->u.simple.size) {
                dst->u.simple.size = H5FL_ARR_ALLOC(hsize_t,(hsize_t)src->u.simple.rank,0);
                for (u = 0; u < src->u.simple.rank; u++) {
                    dst->u.simple.size[u] = src->u.simple.size[u];
                }
            }
            if (src->u.simple.max) {
                dst->u.simple.max = H5FL_ARR_ALLOC(hsize_t,(hsize_t)src->u.simple.rank,0);
                for (u = 0; u < src->u.simple.rank; u++) {
                    dst->u.simple.max[u] = src->u.simple.max[u];
                }
            }
            break;

        case H5S_COMPLEX:
            /*void */
            break;

        default:
            assert("unknown data space type" && 0);
            break;
    }

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_copy
 *
 * Purpose:     Copies a data space, by copying the extent and selection through
 *          H5S_extent_copy and H5S_select_copy
 *
 * Return:      Success:        A pointer to a new copy of SRC
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, December  4, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5S_t *
H5S_copy(const H5S_t *src)
{
    H5S_t                  *dst = NULL;

    FUNC_ENTER(H5S_copy, NULL);

    if (NULL==(dst = H5FL_ALLOC(H5S_t,0))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    }
    *dst = *src;

    /* Copy the source dataspace's extent */
    if (H5S_extent_copy(&(dst->extent),&(src->extent))<0)
        HRETURN_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy extent");

    /* Copy the source dataspace's selection */
    if (H5S_select_copy(dst,src)<0)
        HRETURN_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy select");

    FUNC_LEAVE(dst);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_get_simple_extent_npoints
 *
 * Purpose:     Determines how many data points a dataset extent has.
 *
 * Return:      Success:        Number of data points in the dataset extent.
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *  Changed Name - QAK 7/7/98
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5S_get_simple_extent_npoints(const H5S_t *ds)
{
    hssize_t    ret_value = -1;
    unsigned            u;

    FUNC_ENTER(H5S_get_simple_extent_npoints, -1);

    /* check args */
    assert(ds);

    switch (ds->extent.type) {
        case H5S_SCALAR:
            ret_value = 1;
            break;

        case H5S_SIMPLE:
            for (ret_value=1, u=0; u<ds->extent.u.simple.rank; u++) {
                ret_value *= ds->extent.u.simple.size[u];
            }
            break;

        case H5S_COMPLEX:
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, -1,
                  "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, -1,
                  "internal error (unknown data space class)");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Sget_simple_extent_npoints
 *
 * Purpose:     Determines how many data points a dataset extent has.
 *
 * Return:      Success:        Number of data points in the dataset.
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *  Changed Name - QAK 7/7/98
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5Sget_simple_extent_npoints(hid_t space_id)
{
    H5S_t                  *ds = NULL;
    hssize_t                ret_value = -1;

    FUNC_ENTER(H5Sget_simple_extent_npoints, -1);
    H5TRACE1("Hs","i",space_id);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) || NULL == (ds = H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, -1, "not a data space");
    }
    ret_value = H5S_get_simple_extent_npoints(ds);

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_get_npoints_max
 *
 * Purpose:     Determines the maximum number of data points a data space may
 *              have.  If the `max' array is null then the maximum number of
 *              data points is the same as the current number of data points
 *              without regard to the hyperslab.  If any element of the `max'
 *              array is zero then the maximum possible size is returned.
 *
 * Return:      Success:        Maximum number of data points the data space
 *                              may have.
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5S_get_npoints_max(const H5S_t *ds)
{
    hsize_t         ret_value = 0;
    unsigned        u;

    FUNC_ENTER(H5S_get_npoints_max, 0);

    /* check args */
    assert(ds);

    switch (ds->extent.type) {
        case H5S_SCALAR:
            ret_value = 1;
            break;

        case H5S_SIMPLE:
            if (ds->extent.u.simple.max) {
                for (ret_value=1, u=0; u<ds->extent.u.simple.rank; u++) {
                    if (H5S_UNLIMITED==ds->extent.u.simple.max[u]) {
                        ret_value = HSIZET_MAX;
                        break;
                    } else {
                        ret_value *= ds->extent.u.simple.max[u];
                    }
                }
            } else {
                for (ret_value=1, u=0; u<ds->extent.u.simple.rank; u++) {
                    ret_value *= ds->extent.u.simple.size[u];
                }
            }
            break;

        case H5S_COMPLEX:
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, 0,
                  "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, 0,
                  "internal error (unknown data space class)");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Sget_simple_extent_ndims
 *
 * Purpose:     Determines the dimensionality of a data space.
 *
 * Return:      Success:        The number of dimensions in a data space.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, December 11, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Sget_simple_extent_ndims(hid_t space_id)
{
    H5S_t                  *ds = NULL;
    int            ret_value = 0;

    FUNC_ENTER(H5Sget_simple_extent_ndims, FAIL);
    H5TRACE1("Is","i",space_id);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) ||
        NULL == (ds = H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    ret_value = H5S_get_simple_extent_ndims(ds);

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_get_simple_extent_ndims
 *
 * Purpose:     Returns the number of dimensions in a data space.
 *
 * Return:      Success:        Non-negative number of dimensions.  Zero
 *                              implies a scalar.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, December 11, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_get_simple_extent_ndims(const H5S_t *ds)
{
    int             ret_value = FAIL;

    FUNC_ENTER(H5S_get_simple_extent_ndims, FAIL);

    /* check args */
    assert(ds);

    switch (ds->extent.type) {
        case H5S_SCALAR:
            ret_value = 0;
            break;

        case H5S_SIMPLE:
            ret_value = ds->extent.u.simple.rank;
            break;

        case H5S_COMPLEX:
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL,
                  "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL,
                  "internal error (unknown data space class)");
    }

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5Sget_simple_extent_dims
 *
 * Purpose:     Returns the size and maximum sizes in each dimension of
 *              a data space DS through the DIMS and MAXDIMS arguments.
 *
 * Return:      Success:        Number of dimensions, the same value as
 *                              returned by H5Sget_simple_extent_ndims().
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, December 11, 1997
 *
 * Modifications:
 *              June 18, 1998   Albert Cheng
 *              Added maxdims argument.  Removed dims argument check
 *              since it can still return ndims even if both dims and
 *              maxdims are NULLs.
 *
 *-------------------------------------------------------------------------
 */
int
H5Sget_simple_extent_dims(hid_t space_id, hsize_t dims[]/*out*/,
                          hsize_t maxdims[]/*out*/)
{
    H5S_t                  *ds = NULL;
    int            ret_value = 0;

    FUNC_ENTER(H5Sget_simple_extent_dims, FAIL);
    H5TRACE3("Is","ixx",space_id,dims,maxdims);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) ||
        NULL == (ds = H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace");
    }
    ret_value = H5S_get_simple_extent_dims(ds, dims, maxdims);

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_get_simple_extent_dims
 *
 * Purpose:     Returns the size in each dimension of a data space.  This
 *              function may not be meaningful for all types of data spaces.
 *
 * Return:      Success:        Number of dimensions.  Zero implies scalar.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, December 11, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_get_simple_extent_dims(const H5S_t *ds, hsize_t dims[], hsize_t max_dims[])
{
    int ret_value = FAIL;
    int i;

    FUNC_ENTER(H5S_get_simple_extent_dims, FAIL);

    /* check args */
    assert(ds);

    switch (ds->extent.type) {
        case H5S_SCALAR:
            ret_value = 0;
            break;

        case H5S_SIMPLE:
            ret_value = ds->extent.u.simple.rank;
            for (i=0; i<ret_value; i++) {
                if (dims)
                    dims[i] = ds->extent.u.simple.size[i];
                if (max_dims) {
                    if (ds->extent.u.simple.max) {
                        max_dims[i] = ds->extent.u.simple.max[i];
                    } else {
                        max_dims[i] = ds->extent.u.simple.size[i];
                    }
                }
            }
            break;

        case H5S_COMPLEX:
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL,
                  "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL,
                  "internal error (unknown data space class)");
    }

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_modify
 *
 * Purpose:     Updates a data space by writing a message to an object
 *              header.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_modify(H5G_entry_t *ent, const H5S_t *ds)
{
    FUNC_ENTER(H5S_modify, FAIL);

    assert(ent);
    assert(ds);

    switch (ds->extent.type) {
        case H5S_SCALAR:
        case H5S_SIMPLE:
            if (H5O_modify(ent, H5O_SDSPACE, 0, 0, &(ds->extent.u.simple))<0) {
                HRETURN_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL,
                      "can't update simple data space message");
            }
            break;

        case H5S_COMPLEX:
            HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL,
                  "complex data spaces are not implemented yet");

        default:
            assert("unknown data space class" && 0);
            break;
    }

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_read
 *
 * Purpose:     Reads the data space from an object header.
 *
 * Return:      Success:        Pointer to a new data space.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *      Robb Matzke, 9 Jun 1998
 *      Removed the unused file argument since the file is now part of the
 *      ENT argument.
 *-------------------------------------------------------------------------
 */
H5S_t *
H5S_read(H5G_entry_t *ent)
{
    H5S_t                  *ds = NULL;

    FUNC_ENTER(H5S_read, NULL);

    /* check args */
    assert(ent);

    if (NULL==(ds = H5FL_ALLOC(H5S_t,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    
    if (H5O_read(ent, H5O_SDSPACE, 0, &(ds->extent.u.simple))) {
        ds->extent.type = H5S_SIMPLE;
    } else {
        ds->extent.type = H5S_SCALAR;
    }

    /* Default to entire dataspace being selected */
    ds->select.type=H5S_SEL_ALL;

    /* Allocate space for the offset and set it to zeros */
    if (NULL==(ds->select.offset = H5FL_ARR_ALLOC(hssize_t,(hsize_t)ds->extent.u.simple.rank,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    }

    FUNC_LEAVE(ds);
}

/*-------------------------------------------------------------------------
 * Function:    H5S_cmp
 *
 * Purpose:     Compares two data space extents.
 *
 * Return:      Success:        0 if DS1 and DS2 are the same.
 *                              <0 if DS1 is less than DS2.
 *                              >0 if DS1 is greater than DS2.
 *
 *              Failure:        0, never fails
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 10, 1997
 *
 * Modifications:
 *      6/9/98 Changed to only compare extents - QAK
 *
 *-------------------------------------------------------------------------
 */
int
H5S_cmp(const H5S_t *ds1, const H5S_t *ds2)
{
    unsigned    u;

    FUNC_ENTER(H5S_cmp, 0);

    /* check args */
    assert(ds1);
    assert(ds2);

    /* compare */
    if (ds1->extent.type < ds2->extent.type)
        HRETURN(-1);
    if (ds1->extent.type > ds2->extent.type)
        HRETURN(1);

    switch (ds1->extent.type) {
        case H5S_SIMPLE:
            if (ds1->extent.u.simple.rank < ds2->extent.u.simple.rank)
                HRETURN(-1);
            if (ds1->extent.u.simple.rank > ds2->extent.u.simple.rank)
                HRETURN(1);

            for (u = 0; u < ds1->extent.u.simple.rank; u++) {
                if (ds1->extent.u.simple.size[u] < ds2->extent.u.simple.size[u])
                    HRETURN(-1);
                if (ds1->extent.u.simple.size[u] > ds2->extent.u.simple.size[u])
                    HRETURN(1);
            }

            /* don't compare max dimensions */

#ifdef LATER
            for (u = 0; u < ds1->extent.u.simple.rank; u++) {
                if ((ds1->extent.u.simple.perm ? ds1->extent.u.simple.perm[u] : u) <
                        (ds2->extent.u.simple.perm ? ds2->extent.u.simple.perm[u] : i))
                    HRETURN(-1);
                if ((ds1->extent.u.simple.perm ? ds2->extent.u.simple.perm[u] : u) >
                        (ds2->extent.u.simple.perm ? ds2->extent.u.simple.perm[u] : i))
                    HRETURN(1);
            }
#endif

            break;

        default:
            assert("not implemented yet" && 0);
    }

    FUNC_LEAVE(0);
}


/*--------------------------------------------------------------------------
 NAME
    H5S_is_simple
 PURPOSE
    Check if a dataspace is simple (internal)
 USAGE
    htri_t H5S_is_simple(sdim)
        H5S_t *sdim;            IN: Pointer to dataspace object to query
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
        This function determines the if a dataspace is "simple". ie. if it
    has orthogonal, evenly spaced dimensions.
--------------------------------------------------------------------------*/
htri_t
H5S_is_simple(const H5S_t *sdim)
{
    htri_t                  ret_value = FAIL;

    FUNC_ENTER(H5S_is_simple, FAIL);

    /* Check args and all the boring stuff. */
    assert(sdim);
    ret_value = sdim->extent.type == H5S_SIMPLE ? TRUE : FALSE;

    FUNC_LEAVE(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Sis_simple
 PURPOSE
    Check if a dataspace is simple
 USAGE
    htri_t H5Sis_simple(space_id)
        hid_t space_id;       IN: ID of dataspace object to query
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
        This function determines the if a dataspace is "simple". ie. if it
    has orthogonal, evenly spaced dimensions.
--------------------------------------------------------------------------*/
htri_t
H5Sis_simple(hid_t space_id)
{
    H5S_t                  *space = NULL;       /* dataspace to modify */
    htri_t                  ret_value = FAIL;

    FUNC_ENTER(H5Sis_simple, FAIL);
    H5TRACE1("b","i",space_id);

    /* Check args and all the boring stuff. */
    if ((space = H5I_object(space_id)) == NULL)
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");

    ret_value = H5S_is_simple(space);

  done:
    FUNC_LEAVE(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Sset_extent_simple
 PURPOSE
    Sets the size of a simple dataspace
 USAGE
    herr_t H5Sset_extent_simple(space_id, rank, dims, max)
        hid_t space_id;       IN: Dataspace object to query
        int rank;             IN: # of dimensions for the dataspace
        const size_t *dims;   IN: Size of each dimension for the dataspace
        const size_t *max;    IN: Maximum size of each dimension for the
                                  dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function sets the number and size of each dimension in the
    dataspace. Setting RANK to a value of zero converts the dataspace to a
    scalar dataspace.  Dimensions are specified from slowest to fastest
    changing in the DIMS array (i.e. 'C' order).  Setting the size of a
    dimension in the MAX array to zero indicates that the dimension is of
    unlimited size and should be allowed to expand.  If MAX is NULL, the
    dimensions in the DIMS array are used as the maximum dimensions.
    Currently, only the first dimension in the array (the slowest) may be
    unlimited in size.
--------------------------------------------------------------------------*/
herr_t
H5Sset_extent_simple(hid_t space_id, int rank, const hsize_t dims[/*rank*/],
                      const hsize_t max[/*rank*/])
{
    H5S_t       *space = NULL;  /* dataspace to modify */
    int u;      /* local counting variable */

    FUNC_ENTER(H5Sset_extent_simple, FAIL);
    H5TRACE4("e","iIs*[a1]h*[a1]h",space_id,rank,dims,max);

    /* Check args */
    if ((space = H5I_object(space_id)) == NULL) {
        HRETURN_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");
    }
    if (rank > 0 && dims == NULL) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no dimensions specified");
    }
    if (rank<0 || rank>H5S_MAX_RANK) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid rank");
    }
    if (dims) {
        for (u=0; u<rank; u++) {
            if (((max!=NULL && max[u]!=H5S_UNLIMITED) || max==NULL)
                    && dims[u]==0) {
                HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                               "invalid dimension size");
            }
        }
    }
    if (max!=NULL) {
        if(dims==NULL) {
            HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                           "maximum dimension specified, but no current "
                           "dimensions specified");
        }
        for (u=0; u<rank; u++) {
            if (max[u]!=H5S_UNLIMITED && max[u]<dims[u]) {
                HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                               "invalid maximum dimension size");
            }
        }
    }

    /* Do it */
    if (H5S_set_extent_simple(space, rank, dims, max)<0) {
        HRETURN_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL,
                      "unable to set simple extent");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_set_extent_simple
 *
 * Purpose:     This is where the real work happens for
 *              H5Sset_extent_simple().
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke (copied from H5Sset_extent_simple)
 *              Wednesday, July  8, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_set_extent_simple (H5S_t *space, int rank, const hsize_t *dims,
                       const hsize_t *max)
{
    FUNC_ENTER(H5S_set_extent_simple, FAIL);

    /* Check args */
    assert(rank>=0 && rank<=H5S_MAX_RANK);
    assert(0==rank || dims);
    
    /* If there was a previous offset for the selection, release it */
    if(space->select.offset!=NULL)
        space->select.offset=H5FL_ARR_FREE(hssize_t,space->select.offset);

    /* Allocate space for the offset and set it to zeros */
    if (NULL==(space->select.offset = H5FL_ARR_ALLOC(hssize_t,(hsize_t)rank,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                       "memory allocation failed");
    }

    /* shift out of the previous state to a "simple" dataspace */
    switch (space->extent.type) {
        case H5S_SCALAR:
            /* do nothing */
            break;

        case H5S_SIMPLE:
            H5S_release_simple(&(space->extent.u.simple));
            break;

        case H5S_COMPLEX:
        /*
         * eventually this will destroy whatever "complex" dataspace info
         * is retained, right now it's an error
         */
        /* Fall through to report error */

        default:
            HRETURN_ERROR(H5E_DATASPACE, H5E_BADVALUE, FAIL,
                  "unknown data space class");
    }

    if (rank == 0) {            /* scalar variable */
        space->extent.type = H5S_SCALAR;
        space->extent.u.simple.rank = 0;        /* set to scalar rank */
    } else {
        space->extent.type = H5S_SIMPLE;

        /* Set the rank and copy the dims */
        space->extent.u.simple.rank = rank;
        space->extent.u.simple.size = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0);
        HDmemcpy(space->extent.u.simple.size, dims, sizeof(hsize_t) * rank);

        /* Copy the maximum dimensions if specified */
        if(max!=NULL) {
            space->extent.u.simple.max = H5FL_ARR_ALLOC(hsize_t,(hsize_t)rank,0);
            HDmemcpy(space->extent.u.simple.max, max, sizeof(hsize_t) * rank);
        } /* end if */
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_find
 *
 * Purpose:     Given two data spaces (MEM_SPACE and FILE_SPACE) this
 *              function returns a pointer to the conversion path information,
 *              creating a new conversion path entry if necessary.
 *              
 * Return:      Success:        Ptr to a conversion path entry
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January 21, 1998
 *
 * Modifications:
 *
 *      Quincey Koziol
 *      Instead of returning a point into the data space conversion table we
 *      copy all the information into a user-supplied CONV buffer and return
 *      non-negative on success or negative on failure.
 *
 *      Robb Matzke, 11 Aug 1998
 *      Returns a pointer into the conversion path table.  A path entry
 *      contains pointers to the memory and file half of the conversion (the
 *      pointers registered in the H5S_fconv_g[] and H5S_mconv_g[] tables)
 *      along with other data whose scope is the conversion path (like path
 *      statistics).
 *
 *-------------------------------------------------------------------------
 */
H5S_conv_t *
H5S_find (const H5S_t *mem_space, const H5S_t *file_space)
{
    size_t      i;
    htri_t c1,c2;
    H5S_conv_t  *path;
    
    FUNC_ENTER (H5S_find, NULL);

    /* Check args */
    assert (mem_space && (H5S_SIMPLE==mem_space->extent.type ||
                          H5S_SCALAR==mem_space->extent.type));
    assert (file_space && (H5S_SIMPLE==file_space->extent.type ||
                           H5S_SCALAR==mem_space->extent.type));

    /*
     * We can't do conversion if the source and destination select a
     * different number of data points.
     */
    if (H5S_get_select_npoints(mem_space) !=
        H5S_get_select_npoints (file_space)) {
        HRETURN_ERROR (H5E_DATASPACE, H5E_BADRANGE, NULL,
                       "memory and file data spaces are different sizes");
    }

    /*
     * Is this path already present in the data space conversion path table?
     * If so then return a pointer to that entry.
     */
    for (i=0; i<H5S_nconv_g; i++) {
        if (H5S_conv_g[i]->f->type==file_space->select.type &&
            H5S_conv_g[i]->m->type==mem_space->select.type) {
            /*
             * Initialize direct read/write functions
             */
            c1=H5S_select_contiguous(file_space);
            c2=H5S_select_contiguous(mem_space);
            if(c1==FAIL || c2==FAIL)
                HRETURN_ERROR(H5E_DATASPACE, H5E_BADRANGE, NULL,
                      "invalid check for contiguous dataspace ");

            if (c1==TRUE && c2==TRUE) {
                H5S_conv_g[i]->read = H5S_all_read;
                H5S_conv_g[i]->write = H5S_all_write;
            }
            else {
                H5S_conv_g[i]->read = NULL;
                H5S_conv_g[i]->write = NULL;
            }

            HRETURN(H5S_conv_g[i]);
        }
    }
    
    /*
     * The path wasn't found.  Do we have enough information to create a new
     * path?
     */
    if (NULL==H5S_fconv_g[file_space->select.type] ||
            NULL==H5S_mconv_g[mem_space->select.type]) {
        HRETURN_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, NULL,
                      "unable to convert between data space selections");
    }

    /*
     * Create a new path.
     */
    if (NULL==(path = H5MM_calloc(sizeof(*path)))) {
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "memory allocation failed for data space conversion "
                      "path");
    }
    path->f = H5S_fconv_g[file_space->select.type];
    path->m = H5S_mconv_g[mem_space->select.type];

    /*
     * Initialize direct read/write functions
     */
    c1=H5S_select_contiguous(file_space);
    c2=H5S_select_contiguous(mem_space);
    if(c1==FAIL || c2==FAIL)
        HRETURN_ERROR(H5E_DATASPACE, H5E_BADRANGE, NULL,
                      "invalid check for contiguous dataspace ");

    if (c1==TRUE && c2==TRUE) {
        path->read = H5S_all_read;
        path->write = H5S_all_write;
    }
    
    /*
     * Add the new path to the table.
     */
    if (H5S_nconv_g>=H5S_aconv_g) {
        size_t n = MAX(10, 2*H5S_aconv_g);
        H5S_conv_t **p = H5MM_realloc(H5S_conv_g, n*sizeof(H5S_conv_g[0]));

        if (NULL==p) {
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                  "memory allocation failed for data space conversion "
                  "path table");
        }
        H5S_aconv_g = n;
        H5S_conv_g = p;
    }
    H5S_conv_g[H5S_nconv_g++] = path;

    FUNC_LEAVE(path);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_extend
 *
 * Purpose:     Extend the dimensions of a data space.
 *
 * Return:      Success:        Number of dimensions whose size increased.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, January 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_extend (H5S_t *space, const hsize_t *size)
{
    int ret_value=0;
    unsigned    u;
    
    FUNC_ENTER (H5S_extend, FAIL);

    /* Check args */
    assert (space && H5S_SIMPLE==space->extent.type);
    assert (size);

    for (u=0; u<space->extent.u.simple.rank; u++) {
        if (space->extent.u.simple.size[u]<size[u]) {
            if (space->extent.u.simple.max &&
                    H5S_UNLIMITED!=space->extent.u.simple.max[u] &&
                    space->extent.u.simple.max[u]<size[u]) {
                HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "dimension cannot be increased");
            }
            ret_value++;
        }
    }

    /* Update */
    if (ret_value) {
        for (u=0; u<space->extent.u.simple.rank; u++) {
            if (space->extent.u.simple.size[u]<size[u]) {
                space->extent.u.simple.size[u] = size[u];
            }
        }
    }

    FUNC_LEAVE (ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5Screate_simple
 *
 * Purpose:     Creates a new simple data space object and opens it for
 *              access. The DIMS argument is the size of the simple dataset
 *              and the MAXDIMS argument is the upper limit on the size of
 *              the dataset.  MAXDIMS may be the null pointer in which case
 *              the upper limit is the same as DIMS.  If an element of
 *              MAXDIMS is H5S_UNLIMITED then the corresponding dimension is
 *              unlimited, otherwise no element of MAXDIMS should be smaller
 *              than the corresponding element of DIMS.
 *
 * Return:      Success:        The ID for the new simple data space object.
 *
 *              Failure:        Negative
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, January  27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Screate_simple(int rank, const hsize_t dims[/*rank*/],
                  const hsize_t maxdims[/*rank*/])
{
    hid_t       ret_value = FAIL;
    H5S_t       *space = NULL;
    int         i;

    FUNC_ENTER(H5Screate_simple, FAIL);
    H5TRACE3("i","Is*[a0]h*[a0]h",rank,dims,maxdims);

    /* Check arguments */
    if (rank<0) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "dimensionality cannot be negative");
    }
    if (rank>H5S_MAX_RANK) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "dimensionality is too large");
    }
    if (!dims && dims!=0) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "no dimensions specified");
    }
    /* Check whether the current dimensions are valid */
    for (i=0; i<rank; i++) {
        if (maxdims) {
            if (H5S_UNLIMITED!=maxdims[i] && maxdims[i]<dims[i]) {
                HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "maxdims is smaller than dims");
            }
            if (H5S_UNLIMITED!=maxdims[i] && dims[i]==0) {
                HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "zero sized dimension for non-unlimited dimension");
            }
        }
        else {
            if (dims[i]==0) {
                HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "zero sized dimension for non-unlimited dimension");
            }
        }
    }

    /* Create the space and set the extent */
    if(NULL==(space=H5S_create(H5S_SIMPLE))) {
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTCREATE, FAIL,
                     "can't create simple dataspace");
    }
    if(H5S_set_extent_simple(space,rank,dims,maxdims)<0) {
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL,
                     "can't set dimensions");
    }
    
    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, space))<0) {
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL,
                     "unable to register data space atom");
    }
    
 done:
    if (ret_value<0 && space) H5S_close(space);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5S_get_simple_extent_type
 *
 * Purpose:     Internal function for retrieving the type of extent for a dataspace object
 *
 * Return:      Success:        The class of the dataspace object
 *
 *              Failure:        N5S_NO_CLASS
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *              Thursday, September 28, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5S_class_t
H5S_get_simple_extent_type(const H5S_t *space)
{
    H5S_class_t ret_value = H5S_NO_CLASS;

    FUNC_ENTER(H5S_get_simple_extent_type, H5S_NO_CLASS);

    assert(space);

    ret_value=space->extent.type;
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Sget_simple_extent_type
 *
 * Purpose:     Retrieves the type of extent for a dataspace object
 *
 * Return:      Success:        The class of the dataspace object
 *
 *              Failure:        N5S_NO_CLASS
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5S_class_t
H5Sget_simple_extent_type(hid_t sid)
{
    H5S_class_t ret_value = H5S_NO_CLASS;
    H5S_t       *space = NULL;

    FUNC_ENTER(H5Sget_simple_extent_type, H5S_NO_CLASS);
    H5TRACE1("Sc","i",sid);

    /* Check arguments */
    if (H5I_DATASPACE != H5I_get_type(sid) || NULL == (space = H5I_object(sid))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5S_NO_CLASS, "not a dataspace");
    }

    ret_value=H5S_get_simple_extent_type(space);
    
    FUNC_LEAVE(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Sset_extent_none
 PURPOSE
    Resets the extent of a dataspace back to "none"
 USAGE
    herr_t H5Sset_extent_none(space_id)
        hid_t space_id;       IN: Dataspace object to reset
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
        This function resets the type of a dataspace back to "none" with no
    extent information stored for the dataspace.
--------------------------------------------------------------------------*/
herr_t
H5Sset_extent_none(hid_t space_id)
{
    H5S_t                  *space = NULL;       /* dataspace to modify */

    FUNC_ENTER(H5Sset_extent_none, FAIL);
    H5TRACE1("e","i",space_id);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) || NULL == (space = H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");
    }

    /* Clear the previous extent from the dataspace */
    if(H5S_extent_release(space)<0)
        HRETURN_ERROR(H5E_RESOURCE, H5E_CANTDELETE, FAIL, "can't release previous dataspace");

    space->extent.type=H5S_NO_CLASS;

    FUNC_LEAVE(SUCCEED);
}   /* end H5Sset_extent_none() */

/*--------------------------------------------------------------------------
 NAME
    H5Soffset_simple
 PURPOSE
    Changes the offset of a selection within a simple dataspace extent
 USAGE
    herr_t H5Soffset_simple(space_id, offset)
        hid_t space_id;         IN: Dataspace object to reset
        const hssize_t *offset; IN: Offset to position the selection at
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
        This function creates an offset for the selection within an extent, allowing
    the same shaped selection to be moved to different locations within a
    dataspace without requiring it to be re-defined.
--------------------------------------------------------------------------*/
herr_t
H5Soffset_simple(hid_t space_id, const hssize_t *offset)
{
    H5S_t                  *space = NULL;       /* dataspace to modify */

    FUNC_ENTER(H5Soffset_simple, FAIL);
    H5TRACE2("e","i*Hs",space_id,offset);

    /* Check args */
    if (H5I_DATASPACE != H5I_get_type(space_id) || NULL == (space = H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");
    }
    if (offset == NULL) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no offset specified");
    }

    /* Allocate space for new offset */
    if(space->select.offset==NULL) {
        if (NULL==(space->select.offset = H5FL_ARR_ALLOC(hssize_t,(hsize_t)space->extent.u.simple.rank,0))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                           "memory allocation failed");
        }
    }

    /* Copy the offset over */
    HDmemcpy(space->select.offset,offset,sizeof(hssize_t)*space->extent.u.simple.rank);

    FUNC_LEAVE(SUCCEED);
}   /* end H5Soffset_simple() */


/*-------------------------------------------------------------------------
 * Function:    H5S_debug
 *
 * Purpose:     Prints debugging information about a data space.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_debug(H5F_t *f, const void *_mesg, FILE *stream, int indent, int fwidth)
{
    const H5S_t *mesg = (const H5S_t*)_mesg;
    
    FUNC_ENTER(H5S_debug, FAIL);
    
    switch (mesg->extent.type) {
    case H5S_SCALAR:
        fprintf(stream, "%*s%-*s H5S_SCALAR\n", indent, "", fwidth,
                "Space class:");
        break;
        
    case H5S_SIMPLE:
        fprintf(stream, "%*s%-*s H5S_SIMPLE\n", indent, "", fwidth,
                "Space class:");
        (H5O_SDSPACE->debug)(f, &(mesg->extent.u.simple), stream,
                             indent+3, MAX(0, fwidth-3));
        break;
        
    default:
        fprintf(stream, "%*s%-*s **UNKNOWN-%ld**\n", indent, "", fwidth,
                "Space class:", (long)(mesg->extent.type));
        break;
    }

    FUNC_LEAVE(SUCCEED);
}
