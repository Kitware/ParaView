/* 
 * tclCmdMZ.c --
 *
 *	This file contains the top-level command routines for most of
 *	the Tcl built-in commands whose names begin with the letters
 *	M to Z.  It contains only commands in the generic core (i.e.
 *	those that don't depend much upon UNIX facilities).
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include "tclCompile.h"
#include "tclRegexp.h"

/*
 * Flag values used by Tcl_ScanObjCmd.
 */

#define SCAN_NOSKIP	0x1		  /* Don't skip blanks. */
#define SCAN_SUPPRESS	0x2		  /* Suppress assignment. */
#define SCAN_UNSIGNED	0x4		  /* Read an unsigned value. */
#define SCAN_WIDTH	0x8		  /* A width value was supplied. */

#define SCAN_SIGNOK	0x10		  /* A +/- character is allowed. */
#define SCAN_NODIGITS	0x20		  /* No digits have been scanned. */
#define SCAN_NOZERO	0x40		  /* No zero digits have been scanned. */
#define SCAN_XOK	0x80		  /* An 'x' is allowed. */
#define SCAN_PTOK	0x100		  /* Decimal point is allowed. */
#define SCAN_EXPOK	0x200		  /* An exponent is allowed. */

/*
 * Structure used to hold information about variable traces:
 */

typedef struct {
    int flags;			/* Operations for which Tcl command is
				 * to be invoked. */
    char *errMsg;		/* Error message returned from Tcl command,
				 * or NULL.  Malloc'ed. */
    size_t length;		/* Number of non-NULL chars. in command. */
    char command[4];		/* Space for Tcl command to invoke.  Actual
				 * size will be as large as necessary to
				 * hold command.  This field must be the
				 * last in the structure, so that it can
				 * be larger than 4 bytes. */
} TraceVarInfo;

/*
 * Forward declarations for procedures defined in this file:
 */

static char *		TraceVarProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PwdObjCmd --
 *
 *	This procedure is invoked to process the "pwd" Tcl command.
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
Tcl_PwdObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    Tcl_DString ds;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }

    if (Tcl_GetCwd(interp, &ds) == NULL) {
	return TCL_ERROR;
    }
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegexpObjCmd --
 *
 *	This procedure is invoked to process the "regexp" Tcl command.
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
Tcl_RegexpObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int i, indices, match, about;
    int cflags, eflags;
    Tcl_RegExp regExpr;
    Tcl_Obj *objPtr;
    Tcl_RegExpInfo info;
    static char *options[] = {
	"-indices",	"-nocase",	"-about",	"-expanded",
	"-line",	"-linestop",	"-lineanchor",
	"--",		(char *) NULL
    };
    enum options {
	REGEXP_INDICES, REGEXP_NOCASE,	REGEXP_ABOUT,	REGEXP_EXPANDED,
	REGEXP_LINE,	REGEXP_LINESTOP, REGEXP_LINEANCHOR,
	REGEXP_LAST
    };

    indices = 0;
    about = 0;
    cflags = TCL_REG_ADVANCED;
    eflags = 0;
    
    for (i = 1; i < objc; i++) {
	char *name;
	int index;

	name = Tcl_GetString(objv[i]);
	if (name[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "switch", TCL_EXACT,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum options) index) {
	    case REGEXP_INDICES: {
		indices = 1;
		break;
	    }
	    case REGEXP_NOCASE: {
		cflags |= TCL_REG_NOCASE;
		break;
	    }
	    case REGEXP_ABOUT: {
		about = 1;
		break;
	    }
	    case REGEXP_EXPANDED: {
		cflags |= TCL_REG_EXPANDED;
		break;
	    }
	    case REGEXP_LINE: {
		cflags |= TCL_REG_NEWLINE;
		break;
	    }
	    case REGEXP_LINESTOP: {
		cflags |= TCL_REG_NLSTOP;
		break;
	    }
	    case REGEXP_LINEANCHOR: {
		cflags |= TCL_REG_NLANCH;
		break;
	    }
	    case REGEXP_LAST: {
		i++;
		goto endOfForLoop;
	    }
	}
    }

    endOfForLoop:
    if (objc - i < 2 - about) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?switches? exp string ?matchVar? ?subMatchVar subMatchVar ...?");
	return TCL_ERROR;
    }
    objc -= i;
    objv += i;

    regExpr = Tcl_GetRegExpFromObj(interp, objv[0], cflags);
    if (regExpr == NULL) {
	return TCL_ERROR;
    }
    objPtr = objv[1];

    if (about) {
	if (TclRegAbout(interp, regExpr) < 0) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }

    match = Tcl_RegExpExecObj(interp, regExpr, objPtr, 0 /* offset */,
	    objc-2 /* nmatches */, eflags);

    if (match < 0) {
	return TCL_ERROR;
    }

    if (match == 0) {
	/*
	 * Set the interpreter's object result to an integer object w/
	 * value 0.
	 */
	
	Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
	return TCL_OK;
    }

    /*
     * If additional variable names have been specified, return
     * index information in those variables.
     */

    objc -= 2;
    objv += 2;

    Tcl_RegExpGetInfo(regExpr, &info);
    for (i = 0; i < objc; i++) {
	Tcl_Obj *varPtr, *valuePtr, *newPtr;
	
	varPtr = objv[i];
	if (indices) {
	    int start, end;
	    Tcl_Obj *objs[2];

	    if (i <= info.nsubs) {
		start = info.matches[i].start;
		end = info.matches[i].end;

		/*
		 * Adjust index so it refers to the last character in the
		 * match instead of the first character after the match.
		 */

		if (end >= 0) {
		    end--;
		}
	    } else {
		start = -1;
		end = -1;
	    }

	    objs[0] = Tcl_NewLongObj(start);
	    objs[1] = Tcl_NewLongObj(end);

	    newPtr = Tcl_NewListObj(2, objs);
	} else {
	    if (i <= info.nsubs) {
		newPtr = Tcl_GetRange(objPtr, info.matches[i].start,
			info.matches[i].end - 1);
	    } else {
		newPtr = Tcl_NewObj();
		
	    }
	}
	valuePtr = Tcl_ObjSetVar2(interp, varPtr, NULL, newPtr, 0);
	if (valuePtr == NULL) {
	    Tcl_DecrRefCount(newPtr);
	    Tcl_AppendResult(interp, "couldn't set variable \"",
		    Tcl_GetString(varPtr), "\"", (char *) NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Set the interpreter's object result to an integer object w/ value 1. 
     */
	
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegsubObjCmd --
 *
 *	This procedure is invoked to process the "regsub" Tcl command.
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
Tcl_RegsubObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int i, result, cflags, all, wlen, numMatches, offset;
    Tcl_RegExp regExpr;
    Tcl_Obj *resultPtr, *varPtr, *objPtr;
    Tcl_UniChar *wstring;
    char *subspec;

    static char *options[] = {
	"-all",		"-nocase",	"-expanded",
	"-line",	"-linestop",	"-lineanchor",
	"--",		NULL
    };
    enum options {
	REGSUB_ALL,	REGSUB_NOCASE,	REGSUB_EXPANDED,
	REGSUB_LINE,	REGSUB_LINESTOP, REGSUB_LINEANCHOR,
	REGSUB_LAST
    };

    cflags = TCL_REG_ADVANCED;
    all = 0;

    for (i = 1; i < objc; i++) {
	char *name;
	int index;
	
	name = Tcl_GetString(objv[i]);
	if (name[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "switch", TCL_EXACT,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum options) index) {
	    case REGSUB_ALL: {
		all = 1;
		break;
	    }
	    case REGSUB_NOCASE: {
		cflags |= TCL_REG_NOCASE;
		break;
	    }
	    case REGSUB_EXPANDED: {
		cflags |= TCL_REG_EXPANDED;
		break;
	    }
	    case REGSUB_LINE: {
		cflags |= TCL_REG_NEWLINE;
		break;
	    }
	    case REGSUB_LINESTOP: {
		cflags |= TCL_REG_NLSTOP;
		break;
	    }
	    case REGSUB_LINEANCHOR: {
		cflags |= TCL_REG_NLANCH;
		break;
	    }
	    case REGSUB_LAST: {
		i++;
		goto endOfForLoop;
	    }
	}
    }
    endOfForLoop:
    if (objc - i != 4) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?switches? exp string subSpec varName");
	return TCL_ERROR;
    }

    objv += i;

    regExpr = Tcl_GetRegExpFromObj(interp, objv[0], cflags);
    if (regExpr == NULL) {
	return TCL_ERROR;
    }

    result = TCL_OK;
    resultPtr = Tcl_NewObj();
    Tcl_IncrRefCount(resultPtr);

    objPtr = objv[1];
    wlen = Tcl_GetCharLength(objPtr);
    wstring = Tcl_GetUnicode(objPtr);
    subspec = Tcl_GetString(objv[2]);
    varPtr = objv[3];

    /*
     * The following loop is to handle multiple matches within the
     * same source string;  each iteration handles one match and its
     * corresponding substitution.  If "-all" hasn't been specified
     * then the loop body only gets executed once.
     */

    numMatches = 0;
    offset = 0;
    for (offset = 0; offset < wlen; ) {
	int start, end, subStart, subEnd, match;
	char *src, *firstChar;
	char c;
	Tcl_RegExpInfo info;

	/*
	 * The flags argument is set if string is part of a larger string,
	 * so that "^" won't match.
	 */

	match = Tcl_RegExpExecObj(interp, regExpr, objPtr, offset,
		10 /* matches */, ((offset > 0) ? TCL_REG_NOTBOL : 0));

	if (match < 0) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (match == 0) {
	    break;
	}
	numMatches++;

	/*
	 * Copy the portion of the source string before the match to the
	 * result variable.
	 */

	Tcl_RegExpGetInfo(regExpr, &info);
	start = info.matches[0].start;
	end = info.matches[0].end;
	Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, start);

	/*
	 * Append the subSpec argument to the variable, making appropriate
	 * substitutions.  This code is a bit hairy because of the backslash
	 * conventions and because the code saves up ranges of characters in
	 * subSpec to reduce the number of calls to Tcl_SetVar.
	 */

	src = subspec;
	firstChar = subspec;
	for (c = *src; c != '\0'; src++, c = *src) {
	    int index;
    
	    if (c == '&') {
		index = 0;
	    } else if (c == '\\') {
		c = src[1];
		if ((c >= '0') && (c <= '9')) {
		    index = c - '0';
		} else if ((c == '\\') || (c == '&')) {
		    Tcl_AppendToObj(resultPtr, firstChar, src - firstChar);
		    Tcl_AppendToObj(resultPtr, &c, 1);
		    firstChar = src + 2;
		    src++;
		    continue;
		} else {
		    continue;
		}
	    } else {
		continue;
	    }
	    if (firstChar != src) {
		Tcl_AppendToObj(resultPtr, firstChar, src - firstChar);
	    }
	    if (index <= info.nsubs) {
		subStart = info.matches[index].start;
		subEnd = info.matches[index].end;
		if ((subStart >= 0) && (subEnd >= 0)) {
		    Tcl_AppendUnicodeToObj(resultPtr,
			    wstring + offset + subStart, subEnd - subStart);
		}
	    }
	    if (*src == '\\') {
		src++;
	    }
	    firstChar = src + 1;
	}
	if (firstChar != src) {
	    Tcl_AppendToObj(resultPtr, firstChar, src - firstChar);
	}
	if (end == 0) {
	    /*
	     * Always consume at least one character of the input string
	     * in order to prevent infinite loops.
	     */

	    Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, 1);
	    offset++;
	}
	offset += end;
	if (!all) {
	    break;
	}
    }

    /*
     * Copy the portion of the source string after the last match to the
     * result variable.
     */

    if ((offset < wlen) || (numMatches == 0)) {
	Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, wlen - offset);
    }
    if (Tcl_ObjSetVar2(interp, varPtr, NULL, resultPtr, 0) == NULL) {
	Tcl_AppendResult(interp, "couldn't set variable \"",
		Tcl_GetString(varPtr), "\"", (char *) NULL);
	result = TCL_ERROR;
    } else {
	/*
	 * Set the interpreter's object result to an integer object holding the
	 * number of matches. 
	 */
	
	Tcl_SetIntObj(Tcl_GetObjResult(interp), numMatches);
    }

    done:
    Tcl_DecrRefCount(resultPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RenameObjCmd --
 *
 *	This procedure is invoked to process the "rename" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_RenameObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Arbitrary value passed to the command. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    char *oldName, *newName;
    
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "oldName newName");
	return TCL_ERROR;
    }

    oldName = Tcl_GetString(objv[1]);
    newName = Tcl_GetString(objv[2]);
    return TclRenameCommand(interp, oldName, newName);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ReturnObjCmd --
 *
 *	This object-based procedure is invoked to process the "return" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ReturnObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    int optionLen, argLen, code, result;

    if (iPtr->errorInfo != NULL) {
	ckfree(iPtr->errorInfo);
	iPtr->errorInfo = NULL;
    }
    if (iPtr->errorCode != NULL) {
	ckfree(iPtr->errorCode);
	iPtr->errorCode = NULL;
    }
    code = TCL_OK;
    
    for (objv++, objc--;  objc > 1;  objv += 2, objc -= 2) {
	char *option = Tcl_GetStringFromObj(objv[0], &optionLen);
	char *arg = Tcl_GetStringFromObj(objv[1], &argLen);
    	
	if (strcmp(option, "-code") == 0) {
	    register int c = arg[0];
	    if ((c == 'o') && (strcmp(arg, "ok") == 0)) {
		code = TCL_OK;
	    } else if ((c == 'e') && (strcmp(arg, "error") == 0)) {
		code = TCL_ERROR;
	    } else if ((c == 'r') && (strcmp(arg, "return") == 0)) {
		code = TCL_RETURN;
	    } else if ((c == 'b') && (strcmp(arg, "break") == 0)) {
		code = TCL_BREAK;
	    } else if ((c == 'c') && (strcmp(arg, "continue") == 0)) {
		code = TCL_CONTINUE;
	    } else {
		result = Tcl_GetIntFromObj((Tcl_Interp *) NULL, objv[1],
		        &code);
		if (result != TCL_OK) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			    "bad completion code \"",
			    Tcl_GetString(objv[1]),
			    "\": must be ok, error, return, break, ",
			    "continue, or an integer", (char *) NULL);
		    return result;
		}
	    }
	} else if (strcmp(option, "-errorinfo") == 0) {
	    iPtr->errorInfo =
		(char *) ckalloc((unsigned) (strlen(arg) + 1));
	    strcpy(iPtr->errorInfo, arg);
	} else if (strcmp(option, "-errorcode") == 0) {
	    iPtr->errorCode =
		(char *) ckalloc((unsigned) (strlen(arg) + 1));
	    strcpy(iPtr->errorCode, arg);
	} else {
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "bad option \"", option,
		    "\": must be -code, -errorcode, or -errorinfo",
		    (char *) NULL);
	    return TCL_ERROR;
	}
    }
    
    if (objc == 1) {
	/*
	 * Set the interpreter's object result. An inline version of
	 * Tcl_SetObjResult.
	 */

	Tcl_SetObjResult(interp, objv[0]);
    }
    iPtr->returnCode = code;
    return TCL_RETURN;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SourceObjCmd --
 *
 *	This procedure is invoked to process the "source" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_SourceObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    char *bytes;
    int result;
    
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "fileName");
	return TCL_ERROR;
    }

    bytes = Tcl_GetString(objv[1]);
    result = Tcl_EvalFile(interp, bytes);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SplitObjCmd --
 *
 *	This procedure is invoked to process the "split" Tcl command.
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
Tcl_SplitObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tcl_UniChar ch;
    int len;
    char *splitChars, *string, *end;
    int splitCharLen, stringLen;
    Tcl_Obj *listPtr, *objPtr;

    if (objc == 2) {
	splitChars = " \n\t\r";
	splitCharLen = 4;
    } else if (objc == 3) {
	splitChars = Tcl_GetStringFromObj(objv[2], &splitCharLen);
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?splitChars?");
	return TCL_ERROR;
    }

    string = Tcl_GetStringFromObj(objv[1], &stringLen);
    end = string + stringLen;
    listPtr = Tcl_GetObjResult(interp);
    
    if (stringLen == 0) {
	/*
	 * Do nothing.
	 */
    } else if (splitCharLen == 0) {
	/*
	 * Handle the special case of splitting on every character.
	 */

	for ( ; string < end; string += len) {
	    len = Tcl_UtfToUniChar(string, &ch);
	    objPtr = Tcl_NewStringObj(string, len);
	    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
	}
    } else {
	char *element, *p, *splitEnd;
	int splitLen;
	Tcl_UniChar splitChar;
	
	/*
	 * Normal case: split on any of a given set of characters.
	 * Discard instances of the split characters.
	 */

	splitEnd = splitChars + splitCharLen;

	for (element = string; string < end; string += len) {
	    len = Tcl_UtfToUniChar(string, &ch);
	    for (p = splitChars; p < splitEnd; p += splitLen) {
		splitLen = Tcl_UtfToUniChar(p, &splitChar);
		if (ch == splitChar) {
		    objPtr = Tcl_NewStringObj(element, string - element);
		    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
		    element = string + len;
		    break;
		}
	    }
	}
	objPtr = Tcl_NewStringObj(element, string - element);
	Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StringObjCmd --
 *
 *	This procedure is invoked to process the "string" Tcl command.
 *	See the user documentation for details on what it does.  Note
 *	that this command only functions correctly on properly formed
 *	Tcl UTF strings.
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
Tcl_StringObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int index, left, right;
    Tcl_Obj *resultPtr;
    char *string1, *string2;
    int length1, length2;
    static char *options[] = {
	"bytelength",	"compare",	"equal",	"first",
	"index",	"is",		"last",		"length",
	"map",		"match",	"range",	"repeat",
	"replace",	"tolower",	"toupper",	"totitle",
	"trim",		"trimleft",	"trimright",
	"wordend",	"wordstart",	(char *) NULL
    };
    enum options {
	STR_BYTELENGTH,	STR_COMPARE,	STR_EQUAL,	STR_FIRST,
	STR_INDEX,	STR_IS,		STR_LAST,	STR_LENGTH,
	STR_MAP,	STR_MATCH,	STR_RANGE,	STR_REPEAT,
	STR_REPLACE,	STR_TOLOWER,	STR_TOUPPER,	STR_TOTITLE,
	STR_TRIM,	STR_TRIMLEFT,	STR_TRIMRIGHT,
	STR_WORDEND,	STR_WORDSTART
    };	  

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
	return TCL_ERROR;
    }
    
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    resultPtr = Tcl_GetObjResult(interp);
    switch ((enum options) index) {
	case STR_EQUAL:
	case STR_COMPARE: {
	    int i, match, length, nocase = 0, reqlength = -1;

	    if (objc < 4 || objc > 7) {
	    str_cmp_args:
	        Tcl_WrongNumArgs(interp, 2, objv,
				 "?-nocase? ?-length int? string1 string2");
		return TCL_ERROR;
	    }

	    for (i = 2; i < objc-2; i++) {
		string2 = Tcl_GetStringFromObj(objv[i], &length2);
		if ((length2 > 1)
			&& strncmp(string2, "-nocase", (size_t) length2) == 0) {
		    nocase = 1;
		} else if ((length2 > 1)
			&& strncmp(string2, "-length", (size_t) length2) == 0) {
		    if (i+1 >= objc-2) {
			goto str_cmp_args;
		    }
		    if (Tcl_GetIntFromObj(interp, objv[++i],
			    &reqlength) != TCL_OK) {
			return TCL_ERROR;
		    }
		} else {
		    Tcl_AppendStringsToObj(resultPtr, "bad option \"",
			    string2, "\": must be -nocase or -length",
			    (char *) NULL);
		    return TCL_ERROR;
		}
	    }

	    string1 = Tcl_GetStringFromObj(objv[objc-2], &length1);
	    string2 = Tcl_GetStringFromObj(objv[objc-1], &length2);
	    /*
	     * This is the min length IN BYTES of the two strings
	     */
	    length = (length1 < length2) ? length1 : length2;

	    if (reqlength == 0) {
		/*
		 * Anything matches at 0 chars, right?
		 */

		match = 0;
	    } else if (nocase || ((reqlength > 0) && (reqlength <= length))) {
		/*
		 * with -nocase or -length we have to check true char length
		 * as it could be smaller than expected
		 */

		length1 = Tcl_NumUtfChars(string1, length1);
		length2 = Tcl_NumUtfChars(string2, length2);
		length = (length1 < length2) ? length1 : length2;

		/*
		 * Do the reqlength check again, against 0 as well for
		 * the benfit of nocase
		 */

		if ((reqlength > 0) && (reqlength < length)) {
		    length = reqlength;
		} else if (reqlength < 0) {
		    /*
		     * The requested length is negative, so we ignore it by
		     * setting it to the longer of the two lengths.
		     */

		    reqlength = (length1 > length2) ? length1 : length2;
		}
		if (nocase) {
		    match = Tcl_UtfNcasecmp(string1, string2,
			    (unsigned) length);
		} else {
		    match = Tcl_UtfNcmp(string1, string2,
			    (unsigned) length);
		}
		if ((match == 0) && (reqlength > length)) {
		    match = length1 - length2;
		}
	    } else {
		match = memcmp(string1, string2, (unsigned) length);
		if (match == 0) {
		    match = length1 - length2;
		}
	    }

	    if ((enum options) index == STR_EQUAL) {
		Tcl_SetIntObj(resultPtr, (match) ? 0 : 1);
	    } else {
		Tcl_SetIntObj(resultPtr, ((match > 0) ? 1 :
					  (match < 0) ? -1 : 0));
	    }
	    break;
	}
	case STR_FIRST: {
	    register char *p, *end;
	    int match, utflen, start;

	    if (objc < 4 || objc > 5) {
	        Tcl_WrongNumArgs(interp, 2, objv,
				 "string1 string2 ?startIndex?");
		return TCL_ERROR;
	    }

	    /*
	     * This algorithm fails on improperly formed UTF strings.
	     * We are searching string2 for the sequence string1.
	     */

	    match = -1;
	    start = 0;
	    utflen = -1;
	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    string2 = Tcl_GetStringFromObj(objv[3], &length2);

	    if (objc == 5) {
		/*
		 * If a startIndex is specified, we will need to fast forward
		 * to that point in the string before we think about a match
		 */
		utflen = Tcl_NumUtfChars(string2, length2);
		if (TclGetIntForIndex(interp, objv[4], utflen-1,
				      &start) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (start >= utflen) {
		    goto str_first_done;
		} else if (start > 0) {
		    if (length2 == utflen) {
			/* no unicode chars */
			string2 += start;
			length2 -= start;
		    } else {
			char *s = Tcl_UtfAtIndex(string2, start);
			length2 -= s - string2;
			string2 = s;
		    }
		}
	    }

	    if (length1 > 0) {
		end = string2 + length2 - length1 + 1;
		for (p = string2;  p < end;  p++) {
		    /*
		     * Scan forward to find the first character.
		     */

		    p = memchr(p, *string1, (unsigned) (end - p));
		    if (p == NULL) {
			break;
		    }
		    if (memcmp(string1, p, (unsigned) length1) == 0) {
			match = p - string2;
			break;
		    }
		}
	    }

	    /*
	     * Compute the character index of the matching string by
	     * counting the number of characters before the match.
	     */
	str_first_done:
	    if (match != -1) {
		if (objc == 4) {
		    match = Tcl_NumUtfChars(string2, match);
		} else if (length2 == utflen) {
		    /* no unicode chars */
		    match += start;
		} else {
		    match = start + Tcl_NumUtfChars(string2, match);
		}
	    }
	    Tcl_SetIntObj(resultPtr, match);
	    break;
	}
	case STR_INDEX: {
	    int index;
	    char buf[TCL_UTF_MAX];
	    Tcl_UniChar unichar;

	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string charIndex");
		return TCL_ERROR;
	    }

	    /*
	     * If we have a ByteArray object, avoid indexing in the
	     * Utf string since the byte array contains one byte per
	     * character.  Otherwise, use the Unicode string rep to
	     * get the index'th char.
	     */

	    if (objv[2]->typePtr == &tclByteArrayType) {

		string1 = (char *)Tcl_GetByteArrayFromObj(objv[2], &length1);

		if (TclGetIntForIndex(interp, objv[3], length1 - 1,
			&index) != TCL_OK) {
		    return TCL_ERROR;
		}
		Tcl_SetByteArrayObj(resultPtr,
			(unsigned char *)(&string1[index]), 1);
	    } else {
		string1 = Tcl_GetStringFromObj(objv[2], &length1);
		
		/*
		 * convert to Unicode internal rep to calulate what
		 * 'end' really means.
		 */

		length2 = Tcl_GetCharLength(objv[2]);
    
		if (TclGetIntForIndex(interp, objv[3], length2 - 1,
			&index) != TCL_OK) {
		    return TCL_ERROR;
		}
		if ((index >= 0) && (index < length2)) {
		    unichar = Tcl_GetUniChar(objv[2], index);
		    length2 = Tcl_UniCharToUtf((int)unichar, buf);
		    Tcl_SetStringObj(resultPtr, buf, length2);
		}
	    }
	    break;
	}
	case STR_IS: {
	    char *end;
	    Tcl_UniChar ch;

            /*
	     * The UniChar comparison function
	     */

	    int (*chcomp)_ANSI_ARGS_((int)) = NULL; 
	    int i, failat = 0, result = 1, strict = 0;
	    Tcl_Obj *objPtr, *failVarObj = NULL;

	    static char *isOptions[] = {
		"alnum",	"alpha",	"ascii",	"control",
		"boolean",	"digit",	"double",	"false",
		"graph",	"integer",	"lower",	"print",
		"punct",	"space",	"true",		"upper",
		"wordchar",	"xdigit",	(char *) NULL
	    };
	    enum isOptions {
		STR_IS_ALNUM,	STR_IS_ALPHA,	STR_IS_ASCII,	STR_IS_CONTROL,
		STR_IS_BOOL,	STR_IS_DIGIT,	STR_IS_DOUBLE,	STR_IS_FALSE,
		STR_IS_GRAPH,	STR_IS_INT,	STR_IS_LOWER,	STR_IS_PRINT,
		STR_IS_PUNCT,	STR_IS_SPACE,	STR_IS_TRUE,	STR_IS_UPPER,
		STR_IS_WORD,	STR_IS_XDIGIT
	    };

	    if (objc < 4 || objc > 7) {
		Tcl_WrongNumArgs(interp, 2, objv,
				 "class ?-strict? ?-failindex var? str");
		return TCL_ERROR;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[2], isOptions, "class", 0,
				    &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (objc != 4) {
		for (i = 3; i < objc-1; i++) {
		    string2 = Tcl_GetStringFromObj(objv[i], &length2);
		    if ((length2 > 1) &&
			strncmp(string2, "-strict", (size_t) length2) == 0) {
			strict = 1;
		    } else if ((length2 > 1) &&
			       strncmp(string2, "-failindex", (size_t) length2) == 0) {
			if (i+1 >= objc-1) {
			    Tcl_WrongNumArgs(interp, 3, objv,
					     "?-strict? ?-failindex var? str");
			    return TCL_ERROR;
			}
			failVarObj = objv[++i];
		    } else {
			Tcl_AppendStringsToObj(resultPtr, "bad option \"",
				string2, "\": must be -strict or -failindex",
				(char *) NULL);
			return TCL_ERROR;
		    }
		}
	    }

	    /*
	     * We get the objPtr so that we can short-cut for some classes
	     * by checking the object type (int and double), but we need
	     * the string otherwise, because we don't want any conversion
	     * of type occuring (as, for example, Tcl_Get*FromObj would do
	     */
	    objPtr = objv[objc-1];
	    string1 = Tcl_GetStringFromObj(objPtr, &length1);
	    if (length1 == 0) {
		if (strict) {
		    result = 0;
		}
		goto str_is_done;
	    }
	    end = string1 + length1;

	    /*
	     * When entering here, result == 1 and failat == 0
	     */
	    switch ((enum isOptions) index) {
		case STR_IS_ALNUM:
		    chcomp = Tcl_UniCharIsAlnum;
		    break;
		case STR_IS_ALPHA:
		    chcomp = Tcl_UniCharIsAlpha;
		    break;
		case STR_IS_ASCII:
		    for (; string1 < end; string1++, failat++) {
			/*
			 * This is a valid check in unicode, because all
			 * bytes < 0xC0 are single byte chars (but isascii
			 * limits that def'n to 0x80).
			 */
			if (*((unsigned char *)string1) >= 0x80) {
			    result = 0;
			    break;
			}
		    }
		    break;
		case STR_IS_BOOL:
		case STR_IS_TRUE:
		case STR_IS_FALSE:
		    if (objPtr->typePtr == &tclBooleanType) {
			if ((((enum isOptions) index == STR_IS_TRUE) &&
			     objPtr->internalRep.longValue == 0) ||
			    (((enum isOptions) index == STR_IS_FALSE) &&
			     objPtr->internalRep.longValue != 0)) {
			    result = 0;
			}
		    } else if ((Tcl_GetBoolean(NULL, string1, &i)
				== TCL_ERROR) ||
			       (((enum isOptions) index == STR_IS_TRUE) &&
				i == 0) ||
			       (((enum isOptions) index == STR_IS_FALSE) &&
				i != 0)) {
			result = 0;
		    }
		    break;
		case STR_IS_CONTROL:
		    chcomp = Tcl_UniCharIsControl;
		    break;
		case STR_IS_DIGIT:
		    chcomp = Tcl_UniCharIsDigit;
		    break;
		case STR_IS_DOUBLE: {
		    char *stop;

		    if ((objPtr->typePtr == &tclDoubleType) ||
			(objPtr->typePtr == &tclIntType)) {
			break;
		    }
		    /*
		     * This is adapted from Tcl_GetDouble
		     *
		     * The danger in this function is that
		     * "12345678901234567890" is an acceptable 'double',
		     * but will later be interp'd as an int by something
		     * like [expr].  Therefore, we check to see if it looks
		     * like an int, and if so we do a range check on it.
		     * If strtoul gets to the end, we know we either
		     * received an acceptable int, or over/underflow
		     */
		    if (TclLooksLikeInt(string1, length1)) {
			errno = 0;
			strtoul(string1, &stop, 0);
			if (stop == end) {
			    if (errno == ERANGE) {
				result = 0;
				failat = -1;
			    }
			    break;
			}
		    }
		    errno = 0;
		    strtod(string1, &stop); /* INTL: Tcl source. */
		    if (errno == ERANGE) {
			/*
			 * if (errno == ERANGE), then it was an over/underflow
			 * problem, but in this method, we only want to know
			 * yes or no, so bad flow returns 0 (false) and sets
			 * the failVarObj to the string length.
			 */
			result = 0;
			failat = -1;
		    } else if (stop == string1) {
			/*
			 * In this case, nothing like a number was found
			 */
			result = 0;
			failat = 0;
		    } else {
			/*
			 * Assume we sucked up one char per byte
			 * and then we go onto SPACE, since we are
			 * allowed trailing whitespace
			 */
			failat = stop - string1;
			string1 = stop;
			chcomp = Tcl_UniCharIsSpace;
		    }
		    break;
		}
		case STR_IS_GRAPH:
		    chcomp = Tcl_UniCharIsGraph;
		    break;
		case STR_IS_INT: {
		    char *stop;

		    if ((objPtr->typePtr == &tclIntType) ||
			(Tcl_GetInt(NULL, string1, &i) == TCL_OK)) {
			break;
		    }
		    /*
		     * Like STR_IS_DOUBLE, but we use strtoul.
		     * Since Tcl_GetInt already failed, we set result to 0.
		     */
		    result = 0;
		    errno = 0;
		    strtoul(string1, &stop, 0); /* INTL: Tcl source. */
		    if (errno == ERANGE) {
			/*
			 * if (errno == ERANGE), then it was an over/underflow
			 * problem, but in this method, we only want to know
			 * yes or no, so bad flow returns 0 (false) and sets
			 * the failVarObj to the string length.
			 */
			failat = -1;
		    } else if (stop == string1) {
			/*
			 * In this case, nothing like a number was found
			 */
			failat = 0;
		    } else {
			/*
			 * Assume we sucked up one char per byte
			 * and then we go onto SPACE, since we are
			 * allowed trailing whitespace
			 */
			failat = stop - string1;
			string1 = stop;
			chcomp = Tcl_UniCharIsSpace;
		    }
		    break;
		}
		case STR_IS_LOWER:
		    chcomp = Tcl_UniCharIsLower;
		    break;
		case STR_IS_PRINT:
		    chcomp = Tcl_UniCharIsPrint;
		    break;
		case STR_IS_PUNCT:
		    chcomp = Tcl_UniCharIsPunct;
		    break;
		case STR_IS_SPACE:
		    chcomp = Tcl_UniCharIsSpace;
		    break;
		case STR_IS_UPPER:
		    chcomp = Tcl_UniCharIsUpper;
		    break;
		case STR_IS_WORD:
		    chcomp = Tcl_UniCharIsWordChar;
		    break;
		case STR_IS_XDIGIT: {
		    for (; string1 < end; string1++, failat++) {
			/* INTL: We assume unicode is bad for this class */
			if ((*((unsigned char *)string1) >= 0xC0) ||
			    !isxdigit(*(unsigned char *)string1)) {
			    result = 0;
			    break;
			}
		    }
		    break;
		}
	    }
	    if (chcomp != NULL) {
		for (; string1 < end; string1 += length2, failat++) {
		    length2 = Tcl_UtfToUniChar(string1, &ch);
		    if (!chcomp(ch)) {
			result = 0;
			break;
		    }
		}
	    }
	str_is_done:
	    /*
	     * Only set the failVarObj when we will return 0
	     * and we have indicated a valid fail index (>= 0)
	     */
	    if ((result == 0) && (failVarObj != NULL) &&
		Tcl_ObjSetVar2(interp, failVarObj, NULL, Tcl_NewIntObj(failat),
			       TCL_LEAVE_ERR_MSG) == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetBooleanObj(resultPtr, result);
	    break;
	}
	case STR_LAST: {
	    register char *p;
	    int match, utflen, start;

	    if (objc < 4 || objc > 5) {
	        Tcl_WrongNumArgs(interp, 2, objv,
				 "string1 string2 ?startIndex?");
		return TCL_ERROR;
	    }

	    /*
	     * This algorithm fails on improperly formed UTF strings.
	     */

	    match = -1;
	    start = 0;
	    utflen = -1;
	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    string2 = Tcl_GetStringFromObj(objv[3], &length2);

	    if (objc == 5) {
		/*
		 * If a startIndex is specified, we will need to restrict
		 * the string range to that char index in the string
		 */
		utflen = Tcl_NumUtfChars(string2, length2);
		if (TclGetIntForIndex(interp, objv[4], utflen-1,
				      &start) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (start < 0) {
		    goto str_last_done;
		} else if (start < utflen) {
		    if (length2 == utflen) {
			/* no unicode chars */
			p = string2 + start + 1 - length1;
		    } else {
			p = Tcl_UtfAtIndex(string2, start+1) - length1;
		    }
		} else {
		    p = string2 + length2 - length1;
		}
	    } else {
		p = string2 + length2 - length1;
	    }

	    if (length1 > 0) {
		for (;  p >= string2;  p--) {
		    /*
		     * Scan backwards to find the first character.
		     */

		    while ((p != string2) && (*p != *string1)) {
			p--;
		    }
		    if (memcmp(string1, p, (unsigned) length1) == 0) {
			match = p - string2;
			break;
		    }
		}
	    }

	    /*
	     * Compute the character index of the matching string by counting
	     * the number of characters before the match.
	     */
	str_last_done:
	    if (match != -1) {
		if ((objc == 4) || (length2 != utflen)) {
		    /* only check when we've got unicode chars */
		    match = Tcl_NumUtfChars(string2, match);
		}
	    }
	    Tcl_SetIntObj(resultPtr, match);
	    break;
	}
	case STR_BYTELENGTH:
	case STR_LENGTH: {
	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string");
		return TCL_ERROR;
	    }

	    if ((enum options) index == STR_BYTELENGTH) {
		(void) Tcl_GetStringFromObj(objv[2], &length1);
		Tcl_SetIntObj(resultPtr, length1);
	    } else {
		/*
		 * If we have a ByteArray object, avoid recomputing the
		 * string since the byte array contains one byte per
		 * character.  Otherwise, use the Unicode string rep to
		 * calculate the length.
		 */

		if (objv[2]->typePtr == &tclByteArrayType) {
		    (void) Tcl_GetByteArrayFromObj(objv[2], &length1);
		    Tcl_SetIntObj(resultPtr, length1);
		} else {
		    Tcl_SetIntObj(resultPtr,
			    Tcl_GetCharLength(objv[2]));
		}
	    }
	    break;
	}
	case STR_MAP: {
	    int uselen, mapElemc, len, nocase = 0;
	    Tcl_Obj **mapElemv;
	    char *end;
	    Tcl_UniChar ch;
	    int (*str_comp_fn)();

	    if (objc < 4 || objc > 5) {
	        Tcl_WrongNumArgs(interp, 2, objv, "?-nocase? charMap string");
		return TCL_ERROR;
	    }

	    if (objc == 5) {
		string2 = Tcl_GetStringFromObj(objv[2], &length2);
		if ((length2 > 1) &&
		    strncmp(string2, "-nocase", (size_t) length2) == 0) {
		    nocase = 1;
		} else {
		    Tcl_AppendStringsToObj(resultPtr, "bad option \"",
					   string2, "\": must be -nocase",
					   (char *) NULL);
		    return TCL_ERROR;
		}
	    }

	    if (Tcl_ListObjGetElements(interp, objv[objc-2], &mapElemc,
				       &mapElemv) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (mapElemc == 0) {
		/*
		 * empty charMap, just return whatever string was given
		 */
		Tcl_SetObjResult(interp, objv[objc-1]);
	    } else if (mapElemc & 1) {
		/*
		 * The charMap must be an even number of key/value items
		 */
		Tcl_SetStringObj(resultPtr, "char map list unbalanced", -1);
		return TCL_ERROR;
	    }
	    string1 = Tcl_GetStringFromObj(objv[objc-1], &length1);
	    if (length1 == 0) {
		break;
	    }
	    end = string1 + length1;

	    if (nocase) {
		length1 = Tcl_NumUtfChars(string1, length1);
		str_comp_fn = Tcl_UtfNcasecmp;
	    } else {
		str_comp_fn = memcmp;
	    }

	    for ( ; string1 < end; string1 += len) {
		len = Tcl_UtfToUniChar(string1, &ch);
		for (index = 0; index < mapElemc; index +=2) {
		    /*
		     * Get the key string to match on
		     */
		    string2 = Tcl_GetStringFromObj(mapElemv[index],
						   &length2);
		    if (nocase) {
			uselen = Tcl_NumUtfChars(string2, length2);
		    } else {
			uselen = length2;
		    }
		    if ((uselen > 0) && (uselen <= length1) &&
			(str_comp_fn(string2, string1, uselen) == 0)) {
			/*
			 * Adjust len to be full length of matched string
			 * it has to be the BYTE length
			 */
			len = length2;
			/*
			 * Change string2 and length2 to the map value
			 */
			string2 = Tcl_GetStringFromObj(mapElemv[index+1],
						       &length2);
			Tcl_AppendToObj(resultPtr, string2, length2);
			break;
		    }
		}
		if (index == mapElemc) {
		    /*
		     * No match was found, put the char onto result
		     */
		    Tcl_AppendToObj(resultPtr, string1, len);
		}
		/*
		 * in nocase, length1 is in chars
		 * otherwise it is in bytes
		 */
		if (nocase) {
		    length1--;
		} else {
		    length1 -= len;
		}
	    }
	    break;
	}
	case STR_MATCH: {
	    int nocase = 0;

	    if (objc < 4 || objc > 5) {
	        Tcl_WrongNumArgs(interp, 2, objv, "?-nocase? pattern string");
		return TCL_ERROR;
	    }

	    if (objc == 5) {
		string2 = Tcl_GetStringFromObj(objv[2], &length2);
		if ((length2 > 1) &&
		    strncmp(string2, "-nocase", (size_t) length2) == 0) {
		    nocase = 1;
		} else {
		    Tcl_AppendStringsToObj(resultPtr, "bad option \"",
					   string2, "\": must be -nocase",
					   (char *) NULL);
		    return TCL_ERROR;
		}
	    }

	    Tcl_SetBooleanObj(resultPtr,
			      Tcl_StringCaseMatch(Tcl_GetString(objv[objc-1]),
						  Tcl_GetString(objv[objc-2]),
						  nocase));
	    break;
	}
	case STR_RANGE: {
	    int first, last;

	    if (objc != 5) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string first last");
		return TCL_ERROR;
	    }

	    /*
	     * If we have a ByteArray object, avoid indexing in the
	     * Utf string since the byte array contains one byte per
	     * character.  Otherwise, use the Unicode string rep to
	     * get the range.
	     */

	    if (objv[2]->typePtr == &tclByteArrayType) {

		string1 = (char *)Tcl_GetByteArrayFromObj(objv[2], &length1);

		if (TclGetIntForIndex(interp, objv[3], length1 - 1,
			&first) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (TclGetIntForIndex(interp, objv[4], length1 - 1,
			&last) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (first < 0) {
		    first = 0;
		}
		if (last >= length1 - 1) {
		    last = length1 - 1;
		}
		if (last >= first) {
		    int numBytes = last - first + 1;
		    resultPtr = Tcl_NewByteArrayObj(
				(unsigned char *) &string1[first], numBytes);
		    Tcl_SetObjResult(interp, resultPtr);
		}
	    } else {
		string1 = Tcl_GetStringFromObj(objv[2], &length1);
		
		/*
		 * Convert to Unicode internal rep to calulate length and
		 * create a result object.
		 */

		length2 = Tcl_GetCharLength(objv[2]) - 1;
    
		if (TclGetIntForIndex(interp, objv[3], length2,
			&first) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (TclGetIntForIndex(interp, objv[4], length2,
			&last) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (first < 0) {
		    first = 0;
		}
		if (last >= length2) {
		    last = length2;
		}
		if (last >= first) {
		    resultPtr = Tcl_GetRange(objv[2], first, last);
		    Tcl_SetObjResult(interp, resultPtr);
		}
	    }
	    break;
	}
	case STR_REPEAT: {
	    int count;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "string count");
		return TCL_ERROR;
	    }

	    if (Tcl_GetIntFromObj(interp, objv[3], &count) != TCL_OK) {
		return TCL_ERROR;
	    }

	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    if (length1 > 0) {
		for (index = 0; index < count; index++) {
		    Tcl_AppendToObj(resultPtr, string1, length1);
		}
	    }
	    break;
	}
	case STR_REPLACE: {
	    int first, last;

	    if (objc < 5 || objc > 6) {
	        Tcl_WrongNumArgs(interp, 2, objv,
				 "string first last ?string?");
		return TCL_ERROR;
	    }

	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    length1 = Tcl_NumUtfChars(string1, length1) - 1;
	    if (TclGetIntForIndex(interp, objv[3], length1,
				  &first) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (TclGetIntForIndex(interp, objv[4], length1,
		    &last) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((last < first) || (first > length1) || (last < 0)) {
		Tcl_SetObjResult(interp, objv[2]);
	    } else {
		char *start, *end;

		if (first < 0) {
		    first = 0;
		}
		start = Tcl_UtfAtIndex(string1, first);
		end = Tcl_UtfAtIndex(start, ((last > length1) ? length1 : last)
				     - first + 1);
	        Tcl_SetStringObj(resultPtr, string1, start - string1);
		if (objc == 6) {
		    Tcl_AppendObjToObj(resultPtr, objv[5]);
		}
		if (last < length1) {
		    Tcl_AppendToObj(resultPtr, end, -1);
		}
	    }
	    break;
	}
	case STR_TOLOWER:
	case STR_TOUPPER:
	case STR_TOTITLE:
	    if (objc < 3 || objc > 5) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string ?first? ?last?");
		return TCL_ERROR;
	    }

	    string1 = Tcl_GetStringFromObj(objv[2], &length1);

	    if (objc == 3) {
		/*
		 * Since the result object is not a shared object, it is
		 * safe to copy the string into the result and do the
		 * conversion in place.  The conversion may change the length
		 * of the string, so reset the length after conversion.
		 */

		Tcl_SetStringObj(resultPtr, string1, length1);
		if ((enum options) index == STR_TOLOWER) {
		    length1 = Tcl_UtfToLower(Tcl_GetString(resultPtr));
		} else if ((enum options) index == STR_TOUPPER) {
		    length1 = Tcl_UtfToUpper(Tcl_GetString(resultPtr));
		} else {
		    length1 = Tcl_UtfToTitle(Tcl_GetString(resultPtr));
		}
		Tcl_SetObjLength(resultPtr, length1);
	    } else {
		int first, last;
		char *start, *end;

		length1 = Tcl_NumUtfChars(string1, length1) - 1;
		if (TclGetIntForIndex(interp, objv[3], length1,
				      &first) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (first < 0) {
		    first = 0;
		}
		last = first;
		if ((objc == 5) && (TclGetIntForIndex(interp, objv[4], length1,
						      &last) != TCL_OK)) {
		    return TCL_ERROR;
		}
		if (last >= length1) {
		    last = length1;
		}
		if (last < first) {
		    Tcl_SetObjResult(interp, objv[2]);
		    break;
		}
		start = Tcl_UtfAtIndex(string1, first);
		end = Tcl_UtfAtIndex(start, last - first + 1);
		length2 = end-start;
		string2 = ckalloc((size_t) length2+1);
		memcpy(string2, start, (size_t) length2);
		string2[length2] = '\0';
		if ((enum options) index == STR_TOLOWER) {
		    length2 = Tcl_UtfToLower(string2);
		} else if ((enum options) index == STR_TOUPPER) {
		    length2 = Tcl_UtfToUpper(string2);
		} else {
		    length2 = Tcl_UtfToTitle(string2);
		}
		Tcl_SetStringObj(resultPtr, string1, start - string1);
		Tcl_AppendToObj(resultPtr, string2, length2);
		Tcl_AppendToObj(resultPtr, end, -1);
		ckfree(string2);
	    }
	    break;

	case STR_TRIM: {
	    Tcl_UniChar ch, trim;
	    register char *p, *end;
	    char *check, *checkEnd;
	    int offset;

	    left = 1;
	    right = 1;

	    dotrim:
	    if (objc == 4) {
		string2 = Tcl_GetStringFromObj(objv[3], &length2);
	    } else if (objc == 3) {
		string2 = " \t\n\r";
		length2 = strlen(string2);
	    } else {
	        Tcl_WrongNumArgs(interp, 2, objv, "string ?chars?");
		return TCL_ERROR;
	    }
	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    checkEnd = string2 + length2;

	    if (left) {
		end = string1 + length1;
		/*
		 * The outer loop iterates over the string.  The inner
		 * loop iterates over the trim characters.  The loops
		 * terminate as soon as a non-trim character is discovered
		 * and string1 is left pointing at the first non-trim
		 * character.
		 */

		for (p = string1; p < end; p += offset) {
		    offset = Tcl_UtfToUniChar(p, &ch);
		    
		    for (check = string2; ; ) {
			if (check >= checkEnd) {
			    p = end;
			    break;
			}
			check += Tcl_UtfToUniChar(check, &trim);
			if (ch == trim) {
			    length1 -= offset;
			    string1 += offset;
			    break;
			}
		    }
		}
	    }
	    if (right) {
	        end = string1;

		/*
		 * The outer loop iterates over the string.  The inner
		 * loop iterates over the trim characters.  The loops
		 * terminate as soon as a non-trim character is discovered
		 * and length1 marks the last non-trim character.
		 */

		for (p = string1 + length1; p > end; ) {
		    p = Tcl_UtfPrev(p, string1);
		    offset = Tcl_UtfToUniChar(p, &ch);
		    for (check = string2; ; ) {
		        if (check >= checkEnd) {
			    p = end;
			    break;
			}
			check += Tcl_UtfToUniChar(check, &trim);
			if (ch == trim) {
			    length1 -= offset;
			    break;
			}
		    }
		}
	    }
	    Tcl_SetStringObj(resultPtr, string1, length1);
	    break;
	}
	case STR_TRIMLEFT: {
	    left = 1;
	    right = 0;
	    goto dotrim;
	}
	case STR_TRIMRIGHT: {
	    left = 0;
	    right = 1;
	    goto dotrim;
	}
	case STR_WORDEND: {
	    int cur;
	    Tcl_UniChar ch;
	    char *p, *end;
	    int numChars;
	    
	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string index");
		return TCL_ERROR;
	    }

	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    numChars = Tcl_NumUtfChars(string1, length1);
	    if (TclGetIntForIndex(interp, objv[3], numChars-1,
				  &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (index < 0) {
		index = 0;
	    }
	    if (index < numChars) {
		p = Tcl_UtfAtIndex(string1, index);
		end = string1+length1;
		for (cur = index; p < end; cur++) {
		    p += Tcl_UtfToUniChar(p, &ch);
		    if (!Tcl_UniCharIsWordChar(ch)) {
			break;
		    }
		}
		if (cur == index) {
		    cur++;
		}
	    } else {
		cur = numChars;
	    }
	    Tcl_SetIntObj(resultPtr, cur);
	    break;
	}
	case STR_WORDSTART: {
	    int cur;
	    Tcl_UniChar ch;
	    char *p;
	    int numChars;
	    
	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string index");
		return TCL_ERROR;
	    }

	    string1 = Tcl_GetStringFromObj(objv[2], &length1);
	    numChars = Tcl_NumUtfChars(string1, length1);
	    if (TclGetIntForIndex(interp, objv[3], numChars-1,
				  &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (index >= numChars) {
		index = numChars - 1;
	    }
	    cur = 0;
	    if (index > 0) {
		p = Tcl_UtfAtIndex(string1, index);
	        for (cur = index; cur >= 0; cur--) {
		    Tcl_UtfToUniChar(p, &ch);
		    if (!Tcl_UniCharIsWordChar(ch)) {
			break;
		    }
		    p = Tcl_UtfPrev(p, string1);
		}
		if (cur != index) {
		    cur += 1;
		}
	    }
	    Tcl_SetIntObj(resultPtr, cur);
	    break;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SubstObjCmd --
 *
 *	This procedure is invoked to process the "subst" Tcl command.
 *	See the user documentation for details on what it does.  This
 *	command is an almost direct copy of an implementation by
 *	Andrew Payne.
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
Tcl_SubstObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];       	/* Argument objects. */
{
    static char *substOptions[] = {
	"-nobackslashes", "-nocommands", "-novariables", (char *) NULL
    };
    enum substOptions {
	SUBST_NOBACKSLASHES,      SUBST_NOCOMMANDS,       SUBST_NOVARS
    };
    Interp *iPtr = (Interp *) interp;
    Tcl_DString result;
    char *p, *old, *value;
    int optionIndex, code, count, doVars, doCmds, doBackslashes, i;

    /*
     * Parse command-line options.
     */

    doVars = doCmds = doBackslashes = 1;
    for (i = 1; i < (objc-1); i++) {
	p = Tcl_GetString(objv[i]);
	if (*p != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], substOptions,
		"switch", 0, &optionIndex) != TCL_OK) {

	    return TCL_ERROR;
	}
	switch (optionIndex) {
	    case SUBST_NOBACKSLASHES: {
		doBackslashes = 0;
		break;
	    }
	    case SUBST_NOCOMMANDS: {
		doCmds = 0;
		break;
	    }
	    case SUBST_NOVARS: {
		doVars = 0;
		break;
	    }
	    default: {
		panic("Tcl_SubstObjCmd: bad option index to SubstOptions");
	    }
	}
    }
    if (i != (objc-1)) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-nobackslashes? ?-nocommands? ?-novariables? string");
	return TCL_ERROR;
    }

    /*
     * Scan through the string one character at a time, performing
     * command, variable, and backslash substitutions.
     */

    Tcl_DStringInit(&result);
    old = p = Tcl_GetString(objv[i]);
    while (*p != 0) {
	switch (*p) {
	    case '\\':
		if (doBackslashes) {
		    char buf[TCL_UTF_MAX];

		    if (p != old) {
			Tcl_DStringAppend(&result, old, p-old);
		    }
		    Tcl_DStringAppend(&result, buf,
			    Tcl_UtfBackslash(p, &count, buf));
		    p += count;
		    old = p;
		} else {
		    p++;
		}
		break;

	    case '$':
		if (doVars) {
		    if (p != old) {
			Tcl_DStringAppend(&result, old, p-old);
		    }
		    value = Tcl_ParseVar(interp, p, &p);
		    if (value == NULL) {
			Tcl_DStringFree(&result);
			return TCL_ERROR;
		    }
		    Tcl_DStringAppend(&result, value, -1);
		    old = p;
		} else {
		    p++;
		}
		break;

	    case '[':
		if (doCmds) {
		    if (p != old) {
			Tcl_DStringAppend(&result, old, p-old);
		    }
		    iPtr->evalFlags = TCL_BRACKET_TERM;
		    code = Tcl_Eval(interp, p+1);
		    if (code == TCL_ERROR) {
			Tcl_DStringFree(&result);
			return code;
		    }
		    old = p = (p+1 + iPtr->termOffset+1);
		    Tcl_DStringAppend(&result, iPtr->result, -1);
		    Tcl_ResetResult(interp);
		} else {
		    p++;
		}
		break;

	    default:
		p++;
		break;
	}
    }
    if (p != old) {
	Tcl_DStringAppend(&result, old, p-old);
    }
    Tcl_DStringResult(interp, &result);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SwitchObjCmd --
 *
 *	This object-based procedure is invoked to process the "switch" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_SwitchObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int i, j, index, mode, matched, result, splitObjs, seenComment;
    char *string, *pattern;
    Tcl_Obj *stringObj;
    static char *options[] = {
	"-exact",	"-glob",	"-regexp",	"--", 
	NULL
    };
    enum options {
	OPT_EXACT,	OPT_GLOB,	OPT_REGEXP,	OPT_LAST
    };

    mode = OPT_EXACT;
    for (i = 1; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (string[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0, 
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_LAST) {
	    i++;
	    break;
	}
	mode = index;
    }

    if (objc - i < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?switches? string pattern body ... ?default body?");
	return TCL_ERROR;
    }

    stringObj = objv[i];
    objc -= i + 1;
    objv += i + 1;

    /*
     * If all of the pattern/command pairs are lumped into a single
     * argument, split them out again.
     */

    splitObjs = 0;
    if (objc == 1) {
	Tcl_Obj **listv;

	if (Tcl_ListObjGetElements(interp, objv[0], &objc, &listv) != TCL_OK) {
	    return TCL_ERROR;
	}
	objv = listv;
	splitObjs = 1;
    }

    seenComment = 0;
    for (i = 0; i < objc; i += 2) {
	if (i == objc - 1) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
	            "extra switch pattern with no body", -1);

	    /*
	     * Check if this can be due to a badly placed comment
	     * in the switch block
	     */

	    if (splitObjs && seenComment) {
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
			", this may be due to a comment incorrectly placed outside of a switch body - see the \"switch\" documentation", -1);
	    }

	    return TCL_ERROR;
	}

	/*
	 * See if the pattern matches the string.
	 */

	pattern = Tcl_GetString(objv[i]);

	/*
	 * The following is an heuristic to detect the infamous
	 * "comment in switch" error: just check if a pattern
	 * begins with '#'.
	 */

	if (splitObjs && *pattern == '#') {
	    seenComment = 1;
	}

	matched = 0;
	if ((i == objc - 2) 
		&& (*pattern == 'd') 
		&& (strcmp(pattern, "default") == 0)) {
	    matched = 1;
	} else {
	    switch (mode) {
		case OPT_EXACT:
		    matched = (strcmp(Tcl_GetString(stringObj), pattern) == 0);
		    break;
		case OPT_GLOB:
		    matched = Tcl_StringMatch(Tcl_GetString(stringObj),
			    pattern);
		    break;
		case OPT_REGEXP:
		    matched = Tcl_RegExpMatchObj(interp, stringObj, objv[i]);
		    if (matched < 0) {
			return TCL_ERROR;
		    }
		    break;
	    }
	}
	if (matched == 0) {
	    continue;
	}

	/*
	 * We've got a match. Find a body to execute, skipping bodies
	 * that are "-".
	 */

	for (j = i + 1; ; j += 2) {
	    if (j >= objc) {
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"no body specified for pattern \"", pattern,
			"\"", (char *) NULL);
		return TCL_ERROR;
	    }
	    if (strcmp(Tcl_GetString(objv[j]), "-") != 0) {
		break;
	    }
	}
	result = Tcl_EvalObjEx(interp, objv[j], 0);
	if (result == TCL_ERROR) {
	    char msg[100 + TCL_INTEGER_SPACE];

	    sprintf(msg, "\n    (\"%.50s\" arm line %d)", pattern,
		    interp->errorLine);
	    Tcl_AddObjErrorInfo(interp, msg, -1);
	}
	return result;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TimeObjCmd --
 *
 *	This object-based procedure is invoked to process the "time" Tcl
 *	command.  See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_TimeObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register Tcl_Obj *objPtr;
    register int i, result;
    int count;
    double totalMicroSec;
    Tcl_Time start, stop;
    char buf[100];

    if (objc == 2) {
	count = 1;
    } else if (objc == 3) {
	result = Tcl_GetIntFromObj(interp, objv[2], &count);
	if (result != TCL_OK) {
	    return result;
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "command ?count?");
	return TCL_ERROR;
    }
    
    objPtr = objv[1];
    i = count;
    TclpGetTime(&start);
    while (i-- > 0) {
	result = Tcl_EvalObjEx(interp, objPtr, 0);
	if (result != TCL_OK) {
	    return result;
	}
    }
    TclpGetTime(&stop);
    
    totalMicroSec =
	(stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    sprintf(buf, "%.0f microseconds per iteration",
	((count <= 0) ? 0 : totalMicroSec/count));
    Tcl_ResetResult(interp);
    Tcl_AppendToObj(Tcl_GetObjResult(interp), buf, -1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceObjCmd --
 *
 *	This procedure is invoked to process the "trace" Tcl command.
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
Tcl_TraceObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int optionIndex, commandLength;
    char *name, *rwuOps, *command, *p;
    size_t length;
    static char *traceOptions[] = {
	"variable", "vdelete", "vinfo", (char *) NULL
    };
    enum traceOptions {
	TRACE_VARIABLE,       TRACE_VDELETE,      TRACE_VINFO
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option [arg arg ...]");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], traceOptions,
		"option", 0, &optionIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum traceOptions) optionIndex) {
	    case TRACE_VARIABLE: {
		int flags;
		TraceVarInfo *tvarPtr;
		if (objc != 5) {
		    Tcl_WrongNumArgs(interp, 2, objv, "name ops command");
		    return TCL_ERROR;
		}

		flags = 0;
		rwuOps = Tcl_GetString(objv[3]);
		for (p = rwuOps; *p != 0; p++) {
		    if (*p == 'r') {
			flags |= TCL_TRACE_READS;
		    } else if (*p == 'w') {
			flags |= TCL_TRACE_WRITES;
		    } else if (*p == 'u') {
			flags |= TCL_TRACE_UNSETS;
		    } else {
			goto badOps;
		    }
		}
		if (flags == 0) {
		    goto badOps;
		}

		command = Tcl_GetStringFromObj(objv[4], &commandLength);
		length = (size_t) commandLength;
		tvarPtr = (TraceVarInfo *) ckalloc((unsigned)
			(sizeof(TraceVarInfo) - sizeof(tvarPtr->command)
				+ length + 1));
		tvarPtr->flags = flags;
		tvarPtr->errMsg = NULL;
		tvarPtr->length = length;
		flags |= TCL_TRACE_UNSETS;
		strcpy(tvarPtr->command, command);
		name = Tcl_GetString(objv[2]);
		if (Tcl_TraceVar(interp, name, flags, TraceVarProc,
			(ClientData) tvarPtr) != TCL_OK) {
		    ckfree((char *) tvarPtr);
		    return TCL_ERROR;
		}
		break;
	    }
	    case TRACE_VDELETE: {
		int flags;
		TraceVarInfo *tvarPtr;
		ClientData clientData;

		if (objc != 5) {
		    Tcl_WrongNumArgs(interp, 2, objv, "name ops command");
		    return TCL_ERROR;
		}

		flags = 0;
		rwuOps = Tcl_GetString(objv[3]);
		for (p = rwuOps; *p != 0; p++) {
		    if (*p == 'r') {
			flags |= TCL_TRACE_READS;
		    } else if (*p == 'w') {
			flags |= TCL_TRACE_WRITES;
		    } else if (*p == 'u') {
			flags |= TCL_TRACE_UNSETS;
		    } else {
			goto badOps;
		    }
		}
		if (flags == 0) {
		    goto badOps;
		}

		/*
		 * Search through all of our traces on this variable to
		 * see if there's one with the given command.  If so, then
		 * delete the first one that matches.
		 */
		
		command = Tcl_GetStringFromObj(objv[4], &commandLength);
		length = (size_t) commandLength;
		clientData = 0;
		name = Tcl_GetString(objv[2]);
		while ((clientData = Tcl_VarTraceInfo(interp, name, 0,
			TraceVarProc, clientData)) != 0) {
		    tvarPtr = (TraceVarInfo *) clientData;
		    if ((tvarPtr->length == length) && (tvarPtr->flags == flags)
			    && (strncmp(command, tvarPtr->command,
				    (size_t) length) == 0)) {
			Tcl_UntraceVar(interp, name, flags | TCL_TRACE_UNSETS,
				TraceVarProc, clientData);
			if (tvarPtr->errMsg != NULL) {
			    ckfree(tvarPtr->errMsg);
			}
			ckfree((char *) tvarPtr);
			break;
		    }
		}
		break;
	    }
	    case TRACE_VINFO: {
		ClientData clientData;
		char ops[4];
		Tcl_Obj *resultListPtr, *pairObjPtr, *elemObjPtr;

		if (objc != 3) {
		    Tcl_WrongNumArgs(interp, 2, objv, "name");
		    return TCL_ERROR;
		}
		resultListPtr = Tcl_GetObjResult(interp);
		clientData = 0;
		name = Tcl_GetString(objv[2]);
		while ((clientData = Tcl_VarTraceInfo(interp, name, 0,
			TraceVarProc, clientData)) != 0) {

		    TraceVarInfo *tvarPtr = (TraceVarInfo *) clientData;

		    pairObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
		    p = ops;
		    if (tvarPtr->flags & TCL_TRACE_READS) {
			*p = 'r';
			p++;
		    }
		    if (tvarPtr->flags & TCL_TRACE_WRITES) {
			*p = 'w';
			p++;
		    }
		    if (tvarPtr->flags & TCL_TRACE_UNSETS) {
			*p = 'u';
			p++;
		    }
		    *p = '\0';

		    /*
		     * Build a pair (2-item list) with the ops string as
		     * the first obj element and the tvarPtr->command string
		     * as the second obj element.  Append the pair (as an
		     * element) to the end of the result object list.
		     */

		    elemObjPtr = Tcl_NewStringObj(ops, -1);
		    Tcl_ListObjAppendElement(NULL, pairObjPtr, elemObjPtr);
		    elemObjPtr = Tcl_NewStringObj(tvarPtr->command, -1);
		    Tcl_ListObjAppendElement(NULL, pairObjPtr, elemObjPtr);
		    Tcl_ListObjAppendElement(interp, resultListPtr, pairObjPtr);
		}
		Tcl_SetObjResult(interp, resultListPtr);
		break;
	    }
	default: {
		panic("Tcl_TraceObjCmd: bad option index to TraceOptions");
	    }
    }
    return TCL_OK;

    badOps:
    Tcl_AppendResult(interp, "bad operations \"", rwuOps,
	    "\": should be one or more of rwu", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceVarProc --
 *
 *	This procedure is called to handle variable accesses that have
 *	been traced using the "trace" command.
 *
 * Results:
 *	Normally returns NULL.  If the trace command returns an error,
 *	then this procedure returns an error string.
 *
 * Side effects:
 *	Depends on the command associated with the trace.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
TraceVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about the variable trace. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable or array. */
    char *name2;		/* Name of element within array;  NULL means
				 * scalar variable is being referenced. */
    int flags;			/* OR-ed bits giving operation and other
				 * information. */
{
    Tcl_SavedResult state;
    TraceVarInfo *tvarPtr = (TraceVarInfo *) clientData;
    char *result;
    int code;
    Tcl_DString cmd;

    result = NULL;
    if (tvarPtr->errMsg != NULL) {
	ckfree(tvarPtr->errMsg);
	tvarPtr->errMsg = NULL;
    }
    if ((tvarPtr->flags & flags) && !(flags & TCL_INTERP_DESTROYED)) {

	/*
	 * Generate a command to execute by appending list elements
	 * for the two variable names and the operation.  The five
	 * extra characters are for three space, the opcode character,
	 * and the terminating null.
	 */

	if (name2 == NULL) {
	    name2 = "";
	}
	Tcl_DStringInit(&cmd);
	Tcl_DStringAppend(&cmd, tvarPtr->command, (int) tvarPtr->length);
	Tcl_DStringAppendElement(&cmd, name1);
	Tcl_DStringAppendElement(&cmd, name2);
	if (flags & TCL_TRACE_READS) {
	    Tcl_DStringAppend(&cmd, " r", 2);
	} else if (flags & TCL_TRACE_WRITES) {
	    Tcl_DStringAppend(&cmd, " w", 2);
	} else if (flags & TCL_TRACE_UNSETS) {
	    Tcl_DStringAppend(&cmd, " u", 2);
	}

	/*
	 * Execute the command.  Save the interp's result used for
	 * the command. We discard any object result the command returns.
	 */

	Tcl_SaveResult(interp, &state);

	code = Tcl_Eval(interp, Tcl_DStringValue(&cmd));
	if (code != TCL_OK) {	     /* copy error msg to result */
	    char *string;
	    int length;
	    
	    string = Tcl_GetStringFromObj(Tcl_GetObjResult(interp), &length);
	    tvarPtr->errMsg = (char *) ckalloc((unsigned) (length + 1));
	    memcpy(tvarPtr->errMsg, string, (size_t) (length + 1));
	    result = tvarPtr->errMsg;
	}

	Tcl_RestoreResult(interp, &state);

	Tcl_DStringFree(&cmd);
    }
    if (flags & TCL_TRACE_DESTROYED) {
	result = NULL;
	if (tvarPtr->errMsg != NULL) {
	    ckfree(tvarPtr->errMsg);
	}
	ckfree((char *) tvarPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WhileObjCmd --
 *
 *      This procedure is invoked to process the "while" Tcl command.
 *      See the user documentation for details on what it does.
 *
 *	With the bytecode compiler, this procedure is only called when
 *	a command name is computed at runtime, and is "while" or the name
 *	to which "while" was renamed: e.g., "set z while; $z {$i<100} {}"
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *----------------------------------------------------------------------
 */

        /* ARGSUSED */
int
Tcl_WhileObjCmd(dummy, interp, objc, objv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *CONST objv[];       	/* Argument objects. */
{
    int result, value;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "test command");
        return TCL_ERROR;
    }

    while (1) {
        result = Tcl_ExprBooleanObj(interp, objv[1], &value);
        if (result != TCL_OK) {
            return result;
        }
        if (!value) {
            break;
        }
        result = Tcl_EvalObjEx(interp, objv[2], 0);
        if ((result != TCL_OK) && (result != TCL_CONTINUE)) {
            if (result == TCL_ERROR) {
                char msg[32 + TCL_INTEGER_SPACE];

                sprintf(msg, "\n    (\"while\" body line %d)",
                        interp->errorLine);
                Tcl_AddErrorInfo(interp, msg);
            }
            break;
        }
    }
    if (result == TCL_BREAK) {
        result = TCL_OK;
    }
    if (result == TCL_OK) {
        Tcl_ResetResult(interp);
    }
    return result;
}

