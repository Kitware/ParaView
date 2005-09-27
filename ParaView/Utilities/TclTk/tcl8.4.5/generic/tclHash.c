/* 
 * tclHash.c --
 *
 *	Implementation of in-memory hash tables for Tcl and Tcl-based
 *	applications.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"

/*
 * Prevent macros from clashing with function definitions.
 */

#if TCL_PRESERVE_BINARY_COMPATABILITY
#   undef Tcl_FindHashEntry
#   undef Tcl_CreateHashEntry
#endif

/*
 * When there are this many entries per bucket, on average, rebuild
 * the hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3

/*
 * The following macro takes a preliminary integer hash value and
 * produces an index into a hash tables bucket list.  The idea is
 * to make it so that preliminary values that are arbitrarily similar
 * will end up in different buckets.  The hash function was taken
 * from a random-number generator.
 */

#define RANDOM_INDEX(tablePtr, i) \
    (((((long) (i))*1103515245) >> (tablePtr)->downShift) & (tablePtr)->mask)

/*
 * Prototypes for the array hash key methods.
 */

static Tcl_HashEntry *	AllocArrayEntry _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));
static int		CompareArrayKeys _ANSI_ARGS_((
			    VOID *keyPtr, Tcl_HashEntry *hPtr));
static unsigned int	HashArrayKey _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));

/*
 * Prototypes for the one word hash key methods.
 */

#if 0
static Tcl_HashEntry *	AllocOneWordEntry _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));
static int		CompareOneWordKeys _ANSI_ARGS_((
			    VOID *keyPtr, Tcl_HashEntry *hPtr));
static unsigned int	HashOneWordKey _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));
#endif

/*
 * Prototypes for the string hash key methods.
 */

static Tcl_HashEntry *	AllocStringEntry _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));
static int		CompareStringKeys _ANSI_ARGS_((
			    VOID *keyPtr, Tcl_HashEntry *hPtr));
static unsigned int	HashStringKey _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));

/*
 * Procedure prototypes for static procedures in this file:
 */

#if TCL_PRESERVE_BINARY_COMPATABILITY
static Tcl_HashEntry *	BogusFind _ANSI_ARGS_((Tcl_HashTable *tablePtr,
			    CONST char *key));
static Tcl_HashEntry *	BogusCreate _ANSI_ARGS_((Tcl_HashTable *tablePtr,
			    CONST char *key, int *newPtr));
#endif

static void		RebuildTable _ANSI_ARGS_((Tcl_HashTable *tablePtr));

Tcl_HashKeyType tclArrayHashKeyType = {
    TCL_HASH_KEY_TYPE_VERSION,		/* version */
    TCL_HASH_KEY_RANDOMIZE_HASH,	/* flags */
    HashArrayKey,			/* hashKeyProc */
    CompareArrayKeys,			/* compareKeysProc */
    AllocArrayEntry,			/* allocEntryProc */
    NULL				/* freeEntryProc */
};

Tcl_HashKeyType tclOneWordHashKeyType = {
    TCL_HASH_KEY_TYPE_VERSION,		/* version */
    0,					/* flags */
    NULL, /* HashOneWordKey, */		/* hashProc */
    NULL, /* CompareOneWordKey, */	/* compareProc */
    NULL, /* AllocOneWordKey, */	/* allocEntryProc */
    NULL  /* FreeOneWordKey, */		/* freeEntryProc */
};

Tcl_HashKeyType tclStringHashKeyType = {
    TCL_HASH_KEY_TYPE_VERSION,		/* version */
    0,					/* flags */
    HashStringKey,			/* hashKeyProc */
    CompareStringKeys,			/* compareKeysProc */
    AllocStringEntry,			/* allocEntryProc */
    NULL				/* freeEntryProc */
};


/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare
 *	the hash table for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to Tcl_FindHashEntry and
 *	Tcl_CreateHashEntry.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_InitHashTable
void
Tcl_InitHashTable(tablePtr, keyType)
    register Tcl_HashTable *tablePtr;	/* Pointer to table record, which
					 * is supplied by the caller. */
    int keyType;			/* Type of keys to use in table:
					 * TCL_STRING_KEYS, TCL_ONE_WORD_KEYS,
					 * or an integer >= 2. */
{
    /*
     * Use a special value to inform the extended version that it must
     * not access any of the new fields in the Tcl_HashTable. If an
     * extension is rebuilt then any calls to this function will be
     * redirected to the extended version by a macro.
     */
    Tcl_InitCustomHashTable(tablePtr, keyType, (Tcl_HashKeyType *) -1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitCustomHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare
 *	the hash table for use. This is an extended version of
 *	Tcl_InitHashTable which supports user defined keys.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to Tcl_FindHashEntry and
 *	Tcl_CreateHashEntry.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_InitCustomHashTable(tablePtr, keyType, typePtr)
    register Tcl_HashTable *tablePtr;	/* Pointer to table record, which
					 * is supplied by the caller. */
    int keyType;			/* Type of keys to use in table:
					 * TCL_STRING_KEYS, TCL_ONE_WORD_KEYS,
					 * TCL_CUSTOM_TYPE_KEYS,
					 * TCL_CUSTOM_PTR_KEYS,  or an
					 * integer >= 2. */
    Tcl_HashKeyType *typePtr;		/* Pointer to structure which defines
					 * the behaviour of this table. */
{
#if (TCL_SMALL_HASH_TABLE != 4) 
    panic("Tcl_InitCustomHashTable: TCL_SMALL_HASH_TABLE is %d, not 4\n",
	    TCL_SMALL_HASH_TABLE);
#endif
    
    tablePtr->buckets = tablePtr->staticBuckets;
    tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
    tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
    tablePtr->numBuckets = TCL_SMALL_HASH_TABLE;
    tablePtr->numEntries = 0;
    tablePtr->rebuildSize = TCL_SMALL_HASH_TABLE*REBUILD_MULTIPLIER;
    tablePtr->downShift = 28;
    tablePtr->mask = 3;
    tablePtr->keyType = keyType;
#if TCL_PRESERVE_BINARY_COMPATABILITY
    tablePtr->findProc = Tcl_FindHashEntry;
    tablePtr->createProc = Tcl_CreateHashEntry;

    if (typePtr == NULL) {
	/*
	 * The caller has been rebuilt so the hash table is an extended
	 * version.
	 */
    } else if (typePtr != (Tcl_HashKeyType *) -1) {
	/*
	 * The caller is requesting a customized hash table so it must be
	 * an extended version.
	 */
	tablePtr->typePtr = typePtr;
    } else {
	/*
	 * The caller has not been rebuilt so the hash table is not
	 * extended.
	 */
    }
#else
    if (typePtr == NULL) {
	/*
	 * Use the key type to decide which key type is needed.
	 */
	if (keyType == TCL_STRING_KEYS) {
	    typePtr = &tclStringHashKeyType;
	} else if (keyType == TCL_ONE_WORD_KEYS) {
	    typePtr = &tclOneWordHashKeyType;
	} else if (keyType == TCL_CUSTOM_TYPE_KEYS) {
	    Tcl_Panic ("No type structure specified for TCL_CUSTOM_TYPE_KEYS");
	} else if (keyType == TCL_CUSTOM_PTR_KEYS) {
	    Tcl_Panic ("No type structure specified for TCL_CUSTOM_PTR_KEYS");
	} else {
	    typePtr = &tclArrayHashKeyType;
	}
    } else if (typePtr == (Tcl_HashKeyType *) -1) {
	/*
	 * If the caller has not been rebuilt then we cannot continue as
	 * the hash table is not an extended version.
	 */
	Tcl_Panic ("Hash table is not compatible");
    }
    tablePtr->typePtr = typePtr;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FindHashEntry --
 *
 *	Given a hash table find the entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the
 *	hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_FindHashEntry(tablePtr, key)
    Tcl_HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find matching entry. */
{
    register Tcl_HashEntry *hPtr;
    Tcl_HashKeyType *typePtr;
    unsigned int hash;
    int index;

#if TCL_PRESERVE_BINARY_COMPATABILITY
    if (tablePtr->keyType == TCL_STRING_KEYS) {
	typePtr = &tclStringHashKeyType;
    } else if (tablePtr->keyType == TCL_ONE_WORD_KEYS) {
	typePtr = &tclOneWordHashKeyType;
    } else if (tablePtr->keyType == TCL_CUSTOM_TYPE_KEYS
	       || tablePtr->keyType == TCL_CUSTOM_PTR_KEYS) {
	typePtr = tablePtr->typePtr;
    } else {
	typePtr = &tclArrayHashKeyType;
    }
#else
    typePtr = tablePtr->typePtr;
    if (typePtr == NULL) {
	Tcl_Panic("called Tcl_FindHashEntry on deleted table");
	return NULL;
    }
#endif

    if (typePtr->hashKeyProc) {
	hash = typePtr->hashKeyProc (tablePtr, (VOID *) key);
	if (typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
	    index = RANDOM_INDEX (tablePtr, hash);
	} else {
	    index = hash & tablePtr->mask;
	}
    } else {
	hash = (unsigned int) key;
	index = RANDOM_INDEX (tablePtr, hash);
    }

    /*
     * Search all of the entries in the appropriate bucket.
     */

    if (typePtr->compareKeysProc) {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
#if TCL_HASH_KEY_STORE_HASH
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
#endif
	    if (typePtr->compareKeysProc ((VOID *) key, hPtr)) {
		return hPtr;
	    }
	}
    } else {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
#if TCL_HASH_KEY_STORE_HASH
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
#endif
	    if (key == hPtr->key.oneWordValue) {
		return hPtr;
	    }
	}
    }
    
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateHashEntry --
 *
 *	Given a hash table with string keys, and a string key, find
 *	the entry with a matching key.  If there is no matching entry,
 *	then create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this
 *	is a newly-created entry, then *newPtr will be set to a non-zero
 *	value;  otherwise *newPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_CreateHashEntry(tablePtr, key, newPtr)
    Tcl_HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find or create matching
				 * entry. */
    int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
{
    register Tcl_HashEntry *hPtr;
    Tcl_HashKeyType *typePtr;
    unsigned int hash;
    int index;

#if TCL_PRESERVE_BINARY_COMPATABILITY
    if (tablePtr->keyType == TCL_STRING_KEYS) {
	typePtr = &tclStringHashKeyType;
    } else if (tablePtr->keyType == TCL_ONE_WORD_KEYS) {
	typePtr = &tclOneWordHashKeyType;
    } else if (tablePtr->keyType == TCL_CUSTOM_TYPE_KEYS
	       || tablePtr->keyType == TCL_CUSTOM_PTR_KEYS) {
	typePtr = tablePtr->typePtr;
    } else {
	typePtr = &tclArrayHashKeyType;
    }
#else
    typePtr = tablePtr->typePtr;
    if (typePtr == NULL) {
	Tcl_Panic("called Tcl_CreateHashEntry on deleted table");
	return NULL;
    }
#endif

    if (typePtr->hashKeyProc) {
	hash = typePtr->hashKeyProc (tablePtr, (VOID *) key);
	if (typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
	    index = RANDOM_INDEX (tablePtr, hash);
	} else {
	    index = hash & tablePtr->mask;
	}
    } else {
	hash = (unsigned int) key;
	index = RANDOM_INDEX (tablePtr, hash);
    }

    /*
     * Search all of the entries in the appropriate bucket.
     */

    if (typePtr->compareKeysProc) {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
#if TCL_HASH_KEY_STORE_HASH
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
#endif
	    if (typePtr->compareKeysProc ((VOID *) key, hPtr)) {
		*newPtr = 0;
		return hPtr;
	    }
	}
    } else {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
#if TCL_HASH_KEY_STORE_HASH
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
#endif
	    if (key == hPtr->key.oneWordValue) {
		*newPtr = 0;
		return hPtr;
	    }
	}
    }

    /*
     * Entry not found.  Add a new one to the bucket.
     */

    *newPtr = 1;
    if (typePtr->allocEntryProc) {
	hPtr = typePtr->allocEntryProc (tablePtr, (VOID *) key);
    } else {
	hPtr = (Tcl_HashEntry *) ckalloc((unsigned) sizeof(Tcl_HashEntry));
	hPtr->key.oneWordValue = (char *) key;
    }
					 
    hPtr->tablePtr = tablePtr;
#if TCL_HASH_KEY_STORE_HASH
#   if TCL_PRESERVE_BINARY_COMPATABILITY
    hPtr->hash = (VOID *) hash;
#   else
    hPtr->hash = hash;
#   endif
    hPtr->nextPtr = tablePtr->buckets[index];
    tablePtr->buckets[index] = hPtr;
#else
    hPtr->bucketPtr = &(tablePtr->buckets[index]);
    hPtr->nextPtr = *hPtr->bucketPtr;
    *hPtr->bucketPtr = hPtr;
#endif
    hPtr->clientData = 0;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many
     * more buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
	RebuildTable(tablePtr);
    }
    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteHashEntry --
 *
 *	Remove a single entry from a hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entry given by entryPtr is deleted from its table and
 *	should never again be used by the caller.  It is up to the
 *	caller to free the clientData field of the entry, if that
 *	is relevant.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteHashEntry(entryPtr)
    Tcl_HashEntry *entryPtr;
{
    register Tcl_HashEntry *prevPtr;
    Tcl_HashKeyType *typePtr;
    Tcl_HashTable *tablePtr;
    Tcl_HashEntry **bucketPtr;
#if TCL_HASH_KEY_STORE_HASH
    int index;
#endif

    tablePtr = entryPtr->tablePtr;

#if TCL_PRESERVE_BINARY_COMPATABILITY
    if (tablePtr->keyType == TCL_STRING_KEYS) {
	typePtr = &tclStringHashKeyType;
    } else if (tablePtr->keyType == TCL_ONE_WORD_KEYS) {
	typePtr = &tclOneWordHashKeyType;
    } else if (tablePtr->keyType == TCL_CUSTOM_TYPE_KEYS
	       || tablePtr->keyType == TCL_CUSTOM_PTR_KEYS) {
	typePtr = tablePtr->typePtr;
    } else {
	typePtr = &tclArrayHashKeyType;
    }
#else
    typePtr = tablePtr->typePtr;
#endif
    
#if TCL_HASH_KEY_STORE_HASH
    if (typePtr->hashKeyProc == NULL
	|| typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
	index = RANDOM_INDEX (tablePtr, entryPtr->hash);
    } else {
	index = ((unsigned int) entryPtr->hash) & tablePtr->mask;
    }

    bucketPtr = &(tablePtr->buckets[index]);
#else
    bucketPtr = entryPtr->bucketPtr;
#endif
    
    if (*bucketPtr == entryPtr) {
	*bucketPtr = entryPtr->nextPtr;
    } else {
	for (prevPtr = *bucketPtr; ; prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		panic("malformed bucket chain in Tcl_DeleteHashEntry");
	    }
	    if (prevPtr->nextPtr == entryPtr) {
		prevPtr->nextPtr = entryPtr->nextPtr;
		break;
	    }
	}
    }

    tablePtr->numEntries--;
    if (typePtr->freeEntryProc) {
	typePtr->freeEntryProc (entryPtr);
    } else {
	ckfree((char *) entryPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteHashTable --
 *
 *	Free up everything associated with a hash table except for
 *	the record for the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is no longer useable.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteHashTable(tablePtr)
    register Tcl_HashTable *tablePtr;		/* Table to delete. */
{
    register Tcl_HashEntry *hPtr, *nextPtr;
    Tcl_HashKeyType *typePtr;
    int i;

#if TCL_PRESERVE_BINARY_COMPATABILITY
    if (tablePtr->keyType == TCL_STRING_KEYS) {
	typePtr = &tclStringHashKeyType;
    } else if (tablePtr->keyType == TCL_ONE_WORD_KEYS) {
	typePtr = &tclOneWordHashKeyType;
    } else if (tablePtr->keyType == TCL_CUSTOM_TYPE_KEYS
	       || tablePtr->keyType == TCL_CUSTOM_PTR_KEYS) {
	typePtr = tablePtr->typePtr;
    } else {
	typePtr = &tclArrayHashKeyType;
    }
#else
    typePtr = tablePtr->typePtr;
#endif

    /*
     * Free up all the entries in the table.
     */

    for (i = 0; i < tablePtr->numBuckets; i++) {
	hPtr = tablePtr->buckets[i];
	while (hPtr != NULL) {
	    nextPtr = hPtr->nextPtr;
	    if (typePtr->freeEntryProc) {
		typePtr->freeEntryProc (hPtr);
	    } else {
		ckfree((char *) hPtr);
	    }
	    hPtr = nextPtr;
	}
    }

    /*
     * Free up the bucket array, if it was dynamically allocated.
     */

    if (tablePtr->buckets != tablePtr->staticBuckets) {
	ckfree((char *) tablePtr->buckets);
    }

    /*
     * Arrange for panics if the table is used again without
     * re-initialization.
     */

#if TCL_PRESERVE_BINARY_COMPATABILITY
    tablePtr->findProc = BogusFind;
    tablePtr->createProc = BogusCreate;
#else
    tablePtr->typePtr = NULL;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FirstHashEntry --
 *
 *	Locate the first entry in a hash table and set up a record
 *	that can be used to step through all the remaining entries
 *	of the table.
 *
 * Results:
 *	The return value is a pointer to the first entry in tablePtr,
 *	or NULL if tablePtr has no entries in it.  The memory at
 *	*searchPtr is initialized so that subsequent calls to
 *	Tcl_NextHashEntry will return all of the entries in the table,
 *	one at a time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_FirstHashEntry(tablePtr, searchPtr)
    Tcl_HashTable *tablePtr;		/* Table to search. */
    Tcl_HashSearch *searchPtr;		/* Place to store information about
					 * progress through the table. */
{
    searchPtr->tablePtr = tablePtr;
    searchPtr->nextIndex = 0;
    searchPtr->nextEntryPtr = NULL;
    return Tcl_NextHashEntry(searchPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NextHashEntry --
 *
 *	Once a hash table enumeration has been initiated by calling
 *	Tcl_FirstHashEntry, this procedure may be called to return
 *	successive elements of the table.
 *
 * Results:
 *	The return value is the next entry in the hash table being
 *	enumerated, or NULL if the end of the table is reached.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_NextHashEntry(searchPtr)
    register Tcl_HashSearch *searchPtr;	/* Place to store information about
					 * progress through the table.  Must
					 * have been initialized by calling
					 * Tcl_FirstHashEntry. */
{
    Tcl_HashEntry *hPtr;

    while (searchPtr->nextEntryPtr == NULL) {
	if (searchPtr->nextIndex >= searchPtr->tablePtr->numBuckets) {
	    return NULL;
	}
	searchPtr->nextEntryPtr =
		searchPtr->tablePtr->buckets[searchPtr->nextIndex];
	searchPtr->nextIndex++;
    }
    hPtr = searchPtr->nextEntryPtr;
    searchPtr->nextEntryPtr = hPtr->nextPtr;
    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_HashStats --
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

CONST char *
Tcl_HashStats(tablePtr)
    Tcl_HashTable *tablePtr;		/* Table for which to produce stats. */
{
#define NUM_COUNTERS 10
    int count[NUM_COUNTERS], overflow, i, j;
    double average, tmp;
    register Tcl_HashEntry *hPtr;
    char *result, *p;

    /*
     * Compute a histogram of bucket usage.
     */

    for (i = 0; i < NUM_COUNTERS; i++) {
	count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    for (i = 0; i < tablePtr->numBuckets; i++) {
	j = 0;
	for (hPtr = tablePtr->buckets[i]; hPtr != NULL; hPtr = hPtr->nextPtr) {
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

/*
 *----------------------------------------------------------------------
 *
 * AllocArrayEntry --
 *
 *	Allocate space for a Tcl_HashEntry containing the array key.
 *
 * Results:
 *	The return value is a pointer to the created entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_HashEntry *
AllocArrayEntry(tablePtr, keyPtr)
    Tcl_HashTable *tablePtr;	/* Hash table. */
    VOID *keyPtr;		/* Key to store in the hash table entry. */
{
    int *array = (int *) keyPtr;
    register int *iPtr1, *iPtr2;
    Tcl_HashEntry *hPtr;
    int count;
    unsigned int size;

    count = tablePtr->keyType;
    
    size = sizeof(Tcl_HashEntry) + (count*sizeof(int)) - sizeof(hPtr->key);
    if (size < sizeof(Tcl_HashEntry))
	size = sizeof(Tcl_HashEntry);
    hPtr = (Tcl_HashEntry *) ckalloc(size);
    
    for (iPtr1 = array, iPtr2 = hPtr->key.words;
	    count > 0; count--, iPtr1++, iPtr2++) {
	*iPtr2 = *iPtr1;
    }

    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * CompareArrayKeys --
 *
 *	Compares two array keys.
 *
 * Results:
 *	The return value is 0 if they are different and 1 if they are
 *	the same.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CompareArrayKeys(keyPtr, hPtr)
    VOID *keyPtr;		/* New key to compare. */
    Tcl_HashEntry *hPtr;	/* Existing key to compare. */
{
    register CONST int *iPtr1 = (CONST int *) keyPtr;
    register CONST int *iPtr2 = (CONST int *) hPtr->key.words;
    Tcl_HashTable *tablePtr = hPtr->tablePtr;
    int count;

    for (count = tablePtr->keyType; ; count--, iPtr1++, iPtr2++) {
	if (count == 0) {
	    return 1;
	}
	if (*iPtr1 != *iPtr2) {
	    break;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * HashArrayKey --
 *
 *	Compute a one-word summary of an array, which can be
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
HashArrayKey(tablePtr, keyPtr)
    Tcl_HashTable *tablePtr;	/* Hash table. */
    VOID *keyPtr;		/* Key from which to compute hash value. */
{
    register CONST int *array = (CONST int *) keyPtr;
    register unsigned int result;
    int count;

    for (result = 0, count = tablePtr->keyType; count > 0;
	    count--, array++) {
	result += *array;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * AllocStringEntry --
 *
 *	Allocate space for a Tcl_HashEntry containing the string key.
 *
 * Results:
 *	The return value is a pointer to the created entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_HashEntry *
AllocStringEntry(tablePtr, keyPtr)
    Tcl_HashTable *tablePtr;	/* Hash table. */
    VOID *keyPtr;		/* Key to store in the hash table entry. */
{
    CONST char *string = (CONST char *) keyPtr;
    Tcl_HashEntry *hPtr;
    unsigned int size;

    size = sizeof(Tcl_HashEntry) + strlen(string) + 1 - sizeof(hPtr->key);
    if (size < sizeof(Tcl_HashEntry))
	size = sizeof(Tcl_HashEntry);
    hPtr = (Tcl_HashEntry *) ckalloc(size);
    strcpy(hPtr->key.string, string);

    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * CompareStringKeys --
 *
 *	Compares two string keys.
 *
 * Results:
 *	The return value is 0 if they are different and 1 if they are
 *	the same.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CompareStringKeys(keyPtr, hPtr)
    VOID *keyPtr;		/* New key to compare. */
    Tcl_HashEntry *hPtr;		/* Existing key to compare. */
{
    register CONST char *p1 = (CONST char *) keyPtr;
    register CONST char *p2 = (CONST char *) hPtr->key.string;

    for (;; p1++, p2++) {
	if (*p1 != *p2) {
	    break;
	}
	if (*p1 == '\0') {
	    return 1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * HashStringKey --
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
HashStringKey(tablePtr, keyPtr)
    Tcl_HashTable *tablePtr;	/* Hash table. */
    VOID *keyPtr;		/* Key from which to compute hash value. */
{
    register CONST char *string = (CONST char *) keyPtr;
    register unsigned int result;
    register int c;

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
    while (1) {
	c = *string;
	if (c == 0) {
	    break;
	}
	result += (result<<3) + c;
	string++;
    }
    return result;
}

#if TCL_PRESERVE_BINARY_COMPATABILITY
/*
 *----------------------------------------------------------------------
 *
 * BogusFind --
 *
 *	This procedure is invoked when an Tcl_FindHashEntry is called
 *	on a table that has been deleted.
 *
 * Results:
 *	If panic returns (which it shouldn't) this procedure returns
 *	NULL.
 *
 * Side effects:
 *	Generates a panic.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static Tcl_HashEntry *
BogusFind(tablePtr, key)
    Tcl_HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find matching entry. */
{
    panic("called Tcl_FindHashEntry on deleted table");
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * BogusCreate --
 *
 *	This procedure is invoked when an Tcl_CreateHashEntry is called
 *	on a table that has been deleted.
 *
 * Results:
 *	If panic returns (which it shouldn't) this procedure returns
 *	NULL.
 *
 * Side effects:
 *	Generates a panic.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static Tcl_HashEntry *
BogusCreate(tablePtr, key, newPtr)
    Tcl_HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find or create matching
				 * entry. */
    int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
{
    panic("called Tcl_CreateHashEntry on deleted table");
    return NULL;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *	This procedure is invoked when the ratio of entries to hash
 *	buckets becomes too large.  It creates a new table with a
 *	larger bucket array and moves all of the entries into the
 *	new table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get re-hashed to new
 *	buckets.
 *
 *----------------------------------------------------------------------
 */

static void
RebuildTable(tablePtr)
    register Tcl_HashTable *tablePtr;	/* Table to enlarge. */
{
    int oldSize, count, index;
    Tcl_HashEntry **oldBuckets;
    register Tcl_HashEntry **oldChainPtr, **newChainPtr;
    register Tcl_HashEntry *hPtr;
    Tcl_HashKeyType *typePtr;
    VOID *key;

    oldSize = tablePtr->numBuckets;
    oldBuckets = tablePtr->buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */

    tablePtr->numBuckets *= 4;
    tablePtr->buckets = (Tcl_HashEntry **) ckalloc((unsigned)
	    (tablePtr->numBuckets * sizeof(Tcl_HashEntry *)));
    for (count = tablePtr->numBuckets, newChainPtr = tablePtr->buckets;
	    count > 0; count--, newChainPtr++) {
	*newChainPtr = NULL;
    }
    tablePtr->rebuildSize *= 4;
    tablePtr->downShift -= 2;
    tablePtr->mask = (tablePtr->mask << 2) + 3;

#if TCL_PRESERVE_BINARY_COMPATABILITY
    if (tablePtr->keyType == TCL_STRING_KEYS) {
	typePtr = &tclStringHashKeyType;
    } else if (tablePtr->keyType == TCL_ONE_WORD_KEYS) {
	typePtr = &tclOneWordHashKeyType;
    } else if (tablePtr->keyType == TCL_CUSTOM_TYPE_KEYS
	       || tablePtr->keyType == TCL_CUSTOM_PTR_KEYS) {
	typePtr = tablePtr->typePtr;
    } else {
	typePtr = &tclArrayHashKeyType;
    }
#else
    typePtr = tablePtr->typePtr;
#endif

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    for (oldChainPtr = oldBuckets; oldSize > 0; oldSize--, oldChainPtr++) {
	for (hPtr = *oldChainPtr; hPtr != NULL; hPtr = *oldChainPtr) {
	    *oldChainPtr = hPtr->nextPtr;

	    key = (VOID *) Tcl_GetHashKey (tablePtr, hPtr);

#if TCL_HASH_KEY_STORE_HASH
	    if (typePtr->hashKeyProc == NULL
		|| typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
		index = RANDOM_INDEX (tablePtr, hPtr->hash);
	    } else {
		index = ((unsigned int) hPtr->hash) & tablePtr->mask;
	    }
	    hPtr->nextPtr = tablePtr->buckets[index];
	    tablePtr->buckets[index] = hPtr;
#else
	    if (typePtr->hashKeyProc) {
		unsigned int hash;
		hash = typePtr->hashKeyProc (tablePtr, (VOID *) key);
		if (typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
		    index = RANDOM_INDEX (tablePtr, hash);
		} else {
		    index = hash & tablePtr->mask;
		}
	    } else {
		index = RANDOM_INDEX (tablePtr, key);
	    }

	    hPtr->bucketPtr = &(tablePtr->buckets[index]);
	    hPtr->nextPtr = *hPtr->bucketPtr;
	    *hPtr->bucketPtr = hPtr;
#endif
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != tablePtr->staticBuckets) {
	ckfree((char *) oldBuckets);
    }
}
