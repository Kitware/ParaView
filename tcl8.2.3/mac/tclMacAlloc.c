/*
 * tclMacAlloc.c --
 *
 *	This is a very fast storage allocator.  It allocates blocks of a
 *	small number of different sizes, and keeps free lists of each size.
 *	Blocks that don't exactly fit are passed up to the next larger size.
 *	Blocks over a certain size are directly allocated by calling NewPtr.
 *
 * Copyright (c) 1983 Regents of the University of California.
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * Portions contributed by Chris Kingsley, Jack Jansen and Ray Johnson
 *.
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclMacInt.h"
#include <Memory.h>
#include <stdlib.h>
#include <string.h>


/*
 * Flags that are used by ConfigureMemory to define how the allocator
 * should work.  They can be or'd together.
 */
#define MEMORY_ALL_SYS 1	/* All memory should come from the system
heap. */

/*
 * Amount of space to leave in the application heap for the Toolbox to work.
 */

#define TOOLBOX_SPACE (32 * 1024)

static int memoryFlags = 0;
static Handle toolGuardHandle = NULL;
				/* This handle must be around so that we don't
				 * have NewGWorld failures. This handle is
				 * purgeable. Before we allocate any blocks,
				 * we see if this handle is still around.
				 * If it is not, then we try to get it again.
				 * If we can get it, we lock it and try
				 * to do the normal allocation, unlocking on
				 * the way out. If we can't, we go to the
				 * system heap directly. */


/*
 * The following typedef and variable are used to keep track of memory
 * blocks that are allocated directly from the System Heap.  These chunks
 * of memory must always be freed - even if we crash.
 */

typedef struct listEl {
    Handle		memoryHandle;
    struct listEl *	next;
} ListEl;

ListEl * systemMemory = NULL;
ListEl * appMemory = NULL;

/*
 * Prototypes for functions used only in this file.
 */

static pascal void	CleanUpExitProc _ANSI_ARGS_((void));
void 			ConfigureMemory _ANSI_ARGS_((int flags));
void			FreeAllMemory _ANSI_ARGS_((void));

/*
 *----------------------------------------------------------------------
 *
 * TclpSysRealloc --
 *
 *	This function reallocates a chunk of system memory.  If the
 *	chunk is already big enough to hold the new block, then no
 *	allocation happens.
 *
 * Results:
 *	Returns a pointer to the newly allocated block.
 *
 * Side effects:
 *	May copy the contents of the original block to the new block
 *	and deallocate the original block.
 *
 *----------------------------------------------------------------------
 */

VOID *
TclpSysRealloc(
    VOID *oldPtr,		/* Original block */
    unsigned int size)		/* New size of block. */
{
    Handle hand;
    void *newPtr;
    int maxsize;

    hand = * (Handle *) ((Ptr) oldPtr - sizeof(Handle));
    maxsize = GetHandleSize(hand) - sizeof(Handle);
    if (maxsize < size) {
	newPtr = TclpSysAlloc(size, 1);
	memcpy(newPtr, oldPtr, maxsize);
	TclpSysFree(oldPtr);
    } else {
	newPtr = oldPtr;
    }
    return newPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpSysAlloc --
 *
 *	Allocate a new block of memory free from the System.
 *
 * Results:
 *	Returns a pointer to a new block of memory.
 *
 * Side effects:
 *	May obtain memory from app or sys space.  Info is added to
 *	overhead lists etc.
 *
 *----------------------------------------------------------------------
 */

VOID *
TclpSysAlloc(
    long size,		/* Size of block to allocate. */
    int isBin)		/* Is this a bin allocation? */
{
    Handle hand = NULL;
    ListEl * newMemoryRecord;

    if (!(memoryFlags & MEMORY_ALL_SYS)) {

    	/*
    	 * If the guard handle has been purged, throw it away and try
    	 * to allocate it again.
    	 */

    	if ((toolGuardHandle != NULL) && (*toolGuardHandle == NULL)) {
    	    DisposeHandle(toolGuardHandle);
    	    toolGuardHandle = NULL;
    	}

    	/*
    	 * If we have never allocated the guard handle, or it was purged
    	 * and thrown away, then try to allocate it again.
    	 */

    	if (toolGuardHandle == NULL) {
    	    toolGuardHandle = NewHandle(TOOLBOX_SPACE);
    	    if (toolGuardHandle != NULL) {
    	    	HPurge(toolGuardHandle);
    	    }
    	}

	/*
	 * If we got the handle, lock it and do our allocation.
	 */

    	if (toolGuardHandle != NULL) {
    	    HLock(toolGuardHandle);
	    hand = NewHandle(size + sizeof(Handle));
	    HUnlock(toolGuardHandle);
	}
    }
    if (hand != NULL) {
	newMemoryRecord = (ListEl *) NewPtr(sizeof(ListEl));
	if (newMemoryRecord == NULL) {
	    DisposeHandle(hand);
	    return NULL;
	}
	newMemoryRecord->memoryHandle = hand;
	newMemoryRecord->next = appMemory;
	appMemory = newMemoryRecord;
    } else {
	/*
	 * Ran out of memory in application space.  Lets try to get
	 * more memory from system.  Otherwise, we return NULL to
	 * denote failure.
	 */
	isBin = 0;
	hand = NewHandleSys(size + sizeof(Handle));
	if (hand == NULL) {
	    return NULL;
	}
	if (systemMemory == NULL) {
	    /*
	     * This is the first time we've attempted to allocate memory
	     * directly from the system heap.  We need to now install the
	     * exit handle to ensure the memory is cleaned up.
	     */
	    TclMacInstallExitToShellPatch(CleanUpExitProc);
	}
	newMemoryRecord = (ListEl *) NewPtrSys(sizeof(ListEl));
	if (newMemoryRecord == NULL) {
	    DisposeHandle(hand);
	    return NULL;
	}
	newMemoryRecord->memoryHandle = hand;
	newMemoryRecord->next = systemMemory;
	systemMemory = newMemoryRecord;
    }
    if (isBin) {
	HLockHi(hand);
    } else {
	HLock(hand);
    }
    (** (Handle **) hand) = hand;

    return (*hand + sizeof(Handle));
}

/*
 *----------------------------------------------------------------------
 *
 * TclpSysFree --
 *
 *	Free memory that we allocated back to the system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclpSysFree(
    void * ptr)		/* Free this system memory. */
{
    Handle hand;
    OSErr err;

    hand = * (Handle *) ((Ptr) ptr - sizeof(Handle));
    DisposeHandle(hand);
    *hand = NULL;
    err = MemError();
}

/*
 *----------------------------------------------------------------------
 *
 * CleanUpExitProc --
 *
 *	This procedure is invoked as an exit handler when ExitToShell
 *	is called.  It removes any memory that was allocated directly
 *	from the system heap.  This must be called when the application
 *	quits or the memory will never be freed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May free memory in the system heap.
 *
 *----------------------------------------------------------------------
 */

static pascal void
CleanUpExitProc()
{
    ListEl * memRecord;

    while (systemMemory != NULL) {
	memRecord = systemMemory;
	systemMemory = memRecord->next;
        if (*(memRecord->memoryHandle) != NULL) {
            DisposeHandle(memRecord->memoryHandle);
        }
	DisposePtr((void *) memRecord);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeAllMemory --
 *
 *	This procedure frees all memory blocks allocated by the memory
 *	sub-system.  Make sure you don't have any code that references
 *	any malloced data!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees all memory allocated by TclpAlloc.
 *
 *----------------------------------------------------------------------
 */

void
FreeAllMemory()
{
    ListEl * memRecord;

    while (systemMemory != NULL) {
	memRecord = systemMemory;
	systemMemory = memRecord->next;
	if (*(memRecord->memoryHandle) != NULL) {
            DisposeHandle(memRecord->memoryHandle);
        }
	DisposePtr((void *) memRecord);
    }
    while (appMemory != NULL) {
	memRecord = appMemory;
	appMemory = memRecord->next;
	if (*(memRecord->memoryHandle) != NULL) {
            DisposeHandle(memRecord->memoryHandle);
        }
	DisposePtr((void *) memRecord);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMemory --
 *
 *	This procedure sets certain flags in this file that control
 *	how memory is allocated and managed.  This call must be made
 *	before any call to TclpAlloc is made.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Certain state will be changed.
 *
 *----------------------------------------------------------------------
 */

void
ConfigureMemory(
    int flags)		/* Flags that control memory alloc scheme. */
{
    memoryFlags = flags;
}
