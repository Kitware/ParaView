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
 * TclpLoadFile --
 *
 *	Dynamically loads a binary code file into memory and returns
 *	the addresses of two procedures within that file, if they
 *	are defined.
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
TclpLoadFile(interp, fileName, sym1, sym2, proc1Ptr, proc2Ptr, clientDataPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    char *fileName;		/* Name of the file containing the desired
				 * code. */
    char *sym1, *sym2;		/* Names of two procedures to look up in
				 * the file's symbol table. */
    Tcl_PackageInitProc **proc1Ptr, **proc2Ptr;
				/* Where to return the addresses corresponding
				 * to sym1 and sym2. */
    ClientData *clientDataPtr;	/* Filled with token for dynamically loaded
				 * file which will be passed back to 
				 * TclpUnloadFile() to unload the file. */
{
    HINSTANCE handle;
    TCHAR *nativeName;
    Tcl_DString ds;

    nativeName = Tcl_WinUtfToTChar(fileName, -1, &ds);
    handle = (*tclWinProcs->loadLibraryProc)(nativeName);
    Tcl_DStringFree(&ds);

    *clientDataPtr = (ClientData) handle;
    
    if (handle == NULL) {
	TclWinConvertError(GetLastError());
	Tcl_AppendResult(interp, "couldn't load file \"", fileName,
		"\": ", Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * For each symbol, check for both Symbol and _Symbol, since Borland
     * generates C symbols with a leading '_' by default.
     */

    *proc1Ptr = (Tcl_PackageInitProc *) GetProcAddress(handle, sym1);
    if (*proc1Ptr == NULL) {
	Tcl_DStringAppend(&ds, "_", 1);
	sym1 = Tcl_DStringAppend(&ds, sym1, -1);
	*proc1Ptr = (Tcl_PackageInitProc *) GetProcAddress(handle, sym1);
	Tcl_DStringFree(&ds);
    }
    
    *proc2Ptr = (Tcl_PackageInitProc *) GetProcAddress(handle, sym2);
    if (*proc2Ptr == NULL) {
	Tcl_DStringAppend(&ds, "_", 1);
	sym2 = Tcl_DStringAppend(&ds, sym2, -1);
	*proc2Ptr = (Tcl_PackageInitProc *) GetProcAddress(handle, sym2);
	Tcl_DStringFree(&ds);
    }
    return TCL_OK;
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
TclpUnloadFile(clientData)
    ClientData clientData;	/* ClientData returned by a previous call
				 * to TclpLoadFile().  The clientData is 
				 * a token that represents the loaded 
				 * file. */
{
    HINSTANCE handle;

    handle = (HINSTANCE) clientData;
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
    char *fileName;		/* Name of file containing package (already
				 * translated to local form if needed). */
    Tcl_DString *bufPtr;	/* Initialized empty dstring.  Append
				 * package name to this if possible. */
{
    return 0;
}
