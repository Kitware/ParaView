/* 
 * tclMacDNR.c
 *
 *	This file actually just includes the file "dnr.c" provided by
 *	Apple Computer and redistributed by MetroWerks (and other compiler
 *	vendors.)  Unfortunantly, despite various bug reports, dnr.c uses
 *	C++ style comments and will not compile under the "ANSI Strict"
 *	mode that the rest of Tcl compiles under.  Furthermore, the Apple
 *	license prohibits me from redistributing a corrected version of
 *	dnr.c.  This file uses a pragma to turn off the Strict ANSI option
 *	and then includes the dnr.c file.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#pragma ANSI_strict off
#include <dnr.c>
#pragma ANSI_strict reset
