/* 
 * strtoull.c --
 *
 *	Source code for the "strtoull" library procedure.
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

/*
 * The table below is used to convert from ASCII digits to a
 * numerical equivalent.  It maps from '0' through 'z' to integers
 * (100 for non-digit characters).
 */

static char cvtIn[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,		/* '0' - '9' */
    100, 100, 100, 100, 100, 100, 100,		/* punctuation */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,	/* 'A' - 'Z' */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35,
    100, 100, 100, 100, 100, 100,		/* punctuation */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,	/* 'a' - 'z' */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35};


/*
 *----------------------------------------------------------------------
 *
 * strtoull --
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
unsigned long long
#else
Tcl_WideUInt
#endif
strtoull(string, endPtr, base)
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
    register Tcl_WideUInt result = 0;
    register unsigned digit;
    register Tcl_WideUInt shifted;
    int anyDigits = 0, negative = 0;

    /*
     * Skip any leading blanks.
     */

    p = string;
    while (isspace(UCHAR(*p))) {	/* INTL: locale-dependent */
	p += 1;
    }

    /*
     * Check for a sign.
     */

    if (*p == '-') {
	p += 1;
	negative = 1;
    } else {
	if (*p == '+') {
	    p += 1;
	}
    }

    /*
     * If no base was provided, pick one from the leading characters
     * of the string.
     */
    
    if (base == 0) {
	if (*p == '0') {
	    p += 1;
	    if (*p == 'x' || *p == 'X') {
		p += 1;
		base = 16;
	    } else {

		/*
		 * Must set anyDigits here, otherwise "0" produces a
		 * "no digits" error.
		 */

		anyDigits = 1;
		base = 8;
	    }
	} else {
	    base = 10;
	}
    } else if (base == 16) {

	/*
	 * Skip a leading "0x" from hex numbers.
	 */

	if ((p[0] == '0') && (p[1] == 'x' || *p == 'X')) {
	    p += 2;
	}
    }

    /*
     * Sorry this code is so messy, but speed seems important.  Do
     * different things for base 8, 10, 16, and other.
     */

    if (base == 8) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > 7) {
		break;
	    }
	    shifted = result << 3;
	    if ((shifted >> 3) != result) {
		goto overflow;
	    }
	    result = shifted + digit;
	    if ( result < shifted ) {
		goto overflow;
	    }
	    anyDigits = 1;
	}
    } else if (base == 10) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > 9) {
		break;
	    }
	    shifted = 10 * result;
	    if ((shifted / 10) != result) {
		goto overflow;
	    }
	    result = shifted + digit;
	    if ( result < shifted ) {
		goto overflow;
	    }
	    anyDigits = 1;
	}
    } else if (base == 16) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > ('z' - '0')) {
		break;
	    }
	    digit = cvtIn[digit];
	    if (digit > 15) {
		break;
	    }
	    shifted = result << 4;
	    if ((shifted >> 4) != result) {
		goto overflow;
	    }
	    result = shifted + digit;
	    if ( result < shifted ) {
		goto overflow;
	    }
	    anyDigits = 1;
	}
    } else if ( base >= 2 && base <= 36 ) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > ('z' - '0')) {
		break;
	    }
	    digit = cvtIn[digit];
	    if (digit >= (unsigned) base) {
		break;
	    }
	    shifted = result * base;
	    if ((shifted/base) != result) {
		goto overflow;
	    }
	    result = shifted + digit;
	    if ( result < shifted ) {
		goto overflow;
	    }
	    anyDigits = 1;
	}
    }

    /*
     * Negate if we found a '-' earlier.
     */

    if (negative) {
	result = (Tcl_WideUInt)(-((Tcl_WideInt)result));
    }

    /*
     * See if there were any digits at all.
     */

    if (!anyDigits) {
	p = string;
    }

    if (endPtr != 0) {
	*endPtr = (char *) p;
    }

    return result;

    /*
     * On overflow generate the right output
     */

 overflow:
    errno = ERANGE;
    if (endPtr != 0) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > ('z' - '0')) {
		break;
	    }
	    digit = cvtIn[digit];
	    if (digit >= (unsigned) base) {
		break;
	    }
	}
	*endPtr = (char *) p;
    }
    return (Tcl_WideUInt)Tcl_LongAsWide(-1);
}
