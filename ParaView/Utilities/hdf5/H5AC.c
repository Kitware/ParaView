/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             hdf5cache.c
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
 *-------------------------------------------------------------------------
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"
#include "H5ACprivate.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5FLprivate.h"        /*Free Lists                              */
#include "H5MMprivate.h"

/*
 * Sorting the cache by address before flushing is sometimes faster
 * than flushing in cache order.
 */
#define H5AC_SORT_BY_ADDR

/*
 * Private file-scope variables.
 */
#define PABLO_MASK      H5AC_mask
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

#ifdef H5AC_SORT_BY_ADDR
static H5AC_t          *current_cache_g = NULL;         /*for sorting */
#endif

/* Declare a free list to manage the H5AC_t struct */
H5FL_DEFINE_STATIC(H5AC_t);

/* Declare a PQ free list to manage the cache mapping array information */
H5FL_ARR_DEFINE_STATIC(int,-1);

/* Declare a PQ free list to manage the cache slot array information */
H5FL_ARR_DEFINE_STATIC(H5AC_info_ptr_t,-1);

#ifdef H5AC_DEBUG
/* Declare a PQ free list to manage the protected slot array information */
H5FL_ARR_DEFINE_STATIC(H5AC_prot_t,-1);
#endif /* H5AC_DEBUG */


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
 *-------------------------------------------------------------------------
 */
int
H5AC_create(H5F_t *f, int size_hint)
{
    H5AC_t                 *cache = NULL;
    FUNC_ENTER(H5AC_create, FAIL);

    assert(f);
    assert(NULL == f->shared->cache);
    if (size_hint < 1) size_hint = H5AC_NSLOTS;

    if (NULL==(f->shared->cache = cache = H5FL_ALLOC(H5AC_t,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                       "memory allocation failed");
    }
    cache->nslots = size_hint;
    cache->slot = H5FL_ARR_ALLOC(H5AC_info_ptr_t,(hsize_t)cache->nslots,1);
    if (NULL==cache->slot) {
        f->shared->cache = H5FL_FREE (H5AC_t,f->shared->cache);
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
    }
#ifdef H5AC_DEBUG
    if ((cache->prot = H5FL_ARR_ALLOC(H5AC_prot_t,(hsize_t)cache->nslots,1))==NULL) {
        cache->slot = H5FL_ARR_FREE (H5AC_info_ptr_t,cache->slot);
        f->shared->cache = H5FL_FREE (H5AC_t,f->shared->cache);
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
    }
#endif /* H5AC_DEBUG */

    FUNC_LEAVE(size_hint);
}

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
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_dest(H5F_t *f)
{
    H5AC_t                 *cache = NULL;
    FUNC_ENTER(H5AC_dest, FAIL);

    assert(f);
    assert(f->shared->cache);
    cache = f->shared->cache;

    if (H5AC_flush(f, NULL, HADDR_UNDEF, TRUE) < 0) {
        HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                      "unable to flush cache");
    }
#ifdef H5AC_DEBUG
    {
        unsigned i;
        for (i=0; i<cache->nslots; i++) {
            cache->prot[i].slot = H5MM_xfree(cache->prot[i].slot);
            cache->prot[i].aprots = 0;
            cache->prot[i].nprots = 0;
        }
        cache->prot = H5FL_ARR_FREE(H5AC_prot_t,cache->prot);
    }
#endif

    cache->slot = H5FL_ARR_FREE(H5AC_info_ptr_t,cache->slot);
    cache->nslots = 0;
    f->shared->cache = cache = H5FL_FREE(H5AC_t,cache);
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5AC_find_f
 *
 * Purpose:     Given an object type and the address at which that object
 *              is located in the file, return a pointer to the object.
 *              The optional UDATA1 and UDATA2 structures are passed down to
 *              the function that is responsible for loading the object into
 *              memory.
 *
 *              The returned pointer is guaranteed to be valid until the next
 *              call to an H5AC function (if you want a pointer which is valid
 *              indefinately then see H5AC_protect()).
 *
 *              If H5AC_DEBUG is defined then this function also
 *              checks that the requested object is not currently
 *              protected since it is illegal to modify a protected object
 *              except through the pointer returned by H5AC_protect().
 *
 * Return:      Success:        Pointer to the object.  The pointer is
 *                              valid until some other cache function
 *                              is called.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Aug 1997
 *      Fails immediately if the cached object is at the correct address
 *      but is of the wrong type.  This happens if the caller doesn't know
 *      what type of object is at the address and calls this function with
 *      various type identifiers until one succeeds (cf., the debugger).
 *
 *      Robb Matzke, 30 Oct 1997
 *      Keeps track of hits, misses, and flushes per object type so we have
 *      some cache performance diagnostics.
 *
 *      Robb Matzke, 1999-07-27
 *      The ADDR argument is passed by value.
 *
 *-------------------------------------------------------------------------
 */
void *
H5AC_find_f(H5F_t *f, const H5AC_class_t *type, haddr_t addr,
            const void *udata1, void *udata2)
{
    unsigned                idx;
    herr_t                  status;
    void                   *thing = NULL;
    H5AC_flush_func_t       flush;
    H5AC_info_t           **info = NULL;
    H5AC_t                 *cache = NULL;

    FUNC_ENTER(H5AC_find, NULL);

    assert(f);
    assert(f->shared->cache);
    assert(type);
    assert(type->load);
    assert(type->flush);
    assert(H5F_addr_defined(addr));
    idx = H5AC_HASH(f, addr);
    cache = f->shared->cache;
    info = cache->slot + idx;

    /*
     * Return right away if the item is in the cache.
     */
    if ((*info) && (*info)->type == type && H5F_addr_eq((*info)->addr, addr)) {
        cache->diagnostics[type->id].nhits++;
        HRETURN(*info);
    }
    cache->diagnostics[type->id].nmisses++;

    /*
     * Fail if the item in the cache is at the correct address but is
     * of the wrong type.
     */
    if ((*info) && (*info)->type && (*info)->type != type && H5F_addr_eq((*info)->addr, addr)) {
        HRETURN_ERROR(H5E_CACHE, H5E_BADTYPE, NULL,
                      "internal error (correct address, wrong type)");
    }
#ifdef H5AC_DEBUG
    /*
     * Check that the requested thing isn't protected, for protected things
     * can only be modified through the pointer already handed out by the
     * H5AC_protect() function.
     */
    {
        H5AC_prot_t            *prot = NULL;
        int                    i;

        prot = cache->prot + idx;
        for (i = 0; i < prot->nprots; i++) {
            assert(H5F_addr_ne(addr, prot->slot[i]->addr));
        }
    }
#endif /* H5AC_DEBUG */

    /*
     * Load a new thing.  If it can't be loaded, then return an error
     * without preempting anything.
     */
    if (NULL == (thing = (type->load)(f, addr, udata1, udata2))) {
        HRETURN_ERROR(H5E_CACHE, H5E_CANTLOAD, NULL, "unable to load object");
    }
    /*
     * Free the previous cache entry if there is one.
     */
    if (*info) {
        H5AC_subid_t type_id=(*info)->type->id;  /* Remember this for later */

        flush = (*info)->type->flush;
        status = (flush)(f, TRUE, (*info)->addr, (*info));
        if (status < 0) {
            /*
             * The old thing could not be removed from the stack.
             * Release the new thing and fail.
             */
            if ((type->flush)(f, TRUE, addr, thing) < 0) {
                HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, NULL,
                              "unable to flush just-loaded object");
            }
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, NULL,
                          "unable to flush existing cached object");
        }
        cache->diagnostics[type_id].nflushes++;
    }
    /*
     * Make the cache point to the new thing.
     */
    (*info)=thing;
    (*info)->type = type;
    (*info)->addr = addr;

    FUNC_LEAVE(thing);
}

/*-------------------------------------------------------------------------
 * Function:    H5AC_compare
 *
 * Purpose:     Compare two hash entries by address.  Unused entries are
 *              all equal to one another and greater than all used entries.
 *
 * Return:      Success:        -1, 0, 1
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5AC_SORT_BY_ADDR
static int
H5AC_compare(const void *_a, const void *_b)
{
    int                    a = *((const int *) _a);
    int                    b = *((const int *) _b);

    assert(current_cache_g);

    if(NULL==current_cache_g->slot[a] || NULL == current_cache_g->slot[b]) {
        if(NULL==current_cache_g->slot[a]) {
            if (NULL == current_cache_g->slot[b]) {
                return 0;
            } else {
                return -1;
            }
        }
        else {
            return 1;
        }
    }
    else if (NULL == current_cache_g->slot[a]->type) {
        if (NULL == current_cache_g->slot[b]->type) {
            return 0;
        } else {
            return -1;
        }
    } else if (NULL == current_cache_g->slot[b]->type) {
        return 1;
    } else if (current_cache_g->slot[a]->addr < current_cache_g->slot[b]->addr) {
        return -1;
    } else if (current_cache_g->slot[a]->addr > current_cache_g->slot[b]->addr) {
        return 1;
    }
    return 0;
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5AC_flush
 *
 * Purpose:     Flushes (and destroys if DESTROY is non-zero) the specified
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
 *              Robb Matzke, 1999-07-27
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_flush(H5F_t *f, const H5AC_class_t *type, haddr_t addr, hbool_t destroy)
{
    unsigned                   i;
    herr_t                  status;
    H5AC_flush_func_t       flush=NULL;
    H5AC_info_t           **info;
#ifdef H5AC_SORT_BY_ADDR
    int                   *map = NULL;
#endif /* H5AC_SORT_BY_ADDR */
    unsigned                   nslots;
    H5AC_t                 *cache = NULL;

    FUNC_ENTER(H5AC_flush, FAIL);

    assert(f);
    assert(f->shared->cache);
    cache = f->shared->cache;

    if (!H5F_addr_defined(addr)) {

#ifdef H5AC_SORT_BY_ADDR
        /*
         * Sort the cache entries by address since flushing them in
         * ascending order by address may be much more efficient.
         */
        if (NULL==(map=H5FL_ARR_ALLOC(int,(hsize_t)cache->nslots,0))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                           "memory allocation failed");
        }
        for (i = nslots = 0; i < cache->nslots; i++) {
            if (cache->slot[i]!=NULL)
                map[nslots++] = i;
        }
        assert(NULL == current_cache_g);
        current_cache_g = cache;
        HDqsort(map, nslots, sizeof(int), H5AC_compare);
        current_cache_g = NULL;
#ifdef NDEBUG
        for (i = 1; i < nslots; i++) {
            assert(H5F_addr_lt(cache->slot[i - 1]->addr, cache->slot[i]->addr));
        }
#endif
#else /* H5AC_SORT_BY_ADDR */
        nslots = cache->nslots;
#endif /* H5AC_SORT_BY_ADDR */

        /*
         * Look at all cache entries.
         */
        for (i = 0; i < nslots; i++) {
#ifdef H5AC_SORT_BY_ADDR
            info = cache->slot + map[i];
            if (NULL == (*info))
                 break;          /*the rest are empty */
#else /* H5AC_SORT_BY_ADDR */
            info = cache->slot + i;
            if (NULL == (*info))
                continue;
#endif /* H5AC_SORT_BY_ADDR */
            if (!type || type == (*info)->type) {
                H5AC_subid_t type_id=(*info)->type->id;  /* Remember this for later */

                flush = (*info)->type->flush;
                status = (flush)(f, destroy, (*info)->addr, (*info));
                if (status < 0) {
#ifdef H5AC_SORT_BY_ADDR
                    map = H5FL_ARR_FREE(int,map);
#endif /* H5AC_SORT_BY_ADDR */
                    HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                                  "unable to flush cache");
                }
                cache->diagnostics[type_id].nflushes++;
                if (destroy)
                    (*info)= NULL;
            }
        }
#ifdef H5AC_SORT_BY_ADDR
        map = H5FL_ARR_FREE(int,map);
#endif /* H5AC_SORT_BY_ADDR */

        /*
         * If there are protected object then fail.  However, everything
         * else should have been flushed.
         */
        if (cache->nprots > 0) {
            HRETURN_ERROR(H5E_CACHE, H5E_PROTECT, FAIL,
                          "cache has protected items");
        }
    } else {
        i = H5AC_HASH(f, addr);
        if (cache->slot[i] && (!type || cache->slot[i]->type == type) &&
            H5F_addr_eq(cache->slot[i]->addr, addr)) {

            H5AC_subid_t type_id=cache->slot[i]->type->id;  /* Remember this for later */

            /*
             * Flush just this entry.
             */
            flush = cache->slot[i]->type->flush;
            status = (flush)(f, destroy, cache->slot[i]->addr,
                             cache->slot[i]);
            if (status < 0) {
                HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                              "unable to flush object");
            }
            cache->diagnostics[type_id].nflushes++;
            if (destroy)
                cache->slot[i]= NULL;
        }
    }

    FUNC_LEAVE(SUCCEED);
}

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
 *              Robb Matzke, 1999-07-27
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_set(H5F_t *f, const H5AC_class_t *type, haddr_t addr, void *thing)
{
    herr_t                  status;
    unsigned                   idx;
    H5AC_flush_func_t       flush=NULL;
    H5AC_info_t           **info = NULL;
    H5AC_t                 *cache = NULL;

    FUNC_ENTER(H5AC_set, FAIL);

    assert(f);
    assert(f->shared->cache);
    assert(type);
    assert(type->flush);
    assert(H5F_addr_defined(addr));
    assert(thing);
    idx = H5AC_HASH(f, addr);
    cache = f->shared->cache;
    info = cache->slot + idx;

#ifdef H5AC_DEBUG
    {
        H5AC_prot_t            *prot = NULL;
        int                    i;

        prot = cache->prot + idx;
        for (i = 0; i < prot->nprots; i++) {
            assert(H5F_addr_ne(addr, prot->slot[i]->addr));
        }
    }
#endif


    if ((*info)) {
        H5AC_subid_t type_id=(*info)->type->id;  /* Remember this for later */

        flush = (*info)->type->flush;
        status = (flush)(f, TRUE, (*info)->addr, (*info));
        if (status < 0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush object");
        }
        cache->diagnostics[type_id].nflushes++;
    }
    (*info)=thing;
    (*info)->type = type;
    (*info)->addr = addr;
    cache->diagnostics[type->id].ninits++;

    FUNC_LEAVE(SUCCEED);
}

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
 *              Robb Matzke, 1999-07-27
 *              The OLD_ADDR and NEW_ADDR arguments are passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_rename(H5F_t *f, const H5AC_class_t *type, haddr_t old_addr,
            haddr_t new_addr)
{
    unsigned                   old_idx, new_idx;
    H5AC_flush_func_t       flush=NULL;
    herr_t                  status;
    H5AC_t                 *cache = NULL;

    FUNC_ENTER(H5AC_rename, FAIL);

    assert(f);
    assert(f->shared->cache);
    assert(type);
    old_idx = H5AC_HASH(f, old_addr);
    new_idx = H5AC_HASH(f, new_addr);
    cache = f->shared->cache;

#ifdef H5AC_DEBUG
    {
        H5AC_prot_t            *prot = NULL;
        int                     i;

        prot = cache->prot + old_idx;
        for (i = 0; i < prot->nprots; i++) {
            assert(H5F_addr_ne(old_addr, prot->slot[i]->addr));
        }
        prot = cache->prot + new_idx;
        for (i = 0; i < prot->nprots; i++) {
            assert(H5F_addr_ne(new_addr, prot->slot[i]->addr));
        }
    }
#endif

    /*
     * We don't need to do anything if the object isn't cached or if the
     * new hash value is the same as the old one.
     */
    if (cache->slot[old_idx]->type != type ||
        H5F_addr_ne(cache->slot[old_idx]->addr, old_addr)) {
        HRETURN(SUCCEED);
    }
    if (old_idx == new_idx) {
        cache->slot[old_idx]->addr = new_addr;
        HRETURN(SUCCEED);
    }
    /*
     * Free the item from the destination cache line.
     */
    if (cache->slot[new_idx]) {
        H5AC_subid_t type_id=cache->slot[new_idx]->type->id;  /* Remember this for later */

        flush = cache->slot[new_idx]->type->flush;
        status = (flush)(f, TRUE, cache->slot[new_idx]->addr,
                         cache->slot[new_idx]);
        if (status < 0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush object");
        }
        cache->diagnostics[type_id].nflushes++;
    }
    /*
     * Move the source to the destination (it might not be cached)
     */
    cache->slot[new_idx]= cache->slot[old_idx];
    cache->slot[new_idx]->type = cache->slot[old_idx]->type;
    cache->slot[new_idx]->addr = new_addr;
    cache->slot[old_idx]= NULL;

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5AC_protect
 *
 * Purpose:     Similar to H5AC_find() except the object is removed from
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
 *              Robb Matzke, 1999-07-27
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
void *
H5AC_protect(H5F_t *f, const H5AC_class_t *type, haddr_t addr,
             const void *udata1, void *udata2)
{
    int                     idx;
    void                   *thing = NULL;
    H5AC_t                 *cache = NULL;
    H5AC_info_t           **info = NULL;

#ifdef H5AC_DEBUG
    H5AC_prot_t            *prot = NULL;

    static int ncalls = 0;
    if (0 == ncalls++) {
        if (H5DEBUG(AC)) {
            fprintf(H5DEBUG(AC), "H5AC: debugging cache (expensive)\n");
        }
    }
#endif

    FUNC_ENTER(H5AC_protect, NULL);

    /* check args */
    assert(f);
    assert(f->shared->cache);
    assert(type);
    assert(type->load);
    assert(type->flush);
    assert(H5F_addr_defined(addr));
    idx = H5AC_HASH(f, addr);
    cache = f->shared->cache;
    info = cache->slot + idx;
#ifdef H5AC_DEBUG
    prot = cache->prot + idx;
#endif /* H5AC_DEBUG */

    if ((*info) && (*info)->type == type && H5F_addr_eq((*info)->addr, addr)) {
        /*
         * The object is already cached; simply remove it from the cache.
         */
        cache->diagnostics[(*info)->type->id].nhits++;
        thing = (*info);
        (*info)->type = NULL;
        (*info)->addr = HADDR_UNDEF;
        (*info)= NULL;

    } else if ((*info) && (*info)->type && H5F_addr_eq((*info)->addr, addr)) {
        /*
         * Right address but wrong object type.
         */
        HRETURN_ERROR(H5E_CACHE, H5E_BADTYPE, NULL,
                      "internal error");

    } else {
#ifdef H5AC_DEBUG
        /*
         * Check that the requested thing isn't protected, for protected things
         * can only be modified through the pointer already handed out by the
         * H5AC_protect() function.
         */
        int                    i;

        for (i = 0; i < prot->nprots; i++) {
            assert(H5F_addr_ne(addr, prot->slot[i]->addr));
        }
#endif /* H5AC_DEBUG */

        /*
         * Load a new thing.  If it can't be loaded, then return an error
         * without preempting anything.
         */
        cache->diagnostics[type->id].nmisses++;
        if (NULL == (thing = (type->load)(f, addr, udata1, udata2))) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTLOAD, NULL,
                          "unable to load object");
        }
    }

#ifdef H5AC_DEBUG
    /*
     * Add the protected object to the protect debugging fields of the
     * cache.
     */
    if (prot->nprots >= prot->aprots) {
        size_t na = prot->aprots + 10;
        H5AC_info_t **x = H5MM_realloc(prot->slot,
                          na * sizeof(H5AC_info_t *));
        if (NULL==x) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                   "memory allocation failed");
        }
        prot->aprots = (int)na;
        prot->slot = x;
    }
    prot->slot[prot->nprots]= thing;
    prot->slot[prot->nprots]->type = type;
    prot->slot[prot->nprots]->addr = addr;
    prot->nprots += 1;
#endif /* H5AC_DEBUG */

    cache->nprots += 1;
    FUNC_LEAVE(thing);
}

/*-------------------------------------------------------------------------
 * Function:    H5AC_unprotect
 *
 * Purpose:     This function should be called to undo the effect of
 *              H5AC_protect().  The TYPE and ADDR arguments should be the
 *              same as the corresponding call to H5AC_protect() and the
 *              THING argument should be the value returned by H5AC_protect().
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
 *              Robb Matzke, 1999-07-27
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unprotect(H5F_t *f, const H5AC_class_t *type, haddr_t addr, void *thing)
{
    herr_t                  status;
    unsigned                   idx;
    H5AC_flush_func_t       flush=NULL;
    H5AC_t                 *cache = NULL;
    H5AC_info_t           **info = NULL;

    FUNC_ENTER(H5AC_unprotect, FAIL);

    /* check args */
    assert(f);
    assert(f->shared->cache);
    assert(type);
    assert(type->flush);
    assert(H5F_addr_defined(addr));
    assert(thing);
    idx = H5AC_HASH(f, addr);
    cache = f->shared->cache;
    info = cache->slot + idx;

    /*
     * Flush any object already in the cache at that location.  It had
     * better not be another copy of the protected object.
     */
    if (*info) {
        H5AC_subid_t type_id=(*info)->type->id;  /* Remember this for later */

        assert(H5F_addr_ne((*info)->addr, addr));
        flush = (*info)->type->flush;
        status = (flush)(f, TRUE, (*info)->addr, (*info));
        if (status < 0) {
            HRETURN_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL,
                          "unable to flush object");
        }
        cache->diagnostics[type_id].nflushes++;
    }
#ifdef H5AC_DEBUG
    /*
     * Remove the object's protect data to indicate that it is no longer
     * protected.
     */
    {
        H5AC_prot_t            *prot = NULL;
        int                     found, i;

        prot = cache->prot + idx;
        for (i = 0, found = FALSE; i < prot->nprots && !found; i++) {
            if (H5F_addr_eq(addr, prot->slot[i]->addr)) {
                assert(prot->slot[i]->type == type);
                HDmemmove(prot->slot + i, prot->slot + i + 1,
                          ((prot->nprots - i) - 1) * sizeof(H5AC_info_t *));
                prot->nprots -= 1;
                found = TRUE;
            }
        }
        assert(found);
    }
#endif /* H5AC_DEBUG */

    /*
     * Insert the object back into the cache; it is no longer protected.
     */
    (*info)=thing;
    (*info)->type = type;
    (*info)->addr = addr;
    cache->nprots -= 1;

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5AC_debug
 *
 * Purpose:     Prints debugging info about the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, October 30, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_debug(H5F_t UNUSED *f)
{
#ifdef H5AC_DEBUG
    H5AC_subid_t            i;
    char                    s[32], ascii[32];
    H5AC_t                 *cache = f->shared->cache;
    double                  miss_rate;
#endif

    FUNC_ENTER(H5AC_debug, FAIL);

#ifdef H5AC_DEBUG
    if (H5DEBUG(AC)) {
        fprintf(H5DEBUG(AC), "H5AC: meta data cache statistics for file %s\n",
                f->name);
        fprintf(H5DEBUG(AC), "   %-18s %8s %8s %8s %8s+%-8s\n",
                "Layer", "Hits", "Misses", "MissRate", "Inits", "Flushes");
        fprintf(H5DEBUG(AC), "   %-18s %8s %8s %8s %8s-%-8s\n",
                "-----", "----", "------", "--------", "-----", "-------");

        for (i = H5AC_BT_ID; i < H5AC_NTYPES; i++) {

            switch (i) {
            case H5AC_BT_ID:
                HDstrcpy(s, "B-tree nodes");
                break;
            case H5AC_SNODE_ID:
                HDstrcpy(s, "symbol table nodes");
                break;
            case H5AC_LHEAP_ID:
                HDstrcpy (s, "local heaps");
                break;
            case H5AC_GHEAP_ID:
                HDstrcpy (s, "global heaps");
                break;
            case H5AC_OHDR_ID:
                HDstrcpy(s, "object headers");
                break;
            default:
                sprintf(s, "unknown id %d", i);
            }

            if (cache->diagnostics[i].nhits>0 ||
                cache->diagnostics[i].nmisses>0) {
                miss_rate = 100.0 * cache->diagnostics[i].nmisses /
                            (cache->diagnostics[i].nhits+
                             cache->diagnostics[i].nmisses);
            } else {
                miss_rate = 0.0;
            }

            if (miss_rate > 100) {
                sprintf(ascii, "%7d%%", (int) (miss_rate + 0.5));
            } else {
                sprintf(ascii, "%7.2f%%", miss_rate);
            }
            fprintf(H5DEBUG(AC), "   %-18s %8u %8u %7s %8u%+-9ld\n", s,
                    cache->diagnostics[i].nhits,
                    cache->diagnostics[i].nmisses,
                    ascii,
                    cache->diagnostics[i].ninits,
                    ((long)(cache->diagnostics[i].nflushes) -
                     (long)(cache->diagnostics[i].ninits)));
        }
    }
#endif

    FUNC_LEAVE(SUCCEED);
    f = 0;
}
