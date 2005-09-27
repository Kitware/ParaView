/* 
 * tclLoadNext.c --
 *
 *	This procedure provides a version of the TclLoadFile that
 *	works with NeXTs rld_* dynamic loading.  This file provided
 *	by Pedja Bogdanovich.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include <mach-o/rld.h>
#include <streams/streams.h>

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
    struct mach_header *header;
    char *fileName;
    char *files[2];
    CONST char *native;
    int result = 1;
    
    NXStream *errorStream = NXOpenMemory(0,0,NX_READWRITE);
    
    fileName = Tcl_GetString(pathPtr);

    /* 
     * First try the full path the user gave us.  This is particularly
     * important if the cwd is inside a vfs, and we are trying to load
     * using a relative path.
     */
    native = Tcl_FSGetNativePath(pathPtr);
    files = {native,NULL};

    result = rld_load(errorStream, &header, files, NULL);
    
    if (!result) {
	/* 
	 * Let the OS loader examine the binary search path for
	 * whatever string the user gave us which hopefully refers
	 * to a file on the binary path
	 */
	Tcl_DString ds;
	native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
	files = {native,NULL};
	result = rld_load(errorStream, &header, files, NULL);
	Tcl_DStringFree(&ds);
    }
    
    if (!result) {
	char *data;
	int len, maxlen;
	NXGetMemoryBuffer(errorStream,&data,&len,&maxlen);
	Tcl_AppendResult(interp, "couldn't load file \"",
			 fileName, "\": ", data, NULL);
	NXCloseMemory(errorStream, NX_FREEBUFFER);
	return TCL_ERROR;
    }
    NXCloseMemory(errorStream, NX_FREEBUFFER);
    
    *loadHandle = (Tcl_LoadHandle)1; /* A dummy non-NULL value */
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
    Tcl_PackageInitProc *proc=NULL;
    if(symbol) {
	char sym[strlen(symbol)+2];
	sym[0]='_'; sym[1]=0; strcat(sym,symbol);
	rld_lookup(NULL,sym,(unsigned long *)&proc);
    }
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
 *	Does nothing.  Can anything be done?
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
