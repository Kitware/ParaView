/* 
 * tkWinInit.c --
 *
 *	This file contains Windows-specific interpreter initialization
 *	functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"

/*
 * The Init script (common to Windows and Unix platforms) is
 * defined in tkInitScript.h
 */
#include "tkInitScript.h"


/*
 *----------------------------------------------------------------------
 *
 * TkpInit --
 *
 *	Performs Windows-specific interpreter initialization related to the
 *      tk_library variable.
 *
 * Results:
 *	A standard Tcl completion code (TCL_OK or TCL_ERROR).  Also
 *	leaves information in the interp's result.
 *
 * Side effects:
 *	Sets "tk_library" Tcl variable, runs "tk.tcl" script.
 *
 *----------------------------------------------------------------------
 */

int
TkpInit(interp)
    Tcl_Interp *interp;
{
    /*
     * This is necessary for static initialization, and is ok otherwise
     * because TkWinXInit flips a static bit to do its work just once.
     */
    TkWinXInit(Tk_GetHINSTANCE());
    return Tcl_Eval(interp, initScript);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetAppName --
 *
 *	Retrieves the name of the current application from a platform
 *	specific location.  For Windows, the application name is the
 *	root of the tail of the path contained in the tcl variable argv0.
 *
 * Results:
 *	Returns the application name in the given Tcl_DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetAppName(interp, namePtr)
    Tcl_Interp *interp;
    Tcl_DString *namePtr;	/* A previously initialized Tcl_DString. */
{
    int argc, namelength;
    CONST char **argv = NULL, *name, *p;

    name = Tcl_GetVar(interp, "argv0", TCL_GLOBAL_ONLY);
    namelength = -1;
    if (name != NULL) {
	Tcl_SplitPath(name, &argc, &argv);
	if (argc > 0) {
	    name = argv[argc-1];
	    p = strrchr(name, '.');
	    if (p != NULL) {
		namelength = p - name;
	    }
	} else {
	    name = NULL;
	}
    }
    if ((name == NULL) || (*name == 0)) {
	name = "tk";
	namelength = -1;
    }
    Tcl_DStringAppend(namePtr, name, namelength);
    if (argv != NULL) {
	ckfree((char *)argv);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayWarning --
 *
 *	This routines is called from Tk_Main to display warning
 *	messages that occur during startup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Displays a message box.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayWarning(msg, title)
    CONST char *msg;		/* Message to be displayed. */
    CONST char *title;		/* Title of warning. */
{
    Tcl_DString msgString, titleString;
    Tcl_Encoding unicodeEncoding = TkWinGetUnicodeEncoding();

    /*
     * Truncate MessageBox string if it is too long to not overflow
     * the screen and cause possible oversized window error.
     */
#define TK_MAX_WARN_LEN (1024 * sizeof(WCHAR))
    Tcl_UtfToExternalDString(unicodeEncoding, msg, -1, &msgString);
    Tcl_UtfToExternalDString(unicodeEncoding, title, -1, &titleString);
    if (Tcl_DStringLength(&msgString) > TK_MAX_WARN_LEN) {
	Tcl_DStringSetLength(&msgString, TK_MAX_WARN_LEN);
	Tcl_DStringAppend(&msgString, (char *) L" ...", 4 * sizeof(WCHAR));
    }
    MessageBoxW(NULL, (WCHAR *) Tcl_DStringValue(&msgString),
	    (WCHAR *) Tcl_DStringValue(&titleString),
	    MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL
	    | MB_SETFOREGROUND | MB_TOPMOST);
    Tcl_DStringFree(&msgString);
    Tcl_DStringFree(&titleString);
}
