/* 
 * tkMacAppInit.c --
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

#include <Gestalt.h>
#include <ToolUtils.h>
#include <Fonts.h>
#include <Dialogs.h>
#include <SegLoad.h>
#include <Traps.h>
#include <Appearance.h>

#include "tk.h"
#include "tkInt.h"
#include "tkMacInt.h"
#include "tclInt.h"
#include "tclMac.h"
#include "tclMacInt.h"

#ifdef TK_TEST
extern int		Tktest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TK_TEST */

Tcl_Interp *gStdoutInterp = NULL;

int 	TkMacConvertEvent _ANSI_ARGS_((EventRecord *eventPtr));

/*
 * Prototypes for functions the ANSI library needs to link against.
 */
short			InstallConsole _ANSI_ARGS_((short fd));
void			RemoveConsole _ANSI_ARGS_((void));
long			WriteCharsToConsole _ANSI_ARGS_((char *buff, long n));
long			ReadCharsFromConsole _ANSI_ARGS_((char *buff, long n));
extern char *		__ttyname _ANSI_ARGS_((long fildes));
int				kbhit _ANSI_ARGS_((void));
int				getch _ANSI_ARGS_((void));
void			clrscr _ANSI_ARGS_((void));
short			SIOUXHandleOneEvent _ANSI_ARGS_((EventRecord *event));

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		MacintoshInit _ANSI_ARGS_((void));
static int		SetupMainInterp _ANSI_ARGS_((Tcl_Interp *interp));
static void		SetupSIOUX _ANSI_ARGS_((void));

static int inMacExit = 0;
static pascal void NoMoreOutput() { inMacExit = 1; }

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for Wish.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done
 *
 * Side effects:
 *	This procedure initializes the wish world and then 
 *	calls Tk_Main.
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
    newArgv[0] = "Wish";
    newArgv[1] = NULL;
    
    /* Tk_Main is actually #defined to 
     *     Tk_MainEx(argc, argv, Tcl_AppInit, Tcl_CreateInterp())
     * Unfortunately, you also HAVE to call Tcl_FindExecutable
     * BEFORE creating the first interp, or the tcl_library will not
     * get set properly.  So we call it by hand here...
     */
    
    Tcl_FindExecutable(newArgv[0]);
    Tk_Main(argc, newArgv, Tcl_AppInit);
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
    if (Tk_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);
	
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

#ifdef TK_TEST
    if (Tktest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tktest", Tktest_Init,
            (Tcl_PackageInitProc *) NULL);
#endif /* TK_TEST */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     * Each call would look like this:
     *
     * Tcl_CreateCommand(interp, "tclName", CFuncCmd, NULL, NULL);
     */

    SetupMainInterp(interp);

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
 *	This procedure calls Mac specific initilization calls.  Most of
 *	these calls must be made as soon as possible in the startup
 *	process.
 *
 * Results:
 *	Returns TCL_OK if everything went fine.  If it didn't the 
 *	application should probably fail.
 *
 * Side effects:
 *	Inits the application.
 *
 *----------------------------------------------------------------------
 */

static int
MacintoshInit()
{
    int i;
    long result, mask = 0x0700; 		/* mask = system 7.x */

#if GENERATING68K && !GENERATINGCFM
    SetApplLimit(GetApplLimit() - (TK_MAC_68K_STACK_GROWTH));
#endif
    MaxApplZone();
    for (i = 0; i < 4; i++) {
	(void) MoreMasters();
    }

    /*
     * Tk needs us to set the qd pointer it uses.  This is needed
     * so Tk doesn't have to assume the availablity of the qd global
     * variable.  Which in turn allows Tk to be used in code resources.
     */
    tcl_macQdPtr = &qd;

    /*
     * If appearance is present, then register Tk as an Appearance client
     * This means that the mapping from non-Appearance to Appearance cdefs
     * will be done for Tk regardless of the setting in the Appearance
     * control panel.  
     */
     
     if (TkMacHaveAppearance()) {
         RegisterAppearanceClient();
     }

    InitGraf(&tcl_macQdPtr->thePort);
    InitFonts();
    if (TkMacHaveAppearance() >= 0x110) {
        InitFloatingWindows();
    } else {
    InitWindows();
    }
    InitMenus();
    InitDialogs((long) NULL);		
    InitCursor();

    /*
     * Make sure we are running on system 7 or higher
     */
     
    if ((NGetTrapAddress(_Gestalt, ToolTrap) == 
    	    NGetTrapAddress(_Unimplemented, ToolTrap))
    	    || (((Gestalt(gestaltSystemVersion, &result) != noErr)
	    || (result < mask)))) {
	panic("Tcl/Tk requires System 7 or higher.");
    }

    /*
     * Make sure we have color quick draw 
     * (this means we can't run on 68000 macs)
     */
     
    if (((Gestalt(gestaltQuickdrawVersion, &result) != noErr)
	    || (result < gestalt32BitQD13))) {
	panic("Tk requires Color QuickDraw.");
    }

    
    FlushEvents(everyEvent, 0);
    SetEventMask(everyEvent);


    Tcl_MacSetEventProc(TkMacConvertEvent);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SetupMainInterp --
 *
 *	This procedure calls initalization routines require a Tcl 
 *	interp as an argument.  This call effectively makes the passed
 *	iterpreter the "main" interpreter for the application.
 *
 * Results:
 *	Returns TCL_OK if everything went fine.  If it didn't the 
 *	application should probably fail.
 *
 * Side effects:
 *	More initilization.
 *
 *----------------------------------------------------------------------
 */

static int
SetupMainInterp(
    Tcl_Interp *interp)
{
    /*
     * Initialize the console only if we are running as an interactive
     * application.
     */

    TkMacInitAppleEvents(interp);
    TkMacInitMenus(interp);

    if (strcmp(Tcl_GetVar(interp, "tcl_interactive", TCL_GLOBAL_ONLY), "1")
	    == 0) {
	if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
	    goto error;
	}
	SetupSIOUX();
	TclMacInstallExitToShellPatch(NoMoreOutput);
    }

    /*
     * Attach the global interpreter to tk's expected global console
     */

    gStdoutInterp = interp;

    return TCL_OK;

error:
    panic(Tcl_GetStringResult(interp));
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * InstallConsole, RemoveConsole, etc. --
 *
 *	The following functions provide the UI for the console package.
 *	Users wishing to replace SIOUX with their own console package 
 *	need only provide the four functions below in a library.
 *
 * Results:
 *	See SIOUX documentation for details.
 *
 * Side effects:
 *	See SIOUX documentation for details.
 *
 *----------------------------------------------------------------------
 */

short 
InstallConsole(short fd)
{
#pragma unused (fd)

	return 0;
}

void 
RemoveConsole(void)
{
}

long 
WriteCharsToConsole(char *buffer, long n)
{
    if (!inMacExit) {
    	Tcl_DString	ds;
    	Tcl_ExternalToUtfDString(NULL, buffer, n, &ds);
	    TkConsolePrint(gStdoutInterp, TCL_STDOUT, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	    Tcl_DStringFree(&ds);
	    return n;
    } else {
    	return 0;
    }
}

long 
ReadCharsFromConsole(char *buffer, long n)
{
    return 0;
}

extern char *
__ttyname(long fildes)
{
    static char *__devicename = "null device";

    if (fildes >= 0 && fildes <= 2) {
	return (__devicename);
    }
    
    return (0L);
}

int kbhit(void)
{
    return 0; 
}

int getch(void)
{
    return 0; 
}

void clrscr(void)
{
    return;
}

short
SIOUXHandleOneEvent(EventRecord *event)
{
    return 0;
}
static void SetupSIOUX(void) {
#ifndef STATIC_BUILD
	extern DLLIMPORT void SetupConsolePlugins(void*, void*, void*, void*,
									void*, void*, void*, void*);
	SetupConsolePlugins(	&InstallConsole,
							&RemoveConsole,
							&WriteCharsToConsole,
							&ReadCharsFromConsole,
							&__ttyname,
							&kbhit,
							&getch,
							&clrscr);
#endif
}
