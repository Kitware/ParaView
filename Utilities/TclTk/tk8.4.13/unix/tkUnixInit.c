/* 
 * tkUnixInit.c --
 *
 *  This file contains Unix-specific interpreter initialization
 *  functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkUnixInt.h"

/*
 * The Init script (common to Windows and Unix platforms) is
 * defined in tkInitScript.h
 */
#include "tkInitScript.h"

#ifdef HAVE_COREFOUNDATION
static int    MacOSXGetLibraryPath _ANSI_ARGS_((
          Tcl_Interp *interp));
#endif /* HAVE_COREFOUNDATION */


/*
 *----------------------------------------------------------------------
 *
 * TkpInit --
 *
 *  Performs Unix-specific interpreter initialization related to the
 *      tk_library variable.
 *
 * Results:
 *  Returns a standard Tcl result.  Leaves an error message or result
 *  in the interp's result.
 *
 * Side effects:
 *  Sets "tk_library" Tcl variable, runs "tk.tcl" script.
 *
 *----------------------------------------------------------------------
 */

int
TkpInit(interp)
    Tcl_Interp *interp;
{
    TkCreateXEventSource();
#ifdef HAVE_COREFOUNDATION
    MacOSXGetLibraryPath(interp);
#endif /* HAVE_COREFOUNDATION */
    return Tcl_Eval(interp, initScript);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetAppName --
 *
 *  Retrieves the name of the current application from a platform
 *  specific location.  For Unix, the application name is the tail
 *  of the path contained in the tcl variable argv0.
 *
 * Results:
 *  Returns the application name in the given Tcl_DString.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetAppName(interp, namePtr)
    Tcl_Interp *interp;
    Tcl_DString *namePtr;  /* A previously initialized Tcl_DString. */
{
    CONST char *p, *name;

    name = Tcl_GetVar(interp, "argv0", TCL_GLOBAL_ONLY);
    if ((name == NULL) || (*name == 0)) {
  name = "tk";
    } else {
  p = strrchr(name, '/');
  if (p != NULL) {
      name = p+1;
  }
    }
    Tcl_DStringAppend(namePtr, name, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayWarning --
 *
 *  This routines is called from Tk_Main to display warning
 *  messages that occur during startup.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Generates messages on stdout.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayWarning(msg, title)
    CONST char *msg;    /* Message to be displayed. */
    CONST char *title;    /* Title of warning. */
{
    Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);
    if (errChannel) {
  Tcl_WriteChars(errChannel, title, -1);
  Tcl_WriteChars(errChannel, ": ", 2);
  Tcl_WriteChars(errChannel, msg, -1);
  Tcl_WriteChars(errChannel, "\n", 1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MacOSXGetLibraryPath --
 *
 *  If we have a bundle structure for the Tk installation,
 *  then check there first to see if we can find the libraries
 *  there.
 *
 * Results:
 *  TCL_OK if we have found the tk library; TCL_ERROR otherwise.
 *
 * Side effects:
 *  Same as for Tcl_MacOSXOpenVersionedBundleResources.
 *
 *----------------------------------------------------------------------
 */

#ifdef HAVE_COREFOUNDATION
static int
MacOSXGetLibraryPath(Tcl_Interp *interp)
{
    int foundInFramework = TCL_ERROR;
#ifdef TK_FRAMEWORK
    char tkLibPath[PATH_MAX + 1];
    foundInFramework = Tcl_MacOSXOpenVersionedBundleResources(interp, 
  "com.tcltk.tklibrary", TK_FRAMEWORK_VERSION, 0, PATH_MAX, tkLibPath);
    if (tkLibPath[0] != '\0') {
        Tcl_SetVar(interp, "tk_library", tkLibPath, TCL_GLOBAL_ONLY);
    }
#endif
    return foundInFramework;
}
#endif /* HAVE_COREFOUNDATION */
