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

/*
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Friday, March 27, 1998
 *
 * Purpose:  Operations on the global heap.  The global heap is the set of
 *    all collections and each collection contains one or more
 *    global heap objects.  An object belongs to exactly one
 *    collection.  A collection is treated as an atomic entity for
 *    the purposes of I/O and caching.
 *
 *    Each file has a small cache of global heap collections called
 *    the CWFS list and recently accessed collections with free
 *    space appear on this list.  As collections are accessed the
 *    collection is moved toward the front of the list.  New
 *    collections are added to the front of the list while old
 *    collections are added to the end of the list.
 *
 *    The collection model reduces the overhead which would be
 *    incurred if the global heap were a single object, and the
 *    CWFS list allows the library to cheaply choose a collection
 *    for a new object based on object size, amount of free space
 *    in the collection, and temporal locality.
 */

#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */
#define H5HG_PACKAGE    /*suppress error about including H5HGpkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5ACprivate.h"  /* Metadata cache      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fpkg.h"             /* File access        */
#include "H5FLprivate.h"  /* Free lists                           */
#include "H5HGpkg.h"    /* Global heaps        */
#include "H5MFprivate.h"  /* File memory management    */
#include "H5MMprivate.h"  /* Memory management      */

/* Private macros */

/*
 * Global heap collection version.
 */
#define H5HG_VERSION  1

/*
 * All global heap collections are at least this big.  This allows us to read
 * most collections with a single read() since we don't have to read a few
 * bytes of header to figure out the size.  If the heap is larger than this
 * then a second read gets the rest after we've decoded the header.
 */
#define H5HG_MINSIZE  4096

/*
 * Limit global heap collections to the some reasonable size.  This is
 * fairly arbitrary, but needs to be small enough that no more than H5HG_MAXIDX
 * objects will be allocated from a single heap.
 */
#define H5HG_MAXSIZE  65536

/*
 * Maximum length of the CWFS list, the list of remembered collections that
 * have free space.
 */
#define H5HG_NCWFS  16

/*
 * The maximum number of links allowed to a global heap object.
 */
#define H5HG_MAXLINK  65535

/*
 * The maximum number of indices allowed in a global heap object.
 */
#define H5HG_MAXIDX  65535

/*
 * The size of the collection header, always a multiple of the alignment so
 * that the stuff that follows the header is aligned.
 */
#define H5HG_SIZEOF_HDR(f)                  \
    H5HG_ALIGN(4 +      /*magic number    */        \
         1 +      /*version number  */        \
         3 +      /*reserved    */        \
         H5F_SIZEOF_SIZE(f))  /*collection size  */

/*
 * The initial guess for the number of messages in a collection.  We assume
 * that all objects in that collection are zero length, giving the maximum
 * possible number of objects in the collection.  The collection itself has
 * some overhead and each message has some overhead.  The `+2' accounts for
 * rounding and for the free space object.
 */
#define H5HG_NOBJS(f,z) (int)((((z)-H5HG_SIZEOF_HDR(f))/          \
             H5HG_SIZEOF_OBJHDR(f)+2))

/*
 * Makes a global heap object pointer undefined, or checks whether one is
 * defined.
 */
#define H5HG_undef(HGP)  ((HGP)->idx=0)
#define H5HG_defined(HGP) ((HGP)->idx!=0)

/* Private typedefs */

/* PRIVATE PROTOTYPES */
static haddr_t H5HG_create(H5F_t *f, hid_t dxpl_id, size_t size);
#ifdef NOT_YET
static void *H5HG_peek(H5F_t *f, hid_t dxpl_id, H5HG_t *hobj);
#endif /* NOT_YET */

/* Metadata cache callbacks */
static H5HG_heap_t *H5HG_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, const void *udata1,
            void *udata2);
static herr_t H5HG_flush(H5F_t *f, hid_t dxpl_id, hbool_t dest, haddr_t addr,
       H5HG_heap_t *heap);
static herr_t H5HG_dest(H5F_t *f, H5HG_heap_t *heap);
static herr_t H5HG_clear(H5F_t *f, H5HG_heap_t *heap, hbool_t destroy);
static herr_t H5HG_compute_size(const H5F_t *f, const H5HG_heap_t *heap, size_t *size_ptr);

/*
 * H5HG inherits cache-like properties from H5AC
 */
const H5AC_class_t H5AC_GHEAP[1] = {{
    H5AC_GHEAP_ID,
    (H5AC_load_func_t)H5HG_load,
    (H5AC_flush_func_t)H5HG_flush,
    (H5AC_dest_func_t)H5HG_dest,
    (H5AC_clear_func_t)H5HG_clear,
    (H5AC_size_func_t)H5HG_compute_size,
}};

/* Declare a free list to manage the H5HG_t struct */
H5FL_DEFINE_STATIC(H5HG_heap_t);

/* Declare a free list to manage sequences of H5HG_obj_t's */
H5FL_SEQ_DEFINE_STATIC(H5HG_obj_t);

/* Declare a PQ free list to manage heap chunks */
H5FL_BLK_DEFINE_STATIC(heap_chunk);


/*-------------------------------------------------------------------------
 * Function:  H5HG_create
 *
 * Purpose:  Creates a global heap collection of the specified size.  If
 *    SIZE is less than some minimum it will be readjusted.  The
 *    new collection is allocated in the file and added to the
 *    beginning of the CWFS list.
 *
 * Return:  Success:  Ptr to a cached heap.  The pointer is valid
 *        only until some other hdf5 library function
 *        is called.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, March 27, 1998
 *
 * Modifications:
 *
 *    John Mainzer 5/26/04
 *    Modified function to return the disk address of the new
 *    global heap collection, or HADDR_UNDEF on failure.  This
 *    is necessary, as in some cases (i.e. flexible parallel)
 *    H5AC_set() will imediately flush and destroy the in memory
 *    version of the new collection.  For the same reason, I
 *    moved the code which places the new collection on the cwfs
 *    list to just before the call to H5AC_set().
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5HG_create (H5F_t *f, hid_t dxpl_id, size_t size)
{
    H5HG_heap_t  *heap = NULL;
    haddr_t  ret_value = HADDR_UNDEF;
    uint8_t  *p = NULL;
    haddr_t  addr;
    size_t  n;

    FUNC_ENTER_NOAPI(H5HG_create, HADDR_UNDEF);

    /* Check args */
    assert (f);
    if (size<H5HG_MINSIZE)
        size = H5HG_MINSIZE;
    size = H5HG_ALIGN(size);

    /* Create it */
    H5_CHECK_OVERFLOW(size,size_t,hsize_t);
    if ( HADDR_UNDEF==
         (addr=H5MF_alloc(f, H5FD_MEM_GHEAP, dxpl_id, (hsize_t)size)))
  HGOTO_ERROR (H5E_HEAP, H5E_CANTINIT, HADDR_UNDEF, \
                     "unable to allocate file space for global heap");
    if (NULL==(heap = H5FL_MALLOC (H5HG_heap_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, \
                     "memory allocation failed");
    heap->addr = addr;
    heap->size = size;
    heap->cache_info.is_dirty = TRUE;
    if (NULL==(heap->chunk = H5FL_BLK_MALLOC (heap_chunk,size)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, \
                     "memory allocation failed");
#ifdef H5_USING_PURIFY
HDmemset(heap->chunk,0,size);
#endif /* H5_USING_PURIFY */
    heap->nalloc = H5HG_NOBJS (f, size);
    heap->nused = 1; /* account for index 0, which is used for the free object */
    if (NULL==(heap->obj = H5FL_SEQ_MALLOC (H5HG_obj_t,heap->nalloc)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, \
                     "memory allocation failed");

    /* Initialize the header */
    HDmemcpy (heap->chunk, H5HG_MAGIC, H5HG_SIZEOF_MAGIC);
    p = heap->chunk + H5HG_SIZEOF_MAGIC;
    *p++ = H5HG_VERSION;
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    H5F_ENCODE_LENGTH (f, p, size);

    /*
     * Padding so free space object is aligned. If malloc returned memory
     * which was always at least H5HG_ALIGNMENT aligned then we could just
     * align the pointer, but this might not be the case.
     */
    n = H5HG_ALIGN(p-heap->chunk) - (p-heap->chunk);
#ifdef OLD_WAY
/* Don't bother zeroing out the rest of the info in the heap -QAK */
    HDmemset(p, 0, n);
#endif /* OLD_WAY */
    p += n;

    /* The freespace object */
    heap->obj[0].size = size - H5HG_SIZEOF_HDR(f);
    assert(H5HG_ISALIGNED(heap->obj[0].size));
    heap->obj[0].nrefs = 0;
    heap->obj[0].begin = p;
    UINT16ENCODE(p, 0);  /*object ID*/
    UINT16ENCODE(p, 0);  /*reference count*/
    UINT32ENCODE(p, 0); /*reserved*/
    H5F_ENCODE_LENGTH (f, p, heap->obj[0].size);
#ifdef OLD_WAY
/* Don't bother zeroing out the rest of the info in the heap -QAK */
    HDmemset (p, 0, (size_t)((heap->chunk+heap->size) - p));
#endif /* OLD_WAY */

    /* Add this heap to the beginning of the CWFS list */
    if (NULL==f->shared->cwfs) {
  f->shared->cwfs = H5MM_malloc (H5HG_NCWFS * sizeof(H5HG_heap_t*));
  if (NULL==(f->shared->cwfs))
      HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, \
                         "memory allocation failed");
  f->shared->cwfs[0] = heap;
  f->shared->ncwfs = 1;
    } else {
  HDmemmove (f->shared->cwfs+1, f->shared->cwfs,
                   MIN (f->shared->ncwfs, H5HG_NCWFS-1)*sizeof(H5HG_heap_t*));
  f->shared->cwfs[0] = heap;
  f->shared->ncwfs = MIN (H5HG_NCWFS, f->shared->ncwfs+1);
    }

    /* Add the heap to the cache */
    if (H5AC_set (f, dxpl_id, H5AC_GHEAP, addr, heap)<0)
  HGOTO_ERROR (H5E_HEAP, H5E_CANTINIT, HADDR_UNDEF, \
                     "unable to cache global heap collection");

    ret_value = addr;

done:
    if ( ! ( H5F_addr_defined(addr) ) && heap) {
        if ( H5HG_dest(f,heap) < 0 )
      HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, HADDR_UNDEF, \
                        "unable to destroy global heap collection");
    }

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5HG_create() */


/*-------------------------------------------------------------------------
 * Function:  H5HG_load
 *
 * Purpose:  Loads a global heap collection from disk.
 *
 * Return:  Success:  Ptr to a global heap collection.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, March 27, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *
 *  Quincey Koziol, 2002-7-180
 *  Added dxpl parameter to allow more control over I/O from metadata
 *      cache.
 *-------------------------------------------------------------------------
 */
static H5HG_heap_t *
H5HG_load (H5F_t *f, hid_t dxpl_id, haddr_t addr, const void UNUSED * udata1,
     void UNUSED * udata2)
{
    H5HG_heap_t  *heap = NULL;
    uint8_t  *p = NULL;
    int  i;
    size_t  nalloc, need;
    size_t      max_idx=0;              /* The maximum index seen */
    H5HG_heap_t  *ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI(H5HG_load, NULL);

    /* check arguments */
    assert (f);
    assert (H5F_addr_defined (addr));
    assert (!udata1);
    assert (!udata2);

    /* Read the initial 4k page */
    if (NULL==(heap = H5FL_CALLOC (H5HG_heap_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    heap->addr = addr;
    if (NULL==(heap->chunk = H5FL_BLK_MALLOC (heap_chunk,H5HG_MINSIZE)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    if (H5F_block_read(f, H5FD_MEM_GHEAP, addr, H5HG_MINSIZE, dxpl_id, heap->chunk)<0)
  HGOTO_ERROR (H5E_HEAP, H5E_READERROR, NULL, "unable to read global heap collection");

    /* Magic number */
    if (HDmemcmp (heap->chunk, H5HG_MAGIC, H5HG_SIZEOF_MAGIC))
  HGOTO_ERROR (H5E_HEAP, H5E_CANTLOAD, NULL, "bad global heap collection signature");
    p = heap->chunk + H5HG_SIZEOF_MAGIC;

    /* Version */
    if (H5HG_VERSION!=*p++)
  HGOTO_ERROR (H5E_HEAP, H5E_CANTLOAD, NULL, "wrong version number in global heap");

    /* Reserved */
    p += 3;

    /* Size */
    H5F_DECODE_LENGTH (f, p, heap->size);
    assert (heap->size>=H5HG_MINSIZE);

    /*
     * If we didn't read enough in the first try, then read the rest of the
     * collection now.
     */
    if (heap->size > H5HG_MINSIZE) {
  haddr_t next_addr = addr + (hsize_t)H5HG_MINSIZE;
  if (NULL==(heap->chunk = H5FL_BLK_REALLOC (heap_chunk, heap->chunk, heap->size)))
      HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
  if (H5F_block_read (f, H5FD_MEM_GHEAP, next_addr, (heap->size-H5HG_MINSIZE), dxpl_id, heap->chunk+H5HG_MINSIZE)<0)
      HGOTO_ERROR (H5E_HEAP, H5E_READERROR, NULL, "unable to read global heap collection");
    }

    /* Decode each object */
    p = heap->chunk + H5HG_SIZEOF_HDR (f);
    nalloc = H5HG_NOBJS (f, heap->size);
    if (NULL==(heap->obj = H5FL_SEQ_MALLOC (H5HG_obj_t,nalloc)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    heap->obj[0].size=heap->obj[0].nrefs=0;
    heap->obj[0].begin=NULL;

    heap->nalloc = nalloc;
    while (p<heap->chunk+heap->size) {
  if (p+H5HG_SIZEOF_OBJHDR(f)>heap->chunk+heap->size) {
      /*
       * The last bit of space is too tiny for an object header, so we
       * assume that it's free space.
       */
      assert (NULL==heap->obj[0].begin);
      heap->obj[0].size = (heap->chunk+heap->size) - p;
      heap->obj[0].begin = p;
      p += heap->obj[0].size;
  } else {
      unsigned idx;
      uint8_t *begin = p;

      UINT16DECODE (p, idx);

            /* Check if we need more room to store heap objects */
            if(idx>=heap->nalloc) {
                size_t new_alloc;       /* New allocation number */
                H5HG_obj_t *new_obj;  /* New array of object descriptions */

                /* Determine the new number of objects to index */
                new_alloc=MAX(heap->nalloc*2,(idx+1));

                /* Reallocate array of objects */
                if (NULL==(new_obj = H5FL_SEQ_REALLOC (H5HG_obj_t, heap->obj, new_alloc)))
                    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

                /* Update heap information */
                heap->nalloc=new_alloc;
                heap->obj=new_obj;
            } /* end if */

      UINT16DECODE (p, heap->obj[idx].nrefs);
      p += 4; /*reserved*/
      H5F_DECODE_LENGTH (f, p, heap->obj[idx].size);
      heap->obj[idx].begin = begin;
      /*
       * The total storage size includes the size of the object header
       * and is zero padded so the next object header is properly
       * aligned. The last bit of space is the free space object whose
       * size is never padded and already includes the object header.
       */
      if (idx>0) {
    need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(heap->obj[idx].size);

                /* Check for "gap" in index numbers (caused by deletions) and fill in heap object values */
                if(idx>(max_idx+1))
                    HDmemset(&heap->obj[max_idx+1],0,sizeof(H5HG_obj_t)*(idx-(max_idx+1)));
                max_idx=idx;
      } else {
    need = heap->obj[idx].size;
      }
      p = begin + need;
  }
    }
    assert(p==heap->chunk+heap->size);
    assert(H5HG_ISALIGNED(heap->obj[0].size));

    /* Set the next index value to use */
    if(max_idx>0)
        heap->nused=max_idx+1;
    else
        heap->nused=1;

    /*
     * Add the new heap to the CWFS list, removing some other entry if
     * necessary to make room. We remove the right-most entry that has less
     * free space than this heap.
     */
    if (heap->obj[0].size>0) {
  if (!f->shared->cwfs) {
      f->shared->cwfs = H5MM_malloc (H5HG_NCWFS*sizeof(H5HG_heap_t*));
      if (NULL==f->shared->cwfs)
    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
      f->shared->ncwfs = 1;
      f->shared->cwfs[0] = heap;
  } else if (H5HG_NCWFS==f->shared->ncwfs) {
      for (i=H5HG_NCWFS-1; i>=0; --i) {
    if (f->shared->cwfs[i]->obj[0].size < heap->obj[0].size) {
        HDmemmove (f->shared->cwfs+1, f->shared->cwfs, i * sizeof(H5HG_heap_t*));
        f->shared->cwfs[0] = heap;
        break;
    }
      }
  } else {
      HDmemmove (f->shared->cwfs+1, f->shared->cwfs, f->shared->ncwfs*sizeof(H5HG_heap_t*));
      f->shared->ncwfs += 1;
      f->shared->cwfs[0] = heap;
  }
    }

    ret_value = heap;

done:
    if (!ret_value && heap) {
        if(H5HG_dest(f,heap)<0)
      HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, NULL, "unable to destroy global heap collection");
    }
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5HG_flush
 *
 * Purpose:  Flushes a global heap collection from memory to disk if it's
 *    dirty.  Optionally deletes teh heap from memory.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, March 27, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *
 *  Quincey Koziol, 2002-7-180
 *  Added dxpl parameter to allow more control over I/O from metadata
 *      cache.
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG_flush (H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5HG_heap_t *heap)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HG_flush, FAIL);

    /* Check arguments */
    assert (f);
    assert (H5F_addr_defined (addr));
    assert (H5F_addr_eq (addr, heap->addr));
    assert (heap);

    if (heap->cache_info.is_dirty) {
  if (H5F_block_write (f, H5FD_MEM_GHEAP, addr, heap->size, dxpl_id, heap->chunk)<0)
      HGOTO_ERROR (H5E_HEAP, H5E_WRITEERROR, FAIL, "unable to write global heap collection to file");
  heap->cache_info.is_dirty = FALSE;
    }

    if (destroy) {
        if(H5HG_dest(f,heap)<0)
      HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy global heap collection");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5HG_dest
 *
 * Purpose:  Destroys a global heap collection in memory
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, January 15, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG_dest (H5F_t *f, H5HG_heap_t *heap)
{
    int    i;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HG_dest);

    /* Check arguments */
    assert (heap);

    /* Verify that node is clean */
    assert (heap->cache_info.is_dirty==FALSE);

    for (i=0; i<f->shared->ncwfs; i++) {
        if (f->shared->cwfs[i]==heap) {
            f->shared->ncwfs -= 1;
            HDmemmove (f->shared->cwfs+i, f->shared->cwfs+i+1, (f->shared->ncwfs-i) * sizeof(H5HG_heap_t*));
            break;
        }
    }
    heap->chunk = H5FL_BLK_FREE(heap_chunk,heap->chunk);
    heap->obj = H5FL_SEQ_FREE(H5HG_obj_t,heap->obj);
    H5FL_FREE (H5HG_heap_t,heap);

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* H5HG_dest() */


/*-------------------------------------------------------------------------
 * Function:  H5HG_clear
 *
 * Purpose:  Mark a global heap in memory as non-dirty.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 20, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG_clear(H5F_t *f, H5HG_heap_t *heap, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5HG_clear);

    /* Check arguments */
    assert (heap);

    /* Mark heap as clean */
    heap->cache_info.is_dirty = FALSE;

    if (destroy)
        if (H5HG_dest(f, heap) < 0)
      HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy global heap collection");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5HG_clear() */


/*-------------------------------------------------------------------------
 * Function:  H5HG_compute_size
 *
 * Purpose:  Compute the size in bytes of the specified instance of
 *              H5HG_heap_t on disk, and return it in *len_ptr.  On failure,
 *              the value of *len_ptr is undefined.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              5/13/04
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG_compute_size(const H5F_t UNUSED *f, const H5HG_heap_t *heap, size_t *size_ptr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HG_compute_size);

    /* Check arguments */
    HDassert(heap);
    HDassert(size_ptr);

    *size_ptr = heap->size;

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* H5HG_compute_size() */


/*-------------------------------------------------------------------------
 * Function:  H5HG_alloc
 *
 * Purpose:  Given a heap with enough free space, this function will split
 *    the free space to make a new empty heap object and initialize
 *    the header.  SIZE is the exact size of the object data to be
 *    stored. It will be increased to make room for the object
 *    header and then rounded up for alignment.
 *
 * Return:  Success:  The heap object ID of the new object.
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *              Friday, March 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5HG_alloc (H5F_t *f, H5HG_heap_t *heap, size_t size)
{
    size_t  idx;
    uint8_t  *p = NULL;
    size_t  need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(size);
    size_t ret_value;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HG_alloc);

    /* Check args */
    assert (heap);
    assert (heap->obj[0].size>=need);

    /*
     * Find an ID for the new object. ID zero is reserved for the free space
     * object.
     */
    if(heap->nused<H5HG_MAXIDX)
        idx=heap->nused++;
    else {
        for (idx=1; idx<heap->nused; idx++)
            if (NULL==heap->obj[idx].begin)
                break;
    } /* end else */

    /* Check if we need more room to store heap objects */
    if(idx>=heap->nalloc) {
        size_t new_alloc;       /* New allocation number */
        H5HG_obj_t *new_obj;  /* New array of object descriptions */

        /* Determine the new number of objects to index */
        new_alloc=MAX(heap->nalloc*2,(idx+1));
        assert(new_alloc<=(H5HG_MAXIDX+1));

        /* Reallocate array of objects */
        if (NULL==(new_obj = H5FL_SEQ_REALLOC (H5HG_obj_t, heap->obj, new_alloc)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed");

        /* Update heap information */
        heap->nalloc=new_alloc;
        heap->obj=new_obj;
        assert(heap->nalloc>heap->nused);
    } /* end if */

    /* Initialize the new object */
    heap->obj[idx].nrefs = 0;
    heap->obj[idx].size = size;
    heap->obj[idx].begin = heap->obj[0].begin;
    p = heap->obj[idx].begin;
    UINT16ENCODE(p, idx);
    UINT16ENCODE(p, 0); /*nrefs*/
    UINT32ENCODE(p, 0); /*reserved*/
    H5F_ENCODE_LENGTH (f, p, size);

    /* Fix the free space object */
    if (need==heap->obj[0].size) {
  /*
   * All free space has been exhausted from this collection.
   */
  heap->obj[0].size = 0;
  heap->obj[0].begin = NULL;

    } else if (heap->obj[0].size-need >= H5HG_SIZEOF_OBJHDR (f)) {
  /*
   * Some free space remains and it's larger than a heap object header,
   * so write the new free heap object header to the heap.
   */
  heap->obj[0].size -= need;
  heap->obj[0].begin += need;
  p = heap->obj[0].begin;
  UINT16ENCODE(p, 0);  /*id*/
  UINT16ENCODE(p, 0);  /*nrefs*/
  UINT32ENCODE(p, 0);  /*reserved*/
  H5F_ENCODE_LENGTH (f, p, heap->obj[0].size);
  assert(H5HG_ISALIGNED(heap->obj[0].size));

    } else {
  /*
   * Some free space remains but it's smaller than a heap object header,
   * so we don't write the header.
   */
  heap->obj[0].size -= need;
  heap->obj[0].begin += need;
  assert(H5HG_ISALIGNED(heap->obj[0].size));
    }

    /* Mark the heap as dirty */
    heap->cache_info.is_dirty = TRUE;

    /* Set the return value */
    ret_value=idx;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5HG_extend
 *
 * Purpose:  Extend a heap to hold an object of SIZE bytes.
 *    SIZE is the exact size of the object data to be
 *    stored. It will be increased to make room for the object
 *    header and then rounded up for alignment.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, June 12, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG_extend (H5F_t *f, H5HG_heap_t *heap, size_t size)
{
    size_t  need;                   /* Actual space needed to store object */
    size_t  old_size;               /* Previous size of the heap's chunk */
    uint8_t *new_chunk=NULL;        /* Pointer to new chunk information */
    uint8_t *p = NULL;              /* Pointer to raw heap info */
    unsigned u;                     /* Local index variable */
    herr_t  ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HG_extend);

    /* Check args */
    assert (f);
    assert (heap);

    /* Compute total space need to add to this heap */
    need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(size);

    /* Decrement the amount needed in the heap by the amount of free space available */
    assert(need>heap->obj[0].size);
    need -= heap->obj[0].size;

    /* Don't do anything less than double the size of the heap */
    need = MAX(heap->size,need);

    /* Extend the space allocated for this heap on disk */
    if(H5MF_extend(f,H5FD_MEM_GHEAP,heap->addr,(hsize_t)heap->size,(hsize_t)need)<0)
  HGOTO_ERROR (H5E_HEAP, H5E_NOSPACE, FAIL, "can't extend heap on disk");

    /* Re-allocate the heap information in memory */
    if (NULL==(new_chunk = H5FL_BLK_REALLOC (heap_chunk, heap->chunk, heap->size+need)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "new heap allocation failed");
#ifdef H5_USING_PURIFY
HDmemset(new_chunk+heap->size,0,need);
#endif /* H5_USING_PURIFY */

    /* Adjust the size of the heap */
    old_size=heap->size;
    heap->size+=need;

    /* Encode the new size of the heap */
    p = new_chunk + H5HG_SIZEOF_MAGIC + 1 /* version */ + 3 /* reserved */;
    H5F_ENCODE_LENGTH (f, p, heap->size);

    /* Move the pointers to the existing objects to their new locations */
    for (u=0; u<heap->nused; u++)
        if(heap->obj[u].begin)
            heap->obj[u].begin = new_chunk + (heap->obj[u].begin - heap->chunk);

    /* Update the heap chunk pointer now */
    heap->chunk=new_chunk;

    /* Update the free space information for the heap  */
    heap->obj[0].size+=need;
    if(heap->obj[0].begin==NULL)
        heap->obj[0].begin=heap->chunk+old_size;
    p = heap->obj[0].begin;
    UINT16ENCODE(p, 0);  /*id*/
    UINT16ENCODE(p, 0);  /*nrefs*/
    UINT32ENCODE(p, 0);  /*reserved*/
    H5F_ENCODE_LENGTH (f, p, heap->obj[0].size);
    assert(H5HG_ISALIGNED(heap->obj[0].size));

    /* Mark the heap as dirty */
    heap->cache_info.is_dirty = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HG_extend() */


/*-------------------------------------------------------------------------
 * Function:  H5HG_insert
 *
 * Purpose:  A new object is inserted into the global heap.  It will be
 *    placed in the first collection on the CWFS list which has
 *    enough free space and that collection will be advanced one
 *    position in the list.  If no collection on the CWFS list has
 *    enough space then  a new collection will be created.
 *
 *    It is legal to push a zero-byte object onto the heap to get
 *    the reference count features of heap objects.
 *
 * Return:  Success:  Non-negative, and a heap object handle returned
 *        through the HOBJ pointer.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, March 27, 1998
 *
 * Modifications:
 *
 *    John Mainzer -- 5/24/04
 *    The function used to modify the heap without protecting
 *    the relevant collection first.  I did a half assed job
 *    of fixing the problem, which should hold until we try to
 *    support multi-threading.  At that point it will have to
 *    be done right.
 *
 *    See in line comment of this date for more details.
 *
 *    John Mainzer - 5/26/04
 *    Modified H5HG_create() to return the disk address of the
 *    new collection, instead of the address of its
 *    representation in core.  This was necessary as in FP
 *    mode, the cache will immediately flush and destroy any
 *    entry inserted in it via H5AC_set().  I then modified
 *    this function to account for the change in H5HG_create().
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_insert (H5F_t *f, hid_t dxpl_id, size_t size, void *obj, H5HG_t *hobj/*out*/)
{
    size_t  need;    /*total space needed for object    */
    int  cwfsno;
    size_t  idx;
    haddr_t  addr = HADDR_UNDEF;
    H5HG_heap_t  *heap = NULL;
    hbool_t     found=0;        /* Flag to indicate a heap with enough space was found */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HG_insert, FAIL);

    /* Check args */
    assert (f);
    assert (0==size || obj);
    assert (hobj);

    if (0==(f->intent & H5F_ACC_RDWR))
  HGOTO_ERROR (H5E_HEAP, H5E_WRITEERROR, FAIL, "no write intent on file");

    /* Find a large enough collection on the CWFS list */
    need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(size);

    /* Note that we don't have metadata cache locks on the entries in
     * f->shared->cwfs.
     *
     * In the current situation, this doesn't matter, as we are single
     * threaded, and as best I can tell, entries are added to and deleted
     * from f->shared->cwfs as they are added to and deleted from the
     * metadata cache.
     *
     * To be proper, we should either lock each entry in f->shared->cwfs
     * as we examine it, or lock the whole array.  However, at present
     * I don't see the point as there will be significant overhead,
     * and protecting and unprotecting all the collections in the global
     * heap on a regular basis will skew the replacement policy.
     *
     * However, there is a bigger issue -- as best I can tell, we only look
     * for free space in global heap chunks that are in cache.  If we can't
     * find any, we allocate a new chunk.  This may be a problem in FP mode,
     * as the metadata cache is disabled.  Do we allocate a new heap
     * collection for every entry in this case?
     *
     * Note that all this comes from a cursory read of the source.  Don't
     * take any of it as gospel.
     *                                        JRM - 5/24/04
     */

    for (cwfsno=0; cwfsno<f->shared->ncwfs; cwfsno++) {
  if (f->shared->cwfs[cwfsno]->obj[0].size>=need) {
      addr = f->shared->cwfs[cwfsno]->addr;
            found=1;
      break;
  } /* end if */
    } /* end for */

    /*
     * If we didn't find any collection with enough free space the check if
     * we can extend any of the collections to make enough room.
     */
    if (!found) {
        size_t new_need;

        for (cwfsno=0; cwfsno<f->shared->ncwfs; cwfsno++) {
            new_need = need;
            new_need -= f->shared->cwfs[cwfsno]->obj[0].size;
            new_need = MAX(f->shared->cwfs[cwfsno]->size, new_need);

            if((f->shared->cwfs[cwfsno]->size+new_need)<=H5HG_MAXSIZE && H5MF_can_extend(f,H5FD_MEM_GHEAP,f->shared->cwfs[cwfsno]->addr,(hsize_t)f->shared->cwfs[cwfsno]->size,(hsize_t)new_need)) {
                if(H5HG_extend(f,f->shared->cwfs[cwfsno],size)<0)
                    HGOTO_ERROR (H5E_HEAP, H5E_CANTINIT, FAIL, "unable to extend global heap collection");
          addr = f->shared->cwfs[cwfsno]->addr;
                found=1;
                break;
            } /* end if */
        } /* end for */
    } /* end if */

    /*
     * If we didn't find any collection with enough free space then allocate a
     * new collection large enough for the message plus the collection header.
     */
    if (!found) {

        addr = H5HG_create(f, dxpl_id, need+H5HG_SIZEOF_HDR (f));

        if ( ! H5F_addr_defined(addr) )
      HGOTO_ERROR (H5E_HEAP, H5E_CANTINIT, FAIL, \
                         "unable to allocate a global heap collection");
  cwfsno = 0;
    } /* end if */
    else {

        /* Move the collection forward in the CWFS list, if it's not
         * already at the front
         */
        if (cwfsno>0) {
            H5HG_heap_t *tmp = f->shared->cwfs[cwfsno];
            f->shared->cwfs[cwfsno] = f->shared->cwfs[cwfsno-1];
            f->shared->cwfs[cwfsno-1] = tmp;
            --cwfsno;
        } /* end if */
    } /* end else */

    HDassert(H5F_addr_defined(addr));

    if ( NULL == (heap = H5AC_protect(f, dxpl_id, H5AC_GHEAP, addr, NULL, NULL, H5AC_WRITE)) )
        HGOTO_ERROR (H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to load heap");

    /* Split the free space to make room for the new object */
    idx = H5HG_alloc (f, heap, size);

    /* Copy data into the heap */
    if(size>0) {
        HDmemcpy(heap->obj[idx].begin+H5HG_SIZEOF_OBJHDR(f), obj, size);
#ifdef OLD_WAY
/* Don't bother zeroing out the rest of the info in the heap -QAK */
        HDmemset(heap->obj[idx].begin+H5HG_SIZEOF_OBJHDR(f)+size, 0,
                 need-(H5HG_SIZEOF_OBJHDR(f)+size));
#endif /* OLD_WAY */
    } /* end if */
    heap->cache_info.is_dirty = TRUE;

    /* Return value */
    hobj->addr = heap->addr;
    hobj->idx = idx;

done:
    if ( heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, heap->addr, heap, FALSE) < 0 )
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to unprotect heap.");

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5HG_insert() */


/*-------------------------------------------------------------------------
 * Function:  H5HG_read
 *
 * Purpose:  Reads the specified global heap object into the buffer OBJECT
 *    supplied by the caller.  If the caller doesn't supply a
 *    buffer then one will be allocated.  The buffer should be
 *    large enough to hold the result.
 *
 * Return:  Success:  The buffer containing the result.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Monday, March 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5HG_read (H5F_t *f, hid_t dxpl_id, H5HG_t *hobj, void *object/*out*/)
{
    H5HG_heap_t  *heap = NULL;
    int  i;
    size_t  size;
    uint8_t  *p = NULL;
    void  *ret_value;

    FUNC_ENTER_NOAPI(H5HG_read, NULL);

    /* Check args */
    assert (f);
    assert (hobj);

    /* Load the heap */
    if (NULL == (heap = H5AC_protect(f, dxpl_id, H5AC_GHEAP, hobj->addr, NULL, NULL, H5AC_READ)))
  HGOTO_ERROR (H5E_HEAP, H5E_CANTLOAD, NULL, "unable to load heap");

    assert (hobj->idx<heap->nused);
    assert (heap->obj[hobj->idx].begin);
    size = heap->obj[hobj->idx].size;
    p = heap->obj[hobj->idx].begin + H5HG_SIZEOF_OBJHDR (f);
    if (!object && NULL==(object = H5MM_malloc (size)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    HDmemcpy (object, p, size);

    /*
     * Advance the heap in the CWFS list. We might have done this already
     * with the H5AC_protect(), but it won't hurt to do it twice.
     */
    if (heap->obj[0].begin) {
  for (i=0; i<f->shared->ncwfs; i++) {
      if (f->shared->cwfs[i]==heap) {
    if (i) {
        f->shared->cwfs[i] = f->shared->cwfs[i-1];
        f->shared->cwfs[i-1] = heap;
    }
    break;
      }
  }
    }

    /* Set return value */
    ret_value=object;

done:
    if (heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, hobj->addr, heap, FALSE)<0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, NULL, "unable to release object header");

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5HG_link
 *
 * Purpose:  Adjusts the link count for a global heap object by adding
 *    ADJUST to the current value.  This function will fail if the
 *    new link count would overflow.  Nothing special happens when
 *    the link count reaches zero; in order for a heap object to be
 *    removed one must call H5HG_remove().
 *
 * Return:  Success:  Number of links present after the adjustment.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, March 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5HG_link (H5F_t *f, hid_t dxpl_id, const H5HG_t *hobj, int adjust)
{
    H5HG_heap_t *heap = NULL;
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5HG_link, FAIL);

    /* Check args */
    assert (f);
    assert (hobj);
    if (0==(f->intent & H5F_ACC_RDWR))
  HGOTO_ERROR (H5E_HEAP, H5E_WRITEERROR, FAIL, "no write intent on file");

    if(adjust!=0) {
        /* Load the heap */
        if (NULL == (heap = H5AC_protect(f, dxpl_id, H5AC_GHEAP, hobj->addr, NULL, NULL, H5AC_WRITE)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to load heap");

        assert (hobj->idx<heap->nused);
        assert (heap->obj[hobj->idx].begin);
        if (heap->obj[hobj->idx].nrefs+adjust<0)
            HGOTO_ERROR (H5E_HEAP, H5E_BADRANGE, FAIL, "new link count would be out of range");
        if (heap->obj[hobj->idx].nrefs+adjust>H5HG_MAXLINK)
            HGOTO_ERROR (H5E_HEAP, H5E_BADVALUE, FAIL, "new link count would be out of range");
        heap->obj[hobj->idx].nrefs += adjust;
        heap->cache_info.is_dirty = TRUE;
    } /* end if */

    /* Set return value */
    ret_value=heap->obj[hobj->idx].nrefs;

done:
    if (heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, hobj->addr, heap, FALSE)<0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to release object header");

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5HG_remove
 *
 * Purpose:  Removes the specified object from the global heap.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, March 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_remove (H5F_t *f, hid_t dxpl_id, H5HG_t *hobj)
{
    uint8_t  *p=NULL, *obj_start=NULL;
    H5HG_heap_t  *heap = NULL;
    size_t  need;
    int  i;
    unsigned  u;
    hbool_t     deleted=FALSE;          /* Whether the heap gets deleted */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HG_remove, FAIL);

    /* Check args */
    assert (f);
    assert (hobj);
    if (0==(f->intent & H5F_ACC_RDWR))
        HGOTO_ERROR (H5E_HEAP, H5E_WRITEERROR, FAIL, "no write intent on file");

    /* Load the heap */
    if (NULL == (heap = H5AC_protect(f, dxpl_id, H5AC_GHEAP, hobj->addr, NULL, NULL, H5AC_WRITE)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to load heap");

    assert (hobj->idx<heap->nused);
    assert (heap->obj[hobj->idx].begin);
    obj_start = heap->obj[hobj->idx].begin;
    /* Include object header size */
    need = H5HG_ALIGN(heap->obj[hobj->idx].size)+H5HG_SIZEOF_OBJHDR(f);

    /* Move the new free space to the end of the heap */
    for (u=0; u<heap->nused; u++) {
        if (heap->obj[u].begin > heap->obj[hobj->idx].begin)
            heap->obj[u].begin -= need;
    }
    if (NULL==heap->obj[0].begin) {
        heap->obj[0].begin = heap->chunk + (heap->size-need);
        heap->obj[0].size = need;
        heap->obj[0].nrefs = 0;
    } else {
        heap->obj[0].size += need;
    }
    HDmemmove (obj_start, obj_start+need,
         heap->size-((obj_start+need)-heap->chunk));
    if (heap->obj[0].size>=H5HG_SIZEOF_OBJHDR (f)) {
        p = heap->obj[0].begin;
        UINT16ENCODE(p, 0); /*id*/
        UINT16ENCODE(p, 0); /*nrefs*/
        UINT32ENCODE(p, 0); /*reserved*/
        H5F_ENCODE_LENGTH (f, p, heap->obj[0].size);
    }
    HDmemset (heap->obj+hobj->idx, 0, sizeof(H5HG_obj_t));
    heap->cache_info.is_dirty = TRUE;

    if (heap->obj[0].size+H5HG_SIZEOF_HDR(f)==heap->size) {
        /*
         * The collection is empty. Remove it from the CWFS list and return it
         * to the file free list.
         */
        heap->cache_info.is_dirty = FALSE;
        H5_CHECK_OVERFLOW(heap->size,size_t,hsize_t);
        H5MF_xfree(f, H5FD_MEM_GHEAP, dxpl_id, heap->addr, (hsize_t)heap->size);
        deleted=TRUE;   /* Indicate that the object was deleted, for the unprotect call */
    } else {
        /*
         * If the heap is in the CWFS list then advance it one position.  The
         * H5AC_protect() might have done that too, but that's okay.  If the
         * heap isn't on the CWFS list then add it to the end.
         */
        for (i=0; i<f->shared->ncwfs; i++) {
            if (f->shared->cwfs[i]==heap) {
                if (i) {
                    f->shared->cwfs[i] = f->shared->cwfs[i-1];
                    f->shared->cwfs[i-1] = heap;
                }
                break;
            }
        }
        if (i>=f->shared->ncwfs) {
            f->shared->ncwfs = MIN (f->shared->ncwfs+1, H5HG_NCWFS);
            f->shared->cwfs[f->shared->ncwfs-1] = heap;
        }
    }

done:
    if (heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, hobj->addr, heap, deleted) != SUCCEED)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to release object header");

    FUNC_LEAVE_NOAPI(ret_value);
}
