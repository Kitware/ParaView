/* 
 * tkFrame.c --
 *
 *	This module implements "frame"  and "toplevel" widgets for
 *	the Tk toolkit.  Frames are windows with a background color
 *	and possibly a 3-D effect, but not much else in the way of
 *	attributes.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "default.h"
#include "tkPort.h"
#include "tkInt.h"

/*
 * A data structure of the following type is kept for each
 * frame that currently exists for this process:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the frame.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up. */
    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with widget.  Used
				 * to delete widget command. */
    Tcl_Command widgetCmd;	/* Token for frame's widget command. */
    char *className;		/* Class name for widget (from configuration
				 * option).  Malloc-ed. */
    int mask;			/* Either FRAME or TOPLEVEL;  used to select
				 * which configuration options are valid for
				 * widget. */
    char *screenName;		/* Screen on which widget is created.  Non-null
				 * only for top-levels.  Malloc-ed, may be
				 * NULL. */
    char *visualName;		/* Textual description of visual for window,
				 * from -visual option.  Malloc-ed, may be
				 * NULL. */
    char *colormapName;		/* Textual description of colormap for window,
				 * from -colormap option.  Malloc-ed, may be
				 * NULL. */
    char *menuName;		/* Textual description of menu to use for
				 * menubar. Malloc-ed, may be NULL. */
    Colormap colormap;		/* If not None, identifies a colormap
				 * allocated for this window, which must be
				 * freed when the window is deleted. */
    Tk_3DBorder border;		/* Structure used to draw 3-D border and
				 * background.  NULL means no background
				 * or border. */
    int borderWidth;		/* Width of 3-D border (if any). */
    int relief;			/* 3-d effect: TK_RELIEF_RAISED etc. */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * 0 means don't draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight
				 * area when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int width;			/* Width to request for window.  <= 0 means
				 * don't request any size. */
    int height;			/* Height to request for window.  <= 0 means
				 * don't request any size. */
    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    int isContainer;		/* 1 means this window is a container, 0 means
				 * that it isn't. */
    char *useThis;		/* If the window is embedded, this points to
				 * the name of the window in which it is
				 * embedded (malloc'ed).  For non-embedded
				 * windows this is NULL. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} Frame;

/*
 * Flag bits for frames:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * GOT_FOCUS:			Non-zero means this widget currently
 *				has the input focus.
 */

#define REDRAW_PENDING		1
#define GOT_FOCUS		4

/*
 * The following flag bits are used so that there can be separate
 * defaults for some configuration options for frames and toplevels.
 */

#define FRAME		TK_CONFIG_USER_BIT
#define TOPLEVEL	(TK_CONFIG_USER_BIT << 1)
#define BOTH		(FRAME | TOPLEVEL)

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_FRAME_BG_COLOR, Tk_Offset(Frame, border),
	BOTH|TK_CONFIG_COLOR_ONLY|TK_CONFIG_NULL_OK},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_FRAME_BG_MONO, Tk_Offset(Frame, border),
	BOTH|TK_CONFIG_MONO_ONLY|TK_CONFIG_NULL_OK},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
	(char *) NULL, 0, BOTH},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
	(char *) NULL, 0, BOTH},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_FRAME_BORDER_WIDTH, Tk_Offset(Frame, borderWidth), BOTH},
    {TK_CONFIG_STRING, "-class", "class", "Class",
	DEF_FRAME_CLASS, Tk_Offset(Frame, className), FRAME},
    {TK_CONFIG_STRING, "-class", "class", "Class",
	DEF_TOPLEVEL_CLASS, Tk_Offset(Frame, className), TOPLEVEL},
    {TK_CONFIG_STRING, "-colormap", "colormap", "Colormap",
	DEF_FRAME_COLORMAP, Tk_Offset(Frame, colormapName),
	BOTH|TK_CONFIG_NULL_OK},
    {TK_CONFIG_BOOLEAN, "-container", "container", "Container",
	DEF_FRAME_CONTAINER, Tk_Offset(Frame, isContainer), BOTH},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_FRAME_CURSOR, Tk_Offset(Frame, cursor), BOTH|TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
	DEF_FRAME_HEIGHT, Tk_Offset(Frame, height), BOTH},
    {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_FRAME_HIGHLIGHT_BG,
	Tk_Offset(Frame, highlightBgColorPtr), BOTH},
    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_FRAME_HIGHLIGHT, Tk_Offset(Frame, highlightColorPtr), BOTH},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_FRAME_HIGHLIGHT_WIDTH, Tk_Offset(Frame, highlightWidth), BOTH},
    {TK_CONFIG_STRING, "-menu", "menu", "Menu",
	DEF_TOPLEVEL_MENU, Tk_Offset(Frame, menuName),
	TOPLEVEL|TK_CONFIG_NULL_OK},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_FRAME_RELIEF, Tk_Offset(Frame, relief), BOTH},
    {TK_CONFIG_STRING, "-screen", "screen", "Screen",
	DEF_TOPLEVEL_SCREEN, Tk_Offset(Frame, screenName),
	TOPLEVEL|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_FRAME_TAKE_FOCUS, Tk_Offset(Frame, takeFocus),
	BOTH|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-use", "use", "Use",
	DEF_FRAME_USE, Tk_Offset(Frame, useThis), TOPLEVEL|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-visual", "visual", "Visual",
	DEF_FRAME_VISUAL, Tk_Offset(Frame, visualName),
	BOTH|TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	DEF_FRAME_WIDTH, Tk_Offset(Frame, width), BOTH},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ConfigureFrame _ANSI_ARGS_((Tcl_Interp *interp,
			    Frame *framePtr, int argc, char **argv,
			    int flags));
static void		DestroyFrame _ANSI_ARGS_((char *memPtr));
static void		DisplayFrame _ANSI_ARGS_((ClientData clientData));
static void		FrameCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		FrameEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		FrameWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
static void		MapFrame _ANSI_ARGS_((ClientData clientData));

/*
 *--------------------------------------------------------------
 *
 * Tk_FrameCmd, Tk_ToplevelCmd --
 *
 *	These procedures are invoked to process the "frame" and
 *	"toplevel" Tcl commands.  See the user documentation for
 *	details on what they do.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.  These procedures are just wrappers;
 *	they call ButtonCreate to do all of the real work.
 *
 *--------------------------------------------------------------
 */

int
Tk_FrameCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return TkCreateFrame(clientData, interp, argc, argv, 0, (char *) NULL);
}

int
Tk_ToplevelCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return TkCreateFrame(clientData, interp, argc, argv, 1, (char *) NULL);
}

/*
 *--------------------------------------------------------------
 *
 * TkFrameCreate --
 *
 *	This procedure is invoked to process the "frame" and "toplevel"
 *	Tcl commands;  it is also invoked directly by Tk_Init to create
 *	a new main window.  See the user documentation for the "frame"
 *	and "toplevel" commands for details on what it does.
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
TkCreateFrame(clientData, interp, argc, argv, toplevel, appName)
    ClientData clientData;	/* Main window associated with interpreter.
				 * If we're called by Tk_Init to create a
				 * new application, then this is NULL. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
    int toplevel;		/* Non-zero means create a toplevel window,
				 * zero means create a frame. */
    char *appName;		/* Should only be non-NULL if clientData is
				 * NULL:  gives the base name to use for the
				 * new application. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    Frame *framePtr;
    Tk_Window new;
    char *className, *screenName, *visualName, *colormapName, *arg, *useOption;
    int i, c, length, depth;
    unsigned int mask;
    Colormap colormap;
    Visual *visual;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Pre-process the argument list.  Scan through it to find any
     * "-class", "-screen", "-visual", and "-colormap" options.  These
     * arguments need to be processed specially, before the window
     * is configured using the usual Tk mechanisms.
     */

    className = colormapName = screenName = visualName = useOption = NULL;
    colormap = None;
    for (i = 2; i < argc; i += 2) {
	arg = argv[i];
	length = strlen(arg);
	if (length < 2) {
	    continue;
	}
	c = arg[1];
	if ((c == 'c') && (strncmp(arg, "-class", strlen(arg)) == 0)
		&& (length >= 3)) {
	    className = argv[i+1];
	} else if ((c == 'c')
		&& (strncmp(arg, "-colormap", strlen(arg)) == 0)) {
	    colormapName = argv[i+1];
	} else if ((c == 's') && toplevel
		&& (strncmp(arg, "-screen", strlen(arg)) == 0)) {
	    screenName = argv[i+1];
	} else if ((c == 'u') && toplevel
		&& (strncmp(arg, "-use", strlen(arg)) == 0)) {
	    useOption = argv[i+1];
	} else if ((c == 'v')
		&& (strncmp(arg, "-visual", strlen(arg)) == 0)) {
	    visualName = argv[i+1];
	}
    }

    /*
     * Create the window, and deal with the special options -use,
     * -classname, -colormap, -screenname, and -visual.  These options
     * must be handle before calling ConfigureFrame below, and they must
     * also be processed in a particular order, for the following
     * reasons:
     * 1. Must set the window's class before calling ConfigureFrame,
     *    so that unspecified options are looked up in the option
     *    database using the correct class.
     * 2. Must set visual information before calling ConfigureFrame
     *    so that colors are allocated in a proper colormap.
     * 3. Must call TkpUseWindow before setting non-default visual
     *    information, since TkpUseWindow changes the defaults.
     */

    if (screenName == NULL) {
	screenName = (toplevel) ? "" : NULL;
    }
    if (tkwin != NULL) {
	new = Tk_CreateWindowFromPath(interp, tkwin, argv[1], screenName);
    } else {
	/*
	 * We were called from Tk_Init;  create a new application.
	 */

	if (appName == NULL) {
	    panic("TkCreateFrame didn't get application name");
	}
	new = TkCreateMainWindow(interp, screenName, appName);
    }
    if (new == NULL) {
	goto error;
    }
    if (className == NULL) {
	className = Tk_GetOption(new, "class", "Class");
	if (className == NULL) {
	    className = (toplevel) ? "Toplevel" : "Frame";
	}
    }
    Tk_SetClass(new, className);
    if (useOption == NULL) {
	useOption = Tk_GetOption(new, "use", "Use");
    }
    if (useOption != NULL) {
	if (TkpUseWindow(interp, new, useOption) != TCL_OK) {
	    goto error;
	}
    }
    if (visualName == NULL) {
	visualName = Tk_GetOption(new, "visual", "Visual");
    }
    if (colormapName == NULL) {
	colormapName = Tk_GetOption(new, "colormap", "Colormap");
    }
    if (visualName != NULL) {
	visual = Tk_GetVisual(interp, new, visualName, &depth,
		(colormapName == NULL) ? &colormap : (Colormap *) NULL);
	if (visual == NULL) {
	    goto error;
	}
	Tk_SetWindowVisual(new, visual, depth, colormap);
    }
    if (colormapName != NULL) {
	colormap = Tk_GetColormap(interp, new, colormapName);
	if (colormap == None) {
	    goto error;
	}
	Tk_SetWindowColormap(new, colormap);
    }

    /*
     * For top-level windows, provide an initial geometry request of
     * 200x200,  just so the window looks nicer on the screen if it
     * doesn't request a size for itself.
     */

    if (toplevel) {
	Tk_GeometryRequest(new, 200, 200);
    }

    /*
     * Create the widget record, process configuration options, and
     * create event handlers.  Then fill in a few additional fields
     * in the widget record from the special options.
     */

    framePtr = (Frame *) ckalloc(sizeof(Frame));
    framePtr->tkwin = new;
    framePtr->display = Tk_Display(new);
    framePtr->interp = interp;
    framePtr->widgetCmd = Tcl_CreateCommand(interp,
	    Tk_PathName(new), FrameWidgetCmd,
	    (ClientData) framePtr, FrameCmdDeletedProc);
    framePtr->className = NULL;
    framePtr->mask = (toplevel) ? TOPLEVEL : FRAME;
    framePtr->screenName = NULL;
    framePtr->visualName = NULL;
    framePtr->colormapName = NULL;
    framePtr->colormap = colormap;
    framePtr->border = NULL;
    framePtr->borderWidth = 0;
    framePtr->relief = TK_RELIEF_FLAT;
    framePtr->highlightWidth = 0;
    framePtr->highlightBgColorPtr = NULL;
    framePtr->highlightColorPtr = NULL;
    framePtr->width = 0;
    framePtr->height = 0;
    framePtr->cursor = None;
    framePtr->takeFocus = NULL;
    framePtr->isContainer = 0;
    framePtr->useThis = NULL;
    framePtr->flags = 0;
    framePtr->menuName = NULL;

    /*
     * Store backreference to frame widget in window structure.
     */
    TkSetClassProcs(new, NULL, (ClientData) framePtr);

    mask = ExposureMask | StructureNotifyMask | FocusChangeMask;
    if (toplevel) {
        mask |= ActivateMask;
    }
    Tk_CreateEventHandler(new, mask, FrameEventProc, (ClientData) framePtr);
    if (ConfigureFrame(interp, framePtr, argc-2, argv+2, 0) != TCL_OK) {
	goto error;
    }
    if ((framePtr->isContainer)) {
	if (framePtr->useThis == NULL) {
	    TkpMakeContainer(framePtr->tkwin);
	} else {
	    Tcl_AppendResult(interp,"A window cannot have both the -use ",
		    "and the -container option set.");
	    return TCL_ERROR;
	}
    }
    if (toplevel) {
	Tcl_DoWhenIdle(MapFrame, (ClientData) framePtr);
    }
    Tcl_SetResult(interp, Tk_PathName(new), TCL_STATIC);
    return TCL_OK;

    error:
    if (new != NULL) {
	Tk_DestroyWindow(new);
    }
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * FrameWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a frame widget.  See the user
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
FrameWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Information about frame widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    register Frame *framePtr = (Frame *) clientData;
    int result;
    size_t length;
    int c, i;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) framePtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)
	    && (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " cget option\"",
		    (char *) NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	result = Tk_ConfigureValue(interp, framePtr->tkwin, configSpecs,
		(char *) framePtr, argv[2], framePtr->mask);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)
	    && (length >= 2)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, framePtr->tkwin, configSpecs,
		    (char *) framePtr, (char *) NULL, framePtr->mask);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, framePtr->tkwin, configSpecs,
		    (char *) framePtr, argv[2], framePtr->mask);
	} else {
	    /*
	     * Don't allow the options -class, -colormap, -container,
	     * -newcmap, -screen, -use, or -visual to be changed.
	     */

	    for (i = 2; i < argc; i++) {
		length = strlen(argv[i]);
		if (length < 2) {
		    continue;
		}
		c = argv[i][1];
		if (((c == 'c') && (strncmp(argv[i], "-class", length) == 0)
			&& (length >= 2))
			|| ((c == 'c') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(argv[i], "-colormap", length) == 0)
			&& (length >= 3))
			|| ((c == 'c')
			&& (strncmp(argv[i], "-container", length) == 0)
			&& (length >= 3))
			|| ((c == 's') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(argv[i], "-screen", length) == 0))
			|| ((c == 'u') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(argv[i], "-use", length) == 0))
			|| ((c == 'v') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(argv[i], "-visual", length) == 0))) {
		    Tcl_AppendResult(interp, "can't modify ", argv[i],
			    " option after widget is created", (char *) NULL);
		    result = TCL_ERROR;
		    goto done;
		}
	    }
	    result = ConfigureFrame(interp, framePtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be cget or configure", (char *) NULL);
	result = TCL_ERROR;
    }

    done:
    Tcl_Release((ClientData) framePtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyFrame --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a frame at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the frame is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyFrame(memPtr)
    char *memPtr;		/* Info about frame widget. */
{
    register Frame *framePtr = (Frame *) memPtr;

    Tk_FreeOptions(configSpecs, (char *) framePtr, framePtr->display,
	    framePtr->mask);
    if (framePtr->colormap != None) {
	Tk_FreeColormap(framePtr->display, framePtr->colormap);
    }
    ckfree((char *) framePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureFrame --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a frame widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for framePtr;  old resources get freed, if there
 *	were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureFrame(interp, framePtr, argc, argv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register Frame *framePtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int argc;			/* Number of valid entries in argv. */
    char **argv;		/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    char *oldMenuName;
    
    /*
     * Need the old menubar name for the menu code to delete it.
     */
    
    if (framePtr->menuName == NULL) {
    	oldMenuName = NULL;
    } else {
    	oldMenuName = ckalloc(strlen(framePtr->menuName) + 1);
    	strcpy(oldMenuName, framePtr->menuName);
    }
    
    if (Tk_ConfigureWidget(interp, framePtr->tkwin, configSpecs,
	    argc, argv, (char *) framePtr, flags | framePtr->mask) != TCL_OK) {
	return TCL_ERROR;
    }

    if (((oldMenuName == NULL) && (framePtr->menuName != NULL))
	    || ((oldMenuName != NULL) && (framePtr->menuName == NULL))
	    || ((oldMenuName != NULL) && (framePtr->menuName != NULL)
	    && strcmp(oldMenuName, framePtr->menuName) != 0)) {
	TkSetWindowMenuBar(interp, framePtr->tkwin, oldMenuName,
		framePtr->menuName);
    }
    
    if (framePtr->border != NULL) {
	Tk_SetBackgroundFromBorder(framePtr->tkwin, framePtr->border);
    } else {
	Tk_SetWindowBackgroundPixmap(framePtr->tkwin, None);
    }

    if (framePtr->highlightWidth < 0) {
	framePtr->highlightWidth = 0;
    }
    Tk_SetInternalBorder(framePtr->tkwin,
	    framePtr->borderWidth + framePtr->highlightWidth);
    if ((framePtr->width > 0) || (framePtr->height > 0)) {
	Tk_GeometryRequest(framePtr->tkwin, framePtr->width,
		framePtr->height);
    }

    if (oldMenuName != NULL) {
    	ckfree(oldMenuName);
    }

    if (Tk_IsMapped(framePtr->tkwin)) {
	if (!(framePtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayFrame, (ClientData) framePtr);
	}
	framePtr->flags |= REDRAW_PENDING;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayFrame --
 *
 *	This procedure is invoked to display a frame widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the frame in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayFrame(clientData)
    ClientData clientData;	/* Information about widget. */
{
    register Frame *framePtr = (Frame *) clientData;
    register Tk_Window tkwin = framePtr->tkwin;

    framePtr->flags &= ~REDRAW_PENDING;
    if ((framePtr->tkwin == NULL) || !Tk_IsMapped(tkwin)
        || framePtr->isContainer) {
	return;
    }

    if (framePtr->border != NULL) {
	Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin),
		framePtr->border, framePtr->highlightWidth,
		framePtr->highlightWidth,
		Tk_Width(tkwin) - 2*framePtr->highlightWidth,
		Tk_Height(tkwin) - 2*framePtr->highlightWidth,
		framePtr->borderWidth, framePtr->relief);
    }
    if (framePtr->highlightWidth != 0) {
        GC fgGC, bgGC;
        
	bgGC = Tk_GCForColor(framePtr->highlightBgColorPtr,
		Tk_WindowId(tkwin));
	if (framePtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(framePtr->highlightColorPtr,
		    Tk_WindowId(tkwin));
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, framePtr->highlightWidth,
		    Tk_WindowId(tkwin));
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, framePtr->highlightWidth,
		    Tk_WindowId(tkwin));
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * FrameEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher on
 *	structure changes to a frame.  For frames with 3D
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
FrameEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    register XEvent *eventPtr;	/* Information about event. */
{
    register Frame *framePtr = (Frame *) clientData;

    if (((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0))
	    || (eventPtr->type == ConfigureNotify)) {
	goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
	if (framePtr->menuName != NULL) {
	    TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin,
		    framePtr->menuName, NULL);
	    ckfree(framePtr->menuName);
	    framePtr->menuName = NULL;
	}
	if (framePtr->tkwin != NULL) {

	    /*
	     * If this window is a container, then this event could be
	     * coming from the embedded application, in which case
	     * Tk_DestroyWindow hasn't been called yet.  When Tk_DestroyWindow
	     * is called later, then another destroy event will be generated.
	     * We need to be sure we ignore the second event, since the frame
	     * could be gone by then.  To do so, delete the event handler
	     * explicitly (normally it's done implicitly by Tk_DestroyWindow).
	     */
    
	    Tk_DeleteEventHandler(framePtr->tkwin,
		    ExposureMask|StructureNotifyMask|FocusChangeMask,
		    FrameEventProc, (ClientData) framePtr);
	    framePtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(framePtr->interp, framePtr->widgetCmd);
	}
	if (framePtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayFrame, (ClientData) framePtr);
	}
	Tcl_CancelIdleCall(MapFrame, (ClientData) framePtr);
	Tcl_EventuallyFree((ClientData) framePtr, DestroyFrame);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    framePtr->flags |= GOT_FOCUS;
	    if (framePtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    framePtr->flags &= ~GOT_FOCUS;
	    if (framePtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == ActivateNotify) {
    	TkpSetMainMenubar(framePtr->interp, framePtr->tkwin,
    		framePtr->menuName);
    }
    return;

    redraw:
    if ((framePtr->tkwin != NULL) && !(framePtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayFrame, (ClientData) framePtr);
	framePtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FrameCmdDeletedProc --
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
FrameCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    Frame *framePtr = (Frame *) clientData;
    Tk_Window tkwin = framePtr->tkwin;

    if (framePtr->menuName != NULL) {
	TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin,
		framePtr->menuName, NULL);
	ckfree(framePtr->menuName);
	framePtr->menuName = NULL;
    }

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	framePtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MapFrame --
 *
 *	This procedure is invoked as a when-idle handler to map a
 *	newly-created top-level frame.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The frame given by the clientData argument is mapped.
 *
 *----------------------------------------------------------------------
 */

static void
MapFrame(clientData)
    ClientData clientData;		/* Pointer to frame structure. */
{
    Frame *framePtr = (Frame *) clientData;

    /*
     * Wait for all other background events to be processed before
     * mapping window.  This ensures that the window's correct geometry
     * will have been determined before it is first mapped, so that the
     * window manager doesn't get a false idea of its desired geometry.
     */

    Tcl_Preserve((ClientData) framePtr);
    while (1) {
	if (Tcl_DoOneEvent(TCL_IDLE_EVENTS) == 0) {
	    break;
	}

	/*
	 * After each event, make sure that the window still exists
	 * and quit if the window has been destroyed.
	 */

	if (framePtr->tkwin == NULL) {
	    Tcl_Release((ClientData) framePtr);
	    return;
	}
    }
    Tk_MapWindow(framePtr->tkwin);
    Tcl_Release((ClientData) framePtr);
}

/*
 *--------------------------------------------------------------
 *
 * TkInstallFrameMenu --
 *
 *	This function is needed when a Windows HWND is created
 *	and a menubar has been set to the window with a system
 *	menu. It notifies the menu package so that the system
 *	menu can be rebuilt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The system menu (if any) is created for the menubar
 *	associated with this frame.
 *
 *--------------------------------------------------------------
 */

void
TkInstallFrameMenu(tkwin)
    Tk_Window tkwin;		/* The window that was just created. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->mainPtr != NULL) {
	Frame *framePtr;
	framePtr = (Frame*) winPtr->instanceData;
	TkpMenuNotifyToplevelCreate(winPtr->mainPtr->interp, 
		framePtr->menuName);
    }
}
