/* 
 * tclRegexp.c --
 *
 *	This file contains the public interfaces to the Tcl regular
 *	expression mechanism.
 *
 * Copyright (c) 1998 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include "tclRegexp.h"

/*
 *----------------------------------------------------------------------
 * The routines in this file use Henry Spencer's regular expression
 * package contained in the following additional source files:
 *
 *	regc_color.c	regc_cvec.c	regc_lex.c
 *	regc_nfa.c	regcomp.c	regcustom.h
 *	rege_dfa.c	regerror.c	regerrs.h
 *	regex.h		regexec.c	regfree.c
 *	regfronts.c	regguts.h
 *
 * Copyright (c) 1998 Henry Spencer.  All rights reserved.
 * 
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results.  The author
 * thanks all of them. 
 * 
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 * 
 * I'd appreciate being given credit for this package in the documentation
 * of software which uses it, but that is not a requirement.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * *** NOTE: this code has been altered slightly for use in Tcl: ***
 * *** 1. Names have been changed, e.g. from re_comp to		 ***
 * ***    TclRegComp, to avoid clashes with other 		 ***
 * ***    regexp implementations used by applications. 		 ***
 */

/*
 * Thread local storage used to maintain a per-thread cache of compiled
 * regular expressions.
 */

#define NUM_REGEXPS 30

typedef struct ThreadSpecificData {
    int initialized;		/* Set to 1 when the module is initialized. */
    char *patterns[NUM_REGEXPS];/* Strings corresponding to compiled
				 * regular expression patterns.	 NULL
				 * means that this slot isn't used.
				 * Malloc-ed. */
    int patLengths[NUM_REGEXPS];/* Number of non-null characters in
				 * corresponding entry in patterns.
				 * -1 means entry isn't used. */
    struct TclRegexp *regexps[NUM_REGEXPS];
				/* Compiled forms of above strings.  Also
				 * malloc-ed, or NULL if not in use yet. */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Declarations for functions used only in this file.
 */

static TclRegexp *	CompileRegexp _ANSI_ARGS_((Tcl_Interp *interp,
			    char *pattern, int length, int flags));
static void		DupRegexpInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr));
static void		FinalizeRegexp _ANSI_ARGS_((ClientData clientData));
static void		FreeRegexp _ANSI_ARGS_((TclRegexp *regexpPtr));
static void		FreeRegexpInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr));
static int		RegExpExecUniChar _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_RegExp re, CONST Tcl_UniChar *uniString,
			    int numChars, int nmatches, int flags));
static int		SetRegexpFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));

/*
 * The regular expression Tcl object type.  This serves as a cache
 * of the compiled form of the regular expression.
 */

Tcl_ObjType tclRegexpType = {
    "regexp",				/* name */
    FreeRegexpInternalRep,		/* freeIntRepProc */
    DupRegexpInternalRep,		/* dupIntRepProc */
    NULL,				/* updateStringProc */
    SetRegexpFromAny			/* setFromAnyProc */
};


/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegExpCompile --
 *
 *	Compile a regular expression into a form suitable for fast
 *	matching.  This procedure is DEPRECATED in favor of the
 *	object version of the command.
 *
 * Results:
 *	The return value is a pointer to the compiled form of string,
 *	suitable for passing to Tcl_RegExpExec.  This compiled form
 *	is only valid up until the next call to this procedure, so
 *	don't keep these around for a long time!  If an error occurred
 *	while compiling the pattern, then NULL is returned and an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	Updates the cache of compiled regexps.
 *
 *----------------------------------------------------------------------
 */

Tcl_RegExp
Tcl_RegExpCompile(interp, string)
    Tcl_Interp *interp;		/* For use in error reporting and
				 * to access the interp regexp cache. */
    char *string;		/* String for which to produce
				 * compiled regular expression. */
{
    return (Tcl_RegExp) CompileRegexp(interp, string, (int) strlen(string),
	    REG_ADVANCED);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegExpExec --
 *
 *	Execute the regular expression matcher using a compiled form
 *	of a regular expression and save information about any match
 *	that is found.
 *
 * Results:
 *	If an error occurs during the matching operation then -1
 *	is returned and the interp's result contains an error message.
 *	Otherwise the return value is 1 if a matching range is
 *	found and 0 if there is no matching range.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RegExpExec(interp, re, string, start)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    Tcl_RegExp re;		/* Compiled regular expression;  must have
				 * been returned by previous call to
				 * Tcl_GetRegExpFromObj. */
    CONST char *string;		/* String against which to match re. */
    CONST char *start;		/* If string is part of a larger string,
				 * this identifies beginning of larger
				 * string, so that "^" won't match. */
{
    int flags, result, numChars;
    TclRegexp *regexp = (TclRegexp *)re;
    Tcl_DString ds;
    Tcl_UniChar *ustr;

    /*
     * If the starting point is offset from the beginning of the buffer,
     * then we need to tell the regexp engine not to match "^".
     */

    if (string > start) {
	flags = REG_NOTBOL;
    } else {
	flags = 0;
    }

    /*
     * Remember the string for use by Tcl_RegExpRange().
     */

    regexp->string = string;
    regexp->objPtr = NULL;

    /*
     * Convert the string to Unicode and perform the match.
     */

    Tcl_DStringInit(&ds);
    ustr = Tcl_UtfToUniCharDString(string, -1, &ds);
    numChars = Tcl_DStringLength(&ds) / sizeof(Tcl_UniChar);
    result = RegExpExecUniChar(interp, re, ustr, numChars,
	    -1 /* nmatches */, flags);
    Tcl_DStringFree(&ds);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_RegExpRange --
 *
 *	Returns pointers describing the range of a regular expression match,
 *	or one of the subranges within the match.
 *
 * Results:
 *	The variables at *startPtr and *endPtr are modified to hold the
 *	addresses of the endpoints of the range given by index.  If the
 *	specified range doesn't exist then NULLs are returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
Tcl_RegExpRange(re, index, startPtr, endPtr)
    Tcl_RegExp re;		/* Compiled regular expression that has
				 * been passed to Tcl_RegExpExec. */
    int index;			/* 0 means give the range of the entire
				 * match, > 0 means give the range of
				 * a matching subrange. */
    char **startPtr;		/* Store address of first character in
				 * (sub-) range here. */
    char **endPtr;		/* Store address of character just after last
				 * in (sub-) range here. */
{
    TclRegexp *regexpPtr = (TclRegexp *) re;
    CONST char *string;

    if ((size_t) index > regexpPtr->re.re_nsub) {
	*startPtr = *endPtr = NULL;
    } else if (regexpPtr->matches[index].rm_so < 0) {
	*startPtr = *endPtr = NULL;
    } else {
	if (regexpPtr->objPtr) {
	    string = Tcl_GetString(regexpPtr->objPtr);
	} else {
	    string = regexpPtr->string;
	}
	*startPtr = Tcl_UtfAtIndex(string, regexpPtr->matches[index].rm_so);
	*endPtr = Tcl_UtfAtIndex(string, regexpPtr->matches[index].rm_eo);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * RegExpExecUniChar --
 *
 *	Execute the regular expression matcher using a compiled form of a
 *	regular expression and save information about any match that is
 *	found.
 *
 * Results:
 *	If an error occurs during the matching operation then -1 is
 *	returned and an error message is left in interp's result.
 *	Otherwise the return value is 1 if a matching range was found or
 *	0 if there was no matching range.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
RegExpExecUniChar(interp, re, wString, numChars, nmatches, flags)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    Tcl_RegExp re;		/* Compiled regular expression; returned by
				 * a previous call to Tcl_GetRegExpFromObj */
    CONST Tcl_UniChar *wString;	/* String against which to match re. */
    int numChars;		/* Length of Tcl_UniChar string (must
				 * be >= 0). */
    int nmatches;		/* How many subexpression matches (counting
				 * the whole match as subexpression 0) are
				 * of interest.  -1 means "don't know". */
    int flags;			/* Regular expression flags. */
{
    int status;
    TclRegexp *regexpPtr = (TclRegexp *) re;
    size_t last = regexpPtr->re.re_nsub + 1;
    size_t nm = last;

    if (nmatches >= 0 && (size_t) nmatches < nm) {
	nm = (size_t) nmatches;
    }

    status = TclReExec(&regexpPtr->re, wString, (size_t) numChars,
	    &regexpPtr->details, nm, regexpPtr->matches, flags);

    /*
     * Check for errors.
     */

    if (status != REG_OKAY) {
	if (status == REG_NOMATCH) {
	    return 0;
	}
	if (interp != NULL) {
	    TclRegError(interp, "error while matching regular expression: ",
		    status);
	}
	return -1;
    }
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclRegExpRangeUniChar --
 *
 *	Returns pointers describing the range of a regular expression match,
 *	or one of the subranges within the match, or the hypothetical range
 *	represented by the rm_extend field of the rm_detail_t.
 *
 * Results:
 *	The variables at *startPtr and *endPtr are modified to hold the
 *	offsets of the endpoints of the range given by index.  If the
 *	specified range doesn't exist then -1s are supplied.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TclRegExpRangeUniChar(re, index, startPtr, endPtr)
    Tcl_RegExp re;		/* Compiled regular expression that has
				 * been passed to Tcl_RegExpExec. */
    int index;			/* 0 means give the range of the entire
				 * match, > 0 means give the range of
				 * a matching subrange, -1 means the
				 * range of the rm_extend field. */
    int *startPtr;		/* Store address of first character in
				 * (sub-) range here. */
    int *endPtr;		/* Store address of character just after last
				 * in (sub-) range here. */
{
    TclRegexp *regexpPtr = (TclRegexp *) re;

    if ((regexpPtr->flags&REG_EXPECT) && index == -1) {
	*startPtr = regexpPtr->details.rm_extend.rm_so;
	*endPtr = regexpPtr->details.rm_extend.rm_eo;
    } else if ((size_t) index > regexpPtr->re.re_nsub) {
	*startPtr = -1;
	*endPtr = -1;
    } else {
	*startPtr = regexpPtr->matches[index].rm_so;
	*endPtr = regexpPtr->matches[index].rm_eo;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegExpMatch --
 *
 *	See if a string matches a regular expression.
 *
 * Results:
 *	If an error occurs during the matching operation then -1
 *	is returned and the interp's result contains an error message.
 *	Otherwise the return value is 1 if "string" matches "pattern"
 *	and 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RegExpMatch(interp, string, pattern)
    Tcl_Interp *interp;		/* Used for error reporting. May be NULL. */
    char *string;		/* String. */
    char *pattern;		/* Regular expression to match against
				 * string. */
{
    Tcl_RegExp re;

    re = Tcl_RegExpCompile(interp, pattern);
    if (re == NULL) {
	return -1;
    }
    return Tcl_RegExpExec(interp, re, string, string);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegExpExecObj --
 *
 *	Execute a precompiled regexp against the given object.
 *
 * Results:
 *	If an error occurs during the matching operation then -1
 *	is returned and the interp's result contains an error message.
 *	Otherwise the return value is 1 if "string" matches "pattern"
 *	and 0 otherwise.
 *
 * Side effects:
 *	Converts the object to a Unicode object.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RegExpExecObj(interp, re, objPtr, offset, nmatches, flags)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    Tcl_RegExp re;		/* Compiled regular expression;  must have
				 * been returned by previous call to
				 * Tcl_GetRegExpFromObj. */
    Tcl_Obj *objPtr;		/* String against which to match re. */
    int offset;			/* Character index that marks where matching
				 * should begin. */
    int nmatches;		/* How many subexpression matches (counting
				 * the whole match as subexpression 0) are
				 * of interest.  -1 means all of them. */
    int flags;			/* Regular expression execution flags. */
{
    TclRegexp *regexpPtr = (TclRegexp *) re;
    Tcl_UniChar *udata;
    int length;

    /*
     * Save the target object so we can extract strings from it later.
     */

    regexpPtr->string = NULL;
    regexpPtr->objPtr = objPtr;

    udata = Tcl_GetUnicode(objPtr);
    length = Tcl_GetCharLength(objPtr);

    if (offset > length) {
	offset = length;
    }
    udata += offset;
    length -= offset;
    
    return RegExpExecUniChar(interp, re, udata, length, nmatches, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegExpMatchObj --
 *
 *	See if an object matches a regular expression.
 *
 * Results:
 *	If an error occurs during the matching operation then -1
 *	is returned and the interp's result contains an error message.
 *	Otherwise the return value is 1 if "string" matches "pattern"
 *	and 0 otherwise.
 *
 * Side effects:
 *	Changes the internal rep of the pattern and string objects.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RegExpMatchObj(interp, stringObj, patternObj)
    Tcl_Interp *interp;		/* Used for error reporting. May be NULL. */
    Tcl_Obj *stringObj;		/* Object containing the String to search. */
    Tcl_Obj *patternObj;	/* Regular expression to match against
				 * string. */
{
    Tcl_RegExp re;

    re = Tcl_GetRegExpFromObj(interp, patternObj,
	    TCL_REG_ADVANCED | TCL_REG_NOSUB);
    if (re == NULL) {
	return -1;
    }
    return Tcl_RegExpExecObj(interp, re, stringObj, 0 /* offset */,
	    0 /* nmatches */, 0 /* flags */);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegExpGetInfo --
 *
 *	Retrieve information about the current match.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_RegExpGetInfo(regexp, infoPtr)
    Tcl_RegExp regexp;		/* Pattern from which to get subexpressions. */
    Tcl_RegExpInfo *infoPtr;	/* Match information is stored here.  */
{
    TclRegexp *regexpPtr = (TclRegexp *) regexp;

    infoPtr->nsubs = regexpPtr->re.re_nsub;
    infoPtr->matches = (Tcl_RegExpIndices *) regexpPtr->matches;
    infoPtr->extendStart = regexpPtr->details.rm_extend.rm_so;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetRegExpFromObj --
 *
 *	Compile a regular expression into a form suitable for fast
 *	matching.  This procedure caches the result in a Tcl_Obj.
 *
 * Results:
 *	The return value is a pointer to the compiled form of string,
 *	suitable for passing to Tcl_RegExpExec.  If an error occurred
 *	while compiling the pattern, then NULL is returned and an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	Updates the native rep of the Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

Tcl_RegExp
Tcl_GetRegExpFromObj(interp, objPtr, flags)
    Tcl_Interp *interp;		/* For use in error reporting, and to access
				 * the interp regexp cache. */
    Tcl_Obj *objPtr;		/* Object whose string rep contains regular
				 * expression pattern.  Internal rep will be
				 * changed to compiled form of this regular
				 * expression. */
    int flags;			/* Regular expression compilation flags. */
{
    int length;
    Tcl_ObjType *typePtr;
    TclRegexp *regexpPtr;
    char *pattern;

    typePtr = objPtr->typePtr;
    regexpPtr = (TclRegexp *) objPtr->internalRep.otherValuePtr;

    if ((typePtr != &tclRegexpType) || (regexpPtr->flags != flags)) {
	pattern = Tcl_GetStringFromObj(objPtr, &length);

	regexpPtr = CompileRegexp(interp, pattern, length, flags);
	if (regexpPtr == NULL) {
	    return NULL;
	}

	/*
	 * Add a reference to the regexp so it will persist even if it is
	 * pushed out of the current thread's regexp cache.  This reference
	 * will be removed when the object's internal rep is freed.
	 */

	regexpPtr->refCount++;

	/*
	 * Free the old representation and set our type.
	 */

	if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	    (*typePtr->freeIntRepProc)(objPtr);
	}
	objPtr->internalRep.otherValuePtr = (VOID *) regexpPtr;
	objPtr->typePtr = &tclRegexpType;
    }
    return (Tcl_RegExp) regexpPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclRegAbout --
 *
 *	Return information about a compiled regular expression.
 *
 * Results:
 *	The return value is -1 for failure, 0 for success, although at
 *	the moment there's nothing that could fail.  On success, a list
 *	is left in the interp's result:  first element is the subexpression
 *	count, second is a list of re_info bit names.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclRegAbout(interp, re)
    Tcl_Interp *interp;		/* For use in variable assignment. */
    Tcl_RegExp re;		/* The compiled regular expression. */
{
    TclRegexp *regexpPtr = (TclRegexp *)re;
    char buf[TCL_INTEGER_SPACE];
    static struct infoname {
	int bit;
	char *text;
    } infonames[] = {
	{REG_UBACKREF,		"REG_UBACKREF"},
	{REG_ULOOKAHEAD,	"REG_ULOOKAHEAD"},
	{REG_UBOUNDS,		"REG_UBOUNDS"},
	{REG_UBRACES,		"REG_UBRACES"},
	{REG_UBSALNUM,		"REG_UBSALNUM"},
	{REG_UPBOTCH,		"REG_UPBOTCH"},
	{REG_UBBS,		"REG_UBBS"},
	{REG_UNONPOSIX,		"REG_UNONPOSIX"},
	{REG_UUNSPEC,		"REG_UUNSPEC"},
	{REG_UUNPORT,		"REG_UUNPORT"},
	{REG_ULOCALE,		"REG_ULOCALE"},
	{REG_UEMPTYMATCH,	"REG_UEMPTYMATCH"},
	{REG_UIMPOSSIBLE,	"REG_UIMPOSSIBLE"},
	{REG_USHORTEST,		"REG_USHORTEST"},
	{0,			""}
    };
    struct infoname *inf;
    int n;

    Tcl_ResetResult(interp);

    sprintf(buf, "%u", (unsigned)(regexpPtr->re.re_nsub));
    Tcl_AppendElement(interp, buf);

    /*
     * Must count bits before generating list, because we must know
     * whether {} are needed before we start appending names.
     */
    n = 0;
    for (inf = infonames; inf->bit != 0; inf++) {
	if (regexpPtr->re.re_info&inf->bit) {
	    n++;
	}
    }
    if (n != 1) {
	Tcl_AppendResult(interp, " {", NULL);
    }
    for (inf = infonames; inf->bit != 0; inf++) {
	if (regexpPtr->re.re_info&inf->bit) {
	    Tcl_AppendElement(interp, inf->text);
	}
    }
    if (n != 1) {
	Tcl_AppendResult(interp, "}", NULL);
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclRegError --
 *
 *	Generate an error message based on the regexp status code.
 *
 * Results:
 *	Places an error in the interpreter.
 *
 * Side effects:
 *	Sets errorCode as well.
 *
 *----------------------------------------------------------------------
 */

void
TclRegError(interp, msg, status)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    char *msg;			/* Message to prepend to error. */
    int status;			/* Status code to report. */
{
    char buf[100];		/* ample in practice */
    char cbuf[100];		/* lots in practice */
    size_t n;
    char *p;

    Tcl_ResetResult(interp);
    n = TclReError(status, (regex_t *)NULL, buf, sizeof(buf));
    p = (n > sizeof(buf)) ? "..." : "";
    Tcl_AppendResult(interp, msg, buf, p, NULL);

    sprintf(cbuf, "%d", status);
    (VOID) TclReError(REG_ITOA, (regex_t *)NULL, cbuf, sizeof(cbuf));
    Tcl_SetErrorCode(interp, "REGEXP", cbuf, buf, NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * FreeRegexpInternalRep --
 *
 *	Deallocate the storage associated with a regexp object's internal
 *	representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the compiled regular expression.
 *
 *----------------------------------------------------------------------
 */

static void
FreeRegexpInternalRep(objPtr)
    Tcl_Obj *objPtr;		/* Regexp object with internal rep to free. */
{
    TclRegexp *regexpRepPtr = (TclRegexp *) objPtr->internalRep.otherValuePtr;

    /*
     * If this is the last reference to the regexp, free it.
     */

    if (--(regexpRepPtr->refCount) <= 0) {
	FreeRegexp(regexpRepPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DupRegexpInternalRep --
 *
 *	We copy the reference to the compiled regexp and bump its
 *	reference count.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increments the reference count of the regexp.
 *
 *----------------------------------------------------------------------
 */

static void
DupRegexpInternalRep(srcPtr, copyPtr)
    Tcl_Obj *srcPtr;		/* Object with internal rep to copy. */
    Tcl_Obj *copyPtr;		/* Object with internal rep to set. */
{
    TclRegexp *regexpPtr = (TclRegexp *) srcPtr->internalRep.otherValuePtr;
    regexpPtr->refCount++;
    copyPtr->internalRep.otherValuePtr = srcPtr->internalRep.otherValuePtr;
    copyPtr->typePtr = &tclRegexpType;
}

/*
 *----------------------------------------------------------------------
 *
 * SetRegexpFromAny --
 *
 *	Attempt to generate a compiled regular expression for the Tcl object
 *	"objPtr".
 *
 * Results:
 *	The return value is TCL_OK or TCL_ERROR. If an error occurs during
 *	conversion, an error message is left in the interpreter's result
 *	unless "interp" is NULL.
 *
 * Side effects:
 *	If no error occurs, a regular expression is stored as "objPtr"s
 *	internal representation.
 *
 *----------------------------------------------------------------------
 */

static int
SetRegexpFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    if (Tcl_GetRegExpFromObj(interp, objPtr, REG_ADVANCED) == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CompileRegexp --
 *
 *	Attempt to compile the given regexp pattern.  If the compiled
 *	regular expression can be found in the per-thread cache, it
 *	will be used instead of compiling a new copy.
 *
 * Results:
 *	The return value is a pointer to a newly allocated TclRegexp
 *	that represents the compiled pattern, or NULL if the pattern
 *	could not be compiled.  If NULL is returned, an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	The thread-local regexp cache is updated and a new TclRegexp may
 *	be allocated.
 *
 *----------------------------------------------------------------------
 */

static TclRegexp *
CompileRegexp(interp, string, length, flags)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    char *string;		/* The regexp to compile (UTF-8). */
    int length;			/* The length of the string in bytes. */
    int flags;			/* Compilation flags. */
{
    TclRegexp *regexpPtr;
    Tcl_UniChar *uniString;
    int numChars;
    Tcl_DString stringBuf;
    int status, i;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
 
    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateThreadExitHandler(FinalizeRegexp, NULL);
    }

    /*
     * This routine maintains a second-level regular expression cache in
     * addition to the per-object regexp cache.  The per-thread cache is needed
     * to handle the case where for various reasons the object is lost between
     * invocations of the regexp command, but the literal pattern is the same.
     */

    /*
     * Check the per-thread compiled regexp cache.  We can only reuse
     * a regexp if it has the same pattern and the same flags.
     */

    for (i = 0; (i < NUM_REGEXPS) && (tsdPtr->patterns[i] != NULL); i++) {
	if ((length == tsdPtr->patLengths[i])
		&& (tsdPtr->regexps[i]->flags == flags)
		&& (strcmp(string, tsdPtr->patterns[i]) == 0)) {
	    /*
	     * Move the matched pattern to the first slot in the
	     * cache and shift the other patterns down one position.
	     */

	    if (i != 0) {
		int j;
		char *cachedString;

		cachedString = tsdPtr->patterns[i];
		regexpPtr = tsdPtr->regexps[i];
		for (j = i-1; j >= 0; j--) {
		    tsdPtr->patterns[j+1] = tsdPtr->patterns[j];
		    tsdPtr->patLengths[j+1] = tsdPtr->patLengths[j];
		    tsdPtr->regexps[j+1] = tsdPtr->regexps[j];
		}
		tsdPtr->patterns[0] = cachedString;
		tsdPtr->patLengths[0] = length;
		tsdPtr->regexps[0] = regexpPtr;
	    }
	    return tsdPtr->regexps[0];
	}
    }

    /*
     * This is a new expression, so compile it and add it to the cache.
     */
    
    regexpPtr = (TclRegexp *) ckalloc(sizeof(TclRegexp));
    regexpPtr->objPtr = NULL;
    regexpPtr->string = NULL;
    regexpPtr->details.rm_extend.rm_so = -1;
    regexpPtr->details.rm_extend.rm_eo = -1;

    /*
     * Get the up-to-date string representation and map to unicode.
     */

    Tcl_DStringInit(&stringBuf);
    uniString = Tcl_UtfToUniCharDString(string, length, &stringBuf);
    numChars = Tcl_DStringLength(&stringBuf) / sizeof(Tcl_UniChar);

    /*
     * Compile the string and check for errors.
     */

    regexpPtr->flags = flags;
    status = TclReComp(&regexpPtr->re, uniString, (size_t) numChars, flags);
    Tcl_DStringFree(&stringBuf);

    if (status != REG_OKAY) {
	/*
	 * Clean up and report errors in the interpreter, if possible.
	 */

	ckfree((char *)regexpPtr);
	if (interp) {
	    TclRegError(interp,
		    "couldn't compile regular expression pattern: ",
		    status);
	}
	return NULL;
    }

    /*
     * Allocate enough space for all of the subexpressions, plus one
     * extra for the entire pattern.
     */

    regexpPtr->matches = (regmatch_t *) ckalloc(
	    sizeof(regmatch_t) * (regexpPtr->re.re_nsub + 1));

    /*
     * Initialize the refcount to one initially, since it is in the cache.
     */

    regexpPtr->refCount = 1;

    /*
     * Free the last regexp, if necessary, and make room at the head of the
     * list for the new regexp.
     */

    if (tsdPtr->patterns[NUM_REGEXPS-1] != NULL) {
	TclRegexp *oldRegexpPtr = tsdPtr->regexps[NUM_REGEXPS-1];
	if (--(oldRegexpPtr->refCount) <= 0) {
	    FreeRegexp(oldRegexpPtr);
	}
	ckfree(tsdPtr->patterns[NUM_REGEXPS-1]);
    }
    for (i = NUM_REGEXPS - 2; i >= 0; i--) {
	tsdPtr->patterns[i+1] = tsdPtr->patterns[i];
	tsdPtr->patLengths[i+1] = tsdPtr->patLengths[i];
	tsdPtr->regexps[i+1] = tsdPtr->regexps[i];
    }
    tsdPtr->patterns[0] = (char *) ckalloc((unsigned) (length+1));
    strcpy(tsdPtr->patterns[0], string);
    tsdPtr->patLengths[0] = length;
    tsdPtr->regexps[0] = regexpPtr;

    return regexpPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeRegexp --
 *
 *	Release the storage associated with a TclRegexp.
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
FreeRegexp(regexpPtr)
    TclRegexp *regexpPtr;	/* Compiled regular expression to free. */
{
    TclReFree(&regexpPtr->re);
    if (regexpPtr->matches) {
	ckfree((char *) regexpPtr->matches);
    }
    ckfree((char *) regexpPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FinalizeRegexp --
 *
 *	Release the storage associated with the per-thread regexp
 *	cache.
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
FinalizeRegexp(clientData)
    ClientData clientData;	/* Not used. */
{
    int i;
    TclRegexp *regexpPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    for (i = 0; (i < NUM_REGEXPS) && (tsdPtr->patterns[i] != NULL); i++) {
	regexpPtr = tsdPtr->regexps[i];
	if (--(regexpPtr->refCount) <= 0) {
	    FreeRegexp(regexpPtr);
	}
	ckfree(tsdPtr->patterns[i]);
    }
}
