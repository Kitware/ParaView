/* 
 * tkScrollbar.c --
 *
 *	This module implements a scrollbar widgets for the Tk
 *	toolkit.  A scrollbar displays a slider and two arrows;
 *	mouse clicks on features within the scrollbar cause
 *	scrolling commands to be invoked.
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
#include "tkScrollbar.h"
#include "default.h"

/*
 * Information used for argv parsing.
 */

Tk_ConfigSpec tkpScrollbarConfigSpecs[] = {
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCROLLBAR_ACTIVE_BG_COLOR, Tk_Offset(TkScrollbar, activeBorder),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCROLLBAR_ACTIVE_BG_MONO, Tk_Offset(TkScrollbar, activeBorder),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
	DEF_SCROLLBAR_ACTIVE_RELIEF, Tk_Offset(TkScrollbar, activeRelief), 0},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_SCROLLBAR_BG_COLOR, Tk_Offset(TkScrollbar, bgBorder),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_SCROLLBAR_BG_MONO, Tk_Offset(TkScrollbar, bgBorder),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_SCROLLBAR_BORDER_WIDTH, Tk_Offset(TkScrollbar, borderWidth), 0},
    {TK_CONFIG_STRING, "-command", "command", "Command",
	DEF_SCROLLBAR_COMMAND, Tk_Offset(TkScrollbar, command),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_SCROLLBAR_CURSOR, Tk_Offset(TkScrollbar, cursor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-elementborderwidth", "elementBorderWidth",
	"BorderWidth", DEF_SCROLLBAR_EL_BORDER_WIDTH,
	Tk_Offset(TkScrollbar, elementBorderWidth), 0},
    {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_SCROLLBAR_HIGHLIGHT_BG,
	Tk_Offset(TkScrollbar, highlightBgColorPtr), 0},
    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_SCROLLBAR_HIGHLIGHT,
	Tk_Offset(TkScrollbar, highlightColorPtr), 0},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_SCROLLBAR_HIGHLIGHT_WIDTH, Tk_Offset(TkScrollbar, highlightWidth), 0},
    {TK_CONFIG_BOOLEAN, "-jump", "jump", "Jump",
	DEF_SCROLLBAR_JUMP, Tk_Offset(TkScrollbar, jump), 0},
    {TK_CONFIG_UID, "-orient", "orient", "Orient",
	DEF_SCROLLBAR_ORIENT, Tk_Offset(TkScrollbar, orientUid), 0},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_SCROLLBAR_RELIEF, Tk_Offset(TkScrollbar, relief), 0},
    {TK_CONFIG_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
	DEF_SCROLLBAR_REPEAT_DELAY, Tk_Offset(TkScrollbar, repeatDelay), 0},
    {TK_CONFIG_INT, "-repeatinterval", "repeatInterval", "RepeatInterval",
	DEF_SCROLLBAR_REPEAT_INTERVAL, Tk_Offset(TkScrollbar, repeatInterval), 0},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_SCROLLBAR_TAKE_FOCUS, Tk_Offset(TkScrollbar, takeFocus),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-troughcolor", "troughColor", "Background",
	DEF_SCROLLBAR_TROUGH_COLOR, Tk_Offset(TkScrollbar, troughColorPtr),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_COLOR, "-troughcolor", "troughColor", "Background",
	DEF_SCROLLBAR_TROUGH_MONO, Tk_Offset(TkScrollbar, troughColorPtr),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	DEF_SCROLLBAR_WIDTH, Tk_Offset(TkScrollbar, width), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ConfigureScrollbar _ANSI_ARGS_((Tcl_Interp *interp,
			    TkScrollbar *scrollPtr, int argc, char **argv,
			    int flags));
static void		ScrollbarCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static int		ScrollbarWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *, int argc, char **argv));

/*
 *--------------------------------------------------------------
 *
 * Tk_ScrollbarCmd --
 *
 *	This procedure is invoked to process the "scrollbar" Tcl
 *	command.  See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_ScrollbarCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    register TkScrollbar *scrollPtr;
    Tk_Window new;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    new = Tk_CreateWindowFromPath(interp, tkwin, argv[1], (char *) NULL);
    if (new == NULL) {
	return TCL_ERROR;
    }

    Tk_SetClass(new, "Scrollbar");
    scrollPtr = TkpCreateScrollbar(new);

    TkSetClassProcs(new, &tkpScrollbarProcs, (ClientData) scrollPtr);

    /*
     * Initialize fields that won't be initialized by ConfigureScrollbar,
     * or which ConfigureScrollbar expects to have reasonable values
     * (e.g. resource pointers).
     */

    scrollPtr->tkwin = new;
    scrollPtr->display = Tk_Display(new);
    scrollPtr->interp = interp;
    scrollPtr->widgetCmd = Tcl_CreateCommand(interp,
	    Tk_PathName(scrollPtr->tkwin), ScrollbarWidgetCmd,
	    (ClientData) scrollPtr, ScrollbarCmdDeletedProc);
    scrollPtr->orientUid = NULL;
    scrollPtr->vertical = 0;
    scrollPtr->width = 0;
    scrollPtr->command = NULL;
    scrollPtr->commandSize = 0;
    scrollPtr->repeatDelay = 0;
    scrollPtr->repeatInterval = 0;
    scrollPtr->borderWidth = 0;
    scrollPtr->bgBorder = NULL;
    scrollPtr->activeBorder = NULL;
    scrollPtr->troughColorPtr = NULL;
    scrollPtr->relief = TK_RELIEF_FLAT;
    scrollPtr->highlightWidth = 0;
    scrollPtr->highlightBgColorPtr = NULL;
    scrollPtr->highlightColorPtr = NULL;
    scrollPtr->inset = 0;
    scrollPtr->elementBorderWidth = -1;
    scrollPtr->arrowLength = 0;
    scrollPtr->sliderFirst = 0;
    scrollPtr->sliderLast = 0;
    scrollPtr->activeField = 0;
    scrollPtr->activeRelief = TK_RELIEF_RAISED;
    scrollPtr->totalUnits = 0;
    scrollPtr->windowUnits = 0;
    scrollPtr->firstUnit = 0;
    scrollPtr->lastUnit = 0;
    scrollPtr->firstFraction = 0.0;
    scrollPtr->lastFraction = 0.0;
    scrollPtr->cursor = None;
    scrollPtr->takeFocus = NULL;
    scrollPtr->flags = 0;

    if (ConfigureScrollbar(interp, scrollPtr, argc-2, argv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(scrollPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetResult(interp, Tk_PathName(scrollPtr->tkwin), TCL_STATIC);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
ScrollbarWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Information about scrollbar
					 * widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    register TkScrollbar *scrollPtr = (TkScrollbar *) clientData;
    int result = TCL_OK;
    size_t length;
    int c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) scrollPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'a') && (strncmp(argv[1], "activate", length) == 0)) {
	int oldActiveField;
	if (argc == 2) {
	    switch (scrollPtr->activeField) {
		case TOP_ARROW:
		    Tcl_SetResult(interp, "arrow1", TCL_STATIC);
		    break;
		case SLIDER:
		    Tcl_SetResult(interp, "slider", TCL_STATIC);
		    break;
		case BOTTOM_ARROW:
		    Tcl_SetResult(interp, "arrow2", TCL_STATIC);
		    break;
	    }
	    goto done;
	}
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " activate element\"", (char *) NULL);
	    goto error;
	}
	c = argv[2][0];
	length = strlen(argv[2]);
	oldActiveField = scrollPtr->activeField;
	if ((c == 'a') && (strcmp(argv[2], "arrow1") == 0)) {
	    scrollPtr->activeField = TOP_ARROW;
	} else if ((c == 'a') && (strcmp(argv[2], "arrow2") == 0)) {
	    scrollPtr->activeField = BOTTOM_ARROW;
	} else if ((c == 's') && (strncmp(argv[2], "slider", length) == 0)) {
	    scrollPtr->activeField = SLIDER;
	} else {
	    scrollPtr->activeField = OUTSIDE;
	}
	if (oldActiveField != scrollPtr->activeField) {
	    TkScrollbarEventuallyRedraw(scrollPtr);
	}
    } else if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)
	    && (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " cget option\"",
		    (char *) NULL);
	    goto error;
	}
	result = Tk_ConfigureValue(interp, scrollPtr->tkwin,
		tkpScrollbarConfigSpecs, (char *) scrollPtr, argv[2], 0);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)
	    && (length >= 2)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, scrollPtr->tkwin,
		    tkpScrollbarConfigSpecs, (char *) scrollPtr,
		    (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, scrollPtr->tkwin,
		    tkpScrollbarConfigSpecs, (char *) scrollPtr, argv[2], 0);
	} else {
	    result = ConfigureScrollbar(interp, scrollPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else if ((c == 'd') && (strncmp(argv[1], "delta", length) == 0)) {
	int xDelta, yDelta, pixels, length;
	double fraction;
	char buf[TCL_DOUBLE_SPACE];

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " delta xDelta yDelta\"", (char *) NULL);
	    goto error;
	}
	if ((Tcl_GetInt(interp, argv[2], &xDelta) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &yDelta) != TCL_OK)) {
	    goto error;
	}
	if (scrollPtr->vertical) {
	    pixels = yDelta;
	    length = Tk_Height(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	} else {
	    pixels = xDelta;
	    length = Tk_Width(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	}
	if (length == 0) {
	    fraction = 0.0;
	} else {
	    fraction = ((double) pixels / (double) length);
	}
	sprintf(buf, "%g", fraction);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if ((c == 'f') && (strncmp(argv[1], "fraction", length) == 0)) {
	int x, y, pos, length;
	double fraction;
	char buf[TCL_DOUBLE_SPACE];

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " fraction x y\"", (char *) NULL);
	    goto error;
	}
	if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &y) != TCL_OK)) {
	    goto error;
	}
	if (scrollPtr->vertical) {
	    pos = y - (scrollPtr->arrowLength + scrollPtr->inset);
	    length = Tk_Height(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	} else {
	    pos = x - (scrollPtr->arrowLength + scrollPtr->inset);
	    length = Tk_Width(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	}
	if (length == 0) {
	    fraction = 0.0;
	} else {
	    fraction = ((double) pos / (double) length);
	}
	if (fraction < 0) {
	    fraction = 0;
	} else if (fraction > 1.0) {
	    fraction = 1.0;
	}
	sprintf(buf, "%g", fraction);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if ((c == 'g') && (strncmp(argv[1], "get", length) == 0)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " get\"", (char *) NULL);
	    goto error;
	}
	if (scrollPtr->flags & NEW_STYLE_COMMANDS) {
	    char first[TCL_DOUBLE_SPACE], last[TCL_DOUBLE_SPACE];

	    Tcl_PrintDouble(interp, scrollPtr->firstFraction, first);
	    Tcl_PrintDouble(interp, scrollPtr->lastFraction, last);
	    Tcl_AppendResult(interp, first, " ", last, (char *) NULL);
	} else {
	    char buf[TCL_INTEGER_SPACE * 4];

	    sprintf(buf, "%d %d %d %d", scrollPtr->totalUnits,
		    scrollPtr->windowUnits, scrollPtr->firstUnit,
		    scrollPtr->lastUnit);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
    } else if ((c == 'i') && (strncmp(argv[1], "identify", length) == 0)) {
	int x, y, thing;

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " identify x y\"", (char *) NULL);
	    goto error;
	}
	if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &y) != TCL_OK)) {
	    goto error;
	}
	thing = TkpScrollbarPosition(scrollPtr, x,y);
	switch (thing) {
	    case TOP_ARROW:
		Tcl_SetResult(interp, "arrow1", TCL_STATIC);
		break;
	    case TOP_GAP:
		Tcl_SetResult(interp, "trough1", TCL_STATIC);
		break;
	    case SLIDER:
		Tcl_SetResult(interp, "slider", TCL_STATIC);
		break;
	    case BOTTOM_GAP:
		Tcl_SetResult(interp, "trough2", TCL_STATIC);
		break;
	    case BOTTOM_ARROW:
		Tcl_SetResult(interp, "arrow2", TCL_STATIC);
		break;
	}
    } else if ((c == 's') && (strncmp(argv[1], "set", length) == 0)) {
	int totalUnits, windowUnits, firstUnit, lastUnit;

	if (argc == 4) {
	    double first, last;

	    if (Tcl_GetDouble(interp, argv[2], &first) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetDouble(interp, argv[3], &last) != TCL_OK) {
		goto error;
	    }
	    if (first < 0) {
		scrollPtr->firstFraction = 0;
	    } else if (first > 1.0) {
		scrollPtr->firstFraction = 1.0;
	    } else {
		scrollPtr->firstFraction = first;
	    }
	    if (last < scrollPtr->firstFraction) {
		scrollPtr->lastFraction = scrollPtr->firstFraction;
	    } else if (last > 1.0) {
		scrollPtr->lastFraction = 1.0;
	    } else {
		scrollPtr->lastFraction = last;
	    }
	    scrollPtr->flags |= NEW_STYLE_COMMANDS;
	} else if (argc == 6) {
	    if (Tcl_GetInt(interp, argv[2], &totalUnits) != TCL_OK) {
		goto error;
	    }
	    if (totalUnits < 0) {
		totalUnits = 0;
	    }
	    if (Tcl_GetInt(interp, argv[3], &windowUnits) != TCL_OK) {
		goto error;
	    }
	    if (windowUnits < 0) {
		windowUnits = 0;
	    }
	    if (Tcl_GetInt(interp, argv[4], &firstUnit) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetInt(interp, argv[5], &lastUnit) != TCL_OK) {
		goto error;
	    }
	    if (totalUnits > 0) {
		if (lastUnit < firstUnit) {
		    lastUnit = firstUnit;
		}
	    } else {
		firstUnit = lastUnit = 0;
	    }
	    scrollPtr->totalUnits = totalUnits;
	    scrollPtr->windowUnits = windowUnits;
	    scrollPtr->firstUnit = firstUnit;
	    scrollPtr->lastUnit = lastUnit;
	    if (scrollPtr->totalUnits == 0) {
		scrollPtr->firstFraction = 0.0;
		scrollPtr->lastFraction = 1.0;
	    } else {
		scrollPtr->firstFraction = ((double) firstUnit)/totalUnits;
		scrollPtr->lastFraction = ((double) (lastUnit+1))/totalUnits;
	    }
	    scrollPtr->flags &= ~NEW_STYLE_COMMANDS;
	} else {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " set firstFraction lastFraction\" or \"",
		    argv[0],
		    " set totalUnits windowUnits firstUnit lastUnit\"",
		    (char *) NULL);
	    goto error;
	}
	TkpComputeScrollbarGeometry(scrollPtr);
	TkScrollbarEventuallyRedraw(scrollPtr);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be activate, cget, configure, delta, fraction, ",
		"get, identify, or set", (char *) NULL);
	goto error;
    }
    done:
    Tcl_Release((ClientData) scrollPtr);
    return result;

    error:
    Tcl_Release((ClientData) scrollPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureScrollbar --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a scrollbar widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for scrollPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureScrollbar(interp, scrollPtr, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    register TkScrollbar *scrollPtr;	/* Information about widget;  may or
					 * may not already have values for
					 * some fields. */
    int argc;				/* Number of valid entries in argv. */
    char **argv;			/* Arguments. */
    int flags;				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
    size_t length;

    if (Tk_ConfigureWidget(interp, scrollPtr->tkwin, tkpScrollbarConfigSpecs,
	    argc, argv, (char *) scrollPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few options need special processing, such as parsing the
     * orientation or setting the background from a 3-D border.
     */

    length = strlen(scrollPtr->orientUid);
    if (strncmp(scrollPtr->orientUid, "vertical", length) == 0) {
	scrollPtr->vertical = 1;
    } else if (strncmp(scrollPtr->orientUid, "horizontal", length) == 0) {
	scrollPtr->vertical = 0;
    } else {
	Tcl_AppendResult(interp, "bad orientation \"", scrollPtr->orientUid,
		"\": must be vertical or horizontal", (char *) NULL);
	return TCL_ERROR;
    }

    if (scrollPtr->command != NULL) {
	scrollPtr->commandSize = strlen(scrollPtr->command);
    } else {
	scrollPtr->commandSize = 0;
    }

    /*
     * Configure platform specific options.
     */

    TkpConfigureScrollbar(scrollPtr);

    /*
     * Register the desired geometry for the window (leave enough space
     * for the two arrows plus a minimum-size slider, plus border around
     * the whole window, if any).  Then arrange for the window to be
     * redisplayed.
     */

    TkpComputeScrollbarGeometry(scrollPtr);
    TkScrollbarEventuallyRedraw(scrollPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkScrollbarEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on scrollbars.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

void
TkScrollbarEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    TkScrollbar *scrollPtr = (TkScrollbar *) clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	TkScrollbarEventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == DestroyNotify) {
	TkpDestroyScrollbar(scrollPtr);
	if (scrollPtr->tkwin != NULL) {
	    scrollPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(scrollPtr->interp,
                    scrollPtr->widgetCmd);
	}
	if (scrollPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(TkpDisplayScrollbar, (ClientData) scrollPtr);
	}
	/*
	 * Free up all the stuff that requires special handling, then
	 * let Tk_FreeOptions handle all the standard option-related
	 * stuff.
	 */
	
	Tk_FreeOptions(tkpScrollbarConfigSpecs, (char *) scrollPtr,
		scrollPtr->display, 0);
	Tcl_EventuallyFree((ClientData) scrollPtr, TCL_DYNAMIC);
    } else if (eventPtr->type == ConfigureNotify) {
	TkpComputeScrollbarGeometry(scrollPtr);
	TkScrollbarEventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scrollPtr->flags |= GOT_FOCUS;
	    if (scrollPtr->highlightWidth > 0) {
		TkScrollbarEventuallyRedraw(scrollPtr);
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scrollPtr->flags &= ~GOT_FOCUS;
	    if (scrollPtr->highlightWidth > 0) {
		TkScrollbarEventuallyRedraw(scrollPtr);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ScrollbarCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
ScrollbarCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TkScrollbar *scrollPtr = (TkScrollbar *) clientData;
    Tk_Window tkwin = scrollPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	scrollPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkScrollbarEventuallyRedraw --
 *
 *	Arrange for one or more of the fields of a scrollbar
 *	to be redrawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkScrollbarEventuallyRedraw(scrollPtr)
    register TkScrollbar *scrollPtr;	/* Information about widget. */
{
    if ((scrollPtr->tkwin == NULL) || (!Tk_IsMapped(scrollPtr->tkwin))) {
	return;
    }
    if ((scrollPtr->flags & REDRAW_PENDING) == 0) {
	Tcl_DoWhenIdle(TkpDisplayScrollbar, (ClientData) scrollPtr);
	scrollPtr->flags |= REDRAW_PENDING;
    }
}
