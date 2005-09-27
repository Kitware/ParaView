/* 
 * tclLoadDyld.c --
 *
 *     This procedure provides a version of the TclLoadFile that
 *     works with Apple's dyld dynamic loading.  This file
 *     provided by Wilfredo Sanchez (wsanchez@apple.com).
 *     This works on Mac OS X.
 *
 * Copyright (c) 1995 Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include <mach-o/dyld.h>

typedef struct Tcl_DyldModuleHandle {
    struct Tcl_DyldModuleHandle *nextModuleHandle;
    NSModule module;
} Tcl_DyldModuleHandle;

typedef struct Tcl_DyldLoadHandle {
    const struct mach_header *dyld_lib;
    Tcl_DyldModuleHandle *firstModuleHandle;
} Tcl_DyldLoadHandle;

/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *	Dynamically loads a binary code file into memory and returns
 *	a handle to the new code.
 *
 * Results:
 *     A standard Tcl completion code.  If an error occurs, an error
 *     message is left in the interpreter's result. 
 *
 * Side effects:
 *     New code suddenly appears in memory.
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
    Tcl_DyldLoadHandle *dyldLoadHandle;
    const struct mach_header *dyld_lib;
    CONST char *native;

    /* 
     * First try the full path the user gave us.  This is particularly
     * important if the cwd is inside a vfs, and we are trying to load
     * using a relative path.
     */
    native = Tcl_FSGetNativePath(pathPtr);
    dyld_lib = NSAddImage(native, 
			  NSADDIMAGE_OPTION_WITH_SEARCHING | 
			  NSADDIMAGE_OPTION_RETURN_ON_ERROR);
    
    if (!dyld_lib) {
	/* 
	 * Let the OS loader examine the binary search path for
	 * whatever string the user gave us which hopefully refers
	 * to a file on the binary path
	 */
	Tcl_DString ds;
	char *fileName = Tcl_GetString(pathPtr);
	native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
	dyld_lib = NSAddImage(native, 
			      NSADDIMAGE_OPTION_WITH_SEARCHING | 
			      NSADDIMAGE_OPTION_RETURN_ON_ERROR);
	Tcl_DStringFree(&ds);
    }
    
    if (!dyld_lib) {
        NSLinkEditErrors editError;
        char *name, *msg;
        NSLinkEditError(&editError, &errno, &name, &msg);
        Tcl_AppendResult(interp, msg, (char *) NULL);
        return TCL_ERROR;
    }
    
    dyldLoadHandle = (Tcl_DyldLoadHandle *) ckalloc(sizeof(Tcl_DyldLoadHandle));
    if (!dyldLoadHandle) return TCL_ERROR;
    dyldLoadHandle->dyld_lib = dyld_lib;
    dyldLoadHandle->firstModuleHandle = NULL;
    *loadHandle = (Tcl_LoadHandle) dyldLoadHandle;
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
    NSSymbol nsSymbol;
    CONST char *native;
    Tcl_DString newName, ds;
    Tcl_PackageInitProc* proc = NULL;
    Tcl_DyldLoadHandle *dyldLoadHandle = (Tcl_DyldLoadHandle *) loadHandle;
    /* 
     * dyld adds an underscore to the beginning of symbol names.
     */

    native = Tcl_UtfToExternalDString(NULL, symbol, -1, &ds);
    Tcl_DStringInit(&newName);
    Tcl_DStringAppend(&newName, "_", 1);
    native = Tcl_DStringAppend(&newName, native, -1);
    nsSymbol = NSLookupSymbolInImage(dyldLoadHandle->dyld_lib, native, 
	NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW | 
	NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
    if(nsSymbol) {
	Tcl_DyldModuleHandle *dyldModuleHandle;
	proc = NSAddressOfSymbol(nsSymbol);
	dyldModuleHandle = (Tcl_DyldModuleHandle *) ckalloc(sizeof(Tcl_DyldModuleHandle));
	if (dyldModuleHandle) {
	    dyldModuleHandle->module = NSModuleForSymbol(nsSymbol);
	    dyldModuleHandle->nextModuleHandle = dyldLoadHandle->firstModuleHandle;
	    dyldLoadHandle->firstModuleHandle = dyldModuleHandle;
	}
    } else {
        NSLinkEditErrors editError;
        char *name, *msg;
        NSLinkEditError(&editError, &errno, &name, &msg);
        Tcl_AppendResult(interp, msg, (char *) NULL);
    }
    Tcl_DStringFree(&newName);
    Tcl_DStringFree(&ds);
    
    return proc;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpUnloadFile --
 *
 *     Unloads a dynamically loaded binary code file from memory.
 *     Code pointers in the formerly loaded file are no longer valid
 *     after calling this function.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Code dissapears from memory.
 *     Note that this is a no-op on older (OpenStep) versions of dyld.
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
    Tcl_DyldLoadHandle *dyldLoadHandle = (Tcl_DyldLoadHandle *) loadHandle;
    Tcl_DyldModuleHandle *dyldModuleHandle = dyldLoadHandle->firstModuleHandle;
    void *ptr;

    while (dyldModuleHandle) {
	NSUnLinkModule(dyldModuleHandle->module, NSUNLINKMODULE_OPTION_NONE);
	ptr = dyldModuleHandle;
	dyldModuleHandle = dyldModuleHandle->nextModuleHandle;
	ckfree(ptr);
    }
    ckfree(dyldLoadHandle);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *     If the "load" command is invoked without providing a package
 *     name, this procedure is invoked to try to figure it out.
 *
 * Results:
 *     Always returns 0 to indicate that we couldn't figure out a
 *     package name;  generic code will then try to guess the package
 *     from the file name.  A return value of 1 would have meant that
 *     we figured out the package name and put it in bufPtr.
 *
 * Side effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(fileName, bufPtr)
    CONST char *fileName;      /* Name of file containing package (already
				* translated to local form if needed). */
    Tcl_DString *bufPtr;       /* Initialized empty dstring.  Append
				* package name to this if possible. */
{
    return 0;
}
