/* 
 * tkWinTest.c --
 *
 *	Contains commands for platform specific tests for
 *	the Windows platform.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"

HWND tkWinCurrentDialog;
 
/*
 * Forward declarations of procedures defined later in this file:
 */

int			TkplatformtestInit(Tcl_Interp *interp);
static int		TestclipboardCmd(ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv);
static int		TestwineventCmd(ClientData clientData, 
			    Tcl_Interp *interp, int argc, char **argv);


/*
 *----------------------------------------------------------------------
 *
 * TkplatformtestInit --
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
TkplatformtestInit(
    Tcl_Interp *interp)		/* Interpreter to add commands to. */
{
    /*
     * Add commands for platform specific tests on MacOS here.
     */
    
    Tcl_CreateCommand(interp, "testclipboard", TestclipboardCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testwinevent", TestwineventCmd,
            (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestclipboardCmd --
 *
 *	This procedure implements the testclipboard command. It provides
 *	a way to determine the actual contents of the Windows clipboard.
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
TestclipboardCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    TkWindow *winPtr = (TkWindow *) clientData;
    HGLOBAL handle;
    char *data;

    if (OpenClipboard(NULL)) {
	handle = GetClipboardData(CF_TEXT);
	if (handle != NULL) {
	    data = GlobalLock(handle);
	    Tcl_AppendResult(interp, data, (char *) NULL);
	    GlobalUnlock(handle);
	}
	CloseClipboard();
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestwineventCmd --
 *
 *	This procedure implements the testwinevent command. It provides
 *	a way to send messages to windows dialogs.
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
TestwineventCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    HWND hwnd;
    int id;
    char *rest;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    static TkStateMap messageMap[] = {
	{WM_LBUTTONDOWN,	"WM_LBUTTONDOWN"},
	{WM_LBUTTONUP,		"WM_LBUTTONUP"},
	{WM_CHAR,		"WM_CHAR"},
	{WM_GETTEXT,		"WM_GETTEXT"},
	{WM_SETTEXT,		"WM_SETTEXT"},
	{-1,			NULL}
    };

    if ((argc == 3) && (strcmp(argv[1], "debug") == 0)) {
	int i;

	if (Tcl_GetBoolean(interp, argv[2], &i) != TCL_OK) {
	    return TCL_ERROR;
	}
	TkWinDialogDebug(i);
	return TCL_OK;
    }

    if (argc < 4) {
	return TCL_ERROR;
    }

    hwnd = (HWND) strtol(argv[1], &rest, 0);
    if (rest == argv[2]) {
	hwnd = FindWindow(NULL, argv[1]);
	if (hwnd == NULL) {
	    Tcl_SetResult(interp, "no such window", TCL_STATIC);
	    return TCL_ERROR;
	}
    } 
    UpdateWindow(hwnd);

    id = strtol(argv[2], &rest, 0);
    if (rest == argv[2]) {
	HWND child;
	char buf[256];

	child = GetWindow(hwnd, GW_CHILD);
	while (child != NULL) {
	    SendMessage(child, WM_GETTEXT, (WPARAM) sizeof(buf), (LPARAM) buf);
	    if (strcasecmp(buf, argv[2]) == 0) {
		id = GetDlgCtrlID(child);
		break;
	    }
	    child = GetWindow(child, GW_HWNDNEXT);
	}
	if (child == NULL) {
	    return TCL_ERROR;
	}
    }
    message = TkFindStateNum(NULL, NULL, messageMap, argv[3]);
    if (message < 0) {
	message = strtol(argv[3], NULL, 0);
    }
    wParam = 0;
    lParam = 0;

    if (argc > 4) {
	wParam = strtol(argv[4], NULL, 0);
    }
    if (argc > 5) {
	lParam = strtol(argv[5], NULL, 0);
    }

    switch (message) {
	case WM_GETTEXT: {
	    Tcl_DString ds;
	    char buf[256];

	    GetDlgItemText(hwnd, id, buf, 256);
	    Tcl_ExternalToUtfDString(NULL, buf, -1, &ds);
	    Tcl_AppendResult(interp, Tcl_DStringValue(&ds), NULL);
	    Tcl_DStringFree(&ds);
	    break;
	}
	case WM_SETTEXT: {
	    Tcl_DString ds;

	    Tcl_UtfToExternalDString(NULL, argv[4], -1, &ds);
	    SetDlgItemText(hwnd, id, Tcl_DStringValue(&ds));
	    Tcl_DStringFree(&ds);
	    break;
	}
	default: {
	    char buf[TCL_INTEGER_SPACE];
	    
	    sprintf(buf, "%d", 
		    SendDlgItemMessage(hwnd, id, message, wParam, lParam));
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    break;
	}
    }
    return TCL_OK;
}
    


