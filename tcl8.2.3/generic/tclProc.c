/* 
 * tclProc.c --
 *
 *	This file contains routines that implement Tcl procedures,
 *	including the "proc" and "uplevel" commands.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclCompile.h"

/*
 * Prototypes for static functions in this file
 */

static void	ProcBodyDup _ANSI_ARGS_((Tcl_Obj *srcPtr, Tcl_Obj *dupPtr));
static void	ProcBodyFree _ANSI_ARGS_((Tcl_Obj *objPtr));
static int	ProcBodySetFromAny _ANSI_ARGS_((Tcl_Interp *interp,
		Tcl_Obj *objPtr));
static void	ProcBodyUpdateString _ANSI_ARGS_((Tcl_Obj *objPtr));
static  int	ProcessProcResultCode _ANSI_ARGS_((Tcl_Interp *interp,
		    char *procName, int nameLen, int returnCode));

/*
 * The ProcBodyObjType type
 */

Tcl_ObjType tclProcBodyType = {
    "procbody",			/* name for this type */
    ProcBodyFree,		/* FreeInternalRep procedure */
    ProcBodyDup,		/* DupInternalRep procedure */
    ProcBodyUpdateString,	/* UpdateString procedure */
    ProcBodySetFromAny		/* SetFromAny procedure */
};

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ProcObjCmd --
 *
 *	This object-based procedure is invoked to process the "proc" Tcl 
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	A new procedure gets created.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ProcObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register Interp *iPtr = (Interp *) interp;
    Proc *procPtr;
    char *fullName, *procName;
    Namespace *nsPtr, *altNsPtr, *cxtNsPtr;
    Tcl_Command cmd;
    Tcl_DString ds;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "name args body");
	return TCL_ERROR;
    }

    /*
     * Determine the namespace where the procedure should reside. Unless
     * the command name includes namespace qualifiers, this will be the
     * current namespace.
     */
    
    fullName = TclGetString(objv[1]);
    TclGetNamespaceForQualName(interp, fullName, (Namespace *) NULL,
	    0, &nsPtr, &altNsPtr, &cxtNsPtr, &procName);

    if (nsPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"can't create procedure \"", fullName,
		"\": unknown namespace", (char *) NULL);
        return TCL_ERROR;
    }
    if (procName == NULL) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"can't create procedure \"", fullName,
		"\": bad procedure name", (char *) NULL);
        return TCL_ERROR;
    }
    if ((nsPtr != iPtr->globalNsPtr)
	    && (procName != NULL) && (procName[0] == ':')) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"can't create procedure \"", procName,
		"\" in non-global namespace with name starting with \":\"",
	        (char *) NULL);
        return TCL_ERROR;
    }

    /*
     *  Create the data structure to represent the procedure.
     */
    if (TclCreateProc(interp, nsPtr, procName, objv[2], objv[3],
        &procPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Now create a command for the procedure. This will initially be in
     * the current namespace unless the procedure's name included namespace
     * qualifiers. To create the new command in the right namespace, we
     * generate a fully qualified name for it.
     */

    Tcl_DStringInit(&ds);
    if (nsPtr != iPtr->globalNsPtr) {
	Tcl_DStringAppend(&ds, nsPtr->fullName, -1);
	Tcl_DStringAppend(&ds, "::", 2);
    }
    Tcl_DStringAppend(&ds, procName, -1);
    
    Tcl_CreateCommand(interp, Tcl_DStringValue(&ds), TclProcInterpProc,
	    (ClientData) procPtr, TclProcDeleteProc);
    cmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(&ds),
	    TclObjInterpProc, (ClientData) procPtr, TclProcDeleteProc);

    Tcl_DStringFree(&ds);
    /*
     * Now initialize the new procedure's cmdPtr field. This will be used
     * later when the procedure is called to determine what namespace the
     * procedure will run in. This will be different than the current
     * namespace if the proc was renamed into a different namespace.
     */
    
    procPtr->cmdPtr = (Command *) cmd;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateProc --
 *
 *	Creates the data associated with a Tcl procedure definition.
 *	This procedure knows how to handle two types of body objects:
 *	strings and procbody. Strings are the traditional (and common) value
 *	for bodies, procbody are values created by extensions that have
 *	loaded a previously compiled script.
 *
 * Results:
 *	Returns TCL_OK on success, along with a pointer to a Tcl
 *	procedure definition in procPtrPtr.  This definition should
 *	be freed by calling TclCleanupProc() when it is no longer
 *	needed.  Returns TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	If anything goes wrong, this procedure returns an error
 *	message in the interpreter.
 *
 *----------------------------------------------------------------------
 */
int
TclCreateProc(interp, nsPtr, procName, argsPtr, bodyPtr, procPtrPtr)
    Tcl_Interp *interp;         /* interpreter containing proc */
    Namespace *nsPtr;           /* namespace containing this proc */
    char *procName;             /* unqualified name of this proc */
    Tcl_Obj *argsPtr;           /* description of arguments */
    Tcl_Obj *bodyPtr;           /* command body */
    Proc **procPtrPtr;          /* returns:  pointer to proc data */
{
    Interp *iPtr = (Interp*)interp;
    char **argArray = NULL;

    register Proc *procPtr;
    int i, length, result, numArgs;
    char *args, *bytes, *p;
    register CompiledLocal *localPtr = NULL;
    Tcl_Obj *defPtr;
    int precompiled = 0;
    
    if (bodyPtr->typePtr == &tclProcBodyType) {
        /*
         * Because the body is a TclProProcBody, the actual body is already
         * compiled, and it is not shared with anyone else, so it's OK not to
         * unshare it (as a matter of fact, it is bad to unshare it, because
         * there may be no source code).
         *
         * We don't create and initialize a Proc structure for the procedure;
         * rather, we use what is in the body object. Note that
         * we initialize its cmdPtr field below after we've created the command
         * for the procedure. We increment the ref count of the Proc struct
         * since the command (soon to be created) will be holding a reference
         * to it.
         */
    
        procPtr = (Proc *) bodyPtr->internalRep.otherValuePtr;
        procPtr->iPtr = iPtr;
        procPtr->refCount++;
        precompiled = 1;
    } else {
        /*
         * If the procedure's body object is shared because its string value is
         * identical to, e.g., the body of another procedure, we must create a
         * private copy for this procedure to use. Such sharing of procedure
         * bodies is rare but can cause problems. A procedure body is compiled
         * in a context that includes the number of compiler-allocated "slots"
         * for local variables. Each formal parameter is given a local variable
         * slot (the "procPtr->numCompiledLocals = numArgs" assignment
         * below). This means that the same code can not be shared by two
         * procedures that have a different number of arguments, even if their
         * bodies are identical. Note that we don't use Tcl_DuplicateObj since
         * we would not want any bytecode internal representation.
         */

        if (Tcl_IsShared(bodyPtr)) {
            bytes = Tcl_GetStringFromObj(bodyPtr, &length);
            bodyPtr = Tcl_NewStringObj(bytes, length);
        }

        /*
         * Create and initialize a Proc structure for the procedure. Note that
         * we initialize its cmdPtr field below after we've created the command
         * for the procedure. We increment the ref count of the procedure's
         * body object since there will be a reference to it in the Proc
         * structure.
         */
    
        Tcl_IncrRefCount(bodyPtr);

        procPtr = (Proc *) ckalloc(sizeof(Proc));
        procPtr->iPtr = iPtr;
        procPtr->refCount = 1;
        procPtr->bodyPtr = bodyPtr;
        procPtr->numArgs  = 0;	/* actual argument count is set below. */
        procPtr->numCompiledLocals = 0;
        procPtr->firstLocalPtr = NULL;
        procPtr->lastLocalPtr = NULL;
    }
    
    /*
     * Break up the argument list into argument specifiers, then process
     * each argument specifier.
     * If the body is precompiled, processing is limited to checking that
     * the the parsed argument is consistent with the one stored in the
     * Proc.
     * THIS FAILS IF THE ARG LIST OBJECT'S STRING REP CONTAINS NULLS.
     */

    args = Tcl_GetStringFromObj(argsPtr, &length);
    result = Tcl_SplitList(interp, args, &numArgs, &argArray);
    if (result != TCL_OK) {
        goto procError;
    }

    if (precompiled) {
        if (numArgs > procPtr->numArgs) {
            char buf[64 + TCL_INTEGER_SPACE + TCL_INTEGER_SPACE];
            sprintf(buf, "\": arg list contains %d entries, precompiled header expects %d",
                    numArgs, procPtr->numArgs);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "procedure \"", procName,
                    buf, (char *) NULL);
            goto procError;
        }
        localPtr = procPtr->firstLocalPtr;
    } else {
        procPtr->numArgs = numArgs;
        procPtr->numCompiledLocals = numArgs;
    }
    for (i = 0;  i < numArgs;  i++) {
        int fieldCount, nameLength, valueLength;
        char **fieldValues;

        /*
         * Now divide the specifier up into name and default.
         */

        result = Tcl_SplitList(interp, argArray[i], &fieldCount,
                &fieldValues);
        if (result != TCL_OK) {
            goto procError;
        }
        if (fieldCount > 2) {
            ckfree((char *) fieldValues);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "too many fields in argument specifier \"",
                    argArray[i], "\"", (char *) NULL);
            goto procError;
        }
        if ((fieldCount == 0) || (*fieldValues[0] == 0)) {
            ckfree((char *) fieldValues);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "procedure \"", procName,
                    "\" has argument with no name", (char *) NULL);
            goto procError;
        }
	
        nameLength = strlen(fieldValues[0]);
        if (fieldCount == 2) {
            valueLength = strlen(fieldValues[1]);
        } else {
            valueLength = 0;
        }

        /*
         * Check that the formal parameter name is a scalar.
         */

        p = fieldValues[0];
        while (*p != '\0') {
            if (*p == '(') {
                char *q = p;
                do {
		    q++;
		} while (*q != '\0');
		q--;
		if (*q == ')') { /* we have an array element */
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		            "procedure \"", procName,
		            "\" has formal parameter \"", fieldValues[0],
			    "\" that is an array element",
			    (char *) NULL);
		    ckfree((char *) fieldValues);
		    goto procError;
		}
	    }
	    p++;
	}

        if (precompiled) {
            /*
             * compare the parsed argument with the stored one
             */

            if ((localPtr->nameLength != nameLength)
                    || (strcmp(localPtr->name, fieldValues[0]))
                    || (localPtr->frameIndex != i)
                    || (localPtr->flags != (VAR_SCALAR | VAR_ARGUMENT))
                    || ((localPtr->defValuePtr == NULL)
                            && (fieldCount == 2))
                    || ((localPtr->defValuePtr != NULL)
                            && (fieldCount != 2))) {
                char buf[80 + TCL_INTEGER_SPACE];
                sprintf(buf, "\": formal parameter %d is inconsistent with precompiled body",
                        i);
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                        "procedure \"", procName,
                        buf, (char *) NULL);
                ckfree((char *) fieldValues);
                goto procError;
            }

            /*
             * compare the default value if any
             */

            if (localPtr->defValuePtr != NULL) {
                int tmpLength;
                char *tmpPtr = Tcl_GetStringFromObj(localPtr->defValuePtr,
                        &tmpLength);
                if ((valueLength != tmpLength)
                        || (strncmp(fieldValues[1], tmpPtr,
                                (size_t) tmpLength))) {
                    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                            "procedure \"", procName,
                            "\": formal parameter \"",
                            fieldValues[0],
                            "\" has default value inconsistent with precompiled body",
                            (char *) NULL);
                    ckfree((char *) fieldValues);
                    goto procError;
                }
            }

            localPtr = localPtr->nextPtr;
        } else {
            /*
             * Allocate an entry in the runtime procedure frame's array of
             * local variables for the argument. 
             */

            localPtr = (CompiledLocal *) ckalloc((unsigned) 
                    (sizeof(CompiledLocal) - sizeof(localPtr->name)
                            + nameLength+1));
            if (procPtr->firstLocalPtr == NULL) {
                procPtr->firstLocalPtr = procPtr->lastLocalPtr = localPtr;
            } else {
                procPtr->lastLocalPtr->nextPtr = localPtr;
                procPtr->lastLocalPtr = localPtr;
            }
            localPtr->nextPtr = NULL;
            localPtr->nameLength = nameLength;
            localPtr->frameIndex = i;
            localPtr->flags = VAR_SCALAR | VAR_ARGUMENT;
            localPtr->resolveInfo = NULL;
	
            if (fieldCount == 2) {
                localPtr->defValuePtr =
		    Tcl_NewStringObj(fieldValues[1], valueLength);
                Tcl_IncrRefCount(localPtr->defValuePtr);
            } else {
                localPtr->defValuePtr = NULL;
            }
            strcpy(localPtr->name, fieldValues[0]);
	}

        ckfree((char *) fieldValues);
    }

    /*
     * Now initialize the new procedure's cmdPtr field. This will be used
     * later when the procedure is called to determine what namespace the
     * procedure will run in. This will be different than the current
     * namespace if the proc was renamed into a different namespace.
     */
    
    *procPtrPtr = procPtr;
    ckfree((char *) argArray);
    return TCL_OK;

procError:
    if (precompiled) {
        procPtr->refCount--;
    } else {
        Tcl_DecrRefCount(bodyPtr);
        while (procPtr->firstLocalPtr != NULL) {
            localPtr = procPtr->firstLocalPtr;
            procPtr->firstLocalPtr = localPtr->nextPtr;
	
            defPtr = localPtr->defValuePtr;
            if (defPtr != NULL) {
                Tcl_DecrRefCount(defPtr);
            }
	
            ckfree((char *) localPtr);
        }
        ckfree((char *) procPtr);
    }
    if (argArray != NULL) {
	ckfree((char *) argArray);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetFrame --
 *
 *	Given a description of a procedure frame, such as the first
 *	argument to an "uplevel" or "upvar" command, locate the
 *	call frame for the appropriate level of procedure.
 *
 * Results:
 *	The return value is -1 if an error occurred in finding the frame
 *	(in this case an error message is left in the interp's result).
 *	1 is returned if string was either a number or a number preceded
 *	by "#" and it specified a valid frame.  0 is returned if string
 *	isn't one of the two things above (in this case, the lookup
 *	acts as if string were "1").  The variable pointed to by
 *	framePtrPtr is filled in with the address of the desired frame
 *	(unless an error occurs, in which case it isn't modified).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGetFrame(interp, string, framePtrPtr)
    Tcl_Interp *interp;		/* Interpreter in which to find frame. */
    char *string;		/* String describing frame. */
    CallFrame **framePtrPtr;	/* Store pointer to frame here (or NULL
				 * if global frame indicated). */
{
    register Interp *iPtr = (Interp *) interp;
    int curLevel, level, result;
    CallFrame *framePtr;

    /*
     * Parse string to figure out which level number to go to.
     */

    result = 1;
    curLevel = (iPtr->varFramePtr == NULL) ? 0 : iPtr->varFramePtr->level;
    if (*string == '#') {
	if (Tcl_GetInt(interp, string+1, &level) != TCL_OK) {
	    return -1;
	}
	if (level < 0) {
	    levelError:
	    Tcl_AppendResult(interp, "bad level \"", string, "\"",
		    (char *) NULL);
	    return -1;
	}
    } else if (isdigit(UCHAR(*string))) { /* INTL: digit */
	if (Tcl_GetInt(interp, string, &level) != TCL_OK) {
	    return -1;
	}
	level = curLevel - level;
    } else {
	level = curLevel - 1;
	result = 0;
    }

    /*
     * Figure out which frame to use, and modify the interpreter so
     * its variables come from that frame.
     */

    if (level == 0) {
	framePtr = NULL;
    } else {
	for (framePtr = iPtr->varFramePtr; framePtr != NULL;
		framePtr = framePtr->callerVarPtr) {
	    if (framePtr->level == level) {
		break;
	    }
	}
	if (framePtr == NULL) {
	    goto levelError;
	}
    }
    *framePtrPtr = framePtr;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UplevelObjCmd --
 *
 *	This object procedure is invoked to process the "uplevel" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_UplevelObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register Interp *iPtr = (Interp *) interp;
    char *optLevel;
    int result;
    CallFrame *savedVarFramePtr, *framePtr;

    if (objc < 2) {
	uplevelSyntax:
	Tcl_WrongNumArgs(interp, 1, objv, "?level? command ?arg ...?");
	return TCL_ERROR;
    }

    /*
     * Find the level to use for executing the command.
     */

    optLevel = TclGetString(objv[1]);
    result = TclGetFrame(interp, optLevel, &framePtr);
    if (result == -1) {
	return TCL_ERROR;
    }
    objc -= (result+1);
    if (objc == 0) {
	goto uplevelSyntax;
    }
    objv += (result+1);

    /*
     * Modify the interpreter state to execute in the given frame.
     */

    savedVarFramePtr = iPtr->varFramePtr;
    iPtr->varFramePtr = framePtr;

    /*
     * Execute the residual arguments as a command.
     */

    if (objc == 1) {
	result = Tcl_EvalObjEx(interp, objv[0], 0);
    } else {
	Tcl_Obj *objPtr;

	objPtr = Tcl_ConcatObj(objc, objv);
	result = Tcl_EvalObjEx(interp, objPtr, TCL_EVAL_DIRECT);
    }
    if (result == TCL_ERROR) {
	char msg[32 + TCL_INTEGER_SPACE];
	sprintf(msg, "\n    (\"uplevel\" body line %d)", interp->errorLine);
	Tcl_AddObjErrorInfo(interp, msg, -1);
    }

    /*
     * Restore the variable frame, and return.
     */

    iPtr->varFramePtr = savedVarFramePtr;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFindProc --
 *
 *	Given the name of a procedure, return a pointer to the
 *	record describing the procedure. The procedure will be
 *	looked up using the usual rules: first in the current
 *	namespace and then in the global namespace.
 *
 * Results:
 *	NULL is returned if the name doesn't correspond to any
 *	procedure. Otherwise, the return value is a pointer to
 *	the procedure's record. If the name is found but refers
 *	to an imported command that points to a "real" procedure
 *	defined in another namespace, a pointer to that "real"
 *	procedure's structure is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Proc *
TclFindProc(iPtr, procName)
    Interp *iPtr;		/* Interpreter in which to look. */
    char *procName;		/* Name of desired procedure. */
{
    Tcl_Command cmd;
    Tcl_Command origCmd;
    Command *cmdPtr;
    
    cmd = Tcl_FindCommand((Tcl_Interp *) iPtr, procName,
            (Tcl_Namespace *) NULL, /*flags*/ 0);
    if (cmd == (Tcl_Command) NULL) {
        return NULL;
    }
    cmdPtr = (Command *) cmd;

    origCmd = TclGetOriginalCommand(cmd);
    if (origCmd != NULL) {
	cmdPtr = (Command *) origCmd;
    }
    if (cmdPtr->proc != TclProcInterpProc) {
	return NULL;
    }
    return (Proc *) cmdPtr->clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * TclIsProc --
 *
 *	Tells whether a command is a Tcl procedure or not.
 *
 * Results:
 *	If the given command is actually a Tcl procedure, the
 *	return value is the address of the record describing
 *	the procedure.  Otherwise the return value is 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Proc *
TclIsProc(cmdPtr)
    Command *cmdPtr;		/* Command to test. */
{
    Tcl_Command origCmd;

    origCmd = TclGetOriginalCommand((Tcl_Command) cmdPtr);
    if (origCmd != NULL) {
	cmdPtr = (Command *) origCmd;
    }
    if (cmdPtr->proc == TclProcInterpProc) {
	return (Proc *) cmdPtr->clientData;
    }
    return (Proc *) 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclProcInterpProc --
 *
 *	When a Tcl procedure gets invoked with an argc/argv array of
 *	strings, this routine gets invoked to interpret the procedure.
 *
 * Results:
 *	A standard Tcl result value, usually TCL_OK.
 *
 * Side effects:
 *	Depends on the commands in the procedure.
 *
 *----------------------------------------------------------------------
 */

int
TclProcInterpProc(clientData, interp, argc, argv)
    ClientData clientData;	/* Record describing procedure to be
				 * interpreted. */
    Tcl_Interp *interp;		/* Interpreter in which procedure was
				 * invoked. */
    int argc;			/* Count of number of arguments to this
				 * procedure. */
    register char **argv;	/* Argument values. */
{
    register Tcl_Obj *objPtr;
    register int i;
    int result;

    /*
     * This procedure generates an objv array for object arguments that hold
     * the argv strings. It starts out with stack-allocated space but uses
     * dynamically-allocated storage if needed.
     */

#define NUM_ARGS 20
    Tcl_Obj *(objStorage[NUM_ARGS]);
    register Tcl_Obj **objv = objStorage;

    /*
     * Create the object argument array "objv". Make sure objv is large
     * enough to hold the objc arguments plus 1 extra for the zero
     * end-of-objv word.
     */

    if ((argc + 1) > NUM_ARGS) {
	objv = (Tcl_Obj **)
	    ckalloc((unsigned)(argc + 1) * sizeof(Tcl_Obj *));
    }

    for (i = 0;  i < argc;  i++) {
	objv[i] = Tcl_NewStringObj(argv[i], -1);
	Tcl_IncrRefCount(objv[i]);
    }
    objv[argc] = 0;

    /*
     * Use TclObjInterpProc to actually interpret the procedure.
     */

    result = TclObjInterpProc(clientData, interp, argc, objv);

    /*
     * Move the interpreter's object result to the string result, 
     * then reset the object result.
     */
    
    Tcl_SetResult(interp, TclGetString(Tcl_GetObjResult(interp)),
	    TCL_VOLATILE);

    /*
     * Decrement the ref counts on the objv elements since we are done
     * with them.
     */

    for (i = 0;  i < argc;  i++) {
	objPtr = objv[i];
	TclDecrRefCount(objPtr);
    }
    
    /*
     * Free the objv array if malloc'ed storage was used.
     */

    if (objv != objStorage) {
	ckfree((char *) objv);
    }
    return result;
#undef NUM_ARGS
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjInterpProc --
 *
 *	When a Tcl procedure gets invoked during bytecode evaluation, this 
 *	object-based routine gets invoked to interpret the procedure.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	Depends on the commands in the procedure.
 *
 *----------------------------------------------------------------------
 */

int
TclObjInterpProc(clientData, interp, objc, objv)
    ClientData clientData; 	 /* Record describing procedure to be
				  * interpreted. */
    register Tcl_Interp *interp; /* Interpreter in which procedure was
				  * invoked. */
    int objc;			 /* Count of number of arguments to this
				  * procedure. */
    Tcl_Obj *CONST objv[];	 /* Argument value objects. */
{
    Interp *iPtr = (Interp *) interp;
    register Proc *procPtr = (Proc *) clientData;
    Namespace *nsPtr = procPtr->cmdPtr->nsPtr;
    CallFrame frame;
    register CallFrame *framePtr = &frame;
    register Var *varPtr;
    register CompiledLocal *localPtr;
    char *procName;
    int nameLen, localCt, numArgs, argCt, i, result;

    /*
     * This procedure generates an array "compiledLocals" that holds the
     * storage for local variables. It starts out with stack-allocated space
     * but uses dynamically-allocated storage if needed.
     */

#define NUM_LOCALS 20
    Var localStorage[NUM_LOCALS];
    Var *compiledLocals = localStorage;

    /*
     * Get the procedure's name.
     */
    
    procName = Tcl_GetStringFromObj(objv[0], &nameLen);

    /*
     * If necessary, compile the procedure's body. The compiler will
     * allocate frame slots for the procedure's non-argument local
     * variables.  Note that compiling the body might increase
     * procPtr->numCompiledLocals if new local variables are found
     * while compiling.
     */

    result = TclProcCompileProc(interp, procPtr, procPtr->bodyPtr, nsPtr,
	    "body of proc", procName);
    
    if (result != TCL_OK) {
        return result;
    }

    /*
     * Create the "compiledLocals" array. Make sure it is large enough to
     * hold all the procedure's compiled local variables, including its
     * formal parameters.
     */

    localCt = procPtr->numCompiledLocals;
    if (localCt > NUM_LOCALS) {
	compiledLocals = (Var *) ckalloc((unsigned) localCt * sizeof(Var));
    }
    
    /*
     * Set up and push a new call frame for the new procedure invocation.
     * This call frame will execute in the proc's namespace, which might
     * be different than the current namespace. The proc's namespace is
     * that of its command, which can change if the command is renamed
     * from one namespace to another.
     */

    result = Tcl_PushCallFrame(interp, (Tcl_CallFrame *) framePtr,
            (Tcl_Namespace *) nsPtr, /*isProcCallFrame*/ 1);

    if (result != TCL_OK) {
        return result;
    }

    framePtr->objc = objc;
    framePtr->objv = objv;  /* ref counts for args are incremented below */

    /*
     * Initialize and resolve compiled variable references.
     */

    framePtr->procPtr = procPtr;
    framePtr->numCompiledLocals = localCt;
    framePtr->compiledLocals = compiledLocals;

    TclInitCompiledLocals(interp, framePtr, nsPtr);

    /*
     * Match and assign the call's actual parameters to the procedure's
     * formal arguments. The formal arguments are described by the first
     * numArgs entries in both the Proc structure's local variable list and
     * the call frame's local variable array.
     */

    numArgs = procPtr->numArgs;
    varPtr = framePtr->compiledLocals;
    localPtr = procPtr->firstLocalPtr;
    argCt = objc;
    for (i = 1, argCt -= 1;  i <= numArgs;  i++, argCt--) {
	if (!TclIsVarArgument(localPtr)) {
	    panic("TclObjInterpProc: local variable %s is not argument but should be",
		  localPtr->name);
	    return TCL_ERROR;
	}
	if (TclIsVarTemporary(localPtr)) {
	    panic("TclObjInterpProc: local variable %d is temporary but should be an argument", i);
	    return TCL_ERROR;
	}

	/*
	 * Handle the special case of the last formal being "args".  When
	 * it occurs, assign it a list consisting of all the remaining
	 * actual arguments.
	 */

	if ((i == numArgs) && ((localPtr->name[0] == 'a')
	        && (strcmp(localPtr->name, "args") == 0))) {
	    Tcl_Obj *listPtr = Tcl_NewListObj(argCt, &(objv[i]));
	    varPtr->value.objPtr = listPtr;
	    Tcl_IncrRefCount(listPtr); /* local var is a reference */
	    varPtr->flags &= ~VAR_UNDEFINED;
	    argCt = 0;
	    break;		/* done processing args */
	} else if (argCt > 0) {
	    Tcl_Obj *objPtr = objv[i];
	    varPtr->value.objPtr = objPtr;
	    varPtr->flags &= ~VAR_UNDEFINED;
	    Tcl_IncrRefCount(objPtr);  /* since the local variable now has
					* another reference to object. */
	} else if (localPtr->defValuePtr != NULL) {
	    Tcl_Obj *objPtr = localPtr->defValuePtr;
	    varPtr->value.objPtr = objPtr;
	    varPtr->flags &= ~VAR_UNDEFINED;
	    Tcl_IncrRefCount(objPtr);  /* since the local variable now has
					* another reference to object. */
	} else {
	    Tcl_ResetResult(interp);
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "no value given for parameter \"", localPtr->name,
		    "\" to \"", Tcl_GetString(objv[0]), "\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto procDone;
	}
	varPtr++;
	localPtr = localPtr->nextPtr;
    }
    if (argCt > 0) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"called \"", Tcl_GetString(objv[0]),
		"\" with too many arguments", (char *) NULL);
	result = TCL_ERROR;
	goto procDone;
    }

    /*
     * Invoke the commands in the procedure's body.
     */

    if (tclTraceExec >= 1) {
#ifdef TCL_COMPILE_DEBUG
	fprintf(stdout, "Calling proc ");
	for (i = 0;  i < objc;  i++) {
	    TclPrintObject(stdout, objv[i], 15);
	    fprintf(stdout, " ");
	}
	fprintf(stdout, "\n");
#else /* TCL_COMPILE_DEBUG */
	fprintf(stdout, "Calling proc %.*s\n", nameLen, procName);
#endif /*TCL_COMPILE_DEBUG*/
	fflush(stdout);
    }

    iPtr->returnCode = TCL_OK;
    procPtr->refCount++;
    result = Tcl_EvalObjEx(interp, procPtr->bodyPtr, 0);
    procPtr->refCount--;
    if (procPtr->refCount <= 0) {
	TclProcCleanupProc(procPtr);
    }

    if (result != TCL_OK) {
	result = ProcessProcResultCode(interp, procName, nameLen, result);
    }
    
    /*
     * Pop and free the call frame for this procedure invocation, then
     * free the compiledLocals array if malloc'ed storage was used.
     */
    
    procDone:
    Tcl_PopCallFrame(interp);
    if (compiledLocals != localStorage) {
	ckfree((char *) compiledLocals);
    }
    return result;
#undef NUM_LOCALS
}

/*
 *----------------------------------------------------------------------
 *
 * TclProcCompileProc --
 *
 *	Called just before a procedure is executed to compile the
 *	body to byte codes.  If the type of the body is not
 *	"byte code" or if the compile conditions have changed
 *	(namespace context, epoch counters, etc.) then the body
 *	is recompiled.  Otherwise, this procedure does nothing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change the internal representation of the body object
 *	to compiled code.
 *
 *----------------------------------------------------------------------
 */
 
int
TclProcCompileProc(interp, procPtr, bodyPtr, nsPtr, description, procName)
    Tcl_Interp *interp;		/* Interpreter containing procedure. */
    Proc *procPtr;		/* Data associated with procedure. */
    Tcl_Obj *bodyPtr;		/* Body of proc. (Usually procPtr->bodyPtr,
 				 * but could be any code fragment compiled
 				 * in the context of this procedure.) */
    Namespace *nsPtr;		/* Namespace containing procedure. */
    CONST char *description;	/* string describing this body of code. */
    CONST char *procName;	/* Name of this procedure. */
{
    Interp *iPtr = (Interp*)interp;
    int result;
    Tcl_CallFrame frame;
    Proc *saveProcPtr;
    ByteCode *codePtr = (ByteCode *) bodyPtr->internalRep.otherValuePtr;
 
    /*
     * If necessary, compile the procedure's body. The compiler will
     * allocate frame slots for the procedure's non-argument local
     * variables. If the ByteCode already exists, make sure it hasn't been
     * invalidated by someone redefining a core command (this might make the
     * compiled code wrong). Also, if the code was compiled in/for a
     * different interpreter, we recompile it. Note that compiling the body
     * might increase procPtr->numCompiledLocals if new local variables are
     * found while compiling.
     *
     * Precompiled procedure bodies, however, are immutable and therefore
     * they are not recompiled, even if things have changed.
     */
 
    if (bodyPtr->typePtr == &tclByteCodeType) {
 	if (((Interp *) *codePtr->interpHandle != iPtr)
 	        || (codePtr->compileEpoch != iPtr->compileEpoch)
 	        || (codePtr->nsPtr != nsPtr)) {
            if (codePtr->flags & TCL_BYTECODE_PRECOMPILED) {
                if ((Interp *) *codePtr->interpHandle != iPtr) {
                    Tcl_AppendResult(interp,
                            "a precompiled script jumped interps", NULL);
                    return TCL_ERROR;
                }
	        codePtr->compileEpoch = iPtr->compileEpoch;
                codePtr->nsPtr = nsPtr;
            } else {
                (*tclByteCodeType.freeIntRepProc)(bodyPtr);
                bodyPtr->typePtr = (Tcl_ObjType *) NULL;
            }
 	}
    }
    if (bodyPtr->typePtr != &tclByteCodeType) {
 	int numChars;
 	char *ellipsis;
 	
 	if (tclTraceCompile >= 1) {
 	    /*
 	     * Display a line summarizing the top level command we
 	     * are about to compile.
 	     */
 
 	    numChars = strlen(procName);
 	    ellipsis = "";
 	    if (numChars > 50) {
 		numChars = 50;
 		ellipsis = "...";
 	    }
 	    fprintf(stdout, "Compiling %s \"%.*s%s\"\n",
 		    description, numChars, procName, ellipsis);
 	}
 	
 	/*
 	 * Plug the current procPtr into the interpreter and coerce
 	 * the code body to byte codes.  The interpreter needs to
 	 * know which proc it's compiling so that it can access its
 	 * list of compiled locals.
 	 *
 	 * TRICKY NOTE:  Be careful to push a call frame with the
 	 *   proper namespace context, so that the byte codes are
 	 *   compiled in the appropriate class context.
 	 */
 
 	saveProcPtr = iPtr->compiledProcPtr;
 	iPtr->compiledProcPtr = procPtr;
 
 	result = Tcl_PushCallFrame(interp, &frame,
		(Tcl_Namespace*)nsPtr, /* isProcCallFrame */ 0);
 
 	if (result == TCL_OK) {
	    result = tclByteCodeType.setFromAnyProc(interp, bodyPtr);
	    Tcl_PopCallFrame(interp);
	}
 
 	iPtr->compiledProcPtr = saveProcPtr;
 	
 	if (result != TCL_OK) {
 	    if (result == TCL_ERROR) {
		char buf[100 + TCL_INTEGER_SPACE];

		numChars = strlen(procName);
 		ellipsis = "";
 		if (numChars > 50) {
 		    numChars = 50;
 		    ellipsis = "...";
 		}
 		sprintf(buf, "\n    (compiling %s \"%.*s%s\", line %d)",
 			description, numChars, procName, ellipsis,
 			interp->errorLine);
 		Tcl_AddObjErrorInfo(interp, buf, -1);
 	    }
 	    return result;
 	}
    } else if (codePtr->nsEpoch != nsPtr->resolverEpoch) {
	register CompiledLocal *localPtr;
 	
	/*
	 * The resolver epoch has changed, but we only need to invalidate
	 * the resolver cache.
	 */

	for (localPtr = procPtr->firstLocalPtr;  localPtr != NULL;
	    localPtr = localPtr->nextPtr) {
	    localPtr->flags &= ~(VAR_RESOLVED);
	    if (localPtr->resolveInfo) {
		if (localPtr->resolveInfo->deleteProc) {
		    localPtr->resolveInfo->deleteProc(localPtr->resolveInfo);
		} else {
		    ckfree((char*)localPtr->resolveInfo);
		}
		localPtr->resolveInfo = NULL;
	    }
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcessProcResultCode --
 *
 *	Procedure called by TclObjInterpProc to process a return code other
 *	than TCL_OK returned by a Tcl procedure.
 *
 * Results:
 *	Depending on the argument return code, the result returned is
 *	another return code and the interpreter's result is set to a value
 *	to supplement that return code.
 *
 * Side effects:
 *	If the result returned is TCL_ERROR, traceback information about
 *	the procedure just executed is appended to the interpreter's
 *	"errorInfo" variable.
 *
 *----------------------------------------------------------------------
 */

static int
ProcessProcResultCode(interp, procName, nameLen, returnCode)
    Tcl_Interp *interp;		/* The interpreter in which the procedure
				 * was called and returned returnCode. */
    char *procName;		/* Name of the procedure. Used for error
				 * messages and trace information. */
    int nameLen;		/* Number of bytes in procedure's name. */
    int returnCode;		/* The unexpected result code. */
{
    Interp *iPtr = (Interp *) interp;

    if (returnCode == TCL_RETURN) {
	returnCode = TclUpdateReturnInfo(iPtr);
    } else if (returnCode == TCL_ERROR) {
	char msg[100 + TCL_INTEGER_SPACE];
	char *ellipsis = "";
	int numChars = nameLen;

	if (numChars > 60) {
	    numChars = 60;
	    ellipsis = "...";
	}
	sprintf(msg, "\n    (procedure \"%.*s%s\" line %d)",
		numChars, procName, ellipsis, iPtr->errorLine);
	Tcl_AddObjErrorInfo(interp, msg, -1);
    } else if (returnCode == TCL_BREAK) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
		"invoked \"break\" outside of a loop", -1);
	returnCode = TCL_ERROR;
    } else if (returnCode == TCL_CONTINUE) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "invoked \"continue\" outside of a loop", -1);
	returnCode = TCL_ERROR;
    }
    return returnCode;
}

/*
 *----------------------------------------------------------------------
 *
 * TclProcDeleteProc --
 *
 *	This procedure is invoked just before a command procedure is
 *	removed from an interpreter.  Its job is to release all the
 *	resources allocated to the procedure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed, unless the procedure is actively being
 *	executed.  In this case the cleanup is delayed until the
 *	last call to the current procedure completes.
 *
 *----------------------------------------------------------------------
 */

void
TclProcDeleteProc(clientData)
    ClientData clientData;		/* Procedure to be deleted. */
{
    Proc *procPtr = (Proc *) clientData;

    procPtr->refCount--;
    if (procPtr->refCount <= 0) {
	TclProcCleanupProc(procPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclProcCleanupProc --
 *
 *	This procedure does all the real work of freeing up a Proc
 *	structure.  It's called only when the structure's reference
 *	count becomes zero.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed.
 *
 *----------------------------------------------------------------------
 */

void
TclProcCleanupProc(procPtr)
    register Proc *procPtr;		/* Procedure to be deleted. */
{
    register CompiledLocal *localPtr;
    Tcl_Obj *bodyPtr = procPtr->bodyPtr;
    Tcl_Obj *defPtr;
    Tcl_ResolvedVarInfo *resVarInfo;

    if (bodyPtr != NULL) {
	Tcl_DecrRefCount(bodyPtr);
    }
    for (localPtr = procPtr->firstLocalPtr;  localPtr != NULL;  ) {
	CompiledLocal *nextPtr = localPtr->nextPtr;

        resVarInfo = localPtr->resolveInfo;
	if (resVarInfo) {
	    if (resVarInfo->deleteProc) {
		(*resVarInfo->deleteProc)(resVarInfo);
	    } else {
		ckfree((char *) resVarInfo);
	    }
        }

	if (localPtr->defValuePtr != NULL) {
	    defPtr = localPtr->defValuePtr;
	    Tcl_DecrRefCount(defPtr);
	}
	ckfree((char *) localPtr);
	localPtr = nextPtr;
    }
    ckfree((char *) procPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclUpdateReturnInfo --
 *
 *	This procedure is called when procedures return, and at other
 *	points where the TCL_RETURN code is used.  It examines fields
 *	such as iPtr->returnCode and iPtr->errorCode and modifies
 *	the real return status accordingly.
 *
 * Results:
 *	The return value is the true completion code to use for
 *	the procedure, instead of TCL_RETURN.
 *
 * Side effects:
 *	The errorInfo and errorCode variables may get modified.
 *
 *----------------------------------------------------------------------
 */

int
TclUpdateReturnInfo(iPtr)
    Interp *iPtr;		/* Interpreter for which TCL_RETURN
				 * exception is being processed. */
{
    int code;

    code = iPtr->returnCode;
    iPtr->returnCode = TCL_OK;
    if (code == TCL_ERROR) {
	Tcl_SetVar2((Tcl_Interp *) iPtr, "errorCode", (char *) NULL,
		(iPtr->errorCode != NULL) ? iPtr->errorCode : "NONE",
		TCL_GLOBAL_ONLY);
	iPtr->flags |= ERROR_CODE_SET;
	if (iPtr->errorInfo != NULL) {
	    Tcl_SetVar2((Tcl_Interp *) iPtr, "errorInfo", (char *) NULL,
		    iPtr->errorInfo, TCL_GLOBAL_ONLY);
	    iPtr->flags |= ERR_IN_PROGRESS;
	}
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetInterpProc --
 *
 *  Returns a pointer to the TclProcInterpProc procedure; this is different
 *  from the value obtained from the TclProcInterpProc reference on systems
 *  like Windows where import and export versions of a procedure exported
 *  by a DLL exist.
 *
 * Results:
 *  Returns the internal address of the TclProcInterpProc procedure.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

TclCmdProcType
TclGetInterpProc()
{
    return (TclCmdProcType) TclProcInterpProc;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetObjInterpProc --
 *
 *  Returns a pointer to the TclObjInterpProc procedure; this is different
 *  from the value obtained from the TclObjInterpProc reference on systems
 *  like Windows where import and export versions of a procedure exported
 *  by a DLL exist.
 *
 * Results:
 *  Returns the internal address of the TclObjInterpProc procedure.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

TclObjCmdProcType
TclGetObjInterpProc()
{
    return (TclObjCmdProcType) TclObjInterpProc;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNewProcBodyObj --
 *
 *  Creates a new object, of type "procbody", whose internal
 *  representation is the given Proc struct.
 *  The newly created object's reference count is 0.
 *
 * Results:
 *  Returns a pointer to a newly allocated Tcl_Obj, 0 on error.
 *
 * Side effects:
 *  The reference count in the ByteCode attached to the Proc is bumped up
 *  by one, since the internal rep stores a pointer to it.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclNewProcBodyObj(procPtr)
    Proc *procPtr;	/* the Proc struct to store as the internal
                         * representation. */
{
    Tcl_Obj *objPtr;

    if (!procPtr) {
        return (Tcl_Obj *) NULL;
    }
    
    objPtr = Tcl_NewStringObj("", 0);

    if (objPtr) {
        objPtr->typePtr = &tclProcBodyType;
        objPtr->internalRep.otherValuePtr = (VOID *) procPtr;

        procPtr->refCount++;
    }

    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcBodyDup --
 *
 *  Tcl_ObjType's Dup function for the proc body object.
 *  Bumps the reference count on the Proc stored in the internal
 *  representation.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Sets up the object in dupPtr to be a duplicate of the one in srcPtr.
 *
 *----------------------------------------------------------------------
 */

static void ProcBodyDup(srcPtr, dupPtr)
    Tcl_Obj *srcPtr;		/* object to copy */
    Tcl_Obj *dupPtr;		/* target object for the duplication */
{
    Proc *procPtr = (Proc *) srcPtr->internalRep.otherValuePtr;
    
    dupPtr->typePtr = &tclProcBodyType;
    dupPtr->internalRep.otherValuePtr = (VOID *) procPtr;
    procPtr->refCount++;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcBodyFree --
 *
 *  Tcl_ObjType's Free function for the proc body object.
 *  The reference count on its Proc struct is decreased by 1; if the count
 *  reaches 0, the proc is freed.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  If the reference count on the Proc struct reaches 0, the struct is freed.
 *
 *----------------------------------------------------------------------
 */

static void
ProcBodyFree(objPtr)
    Tcl_Obj *objPtr;		/* the object to clean up */
{
    Proc *procPtr = (Proc *) objPtr->internalRep.otherValuePtr;
    procPtr->refCount--;
    if (procPtr->refCount <= 0) {
        TclProcCleanupProc(procPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ProcBodySetFromAny --
 *
 *  Tcl_ObjType's SetFromAny function for the proc body object.
 *  Calls panic.
 *
 * Results:
 *  Theoretically returns a TCL result code.
 *
 * Side effects:
 *  Calls panic, since we can't set the value of the object from a string
 *  representation (or any other internal ones).
 *
 *----------------------------------------------------------------------
 */

static int
ProcBodySetFromAny(interp, objPtr)
    Tcl_Interp *interp;			/* current interpreter */
    Tcl_Obj *objPtr;			/* object pointer */
{
    panic("called ProcBodySetFromAny");

    /*
     * this to keep compilers happy.
     */
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcBodyUpdateString --
 *
 *  Tcl_ObjType's UpdateString function for the proc body object.
 *  Calls panic.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Calls panic, since we this type has no string representation.
 *
 *----------------------------------------------------------------------
 */

static void
ProcBodyUpdateString(objPtr)
    Tcl_Obj *objPtr;		/* the object to update */
{
    panic("called ProcBodyUpdateString");
}
