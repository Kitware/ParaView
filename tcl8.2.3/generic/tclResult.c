/* 
 * tclResult.c --
 *
 *	This file contains code to manage the interpreter result.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"

/*
 * Function prototypes for local procedures in this file:
 */

static void             ResetObjResult _ANSI_ARGS_((Interp *iPtr));
static void		SetupAppendBuffer _ANSI_ARGS_((Interp *iPtr,
			    int newSpace));


/*
 *----------------------------------------------------------------------
 *
 * Tcl_SaveResult --
 *
 *      Takes a snapshot of the current result state of the interpreter.
 *      The snapshot can be restored at any point by
 *      Tcl_RestoreResult. Note that this routine does not 
 *	preserve the errorCode, errorInfo, or flags fields so it
 *	should not be used if an error is in progress.
 *
 *      Once a snapshot is saved, it must be restored by calling
 *      Tcl_RestoreResult, or discarded by calling
 *      Tcl_DiscardResult.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the interpreter result.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SaveResult(interp, statePtr)
    Tcl_Interp *interp;		/* Interpreter to save. */
    Tcl_SavedResult *statePtr;	/* Pointer to state structure. */
{
    Interp *iPtr = (Interp *) interp;

    /*
     * Move the result object into the save state.  Note that we don't need
     * to change its refcount because we're moving it, not adding a new
     * reference.  Put an empty object into the interpreter.
     */

    statePtr->objResultPtr = iPtr->objResultPtr;
    iPtr->objResultPtr = Tcl_NewObj(); 
    Tcl_IncrRefCount(iPtr->objResultPtr); 

    /*
     * Save the string result. 
     */

    statePtr->freeProc = iPtr->freeProc;
    if (iPtr->result == iPtr->resultSpace) {
	/*
	 * Copy the static string data out of the interp buffer.
	 */

	statePtr->result = statePtr->resultSpace;
	strcpy(statePtr->result, iPtr->result);
	statePtr->appendResult = NULL;
    } else if (iPtr->result == iPtr->appendResult) {
	/*
	 * Move the append buffer out of the interp.
	 */

	statePtr->appendResult = iPtr->appendResult;
	statePtr->appendAvl = iPtr->appendAvl;
	statePtr->appendUsed = iPtr->appendUsed;
	statePtr->result = statePtr->appendResult;
	iPtr->appendResult = NULL;
	iPtr->appendAvl = 0;
	iPtr->appendUsed = 0;
    } else {
	/*
	 * Move the dynamic or static string out of the interpreter.
	 */

	statePtr->result = iPtr->result;
	statePtr->appendResult = NULL;
    }

    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
    iPtr->freeProc = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RestoreResult --
 *
 *      Restores the state of the interpreter to a snapshot taken
 *      by Tcl_SaveResult.  After this call, the token for
 *      the interpreter state is no longer valid.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Restores the interpreter result.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_RestoreResult(interp, statePtr)
    Tcl_Interp* interp;		/* Interpreter being restored. */
    Tcl_SavedResult *statePtr;	/* State returned by Tcl_SaveResult. */
{
    Interp *iPtr = (Interp *) interp;

    Tcl_ResetResult(interp);

    /*
     * Restore the string result.
     */

    iPtr->freeProc = statePtr->freeProc;
    if (statePtr->result == statePtr->resultSpace) {
	/*
	 * Copy the static string data into the interp buffer.
	 */

	iPtr->result = iPtr->resultSpace;
	strcpy(iPtr->result, statePtr->result);
    } else if (statePtr->result == statePtr->appendResult) {
	/*
	 * Move the append buffer back into the interp.
	 */

	if (iPtr->appendResult != NULL) {
	    ckfree((char *)iPtr->appendResult);
	}

	iPtr->appendResult = statePtr->appendResult;
	iPtr->appendAvl = statePtr->appendAvl;
	iPtr->appendUsed = statePtr->appendUsed;
	iPtr->result = iPtr->appendResult;
    } else {
	/*
	 * Move the dynamic or static string back into the interpreter.
	 */

	iPtr->result = statePtr->result;
    }

    /*
     * Restore the object result.
     */

    Tcl_DecrRefCount(iPtr->objResultPtr);
    iPtr->objResultPtr = statePtr->objResultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DiscardResult --
 *
 *      Frees the memory associated with an interpreter snapshot
 *      taken by Tcl_SaveResult.  If the snapshot is not
 *      restored, this procedure must be called to discard it,
 *      or the memory will be lost.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DiscardResult(statePtr)
    Tcl_SavedResult *statePtr;	/* State returned by Tcl_SaveResult. */
{
    TclDecrRefCount(statePtr->objResultPtr);

    if (statePtr->result == statePtr->appendResult) {
	ckfree(statePtr->appendResult);
    } else if (statePtr->freeProc) {
	if ((statePtr->freeProc == TCL_DYNAMIC)
	        || (statePtr->freeProc == (Tcl_FreeProc *) free)) {
	    ckfree(statePtr->result);
	} else {
	    (*statePtr->freeProc)(statePtr->result);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetResult --
 *
 *	Arrange for "string" to be the Tcl return value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	interp->result is left pointing either to "string" (if "copy" is 0)
 *	or to a copy of string. Also, the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetResult(interp, string, freeProc)
    Tcl_Interp *interp;		/* Interpreter with which to associate the
				 * return value. */
    register char *string;	/* Value to be returned.  If NULL, the
				 * result is set to an empty string. */
    Tcl_FreeProc *freeProc;	/* Gives information about the string:
				 * TCL_STATIC, TCL_VOLATILE, or the address
				 * of a Tcl_FreeProc such as free. */
{
    Interp *iPtr = (Interp *) interp;
    int length;
    register Tcl_FreeProc *oldFreeProc = iPtr->freeProc;
    char *oldResult = iPtr->result;

    if (string == NULL) {
	iPtr->resultSpace[0] = 0;
	iPtr->result = iPtr->resultSpace;
	iPtr->freeProc = 0;
    } else if (freeProc == TCL_VOLATILE) {
	length = strlen(string);
	if (length > TCL_RESULT_SIZE) {
	    iPtr->result = (char *) ckalloc((unsigned) length+1);
	    iPtr->freeProc = TCL_DYNAMIC;
	} else {
	    iPtr->result = iPtr->resultSpace;
	    iPtr->freeProc = 0;
	}
	strcpy(iPtr->result, string);
    } else {
	iPtr->result = string;
	iPtr->freeProc = freeProc;
    }

    /*
     * If the old result was dynamically-allocated, free it up.  Do it
     * here, rather than at the beginning, in case the new result value
     * was part of the old result value.
     */

    if (oldFreeProc != 0) {
	if ((oldFreeProc == TCL_DYNAMIC)
		|| (oldFreeProc == (Tcl_FreeProc *) free)) {
	    ckfree(oldResult);
	} else {
	    (*oldFreeProc)(oldResult);
	}
    }

    /*
     * Reset the object result since we just set the string result.
     */

    ResetObjResult(iPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetStringResult --
 *
 *	Returns an interpreter's result value as a string.
 *
 * Results:
 *	The interpreter's result as a string.
 *
 * Side effects:
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_GetStringResult(interp)
     register Tcl_Interp *interp; /* Interpreter whose result to return. */
{
    /*
     * If the string result is empty, move the object result to the
     * string result, then reset the object result.
     */
    
    if (*(interp->result) == 0) {
	Tcl_SetResult(interp, TclGetString(Tcl_GetObjResult(interp)),
	        TCL_VOLATILE);
    }
    return interp->result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjResult --
 *
 *	Arrange for objPtr to be an interpreter's result value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	interp->objResultPtr is left pointing to the object referenced
 *	by objPtr. The object's reference count is incremented since
 *	there is now a new reference to it. The reference count for any
 *	old objResultPtr value is decremented. Also, the string result
 *	is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetObjResult(interp, objPtr)
    Tcl_Interp *interp;		/* Interpreter with which to associate the
				 * return object value. */
    register Tcl_Obj *objPtr;	/* Tcl object to be returned. If NULL, the
				 * obj result is made an empty string
				 * object. */
{
    register Interp *iPtr = (Interp *) interp;
    register Tcl_Obj *oldObjResult = iPtr->objResultPtr;

    iPtr->objResultPtr = objPtr;
    Tcl_IncrRefCount(objPtr);	/* since interp result is a reference */

    /*
     * We wait until the end to release the old object result, in case
     * we are setting the result to itself.
     */
    
    TclDecrRefCount(oldObjResult);

    /*
     * Reset the string result since we just set the result object.
     */

    if (iPtr->freeProc != NULL) {
	if ((iPtr->freeProc == TCL_DYNAMIC)
	        || (iPtr->freeProc == (Tcl_FreeProc *) free)) {
	    ckfree(iPtr->result);
	} else {
	    (*iPtr->freeProc)(iPtr->result);
	}
	iPtr->freeProc = 0;
    }
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetObjResult --
 *
 *	Returns an interpreter's result value as a Tcl object. The object's
 *	reference count is not modified; the caller must do that if it
 *	needs to hold on to a long-term reference to it.
 *
 * Results:
 *	The interpreter's result as an object.
 *
 * Side effects:
 *	If the interpreter has a non-empty string result, the result object
 *	is either empty or stale because some procedure set interp->result
 *	directly. If so, the string result is moved to the result object
 *	then the string result is reset.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetObjResult(interp)
    Tcl_Interp *interp;		/* Interpreter whose result to return. */
{
    register Interp *iPtr = (Interp *) interp;
    Tcl_Obj *objResultPtr;
    int length;

    /*
     * If the string result is non-empty, move the string result to the
     * object result, then reset the string result.
     */
    
    if (*(iPtr->result) != 0) {
	ResetObjResult(iPtr);
	
	objResultPtr = iPtr->objResultPtr;
	length = strlen(iPtr->result);
	TclInitStringRep(objResultPtr, iPtr->result, length);
	
	if (iPtr->freeProc != NULL) {
	    if ((iPtr->freeProc == TCL_DYNAMIC)
	            || (iPtr->freeProc == (Tcl_FreeProc *) free)) {
		ckfree(iPtr->result);
	    } else {
		(*iPtr->freeProc)(iPtr->result);
	    }
	    iPtr->freeProc = 0;
	}
	iPtr->result = iPtr->resultSpace;
	iPtr->resultSpace[0] = 0;
    }
    return iPtr->objResultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendResultVA --
 *
 *	Append a variable number of strings onto the interpreter's string
 *	result.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The result of the interpreter given by the first argument is
 *	extended by the strings in the va_list (up to a terminating NULL
 *	argument).
 *
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendResultVA (interp, argList)
    Tcl_Interp *interp;		/* Interpreter with which to associate the
				 * return value. */
    va_list argList;		/* Variable argument list. */
{
#define STATIC_LIST_SIZE 16
    Interp *iPtr = (Interp *) interp;
    char *string, *static_list[STATIC_LIST_SIZE];
    char **args = static_list;
    int nargs_space = STATIC_LIST_SIZE;
    int nargs, newSpace, i;

    /*
     * If the string result is empty, move the object result to the
     * string result, then reset the object result.
     */

    if (*(iPtr->result) == 0) {
	Tcl_SetResult((Tcl_Interp *) iPtr,
	        TclGetString(Tcl_GetObjResult((Tcl_Interp *) iPtr)),
	        TCL_VOLATILE);
    }
    
    /*
     * Scan through all the arguments to see how much space is needed
     * and save pointers to the arguments in the args array,
     * reallocating as necessary.
     */

    nargs = 0;
    newSpace = 0;
    while (1) {
 	string = va_arg(argList, char *);
	if (string == NULL) {
	    break;
	}
 	if (nargs >= nargs_space) {
 	    /* 
 	     * Expand the args buffer
 	     */
 	    nargs_space += STATIC_LIST_SIZE;
 	    if (args == static_list) {
 	    	args = (void *)ckalloc(nargs_space * sizeof(char *));
 		for (i = 0; i < nargs; ++i) {
 		    args[i] = static_list[i];
 		}
 	    } else {
 		args = (void *)ckrealloc((void *)args,
			nargs_space * sizeof(char *));
 	    }
 	}
  	newSpace += strlen(string);
	args[nargs++] = string;
    }

    /*
     * If the append buffer isn't already setup and large enough to hold
     * the new data, set it up.
     */

    if ((iPtr->result != iPtr->appendResult)
	    || (iPtr->appendResult[iPtr->appendUsed] != 0)
	    || ((newSpace + iPtr->appendUsed) >= iPtr->appendAvl)) {
       SetupAppendBuffer(iPtr, newSpace);
    }

    /*
     * Now go through all the argument strings again, copying them into the
     * buffer.
     */

    for (i = 0; i < nargs; ++i) {
 	string = args[i];
  	strcpy(iPtr->appendResult + iPtr->appendUsed, string);
  	iPtr->appendUsed += strlen(string);
    }
 
    /*
     * If we had to allocate a buffer from the heap, 
     * free it now.
     */
 
    if (args != static_list) {
     	ckfree((void *)args);
    }
#undef STATIC_LIST_SIZE
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendResult --
 *
 *	Append a variable number of strings onto the interpreter's string
 *	result.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The result of the interpreter given by the first argument is
 *	extended by the strings given by the second and following arguments
 *	(up to a terminating NULL argument).
 *
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendResult TCL_VARARGS_DEF(Tcl_Interp *,arg1)
{
    Tcl_Interp *interp;
    va_list argList;

    interp = TCL_VARARGS_START(Tcl_Interp *,arg1,argList);
    Tcl_AppendResultVA(interp, argList);
    va_end(argList);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendElement --
 *
 *	Convert a string to a valid Tcl list element and append it to the
 *	result (which is ostensibly a list).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The result in the interpreter given by the first argument is
 *	extended with a list element converted from string. A separator
 *	space is added before the converted list element unless the current
 *	result is empty, contains the single character "{", or ends in " {".
 *
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendElement(interp, string)
    Tcl_Interp *interp;		/* Interpreter whose result is to be
				 * extended. */
    CONST char *string;		/* String to convert to list element and
				 * add to result. */
{
    Interp *iPtr = (Interp *) interp;
    char *dst;
    int size;
    int flags;

    /*
     * If the string result is empty, move the object result to the
     * string result, then reset the object result.
     */

    if (*(iPtr->result) == 0) {
	Tcl_SetResult(interp, TclGetString(Tcl_GetObjResult(interp)),
	        TCL_VOLATILE);
    }

    /*
     * See how much space is needed, and grow the append buffer if
     * needed to accommodate the list element.
     */

    size = Tcl_ScanElement(string, &flags) + 1;
    if ((iPtr->result != iPtr->appendResult)
	    || (iPtr->appendResult[iPtr->appendUsed] != 0)
	    || ((size + iPtr->appendUsed) >= iPtr->appendAvl)) {
       SetupAppendBuffer(iPtr, size+iPtr->appendUsed);
    }

    /*
     * Convert the string into a list element and copy it to the
     * buffer that's forming, with a space separator if needed.
     */

    dst = iPtr->appendResult + iPtr->appendUsed;
    if (TclNeedSpace(iPtr->appendResult, dst)) {
	iPtr->appendUsed++;
	*dst = ' ';
	dst++;
    }
    iPtr->appendUsed += Tcl_ConvertElement(string, dst, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * SetupAppendBuffer --
 *
 *	This procedure makes sure that there is an append buffer properly
 *	initialized, if necessary, from the interpreter's result, and
 *	that it has at least enough room to accommodate newSpace new
 *	bytes of information.
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
SetupAppendBuffer(iPtr, newSpace)
    Interp *iPtr;		/* Interpreter whose result is being set up. */
    int newSpace;		/* Make sure that at least this many bytes
				 * of new information may be added. */
{
    int totalSpace;

    /*
     * Make the append buffer larger, if that's necessary, then copy the
     * result into the append buffer and make the append buffer the official
     * Tcl result.
     */

    if (iPtr->result != iPtr->appendResult) {
	/*
	 * If an oversized buffer was used recently, then free it up
	 * so we go back to a smaller buffer.  This avoids tying up
	 * memory forever after a large operation.
	 */

	if (iPtr->appendAvl > 500) {
	    ckfree(iPtr->appendResult);
	    iPtr->appendResult = NULL;
	    iPtr->appendAvl = 0;
	}
	iPtr->appendUsed = strlen(iPtr->result);
    } else if (iPtr->result[iPtr->appendUsed] != 0) {
	/*
	 * Most likely someone has modified a result created by
	 * Tcl_AppendResult et al. so that it has a different size.
	 * Just recompute the size.
	 */

	iPtr->appendUsed = strlen(iPtr->result);
    }
    
    totalSpace = newSpace + iPtr->appendUsed;
    if (totalSpace >= iPtr->appendAvl) {
	char *new;

	if (totalSpace < 100) {
	    totalSpace = 200;
	} else {
	    totalSpace *= 2;
	}
	new = (char *) ckalloc((unsigned) totalSpace);
	strcpy(new, iPtr->result);
	if (iPtr->appendResult != NULL) {
	    ckfree(iPtr->appendResult);
	}
	iPtr->appendResult = new;
	iPtr->appendAvl = totalSpace;
    } else if (iPtr->result != iPtr->appendResult) {
	strcpy(iPtr->appendResult, iPtr->result);
    }
    
    Tcl_FreeResult((Tcl_Interp *) iPtr);
    iPtr->result = iPtr->appendResult;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FreeResult --
 *
 *	This procedure frees up the memory associated with an interpreter's
 *	string result. It also resets the interpreter's result object.
 *	Tcl_FreeResult is most commonly used when a procedure is about to
 *	replace one result value with another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the memory associated with interp's string result and sets
 *	interp->freeProc to zero, but does not change interp->result or
 *	clear error state. Resets interp's result object to an unshared
 *	empty object.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FreeResult(interp)
    register Tcl_Interp *interp; /* Interpreter for which to free result. */
{
    register Interp *iPtr = (Interp *) interp;
    
    if (iPtr->freeProc != NULL) {
	if ((iPtr->freeProc == TCL_DYNAMIC)
	        || (iPtr->freeProc == (Tcl_FreeProc *) free)) {
	    ckfree(iPtr->result);
	} else {
	    (*iPtr->freeProc)(iPtr->result);
	}
	iPtr->freeProc = 0;
    }
    
    ResetObjResult(iPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ResetResult --
 *
 *	This procedure resets both the interpreter's string and object
 *	results.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	It resets the result object to an unshared empty object. It
 *	then restores the interpreter's string result area to its default
 *	initialized state, freeing up any memory that may have been
 *	allocated. It also clears any error information for the interpreter.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ResetResult(interp)
    register Tcl_Interp *interp; /* Interpreter for which to clear result. */
{
    register Interp *iPtr = (Interp *) interp;

    ResetObjResult(iPtr);
    if (iPtr->freeProc != NULL) {
	if ((iPtr->freeProc == TCL_DYNAMIC)
	        || (iPtr->freeProc == (Tcl_FreeProc *) free)) {
	    ckfree(iPtr->result);
	} else {
	    (*iPtr->freeProc)(iPtr->result);
	}
	iPtr->freeProc = 0;
    }
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
    iPtr->flags &= ~(ERR_ALREADY_LOGGED | ERR_IN_PROGRESS | ERROR_CODE_SET);
}

/*
 *----------------------------------------------------------------------
 *
 * ResetObjResult --
 *
 *	Procedure used to reset an interpreter's Tcl result object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the interpreter's result object to an unshared empty string
 *	object with ref count one. It does not clear any error information
 *	in the interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
ResetObjResult(iPtr)
    register Interp *iPtr;	/* Points to the interpreter whose result
				 * object should be reset. */
{
    register Tcl_Obj *objResultPtr = iPtr->objResultPtr;

    if (Tcl_IsShared(objResultPtr)) {
	TclDecrRefCount(objResultPtr);
	TclNewObj(objResultPtr);
	Tcl_IncrRefCount(objResultPtr);
	iPtr->objResultPtr = objResultPtr;
    } else {
	if ((objResultPtr->bytes != NULL)
	        && (objResultPtr->bytes != tclEmptyStringRep)) {
	    ckfree((char *) objResultPtr->bytes);
	}
	objResultPtr->bytes  = tclEmptyStringRep;
	objResultPtr->length = 0;
	if ((objResultPtr->typePtr != NULL)
	        && (objResultPtr->typePtr->freeIntRepProc != NULL)) {
	    objResultPtr->typePtr->freeIntRepProc(objResultPtr);
	}
	objResultPtr->typePtr = (Tcl_ObjType *) NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetErrorCodeVA --
 *
 *	This procedure is called to record machine-readable information
 *	about an error that is about to be returned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The errorCode global variable is modified to hold all of the
 *	arguments to this procedure, in a list form with each argument
 *	becoming one element of the list.  A flag is set internally
 *	to remember that errorCode has been set, so the variable doesn't
 *	get set automatically when the error is returned.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetErrorCodeVA (interp, argList)
    Tcl_Interp *interp;		/* Interpreter in which to access the errorCode
				 * variable. */
    va_list argList;		/* Variable argument list. */
{
    char *string;
    int flags;
    Interp *iPtr = (Interp *) interp;

    /*
     * Scan through the arguments one at a time, appending them to
     * $errorCode as list elements.
     */

    flags = TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT;
    while (1) {
	string = va_arg(argList, char *);
	if (string == NULL) {
	    break;
	}
	(void) Tcl_SetVar2((Tcl_Interp *) iPtr, "errorCode",
		(char *) NULL, string, flags);
	flags |= TCL_APPEND_VALUE;
    }
    iPtr->flags |= ERROR_CODE_SET;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetErrorCode --
 *
 *	This procedure is called to record machine-readable information
 *	about an error that is about to be returned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The errorCode global variable is modified to hold all of the
 *	arguments to this procedure, in a list form with each argument
 *	becoming one element of the list.  A flag is set internally
 *	to remember that errorCode has been set, so the variable doesn't
 *	get set automatically when the error is returned.
 *
 *----------------------------------------------------------------------
 */
	/* VARARGS2 */
void
Tcl_SetErrorCode TCL_VARARGS_DEF(Tcl_Interp *,arg1)
{
    Tcl_Interp *interp;
    va_list argList;

    /*
     * Scan through the arguments one at a time, appending them to
     * $errorCode as list elements.
     */

    interp = TCL_VARARGS_START(Tcl_Interp *,arg1,argList);
    Tcl_SetErrorCodeVA(interp, argList);
    va_end(argList);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjErrorCode --
 *
 *	This procedure is called to record machine-readable information
 *	about an error that is about to be returned. The caller should
 *	build a list object up and pass it to this routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The errorCode global variable is modified to be the new value.
 *	A flag is set internally to remember that errorCode has been
 *	set, so the variable doesn't get set automatically when the
 *	error is returned.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetObjErrorCode(interp, errorObjPtr)
    Tcl_Interp *interp;
    Tcl_Obj *errorObjPtr;
{
    Interp *iPtr;
    
    iPtr = (Interp *) interp;
    Tcl_SetVar2Ex(interp, "errorCode", NULL, errorObjPtr, TCL_GLOBAL_ONLY);
    iPtr->flags |= ERROR_CODE_SET;
}

/*
 *-------------------------------------------------------------------------
 *
 * TclTransferResult --
 *
 *	Copy the result (and error information) from one interp to 
 *	another.  Used when one interp has caused another interp to 
 *	evaluate a script and then wants to transfer the results back
 *	to itself.
 *
 *	This routine copies the string reps of the result and error 
 *	information.  It does not simply increment the refcounts of the
 *	result and error information objects themselves.
 *	It is not legal to exchange objects between interps, because an
 *	object may be kept alive by one interp, but have an internal rep 
 *	that is only valid while some other interp is alive.  
 *
 * Results:
 *	The target interp's result is set to a copy of the source interp's
 *	result.  The source's error information "$errorInfo" may be
 *	appended to the target's error information and the source's error
 *	code "$errorCode" may be stored in the target's error code.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
	
void
TclTransferResult(sourceInterp, result, targetInterp)
    Tcl_Interp *sourceInterp;	/* Interp whose result and error information
				 * should be moved to the target interp.  
				 * After moving result, this interp's result 
				 * is reset. */
    int result;			/* TCL_OK if just the result should be copied, 
				 * TCL_ERROR if both the result and error 
				 * information should be copied. */
    Tcl_Interp *targetInterp;	/* Interp where result and error information 
				 * should be stored.  If source and target
				 * are the same, nothing is done. */
{
    Interp *iPtr;
    Tcl_Obj *objPtr;

    if (sourceInterp == targetInterp) {
	return;
    }

    if (result == TCL_ERROR) {
	/*
	 * An error occurred, so transfer error information from the source
	 * interpreter to the target interpreter.  Setting the flags tells
	 * the target interp that it has inherited a partial traceback
	 * chain, not just a simple error message.
	 */

	iPtr = (Interp *) sourceInterp;
        if ((iPtr->flags & ERR_ALREADY_LOGGED) == 0) {
            Tcl_AddErrorInfo(sourceInterp, "");
        }
        iPtr->flags &= ~(ERR_ALREADY_LOGGED);
        
        Tcl_ResetResult(targetInterp);
        
	objPtr = Tcl_GetVar2Ex(sourceInterp, "errorInfo", NULL,
		TCL_GLOBAL_ONLY);
	Tcl_SetVar2Ex(targetInterp, "errorInfo", NULL, objPtr,
		TCL_GLOBAL_ONLY);

	objPtr = Tcl_GetVar2Ex(sourceInterp, "errorCode", NULL,
		TCL_GLOBAL_ONLY);
	Tcl_SetVar2Ex(targetInterp, "errorCode", NULL, objPtr,
		TCL_GLOBAL_ONLY);

	((Interp *) targetInterp)->flags |= (ERR_IN_PROGRESS | ERROR_CODE_SET);
    }

    ((Interp *) targetInterp)->returnCode = ((Interp *) sourceInterp)->returnCode;
    Tcl_SetObjResult(targetInterp, Tcl_GetObjResult(sourceInterp));
    Tcl_ResetResult(sourceInterp);
}
