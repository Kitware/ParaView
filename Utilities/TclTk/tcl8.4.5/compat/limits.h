/*
 * limits.h --
 *
 *	This is a dummy header file to #include in Tcl when there
 *	is no limits.h in /usr/include.  There are only a few
 *	definitions here;  also see tclPort.h, which already
 *	#defines some of the things here if they're not arleady
 *	defined.
 *
 * Copyright (c) 1991 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define LONG_MIN		0x80000000
#define LONG_MAX		0x7fffffff
#define INT_MIN			0x80000000
#define INT_MAX			0x7fffffff
#define SHRT_MIN		0x8000
#define SHRT_MAX		0x7fff
