/*
 * tclThreadAlloc.c --
 *
 *	This is a very fast storage allocator for used with threads (designed
 *	avoid lock contention).  The basic strategy is to allocate memory in
 *  	fixed size blocks from block caches.
 * 
 * The Initial Developer of the Original Code is America Online, Inc.
 * Portions created by AOL are Copyright (C) 1999 America Online, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id 
 */

#if defined(TCL_THREADS) && defined(USE_THREAD_ALLOC)

#include "tclInt.h"

#ifdef WIN32
#include "tclWinInt.h"
#else
extern Tcl_Mutex *TclpNewAllocMutex(void);
extern void *TclpGetAllocCache(void);
extern void TclpSetAllocCache(void *);
#endif

/*
 * If range checking is enabled, an additional byte will be allocated
 * to store the magic number at the end of the requested memory.
 */

#ifndef RCHECK
#ifdef  NDEBUG
#define RCHECK		0
#else
#define RCHECK		1
#endif
#endif

/*
 * The following define the number of Tcl_Obj's to allocate/move
 * at a time and the high water mark to prune a per-thread cache.
 * On a 32 bit system, sizeof(Tcl_Obj) = 24 so 800 * 24 = ~16k.
 *
 */
 
#define NOBJALLOC	 800
#define NOBJHIGH	1200

/*
 * The following defines the number of buckets in the bucket
 * cache and those block sizes from (1<<4) to (1<<(3+NBUCKETS))
 */

#define NBUCKETS	  11
#define MAXALLOC	  16284

/*
 * The following union stores accounting information for
 * each block including two small magic numbers and
 * a bucket number when in use or a next pointer when
 * free.  The original requested size (not including
 * the Block overhead) is also maintained.
 */
 
typedef struct Block {
    union {
    	struct Block *next;	  /* Next in free list. */
    	struct {
	    unsigned char magic1; /* First magic number. */
	    unsigned char bucket; /* Bucket block allocated from. */
	    unsigned char unused; /* Padding. */
	    unsigned char magic2; /* Second magic number. */
        } b_s;
    } b_u;
    size_t b_reqsize;		  /* Requested allocation size. */
} Block;
#define b_next		b_u.next
#define b_bucket	b_u.b_s.bucket
#define b_magic1	b_u.b_s.magic1
#define b_magic2	b_u.b_s.magic2
#define MAGIC		0xef

/*
 * The following structure defines a bucket of blocks with
 * various accounting and statistics information.
 */

typedef struct Bucket {
    Block *firstPtr;
    int nfree;
    int nget;
    int nput;
    int nwait;
    int nlock;
    int nrequest;
} Bucket;

/*
 * The following structure defines a cache of buckets and objs.
 */

typedef struct Cache {
    struct Cache  *nextPtr;
    Tcl_ThreadId   owner;
    Tcl_Obj       *firstObjPtr;
    int            nobjs;
    int	           nsysalloc;
    Bucket         buckets[NBUCKETS];
} Cache;

/*
 * The following array specifies various per-bucket 
 * limits and locks.  The values are statically initialized
 * to avoid calculating them repeatedly.
 */

struct binfo {
    size_t blocksize;	/* Bucket blocksize. */
    int maxblocks;	/* Max blocks before move to share. */
    int nmove;		/* Num blocks to move to share. */
    Tcl_Mutex *lockPtr; /* Share bucket lock. */
} binfo[NBUCKETS] = {
    {   16, 1024, 512, NULL},
    {   32,  512, 256, NULL},
    {   64,  256, 128, NULL},
    {  128,  128,  64, NULL},
    {  256,   64,  32, NULL},
    {  512,   32,  16, NULL},
    { 1024,   16,   8, NULL},
    { 2048,    8,   4, NULL},
    { 4096,    4,   2, NULL},
    { 8192,    2,   1, NULL},
    {16284,    1,   1, NULL},
};

/*
 * Static functions defined in this file.
 */

static void LockBucket(Cache *cachePtr, int bucket);
static void UnlockBucket(Cache *cachePtr, int bucket);
static void PutBlocks(Cache *cachePtr, int bucket, int nmove);
static int  GetBlocks(Cache *cachePtr, int bucket);
static Block *Ptr2Block(char *ptr);
static char *Block2Ptr(Block *blockPtr, int bucket, unsigned int reqsize);
static void MoveObjs(Cache *fromPtr, Cache *toPtr, int nmove);

/*
 * Local variables defined in this file and initialized at
 * startup.
 */

static Tcl_Mutex *listLockPtr;
static Tcl_Mutex *objLockPtr;
static Cache     sharedCache;
static Cache    *sharedPtr = &sharedCache;
static Cache    *firstCachePtr = &sharedCache;


/*
 *----------------------------------------------------------------------
 *
 *  GetCache ---
 *
 *	Gets per-thread memory cache, allocating it if necessary.
 *
 * Results:
 *	Pointer to cache.
 *
 * Side effects:
 *  	None.
 *
 *----------------------------------------------------------------------
 */

static Cache *
GetCache(void)
{
    Cache *cachePtr;

    /*
     * Check for first-time initialization.
     */

    if (listLockPtr == NULL) {
	Tcl_Mutex *initLockPtr;
    	int i;

	initLockPtr = Tcl_GetAllocMutex();
	Tcl_MutexLock(initLockPtr);
	if (listLockPtr == NULL) {
	    listLockPtr = TclpNewAllocMutex();
	    objLockPtr = TclpNewAllocMutex();
	    for (i = 0; i < NBUCKETS; ++i) {
	        binfo[i].lockPtr = TclpNewAllocMutex();
	    }
	}
	Tcl_MutexUnlock(initLockPtr);
    }

    /*
     * Get this thread's cache, allocating if necessary.
     */

    cachePtr = TclpGetAllocCache();
    if (cachePtr == NULL) {
    	cachePtr = calloc(1, sizeof(Cache));
    	if (cachePtr == NULL) {
	    panic("alloc: could not allocate new cache");
    	}
    	Tcl_MutexLock(listLockPtr);
    	cachePtr->nextPtr = firstCachePtr;
    	firstCachePtr = cachePtr;
    	Tcl_MutexUnlock(listLockPtr);
    	cachePtr->owner = Tcl_GetCurrentThread();
	TclpSetAllocCache(cachePtr);
    }
    return cachePtr;
}


/*
 *----------------------------------------------------------------------
 *
 *  TclFreeAllocCache --
 *
 *	Flush and delete a cache, removing from list of caches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclFreeAllocCache(void *arg)
{
    Cache *cachePtr = arg;
    Cache **nextPtrPtr;
    register int   bucket;

    /*
     * Flush blocks.
     */

    for (bucket = 0; bucket < NBUCKETS; ++bucket) {
	if (cachePtr->buckets[bucket].nfree > 0) {
	    PutBlocks(cachePtr, bucket, cachePtr->buckets[bucket].nfree);
	}
    }

    /*
     * Flush objs.
     */

    if (cachePtr->nobjs > 0) {
    	Tcl_MutexLock(objLockPtr);
    	MoveObjs(cachePtr, sharedPtr, cachePtr->nobjs);
    	Tcl_MutexUnlock(objLockPtr);
    }

    /*
     * Remove from pool list.
     */

    Tcl_MutexLock(listLockPtr);
    nextPtrPtr = &firstCachePtr;
    while (*nextPtrPtr != cachePtr) {
	nextPtrPtr = &(*nextPtrPtr)->nextPtr;
    }
    *nextPtrPtr = cachePtr->nextPtr;
    cachePtr->nextPtr = NULL;
    Tcl_MutexUnlock(listLockPtr);
    free(cachePtr);
}


/*
 *----------------------------------------------------------------------
 *
 *  TclpAlloc --
 *
 *	Allocate memory.
 *
 * Results:
 *	Pointer to memory just beyond Block pointer.
 *
 * Side effects:
 *	May allocate more blocks for a bucket.
 *
 *----------------------------------------------------------------------
 */

char *
TclpAlloc(unsigned int reqsize)
{
    Cache         *cachePtr = TclpGetAllocCache();
    Block         *blockPtr;
    register int   bucket;
    size_t  	   size;

    if (cachePtr == NULL) {
	cachePtr = GetCache();
    }
    
    /*
     * Increment the requested size to include room for 
     * the Block structure.  Call malloc() directly if the
     * required amount is greater than the largest block,
     * otherwise pop the smallest block large enough,
     * allocating more blocks if necessary.
     */

    blockPtr = NULL;     
    size = reqsize + sizeof(Block);
#if RCHECK
    ++size;
#endif
    if (size > MAXALLOC) {
	bucket = NBUCKETS;
    	blockPtr = malloc(size);
	if (blockPtr != NULL) {
	    cachePtr->nsysalloc += reqsize;
	}
    } else {
    	bucket = 0;
    	while (binfo[bucket].blocksize < size) {
    	    ++bucket;
    	}
    	if (cachePtr->buckets[bucket].nfree || GetBlocks(cachePtr, bucket)) {
	    blockPtr = cachePtr->buckets[bucket].firstPtr;
	    cachePtr->buckets[bucket].firstPtr = blockPtr->b_next;
	    --cachePtr->buckets[bucket].nfree;
    	    ++cachePtr->buckets[bucket].nget;
	    cachePtr->buckets[bucket].nrequest += reqsize;
	}
    }
    if (blockPtr == NULL) {
    	return NULL;
    }
    return Block2Ptr(blockPtr, bucket, reqsize);
}


/*
 *----------------------------------------------------------------------
 *
 *  TclpFree --
 *
 *	Return blocks to the thread block cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May move blocks to shared cache.
 *
 *----------------------------------------------------------------------
 */

void
TclpFree(char *ptr)
{
    if (ptr != NULL) {
	Cache  *cachePtr = TclpGetAllocCache();
	Block *blockPtr;
	int bucket;

	if (cachePtr == NULL) {
	    cachePtr = GetCache();
	}
 
	/*
	 * Get the block back from the user pointer and
	 * call system free directly for large blocks.
	 * Otherwise, push the block back on the bucket and
	 * move blocks to the shared cache if there are now
	 * too many free.
	 */

	blockPtr = Ptr2Block(ptr);
	bucket = blockPtr->b_bucket;
	if (bucket == NBUCKETS) {
	    cachePtr->nsysalloc -= blockPtr->b_reqsize;
	    free(blockPtr);
	} else {
	    cachePtr->buckets[bucket].nrequest -= blockPtr->b_reqsize;
	    blockPtr->b_next = cachePtr->buckets[bucket].firstPtr;
	    cachePtr->buckets[bucket].firstPtr = blockPtr;
	    ++cachePtr->buckets[bucket].nfree;
	    ++cachePtr->buckets[bucket].nput;
	    if (cachePtr != sharedPtr &&
		    cachePtr->buckets[bucket].nfree > binfo[bucket].maxblocks) {
		PutBlocks(cachePtr, bucket, binfo[bucket].nmove);
	    }
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 *  TclpRealloc --
 *
 *	Re-allocate memory to a larger or smaller size.
 *
 * Results:
 *	Pointer to memory just beyond Block pointer.
 *
 * Side effects:
 *	Previous memory, if any, may be freed.
 *
 *----------------------------------------------------------------------
 */

char *
TclpRealloc(char *ptr, unsigned int reqsize)
{
    Cache *cachePtr = TclpGetAllocCache();
    Block *blockPtr;
    void *new;
    size_t size, min;
    int bucket;

    if (ptr == NULL) {
	return TclpAlloc(reqsize);
    }

    if (cachePtr == NULL) {
	cachePtr = GetCache();
    }

    /*
     * If the block is not a system block and fits in place,
     * simply return the existing pointer.  Otherwise, if the block
     * is a system block and the new size would also require a system
     * block, call realloc() directly.
     */

    blockPtr = Ptr2Block(ptr);
    size = reqsize + sizeof(Block);
#if RCHECK
    ++size;
#endif
    bucket = blockPtr->b_bucket;
    if (bucket != NBUCKETS) {
	if (bucket > 0) {
	    min = binfo[bucket-1].blocksize;
	} else {
	    min = 0;
	}
	if (size > min && size <= binfo[bucket].blocksize) {
	    cachePtr->buckets[bucket].nrequest -= blockPtr->b_reqsize;
	    cachePtr->buckets[bucket].nrequest += reqsize;
	    return Block2Ptr(blockPtr, bucket, reqsize);
	}
    } else if (size > MAXALLOC) {
	cachePtr->nsysalloc -= blockPtr->b_reqsize;
	cachePtr->nsysalloc += reqsize;
	blockPtr = realloc(blockPtr, size);
	if (blockPtr == NULL) {
	    return NULL;
	}
	return Block2Ptr(blockPtr, NBUCKETS, reqsize);
    }

    /*
     * Finally, perform an expensive malloc/copy/free.
     */

    new = TclpAlloc(reqsize);
    if (new != NULL) {
	if (reqsize > blockPtr->b_reqsize) {
	    reqsize = blockPtr->b_reqsize;
	}
    	memcpy(new, ptr, reqsize);
    	TclpFree(ptr);
    }
    return new;
}


/*
 *----------------------------------------------------------------------
 *
 * TclThreadAllocObj --
 *
 *	Allocate a Tcl_Obj from the per-thread cache.
 *
 * Results:
 *	Pointer to uninitialized Tcl_Obj.
 *
 * Side effects:
 *	May move Tcl_Obj's from shared cached or allocate new Tcl_Obj's
 *  	if list is empty.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclThreadAllocObj(void)
{
    register Cache *cachePtr = TclpGetAllocCache();
    register int nmove;
    register Tcl_Obj *objPtr;
    Tcl_Obj *newObjsPtr;

    if (cachePtr == NULL) {
	cachePtr = GetCache();
    }

    /*
     * Get this thread's obj list structure and move
     * or allocate new objs if necessary.
     */
     
    if (cachePtr->nobjs == 0) {
    	Tcl_MutexLock(objLockPtr);
	nmove = sharedPtr->nobjs;
	if (nmove > 0) {
	    if (nmove > NOBJALLOC) {
		nmove = NOBJALLOC;
	    }
	    MoveObjs(sharedPtr, cachePtr, nmove);
	}
    	Tcl_MutexUnlock(objLockPtr);
	if (cachePtr->nobjs == 0) {
	    cachePtr->nobjs = nmove = NOBJALLOC;
	    newObjsPtr = malloc(sizeof(Tcl_Obj) * nmove);
	    if (newObjsPtr == NULL) {
		panic("alloc: could not allocate %d new objects", nmove);
	    }
	    while (--nmove >= 0) {
		objPtr = &newObjsPtr[nmove];
		objPtr->internalRep.otherValuePtr = cachePtr->firstObjPtr;
		cachePtr->firstObjPtr = objPtr;
	    }
	}
    }

    /*
     * Pop the first object.
     */

    objPtr = cachePtr->firstObjPtr;
    cachePtr->firstObjPtr = objPtr->internalRep.otherValuePtr;
    --cachePtr->nobjs;
    return objPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * TclThreadFreeObj --
 *
 *	Return a free Tcl_Obj to the per-thread cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May move free Tcl_Obj's to shared list upon hitting high
 *  	water mark.
 *
 *----------------------------------------------------------------------
 */

void
TclThreadFreeObj(Tcl_Obj *objPtr)
{
    Cache *cachePtr = TclpGetAllocCache();

    if (cachePtr == NULL) {
	cachePtr = GetCache();
    }

    /*
     * Get this thread's list and push on the free Tcl_Obj.
     */
     
    objPtr->internalRep.otherValuePtr = cachePtr->firstObjPtr;
    cachePtr->firstObjPtr = objPtr;
    ++cachePtr->nobjs;
    
    /*
     * If the number of free objects has exceeded the high
     * water mark, move some blocks to the shared list.
     */
     
    if (cachePtr->nobjs > NOBJHIGH) {
	Tcl_MutexLock(objLockPtr);
	MoveObjs(cachePtr, sharedPtr, NOBJALLOC);
	Tcl_MutexUnlock(objLockPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetMemoryInfo --
 *
 *	Return a list-of-lists of memory stats.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *  	List appended to given dstring.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetMemoryInfo(Tcl_DString *dsPtr)
{
    Cache *cachePtr;
    char buf[200];
    int n;

    Tcl_MutexLock(listLockPtr);
    cachePtr = firstCachePtr;
    while (cachePtr != NULL) {
	Tcl_DStringStartSublist(dsPtr);
	if (cachePtr == sharedPtr) {
    	    Tcl_DStringAppendElement(dsPtr, "shared");
	} else {
	    sprintf(buf, "thread%d", (int) cachePtr->owner);
    	    Tcl_DStringAppendElement(dsPtr, buf);
	}
	for (n = 0; n < NBUCKETS; ++n) {
    	    sprintf(buf, "%d %d %d %d %d %d %d",
		(int) binfo[n].blocksize,
		cachePtr->buckets[n].nfree,
		cachePtr->buckets[n].nget,
		cachePtr->buckets[n].nput,
		cachePtr->buckets[n].nrequest,
		cachePtr->buckets[n].nlock,
		cachePtr->buckets[n].nwait);
	    Tcl_DStringAppendElement(dsPtr, buf);
	}
	Tcl_DStringEndSublist(dsPtr);
	    cachePtr = cachePtr->nextPtr;
    }
    Tcl_MutexUnlock(listLockPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * MoveObjs --
 *
 *	Move Tcl_Obj's between caches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *  	None.
 *
 *----------------------------------------------------------------------
 */

static void
MoveObjs(Cache *fromPtr, Cache *toPtr, int nmove)
{
    register Tcl_Obj *objPtr = fromPtr->firstObjPtr;
    Tcl_Obj *fromFirstObjPtr = objPtr;

    toPtr->nobjs += nmove;
    fromPtr->nobjs -= nmove;

    /*
     * Find the last object to be moved; set the next one
     * (the first one not to be moved) as the first object
     * in the 'from' cache.
     */

    while (--nmove) {
	objPtr = objPtr->internalRep.otherValuePtr;
    }
    fromPtr->firstObjPtr = objPtr->internalRep.otherValuePtr;    

    /*
     * Move all objects as a block - they are already linked to
     * each other, we just have to update the first and last.
     */

    objPtr->internalRep.otherValuePtr = toPtr->firstObjPtr;
    toPtr->firstObjPtr = fromFirstObjPtr;
}


/*
 *----------------------------------------------------------------------
 *
 *  Block2Ptr, Ptr2Block --
 *
 *	Convert between internal blocks and user pointers.
 *
 * Results:
 *	User pointer or internal block.
 *
 * Side effects:
 *	Invalid blocks will abort the server.
 *
 *----------------------------------------------------------------------
 */

static char *
Block2Ptr(Block *blockPtr, int bucket, unsigned int reqsize) 
{
    register void *ptr;

    blockPtr->b_magic1 = blockPtr->b_magic2 = MAGIC;
    blockPtr->b_bucket = bucket;
    blockPtr->b_reqsize = reqsize;
    ptr = ((void *) (blockPtr + 1));
#if RCHECK
    ((unsigned char *)(ptr))[reqsize] = MAGIC;
#endif
    return (char *) ptr;
}

static Block *
Ptr2Block(char *ptr)
{
    register Block *blockPtr;

    blockPtr = (((Block *) ptr) - 1);
    if (blockPtr->b_magic1 != MAGIC
#if RCHECK
	|| ((unsigned char *) ptr)[blockPtr->b_reqsize] != MAGIC
#endif
	|| blockPtr->b_magic2 != MAGIC) {
	panic("alloc: invalid block: %p: %x %x %x\n",
	    blockPtr, blockPtr->b_magic1, blockPtr->b_magic2,
	    ((unsigned char *) ptr)[blockPtr->b_reqsize]);
    }
    return blockPtr;
}


/*
 *----------------------------------------------------------------------
 *
 *  LockBucket, UnlockBucket --
 *
 *	Set/unset the lock to access a bucket in the shared cache.
 *
 * Results:
 *  	None.
 *
 * Side effects:
 *	Lock activity and contention are monitored globally and on
 *  	a per-cache basis.
 *
 *----------------------------------------------------------------------
 */

static void
LockBucket(Cache *cachePtr, int bucket)
{
#if 0
    if (Tcl_MutexTryLock(binfo[bucket].lockPtr) != TCL_OK) {
	Tcl_MutexLock(binfo[bucket].lockPtr);
    	++cachePtr->buckets[bucket].nwait;
    	++sharedPtr->buckets[bucket].nwait;
    }
#else
    Tcl_MutexLock(binfo[bucket].lockPtr);
#endif
    ++cachePtr->buckets[bucket].nlock;
    ++sharedPtr->buckets[bucket].nlock;
}


static void
UnlockBucket(Cache *cachePtr, int bucket)
{
    Tcl_MutexUnlock(binfo[bucket].lockPtr);
}


/*
 *----------------------------------------------------------------------
 *
 *  PutBlocks --
 *
 *	Return unused blocks to the shared cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PutBlocks(Cache *cachePtr, int bucket, int nmove)
{
    register Block *lastPtr, *firstPtr;
    register int n = nmove;

    /*
     * Before acquiring the lock, walk the block list to find
     * the last block to be moved.
     */

    firstPtr = lastPtr = cachePtr->buckets[bucket].firstPtr;
    while (--n > 0) {
	lastPtr = lastPtr->b_next;
    }
    cachePtr->buckets[bucket].firstPtr = lastPtr->b_next;
    cachePtr->buckets[bucket].nfree -= nmove;

    /*
     * Aquire the lock and place the list of blocks at the front
     * of the shared cache bucket.
     */

    LockBucket(cachePtr, bucket);
    lastPtr->b_next = sharedPtr->buckets[bucket].firstPtr;
    sharedPtr->buckets[bucket].firstPtr = firstPtr;
    sharedPtr->buckets[bucket].nfree += nmove;
    UnlockBucket(cachePtr, bucket);
}


/*
 *----------------------------------------------------------------------
 *
 *  GetBlocks --
 *
 *	Get more blocks for a bucket.
 *
 * Results:
 *	1 if blocks where allocated, 0 otherwise.
 *
 * Side effects:
 *	Cache may be filled with available blocks.
 *
 *----------------------------------------------------------------------
 */

static int
GetBlocks(Cache *cachePtr, int bucket)
{
    register Block *blockPtr;
    register int n;
    register size_t size;

    /*
     * First, atttempt to move blocks from the shared cache.  Note
     * the potentially dirty read of nfree before acquiring the lock
     * which is a slight performance enhancement.  The value is
     * verified after the lock is actually acquired.
     */
     
    if (cachePtr != sharedPtr && sharedPtr->buckets[bucket].nfree > 0) {
	LockBucket(cachePtr, bucket);
	if (sharedPtr->buckets[bucket].nfree > 0) {

	    /*
	     * Either move the entire list or walk the list to find
	     * the last block to move.
	     */

	    n = binfo[bucket].nmove;
	    if (n >= sharedPtr->buckets[bucket].nfree) {
		cachePtr->buckets[bucket].firstPtr =
		    sharedPtr->buckets[bucket].firstPtr;
		cachePtr->buckets[bucket].nfree =
		    sharedPtr->buckets[bucket].nfree;
		sharedPtr->buckets[bucket].firstPtr = NULL;
		sharedPtr->buckets[bucket].nfree = 0;
	    } else {
		blockPtr = sharedPtr->buckets[bucket].firstPtr;
		cachePtr->buckets[bucket].firstPtr = blockPtr;
		sharedPtr->buckets[bucket].nfree -= n;
		cachePtr->buckets[bucket].nfree = n;
		while (--n > 0) {
    		    blockPtr = blockPtr->b_next;
		}
		sharedPtr->buckets[bucket].firstPtr = blockPtr->b_next;
		blockPtr->b_next = NULL;
	    }
	}
	UnlockBucket(cachePtr, bucket);
    }
    
    if (cachePtr->buckets[bucket].nfree == 0) {

	/*
	 * If no blocks could be moved from shared, first look for a
	 * larger block in this cache to split up.
	 */

    	blockPtr = NULL;
	n = NBUCKETS;
	size = 0; /* lint */
	while (--n > bucket) {
    	    if (cachePtr->buckets[n].nfree > 0) {
		size = binfo[n].blocksize;
		blockPtr = cachePtr->buckets[n].firstPtr;
		cachePtr->buckets[n].firstPtr = blockPtr->b_next;
		--cachePtr->buckets[n].nfree;
		break;
	    }
	}

	/*
	 * Otherwise, allocate a big new block directly.
	 */

	if (blockPtr == NULL) {
	    size = MAXALLOC;
	    blockPtr = malloc(size);
	    if (blockPtr == NULL) {
		return 0;
	    }
	}

	/*
	 * Split the larger block into smaller blocks for this bucket.
	 */

	n = size / binfo[bucket].blocksize;
	cachePtr->buckets[bucket].nfree = n;
	cachePtr->buckets[bucket].firstPtr = blockPtr;
	while (--n > 0) {
	    blockPtr->b_next = (Block *) 
		((char *) blockPtr + binfo[bucket].blocksize);
	    blockPtr = blockPtr->b_next;
	}
	blockPtr->b_next = NULL;
    }
    return 1;
}

#endif /* TCL_THREADS */
