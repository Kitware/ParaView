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

#define H5S_PACKAGE    /*suppress error about including H5Spkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5S_init_interface


#define _H5S_IN_H5S_C
#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free lists                           */
#include "H5Iprivate.h"    /* IDs            */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Oprivate.h"    /* Object headers        */
#include "H5Spkg.h"    /* Dataspaces         */

/* Local static function prototypes */
static H5S_t * H5S_create(H5S_class_t type);
static herr_t H5S_set_extent_simple (H5S_t *space, unsigned rank,
    const hsize_t *dims, const hsize_t *max);
static htri_t H5S_is_simple(const H5S_t *sdim);

#ifdef H5S_DEBUG
/* Names of the selection names, for debugging */
static const char *H5S_sel_names[]={
    "none", "point", "hyperslab", "all"
};

/* The path table, variable length */
static H5S_iostats_t    **H5S_iostats_g = NULL;
static size_t      H5S_aiostats_g = 0;  /*entries allocated*/
static size_t      H5S_niostats_g = 0;  /*entries used*/
#endif /* H5S_DEBUG */

#ifdef H5_HAVE_PARALLEL
/* Global vars whose value can be set from environment variable also */
hbool_t H5S_mpi_opt_types_g = TRUE;
#endif /* H5_HAVE_PARALLEL */

/* Declare a free list to manage the H5S_extent_t struct */
H5FL_DEFINE(H5S_extent_t);

/* Declare a free list to manage the H5S_t struct */
H5FL_DEFINE(H5S_t);

/* Declare a free list to manage the array's of hsize_t's */
H5FL_ARR_DEFINE(hsize_t,H5S_MAX_RANK);


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
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5S_init_interface);

    /* Initialize the atom group for the file IDs */
    if (H5I_init_group(H5I_DATASPACE, H5I_DATASPACEID_HASHSIZE,
           H5S_RESERVED_ATOMS, (H5I_free_t)H5S_close)<0)
  HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize interface");

#ifdef H5_HAVE_PARALLEL
    {
        /* Allow MPI buf-and-file-type optimizations? */
        const char *s = HDgetenv ("HDF5_MPI_OPT_TYPES");
        if (s && HDisdigit(*s))
            H5S_mpi_opt_types_g = (int)HDstrtol (s, NULL, 0);
    }
#endif /* H5_HAVE_PARALLEL */

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
    int  n=0;
#ifdef H5S_DEBUG
    size_t  i;
    int    j, nprints=0;
    H5S_iostats_t  *path=NULL;
    char  buf[256];
#endif /* H5S_DEBUG */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_term_interface);

    if (H5_interface_initialize_g) {
  if ((n=H5I_nmembers(H5I_DATASPACE))) {
      H5I_clear_group(H5I_DATASPACE, FALSE);
  } else {
#ifdef H5S_DEBUG
      /*
       * Print statistics about each conversion path.
       */
      if (H5DEBUG(S)) {
    for (i=0; i<H5S_niostats_g; i++) {
        path = H5S_iostats_g[i];
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
        H5S_sel_names[path->mtype], 0==j?'>':'<', H5S_sel_names[path->ftype]);
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
#endif /* H5S_DEBUG */

      /* Free data types */
      H5I_destroy_group(H5I_DATASPACE);

#ifdef H5S_DEBUG
      /* Clear/free conversion table */
      for (i=0; i<H5S_niostats_g; i++)
                H5MM_xfree(H5S_iostats_g[i]);
      H5S_iostats_g = H5MM_xfree(H5S_iostats_g);
      H5S_niostats_g = H5S_aiostats_g = 0;
#endif /* H5S_DEBUG */

      /* Shut down interface */
      H5_interface_initialize_g = 0;
      n = 1; /*H5I*/
  }
    }

    FUNC_LEAVE_NOAPI(n);
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
    Creates a new dataspace of a given type.  The extent is undefined and the
    selection is set to the "all" selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5S_t *
H5S_create(H5S_class_t type)
{
    H5S_t *ret_value;

    FUNC_ENTER_NOAPI(H5S_create, NULL);

    /* Create a new data space */
    if((ret_value = H5FL_MALLOC(H5S_t))!=NULL) {
        ret_value->extent.type = type;
        ret_value->extent.rank = 0;
        ret_value->extent.size = ret_value->extent.max = NULL;

        switch(type) {
            case H5S_SCALAR:
                ret_value->extent.nelem = 1;
                break;
            case H5S_SIMPLE:
                ret_value->extent.nelem = 0;
                break;
            default:
                assert("unknown dataspace (extent) type" && 0);
                break;
        } /* end switch */

        /* Start with "all" selection */
        if(H5S_select_all(ret_value,0)<0)
            HGOTO_ERROR (H5E_DATASPACE, H5E_CANTSET, NULL, "unable to set all selection");

        /* Reset common selection info pointer */
        ret_value->select.sel_info.hslab=NULL;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
    H5S_t *new_ds=NULL;         /* New dataspace structure */
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_API(H5Screate, FAIL);
    H5TRACE1("i","Sc",type);

    /* Check args */
    if(type<=H5S_NO_CLASS || type> H5S_SIMPLE)  /* don't allow complex dataspace yet */
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid dataspace type");

    if (NULL==(new_ds=H5S_create(type)))
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTCREATE, FAIL, "unable to create dataspace");

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, new_ds))<0)
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data space atom");

done:
    if (ret_value < 0) {
        if(new_ds!=NULL)
            H5S_close(new_ds);
    } /* end if */

    FUNC_LEAVE_API(ret_value);
} /* end H5Screate() */


/*-------------------------------------------------------------------------
 * Function:  H5S_extent_release
 *
 * Purpose:  Releases all memory associated with a dataspace extent.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_extent_release(H5S_extent_t *extent)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_extent_release, FAIL);

    assert(extent);

    /* Release extent */
    if(extent->type==H5S_SIMPLE) {
        if(extent->size)
            H5FL_ARR_FREE(hsize_t,extent->size);
        if(extent->max)
            H5FL_ARR_FREE(hsize_t,extent->max);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5S_extent_release() */


/*-------------------------------------------------------------------------
 * Function:  H5S_close
 *
 * Purpose:  Releases all memory associated with a data space.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_close(H5S_t *ds)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_close, FAIL);

    assert(ds);

    /* Release selection (this should come before the extent release) */
    H5S_SELECT_RELEASE(ds);

    /* Release extent */
    H5S_extent_release(&ds->extent);

    /* Release the main structure */
    H5FL_FREE(H5S_t,ds);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Sclose
 *
 * Purpose:  Release access to a data space object.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Errors:
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Sclose(hid_t space_id)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Sclose, FAIL);
    H5TRACE1("e","i",space_id);

    /* Check args */
    if (NULL == H5I_object_verify(space_id,H5I_DATASPACE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    /* When the reference count reaches zero the resources are freed */
    if (H5I_dec_ref(space_id) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "problem freeing id");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Scopy
 *
 * Purpose:  Copies a dataspace.
 *
 * Return:  Success:  ID of the new dataspace
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Friday, January 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Scopy(hid_t space_id)
{
    H5S_t  *src = NULL;
    H5S_t  *dst = NULL;
    hid_t  ret_value;

    FUNC_ENTER_API(H5Scopy, FAIL);
    H5TRACE1("i","i",space_id);

    /* Check args */
    if (NULL==(src=H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    /* Copy */
    if (NULL==(dst=H5S_copy (src, FALSE)))
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to copy data space");

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, dst))<0)
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data space atom");

done:
    if(ret_value<0) {
        if(dst!=NULL)
            H5S_close(dst);
    } /* end if */

    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Sextent_copy
 *
 * Purpose:  Copies a dataspace extent.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Sextent_copy(hid_t dst_id,hid_t src_id)
{
    H5S_t  *src = NULL;
    H5S_t  *dst = NULL;
    hid_t  ret_value = SUCCEED;

    FUNC_ENTER_API(H5Sextent_copy, FAIL);
    H5TRACE2("e","ii",dst_id,src_id);

    /* Check args */
    if (NULL==(src=H5I_object_verify(src_id, H5I_DATASPACE)))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    if (NULL==(dst=H5I_object_verify(dst_id, H5I_DATASPACE)))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    /* Copy */
    if (H5S_extent_copy(&(dst->extent),&(src->extent))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy extent");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_extent_copy
 *
 * Purpose:  Copies a dataspace extent
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June  3, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_extent_copy(H5S_extent_t *dst, const H5S_extent_t *src)
{
    unsigned u;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_extent_copy, FAIL);

    /* Copy the regular fields */
    dst->type=src->type;
    dst->nelem=src->nelem;
    dst->rank=src->rank;

    switch (src->type) {
        case H5S_SCALAR:
            dst->size=NULL;
            dst->max=NULL;
            break;

        case H5S_SIMPLE:
            if (src->size) {
                dst->size = H5FL_ARR_MALLOC(hsize_t,src->rank);
                for (u = 0; u < src->rank; u++)
                    dst->size[u] = src->size[u];
            }
            else
                dst->size=NULL;
            if (src->max) {
                dst->max = H5FL_ARR_MALLOC(hsize_t,src->rank);
                for (u = 0; u < src->rank; u++)
                    dst->max[u] = src->max[u];
            }
            else
                dst->max=NULL;
            break;

        case H5S_COMPLEX:
            /*void */
            break;

        default:
            assert("unknown data space type" && 0);
            break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_copy
 *
 * Purpose:  Copies a data space, by copying the extent and selection through
 *          H5S_extent_copy and H5S_select_copy.  If the SHARE_SELECTION flag
 *          is set, then the selection can be shared between the source and
 *          destination dataspaces.  (This should only occur in situations
 *          where the destination dataspace will immediately change to a new
 *          selection)
 *
 * Return:  Success:  A pointer to a new copy of SRC
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *    Thursday, December  4, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5S_t *
H5S_copy(const H5S_t *src, hbool_t share_selection)
{
    H5S_t       *dst = NULL;
    H5S_t       *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_copy, NULL);

    if (NULL==(dst = H5FL_MALLOC(H5S_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy the source dataspace's extent */
    if (H5S_extent_copy(&(dst->extent),&(src->extent))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy extent");

    /* Copy the source dataspace's selection */
    if (H5S_select_copy(dst,src,share_selection)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy select");

    /* Set the return value */
    ret_value=dst;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_get_simple_extent_npoints
 *
 * Purpose:  Determines how many data points a dataset extent has.
 *
 * Return:  Success:  Number of data points in the dataset extent.
 *
 *    Failure:  negative
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Note:        This routine participates in the "Inlining C function pointers"
 *              pattern, don't call it directly, use the appropriate macro
 *              defined in H5Sprivate.h.
 *
 * Modifications:
 *  Changed Name - QAK 7/7/98
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5S_get_simple_extent_npoints(const H5S_t *ds)
{
    hssize_t    ret_value;

    FUNC_ENTER_NOAPI(H5S_get_simple_extent_npoints, -1);

    /* check args */
    assert(ds);

    /* Get the number of elements in extent */
    ret_value = ds->extent.nelem;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Sget_simple_extent_npoints
 *
 * Purpose:  Determines how many data points a dataset extent has.
 *
 * Return:  Success:  Number of data points in the dataset.
 *
 *    Failure:  negative
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *  Changed Name - QAK 7/7/98
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5Sget_simple_extent_npoints(hid_t space_id)
{
    H5S_t       *ds = NULL;
    hssize_t        ret_value;

    FUNC_ENTER_API(H5Sget_simple_extent_npoints, FAIL);
    H5TRACE1("Hs","i",space_id);

    /* Check args */
    if (NULL == (ds = H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    ret_value = H5S_GET_EXTENT_NPOINTS(ds);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_get_npoints_max
 *
 * Purpose:  Determines the maximum number of data points a data space may
 *    have.  If the `max' array is null then the maximum number of
 *    data points is the same as the current number of data points
 *    without regard to the hyperslab.  If any element of the `max'
 *    array is zero then the maximum possible size is returned.
 *
 * Return:  Success:  Maximum number of data points the data space
 *        may have.
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5S_get_npoints_max(const H5S_t *ds)
{
    hsize_t      ret_value;
    unsigned      u;

    FUNC_ENTER_NOAPI(H5S_get_npoints_max, 0);

    /* check args */
    assert(ds);

    switch (H5S_GET_EXTENT_TYPE(ds)) {
        case H5S_SCALAR:
            ret_value = 1;
            break;

        case H5S_SIMPLE:
            if (ds->extent.max) {
                for (ret_value=1, u=0; u<ds->extent.rank; u++) {
                    if (H5S_UNLIMITED==ds->extent.max[u]) {
                        ret_value = HSIZET_MAX;
                        break;
                    }
                    else
                        ret_value *= ds->extent.max[u];
                }
            }
            else {
                for (ret_value=1, u=0; u<ds->extent.rank; u++)
                    ret_value *= ds->extent.size[u];
            }
            break;

        case H5S_COMPLEX:
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, 0, "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, 0, "internal error (unknown data space class)");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Sget_simple_extent_ndims
 *
 * Purpose:  Determines the dimensionality of a data space.
 *
 * Return:  Success:  The number of dimensions in a data space.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Thursday, December 11, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Sget_simple_extent_ndims(hid_t space_id)
{
    H5S_t       *ds = NULL;
    int       ret_value;

    FUNC_ENTER_API(H5Sget_simple_extent_ndims, FAIL);
    H5TRACE1("Is","i",space_id);

    /* Check args */
    if (NULL == (ds = H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    ret_value = H5S_GET_EXTENT_NDIMS(ds);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_get_simple_extent_ndims
 *
 * Purpose:  Returns the number of dimensions in a data space.
 *
 * Return:  Success:  Non-negative number of dimensions.  Zero
 *        implies a scalar.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Thursday, December 11, 1997
 *
 * Note:        This routine participates in the "Inlining C function pointers"
 *              pattern, don't call it directly, use the appropriate macro
 *              defined in H5Sprivate.h.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_get_simple_extent_ndims(const H5S_t *ds)
{
    int        ret_value;

    FUNC_ENTER_NOAPI(H5S_get_simple_extent_ndims, FAIL);

    /* check args */
    assert(ds);

    switch (H5S_GET_EXTENT_TYPE(ds)) {
        case H5S_SCALAR:
        case H5S_SIMPLE:
            ret_value = ds->extent.rank;
            break;

        case H5S_COMPLEX:
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "internal error (unknown data space class)");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Sget_simple_extent_dims
 *
 * Purpose:  Returns the size and maximum sizes in each dimension of
 *    a data space DS through  the DIMS and MAXDIMS arguments.
 *
 * Return:  Success:  Number of dimensions, the same value as
 *        returned by H5Sget_simple_extent_ndims().
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Thursday, December 11, 1997
 *
 * Modifications:
 *    June 18, 1998  Albert Cheng
 *    Added maxdims argument.  Removed dims argument check
 *    since it can still return ndims even if both dims and
 *    maxdims are NULLs.
 *
 *-------------------------------------------------------------------------
 */
int
H5Sget_simple_extent_dims(hid_t space_id, hsize_t dims[]/*out*/,
        hsize_t maxdims[]/*out*/)
{
    H5S_t       *ds = NULL;
    int       ret_value;

    FUNC_ENTER_API(H5Sget_simple_extent_dims, FAIL);
    H5TRACE3("Is","ixx",space_id,dims,maxdims);

    /* Check args */
    if (NULL == (ds = H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace");

    ret_value = H5S_get_simple_extent_dims(ds, dims, maxdims);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_get_simple_extent_dims
 *
 * Purpose:  Returns the size in each dimension of a data space.  This
 *    function may not be meaningful for all types of data spaces.
 *
 * Return:  Success:  Number of dimensions.  Zero implies scalar.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Thursday, December 11, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_get_simple_extent_dims(const H5S_t *ds, hsize_t dims[], hsize_t max_dims[])
{
    int  ret_value;
    int  i;

    FUNC_ENTER_NOAPI(H5S_get_simple_extent_dims, FAIL);

    /* check args */
    assert(ds);

    switch (H5S_GET_EXTENT_TYPE(ds)) {
        case H5S_SCALAR:
            ret_value = 0;
            break;

        case H5S_SIMPLE:
            ret_value = ds->extent.rank;
            for (i=0; i<ret_value; i++) {
                if (dims)
                    dims[i] = ds->extent.size[i];
                if (max_dims) {
                    if (ds->extent.max)
                        max_dims[i] = ds->extent.max[i];
                    else
                        max_dims[i] = ds->extent.size[i];
                }
            }
            break;

        case H5S_COMPLEX:
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "complex data spaces are not supported yet");

        default:
            assert("unknown data space class" && 0);
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "internal error (unknown data space class)");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_modify
 *
 * Purpose:  Updates a data space by writing a message to an object
 *    header.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_modify(H5G_entry_t *ent, const H5S_t *ds, hbool_t update_time, hid_t dxpl_id)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_modify, FAIL);

    assert(ent);
    assert(ds);

    switch (H5S_GET_EXTENT_TYPE(ds)) {
        case H5S_SCALAR:
        case H5S_SIMPLE:
            if (H5O_modify(ent, H5O_SDSPACE_ID, 0, 0, update_time, &(ds->extent), dxpl_id)<0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "can't update simple data space message");
            break;

        case H5S_COMPLEX:
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "complex data spaces are not implemented yet");

        default:
            assert("unknown data space class" && 0);
            break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_append
 *
 * Purpose:  Updates a data space by adding a message to an object
 *    header.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Tuesday, December 31, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_append(H5F_t *f, hid_t dxpl_id, struct H5O_t *oh, const H5S_t *ds)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_append, FAIL);

    assert(f);
    assert(oh);
    assert(ds);

    switch (H5S_GET_EXTENT_TYPE(ds)) {
        case H5S_SCALAR:
        case H5S_SIMPLE:
            if (H5O_append(f, dxpl_id, oh, H5O_SDSPACE_ID, 0, &(ds->extent))<0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "can't update simple data space message");
            break;

        case H5S_COMPLEX:
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "complex data spaces are not implemented yet");

        default:
            assert("unknown data space class" && 0);
            break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_append() */


/*-------------------------------------------------------------------------
 * Function:  H5S_read
 *
 * Purpose:  Reads the data space from an object header.
 *
 * Return:  Success:  Pointer to a new data space.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *    Tuesday, December  9, 1997
 *
 * Modifications:
 *  Robb Matzke, 9 Jun 1998
 *  Removed the unused file argument since the file is now part of the
 *  ENT argument.
 *-------------------------------------------------------------------------
 */
H5S_t *
H5S_read(const H5G_entry_t *ent, hid_t dxpl_id)
{
    H5S_t       *ds = NULL;          /* Dataspace to return */
    H5S_t       *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_read, NULL);

    /* check args */
    assert(ent);

    if (NULL==(ds = H5FL_CALLOC(H5S_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    if (H5O_read(ent, H5O_SDSPACE_ID, 0, &(ds->extent), dxpl_id) == NULL)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, NULL, "unable to load dataspace info from dataset header");

    /* Default to entire dataspace being selected */
    if(H5S_select_all(ds,0)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTSET, NULL, "unable to set all selection");

    /* Set the value for successful return */
    ret_value=ds;

done:
    if(ret_value==NULL) {
        if(ds!=NULL)
            H5FL_FREE(H5S_t,ds);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5S_is_simple
 PURPOSE
    Check if a dataspace is simple (internal)
 USAGE
    htri_t H5S_is_simple(sdim)
  H5S_t *sdim;    IN: Pointer to dataspace object to query
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
  This function determines the if a dataspace is "simple". ie. if it
    has orthogonal, evenly spaced dimensions.
--------------------------------------------------------------------------*/
static htri_t
H5S_is_simple(const H5S_t *sdim)
{
    htri_t        ret_value;

    FUNC_ENTER_NOAPI(H5S_is_simple, FAIL);

    /* Check args and all the boring stuff. */
    assert(sdim);
    ret_value = (H5S_GET_EXTENT_TYPE(sdim) == H5S_SIMPLE ||
    H5S_GET_EXTENT_TYPE(sdim) == H5S_SCALAR) ? TRUE : FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Sis_simple
 PURPOSE
    Check if a dataspace is simple
 USAGE
    htri_t H5Sis_simple(space_id)
  hid_t space_id;        IN: ID of dataspace object to query
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
  This function determines the if a dataspace is "simple". ie. if it
    has orthogonal, evenly spaced dimensions.
--------------------------------------------------------------------------*/
htri_t
H5Sis_simple(hid_t space_id)
{
    H5S_t       *space = NULL;  /* dataspace to modify */
    htri_t        ret_value;

    FUNC_ENTER_API(H5Sis_simple, FAIL);
    H5TRACE1("t","i",space_id);

    /* Check args and all the boring stuff. */
    if ((space = H5I_object_verify(space_id,H5I_DATASPACE)) == NULL)
  HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");

    ret_value = H5S_is_simple(space);

  done:
    FUNC_LEAVE_API(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Sset_extent_simple
 PURPOSE
    Sets the size of a simple dataspace
 USAGE
    herr_t H5Sset_extent_simple(space_id, rank, dims, max)
        hid_t space_id;        IN: Dataspace object to query
        int rank;        IN: # of dimensions for the dataspace
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
    H5S_t  *space = NULL;  /* dataspace to modify */
    int  u;  /* local counting variable */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Sset_extent_simple, FAIL);
    H5TRACE4("e","iIs*[a1]h*[a1]h",space_id,rank,dims,max);

    /* Check args */
    if ((space = H5I_object_verify(space_id,H5I_DATASPACE)) == NULL)
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");
    if (rank > 0 && dims == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no dimensions specified");
    if (rank<0 || rank>H5S_MAX_RANK)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid rank");
    if (dims) {
        for (u=0; u<rank; u++) {
            if (((max!=NULL && max[u]!=H5S_UNLIMITED) || max==NULL) && dims[u]==0)
                HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid dimension size");
        }
    }
    if (max!=NULL) {
        if(dims==NULL)
            HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "maximum dimension specified, but no current dimensions specified");
        for (u=0; u<rank; u++) {
            if (max[u]!=H5S_UNLIMITED && max[u]<dims[u])
                HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid maximum dimension size");
        }
    }

    /* Do it */
    if (H5S_set_extent_simple(space, (unsigned)rank, dims, max)<0)
  HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to set simple extent");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_set_extent_simple
 *
 * Purpose:  This is where the real work happens for
 *    H5Sset_extent_simple().
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke (copied from H5Sset_extent_simple)
 *              Wednesday, July  8, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_set_extent_simple (H5S_t *space, unsigned rank, const hsize_t *dims,
           const hsize_t *max)
{
    unsigned u;                 /* Local index variable */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_set_extent_simple, FAIL);

    /* Check args */
    assert(rank<=H5S_MAX_RANK);
    assert(0==rank || dims);

    /* shift out of the previous state to a "simple" dataspace.  */
    if(H5S_extent_release(&space->extent)<0)
        HGOTO_ERROR (H5E_RESOURCE, H5E_CANTFREE, FAIL, "failed to release previous dataspace extent");

    if (rank == 0) {    /* scalar variable */
        space->extent.type = H5S_SCALAR;
        space->extent.nelem = 1;
        space->extent.rank = 0;  /* set to scalar rank */
    } else {
        hsize_t nelem;  /* Number of elements in extent */

        space->extent.type = H5S_SIMPLE;

        /* Set the rank and allocate space for the dims */
        space->extent.rank = rank;
        space->extent.size = H5FL_ARR_MALLOC(hsize_t,rank);

        /* Copy the dimensions & compute the number of elements in the extent */
        for(u=0, nelem=1; u<space->extent.rank; u++) {
            space->extent.size[u]=dims[u];
            nelem*=dims[u];
        } /* end for */
        space->extent.nelem = nelem;

        /* Copy the maximum dimensions if specified */
        if(max!=NULL) {
            space->extent.max = H5FL_ARR_MALLOC(hsize_t,rank);
            HDmemcpy(space->extent.max, max, sizeof(hsize_t) * rank);
        } /* end if */
        else {
            space->extent.max = NULL;
        }
    }

    /* Selection related cleanup */

    /* Set offset to zeros */
    for(u=0; u<space->extent.rank; u++)
        space->select.offset[u]=0;

    /* If the selection is 'all', update the number of elements selected */
    if(H5S_GET_SELECT_TYPE(space)==H5S_SEL_ALL)
        if(H5S_select_all(space, FALSE)<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

#ifdef H5S_DEBUG

/*-------------------------------------------------------------------------
 * Function:  H5S_find
 *
 * Purpose:  Given two data spaces (MEM_SPACE and FILE_SPACE) this
 *    function returns a pointer to the conversion path information,
 *    creating a new conversion path entry if necessary.
 *
 * Return:  Success:  Ptr to a conversion path entry
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *    Wednesday, January 21, 1998
 *
 * Modifications:
 *
 *   Quincey Koziol
 *  Instead of returning a point into the data space conversion table we
 *  copy all the information into a user-supplied CONV buffer and return
 *  non-negative on success or negative on failure.
 *
 *   Robb Matzke, 11 Aug 1998
 *  Returns a pointer into the conversion path table.  A path entry
 *  contains pointers to the memory and file half of the conversion (the
 *  pointers registered in the H5S_fconv_g[] and H5S_mconv_g[] tables)
 *  along with other data whose scope is the conversion path (like path
 *  statistics).
 *
 *  John Mainzer, 8/30/04
 *  Modified code to check with all other processes that have the
 *  file open before OKing collective I/O.
 *
 *-------------------------------------------------------------------------
 */
H5S_iostats_t *
H5S_find (const H5S_t *mem_space, const H5S_t *file_space)
{
    H5S_iostats_t  *path=NULL;  /* Space conversion path */
    size_t  u;      /* Index variable */
    H5S_iostats_t *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_find, NULL);

    /* Check args */
    assert (mem_space && (H5S_SIMPLE==H5S_GET_EXTENT_TYPE(mem_space) ||
        H5S_SCALAR==H5S_GET_EXTENT_TYPE(mem_space)));
    assert (file_space && (H5S_SIMPLE==H5S_GET_EXTENT_TYPE(file_space) ||
         H5S_SCALAR==H5S_GET_EXTENT_TYPE(file_space)));

    /*
     * Is this path already present in the data space conversion path table?
     * If so then return a pointer to that entry.
     */
    for (u=0; u<H5S_niostats_g; u++)
        if (H5S_iostats_g[u]->ftype==H5S_GET_SELECT_TYPE(file_space) &&
                H5S_iostats_g[u]->mtype==H5S_GET_SELECT_TYPE(mem_space))
            HGOTO_DONE(H5S_iostats_g[u]);

    /*
     * The path wasn't found.  Create a new path.
     */
    if (NULL==(path = H5MM_calloc(sizeof(*path))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for data space conversion path");

    /* Initialize file & memory conversion functions */
    path->ftype = H5S_GET_SELECT_TYPE(file_space);
    path->mtype = H5S_GET_SELECT_TYPE(mem_space);

    /*
     * Add the new path to the table.
     */
    if (H5S_niostats_g>=H5S_aiostats_g) {
        size_t n = MAX(10, 2*H5S_aiostats_g);
        H5S_iostats_t **p = H5MM_realloc(H5S_iostats_g, n*sizeof(H5S_iostats_g[0]));

        if (NULL==p)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for data space conversion path table");
        H5S_aiostats_g = n;
        H5S_iostats_g = p;
    } /* end if */
    H5S_iostats_g[H5S_niostats_g++] = path;

    /* Set the return value */
    ret_value=path;

done:
    if(ret_value==NULL) {
        if(path!=NULL)
            H5MM_xfree(path);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_find() */
#endif /* H5S_DEBUG */


/*-------------------------------------------------------------------------
 * Function:  H5S_extend
 *
 * Purpose:  Extend the dimensions of a data space.
 *
 * Return:  Success:  Number of dimensions whose size increased.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *    Friday, January 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5S_extend (H5S_t *space, const hsize_t *size)
{
    int  ret_value=0;
    unsigned  u;

    FUNC_ENTER_NOAPI(H5S_extend, FAIL);

    /* Check args */
    assert (space && H5S_SIMPLE==H5S_GET_EXTENT_TYPE(space));
    assert (size);

    /* Check through all the dimensions to see if modifying the dataspace is allowed */
    for (u=0; u<space->extent.rank; u++) {
        if (space->extent.size[u]<size[u]) {
            if (space->extent.max &&
                    H5S_UNLIMITED!=space->extent.max[u] &&
                    space->extent.max[u]<size[u])
                HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "dimension cannot be increased");
            ret_value++;
        }
    }

    /* Update */
    if (ret_value) {
        hsize_t nelem;  /* Number of elements in extent */

        /* Change the dataspace size & re-compute the number of elements in the extent */
        for (u=0, nelem=1; u<space->extent.rank; u++) {
            if (space->extent.size[u]<size[u])
                space->extent.size[u] = size[u];

            nelem*=space->extent.size[u];
        }
        space->extent.nelem = nelem;

        /* If the selection is 'all', update the number of elements selected */
        if(H5S_GET_SELECT_TYPE(space)==H5S_SEL_ALL)
            if(H5S_select_all(space, FALSE)<0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Screate_simple
 *
 * Purpose:  Creates a new simple data space object and opens it for
 *    access. The DIMS argument is the size of the simple dataset
 *    and the MAXDIMS argument is the upper limit on the size of
 *    the dataset.  MAXDIMS may be the null pointer in which case
 *    the upper limit is the same as DIMS.  If an element of
 *    MAXDIMS is H5S_UNLIMITED then the corresponding dimension is
 *    unlimited, otherwise no element of MAXDIMS should be smaller
 *    than the corresponding element of DIMS.
 *
 * Return:  Success:  The ID for the new simple data space object.
 *
 *    Failure:  Negative
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Tuesday, January  27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Screate_simple(int rank, const hsize_t dims[/*rank*/],
      const hsize_t maxdims[/*rank*/])
{
    hid_t  ret_value;
    H5S_t  *space = NULL;
    int    i;

    FUNC_ENTER_API(H5Screate_simple, FAIL);
    H5TRACE3("i","Is*[a0]h*[a0]h",rank,dims,maxdims);

    /* Check arguments */
    if (rank<0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "dimensionality cannot be negative");
    if (rank>H5S_MAX_RANK)
  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dimensionality is too large");
    if (!dims && dims!=0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no dimensions specified");
    /* Check whether the current dimensions are valid */
    for (i=0; i<rank; i++) {
        if (maxdims) {
            if (H5S_UNLIMITED!=maxdims[i] && maxdims[i]<dims[i])
                HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "maxdims is smaller than dims");
            if (H5S_UNLIMITED!=maxdims[i] && dims[i]==0)
                HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "zero sized dimension for non-unlimited dimension");
        }
        else {
            if (dims[i]==0)
                HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "zero sized dimension for non-unlimited dimension");
        }
    }

    /* Create the space and set the extent */
    if(NULL==(space=H5S_create_simple((unsigned)rank,dims,maxdims)))
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace");

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, space))<0)
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace ID");

done:
    if (ret_value<0) {
        if (space!=NULL)
            H5S_close(space);
    } /* end if */

    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5S_create_simple
 *
 * Purpose:  Internal function to create simple dataspace
 *
 * Return:  Success:  The ID for the new simple data space object.
 *    Failure:  Negative
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Thursday, April  3, 2003
 *
 * Modifications:
 *              Extracted from H5Screate_simple
 *              Quincey Koziol, Thursday, April  3, 2003
 *
 *-------------------------------------------------------------------------
 */
H5S_t *
H5S_create_simple(unsigned rank, const hsize_t dims[/*rank*/],
      const hsize_t maxdims[/*rank*/])
{
    H5S_t  *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI(H5S_create_simple, NULL);

    /* Check arguments */
    assert(rank <=H5S_MAX_RANK);

    /* Create the space and set the extent */
    if(NULL==(ret_value=H5S_create(H5S_SIMPLE)))
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTCREATE, NULL, "can't create simple dataspace");
    if(H5S_set_extent_simple(ret_value,rank,dims,maxdims)<0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, NULL, "can't set dimensions");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_create_simple() */


/*-------------------------------------------------------------------------
 * Function:  H5S_raw_size
 *
 * Purpose:  Compute the 'raw' size of the extent, as stored on disk.
 *
 * Return:  Success:  non-zero
 *    Failure:  zero
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              October 14, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5S_raw_size(const H5F_t *f, const H5S_t *space)
{
    size_t      ret_value;

    FUNC_ENTER_NOAPI(H5S_raw_size, 0);

    /* Find out the size of buffer needed for extent */
    ret_value=H5O_raw_size(H5O_SDSPACE_ID, f, &(space->extent));

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_raw_size() */


/*-------------------------------------------------------------------------
 * Function:  H5S_get_simple_extent_type
 *
 * Purpose:  Internal function for retrieving the type of extent for a dataspace object
 *
 * Return:  Success:  The class of the dataspace object
 *
 *    Failure:  N5S_NO_CLASS
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Thursday, September 28, 2000
 *
 * Note:        This routine participates in the "Inlining C function pointers"
 *              pattern, don't call it directly, use the appropriate macro
 *              defined in H5Sprivate.h.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5S_class_t
H5S_get_simple_extent_type(const H5S_t *space)
{
    H5S_class_t  ret_value;

    FUNC_ENTER_NOAPI(H5S_get_simple_extent_type, H5S_NO_CLASS);

    assert(space);

    ret_value=H5S_GET_EXTENT_TYPE(space);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5Sget_simple_extent_type
 *
 * Purpose:  Retrieves the type of extent for a dataspace object
 *
 * Return:  Success:  The class of the dataspace object
 *
 *    Failure:  N5S_NO_CLASS
 *
 * Errors:
 *
 * Programmer:  Quincey Koziol
 *    Thursday, July 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5S_class_t
H5Sget_simple_extent_type(hid_t sid)
{
    H5S_class_t  ret_value;
    H5S_t  *space;

    FUNC_ENTER_API(H5Sget_simple_extent_type, H5S_NO_CLASS);
    H5TRACE1("Sc","i",sid);

    /* Check arguments */
    if (NULL == (space = H5I_object_verify(sid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5S_NO_CLASS, "not a dataspace");

    ret_value=H5S_GET_EXTENT_TYPE(space);

done:
    FUNC_LEAVE_API(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Sset_extent_none
 PURPOSE
    Resets the extent of a dataspace back to "none"
 USAGE
    herr_t H5Sset_extent_none(space_id)
        hid_t space_id;        IN: Dataspace object to reset
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
  This function resets the type of a dataspace back to "none" with no
    extent information stored for the dataspace.
--------------------------------------------------------------------------*/
herr_t
H5Sset_extent_none(hid_t space_id)
{
    H5S_t       *space = NULL;  /* dataspace to modify */
    herr_t                  ret_value=SUCCEED;  /* Return value */

    FUNC_ENTER_API(H5Sset_extent_none, FAIL);
    H5TRACE1("e","i",space_id);

    /* Check args */
    if (NULL == (space = H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");

    /* Clear the previous extent from the dataspace */
    if(H5S_extent_release(&space->extent)<0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTDELETE, FAIL, "can't release previous dataspace");

    space->extent.type=H5S_NO_CLASS;

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Sset_extent_none() */


/*--------------------------------------------------------------------------
 NAME
    H5Soffset_simple
 PURPOSE
    Changes the offset of a selection within a simple dataspace extent
 USAGE
    herr_t H5Soffset_simple(space_id, offset)
        hid_t space_id;          IN: Dataspace object to reset
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
    H5S_t       *space = NULL;  /* dataspace to modify */
    herr_t                  ret_value=SUCCEED;  /* Return value */

    FUNC_ENTER_API(H5Soffset_simple, FAIL);
    H5TRACE2("e","i*Hs",space_id,offset);

    /* Check args */
    if (NULL == (space = H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a data space");
    if (space->extent.rank==0 || H5S_GET_EXTENT_TYPE(space)==H5S_SCALAR)
        HGOTO_ERROR(H5E_ATOM, H5E_UNSUPPORTED, FAIL, "can't set offset on scalar dataspace");
    if (offset == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no offset specified");

    /* Set the selection offset */
    if(H5S_select_offset(space,offset)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "can't set offset");

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Soffset_simple() */


/*-------------------------------------------------------------------------
 * Function: H5S_set_extent
 *
 * Purpose: Modify the dimensions of a data space. Based on H5S_extend
 *
 * Return: Success: Non-negative
 *
 * Failure: Negative
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: March 13, 2002
 *
 *-------------------------------------------------------------------------
 */
int
H5S_set_extent( H5S_t *space, const hsize_t *size )
{
    unsigned u;
    herr_t ret_value=0;

    FUNC_ENTER_NOAPI( H5S_set_extent, FAIL );

    /* Check args */
    assert( space && H5S_SIMPLE==H5S_GET_EXTENT_TYPE(space) );
    assert( size);

    /* Verify that the dimensions being changed are allowed to change */
    for ( u = 0; u < space->extent.rank; u++ ) {
        if ( space->extent.max && H5S_UNLIMITED != space->extent.max[u] &&
                 space->extent.max[u]!=size[u] )
             HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,"dimension cannot be modified");
        ret_value++;
    } /* end for */

    /* Update */
    if (ret_value)
        H5S_set_extent_real(space,size);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

/*-------------------------------------------------------------------------
 * Function: H5S_has_extent
 *
 * Purpose: Determines if a simple dataspace's extent has been set (e.g.,
 *          by H5Sset_extent_simple() ).  Helps avoid write errors.
 *
 * Return: TRUE if dataspace has extent set
 *         FALSE if dataspace's extent is uninitialized
 *
 * Programmer: James Laird
 *
 * Date: July 23, 2004
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5S_has_extent(const H5S_t *ds)
{
    htri_t ret_value;
    FUNC_ENTER_NOAPI(H5S_has_extent, FAIL)

    assert(ds);

    if(ds->extent.rank==0 && ds->extent.nelem == 0)
        ret_value = FALSE;
    else
        ret_value = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function: H5S_set_extent_real
 *
 * Purpose: Modify the dimensions of a data space. Based on H5S_extend
 *
 * Return: Success: Non-negative
 *
 * Failure: Negative
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: March 13, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_set_extent_real( H5S_t *space, const hsize_t *size )
{
    hsize_t nelem;      /* Number of elements in extent */
    unsigned u;         /* Local index variable */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_set_extent_real, FAIL );

    /* Check args */
    assert(space && H5S_SIMPLE==H5S_GET_EXTENT_TYPE(space));
    assert(size);

    /* Change the dataspace size & re-compute the number of elements in the extent */
    for (u=0, nelem=1; u < space->extent.rank; u++ ) {
        space->extent.size[u] = size[u];
        nelem*=space->extent.size[u];
    } /* end for */
    space->extent.nelem = nelem;

    /* If the selection is 'all', update the number of elements selected */
    if(H5S_GET_SELECT_TYPE(space)==H5S_SEL_ALL)
        if(H5S_select_all(space, FALSE)<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_set_extent_real() */


/*-------------------------------------------------------------------------
 * Function:  H5S_debug
 *
 * Purpose:  Prints debugging information about a data space.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE *stream, int indent, int fwidth)
{
    const H5S_t  *mesg = (const H5S_t*)_mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_debug, FAIL);

    switch (H5S_GET_EXTENT_TYPE(mesg)) {
        case H5S_SCALAR:
            fprintf(stream, "%*s%-*s H5S_SCALAR\n", indent, "", fwidth,
                    "Space class:");
            break;

        case H5S_SIMPLE:
            fprintf(stream, "%*s%-*s H5S_SIMPLE\n", indent, "", fwidth,
                    "Space class:");
            H5O_debug_id(H5O_SDSPACE_ID, f, dxpl_id, &(mesg->extent), stream,
                                 indent+3, MAX(0, fwidth-3));
            break;

        default:
            fprintf(stream, "%*s%-*s **UNKNOWN-%ld**\n", indent, "", fwidth,
                    "Space class:", (long)(H5S_GET_EXTENT_TYPE(mesg)));
            break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

