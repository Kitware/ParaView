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
#include <Gestalt.h>
#include <stdlib.h>
#include <string.h>


/*
 * Flags that are used by ConfigureMemory to define how the allocator
 * should work.  They can be or'd together.
 */
#define MEMORY_ALL_SYS 1	/* All memory should come from the system
heap. */
#define MEMORY_DONT_USE_TEMPMEM 2	/* Don't use temporary memory but system memory. */

/*
 * Amount of space to leave in the application heap for the Toolbox to work.
 */

#define TOOLBOX_SPACE (512 * 1024)

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

static int tclUseMemTracking = 0; /* Are we tracking memory allocations?
								   * On recent versions of the MacOS this
								   * is no longer necessary, as we can use
								   * temporary memory which is freed by the
								   * OS after a quit or crash. */
								   
static size_t tclExtraHdlSize = 0; /* Size of extra memory allocated at the start
									* of each block when using memory tracking
									* ( == 0 otherwise) */

/*
 * The following typedef and variable are used to keep track of memory
 * blocks that are allocated directly from the System Heap.  These chunks
 * of memory must always be freed - even if we crash.
 */

typedef struct listEl {
    Handle		memoryHandle;
    struct listEl *	next;
    struct listEl *	prec;
} ListEl;

static ListEl * systemMemory = NULL;
static ListEl * appMemory = NULL;

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
    OSErr err;

	if (tclUseMemTracking) {
    hand = ((ListEl *) ((Ptr) oldPtr - tclExtraHdlSize))->memoryHandle;
    } else {
    hand = RecoverHandle((Ptr) oldPtr);
	}
    maxsize = GetHandleSize(hand) - sizeof(Handle);
    if (maxsize < size) {
    HUnlock(hand);
    SetHandleSize(hand,size + tclExtraHdlSize);
    err = MemError();
    HLock(hand);
    if(err==noErr){
    	newPtr=(*hand + tclExtraHdlSize);
    } else {
	newPtr = TclpSysAlloc(size, 1);
	if(newPtr!=NULL) {
	memmove(newPtr, oldPtr, maxsize);
	TclpSysFree(oldPtr);
	}
	}
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
	int isSysMem = 0;
	static int initialized=0;
	
	if (!initialized) {
	long response = 0;
	OSErr err = noErr;
	int useTempMem = 0;
	
	/* Check if we can use temporary memory */
	initialized=1;
	err = Gestalt(gestaltOSAttr, &response);
	if (err == noErr) {
    	useTempMem = response & (1 << gestaltRealTempMemory);
	}
	tclUseMemTracking = !useTempMem || (memoryFlags & MEMORY_DONT_USE_TEMPMEM);
	if(tclUseMemTracking) {
	    tclExtraHdlSize = sizeof(ListEl);
	    /*
	     * We are allocating memory directly from the system
	     * heap. We need to install an exit handle 
	     * to ensure the memory is cleaned up.
	     */
	    TclMacInstallExitToShellPatch(CleanUpExitProc);
	}
	}

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
    	    	HLock(toolGuardHandle);
    	    	HPurge(toolGuardHandle);
    	    }
    	}

	/*
	 * If we got the handle, lock it and do our allocation.
	 */

    	if (toolGuardHandle != NULL) {
    	    HLock(toolGuardHandle);
	    hand = NewHandle(size + tclExtraHdlSize);
	    HUnlock(toolGuardHandle);
	}
    }
    if (hand == NULL) {
	/*
	 * Ran out of memory in application space.  Lets try to get
	 * more memory from system.  Otherwise, we return NULL to
	 * denote failure.
	 */
	if(!tclUseMemTracking) {
		/* Use Temporary Memory instead of System Heap when available */
		OSErr err;
		isBin = 1; /* always HLockHi TempMemHandles */
		hand = TempNewHandle(size + tclExtraHdlSize,&err);
		if(err!=noErr) { hand=NULL; }
	} else {
	/* Use system heap when tracking memory */
	isSysMem=1;
	isBin = 0;
	hand = NewHandleSys(size + tclExtraHdlSize);
	}
	}
	if (hand == NULL) {
	    return NULL;
	}
    if (isBin) {
	HLockHi(hand);
    } else {
	HLock(hand);
    }
	if(tclUseMemTracking) {
	/* Only need to do this when tracking memory */
	newMemoryRecord = (ListEl *) *hand;
	newMemoryRecord->memoryHandle = hand;
	newMemoryRecord->prec = NULL;
	if(isSysMem) {
	newMemoryRecord->next = systemMemory;
	systemMemory = newMemoryRecord;
	} else {
	newMemoryRecord->next = appMemory;
	appMemory = newMemoryRecord;
	}
	if(newMemoryRecord->next!=NULL) {
	newMemoryRecord->next->prec=newMemoryRecord;
	}
	}
	
    return (*hand + tclExtraHdlSize);
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
	if(tclUseMemTracking) {
    /* Only need to do this when tracking memory */
    ListEl *memRecord;

    memRecord = (ListEl *) ((Ptr) ptr - tclExtraHdlSize);
    /* Remove current record from linked list */
    if(memRecord->next!=NULL) {
    	memRecord->next->prec=memRecord->prec;
    }
    if(memRecord->prec!=NULL) {
    	memRecord->prec->next=memRecord->next;
    }
    if(memRecord==appMemory) {
    	appMemory=memRecord->next;
    } else if(memRecord==systemMemory) {
    	systemMemory=memRecord->next;
    }
    DisposeHandle(memRecord->memoryHandle);
	} else {
    DisposeHandle(RecoverHandle((Ptr) ptr));
	}
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

    if(tclUseMemTracking) {
    /* Only need to do this when tracking memory */
    while (systemMemory != NULL) {
	memRecord = systemMemory;
	systemMemory = memRecord->next;
	DisposeHandle(memRecord->memoryHandle);
    }
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

	if(tclUseMemTracking) {
	/* Only need to do this when tracking memory */
    while (systemMemory != NULL) {
	memRecord = systemMemory;
	systemMemory = memRecord->next;
	DisposeHandle(memRecord->memoryHandle);
    }
    while (appMemory != NULL) {
	memRecord = appMemory;
	appMemory = memRecord->next;
	DisposeHandle(memRecord->memoryHandle);
	}
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
