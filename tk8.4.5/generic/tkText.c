/* 
 * tkText.c --
 *
 *	This module provides a big chunk of the implementation of
 *	multi-line editable text widgets for Tk.  Among other things,
 *	it provides the Tcl command interfaces to text widgets and
 *	the display code.  The B-tree representation of text is
 *	implemented elsewhere.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "default.h"
#include "tkPort.h"
#include "tkInt.h"
#include "tkUndo.h"

#if defined(MAC_TCL) || defined(MAC_OSX_TK)
#define Style TkStyle
#define DInfo TkDInfo
#endif

#include "tkText.h"

/*
 * Custom options for handling "-state"
 */

static Tk_CustomOption stateOption = {
    (Tk_OptionParseProc *) TkStateParseProc,
    TkStatePrintProc, (ClientData) NULL /* only "normal" and "disabled" */
};

/*
 * Information used to parse text configuration options:
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_BOOLEAN, "-autoseparators", "autoSeparators",
        "AutoSeparators", DEF_TEXT_AUTO_SEPARATORS,
        Tk_Offset(TkText, autoSeparators), 0},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_TEXT_BG_COLOR, Tk_Offset(TkText, border), TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_TEXT_BG_MONO, Tk_Offset(TkText, border), TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_TEXT_BORDER_WIDTH, Tk_Offset(TkText, borderWidth), 0},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_TEXT_CURSOR, Tk_Offset(TkText, cursor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_BOOLEAN, "-exportselection", "exportSelection",
	"ExportSelection", DEF_TEXT_EXPORT_SELECTION,
	Tk_Offset(TkText, exportSelection), 0},
    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_FONT, "-font", "font", "Font",
	DEF_TEXT_FONT, Tk_Offset(TkText, tkfont), 0},
    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_TEXT_FG, Tk_Offset(TkText, fgColor), 0},
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
	DEF_TEXT_HEIGHT, Tk_Offset(TkText, height), 0},
    {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_TEXT_HIGHLIGHT_BG,
	Tk_Offset(TkText, highlightBgColorPtr), 0},
    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_TEXT_HIGHLIGHT, Tk_Offset(TkText, highlightColorPtr), 0},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_TEXT_HIGHLIGHT_WIDTH, Tk_Offset(TkText, highlightWidth), 0},
    {TK_CONFIG_BORDER, "-insertbackground", "insertBackground", "Foreground",
	DEF_TEXT_INSERT_BG, Tk_Offset(TkText, insertBorder), 0},
    {TK_CONFIG_PIXELS, "-insertborderwidth", "insertBorderWidth", "BorderWidth",
	DEF_TEXT_INSERT_BD_COLOR, Tk_Offset(TkText, insertBorderWidth),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_PIXELS, "-insertborderwidth", "insertBorderWidth", "BorderWidth",
	DEF_TEXT_INSERT_BD_MONO, Tk_Offset(TkText, insertBorderWidth),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_INT, "-insertofftime", "insertOffTime", "OffTime",
	DEF_TEXT_INSERT_OFF_TIME, Tk_Offset(TkText, insertOffTime), 0},
    {TK_CONFIG_INT, "-insertontime", "insertOnTime", "OnTime",
	DEF_TEXT_INSERT_ON_TIME, Tk_Offset(TkText, insertOnTime), 0},
    {TK_CONFIG_PIXELS, "-insertwidth", "insertWidth", "InsertWidth",
	DEF_TEXT_INSERT_WIDTH, Tk_Offset(TkText, insertWidth), 0},
    {TK_CONFIG_INT, "-maxundo", "maxUndo", "MaxUndo",
	DEF_TEXT_MAX_UNDO, Tk_Offset(TkText, maxUndo), 0},
    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_TEXT_PADX, Tk_Offset(TkText, padX), 0},
    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_TEXT_PADY, Tk_Offset(TkText, padY), 0},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_TEXT_RELIEF, Tk_Offset(TkText, relief), 0},
    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_TEXT_SELECT_COLOR, Tk_Offset(TkText, selBorder),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_TEXT_SELECT_MONO, Tk_Offset(TkText, selBorder),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_STRING, "-selectborderwidth", "selectBorderWidth", "BorderWidth",
	DEF_TEXT_SELECT_BD_COLOR, Tk_Offset(TkText, selBdString),
	TK_CONFIG_COLOR_ONLY|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-selectborderwidth", "selectBorderWidth", "BorderWidth",
	DEF_TEXT_SELECT_BD_MONO, Tk_Offset(TkText, selBdString),
	TK_CONFIG_MONO_ONLY|TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_TEXT_SELECT_FG_COLOR, Tk_Offset(TkText, selFgColorPtr),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_TEXT_SELECT_FG_MONO, Tk_Offset(TkText, selFgColorPtr),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_BOOLEAN, "-setgrid", "setGrid", "SetGrid",
	DEF_TEXT_SET_GRID, Tk_Offset(TkText, setGrid), 0},
    {TK_CONFIG_PIXELS, "-spacing1", "spacing1", "Spacing",
	DEF_TEXT_SPACING1, Tk_Offset(TkText, spacing1),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_PIXELS, "-spacing2", "spacing2", "Spacing",
	DEF_TEXT_SPACING2, Tk_Offset(TkText, spacing2),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_PIXELS, "-spacing3", "spacing3", "Spacing",
	DEF_TEXT_SPACING3, Tk_Offset(TkText, spacing3),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, "-state", "state", "State",
	DEF_TEXT_STATE, Tk_Offset(TkText, state), 0, &stateOption},
    {TK_CONFIG_STRING, "-tabs", "tabs", "Tabs",
	DEF_TEXT_TABS, Tk_Offset(TkText, tabOptionString), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TEXT_TAKE_FOCUS, Tk_Offset(TkText, takeFocus),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BOOLEAN, "-undo", "undo", "Undo",
        DEF_TEXT_UNDO, Tk_Offset(TkText, undo), 0},
    {TK_CONFIG_INT, "-width", "width", "Width",
	DEF_TEXT_WIDTH, Tk_Offset(TkText, width), 0},
    {TK_CONFIG_CUSTOM, "-wrap", "wrap", "Wrap",
	DEF_TEXT_WRAP, Tk_Offset(TkText, wrapMode), 0, &textWrapModeOption},
    {TK_CONFIG_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_TEXT_XSCROLL_COMMAND, Tk_Offset(TkText, xScrollCmd),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	DEF_TEXT_YSCROLL_COMMAND, Tk_Offset(TkText, yScrollCmd),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Boolean variable indicating whether or not special debugging code
 * should be executed.
 */

int tkTextDebug = 0;

/*
 * Custom options for handling "-wrap":
 */

static int		WrapModeParseProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    CONST char *value, char *widgRec, int offset));
static char *		WrapModePrintProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin, char *widgRec, int offset,
			    Tcl_FreeProc **freeProcPtr));

Tk_CustomOption textWrapModeOption = {
    WrapModeParseProc,
    WrapModePrintProc,
    (ClientData) NULL
};

/*
 *--------------------------------------------------------------
 *
 * WrapModeParseProc --
 *
 *	This procedure is invoked during option processing to handle
 *	"-wrap" options for text widgets.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The wrap mode for a given item gets replaced by the wrap mode
 *	indicated in the value argument.
 *
 *--------------------------------------------------------------
 */

static int
WrapModeParseProc(clientData, interp, tkwin, value, widgRec, offset)
    ClientData clientData;		/* some flags.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    CONST char *value;			/* Value of option (list of tag
					 * names). */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
{
    int c;
    size_t length;

    register TkWrapMode *wrapPtr = (TkWrapMode *) (widgRec + offset);

    if(value == NULL || *value == 0) {
	*wrapPtr = TEXT_WRAPMODE_NULL;
	return TCL_OK;
    }

    c = value[0];
    length = strlen(value);

    if ((c == 'c') && (strncmp(value, "char", length) == 0)) {
	*wrapPtr = TEXT_WRAPMODE_CHAR;
	return TCL_OK;
    }
    if ((c == 'n') && (strncmp(value, "none", length) == 0)) {
	*wrapPtr = TEXT_WRAPMODE_NONE;
	return TCL_OK;
    }
    if ((c == 'w') && (strncmp(value, "word", length) == 0)) {
	*wrapPtr = TEXT_WRAPMODE_WORD;
	return TCL_OK;
    }
    Tcl_AppendResult(interp, "bad wrap mode \"", value,
	    "\": must be char, none, or word",
	    (char *) NULL);
    *wrapPtr = TEXT_WRAPMODE_CHAR;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * WrapModePrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-wrap" configuration
 *	option for canvas items.
 *
 * Results:
 *	The return value is a string describing the state for
 *	the item referred to by "widgRec".  In addition, *freeProcPtr
 *	is filled in with the address of a procedure to call to free
 *	the result string when it's no longer needed (or NULL to
 *	indicate that the string doesn't need to be freed).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static char *
WrapModePrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Ignored. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    register TkWrapMode *wrapPtr = (TkWrapMode *) (widgRec + offset);

    if (*wrapPtr==TEXT_WRAPMODE_CHAR) {
	return "char";
    } else if (*wrapPtr==TEXT_WRAPMODE_NONE) {
	return "none";
    } else if (*wrapPtr==TEXT_WRAPMODE_WORD) {
	return "word";
    } else {
	return "";
    }
}

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ConfigureText _ANSI_ARGS_((Tcl_Interp *interp,
			    TkText *textPtr, int argc, CONST char **argv,
			    int flags));
static int		DeleteChars _ANSI_ARGS_((TkText *textPtr,
			    CONST char *index1String, CONST char *index2String,
			    TkTextIndex *indexPtr1, TkTextIndex *indexPtr2));
static void		DestroyText _ANSI_ARGS_((char *memPtr));
static void		InsertChars _ANSI_ARGS_((TkText *textPtr,
			    TkTextIndex *indexPtr, CONST char *string));
static void		TextBlinkProc _ANSI_ARGS_((ClientData clientData));
static void		TextCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		TextEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		TextFetchSelection _ANSI_ARGS_((ClientData clientData,
			    int offset, char *buffer, int maxBytes));
static int		TextIndexSortProc _ANSI_ARGS_((CONST VOID *first,
			    CONST VOID *second));
static int		TextSearchCmd _ANSI_ARGS_((TkText *textPtr,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TextEditCmd _ANSI_ARGS_((TkText *textPtr,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TextWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static void		TextWorldChanged _ANSI_ARGS_((
			    ClientData instanceData));
static int		TextDumpCmd _ANSI_ARGS_((TkText *textPtr,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static void		DumpLine _ANSI_ARGS_((Tcl_Interp *interp, 
			    TkText *textPtr, int what, TkTextLine *linePtr,
			    int start, int end, int lineno,
			    CONST char *command));
static int		DumpSegment _ANSI_ARGS_((Tcl_Interp *interp, char *key,
			    char *value, CONST char * command,
			    TkTextIndex *index, int what));
static int		TextEditUndo _ANSI_ARGS_((TkText *textPtr));
static int		TextEditRedo _ANSI_ARGS_((TkText *textPtr));
static void		TextGetText _ANSI_ARGS_((TkTextIndex * index1,
			    TkTextIndex * index2, Tcl_DString *dsPtr));
static void		updateDirtyFlag _ANSI_ARGS_((TkText *textPtr));

/*
 * The structure below defines text class behavior by means of procedures
 * that can be invoked from generic window code.
 */

static Tk_ClassProcs textClass = {
    sizeof(Tk_ClassProcs),	/* size */
    TextWorldChanged,		/* worldChangedProc */
};


/*
 *--------------------------------------------------------------
 *
 * Tk_TextCmd --
 *
 *	This procedure is invoked to process the "text" Tcl command.
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

int
Tk_TextCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    Tk_Window new;
    register TkText *textPtr;
    TkTextIndex startIndex;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Create the window.
     */

    new = Tk_CreateWindowFromPath(interp, tkwin, argv[1], (char *) NULL);
    if (new == NULL) {
	return TCL_ERROR;
    }

    /*
     * Create the text widget and initialize everything to zero,
     * then set the necessary initial (non-NULL) values.
     */

    textPtr = (TkText *) ckalloc(sizeof(TkText));
    memset((VOID *) textPtr, 0, sizeof(TkText));

    textPtr->tkwin = new;
    textPtr->display = Tk_Display(new);
    textPtr->interp = interp;
    textPtr->widgetCmd = Tcl_CreateCommand(interp,
	    Tk_PathName(textPtr->tkwin), TextWidgetCmd,
	    (ClientData) textPtr, TextCmdDeletedProc);
    textPtr->tree = TkBTreeCreate(textPtr);
    Tcl_InitHashTable(&textPtr->tagTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&textPtr->markTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&textPtr->windowTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&textPtr->imageTable, TCL_STRING_KEYS);
    textPtr->state = TK_STATE_NORMAL;
    textPtr->relief = TK_RELIEF_FLAT;
    textPtr->cursor = None;
    textPtr->charWidth = 1;
    textPtr->wrapMode = TEXT_WRAPMODE_CHAR;
    textPtr->prevWidth = Tk_Width(new);
    textPtr->prevHeight = Tk_Height(new);
    TkTextCreateDInfo(textPtr);
    TkTextMakeByteIndex(textPtr->tree, 0, 0, &startIndex);
    TkTextSetYView(textPtr, &startIndex, 0);
    textPtr->exportSelection = 1;
    textPtr->pickEvent.type = LeaveNotify;
    textPtr->undoStack = TkUndoInitStack(interp,0);
    textPtr->undo = 1;
    textPtr->isDirtyIncrement = 1;
    textPtr->autoSeparators = 1;
    textPtr->lastEditMode = TK_TEXT_EDIT_OTHER;

    /*
     * Create the "sel" tag and the "current" and "insert" marks.
     */

    textPtr->selTagPtr = TkTextCreateTag(textPtr, "sel");
    textPtr->selTagPtr->reliefString =
	    (char *) ckalloc(sizeof(DEF_TEXT_SELECT_RELIEF));
    strcpy(textPtr->selTagPtr->reliefString, DEF_TEXT_SELECT_RELIEF);
    textPtr->selTagPtr->relief = TK_RELIEF_RAISED;
    textPtr->currentMarkPtr = TkTextSetMark(textPtr, "current", &startIndex);
    textPtr->insertMarkPtr = TkTextSetMark(textPtr, "insert", &startIndex);

    Tk_SetClass(textPtr->tkwin, "Text");
    Tk_SetClassProcs(textPtr->tkwin, &textClass, (ClientData) textPtr);
    Tk_CreateEventHandler(textPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    TextEventProc, (ClientData) textPtr);
    Tk_CreateEventHandler(textPtr->tkwin, KeyPressMask|KeyReleaseMask
	    |ButtonPressMask|ButtonReleaseMask|EnterWindowMask
	    |LeaveWindowMask|PointerMotionMask|VirtualEventMask,
	    TkTextBindProc, (ClientData) textPtr);
    Tk_CreateSelHandler(textPtr->tkwin, XA_PRIMARY, XA_STRING,
	    TextFetchSelection, (ClientData) textPtr, XA_STRING);
    if (ConfigureText(interp, textPtr, argc-2, argv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(textPtr->tkwin);
	return TCL_ERROR;
    }
    Tcl_SetResult(interp, Tk_PathName(textPtr->tkwin), TCL_STATIC);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TextWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a text widget.  See the user
 *	documentation for details on what it does.
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
TextWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Information about text widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST char **argv;		/* Argument strings. */
{
    register TkText *textPtr = (TkText *) clientData;
    int c, result = TCL_OK;
    size_t length;
    TkTextIndex index1, index2;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) textPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'b') && (strncmp(argv[1], "bbox", length) == 0)) {
	int x, y, width, height;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " bbox index\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextGetIndex(interp, textPtr, argv[2], &index1) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextCharBbox(textPtr, &index1, &x, &y, &width, &height) == 0) {
	    char buf[TCL_INTEGER_SPACE * 4];
	    
	    sprintf(buf, "%d %d %d %d", x, y, width, height);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
    } else if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)
	    && (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " cget option\"",
		    (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	result = Tk_ConfigureValue(interp, textPtr->tkwin, configSpecs,
		(char *) textPtr, argv[2], 0);
    } else if ((c == 'c') && (strncmp(argv[1], "compare", length) == 0)
	    && (length >= 3)) {
	int relation, value;
	CONST char *p;

	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " compare index1 op index2\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if ((TkTextGetIndex(interp, textPtr, argv[2], &index1) != TCL_OK)
		|| (TkTextGetIndex(interp, textPtr, argv[4], &index2)
		!= TCL_OK)) {
	    result = TCL_ERROR;
	    goto done;
	}
	relation = TkTextIndexCmp(&index1, &index2);
	p = argv[3];
	if (p[0] == '<') {
		value = (relation < 0);
	    if ((p[1] == '=') && (p[2] == 0)) {
		value = (relation <= 0);
	    } else if (p[1] != 0) {
		compareError:
		Tcl_AppendResult(interp, "bad comparison operator \"",
			argv[3], "\": must be <, <=, ==, >=, >, or !=",
			(char *) NULL);
		result = TCL_ERROR;
		goto done;
	    }
	} else if (p[0] == '>') {
		value = (relation > 0);
	    if ((p[1] == '=') && (p[2] == 0)) {
		value = (relation >= 0);
	    } else if (p[1] != 0) {
		goto compareError;
	    }
	} else if ((p[0] == '=') && (p[1] == '=') && (p[2] == 0)) {
	    value = (relation == 0);
	} else if ((p[0] == '!') && (p[1] == '=') && (p[2] == 0)) {
	    value = (relation != 0);
	} else {
	    goto compareError;
	}
	Tcl_SetResult(interp, ((value) ? "1" : "0"), TCL_STATIC);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)
	    && (length >= 3)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, textPtr->tkwin, configSpecs,
		    (char *) textPtr, (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, textPtr->tkwin, configSpecs,
		    (char *) textPtr, argv[2], 0);
	} else {
	    result = ConfigureText(interp, textPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else if ((c == 'd') && (strncmp(argv[1], "debug", length) == 0)
	    && (length >= 3)) {
	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " debug boolean\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (argc == 2) {
	    Tcl_SetResult(interp, ((tkBTreeDebug) ? "1" : "0"), TCL_STATIC);
	} else {
	    if (Tcl_GetBoolean(interp, argv[2], &tkBTreeDebug) != TCL_OK) {
		result = TCL_ERROR;
		goto done;
	    }
	    tkTextDebug = tkBTreeDebug;
	}
    } else if ((c == 'd') && (strncmp(argv[1], "delete", length) == 0)
	    && (length >= 3)) {
	int i;

	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " delete index1 ?index2 ...?\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (textPtr->state == TK_STATE_NORMAL) {
	    if (argc < 5) {
		/*
		 * Simple case requires no predetermination of indices.
		 */
		result = DeleteChars(textPtr, argv[2],
			(argc == 4) ? argv[3] : NULL, NULL, NULL);
	    } else {
		/*
		 * Multi-index pair case requires that we prevalidate the
		 * indices and sort from last to first so that deletes
		 * occur in the exact (unshifted) text.  It also needs to
		 * handle partial and fully overlapping ranges.  We have to
		 * do this with multiple passes.
		 */
		TkTextIndex *indices, *ixStart, *ixEnd, *lastStart;
		char *useIdx;

		argc -= 2;
		argv += 2;
		indices = (TkTextIndex *)
		    ckalloc((argc + 1) * sizeof(TkTextIndex));

		/*
		 * First pass verifies that all indices are valid.
		 */
		for (i = 0; i < argc; i++) {
		    if (TkTextGetIndex(interp, textPtr, argv[i],
			    &indices[i]) != TCL_OK) {
			result = TCL_ERROR;
			ckfree((char *) indices);
			goto done;
		    }
		}
		/*
		 * Pad out the pairs evenly to make later code easier.
		 */
		if (argc & 1) {
		    indices[i] = indices[i-1];
		    TkTextIndexForwChars(&indices[i], 1, &indices[i]);
		    argc++;
		}
		useIdx = (char *) ckalloc((unsigned) argc);
		memset(useIdx, 0, (unsigned) argc);
		/*
		 * Do a decreasing order sort so that we delete the end
		 * ranges first to maintain index consistency.
		 */
		qsort((VOID *) indices, (unsigned) (argc / 2),
			2 * sizeof(TkTextIndex), TextIndexSortProc);
		lastStart = NULL;
		/*
		 * Second pass will handle bogus ranges (end < start) and
		 * overlapping ranges.
		 */
		for (i = 0; i < argc; i += 2) {
		    ixStart = &indices[i];
		    ixEnd   = &indices[i+1];
		    if (TkTextIndexCmp(ixEnd, ixStart) <= 0) {
			continue;
		    }
		    if (lastStart) {
			if (TkTextIndexCmp(ixStart, lastStart) == 0) {
			    /*
			     * Start indices were equal, and the sort placed
			     * the longest range first, so skip this one.
			     */
			    continue;
			} else if (TkTextIndexCmp(lastStart, ixEnd) < 0) {
			    /*
			     * The next pair has a start range before the end
			     * point of the last range.  Constrain the delete
			     * range, but use the pointer values.
			     */
			    *ixEnd = *lastStart;
			    if (TkTextIndexCmp(ixEnd, ixStart) <= 0) {
				continue;
			    }
			}
		    }
		    lastStart = ixStart;
		    useIdx[i]   = 1;
		}
		/*
		 * Final pass take the input from the previous and deletes
		 * the ranges which are flagged to be deleted.
		 */
		for (i = 0; i < argc; i += 2) {
		    if (useIdx[i]) {
			/*
			 * We don't need to check the return value because all
			 * indices are preparsed above.
			 */
			DeleteChars(textPtr, NULL, NULL,
				&indices[i], &indices[i+1]);
		    }
		}
		ckfree((char *) indices);
	    }
	}
    } else if ((c == 'd') && (strncmp(argv[1], "dlineinfo", length) == 0)
	    && (length >= 2)) {
	int x, y, width, height, base;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " dlineinfo index\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextGetIndex(interp, textPtr, argv[2], &index1) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextDLineInfo(textPtr, &index1, &x, &y, &width, &height, &base)
		== 0) {
	    char buf[TCL_INTEGER_SPACE * 5];
	    
	    sprintf(buf, "%d %d %d %d %d", x, y, width, height, base);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
    } else if ((c == 'e') && (strncmp(argv[1], "edit", length) == 0)) {
        result = TextEditCmd(textPtr, interp, argc, argv);
    } else if ((c == 'g') && (strncmp(argv[1], "get", length) == 0)) {
	Tcl_Obj *objPtr = NULL;
	Tcl_DString ds;
	int i, found = 0;

	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " get index1 ?index2 ...?\"", (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	for (i = 2; i < argc; i += 2) {
	    if (TkTextGetIndex(interp, textPtr, argv[i], &index1) != TCL_OK) {
		result = TCL_ERROR;
		goto done;
	    }
	    if (i+1 == argc) {
		index2 = index1;
		TkTextIndexForwChars(&index2, 1, &index2);
	    } else if (TkTextGetIndex(interp, textPtr, argv[i+1], &index2)
		    != TCL_OK) {
		if (objPtr) {
		    Tcl_DecrRefCount(objPtr);
		}
		result = TCL_ERROR;
		goto done;
	    }
	    if (TkTextIndexCmp(&index1, &index2) < 0) {
		/* 
		 * Place the text in a DString and move it to the result.
		 * Since this could in principle be a megabyte or more, we
		 * want to do it efficiently!
		 */
		TextGetText(&index1, &index2, &ds);
		found++;
		if (found == 1) {
		    Tcl_DStringResult(interp, &ds);
		} else {
		    if (found == 2) {
			/*
			 * Move the first item we put into the result into
			 * the first element of the list object.
			 */
			objPtr = Tcl_NewObj();
			Tcl_ListObjAppendElement(NULL, objPtr,
				Tcl_GetObjResult(interp));
		    }
		    Tcl_ListObjAppendElement(NULL, objPtr,
			    Tcl_NewStringObj(Tcl_DStringValue(&ds),
				    Tcl_DStringLength(&ds)));
		}
		Tcl_DStringFree(&ds);
	    }
	}
	if (found > 1) {
	    Tcl_SetObjResult(interp, objPtr);
	}
    } else if ((c == 'i') && (strncmp(argv[1], "index", length) == 0)
	    && (length >= 3)) {
	char buf[200];

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " index index\"",
		    (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextGetIndex(interp, textPtr, argv[2], &index1) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	TkTextPrintIndex(&index1, buf);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if ((c == 'i') && (strncmp(argv[1], "insert", length) == 0)
	    && (length >= 3)) {
	int i, j, numTags;
	CONST char **tagNames;
	TkTextTag **oldTagArrayPtr;

	if (argc < 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0],
		    " insert index chars ?tagList chars tagList ...?\"",
		    (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextGetIndex(interp, textPtr, argv[2], &index1) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (textPtr->state == TK_STATE_NORMAL) {
	    for (j = 3;  j < argc; j += 2) {
		InsertChars(textPtr, &index1, argv[j]);
		if (argc > (j+1)) {
		    TkTextIndexForwBytes(&index1, (int) strlen(argv[j]),
			    &index2);
		    oldTagArrayPtr = TkBTreeGetTags(&index1, &numTags);
		    if (oldTagArrayPtr != NULL) {
			for (i = 0; i < numTags; i++) {
			    TkBTreeTag(&index1, &index2, oldTagArrayPtr[i], 0);
			}
			ckfree((char *) oldTagArrayPtr);
		    }
		    if (Tcl_SplitList(interp, argv[j+1], &numTags, &tagNames)
			    != TCL_OK) {
			result = TCL_ERROR;
			goto done;
		    }
		    for (i = 0; i < numTags; i++) {
			TkBTreeTag(&index1, &index2,
				TkTextCreateTag(textPtr, tagNames[i]), 1);
		    }
		    ckfree((char *) tagNames);
		    index1 = index2;
		}
	    }
	}
    } else if ((c == 'd') && (strncmp(argv[1], "dump", length) == 0)) {
	result = TextDumpCmd(textPtr, interp, argc, argv);
    } else if ((c == 'i') && (strncmp(argv[1], "image", length) == 0)) {
	result = TkTextImageCmd(textPtr, interp, argc, argv);
    } else if ((c == 'm') && (strncmp(argv[1], "mark", length) == 0)) {
	result = TkTextMarkCmd(textPtr, interp, argc, argv);
    } else if ((c == 's') && (strcmp(argv[1], "scan") == 0) && (length >= 2)) {
	result = TkTextScanCmd(textPtr, interp, argc, argv);
    } else if ((c == 's') && (strcmp(argv[1], "search") == 0)
	    && (length >= 3)) {
	result = TextSearchCmd(textPtr, interp, argc, argv);
    } else if ((c == 's') && (strcmp(argv[1], "see") == 0) && (length >= 3)) {
	result = TkTextSeeCmd(textPtr, interp, argc, argv);
    } else if ((c == 't') && (strcmp(argv[1], "tag") == 0)) {
	result = TkTextTagCmd(textPtr, interp, argc, argv);
    } else if ((c == 'w') && (strncmp(argv[1], "window", length) == 0)) {
	result = TkTextWindowCmd(textPtr, interp, argc, argv);
    } else if ((c == 'x') && (strncmp(argv[1], "xview", length) == 0)) {
	result = TkTextXviewCmd(textPtr, interp, argc, argv);
    } else if ((c == 'y') && (strncmp(argv[1], "yview", length) == 0)
	    && (length >= 2)) {
	result = TkTextYviewCmd(textPtr, interp, argc, argv);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be bbox, cget, compare, configure, debug, delete, ",
                "dlineinfo, dump, edit, get, image, index, insert, mark, ",
                "scan, search, see, tag, window, xview, or yview",
		(char *) NULL);
	result = TCL_ERROR;
    }

    done:
    Tcl_Release((ClientData) textPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TextIndexSortProc --
 *
 *	This procedure is called by qsort when sorting an array of
 *	indices in *decreasing* order (last to first).
 *
 * Results:
 *	The return value is -1 if the first argument should be before
 *	the second element, 0 if it's equivalent, and 1 if it should be
 *	after the second element.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextIndexSortProc(first, second)
    CONST VOID *first, *second;		/* Elements to be compared. */
{
    TkTextIndex *pair1 = (TkTextIndex *) first;
    TkTextIndex *pair2 = (TkTextIndex *) second;
    int cmp = TkTextIndexCmp(&pair1[1], &pair2[1]);

    if (cmp == 0) {
	/*
	 * If the first indices were equal, we want the second index of the
	 * pair also to be the greater.  Use pointer magic to access the
	 * second index pair.
	 */
	cmp = TkTextIndexCmp(&pair1[0], &pair2[0]);
    }
    if (cmp > 0) {
	return -1;
    } else if (cmp < 0) {
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyText --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a text at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the text is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyText(memPtr)
    char *memPtr;		/* Info about text widget. */
{
    register TkText *textPtr = (TkText *) memPtr;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    TkTextTag *tagPtr;

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.  Special note:  free up display-related information
     * before deleting the B-tree, since display-related stuff
     * may refer to stuff in the B-tree.
     */

    TkTextFreeDInfo(textPtr);
    TkBTreeDestroy(textPtr->tree);
    for (hPtr = Tcl_FirstHashEntry(&textPtr->tagTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	tagPtr = (TkTextTag *) Tcl_GetHashValue(hPtr);
	TkTextFreeTag(textPtr, tagPtr);
    }
    Tcl_DeleteHashTable(&textPtr->tagTable);
    for (hPtr = Tcl_FirstHashEntry(&textPtr->markTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	ckfree((char *) Tcl_GetHashValue(hPtr));
    }
    Tcl_DeleteHashTable(&textPtr->markTable);
    if (textPtr->tabArrayPtr != NULL) {
	ckfree((char *) textPtr->tabArrayPtr);
    }
    if (textPtr->insertBlinkHandler != NULL) {
	Tcl_DeleteTimerHandler(textPtr->insertBlinkHandler);
    }
    if (textPtr->bindingTable != NULL) {
	Tk_DeleteBindingTable(textPtr->bindingTable);
    }
    TkUndoFreeStack(textPtr->undoStack);

    /*
     * NOTE: do NOT free up selBorder, selBdString, or selFgColorPtr:
     * they are duplicates of information in the "sel" tag, which was
     * freed up as part of deleting the tags above.
     */

    textPtr->selBorder = NULL;
    textPtr->selBdString = NULL;
    textPtr->selFgColorPtr = NULL;
    Tk_FreeOptions(configSpecs, (char *) textPtr, textPtr->display, 0);
    ckfree((char *) textPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureText --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a text widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for textPtr;  old resources get freed, if there
 *	were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureText(interp, textPtr, argc, argv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register TkText *textPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int argc;			/* Number of valid entries in argv. */
    CONST char **argv;		/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    int oldExport = textPtr->exportSelection;

    if (Tk_ConfigureWidget(interp, textPtr->tkwin, configSpecs,
	    argc, argv, (char *) textPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    TkUndoSetDepth(textPtr->undoStack, textPtr->maxUndo);

    /*
     * A few other options also need special processing, such as parsing
     * the geometry and setting the background from a 3-D border.
     */

    Tk_SetBackgroundFromBorder(textPtr->tkwin, textPtr->border);

    /*
     * Don't allow negative spacings.
     */

    if (textPtr->spacing1 < 0) {
	textPtr->spacing1 = 0;
    }
    if (textPtr->spacing2 < 0) {
	textPtr->spacing2 = 0;
    }
    if (textPtr->spacing3 < 0) {
	textPtr->spacing3 = 0;
    }

    /*
     * Parse tab stops.
     */

    if (textPtr->tabArrayPtr != NULL) {
	ckfree((char *) textPtr->tabArrayPtr);
	textPtr->tabArrayPtr = NULL;
    }
    if (textPtr->tabOptionString != NULL) {
	textPtr->tabArrayPtr = TkTextGetTabs(interp, textPtr->tkwin,
		textPtr->tabOptionString);
	if (textPtr->tabArrayPtr == NULL) {
	    Tcl_AddErrorInfo(interp,"\n    (while processing -tabs option)");
	    return TCL_ERROR;
	}
    }

    /*
     * Make sure that configuration options are properly mirrored
     * between the widget record and the "sel" tags.  NOTE: we don't
     * have to free up information during the mirroring;  old
     * information was freed when it was replaced in the widget
     * record.
     */

    textPtr->selTagPtr->border = textPtr->selBorder;
    if (textPtr->selTagPtr->bdString != textPtr->selBdString) {
	textPtr->selTagPtr->bdString = textPtr->selBdString;
	if (textPtr->selBdString != NULL) {
	    if (Tk_GetPixels(interp, textPtr->tkwin, textPtr->selBdString,
		    &textPtr->selTagPtr->borderWidth) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (textPtr->selTagPtr->borderWidth < 0) {
		textPtr->selTagPtr->borderWidth = 0;
	    }
	}
    }
    textPtr->selTagPtr->fgColor = textPtr->selFgColorPtr;
    textPtr->selTagPtr->affectsDisplay = 0;
    if ((textPtr->selTagPtr->border != NULL)
	    || (textPtr->selTagPtr->bdString != NULL)
	    || (textPtr->selTagPtr->reliefString != NULL)
	    || (textPtr->selTagPtr->bgStipple != None)
	    || (textPtr->selTagPtr->fgColor != NULL)
	    || (textPtr->selTagPtr->tkfont != None)
	    || (textPtr->selTagPtr->fgStipple != None)
	    || (textPtr->selTagPtr->justifyString != NULL)
	    || (textPtr->selTagPtr->lMargin1String != NULL)
	    || (textPtr->selTagPtr->lMargin2String != NULL)
	    || (textPtr->selTagPtr->offsetString != NULL)
	    || (textPtr->selTagPtr->overstrikeString != NULL)
	    || (textPtr->selTagPtr->rMarginString != NULL)
	    || (textPtr->selTagPtr->spacing1String != NULL)
	    || (textPtr->selTagPtr->spacing2String != NULL)
	    || (textPtr->selTagPtr->spacing3String != NULL)
	    || (textPtr->selTagPtr->tabString != NULL)
	    || (textPtr->selTagPtr->underlineString != NULL)
	    || (textPtr->selTagPtr->elideString != NULL)
	    || (textPtr->selTagPtr->wrapMode != TEXT_WRAPMODE_NULL)) {
	textPtr->selTagPtr->affectsDisplay = 1;
    }
    TkTextRedrawTag(textPtr, (TkTextIndex *) NULL, (TkTextIndex *) NULL,
	    textPtr->selTagPtr, 1);

    /*
     * Claim the selection if we've suddenly started exporting it and there
     * are tagged characters.
     */

    if (textPtr->exportSelection && (!oldExport)) {
	TkTextSearch search;
	TkTextIndex first, last;

	TkTextMakeByteIndex(textPtr->tree, 0, 0, &first);
	TkTextMakeByteIndex(textPtr->tree,
		TkBTreeNumLines(textPtr->tree), 0, &last);
	TkBTreeStartSearch(&first, &last, textPtr->selTagPtr, &search);
	if (TkBTreeCharTagged(&first, textPtr->selTagPtr)
		|| TkBTreeNextTag(&search)) {
	    Tk_OwnSelection(textPtr->tkwin, XA_PRIMARY, TkTextLostSelection,
		    (ClientData) textPtr);
	    textPtr->flags |= GOT_SELECTION;
	}
    }

    /*
     * Account for state changes that would reenable blinking cursor state.
     */

    if (textPtr->flags & GOT_FOCUS) {
	Tcl_DeleteTimerHandler(textPtr->insertBlinkHandler);
	textPtr->insertBlinkHandler = (Tcl_TimerToken) NULL;
	TextBlinkProc((ClientData) textPtr);
    }

    /*
     * Register the desired geometry for the window, and arrange for
     * the window to be redisplayed.
     */

    if (textPtr->width <= 0) {
	textPtr->width = 1;
    }
    if (textPtr->height <= 0) {
	textPtr->height = 1;
    }
    TextWorldChanged((ClientData) textPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextWorldChanged --
 *
 *      This procedure is called when the world has changed in some
 *      way and the widget needs to recompute all its graphics contexts
 *	and determine its new geometry.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Configures all tags in the Text with a empty argc/argv, for
 *	the side effect of causing all the items to recompute their
 *	geometry and to be redisplayed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
TextWorldChanged(instanceData)
    ClientData instanceData;	/* Information about widget. */
{
    TkText *textPtr;
    Tk_FontMetrics fm;

    textPtr = (TkText *) instanceData;

    textPtr->charWidth = Tk_TextWidth(textPtr->tkfont, "0", 1);
    if (textPtr->charWidth <= 0) {
	textPtr->charWidth = 1;
    }
    Tk_GetFontMetrics(textPtr->tkfont, &fm);
    Tk_GeometryRequest(textPtr->tkwin,
	    textPtr->width * textPtr->charWidth + 2*textPtr->borderWidth
		    + 2*textPtr->padX + 2*textPtr->highlightWidth,
	    textPtr->height * (fm.linespace + textPtr->spacing1
		    + textPtr->spacing3) + 2*textPtr->borderWidth
		    + 2*textPtr->padY + 2*textPtr->highlightWidth);
    Tk_SetInternalBorder(textPtr->tkwin,
	    textPtr->borderWidth + textPtr->highlightWidth);
    if (textPtr->setGrid) {
	Tk_SetGrid(textPtr->tkwin, textPtr->width, textPtr->height,
		textPtr->charWidth, fm.linespace);
    } else {
	Tk_UnsetGrid(textPtr->tkwin);
    }

    TkTextRelayoutWindow(textPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TextEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher on
 *	structure changes to a text.  For texts with 3D
 *	borders, this procedure is also invoked for exposures.
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

static void
TextEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    register XEvent *eventPtr;	/* Information about event. */
{
    register TkText *textPtr = (TkText *) clientData;
    TkTextIndex index, index2;

    if (eventPtr->type == Expose) {
	TkTextRedrawRegion(textPtr, eventPtr->xexpose.x,
		eventPtr->xexpose.y, eventPtr->xexpose.width,
		eventPtr->xexpose.height);
    } else if (eventPtr->type == ConfigureNotify) {
	if ((textPtr->prevWidth != Tk_Width(textPtr->tkwin))
		|| (textPtr->prevHeight != Tk_Height(textPtr->tkwin))) {
	    TkTextRelayoutWindow(textPtr);
	    textPtr->prevWidth = Tk_Width(textPtr->tkwin);
	    textPtr->prevHeight = Tk_Height(textPtr->tkwin);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (textPtr->tkwin != NULL) {
	    if (textPtr->setGrid) {
		Tk_UnsetGrid(textPtr->tkwin);
	    }
	    textPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(textPtr->interp,
		    textPtr->widgetCmd);
	}
	Tcl_EventuallyFree((ClientData) textPtr, DestroyText);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    Tcl_DeleteTimerHandler(textPtr->insertBlinkHandler);
	    if (eventPtr->type == FocusIn) {
		textPtr->flags |= GOT_FOCUS | INSERT_ON;
		if (textPtr->insertOffTime != 0) {
		    textPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
			    textPtr->insertOnTime, TextBlinkProc,
			    (ClientData) textPtr);
		}
	    } else {
		textPtr->flags &= ~(GOT_FOCUS | INSERT_ON);
		textPtr->insertBlinkHandler = (Tcl_TimerToken) NULL;
	    }
#ifndef ALWAYS_SHOW_SELECTION
	    TkTextRedrawTag(textPtr, NULL, NULL, textPtr->selTagPtr, 1);
#endif
	    TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
	    TkTextIndexForwChars(&index, 1, &index2);
	    TkTextChanged(textPtr, &index, &index2);
	    if (textPtr->highlightWidth > 0) {
		TkTextRedrawRegion(textPtr, 0, 0, textPtr->highlightWidth,
			textPtr->highlightWidth);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextCmdDeletedProc --
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
TextCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TkText *textPtr = (TkText *) clientData;
    Tk_Window tkwin = textPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	if (textPtr->setGrid) {
	    Tk_UnsetGrid(textPtr->tkwin);
	}
	textPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InsertChars --
 *
 *	This procedure implements most of the functionality of the
 *	"insert" widget command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The characters in "string" get added to the text just before
 *	the character indicated by "indexPtr".
 *
 *----------------------------------------------------------------------
 */

static void
InsertChars(textPtr, indexPtr, string)
    TkText *textPtr;		/* Overall information about text widget. */
    TkTextIndex *indexPtr;	/* Where to insert new characters.  May be
				 * modified and/or invalidated. */
    CONST char *string;		/* Null-terminated string containing new
				 * information to add to text. */
{
    int lineIndex, resetView, offset;
    TkTextIndex newTop;
    char indexBuffer[TK_POS_CHARS];

    /*
     * Don't allow insertions on the last (dummy) line of the text.
     */

    lineIndex = TkBTreeLineIndex(indexPtr->linePtr);
    if (lineIndex == TkBTreeNumLines(textPtr->tree)) {
	lineIndex--;
	TkTextMakeByteIndex(textPtr->tree, lineIndex, 1000000, indexPtr);
    }

    /*
     * Notify the display module that lines are about to change, then do
     * the insertion.  If the insertion occurs on the top line of the
     * widget (textPtr->topIndex), then we have to recompute topIndex
     * after the insertion, since the insertion could invalidate it.
     */

    resetView = offset = 0;
    if (indexPtr->linePtr == textPtr->topIndex.linePtr) {
	resetView = 1;
	offset = textPtr->topIndex.byteIndex;
	if (offset > indexPtr->byteIndex) {
	    offset += strlen(string);
	}
    }
    TkTextChanged(textPtr, indexPtr, indexPtr);
    TkBTreeInsertChars(indexPtr, string);

    /*
     * Push the insertion on the undo stack
     */

    if ( textPtr->undo ) {
        TkTextIndex     toIndex;

        Tcl_DString actionCommand;
        Tcl_DString revertCommand;
        
        if (textPtr->autoSeparators &&
            textPtr->lastEditMode != TK_TEXT_EDIT_INSERT) {
            TkUndoInsertUndoSeparator(textPtr->undoStack);
        }
        
        textPtr->lastEditMode = TK_TEXT_EDIT_INSERT;
        
        Tcl_DStringInit(&actionCommand);
        Tcl_DStringInit(&revertCommand);
        
        Tcl_DStringAppend(&actionCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&actionCommand," insert ",-1);
        TkTextPrintIndex(indexPtr,indexBuffer);
        Tcl_DStringAppend(&actionCommand,indexBuffer,-1);
        Tcl_DStringAppend(&actionCommand," ",-1);
        Tcl_DStringAppendElement(&actionCommand,string);
        Tcl_DStringAppend(&actionCommand,";",-1);
        Tcl_DStringAppend(&actionCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&actionCommand," mark set insert ",-1);
        TkTextIndexForwBytes(indexPtr, (int) strlen(string),
			&toIndex);
        TkTextPrintIndex(&toIndex, indexBuffer);
        Tcl_DStringAppend(&actionCommand,indexBuffer,-1);
        Tcl_DStringAppend(&actionCommand,"; ",-1);
        Tcl_DStringAppend(&actionCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&actionCommand," see insert",-1);
        
        Tcl_DStringAppend(&revertCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&revertCommand," delete ",-1);
        TkTextPrintIndex(indexPtr,indexBuffer);
        Tcl_DStringAppend(&revertCommand,indexBuffer,-1);
        Tcl_DStringAppend(&revertCommand," ",-1);
        TkTextPrintIndex(&toIndex, indexBuffer);
        Tcl_DStringAppend(&revertCommand,indexBuffer,-1);
        Tcl_DStringAppend(&revertCommand," ;",-1);
        Tcl_DStringAppend(&revertCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&revertCommand," mark set insert ",-1);
        TkTextPrintIndex(indexPtr,indexBuffer);
        Tcl_DStringAppend(&revertCommand,indexBuffer,-1);
        Tcl_DStringAppend(&revertCommand,"; ",-1);
        Tcl_DStringAppend(&revertCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&revertCommand," see insert",-1);
        
        TkUndoPushAction(textPtr->undoStack,&actionCommand, &revertCommand);

     	Tcl_DStringFree(&actionCommand);
     	Tcl_DStringFree(&revertCommand);

    }
    updateDirtyFlag(textPtr);

    if (resetView) {
	TkTextMakeByteIndex(textPtr->tree, lineIndex, 0, &newTop);
	TkTextIndexForwBytes(&newTop, offset, &newTop);
	TkTextSetYView(textPtr, &newTop, 0);
    }

    /*
     * Invalidate any selection retrievals in progress.
     */

    textPtr->abortSelections = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteChars --
 *
 *	This procedure implements most of the functionality of the
 *	"delete" widget command.
 *
 * Results:
 *	Returns a standard Tcl result, and leaves an error message
 *	in textPtr->interp if there is an error.
 *
 * Side effects:
 *	Characters get deleted from the text.
 *
 *----------------------------------------------------------------------
 */

static int
DeleteChars(textPtr, index1String, index2String, indexPtr1, indexPtr2)
    TkText *textPtr;		/* Overall information about text widget. */
    CONST char *index1String;	/* String describing location of first
				 * character to delete. */
    CONST char *index2String;	/* String describing location of last
				 * character to delete.  NULL means just
				 * delete the one character given by
				 * index1String. */
    TkTextIndex *indexPtr1;	/* index describing location of first
				 * character to delete. */
    TkTextIndex *indexPtr2;	/* index describing location of last
				 * character to delete.  NULL means just
				 * delete the one character given by
				 * indexPtr1. */
{
    int line1, line2, line, byteIndex, resetView;
    TkTextIndex index1, index2;
    char indexBuffer[TK_POS_CHARS];

    /*
     * Parse the starting and stopping indices.
     */

    if (index1String != NULL) {
	if (TkTextGetIndex(textPtr->interp, textPtr, index1String, &index1)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if (index2String != NULL) {
	    if (TkTextGetIndex(textPtr->interp, textPtr, index2String, &index2)
		    != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    index2 = index1;
	    TkTextIndexForwChars(&index2, 1, &index2);
	}
    } else {
	index1 = *indexPtr1;
	if (indexPtr2 != NULL) {
	    index2 = *indexPtr2;
	} else {
	    index2 = index1;
	    TkTextIndexForwChars(&index2, 1, &index2);
	}
    }

    /*
     * Make sure there's really something to delete.
     */

    if (TkTextIndexCmp(&index1, &index2) >= 0) {
	return TCL_OK;
    }

    /*
     * The code below is ugly, but it's needed to make sure there
     * is always a dummy empty line at the end of the text.  If the
     * final newline of the file (just before the dummy line) is being
     * deleted, then back up index to just before the newline.  If
     * there is a newline just before the first character being deleted,
     * then back up the first index too, so that an even number of lines
     * gets deleted.  Furthermore, remove any tags that are present on
     * the newline that isn't going to be deleted after all (this simulates
     * deleting the newline and then adding a "clean" one back again).
     */

    line1 = TkBTreeLineIndex(index1.linePtr);
    line2 = TkBTreeLineIndex(index2.linePtr);
    if (line2 == TkBTreeNumLines(textPtr->tree)) {
	TkTextTag **arrayPtr;
	int arraySize, i;
	TkTextIndex oldIndex2;

	oldIndex2 = index2;
	TkTextIndexBackChars(&oldIndex2, 1, &index2);
	line2--;
	if ((index1.byteIndex == 0) && (line1 != 0)) {
	    TkTextIndexBackChars(&index1, 1, &index1);
	    line1--;
	}
	arrayPtr = TkBTreeGetTags(&index2, &arraySize);
	if (arrayPtr != NULL) {
	    for (i = 0; i < arraySize; i++) {
		TkBTreeTag(&index2, &oldIndex2, arrayPtr[i], 0);
	    }
	    ckfree((char *) arrayPtr);
	}
    }

    /*
     * Tell the display what's about to happen so it can discard
     * obsolete display information, then do the deletion.  Also,
     * if the deletion involves the top line on the screen, then
     * we have to reset the view (the deletion will invalidate
     * textPtr->topIndex).  Compute what the new first character
     * will be, then do the deletion, then reset the view.
     */

    TkTextChanged(textPtr, &index1, &index2);
    resetView = 0;
    line = 0;
    byteIndex = 0;
    if (TkTextIndexCmp(&index2, &textPtr->topIndex) >= 0) {
	if (TkTextIndexCmp(&index1, &textPtr->topIndex) <= 0) {
	    /*
	     * Deletion range straddles topIndex: use the beginning
	     * of the range as the new topIndex.
	     */

	    resetView = 1;
	    line = line1;
	    byteIndex = index1.byteIndex;
	} else if (index1.linePtr == textPtr->topIndex.linePtr) {
	    /*
	     * Deletion range starts on top line but after topIndex.
	     * Use the current topIndex as the new one.
	     */

	    resetView = 1;
	    line = line1;
	    byteIndex = textPtr->topIndex.byteIndex;
	}
    } else if (index2.linePtr == textPtr->topIndex.linePtr) {
	/*
	 * Deletion range ends on top line but before topIndex.
	 * Figure out what will be the new character index for
	 * the character currently pointed to by topIndex.
	 */

	resetView = 1;
	line = line2;
	byteIndex = textPtr->topIndex.byteIndex;
	if (index1.linePtr != index2.linePtr) {
	    byteIndex -= index2.byteIndex;
	} else {
	    byteIndex -= (index2.byteIndex - index1.byteIndex);
	}
    }

    /*
     * Push the deletion on the undo stack
     */

    if (textPtr->undo) {
	Tcl_DString ds;
        Tcl_DString actionCommand;
        Tcl_DString revertCommand;
    
	if (textPtr->autoSeparators
		&& (textPtr->lastEditMode != TK_TEXT_EDIT_DELETE)) {
	   TkUndoInsertUndoSeparator(textPtr->undoStack);
	}

	textPtr->lastEditMode = TK_TEXT_EDIT_DELETE;

        Tcl_DStringInit(&actionCommand);
        Tcl_DStringInit(&revertCommand);

        Tcl_DStringAppend(&actionCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&actionCommand," delete ",-1);
        TkTextPrintIndex(&index1,indexBuffer);
        Tcl_DStringAppend(&actionCommand,indexBuffer,-1);
        Tcl_DStringAppend(&actionCommand," ",-1);
        TkTextPrintIndex(&index2, indexBuffer);
        Tcl_DStringAppend(&actionCommand,indexBuffer,-1);
        Tcl_DStringAppend(&actionCommand,"; ",-1);
        Tcl_DStringAppend(&actionCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&actionCommand," mark set insert ",-1);
        TkTextPrintIndex(&index1,indexBuffer);
        Tcl_DStringAppend(&actionCommand,indexBuffer,-1);

        Tcl_DStringAppend(&actionCommand,"; ",-1);
        Tcl_DStringAppend(&actionCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&actionCommand," see insert",-1);

	TextGetText(&index1, &index2, &ds);

        Tcl_DStringAppend(&revertCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&revertCommand," insert ",-1);
        TkTextPrintIndex(&index1,indexBuffer);
        Tcl_DStringAppend(&revertCommand,indexBuffer,-1);
        Tcl_DStringAppend(&revertCommand," ",-1);
        Tcl_DStringAppendElement(&revertCommand,Tcl_DStringValue(&ds));
        Tcl_DStringAppend(&revertCommand,"; ",-1);
        Tcl_DStringAppend(&revertCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&revertCommand," mark set insert ",-1);
        TkTextPrintIndex(&index2, indexBuffer);
        Tcl_DStringAppend(&revertCommand,indexBuffer,-1);
        Tcl_DStringAppend(&revertCommand,"; ",-1);
        Tcl_DStringAppend(&revertCommand,Tcl_GetCommandName(textPtr->interp,textPtr->widgetCmd),-1);
        Tcl_DStringAppend(&revertCommand," see insert",-1);

        TkUndoPushAction(textPtr->undoStack,&actionCommand, &revertCommand);

        Tcl_DStringFree(&actionCommand);
        Tcl_DStringFree(&revertCommand);

    }
    updateDirtyFlag(textPtr);

    TkBTreeDeleteChars(&index1, &index2);
    if (resetView) {
	TkTextMakeByteIndex(textPtr->tree, line, byteIndex, &index1);
	TkTextSetYView(textPtr, &index1, 0);
    }

    /*
     * Invalidate any selection retrievals in progress.
     */

    textPtr->abortSelections = 1;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TextFetchSelection --
 *
 *	This procedure is called back by Tk when the selection is
 *	requested by someone.  It returns part or all of the selection
 *	in a buffer provided by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored
 *	at buffer.  Buffer is filled (or partially filled) with a
 *	NULL-terminated string containing part or all of the selection,
 *	as given by offset and maxBytes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextFetchSelection(clientData, offset, buffer, maxBytes)
    ClientData clientData;		/* Information about text widget. */
    int offset;				/* Offset within selection of first
					 * character to be returned. */
    char *buffer;			/* Location in which to place
					 * selection. */
    int maxBytes;			/* Maximum number of bytes to place
					 * at buffer, not including terminating
					 * NULL character. */
{
    register TkText *textPtr = (TkText *) clientData;
    TkTextIndex eof;
    int count, chunkSize, offsetInSeg;
    TkTextSearch search;
    TkTextSegment *segPtr;

    if (!textPtr->exportSelection) {
	return -1;
    }

    /*
     * Find the beginning of the next range of selected text.  Note:  if
     * the selection is being retrieved in multiple pieces (offset != 0)
     * and some modification has been made to the text that affects the
     * selection then reject the selection request (make 'em start over
     * again).
     */

    if (offset == 0) {
	TkTextMakeByteIndex(textPtr->tree, 0, 0, &textPtr->selIndex);
	textPtr->abortSelections = 0;
    } else if (textPtr->abortSelections) {
	return 0;
    }
    TkTextMakeByteIndex(textPtr->tree, TkBTreeNumLines(textPtr->tree), 0, &eof);
    TkBTreeStartSearch(&textPtr->selIndex, &eof, textPtr->selTagPtr, &search);
    if (!TkBTreeCharTagged(&textPtr->selIndex, textPtr->selTagPtr)) {
	if (!TkBTreeNextTag(&search)) {
	    if (offset == 0) {
		return -1;
	    } else {
		return 0;
	    }
	}
	textPtr->selIndex = search.curIndex;
    }

    /*
     * Each iteration through the outer loop below scans one selected range.
     * Each iteration through the inner loop scans one segment in the
     * selected range.
     */

    count = 0;
    while (1) {
	/*
	 * Find the end of the current range of selected text.
	 */

	if (!TkBTreeNextTag(&search)) {
	    panic("TextFetchSelection couldn't find end of range");
	}

	/*
	 * Copy information from character segments into the buffer
	 * until either we run out of space in the buffer or we get
	 * to the end of this range of text.
	 */

	while (1) {
	    if (maxBytes == 0) {
		goto done;
	    }
	    segPtr = TkTextIndexToSeg(&textPtr->selIndex, &offsetInSeg);
	    chunkSize = segPtr->size - offsetInSeg;
	    if (chunkSize > maxBytes) {
		chunkSize = maxBytes;
	    }
	    if (textPtr->selIndex.linePtr == search.curIndex.linePtr) {
		int leftInRange;

		leftInRange = search.curIndex.byteIndex
			- textPtr->selIndex.byteIndex;
		if (leftInRange < chunkSize) {
		    chunkSize = leftInRange;
		    if (chunkSize <= 0) {
			break;
		    }
		}
	    }
	    if ((segPtr->typePtr == &tkTextCharType)
		    && !TkTextIsElided(textPtr, &textPtr->selIndex)) {
		memcpy((VOID *) buffer, (VOID *) (segPtr->body.chars
			+ offsetInSeg), (size_t) chunkSize);
		buffer += chunkSize;
		maxBytes -= chunkSize;
		count += chunkSize;
	    }
	    TkTextIndexForwBytes(&textPtr->selIndex, chunkSize,
		    &textPtr->selIndex);
	}

	/*
	 * Find the beginning of the next range of selected text.
	 */

	if (!TkBTreeNextTag(&search)) {
	    break;
	}
	textPtr->selIndex = search.curIndex;
    }

    done:
    *buffer = 0;
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextLostSelection --
 *
 *	This procedure is called back by Tk when the selection is
 *	grabbed away from a text widget.  On Windows and Mac systems, we
 *	want to remember the selection for the next time the focus
 *	enters the window.  On Unix, just remove the "sel" tag from
 *	everything in the widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The "sel" tag is cleared from the window.
 *
 *----------------------------------------------------------------------
 */

void
TkTextLostSelection(clientData)
    ClientData clientData;		/* Information about text widget. */
{
    register TkText *textPtr = (TkText *) clientData;
    XEvent event;
#ifdef ALWAYS_SHOW_SELECTION
    TkTextIndex start, end;

    if (!textPtr->exportSelection) {
	return;
    }

    /*
     * On Windows and Mac systems, we want to remember the selection
     * for the next time the focus enters the window.  On Unix, 
     * just remove the "sel" tag from everything in the widget.
     */

    TkTextMakeByteIndex(textPtr->tree, 0, 0, &start);
    TkTextMakeByteIndex(textPtr->tree, TkBTreeNumLines(textPtr->tree), 0, &end);
    TkTextRedrawTag(textPtr, &start, &end, textPtr->selTagPtr, 1);
    TkBTreeTag(&start, &end, textPtr->selTagPtr, 0);
#endif

    /*
     * Send an event that the selection changed.  This is equivalent to
     * "event generate $textWidget <<Selection>>"
     */

    memset((VOID *) &event, 0, sizeof(event));
    event.xany.type = VirtualEvent;
    event.xany.serial = NextRequest(Tk_Display(textPtr->tkwin));
    event.xany.send_event = False;
    event.xany.window = Tk_WindowId(textPtr->tkwin);
    event.xany.display = Tk_Display(textPtr->tkwin);
    ((XVirtualEvent *) &event)->name = Tk_GetUid("Selection");
    Tk_HandleEvent(&event);

    textPtr->flags &= ~GOT_SELECTION;
}

/*
 *----------------------------------------------------------------------
 *
 * TextBlinkProc --
 *
 *	This procedure is called as a timer handler to blink the
 *	insertion cursor off and on.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off, redisplay gets invoked,
 *	and this procedure reschedules itself.
 *
 *----------------------------------------------------------------------
 */

static void
TextBlinkProc(clientData)
    ClientData clientData;	/* Pointer to record describing text. */
{
    register TkText *textPtr = (TkText *) clientData;
    TkTextIndex index;
    int x, y, w, h;

    if ((textPtr->state == TK_STATE_DISABLED) ||
	    !(textPtr->flags & GOT_FOCUS) || (textPtr->insertOffTime == 0)) {
	return;
    }
    if (textPtr->flags & INSERT_ON) {
	textPtr->flags &= ~INSERT_ON;
	textPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		textPtr->insertOffTime, TextBlinkProc, (ClientData) textPtr);
    } else {
	textPtr->flags |= INSERT_ON;
	textPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		textPtr->insertOnTime, TextBlinkProc, (ClientData) textPtr);
    }
    TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
    if (TkTextCharBbox(textPtr, &index, &x, &y, &w, &h) == 0) {
	TkTextRedrawRegion(textPtr, x - textPtr->insertWidth / 2, y,
		textPtr->insertWidth, h);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextSearchCmd --
 *
 *	This procedure is invoked to process the "search" widget command
 *	for text widgets.  See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
TextSearchCmd(textPtr, interp, argc, argv)
    TkText *textPtr;		/* Information about text widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST char **argv;		/* Argument strings. */
{
    int backwards, exact, searchElide, c, i, argsLeft, noCase, leftToScan;
    size_t length;
    int numLines, startingLine, startingByte, lineNum, firstByte, lastByte;
    int code, matchLength, matchByte, passes, stopLine, searchWholeText;
    int patLength;
    CONST char *arg, *pattern, *varName, *p, *startOfLine;
    char buffer[20];
    TkTextIndex index, stopIndex;
    Tcl_DString line, patDString;
    TkTextSegment *segPtr;
    TkTextLine *linePtr;
    TkTextIndex curIndex;
    Tcl_Obj *patObj = NULL;
    Tcl_RegExp regexp = NULL;		/* Initialization needed only to
					 * prevent compiler warning. */

    /*
     * Parse switches and other arguments.
     */

    exact = 1;
    searchElide = 0;
    curIndex.tree = textPtr->tree;
    backwards = 0;
    noCase = 0;
    varName = NULL;
    for (i = 2; i < argc; i++) {
	arg = argv[i];
	if (arg[0] != '-') {
	    break;
	}
	length = strlen(arg);
	if (length < 2) {
	    badSwitch:
	    Tcl_AppendResult(interp, "bad switch \"", arg,
		    "\": must be --, -backward, -count, -elide, -exact, ",
		    "-forward, -nocase, or -regexp", (char *) NULL);
	    return TCL_ERROR;
	}
	c = arg[1];
	if ((c == 'b') && (strncmp(argv[i], "-backwards", length) == 0)) {
	    backwards = 1;
	} else if ((c == 'c') && (strncmp(argv[i], "-count", length) == 0)) {
	    if (i >= (argc-1)) {
		Tcl_SetResult(interp, "no value given for \"-count\" option",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	    i++;
	    varName = argv[i];
	} else if ((c == 'e') && (length > 2)
		&& (strncmp(argv[i], "-exact", length) == 0)) {
	    exact = 1;
	} else if ((c == 'e') && (length > 2)
		&& (strncmp(argv[i], "-elide", length) == 0)) {
	    searchElide = 1;
	} else if ((c == 'h') && (strncmp(argv[i], "-hidden", length) == 0)) {
	    /*
	     * -hidden is kept around for backwards compatibility with
	     * the dash patch, but -elide is the official option
	     */
	    searchElide = 1;
	} else if ((c == 'f') && (strncmp(argv[i], "-forwards", length) == 0)) {
	    backwards = 0;
	} else if ((c == 'n') && (strncmp(argv[i], "-nocase", length) == 0)) {
	    noCase = 1;
	} else if ((c == 'r') && (strncmp(argv[i], "-regexp", length) == 0)) {
	    exact = 0;
	} else if ((c == '-') && (strncmp(argv[i], "--", length) == 0)) {
	    i++;
	    break;
	} else {
	    goto badSwitch;
	}
    }
    argsLeft = argc - (i+2);
    if ((argsLeft != 0) && (argsLeft != 1)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " search ?switches? pattern index ?stopIndex?\"",
		(char *) NULL);
	return TCL_ERROR;
    }
    pattern = argv[i];

    /*
     * Convert the pattern to lower-case if we're supposed to ignore case.
     */

    if (noCase && exact) {
	Tcl_DStringInit(&patDString);
	Tcl_DStringAppend(&patDString, pattern, -1);
	Tcl_UtfToLower(Tcl_DStringValue(&patDString));
	pattern = Tcl_DStringValue(&patDString);
    }

    Tcl_DStringInit(&line);
    if (TkTextGetIndex(interp, textPtr, argv[i+1], &index) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }
    numLines = TkBTreeNumLines(textPtr->tree);
    startingLine = TkBTreeLineIndex(index.linePtr);
    startingByte = index.byteIndex;
    if (startingLine >= numLines) {
	if (backwards) {
	    startingLine = TkBTreeNumLines(textPtr->tree) - 1;
	    startingByte = TkBTreeBytesInLine(TkBTreeFindLine(textPtr->tree,
		    startingLine));
	} else {
	    startingLine = 0;
	    startingByte = 0;
	}
    }
    if (argsLeft == 1) {
	if (TkTextGetIndex(interp, textPtr, argv[i+2], &stopIndex) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	stopLine = TkBTreeLineIndex(stopIndex.linePtr);
	if (!backwards && (stopLine == numLines)) {
	    stopLine = numLines-1;
	}
	searchWholeText = 0;
    } else {
	stopLine = 0;
	searchWholeText = 1;
    }

    /*
     * Scan through all of the lines of the text circularly, starting
     * at the given index.
     */

    matchLength = patLength = 0;	/* Only needed to prevent compiler
					 * warnings. */
    if (exact) {
	patLength = strlen(pattern);
    } else {
	patObj = Tcl_NewStringObj(pattern, -1);
	Tcl_IncrRefCount(patObj);
	regexp = Tcl_GetRegExpFromObj(interp, patObj,
		(noCase ? TCL_REG_NOCASE : 0) | TCL_REG_ADVANCED);
	if (regexp == NULL) {
	    code = TCL_ERROR;
	    goto done;
	}
    }
    lineNum = startingLine;
    code = TCL_OK;
    for (passes = 0; passes < 2; ) {
	if (lineNum >= numLines) {
	    /*
	     * Don't search the dummy last line of the text.
	     */

	    goto nextLine;
	}

	/*
	 * Extract the text from the line.  If we're doing regular
	 * expression matching, drop the newline from the line, so
	 * that "$" can be used to match the end of the line.
	 */

	linePtr = TkBTreeFindLine(textPtr->tree, lineNum);
	curIndex.linePtr = linePtr; curIndex.byteIndex = 0;
	for (segPtr = linePtr->segPtr; segPtr != NULL;
		curIndex.byteIndex += segPtr->size, segPtr = segPtr->nextPtr) {
	    if ((segPtr->typePtr != &tkTextCharType)
		    || (!searchElide && TkTextIsElided(textPtr, &curIndex))) {
		continue;
	    }
	    Tcl_DStringAppend(&line, segPtr->body.chars, segPtr->size);
	}
	if (!exact) {
	    Tcl_DStringSetLength(&line, Tcl_DStringLength(&line)-1);
	}
	startOfLine = Tcl_DStringValue(&line);

	/*
	 * If we're ignoring case, convert the line to lower case.
	 */

	if (noCase) {
	    Tcl_DStringSetLength(&line,
		    Tcl_UtfToLower(Tcl_DStringValue(&line)));
	}

	/*
	 * Check for matches within the current line.  If so, and if we're
	 * searching backwards, repeat the search to find the last match
	 * in the line.  (Note: The lastByte should include the NULL char
	 * so we can handle searching for end of line easier.)
	 */

	matchByte = -1;
	firstByte = 0;
	lastByte = Tcl_DStringLength(&line) + 1;
	if (lineNum == startingLine) {
	    int indexInDString;

	    /*
	     * The starting line is tricky: the first time we see it
	     * we check one part of the line, and the second pass through
	     * we check the other part of the line.  We have to be very
	     * careful here because there could be embedded windows or
	     * other things that are not in the extracted line.  Rescan
	     * the original line to compute the index in it of the first
	     * character.
	     */

	    indexInDString = startingByte;
	    for (segPtr = linePtr->segPtr, leftToScan = startingByte;
		    leftToScan > 0; segPtr = segPtr->nextPtr) {
		if (segPtr->typePtr != &tkTextCharType) {
		    indexInDString -= segPtr->size;
		}
		leftToScan -= segPtr->size;
	    }

	    passes++;
	    if ((passes == 1) ^ backwards) {
		/*
		 * Only use the last part of the line.
		 */

		firstByte = indexInDString;
		if ((firstByte >= Tcl_DStringLength(&line))
			&& !((Tcl_DStringLength(&line) == 0) && !exact)) {
		    goto nextLine;
		}
	    } else {
		/*
		 * Use only the first part of the line.
		 */

		lastByte = indexInDString;
	    }
	}
	do {
	    int thisLength;
	    Tcl_UniChar ch;

	    if (exact) {
		p = strstr(startOfLine + firstByte,	/* INTL: Native. */
			pattern); 
		if (p == NULL) {
		    break;
		}
		i = p - startOfLine;
		thisLength = patLength;
	    } else {
		CONST char *start, *end;
		int match;

		match = Tcl_RegExpExec(interp, regexp,
			startOfLine + firstByte, startOfLine);
		if (match < 0) {
		    code = TCL_ERROR;
		    goto done;
		}
		if (!match) {
		    break;
		}
		Tcl_RegExpRange(regexp, 0, &start, &end);
		i = start - startOfLine;
		thisLength = end - start;
	    }
	    if (i >= lastByte) {
		break;
	    }
	    matchByte = i;
	    matchLength = thisLength;
	    firstByte = i + Tcl_UtfToUniChar(startOfLine + matchByte, &ch);
	} while (backwards);

	/*
	 * If we found a match then we're done.  Make sure that
	 * the match occurred before the stopping index, if one was
	 * specified.
	 */

	if (matchByte >= 0) {
	    int numChars;

	    /*
	     * Convert the byte length to a character count.
	     */

	    numChars = Tcl_NumUtfChars(startOfLine + matchByte,
		    matchLength);

	    /*
	     * The index information returned by the regular expression
	     * parser only considers textual information:  it doesn't
	     * account for embedded windows, elided text (when we are not
	     * searching elided text) or any other non-textual info.
	     * Scan through the line's segments again to adjust both
	     * matchChar and matchCount.
	     *
	     * We will walk through the segments of this line until we have
	     * either reached the end of the match or we have reached the end
	     * of the line.
	     */

	    curIndex.linePtr = linePtr; curIndex.byteIndex = 0;
	    for (segPtr = linePtr->segPtr, leftToScan = matchByte;
		    leftToScan >= 0 && segPtr; segPtr = segPtr->nextPtr) {
		if (segPtr->typePtr != &tkTextCharType || \
			(!searchElide && TkTextIsElided(textPtr, &curIndex))) {
		    matchByte += segPtr->size;
		} else {
		    leftToScan -= segPtr->size;
		}
		curIndex.byteIndex += segPtr->size;
	    }
	    for (leftToScan += matchLength; leftToScan > 0;
		    segPtr = segPtr->nextPtr) {
		if (segPtr->typePtr != &tkTextCharType) {
		    numChars += segPtr->size;
		    continue;
		}
		leftToScan -= segPtr->size;
	    }
	    TkTextMakeByteIndex(textPtr->tree, lineNum, matchByte, &index);
	    if (!searchWholeText) {
		if (!backwards && (TkTextIndexCmp(&index, &stopIndex) >= 0)) {
		    goto done;
		}
		if (backwards && (TkTextIndexCmp(&index, &stopIndex) < 0)) {
		    goto done;
		}
	    }
	    if (varName != NULL) {
		sprintf(buffer, "%d", numChars);
		if (Tcl_SetVar(interp, varName, buffer, TCL_LEAVE_ERR_MSG)
			== NULL) {
		    code = TCL_ERROR;
		    goto done;
		}
	    }
	    TkTextPrintIndex(&index, buffer);
	    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
	    goto done;
	}

	/*
	 * Go to the next (or previous) line;
	 */

	nextLine:
	if (backwards) {
	    lineNum--;
	    if (!searchWholeText) {
		if (lineNum < stopLine) {
		    break;
		}
	    } else if (lineNum < 0) {
		lineNum = numLines-1;
	    }
	} else {
	    lineNum++;
	    if (!searchWholeText) {
		if (lineNum > stopLine) {
		    break;
		}
	    } else if (lineNum >= numLines) {
		lineNum = 0;
	    }
	}
	Tcl_DStringSetLength(&line, 0);
    }
    done:
    Tcl_DStringFree(&line);
    if (noCase && exact) {
	Tcl_DStringFree(&patDString);
    }
    if (patObj != NULL) {
	Tcl_DecrRefCount(patObj);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextGetTabs --
 *
 *	Parses a string description of a set of tab stops.
 *
 * Results:
 *	The return value is a pointer to a malloc'ed structure holding
 *	parsed information about the tab stops.  If an error occurred
 *	then the return value is NULL and an error message is left in
 *	the interp's result.
 *
 * Side effects:
 *	Memory is allocated for the structure that is returned.  It is
 *	up to the caller to free this structure when it is no longer
 *	needed.
 *
 *----------------------------------------------------------------------
 */

TkTextTabArray *
TkTextGetTabs(interp, tkwin, string)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Tk_Window tkwin;			/* Window in which the tabs will be
					 * used. */
    char *string;			/* Description of the tab stops.  See
					 * the text manual entry for details. */
{
    int argc, i, count, c;
    CONST char **argv;
    TkTextTabArray *tabArrayPtr;
    TkTextTab *tabPtr;
    Tcl_UniChar ch;

    if (Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
	return NULL;
    }

    /*
     * First find out how many entries we need to allocate in the
     * tab array.
     */

    count = 0;
    for (i = 0; i < argc; i++) {
	c = argv[i][0];
	if ((c != 'l') && (c != 'r') && (c != 'c') && (c != 'n')) {
	    count++;
	}
    }

    /*
     * Parse the elements of the list one at a time to fill in the
     * array.
     */

    tabArrayPtr = (TkTextTabArray *) ckalloc((unsigned)
	    (sizeof(TkTextTabArray) + (count-1)*sizeof(TkTextTab)));
    tabArrayPtr->numTabs = 0;
    for (i = 0, tabPtr = &tabArrayPtr->tabs[0]; i  < argc; i++, tabPtr++) {
	if (Tk_GetPixels(interp, tkwin, argv[i], &tabPtr->location)
		!= TCL_OK) {
	    goto error;
	}
	tabArrayPtr->numTabs++;

	/*
	 * See if there is an explicit alignment in the next list
	 * element.  Otherwise just use "left".
	 */

	tabPtr->alignment = LEFT;
	if ((i+1) == argc) {
	    continue;
	}
	Tcl_UtfToUniChar(argv[i+1], &ch);
	if (!Tcl_UniCharIsAlpha(ch)) {
	    continue;
	}
	i += 1;
	c = argv[i][0];
	if ((c == 'l') && (strncmp(argv[i], "left",
		strlen(argv[i])) == 0)) {
	    tabPtr->alignment = LEFT;
	} else if ((c == 'r') && (strncmp(argv[i], "right",
		strlen(argv[i])) == 0)) {
	    tabPtr->alignment = RIGHT;
	} else if ((c == 'c') && (strncmp(argv[i], "center",
		strlen(argv[i])) == 0)) {
	    tabPtr->alignment = CENTER;
	} else if ((c == 'n') && (strncmp(argv[i],
		"numeric", strlen(argv[i])) == 0)) {
	    tabPtr->alignment = NUMERIC;
	} else {
	    Tcl_AppendResult(interp, "bad tab alignment \"",
		    argv[i], "\": must be left, right, center, or numeric",
		    (char *) NULL);
	    goto error;
	}
    }
    ckfree((char *) argv);
    return tabArrayPtr;

    error:
    ckfree((char *) tabArrayPtr);
    ckfree((char *) argv);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TextDumpCmd --
 *
 *	Return information about the text, tags, marks, and embedded windows
 *	and images in a text widget.  See the man page for the description
 *	of the text dump operation for all the details.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Memory is allocated for the result, if needed (standard Tcl result
 *	side effects).
 *
 *----------------------------------------------------------------------
 */

static int
TextDumpCmd(textPtr, interp, argc, argv)
    register TkText *textPtr;	/* Information about text widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST char **argv;		/* Argument strings.  Someone else has already
				 * parsed this command enough to know that
				 * argv[1] is "dump". */
{
    TkTextIndex index1, index2;
    int arg;
    int lineno;			/* Current line number */
    int what = 0;		/* bitfield to select segment types */
    int atEnd;			/* True if dumping up to logical end */
    TkTextLine *linePtr;
    CONST char *command = NULL;	/* Script callback to apply to segments */
#define TK_DUMP_TEXT	0x1
#define TK_DUMP_MARK	0x2
#define TK_DUMP_TAG	0x4
#define TK_DUMP_WIN	0x8
#define TK_DUMP_IMG	0x10
#define TK_DUMP_ALL	(TK_DUMP_TEXT|TK_DUMP_MARK|TK_DUMP_TAG| \
	TK_DUMP_WIN|TK_DUMP_IMG)

    for (arg=2 ; argv[arg] != (char *) NULL ; arg++) {
	size_t len;
	if (argv[arg][0] != '-') {
	    break;
	}
	len = strlen(argv[arg]);
	if (strncmp("-all", argv[arg], len) == 0) {
	    what = TK_DUMP_ALL;
	} else if (strncmp("-text", argv[arg], len) == 0) {
	    what |= TK_DUMP_TEXT;
	} else if (strncmp("-tag", argv[arg], len) == 0) {
	    what |= TK_DUMP_TAG;
	} else if (strncmp("-mark", argv[arg], len) == 0) {
	    what |= TK_DUMP_MARK;
	} else if (strncmp("-image", argv[arg], len) == 0) {
	    what |= TK_DUMP_IMG;
	} else if (strncmp("-window", argv[arg], len) == 0) {
	    what |= TK_DUMP_WIN;
	} else if (strncmp("-command", argv[arg], len) == 0) {
	    arg++;
	    if (arg >= argc) {
		Tcl_AppendResult(interp, "Usage: ", argv[0], " dump ?-all -image -text -mark -tag -window? ?-command script? index ?index2?", NULL);
		return TCL_ERROR;
	    }
	    command = argv[arg];
	} else {
	    Tcl_AppendResult(interp, "Usage: ", argv[0], " dump ?-all -image -text -mark -tag -window? ?-command script? index ?index2?", NULL);
	    return TCL_ERROR;
	}
    }
    if (arg >= argc) {
	Tcl_AppendResult(interp, "Usage: ", argv[0], " dump ?-all -image -text -mark -tag -window? ?-command script? index ?index2?", NULL);
	return TCL_ERROR;
    }
    if (what == 0) {
	what = TK_DUMP_ALL;
    }
    if (TkTextGetIndex(interp, textPtr, argv[arg], &index1) != TCL_OK) {
	return TCL_ERROR;
    }
    lineno = TkBTreeLineIndex(index1.linePtr);
    arg++;
    atEnd = 0;
    if (argc == arg) {
	TkTextIndexForwChars(&index1, 1, &index2);
    } else {
	if (TkTextGetIndex(interp, textPtr, argv[arg], &index2) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (strncmp(argv[arg], "end", strlen(argv[arg])) == 0) {
	    atEnd = 1;
	}
    }
    if (TkTextIndexCmp(&index1, &index2) >= 0) {
	return TCL_OK;
    }
    if (index1.linePtr == index2.linePtr) {
	DumpLine(interp, textPtr, what, index1.linePtr,
	    index1.byteIndex, index2.byteIndex, lineno, command);
    } else {
	DumpLine(interp, textPtr, what, index1.linePtr,
		index1.byteIndex, 32000000, lineno, command);
	linePtr = index1.linePtr;
	while ((linePtr = TkBTreeNextLine(linePtr)) != (TkTextLine *)NULL) {
	    lineno++;
	    if (linePtr == index2.linePtr) {
		break;
	    }
	    DumpLine(interp, textPtr, what, linePtr, 0, 32000000,
		    lineno, command);
	}
	DumpLine(interp, textPtr, what, index2.linePtr, 0,
		index2.byteIndex, lineno, command);
    }
    /*
     * Special case to get the leftovers hiding at the end mark.
     */
    if (atEnd) {
	DumpLine(interp, textPtr, what & ~TK_DUMP_TEXT, index2.linePtr,
		0, 1, lineno, command);			    

    }
    return TCL_OK;
}

/*
 * DumpLine
 * 	Return information about a given text line from character
 *	position "start" up to, but not including, "end".
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None, but see DumpSegment.
 */
static void
DumpLine(interp, textPtr, what, linePtr, startByte, endByte, lineno, command)
    Tcl_Interp *interp;
    TkText *textPtr;
    int what;			/* bit flags to select segment types */
    TkTextLine *linePtr;	/* The current line */
    int startByte, endByte;	/* Byte range to dump */
    int lineno;			/* Line number for indices dump */
    CONST char *command;	/* Script to apply to the segment */
{
    int offset;
    TkTextSegment *segPtr;
    TkTextIndex index;
    /*
     * Must loop through line looking at its segments.
     * character
     * toggleOn, toggleOff
     * mark
     * image
     * window
     */

    for (offset = 0, segPtr = linePtr->segPtr ;
	    (offset < endByte) && (segPtr != (TkTextSegment *)NULL) ;
	    offset += segPtr->size, segPtr = segPtr->nextPtr) {
	if ((what & TK_DUMP_TEXT) && (segPtr->typePtr == &tkTextCharType) &&
		(offset + segPtr->size > startByte)) {
	    char savedChar;		/* Last char used in the seg */
	    int last = segPtr->size;	/* Index of savedChar */
	    int first = 0;		/* Index of first char in seg */
	    if (offset + segPtr->size > endByte) {
		last = endByte - offset;
	    }
	    if (startByte > offset) {
		first = startByte - offset;
	    }
	    savedChar = segPtr->body.chars[last];
	    segPtr->body.chars[last] = '\0';
	    
	    TkTextMakeByteIndex(textPtr->tree, lineno, offset + first, &index);
	    DumpSegment(interp, "text", segPtr->body.chars + first,
		    command, &index, what);
	    segPtr->body.chars[last] = savedChar;
	} else if ((offset >= startByte)) {
	    if ((what & TK_DUMP_MARK) && (segPtr->typePtr->name[0] == 'm')) {
		TkTextMark *markPtr = (TkTextMark *)&segPtr->body;
		char *name = Tcl_GetHashKey(&textPtr->markTable, markPtr->hPtr);

		TkTextMakeByteIndex(textPtr->tree, lineno, offset, &index);
		DumpSegment(interp, "mark", name, command, &index, what);
	    } else if ((what & TK_DUMP_TAG) &&
			(segPtr->typePtr == &tkTextToggleOnType)) {
		TkTextMakeByteIndex(textPtr->tree, lineno, offset, &index);
		DumpSegment(interp, "tagon",
			segPtr->body.toggle.tagPtr->name,
			command, &index, what);
	    } else if ((what & TK_DUMP_TAG) && 
			(segPtr->typePtr == &tkTextToggleOffType)) {
		TkTextMakeByteIndex(textPtr->tree, lineno, offset, &index);
		DumpSegment(interp, "tagoff",
			segPtr->body.toggle.tagPtr->name,
			command, &index, what);
	    } else if ((what & TK_DUMP_IMG) && 
			(segPtr->typePtr->name[0] == 'i')) {
		TkTextEmbImage *eiPtr = (TkTextEmbImage *)&segPtr->body;
		char *name = (eiPtr->name ==  NULL) ? "" : eiPtr->name;
		TkTextMakeByteIndex(textPtr->tree, lineno, offset, &index);
		DumpSegment(interp, "image", name,
			command, &index, what);
	    } else if ((what & TK_DUMP_WIN) && 
			(segPtr->typePtr->name[0] == 'w')) {
		TkTextEmbWindow *ewPtr = (TkTextEmbWindow *)&segPtr->body;
		char *pathname;
		if (ewPtr->tkwin == (Tk_Window) NULL) {
		    pathname = "";
		} else {
		    pathname = Tk_PathName(ewPtr->tkwin);
		}
		TkTextMakeByteIndex(textPtr->tree, lineno, offset, &index);
		DumpSegment(interp, "window", pathname,
			command, &index, what);
	    }
	}
    }
}

/*
 * DumpSegment
 *	Either append information about the current segment to the result,
 *	or make a script callback with that information as arguments.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Either evals the callback or appends elements to the result string.
 */
static int
DumpSegment(interp, key, value, command, index, what)
    Tcl_Interp *interp;
    char *key;			/* Segment type key */
    char *value;		/* Segment value */
    CONST char *command;	/* Script callback */
    TkTextIndex *index;         /* index with line/byte position info */
    int what;			/* Look for TK_DUMP_INDEX bit */
{
    char buffer[TCL_INTEGER_SPACE*2];
    TkTextPrintIndex(index, buffer);
    if (command == NULL) {
	Tcl_AppendElement(interp, key);
	Tcl_AppendElement(interp, value);
	Tcl_AppendElement(interp, buffer);
	return TCL_OK;
    } else {
	CONST char *argv[4];
	char *list;
	int result;
	argv[0] = key;
	argv[1] = value;
	argv[2] = buffer;
	argv[3] = NULL;
	list = Tcl_Merge(3, argv);
	result = Tcl_VarEval(interp, command, " ", list, (char *) NULL);
	ckfree(list);
	return result;
    }
}

/*
 * TextEditUndo --
 *    undo the last change.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */

static int
TextEditUndo(textPtr)
    TkText     * textPtr;          /* Overall information about text widget. */
{
    int status;

    if (!textPtr->undo) {
       return TCL_OK;
    }

    /* Turn off the undo feature */
    textPtr->undo = 0;

    /* The dirty counter should count downwards as we are undoing things */
    textPtr->isDirtyIncrement = -1;

    /* revert one compound action */
    status = TkUndoRevert(textPtr->undoStack);

    /* Restore the isdirty increment */
    textPtr->isDirtyIncrement = 1;

    /* Turn back on the undo feature */
    textPtr->undo = 1;

    return status;
}

/*
 * TextEditRedo --
 *    redo the last undone change.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */

static int
TextEditRedo(textPtr)
    TkText     * textPtr;       /* Overall information about text widget. */
{
    int status;

    if (!textPtr->undo) {
       return TCL_OK;
    }

    /* Turn off the undo feature temporarily */
    textPtr->undo = 0;

    /* reapply one compound action */
    status = TkUndoApply(textPtr->undoStack);

    /* Turn back on the undo feature */
    textPtr->undo = 1;

    return status;
}

/*
 * TextEditCmd --
 *
 *    Handle the subcommands to "$text edit ...".
 *    See documentation for details.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */

static int
TextEditCmd(textPtr, interp, argc, argv)
    TkText *textPtr;          /* Information about text widget. */
    Tcl_Interp *interp;       /* Current interpreter. */
    int argc;                 /* Number of arguments. */
    CONST char **argv;        /* Argument strings. */
{
    int      c, setModified;
    size_t   length;

    if (argc < 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " edit option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[2][0];
    length = strlen(argv[2]);
    if ((c == 'm') && (strncmp(argv[2], "modified", length) == 0)) {
	if (argc == 3) {
	    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(textPtr->isDirty));
	} else if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " edit modified ?boolean?\"", (char *) NULL);
	    return TCL_ERROR;
	} else {
	    XEvent event;
	    if (Tcl_GetBoolean(interp, argv[3], &setModified) != TCL_OK) {
		return TCL_ERROR;
            }
	    /*
	     * Set or reset the dirty info and trigger a Modified event.
	     */

	    if (setModified) {
		textPtr->isDirty     = 1;
		textPtr->modifiedSet = 1;
	    } else {
		textPtr->isDirty     = 0;
		textPtr->modifiedSet = 0;
	    }

	    /*
	     * Send an event that the text was modified.  This is equivalent to
	     * "event generate $textWidget <<Modified>>"
	     */

	    memset((VOID *) &event, 0, sizeof(event));
	    event.xany.type = VirtualEvent;
	    event.xany.serial = NextRequest(Tk_Display(textPtr->tkwin));
	    event.xany.send_event = False;
	    event.xany.window = Tk_WindowId(textPtr->tkwin);
	    event.xany.display = Tk_Display(textPtr->tkwin);
	    ((XVirtualEvent *) &event)->name = Tk_GetUid("Modified");
	    Tk_HandleEvent(&event);
        }
    } else if ((c == 'r') && (strncmp(argv[2], "redo", length) == 0)
	    && (length >= 3)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " edit redo\"", (char *) NULL);
	    return TCL_ERROR;
	}
        if ( TextEditRedo(textPtr) ) {
            Tcl_AppendResult(interp, "nothing to redo", (char *) NULL);
	    return TCL_ERROR;
        }
    } else if ((c == 'r') && (strncmp(argv[2], "reset", length) == 0)
	    && (length >= 3)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " edit reset\"", (char *) NULL);
	    return TCL_ERROR;
	}
        TkUndoClearStacks(textPtr->undoStack);
    } else if ((c == 's') && (strncmp(argv[2], "separator", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " edit separator\"", (char *) NULL);
	    return TCL_ERROR;
	}
        TkUndoInsertUndoSeparator(textPtr->undoStack);
    } else if ((c == 'u') && (strncmp(argv[2], "undo", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " edit undo\"", (char *) NULL);
	    return TCL_ERROR;
	}
        if ( TextEditUndo(textPtr) ) {
            Tcl_AppendResult(interp, "nothing to undo",
		    (char *) NULL);
	    return TCL_ERROR;
        }
    } else {
	Tcl_AppendResult(interp, "bad edit option \"", argv[2],
		"\": must be modified, redo, reset, separator or undo",
		(char *) NULL);
	return TCL_ERROR;
    }
    
    return TCL_OK;
}

/*
 * TextGetText --
 *    Returns the text from indexPtr1 to indexPtr2, placing that text
 *    in the Tcl_DString given.  That DString should be free or uninitialized.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Memory will be allocated for the DString.  Remember to free it.
 */

static void 
TextGetText(indexPtr1,indexPtr2, dsPtr)
    TkTextIndex *indexPtr1;
    TkTextIndex *indexPtr2;
    Tcl_DString *dsPtr;
{
    TkTextIndex tmpIndex;
    Tcl_DStringInit(dsPtr);
    
    TkTextMakeByteIndex(indexPtr1->tree, TkBTreeLineIndex(indexPtr1->linePtr),
	    indexPtr1->byteIndex, &tmpIndex);

    if (TkTextIndexCmp(indexPtr1, indexPtr2) < 0) {
	while (1) {
	    int offset, last;
	    TkTextSegment *segPtr;

	    segPtr = TkTextIndexToSeg(&tmpIndex, &offset);
	    last = segPtr->size;
	    if (tmpIndex.linePtr == indexPtr2->linePtr) {
		int last2;

		if (indexPtr2->byteIndex == tmpIndex.byteIndex) {
		    break;
		}
		last2 = indexPtr2->byteIndex - tmpIndex.byteIndex + offset;
		if (last2 < last) {
		    last = last2;
		}
	    }
	    if (segPtr->typePtr == &tkTextCharType) {
		Tcl_DStringAppend(dsPtr, segPtr->body.chars + offset,
			last - offset);
	    }
	    TkTextIndexForwBytes(&tmpIndex, last-offset, &tmpIndex);
	}
    }
}

/*
 * updateDirtyFlag --
 *    increases the dirtyness of the text widget
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */

static void updateDirtyFlag (textPtr)
    TkText *textPtr;          /* Information about text widget. */
{
    int oldDirtyFlag;

    if (textPtr->modifiedSet) {
        return;
    }
    oldDirtyFlag = textPtr->isDirty;
    textPtr->isDirty += textPtr->isDirtyIncrement;
    if (textPtr->isDirty == 0 || oldDirtyFlag == 0) {
	XEvent event;
	/*
	 * Send an event that the text was modified.  This is equivalent to
	 * "event generate $textWidget <<Modified>>"
	 */

	memset((VOID *) &event, 0, sizeof(event));
	event.xany.type = VirtualEvent;
	event.xany.serial = NextRequest(Tk_Display(textPtr->tkwin));
	event.xany.send_event = False;
	event.xany.window = Tk_WindowId(textPtr->tkwin);
	event.xany.display = Tk_Display(textPtr->tkwin);
	((XVirtualEvent *) &event)->name = Tk_GetUid("Modified");
	Tk_HandleEvent(&event);
    }
}
