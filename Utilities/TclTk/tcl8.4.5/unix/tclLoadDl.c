/* 
 * tclLoadDl.c --
 *
 *	This procedure provides a version of the TclLoadFile that
 *	works with the "dlopen" and "dlsym" library procedures for
 *	dynamic loading.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#ifdef NO_DLFCN_H
#   include "../compat/dlfcn.h"
#else
#   include <dlfcn.h>
#endif

/*
 * In some systems, like SunOS 4.1.3, the RTLD_NOW flag isn't defined
 * and this argument to dlopen must always be 1.  The RTLD_GLOBAL
 * flag is needed on some systems (e.g. SCO and UnixWare) but doesn't
 * exist on others;  if it doesn't exist, set it to 0 so it has no effect.
 */

#ifndef RTLD_NOW
#   define RTLD_NOW 1
#endif

#ifndef RTLD_GLOBAL
#   define RTLD_GLOBAL 0
#endif

/*
 *---------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------
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
    VOID *handle;
    CONST char *native;

    /* 
     * First try the full path the user gave us.  This is particularly
     * important if the cwd is inside a vfs, and we are trying to load
     * using a relative path.
     */
    native = Tcl_FSGetNativePath(pathPtr);
    handle = dlopen(native, RTLD_NOW | RTLD_GLOBAL);
    if (handle == NULL) {
	/* 
	 * Let the OS loader examine the binary search path for
	 * whatever string the user gave us which hopefully refers
	 * to a file on the binary path
	 */
	Tcl_DString ds;
	char *fileName = Tcl_GetString(pathPtr);
	native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
	handle = dlopen(native, RTLD_NOW | RTLD_GLOBAL);
	Tcl_DStringFree(&ds);
    }
    
    if (handle == NULL) {
	Tcl_AppendResult(interp, "couldn't load file \"", 
			 Tcl_GetString(pathPtr),
			 "\": ", dlerror(), (char *) NULL);
	return TCL_ERROR;
    }

    *unloadProcPtr = &TclpUnloadFile;
    *loadHandle = (Tcl_LoadHandle)handle;
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
    CONST char *native;
    Tcl_DString newName, ds;
    VOID *handle = (VOID*)loadHandle;
    Tcl_PackageInitProc *proc;
    /* 
     * Some platforms still add an underscore to the beginning of symbol
     * names.  If we can't find a name without an underscore, try again
     * with the underscore.
     */

    native = Tcl_UtfToExternalDString(NULL, symbol, -1, &ds);
    proc = (Tcl_PackageInitProc *) dlsym(handle,	/* INTL: Native. */
	    native);	
    if (proc == NULL) {
	Tcl_DStringInit(&newName);
	Tcl_DStringAppend(&newName, "_", 1);
	native = Tcl_DStringAppend(&newName, native, -1);
	proc = (Tcl_PackageInitProc *) dlsym(handle, /* INTL: Native. */
		native);
	Tcl_DStringFree(&newName);
    }
    Tcl_DStringFree(&ds);

    return proc;
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
    VOID *handle;

    handle = (VOID *) loadHandle;
    dlclose(handle);
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
