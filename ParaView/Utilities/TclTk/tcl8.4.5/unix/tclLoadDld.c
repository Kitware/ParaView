/* 
 * tclLoadDld.c --
 *
 *	This procedure provides a version of the TclLoadFile that
 *	works with the "dld_link" and "dld_get_func" library procedures
 *	for dynamic loading.  It has been tested on Linux 1.1.95 and
 *	dld-3.2.7.  This file probably isn't needed anymore, since it
 *	makes more sense to use "dl_open" etc.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "dld.h"

/*
 * In some systems, like SunOS 4.1.3, the RTLD_NOW flag isn't defined
 * and this argument to dlopen must always be 1.
 */

#ifndef RTLD_NOW
#   define RTLD_NOW 1
#endif

/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *	Dynamically loads a binary code file into memory and returns
 *	a handle to the new code.
 *
 * Results:
 *	A standard Tcl completion code.  If an error occurs, an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	New code suddenly appears in memory.
 *
 *----------------------------------------------------------------------
 */

int
TclpDlopen(interp, pathPtr, loadHandle, unloadProcPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Obj *pathPtr;		/* Name of the file containing the desired
				 * code (UTF-8). */
    Tcl_LoadHandle *loadHandle;	/* Filled with token for dynamically loaded
				 * file which will be passed back to 
				 * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr;	
				/* Filled with address of Tcl_FSUnloadFileProc
				 * function which should be used for
				 * this file. */
{
    static int firstTime = 1;
    int returnCode;
    char *fileName;
    CONST char *native;
    
    /*
     *  The dld package needs to know the pathname to the tcl binary.
     *  If that's not known, return an error.
     */

    if (firstTime) {
	if (tclExecutableName == NULL) {
	    Tcl_SetResult(interp,
		    "don't know name of application binary file, so can't initialize dynamic loader",
		    TCL_STATIC);
	    return TCL_ERROR;
	}
	returnCode = dld_init(tclExecutableName);
	if (returnCode != 0) {
	    Tcl_AppendResult(interp,
		    "initialization failed for dynamic loader: ",
		    dld_strerror(returnCode), (char *) NULL);
	    return TCL_ERROR;
	}
	firstTime = 0;
    }

    fileName = Tcl_GetString(pathPtr);

    /* 
     * First try the full path the user gave us.  This is particularly
     * important if the cwd is inside a vfs, and we are trying to load
     * using a relative path.
     */
    native = Tcl_FSGetNativePath(pathPtr);
    returnCode = dld_link(native);
    
    if (returnCode != 0) {
	Tcl_DString ds;
	native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
	returnCode = dld_link(native);
	Tcl_DStringFree(&ds);
    }

    if (returnCode != 0) {
	Tcl_AppendResult(interp, "couldn't load file \"", 
			 fileName, "\": ", 
			 dld_strerror(returnCode), (char *) NULL);
	return TCL_ERROR;
    }
    *loadHandle = (Tcl_LoadHandle) strcpy(
	    (char *) ckalloc((unsigned) (strlen(fileName) + 1)), fileName);
    *unloadProcPtr = &TclpUnloadFile;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindSymbol --
 *
 *	Looks up a symbol, by name, through a handle associated with
 *	a previously loaded piece of code (shared library).
 *
 * Results:
 *	Returns a pointer to the function associated with 'symbol' if
 *	it is found.  Otherwise returns NULL and may leave an error
 *	message in the interp's result.
 *
 *----------------------------------------------------------------------
 */
Tcl_PackageInitProc*
TclpFindSymbol(interp, loadHandle, symbol) 
    Tcl_Interp *interp;
    Tcl_LoadHandle loadHandle;
    CONST char *symbol;
{
    return (Tcl_PackageInitProc *) dld_get_func(symbol);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpUnloadFile --
 *
 *	Unloads a dynamically loaded binary code file from memory.
 *	Code pointers in the formerly loaded file are no longer valid
 *	after calling this function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Code removed from memory.
 *
 *----------------------------------------------------------------------
 */

void
TclpUnloadFile(loadHandle)
    Tcl_LoadHandle loadHandle;	/* loadHandle returned by a previous call
				 * to TclpDlopen().  The loadHandle is 
				 * a token that represents the loaded 
				 * file. */
{
    char *fileName;

    handle = (char *) loadHandle;
    dld_unlink_by_file(handle, 0);
    ckfree(handle);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *	If the "load" command is invoked without providing a package
 *	name, this procedure is invoked to try to figure it out.
 *
 * Results:
 *	Always returns 0 to indicate that we couldn't figure out a
 *	package name;  generic code will then try to guess the package
 *	from the file name.  A return value of 1 would have meant that
 *	we figured out the package name and put it in bufPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(fileName, bufPtr)
    CONST char *fileName;	/* Name of file containing package (already
				 * translated to local form if needed). */
    Tcl_DString *bufPtr;	/* Initialized empty dstring.  Append
				 * package name to this if possible. */
{
    return 0;
}
