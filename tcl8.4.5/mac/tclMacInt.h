/*
 * tclMacInt.h --
 *
 *	Declarations of Macintosh specific shared variables and procedures.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCLMACINT
#define _TCLMACINT

#ifndef _TCLINT
#include "tclInt.h"
#endif
#ifndef _TCLPORT
#include "tclPort.h"
#endif

#include <Events.h>
#include <Files.h>

/*
 * Defines to control stack behavior.
 *
 * The Tcl8.2 regexp code is highly recursive for patterns with many
 * subexpressions.  So we have to increase the stack space to accomodate.
 * 512 K is good enough for ordinary work, but you need 768 to pass the Tcl
 * regexp testsuite.
 *
 * For the PPC, you need to set the stack space in the Project file.
 *
 */

#ifdef TCL_TEST
#	define TCL_MAC_68K_STACK_GROWTH (768*1024)
#else
#	define TCL_MAC_68K_STACK_GROWTH (512*1024)
#endif

#define TCL_MAC_STACK_THRESHOLD 16384

#ifdef BUILD_tcl
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * This flag is passed to TclMacRegisterResourceFork
 * by a file (usually a library) whose resource fork
 * should not be closed by the resource command.
 */
 
#define TCL_RESOURCE_DONT_CLOSE  2

/*
 * Typedefs used by Macintosh parts of Tcl.
 */

/*
 * Prototypes of Mac only internal functions.
 */

EXTERN char *	TclMacGetFontEncoding _ANSI_ARGS_((int fontId));
EXTERN int		TclMacHaveThreads _ANSI_ARGS_((void));
EXTERN long		TclpGetGMTOffset _ANSI_ARGS_((void));

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#include "tclIntPlatDecls.h"
    
#endif /* _TCLMACINT */
