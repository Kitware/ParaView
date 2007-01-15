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

/*-------------------------------------------------------------------------
 *
 * Created:             H5AC.c
 *                      Jul  9 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Functions in this file implement a cache for
 *                      things which exist on disk.  All "things" associated
 *                      with a particular HDF file share the same cache; each
 *                      HDF file has it's own cache.
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Aug 1997
 *      Added calls to H5E.
 *
 *      Quincey Koziol, 22 Apr 2000
 *      Turned on "H5AC_SORT_BY_ADDR"
 *
 *  John Mainzer, 5/19/04
 *   Complete redesign and rewrite.  See the header comments for
 *      H5AC_t for an overview of what is going on.
 *
 *  John Mainzer, 6/4/04
 *  Factored the new cache code into a separate file (H5C.c) to
 *  facilitate re-use.  Re-worked this file again to use H5C.
 *
 *-------------------------------------------------------------------------
 */

#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5AC_init_interface

#include "H5private.h"    /* Generic Functions      */
#include "H5ACprivate.h"  /* Metadata cache      */
#include "H5Dprivate.h"    /* Dataset functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fpkg.h"    /* Files        */
#include "H5FDprivate.h"  /* File drivers        */
#include "H5Iprivate.h"    /* IDs            */
#include "H5Pprivate.h"         /* Property lists                       */


/*
 * Private file-scope variables.
 */

/* Default dataset transfer property list for metadata I/O calls */
/* (Collective set, "block before metadata write" set and "library internal" set) */
/* (Global variable definition, declaration is in H5ACprivate.h also) */
hid_t H5AC_dxpl_id=(-1);

/* Private dataset transfer property list for metadata I/O calls */
/* (Collective set and "library internal" set) */
/* (Static variable definition) */
static hid_t H5AC_noblock_dxpl_id=(-1);

/* Dataset transfer property list for independent metadata I/O calls */
/* (just "library internal" set - i.e. independent transfer mode) */
/* (Global variable definition, declaration is in H5ACprivate.h also) */
hid_t H5AC_ind_dxpl_id=(-1);


/*
 * Private file-scope function declarations:
 */

static herr_t H5AC_check_if_write_permitted(const H5F_t *f,
                                            hid_t dxpl_id,
                                            hbool_t * write_permitted_ptr);


/*-------------------------------------------------------------------------
 * Function:  H5AC_init
 *
 * Purpose:  Initialize the interface from some other layer.
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, January 18, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_init(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5AC_init, FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5AC_init_interface
 *
 * Purpose:  Initialize interface-specific information
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 18, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5AC_init_interface(void)
{
#ifdef H5_HAVE_PARALLEL
    H5P_genclass_t  *xfer_pclass;   /* Dataset transfer property list class object */
    H5P_genplist_t  *xfer_plist;    /* Dataset transfer property list object */
    unsigned block_before_meta_write; /* "block before meta write" property value */
    unsigned library_internal=1;    /* "library internal" property value */
    H5FD_mpio_xfer_t xfer_mode;     /* I/O transfer mode property value */
    herr_t ret_value=SUCCEED;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5AC_init_interface)

    /* Sanity check */
    HDassert(H5P_CLS_DATASET_XFER_g!=(-1));

    /* Get the dataset transfer property list class object */
    if (NULL == (xfer_pclass = H5I_object(H5P_CLS_DATASET_XFER_g)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get property list class")

    /* Get an ID for the blocking, collective H5AC dxpl */
    if ((H5AC_dxpl_id=H5P_create_id(xfer_pclass)) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "unable to register property list")

    /* Get the property list object */
    if (NULL == (xfer_plist = H5I_object(H5AC_dxpl_id)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get new property list object")

    /* Insert 'block before metadata write' property */
    block_before_meta_write=1;
    if(H5P_insert(xfer_plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,H5AC_BLOCK_BEFORE_META_WRITE_SIZE,&block_before_meta_write,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Insert 'library internal' property */
    if(H5P_insert(xfer_plist,H5AC_LIBRARY_INTERNAL_NAME,H5AC_LIBRARY_INTERNAL_SIZE,&library_internal,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Set the transfer mode */
    xfer_mode=H5FD_MPIO_COLLECTIVE;
    if (H5P_set(xfer_plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

    /* Get an ID for the non-blocking, collective H5AC dxpl */
    if ((H5AC_noblock_dxpl_id=H5P_create_id(xfer_pclass)) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "unable to register property list")

    /* Get the property list object */
    if (NULL == (xfer_plist = H5I_object(H5AC_noblock_dxpl_id)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get new property list object")

    /* Insert 'block before metadata write' property */
    block_before_meta_write=0;
    if(H5P_insert(xfer_plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,H5AC_BLOCK_BEFORE_META_WRITE_SIZE,&block_before_meta_write,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Insert 'library internal' property */
    if(H5P_insert(xfer_plist,H5AC_LIBRARY_INTERNAL_NAME,H5AC_LIBRARY_INTERNAL_SIZE,&library_internal,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Set the transfer mode */
    xfer_mode=H5FD_MPIO_COLLECTIVE;
    if (H5P_set(xfer_plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

    /* Get an ID for the non-blocking, independent H5AC dxpl */
    if ((H5AC_ind_dxpl_id=H5P_create_id(xfer_pclass)) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "unable to register property list")

    /* Get the property list object */
    if (NULL == (xfer_plist = H5I_object(H5AC_ind_dxpl_id)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get new property list object")

    /* Insert 'block before metadata write' property */
    block_before_meta_write=0;
    if(H5P_insert(xfer_plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,H5AC_BLOCK_BEFORE_META_WRITE_SIZE,&block_before_meta_write,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Insert 'library internal' property */
    if(H5P_insert(xfer_plist,H5AC_LIBRARY_INTERNAL_NAME,H5AC_LIBRARY_INTERNAL_SIZE,&library_internal,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Set the transfer mode */
    xfer_mode=H5FD_MPIO_INDEPENDENT;
    if (H5P_set(xfer_plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_NOAPI(ret_value)

#else /* H5_HAVE_PARALLEL */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5AC_init_interface)

    /* Sanity check */
    assert(H5P_LST_DATASET_XFER_g!=(-1));

    H5AC_dxpl_id=H5P_DATASET_XFER_DEFAULT;
    H5AC_noblock_dxpl_id=H5P_DATASET_XFER_DEFAULT;
    H5AC_ind_dxpl_id=H5P_DATASET_XFER_DEFAULT;

    FUNC_LEAVE_NOAPI(SUCCEED)
#endif /* H5_HAVE_PARALLEL */
} /* end H5AC_init_interface() */


/*-------------------------------------------------------------------------
 * Function:  H5AC_term_interface
 *
 * Purpose:  Terminate this interface.
 *
 * Return:  Success:  Positive if anything was done that might
 *        affect other interfaces; zero otherwise.
 *
 *     Failure:  Negative.
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 18, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5AC_term_interface(void)
{
    int    n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5AC_term_interface)

    if (H5_interface_initialize_g) {
#ifdef H5_HAVE_PARALLEL
        if(H5AC_dxpl_id>0 || H5AC_noblock_dxpl_id>0 || H5AC_ind_dxpl_id>0) {
            /* Indicate more work to do */
            n = 1; /* H5I */

            /* Close H5AC dxpl */
            if (H5I_dec_ref(H5AC_dxpl_id) < 0 ||
                    H5I_dec_ref(H5AC_noblock_dxpl_id) < 0 ||
                    H5I_dec_ref(H5AC_ind_dxpl_id) < 0)
                H5E_clear(); /*ignore error*/
            else {
                /* Reset static IDs */
                H5AC_dxpl_id=(-1);
                H5AC_noblock_dxpl_id=(-1);
                H5AC_ind_dxpl_id=(-1);

                /* Reset interface initialization flag */
                H5_interface_initialize_g = 0;
            } /* end else */
        } /* end if */
        else
#else /* H5_HAVE_PARALLEL */
            /* Reset static IDs */
            H5AC_dxpl_id=(-1);
            H5AC_noblock_dxpl_id=(-1);
            H5AC_ind_dxpl_id=(-1);

#endif /* H5_HAVE_PARALLEL */
            /* Reset interface initialization flag */
            H5_interface_initialize_g = 0;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5AC_term_interface() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_create
 *
 * Purpose:     Initialize the cache just after a file is opened.  The
 *              SIZE_HINT is the number of cache slots desired.  If you
 *              pass an invalid value then H5AC_NSLOTS is used.  You can
 *              turn off caching by using 1 for the SIZE_HINT value.
 *
 * Return:      Success:        Number of slots actually used.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *
 *    Complete re-design and re-write to support the re-designed
 *    metadata cache.
 *
 *    At present, the size_hint is ignored, and the
 *    max_cache_size and min_clean_size fields are hard
 *    coded.  This should be fixed, but a parameter
 *    list change will be required, so I will leave it
 *    for now.
 *
 *    Since no-one seems to care, the function now returns
 *    one on success.
 *            JRM - 4/28/04
 *
 *    Reworked the function again after abstracting its guts to
 *    the similar function in H5C.c.  The function is now a
 *    wrapper for H5C_create().
 *            JRM - 6/4/04
 *-------------------------------------------------------------------------
 */

static const char * H5AC_entry_type_names[H5AC_NTYPES] =
{
    "B-tree nodes",
    "symbol table nodes",
    "local heaps",
    "global heaps",
    "object headers"
};

herr_t
H5AC_create(const H5F_t *f, int UNUSED size_hint)
{
    int ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_create, FAIL)

    HDassert(f);
    HDassert(NULL == f->shared->cache);

    /* this is test code that should be removed when we start passing
     * in proper size hints.
     *                                             -- JRM
     */
    f->shared->cache = H5C_create(H5C__DEFAULT_MAX_CACHE_SIZE,
                           H5C__DEFAULT_MIN_CLEAN_SIZE,
                           (H5AC_NTYPES - 1),
                           (const char **)H5AC_entry_type_names,
                           H5AC_check_if_write_permitted);

    if ( NULL == f->shared->cache ) {

  HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_create() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_dest
 *
 * Purpose:     Flushes all data to disk and destroys the cache.
 *              This function fails if any object are protected since the
 *              resulting file might not be consistent.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *
 *    Complete re-design and re-write to support the re-designed
 *    metadata cache.
 *                                                 JRM - 5/12/04
 *
 *    Abstracted the guts of the function to H5C_dest() in H5C.c,
 *    and then re-wrote the function as a wrapper for H5C_dest().
 *
 *                                                 JRM - 6/7/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_dest(H5F_t *f, hid_t dxpl_id)
{
    H5AC_t *cache = NULL;
    herr_t ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_dest, FAIL)

    assert(f);
    assert(f->shared->cache);
    cache = f->shared->cache;

    f->shared->cache = NULL;

    if ( H5C_dest(f, dxpl_id, H5AC_noblock_dxpl_id, cache) < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTFREE, FAIL, "can't destroy cache")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_dest() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_flush
 *
 * Purpose:  Flush (and possibly destroy) the metadata cache associated
 *    with the specified file.
 *
 *    This is a re-write of an earlier version of the function
 *    which was reputedly capable of flushing (and destroying
 *    if requested) individual entries, individual entries if
 *    they match the supplied type, all entries of a given type,
 *    as well as all entries in the cache.
 *
 *    As only this last capability is actually used at present,
 *    I have not implemented the other capabilities in this
 *    version of the function.
 *
 *    The type and addr parameters are retained to avoid source
 *    code changed, but values other than NULL and HADDR_UNDEF
 *    respectively are errors.  If all goes well, they should
 *    be removed, and the function renamed to something more
 *    descriptive -- perhaps H5AC_flush_cache.
 *
 *    If the cache contains protected entries, the function will
 *    fail, as protected entries cannot be flushed.  However
 *    all unprotected entries should be flushed before the
 *    function returns failure.
 *
 *    For historical purposes, the original version of the
 *    purpose section is reproduced below:
 *
 *              ============ Original Version of "Purpose:" ============
 *
 *              Flushes (and destroys if DESTROY is non-zero) the specified
 *              entry from the cache.  If the entry TYPE is CACHE_FREE and
 *              ADDR is HADDR_UNDEF then all types of entries are
 *              flushed. If TYPE is CACHE_FREE and ADDR is defined then
 *              whatever is cached at ADDR is flushed.  Otherwise the thing
 *              at ADDR is flushed if it is the correct type.
 *
 *              If there are protected objects they will not be flushed.
 *              However, an attempt will be made to flush all non-protected
 *              items before this function returns failure.
 *
 * Return:      Non-negative on success/Negative on failure if there was a
 *              request to flush all items and something was protected.
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *     Robb Matzke, 1999-07-27
 *    The ADDR argument is passed by value.
 *
 *    Complete re-write. See above for details.  -- JRM 5/11/04
 *
 *    Abstracted the guts of the function to H5C_dest() in H5C.c,
 *    and then re-wrote the function as a wrapper for H5C_dest().
 *
 *                                                 JRM - 6/7/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_flush(H5F_t *f, hid_t dxpl_id, unsigned flags)
{
    herr_t status;
    herr_t ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_flush, FAIL)

    HDassert(f);
    HDassert(f->shared->cache);

    status = H5C_flush_cache(f,
                             dxpl_id,
                             H5AC_noblock_dxpl_id,
                             f->shared->cache,
                             flags);

    if ( status < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush entry.")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_flush() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_set
 *
 * Purpose:     Adds the specified thing to the cache.  The thing need not
 *              exist on disk yet, but it must have an address and disk
 *              space reserved.
 *
 *              If H5AC_DEBUG is defined then this function checks
 *              that the object being inserted isn't a protected object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *     Robb Matzke, 1999-07-27
 *    The ADDR argument is passed by value.
 *
 *    Bill Wendling, 2003-09-16
 *    Added automatic "flush" if the FPHDF5 driver is being
 *    used. This'll write the metadata to the SAP where other,
 *    lesser processes can grab it.
 *
 *    JRM - 5/13/04
 *    Complete re-write for the new metadata cache.  The new
 *    code is functionally almost identical to the old, although
 *    the sanity check for a protected entry is now an assert
 *    at the beginning of the function.
 *
 *    JRM - 6/7/04
 *    Abstracted the guts of the function to H5C_insert_entry()
 *    in H5C.c, and then re-wrote the function as a wrapper for
 *    H5C_insert_entry().
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_set(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr, void *thing)
{
    herr_t    result;
    H5AC_info_t        *info;
    H5AC_t             *cache;
    herr_t ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_set, FAIL)

    HDassert(f);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->flush);
    HDassert(type->size);
    HDassert(H5F_addr_defined(addr));
    HDassert(thing);

    /* Get local copy of this information */
    cache = f->shared->cache;
    info = (H5AC_info_t *)thing;

    info->addr = addr;
    info->type = type;
    info->is_protected = FALSE;

    result = H5C_insert_entry(f,
                              dxpl_id,
                              H5AC_noblock_dxpl_id,
                              cache,
                              type,
                              addr,
                              thing);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, "H5C_insert_entry() failed")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_set() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_rename
 *
 * Purpose:     Use this function to notify the cache that an object's
 *              file address changed.
 *
 *              If H5AC_DEBUG is defined then this function checks
 *              that the old and new addresses don't correspond to the
 *              address of a protected object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *     Robb Matzke, 1999-07-27
 *    The OLD_ADDR and NEW_ADDR arguments are passed by value.
 *
 *    JRM 5/17/04
 *    Complete rewrite for the new meta-data cache.
 *
 *    JRM - 6/7/04
 *    Abstracted the guts of the function to H5C_rename_entry()
 *    in H5C.c, and then re-wrote the function as a wrapper for
 *    H5C_rename_entry().
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_rename(H5F_t *f, const H5AC_class_t *type, haddr_t old_addr, haddr_t new_addr)
{
    herr_t    result;
    herr_t ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_rename, FAIL)

    HDassert(f);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(H5F_addr_defined(old_addr));
    HDassert(H5F_addr_defined(new_addr));
    HDassert(H5F_addr_ne(old_addr, new_addr));

    result = H5C_rename_entry(f->shared->cache,
                              type,
                              old_addr,
                              new_addr);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTRENAME, FAIL, \
                    "H5C_rename_entry() failed.")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_rename() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_protect
 *
 * Purpose:     If the target entry is not in the cache, load it.  If
 *    necessary, attempt to evict one or more entries to keep
 *    the cache within its maximum size.
 *
 *    Mark the target entry as protected, and return its address
 *    to the caller.  The caller must call H5AC_unprotect() when
 *    finished with the entry.
 *
 *    While it is protected, the entry may not be either evicted
 *    or flushed -- nor may it be accessed by another call to
 *    H5AC_protect.  Any attempt to do so will result in a failure.
 *
 *    This comment is a re-write of the original Purpose: section.
 *    For historical interest, the original version is reproduced
 *    below:
 *
 *    Original Purpose section:
 *
 *              Similar to H5AC_find() except the object is removed from
 *              the cache and given to the caller, preventing other parts
 *              of the program from modifying the protected object or
 *              preempting it from the cache.
 *
 *              The caller must call H5AC_unprotect() when finished with
 *              the pointer.
 *
 *              If H5AC_DEBUG is defined then we check that the
 *              requested object isn't already protected.
 *
 * Return:      Success:        Ptr to the object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Sep  2 1997
 *
 * Modifications:
 *    Robb Matzke, 1999-07-27
 *    The ADDR argument is passed by value.
 *
 *              Bill Wendling, 2003-09-10
 *              Added parameter to indicate whether this is a READ or
 *              WRITE type of protect.
 *
 *    JRM -- 5/17/04
 *    Complete re-write for the new client cache.  See revised
 *    Purpose section above.
 *
 *    JRM - 6/7/04
 *    Abstracted the guts of the function to H5C_protect()
 *    in H5C.c, and then re-wrote the function as a wrapper for
 *    H5C_protect().
 *
 *-------------------------------------------------------------------------
 */
void *
H5AC_protect(H5F_t *f,
             hid_t dxpl_id,
             const H5AC_class_t *type,
             haddr_t addr,
       const void *udata1,
             void *udata2,
             H5AC_protect_t
             UNUSED
             rw)
{
    void *    thing = NULL;
    void *    ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_protect, NULL)

    /* check args */
    HDassert(f);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->flush);
    HDassert(type->load);
    HDassert(H5F_addr_defined(addr));


    thing = H5C_protect(f,
                        dxpl_id,
                        H5AC_noblock_dxpl_id,
                        f->shared->cache,
                        type,
                        addr,
                        udata1,
                        udata2);

    if ( thing == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, "H5C_protect() failed.")
    }

    /* Set return value */
    ret_value = thing;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_protect() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_unprotect
 *
 * Purpose:  Undo an H5AC_protect() call -- specifically, mark the
 *    entry as unprotected, remove it from the protected list,
 *    and give it back to the replacement policy.
 *
 *    The TYPE and ADDR arguments must be the same as those in
 *    the corresponding call to H5AC_protect() and the THING
 *    argument must be the value returned by that call to
 *    H5AC_protect().
 *
 *    If the deleted flag is TRUE, simply remove the target entry
 *    from the cache, clear it, and free it without writing it to
 *    disk.
 *
 *    This verion of the function is a complete re-write to
 *    use the new metadata cache.  While there isn't all that
 *    much difference between the old and new Purpose sections,
 *    the original version is given below.
 *
 *    Original purpose section:
 *
 *    This function should be called to undo the effect of
 *              H5AC_protect().  The TYPE and ADDR arguments should be the
 *              same as the corresponding call to H5AC_protect() and the
 *              THING argument should be the value returned by H5AC_protect().
 *              If the DELETED flag is set, then this object has been deleted
 *              from the file and should not be returned to the cache.
 *
 *              If H5AC_DEBUG is defined then this function fails
 *              if the TYPE and ADDR arguments are not what was used when the
 *              object was protected or if the object was never protected.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Sep  2 1997
 *
 * Modifications:
 *    Robb Matzke, 1999-07-27
 *    The ADDR argument is passed by value.
 *
 *    Quincey Koziol, 2003-03-19
 *    Added "deleted" argument
 *
 *              Bill Wendling, 2003-09-18
 *              If this is an FPHDF5 driver and the data is dirty,
 *              perform a "flush" that writes the data to the SAP.
 *
 *    John Mainzer 5/19/04
 *    Complete re-write for the new metadata cache.
 *
 *    JRM - 6/7/04
 *    Abstracted the guts of the function to H5C_unprotect()
 *    in H5C.c, and then re-wrote the function as a wrapper for
 *    H5C_unprotect().
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unprotect(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr, void *thing, hbool_t deleted)
{
    herr_t    result;
    herr_t                  ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_unprotect, FAIL)

    HDassert(f);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->clear);
    HDassert(type->flush);
    HDassert(H5F_addr_defined(addr));
    HDassert(thing);
    HDassert( ((H5AC_info_t *)thing)->addr == addr );
    HDassert( ((H5AC_info_t *)thing)->type == type );

    result = H5C_unprotect(f,
                           dxpl_id,
                           H5AC_noblock_dxpl_id,
                           f->shared->cache,
                           type,
                           addr,
                           thing,
                           deleted);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                    "H5C_unprotect() failed.")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_unprotect() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_stats
 *
 * Purpose:     Prints statistics about the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, October 30, 1997
 *
 * Modifications:
 *    John Mainzer 5/19/04
 *    Re-write to support the new metadata cache.
 *
 *    JRM - 6/7/04
 *    Abstracted the guts of the function to H5C_stats()
 *    in H5C.c, and then re-wrote the function as a wrapper for
 *    H5C_stats().
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_stats(const H5F_t *f)
{
    herr_t    ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5AC_stats, FAIL)

    HDassert(f);
    HDassert(f->shared->cache);

    (void)H5C_stats(f->shared->cache, f->name, FALSE); /* at present, this can't fail */

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_stats() */


/*************************************************************************/
/**************************** Private Functions: *************************/
/*************************************************************************/

/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_check_if_write_permitted
 *
 * Purpose:     Determine if a write is permitted under the current
 *    circumstances, and set *write_permitted_ptr accordingly.
 *    As a general rule it is, but when we are running in parallel
 *    mode with collective I/O, we must ensure that a read cannot
 *    cause a write.
 *
 *    In the event of failure, the value of *write_permitted_ptr
 *    is undefined.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 5/15/04
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_check_if_write_permitted(const H5F_t *f,
                              hid_t dxpl_id,
                              hbool_t * write_permitted_ptr)
#else /* H5_HAVE_PARALLEL */
static herr_t
H5AC_check_if_write_permitted(const H5F_t UNUSED * f,
                              hid_t UNUSED dxpl_id,
                              hbool_t * write_permitted_ptr)
#endif /* H5_HAVE_PARALLEL */
{
    hbool_t    write_permitted = TRUE;
    herr_t    ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_check_if_write_permitted, FAIL)

#ifdef H5_HAVE_PARALLEL

    if ( IS_H5FD_MPI(f) ) {

        H5P_genplist_t     *dxpl;       /* Dataset transfer property list   */
        H5FD_mpio_xfer_t    xfer_mode;  /* I/O transfer mode property value */

        /* Get the dataset transfer property list */
        if ( NULL == (dxpl = H5I_object(dxpl_id)) ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, \
                        "not a dataset creation property list")

        }

        /* Get the transfer mode property */
        if( H5P_get(dxpl, H5D_XFER_IO_XFER_MODE_NAME, &xfer_mode) < 0 ) {

            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, \
                        "can't retrieve xfer mode")

        }

        if ( xfer_mode == H5FD_MPIO_INDEPENDENT ) {

            write_permitted = FALSE;

        } else {

            HDassert(xfer_mode == H5FD_MPIO_COLLECTIVE );

        }
    }

#endif /* H5_HAVE_PARALLEL */

    *write_permitted_ptr = write_permitted;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_check_if_write_permitted() */
