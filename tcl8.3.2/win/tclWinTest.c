/* 
 * tclWinTest.c --
 *
 *	Contains commands for platform specific tests on Windows.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"

/*
 * Forward declarations of procedures defined later in this file:
 */
int		TclplatformtestInit _ANSI_ARGS_((Tcl_Interp *interp));
static int	TesteventloopCmd _ANSI_ARGS_((ClientData dummy,
	Tcl_Interp *interp, int argc, char **argv));
static int	TestvolumetypeCmd _ANSI_ARGS_((ClientData dummy,
	Tcl_Interp *interp, int objc,
	Tcl_Obj *CONST objv[]));

/*
 *----------------------------------------------------------------------
 *
 * TclplatformtestInit --
 *
 *	Defines commands that test platform specific functionality for
 *	Unix platforms.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Defines new commands.
 *
 *----------------------------------------------------------------------
 */

int
TclplatformtestInit(interp)
    Tcl_Interp *interp;		/* Interpreter to add commands to. */
{
    /*
     * Add commands for platform specific tests for Windows here.
     */

    Tcl_CreateCommand(interp, "testeventloop", TesteventloopCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testvolumetype", TestvolumetypeCmd,
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
	MSG msg;
	int oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);

	/*
	 * Save the old stack frame pointer and set up the current frame.
	 */

	oldFramePtr = framePtr;
	framePtr = &done;

	/*
	 * Enter a standard Windows event loop until the flag changes.
	 * Note that we do not explicitly call Tcl_ServiceEvent().
	 */

	done = 0;
	while (!done) {
	    if (!GetMessage(&msg, NULL, 0, 0)) {
		/*
		 * The application is exiting, so repost the quit message
		 * and start unwinding.
		 */

		PostQuitMessage(msg.wParam);
		break;
	    }
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
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

/*
 *----------------------------------------------------------------------
 *
 * Testvolumetype --
 *
 *	This procedure implements the "testvolumetype" command. It is
 *	used to check the volume type (FAT, NTFS) of a volume.
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
TestvolumetypeCmd(clientData, interp, objc, objv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
#define VOL_BUF_SIZE 32
    int found;
    char volType[VOL_BUF_SIZE];
    char *path;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?name?");
        return TCL_ERROR;
    }
    if (objc == 2) {
	/*
	 * path has to be really a proper volume, but we don't
	 * get query APIs for that until NT5
	 */
	path = Tcl_GetString(objv[1]);
    } else {
	path = NULL;
    }
    found = GetVolumeInformationA(path, NULL, 0, NULL, NULL, 
	    NULL, volType, VOL_BUF_SIZE);

    if (found == 0) {
	Tcl_AppendResult(interp, "could not get volume type for \"",
		(path?path:""), "\"", (char *) NULL);
	TclWinConvertError(GetLastError());
	return TCL_ERROR;
    }
    Tcl_SetResult(interp, volType, TCL_VOLATILE);
    return TCL_OK;
#undef VOL_BUF_SIZE
}
