/* 
 * tclMacThrd.c --
 *
 *	This file implements the Mac-specific thread support.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS:  @(#) tclMacThrd.c 1.2 98/02/23 16:48:07
 */

#include "tclInt.h"
#include "tclPort.h"
#include "tclMacInt.h"
#include <Threads.h>
#include <Gestalt.h>

#define TCL_MAC_THRD_DEFAULT_STACK (256*1024)


typedef struct TclMacThrdData {
    ThreadID threadID;
    VOID *data;
    struct TclMacThrdData *next;
} TclMacThrdData;

/*
 * This is an array of the Thread Data Keys.  It is a process-wide table.
 * Its size is originally set to 32, but it can grow if needed.
 */
 
static TclMacThrdData **tclMacDataKeyArray;
#define TCL_MAC_INITIAL_KEYSIZE 32

/*
 * These two bits of data store the current maximum number of keys
 * and the keyCounter (which is the number of occupied slots in the
 * KeyData array.
 * 
 */
 
static int maxNumKeys = 0;
static int keyCounter = 0;

/*
 * Prototypes for functions used only in this file
 */
 
TclMacThrdData *GetThreadDataStruct(Tcl_ThreadDataKey keyVal);
TclMacThrdData *RemoveThreadDataStruct(Tcl_ThreadDataKey keyVal);

/*
 * Note: The race evoked by the emulation layer for joinable threads
 * (see ../win/tclWinThrd.c) cannot occur on this platform due to
 * the cooperative implementation of multithreading.
 */

/*
 *----------------------------------------------------------------------
 *
 * TclMacHaveThreads --
 *
 *	Do we have the Thread Manager?
 *
 * Results:
 *	1 if the ThreadManager is present, 0 otherwise.
 *
 * Side effects:
 *	If this is the first time this is called, the return is cached.
 *
 *----------------------------------------------------------------------
 */

int
TclMacHaveThreads(void)
{
    static initialized = false;
    static int tclMacHaveThreads = false;
    long response = 0;
    OSErr err = noErr;
    
    if (!initialized) {
	err = Gestalt(gestaltThreadMgrAttr, &response);
	if (err == noErr) {
	    tclMacHaveThreads = response | (1 << gestaltThreadMgrPresent);
	}
    }

    return tclMacHaveThreads;
}

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
    if (!TclMacHaveThreads()) {
        return TCL_ERROR;
    }

    if (stackSize == TCL_THREAD_STACK_DEFAULT) {
        stackSize = TCL_MAC_THRD_DEFAULT_STACK;
    }

#if TARGET_CPU_68K && TARGET_RT_MAC_CFM
    {
        ThreadEntryProcPtr entryProc;
        entryProc = NewThreadEntryUPP(proc);
        
        NewThread(kCooperativeThread, entryProc, (void *) clientData, 
            stackSize, kCreateIfNeeded, NULL, (ThreadID *) idPtr);
    }
#else
    NewThread(kCooperativeThread, proc, (void *) clientData, 
        stackSize, kCreateIfNeeded, NULL, (ThreadID *) idPtr);
#endif        
    if ((ThreadID) *idPtr == kNoThreadID) {
        return TCL_ERROR;
    } else {
        if (flags & TCL_THREAD_JOINABLE) {
	    TclRememberJoinableThread (*idPtr);
	}

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
    Tcl_ThreadId threadId; /* Id of the thread to wait upon */
    int*     result;	   /* Reference to the storage the result
			    * of the thread we wait upon will be
			    * written into. */
{
    if (!TclMacHaveThreads()) {
        return TCL_ERROR;
    }

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
    ThreadID curThread;
    
    if (!TclMacHaveThreads()) {
        return;
    }
    
    GetCurrentThread(&curThread);
    TclSignalExitThread ((Tcl_ThreadId) curThread, status);

    DisposeThread(curThread, NULL, false);
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
#ifdef TCL_THREADS
    ThreadID curThread;

    if (!TclMacHaveThreads()) {
        return (Tcl_ThreadId) 0;
    } else {
        GetCurrentThread(&curThread);
        return (Tcl_ThreadId) curThread;
    }
#else
    return (Tcl_ThreadId) 0;
#endif
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
#ifdef TCL_THREADS
    /* There is nothing to do on the Mac. */;
#endif
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
#ifdef TCL_THREADS
    /* There is nothing to do on the Mac */;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMasterLock
 *
 *	This procedure is used to grab a lock that serializes creation
 *	and finalization of serialization objects.  This interface is
 *	only needed in finalization; it is hidden during
 *	creation of the objects.
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
#ifdef TCL_THREADS
    /* There is nothing to do on the Mac */;
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * TclpMasterUnlock
 *
 *	This procedure is used to release a lock that serializes creation
 *	and finalization of synchronization objects.
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
#ifdef TCL_THREADS
    /* There is nothing to do on the Mac */
#endif
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
    /* There is nothing to do on the Mac */
    return NULL;
}

#ifdef TCL_THREADS

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MutexLock --
 *
 *	This procedure is invoked to lock a mutex.  This procedure
 *	handles initializing the mutex, if necessary.  The caller
 *	can rely on the fact that Tcl_Mutex is an opaque pointer.
 *	This routine will change that pointer from NULL after first use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May block the current thread.  The mutex is aquired when
 *	this returns.  Will allocate memory for a pthread_mutex_t
 *	and initialize this the first time this Tcl_Mutex is used.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_MutexLock(mutexPtr)
    Tcl_Mutex *mutexPtr;	/* Really (pthread_mutex_t **) */
{
/* There is nothing to do on the Mac */
}


/*
 *----------------------------------------------------------------------
 *
 * TclpMutexUnlock --
 *
 *	This procedure is invoked to unlock a mutex.  The mutex must
 *	have been locked by Tcl_MutexLock.
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
    Tcl_Mutex *mutexPtr;	/* Really (pthread_mutex_t **) */
{
/* There is nothing to do on the Mac */
}


/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeMutex --
 *
 *	This procedure is invoked to clean up one mutex.  This is only
 *	safe to call at the end of time.
 *
 *	This assumes the Master Lock is held.
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
/* There is nothing to do on the Mac */
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
 *      There is no system-wide support for thread specific data on the 
 *	Mac.  So we implement this as an array of pointers.  The keys are
 *	allocated sequentially, and each key maps to a slot in the table.
 *      The table element points to a linked list of the instances of
 *	the data for each thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will bump the key counter if this is the first time this key
 *      has been initialized.  May grow the DataKeyArray if that is
 *	necessary.
 *
 *----------------------------------------------------------------------
 */

void
TclpThreadDataKeyInit(keyPtr)
    Tcl_ThreadDataKey *keyPtr;	/* Identifier for the data chunk,
				 * really (pthread_key_t **) */
{
            
    if (*keyPtr == NULL) {
        keyCounter += 1;
	*keyPtr = (Tcl_ThreadDataKey) keyCounter;
	if (keyCounter > maxNumKeys) {
	    TclMacThrdData **newArray;
	    int i, oldMax = maxNumKeys;
	     
	    maxNumKeys = maxNumKeys + TCL_MAC_INITIAL_KEYSIZE;
	     
	    newArray = (TclMacThrdData **) 
	            ckalloc(maxNumKeys * sizeof(TclMacThrdData *));
	     
	    for (i = 0; i < oldMax; i++) {
	        newArray[i] = tclMacDataKeyArray[i];
	    }
	    for (i = oldMax; i < maxNumKeys; i++) {
	        newArray[i] = NULL;
	    }
	     
	    if (tclMacDataKeyArray != NULL) {
		ckfree((char *) tclMacDataKeyArray);
	    }
	    tclMacDataKeyArray = newArray;
	     
	}             
	/* TclRememberDataKey(keyPtr); */
    }
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
				 * really (pthread_key_t **) */
{
    TclMacThrdData *dataPtr;
    
    dataPtr = GetThreadDataStruct(*keyPtr);
    
    if (dataPtr == NULL) {
        return NULL;
    } else {
        return dataPtr->data;
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
    TclMacThrdData *dataPtr;
    ThreadID curThread;
    
    dataPtr = GetThreadDataStruct(*keyPtr);
    
    /* 
     * Is it legal to reset the thread data like this?
     * And if so, who owns the memory?
     */
     
    if (dataPtr != NULL) {
        dataPtr->data = data;
    } else {
        dataPtr = (TclMacThrdData *) ckalloc(sizeof(TclMacThrdData));
        GetCurrentThread(&curThread);
        dataPtr->threadID = curThread;
        dataPtr->data = data;
        dataPtr->next = tclMacDataKeyArray[(int) *keyPtr - 1];
        tclMacDataKeyArray[(int) *keyPtr - 1] = dataPtr;
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
 *	Frees up all thread local storage.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeThreadData(keyPtr)
    Tcl_ThreadDataKey *keyPtr;
{
    TclMacThrdData *dataPtr;
    
    if (*keyPtr != NULL) {
        dataPtr = RemoveThreadDataStruct(*keyPtr);
        
	if ((dataPtr != NULL) && (dataPtr->data != NULL)) {
	    ckfree((char *) dataPtr->data);
	    ckfree((char *) dataPtr);
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
 *      On the Mac, there is really nothing to do here, since the key
 *      is just an array index.  But we set the key to 0 just in case
 *	someone else is relying on that.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The keyPtr value is set to 0.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeThreadDataKey(keyPtr)
    Tcl_ThreadDataKey *keyPtr;
{
    ckfree((char *) tclMacDataKeyArray[(int) *keyPtr - 1]);
    tclMacDataKeyArray[(int) *keyPtr - 1] = NULL;
    *keyPtr = NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * GetThreadDataStruct --
 *
 *	This procedure gets the data structure corresponding to
 *      keyVal for the current process.
 *
 * Results:
 *	The requested key data.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclMacThrdData *
GetThreadDataStruct(keyVal)
    Tcl_ThreadDataKey keyVal;
{
    ThreadID curThread;
    TclMacThrdData *dataPtr;
    
    /*
     * The keyPtr will only be greater than keyCounter is someone
     * has passed us a key without getting the value from 
     * TclpInitDataKey.
     */
     
    if ((int) keyVal <= 0)  {
        return NULL;
    } else if ((int) keyVal > keyCounter) {
        panic("illegal data key value");
    }
    
    GetCurrentThread(&curThread);
    
    for (dataPtr = tclMacDataKeyArray[(int) keyVal - 1]; dataPtr != NULL;
            dataPtr = dataPtr->next) {
        if (dataPtr->threadID ==  curThread) {
            break;
        }
    }
    
    return dataPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * RemoveThreadDataStruct --
 *
 *	This procedure removes the data structure corresponding to
 *      keyVal for the current process from the list kept for keyVal.
 *
 * Results:
 *	The requested key data is removed from the list, and a pointer 
 *      to it is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclMacThrdData *
RemoveThreadDataStruct(keyVal)
    Tcl_ThreadDataKey keyVal;
{
    ThreadID curThread;
    TclMacThrdData *dataPtr, *prevPtr;
    
     
    if ((int) keyVal <= 0)  {
        return NULL;
    } else if ((int) keyVal > keyCounter) {
        panic("illegal data key value");
    }
    
    GetCurrentThread(&curThread);
    
    for (dataPtr = tclMacDataKeyArray[(int) keyVal - 1], prevPtr = NULL; 
            dataPtr != NULL;
            prevPtr = dataPtr, dataPtr = dataPtr->next) {
        if (dataPtr->threadID == curThread) {
            break;
        }
    }
    
    if (dataPtr == NULL) {
        /* No body */
    } else if ( prevPtr == NULL) {
        tclMacDataKeyArray[(int) keyVal - 1] = dataPtr->next;
    } else {
        prevPtr->next = dataPtr->next;
    }
    
    return dataPtr; 
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConditionWait --
 *
 *	This procedure is invoked to wait on a condition variable.
 *	On the Mac, mutexes are no-ops, and we just yield.  After
 *	all, it is the application's job to loop till the condition 
 *	variable is changed...
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will block the current thread till someone else yields.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ConditionWait(condPtr, mutexPtr, timePtr)
    Tcl_Condition *condPtr;	/* Really (pthread_cond_t **) */
    Tcl_Mutex *mutexPtr;	/* Really (pthread_mutex_t **) */
    Tcl_Time *timePtr;		/* Timeout on waiting period */
{
    if (TclMacHaveThreads()) {
        YieldToAnyThread();
    }
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
    if (TclMacHaveThreads()) {
         YieldToAnyThread();
    }
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
    /* Nothing to do on the Mac */
}



#endif /* TCL_THREADS */

