/* 
 * tclThreadTest.c --
 *
 *	This file implements the testthread command.  Eventually this
 *	should be tclThreadCmd.c
 *	Some of this code is based on work done by Richard Hipp on behalf of
 *	Conservation Through Innovation, Limited, with their permission.
 *
 * Copyright (c) 1998 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"

#ifdef TCL_THREADS
/*
 * Each thread has an single instance of the following structure.  There
 * is one instance of this structure per thread even if that thread contains
 * multiple interpreters.  The interpreter identified by this structure is
 * the main interpreter for the thread.  
 *
 * The main interpreter is the one that will process any messages 
 * received by a thread.  Any thread can send messages but only the
 * main interpreter can receive them.
 */

typedef struct ThreadSpecificData {
    Tcl_ThreadId  threadId;          /* Tcl ID for this thread */
    Tcl_Interp *interp;              /* Main interpreter for this thread */
    int flags;                       /* See the TP_ defines below... */
    struct ThreadSpecificData *nextPtr;	/* List for "thread names" */
    struct ThreadSpecificData *prevPtr;	/* List for "thread names" */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * This list is used to list all threads that have interpreters.
 * This is protected by threadMutex.
 */

static struct ThreadSpecificData *threadList;

/*
 * The following bit-values are legal for the "flags" field of the
 * ThreadSpecificData structure.
 */
#define TP_Dying               0x001 /* This thread is being cancelled */

/*
 * An instance of the following structure contains all information that is
 * passed into a new thread when the thread is created using either the
 * "thread create" Tcl command or the TclCreateThread() C function.
 */

typedef struct ThreadCtrl {
    char *script;    /* The TCL command this thread should execute */
    int flags;        /* Initial value of the "flags" field in the 
                       * ThreadSpecificData structure for the new thread.
                       * Might contain TP_Detached or TP_TclThread. */
    Tcl_Condition condWait;
    /* This condition variable is used to synchronize
     * the parent and child threads.  The child won't run
     * until it acquires threadMutex, and the parent function
     * won't complete until signaled on this condition
     * variable. */
} ThreadCtrl;

/*
 * This is the event used to send scripts to other threads.
 */

typedef struct ThreadEvent {
    Tcl_Event event;		/* Must be first */
    char *script;		/* The script to execute. */
    struct ThreadEventResult *resultPtr;
				/* To communicate the result.  This is
				 * NULL if we don't care about it. */
} ThreadEvent;

typedef struct ThreadEventResult {
    Tcl_Condition done;		/* Signaled when the script completes */
    int code;			/* Return value of Tcl_Eval */
    char *result;		/* Result from the script */
    char *errorInfo;		/* Copy of errorInfo variable */
    char *errorCode;		/* Copy of errorCode variable */
    Tcl_ThreadId srcThreadId;	/* Id of sending thread, in case it dies */
    Tcl_ThreadId dstThreadId;	/* Id of target thread, in case it dies */
    struct ThreadEvent *eventPtr;	/* Back pointer */
    struct ThreadEventResult *nextPtr;	/* List for cleanup */
    struct ThreadEventResult *prevPtr;

} ThreadEventResult;

static ThreadEventResult *resultList;

/*
 * This is for simple error handling when a thread script exits badly.
 */

static Tcl_ThreadId errorThreadId;
static char *errorProcString;

/* 
 * Access to the list of threads and to the thread send results is
 * guarded by this mutex. 
 */

TCL_DECLARE_MUTEX(threadMutex)

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

EXTERN int	TclThread_Init _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int	Tcl_ThreadObjCmd _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
EXTERN int	TclCreateThread _ANSI_ARGS_((Tcl_Interp *interp,
	CONST char *script));
EXTERN int	TclThreadList _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int	TclThreadSend _ANSI_ARGS_((Tcl_Interp *interp, Tcl_ThreadId id,
	char *script, int wait));

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#ifdef MAC_TCL
static pascal void *NewThread _ANSI_ARGS_((ClientData clientData));
#else
static void     NewThread _ANSI_ARGS_((ClientData clientData));
#endif
static void	ListRemove _ANSI_ARGS_((ThreadSpecificData *tsdPtr));
static void	ListUpdateInner _ANSI_ARGS_((ThreadSpecificData *tsdPtr));
static int	ThreadEventProc _ANSI_ARGS_((Tcl_Event *evPtr, int mask));
static void	ThreadErrorProc _ANSI_ARGS_((Tcl_Interp *interp));
static void	ThreadFreeProc _ANSI_ARGS_((ClientData clientData));
static int	ThreadDeleteEvent _ANSI_ARGS_((Tcl_Event *eventPtr,
	ClientData clientData));
static void	ThreadExitProc _ANSI_ARGS_((ClientData clientData));


/*
 *----------------------------------------------------------------------
 *
 * TclThread_Init --
 *
 *	Initialize the test thread command.
 *
 * Results:
 *      TCL_OK if the package was properly initialized.
 *
 * Side effects:
 *	Add the "testthread" command to the interp.
 *
 *----------------------------------------------------------------------
 */

int
TclThread_Init(interp)
    Tcl_Interp *interp; /* The current Tcl interpreter */
{
    
    Tcl_CreateObjCommand(interp,"testthread", Tcl_ThreadObjCmd, 
	    (ClientData)NULL ,NULL);
    if (Tcl_PkgProvide(interp, "Thread", "1.0" ) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_ThreadObjCmd --
 *
 *	This procedure is invoked to process the "testthread" Tcl command.
 *	See the user documentation for details on what it does.
 *
 *	thread create
 *	thread send id ?-async? script
 *	thread exit
 *	thread info id
 *	thread names
 *	thread wait
 *	thread errorproc proc
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ThreadObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    int option;
    static char *threadOptions[] = {"create", "exit", "id", "names",
				    "send", "wait", "errorproc", (char *) NULL};
    enum options {THREAD_CREATE, THREAD_EXIT, THREAD_ID, THREAD_NAMES,
		  THREAD_SEND, THREAD_WAIT, THREAD_ERRORPROC};

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], threadOptions,
	    "option", 0, &option) != TCL_OK) {
	return TCL_ERROR;
    }

    /* 
     * Make sure the initial thread is on the list before doing anything.
     */

    if (tsdPtr->interp == NULL) {
	Tcl_MutexLock(&threadMutex);
	tsdPtr->interp = interp;
	ListUpdateInner(tsdPtr);
	Tcl_CreateThreadExitHandler(ThreadExitProc, NULL);
	Tcl_MutexUnlock(&threadMutex);
    }

    switch ((enum options)option) {
	case THREAD_CREATE: {
	    char *script;
	    if (objc == 2) {
		script = "testthread wait";	/* Just enter the event loop */
	    } else if (objc == 3) {
		script = Tcl_GetString(objv[2]);
	    } else {
		Tcl_WrongNumArgs(interp, 2, objv, "?script?");
		return TCL_ERROR;
	    }
	    return TclCreateThread(interp, script);
	}
	case THREAD_EXIT: {
	    if (objc > 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return TCL_ERROR;
	    }
	    ListRemove(NULL);
	    Tcl_ExitThread(0);
	    return TCL_OK;
	}
	case THREAD_ID:
	    if (objc == 2) {
		Tcl_Obj *idObj = Tcl_NewIntObj((int)Tcl_GetCurrentThread());
		Tcl_SetObjResult(interp, idObj);
		return TCL_OK;
	    } else {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }
	case THREAD_NAMES: {
	    if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }
	    return TclThreadList(interp);
	}
	case THREAD_SEND: {
	    int id;
	    char *script;
	    int wait, arg;

	    if ((objc != 4) && (objc != 5)) {
		Tcl_WrongNumArgs(interp, 1, objv, "send ?-async? id script");
		return TCL_ERROR;
	    }
	    if (objc == 5) {
		if (strcmp("-async", Tcl_GetString(objv[2])) != 0) {
		    Tcl_WrongNumArgs(interp, 1, objv, "send ?-async? id script");
		    return TCL_ERROR;
		}
		wait = 0;
		arg = 3;
	    } else {
		wait = 1;
		arg = 2;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[arg], &id) != TCL_OK) {
		return TCL_ERROR;
	    }
	    arg++;
	    script = Tcl_GetString(objv[arg]);
	    return TclThreadSend(interp, (Tcl_ThreadId) id, script, wait);
	}
	case THREAD_WAIT: {
	    while (1) {
		(void) Tcl_DoOneEvent(TCL_ALL_EVENTS);
	    }
	}
	case THREAD_ERRORPROC: {
	    /*
	     * Arrange for this proc to handle thread death errors.
	     */

	    char *proc;
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "errorproc proc");
		return TCL_ERROR;
	    }
	    Tcl_MutexLock(&threadMutex);
	    errorThreadId = Tcl_GetCurrentThread();
	    if (errorProcString) {
		ckfree(errorProcString);
	    }
	    proc = Tcl_GetString(objv[2]);
	    errorProcString = ckalloc(strlen(proc)+1);
	    strcpy(errorProcString, proc);
	    Tcl_MutexUnlock(&threadMutex);
	    return TCL_OK;
	}
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * TclCreateThread --
 *
 *	This procedure is invoked to create a thread containing an interp to
 *	run a script.  This returns after the thread has started executing.
 *
 * Results:
 *	A standard Tcl result, which is the thread ID.
 *
 * Side effects:
 *	Create a thread.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclCreateThread(interp, script)
    Tcl_Interp *interp;			/* Current interpreter. */
    CONST char *script;			/* Script to execute */
{
    ThreadCtrl ctrl;
    Tcl_ThreadId id;

    ctrl.script = (char *) script;
    ctrl.condWait = NULL;
    ctrl.flags = 0;

    Tcl_MutexLock(&threadMutex);
    if (TclpThreadCreate(&id, NewThread, (ClientData) &ctrl) != TCL_OK) {
	Tcl_MutexUnlock(&threadMutex);
        Tcl_AppendResult(interp,"can't create a new thread",0);
	ckfree((void*)ctrl.script);
	return TCL_ERROR;
    }

    /*
     * Wait for the thread to start because it is using something on our stack!
     */

    Tcl_ConditionWait(&ctrl.condWait, &threadMutex, NULL);
    Tcl_MutexUnlock(&threadMutex);
    TclFinalizeCondition(&ctrl.condWait);
    Tcl_SetObjResult(interp, Tcl_NewIntObj((int)id));
    return TCL_OK;
}

/*
 *------------------------------------------------------------------------
 *
 * NewThread --
 *
 *    This routine is the "main()" for a new thread whose task is to
 *    execute a single TCL script.  The argument to this function is
 *    a pointer to a structure that contains the text of the TCL script
 *    to be executed.
 *
 *    Space to hold the script field of the ThreadControl structure passed 
 *    in as the only argument was obtained from malloc() and must be freed 
 *    by this function before it exits.  Space to hold the ThreadControl
 *    structure itself is released by the calling function, and the
 *    two condition variables in the ThreadControl structure are destroyed
 *    by the calling function.  The calling function will destroy the
 *    ThreadControl structure and the condition variable as soon as
 *    ctrlPtr->condWait is signaled, so this routine must make copies of
 *    any data it might need after that point.
 *
 * Results:
 *    none
 *
 * Side effects:
 *    A TCL script is executed in a new thread.
 *
 *------------------------------------------------------------------------
 */
#ifdef MAC_TCL
static pascal void *
#else
static void
#endif
NewThread(clientData)
    ClientData clientData;
{
    ThreadCtrl *ctrlPtr = (ThreadCtrl*)clientData;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    int result;
    char *threadEvalScript;

    /*
     * Initialize the interpreter.  This should be more general.
     */

    tsdPtr->interp = Tcl_CreateInterp();
    result = Tcl_Init(tsdPtr->interp);
    result = TclThread_Init(tsdPtr->interp);

    /*
     * Update the list of threads.
     */

    Tcl_MutexLock(&threadMutex);
    ListUpdateInner(tsdPtr);
    /*
     * We need to keep a pointer to the alloc'ed mem of the script
     * we are eval'ing, for the case that we exit during evaluation
     */
    threadEvalScript = (char *) ckalloc(strlen(ctrlPtr->script)+1);
    strcpy(threadEvalScript, ctrlPtr->script);

    Tcl_CreateThreadExitHandler(ThreadExitProc, (ClientData) threadEvalScript);

    /*
     * Notify the parent we are alive.
     */

    Tcl_ConditionNotify(&ctrlPtr->condWait);
    Tcl_MutexUnlock(&threadMutex);

    /*
     * Run the script.
     */

    Tcl_Preserve((ClientData) tsdPtr->interp);
    result = Tcl_Eval(tsdPtr->interp, threadEvalScript);
    if (result != TCL_OK) {
	ThreadErrorProc(tsdPtr->interp);
    }

    /*
     * Clean up.
     */

    ListRemove(tsdPtr);
    Tcl_Release((ClientData) tsdPtr->interp);
    Tcl_DeleteInterp(tsdPtr->interp);
    Tcl_ExitThread(result);
#ifdef MAC_TCL
    return NULL;
#endif
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadErrorProc --
 *
 *    Send a message to the thread willing to hear about errors.
 *
 * Results:
 *    none
 *
 * Side effects:
 *    Send an event.
 *
 *------------------------------------------------------------------------
 */
static void
ThreadErrorProc(interp)
    Tcl_Interp *interp;		/* Interp that failed */
{
    Tcl_Channel errChannel;
    char *errorInfo, *script;
    char *argv[3];
    char buf[10];
    sprintf(buf, "%ld", (long) Tcl_GetCurrentThread());

    errorInfo = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (errorProcString == NULL) {
	errChannel = Tcl_GetStdChannel(TCL_STDERR);
	Tcl_WriteChars(errChannel, "Error from thread ", -1);
	Tcl_WriteChars(errChannel, buf, -1);
	Tcl_WriteChars(errChannel, "\n", 1);
	Tcl_WriteChars(errChannel, errorInfo, -1);
	Tcl_WriteChars(errChannel, "\n", 1);
    } else {
	argv[0] = errorProcString;
	argv[1] = buf;
	argv[2] = errorInfo;
	script = Tcl_Merge(3, argv);
	TclThreadSend(interp, errorThreadId, script, 0);
	ckfree(script);
    }
}


/*
 *------------------------------------------------------------------------
 *
 * ListUpdateInner --
 *
 *    Add the thread local storage to the list.  This assumes
 *	the caller has obtained the mutex.
 *
 * Results:
 *    none
 *
 * Side effects:
 *    Add the thread local storage to its list.
 *
 *------------------------------------------------------------------------
 */
static void
ListUpdateInner(tsdPtr)
    ThreadSpecificData *tsdPtr;
{
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
    }
    tsdPtr->threadId = Tcl_GetCurrentThread();
    tsdPtr->nextPtr = threadList;
    if (threadList) {
	threadList->prevPtr = tsdPtr;
    }
    tsdPtr->prevPtr = NULL;
    threadList = tsdPtr;
}

/*
 *------------------------------------------------------------------------
 *
 * ListRemove --
 *
 *    Remove the thread local storage from its list.  This grabs the
 *	mutex to protect the list.
 *
 * Results:
 *    none
 *
 * Side effects:
 *    Remove the thread local storage from its list.
 *
 *------------------------------------------------------------------------
 */
static void
ListRemove(tsdPtr)
    ThreadSpecificData *tsdPtr;
{
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
    }
    Tcl_MutexLock(&threadMutex);
    if (tsdPtr->prevPtr) {
	tsdPtr->prevPtr->nextPtr = tsdPtr->nextPtr;
    } else {
	threadList = tsdPtr->nextPtr;
    }
    if (tsdPtr->nextPtr) {
	tsdPtr->nextPtr->prevPtr = tsdPtr->prevPtr;
    }
    tsdPtr->nextPtr = tsdPtr->prevPtr = 0;
    Tcl_MutexUnlock(&threadMutex);
}


/*
 *------------------------------------------------------------------------
 *
 * TclThreadList --
 *
 *    Return a list of threads running Tcl interpreters.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------
 */
int
TclThreadList(interp)
    Tcl_Interp *interp;
{
    ThreadSpecificData *tsdPtr;
    Tcl_Obj *listPtr;

    listPtr = Tcl_NewListObj(0, NULL);
    Tcl_MutexLock(&threadMutex);
    for (tsdPtr = threadList ; tsdPtr ; tsdPtr = tsdPtr->nextPtr) {
	Tcl_ListObjAppendElement(interp, listPtr,
		Tcl_NewIntObj((int)tsdPtr->threadId));
    }
    Tcl_MutexUnlock(&threadMutex);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}


/*
 *------------------------------------------------------------------------
 *
 * TclThreadSend --
 *
 *    Send a script to another thread.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------
 */
int
TclThreadSend(interp, id, script, wait)
    Tcl_Interp *interp;		/* The current interpreter. */
    Tcl_ThreadId id;		/* Thread Id of other interpreter. */
    char *script;		/* The script to evaluate. */
    int wait;			/* If 1, we block for the result. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ThreadEvent *threadEventPtr;
    ThreadEventResult *resultPtr;
    int found, code;
    Tcl_ThreadId threadId = (Tcl_ThreadId) id;

    /* 
     * Verify the thread exists.
     */

    Tcl_MutexLock(&threadMutex);
    found = 0;
    for (tsdPtr = threadList ; tsdPtr ; tsdPtr = tsdPtr->nextPtr) {
	if (tsdPtr->threadId == threadId) {
	    found = 1;
	    break;
	}
    }
    if (!found) {
	Tcl_MutexUnlock(&threadMutex);
	Tcl_AppendResult(interp, "invalid thread id", NULL);
	return TCL_ERROR;
    }

    /*
     * Short circut sends to ourself.  Ought to do something with -async,
     * like run in an idle handler.
     */

    if (threadId == Tcl_GetCurrentThread()) {
        Tcl_MutexUnlock(&threadMutex);
	return Tcl_GlobalEval(interp, script);
    }

    /* 
     * Create the event for its event queue.
     */

    threadEventPtr = (ThreadEvent *) ckalloc(sizeof(ThreadEvent));
    threadEventPtr->script = ckalloc(strlen(script) + 1);
    strcpy(threadEventPtr->script, script);
    if (!wait) {
	resultPtr = threadEventPtr->resultPtr = NULL;
    } else {
	resultPtr = (ThreadEventResult *) ckalloc(sizeof(ThreadEventResult));
	threadEventPtr->resultPtr = resultPtr;

	/*
	 * Initialize the result fields.
	 */

	resultPtr->done = NULL;
	resultPtr->code = 0;
	resultPtr->result = NULL;
	resultPtr->errorInfo = NULL;
	resultPtr->errorCode = NULL;

	/* 
	 * Maintain the cleanup list.
	 */

	resultPtr->srcThreadId = Tcl_GetCurrentThread();
	resultPtr->dstThreadId = threadId;
	resultPtr->eventPtr = threadEventPtr;
	resultPtr->nextPtr = resultList;
	if (resultList) {
	    resultList->prevPtr = resultPtr;
	}
	resultPtr->prevPtr = NULL;
	resultList = resultPtr;
    }

    /*
     * Queue the event and poke the other thread's notifier.
     */

    threadEventPtr->event.proc = ThreadEventProc;
    Tcl_ThreadQueueEvent(threadId, (Tcl_Event *)threadEventPtr, 
	    TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(threadId);

    if (!wait) {
	Tcl_MutexUnlock(&threadMutex);
	return TCL_OK;
    }

    /* 
     * Block on the results and then get them.
     */

    Tcl_ResetResult(interp);
    while (resultPtr->result == NULL) {
        Tcl_ConditionWait(&resultPtr->done, &threadMutex, NULL);
    }

    /*
     * Unlink result from the result list.
     */

    if (resultPtr->prevPtr) {
	resultPtr->prevPtr->nextPtr = resultPtr->nextPtr;
    } else {
	resultList = resultPtr->nextPtr;
    }
    if (resultPtr->nextPtr) {
	resultPtr->nextPtr->prevPtr = resultPtr->prevPtr;
    }
    resultPtr->eventPtr = NULL;
    resultPtr->nextPtr = NULL;
    resultPtr->prevPtr = NULL;

    Tcl_MutexUnlock(&threadMutex);

    if (resultPtr->code != TCL_OK) {
	if (resultPtr->errorCode) {
	    Tcl_SetErrorCode(interp, resultPtr->errorCode, NULL);
	    ckfree(resultPtr->errorCode);
	}
	if (resultPtr->errorInfo) {
	    Tcl_AddErrorInfo(interp, resultPtr->errorInfo);
	    ckfree(resultPtr->errorInfo);
	}
    }
    Tcl_SetResult(interp, resultPtr->result, TCL_DYNAMIC);
    TclFinalizeCondition(&resultPtr->done);
    code = resultPtr->code;

    ckfree((char *) resultPtr);

    return code;
}


/*
 *------------------------------------------------------------------------
 *
 * ThreadEventProc --
 *
 *    Handle the event in the target thread.
 *
 * Results:
 *    Returns 1 to indicate that the event was processed.
 *
 * Side effects:
 *    Fills out the ThreadEventResult struct.
 *
 *------------------------------------------------------------------------
 */
int
ThreadEventProc(evPtr, mask)
    Tcl_Event *evPtr;		/* Really ThreadEvent */
    int mask;
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ThreadEvent *threadEventPtr = (ThreadEvent *)evPtr;
    ThreadEventResult *resultPtr = threadEventPtr->resultPtr;
    Tcl_Interp *interp = tsdPtr->interp;
    int code;
    char *result, *errorCode, *errorInfo;

    if (interp == NULL) {
	code = TCL_ERROR;
	result = "no target interp!";
	errorCode = "THREAD";
	errorInfo = "";
    } else {
	Tcl_Preserve((ClientData) interp);
	Tcl_ResetResult(interp);
	Tcl_CreateThreadExitHandler(ThreadFreeProc,
		(ClientData) threadEventPtr->script);
	code = Tcl_GlobalEval(interp, threadEventPtr->script);
	Tcl_DeleteThreadExitHandler(ThreadFreeProc,
		(ClientData) threadEventPtr->script);
	result = Tcl_GetStringResult(interp);
	if (code != TCL_OK) {
	    errorCode = Tcl_GetVar(interp, "errorCode", TCL_GLOBAL_ONLY);
	    errorInfo = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
	} else {
	    errorCode = errorInfo = NULL;
	}
    }
    ckfree(threadEventPtr->script);
    if (resultPtr) {
	Tcl_MutexLock(&threadMutex);
	resultPtr->code = code;
	resultPtr->result = ckalloc(strlen(result) + 1);
	strcpy(resultPtr->result, result);
	if (errorCode != NULL) {
	    resultPtr->errorCode = ckalloc(strlen(errorCode) + 1);
	    strcpy(resultPtr->errorCode, errorCode);
	}
	if (errorInfo != NULL) {
	    resultPtr->errorInfo = ckalloc(strlen(errorInfo) + 1);
	    strcpy(resultPtr->errorInfo, errorInfo);
	}
	Tcl_ConditionNotify(&resultPtr->done);
	Tcl_MutexUnlock(&threadMutex);
    }
    if (interp != NULL) {
	Tcl_Release((ClientData) interp);
    }
    return 1;
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadFreeProc --
 *
 *    This is called from when we are exiting and memory needs
 *    to be freed.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *	Clears up mem specified in ClientData
 *
 *------------------------------------------------------------------------
 */
     /* ARGSUSED */
void
ThreadFreeProc(clientData)
    ClientData clientData;
{
    if (clientData) {
	ckfree((char *) clientData);
    }
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadDeleteEvent --
 *
 *    This is called from the ThreadExitProc to delete memory related
 *    to events that we put on the queue.
 *
 * Results:
 *    1 it was our event and we want it removed, 0 otherwise.
 *
 * Side effects:
 *	It cleans up our events in the event queue for this thread.
 *
 *------------------------------------------------------------------------
 */
     /* ARGSUSED */
int
ThreadDeleteEvent(eventPtr, clientData)
    Tcl_Event *eventPtr;		/* Really ThreadEvent */
    ClientData clientData;		/* dummy */
{
    if (eventPtr->proc == ThreadEventProc) {
	ckfree((char *) ((ThreadEvent *) eventPtr)->script);
	return 1;
    }
    /*
     * If it was NULL, we were in the middle of servicing the event
     * and it should be removed
     */
    return (eventPtr->proc == NULL);
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadExitProc --
 *
 *    This is called when the thread exits.  
 *
 * Results:
 *    None.
 *
 * Side effects:
 *	It unblocks anyone that is waiting on a send to this thread.
 *	It cleans up any events in the event queue for this thread.
 *
 *------------------------------------------------------------------------
 */
     /* ARGSUSED */
void
ThreadExitProc(clientData)
    ClientData clientData;
{
    char *threadEvalScript = (char *) clientData;
    ThreadEventResult *resultPtr, *nextPtr;
    Tcl_ThreadId self = Tcl_GetCurrentThread();

    Tcl_MutexLock(&threadMutex);

    if (threadEvalScript) {
	ckfree((char *) threadEvalScript);
	threadEvalScript = NULL;
    }
    Tcl_DeleteEvents((Tcl_EventDeleteProc *)ThreadDeleteEvent, NULL);

    for (resultPtr = resultList ; resultPtr ; resultPtr = nextPtr) {
	nextPtr = resultPtr->nextPtr;
	if (resultPtr->srcThreadId == self) {
	    /*
	     * We are going away.  By freeing up the result we signal
	     * to the other thread we don't care about the result.
	     */
	    if (resultPtr->prevPtr) {
		resultPtr->prevPtr->nextPtr = resultPtr->nextPtr;
	    } else {
		resultList = resultPtr->nextPtr;
	    }
	    if (resultPtr->nextPtr) {
		resultPtr->nextPtr->prevPtr = resultPtr->prevPtr;
	    }
	    resultPtr->nextPtr = resultPtr->prevPtr = 0;
	    resultPtr->eventPtr->resultPtr = NULL;
	    ckfree((char *)resultPtr);
	} else if (resultPtr->dstThreadId == self) {
	    /*
	     * Dang.  The target is going away.  Unblock the caller.
	     * The result string must be dynamically allocated because
	     * the main thread is going to call free on it.
	     */

	    char *msg = "target thread died";
	    resultPtr->result = ckalloc(strlen(msg)+1);
	    strcpy(resultPtr->result, msg);
	    resultPtr->code = TCL_ERROR;
	    Tcl_ConditionNotify(&resultPtr->done);
	}
    }
    Tcl_MutexUnlock(&threadMutex);
}

#endif /* TCL_THREADS */
