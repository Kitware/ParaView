/* 
 * tclXtTest.c --
 *
 *	Contains commands for Xt notifier specific tests on Unix.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <X11/Intrinsic.h>
#include "tcl.h"

static int	TesteventloopCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern void	InitNotifier _ANSI_ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * Tclxttest_Init --
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
Tclxttest_Init(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    XtToolkitInitialize();
    InitNotifier();
    Tcl_CreateCommand(interp, "testeventloop", TesteventloopCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TesteventloopCmd --
 *
 *	This procedure implements the "testeventloop" command. It is
 *	used to test the Tcl notifier from an "external" event loop
 *	(i.e. not Tcl_DoOneEvent()).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TesteventloopCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    static int *framePtr = NULL; /* Pointer to integer on stack frame of
				  * innermost invocation of the "wait"
				  * subcommand. */

   if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " option ... \"", (char *) NULL);
        return TCL_ERROR;
    }
    if (strcmp(argv[1], "done") == 0) {
	*framePtr = 1;
    } else if (strcmp(argv[1], "wait") == 0) {
	int *oldFramePtr;
	int done;
	int oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);

	/*
	 * Save the old stack frame pointer and set up the current frame.
	 */

	oldFramePtr = framePtr;
	framePtr = &done;

	/*
	 * Enter an Xt event loop until the flag changes.
	 * Note that we do not explicitly call Tcl_ServiceEvent().
	 */

	done = 0;
	while (!done) {
	    XtAppProcessEvent(TclSetAppContext(NULL), XtIMAll);
	}
	(void) Tcl_SetServiceMode(oldMode);
	framePtr = oldFramePtr;
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be done or wait", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}
