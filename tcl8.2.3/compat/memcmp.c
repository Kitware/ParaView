/* 
 * memcmp.c --
 *
 *	Source code for the "memcmp" library routine.
 *
 * Copyright (c) 1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) memcmp.c 1.2 98/01/19 10:48:58
 */

#include "tcl.h"
#include "tclPort.h"

/*
 * Here is the prototype just in case it is not included
 * in tclPort.h.
 */

int		memcmp _ANSI_ARGS_((CONST VOID *s1,
			    CONST VOID *s2, size_t n));

/*
 *----------------------------------------------------------------------
 *
 * memcmp --
 *
 *	Compares two bytes sequences.
 *
 * Results:
 *     compares  its  arguments, looking at the first n
 *     bytes (each interpreted as an unsigned char), and  returns
 *     an integer less than, equal to, or greater than 0, accord-
 *     ing as s1 is less  than,  equal  to,  or
 *     greater than s2 when taken to be unsigned 8 bit numbers.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
memcmp(s1, s2, n)
    CONST VOID *s1;			/* First string. */
    CONST VOID *s2;			/* Second string. */
    size_t      n;                      /* Length to compare. */
{
    unsigned char u1, u2;

    for ( ; n-- ; s1++, s2++) {
	u1 = * (unsigned char *) s1;
	u2 = * (unsigned char *) s2;
	if ( u1 != u2) {
	    return (u1-u2);
	}
    }
    return 0;
}
