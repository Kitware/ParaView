/* 
 * tclStubLib.c --
 *
 *	Stub object that will be statically linked into extensions that wish
 *	to access Tcl.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 1998 Paul Duffin.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

/*
 * We need to ensure that we use the stub macros so that this file contains
 * no references to any of the stub functions.  This will make it possible
 * to build an extension that references Tcl_InitStubs but doesn't end up
 * including the rest of the stub functions.
 */

#ifndef USE_TCL_STUBS
#define USE_TCL_STUBS
#endif
#undef USE_TCL_STUB_PROCS

#include "tclInt.h"
#include "tclPort.h"

/*
 * Ensure that Tcl_InitStubs is built as an exported symbol.  The other stub
 * functions should be built as non-exported symbols.
 */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

TclStubs *tclStubsPtr;
TclPlatStubs *tclPlatStubsPtr;
TclIntStubs *tclIntStubsPtr;
TclIntPlatStubs *tclIntPlatStubsPtr;

static TclStubs *	HasStubSupport _ANSI_ARGS_((Tcl_Interp *interp));

static TclStubs *
HasStubSupport (interp)
    Tcl_Interp *interp;
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr->stubTable && (iPtr->stubTable->magic == TCL_STUB_MAGIC)) {
	return iPtr->stubTable;
    }
    interp->result = "This interpreter does not support stubs-enabled extensions.";
    interp->freeProc = TCL_STATIC;

    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitStubs --
 *
 *	Tries to initialise the stub table pointers and ensures that
 *	the correct version of Tcl is loaded.
 *
 * Results:
 *	The actual version of Tcl that satisfies the request, or
 *	NULL to indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

#ifdef Tcl_InitStubs
#undef Tcl_InitStubs
#endif

char *
Tcl_InitStubs (interp, version, exact)
    Tcl_Interp *interp;
    char *version;
    int exact;
{
    char *actualVersion;
    TclStubs *tmp;
    
    if (!tclStubsPtr) {
	tclStubsPtr = HasStubSupport(interp);
	if (!tclStubsPtr) {
            return NULL;
        }
    }

    actualVersion = Tcl_PkgRequireEx(interp, "Tcl", version, exact,
	    (ClientData *) &tmp);
    if (actualVersion == NULL) {
	tclStubsPtr = NULL;
	return NULL;
    }

    if (tclStubsPtr->hooks) {
	tclPlatStubsPtr = tclStubsPtr->hooks->tclPlatStubs;
	tclIntStubsPtr = tclStubsPtr->hooks->tclIntStubs;
	tclIntPlatStubsPtr = tclStubsPtr->hooks->tclIntPlatStubs;
    } else {
	tclPlatStubsPtr = NULL;
	tclIntStubsPtr = NULL;
	tclIntPlatStubsPtr = NULL;
    }
    
    return actualVersion;
}
