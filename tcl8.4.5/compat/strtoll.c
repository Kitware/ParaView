/* 
 * strtoll.c --
 *
 *	Source code for the "strtoll" library procedure.
 *
 * Copyright (c) 1988 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclPort.h"
#include <ctype.h>

#define TCL_WIDEINT_MAX	(((Tcl_WideUInt)Tcl_LongAsWide(-1))>>1)


/*
 *----------------------------------------------------------------------
 *
 * strtoll --
 *
 *	Convert an ASCII string into an integer.
 *
 * Results:
 *	The return value is the integer equivalent of string.  If endPtr
 *	is non-NULL, then *endPtr is filled in with the character
 *	after the last one that was part of the integer.  If string
 *	doesn't contain a valid integer value, then zero is returned
 *	and *endPtr is set to string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#if TCL_WIDE_INT_IS_LONG
long long
#else
Tcl_WideInt
#endif
strtoll(string, endPtr, base)
    CONST char *string;		/* String of ASCII digits, possibly
				 * preceded by white space.  For bases
				 * greater than 10, either lower- or
				 * upper-case digits may be used.
				 */
    char **endPtr;		/* Where to store address of terminating
				 * character, or NULL. */
    int base;			/* Base for conversion.  Must be less
				 * than 37.  If 0, then the base is chosen
				 * from the leading characters of string:
				 * "0x" means hex, "0" means octal, anything
				 * else means decimal.
				 */
{
    register CONST char *p;
    Tcl_WideInt result = Tcl_LongAsWide(0);
    Tcl_WideUInt uwResult;

    /*
     * Skip any leading blanks.
     */

    p = string;
    while (isspace(UCHAR(*p))) {
	p += 1;
    }

    /*
     * Check for a sign.
     */

    errno = 0;
    if (*p == '-') {
	p += 1;
	uwResult = strtoull(p, endPtr, base);
	if (errno != ERANGE) {
	    if (uwResult > TCL_WIDEINT_MAX+1) {
		errno = ERANGE;
		return Tcl_LongAsWide(-1);
	    } else if (uwResult > TCL_WIDEINT_MAX) {
		return ~((Tcl_WideInt)TCL_WIDEINT_MAX);
	    } else {
		result = -((Tcl_WideInt) uwResult);
	    }
	}
    } else {
	if (*p == '+') {
	    p += 1;
	}
	uwResult = strtoull(p, endPtr, base);
	if (errno != ERANGE) {
	    if (uwResult > TCL_WIDEINT_MAX) {
		errno = ERANGE;
		return Tcl_LongAsWide(-1);
	    } else {
		result = uwResult;
	    }
	}
    }
    if ((result == 0) && (endPtr != 0) && (*endPtr == p)) {
	*endPtr = (char *) string;
    }
    return result;
}
