/* 
 * tkCmds.c --
 *
 *	This file contains a collection of Tk-related Tcl commands
 *	that didn't fit in any particular file of the toolkit.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkPort.h"
#include "tkInt.h"
#include <errno.h>

#if defined(__WIN32__)
#include "tkWinInt.h"
#elif defined(MAC_TCL)
#include "tkMacInt.h"
#else
#include "tkUnixInt.h"
#endif


/*
 * Forward declarations for procedures defined later in this file:
 */

static TkWindow *	GetToplevel _ANSI_ARGS_((Tk_Window tkwin));
static char *		WaitVariableProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
static void		WaitVisibilityProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		WaitWindowProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));

/*
 *----------------------------------------------------------------------
 *
 * Tk_BellObjCmd --
 *
 *	This procedure is invoked to process the "bell" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_BellObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    static char *bellOptions[] = {"-displayof", (char *) NULL};
    Tk_Window tkwin = (Tk_Window) clientData;
    char *displayName;
    int index;

    if ((objc != 1) && (objc != 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-displayof window?");
	return TCL_ERROR;
    }

    if (objc == 3) {
	if (Tcl_GetIndexFromObj(interp, objv[1], bellOptions, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	displayName = Tcl_GetStringFromObj(objv[2], (int *) NULL);
	
	tkwin = Tk_NameToWindow(interp, displayName, tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    }
    XBell(Tk_Display(tkwin), 0);
    XForceScreenSaver(Tk_Display(tkwin), ScreenSaverReset);
    XFlush(Tk_Display(tkwin));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BindCmd --
 *
 *	This procedure is invoked to process the "bind" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_BindCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr;
    ClientData object;

    if ((argc < 2) || (argc > 4)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" window ?pattern? ?command?\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (argv[1][0] == '.') {
	winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
	if (winPtr == NULL) {
	    return TCL_ERROR;
	}
	object = (ClientData) winPtr->pathName;
    } else {
	winPtr = (TkWindow *) clientData;
	object = (ClientData) Tk_GetUid(argv[1]);
    }

    if (argc == 4) {
	int append = 0;
	unsigned long mask;

	if (argv[3][0] == 0) {
	    return Tk_DeleteBinding(interp, winPtr->mainPtr->bindingTable,
		    object, argv[2]);
	}
	if (argv[3][0] == '+') {
	    argv[3]++;
	    append = 1;
	}
	mask = Tk_CreateBinding(interp, winPtr->mainPtr->bindingTable,
		object, argv[2], argv[3], append);
	if (mask == 0) {
	    return TCL_ERROR;
	}
    } else if (argc == 3) {
	char *command;

	command = Tk_GetBinding(interp, winPtr->mainPtr->bindingTable,
		object, argv[2]);
	if (command == NULL) {
	    Tcl_ResetResult(interp);
	    return TCL_OK;
	}
	Tcl_SetResult(interp, command, TCL_STATIC);
    } else {
	Tk_GetAllBindings(interp, winPtr->mainPtr->bindingTable, object);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkBindEventProc --
 *
 *	This procedure is invoked by Tk_HandleEvent for each event;  it
 *	causes any appropriate bindings for that event to be invoked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what bindings have been established with the "bind"
 *	command.
 *
 *----------------------------------------------------------------------
 */

void
TkBindEventProc(winPtr, eventPtr)
    TkWindow *winPtr;			/* Pointer to info about window. */
    XEvent *eventPtr;			/* Information about event. */
{
#define MAX_OBJS 20
    ClientData objects[MAX_OBJS], *objPtr;
    TkWindow *topLevPtr;
    int i, count;
    char *p;
    Tcl_HashEntry *hPtr;

    if ((winPtr->mainPtr == NULL) || (winPtr->mainPtr->bindingTable == NULL)) {
	return;
    }

    objPtr = objects;
    if (winPtr->numTags != 0) {
	/*
	 * Make a copy of the tags for the window, replacing window names
	 * with pointers to the pathName from the appropriate window.
	 */

	if (winPtr->numTags > MAX_OBJS) {
	    objPtr = (ClientData *) ckalloc((unsigned)
		    (winPtr->numTags * sizeof(ClientData)));
	}
	for (i = 0; i < winPtr->numTags; i++) {
	    p = (char *) winPtr->tagPtr[i];
	    if (*p == '.') {
		hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->nameTable, p);
		if (hPtr != NULL) {
		    p = ((TkWindow *) Tcl_GetHashValue(hPtr))->pathName;
		} else {
		    p = NULL;
		}
	    }
	    objPtr[i] = (ClientData) p;
	}
	count = winPtr->numTags;
    } else {
	objPtr[0] = (ClientData) winPtr->pathName;
	objPtr[1] = (ClientData) winPtr->classUid;
	for (topLevPtr = winPtr;
		(topLevPtr != NULL) && !(topLevPtr->flags & TK_TOP_LEVEL);
		topLevPtr = topLevPtr->parentPtr) {
	    /* Empty loop body. */
	}
	if ((winPtr != topLevPtr) && (topLevPtr != NULL)) {
	    count = 4;
	    objPtr[2] = (ClientData) topLevPtr->pathName;
	} else {
	    count = 3;
	}
	objPtr[count-1] = (ClientData) Tk_GetUid("all");
    }
    Tk_BindEvent(winPtr->mainPtr->bindingTable, eventPtr, (Tk_Window) winPtr,
	    count, objPtr);
    if (objPtr != objects) {
	ckfree((char *) objPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BindtagsCmd --
 *
 *	This procedure is invoked to process the "bindtags" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_BindtagsCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr, *winPtr2;
    int i, tagArgc;
    char *p, **tagArgv;

    if ((argc < 2) || (argc > 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" window ?tags?\"", (char *) NULL);
	return TCL_ERROR;
    }
    winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	if (winPtr->numTags == 0) {
	    Tcl_AppendElement(interp, winPtr->pathName);
	    Tcl_AppendElement(interp, winPtr->classUid);
	    for (winPtr2 = winPtr;
		    (winPtr2 != NULL) && !(winPtr2->flags & TK_TOP_LEVEL);
		    winPtr2 = winPtr2->parentPtr) {
		/* Empty loop body. */
	    }
	    if ((winPtr != winPtr2) && (winPtr2 != NULL)) {
		Tcl_AppendElement(interp, winPtr2->pathName);
	    }
	    Tcl_AppendElement(interp, "all");
	} else {
	    for (i = 0; i < winPtr->numTags; i++) {
		Tcl_AppendElement(interp, (char *) winPtr->tagPtr[i]);
	    }
	}
	return TCL_OK;
    }
    if (winPtr->tagPtr != NULL) {
	TkFreeBindingTags(winPtr);
    }
    if (argv[2][0] == 0) {
	return TCL_OK;
    }
    if (Tcl_SplitList(interp, argv[2], &tagArgc, &tagArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    winPtr->numTags = tagArgc;
    winPtr->tagPtr = (ClientData *) ckalloc((unsigned)
	    (tagArgc * sizeof(ClientData)));
    for (i = 0; i < tagArgc; i++) {
	p = tagArgv[i];
	if (p[0] == '.') {
	    char *copy;

	    /*
	     * Handle names starting with "." specially: store a malloc'ed
	     * string, rather than a Uid;  at event time we'll look up the
	     * name in the window table and use the corresponding window,
	     * if there is one.
	     */

	    copy = (char *) ckalloc((unsigned) (strlen(p) + 1));
	    strcpy(copy, p);
	    winPtr->tagPtr[i] = (ClientData) copy;
	} else {
	    winPtr->tagPtr[i] = (ClientData) Tk_GetUid(p);
	}
    }
    ckfree((char *) tagArgv);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeBindingTags --
 *
 *	This procedure is called to free all of the binding tags
 *	associated with a window;  typically it is only invoked where
 *	there are window-specific tags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any binding tags for winPtr are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkFreeBindingTags(winPtr)
    TkWindow *winPtr;		/* Window whose tags are to be released. */
{
    int i;
    char *p;

    for (i = 0; i < winPtr->numTags; i++) {
	p = (char *) (winPtr->tagPtr[i]);
	if (*p == '.') {
	    /*
	     * Names starting with "." are malloced rather than Uids, so
	     * they have to be freed.
	     */
    
	    ckfree(p);
	}
    }
    ckfree((char *) winPtr->tagPtr);
    winPtr->numTags = 0;
    winPtr->tagPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DestroyCmd --
 *
 *	This procedure is invoked to process the "destroy" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_DestroyCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window window;
    Tk_Window tkwin = (Tk_Window) clientData;
    int i;

    for (i = 1; i < argc; i++) {
	window = Tk_NameToWindow(interp, argv[i], tkwin);
	if (window == NULL) {
	    Tcl_ResetResult(interp);
	    continue;
	}
	Tk_DestroyWindow(window);
	if (window == tkwin) {
	    /*
	     * We just deleted the main window for the application! This
	     * makes it impossible to do anything more (tkwin isn't
	     * valid anymore).
	     */

	    break;
	 }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_LowerCmd --
 *
 *	This procedure is invoked to process the "lower" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_LowerCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window mainwin = (Tk_Window) clientData;
    Tk_Window tkwin, other;

    if ((argc != 2) && (argc != 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window ?belowThis?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, argv[1], mainwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	other = NULL;
    } else {
	other = Tk_NameToWindow(interp, argv[2], mainwin);
	if (other == NULL) {
	    return TCL_ERROR;
	}
    }
    if (Tk_RestackWindow(tkwin, Below, other) != TCL_OK) {
	Tcl_AppendResult(interp, "can't lower \"", argv[1], "\" below \"",
		argv[2], "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RaiseCmd --
 *
 *	This procedure is invoked to process the "raise" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_RaiseCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window mainwin = (Tk_Window) clientData;
    Tk_Window tkwin, other;

    if ((argc != 2) && (argc != 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window ?aboveThis?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, argv[1], mainwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	other = NULL;
    } else {
	other = Tk_NameToWindow(interp, argv[2], mainwin);
	if (other == NULL) {
	    return TCL_ERROR;
	}
    }
    if (Tk_RestackWindow(tkwin, Above, other) != TCL_OK) {
	Tcl_AppendResult(interp, "can't raise \"", argv[1], "\" above \"",
		argv[2], "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TkObjCmd --
 *
 *	This procedure is invoked to process the "tk" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_TkObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int index;
    Tk_Window tkwin;
    static char *optionStrings[] = {
	"appname",	"scaling",	NULL
    };
    enum options {
	TK_APPNAME,	TK_SCALING
    };

    tkwin = (Tk_Window) clientData;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
        case TK_APPNAME: {
	    TkWindow *winPtr;
	    char *string;

	    winPtr = (TkWindow *) tkwin;

	    if (objc > 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "?newName?");
		return TCL_ERROR;
	    }
	    if (objc == 3) {
		string = Tcl_GetStringFromObj(objv[2], NULL);
		winPtr->nameUid = Tk_GetUid(Tk_SetAppName(tkwin, string));
	    }
	    Tcl_AppendResult(interp, winPtr->nameUid, NULL);
	    break;
	}
	case TK_SCALING: {
	    Screen *screenPtr;
	    int skip, width, height;
	    double d;
	    
	    screenPtr = Tk_Screen(tkwin);

	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip == 2) {
		d = 25.4 / 72;
		d *= WidthOfScreen(screenPtr);
		d /= WidthMMOfScreen(screenPtr);
		Tcl_SetDoubleObj(Tcl_GetObjResult(interp), d);
	    } else if (objc - skip == 3) {
		if (Tcl_GetDoubleFromObj(interp, objv[2 + skip], &d) != TCL_OK) {
		    return TCL_ERROR;
		}
		d = (25.4 / 72) / d;
		width = (int) (d * WidthOfScreen(screenPtr) + 0.5);
		if (width <= 0) {
		    width = 1;
		}
		height = (int) (d * HeightOfScreen(screenPtr) + 0.5); 
		if (height <= 0) {
		    height = 1;
		}
		WidthMMOfScreen(screenPtr) = width;
		HeightMMOfScreen(screenPtr) = height;
	    } else {
		Tcl_WrongNumArgs(interp, 2, objv,
			"?-displayof window? ?factor?");
		return TCL_ERROR;
	    }
	    break;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TkwaitCmd --
 *
 *	This procedure is invoked to process the "tkwait" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_TkwaitCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    int c, done;
    size_t length;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " variable|visibility|window name\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'v') && (strncmp(argv[1], "variable", length) == 0)
	    && (length >= 2)) {
	if (Tcl_TraceVar(interp, argv[2],
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		WaitVariableProc, (ClientData) &done) != TCL_OK) {
	    return TCL_ERROR;
	}
	done = 0;
	while (!done) {
	    Tcl_DoOneEvent(0);
	}
	Tcl_UntraceVar(interp, argv[2],
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		WaitVariableProc, (ClientData) &done);
    } else if ((c == 'v') && (strncmp(argv[1], "visibility", length) == 0)
	    && (length >= 2)) {
	Tk_Window window;

	window = Tk_NameToWindow(interp, argv[2], tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_CreateEventHandler(window, VisibilityChangeMask|StructureNotifyMask,
	    WaitVisibilityProc, (ClientData) &done);
	done = 0;
	while (!done) {
	    Tcl_DoOneEvent(0);
	}
	if (done != 1) {
	    /*
	     * Note that we do not delete the event handler because it
	     * was deleted automatically when the window was destroyed.
	     */

	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "window \"", argv[2],
		    "\" was deleted before its visibility changed",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	Tk_DeleteEventHandler(window, VisibilityChangeMask|StructureNotifyMask,
	    WaitVisibilityProc, (ClientData) &done);
    } else if ((c == 'w') && (strncmp(argv[1], "window", length) == 0)) {
	Tk_Window window;

	window = Tk_NameToWindow(interp, argv[2], tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_CreateEventHandler(window, StructureNotifyMask,
	    WaitWindowProc, (ClientData) &done);
	done = 0;
	while (!done) {
	    Tcl_DoOneEvent(0);
	}
	/*
	 * Note:  there's no need to delete the event handler.  It was
	 * deleted automatically when the window was destroyed.
	 */
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be variable, visibility, or window", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Clear out the interpreter's result, since it may have been set
     * by event handlers.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}

	/* ARGSUSED */
static char *
WaitVariableProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Pointer to integer to set to 1. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    int *donePtr = (int *) clientData;

    *donePtr = 1;
    return (char *) NULL;
}

	/*ARGSUSED*/
static void
WaitVisibilityProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to integer to set to 1. */
    XEvent *eventPtr;		/* Information about event (not used). */
{
    int *donePtr = (int *) clientData;

    if (eventPtr->type == VisibilityNotify) {
	*donePtr = 1;
    }
    if (eventPtr->type == DestroyNotify) {
	*donePtr = 2;
    }
}

static void
WaitWindowProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to integer to set to 1. */
    XEvent *eventPtr;		/* Information about event. */
{
    int *donePtr = (int *) clientData;

    if (eventPtr->type == DestroyNotify) {
	*donePtr = 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UpdateObjCmd --
 *
 *	This procedure is invoked to process the "update" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_UpdateObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    static char *updateOptions[] = {"idletasks", (char *) NULL};
    int flags, index;
    TkDisplay *dispPtr;

    if (objc == 1) {
	flags = TCL_DONT_WAIT;
    } else if (objc == 2) {
	if (Tcl_GetIndexFromObj(interp, objv[1], updateOptions, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	flags = TCL_IDLE_EVENTS;
    } else {
        Tcl_WrongNumArgs(interp, 1, objv, "?idletasks?");
	return TCL_ERROR;
    }

    /*
     * Handle all pending events, sync all displays, and repeat over
     * and over again until all pending events have been handled.
     * Special note:  it's possible that the entire application could
     * be destroyed by an event handler that occurs during the update.
     * Thus, don't use any information from tkwin after calling
     * Tcl_DoOneEvent.
     */
  
    while (1) {
	while (Tcl_DoOneEvent(flags) != 0) {
	    /* Empty loop body */
	}
	for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
		dispPtr = dispPtr->nextPtr) {
	    XSync(dispPtr->display, False);
	}
	if (Tcl_DoOneEvent(flags) == 0) {
	    break;
	}
    }

    /*
     * Must clear the interpreter's result because event handlers could
     * have executed commands.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_WinfoObjCmd --
 *
 *	This procedure is invoked to process the "winfo" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_WinfoObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int index, x, y, width, height, useX, useY, class, skip;
    char *string;
    TkWindow *winPtr;
    Tk_Window tkwin;
    Tcl_Obj *resultPtr;

    static TkStateMap visualMap[] = {
	{PseudoColor,	"pseudocolor"},
	{GrayScale,	"grayscale"},
	{DirectColor,	"directcolor"},
	{TrueColor,	"truecolor"},
	{StaticColor,	"staticcolor"},
	{StaticGray,	"staticgray"},
	{-1,		NULL}
    };
    static char *optionStrings[] = {
	"cells",	"children",	"class",	"colormapfull",
	"depth",	"geometry",	"height",	"id",
	"ismapped",	"manager",	"name",		"parent",
	"pointerx",	"pointery",	"pointerxy",	"reqheight",
	"reqwidth",	"rootx",	"rooty",	"screen",
	"screencells",	"screendepth",	"screenheight",	"screenwidth",
	"screenmmheight","screenmmwidth","screenvisual","server",
	"toplevel",	"viewable",	"visual",	"visualid",
	"vrootheight",	"vrootwidth",	"vrootx",	"vrooty",
	"width",	"x",		"y",
	
	"atom",		"atomname",	"containing",	"interps",
	"pathname",

	"exists",	"fpixels",	"pixels",	"rgb",
	"visualsavailable",

	NULL
    };
    enum options {
	WIN_CELLS,	WIN_CHILDREN,	WIN_CLASS,	WIN_COLORMAPFULL,
	WIN_DEPTH,	WIN_GEOMETRY,	WIN_HEIGHT,	WIN_ID,
	WIN_ISMAPPED,	WIN_MANAGER,	WIN_NAME,	WIN_PARENT,
	WIN_POINTERX,	WIN_POINTERY,	WIN_POINTERXY,	WIN_REQHEIGHT,
	WIN_REQWIDTH,	WIN_ROOTX,	WIN_ROOTY,	WIN_SCREEN,
	WIN_SCREENCELLS,WIN_SCREENDEPTH,WIN_SCREENHEIGHT,WIN_SCREENWIDTH,
	WIN_SCREENMMHEIGHT,WIN_SCREENMMWIDTH,WIN_SCREENVISUAL,WIN_SERVER,
	WIN_TOPLEVEL,	WIN_VIEWABLE,	WIN_VISUAL,	WIN_VISUALID,
	WIN_VROOTHEIGHT,WIN_VROOTWIDTH,	WIN_VROOTX,	WIN_VROOTY,
	WIN_WIDTH,	WIN_X,		WIN_Y,
	
	WIN_ATOM,	WIN_ATOMNAME,	WIN_CONTAINING,	WIN_INTERPS,
	WIN_PATHNAME,

	WIN_EXISTS,	WIN_FPIXELS,	WIN_PIXELS,	WIN_RGB,
	WIN_VISUALSAVAILABLE
    };

    tkwin = (Tk_Window) clientData;
    
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (index < WIN_ATOM) {
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	string = Tcl_GetStringFromObj(objv[2], NULL);
	tkwin = Tk_NameToWindow(interp, string, tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    }
    winPtr = (TkWindow *) tkwin;
    resultPtr = Tcl_GetObjResult(interp);

    switch ((enum options) index) {
	case WIN_CELLS: {
	    Tcl_SetIntObj(resultPtr, Tk_Visual(tkwin)->map_entries);
	    break;
	}
	case WIN_CHILDREN: {
	    Tcl_Obj *strPtr;

	    winPtr = winPtr->childList;
	    for ( ; winPtr != NULL; winPtr = winPtr->nextPtr) {
		strPtr = Tcl_NewStringObj(winPtr->pathName, -1);
		Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	    }
	    break;
	}
	case WIN_CLASS: {
	    Tcl_SetStringObj(resultPtr, Tk_Class(tkwin), -1);
	    break;
	}
	case WIN_COLORMAPFULL: {
	    Tcl_SetBooleanObj(resultPtr,
		    TkpCmapStressed(tkwin, Tk_Colormap(tkwin)));
	    break;
	}
	case WIN_DEPTH: {
	    Tcl_SetIntObj(resultPtr, Tk_Depth(tkwin));
	    break;
	}
	case WIN_GEOMETRY: {
	    char buf[16 + TCL_INTEGER_SPACE * 4];

	    sprintf(buf, "%dx%d+%d+%d", Tk_Width(tkwin), Tk_Height(tkwin),
		    Tk_X(tkwin), Tk_Y(tkwin));
	    Tcl_SetStringObj(resultPtr, buf, -1);
	    break;
	}
	case WIN_HEIGHT: {
	    Tcl_SetIntObj(resultPtr, Tk_Height(tkwin));
	    break;
	}
	case WIN_ID: {
	    char buf[TCL_INTEGER_SPACE];
	    
	    Tk_MakeWindowExist(tkwin);
	    TkpPrintWindowId(buf, Tk_WindowId(tkwin));
	    Tcl_SetStringObj(resultPtr, buf, -1);
	    break;
	}
	case WIN_ISMAPPED: {
	    Tcl_SetBooleanObj(resultPtr, (int) Tk_IsMapped(tkwin));
	    break;
	}
	case WIN_MANAGER: {
	    if (winPtr->geomMgrPtr != NULL) {
		Tcl_SetStringObj(resultPtr, winPtr->geomMgrPtr->name, -1);
	    }
	    break;
	}
	case WIN_NAME: {
	    Tcl_SetStringObj(resultPtr, Tk_Name(tkwin), -1);
	    break;
	}
	case WIN_PARENT: {
	    if (winPtr->parentPtr != NULL) {
		Tcl_SetStringObj(resultPtr, winPtr->parentPtr->pathName, -1);
	    }
	    break;
	}
	case WIN_POINTERX: {
	    useX = 1;
	    useY = 0;
	    goto pointerxy;
	}
	case WIN_POINTERY: {
	    useX = 0;
	    useY = 1;
	    goto pointerxy;
	}
	case WIN_POINTERXY: {
	    useX = 1;
	    useY = 1;

	    pointerxy:
	    winPtr = GetToplevel(tkwin);
	    if (winPtr == NULL) {
		x = -1;
		y = -1;
	    } else {
		TkGetPointerCoords((Tk_Window) winPtr, &x, &y);
	    }
	    if (useX & useY) {
		char buf[TCL_INTEGER_SPACE * 2];
		
		sprintf(buf, "%d %d", x, y);
		Tcl_SetStringObj(resultPtr, buf, -1);
	    } else if (useX) {
		Tcl_SetIntObj(resultPtr, x);
	    } else {
		Tcl_SetIntObj(resultPtr, y);
	    }
	    break;
	}
	case WIN_REQHEIGHT: {
	    Tcl_SetIntObj(resultPtr, Tk_ReqHeight(tkwin));
	    break;
	}
	case WIN_REQWIDTH: {
	    Tcl_SetIntObj(resultPtr, Tk_ReqWidth(tkwin));
	    break;
	}
	case WIN_ROOTX: {
	    Tk_GetRootCoords(tkwin, &x, &y);
	    Tcl_SetIntObj(resultPtr, x);
	    break;
	}
	case WIN_ROOTY: {
	    Tk_GetRootCoords(tkwin, &x, &y);
	    Tcl_SetIntObj(resultPtr, y);
	    break;
	}
	case WIN_SCREEN: {
	    char buf[TCL_INTEGER_SPACE];
	    
	    sprintf(buf, "%d", Tk_ScreenNumber(tkwin));
	    Tcl_AppendStringsToObj(resultPtr, Tk_DisplayName(tkwin), ".",
		    buf, NULL);
	    break;
	}
	case WIN_SCREENCELLS: {
	    Tcl_SetIntObj(resultPtr, CellsOfScreen(Tk_Screen(tkwin)));
	    break;
	}
	case WIN_SCREENDEPTH: {
	    Tcl_SetIntObj(resultPtr, DefaultDepthOfScreen(Tk_Screen(tkwin)));
	    break;
	}
	case WIN_SCREENHEIGHT: {
	    Tcl_SetIntObj(resultPtr, HeightOfScreen(Tk_Screen(tkwin)));
	    break;
	}
	case WIN_SCREENWIDTH: {
	    Tcl_SetIntObj(resultPtr, WidthOfScreen(Tk_Screen(tkwin)));
	    break;
	}
	case WIN_SCREENMMHEIGHT: {
	    Tcl_SetIntObj(resultPtr, HeightMMOfScreen(Tk_Screen(tkwin)));
	    break;
	}
	case WIN_SCREENMMWIDTH: {
	    Tcl_SetIntObj(resultPtr, WidthMMOfScreen(Tk_Screen(tkwin)));
	    break;
	}
	case WIN_SCREENVISUAL: {
	    class = DefaultVisualOfScreen(Tk_Screen(tkwin))->class;
	    goto visual;
	}
	case WIN_SERVER: {
	    TkGetServerInfo(interp, tkwin);
	    break;
	}
	case WIN_TOPLEVEL: {
	    winPtr = GetToplevel(tkwin);
	    if (winPtr != NULL) {
		Tcl_SetStringObj(resultPtr, winPtr->pathName, -1);
	    }
	    break;
	}
	case WIN_VIEWABLE: {
	    int viewable;

	    viewable = 0;
	    for ( ; ; winPtr = winPtr->parentPtr) {
		if ((winPtr == NULL) || !(winPtr->flags & TK_MAPPED)) {
		    break;
		}
		if (winPtr->flags & TK_TOP_LEVEL) {
		    viewable = 1;
		    break;
		}
	    }
	    Tcl_SetBooleanObj(resultPtr, viewable);
	    break;
	}
	case WIN_VISUAL: {
	    class = Tk_Visual(tkwin)->class;

	    visual:
	    string = TkFindStateString(visualMap, class);
	    if (string == NULL) {
		string = "unknown";
	    }
	    Tcl_SetStringObj(resultPtr, string, -1);
	    break;
	}
	case WIN_VISUALID: {
	    char buf[TCL_INTEGER_SPACE];

	    sprintf(buf, "0x%x",
		    (unsigned int) XVisualIDFromVisual(Tk_Visual(tkwin)));
	    Tcl_SetStringObj(resultPtr, buf, -1);
	    break;
	}
	case WIN_VROOTHEIGHT: {
	    Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	    Tcl_SetIntObj(resultPtr, height);
	    break;
	}
	case WIN_VROOTWIDTH: {
	    Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	    Tcl_SetIntObj(resultPtr, width);
	    break;
	}
	case WIN_VROOTX: {
	    Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	    Tcl_SetIntObj(resultPtr, x);
	    break;
	}
	case WIN_VROOTY: {
	    Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	    Tcl_SetIntObj(resultPtr, y);
	    break;
	}
	case WIN_WIDTH: {
	    Tcl_SetIntObj(resultPtr, Tk_Width(tkwin));
	    break;
	}
	case WIN_X: {
	    Tcl_SetIntObj(resultPtr, Tk_X(tkwin));
	    break;
	}
	case WIN_Y: {
	    Tcl_SetIntObj(resultPtr, Tk_Y(tkwin));
	    break;
	}

	/*
	 * Uses -displayof.
	 */
	 
	case WIN_ATOM: {
	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window? name");
		return TCL_ERROR;
	    }
	    objv += skip;
	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    Tcl_SetLongObj(resultPtr, (long) Tk_InternAtom(tkwin, string));
	    break;
	}
	case WIN_ATOMNAME: {
	    char *name;
	    long id;
	    
	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window? id");
		return TCL_ERROR;
	    }
	    objv += skip;
	    if (Tcl_GetLongFromObj(interp, objv[2], &id) != TCL_OK) {
		return TCL_ERROR;
	    }
	    name = Tk_GetAtomName(tkwin, (Atom) id);
	    if (strcmp(name, "?bad atom?") == 0) {
		string = Tcl_GetStringFromObj(objv[2], NULL);
		Tcl_AppendStringsToObj(resultPtr, 
			"no atom exists with id \"", string, "\"", NULL);
		return TCL_ERROR;
	    }
	    Tcl_SetStringObj(resultPtr, name, -1);
	    break;
	}
	case WIN_CONTAINING: {
	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 4) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"?-displayof window? rootX rootY");
		return TCL_ERROR;
	    }
	    objv += skip;
	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    if (Tk_GetPixels(interp, tkwin, string, &x) != TCL_OK) {
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[3], NULL);
	    if (Tk_GetPixels(interp, tkwin, string, &y) != TCL_OK) {
		return TCL_ERROR;
	    }
	    tkwin = Tk_CoordsToWindow(x, y, tkwin);
	    if (tkwin != NULL) {
		Tcl_SetStringObj(resultPtr, Tk_PathName(tkwin), -1);
	    }
	    break;
	}
	case WIN_INTERPS: {
	    int result;
	    
	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window?");
		return TCL_ERROR;
	    }
	    result = TkGetInterpNames(interp, tkwin);
	    return result;
	}
	case WIN_PATHNAME: {
	    int id;

	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window? id");
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[2 + skip], NULL);
	    if (TkpScanWindowId(interp, string, &id) != TCL_OK) {
		return TCL_ERROR;
	    }
	    winPtr = (TkWindow *)
	            Tk_IdToWindow(Tk_Display(tkwin), (Window) id);
	    if ((winPtr == NULL) ||
		    (winPtr->mainPtr != ((TkWindow *) tkwin)->mainPtr)) {
		Tcl_AppendStringsToObj(resultPtr, "window id \"", string,
			"\" doesn't exist in this application", (char *) NULL);
		return TCL_ERROR;
	    }

	    /*
	     * If the window is a utility window with no associated path
	     * (such as a wrapper window or send communication window), just
	     * return an empty string.
	     */

	    tkwin = (Tk_Window) winPtr;
	    if (Tk_PathName(tkwin) != NULL) {
		Tcl_SetStringObj(resultPtr, Tk_PathName(tkwin), -1);
	    }
	    break;
	}

	/*
	 * objv[3] is window.
	 */

	case WIN_EXISTS: {
	    int alive;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "window");
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    winPtr = (TkWindow *) Tk_NameToWindow(interp, string, tkwin);
	    Tcl_ResetResult(interp);
	    resultPtr = Tcl_GetObjResult(interp);

	    alive = 1;
	    if ((winPtr == NULL) || (winPtr->flags & TK_ALREADY_DEAD)) {
		alive = 0;
	    }
	    Tcl_SetBooleanObj(resultPtr, alive);
	    break;
	}
	case WIN_FPIXELS: {
	    double mm, pixels;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "window number");
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    tkwin = Tk_NameToWindow(interp, string, tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[3], NULL);
	    if (Tk_GetScreenMM(interp, tkwin, string, &mm) != TCL_OK) {
		return TCL_ERROR;
	    }
	    pixels = mm * WidthOfScreen(Tk_Screen(tkwin))
		    / WidthMMOfScreen(Tk_Screen(tkwin));
	    Tcl_SetDoubleObj(resultPtr, pixels);
	    break;
	}
	case WIN_PIXELS: {
	    int pixels;
	    
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "window number");
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    tkwin = Tk_NameToWindow(interp, string, tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[3], NULL);
	    if (Tk_GetPixels(interp, tkwin, string, &pixels) != TCL_OK) {
		return TCL_ERROR;
	    }
	    Tcl_SetIntObj(resultPtr, pixels);
	    break;
	}
	case WIN_RGB: {
	    XColor *colorPtr;
	    char buf[TCL_INTEGER_SPACE * 3];

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "window colorName");
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    tkwin = Tk_NameToWindow(interp, string, tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[3], NULL);
	    colorPtr = Tk_GetColor(interp, tkwin, string);
	    if (colorPtr == NULL) {
		return TCL_ERROR;
	    }
	    sprintf(buf, "%d %d %d", colorPtr->red, colorPtr->green,
		    colorPtr->blue);
	    Tk_FreeColor(colorPtr);
	    Tcl_SetStringObj(resultPtr, buf, -1);
	    break;
	}
	case WIN_VISUALSAVAILABLE: {
	    XVisualInfo template, *visInfoPtr;
	    int count, i;
	    int includeVisualId;
	    Tcl_Obj *strPtr;
	    char buf[16 + TCL_INTEGER_SPACE];
	    char visualIdString[TCL_INTEGER_SPACE];

	    if (objc == 3) {
		includeVisualId = 0;
	    } else if ((objc == 4)
		    && (strcmp(Tcl_GetStringFromObj(objv[3], NULL),
			    "includeids") == 0)) {
		includeVisualId = 1;
	    } else {
		Tcl_WrongNumArgs(interp, 2, objv, "window ?includeids?");
		return TCL_ERROR;
	    }

	    string = Tcl_GetStringFromObj(objv[2], NULL);
	    tkwin = Tk_NameToWindow(interp, string, tkwin); 
	    if (tkwin == NULL) { 
		return TCL_ERROR; 
	    }

	    template.screen = Tk_ScreenNumber(tkwin);
	    visInfoPtr = XGetVisualInfo(Tk_Display(tkwin), VisualScreenMask,
		    &template, &count);
	    if (visInfoPtr == NULL) {
		Tcl_SetStringObj(resultPtr,
			"can't find any visuals for screen", -1);
		return TCL_ERROR;
	    }
	    for (i = 0; i < count; i++) {
		string = TkFindStateString(visualMap, visInfoPtr[i].class);
		if (string == NULL) {
		    strcpy(buf, "unknown");
		} else {
		    sprintf(buf, "%s %d", string, visInfoPtr[i].depth);
		}
		if (includeVisualId) {
		    sprintf(visualIdString, " 0x%x",
			    (unsigned int) visInfoPtr[i].visualid);
		    strcat(buf, visualIdString);
		}
		strPtr = Tcl_NewStringObj(buf, -1);
		Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	    }
	    XFree((char *) visInfoPtr);
	    break;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetDisplayOf --
 *
 *	Parses a "-displayof window" option for various commands.  If
 *	present, the literal "-displayof" should be in objv[0] and the
 *	window name in objv[1].
 *
 * Results:
 *	The return value is 0 if the argument strings did not contain
 *	the "-displayof" option.  The return value is 2 if the
 *	argument strings contained both the "-displayof" option and
 *	a valid window name.  Otherwise, the return value is -1 if
 *	the window name was missing or did not specify a valid window.
 *
 *	If the return value was 2, *tkwinPtr is filled with the
 *	token for the window specified on the command line.  If the
 *	return value was -1, an error message is left in interp's
 *	result object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkGetDisplayOf(interp, objc, objv, tkwinPtr)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. If it is present,
				 * "-displayof" should be in objv[0] and
				 * objv[1] the name of a window. */
    Tk_Window *tkwinPtr;	/* On input, contains main window of
				 * application associated with interp.  On
				 * output, filled with window specified as
				 * option to "-displayof" argument, or
				 * unmodified if "-displayof" argument was not
				 * present. */
{
    char *string;
    int length;
    
    if (objc < 1) {
	return 0;
    }
    string = Tcl_GetStringFromObj(objv[0], &length);
    if ((length >= 2) && (strncmp(string, "-displayof", (unsigned) length) == 0)) {
        if (objc < 2) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp),
		    "value for \"-displayof\" missing", -1);
	    return -1;
	}
	string = Tcl_GetStringFromObj(objv[1], NULL);
	*tkwinPtr = Tk_NameToWindow(interp, string, *tkwinPtr);
	if (*tkwinPtr == NULL) {
	    return -1;
	}
	return 2;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDeadAppCmd --
 *
 *	If an application has been deleted then all Tk commands will be
 *	re-bound to this procedure.
 *
 * Results:
 *	A standard Tcl error is reported to let the user know that
 *	the application is dead.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkDeadAppCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Dummy. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tcl_AppendResult(interp, "can't invoke \"", argv[0],
	    "\" command:  application has been destroyed", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetToplevel --
 *
 *	Retrieves the toplevel window which is the nearest ancestor of
 *	of the specified window.
 *
 * Results:
 *	Returns the toplevel window or NULL if the window has no
 *	ancestor which is a toplevel.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkWindow *
GetToplevel(tkwin)
    Tk_Window tkwin;		/* Window for which the toplevel should be
				 * deterined. */
{
     TkWindow *winPtr = (TkWindow *) tkwin;

     while (!(winPtr->flags & TK_TOP_LEVEL)) {
	 winPtr = winPtr->parentPtr;
	 if (winPtr == NULL) {
	     return NULL;
	 }
     }
     return winPtr;
}
