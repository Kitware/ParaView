/* 
 * tclIndexObj.c --
 *
 *	This file implements objects of type "index".  This object type
 *	is used to lookup a keyword in a table of valid values and cache
 *	the index of the matching entry.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"

/*
 * Prototypes for procedures defined later in this file:
 */

static int		SetIndexFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));

/*
 * The structure below defines the index Tcl object type by means of
 * procedures that can be invoked by generic object code.
 */

Tcl_ObjType tclIndexType = {
    "index",				/* name */
    (Tcl_FreeInternalRepProc *) NULL,	/* freeIntRepProc */
    (Tcl_DupInternalRepProc *) NULL,	/* dupIntRepProc */
    (Tcl_UpdateStringProc *) NULL,	/* updateStringProc */
    SetIndexFromAny			/* setFromAnyProc */
};

/*
 * Boolean flag indicating whether or not the tclIndexType object
 * type has been registered with the Tcl compiler.
 */

static int indexTypeInitialized = 0;

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetIndexFromObj --
 *
 *	This procedure looks up an object's value in a table of strings
 *	and returns the index of the matching string, if any.
 *
 * Results:
 *
 *	If the value of objPtr is identical to or a unique abbreviation
 *	for one of the entries in objPtr, then the return value is
 *	TCL_OK and the index of the matching entry is stored at
 *	*indexPtr.  If there isn't a proper match, then TCL_ERROR is
 *	returned and an error message is left in interp's result (unless
 *	interp is NULL).  The msg argument is used in the error
 *	message; for example, if msg has the value "option" then the
 *	error message will say something flag 'bad option "foo": must be
 *	...'
 *
 * Side effects:
 *	The result of the lookup is cached as the internal rep of
 *	objPtr, so that repeated lookups can be done quickly.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetIndexFromObj(interp, objPtr, tablePtr, msg, flags, indexPtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* Object containing the string to lookup. */
    char **tablePtr;		/* Array of strings to compare against the
				 * value of objPtr; last entry must be NULL
				 * and there must not be duplicate entries. */
    char *msg;			/* Identifying word to use in error messages. */
    int flags;			/* 0 or TCL_EXACT */
    int *indexPtr;		/* Place to store resulting integer index. */
{

    /*
     * See if there is a valid cached result from a previous lookup
     * (doing the check here saves the overhead of calling
     * Tcl_GetIndexFromObjStruct in the common case where the result
     * is cached).
     */

    if ((objPtr->typePtr == &tclIndexType)
	    && (objPtr->internalRep.twoPtrValue.ptr1 == (VOID *) tablePtr)) {
	*indexPtr = (int) objPtr->internalRep.twoPtrValue.ptr2;
	return TCL_OK;
    }
    return Tcl_GetIndexFromObjStruct(interp, objPtr, tablePtr, sizeof(char *),
	    msg, flags, indexPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetIndexFromObjStruct --
 *
 *	This procedure looks up an object's value given a starting
 *	string and an offset for the amount of space between strings.
 *	This is useful when the strings are embedded in some other
 *	kind of array.
 *
 * Results:
 *
 *	If the value of objPtr is identical to or a unique abbreviation
 *	for one of the entries in objPtr, then the return value is
 *	TCL_OK and the index of the matching entry is stored at
 *	*indexPtr.  If there isn't a proper match, then TCL_ERROR is
 *	returned and an error message is left in interp's result (unless
 *	interp is NULL).  The msg argument is used in the error
 *	message; for example, if msg has the value "option" then the
 *	error message will say something flag 'bad option "foo": must be
 *	...'
 *
 * Side effects:
 *	The result of the lookup is cached as the internal rep of
 *	objPtr, so that repeated lookups can be done quickly.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetIndexFromObjStruct(interp, objPtr, tablePtr, offset, msg, flags, 
	indexPtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* Object containing the string to lookup. */
    char **tablePtr;		/* The first string in the table. The second
				 * string will be at this address plus the
				 * offset, the third plus the offset again,
				 * etc. The last entry must be NULL
				 * and there must not be duplicate entries. */
    int offset;			/* The number of bytes between entries */
    char *msg;			/* Identifying word to use in error messages. */
    int flags;			/* 0 or TCL_EXACT */
    int *indexPtr;		/* Place to store resulting integer index. */
{
    int index, length, i, numAbbrev;
    char *key, *p1, *p2, **entryPtr;
    Tcl_Obj *resultPtr;

    /*
     * See if there is a valid cached result from a previous lookup.
     */

    if ((objPtr->typePtr == &tclIndexType)
	    && (objPtr->internalRep.twoPtrValue.ptr1 == (VOID *) tablePtr)) {
	*indexPtr = (int) objPtr->internalRep.twoPtrValue.ptr2;
	return TCL_OK;
    }

    /*
     * Lookup the value of the object in the table.  Accept unique
     * abbreviations unless TCL_EXACT is set in flags.
     */

    if (!indexTypeInitialized) {
	/*
	 * This is the first time we've done a lookup.  Register the
	 * tclIndexType.
	 */

        Tcl_RegisterObjType(&tclIndexType);
        indexTypeInitialized = 1;
    }

    key = Tcl_GetStringFromObj(objPtr, &length);
    index = -1;
    numAbbrev = 0;

    /*
     * The key should not be empty, otherwise it's not a match.
     */
    
    if (key[0] == '\0') {
	goto error;
    }
    
    for (entryPtr = tablePtr, i = 0; *entryPtr != NULL; 
	    entryPtr = (char **) ((long) entryPtr + offset), i++) {
	for (p1 = key, p2 = *entryPtr; *p1 == *p2; p1++, p2++) {
	    if (*p1 == 0) {
		index = i;
		goto done;
	    }
	}
	if (*p1 == 0) {
	    /*
	     * The value is an abbreviation for this entry.  Continue
	     * checking other entries to make sure it's unique.  If we
	     * get more than one unique abbreviation, keep searching to
	     * see if there is an exact match, but remember the number
	     * of unique abbreviations and don't allow either.
	     */

	    numAbbrev++;
	    index = i;
	}
    }
    if ((flags & TCL_EXACT) || (numAbbrev != 1)) {
	goto error;
    }

    done:
    if ((objPtr->typePtr != NULL)
	    && (objPtr->typePtr->freeIntRepProc != NULL)) {
	objPtr->typePtr->freeIntRepProc(objPtr);
    }
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) tablePtr;
    objPtr->internalRep.twoPtrValue.ptr2 = (VOID *) index;
    objPtr->typePtr = &tclIndexType;
    *indexPtr = index;
    return TCL_OK;

    error:
    if (interp != NULL) {
	int count;
	resultPtr = Tcl_GetObjResult(interp);
	Tcl_AppendStringsToObj(resultPtr,
		(numAbbrev > 1) ? "ambiguous " : "bad ", msg, " \"",
		key, "\": must be ", *tablePtr, (char *) NULL);
	for (entryPtr = (char **) ((long) tablePtr + offset), count = 0;
		*entryPtr != NULL;
		entryPtr = (char **) ((long) entryPtr + offset), count++) {
	    if ((*((char **) ((long) entryPtr + offset))) == NULL) {
		Tcl_AppendStringsToObj(resultPtr,
			(count > 0) ? ", or " : " or ", *entryPtr,
			(char *) NULL);
	    } else {
		Tcl_AppendStringsToObj(resultPtr, ", ", *entryPtr,
			(char *) NULL);
	    }
	}
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * SetIndexFromAny --
 *
 *	This procedure is called to convert a Tcl object to index
 *	internal form. However, this doesn't make sense (need to have a
 *	table of keywords in order to do the conversion) so the
 *	procedure always generates an error.
 *
 * Results:
 *	The return value is always TCL_ERROR, and an error message is
 *	left in interp's result if interp isn't NULL. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SetIndexFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr;	/* The object to convert. */
{
    Tcl_AppendToObj(Tcl_GetObjResult(interp),
	    "can't convert value to index except via Tcl_GetIndexFromObj API",
	    -1);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WrongNumArgs --
 *
 *	This procedure generates a "wrong # args" error message in an
 *	interpreter.  It is used as a utility function by many command
 *	procedures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is generated in interp's result object to
 *	indicate that a command was invoked with the wrong number of
 *	arguments.  The message has the form
 *		wrong # args: should be "foo bar additional stuff"
 *	where "foo" and "bar" are the initial objects in objv (objc
 *	determines how many of these are printed) and "additional stuff"
 *	is the contents of the message argument.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_WrongNumArgs(interp, objc, objv, message)
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments to print
					 * from objv. */
    Tcl_Obj *CONST objv[];		/* Initial argument objects, which
					 * should be included in the error
					 * message. */
    char *message;			/* Error message to print after the
					 * leading objects in objv. The
					 * message may be NULL. */
{
    Tcl_Obj *objPtr;
    char **tablePtr;
    int i;

    objPtr = Tcl_GetObjResult(interp);
    Tcl_AppendToObj(objPtr, "wrong # args: should be \"", -1);
    for (i = 0; i < objc; i++) {
	/*
	 * If the object is an index type use the index table which allows
	 * for the correct error message even if the subcommand was
	 * abbreviated.  Otherwise, just use the string rep.
	 */
	
	if (objv[i]->typePtr == &tclIndexType) {
	    tablePtr = ((char **) objv[i]->internalRep.twoPtrValue.ptr1);
	    Tcl_AppendStringsToObj(objPtr,
		    tablePtr[(int) objv[i]->internalRep.twoPtrValue.ptr2],
		    (char *) NULL);
	} else {
	    Tcl_AppendStringsToObj(objPtr, Tcl_GetString(objv[i]),
		    (char *) NULL);
	}
	if (i < (objc - 1)) {
	    Tcl_AppendStringsToObj(objPtr, " ", (char *) NULL);
	}
    }
    if (message) {
      Tcl_AppendStringsToObj(objPtr, " ", message, (char *) NULL);
    }
    Tcl_AppendStringsToObj(objPtr, "\"", (char *) NULL);
}
