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
 * Created:    H5Cprivate.h
 *      6/3/04
 *      John Mainzer
 *
 * Purpose:    Constants and typedefs available to the rest of the
 *      library.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifndef _H5Cprivate_H
#define _H5Cprivate_H

#include "H5Cpublic.h"    /*public prototypes           */

/* Pivate headers needed by this header */
#include "H5private.h"    /* Generic Functions      */
#include "H5Fprivate.h"    /* File access        */

#define H5C_DO_SANITY_CHECKS    0

/* This sanity checking constant was picked out of the air.  Increase
 * or decrease it if appropriate.  Its purposes is to detect corrupt
 * object sizes, so it probably doesn't matter if it is a bit big.
 *
 *          JRM - 5/17/04
 */
#define H5C_MAX_ENTRY_SIZE    ((size_t)(10 * 1024 * 1024))

/* H5C_COLLECT_CACHE_STATS controls overall collection of statistics
 * on cache activity.  In general, this #define should be set to 0.
 */
#define H5C_COLLECT_CACHE_STATS  0

/* H5C_COLLECT_CACHE_ENTRY_STATS controls collection of statistics
 * in individual cache entries.
 *
 * H5C_COLLECT_CACHE_ENTRY_STATS should only be defined to true if
 * H5C_COLLECT_CACHE_STATS is also defined to true.
 */
#if H5C_COLLECT_CACHE_STATS

#define H5C_COLLECT_CACHE_ENTRY_STATS  1

#else

#define H5C_COLLECT_CACHE_ENTRY_STATS  0

#endif /* H5C_COLLECT_CACHE_STATS */


#ifdef H5_HAVE_PARALLEL

/* we must maintain the clean and dirty LRU lists when we are compiled
 * with parallel support.
 */
#define H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS  1

#else /* H5_HAVE_PARALLEL */

/* The clean and dirty LRU lists don't buy us anything here -- we may
 * want them on for testing on occasion, but in general they should be
 * off.
 */
#define H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS  0

#endif /* H5_HAVE_PARALLEL */


/*
 * Class methods pertaining to caching.   Each type of cached object will
 * have a constant variable with permanent life-span that describes how
 * to cache the object.   That variable will be of type H5C_class_t and
 * have the following required fields...
 *
 * LOAD:  Loads an object from disk to memory.  The function
 *    should allocate some data structure and return it.
 *
 * FLUSH:  Writes some data structure back to disk.  It would be
 *    wise for the data structure to include dirty flags to
 *    indicate whether it really needs to be written.   This
 *    function is also responsible for freeing memory allocated
 *    by the LOAD method if the DEST argument is non-zero (by
 *              calling the DEST method).
 *
 * DEST:  Just frees memory allocated by the LOAD method.
 *
 * CLEAR:  Just marks object as non-dirty.
 *
 * SIZE:  Report the size (on disk) of the specified cache object.
 *    Note that the space allocated on disk may not be contiguous.
 */

typedef void *(*H5C_load_func_t)(H5F_t *f,
                                 hid_t dxpl_id,
                                 haddr_t addr,
                                 const void *udata1,
                                 void *udata2);
typedef herr_t (*H5C_flush_func_t)(H5F_t *f,
                                   hid_t dxpl_id,
                                   hbool_t dest,
                                   haddr_t addr,
                                   void *thing);
typedef herr_t (*H5C_dest_func_t)(H5F_t *f,
                                  void *thing);
typedef herr_t (*H5C_clear_func_t)(H5F_t *f,
                                   void *thing,
                                   hbool_t dest);
typedef herr_t (*H5C_size_func_t)(const H5F_t *f,
                                  const void *thing,
                                  size_t *size_ptr);

typedef struct H5C_class_t {
    int      id;
    H5C_load_func_t  load;
    H5C_flush_func_t  flush;
    H5C_dest_func_t  dest;
    H5C_clear_func_t  clear;
    H5C_size_func_t  size;
} H5C_class_t;


/* Type defintions of call back functions used by the cache as a whole */

typedef herr_t (*H5C_write_permitted_func_t)(const H5F_t *f,
                                             hid_t dxpl_id,
                                             hbool_t * write_permitted_ptr);


/* Default max cache size and min clean size are give here to make
 * them generally accessable.
 */

#define H5C__DEFAULT_MAX_CACHE_SIZE     ((size_t)(8 * 1024 * 1024))
#define H5C__DEFAULT_MIN_CLEAN_SIZE     ((size_t)(4 * 1024 * 1024))


/****************************************************************************
 *
 * structure H5C_cache_entry_t
 *
 * Instances of the H5C_cache_entry_t structure are used to store cache
 * entries in a hash table and sometimes in a skip list.
 * See H5SL.c for the particulars of the skip list.
 *
 * In typical application, this structure is the first field in a
 * structure to be cached.  For historical reasons, the external module
 * is responsible for managing the is_dirty field.  All other fields are
 * managed by the cache.
 *
 * The fields of this structure are discussed individually below:
 *
 *            JRM - 4/26/04
 *
 * addr:  Base address of the cache entry on disk.
 *
 * size:  Length of the cache entry on disk.  Note that unlike normal
 *    caches, the entries in this cache are of variable length.
 *    The entries should never overlap, and when we do writebacks,
 *    we will want to writeback adjacent entries where possible.
 *
 * type:  Pointer to the instance of H5C_class_t containing pointers
 *    to the methods for cache entries of the current type.  This
 *    field should be NULL when the instance of H5C_cache_entry_t
 *    is not in use.
 *
 *    The name is not particularly descriptive, but is retained
 *    to avoid changes in existing code.
 *
 * is_dirty:  Boolean flag indicating whether the contents of the cache
 *    entry has been modified since the last time it was written
 *    to disk.
 *
 *    NOTE: For historical reasons, this field is not maintained
 *          by the cache.  Instead, the module using the cache
 *          sets this flag when it modifies the entry, and the
 *          flush and clear functions supplied by that module
 *          reset the dirty when appropriate.
 *
 *          This is a bit quirky, so we may want to change this
 *          someday.  However it will require a change in the
 *          cache interface.
 *
 * is_protected: Boolean flag indicating whether this entry is protected
 *    (or locked, to use more conventional terms).  When it is
 *    protected, the entry cannot be flushed or accessed until
 *    it is unprotected (or unlocked -- again to use more
 *    conventional terms).
 *
 *    Note that protected entries are removed from the LRU lists
 *    and inserted on the protected list.
 *
 * in_slist:  Boolean flag indicating whether the entry is in the skip list
 *    As a general rule, entries are placed in the list when they
 *              are marked dirty.  However they may remain in the list after
 *              being flushed.
 *
 *
 * Fields supporting the hash table:
 *
 * Fields in the cache are indexed by a more or less conventional hash table.
 * If there are multiple entries in any hash bin, they are stored in a doubly
 * linked list.
 *
 * ht_next:  Next pointer used by the hash table to store multiple
 *    entries in a single hash bin.  This field points to the
 *    next entry in the doubly linked list of entries in the
 *    hash bin, or NULL if there is no next entry.
 *
 * ht_prev:     Prev pointer used by the hash table to store multiple
 *              entries in a single hash bin.  This field points to the
 *              previous entry in the doubly linked list of entries in
 *    the hash bin, or NULL if there is no previuos entry.
 *
 *
 * Fields supporting replacement policies:
 *
 * The cache must have a replacement policy, and it will usually be
 * necessary for this structure to contain fields supporting that policy.
 *
 * While there has been interest in several replacement policies for
 * this cache, the initial development schedule is tight.  Thus I have
 * elected to support only a modified LRU policy for the first cut.
 *
 * When additional replacement policies are added, the fields in this
 * section will be used in different ways or not at all.  Thus the
 * documentation of these fields is repeated for each replacement policy.
 *
 * Modified LRU:
 *
 * When operating in parallel mode, we must ensure that a read does not
 * cause a write.  If it does, the process will hang, as the write will
 * be collective and the other processes will not know to participate.
 *
 * To deal with this issue, I have modified the usual LRU policy by adding
 * clean and dirty LRU lists to the usual LRU list.  When reading in
 * parallel mode, we evict from the clean LRU list only.  This implies
 * that we must try to ensure that the clean LRU list is reasonably well
 * stocked.  See the comments on H5C_t in H5C.c for more details.
 *
 * Note that even if we start with a completely clean cache, a sequence
 * of protects without unprotects can empty the clean LRU list.  In this
 * case, the cache must grow temporarily.  At the next write, we will
 * attempt to evict enough entries to get the cache down to its nominal
 * maximum size.
 *
 * The use of the replacement policy fields under the Modified LRU policy
 * is discussed below:
 *
 * next:  Next pointer in either the LRU or the protected list,
 *    depending on the current value of protected.  If there
 *    is no next entry on the list, this field should be set
 *    to NULL.
 *
 * prev:  Prev pointer in either the LRU or the protected list,
 *    depending on the current value of protected.  If there
 *    is no previous entry on the list, this field should be
 *    set to NULL.
 *
 * aux_next:  Next pointer on either the clean or dirty LRU lists.
 *    This entry should be NULL when protected is true.  When
 *    protected is false, and dirty is true, it should point
 *    to the next item on the dirty LRU list.  When protected
 *    is false, and dirty is false, it should point to the
 *    next item on the clean LRU list.  In either case, when
 *    there is no next item, it should be NULL.
 *
 * aux_prev:  Previous pointer on either the clean or dirty LRU lists.
 *    This entry should be NULL when protected is true.  When
 *    protected is false, and dirty is true, it should point
 *    to the previous item on the dirty LRU list.  When protected
 *    is false, and dirty is false, it should point to the
 *    previous item on the clean LRU list.  In either case, when
 *    there is no previous item, it should be NULL.
 *
 *
 * Cache entry stats collection fields:
 *
 * These fields should only be compiled in when both H5C_COLLECT_CACHE_STATS
 * and H5C_COLLECT_CACHE_ENTRY_STATS are true.  When present, they allow
 * collection of statistics on individual cache entries.
 *
 * accesses:  int32_t containing the number of times this cache entry has
 *    been referenced in its lifetime.
 *
 * clears:  int32_t containing the number of times this cache entry has
 *              been cleared in its life time.
 *
 * flushes:  int32_t containing the number of times this cache entry has
 *              been flushed to file in its life time.
 *
 ****************************************************************************/

typedef struct H5C_cache_entry_t
{
    haddr_t    addr;
    size_t    size;
    const H5C_class_t *  type;
    hbool_t    is_dirty;
    hbool_t    is_protected;
    hbool_t    in_slist;

    /* fields supporting the hash table: */

    struct H5C_cache_entry_t *  ht_next;
    struct H5C_cache_entry_t *  ht_prev;

    /* fields supporting replacement policies: */

    struct H5C_cache_entry_t *  next;
    struct H5C_cache_entry_t *  prev;
    struct H5C_cache_entry_t *  aux_next;
    struct H5C_cache_entry_t *  aux_prev;

#if H5C_COLLECT_CACHE_ENTRY_STATS

    /* cache entry stats fields */

    int32_t      accesses;
    int32_t      clears;
    int32_t      flushes;

#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */

} H5C_cache_entry_t;


/* Typedef for the main structure for the cache (defined in H5C.c) */

typedef struct H5C_t H5C_t;

/*
 * Library prototypes.
 */
H5_DLL H5C_t * H5C_create(size_t                     max_cache_size,
                          size_t                     min_clean_size,
                          int                        max_type_id,
                          const char *              (* type_name_table_ptr),
                          H5C_write_permitted_func_t check_write_permitted);

H5_DLL herr_t H5C_dest(H5F_t * f,
                       hid_t   primary_dxpl_id,
                       hid_t   secondary_dxpl_id,
                       H5C_t * cache_ptr);

H5_DLL herr_t H5C_dest_empty(H5C_t * cache_ptr);

H5_DLL herr_t H5C_flush_cache(H5F_t *  f,
                              hid_t    primary_dxpl_id,
                              hid_t    secondary_dxpl_id,
                              H5C_t *  cache_ptr,
                              unsigned flags);

H5_DLL herr_t H5C_insert_entry(H5F_t *             f,
                               hid_t               primary_dxpl_id,
                               hid_t               secondary_dxpl_id,
                               H5C_t *             cache_ptr,
                               const H5C_class_t * type,
                               haddr_t             addr,
                               void *              thing);

H5_DLL herr_t H5C_rename_entry(H5C_t *             cache_ptr,
                               const H5C_class_t * type,
                               haddr_t             old_addr,
                               haddr_t             new_addr);

H5_DLL void * H5C_protect(H5F_t *             f,
                          hid_t               primary_dxpl_id,
                          hid_t               secondary_dxpl_id,
                          H5C_t *             cache_ptr,
                          const H5C_class_t * type,
                          haddr_t             addr,
                          const void *        udata1,
                          void *              udata2);

H5_DLL herr_t H5C_unprotect(H5F_t *             f,
                            hid_t               primary_dxpl_id,
                            hid_t               secondary_dxpl_id,
                            H5C_t *             cache_ptr,
                            const H5C_class_t * type,
                            haddr_t             addr,
                            void *              thing,
                            hbool_t             deleted);

H5_DLL herr_t H5C_stats(H5C_t * cache_ptr,
                        const char * cache_name,
                        hbool_t display_detailed_stats);

H5_DLL void H5C_stats__reset(H5C_t * cache_ptr);

H5_DLL herr_t H5C_set_skip_flags(H5C_t * cache_ptr,
                                 hbool_t skip_file_checks,
                                 hbool_t skip_dxpl_id_checks);

#endif /* !_H5Cprivate_H */

