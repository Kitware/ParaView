/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5HL.c
 *                      Jul 16 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Heap functions for the local heaps used by symbol
 *                      tables to store names (among other things).
 *
 * Modifications:
 *
 *      Robb Matzke, 5 Aug 1997
 *      Added calls to H5E.
 *
 *-------------------------------------------------------------------------
 */
#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"                  /*library                       */
#include "H5ACprivate.h"                /*cache                         */
#include "H5Eprivate.h"                 /*error handling                */
#include "H5Fpkg.h"                     /*file access                   */
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5HLprivate.h"                /*self                          */
#include "H5MFprivate.h"                /*file memory management        */
#include "H5MMprivate.h"                /*core memory management        */
#include "H5Pprivate.h"                 /*property lists                */

#include "H5FDmpio.h"                   /*for H5FD_mpio_tas_allsame()   */

#define H5HL_FREE_NULL  1               /*end of free list on disk      */
#define PABLO_MASK      H5HL_mask

typedef struct H5HL_free_t {
    size_t              offset;         /*offset of free block          */
    size_t              size;           /*size of free block            */
    struct H5HL_free_t  *prev;          /*previous entry in free list   */
    struct H5HL_free_t  *next;          /*next entry in free list       */
} H5HL_free_t;

typedef struct H5HL_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    int             dirty;
    haddr_t                 addr;       /*address of data               */
    size_t                  disk_alloc; /*data bytes allocated on disk  */
    size_t                  mem_alloc;  /*data bytes allocated in mem   */
    uint8_t                *chunk;      /*the chunk, including header   */
    H5HL_free_t            *freelist;   /*the free list                 */
} H5HL_t;

/* PRIVATE PROTOTYPES */
static H5HL_t *H5HL_load(H5F_t *f, haddr_t addr, const void *udata1,
                         void *udata2);
static herr_t H5HL_flush(H5F_t *f, hbool_t dest, haddr_t addr, H5HL_t *heap);

/*
 * H5HL inherits cache-like properties from H5AC
 */
static const H5AC_class_t H5AC_LHEAP[1] = {{
    H5AC_LHEAP_ID,
    (void *(*)(H5F_t*, haddr_t, const void*, void*))H5HL_load,
    (herr_t (*)(H5F_t*, hbool_t, haddr_t, void*))H5HL_flush,
}};

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT NULL

/* Declare a free list to manage the H5HL_free_t struct */
H5FL_DEFINE_STATIC(H5HL_free_t);

/* Declare a free list to manage the H5HL_t struct */
H5FL_DEFINE_STATIC(H5HL_t);

/* Declare a PQ free list to manage the heap chunk information */
H5FL_BLK_DEFINE_STATIC(heap_chunk);


/*-------------------------------------------------------------------------
 * Function:    H5HL_create
 *
 * Purpose:     Creates a new heap data structure on disk and caches it
 *              in memory.  SIZE_HINT is a hint for the initial size of the
 *              data area of the heap.  If size hint is invalid then a
 *              reasonable (but probably not optimal) size will be chosen.
 *              If the heap ever has to grow, then REALLOC_HINT is the
 *              minimum amount by which the heap will grow.
 *
 * Return:      Success:        Non-negative. The file address of new heap is
 *                              returned through the ADDR argument.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 16 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 5 Aug 1997
 *      Takes a flag that determines the type of heap that is
 *      created.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_create(H5F_t *f, size_t size_hint, haddr_t *addr_p/*out*/)
{
    H5HL_t      *heap = NULL;
    hsize_t     total_size;             /*total heap size on disk       */
    herr_t      ret_value = FAIL;

    FUNC_ENTER(H5HL_create, FAIL);

    /* check arguments */
    assert(f);
    assert(addr_p);

    if (size_hint && size_hint < H5HL_SIZEOF_FREE(f)) {
        size_hint = H5HL_SIZEOF_FREE(f);
    }
    size_hint = H5HL_ALIGN(size_hint);

    /* allocate file version */
    total_size = H5HL_SIZEOF_HDR(f) + size_hint;
    if (HADDR_UNDEF==(*addr_p=H5MF_alloc(f, H5FD_MEM_LHEAP, total_size))) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                      "unable to allocate file memory");
    }

    /* allocate memory version */
    if (NULL==(heap = H5FL_ALLOC(H5HL_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }
    heap->addr = *addr_p + (hsize_t)H5HL_SIZEOF_HDR(f);
    heap->disk_alloc = size_hint;
    heap->mem_alloc = size_hint;
    if (NULL==(heap->chunk = H5FL_BLK_ALLOC(heap_chunk,(hsize_t)(H5HL_SIZEOF_HDR(f) + size_hint),1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }

    /* free list */
    if (size_hint) {
        if (NULL==(heap->freelist = H5FL_ALLOC(H5HL_free_t,0))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                         "memory allocation failed");
        }
        heap->freelist->offset = 0;
        heap->freelist->size = size_hint;
        heap->freelist->prev = heap->freelist->next = NULL;
    } else {
        heap->freelist = NULL;
    }

    /* add to cache */
    heap->dirty = 1;
    if (H5AC_set(f, H5AC_LHEAP, *addr_p, heap) < 0) {
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL,
                    "unable to cache heap");
    }
    ret_value = SUCCEED;

 done:
    if (ret_value<0) {
        if (H5F_addr_defined(*addr_p)) {
            H5MF_xfree(f, H5FD_MEM_LHEAP, *addr_p, total_size);
        }
        if (heap) {
            H5FL_BLK_FREE (heap_chunk,heap->chunk);
            H5FL_FREE (H5HL_free_t,heap->freelist);
            H5FL_FREE (H5HL_t,heap);
        }
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_load
 *
 * Purpose:     Loads a heap from disk.
 *
 * Return:      Success:        Ptr to a local heap memory data structure.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 17 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static H5HL_t *
H5HL_load(H5F_t *f, haddr_t addr, const void UNUSED *udata1,
          void UNUSED *udata2)
{
    uint8_t             hdr[52];
    const uint8_t       *p = NULL;
    H5HL_t              *heap = NULL;
    H5HL_free_t         *fl = NULL, *tail = NULL;
    size_t              free_block = H5HL_FREE_NULL;
    H5HL_t              *ret_value = NULL;

    FUNC_ENTER(H5HL_load, NULL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(H5HL_SIZEOF_HDR(f) <= sizeof hdr);
    assert(!udata1);
    assert(!udata2);

    if (H5F_block_read(f, H5FD_MEM_LHEAP, addr, (hsize_t)H5HL_SIZEOF_HDR(f), H5P_DEFAULT,
                       hdr) < 0) {
        HRETURN_ERROR(H5E_HEAP, H5E_READERROR, NULL,
                      "unable to read heap header");
    }
    p = hdr;
    if (NULL==(heap = H5FL_ALLOC(H5HL_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
    }
    
    /* magic number */
    if (HDmemcmp(hdr, H5HL_MAGIC, H5HL_SIZEOF_MAGIC)) {
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL,
                    "bad heap signature");
    }
    p += H5HL_SIZEOF_MAGIC;

    /* Reserved */
    p += 4;

    /* heap data size */
    H5F_DECODE_LENGTH(f, p, heap->disk_alloc);
    heap->mem_alloc = heap->disk_alloc;

    /* free list head */
    H5F_DECODE_LENGTH(f, p, free_block);
    if (free_block != H5HL_FREE_NULL && free_block >= heap->disk_alloc) {
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL,
                    "bad heap free list");
    }

    /* data */
    H5F_addr_decode(f, &p, &(heap->addr));
    heap->chunk = H5FL_BLK_ALLOC(heap_chunk,(hsize_t)(H5HL_SIZEOF_HDR(f) + heap->mem_alloc),1);
    if (NULL==heap->chunk) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
    }
    if (heap->disk_alloc &&
        H5F_block_read(f, H5FD_MEM_LHEAP, heap->addr, (hsize_t)(heap->disk_alloc),
                       H5P_DEFAULT, heap->chunk + H5HL_SIZEOF_HDR(f)) < 0) {
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL,
                    "unable to read heap data");
    }

    /* free list */
    while (H5HL_FREE_NULL != free_block) {
        if (free_block >= heap->disk_alloc) {
            HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL,
                        "bad heap free list");
        }
        if (NULL==(fl = H5FL_ALLOC(H5HL_free_t,0))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                         "memory allocation failed");
        }
        fl->offset = free_block;
        fl->prev = tail;
        fl->next = NULL;
        if (tail) tail->next = fl;
        tail = fl;
        if (!heap->freelist) heap->freelist = fl;

        p = heap->chunk + H5HL_SIZEOF_HDR(f) + free_block;
        H5F_DECODE_LENGTH(f, p, free_block);
        H5F_DECODE_LENGTH(f, p, fl->size);

        if (fl->offset + fl->size > heap->disk_alloc) {
            HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL,
                        "bad heap free list");
        }
    }

    ret_value = heap;

  done:
    if (!ret_value && heap) {
        if(heap->chunk)
            heap->chunk = H5FL_BLK_FREE(heap_chunk,heap->chunk);
        for (fl = heap->freelist; fl; fl = tail) {
            tail = fl->next;
            H5FL_FREE(H5HL_free_t,fl);
        }
        H5FL_FREE(H5HL_t,heap);
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_flush
 *
 * Purpose:     Flushes a heap from memory to disk if it's dirty.  Optionally
 *              deletes the heap from memory.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 17 1997
 *
 * Modifications:
 *              rky, 1998-08-28
 *              Only p0 writes metadata to disk.
 *
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL_flush(H5F_t *f, hbool_t destroy, haddr_t addr, H5HL_t *heap)
{
    uint8_t     *p = heap->chunk;
    H5HL_free_t *fl = heap->freelist;
    haddr_t     hdr_end_addr;

    FUNC_ENTER(H5HL_flush, FAIL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(heap);

    if (heap->dirty) {
        /*
         * If the heap grew larger than disk storage then move the
         * data segment of the heap to a larger contiguous block of
         * disk storage.
         */
        if (heap->mem_alloc > heap->disk_alloc) {
            haddr_t old_addr = heap->addr, new_addr;
            if (HADDR_UNDEF==(new_addr=H5MF_alloc(f, H5FD_MEM_LHEAP,
                                                  (hsize_t)heap->mem_alloc))) {
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                              "unable to allocate file space for heap");
            }
            heap->addr = new_addr;
            H5MF_xfree(f, H5FD_MEM_LHEAP, old_addr, (hsize_t)heap->disk_alloc);
            H5E_clear(); /*don't really care if the free failed */
            heap->disk_alloc = heap->mem_alloc;
        }

        /*
         * Write the header.
         */
        HDmemcpy(p, H5HL_MAGIC, H5HL_SIZEOF_MAGIC);
        p += H5HL_SIZEOF_MAGIC;
        *p++ = 0;       /*reserved*/
        *p++ = 0;       /*reserved*/
        *p++ = 0;       /*reserved*/
        *p++ = 0;       /*reserved*/
        H5F_ENCODE_LENGTH(f, p, heap->mem_alloc);
        H5F_ENCODE_LENGTH(f, p, fl ? fl->offset : H5HL_FREE_NULL);
        H5F_addr_encode(f, &p, heap->addr);

        /*
         * Write the free list.
         */
        while (fl) {
            assert (fl->offset == H5HL_ALIGN (fl->offset));
            p = heap->chunk + H5HL_SIZEOF_HDR(f) + fl->offset;
            if (fl->next) {
                H5F_ENCODE_LENGTH(f, p, fl->next->offset);
            } else {
                H5F_ENCODE_LENGTH(f, p, H5HL_FREE_NULL);
            }
            H5F_ENCODE_LENGTH(f, p, fl->size);
            fl = fl->next;
        }

        /*
         * Copy buffer to disk.
         */
        hdr_end_addr = addr + (hsize_t)H5HL_SIZEOF_HDR(f);
        if (H5F_addr_eq(heap->addr, hdr_end_addr)) {
            /* The header and data are contiguous */
#ifdef H5_HAVE_PARALLEL
            if (IS_H5FD_MPIO(f))
                H5FD_mpio_tas_allsame( f->shared->lf, TRUE ); /* only p0 writes */
#endif /* H5_HAVE_PARALLEL */
            if (H5F_block_write(f, H5FD_MEM_LHEAP, addr,
                                (hsize_t)(H5HL_SIZEOF_HDR(f)+heap->disk_alloc),
                                H5P_DEFAULT, heap->chunk) < 0) {
                HRETURN_ERROR(H5E_HEAP, H5E_WRITEERROR, FAIL,
                            "unable to write heap header and data to file");
            }
        } else {
#ifdef H5_HAVE_PARALLEL
            if (IS_H5FD_MPIO(f))
                H5FD_mpio_tas_allsame( f->shared->lf, TRUE ); /* only p0 writes */
#endif /* H5_HAVE_PARALLEL */
            if (H5F_block_write(f, H5FD_MEM_LHEAP, addr, (hsize_t)H5HL_SIZEOF_HDR(f),
                                H5P_DEFAULT, heap->chunk)<0) {
                HRETURN_ERROR(H5E_HEAP, H5E_WRITEERROR, FAIL,
                              "unable to write heap header to file");
            }
#ifdef H5_HAVE_PARALLEL
            if (IS_H5FD_MPIO(f))
                H5FD_mpio_tas_allsame( f->shared->lf, TRUE ); /* only p0 writes */
#endif /* H5_HAVE_PARALLEL */
            if (H5F_block_write(f, H5FD_MEM_LHEAP, heap->addr, (hsize_t)(heap->disk_alloc),
                                H5P_DEFAULT,
                                heap->chunk + H5HL_SIZEOF_HDR(f)) < 0) {
                HRETURN_ERROR(H5E_HEAP, H5E_WRITEERROR, FAIL,
                              "unable to write heap data to file");
            }
        }

        heap->dirty = 0;
    }

    /*
     * Should we destroy the memory version?
     */
    if (destroy) {
        heap->chunk = H5FL_BLK_FREE(heap_chunk,heap->chunk);
        while (heap->freelist) {
            fl = heap->freelist;
            heap->freelist = fl->next;
            H5FL_FREE(H5HL_free_t,fl);
        }
        H5FL_FREE(H5HL_t,heap);
    }
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_read
 *
 * Purpose:     Reads some object (or part of an object) from the heap
 *              whose address is ADDR in file F.  OFFSET is the byte offset
 *              from the beginning of the heap at which to begin reading
 *              and SIZE is the number of bytes to read.
 *
 *              If BUF is the null pointer then a buffer is allocated by
 *              this function.
 *
 *              Attempting to read past the end of an object may cause this
 *              function to fail.
 *
 * Return:      Success:        BUF (or the allocated buffer)
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 16 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
void *
H5HL_read(H5F_t *f, haddr_t addr, size_t offset, size_t size, void *buf)
{
    H5HL_t      *heap = NULL;

    FUNC_ENTER(H5HL_read, NULL);

    /* check arguments */
    assert(f);
    assert (H5F_addr_defined(addr));

    if (NULL == (heap = H5AC_find(f, H5AC_LHEAP, addr, NULL, NULL))) {
        HRETURN_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL,
                      "unable to load heap");
    }
    assert(offset < heap->mem_alloc);
    assert(offset + size <= heap->mem_alloc);

    if (!buf && NULL==(buf = H5MM_malloc(size))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    HDmemcpy(buf, heap->chunk + H5HL_SIZEOF_HDR(f) + offset, size);

    FUNC_LEAVE(buf);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_peek
 *
 * Purpose:     This function is a more efficient version of H5HL_read.
 *              Instead of copying a heap object into a caller-supplied
 *              buffer, this function returns a pointer directly into the
 *              cache where the heap is being held.  Thus, the return pointer
 *              is valid only until the next call to the cache.
 *
 *              The address of the heap is ADDR in file F.  OFFSET is the
 *              byte offset of the object from the beginning of the heap and
 *              may include an offset into the interior of the object.
 *
 * Return:      Success:        Ptr to the object.  The pointer points to
 *                              a chunk of memory large enough to hold the
 *                              object from the specified offset (usually
 *                              the beginning of the object) to the end
 *                              of the object.  Do not attempt to read past
 *                              the end of the object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 16 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
const void *
H5HL_peek(H5F_t *f, haddr_t addr, size_t offset)
{
    H5HL_t              *heap = NULL;
    const void          *retval = NULL;

    FUNC_ENTER(H5HL_peek, NULL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));

    if (NULL == (heap = H5AC_find(f, H5AC_LHEAP, addr, NULL, NULL))) {
        HRETURN_ERROR(H5E_HEAP, H5E_CANTLOAD, NULL, "unable to load heap");
    }
    assert(offset < heap->mem_alloc);

    retval = heap->chunk + H5HL_SIZEOF_HDR(f) + offset;
    FUNC_LEAVE(retval);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_remove_free
 *
 * Purpose:     Removes free list element FL from the specified heap and
 *              frees it.
 *
 * Return:      NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 17 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5HL_free_t *
H5HL_remove_free(H5HL_t *heap, H5HL_free_t *fl)
{
    if (fl->prev) fl->prev->next = fl->next;
    if (fl->next) fl->next->prev = fl->prev;

    if (!fl->prev) heap->freelist = fl->next;
    return H5FL_FREE(H5HL_free_t,fl);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_insert
 *
 * Purpose:     Inserts a new item into the heap.
 *
 * Return:      Success:        Offset of new item within heap.
 *
 *              Failure:        (size_t)(-1)
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 17 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
size_t
H5HL_insert(H5F_t *f, haddr_t addr, size_t buf_size, const void *buf)
{
    H5HL_t      *heap = NULL;
    H5HL_free_t *fl = NULL, *max_fl = NULL;
    size_t      offset = 0;
    size_t      need_size, old_size, need_more;
    hbool_t     found;

    FUNC_ENTER(H5HL_insert, (size_t)(-1));

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(buf_size > 0);
    assert(buf);
    if (0==(f->intent & H5F_ACC_RDWR)) {
        HRETURN_ERROR (H5E_HEAP, H5E_WRITEERROR, (size_t)(-1),
                       "no write intent on file");
    }

    if (NULL == (heap = H5AC_find(f, H5AC_LHEAP, addr, NULL, NULL))) {
        HRETURN_ERROR(H5E_HEAP, H5E_CANTLOAD, (size_t)(-1),
                      "unable to load heap");
    }
    heap->dirty += 1;

    /*
     * In order to keep the free list descriptors aligned on word boundaries,
     * whatever that might mean, we round the size up to the next multiple of
     * a word.
     */
    need_size = H5HL_ALIGN(buf_size);

    /*
     * Look for a free slot large enough for this object and which would
     * leave zero or at least H5G_SIZEOF_FREE bytes left over.
     */
    for (fl=heap->freelist, found=FALSE; fl; fl=fl->next) {
        if (fl->size > need_size &&
            fl->size - need_size >= H5HL_SIZEOF_FREE(f)) {
            /* a bigger free block was found */
            offset = fl->offset;
            fl->offset += need_size;
            fl->size -= need_size;
            assert (fl->offset==H5HL_ALIGN (fl->offset));
            assert (fl->size==H5HL_ALIGN (fl->size));
            found = TRUE;
            break;
        } else if (fl->size == need_size) {
            /* free block of exact size found */
            offset = fl->offset;
            fl = H5HL_remove_free(heap, fl);
            found = TRUE;
            break;
        } else if (!max_fl || max_fl->offset < fl->offset) {
            /* use worst fit */
            max_fl = fl;
        }
    }

    /*
     * If no free chunk was large enough, then allocate more space and
     * add it to the free list.  If the heap ends with a free chunk, we
     * can extend that free chunk.  Otherwise we'll have to make another
     * free chunk.  If the heap must expand, we double its size.
     */
    if (found==FALSE) {
        need_more = MAX3(need_size, heap->mem_alloc, H5HL_SIZEOF_FREE(f));

        if (max_fl && max_fl->offset + max_fl->size == heap->mem_alloc) {
            /*
             * Increase the size of the maximum free block.
             */
            offset = max_fl->offset;
            max_fl->offset += need_size;
            max_fl->size += need_more - need_size;
            assert (max_fl->offset==H5HL_ALIGN (max_fl->offset));
            assert (max_fl->size==H5HL_ALIGN (max_fl->size));

            if (max_fl->size < H5HL_SIZEOF_FREE(f)) {
#ifdef H5HL_DEBUG
                if (H5DEBUG(HL) && max_fl->size) {
                    fprintf(H5DEBUG(HL), "H5HL: lost %lu bytes at line %d\n",
                            (unsigned long)(max_fl->size), __LINE__);
                }
#endif
                max_fl = H5HL_remove_free(heap, max_fl);
            }
        } else {
            /*
             * Create a new free list element large enough that we can
             * take some space out of it right away.
             */
            offset = heap->mem_alloc;
            if (need_more - need_size >= H5HL_SIZEOF_FREE(f)) {
                if (NULL==(fl = H5FL_ALLOC(H5HL_free_t,0))) {
                    HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, (size_t)(-1),
                                   "memory allocation failed");
                }
                fl->offset = heap->mem_alloc + need_size;
                fl->size = need_more - need_size;
                assert (fl->offset==H5HL_ALIGN (fl->offset));
                assert (fl->size==H5HL_ALIGN (fl->size));
                fl->prev = NULL;
                fl->next = heap->freelist;
                if (heap->freelist) heap->freelist->prev = fl;
                heap->freelist = fl;
#ifdef H5HL_DEBUG
            } else if (H5DEBUG(HL) && need_more > need_size) {
                fprintf(H5DEBUG(HL),
                        "H5HL_insert: lost %lu bytes at line %d\n",
                        (unsigned long)(need_more - need_size), __LINE__);
#endif
            }
        }

#ifdef H5HL_DEBUG
        if (H5DEBUG(HL)) {
            fprintf(H5DEBUG(HL),
                    "H5HL: resize mem buf from %lu to %lu bytes\n",
                    (unsigned long)(heap->mem_alloc),
                    (unsigned long)(heap->mem_alloc + need_more));
        }
#endif
        old_size = heap->mem_alloc;
        heap->mem_alloc += need_more;
        heap->chunk = H5FL_BLK_REALLOC(heap_chunk,heap->chunk,
                                   (hsize_t)(H5HL_SIZEOF_HDR(f) + heap->mem_alloc));
        if (NULL==heap->chunk) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, (size_t)(-1),
                           "memory allocation failed");
        }
        
        /* clear new section so junk doesn't appear in the file */
        HDmemset(heap->chunk + H5HL_SIZEOF_HDR(f) + old_size, 0, need_more);
    }
    /*
     * Copy the data into the heap
     */
    HDmemcpy(heap->chunk + H5HL_SIZEOF_HDR(f) + offset, buf, buf_size);
    FUNC_LEAVE(offset);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_write
 *
 * Purpose:     Writes (overwrites) the object (or part of object) stored
 *              in BUF to the heap at file address ADDR in file F.  The
 *              writing begins at byte offset OFFSET from the beginning of
 *              the heap and continues for SIZE bytes.
 *
 *              Do not partially write an object to create it;  the first
 *              write for an object must be for the entire object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 16 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_write(H5F_t *f, haddr_t addr, size_t offset, size_t size, const void *buf)
{
    H5HL_t *heap = NULL;

    FUNC_ENTER(H5HL_write, FAIL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(buf);
    assert (offset==H5HL_ALIGN (offset));
    if (0==(f->intent & H5F_ACC_RDWR)) {
        HRETURN_ERROR (H5E_HEAP, H5E_WRITEERROR, FAIL,
                       "no write intent on file");
    }

    if (NULL == (heap = H5AC_find(f, H5AC_LHEAP, addr, NULL, NULL))) {
        HRETURN_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL,
                      "unable to load heap");
    }
    assert(offset < heap->mem_alloc);
    assert(offset + size <= heap->mem_alloc);

    heap->dirty += 1;
    HDmemcpy(heap->chunk + H5HL_SIZEOF_HDR(f) + offset, buf, size);

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_remove
 *
 * Purpose:     Removes an object or part of an object from the heap at
 *              address ADDR of file F.  The object (or part) to remove
 *              begins at byte OFFSET from the beginning of the heap and
 *              continues for SIZE bytes.
 *
 *              Once part of an object is removed, one must not attempt
 *              to access that part.  Removing the beginning of an object
 *              results in the object OFFSET increasing by the amount
 *              truncated.  Removing the end of an object results in
 *              object truncation.  Removing the middle of an object results
 *              in two separate objects, one at the original offset and
 *              one at the first offset past the removed portion.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 16 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_remove(H5F_t *f, haddr_t addr, size_t offset, size_t size)
{
    H5HL_t              *heap = NULL;
    H5HL_free_t         *fl = NULL, *fl2 = NULL;

    FUNC_ENTER(H5HL_remove, FAIL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(size > 0);
    assert (offset==H5HL_ALIGN (offset));
    if (0==(f->intent & H5F_ACC_RDWR)) {
        HRETURN_ERROR (H5E_HEAP, H5E_WRITEERROR, FAIL,
                       "no write intent on file");
    }

    size = H5HL_ALIGN (size);
    if (NULL == (heap = H5AC_find(f, H5AC_LHEAP, addr, NULL, NULL))) {
        HRETURN_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL,
                      "unable to load heap");
    }
    assert(offset < heap->mem_alloc);
    assert(offset + size <= heap->mem_alloc);
    fl = heap->freelist;

    heap->dirty += 1;

    /*
     * Check if this chunk can be prepended or appended to an already
     * free chunk.  It might also fall between two chunks in such a way
     * that all three chunks can be combined into one.
     */
    while (fl) {
        if (offset + size == fl->offset) {
            fl->offset = offset;
            fl->size += size;
            assert (fl->offset==H5HL_ALIGN (fl->offset));
            assert (fl->size==H5HL_ALIGN (fl->size));
            fl2 = fl->next;
            while (fl2) {
                if (fl2->offset + fl2->size == fl->offset) {
                    fl->offset = fl2->offset;
                    fl->size += fl2->size;
                    assert (fl->offset==H5HL_ALIGN (fl->offset));
                    assert (fl->size==H5HL_ALIGN (fl->size));
                    fl2 = H5HL_remove_free(heap, fl2);
                    HRETURN(SUCCEED);
                }
                fl2 = fl2->next;
            }
            HRETURN(SUCCEED);

        } else if (fl->offset + fl->size == offset) {
            fl->size += size;
            fl2 = fl->next;
            assert (fl->size==H5HL_ALIGN (fl->size));
            while (fl2) {
                if (fl->offset + fl->size == fl2->offset) {
                    fl->size += fl2->size;
                    assert (fl->size==H5HL_ALIGN (fl->size));
                    fl2 = H5HL_remove_free(heap, fl2);
                    HRETURN(SUCCEED);
                }
                fl2 = fl2->next;
            }
            HRETURN(SUCCEED);
        }
        fl = fl->next;
    }

    /*
     * The amount which is being removed must be large enough to
     * hold the free list data.  If not, the freed chunk is forever
     * lost.
     */
    if (size < H5HL_SIZEOF_FREE(f)) {
#ifdef H5HL_DEBUG
        if (H5DEBUG(HL)) {
            fprintf(H5DEBUG(HL), "H5HL: lost %lu bytes\n",
                    (unsigned long) size);
        }
#endif
        HRETURN(SUCCEED);
    }
    /*
     * Add an entry to the free list.
     */
    if (NULL==(fl = H5FL_ALLOC(H5HL_free_t,0))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                       "memory allocation failed");
    }
    fl->offset = offset;
    fl->size = size;
    assert (fl->offset==H5HL_ALIGN (fl->offset));
    assert (fl->size==H5HL_ALIGN (fl->size));
    fl->prev = NULL;
    fl->next = heap->freelist;
    if (heap->freelist) heap->freelist->prev = fl;
    heap->freelist = fl;

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5HL_debug
 *
 * Purpose:     Prints debugging information about a heap.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  1 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_debug(H5F_t *f, haddr_t addr, FILE * stream, int indent, int fwidth)
{
    H5HL_t              *h = NULL;
    int                 i, j, overlap;
    uint8_t             c;
    H5HL_free_t         *freelist = NULL;
    uint8_t             *marker = NULL;
    size_t              amount_free = 0;

    FUNC_ENTER(H5HL_debug, FAIL);

    /* check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    if (NULL == (h = H5AC_find(f, H5AC_LHEAP, addr, NULL, NULL))) {
        HRETURN_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL,
                      "unable to load heap");
    }
    fprintf(stream, "%*sLocal Heap...\n", indent, "");
    fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
            "Dirty:",
            (int) (h->dirty));
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Header size (in bytes):",
            (unsigned long) H5HL_SIZEOF_HDR(f));
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              "Address of heap data:",
              h->addr);
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Data bytes allocated on disk:",
            (unsigned long) (h->disk_alloc));
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Data bytes allocated in core:",
            (unsigned long) (h->mem_alloc));

    /*
     * Traverse the free list and check that all free blocks fall within
     * the heap and that no two free blocks point to the same region of
     * the heap.
     */
    if (NULL==(marker = H5MM_calloc(h->mem_alloc))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                       "memory allocation failed");
    }
    for (freelist = h->freelist; freelist; freelist = freelist->next) {
        fprintf(stream, "%*s%-*s %8lu, %8lu\n", indent, "", fwidth,
                "Free Block (offset,size):",
                (unsigned long) (freelist->offset),
                (unsigned long) (freelist->size));
        if (freelist->offset + freelist->size > h->mem_alloc) {
            fprintf(stream, "***THAT FREE BLOCK IS OUT OF BOUNDS!\n");
        } else {
            for (i=overlap=0; i<(int)(freelist->size); i++) {
                if (marker[freelist->offset + i])
                    overlap++;
                marker[freelist->offset + i] = 1;
            }
            if (overlap) {
                fprintf(stream, "***THAT FREE BLOCK OVERLAPPED A PREVIOUS "
                        "ONE!\n");
            } else {
                amount_free += freelist->size;
            }
        }
    }

    if (h->mem_alloc) {
        fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
                "Percent of heap used:",
                (unsigned long) (100 * (h->mem_alloc - amount_free) /
                                 h->mem_alloc));
    }
    /*
     * Print the data in a VMS-style octal dump.
     */
    fprintf(stream, "%*sData follows (`__' indicates free region)...\n",
            indent, "");
    for (i=0; i<(int)(h->disk_alloc); i+=16) {
        fprintf(stream, "%*s %8d: ", indent, "", i);
        for (j = 0; j < 16; j++) {
            if (i+j<(int)(h->disk_alloc)) {
                if (marker[i + j]) {
                    fprintf(stream, "__ ");
                } else {
                    c = h->chunk[H5HL_SIZEOF_HDR(f) + i + j];
                    fprintf(stream, "%02x ", c);
                }
            } else {
                fprintf(stream, "   ");
            }
            if (7 == j)
                HDfputc(' ', stream);
        }

        for (j = 0; j < 16; j++) {
            if (i+j < (int)(h->disk_alloc)) {
                if (marker[i + j]) {
                    HDfputc(' ', stream);
                } else {
                    c = h->chunk[H5HL_SIZEOF_HDR(f) + i + j];
                    if (c > ' ' && c < '~')
                        HDfputc(c, stream);
                    else
                        HDfputc('.', stream);
                }
            }
        }

        HDfputc('\n', stream);
    }

    H5MM_xfree(marker);
    FUNC_LEAVE(SUCCEED);
}
