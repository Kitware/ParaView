/* 
 * tclWinThread.c --
 *
 *	This file implements the Windows-specific thread operations.
 *
 * Copyright (c) 1998 by Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

/*
 * This is the master lock used to serialize access to other
 * serialization data structures.
 */

static CRITICAL_SECTION masterLock;
static int init = 0;
#define MASTER_LOCK  EnterCriticalSection(&masterLock)
#define MASTER_UNLOCK  LeaveCriticalSection(&masterLock)

/*
 * This is the master lock used to serialize initialization and finalization
 * of Tcl as a whole.
 */

static CRITICAL_SECTION initLock;

/*
 * allocLock is used by Tcl's version of malloc for synchronization.
 * For obvious reasons, cannot use any dyamically allocated storage.
 */

#ifdef TCL_THREADS

static CRITICAL_SECTION allocLock;
static Tcl_Mutex allocLockPtr = (Tcl_Mutex) &allocLock;

#endif /* TCL_THREADS */

/*
 * The joinLock serializes Create- and ExitThread. This is necessary to
 * prevent a race where a new joinable thread exits before the creating
 * thread had the time to create the necessary data structures in the
 * emulation layer.
 */

static CRITICAL_SECTION joinLock;

/*
 * Condition variables are implemented with a combination of a 
 * per-thread Windows Event and a per-condition waiting queue.
 * The idea is that each thread has its own Event that it waits
 * on when it is doing a ConditionWait; it uses the same event for
 * all condition variables because it only waits on one at a time.
 * Each condition variable has a queue of waiting threads, and a 
 * mutex used to serialize access to this queue.
 *
 * Special thanks to David Nichols and
 * Jim Davidson for advice on the Condition Variable implementation.
 */

/*
 * The per-thread event and queue pointers.
 */

#ifdef TCL_THREADS

typedef struct ThreadSpecificData {
    HANDLE condEvent;			/* Per-thread condition event */
    struct ThreadSpecificData *nextPtr;	/* Queue pointers */
    struct ThreadSpecificData *prevPtr;
    int flags;				/* See flags below */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

#endif /* TCL_THREADS */

/*
 * State bits for the thread.
 * WIN_THREAD_UNINIT		Uninitialized.  Must be zero because
 *				of the way ThreadSpecificData is created.
 * WIN_THREAD_RUNNING		Running, not waiting.
 * WIN_THREAD_BLOCKED		Waiting, or trying to wait.
 * WIN_THREAD_DEAD		Dying - no per-thread event anymore.
 */ 

#define WIN_THREAD_UNINIT	0x0
#define WIN_THREAD_RUNNING	0x1
#define WIN_THREAD_BLOCKED	0x2
#define WIN_THREAD_DEAD		0x4

/*
 * The per condition queue pointers and the
 * Mutex used to serialize access to the queue.
 */

typedef struct WinCondition {
    CRITICAL_SECTION condLock;	/* Lock to serialize queuing on the condition */
    struct ThreadSpecificData *firstPtr;	/* Queue pointers */
    struct ThreadSpecificData *lastPtr;
} WinCondition;


/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateThread --
 *
 *	This procedure creates a new thread.
 *
 * Results:
 *	TCL_OK if the thread could be created.  The thread ID is
 *	returned in a parameter.
 *
 * Side effects:
 *	A new thread is created.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CreateThread(idPtr, proc, clientData, stackSize, flags)
    Tcl_ThreadId *idPtr;		/* Return, the ID of the thread */
    Tcl_ThreadCreateProc proc;		/* Main() function of the thread */
    ClientData clientData;		/* The one argument to Main() */
    int stackSize;			/* Size of stack for the new thread */
    int flags;				/* Flags controlling behaviour of
					 * the new thread */
{
    HANDLE tHandle;

    EnterCriticalSection(&joinLock);

#if defined(_MSC_VER) || defined(__MSVCRT__) || defined(__BORLANDC__)
    tHandle = (HANDLE) _beginthreadex(NULL, (unsigned) stackSize, proc,
	clientData, 0, (unsigned *)idPtr);
#else
    tHandle = CreateThread(NULL, (DWORD) stackSize,
	    (LPTHREAD_START_ROUTINE) proc, (LPVOID) clientData,
	    (DWORD) 0, (LPDWORD)idPtr);
#endif

    if (tHandle == NULL) {
        LeaveCriticalSection(&joinLock);
	return TCL_ERROR;
    } else {
        if (flags & TCL_THREAD_JOINABLE) {
	    TclRememberJoinableThread (*idPtr);
	}

	/*
	 * The only purpose of this is to decrement the reference count so the
	 * OS resources will be reaquired when the thread closes.
	 */

	CloseHandle(tHandle);
	LeaveCriticalSection(&joinLock);
	return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_JoinThread --
 *
 *	This procedure waits upon the exit of the specified thread.
 *
 * Results:
 *	TCL_OK if the wait was successful, TCL_ERROR else.
 *
 * Side effects:
 *	The result area is set to the exit code of the thread we
 *	waited upon.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_JoinThread(threadId, result)
    Tcl_ThreadId threadId;  /* Id of the thread to wait upon */
    int*     result;	    /* Reference to the storage the result
			     * of the thread we wait upon will be
			     * written into. */
{
    return TclJoinThread (threadId, result);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpThreadExit --
 *
 *	This procedure terminates the current thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This procedure terminates the current thread.
 *
 *----------------------------------------------------------------------
 */

void
TclpThreadExit(status)
    int status;
{
    EnterCriticalSection(&joinLock);
    TclSignalExitThread (Tcl_GetCurrentThread (), status);
    LeaveCriticalSection(&joinLock);

#if defined(_MSC_VER) || defined(__MSVCRT__) || defined(__BORLANDC__)
    _endthreadex((unsigned) status);
#else
    ExitThread((DWORD) status);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCurrentThread --
 *
 *	This procedure returns the ID of the currently running thread.
 *
 * Results:
 *	A thread ID.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_ThreadId
Tcl_GetCurrentThread()
{
    return (Tcl_ThreadId)GetCurrentThreadId();
}

/*
 *----------------------------------------------------------------------
 *
 * TclpInitLock
 *
 *	This procedure is used to grab a lock that serializes initialization
 *	and finalization of Tcl.  On some platforms this may also initialize
 *	the mutex used to serialize creation of more mutexes and thread
 *	local storage keys.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Acquire the initialization mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpInitLock()
{
    if (!init) {
	/*
	 * There is a fundamental race here that is solved by creating
	 * the first Tcl interpreter in a single threaded environment.
	 * Once the interpreter has been created, it is safe to create
	 * more threads that create interpreters in parallel.
	 */
	init = 1;
	InitializeCriticalSection(&joinLock);
	InitializeCriticalSection(&initLock);
	InitializeCriticalSection(&masterLock);
    }
    EnterCriticalSection(&initLock);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpInitUnlock
 *
 *	This procedure is used to release a lock that serializes initialization
 *	and finalization of Tcl.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Release the initialization mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpInitUnlock()
{
    LeaveCriticalSection(&initLock);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMasterLock
 *
 *	This procedure is used to grab a lock that serializes creation
 *	of mutexes, condition variables, and thread local storage keys.
 *
 *	This lock must be different than the initLock because the
 *	initLock is held during creation of syncronization objects.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Acquire the master mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpMasterLock()
{
    if (!init) {
	/*
	 * There is a fundamental race here that is solved by creating
	 * the first Tcl interpreter in a single threaded environment.
	 * Once the interpreter has been created, it is safe to create
	 * more threads that create interpreters in parallel.
	 */
	init = 1;
	InitializeCriticalSection(&joinLock);
	InitializeCriticalSection(&initLock);
	InitializeCriticalSection(&masterLock);
    }
    EnterCriticalSection(&masterLock);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMasterUnlock
 *
 *	This procedure is used to release a lock that serializes creation
 *	and deletion of synchronization objects.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Release the master mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpMasterUnlock()
{
    LeaveCriticalSection(&masterLock);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAllocMutex
 *
 *	This procedure returns a pointer to a statically initialized
 *	mutex for use by the memory allocator.  The alloctor must
 *	use this lock, because all other locks are allocated...
 *
 * Results:
 *	A pointer to a mutex that is suitable for passing to
 *	Tcl_MutexLock and Tcl_MutexUnlock.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Mutex *
Tcl_GetAllocMutex()
{
#ifdef TCL_THREADS
    static int once = 0;

    if (!once) {
	InitializeCriticalSection(&allocLock);
	once = 1;
    }
    return &allocLockPtr;
#else
    return NULL;
#endif
}


#ifdef TCL_THREADS

/* locally used prototype */
static void FinalizeConditionEvent(ClientData data);


/*
 *----------------------------------------------------------------------
 *
 * Tcl_MutexLock --
 *
 *	This procedure is invoked to lock a mutex.  This is a self 
 *	initializing mutex that is automatically finalized during
 *	Tcl_Finalize.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May block the current thread.  The mutex is aquired when
 *	this returns.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_MutexLock(mutexPtr)
    Tcl_Mutex *mutexPtr;	/* The lock */
{
    CRITICAL_SECTION *csPtr;
    if (*mutexPtr == NULL) {
	MASTER_LOCK;

	/* 
	 * Double inside master lock check to avoid a race.
	 */

	if (*mutexPtr == NULL) {
	    csPtr = (CRITICAL_SECTION *)ckalloc(sizeof(CRITICAL_SECTION));
	    InitializeCriticalSection(csPtr);
	    *mutexPtr = (Tcl_Mutex)csPtr;
	    TclRememberMutex(mutexPtr);
	}
	MASTER_UNLOCK;
    }
    csPtr = *((CRITICAL_SECTION **)mutexPtr);
    EnterCriticalSection(csPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MutexUnlock --
 *
 *	This procedure is invoked to unlock a mutex.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mutex is released when this returns.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_MutexUnlock(mutexPtr)
    Tcl_Mutex *mutexPtr;	/* The lock */
{
    CRITICAL_SECTION *csPtr = *((CRITICAL_SECTION **)mutexPtr);
    LeaveCriticalSection(csPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeMutex --
 *
 *	This procedure is invoked to clean up one mutex.  This is only
 *	safe to call at the end of time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mutex list is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeMutex(mutexPtr)
    Tcl_Mutex *mutexPtr;
{
    CRITICAL_SECTION *csPtr = *(CRITICAL_SECTION **)mutexPtr;
    if (csPtr != NULL) {
	DeleteCriticalSection(csPtr);
	ckfree((char *)csPtr);
	*mutexPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpThreadDataKeyInit --
 *
 *	This procedure initializes a thread specific data block key.
 *	Each thread has table of pointers to thread specific data.
 *	all threads agree on which table entry is used by each module.
 *	this is remembered in a "data key", that is just an index into
 *	this table.  To allow self initialization, the interface
 *	passes a pointer to this key and the first thread to use
 *	the key fills in the pointer to the key.  The key should be
 *	a process-wide static.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will allocate memory the first time this process calls for
 *	this key.  In this case it modifies its argument
 *	to hold the pointer to information about the key.
 *
 *----------------------------------------------------------------------
 */

void
TclpThreadDataKeyInit(keyPtr)
    Tcl_ThreadDataKey *keyPtr;	/* Identifier for the data chunk,
				 * really (DWORD **) */
{
    DWORD *indexPtr;
    DWORD newKey;

    MASTER_LOCK;
    if (*keyPtr == NULL) {
	indexPtr = (DWORD *)ckalloc(sizeof(DWORD));
	newKey = TlsAlloc();
        if (newKey != TLS_OUT_OF_INDEXES) {
            *indexPtr = newKey;
        } else {
            panic("TlsAlloc failed from TclpThreadDataKeyInit!"); /* this should be a fatal error */
        }
	*keyPtr = (Tcl_ThreadDataKey)indexPtr;
	TclRememberDataKey(keyPtr);
    }
    MASTER_UNLOCK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpThreadDataKeyGet --
 *
 *	This procedure returns a pointer to a block of thread local storage.
 *
 * Results:
 *	A thread-specific pointer to the data structure, or NULL
 *	if the memory has not been assigned to this key for this thread.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

VOID *
TclpThreadDataKeyGet(keyPtr)
    Tcl_ThreadDataKey *keyPtr;	/* Identifier for the data chunk,
				 * really (DWORD **) */
{
    DWORD *indexPtr = *(DWORD **)keyPtr;
    LPVOID result;
    if (indexPtr == NULL) {
	return NULL;
    } else {
        result = TlsGetValue(*indexPtr);
        if ((result == NULL) && (GetLastError() != NO_ERROR)) {
            panic("TlsGetValue failed from TclpThreadDataKeyGet!");
        }
	return result;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpThreadDataKeySet --
 *
 *	This procedure sets the pointer to a block of thread local storage.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up the thread so future calls to TclpThreadDataKeyGet with
 *	this key will return the data pointer.
 *
 *----------------------------------------------------------------------
 */

void
TclpThreadDataKeySet(keyPtr, data)
    Tcl_ThreadDataKey *keyPtr;	/* Identifier for the data chunk,
				 * really (pthread_key_t **) */
    VOID *data;			/* Thread local storage */
{
    DWORD *indexPtr = *(DWORD **)keyPtr;
    BOOL success;
    success = TlsSetValue(*indexPtr, (void *)data);
    if (!success) {
        panic("TlsSetValue failed from TclpThreadDataKeySet!");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeThreadData --
 *
 *	This procedure cleans up the thread-local storage.  This is
 *	called once for each thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees up the memory.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeThreadData(keyPtr)
    Tcl_ThreadDataKey *keyPtr;
{
    VOID *result;
    DWORD *indexPtr;
    BOOL success;

#ifdef USE_THREAD_ALLOC
    TclWinFreeAllocCache();
#endif
    if (*keyPtr != NULL) {
	indexPtr = *(DWORD **)keyPtr;
	result = (VOID *)TlsGetValue(*indexPtr);
	if (result != NULL) {
	    ckfree((char *)result);
	    success = TlsSetValue(*indexPtr, (void *)NULL);
            if (!success) {
                panic("TlsSetValue failed from TclpFinalizeThreadData!");
            }
	} else {
            if (GetLastError() != NO_ERROR) {
                panic("TlsGetValue failed from TclpFinalizeThreadData!");
            }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeThreadDataKey --
 *
 *	This procedure is invoked to clean up one key.  This is a
 *	process-wide storage identifier.  The thread finalization code
 *	cleans up the thread local storage itself.
 *
 *	This assumes the master lock is held.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The key is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeThreadDataKey(keyPtr)
    Tcl_ThreadDataKey *keyPtr;
{
    DWORD *indexPtr;
    BOOL success;
    if (*keyPtr != NULL) {
	indexPtr = *(DWORD **)keyPtr;
	success = TlsFree(*indexPtr);
        if (!success) {
            panic("TlsFree failed from TclpFinalizeThreadDataKey!");
        }
	ckfree((char *)indexPtr);
	*keyPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConditionWait --
 *
 *	This procedure is invoked to wait on a condition variable.
 *	The mutex is atomically released as part of the wait, and
 *	automatically grabbed when the condition is signaled.
 *
 *	The mutex must be held when this procedure is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May block the current thread.  The mutex is aquired when
 *	this returns.  Will allocate memory for a HANDLE
 *	and initialize this the first time this Tcl_Condition is used.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ConditionWait(condPtr, mutexPtr, timePtr)
    Tcl_Condition *condPtr;	/* Really (WinCondition **) */
    Tcl_Mutex *mutexPtr;	/* Really (CRITICAL_SECTION **) */
    Tcl_Time *timePtr;		/* Timeout on waiting period */
{
    WinCondition *winCondPtr;	/* Per-condition queue head */
    CRITICAL_SECTION *csPtr;	/* Caller's Mutex, after casting */
    DWORD wtime;		/* Windows time value */
    int timeout;		/* True if we got a timeout */
    int doExit = 0;		/* True if we need to do exit setup */
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->flags & WIN_THREAD_DEAD) {
	/*
	 * No more per-thread event on which to wait.
	 */

	return;
    }

    /*
     * Self initialize the two parts of the condition.
     * The per-condition and per-thread parts need to be
     * handled independently.
     */

    if (tsdPtr->flags == WIN_THREAD_UNINIT) {
	MASTER_LOCK;

	/* 
	 * Create the per-thread event and queue pointers.
	 */

	if (tsdPtr->flags == WIN_THREAD_UNINIT) {
	    tsdPtr->condEvent = CreateEvent(NULL, TRUE /* manual reset */,
			FALSE /* non signaled */, NULL);
	    tsdPtr->nextPtr = NULL;
	    tsdPtr->prevPtr = NULL;
	    tsdPtr->flags = WIN_THREAD_RUNNING;
	    doExit = 1;
	}
	MASTER_UNLOCK;

	if (doExit) {
	    /*
	     * Create a per-thread exit handler to clean up the condEvent.
	     * We must be careful to do this outside the Master Lock
	     * because Tcl_CreateThreadExitHandler uses its own
	     * ThreadSpecificData, and initializing that may drop
	     * back into the Master Lock.
	     */
	    
	    Tcl_CreateThreadExitHandler(FinalizeConditionEvent,
		    (ClientData) tsdPtr);
	}
    }

    if (*condPtr == NULL) {
	MASTER_LOCK;

	/*
	 * Initialize the per-condition queue pointers and Mutex.
	 */

	if (*condPtr == NULL) {
	    winCondPtr = (WinCondition *)ckalloc(sizeof(WinCondition));
	    InitializeCriticalSection(&winCondPtr->condLock);
	    winCondPtr->firstPtr = NULL;
	    winCondPtr->lastPtr = NULL;
	    *condPtr = (Tcl_Condition)winCondPtr;
	    TclRememberCondition(condPtr);
	}
	MASTER_UNLOCK;
    }
    csPtr = *((CRITICAL_SECTION **)mutexPtr);
    winCondPtr = *((WinCondition **)condPtr);
    if (timePtr == NULL) {
	wtime = INFINITE;
    } else {
	wtime = timePtr->sec * 1000 + timePtr->usec / 1000;
    }

    /*
     * Queue the thread on the condition, using
     * the per-condition lock for serialization.
     */

    tsdPtr->flags = WIN_THREAD_BLOCKED;
    tsdPtr->nextPtr = NULL;
    EnterCriticalSection(&winCondPtr->condLock);
    tsdPtr->prevPtr = winCondPtr->lastPtr;		/* A: */
    winCondPtr->lastPtr = tsdPtr;
    if (tsdPtr->prevPtr != NULL) {
        tsdPtr->prevPtr->nextPtr = tsdPtr;
    }
    if (winCondPtr->firstPtr == NULL) {
        winCondPtr->firstPtr = tsdPtr;
    }

    /*
     * Unlock the caller's mutex and wait for the condition, or a timeout.
     * There is a minor issue here in that we don't count down the
     * timeout if we get notified, but another thread grabs the condition
     * before we do.  In that race condition we'll wait again for the
     * full timeout.  Timed waits are dubious anyway.  Either you have
     * the locking protocol wrong and are masking a deadlock,
     * or you are using conditions to pause your thread.
     */
    
    LeaveCriticalSection(csPtr);
    timeout = 0;
    while (!timeout && (tsdPtr->flags & WIN_THREAD_BLOCKED)) {
	ResetEvent(tsdPtr->condEvent);
	LeaveCriticalSection(&winCondPtr->condLock);
	if (WaitForSingleObject(tsdPtr->condEvent, wtime) == WAIT_TIMEOUT) {
	    timeout = 1;
	}
	EnterCriticalSection(&winCondPtr->condLock);
    }

    /*
     * Be careful on timeouts because the signal might arrive right around
     * the time limit and someone else could have taken us off the queue.
     */
    
    if (timeout) {
	if (tsdPtr->flags & WIN_THREAD_RUNNING) {
	    timeout = 0;
	} else {
	    /*
	     * When dequeuing, we can leave the tsdPtr->nextPtr
	     * and tsdPtr->prevPtr with dangling pointers because
	     * they are reinitialilzed w/out reading them when the
	     * thread is enqueued later.
	     */

            if (winCondPtr->firstPtr == tsdPtr) {
                winCondPtr->firstPtr = tsdPtr->nextPtr;
            } else {
                tsdPtr->prevPtr->nextPtr = tsdPtr->nextPtr;
            }
            if (winCondPtr->lastPtr == tsdPtr) {
                winCondPtr->lastPtr = tsdPtr->prevPtr;
            } else {
                tsdPtr->nextPtr->prevPtr = tsdPtr->prevPtr;
            }
            tsdPtr->flags = WIN_THREAD_RUNNING;
	}
    }

    LeaveCriticalSection(&winCondPtr->condLock);
    EnterCriticalSection(csPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConditionNotify --
 *
 *	This procedure is invoked to signal a condition variable.
 *
 *	The mutex must be held during this call to avoid races,
 *	but this interface does not enforce that.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May unblock another thread.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ConditionNotify(condPtr)
    Tcl_Condition *condPtr;
{
    WinCondition *winCondPtr;
    ThreadSpecificData *tsdPtr;
    if (condPtr != NULL) {
	winCondPtr = *((WinCondition **)condPtr);

	/*
	 * Loop through all the threads waiting on the condition
	 * and notify them (i.e., broadcast semantics).  The queue
	 * manipulation is guarded by the per-condition coordinating mutex.
	 */

	EnterCriticalSection(&winCondPtr->condLock);
	while (winCondPtr->firstPtr != NULL) {
	    tsdPtr = winCondPtr->firstPtr;
	    winCondPtr->firstPtr = tsdPtr->nextPtr;
	    if (winCondPtr->lastPtr == tsdPtr) {
		winCondPtr->lastPtr = NULL;
	    }
	    tsdPtr->flags = WIN_THREAD_RUNNING;
	    tsdPtr->nextPtr = NULL;
	    tsdPtr->prevPtr = NULL;	/* Not strictly necessary, see A: */
	    SetEvent(tsdPtr->condEvent);
	}
	LeaveCriticalSection(&winCondPtr->condLock);
    } else {
	/*
	 * Noone has used the condition variable, so there are no waiters.
	 */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FinalizeConditionEvent --
 *
 *	This procedure is invoked to clean up the per-thread
 *	event used to implement condition waiting.
 *	This is only safe to call at the end of time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The per-thread event is closed.
 *
 *----------------------------------------------------------------------
 */

static void
FinalizeConditionEvent(data)
    ClientData data;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)data;
    tsdPtr->flags = WIN_THREAD_DEAD;
    CloseHandle(tsdPtr->condEvent);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeCondition --
 *
 *	This procedure is invoked to clean up a condition variable.
 *	This is only safe to call at the end of time.
 *
 *	This assumes the Master Lock is held.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The condition variable is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeCondition(condPtr)
    Tcl_Condition *condPtr;
{
    WinCondition *winCondPtr = *(WinCondition **)condPtr;

    /*
     * Note - this is called long after the thread-local storage is
     * reclaimed.  The per-thread condition waiting event is
     * reclaimed earlier in a per-thread exit handler, which is
     * called before thread local storage is reclaimed.
     */

    if (winCondPtr != NULL) {
	DeleteCriticalSection(&winCondPtr->condLock);
	ckfree((char *)winCondPtr);
	*condPtr = NULL;
    }
}

/*
 * Additions by AOL for specialized thread memory allocator.
 */
#ifdef USE_THREAD_ALLOC
static DWORD key;

Tcl_Mutex *
TclpNewAllocMutex(void)
{
    struct lock {
        Tcl_Mutex        tlock;
        CRITICAL_SECTION wlock;
    } *lockPtr;

    lockPtr = malloc(sizeof(struct lock));
    if (lockPtr == NULL) {
	panic("could not allocate lock");
    }
    lockPtr->tlock = (Tcl_Mutex) &lockPtr->wlock;
    InitializeCriticalSection(&lockPtr->wlock);
    return &lockPtr->tlock;
}

void *
TclpGetAllocCache(void)
{
    static int once = 0;
    VOID *result;

    if (!once) {
	/*
	 * We need to make sure that TclWinFreeAllocCache is called
	 * on each thread that calls this, but only on threads that
	 * call this.
	 */
    	key = TlsAlloc();
	once = 1;
	if (key == TLS_OUT_OF_INDEXES) {
	    panic("could not allocate thread local storage");
	}
    }

    result = TlsGetValue(key);
    if ((result == NULL) && (GetLastError() != NO_ERROR)) {
        panic("TlsGetValue failed from TclpGetAllocCache!");
    }
    return result;
}

void
TclpSetAllocCache(void *ptr)
{
    BOOL success;
    success = TlsSetValue(key, ptr);
    if (!success) {
        panic("TlsSetValue failed from TclpSetAllocCache!");
    }
}

void
TclWinFreeAllocCache(void)
{
    void *ptr;
    BOOL success;

    ptr = TlsGetValue(key);
    if (ptr != NULL) {
	success = TlsSetValue(key, NULL);
        if (!success) {
            panic("TlsSetValue failed from TclWinFreeAllocCache!");
        }
	TclFreeAllocCache(ptr);
    } else {
      if (GetLastError() != NO_ERROR) {
          panic("TlsGetValue failed from TclWinFreeAllocCache!");
      }
    }
}

#endif /* USE_THREAD_ALLOC */
#endif /* TCL_THREADS */
