/* 
 * tclUnixThrd.c --
 *
 *	This file implements the UNIX-specific thread support.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS:  @(#) tclUnixThrd.c 1.18 98/02/19 14:24:12
 */

#include "tclInt.h"
#include "tclPort.h"

#ifdef TCL_THREADS

#include "pthread.h"

typedef struct ThreadSpecificData {
    char	    	nabuf[16];
    struct tm   	gtbuf;
    struct tm   	ltbuf;
    struct {
	Tcl_DirEntry ent;
	char name[MAXNAMLEN+1];
    } rdbuf;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * masterLock is used to serialize creation of mutexes, condition
 * variables, and thread local storage.
 * This is the only place that can count on the ability to statically
 * initialize the mutex.
 */

static pthread_mutex_t masterLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * initLock is used to serialize initialization and finalization
 * of Tcl.  It cannot use any dyamically allocated storage.
 */

static pthread_mutex_t initLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * allocLock is used by Tcl's version of malloc for synchronization.
 * For obvious reasons, cannot use any dyamically allocated storage.
 */

static pthread_mutex_t allocLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *allocLockPtr = &allocLock;

/*
 * These are for the critical sections inside this file.
 */

#define MASTER_LOCK	pthread_mutex_lock(&masterLock)
#define MASTER_UNLOCK	pthread_mutex_unlock(&masterLock)

#endif /* TCL_THREADS */


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
#ifdef TCL_THREADS
    pthread_attr_t attr;
    int result;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
    if (stackSize != TCL_THREAD_STACK_DEFAULT) {
        pthread_attr_setstacksize(&attr, (size_t) stackSize);
#ifdef TCL_THREAD_STACK_MIN
    } else {
        /*
	 * Certain systems define a thread stack size that by default is
	 * too small for many operations.  The user has the option of
	 * defining TCL_THREAD_STACK_MIN to a value large enough to work
	 * for their needs.  This would look like (for 128K min stack):
	 *    make MEM_DEBUG_FLAGS=-DTCL_THREAD_STACK_MIN=131072L
	 *
	 * This solution is not optimal, as we should allow the user to
	 * specify a size at runtime, but we don't want to slow this function
	 * down, and that would still leave the main thread at the default.
	 */

        size_t size;
	result = pthread_attr_getstacksize(&attr, &size);
	if (!result && (size < TCL_THREAD_STACK_MIN)) {
	    pthread_attr_setstacksize(&attr, (size_t) TCL_THREAD_STACK_MIN);
	}
#endif
    }
#endif
    if (! (flags & TCL_THREAD_JOINABLE)) {
        pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    }


    if (pthread_create((pthread_t *)idPtr, &attr,
	    (void * (*)())proc, (void *)clientData) &&
	    pthread_create((pthread_t *)idPtr, NULL,
		    (void * (*)())proc, (void *)clientData)) {
	result = TCL_ERROR;
    } else {
	result = TCL_OK;
    }
    pthread_attr_destroy(&attr);
    return result;
#else
    return TCL_ERROR;
#endif /* TCL_THREADS */
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
Tcl_JoinThread(threadId, state)
    Tcl_ThreadId threadId; /* Id of the thread to wait upon */
    int*     state;	   /* Reference to the storage the result
			    * of the thread we wait upon will be
			    * written into. */
{
#ifdef TCL_THREADS
    int result;

    result = pthread_join ((pthread_t) threadId, (VOID**) state);
    return (result == 0) ? TCL_OK : TCL_ERROR;
#else
    return TCL_ERROR;
#endif
}

#ifdef TCL_THREADS
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
    pthread_exit((VOID *)status);
}
#endif /* TCL_THREADS */

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
    return (Tcl_ThreadId) pthread_self();
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
    pthread_mutex_lock(&initLock);
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
    pthread_mutex_unlock(&initLock);
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
    pthread_mutex_lock(&masterLock);
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
    pthread_mutex_unlock(&masterLock);
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
#ifdef TCL_THREADS
    return (Tcl_Mutex *)&allocLockPtr;
#else
    return NULL;
#endif
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
    pthread_mutex_t *pmutexPtr;
    if (*mutexPtr == NULL) {
	MASTER_LOCK;
	if (*mutexPtr == NULL) {
	    /* 
	     * Double inside master lock check to avoid a race condition.
	     */
    
	    pmutexPtr = (pthread_mutex_t *)ckalloc(sizeof(pthread_mutex_t));
	    pthread_mutex_init(pmutexPtr, NULL);
	    *mutexPtr = (Tcl_Mutex)pmutexPtr;
	    TclRememberMutex(mutexPtr);
	}
	MASTER_UNLOCK;
    }
    pmutexPtr = *((pthread_mutex_t **)mutexPtr);
    pthread_mutex_lock(pmutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_MutexUnlock --
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
    pthread_mutex_t *pmutexPtr = *(pthread_mutex_t **)mutexPtr;
    pthread_mutex_unlock(pmutexPtr);
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
    pthread_mutex_t *pmutexPtr = *(pthread_mutex_t **)mutexPtr;
    if (pmutexPtr != NULL) {
	ckfree((char *)pmutexPtr);
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
				 * really (pthread_key_t **) */
{
    pthread_key_t *pkeyPtr;

    MASTER_LOCK;
    if (*keyPtr == NULL) {
	pkeyPtr = (pthread_key_t *)ckalloc(sizeof(pthread_key_t));
	pthread_key_create(pkeyPtr, NULL);
	*keyPtr = (Tcl_ThreadDataKey)pkeyPtr;
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
				 * really (pthread_key_t **) */
{
    pthread_key_t *pkeyPtr = *(pthread_key_t **)keyPtr;
    if (pkeyPtr == NULL) {
	return NULL;
    } else {
	return (VOID *)pthread_getspecific(*pkeyPtr);
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
    pthread_key_t *pkeyPtr = *(pthread_key_t **)keyPtr;
    pthread_setspecific(*pkeyPtr, data);
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
    VOID *result;
    pthread_key_t *pkeyPtr;

    if (*keyPtr != NULL) {
	pkeyPtr = *(pthread_key_t **)keyPtr;
	result = (VOID *)pthread_getspecific(*pkeyPtr);
	if (result != NULL) {
	    ckfree((char *)result);
	    pthread_setspecific(*pkeyPtr, (void *)NULL);
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
    pthread_key_t *pkeyPtr;
    if (*keyPtr != NULL) {
	pkeyPtr = *(pthread_key_t **)keyPtr;
	pthread_key_delete(*pkeyPtr);
	ckfree((char *)pkeyPtr);
	*keyPtr = NULL;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConditionWait --
 *
 *	This procedure is invoked to wait on a condition variable.
 *	The mutex is automically released as part of the wait, and
 *	automatically grabbed when the condition is signaled.
 *
 *	The mutex must be held when this procedure is called.
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
Tcl_ConditionWait(condPtr, mutexPtr, timePtr)
    Tcl_Condition *condPtr;	/* Really (pthread_cond_t **) */
    Tcl_Mutex *mutexPtr;	/* Really (pthread_mutex_t **) */
    Tcl_Time *timePtr;		/* Timeout on waiting period */
{
    pthread_cond_t *pcondPtr;
    pthread_mutex_t *pmutexPtr;
    struct timespec ptime;

    if (*condPtr == NULL) {
	MASTER_LOCK;

	/* 
	 * Double check inside mutex to avoid race,
	 * then initialize condition variable if necessary.
	 */

	if (*condPtr == NULL) {
	    pcondPtr = (pthread_cond_t *)ckalloc(sizeof(pthread_cond_t));
	    pthread_cond_init(pcondPtr, NULL);
	    *condPtr = (Tcl_Condition)pcondPtr;
	    TclRememberCondition(condPtr);
	}
	MASTER_UNLOCK;
    }
    pmutexPtr = *((pthread_mutex_t **)mutexPtr);
    pcondPtr = *((pthread_cond_t **)condPtr);
    if (timePtr == NULL) {
	pthread_cond_wait(pcondPtr, pmutexPtr);
    } else {
	Tcl_Time now;

	/*
	 * Make sure to take into account the microsecond component of the
	 * current time, including possible overflow situations. [Bug #411603]
	 */

	Tcl_GetTime(&now);
	ptime.tv_sec = timePtr->sec + now.sec +
	    (timePtr->usec + now.usec) / 1000000;
	ptime.tv_nsec = 1000 * ((timePtr->usec + now.usec) % 1000000);
	pthread_cond_timedwait(pcondPtr, pmutexPtr, &ptime);
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
    pthread_cond_t *pcondPtr = *((pthread_cond_t **)condPtr);
    if (pcondPtr != NULL) {
	pthread_cond_broadcast(pcondPtr);
    } else {
	/*
	 * Noone has used the condition variable, so there are no waiters.
	 */
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
    pthread_cond_t *pcondPtr = *(pthread_cond_t **)condPtr;
    if (pcondPtr != NULL) {
	pthread_cond_destroy(pcondPtr);
	ckfree((char *)pcondPtr);
	*condPtr = NULL;
    }
}
#endif /* TCL_THREADS */

/*
 *----------------------------------------------------------------------
 *
 * TclpReaddir, TclpLocaltime, TclpGmtime, TclpInetNtoa --
 *
 *	These procedures replace core C versions to be used in a
 *	threaded environment.
 *
 * Results:
 *	See documentation of C functions.
 *
 * Side effects:
 *	See documentation of C functions.
 *
 *----------------------------------------------------------------------
 */

#if defined(TCL_THREADS) && !defined(HAVE_READDIR_R)
TCL_DECLARE_MUTEX( rdMutex )
#undef readdir
#endif

Tcl_DirEntry *
TclpReaddir(DIR * dir)
{
    Tcl_DirEntry *ent;
#ifdef TCL_THREADS
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#ifdef HAVE_READDIR_R
    ent = &tsdPtr->rdbuf.ent; 
    if (TclOSreaddir_r(dir, ent, &ent) != 0) {
	ent = NULL;
    }

#else /* !HAVE_READDIR_R */

    Tcl_MutexLock(&rdMutex);
#   ifdef HAVE_STRUCT_DIRENT64
    ent = readdir64(dir);
#   else /* !HAVE_STRUCT_DIRENT64 */
    ent = readdir(dir);
#   endif /* HAVE_STRUCT_DIRENT64 */
    if (ent != NULL) {
	memcpy((VOID *) &tsdPtr->rdbuf.ent, (VOID *) ent,
		sizeof(tsdPtr->rdbuf));
	ent = &tsdPtr->rdbuf.ent;
    }
    Tcl_MutexUnlock(&rdMutex);

#endif /* HAVE_READDIR_R */
#else
#   ifdef HAVE_STRUCT_DIRENT64
    ent = readdir64(dir);
#   else /* !HAVE_STRUCT_DIRENT64 */
    ent = readdir(dir);
#   endif /* HAVE_STRUCT_DIRENT64 */
#endif
    return ent;
}

#if defined(TCL_THREADS) && (!defined(HAVE_GMTIME_R) || !defined(HAVE_LOCALTIME_R))
TCL_DECLARE_MUTEX( tmMutex )
#undef localtime
#undef gmtime
#endif

struct tm *
TclpLocaltime(time_t * clock)
{
#ifdef TCL_THREADS
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#ifdef HAVE_LOCALTIME_R
    return localtime_r(clock, &tsdPtr->ltbuf);
#else
    Tcl_MutexLock( &tmMutex );
    memcpy( (VOID *) &tsdPtr->ltbuf, (VOID *) localtime( clock ), sizeof (struct tm) );
    Tcl_MutexUnlock( &tmMutex );
    return &tsdPtr->ltbuf;
#endif    
#else
    return localtime(clock);
#endif
}

struct tm *
TclpGmtime(time_t * clock)
{
#ifdef TCL_THREADS
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#ifdef HAVE_GMTIME_R
    return gmtime_r(clock, &tsdPtr->gtbuf);
#else
    Tcl_MutexLock( &tmMutex );
    memcpy( (VOID *) &tsdPtr->gtbuf, (VOID *) gmtime( clock ), sizeof (struct tm) );
    Tcl_MutexUnlock( &tmMutex );
    return &tsdPtr->gtbuf;
#endif    
#else
    return gmtime(clock);
#endif
}

char *
TclpInetNtoa(struct in_addr addr)
{
#ifdef TCL_THREADS
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    union {
    	unsigned long l;
    	unsigned char b[4];
    } u;
    
    u.l = (unsigned long) addr.s_addr;
    sprintf(tsdPtr->nabuf, "%u.%u.%u.%u", u.b[0], u.b[1], u.b[2], u.b[3]);
    return tsdPtr->nabuf;
#else
    return inet_ntoa(addr);
#endif
}

#ifdef TCL_THREADS
/*
 * Additions by AOL for specialized thread memory allocator.
 */
#ifdef USE_THREAD_ALLOC
static int initialized = 0;
static pthread_key_t	key;
static pthread_once_t	once = PTHREAD_ONCE_INIT;

Tcl_Mutex *
TclpNewAllocMutex(void)
{
    struct lock {
        Tcl_Mutex       tlock;
        pthread_mutex_t plock;
    } *lockPtr;

    lockPtr = malloc(sizeof(struct lock));
    if (lockPtr == NULL) {
	panic("could not allocate lock");
    }
    lockPtr->tlock = (Tcl_Mutex) &lockPtr->plock;
    pthread_mutex_init(&lockPtr->plock, NULL);
    return &lockPtr->tlock;
}

static void
InitKey(void)
{
    extern void TclFreeAllocCache(void *);

    pthread_key_create(&key, TclFreeAllocCache);
    initialized = 1;
}

void *
TclpGetAllocCache(void)
{
    if (!initialized) {
	pthread_once(&once, InitKey);
    }
    return pthread_getspecific(key);
}

void
TclpSetAllocCache(void *arg)
{
    pthread_setspecific(key, arg);
}

#endif /* USE_THREAD_ALLOC */
#endif /* TCL_THREADS */
