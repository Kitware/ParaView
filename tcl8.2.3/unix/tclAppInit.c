/* 
 * tclAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for Tcl applications (without Tk).
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;


#ifdef TCL_TEST

#include "tclInt.h"

extern int		Procbodytest_Init _ANSI_ARGS_((Tcl_Interp *interp));
extern int		Procbodytest_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));
extern int		TclObjTest_Init _ANSI_ARGS_((Tcl_Interp *interp));
extern int		Tcltest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#ifdef TCL_THREADS
extern int		TclThread_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif

#endif /* TCL_TEST */

#ifdef TCL_XT_TEST
extern void		XtToolkitInitialize _ANSI_ARGS_((void));
extern int		Tclxttest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tcl_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;			/* Number of command-line arguments. */
    char **argv;		/* Values of command-line arguments. */
{
#ifdef TCL_TEST
    /*
     * Pass the build time location of the tcl library (to find init.tcl)
     */
    Tcl_Obj *path;
    path = Tcl_NewStringObj(TCL_BUILDTIME_LIBRARY, -1);
    TclSetLibraryPath(Tcl_NewListObj(1,&path));

#endif

#ifdef TCL_XT_TEST
    XtToolkitInitialize();
#endif

    Tcl_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in the interp's result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

#ifdef TCL_TEST
#ifdef TCL_XT_TEST
     if (Tclxttest_Init(interp) == TCL_ERROR) {
	 return TCL_ERROR;
     }
#endif
    if (Tcltest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init,
            (Tcl_PackageInitProc *) NULL);
    if (TclObjTest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
#ifdef TCL_THREADS
    if (TclThread_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
#endif
    if (Procbodytest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "procbodytest", Procbodytest_Init,
            Procbodytest_SafeInit);
#endif /* TCL_TEST */

    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    Tcl_SetVar(interp, "tcl_rcFileName", "~/.tclshrc", TCL_GLOBAL_ONLY);
    return TCL_OK;
}
