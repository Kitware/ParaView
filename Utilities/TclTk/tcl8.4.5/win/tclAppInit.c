/*
 * tclAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for Tcl applications (without Tk).  Note that this
 *	program must be built in Win32 console mode to work properly.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include <windows.h>
#include <locale.h>

#ifdef TCL_TEST
extern int		Procbodytest_Init _ANSI_ARGS_((Tcl_Interp *interp));
extern int		Procbodytest_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));
extern int		Tcltest_Init _ANSI_ARGS_((Tcl_Interp *interp));
extern int		TclObjTest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#ifdef TCL_THREADS
extern int		TclThread_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif
#endif /* TCL_TEST */

static void		setargv _ANSI_ARGS_((int *argcPtr, char ***argvPtr));
static BOOL __stdcall	sigHandler (DWORD fdwCtrlType);
static Tcl_AsyncProc	asyncExit;
static void		AppInitExitHandler(ClientData clientData);

static char **          argvSave = NULL;
static Tcl_AsyncHandler exitToken = NULL;
static DWORD            exitErrorCode = 0;


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
    /*
     * The following #if block allows you to change the AppInit
     * function by using a #define of TCL_LOCAL_APPINIT instead
     * of rewriting this entire file.  The #if checks for that
     * #define and uses Tcl_AppInit if it doesn't exist.
     */

#ifndef TCL_LOCAL_APPINIT
#define TCL_LOCAL_APPINIT Tcl_AppInit
#endif
    extern int TCL_LOCAL_APPINIT _ANSI_ARGS_((Tcl_Interp *interp));

    /*
     * The following #if block allows you to change how Tcl finds the startup
     * script, prime the library or encoding paths, fiddle with the argv,
     * etc., without needing to rewrite Tcl_Main()
     */

#ifdef TCL_LOCAL_MAIN_HOOK
    extern int TCL_LOCAL_MAIN_HOOK _ANSI_ARGS_((int *argc, char ***argv));
#endif

    char buffer[MAX_PATH +1];
    char *p;
    /*
     * Set up the default locale to be standard "C" locale so parsing
     * is performed correctly.
     */

    setlocale(LC_ALL, "C");
    setargv(&argc, &argv);

    /*
     * Save this for later, so we can free it.
     */
    argvSave = argv;

    /*
     * Replace argv[0] with full pathname of executable, and forward
     * slashes substituted for backslashes.
     */

    GetModuleFileName(NULL, buffer, sizeof(buffer));
    argv[0] = buffer;
    for (p = buffer; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }

#ifdef TCL_LOCAL_MAIN_HOOK
    TCL_LOCAL_MAIN_HOOK(&argc, &argv);
#endif

    Tcl_Main(argc, argv, TCL_LOCAL_APPINIT);

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

    /*
     * Install a signal handler to the win32 console tclsh is running in.
     */
    SetConsoleCtrlHandler(sigHandler, TRUE);
    exitToken = Tcl_AsyncCreate(asyncExit, NULL);

    /*
     * This exit handler will be used to free the
     * resources allocated in this file.
     */
    Tcl_CreateExitHandler(AppInitExitHandler, NULL);

#ifdef TCL_TEST
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

#if defined(STATIC_BUILD) && defined(TCL_USE_STATIC_PACKAGES)
    {
	extern Tcl_PackageInitProc Registry_Init;
	extern Tcl_PackageInitProc Dde_Init;

	if (Registry_Init(interp) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	Tcl_StaticPackage(interp, "registry", Registry_Init, NULL);

	if (Dde_Init(interp) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	Tcl_StaticPackage(interp, "dde", Dde_Init, NULL);
   }
#endif

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

    Tcl_SetVar(interp, "tcl_rcFileName", "~/tclshrc.tcl", TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AppInitExitHandler --
 *
 *	This function is called to cleanup the app init resources before
 *	Tcl is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the saved argv and deletes the async exit handler.
 *
 *----------------------------------------------------------------------
 */

static void
AppInitExitHandler(
    ClientData clientData)
{
    if (argvSave != NULL) {
        ckfree((char *)argvSave);
        argvSave = NULL;
    }

    if (exitToken != NULL) {
        /*
         * This should be safe to do even if we
         * are in an async exit right now.
         */
        Tcl_AsyncDelete(exitToken);
        exitToken = NULL;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * setargv --
 *
 *	Parse the Windows command line string into argc/argv.  Done here
 *	because we don't trust the builtin argument parser in crt0.
 *	Windows applications are responsible for breaking their command
 *	line into arguments.
 *
 *	2N backslashes + quote -> N backslashes + begin quoted string
 *	2N + 1 backslashes + quote -> literal
 *	N backslashes + non-quote -> literal
 *	quote + quote in a quoted string -> single quote
 *	quote + quote not in quoted string -> empty string
 *	quote -> begin quoted string
 *
 * Results:
 *	Fills argcPtr with the number of arguments and argvPtr with the
 *	array of arguments.
 *
 * Side effects:
 *	Memory allocated.
 *
 *--------------------------------------------------------------------------
 */

static void
setargv(argcPtr, argvPtr)
    int *argcPtr;		/* Filled with number of argument strings. */
    char ***argvPtr;		/* Filled with argument strings (malloc'd). */
{
    char *cmdLine, *p, *arg, *argSpace;
    char **argv;
    int argc, size, inquote, copy, slashes;

    cmdLine = GetCommandLine();	/* INTL: BUG */

    /*
     * Precompute an overly pessimistic guess at the number of arguments
     * in the command line by counting non-space spans.
     */

    size = 2;
    for (p = cmdLine; *p != '\0'; p++) {
	if ((*p == ' ') || (*p == '\t')) {	/* INTL: ISO space. */
	    size++;
	    while ((*p == ' ') || (*p == '\t')) { /* INTL: ISO space. */
		p++;
	    }
	    if (*p == '\0') {
		break;
	    }
	}
    }
    argSpace = (char *) Tcl_Alloc(
	    (unsigned) (size * sizeof(char *) + strlen(cmdLine) + 1));
    argv = (char **) argSpace;
    argSpace += size * sizeof(char *);
    size--;

    p = cmdLine;
    for (argc = 0; argc < size; argc++) {
	argv[argc] = arg = argSpace;
	while ((*p == ' ') || (*p == '\t')) {	/* INTL: ISO space. */
	    p++;
	}
	if (*p == '\0') {
	    break;
	}

	inquote = 0;
	slashes = 0;
	while (1) {
	    copy = 1;
	    while (*p == '\\') {
		slashes++;
		p++;
	    }
	    if (*p == '"') {
		if ((slashes & 1) == 0) {
		    copy = 0;
		    if ((inquote) && (p[1] == '"')) {
			p++;
			copy = 1;
		    } else {
			inquote = !inquote;
		    }
                }
                slashes >>= 1;
            }

            while (slashes) {
		*arg = '\\';
		arg++;
		slashes--;
	    }

	    if ((*p == '\0')
		    || (!inquote && ((*p == ' ') || (*p == '\t')))) { /* INTL: ISO space. */
		break;
	    }
	    if (copy != 0) {
		*arg = *p;
		arg++;
	    }
	    p++;
        }
	*arg = '\0';
	argSpace = arg + 1;
    }
    argv[argc] = NULL;

    *argcPtr = argc;
    *argvPtr = argv;
}

/*
 *----------------------------------------------------------------------
 *
 * asyncExit --
 *
 * 	The AsyncProc for the exitToken.
 *
 * Results:
 * 	doesn't actually return.
 *
 * Side effects:
 * 	tclsh cleanly exits.
 *
 *----------------------------------------------------------------------
 */

int
asyncExit (ClientData clientData, Tcl_Interp *interp, int code)
{
    Tcl_Exit((int)exitErrorCode);

    /* NOTREACHED */
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * sigHandler --
 *
 *	Signal handler for the Win32 OS. Catches Ctrl+C, Ctrl+Break and
 *	other exits. This is needed so tclsh can do it's real clean-up
 *	and not an unclean crash terminate.
 *
 * Results:
 *	TRUE.
 *
 * Side effects:
 *	Effects the way the app exits from a signal. This is an
 *	operating system supplied thread and unsafe to call ANY
 *	Tcl commands except for Tcl_AsyncMark.
 *
 *----------------------------------------------------------------------
 */

BOOL __stdcall
sigHandler(DWORD fdwCtrlType)
{
    HANDLE hStdIn;

    if (!exitToken) {
	/* Async token must have been destroyed, punt gracefully. */
	return FALSE;
    }

    /*
     * If Tcl is currently executing some bytecode or in the eventloop,
     * this will cause Tcl to enter asyncExit at the next command
     * boundry.
     */
    exitErrorCode = fdwCtrlType;
    Tcl_AsyncMark(exitToken);

    /*
     * This will cause Tcl_Gets in Tcl_Main() to drop-out with an <EOF>
     * should it be blocked on input and our Tcl_AsyncMark didn't grab
     * the attention of the interpreter.
     */
    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn) {
	CloseHandle(hStdIn);
    }

    /* indicate to the OS not to call the default terminator */
    return TRUE;
}
