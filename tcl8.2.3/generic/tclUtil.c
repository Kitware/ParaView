/* 
 * tclUtil.c --
 *
 *	This file contains utility procedures that are used by many Tcl
 *	commands.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * The following variable holds the full path name of the binary
 * from which this application was executed, or NULL if it isn't
 * know.  The value of the variable is set by the procedure
 * Tcl_FindExecutable.  The storage space is dynamically allocated.
 */

char *tclExecutableName = NULL;
char *tclNativeExecutableName = NULL;

/*
 * The following values are used in the flags returned by Tcl_ScanElement
 * and used by Tcl_ConvertElement.  The value TCL_DONT_USE_BRACES is also
 * defined in tcl.h;  make sure its value doesn't overlap with any of the
 * values below.
 *
 * TCL_DONT_USE_BRACES -	1 means the string mustn't be enclosed in
 *				braces (e.g. it contains unmatched braces,
 *				or ends in a backslash character, or user
 *				just doesn't want braces);  handle all
 *				special characters by adding backslashes.
 * USE_BRACES -			1 means the string contains a special
 *				character that can be handled simply by
 *				enclosing the entire argument in braces.
 * BRACES_UNMATCHED -		1 means that braces aren't properly matched
 *				in the argument.
 */

#define USE_BRACES		2
#define BRACES_UNMATCHED	4

/*
 * The following values determine the precision used when converting
 * floating-point values to strings.  This information is linked to all
 * of the tcl_precision variables in all interpreters via the procedure
 * TclPrecTraceProc.
 */

static char precisionString[10] = "12";
				/* The string value of all the tcl_precision
				 * variables. */
static char precisionFormat[10] = "%.12g";
				/* The format string actually used in calls
				 * to sprintf. */
TCL_DECLARE_MUTEX(precisionMutex)


/*
 *----------------------------------------------------------------------
 *
 * TclFindElement --
 *
 *	Given a pointer into a Tcl list, locate the first (or next)
 *	element in the list.
 *
 * Results:
 *	The return value is normally TCL_OK, which means that the
 *	element was successfully located.  If TCL_ERROR is returned
 *	it means that list didn't have proper list structure;
 *	the interp's result contains a more detailed error message.
 *
 *	If TCL_OK is returned, then *elementPtr will be set to point to the
 *	first element of list, and *nextPtr will be set to point to the
 *	character just after any white space following the last character
 *	that's part of the element. If this is the last argument in the
 *	list, then *nextPtr will point just after the last character in the
 *	list (i.e., at the character at list+listLength). If sizePtr is
 *	non-NULL, *sizePtr is filled in with the number of characters in the
 *	element.  If the element is in braces, then *elementPtr will point
 *	to the character after the opening brace and *sizePtr will not
 *	include either of the braces. If there isn't an element in the list,
 *	*sizePtr will be zero, and both *elementPtr and *termPtr will point
 *	just after the last character in the list. Note: this procedure does
 *	NOT collapse backslash sequences.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclFindElement(interp, list, listLength, elementPtr, nextPtr, sizePtr,
	       bracePtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. 
				 * If NULL, then no error message is left
				 * after errors. */
    CONST char *list;		/* Points to the first byte of a string
				 * containing a Tcl list with zero or more
				 * elements (possibly in braces). */
    int listLength;		/* Number of bytes in the list's string. */
    CONST char **elementPtr;	/* Where to put address of first significant
				 * character in first element of list. */
    CONST char **nextPtr;	/* Fill in with location of character just
				 * after all white space following end of
				 * argument (next arg or end of list). */
    int *sizePtr;		/* If non-zero, fill in with size of
				 * element. */
    int *bracePtr;		/* If non-zero, fill in with non-zero/zero
				 * to indicate that arg was/wasn't
				 * in braces. */
{
    CONST char *p = list;
    CONST char *elemStart;	/* Points to first byte of first element. */
    CONST char *limit;		/* Points just after list's last byte. */
    int openBraces = 0;		/* Brace nesting level during parse. */
    int inQuotes = 0;
    int size = 0;		/* lint. */
    int numChars;
    CONST char *p2;
    
    /*
     * Skim off leading white space and check for an opening brace or
     * quote. We treat embedded NULLs in the list as bytes belonging to
     * a list element.
     */

    limit = (list + listLength);
    while ((p < limit) && (isspace(UCHAR(*p)))) { /* INTL: ISO space. */
	p++;
    }
    if (p == limit) {		/* no element found */
	elemStart = limit;
	goto done;
    }

    if (*p == '{') {
	openBraces = 1;
	p++;
    } else if (*p == '"') {
	inQuotes = 1;
	p++;
    }
    elemStart = p;
    if (bracePtr != 0) {
	*bracePtr = openBraces;
    }

    /*
     * Find element's end (a space, close brace, or the end of the string).
     */

    while (p < limit) {
	switch (*p) {

	    /*
	     * Open brace: don't treat specially unless the element is in
	     * braces. In this case, keep a nesting count.
	     */

	    case '{':
		if (openBraces != 0) {
		    openBraces++;
		}
		break;

	    /*
	     * Close brace: if element is in braces, keep nesting count and
	     * quit when the last close brace is seen.
	     */

	    case '}':
		if (openBraces > 1) {
		    openBraces--;
		} else if (openBraces == 1) {
		    size = (p - elemStart);
		    p++;
		    if ((p >= limit)
			    || isspace(UCHAR(*p))) { /* INTL: ISO space. */
			goto done;
		    }

		    /*
		     * Garbage after the closing brace; return an error.
		     */
		    
		    if (interp != NULL) {
			char buf[100];
			
			p2 = p;
			while ((p2 < limit)
				&& (!isspace(UCHAR(*p2))) /* INTL: ISO space. */
			        && (p2 < p+20)) {
			    p2++;
			}
			sprintf(buf,
				"list element in braces followed by \"%.*s\" instead of space",
				(int) (p2-p), p);
			Tcl_SetResult(interp, buf, TCL_VOLATILE);
		    }
		    return TCL_ERROR;
		}
		break;

	    /*
	     * Backslash:  skip over everything up to the end of the
	     * backslash sequence.
	     */

	    case '\\': {
		Tcl_UtfBackslash(p, &numChars, NULL);
		p += (numChars - 1);
		break;
	    }

	    /*
	     * Space: ignore if element is in braces or quotes; otherwise
	     * terminate element.
	     */

	    case ' ':
	    case '\f':
	    case '\n':
	    case '\r':
	    case '\t':
	    case '\v':
		if ((openBraces == 0) && !inQuotes) {
		    size = (p - elemStart);
		    goto done;
		}
		break;

	    /*
	     * Double-quote: if element is in quotes then terminate it.
	     */

	    case '"':
		if (inQuotes) {
		    size = (p - elemStart);
		    p++;
		    if ((p >= limit)
			    || isspace(UCHAR(*p))) { /* INTL: ISO space */
			goto done;
		    }

		    /*
		     * Garbage after the closing quote; return an error.
		     */
		    
		    if (interp != NULL) {
			char buf[100];
			
			p2 = p;
			while ((p2 < limit)
				&& (!isspace(UCHAR(*p2))) /* INTL: ISO space */
				 && (p2 < p+20)) {
			    p2++;
			}
			sprintf(buf,
				"list element in quotes followed by \"%.*s\" %s",
				(int) (p2-p), p, "instead of space");
			Tcl_SetResult(interp, buf, TCL_VOLATILE);
		    }
		    return TCL_ERROR;
		}
		break;
	}
	p++;
    }


    /*
     * End of list: terminate element.
     */

    if (p == limit) {
	if (openBraces != 0) {
	    if (interp != NULL) {
		Tcl_SetResult(interp, "unmatched open brace in list",
			TCL_STATIC);
	    }
	    return TCL_ERROR;
	} else if (inQuotes) {
	    if (interp != NULL) {
		Tcl_SetResult(interp, "unmatched open quote in list",
			TCL_STATIC);
	    }
	    return TCL_ERROR;
	}
	size = (p - elemStart);
    }

    done:
    while ((p < limit) && (isspace(UCHAR(*p)))) { /* INTL: ISO space. */
	p++;
    }
    *elementPtr = elemStart;
    *nextPtr = p;
    if (sizePtr != 0) {
	*sizePtr = size;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCopyAndCollapse --
 *
 *	Copy a string and eliminate any backslashes that aren't in braces.
 *
 * Results:
 *	There is no return value. Count characters get copied from src to
 *	dst. Along the way, if backslash sequences are found outside braces,
 *	the backslashes are eliminated in the copy. After scanning count
 *	chars from source, a null character is placed at the end of dst.
 *	Returns the number of characters that got copied.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclCopyAndCollapse(count, src, dst)
    int count;			/* Number of characters to copy from src. */
    CONST char *src;		/* Copy from here... */
    char *dst;			/* ... to here. */
{
    register char c;
    int numRead;
    int newCount = 0;
    int backslashCount;

    for (c = *src;  count > 0;  src++, c = *src, count--) {
	if (c == '\\') {
	    backslashCount = Tcl_UtfBackslash(src, &numRead, dst);
	    dst += backslashCount;
	    newCount += backslashCount;
	    src += numRead-1;
	    count -= numRead-1;
	} else {
	    *dst = c;
	    dst++;
	    newCount++;
	}
    }
    *dst = 0;
    return newCount;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SplitList --
 *
 *	Splits a list up into its constituent fields.
 *
 * Results
 *	The return value is normally TCL_OK, which means that
 *	the list was successfully split up.  If TCL_ERROR is
 *	returned, it means that "list" didn't have proper list
 *	structure;  the interp's result will contain a more detailed
 *	error message.
 *
 *	*argvPtr will be filled in with the address of an array
 *	whose elements point to the elements of list, in order.
 *	*argcPtr will get filled in with the number of valid elements
 *	in the array.  A single block of memory is dynamically allocated
 *	to hold both the argv array and a copy of the list (with
 *	backslashes and braces removed in the standard way).
 *	The caller must eventually free this memory by calling free()
 *	on *argvPtr.  Note:  *argvPtr and *argcPtr are only modified
 *	if the procedure returns normally.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SplitList(interp, list, argcPtr, argvPtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. 
				 * If NULL, no error message is left. */
    CONST char *list;		/* Pointer to string with list structure. */
    int *argcPtr;		/* Pointer to location to fill in with
				 * the number of elements in the list. */
    char ***argvPtr;		/* Pointer to place to store pointer to
				 * array of pointers to list elements. */
{
    char **argv;
    CONST char *l;
    char *p;
    int length, size, i, result, elSize, brace;
    CONST char *element;

    /*
     * Figure out how much space to allocate.  There must be enough
     * space for both the array of pointers and also for a copy of
     * the list.  To estimate the number of pointers needed, count
     * the number of space characters in the list.
     */

    for (size = 1, l = list; *l != 0; l++) {
	if (isspace(UCHAR(*l))) { /* INTL: ISO space. */
	    size++;
	}
    }
    size++;			/* Leave space for final NULL pointer. */
    argv = (char **) ckalloc((unsigned)
	    ((size * sizeof(char *)) + (l - list) + 1));
    length = strlen(list);
    for (i = 0, p = ((char *) argv) + size*sizeof(char *);
	    *list != 0;  i++) {
	CONST char *prevList = list;
	
	result = TclFindElement(interp, list, length, &element,
				&list, &elSize, &brace);
	length -= (list - prevList);
	if (result != TCL_OK) {
	    ckfree((char *) argv);
	    return result;
	}
	if (*element == 0) {
	    break;
	}
	if (i >= size) {
	    ckfree((char *) argv);
	    if (interp != NULL) {
		Tcl_SetResult(interp, "internal error in Tcl_SplitList",
			TCL_STATIC);
	    }
	    return TCL_ERROR;
	}
	argv[i] = p;
	if (brace) {
	    memcpy((VOID *) p, (VOID *) element, (size_t) elSize);
	    p += elSize;
	    *p = 0;
	    p++;
	} else {
	    TclCopyAndCollapse(elSize, element, p);
	    p += elSize+1;
	}
    }

    argv[i] = NULL;
    *argvPtr = argv;
    *argcPtr = i;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ScanElement --
 *
 *	This procedure is a companion procedure to Tcl_ConvertElement.
 *	It scans a string to see what needs to be done to it (e.g. add
 *	backslashes or enclosing braces) to make the string into a
 *	valid Tcl list element.
 *
 * Results:
 *	The return value is an overestimate of the number of characters
 *	that will be needed by Tcl_ConvertElement to produce a valid
 *	list element from string.  The word at *flagPtr is filled in
 *	with a value needed by Tcl_ConvertElement when doing the actual
 *	conversion.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ScanElement(string, flagPtr)
    register CONST char *string; /* String to convert to list element. */
    register int *flagPtr;	 /* Where to store information to guide
				  * Tcl_ConvertCountedElement. */
{
    return Tcl_ScanCountedElement(string, -1, flagPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ScanCountedElement --
 *
 *	This procedure is a companion procedure to
 *	Tcl_ConvertCountedElement.  It scans a string to see what
 *	needs to be done to it (e.g. add backslashes or enclosing
 *	braces) to make the string into a valid Tcl list element.
 *	If length is -1, then the string is scanned up to the first
 *	null byte.
 *
 * Results:
 *	The return value is an overestimate of the number of characters
 *	that will be needed by Tcl_ConvertCountedElement to produce a
 *	valid list element from string.  The word at *flagPtr is
 *	filled in with a value needed by Tcl_ConvertCountedElement
 *	when doing the actual conversion.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ScanCountedElement(string, length, flagPtr)
    CONST char *string;		/* String to convert to Tcl list element. */
    int length;			/* Number of bytes in string, or -1. */
    int *flagPtr;		/* Where to store information to guide
				 * Tcl_ConvertElement. */
{
    int flags, nestingLevel;
    register CONST char *p, *lastChar;

    /*
     * This procedure and Tcl_ConvertElement together do two things:
     *
     * 1. They produce a proper list, one that will yield back the
     * argument strings when evaluated or when disassembled with
     * Tcl_SplitList.  This is the most important thing.
     * 
     * 2. They try to produce legible output, which means minimizing the
     * use of backslashes (using braces instead).  However, there are
     * some situations where backslashes must be used (e.g. an element
     * like "{abc": the leading brace will have to be backslashed.
     * For each element, one of three things must be done:
     *
     * (a) Use the element as-is (it doesn't contain any special
     * characters).  This is the most desirable option.
     *
     * (b) Enclose the element in braces, but leave the contents alone.
     * This happens if the element contains embedded space, or if it
     * contains characters with special interpretation ($, [, ;, or \),
     * or if it starts with a brace or double-quote, or if there are
     * no characters in the element.
     *
     * (c) Don't enclose the element in braces, but add backslashes to
     * prevent special interpretation of special characters.  This is a
     * last resort used when the argument would normally fall under case
     * (b) but contains unmatched braces.  It also occurs if the last
     * character of the argument is a backslash or if the element contains
     * a backslash followed by newline.
     *
     * The procedure figures out how many bytes will be needed to store
     * the result (actually, it overestimates). It also collects information
     * about the element in the form of a flags word.
     *
     * Note: list elements produced by this procedure and
     * Tcl_ConvertCountedElement must have the property that they can be
     * enclosing in curly braces to make sub-lists.  This means, for
     * example, that we must not leave unmatched curly braces in the
     * resulting list element.  This property is necessary in order for
     * procedures like Tcl_DStringStartSublist to work.
     */

    nestingLevel = 0;
    flags = 0;
    if (string == NULL) {
	string = "";
    }
    if (length == -1) {
	length = strlen(string);
    }
    lastChar = string + length;
    p = string;
    if ((p == lastChar) || (*p == '{') || (*p == '"')) {
	flags |= USE_BRACES;
    }
    for ( ; p < lastChar; p++) {
	switch (*p) {
	    case '{':
		nestingLevel++;
		break;
	    case '}':
		nestingLevel--;
		if (nestingLevel < 0) {
		    flags |= TCL_DONT_USE_BRACES|BRACES_UNMATCHED;
		}
		break;
	    case '[':
	    case '$':
	    case ';':
	    case ' ':
	    case '\f':
	    case '\n':
	    case '\r':
	    case '\t':
	    case '\v':
		flags |= USE_BRACES;
		break;
	    case '\\':
		if ((p+1 == lastChar) || (p[1] == '\n')) {
		    flags = TCL_DONT_USE_BRACES | BRACES_UNMATCHED;
		} else {
		    int size;

		    Tcl_UtfBackslash(p, &size, NULL);
		    p += size-1;
		    flags |= USE_BRACES;
		}
		break;
	}
    }
    if (nestingLevel != 0) {
	flags = TCL_DONT_USE_BRACES | BRACES_UNMATCHED;
    }
    *flagPtr = flags;

    /*
     * Allow enough space to backslash every character plus leave
     * two spaces for braces.
     */

    return 2*(p-string) + 2;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConvertElement --
 *
 *	This is a companion procedure to Tcl_ScanElement.  Given
 *	the information produced by Tcl_ScanElement, this procedure
 *	converts a string to a list element equal to that string.
 *
 * Results:
 *	Information is copied to *dst in the form of a list element
 *	identical to src (i.e. if Tcl_SplitList is applied to dst it
 *	will produce a string identical to src).  The return value is
 *	a count of the number of characters copied (not including the
 *	terminating NULL character).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ConvertElement(src, dst, flags)
    register CONST char *src;	/* Source information for list element. */
    register char *dst;		/* Place to put list-ified element. */
    register int flags;		/* Flags produced by Tcl_ScanElement. */
{
    return Tcl_ConvertCountedElement(src, -1, dst, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConvertCountedElement --
 *
 *	This is a companion procedure to Tcl_ScanCountedElement.  Given
 *	the information produced by Tcl_ScanCountedElement, this
 *	procedure converts a string to a list element equal to that
 *	string.
 *
 * Results:
 *	Information is copied to *dst in the form of a list element
 *	identical to src (i.e. if Tcl_SplitList is applied to dst it
 *	will produce a string identical to src).  The return value is
 *	a count of the number of characters copied (not including the
 *	terminating NULL character).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ConvertCountedElement(src, length, dst, flags)
    register CONST char *src;	/* Source information for list element. */
    int length;			/* Number of bytes in src, or -1. */
    char *dst;			/* Place to put list-ified element. */
    int flags;			/* Flags produced by Tcl_ScanElement. */
{
    register char *p = dst;
    register CONST char *lastChar;

    /*
     * See the comment block at the beginning of the Tcl_ScanElement
     * code for details of how this works.
     */

    if (src && length == -1) {
	length = strlen(src);
    }
    if ((src == NULL) || (length == 0)) {
	p[0] = '{';
	p[1] = '}';
	p[2] = 0;
	return 2;
    }
    lastChar = src + length;
    if ((flags & USE_BRACES) && !(flags & TCL_DONT_USE_BRACES)) {
	*p = '{';
	p++;
	for ( ; src != lastChar; src++, p++) {
	    *p = *src;
	}
	*p = '}';
	p++;
    } else {
	if (*src == '{') {
	    /*
	     * Can't have a leading brace unless the whole element is
	     * enclosed in braces.  Add a backslash before the brace.
	     * Furthermore, this may destroy the balance between open
	     * and close braces, so set BRACES_UNMATCHED.
	     */

	    p[0] = '\\';
	    p[1] = '{';
	    p += 2;
	    src++;
	    flags |= BRACES_UNMATCHED;
	}
	for (; src != lastChar; src++) {
	    switch (*src) {
		case ']':
		case '[':
		case '$':
		case ';':
		case ' ':
		case '\\':
		case '"':
		    *p = '\\';
		    p++;
		    break;
		case '{':
		case '}':
		    /*
		     * It may not seem necessary to backslash braces, but
		     * it is.  The reason for this is that the resulting
		     * list element may actually be an element of a sub-list
		     * enclosed in braces (e.g. if Tcl_DStringStartSublist
		     * has been invoked), so there may be a brace mismatch
		     * if the braces aren't backslashed.
		     */

		    if (flags & BRACES_UNMATCHED) {
			*p = '\\';
			p++;
		    }
		    break;
		case '\f':
		    *p = '\\';
		    p++;
		    *p = 'f';
		    p++;
		    continue;
		case '\n':
		    *p = '\\';
		    p++;
		    *p = 'n';
		    p++;
		    continue;
		case '\r':
		    *p = '\\';
		    p++;
		    *p = 'r';
		    p++;
		    continue;
		case '\t':
		    *p = '\\';
		    p++;
		    *p = 't';
		    p++;
		    continue;
		case '\v':
		    *p = '\\';
		    p++;
		    *p = 'v';
		    p++;
		    continue;
	    }
	    *p = *src;
	    p++;
	}
    }
    *p = '\0';
    return p-dst;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Merge --
 *
 *	Given a collection of strings, merge them together into a
 *	single string that has proper Tcl list structured (i.e.
 *	Tcl_SplitList may be used to retrieve strings equal to the
 *	original elements, and Tcl_Eval will parse the string back
 *	into its original elements).
 *
 * Results:
 *	The return value is the address of a dynamically-allocated
 *	string containing the merged list.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_Merge(argc, argv)
    int argc;			/* How many strings to merge. */
    char **argv;		/* Array of string values. */
{
#   define LOCAL_SIZE 20
    int localFlags[LOCAL_SIZE], *flagPtr;
    int numChars;
    char *result;
    char *dst;
    int i;

    /*
     * Pass 1: estimate space, gather flags.
     */

    if (argc <= LOCAL_SIZE) {
	flagPtr = localFlags;
    } else {
	flagPtr = (int *) ckalloc((unsigned) argc*sizeof(int));
    }
    numChars = 1;
    for (i = 0; i < argc; i++) {
	numChars += Tcl_ScanElement(argv[i], &flagPtr[i]) + 1;
    }

    /*
     * Pass two: copy into the result area.
     */

    result = (char *) ckalloc((unsigned) numChars);
    dst = result;
    for (i = 0; i < argc; i++) {
	numChars = Tcl_ConvertElement(argv[i], dst, flagPtr[i]);
	dst += numChars;
	*dst = ' ';
	dst++;
    }
    if (dst == result) {
	*dst = 0;
    } else {
	dst[-1] = 0;
    }

    if (flagPtr != localFlags) {
	ckfree((char *) flagPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Backslash --
 *
 *	Figure out how to handle a backslash sequence.
 *
 * Results:
 *	The return value is the character that should be substituted
 *	in place of the backslash sequence that starts at src.  If
 *	readPtr isn't NULL then it is filled in with a count of the
 *	number of characters in the backslash sequence.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char
Tcl_Backslash(src, readPtr)
    CONST char *src;		/* Points to the backslash character of
				 * a backslash sequence. */
    int *readPtr;		/* Fill in with number of characters read
				 * from src, unless NULL. */
{
    char buf[TCL_UTF_MAX];
    Tcl_UniChar ch;

    Tcl_UtfBackslash(src, readPtr, buf);
    Tcl_UtfToUniChar(buf, &ch);
    return (char) ch;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Concat --
 *
 *	Concatenate a set of strings into a single large string.
 *
 * Results:
 *	The return value is dynamically-allocated string containing
 *	a concatenation of all the strings in argv, with spaces between
 *	the original argv elements.
 *
 * Side effects:
 *	Memory is allocated for the result;  the caller is responsible
 *	for freeing the memory.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_Concat(argc, argv)
    int argc;			/* Number of strings to concatenate. */
    char **argv;		/* Array of strings to concatenate. */
{
    int totalSize, i;
    char *p;
    char *result;

    for (totalSize = 1, i = 0; i < argc; i++) {
	totalSize += strlen(argv[i]) + 1;
    }
    result = (char *) ckalloc((unsigned) totalSize);
    if (argc == 0) {
	*result = '\0';
	return result;
    }
    for (p = result, i = 0; i < argc; i++) {
	char *element;
	int length;

	/*
	 * Clip white space off the front and back of the string
	 * to generate a neater result, and ignore any empty
	 * elements.
	 */

	element = argv[i];
	while (isspace(UCHAR(*element))) { /* INTL: ISO space. */
	    element++;
	}
	for (length = strlen(element);
		(length > 0)
		&& (isspace(UCHAR(element[length-1]))) /* INTL: ISO space. */
		&& ((length < 2) || (element[length-2] != '\\'));
	        length--) {
	    /* Null loop body. */
	}
	if (length == 0) {
	    continue;
	}
	memcpy((VOID *) p, (VOID *) element, (size_t) length);
	p += length;
	*p = ' ';
	p++;
    }
    if (p != result) {
	p[-1] = 0;
    } else {
	*p = 0;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConcatObj --
 *
 *	Concatenate the strings from a set of objects into a single string
 *	object with spaces between the original strings.
 *
 * Results:
 *	The return value is a new string object containing a concatenation
 *	of the strings in objv. Its ref count is zero.
 *
 * Side effects:
 *	A new object is created.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ConcatObj(objc, objv)
    int objc;			/* Number of objects to concatenate. */
    Tcl_Obj *CONST objv[];	/* Array of objects to concatenate. */
{
    int allocSize, finalSize, length, elemLength, i;
    char *p;
    char *element;
    char *concatStr;
    Tcl_Obj *objPtr;

    allocSize = 0;
    for (i = 0;  i < objc;  i++) {
	objPtr = objv[i];
	element = Tcl_GetStringFromObj(objPtr, &length);
	if ((element != NULL) && (length > 0)) {
	    allocSize += (length + 1);
	}
    }
    if (allocSize == 0) {
	allocSize = 1;		/* enough for the NULL byte at end */
    }

    /*
     * Allocate storage for the concatenated result. Note that allocSize
     * is one more than the total number of characters, and so includes
     * room for the terminating NULL byte.
     */
    
    concatStr = (char *) ckalloc((unsigned) allocSize);

    /*
     * Now concatenate the elements. Clip white space off the front and back
     * to generate a neater result, and ignore any empty elements. Also put
     * a null byte at the end.
     */

    finalSize = 0;
    if (objc == 0) {
	*concatStr = '\0';
    } else {
	p = concatStr;
        for (i = 0;  i < objc;  i++) {
	    objPtr = objv[i];
	    element = Tcl_GetStringFromObj(objPtr, &elemLength);
	    while ((elemLength > 0)
		    && (isspace(UCHAR(*element)))) { /* INTL: ISO space. */
	         element++;
		 elemLength--;
	    }

	    /*
	     * Trim trailing white space.  But, be careful not to trim
	     * a space character if it is preceded by a backslash: in
	     * this case it could be significant.
	     */

	    while ((elemLength > 0)
		    && isspace(UCHAR(element[elemLength-1])) /* INTL: ISO space. */
		    && ((elemLength < 2) || (element[elemLength-2] != '\\'))) {
		elemLength--;
	    }
	    if (elemLength == 0) {
	         continue;	/* nothing left of this element */
	    }
	    memcpy((VOID *) p, (VOID *) element, (size_t) elemLength);
	    p += elemLength;
	    *p = ' ';
	    p++;
	    finalSize += (elemLength + 1);
        }
        if (p != concatStr) {
	    p[-1] = 0;
	    finalSize -= 1;	/* we overwrote the final ' ' */
        } else {
	    *p = 0;
        }
    }
    
    TclNewObj(objPtr);
    objPtr->bytes  = concatStr;
    objPtr->length = finalSize;
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StringMatch --
 *
 *	See if a particular string matches a particular pattern.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and
 *	0 otherwise.  The matching operation permits the following
 *	special characters in the pattern: *?\[] (see the manual
 *	entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_StringMatch(string, pattern)
    CONST char *string;		/* String. */
    CONST char *pattern;	/* Pattern, which may contain special
				 * characters. */
{
    int p, s;
    CONST char *pstart = pattern;
    
    while (1) {
	p = *pattern;
	s = *string;
	
	/*
	 * See if we're at the end of both the pattern and the string.  If
	 * so, we succeeded.  If we're at the end of the pattern but not at
	 * the end of the string, we failed.
	 */
	
	if (p == '\0') {
	    if (s == '\0') {
		return 1;
	    } else {
		return 0;
	    }
	}
	if ((s == '\0') && (p != '*')) {
	    return 0;
	}

	/* Check for a "*" as the next pattern character.  It matches
	 * any substring.  We handle this by calling ourselves
	 * recursively for each postfix of string, until either we
	 * match or we reach the end of the string.
	 */
	
	if (p == '*') {
	    pattern++;
	    if (*pattern == '\0') {
		return 1;
	    }
	    while (1) {
		if (Tcl_StringMatch(string, pattern)) {
		    return 1;
		}
		if (*string == '\0') {
		    return 0;
		}
		string++;
	    }
	}

	/* Check for a "?" as the next pattern character.  It matches
	 * any single character.
	 */

	if (p == '?') {
	    Tcl_UniChar ch;
	    
	    pattern++;
	    string += Tcl_UtfToUniChar(string, &ch);
	    continue;
	}

	/* Check for a "[" as the next pattern character.  It is followed
	 * by a list of characters that are acceptable, or by a range
	 * (two characters separated by "-").
	 */
	
	if (p == '[') {
	    Tcl_UniChar ch, startChar, endChar;

	    pattern++;
	    string += Tcl_UtfToUniChar(string, &ch);

	    while (1) {
		if ((*pattern == ']') || (*pattern == '\0')) {
		    return 0;
		}
		pattern += Tcl_UtfToUniChar(pattern, &startChar);
		if (*pattern == '-') {
		    pattern++;
		    if (*pattern == '\0') {
			return 0;
		    }
		    pattern += Tcl_UtfToUniChar(pattern, &endChar);
		    if (((startChar <= ch) && (ch <= endChar))
			    || ((endChar <= ch) && (ch <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */

			break;
		    }
		} else if (startChar == ch) {
		    break;
		}
	    }
	    while (*pattern != ']') {
		if (*pattern == '\0') {
		    pattern = Tcl_UtfPrev(pattern, pstart);
		    break;
		}
		pattern++;
	    }
	    pattern++;
	    continue;
	}
    
	/* If the next pattern character is '\', just strip off the '\'
	 * so we do exact matching on the character that follows.
	 */
	
	if (p == '\\') {
	    pattern++;
	    p = *pattern;
	    if (p == '\0') {
		return 0;
	    }
	}

	/* There's no special character.  Just make sure that the next
	 * bytes of each string match.
	 */
	
	if (s != p) {
	    return 0;
	}
	pattern++;
	string++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StringCaseMatch --
 *
 *	See if a particular string matches a particular pattern.
 *	Allows case insensitivity.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and
 *	0 otherwise.  The matching operation permits the following
 *	special characters in the pattern: *?\[] (see the manual
 *	entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_StringCaseMatch(string, pattern, nocase)
    CONST char *string;		/* String. */
    CONST char *pattern;	/* Pattern, which may contain special
				 * characters. */
    int nocase;			/* 0 for case sensitive, 1 for insensitive */
{
    int p, s;
    CONST char *pstart = pattern;
    Tcl_UniChar ch1, ch2;
    
    while (1) {
	p = *pattern;
	s = *string;
	
	/*
	 * See if we're at the end of both the pattern and the string.  If
	 * so, we succeeded.  If we're at the end of the pattern but not at
	 * the end of the string, we failed.
	 */
	
	if (p == '\0') {
	    return (s == '\0');
	}
	if ((s == '\0') && (p != '*')) {
	    return 0;
	}

	/* Check for a "*" as the next pattern character.  It matches
	 * any substring.  We handle this by calling ourselves
	 * recursively for each postfix of string, until either we
	 * match or we reach the end of the string.
	 */
	
	if (p == '*') {
	    pattern++;
	    if (*pattern == '\0') {
		return 1;
	    }
	    while (1) {
		if (Tcl_StringCaseMatch(string, pattern, nocase)) {
		    return 1;
		}
		if (*string == '\0') {
		    return 0;
		}
		string++;
	    }
	}

	/* Check for a "?" as the next pattern character.  It matches
	 * any single character.
	 */

	if (p == '?') {
	    pattern++;
	    string += Tcl_UtfToUniChar(string, &ch1);
	    continue;
	}

	/* Check for a "[" as the next pattern character.  It is followed
	 * by a list of characters that are acceptable, or by a range
	 * (two characters separated by "-").
	 */
	
	if (p == '[') {
	    Tcl_UniChar startChar, endChar;

	    pattern++;
	    string += Tcl_UtfToUniChar(string, &ch1);
	    if (nocase) {
		ch1 = Tcl_UniCharToLower(ch1);
	    }
	    while (1) {
		if ((*pattern == ']') || (*pattern == '\0')) {
		    return 0;
		}
		pattern += Tcl_UtfToUniChar(pattern, &startChar);
		if (nocase) {
		    startChar = Tcl_UniCharToLower(startChar);
		}
		if (*pattern == '-') {
		    pattern++;
		    if (*pattern == '\0') {
			return 0;
		    }
		    pattern += Tcl_UtfToUniChar(pattern, &endChar);
		    if (nocase) {
			endChar = Tcl_UniCharToLower(endChar);
		    }
		    if (((startChar <= ch1) && (ch1 <= endChar))
			    || ((endChar <= ch1) && (ch1 <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */

			break;
		    }
		} else if (startChar == ch1) {
		    break;
		}
	    }
	    while (*pattern != ']') {
		if (*pattern == '\0') {
		    pattern = Tcl_UtfPrev(pattern, pstart);
		    break;
		}
		pattern++;
	    }
	    pattern++;
	    continue;
	}
    
	/* If the next pattern character is '\', just strip off the '\'
	 * so we do exact matching on the character that follows.
	 */
	
	if (p == '\\') {
	    pattern++;
	    p = *pattern;
	    if (p == '\0') {
		return 0;
	    }
	}

	/* There's no special character.  Just make sure that the next
	 * bytes of each string match.
	 */
	
	string  += Tcl_UtfToUniChar(string, &ch1);
	pattern += Tcl_UtfToUniChar(pattern, &ch2);
	if (nocase) {
	    if (Tcl_UniCharToLower(ch1) != Tcl_UniCharToLower(ch2)) {
		return 0;
	    }
	} else if (ch1 != ch2) {
	    return 0;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringInit --
 *
 *	Initializes a dynamic string, discarding any previous contents
 *	of the string (Tcl_DStringFree should have been called already
 *	if the dynamic string was previously in use).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The dynamic string is initialized to be empty.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringInit(dsPtr)
    Tcl_DString *dsPtr;		/* Pointer to structure for dynamic string. */
{
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->staticSpace[0] = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringAppend --
 *
 *	Append more characters to the current value of a dynamic string.
 *
 * Results:
 *	The return value is a pointer to the dynamic string's new value.
 *
 * Side effects:
 *	Length bytes from string (or all of string if length is less
 *	than zero) are added to the current value of the string. Memory
 *	gets reallocated if needed to accomodate the string's new size.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_DStringAppend(dsPtr, string, length)
    Tcl_DString *dsPtr;		/* Structure describing dynamic string. */
    CONST char *string;		/* String to append.  If length is -1 then
				 * this must be null-terminated. */
    int length;			/* Number of characters from string to
				 * append.  If < 0, then append all of string,
				 * up to null at end. */
{
    int newSize;
    char *dst;
    CONST char *end;

    if (length < 0) {
	length = strlen(string);
    }
    newSize = length + dsPtr->length;

    /*
     * Allocate a larger buffer for the string if the current one isn't
     * large enough. Allocate extra space in the new buffer so that there
     * will be room to grow before we have to allocate again.
     */

    if (newSize >= dsPtr->spaceAvl) {
	dsPtr->spaceAvl = newSize * 2;
	if (dsPtr->string == dsPtr->staticSpace) {
	    char *newString;

	    newString = (char *) ckalloc((unsigned) dsPtr->spaceAvl);
	    memcpy((VOID *) newString, (VOID *) dsPtr->string,
		    (size_t) dsPtr->length);
	    dsPtr->string = newString;
	} else {
	    dsPtr->string = (char *) ckrealloc((VOID *) dsPtr->string,
		    (size_t) dsPtr->spaceAvl);
	}
    }

    /*
     * Copy the new string into the buffer at the end of the old
     * one.
     */

    for (dst = dsPtr->string + dsPtr->length, end = string+length;
	    string < end; string++, dst++) {
	*dst = *string;
    }
    *dst = '\0';
    dsPtr->length += length;
    return dsPtr->string;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringAppendElement --
 *
 *	Append a list element to the current value of a dynamic string.
 *
 * Results:
 *	The return value is a pointer to the dynamic string's new value.
 *
 * Side effects:
 *	String is reformatted as a list element and added to the current
 *	value of the string.  Memory gets reallocated if needed to
 *	accomodate the string's new size.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_DStringAppendElement(dsPtr, string)
    Tcl_DString *dsPtr;		/* Structure describing dynamic string. */
    CONST char *string;		/* String to append.  Must be
				 * null-terminated. */
{
    int newSize, flags;
    char *dst;

    newSize = Tcl_ScanElement(string, &flags) + dsPtr->length + 1;

    /*
     * Allocate a larger buffer for the string if the current one isn't
     * large enough.  Allocate extra space in the new buffer so that there
     * will be room to grow before we have to allocate again.
     * SPECIAL NOTE: must use memcpy, not strcpy, to copy the string
     * to a larger buffer, since there may be embedded NULLs in the
     * string in some cases.
     */

    if (newSize >= dsPtr->spaceAvl) {
	dsPtr->spaceAvl = newSize * 2;
	if (dsPtr->string == dsPtr->staticSpace) {
	    char *newString;

	    newString = (char *) ckalloc((unsigned) dsPtr->spaceAvl);
	    memcpy((VOID *) newString, (VOID *) dsPtr->string,
		    (size_t) dsPtr->length);
	    dsPtr->string = newString;
	} else {
	    dsPtr->string = (char *) ckrealloc((VOID *) dsPtr->string,
		    (size_t) dsPtr->spaceAvl);
	}
    }

    /*
     * Convert the new string to a list element and copy it into the
     * buffer at the end, with a space, if needed.
     */

    dst = dsPtr->string + dsPtr->length;
    if (TclNeedSpace(dsPtr->string, dst)) {
	*dst = ' ';
	dst++;
	dsPtr->length++;
    }
    dsPtr->length += Tcl_ConvertElement(string, dst, flags);
    return dsPtr->string;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringSetLength --
 *
 *	Change the length of a dynamic string.  This can cause the
 *	string to either grow or shrink, depending on the value of
 *	length.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The length of dsPtr is changed to length and a null byte is
 *	stored at that position in the string.  If length is larger
 *	than the space allocated for dsPtr, then a panic occurs.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringSetLength(dsPtr, length)
    Tcl_DString *dsPtr;		/* Structure describing dynamic string. */
    int length;			/* New length for dynamic string. */
{
    int newsize;

    if (length < 0) {
	length = 0;
    }
    if (length >= dsPtr->spaceAvl) {
	/*
	 * There are two interesting cases here.  In the first case, the user
	 * may be trying to allocate a large buffer of a specific size.  It
	 * would be wasteful to overallocate that buffer, so we just allocate
	 * enough for the requested size plus the trailing null byte.  In the
	 * second case, we are growing the buffer incrementally, so we need
	 * behavior similar to Tcl_DStringAppend.  The requested length will
	 * usually be a small delta above the current spaceAvl, so we'll end up
	 * doubling the old size.  This won't grow the buffer quite as quickly,
	 * but it should be close enough.
	 */

	newsize = dsPtr->spaceAvl * 2;
	if (length < newsize) {
	    dsPtr->spaceAvl = newsize;
	} else {
	    dsPtr->spaceAvl = length + 1;
	}
	if (dsPtr->string == dsPtr->staticSpace) {
	    char *newString;

	    newString = (char *) ckalloc((unsigned) dsPtr->spaceAvl);
	    memcpy((VOID *) newString, (VOID *) dsPtr->string,
		    (size_t) dsPtr->length);
	    dsPtr->string = newString;
	} else {
	    dsPtr->string = (char *) ckrealloc((VOID *) dsPtr->string,
		    (size_t) dsPtr->spaceAvl);
	}
    }
    dsPtr->length = length;
    dsPtr->string[length] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringFree --
 *
 *	Frees up any memory allocated for the dynamic string and
 *	reinitializes the string to an empty state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The previous contents of the dynamic string are lost, and
 *	the new value is an empty string.
 *
 *---------------------------------------------------------------------- */

void
Tcl_DStringFree(dsPtr)
    Tcl_DString *dsPtr;		/* Structure describing dynamic string. */
{
    if (dsPtr->string != dsPtr->staticSpace) {
	ckfree(dsPtr->string);
    }
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->staticSpace[0] = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringResult --
 *
 *	This procedure moves the value of a dynamic string into an
 *	interpreter as its string result. Afterwards, the dynamic string
 *	is reset to an empty string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The string is "moved" to interp's result, and any existing
 *	string result for interp is freed. dsPtr is reinitialized to
 *	an empty string.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringResult(interp, dsPtr)
    Tcl_Interp *interp;		/* Interpreter whose result is to be reset. */
    Tcl_DString *dsPtr;		/* Dynamic string that is to become the
				 * result of interp. */
{
    Tcl_ResetResult(interp);
    
    if (dsPtr->string != dsPtr->staticSpace) {
	interp->result = dsPtr->string;
	interp->freeProc = TCL_DYNAMIC;
    } else if (dsPtr->length < TCL_RESULT_SIZE) {
	interp->result = ((Interp *) interp)->resultSpace;
	strcpy(interp->result, dsPtr->string);
    } else {
	Tcl_SetResult(interp, dsPtr->string, TCL_VOLATILE);
    }
    
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->staticSpace[0] = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringGetResult --
 *
 *	This procedure moves an interpreter's result into a dynamic string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter's string result is cleared, and the previous
 *	contents of dsPtr are freed.
 *
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringGetResult(interp, dsPtr)
    Tcl_Interp *interp;		/* Interpreter whose result is to be reset. */
    Tcl_DString *dsPtr;		/* Dynamic string that is to become the
				 * result of interp. */
{
    Interp *iPtr = (Interp *) interp;
    
    if (dsPtr->string != dsPtr->staticSpace) {
	ckfree(dsPtr->string);
    }

    /*
     * If the string result is empty, move the object result to the
     * string result, then reset the object result.
     */

    if (*(iPtr->result) == 0) {
	Tcl_SetResult(interp, TclGetString(Tcl_GetObjResult(interp)),
	        TCL_VOLATILE);
    }

    dsPtr->length = strlen(iPtr->result);
    if (iPtr->freeProc != NULL) {
	if ((iPtr->freeProc == TCL_DYNAMIC)
		|| (iPtr->freeProc == (Tcl_FreeProc *) free)) {
	    dsPtr->string = iPtr->result;
	    dsPtr->spaceAvl = dsPtr->length+1;
	} else {
	    dsPtr->string = (char *) ckalloc((unsigned) (dsPtr->length+1));
	    strcpy(dsPtr->string, iPtr->result);
	    (*iPtr->freeProc)(iPtr->result);
	}
	dsPtr->spaceAvl = dsPtr->length+1;
	iPtr->freeProc = NULL;
    } else {
	if (dsPtr->length < TCL_DSTRING_STATIC_SIZE) {
	    dsPtr->string = dsPtr->staticSpace;
	    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
	} else {
	    dsPtr->string = (char *) ckalloc((unsigned) (dsPtr->length + 1));
	    dsPtr->spaceAvl = dsPtr->length + 1;
	}
	strcpy(dsPtr->string, iPtr->result);
    }
    
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringStartSublist --
 *
 *	This procedure adds the necessary information to a dynamic
 *	string (e.g. " {" to start a sublist.  Future element
 *	appends will be in the sublist rather than the main list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters get added to the dynamic string.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringStartSublist(dsPtr)
    Tcl_DString *dsPtr;			/* Dynamic string. */
{
    if (TclNeedSpace(dsPtr->string, dsPtr->string + dsPtr->length)) {
	Tcl_DStringAppend(dsPtr, " {", -1);
    } else {
	Tcl_DStringAppend(dsPtr, "{", -1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringEndSublist --
 *
 *	This procedure adds the necessary characters to a dynamic
 *	string to end a sublist (e.g. "}").  Future element appends
 *	will be in the enclosing (sub)list rather than the current
 *	sublist.
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
Tcl_DStringEndSublist(dsPtr)
    Tcl_DString *dsPtr;			/* Dynamic string. */
{
    Tcl_DStringAppend(dsPtr, "}", -1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PrintDouble --
 *
 *	Given a floating-point value, this procedure converts it to
 *	an ASCII string using.
 *
 * Results:
 *	The ASCII equivalent of "value" is written at "dst".  It is
 *	written using the current precision, and it is guaranteed to
 *	contain a decimal point or exponent, so that it looks like
 *	a floating-point value and not an integer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_PrintDouble(interp, value, dst)
    Tcl_Interp *interp;			/* Interpreter whose tcl_precision
					 * variable used to be used to control
					 * printing.  It's ignored now. */
    double value;			/* Value to print as string. */
    char *dst;				/* Where to store converted value;
					 * must have at least TCL_DOUBLE_SPACE
					 * characters. */
{
    char *p, c;
    Tcl_UniChar ch;

    Tcl_MutexLock(&precisionMutex);
    sprintf(dst, precisionFormat, value);
    Tcl_MutexUnlock(&precisionMutex);

    /*
     * If the ASCII result looks like an integer, add ".0" so that it
     * doesn't look like an integer anymore.  This prevents floating-point
     * values from being converted to integers unintentionally.
     */

    for (p = dst; *p != 0; ) {
	p += Tcl_UtfToUniChar(p, &ch);
	c = UCHAR(ch);
	if ((c == '.') || isalpha(UCHAR(c))) {	/* INTL: ISO only. */
	    return;
	}
    }
    p[0] = '.';
    p[1] = '0';
    p[2] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclPrecTraceProc --
 *
 *	This procedure is invoked whenever the variable "tcl_precision"
 *	is written.
 *
 * Results:
 *	Returns NULL if all went well, or an error message if the
 *	new value for the variable doesn't make sense.
 *
 * Side effects:
 *	If the new value doesn't make sense then this procedure
 *	undoes the effect of the variable modification.  Otherwise
 *	it modifies the format string that's used by Tcl_PrintDouble.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
char *
TclPrecTraceProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    char *value, *end;
    int prec;

    /*
     * If the variable is unset, then recreate the trace.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_TraceVar2(interp, name1, name2,
		    TCL_GLOBAL_ONLY|TCL_TRACE_READS|TCL_TRACE_WRITES
		    |TCL_TRACE_UNSETS, TclPrecTraceProc, clientData);
	}
	return (char *) NULL;
    }

    /*
     * When the variable is read, reset its value from our shared
     * value.  This is needed in case the variable was modified in
     * some other interpreter so that this interpreter's value is
     * out of date.
     */

    Tcl_MutexLock(&precisionMutex);

    if (flags & TCL_TRACE_READS) {
	Tcl_SetVar2(interp, name1, name2, precisionString,
		flags & TCL_GLOBAL_ONLY);
	Tcl_MutexUnlock(&precisionMutex);
	return (char *) NULL;
    }

    /*
     * The variable is being written.  Check the new value and disallow
     * it if it isn't reasonable or if this is a safe interpreter (we
     * don't want safe interpreters messing up the precision of other
     * interpreters).
     */

    if (Tcl_IsSafe(interp)) {
	Tcl_SetVar2(interp, name1, name2, precisionString,
		flags & TCL_GLOBAL_ONLY);
	Tcl_MutexUnlock(&precisionMutex);
	return "can't modify precision from a safe interpreter";
    }
    value = Tcl_GetVar2(interp, name1, name2, flags & TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    prec = strtoul(value, &end, 10);
    if ((prec <= 0) || (prec > TCL_MAX_PREC) || (prec > 100) ||
	    (end == value) || (*end != 0)) {
	Tcl_SetVar2(interp, name1, name2, precisionString,
		flags & TCL_GLOBAL_ONLY);
	Tcl_MutexUnlock(&precisionMutex);
	return "improper value for precision";
    }
    TclFormatInt(precisionString, prec);
    sprintf(precisionFormat, "%%.%dg", prec);
    Tcl_MutexUnlock(&precisionMutex);
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNeedSpace --
 *
 *	This procedure checks to see whether it is appropriate to
 *	add a space before appending a new list element to an
 *	existing string.
 *
 * Results:
 *	The return value is 1 if a space is appropriate, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclNeedSpace(start, end)
    char *start;		/* First character in string. */
    char *end;			/* End of string (place where space will
				 * be added, if appropriate). */
{
    /*
     * A space is needed unless either
     * (a) we're at the start of the string, or
     * (b) the trailing characters of the string consist of one or more
     *     open curly braces preceded by a space or extending back to
     *     the beginning of the string.
     * (c) the trailing characters of the string consist of a space
     *	   preceded by a character other than backslash.
     */

    if (end == start) {
	return 0;
    }
    end--;
    if (*end != '{') {
	if (isspace(UCHAR(*end)) /* INTL: ISO space. */
		&& ((end == start) || (end[-1] != '\\'))) {
	    return 0;
	}
	return 1;
    }
    do {
	if (end == start) {
	    return 0;
	}
	end--;
    } while (*end == '{');
    if (isspace(UCHAR(*end))) {	/* INTL: ISO space. */
	return 0;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFormatInt --
 *
 *	This procedure formats an integer into a sequence of decimal digit
 *	characters in a buffer. If the integer is negative, a minus sign is
 *	inserted at the start of the buffer. A null character is inserted at
 *	the end of the formatted characters. It is the caller's
 *	responsibility to ensure that enough storage is available. This
 *	procedure has the effect of sprintf(buffer, "%d", n) but is faster.
 *
 * Results:
 *	An integer representing the number of characters formatted, not
 *	including the terminating \0.
 *
 * Side effects:
 *	The formatted characters are written into the storage pointer to
 *	by the "buffer" argument.
 *
 *----------------------------------------------------------------------
 */

int
TclFormatInt(buffer, n)
    char *buffer;		/* Points to the storage into which the
				 * formatted characters are written. */
    long n;			/* The integer to format. */
{
    long intVal;
    int i;
    int numFormatted, j;
    char *digits = "0123456789";

    /*
     * Check first whether "n" is zero.
     */

    if (n == 0) {
	buffer[0] = '0';
	buffer[1] = 0;
	return 1;
    }

    /*
     * Check whether "n" is the maximum negative value. This is
     * -2^(m-1) for an m-bit word, and has no positive equivalent;
     * negating it produces the same value.
     */

    if (n == -n) {
	sprintf(buffer, "%ld", n);
	return strlen(buffer);
    }

    /*
     * Generate the characters of the result backwards in the buffer.
     */

    intVal = (n < 0? -n : n);
    i = 0;
    buffer[0] = '\0';
    do {
	i++;
	buffer[i] = digits[intVal % 10];
	intVal = intVal/10;
    } while (intVal > 0);
    if (n < 0) {
	i++;
	buffer[i] = '-';
    }
    numFormatted = i;

    /*
     * Now reverse the characters.
     */

    for (j = 0;  j < i;  j++, i--) {
	char tmp = buffer[i];
	buffer[i] = buffer[j];
	buffer[j] = tmp;
    }
    return numFormatted;
}

/*
 *----------------------------------------------------------------------
 *
 * TclLooksLikeInt --
 *
 *	This procedure decides whether the leading characters of a
 *	string look like an integer or something else (such as a
 *	floating-point number or string).
 *
 * Results:
 *	The return value is 1 if the leading characters of p look
 *	like a valid Tcl integer.  If they look like a floating-point
 *	number (e.g. "e01" or "2.4"), or if they don't look like a
 *	number at all, then 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclLooksLikeInt(bytes, length)
    register char *bytes;	/* Points to first byte of the string. */
    int length;			/* Number of bytes in the string. If < 0
				 * bytes up to the first null byte are
				 * considered (if they may appear in an 
				 * integer). */
{
    register char *p, *end;

    if (length < 0) {
	length = (bytes? strlen(bytes) : 0);
    }
    end = (bytes + length);

    p = bytes;
    while ((p < end) && isspace(UCHAR(*p))) { /* INTL: ISO space. */
	p++;
    }
    if (p == end) {
	return 0;
    }
    
    if ((*p == '+') || (*p == '-')) {
	p++;
    }
    if ((p == end) || !isdigit(UCHAR(*p))) { /* INTL: digit */
	return 0;
    }
    p++;
    while ((p < end) && isdigit(UCHAR(*p))) { /* INTL: digit */
	p++;
    }
    if (p == end) {
	return 1;
    }
    if ((*p != '.') && (*p != 'e') && (*p != 'E')) {
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetIntForIndex --
 *
 *	This procedure returns an integer corresponding to the list index
 *	held in a Tcl object. The Tcl object's value is expected to be
 *	either an integer or a string of the form "end([+-]integer)?". 
 *
 * Results:
 *	The return value is normally TCL_OK, which means that the index was
 *	successfully stored into the location referenced by "indexPtr".  If
 *	the Tcl object referenced by "objPtr" has the value "end", the
 *	value stored is "endValue". If "objPtr"s values is not of the form
 *	"end([+-]integer)?" and
 *	can not be converted to an integer, TCL_ERROR is returned and, if
 *	"interp" is non-NULL, an error message is left in the interpreter's
 *	result object.
 *
 * Side effects:
 *	The object referenced by "objPtr" might be converted to an
 *	integer object.
 *
 *----------------------------------------------------------------------
 */

int
TclGetIntForIndex(interp, objPtr, endValue, indexPtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. 
				 * If NULL, then no error message is left
				 * after errors. */
    Tcl_Obj *objPtr;		/* Points to an object containing either
				 * "end" or an integer. */
    int endValue;		/* The value to be stored at "indexPtr" if
				 * "objPtr" holds "end". */
    int *indexPtr;		/* Location filled in with an integer
				 * representing an index. */
{
    char *bytes;
    int length, offset;

    if (objPtr->typePtr == &tclIntType) {
	*indexPtr = (int)objPtr->internalRep.longValue;
	return TCL_OK;
    }

    bytes = Tcl_GetStringFromObj(objPtr, &length);

    if ((*bytes != 'e') ||
	(strncmp(bytes, "end", (size_t)((length > 3) ? 3 : length)) != 0)) {
      if (Tcl_GetIntFromObj(NULL, objPtr, &offset) != TCL_OK) {
	  goto intforindex_error;
      }
      *indexPtr = offset;
      return TCL_OK;
    }

    if (length <= 3) {
      *indexPtr = endValue;
    } else if (bytes[3] == '-') {
      /*
       * This is our limited string expression evaluator
       */
      if (Tcl_GetInt(interp, bytes+3, &offset) != TCL_OK) {
	return TCL_ERROR;
      }
      *indexPtr = endValue + offset;
    } else {
    intforindex_error:
      if ((Interp *)interp != NULL) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			       "bad index \"", bytes,
			       "\": must be integer or end?-integer?",
			       (char *) NULL);
      }
      return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetNameOfExecutable --
 *
 *	This procedure simply returns a pointer to the internal full
 *	path name of the executable file as computed by
 *	Tcl_FindExecutable.  This procedure call is the C API
 *	equivalent to the "info nameofexecutable" command.
 *
 * Results:
 *	A pointer to the internal string or NULL if the internal full
 *	path name has not been computed or unknown.
 *
 * Side effects:
 *	The object referenced by "objPtr" might be converted to an
 *	integer object.
 *
 *----------------------------------------------------------------------
 */

CONST char *
Tcl_GetNameOfExecutable()
{
    return (tclExecutableName);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCwd --
 *
 *	This function replaces the library version of getcwd().
 *
 * Results:
 *	The result is a pointer to a string specifying the current
 *	directory, or NULL if the current directory could not be
 *	determined.  If NULL is returned, an error message is left in the
 *	interp's result.  Storage for the result string is allocated in
 *	bufferPtr; the caller must call Tcl_DStringFree() when the result
 *	is no longer needed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_GetCwd(interp, cwdPtr)
    Tcl_Interp *interp;
    Tcl_DString *cwdPtr;
{
    return TclpGetCwd(interp, cwdPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Chdir --
 *
 *	This function replaces the library version of chdir().
 *
 * Results:
 *	See chdir() documentation.
 *
 * Side effects:
 *	See chdir() documentation.  
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Chdir(dirName)
    CONST char *dirName;
{
    return TclpChdir(dirName);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Access --
 *
 *	This function replaces the library version of access().
 *
 * Results:
 *	See access() documentation.
 *
 * Side effects:
 *	See access() documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Access(path, mode)
    CONST char *path;		/* Path of file to access (UTF-8). */
    int mode;			/* Permission setting. */
{
    return TclAccess(path, mode);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Stat --
 *
 *	This function replaces the library version of stat().
 *
 * Results:
 *	See stat() documentation.
 *
 * Side effects:
 *	See stat() documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Stat(path, bufPtr)
    CONST char *path;		/* Path of file to stat (in UTF-8). */
    struct stat *bufPtr;	/* Filled with results of stat call. */
{
    return TclStat(path, bufPtr);
}
