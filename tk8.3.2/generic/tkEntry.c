/* 
 * tkEntry.c --
 *
 *	This module implements entry widgets for the Tk
 *	toolkit.  An entry displays a string and allows
 *	the string to be edited.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "default.h"

/*
 * A data structure of the following type is kept for each entry
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the entry. NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with entry. */
    Tcl_Command widgetCmd;	/* Token for entry's widget command. */
    Tk_OptionTable optionTable;	/* Table that defines configuration options
				 * available for this widget. */


    /*
     * Fields that are set by widget commands other than "configure".
     */
     
    char *string;		/* Pointer to storage for string;
				 * NULL-terminated;  malloc-ed. */
    int insertPos;		/* Character index before which next typed
				 * character will be inserted. */

    /*
     * Information about what's selected, if any.
     */

    int selectFirst;		/* Character index of first selected
				 * character (-1 means nothing selected. */
    int selectLast;		/* Character index just after last selected
				 * character (-1 means nothing selected. */
    int selectAnchor;		/* Fixed end of selection (i.e. "select to"
				 * operation will use this as one end of the
				 * selection). */

    /*
     * Information for scanning:
     */

    int scanMarkX;		/* X-position at which scan started (e.g.
				 * button was pressed here). */
    int scanMarkIndex;		/* Character index of character that was at
				 * left of window when scan started. */

    /*
     * Configuration settings that are updated by Tk_ConfigureWidget.
     */

    Tk_3DBorder normalBorder;	/* Used for drawing border around whole
				 * window, plus used for background. */
    int borderWidth;		/* Width of 3-D border around window. */
    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    int exportSelection;	/* Non-zero means tie internal entry selection
				 * to X selection. */
    Tk_Font tkfont;		/* Information about text font, or NULL. */
    XColor *fgColorPtr;		/* Text color in normal mode. */
    XColor *highlightBgColorPtr;/* Color for drawing traversal highlight
				 * area when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    Tk_3DBorder insertBorder;	/* Used to draw vertical bar for insertion
				 * cursor. */
    int insertBorderWidth;	/* Width of 3-D border around insert cursor. */
    int insertOffTime;		/* Number of milliseconds cursor should spend
				 * in "off" state for each blink. */
    int insertOnTime;		/* Number of milliseconds cursor should spend
				 * in "on" state for each blink. */
    int insertWidth;		/* Total width of insert cursor. */
    Tk_Justify justify;		/* Justification to use for text within
				 * window. */
    int relief;			/* 3-D effect: TK_RELIEF_RAISED, etc. */
    Tk_3DBorder selBorder;	/* Border and background for selected
				 * characters. */
    int selBorderWidth;		/* Width of border around selection. */
    XColor *selFgColorPtr;	/* Foreground color for selected text. */
    char *showChar;		/* Value of -show option.  If non-NULL, first
				 * character is used for displaying all
				 * characters in entry.  Malloc'ed. */
    int state;		        /* Normal or disabled.  Entry is read-only
				 * when disabled. */
    char *textVarName;		/* Name of variable (malloc'ed) or NULL.
				 * If non-NULL, entry's string tracks the
				 * contents of this variable and vice versa. */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    int prefWidth;		/* Desired width of window, measured in
				 * average characters. */
    char *scrollCmd;		/* Command prefix for communicating with
				 * scrollbar(s).  Malloc'ed.  NULL means
				 * no command to issue. */

    /*
     * Fields whose values are derived from the current values of the
     * configuration settings above.
     */

    int numBytes;		/* Length of string in bytes. */
    int numChars;		/* Length of string in characters.  Both
				 * string and displayString have the same
				 * character length, but may have different
				 * byte lengths due to being made from
				 * different UTF-8 characters. */
    char *displayString;	/* String to use when displaying.  This may
				 * be a pointer to string, or a pointer to
				 * malloced memory with the same character
				 * length as string but whose characters
				 * are all equal to showChar. */
    int numDisplayBytes;	/* Length of displayString in bytes. */
    int inset;			/* Number of pixels on the left and right
				 * sides that are taken up by XPAD, borderWidth
				 * (if any), and highlightWidth (if any). */
    Tk_TextLayout textLayout;	/* Cached text layout information. */
    int layoutX, layoutY;	/* Origin for layout. */
    int leftX;			/* X position at which character at leftIndex
				 * is drawn (varies depending on justify). */
    int leftIndex;		/* Character index of left-most character
				 * visible in window. */
    Tcl_TimerToken insertBlinkHandler;
				/* Timer handler used to blink cursor on and
				 * off. */
    GC textGC;			/* For drawing normal text. */
    GC selTextGC;		/* For drawing selected text. */
    GC highlightGC;		/* For drawing traversal highlight. */
    int avgWidth;		/* Width of average character. */
    int flags;			/* Miscellaneous flags;  see below for
				 * definitions. */
    Tk_TSOffset tsoffset;

    char *validateCmd;          /* Command prefix to use when invoking
				 * validate command.  NULL means don't
				 * invoke commands.  Malloc'ed. */
    int validate;               /* Non-zero means try to validate */
    char *invalidCmd;		/* Command called when a validation returns 0
				 * (successfully fails), defaults to {}. */
} Entry;

/*
 * Assigned bits of "flags" fields of Entry structures, and what those
 * bits mean:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler has
 *				already been queued to redisplay the entry.
 * BORDER_NEEDED:		Non-zero means 3-D border must be redrawn
 *				around window during redisplay.  Normally
 *				only text portion needs to be redrawn.
 * CURSOR_ON:			Non-zero means insert cursor is displayed at
 *				present.  0 means it isn't displayed.
 * GOT_FOCUS:			Non-zero means this window has the input
 *				focus.
 * UPDATE_SCROLLBAR:		Non-zero means scrollbar should be updated
 *				during next redisplay operation.
 * GOT_SELECTION:		Non-zero means we've claimed the selection.
 * ENTRY_DELETED:		This entry has been effectively destroyed.
 * VALIDATING:                  Non-zero means we are in a validateCmd
 * VALIDATE_VAR:                Non-zero means we are attempting to validate
 *                              the entry's textvariable with validateCmd
 * VALIDATE_ABORT:              Non-zero if validatecommand signals an abort
 *                              for current procedure and make no changes
 */

#define REDRAW_PENDING		1
#define BORDER_NEEDED		2
#define CURSOR_ON		4
#define GOT_FOCUS		8
#define UPDATE_SCROLLBAR	0x10
#define GOT_SELECTION		0x20
#define ENTRY_DELETED           0x40
#define VALIDATING              0x80
#define VALIDATE_VAR            0x100
#define VALIDATE_ABORT          0x200

/*
 * The following macro defines how many extra pixels to leave on each
 * side of the text in the entry.
 */

#define XPAD 1
#define YPAD 1

/*
 * The following enum is used to define a type for the -state option
 * of the Entry widget.  These values are used as indices into the 
 * string table below.
 */

enum state {
    STATE_DISABLED, STATE_NORMAL
};

static char *stateStrings[] = {
    "disabled", "normal", (char *) NULL
};

/*
 * Definitions for -validate option values:
 */

static char *validateStrings[] = {
    "all", "key", "focus", "focusin", "focusout", "none", (char *) NULL
};
enum validateType {
    VALIDATE_ALL, VALIDATE_KEY, VALIDATE_FOCUS,
    VALIDATE_FOCUSIN, VALIDATE_FOCUSOUT, VALIDATE_NONE,
    /*
     * These extra enums are for use with EntryValidateChange
     */
    VALIDATE_FORCED, VALIDATE_DELETE, VALIDATE_INSERT
};
#define DEF_ENTRY_VALIDATE	"none"
#define DEF_ENTRY_INVALIDCMD	""

/*
 * Information used for argv parsing.
 */

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_ENTRY_BG_COLOR, -1, Tk_Offset(Entry, normalBorder),
	0, (ClientData) DEF_ENTRY_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_ENTRY_BORDER_WIDTH, -1, Tk_Offset(Entry, borderWidth), 
        0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_ENTRY_CURSOR, -1, Tk_Offset(Entry, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-exportselection", "exportSelection",
        "ExportSelection", DEF_ENTRY_EXPORT_SELECTION, -1, 
        Tk_Offset(Entry, exportSelection), 0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_ENTRY_FONT, -1, Tk_Offset(Entry, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_ENTRY_FG, -1, Tk_Offset(Entry, fgColorPtr), 0, 
        0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_ENTRY_HIGHLIGHT_BG,
	-1, Tk_Offset(Entry, highlightBgColorPtr), 
        0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_ENTRY_HIGHLIGHT, -1, Tk_Offset(Entry, highlightColorPtr),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_ENTRY_HIGHLIGHT_WIDTH, -1, 
	Tk_Offset(Entry, highlightWidth), 0, 0, 0},
    {TK_OPTION_BORDER, "-insertbackground", "insertBackground", "Foreground",
	DEF_ENTRY_INSERT_BG,
	-1, Tk_Offset(Entry, insertBorder), 
        0, 0, 0},
    {TK_OPTION_PIXELS, "-insertborderwidth", "insertBorderWidth", 
        "BorderWidth", DEF_ENTRY_INSERT_BD_COLOR, -1, 
        Tk_Offset(Entry, insertBorderWidth), 0, 
        (ClientData) DEF_ENTRY_INSERT_BD_MONO, 0},
    {TK_OPTION_INT, "-insertofftime", "insertOffTime", "OffTime",
        DEF_ENTRY_INSERT_OFF_TIME, -1, Tk_Offset(Entry, insertOffTime), 
        0, 0, 0},
    {TK_OPTION_INT, "-insertontime", "insertOnTime", "OnTime",
        DEF_ENTRY_INSERT_ON_TIME, -1, Tk_Offset(Entry, insertOnTime), 
        0, 0, 0},
    {TK_OPTION_PIXELS, "-insertwidth", "insertWidth", "InsertWidth",
	DEF_ENTRY_INSERT_WIDTH, -1, Tk_Offset(Entry, insertWidth), 
        0, 0, 0},
    {TK_OPTION_STRING, "-invalidcommand", "invalidCommand", "InvalidCommand",
	DEF_ENTRY_INVALIDCMD, -1, Tk_Offset(Entry, invalidCmd),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_SYNONYM, "-invcmd", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-invalidcommand", 0},
    {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
	DEF_ENTRY_JUSTIFY, -1, Tk_Offset(Entry, justify), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_ENTRY_RELIEF, -1, Tk_Offset(Entry, relief), 
        0, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
        DEF_ENTRY_SELECT_COLOR, -1, Tk_Offset(Entry, selBorder),
        0, (ClientData) DEF_ENTRY_SELECT_MONO, 0},
    {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth", 
        "BorderWidth", DEF_ENTRY_SELECT_BD_COLOR, -1, 
        Tk_Offset(Entry, selBorderWidth), 
        0, (ClientData) DEF_ENTRY_SELECT_BD_MONO, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_ENTRY_SELECT_FG_COLOR, -1, Tk_Offset(Entry, selFgColorPtr),
	0, (ClientData) DEF_ENTRY_SELECT_FG_MONO, 0},
    {TK_OPTION_STRING, "-show", "show", "Show",
        DEF_ENTRY_SHOW, -1, Tk_Offset(Entry, showChar), 
        TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	DEF_ENTRY_STATE, -1, Tk_Offset(Entry, state), 
        0, (ClientData) stateStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_ENTRY_TAKE_FOCUS, -1, Tk_Offset(Entry, takeFocus), 
        TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-textvariable", "textVariable", "Variable",
	DEF_ENTRY_TEXT_VARIABLE, -1, Tk_Offset(Entry, textVarName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-validate", "validate", "Validate",
       DEF_ENTRY_VALIDATE, -1, Tk_Offset(Entry, validate),
       0, (ClientData) validateStrings, 0},
    {TK_OPTION_STRING, "-validatecommand", "validateCommand", "ValidateCommand",
       (char *) NULL, -1, Tk_Offset(Entry, validateCmd),
       TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_SYNONYM, "-vcmd", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-validatecommand", 0},
    {TK_OPTION_INT, "-width", "width", "Width",
	DEF_ENTRY_WIDTH, -1, Tk_Offset(Entry, prefWidth), 0, 0, 0},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_ENTRY_SCROLL_COMMAND, -1, Tk_Offset(Entry, scrollCmd),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, 0, 0}
};

/*
 * Flags for GetEntryIndex procedure:
 */

#define ZERO_OK			1
#define LAST_PLUS_ONE_OK	2

/*
 * The following tables define the entry widget commands (and sub-
 * commands) and map the indexes into the string tables into 
 * enumerated types used to dispatch the entry widget command.
 */

static char *commandNames[] = {
    "bbox", "cget", "configure", "delete", "get", "icursor", "index", 
    "insert", "scan", "selection", "validate", "xview", (char *) NULL
};

enum command {
    COMMAND_BBOX, COMMAND_CGET, COMMAND_CONFIGURE, COMMAND_DELETE, 
    COMMAND_GET, COMMAND_ICURSOR, COMMAND_INDEX, COMMAND_INSERT, 
    COMMAND_SCAN, COMMAND_SELECTION, COMMAND_VALIDATE, COMMAND_XVIEW
};

static char *selCommandNames[] = {
    "adjust", "clear", "from", "present", "range", "to", (char *) NULL
};

enum selcommand {
    SELECTION_ADJUST, SELECTION_CLEAR, SELECTION_FROM,
    SELECTION_PRESENT, SELECTION_RANGE, SELECTION_TO
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ConfigureEntry _ANSI_ARGS_((Tcl_Interp *interp,
			    Entry *entryPtr, int objc, 
                            Tcl_Obj *CONST objv[], int flags));
static void		DeleteChars _ANSI_ARGS_((Entry *entryPtr, int index,
			    int count));
static void		DestroyEntry _ANSI_ARGS_((char *memPtr));
static void		DisplayEntry _ANSI_ARGS_((ClientData clientData));
static void		EntryBlinkProc _ANSI_ARGS_((ClientData clientData));
static void		EntryCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		EntryComputeGeometry _ANSI_ARGS_((Entry *entryPtr));
static void		EntryEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		EntryFocusProc _ANSI_ARGS_ ((Entry *entryPtr,
			    int gotFocus));
static int		EntryFetchSelection _ANSI_ARGS_((ClientData clientData,
			    int offset, char *buffer, int maxBytes));
static void		EntryLostSelection _ANSI_ARGS_((
			    ClientData clientData));
static void		EventuallyRedraw _ANSI_ARGS_((Entry *entryPtr));
static void		EntryScanTo _ANSI_ARGS_((Entry *entryPtr, int y));
static void		EntrySetValue _ANSI_ARGS_((Entry *entryPtr,
			    char *value));
static void		EntrySelectTo _ANSI_ARGS_((
			    Entry *entryPtr, int index));
static char *		EntryTextVarProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
static void		EntryUpdateScrollbar _ANSI_ARGS_((Entry *entryPtr));
static int		EntryValidate _ANSI_ARGS_((Entry *entryPtr,
			    char *cmd));
static int		EntryValidateChange _ANSI_ARGS_((Entry *entryPtr,
			    char *change, char *new, int index, int type));
static void		ExpandPercents _ANSI_ARGS_((Entry *entryPtr,
			    char *before, char *change, char *new,
			    int index, int type, Tcl_DString *dsPtr));
static void		EntryValueChanged _ANSI_ARGS_((Entry *entryPtr));
static void		EntryVisibleRange _ANSI_ARGS_((Entry *entryPtr,
			    double *firstPtr, double *lastPtr));
static int		EntryWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static void		EntryWorldChanged _ANSI_ARGS_((
			    ClientData instanceData));
static int		GetEntryIndex _ANSI_ARGS_((Tcl_Interp *interp,
			    Entry *entryPtr, char *string, int *indexPtr));
static void		InsertChars _ANSI_ARGS_((Entry *entryPtr, int index,
			    char *string));

/*
 * The structure below defines entry class behavior by means of procedures
 * that can be invoked from generic window code.
 */

static TkClassProcs entryClass = {
    NULL,			/* createProc. */
    EntryWorldChanged,		/* geometryProc. */
    NULL			/* modalProc. */
};


/*
 *--------------------------------------------------------------
 *
 * Tk_EntryObjCmd --
 *
 *	This procedure is invoked to process the "entry" Tcl
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
Tk_EntryObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];      /* Argument objects. */
{
    register Entry *entryPtr;
    Tk_OptionTable optionTable;
    Tk_Window tkwin;

    optionTable = (Tk_OptionTable) clientData;
    if (optionTable == NULL) {
	Tcl_CmdInfo info;
	char *name;

	/*
	 * We haven't created the option table for this widget class
	 * yet.  Do it now and save the table as the clientData for
	 * the command, so we'll have access to it in future
	 * invocations of the command.
	 */

	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
	name = Tcl_GetString(objv[0]);
	Tcl_GetCommandInfo(interp, name, &info);
	info.objClientData = (ClientData) optionTable;
	Tcl_SetCommandInfo(interp, name, &info);
    }

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
            Tcl_GetString(objv[1]), (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    /*
     * Initialize the fields of the structure that won't be initialized
     * by ConfigureEntry, or that ConfigureEntry requires to be
     * initialized already (e.g. resource pointers).
     */

    entryPtr			= (Entry *) ckalloc(sizeof(Entry));
    entryPtr->tkwin		= tkwin;
    entryPtr->display		= Tk_Display(tkwin);
    entryPtr->interp		= interp;
    entryPtr->widgetCmd		= Tcl_CreateObjCommand(interp,
	    Tk_PathName(entryPtr->tkwin), EntryWidgetObjCmd,
	    (ClientData) entryPtr, EntryCmdDeletedProc);
    entryPtr->optionTable	= optionTable;
    entryPtr->string		= (char *) ckalloc(1);
    entryPtr->string[0]		= '\0';
    entryPtr->insertPos		= 0;
    entryPtr->selectFirst	= -1;
    entryPtr->selectLast	= -1;
    entryPtr->selectAnchor	= 0;
    entryPtr->scanMarkX		= 0;
    entryPtr->scanMarkIndex	= 0;

    entryPtr->normalBorder	= NULL;
    entryPtr->borderWidth	= 0;
    entryPtr->cursor		= None;
    entryPtr->exportSelection	= 1;
    entryPtr->tkfont		= NULL;
    entryPtr->fgColorPtr	= NULL;
    entryPtr->highlightBgColorPtr	= NULL;
    entryPtr->highlightColorPtr	= NULL;
    entryPtr->highlightWidth	= 0;
    entryPtr->insertBorder	= NULL;
    entryPtr->insertBorderWidth	= 0;
    entryPtr->insertOffTime	= 0;
    entryPtr->insertOnTime	= 0;
    entryPtr->insertWidth	= 0;
    entryPtr->justify		= TK_JUSTIFY_LEFT;
    entryPtr->relief		= TK_RELIEF_FLAT;
    entryPtr->selBorder		= NULL;
    entryPtr->selBorderWidth	= 0;
    entryPtr->selFgColorPtr	= NULL;
    entryPtr->showChar		= NULL;
    entryPtr->state		= STATE_NORMAL;
    entryPtr->textVarName	= NULL;
    entryPtr->takeFocus		= NULL;
    entryPtr->prefWidth		= 0;
    entryPtr->scrollCmd		= NULL;
    entryPtr->numBytes		= 0;
    entryPtr->numChars		= 0;
    entryPtr->displayString	= entryPtr->string;
    entryPtr->numDisplayBytes	= 0;
    entryPtr->inset		= XPAD;
    entryPtr->textLayout	= NULL;
    entryPtr->layoutX		= 0;
    entryPtr->layoutY		= 0;
    entryPtr->leftX		= 0;
    entryPtr->leftIndex		= 0;
    entryPtr->insertBlinkHandler	= (Tcl_TimerToken) NULL;
    entryPtr->textGC		= None;
    entryPtr->selTextGC		= None;
    entryPtr->highlightGC	= None;
    entryPtr->avgWidth		= 1;
    entryPtr->flags		= 0;
    entryPtr->validateCmd	= NULL;
    entryPtr->validate		= VALIDATE_NONE;
    entryPtr->invalidCmd	= NULL;

    Tk_SetClass(entryPtr->tkwin, "Entry");
    TkSetClassProcs(entryPtr->tkwin, &entryClass, (ClientData) entryPtr);
    Tk_CreateEventHandler(entryPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    EntryEventProc, (ClientData) entryPtr);
    Tk_CreateSelHandler(entryPtr->tkwin, XA_PRIMARY, XA_STRING,
	    EntryFetchSelection, (ClientData) entryPtr, XA_STRING);

    if ((Tk_InitOptions(interp, (char *) entryPtr, optionTable, tkwin)
	    != TCL_OK) ||
	    (ConfigureEntry(interp, entryPtr, objc-2, objv+2, 0) != TCL_OK)) {
	Tk_DestroyWindow(entryPtr->tkwin);
	return TCL_ERROR;
    }
    
    Tcl_SetResult(interp, Tk_PathName(entryPtr->tkwin), TCL_STATIC);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * EntryWidgetObjCmd --
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
EntryWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about entry widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Entry *entryPtr = (Entry *) clientData;
    int cmdIndex, selIndex, result;
    Tcl_Obj *objPtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) entryPtr);

    /* 
     * Parse the widget command by looking up the second token in
     * the list of valid command names. 
     */

    result = Tcl_GetIndexFromObj(interp, objv[1], commandNames,
	    "option", 0, &cmdIndex);
    if (result != TCL_OK) {
	return result;
    }

    switch (cmdIndex) {
        case COMMAND_BBOX: {
	    int index, x, y, width, height;
	    char buf[TCL_INTEGER_SPACE * 4];

	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "index");
		goto error;
	    }
	    if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[2]), 
                    &index) != TCL_OK) {
	        goto error;
	    }
	    if ((index == entryPtr->numChars) && (index > 0)) {
	        index--;
	    }
	    Tk_CharBbox(entryPtr->textLayout, index, &x, &y, 
                    &width, &height);
	    sprintf(buf, "%d %d %d %d", x + entryPtr->layoutX,
		    y + entryPtr->layoutY, width, height);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    break;
	} 
	
        case COMMAND_CGET: {
	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "option");
		goto error;
	    }
	    
	    objPtr = Tk_GetOptionValue(interp, (char *) entryPtr,
		    entryPtr->optionTable, objv[2], entryPtr->tkwin);
	    if (objPtr == NULL) {
		 goto error;
	    } else {
		Tcl_SetObjResult(interp, objPtr);
	    }
	    break;
	}

        case COMMAND_CONFIGURE: {
	    if (objc <= 3) {
		objPtr = Tk_GetOptionInfo(interp, (char *) entryPtr,
			entryPtr->optionTable,
			(objc == 3) ? objv[2] : (Tcl_Obj *) NULL,
			entryPtr->tkwin);
		if (objPtr == NULL) {
		    goto error;
		} else {
		    Tcl_SetObjResult(interp, objPtr);
		}
	    } else {
		result = ConfigureEntry(interp, entryPtr, objc-2, objv+2, 0);
	    }
	    break;
	}

        case COMMAND_DELETE: {
	    int first, last;

	    if ((objc < 3) || (objc > 4)) {
	        Tcl_WrongNumArgs(interp, 2, objv, "firstIndex ?lastIndex?");
		goto error;
	    }
	    if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[2]), 
                    &first) != TCL_OK) {
	        goto error;
	    }
	    if (objc == 3) {
	        last = first + 1;
	    } else {
	        if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[3]), 
                        &last) != TCL_OK) {
		    goto error;
		}
	    }
	    if ((last >= first) && (entryPtr->state == STATE_NORMAL)) {
	        DeleteChars(entryPtr, first, last - first);
	    }
	    break;
	}

        case COMMAND_GET: {
	    if (objc != 2) {
	        Tcl_WrongNumArgs(interp, 2, objv, (char *) NULL);
		goto error;
	    }
	    Tcl_SetResult(interp, entryPtr->string, TCL_STATIC);
	    break;
	}

        case COMMAND_ICURSOR: {
	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "pos");
		goto error;
	    }
	    if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[2]),
                    &entryPtr->insertPos) != TCL_OK) {
	        goto error;
	    }
	    EventuallyRedraw(entryPtr);
	    break;
	}
	
        case COMMAND_INDEX: {
	    int index;

	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "string");
		goto error;
	    }
	    if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[2]), 
                    &index) != TCL_OK) {
	        goto error;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(index));
	    break;
	}

        case COMMAND_INSERT: {
	    int index;

	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, "index text");
		goto error;
	    }
	    if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[2]), 
                    &index) != TCL_OK) {
	        goto error;
	    }
	    if (entryPtr->state == STATE_NORMAL) {
	        InsertChars(entryPtr, index, Tcl_GetString(objv[3]));
	    }
	    break;
	}

        case COMMAND_SCAN: {
	    int x;
	    char *minorCmd;

	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 2, objv, "mark|dragto x");
		goto error;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
	        goto error;
	    }

	    minorCmd = Tcl_GetString(objv[2]);
	    if (minorCmd[0] == 'm' 
                    && (strncmp(minorCmd, "mark", strlen(minorCmd)) == 0)) {
	        entryPtr->scanMarkX = x;
		entryPtr->scanMarkIndex = entryPtr->leftIndex;
	    } else if ((minorCmd[0] == 'd')
		&& (strncmp(minorCmd, "dragto", strlen(minorCmd)) == 0)) {
	        EntryScanTo(entryPtr, x);
	    } else {
	        Tcl_AppendResult(interp, "bad scan option \"", 
                        Tcl_GetString(objv[2]), "\": must be mark or dragto", 
                        (char *) NULL);
		goto error;
	    }
	    break;
	}
	    
	case COMMAND_SELECTION: {
	    int index, index2;

	    if (objc < 3) {
	        Tcl_WrongNumArgs(interp, 2, objv, "option ?index?");
		goto error;
	    }

	    /* 
	     * Parse the selection sub-command, using the command
	     * table "selCommandNames" defined above.
	     */
	    
	    result = Tcl_GetIndexFromObj(interp, objv[2], selCommandNames,
                    "selection option", 0, &selIndex);
	    if (result != TCL_OK) {
	        goto error;
	    }

	    switch(selIndex) {
	        case SELECTION_ADJUST: {
		    if (objc != 4) {
		        Tcl_WrongNumArgs(interp, 3, objv, "index");
			goto error;
		    }
		    if (GetEntryIndex(interp, entryPtr, 
                            Tcl_GetString(objv[3]), &index) != TCL_OK) {
		        goto error;
		    }
		    if (entryPtr->selectFirst >= 0) {
		        int half1, half2;
		
			half1 = (entryPtr->selectFirst 
			        + entryPtr->selectLast)/2;
			half2 = (entryPtr->selectFirst 
				+ entryPtr->selectLast + 1)/2;
			if (index < half1) {
			    entryPtr->selectAnchor = entryPtr->selectLast;
			} else if (index > half2) {
			    entryPtr->selectAnchor = entryPtr->selectFirst;
			} else {
			  /*
			   * We're at about the halfway point in the 
			   * selection; just keep the existing anchor.
			   */
			}
		    }
		    EntrySelectTo(entryPtr, index);
		    break;
		}

	        case SELECTION_CLEAR: {
		    if (objc != 3) {
		        Tcl_WrongNumArgs(interp, 3, objv, (char *) NULL);
			goto error;
		    }
		    if (entryPtr->selectFirst >= 0) {
		        entryPtr->selectFirst = -1;
			entryPtr->selectLast = -1;
			EventuallyRedraw(entryPtr);
		    }
		    goto done;
		}

	        case SELECTION_FROM: {
		    if (objc != 4) {
		        Tcl_WrongNumArgs(interp, 3, objv, "index");
			goto error;
		    }
		    if (GetEntryIndex(interp, entryPtr, 
                            Tcl_GetString(objv[3]), &index) != TCL_OK) {
		        goto error;
		    }
		    entryPtr->selectAnchor = index;
		    break;
		}

	        case SELECTION_PRESENT: {
		    if (objc != 3) {
		        Tcl_WrongNumArgs(interp, 3, objv, (char *) NULL);
			goto error;
		    }
		    if (entryPtr->selectFirst < 0) {
		        Tcl_SetResult(interp, "0", TCL_STATIC);
		    } else {
		        Tcl_SetResult(interp, "1", TCL_STATIC);
		    }
		    goto done;
		}

	        case SELECTION_RANGE: {
		    if (objc != 5) {
		        Tcl_WrongNumArgs(interp, 3, objv, "start end");
			goto error;
		    }
		    if (GetEntryIndex(interp, entryPtr, 
                            Tcl_GetString(objv[3]), &index) != TCL_OK) {
		        goto error;
		    }
		    if (GetEntryIndex(interp, entryPtr, 
                            Tcl_GetString(objv[4]),& index2) != TCL_OK) {
		        goto error;
		    }
		    if (index >= index2) {
		        entryPtr->selectFirst = -1;
			entryPtr->selectLast = -1;
		    } else {
		        entryPtr->selectFirst = index;
			entryPtr->selectLast = index2;
		    }
		    if (!(entryPtr->flags & GOT_SELECTION)
			    && (entryPtr->exportSelection)) {
		        Tk_OwnSelection(entryPtr->tkwin, XA_PRIMARY, 
			        EntryLostSelection, (ClientData) entryPtr);
			entryPtr->flags |= GOT_SELECTION;
		    }
		    EventuallyRedraw(entryPtr);
		    break;
		}
		
	        case SELECTION_TO: {
		    if (objc != 4) {
		        Tcl_WrongNumArgs(interp, 3, objv, "index");
			goto error;
		    }
		    if (GetEntryIndex(interp, entryPtr, 
                            Tcl_GetString(objv[3]), &index) != TCL_OK) {
		        goto error;
		    }
		    EntrySelectTo(entryPtr, index);
		    break;
		}
	    }
	    break;
	}

        case COMMAND_VALIDATE: {
	    int code;

	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, (char *) NULL);
		goto error;
	    }
	    selIndex = entryPtr->validate;
	    entryPtr->validate = VALIDATE_ALL;
	    code = EntryValidateChange(entryPtr, (char *) NULL,
				       entryPtr->string, -1, VALIDATE_FORCED);
	    if (entryPtr->validate != VALIDATE_NONE) {
		entryPtr->validate = selIndex;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewBooleanObj((code == TCL_OK)));
	    break;
	}

        case COMMAND_XVIEW: {
	    int index;

	    if (objc == 2) {
	        double first, last;
		char buf[TCL_DOUBLE_SPACE * 2];
	    
		EntryVisibleRange(entryPtr, &first, &last);
		sprintf(buf, "%g %g", first, last);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		goto done;
	    } else if (objc == 3) {
	        if (GetEntryIndex(interp, entryPtr, Tcl_GetString(objv[2]), 
                        &index) != TCL_OK) {
		    goto error;
		}
	    } else {
	        double fraction;
		int count;

		index = entryPtr->leftIndex;
		switch (Tk_GetScrollInfoObj(interp, objc, objv, &fraction, 
                        &count)) {
		    case TK_SCROLL_ERROR: {
		        goto error;
		    }
		    case TK_SCROLL_MOVETO: {
		        index = (int) ((fraction * entryPtr->numChars) + 0.5);
			break;
		    }
		    case TK_SCROLL_PAGES: {
		        int charsPerPage;
		    
			charsPerPage = ((Tk_Width(entryPtr->tkwin)
    			        - 2 * entryPtr->inset) 
                                / entryPtr->avgWidth) - 2;
			if (charsPerPage < 1) {
			    charsPerPage = 1;
			}
			index += count * charsPerPage;
			break;
		    }
		    case TK_SCROLL_UNITS: {
		        index += count;
			break;
		    }
		}
	    }
	    if (index >= entryPtr->numChars) {
	        index = entryPtr->numChars - 1;
	    }
	    if (index < 0) {
	        index = 0;
	    }
	    entryPtr->leftIndex = index;
	    entryPtr->flags |= UPDATE_SCROLLBAR;
	    EntryComputeGeometry(entryPtr);
	    EventuallyRedraw(entryPtr);
	    break;
	}
    }

    done:
    Tcl_Release((ClientData) entryPtr);
    return result;

    error:
    Tcl_Release((ClientData) entryPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyEntry --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of an entry at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the entry is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyEntry(memPtr)
    char *memPtr;		/* Info about entry widget. */
{
    Entry *entryPtr = (Entry *) memPtr;
    entryPtr->flags |= ENTRY_DELETED;

    Tcl_DeleteCommandFromToken(entryPtr->interp, entryPtr->widgetCmd);
    if (entryPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(DisplayEntry, (ClientData) entryPtr);
    }

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    ckfree(entryPtr->string);
    if (entryPtr->textVarName != NULL) {
	Tcl_UntraceVar(entryPtr->interp, entryPtr->textVarName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		EntryTextVarProc, (ClientData) entryPtr);
    }
    if (entryPtr->textGC != None) {
	Tk_FreeGC(entryPtr->display, entryPtr->textGC);
    }
    if (entryPtr->selTextGC != None) {
	Tk_FreeGC(entryPtr->display, entryPtr->selTextGC);
    }
    Tcl_DeleteTimerHandler(entryPtr->insertBlinkHandler);
    if (entryPtr->displayString != entryPtr->string) {
	ckfree(entryPtr->displayString);
    }
    Tk_FreeTextLayout(entryPtr->textLayout);
    Tk_FreeConfigOptions((char *) entryPtr, entryPtr->optionTable,
	    entryPtr->tkwin);
    entryPtr->tkwin = NULL;
    ckfree((char *) entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureEntry --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or reconfigure)
 *	an entry widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for entryPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureEntry(interp, entryPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Entry *entryPtr;		/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc;			/* Number of valid entries in argv. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error;
    int oldExport;

    /*
     * Eliminate any existing trace on a variable monitored by the entry.
     */

    if (entryPtr->textVarName != NULL) {
	Tcl_UntraceVar(interp, entryPtr->textVarName, 
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		EntryTextVarProc, (ClientData) entryPtr);
    }

    oldExport = entryPtr->exportSelection;

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) entryPtr,
		    entryPtr->optionTable, objc, objv,
		    entryPtr->tkwin, &savedOptions, (int *) NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: restore options to old values.
	     */

	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}

	/*
	 * A few other options also need special processing, such as parsing
	 * the geometry and setting the background from a 3-D border.
	 */

	Tk_SetBackgroundFromBorder(entryPtr->tkwin, entryPtr->normalBorder);

	if (entryPtr->insertWidth <= 0) {
	    entryPtr->insertWidth = 2;
	}
	if (entryPtr->insertBorderWidth > entryPtr->insertWidth/2) {
	    entryPtr->insertBorderWidth = entryPtr->insertWidth/2;
	}

	/*
	 * Restart the cursor timing sequence in case the on-time or 
	 * off-time just changed.  Set validate temporarily to none,
	 * so the configure doesn't cause it to be triggered.
	 */

	if (entryPtr->flags & GOT_FOCUS) {
	    int validate = entryPtr->validate;
	    entryPtr->validate = VALIDATE_NONE;
	    EntryFocusProc(entryPtr, 1);
	    entryPtr->validate = validate;
	}

	/*
	 * Claim the selection if we've suddenly started exporting it.
	 */

	if (entryPtr->exportSelection && (!oldExport)
	        && (entryPtr->selectFirst != -1)
	        && !(entryPtr->flags & GOT_SELECTION)) {
	    Tk_OwnSelection(entryPtr->tkwin, XA_PRIMARY, EntryLostSelection,
		    (ClientData) entryPtr);
	    entryPtr->flags |= GOT_SELECTION;
	}

	/*
	 * Recompute the window's geometry and arrange for it to be
	 * redisplayed.
	 */

	Tk_SetInternalBorder(entryPtr->tkwin,
	        entryPtr->borderWidth + entryPtr->highlightWidth);
	if (entryPtr->highlightWidth <= 0) {
	    entryPtr->highlightWidth = 0;
	}
	entryPtr->inset = entryPtr->highlightWidth 
	        + entryPtr->borderWidth + XPAD;
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
    }

    /*
     * If the entry is tied to the value of a variable, then set up
     * a trace on the variable's value, create the variable if it doesn't
     * exist, and set the entry's value from the variable's value.
     */

    if (entryPtr->textVarName != NULL) {
	char *value;

	value = Tcl_GetVar(interp, entryPtr->textVarName, TCL_GLOBAL_ONLY);
	if (value == NULL) {
	    EntryValueChanged(entryPtr);
	} else {
	    EntrySetValue(entryPtr, value);
	}
	Tcl_TraceVar(interp, entryPtr->textVarName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		EntryTextVarProc, (ClientData) entryPtr);
    }

    EntryWorldChanged((ClientData) entryPtr);
    if (error) {
        Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    } else {
        return TCL_OK;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryWorldChanged --
 *
 *      This procedure is called when the world has changed in some
 *      way and the widget needs to recompute all its graphics contexts
 *	and determine its new geometry.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Entry will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
EntryWorldChanged(instanceData)
    ClientData instanceData;	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc = None;
    unsigned long mask;
    Entry *entryPtr;

    entryPtr = (Entry *) instanceData;

    entryPtr->avgWidth = Tk_TextWidth(entryPtr->tkfont, "0", 1);
    if (entryPtr->avgWidth == 0) {
	entryPtr->avgWidth = 1;
    }

    if (entryPtr->normalBorder != NULL) {
	Tk_SetBackgroundFromBorder(entryPtr->tkwin, entryPtr->normalBorder);
    }

    gcValues.foreground = entryPtr->fgColorPtr->pixel;
    gcValues.font = Tk_FontId(entryPtr->tkfont);
    gcValues.graphics_exposures = False;
    mask = GCForeground | GCFont | GCGraphicsExposures;
    gc = Tk_GetGC(entryPtr->tkwin, mask, &gcValues);
    if (entryPtr->textGC != None) {
	Tk_FreeGC(entryPtr->display, entryPtr->textGC);
    }
    entryPtr->textGC = gc;

    gcValues.foreground = entryPtr->selFgColorPtr->pixel;
    gcValues.font = Tk_FontId(entryPtr->tkfont);
    mask = GCForeground | GCFont;
    gc = Tk_GetGC(entryPtr->tkwin, mask, &gcValues);
    if (entryPtr->selTextGC != None) {
	Tk_FreeGC(entryPtr->display, entryPtr->selTextGC);
    }
    entryPtr->selTextGC = gc;

    /*
     * Recompute the window's geometry and arrange for it to be
     * redisplayed.
     */

    EntryComputeGeometry(entryPtr);
    entryPtr->flags |= UPDATE_SCROLLBAR;
    EventuallyRedraw(entryPtr);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayEntry --
 *
 *	This procedure redraws the contents of an entry window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayEntry(clientData)
    ClientData clientData;	/* Information about window. */
{
    Entry *entryPtr = (Entry *) clientData;
    Tk_Window tkwin = entryPtr->tkwin;
    int baseY, selStartX, selEndX, cursorX;
    int xBound;
    Tk_FontMetrics fm;
    Pixmap pixmap;
    int showSelection;

    entryPtr->flags &= ~REDRAW_PENDING;
    if ((entryPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    Tk_GetFontMetrics(entryPtr->tkfont, &fm);

    /*
     * Update the scrollbar if that's needed.
     */

    if (entryPtr->flags & UPDATE_SCROLLBAR) {
	entryPtr->flags &= ~UPDATE_SCROLLBAR;
	EntryUpdateScrollbar(entryPtr);
    }

    /*
     * In order to avoid screen flashes, this procedure redraws the
     * textual area of the entry into off-screen memory, then copies
     * it back on-screen in a single operation.  This means there's
     * no point in time where the on-screen image has been cleared.
     */

    pixmap = Tk_GetPixmap(entryPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

    /*
     * Compute x-coordinate of the pixel just after last visible
     * one, plus vertical position of baseline of text.
     */

    xBound = Tk_Width(tkwin) - entryPtr->inset;
    baseY = (Tk_Height(tkwin) + fm.ascent - fm.descent) / 2;

    /*
     * On Windows and Mac, we need to hide the selection whenever we
     * don't have the focus.
     */

#ifdef ALWAYS_SHOW_SELECTION
    showSelection = 1;
#else
    showSelection = (entryPtr->flags & GOT_FOCUS);
#endif

    /*
     * Draw the background in three layers.  From bottom to top the
     * layers are:  normal background, selection background, and
     * insertion cursor background.
     */

    Tk_Fill3DRectangle(tkwin, pixmap, entryPtr->normalBorder,
		0, 0, Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    if (showSelection
	    && (entryPtr->selectLast > entryPtr->leftIndex)) {
	if (entryPtr->selectFirst <= entryPtr->leftIndex) {
	    selStartX = entryPtr->leftX;
	} else {
	    Tk_CharBbox(entryPtr->textLayout, entryPtr->selectFirst,
		    &selStartX, NULL, NULL, NULL);
	    selStartX += entryPtr->layoutX;
	}
	if ((selStartX - entryPtr->selBorderWidth) < xBound) {
	    Tk_CharBbox(entryPtr->textLayout, entryPtr->selectLast,
		    &selEndX, NULL, NULL, NULL);
	    selEndX += entryPtr->layoutX;
	    Tk_Fill3DRectangle(tkwin, pixmap, entryPtr->selBorder,
		    selStartX - entryPtr->selBorderWidth,
		    baseY - fm.ascent - entryPtr->selBorderWidth,
		    (selEndX - selStartX) + 2*entryPtr->selBorderWidth,
		    (fm.ascent + fm.descent) + 2*entryPtr->selBorderWidth,
		    entryPtr->selBorderWidth, TK_RELIEF_RAISED);
	} 
    }

    /*
     * Draw a special background for the insertion cursor, overriding
     * even the selection background.  As a special hack to keep the
     * cursor visible when the insertion cursor color is the same as
     * the color for selected text (e.g., on mono displays), write
     * background in the cursor area (instead of nothing) when the
     * cursor isn't on.  Otherwise the selection would hide the cursor.
     */

    if ((entryPtr->insertPos >= entryPtr->leftIndex)
	    && (entryPtr->state == STATE_NORMAL)
	    && (entryPtr->flags & GOT_FOCUS)) {
	Tk_CharBbox(entryPtr->textLayout, entryPtr->insertPos, &cursorX, NULL,
		NULL, NULL);
	cursorX += entryPtr->layoutX;
	cursorX -= (entryPtr->insertWidth)/2;
	if (cursorX < xBound) {
	    if (entryPtr->flags & CURSOR_ON) {
		Tk_Fill3DRectangle(tkwin, pixmap, entryPtr->insertBorder,
			cursorX, baseY - fm.ascent, entryPtr->insertWidth,
			fm.ascent + fm.descent, entryPtr->insertBorderWidth,
			TK_RELIEF_RAISED);
	    } else if (entryPtr->insertBorder == entryPtr->selBorder) {
		Tk_Fill3DRectangle(tkwin, pixmap, entryPtr->normalBorder,
			cursorX, baseY - fm.ascent, entryPtr->insertWidth,
			fm.ascent + fm.descent, 0, TK_RELIEF_FLAT);
	    }
	}
    }

    /*
     * Draw the text in two pieces:  first the unselected portion, then the
     * selected portion on top of it.
     */

    Tk_DrawTextLayout(entryPtr->display, pixmap, entryPtr->textGC,
	    entryPtr->textLayout, entryPtr->layoutX, entryPtr->layoutY,
	    entryPtr->leftIndex, entryPtr->numChars);

    if (showSelection
	    && (entryPtr->selTextGC != entryPtr->textGC)
	    && (entryPtr->selectFirst < entryPtr->selectLast)) {
	int selFirst;

	if (entryPtr->selectFirst < entryPtr->leftIndex) {
	    selFirst = entryPtr->leftIndex;
	} else {
	    selFirst = entryPtr->selectFirst;
	}
	Tk_DrawTextLayout(entryPtr->display, pixmap, entryPtr->selTextGC,
		entryPtr->textLayout, entryPtr->layoutX, entryPtr->layoutY,
		selFirst, entryPtr->selectLast);
    }

    /*
     * Draw the border and focus highlight last, so they will overwrite
     * any text that extends past the viewable part of the window.
     */

    if (entryPtr->relief != TK_RELIEF_FLAT) {
	Tk_Draw3DRectangle(tkwin, pixmap, entryPtr->normalBorder,
		entryPtr->highlightWidth, entryPtr->highlightWidth,
		Tk_Width(tkwin) - 2 * entryPtr->highlightWidth,
		Tk_Height(tkwin) - 2 * entryPtr->highlightWidth,
		entryPtr->borderWidth, entryPtr->relief);
    }
    if (entryPtr->highlightWidth != 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(entryPtr->highlightBgColorPtr, pixmap);
	if (entryPtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(entryPtr->highlightColorPtr, pixmap);
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, 
	            entryPtr->highlightWidth, pixmap);
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, 
	            entryPtr->highlightWidth, pixmap);
	}
    }

    /*
     * Everything's been redisplayed;  now copy the pixmap onto the screen
     * and free up the pixmap.
     */

    XCopyArea(entryPtr->display, pixmap, Tk_WindowId(tkwin), entryPtr->textGC,
	    0, 0, (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin),
	    0, 0);
    Tk_FreePixmap(entryPtr->display, pixmap);
    entryPtr->flags &= ~BORDER_NEEDED;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryComputeGeometry --
 *
 *	This procedure is invoked to recompute information about where
 *	in its window an entry's string will be displayed.  It also
 *	computes the requested size for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The leftX and tabOrigin fields are recomputed for entryPtr,
 *	and leftIndex may be adjusted.  Tk_GeometryRequest is called
 *	to register the desired dimensions for the window.
 *
 *----------------------------------------------------------------------
 */

static void
EntryComputeGeometry(entryPtr)
    Entry *entryPtr;		/* Widget record for entry. */
{
    int totalLength, overflow, maxOffScreen, rightX;
    int height, width, i;
    Tk_FontMetrics fm;
    char *p;

    if (entryPtr->displayString != entryPtr->string) {
	ckfree(entryPtr->displayString);
	entryPtr->displayString = entryPtr->string;
	entryPtr->numDisplayBytes = entryPtr->numBytes;
    }

    /*
     * If we're displaying a special character instead of the value of
     * the entry, recompute the displayString.
     */

    if (entryPtr->showChar != NULL) {
	Tcl_UniChar ch;
	char buf[TCL_UTF_MAX];
	int size;

	/*
	 * Normalize the special character so we can safely duplicate it
	 * in the display string.  If we didn't do this, then two malformed
	 * characters might end up looking like one valid UTF character in
	 * the resulting string.
	 */

	Tcl_UtfToUniChar(entryPtr->showChar, &ch);
	size = Tcl_UniCharToUtf(ch, buf);

	entryPtr->numDisplayBytes = entryPtr->numChars * size;
	entryPtr->displayString =
		(char *) ckalloc((unsigned) (entryPtr->numDisplayBytes + 1));

	p = entryPtr->displayString;
	for (i = entryPtr->numChars; --i >= 0; ) {
	    p += Tcl_UniCharToUtf(ch, p);
	}
	*p = '\0';
    }
    Tk_FreeTextLayout(entryPtr->textLayout);
    entryPtr->textLayout = Tk_ComputeTextLayout(entryPtr->tkfont,
	    entryPtr->displayString, entryPtr->numChars, 0,
	    entryPtr->justify, TK_IGNORE_NEWLINES, &totalLength, &height);

    entryPtr->layoutY = (Tk_Height(entryPtr->tkwin) - height) / 2;

    /*
     * Recompute where the leftmost character on the display will
     * be drawn (entryPtr->leftX) and adjust leftIndex if necessary
     * so that we don't let characters hang off the edge of the
     * window unless the entire window is full.
     */

    overflow = totalLength - (Tk_Width(entryPtr->tkwin) - 2*entryPtr->inset);
    if (overflow <= 0) {
	entryPtr->leftIndex = 0;
	if (entryPtr->justify == TK_JUSTIFY_LEFT) {
	    entryPtr->leftX = entryPtr->inset;
	} else if (entryPtr->justify == TK_JUSTIFY_RIGHT) {
	    entryPtr->leftX = Tk_Width(entryPtr->tkwin) - entryPtr->inset
		    - totalLength;
	} else {
	    entryPtr->leftX = (Tk_Width(entryPtr->tkwin) - totalLength)/2;
	}
	entryPtr->layoutX = entryPtr->leftX;
    } else {
	/*
	 * The whole string can't fit in the window.  Compute the
	 * maximum number of characters that may be off-screen to
	 * the left without leaving empty space on the right of the
	 * window, then don't let leftIndex be any greater than that.
	 */

	maxOffScreen = Tk_PointToChar(entryPtr->textLayout, overflow, 0);
	Tk_CharBbox(entryPtr->textLayout, maxOffScreen,
		&rightX, NULL, NULL, NULL);
	if (rightX < overflow) {
	    maxOffScreen++;
	}
	if (entryPtr->leftIndex > maxOffScreen) {
	    entryPtr->leftIndex = maxOffScreen;
	}
	Tk_CharBbox(entryPtr->textLayout, entryPtr->leftIndex, &rightX,
		NULL, NULL, NULL);
	entryPtr->leftX = entryPtr->inset;
	entryPtr->layoutX = entryPtr->leftX - rightX;
    }

    Tk_GetFontMetrics(entryPtr->tkfont, &fm);
    height = fm.linespace + 2*entryPtr->inset + 2*(YPAD-XPAD);
    if (entryPtr->prefWidth > 0) {
	width = entryPtr->prefWidth*entryPtr->avgWidth + 2*entryPtr->inset;
    } else {
	if (totalLength == 0) {
	    width = entryPtr->avgWidth + 2*entryPtr->inset;
	} else {
	    width = totalLength + 2*entryPtr->inset;
	}
    }
    Tk_GeometryRequest(entryPtr->tkwin, width, height);
}

/*
 *----------------------------------------------------------------------
 *
 * InsertChars --
 *
 *	Add new characters to an entry widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New information gets added to entryPtr;  it will be redisplayed
 *	soon, but not necessarily immediately.
 *
 *----------------------------------------------------------------------
 */

static void
InsertChars(entryPtr, index, value)
    Entry *entryPtr;		/* Entry that is to get the new elements. */
    int index;			/* Add the new elements before this
				 * character index. */
    char *value;		/* New characters to add (NULL-terminated
				 * string). */
{
    int byteIndex, byteCount, oldChars, charsAdded, newByteCount;
    char *new, *string;

    string = entryPtr->string;
    byteIndex = Tcl_UtfAtIndex(string, index) - string;
    byteCount = strlen(value);
    if (byteCount == 0) {
	return;
    }

    newByteCount = entryPtr->numBytes + byteCount + 1;
    new = (char *) ckalloc((unsigned) newByteCount);
    memcpy(new, string, (size_t) byteIndex);
    strcpy(new + byteIndex, value);
    strcpy(new + byteIndex + byteCount, string + byteIndex);

    if ((entryPtr->validate == VALIDATE_KEY ||
	 entryPtr->validate == VALIDATE_ALL) &&
	EntryValidateChange(entryPtr, value, new, index,
			    VALIDATE_INSERT) != TCL_OK) {
	ckfree(new);
	return;
    }

    ckfree(string);
    entryPtr->string = new;

    /*
     * The following construction is used because inserting improperly
     * formed UTF-8 sequences between other improperly formed UTF-8
     * sequences could result in actually forming valid UTF-8 sequences;
     * the number of characters added may not be Tcl_NumUtfChars(string, -1),
     * because of context.  The actual number of characters added is how
     * many characters are in the string now minus the number that
     * used to be there.
     */

    oldChars = entryPtr->numChars;
    entryPtr->numChars = Tcl_NumUtfChars(new, -1);
    charsAdded = entryPtr->numChars - oldChars;
    entryPtr->numBytes += byteCount;

    if (entryPtr->displayString == string) {
	entryPtr->displayString = new;
	entryPtr->numDisplayBytes = entryPtr->numBytes;
    }

    /*
     * Inserting characters invalidates all indexes into the string.
     * Touch up the indexes so that they still refer to the same
     * characters (at new positions).  When updating the selection
     * end-points, don't include the new text in the selection unless
     * it was completely surrounded by the selection.
     */

    if (entryPtr->selectFirst >= index) {
	entryPtr->selectFirst += charsAdded;
    }
    if (entryPtr->selectLast > index) {
	entryPtr->selectLast += charsAdded;
    }
    if ((entryPtr->selectAnchor > index)
	    || (entryPtr->selectFirst >= index)) {
	entryPtr->selectAnchor += charsAdded;
    }
    if (entryPtr->leftIndex > index) {
	entryPtr->leftIndex += charsAdded;
    }
    if (entryPtr->insertPos >= index) {
	entryPtr->insertPos += charsAdded;
    }
    EntryValueChanged(entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteChars --
 *
 *	Remove one or more characters from an entry widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed, the entry gets modified and (eventually)
 *	redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteChars(entryPtr, index, count)
    Entry *entryPtr;		/* Entry widget to modify. */
    int index;			/* Index of first character to delete. */
    int count;			/* How many characters to delete. */
{
    int byteIndex, byteCount, newByteCount;
    char *new, *string, *todelete;

    if ((index + count) > entryPtr->numChars) {
	count = entryPtr->numChars - index;
    }
    if (count <= 0) {
	return;
    }

    string = entryPtr->string;
    byteIndex = Tcl_UtfAtIndex(string, index) - string;
    byteCount = Tcl_UtfAtIndex(string + byteIndex, count) - (string + byteIndex);

    newByteCount = entryPtr->numBytes + 1 - byteCount;
    new = (char *) ckalloc((unsigned) newByteCount);
    memcpy(new, string, (size_t) byteIndex);
    strcpy(new + byteIndex, string + byteIndex + byteCount);

    todelete = (char *) ckalloc((unsigned) (byteCount + 1));
    memcpy(todelete, string + byteIndex, (size_t) byteCount);
    todelete[byteCount] = '\0';

    if ((entryPtr->validate == VALIDATE_KEY ||
	 entryPtr->validate == VALIDATE_ALL) &&
	EntryValidateChange(entryPtr, todelete, new, index,
			    VALIDATE_DELETE) != TCL_OK) {
	ckfree(new);
	ckfree(todelete);
	return;
    }

    ckfree(todelete);
    ckfree(entryPtr->string);
    entryPtr->string = new;
    entryPtr->numChars -= count;
    entryPtr->numBytes -= byteCount;

    if (entryPtr->displayString == string) {
	entryPtr->displayString = new;
	entryPtr->numDisplayBytes = entryPtr->numBytes;
    }

    /*
     * Deleting characters results in the remaining characters being
     * renumbered.  Update the various indexes into the string to reflect
     * this change.
     */

    if (entryPtr->selectFirst >= index) {
	if (entryPtr->selectFirst >= (index + count)) {
	    entryPtr->selectFirst -= count;
	} else {
	    entryPtr->selectFirst = index;
	}
    }
    if (entryPtr->selectLast >= index) {
	if (entryPtr->selectLast >= (index + count)) {
	    entryPtr->selectLast -= count;
	} else {
	    entryPtr->selectLast = index;
	}
    }
    if (entryPtr->selectLast <= entryPtr->selectFirst) {
	entryPtr->selectFirst = -1;
	entryPtr->selectLast = -1;
    }
    if (entryPtr->selectAnchor >= index) {
	if (entryPtr->selectAnchor >= (index+count)) {
	    entryPtr->selectAnchor -= count;
	} else {
	    entryPtr->selectAnchor = index;
	}
    }
    if (entryPtr->leftIndex > index) {
	if (entryPtr->leftIndex >= (index + count)) {
	    entryPtr->leftIndex -= count;
	} else {
	    entryPtr->leftIndex = index;
	}
    }
    if (entryPtr->insertPos >= index) {
	if (entryPtr->insertPos >= (index + count)) {
	    entryPtr->insertPos -= count;
	} else {
	    entryPtr->insertPos = index;
	}
    }
    EntryValueChanged(entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * EntryValueChanged --
 *
 *	This procedure is invoked when characters are inserted into
 *	an entry or deleted from it.  It updates the entry's associated
 *	variable, if there is one, and does other bookkeeping such
 *	as arranging for redisplay.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
EntryValueChanged(entryPtr)
    Entry *entryPtr;		/* Entry whose value just changed. */
{
    char *newValue;

    if (entryPtr->textVarName == NULL) {
	newValue = NULL;
    } else {
	newValue = Tcl_SetVar(entryPtr->interp, entryPtr->textVarName,
		entryPtr->string, TCL_GLOBAL_ONLY);
    }

    if ((newValue != NULL) && (strcmp(newValue, entryPtr->string) != 0)) {
	/*
	 * The value of the variable is different than what we asked for.
	 * This means that a trace on the variable modified it.  In this
	 * case our trace procedure wasn't invoked since the modification
	 * came while a trace was already active on the variable.  So,
	 * update our value to reflect the variable's latest value.
	 */

	EntrySetValue(entryPtr, newValue);
    } else {
	/*
	 * Arrange for redisplay.
	 */

	entryPtr->flags |= UPDATE_SCROLLBAR;
	EntryComputeGeometry(entryPtr);
	EventuallyRedraw(entryPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EntrySetValue --
 *
 *	Replace the contents of a text entry with a given value.  This
 *	procedure is invoked when updating the entry from the entry's
 *	associated variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The string displayed in the entry will change.  The selection,
 *	insertion point, and view may have to be adjusted to keep them
 *	within the bounds of the new string.  Note: this procedure does
 *	*not* update the entry's associated variable, since that could
 *	result in an infinite loop.
 *
 *----------------------------------------------------------------------
 */

static void
EntrySetValue(entryPtr, value)
    Entry *entryPtr;		/* Entry whose value is to be changed. */
    char *value;		/* New text to display in entry. */
{
    char *oldSource;
    int code, valueLen, malloced = 0;

    if (strcmp(value, entryPtr->string) == 0) {
	return;
    }
    valueLen = strlen(value);

    if (entryPtr->flags & VALIDATE_VAR) {
	entryPtr->flags |= VALIDATE_ABORT;
    } else {
	/*
	 * If we validate, we create a copy of the value, as it may
	 * point to volatile memory, like the value of the -textvar
	 * which may get freed during validation
	 */
	oldSource = (char *) ckalloc((unsigned) (valueLen + 1));
	strcpy(oldSource, value);
	value = oldSource;
	malloced = 1;

	entryPtr->flags |= VALIDATE_VAR;
	code = EntryValidateChange(entryPtr, (char *) NULL, value, -1,
		VALIDATE_FORCED);
	entryPtr->flags &= ~VALIDATE_VAR;
	/*
	 * If VALIDATE_ABORT has been set, then this operation should be
	 * aborted because the validatecommand did something else instead
	 */
	if (entryPtr->flags & VALIDATE_ABORT) {
	    entryPtr->flags &= ~VALIDATE_ABORT;
	    ckfree(value);
	    return;
	}
    }

    oldSource = entryPtr->string;
    ckfree(entryPtr->string);

    if (malloced) {
	entryPtr->string = value;
    } else {
	entryPtr->string   = (char *) ckalloc((unsigned) (valueLen + 1));
	strcpy(entryPtr->string, value);
    }
    entryPtr->numBytes = valueLen;
    entryPtr->numChars = Tcl_NumUtfChars(value, valueLen);

    if (entryPtr->displayString == oldSource) {
	entryPtr->displayString = entryPtr->string;
	entryPtr->numDisplayBytes = entryPtr->numBytes;
    }

    if (entryPtr->selectFirst >= 0) {
	if (entryPtr->selectFirst >= entryPtr->numChars) {
	    entryPtr->selectFirst = -1;
	    entryPtr->selectLast = -1;
	} else if (entryPtr->selectLast > entryPtr->numChars) {
	    entryPtr->selectLast = entryPtr->numChars;
	}
    }
    if (entryPtr->leftIndex >= entryPtr->numChars) {
	if (entryPtr->numChars > 0) {
	    entryPtr->leftIndex = entryPtr->numChars - 1;
	} else {
	    entryPtr->leftIndex = 0;
	}
    }
    if (entryPtr->insertPos > entryPtr->numChars) {
	entryPtr->insertPos = entryPtr->numChars;
    }

    entryPtr->flags |= UPDATE_SCROLLBAR;
    EntryComputeGeometry(entryPtr);
    EventuallyRedraw(entryPtr);
}

/*
 *--------------------------------------------------------------
 *
 * EntryEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on entryes.
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
EntryEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    Entry *entryPtr = (Entry *) clientData;
    if (eventPtr->type == Expose) {
	EventuallyRedraw(entryPtr);
	entryPtr->flags |= BORDER_NEEDED;
    } else if (eventPtr->type == DestroyNotify) {
        DestroyEntry((char *) clientData);
    } else if (eventPtr->type == ConfigureNotify) {
	Tcl_Preserve((ClientData) entryPtr);
	entryPtr->flags |= UPDATE_SCROLLBAR;
	EntryComputeGeometry(entryPtr);
	EventuallyRedraw(entryPtr);
	Tcl_Release((ClientData) entryPtr);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    EntryFocusProc(entryPtr, 1);
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    EntryFocusProc(entryPtr, 0);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EntryCmdDeletedProc --
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
EntryCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    Entry *entryPtr = (Entry *) clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (!(entryPtr->flags & ENTRY_DELETED)) {
        Tk_DestroyWindow(entryPtr->tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryIndex --
 *
 *	Parse an index into an entry and return either its value
 *	or an error.
 *
 * Results:
 *	A standard Tcl result.  If all went well, then *indexPtr is
 *	filled in with the character index (into entryPtr) corresponding to
 *	string.  The index value is guaranteed to lie between 0 and
 *	the number of characters in the string, inclusive.  If an
 *	error occurs then an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
GetEntryIndex(interp, entryPtr, string, indexPtr)
    Tcl_Interp *interp;		/* For error messages. */
    Entry *entryPtr;		/* Entry for which the index is being
				 * specified. */
    char *string;		/* Specifies character in entryPtr. */
    int *indexPtr;		/* Where to store converted character
				 * index. */
{
    size_t length;

    length = strlen(string);

    if (string[0] == 'a') {
	if (strncmp(string, "anchor", length) == 0) {
	    *indexPtr = entryPtr->selectAnchor;
	} else {
	    badIndex:

	    /*
	     * Some of the paths here leave messages in the interp's result,
	     * so we have to clear it out before storing our own message.
	     */

	    Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
	    Tcl_AppendResult(interp, "bad entry index \"", string,
		    "\"", (char *) NULL);
	    return TCL_ERROR;
	}
    } else if (string[0] == 'e') {
	if (strncmp(string, "end", length) == 0) {
	    *indexPtr = entryPtr->numChars;
	} else {
	    goto badIndex;
	}
    } else if (string[0] == 'i') {
	if (strncmp(string, "insert", length) == 0) {
	    *indexPtr = entryPtr->insertPos;
	} else {
	    goto badIndex;
	}
    } else if (string[0] == 's') {
	if (entryPtr->selectFirst < 0) {
	    Tcl_SetResult(interp, "selection isn't in entry", TCL_STATIC);
	    return TCL_ERROR;
	}
	if (length < 5) {
	    goto badIndex;
	}
	if (strncmp(string, "sel.first", length) == 0) {
	    *indexPtr = entryPtr->selectFirst;
	} else if (strncmp(string, "sel.last", length) == 0) {
	    *indexPtr = entryPtr->selectLast;
	} else {
	    goto badIndex;
	}
    } else if (string[0] == '@') {
	int x, roundUp;

	if (Tcl_GetInt(interp, string + 1, &x) != TCL_OK) {
	    goto badIndex;
	}
	if (x < entryPtr->inset) {
	    x = entryPtr->inset;
	}
	roundUp = 0;
	if (x >= (Tk_Width(entryPtr->tkwin) - entryPtr->inset)) {
	    x = Tk_Width(entryPtr->tkwin) - entryPtr->inset - 1;
	    roundUp = 1;
	}
	*indexPtr = Tk_PointToChar(entryPtr->textLayout,
		x - entryPtr->layoutX, 0);

	/*
	 * Special trick:  if the x-position was off-screen to the right,
	 * round the index up to refer to the character just after the
	 * last visible one on the screen.  This is needed to enable the
	 * last character to be selected, for example.
	 */

	if (roundUp && (*indexPtr < entryPtr->numChars)) {
	    *indexPtr += 1;
	}
    } else {
	if (Tcl_GetInt(interp, string, indexPtr) != TCL_OK) {
	    goto badIndex;
	}
	if (*indexPtr < 0){
	    *indexPtr = 0;
	} else if (*indexPtr > entryPtr->numChars) {
	    *indexPtr = entryPtr->numChars;
	} 
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryScanTo --
 *
 *	Given a y-coordinate (presumably of the curent mouse location)
 *	drag the view in the window to implement the scan operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The view in the window may change.
 *
 *----------------------------------------------------------------------
 */

static void
EntryScanTo(entryPtr, x)
    Entry *entryPtr;		/* Information about widget. */
    int x;			/* X-coordinate to use for scan operation. */
{
    int newLeftIndex;

    /*
     * Compute new leftIndex for entry by amplifying the difference
     * between the current position and the place where the scan
     * started (the "mark" position).  If we run off the left or right
     * side of the entry, then reset the mark point so that the current
     * position continues to correspond to the edge of the window.
     * This means that the picture will start dragging as soon as the
     * mouse reverses direction (without this reset, might have to slide
     * mouse a long ways back before the picture starts moving again).
     */

    newLeftIndex = entryPtr->scanMarkIndex
	    - (10 * (x - entryPtr->scanMarkX)) / entryPtr->avgWidth;
    if (newLeftIndex >= entryPtr->numChars) {
	newLeftIndex = entryPtr->scanMarkIndex = entryPtr->numChars - 1;
	entryPtr->scanMarkX = x;
    }
    if (newLeftIndex < 0) {
	newLeftIndex = entryPtr->scanMarkIndex = 0;
	entryPtr->scanMarkX = x;
    } 

    if (newLeftIndex != entryPtr->leftIndex) {
	entryPtr->leftIndex = newLeftIndex;
	entryPtr->flags |= UPDATE_SCROLLBAR;
	EntryComputeGeometry(entryPtr);
	if (newLeftIndex != entryPtr->leftIndex) {
	    entryPtr->scanMarkIndex = entryPtr->leftIndex;
	    entryPtr->scanMarkX = x;
	}
	EventuallyRedraw(entryPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EntrySelectTo --
 *
 *	Modify the selection by moving its un-anchored end.  This could
 *	make the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */

static void
EntrySelectTo(entryPtr, index)
    Entry *entryPtr;		/* Information about widget. */
    int index;			/* Character index of element that is to
				 * become the "other" end of the selection. */
{
    int newFirst, newLast;

    /*
     * Grab the selection if we don't own it already.
     */

    if (!(entryPtr->flags & GOT_SELECTION) && (entryPtr->exportSelection)) {
	Tk_OwnSelection(entryPtr->tkwin, XA_PRIMARY, EntryLostSelection,
		(ClientData) entryPtr);
	entryPtr->flags |= GOT_SELECTION;
    }

    /*
     * Pick new starting and ending points for the selection.
     */

    if (entryPtr->selectAnchor > entryPtr->numChars) {
	entryPtr->selectAnchor = entryPtr->numChars;
    }
    if (entryPtr->selectAnchor <= index) {
	newFirst = entryPtr->selectAnchor;
	newLast = index;
    } else {
	newFirst = index;
	newLast = entryPtr->selectAnchor;
	if (newLast < 0) {
	    newFirst = newLast = -1;
	}
    }
    if ((entryPtr->selectFirst == newFirst)
	    && (entryPtr->selectLast == newLast)) {
	return;
    }
    entryPtr->selectFirst = newFirst;
    entryPtr->selectLast = newLast;
    EventuallyRedraw(entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * EntryFetchSelection --
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
EntryFetchSelection(clientData, offset, buffer, maxBytes)
    ClientData clientData;	/* Information about entry widget. */
    int offset;			/* Byte offset within selection of first
				 * character to be returned. */
    char *buffer;		/* Location in which to place selection. */
    int maxBytes;		/* Maximum number of bytes to place at
				 * buffer, not including terminating NULL
				 * character. */
{
    Entry *entryPtr = (Entry *) clientData;
    int byteCount;
    char *string, *selStart, *selEnd;

    if ((entryPtr->selectFirst < 0) || !(entryPtr->exportSelection)) {
	return -1;
    }
    string = entryPtr->displayString;
    selStart = Tcl_UtfAtIndex(string, entryPtr->selectFirst);
    selEnd = Tcl_UtfAtIndex(selStart,
	    entryPtr->selectLast - entryPtr->selectFirst);
    byteCount = selEnd - selStart - offset;
    if (byteCount > maxBytes) {
	byteCount = maxBytes;
    }
    if (byteCount <= 0) {
	return 0;
    }
    memcpy(buffer, selStart + offset, (size_t) byteCount);
    buffer[byteCount] = '\0';
    return byteCount;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryLostSelection --
 *
 *	This procedure is called back by Tk when the selection is
 *	grabbed away from an entry widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is
 *	marked as not containing a selection.
 *
 *----------------------------------------------------------------------
 */

static void
EntryLostSelection(clientData)
    ClientData clientData;	/* Information about entry widget. */
{
    Entry *entryPtr = (Entry *) clientData;

    entryPtr->flags &= ~GOT_SELECTION;

    /*
     * On Windows and Mac systems, we want to remember the selection
     * for the next time the focus enters the window.  On Unix, we need
     * to clear the selection since it is always visible.
     */

#ifdef ALWAYS_SHOW_SELECTION
    if ((entryPtr->selectFirst >= 0) && entryPtr->exportSelection) {
	entryPtr->selectFirst = -1;
	entryPtr->selectLast = -1;
	EventuallyRedraw(entryPtr);
    }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *	Ensure that an entry is eventually redrawn on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets redisplayed.  Right now we don't do selective
 *	redisplays:  the whole window will be redrawn.  This doesn't
 *	seem to hurt performance noticeably, but if it does then this
 *	could be changed.
 *
 *----------------------------------------------------------------------
 */

static void
EventuallyRedraw(entryPtr)
    Entry *entryPtr;		/* Information about widget. */
{
    if ((entryPtr->tkwin == NULL) || !Tk_IsMapped(entryPtr->tkwin)) {
	return;
    }

    /*
     * Right now we don't do selective redisplays:  the whole window
     * will be redrawn.  This doesn't seem to hurt performance noticeably,
     * but if it does then this could be changed.
     */

    if (!(entryPtr->flags & REDRAW_PENDING)) {
	entryPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayEntry, (ClientData) entryPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EntryVisibleRange --
 *
 *	Return information about the range of the entry that is
 *	currently visible.
 *
 * Results:
 *	*firstPtr and *lastPtr are modified to hold fractions between
 *	0 and 1 identifying the range of characters visible in the
 *	entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
EntryVisibleRange(entryPtr, firstPtr, lastPtr)
    Entry *entryPtr;		/* Information about widget. */
    double *firstPtr;		/* Return position of first visible
				 * character in widget. */
    double *lastPtr;		/* Return position of char just after last
				 * visible one. */
{
    int charsInWindow;

    if (entryPtr->numChars == 0) {
	*firstPtr = 0.0;
	*lastPtr = 1.0;
    } else {
	charsInWindow = Tk_PointToChar(entryPtr->textLayout,
		Tk_Width(entryPtr->tkwin) - entryPtr->inset
			- entryPtr->layoutX - 1, 0);
	if (charsInWindow < entryPtr->numChars) {
	    charsInWindow++;
	}
	charsInWindow -= entryPtr->leftIndex;
	if (charsInWindow == 0) {
	    charsInWindow = 1;
	}

	*firstPtr = (double) entryPtr->leftIndex / entryPtr->numChars;
	*lastPtr = (double) (entryPtr->leftIndex + charsInWindow)
		/ entryPtr->numChars;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EntryUpdateScrollbar --
 *
 *	This procedure is invoked whenever information has changed in
 *	an entry in a way that would invalidate a scrollbar display.
 *	If there is an associated scrollbar, then this procedure updates
 *	it by invoking a Tcl command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional command may be
 *	invoked to process errors in the command.
 *
 *----------------------------------------------------------------------
 */

static void
EntryUpdateScrollbar(entryPtr)
    Entry *entryPtr;			/* Information about widget. */
{
    char args[TCL_DOUBLE_SPACE * 2];
    int code;
    double first, last;
    Tcl_Interp *interp;

    if (entryPtr->scrollCmd == NULL) {
	return;
    }

    interp = entryPtr->interp;
    Tcl_Preserve((ClientData) interp);
    EntryVisibleRange(entryPtr, &first, &last);
    sprintf(args, " %g %g", first, last);
    code = Tcl_VarEval(interp, entryPtr->scrollCmd, args, (char *) NULL);
    if (code != TCL_OK) {
	Tcl_AddErrorInfo(interp,
		"\n    (horizontal scrolling command executed by entry)");
	Tcl_BackgroundError(interp);
    }
    Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
    Tcl_Release((ClientData) interp);
}

/*
 *----------------------------------------------------------------------
 *
 * EntryBlinkProc --
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
EntryBlinkProc(clientData)
    ClientData clientData;	/* Pointer to record describing entry. */
{
    Entry *entryPtr = (Entry *) clientData;

    if ((entryPtr->state == STATE_DISABLED) ||
	    !(entryPtr->flags & GOT_FOCUS) || (entryPtr->insertOffTime == 0)) {
	return;
    }
    if (entryPtr->flags & CURSOR_ON) {
	entryPtr->flags &= ~CURSOR_ON;
	entryPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		entryPtr->insertOffTime, EntryBlinkProc, (ClientData) entryPtr);
    } else {
	entryPtr->flags |= CURSOR_ON;
	entryPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		entryPtr->insertOnTime, EntryBlinkProc, (ClientData) entryPtr);
    }
    EventuallyRedraw(entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * EntryFocusProc --
 *
 *	This procedure is called whenever the entry gets or loses the
 *	input focus.  It's also called whenever the window is reconfigured
 *	while it has the focus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off.
 *
 *----------------------------------------------------------------------
 */

static void
EntryFocusProc(entryPtr, gotFocus)
    Entry *entryPtr;		/* Entry that got or lost focus. */
    int gotFocus;		/* 1 means window is getting focus, 0 means
				 * it's losing it. */
{
    Tcl_DeleteTimerHandler(entryPtr->insertBlinkHandler);
    if (gotFocus) {
	entryPtr->flags |= GOT_FOCUS | CURSOR_ON;
	if (entryPtr->insertOffTime != 0) {
	    entryPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		    entryPtr->insertOnTime, EntryBlinkProc,
		    (ClientData) entryPtr);
	}
	if (entryPtr->validate == VALIDATE_ALL ||
	    entryPtr->validate == VALIDATE_FOCUS ||
	    entryPtr->validate == VALIDATE_FOCUSIN) {
	    EntryValidateChange(entryPtr, (char *) NULL,
				entryPtr->string, -1, VALIDATE_FOCUSIN);
	}
    } else {
	entryPtr->flags &= ~(GOT_FOCUS | CURSOR_ON);
	entryPtr->insertBlinkHandler = (Tcl_TimerToken) NULL;
	if (entryPtr->validate == VALIDATE_ALL ||
	    entryPtr->validate == VALIDATE_FOCUS ||
	    entryPtr->validate == VALIDATE_FOCUSOUT) {
	    EntryValidateChange(entryPtr, (char *) NULL,
				entryPtr->string, -1, VALIDATE_FOCUSOUT);
	}
    }
    EventuallyRedraw(entryPtr);
}

/*
 *--------------------------------------------------------------
 *
 * EntryTextVarProc --
 *
 *	This procedure is invoked when someone changes the variable
 *	whose contents are to be displayed in an entry.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The text displayed in the entry will change to match the
 *	variable.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
EntryTextVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about button. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Not used. */
    char *name2;		/* Not used. */
    int flags;			/* Information about what happened. */
{
    Entry *entryPtr = (Entry *) clientData;
    char *value;

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_SetVar(interp, entryPtr->textVarName, entryPtr->string,
		    TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, entryPtr->textVarName,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    EntryTextVarProc, clientData);
	}
	return (char *) NULL;
    }

    /*
     * Update the entry's text with the value of the variable, unless
     * the entry already has that value (this happens when the variable
     * changes value because we changed it because someone typed in
     * the entry).
     */

    value = Tcl_GetVar(interp, entryPtr->textVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    EntrySetValue(entryPtr, value);
    return (char *) NULL;
}

/*
 *--------------------------------------------------------------
 *
 * EntryValidate --
 *
 *	This procedure is invoked when any character is added or
 *	removed from the entry widget, or a focus has trigerred validation.
 *
 * Results:
 *	TCL_OK if the validatecommand passes the new string.
 *      TCL_BREAK if the vcmd executed OK, but rejects the string.
 *      TCL_ERROR if an error occurred while executing the vcmd
 *      or a valid Tcl_Bool is not returned.
 *
 * Side effects:
 *      An error condition may arise
 *
 *--------------------------------------------------------------
 */

static int
EntryValidate(entryPtr, cmd)
     register Entry *entryPtr;	/* Entry that needs validation. */
     register char *cmd;	/* Validation command (NULL-terminated
				 * string). */
{
    register Tcl_Interp *interp = entryPtr->interp;
    int code, bool;

    code = Tcl_EvalEx(interp, cmd, -1, TCL_EVAL_GLOBAL | TCL_EVAL_DIRECT);

    if (code != TCL_OK && code != TCL_RETURN) {
	Tcl_AddErrorInfo(interp,
			 "\n\t(in validation command executed by entry)");
	Tcl_BackgroundError(interp);
	return TCL_ERROR;
    }

    if (Tcl_GetBooleanFromObj(interp, Tcl_GetObjResult(interp),
			      &bool) != TCL_OK) {
	Tcl_AddErrorInfo(interp,
		 "\nvalid boolean not returned by validation command");
	Tcl_BackgroundError(interp);
	Tcl_SetResult(interp, NULL, 0);
	return TCL_ERROR;
    }

    Tcl_SetResult(interp, NULL, 0);
    return (bool ? TCL_OK : TCL_BREAK);
}

/*
 *--------------------------------------------------------------
 *
 * EntryValidateChange --
 *
 *	This procedure is invoked when any character is added or
 *	removed from the entry widget, or a focus has trigerred validation.
 *
 * Results:
 *	TCL_OK if the validatecommand accepts the new string,
 *      TCL_ERROR if any problems occured with validatecommand.
 *
 * Side effects:
 *      The insertion/deletion may be aborted, and the
 *      validatecommand might turn itself off (if an error
 *      or loop condition arises).
 *
 *--------------------------------------------------------------
 */

static int
EntryValidateChange(entryPtr, change, new, index, type)
     register Entry *entryPtr;	/* Entry that needs validation. */
     char *change;		/* Characters to be added/deleted
				 * (NULL-terminated string). */
     char *new;                 /* Potential new value of entry string */
     int index;                 /* index of insert/delete, -1 otherwise */
     int type;                  /* forced, delete, insert,
				 * focusin or focusout */
{
    int code, varValidate = (entryPtr->flags & VALIDATE_VAR);
    char *p;
    Tcl_DString script;
    
    if (entryPtr->validateCmd == NULL ||
	entryPtr->validate == VALIDATE_NONE) {
	return (varValidate ? TCL_ERROR : TCL_OK);
    }

    /*
     * If we're already validating, then we're hitting a loop condition
     * Return and set validate to 0 to disallow further validations
     * and prevent current validation from finishing
     */
    if (entryPtr->flags & VALIDATING) {
	entryPtr->validate = VALIDATE_NONE;
	return (varValidate ? TCL_ERROR : TCL_OK);
    }

    entryPtr->flags |= VALIDATING;

    /*
     * Now form command string and run through the -validatecommand
     */

    Tcl_DStringInit(&script);
    ExpandPercents(entryPtr, entryPtr->validateCmd,
	    change, new, index, type, &script);
    Tcl_DStringAppend(&script, "", 1);

    p = Tcl_DStringValue(&script);
    code = EntryValidate(entryPtr, p);
    Tcl_DStringFree(&script);

    /*
     * If e->validate has become VALIDATE_NONE during the validation, or
     * we now have VALIDATE_VAR set (from EntrySetValue) and didn't before,
     * it means that a loop condition almost occured.  Do not allow
     * this validation result to finish.
     */
    if (entryPtr->validate == VALIDATE_NONE
	    || (!varValidate && (entryPtr->flags & VALIDATE_VAR))) {
	code = TCL_ERROR;
    }
    /*
     * If validate will return ERROR, then disallow further validations
     * Otherwise, if it didn't accept the new string (returned TCL_BREAK)
     * then eval the invalidCmd (if it's set)
     */
    if (code == TCL_ERROR) {
	entryPtr->validate = VALIDATE_NONE;
    } else if (code == TCL_BREAK) {
	/*
	 * If we were doing forced validation (like via a variable
	 * trace) and the command returned 0, the we turn off validation
	 * because we assume that textvariables have precedence in
	 * managing the value.  We also don't call the invcmd, as it
	 * may want to do entry manipulation which the setting of the
	 * var will later wipe anyway.
	 */
	if (varValidate) {
	    entryPtr->validate = VALIDATE_NONE;
	} else if (entryPtr->invalidCmd != NULL) {
	    Tcl_DStringInit(&script);
	    ExpandPercents(entryPtr, entryPtr->invalidCmd,
			   change, new, index, type, &script);
	    Tcl_DStringAppend(&script, "", 1);
	    p = Tcl_DStringValue(&script);
	    if (Tcl_EvalEx(entryPtr->interp, p, -1,
		    TCL_EVAL_GLOBAL | TCL_EVAL_DIRECT) != TCL_OK) {
		Tcl_AddErrorInfo(entryPtr->interp,
				 "\n\t(in invalidcommand executed by entry)");
		Tcl_BackgroundError(entryPtr->interp);
		code = TCL_ERROR;
		entryPtr->validate = VALIDATE_NONE;
	    }
	    Tcl_DStringFree(&script);
	}
    }

    entryPtr->flags &= ~VALIDATING;

    return code;
}

/*
 *--------------------------------------------------------------
 *
 * ExpandPercents --
 *
 *	Given a command and an event, produce a new command
 *	by replacing % constructs in the original command
 *	with information from the X event.
 *
 * Results:
 *	The new expanded command is appended to the dynamic string
 *	given by dsPtr.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
ExpandPercents(entryPtr, before, change, new, index, type, dsPtr)
     register Entry *entryPtr;	/* Entry that needs validation. */
     register char *before;	/* Command containing percent
				 * expressions to be replaced. */
     char *change;		/* Characters to added/deleted
				 * (NULL-terminated string). */
     char *new;			/* Potential new value of entry string */
     int index;			/* index of insert/delete */
     int type;			/* INSERT or DELETE */
     Tcl_DString *dsPtr;	/* Dynamic string in which to append
				 * new command. */
{
    int spaceNeeded, cvtFlags;	/* Used to substitute string as proper Tcl
				 * list element. */
    int number, length;
    register char *string;
    Tcl_UniChar ch;
    char numStorage[2*TCL_INTEGER_SPACE];

    while (1) {
	if (*before == '\0') {
	    break;
	}
	/*
	 * Find everything up to the next % character and append it
	 * to the result string.
	 */

	string = before;
	/* No need to convert '%', as it is in ascii range */
	string = Tcl_UtfFindFirst(before, '%');
	if (string == (char *) NULL) {
	    Tcl_DStringAppend(dsPtr, before, -1);
	    break;
	} else if (string != before) {
	    Tcl_DStringAppend(dsPtr, before, string-before);
	    before = string;
	}

	/*
	 * There's a percent sequence here.  Process it.
	 */

	before++; /* skip over % */
	if (*before != '\0') {
	    before += Tcl_UtfToUniChar(before, &ch);
	} else {
	    ch = '%';
	}
	switch (ch) {
	case 'd': /* Type of call that caused validation */
	    switch (type) {
	    case VALIDATE_INSERT:
		number = 1;
		break;
	    case VALIDATE_DELETE:
		number = 0;
		break;
	    default:
		number = -1;
		break;
	    }
	    sprintf(numStorage, "%d", number);
	    string = numStorage;
	    break;
	case 'i': /* index of insert/delete */
	    sprintf(numStorage, "%d", index);
	    string = numStorage;
	    break;
	case 'P': /* 'Peeked' new value of the string */
	    string = new;
	    break;
	case 's': /* Current string value of entry */
	    string = entryPtr->string;
	    break;
	case 'S': /* string to be inserted/deleted, if any */
	    string = change;
	    break;
	case 'v': /* type of validation currently set */
	    string = validateStrings[entryPtr->validate];
	    break;
	case 'V': /* type of validation in effect */
	    switch (type) {
	    case VALIDATE_INSERT:
	    case VALIDATE_DELETE:
		string = validateStrings[VALIDATE_KEY];
		break;
	    case VALIDATE_FORCED:
		string = "forced";
		break;
	    default:
		string = validateStrings[type];
		break;
	    }
	    break;
	case 'W': /* widget name */
	    string = Tk_PathName(entryPtr->tkwin);
	    break;
	default:
	    length = Tcl_UniCharToUtf(ch, numStorage);
	    numStorage[length] = '\0';
	    string = numStorage;
	    break;
	}

	spaceNeeded = Tcl_ScanCountedElement(string, -1, &cvtFlags);
	length = Tcl_DStringLength(dsPtr);
	Tcl_DStringSetLength(dsPtr, length + spaceNeeded);
	spaceNeeded = Tcl_ConvertCountedElement(string, -1,
		Tcl_DStringValue(dsPtr) + length,
		cvtFlags | TCL_DONT_USE_BRACES);
	Tcl_DStringSetLength(dsPtr, length + spaceNeeded);
    }
}
