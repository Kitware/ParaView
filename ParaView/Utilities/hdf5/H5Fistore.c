/*
 * Copyright (C) 1997 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, October  8, 1997
 *
 * Purpose:     Indexed (chunked) I/O functions.  The logical
 *              multi-dimensional data space is regularly partitioned into
 *              same-sized "chunks", the first of which is aligned with the
 *              logical origin.  The chunks are given a multi-dimensional
 *              index which is used as a lookup key in a B-tree that maps
 *              chunk index to disk address.  Each chunk can be compressed
 *              independently and the chunks may move around in the file as
 *              their storage requirements change.
 *
 * Cache:       Disk I/O is performed in units of chunks and H5MF_alloc()
 *              contains code to optionally align chunks on disk block
 *              boundaries for performance.
 *
 *              The chunk cache is an extendible hash indexed by a function
 *              of storage B-tree address and chunk N-dimensional offset
 *              within the dataset.  Collisions are not resolved -- one of
 *              the two chunks competing for the hash slot must be preempted
 *              from the cache.  All entries in the hash also participate in
 *              a doubly-linked list and entries are penalized by moving them
 *              toward the front of the list.  When a new chunk is about to
 *              be added to the cache the heap is pruned by preempting
 *              entries near the front of the list to make room for the new
 *              entry which is added to the end of the list.
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5Iprivate.h"
#include "H5MFprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"
#include "H5Pprivate.h"
#include "H5Vprivate.h"

/* MPIO driver needed for special checks */
#include "H5FDmpio.h"

/*
 * Feature: If this constant is defined then every cache preemption and load
 *          causes a character to be printed on the standard error stream:
 *
 *     `.': Entry was preempted because it has been completely read or
 *          completely written but not partially read and not partially
 *          written. This is often a good reason for preemption because such
 *          a chunk will be unlikely to be referenced in the near future.
 *
 *     `:': Entry was preempted because it hasn't been used recently.
 *
 *     `#': Entry was preempted because another chunk collided with it. This
 *          is usually a relatively bad thing.  If there are too many of
 *          these then the number of entries in the cache can be increased.
 *
 *       c: Entry was preempted because the file is closing.
 *
 *       w: A chunk read operation was eliminated because the library is
 *          about to write new values to the entire chunk.  This is a good
 *          thing, especially on files where the chunk size is the same as
 *          the disk block size, chunks are aligned on disk block boundaries,
 *          and the operating system can also eliminate a read operation.
 */
/* #define H5F_ISTORE_DEBUG */

/* Interface initialization */
#define PABLO_MASK      H5Fistore_mask
static int              interface_initialize_g = 0;
#define INTERFACE_INIT NULL

/*
 * Given a B-tree node return the dimensionality of the chunks pointed to by
 * that node.
 */
#define H5F_ISTORE_NDIMS(X)     ((int)(((X)->sizeof_rkey-8)/8))

/* Raw data chunks are cached.  Each entry in the cache is: */
typedef struct H5F_rdcc_ent_t {
    hbool_t     locked;         /*entry is locked in cache              */
    hbool_t     dirty;          /*needs to be written to disk?          */
    H5O_layout_t *layout;       /*the layout message                    */
    double      split_ratios[3];/*B-tree node splitting ratios          */
    H5O_pline_t *pline;         /*filter pipeline message               */
    hssize_t    offset[H5O_LAYOUT_NDIMS]; /*chunk name                  */
    size_t      rd_count;       /*bytes remaining to be read            */
    size_t      wr_count;       /*bytes remaining to be written         */
    size_t      chunk_size;     /*size of a chunk                       */
    size_t      alloc_size;     /*amount allocated for the chunk        */
    uint8_t     *chunk;         /*the unfiltered chunk data             */
    int idx;            /*index in hash table                   */
    struct H5F_rdcc_ent_t *next;/*next item in doubly-linked list       */
    struct H5F_rdcc_ent_t *prev;/*previous item in doubly-linked list   */
} H5F_rdcc_ent_t;
typedef H5F_rdcc_ent_t *H5F_rdcc_ent_ptr_t; /* For free lists */

/* Private prototypes */
static size_t H5F_istore_sizeof_rkey(H5F_t *f, const void *_udata);
static herr_t H5F_istore_new_node(H5F_t *f, H5B_ins_t, void *_lt_key,
                                  void *_udata, void *_rt_key,
                                  haddr_t* /*out*/);
static int H5F_istore_cmp2(H5F_t *f, void *_lt_key, void *_udata,
                            void *_rt_key);
static int H5F_istore_cmp3(H5F_t *f, void *_lt_key, void *_udata,
                            void *_rt_key);
static herr_t H5F_istore_found(H5F_t *f, haddr_t addr, const void *_lt_key,
                               void *_udata, const void *_rt_key);
static H5B_ins_t H5F_istore_insert(H5F_t *f, haddr_t addr, void *_lt_key,
                                   hbool_t *lt_key_changed, void *_md_key,
                                   void *_udata, void *_rt_key,
                                   hbool_t *rt_key_changed,
                                   haddr_t *new_node/*out*/);
static herr_t H5F_istore_iterate(H5F_t *f, void *left_key, haddr_t addr,
                                 void *right_key, void *_udata);
static herr_t H5F_istore_decode_key(H5F_t *f, H5B_t *bt, uint8_t *raw,
                                    void *_key);
static herr_t H5F_istore_encode_key(H5F_t *f, H5B_t *bt, uint8_t *raw,
                                    void *_key);
static herr_t H5F_istore_debug_key(FILE *stream, int indent, int fwidth,
                                   const void *key, const void *udata);
static haddr_t H5F_istore_get_addr(H5F_t *f, const H5O_layout_t *layout,
                                  const hssize_t offset[]);

/*
 * B-tree key.  A key contains the minimum logical N-dimensional address and
 * the logical size of the chunk to which this key refers.  The
 * fastest-varying dimension is assumed to reference individual bytes of the
 * array, so a 100-element 1-d array of 4-byte integers would really be a 2-d
 * array with the slow varying dimension of size 100 and the fast varying
 * dimension of size 4 (the storage dimensionality has very little to do with
 * the real dimensionality).
 *
 * Only the first few values of the OFFSET and SIZE fields are actually
 * stored on disk, depending on the dimensionality.
 *
 * The chunk's file address is part of the B-tree and not part of the key.
 */
typedef struct H5F_istore_key_t {
    size_t      nbytes;                         /*size of stored data   */
    hssize_t    offset[H5O_LAYOUT_NDIMS];       /*logical offset to start*/
    unsigned    filter_mask;                    /*excluded filters      */
} H5F_istore_key_t;

typedef struct H5F_istore_ud1_t {
    H5F_istore_key_t    key;    /*key values            */
    haddr_t             addr;                   /*file address of chunk */
    H5O_layout_t        mesg;           /*layout message        */
    hsize_t             total_storage;  /*output from iterator  */
    FILE                *stream;                /*debug output stream   */
} H5F_istore_ud1_t;

/* inherits B-tree like properties from H5B */
H5B_class_t H5B_ISTORE[1] = {{
    H5B_ISTORE_ID,                              /*id                    */
    sizeof(H5F_istore_key_t),                   /*sizeof_nkey           */
    H5F_istore_sizeof_rkey,                     /*get_sizeof_rkey       */
    H5F_istore_new_node,                        /*new                   */
    H5F_istore_cmp2,                            /*cmp2                  */
    H5F_istore_cmp3,                            /*cmp3                  */
    H5F_istore_found,                           /*found                 */
    H5F_istore_insert,                          /*insert                */
    FALSE,                                      /*follow min branch?    */
    FALSE,                                      /*follow max branch?    */
    NULL,                                       /*remove                */
    H5F_istore_iterate,                         /*iterator              */
    H5F_istore_decode_key,                      /*decode                */
    H5F_istore_encode_key,                      /*encode                */
    H5F_istore_debug_key,                       /*debug                 */
}};

#define H5F_HASH_DIVISOR 8     /* Attempt to spread out the hashing */
                                /* This should be the same size as the alignment of */
                                /* of the smallest file format object written to the file.  */
#define H5F_HASH(F,ADDR) H5F_addr_hash((ADDR/H5F_HASH_DIVISOR),(F)->shared->rdcc.nslots)


/* Declare a free list to manage the chunk information */
H5FL_BLK_DEFINE_STATIC(istore_chunk);

/* Declare a free list to manage H5F_rdcc_ent_t objects */
H5FL_DEFINE_STATIC(H5F_rdcc_ent_t);

/* Declare a PQ free list to manage the H5F_rdcc_ent_ptr_t array information */
H5FL_ARR_DEFINE_STATIC(H5F_rdcc_ent_ptr_t,-1);


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_chunk_alloc
 *
 * Purpose:     Allocates memory for a chunk of a dataset.  This routine is used
 *      instead of malloc because the chunks can be kept on a free list so
 *      they don't thrash malloc/free as much.
 *
 * Return:      Success:        valid pointer to the chunk
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  21, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5F_istore_chunk_alloc(size_t chunk_size)
{
    void *ret_value;                    /* Pointer to the chunk to return to the user */

    FUNC_ENTER(H5F_istore_chunk_alloc, NULL);

    ret_value=H5FL_BLK_ALLOC(istore_chunk,(hsize_t)chunk_size,0);

    FUNC_LEAVE(ret_value);
} /* end H5F_istore_chunk_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_chunk_free
 *
 * Purpose:     Releases memory for a chunk of a dataset.  This routine is used
 *      instead of free because the chunks can be kept on a free list so
 *      they don't thrash malloc/free as much.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        never fails
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  21, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5F_istore_chunk_free(void *chunk)
{
    FUNC_ENTER(H5F_istore_chunk_free, NULL);

    H5FL_BLK_FREE(istore_chunk,chunk);

    FUNC_LEAVE(NULL);
} /* end H5F_istore_chunk_free() */


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_chunk_realloc
 *
 * Purpose:     Resizes a chunk in chunking memory allocation system.  This
 *      does things the straightforward, simple way, not actually using
 *      realloc.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        never fails
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  21, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5F_istore_chunk_realloc(void *chunk, size_t new_size)
{
    void *ret_value=NULL;               /* Return value */

    FUNC_ENTER(H5F_istore_chunk_realloc, NULL);

    ret_value=H5FL_BLK_REALLOC(istore_chunk,chunk,(hsize_t)new_size);

    FUNC_LEAVE(ret_value);
} /* end H5F_istore_chunk_realloc() */


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_sizeof_rkey
 *
 * Purpose:     Returns the size of a raw key for the specified UDATA.  The
 *              size of the key is dependent on the number of dimensions for
 *              the object to which this B-tree points.  The dimensionality
 *              of the UDATA is the only portion that's referenced here.
 *
 * Return:      Success:        Size of raw key in bytes.
 *
 *              Failure:        abort()
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5F_istore_sizeof_rkey(H5F_t UNUSED *f, const void *_udata)
{
    const H5F_istore_ud1_t *udata = (const H5F_istore_ud1_t *) _udata;
    size_t                  nbytes;

    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims <= H5O_LAYOUT_NDIMS);

    nbytes = 4 +                        /*storage size          */
             4 +                        /*filter mask           */
             udata->mesg.ndims*8;       /*dimension indices     */

    f = 0;

    return nbytes;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_decode_key
 *
 * Purpose:     Decodes a raw key into a native key for the B-tree
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_decode_key(H5F_t UNUSED *f, H5B_t *bt, uint8_t *raw, void *_key)
{
    H5F_istore_key_t    *key = (H5F_istore_key_t *) _key;
    int         i;
    int         ndims = H5F_ISTORE_NDIMS(bt);

    FUNC_ENTER(H5F_istore_decode_key, FAIL);

    /* check args */
    assert(f);
    assert(bt);
    assert(raw);
    assert(key);
    assert(ndims>0 && ndims<=H5O_LAYOUT_NDIMS);

    /* decode */
    UINT32DECODE(raw, key->nbytes);
    UINT32DECODE(raw, key->filter_mask);
    for (i=0; i<ndims; i++) {
        UINT64DECODE(raw, key->offset[i]);
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_encode_key
 *
 * Purpose:     Encode a key from native format to raw format.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_encode_key(H5F_t UNUSED *f, H5B_t *bt, uint8_t *raw, void *_key)
{
    H5F_istore_key_t    *key = (H5F_istore_key_t *) _key;
    int         ndims = H5F_ISTORE_NDIMS(bt);
    int         i;

    FUNC_ENTER(H5F_istore_encode_key, FAIL);

    /* check args */
    assert(f);
    assert(bt);
    assert(raw);
    assert(key);
    assert(ndims>0 && ndims<=H5O_LAYOUT_NDIMS);

    /* encode */
    UINT32ENCODE(raw, key->nbytes);
    UINT32ENCODE(raw, key->filter_mask);
    for (i=0; i<ndims; i++) {
        UINT64ENCODE(raw, key->offset[i]);
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_debug_key
 *
 * Purpose:     Prints a key.
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
H5F_istore_debug_key (FILE *stream, int indent, int fwidth,
                      const void *_key, const void *_udata)
{
    const H5F_istore_key_t      *key = (const H5F_istore_key_t *)_key;
    const H5F_istore_ud1_t      *udata = (const H5F_istore_ud1_t *)_udata;
    unsigned            u;
    
    FUNC_ENTER (H5F_istore_debug_key, FAIL);
    assert (key);

    HDfprintf(stream, "%*s%-*s %Zd bytes\n", indent, "", fwidth,
              "Chunk size:", key->nbytes);
    HDfprintf(stream, "%*s%-*s 0x%08x\n", indent, "", fwidth,
              "Filter mask:", key->filter_mask);
    HDfprintf(stream, "%*s%-*s {", indent, "", fwidth,
              "Logical offset:");
    for (u=0; u<udata->mesg.ndims; u++) {
        HDfprintf (stream, "%s%Hd", u?", ":"", key->offset[u]);
    }
    HDfputs ("}\n", stream);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_cmp2
 *
 * Purpose:     Compares two keys sort of like strcmp().  The UDATA pointer
 *              is only to supply extra information not carried in the keys
 *              (in this case, the dimensionality) and is not compared
 *              against the keys.
 *
 * Return:      Success:        -1 if LT_KEY is less than RT_KEY;
 *                              1 if LT_KEY is greater than RT_KEY;
 *                              0 if LT_KEY and RT_KEY are equal.
 *
 *              Failure:        FAIL (same as LT_KEY<RT_KEY)
 *
 * Programmer:  Robb Matzke
 *              Thursday, November  6, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_istore_cmp2(H5F_t UNUSED *f, void *_lt_key, void *_udata,
                void *_rt_key)
{
    H5F_istore_key_t    *lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t    *rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t    *udata = (H5F_istore_ud1_t *) _udata;
    int         cmp;

    FUNC_ENTER(H5F_istore_cmp2, FAIL);

    assert(lt_key);
    assert(rt_key);
    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims <= H5O_LAYOUT_NDIMS);

    /* Compare the offsets but ignore the other fields */
    cmp = H5V_vector_cmp_s(udata->mesg.ndims, lt_key->offset, rt_key->offset);

    FUNC_LEAVE(cmp);
    f = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_cmp3
 *
 * Purpose:     Compare the requested datum UDATA with the left and right
 *              keys of the B-tree.
 *
 * Return:      Success:        negative if the min_corner of UDATA is less
 *                              than the min_corner of LT_KEY.
 *
 *                              positive if the min_corner of UDATA is
 *                              greater than or equal the min_corner of
 *                              RT_KEY.
 *
 *                              zero otherwise.  The min_corner of UDATA is
 *                              not necessarily contained within the address
 *                              space represented by LT_KEY, but a key that
 *                              would describe the UDATA min_corner address
 *                              would fall lexicographically between LT_KEY
 *                              and RT_KEY.
 *                              
 *              Failure:        FAIL (same as UDATA < LT_KEY)
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_istore_cmp3(H5F_t UNUSED *f, void *_lt_key, void *_udata,
                void *_rt_key)
{
    H5F_istore_key_t    *lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t    *rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t    *udata = (H5F_istore_ud1_t *) _udata;
    int         cmp = 0;

    FUNC_ENTER(H5F_istore_cmp3, FAIL);

    assert(lt_key);
    assert(rt_key);
    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims <= H5O_LAYOUT_NDIMS);

    if (H5V_vector_lt_s(udata->mesg.ndims, udata->key.offset,
                        lt_key->offset)) {
        cmp = -1;
    } else if (H5V_vector_ge_s(udata->mesg.ndims, udata->key.offset,
                             rt_key->offset)) {
        cmp = 1;
    }
    FUNC_LEAVE(cmp);

    f = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_new_node
 *
 * Purpose:     Adds a new entry to an i-storage B-tree.  We can assume that
 *              the domain represented by UDATA doesn't intersect the domain
 *              already represented by the B-tree.
 *
 * Return:      Success:        Non-negative. The address of leaf is returned
 *                              through the ADDR argument.  It is also added
 *                              to the UDATA.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October 14, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_new_node(H5F_t *f, H5B_ins_t op,
                    void *_lt_key, void *_udata, void *_rt_key,
                    haddr_t *addr_p/*out*/)
{
    H5F_istore_key_t    *lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t    *rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t    *udata = (H5F_istore_ud1_t *) _udata;
    unsigned            u;

    FUNC_ENTER(H5F_istore_new_node, FAIL);
#ifdef AKC
    printf("%s: Called\n", FUNC);
#endif
    /* check args */
    assert(f);
    assert(lt_key);
    assert(rt_key);
    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims < H5O_LAYOUT_NDIMS);
    assert(addr_p);

    /* Allocate new storage */
    assert (udata->key.nbytes > 0);
#ifdef AKC
    printf("calling H5MF_alloc for new chunk\n");
#endif
    if (HADDR_UNDEF==(*addr_p=H5MF_alloc(f, H5FD_MEM_DRAW, (hsize_t)udata->key.nbytes))) {
        HRETURN_ERROR(H5E_IO, H5E_CANTINIT, FAIL,
                      "couldn't allocate new file storage");
    }
    udata->addr = *addr_p;

    /*
     * The left key describes the storage of the UDATA chunk being
     * inserted into the tree.
     */
    lt_key->nbytes = udata->key.nbytes;
    lt_key->filter_mask = udata->key.filter_mask;
    for (u=0; u<udata->mesg.ndims; u++) {
        lt_key->offset[u] = udata->key.offset[u];
    }

    /*
     * The right key might already be present.  If not, then add a zero-width
     * chunk.
     */
    if (H5B_INS_LEFT != op) {
        rt_key->nbytes = 0;
        rt_key->filter_mask = 0;
        for (u=0; u<udata->mesg.ndims; u++) {
            assert (udata->mesg.dim[u] < HSSIZET_MAX);
            assert (udata->key.offset[u]+(hssize_t)(udata->mesg.dim[u]) >
                udata->key.offset[u]);
            rt_key->offset[u] = udata->key.offset[u] +
                    (hssize_t)(udata->mesg.dim[u]);
        }
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_found
 *
 * Purpose:     This function is called when the B-tree search engine has
 *              found the leaf entry that points to a chunk of storage that
 *              contains the beginning of the logical address space
 *              represented by UDATA.  The LT_KEY is the left key (the one
 *              that describes the chunk) and RT_KEY is the right key (the
 *              one that describes the next or last chunk).
 *
 * Note:        It's possible that the chunk isn't really found.  For
 *              instance, in a sparse dataset the requested chunk might fall
 *              between two stored chunks in which case this function is
 *              called with the maximum stored chunk indices less than the
 *              requested chunk indices.
 *
 * Return:      Non-negative on success with information about the chunk
 *              returned through the UDATA argument. Negative on failure.
 *
 * Programmer:  Robb Matzke
 *              Thursday, October  9, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_found(H5F_t UNUSED *f, haddr_t addr, const void *_lt_key,
                 void *_udata, const void UNUSED *_rt_key)
{
    H5F_istore_ud1_t       *udata = (H5F_istore_ud1_t *) _udata;
    const H5F_istore_key_t *lt_key = (const H5F_istore_key_t *) _lt_key;
    unsigned            u;

    FUNC_ENTER(H5F_istore_found, FAIL);

    /* Check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(udata);
    assert(lt_key);

    /* Is this *really* the requested chunk? */
    for (u=0; u<udata->mesg.ndims; u++) {
        if (udata->key.offset[u] >= lt_key->offset[u]+(hssize_t)(udata->mesg.dim[u])) {
            HRETURN(FAIL);
        }
    }

    /* Initialize return values */
    udata->addr = addr;
    udata->key.nbytes = lt_key->nbytes;
    udata->key.filter_mask = lt_key->filter_mask;
    assert (lt_key->nbytes>0);
    for (u = 0; u < udata->mesg.ndims; u++) {
        udata->key.offset[u] = lt_key->offset[u];
    }

    FUNC_LEAVE(SUCCEED);

    _rt_key = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_insert
 *
 * Purpose:     This function is called when the B-tree insert engine finds
 *              the node to use to insert new data.  The UDATA argument
 *              points to a struct that describes the logical addresses being
 *              added to the file.  This function allocates space for the
 *              data and returns information through UDATA describing a
 *              file chunk to receive (part of) the data.
 *
 *              The LT_KEY is always the key describing the chunk of file
 *              memory at address ADDR. On entry, UDATA describes the logical
 *              addresses for which storage is being requested (through the
 *              `offset' and `size' fields). On return, UDATA describes the
 *              logical addresses contained in a chunk on disk.
 *
 * Return:      Success:        An insertion command for the caller, one of
 *                              the H5B_INS_* constants.  The address of the
 *                              new chunk is returned through the NEW_NODE
 *                              argument.
 *
 *              Failure:        H5B_INS_ERROR
 *
 * Programmer:  Robb Matzke
 *              Thursday, October  9, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value. The NEW_NODE argument
 *              is renamed NEW_NODE_P.
 *-------------------------------------------------------------------------
 */
static H5B_ins_t
H5F_istore_insert(H5F_t *f, haddr_t addr, void *_lt_key,
                  hbool_t UNUSED *lt_key_changed,
                  void *_md_key, void *_udata, void *_rt_key,
                  hbool_t UNUSED *rt_key_changed,
                  haddr_t *new_node_p/*out*/)
{
    H5F_istore_key_t    *lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t    *md_key = (H5F_istore_key_t *) _md_key;
    H5F_istore_key_t    *rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t    *udata = (H5F_istore_ud1_t *) _udata;
    int         cmp;
    unsigned            u;
    H5B_ins_t           ret_value = H5B_INS_ERROR;

    FUNC_ENTER(H5F_istore_insert, H5B_INS_ERROR);
#ifdef AKC
    printf("%s: Called\n", FUNC);
#endif

    /* check args */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(lt_key);
    assert(lt_key_changed);
    assert(md_key);
    assert(udata);
    assert(rt_key);
    assert(rt_key_changed);
    assert(new_node_p);

    cmp = H5F_istore_cmp3(f, lt_key, udata, rt_key);
    assert(cmp <= 0);

    if (cmp < 0) {
        /* Negative indices not supported yet */
        assert("HDF5 INTERNAL ERROR -- see rpm" && 0);
        HRETURN_ERROR(H5E_STORAGE, H5E_UNSUPPORTED, H5B_INS_ERROR,
                      "internal error");
        
    } else if (H5V_vector_eq_s (udata->mesg.ndims,
                                udata->key.offset, lt_key->offset) &&
               lt_key->nbytes>0) {
        /*
         * Already exists.  If the new size is not the same as the old size
         * then we should reallocate storage.
         */
        if (lt_key->nbytes != udata->key.nbytes) {
#ifdef AKC
            printf("calling H5MF_realloc for new chunk\n");
#endif
            if (HADDR_UNDEF==(*new_node_p=H5MF_realloc(f, H5FD_MEM_DRAW, addr,
                                  (hsize_t)lt_key->nbytes,
                                  (hsize_t)udata->key.nbytes))) {
                HRETURN_ERROR (H5E_STORAGE, H5E_WRITEERROR, H5B_INS_ERROR,
                       "unable to reallocate chunk storage");
            }
            lt_key->nbytes = udata->key.nbytes;
            lt_key->filter_mask = udata->key.filter_mask;
            *lt_key_changed = TRUE;
            udata->addr = *new_node_p;
            ret_value = H5B_INS_CHANGE;
        } else {
            udata->addr = addr;
            ret_value = H5B_INS_NOOP;
        }

    } else if (H5V_hyper_disjointp(udata->mesg.ndims,
                                   lt_key->offset, udata->mesg.dim,
                                   udata->key.offset, udata->mesg.dim)) {
        assert(H5V_hyper_disjointp(udata->mesg.ndims,
                                   rt_key->offset, udata->mesg.dim,
                                   udata->key.offset, udata->mesg.dim));
        /*
         * Split this node, inserting the new new node to the right of the
         * current node.  The MD_KEY is where the split occurs.
         */
        md_key->nbytes = udata->key.nbytes;
        md_key->filter_mask = udata->key.filter_mask;
        for (u=0; u<udata->mesg.ndims; u++) {
            assert(0 == udata->key.offset[u] % udata->mesg.dim[u]);
            md_key->offset[u] = udata->key.offset[u];
        }

        /*
         * Allocate storage for the new chunk
         */
#ifdef AKC
        printf("calling H5MF_alloc for new chunk\n");
#endif
        if (HADDR_UNDEF==(*new_node_p=H5MF_alloc(f, H5FD_MEM_DRAW,
                             (hsize_t)udata->key.nbytes))) {
            HRETURN_ERROR(H5E_IO, H5E_CANTINIT, H5B_INS_ERROR,
                  "file allocation failed");
        }
        udata->addr = *new_node_p;
        ret_value = H5B_INS_RIGHT;

    } else {
        assert("HDF5 INTERNAL ERROR -- see rpm" && 0);
        HRETURN_ERROR(H5E_IO, H5E_UNSUPPORTED, H5B_INS_ERROR,
                      "internal error");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_iterate
 *
 * Purpose:     Simply counts the number of chunks for a dataset. If the
 *              UDATA.STREAM member is non-null then debugging information is
 *              written to that stream.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_iterate (H5F_t UNUSED *f, void *_lt_key, haddr_t UNUSED addr,
                    void UNUSED *_rt_key, void *_udata)
{
    H5F_istore_ud1_t    *bt_udata = (H5F_istore_ud1_t *)_udata;
    H5F_istore_key_t    *lt_key = (H5F_istore_key_t *)_lt_key;
    unsigned            u;

    FUNC_ENTER(H5F_istore_iterate, FAIL);

    if (bt_udata->stream) {
        if (0==bt_udata->total_storage) {
            fprintf(bt_udata->stream, "    Address:\n");
            fprintf(bt_udata->stream,
                "             Flags    Bytes    Address Logical Offset\n");
            fprintf(bt_udata->stream,
                "        ========== ======== ========== "
                "==============================\n");
        }
        HDfprintf(bt_udata->stream, "        0x%08x %8Zu %10a [",
              lt_key->filter_mask, lt_key->nbytes, addr);
        for (u=0; u<bt_udata->mesg.ndims; u++) {
            HDfprintf(bt_udata->stream, "%s%Hd", u?", ":"", lt_key->offset[u]);
        }
        fputs("]\n", bt_udata->stream);
    }

    bt_udata->total_storage += lt_key->nbytes;
    FUNC_LEAVE(SUCCEED);

    f = 0;
    _rt_key = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_init
 *
 * Purpose:     Initialize the raw data chunk cache for a file.  This is
 *              called when the file handle is initialized.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, May 18, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_init (H5F_t *f)
{
    H5F_rdcc_t  *rdcc = &(f->shared->rdcc);
    
    FUNC_ENTER (H5F_istore_init, FAIL);

    HDmemset (rdcc, 0, sizeof(H5F_rdcc_t));
    if (f->shared->rdcc_nbytes>0 && f->shared->rdcc_nelmts>0) {
        rdcc->nslots = f->shared->rdcc_nelmts;
    assert(rdcc->nslots>=0);
        rdcc->slot = H5FL_ARR_ALLOC (H5F_rdcc_ent_ptr_t,(hsize_t)rdcc->nslots,1);
        if (NULL==rdcc->slot) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                           "memory allocation failed");
        }
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_flush_entry
 *
 * Purpose:     Writes a chunk to disk.  If RESET is non-zero then the
 *              entry is cleared -- it's slightly faster to flush a chunk if
 *              the RESET flag is turned on because it results in one fewer
 *              memory copy.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_flush_entry(H5F_t *f, H5F_rdcc_ent_t *ent, hbool_t reset)
{
    herr_t              ret_value=FAIL; /*return value                  */
    H5F_istore_ud1_t    udata;          /*pass through B-tree           */
    unsigned            u;              /*counters                      */
    void                *buf=NULL;      /*temporary buffer              */
    size_t              alloc;          /*bytes allocated for BUF       */
    hbool_t             point_of_no_return = FALSE;
    
    FUNC_ENTER(H5F_istore_flush_entry, FAIL);
    assert(f);
    assert(ent);
    assert(!ent->locked);

    buf = ent->chunk;
    if (ent->dirty) {
        udata.mesg = *(ent->layout);
        udata.key.filter_mask = 0;
        udata.addr = HADDR_UNDEF;
        udata.key.nbytes = ent->chunk_size;
        for (u=0; u<ent->layout->ndims; u++) {
            udata.key.offset[u] = ent->offset[u];
        }
        alloc = ent->alloc_size;

        /* Should the chunk be filtered before writing it to disk? */
        if (ent->pline && ent->pline->nfilters) {
            if (!reset) {
                /*
                 * Copy the chunk to a new buffer before running it through
                 * the pipeline because we'll want to save the original buffer
                 * for later.
                 */
                alloc = ent->chunk_size;
                if (NULL==(buf = H5F_istore_chunk_alloc(alloc))) {
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                        "memory allocation failed for pipeline");
                }
                HDmemcpy(buf, ent->chunk, ent->chunk_size);
            } else {
                /*
                 * If we are reseting and something goes wrong after this
                 * point then it's too late to recover because we may have
                 * destroyed the original data by calling H5Z_pipeline().
                 * The only safe option is to continue with the reset
                 * even if we can't write the data to disk.
                 */
                point_of_no_return = TRUE;
                ent->chunk = NULL;
            }
            if (H5Z_pipeline(f, ent->pline, 0, &(udata.key.filter_mask),
                     &(udata.key.nbytes), &alloc, &buf)<0) {
                HGOTO_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL,
                    "output pipeline failed");
            }
        }

        /*
         * Create the chunk it if it doesn't exist, or reallocate the chunk if
         * its size changed.  Then write the data into the file.
         */
        if (H5B_insert(f, H5B_ISTORE, ent->layout->addr, ent->split_ratios,
                   &udata)<0) {
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                "unable to allocate chunk");
        }
        if (H5F_block_write(f, H5FD_MEM_DRAW, udata.addr, (hsize_t)udata.key.nbytes, H5P_DEFAULT,
                    buf)<0) {
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                "unable to write raw data to file");
        }

        /* Mark cache entry as clean */
        ent->dirty = FALSE;
        f->shared->rdcc.nflushes++;
    }
    
    /* Reset, but do not free or removed from list */
    if (reset) {
        point_of_no_return = FALSE;
        ent->layout = H5O_free(H5O_LAYOUT, ent->layout);
        ent->pline = H5O_free(H5O_PLINE, ent->pline);
        if (buf==ent->chunk) buf = NULL;
        if(ent->chunk!=NULL)
            ent->chunk = H5F_istore_chunk_free(ent->chunk);
    }
    
    ret_value = SUCCEED;

done:
    /* Free the temp buffer only if it's different than the entry chunk */
    if (buf!=ent->chunk)
        H5F_istore_chunk_free(buf);
    
    /*
     * If we reached the point of no return then we have no choice but to
     * reset the entry.  This can only happen if RESET is true but the
     * output pipeline failed.  Do not free the entry or remove it from the
     * list.
     */
    if (ret_value<0 && point_of_no_return) {
        ent->layout = H5O_free(H5O_LAYOUT, ent->layout);
        ent->pline = H5O_free(H5O_PLINE, ent->pline);
        if(ent->chunk)
            ent->chunk = H5F_istore_chunk_free(ent->chunk);
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5F_istore_preempt
 *
 * Purpose:     Preempts the specified entry from the cache, flushing it to
 *              disk if necessary.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_preempt (H5F_t *f, H5F_rdcc_ent_t *ent)
{
    H5F_rdcc_t          *rdcc = &(f->shared->rdcc);
    
    FUNC_ENTER (H5F_istore_preempt, FAIL);

    assert(f);
    assert(ent);
    assert(!ent->locked);
    assert(ent->idx>=0 && ent->idx<rdcc->nslots);

    /* Flush */
    if (H5F_istore_flush_entry(f, ent, TRUE)<0) {
        HRETURN_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,
                      "cannot flush indexed storage buffer");
    }

    /* Unlink from list */
    if (ent->prev) {
        ent->prev->next = ent->next;
    } else {
        rdcc->head = ent->next;
    }
    if (ent->next) {
        ent->next->prev = ent->prev;
    } else {
        rdcc->tail = ent->prev;
    }
    ent->prev = ent->next = NULL;

    /* Remove from cache */
    rdcc->slot[ent->idx] = NULL;
    ent->idx = -1;
    rdcc->nbytes -= ent->chunk_size;
    --rdcc->nused;

    /* Free */
    H5FL_FREE(H5F_rdcc_ent_t, ent);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_flush
 *
 * Purpose:     Writes all dirty chunks to disk and optionally preempts them
 *              from the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_flush (H5F_t *f, hbool_t preempt)
{
    H5F_rdcc_t          *rdcc = &(f->shared->rdcc);
    int         nerrors=0;
    H5F_rdcc_ent_t      *ent=NULL, *next=NULL;
    
    FUNC_ENTER (H5F_istore_flush, FAIL);

    for (ent=rdcc->head; ent; ent=next) {
        next = ent->next;
        if (preempt) {
            if (H5F_istore_preempt(f, ent)<0) {
                nerrors++;
            }
        } else {
            if (H5F_istore_flush_entry(f, ent, FALSE)<0) {
                nerrors++;
            }
        }
    }
    
    if (nerrors) {
        HRETURN_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL,
                       "unable to flush one or more raw data chunks");
    }
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_dest
 *
 * Purpose:     Destroy the entire chunk cache by flushing dirty entries,
 *              preempting all entries, and freeing the cache itself.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_dest (H5F_t *f)
{
    H5F_rdcc_t          *rdcc = &(f->shared->rdcc);
    int         nerrors=0;
    H5F_rdcc_ent_t      *ent=NULL, *next=NULL;
    
    FUNC_ENTER (H5F_istore_dest, FAIL);

    for (ent=rdcc->head; ent; ent=next) {
#ifdef H5F_ISTORE_DEBUG
        fputc('c', stderr);
        fflush(stderr);
#endif
        next = ent->next;
        if (H5F_istore_preempt(f, ent)<0) {
            nerrors++;
        }
    }
    if (nerrors) {
        HRETURN_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL,
                       "unable to flush one or more raw data chunks");
    }

    H5FL_ARR_FREE (H5F_rdcc_ent_ptr_t,rdcc->slot);
    HDmemset (rdcc, 0, sizeof(H5F_rdcc_t));
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_prune
 *
 * Purpose:     Prune the cache by preempting some things until the cache has
 *              room for something which is SIZE bytes.  Only unlocked
 *              entries are considered for preemption.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_prune (H5F_t *f, size_t size)
{
    int         i, j, nerrors=0;
    H5F_rdcc_t          *rdcc = &(f->shared->rdcc);
    size_t              total = f->shared->rdcc_nbytes;
    const int           nmeth=2;        /*number of methods             */
    int         w[1];           /*weighting as an interval      */
    H5F_rdcc_ent_t      *p[2], *cur;    /*list pointers                 */
    H5F_rdcc_ent_t      *n[2];          /*list next pointers            */

    FUNC_ENTER (H5F_istore_prune, FAIL);

    /*
     * Preemption is accomplished by having multiple pointers (currently two)
     * slide down the list beginning at the head. Pointer p(N+1) will start
     * traversing the list when pointer pN reaches wN percent of the original
     * list.  In other words, preemption method N gets to consider entries in
     * approximate least recently used order w0 percent before method N+1
     * where 100% means tha method N will run to completion before method N+1
     * begins.  The pointers participating in the list traversal are each
     * given a chance at preemption before any of the pointers are advanced.
     */
    w[0] = rdcc->nused * f->shared->rdcc_w0;
    p[0] = rdcc->head;
    p[1] = NULL;

    while ((p[0] || p[1]) && rdcc->nbytes+size>total) {

        /* Introduce new pointers */
        for (i=0; i<nmeth-1; i++) if (0==w[i]) p[i+1] = rdcc->head;
        
        /* Compute next value for each pointer */
        for (i=0; i<nmeth; i++) n[i] = p[i] ? p[i]->next : NULL;

        /* Give each method a chance */
        for (i=0; i<nmeth && rdcc->nbytes+size>total; i++) {
            if (0==i && p[0] && !p[0]->locked &&
                ((0==p[0]->rd_count && 0==p[0]->wr_count) ||
                 (0==p[0]->rd_count && p[0]->chunk_size==p[0]->wr_count) ||
                 (p[0]->chunk_size==p[0]->rd_count && 0==p[0]->wr_count))) {
                /*
                 * Method 0: Preempt entries that have been completely written
                 * and/or completely read but not entries that are partially
                 * written or partially read.
                 */
                cur = p[0];
#ifdef H5F_ISTORE_DEBUG
                putc('.', stderr);
                fflush(stderr);
#endif
                
            } else if (1==i && p[1] && !p[1]->locked) {
                /*
                 * Method 1: Preempt the entry without regard to
                 * considerations other than being locked.  This is the last
                 * resort preemption.
                 */
                cur = p[1];
#ifdef H5F_ISTORE_DEBUG
                putc(':', stderr);
                fflush(stderr);
#endif
                
            } else {
                /* Nothing to preempt at this point */
                cur= NULL;
            }

            if (cur) {
                for (j=0; j<nmeth; j++) {
                    if (p[j]==cur) p[j] = NULL;
                    if (n[j]==cur) n[j] = cur->next;
                }
                if (H5F_istore_preempt(f, cur)<0) nerrors++;
            }
        }
        
        /* Advance pointers */
        for (i=0; i<nmeth; i++) p[i] = n[i];
        for (i=0; i<nmeth-1; i++) w[i] -= 1;
    }

    if (nerrors) {
        HRETURN_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL,
                       "unable to preempt one or more raw data cache entry");
    }
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_lock
 *
 * Purpose:     Return a pointer to a dataset chunk.  The pointer points
 *              directly into the chunk cache and should not be freed
 *              by the caller but will be valid until it is unlocked.  The
 *              input value IDX_HINT is used to speed up cache lookups and
 *              it's output value should be given to H5F_rdcc_unlock().
 *              IDX_HINT is ignored if it is out of range, and if it points
 *              to the wrong entry then we fall back to the normal search
 *              method.
 *
 *              If RELAX is non-zero and the chunk isn't in the cache then
 *              don't try to read it from the file, but just allocate an
 *              uninitialized buffer to hold the result.  This is intended
 *              for output functions that are about to overwrite the entire
 *              chunk.
 *
 * Return:      Success:        Ptr to a file chunk.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              The split ratios are passed in as part of the data transfer
 *              property list.
 *-------------------------------------------------------------------------
 */
static void *
H5F_istore_lock(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
                const H5O_pline_t *pline, const H5O_fill_t *fill,
                const hssize_t offset[], hbool_t relax,
                int *idx_hint/*in,out*/)
{
    int         idx=0;                  /*hash index number     */
    unsigned            temp_idx=0;                     /* temporary index number       */
    hbool_t             found = FALSE;          /*already in cache?     */
    H5F_rdcc_t          *rdcc = &(f->shared->rdcc);/*raw data chunk cache*/
    H5F_rdcc_ent_t      *ent = NULL;            /*cache entry           */
    unsigned            u;                      /*counters              */
    H5F_istore_ud1_t    udata;                  /*B-tree pass-through   */
    size_t              chunk_size=0;           /*size of a chunk       */
    size_t              chunk_alloc=0;          /*allocated chunk size  */
    herr_t              status;                 /*func return status    */
    void                *chunk=NULL;            /*the file chunk        */
    void                *ret_value=NULL;        /*return value          */

    FUNC_ENTER (H5F_istore_lock, NULL);

    if (rdcc->nslots>0) {
        /* We don't care about loss of precision in the following statement. */
        for (u=0, temp_idx=0; u<layout->ndims; u++) {
            temp_idx *= layout->dim[u];
            temp_idx += offset[u];
        }
        temp_idx += (unsigned)(layout->addr);
        idx=H5F_HASH(f,temp_idx);
        ent = rdcc->slot[idx];
        
        if (ent && layout->ndims==ent->layout->ndims &&
                H5F_addr_eq(layout->addr, ent->layout->addr)) {
            for (u=0, found=TRUE; u<ent->layout->ndims; u++) {
                if (offset[u]!=ent->offset[u]) {
                    found = FALSE;
                    break;
                }
            }
        }
    }

    if (found) {
        /*
         * Already in the cache.  Count a hit.
         */
        rdcc->nhits++;

    } else if (!found && relax) {
        /*
         * Not in the cache, but we're about to overwrite the whole thing
         * anyway, so just allocate a buffer for it but don't initialize that
         * buffer with the file contents. Count this as a hit instead of a
         * miss because we saved ourselves lots of work.
         */
#ifdef H5F_ISTORE_DEBUG
        putc('w', stderr);
        fflush(stderr);
#endif
        rdcc->nhits++;
        for (u=0, chunk_size=1; u<layout->ndims; u++) {
            chunk_size *= layout->dim[u];
        }
        chunk_alloc = chunk_size;
        if (NULL==(chunk=H5F_istore_chunk_alloc (chunk_alloc))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                 "memory allocation failed for raw data chunk");
        }
        
    } else {
        /*
         * Not in the cache.  Read it from the file and count this as a miss
         * if it's in the file or an init if it isn't.
         */
        for (u=0, chunk_size=1; u<layout->ndims; u++) {
            udata.key.offset[u] = offset[u];
            chunk_size *= layout->dim[u];
        }
        chunk_alloc = chunk_size;
        udata.mesg = *layout;
        udata.addr = HADDR_UNDEF;
        status = H5B_find (f, H5B_ISTORE, layout->addr, &udata);
        H5E_clear ();
        if (NULL==(chunk = H5F_istore_chunk_alloc (chunk_alloc))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                 "memory allocation failed for raw data chunk");
        }
        if (status>=0 && H5F_addr_defined(udata.addr)) {
            /*
             * The chunk exists on disk.
             */
            if (H5F_block_read(f, H5FD_MEM_DRAW, udata.addr, (hsize_t)udata.key.nbytes, H5P_DEFAULT,
                       chunk)<0) {
                HGOTO_ERROR (H5E_IO, H5E_READERROR, NULL,
                     "unable to read raw data chunk");
            }
            if (H5Z_pipeline(f, pline, H5Z_FLAG_REVERSE,
                     &(udata.key.filter_mask), &(udata.key.nbytes),
                     &chunk_alloc, &chunk)<0 || udata.key.nbytes!=chunk_size) {
                HGOTO_ERROR(H5E_PLINE, H5E_READERROR, NULL,
                    "data pipeline read failed");
            }
            rdcc->nmisses++;
        } else if (fill && fill->buf) {
            /*
             * The chunk doesn't exist in the file.  Replicate the fill
             * value throughout the chunk.
             */
            assert(0==chunk_size % fill->size);
            H5V_array_fill(chunk, fill->buf, fill->size, chunk_size/fill->size);
            rdcc->ninits++;
        } else {
            /*
             * The chunk doesn't exist in the file and no fill value was
             * specified.  Assume all zeros.
             */
            HDmemset (chunk, 0, chunk_size);
            rdcc->ninits++;
        }
    }
    assert (found || chunk_size>0);
    
    if (!found && rdcc->nslots>0 && chunk_size<=f->shared->rdcc_nbytes &&
            (!ent || !ent->locked)) {
        /*
         * Add the chunk to the cache only if the slot is not already locked.
         * Preempt enough things from the cache to make room.
         */
        if (ent) {
#ifdef H5F_ISTORE_DEBUG
            putc('#', stderr);
            fflush(stderr);
#endif
#if 0
            HDfprintf(stderr, "\ncollision %3d %10a {",
                  idx, ent->layout->addr);
            for (u=0; u<layout->ndims; u++) {
                HDfprintf(stderr, "%s%Zu", u?",":"", ent->offset[u]);
            }
            HDfprintf(stderr, "}\n              %10a {", layout->addr);
            for (u=0; u<layout->ndims; u++) {
                HDfprintf(stderr, "%s%Zu", u?",":"", offset[u]);
            }
            fprintf(stderr, "}\n");
#endif
            if (H5F_istore_preempt(f, ent)<0) {
                HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL,
                    "unable to preempt chunk from cache");
            }
        }
        if (H5F_istore_prune(f, chunk_size)<0) {
            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL,
                "unable to preempt chunk(s) from cache");
        }

        /* Create a new entry */
        ent = H5FL_ALLOC(H5F_rdcc_ent_t,0);
        ent->locked = 0;
        ent->dirty = FALSE;
        ent->chunk_size = chunk_size;
        ent->alloc_size = chunk_size;
        ent->layout = H5O_copy(H5O_LAYOUT, layout, NULL);
        ent->pline = H5O_copy(H5O_PLINE, pline, NULL);
        for (u=0; u<layout->ndims; u++) {
            ent->offset[u] = offset[u];
        }
        ent->rd_count = chunk_size;
        ent->wr_count = chunk_size;
        ent->chunk = chunk;
        
        {
            H5D_xfer_t *dxpl;
            dxpl = (H5P_DEFAULT==dxpl_id) ? &H5D_xfer_dflt : (H5D_xfer_t *)H5I_object(dxpl_id);
            ent->split_ratios[0] = dxpl->split_ratios[0];
            ent->split_ratios[1] = dxpl->split_ratios[1];
            ent->split_ratios[2] = dxpl->split_ratios[2];
        }
        
        /* Add it to the cache */
        assert(NULL==rdcc->slot[idx]);
        rdcc->slot[idx] = ent;
        ent->idx = idx;
        rdcc->nbytes += chunk_size;
        rdcc->nused++;

        /* Add it to the linked list */
        ent->next = NULL;
        if (rdcc->tail) {
            rdcc->tail->next = ent;
            ent->prev = rdcc->tail;
            rdcc->tail = ent;
        } else {
            rdcc->head = rdcc->tail = ent;
            ent->prev = NULL;
        }
        found = TRUE;
        
    } else if (!found) {
        /*
         * The chunk is larger than the entire cache so we don't cache it.
         * This is the reason all those arguments have to be repeated for the
         * unlock function.
         */
        ent = NULL;
        idx = INT_MIN;

    } else if (found) {
        /*
         * The chunk is not at the beginning of the cache; move it backward
         * by one slot.  This is how we implement the LRU preemption
         * algorithm.
         */
        if (ent->next) {
            if (ent->next->next) {
                ent->next->next->prev = ent;
            } else {
                rdcc->tail = ent;
            }
            ent->next->prev = ent->prev;
            if (ent->prev) {
                ent->prev->next = ent->next;
            } else {
                rdcc->head = ent->next;
            }
            ent->prev = ent->next;
            ent->next = ent->next->next;
            ent->prev->next = ent;
        }
    }

    /* Lock the chunk into the cache */
    if (ent) {
        assert (!ent->locked);
        ent->locked = TRUE;
        chunk = ent->chunk;
    }

    if (idx_hint)
        *idx_hint = idx;
    ret_value = chunk;
    
 done:
    if (!ret_value)
        H5F_istore_chunk_free (chunk);
    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_unlock
 *
 * Purpose:     Unlocks a previously locked chunk. The LAYOUT, COMP, and
 *              OFFSET arguments should be the same as for H5F_rdcc_lock().
 *              The DIRTY argument should be set to non-zero if the chunk has
 *              been modified since it was locked. The IDX_HINT argument is
 *              the returned index hint from the lock operation and BUF is
 *              the return value from the lock.
 *
 *              The NACCESSED argument should be the number of bytes accessed
 *              for reading or writing (depending on the value of DIRTY).
 *              It's only purpose is to provide additional information to the
 *              preemption policy.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              The split_ratios are passed as part of the data transfer
 *              property list.
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_unlock(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
                  const H5O_pline_t *pline, hbool_t dirty,
                  const hssize_t offset[], int *idx_hint,
                  uint8_t *chunk, size_t naccessed)
{
    H5F_rdcc_t          *rdcc = &(f->shared->rdcc);
    H5F_rdcc_ent_t      *ent = NULL;
    int         found = -1;
    unsigned            u;
    
    FUNC_ENTER (H5F_istore_unlock, FAIL);

    if (INT_MIN==*idx_hint) {
        /*not in cache*/
    } else {
        assert(*idx_hint>=0 && *idx_hint<rdcc->nslots);
        assert(rdcc->slot[*idx_hint]);
        assert(rdcc->slot[*idx_hint]->chunk==chunk);
        found = *idx_hint;
    }
    
    if (found<0) {
        /*
         * It's not in the cache, probably because it's too big.  If it's
         * dirty then flush it to disk.  In any case, free the chunk.
         * Note: we have to copy the layout and filter messages so we
         *       don't discard the `const' qualifier.
         */
        if (dirty) {
            H5F_rdcc_ent_t x;

            HDmemset (&x, 0, sizeof x);
            x.dirty = TRUE;
            x.layout = H5O_copy (H5O_LAYOUT, layout, NULL);
            x.pline = H5O_copy (H5O_PLINE, pline, NULL);
            for (u=0, x.chunk_size=1; u<layout->ndims; u++) {
                x.offset[u] = offset[u];
                x.chunk_size *= layout->dim[u];
            }
            x.alloc_size = x.chunk_size;
            x.chunk = chunk;
            {
            H5D_xfer_t *dxpl;
            dxpl = (H5P_DEFAULT==dxpl_id) ? &H5D_xfer_dflt : (H5D_xfer_t *)H5I_object(dxpl_id);
            x.split_ratios[0] = dxpl->split_ratios[0];
            x.split_ratios[1] = dxpl->split_ratios[1];
            x.split_ratios[2] = dxpl->split_ratios[2];
            }
            
            H5F_istore_flush_entry (f, &x, TRUE);
        } else {
            if(chunk)
                H5F_istore_chunk_free (chunk);
        }
    } else {
        /*
         * It's in the cache so unlock it.
         */
        ent = rdcc->slot[found];
        assert (ent->locked);
        if (dirty) {
            ent->dirty = TRUE;
            ent->wr_count -= MIN (ent->wr_count, naccessed);
        } else {
            ent->rd_count -= MIN (ent->rd_count, naccessed);
        }
        ent->locked = FALSE;
    }
    
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_read
 *
 * Purpose:     Reads a multi-dimensional buffer from (part of) an indexed raw
 *              storage array.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October 15, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              The data transfer property list is passed as an object ID
 *              since that's how the virtual file layer wants it.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_read(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
                const H5O_pline_t *pline, const H5O_fill_t *fill,
                const hssize_t offset_f[], const hsize_t size[], void *buf)
{
    hssize_t            offset_m[H5O_LAYOUT_NDIMS];
    hsize_t             size_m[H5O_LAYOUT_NDIMS];
    hsize_t             idx_cur[H5O_LAYOUT_NDIMS];
    hsize_t             idx_min[H5O_LAYOUT_NDIMS];
    hsize_t             idx_max[H5O_LAYOUT_NDIMS];
    hsize_t             sub_size[H5O_LAYOUT_NDIMS];
    hssize_t            offset_wrt_chunk[H5O_LAYOUT_NDIMS];
    hssize_t            sub_offset_m[H5O_LAYOUT_NDIMS];
    hssize_t            chunk_offset[H5O_LAYOUT_NDIMS];
    int         i, carry;
    unsigned            u;
    size_t              naccessed;              /*bytes accessed in chnk*/
    uint8_t             *chunk=NULL;            /*ptr to a chunk buffer */
    int         idx_hint=0;             /*cache index hint      */
    hsize_t             chunk_size;     /* Bytes in chunk */
    haddr_t             chunk_addr;     /* Chunk address on disk */

    FUNC_ENTER(H5F_istore_read, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED==layout->type);
    assert(layout->ndims>0 && layout->ndims<=H5O_LAYOUT_NDIMS);
    assert(H5F_addr_defined(layout->addr));
    assert(offset_f);
    assert(size);
    assert(buf);

    /*
     * For now, a hyperslab of the file must be read into an array in
     * memory.We do not yet support reading into a hyperslab of memory.
     */
    for (u=0, chunk_size=1; u<layout->ndims; u++) {
        offset_m[u] = 0;
        size_m[u] = size[u];
        chunk_size *= layout->dim[u];
    }
    
#ifndef NDEBUG
    for (u=0; u<layout->ndims; u++) {
        assert(offset_f[u]>=0); /*negative offsets not supported*/
        assert(offset_m[u]>=0); /*negative offsets not supported*/
        assert(size[u]<SIZET_MAX);
        assert(offset_m[u]+(hssize_t)size[u]<=(hssize_t)size_m[u]);
        assert(layout->dim[u]>0);
    }
#endif

    /*
     * Set up multi-dimensional counters (idx_min, idx_max, and idx_cur) and
     * loop through the chunks copying each to its final destination in the
     * application buffer.
     */
    for (u=0; u<layout->ndims; u++) {
        idx_min[u] = offset_f[u] / layout->dim[u];
        idx_max[u] = (offset_f[u]+size[u]-1) / layout->dim[u] + 1;
        idx_cur[u] = idx_min[u];
    }

    /* Loop over all chunks */
    while (1) {
        for (u=0, naccessed=1; u<layout->ndims; u++) {
            /* The location and size of the chunk being accessed */
            assert(layout->dim[u] < HSSIZET_MAX);
            chunk_offset[u] = idx_cur[u] * (hssize_t)(layout->dim[u]);

            /* The offset and size wrt the chunk */
            offset_wrt_chunk[u] = MAX(offset_f[u], chunk_offset[u]) -
                      chunk_offset[u];
            sub_size[u] = MIN((idx_cur[u]+1)*layout->dim[u],
                      offset_f[u]+size[u]) -
                  (chunk_offset[u] + offset_wrt_chunk[u]);
            naccessed *= sub_size[u];
            
            /* Offset into mem buffer */
            sub_offset_m[u] = chunk_offset[u] + offset_wrt_chunk[u] +
                      offset_m[u] - offset_f[u];
        }
        /* Get the address of this chunk on disk */
        chunk_addr=H5F_istore_get_addr(f, layout, chunk_offset);

        /*
         * If the chunk is too large to load into the cache and it has no
         * filters in the pipeline (i.e. not compressed) and if the address
         * for the chunk has been defined, then don't load the chunk into the
         * cache, just read the data from it directly.
         */
        if ((chunk_size>f->shared->rdcc_nbytes && pline->nfilters==0 &&
                chunk_addr!=HADDR_UNDEF)

#ifdef H5_HAVE_PARALLEL
        /*
         * If MPIO is used, must bypass the chunk-cache scheme because other
         * MPI processes could be writing to other elements in the same chunk.
         * Do a direct write-through of only the elements requested.
         */
            || IS_H5FD_MPIO(f)
#endif /* H5_HAVE_PARALLEL */
            ) {
            H5O_layout_t        l;      /* temporary layout */

#ifdef H5_HAVE_PARALLEL
            /* Additional sanity checks when operating in parallel */
            if (chunk_addr==HADDR_UNDEF || pline->nfilters>0)
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL, "unable to locate raw data chunk");
#endif /* H5_HAVE_PARALLEL */
            
            /*
             * use default transfer mode as we do not support collective
             * transfer mode since each data write could decompose into
             * multiple chunk writes and we are not doing the calculation yet.
             */
            l.type = H5D_CONTIGUOUS;
            l.ndims = layout->ndims;
            for (u=l.ndims; u-- > 0; /*void*/)
                l.dim[u] = layout->dim[u];
            l.addr = chunk_addr;
            if (H5F_arr_read(f, H5P_DEFAULT, &l, pline, fill, NULL/*no efl*/,
                     sub_size, size_m, sub_offset_m, offset_wrt_chunk, buf)<0) {
                HRETURN_ERROR (H5E_IO, H5E_READERROR, FAIL,
                     "unable to read raw data from file");
            }
        } else {
            /*
             * Lock the chunk, transfer data to the application, then unlock
             * the chunk.
             */
            if (NULL==(chunk=H5F_istore_lock(f, dxpl_id, layout, pline, fill,
                         chunk_offset, FALSE, &idx_hint))) {
                HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                      "unable to read raw data chunk");
            }
            H5V_hyper_copy(layout->ndims, sub_size, size_m, sub_offset_m,
                   (void*)buf, layout->dim, offset_wrt_chunk, chunk);
            if (H5F_istore_unlock(f, dxpl_id, layout, pline, FALSE,
                      chunk_offset, &idx_hint, chunk,
                      naccessed)<0) {
            HRETURN_ERROR(H5E_IO, H5E_READERROR, FAIL,
                      "unable to unlock raw data chunk");
            }
        }

        /* Increment indices */
        for (i=(int)(layout->ndims-1), carry=1; i>=0 && carry; --i) {
            if (++idx_cur[i]>=idx_max[i])
                idx_cur[i] = idx_min[i];
            else
                carry = 0;
        }
        if (carry)
            break;
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_write
 *
 * Purpose:     Writes a multi-dimensional buffer to (part of) an indexed raw
 *              storage array.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October 15, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              The data transfer property list is passed as an object ID
 *              since that's how the virtual file layer wants it.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_write(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
                 const H5O_pline_t *pline, const H5O_fill_t *fill,
                 const hssize_t offset_f[], const hsize_t size[],
                 const void *buf)
{
    hssize_t    offset_m[H5O_LAYOUT_NDIMS];
    hsize_t             size_m[H5O_LAYOUT_NDIMS];
    int         i, carry;
    unsigned            u;
    hsize_t             idx_cur[H5O_LAYOUT_NDIMS];
    hsize_t             idx_min[H5O_LAYOUT_NDIMS];
    hsize_t             idx_max[H5O_LAYOUT_NDIMS];
    hsize_t             sub_size[H5O_LAYOUT_NDIMS];
    hssize_t    chunk_offset[H5O_LAYOUT_NDIMS];
    hssize_t    offset_wrt_chunk[H5O_LAYOUT_NDIMS];
    hssize_t    sub_offset_m[H5O_LAYOUT_NDIMS];
    uint8_t             *chunk=NULL;
    int         idx_hint=0;
    size_t              chunk_size, naccessed;
    haddr_t             chunk_addr;     /* Chunk address on disk */
    
    FUNC_ENTER(H5F_istore_write, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED==layout->type);
    assert(layout->ndims>0 && layout->ndims<=H5O_LAYOUT_NDIMS);
    assert(H5F_addr_defined(layout->addr));
    assert(offset_f);
    assert(size);
    assert(buf);

    /*
     * For now the source must not be a hyperslab.  It must be an entire
     * memory buffer.
     */
    for (u=0, chunk_size=1; u<layout->ndims; u++) {
        offset_m[u] = 0;
        size_m[u] = size[u];
        chunk_size *= layout->dim[u];
    }

#ifndef NDEBUG
    for (u=0; u<layout->ndims; u++) {
        assert(offset_f[u]>=0); /*negative offsets not supported*/
        assert(offset_m[u]>=0); /*negative offsets not supported*/
        assert(size[u]<SIZET_MAX);
        assert(offset_m[u]+(hssize_t)size[u]<=(hssize_t)size_m[u]);
        assert(layout->dim[u]>0);
    }
#endif

    /*
     * Set up multi-dimensional counters (idx_min, idx_max, and idx_cur) and
     * loop through the chunks copying each chunk from the application to the
     * chunk cache.
     */
    for (u=0; u<layout->ndims; u++) {
        idx_min[u] = offset_f[u] / layout->dim[u];
        idx_max[u] = (offset_f[u]+size[u]-1) / layout->dim[u] + 1;
        idx_cur[u] = idx_min[u];
    }


    /* Loop over all chunks */
    while (1) {
        for (u=0, naccessed=1; u<layout->ndims; u++) {
            /* The location and size of the chunk being accessed */
            assert(layout->dim[u] < HSSIZET_MAX);
            chunk_offset[u] = idx_cur[u] * (hssize_t)(layout->dim[u]);

            /* The offset and size wrt the chunk */
            offset_wrt_chunk[u] = MAX(offset_f[u], chunk_offset[u]) -
                      chunk_offset[u];
            sub_size[u] = MIN((idx_cur[u]+1)*layout->dim[u],
                      offset_f[u]+size[u]) -
                  (chunk_offset[u] + offset_wrt_chunk[u]);
            naccessed *= sub_size[u];
            
            /* Offset into mem buffer */
            sub_offset_m[u] = chunk_offset[u] + offset_wrt_chunk[u] +
                      offset_m[u] - offset_f[u];
        }

        /* Get the address of this chunk on disk */
        chunk_addr=H5F_istore_get_addr(f, layout, chunk_offset);

        /*
         * If the chunk is too large to load into the cache and it has no
         * filters in the pipeline (i.e. not compressed) and if the address
         * for the chunk has been defined, then don't load the chunk into the
         * cache, just write the data to it directly.
         */
        if ((chunk_size>f->shared->rdcc_nbytes && pline->nfilters==0 &&
                chunk_addr!=HADDR_UNDEF)

#ifdef H5_HAVE_PARALLEL
        /*
         * If MPIO is used, must bypass the chunk-cache scheme because other
         * MPI processes could be writing to other elements in the same chunk.
         * Do a direct write-through of only the elements requested.
         */
            || IS_H5FD_MPIO(f)
#endif /* H5_HAVE_PARALLEL */
            ) {
            H5O_layout_t        l;      /* temporary layout */

#ifdef H5_HAVE_PARALLEL
            /* Additional sanity check when operating in parallel */
            if (chunk_addr==HADDR_UNDEF || pline->nfilters>0)
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL, "unable to locate raw data chunk");
#endif /* H5_HAVE_PARALLEL */
            
            /*
             * use default transfer mode as we do not support collective
             * transfer mode since each data write could decompose into
             * multiple chunk writes and we are not doing the calculation yet.
             */
            l.type = H5D_CONTIGUOUS;
            l.ndims = layout->ndims;
            for (u=l.ndims; u-- > 0; /*void*/)
                l.dim[u] = layout->dim[u];
            l.addr = chunk_addr;
            if (H5F_arr_write(f, H5P_DEFAULT, &l, pline, fill, NULL/*no efl*/,
                     sub_size, size_m, sub_offset_m, offset_wrt_chunk, buf)<0) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "unable to write raw data to file");
            }
        } else {
            /*
             * Lock the chunk, copy from application to chunk, then unlock the
             * chunk.
             */
            if (NULL==(chunk=H5F_istore_lock(f, dxpl_id, layout, pline, fill,
                             chunk_offset,
                             (hbool_t)(naccessed==chunk_size),
                             &idx_hint))) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "unable to read raw data chunk");
            }
            H5V_hyper_copy(layout->ndims, sub_size,
               layout->dim, offset_wrt_chunk, chunk, size_m, sub_offset_m, buf);
            if (H5F_istore_unlock(f, dxpl_id, layout, pline, TRUE,
                      chunk_offset, &idx_hint, chunk,
                      naccessed)<0) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "uanble to unlock raw data chunk");
            }
        }
        
        /* Increment indices */
        for (i=layout->ndims-1, carry=1; i>=0 && carry; --i) {
            if (++idx_cur[i]>=idx_max[i])
                idx_cur[i] = idx_min[i];
            else
                carry = 0;
        }
        if (carry)
            break;
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_create
 *
 * Purpose:     Creates a new indexed-storage B-tree and initializes the
 *              istore struct with information about the storage.  The
 *              struct should be immediately written to the object header.
 *
 *              This function must be called before passing ISTORE to any of
 *              the other indexed storage functions!
 *
 * Return:      Non-negative on success (with the ISTORE argument initialized
 *              and ready to write to an object header). Negative on failure.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October 21, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_create(H5F_t *f, H5O_layout_t *layout /*out */ )
{
    H5F_istore_ud1_t    udata;
#ifndef NDEBUG
    unsigned                    u;
#endif

    FUNC_ENTER(H5F_istore_create, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED == layout->type);
    assert(layout->ndims > 0 && layout->ndims <= H5O_LAYOUT_NDIMS);
#ifndef NDEBUG
    for (u = 0; u < layout->ndims; u++) {
        assert(layout->dim[u] > 0);
    }
#endif

    udata.mesg.ndims = layout->ndims;
    if (H5B_create(f, H5B_ISTORE, &udata, &(layout->addr)/*out*/) < 0) {
        HRETURN_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "can't create B-tree");
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_allocated
 *
 * Purpose:     Return the number of bytes allocated in the file for storage
 *              of raw data under the specified B-tree (ADDR is the address
 *              of the B-tree).
 *
 * Return:      Success:        Number of bytes stored in all chunks.
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
hsize_t
H5F_istore_allocated(H5F_t *f, unsigned ndims, haddr_t addr)
{
    H5F_istore_ud1_t    udata;

    FUNC_ENTER(H5F_istore_nchunks, 0);

    HDmemset(&udata, 0, sizeof udata);
    udata.mesg.ndims = ndims;
    if (H5B_iterate(f, H5B_ISTORE, addr, &udata)<0) {
        HRETURN_ERROR(H5E_IO, H5E_CANTINIT, 0,
                      "unable to iterate over chunk B-tree");
    }
    FUNC_LEAVE(udata.total_storage);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_dump_btree
 *
 * Purpose:     Prints information about the storage B-tree to the specified
 *              stream.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 28, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_dump_btree(H5F_t *f, FILE *stream, unsigned ndims, haddr_t addr)
{
    H5F_istore_ud1_t    udata;

    FUNC_ENTER(H5F_istore_dump_btree, FAIL);

    HDmemset(&udata, 0, sizeof udata);
    udata.mesg.ndims = ndims;
    udata.stream = stream;
    if (H5B_iterate(f, H5B_ISTORE, addr, &udata)<0) {
        HRETURN_ERROR(H5E_IO, H5E_CANTINIT, 0,
                      "unable to iterate over chunk B-tree");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_stats
 *
 * Purpose:     Print raw data cache statistics to the debug stream.  If
 *              HEADERS is non-zero then print table column headers,
 *              otherwise assume that the H5AC layer has already printed them.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_stats (H5F_t *f, hbool_t headers)
{
    H5F_rdcc_t  *rdcc = &(f->shared->rdcc);
    double      miss_rate;
    char        ascii[32];
    
    FUNC_ENTER (H5F_istore_stats, FAIL);
    if (!H5DEBUG(AC)) HRETURN(SUCCEED);

    if (headers) {
        fprintf(H5DEBUG(AC), "H5F: raw data cache statistics for file %s\n",
            f->name);
        fprintf(H5DEBUG(AC), "   %-18s %8s %8s %8s %8s+%-8s\n",
            "Layer", "Hits", "Misses", "MissRate", "Inits", "Flushes");
        fprintf(H5DEBUG(AC), "   %-18s %8s %8s %8s %8s-%-8s\n",
            "-----", "----", "------", "--------", "-----", "-------");
    }

#ifdef H5AC_DEBUG
    if (H5DEBUG(AC)) headers = TRUE;
#endif

    if (headers) {
        if (rdcc->nhits>0 || rdcc->nmisses>0) {
            miss_rate = 100.0 * rdcc->nmisses /
                    (rdcc->nhits + rdcc->nmisses);
        } else {
            miss_rate = 0.0;
        }
        if (miss_rate > 100) {
            sprintf(ascii, "%7d%%", (int) (miss_rate + 0.5));
        } else {
            sprintf(ascii, "%7.2f%%", miss_rate);
        }

        fprintf(H5DEBUG(AC), "   %-18s %8u %8u %7s %8d+%-9ld\n",
            "raw data chunks", rdcc->nhits, rdcc->nmisses, ascii,
            rdcc->ninits, (long)(rdcc->nflushes)-(long)(rdcc->ninits));
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_debug
 *
 * Purpose:     Debugs a B-tree node for indexed raw data storage.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_debug(H5F_t *f, haddr_t addr, FILE * stream, int indent,
                 int fwidth, int ndims)
{
    H5F_istore_ud1_t    udata;
    
    FUNC_ENTER (H5F_istore_debug, FAIL);

    HDmemset (&udata, 0, sizeof udata);
    udata.mesg.ndims = ndims;

    H5B_debug (f, addr, stream, indent, fwidth, H5B_ISTORE, &udata);

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_get_addr
 *
 * Purpose:     Get the file address of a chunk if file space has been
 *              assigned.  Save the retrieved information in the udata
 *              supplied.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Albert Cheng
 *              June 27, 1998
 *
 * Modifications:
 *              Modified to return the address instead of returning it through
 *              a parameter - QAK, 1/30/02
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5F_istore_get_addr(H5F_t *f, const H5O_layout_t *layout,
                    const hssize_t offset[])
{
    H5F_istore_ud1_t    udata;                  /* Information about a chunk */
    unsigned    u;
    haddr_t     ret_value=HADDR_UNDEF;          /* Return value */
    
    FUNC_ENTER (H5F_istore_get_addr, HADDR_UNDEF);

    assert(f);
    assert(layout && (layout->ndims > 0));
    assert(offset);

    /* Initialize the information about the chunk we are looking for */
    for (u=0; u<layout->ndims; u++)
        udata.key.offset[u] = offset[u];
    udata.mesg = *layout;
    udata.addr = HADDR_UNDEF;

    /* Go get the chunk information */
    if (H5B_find (f, H5B_ISTORE, layout->addr, &udata)<0) {
        H5E_clear();
        HGOTO_ERROR(H5E_BTREE,H5E_NOTFOUND,HADDR_UNDEF,"Can't locate chunk info");
    } /* end if */

    /* Success!  Set the return value */
    ret_value=udata.addr;

done:
    FUNC_LEAVE (ret_value);
} /* H5F_istore_get_addr() */


/*-------------------------------------------------------------------------
 * Function:    H5F_istore_allocate
 *
 * Purpose:     Allocate file space for all chunks that are not allocated yet.
 *              Return SUCCEED if all needed allocation succeed, otherwise
 *              FAIL.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Note:        Current implementation relies on cache_size being 0,
 *              thus no chunk is cashed and written to disk immediately
 *              when a chunk is unlocked (via H5F_istore_unlock)
 *              This should be changed to do a direct flush independent
 *              of the cache value.
 *
 * Programmer:  Albert Cheng
 *              June 26, 1998
 *
 * Modifications:
 *              rky, 1998-09-23
 *              Added barrier to preclude racing with data writes.
 *
 *              rky, 1998-12-07
 *              Added Wait-Signal wrapper around unlock-lock critical region
 *              to prevent race condition (unlock reads, lock writes the
 *              chunk).
 *
 *              Robb Matzke, 1999-08-02
 *              The split_ratios are passed in as part of the data transfer
 *              property list.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_allocate(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
                    const hsize_t *space_dim, const H5O_pline_t *pline,
                    const H5O_fill_t *fill)
{

    int         i, carry;
    unsigned            u;
    hssize_t            chunk_offset[H5O_LAYOUT_NDIMS];
    uint8_t             *chunk=NULL;
    int         idx_hint=0;
    size_t              chunk_size;
#ifdef AKC
    H5F_istore_ud1_t    udata;
#endif
    
    FUNC_ENTER(H5F_istore_allocate, FAIL);
#ifdef AKC
    printf("Enter %s:\n", FUNC);
#endif

    /* Check args */
    assert(f);
    assert(space_dim);
    assert(pline);
    assert(layout && H5D_CHUNKED==layout->type);
    assert(layout->ndims>0 && layout->ndims<=H5O_LAYOUT_NDIMS);
    assert(H5F_addr_defined(layout->addr));

    /*
     * Setup indice to go through all chunks. (Future improvement
     * should allocate only chunks that have no file space assigned yet.
     */
    for (u=0, chunk_size=1; u<layout->ndims; u++) {
        chunk_offset[u]=0;
        chunk_size *= layout->dim[u];
    }

    /* Loop over all chunks */
    while (1) {
        
#ifdef AKC
        printf("Checking allocation for chunk( ");
        for (u=0; u<layout->ndims; u++){
            printf("%ld ", chunk_offset[u]);
        }
        printf(")\n");
#endif
#ifdef NO
        if (H5F_istore_get_addr(f, layout, chunk_offset, &udata)<0) {
#endif
            /* No file space assigned yet.  Allocate it. */
            /* The following needs improvement like calling the */
            /* allocation directly rather than indirectly using the */
            /* allocation effect in the unlock process. */

#ifdef AKC
            printf("need allocation\n");
#endif
            /*
             * Lock the chunk, copy from application to chunk, then unlock the
             * chunk.
             */

#ifdef H5_HAVE_PARALLEL
            /* rky 981207 Serialize access to this critical region. */
            if (SUCCEED!= H5FD_mpio_wait_for_left_neighbor(f->shared->lf)) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "unable to lock the data chunk");
            }
#endif
            if (NULL==(chunk=H5F_istore_lock(f, dxpl_id, layout, pline,
                          fill, chunk_offset, FALSE, &idx_hint))) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "unable to read raw data chunk");
            }
            if (H5F_istore_unlock(f, dxpl_id, layout, pline, TRUE,
                      chunk_offset, &idx_hint, chunk, chunk_size)<0) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "uanble to unlock raw data chunk");
            }
#ifdef H5_HAVE_PARALLEL
            if (SUCCEED!= H5FD_mpio_signal_right_neighbor(f->shared->lf)) {
                HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL,
                       "unable to unlock the data chunk");
            }
#endif
#ifdef NO
        } else {
#ifdef AKC
            printf("NO need for allocation\n");
            HDfprintf(stdout, "udata.addr=%a\n", udata.addr);
#endif
        }
#endif
        
        /* Increment indices */
        for (i=layout->ndims-1, carry=1; i>=0 && carry; --i) {
            chunk_offset[i] += layout->dim[i];
            if (chunk_offset[i] >= (hssize_t)(space_dim[i])) {
                chunk_offset[i] = 0;
            } else {
                carry = 0;
            }
        }
        if (carry)
            break;
    }

#ifdef H5_HAVE_PARALLEL
    /*
     * rky 980923
     * 
     * The following barrier is a temporary fix to prevent overwriting real
     * data caused by a race between one proc's call of H5F_istore_allocate
     * (from H5D_init_storage, ultimately from H5Dcreate and H5Dextend) and
     * another proc's call of H5Dwrite.  Eventually, this barrier should be
     * removed, when H5D_init_storage is changed to call H5MF_alloc directly
     * to allocate space, instead of calling H5F_istore_unlock.
     */
    if (MPI_Barrier(H5FD_mpio_communicator(f->shared->lf))) {
        HRETURN_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "MPI_Barrier failed");
    }
#endif

    FUNC_LEAVE(SUCCEED);
}
