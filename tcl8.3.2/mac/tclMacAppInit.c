/* 
 * tclMacAppInit.c --
 *
 *	Provides a version of the Tcl_AppInit procedure for the example shell.
 *
 * Copyright (c) 1993-1994 Lockheed Missle & Space Company, AI Center
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclInt.h"
#include "tclPort.h"
#include "tclMac.h"
#include "tclMacInt.h"

#if defined(THINK_C)
#   include <console.h>
#elif defined(__MWERKS__)
#   include <SIOUX.h>
short InstallConsole _ANSI_ARGS_((short fd));
#endif

#ifdef TCL_TEST
extern int		Procbodytest_Init _ANSI_ARGS_((Tcl_Interp *interp));
extern int		Procbodytest_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));
extern int		TclObjTest_Init _ANSI_ARGS_((Tcl_Interp *interp));
extern int		Tcltest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TCL_TEST */

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		MacintoshInit _ANSI_ARGS_((void));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for tclsh.  This file can be used as a prototype
 *	for other applications using the Tcl library.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done.
 *
 * Side effects:
 *	This procedure initializes the Macintosh world and then 
 *	calls Tcl_Main.  Tcl_Main will never return except to exit.
 *
 *----------------------------------------------------------------------
 */

void
main(
    int argc,				/* Number of arguments. */
    char **argv)			/* Array of argument strings. */
{
    char *newArgv[2];
    
    if (MacintoshInit()  != TCL_OK) {
	Tcl_Exit(1);
    }

    argc = 1;
    newArgv[0] = "tclsh";
    newArgv[1] = NULL;
    Tcl_Main(argc, newArgv, Tcl_AppInit);
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
Tcl_AppInit(
    Tcl_Interp *interp)		/* Interpreter for application. */
{
    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

#ifdef TCL_TEST
    if (Tcltest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init,
            (Tcl_PackageInitProc *) NULL);
    if (TclObjTest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
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
     * Each call would loo like this:
     *
     * Tcl_CreateCommand(interp, "tclName", CFuncCmd, NULL, NULL);
     */

    /*
     * Specify a user-specific startup script to invoke if the application
     * is run interactively.  On the Mac we can specifiy either a TEXT resource
     * which contains the script or the more UNIX like file location
     * may also used.  (I highly recommend using the resource method.)
     */

    Tcl_SetVar(interp, "tcl_rcRsrcName", "tclshrc", TCL_GLOBAL_ONLY);
    /* Tcl_SetVar(interp, "tcl_rcFileName", "~/.tclshrc", TCL_GLOBAL_ONLY); */

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * MacintoshInit --
 *
 *	This procedure calls initalization routines to set up a simple
 *	console on a Macintosh.  This is necessary as the Mac doesn't
 *	have a stdout & stderr by default.
 *
 * Results:
 *	Returns TCL_OK if everything went fine.  If it didn't the 
 *	application should probably fail.
 *
 * Side effects:
 *	Inits the appropiate console package.
 *
 *----------------------------------------------------------------------
 */

static int
MacintoshInit()
{
#if GENERATING68K && !GENERATINGCFM
    SetApplLimit(GetApplLimit() - (TCL_MAC_68K_STACK_GROWTH));
#endif
    MaxApplZone();

#if defined(THINK_C)

    /* Set options for Think C console package */
    /* The console package calls the Mac init calls */
    console_options.pause_atexit = 0;
    console_options.title = "\pTcl Interpreter";
		
#elif defined(__MWERKS__)

    /* Set options for CodeWarrior SIOUX package */
    SIOUXSettings.autocloseonquit = true;
    SIOUXSettings.showstatusline = true;
    SIOUXSettings.asktosaveonclose = false;
    InstallConsole(0);
    SIOUXSetTitle("\pTcl Interpreter");
		
#elif defined(applec)

    /* Init packages used by MPW SIOW package */
    InitGraf((Ptr)&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(nil);
    InitCursor();
		
#endif

    Tcl_MacSetEventProc((Tcl_MacConvertEventPtr) SIOUXHandleOneEvent);
    
    /* No problems with initialization */
    return TCL_OK;
}
