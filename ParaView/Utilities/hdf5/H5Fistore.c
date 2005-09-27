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

/* Programmer: 	Robb Matzke <matzke@llnl.gov>
 *	       	Wednesday, October  8, 1997
 *
 * Purpose:	Indexed (chunked) I/O functions.  The logical
 *		multi-dimensional data space is regularly partitioned into
 *		same-sized "chunks", the first of which is aligned with the
 *		logical origin.  The chunks are given a multi-dimensional
 *		index which is used as a lookup key in a B-tree that maps
 *		chunk index to disk address.  Each chunk can be compressed
 *		independently and the chunks may move around in the file as
 *		their storage requirements change.
 *
 * Cache:	Disk I/O is performed in units of chunks and H5MF_alloc()
 *		contains code to optionally align chunks on disk block
 *		boundaries for performance.
 *
 *		The chunk cache is an extendible hash indexed by a function
 *		of storage B-tree address and chunk N-dimensional offset
 *		within the dataset.  Collisions are not resolved -- one of
 *		the two chunks competing for the hash slot must be preempted
 *		from the cache.  All entries in the hash also participate in
 *		a doubly-linked list and entries are penalized by moving them
 *		toward the front of the list.  When a new chunk is about to
 *		be added to the cache the heap is pruned by preempting
 *		entries near the front of the list to make room for the new
 *		entry which is added to the end of the list.
 */

#define H5B_PACKAGE		/*suppress error about including H5Bpkg	  */
#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK	H5Fistore_mask

#include "H5private.h"		/* Generic Functions			*/
#include "H5Bpkg.h"		/* B-link trees				*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* Files				*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MFprivate.h"	/* File space management		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Sprivate.h"         /* Dataspaces                           */
#include "H5Vprivate.h"		/* Vector and array functions		*/

/* MPIO, MPIPOSIX, & FPHDF5 drivers needed for special checks */
#include "H5FDfphdf5.h"
#include "H5FDmpio.h"
#include "H5FDmpiposix.h"

/*
 * Feature: If this constant is defined then every cache preemption and load
 *	    causes a character to be printed on the standard error stream:
 *
 *     `.': Entry was preempted because it has been completely read or
 *	    completely written but not partially read and not partially
 *	    written. This is often a good reason for preemption because such
 *	    a chunk will be unlikely to be referenced in the near future.
 *
 *     `:': Entry was preempted because it hasn't been used recently.
 *
 *     `#': Entry was preempted because another chunk collided with it. This
 *	    is usually a relatively bad thing.  If there are too many of
 *	    these then the number of entries in the cache can be increased.
 *
 *       c: Entry was preempted because the file is closing.
 *
 *	 w: A chunk read operation was eliminated because the library is
 *	    about to write new values to the entire chunk.  This is a good
 *	    thing, especially on files where the chunk size is the same as
 *	    the disk block size, chunks are aligned on disk block boundaries,
 *	    and the operating system can also eliminate a read operation.
 */

/*#define H5F_ISTORE_DEBUG */

/* Interface initialization */
static int		interface_initialize_g = 0;
#define INTERFACE_INIT NULL

/*
 * Given a B-tree node return the dimensionality of the chunks pointed to by
 * that node.
 */
#define H5F_ISTORE_NDIMS(X)	((int)(((X)->sizeof_rkey-8)/8))

/* Raw data chunks are cached.  Each entry in the cache is: */
typedef struct H5F_rdcc_ent_t {
    hbool_t	locked;		/*entry is locked in cache		*/
    hbool_t	dirty;		/*needs to be written to disk?		*/
    H5O_layout_t *layout;	/*the layout message			*/
    double	split_ratios[3];/*B-tree node splitting ratios		*/
    H5O_pline_t	*pline;		/*filter pipeline message		*/
    hssize_t	offset[H5O_LAYOUT_NDIMS]; /*chunk name			*/
    size_t	rd_count;	/*bytes remaining to be read		*/
    size_t	wr_count;	/*bytes remaining to be written		*/
    size_t	chunk_size;	/*size of a chunk			*/
    size_t	alloc_size;	/*amount allocated for the chunk	*/
    uint8_t	*chunk;		/*the unfiltered chunk data		*/
    unsigned	idx;		/*index in hash table			*/
    struct H5F_rdcc_ent_t *next;/*next item in doubly-linked list	*/
    struct H5F_rdcc_ent_t *prev;/*previous item in doubly-linked list	*/
} H5F_rdcc_ent_t;
typedef H5F_rdcc_ent_t *H5F_rdcc_ent_ptr_t; /* For free lists */

/* Private prototypes */
static haddr_t H5F_istore_get_addr(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
				  const hssize_t offset[]);

/* B-tree iterator callbacks */
static int H5F_istore_iter_allocated(H5F_t *f, hid_t dxpl_id, void *left_key, haddr_t addr,
				 void *right_key, void *_udata);
static int H5F_istore_iter_dump(H5F_t *f, hid_t dxpl_id, void *left_key, haddr_t addr,
				 void *right_key, void *_udata);
static int H5F_istore_prune_extent(H5F_t *f, hid_t dxpl_id, void *_lt_key, haddr_t addr,
        void *_rt_key, void *_udata);

/* B-tree callbacks */
static size_t H5F_istore_sizeof_rkey(H5F_t *f, const void *_udata);
static herr_t H5F_istore_new_node(H5F_t *f, hid_t dxpl_id, H5B_ins_t, void *_lt_key,
				  void *_udata, void *_rt_key,
				  haddr_t *addr_p /*out*/);
static int H5F_istore_cmp2(H5F_t *f, hid_t dxpl_id, void *_lt_key, void *_udata,
			    void *_rt_key);
static int H5F_istore_cmp3(H5F_t *f, hid_t dxpl_id, void *_lt_key, void *_udata,
			    void *_rt_key);
static herr_t H5F_istore_found(H5F_t *f, hid_t dxpl_id, haddr_t addr, const void *_lt_key,
			       void *_udata, const void *_rt_key);
static H5B_ins_t H5F_istore_insert(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_lt_key,
				   hbool_t *lt_key_changed, void *_md_key,
				   void *_udata, void *_rt_key,
				   hbool_t *rt_key_changed,
				   haddr_t *new_node/*out*/);
static H5B_ins_t H5F_istore_remove( H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_lt_key,
                  hbool_t *lt_key_changed, void *_udata, void *_rt_key,
                  hbool_t *rt_key_changed);
static herr_t H5F_istore_decode_key(H5F_t *f, H5B_t *bt, uint8_t *raw,
				    void *_key);
static herr_t H5F_istore_encode_key(H5F_t *f, H5B_t *bt, uint8_t *raw,
				    void *_key);
static herr_t H5F_istore_debug_key(FILE *stream, H5F_t *f, hid_t dxpl_id,
                                int indent, int fwidth, const void *key,
                                    const void *udata);

/*
 * B-tree key.	A key contains the minimum logical N-dimensional address and
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
    size_t	nbytes;				/*size of stored data	*/
    hssize_t	offset[H5O_LAYOUT_NDIMS];	/*logical offset to start*/
    unsigned	filter_mask;			/*excluded filters	*/
} H5F_istore_key_t;

typedef struct H5F_istore_ud1_t {
    H5F_istore_key_t	key;	                /*key values		*/
    haddr_t		addr;			/*file address of chunk */
    H5O_layout_t	mesg;		        /*layout message	*/
    hsize_t		total_storage;	        /*output from iterator	*/
    FILE		*stream;		/*debug output stream	*/
    hsize_t		*dims;		        /*dataset dimensions	*/
} H5F_istore_ud1_t;

/* inherits B-tree like properties from H5B */
H5B_class_t H5B_ISTORE[1] = {{
    H5B_ISTORE_ID,		/*id			*/
    sizeof(H5F_istore_key_t),	/*sizeof_nkey		*/
    H5F_istore_sizeof_rkey, 	/*get_sizeof_rkey	*/
    H5F_istore_new_node,	/*new			*/
    H5F_istore_cmp2,		/*cmp2			*/
    H5F_istore_cmp3,		/*cmp3			*/
    H5F_istore_found,		/*found			*/
    H5F_istore_insert,		/*insert		*/
    FALSE,			/*follow min branch?	*/
    FALSE,			/*follow max branch?	*/
    H5F_istore_remove,          /*remove		*/
    H5F_istore_decode_key,	/*decode		*/
    H5F_istore_encode_key,	/*encode		*/
    H5F_istore_debug_key,	/*debug			*/
}};

#define H5F_HASH_DIVISOR 1     /* Attempt to spread out the hashing */
                               /* This should be the same size as the alignment of */
                               /* of the smallest file format object written to the file.  */
#define H5F_HASH(F,ADDR) H5F_addr_hash((ADDR/H5F_HASH_DIVISOR),(F)->shared->rdcc.nslots)


/* Declare a free list to manage H5F_rdcc_ent_t objects */
H5FL_DEFINE_STATIC(H5F_rdcc_ent_t);

/* Declare a PQ free list to manage the H5F_rdcc_ent_ptr_t array information */
H5FL_ARR_DEFINE_STATIC(H5F_rdcc_ent_ptr_t,-1);


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_sizeof_rkey
 *
 * Purpose:	Returns the size of a raw key for the specified UDATA.	The
 *		size of the key is dependent on the number of dimensions for
 *		the object to which this B-tree points.	 The dimensionality
 *		of the UDATA is the only portion that's referenced here.
 *
 * Return:	Success:	Size of raw key in bytes.
 *
 *		Failure:	abort()
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5F_istore_sizeof_rkey(H5F_t UNUSED *f, const void *_udata)
{
    const H5F_istore_ud1_t *udata = (const H5F_istore_ud1_t *) _udata;
    size_t		    nbytes;

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_istore_sizeof_rkey);

    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims <= H5O_LAYOUT_NDIMS);

    nbytes = 4 +			/*storage size		*/
	     4 +			/*filter mask		*/
	     udata->mesg.ndims*8;	/*dimension indices	*/

    FUNC_LEAVE_NOAPI(nbytes);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_decode_key
 *
 * Purpose:	Decodes a raw key into a native key for the B-tree
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_decode_key(H5F_t UNUSED *f, H5B_t *bt, uint8_t *raw, void *_key)
{
    H5F_istore_key_t	*key = (H5F_istore_key_t *) _key;
    int		i;
    int		ndims = H5F_ISTORE_NDIMS(bt);
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_decode_key, FAIL);

    /* check args */
    assert(f);
    assert(bt);
    assert(raw);
    assert(key);
    assert(ndims>0 && ndims<=H5O_LAYOUT_NDIMS);

    /* decode */
    UINT32DECODE(raw, key->nbytes);
    UINT32DECODE(raw, key->filter_mask);
    for (i=0; i<ndims; i++)
	UINT64DECODE(raw, key->offset[i]);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_encode_key
 *
 * Purpose:	Encode a key from native format to raw format.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, October 10, 1997
 *
 * Modifications:
 *	
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_encode_key(H5F_t UNUSED *f, H5B_t *bt, uint8_t *raw, void *_key)
{
    H5F_istore_key_t	*key = (H5F_istore_key_t *) _key;
    int		ndims = H5F_ISTORE_NDIMS(bt);
    int		i;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_encode_key, FAIL);

    /* check args */
    assert(f);
    assert(bt);
    assert(raw);
    assert(key);
    assert(ndims>0 && ndims<=H5O_LAYOUT_NDIMS);

    /* encode */
    UINT32ENCODE(raw, key->nbytes);
    UINT32ENCODE(raw, key->filter_mask);
    for (i=0; i<ndims; i++)
	UINT64ENCODE(raw, key->offset[i]);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_debug_key
 *
 * Purpose:	Prints a key.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_debug_key (FILE *stream, H5F_t UNUSED *f, hid_t UNUSED dxpl_id, int indent, int fwidth,
		      const void *_key, const void *_udata)
{
    const H5F_istore_key_t	*key = (const H5F_istore_key_t *)_key;
    const H5F_istore_ud1_t	*udata = (const H5F_istore_ud1_t *)_udata;
    unsigned		u;
    herr_t ret_value=SUCCEED;   /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_debug_key, FAIL);

    assert (key);

    HDfprintf(stream, "%*s%-*s %Zd bytes\n", indent, "", fwidth,
	      "Chunk size:", key->nbytes);
    HDfprintf(stream, "%*s%-*s 0x%08x\n", indent, "", fwidth,
	      "Filter mask:", key->filter_mask);
    HDfprintf(stream, "%*s%-*s {", indent, "", fwidth,
	      "Logical offset:");
    for (u=0; u<udata->mesg.ndims; u++)
        HDfprintf (stream, "%s%Hd", u?", ":"", key->offset[u]);
    HDfputs ("}\n", stream);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_cmp2
 *
 * Purpose:	Compares two keys sort of like strcmp().  The UDATA pointer
 *		is only to supply extra information not carried in the keys
 *		(in this case, the dimensionality) and is not compared
 *		against the keys.
 *
 * Return:	Success:	-1 if LT_KEY is less than RT_KEY;
 *				1 if LT_KEY is greater than RT_KEY;
 *				0 if LT_KEY and RT_KEY are equal.
 *
 *		Failure:	FAIL (same as LT_KEY<RT_KEY)
 *
 * Programmer:	Robb Matzke
 *		Thursday, November  6, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_istore_cmp2(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_lt_key, void *_udata,
		void *_rt_key)
{
    H5F_istore_key_t	*lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t	*rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t	*udata = (H5F_istore_ud1_t *) _udata;
    int		ret_value;

    FUNC_ENTER_NOAPI(H5F_istore_cmp2, FAIL);

    assert(lt_key);
    assert(rt_key);
    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims <= H5O_LAYOUT_NDIMS);

    /* Compare the offsets but ignore the other fields */
    ret_value = H5V_vector_cmp_s(udata->mesg.ndims, lt_key->offset, rt_key->offset);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_cmp3
 *
 * Purpose:	Compare the requested datum UDATA with the left and right
 *		keys of the B-tree.
 *
 * Return:	Success:	negative if the min_corner of UDATA is less
 *				than the min_corner of LT_KEY.
 *
 *				positive if the min_corner of UDATA is
 *				greater than or equal the min_corner of
 *				RT_KEY.
 *
 *				zero otherwise.	 The min_corner of UDATA is
 *				not necessarily contained within the address
 *				space represented by LT_KEY, but a key that
 *				would describe the UDATA min_corner address
 *				would fall lexicographically between LT_KEY
 *				and RT_KEY.
 *				
 *		Failure:	FAIL (same as UDATA < LT_KEY)
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_istore_cmp3(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_lt_key, void *_udata,
		void *_rt_key)
{
    H5F_istore_key_t	*lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t	*rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t	*udata = (H5F_istore_ud1_t *) _udata;
    int		ret_value = 0;

    FUNC_ENTER_NOAPI(H5F_istore_cmp3, FAIL);

    assert(lt_key);
    assert(rt_key);
    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims <= H5O_LAYOUT_NDIMS);

    if (H5V_vector_lt_s(udata->mesg.ndims, udata->key.offset,
			lt_key->offset)) {
	ret_value = -1;
    } else if (H5V_vector_ge_s(udata->mesg.ndims, udata->key.offset,
			     rt_key->offset)) {
	ret_value = 1;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_new_node
 *
 * Purpose:	Adds a new entry to an i-storage B-tree.  We can assume that
 *		the domain represented by UDATA doesn't intersect the domain
 *		already represented by the B-tree.
 *
 * Return:	Success:	Non-negative. The address of leaf is returned
 *				through the ADDR argument.  It is also added
 *				to the UDATA.
 *
 * 		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Tuesday, October 14, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_new_node(H5F_t *f, hid_t dxpl_id, H5B_ins_t op,
		    void *_lt_key, void *_udata, void *_rt_key,
		    haddr_t *addr_p/*out*/)
{
    H5F_istore_key_t	*lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t	*rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t	*udata = (H5F_istore_ud1_t *) _udata;
    unsigned		u;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_new_node, FAIL);

    /* check args */
    assert(f);
    assert(lt_key);
    assert(rt_key);
    assert(udata);
    assert(udata->mesg.ndims > 0 && udata->mesg.ndims < H5O_LAYOUT_NDIMS);
    assert(addr_p);

    /* Allocate new storage */
    assert (udata->key.nbytes > 0);
    H5_CHECK_OVERFLOW( udata->key.nbytes ,size_t, hsize_t);
    if (HADDR_UNDEF==(*addr_p=H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, (hsize_t)udata->key.nbytes)))
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "couldn't allocate new file storage");
    udata->addr = *addr_p;

    /*
     * The left key describes the storage of the UDATA chunk being
     * inserted into the tree.
     */
    lt_key->nbytes = udata->key.nbytes;
    lt_key->filter_mask = udata->key.filter_mask;
    for (u=0; u<udata->mesg.ndims; u++)
        lt_key->offset[u] = udata->key.offset[u];

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

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_found
 *
 * Purpose:	This function is called when the B-tree search engine has
 *		found the leaf entry that points to a chunk of storage that
 *		contains the beginning of the logical address space
 *		represented by UDATA.  The LT_KEY is the left key (the one
 *		that describes the chunk) and RT_KEY is the right key (the
 *		one that describes the next or last chunk).
 *
 * Note:	It's possible that the chunk isn't really found.  For
 *		instance, in a sparse dataset the requested chunk might fall
 *		between two stored chunks in which case this function is
 *		called with the maximum stored chunk indices less than the
 *		requested chunk indices.
 *
 * Return:	Non-negative on success with information about the chunk
 *		returned through the UDATA argument. Negative on failure.
 *
 * Programmer:	Robb Matzke
 *		Thursday, October  9, 1997
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_found(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, haddr_t addr, const void *_lt_key,
		 void *_udata, const void UNUSED *_rt_key)
{
    H5F_istore_ud1_t	   *udata = (H5F_istore_ud1_t *) _udata;
    const H5F_istore_key_t *lt_key = (const H5F_istore_key_t *) _lt_key;
    unsigned		u;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_found, FAIL);

    /* Check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(udata);
    assert(lt_key);

    /* Is this *really* the requested chunk? */
    for (u=0; u<udata->mesg.ndims; u++) {
        if (udata->key.offset[u] >= lt_key->offset[u]+(hssize_t)(udata->mesg.dim[u]))
            HGOTO_DONE(FAIL);
    }

    /* Initialize return values */
    udata->addr = addr;
    udata->key.nbytes = lt_key->nbytes;
    udata->key.filter_mask = lt_key->filter_mask;
    assert (lt_key->nbytes>0);
    for (u = 0; u < udata->mesg.ndims; u++)
        udata->key.offset[u] = lt_key->offset[u];

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_insert
 *
 * Purpose:	This function is called when the B-tree insert engine finds
 *		the node to use to insert new data.  The UDATA argument
 *		points to a struct that describes the logical addresses being
 *		added to the file.  This function allocates space for the
 *		data and returns information through UDATA describing a
 *		file chunk to receive (part of) the data.
 *
 *		The LT_KEY is always the key describing the chunk of file
 *		memory at address ADDR. On entry, UDATA describes the logical
 *		addresses for which storage is being requested (through the
 *		`offset' and `size' fields). On return, UDATA describes the
 *		logical addresses contained in a chunk on disk.
 *
 * Return:	Success:	An insertion command for the caller, one of
 *				the H5B_INS_* constants.  The address of the
 *				new chunk is returned through the NEW_NODE
 *				argument.
 *
 *		Failure:	H5B_INS_ERROR
 *
 * Programmer:	Robb Matzke
 *		Thursday, October  9, 1997
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value. The NEW_NODE argument
 *		is renamed NEW_NODE_P.
 *-------------------------------------------------------------------------
 */
static H5B_ins_t
H5F_istore_insert(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_lt_key,
		  hbool_t UNUSED *lt_key_changed,
		  void *_md_key, void *_udata, void *_rt_key,
		  hbool_t UNUSED *rt_key_changed,
		  haddr_t *new_node_p/*out*/)
{
    H5F_istore_key_t	*lt_key = (H5F_istore_key_t *) _lt_key;
    H5F_istore_key_t	*md_key = (H5F_istore_key_t *) _md_key;
    H5F_istore_key_t	*rt_key = (H5F_istore_key_t *) _rt_key;
    H5F_istore_ud1_t	*udata = (H5F_istore_ud1_t *) _udata;
    int		cmp;
    unsigned		u;
    H5B_ins_t		ret_value;

    FUNC_ENTER_NOAPI(H5F_istore_insert, H5B_INS_ERROR);

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

    cmp = H5F_istore_cmp3(f, dxpl_id, lt_key, udata, rt_key);
    assert(cmp <= 0);

    if (cmp < 0) {
        /* Negative indices not supported yet */
        assert("HDF5 INTERNAL ERROR -- see rpm" && 0);
        HGOTO_ERROR(H5E_STORAGE, H5E_UNSUPPORTED, H5B_INS_ERROR, "internal error");
	
    } else if (H5V_vector_eq_s (udata->mesg.ndims,
				udata->key.offset, lt_key->offset) &&
	       lt_key->nbytes>0) {
        /*
         * Already exists.  If the new size is not the same as the old size
         * then we should reallocate storage.
         */
        if (lt_key->nbytes != udata->key.nbytes) {
/* Currently, the old chunk data is "thrown away" after the space is reallocated,
 * so avoid data copy in H5MF_realloc() call by just free'ing the space and
 * allocating new space.
 * 
 * This should keep the file smaller also, by freeing the space and then
 * allocating new space, instead of vice versa (in H5MF_realloc).
 *
 * QAK - 11/19/2002
 */
#ifdef OLD_WAY
            if (HADDR_UNDEF==(*new_node_p=H5MF_realloc(f, H5FD_MEM_DRAW, addr,
                      (hsize_t)lt_key->nbytes, (hsize_t)udata->key.nbytes)))
                HGOTO_ERROR (H5E_STORAGE, H5E_NOSPACE, H5B_INS_ERROR, "unable to reallocate chunk storage");
#else /* OLD_WAY */
            H5_CHECK_OVERFLOW( lt_key->nbytes ,size_t, hsize_t);
            if (H5MF_xfree(f, H5FD_MEM_DRAW, dxpl_id, addr, (hsize_t)lt_key->nbytes)<0)
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTFREE, H5B_INS_ERROR, "unable to free chunk");
            H5_CHECK_OVERFLOW( udata->key.nbytes ,size_t, hsize_t);
            if (HADDR_UNDEF==(*new_node_p=H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, (hsize_t)udata->key.nbytes)))
                HGOTO_ERROR(H5E_STORAGE, H5E_NOSPACE, H5B_INS_ERROR, "unable to reallocate chunk");
#endif /* OLD_WAY */
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
        H5_CHECK_OVERFLOW( udata->key.nbytes ,size_t, hsize_t);
        if (HADDR_UNDEF==(*new_node_p=H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, (hsize_t)udata->key.nbytes)))
            HGOTO_ERROR(H5E_STORAGE, H5E_NOSPACE, H5B_INS_ERROR, "file allocation failed");
        udata->addr = *new_node_p;
        ret_value = H5B_INS_RIGHT;

    } else {
        assert("HDF5 INTERNAL ERROR -- see rpm" && 0);
        HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, H5B_INS_ERROR, "internal error");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_iter_allocated
 *
 * Purpose:	Simply counts the number of chunks for a dataset.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *
 *		Quincey Koziol, 2002-04-22
 *		Changed to callback from H5B_iterate
 *-------------------------------------------------------------------------
 */
static int 
H5F_istore_iter_allocated (H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_lt_key, haddr_t UNUSED addr,
		    void UNUSED *_rt_key, void *_udata)
{
    H5F_istore_ud1_t	*bt_udata = (H5F_istore_ud1_t *)_udata;
    H5F_istore_key_t	*lt_key = (H5F_istore_key_t *)_lt_key;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_istore_iter_allocated);

    bt_udata->total_storage += lt_key->nbytes;

    FUNC_LEAVE_NOAPI(H5B_ITER_CONT);
} /* H5F_istore_iter_allocated() */


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_iter_dump
 *
 * Purpose:	If the UDATA.STREAM member is non-null then debugging
 *              information is written to that stream.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *
 *		Quincey Koziol, 2002-04-22
 *		Changed to callback from H5B_iterate
 *-------------------------------------------------------------------------
 */
static int
H5F_istore_iter_dump (H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_lt_key, haddr_t UNUSED addr,
		    void UNUSED *_rt_key, void *_udata)
{
    H5F_istore_ud1_t	*bt_udata = (H5F_istore_ud1_t *)_udata;
    H5F_istore_key_t	*lt_key = (H5F_istore_key_t *)_lt_key;
    unsigned		u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_istore_iter_dump);

    if (bt_udata->stream) {
        if (0==bt_udata->total_storage) {
            fprintf(bt_udata->stream,
                "             Flags    Bytes    Address Logical Offset\n");
            fprintf(bt_udata->stream,
                "        ========== ======== ========== "
                "==============================\n");
        }
        HDfprintf(bt_udata->stream, "        0x%08x %8Zu %10a [",
              lt_key->filter_mask, lt_key->nbytes, addr);
        for (u=0; u<bt_udata->mesg.ndims; u++)
            HDfprintf(bt_udata->stream, "%s%Hd", u?", ":"", lt_key->offset[u]);
        HDfputs("]\n", bt_udata->stream);

        /* Use "total storage" information as flag for printing headers */
        bt_udata->total_storage++;
    }

    FUNC_LEAVE_NOAPI(H5B_ITER_CONT);
} /* H5F_istore_iter_dump() */


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_init
 *
 * Purpose:	Initialize the raw data chunk cache for a file.  This is
 *		called when the file handle is initialized.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, May 18, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_init (H5F_t *f)
{
    H5F_rdcc_t	*rdcc = &(f->shared->rdcc);
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_init, FAIL);

    HDmemset (rdcc, 0, sizeof(H5F_rdcc_t));
    if (f->shared->rdcc_nbytes>0 && f->shared->rdcc_nelmts>0) {
	rdcc->nslots = f->shared->rdcc_nelmts;
	rdcc->slot = H5FL_ARR_CALLOC (H5F_rdcc_ent_ptr_t,rdcc->nslots);
	if (NULL==rdcc->slot)
	    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_flush_entry
 *
 * Purpose:	Writes a chunk to disk.  If RESET is non-zero then the
 *		entry is cleared -- it's slightly faster to flush a chunk if
 *		the RESET flag is turned on because it results in one fewer
 *		memory copy.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_flush_entry(H5F_t *f, hid_t dxpl_id, H5F_rdcc_ent_t *ent, hbool_t reset)
{
    herr_t		ret_value=SUCCEED;	/*return value			*/
    H5F_istore_ud1_t 	udata;		/*pass through B-tree		*/
    unsigned		u;		/*counters			*/
    void		*buf=NULL;	/*temporary buffer		*/
    size_t		alloc;		/*bytes allocated for BUF	*/
    hbool_t		point_of_no_return = FALSE;
   
    FUNC_ENTER_NOAPI_NOINIT(H5F_istore_flush_entry);

    assert(f);
    assert(ent);
    assert(!ent->locked);
    HDmemset(&udata, 0, sizeof(H5F_istore_ud1_t));

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
            H5P_genplist_t *plist;   /* Data xfer property list */
            H5Z_cb_t            cb_struct;
            H5Z_EDC_t           edc;

            if (!reset) {
                /*
                 * Copy the chunk to a new buffer before running it through
                 * the pipeline because we'll want to save the original buffer
                 * for later.
                 */
                alloc = ent->chunk_size;
                if (NULL==(buf = H5MM_malloc(alloc))) {
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
            /* Don't know whether we should involve transfer property list.  So
             * just pass in H5Z_ENABLE_EDC and default callback setting for data 
             * read. */
            if (NULL == (plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list");
            if(H5P_get(plist,H5D_XFER_EDC_NAME,&edc)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get edc information");
            if(H5P_get(plist,H5D_XFER_FILTER_CB_NAME,&cb_struct)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get filter callback struct");
            if (H5Z_pipeline(ent->pline, 0, &(udata.key.filter_mask), edc, 
                             cb_struct, &(udata.key.nbytes), &alloc, &buf)<0) {
                HGOTO_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL,
                    "output pipeline failed");
            }
        }

        /*
         * Create the chunk it if it doesn't exist, or reallocate the chunk if
         * its size changed.  Then write the data into the file.
         */
        if (H5B_insert(f, dxpl_id, H5B_ISTORE, ent->layout->addr, ent->split_ratios, &udata)<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to allocate chunk");
        if (H5F_block_write(f, H5FD_MEM_DRAW, udata.addr, udata.key.nbytes, dxpl_id, buf)<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file");

        /* Mark cache entry as clean */
        ent->dirty = FALSE;
        f->shared->rdcc.nflushes++;
    }
    
    /* Reset, but do not free or removed from list */
    if (reset) {
        point_of_no_return = FALSE;
        ent->layout = H5O_free(H5O_LAYOUT_ID, ent->layout);
        ent->pline = H5O_free(H5O_PLINE_ID, ent->pline);
        if (buf==ent->chunk) buf = NULL;
        if(ent->chunk!=NULL)
            ent->chunk = H5MM_xfree(ent->chunk);
    }
    
done:
    /* Free the temp buffer only if it's different than the entry chunk */
    if (buf!=ent->chunk)
        H5MM_xfree(buf);
    
    /*
     * If we reached the point of no return then we have no choice but to
     * reset the entry.  This can only happen if RESET is true but the
     * output pipeline failed.  Do not free the entry or remove it from the
     * list.
     */
    if (ret_value<0 && point_of_no_return) {
        ent->layout = H5O_free(H5O_LAYOUT_ID, ent->layout);
        ent->pline = H5O_free(H5O_PLINE_ID, ent->pline);
        if(ent->chunk)
            ent->chunk = H5MM_xfree(ent->chunk);
    }
    FUNC_LEAVE_NOAPI(ret_value);
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
 *      Pedro Vicente, March 28, 2002
 *      Added flush parameter that switches the call to H5F_istore_flush_entry 
 *      The call with FALSE is used by the H5F_istore_prune_by_extent function
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_preempt(H5F_t *f, hid_t dxpl_id, H5F_rdcc_ent_t * ent, hbool_t flush)
{
    H5F_rdcc_t             *rdcc = &(f->shared->rdcc);
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5F_istore_preempt);

    assert(f);
    assert(ent);
    assert(!ent->locked);
    assert(ent->idx < rdcc->nslots);

    if(flush) {
	/* Flush */
	if(H5F_istore_flush_entry(f, dxpl_id, ent, TRUE) < 0)
	    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer");
    }
    else {
        /* Don't flush, just free chunk */
	ent->layout = H5O_free(H5O_LAYOUT_ID, ent->layout);
	ent->pline = H5O_free(H5O_PLINE_ID, ent->pline);
	if(ent->chunk != NULL)
	    ent->chunk = H5MM_xfree(ent->chunk);
    }

    /* Unlink from list */
    if(ent->prev)
	ent->prev->next = ent->next;
    else
	rdcc->head = ent->next;
    if(ent->next)
	ent->next->prev = ent->prev;
    else
	rdcc->tail = ent->prev;
    ent->prev = ent->next = NULL;

    /* Remove from cache */
    rdcc->slot[ent->idx] = NULL;
    ent->idx = UINT_MAX;
    rdcc->nbytes -= ent->chunk_size;
    --rdcc->nused;

    /* Free */
    H5FL_FREE(H5F_rdcc_ent_t, ent);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_flush
 *
 * Purpose:	Writes all dirty chunks to disk and optionally preempts them
 *		from the cache.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *      Pedro Vicente, March 28, 2002
 *      Added TRUE parameter to the call to H5F_istore_preempt
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_flush (H5F_t *f, hid_t dxpl_id, unsigned flags)
{
    H5F_rdcc_t		*rdcc = &(f->shared->rdcc);
    int		nerrors=0;
    H5F_rdcc_ent_t	*ent=NULL, *next=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_flush, FAIL);

    for (ent=rdcc->head; ent; ent=next) {
	next = ent->next;
	if ((flags&H5F_FLUSH_CLEAR_ONLY)) {
            /* Just mark cache entry as clean */
            ent->dirty = FALSE;
        } /* end if */
	else if ((flags&H5F_FLUSH_INVALIDATE)) {
	    if (H5F_istore_preempt(f, dxpl_id, ent, TRUE )<0)
		nerrors++;
	} else {
	    if (H5F_istore_flush_entry(f, dxpl_id, ent, FALSE)<0)
		nerrors++;
	}
    }
    
    if (nerrors)
	HGOTO_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_dest
 *
 * Purpose:	Destroy the entire chunk cache by flushing dirty entries,
 *		preempting all entries, and freeing the cache itself.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *      Pedro Vicente, March 28, 2002
 *      Added TRUE parameter to the call to H5F_istore_preempt
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_dest (H5F_t *f, hid_t dxpl_id)
{
    H5F_rdcc_t		*rdcc = &(f->shared->rdcc);
    int		nerrors=0;
    H5F_rdcc_ent_t	*ent=NULL, *next=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_dest, FAIL);

    for (ent=rdcc->head; ent; ent=next) {
#ifdef H5F_ISTORE_DEBUG
	HDfputc('c', stderr);
	HDfflush(stderr);
#endif
	next = ent->next;
	if (H5F_istore_preempt(f, dxpl_id, ent, TRUE )<0)
	    nerrors++;
    }
    if (nerrors)
	HGOTO_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks");

    H5FL_ARR_FREE (H5F_rdcc_ent_ptr_t,rdcc->slot);
    HDmemset (rdcc, 0, sizeof(H5F_rdcc_t));

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_prune
 *
 * Purpose:	Prune the cache by preempting some things until the cache has
 *		room for something which is SIZE bytes.  Only unlocked
 *		entries are considered for preemption.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *      Pedro Vicente, March 28, 2002
 *      TRUE parameter to the call to H5F_istore_preempt
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_prune (H5F_t *f, hid_t dxpl_id, size_t size)
{
    int		i, j, nerrors=0;
    H5F_rdcc_t		*rdcc = &(f->shared->rdcc);
    size_t		total = f->shared->rdcc_nbytes;
    const int		nmeth=2;	/*number of methods		*/
    int		        w[1];		/*weighting as an interval	*/
    H5F_rdcc_ent_t	*p[2], *cur;	/*list pointers			*/
    H5F_rdcc_ent_t	*n[2];		/*list next pointers		*/
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5F_istore_prune);

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
    w[0] = (int)(rdcc->nused * f->shared->rdcc_w0);
    p[0] = rdcc->head;
    p[1] = NULL;

    while ((p[0] || p[1]) && rdcc->nbytes+size>total) {

	/* Introduce new pointers */
	for (i=0; i<nmeth-1; i++)
            if (0==w[i])
                p[i+1] = rdcc->head;
	
	/* Compute next value for each pointer */
	for (i=0; i<nmeth; i++)
            n[i] = p[i] ? p[i]->next : NULL;

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
		HDputc('.', stderr);
		HDfflush(stderr);
#endif
		
	    } else if (1==i && p[1] && !p[1]->locked) {
		/*
		 * Method 1: Preempt the entry without regard to
		 * considerations other than being locked.  This is the last
		 * resort preemption.
		 */
		cur = p[1];
#ifdef H5F_ISTORE_DEBUG
		HDputc(':', stderr);
		HDfflush(stderr);
#endif
		
	    } else {
		/* Nothing to preempt at this point */
		cur= NULL;
	    }

	    if (cur) {
		for (j=0; j<nmeth; j++) {
		    if (p[j]==cur)
                        p[j] = NULL;
		    if (n[j]==cur)
                        n[j] = cur->next;
		}
		if (H5F_istore_preempt(f, dxpl_id, cur, TRUE)<0)
                    nerrors++;
	    }
	}
	
	/* Advance pointers */
	for (i=0; i<nmeth; i++)
            p[i] = n[i];
	for (i=0; i<nmeth-1; i++)
            w[i] -= 1;
    }

    if (nerrors)
	HGOTO_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL, "unable to preempt one or more raw data cache entry");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_lock
 *
 * Purpose:	Return a pointer to a dataset chunk.  The pointer points
 *		directly into the chunk cache and should not be freed
 *		by the caller but will be valid until it is unlocked.  The
 *		input value IDX_HINT is used to speed up cache lookups and
 *		it's output value should be given to H5F_istore_unlock().
 *		IDX_HINT is ignored if it is out of range, and if it points
 *		to the wrong entry then we fall back to the normal search
 *		method.
 *
 *		If RELAX is non-zero and the chunk isn't in the cache then
 *		don't try to read it from the file, but just allocate an
 *		uninitialized buffer to hold the result.  This is intended
 *		for output functions that are about to overwrite the entire
 *		chunk.
 *
 * Return:	Success:	Ptr to a file chunk.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *		Robb Matzke, 1999-08-02
 *		The split ratios are passed in as part of the data transfer
 *		property list.
 *
 *              Pedro Vicente, March 28, 2002
 *              TRUE parameter to the call to H5F_istore_preempt
 *-------------------------------------------------------------------------
 */
static void *
H5F_istore_lock(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
    const H5O_pline_t *pline, const H5O_fill_t *fill, H5D_fill_time_t fill_time,
    const hssize_t offset[], hbool_t relax,
    unsigned *idx_hint/*in,out*/)
{
    int		idx=0;			/*hash index number	*/
    hsize_t	temp_idx=0;			/* temporary index number	*/
    hbool_t		found = FALSE;		/*already in cache?	*/
    H5F_rdcc_t		*rdcc = &(f->shared->rdcc);/*raw data chunk cache*/
    H5F_rdcc_ent_t	*ent = NULL;		/*cache entry		*/
    unsigned		u;			/*counters		*/
    H5F_istore_ud1_t	udata;			/*B-tree pass-through	*/
    size_t		chunk_size=0;		/*size of a chunk	*/
    hsize_t             tempchunk_size;
    herr_t		status;			/*func return status	*/
    void		*chunk=NULL;		/*the file chunk	*/
    void		*ret_value;	        /*return value		*/
    H5P_genplist_t      *plist;                 /* Property list */
    H5Z_EDC_t           edc;
    H5Z_cb_t            cb_struct;

    FUNC_ENTER_NOAPI_NOINIT(H5F_istore_lock);
    
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
    plist=H5I_object(dxpl_id);
    assert(plist!=NULL);
    HDmemset(&udata, 0, sizeof(H5F_istore_ud1_t));

    if (rdcc->nslots>0) {
        for (u=0, temp_idx=0; u<layout->ndims; u++) {
            temp_idx += offset[u];
            temp_idx *= layout->dim[u];
        }
        temp_idx += (hsize_t)(layout->addr);
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
        HDputc('w', stderr);
        HDfflush(stderr);
#endif
        rdcc->nhits++;
        for (u=0, tempchunk_size=1; u<layout->ndims; u++)
            tempchunk_size *= layout->dim[u];
        H5_ASSIGN_OVERFLOW(chunk_size,tempchunk_size,hsize_t,size_t);
        if (NULL==(chunk=H5MM_malloc (chunk_size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk");
        
    } else {

        /*
         * Not in the cache.  Read it from the file and count this as a miss
         * if it's in the file or an init if it isn't.
         */
        for (u=0, tempchunk_size=1; u<layout->ndims; u++) {
            udata.key.offset[u] = offset[u];
            tempchunk_size *= layout->dim[u];
        }
        H5_ASSIGN_OVERFLOW(chunk_size,tempchunk_size,hsize_t,size_t);
        udata.mesg = *layout;
        udata.addr = HADDR_UNDEF;
        status = H5B_find (f, dxpl_id, H5B_ISTORE, layout->addr, &udata);
        H5E_clear ();

        if (status>=0 && H5F_addr_defined(udata.addr)) {
            size_t		chunk_alloc=0;		/*allocated chunk size	*/

            /*
             * The chunk exists on disk.
             */
            /* Chunk size on disk isn't [likely] the same size as the final chunk
             * size in memory, so allocate memory big enough. */
            chunk_alloc = udata.key.nbytes;
            if (NULL==(chunk = H5MM_malloc (chunk_alloc)))
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk");
            if (H5F_block_read(f, H5FD_MEM_DRAW, udata.addr, udata.key.nbytes, dxpl_id, chunk)<0)
                HGOTO_ERROR (H5E_IO, H5E_READERROR, NULL, "unable to read raw data chunk");
            if(H5P_get(plist,H5D_XFER_EDC_NAME,&edc)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't get edc information");
            if(H5P_get(plist,H5D_XFER_FILTER_CB_NAME,&cb_struct)<0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't get filter callback struct");
            if (H5Z_pipeline(pline, H5Z_FLAG_REVERSE, &(udata.key.filter_mask), edc, 
                     cb_struct, &(udata.key.nbytes), &chunk_alloc, &chunk)<0) {
                HGOTO_ERROR(H5E_PLINE, H5E_READERROR, NULL, "data pipeline read failed");
            }
            rdcc->nmisses++;
        } else {
            H5D_fill_value_t	fill_status;

            /* Chunk size on disk isn't [likely] the same size as the final chunk
             * size in memory, so allocate memory big enough. */
            if (NULL==(chunk = H5MM_malloc (chunk_size)))
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk");

            if (H5P_is_fill_value_defined(fill, &fill_status) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't tell if fill value defined");

            if(fill_time==H5D_FILL_TIME_ALLOC ||
                    (fill_time==H5D_FILL_TIME_IFSET && fill_status==H5D_FILL_VALUE_USER_DEFINED)) {
                if (fill && fill->buf) {
                    /*
                     * The chunk doesn't exist in the file.  Replicate the fill
                     * value throughout the chunk.
                     */
                    assert(0==chunk_size % fill->size);
                    H5V_array_fill(chunk, fill->buf, fill->size, chunk_size/fill->size);
                } else {
                    /*
                     * The chunk doesn't exist in the file and no fill value was
                     * specified.  Assume all zeros.
                     */
                    HDmemset (chunk, 0, chunk_size);
                } /* end else */
            } /* end if */
            rdcc->ninits++;
        } /* end else */
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
            HDputc('#', stderr);
            HDfflush(stderr);
#endif
            if (H5F_istore_preempt(f, dxpl_id, ent, TRUE)<0)
                HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL, "unable to preempt chunk from cache");
        }
        if (H5F_istore_prune(f, dxpl_id, chunk_size)<0)
            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL, "unable to preempt chunk(s) from cache");

        /* Create a new entry */
        ent = H5FL_MALLOC(H5F_rdcc_ent_t);
        ent->locked = 0;
        ent->dirty = FALSE;
        ent->chunk_size = chunk_size;
        ent->alloc_size = chunk_size;
        ent->layout = H5O_copy(H5O_LAYOUT_ID, layout, NULL);
        ent->pline = H5O_copy(H5O_PLINE_ID, pline, NULL);
        for (u=0; u<layout->ndims; u++)
            ent->offset[u] = offset[u];
        ent->rd_count = chunk_size;
        ent->wr_count = chunk_size;
        ent->chunk = chunk;
        
        H5P_get(plist,H5D_XFER_BTREE_SPLIT_RATIO_NAME,&(ent->split_ratios));
        
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
        idx = UINT_MAX;

    } else if (found) {
        /*
         * The chunk is not at the beginning of the cache; move it backward
         * by one slot.  This is how we implement the LRU preemption
         * algorithm.
         */
        if (ent->next) {
            if (ent->next->next)
                ent->next->next->prev = ent;
            else
                rdcc->tail = ent;
            ent->next->prev = ent->prev;
            if (ent->prev)
                ent->prev->next = ent->next;
            else
                rdcc->head = ent->next;
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

    /* Set return value */
    ret_value = chunk;
    
done:
    if (!ret_value)
        if(chunk)
            H5MM_xfree (chunk);
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_unlock
 *
 * Purpose:	Unlocks a previously locked chunk. The LAYOUT, COMP, and
 *		OFFSET arguments should be the same as for H5F_rdcc_lock().
 *		The DIRTY argument should be set to non-zero if the chunk has
 *		been modified since it was locked. The IDX_HINT argument is
 *		the returned index hint from the lock operation and BUF is
 *		the return value from the lock.
 *
 *		The NACCESSED argument should be the number of bytes accessed
 *		for reading or writing (depending on the value of DIRTY).
 *		It's only purpose is to provide additional information to the
 *		preemption policy.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *		Robb Matzke, 1999-08-02
 *		The split_ratios are passed as part of the data transfer
 *		property list.
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_istore_unlock(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
		  const H5O_pline_t *pline, hbool_t dirty,
		  const hssize_t offset[], unsigned *idx_hint,
		  uint8_t *chunk, size_t naccessed)
{
    H5F_rdcc_t		*rdcc = &(f->shared->rdcc);
    H5F_rdcc_ent_t	*ent = NULL;
    int		found = -1;
    unsigned		u;
    H5P_genplist_t *plist;      /* Property list */
    
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_istore_unlock);

    if (UINT_MAX==*idx_hint) {
	/*not in cache*/
    } else {
	assert(*idx_hint<rdcc->nslots);
	assert(rdcc->slot[*idx_hint]);
	assert(rdcc->slot[*idx_hint]->chunk==chunk);
	found = *idx_hint;
    }
    
    if (found<0) {
        /*
         * It's not in the cache, probably because it's too big.  If it's
         * dirty then flush it to disk.  In any case, free the chunk.
         * Note: we have to copy the layout and filter messages so we
         *	 don't discard the `const' qualifier.
         */
        if (dirty) {
            H5F_rdcc_ent_t x;
            hsize_t tempchunk_size;

            HDmemset (&x, 0, sizeof x);
            x.dirty = TRUE;
            x.layout = H5O_copy (H5O_LAYOUT_ID, layout, NULL);
            x.pline = H5O_copy (H5O_PLINE_ID, pline, NULL);
            for (u=0, tempchunk_size=1; u<layout->ndims; u++) {
                x.offset[u] = offset[u];
                tempchunk_size *= layout->dim[u];
            }
            H5_ASSIGN_OVERFLOW(x.chunk_size,tempchunk_size,hsize_t,size_t);
            x.alloc_size = x.chunk_size;
            x.chunk = chunk;

            assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
            plist=H5I_object(dxpl_id);
            assert(plist!=NULL);
            H5P_get(plist,H5D_XFER_BTREE_SPLIT_RATIO_NAME,&(x.split_ratios));
            
            H5F_istore_flush_entry (f, dxpl_id, &x, TRUE);
        } else {
            if(chunk)
                H5MM_xfree (chunk);
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
    
    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_readvv
 *
 * Purpose:	Reads a multi-dimensional buffer from (part of) an indexed raw
 *		storage array.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, May  7, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5F_istore_readvv(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
    H5P_genplist_t *dc_plist, hssize_t chunk_coords[],
    size_t chunk_max_nseq, size_t *chunk_curr_seq, size_t chunk_len_arr[], hsize_t chunk_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[],
    void *buf)
{
    hsize_t		chunk_size;     /* Chunk size, in bytes */
    haddr_t	        chunk_addr;     /* Chunk address on disk */
    hssize_t	        chunk_coords_in_elmts[H5O_LAYOUT_NDIMS];
    H5O_pline_t         pline;          /* I/O pipeline information */
    size_t		u;              /* Local index variables */
    ssize_t             ret_value;      /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_readvv, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED==layout->type);
    assert(layout->ndims>0 && layout->ndims<=H5O_LAYOUT_NDIMS);
    assert(chunk_len_arr);
    assert(chunk_offset_arr);
    assert(mem_len_arr);
    assert(mem_offset_arr);
    assert(buf);

    /* Get necessary properties from property list */
    if(H5P_get(dc_plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get data pipeline");

    /* Compute chunk size */
    for (u=0, chunk_size=1; u<layout->ndims; u++)
        chunk_size *= layout->dim[u];

#ifndef NDEBUG
    for (u=0; u<layout->ndims; u++)
        assert(chunk_coords[u]>=0); /*negative coordinates not supported (yet) */
#endif
    
    for (u=0; u<layout->ndims; u++)
        chunk_coords_in_elmts[u] = chunk_coords[u] * (hssize_t)(layout->dim[u]);

    /* Get the address of this chunk on disk */
#ifdef QAK
HDfprintf(stderr,"%s: chunk_coords_in_elmts={",FUNC);
for(u=0; u<layout->ndims; u++)
    HDfprintf(stderr,"%Hd%s",chunk_coords_in_elmts[u],(u<(layout->ndims-1) ? ", " : "}\n"));
#endif /* QAK */
    chunk_addr=H5F_istore_get_addr(f, dxpl_id, layout, chunk_coords_in_elmts);
#ifdef QAK
HDfprintf(stderr,"%s: chunk_addr=%a, chunk_size=%Hu\n",FUNC,chunk_addr,chunk_size);
HDfprintf(stderr,"%s: chunk_len_arr[%Zu]=%Zu\n",FUNC,*chunk_curr_seq,chunk_len_arr[*chunk_curr_seq]);
HDfprintf(stderr,"%s: chunk_offset_arr[%Zu]=%Hu\n",FUNC,*chunk_curr_seq,chunk_offset_arr[*chunk_curr_seq]);
HDfprintf(stderr,"%s: mem_len_arr[%Zu]=%Zu\n",FUNC,*mem_curr_seq,mem_len_arr[*mem_curr_seq]);
HDfprintf(stderr,"%s: mem_offset_arr[%Zu]=%Hu\n",FUNC,*mem_curr_seq,mem_offset_arr[*mem_curr_seq]);
#endif /* QAK */

    /*
     * If the chunk is too large to load into the cache and it has no
     * filters in the pipeline (i.e. not compressed) and if the address
     * for the chunk has been defined, then don't load the chunk into the
     * cache, just write the data to it directly.
     */
    if (chunk_size>f->shared->rdcc_nbytes && pline.nfilters==0 &&
            chunk_addr!=HADDR_UNDEF) {
        if ((ret_value=H5F_contig_readvv(f, chunk_size, chunk_addr, chunk_max_nseq, chunk_curr_seq, chunk_len_arr, chunk_offset_arr, mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr, dxpl_id, buf))<0)
            HGOTO_ERROR (H5E_IO, H5E_READERROR, FAIL, "unable to read raw data to file");
    } /* end if */
    else {
        uint8_t         *chunk;         /* Pointer to cached chunk in memory */
        H5O_fill_t      fill;           /* Fill value information */
        H5D_fill_time_t fill_time;      /* Fill time information */
        unsigned        idx_hint=0;     /* Cache index hint      */
        ssize_t         naccessed;      /* Number of bytes accessed in chunk */

        /* Get necessary properties from property list */
        if(H5P_get(dc_plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill value");
        if(H5P_get(dc_plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill time");

        /*
         * Lock the chunk, copy from application to chunk, then unlock the
         * chunk.
         */
        if (NULL==(chunk=H5F_istore_lock(f, dxpl_id, layout, &pline, &fill, fill_time,
                     chunk_coords_in_elmts, FALSE, &idx_hint)))
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk");

        /* Use the vectorized memory copy routine to do actual work */
        if((naccessed=H5V_memcpyvv(buf,mem_max_nseq,mem_curr_seq,mem_len_arr,mem_offset_arr,chunk,chunk_max_nseq,chunk_curr_seq,chunk_len_arr,chunk_offset_arr))<0)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "vectorized memcpy failed");

        H5_CHECK_OVERFLOW(naccessed,ssize_t,size_t);
        if (H5F_istore_unlock(f, dxpl_id, layout, &pline, FALSE,
                  chunk_coords_in_elmts, &idx_hint, chunk, (size_t)naccessed)<0)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk");

        /* Set return value */
        ret_value=naccessed;
    } /* end else */
        
done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5F_istore_readvv() */


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_writevv
 *
 * Purpose:	Writes a multi-dimensional buffer to (part of) an indexed raw
 *		storage array.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Friday, May  2, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5F_istore_writevv(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
    H5P_genplist_t *dc_plist, hssize_t chunk_coords[],
    size_t chunk_max_nseq, size_t *chunk_curr_seq, size_t chunk_len_arr[], hsize_t chunk_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[],
    const void *buf)
{
    hsize_t		chunk_size;     /* Chunk size, in bytes */
    haddr_t	        chunk_addr;     /* Chunk address on disk */
    hssize_t	        chunk_coords_in_elmts[H5O_LAYOUT_NDIMS];
    H5O_pline_t         pline;          /* I/O pipeline information */
    size_t		u;              /* Local index variables */
    ssize_t             ret_value;      /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_writevv, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED==layout->type);
    assert(layout->ndims>0 && layout->ndims<=H5O_LAYOUT_NDIMS);
    assert(chunk_len_arr);
    assert(chunk_offset_arr);
    assert(mem_len_arr);
    assert(mem_offset_arr);
    assert(buf);

    /* Get necessary properties from property list */
    if(H5P_get(dc_plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get data pipeline");

    /* Compute chunk size */
    for (u=0, chunk_size=1; u<layout->ndims; u++)
        chunk_size *= layout->dim[u];

#ifndef NDEBUG
    for (u=0; u<layout->ndims; u++)
        assert(chunk_coords[u]>=0); /*negative coordinates not supported (yet) */
#endif
    
    for (u=0; u<layout->ndims; u++)
        chunk_coords_in_elmts[u] = chunk_coords[u] * (hssize_t)(layout->dim[u]);

    /* Get the address of this chunk on disk */
#ifdef QAK
HDfprintf(stderr,"%s: chunk_coords_in_elmts={",FUNC);
for(u=0; u<layout->ndims; u++)
    HDfprintf(stderr,"%Hd%s",chunk_coords_in_elmts[u],(u<(layout->ndims-1) ? ", " : "}\n"));
#endif /* QAK */
    chunk_addr=H5F_istore_get_addr(f, dxpl_id, layout, chunk_coords_in_elmts);
#ifdef QAK
HDfprintf(stderr,"%s: chunk_addr=%a, chunk_size=%Hu\n",FUNC,chunk_addr,chunk_size);
HDfprintf(stderr,"%s: chunk_len_arr[%Zu]=%Zu\n",FUNC,*chunk_curr_seq,chunk_len_arr[*chunk_curr_seq]);
HDfprintf(stderr,"%s: chunk_offset_arr[%Zu]=%Hu\n",FUNC,*chunk_curr_seq,chunk_offset_arr[*chunk_curr_seq]);
HDfprintf(stderr,"%s: mem_len_arr[%Zu]=%Zu\n",FUNC,*mem_curr_seq,mem_len_arr[*mem_curr_seq]);
HDfprintf(stderr,"%s: mem_offset_arr[%Zu]=%Hu\n",FUNC,*mem_curr_seq,mem_offset_arr[*mem_curr_seq]);
#endif /* QAK */

    /*
     * If the chunk is too large to load into the cache and it has no
     * filters in the pipeline (i.e. not compressed) and if the address
     * for the chunk has been defined, then don't load the chunk into the
     * cache, just write the data to it directly.
     *
     * If MPIO, MPIPOSIX, or FPHDF5 is used, must bypass the
     * chunk-cache scheme because other MPI processes could be
     * writing to other elements in the same chunk.  Do a direct
     * write-through of only the elements requested.
     */
    if ((chunk_size>f->shared->rdcc_nbytes && pline.nfilters==0 &&
            chunk_addr!=HADDR_UNDEF)
        || ((IS_H5FD_MPIO(f) ||IS_H5FD_MPIPOSIX(f) || IS_H5FD_FPHDF5(f)) &&
            (H5F_ACC_RDWR & f->shared->flags))) {
#ifdef H5_HAVE_PARALLEL
        /* Additional sanity check when operating in parallel */
        if (chunk_addr==HADDR_UNDEF || pline.nfilters>0)
            HGOTO_ERROR (H5E_IO, H5E_WRITEERROR, FAIL, "unable to locate raw data chunk");
#endif /* H5_HAVE_PARALLEL */
        if ((ret_value=H5F_contig_writevv(f, chunk_size, chunk_addr, chunk_max_nseq, chunk_curr_seq, chunk_len_arr, chunk_offset_arr, mem_max_nseq, mem_curr_seq, mem_len_arr, mem_offset_arr, dxpl_id, buf))<0)
            HGOTO_ERROR (H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file");
    } /* end if */
    else {
        uint8_t         *chunk;         /* Pointer to cached chunk in memory */
        H5O_fill_t      fill;           /* Fill value information */
        H5D_fill_time_t fill_time;      /* Fill time information */
        unsigned        idx_hint=0;     /* Cache index hint      */
        ssize_t         naccessed;      /* Number of bytes accessed in chunk */
        hbool_t         relax;          /* Whether whole chunk is selected */

        /* Get necessary properties from property list */
        if(H5P_get(dc_plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill value");
        if(H5P_get(dc_plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill time");

        /*
         * Lock the chunk, copy from application to chunk, then unlock the
         * chunk.
         */
        if(chunk_max_nseq==1 && chunk_len_arr[0] == chunk_size)
            relax = TRUE;
        else
            relax = FALSE;

        if (NULL==(chunk=H5F_istore_lock(f, dxpl_id, layout, &pline, &fill, fill_time,
                 chunk_coords_in_elmts, relax, &idx_hint)))
            HGOTO_ERROR (H5E_IO, H5E_WRITEERROR, FAIL, "unable to read raw data chunk");

        /* Use the vectorized memory copy routine to do actual work */
        if((naccessed=H5V_memcpyvv(chunk,chunk_max_nseq,chunk_curr_seq,chunk_len_arr,chunk_offset_arr,buf,mem_max_nseq,mem_curr_seq,mem_len_arr,mem_offset_arr))<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "vectorized memcpy failed");

        H5_CHECK_OVERFLOW(naccessed,ssize_t,size_t);
        if (H5F_istore_unlock(f, dxpl_id, layout, &pline, TRUE,
                  chunk_coords_in_elmts, &idx_hint, chunk, (size_t)naccessed)<0)
            HGOTO_ERROR (H5E_IO, H5E_WRITEERROR, FAIL, "uanble to unlock raw data chunk");

        /* Set return value */
        ret_value=naccessed;
    } /* end else */
        
done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5F_istore_writevv() */


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_create
 *
 * Purpose:	Creates a new indexed-storage B-tree and initializes the
 *		istore struct with information about the storage.  The
 *		struct should be immediately written to the object header.
 *
 *		This function must be called before passing ISTORE to any of
 *		the other indexed storage functions!
 *
 * Return:	Non-negative on success (with the ISTORE argument initialized
 *		and ready to write to an object header). Negative on failure.
 *
 * Programmer:	Robb Matzke
 *		Tuesday, October 21, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_create(H5F_t *f, hid_t dxpl_id, H5O_layout_t *layout /*out */ )
{
    H5F_istore_ud1_t	udata;
#ifndef NDEBUG
    unsigned			u;
#endif
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_create, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED == layout->type);
    assert(layout->ndims > 0 && layout->ndims <= H5O_LAYOUT_NDIMS);
#ifndef NDEBUG
    for (u = 0; u < layout->ndims; u++)
	assert(layout->dim[u] > 0);
#endif

    udata.mesg.ndims = layout->ndims;
    if (H5B_create(f, dxpl_id, H5B_ISTORE, &udata, &(layout->addr)/*out*/) < 0)
	HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "can't create B-tree");
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_allocated
 *
 * Purpose:	Return the number of bytes allocated in the file for storage
 *		of raw data under the specified B-tree (ADDR is the address
 *		of the B-tree).
 *
 * Return:	Success:	Number of bytes stored in all chunks.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
hsize_t
H5F_istore_allocated(H5F_t *f, hid_t dxpl_id, unsigned ndims, haddr_t addr)
{
    H5F_istore_ud1_t	udata;
    hsize_t      ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_nchunks, 0);

    HDmemset(&udata, 0, sizeof udata);
    udata.mesg.ndims = ndims;
    if (H5B_iterate(f, dxpl_id, H5B_ISTORE, H5F_istore_iter_allocated, addr, &udata)<0)
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, 0, "unable to iterate over chunk B-tree");

    /* Set return value */
    ret_value=udata.total_storage;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_get_addr
 *
 * Purpose:	Get the file address of a chunk if file space has been
 *		assigned.  Save the retrieved information in the udata
 *		supplied.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Albert Cheng
 *              June 27, 1998
 *
 * Modifications:
 *              Modified to return the address instead of returning it through
 *              a parameter - QAK, 1/30/02
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5F_istore_get_addr(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
		    const hssize_t offset[])
{
    H5F_istore_ud1_t	udata;                  /* Information about a chunk */
    unsigned	u;
    haddr_t	ret_value;		/* Return value */
    
    FUNC_ENTER_NOAPI_NOINIT(H5F_istore_get_addr);

    assert(f);
    assert(layout && (layout->ndims > 0));
    assert(offset);

    /* Initialize the information about the chunk we are looking for */
    for (u=0; u<layout->ndims; u++)
	udata.key.offset[u] = offset[u];
    udata.mesg = *layout;
    udata.addr = HADDR_UNDEF;

    /* Go get the chunk information */
    if (H5B_find (f, dxpl_id, H5B_ISTORE, layout->addr, &udata)<0) {
        H5E_clear();
	HGOTO_ERROR(H5E_BTREE,H5E_NOTFOUND,HADDR_UNDEF,"Can't locate chunk info");
    } /* end if */

    /* Success!  Set the return value */
    ret_value=udata.addr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5F_istore_get_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_allocate
 *
 * Purpose:	Allocate file space for all chunks that are not allocated yet.
 *		Return SUCCEED if all needed allocation succeed, otherwise
 *		FAIL.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Note:	Current implementation relies on cache_size being 0,
 *		thus no chunk is cashed and written to disk immediately
 *		when a chunk is unlocked (via H5F_istore_unlock)
 *		This should be changed to do a direct flush independent
 *		of the cache value.
 *
 * Programmer:	Albert Cheng
 *		June 26, 1998
 *
 * Modifications:
 *		rky, 1998-09-23
 *		Added barrier to preclude racing with data writes.
 *
 *		rky, 1998-12-07
 *		Added Wait-Signal wrapper around unlock-lock critical region
 *		to prevent race condition (unlock reads, lock writes the
 *		chunk).
 *
 * 		Robb Matzke, 1999-08-02
 *		The split_ratios are passed in as part of the data transfer
 *		property list.
 *
 * 		Quincey Koziol, 2002-05-16
 *		Rewrote algorithm to allocate & write blocks without using
 *              lock/unlock code.
 *
 * 		Quincey Koziol, 2002-05-17
 *		Added feature to avoid writing fill-values if user has indicated
 *              that they should never be written.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_allocate(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
        const hsize_t *space_dim, H5P_genplist_t *dc_plist, hbool_t full_overwrite)
{
    hssize_t	chunk_offset[H5O_LAYOUT_NDIMS]; /* Offset of current chunk */
    hsize_t	chunk_size;     /* Size of chunk in bytes */
    H5O_pline_t pline;          /* I/O pipeline information */
    H5O_fill_t fill;            /* Fill value information */
    H5D_fill_time_t fill_time;  /* When to write fill values */
    H5D_fill_value_t fill_status;    /* The fill value status */
    unsigned   should_fill=0;   /* Whether fill values should be written */
    H5F_istore_ud1_t udata;	/* B-tree pass-through for creating chunk */
    void *chunk=NULL;           /* Chunk buffer for writing fill values */
    H5P_genplist_t *dx_plist;   /* Data xfer property list */
    double	split_ratios[3];/* B-tree node splitting ratios		*/
#ifdef H5_HAVE_PARALLEL
    MPI_Comm	mpi_comm=MPI_COMM_NULL;	/* MPI communicator for file */
    int         mpi_rank=(-1);  /* This process's rank  */
    int         mpi_size=(-1);  /* Total # of processes */
    int         mpi_code;       /* MPI return code */
    unsigned    blocks_written=0; /* Flag to indicate that chunk was actually written */
    unsigned    using_mpi=0;    /* Flag to indicate that the file is being accessed with an MPI-capable file driver */
#endif /* H5_HAVE_PARALLEL */
    int		carry;          /* Flag to indicate that chunk increment carrys to higher dimension (sorta) */
    unsigned	chunk_exists;   /* Flag to indicate whether a chunk exists already */
    int		i;              /* Local index variable */
    unsigned	u;              /* Local index variable */
    H5Z_EDC_t   edc;            /* Decide whether to enable EDC for read */
    H5Z_cb_t    cb_struct;
    herr_t	ret_value=SUCCEED;	/* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_allocate, FAIL);

    /* Check args */
    assert(f);
    assert(space_dim);
    assert(layout && H5D_CHUNKED==layout->type);
    assert(layout->ndims>0 && layout->ndims<=H5O_LAYOUT_NDIMS);
    assert(H5F_addr_defined(layout->addr));
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));
    assert(dc_plist!=NULL);

    /* Get necessary properties from dataset creation property list */
    if(H5P_get(dc_plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "can't get fill value");
    if(H5P_get(dc_plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "can't get data pipeline");
    if(H5P_get(dc_plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve fill time");

    /* Get necessary properties from dataset transfer property list */
    if (NULL == (dx_plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list");
    if(H5P_get(dx_plist,H5D_XFER_BTREE_SPLIT_RATIO_NAME,split_ratios)<0)
        HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "can't get B-tree split ratios");
    if(H5P_get(dx_plist,H5D_XFER_EDC_NAME,&edc)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get edc information");
    if(H5P_get(dx_plist,H5D_XFER_FILTER_CB_NAME,&cb_struct)<0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get filter callback struct");

#ifdef H5_HAVE_PARALLEL
    /* Retrieve up MPI parameters */
    if(IS_H5FD_MPIO(f)) {
        /* Get the MPI communicator */
        if (MPI_COMM_NULL == (mpi_comm=H5FD_mpio_communicator(f->shared->lf)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI communicator");

        /* Get the MPI rank & size */
        if ((mpi_rank=H5FD_mpio_mpi_rank(f->shared->lf))<0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI rank");
        if ((mpi_size=H5FD_mpio_mpi_size(f->shared->lf))<0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI size");

        /* Set the MPI-capable file driver flag */
        using_mpi=1;
    } /* end if */
    else if(IS_H5FD_MPIPOSIX(f)) {
        /* Get the MPI communicator */
        if (MPI_COMM_NULL == (mpi_comm=H5FD_mpiposix_communicator(f->shared->lf)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI communicator");

        /* Get the MPI rank & size */
        if ((mpi_rank=H5FD_mpiposix_mpi_rank(f->shared->lf))<0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI rank");
        if ((mpi_size=H5FD_mpiposix_mpi_size(f->shared->lf))<0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI size");

        /* Set the MPI-capable file driver flag */
        using_mpi=1;
    } /* end else */
#ifdef H5_HAVE_FPHDF5
    else if (IS_H5FD_FPHDF5(f)) {
        /* Get the FPHDF5 barrier communicator */
        if (MPI_COMM_NULL == (mpi_comm = H5FD_fphdf5_barrier_communicator(f->shared->lf)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI communicator");

        /* Get the MPI rank & size */
        if ((mpi_rank = H5FD_fphdf5_mpi_rank(f->shared->lf)) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI rank");

        if ((mpi_size = H5FD_fphdf5_mpi_size(f->shared->lf)) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI size");

        /* Set the MPI-capable file driver flag */
        using_mpi = 1;
    } /* end if */
#endif  /* H5_HAVE_FPHDF5 */
#endif  /* H5_HAVE_PARALLEL */

    /*
     * Setup indice to go through all chunks. (Future improvement
     * should allocate only chunks that have no file space assigned yet.
     */
    for (u=0, chunk_size=1; u<layout->ndims; u++) {
        chunk_offset[u] = 0;
        chunk_size *= layout->dim[u];
    } /* end for */

    /* Check the dataset's fill-value status */
    if (H5P_is_fill_value_defined(&fill, &fill_status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined");

    /* If we are filling the dataset on allocation or "if set" and
     * the fill value _is_ set, _and_ we are not overwriting the new blocks,
     * set the "should fill" flag
     */
    if(!full_overwrite && (fill_time==H5D_FILL_TIME_ALLOC ||
            (fill_time==H5D_FILL_TIME_IFSET && fill_status==H5D_FILL_VALUE_USER_DEFINED)))
        should_fill=1;

    /* Check if fill values should be written to blocks */
    if(should_fill) {
        /* Allocate chunk buffer for processes to use when writing fill values */
        H5_CHECK_OVERFLOW(chunk_size,hsize_t,size_t);
        if (NULL==(chunk = H5MM_malloc((size_t)chunk_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for chunk");

        /* Fill the chunk with the proper values */
        if(fill.buf) {
            /*
             * Replicate the fill value throughout the chunk.
             */
            assert(0==chunk_size % fill.size);
            H5V_array_fill(chunk, fill.buf, fill.size, (size_t)chunk_size/fill.size);
        } else {
            /*
             * No fill value was specified, assume all zeros.
             */
            HDmemset (chunk, 0, (size_t)chunk_size);
        } /* end else */

        /* Check if there are filters which need to be applied to the chunk */
        if (pline.nfilters>0) {
            unsigned filter_mask=0;
            size_t buf_size=(size_t)chunk_size;
            size_t nbytes=(size_t)chunk_size;

            /* Push the chunk through the filters */
            if (H5Z_pipeline(&pline, 0, &filter_mask, edc, cb_struct, &nbytes, &buf_size, &chunk)<0)
                HGOTO_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL, "output pipeline failed");

            /* Keep the number of bytes the chunk turned in to */
            chunk_size=nbytes;
        } /* end if */
    } /* end if */

    /* Loop over all chunks */
    carry=0;
    while (carry==0) {
        /* Check if the chunk exists yet on disk */
        chunk_exists=1;
        if(H5F_istore_get_addr(f,dxpl_id,layout,chunk_offset)==HADDR_UNDEF) {
            H5F_rdcc_t             *rdcc = &(f->shared->rdcc);	/*raw data chunk cache */
            H5F_rdcc_ent_t         *ent = NULL;              	/*cache entry  */

            /* Didn't find the chunk on disk */
            chunk_exists = 0;

            /* Look for chunk in cache */
            for(ent = rdcc->head; ent && !chunk_exists; ent = ent->next) {
                /* Make certain we are dealing with the correct B-tree, etc */
                if (layout->ndims==ent->layout->ndims &&
                        H5F_addr_eq(layout->addr, ent->layout->addr)) {

                    /* Assume a match */
                    chunk_exists = 1;
                    for(u = 0; u < layout->ndims && chunk_exists; u++) {
                        if(ent->offset[u] != chunk_offset[u])
                            chunk_exists = 0;       /* Reset if no match */
                    } /* end for */
                } /* end if */
            } /* end for */
        } /* end if */

        if(!chunk_exists) {
            /* Initialize the chunk information */
            udata.mesg = *layout;
            udata.key.filter_mask = 0;
            udata.addr = HADDR_UNDEF;
            H5_CHECK_OVERFLOW(chunk_size,hsize_t,size_t);
            udata.key.nbytes = (size_t)chunk_size;
            for (u=0; u<layout->ndims; u++)
                udata.key.offset[u] = chunk_offset[u];

            /* Allocate the chunk with all processes */
            if (H5B_insert(f, dxpl_id, H5B_ISTORE, layout->addr, split_ratios, &udata)<0)
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to allocate chunk");

            /* Check if fill values should be written to blocks */
            if(should_fill) {
#ifdef H5_HAVE_PARALLEL
                /* Check if this file is accessed with an MPI-capable file driver */
                if(using_mpi) {
                    /* Write the chunks out from only one process */
                    /* !! Use the internal "independent" DXPL!! -QAK */
                    if(H5_PAR_META_WRITE==mpi_rank) {
                        if (H5F_block_write(f, H5FD_MEM_DRAW, udata.addr, udata.key.nbytes, H5AC_ind_dxpl_id, chunk)<0)
                            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file");
                    } /* end if */

                    /* Indicate that blocks are being written */
                    blocks_written=1;
                } /* end if */
                else {
#endif /* H5_HAVE_PARALLEL */
                    if (H5F_block_write(f, H5FD_MEM_DRAW, udata.addr, udata.key.nbytes, dxpl_id, chunk)<0)
                        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file");
#ifdef H5_HAVE_PARALLEL
                } /* end else */
#endif /* H5_HAVE_PARALLEL */
            } /* end if */
        } /* end if */
	
        /* Increment indices */
        for (i=layout->ndims-1, carry=1; i>=0 && carry; --i) {
            chunk_offset[i] += layout->dim[i];
            if (chunk_offset[i] >= (hssize_t)(space_dim[i]))
                chunk_offset[i] = 0;
            else
                carry = 0;
        } /* end for */
    } /* end while */

#ifdef H5_HAVE_PARALLEL
    /* Only need to block at the barrier if we actually allocated a chunk */
    /* And if we are using an MPI-capable file driver */
    if(using_mpi && blocks_written) {
        /* Wait at barrier to avoid race conditions where some processes are
         * still writing out chunks and other processes race ahead to read
         * them in, getting bogus data.
         */
        if (MPI_SUCCESS != (mpi_code=MPI_Barrier(mpi_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code);
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:
    /* Free the chunk for fill values */
    if(chunk!=NULL)
        H5MM_xfree(chunk);

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function: H5F_istore_prune_by_extent
 *
 * Purpose: This function searches for chunks that are no longer necessary both in the
 *  raw data cache and in the B-tree. 
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 * Algorithm: Robb Matzke
 *
 * Date: March 27, 2002
 *
 * The algorithm is:
 *
 *  For chunks that are no longer necessary:
 *
 *  1. Search in the raw data cache for each chunk 
 *  2. If found then preempt it from the cache
 *  3. Search in the B-tree for each chunk 
 *  4. If found then remove it from the B-tree and deallocate file storage for the chunk
 *
 * This example shows a 2d dataset of 90x90 with a chunk size of 20x20. 
 *
 *
 *     0         20        40        60        80    90   100
 *    0 +---------+---------+---------+---------+-----+...+
 *      |:::::X::::::::::::::         :         :     |   :
 *      |:::::::X::::::::::::         :         :     |   :   Key
 *      |::::::::::X:::::::::         :         :     |   :   --------
 *      |::::::::::::X:::::::         :         :     |   :  +-+ Dataset
 *    20+::::::::::::::::::::.........:.........:.....+...:  | | Extent
 *      |         :::::X:::::         :         :     |   :  +-+
 *      |         :::::::::::         :         :     |   :
 *      |         :::::::::::         :         :     |   :  ... Chunk
 *      |         :::::::X:::         :         :     |   :  : : Boundary
 *    40+.........:::::::::::.........:.........:.....+...:  :.:
 *      |         :         :         :         :     |   :
 *      |         :         :         :         :     |   :  ... Allocated
 *      |         :         :         :         :     |   :  ::: & Filled
 *      |         :         :         :         :     |   :  ::: Chunk
 *    60+.........:.........:.........:.........:.....+...:
 *      |         :         :::::::X:::         :     |   :   X  Element
 *      |         :         :::::::::::         :     |   :      Written
 *      |         :         :::::::::::         :     |   :
 *      |         :         :::::::::::         :     |   :
 *    80+.........:.........:::::::::::.........:.....+...:   O  Fill Val
 *      |         :         :         :::::::::::     |   :      Explicitly
 *      |         :         :         ::::::X::::     |   :      Written    
 *    90+---------+---------+---------+---------+-----+   :
 *      :         :         :         :::::::::::         :
 *   100:.........:.........:.........:::::::::::.........:
 *
 *
 * We have 25 total chunks for this dataset, 5 of which have space
 * allocated in the file because they were written to one or more
 * elements. These five chunks (and only these five) also have entries in
 * the storage B-tree for this dataset.
 *
 * Now lets say we want to shrink the dataset down to 70x70:
 *
 *
 *      0         20        40        60   70   80    90   100
 *    0 +---------+---------+---------+----+----+-----+...+
 *      |:::::X::::::::::::::         :    |    :     |   :
 *      |:::::::X::::::::::::         :    |    :     |   :    Key           
 *      |::::::::::X:::::::::         :    |    :     |   :    --------      
 *      |::::::::::::X:::::::         :    |    :     |   :   +-+ Dataset    
 *    20+::::::::::::::::::::.........:....+....:.....|...:   | | Extent     
 *      |         :::::X:::::         :    |    :     |   :   +-+            
 *      |         :::::::::::         :    |    :     |   :                  
 *      |         :::::::::::         :    |    :     |   :   ... Chunk      
 *      |         :::::::X:::         :    |    :     |   :   : : Boundary   
 *    40+.........:::::::::::.........:....+....:.....|...:   :.:            
 *      |         :         :         :    |    :     |   :                  
 *      |         :         :         :    |    :     |   :   ... Allocated  
 *      |         :         :         :    |    :     |   :   ::: & Filled   
 *      |         :         :         :    |    :     |   :   ::: Chunk      
 *    60+.........:.........:.........:....+....:.....|...:                  
 *      |         :         :::::::X:::    |    :     |   :    X  Element    
 *      |         :         :::::::::::    |    :     |   :       Written    
 *      +---------+---------+---------+----+    :     |   :                  
 *      |         :         :::::::::::         :     |   :                  
 *    80+.........:.........:::::::::X:.........:.....|...:    O  Fill Val   
 *      |         :         :         :::::::::::     |   :       Explicitly 
 *      |         :         :         ::::::X::::     |   :       Written    
 *    90+---------+---------+---------+---------+-----+   :
 *      :         :         :         :::::::::::         :
 *   100:.........:.........:.........:::::::::::.........:
 *
 *
 * That means that the nine chunks along the bottom and right side should
 * no longer exist. Of those nine chunks, (0,80), (20,80), (40,80),
 * (60,80), (80,80), (80,60), (80,40), (80,20), and (80,0), one is actually allocated 
 * that needs to be released.
 * To release the chunks, we traverse the B-tree to obtain a list of unused
 * allocated chunks, and then call H5B_remove() for each chunk.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_prune_by_extent(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout, const H5S_t * space)
{
    H5F_rdcc_t             *rdcc = &(f->shared->rdcc);	/*raw data chunk cache */
    H5F_rdcc_ent_t         *ent = NULL, *next = NULL;	/*cache entry  */
    unsigned                u;	/*counters  */
    int                     found = 0;	/*remove this entry  */
    H5F_istore_ud1_t        udata;	/*B-tree pass-through */
    hsize_t                 curr_dims[H5O_LAYOUT_NDIMS];	/*current dataspace dimensions */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_prune_by_extent, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED == layout->type);
    assert(layout->ndims > 0 && layout->ndims <= H5O_LAYOUT_NDIMS);
    assert(H5F_addr_defined(layout->addr));
    assert(space);

    /* Go get the rank & dimensions */
    if(H5S_get_simple_extent_dims(space, curr_dims, NULL) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataset dimensions");

 /*-------------------------------------------------------------------------
  * Figure out what chunks are no longer in use for the specified extent 
  * and release them from the linked list raw data cache
  *-------------------------------------------------------------------------
  */
    for(ent = rdcc->head; ent; ent = next) {
	next = ent->next;

        /* Make certain we are dealing with the correct B-tree, etc */
        if (layout->ndims==ent->layout->ndims &&
                H5F_addr_eq(layout->addr, ent->layout->addr)) {
            found = 0;
            for(u = 0; u < ent->layout->ndims - 1; u++) {
                if((hsize_t)ent->offset[u] > curr_dims[u]) {
                    found = 1;
                    break;
                }
            }
        } /* end if */

	if(found) {
#if defined (H5F_ISTORE_DEBUG)
	    HDfputs("cache:remove:[", stdout);
	    for(u = 0; u < ent->layout->ndims - 1; u++) {
		HDfprintf(stdout, "%s%Hd", u ? ", " : "", ent->offset[u]);
	    }
	    HDfputs("]\n", stdout);
#endif

	    /* Preempt the entry from the cache, but do not flush it to disk */
	    if(H5F_istore_preempt(f, dxpl_id, ent, FALSE) < 0)
		HGOTO_ERROR(H5E_IO, H5E_CANTINIT, 0, "unable to preempt chunk");
	}
    }

/*-------------------------------------------------------------------------
 * Check if there are any chunks on the B-tree
 *-------------------------------------------------------------------------
 */

    HDmemset(&udata, 0, sizeof udata);
    udata.stream = stdout;
    udata.mesg.addr = layout->addr;
    udata.mesg.ndims = layout->ndims;
    for(u = 0; u < udata.mesg.ndims; u++)
	udata.mesg.dim[u] = layout->dim[u];
    udata.dims = curr_dims;

    if(H5B_iterate(f, dxpl_id, H5B_ISTORE, H5F_istore_prune_extent, layout->addr, &udata) < 0)
	HGOTO_ERROR(H5E_IO, H5E_CANTINIT, 0, "unable to iterate over B-tree");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function: H5F_istore_prune_extent
 *
 * Purpose: Search for chunks that are no longer necessary in the B-tree. 
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: March 26, 2002
 *
 * Comments: Called by H5B_prune_by_extent, part of H5B_ISTORE
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5F_istore_prune_extent(H5F_t *f, hid_t dxpl_id, void *_lt_key, haddr_t UNUSED addr,
        void UNUSED *_rt_key, void *_udata)
{
    H5F_istore_ud1_t       *bt_udata = (H5F_istore_ud1_t *)_udata;
    H5F_istore_key_t       *lt_key = (H5F_istore_key_t *)_lt_key;
    unsigned                u;
    H5F_istore_ud1_t        udata;
    int                     ret_value=H5B_ITER_CONT;       /* Return value */

    /* The LT_KEY is the left key (the one that describes the chunk). It points to a chunk of 
     * storage that contains the beginning of the logical address space represented by UDATA.
     */

    FUNC_ENTER_NOAPI_NOINIT(H5F_istore_prune_extent);

    /* Figure out what chunks are no longer in use for the specified extent and release them */
    for(u = 0; u < bt_udata->mesg.ndims - 1; u++)
	if((hsize_t)lt_key->offset[u] > bt_udata->dims[u]) {
#if defined (H5F_ISTORE_DEBUG)
            HDfputs("b-tree:remove:[", bt_udata->stream);
            for(u = 0; u < bt_udata->mesg.ndims - 1; u++) {
                HDfprintf(bt_udata->stream, "%s%Hd", u ? ", " : "",
                        lt_key->offset[u]);
            }
            HDfputs("]\n", bt_udata->stream);
#endif

            HDmemset(&udata, 0, sizeof udata);
            udata.key = *lt_key;
            udata.mesg = bt_udata->mesg;

            /* Remove */
            if(H5B_remove(f, dxpl_id, H5B_ISTORE, bt_udata->mesg.addr, &udata) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, H5B_ITER_ERROR, "unable to remove entry");
	    break;
	} /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function: H5F_istore_remove
 *
 * Purpose: Removes chunks that are no longer necessary in the B-tree. 
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Robb Matzke
 *             Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: March 28, 2002
 *
 * Comments: Part of H5B_ISTORE
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5B_ins_t
H5F_istore_remove(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_lt_key /*in,out */ ,
	hbool_t *lt_key_changed /*out */ ,
	void UNUSED * _udata /*in,out */ ,
	void UNUSED * _rt_key /*in,out */ ,
	hbool_t *rt_key_changed /*out */ )
{
    H5F_istore_key_t    *lt_key = (H5F_istore_key_t *)_lt_key;
    H5B_ins_t ret_value=H5B_INS_REMOVE; /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_remove,H5B_INS_ERROR);

    /* Check for overlap with the sieve buffer and reset it */
    if (H5F_sieve_overlap_clear(f, dxpl_id, addr, (hsize_t)lt_key->nbytes)<0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, H5B_INS_ERROR, "unable to clear sieve buffer");

    /* Remove raw data chunk from file */
    H5MF_xfree(f, H5FD_MEM_DRAW, dxpl_id, addr, (hsize_t)lt_key->nbytes);

    /* Mark keys as unchanged */
    *lt_key_changed = FALSE;
    *rt_key_changed = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function: H5F_istore_initialize_by_extent
 *
 * Purpose:  This function searches for chunks that have to be initialized with the fill
 *   value both in the raw data cache and in the B-tree. 
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: April 4, 2002
 *
 * Comments: 
 *
 * (See the example of H5F_istore_prune_by_extent)
 * Next, there are seven chunks where the database extent boundary is
 * within the chunk. We find those seven just like we did with the previous nine.
 * Fot the ones that are allocated we initialize the part that lies outside the boundary 
 * with the fill value.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_initialize_by_extent(H5F_t *f, hid_t dxpl_id, const H5O_layout_t *layout,
        H5P_genplist_t *dc_plist, const H5S_t * space)
{
    uint8_t                *chunk = NULL;	/*the file chunk  */
    unsigned                idx_hint = 0;	/*input value for H5F_istore_lock */
    hssize_t                chunk_offset[H5O_LAYOUT_NDIMS];	/*logical location of the chunks */
    hsize_t                 idx_cur[H5O_LAYOUT_NDIMS];	/*multi-dimensional counters */
    hsize_t                 idx_min[H5O_LAYOUT_NDIMS];
    hsize_t                 idx_max[H5O_LAYOUT_NDIMS];
    hsize_t                 sub_size[H5O_LAYOUT_NDIMS];
    hsize_t                 naccessed;	/*bytes accessed in chunk */
    hsize_t                 end_chunk;	/*chunk position counter */
    hssize_t                start[H5O_LAYOUT_NDIMS];	/*starting location of hyperslab */
    hsize_t                 count[H5O_LAYOUT_NDIMS];	/*element count of hyperslab */
    hsize_t                 size[H5O_LAYOUT_NDIMS];	/*current size of dimensions */
    H5S_t                  *space_chunk = NULL;	/*dataspace for a chunk */
    hsize_t                 curr_dims[H5O_LAYOUT_NDIMS];	/*current dataspace dimensions */
    int                     srank;	/*current # of dimensions (signed) */
    unsigned                rank;	/*current # of dimensions */
    int                     i, carry;	/*counters  */
    unsigned                u;
    int                     found = 0;	/*initialize this entry  */
    H5O_pline_t             pline;      /* I/O pipeline information */
    H5O_fill_t              fill;       /* Fill value information */
    H5D_fill_time_t         fill_time;  /* Fill time information */
    herr_t	            ret_value=SUCCEED;	/* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_initialize_by_extent, FAIL);

    /* Check args */
    assert(f);
    assert(layout && H5D_CHUNKED == layout->type);
    assert(layout->ndims > 0 && layout->ndims <= H5O_LAYOUT_NDIMS);
    assert(H5F_addr_defined(layout->addr));
    assert(space);

    /* Get necessary properties from property list */
    if(H5P_get(dc_plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill value");
    if(H5P_get(dc_plist, H5D_CRT_FILL_TIME_NAME, &fill_time) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill time");
    if(H5P_get(dc_plist, H5D_CRT_DATA_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get data pipeline");

    /* Reset start & count arrays */
    HDmemset(start, 0, sizeof(start));
    HDmemset(count, 0, sizeof(count));

    /* Go get the rank & dimensions */
    if((srank = H5S_get_simple_extent_dims(space, curr_dims, NULL)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataset dimensions");
    rank=srank;

    /* Copy current dimensions */
    for(u = 0; u < rank; u++)
	size[u] = curr_dims[u];
    size[u] = layout->dim[u];

    /* Create a data space for a chunk & set the extent */
    if(NULL == (space_chunk = H5S_create_simple(rank,layout->dim,NULL)))
	HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace");

/*
 * Set up multi-dimensional counters (idx_min, idx_max, and idx_cur) and
 * loop through the chunks copying each chunk from the application to the
 * chunk cache.
 */
    for(u = 0; u < layout->ndims; u++) {
	idx_min[u] = 0;
	idx_max[u] = (size[u] - 1) / layout->dim[u] + 1;
	idx_cur[u] = idx_min[u];
    } /* end for */

    /* Loop over all chunks */
    carry=0;
    while(carry==0) {
	for(u = 0, naccessed = 1; u < layout->ndims; u++) {
	    /* The location and size of the chunk being accessed */
	    chunk_offset[u] = idx_cur[u] * (hssize_t)(layout->dim[u]);
	    sub_size[u] = MIN((idx_cur[u] + 1) * layout->dim[u],
		    size[u]) - chunk_offset[u];
	    naccessed *= sub_size[u];
	} /* end for */

	/* 
	 * Figure out what chunks have to be initialized. These are the chunks where the dataspace 
	 * extent boundary is within the chunk
	 */
	for(u = 0, found = 0; u < layout->ndims - 1; u++) {
	    end_chunk = chunk_offset[u] + layout->dim[u];
	    if(end_chunk > size[u]) {
		found = 1;
		break;
	    }
	} /* end for */

	if(found) {

	    if(NULL == (chunk = H5F_istore_lock(f, dxpl_id, layout, &pline, &fill, fill_time,
			    chunk_offset, FALSE, &idx_hint)))
		HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to read raw data chunk");

	    if(H5S_select_all(space_chunk,1) < 0)
		HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to select space");

	    for(u = 0; u < rank; u++)
		count[u] = MIN((idx_cur[u] + 1) * layout->dim[u], size[u] - chunk_offset[u]);

#if defined (H5F_ISTORE_DEBUG)
	    HDfputs("cache:initialize:offset:[", stdout);
	    for(u = 0; u < layout->ndims - 1; u++)
		HDfprintf(stdout, "%s%Hd", u ? ", " : "", chunk_offset[u]);
	    HDfputs("]", stdout);
	    HDfputs(":count:[", stdout);
	    for(u = 0; u < layout->ndims - 1; u++)
		HDfprintf(stdout, "%s%Hd", u ? ", " : "", count[u]);
	    HDfputs("]\n", stdout);
#endif

	    if(H5S_select_hyperslab(space_chunk, H5S_SELECT_NOTB, start, NULL,
			count, NULL) < 0)
		HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to select hyperslab");

	    /* Fill the selection in the memory buffer */
            /* Use the size of the elements in the chunk directly instead of */
            /* relying on the fill.size, which might be set to 0 if there is */
            /* no fill-value defined for the dataset -QAK */
            H5_CHECK_OVERFLOW(size[rank],hsize_t,size_t);
	    if(H5S_select_fill(fill.buf, (size_t)size[rank], space_chunk, chunk) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTENCODE, FAIL, "filling selection failed");

	    if(H5F_istore_unlock(f, dxpl_id, layout, &pline, TRUE,
                    chunk_offset, &idx_hint, chunk, (size_t)naccessed) < 0)
		HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to unlock raw data chunk");
	} /*found */

	/* Increment indices */
	for(i = layout->ndims - 1, carry = 1; i >= 0 && carry; --i) {
	    if(++idx_cur[i] >= idx_max[i])
		idx_cur[i] = idx_min[i];
	    else
		carry = 0;
	} /* end for */
    } /* end while */

done:
    if(space_chunk)
	H5S_close(space_chunk);

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_delete
 *
 * Purpose:	Delete raw data storage for entire dataset (i.e. all chunks)
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 20, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_delete(H5F_t *f, hid_t dxpl_id, const struct H5O_layout_t *layout)
{
    H5F_istore_ud1_t	udata;  /* User data for B-tree iterator call */
    H5F_rdcc_t		*rdcc = &(f->shared->rdcc);     /* File's raw data chunk cache */
    H5F_rdcc_ent_t	*ent, *next;    /* Pointers to cache entries */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_delete, FAIL);

    /* Check if the B-tree has been created in the file */
    if(H5F_addr_defined(layout->addr)) {
        /* Iterate through the entries in the cache, checking for the chunks to be deleted */
        for (ent=rdcc->head; ent; ent=next) {
            /* Get pointer to next node, in case this one is deleted */
            next=ent->next;

            /* Is the chunk to be deleted this cache entry? */
            if(layout->addr==ent->layout->addr)
                /* Remove entry without flushing */
            if (H5F_istore_preempt(f, dxpl_id, ent, FALSE )<0)
                    HGOTO_ERROR (H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks");
        } /* end for */

        /* Set up user data for B-tree deletion */
        HDmemset(&udata, 0, sizeof udata);
        udata.mesg = *layout;

        /* Delete entire B-tree */
        if(H5B_delete(f, dxpl_id, H5B_ISTORE, layout->addr, &udata)<0)
            HGOTO_ERROR(H5E_IO, H5E_CANTDELETE, 0, "unable to delete chunk B-tree");
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5F_istore_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_dump_btree
 *
 * Purpose:	Prints information about the storage B-tree to the specified
 *		stream.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 28, 1999
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_dump_btree(H5F_t *f, hid_t dxpl_id, FILE *stream, unsigned ndims, haddr_t addr)
{
    H5F_istore_ud1_t	udata;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5F_istore_dump_btree, FAIL);

    HDmemset(&udata, 0, sizeof udata);
    udata.mesg.ndims = ndims;
    udata.stream = stream;
    if(stream)
        HDfprintf(stream, "    Address: %a\n",addr);
    if(H5B_iterate(f, dxpl_id, H5B_ISTORE, H5F_istore_iter_dump, addr, &udata)<0)
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, 0, "unable to iterate over chunk B-tree");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_stats
 *
 * Purpose:	Print raw data cache statistics to the debug stream.  If
 *		HEADERS is non-zero then print table column headers,
 *		otherwise assume that the H5AC layer has already printed them.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_stats (H5F_t *f, hbool_t headers)
{
    H5F_rdcc_t	*rdcc = &(f->shared->rdcc);
    double	miss_rate;
    char	ascii[32];
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_stats, FAIL);

    if (!H5DEBUG(AC))
        HGOTO_DONE(SUCCEED);

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

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_istore_debug
 *
 * Purpose:	Debugs a B-tree node for indexed raw data storage.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5F_istore_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE * stream, int indent,
		 int fwidth, int ndims)
{
    H5F_istore_ud1_t	udata;
    herr_t ret_value=SUCCEED;   /* Return value */
    
    FUNC_ENTER_NOAPI(H5F_istore_debug, FAIL);

    HDmemset (&udata, 0, sizeof udata);
    udata.mesg.ndims = ndims;

    H5B_debug (f, dxpl_id, addr, stream, indent, fwidth, H5B_ISTORE, &udata);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

