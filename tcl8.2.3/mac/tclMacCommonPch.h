/*
 * tclMacCommonPch.h --
 *
 * 	Macintosh Tcl must be compiled with certain compiler options to
 *	ensure that it will work correctly. The following pragmas are
 *	used to ensure that those options are set correctly. An error
 * 	will occur at compile time if they are not set correctly.
 *
 * Copyright (c) 1998 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#if !__option(enumsalwaysint)
#error Tcl requires the Metrowerks setting "Enums always ints".
#endif


#if !defined(__POWERPC__)
#if !__option(far_data)
#error Tcl requires the Metrowerks setting "Far data".
#endif
#endif


#if !defined(__POWERPC__)
#if !__option(fourbyteints)
#error Tcl requires the Metrowerks setting "4 byte ints".
#endif
#endif


#if !defined(__POWERPC__)
#if !__option(IEEEdoubles)
#error Tcl requires the Metrowerks setting "8 byte doubles".
#endif
#endif


/*
* The define is used most everywhere to tell Tcl (or any Tcl
* extensions) that we are compiling for the Macintosh platform.
*/


#define MAC_TCL


/*
* The following defines control the behavior of the Macintosh
* Universial Headers.
*/


#define SystemSevenOrLater 1
#define STRICT_CONTROLS 1
#define STRICT_WINDOWS 1


/*
* Define the following symbol if you want
* comprehensive debugging turned on.
*/


/* #define TCL_DEBUG */


#ifdef TCL_DEBUG
# define TCL_MEM_DEBUG
# define TCL_TEST
#endif



/*
* For a while, we will continue to use the old routine names, so that
* people with older versions of CodeWarrior will still be able to compile
* the source (albeit they will have to update the project files themselves).
*
* At some point, we will convert over to the new routine names.
*/


#define OLDROUTINENAMES 1
