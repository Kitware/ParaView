/* 
 * tkStubLib.c --
 *
 *	Stub object that will be statically linked into extensions that wish
 *	to access Tk.
 *
 * Copyright (c) 1998 Paul Duffin.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

/*
 * Because of problems with pre-compiled headers on the Mac, we need to
 * do these includes before we add the stubs defines.  This a hack.
 */

#ifdef MAC_TCL
#include "tkMacInt.h"
#include "tkInt.h"
#include "tkPort.h"
#endif /* MAC_TCL */

/*
 * We need to ensure that we use the stub macros so that this file contains
 * no references to any of the stub functions.  This will make it possible
 * to build an extension that references Tk_InitStubs but doesn't end up
 * including the rest of the stub functions.
 */

#ifndef USE_TCL_STUBS
#define USE_TCL_STUBS
#endif
#undef USE_TCL_STUB_PROCS

#ifndef USE_TK_STUBS
#define USE_TK_STUBS
#endif
#undef USE_TK_STUB_PROCS

#ifndef MAC_TCL

#include "tkPort.h"
#include "tkInt.h"

#ifdef __WIN32__
#include "tkWinInt.h"
#endif

#endif /* !MAC_TCL */

#include "tkDecls.h"
#include "tkIntDecls.h"
#include "tkPlatDecls.h"
#include "tkIntPlatDecls.h"
#include "tkIntXlibDecls.h"

/*
 * Ensure that Tk_InitStubs is built as an exported symbol.  The other stub
 * functions should be built as non-exported symbols.
 */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

TkStubs *tkStubsPtr;
TkPlatStubs *tkPlatStubsPtr;
TkIntStubs *tkIntStubsPtr;
TkIntPlatStubs *tkIntPlatStubsPtr;
TkIntXlibStubs *tkIntXlibStubsPtr;


/*
 *----------------------------------------------------------------------
 *
 * Tk_InitStubs --
 *
 *	Checks that the correct version of Tk is loaded and that it
 *	supports stubs. It then initialises the stub table pointers.
 *
 * Results:
 *	The actual version of Tk that satisfies the request, or
 *	NULL to indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

#ifdef Tk_InitStubs
#undef Tk_InitStubs
#endif

char *
Tk_InitStubs(interp, version, exact)
    Tcl_Interp *interp;
    char *version;
    int exact;
{
    char *actualVersion;

    actualVersion = Tcl_PkgRequireEx(interp, "Tk", version, exact,
		(ClientData *) &tkStubsPtr);
    if (!actualVersion) {
	return NULL;
    }

    if (!tkStubsPtr) {
	Tcl_SetResult(interp,
		"This implementation of Tk does not support stubs",
		TCL_STATIC);
	return NULL;
    }
    
    tkPlatStubsPtr = tkStubsPtr->hooks->tkPlatStubs;
    tkIntStubsPtr = tkStubsPtr->hooks->tkIntStubs;
    tkIntPlatStubsPtr = tkStubsPtr->hooks->tkIntPlatStubs;
    tkIntXlibStubsPtr = tkStubsPtr->hooks->tkIntXlibStubs;
    
    return actualVersion;
}
