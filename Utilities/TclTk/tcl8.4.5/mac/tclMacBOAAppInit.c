/* 
 * tclMacBOAAppInit.c --
 *
 *	Provides a version of the Tcl_AppInit procedure for a 
 *      Macintosh Background Only Application.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
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
#include <Fonts.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Aliases.h>
#include <LowMem.h>

#include <AppleEvents.h>
#include <SegLoad.h>
#include <ToolUtils.h>

#if defined(THINK_C)
#   include <console.h>
#elif defined(__MWERKS__)
#   include <SIOUX.h>
short InstallConsole _ANSI_ARGS_((short fd));
#endif

void TkMacInitAppleEvents(Tcl_Interp *interp);
int HandleHighLevelEvents(EventRecord *eventPtr);	

#ifdef TCL_TEST
EXTERN int		TclObjTest_Init _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int		Tcltest_Init _ANSI_ARGS_((Tcl_Interp *interp));
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
    char *newArgv[3];
    
    if (MacintoshInit()  != TCL_OK) {
	Tcl_Exit(1);
    }

    argc = 2;
    newArgv[0] = "tclsh";
    newArgv[1] = "bgScript.tcl";
    newArgv[2] = NULL;
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
    Tcl_Channel tempChan;
    
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

    /*
     * We have to support at least the quit Apple Event. 
     */
    
    TkMacInitAppleEvents(interp);
    
    /* 
     * Open a file channel to put stderr, stdin, stdout... 
     */
    
    tempChan = Tcl_OpenFileChannel(interp, ":temp.in", "a+", 0);
    Tcl_SetStdChannel(tempChan,TCL_STDIN);
    Tcl_RegisterChannel(interp, tempChan);
    Tcl_SetChannelOption(NULL, tempChan, "-translation", "cr");
    Tcl_SetChannelOption(NULL, tempChan, "-buffering", "line");

    tempChan = Tcl_OpenFileChannel(interp, ":temp.out", "a+", 0);
    Tcl_SetStdChannel(tempChan,TCL_STDOUT);
    Tcl_RegisterChannel(interp, tempChan);
    Tcl_SetChannelOption(NULL, tempChan, "-translation", "cr");
    Tcl_SetChannelOption(NULL, tempChan, "-buffering", "line");

    tempChan = Tcl_OpenFileChannel(interp, ":temp.err", "a+", 0);
    Tcl_SetStdChannel(tempChan,TCL_STDERR);
    Tcl_RegisterChannel(interp, tempChan);
    Tcl_SetChannelOption(NULL, tempChan, "-translation", "cr");
    Tcl_SetChannelOption(NULL, tempChan, "-buffering", "none");
   
    
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
    THz theZone = GetZone();
    SysEnvRec   sys;

    
    /*
     * There is a bug in systems earlier that 7.5.5, where a second BOA will
     * get a corrupted heap.  This is the fix from TechNote 1070
     */
     
    SysEnvirons(1, &sys);

    if (sys.systemVersion < 0x0755)
    {
        if ( LMGetHeapEnd() != theZone->bkLim) {
            LMSetHeapEnd(theZone->bkLim);
        }
    }
    
#if GENERATING68K && !GENERATINGCFM
    SetApplLimit(GetApplLimit() - (TCL_MAC_68K_STACK_GROWTH));
#endif
    MaxApplZone();

    InitGraf((Ptr)&qd.thePort);
    		    
    /* No problems with initialization */
    Tcl_MacSetEventProc(HandleHighLevelEvents);

    return TCL_OK;
}

int
HandleHighLevelEvents(
    EventRecord *eventPtr)
{
    int eventFound = false;
    
    if (eventPtr->what == kHighLevelEvent) {
	AEProcessAppleEvent(eventPtr);
        eventFound = true;
    } else if (eventPtr->what == nullEvent) {
        eventFound = true;
    }
    return eventFound;    
}
