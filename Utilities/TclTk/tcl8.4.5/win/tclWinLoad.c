/* 
 * tclWinLoad.c --
 *
 *	This procedure provides a version of the TclLoadFile that
 *	works with the Windows "LoadLibrary" and "GetProcAddress"
 *	API for dynamic loading.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"


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
    HINSTANCE handle;
    CONST TCHAR *nativeName;

    /* 
     * First try the full path the user gave us.  This is particularly
     * important if the cwd is inside a vfs, and we are trying to load
     * using a relative path.
     */
    nativeName = Tcl_FSGetNativePath(pathPtr);
    handle = (*tclWinProcs->loadLibraryProc)(nativeName);
    if (handle == NULL) {
	/* 
	 * Let the OS loader examine the binary search path for
	 * whatever string the user gave us which hopefully refers
	 * to a file on the binary path
	 */
	Tcl_DString ds;
        char *fileName = Tcl_GetString(pathPtr);
	nativeName = Tcl_WinUtfToTChar(fileName, -1, &ds);
	handle = (*tclWinProcs->loadLibraryProc)(nativeName);
	Tcl_DStringFree(&ds);
    }

    *loadHandle = (Tcl_LoadHandle) handle;
    
    if (handle == NULL) {
	DWORD lastError = GetLastError();
#if 0
	/*
	 * It would be ideal if the FormatMessage stuff worked better,
	 * but unfortunately it doesn't seem to want to...
	 */
	LPTSTR lpMsgBuf;
	char *buf;
	int size;
	size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, lastError, 0,
		(LPTSTR) &lpMsgBuf, 0, NULL);
	buf = (char *) ckalloc((unsigned) TCL_INTEGER_SPACE + size + 1);
	sprintf(buf, "%d %s", lastError, (char *)lpMsgBuf);
#endif
	Tcl_AppendResult(interp, "couldn't load library \"",
			 Tcl_GetString(pathPtr), "\": ", (char *) NULL);
	/*
	 * Check for possible DLL errors.  This doesn't work quite right,
	 * because Windows seems to only return ERROR_MOD_NOT_FOUND for
	 * just about any problem, but it's better than nothing.  It'd be
	 * even better if there was a way to get what DLLs
	 */
	switch (lastError) {
	    case ERROR_MOD_NOT_FOUND:
	    case ERROR_DLL_NOT_FOUND:
		Tcl_AppendResult(interp, "this library or a dependent library",
			" could not be found in library path",
			(char *) NULL);
		break;
	    case ERROR_PROC_NOT_FOUND:
		Tcl_AppendResult(interp, "could not find specified procedure",
			(char *) NULL);
		break;
	    case ERROR_INVALID_DLL:
		Tcl_AppendResult(interp, "this library or a dependent library",
			" is damaged", (char *) NULL);
		break;
	    case ERROR_DLL_INIT_FAILED:
		Tcl_AppendResult(interp, "the library initialization",
			" routine failed", (char *) NULL);
		break;
	    default:
		TclWinConvertError(lastError);
		Tcl_AppendResult(interp, Tcl_PosixError(interp),
			(char *) NULL);
	}
	return TCL_ERROR;
    } else {
	*unloadProcPtr = &TclpUnloadFile;
    }
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
    Tcl_PackageInitProc *proc = NULL;
    HINSTANCE handle = (HINSTANCE)loadHandle;

    /*
     * For each symbol, check for both Symbol and _Symbol, since Borland
     * generates C symbols with a leading '_' by default.
     */

    proc = (Tcl_PackageInitProc *) GetProcAddress(handle, symbol);
    if (proc == NULL) {
	Tcl_DString ds;
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, "_", 1);
	symbol = Tcl_DStringAppend(&ds, symbol, -1);
	proc = (Tcl_PackageInitProc *) GetProcAddress(handle, symbol);
	Tcl_DStringFree(&ds);
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
    HINSTANCE handle;

    handle = (HINSTANCE) loadHandle;
    FreeLibrary(handle);
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
