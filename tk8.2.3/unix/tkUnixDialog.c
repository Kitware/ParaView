/*
 * tkUnixDialog.c --
 *
 *	Contains the Unix implementation of the common dialog boxes:
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 *
 */
 
#include "tkPort.h"
#include "tkInt.h"
#include "tkUnixInt.h"

/*
 *----------------------------------------------------------------------
 *
 * EvalArgv --
 *
 *	Invokes the Tcl procedure with the arguments. argv[0] is set by
 *	the caller of this function. It may be different than cmdName.
 *	The TCL command will see argv[0], not cmdName, as its name if it
 *	invokes [lindex [info level 0] 0]
 *
 * Results:
 *	TCL_ERROR if the command does not exist and cannot be autoloaded.
 *	Otherwise, return the result of the evaluation of the command.
 *
 * Side effects:
 *	The command may be autoloaded.
 *
 *----------------------------------------------------------------------
 */

static int EvalArgv(interp, cmdName, argc, argv)
    Tcl_Interp *interp;		/* Current interpreter. */
    char * cmdName;		/* Name of the TCL command to call */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tcl_CmdInfo cmdInfo;

    if (!Tcl_GetCommandInfo(interp, cmdName, &cmdInfo)) {
	char * cmdArgv[2];

	/*
	 * This comand is not in the interpreter yet -- looks like we
	 * have to auto-load it
	 */
	if (!Tcl_GetCommandInfo(interp, "auto_load", &cmdInfo)) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "cannot execute command \"auto_load\"",
		NULL);
	    return TCL_ERROR;
	}

	cmdArgv[0] = "auto_load";
	cmdArgv[1] = cmdName;

	if ((*cmdInfo.proc)(cmdInfo.clientData, interp, 2, cmdArgv)!= TCL_OK){ 
	    return TCL_ERROR;
	}

	if (!Tcl_GetCommandInfo(interp, cmdName, &cmdInfo)) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "cannot auto-load command \"",
		cmdName, "\"",NULL);
	    return TCL_ERROR;
	}
    }

    return (*cmdInfo.proc)(cmdInfo.clientData, interp, argc, argv);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseColorCmd --
 *
 *	This procedure implements the color dialog box for the Unix
 *	platform. See the user documentation for details on what it
 *	does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A dialog window is created the first time this procedure is called.
 *	This window is not destroyed and will be reused the next time the
 *	application invokes the "tk_chooseColor" command.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseColorCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return EvalArgv(interp, "tkColorDialog", argc, argv);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetOpenFileCmd --
 *
 *	This procedure implements the "open file" dialog box for the
 *	Unix platform. See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A dialog window is created the first this procedure is called.
 *	This window is not destroyed and will be reused the next time
 *	the application invokes the "tk_getOpenFile" or
 *	"tk_getSaveFile" command.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetOpenFileCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window)clientData;

    if (Tk_StrictMotif(tkwin)) {
	return EvalArgv(interp, "tkMotifFDialog", argc, argv);
    } else {
	return EvalArgv(interp, "tkFDialog", argc, argv);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetSaveFileCmd --
 *
 *	Same as Tk_GetOpenFileCmd but opens a "save file" dialog box
 *	instead
 *
 * Results:
 *	Same as Tk_GetOpenFileCmd.
 *
 * Side effects:
 *	Same as Tk_GetOpenFileCmd.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetSaveFileCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window)clientData;

    if (Tk_StrictMotif(tkwin)) {
	return EvalArgv(interp, "tkMotifFDialog", argc, argv);
    } else {
	return EvalArgv(interp, "tkFDialog", argc, argv);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MessageBoxCmd --
 *
 *	This procedure implements the MessageBox window for the
 *	Unix platform. See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	None. The MessageBox window will be destroy before this procedure
 *	returns.
 *
 *----------------------------------------------------------------------
 */

int
Tk_MessageBoxCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return EvalArgv(interp, "tkMessageBox", argc, argv);
}

