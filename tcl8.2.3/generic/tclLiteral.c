/* 
 * tclLiteral.c --
 *
 *	Implementation of the global and ByteCode-local literal tables
 *	used to manage the Tcl objects created for literal values during
 *	compilation of Tcl scripts. This implementation borrows heavily
 *	from the more general hashtable implementation of Tcl hash tables
 *	that appears in tclHash.c.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclCompile.h"
#include "tclPort.h"
/*
 * When there are this many entries per bucket, on average, rebuild
 * a literal's hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3

/*
 * Procedure prototypes for static procedures in this file:
 */

static int		AddLocalLiteralEntry _ANSI_ARGS_((
			    CompileEnv *envPtr, LiteralEntry *globalPtr,
			    int localHash));
static void		ExpandLocalLiteralArray _ANSI_ARGS_((
			    CompileEnv *envPtr));
static unsigned int	HashString _ANSI_ARGS_((CONST char *bytes,
			    int length));
static void		RebuildLiteralTable _ANSI_ARGS_((
			    LiteralTable *tablePtr));

/*
 *----------------------------------------------------------------------
 *
 * TclInitLiteralTable --
 *
 *	This procedure is called to initialize the fields of a literal table
 *	structure for either an interpreter or a compilation's CompileEnv
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side effects: 
 *	The literal table is made ready for use.
 *
 *----------------------------------------------------------------------
 */

void
TclInitLiteralTable(tablePtr)
    register LiteralTable *tablePtr; /* Pointer to table structure, which
				      * is supplied by the caller. */
{
#if (TCL_SMALL_HASH_TABLE != 4) 
    panic("TclInitLiteralTable: TCL_SMALL_HASH_TABLE is %d, not 4\n",
	    TCL_SMALL_HASH_TABLE);
#endif
    
    tablePtr->buckets = tablePtr->staticBuckets;
    tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
    tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
    tablePtr->numBuckets = TCL_SMALL_HASH_TABLE;
    tablePtr->numEntries = 0;
    tablePtr->rebuildSize = TCL_SMALL_HASH_TABLE*REBUILD_MULTIPLIER;
    tablePtr->mask = 3;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteLiteralTable --
 *
 *	This procedure frees up everything associated with a literal table
 *	except for the table's structure itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each literal in the table is released: i.e., its reference count
 *	in the global literal table is decremented and, if it becomes zero,
 *	the literal is freed. In addition, the table's bucket array is
 *	freed.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteLiteralTable(interp, tablePtr)
    Tcl_Interp *interp;		/* Interpreter containing shared literals
				 * referenced by the table to delete. */
    LiteralTable *tablePtr;	/* Points to the literal table to delete. */
{
    LiteralEntry *entryPtr;
    int i, start;

    /*
     * Release remaining literals in the table. Note that releasing a
     * literal might release other literals, modifying the table, so we
     * restart the search from the bucket chain we last found an entry.
     */

#ifdef TCL_COMPILE_DEBUG
    TclVerifyGlobalLiteralTable((Interp *) interp);
#endif /*TCL_COMPILE_DEBUG*/

    start = 0;
    while (tablePtr->numEntries > 0) {
	for (i = start;  i < tablePtr->numBuckets;  i++) {
	    entryPtr = tablePtr->buckets[i];
	    if (entryPtr != NULL) {
		TclReleaseLiteral(interp, entryPtr->objPtr);
		start = i;
		break;
	    }
	}
    }

    /*
     * Free up the table's bucket array if it was dynamically allocated.
     */

    if (tablePtr->buckets != tablePtr->staticBuckets) {
	ckfree((char *) tablePtr->buckets);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclRegisterLiteral --
 *
 *	Find, or if necessary create, an object in a CompileEnv literal
 *	array that has a string representation matching the argument string.
 *
 * Results:
 *	The index in the CompileEnv's literal array that references a
 *	shared literal matching the string. The object is created if
 *	necessary.
 *
 * Side effects:
 *	To maximize sharing, we look up the string in the interpreter's
 *	global literal table. If not found, we create a new shared literal
 *	in the global table. We then add a reference to the shared
 *	literal in the CompileEnv's literal array. 
 *
 *	If onHeap is 1, this procedure is given ownership of the string: if
 *	an object is created then its string representation is set directly
 *	from string, otherwise the string is freed. Typically, a caller sets
 *	onHeap 1 if "string" is an already heap-allocated buffer holding the
 *	result of backslash substitutions.
 *
 *----------------------------------------------------------------------
 */

int
TclRegisterLiteral(envPtr, bytes, length, onHeap)
    CompileEnv *envPtr;		/* Points to the CompileEnv in whose object
				 * array an object is found or created. */
    register char *bytes;	/* Points to string for which to find or
				 * create an object in CompileEnv's object
				 * array. */
    int length;			/* Number of bytes in the string. If < 0,
				 * the string consists of all bytes up to
				 * the first null character. */
    int onHeap;			/* If 1 then the caller already malloc'd
				 * bytes and ownership is passed to this
				 * procedure. */
{
    Interp *iPtr = envPtr->iPtr;
    LiteralTable *globalTablePtr = &(iPtr->literalTable);
    LiteralTable *localTablePtr = &(envPtr->localLitTable);
    register LiteralEntry *globalPtr, *localPtr;
    register Tcl_Obj *objPtr;
    unsigned int hash;
    int localHash, globalHash, objIndex;
    long n;
    char buf[TCL_INTEGER_SPACE];

 
    if (length < 0) {
	length = (bytes? strlen(bytes) : 0);
    }
    hash = HashString(bytes, length);

    /*
     * Is the literal already in the CompileEnv's local literal array?
     * If so, just return its index.
     */

    localHash = (hash & localTablePtr->mask);
    for (localPtr = localTablePtr->buckets[localHash];
	  localPtr != NULL;  localPtr = localPtr->nextPtr) {
	objPtr = localPtr->objPtr;
	if ((objPtr->length == length) && ((length == 0)
		|| ((objPtr->bytes[0] == bytes[0])
			&& (memcmp(objPtr->bytes, bytes, (unsigned) length)
				== 0)))) {
	    if (onHeap) {
		ckfree(bytes);
	    }
	    objIndex = (localPtr - envPtr->literalArrayPtr);
#ifdef TCL_COMPILE_DEBUG
	    TclVerifyLocalLiteralTable(envPtr);
#endif /*TCL_COMPILE_DEBUG*/

	    return objIndex;
	}
    }

    /*
     * The literal is new to this CompileEnv. Is it in the interpreter's
     * global literal table?
     */

    globalHash = (hash & globalTablePtr->mask);
    for (globalPtr = globalTablePtr->buckets[globalHash];
	 globalPtr != NULL;  globalPtr = globalPtr->nextPtr) {
	objPtr = globalPtr->objPtr;
	if ((objPtr->length == length) && ((length == 0)
		|| ((objPtr->bytes[0] == bytes[0])
			&& (memcmp(objPtr->bytes, bytes, (unsigned) length)
				== 0)))) {
	    /*
	     * A global literal was found. Add an entry to the CompileEnv's
	     * local literal array.
	     */
	    
	    if (onHeap) {
		ckfree(bytes);
	    }
	    objIndex = AddLocalLiteralEntry(envPtr, globalPtr, localHash);
#ifdef TCL_COMPILE_DEBUG
	    if (globalPtr->refCount < 1) {
		panic("TclRegisterLiteral: global literal \"%.*s\" had bad refCount %d",
			(length>60? 60 : length), bytes,
			globalPtr->refCount);
	    }
	    TclVerifyLocalLiteralTable(envPtr);
#endif /*TCL_COMPILE_DEBUG*/ 
	    return objIndex;
	}
    }

    /*
     * The literal is new to the interpreter. Add it to the global literal
     * table then add an entry to the CompileEnv's local literal array.
     * Convert the object to an integer object if possible.
     */

    TclNewObj(objPtr);
    Tcl_IncrRefCount(objPtr);
    if (onHeap) {
	objPtr->bytes = bytes;
	objPtr->length = length;
    } else {
	TclInitStringRep(objPtr, bytes, length);
    }

    if (TclLooksLikeInt(bytes, length)) {
	/*
	 * From here we use the objPtr, because it is NULL terminated
	 */
	if (TclGetLong((Tcl_Interp *) NULL, objPtr->bytes, &n) == TCL_OK) {
	    TclFormatInt(buf, n);
	    if (strcmp(objPtr->bytes, buf) == 0) {
		objPtr->internalRep.longValue = n;
		objPtr->typePtr = &tclIntType;
	    }
	}
    }
    
#ifdef TCL_COMPILE_DEBUG
    if (TclLookupLiteralEntry((Tcl_Interp *) iPtr, objPtr) != NULL) {
	panic("TclRegisterLiteral: literal \"%.*s\" found globally but shouldn't be",
	        (length>60? 60 : length), bytes);
    }
#endif

    globalPtr = (LiteralEntry *) ckalloc((unsigned) sizeof(LiteralEntry));
    globalPtr->objPtr = objPtr;
    globalPtr->refCount = 0;
    globalPtr->nextPtr = globalTablePtr->buckets[globalHash];
    globalTablePtr->buckets[globalHash] = globalPtr;
    globalTablePtr->numEntries++;

    /*
     * If the global literal table has exceeded a decent size, rebuild it
     * with more buckets.
     */

    if (globalTablePtr->numEntries >= globalTablePtr->rebuildSize) {
	RebuildLiteralTable(globalTablePtr);
    }
    
    objIndex = AddLocalLiteralEntry(envPtr, globalPtr, localHash);

#ifdef TCL_COMPILE_DEBUG
    TclVerifyGlobalLiteralTable(iPtr);
    TclVerifyLocalLiteralTable(envPtr);
    {
	LiteralEntry *entryPtr;
	int found, i;
	found = 0;
	for (i = 0;  i < globalTablePtr->numBuckets;  i++) {
	    for (entryPtr = globalTablePtr->buckets[i];
		    entryPtr != NULL;  entryPtr = entryPtr->nextPtr) {
		if ((entryPtr == globalPtr)
		        && (entryPtr->objPtr == objPtr)) {
		    found = 1;
		}
	    }
	}
	if (!found) {
	    panic("TclRegisterLiteral: literal \"%.*s\" wasn't global",
	            (length>60? 60 : length), bytes);
	}
    }
#endif /*TCL_COMPILE_DEBUG*/
#ifdef TCL_COMPILE_STATS   
    iPtr->stats.numLiteralsCreated++;
    iPtr->stats.totalLitStringBytes   += (double) (length + 1);
    iPtr->stats.currentLitStringBytes += (double) (length + 1);
    iPtr->stats.literalCount[TclLog2(length)]++;
#endif /*TCL_COMPILE_STATS*/
    return objIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * TclLookupLiteralEntry --
 *
 *	Finds the LiteralEntry that corresponds to a literal Tcl object
 *      holding a literal.
 *
 * Results:
 *      Returns the matching LiteralEntry if found, otherwise NULL.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

LiteralEntry *
TclLookupLiteralEntry(interp, objPtr)
    Tcl_Interp *interp;		/* Interpreter for which objPtr was created
                                 * to hold a literal. */
    register Tcl_Obj *objPtr;	/* Points to a Tcl object holding a
                                 * literal that was previously created by a
                                 * call to TclRegisterLiteral. */
{
    Interp *iPtr = (Interp *) interp;
    LiteralTable *globalTablePtr = &(iPtr->literalTable);
    register LiteralEntry *entryPtr;
    char *bytes;
    int length, globalHash;

    bytes = Tcl_GetStringFromObj(objPtr, &length);
    globalHash = (HashString(bytes, length) & globalTablePtr->mask);
    for (entryPtr = globalTablePtr->buckets[globalHash];
            entryPtr != NULL;  entryPtr = entryPtr->nextPtr) {
        if (entryPtr->objPtr == objPtr) {
            return entryPtr;
        }
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclHideLiteral --
 *
 *	Remove a literal entry from the literal hash tables, leaving it in
 *	the literal array so existing references continue to function.
 *	This makes it possible to turn a shared literal into a private
 *	literal that cannot be shared.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the literal from the local hash table and decrements the
 *	global hash entry's reference count.
 *
 *----------------------------------------------------------------------
 */

void
TclHideLiteral(interp, envPtr, index)
    Tcl_Interp *interp;		 /* Interpreter for which objPtr was created
                                  * to hold a literal. */
    register CompileEnv *envPtr; /* Points to CompileEnv whose literal array
				  * contains the entry being hidden. */
    int index;			 /* The index of the entry in the literal
				  * array. */
{
    LiteralEntry **nextPtrPtr, *entryPtr, *lPtr;
    LiteralTable *localTablePtr = &(envPtr->localLitTable);
    int localHash, length;
    char *bytes;
    Tcl_Obj *newObjPtr;

    lPtr = &(envPtr->literalArrayPtr[index]);

    /*
     * To avoid unwanted sharing we need to copy the object and remove it from
     * the local and global literal tables.  It still has a slot in the literal
     * array so it can be referred to by byte codes, but it will not be matched
     * by literal searches.
     */

    newObjPtr = Tcl_DuplicateObj(lPtr->objPtr);
    Tcl_IncrRefCount(newObjPtr);
    TclReleaseLiteral(interp, lPtr->objPtr);
    lPtr->objPtr = newObjPtr;

    bytes = Tcl_GetStringFromObj(newObjPtr, &length);
    localHash = (HashString(bytes, length) & localTablePtr->mask);
    nextPtrPtr = &localTablePtr->buckets[localHash];

    for (entryPtr = *nextPtrPtr; entryPtr != NULL; entryPtr = *nextPtrPtr) {
	if (entryPtr == lPtr) {
	    *nextPtrPtr = lPtr->nextPtr;
	    lPtr->nextPtr = NULL;
	    localTablePtr->numEntries--;
	    break;
	}
	nextPtrPtr = &entryPtr->nextPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclAddLiteralObj --
 *
 *	Add a single literal object to the literal array.  This
 *	function does not add the literal to the local or global
 *	literal tables.  The caller is expected to add the entry
 *	to whatever tables are appropriate.
 *
 * Results:
 *	The index in the CompileEnv's literal array that references the
 *	literal.  Stores the pointer to the new literal entry in the
 *	location referenced by the localPtrPtr argument.
 *
 * Side effects:
 *	Expands the literal array if necessary.  Increments the refcount
 *	on the literal object.
 *
 *----------------------------------------------------------------------
 */

int
TclAddLiteralObj(envPtr, objPtr, litPtrPtr)
    register CompileEnv *envPtr; /* Points to CompileEnv in whose literal
				  * array the object is to be inserted. */
    Tcl_Obj *objPtr;		 /* The object to insert into the array. */
    LiteralEntry **litPtrPtr;	 /* The location where the pointer to the
				  * new literal entry should be stored.
				  * May be NULL. */
{
    register LiteralEntry *lPtr;
    int objIndex;

    if (envPtr->literalArrayNext >= envPtr->literalArrayEnd) {
	ExpandLocalLiteralArray(envPtr);
    }
    objIndex = envPtr->literalArrayNext;
    envPtr->literalArrayNext++;

    lPtr = &(envPtr->literalArrayPtr[objIndex]);
    lPtr->objPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    lPtr->refCount = -1;	/* i.e., unused */
    lPtr->nextPtr = NULL;

    if (litPtrPtr) {
	*litPtrPtr = lPtr;
    }

    return objIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * AddLocalLiteralEntry --
 *
 *	Insert a new literal into a CompileEnv's local literal array.
 *
 * Results:
 *	The index in the CompileEnv's literal array that references the
 *	literal.
 *
 * Side effects:
 *	Increments the ref count of the global LiteralEntry since the
 *	CompileEnv now refers to the literal. Expands the literal array
 *	if necessary. May rebuild the hash bucket array of the CompileEnv's
 *	literal array if it becomes too large.
 *
 *----------------------------------------------------------------------
 */

static int
AddLocalLiteralEntry(envPtr, globalPtr, localHash)
    register CompileEnv *envPtr; /* Points to CompileEnv in whose literal
				  * array the object is to be inserted. */
    LiteralEntry *globalPtr;	 /* Points to the global LiteralEntry for
				  * the literal to add to the CompileEnv. */
    int localHash;		 /* Hash value for the literal's string. */
{
    register LiteralTable *localTablePtr = &(envPtr->localLitTable);
    LiteralEntry *localPtr;
    int objIndex;
    
    objIndex = TclAddLiteralObj(envPtr, globalPtr->objPtr, &localPtr);

    /*
     * Add the literal to the local table.
     */

    localPtr->nextPtr = localTablePtr->buckets[localHash];
    localTablePtr->buckets[localHash] = localPtr;
    localTablePtr->numEntries++;

    globalPtr->refCount++;

    /*
     * If the CompileEnv's local literal table has exceeded a decent size,
     * rebuild it with more buckets.
     */

    if (localTablePtr->numEntries >= localTablePtr->rebuildSize) {
	RebuildLiteralTable(localTablePtr);
    }

#ifdef TCL_COMPILE_DEBUG
    TclVerifyLocalLiteralTable(envPtr);
    {
	char *bytes;
	int length, found, i;
	found = 0;
	for (i = 0;  i < localTablePtr->numBuckets;  i++) {
	    for (localPtr = localTablePtr->buckets[i];
		    localPtr != NULL;  localPtr = localPtr->nextPtr) {
		if (localPtr->objPtr == globalPtr->objPtr) {
		    found = 1;
		}
	    }
	}
	if (!found) {
	    bytes = Tcl_GetStringFromObj(globalPtr->objPtr, &length);
	    panic("AddLocalLiteralEntry: literal \"%.*s\" wasn't found locally",
	            (length>60? 60 : length), bytes);
	}
    }
#endif /*TCL_COMPILE_DEBUG*/
    return objIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * ExpandLocalLiteralArray --
 *
 *	Procedure that uses malloc to allocate more storage for a
 *	CompileEnv's local literal array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The literal array in *envPtr is reallocated to a new array of
 *	double the size, and if envPtr->mallocedLiteralArray is non-zero
 *	the old array is freed. Entries are copied from the old array
 *	to the new one. The local literal table is updated to refer to
 *	the new entries.
 *
 *----------------------------------------------------------------------
 */

static void
ExpandLocalLiteralArray(envPtr)
    register CompileEnv *envPtr; /* Points to the CompileEnv whose object
				  * array must be enlarged. */
{
    /*
     * The current allocated local literal entries are stored between
     * elements 0 and (envPtr->literalArrayNext - 1) [inclusive].
     */

    LiteralTable *localTablePtr = &(envPtr->localLitTable);
    int currElems = envPtr->literalArrayNext;
    size_t currBytes = (currElems * sizeof(LiteralEntry));
    register LiteralEntry *currArrayPtr = envPtr->literalArrayPtr;
    register LiteralEntry *newArrayPtr =
	    (LiteralEntry *) ckalloc((unsigned) (2 * currBytes));
    int i;
    
    /*
     * Copy from the old literal array to the new, then update the local
     * literal table's bucket array.
     */

    memcpy((VOID *) newArrayPtr, (VOID *) currArrayPtr, currBytes);
    for (i = 0;  i < currElems;  i++) {
	if (currArrayPtr[i].nextPtr == NULL) {
	    newArrayPtr[i].nextPtr = NULL;
	} else {
	    newArrayPtr[i].nextPtr = newArrayPtr
		    + (currArrayPtr[i].nextPtr - currArrayPtr);
	}
    }
    for (i = 0;  i < localTablePtr->numBuckets;  i++) {
	if (localTablePtr->buckets[i] != NULL) {
	    localTablePtr->buckets[i] = newArrayPtr
	            + (localTablePtr->buckets[i] - currArrayPtr);
	}
    }

    /*
     * Free the old literal array if needed, and mark the new literal
     * array as malloced.
     */
    
    if (envPtr->mallocedLiteralArray) {
	ckfree((char *) currArrayPtr);
    }
    envPtr->literalArrayPtr = newArrayPtr;
    envPtr->literalArrayEnd = (2 * currElems);
    envPtr->mallocedLiteralArray = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclReleaseLiteral --
 *
 *	This procedure releases a reference to one of the shared Tcl objects
 *	that hold literals. It is called to release the literals referenced
 *	by a ByteCode that is being destroyed, and it is also called by
 *	TclDeleteLiteralTable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count for the global LiteralTable entry that 
 *	corresponds to the literal is decremented. If no other reference
 *	to a global literal object remains, it is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclReleaseLiteral(interp, objPtr)
    Tcl_Interp *interp;		/* Interpreter for which objPtr was created
				 * to hold a literal. */
    register Tcl_Obj *objPtr;	/* Points to a literal object that was
				 * previously created by a call to
				 * TclRegisterLiteral. */
{
    Interp *iPtr = (Interp *) interp;
    LiteralTable *globalTablePtr = &(iPtr->literalTable);
    register LiteralEntry *entryPtr, *prevPtr;
    ByteCode* codePtr;
    char *bytes;
    int length, index;

    bytes = Tcl_GetStringFromObj(objPtr, &length);
    index = (HashString(bytes, length) & globalTablePtr->mask);

    /*
     * Check to see if the object is in the global literal table and 
     * remove this reference.  The object may not be in the table if
     * it is a hidden local literal.
     */

    for (prevPtr = NULL, entryPtr = globalTablePtr->buckets[index];
	    entryPtr != NULL;
	    prevPtr = entryPtr, entryPtr = entryPtr->nextPtr) {
	if (entryPtr->objPtr == objPtr) {
	    entryPtr->refCount--;

	    /*
	     * We found the matching LiteralEntry. Check if it's only being
	     * kept alive only by a circular reference from a ByteCode
	     * stored as its internal rep.
	     */
	    
	    if ((entryPtr->refCount == 1)
		    && (objPtr->typePtr == &tclByteCodeType)) {
		codePtr = (ByteCode *) objPtr->internalRep.otherValuePtr;
		if ((codePtr->numLitObjects == 1)
		        && (codePtr->objArrayPtr[0] == objPtr)) {
		    entryPtr->refCount = 0;

		    /*
		     * Set the ByteCode object array entry NULL to signal
		     * to TclCleanupByteCode to not try to release this
		     * about to be freed literal again.
		     */

		    codePtr->objArrayPtr[0] = NULL;
		}
	    }

	    /*
	     * If the literal is no longer being used by any ByteCode,
	     * delete the entry then decrement the ref count of its object.
	     */
		
	    if (entryPtr->refCount == 0) {
		if (prevPtr == NULL) {
		    globalTablePtr->buckets[index] = entryPtr->nextPtr;
		} else {
		    prevPtr->nextPtr = entryPtr->nextPtr;
		}
#ifdef TCL_COMPILE_STATS
		iPtr->stats.currentLitStringBytes -= (double) (length + 1);
#endif /*TCL_COMPILE_STATS*/
		ckfree((char *) entryPtr);
		globalTablePtr->numEntries--;

		/*
		 * Remove the reference corresponding to the global 
		 * literal table entry.
		 */

		TclDecrRefCount(objPtr);
	    }
	    break;
	}
    }

    /*
     * Remove the reference corresponding to the local literal table
     * entry.
     */

    Tcl_DecrRefCount(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * HashString --
 *
 *	Compute a one-word summary of a text string, which can be
 *	used to generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in
 *	string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
HashString(bytes, length)
    register CONST char *bytes; /* String for which to compute hash
				 * value. */
    int length;			/* Number of bytes in the string. */
{
    register unsigned int result;
    register int i;

    /*
     * I tried a zillion different hash functions and asked many other
     * people for advice.  Many people had their own favorite functions,
     * all different, but no-one had much idea why they were good ones.
     * I chose the one below (multiply by 9 and add new character)
     * because of the following reasons:
     *
     * 1. Multiplying by 10 is perfect for keys that are decimal strings,
     *    and multiplying by 9 is just about as good.
     * 2. Times-9 is (shift-left-3) plus (old).  This means that each
     *    character's bits hang around in the low-order bits of the
     *    hash value for ever, plus they spread fairly rapidly up to
     *    the high-order bits to fill out the hash value.  This seems
     *    works well both for decimal and non-decimal strings.
     */

    result = 0;
    for (i = 0;  i < length;  i++) {
	result += (result<<3) + *bytes++;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RebuildLiteralTable --
 *
 *	This procedure is invoked when the ratio of entries to hash buckets
 *	becomes too large in a local or global literal table. It allocates
 *	a larger bucket array and moves the entries into the new buckets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get rehashed into new buckets.
 *
 *----------------------------------------------------------------------
 */

static void
RebuildLiteralTable(tablePtr)
    register LiteralTable *tablePtr; /* Local or global table to enlarge. */
{
    LiteralEntry **oldBuckets;
    register LiteralEntry **oldChainPtr, **newChainPtr;
    register LiteralEntry *entryPtr;
    LiteralEntry **bucketPtr;
    char *bytes;
    int oldSize, count, index, length;

    oldSize = tablePtr->numBuckets;
    oldBuckets = tablePtr->buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */

    tablePtr->numBuckets *= 4;
    tablePtr->buckets = (LiteralEntry **) ckalloc((unsigned)
	    (tablePtr->numBuckets * sizeof(LiteralEntry *)));
    for (count = tablePtr->numBuckets, newChainPtr = tablePtr->buckets;
	    count > 0;
	    count--, newChainPtr++) {
	*newChainPtr = NULL;
    }
    tablePtr->rebuildSize *= 4;
    tablePtr->mask = (tablePtr->mask << 2) + 3;

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    for (oldChainPtr = oldBuckets;
	    oldSize > 0;
	    oldSize--, oldChainPtr++) {
	for (entryPtr = *oldChainPtr;  entryPtr != NULL;
	        entryPtr = *oldChainPtr) {
	    bytes = Tcl_GetStringFromObj(entryPtr->objPtr, &length);
	    index = (HashString(bytes, length) & tablePtr->mask);
	    
	    *oldChainPtr = entryPtr->nextPtr;
	    bucketPtr = &(tablePtr->buckets[index]);
	    entryPtr->nextPtr = *bucketPtr;
	    *bucketPtr = entryPtr;
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != tablePtr->staticBuckets) {
	ckfree((char *) oldBuckets);
    }
}

#ifdef TCL_COMPILE_STATS
/*
 *----------------------------------------------------------------------
 *
 * TclLiteralStats --
 *
 *	Return statistics describing the layout of the hash table
 *	in its hash buckets.
 *
 * Results:
 *	The return value is a malloc-ed string containing information
 *	about tablePtr.  It is the caller's responsibility to free
 *	this string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TclLiteralStats(tablePtr)
    LiteralTable *tablePtr;	/* Table for which to produce stats. */
{
#define NUM_COUNTERS 10
    int count[NUM_COUNTERS], overflow, i, j;
    double average, tmp;
    register LiteralEntry *entryPtr;
    char *result, *p;

    /*
     * Compute a histogram of bucket usage. For each bucket chain i,
     * j is the number of entries in the chain.
     */

    for (i = 0;  i < NUM_COUNTERS;  i++) {
	count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    for (i = 0;  i < tablePtr->numBuckets;  i++) {
	j = 0;
	for (entryPtr = tablePtr->buckets[i];  entryPtr != NULL;
	        entryPtr = entryPtr->nextPtr) {
	    j++;
	}
	if (j < NUM_COUNTERS) {
	    count[j]++;
	} else {
	    overflow++;
	}
	tmp = j;
	average += (tmp+1.0)*(tmp/tablePtr->numEntries)/2.0;
    }

    /*
     * Print out the histogram and a few other pieces of information.
     */

    result = (char *) ckalloc((unsigned) ((NUM_COUNTERS*60) + 300));
    sprintf(result, "%d entries in table, %d buckets\n",
	    tablePtr->numEntries, tablePtr->numBuckets);
    p = result + strlen(result);
    for (i = 0; i < NUM_COUNTERS; i++) {
	sprintf(p, "number of buckets with %d entries: %d\n",
		i, count[i]);
	p += strlen(p);
    }
    sprintf(p, "number of buckets with %d or more entries: %d\n",
	    NUM_COUNTERS, overflow);
    p += strlen(p);
    sprintf(p, "average search distance for entry: %.1f", average);
    return result;
}
#endif /*TCL_COMPILE_STATS*/

#ifdef TCL_COMPILE_DEBUG
/*
 *----------------------------------------------------------------------
 *
 * TclVerifyLocalLiteralTable --
 *
 *	Check a CompileEnv's local literal table for consistency.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Panics if problems are found.
 *
 *----------------------------------------------------------------------
 */

void
TclVerifyLocalLiteralTable(envPtr)
    CompileEnv *envPtr;		/* Points to CompileEnv whose literal
				 * table is to be validated. */
{
    register LiteralTable *localTablePtr = &(envPtr->localLitTable);
    register LiteralEntry *localPtr;
    char *bytes;
    register int i;
    int length, count;

    count = 0;
    for (i = 0;  i < localTablePtr->numBuckets;  i++) {
	for (localPtr = localTablePtr->buckets[i];
	        localPtr != NULL;  localPtr = localPtr->nextPtr) {
	    count++;
	    if (localPtr->refCount != -1) {
		bytes = Tcl_GetStringFromObj(localPtr->objPtr, &length);
		panic("TclVerifyLocalLiteralTable: local literal \"%.*s\" had bad refCount %d",
		        (length>60? 60 : length), bytes,
		        localPtr->refCount);
	    }
	    if (TclLookupLiteralEntry((Tcl_Interp *) envPtr->iPtr,
		    localPtr->objPtr) == NULL) {
		bytes = Tcl_GetStringFromObj(localPtr->objPtr, &length);
		panic("TclVerifyLocalLiteralTable: local literal \"%.*s\" is not global",
		         (length>60? 60 : length), bytes);
	    }
	    if (localPtr->objPtr->bytes == NULL) {
		panic("TclVerifyLocalLiteralTable: literal has NULL string rep");
	    }
	}
    }
    if (count != localTablePtr->numEntries) {
	panic("TclVerifyLocalLiteralTable: local literal table had %d entries, should be %d",
	      count, localTablePtr->numEntries);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclVerifyGlobalLiteralTable --
 *
 *	Check an interpreter's global literal table literal for consistency.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Panics if problems are found.
 *
 *----------------------------------------------------------------------
 */

void
TclVerifyGlobalLiteralTable(iPtr)
    Interp *iPtr;		/* Points to interpreter whose global
				 * literal table is to be validated. */
{
    register LiteralTable *globalTablePtr = &(iPtr->literalTable);
    register LiteralEntry *globalPtr;
    char *bytes;
    register int i;
    int length, count;

    count = 0;
    for (i = 0;  i < globalTablePtr->numBuckets;  i++) {
	for (globalPtr = globalTablePtr->buckets[i];
	        globalPtr != NULL;  globalPtr = globalPtr->nextPtr) {
	    count++;
	    if (globalPtr->refCount < 1) {
		bytes = Tcl_GetStringFromObj(globalPtr->objPtr, &length);
		panic("TclVerifyGlobalLiteralTable: global literal \"%.*s\" had bad refCount %d",
		        (length>60? 60 : length), bytes,
		        globalPtr->refCount);
	    }
	    if (globalPtr->objPtr->bytes == NULL) {
		panic("TclVerifyGlobalLiteralTable: literal has NULL string rep");
	    }
	}
    }
    if (count != globalTablePtr->numEntries) {
	panic("TclVerifyGlobalLiteralTable: global literal table had %d entries, should be %d",
	      count, globalTablePtr->numEntries);
    }
}
#endif /*TCL_COMPILE_DEBUG*/
