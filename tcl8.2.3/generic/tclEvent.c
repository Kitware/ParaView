/* 
 * tclEvent.c --
 *
 *	This file implements some general event related interfaces including
 *	background errors, exit handlers, and the "vwait" and "update"
 *	command procedures. 
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * The data structure below is used to report background errors.  One
 * such structure is allocated for each error;  it holds information
 * about the interpreter and the error until bgerror can be invoked
 * later as an idle handler.
 */

typedef struct BgError {
    Tcl_Interp *interp;		/* Interpreter in which error occurred.  NULL
				 * means this error report has been cancelled
				 * (a previous report generated a break). */
    char *errorMsg;		/* Copy of the error message (the interp's
				 * result when the error occurred).
				 * Malloc-ed. */
    char *errorInfo;		/* Value of the errorInfo variable
				 * (malloc-ed). */
    char *errorCode;		/* Value of the errorCode variable
				 * (malloc-ed). */
    struct BgError *nextPtr;	/* Next in list of all pending error
				 * reports for this interpreter, or NULL
				 * for end of list. */
} BgError;

/*
 * One of the structures below is associated with the "tclBgError"
 * assoc data for each interpreter.  It keeps track of the head and
 * tail of the list of pending background errors for the interpreter.
 */

typedef struct ErrAssocData {
    BgError *firstBgPtr;	/* First in list of all background errors
				 * waiting to be processed for this
				 * interpreter (NULL if none). */
    BgError *lastBgPtr;		/* Last in list of all background errors
				 * waiting to be processed for this
				 * interpreter (NULL if none). */
} ErrAssocData;

/*
 * For each exit handler created with a call to Tcl_CreateExitHandler
 * there is a structure of the following type:
 */

typedef struct ExitHandler {
    Tcl_ExitProc *proc;		/* Procedure to call when process exits. */
    ClientData clientData;	/* One word of information to pass to proc. */
    struct ExitHandler *nextPtr;/* Next in list of all exit handlers for
				 * this application, or NULL for end of list. */
} ExitHandler;

/*
 * There is both per-process and per-thread exit handlers.
 * The first list is controlled by a mutex.  The other is in
 * thread local storage.
 */

static ExitHandler *firstExitPtr = NULL;
				/* First in list of all exit handlers for
				 * application. */
TCL_DECLARE_MUTEX(exitMutex)

/*
 * This variable is set to 1 when Tcl_Finalize is called, and at the end of
 * its work, it is reset to 0. The variable is checked by TclInExit() to
 * allow different behavior for exit-time processing, e.g. in closing of
 * files and pipes.
 */

static int inFinalize = 0;
static int subsystemsInitialized = 0;
static int encodingsInitialized  = 0;

static Tcl_Obj *tclLibraryPath = NULL;

typedef struct ThreadSpecificData {
    ExitHandler *firstExitPtr;  /* First in list of all exit handlers for
				 * this thread. */
    int inExit;			/* True when this thread is exiting. This
				 * is used as a hack to decide to close
				 * the standard channels. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Prototypes for procedures referenced only in this file:
 */

static void		BgErrorDeleteProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp));
static void		HandleBgErrors _ANSI_ARGS_((ClientData clientData));
static char *		VwaitVarProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));

/*
 *----------------------------------------------------------------------
 *
 * Tcl_BackgroundError --
 *
 *	This procedure is invoked to handle errors that occur in Tcl
 *	commands that are invoked in "background" (e.g. from event or
 *	timer bindings).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The command "bgerror" is invoked later as an idle handler to
 *	process the error, passing it the error message.  If that fails,
 *	then an error message is output on stderr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_BackgroundError(interp)
    Tcl_Interp *interp;		/* Interpreter in which an error has
				 * occurred. */
{
    BgError *errPtr;
    char *errResult, *varValue;
    ErrAssocData *assocPtr;
    int length;

    /*
     * The Tcl_AddErrorInfo call below (with an empty string) ensures that
     * errorInfo gets properly set.  It's needed in cases where the error
     * came from a utility procedure like Tcl_GetVar instead of Tcl_Eval;
     * in these cases errorInfo still won't have been set when this
     * procedure is called.
     */

    Tcl_AddErrorInfo(interp, "");

    errResult = Tcl_GetStringFromObj(Tcl_GetObjResult(interp), &length);
	
    errPtr = (BgError *) ckalloc(sizeof(BgError));
    errPtr->interp = interp;
    errPtr->errorMsg = (char *) ckalloc((unsigned) (length + 1));
    memcpy(errPtr->errorMsg, errResult, (size_t) (length + 1));
    varValue = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (varValue == NULL) {
	varValue = errPtr->errorMsg;
    }
    errPtr->errorInfo = (char *) ckalloc((unsigned) (strlen(varValue) + 1));
    strcpy(errPtr->errorInfo, varValue);
    varValue = Tcl_GetVar(interp, "errorCode", TCL_GLOBAL_ONLY);
    if (varValue == NULL) {
	varValue = "";
    }
    errPtr->errorCode = (char *) ckalloc((unsigned) (strlen(varValue) + 1));
    strcpy(errPtr->errorCode, varValue);
    errPtr->nextPtr = NULL;

    assocPtr = (ErrAssocData *) Tcl_GetAssocData(interp, "tclBgError",
	    (Tcl_InterpDeleteProc **) NULL);
    if (assocPtr == NULL) {

	/*
	 * This is the first time a background error has occurred in
	 * this interpreter.  Create associated data to keep track of
	 * pending error reports.
	 */

	assocPtr = (ErrAssocData *) ckalloc(sizeof(ErrAssocData));
	assocPtr->firstBgPtr = NULL;
	assocPtr->lastBgPtr = NULL;
	Tcl_SetAssocData(interp, "tclBgError", BgErrorDeleteProc,
		(ClientData) assocPtr);
    }
    if (assocPtr->firstBgPtr == NULL) {
	assocPtr->firstBgPtr = errPtr;
	Tcl_DoWhenIdle(HandleBgErrors, (ClientData) assocPtr);
    } else {
	assocPtr->lastBgPtr->nextPtr = errPtr;
    }
    assocPtr->lastBgPtr = errPtr;
    Tcl_ResetResult(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * HandleBgErrors --
 *
 *	This procedure is invoked as an idle handler to process all of
 *	the accumulated background errors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what actions "bgerror" takes for the errors.
 *
 *----------------------------------------------------------------------
 */

static void
HandleBgErrors(clientData)
    ClientData clientData;	/* Pointer to ErrAssocData structure. */
{
    Tcl_Interp *interp;
    char *argv[2];
    int code;
    BgError *errPtr;
    ErrAssocData *assocPtr = (ErrAssocData *) clientData;
    Tcl_Channel errChannel;

    Tcl_Preserve((ClientData) assocPtr);
    
    while (assocPtr->firstBgPtr != NULL) {
	interp = assocPtr->firstBgPtr->interp;
	if (interp == NULL) {
	    goto doneWithInterp;
	}

	/*
	 * Restore important state variables to what they were at
	 * the time the error occurred.
	 */

	Tcl_SetVar(interp, "errorInfo", assocPtr->firstBgPtr->errorInfo,
		TCL_GLOBAL_ONLY);
	Tcl_SetVar(interp, "errorCode", assocPtr->firstBgPtr->errorCode,
		TCL_GLOBAL_ONLY);

	/*
	 * Create and invoke the bgerror command.
	 */

	argv[0] = "bgerror";
	argv[1] = assocPtr->firstBgPtr->errorMsg;
	
	Tcl_AllowExceptions(interp);
        Tcl_Preserve((ClientData) interp);
	code = TclGlobalInvoke(interp, 2, argv, 0);
	if (code == TCL_ERROR) {

            /*
             * If the interpreter is safe, we look for a hidden command
             * named "bgerror" and call that with the error information.
             * Otherwise, simply ignore the error. The rationale is that
             * this could be an error caused by a malicious applet trying
             * to cause an infinite barrage of error messages. The hidden
             * "bgerror" command can be used by a security policy to
             * interpose on such attacks and e.g. kill the applet after a
             * few attempts.
             */

            if (Tcl_IsSafe(interp)) {
		Tcl_SavedResult save;
		
		Tcl_SaveResult(interp, &save);
                TclGlobalInvoke(interp, 2, argv, TCL_INVOKE_HIDDEN);
		Tcl_RestoreResult(interp, &save);

                goto doneWithInterp;
            } 

            /*
             * We have to get the error output channel at the latest possible
             * time, because the eval (above) might have changed the channel.
             */
            
            errChannel = Tcl_GetStdChannel(TCL_STDERR);
            if (errChannel != (Tcl_Channel) NULL) {
		char *string;
		int len;

		string = Tcl_GetStringFromObj(Tcl_GetObjResult(interp), &len);
                if (strcmp(string, "\"bgerror\" is an invalid command name or ambiguous abbreviation") == 0) {
                    Tcl_WriteChars(errChannel, assocPtr->firstBgPtr->errorInfo, -1);
                    Tcl_WriteChars(errChannel, "\n", -1);
                } else {
                    Tcl_WriteChars(errChannel,
                            "bgerror failed to handle background error.\n",
                            -1);
                    Tcl_WriteChars(errChannel, "    Original error: ", -1);
                    Tcl_WriteChars(errChannel, assocPtr->firstBgPtr->errorMsg,
                            -1);
                    Tcl_WriteChars(errChannel, "\n", -1);
                    Tcl_WriteChars(errChannel, "    Error in bgerror: ", -1);
                    Tcl_WriteChars(errChannel, string, len);
                    Tcl_WriteChars(errChannel, "\n", -1);
                }
                Tcl_Flush(errChannel);
            }
	} else if (code == TCL_BREAK) {

	    /*
	     * Break means cancel any remaining error reports for this
	     * interpreter.
	     */

	    for (errPtr = assocPtr->firstBgPtr; errPtr != NULL;
		    errPtr = errPtr->nextPtr) {
		if (errPtr->interp == interp) {
		    errPtr->interp = NULL;
		}
	    }
	}

	/*
	 * Discard the command and the information about the error report.
	 */

doneWithInterp:

	if (assocPtr->firstBgPtr) {
	    ckfree(assocPtr->firstBgPtr->errorMsg);
	    ckfree(assocPtr->firstBgPtr->errorInfo);
	    ckfree(assocPtr->firstBgPtr->errorCode);
	    errPtr = assocPtr->firstBgPtr->nextPtr;
	    ckfree((char *) assocPtr->firstBgPtr);
	    assocPtr->firstBgPtr = errPtr;
	}
        
        if (interp != NULL) {
            Tcl_Release((ClientData) interp);
        }
    }
    assocPtr->lastBgPtr = NULL;

    Tcl_Release((ClientData) assocPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * BgErrorDeleteProc --
 *
 *	This procedure is associated with the "tclBgError" assoc data
 *	for an interpreter;  it is invoked when the interpreter is
 *	deleted in order to free the information assoicated with any
 *	pending error reports.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Background error information is freed: if there were any
 *	pending error reports, they are cancelled.
 *
 *----------------------------------------------------------------------
 */

static void
BgErrorDeleteProc(clientData, interp)
    ClientData clientData;	/* Pointer to ErrAssocData structure. */
    Tcl_Interp *interp;		/* Interpreter being deleted. */
{
    ErrAssocData *assocPtr = (ErrAssocData *) clientData;
    BgError *errPtr;

    while (assocPtr->firstBgPtr != NULL) {
	errPtr = assocPtr->firstBgPtr;
	assocPtr->firstBgPtr = errPtr->nextPtr;
	ckfree(errPtr->errorMsg);
	ckfree(errPtr->errorInfo);
	ckfree(errPtr->errorCode);
	ckfree((char *) errPtr);
    }
    Tcl_CancelIdleCall(HandleBgErrors, (ClientData) assocPtr);
    Tcl_EventuallyFree((ClientData) assocPtr, TCL_DYNAMIC);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateExitHandler --
 *
 *	Arrange for a given procedure to be invoked just before the
 *	application exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc will be invoked with clientData as argument when the
 *	application exits.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateExitHandler(proc, clientData)
    Tcl_ExitProc *proc;		/* Procedure to invoke. */
    ClientData clientData;	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;

    exitPtr = (ExitHandler *) ckalloc(sizeof(ExitHandler));
    exitPtr->proc = proc;
    exitPtr->clientData = clientData;
    Tcl_MutexLock(&exitMutex);
    exitPtr->nextPtr = firstExitPtr;
    firstExitPtr = exitPtr;
    Tcl_MutexUnlock(&exitMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteExitHandler --
 *
 *	This procedure cancels an existing exit handler matching proc
 *	and clientData, if such a handler exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is an exit handler corresponding to proc and clientData
 *	then it is cancelled;  if no such handler exists then nothing
 *	happens.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteExitHandler(proc, clientData)
    Tcl_ExitProc *proc;		/* Procedure that was previously registered. */
    ClientData clientData;	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;

    Tcl_MutexLock(&exitMutex);
    for (prevPtr = NULL, exitPtr = firstExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		firstExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree((char *) exitPtr);
	    break;
	}
    }
    Tcl_MutexUnlock(&exitMutex);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateThreadExitHandler --
 *
 *	Arrange for a given procedure to be invoked just before the
 *	current thread exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc will be invoked with clientData as argument when the
 *	application exits.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateThreadExitHandler(proc, clientData)
    Tcl_ExitProc *proc;		/* Procedure to invoke. */
    ClientData clientData;	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    exitPtr = (ExitHandler *) ckalloc(sizeof(ExitHandler));
    exitPtr->proc = proc;
    exitPtr->clientData = clientData;
    exitPtr->nextPtr = tsdPtr->firstExitPtr;
    tsdPtr->firstExitPtr = exitPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteThreadExitHandler --
 *
 *	This procedure cancels an existing exit handler matching proc
 *	and clientData, if such a handler exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is an exit handler corresponding to proc and clientData
 *	then it is cancelled;  if no such handler exists then nothing
 *	happens.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteThreadExitHandler(proc, clientData)
    Tcl_ExitProc *proc;		/* Procedure that was previously registered. */
    ClientData clientData;	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    for (prevPtr = NULL, exitPtr = tsdPtr->firstExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		tsdPtr->firstExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree((char *) exitPtr);
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Exit --
 *
 *	This procedure is called to terminate the application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All existing exit handlers are invoked, then the application
 *	ends.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Exit(status)
    int status;			/* Exit status for application;  typically
				 * 0 for normal return, 1 for error return. */
{
    Tcl_Finalize();
    TclpExit(status);
}

/*
 *-------------------------------------------------------------------------
 * 
 * TclSetLibraryPath --
 *
 *	Set the path that will be used for searching for init.tcl and 
 *	encodings when an interp is being created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changing the library path will affect what directories are
 *	examined when looking for encodings for all interps from that
 *	point forward.
 *
 *	The refcount of the new library path is incremented and the 
 *	refcount of the old path is decremented.
 *
 *-------------------------------------------------------------------------
 */

void
TclSetLibraryPath(pathPtr)
    Tcl_Obj *pathPtr;		/* A Tcl list object whose elements are
				 * the new library path. */
{
    Tcl_MutexLock(&exitMutex);
    if (pathPtr != NULL) {
	Tcl_IncrRefCount(pathPtr);
    }
    if (tclLibraryPath != NULL) {
	Tcl_DecrRefCount(tclLibraryPath);
    }
    tclLibraryPath = pathPtr;
    Tcl_MutexUnlock(&exitMutex);
}

/*
 *-------------------------------------------------------------------------
 *
 * TclGetLibraryPath --
 *
 *	Return a Tcl list object whose elements are the library path.
 *	The caller should not modify the contents of the returned object.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

Tcl_Obj *
TclGetLibraryPath()
{
    return tclLibraryPath;
}

/*
 *-------------------------------------------------------------------------
 *
 * TclInitSubsystems --
 *
 *	Initialize various subsytems in Tcl.  This should be called the
 *	first time an interp is created, or before any of the subsystems
 *	are used.  This function ensures an order for the initialization 
 *	of subsystems:
 *
 *	1. that cannot be initialized in lazy order because they are 
 *	mutually dependent.
 *
 *	2. so that they can be finalized in a known order w/o causing
 *	the subsequent re-initialization of a subsystem in the act of
 *	shutting down another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective initialization routines.
 *
 *-------------------------------------------------------------------------
 */

void
TclInitSubsystems(argv0)
    CONST char *argv0;		/* Name of executable from argv[0] to main()
				 * in native multi-byte encoding. */
{
    ThreadSpecificData *tsdPtr;

    if (inFinalize != 0) {
	panic("TclInitSubsystems called while finalizing");
    }

    /*
     * Grab the thread local storage pointer before doing anything because
     * the initialization routines will be registering exit handlers.
     * We use this pointer to detect if this is the first time this
     * thread has created an interpreter.
     */

    tsdPtr = (ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);
    if (subsystemsInitialized == 0) {
	/* 
	 * Double check inside the mutex.  There are definitly calls
	 * back into this routine from some of the procedures below.
	 */

	TclpInitLock();
	if (subsystemsInitialized == 0) {
	    /*
	     * Have to set this bit here to avoid deadlock with the
	     * routines below us that call into TclInitSubsystems.
	     */

	    subsystemsInitialized = 1;

	    tclExecutableName = NULL;

	    /*
	     * Initialize locks used by the memory allocators before anything
	     * interesting happens so we can use the allocators in the
	     * implementation of self-initializing locks.
	     */
#if USE_TCLALLOC
	    TclInitAlloc();
#endif
#ifdef TCL_MEM_DEBUG
	    TclInitDbCkalloc();
#endif

	    TclpInitPlatform();
    	    TclInitObjSubsystem();
	    TclInitIOSubsystem();
	    TclInitEncodingSubsystem();
    	    TclInitNamespaceSubsystem();
	}
	TclpInitUnlock();
    }

    if (tsdPtr == NULL) {
	/*
	 * First time this thread has created an interpreter.
	 * We fetch the key again just in case no exit handlers were
	 * registered by this point.
	 */

	(void) TCL_TSD_INIT(&dataKey);
	TclInitNotifier();
     }
}

/*
 *-------------------------------------------------------------------------
 *
 * TclFindEncodings --
 *
 *	Find and load the encoding file for this operating system.
 *	Before this is called, Tcl makes assumptions about the
 *	native string representation, but the true encoding is not
 *	assured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective initialization routines.
 *
 *-------------------------------------------------------------------------
 */

void
TclFindEncodings(argv0)
    CONST char *argv0;		/* Name of executable from argv[0] to main()
				 * in native multi-byte encoding. */
{
    char *native;
    Tcl_Obj *pathPtr;
    Tcl_DString libPath, buffer;

    if (encodingsInitialized == 0) {
	/* 
	 * Double check inside the mutex.  There may be calls
	 * back into this routine from some of the procedures below.
	 */

	TclpInitLock();
	if (encodingsInitialized == 0) {
	    /*
	     * Have to set this bit here to avoid deadlock with the
	     * routines below us that call into TclInitSubsystems.
	     */

	    encodingsInitialized = 1;

	    native = TclpFindExecutable(argv0);
	    TclpInitLibraryPath(native);

	    /*
	     * The library path was set in the TclpInitLibraryPath routine.
	     * The string set is a dirty UTF string.  To preserve the value
	     * convert the UTF string back to native before setting the new
	     * default encoding.
	     */
	    
	    pathPtr = TclGetLibraryPath();
	    if (pathPtr != NULL) {
		Tcl_UtfToExternalDString(NULL, Tcl_GetString(pathPtr), -1,
			&libPath);
	    }

	    TclpSetInitialEncodings();

	    /*
	     * Now convert the native string back to UTF.
	     */
	     
	    if (pathPtr != NULL) {
		Tcl_ExternalToUtfDString(NULL, Tcl_DStringValue(&libPath), -1,
			&buffer);
		pathPtr = Tcl_NewStringObj(Tcl_DStringValue(&buffer), -1);
		TclSetLibraryPath(pathPtr);

		Tcl_DStringFree(&libPath);
		Tcl_DStringFree(&buffer);
	    }
	}
	TclpInitUnlock();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Finalize --
 *
 *	Shut down Tcl.  First calls registered exit handlers, then
 *	carefully shuts down various subsystems.
 *	Called by Tcl_Exit or when the Tcl shared library is being 
 *	unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective finalization routines.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Finalize()
{
    ExitHandler *exitPtr;

    TclpInitLock();
    if (subsystemsInitialized != 0) {
	subsystemsInitialized = 0;
	encodingsInitialized  = 0;

	/*
	 * Invoke exit handlers first.
	 */

	Tcl_MutexLock(&exitMutex);
	inFinalize = 1;
	for (exitPtr = firstExitPtr; exitPtr != NULL; exitPtr = firstExitPtr) {
	    /*
	     * Be careful to remove the handler from the list before
	     * invoking its callback.  This protects us against
	     * double-freeing if the callback should call
	     * Tcl_DeleteExitHandler on itself.
	     */

	    firstExitPtr = exitPtr->nextPtr;
	    Tcl_MutexUnlock(&exitMutex);
	    (*exitPtr->proc)(exitPtr->clientData);
	    ckfree((char *) exitPtr);
	    Tcl_MutexLock(&exitMutex);
	}    
	firstExitPtr = NULL;
	Tcl_MutexUnlock(&exitMutex);

	/*
	 * Clean up after the current thread now, after exit handlers.
	 * In particular, the testexithandler command sets up something
	 * that writes to standard output, which gets closed.
	 * Note that there is no thread-local storage after this call.
	 */
    
	Tcl_FinalizeThread();

	/*
	 * Now finalize the Tcl execution environment.  Note that this
	 * must be done after the exit handlers, because there are
	 * order dependencies.
	 */

	TclFinalizeCompExecEnv();
	TclFinalizeEnvironment();

	TclFinalizeEncodingSubsystem();

	if (tclLibraryPath != NULL) {
	    Tcl_DecrRefCount(tclLibraryPath);
	    tclLibraryPath = NULL;
	}
	if (tclExecutableName != NULL) {
	    ckfree(tclExecutableName);
	    tclExecutableName = NULL;
	}
	if (tclNativeExecutableName != NULL) {
	    ckfree(tclNativeExecutableName);
	    tclNativeExecutableName = NULL;
	}
	if (tclDefaultEncodingDir != NULL) {
	    ckfree(tclDefaultEncodingDir);
	    tclDefaultEncodingDir = NULL;
	}
	
	Tcl_SetPanicProc(NULL);

	/*
	 * Free synchronization objects.  There really should only be one
	 * thread alive at this moment.
	 */

	TclFinalizeSynchronization();

	/*
	 * We defer unloading of packages until very late 
	 * to avoid memory access issues.  Both exit callbacks and
	 * synchronization variables may be stored in packages.
	 */

	TclFinalizeLoad();

	/*
	 * There shouldn't be any malloc'ed memory after this.
	 */

	TclFinalizeMemorySubsystem();
	inFinalize = 0;
    }
    TclpInitUnlock();
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FinalizeThread --
 *
 *	Runs the exit handlers to allow Tcl to clean up its state
 *	about a particular thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective finalization routines.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FinalizeThread()
{
    ExitHandler *exitPtr;
    ThreadSpecificData *tsdPtr =
	    (ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);

    if (tsdPtr != NULL) {
	/*
	 * Invoke thread exit handlers first.
	 */

	tsdPtr->inExit = 1;
	for (exitPtr = tsdPtr->firstExitPtr; exitPtr != NULL;
		exitPtr = tsdPtr->firstExitPtr) {
	    /*
	     * Be careful to remove the handler from the list before invoking
	     * its callback.  This protects us against double-freeing if the
	     * callback should call Tcl_DeleteThreadExitHandler on itself.
	     */

	    tsdPtr->firstExitPtr = exitPtr->nextPtr;
	    (*exitPtr->proc)(exitPtr->clientData);
	    ckfree((char *) exitPtr);
	}
	TclFinalizeIOSubsystem();
	TclFinalizeNotifier();

	/*
	 * Blow away all thread local storage blocks.
	 */

	TclFinalizeThreadData();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclInExit --
 *
 *	Determines if we are in the middle of exit-time cleanup.
 *
 * Results:
 *	If we are in the middle of exiting, 1, otherwise 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclInExit()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    return tsdPtr->inExit;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VwaitObjCmd --
 *
 *	This procedure is invoked to process the "vwait" Tcl command.
 *	See the user documentation for details on what it does.
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
Tcl_VwaitObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int done, foundEvent;
    char *nameString;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "name");
	return TCL_ERROR;
    }
    nameString = Tcl_GetString(objv[1]);
    if (Tcl_TraceVar(interp, nameString,
	    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    VwaitVarProc, (ClientData) &done) != TCL_OK) {
	return TCL_ERROR;
    };
    done = 0;
    foundEvent = 1;
    while (!done && foundEvent) {
	foundEvent = Tcl_DoOneEvent(TCL_ALL_EVENTS);
    }
    Tcl_UntraceVar(interp, nameString,
	    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    VwaitVarProc, (ClientData) &done);

    /*
     * Clear out the interpreter's result, since it may have been set
     * by event handlers.
     */

    Tcl_ResetResult(interp);
    if (!foundEvent) {
	Tcl_AppendResult(interp, "can't wait for variable \"", nameString,
		"\":  would wait forever", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

	/* ARGSUSED */
static char *
VwaitVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Pointer to integer to set to 1. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    int *donePtr = (int *) clientData;

    *donePtr = 1;
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpdateObjCmd --
 *
 *	This procedure is invoked to process the "update" Tcl command.
 *	See the user documentation for details on what it does.
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
Tcl_UpdateObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int optionIndex;
    int flags = 0;		/* Initialized to avoid compiler warning. */
    static char *updateOptions[] = {"idletasks", (char *) NULL};
    enum updateOptions {REGEXP_IDLETASKS};

    if (objc == 1) {
	flags = TCL_ALL_EVENTS|TCL_DONT_WAIT;
    } else if (objc == 2) {
	if (Tcl_GetIndexFromObj(interp, objv[1], updateOptions,
		"option", 0, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum updateOptions) optionIndex) {
	    case REGEXP_IDLETASKS: {
		flags = TCL_WINDOW_EVENTS|TCL_IDLE_EVENTS|TCL_DONT_WAIT;
		break;
	    }
	    default: {
		panic("Tcl_UpdateObjCmd: bad option index to UpdateOptions");
	    }
	}
    } else {
        Tcl_WrongNumArgs(interp, 1, objv, "?idletasks?");
	return TCL_ERROR;
    }
    
    while (Tcl_DoOneEvent(flags) != 0) {
	/* Empty loop body */
    }

    /*
     * Must clear the interpreter's result because event handlers could
     * have executed commands.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}
