/* 
 * tclVar.c --
 *
 *	This file contains routines that implement Tcl variables
 *	(both scalars and arrays).
 *
 *	The implementation of arrays is modelled after an initial
 *	implementation by Mark Diekhans and Karl Lehenbauer.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2001 by Kevin B. Kenny.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"


/*
 * The strings below are used to indicate what went wrong when a
 * variable access is denied.
 */

static CONST char *noSuchVar =		"no such variable";
static CONST char *isArray =		"variable is array";
static CONST char *needArray =		"variable isn't array";
static CONST char *noSuchElement =	"no such element in array";
static CONST char *danglingElement =
				"upvar refers to element in deleted array";
static CONST char *danglingVar =	
				"upvar refers to variable in deleted namespace";
static CONST char *badNamespace =	"parent namespace doesn't exist";
static CONST char *missingName =	"missing variable name";
static CONST char *isArrayElement =	"name refers to an element in an array";

/*
 * Forward references to procedures defined later in this file:
 */

static int		CallVarTraces _ANSI_ARGS_((Interp *iPtr, Var *arrayPtr,
			    Var *varPtr, CONST char *part1, CONST char *part2,
			    int flags, CONST int leaveErrMsg));
static void		CleanupVar _ANSI_ARGS_((Var *varPtr,
			    Var *arrayPtr));
static void		DeleteSearches _ANSI_ARGS_((Var *arrayVarPtr));
static void		DeleteArray _ANSI_ARGS_((Interp *iPtr,
			    CONST char *arrayName, Var *varPtr, int flags));
static void		DisposeTraceResult _ANSI_ARGS_((int flags,
			    char *result));
static int              ObjMakeUpvar _ANSI_ARGS_((Tcl_Interp *interp, 
                            CallFrame *framePtr, Tcl_Obj *otherP1Ptr, 
                            CONST char *otherP2, CONST int otherFlags,
		            CONST char *myName, int myFlags, int index));
static Var *		NewVar _ANSI_ARGS_((void));
static ArraySearch *	ParseSearchId _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST Var *varPtr, CONST char *varName,
			    Tcl_Obj *handleObj));
static void		VarErrMsg _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST char *part1, CONST char *part2,
			    CONST char *operation, CONST char *reason));
static int		SetArraySearchObj _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));


/*
 * Functions defined in this file that may be exported in the future
 * for use by the bytecode compiler and engine or to the public interface.
 */

Var *		TclLookupSimpleVar _ANSI_ARGS_((Tcl_Interp *interp,
		    CONST char *varName, int flags, CONST int create,
		    CONST char **errMsgPtr, int *indexPtr));
int		TclObjUnsetVar2 _ANSI_ARGS_((Tcl_Interp *interp,
		    Tcl_Obj *part1Ptr, CONST char *part2, int flags));

static Tcl_FreeInternalRepProc FreeLocalVarName;
static Tcl_DupInternalRepProc DupLocalVarName;
static Tcl_UpdateStringProc UpdateLocalVarName;
static Tcl_FreeInternalRepProc FreeNsVarName;
static Tcl_DupInternalRepProc DupNsVarName;
static Tcl_FreeInternalRepProc FreeParsedVarName;
static Tcl_DupInternalRepProc DupParsedVarName;
static Tcl_UpdateStringProc UpdateParsedVarName;

/*
 * Types of Tcl_Objs used to cache variable lookups.
 *
 * 
 * localVarName - INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1 = pointer to the corresponding Proc 
 *   twoPtrValue.ptr2 = index into locals table
 *
 * nsVarName - INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1: pointer to the namespace containing the 
 *                     reference
 *   twoPtrValue.ptr2: pointer to the corresponding Var 
 *
 * parsedVarName - INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1 = pointer to the array name Tcl_Obj, 
 *                      or NULL if it is a scalar variable
 *   twoPtrValue.ptr2 = pointer to the element name string
 *                      (owned by this Tcl_Obj), or NULL if 
 *                      it is a scalar variable
 */

Tcl_ObjType tclLocalVarNameType = {
    "localVarName",
    FreeLocalVarName, DupLocalVarName, UpdateLocalVarName, NULL
};

Tcl_ObjType tclNsVarNameType = {
    "namespaceVarName",
    FreeNsVarName, DupNsVarName, NULL, NULL
};

Tcl_ObjType tclParsedVarNameType = {
    "parsedVarName",
    FreeParsedVarName, DupParsedVarName, UpdateParsedVarName, NULL
};

/*
 * Type of Tcl_Objs used to speed up array searches.
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1 = searchIdNumber as offset from (char*)NULL
 *   twoPtrValue.ptr2 = variableNameStartInString as offset from (char*)NULL
 *
 * Note that the value stored in ptr2 is the offset into the string of
 * the start of the variable name and not the address of the variable
 * name itself, as this can be safely copied.
 */
Tcl_ObjType tclArraySearchType = {
    "array search",
    NULL, NULL, NULL, SetArraySearchObj
};


/*
 *----------------------------------------------------------------------
 *
 * TclLookupVar --
 *
 *	This procedure is used to locate a variable given its name(s). It
 *      has been mostly superseded by TclObjLookupVar, it is now only used 
 *      by the string-based interfaces. It is kept in tcl8.4 mainly because 
 *      it is in the internal stubs table, so that some extension may be 
 *      calling it. 
 *
 * Results:
 *	The return value is a pointer to the variable structure indicated by
 *	part1 and part2, or NULL if the variable couldn't be found. If the
 *	variable is found, *arrayPtrPtr is filled in with the address of the
 *	variable structure for the array that contains the variable (or NULL
 *	if the variable is a scalar). If the variable can't be found and
 *	either createPart1 or createPart2 are 1, a new as-yet-undefined
 *	(VAR_UNDEFINED) variable structure is created, entered into a hash
 *	table, and returned.
 *
 *	If the variable isn't found and creation wasn't specified, or some
 *	other error occurs, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags. 
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED
 *	even if createPart1 or createPart2 are 1 (these only cause the hash
 *	table entry or array to be created). For example, the variable might
 *	be a global that has been unset but is still referenced by a
 *	procedure, or a variable that has been unset but it only being kept
 *	in existence (if VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *	New hashtable entries may be created if createPart1 or createPart2
 *	are 1.
 *
 *----------------------------------------------------------------------
 */
Var *
TclLookupVar(interp, part1, part2, flags, msg, createPart1, createPart2,
        arrayPtrPtr)
    Tcl_Interp *interp;		/* Interpreter to use for lookup. */
    CONST char *part1;	        /* If part2 isn't NULL, this is the name of
				 * an array. Otherwise, this
				 * is a full variable name that could
				 * include a parenthesized array element. */
    CONST char *part2;		/* Name of element within array, or NULL. */
    int flags;			/* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits matter. */
    CONST char *msg;			/* Verb to use in error messages, e.g.
				 * "read" or "set". Only needed if
				 * TCL_LEAVE_ERR_MSG is set in flags. */
    int createPart1;		/* If 1, create hash table entry for part 1
				 * of name, if it doesn't already exist. If
				 * 0, return error if it doesn't exist. */
    int createPart2;		/* If 1, create hash table entry for part 2
				 * of name, if it doesn't already exist. If
				 * 0, return error if it doesn't exist. */
    Var **arrayPtrPtr;		/* If the name refers to an element of an
				 * array, *arrayPtrPtr gets filled in with
				 * address of array variable. Otherwise
				 * this is set to NULL. */
{
    Var *varPtr;
    CONST char *elName;		/* Name of array element or NULL; may be
				 * same as part2, or may be openParen+1. */
    int openParen, closeParen;
                                /* If this procedure parses a name into
				 * array and index, these are the offsets to 
				 * the parens around the index.  Otherwise 
				 * they are -1. */
    register CONST char *p;
    CONST char *errMsg = NULL;
    int index;
#define VAR_NAME_BUF_SIZE 26
    char buffer[VAR_NAME_BUF_SIZE];
    char *newVarName = buffer;

    varPtr = NULL;
    *arrayPtrPtr = NULL;
    openParen = closeParen = -1;

    /*
     * Parse part1 into array name and index.
     * Always check if part1 is an array element name and allow it only if
     * part2 is not given.   
     * (if one does not care about creating array elements that can't be used
     *  from tcl, and prefer slightly better performance, one can put
     *  the following in an   if (part2 == NULL) { ... } block and remove
     *  the part2's test and error reporting  or move that code in array set)
     */

    elName = part2;
    for (p = part1; *p ; p++) {
	if (*p == '(') {
	    openParen = p - part1;
	    do {
		p++;
	    } while (*p != '\0');
	    p--;
	    if (*p == ')') {
		if (part2 != NULL) {
		    if (flags & TCL_LEAVE_ERR_MSG) {
			VarErrMsg(interp, part1, part2, msg, needArray);
		    }
		    return NULL;
		}
		closeParen = p - part1;
	    } else {
		openParen = -1;
	    }
	    break;
	}
    }
    if (openParen != -1) {
	if (closeParen >= VAR_NAME_BUF_SIZE) {
	    newVarName = ckalloc((unsigned int) (closeParen+1));
	}
	memcpy(newVarName, part1, (unsigned int) closeParen);
	newVarName[openParen] = '\0';
	newVarName[closeParen] = '\0';
	part1 = newVarName;
	elName = newVarName + openParen + 1;
    }

    varPtr = TclLookupSimpleVar(interp, part1, flags, 
            createPart1, &errMsg, &index);
    if (varPtr == NULL) {
	if ((errMsg != NULL) && (flags & TCL_LEAVE_ERR_MSG)) {
	    VarErrMsg(interp, part1, elName, msg, errMsg);
	}
    } else {
	while (TclIsVarLink(varPtr)) {
	    varPtr = varPtr->value.linkPtr;
	}
	if (elName != NULL) {
	    *arrayPtrPtr = varPtr;
	    varPtr = TclLookupArrayElement(interp, part1, elName, flags, 
		    msg, createPart1, createPart2, varPtr);
	}
    }
    if (newVarName != buffer) {
	ckfree(newVarName);
    }

    return varPtr;
	
#undef VAR_NAME_BUF_SIZE
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjLookupVar --
 *
 *	This procedure is used by virtually all of the variable code to
 *	locate a variable given its name(s). The parsing into array/element
 *      components and (if possible) the lookup results are cached in 
 *      part1Ptr, which is converted to one of the varNameTypes.
 *
 * Results:
 *	The return value is a pointer to the variable structure indicated by
 *	part1Ptr and part2, or NULL if the variable couldn't be found. If 
 *      the variable is found, *arrayPtrPtr is filled with the address of the
 *	variable structure for the array that contains the variable (or NULL
 *	if the variable is a scalar). If the variable can't be found and
 *	either createPart1 or createPart2 are 1, a new as-yet-undefined
 *	(VAR_UNDEFINED) variable structure is created, entered into a hash
 *	table, and returned.
 *
 *	If the variable isn't found and creation wasn't specified, or some
 *	other error occurs, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags. 
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED
 *	even if createPart1 or createPart2 are 1 (these only cause the hash
 *	table entry or array to be created). For example, the variable might
 *	be a global that has been unset but is still referenced by a
 *	procedure, or a variable that has been unset but it only being kept
 *	in existence (if VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *	New hashtable entries may be created if createPart1 or createPart2
 *	are 1.
 *      The object part1Ptr is converted to one of tclLocalVarNameType, 
 *      tclNsVarNameType or tclParsedVarNameType and caches as much of the
 *      lookup as it can.
 *
 *----------------------------------------------------------------------
 */
Var *
TclObjLookupVar(interp, part1Ptr, part2, flags, msg, createPart1, createPart2,
        arrayPtrPtr)
    Tcl_Interp *interp;		/* Interpreter to use for lookup. */
    register Tcl_Obj *part1Ptr;	/* If part2 isn't NULL, this is the name 
				 * of an array. Otherwise, this is a full 
				 * variable name that could include a parenthesized 
				 * array element. */
    CONST char *part2;		/* Name of element within array, or NULL. */
    int flags;		        /* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits matter. */
    CONST char *msg;		/* Verb to use in error messages, e.g.
				 * "read" or "set". Only needed if
				 * TCL_LEAVE_ERR_MSG is set in flags. */
    CONST int createPart1;	/* If 1, create hash table entry for part 1
				 * of name, if it doesn't already exist. If
				 * 0, return error if it doesn't exist. */
    CONST int createPart2;	/* If 1, create hash table entry for part 2
				 * of name, if it doesn't already exist. If
				 * 0, return error if it doesn't exist. */
    Var **arrayPtrPtr;		/* If the name refers to an element of an
				 * array, *arrayPtrPtr gets filled in with
				 * address of array variable. Otherwise
				 * this is set to NULL. */
{
    Interp *iPtr = (Interp *) interp;
    register Var *varPtr;	/* Points to the variable's in-frame Var
				 * structure. */
    char *part1;
    int index, len1, len2;
    int parsed = 0;
    Tcl_Obj *objPtr;
    Tcl_ObjType *typePtr = part1Ptr->typePtr;
    CONST char *errMsg = NULL;
    CallFrame *varFramePtr = iPtr->varFramePtr;
    Namespace *nsPtr;

    /*
     * If part1Ptr is a tclParsedVarNameType, separate it into the 
     * pre-parsed parts.
     */

    *arrayPtrPtr = NULL;
    if (typePtr == &tclParsedVarNameType) {
	if (part1Ptr->internalRep.twoPtrValue.ptr1 != NULL) {
	    if (part2 != NULL) {
		/*
		 * ERROR: part1Ptr is already an array element, cannot 
		 * specify a part2.
		 */

		if (flags & TCL_LEAVE_ERR_MSG) {
		    part1 = TclGetString(part1Ptr);
		    VarErrMsg(interp, part1, part2, msg, needArray);
		}
		return NULL;
	    }
	    part2 = (char *) part1Ptr->internalRep.twoPtrValue.ptr2;
	    part1Ptr = (Tcl_Obj *) part1Ptr->internalRep.twoPtrValue.ptr1;
	    typePtr = part1Ptr->typePtr;
	}
	parsed = 1;
    }
    part1 = Tcl_GetStringFromObj(part1Ptr, &len1);    

    nsPtr = ((varFramePtr == NULL)? iPtr->globalNsPtr : varFramePtr->nsPtr);
    if (nsPtr->varResProc != NULL || iPtr->resolverPtr != NULL) {
	goto doParse;
    }
    
    if (typePtr == &tclLocalVarNameType) {
	Proc *procPtr = (Proc *) part1Ptr->internalRep.twoPtrValue.ptr1;
	int localIndex = (int) part1Ptr->internalRep.twoPtrValue.ptr2;
	int useLocal;

	useLocal = ((varFramePtr != NULL) && varFramePtr->isProcCallFrame
	        && !(flags & (TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY)));
	if (useLocal && (procPtr == varFramePtr->procPtr)) {
	    /*
	     * part1Ptr points to an indexed local variable of the
	     * correct procedure: use the cached value.
	     */
	    
	    varPtr = &(varFramePtr->compiledLocals[localIndex]);
	    goto donePart1;
	}
	goto doneParsing;
    } else if (typePtr == &tclNsVarNameType) {
	Namespace *cachedNsPtr;
	int useGlobal, useReference;

	varPtr = (Var *) part1Ptr->internalRep.twoPtrValue.ptr2;
	cachedNsPtr = (Namespace *) part1Ptr->internalRep.twoPtrValue.ptr1;
	useGlobal = (cachedNsPtr == iPtr->globalNsPtr) 
	    && ((flags & TCL_GLOBAL_ONLY) 
		|| ((*part1 == ':') && (*(part1+1) == ':'))
		|| (varFramePtr == NULL) 
		|| (!varFramePtr->isProcCallFrame 
		    && (nsPtr == iPtr->globalNsPtr)));
	useReference = useGlobal || ((cachedNsPtr == nsPtr) 
	        && ((flags & TCL_NAMESPACE_ONLY) 
		    || (varFramePtr && !varFramePtr->isProcCallFrame 
			&& !(flags & TCL_GLOBAL_ONLY)
			/* careful: an undefined ns variable could
			 * be hiding a valid global reference. */
			&& !(varPtr->flags & VAR_UNDEFINED))));
	if (useReference && (varPtr->hPtr != NULL)) {
	    /*
	     * A straight global or namespace reference, use it. It isn't 
	     * so simple to deal with 'implicit' namespace references, i.e., 
	     * those where the reference could be to either a namespace 
	     * or a global variable. Those we lookup again.
	     *
	     * If (varPtr->hPtr == NULL), this might be a reference to a
	     * variable in a deleted namespace, kept alive by e.g. part1Ptr.
	     * We could conceivably be so unlucky that a new namespace was
	     * created at the same address as the deleted one, so to be 
	     * safe we test for a valid hPtr.
	     */
	    goto donePart1;
	}
	goto doneParsing;
    }

    doParse:
    if (!parsed && (*(part1 + len1 - 1) == ')')) {
	/*
	 * part1Ptr is possibly an unparsed array element.
	 */
	register int i;
	char *newPart2;
	len2 = -1;
	for (i = 0; i < len1; i++) {
	    if (*(part1 + i) == '(') {
		if (part2 != NULL) {
		    if (flags & TCL_LEAVE_ERR_MSG) {
			VarErrMsg(interp, part1, part2, msg, needArray);
		    }
		}			

		/*
		 * part1Ptr points to an array element; first copy 
		 * the element name to a new string part2.
		 */

		part2 = part1 + i + 1;
		len2 = len1 - i - 2;
		len1 = i;

		newPart2 = ckalloc((unsigned int) (len2+1));
		memcpy(newPart2, part2, (unsigned int) len2);
		*(newPart2+len2) = '\0';
		part2 = newPart2;

		/*
		 * Free the internal rep of the original part1Ptr, now
		 * renamed objPtr, and set it to tclParsedVarNameType.
		 */

		objPtr = part1Ptr;
		if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
		    typePtr->freeIntRepProc(objPtr);
		}
		objPtr->typePtr = &tclParsedVarNameType;

		/*
		 * Define a new string object to hold the new part1Ptr, i.e., 
		 * the array name. Set the internal rep of objPtr, reset
		 * typePtr and part1 to contain the references to the
		 * array name.
		 */

		part1Ptr = Tcl_NewStringObj(part1, len1);
		Tcl_IncrRefCount(part1Ptr);

		objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) part1Ptr;
		objPtr->internalRep.twoPtrValue.ptr2 = (VOID *) part2;		

		typePtr = part1Ptr->typePtr;
		part1 = TclGetString(part1Ptr);
		break;
	    }
	}
    }
    
    doneParsing:
    /*
     * part1Ptr is not an array element; look it up, and convert 
     * it to one of the cached types if possible.
     */

    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	typePtr->freeIntRepProc(part1Ptr);
	part1Ptr->typePtr = NULL;
    }

    varPtr = TclLookupSimpleVar(interp, part1, flags, 
            createPart1, &errMsg, &index);
    if (varPtr == NULL) {
	if ((errMsg != NULL) && (flags & TCL_LEAVE_ERR_MSG)) {
	    VarErrMsg(interp, part1, part2, msg, errMsg);
	}
	return NULL;
    }

    /*
     * Cache the newly found variable if possible.
     */

    if (index >= 0) {
        /*
	 * An indexed local variable.
	 */

	Proc *procPtr = ((Interp *) interp)->varFramePtr->procPtr;

	part1Ptr->typePtr = &tclLocalVarNameType;
	procPtr->refCount++;
	part1Ptr->internalRep.twoPtrValue.ptr1 = (VOID *) procPtr;
	part1Ptr->internalRep.twoPtrValue.ptr2 = (VOID *) index;
#if 0
    /*
     * TEMPORARYLY DISABLED tclNsVarNameType
     *
     * This is a stop-gap fix for [Bug 735335]; it may not address the 
     * real issue (which I haven't pinned down yet), but it avoids the 
     * segfault in the test case.
     * This optimisation will hopefully be turned back on soon.
     *      Miguel Sofer, 2003-05-12
     */

    } else if (index > -3) {
	Namespace *nsPtr;
    
	nsPtr = ((index == -1)? iPtr->globalNsPtr : varFramePtr->nsPtr);
	varPtr->refCount++;
	part1Ptr->typePtr = &tclNsVarNameType;
	part1Ptr->internalRep.twoPtrValue.ptr1 = (VOID *) nsPtr;
	part1Ptr->internalRep.twoPtrValue.ptr2 = (VOID *) varPtr;
#endif
    } else {
	/*
	 * At least mark part1Ptr as already parsed.
	 */
	part1Ptr->typePtr = &tclParsedVarNameType;
	part1Ptr->internalRep.twoPtrValue.ptr1 = NULL;
	part1Ptr->internalRep.twoPtrValue.ptr2 = NULL;
    }
    
    donePart1:
#if 0
    if (varPtr == NULL) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    part1 = TclGetString(part1Ptr);
	    VarErrMsg(interp, part1, part2, msg, 
		    "Cached variable reference is NULL.");
	}
	return NULL;
    }
#endif
    while (TclIsVarLink(varPtr)) {
	varPtr = varPtr->value.linkPtr;
    }

    if (part2 != NULL) {
	/*
	 * Array element sought: look it up.
	 */

	part1 = TclGetString(part1Ptr);
	*arrayPtrPtr = varPtr;
	varPtr = TclLookupArrayElement(interp, part1, part2, 
                flags, msg, createPart1, createPart2, varPtr);
    }
    return varPtr;
}

/*
 * This flag bit should not interfere with TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
 * or TCL_LEAVE_ERR_MSG; it signals that the variable lookup is performed for 
 * upvar (or similar) purposes, with slightly different rules:
 *   - Bug #696893 - variable is either proc-local or in the current
 *     namespace; never follow the second (global) resolution path 
 *   - Bug #631741 - do not use special namespace or interp resolvers
 */
#define LOOKUP_FOR_UPVAR 0x40000

/*
 *----------------------------------------------------------------------
 *
 * TclLookupSimpleVar --
 *
 *	This procedure is used by to locate a simple variable (i.e., not
 *      an array element) given its name.
 *
 * Results:
 *	The return value is a pointer to the variable structure indicated by
 *	varName, or NULL if the variable couldn't be found. If the variable 
 *      can't be found and create is 1, a new as-yet-undefined (VAR_UNDEFINED) 
 *      variable structure is created, entered into a hash table, and returned.
 *
 *      If the current CallFrame corresponds to a proc and the variable found is
 *      one of the compiledLocals, its index is placed in *indexPtr. Otherwise,
 *      *indexPtr will be set to (according to the needs of TclObjLookupVar):
 *               -1 a global reference
 *               -2 a reference to a namespace variable
 *               -3 a non-cachable reference, i.e., one of:
 *                    . non-indexed local var
 *                    . a reference of unknown origin;
 *                    . resolution by a namespace or interp resolver
 *
 *	If the variable isn't found and creation wasn't specified, or some
 *	other error occurs, NULL is returned and the corresponding error
 *	message is left in *errMsgPtr. 
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED
 *	even if create is 1 (this only causes the hash table entry to be
 *	created).  For example, the variable might be a global that has been
 *	unset but is still referenced by a procedure, or a variable that has
 *	been unset but it only being kept in existence (if VAR_UNDEFINED) by
 *	a trace.
 *
 * Side effects:
 *	A new hashtable entry may be created if create is 1.
 *
 *----------------------------------------------------------------------
 */

Var *
TclLookupSimpleVar(interp, varName, flags, create, errMsgPtr, indexPtr)
    Tcl_Interp *interp;		/* Interpreter to use for lookup. */
    CONST char *varName;        /* This is a simple variable name that could
				 * representa scalar or an array. */
    int flags;		        /* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * LOOKUP_FOR_UPVAR and TCL_LEAVE_ERR_MSG bits 
				 * matter. */
    CONST int create;		/* If 1, create hash table entry for varname,
				 * if it doesn't already exist. If 0, return 
				 * error if it doesn't exist. */
    CONST char **errMsgPtr;
    int *indexPtr;
{    
    Interp *iPtr = (Interp *) interp;
    CallFrame *varFramePtr = iPtr->varFramePtr;
				/* Points to the procedure call frame whose
				 * variables are currently in use. Same as
				 * the current procedure's frame, if any,
				 * unless an "uplevel" is executing. */
    Tcl_HashTable *tablePtr;	/* Points to the hashtable, if any, in which
				 * to look up the variable. */
    Tcl_Var var;                /* Used to search for global names. */
    Var *varPtr;		/* Points to the Var structure returned for
				 * the variable. */
    Namespace *varNsPtr, *cxtNsPtr, *dummy1Ptr, *dummy2Ptr;
    ResolverScheme *resPtr;
    Tcl_HashEntry *hPtr;
    int new, i, result;

    varPtr = NULL;
    varNsPtr = NULL;		/* set non-NULL if a nonlocal variable */
    *indexPtr = -3;

    if ((flags & TCL_GLOBAL_ONLY) || iPtr->varFramePtr == NULL) {
        cxtNsPtr = iPtr->globalNsPtr;
    } else {
        cxtNsPtr = iPtr->varFramePtr->nsPtr;
    }

    /*
     * If this namespace has a variable resolver, then give it first
     * crack at the variable resolution.  It may return a Tcl_Var
     * value, it may signal to continue onward, or it may signal
     * an error.
     */

    if ((cxtNsPtr->varResProc != NULL || iPtr->resolverPtr != NULL) 
	    && !(flags & LOOKUP_FOR_UPVAR)) {
        resPtr = iPtr->resolverPtr;

        if (cxtNsPtr->varResProc) {
            result = (*cxtNsPtr->varResProc)(interp, varName,
		    (Tcl_Namespace *) cxtNsPtr, flags, &var);
        } else {
            result = TCL_CONTINUE;
        }

        while (result == TCL_CONTINUE && resPtr) {
            if (resPtr->varResProc) {
                result = (*resPtr->varResProc)(interp, varName,
			(Tcl_Namespace *) cxtNsPtr, flags, &var);
            }
            resPtr = resPtr->nextPtr;
        }

        if (result == TCL_OK) {
            varPtr = (Var *) var;
	    return varPtr;
        } else if (result != TCL_CONTINUE) {
	    return NULL;
        }
    }

    /*
     * Look up varName. Look it up as either a namespace variable or as a
     * local variable in a procedure call frame (varFramePtr).
     * Interpret varName as a namespace variable if:
     *    1) so requested by a TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY flag,
     *    2) there is no active frame (we're at the global :: scope),
     *    3) the active frame was pushed to define the namespace context
     *       for a "namespace eval" or "namespace inscope" command,
     *    4) the name has namespace qualifiers ("::"s).
     * Otherwise, if varName is a local variable, search first in the
     * frame's array of compiler-allocated local variables, then in its
     * hashtable for runtime-created local variables.
     *
     * If create and the variable isn't found, create the variable and,
     * if necessary, create varFramePtr's local var hashtable.
     */

    if (((flags & (TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY)) != 0)
	    || (varFramePtr == NULL)
	    || !varFramePtr->isProcCallFrame
	    || (strstr(varName, "::") != NULL)) {
	CONST char *tail;
	int lookGlobal;
	
	lookGlobal = (flags & TCL_GLOBAL_ONLY) 
	    || (cxtNsPtr == iPtr->globalNsPtr)
	    || ((*varName == ':') && (*(varName+1) == ':'));
	if (lookGlobal) {
	    *indexPtr = -1;
	    flags = (flags | TCL_GLOBAL_ONLY) & ~(TCL_NAMESPACE_ONLY|LOOKUP_FOR_UPVAR);
	} else {
	    if (flags & LOOKUP_FOR_UPVAR) {
		flags = (flags | TCL_NAMESPACE_ONLY) & ~LOOKUP_FOR_UPVAR;
	    }
	    if (flags & TCL_NAMESPACE_ONLY) {
		*indexPtr = -2;
	    }
	} 

	/*
	 * Don't pass TCL_LEAVE_ERR_MSG, we may yet create the variable,
	 * or otherwise generate our own error!
	 */
	var = Tcl_FindNamespaceVar(interp, varName, (Tcl_Namespace *) cxtNsPtr,
		flags & ~TCL_LEAVE_ERR_MSG);
	if (var != (Tcl_Var) NULL) {
            varPtr = (Var *) var;
        }
	if (varPtr == NULL) {
	    if (create) {   /* var wasn't found so create it  */
		TclGetNamespaceForQualName(interp, varName, cxtNsPtr,
			flags, &varNsPtr, &dummy1Ptr, &dummy2Ptr, &tail);
		if (varNsPtr == NULL) {
		    *errMsgPtr = badNamespace;
		    return NULL;
		}
		if (tail == NULL) {
		    *errMsgPtr = missingName;
		    return NULL;
		}
		hPtr = Tcl_CreateHashEntry(&varNsPtr->varTable, tail, &new);
		varPtr = NewVar();
		Tcl_SetHashValue(hPtr, varPtr);
		varPtr->hPtr = hPtr;
		varPtr->nsPtr = varNsPtr;
		if ((lookGlobal)  || (varNsPtr == NULL)) {
		    /*
		     * The variable was created starting from the global
		     * namespace: a global reference is returned even if 
		     * it wasn't explicitly requested.
		     */
		    *indexPtr = -1;
		} else {
		    *indexPtr = -2;
		}
	    } else {		/* var wasn't found and not to create it */
		*errMsgPtr = noSuchVar;
		return NULL;
	    }
	}
    } else {			/* local var: look in frame varFramePtr */
	Proc *procPtr = varFramePtr->procPtr;
	int localCt = procPtr->numCompiledLocals;
	CompiledLocal *localPtr = procPtr->firstLocalPtr;
	Var *localVarPtr = varFramePtr->compiledLocals;
	int varNameLen = strlen(varName);
	
	for (i = 0;  i < localCt;  i++) {
	    if (!TclIsVarTemporary(localPtr)) {
		register char *localName = localVarPtr->name;
		if ((varName[0] == localName[0])
		        && (varNameLen == localPtr->nameLength)
		        && (strcmp(varName, localName) == 0)) {
		    *indexPtr = i;
		    return localVarPtr;
		}
	    }
	    localVarPtr++;
	    localPtr = localPtr->nextPtr;
	}
	tablePtr = varFramePtr->varTablePtr;
	if (create) {
	    if (tablePtr == NULL) {
		tablePtr = (Tcl_HashTable *)
		    ckalloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr, TCL_STRING_KEYS);
		varFramePtr->varTablePtr = tablePtr;
	    }
	    hPtr = Tcl_CreateHashEntry(tablePtr, varName, &new);
	    if (new) {
		varPtr = NewVar();
		Tcl_SetHashValue(hPtr, varPtr);
		varPtr->hPtr = hPtr;
		varPtr->nsPtr = NULL; /* a local variable */
	    } else {
		varPtr = (Var *) Tcl_GetHashValue(hPtr);
	    }
	} else {
	    hPtr = NULL;
	    if (tablePtr != NULL) {
		hPtr = Tcl_FindHashEntry(tablePtr, varName);
	    }
	    if (hPtr == NULL) {
		*errMsgPtr = noSuchVar;
		return NULL;
	    }
	    varPtr = (Var *) Tcl_GetHashValue(hPtr);
	}
    }
    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclLookupArrayElement --
 *
 *	This procedure is used to locate a variable which is in an array's 
 *      hashtable given a pointer to the array's Var structure and the 
 *      element's name.
 *
 * Results:
 *	The return value is a pointer to the variable structure , or NULL if 
 *      the variable couldn't be found. 
 *
 *      If arrayPtr points to a variable that isn't an array and createPart1 
 *      is 1, the corresponding variable will be converted to an array. 
 *      Otherwise, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags.
 *
 *      If the variable is not found and createPart2 is 1, the variable is
 *      created. Otherwise, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags.
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED
 *	even if createPart1 or createPart2 are 1 (these only cause the hash
 *	table entry or array to be created). For example, the variable might
 *	be a global that has been unset but is still referenced by a
 *	procedure, or a variable that has been unset but it only being kept
 *	in existence (if VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *      The variable at arrayPtr may be converted to be an array if 
 *      createPart1 is 1. A new hashtable entry may be created if createPart2 
 *      is 1.
 *
 *----------------------------------------------------------------------
 */

Var *
TclLookupArrayElement(interp, arrayName, elName, flags, msg, createArray, createElem, arrayPtr)
    Tcl_Interp *interp;		/* Interpreter to use for lookup. */
    CONST char *arrayName;	        /* This is the name of the array. */
    CONST char *elName;		/* Name of element within array. */
    CONST int flags;		/* Only TCL_LEAVE_ERR_MSG bit matters. */
    CONST char *msg;			/* Verb to use in error messages, e.g.
				 * "read" or "set". Only needed if
				 * TCL_LEAVE_ERR_MSG is set in flags. */
    CONST int createArray;	/* If 1, transform arrayName to be an array
				 * if it isn't one yet and the transformation 
				 * is possible. If 0, return error if it 
				 * isn't already an array. */
    CONST int createElem;	/* If 1, create hash table entry for the 
				 * element, if it doesn't already exist. If
				 * 0, return error if it doesn't exist. */
    Var *arrayPtr;	        /* Pointer to the array's Var structure. */
{
    Tcl_HashEntry *hPtr;
    int new;
    Var *varPtr;

    /*
     * We're dealing with an array element. Make sure the variable is an
     * array and look up the element (create the element if desired).
     */

    if (TclIsVarUndefined(arrayPtr) && !TclIsVarArrayElement(arrayPtr)) {
	if (!createArray) {
	    if (flags & TCL_LEAVE_ERR_MSG) {
		VarErrMsg(interp, arrayName, elName, msg, noSuchVar);
	    }
	    return NULL;
	}

	/*
	 * Make sure we are not resurrecting a namespace variable from a
	 * deleted namespace!
	 */
	if ((arrayPtr->flags & VAR_IN_HASHTABLE) && (arrayPtr->hPtr == NULL)) {
	    if (flags & TCL_LEAVE_ERR_MSG) {
		VarErrMsg(interp, arrayName, elName, msg, danglingVar);
	    }
	    return NULL;
	}

	TclSetVarArray(arrayPtr);
	TclClearVarUndefined(arrayPtr);
	arrayPtr->value.tablePtr =
	    (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(arrayPtr->value.tablePtr, TCL_STRING_KEYS);
    } else if (!TclIsVarArray(arrayPtr)) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    VarErrMsg(interp, arrayName, elName, msg, needArray);
	}
	return NULL;
    }

    if (createElem) {
	hPtr = Tcl_CreateHashEntry(arrayPtr->value.tablePtr, elName, &new);
	if (new) {
	    if (arrayPtr->searchPtr != NULL) {
		DeleteSearches(arrayPtr);
	    }
	    varPtr = NewVar();
	    Tcl_SetHashValue(hPtr, varPtr);
	    varPtr->hPtr = hPtr;
	    varPtr->nsPtr = arrayPtr->nsPtr;
	    TclSetVarArrayElement(varPtr);
	}
    } else {
	hPtr = Tcl_FindHashEntry(arrayPtr->value.tablePtr, elName);
	if (hPtr == NULL) {
	    if (flags & TCL_LEAVE_ERR_MSG) {
		VarErrMsg(interp, arrayName, elName, msg, noSuchElement);
	    }
	    return NULL;
	}
    }
    return (Var *) Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVar --
 *
 *	Return the value of a Tcl variable as a string.
 *
 * Results:
 *	The return value points to the current value of varName as a string.
 *	If the variable is not defined or can't be read because of a clash
 *	in array usage then a NULL pointer is returned and an error message
 *	is left in the interp's result if the TCL_LEAVE_ERR_MSG flag is set.
 *	Note: the return value is only valid up until the next change to the
 *	variable; if you depend on the value lasting longer than that, then
 *	make yourself a private copy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

CONST char *
Tcl_GetVar(interp, varName, flags)
    Tcl_Interp *interp;		/* Command interpreter in which varName is
				 * to be looked up. */
    CONST char *varName;	/* Name of a variable in interp. */
    int flags;			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY or TCL_LEAVE_ERR_MSG
				 * bits. */
{
    return Tcl_GetVar2(interp, varName, (char *) NULL, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVar2 --
 *
 *	Return the value of a Tcl variable as a string, given a two-part
 *	name consisting of array name and element within array.
 *
 * Results:
 *	The return value points to the current value of the variable given
 *	by part1 and part2 as a string. If the specified variable doesn't
 *	exist, or if there is a clash in array usage, then NULL is returned
 *	and a message will be left in the interp's result if the
 *	TCL_LEAVE_ERR_MSG flag is set. Note: the return value is only valid
 *	up until the next change to the variable; if you depend on the value
 *	lasting longer than that, then make yourself a private copy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

CONST char *
Tcl_GetVar2(interp, part1, part2, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be looked up. */
    CONST char *part1;		/* Name of an array (if part2 is non-NULL)
				 * or the name of a variable. */
    CONST char *part2;		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    int flags;			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY and TCL_LEAVE_ERR_MSG
                                 * bits. */
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_GetVar2Ex(interp, part1, part2, flags);
    if (objPtr == NULL) {
	return NULL;
    }
    return TclGetString(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVar2Ex --
 *
 *	Return the value of a Tcl variable as a Tcl object, given a
 *	two-part name consisting of array name and element within array.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by part1Ptr and part2Ptr. If the specified variable doesn't
 *	exist, or if there is a clash in array usage, then NULL is returned
 *	and a message will be left in the interpreter's result if the
 *	TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to
 *	reflect the returned reference; if you want to keep a reference to
 *	the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetVar2Ex(interp, part1, part2, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be looked up. */
    CONST char *part1;		/* Name of an array (if part2 is non-NULL)
				 * or the name of a variable. */
    CONST char *part2;		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    int flags;			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits. */
{
    Var *varPtr, *arrayPtr;

    /*
     * We need a special flag check to see if we want to create part 1,
     * because commands like lappend require read traces to trigger for
     * previously non-existent values.
     */
    varPtr = TclLookupVar(interp, part1, part2, flags, "read",
            /*createPart1*/ (flags & TCL_TRACE_READS),
	    /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    return TclPtrGetVar(interp, varPtr, arrayPtr, part1, part2, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ObjGetVar2 --
 *
 *	Return the value of a Tcl variable as a Tcl object, given a
 *	two-part name consisting of array name and element within array.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by part1Ptr and part2Ptr. If the specified variable doesn't
 *	exist, or if there is a clash in array usage, then NULL is returned
 *	and a message will be left in the interpreter's result if the
 *	TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to
 *	reflect the returned reference; if you want to keep a reference to
 *	the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ObjGetVar2(interp, part1Ptr, part2Ptr, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be looked up. */
    register Tcl_Obj *part1Ptr;	/* Points to an object holding the name of
				 * an array (if part2 is non-NULL) or the
				 * name of a variable. */
    register Tcl_Obj *part2Ptr;	/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    int flags;			/* OR-ed combination of TCL_GLOBAL_ONLY and
				 * TCL_LEAVE_ERR_MSG bits. */
{
    Var *varPtr, *arrayPtr;
    char *part1, *part2;

    part1 = Tcl_GetString(part1Ptr);
    part2 = ((part2Ptr == NULL) ? NULL : Tcl_GetString(part2Ptr));
    
    /*
     * We need a special flag check to see if we want to create part 1,
     * because commands like lappend require read traces to trigger for
     * previously non-existent values.
     */
    varPtr = TclObjLookupVar(interp, part1Ptr, part2, flags, "read",
            /*createPart1*/ (flags & TCL_TRACE_READS),
	    /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    return TclPtrGetVar(interp, varPtr, arrayPtr, part1, part2, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrGetVar --
 *
 *	Return the value of a Tcl variable as a Tcl object, given the
 *      pointers to the variable's (and possibly containing array's) 
 *      VAR structure.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by varPtr. If the specified variable doesn't exist, or if there 
 *      is a clash in array usage, then NULL is returned and a message will be 
 *      left in the interpreter's result if the TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to
 *	reflect the returned reference; if you want to keep a reference to
 *	the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrGetVar(interp, varPtr, arrayPtr, part1, part2, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be looked up. */
    register Var *varPtr;       /* The variable to be read.*/
    Var *arrayPtr;              /* NULL for scalar variables, pointer to
				 * the containing array otherwise. */
    CONST char *part1;		/* Name of an array (if part2 is non-NULL)
				 * or the name of a variable. */
    CONST char *part2;		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    CONST int flags;		/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits. */
{
    Interp *iPtr = (Interp *) interp;
    CONST char *msg;

    /*
     * Invoke any traces that have been set for the variable.
     */

    if ((varPtr->tracePtr != NULL)
	    || ((arrayPtr != NULL) && (arrayPtr->tracePtr != NULL))) {
	if (TCL_ERROR == CallVarTraces(iPtr, arrayPtr, varPtr, part1, part2,
		(flags & (TCL_NAMESPACE_ONLY|TCL_GLOBAL_ONLY))
		| TCL_TRACE_READS, (flags & TCL_LEAVE_ERR_MSG))) {
	    goto errorReturn;
	}
    }

    /*
     * Return the element if it's an existing scalar variable.
     */
    
    if (TclIsVarScalar(varPtr) && !TclIsVarUndefined(varPtr)) {
	return varPtr->value.objPtr;
    }
    
    if (flags & TCL_LEAVE_ERR_MSG) {
	if (TclIsVarUndefined(varPtr) && (arrayPtr != NULL)
	        && !TclIsVarUndefined(arrayPtr)) {
	    msg = noSuchElement;
	} else if (TclIsVarArray(varPtr)) {
	    msg = isArray;
	} else {
	    msg = noSuchVar;
	}
	VarErrMsg(interp, part1, part2, "read", msg);
    }

    /*
     * An error. If the variable doesn't exist anymore and no-one's using
     * it, then free up the relevant structures and hash table entries.
     */

    errorReturn:
    if (TclIsVarUndefined(varPtr)) {
	CleanupVar(varPtr, arrayPtr);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjCmd --
 *
 *	This procedure is invoked to process the "set" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result value.
 *
 * Side effects:
 *	A variable's value may be changed.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_SetObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    Tcl_Obj *varValueObj;

    if (objc == 2) {
	varValueObj = Tcl_ObjGetVar2(interp, objv[1], NULL, TCL_LEAVE_ERR_MSG);
	if (varValueObj == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, varValueObj);
	return TCL_OK;
    } else if (objc == 3) {

	varValueObj = Tcl_ObjSetVar2(interp, objv[1], NULL, objv[2],
		TCL_LEAVE_ERR_MSG);
	if (varValueObj == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, varValueObj);
	return TCL_OK;
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?newValue?");
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetVar --
 *
 *	Change the value of a variable.
 *
 * Results:
 *	Returns a pointer to the malloc'ed string which is the character
 *	representation of the variable's new value. The caller must not
 *	modify this string. If the write operation was disallowed then NULL
 *	is returned; if the TCL_LEAVE_ERR_MSG flag is set, then an
 *	explanatory message will be left in the interp's result. Note that the
 *	returned string may not be the same as newValue; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	If varName is defined as a local or global variable in interp,
 *	its value is changed to newValue. If varName isn't currently
 *	defined, then a new global variable by that name is created.
 *
 *----------------------------------------------------------------------
 */

CONST char *
Tcl_SetVar(interp, varName, newValue, flags)
    Tcl_Interp *interp;		/* Command interpreter in which varName is
				 * to be looked up. */
    CONST char *varName;	/* Name of a variable in interp. */
    CONST char *newValue;	/* New value for varName. */
    int flags;			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_APPEND_VALUE,
				 * TCL_LIST_ELEMENT, TCL_LEAVE_ERR_MSG. */
{
    return Tcl_SetVar2(interp, varName, (char *) NULL, newValue, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetVar2 --
 *
 *      Given a two-part variable name, which may refer either to a
 *      scalar variable or an element of an array, change the value
 *      of the variable.  If the named scalar or array or element
 *      doesn't exist then create one.
 *
 * Results:
 *	Returns a pointer to the malloc'ed string which is the character
 *	representation of the variable's new value. The caller must not
 *	modify this string. If the write operation was disallowed because an
 *	array was expected but not found (or vice versa), then NULL is
 *	returned; if the TCL_LEAVE_ERR_MSG flag is set, then an explanatory
 *	message will be left in the interp's result. Note that the returned
 *	string may not be the same as newValue; this is because variable
 *	traces may modify the variable's value.
 *
 * Side effects:
 *      The value of the given variable is set. If either the array
 *      or the entry didn't exist then a new one is created.
 *
 *----------------------------------------------------------------------
 */

CONST char *
Tcl_SetVar2(interp, part1, part2, newValue, flags)
    Tcl_Interp *interp;         /* Command interpreter in which variable is
                                 * to be looked up. */
    CONST char *part1;          /* If part2 is NULL, this is name of scalar
                                 * variable. Otherwise it is the name of
                                 * an array. */
    CONST char *part2;		/* Name of an element within an array, or
				 * NULL. */
    CONST char *newValue;       /* New value for variable. */
    int flags;                  /* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_APPEND_VALUE,
				 * TCL_LIST_ELEMENT, or TCL_LEAVE_ERR_MSG */
{
    register Tcl_Obj *valuePtr;
    Tcl_Obj *varValuePtr;

    /*
     * Create an object holding the variable's new value and use
     * Tcl_SetVar2Ex to actually set the variable.
     */

    valuePtr = Tcl_NewStringObj(newValue, -1);
    Tcl_IncrRefCount(valuePtr);

    varValuePtr = Tcl_SetVar2Ex(interp, part1, part2, valuePtr, flags);
    Tcl_DecrRefCount(valuePtr); /* done with the object */
    
    if (varValuePtr == NULL) {
	return NULL;
    }
    return TclGetString(varValuePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetVar2Ex --
 *
 *	Given a two-part variable name, which may refer either to a scalar
 *	variable or an element of an array, change the value of the variable
 *	to a new Tcl object value. If the named scalar or array or element
 *	doesn't exist then create one.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if
 *	the TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will
 *	be left in the interpreter's result. Note that the returned object
 *	may not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.
 *
 *	The reference count is decremented for any old value of the variable
 *	and incremented for its new value. If the new value for the variable
 *	is not the same one referenced by newValuePtr (perhaps as a result
 *	of a variable trace), then newValuePtr's ref count is left unchanged
 *	by Tcl_SetVar2Ex. newValuePtr's ref count is also left unchanged if
 *	we are appending it as a string value: that is, if "flags" includes
 *	TCL_APPEND_VALUE but not TCL_LIST_ELEMENT.
 *
 *	The reference count for the returned object is _not_ incremented: if
 *	you want to keep a reference to the object you must increment its
 *	ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_SetVar2Ex(interp, part1, part2, newValuePtr, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be found. */
    CONST char *part1;		/* Name of an array (if part2 is non-NULL)
				 * or the name of a variable. */
    CONST char *part2;		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    Tcl_Obj *newValuePtr;	/* New value for variable. */
    int flags;			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_APPEND_VALUE,
				 * TCL_LIST_ELEMENT or TCL_LEAVE_ERR_MSG. */
{
    Var *varPtr, *arrayPtr;

    varPtr = TclLookupVar(interp, part1, part2, flags, "set",
	    /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    return TclPtrSetVar(interp, varPtr, arrayPtr, part1, part2, 
            newValuePtr, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ObjSetVar2 --
 *
 *	This function is the same as Tcl_SetVar2Ex above, except the
 *	variable names are passed in Tcl object instead of strings.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if
 *	the TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will
 *	be left in the interpreter's result. Note that the returned object
 *	may not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ObjSetVar2(interp, part1Ptr, part2Ptr, newValuePtr, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be found. */
    register Tcl_Obj *part1Ptr;	/* Points to an object holding the name of
				 * an array (if part2 is non-NULL) or the
				 * name of a variable. */
    register Tcl_Obj *part2Ptr;	/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    Tcl_Obj *newValuePtr;	/* New value for variable. */
    int flags;			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_APPEND_VALUE,
				 * TCL_LIST_ELEMENT, or TCL_LEAVE_ERR_MSG. */
{
    Var *varPtr, *arrayPtr;
    char *part1, *part2;

    part1 = TclGetString(part1Ptr);
    part2 = ((part2Ptr == NULL) ? NULL : Tcl_GetString(part2Ptr));    

    varPtr = TclObjLookupVar(interp, part1Ptr, part2, flags, "set",
	    /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    return TclPtrSetVar(interp, varPtr, arrayPtr, part1, part2, 
            newValuePtr, flags);
}


/*
 *----------------------------------------------------------------------
 *
 * TclPtrSetVar --
 *
 *	This function is the same as Tcl_SetVar2Ex above, except that
 *      it requires pointers to the variable's Var structs in addition
 *	to the variable names.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if
 *	the TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will
 *	be left in the interpreter's result. Note that the returned object
 *	may not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.

 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrSetVar(interp, varPtr, arrayPtr, part1, part2, newValuePtr, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be looked up. */
    register Var *varPtr;
    Var *arrayPtr;
    CONST char *part1;		/* Name of an array (if part2 is non-NULL)
				 * or the name of a variable. */
    CONST char *part2;		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    Tcl_Obj *newValuePtr;	/* New value for variable. */
    CONST int flags;			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *oldValuePtr;
    Tcl_Obj *resultPtr = NULL;
    int result;

    /*
     * If the variable is in a hashtable and its hPtr field is NULL, then we
     * may have an upvar to an array element where the array was deleted
     * or an upvar to a namespace variable whose namespace was deleted.
     * Generate an error (allowing the variable to be reset would screw up
     * our storage allocation and is meaningless anyway).
     */

    if ((varPtr->flags & VAR_IN_HASHTABLE) && (varPtr->hPtr == NULL)) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    if (TclIsVarArrayElement(varPtr)) {
		VarErrMsg(interp, part1, part2, "set", danglingElement);
	    } else {
		VarErrMsg(interp, part1, part2, "set", danglingVar);
	    }
	}
	return NULL;
    }

    /*
     * It's an error to try to set an array variable itself.
     */

    if (TclIsVarArray(varPtr) && !TclIsVarUndefined(varPtr)) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    VarErrMsg(interp, part1, part2, "set", isArray);
	}
	return NULL;
    }

    /*
     * Invoke any read traces that have been set for the variable if it
     * is requested; this is only done in the core when lappending.
     */

    if ((flags & TCL_TRACE_READS) && ((varPtr->tracePtr != NULL) 
	    || ((arrayPtr != NULL) && (arrayPtr->tracePtr != NULL)))) {
	if (TCL_ERROR == CallVarTraces(iPtr, arrayPtr, varPtr, part1, part2,
		TCL_TRACE_READS, (flags & TCL_LEAVE_ERR_MSG))) {
	    return NULL;
	}
    }

    /*
     * Set the variable's new value. If appending, append the new value to
     * the variable, either as a list element or as a string. Also, if
     * appending, then if the variable's old value is unshared we can modify
     * it directly, otherwise we must create a new copy to modify: this is
     * "copy on write".
     */

    oldValuePtr = varPtr->value.objPtr;
    if (flags & TCL_APPEND_VALUE) {
	if (TclIsVarUndefined(varPtr) && (oldValuePtr != NULL)) {
	    Tcl_DecrRefCount(oldValuePtr);     /* discard old value */
	    varPtr->value.objPtr = NULL;
	    oldValuePtr = NULL;
	}
	if (flags & TCL_LIST_ELEMENT) {	       /* append list element */
	    if (oldValuePtr == NULL) {
		TclNewObj(oldValuePtr);
		varPtr->value.objPtr = oldValuePtr;
		Tcl_IncrRefCount(oldValuePtr); /* since var is referenced */
	    } else if (Tcl_IsShared(oldValuePtr)) {
		varPtr->value.objPtr = Tcl_DuplicateObj(oldValuePtr);
		Tcl_DecrRefCount(oldValuePtr);
		oldValuePtr = varPtr->value.objPtr;
		Tcl_IncrRefCount(oldValuePtr); /* since var is referenced */
	    }
	    result = Tcl_ListObjAppendElement(interp, oldValuePtr,
		    newValuePtr);
	    if (result != TCL_OK) {
		return NULL;
	    }
	} else {		               /* append string */
	    /*
	     * We append newValuePtr's bytes but don't change its ref count.
	     */

	    if (oldValuePtr == NULL) {
		varPtr->value.objPtr = newValuePtr;
		Tcl_IncrRefCount(newValuePtr);
	    } else {
		if (Tcl_IsShared(oldValuePtr)) {   /* append to copy */
		    varPtr->value.objPtr = Tcl_DuplicateObj(oldValuePtr);
		    TclDecrRefCount(oldValuePtr);
		    oldValuePtr = varPtr->value.objPtr;
		    Tcl_IncrRefCount(oldValuePtr); /* since var is ref */
		}
		Tcl_AppendObjToObj(oldValuePtr, newValuePtr);
	    }
	}
    } else if (newValuePtr != oldValuePtr) {
	/*
	 * In this case we are replacing the value, so we don't need to
	 * do more than swap the objects.
	 */

	varPtr->value.objPtr = newValuePtr;
	Tcl_IncrRefCount(newValuePtr);      /* var is another ref */
	if (oldValuePtr != NULL) {
	    TclDecrRefCount(oldValuePtr);   /* discard old value */
	}
    }
    TclSetVarScalar(varPtr);
    TclClearVarUndefined(varPtr);
    if (arrayPtr != NULL) {
	TclClearVarUndefined(arrayPtr);
    }

    /*
     * Invoke any write traces for the variable.
     */

    if ((varPtr->tracePtr != NULL)
	    || ((arrayPtr != NULL) && (arrayPtr->tracePtr != NULL))) {
	if (TCL_ERROR == CallVarTraces(iPtr, arrayPtr, varPtr, part1, part2,
	        (flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY))
		| TCL_TRACE_WRITES, (flags & TCL_LEAVE_ERR_MSG))) {
	    goto cleanup;
	}
    }

    /*
     * Return the variable's value unless the variable was changed in some
     * gross way by a trace (e.g. it was unset and then recreated as an
     * array). 
     */

    if (TclIsVarScalar(varPtr) && !TclIsVarUndefined(varPtr)) {
	return varPtr->value.objPtr;
    }

    /*
     * A trace changed the value in some gross way. Return an empty string
     * object.
     */
    
    resultPtr = iPtr->emptyObjPtr;

    /*
     * If the variable doesn't exist anymore and no-one's using it, then
     * free up the relevant structures and hash table entries.
     */

    cleanup:
    if (TclIsVarUndefined(varPtr)) {
	CleanupVar(varPtr, arrayPtr);
    }
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclIncrVar2 --
 *
 *	Given a two-part variable name, which may refer either to a scalar
 *	variable or an element of an array, increment the Tcl object value
 *	of the variable by a specified amount.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the specified variable doesn't exist, or there is a
 *	clash in array usage, or an error occurs while executing variable
 *	traces, then NULL is returned and a message will be left in
 *	the interpreter's result.
 *
 * Side effects:
 *	The value of the given variable is incremented by the specified
 *	amount. If either the array or the entry didn't exist then a new
 *	variable is created. The ref count for the returned object is _not_
 *	incremented to reflect the returned reference; if you want to keep a
 *	reference to the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclIncrVar2(interp, part1Ptr, part2Ptr, incrAmount, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be found. */
    Tcl_Obj *part1Ptr;		/* Points to an object holding the name of
				 * an array (if part2 is non-NULL) or the
				 * name of a variable. */
    Tcl_Obj *part2Ptr;		/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    long incrAmount;		/* Amount to be added to variable. */
    int flags;                  /* Various flags that tell how to incr value:
				 * any of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_APPEND_VALUE,
				 * TCL_LIST_ELEMENT, TCL_LEAVE_ERR_MSG. */
{
    Var *varPtr, *arrayPtr;
    char *part1, *part2;

    part1 = TclGetString(part1Ptr);
    part2 = ((part2Ptr == NULL)? NULL : TclGetString(part2Ptr));

    varPtr = TclObjLookupVar(interp, part1Ptr, part2, flags, "read",
	    0, 1, &arrayPtr);
    if (varPtr == NULL) {
	Tcl_AddObjErrorInfo(interp,
		"\n    (reading value of variable to increment)", -1);
	return NULL;
    }
    return TclPtrIncrVar(interp, varPtr, arrayPtr, part1, part2,
	    incrAmount, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrIncrVar --
 *
 *	Given the pointers to a variable and possible containing array, 
 *      increment the Tcl object value of the variable by a specified 
 *      amount.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the specified variable doesn't exist, or there is a
 *	clash in array usage, or an error occurs while executing variable
 *	traces, then NULL is returned and a message will be left in
 *	the interpreter's result.
 *
 * Side effects:
 *	The value of the given variable is incremented by the specified
 *	amount. If either the array or the entry didn't exist then a new
 *	variable is created. The ref count for the returned object is _not_
 *	incremented to reflect the returned reference; if you want to keep a
 *	reference to the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrIncrVar(interp, varPtr, arrayPtr, part1, part2, incrAmount, flags)
    Tcl_Interp *interp;		/* Command interpreter in which variable is
				 * to be found. */
    Var *varPtr;
    Var *arrayPtr;
    CONST char *part1;		/* Points to an object holding the name of
				 * an array (if part2 is non-NULL) or the
				 * name of a variable. */
    CONST char *part2;		/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    CONST long incrAmount;	/* Amount to be added to variable. */
    CONST int flags;            /* Various flags that tell how to incr value:
				 * any of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_APPEND_VALUE,
				 * TCL_LIST_ELEMENT, TCL_LEAVE_ERR_MSG. */
{
    register Tcl_Obj *varValuePtr;
    int createdNewObj;		/* Set 1 if var's value object is shared
				 * so we must increment a copy (i.e. copy
				 * on write). */
    long i;

    varValuePtr = TclPtrGetVar(interp, varPtr, arrayPtr, part1, part2, flags);

    if (varValuePtr == NULL) {
	Tcl_AddObjErrorInfo(interp,
		"\n    (reading value of variable to increment)", -1);
	return NULL;
    }

    /*
     * Increment the variable's value. If the object is unshared we can
     * modify it directly, otherwise we must create a new copy to modify:
     * this is "copy on write". Then free the variable's old string
     * representation, if any, since it will no longer be valid.
     */

    createdNewObj = 0;
    if (Tcl_IsShared(varValuePtr)) {
	varValuePtr = Tcl_DuplicateObj(varValuePtr);
	createdNewObj = 1;
    }
    if (varValuePtr->typePtr == &tclWideIntType) {
	Tcl_WideInt wide;
	TclGetWide(wide,varValuePtr);
	Tcl_SetWideIntObj(varValuePtr, wide + Tcl_LongAsWide(incrAmount));
    } else if (varValuePtr->typePtr == &tclIntType) {
	i = varValuePtr->internalRep.longValue;
	Tcl_SetIntObj(varValuePtr, i + incrAmount);
    } else {
	/*
	 * Not an integer or wide internal-rep...
	 */
	Tcl_WideInt wide;
	if (Tcl_GetWideIntFromObj(interp, varValuePtr, &wide) != TCL_OK) {
	    if (createdNewObj) {
		Tcl_DecrRefCount(varValuePtr); /* free unneeded copy */
	    }
	    return NULL;
	}
	if (wide <= Tcl_LongAsWide(LONG_MAX)
		&& wide >= Tcl_LongAsWide(LONG_MIN)) {
	    Tcl_SetLongObj(varValuePtr, Tcl_WideAsLong(wide) + incrAmount);
	} else {
	    Tcl_SetWideIntObj(varValuePtr, wide + Tcl_LongAsWide(incrAmount));
	}
    }

    /*
     * Store the variable's new value and run any write traces.
     */
    
    return TclPtrSetVar(interp, varPtr, arrayPtr, part1, part2,
	    varValuePtr, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnsetVar --
 *
 *	Delete a variable, so that it may not be accessed anymore.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR
 *	if the variable can't be unset.  In the event of an error,
 *	if the TCL_LEAVE_ERR_MSG flag is set then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	If varName is defined as a local or global variable in interp,
 *	it is deleted.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UnsetVar(interp, varName, flags)
    Tcl_Interp *interp;		/* Command interpreter in which varName is
				 * to be looked up. */
    CONST char *varName;	/* Name of a variable in interp.  May be
				 * either a scalar name or an array name
				 * or an element in an array. */
    int flags;			/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY or
				 * TCL_LEAVE_ERR_MSG. */
{
    return Tcl_UnsetVar2(interp, varName, (char *) NULL, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnsetVar2 --
 *
 *	Delete a variable, given a 2-part name.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR
 *	if the variable can't be unset.  In the event of an error,
 *	if the TCL_LEAVE_ERR_MSG flag is set then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	If part1 and part2 indicate a local or global variable in interp,
 *	it is deleted.  If part1 is an array name and part2 is NULL, then
 *	the whole array is deleted.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UnsetVar2(interp, part1, part2, flags)
    Tcl_Interp *interp;		/* Command interpreter in which varName is
				 * to be looked up. */
    CONST char *part1;		/* Name of variable or array. */
    CONST char *part2;		/* Name of element within array or NULL. */
    int flags;			/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_LEAVE_ERR_MSG. */
{
    int result;
    Tcl_Obj *part1Ptr;

    part1Ptr = Tcl_NewStringObj(part1, -1);
    Tcl_IncrRefCount(part1Ptr);
    result = TclObjUnsetVar2(interp, part1Ptr, part2, flags);
    TclDecrRefCount(part1Ptr);

    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * TclObjUnsetVar2 --
 *
 *	Delete a variable, given a 2-object name.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR
 *	if the variable can't be unset.  In the event of an error,
 *	if the TCL_LEAVE_ERR_MSG flag is set then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	If part1ptr and part2Ptr indicate a local or global variable in interp,
 *	it is deleted.  If part1Ptr is an array name and part2Ptr is NULL, then
 *	the whole array is deleted.
 *
 *----------------------------------------------------------------------
 */

int
TclObjUnsetVar2(interp, part1Ptr, part2, flags)
    Tcl_Interp *interp;		/* Command interpreter in which varName is
				 * to be looked up. */
    Tcl_Obj *part1Ptr;		/* Name of variable or array. */
    CONST char *part2;		/* Name of element within array or NULL. */
    int flags;			/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_LEAVE_ERR_MSG. */
{
    Var dummyVar;
    Var *varPtr, *dummyVarPtr;
    Interp *iPtr = (Interp *) interp;
    Var *arrayPtr;
    ActiveVarTrace *activePtr;
    Tcl_Obj *objPtr;
    int result;
    char *part1;

    part1 = TclGetString(part1Ptr);
    varPtr = TclObjLookupVar(interp, part1Ptr, part2, flags, "unset",
	    /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL) {
	return TCL_ERROR;
    }
 
    result = (TclIsVarUndefined(varPtr)? TCL_ERROR : TCL_OK);

    if ((arrayPtr != NULL) && (arrayPtr->searchPtr != NULL)) {
	DeleteSearches(arrayPtr);
    }

    /*
     * The code below is tricky, because of the possibility that
     * a trace procedure might try to access a variable being
     * deleted. To handle this situation gracefully, do things
     * in three steps:
     * 1. Copy the contents of the variable to a dummy variable
     *    structure, and mark the original Var structure as undefined.
     * 2. Invoke traces and clean up the variable, using the dummy copy.
     * 3. If at the end of this the original variable is still
     *    undefined and has no outstanding references, then delete
     *	  it (but it could have gotten recreated by a trace).
     */

    dummyVar = *varPtr;
    TclSetVarUndefined(varPtr);
    TclSetVarScalar(varPtr);
    varPtr->value.objPtr = NULL; /* dummyVar points to any value object */
    varPtr->tracePtr = NULL;
    varPtr->searchPtr = NULL;

    /*
     * Call trace procedures for the variable being deleted. Then delete
     * its traces. Be sure to abort any other traces for the variable
     * that are still pending. Special tricks:
     * 1. We need to increment varPtr's refCount around this: CallVarTraces
     *    will use dummyVar so it won't increment varPtr's refCount itself.
     * 2. Turn off the VAR_TRACE_ACTIVE flag in dummyVar: we want to
     *    call unset traces even if other traces are pending.
     */

    if ((dummyVar.tracePtr != NULL)
	    || ((arrayPtr != NULL) && (arrayPtr->tracePtr != NULL))) {
	varPtr->refCount++;
	dummyVar.flags &= ~VAR_TRACE_ACTIVE;
	CallVarTraces(iPtr, arrayPtr, &dummyVar, part1, part2,
		(flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY))
		| TCL_TRACE_UNSETS, /* leaveErrMsg */ 0);
	while (dummyVar.tracePtr != NULL) {
	    VarTrace *tracePtr = dummyVar.tracePtr;
	    dummyVar.tracePtr = tracePtr->nextPtr;
	    Tcl_EventuallyFree((ClientData) tracePtr, TCL_DYNAMIC);
	}
	for (activePtr = iPtr->activeVarTracePtr;  activePtr != NULL;
	     activePtr = activePtr->nextPtr) {
	    if (activePtr->varPtr == varPtr) {
		activePtr->nextTracePtr = NULL;
	    }
	}
	varPtr->refCount--;
    }

    /*
     * If the variable is an array, delete all of its elements. This must be
     * done after calling the traces on the array, above (that's the way
     * traces are defined). If it is a scalar, "discard" its object
     * (decrement the ref count of its object, if any).
     */

    dummyVarPtr = &dummyVar;
    if (TclIsVarArray(dummyVarPtr) && !TclIsVarUndefined(dummyVarPtr)) {
	/*
	 * Deleting the elements of the array may cause traces to be fired
	 * on those elements.  Before deleting them, bump the reference count
	 * of the array, so that if those trace procs make a global or upvar
	 * link to the array, the array is not deleted when the call stack
	 * gets popped (we will delete the array ourselves later in this
	 * function).
	 *
	 * Bumping the count can lead to the odd situation that elements of the
	 * array are being deleted when the array still exists, but since the
	 * array is about to be removed anyway, that shouldn't really matter.
	 */
	varPtr->refCount++;
	DeleteArray(iPtr, part1, dummyVarPtr,
		(flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY)) 
		| TCL_TRACE_UNSETS);
	/* Decr ref count */
	varPtr->refCount--;
    }
    if (TclIsVarScalar(dummyVarPtr)
	    && (dummyVarPtr->value.objPtr != NULL)) {
	objPtr = dummyVarPtr->value.objPtr;
	TclDecrRefCount(objPtr);
	dummyVarPtr->value.objPtr = NULL;
    }

    /*
     * If the variable was a namespace variable, decrement its reference count.
     */
    
    if (varPtr->flags & VAR_NAMESPACE_VAR) {
	varPtr->flags &= ~VAR_NAMESPACE_VAR;
	varPtr->refCount--;
    }

    /*
     * It's an error to unset an undefined variable.
     */
	
    if (result != TCL_OK) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    VarErrMsg(interp, part1, part2, "unset", 
		    ((arrayPtr == NULL) ? noSuchVar : noSuchElement));
	}
    }

    /*
     * Finally, if the variable is truly not in use then free up its Var
     * structure and remove it from its hash table, if any. The ref count of
     * its value object, if any, was decremented above.
     */

    CleanupVar(varPtr, arrayPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceVar --
 *
 *	Arrange for reads and/or writes to a variable to cause a
 *	procedure to be invoked, which can monitor the operations
 *	and/or change their actions.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A trace is set up on the variable given by varName, such that
 *	future references to the variable will be intermediated by
 *	proc.  See the manual entry for complete details on the calling
 *	sequence for proc.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TraceVar(interp, varName, flags, proc, clientData)
    Tcl_Interp *interp;		/* Interpreter in which variable is
				 * to be traced. */
    CONST char *varName;	/* Name of variable;  may end with "(index)"
				 * to signify an array reference. */
    int flags;			/* OR-ed collection of bits, including any
				 * of TCL_TRACE_READS, TCL_TRACE_WRITES,
				 * TCL_TRACE_UNSETS, TCL_GLOBAL_ONLY, and
				 * TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc;	/* Procedure to call when specified ops are
				 * invoked upon varName. */
    ClientData clientData;	/* Arbitrary argument to pass to proc. */
{
    return Tcl_TraceVar2(interp, varName, (char *) NULL, 
	    flags, proc, clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceVar2 --
 *
 *	Arrange for reads and/or writes to a variable to cause a
 *	procedure to be invoked, which can monitor the operations
 *	and/or change their actions.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A trace is set up on the variable given by part1 and part2, such
 *	that future references to the variable will be intermediated by
 *	proc.  See the manual entry for complete details on the calling
 *	sequence for proc.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TraceVar2(interp, part1, part2, flags, proc, clientData)
    Tcl_Interp *interp;		/* Interpreter in which variable is
				 * to be traced. */
    CONST char *part1;		/* Name of scalar variable or array. */
    CONST char *part2;		/* Name of element within array;  NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    int flags;			/* OR-ed collection of bits, including any
				 * of TCL_TRACE_READS, TCL_TRACE_WRITES,
				 * TCL_TRACE_UNSETS, TCL_GLOBAL_ONLY,
				 * and TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc;	/* Procedure to call when specified ops are
				 * invoked upon varName. */
    ClientData clientData;	/* Arbitrary argument to pass to proc. */
{
    Var *varPtr, *arrayPtr;
    register VarTrace *tracePtr;
    int flagMask;
    
    /* 
     * We strip 'flags' down to just the parts which are relevant to
     * TclLookupVar, to avoid conflicts between trace flags and
     * internal namespace flags such as 'FIND_ONLY_NS'.  This can
     * now occur since we have trace flags with values 0x1000 and higher.
     */
    flagMask = TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY;
    varPtr = TclLookupVar(interp, part1, part2,
	    (flags & flagMask) | TCL_LEAVE_ERR_MSG,
	    "trace", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Check for a nonsense flag combination.  Note that this is a
     * panic() because there should be no code path that ever sets
     * both flags.
     */
    if ((flags&TCL_TRACE_RESULT_DYNAMIC) && (flags&TCL_TRACE_RESULT_OBJECT)) {
	panic("bad result flag combination");
    }

    /*
     * Set up trace information.
     */

    flagMask = TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS | 
	TCL_TRACE_ARRAY | TCL_TRACE_RESULT_DYNAMIC | TCL_TRACE_RESULT_OBJECT;
#ifndef TCL_REMOVE_OBSOLETE_TRACES
    flagMask |= TCL_TRACE_OLD_STYLE;
#endif
    tracePtr = (VarTrace *) ckalloc(sizeof(VarTrace));
    tracePtr->traceProc		= proc;
    tracePtr->clientData	= clientData;
    tracePtr->flags		= flags & flagMask;
    tracePtr->nextPtr		= varPtr->tracePtr;
    varPtr->tracePtr		= tracePtr;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UntraceVar --
 *
 *	Remove a previously-created trace for a variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there exists a trace for the variable given by varName
 *	with the given flags, proc, and clientData, then that trace
 *	is removed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_UntraceVar(interp, varName, flags, proc, clientData)
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    CONST char *varName;	/* Name of variable; may end with "(index)"
				 * to signify an array reference. */
    int flags;			/* OR-ed collection of bits describing
				 * current trace, including any of
				 * TCL_TRACE_READS, TCL_TRACE_WRITES,
				 * TCL_TRACE_UNSETS, TCL_GLOBAL_ONLY
				 * and TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc;	/* Procedure assocated with trace. */
    ClientData clientData;	/* Arbitrary argument to pass to proc. */
{
    Tcl_UntraceVar2(interp, varName, (char *) NULL, flags, proc, clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UntraceVar2 --
 *
 *	Remove a previously-created trace for a variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there exists a trace for the variable given by part1
 *	and part2 with the given flags, proc, and clientData, then
 *	that trace is removed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_UntraceVar2(interp, part1, part2, flags, proc, clientData)
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    CONST char *part1;		/* Name of variable or array. */
    CONST char *part2;		/* Name of element within array;  NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    int flags;			/* OR-ed collection of bits describing
				 * current trace, including any of
				 * TCL_TRACE_READS, TCL_TRACE_WRITES,
				 * TCL_TRACE_UNSETS, TCL_GLOBAL_ONLY,
				 * and TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc;	/* Procedure assocated with trace. */
    ClientData clientData;	/* Arbitrary argument to pass to proc. */
{
    register VarTrace *tracePtr;
    VarTrace *prevPtr;
    Var *varPtr, *arrayPtr;
    Interp *iPtr = (Interp *) interp;
    ActiveVarTrace *activePtr;
    int flagMask;
    
    /*
     * Set up a mask to mask out the parts of the flags that we are not
     * interested in now.
     */
    flagMask = TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY;
    varPtr = TclLookupVar(interp, part1, part2, flags & flagMask,
	    /*msg*/ (char *) NULL,
	    /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL) {
	return;
    }


    /*
     * Set up a mask to mask out the parts of the flags that we are not
     * interested in now.
     */
    flagMask = TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS |
	TCL_TRACE_ARRAY | TCL_TRACE_RESULT_DYNAMIC | TCL_TRACE_RESULT_OBJECT; 
#ifndef TCL_REMOVE_OBSOLETE_TRACES
    flagMask |= TCL_TRACE_OLD_STYLE;
#endif
    flags &= flagMask;
    for (tracePtr = varPtr->tracePtr, prevPtr = NULL;  ;
	 prevPtr = tracePtr, tracePtr = tracePtr->nextPtr) {
	if (tracePtr == NULL) {
	    return;
	}
	if ((tracePtr->traceProc == proc) && (tracePtr->flags == flags)
		&& (tracePtr->clientData == clientData)) {
	    break;
	}
    }

    /*
     * The code below makes it possible to delete traces while traces
     * are active: it makes sure that the deleted trace won't be
     * processed by CallVarTraces.
     */

    for (activePtr = iPtr->activeVarTracePtr;  activePtr != NULL;
	 activePtr = activePtr->nextPtr) {
	if (activePtr->nextTracePtr == tracePtr) {
	    activePtr->nextTracePtr = tracePtr->nextPtr;
	}
    }
    if (prevPtr == NULL) {
	varPtr->tracePtr = tracePtr->nextPtr;
    } else {
	prevPtr->nextPtr = tracePtr->nextPtr;
    }
    Tcl_EventuallyFree((ClientData) tracePtr, TCL_DYNAMIC);

    /*
     * If this is the last trace on the variable, and the variable is
     * unset and unused, then free up the variable.
     */

    if (TclIsVarUndefined(varPtr)) {
	CleanupVar(varPtr, (Var *) NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VarTraceInfo --
 *
 *	Return the clientData value associated with a trace on a
 *	variable.  This procedure can also be used to step through
 *	all of the traces on a particular variable that have the
 *	same trace procedure.
 *
 * Results:
 *	The return value is the clientData value associated with
 *	a trace on the given variable.  Information will only be
 *	returned for a trace with proc as trace procedure.  If
 *	the clientData argument is NULL then the first such trace is
 *	returned;  otherwise, the next relevant one after the one
 *	given by clientData will be returned.  If the variable
 *	doesn't exist, or if there are no (more) traces for it,
 *	then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_VarTraceInfo(interp, varName, flags, proc, prevClientData)
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    CONST char *varName;	/* Name of variable;  may end with "(index)"
				 * to signify an array reference. */
    int flags;			/* OR-ed combo or TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY (can be 0). */
    Tcl_VarTraceProc *proc;	/* Procedure assocated with trace. */
    ClientData prevClientData;	/* If non-NULL, gives last value returned
				 * by this procedure, so this call will
				 * return the next trace after that one.
				 * If NULL, this call will return the
				 * first trace. */
{
    return Tcl_VarTraceInfo2(interp, varName, (char *) NULL,
	    flags, proc, prevClientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VarTraceInfo2 --
 *
 *	Same as Tcl_VarTraceInfo, except takes name in two pieces
 *	instead of one.
 *
 * Results:
 *	Same as Tcl_VarTraceInfo.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_VarTraceInfo2(interp, part1, part2, flags, proc, prevClientData)
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    CONST char *part1;		/* Name of variable or array. */
    CONST char *part2;		/* Name of element within array;  NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    int flags;			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc;	/* Procedure assocated with trace. */
    ClientData prevClientData;	/* If non-NULL, gives last value returned
				 * by this procedure, so this call will
				 * return the next trace after that one.
				 * If NULL, this call will return the
				 * first trace. */
{
    register VarTrace *tracePtr;
    Var *varPtr, *arrayPtr;

    varPtr = TclLookupVar(interp, part1, part2,
	    flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY),
	    /*msg*/ (char *) NULL,
	    /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    /*
     * Find the relevant trace, if any, and return its clientData.
     */

    tracePtr = varPtr->tracePtr;
    if (prevClientData != NULL) {
	for ( ;  tracePtr != NULL;  tracePtr = tracePtr->nextPtr) {
	    if ((tracePtr->clientData == prevClientData)
		    && (tracePtr->traceProc == proc)) {
		tracePtr = tracePtr->nextPtr;
		break;
	    }
	}
    }
    for ( ;  tracePtr != NULL;  tracePtr = tracePtr->nextPtr) {
	if (tracePtr->traceProc == proc) {
	    return tracePtr->clientData;
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnsetObjCmd --
 *
 *	This object-based procedure is invoked to process the "unset" Tcl
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
Tcl_UnsetObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register int i, flags = TCL_LEAVE_ERR_MSG;
    register char *name;

    if (objc < 1) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-nocomplain? ?--? ?varName varName ...?");
	return TCL_ERROR;
    } else if (objc == 1) {
	/*
	 * Do nothing if no arguments supplied, so as to match
	 * command documentation.
	 */
	return TCL_OK;
    }

    /*
     * Simple, restrictive argument parsing.  The only options are --
     * and -nocomplain (which must come first and be given exactly to
     * be an option).
     */
    i = 1;
    name = TclGetString(objv[i]);
    if (name[0] == '-') {
 	if (strcmp("-nocomplain", name) == 0) {
	    i++;
 	    if (i == objc) {
		return TCL_OK;
	    }
 	    flags = 0;
 	    name = TclGetString(objv[i]);
 	}
 	if (strcmp("--", name) == 0) {
 	    i++;
 	}
    }

    for (; i < objc;  i++) {
	if ((TclObjUnsetVar2(interp, objv[i], NULL, flags) != TCL_OK)
		&& (flags == TCL_LEAVE_ERR_MSG)) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendObjCmd --
 *
 *	This object-based procedure is invoked to process the "append" 
 *	Tcl command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	A variable's value may be changed.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_AppendObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Var *varPtr, *arrayPtr;
    char *part1;

    register Tcl_Obj *varValuePtr = NULL;
    					/* Initialized to avoid compiler
				         * warning. */
    int i;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?value value ...?");
	return TCL_ERROR;
    }

    if (objc == 2) {
	varValuePtr = Tcl_ObjGetVar2(interp, objv[1], NULL, TCL_LEAVE_ERR_MSG);
	if (varValuePtr == NULL) {
	    return TCL_ERROR;
	}
    } else {
	varPtr = TclObjLookupVar(interp, objv[1], NULL, TCL_LEAVE_ERR_MSG,
		"set", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
	part1 = TclGetString(objv[1]);
	if (varPtr == NULL) {
	    return TCL_ERROR;
	}
	for (i = 2;  i < objc;  i++) {	  
	    /*
	     * Note that we do not need to increase the refCount of
	     * the Var pointers: should a trace delete the variable,
	     * the return value of TclPtrSetVar will be NULL, and we 
	     * will not access the variable again.
	     */

	    varValuePtr = TclPtrSetVar(interp, varPtr, arrayPtr, part1, NULL, 
	            objv[i], (TCL_APPEND_VALUE | TCL_LEAVE_ERR_MSG));
	    if (varValuePtr == NULL) {
		return TCL_ERROR;
	    }
	}
    }
    Tcl_SetObjResult(interp, varValuePtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LappendObjCmd --
 *
 *	This object-based procedure is invoked to process the "lappend" 
 *	Tcl command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	A variable's value may be changed.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LappendObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tcl_Obj *varValuePtr, *newValuePtr;
    register List *listRepPtr;
    register Tcl_Obj **elemPtrs;
    int numElems, numRequired, createdNewObj, createVar, i, j;
    Var *varPtr, *arrayPtr;
    char *part1;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?value value ...?");
	return TCL_ERROR;
    }
    if (objc == 2) {
	newValuePtr = Tcl_ObjGetVar2(interp, objv[1], (Tcl_Obj *) NULL, 0);
	if (newValuePtr == NULL) {
	    /*
	     * The variable doesn't exist yet. Just create it with an empty
	     * initial value.
	     */
	    
	    varValuePtr = Tcl_NewObj();
	    newValuePtr = Tcl_ObjSetVar2(interp, objv[1], NULL, varValuePtr,
		    TCL_LEAVE_ERR_MSG);
	    if (newValuePtr == NULL) {
		Tcl_DecrRefCount(varValuePtr); /* free unneeded object */
		return TCL_ERROR;
	    }
	}
    } else {
	/*
	 * We have arguments to append. We used to call Tcl_SetVar2 to
	 * append each argument one at a time to ensure that traces were run
	 * for each append step. We now append the arguments all at once
	 * because it's faster. Note that a read trace and a write trace for
	 * the variable will now each only be called once. Also, if the
	 * variable's old value is unshared we modify it directly, otherwise
	 * we create a new copy to modify: this is "copy on write".
	 */

	createdNewObj = 0;
	createVar = 1;

	/*
	 * Use the TCL_TRACE_READS flag to ensure that if we have an
	 * array with no elements set yet, but with a read trace on it,
	 * we will create the variable and get read traces triggered.
	 * Note that you have to protect the variable pointers around
	 * the TclPtrGetVar call to insure that they remain valid 
	 * even if the variable was undefined and unused.
	 */

	varPtr = TclObjLookupVar(interp, objv[1], NULL, TCL_LEAVE_ERR_MSG,
		"set", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
	if (varPtr == NULL) {
	    return TCL_ERROR;
	}
	varPtr->refCount++;
	if (arrayPtr != NULL) {
	    arrayPtr->refCount++;
	}
	part1 = TclGetString(objv[1]);
	varValuePtr = TclPtrGetVar(interp, varPtr, arrayPtr, part1, NULL, 
	        (TCL_TRACE_READS | TCL_LEAVE_ERR_MSG));
	varPtr->refCount--;
	if (arrayPtr != NULL) {
	    arrayPtr->refCount--;
	}

	if (varValuePtr == NULL) {
	    /*
	     * We couldn't read the old value: either the var doesn't yet
	     * exist or it's an array element.  If it's new, we will try to
	     * create it with Tcl_ObjSetVar2 below.
	     */
	    
	    createVar = (TclIsVarUndefined(varPtr));
	    varValuePtr = Tcl_NewObj();
	    createdNewObj = 1;
	} else if (Tcl_IsShared(varValuePtr)) {	
	    varValuePtr = Tcl_DuplicateObj(varValuePtr);
	    createdNewObj = 1;
	}

	/*
	 * Convert the variable's old value to a list object if necessary.
	 */

	if (varValuePtr->typePtr != &tclListType) {
	    int result = tclListType.setFromAnyProc(interp, varValuePtr);
	    if (result != TCL_OK) {
		if (createdNewObj) {
		    Tcl_DecrRefCount(varValuePtr); /* free unneeded obj. */
		}
		return result;
	    }
	}
	listRepPtr = (List *) varValuePtr->internalRep.twoPtrValue.ptr1;
	elemPtrs = listRepPtr->elements;
	numElems = listRepPtr->elemCount;

	/*
	 * If there is no room in the current array of element pointers,
	 * allocate a new, larger array and copy the pointers to it.
	 */
	
	numRequired = numElems + (objc-2);
	if (numRequired > listRepPtr->maxElemCount) {
	    int newMax = (2 * numRequired);
	    Tcl_Obj **newElemPtrs = (Tcl_Obj **)
		ckalloc((unsigned) (newMax * sizeof(Tcl_Obj *)));
	    
	    memcpy((VOID *) newElemPtrs, (VOID *) elemPtrs,
		    (size_t) (numElems * sizeof(Tcl_Obj *)));
	    listRepPtr->maxElemCount = newMax;
	    listRepPtr->elements = newElemPtrs;
	    ckfree((char *) elemPtrs);
	    elemPtrs = newElemPtrs;
	}

	/*
	 * Insert the new elements at the end of the list.
	 */

	for (i = 2, j = numElems;  i < objc;  i++, j++) {
            elemPtrs[j] = objv[i];
            Tcl_IncrRefCount(objv[i]);
        }
	listRepPtr->elemCount = numRequired;

	/*
	 * Invalidate and free any old string representation since it no
	 * longer reflects the list's internal representation.
	 */

	Tcl_InvalidateStringRep(varValuePtr);

	/*
	 * Now store the list object back into the variable. If there is an
	 * error setting the new value, decrement its ref count if it
	 * was new and we didn't create the variable.
	 */
	
	newValuePtr = TclPtrSetVar(interp, varPtr, arrayPtr, part1, NULL, 
	            varValuePtr, TCL_LEAVE_ERR_MSG);	
	if (newValuePtr == NULL) {
	    if (createdNewObj && !createVar) {
		Tcl_DecrRefCount(varValuePtr); /* free unneeded obj */
	    }
	    return TCL_ERROR;
	}
    }

    /*
     * Set the interpreter's object result to refer to the variable's value
     * object.
     */

    Tcl_SetObjResult(interp, newValuePtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ArrayObjCmd --
 *
 *	This object-based procedure is invoked to process the "array" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ArrayObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    /*
     * The list of constants below should match the arrayOptions string array
     * below.
     */

    enum {ARRAY_ANYMORE, ARRAY_DONESEARCH,  ARRAY_EXISTS, ARRAY_GET,
	  ARRAY_NAMES, ARRAY_NEXTELEMENT, ARRAY_SET, ARRAY_SIZE,
	  ARRAY_STARTSEARCH, ARRAY_STATISTICS, ARRAY_UNSET}; 
    static CONST char *arrayOptions[] = {
	"anymore", "donesearch", "exists", "get", "names", "nextelement",
	"set", "size", "startsearch", "statistics", "unset", (char *) NULL
    };

    Interp *iPtr = (Interp *) interp;
    Var *varPtr, *arrayPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *resultPtr, *varNamePtr;
    int notArray;
    char *varName;
    int index, result;


    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option arrayName ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], arrayOptions, "option",
	    0, &index) != TCL_OK) {
    	return TCL_ERROR;
    }

    /*
     * Locate the array variable
     */
    
    varNamePtr = objv[2];
    varName = TclGetString(varNamePtr);
    varPtr = TclObjLookupVar(interp, varNamePtr, NULL, /*flags*/ 0,
            /*msg*/ 0, /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);

    /*
     * Special array trace used to keep the env array in sync for
     * array names, array get, etc.
     */

    if (varPtr != NULL && varPtr->tracePtr != NULL
	    && (TclIsVarArray(varPtr) || TclIsVarUndefined(varPtr))) {
	if (TCL_ERROR == CallVarTraces(iPtr, arrayPtr, varPtr, varName, NULL,
		(TCL_LEAVE_ERR_MSG|TCL_NAMESPACE_ONLY|TCL_GLOBAL_ONLY|
		TCL_TRACE_ARRAY), /* leaveErrMsg */ 1)) {
	    return TCL_ERROR;
	}
    }

    /*
     * Verify that it is indeed an array variable. This test comes after
     * the traces - the variable may actually become an array as an effect 
     * of said traces.
     */

    notArray = 0;
    if ((varPtr == NULL) || !TclIsVarArray(varPtr)
	    || TclIsVarUndefined(varPtr)) {
	notArray = 1;
    }

    /*
     * We have to wait to get the resultPtr until here because
     * CallVarTraces can affect the result.
     */

    resultPtr = Tcl_GetObjResult(interp);

    switch (index) {
        case ARRAY_ANYMORE: {
	    ArraySearch *searchPtr;
	    
	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, 
                        "arrayName searchId");
		return TCL_ERROR;
	    }
	    if (notArray) {
	        goto error;
	    }
	    searchPtr = ParseSearchId(interp, varPtr, varName, objv[3]);
	    if (searchPtr == NULL) {
	        return TCL_ERROR;
	    }
	    while (1) {
	        Var *varPtr2;

		if (searchPtr->nextEntry != NULL) {
		    varPtr2 = (Var *) Tcl_GetHashValue(searchPtr->nextEntry);
		    if (!TclIsVarUndefined(varPtr2)) {
		        break;
		    }
		}
		searchPtr->nextEntry = Tcl_NextHashEntry(&searchPtr->search);
		if (searchPtr->nextEntry == NULL) {
		    Tcl_SetIntObj(resultPtr, 0);
		    return TCL_OK;
		}
	    }
	    Tcl_SetIntObj(resultPtr, 1);
	    break;
	}
        case ARRAY_DONESEARCH: {
	    ArraySearch *searchPtr, *prevPtr;

	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, 
                        "arrayName searchId");
		return TCL_ERROR;
	    }
	    if (notArray) {
	        goto error;
	    }
	    searchPtr = ParseSearchId(interp, varPtr, varName, objv[3]);
	    if (searchPtr == NULL) {
	        return TCL_ERROR;
	    }
	    if (varPtr->searchPtr == searchPtr) {
	        varPtr->searchPtr = searchPtr->nextPtr;
	    } else {
	        for (prevPtr = varPtr->searchPtr;  ;
		     prevPtr = prevPtr->nextPtr) {
		    if (prevPtr->nextPtr == searchPtr) {
		        prevPtr->nextPtr = searchPtr->nextPtr;
			break;
		    }
		}
	    }
	    ckfree((char *) searchPtr);
	    break;
	}
        case ARRAY_EXISTS: {
	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "arrayName");
	        return TCL_ERROR;
	    }
	    Tcl_SetIntObj(resultPtr, !notArray);
	    break;
	}
        case ARRAY_GET: {
	    Tcl_HashSearch search;
	    Var *varPtr2;
	    char *pattern = NULL;
	    char *name;
	    Tcl_Obj *namePtr, *valuePtr, *nameLstPtr, *tmpResPtr, **namePtrPtr;
	    int i, count;
	    
	    if ((objc != 3) && (objc != 4)) {
	        Tcl_WrongNumArgs(interp, 2, objv, "arrayName ?pattern?");
		return TCL_ERROR;
	    }
	    if (notArray) {
	        return TCL_OK;
	    }
	    if (objc == 4) {
	        pattern = TclGetString(objv[3]);
	    }

	    /*
	     * Store the array names in a new object.
	     */

	    nameLstPtr = Tcl_NewObj();
	    Tcl_IncrRefCount(nameLstPtr);

	    for (hPtr = Tcl_FirstHashEntry(varPtr->value.tablePtr, &search);
		 hPtr != NULL;  hPtr = Tcl_NextHashEntry(&search)) {
	        varPtr2 = (Var *) Tcl_GetHashValue(hPtr);
		if (TclIsVarUndefined(varPtr2)) {
		    continue;
		}
		name = Tcl_GetHashKey(varPtr->value.tablePtr, hPtr);
		if ((objc == 4) && !Tcl_StringMatch(name, pattern)) {
		    continue;	/* element name doesn't match pattern */
		}
		
		namePtr = Tcl_NewStringObj(name, -1);
		result = Tcl_ListObjAppendElement(interp, nameLstPtr,
		        namePtr);
		if (result != TCL_OK) {
		    Tcl_DecrRefCount(namePtr); /* free unneeded name obj */
		    Tcl_DecrRefCount(nameLstPtr);
		    return result;
		}
	    }

	    /*
	     * Make sure the Var structure of the array is not removed by
	     * a trace while we're working.
	     */

	    varPtr->refCount++;

	    /*
	     * Get the array values corresponding to each element name 
	     */

	    tmpResPtr = Tcl_NewObj();
	    result = Tcl_ListObjGetElements(interp, nameLstPtr,
		    &count, &namePtrPtr);
	    if (result != TCL_OK) {
		goto errorInArrayGet;
	    }
	    
	    for (i = 0; i < count; i++) { 
		namePtr = *namePtrPtr++;
		valuePtr = Tcl_ObjGetVar2(interp, objv[2], namePtr,
	                TCL_LEAVE_ERR_MSG);
		if (valuePtr == NULL) {
		    /*
		     * Some trace played a trick on us; we need to diagnose to
		     * adapt our behaviour: was the array element unset, or did
		     * the modification modify the complete array?
		     */

		    if (TclIsVarArray(varPtr) && !TclIsVarUndefined(varPtr)) {
			/*
			 * The array itself looks OK, the variable was
			 * undefined: forget it.
			 */
			
			continue;
		    } else {
			result = TCL_ERROR;
			goto errorInArrayGet;
		    }
		}
		result = Tcl_ListObjAppendElement(interp, tmpResPtr, namePtr);
		if (result != TCL_OK) {
		    goto errorInArrayGet;
		}
		result = Tcl_ListObjAppendElement(interp, tmpResPtr, valuePtr);
		if (result != TCL_OK) {
		    goto errorInArrayGet;
		}
	    }
	    varPtr->refCount--;
	    Tcl_SetObjResult(interp, tmpResPtr);
	    Tcl_DecrRefCount(nameLstPtr);
	    break;

	    errorInArrayGet:
	    varPtr->refCount--;
	    Tcl_DecrRefCount(nameLstPtr);
	    Tcl_DecrRefCount(tmpResPtr); /* free unneeded temp result obj */
	    return result;
	}
        case ARRAY_NAMES: {
	    Tcl_HashSearch search;
	    Var *varPtr2;
	    char *pattern = NULL;
	    char *name;
	    Tcl_Obj *namePtr;
	    int mode, matched = 0;
	    static CONST char *options[] = {
		"-exact", "-glob", "-regexp", (char *) NULL
	    };
	    enum options { OPT_EXACT, OPT_GLOB, OPT_REGEXP };

	    mode = OPT_GLOB;
	    
	    if ((objc < 3) || (objc > 5)) {
  	        Tcl_WrongNumArgs(interp, 2, objv,
			"arrayName ?mode? ?pattern?");
		return TCL_ERROR;
	    }
	    if (notArray) {
	        return TCL_OK;
	    }
	    if (objc == 4) {
	        pattern = Tcl_GetString(objv[3]);
	    } else if (objc == 5) {
		pattern = Tcl_GetString(objv[4]);
		if (Tcl_GetIndexFromObj(interp, objv[3], options, "option",
			0, &mode) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }       		
	    for (hPtr = Tcl_FirstHashEntry(varPtr->value.tablePtr, &search);
		 hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	        varPtr2 = (Var *) Tcl_GetHashValue(hPtr);
		if (TclIsVarUndefined(varPtr2)) {
		    continue;
		}
		name = Tcl_GetHashKey(varPtr->value.tablePtr, hPtr);
		if (objc > 3) {
		    switch ((enum options) mode) {
			case OPT_EXACT:
			    matched = (strcmp(name, pattern) == 0);
			    break;
			case OPT_GLOB:
			    matched = Tcl_StringMatch(name, pattern);
			    break;
			case OPT_REGEXP:
			    matched = Tcl_RegExpMatch(interp, name,
				    pattern);
			    if (matched < 0) {
				return TCL_ERROR;
			    }
			    break;
		    }
		    if (matched == 0) {
			continue;
		    }
		}
		
		namePtr = Tcl_NewStringObj(name, -1);
		result = Tcl_ListObjAppendElement(interp, resultPtr, namePtr);
		if (result != TCL_OK) {
		    Tcl_DecrRefCount(namePtr); /* free unneeded name obj */
		    return result;
		}
	    }
	    break;
	}
        case ARRAY_NEXTELEMENT: {
	    ArraySearch *searchPtr;
	    Tcl_HashEntry *hPtr;
	    
	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, 
                        "arrayName searchId");
		return TCL_ERROR;
	    }
	    if (notArray) {
  	        goto error;
	    }
	    searchPtr = ParseSearchId(interp, varPtr, varName, objv[3]);
	    if (searchPtr == NULL) {
	        return TCL_ERROR;
	    }
	    while (1) {
	        Var *varPtr2;

		hPtr = searchPtr->nextEntry;
		if (hPtr == NULL) {
		    hPtr = Tcl_NextHashEntry(&searchPtr->search);
		    if (hPtr == NULL) {
		        return TCL_OK;
		    }
		} else {
		    searchPtr->nextEntry = NULL;
		}
		varPtr2 = (Var *) Tcl_GetHashValue(hPtr);
		if (!TclIsVarUndefined(varPtr2)) {
		    break;
		}
	    }
	    Tcl_SetStringObj(resultPtr,
	            Tcl_GetHashKey(varPtr->value.tablePtr, hPtr), -1);
	    break;
	}
        case ARRAY_SET: {
	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, "arrayName list");
		return TCL_ERROR;
	    }
	    return(TclArraySet(interp, objv[2], objv[3]));
	}
        case ARRAY_SIZE: {
	    Tcl_HashSearch search;
	    Var *varPtr2;
	    int size;

	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "arrayName");
		return TCL_ERROR;
	    }
	    size = 0;
	    if (!notArray) {
	        for (hPtr = Tcl_FirstHashEntry(varPtr->value.tablePtr, 
                        &search);
		     hPtr != NULL;  hPtr = Tcl_NextHashEntry(&search)) {
		    varPtr2 = (Var *) Tcl_GetHashValue(hPtr);
		    if (TclIsVarUndefined(varPtr2)) {
		        continue;
		    }
		    size++;
		}
	    }
	    Tcl_SetIntObj(resultPtr, size);
	    break;
	}
        case ARRAY_STARTSEARCH: {
	    ArraySearch *searchPtr;

	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "arrayName");
		return TCL_ERROR;
	    }
	    if (notArray) {
	        goto error;
	    }
	    searchPtr = (ArraySearch *) ckalloc(sizeof(ArraySearch));
	    if (varPtr->searchPtr == NULL) {
	        searchPtr->id = 1;
		Tcl_AppendStringsToObj(resultPtr, "s-1-", varName,
		        (char *) NULL);
	    } else {
	        char string[TCL_INTEGER_SPACE];

		searchPtr->id = varPtr->searchPtr->id + 1;
		TclFormatInt(string, searchPtr->id);
		Tcl_AppendStringsToObj(resultPtr, "s-", string, "-", varName,
			(char *) NULL);
	    }
	    searchPtr->varPtr = varPtr;
	    searchPtr->nextEntry = Tcl_FirstHashEntry(varPtr->value.tablePtr,
		    &searchPtr->search);
	    searchPtr->nextPtr = varPtr->searchPtr;
	    varPtr->searchPtr = searchPtr;
	    break;
	}

	case ARRAY_STATISTICS: {
	    CONST char *stats;

	    if (notArray) {
		goto error;
	    }

	    stats = Tcl_HashStats(varPtr->value.tablePtr);
	    if (stats != NULL) {
		Tcl_SetStringObj(Tcl_GetObjResult(interp), stats, -1);
		ckfree((void *)stats);
	    } else {
		Tcl_SetResult(interp, "error reading array statistics",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	    break;
        }
	
	case ARRAY_UNSET: {
	    Tcl_HashSearch search;
	    Var *varPtr2;
	    char *pattern = NULL;
	    char *name;
          
	    if ((objc != 3) && (objc != 4)) {
		Tcl_WrongNumArgs(interp, 2, objv, "arrayName ?pattern?");
		return TCL_ERROR;
	    }
	    if (notArray) {
		return TCL_OK;
	    }
	    if (objc == 3) {
		/*
		 * When no pattern is given, just unset the whole array
		 */
		if (TclObjUnsetVar2(interp, varNamePtr, NULL, 0)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
	    } else {
		pattern = Tcl_GetString(objv[3]);
		for (hPtr = Tcl_FirstHashEntry(varPtr->value.tablePtr,
			&search);
		     hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
		    varPtr2 = (Var *) Tcl_GetHashValue(hPtr);
		    if (TclIsVarUndefined(varPtr2)) {
			continue;
		    }
		    name = Tcl_GetHashKey(varPtr->value.tablePtr, hPtr);
		    if (Tcl_StringMatch(name, pattern) &&
			    (TclObjUnsetVar2(interp, varNamePtr, name, 0)
				    != TCL_OK)) {
			return TCL_ERROR;
		    }
		}
	    }
	    break;
	}
    }
    return TCL_OK;

    error:
    Tcl_AppendStringsToObj(resultPtr, "\"", varName, "\" isn't an array",
	    (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclArraySet --
 *
 *	Set the elements of an array.  If there are no elements to
 *	set, create an empty array.  This routine is used by the
 *	Tcl_ArrayObjCmd and by the TclSetupEnv routine.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	A variable will be created if one does not already exist.
 *
 *----------------------------------------------------------------------
 */

int
TclArraySet(interp, arrayNameObj, arrayElemObj)
    Tcl_Interp *interp;		/* Current interpreter. */
    Tcl_Obj *arrayNameObj;	/* The array name. */
    Tcl_Obj *arrayElemObj;	/* The array elements list.  If this is
				 * NULL, create an empty array. */
{
    Var *varPtr, *arrayPtr;
    Tcl_Obj **elemPtrs;
    int result, elemLen, i, nameLen;
    char *varName, *p;
    
    varName = Tcl_GetStringFromObj(arrayNameObj, &nameLen);
    p = varName + nameLen - 1;
    if (*p == ')') {
	while (--p >= varName) {
	    if (*p == '(') {
		VarErrMsg(interp, varName, NULL, "set", needArray);
		return TCL_ERROR;
	    }
	}
    }

    varPtr = TclObjLookupVar(interp, arrayNameObj, NULL,
	    /*flags*/ TCL_LEAVE_ERR_MSG, /*msg*/ "set", /*createPart1*/ 1,
	    /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL) {
	return TCL_ERROR;
    }

    if (arrayElemObj != NULL) {
	result = Tcl_ListObjGetElements(interp, arrayElemObj,
		&elemLen, &elemPtrs);
	if (result != TCL_OK) {
	    return result;
	}
	if (elemLen & 1) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		    "list must have an even number of elements", -1);
	    return TCL_ERROR;
	}
	if (elemLen > 0) {
	    /*
	     * We needn't worry about traces invalidating arrayPtr:
	     * should that be the case, TclPtrSetVar will return NULL
	     * so that we break out of the loop and return an error.
	     */

	    for (i = 0;  i < elemLen;  i += 2) {
		char *part2 = TclGetString(elemPtrs[i]);
		Var *elemVarPtr = TclLookupArrayElement(interp, varName, 
                        part2, TCL_LEAVE_ERR_MSG, "set", 1, 1, varPtr);
		if ((elemVarPtr == NULL) ||
		        (TclPtrSetVar(interp, elemVarPtr, varPtr, varName,
			 part2, elemPtrs[i+1], TCL_LEAVE_ERR_MSG) == NULL)) {
		    result = TCL_ERROR;
		    break;
		}
	    }
	    return result;
	}
    }
    
    /*
     * The list is empty make sure we have an array, or create
     * one if necessary.
     */
    
    if (varPtr != NULL) {
	if (!TclIsVarUndefined(varPtr) && TclIsVarArray(varPtr)) {
	    /*
	     * Already an array, done.
	     */
	    
	    return TCL_OK;
	}
	if (TclIsVarArrayElement(varPtr) || !TclIsVarUndefined(varPtr)) {
	    /*
	     * Either an array element, or a scalar: lose!
	     */
	    
	    VarErrMsg(interp, varName, (char *)NULL, "array set", needArray);
	    return TCL_ERROR;
	}
    }
    TclSetVarArray(varPtr);
    TclClearVarUndefined(varPtr);
    varPtr->value.tablePtr =
	(Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(varPtr->value.tablePtr, TCL_STRING_KEYS);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ObjMakeUpvar --
 *
 *	This procedure does all of the work of the "global" and "upvar"
 *	commands.
 *
 * Results:
 *	A standard Tcl completion code. If an error occurs then an
 *	error message is left in iPtr->result.
 *
 * Side effects:
 *	The variable given by myName is linked to the variable in framePtr
 *	given by otherP1 and otherP2, so that references to myName are
 *	redirected to the other variable like a symbolic link.
 *
 *----------------------------------------------------------------------
 */

static int
ObjMakeUpvar(interp, framePtr, otherP1Ptr, otherP2, otherFlags, myName, myFlags, index)
    Tcl_Interp *interp;		/* Interpreter containing variables. Used
			         * for error messages, too. */
    CallFrame *framePtr;	/* Call frame containing "other" variable.
				 * NULL means use global :: context. */
    Tcl_Obj *otherP1Ptr;
    CONST char *otherP2;	/* Two-part name of variable in framePtr. */
    CONST int otherFlags;	/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of "other" variable. */
    CONST char *myName;		/* Name of variable which will refer to
				 * otherP1/otherP2. Must be a scalar. */
    int myFlags;		/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of myName. */
    int index;                  /* If the variable to be linked is an indexed
				 * scalar, this is its index. Otherwise, -1. */
{
    Interp *iPtr = (Interp *) interp;
    Var *otherPtr, *varPtr, *arrayPtr;
    CallFrame *varFramePtr;
    CONST char *errMsg;

    /*
     * Find "other" in "framePtr". If not looking up other in just the
     * current namespace, temporarily replace the current var frame
     * pointer in the interpreter in order to use TclObjLookupVar.
     */

    varFramePtr = iPtr->varFramePtr;
    if (!(otherFlags & TCL_NAMESPACE_ONLY)) {
	iPtr->varFramePtr = framePtr;
    }
    otherPtr = TclObjLookupVar(interp, otherP1Ptr, otherP2,
	    (otherFlags | TCL_LEAVE_ERR_MSG), "access",
            /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (!(otherFlags & TCL_NAMESPACE_ONLY)) {
	iPtr->varFramePtr = varFramePtr;
    }
    if (otherPtr == NULL) {
	return TCL_ERROR;
    }

    if (index >= 0) {
	if (!varFramePtr->isProcCallFrame) {
	    panic("ObjMakeUpvar called with an index outside from a proc.\n");
	}
	varPtr = &(varFramePtr->compiledLocals[index]);
    } else {
	/*
	 * Check that we are not trying to create a namespace var linked to
	 * a local variable in a procedure. If we allowed this, the local
	 * variable in the shorter-lived procedure frame could go away
	 * leaving the namespace var's reference invalid.
	 */
	
	if (((otherP2 ? arrayPtr->nsPtr : otherPtr->nsPtr) == NULL) 
	    && ((myFlags & (TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY))
		|| (varFramePtr == NULL)
		|| !varFramePtr->isProcCallFrame
		|| (strstr(myName, "::") != NULL))) {
	    Tcl_AppendResult((Tcl_Interp *) iPtr, "bad variable name \"",
		    myName, "\": upvar won't create namespace variable that ",
		    "refers to procedure variable", (char *) NULL);
	    return TCL_ERROR;
	}
	
	/*
	 * Lookup and eventually create the new variable. Set the flag bit
	 * LOOKUP_FOR_UPVAR to indicate the special resolution rules for 
	 * upvar purposes: 
	 *   - Bug #696893 - variable is either proc-local or in the current
	 *     namespace; never follow the second (global) resolution path 
	 *   - Bug #631741 - do not use special namespace or interp resolvers
	 */
	
	varPtr = TclLookupSimpleVar(interp, myName, (myFlags | LOOKUP_FOR_UPVAR), 
	        /* create */ 1, &errMsg, &index);
	if (varPtr == NULL) {
	    VarErrMsg(interp, myName, NULL, "create", errMsg);
	    return TCL_ERROR;
	}
    }

    if (varPtr == otherPtr) {
	Tcl_SetResult((Tcl_Interp *) iPtr,
		      "can't upvar from variable to itself", TCL_STATIC);
	return TCL_ERROR;
    }

    if (varPtr->tracePtr != NULL) {
	Tcl_AppendResult((Tcl_Interp *) iPtr, "variable \"", myName,
	        "\" has traces: can't use for upvar", (char *) NULL);
	return TCL_ERROR;
    } else if (!TclIsVarUndefined(varPtr)) {
	/*
	 * The variable already existed. Make sure this variable "varPtr"
	 * isn't the same as "otherPtr" (avoid circular links). Also, if
	 * it's not an upvar then it's an error. If it is an upvar, then
	 * just disconnect it from the thing it currently refers to.
	 */

	if (TclIsVarLink(varPtr)) {
	    Var *linkPtr = varPtr->value.linkPtr;
	    if (linkPtr == otherPtr) {
		return TCL_OK;
	    }
	    linkPtr->refCount--;
	    if (TclIsVarUndefined(linkPtr)) {
		CleanupVar(linkPtr, (Var *) NULL);
	    }
	} else {
	    Tcl_AppendResult((Tcl_Interp *) iPtr, "variable \"", myName,
		    "\" already exists", (char *) NULL);
	    return TCL_ERROR;
	}
    }
    TclSetVarLink(varPtr);
    TclClearVarUndefined(varPtr);
    varPtr->value.linkPtr = otherPtr;
    otherPtr->refCount++;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpVar --
 *
 *	This procedure links one variable to another, just like
 *	the "upvar" command.
 *
 * Results:
 *	A standard Tcl completion code.  If an error occurs then
 *	an error message is left in the interp's result.
 *
 * Side effects:
 *	The variable in frameName whose name is given by varName becomes
 *	accessible under the name localName, so that references to
 *	localName are redirected to the other variable like a symbolic
 *	link.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UpVar(interp, frameName, varName, localName, flags)
    Tcl_Interp *interp;		/* Command interpreter in which varName is
				 * to be looked up. */
    CONST char *frameName;	/* Name of the frame containing the source
				 * variable, such as "1" or "#0". */
    CONST char *varName;	/* Name of a variable in interp to link to.
				 * May be either a scalar name or an
				 * element in an array. */
    CONST char *localName;	/* Name of link variable. */
    int flags;			/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of localName. */
{
    return Tcl_UpVar2(interp, frameName, varName, NULL, localName, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpVar2 --
 *
 *	This procedure links one variable to another, just like
 *	the "upvar" command.
 *
 * Results:
 *	A standard Tcl completion code.  If an error occurs then
 *	an error message is left in the interp's result.
 *
 * Side effects:
 *	The variable in frameName whose name is given by part1 and
 *	part2 becomes accessible under the name localName, so that
 *	references to localName are redirected to the other variable
 *	like a symbolic link.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UpVar2(interp, frameName, part1, part2, localName, flags)
    Tcl_Interp *interp;		/* Interpreter containing variables.  Used
				 * for error messages too. */
    CONST char *frameName;	/* Name of the frame containing the source
				 * variable, such as "1" or "#0". */
    CONST char *part1;
    CONST char *part2;		/* Two parts of source variable name to
				 * link to. */
    CONST char *localName;	/* Name of link variable. */
    int flags;			/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of localName. */
{
    int result;
    CallFrame *framePtr;
    Tcl_Obj *part1Ptr;

    if (TclGetFrame(interp, frameName, &framePtr) == -1) {
	return TCL_ERROR;
    }

    part1Ptr = Tcl_NewStringObj(part1, -1);
    Tcl_IncrRefCount(part1Ptr);
    result = ObjMakeUpvar(interp, framePtr, part1Ptr, part2, 0,
	    localName, flags, -1);
    TclDecrRefCount(part1Ptr);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVariableFullName --
 *
 *	Given a Tcl_Var token returned by Tcl_FindNamespaceVar, this
 *	procedure appends to an object the namespace variable's full
 *	name, qualified by a sequence of parent namespace names.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The variable's fully-qualified name is appended to the string
 *	representation of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetVariableFullName(interp, variable, objPtr)
    Tcl_Interp *interp;	        /* Interpreter containing the variable. */
    Tcl_Var variable;		/* Token for the variable returned by a
				 * previous call to Tcl_FindNamespaceVar. */
    Tcl_Obj *objPtr;		/* Points to the object onto which the
				 * variable's full name is appended. */
{
    Interp *iPtr = (Interp *) interp;
    register Var *varPtr = (Var *) variable;
    char *name;

    /*
     * Add the full name of the containing namespace (if any), followed by
     * the "::" separator, then the variable name.
     */

    if (varPtr != NULL) {
	if (!TclIsVarArrayElement(varPtr)) {
	    if (varPtr->nsPtr != NULL) {
		Tcl_AppendToObj(objPtr, varPtr->nsPtr->fullName, -1);
		if (varPtr->nsPtr != iPtr->globalNsPtr) {
		    Tcl_AppendToObj(objPtr, "::", 2);
		}
	    }
	    if (varPtr->name != NULL) {
		Tcl_AppendToObj(objPtr, varPtr->name, -1);
	    } else if (varPtr->hPtr != NULL) {
		name = Tcl_GetHashKey(varPtr->hPtr->tablePtr, varPtr->hPtr);
		Tcl_AppendToObj(objPtr, name, -1);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobalObjCmd --
 *
 *	This object-based procedure is invoked to process the "global" Tcl
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

int
Tcl_GlobalObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    register Tcl_Obj *objPtr;
    char *varName;
    register char *tail;
    int result, i;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?varName ...?");
	return TCL_ERROR;
    }

    /*
     * If we are not executing inside a Tcl procedure, just return.
     */
    
    if ((iPtr->varFramePtr == NULL)
	    || !iPtr->varFramePtr->isProcCallFrame) {
	return TCL_OK;
    }

    for (i = 1;  i < objc;  i++) {
	/*
	 * Make a local variable linked to its counterpart in the global ::
	 * namespace.
	 */
	
	objPtr = objv[i];
	varName = TclGetString(objPtr);

	/*
	 * The variable name might have a scope qualifier, but the name for
         * the local "link" variable must be the simple name at the tail.
	 */

	for (tail = varName;  *tail != '\0';  tail++) {
	    /* empty body */
	}
        while ((tail > varName) && ((*tail != ':') || (*(tail-1) != ':'))) {
            tail--;
	}
        if ((*tail == ':') && (tail > varName)) {
            tail++;
	}

	/*
	 * Link to the variable "varName" in the global :: namespace.
	 */
	
	result = ObjMakeUpvar(interp, (CallFrame *) NULL,
		objPtr, NULL, /*otherFlags*/ TCL_GLOBAL_ONLY,
	        /*myName*/ tail, /*myFlags*/ 0, -1);
	if (result != TCL_OK) {
	    return result;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VariableObjCmd --
 *
 *	Invoked to implement the "variable" command that creates one or more
 *	global variables. Handles the following syntax:
 *
 *	    variable ?name value...? name ?value?
 *
 *	One or more variables can be created. The variables are initialized
 *	with the specified values. The value for the last variable is
 *	optional.
 *
 *	If the variable does not exist, it is created and given the optional
 *	value. If it already exists, it is simply set to the optional
 *	value. Normally, "name" is an unqualified name, so it is created in
 *	the current namespace. If it includes namespace qualifiers, it can
 *	be created in another namespace.
 *
 *	If the variable command is executed inside a Tcl procedure, it
 *	creates a local variable linked to the newly-created namespace
 *	variable.
 *
 * Results:
 *	Returns TCL_OK if the variable is found or created. Returns
 *	TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	If anything goes wrong, this procedure returns an error message
 *	as the result in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_VariableObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    char *varName, *tail, *cp;
    Var *varPtr, *arrayPtr;
    Tcl_Obj *varValuePtr;
    int i, result;
    Tcl_Obj *varNamePtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?name value...? name ?value?");
	return TCL_ERROR;
    }

    for (i = 1;  i < objc;  i = i+2) {
	/*
	 * Look up each variable in the current namespace context, creating
	 * it if necessary.
	 */
	
	varNamePtr = objv[i];
	varName = TclGetString(varNamePtr);
	varPtr = TclObjLookupVar(interp, varNamePtr, NULL,
                (TCL_NAMESPACE_ONLY | TCL_LEAVE_ERR_MSG), "define",
                /*createPart1*/ 1, /*createPart2*/ 0, &arrayPtr);
	
        if (arrayPtr != NULL) {
            /*
             * Variable cannot be an element in an array.  If arrayPtr is
             * non-null, it is, so throw up an error and return.
             */
            VarErrMsg(interp, varName, NULL, "define", isArrayElement);
            return TCL_ERROR;
        }

	if (varPtr == NULL) {
	    return TCL_ERROR;
	}

	/*
	 * Mark the variable as a namespace variable and increment its 
	 * reference count so that it will persist until its namespace is
	 * destroyed or until the variable is unset.
	 */

	if (!(varPtr->flags & VAR_NAMESPACE_VAR)) {
	    varPtr->flags |= VAR_NAMESPACE_VAR;
	    varPtr->refCount++;
	}

	/*
	 * If a value was specified, set the variable to that value.
	 * Otherwise, if the variable is new, leave it undefined.
	 * (If the variable already exists and no value was specified,
	 * leave its value unchanged; just create the local link if
	 * we're in a Tcl procedure).
	 */

	if (i+1 < objc) {	/* a value was specified */
	    varValuePtr = TclPtrSetVar(interp, varPtr, arrayPtr, varName, NULL,
		    objv[i+1], (TCL_NAMESPACE_ONLY | TCL_LEAVE_ERR_MSG));
	    if (varValuePtr == NULL) {
		return TCL_ERROR;
	    }
	}

	/*
	 * If we are executing inside a Tcl procedure, create a local
	 * variable linked to the new namespace variable "varName".
	 */

	if ((iPtr->varFramePtr != NULL)
	        && iPtr->varFramePtr->isProcCallFrame) {
	    /*
	     * varName might have a scope qualifier, but the name for the
	     * local "link" variable must be the simple name at the tail.
	     *
	     * Locate tail in one pass: drop any prefix after two *or more*
	     * consecutive ":" characters).
	     */

	    for (tail = cp = varName;  *cp != '\0'; ) {
		if (*cp++ == ':') {
		    while (*cp == ':') {
			tail = ++cp;
		    }
		}
	    }
	    
	    /*
	     * Create a local link "tail" to the variable "varName" in the
	     * current namespace.
	     */
	    
	    result = ObjMakeUpvar(interp, (CallFrame *) NULL,
		    /*otherP1*/ varNamePtr, /*otherP2*/ NULL,
                    /*otherFlags*/ TCL_NAMESPACE_ONLY,
		    /*myName*/ tail, /*myFlags*/ 0, -1);
	    if (result != TCL_OK) {
		return result;
	    }
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpvarObjCmd --
 *
 *	This object-based procedure is invoked to process the "upvar"
 *	Tcl command. See the user documentation for details on what it does.
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
Tcl_UpvarObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    CallFrame *framePtr;
    char *frameSpec, *localName;
    int result;

    if (objc < 3) {
	upvarSyntax:
	Tcl_WrongNumArgs(interp, 1, objv,
		"?level? otherVar localVar ?otherVar localVar ...?");
	return TCL_ERROR;
    }

    /*
     * Find the call frame containing each of the "other variables" to be
     * linked to. 
     */

    frameSpec = TclGetString(objv[1]);
    result = TclGetFrame(interp, frameSpec, &framePtr);
    if (result == -1) {
	return TCL_ERROR;
    }
    objc -= result+1;
    if ((objc & 1) != 0) {
	goto upvarSyntax;
    }
    objv += result+1;

    /*
     * Iterate over each (other variable, local variable) pair.
     * Divide the other variable name into two parts, then call
     * MakeUpvar to do all the work of linking it to the local variable.
     */

    for ( ;  objc > 0;  objc -= 2, objv += 2) {
	localName = TclGetString(objv[1]);
	result = ObjMakeUpvar(interp, framePtr, /* othervarName */ objv[0],
		NULL, 0, /* myVarName */ localName, /*flags*/ 0, -1);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DisposeTraceResult--
 *
 *	This procedure is called to dispose of the result returned from
 *	a trace procedure.  The disposal method appropriate to the type
 *	of result is determined by flags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The memory allocated for the trace result may be freed.
 *
 *----------------------------------------------------------------------
 */

static void
DisposeTraceResult(flags, result)
    int flags;			/* Indicates type of result to determine
				 * proper disposal method */
    char *result;		/* The result returned from a trace
				 * procedure to be disposed */
{
    if (flags & TCL_TRACE_RESULT_DYNAMIC) {
	ckfree(result);
    } else if (flags & TCL_TRACE_RESULT_OBJECT) {
	Tcl_DecrRefCount((Tcl_Obj *) result);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CallVarTraces --
 *
 *	This procedure is invoked to find and invoke relevant
 *	trace procedures associated with a particular operation on
 *	a variable. This procedure invokes traces both on the
 *	variable and on its containing array (where relevant).
 *
 * Results:
 *      Returns TCL_OK to indicate normal operation.  Returns TCL_ERROR
 *      if invocation of a trace procedure indicated an error.  When
 *      TCL_ERROR is returned and leaveErrMsg is true, then the
 *      ::errorInfo variable of iPtr has information about the error
 *      appended to it.
 *
 * Side effects:
 *	Almost anything can happen, depending on trace; this procedure
 *	itself doesn't have any side effects.
 *
 *----------------------------------------------------------------------
 */

static int
CallVarTraces(iPtr, arrayPtr, varPtr, part1, part2, flags, leaveErrMsg)
    Interp *iPtr;		/* Interpreter containing variable. */
    register Var *arrayPtr;	/* Pointer to array variable that contains
				 * the variable, or NULL if the variable
				 * isn't an element of an array. */
    Var *varPtr;		/* Variable whose traces are to be
				 * invoked. */
    CONST char *part1;
    CONST char *part2;		/* Variable's two-part name. */
    int flags;			/* Flags passed to trace procedures:
				 * indicates what's happening to variable,
				 * plus other stuff like TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, and
				 * TCL_INTERP_DESTROYED. */
    CONST int leaveErrMsg;	/* If true, and one of the traces indicates an
				 * error, then leave an error message and stack
				 * trace information in *iPTr. */
{
    register VarTrace *tracePtr;
    ActiveVarTrace active;
    char *result;
    CONST char *openParen, *p;
    Tcl_DString nameCopy;
    int copiedName;
    int code = TCL_OK;
    int disposeFlags = 0;

    /*
     * If there are already similar trace procedures active for the
     * variable, don't call them again.
     */

    if (varPtr->flags & VAR_TRACE_ACTIVE) {
	return code;
    }
    varPtr->flags |= VAR_TRACE_ACTIVE;
    varPtr->refCount++;
    if (arrayPtr != NULL) {
	arrayPtr->refCount++;
    }

    /*
     * If the variable name hasn't been parsed into array name and
     * element, do it here.  If there really is an array element,
     * make a copy of the original name so that NULLs can be
     * inserted into it to separate the names (can't modify the name
     * string in place, because the string might get used by the
     * callbacks we invoke).
     */

    copiedName = 0;
    if (part2 == NULL) {
	for (p = part1; *p ; p++) {
	    if (*p == '(') {
		openParen = p;
		do {
		    p++;
		} while (*p != '\0');
		p--;
		if (*p == ')') {
		    int offset = (openParen - part1);
		    char *newPart1;
		    Tcl_DStringInit(&nameCopy);
		    Tcl_DStringAppend(&nameCopy, part1, (p-part1));
		    newPart1 = Tcl_DStringValue(&nameCopy);
		    newPart1[offset] = 0;
		    part1 = newPart1;
		    part2 = newPart1 + offset + 1;
		    copiedName = 1;
		}
		break;
	    }
	}
    }

    /*
     * Invoke traces on the array containing the variable, if relevant.
     */

    result = NULL;
    active.nextPtr = iPtr->activeVarTracePtr;
    iPtr->activeVarTracePtr = &active;
    Tcl_Preserve((ClientData) iPtr);
    if (arrayPtr != NULL && !(arrayPtr->flags & VAR_TRACE_ACTIVE)) {
	active.varPtr = arrayPtr;
	for (tracePtr = arrayPtr->tracePtr;  tracePtr != NULL;
	     tracePtr = active.nextTracePtr) {
	    active.nextTracePtr = tracePtr->nextPtr;
	    if (!(tracePtr->flags & flags)) {
		continue;
	    }
	    Tcl_Preserve((ClientData) tracePtr);
	    result = (*tracePtr->traceProc)(tracePtr->clientData,
		    (Tcl_Interp *) iPtr, part1, part2, flags);
	    if (result != NULL) {
		if (flags & TCL_TRACE_UNSETS) {
		    /* Ignore errors in unset traces */
		    DisposeTraceResult(tracePtr->flags, result);
		} else {
	            disposeFlags = tracePtr->flags;
		    code = TCL_ERROR;
		}
	    }
	    Tcl_Release((ClientData) tracePtr);
	    if (code == TCL_ERROR) {
		goto done;
	    }
	}
    }

    /*
     * Invoke traces on the variable itself.
     */

    if (flags & TCL_TRACE_UNSETS) {
	flags |= TCL_TRACE_DESTROYED;
    }
    active.varPtr = varPtr;
    for (tracePtr = varPtr->tracePtr; tracePtr != NULL;
	 tracePtr = active.nextTracePtr) {
	active.nextTracePtr = tracePtr->nextPtr;
	if (!(tracePtr->flags & flags)) {
	    continue;
	}
	Tcl_Preserve((ClientData) tracePtr);
	result = (*tracePtr->traceProc)(tracePtr->clientData,
		(Tcl_Interp *) iPtr, part1, part2, flags);
	if (result != NULL) {
	    if (flags & TCL_TRACE_UNSETS) {
		/* Ignore errors in unset traces */
		DisposeTraceResult(tracePtr->flags, result);
	    } else {
		disposeFlags = tracePtr->flags;
		code = TCL_ERROR;
	    }
	}
	Tcl_Release((ClientData) tracePtr);
	if (code == TCL_ERROR) {
	    goto done;
	}
    }

    /*
     * Restore the variable's flags, remove the record of our active
     * traces, and then return.
     */

    done:
    if (code == TCL_ERROR) {
	if (leaveErrMsg) {
	    CONST char *type = "";
	    switch (flags&(TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_ARRAY)) {
		case TCL_TRACE_READS: {
		    type = "read";
		    break;
		}
		case TCL_TRACE_WRITES: {
		    type = "set";
		    break;
		}
		case TCL_TRACE_ARRAY: {
		    type = "trace array";
		    break;
		}
	    }
	    if (disposeFlags & TCL_TRACE_RESULT_OBJECT) {
		VarErrMsg((Tcl_Interp *) iPtr, part1, part2, type,
			Tcl_GetString((Tcl_Obj *) result));
	    } else {
		VarErrMsg((Tcl_Interp *) iPtr, part1, part2, type, result);
	    }
	}
	DisposeTraceResult(disposeFlags,result);
    }

    if (arrayPtr != NULL) {
	arrayPtr->refCount--;
    }
    if (copiedName) {
	Tcl_DStringFree(&nameCopy);
    }
    varPtr->flags &= ~VAR_TRACE_ACTIVE;
    varPtr->refCount--;
    iPtr->activeVarTracePtr = active.nextPtr;
    Tcl_Release((ClientData) iPtr);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * NewVar --
 *
 *	Create a new heap-allocated variable that will eventually be
 *	entered into a hashtable.
 *
 * Results:
 *	The return value is a pointer to the new variable structure. It is
 *	marked as a scalar variable (and not a link or array variable). Its
 *	value initially is NULL. The variable is not part of any hash table
 *	yet. Since it will be in a hashtable and not in a call frame, its
 *	name field is set NULL. It is initially marked as undefined.
 *
 * Side effects:
 *	Storage gets allocated.
 *
 *----------------------------------------------------------------------
 */

static Var *
NewVar()
{
    register Var *varPtr;

    varPtr = (Var *) ckalloc(sizeof(Var));
    varPtr->value.objPtr = NULL;
    varPtr->name = NULL;
    varPtr->nsPtr = NULL;
    varPtr->hPtr = NULL;
    varPtr->refCount = 0;
    varPtr->tracePtr = NULL;
    varPtr->searchPtr = NULL;
    varPtr->flags = (VAR_SCALAR | VAR_UNDEFINED | VAR_IN_HASHTABLE);
    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * SetArraySearchObj --
 *
 *	This function converts the given tcl object into one that
 *	has the "array search" internal type.
 *
 * Results:
 *	TCL_OK if the conversion succeeded, and TCL_ERROR if it failed
 *	(when an error message will be placed in the interpreter's
 *	result.)
 *
 * Side effects:
 *	Updates the internal type and representation of the object to
 *	make this an array-search object.  See the tclArraySearchType
 *	declaration above for details of the internal representation.
 *
 *----------------------------------------------------------------------
 */

static int
SetArraySearchObj(interp, objPtr)
    Tcl_Interp *interp;
    Tcl_Obj *objPtr;
{
    char *string;
    char *end;
    int id;
    size_t offset;

    /*
     * Get the string representation. Make it up-to-date if necessary.
     */

    string = Tcl_GetString(objPtr);

    /*
     * Parse the id into the three parts separated by dashes.
     */
    if ((string[0] != 's') || (string[1] != '-')) {
	syntax:
	Tcl_AppendResult(interp, "illegal search identifier \"", string,
		"\"", (char *) NULL);
	return TCL_ERROR;
    }
    id = strtoul(string+2, &end, 10);
    if ((end == (string+2)) || (*end != '-')) {
	goto syntax;
    }
    /*
     * Can't perform value check in this context, so place reference
     * to place in string to use for the check in the object instead.
     */
    end++;
    offset = end - string;

    if (objPtr->typePtr != NULL && objPtr->typePtr->freeIntRepProc != NULL) {
	objPtr->typePtr->freeIntRepProc(objPtr);
    }
    objPtr->typePtr = &tclArraySearchType;
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *)(((char *)NULL)+id);
    objPtr->internalRep.twoPtrValue.ptr2 = (VOID *)(((char *)NULL)+offset);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseSearchId --
 *
 *	This procedure translates from a tcl object to a pointer to an
 *	active array search (if there is one that matches the string).
 *
 * Results:
 *	The return value is a pointer to the array search indicated
 *	by string, or NULL if there isn't one.  If NULL is returned,
 *	the interp's result contains an error message.
 *
 * Side effects:
 *	The tcl object might have its internal type and representation
 *	modified.
 *
 *----------------------------------------------------------------------
 */

static ArraySearch *
ParseSearchId(interp, varPtr, varName, handleObj)
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    CONST Var *varPtr;		/* Array variable search is for. */
    CONST char *varName;	/* Name of array variable that search is
				 * supposed to be for. */
    Tcl_Obj *handleObj;		/* Object containing id of search. Must have
				 * form "search-num-var" where "num" is a
				 * decimal number and "var" is a variable
				 * name. */
{
    register char *string;
    register size_t offset;
    int id;
    ArraySearch *searchPtr;

    /*
     * Parse the id.
     */
    if (Tcl_ConvertToType(interp, handleObj, &tclArraySearchType) != TCL_OK) {
	return NULL;
    }
    /*
     * Cast is safe, since always came from an int in the first place.
     */
    id = (int)(((char*)handleObj->internalRep.twoPtrValue.ptr1) -
	       ((char*)NULL));
    string = Tcl_GetString(handleObj);
    offset = (((char*)handleObj->internalRep.twoPtrValue.ptr2) -
	      ((char*)NULL));
    /*
     * This test cannot be placed inside the Tcl_Obj machinery, since
     * it is dependent on the variable context.
     */
    if (strcmp(string+offset, varName) != 0) {
	Tcl_AppendResult(interp, "search identifier \"", string,
		"\" isn't for variable \"", varName, "\"", (char *) NULL);
	return NULL;
    }

    /*
     * Search through the list of active searches on the interpreter
     * to see if the desired one exists.
     *
     * Note that we cannot store the searchPtr directly in the Tcl_Obj
     * as that would run into trouble when DeleteSearches() was called
     * so we must scan this list every time.
     */

    for (searchPtr = varPtr->searchPtr; searchPtr != NULL;
	 searchPtr = searchPtr->nextPtr) {
	if (searchPtr->id == id) {
	    return searchPtr;
	}
    }
    Tcl_AppendResult(interp, "couldn't find search \"", string, "\"",
	    (char *) NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteSearches --
 *
 *	This procedure is called to free up all of the searches
 *	associated with an array variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is released to the storage allocator.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteSearches(arrayVarPtr)
    register Var *arrayVarPtr;		/* Variable whose searches are
					 * to be deleted. */
{
    ArraySearch *searchPtr;

    while (arrayVarPtr->searchPtr != NULL) {
	searchPtr = arrayVarPtr->searchPtr;
	arrayVarPtr->searchPtr = searchPtr->nextPtr;
	ckfree((char *) searchPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteVars --
 *
 *	This procedure is called to recycle all the storage space
 *	associated with a table of variables. For this procedure
 *	to work correctly, it must not be possible for any of the
 *	variables in the table to be accessed from Tcl commands
 *	(e.g. from trace procedures).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables are deleted and trace procedures are invoked, if
 *	any are declared.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteVars(iPtr, tablePtr)
    Interp *iPtr;		/* Interpreter to which variables belong. */
    Tcl_HashTable *tablePtr;	/* Hash table containing variables to
				 * delete. */
{
    Tcl_Interp *interp = (Tcl_Interp *) iPtr;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    register Var *varPtr;
    Var *linkPtr;
    int flags;
    ActiveVarTrace *activePtr;
    Tcl_Obj *objPtr;
    Namespace *currNsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);

    /*
     * Determine what flags to pass to the trace callback procedures.
     */

    flags = TCL_TRACE_UNSETS;
    if (tablePtr == &iPtr->globalNsPtr->varTable) {
	flags |= TCL_GLOBAL_ONLY;
    } else if (tablePtr == &currNsPtr->varTable) {
	flags |= TCL_NAMESPACE_ONLY;
    }
    if (Tcl_InterpDeleted(interp)) {
	flags |= TCL_INTERP_DESTROYED;
    }

    for (hPtr = Tcl_FirstHashEntry(tablePtr, &search);  hPtr != NULL;
	 hPtr = Tcl_NextHashEntry(&search)) {
	varPtr = (Var *) Tcl_GetHashValue(hPtr);

	/*
	 * For global/upvar variables referenced in procedures, decrement
	 * the reference count on the variable referred to, and free
	 * the referenced variable if it's no longer needed. Don't delete
	 * the hash entry for the other variable if it's in the same table
	 * as us: this will happen automatically later on.
	 */

	if (TclIsVarLink(varPtr)) {
	    linkPtr = varPtr->value.linkPtr;
	    linkPtr->refCount--;
	    if ((linkPtr->refCount == 0) && TclIsVarUndefined(linkPtr)
		    && (linkPtr->tracePtr == NULL)
		    && (linkPtr->flags & VAR_IN_HASHTABLE)) {
		if (linkPtr->hPtr == NULL) {
		    ckfree((char *) linkPtr);
		} else if (linkPtr->hPtr->tablePtr != tablePtr) {
		    Tcl_DeleteHashEntry(linkPtr->hPtr);
		    ckfree((char *) linkPtr);
		}
	    }
	}

	/*
	 * Invoke traces on the variable that is being deleted, then
	 * free up the variable's space (no need to free the hash entry
	 * here, unless we're dealing with a global variable: the
	 * hash entries will be deleted automatically when the whole
	 * table is deleted). Note that we give CallVarTraces the variable's
	 * fully-qualified name so that any called trace procedures can
	 * refer to these variables being deleted.
	 */

	if (varPtr->tracePtr != NULL) {
	    objPtr = Tcl_NewObj();
	    Tcl_IncrRefCount(objPtr); /* until done with traces */
	    Tcl_GetVariableFullName(interp, (Tcl_Var) varPtr, objPtr);
	    CallVarTraces(iPtr, (Var *) NULL, varPtr, Tcl_GetString(objPtr),
		    NULL, flags, /* leaveErrMsg */ 0);
	    Tcl_DecrRefCount(objPtr); /* free no longer needed obj */

	    while (varPtr->tracePtr != NULL) {
		VarTrace *tracePtr = varPtr->tracePtr;
		varPtr->tracePtr = tracePtr->nextPtr;
		Tcl_EventuallyFree((ClientData) tracePtr, TCL_DYNAMIC);
	    }
	    for (activePtr = iPtr->activeVarTracePtr; activePtr != NULL;
		 activePtr = activePtr->nextPtr) {
		if (activePtr->varPtr == varPtr) {
		    activePtr->nextTracePtr = NULL;
		}
	    }
	}
	    
	if (TclIsVarArray(varPtr)) {
	    DeleteArray(iPtr, Tcl_GetHashKey(tablePtr, hPtr), varPtr,
	            flags);
	    varPtr->value.tablePtr = NULL;
	}
	if (TclIsVarScalar(varPtr) && (varPtr->value.objPtr != NULL)) {
	    objPtr = varPtr->value.objPtr;
	    TclDecrRefCount(objPtr);
	    varPtr->value.objPtr = NULL;
	}
	varPtr->hPtr = NULL;
	varPtr->tracePtr = NULL;
	TclSetVarUndefined(varPtr);
	TclSetVarScalar(varPtr);

	/*
	 * If the variable was a namespace variable, decrement its 
	 * reference count. We are in the process of destroying its
	 * namespace so that namespace will no longer "refer" to the
	 * variable.
	 */

	if (varPtr->flags & VAR_NAMESPACE_VAR) {
	    varPtr->flags &= ~VAR_NAMESPACE_VAR;
	    varPtr->refCount--;
	}

	/*
	 * Recycle the variable's memory space if there aren't any upvar's
	 * pointing to it. If there are upvars to this variable, then the
	 * variable will get freed when the last upvar goes away.
	 */

	if (varPtr->refCount == 0) {
	    ckfree((char *) varPtr); /* this Var must be VAR_IN_HASHTABLE */
	}
    }
    Tcl_DeleteHashTable(tablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteCompiledLocalVars --
 *
 *	This procedure is called to recycle storage space associated with
 *	the compiler-allocated array of local variables in a procedure call
 *	frame. This procedure resembles TclDeleteVars above except that each
 *	variable is stored in a call frame and not a hash table. For this
 *	procedure to work correctly, it must not be possible for any of the
 *	variable in the table to be accessed from Tcl commands (e.g. from
 *	trace procedures).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables are deleted and trace procedures are invoked, if
 *	any are declared.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteCompiledLocalVars(iPtr, framePtr)
    Interp *iPtr;		/* Interpreter to which variables belong. */
    CallFrame *framePtr;	/* Procedure call frame containing
				 * compiler-assigned local variables to
				 * delete. */
{
    register Var *varPtr;
    int flags;			/* Flags passed to trace procedures. */
    Var *linkPtr;
    ActiveVarTrace *activePtr;
    int numLocals, i;

    flags = TCL_TRACE_UNSETS;
    numLocals = framePtr->numCompiledLocals;
    varPtr = framePtr->compiledLocals;
    for (i = 0;  i < numLocals;  i++) {
	/*
	 * For global/upvar variables referenced in procedures, decrement
	 * the reference count on the variable referred to, and free
	 * the referenced variable if it's no longer needed. Don't delete
	 * the hash entry for the other variable if it's in the same table
	 * as us: this will happen automatically later on.
	 */

	if (TclIsVarLink(varPtr)) {
	    linkPtr = varPtr->value.linkPtr;
	    linkPtr->refCount--;
	    if ((linkPtr->refCount == 0) && TclIsVarUndefined(linkPtr)
		    && (linkPtr->tracePtr == NULL)
		    && (linkPtr->flags & VAR_IN_HASHTABLE)) {
		if (linkPtr->hPtr == NULL) {
		    ckfree((char *) linkPtr);
		} else {
		    Tcl_DeleteHashEntry(linkPtr->hPtr);
		    ckfree((char *) linkPtr);
		}
	    }
	}

	/*
	 * Invoke traces on the variable that is being deleted. Then delete
	 * the variable's trace records.
	 */

	if (varPtr->tracePtr != NULL) {
	    CallVarTraces(iPtr, (Var *) NULL, varPtr, varPtr->name, NULL,
		    flags, /* leaveErrMsg */ 0);
	    while (varPtr->tracePtr != NULL) {
		VarTrace *tracePtr = varPtr->tracePtr;
		varPtr->tracePtr = tracePtr->nextPtr;
		Tcl_EventuallyFree((ClientData) tracePtr, TCL_DYNAMIC);
	    }
	    for (activePtr = iPtr->activeVarTracePtr; activePtr != NULL;
		 activePtr = activePtr->nextPtr) {
		if (activePtr->varPtr == varPtr) {
		    activePtr->nextTracePtr = NULL;
		}
	    }
	}

        /*
	 * Now if the variable is an array, delete its element hash table.
	 * Otherwise, if it's a scalar variable, decrement the ref count
	 * of its value.
	 */
	    
	if (TclIsVarArray(varPtr) && (varPtr->value.tablePtr != NULL)) {
	    DeleteArray(iPtr, varPtr->name, varPtr, flags);
	}
	if (TclIsVarScalar(varPtr) && (varPtr->value.objPtr != NULL)) {
	    TclDecrRefCount(varPtr->value.objPtr);
	    varPtr->value.objPtr = NULL;
	}
	varPtr->hPtr = NULL;
	varPtr->tracePtr = NULL;
	TclSetVarUndefined(varPtr);
	TclSetVarScalar(varPtr);
	varPtr++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteArray --
 *
 *	This procedure is called to free up everything in an array
 *	variable.  It's the caller's responsibility to make sure
 *	that the array is no longer accessible before this procedure
 *	is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All storage associated with varPtr's array elements is deleted
 *	(including the array's hash table). Deletion trace procedures for
 *	array elements are invoked, then deleted. Any pending traces for
 *	array elements are also deleted.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteArray(iPtr, arrayName, varPtr, flags)
    Interp *iPtr;			/* Interpreter containing array. */
    CONST char *arrayName;	        /* Name of array (used for trace
					 * callbacks). */
    Var *varPtr;			/* Pointer to variable structure. */
    int flags;				/* Flags to pass to CallVarTraces:
					 * TCL_TRACE_UNSETS and sometimes
					 * TCL_INTERP_DESTROYED,
					 * TCL_NAMESPACE_ONLY, or
					 * TCL_GLOBAL_ONLY. */
{
    Tcl_HashSearch search;
    register Tcl_HashEntry *hPtr;
    register Var *elPtr;
    ActiveVarTrace *activePtr;
    Tcl_Obj *objPtr;

    DeleteSearches(varPtr);
    for (hPtr = Tcl_FirstHashEntry(varPtr->value.tablePtr, &search);
	 hPtr != NULL;  hPtr = Tcl_NextHashEntry(&search)) {
	elPtr = (Var *) Tcl_GetHashValue(hPtr);
	if (TclIsVarScalar(elPtr) && (elPtr->value.objPtr != NULL)) {
	    objPtr = elPtr->value.objPtr;
	    TclDecrRefCount(objPtr);
	    elPtr->value.objPtr = NULL;
	}
	elPtr->hPtr = NULL;
	if (elPtr->tracePtr != NULL) {
	    elPtr->flags &= ~VAR_TRACE_ACTIVE;
	    CallVarTraces(iPtr, (Var *) NULL, elPtr, arrayName,
		    Tcl_GetHashKey(varPtr->value.tablePtr, hPtr), flags,
		    /* leaveErrMsg */ 0);
	    while (elPtr->tracePtr != NULL) {
		VarTrace *tracePtr = elPtr->tracePtr;
		elPtr->tracePtr = tracePtr->nextPtr;
		Tcl_EventuallyFree((ClientData) tracePtr,TCL_DYNAMIC);
	    }
	    for (activePtr = iPtr->activeVarTracePtr; activePtr != NULL;
		 activePtr = activePtr->nextPtr) {
		if (activePtr->varPtr == elPtr) {
		    activePtr->nextTracePtr = NULL;
		}
	    }
	}
	TclSetVarUndefined(elPtr);
	TclSetVarScalar(elPtr);

	/*
	 * Even though array elements are not supposed to be namespace
	 * variables, some combinations of [upvar] and [variable] may
	 * create such beasts - see [Bug 604239]. This is necessary to
	 * avoid leaking the corresponding Var struct, and is otherwise
	 * harmless. 
	 */

	if (elPtr->flags & VAR_NAMESPACE_VAR) {
	    elPtr->flags &= ~VAR_NAMESPACE_VAR;
	    elPtr->refCount--;
	}
	if (elPtr->refCount == 0) {
	    ckfree((char *) elPtr); /* element Vars are VAR_IN_HASHTABLE */
	}
    }
    Tcl_DeleteHashTable(varPtr->value.tablePtr);
    ckfree((char *) varPtr->value.tablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CleanupVar --
 *
 *	This procedure is called when it looks like it may be OK to free up
 *	a variable's storage. If the variable is in a hashtable, its Var
 *	structure and hash table entry will be freed along with those of its
 *	containing array, if any. This procedure is called, for example,
 *	when a trace on a variable deletes a variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the variable (or its containing array) really is dead and in a
 *	hashtable, then its Var structure, and possibly its hash table
 *	entry, is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
CleanupVar(varPtr, arrayPtr)
    Var *varPtr;		/* Pointer to variable that may be a
				 * candidate for being expunged. */
    Var *arrayPtr;		/* Array that contains the variable, or
				 * NULL if this variable isn't an array
				 * element. */
{
    if (TclIsVarUndefined(varPtr) && (varPtr->refCount == 0)
	    && (varPtr->tracePtr == NULL)
	    && (varPtr->flags & VAR_IN_HASHTABLE)) {
	if (varPtr->hPtr != NULL) {
	    Tcl_DeleteHashEntry(varPtr->hPtr);
	}
	ckfree((char *) varPtr);
    }
    if (arrayPtr != NULL) {
	if (TclIsVarUndefined(arrayPtr) && (arrayPtr->refCount == 0)
		&& (arrayPtr->tracePtr == NULL)
	        && (arrayPtr->flags & VAR_IN_HASHTABLE)) {
	    if (arrayPtr->hPtr != NULL) {
		Tcl_DeleteHashEntry(arrayPtr->hPtr);
	    }
	    ckfree((char *) arrayPtr);
	}
    }
}
/*
 *----------------------------------------------------------------------
 *
 * VarErrMsg --
 *
 *      Generate a reasonable error message describing why a variable
 *      operation failed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The interp's result is set to hold a message identifying the
 *      variable given by part1 and part2 and describing why the
 *      variable operation failed.
 *
 *----------------------------------------------------------------------
 */

static void
VarErrMsg(interp, part1, part2, operation, reason)
    Tcl_Interp *interp;         /* Interpreter in which to record message. */
    CONST char *part1;
    CONST char *part2;		/* Variable's two-part name. */
    CONST char *operation;      /* String describing operation that failed,
                                 * e.g. "read", "set", or "unset". */
    CONST char *reason;         /* String describing why operation failed. */
{
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "can't ", operation, " \"", part1,
	    (char *) NULL);
    if (part2 != NULL) {
        Tcl_AppendResult(interp, "(", part2, ")", (char *) NULL);
    }
    Tcl_AppendResult(interp, "\": ", reason, (char *) NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TclTraceVarExists --
 *
 *	This is called from info exists.  We need to trigger read
 *	and/or array traces because they may end up creating a
 *	variable that doesn't currently exist.
 *
 * Results:
 *	A pointer to the Var structure, or NULL.
 *
 * Side effects:
 *	May fill in error messages in the interp.
 *
 *----------------------------------------------------------------------
 */

Var *
TclVarTraceExists(interp, varName)
    Tcl_Interp *interp;		/* The interpreter */
    CONST char *varName;	/* The variable name */
{
    Var *varPtr;
    Var *arrayPtr;

    /*
     * The choice of "create" flag values is delicate here, and
     * matches the semantics of GetVar.  Things are still not perfect,
     * however, because if you do "info exists x" you get a varPtr
     * and therefore trigger traces.  However, if you do 
     * "info exists x(i)", then you only get a varPtr if x is already
     * known to be an array.  Otherwise you get NULL, and no trace
     * is triggered.  This matches Tcl 7.6 semantics.
     */

    varPtr = TclLookupVar(interp, varName, (char *) NULL,
            0, "access", /*createPart1*/ 0, /*createPart2*/ 1, &arrayPtr);

    if (varPtr == NULL) {
	return NULL;
    }

    if ((varPtr->tracePtr != NULL)
	    || ((arrayPtr != NULL) && (arrayPtr->tracePtr != NULL))) {
	CallVarTraces((Interp *)interp, arrayPtr, varPtr, varName, NULL,
		TCL_TRACE_READS, /* leaveErrMsg */ 0);
    }

    /*
     * If the variable doesn't exist anymore and no-one's using
     * it, then free up the relevant structures and hash table entries.
     */

    if (TclIsVarUndefined(varPtr)) {
	CleanupVar(varPtr, arrayPtr);
	return NULL;
    }

    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Internal functions for variable name object types --
 *
 *----------------------------------------------------------------------
 */

/* 
 * localVarName -
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1 = pointer to the corresponding Proc 
 *   twoPtrValue.ptr2 = index into locals table
*/

static void 
FreeLocalVarName(objPtr)
    Tcl_Obj *objPtr;
{
    register Proc *procPtr = (Proc *) objPtr->internalRep.twoPtrValue.ptr1;
    procPtr->refCount--;
    if (procPtr->refCount <= 0) {
	TclProcCleanupProc(procPtr);
    }
}

static void
DupLocalVarName(srcPtr, dupPtr)
    Tcl_Obj *srcPtr;
    Tcl_Obj *dupPtr;
{
    register Proc *procPtr = (Proc *) srcPtr->internalRep.twoPtrValue.ptr1;

    dupPtr->internalRep.twoPtrValue.ptr1 = (VOID *) procPtr;
    dupPtr->internalRep.twoPtrValue.ptr2 = srcPtr->internalRep.twoPtrValue.ptr2;
    procPtr->refCount++;
    dupPtr->typePtr = &tclLocalVarNameType;
}

static void
UpdateLocalVarName(objPtr)
    Tcl_Obj *objPtr;
{
    Proc *procPtr = (Proc *) objPtr->internalRep.twoPtrValue.ptr1;
    unsigned int index = (unsigned int) objPtr->internalRep.twoPtrValue.ptr2;
    CompiledLocal *localPtr = procPtr->firstLocalPtr;
    unsigned int nameLen;

    if (localPtr == NULL) {
	goto emptyName;
    }
    while (index--) {
	localPtr = localPtr->nextPtr;
	if (localPtr == NULL) {
	    goto emptyName;
	}
    }

    nameLen = (unsigned int) localPtr->nameLength;
    objPtr->bytes = ckalloc(nameLen + 1);
    memcpy(objPtr->bytes, localPtr->name, nameLen + 1);
    objPtr->length = nameLen;
    return;

    emptyName:
    objPtr->bytes = ckalloc(1);
    *(objPtr->bytes) = '\0';
    objPtr->length = 0;
}

/* 
 * nsVarName -
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1: pointer to the namespace containing the 
 *                     reference.
 *   twoPtrValue.ptr2: pointer to the corresponding Var 
*/

static void 
FreeNsVarName(objPtr)
    Tcl_Obj *objPtr;
{
    register Var *varPtr = (Var *) objPtr->internalRep.twoPtrValue.ptr2;

    varPtr->refCount--;
    if (TclIsVarUndefined(varPtr) && (varPtr->refCount <= 0)) {
	if (TclIsVarLink(varPtr)) {
	    Var *linkPtr = varPtr->value.linkPtr;
	    linkPtr->refCount--;
	    if (TclIsVarUndefined(linkPtr) && (linkPtr->refCount <= 0)) {
		CleanupVar(linkPtr, (Var *) NULL);
	    }
	}
	CleanupVar(varPtr, NULL);
    }
}

static void
DupNsVarName(srcPtr, dupPtr)
    Tcl_Obj *srcPtr;
    Tcl_Obj *dupPtr;
{
    Namespace *nsPtr = (Namespace *) srcPtr->internalRep.twoPtrValue.ptr1;
    register Var *varPtr = (Var *) srcPtr->internalRep.twoPtrValue.ptr2;

    dupPtr->internalRep.twoPtrValue.ptr1 =  (VOID *) nsPtr;
    dupPtr->internalRep.twoPtrValue.ptr2 = (VOID *) varPtr;
    varPtr->refCount++;
    dupPtr->typePtr = &tclNsVarNameType;
}

/* 
 * parsedVarName -
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1 = pointer to the array name Tcl_Obj
 *                      (NULL if scalar)
 *   twoPtrValue.ptr2 = pointer to the element name string
 *                      (owned by this Tcl_Obj), or NULL if 
 *                      it is a scalar variable
 */

static void 
FreeParsedVarName(objPtr)
    Tcl_Obj *objPtr;
{
    register Tcl_Obj *arrayPtr =
	    (Tcl_Obj *) objPtr->internalRep.twoPtrValue.ptr1;
    register char *elem = (char *) objPtr->internalRep.twoPtrValue.ptr2;
    
    if (arrayPtr != NULL) {
	TclDecrRefCount(arrayPtr);
	ckfree(elem);
    }
}

static void
DupParsedVarName(srcPtr, dupPtr)
    Tcl_Obj *srcPtr;
    Tcl_Obj *dupPtr;
{
    register Tcl_Obj *arrayPtr =
	    (Tcl_Obj *) srcPtr->internalRep.twoPtrValue.ptr1;
    register char *elem = (char *) srcPtr->internalRep.twoPtrValue.ptr2;
    char *elemCopy;
    unsigned int elemLen;

    if (arrayPtr != NULL) {
	Tcl_IncrRefCount(arrayPtr);
	elemLen = strlen(elem);
	elemCopy = ckalloc(elemLen+1);
	memcpy(elemCopy, elem, elemLen);
	*(elemCopy + elemLen) = '\0';
	elem = elemCopy;
    }

    dupPtr->internalRep.twoPtrValue.ptr1 = (VOID *) arrayPtr;
    dupPtr->internalRep.twoPtrValue.ptr2 = (VOID *) elem;
    dupPtr->typePtr = &tclParsedVarNameType;
}

static void
UpdateParsedVarName(objPtr)
    Tcl_Obj *objPtr;
{
    Tcl_Obj *arrayPtr = (Tcl_Obj *) objPtr->internalRep.twoPtrValue.ptr1;
    char *part2 = (char *) objPtr->internalRep.twoPtrValue.ptr2;
    char *part1, *p;
    int len1, len2, totalLen;

    if (arrayPtr == NULL) {
	/*
	 * This is a parsed scalar name: what is it
	 * doing here?
	 */
	panic("ERROR: scalar parsedVarName without a string rep.\n");
    }
    part1 = Tcl_GetStringFromObj(arrayPtr, &len1);
    len2 = strlen(part2);
	
    totalLen = len1 + len2 + 2;
    p = ckalloc((unsigned int) totalLen + 1);
    objPtr->bytes = p;
    objPtr->length = totalLen;

    memcpy(p, part1, (unsigned int) len1);
    p += len1;
    *p++ = '(';
    memcpy(p, part2, (unsigned int) len2);
    p += len2;
    *p++ = ')';
    *p   = '\0';
}
