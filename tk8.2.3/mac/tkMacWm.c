/* 
 * tkMacWm.c --
 *
 *	This module takes care of the interactions between a Tk-based
 *	application and the window manager.  Among other things, it
 *	implements the "wm" command and passes geometry information
 *	to the window manager.
 *
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Gestalt.h>
#include <QDOffscreen.h>
#include <Windows.h>
#include <ToolUtils.h>

#include <tclMac.h>
#include "tkPort.h"
#include "tkInt.h"
#include "tkMacInt.h"
#include <errno.h>
#include "tkScrollbar.h"

/*
 * We now require the Appearance headers.  They come with CodeWarrior Pro,
 * and are on the SDK CD.  However, we do not require the Appearance 
 * extension
 */
 
#include <Appearance.h>

/*
 * A data structure of the following type holds information for
 * each window manager protocol (such as WM_DELETE_WINDOW) for
 * which a handler (i.e. a Tcl command) has been defined for a
 * particular top-level window.
 */

typedef struct ProtocolHandler {
    Atom protocol;		/* Identifies the protocol. */
    struct ProtocolHandler *nextPtr;
				/* Next in list of protocol handlers for
				 * the same top-level window, or NULL for
				 * end of list. */
    Tcl_Interp *interp;		/* Interpreter in which to invoke command. */
    char command[4];		/* Tcl command to invoke when a client
				 * message for this protocol arrives. 
				 * The actual size of the structure varies
				 * to accommodate the needs of the actual
				 * command. THIS MUST BE THE LAST FIELD OF
				 * THE STRUCTURE. */
} ProtocolHandler;

#define HANDLER_SIZE(cmdLength) \
((unsigned) (sizeof(ProtocolHandler) - 3 + cmdLength))

/*
 * A data structure of the following type holds window-manager-related
 * information for each top-level window in an application.
 */

typedef struct TkWmInfo {
    TkWindow *winPtr;		/* Pointer to main Tk information for
				 * this window. */
    Window reparent;		/* If the window has been reparented, this
				 * gives the ID of the ancestor of the window
				 * that is a child of the root window (may
				 * not be window's immediate parent).  If
				 * the window isn't reparented, this has the
				 * value None. */
    Tk_Uid titleUid;		/* Title to display in window caption.  If
				 * NULL, use name of widget. */
    Tk_Uid iconName;		/* Name to display in icon. */
    Window master;		/* Master window for TRANSIENT_FOR property,
				 * or None. */
    XWMHints hints;		/* Various pieces of information for
				 * window manager. */
    char *leaderName;		/* Path name of leader of window group
				 * (corresponds to hints.window_group).
				 * Malloc-ed.  Note:  this field doesn't
				 * get updated if leader is destroyed. */
    char *masterWindowName;	/* Path name of window specified as master
				 * in "wm transient" command, or NULL.
				 * Malloc-ed. Note:  this field doesn't
				 * get updated if masterWindowName is
				 * destroyed. */
    Tk_Window icon;		/* Window to use as icon for this window,
				 * or NULL. */
    Tk_Window iconFor;		/* Window for which this window is icon, or
				 * NULL if this isn't an icon for anyone. */

    /*
     * Information used to construct an XSizeHints structure for
     * the window manager:
     */

    int sizeHintsFlags;		/* Flags word for XSizeHints structure.
				 * If the PBaseSize flag is set then the
				 * window is gridded;  otherwise it isn't
				 * gridded. */
    int minWidth, minHeight;	/* Minimum dimensions of window, in
				 * grid units, not pixels. */
    int maxWidth, maxHeight;	/* Maximum dimensions of window, in
				 * grid units, not pixels. */
    Tk_Window gridWin;		/* Identifies the window that controls
				 * gridding for this top-level, or NULL if
				 * the top-level isn't currently gridded. */
    int widthInc, heightInc;	/* Increments for size changes (# pixels
				 * per step). */
    struct {
	int x;	/* numerator */
	int y;  /* denominator */
    } minAspect, maxAspect;	/* Min/max aspect ratios for window. */
    int reqGridWidth, reqGridHeight;
				/* The dimensions of the window (in
				 * grid units) requested through
				 * the geometry manager. */
    int gravity;		/* Desired window gravity. */

    /*
     * Information used to manage the size and location of a window.
     */

    int width, height;		/* Desired dimensions of window, specified
				 * in grid units.  These values are
				 * set by the "wm geometry" command and by
				 * ConfigureNotify events (for when wm
				 * resizes window).  -1 means user hasn't
				 * requested dimensions. */
    int x, y;			/* Desired X and Y coordinates for window.
				 * These values are set by "wm geometry",
				 * plus by ConfigureNotify events (when wm
				 * moves window).  These numbers are
				 * different than the numbers stored in
				 * winPtr->changes because (a) they could be
				 * measured from the right or bottom edge
				 * of the screen (see WM_NEGATIVE_X and
				 * WM_NEGATIVE_Y flags) and (b) if the window
				 * has been reparented then they refer to the
				 * parent rather than the window itself. */
    int parentWidth, parentHeight;
				/* Width and height of reparent, in pixels
				 * *including border*.  If window hasn't been
				 * reparented then these will be the outer
				 * dimensions of the window, including
				 * border. */
    int xInParent, yInParent;	/* Offset of window within reparent,  measured
				 * from upper-left outer corner of parent's
				 * border to upper-left outer corner of child's
				 * border.  If not reparented then these are
				 * zero. */
    int configWidth, configHeight;
				/* Dimensions passed to last request that we
				 * issued to change geometry of window.  Used
				 * to eliminate redundant resize operations. */

    /*
     * Information about the virtual root window for this top-level,
     * if there is one.
     */

    Window vRoot;		/* Virtual root window for this top-level,
				 * or None if there is no virtual root
				 * window (i.e. just use the screen's root). */
    int vRootX, vRootY;		/* Position of the virtual root inside the
				 * root window.  If the WM_VROOT_OFFSET_STALE
				 * flag is set then this information may be
				 * incorrect and needs to be refreshed from
				 * the X server.  If vRoot is None then these
				 * values are both 0. */
    unsigned int vRootWidth, vRootHeight;
				/* Dimensions of the virtual root window.
				 * If vRoot is None, gives the dimensions
				 * of the containing screen.  This information
				 * is never stale, even though vRootX and
				 * vRootY can be. */

    /*
     * List of children of the toplevel which have private colormaps.
     */

    TkWindow **cmapList;	/* Array of window with private colormaps. */
    int cmapCount;		/* Number of windows in array. */

    /*
     * Miscellaneous information.
     */

    ProtocolHandler *protPtr;	/* First in list of protocol handlers for
				 * this window (NULL means none). */
    int cmdArgc;		/* Number of elements in cmdArgv below. */
    char **cmdArgv;		/* Array of strings to store in the
				 * WM_COMMAND property.  NULL means nothing
				 * available. */
    char *clientMachine;	/* String to store in WM_CLIENT_MACHINE
				 * property, or NULL. */
    int flags;			/* Miscellaneous flags, defined below. */

    /*
     * Macintosh information.
     */
    int style;			/* Native window style. */
    TkWindow *scrollWinPtr;	/* Ptr to scrollbar handling grow widget. */
} WmInfo;


/*
 * Flag values for WmInfo structures:
 *
 * WM_NEVER_MAPPED -		non-zero means window has never been
 *				mapped;  need to update all info when
 *				window is first mapped.
 * WM_UPDATE_PENDING -		non-zero means a call to UpdateGeometryInfo
 *				has already been scheduled for this
 *				window;  no need to schedule another one.
 * WM_NEGATIVE_X -		non-zero means x-coordinate is measured in
 *				pixels from right edge of screen, rather
 *				than from left edge.
 * WM_NEGATIVE_Y -		non-zero means y-coordinate is measured in
 *				pixels up from bottom of screen, rather than
 *				down from top.
 * WM_UPDATE_SIZE_HINTS -	non-zero means that new size hints need to be
 *				propagated to window manager.
 * WM_SYNC_PENDING -		set to non-zero while waiting for the window
 *				manager to respond to some state change.
 * WM_VROOT_OFFSET_STALE -	non-zero means that (x,y) offset information
 *				about the virtual root window is stale and
 *				needs to be fetched fresh from the X server.
 * WM_ABOUT_TO_MAP -		non-zero means that the window is about to
 *				be mapped by TkWmMapWindow.  This is used
 *				by UpdateGeometryInfo to modify its behavior.
 * WM_MOVE_PENDING -		non-zero means the application has requested
 *				a new position for the window, but it hasn't
 *				been reflected through the window manager
 *				yet.
 * WM_COLORMAPS_EXPLICIT -	non-zero means the colormap windows were
 *				set explicitly via "wm colormapwindows".
 * WM_ADDED_TOPLEVEL_COLORMAP - non-zero means that when "wm colormapwindows"
 *				was called the top-level itself wasn't
 *				specified, so we added it implicitly at
 *				the end of the list.
 * WM_WIDTH_NOT_RESIZABLE -	non-zero means that we're not supposed to
 *				allow the user to change the width of the
 *				window (controlled by "wm resizable"
 *				command).
 * WM_HEIGHT_NOT_RESIZABLE -	non-zero means that we're not supposed to
 *				allow the user to change the height of the
 *				window (controlled by "wm resizable"
 *				command).
 */

#define WM_NEVER_MAPPED			1
#define WM_UPDATE_PENDING		2
#define WM_NEGATIVE_X			4
#define WM_NEGATIVE_Y			8
#define WM_UPDATE_SIZE_HINTS		0x10
#define WM_SYNC_PENDING			0x20
#define WM_VROOT_OFFSET_STALE		0x40
#define WM_ABOUT_TO_MAP			0x100
#define WM_MOVE_PENDING			0x200
#define WM_COLORMAPS_EXPLICIT		0x400
#define WM_ADDED_TOPLEVEL_COLORMAP	0x800
#define WM_WIDTH_NOT_RESIZABLE		0x1000
#define WM_HEIGHT_NOT_RESIZABLE		0x2000

/*
 * This is a list of all of the toplevels that have been mapped so far. It is
 * used by the menu code to inval windows that were damaged by menus, and will
 * eventually also be used to keep track of floating windows.
 */

TkMacWindowList *tkMacWindowListPtr = NULL;

/*
 * The variable below is used to enable or disable tracing in this
 * module.  If tracing is enabled, then information is printed on
 * standard output about interesting interactions with the window
 * manager.
 */

static int wmTracing = 0;

/*
 * The following structure is the official type record for geometry
 * management of top-level windows.
 */

static void		TopLevelReqProc _ANSI_ARGS_((ClientData dummy,
	Tk_Window tkwin));

static Tk_GeomMgr wmMgrType = {
    "wm",				/* name */
    TopLevelReqProc,			/* requestProc */
    (Tk_GeomLostSlaveProc *) NULL,	/* lostSlaveProc */
};

/*
 * Hash table for Mac Window -> TkWindow mapping.
 */

static Tcl_HashTable windowTable;
static int windowHashInit = false;

void tkMacMoveWindow(WindowRef window, int x, int y);

/*
 * Forward declarations for procedures defined in this file:
 */

static void		InitialWindowBounds _ANSI_ARGS_((TkWindow *winPtr, 
			    Rect *geometry));
static int		ParseGeometry _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, TkWindow *winPtr));
static void		TopLevelEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		TopLevelReqProc _ANSI_ARGS_((ClientData dummy,
			    Tk_Window tkwin));
static void		UpdateGeometryInfo _ANSI_ARGS_((
			    ClientData clientData));
static void		UpdateSizeHints _ANSI_ARGS_((TkWindow *winPtr));
static void		UpdateVRootGeometry _ANSI_ARGS_((WmInfo *wmPtr));

/*
 *--------------------------------------------------------------
 *
 * TkWmNewWindow --
 *
 *	This procedure is invoked whenever a new top-level
 *	window is created.  Its job is to initialize the WmInfo
 *	structure for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A WmInfo structure gets allocated and initialized.
 *
 *--------------------------------------------------------------
 */

void
TkWmNewWindow(
    TkWindow *winPtr)		/* Newly-created top-level window. */
{
    register WmInfo *wmPtr;
	
    wmPtr = (WmInfo *) ckalloc(sizeof(WmInfo));
    wmPtr->winPtr = winPtr;
    wmPtr->reparent = None;
    wmPtr->titleUid = NULL;
    wmPtr->iconName = NULL;
    wmPtr->master = None;
    wmPtr->hints.flags = InputHint | StateHint;
    wmPtr->hints.input = True;
    wmPtr->hints.initial_state = NormalState;
    wmPtr->hints.icon_pixmap = None;
    wmPtr->hints.icon_window = None;
    wmPtr->hints.icon_x = wmPtr->hints.icon_y = 0;
    wmPtr->hints.icon_mask = None;
    wmPtr->hints.window_group = None;
    wmPtr->leaderName = NULL;
    wmPtr->masterWindowName = NULL;
    wmPtr->icon = NULL;
    wmPtr->iconFor = NULL;
    wmPtr->sizeHintsFlags = 0;
    wmPtr->minWidth = wmPtr->minHeight = 1;

    /*
     * Default the maximum dimensions to the size of the display, minus
     * a guess about how space is needed for window manager decorations.
     */

    wmPtr->maxWidth = DisplayWidth(winPtr->display, winPtr->screenNum) - 15;
    wmPtr->maxHeight = DisplayHeight(winPtr->display, winPtr->screenNum) - 30;
    wmPtr->gridWin = NULL;
    wmPtr->widthInc = wmPtr->heightInc = 1;
    wmPtr->minAspect.x = wmPtr->minAspect.y = 1;
    wmPtr->maxAspect.x = wmPtr->maxAspect.y = 1;
    wmPtr->reqGridWidth = wmPtr->reqGridHeight = -1;
    wmPtr->gravity = NorthWestGravity;
    wmPtr->width = -1;
    wmPtr->height = -1;
    wmPtr->x = winPtr->changes.x;
    wmPtr->y = winPtr->changes.y;
    wmPtr->parentWidth = winPtr->changes.width
	+ 2*winPtr->changes.border_width;
    wmPtr->parentHeight = winPtr->changes.height
	+ 2*winPtr->changes.border_width;
    wmPtr->xInParent = 0;
    wmPtr->yInParent = 0;
    wmPtr->cmapList = NULL;
    wmPtr->cmapCount = 0;
    wmPtr->configWidth = -1;
    wmPtr->configHeight = -1;
    wmPtr->vRoot = None;
    wmPtr->protPtr = NULL;
    wmPtr->cmdArgv = NULL;
    wmPtr->clientMachine = NULL;
    wmPtr->flags = WM_NEVER_MAPPED;
    wmPtr->style = zoomDocProc;
    wmPtr->scrollWinPtr = NULL;
    winPtr->wmInfoPtr = wmPtr;

    UpdateVRootGeometry(wmPtr);

    /*
     * Tk must monitor structure events for top-level windows, in order
     * to detect size and position changes caused by window managers.
     */

    Tk_CreateEventHandler((Tk_Window) winPtr, StructureNotifyMask,
	    TopLevelEventProc, (ClientData) winPtr);

    /*
     * Arrange for geometry requests to be reflected from the window
     * to the window manager.
     */

    Tk_ManageGeometry((Tk_Window) winPtr, &wmMgrType, (ClientData) 0);
}

/*
 *--------------------------------------------------------------
 *
 * TkWmMapWindow --
 *
 *	This procedure is invoked to map a top-level window.  This
 *	module gets a chance to update all window-manager-related
 *	information in properties before the window manager sees
 *	the map event and checks the properties.  It also gets to
 *	decide whether or not to even map the window after all.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Properties of winPtr may get updated to provide up-to-date
 *	information to the window manager.  The window may also get
 *	mapped, but it may not be if this procedure decides that
 *	isn't appropriate (e.g. because the window is withdrawn).
 *
 *--------------------------------------------------------------
 */

void
TkWmMapWindow(
    TkWindow *winPtr)		/* Top-level window that's about to
				 * be mapped. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Point where = {0, 0};
    int xOffset, yOffset;	    
    int firstMap = false;
    MacDrawable *macWin;

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	wmPtr->flags &= ~WM_NEVER_MAPPED;
	firstMap = true;

	/*
	 * Create the underlying Mac window for this Tk window.
	 */
	macWin = (MacDrawable *) winPtr->window;
	if (!TkMacHostToplevelExists(winPtr)) {
	    TkMacMakeRealWindowExist(winPtr);
	}
	
	/*
	 * Generate configure event when we first map the window.
	 */
	LocalToGlobal(&where);
	TkMacWindowOffset((WindowRef) TkMacGetDrawablePort((Drawable) macWin),
		&xOffset, &yOffset);
	where.h -= xOffset;
	where.v -= yOffset;
	TkGenWMConfigureEvent((Tk_Window) winPtr, 
		where.h, where.v, -1, -1, TK_LOCATION_CHANGED);
	
	/*
	 * This is the first time this window has ever been mapped.
	 * Store all the window-manager-related information for the
	 * window.
	 */

	if (wmPtr->titleUid == NULL) {
	    wmPtr->titleUid = winPtr->nameUid;
	}
	
	if (!Tk_IsEmbedded(winPtr)) {
	    TkSetWMName(winPtr, wmPtr->titleUid);
	}

	TkWmSetClass(winPtr);
    
	if (wmPtr->iconName != NULL) {
	    XSetIconName(winPtr->display, winPtr->window, wmPtr->iconName);
	}
    
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    }
    if (wmPtr->hints.initial_state == WithdrawnState) {
	return;
    }
    
    /*
     * TODO: we need to display a window if it's iconic on creation.
     */

    if (wmPtr->hints.initial_state == IconicState) {
	return;
    }
    
    /*
     * Update geometry information.
     */
    wmPtr->flags |= WM_ABOUT_TO_MAP;
    if (wmPtr->flags & WM_UPDATE_PENDING) {
	Tk_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
    }
    UpdateGeometryInfo((ClientData) winPtr);
    wmPtr->flags &= ~WM_ABOUT_TO_MAP;

    /*
     * Map the window.
     */

    XMapWindow(winPtr->display, winPtr->window);
    
    /*
     * Now that the window is visable we can determine the offset
     * from the window's content orgin to the window's decorative
     * orgin (structure orgin).
     */
    TkMacWindowOffset((WindowRef) TkMacGetDrawablePort(Tk_WindowId(winPtr)), 
	&wmPtr->xInParent, &wmPtr->yInParent);
}

/*
 *--------------------------------------------------------------
 *
 * TkWmUnmapWindow --
 *
 *	This procedure is invoked to unmap a top-level window.
 *	On the Macintosh all we do is call XUnmapWindow.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unmaps the window.
 *
 *--------------------------------------------------------------
 */

void
TkWmUnmapWindow(
    TkWindow *winPtr)		/* Top-level window that's about to
				 * be mapped. */
{
    XUnmapWindow(winPtr->display, winPtr->window);
}

/*
 *--------------------------------------------------------------
 *
 * TkWmDeadWindow --
 *
 *	This procedure is invoked when a top-level window is
 *	about to be deleted.  It cleans up the wm-related data
 *	structures for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The WmInfo structure for winPtr gets freed up.
 *
 *--------------------------------------------------------------
 */

void
TkWmDeadWindow(winPtr)
    TkWindow *winPtr;		/* Top-level window that's being deleted. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    WmInfo *wmPtr2;

    if (wmPtr == NULL) {
	return;
    }
    if (wmPtr->hints.flags & IconPixmapHint) {
	Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_pixmap);
    }
    if (wmPtr->hints.flags & IconMaskHint) {
	Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_mask);
    }
    if (wmPtr->leaderName != NULL) {
	ckfree(wmPtr->leaderName);
    }
    if (wmPtr->masterWindowName != NULL) {
	ckfree(wmPtr->masterWindowName);
    }
    if (wmPtr->icon != NULL) {
	wmPtr2 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
	wmPtr2->iconFor = NULL;
    }
    if (wmPtr->iconFor != NULL) {
	wmPtr2 = ((TkWindow *) wmPtr->iconFor)->wmInfoPtr;
	wmPtr2->icon = NULL;
	wmPtr2->hints.flags &= ~IconWindowHint;
    }
    while (wmPtr->protPtr != NULL) {
	ProtocolHandler *protPtr;

	protPtr = wmPtr->protPtr;
	wmPtr->protPtr = protPtr->nextPtr;
	Tcl_EventuallyFree((ClientData) protPtr, TCL_DYNAMIC);
    }
    if (wmPtr->cmdArgv != NULL) {
	ckfree((char *) wmPtr->cmdArgv);
    }
    if (wmPtr->clientMachine != NULL) {
	ckfree((char *) wmPtr->clientMachine);
    }
    if (wmPtr->flags & WM_UPDATE_PENDING) {
	Tk_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
    }
    ckfree((char *) wmPtr);
    winPtr->wmInfoPtr = NULL;
}

/*
 *--------------------------------------------------------------
 *
 * TkWmSetClass --
 *
 *	This procedure is invoked whenever a top-level window's
 *	class is changed.  If the window has been mapped then this
 *	procedure updates the window manager property for the
 *	class.  If the window hasn't been mapped, the update is
 *	deferred until just before the first mapping.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A window property may get updated.
 *
 *--------------------------------------------------------------
 */

void
TkWmSetClass(
    TkWindow *winPtr)		/* Newly-created top-level window. */
{
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_WmCmd --
 *
 *	This procedure is invoked to process the "wm" Tcl command.
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
Tk_WmCmd(
    ClientData clientData,	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    char **argv)		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr;
    register WmInfo *wmPtr;
    int c;
    size_t length;

    if (argc < 2) {
	wrongNumArgs:
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option window ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 't') && (strncmp(argv[1], "tracing", length) == 0)
	    && (length >= 3)) {
	if ((argc != 2) && (argc != 3)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " tracing ?boolean?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 2) {
	    Tcl_SetResult(interp, ((wmTracing) ? "on" : "off"), TCL_STATIC);
	    return TCL_OK;
	}
	return Tcl_GetBoolean(interp, argv[2], &wmTracing);
    }

    if (argc < 3) {
	goto wrongNumArgs;
    }
    winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[2], tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	Tcl_AppendResult(interp, "window \"", winPtr->pathName,
		"\" isn't a top-level window", (char *) NULL);
	return TCL_ERROR;
    }
    wmPtr = winPtr->wmInfoPtr;
    if ((c == 'a') && (strncmp(argv[1], "aspect", length) == 0)) {
	int numer1, denom1, numer2, denom2;

	if ((argc != 3) && (argc != 7)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " aspect window ?minNumer minDenom ",
		    "maxNumer maxDenom?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->sizeHintsFlags & PAspect) {
		char buf[TCL_INTEGER_SPACE * 4];

		sprintf(buf, "%d %d %d %d", wmPtr->minAspect.x,
			wmPtr->minAspect.y, wmPtr->maxAspect.x,
			wmPtr->maxAspect.y);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->sizeHintsFlags &= ~PAspect;
	} else {
	    if ((Tcl_GetInt(interp, argv[3], &numer1) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[4], &denom1) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[5], &numer2) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[6], &denom2) != TCL_OK)) {
		return TCL_ERROR;
	    }
	    if ((numer1 <= 0) || (denom1 <= 0) || (numer2 <= 0) ||
		    (denom2 <= 0)) {
		Tcl_SetResult(interp, "aspect number can't be <= 0",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	    wmPtr->minAspect.x = numer1;
	    wmPtr->minAspect.y = denom1;
	    wmPtr->maxAspect.x = numer2;
	    wmPtr->maxAspect.y = denom2;
	    wmPtr->sizeHintsFlags |= PAspect;
	}
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	goto updateGeom;
    } else if ((c == 'c') && (strncmp(argv[1], "client", length) == 0)
	    && (length >= 2)) {
	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " client window ?name?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->clientMachine != NULL) {
		Tcl_SetResult(interp, wmPtr->clientMachine, TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (argv[3][0] == 0) {
	    if (wmPtr->clientMachine != NULL) {
		ckfree((char *) wmPtr->clientMachine);
		wmPtr->clientMachine = NULL;
	    }
	    return TCL_OK;
	}
	if (wmPtr->clientMachine != NULL) {
	    ckfree((char *) wmPtr->clientMachine);
	}
	wmPtr->clientMachine = (char *)
	    ckalloc((unsigned) (strlen(argv[3]) + 1));
	strcpy(wmPtr->clientMachine, argv[3]);
    } else if ((c == 'c') && (strncmp(argv[1], "colormapwindows", length) == 0)
	    && (length >= 3)) {
	TkWindow **cmapList;
	TkWindow *winPtr2;
	int i, windowArgc, gotToplevel;
	char **windowArgv;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " colormapwindows window ?windowList?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    Tk_MakeWindowExist((Tk_Window) winPtr);
	    for (i = 0; i < wmPtr->cmapCount; i++) {
		if ((i == (wmPtr->cmapCount-1))
			&& (wmPtr->flags & WM_ADDED_TOPLEVEL_COLORMAP)) {
		    break;
		}
		Tcl_AppendElement(interp, wmPtr->cmapList[i]->pathName);
	    }
	    return TCL_OK;
	}
	if (Tcl_SplitList(interp, argv[3], &windowArgc, &windowArgv)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	cmapList = (TkWindow **) ckalloc((unsigned)
		((windowArgc+1)*sizeof(TkWindow*)));
	for (i = 0; i < windowArgc; i++) {
	    winPtr2 = (TkWindow *) Tk_NameToWindow(interp, windowArgv[i],
		    tkwin);
	    if (winPtr2 == NULL) {
		ckfree((char *) cmapList);
		ckfree((char *) windowArgv);
		return TCL_ERROR;
	    }
	    if (winPtr2 == winPtr) {
		gotToplevel = 1;
	    }
	    if (winPtr2->window == None) {
		Tk_MakeWindowExist((Tk_Window) winPtr2);
	    }
	    cmapList[i] = winPtr2;
	}
	if (!gotToplevel) {
	    wmPtr->flags |= WM_ADDED_TOPLEVEL_COLORMAP;
	    cmapList[windowArgc] = winPtr;
	    windowArgc++;
	} else {
	    wmPtr->flags &= ~WM_ADDED_TOPLEVEL_COLORMAP;
	}
	wmPtr->flags |= WM_COLORMAPS_EXPLICIT;
	if (wmPtr->cmapList != NULL) {
	    ckfree((char *)wmPtr->cmapList);
	}
	wmPtr->cmapList = cmapList;
	wmPtr->cmapCount = windowArgc;
	ckfree((char *) windowArgv);

	/*
	 * On the Macintosh all of this is just an excercise
	 * in compatability as we don't support colormaps.  If 
	 * we did they would be installed here.
	 */

	return TCL_OK;
    } else if ((c == 'c') && (strncmp(argv[1], "command", length) == 0)
	    && (length >= 3)) {
	int cmdArgc;
	char **cmdArgv;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " command window ?value?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->cmdArgv != NULL) {
		Tcl_SetResult(interp,
			Tcl_Merge(wmPtr->cmdArgc, wmPtr->cmdArgv),
			TCL_DYNAMIC);
	    }
	    return TCL_OK;
	}
	if (argv[3][0] == 0) {
	    if (wmPtr->cmdArgv != NULL) {
		ckfree((char *) wmPtr->cmdArgv);
		wmPtr->cmdArgv = NULL;
	    }
	    return TCL_OK;
	}
	if (Tcl_SplitList(interp, argv[3], &cmdArgc, &cmdArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (wmPtr->cmdArgv != NULL) {
	    ckfree((char *) wmPtr->cmdArgv);
	}
	wmPtr->cmdArgc = cmdArgc;
	wmPtr->cmdArgv = cmdArgv;
    } else if ((c == 'd') && (strncmp(argv[1], "deiconify", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " deiconify window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't deiconify ", argv[2],
		    ": it is an icon for ", winPtr->pathName, (char *) NULL);
	    return TCL_ERROR;
	}
        if (winPtr->flags & TK_EMBEDDED) {
            Tcl_AppendResult(interp, "can't deiconify ", winPtr->pathName,
                    ": it is an embedded window", (char *) NULL);
            return TCL_ERROR;
        }
	
	/*
	 * TODO: may not want to call this function - look at Map events gened.
	 */

	TkpWmSetState(winPtr, NormalState);
    } else if ((c == 'f') && (strncmp(argv[1], "focusmodel", length) == 0)
	    && (length >= 2)) {
	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " focusmodel window ?active|passive?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    Tcl_SetResult(interp, (wmPtr->hints.input ? "passive" : "active"),
		    TCL_STATIC);
	    return TCL_OK;
	}
	c = argv[3][0];
	length = strlen(argv[3]);
	if ((c == 'a') && (strncmp(argv[3], "active", length) == 0)) {
	    wmPtr->hints.input = False;
	} else if ((c == 'p') && (strncmp(argv[3], "passive", length) == 0)) {
	    wmPtr->hints.input = True;
	} else {
	    Tcl_AppendResult(interp, "bad argument \"", argv[3],
		    "\": must be active or passive", (char *) NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 'f') && (strncmp(argv[1], "frame", length) == 0)
	    && (length >= 2)) {
	Window window;
	char buf[TCL_INTEGER_SPACE];

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " frame window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	window = wmPtr->reparent;
	if (window == None) {
	    window = Tk_WindowId((Tk_Window) winPtr);
	}
	sprintf(buf, "0x%x", (unsigned int) window);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if ((c == 'g') && (strncmp(argv[1], "geometry", length) == 0)
	    && (length >= 2)) {
	char xSign, ySign;
	int width, height;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " geometry window ?newGeometry?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    char buf[16 + TCL_INTEGER_SPACE * 4];

	    xSign = (wmPtr->flags & WM_NEGATIVE_X) ? '-' : '+';
	    ySign = (wmPtr->flags & WM_NEGATIVE_Y) ? '-' : '+';
	    if (wmPtr->gridWin != NULL) {
		width = wmPtr->reqGridWidth + (winPtr->changes.width
			- winPtr->reqWidth)/wmPtr->widthInc;
		height = wmPtr->reqGridHeight + (winPtr->changes.height
			- winPtr->reqHeight)/wmPtr->heightInc;
	    } else {
		width = winPtr->changes.width;
		height = winPtr->changes.height;
	    }
	    sprintf(buf, "%dx%d%c%d%c%d", width, height, xSign, wmPtr->x,
		    ySign, wmPtr->y);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->width = -1;
	    wmPtr->height = -1;
	    goto updateGeom;
	}
	return ParseGeometry(interp, argv[3], winPtr);
    } else if ((c == 'g') && (strncmp(argv[1], "grid", length) == 0)
	    && (length >= 3)) {
	int reqWidth, reqHeight, widthInc, heightInc;

	if ((argc != 3) && (argc != 7)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " grid window ?baseWidth baseHeight ",
		    "widthInc heightInc?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->sizeHintsFlags & PBaseSize) {
		char buf[TCL_INTEGER_SPACE * 4];

		sprintf(buf, "%d %d %d %d", wmPtr->reqGridWidth,
			wmPtr->reqGridHeight, wmPtr->widthInc,
			wmPtr->heightInc);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    /*
	     * Turn off gridding and reset the width and height
	     * to make sense as ungridded numbers.
	     */

	    wmPtr->sizeHintsFlags &= ~(PBaseSize|PResizeInc);
	    if (wmPtr->width != -1) {
		wmPtr->width = winPtr->reqWidth + (wmPtr->width
			- wmPtr->reqGridWidth)*wmPtr->widthInc;
		wmPtr->height = winPtr->reqHeight + (wmPtr->height
			- wmPtr->reqGridHeight)*wmPtr->heightInc;
	    }
	    wmPtr->widthInc = 1;
	    wmPtr->heightInc = 1;
	} else {
	    if ((Tcl_GetInt(interp, argv[3], &reqWidth) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[4], &reqHeight) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[5], &widthInc) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[6], &heightInc) != TCL_OK)) {
		return TCL_ERROR;
	    }
	    if (reqWidth < 0) {
		Tcl_SetResult(interp, "baseWidth can't be < 0", TCL_STATIC);
		return TCL_ERROR;
	    }
	    if (reqHeight < 0) {
		Tcl_SetResult(interp, "baseHeight can't be < 0", TCL_STATIC);
		return TCL_ERROR;
	    }
	    if (widthInc < 0) {
		Tcl_SetResult(interp, "widthInc can't be < 0", TCL_STATIC);
		return TCL_ERROR;
	    }
	    if (heightInc < 0) {
		Tcl_SetResult(interp, "heightInc can't be < 0", TCL_STATIC);
		return TCL_ERROR;
	    }
	    Tk_SetGrid((Tk_Window) winPtr, reqWidth, reqHeight, widthInc,
		    heightInc);
	}
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	goto updateGeom;
    } else if ((c == 'g') && (strncmp(argv[1], "group", length) == 0)
	    && (length >= 3)) {
	Tk_Window tkwin2;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " group window ?pathName?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->hints.flags & WindowGroupHint) {
		Tcl_SetResult(interp, wmPtr->leaderName, TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->hints.flags &= ~WindowGroupHint;
	    if (wmPtr->leaderName != NULL) {
		ckfree(wmPtr->leaderName);
	    }
	    wmPtr->leaderName = NULL;
	} else {
	    tkwin2 = Tk_NameToWindow(interp, argv[3], tkwin);
	    if (tkwin2 == NULL) {
		return TCL_ERROR;
	    }
	    Tk_MakeWindowExist(tkwin2);
	    wmPtr->hints.window_group = Tk_WindowId(tkwin2);
	    wmPtr->hints.flags |= WindowGroupHint;
	    wmPtr->leaderName = ckalloc((unsigned) (strlen(argv[3])+1));
	    strcpy(wmPtr->leaderName, argv[3]);
	}
    } else if ((c == 'i') && (strncmp(argv[1], "iconbitmap", length) == 0)
	    && (length >= 5)) {
	Pixmap pixmap;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " iconbitmap window ?bitmap?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->hints.flags & IconPixmapHint) {
		Tcl_SetResult(interp,
			Tk_NameOfBitmap(winPtr->display, wmPtr->hints.icon_pixmap),
			TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    if (wmPtr->hints.icon_pixmap != None) {
		Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_pixmap);
	    }
	    wmPtr->hints.flags &= ~IconPixmapHint;
	} else {
	    pixmap = Tk_GetBitmap(interp, (Tk_Window) winPtr,
		    Tk_GetUid(argv[3]));
	    if (pixmap == None) {
		return TCL_ERROR;
	    }
	    wmPtr->hints.icon_pixmap = pixmap;
	    wmPtr->hints.flags |= IconPixmapHint;
	}
    } else if ((c == 'i') && (strncmp(argv[1], "iconify", length) == 0)
	    && (length >= 5)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " iconify window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (Tk_Attributes((Tk_Window) winPtr)->override_redirect) {
	    Tcl_AppendResult(interp, "can't iconify \"", winPtr->pathName,
		    "\": override-redirect flag is set", (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->master != None) {
	    Tcl_AppendResult(interp, "can't iconify \"", winPtr->pathName,
		    "\": it is a transient", (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't iconify ", argv[2],
		    ": it is an icon for ", Tk_PathName(wmPtr->iconFor),
		   (char *) NULL);
	    return TCL_ERROR;
	}
        if (winPtr->flags & TK_EMBEDDED) {
            Tcl_AppendResult(interp, "can't iconify ", winPtr->pathName,
                    ": it is an embedded window", (char *) NULL);
            return TCL_ERROR;
        }
	TkpWmSetState(winPtr, IconicState);
    } else if ((c == 'i') && (strncmp(argv[1], "iconmask", length) == 0)
	    && (length >= 5)) {
	Pixmap pixmap;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " iconmask window ?bitmap?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->hints.flags & IconMaskHint) {
		Tcl_SetResult(interp,
			Tk_NameOfBitmap(winPtr->display, wmPtr->hints.icon_mask),
			TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    if (wmPtr->hints.icon_mask != None) {
		Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_mask);
	    }
	    wmPtr->hints.flags &= ~IconMaskHint;
	} else {
	    pixmap = Tk_GetBitmap(interp, tkwin, Tk_GetUid(argv[3]));
	    if (pixmap == None) {
		return TCL_ERROR;
	    }
	    wmPtr->hints.icon_mask = pixmap;
	    wmPtr->hints.flags |= IconMaskHint;
	}
    } else if ((c == 'i') && (strncmp(argv[1], "iconname", length) == 0)
	    && (length >= 5)) {
	if (argc > 4) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " iconname window ?newName?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    Tcl_SetResult(interp,
		    ((wmPtr->iconName != NULL) ? wmPtr->iconName : ""),
		    TCL_STATIC);
	    return TCL_OK;
	} else {
	    wmPtr->iconName = Tk_GetUid(argv[3]);
	    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		XSetIconName(winPtr->display, winPtr->window, wmPtr->iconName);
	    }
	}
    } else if ((c == 'i') && (strncmp(argv[1], "iconposition", length) == 0)
	    && (length >= 5)) {
	int x, y;

	if ((argc != 3) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " iconposition window ?x y?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->hints.flags & IconPositionHint) {
		char buf[TCL_INTEGER_SPACE * 2];
		
		sprintf(buf, "%d %d", wmPtr->hints.icon_x,
			wmPtr->hints.icon_y);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->hints.flags &= ~IconPositionHint;
	} else {
	    if ((Tcl_GetInt(interp, argv[3], &x) != TCL_OK)
		    || (Tcl_GetInt(interp, argv[4], &y) != TCL_OK)){
		return TCL_ERROR;
	    }
	    wmPtr->hints.icon_x = x;
	    wmPtr->hints.icon_y = y;
	    wmPtr->hints.flags |= IconPositionHint;
	}
    } else if ((c == 'i') && (strncmp(argv[1], "iconwindow", length) == 0)
	    && (length >= 5)) {
	Tk_Window tkwin2;
	WmInfo *wmPtr2;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " iconwindow window ?pathName?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->icon != NULL) {
		Tcl_SetResult(interp, Tk_PathName(wmPtr->icon), TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->hints.flags &= ~IconWindowHint;
	    if (wmPtr->icon != NULL) {
		wmPtr2 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
		wmPtr2->iconFor = NULL;
		wmPtr2->hints.initial_state = WithdrawnState;
	    }
	    wmPtr->icon = NULL;
	} else {
	    tkwin2 = Tk_NameToWindow(interp, argv[3], tkwin);
	    if (tkwin2 == NULL) {
		return TCL_ERROR;
	    }
	    if (!Tk_IsTopLevel(tkwin2)) {
		Tcl_AppendResult(interp, "can't use ", argv[3],
			" as icon window: not at top level", (char *) NULL);
		return TCL_ERROR;
	    }
	    wmPtr2 = ((TkWindow *) tkwin2)->wmInfoPtr;
	    if (wmPtr2->iconFor != NULL) {
		Tcl_AppendResult(interp, argv[3], " is already an icon for ",
			Tk_PathName(wmPtr2->iconFor), (char *) NULL);
		return TCL_ERROR;
	    }
	    if (wmPtr->icon != NULL) {
		WmInfo *wmPtr3 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
		wmPtr3->iconFor = NULL;
	    }
	    Tk_MakeWindowExist(tkwin2);
	    wmPtr->hints.icon_window = Tk_WindowId(tkwin2);
	    wmPtr->hints.flags |= IconWindowHint;
	    wmPtr->icon = tkwin2;
	    wmPtr2->iconFor = (Tk_Window) winPtr;
	    if (!(wmPtr2->flags & WM_NEVER_MAPPED)) {
		/*
		 * Don't have iconwindows on the Mac.  We just withdraw.
		 */

		Tk_UnmapWindow(tkwin2);
	    }
	}
    } else if ((c == 'm') && (strncmp(argv[1], "maxsize", length) == 0)
	    && (length >= 2)) {
	int width, height;
	if ((argc != 3) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " maxsize window ?width height?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    char buf[TCL_INTEGER_SPACE * 2];
	    
	    sprintf(buf, "%d %d", wmPtr->maxWidth, wmPtr->maxHeight);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_OK;
	}
	if ((Tcl_GetInt(interp, argv[3], &width) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[4], &height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	wmPtr->maxWidth = width;
	wmPtr->maxHeight = height;
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	goto updateGeom;
    } else if ((c == 'm') && (strncmp(argv[1], "minsize", length) == 0)
	    && (length >= 2)) {
	int width, height;
	if ((argc != 3) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		   argv[0], " minsize window ?width height?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    char buf[TCL_INTEGER_SPACE * 2];
	    
	    sprintf(buf, "%d %d", wmPtr->minWidth, wmPtr->minHeight);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_OK;
	}
	if ((Tcl_GetInt(interp, argv[3], &width) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[4], &height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	wmPtr->minWidth = width;
	wmPtr->minHeight = height;
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	goto updateGeom;
    } else if ((c == 'o')
	    && (strncmp(argv[1], "overrideredirect", length) == 0)) {
	int boolean;
	XSetWindowAttributes atts;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " overrideredirect window ?boolean?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (Tk_Attributes((Tk_Window) winPtr)->override_redirect) {
		Tcl_SetResult(interp, "1", TCL_STATIC);
	    } else {
		Tcl_SetResult(interp, "0", TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (Tcl_GetBoolean(interp, argv[3], &boolean) != TCL_OK) {
	    return TCL_ERROR;
	}
	atts.override_redirect = (boolean) ? True : False;
	Tk_ChangeWindowAttributes((Tk_Window) winPtr, CWOverrideRedirect,
		&atts);
	wmPtr->style = (boolean) ? plainDBox : documentProc;
    } else if ((c == 'p') && (strncmp(argv[1], "positionfrom", length) == 0)
	    && (length >= 2)) {
	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " positionfrom window ?user/program?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->sizeHintsFlags & USPosition) {
		Tcl_SetResult(interp, "user", TCL_STATIC);
	    } else if (wmPtr->sizeHintsFlags & PPosition) {
		Tcl_SetResult(interp, "program", TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->sizeHintsFlags &= ~(USPosition|PPosition);
	} else {
	    c = argv[3][0];
	    length = strlen(argv[3]);
	    if ((c == 'u') && (strncmp(argv[3], "user", length) == 0)) {
		wmPtr->sizeHintsFlags &= ~PPosition;
		wmPtr->sizeHintsFlags |= USPosition;
	    } else if ((c == 'p') &&
		    (strncmp(argv[3], "program", length) == 0)) {
		wmPtr->sizeHintsFlags &= ~USPosition;
		wmPtr->sizeHintsFlags |= PPosition;
	    } else {
		Tcl_AppendResult(interp, "bad argument \"", argv[3],
			"\": must be program or user", (char *) NULL);
		return TCL_ERROR;
	    }
	}
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	goto updateGeom;
    } else if ((c == 'p') && (strncmp(argv[1], "protocol", length) == 0)
	    && (length >= 2)) {
	register ProtocolHandler *protPtr, *prevPtr;
	Atom protocol;
	int cmdLength;

	if ((argc < 3) || (argc > 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " protocol window ?name? ?command?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    /*
	     * Return a list of all defined protocols for the window.
	     */
	    for (protPtr = wmPtr->protPtr; protPtr != NULL;
					   protPtr = protPtr->nextPtr) {
		Tcl_AppendElement(interp,
			Tk_GetAtomName((Tk_Window) winPtr, protPtr->protocol));
	    }
	    return TCL_OK;
	}
	protocol = Tk_InternAtom((Tk_Window) winPtr, argv[3]);
	if (argc == 4) {
	    /*
	     * Return the command to handle a given protocol.
	     */
	    for (protPtr = wmPtr->protPtr; protPtr != NULL;
					   protPtr = protPtr->nextPtr) {
		if (protPtr->protocol == protocol) {
		    Tcl_SetResult(interp, protPtr->command, TCL_STATIC);
		    return TCL_OK;
		}
	    }
	    return TCL_OK;
	}

	/*
	 * Delete any current protocol handler, then create a new
	 * one with the specified command, unless the command is
	 * empty.
	 */

	for (protPtr = wmPtr->protPtr, prevPtr = NULL; protPtr != NULL;
	       prevPtr = protPtr, protPtr = protPtr->nextPtr) {
	    if (protPtr->protocol == protocol) {
		if (prevPtr == NULL) {
		    wmPtr->protPtr = protPtr->nextPtr;
		} else {
		    prevPtr->nextPtr = protPtr->nextPtr;
		}
		Tcl_EventuallyFree((ClientData) protPtr, TCL_DYNAMIC);
		break;
	    }
	}
	cmdLength = strlen(argv[4]);
	if (cmdLength > 0) {
	    protPtr = (ProtocolHandler *) ckalloc(HANDLER_SIZE(cmdLength));
	    protPtr->protocol = protocol;
	    protPtr->nextPtr = wmPtr->protPtr;
	    wmPtr->protPtr = protPtr;
	    protPtr->interp = interp;
	    strcpy(protPtr->command, argv[4]);
	}
    } else if ((c == 'r') && (strncmp(argv[1], "resizable", length) == 0)) {
	int width, height;

	if ((argc != 3) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " resizable window ?width height?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    char buf[TCL_INTEGER_SPACE * 2];

	    sprintf(buf, "%d %d",
		    (wmPtr->flags  & WM_WIDTH_NOT_RESIZABLE) ? 0 : 1,
		    (wmPtr->flags  & WM_HEIGHT_NOT_RESIZABLE) ? 0 : 1);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_OK;
	}
	if ((Tcl_GetBoolean(interp, argv[3], &width) != TCL_OK)
		|| (Tcl_GetBoolean(interp, argv[4], &height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (width) {
	    wmPtr->flags &= ~WM_WIDTH_NOT_RESIZABLE;
	} else {
	    wmPtr->flags |= WM_WIDTH_NOT_RESIZABLE;
	}
	if (height) {
	    wmPtr->flags &= ~WM_HEIGHT_NOT_RESIZABLE;
	} else {
	    wmPtr->flags |= WM_HEIGHT_NOT_RESIZABLE;
	}
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	if (wmPtr->scrollWinPtr != NULL) {
	    TkScrollbarEventuallyRedraw(
		(TkScrollbar *) wmPtr->scrollWinPtr->instanceData);
	}
	goto updateGeom;
    } else if ((c == 's') && (strncmp(argv[1], "sizefrom", length) == 0)
	    && (length >= 2)) {
	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " sizefrom window ?user|program?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->sizeHintsFlags & USSize) {
		Tcl_SetResult(interp, "user", TCL_STATIC);
	    } else if (wmPtr->sizeHintsFlags & PSize) {
		Tcl_SetResult(interp, "program", TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (*argv[3] == '\0') {
	    wmPtr->sizeHintsFlags &= ~(USSize|PSize);
	} else {
	    c = argv[3][0];
	    length = strlen(argv[3]);
	    if ((c == 'u') && (strncmp(argv[3], "user", length) == 0)) {
		wmPtr->sizeHintsFlags &= ~PSize;
		wmPtr->sizeHintsFlags |= USSize;
	    } else if ((c == 'p')
		    && (strncmp(argv[3], "program", length) == 0)) {
		wmPtr->sizeHintsFlags &= ~USSize;
		wmPtr->sizeHintsFlags |= PSize;
	    } else {
		Tcl_AppendResult(interp, "bad argument \"", argv[3],
			"\": must be program or user", (char *) NULL);
		return TCL_ERROR;
	    }
	}
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	goto updateGeom;
    } else if ((c == 's') && (strncmp(argv[1], "state", length) == 0)
	    && (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " state window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->iconFor != NULL) {
	    Tcl_SetResult(interp, "icon", TCL_STATIC);
	} else {
	    switch (wmPtr->hints.initial_state) {
		case NormalState:
		    Tcl_SetResult(interp, "normal", TCL_STATIC);
		    break;
		case IconicState:
		    Tcl_SetResult(interp, "iconic", TCL_STATIC);
		    break;
		case WithdrawnState:
		    Tcl_SetResult(interp, "withdrawn", TCL_STATIC);
		    break;
		case ZoomState:
		    Tcl_SetResult(interp, "zoomed", TCL_STATIC);
		    break;
	    }
	}
    } else if ((c == 't') && (strncmp(argv[1], "title", length) == 0)
	    && (length >= 2)) {
	if (argc > 4) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " title window ?newTitle?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    Tcl_SetResult(interp,
		    ((wmPtr->titleUid != NULL) ? wmPtr->titleUid : winPtr->nameUid),
		    TCL_STATIC);
	    return TCL_OK;
	} else {
	    wmPtr->titleUid = Tk_GetUid(argv[3]);
	    if (!(wmPtr->flags & WM_NEVER_MAPPED) && !Tk_IsEmbedded(winPtr)) {
		TkSetWMName(winPtr, wmPtr->titleUid);
	    }
	}
    } else if ((c == 't') && (strncmp(argv[1], "transient", length) == 0)
	    && (length >= 3)) {
	Tk_Window master;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " transient window ?master?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->master != None) {
		Tcl_SetResult(interp, wmPtr->masterWindowName, TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (argv[3][0] == '\0') {
	    wmPtr->master = None;
	    if (wmPtr->masterWindowName != NULL) {
		ckfree(wmPtr->masterWindowName);
	    }
	    wmPtr->masterWindowName = NULL;
	    wmPtr->style = documentProc;
	} else {
	    master = Tk_NameToWindow(interp, argv[3], tkwin);
	    if (master == NULL) {
		return TCL_ERROR;
	    }
	    Tk_MakeWindowExist(master);
	    wmPtr->master = Tk_WindowId(master);
	    wmPtr->masterWindowName = ckalloc((unsigned) (strlen(argv[3])+1));
	    strcpy(wmPtr->masterWindowName, argv[3]);
	    wmPtr->style = plainDBox;
	}
    } else if ((c == 'w') && (strncmp(argv[1], "withdraw", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " withdraw window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't withdraw ", argv[2],
		    ": it is an icon for ", Tk_PathName(wmPtr->iconFor),
		    (char *) NULL);
	    return TCL_ERROR;
	}
	TkpWmSetState(winPtr, WithdrawnState);
    } else {
	Tcl_AppendResult(interp, "unknown or ambiguous option \"", argv[1],
		"\": must be aspect, client, command, deiconify, ",
		"focusmodel, frame, geometry, grid, group, iconbitmap, ",
		"iconify, iconmask, iconname, iconposition, ",
		"iconwindow, maxsize, minsize, overrideredirect, ",
		"positionfrom, protocol, resizable, sizefrom, state, title, ",
		"transient, or withdraw",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;

    updateGeom:
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tk_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetGrid --
 *
 *	This procedure is invoked by a widget when it wishes to set a grid
 *	coordinate system that controls the size of a top-level window.
 *	It provides a C interface equivalent to the "wm grid" command and
 *	is usually asscoiated with the -setgrid option.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Grid-related information will be passed to the window manager, so
 *	that the top-level window associated with tkwin will resize on
 *	even grid units.  If some other window already controls gridding
 *	for the top-level window then this procedure call has no effect.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetGrid(
    Tk_Window tkwin,		/* Token for window.  New window mgr info
				 * will be posted for the top-level window
				 * associated with this window. */
    int reqWidth,		/* Width (in grid units) corresponding to
				 * the requested geometry for tkwin. */
    int reqHeight,		/* Height (in grid units) corresponding to
				 * the requested geometry for tkwin. */
    int widthInc, int heightInc)/* Pixel increments corresponding to a
				 * change of one grid unit. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr;

    /*
     * Find the top-level window for tkwin, plus the window manager
     * information.
     */

    while (!(winPtr->flags & TK_TOP_LEVEL)) {
	winPtr = winPtr->parentPtr;
    }
    wmPtr = winPtr->wmInfoPtr;

    if ((wmPtr->gridWin != NULL) && (wmPtr->gridWin != tkwin)) {
	return;
    }

    if ((wmPtr->reqGridWidth == reqWidth)
	    && (wmPtr->reqGridHeight == reqHeight)
	    && (wmPtr->widthInc == widthInc)
	    && (wmPtr->heightInc == heightInc)
	    && ((wmPtr->sizeHintsFlags & (PBaseSize|PResizeInc))
		    == PBaseSize|PResizeInc)) {
	return;
    }

    /*
     * If gridding was previously off, then forget about any window
     * size requests made by the user or via "wm geometry":  these are
     * in pixel units and there's no easy way to translate them to
     * grid units since the new requested size of the top-level window in
     * pixels may not yet have been registered yet (it may filter up
     * the hierarchy in DoWhenIdle handlers).  However, if the window
     * has never been mapped yet then just leave the window size alone:
     * assume that it is intended to be in grid units but just happened
     * to have been specified before this procedure was called.
     */

    if ((wmPtr->gridWin == NULL) && !(wmPtr->flags & WM_NEVER_MAPPED)) {
	wmPtr->width = -1;
	wmPtr->height = -1;
    }

    /* 
     * Set the new gridding information, and start the process of passing
     * all of this information to the window manager.
     */

    wmPtr->gridWin = tkwin;
    wmPtr->reqGridWidth = reqWidth;
    wmPtr->reqGridHeight = reqHeight;
    wmPtr->widthInc = widthInc;
    wmPtr->heightInc = heightInc;
    wmPtr->sizeHintsFlags |= PBaseSize|PResizeInc;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tk_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UnsetGrid --
 *
 *	This procedure cancels the effect of a previous call
 *	to Tk_SetGrid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If tkwin currently controls gridding for its top-level window,
 *	gridding is cancelled for that top-level window;  if some other
 *	window controls gridding then this procedure has no effect.
 *
 *----------------------------------------------------------------------
 */

void
Tk_UnsetGrid(
    Tk_Window tkwin)		/* Token for window that is currently
				 * controlling gridding. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr;

    /*
     * Find the top-level window for tkwin, plus the window manager
     * information.
     */

    while (!(winPtr->flags & TK_TOP_LEVEL)) {
	winPtr = winPtr->parentPtr;
    }
    wmPtr = winPtr->wmInfoPtr;
    if (tkwin != wmPtr->gridWin) {
	return;
    }

    wmPtr->gridWin = NULL;
    wmPtr->sizeHintsFlags &= ~(PBaseSize|PResizeInc);
    if (wmPtr->width != -1) {
	wmPtr->width = winPtr->reqWidth + (wmPtr->width
		- wmPtr->reqGridWidth)*wmPtr->widthInc;
	wmPtr->height = winPtr->reqHeight + (wmPtr->height
		- wmPtr->reqGridHeight)*wmPtr->heightInc;
    }
    wmPtr->widthInc = 1;
    wmPtr->heightInc = 1;

    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tk_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TopLevelEventProc --
 *
 *	This procedure is invoked when a top-level (or other externally-
 *	managed window) is restructured in any way.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tk's internal data structures for the window get modified to
 *	reflect the structural change.
 *
 *----------------------------------------------------------------------
 */

static void
TopLevelEventProc(
    ClientData clientData,		/* Window for which event occurred. */
    XEvent *eventPtr)			/* Event that just happened. */
{
    register TkWindow *winPtr = (TkWindow *) clientData;

    winPtr->wmInfoPtr->flags |= WM_VROOT_OFFSET_STALE;
    if (eventPtr->type == DestroyNotify) {
	Tk_ErrorHandler handler;

	if (!(winPtr->flags & TK_ALREADY_DEAD)) {
	    /*
	     * A top-level window was deleted externally (e.g., by the window
	     * manager).  This is probably not a good thing, but cleanup as
	     * best we can.  The error handler is needed because
	     * Tk_DestroyWindow will try to destroy the window, but of course
	     * it's already gone.
	     */
    
	    handler = Tk_CreateErrorHandler(winPtr->display, -1, -1, -1,
		    (Tk_ErrorProc *) NULL, (ClientData) NULL);
	    Tk_DestroyWindow((Tk_Window) winPtr);
	    Tk_DeleteErrorHandler(handler);
	}
	if (wmTracing) {
	    printf("TopLevelEventProc: %s deleted\n", winPtr->pathName);
	}
    } else if (eventPtr->type == ReparentNotify) {
	panic("recieved unwanted reparent event");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TopLevelReqProc --
 *
 *	This procedure is invoked by the geometry manager whenever
 *	the requested size for a top-level window is changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arrange for the window to be resized to satisfy the request
 *	(this happens as a when-idle action).
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
TopLevelReqProc(
    ClientData dummy,			/* Not used. */
    Tk_Window tkwin)			/* Information about window. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr;

    wmPtr = winPtr->wmInfoPtr;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tk_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateGeometryInfo --
 *
 *	This procedure is invoked when a top-level window is first
 *	mapped, and also as a when-idle procedure, to bring the
 *	geometry and/or position of a top-level window back into
 *	line with what has been requested by the user and/or widgets.
 *	This procedure doesn't return until the window manager has
 *	responded to the geometry change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window's size and location may change, unless the WM prevents
 *	that from happening.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateGeometryInfo(
    ClientData clientData)		/* Pointer to the window's record. */
{
    register TkWindow *winPtr = (TkWindow *) clientData;
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int x, y, width, height;
    unsigned long serial;

    wmPtr->flags &= ~WM_UPDATE_PENDING;

    /*
     * Compute the new size for the top-level window.  See the
     * user documentation for details on this, but the size
     * requested depends on (a) the size requested internally
     * by the window's widgets, (b) the size requested by the
     * user in a "wm geometry" command or via wm-based interactive
     * resizing (if any), and (c) whether or not the window is
     * gridded.  Don't permit sizes <= 0 because this upsets
     * the X server.
     */

    if (wmPtr->width == -1) {
	width = winPtr->reqWidth;
    } else if (wmPtr->gridWin != NULL) {
	width = winPtr->reqWidth
		+ (wmPtr->width - wmPtr->reqGridWidth)*wmPtr->widthInc;
    } else {
	width = wmPtr->width;
    }
    if (width <= 0) {
	width = 1;
    }
    if (wmPtr->height == -1) {
	height = winPtr->reqHeight;
    } else if (wmPtr->gridWin != NULL) {
	height = winPtr->reqHeight
		+ (wmPtr->height - wmPtr->reqGridHeight)*wmPtr->heightInc;
    } else {
	height = wmPtr->height;
    }
    if (height <= 0) {
	height = 1;
    }

    /*
     * Compute the new position for the upper-left pixel of the window's
     * decorative frame.  This is tricky, because we need to include the
     * border widths supplied by a reparented parent in this calculation,
     * but can't use the parent's current overall size since that may
     * change as a result of this code.
     */

    if (wmPtr->flags & WM_NEGATIVE_X) {
	x = wmPtr->vRootWidth - wmPtr->x
	    - (width + (wmPtr->parentWidth - winPtr->changes.width));
    } else {
	x =  wmPtr->x;
    }
    if (wmPtr->flags & WM_NEGATIVE_Y) {
	y = wmPtr->vRootHeight - wmPtr->y
	    - (height + (wmPtr->parentHeight - winPtr->changes.height));
    } else {
	y =  wmPtr->y;
    }

    /*
     * If the window's size is going to change and the window is
     * supposed to not be resizable by the user, then we have to
     * update the size hints.  There may also be a size-hint-update
     * request pending from somewhere else, too.
     */

    if (((width != winPtr->changes.width)
	    || (height != winPtr->changes.height))
	    && (wmPtr->gridWin == NULL)
	    && ((wmPtr->sizeHintsFlags & (PMinSize|PMaxSize)) == 0)) {
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    }
    if (wmPtr->flags & WM_UPDATE_SIZE_HINTS) {
	UpdateSizeHints(winPtr);
    }

    /*
     * Reconfigure the window if it isn't already configured correctly.
     * A few tricky points:
     *
     * 1. If the window is embedded and the container is also in this
     *    process, don't actually reconfigure the window; just pass the
     *    desired size on to the container.  Also, zero out any position
     *    information, since embedded windows are not allowed to move.
     * 2. Sometimes the window manager will give us a different size
     *    than we asked for (e.g. mwm has a minimum size for windows), so
     *    base the size check on what we *asked for* last time, not what we
     *    got.
     * 3. Don't move window unless a new position has been requested for
     *	  it.  This is because of "features" in some window managers (e.g.
     *    twm, as of 4/24/91) where they don't interpret coordinates
     *    according to ICCCM.  Moving a window to its current location may
     *    cause it to shift position on the screen.
     */

    if (Tk_IsEmbedded(winPtr)) {
        TkWindow *contWinPtr;

	contWinPtr = TkpGetOtherWindow(winPtr);
	
	/*
	 * NOTE: Here we should handle out of process embedding.
	 */

	if (contWinPtr != NULL) {	    
	    /*
	     * This window is embedded and the container is also in this
	     * process, so we don't need to do anything special about the
	     * geometry, except to make sure that the desired size is known
	     * by the container.  Also, zero out any position information,
	     * since embedded windows are not allowed to move.
	     */

	    wmPtr->x = wmPtr->y = 0;
	    wmPtr->flags &= ~(WM_NEGATIVE_X|WM_NEGATIVE_Y);
	    Tk_GeometryRequest((Tk_Window) contWinPtr, width, height);
        }
	return;
    }
    serial = NextRequest(winPtr->display);
    if (wmPtr->flags & WM_MOVE_PENDING) {
	wmPtr->configWidth = width;
	wmPtr->configHeight = height;
	if (wmTracing) {
	    printf(
		"UpdateGeometryInfo moving to %d %d, resizing to %d x %d,\n",
		    x, y, width, height);
	}
	Tk_MoveResizeWindow((Tk_Window) winPtr, x, y, (unsigned) width,
		(unsigned) height);
    } else if ((width != wmPtr->configWidth)
	    || (height != wmPtr->configHeight)) {
	wmPtr->configWidth = width;
	wmPtr->configHeight = height;
	if (wmTracing) {
	    printf("UpdateGeometryInfo resizing to %d x %d\n", width, height);
	}
	Tk_ResizeWindow((Tk_Window) winPtr, (unsigned) width,
		(unsigned) height);
    } else {
	return;
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdateSizeHints --
 *
 *	This procedure is called to update the window manager's
 *	size hints information from the information in a WmInfo
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Properties get changed for winPtr.
 *
 *--------------------------------------------------------------
 */

static void
UpdateSizeHints(
    TkWindow *winPtr)
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    XSizeHints *hintsPtr;

    wmPtr->flags &= ~WM_UPDATE_SIZE_HINTS;

    hintsPtr = XAllocSizeHints();
    if (hintsPtr == NULL) {
	return;
    }

    /*
     * Compute the pixel-based sizes for the various fields in the
     * size hints structure, based on the grid-based sizes in
     * our structure.
     */

    if (wmPtr->gridWin != NULL) {
	hintsPtr->base_width = winPtr->reqWidth
		- (wmPtr->reqGridWidth * wmPtr->widthInc);
	if (hintsPtr->base_width < 0) {
	    hintsPtr->base_width = 0;
	}
	hintsPtr->base_height = winPtr->reqHeight
		- (wmPtr->reqGridHeight * wmPtr->heightInc);
	if (hintsPtr->base_height < 0) {
	    hintsPtr->base_height = 0;
	}
	hintsPtr->min_width = hintsPtr->base_width
		+ (wmPtr->minWidth * wmPtr->widthInc);
	hintsPtr->min_height = hintsPtr->base_height
		+ (wmPtr->minHeight * wmPtr->heightInc);
	hintsPtr->max_width = hintsPtr->base_width
		+ (wmPtr->maxWidth * wmPtr->widthInc);
	hintsPtr->max_height = hintsPtr->base_height
		+ (wmPtr->maxHeight * wmPtr->heightInc);
    } else {
	hintsPtr->min_width = wmPtr->minWidth;
	hintsPtr->min_height = wmPtr->minHeight;
	hintsPtr->max_width = wmPtr->maxWidth;
	hintsPtr->max_height = wmPtr->maxHeight;
	hintsPtr->base_width = 0;
	hintsPtr->base_height = 0;
    }
    hintsPtr->width_inc = wmPtr->widthInc;
    hintsPtr->height_inc = wmPtr->heightInc;
    hintsPtr->min_aspect.x = wmPtr->minAspect.x;
    hintsPtr->min_aspect.y = wmPtr->minAspect.y;
    hintsPtr->max_aspect.x = wmPtr->maxAspect.x;
    hintsPtr->max_aspect.y = wmPtr->maxAspect.y;
    hintsPtr->win_gravity = wmPtr->gravity;
    hintsPtr->flags = wmPtr->sizeHintsFlags | PMinSize | PMaxSize;

    /*
     * If the window isn't supposed to be resizable, then set the
     * minimum and maximum dimensions to be the same.
     */

    if (wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) {
	if (wmPtr->width >= 0) {
	    hintsPtr->min_width = wmPtr->width;
	} else {
	    hintsPtr->min_width = winPtr->reqWidth;
	}
	hintsPtr->max_width = hintsPtr->min_width;
    }
    if (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE) {
	if (wmPtr->height >= 0) {
	    hintsPtr->min_height = wmPtr->height;
	} else {
	    hintsPtr->min_height = winPtr->reqHeight;
	}
	hintsPtr->max_height = hintsPtr->min_height;
    }

    XSetWMNormalHints(winPtr->display, winPtr->window, hintsPtr);

    XFree((char *) hintsPtr);
}

/*
 *--------------------------------------------------------------
 *
 * ParseGeometry --
 *
 *	This procedure parses a geometry string and updates
 *	information used to control the geometry of a top-level
 *	window.
 *
 * Results:
 *	A standard Tcl return value, plus an error message in
 *	the interp's result if an error occurs.
 *
 * Side effects:
 *	The size and/or location of winPtr may change.
 *
 *--------------------------------------------------------------
 */

static int
ParseGeometry(
    Tcl_Interp *interp,		/* Used for error reporting. */
    char *string,		/* String containing new geometry.  Has the
				 * standard form "=wxh+x+y". */
    TkWindow *winPtr)		/* Pointer to top-level window whose
				 * geometry is to be changed. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int x, y, width, height, flags;
    char *end;
    register char *p = string;

    /*
     * The leading "=" is optional.
     */

    if (*p == '=') {
	p++;
    }

    /*
     * Parse the width and height, if they are present.  Don't
     * actually update any of the fields of wmPtr until we've
     * successfully parsed the entire geometry string.
     */

    width = wmPtr->width;
    height = wmPtr->height;
    x = wmPtr->x;
    y = wmPtr->y;
    flags = wmPtr->flags;
    if (isdigit(UCHAR(*p))) {
	width = strtoul(p, &end, 10);
	p = end;
	if (*p != 'x') {
	    goto error;
	}
	p++;
	if (!isdigit(UCHAR(*p))) {
	    goto error;
	}
	height = strtoul(p, &end, 10);
	p = end;
    }

    /*
     * Parse the X and Y coordinates, if they are present.
     */

    if (*p != '\0') {
	flags &= ~(WM_NEGATIVE_X | WM_NEGATIVE_Y);
	if (*p == '-') {
	    flags |= WM_NEGATIVE_X;
	} else if (*p != '+') {
	    goto error;
	}
	x = strtol(p+1, &end, 10);
	p = end;
	if (*p == '-') {
	    flags |= WM_NEGATIVE_Y;
	} else if (*p != '+') {
	    goto error;
	}
	y = strtol(p+1, &end, 10);
	if (*end != '\0') {
	    goto error;
	}

	/*
	 * Assume that the geometry information came from the user,
	 * unless an explicit source has been specified.  Otherwise
	 * most window managers assume that the size hints were
	 * program-specified and they ignore them.
	 */

	if ((wmPtr->sizeHintsFlags & (USPosition|PPosition)) == 0) {
	    wmPtr->sizeHintsFlags |= USPosition;
	    flags |= WM_UPDATE_SIZE_HINTS;
	}
    }

    /*
     * Everything was parsed OK.  Update the fields of *wmPtr and
     * arrange for the appropriate information to be percolated out
     * to the window manager at the next idle moment.
     */

    wmPtr->width = width;
    wmPtr->height = height;
    if ((x != wmPtr->x) || (y != wmPtr->y)
	    || ((flags & (WM_NEGATIVE_X|WM_NEGATIVE_Y))
		    != (wmPtr->flags & (WM_NEGATIVE_X|WM_NEGATIVE_Y)))) {
	wmPtr->x = x;
	wmPtr->y = y;
	flags |= WM_MOVE_PENDING;
    }
    wmPtr->flags = flags;

    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tk_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
    return TCL_OK;

    error:
    Tcl_AppendResult(interp, "bad geometry specifier \"",
	    string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetRootCoords --
 *
 *	Given a token for a window, this procedure traces through the
 *	window's lineage to find the (virtual) root-window coordinates
 *	corresponding to point (0,0) in the window.
 *
 * Results:
 *	The locations pointed to by xPtr and yPtr are filled in with
 *	the root coordinates of the (0,0) point in tkwin.  If a virtual
 *	root window is in effect for the window, then the coordinates
 *	in the virtual root are returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_GetRootCoords(
    Tk_Window tkwin,		/* Token for window. */
    int *xPtr,			/* Where to store x-displacement of (0,0). */
    int *yPtr)			/* Where to store y-displacement of (0,0). */
{
    int x, y;
    register TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * Search back through this window's parents all the way to a
     * top-level window, combining the offsets of each window within
     * its parent.
     */

    x = y = 0;
    while (1) {
	x += winPtr->changes.x + winPtr->changes.border_width;
	y += winPtr->changes.y + winPtr->changes.border_width;
	if (winPtr->flags & TK_TOP_LEVEL) {
	    if (!(Tk_IsEmbedded(winPtr))) {
	    	x += winPtr->wmInfoPtr->xInParent;
	    	y += winPtr->wmInfoPtr->yInParent;
	    	break;	    	
	    } else {
	        TkWindow *otherPtr;
		
		otherPtr = TkpGetOtherWindow(winPtr);
		if (otherPtr != NULL) {
		    /*
		     * The container window is in the same application.
		     * Query its coordinates.
		     */
		    winPtr = otherPtr;
		    
		    /*
		     * Remember to offset by the container window here,
		     * since at the end of this if branch, we will
		     * pop out to the container's parent...
		     */
		     
	            x += winPtr->changes.x + winPtr->changes.border_width;
	            y += winPtr->changes.y + winPtr->changes.border_width;
		    
		} else {
		    Point theOffset;
		    
		    if (gMacEmbedHandler->getOffsetProc != NULL) {
		        /*
		         * We do not require that the changes.x & changes.y for 
		         * a non-Tk master window be kept up to date.  So we
		         * first subtract off the possibly bogus values that have
		         * been added on at the top of this pass through the loop,
		         * and then call out to the getOffsetProc to give us
		         * the correct offset.
		         */
		         
	                x -= winPtr->changes.x + winPtr->changes.border_width;
	                y -= winPtr->changes.y + winPtr->changes.border_width;
	                
		        gMacEmbedHandler->getOffsetProc((Tk_Window) winPtr, &theOffset);
		        
		        x += theOffset.h;
		        y += theOffset.v;
		    }
		    break;
		}
	    }
	}
	winPtr = winPtr->parentPtr;
    }
    *xPtr = x;
    *yPtr = y;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CoordsToWindow --
 *
 *	This is a Macintosh specific implementation of this function.
 *	Given the root coordinates of a point, this procedure returns
 *	the token for the top-most window covering that point, if
 *	there exists such a window in this application.
 *
 * Results:
 *	The return result is either a token for the window corresponding
 *	to rootX and rootY, or else NULL to indicate that there is no such
 *	window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_CoordsToWindow(
    int rootX, int rootY,	/* Coordinates of point in root window.  If
				 * a virtual-root window manager is in use,
				 * these coordinates refer to the virtual
				 * root, not the real root. */
    Tk_Window tkwin)		/* Token for any window in application;
				 * used to identify the display. */
{
    WindowPtr whichWin;
    Point where;
    Window rootChild;
    register TkWindow *winPtr, *childPtr;
    TkWindow *nextPtr;		/* Coordinates of highest child found so
				 * far that contains point. */
    int x, y;			/* Coordinates in winPtr. */
    int tmpx, tmpy, bd;
    TkDisplay *dispPtr;

    /*
     * Step 1: find the top-level window that contains the desired point.
     */
     
    where.h = rootX;
    where.v = rootY;
    FindWindow(where, &whichWin);
    if (whichWin == NULL) {
	return NULL;
    }
    rootChild = TkMacGetXWindow(whichWin);
    dispPtr = TkGetDisplayList();
    winPtr = (TkWindow *) Tk_IdToWindow(dispPtr->display, rootChild);
    if (winPtr == NULL) {
        return NULL;
    }

    /*
     * Step 2: work down through the hierarchy underneath this window.
     * At each level, scan through all the children to find the highest
     * one in the stacking order that contains the point.  Then repeat
     * the whole process on that child.
     */

    x = rootX - winPtr->wmInfoPtr->xInParent;
    y = rootY - winPtr->wmInfoPtr->yInParent;
    while (1) {
	x -= winPtr->changes.x;
	y -= winPtr->changes.y;
	nextPtr = NULL;
	
	/*
	 * Container windows cannot have children.  So if it is a container,
	 * look there, otherwise inspect the children.
	 */
	 
	if (Tk_IsContainer(winPtr)) {
	    childPtr = TkpGetOtherWindow(winPtr);
	    if (childPtr != NULL) {
		if (Tk_IsMapped(childPtr)) {
	            tmpx = x - childPtr->changes.x;
	            tmpy = y - childPtr->changes.y;
	            bd = childPtr->changes.border_width;
	    	          
	            if ((tmpx >= -bd) && (tmpy >= -bd)
		        && (tmpx < (childPtr->changes.width + bd))
		        && (tmpy < (childPtr->changes.height + bd))) {
		        nextPtr = childPtr;
	            }
	    	}
	    }
	    

	    /*
	     * NOTE: Here we should handle out of process embedding.
	     */
	
	} else {
	    for (childPtr = winPtr->childList; childPtr != NULL;
		    childPtr = childPtr->nextPtr) {
	        if (!Tk_IsMapped(childPtr) ||
			(childPtr->flags & TK_TOP_LEVEL)) {
		    continue;
	        }
	        tmpx = x - childPtr->changes.x;
	        tmpy = y - childPtr->changes.y;
	        bd = childPtr->changes.border_width;
	        if ((tmpx >= -bd) && (tmpy >= -bd)
		        && (tmpx < (childPtr->changes.width + bd))
		        && (tmpy < (childPtr->changes.height + bd))) {
		    nextPtr = childPtr;
	        }
	    }
	}
	if (nextPtr == NULL) {
	    break;
	}
	winPtr = nextPtr;
    }
    return (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TopCoordsToWindow --
 *
 *	Given a Tk Window, and coordinates of a point relative to that window
 *      this procedure returns the top-most child of the window (excluding
 *      toplevels) covering that point, if there exists such a window in this
 *	application.
 *	It also sets newX, and newY to the coords of the point relative to the
 *      window returned.
 *
 * Results:
 *	The return result is either a token for the window corresponding
 *	to rootX and rootY, or else NULL to indicate that there is no such
 *	window.  newX and newY are also set to the coords of the point relative
 *      to the returned window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_TopCoordsToWindow(
    Tk_Window tkwin,		/* Token for a Tk Window which defines the;
				 * coordinates for rootX & rootY */
    int rootX, int rootY,	/* Coordinates of a point in tkWin. */
    int *newX, int *newY)	/* Coordinates of point in the upperMost child of
                                 * tkWin containing (rootX,rootY) */
{
    register TkWindow *winPtr, *childPtr;
    TkWindow *nextPtr;		/* Coordinates of highest child found so
				 * far that contains point. */
    int x, y;			/* Coordinates in winPtr. */
    Window *children;		/* Children of winPtr, or NULL. */

    winPtr = (TkWindow *) tkwin;
    x = rootX;
    y = rootY;
    while (1) {
	nextPtr = NULL;
	children = NULL;

	/*
	 * Container windows cannot have children.  So if it is a container,
	 * look there, otherwise inspect the children.
	 */
	 
	if (Tk_IsContainer(winPtr)) {
	    childPtr = TkpGetOtherWindow(winPtr);
	    if (childPtr != NULL) {
		if (Tk_IsMapped(childPtr) && 
	    	         (x > childPtr->changes.x && 
	    	             x < childPtr->changes.x +
				 childPtr->changes.width) &&
	    	         (y > childPtr->changes.y &&
	    	             y < childPtr->changes.y +
				 childPtr->changes.height)) {	    
	    	    nextPtr = childPtr;
	    	}
	    }

	    /*
	     * NOTE: Here we should handle out of process embedding.
	     */
	
	} else {
	
	    for (childPtr = winPtr->childList; childPtr != NULL;
					   childPtr = childPtr->nextPtr) {
	        if (!Tk_IsMapped(childPtr) ||
			(childPtr->flags & TK_TOP_LEVEL)) {
		    continue;
	        }
	        if (x < childPtr->changes.x || y < childPtr->changes.y) {
		    continue;
	        }
	        if (x > childPtr->changes.x + childPtr->changes.width ||
		        y > childPtr->changes.y + childPtr->changes.height) {
		    continue;
	        }
	        nextPtr = childPtr;
	    }
	}
	if (nextPtr == NULL) {
	    break;
	}
	winPtr = nextPtr;
	x -= winPtr->changes.x;
	y -= winPtr->changes.y;
    }
    *newX = x;
    *newY = y;
    return (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateVRootGeometry --
 *
 *	This procedure is called to update all the virtual root
 *	geometry information in wmPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The vRootX, vRootY, vRootWidth, and vRootHeight fields in
 *	wmPtr are filled with the most up-to-date information.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateVRootGeometry(
    WmInfo *wmPtr)		/* Window manager information to be
				 * updated.  The wmPtr->vRoot field must
				 * be valid. */
{
    TkWindow *winPtr = wmPtr->winPtr;
    unsigned int bd, dummy;
    Window dummy2;
    Status status;
    Tk_ErrorHandler handler;

    /*
     * If this isn't a virtual-root window manager, just return information
     * about the screen.
     */

    wmPtr->flags &= ~WM_VROOT_OFFSET_STALE;
    if (wmPtr->vRoot == None) {
	noVRoot:
	wmPtr->vRootX = wmPtr->vRootY = 0;
	wmPtr->vRootWidth = DisplayWidth(winPtr->display, winPtr->screenNum);
	wmPtr->vRootHeight = DisplayHeight(winPtr->display, winPtr->screenNum);
	return;
    }

    /*
     * Refresh the virtual root information if it's out of date.
     */

    handler = Tk_CreateErrorHandler(winPtr->display, -1, -1, -1,
	    (Tk_ErrorProc *) NULL, (ClientData) NULL);
    status = XGetGeometry(winPtr->display, wmPtr->vRoot,
	    &dummy2, &wmPtr->vRootX, &wmPtr->vRootY,
	    &wmPtr->vRootWidth, &wmPtr->vRootHeight, &bd, &dummy);
    if (wmTracing) {
	printf("UpdateVRootGeometry: x = %d, y = %d, width = %d, ",
		wmPtr->vRootX, wmPtr->vRootY, wmPtr->vRootWidth);
	printf("height = %d, status = %d\n", wmPtr->vRootHeight, status);
    }
    Tk_DeleteErrorHandler(handler);
    if (status == 0) {
	/*
	 * The virtual root is gone!  Pretend that it never existed.
	 */

	wmPtr->vRoot = None;
	goto noVRoot;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetVRootGeometry --
 *
 *	This procedure returns information about the virtual root
 *	window corresponding to a particular Tk window.
 *
 * Results:
 *	The values at xPtr, yPtr, widthPtr, and heightPtr are set
 *	with the offset and dimensions of the root window corresponding
 *	to tkwin.  If tkwin is being managed by a virtual root window
 *	manager these values correspond to the virtual root window being
 *	used for tkwin;  otherwise the offsets will be 0 and the
 *	dimensions will be those of the screen.
 *
 * Side effects:
 *	Vroot window information is refreshed if it is out of date.
 *
 *----------------------------------------------------------------------
 */

void
Tk_GetVRootGeometry(
    Tk_Window tkwin,		/* Window whose virtual root is to be
				 * queried. */
    int *xPtr, int *yPtr,	/* Store x and y offsets of virtual root
				 * here. */
    int *widthPtr,		/* Store dimensions of virtual root here. */
    int *heightPtr)
{
    WmInfo *wmPtr;
    TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * Find the top-level window for tkwin, and locate the window manager
     * information for that window.
     */

    while (!(winPtr->flags & TK_TOP_LEVEL)) {
	winPtr = winPtr->parentPtr;
    }
    wmPtr = winPtr->wmInfoPtr;

    /*
     * Make sure that the geometry information is up-to-date, then copy
     * it out to the caller.
     */

    if (wmPtr->flags & WM_VROOT_OFFSET_STALE) {
	UpdateVRootGeometry(wmPtr);
    }
    *xPtr = wmPtr->vRootX;
    *yPtr = wmPtr->vRootY;
    *widthPtr = wmPtr->vRootWidth;
    *heightPtr = wmPtr->vRootHeight;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MoveToplevelWindow --
 *
 *	This procedure is called instead of Tk_MoveWindow to adjust
 *	the x-y location of a top-level window.  It delays the actual
 *	move to a later time and keeps window-manager information
 *	up-to-date with the move
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is eventually moved so that its upper-left corner
 *	(actually, the upper-left corner of the window's decorative
 *	frame, if there is one) is at (x,y).
 *
 *----------------------------------------------------------------------
 */

void
Tk_MoveToplevelWindow(
    Tk_Window tkwin,		/* Window to move. */
    int x, int y)		/* New location for window (within
				 * parent). */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	panic("Tk_MoveToplevelWindow called with non-toplevel window");
    }
    wmPtr->x = x;
    wmPtr->y = y;
    wmPtr->flags |= WM_MOVE_PENDING;
    wmPtr->flags &= ~(WM_NEGATIVE_X|WM_NEGATIVE_Y);
    if ((wmPtr->sizeHintsFlags & (USPosition|PPosition)) == 0) {
	wmPtr->sizeHintsFlags |= USPosition;
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    }

    /*
     * If the window has already been mapped, must bring its geometry
     * up-to-date immediately, otherwise an event might arrive from the
     * server that would overwrite wmPtr->x and wmPtr->y and lose the
     * new position.
     */

    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	if (wmPtr->flags & WM_UPDATE_PENDING) {
	    Tk_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
	}
	UpdateGeometryInfo((ClientData) winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmProtocolEventProc --
 *
 *	This procedure is called by the Tk_HandleEvent whenever a
 *	ClientMessage event arrives whose type is "WM_PROTOCOLS".
 *	This procedure handles the message from the window manager
 *	in an appropriate fashion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what sort of handler, if any, was set up for the
 *	protocol.
 *
 *----------------------------------------------------------------------
 */

void
TkWmProtocolEventProc(
    TkWindow *winPtr,		/* Window to which the event was sent. */
    XEvent *eventPtr)		/* X event. */
{
    WmInfo *wmPtr;
    register ProtocolHandler *protPtr;
    Tcl_Interp *interp;
    Atom protocol;
    int result;

    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }
    protocol = (Atom) eventPtr->xclient.data.l[0];
    for (protPtr = wmPtr->protPtr; protPtr != NULL;
				   protPtr = protPtr->nextPtr) {
	if (protocol == protPtr->protocol) {
	    Tcl_Preserve((ClientData) protPtr);
            interp = protPtr->interp;
            Tcl_Preserve((ClientData) interp);
	    result = Tcl_GlobalEval(interp, protPtr->command);
	    if (result != TCL_OK) {
		Tcl_AddErrorInfo(interp, "\n    (command for \"");
		Tcl_AddErrorInfo(interp,
			Tk_GetAtomName((Tk_Window) winPtr, protocol));
		Tcl_AddErrorInfo(interp, "\" window manager protocol)");
		Tk_BackgroundError(interp);
	    }
            Tcl_Release((ClientData) interp);
	    Tcl_Release((ClientData) protPtr);
	    return;
	}
    }

    /*
     * No handler was present for this protocol.  If this is a
     * WM_DELETE_WINDOW message then just destroy the window.
     */

    if (protocol == Tk_InternAtom((Tk_Window) winPtr, "WM_DELETE_WINDOW")) {
	Tk_DestroyWindow((Tk_Window) winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmRestackToplevel --
 *
 *	This procedure restacks a top-level window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WinPtr gets restacked  as specified by aboveBelow and otherPtr.
 *	This procedure doesn't return until the restack has taken
 *	effect and the ConfigureNotify event for it has been received.
 *
 *----------------------------------------------------------------------
 */

void
TkWmRestackToplevel(
    TkWindow *winPtr,		/* Window to restack. */
    int aboveBelow,		/* Gives relative position for restacking;
				 * must be Above or Below. */
    TkWindow *otherPtr)		/* Window relative to which to restack;
				 * if NULL, then winPtr gets restacked
				 * above or below *all* siblings. */
{
    WmInfo *wmPtr;
    WindowPeek macWindow, otherMacWindow, frontWindow;

    wmPtr = winPtr->wmInfoPtr;
    
    /*
     * Get the mac window.  Make sure it exists & is mapped.
     */
    
    if (winPtr->window == None) {
	Tk_MakeWindowExist((Tk_Window) winPtr);
    }
    if (winPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {

	/*
	 * Can't set stacking order properly until the window is on the
	 * screen (mapping it may give it a reparent window), so make sure
	 * it's on the screen.
	 */

	TkWmMapWindow(winPtr);
    }
    macWindow = (WindowPeek) TkMacGetDrawablePort(winPtr->window);
    
    /*
     * Get the window in which a raise or lower is in relation to.
     */
    if (otherPtr != NULL) {
	if (otherPtr->window == None) {
	    Tk_MakeWindowExist((Tk_Window) otherPtr);
	}
	if (otherPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	    TkWmMapWindow(otherPtr);
	}
	otherMacWindow = (WindowPeek) TkMacGetDrawablePort(otherPtr->window);
    } else {
	otherMacWindow = NULL;
    }
	
    frontWindow = (WindowPeek) FrontWindow();
    if (aboveBelow == Above) {
	if (macWindow == frontWindow) {
	    /* 
	     * Do nothing - it's already at the top.
	     */
	} else if (otherMacWindow == frontWindow || otherMacWindow == NULL) {
	    /*
	     * Raise the window to the top.  If the window is visable then
	     * we also make it the active window.
	     */

	    if (wmPtr->hints.initial_state == WithdrawnState) {
		BringToFront((WindowPtr) macWindow);
	    } else {
		SelectWindow((WindowPtr)  macWindow);
	    }
	} else {
	    /*
	     * Find the window to be above.  (Front window will actually be the
	     * window to be behind.)  Front window is NULL if no other windows.
	     */
	    while (frontWindow != NULL &&
		    frontWindow->nextWindow != otherMacWindow) {
		frontWindow = frontWindow->nextWindow;
	    }
	    if (frontWindow != NULL) {
		SendBehind((WindowPtr) macWindow, (WindowPtr) frontWindow);
	    }
	}
    } else {
	/*
	 * Send behind.  If it was in front find another window to make active.
	 */
	if (macWindow == frontWindow) {
	    if (macWindow->nextWindow != NULL) {
		SelectWindow((WindowPtr)  macWindow->nextWindow);
	    }
	}
	SendBehind((WindowPtr) macWindow, (WindowPtr) otherMacWindow);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmAddToColormapWindows --
 *
 *	This procedure is called to add a given window to the
 *	WM_COLORMAP_WINDOWS property for its top-level, if it
 *	isn't already there.  It is invoked by the Tk code that
 *	creates a new colormap, in order to make sure that colormap
 *	information is propagated to the window manager by default.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WinPtr's window gets added to the WM_COLORMAP_WINDOWS
 *	property of its nearest top-level ancestor, unless the
 *	colormaps have been set explicitly with the
 *	"wm colormapwindows" command.
 *
 *----------------------------------------------------------------------
 */

void
TkWmAddToColormapWindows(
    TkWindow *winPtr)		/* Window with a non-default colormap.
				 * Should not be a top-level window. */
{
    TkWindow *topPtr;
    TkWindow **oldPtr, **newPtr;
    int count, i;

    if (winPtr->window == None) {
	return;
    }

    for (topPtr = winPtr->parentPtr; ; topPtr = topPtr->parentPtr) {
	if (topPtr == NULL) {
	    /*
	     * Window is being deleted.  Skip the whole operation.
	     */

	    return;
	}
	if (topPtr->flags & TK_TOP_LEVEL) {
	    break;
	}
    }
    if (topPtr->wmInfoPtr->flags & WM_COLORMAPS_EXPLICIT) {
	return;
    }

    /*
     * Make sure that the window isn't already in the list.
     */

    count = topPtr->wmInfoPtr->cmapCount;
    oldPtr = topPtr->wmInfoPtr->cmapList;

    for (i = 0; i < count; i++) {
	if (oldPtr[i] == winPtr) {
	    return;
	}
    }

    /*
     * Make a new bigger array and use it to reset the property.
     * Automatically add the toplevel itself as the last element
     * of the list.
     */

    newPtr = (TkWindow **) ckalloc((unsigned) ((count+2)*sizeof(TkWindow*)));
    if (count > 0) {
	memcpy(newPtr, oldPtr, count * sizeof(TkWindow*));
    }
    if (count == 0) {
	count++;
    }
    newPtr[count-1] = winPtr;
    newPtr[count] = topPtr;
    if (oldPtr != NULL) {
	ckfree((char *) oldPtr);
    }

    topPtr->wmInfoPtr->cmapList = newPtr;
    topPtr->wmInfoPtr->cmapCount = count+1;

    /*
     * On the Macintosh all of this is just an excercise
     * in compatability as we don't support colormaps.  If 
     * we did they would be installed here.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmRemoveFromColormapWindows --
 *
 *	This procedure is called to remove a given window from the
 *	WM_COLORMAP_WINDOWS property for its top-level.  It is invoked
 *	when windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WinPtr's window gets removed from the WM_COLORMAP_WINDOWS
 *	property of its nearest top-level ancestor, unless the
 *	top-level itself is being deleted too.
 *
 *----------------------------------------------------------------------
 */

void
TkWmRemoveFromColormapWindows(
    TkWindow *winPtr)		/* Window that may be present in
				 * WM_COLORMAP_WINDOWS property for its
				 * top-level.  Should not be a top-level
				 * window. */
{
    TkWindow *topPtr;
    TkWindow **oldPtr;
    int count, i, j;

    for (topPtr = winPtr->parentPtr; ; topPtr = topPtr->parentPtr) {
	if (topPtr == NULL) {
	    /*
	     * Ancestors have been deleted, so skip the whole operation.
	     * Seems like this can't ever happen?
	     */

	    return;
	}
	if (topPtr->flags & TK_TOP_LEVEL) {
	    break;
	}
    }
    if (topPtr->flags & TK_ALREADY_DEAD) {
	/*
	 * Top-level is being deleted, so there's no need to cleanup
	 * the WM_COLORMAP_WINDOWS property.
	 */

	return;
    }

    /*
     * Find the window and slide the following ones down to cover
     * it up.
     */

    count = topPtr->wmInfoPtr->cmapCount;
    oldPtr = topPtr->wmInfoPtr->cmapList;
    for (i = 0; i < count; i++) {
	if (oldPtr[i] == winPtr) {
	    for (j = i ; j < count-1; j++) {
		oldPtr[j] = oldPtr[j+1];
	    }
	    topPtr->wmInfoPtr->cmapCount = count - 1;
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetPointerCoords --
 *
 *	Fetch the position of the mouse pointer.
 *
 * Results:
 *	*xPtr and *yPtr are filled in with the (virtual) root coordinates
 *	of the mouse pointer for tkwin's display.  If the pointer isn't
 *	on tkwin's screen, then -1 values are returned for both
 *	coordinates.  The argument tkwin must be a toplevel window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkGetPointerCoords(
    Tk_Window tkwin,		/* Toplevel window that identifies screen
				 * on which lookup is to be done. */
    int *xPtr, int *yPtr)	/* Store pointer coordinates here. */
{
    Point where;

    GetMouse(&where);
    LocalToGlobal(&where);
    *xPtr = where.h;
    *yPtr = where.v;
}

/*
 *----------------------------------------------------------------------
 *
 * InitialWindowBounds --
 *
 *	This function calculates the initial bounds for a new Mac
 *	toplevel window.  Unless the geometry is specified by the user
 *	this code will auto place the windows in a cascade diagonially
 *	across the main monitor of the Mac.
 *
 * Results:
 *	The bounds are returned in geometry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
InitialWindowBounds(
    TkWindow *winPtr,		/* Window to get initial bounds for. */
    Rect *geometry)		/* On return the initial bounds. */
{
    int x, y;
    static int defaultX = 5;
    static int defaultY = 45;
	
    if (!(winPtr->wmInfoPtr->sizeHintsFlags & (USPosition | PPosition))) {
	/* 
	 * We will override the program & hopefully place the
	 * window in a "better" location.
	 */
		
	if (((tcl_macQdPtr->screenBits.bounds.right - defaultX) < 30) ||
		((tcl_macQdPtr->screenBits.bounds.bottom - defaultY) < 30)) {
	    defaultX = 5;
	    defaultY = 45;
	}
	x = defaultX;
	y = defaultY;
	defaultX += 20;
	defaultY += 20;
    } else {
	x = winPtr->wmInfoPtr->x;
	y = winPtr->wmInfoPtr->y;
    }
	
    geometry->left = x;
    geometry->top = y;
    geometry->right = x + winPtr->changes.width;
    geometry->bottom = y + winPtr->changes.height;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacResizable --
 *
 *	This function determines if the passed in window is part of
 *	a toplevel window that is resizable.  If the window is 
 *	resizable in the x, y or both directions, true is returned.
 *
 * Results:
 *	True if resizable, false otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkMacResizable(
    TkWindow *winPtr)		/* Tk window or NULL. */
{
    WmInfo *wmPtr;

    if (winPtr == NULL) {
	return false;
    }
    while (winPtr->wmInfoPtr == NULL) {
	winPtr = winPtr->parentPtr;
    }
    
    wmPtr = winPtr->wmInfoPtr;
    if ((wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) &&
	    (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE)) {
	return false;
    } else {
	return true;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacGrowToplevel --
 *
 *	The function is invoked when the user clicks in the grow region
 *	of a Tk window.  The function will handle the dragging
 *	procedure and not return until completed.  Finally, the function
 *	may place information Tk's event queue is the window was resized.
 *
 * Results:
 *	True if events were placed on event queue, false otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkMacGrowToplevel(
    WindowPtr whichWindow,
    Point start)
{
    Point where = start;
    TkDisplay *dispPtr;

    GlobalToLocal(&where);
    if (where.h > (whichWindow->portRect.right - 16) &&
	    where.v > (whichWindow->portRect.bottom - 16)) {
		
	Window window;
	TkWindow *winPtr;
	WmInfo *wmPtr;
	Rect bounds;
	long growResult;

	window = TkMacGetXWindow(whichWindow);
	dispPtr = TkGetDisplayList();
	winPtr = (TkWindow *) Tk_IdToWindow(dispPtr->display, window);
	wmPtr = winPtr->wmInfoPtr;
	
	/* TODO: handle grid size options. */
	if ((wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) &&
		(wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE)) {
	    return false;
	}
	if (wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) {
	    bounds.left = bounds.right = winPtr->changes.width;
	} else {
	    bounds.left = (wmPtr->minWidth < 64) ? 64 : wmPtr->minWidth;
	    bounds.right = (wmPtr->maxWidth < 64) ? 64 : wmPtr->maxWidth;
	}
	if (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE) {
	    bounds.top = bounds.bottom = winPtr->changes.height;
	} else {
	    bounds.top = (wmPtr->minHeight < 64) ? 64 : wmPtr->minHeight;
	    bounds.bottom = (wmPtr->maxHeight < 64) ? 64 : wmPtr->maxHeight;
	}
	
	growResult = GrowWindow(whichWindow, start, &bounds);

	if (growResult != 0) {
	    SizeWindow(whichWindow,
		    LoWord(growResult), HiWord(growResult), true);
	    SetPort(whichWindow);
	    InvalRect(&whichWindow->portRect);	/* TODO: may not be needed */
	    TkMacInvalClipRgns(winPtr);
	    TkGenWMConfigureEvent((Tk_Window) winPtr, -1, -1, 
		    (int) LoWord(growResult), (int) HiWord(growResult),
		    TK_SIZE_CHANGED);
	    return true;
	}
	return false;
    }
    return false;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetWMName --
 *
 *	Set the title for a toplevel window.  If the window is embedded, 
 *	do not change the window title.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The title of the window is changed.
 *
 *----------------------------------------------------------------------
 */

void
TkSetWMName(
    TkWindow *winPtr,
    Tk_Uid titleUid)
{
    Str255  pTitle;
    GWorldPtr macWin;
    int destWrote;
    
    if (Tk_IsEmbedded(winPtr)) {
        return;
    }
    Tcl_UtfToExternal(NULL, NULL, titleUid,
	    strlen(titleUid), 0, NULL, 
	    (char *) &pTitle[1],
	    255, NULL, &destWrote, NULL); /* Internalize native */
    pTitle[0] = destWrote;
    
    macWin = TkMacGetDrawablePort(winPtr->window);

    SetWTitle((WindowPtr) macWin, pTitle);
}

void
TkGenWMDestroyEvent(
    Tk_Window tkwin)
{
    XEvent event;
    
    event.xany.serial = Tk_Display(tkwin)->request;
    event.xany.send_event = False;
    event.xany.display = Tk_Display(tkwin);
	
    event.xclient.window = Tk_WindowId(tkwin);
    event.xclient.type = ClientMessage;
    event.xclient.message_type = Tk_InternAtom(tkwin, "WM_PROTOCOLS");
    event.xclient.format = 32;
    event.xclient.data.l[0] = Tk_InternAtom(tkwin, "WM_DELETE_WINDOW");
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenWMConfigureEvent --
 *
 *	Generate a ConfigureNotify event for Tk.  Depending on the 
 *	value of flag the values of width/height, x/y, or both may
 *	be changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A ConfigureNotify event is sent to Tk.
 *
 *----------------------------------------------------------------------
 */

void
TkGenWMConfigureEvent(
    Tk_Window tkwin,
    int x,
    int y,
    int width,
    int height,
    int flags)
{
    XEvent event;
    WmInfo *wmPtr;
    TkWindow *winPtr = (TkWindow *) tkwin;
    
    if (tkwin == NULL) {
	return;
    }
    
    event.type = ConfigureNotify;
    event.xconfigure.serial = Tk_Display(tkwin)->request;
    event.xconfigure.send_event = False;
    event.xconfigure.display = Tk_Display(tkwin);
    event.xconfigure.event = Tk_WindowId(tkwin);
    event.xconfigure.window = Tk_WindowId(tkwin);
    event.xconfigure.border_width = winPtr->changes.border_width;
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    if (winPtr->changes.stack_mode == Above) {
	event.xconfigure.above = winPtr->changes.sibling;
    } else {
	event.xconfigure.above = None;
    }

    if (flags & TK_LOCATION_CHANGED) {
	event.xconfigure.x = x;
	event.xconfigure.y = y;
    } else {
	event.xconfigure.x = Tk_X(tkwin);
	event.xconfigure.y = Tk_Y(tkwin);
	x = Tk_X(tkwin);
	y = Tk_Y(tkwin);
    }
    if (flags & TK_SIZE_CHANGED) {
	event.xconfigure.width = width;
	event.xconfigure.height = height;
    } else {
	event.xconfigure.width = Tk_Width(tkwin);
	event.xconfigure.height = Tk_Height(tkwin);
	width = Tk_Width(tkwin);
	height = Tk_Height(tkwin);
    }
    
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    
    /*
     * Update window manager information.
     */
    if (Tk_IsTopLevel(winPtr)) {
	wmPtr = winPtr->wmInfoPtr;
	if (flags & TK_LOCATION_CHANGED) {
	    wmPtr->x = x;
	    wmPtr->y = y;
	    wmPtr->flags &= ~(WM_NEGATIVE_X | WM_NEGATIVE_Y);
	}
	if ((flags & TK_SIZE_CHANGED) && 
		((width != Tk_Width(tkwin)) || (height != Tk_Height(tkwin)))) {
	    if ((wmPtr->width == -1) && (width == winPtr->reqWidth)) {
		/*
		 * Don't set external width, since the user didn't change it
		 * from what the widgets asked for.
		 */
	    } else {
		if (wmPtr->gridWin != NULL) {
		    wmPtr->width = wmPtr->reqGridWidth
			+ (width - winPtr->reqWidth)/wmPtr->widthInc;
		    if (wmPtr->width < 0) {
			wmPtr->width = 0;
		    }
		} else {
		    wmPtr->width = width;
		}
	    }
	    if ((wmPtr->height == -1) && (height == winPtr->reqHeight)) {
		/*
		 * Don't set external height, since the user didn't change it
		 * from what the widgets asked for.
		 */
	    } else {
		if (wmPtr->gridWin != NULL) {
		    wmPtr->height = wmPtr->reqGridHeight
			+ (height - winPtr->reqHeight)/wmPtr->heightInc;
		    if (wmPtr->height < 0) {
			wmPtr->height = 0;
		    }
		} else {
		    wmPtr->height = height;
		}
	    }
	    wmPtr->configWidth = width;
	    wmPtr->configHeight = height;
	}
    }
    
    /*
     * Now set up the changes structure.  Under X we wait for the
     * ConfigureNotify to set these values.  On the Mac we know imediatly that
     * this is what we want - so we just set them.  However, we need to
     * make sure the windows clipping region is marked invalid so the
     * change is visable to the subwindow.
     */
    winPtr->changes.x = x;
    winPtr->changes.y = y;
    winPtr->changes.width = width;
    winPtr->changes.height = height;
    TkMacInvalClipRgns(winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetTransientMaster --
 *
 *	If the passed window has the TRANSIENT_FOR property set this
 *	will return the master window.  Otherwise it will return None.
 *
 * Results:
 *      The master window or None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Window
TkGetTransientMaster(
    TkWindow *winPtr)
{
    if (winPtr->wmInfoPtr != NULL) {
	return winPtr->wmInfoPtr->master;
    }
    return None;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacGetXWindow --
 *
 *	Returns the X window Id associated with the given WindowRef.
 *
 * Results:
 *	The window id is returned.  None is returned if not a Tk window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Window
TkMacGetXWindow(
    WindowRef macWinPtr)
{
    register Tcl_HashEntry *hPtr;

    if ((macWinPtr == NULL) || !windowHashInit) {
	return None;
    }
    hPtr = Tcl_FindHashEntry(&windowTable, (char *) macWinPtr);
    if (hPtr == NULL) {
	return None;
    }
    return (Window) Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacZoomToplevel --
 *
 *	The function is invoked when the user clicks in the zoom region
 *	of a Tk window.  The function will handle the mouse tracking
 *	for the interaction.  If the window is to be zoomed the window
 *	size is changed and events are generated to let Tk know what
 *	happened.
 *
 * Results:
 *	True if events were placed on event queue, false otherwise.
 *
 * Side effects:
 *	The window may be resized & events placed on Tk's queue.
 *
 *----------------------------------------------------------------------
 */

int
TkMacZoomToplevel(
    WindowPtr whichWindow,	/* The Macintosh window to zoom. */
    Point where,		/* The current mouse position. */
    short zoomPart)		/* Either inZoomIn or inZoomOut */
{
    Window window;
    Tk_Window tkwin;
    Point location = {0, 0};
    int xOffset, yOffset;
    WmInfo *wmPtr;
    TkDisplay *dispPtr;

    SetPort(whichWindow);
    if (!TrackBox(whichWindow, where, zoomPart)) {
	return false;
    }

    /*
     * We should now zoom the window (as long as it's one of ours).  We 
     * also need to generate an event to let Tk know that the window size 
     * has changed.
     */
    window = TkMacGetXWindow(whichWindow);
    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    if (tkwin == NULL) {
	return false;
    }

    /*
     * The following block of code works around a bug in the window
     * definition for Apple's floating windows.  The zoom behavior is
     * broken - we must manually set the standard state (by default
     * it's something like 1x1) and we must swap the zoomPart manually
     * otherwise we always get the same zoomPart and nothing happens.
     */
    wmPtr = ((TkWindow *) tkwin)->wmInfoPtr;
    if (wmPtr->style >= floatProc && wmPtr->style <= floatSideZoomGrowProc) {
	if (zoomPart == inZoomIn) {
	    Rect zoomRect = tcl_macQdPtr->screenBits.bounds;
	    InsetRect(&zoomRect, 60, 60);
	    SetWindowStandardState(whichWindow, &zoomRect);
	    zoomPart = inZoomOut;
	} else {
	    zoomPart = inZoomIn;
	}
    }
    
    ZoomWindow(whichWindow, zoomPart, false);
    InvalRect(&whichWindow->portRect);
    TkMacInvalClipRgns((TkWindow *) tkwin);

    LocalToGlobal(&location);
    TkMacWindowOffset(whichWindow, &xOffset, &yOffset);
    location.h -= xOffset;
    location.v -= yOffset;
    TkGenWMConfigureEvent(tkwin, location.h, location.v, 
	    whichWindow->portRect.right - whichWindow->portRect.left,
	    whichWindow->portRect.bottom - whichWindow->portRect.top,
	    TK_BOTH_CHANGED);
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnsupported1Cmd --
 *
 *	This procedure is invoked to process the "unsupported1" Tcl 
 *	command.  This command allows you to set the style of decoration
 *	for a Macintosh window.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Changes the style of a new Mac window.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkUnsupported1Cmd(
    ClientData clientData,	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    char **argv)		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr;
    register WmInfo *wmPtr;
    int c;
    size_t length;

    if (argc < 3) {
	wrongNumArgs:
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option window ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

    winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[2], tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	Tcl_AppendResult(interp, "window \"", winPtr->pathName,
		"\" isn't a top-level window", (char *) NULL);
	return TCL_ERROR;
    }
    wmPtr = winPtr->wmInfoPtr;
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 's')  && (strncmp(argv[1], "style", length) == 0)) {
	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " style window ?windowStyle?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    switch (wmPtr->style) {
		case noGrowDocProc:
		case documentProc:
		    Tcl_SetResult(interp, "documentProc", TCL_STATIC);
		    break;
		case dBoxProc:
		    Tcl_SetResult(interp, "dBoxProc", TCL_STATIC);
		    break;
		case plainDBox:
		    Tcl_SetResult(interp, "plainDBox", TCL_STATIC);
		    break;
		case altDBoxProc:
		    Tcl_SetResult(interp, "altDBoxProc", TCL_STATIC);
		    break;
		case movableDBoxProc:
		    Tcl_SetResult(interp, "movableDBoxProc", TCL_STATIC);
		    break;
		case zoomDocProc:
		case zoomNoGrow:
		    Tcl_SetResult(interp, "zoomDocProc", TCL_STATIC);
		    break;
		case rDocProc:
		    Tcl_SetResult(interp, "rDocProc", TCL_STATIC);
		    break;
		case floatProc:
		case floatGrowProc:
		    Tcl_SetResult(interp, "floatProc", TCL_STATIC);
		    break;
		case floatZoomProc:
		case floatZoomGrowProc:
		    Tcl_SetResult(interp, "floatZoomProc", TCL_STATIC);
		    break;
		case floatSideProc:
		case floatSideGrowProc:
		    Tcl_SetResult(interp, "floatSideProc", TCL_STATIC);
		    break;
		case floatSideZoomProc:
		case floatSideZoomGrowProc:
		    Tcl_SetResult(interp, "floatSideZoomProc", TCL_STATIC);
		    break;
		default:
		   panic("invalid style");
	    }
	    return TCL_OK;
	}
	if (strcmp(argv[3], "documentProc") == 0) {
	    wmPtr->style = documentProc;
	} else if (strcmp(argv[3], "noGrowDocProc") == 0) {
	    wmPtr->style = documentProc;
	} else if (strcmp(argv[3], "dBoxProc") == 0) {
	    wmPtr->style = dBoxProc;
	} else if (strcmp(argv[3], "plainDBox") == 0) {
	    wmPtr->style = plainDBox;
	} else if (strcmp(argv[3], "altDBoxProc") == 0) {
	    wmPtr->style = altDBoxProc;
	} else if (strcmp(argv[3], "movableDBoxProc") == 0) {
	    wmPtr->style = movableDBoxProc;
	} else if (strcmp(argv[3], "zoomDocProc") == 0) {
	    wmPtr->style = zoomDocProc;
	} else if (strcmp(argv[3], "zoomNoGrow") == 0) {
	    wmPtr->style = zoomNoGrow;
	} else if (strcmp(argv[3], "rDocProc") == 0) {
	    wmPtr->style = rDocProc;
	} else if (strcmp(argv[3], "floatProc") == 0) {
	    wmPtr->style = floatGrowProc;
	} else if (strcmp(argv[3], "floatGrowProc") == 0) {
	    wmPtr->style = floatGrowProc;
	} else if (strcmp(argv[3], "floatZoomProc") == 0) {
	    wmPtr->style = floatZoomGrowProc;
	} else if (strcmp(argv[3], "floatZoomGrowProc") == 0) {
	    wmPtr->style = floatZoomGrowProc;
	} else if (strcmp(argv[3], "floatSideProc") == 0) {
	    wmPtr->style = floatSideGrowProc;
	} else if (strcmp(argv[3], "floatSideGrowProc") == 0) {
	    wmPtr->style = floatSideGrowProc;
	} else if (strcmp(argv[3], "floatSideZoomProc") == 0) {
	    wmPtr->style = floatSideZoomGrowProc;
	} else if (strcmp(argv[3], "floatSideZoomGrowProc") == 0) {
	    wmPtr->style = floatSideZoomGrowProc;
	} else {
	    Tcl_AppendResult(interp, "bad style: should be documentProc, ",
	    	    "dBoxProc, plainDBox, altDBoxProc, movableDBoxProc, ",
	    	    "zoomDocProc, rDocProc, floatProc, floatZoomProc, ",
		    "floatSideProc, or floatSideZoomProc",
		    (char *) NULL);
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "unknown or ambiguous option \"", argv[1],
		"\": must be style",
		(char *) NULL);
	return TCL_ERROR;
    }
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeMenuWindow --
 *
 *	Configure the window to be either a undecorated pull-down 
 *	(or pop-up) menu, or as a toplevel floating menu (palette).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the style bit used to create a new Mac toplevel.
 *
 *----------------------------------------------------------------------
 */

void
TkpMakeMenuWindow(
    Tk_Window tkwin,		/* New window. */
    int transient)		/* 1 means menu is only posted briefly as
				 * a popup or pulldown or cascade.  0 means
				 * menu is always visible, e.g. as a 
				 * floating menu. */
{
    if (transient) {
	((TkWindow *) tkwin)->wmInfoPtr->style = plainDBox;
    } else {
	((TkWindow *) tkwin)->wmInfoPtr->style = floatProc;
	((TkWindow *) tkwin)->wmInfoPtr->flags |= WM_WIDTH_NOT_RESIZABLE;
	((TkWindow *) tkwin)->wmInfoPtr->flags |= WM_HEIGHT_NOT_RESIZABLE;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacMakeRealWindowExist --
 *
 *	This function finally creates the real Macintosh window that
 *	the Mac actually understands.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new Macintosh toplevel is created.
 *
 *----------------------------------------------------------------------
 */

void 
TkMacMakeRealWindowExist(
    TkWindow *winPtr)	/* Tk window. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    WindowRef newWindow = NULL;
    MacDrawable *macWin;
    Rect geometry;
    Tcl_HashEntry *valueHashPtr;
    int new;
    TkMacWindowList *listPtr;

    if (TkMacHostToplevelExists(winPtr)) {
    	return;
    }
    
    macWin = (MacDrawable *) winPtr->window;

    /*
     * If this is embedded, make sure its container's toplevel exists,
     * then return... 
     */
    
    if (Tk_IsEmbedded(winPtr)) {
	TkWindow *contWinPtr;

	contWinPtr = TkpGetOtherWindow(winPtr);
	if (contWinPtr != NULL) {
	    TkMacMakeRealWindowExist(contWinPtr->privatePtr->toplevel->winPtr);
	    macWin->flags |= TK_HOST_EXISTS;
	    return;
	} else if (gMacEmbedHandler != NULL) {
	    if (gMacEmbedHandler->containerExistProc != NULL) {
	        if (gMacEmbedHandler->containerExistProc((Tk_Window) winPtr) != TCL_OK) {
	           panic("ContainerExistProc could not make container");
	       }
	    }
	    return;
	} else {
	    panic("TkMacMakeRealWindowExist could not find container");
	}

	/*
	 * NOTE: Here we should handle out of process embedding.
	 */
	
    }
    
    InitialWindowBounds(winPtr, &geometry);
	
    newWindow = NewCWindow(NULL, &geometry, "\ptemp", false, 
	    (short) wmPtr->style, (WindowRef) -1, true, 0);
    if (newWindow == NULL) {
	panic("couldn't allocate new Mac window");
    }
    
    /*
     * Add this window to the list of toplevel windows.
     */
    
    listPtr = (TkMacWindowList *) ckalloc(sizeof(TkMacWindowList));
    listPtr->nextPtr = tkMacWindowListPtr;
    listPtr->winPtr = winPtr;
    tkMacWindowListPtr = listPtr;
    
    macWin->portPtr = (GWorldPtr) newWindow;
    tkMacMoveWindow(newWindow, (int) geometry.left, (int) geometry.top);
    SetPort((GrafPtr) newWindow);
	
    if (!windowHashInit) {
	Tcl_InitHashTable(&windowTable, TCL_ONE_WORD_KEYS);
	windowHashInit = true;
    }
    valueHashPtr = Tcl_CreateHashEntry(&windowTable,
	    (char *) newWindow, &new);
    if (!new) {
	panic("same macintosh window allocated twice!");
    }
    Tcl_SetHashValue(valueHashPtr, macWin);
    
    macWin->flags |= TK_HOST_EXISTS;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacRegisterOffScreenWindow --
 *
 *	This function adds the passed in Off Screen Port to the
 *	hash table that maps Mac windows to root X windows.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An entry is added to the windowTable hash table.
 *
 *----------------------------------------------------------------------
 */

void 
TkMacRegisterOffScreenWindow(
    Window window,	/* Window structure. */
    GWorldPtr portPtr)	/* Pointer to a Mac GWorld. */
{
    WindowRef newWindow = NULL;
    MacDrawable *macWin;
    Tcl_HashEntry *valueHashPtr;
    int new;

    macWin = (MacDrawable *) window;
    if (!windowHashInit) {
	Tcl_InitHashTable(&windowTable, TCL_ONE_WORD_KEYS);
	windowHashInit = true;
    }
    valueHashPtr = Tcl_CreateHashEntry(&windowTable,
	    (char *) portPtr, &new);
    if (!new) {
	panic("same macintosh window allocated twice!");
    }
    Tcl_SetHashValue(valueHashPtr, macWin);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacUnregisterMacWindow --
 *
 *	Given a macintosh port window, this function removes the 
 *	association between this window and the root X window that
 *	Tk cares about.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An entry is removed from the windowTable hash table.
 *
 *----------------------------------------------------------------------
 */

void 
TkMacUnregisterMacWindow(
    GWorldPtr portPtr)	/* Pointer to a Mac GWorld. */
{
    if (!windowHashInit) {
	panic("TkMacUnregisterMacWindow: unmapping before inited");;
    }
    Tcl_DeleteHashEntry(Tcl_FindHashEntry(&windowTable,
	(char *) portPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacSetScrollbarGrow --
 *
 *	Sets a flag for a toplevel window indicating that the passed
 *	Tk scrollbar window will display the grow region for the 
 *	toplevel window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A flag is set int windows toplevel parent.
 *
 *----------------------------------------------------------------------
 */

void 
TkMacSetScrollbarGrow(
    TkWindow *winPtr,		/* Tk scrollbar window. */
    int flag)			/* Boolean value true or false. */
{
    if (flag) {
	winPtr->privatePtr->toplevel->flags |= TK_SCROLLBAR_GROW;
	winPtr->privatePtr->toplevel->winPtr->wmInfoPtr->scrollWinPtr = winPtr;
    } else if (winPtr->privatePtr->toplevel->winPtr->wmInfoPtr->scrollWinPtr
	    == winPtr) {
	winPtr->privatePtr->toplevel->flags &= ~TK_SCROLLBAR_GROW;
	winPtr->privatePtr->toplevel->winPtr->wmInfoPtr->scrollWinPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacGetScrollbarGrowWindow --
 *
 *	Tests to see if a given window's toplevel window contains a
 *	scrollbar that will draw the GrowIcon for the window.
 *
 * Results:
 *	Boolean value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow * 
TkMacGetScrollbarGrowWindow(
    TkWindow *winPtr)	/* Tk window. */
{
    TkWindow *scrollWinPtr;
    
    if (winPtr == NULL) {
	return NULL;
    }
    scrollWinPtr =
	winPtr->privatePtr->toplevel->winPtr->wmInfoPtr->scrollWinPtr;
    if (winPtr != NULL) {
	/*
	 * We need to confirm the window exists.
	 */
	if ((Tk_Window) scrollWinPtr !=
		Tk_IdToWindow(winPtr->display, winPtr->window)) {
	    scrollWinPtr = NULL;
	}
    }
    return scrollWinPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmFocusToplevel --
 *
 *	This is a utility procedure invoked by focus-management code. It
 *	exists because of the extra wrapper windows that exist under
 *	Unix; its job is to map from wrapper windows to the
 *	corresponding toplevel windows.  On PCs and Macs there are no
 *	wrapper windows so no mapping is necessary;  this procedure just
 *	determines whether a window is a toplevel or not.
 *
 * Results:
 *	If winPtr is a toplevel window, returns the pointer to the
 *	window; otherwise returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkWmFocusToplevel(
    TkWindow *winPtr)		/* Window that received a focus-related
				 * event. */
{
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	return NULL;
    }
    return winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetWrapperWindow --
 *
 *	This is a utility procedure invoked by focus-management code. It
 *	maps to the wrapper for a top-level, which is just the same
 *	as the top-level on Macs and PCs.
 *
 * Results:
 *	If winPtr is a toplevel window, returns the pointer to the
 *	window; otherwise returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkpGetWrapperWindow(
    TkWindow *winPtr)		/* Window that received a focus-related
				 * event. */
{
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	return NULL;
    }
    return winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpWmSetState --
 *
 *	Sets the window manager state for the wrapper window of a
 *	given toplevel window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May maximize, minimize, restore, or withdraw a window.
 *
 *----------------------------------------------------------------------
 */

void
TkpWmSetState(winPtr, state)
     TkWindow *winPtr;		/* Toplevel window to operate on. */
     int state;			/* One of IconicState, ZoomState, NormalState,
				 * or WithdrawnState. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    GWorldPtr macWin;
    
    wmPtr->hints.initial_state = state;
    if (wmPtr->flags & WM_NEVER_MAPPED) {
	return;
    }

    macWin = TkMacGetDrawablePort(winPtr->window);

    if (state == WithdrawnState) {
	Tk_UnmapWindow((Tk_Window) winPtr);
    } else if (state == IconicState) {
	Tk_UnmapWindow((Tk_Window) winPtr);
	if (TkMacHaveAppearance()) {
	    /*
	     * The window always gets unmapped.  However, if we can show the
	     * icon version of the window (collapsed) we make the window visable
	     * and then collapse it.
	     *
	     * TODO: This approach causes flashing!
	     */

	    if (IsWindowCollapsable((WindowRef) macWin)) {
		ShowWindow((WindowRef) macWin);
		CollapseWindow((WindowPtr) macWin, true);
	    }
	}
    } else if (state == NormalState) {
	Tk_MapWindow((Tk_Window) winPtr);
	if (TkMacHaveAppearance()) {
	    CollapseWindow((WindowPtr) macWin, false);
	}
    } else if (state == ZoomState) {
	/* TODO: need to support zoomed windows */
    }
}
/*
 *----------------------------------------------------------------------
 *
 * TkMacHaveAppearance --
 *
 *	Determine if the appearance manager is available on this Mac.
 *	We cache the result so future calls are fast.  Return a different
 *      value if 1.0.1 is present, since many interfaces were added in
 *      1.0.1
 *
 * Results:
 *	1 if the appearance manager is present, 2 if the appearance
 *      manager version is 1.0.1 or greater, 0 if it is not present.
 *
 * Side effects:
 *	Calls Gestalt to query system values.
 *
 *----------------------------------------------------------------------
 */

int
TkMacHaveAppearance()
{
    static initialized = false;
    static int TkMacHaveAppearance = 0;
    long response = 0;
    OSErr err = noErr;
    
    if (!initialized) {
	err = Gestalt(gestaltAppearanceAttr, &response);
	if (err == noErr) {
	    TkMacHaveAppearance = 1;
	}
	err = Gestalt(gestaltAppearanceVersion, &response);
	if (err == noErr) {
	    TkMacHaveAppearance = 2;
	}
    }

    return TkMacHaveAppearance;
}
