/* 
 * tkStubImg.c --
 *
 *	Stub object that will be statically linked into extensions that wish
 *	to access Tk.
 *
 * Copyright (c) 1999 Jan Nijtmans.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"


/*
 *----------------------------------------------------------------------
 *
 * Tk_InitImageArgs --
 *
 *	Performs the necessary conversion from Tcl_Obj's to strings
 *      in the createProc for Tcl_CreateImageType. If running under
 *      Tk 8.2 or earlier without the Img-patch, this function has
 *      no effect.
 *
 * Results:
 *	argvPtr will point to an argument list which is guaranteed to
 *	contain strings, no matter what Tk version is running.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

#ifdef Tk_InitImageArgs
#undef Tk_InitImageArgs
#endif

void
Tk_InitImageArgs(interp, argc, argvPtr)
    Tcl_Interp *interp;
    int argc;
    char ***argvPtr;
{
    static int useNewImage = -1;
    static char **argv = NULL;

    if (argv) {
	tclStubsPtr->tcl_Free((char *) argv);
	argv = NULL;
    }

    if (useNewImage < 0) {
	Tcl_CmdInfo cmdInfo;
	if (!tclStubsPtr->tcl_GetCommandInfo(interp,"image", &cmdInfo)) {
	    tclStubsPtr->tcl_Panic("cannot find the \"image\" command");
	}
	if (cmdInfo.isNativeObjectProc == 1) {
	    useNewImage = 1; /* Tk uses the new image interface */
	} else {
	    useNewImage = 0; /* Tk uses old image interface */
	}
    }
    if (useNewImage && (argc > 0)) {
	int i;
	argv = (char **) tclStubsPtr->tcl_Alloc(argc * sizeof(char *));
	for (i = 0; i < argc; i++) {
	    argv[i] = tclStubsPtr->tcl_GetString((Tcl_Obj *)(*argvPtr)[i]);
	}
	*argvPtr = (char **) argv;
    }
}
