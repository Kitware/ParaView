/*
 * tkUnixWm.c --
 *
 *	This module takes care of the interactions between a Tk-based
 *	application and the window manager.  Among other things, it
 *	implements the "wm" command and passes geometry information
 *	to the window manager.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkPort.h"
#include "tkInt.h"
#include "tkUnixInt.h"
#include <errno.h>

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
    char *title;		/* Title to display in window caption.  If
				 * NULL, use name of widget.  Malloced. */
    char *iconName;		/* Name to display in icon.  Malloced. */
    XWMHints hints;		/* Various pieces of information for
				 * window manager. */
    char *leaderName;		/* Path name of leader of window group
				 * (corresponds to hints.window_group).
				 * Malloc-ed.  Note:  this field doesn't
				 * get updated if leader is destroyed. */
    TkWindow *masterPtr;	/* Master window for TRANSIENT_FOR property,
				 * or NULL. */
    Tk_Window icon;		/* Window to use as icon for this window,
				 * or NULL. */
    Tk_Window iconFor;		/* Window for which this window is icon, or
				 * NULL if this isn't an icon for anyone. */
    int withdrawn;		/* Non-zero means window has been withdrawn. */

    /*
     * In order to support menubars transparently under X, each toplevel
     * window is encased in an additional window, called the wrapper,
     * that holds the toplevel and the menubar, if any.  The information
     * below is used to keep track of the wrapper and the menubar.
     */

    TkWindow *wrapperPtr;	/* Pointer to information about the wrapper.
				 * This is the "real" toplevel window as
				 * seen by the window manager. Although
				 * this is an official Tk window, it
				 * doesn't appear in the application's
				 * window hierarchy.  NULL means that
				 * the wrapper hasn't been created yet. */
    Tk_Window menubar;		/* Pointer to information about the
				 * menubar, or NULL if there is no
				 * menubar for this toplevel. */
    int menuHeight;		/* Amount of vertical space needed for
				 * menubar, measured in pixels.  If
				 * menubar is non-NULL, this is >= 1 (X
				 * servers don't like dimensions of 0). */

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
    int xInParent, yInParent;	/* Offset of wrapperPtr within reparent,
				 * measured in pixels from upper-left outer
				 * corner of reparent's border to upper-left
				 * outer corner of wrapperPtr's border.  If
				 * not reparented then these are zero. */
    int configWidth, configHeight;
				/* Dimensions passed to last request that we
				 * issued to change geometry of the wrapper.
				 * Used to eliminate redundant resize
				 * operations. */

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
    int vRootWidth, vRootHeight;/* Dimensions of the virtual root window.
				 * If vRoot is None, gives the dimensions
				 * of the containing screen.  This information
				 * is never stale, even though vRootX and
				 * vRootY can be. */

    /*
     * Miscellaneous information.
     */

    ProtocolHandler *protPtr;	/* First in list of protocol handlers for
				 * this window (NULL means none). */
    int cmdArgc;		/* Number of elements in cmdArgv below. */
    CONST char **cmdArgv;	/* Array of strings to store in the
				 * WM_COMMAND property.  NULL means nothing
				 * available. */
    char *clientMachine;	/* String to store in WM_CLIENT_MACHINE
				 * property, or NULL. */
    int flags;			/* Miscellaneous flags, defined below. */
    int numTransients;		/* number of transients on this window */
    struct TkWmInfo *nextPtr;	/* Next in list of all top-level windows. */
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
 * WM_WITHDRAWN -		non-zero means that this window has explicitly
 *				been withdrawn. If it's a transient, it should
 *				not mirror state changes in the master.
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
#define WM_WITHDRAWN		       	0x4000

/*
 * This module keeps a list of all top-level windows, primarily to
 * simplify the job of Tk_CoordsToWindow.  The list is called
 * firstWmPtr and is stored in the TkDisplay structure.
 */

/*
 * The following structures are the official type records for geometry
 * management of top-level and menubar windows.
 */

static void		TopLevelReqProc _ANSI_ARGS_((ClientData dummy,
			    Tk_Window tkwin));

static Tk_GeomMgr wmMgrType = {
    "wm",				/* name */
    TopLevelReqProc,			/* requestProc */
    (Tk_GeomLostSlaveProc *) NULL,	/* lostSlaveProc */
};

static void		MenubarReqProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin));

static Tk_GeomMgr menubarMgrType = {
    "menubar",				/* name */
    MenubarReqProc,			/* requestProc */
    (Tk_GeomLostSlaveProc *) NULL,	/* lostSlaveProc */
};

/*
 * Structures of the following type are used for communication between
 * WaitForEvent, WaitRestrictProc, and WaitTimeoutProc.
 */

typedef struct WaitRestrictInfo {
    Display *display;		/* Window belongs to this display. */
    WmInfo *wmInfoPtr;
    int type;			/* We only care about this type of event. */
    XEvent *eventPtr;		/* Where to store the event when it's found. */
    int foundEvent;		/* Non-zero means that an event of the
				 * desired type has been found. */
} WaitRestrictInfo;

/*
 * Forward declarations for procedures defined in this file:
 */

static int		ComputeReparentGeometry _ANSI_ARGS_((WmInfo *wmPtr));
static void		ConfigureEvent _ANSI_ARGS_((WmInfo *wmPtr,
			    XConfigureEvent *eventPtr));
static void		CreateWrapper _ANSI_ARGS_((WmInfo *wmPtr));
static void		GetMaxSize _ANSI_ARGS_((WmInfo *wmPtr,
			    int *maxWidthPtr, int *maxHeightPtr));
static void		MenubarDestroyProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		ParseGeometry _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, TkWindow *winPtr));
static void		ReparentEvent _ANSI_ARGS_((WmInfo *wmPtr,
			    XReparentEvent *eventPtr));
static void		TkWmStackorderToplevelWrapperMap _ANSI_ARGS_((
			    TkWindow *winPtr,
			    Tcl_HashTable *reparentTable));
static void		TopLevelReqProc _ANSI_ARGS_((ClientData dummy,
			    Tk_Window tkwin));
static void		UpdateCommand _ANSI_ARGS_((TkWindow *winPtr));
static void		UpdateGeometryInfo _ANSI_ARGS_((
			    ClientData clientData));
static void		UpdateHints _ANSI_ARGS_((TkWindow *winPtr));
static void		UpdateSizeHints _ANSI_ARGS_((TkWindow *winPtr));
static void		UpdateVRootGeometry _ANSI_ARGS_((WmInfo *wmPtr));
static void		UpdateWmProtocols _ANSI_ARGS_((WmInfo *wmPtr));
static void		WaitForConfigureNotify _ANSI_ARGS_((TkWindow *winPtr,
			    unsigned long serial));
static int		WaitForEvent _ANSI_ARGS_((Display *display,
			    WmInfo *wmInfoPtr, int type, XEvent *eventPtr));
static void		WaitForMapNotify _ANSI_ARGS_((TkWindow *winPtr,
			    int mapped));
static Tk_RestrictAction
			WaitRestrictProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		WrapperEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		WmWaitMapProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));

static int 		WmAspectCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmAttributesCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmClientCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmColormapwindowsCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmCommandCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmDeiconifyCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmFocusmodelCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmFrameCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmGeometryCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmGridCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmGroupCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmIconbitmapCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmIconifyCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmIconmaskCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmIconnameCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmIconpositionCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmIconwindowCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmMaxsizeCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmMinsizeCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmOverrideredirectCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmPositionfromCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmProtocolCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmResizableCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmSizefromCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmStackorderCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmStateCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmTitleCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmTransientCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int 		WmWithdrawCmd _ANSI_ARGS_((Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static void		WmUpdateGeom _ANSI_ARGS_((WmInfo *wmPtr,
			    TkWindow *winPtr));

/*
 *--------------------------------------------------------------
 *
 * TkWmCleanup --
 *
 *	This procedure is invoked to cleanup remaining wm resources
 *	associated with a display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All WmInfo structure resources are freed and invalidated.
 *
 *--------------------------------------------------------------
 */

void TkWmCleanup(dispPtr)
    TkDisplay *dispPtr;
{
    WmInfo *wmPtr, *nextPtr;
    for (wmPtr = dispPtr->firstWmPtr; wmPtr != NULL; wmPtr = nextPtr) {
	/*
	 * We can't assume we have access to winPtr's anymore, so some
	 * cleanup requiring winPtr data is avoided.
	 */
	nextPtr = wmPtr->nextPtr;
	if (wmPtr->title != NULL) {
	    ckfree(wmPtr->title);
	}
	if (wmPtr->iconName != NULL) {
	    ckfree(wmPtr->iconName);
	}
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	if (wmPtr->menubar != NULL) {
	    Tk_DestroyWindow(wmPtr->menubar);
	}
	if (wmPtr->wrapperPtr != NULL) {
	    Tk_DestroyWindow((Tk_Window) wmPtr->wrapperPtr);
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
	ckfree((char *) wmPtr);
    }
}

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
TkWmNewWindow(winPtr)
    TkWindow *winPtr;		/* Newly-created top-level window. */
{
    register WmInfo *wmPtr;
    TkDisplay *dispPtr = winPtr->dispPtr;

    wmPtr = (WmInfo *) ckalloc(sizeof(WmInfo));
    memset(wmPtr, 0, sizeof(WmInfo));
    wmPtr->winPtr = winPtr;
    wmPtr->reparent = None;
    wmPtr->masterPtr = NULL;
    wmPtr->numTransients = 0;
    wmPtr->hints.flags = InputHint | StateHint;
    wmPtr->hints.input = True;
    wmPtr->hints.initial_state = NormalState;
    wmPtr->hints.icon_pixmap = None;
    wmPtr->hints.icon_window = None;
    wmPtr->hints.icon_x = wmPtr->hints.icon_y = 0;
    wmPtr->hints.icon_mask = None;
    wmPtr->hints.window_group = None;
    wmPtr->minWidth = wmPtr->minHeight = 1;

    /*
     * Default the maximum dimensions to the size of the display, minus
     * a guess about how space is needed for window manager decorations.
     */

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
    wmPtr->configWidth = -1;
    wmPtr->configHeight = -1;
    wmPtr->vRoot = None;
    wmPtr->flags = WM_NEVER_MAPPED;
    wmPtr->nextPtr = (WmInfo *) dispPtr->firstWmPtr;
    dispPtr->firstWmPtr = wmPtr;
    winPtr->wmInfoPtr = wmPtr;

    UpdateVRootGeometry(wmPtr);

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
TkWmMapWindow(winPtr)
    TkWindow *winPtr;		/* Top-level window that's about to
				 * be mapped. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    XTextProperty textProp;
    Tk_Uid string;

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	Tcl_DString ds;

	wmPtr->flags &= ~WM_NEVER_MAPPED;

	/*
	 * This is the first time this window has ever been mapped.
	 * First create the wrapper window that provides space for a
	 * menubar.
	 */

	if (wmPtr->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr);
	}

	/*
	 * Store all the window-manager-related information for the
	 * window.
	 */

	string = (wmPtr->title != NULL) ? wmPtr->title : winPtr->nameUid;
	Tcl_UtfToExternalDString(NULL, string, -1, &ds);
	string = Tcl_DStringValue(&ds);
	if (XStringListToTextProperty(&(Tcl_DStringValue(&ds)), 1,
		&textProp)  != 0) {
	    XSetWMName(winPtr->display, wmPtr->wrapperPtr->window, &textProp);
	    XFree((char *) textProp.value);
	}
	Tcl_DStringFree(&ds);

	TkWmSetClass(winPtr);

	if (wmPtr->iconName != NULL) {
	    Tcl_UtfToExternalDString(NULL, wmPtr->iconName, -1, &ds);
	    XSetIconName(winPtr->display, wmPtr->wrapperPtr->window,
		    Tcl_DStringValue(&ds));
	    Tcl_DStringFree(&ds);
	}

	if (wmPtr->masterPtr != NULL) {
	    /*
	     * Don't map a transient if the master is not mapped.
	     */

	    if (!Tk_IsMapped(wmPtr->masterPtr)) {
	        wmPtr->withdrawn = 1;
	        wmPtr->hints.initial_state = WithdrawnState;
	    } else {
	        XSetTransientForHint(winPtr->display, wmPtr->wrapperPtr->window,
		        wmPtr->masterPtr->wmInfoPtr->wrapperPtr->window);
	    }
	}

	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	UpdateHints(winPtr);
	UpdateWmProtocols(wmPtr);
	if (wmPtr->cmdArgv != NULL) {
	    UpdateCommand(winPtr);
	}
	if (wmPtr->clientMachine != NULL) {
	    Tcl_UtfToExternalDString(NULL, wmPtr->clientMachine, -1, &ds);
	    if (XStringListToTextProperty(&(Tcl_DStringValue(&ds)), 1,
		    &textProp) != 0) {
		XSetWMClientMachine(winPtr->display, wmPtr->wrapperPtr->window,
			&textProp);
		XFree((char *) textProp.value);
	    }
	    Tcl_DStringFree(&ds);
	}
    }
    if (wmPtr->hints.initial_state == WithdrawnState) {
	return;
    }
    if (wmPtr->iconFor != NULL) {
	/*
	 * This window is an icon for somebody else.  Make sure that
	 * the geometry is up-to-date, then return without mapping
	 * the window.
	 */

	if (wmPtr->flags & WM_UPDATE_PENDING) {
	    Tcl_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
	}
	UpdateGeometryInfo((ClientData) winPtr);
	return;
    }
    wmPtr->flags |= WM_ABOUT_TO_MAP;
    if (wmPtr->flags & WM_UPDATE_PENDING) {
	Tcl_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
    }
    UpdateGeometryInfo((ClientData) winPtr);
    wmPtr->flags &= ~WM_ABOUT_TO_MAP;

    /*
     * Map the window, then wait to be sure that the window manager has
     * processed the map operation.
     */

    XMapWindow(winPtr->display, wmPtr->wrapperPtr->window);
    if (wmPtr->hints.initial_state == NormalState) {
	WaitForMapNotify(winPtr, 1);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkWmUnmapWindow --
 *
 *	This procedure is invoked to unmap a top-level window.  The
 *	only thing it does special is to wait for the window actually
 *	to be unmapped.
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
TkWmUnmapWindow(winPtr)
    TkWindow *winPtr;		/* Top-level window that's about to
				 * be mapped. */
{
    /*
     * It seems to be important to wait after unmapping a top-level
     * window until the window really gets unmapped.  I don't completely
     * understand all the interactions with the window manager, but if
     * we go on without waiting, and if the window is then mapped again
     * quickly, events seem to get lost so that we think the window isn't
     * mapped when in fact it is mapped.  I suspect that this has something
     * to do with the window manager filtering Map events (and possily not
     * filtering Unmap events?).
     */
    XUnmapWindow(winPtr->display, winPtr->wmInfoPtr->wrapperPtr->window);
    WaitForMapNotify(winPtr, 0);
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
    if ((WmInfo *) winPtr->dispPtr->firstWmPtr == wmPtr) {
	winPtr->dispPtr->firstWmPtr = wmPtr->nextPtr;
    } else {
	register WmInfo *prevPtr;

	for (prevPtr = (WmInfo *) winPtr->dispPtr->firstWmPtr; ;
                prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		panic("couldn't unlink window in TkWmDeadWindow");
	    }
	    if (prevPtr->nextPtr == wmPtr) {
		prevPtr->nextPtr = wmPtr->nextPtr;
		break;
	    }
	}
    }
    if (wmPtr->title != NULL) {
	ckfree(wmPtr->title);
    }
    if (wmPtr->iconName != NULL) {
	ckfree(wmPtr->iconName);
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
    if (wmPtr->icon != NULL) {
	wmPtr2 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
	wmPtr2->iconFor = NULL;
	wmPtr2->withdrawn = 1;
    }
    if (wmPtr->iconFor != NULL) {
	wmPtr2 = ((TkWindow *) wmPtr->iconFor)->wmInfoPtr;
	wmPtr2->icon = NULL;
	wmPtr2->hints.flags &= ~IconWindowHint;
	UpdateHints((TkWindow *) wmPtr->iconFor);
    }
    if (wmPtr->menubar != NULL) {
	Tk_DestroyWindow(wmPtr->menubar);
    }
    if (wmPtr->wrapperPtr != NULL) {
	/*
	 * The rest of Tk doesn't know that we reparent the toplevel
	 * inside the wrapper, so reparent it back out again before
	 * deleting the wrapper; otherwise the toplevel will get deleted
	 * twice (once implicitly by the deletion of the wrapper).
	 */

	XUnmapWindow(winPtr->display, winPtr->window);
	XReparentWindow(winPtr->display, winPtr->window,
		XRootWindow(winPtr->display, winPtr->screenNum), 0, 0);
	Tk_DestroyWindow((Tk_Window) wmPtr->wrapperPtr);
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
	Tcl_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
    }
    /*
     * Reset all transient windows whose master is the dead window.
     */

    for (wmPtr2 = winPtr->dispPtr->firstWmPtr; wmPtr2 != NULL;
	 wmPtr2 = wmPtr2->nextPtr) {
	if (wmPtr2->masterPtr == winPtr) {
	    wmPtr->numTransients--;
	    Tk_DeleteEventHandler((Tk_Window) wmPtr2->masterPtr,
	            StructureNotifyMask,
	            WmWaitMapProc, (ClientData) wmPtr2->winPtr);
	    wmPtr2->masterPtr = NULL;
	    if (!(wmPtr2->flags & WM_NEVER_MAPPED)) {
		XSetTransientForHint(wmPtr2->winPtr->display,
		        wmPtr2->wrapperPtr->window, None);
		/* FIXME: Need a call like Win32's UpdateWrapper() so
		   we can recreate the wrapper and get rid of the
		   transient window decorations. */
	    }
	}
    }
    if (wmPtr->numTransients != 0)
        panic("numTransients should be 0");

    if (wmPtr->masterPtr != NULL) {
	wmPtr2 = wmPtr->masterPtr->wmInfoPtr;
	/*
	 * If we had a master, tell them that we aren't tied
	 * to them anymore
	 */
	if (wmPtr2 != NULL) {
	    wmPtr2->numTransients--;
	}
	Tk_DeleteEventHandler((Tk_Window) wmPtr->masterPtr,
		StructureNotifyMask,
		WmWaitMapProc, (ClientData) winPtr);
	wmPtr->masterPtr = NULL;
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
TkWmSetClass(winPtr)
    TkWindow *winPtr;		/* Newly-created top-level window. */
{
    if (winPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	return;
    }

    if (winPtr->classUid != NULL) {
	XClassHint *classPtr;
	Tcl_DString name, class;

	Tcl_UtfToExternalDString(NULL, winPtr->nameUid, -1, &name);
	Tcl_UtfToExternalDString(NULL, winPtr->classUid, -1, &class);
	classPtr = XAllocClassHint();
	classPtr->res_name = Tcl_DStringValue(&name);
	classPtr->res_class = Tcl_DStringValue(&class);
	XSetClassHint(winPtr->display, winPtr->wmInfoPtr->wrapperPtr->window,
		classPtr);
	XFree((char *) classPtr);
	Tcl_DStringFree(&name);
	Tcl_DStringFree(&class);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_WmObjCmd --
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
Tk_WmObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    static CONST char *optionStrings[] = {
	"aspect", "attributes", "client", "colormapwindows",
	"command", "deiconify", "focusmodel", "frame",
	"geometry", "grid", "group", "iconbitmap",
	"iconify", "iconmask", "iconname", "iconposition",
	"iconwindow", "maxsize", "minsize", "overrideredirect",
        "positionfrom", "protocol", "resizable", "sizefrom",
        "stackorder", "state", "title", "transient",
	"withdraw", (char *) NULL };
    enum options {
        WMOPT_ASPECT, WMOPT_ATTRIBUTES, WMOPT_CLIENT, WMOPT_COLORMAPWINDOWS,
	WMOPT_COMMAND, WMOPT_DEICONIFY, WMOPT_FOCUSMODEL, WMOPT_FRAME,
	WMOPT_GEOMETRY, WMOPT_GRID, WMOPT_GROUP, WMOPT_ICONBITMAP,
	WMOPT_ICONIFY, WMOPT_ICONMASK, WMOPT_ICONNAME, WMOPT_ICONPOSITION,
	WMOPT_ICONWINDOW, WMOPT_MAXSIZE, WMOPT_MINSIZE, WMOPT_OVERRIDEREDIRECT,
        WMOPT_POSITIONFROM, WMOPT_PROTOCOL, WMOPT_RESIZABLE, WMOPT_SIZEFROM,
        WMOPT_STACKORDER, WMOPT_STATE, WMOPT_TITLE, WMOPT_TRANSIENT,
	WMOPT_WITHDRAW };
    int index; 
    int length;
    char *argv1;
    TkWindow *winPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objc < 2) {
	wrongNumArgs:
	Tcl_WrongNumArgs(interp, 1, objv, "option window ?arg ...?");
	return TCL_ERROR;
    }

    argv1 = Tcl_GetStringFromObj(objv[1], &length);
    if ((argv1[0] == 't') && (strncmp(argv1, "tracing", (size_t) length) == 0)
	    && (length >= 3)) {
	int wmTracing;
	if ((objc != 2) && (objc != 3)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?boolean?");
	    return TCL_ERROR;
	}
	if (objc == 2) {
	    Tcl_SetResult(interp,
		    ((dispPtr->flags & TK_DISPLAY_WM_TRACING) ? "on" : "off"),
		    TCL_STATIC);
	    return TCL_OK;
	}
	if (Tcl_GetBooleanFromObj(interp, objv[2], &wmTracing) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (wmTracing) {
	    dispPtr->flags |= TK_DISPLAY_WM_TRACING;
	} else {
	    dispPtr->flags &= ~TK_DISPLAY_WM_TRACING;
	}
	return TCL_OK;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc < 3) {
	goto wrongNumArgs;
    }

    if (TkGetWindowFromObj(interp, tkwin, objv[2], (Tk_Window *) &winPtr)
	    != TCL_OK) {
	return TCL_ERROR;
    }
    if (!Tk_IsTopLevel(winPtr)) {
	Tcl_AppendResult(interp, "window \"", winPtr->pathName,
		"\" isn't a top-level window", (char *) NULL);
	return TCL_ERROR;
    }

    switch ((enum options) index) {
      case WMOPT_ASPECT:
	return WmAspectCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ATTRIBUTES:
	return WmAttributesCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_CLIENT:
	return WmClientCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_COLORMAPWINDOWS:
	return WmColormapwindowsCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_COMMAND:
	return WmCommandCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_DEICONIFY:
	return WmDeiconifyCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_FOCUSMODEL:
	return WmFocusmodelCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_FRAME:
	return WmFrameCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_GEOMETRY:
	return WmGeometryCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_GRID:
	return WmGridCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_GROUP:
	return WmGroupCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ICONBITMAP:
	return WmIconbitmapCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ICONIFY:
	return WmIconifyCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ICONMASK:
	return WmIconmaskCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ICONNAME:
	return WmIconnameCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ICONPOSITION:
	return WmIconpositionCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_ICONWINDOW:
	return WmIconwindowCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_MAXSIZE:
	return WmMaxsizeCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_MINSIZE:
	return WmMinsizeCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_OVERRIDEREDIRECT:
	return WmOverrideredirectCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_POSITIONFROM:
	return WmPositionfromCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_PROTOCOL:
	return WmProtocolCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_RESIZABLE:
	return WmResizableCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_SIZEFROM:
	return WmSizefromCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_STACKORDER:
	return WmStackorderCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_STATE:
	return WmStateCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_TITLE:
	return WmTitleCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_TRANSIENT:
	return WmTransientCmd(tkwin, winPtr, interp, objc, objv);
      case WMOPT_WITHDRAW:
	return WmWithdrawCmd(tkwin, winPtr, interp, objc, objv);
    }

    /* This should not happen */
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * WmAspectCmd --
 *
 *	This procedure is invoked to process the "wm aspect" Tcl command.
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

static int
WmAspectCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int numer1, denom1, numer2, denom2;

    if ((objc != 3) && (objc != 7)) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"window ?minNumer minDenom maxNumer maxDenom?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->sizeHintsFlags & PAspect) {
	    char buf[TCL_INTEGER_SPACE * 4];

	    sprintf(buf, "%d %d %d %d", wmPtr->minAspect.x,
		    wmPtr->minAspect.y, wmPtr->maxAspect.x,
		    wmPtr->maxAspect.y);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->sizeHintsFlags &= ~PAspect;
    } else {
	if ((Tcl_GetIntFromObj(interp, objv[3], &numer1) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &denom1) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[5], &numer2) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[6], &denom2) != TCL_OK)) {
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
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmAttributesCmd --
 *
 *	This procedure is invoked to process the "wm attributes" Tcl command.
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

static int
WmAttributesCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmClientCmd --
 *
 *	This procedure is invoked to process the "wm client" Tcl command.
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

static int
WmClientCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    char *argv3;
    int length;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?name?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->clientMachine != NULL) {
	    Tcl_SetResult(interp, wmPtr->clientMachine, TCL_STATIC);
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetStringFromObj(objv[3], &length);
    if (argv3[0] == 0) {
	if (wmPtr->clientMachine != NULL) {
	    ckfree((char *) wmPtr->clientMachine);
	    wmPtr->clientMachine = NULL;
	    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		XDeleteProperty(winPtr->display, wmPtr->wrapperPtr->window,
			Tk_InternAtom((Tk_Window) winPtr,
				"WM_CLIENT_MACHINE"));
	    }
	}
	return TCL_OK;
    }
    if (wmPtr->clientMachine != NULL) {
	ckfree((char *) wmPtr->clientMachine);
    }
    wmPtr->clientMachine = (char *)
	    ckalloc((unsigned) (length + 1));
    strcpy(wmPtr->clientMachine, argv3);
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	XTextProperty textProp;
	Tcl_DString ds;

	Tcl_UtfToExternalDString(NULL, wmPtr->clientMachine, -1, &ds);
	if (XStringListToTextProperty(&(Tcl_DStringValue(&ds)), 1,
		&textProp) != 0) {
	    XSetWMClientMachine(winPtr->display, wmPtr->wrapperPtr->window,
		    &textProp);
	    XFree((char *) textProp.value);
	}
	Tcl_DStringFree(&ds);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmColormapwindowsCmd --
 *
 *	This procedure is invoked to process the "wm colormapwindows"
 *	Tcl command.
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

static int
WmColormapwindowsCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Window *cmapList;
    TkWindow *winPtr2;
    int count, i, windowObjc, gotToplevel;
    Tcl_Obj **windowObjv;
    char buffer[20];

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?windowList?");
	return TCL_ERROR;
    }
    Tk_MakeWindowExist((Tk_Window) winPtr);
    if (wmPtr->wrapperPtr == NULL) {
	CreateWrapper(wmPtr);
    }
    if (objc == 3) {
	if (XGetWMColormapWindows(winPtr->display,
		wmPtr->wrapperPtr->window, &cmapList, &count) == 0) {
	    return TCL_OK;
	}
	for (i = 0; i < count; i++) {
	    if ((i == (count-1))
		    && (wmPtr->flags & WM_ADDED_TOPLEVEL_COLORMAP)) {
		break;
	    }
	    winPtr2  = (TkWindow *) Tk_IdToWindow(winPtr->display,
		    cmapList[i]);
	    if (winPtr2 == NULL) {
		sprintf(buffer, "0x%lx", cmapList[i]);
		Tcl_AppendElement(interp, buffer);
	    } else {
		Tcl_AppendElement(interp, winPtr2->pathName);
	    }
	}
	XFree((char *) cmapList);
	return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objv[3], &windowObjc, &windowObjv)
	    != TCL_OK) {
	return TCL_ERROR;
    }
    cmapList = (Window *) ckalloc((unsigned)
	    ((windowObjc+1)*sizeof(Window)));
    gotToplevel = 0;
    for (i = 0; i < windowObjc; i++) {
	if (TkGetWindowFromObj(interp, tkwin, windowObjv[i],
		(Tk_Window *) &winPtr2) != TCL_OK)
	{
	    ckfree((char *) cmapList);
	    return TCL_ERROR;
	}
	if (winPtr2 == winPtr) {
	    gotToplevel = 1;
	}
	if (winPtr2->window == None) {
	    Tk_MakeWindowExist((Tk_Window) winPtr2);
	}
	cmapList[i] = winPtr2->window;
    }
    if (!gotToplevel) {
	wmPtr->flags |= WM_ADDED_TOPLEVEL_COLORMAP;
	cmapList[windowObjc] = wmPtr->wrapperPtr->window;
	windowObjc++;
    } else {
	wmPtr->flags &= ~WM_ADDED_TOPLEVEL_COLORMAP;
    }
    wmPtr->flags |= WM_COLORMAPS_EXPLICIT;
    XSetWMColormapWindows(winPtr->display, wmPtr->wrapperPtr->window,
	    cmapList, windowObjc);
    ckfree((char *) cmapList);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmCommandCmd --
 *
 *	This procedure is invoked to process the "wm command" Tcl command.
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

static int
WmCommandCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    char *argv3;
    int cmdArgc;
    CONST char **cmdArgv;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?value?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->cmdArgv != NULL) {
	    Tcl_SetResult(interp,
		    Tcl_Merge(wmPtr->cmdArgc, wmPtr->cmdArgv),
		    TCL_DYNAMIC);
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (argv3[0] == 0) {
	if (wmPtr->cmdArgv != NULL) {
	    ckfree((char *) wmPtr->cmdArgv);
	    wmPtr->cmdArgv = NULL;
	    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		XDeleteProperty(winPtr->display, wmPtr->wrapperPtr->window,
			Tk_InternAtom((Tk_Window) winPtr, "WM_COMMAND"));
	    }
	}
	return TCL_OK;
    }
    if (Tcl_SplitList(interp, argv3, &cmdArgc, &cmdArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (wmPtr->cmdArgv != NULL) {
	ckfree((char *) wmPtr->cmdArgv);
    }
    wmPtr->cmdArgc = cmdArgc;
    wmPtr->cmdArgv = cmdArgv;
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	UpdateCommand(winPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmDeiconifyCmd --
 *
 *	This procedure is invoked to process the "wm deiconify" Tcl command.
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

static int
WmDeiconifyCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    if (wmPtr->iconFor != NULL) {
	Tcl_AppendResult(interp, "can't deiconify ", Tcl_GetString(objv[2]),
		": it is an icon for ", Tk_PathName(wmPtr->iconFor),
		(char *) NULL);
	return TCL_ERROR;
    }
    if (winPtr->flags & TK_EMBEDDED) {
	Tcl_AppendResult(interp, "can't deiconify ", winPtr->pathName,
		": it is an embedded window", (char *) NULL);
	return TCL_ERROR;
    }
    wmPtr->flags &= ~WM_WITHDRAWN;
    TkpWmSetState(winPtr, NormalState);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmFocusmodelCmd --
 *
 *	This procedure is invoked to process the "wm focusmodel" Tcl command.
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

static int
WmFocusmodelCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static CONST char *optionStrings[] = {
	"active", "passive", (char *) NULL };
    enum options {
	OPT_ACTIVE, OPT_PASSIVE };
    int index;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?active|passive?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_SetResult(interp, (wmPtr->hints.input ? "passive" : "active"),
		TCL_STATIC);
	return TCL_OK;
    }

    if (Tcl_GetIndexFromObj(interp, objv[3], optionStrings, "argument", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index == OPT_ACTIVE) {
	wmPtr->hints.input = False;
    } else { /* OPT_PASSIVE */
	wmPtr->hints.input = True;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmFrameCmd --
 *
 *	This procedure is invoked to process the "wm frame" Tcl command.
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

static int
WmFrameCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Window window;
    char buf[TCL_INTEGER_SPACE];

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    window = wmPtr->reparent;
    if (window == None) {
	window = Tk_WindowId((Tk_Window) winPtr);
    }
    sprintf(buf, "0x%x", (unsigned int) window);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmGeometryCmd --
 *
 *	This procedure is invoked to process the "wm geometry" Tcl command.
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

static int
WmGeometryCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    char xSign, ySign;
    int width, height;
    char *argv3;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?newGeometry?");
	return TCL_ERROR;
    }
    if (objc == 3) {
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
    argv3 = Tcl_GetString(objv[3]);
    if (*argv3 == '\0') {
	wmPtr->width = -1;
	wmPtr->height = -1;
	WmUpdateGeom(wmPtr, winPtr);
	return TCL_OK;
    }
    return ParseGeometry(interp, argv3, winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WmGridCmd --
 *
 *	This procedure is invoked to process the "wm grid" Tcl command.
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

static int
WmGridCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int reqWidth, reqHeight, widthInc, heightInc;

    if ((objc != 3) && (objc != 7)) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"window ?baseWidth baseHeight widthInc heightInc?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->sizeHintsFlags & PBaseSize) {
	    char buf[TCL_INTEGER_SPACE * 4];

	    sprintf(buf, "%d %d %d %d", wmPtr->reqGridWidth,
		    wmPtr->reqGridHeight, wmPtr->widthInc,
		    wmPtr->heightInc);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
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
	if ((Tcl_GetIntFromObj(interp, objv[3], &reqWidth) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &reqHeight) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[5], &widthInc) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[6], &heightInc) != TCL_OK)) {
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
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmGroupCmd --
 *
 *	This procedure is invoked to process the "wm group" Tcl command.
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

static int
WmGroupCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tk_Window tkwin2;
    WmInfo *wmPtr2;
    char *argv3;
    int length;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?pathName?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & WindowGroupHint) {
	    Tcl_SetResult(interp, wmPtr->leaderName, TCL_STATIC);
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetStringFromObj(objv[3], &length);
    if (*argv3 == '\0') {
	wmPtr->hints.flags &= ~WindowGroupHint;
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	wmPtr->leaderName = NULL;
    } else {
	if (TkGetWindowFromObj(interp, tkwin, objv[3], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	while (!Tk_TopWinHierarchy(tkwin2)) {
	    /*
	     * Ensure that the group leader is actually a Tk toplevel.
	     */

	    tkwin2 = Tk_Parent(tkwin2);
	}
	Tk_MakeWindowExist(tkwin2);
	wmPtr2 = ((TkWindow *) tkwin2)->wmInfoPtr;
	if (wmPtr2->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr2);
	}
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	wmPtr->hints.window_group = Tk_WindowId(wmPtr2->wrapperPtr);
	wmPtr->hints.flags |= WindowGroupHint;
	wmPtr->leaderName = ckalloc((unsigned) (length + 1));
	strcpy(wmPtr->leaderName, argv3);
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconbitmapCmd --
 *
 *	This procedure is invoked to process the "wm iconbitmap" Tcl command.
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

static int
WmIconbitmapCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Pixmap pixmap;
    char *argv3;

    if ((objc < 3) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?bitmap?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & IconPixmapHint) {
	    Tcl_SetResult(interp, (char *)
		    Tk_NameOfBitmap(winPtr->display, wmPtr->hints.icon_pixmap),
		    TCL_STATIC);
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (*argv3 == '\0') {
	if (wmPtr->hints.icon_pixmap != None) {
	    Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_pixmap);
	    wmPtr->hints.icon_pixmap = None;
	}
	wmPtr->hints.flags &= ~IconPixmapHint;
    } else {
	pixmap = Tk_GetBitmap(interp, (Tk_Window) winPtr, argv3);
	if (pixmap == None) {
	    return TCL_ERROR;
	}
	wmPtr->hints.icon_pixmap = pixmap;
	wmPtr->hints.flags |= IconPixmapHint;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconifyCmd --
 *
 *	This procedure is invoked to process the "wm iconify" Tcl command.
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

static int
WmIconifyCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    if (Tk_Attributes((Tk_Window) winPtr)->override_redirect) {
	Tcl_AppendResult(interp, "can't iconify \"", winPtr->pathName,
		"\": override-redirect flag is set", (char *) NULL);
	return TCL_ERROR;
    }
    if (wmPtr->masterPtr != NULL) {
	Tcl_AppendResult(interp, "can't iconify \"", winPtr->pathName,
		"\": it is a transient", (char *) NULL);
	return TCL_ERROR;
    }
    if (wmPtr->iconFor != NULL) {
	Tcl_AppendResult(interp, "can't iconify ", winPtr->pathName,
		": it is an icon for ", Tk_PathName(wmPtr->iconFor),
		(char *) NULL);
	return TCL_ERROR;
    }
    if (winPtr->flags & TK_EMBEDDED) {
	Tcl_AppendResult(interp, "can't iconify ", winPtr->pathName,
		": it is an embedded window", (char *) NULL);
	return TCL_ERROR;
    }
    if (TkpWmSetState(winPtr, IconicState) == 0) {
	Tcl_SetResult(interp,
		"couldn't send iconify message to window manager",
		TCL_STATIC);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconmaskCmd --
 *
 *	This procedure is invoked to process the "wm iconmask" Tcl command.
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

static int
WmIconmaskCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Pixmap pixmap;
    char *argv3;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?bitmap?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & IconMaskHint) {
	    Tcl_SetResult(interp, (char *)
		    Tk_NameOfBitmap(winPtr->display, wmPtr->hints.icon_mask),
		    TCL_STATIC);
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (*argv3 == '\0') {
	if (wmPtr->hints.icon_mask != None) {
	    Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_mask);
	}
	wmPtr->hints.flags &= ~IconMaskHint;
    } else {
	pixmap = Tk_GetBitmap(interp, tkwin, argv3);
	if (pixmap == None) {
	    return TCL_ERROR;
	}
	wmPtr->hints.icon_mask = pixmap;
	wmPtr->hints.flags |= IconMaskHint;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconnameCmd --
 *
 *	This procedure is invoked to process the "wm iconname" Tcl command.
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

static int
WmIconnameCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    char *argv3;
    int length;

    if (objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?newName?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_SetResult(interp,
		((wmPtr->iconName != NULL) ? wmPtr->iconName : ""),
		TCL_STATIC);
	return TCL_OK;
    } else {
	if (wmPtr->iconName != NULL) {
	    ckfree((char *) wmPtr->iconName);
	}
	argv3 = Tcl_GetStringFromObj(objv[3], &length);
	wmPtr->iconName = ckalloc((unsigned) (length + 1));
	strcpy(wmPtr->iconName, argv3);
	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    Tcl_DString ds;

	    Tcl_UtfToExternalDString(NULL, wmPtr->iconName, -1, &ds);
	    XSetIconName(winPtr->display, wmPtr->wrapperPtr->window,
		    Tcl_DStringValue(&ds));
	    Tcl_DStringFree(&ds);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconpositionCmd --
 *
 *	This procedure is invoked to process the "wm iconposition"
 *	Tcl command.
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

static int
WmIconpositionCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int x, y;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?x y?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & IconPositionHint) {
	    char buf[TCL_INTEGER_SPACE * 2];

	    sprintf(buf, "%d %d", wmPtr->hints.icon_x,
		    wmPtr->hints.icon_y);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->hints.flags &= ~IconPositionHint;
    } else {
	if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)){
	    return TCL_ERROR;
	}
	wmPtr->hints.icon_x = x;
	wmPtr->hints.icon_y = y;
	wmPtr->hints.flags |= IconPositionHint;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconwindowCmd --
 *
 *	This procedure is invoked to process the "wm iconwindow" Tcl command.
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

static int
WmIconwindowCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tk_Window tkwin2;
    WmInfo *wmPtr2;
    XSetWindowAttributes atts;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?pathName?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->icon != NULL) {
	    Tcl_SetResult(interp, Tk_PathName(wmPtr->icon), TCL_STATIC);
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->hints.flags &= ~IconWindowHint;
	if (wmPtr->icon != NULL) {
	    /*
	     * Remove the icon window relationship.  In principle we
	     * should also re-enable button events for the window, but
	     * this doesn't work in general because the window manager
	     * is probably selecting on them (we'll get an error if
	     * we try to re-enable the events).  So, just leave the
	     * icon window event-challenged;  the user will have to
	     * recreate it if they want button events.
	     */

	    wmPtr2 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
	    wmPtr2->iconFor = NULL;
	    wmPtr2->withdrawn = 1;
	    wmPtr2->hints.initial_state = WithdrawnState;
	}
	wmPtr->icon = NULL;
    } else {
	if (TkGetWindowFromObj(interp, tkwin, objv[3], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (!Tk_IsTopLevel(tkwin2)) {
	    Tcl_AppendResult(interp, "can't use ", Tcl_GetString(objv[3]),
		    " as icon window: not at top level", (char *) NULL);
	    return TCL_ERROR;
	}
	wmPtr2 = ((TkWindow *) tkwin2)->wmInfoPtr;
	if (wmPtr2->iconFor != NULL) {
	    Tcl_AppendResult(interp, Tcl_GetString(objv[3]),
		    " is already an icon for ",
		    Tk_PathName(wmPtr2->iconFor), (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->icon != NULL) {
	    WmInfo *wmPtr3 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
	    wmPtr3->iconFor = NULL;
	    wmPtr3->withdrawn = 1;
	    wmPtr3->hints.initial_state = WithdrawnState;
	}

	/*
	 * Disable button events in the icon window:  some window
	 * managers (like olvwm) want to get the events themselves,
	 * but X only allows one application at a time to receive
	 * button events for a window.
	 */

	atts.event_mask = Tk_Attributes(tkwin2)->event_mask
		& ~ButtonPressMask;
	Tk_ChangeWindowAttributes(tkwin2, CWEventMask, &atts);
	Tk_MakeWindowExist(tkwin2);
	if (wmPtr2->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr2);
	}
	wmPtr->hints.icon_window = Tk_WindowId(wmPtr2->wrapperPtr);
	wmPtr->hints.flags |= IconWindowHint;
	wmPtr->icon = tkwin2;
	wmPtr2->iconFor = (Tk_Window) winPtr;
	if (!wmPtr2->withdrawn && !(wmPtr2->flags & WM_NEVER_MAPPED)) {
	    wmPtr2->withdrawn = 0;
	    if (XWithdrawWindow(Tk_Display(tkwin2),
		    Tk_WindowId(wmPtr2->wrapperPtr),
		    Tk_ScreenNumber(tkwin2)) == 0) {
		Tcl_SetResult(interp,
			"couldn't send withdraw message to window manager",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	    WaitForMapNotify((TkWindow *) tkwin2, 0);
	}
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmMaxsizeCmd --
 *
 *	This procedure is invoked to process the "wm maxsize" Tcl command.
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

static int
WmMaxsizeCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int width, height;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?width height?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	char buf[TCL_INTEGER_SPACE * 2];

	GetMaxSize(wmPtr, &width, &height);
	sprintf(buf, "%d %d", width, height);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[4], &height) != TCL_OK)) {
	return TCL_ERROR;
    }
    wmPtr->maxWidth = width;
    wmPtr->maxHeight = height;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmMinsizeCmd --
 *
 *	This procedure is invoked to process the "wm minsize" Tcl command.
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

static int
WmMinsizeCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int width, height;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?width height?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	char buf[TCL_INTEGER_SPACE * 2];

	sprintf(buf, "%d %d", wmPtr->minWidth, wmPtr->minHeight);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[4], &height) != TCL_OK)) {
	return TCL_ERROR;
    }
    wmPtr->minWidth = width;
    wmPtr->minHeight = height;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmOverrideredirectCmd --
 *
 *	This procedure is invoked to process the "wm overrideredirect"
 *	Tcl command.
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

static int
WmOverrideredirectCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int boolean, curValue;
    XSetWindowAttributes atts;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?boolean?");
	return TCL_ERROR;
    }
    curValue = Tk_Attributes((Tk_Window) winPtr)->override_redirect;
    if (objc == 3) {
	Tcl_SetBooleanObj(Tcl_GetObjResult(interp), curValue);
	return TCL_OK;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[3], &boolean) != TCL_OK) {
	return TCL_ERROR;
    }
    if (curValue != boolean) {
	/*
	 * Only do this if we are really changing value, because it
	 * causes some funky stuff to occur
	 */
	atts.override_redirect = (boolean) ? True : False;
	Tk_ChangeWindowAttributes((Tk_Window) winPtr, CWOverrideRedirect,
		&atts);
	if (winPtr->wmInfoPtr->wrapperPtr != NULL) {
	    Tk_ChangeWindowAttributes(
		    (Tk_Window) winPtr->wmInfoPtr->wrapperPtr,
		    CWOverrideRedirect, &atts);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmPositionfromCmd --
 *
 *	This procedure is invoked to process the "wm positionfrom"
 *	Tcl command.
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

static int
WmPositionfromCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static CONST char *optionStrings[] = {
	"program", "user", (char *) NULL };
    enum options {
	OPT_PROGRAM, OPT_USER };
    int index;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?user/program?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->sizeHintsFlags & USPosition) {
	    Tcl_SetResult(interp, "user", TCL_STATIC);
	} else if (wmPtr->sizeHintsFlags & PPosition) {
	    Tcl_SetResult(interp, "program", TCL_STATIC);
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->sizeHintsFlags &= ~(USPosition|PPosition);
    } else {
	if (Tcl_GetIndexFromObj(interp, objv[3], optionStrings, "argument", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_USER) {
	    wmPtr->sizeHintsFlags &= ~PPosition;
	    wmPtr->sizeHintsFlags |= USPosition;
	} else {
	    wmPtr->sizeHintsFlags &= ~USPosition;
	    wmPtr->sizeHintsFlags |= PPosition;
	}
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmProtocolCmd --
 *
 *	This procedure is invoked to process the "wm protocol" Tcl command.
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

static int
WmProtocolCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    register ProtocolHandler *protPtr, *prevPtr;
    Atom protocol;
    char *cmd;
    int cmdLength;

    if ((objc < 3) || (objc > 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?name? ?command?");
	return TCL_ERROR;
    }
    if (objc == 3) {
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
    protocol = Tk_InternAtom((Tk_Window) winPtr, Tcl_GetString(objv[3]));
    if (objc == 4) {
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
    cmd = Tcl_GetStringFromObj(objv[4], &cmdLength);
    if (cmdLength > 0) {
	protPtr = (ProtocolHandler *) ckalloc(HANDLER_SIZE(cmdLength));
	protPtr->protocol = protocol;
	protPtr->nextPtr = wmPtr->protPtr;
	wmPtr->protPtr = protPtr;
	protPtr->interp = interp;
	strcpy(protPtr->command, cmd);
    }
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	UpdateWmProtocols(wmPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmResizableCmd --
 *
 *	This procedure is invoked to process the "wm resizable" Tcl command.
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

static int
WmResizableCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int width, height;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?width height?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	char buf[TCL_INTEGER_SPACE * 2];

	sprintf(buf, "%d %d",
		(wmPtr->flags  & WM_WIDTH_NOT_RESIZABLE) ? 0 : 1,
		(wmPtr->flags  & WM_HEIGHT_NOT_RESIZABLE) ? 0 : 1);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_OK;
    }
    if ((Tcl_GetBooleanFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetBooleanFromObj(interp, objv[4], &height) != TCL_OK)) {
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
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmSizefromCmd --
 *
 *	This procedure is invoked to process the "wm sizefrom" Tcl command.
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

static int
WmSizefromCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static CONST char *optionStrings[] = {
	"program", "user", (char *) NULL };
    enum options {
	OPT_PROGRAM, OPT_USER };
    int index;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?user|program?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->sizeHintsFlags & USSize) {
	    Tcl_SetResult(interp, "user", TCL_STATIC);
	} else if (wmPtr->sizeHintsFlags & PSize) {
	    Tcl_SetResult(interp, "program", TCL_STATIC);
	}
	return TCL_OK;
    }

    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->sizeHintsFlags &= ~(USSize|PSize);
    } else {
	if (Tcl_GetIndexFromObj(interp, objv[3], optionStrings, "argument", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_USER) {
	    wmPtr->sizeHintsFlags &= ~PSize;
	    wmPtr->sizeHintsFlags |= USSize;
	} else { /* OPT_PROGRAM */
	    wmPtr->sizeHintsFlags &= ~USSize;
	    wmPtr->sizeHintsFlags |= PSize;
	}
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmStackorderCmd --
 *
 *	This procedure is invoked to process the "wm stackorder" Tcl command.
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

static int
WmStackorderCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    TkWindow **windows, **window_ptr;
    static CONST char *optionStrings[] = {
	"isabove", "isbelow", (char *) NULL };
    enum options {
	OPT_ISABOVE, OPT_ISBELOW };
    int index;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?isabove|isbelow window?");
	return TCL_ERROR;
    }

    if (objc == 3) {
	windows = TkWmStackorderToplevel(winPtr);
	if (windows == NULL) {
	    panic("TkWmStackorderToplevel failed");
	} else {
	    for (window_ptr = windows; *window_ptr ; window_ptr++) {
		Tcl_AppendElement(interp, (*window_ptr)->pathName);
	    }
	    ckfree((char *) windows);
	    return TCL_OK;
	}
    } else {
	TkWindow *winPtr2;
	int index1=-1, index2=-1, result;

	if (TkGetWindowFromObj(interp, tkwin, objv[4], (Tk_Window *) &winPtr2)
		!= TCL_OK) {
	    return TCL_ERROR;
	}

	if (!Tk_IsTopLevel(winPtr2)) {
	    Tcl_AppendResult(interp, "window \"", winPtr2->pathName,
		    "\" isn't a top-level window", (char *) NULL);
	    return TCL_ERROR;
	}

	if (!Tk_IsMapped(winPtr)) {
	    Tcl_AppendResult(interp, "window \"", winPtr->pathName,
		    "\" isn't mapped", (char *) NULL);
	    return TCL_ERROR;
	}

	if (!Tk_IsMapped(winPtr2)) {
	    Tcl_AppendResult(interp, "window \"", winPtr2->pathName,
		    "\" isn't mapped", (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Lookup stacking order of all toplevels that are children
	 * of "." and find the position of winPtr and winPtr2
	 * in the stacking order.
	 */

	windows = TkWmStackorderToplevel(winPtr->mainPtr->winPtr);

	if (windows == NULL) {
	    Tcl_AppendResult(interp, "TkWmStackorderToplevel failed",
                    (char *) NULL);
	    return TCL_ERROR;
	} else {
	    for (window_ptr = windows; *window_ptr ; window_ptr++) {
		if (*window_ptr == winPtr)
		    index1 = (window_ptr - windows);
		if (*window_ptr == winPtr2)
		    index2 = (window_ptr - windows);
	    }
	    if (index1 == -1)
		panic("winPtr window not found");
	    if (index2 == -1)
		panic("winPtr2 window not found");

	    ckfree((char *) windows);
	}

	if (Tcl_GetIndexFromObj(interp, objv[3], optionStrings, "argument", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_ISABOVE) {
	    result = index1 > index2;
	} else { /* OPT_ISBELOW */
	    result = index1 < index2;
	}
	Tcl_SetIntObj(Tcl_GetObjResult(interp), result);
	return TCL_OK;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmStateCmd --
 *
 *	This procedure is invoked to process the "wm state" Tcl command.
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

static int
WmStateCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static CONST char *optionStrings[] = {
	"normal", "iconic", "withdrawn", (char *) NULL };
    enum options {
	OPT_NORMAL, OPT_ICONIC, OPT_WITHDRAWN };
    int index;

    if ((objc < 3) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?state?");
	return TCL_ERROR;
    }
    if (objc == 4) {
	if (wmPtr->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't change state of ",
		    Tcl_GetString(objv[2]),
		    ": it is an icon for ", Tk_PathName(wmPtr->iconFor),
		    (char *) NULL);
	    return TCL_ERROR;
	}

	if (Tcl_GetIndexFromObj(interp, objv[3], optionStrings, "argument", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (index == OPT_NORMAL) {
	    wmPtr->flags &= ~WM_WITHDRAWN;
	    (void) TkpWmSetState(winPtr, NormalState);
	} else if (index == OPT_ICONIC) {
	    if (Tk_Attributes((Tk_Window) winPtr)->override_redirect) {
		Tcl_AppendResult(interp, "can't iconify \"",
			winPtr->pathName,
			"\": override-redirect flag is set",
			(char *) NULL);
		return TCL_ERROR;
	    }
	    if (wmPtr->masterPtr != NULL) {
		Tcl_AppendResult(interp, "can't iconify \"",
			winPtr->pathName,
			"\": it is a transient", (char *) NULL);
		return TCL_ERROR;
	    }
	    if (TkpWmSetState(winPtr, IconicState) == 0) {
		Tcl_SetResult(interp,
			"couldn't send iconify message to window manager",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	} else { /* OPT_WITHDRAWN */
	    wmPtr->flags |= WM_WITHDRAWN;
	    if (TkpWmSetState(winPtr, WithdrawnState) == 0) {
		Tcl_SetResult(interp,
			"couldn't send withdraw message to window manager",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	}
    } else {
	if (wmPtr->iconFor != NULL) {
	    Tcl_SetResult(interp, "icon", TCL_STATIC);
	} else if (wmPtr->withdrawn) {
	    Tcl_SetResult(interp, "withdrawn", TCL_STATIC);
	} else if (Tk_IsMapped((Tk_Window) winPtr)
		|| ((wmPtr->flags & WM_NEVER_MAPPED)
			&& (wmPtr->hints.initial_state == NormalState))) {
	    Tcl_SetResult(interp, "normal", TCL_STATIC);
	} else {
	    Tcl_SetResult(interp, "iconic", TCL_STATIC);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmTitleCmd --
 *
 *	This procedure is invoked to process the "wm title" Tcl command.
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

static int
WmTitleCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    char *argv3;
    int length;

    if (objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?newTitle?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_SetResult(interp, (char *)
		((wmPtr->title != NULL) ? wmPtr->title : winPtr->nameUid),
		TCL_STATIC);
	return TCL_OK;
    } else {
	if (wmPtr->title != NULL) {
	    ckfree((char *) wmPtr->title);
	}
	argv3 = Tcl_GetStringFromObj(objv[3], &length);
	wmPtr->title = ckalloc((unsigned) (length + 1));
	strcpy(wmPtr->title, argv3);

	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    XTextProperty textProp;
	    Tcl_DString ds;

	    Tcl_UtfToExternalDString(NULL, wmPtr->title, -1, &ds);
	    if (XStringListToTextProperty(&(Tcl_DStringValue(&ds)), 1,
		    &textProp)  != 0) {
		XSetWMName(winPtr->display, wmPtr->wrapperPtr->window,
			&textProp);
		XFree((char *) textProp.value);
	    }
	    Tcl_DStringFree(&ds);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmTransientCmd --
 *
 *	This procedure is invoked to process the "wm transient" Tcl command.
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

static int
WmTransientCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    TkWindow *masterPtr = wmPtr->masterPtr;
    WmInfo *wmPtr2;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?master?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (masterPtr != NULL) {
	    Tcl_SetResult(interp, Tk_PathName(masterPtr), TCL_STATIC);
	}
	return TCL_OK;
    }
    if (Tcl_GetString(objv[3])[0] == '\0') {
	if (masterPtr != NULL) {
	    /*
	     * If we had a master, tell them that we aren't tied
	     * to them anymore
	     */

	    masterPtr->wmInfoPtr->numTransients--;
	    Tk_DeleteEventHandler((Tk_Window) masterPtr,
		    StructureNotifyMask,
		    WmWaitMapProc, (ClientData) winPtr);

	    /* FIXME: Need a call like Win32's UpdateWrapper() so
	       we can recreate the wrapper and get rid of the
	       transient window decorations. */
	}

	wmPtr->masterPtr = NULL;
    } else {
	if (TkGetWindowFromObj(interp, tkwin, objv[3],
		(Tk_Window *) &masterPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	while (!Tk_TopWinHierarchy(masterPtr)) {
	    /*
	     * Ensure that the master window is actually a Tk toplevel.
	     */

	    masterPtr = masterPtr->parentPtr;
	}
	Tk_MakeWindowExist((Tk_Window) masterPtr);

	if (wmPtr->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't make \"",
		    Tcl_GetString(objv[2]),
		    "\" a transient: it is an icon for ",
		    Tk_PathName(wmPtr->iconFor),
		    (char *) NULL);
	    return TCL_ERROR;
	}

	wmPtr2 = masterPtr->wmInfoPtr;
	if (wmPtr2->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr2);
	}

	if (wmPtr2->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't make \"",
		    Tcl_GetString(objv[3]),
		    "\" a master: it is an icon for ",
		    Tk_PathName(wmPtr2->iconFor),
		    (char *) NULL);
	    return TCL_ERROR;
	}

	if (masterPtr == winPtr) {
	    Tcl_AppendResult(interp, "can't make \"", Tk_PathName(winPtr),
		    "\" its own master",
		    (char *) NULL);
	    return TCL_ERROR;
	} else if (masterPtr != wmPtr->masterPtr) {
	    /*
	     * Remove old master map/unmap binding before setting
	     * the new master. The event handler will ensure that
	     * transient states reflect the state of the master.
	     */

	    if (wmPtr->masterPtr != NULL) {
		wmPtr->masterPtr->wmInfoPtr->numTransients--;
		Tk_DeleteEventHandler((Tk_Window) wmPtr->masterPtr,
			StructureNotifyMask,
			WmWaitMapProc, (ClientData) winPtr);
	    }

	    masterPtr->wmInfoPtr->numTransients++;
	    Tk_CreateEventHandler((Tk_Window) masterPtr,
		    StructureNotifyMask,
		    WmWaitMapProc, (ClientData) winPtr);

	    wmPtr->masterPtr = masterPtr;
	}
    }
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	if (wmPtr->masterPtr != NULL && !Tk_IsMapped(wmPtr->masterPtr)) {
	    if (TkpWmSetState(winPtr, WithdrawnState) == 0) {
		Tcl_SetResult(interp,
			"couldn't send withdraw message to window manager",
			TCL_STATIC);
		return TCL_ERROR;
	    }
	} else {
	    Window xwin = (wmPtr->masterPtr == NULL) ? None :
		    wmPtr->masterPtr->wmInfoPtr->wrapperPtr->window;
	    XSetTransientForHint(winPtr->display, wmPtr->wrapperPtr->window,
		    xwin);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmWithdrawCmd --
 *
 *	This procedure is invoked to process the "wm withdraw" Tcl command.
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

static int
WmWithdrawCmd(tkwin, winPtr, interp, objc, objv)
    Tk_Window tkwin;		/* Main window of the application. */
    TkWindow *winPtr;           /* Toplevel to work with */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    if (wmPtr->iconFor != NULL) {
	Tcl_AppendResult(interp, "can't withdraw ", Tcl_GetString(objv[2]),
		": it is an icon for ", Tk_PathName(wmPtr->iconFor),
		(char *) NULL);
	return TCL_ERROR;
    }
    wmPtr->flags |= WM_WITHDRAWN;
    if (TkpWmSetState(winPtr, WithdrawnState) == 0) {
	Tcl_SetResult(interp,
		"couldn't send withdraw message to window manager",
		TCL_STATIC);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * Invoked by those wm subcommands that affect geometry.
 * Schedules a geometry update.
 */
static void
WmUpdateGeom(wmPtr, winPtr)
    WmInfo *wmPtr;
    TkWindow *winPtr;
{
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 * Invoked when a MapNotify or UnmapNotify event is delivered for a
 * toplevel that is the master of a transient toplevel.
 */
static void
WmWaitMapProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to window. */
    XEvent *eventPtr;		/* Information about event. */
{
    TkWindow *winPtr = (TkWindow *) clientData;
    TkWindow *masterPtr = winPtr->wmInfoPtr->masterPtr;

    if (masterPtr == NULL)
        return;

    if (eventPtr->type == MapNotify) {
        if (!(winPtr->wmInfoPtr->flags & WM_WITHDRAWN))
            (void) TkpWmSetState(winPtr, NormalState);
    } else if (eventPtr->type == UnmapNotify) {
        (void) TkpWmSetState(winPtr, WithdrawnState);
    }
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
Tk_SetGrid(tkwin, reqWidth, reqHeight, widthInc, heightInc)
    Tk_Window tkwin;		/* Token for window.  New window mgr info
				 * will be posted for the top-level window
				 * associated with this window. */
    int reqWidth;		/* Width (in grid units) corresponding to
				 * the requested geometry for tkwin. */
    int reqHeight;		/* Height (in grid units) corresponding to
				 * the requested geometry for tkwin. */
    int widthInc, heightInc;	/* Pixel increments corresponding to a
				 * change of one grid unit. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr;

    /*
     * Find the top-level window for tkwin, plus the window manager
     * information.
     */

    while (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	winPtr = winPtr->parentPtr;
	if (winPtr == NULL) {
	    /*
	     * The window is being deleted... just skip this operation.
	     */

	    return;
	}
    }
    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }

    if ((wmPtr->gridWin != NULL) && (wmPtr->gridWin != tkwin)) {
	return;
    }

    if ((wmPtr->reqGridWidth == reqWidth)
	    && (wmPtr->reqGridHeight == reqHeight)
	    && (wmPtr->widthInc == widthInc)
	    && (wmPtr->heightInc == heightInc)
	    && ((wmPtr->sizeHintsFlags & (PBaseSize|PResizeInc))
		    == (PBaseSize|PResizeInc))) {
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
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
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
Tk_UnsetGrid(tkwin)
    Tk_Window tkwin;		/* Token for window that is currently
				 * controlling gridding. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr;

    /*
     * Find the top-level window for tkwin, plus the window manager
     * information.
     */

    while (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	winPtr = winPtr->parentPtr;
	if (winPtr == NULL) {
	    /*
	     * The window is being deleted... just skip this operation.
	     */

	    return;
	}
    }
    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }

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
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureEvent --
 *
 *	This procedure is called to handle ConfigureNotify events on
 *	wrapper windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets updated in the WmInfo structure for the window
 *	and the toplevel itself gets repositioned within the wrapper.
 *
 *----------------------------------------------------------------------
 */

static void
ConfigureEvent(wmPtr, configEventPtr)
    WmInfo *wmPtr;			/* Information about toplevel window. */
    XConfigureEvent *configEventPtr;	/* Event that just occurred for
					 * wmPtr->wrapperPtr. */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    TkWindow *winPtr = wmPtr->winPtr;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;
    Tk_ErrorHandler handler;

    /*
     * Update size information from the event.  There are a couple of
     * tricky points here:
     *
     * 1. If the user changed the size externally then set wmPtr->width
     *    and wmPtr->height just as if a "wm geometry" command had been
     *    invoked with the same information.
     * 2. However, if the size is changing in response to a request
     *    coming from us (WM_SYNC_PENDING is set), then don't set wmPtr->width
     *    or wmPtr->height if they were previously -1 (otherwise the
     *    window will stop tracking geometry manager requests).
     */

    if (((wrapperPtr->changes.width != configEventPtr->width)
	    || (wrapperPtr->changes.height != configEventPtr->height))
	    && !(wmPtr->flags & WM_SYNC_PENDING)){
	if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("TopLevelEventProc: user changed %s size to %dx%d\n",
		    winPtr->pathName, configEventPtr->width,
		    configEventPtr->height);
	}
	if ((wmPtr->width == -1)
		&& (configEventPtr->width == winPtr->reqWidth)) {
	    /*
	     * Don't set external width, since the user didn't change it
	     * from what the widgets asked for.
	     */
	} else {
	    /*
	     * Note: if this window is embedded then don't set the external
	     * size, since it came from the containing application, not the
	     * user.  In this case we want to keep sending our size requests
	     * to the containing application;  if the user fixes the size
	     * of that application then it will still percolate down to us
	     * in the right way.
	     */

	    if (!(winPtr->flags & TK_EMBEDDED)) {
		if (wmPtr->gridWin != NULL) {
		    wmPtr->width = wmPtr->reqGridWidth
			    + (configEventPtr->width
			    - winPtr->reqWidth)/wmPtr->widthInc;
		    if (wmPtr->width < 0) {
			wmPtr->width = 0;
		    }
		} else {
		    wmPtr->width = configEventPtr->width;
		}
	    }
	}
	if ((wmPtr->height == -1)
		&& (configEventPtr->height ==
			(winPtr->reqHeight + wmPtr->menuHeight))) {
	    /*
	     * Don't set external height, since the user didn't change it
	     * from what the widgets asked for.
	     */
	} else {
	    /*
	     * See note for wmPtr->width about not setting external size
	     * for embedded windows.
	     */

	    if (!(winPtr->flags & TK_EMBEDDED)) {
		if (wmPtr->gridWin != NULL) {
		    wmPtr->height = wmPtr->reqGridHeight
			    + (configEventPtr->height - wmPtr->menuHeight
			    - winPtr->reqHeight)/wmPtr->heightInc;
		    if (wmPtr->height < 0) {
			wmPtr->height = 0;
		    }
		} else {
		    wmPtr->height = configEventPtr->height - wmPtr->menuHeight;
		}
            }
	}
	wmPtr->configWidth = configEventPtr->width;
	wmPtr->configHeight = configEventPtr->height;
    }

    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("ConfigureEvent: %s x = %d y = %d, width = %d, height = %d\n",
		winPtr->pathName, configEventPtr->x, configEventPtr->y,
		configEventPtr->width, configEventPtr->height);
	printf("    send_event = %d, serial = %ld (win %p, wrapper %p)\n",
		configEventPtr->send_event, configEventPtr->serial,
		winPtr, wrapperPtr);
    }
    wrapperPtr->changes.width = configEventPtr->width;
    wrapperPtr->changes.height = configEventPtr->height;
    wrapperPtr->changes.border_width = configEventPtr->border_width;
    wrapperPtr->changes.sibling = configEventPtr->above;
    wrapperPtr->changes.stack_mode = Above;

    /*
     * Reparenting window managers make life difficult.  If the
     * window manager reparents a top-level window then the x and y
     * information that comes in events for the window is wrong:
     * it gives the location of the window inside its decorative
     * parent, rather than the location of the window in root
     * coordinates, which is what we want.  Window managers
     * are supposed to send synthetic events with the correct
     * information, but ICCCM doesn't require them to do this
     * under all conditions, and the information provided doesn't
     * include everything we need here.  So, the code below
     * maintains a bunch of information about the parent window.
     * If the window hasn't been reparented, we pretend that
     * there is a parent shrink-wrapped around the window.
     */

    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("    %s parent == %p, above %p\n",
		winPtr->pathName, (void *) wmPtr->reparent,
		(void *) configEventPtr->above);
    }

    if ((wmPtr->reparent == None) || !ComputeReparentGeometry(wmPtr)) {
	wmPtr->parentWidth = configEventPtr->width
		+ 2*configEventPtr->border_width;
	wmPtr->parentHeight = configEventPtr->height
		+ 2*configEventPtr->border_width;
	wrapperPtr->changes.x = wmPtr->x = configEventPtr->x;
	wrapperPtr->changes.y = wmPtr->y = configEventPtr->y;
	if (wmPtr->flags & WM_NEGATIVE_X) {
	    wmPtr->x = wmPtr->vRootWidth - (wmPtr->x + wmPtr->parentWidth);
	}
	if (wmPtr->flags & WM_NEGATIVE_Y) {
	    wmPtr->y = wmPtr->vRootHeight - (wmPtr->y + wmPtr->parentHeight);
	}
    }

    /*
     * Make sure that the toplevel and menubar are properly positioned within
     * the wrapper.  If the menuHeight happens to be zero, we'll get a
     * BadValue X error that we want to ignore [Bug: 3377]
     */
    handler = Tk_CreateErrorHandler(winPtr->display, -1, -1, -1,
	    (Tk_ErrorProc *) NULL, (ClientData) NULL);
    XMoveResizeWindow(winPtr->display, winPtr->window, 0,
	    wmPtr->menuHeight, (unsigned) wrapperPtr->changes.width,
	    (unsigned) (wrapperPtr->changes.height - wmPtr->menuHeight));
    Tk_DeleteErrorHandler(handler);
    if ((wmPtr->menubar != NULL)
	    && ((Tk_Width(wmPtr->menubar) != wrapperPtr->changes.width)
	    || (Tk_Height(wmPtr->menubar) != wmPtr->menuHeight))) {
	Tk_MoveResizeWindow(wmPtr->menubar, 0, 0, wrapperPtr->changes.width,
		wmPtr->menuHeight);
    }

    /*
     * Update the coordinates in the toplevel (they should refer to the
     * position in root window coordinates, not the coordinates of the
     * wrapper window).  Then synthesize a ConfigureNotify event to tell
     * the application about the change.
     */

    winPtr->changes.x = wrapperPtr->changes.x;
    winPtr->changes.y = wrapperPtr->changes.y + wmPtr->menuHeight;
    winPtr->changes.width = wrapperPtr->changes.width;
    winPtr->changes.height = wrapperPtr->changes.height - wmPtr->menuHeight;
    TkDoConfigureNotify(winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ReparentEvent --
 *
 *	This procedure is called to handle ReparentNotify events on
 *	wrapper windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets updated in the WmInfo structure for the window.
 *
 *----------------------------------------------------------------------
 */

static void
ReparentEvent(wmPtr, reparentEventPtr)
    WmInfo *wmPtr;			/* Information about toplevel window. */
    XReparentEvent *reparentEventPtr;	/* Event that just occurred for
					 * wmPtr->wrapperPtr. */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    Window vRoot, ancestor, *children, dummy2, *virtualRootPtr;
    Atom actualType;
    int actualFormat;
    unsigned long numItems, bytesAfter;
    unsigned int dummy;
    Tk_ErrorHandler handler;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;

    /*
     * Identify the root window for wrapperPtr.  This is tricky because of
     * virtual root window managers like tvtwm.  If the window has a
     * property named __SWM_ROOT or __WM_ROOT then this property gives
     * the id for a virtual root window that should be used instead of
     * the root window of the screen.
     */

    vRoot = RootWindow(wrapperPtr->display, wrapperPtr->screenNum);
    wmPtr->vRoot = None;
    handler = Tk_CreateErrorHandler(wrapperPtr->display, -1, -1, -1,
	    (Tk_ErrorProc *) NULL, (ClientData) NULL);
    if (((XGetWindowProperty(wrapperPtr->display, wrapperPtr->window,
	    Tk_InternAtom((Tk_Window) wrapperPtr, "__WM_ROOT"), 0, (long) 1,
	    False, XA_WINDOW, &actualType, &actualFormat, &numItems,
	    &bytesAfter, (unsigned char **) &virtualRootPtr) == Success)
	    && (actualType == XA_WINDOW))
	    || ((XGetWindowProperty(wrapperPtr->display, wrapperPtr->window,
	    Tk_InternAtom((Tk_Window) wrapperPtr, "__SWM_ROOT"), 0, (long) 1,
	    False, XA_WINDOW, &actualType, &actualFormat, &numItems,
	    &bytesAfter, (unsigned char **) &virtualRootPtr) == Success)
	    && (actualType == XA_WINDOW))) {
	if ((actualFormat == 32) && (numItems == 1)) {
	    vRoot = wmPtr->vRoot = *virtualRootPtr;
	} else if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("%s format %d numItems %ld\n",
		    "ReparentEvent got bogus VROOT property:", actualFormat,
		    numItems);
	}
	XFree((char *) virtualRootPtr);
    }
    Tk_DeleteErrorHandler(handler);

    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("ReparentEvent: %s (%p) reparented to 0x%x, vRoot = 0x%x\n",
		wmPtr->winPtr->pathName, wmPtr->winPtr,
		(unsigned int) reparentEventPtr->parent, (unsigned int) vRoot);
    }

    /*
     * Fetch correct geometry information for the new virtual root.
     */

    UpdateVRootGeometry(wmPtr);

    /*
     * If the window's new parent is the root window, then mark it as
     * no longer reparented.
     */

    if (reparentEventPtr->parent == vRoot) {
	noReparent:
	wmPtr->reparent = None;
	wmPtr->parentWidth = wrapperPtr->changes.width;
	wmPtr->parentHeight = wrapperPtr->changes.height;
	wmPtr->xInParent = wmPtr->yInParent = 0;
	wrapperPtr->changes.x = reparentEventPtr->x;
	wrapperPtr->changes.y = reparentEventPtr->y;
	wmPtr->winPtr->changes.x = reparentEventPtr->x;
	wmPtr->winPtr->changes.y = reparentEventPtr->y + wmPtr->menuHeight;
	return;
    }

    /*
     * Search up the window hierarchy to find the ancestor of this
     * window that is just below the (virtual) root.  This is tricky
     * because it's possible that things have changed since the event
     * was generated so that the ancestry indicated by the event no
     * longer exists.  If this happens then an error will occur and
     * we just discard the event (there will be a more up-to-date
     * ReparentNotify event coming later).
     */

    handler = Tk_CreateErrorHandler(wrapperPtr->display, -1, -1, -1,
	    (Tk_ErrorProc *) NULL, (ClientData) NULL);
    wmPtr->reparent = reparentEventPtr->parent;
    while (1) {
	if (XQueryTree(wrapperPtr->display, wmPtr->reparent, &dummy2,
		&ancestor, &children, &dummy) == 0) {
	    Tk_DeleteErrorHandler(handler);
	    goto noReparent;
	}
	XFree((char *) children);
	if ((ancestor == vRoot) ||
		(ancestor == RootWindow(wrapperPtr->display,
		wrapperPtr->screenNum))) {
	    break;
	}
	wmPtr->reparent = ancestor;
    }
    Tk_DeleteErrorHandler(handler);

    if (!ComputeReparentGeometry(wmPtr)) {
	goto noReparent;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeReparentGeometry --
 *
 *	This procedure is invoked to recompute geometry information
 *	related to a reparented top-level window, such as the position
 *	and total size of the parent and the position within it of
 *	the top-level window.
 *
 * Results:
 *	The return value is 1 if everything completed successfully
 *	and 0 if an error occurred while querying information about
 *	winPtr's parents.  In this case winPtr is marked as no longer
 *	being reparented.
 *
 * Side effects:
 *	Geometry information in wmPtr, wmPtr->winPtr, and
 *	wmPtr->wrapperPtr gets updated.
 *
 *----------------------------------------------------------------------
 */

static int
ComputeReparentGeometry(wmPtr)
    WmInfo *wmPtr;		/* Information about toplevel window
				 * whose reparent info is to be recomputed. */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    int width, height, bd;
    unsigned int dummy;
    int xOffset, yOffset, x, y;
    Window dummy2;
    Status status;
    Tk_ErrorHandler handler;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;

    handler = Tk_CreateErrorHandler(wrapperPtr->display, -1, -1, -1,
	    (Tk_ErrorProc *) NULL, (ClientData) NULL);
    (void) XTranslateCoordinates(wrapperPtr->display, wrapperPtr->window,
	    wmPtr->reparent, 0, 0, &xOffset, &yOffset, &dummy2);
    status = XGetGeometry(wrapperPtr->display, wmPtr->reparent,
	    &dummy2, &x, &y, (unsigned int *) &width,
	    (unsigned int *) &height, (unsigned int *) &bd, &dummy);
    Tk_DeleteErrorHandler(handler);
    if (status == 0) {
	/*
	 * It appears that the reparented parent went away and
	 * no-one told us.  Reset the window to indicate that
	 * it's not reparented.
	 */
	wmPtr->reparent = None;
	wmPtr->xInParent = wmPtr->yInParent = 0;
	return 0;
    }
    wmPtr->xInParent = xOffset + bd;
    wmPtr->yInParent = yOffset + bd;
    wmPtr->parentWidth = width + 2*bd;
    wmPtr->parentHeight = height + 2*bd;

    /*
     * Some tricky issues in updating wmPtr->x and wmPtr->y:
     *
     * 1. Don't update them if the event occurred because of something
     * we did (i.e. WM_SYNC_PENDING and WM_MOVE_PENDING are both set).
     * This is because window managers treat coords differently than Tk,
     * and no two window managers are alike. If the window manager moved
     * the window because we told it to, remember the coordinates we told
     * it, not the ones it actually moved it to.  This allows us to move
     * the window back to the same coordinates later and get the same
     * result. Without this check, windows can "walk" across the screen
     * under some conditions.
     *
     * 2. Don't update wmPtr->x and wmPtr->y unless wrapperPtr->changes.x
     * or wrapperPtr->changes.y has changed (otherwise a size change can
     * spoof us into thinking that the position changed too and defeat
     * the intent of (1) above.
     *
     * (As of 9/96 the above 2 comments appear to be stale.  They're
     * being left in place as a reminder of what was once true (and
     * perhaps should still be true?)).
     *
     * 3. Ignore size changes coming from the window system if we're
     * about to change the size ourselves but haven't seen the event for
     * it yet:  our size change is supposed to take priority.
     */

    if (!(wmPtr->flags & WM_MOVE_PENDING)
	    && ((wrapperPtr->changes.x != (x + wmPtr->xInParent))
	    || (wrapperPtr->changes.y != (y + wmPtr->yInParent)))) {
	wmPtr->x = x;
	if (wmPtr->flags & WM_NEGATIVE_X) {
	    wmPtr->x = wmPtr->vRootWidth - (wmPtr->x + wmPtr->parentWidth);
	}
	wmPtr->y = y;
	if (wmPtr->flags & WM_NEGATIVE_Y) {
	    wmPtr->y = wmPtr->vRootHeight - (wmPtr->y + wmPtr->parentHeight);
	}
    }

    wrapperPtr->changes.x = x + wmPtr->xInParent;
    wrapperPtr->changes.y = y + wmPtr->yInParent;
    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("wrapperPtr %p coords %d,%d\n",
		wrapperPtr, wrapperPtr->changes.x, wrapperPtr->changes.y);
	printf("     wmPtr %p coords %d,%d, offsets %d %d\n",
		wmPtr, wmPtr->x, wmPtr->y, wmPtr->xInParent, wmPtr->yInParent);
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * WrapperEventProc --
 *
 *	This procedure is invoked by the event loop when a wrapper window
 *	is restructured.
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
WrapperEventProc(clientData, eventPtr)
    ClientData clientData;		/* Information about toplevel window. */
    XEvent *eventPtr;			/* Event that just happened. */
{
    WmInfo *wmPtr = (WmInfo *) clientData;
    XEvent mapEvent;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;

    wmPtr->flags |= WM_VROOT_OFFSET_STALE;
    if (eventPtr->type == DestroyNotify) {
	Tk_ErrorHandler handler;

	if (!(wmPtr->wrapperPtr->flags & TK_ALREADY_DEAD)) {
	    /*
	     * A top-level window was deleted externally (e.g., by the window
	     * manager).  This is probably not a good thing, but cleanup as
	     * best we can.  The error handler is needed because
	     * Tk_DestroyWindow will try to destroy the window, but of course
	     * it's already gone.
	     */

	    handler = Tk_CreateErrorHandler(wmPtr->winPtr->display, -1, -1, -1,
		    (Tk_ErrorProc *) NULL, (ClientData) NULL);
	    Tk_DestroyWindow((Tk_Window) wmPtr->winPtr);
	    Tk_DeleteErrorHandler(handler);
	}
	if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("TopLevelEventProc: %s deleted\n", wmPtr->winPtr->pathName);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	/*
	 * Ignore the event if the window has never been mapped yet.
	 * Such an event occurs only in weird cases like changing the
	 * internal border width of a top-level window, which results
	 * in a synthetic Configure event.  These events are not relevant
	 * to us, and if we process them confusion may result (e.g. we
	 * may conclude erroneously that the user repositioned or resized
	 * the window).
	 */

	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    ConfigureEvent(wmPtr, &eventPtr->xconfigure);
	}
    } else if (eventPtr->type == MapNotify) {
	wmPtr->wrapperPtr->flags |= TK_MAPPED;
	wmPtr->winPtr->flags |= TK_MAPPED;
	XMapWindow(wmPtr->winPtr->display, wmPtr->winPtr->window);
	goto doMapEvent;
    } else if (eventPtr->type == UnmapNotify) {
	wmPtr->wrapperPtr->flags &= ~TK_MAPPED;
	wmPtr->winPtr->flags &= ~TK_MAPPED;
	XUnmapWindow(wmPtr->winPtr->display, wmPtr->winPtr->window);
	goto doMapEvent;
    } else if (eventPtr->type == ReparentNotify) {
	ReparentEvent(wmPtr, &eventPtr->xreparent);
    }
    return;

    doMapEvent:
    mapEvent = *eventPtr;
    mapEvent.xmap.event = wmPtr->winPtr->window;
    mapEvent.xmap.window = wmPtr->winPtr->window;
    Tk_HandleEvent(&mapEvent);
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
TopLevelReqProc(dummy, tkwin)
    ClientData dummy;			/* Not used. */
    Tk_Window tkwin;			/* Information about window. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr;

    wmPtr = winPtr->wmInfoPtr;

    if ((wmPtr->width >= 0) && (wmPtr->height >= 0)) {
	/*
	 * Explicit dimensions have been set for this window, so we
	 * should ignore the geometry request.  It's actually important
	 * to ignore the geometry request because, due to quirks in
	 * window managers, invoking UpdateGeometryInfo may cause the
	 * window to move.  For example, if "wm geometry -10-20" was
	 * invoked, the window may be positioned incorrectly the first
	 * time it appears (because we didn't know the proper width of
	 * the window manager borders); if we invoke UpdateGeometryInfo
	 * again, the window will be positioned correctly, which may
	 * cause it to jump on the screen.
	 */

	return;
    }

    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }

    /*
     * If the window isn't being positioned by its upper left corner
     * then we have to move it as well.
     */

    if (wmPtr->flags & (WM_NEGATIVE_X | WM_NEGATIVE_Y)) {
	wmPtr->flags |= WM_MOVE_PENDING;
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
 *	The size and location of both the toplevel window and its wrapper
 *	may change, unless the WM prevents that from happening.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateGeometryInfo(clientData)
    ClientData clientData;		/* Pointer to the window's record. */
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
     * Reconfigure the wrapper if it isn't already configured correctly.
     * A few tricky points:
     *
     * 1. If the window is embeddedand the container is also in this
     *    process, don't actually reconfigure the window; just pass the
     *    desired size on to the container.  Also, zero out any position
     *    information, since embedded windows are not allowed to move.
     * 2. Sometimes the window manager will give us a different size
     *    than we asked for (e.g. mwm has a minimum size for windows), so
     *    base the size check on what we *asked for* last time, not what we
     *    got.
     * 3. Can't just reconfigure always, because we may not get a
     *    ConfigureNotify event back if nothing changed, so
     *    WaitForConfigureNotify will hang a long time.
     * 4. Don't move window unless a new position has been requested for
     *	  it.  This is because of "features" in some window managers (e.g.
     *    twm, as of 4/24/91) where they don't interpret coordinates
     *    according to ICCCM.  Moving a window to its current location may
     *    cause it to shift position on the screen.
     */

    if ((winPtr->flags & (TK_EMBEDDED|TK_BOTH_HALVES))
	    == (TK_EMBEDDED|TK_BOTH_HALVES)) {
	/*
	 * This window is embedded and the container is also in this
	 * process, so we don't need to do anything special about the
	 * geometry, except to make sure that the desired size is known
	 * by the container.  Also, zero out any position information,
	 * since embedded windows are not allowed to move.
	 */

	wmPtr->x = wmPtr->y = 0;
	wmPtr->flags &= ~(WM_NEGATIVE_X|WM_NEGATIVE_Y);
	height += wmPtr->menuHeight;
	Tk_GeometryRequest((Tk_Window) TkpGetOtherWindow(winPtr),
		width, height);
	return;
    }
    serial = NextRequest(winPtr->display);
    height += wmPtr->menuHeight;
    if (wmPtr->flags & WM_MOVE_PENDING) {
	if ((x + wmPtr->xInParent == winPtr->changes.x) &&
		(y + wmPtr->yInParent + wmPtr->menuHeight == winPtr->changes.y)
		&& (width == wmPtr->wrapperPtr->changes.width)
		&& (height == wmPtr->wrapperPtr->changes.height)) {
	    /*
	     * The window already has the correct geometry, so don't bother
	     * to configure it;  the X server appears to ignore these
	     * requests, so we won't get back a ConfigureNotify and the
	     * WaitForConfigureNotify call below will hang for a while.
	     */

	    wmPtr->flags &= ~WM_MOVE_PENDING;
	    return;
	}
	wmPtr->configWidth = width;
	wmPtr->configHeight = height;
	if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	   printf("UpdateGeometryInfo moving to %d %d, resizing to %d x %d,\n",
                   x, y, width, height);
	}
	XMoveResizeWindow(winPtr->display, wmPtr->wrapperPtr->window, x, y,
		(unsigned) width, (unsigned) height);
    } else if ((width != wmPtr->configWidth)
	    || (height != wmPtr->configHeight)) {
	if ((width == wmPtr->wrapperPtr->changes.width)
		&& (height == wmPtr->wrapperPtr->changes.height)) {
	    /*
	     * The window is already just the size we want, so don't bother
	     * to configure it;  the X server appears to ignore these
	     * requests, so we won't get back a ConfigureNotify and the
	     * WaitForConfigureNotify call below will hang for a while.
	     */

	    return;
	}
	wmPtr->configWidth = width;
	wmPtr->configHeight = height;
	if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("UpdateGeometryInfo resizing %p to %d x %d\n",
		    (void *)wmPtr->wrapperPtr->window, width, height);
	}
	XResizeWindow(winPtr->display, wmPtr->wrapperPtr->window,
		(unsigned) width, (unsigned) height);
    } else if ((wmPtr->menubar != NULL)
	    && ((Tk_Width(wmPtr->menubar) != wmPtr->wrapperPtr->changes.width)
	    || (Tk_Height(wmPtr->menubar) != wmPtr->menuHeight))) {
	/*
	 * It is possible that the window's overall size has not changed
	 * but the menu size has.
	 */

	Tk_MoveResizeWindow(wmPtr->menubar, 0, 0,
		wmPtr->wrapperPtr->changes.width, wmPtr->menuHeight);
	XResizeWindow(winPtr->display, wmPtr->wrapperPtr->window,
		(unsigned) width, (unsigned) height);
    } else {
	return;
    }

    /*
     * Wait for the configure operation to complete.  Don't need to do
     * this, however, if the window is about to be mapped:  it will be
     * taken care of elsewhere.
     */

    if (!(wmPtr->flags & WM_ABOUT_TO_MAP)) {
	WaitForConfigureNotify(winPtr, serial);
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
UpdateSizeHints(winPtr)
    TkWindow *winPtr;
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    XSizeHints *hintsPtr;
    int maxWidth, maxHeight;

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

    GetMaxSize(wmPtr, &maxWidth, &maxHeight);
    if (wmPtr->gridWin != NULL) {
	hintsPtr->base_width = winPtr->reqWidth
		- (wmPtr->reqGridWidth * wmPtr->widthInc);
	if (hintsPtr->base_width < 0) {
	    hintsPtr->base_width = 0;
	}
	hintsPtr->base_height = winPtr->reqHeight + wmPtr->menuHeight
		- (wmPtr->reqGridHeight * wmPtr->heightInc);
	if (hintsPtr->base_height < 0) {
	    hintsPtr->base_height = 0;
	}
	hintsPtr->min_width = hintsPtr->base_width
		+ (wmPtr->minWidth * wmPtr->widthInc);
	hintsPtr->min_height = hintsPtr->base_height
		+ (wmPtr->minHeight * wmPtr->heightInc);
	hintsPtr->max_width = hintsPtr->base_width
		+ (maxWidth * wmPtr->widthInc);
	hintsPtr->max_height = hintsPtr->base_height
		+ (maxHeight * wmPtr->heightInc);
    } else {
	hintsPtr->min_width = wmPtr->minWidth;
	hintsPtr->min_height = wmPtr->minHeight;
	hintsPtr->max_width = maxWidth;
	hintsPtr->max_height = maxHeight;
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
	    hintsPtr->min_height = winPtr->reqHeight + wmPtr->menuHeight;
	}
	hintsPtr->max_height = hintsPtr->min_height;
    }

    XSetWMNormalHints(winPtr->display, wmPtr->wrapperPtr->window, hintsPtr);

    XFree((char *) hintsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForConfigureNotify --
 *
 *	This procedure is invoked in order to synchronize with the
 *	window manager.  It waits for a ConfigureNotify event to
 *	arrive, signalling that the window manager has seen an attempt
 *	on our part to move or resize a top-level window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Delays the execution of the process until a ConfigureNotify event
 *	arrives with serial number at least as great as serial.  This
 *	is useful for two reasons:
 *
 *	1. It's important to distinguish ConfigureNotify events that are
 *	   coming in response to a request we've made from those generated
 *	   spontaneously by the user.  The reason for this is that if the
 *	   user resizes the window we take that as an order to ignore
 *	   geometry requests coming from inside the window hierarchy.  If
 *	   we accidentally interpret a response to our request as a
 *	   user-initiated action, the window will stop responding to
 *	   new geometry requests.  To make this distinction, (a) this
 *	   procedure sets a flag for TopLevelEventProc to indicate that
 *	   we're waiting to sync with the wm, and (b) all changes to
 *	   the size of a top-level window are followed by calls to this
 *	   procedure.
 *	2. Races and confusion can come about if there are multiple
 *	   operations outstanding at a time (e.g. two different resizes
 *	   of the top-level window:  it's hard to tell which of the
 *	   ConfigureNotify events coming back is for which request).
 *	While waiting, some events covered by StructureNotifyMask are
 *	processed (ConfigureNotify, MapNotify, and UnmapNotify) 
 *	and all others are deferred.
 *
 *----------------------------------------------------------------------
 */

static void
WaitForConfigureNotify(winPtr, serial)
    TkWindow *winPtr;		/* Top-level window for which we want
				 * to see a ConfigureNotify. */
    unsigned long serial;	/* Serial number of resize request.  Want to
				 * be sure wm has seen this. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    XEvent event;
    int diff, code;
    int gotConfig = 0;

    /*
     * One more tricky detail about this procedure.  In some cases the
     * window manager will decide to ignore a configure request (e.g.
     * because it thinks the window is already in the right place).
     * To avoid hanging in this situation, only wait for a few seconds,
     * then give up.
     */

    while (!gotConfig) {
	wmPtr->flags |= WM_SYNC_PENDING;
	code = WaitForEvent(winPtr->display, wmPtr, ConfigureNotify, &event);
	wmPtr->flags &= ~WM_SYNC_PENDING;
	if (code != TCL_OK) {
	    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
		printf("WaitForConfigureNotify giving up on %s\n",
			winPtr->pathName);
	    }
	    break;
	}
	diff = event.xconfigure.serial - serial;
	if (diff >= 0) {
	    gotConfig = 1;
	}
    }
    wmPtr->flags &= ~WM_MOVE_PENDING;
    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("WaitForConfigureNotify finished with %s, serial %ld\n",
		winPtr->pathName, serial);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForEvent --
 *
 *	This procedure is used by WaitForConfigureNotify and
 *	WaitForMapNotify to wait for an event of a certain type
 *	to arrive.
 *
 * Results:
 *	Under normal conditions, TCL_OK is returned and an event for
 *	display and window that matches "mask" is stored in *eventPtr.
 *	This event  has already been processed by Tk before this procedure
 *	returns.  If a long time goes by with no event of the right type
 *	arriving, or if an error occurs while waiting for the event to
 *	arrive, then TCL_ERROR is returned.
 *
 * Side effects:
 *	While waiting for the desired event to occur, Configurenotify,
 *	MapNotify, and UnmapNotify events for window are processed, 
 *	as are all ReparentNotify events.
 *
 *----------------------------------------------------------------------
 */

static int
WaitForEvent(display, wmInfoPtr, type, eventPtr)
    Display *display;		/* Display event is coming from. */
    WmInfo *wmInfoPtr;		/* Window for which event is desired. */
    int type;			/* Type of event that is wanted. */
    XEvent *eventPtr;		/* Place to store event. */
{
    WaitRestrictInfo info;
    Tk_RestrictProc *oldRestrictProc;
    ClientData oldRestrictData;
    Tcl_Time timeout;

    /*
     * Set up an event filter to select just the events we want, and
     * a timer handler, then wait for events until we get the event
     * we want or a timeout happens.
     */

    info.display = display;
    info.wmInfoPtr = wmInfoPtr;
    info.type = type;
    info.eventPtr = eventPtr;
    info.foundEvent = 0;
    oldRestrictProc = Tk_RestrictEvents(WaitRestrictProc, (ClientData) &info,
	    &oldRestrictData);

    TclpGetTime(&timeout);
    timeout.sec += 2;

    while (!info.foundEvent) {
	if (!TkUnixDoOneXEvent(&timeout)) {
	    break;
	}
    }
    (void) Tk_RestrictEvents(oldRestrictProc, oldRestrictData,
	    &oldRestrictData);
    if (info.foundEvent) {
	return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitRestrictProc --
 *
 *	This procedure is a Tk_RestrictProc that is used to filter
 *	events while WaitForEvent is active.
 *
 * Results:
 *	Returns TK_PROCESS_EVENT if the right event is found.  Also
 *	returns TK_PROCESS_EVENT if any ReparentNotify event is found
 *	or if the event is a ConfigureNotify, MapNotify, or UnmapNotify
 *	for window.  Otherwise returns TK_DEFER_EVENT.
 *
 * Side effects:
 *	An event may get stored in the area indicated by the caller
 *	of WaitForEvent.
 *
 *----------------------------------------------------------------------
 */

static Tk_RestrictAction
WaitRestrictProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to WaitRestrictInfo structure. */
    XEvent *eventPtr;		/* Event that is about to be handled. */
{
    WaitRestrictInfo *infoPtr = (WaitRestrictInfo *) clientData;

    if (eventPtr->type == ReparentNotify) {
	return TK_PROCESS_EVENT;
    }
    if (((eventPtr->xany.window != infoPtr->wmInfoPtr->wrapperPtr->window)
	    && (eventPtr->xany.window != infoPtr->wmInfoPtr->reparent))
	    || (eventPtr->xany.display != infoPtr->display)) {
	return TK_DEFER_EVENT;
    }
    if (eventPtr->type == infoPtr->type) {
	*infoPtr->eventPtr = *eventPtr;
	infoPtr->foundEvent = 1;
	return TK_PROCESS_EVENT;
    }
    if (eventPtr->type == ConfigureNotify 
	   || eventPtr->type == MapNotify
	   || eventPtr->type == UnmapNotify) {
	return TK_PROCESS_EVENT;
    }
    return TK_DEFER_EVENT;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForMapNotify --
 *
 *	This procedure is invoked in order to synchronize with the
 *	window manager.  It waits for the window's mapped state to
 *	reach the value given by mapped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Delays the execution of the process until winPtr becomes mapped
 *	or unmapped, depending on the "mapped" argument.  This allows us
 *	to synchronize with the window manager, and allows us to
 *	identify changes in window size that come about when the window
 *	manager first starts managing the window (as opposed to those
 *	requested interactively by the user later).  See the comments
 *	for WaitForConfigureNotify and WM_SYNC_PENDING.  While waiting,
 *	some events covered by StructureNotifyMask are processed and all
 *	others are deferred.
 *
 *----------------------------------------------------------------------
 */

static void
WaitForMapNotify(winPtr, mapped)
    TkWindow *winPtr;		/* Top-level window for which we want
				 * to see a particular mapping state. */
    int mapped;			/* If non-zero, wait for window to become
				 * mapped, otherwise wait for it to become
				 * unmapped. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    XEvent event;
    int code;

    while (1) {
	if (mapped) {
	    if (winPtr->flags & TK_MAPPED) {
		break;
	    }
	} else if (!(winPtr->flags & TK_MAPPED)) {
	    break;
	}
	wmPtr->flags |= WM_SYNC_PENDING;
	code = WaitForEvent(winPtr->display, wmPtr,
		mapped ? MapNotify : UnmapNotify, &event);
	wmPtr->flags &= ~WM_SYNC_PENDING;
	if (code != TCL_OK) {
	    /*
	     * There are some bizarre situations in which the window
	     * manager can't respond or chooses not to (e.g. if we've
	     * got a grab set it can't respond).  If this happens then
	     * just quit.
	     */

	    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
		printf("WaitForMapNotify giving up on %s\n", winPtr->pathName);
	    }
	    break;
	}
    }
    wmPtr->flags &= ~WM_MOVE_PENDING;
    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("WaitForMapNotify finished with %s (winPtr %p, wmPtr %p)\n",
		winPtr->pathName, winPtr, wmPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdateHints --
 *
 *	This procedure is called to update the window manager's
 *	hints information from the information in a WmInfo
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
UpdateHints(winPtr)
    TkWindow *winPtr;
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	return;
    }
    XSetWMHints(winPtr->display, wmPtr->wrapperPtr->window, &wmPtr->hints);
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
ParseGeometry(interp, string, winPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    char *string;		/* String containing new geometry.  Has the
				 * standard form "=wxh+x+y". */
    TkWindow *winPtr;		/* Pointer to top-level window whose
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
	p++;
	if (!isdigit(UCHAR(*p)) && (*p != '-')) {
	    goto error;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p == '-') {
	    flags |= WM_NEGATIVE_Y;
	} else if (*p != '+') {
	    goto error;
	}
	p++;
	if (!isdigit(UCHAR(*p)) && (*p != '-')) {
	    goto error;
	}
	y = strtol(p, &end, 10);
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
    wmPtr->x = x;
    wmPtr->y = y;
    flags |= WM_MOVE_PENDING;
    wmPtr->flags = flags;

    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
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
Tk_GetRootCoords(tkwin, xPtr, yPtr)
    Tk_Window tkwin;		/* Token for window. */
    int *xPtr;			/* Where to store x-displacement of (0,0). */
    int *yPtr;			/* Where to store y-displacement of (0,0). */
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
	if ((winPtr->wmInfoPtr != NULL)
		&& (winPtr->wmInfoPtr->menubar == (Tk_Window) winPtr)) {
	    /*
	     * This window is a special menubar; switch over to its
	     * associated toplevel, compensate for their differences in
	     * y coordinates, then continue with the toplevel (in case
	     * it's embedded).
	     */

	    y -= winPtr->wmInfoPtr->menuHeight;
	    winPtr = winPtr->wmInfoPtr->winPtr;
	    continue;
	}
	if (winPtr->flags & TK_TOP_LEVEL) {
	    TkWindow *otherPtr;

	    if (!(winPtr->flags & TK_EMBEDDED)) {
		break;
	    }
	    otherPtr = TkpGetOtherWindow(winPtr);
	    if (otherPtr == NULL) {
		/*
		 * The container window is not in the same application.
		 * Query the X server.
		 */

		Window root, dummyChild;
		int rootX, rootY;

		root = winPtr->wmInfoPtr->vRoot;
		if (root == None) {
		    root = RootWindowOfScreen(Tk_Screen((Tk_Window)winPtr));
		}
		XTranslateCoordinates(winPtr->display, winPtr->window,
		    root, 0, 0, &rootX, &rootY, &dummyChild);
		x += rootX;
		y += rootY;
		break;
	    } else {
		/*
		 * The container window is in the same application.
		 * Let's query its coordinates.
		 */

		winPtr = otherPtr;
		continue;
	    }
	}
	winPtr = winPtr->parentPtr;
	if (winPtr == NULL) {
	    break;
	}
    }
    *xPtr = x;
    *yPtr = y;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CoordsToWindow --
 *
 *	Given the (virtual) root coordinates of a point, this procedure
 *	returns the token for the top-most window covering that point,
 *	if there exists such a window in this application.
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
Tk_CoordsToWindow(rootX, rootY, tkwin)
    int rootX, rootY;		/* Coordinates of point in root window.  If
				 * a virtual-root window manager is in use,
				 * these coordinates refer to the virtual
				 * root, not the real root. */
    Tk_Window tkwin;		/* Token for any window in application;
				 * used to identify the display. */
{
    Window window, parent, child;
    int x, y, childX, childY, tmpx, tmpy, bd;
    WmInfo *wmPtr;
    TkWindow *winPtr, *childPtr, *nextPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    Tk_ErrorHandler handler = NULL;

    /*
     * Step 1: scan the list of toplevel windows to see if there is a
     * virtual root for the screen we're interested in.  If so, we have
     * to translate the coordinates from virtual root to root
     * coordinates.
     */

    parent = window = RootWindowOfScreen(Tk_Screen(tkwin));
    x = rootX;
    y = rootY;
    for (wmPtr = (WmInfo *) dispPtr->firstWmPtr; wmPtr != NULL; wmPtr = wmPtr->nextPtr) {
	if (Tk_Screen(wmPtr->winPtr) != Tk_Screen(tkwin)) {
	    continue;
	}
	if (wmPtr->vRoot == None) {
	    continue;
	}
	UpdateVRootGeometry(wmPtr);
	parent = wmPtr->vRoot;
	break;
    }

    /*
     * Step 2: work down through the window hierarchy starting at the
     * root. For each window, find the child that contains the given
     * point and then see if this child is either a wrapper for one of
     * our toplevel windows or a window manager decoration window for
     * one of our toplevels.  This approach handles several tricky
     * cases:
     *
     * 1. There may be a virtual root window between the root and one of
     *    our toplevels.
     * 2. If a toplevel is embedded, we may have to search through the
     *    windows of the container application(s) before getting to
     *    the toplevel.
     */

    handler = Tk_CreateErrorHandler(Tk_Display(tkwin), -1, -1, -1,
	    (Tk_ErrorProc *) NULL, (ClientData) NULL);
    while (1) {
	if (XTranslateCoordinates(Tk_Display(tkwin), parent, window,
		x, y, &childX, &childY, &child) == False) {
	    /*
	     * We can end up here when the window is in the middle of
	     * being deleted
	     */
	    Tk_DeleteErrorHandler(handler);
	    return NULL;
	}
	if (child == None) {
	    Tk_DeleteErrorHandler(handler);
	    return NULL;
	}
	for (wmPtr = (WmInfo *) dispPtr->firstWmPtr; wmPtr != NULL;
                wmPtr = wmPtr->nextPtr) {
	    if (wmPtr->reparent == child) {
		goto gotToplevel;
	    }
	    if (wmPtr->wrapperPtr != NULL) {
		if (child == wmPtr->wrapperPtr->window) {
		    goto gotToplevel;
		}
	    } else if (child == wmPtr->winPtr->window) {
		goto gotToplevel;
	    }
	}
	x = childX;
	y = childY;
	parent = window;
	window = child;
    }

    gotToplevel:
    if (handler) {
	/*
	 * Check value of handler, because we can reach this label
	 * from above or below
	 */
	Tk_DeleteErrorHandler(handler);
	handler = NULL;
    }
    winPtr = wmPtr->winPtr;
    if (winPtr->mainPtr != ((TkWindow *) tkwin)->mainPtr) {
	return NULL;
    }

    /*
     * Step 3: at this point winPtr and wmPtr refer to the toplevel that
     * contains the given coordinates, and childX and childY give the
     * translated coordinates in the *parent* of the toplevel.  Now
     * decide whether the coordinates are in the menubar or the actual
     * toplevel, and translate the coordinates into the coordinate
     * system of that window.
     */

    x = childX - winPtr->changes.x;
    y = childY - winPtr->changes.y;
    if ((x < 0) || (x >= winPtr->changes.width)
	    || (y >= winPtr->changes.height)) {
	return NULL;
    }
    if (y < 0) {
	winPtr = (TkWindow *) wmPtr->menubar;
	if (winPtr == NULL) {
	    return NULL;
	}
	y += wmPtr->menuHeight;
	if (y < 0) {
	    return NULL;
	}
    }

    /*
     * Step 4: work down through the hierarchy underneath the current
     * window. At each level, scan through all the children to find the
     * highest one in the stacking order that contains the point.  Then
     * repeat the whole process on that child.
     */

    while (1) {
	nextPtr = NULL;
	for (childPtr = winPtr->childList; childPtr != NULL;
		childPtr = childPtr->nextPtr) {
	    if (!Tk_IsMapped(childPtr) || (childPtr->flags & TK_TOP_HIERARCHY)) {
		continue;
	    }
	    if (childPtr->flags & TK_REPARENTED) {
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
	if (nextPtr == NULL) {
	    break;
	}
	winPtr = nextPtr;
	x -= winPtr->changes.x;
	y -= winPtr->changes.y;
	if ((winPtr->flags & TK_CONTAINER)
		&& (winPtr->flags & TK_BOTH_HALVES)) {
	    /*
	     * The window containing the point is a container, and the
	     * embedded application is in this same process.  Switch
	     * over to the toplevel for the embedded application and
	     * start processing that toplevel from scratch.
	     */

	    winPtr = TkpGetOtherWindow(winPtr);
	    wmPtr = winPtr->wmInfoPtr;
	    childX = x;
	    childY = y;
	    goto gotToplevel;
	}
    }
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
UpdateVRootGeometry(wmPtr)
    WmInfo *wmPtr;		/* Window manager information to be
				 * updated.  The wmPtr->vRoot field must
				 * be valid. */
{
    TkWindow *winPtr = wmPtr->winPtr;
    int bd;
    unsigned int dummy;
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
	    (unsigned int *) &wmPtr->vRootWidth,
	    (unsigned int *) &wmPtr->vRootHeight, (unsigned int *) &bd,
	    &dummy);
    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
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
Tk_GetVRootGeometry(tkwin, xPtr, yPtr, widthPtr, heightPtr)
    Tk_Window tkwin;		/* Window whose virtual root is to be
				 * queried. */
    int *xPtr, *yPtr;		/* Store x and y offsets of virtual root
				 * here. */
    int *widthPtr, *heightPtr;	/* Store dimensions of virtual root here. */
{
    WmInfo *wmPtr;
    TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * Find the top-level window for tkwin, and locate the window manager
     * information for that window.
     */

    while (!(winPtr->flags & TK_TOP_HIERARCHY) && (winPtr->parentPtr != NULL)) {
	winPtr = winPtr->parentPtr;
    }
    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	/* Punt. */
	*xPtr = 0;
	*yPtr = 0;
	*widthPtr = 0;
	*heightPtr = 0;
    }


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
Tk_MoveToplevelWindow(tkwin, x, y)
    Tk_Window tkwin;		/* Window to move. */
    int x, y;			/* New location for window (within
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
	    Tcl_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
	}
	UpdateGeometryInfo((ClientData) winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateWmProtocols --
 *
 *	This procedure transfers the most up-to-date information about
 *	window manager protocols from the WmInfo structure to the actual
 *	property on the top-level window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The WM_PROTOCOLS property gets changed for wmPtr's window.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateWmProtocols(wmPtr)
    register WmInfo *wmPtr;	/* Information about top-level window. */
{
    register ProtocolHandler *protPtr;
    Atom deleteWindowAtom;
    int count;
    Atom *arrayPtr, *atomPtr;

    /*
     * There are only two tricky parts here.  First, there could be any
     * number of atoms for the window, so count them and malloc an array
     * to hold all of their atoms.  Second, we *always* want to respond
     * to the WM_DELETE_WINDOW protocol, even if no-one's officially asked.
     */

    for (protPtr = wmPtr->protPtr, count = 1; protPtr != NULL;
	    protPtr = protPtr->nextPtr, count++) {
	/* Empty loop body;  we're just counting the handlers. */
    }
    arrayPtr = (Atom *) ckalloc((unsigned) (count * sizeof(Atom)));
    deleteWindowAtom = Tk_InternAtom((Tk_Window) wmPtr->winPtr,
	    "WM_DELETE_WINDOW");
    arrayPtr[0] = deleteWindowAtom;
    for (protPtr = wmPtr->protPtr, atomPtr = &arrayPtr[1];
	    protPtr != NULL; protPtr = protPtr->nextPtr) {
	if (protPtr->protocol != deleteWindowAtom) {
	    *atomPtr = protPtr->protocol;
	    atomPtr++;
	}
    }
    XChangeProperty(wmPtr->winPtr->display, wmPtr->wrapperPtr->window,
	    Tk_InternAtom((Tk_Window) wmPtr->winPtr, "WM_PROTOCOLS"),
	    XA_ATOM, 32, PropModeReplace, (unsigned char *) arrayPtr,
	    atomPtr-arrayPtr);
    ckfree((char *) arrayPtr);
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
TkWmProtocolEventProc(winPtr, eventPtr)
    TkWindow *winPtr;		/* Window to which the event was sent. */
    XEvent *eventPtr;		/* X event. */
{
    WmInfo *wmPtr;
    register ProtocolHandler *protPtr;
    Atom protocol;
    int result;
    CONST char *protocolName;
    Tcl_Interp *interp;

    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }
    protocol = (Atom) eventPtr->xclient.data.l[0];

    /*
     * Note: it's very important to retrieve the protocol name now,
     * before invoking the command, even though the name won't be used
     * until after the command returns.  This is because the command
     * could delete winPtr, making it impossible for us to use it
     * later in the call to Tk_GetAtomName.
     */

    protocolName = Tk_GetAtomName((Tk_Window) winPtr, protocol);
    for (protPtr = wmPtr->protPtr; protPtr != NULL;
	    protPtr = protPtr->nextPtr) {
	if (protocol == protPtr->protocol) {
	    Tcl_Preserve((ClientData) protPtr);
            interp = protPtr->interp;
            Tcl_Preserve((ClientData) interp);
	    result = Tcl_GlobalEval(interp, protPtr->command);
	    if (result != TCL_OK) {
		Tcl_AddErrorInfo(interp, "\n    (command for \"");
		Tcl_AddErrorInfo(interp, protocolName);
		Tcl_AddErrorInfo(interp,
			"\" window manager protocol)");
		Tcl_BackgroundError(interp);
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
	Tk_DestroyWindow((Tk_Window) wmPtr->winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmStackorderToplevelWrapperMap --
 *
 *	This procedure will create a table that maps the reparent wrapper
 *	X id for a toplevel to the TkWindow structure that is wraps.
 *	Tk keeps track of a mapping from the window X id to the TkWindow
 *	structure but that does us no good here since we only get the X
 *	id of the wrapper window. Only those toplevel windows that are
 *	mapped have a position in the stacking order.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds entries to the passed hashtable.
 *
 *----------------------------------------------------------------------
 */

static void
TkWmStackorderToplevelWrapperMap(winPtr, table)
    TkWindow *winPtr;				/* TkWindow to recurse on */
    Tcl_HashTable *table;			/* Maps X id to TkWindow */
{
    TkWindow *childPtr;
    Tcl_HashEntry *hPtr;
    Window wrapper;
    int newEntry;

    if (Tk_IsMapped(winPtr) && Tk_IsTopLevel(winPtr) &&
            !Tk_IsEmbedded(winPtr)) {
        wrapper = (winPtr->wmInfoPtr->reparent != None)
            ? winPtr->wmInfoPtr->reparent
            : winPtr->wmInfoPtr->wrapperPtr->window;

        hPtr = Tcl_CreateHashEntry(table,
            (char *) wrapper, &newEntry);
        Tcl_SetHashValue(hPtr, winPtr);
    }

    for (childPtr = winPtr->childList; childPtr != NULL;
            childPtr = childPtr->nextPtr) {
        TkWmStackorderToplevelWrapperMap(childPtr, table);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmStackorderToplevel --
 *
 *	This procedure returns the stack order of toplevel windows.
 *
 * Results:
 *	An array of pointers to tk window objects in stacking order
 *	or else NULL if there was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow **
TkWmStackorderToplevel(parentPtr)
    TkWindow *parentPtr;		/* Parent toplevel window. */
{
    Window dummy1, dummy2, vRoot;
    Window *children;
    unsigned int numChildren, i;
    TkWindow *childWinPtr, **windows, **window_ptr;
    Tcl_HashTable table;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    /*
     * Map X Window ids to a TkWindow of the wrapped toplevel.
     */

    Tcl_InitHashTable(&table, TCL_ONE_WORD_KEYS);
    TkWmStackorderToplevelWrapperMap(parentPtr, &table);

    window_ptr = windows = (TkWindow **) ckalloc((table.numEntries+1)
        * sizeof(TkWindow *));

    /*
     * Special cases: If zero or one toplevels were mapped
     * there is no need to call XQueryTree.
     */

    switch (table.numEntries) {
    case 0:
        windows[0] = NULL;
        goto done;
    case 1:
        hPtr = Tcl_FirstHashEntry(&table, &search);
        windows[0] = (TkWindow *) Tcl_GetHashValue(hPtr);
        windows[1] = NULL;
        goto done;
    }

    vRoot = parentPtr->wmInfoPtr->vRoot;
    if (vRoot == None) {
        vRoot = RootWindowOfScreen(Tk_Screen((Tk_Window) parentPtr));
    }

    if (XQueryTree(parentPtr->display, vRoot, &dummy1, &dummy2,
            &children, &numChildren) == 0) {
        ckfree((char *) windows);
        windows = NULL;
    } else {
        for (i = 0; i < numChildren; i++) {
            hPtr = Tcl_FindHashEntry(&table, (char *) children[i]);
            if (hPtr != NULL) {
                childWinPtr = (TkWindow *) Tcl_GetHashValue(hPtr);
                *window_ptr++ = childWinPtr;
            }
        }
        if ((window_ptr - windows) != table.numEntries)
            panic("num matched toplevel windows does not equal num children");
        *window_ptr = NULL;
	if (numChildren) {
	    XFree((char *) children);
	}
    }

    done:
    Tcl_DeleteHashTable(&table);
    return windows;
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
 *
 *----------------------------------------------------------------------
 */

void
TkWmRestackToplevel(winPtr, aboveBelow, otherPtr)
    TkWindow *winPtr;		/* Window to restack. */
    int aboveBelow;		/* Gives relative position for restacking;
				 * must be Above or Below. */
    TkWindow *otherPtr;		/* Window relative to which to restack;
				 * if NULL, then winPtr gets restacked
				 * above or below *all* siblings. */
{
    XWindowChanges changes;
    unsigned int mask;
    TkWindow *wrapperPtr;

    memset(&changes, 0, sizeof(XWindowChanges));
    changes.stack_mode = aboveBelow;
    mask = CWStackMode;

    /*
     * Make sure that winPtr and its wrapper window have been created.
     */
    if (winPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	TkWmMapWindow(winPtr);
    }
    wrapperPtr = winPtr->wmInfoPtr->wrapperPtr;

    if (otherPtr != NULL) {
	/*
	 * The window is to be restacked with respect to another toplevel.
	 * Make sure it has been created as well.
	 */
	if (otherPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	    TkWmMapWindow(otherPtr);
	}
	changes.sibling = otherPtr->wmInfoPtr->wrapperPtr->window;
	mask |= CWSibling;
    }

    /*
     * Reconfigure the window.  Note that we use XReconfigureWMWindow
     * instead of XConfigureWindow, in order to handle the case
     * where the window is to be restacked with respect to another toplevel.  
     * See [ICCCM] 4.1.5 "Configuring the Window" and XReconfigureWMWindow(3)
     * for details.
     */

    XReconfigureWMWindow(winPtr->display, wrapperPtr->window,
	    Tk_ScreenNumber((Tk_Window) winPtr), mask,  &changes);
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
TkWmAddToColormapWindows(winPtr)
    TkWindow *winPtr;		/* Window with a non-default colormap.
				 * Should not be a top-level window. */
{
    TkWindow *wrapperPtr;
    TkWindow *topPtr;
    Window *oldPtr, *newPtr;
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
	if (topPtr->flags & TK_TOP_HIERARCHY) {
	    break;
	}
    }
    if (topPtr->wmInfoPtr == NULL) {
	return;
    }

    if (topPtr->wmInfoPtr->flags & WM_COLORMAPS_EXPLICIT) {
	return;
    }
    if (topPtr->wmInfoPtr->wrapperPtr == NULL) {
	CreateWrapper(topPtr->wmInfoPtr);
    }
    wrapperPtr = topPtr->wmInfoPtr->wrapperPtr;

    /*
     * Fetch the old value of the property.
     */

    if (XGetWMColormapWindows(topPtr->display, wrapperPtr->window,
	    &oldPtr, &count) == 0) {
	oldPtr = NULL;
	count = 0;
    }

    /*
     * Make sure that the window isn't already in the list.
     */

    for (i = 0; i < count; i++) {
	if (oldPtr[i] == winPtr->window) {
	    return;
	}
    }

    /*
     * Make a new bigger array and use it to reset the property.
     * Automatically add the toplevel itself as the last element
     * of the list.
     */

    newPtr = (Window *) ckalloc((unsigned) ((count+2)*sizeof(Window)));
    for (i = 0; i < count; i++) {
	newPtr[i] = oldPtr[i];
    }
    if (count == 0) {
	count++;
    }
    newPtr[count-1] = winPtr->window;
    newPtr[count] = topPtr->window;
    XSetWMColormapWindows(topPtr->display, wrapperPtr->window, newPtr,
	    count+1);
    ckfree((char *) newPtr);
    if (oldPtr != NULL) {
	XFree((char *) oldPtr);
    }
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
TkWmRemoveFromColormapWindows(winPtr)
    TkWindow *winPtr;		/* Window that may be present in
				 * WM_COLORMAP_WINDOWS property for its
				 * top-level.  Should not be a top-level
				 * window. */
{
    TkWindow *wrapperPtr;
    TkWindow *topPtr;
    Window *oldPtr;
    int count, i, j;

    if (winPtr->window == None) {
	return;
    }

    for (topPtr = winPtr->parentPtr; ; topPtr = topPtr->parentPtr) {
	if (topPtr == NULL) {
	    /*
	     * Ancestors have been deleted, so skip the whole operation.
	     * Seems like this can't ever happen?
	     */

	    return;
	}
	if (topPtr->flags & TK_TOP_HIERARCHY) {
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
    if (topPtr->wmInfoPtr == NULL) {
	return;
    }

    if (topPtr->wmInfoPtr->wrapperPtr == NULL) {
	CreateWrapper(topPtr->wmInfoPtr);
    }
    wrapperPtr = topPtr->wmInfoPtr->wrapperPtr;
    if (wrapperPtr == NULL) {
	return;
    }

    /*
     * Fetch the old value of the property.
     */

    if (XGetWMColormapWindows(topPtr->display, wrapperPtr->window,
	    &oldPtr, &count) == 0) {
	return;
    }

    /*
     * Find the window and slide the following ones down to cover
     * it up.
     */

    for (i = 0; i < count; i++) {
	if (oldPtr[i] == winPtr->window) {
	    for (j = i ; j < count-1; j++) {
		oldPtr[j] = oldPtr[j+1];
	    }
	    XSetWMColormapWindows(topPtr->display, wrapperPtr->window,
		    oldPtr, count-1);
	    break;
	}
    }
    XFree((char *) oldPtr);
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
TkGetPointerCoords(tkwin, xPtr, yPtr)
    Tk_Window tkwin;		/* Toplevel window that identifies screen
				 * on which lookup is to be done. */
    int *xPtr, *yPtr;		/* Store pointer coordinates here. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr;
    Window w, root, child;
    int rootX, rootY;
    unsigned int mask;

    wmPtr = winPtr->wmInfoPtr;

    w = wmPtr->vRoot;
    if (w == None) {
	w = RootWindow(winPtr->display, winPtr->screenNum);
    }
    if (XQueryPointer(winPtr->display, w, &root, &child, &rootX, &rootY,
	    xPtr, yPtr, &mask) != True) {
	*xPtr = -1;
	*yPtr = -1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMaxSize --
 *
 *	This procedure computes the current maxWidth and maxHeight
 *	values for a window, taking into account the possibility
 *	that they may be defaulted.
 *
 * Results:
 *	The values at *maxWidthPtr and *maxHeightPtr are filled
 *	in with the maximum allowable dimensions of wmPtr's window,
 *	in grid units.  If no maximum has been specified for the
 *	window, then this procedure computes the largest sizes that
 *	will fit on the screen.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMaxSize(wmPtr, maxWidthPtr, maxHeightPtr)
    WmInfo *wmPtr;		/* Window manager information for the
				 * window. */
    int *maxWidthPtr;		/* Where to store the current maximum
				 * width of the window. */
    int *maxHeightPtr;		/* Where to store the current maximum
				 * height of the window. */
{
    int tmp;

    if (wmPtr->maxWidth > 0) {
	*maxWidthPtr = wmPtr->maxWidth;
    } else {
	/*
	 * Must compute a default width.  Fill up the display, leaving a
	 * bit of extra space for the window manager's borders.
	 */

	tmp = DisplayWidth(wmPtr->winPtr->display, wmPtr->winPtr->screenNum)
	    - 15;
	if (wmPtr->gridWin != NULL) {
	    /*
	     * Gridding is turned on;  convert from pixels to grid units.
	     */

	    tmp = wmPtr->reqGridWidth
		    + (tmp - wmPtr->winPtr->reqWidth)/wmPtr->widthInc;
	}
	*maxWidthPtr = tmp;
    }
    if (wmPtr->maxHeight > 0) {
	*maxHeightPtr = wmPtr->maxHeight;
    } else {
	tmp = DisplayHeight(wmPtr->winPtr->display, wmPtr->winPtr->screenNum)
	    - 30;
	if (wmPtr->gridWin != NULL) {
	    tmp = wmPtr->reqGridHeight
		    + (tmp - wmPtr->winPtr->reqHeight)/wmPtr->heightInc;
	}
	*maxHeightPtr = tmp;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeMenuWindow --
 *
 *	Configure the window to be either a pull-down (or pop-up)
 *	menu, or as a toplevel (torn-off) menu or palette.
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
TkpMakeMenuWindow(tkwin, transient)
    Tk_Window tkwin;		/* New window. */
    int transient;		/* 1 means menu is only posted briefly as
				 * a popup or pulldown or cascade.  0 means
				 * menu is always visible, e.g. as a torn-off
				 * menu.  Determines whether save_under and
				 * override_redirect should be set. */
{
    WmInfo *wmPtr;
    XSetWindowAttributes atts;
    TkWindow *wrapperPtr;

    if (!Tk_HasWrapper(tkwin)) {
	return;
    }
    wmPtr = ((TkWindow *) tkwin)->wmInfoPtr;
    if (wmPtr->wrapperPtr == NULL) {
	CreateWrapper(wmPtr);
    }
    wrapperPtr = wmPtr->wrapperPtr;
    if (transient) {
	atts.override_redirect = True;
	atts.save_under = True;
    } else {
	atts.override_redirect = False;
	atts.save_under = False;
    }

    /*
     * The override-redirect and save-under bits must be set on the
     * wrapper window in order to have the desired effect.  However,
     * also set the override-redirect bit on the window itself, so
     * that the "wm overrideredirect" command will see it.
     */

    if ((atts.override_redirect != Tk_Attributes(wrapperPtr)->override_redirect)
	    || (atts.save_under != Tk_Attributes(wrapperPtr)->save_under)) {
	Tk_ChangeWindowAttributes((Tk_Window) wrapperPtr,
		CWOverrideRedirect|CWSaveUnder, &atts);
    }
    if (atts.override_redirect != Tk_Attributes(tkwin)->override_redirect) {
	Tk_ChangeWindowAttributes(tkwin, CWOverrideRedirect, &atts);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateWrapper --
 *
 *	This procedure is invoked to create the wrapper window for a
 *	toplevel window.  It is called just before a toplevel is mapped
 *	for the first time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The wrapper is created and the toplevel is reparented inside it.
 *
 *----------------------------------------------------------------------
 */

static void
CreateWrapper(wmPtr)
    WmInfo *wmPtr;		/* Window manager information for the
				 * window. */
{
    TkWindow *winPtr, *wrapperPtr;
    Window parent;
    Tcl_HashEntry *hPtr;
    int new;

    winPtr = wmPtr->winPtr;
    if (winPtr->window == None) {
	Tk_MakeWindowExist((Tk_Window) winPtr);
    }

    /*
     * The code below is copied from CreateTopLevelWindow,
     * Tk_MakeWindowExist, and TkpMakeWindow; The idea is to create an
     * "official" Tk window (so that we can get events on it), but to
     * hide the window outside the official Tk hierarchy so that it
     * isn't visible to the application.  See the comments for the other
     * procedures if you have questions about this code.
     */

    wmPtr->wrapperPtr = wrapperPtr = TkAllocWindow(winPtr->dispPtr,
	    Tk_ScreenNumber((Tk_Window) winPtr), winPtr);
    wrapperPtr->dirtyAtts |= CWBorderPixel;

    /*
     * Tk doesn't normally select for StructureNotifyMask events because
     * the events are synthesized internally.  However, for wrapper
     * windows we need to know when the window manager modifies the
     * window configuration.  We also need to select on focus change
     * events; these are the only windows for which we care about focus
     * changes.
     */

    wrapperPtr->flags |= TK_WRAPPER;
    wrapperPtr->atts.event_mask |= StructureNotifyMask|FocusChangeMask;
    wrapperPtr->atts.override_redirect = winPtr->atts.override_redirect;
    if (winPtr->flags & TK_EMBEDDED) {
	parent = TkUnixContainerId(winPtr);
    } else {
	parent = XRootWindow(wrapperPtr->display, wrapperPtr->screenNum);
    }
    wrapperPtr->window = XCreateWindow(wrapperPtr->display,
	    parent, wrapperPtr->changes.x, wrapperPtr->changes.y,
	    (unsigned) wrapperPtr->changes.width,
	    (unsigned) wrapperPtr->changes.height,
	    (unsigned) wrapperPtr->changes.border_width, wrapperPtr->depth,
	    InputOutput, wrapperPtr->visual,
	    wrapperPtr->dirtyAtts|CWOverrideRedirect, &wrapperPtr->atts);
    hPtr = Tcl_CreateHashEntry(&wrapperPtr->dispPtr->winTable,
	    (char *) wrapperPtr->window, &new);
    Tcl_SetHashValue(hPtr, wrapperPtr);
    wrapperPtr->mainPtr = winPtr->mainPtr;
    wrapperPtr->mainPtr->refCount++;
    wrapperPtr->dirtyAtts = 0;
    wrapperPtr->dirtyChanges = 0;
    wrapperPtr->wmInfoPtr = wmPtr;

    /*
     * Reparent the toplevel window inside the wrapper.
     */

    XReparentWindow(wrapperPtr->display, winPtr->window, wrapperPtr->window,
	    0, 0);

    /*
     * Tk must monitor structure events for wrapper windows in order
     * to detect changes made by window managers such as resizing,
     * mapping, unmapping, etc..
     */

    Tk_CreateEventHandler((Tk_Window) wmPtr->wrapperPtr, StructureNotifyMask,
	    WrapperEventProc, (ClientData) wmPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmFocusToplevel --
 *
 *	This is a utility procedure invoked by focus-management code.
 *	The focus code responds to externally generated focus-related
 *	events on wrapper windows but ignores those events for any other
 *	windows.  This procedure determines whether a given window is a
 *	wrapper window and, if so, returns the toplevel window
 *	corresponding to the wrapper.
 *
 * Results:
 *	If winPtr is a wrapper window, returns a pointer to the
 *	corresponding toplevel window; otherwise returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkWmFocusToplevel(winPtr)
    TkWindow *winPtr;		/* Window that received a focus-related
				 * event. */
{
    if (!(winPtr->flags & TK_WRAPPER)) {
	return NULL;
    }
    return winPtr->wmInfoPtr->winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnixSetMenubar --
 *
 *	This procedure is invoked by menu management code to specify the
 *	window to use as a menubar for a given toplevel window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window given by menubar will be mapped and positioned inside
 *	the wrapper for tkwin and above tkwin.  Menubar will
 *	automatically be resized to maintain the height specified by
 *	TkUnixSetMenuHeight the same width as tkwin.  Any previous
 *	menubar specified for tkwin will be unmapped and ignored from
 *	now on.
 *
 *----------------------------------------------------------------------
 */

void
TkUnixSetMenubar(tkwin, menubar)
    Tk_Window tkwin;		/* Token for toplevel window. */
    Tk_Window menubar;		/* Token for window that is to serve as
				 * menubar for tkwin.  Must not be a
				 * toplevel window.  If NULL, any
				 * existing menubar is canceled and the
				 * menu height is reset to 0. */
{
    WmInfo *wmPtr = ((TkWindow *) tkwin)->wmInfoPtr;
    Tk_Window parent;
    TkWindow *menubarPtr = (TkWindow *) menubar;

    if (wmPtr->menubar != NULL) {
	/*
	 * There's already a menubar for this toplevel.  If it isn't the
	 * same as the new menubar, unmap it so that it is out of the
	 * way, and reparent it back to its original parent.
	 */

	if (wmPtr->menubar == menubar) {
	    return;
	}
	((TkWindow *) wmPtr->menubar)->wmInfoPtr = NULL;
	((TkWindow *) wmPtr->menubar)->flags &= ~TK_REPARENTED;
	Tk_UnmapWindow(wmPtr->menubar);
	parent = Tk_Parent(wmPtr->menubar);
	if (parent != NULL) {
	    Tk_MakeWindowExist(parent);
	    XReparentWindow(Tk_Display(wmPtr->menubar),
		    Tk_WindowId(wmPtr->menubar), Tk_WindowId(parent), 0, 0);
	}
	Tk_DeleteEventHandler(wmPtr->menubar, StructureNotifyMask,
		MenubarDestroyProc, (ClientData) wmPtr->menubar);
	Tk_ManageGeometry(wmPtr->menubar, NULL, (ClientData) NULL);
    }

    wmPtr->menubar = menubar;
    if (menubar == NULL) {
	wmPtr->menuHeight = 0;
    } else {
	if ((menubarPtr->flags & TK_TOP_LEVEL)
	     || (Tk_Screen(menubar) != Tk_Screen(tkwin))) {
	    panic("TkUnixSetMenubar got bad menubar");
	}
	wmPtr->menuHeight = Tk_ReqHeight(menubar);
	if (wmPtr->menuHeight == 0) {
	    wmPtr->menuHeight = 1;
	}
	Tk_MakeWindowExist(tkwin);
	Tk_MakeWindowExist(menubar);
	if (wmPtr->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr);
	}
	XReparentWindow(Tk_Display(menubar), Tk_WindowId(menubar),
		wmPtr->wrapperPtr->window, 0, 0);
	menubarPtr->wmInfoPtr = wmPtr;
	Tk_MoveResizeWindow(menubar, 0, 0, Tk_Width(tkwin), wmPtr->menuHeight);
	Tk_MapWindow(menubar);
	Tk_CreateEventHandler(menubar, StructureNotifyMask, MenubarDestroyProc,
		(ClientData) menubar);
	Tk_ManageGeometry(menubar, &menubarMgrType, (ClientData) wmPtr);
	menubarPtr->flags |= TK_REPARENTED;
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) tkwin);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenubarDestroyProc --
 *
 *	This procedure is invoked by the event dispatcher whenever a
 *	menubar window is destroyed (it's also invoked for a few other
 *	kinds of events, but we ignore those).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The association between the window and its toplevel is broken,
 *	so that the window is no longer considered to be a menubar.
 *
 *----------------------------------------------------------------------
 */

static void
MenubarDestroyProc(clientData, eventPtr)
    ClientData clientData;		/* TkWindow pointer for menubar. */
    XEvent *eventPtr;			/* Describes what just happened. */
{
    WmInfo *wmPtr;

    if (eventPtr->type != DestroyNotify) {
	return;
    }
    wmPtr = ((TkWindow *) clientData)->wmInfoPtr;
    wmPtr->menubar = NULL;
    wmPtr->menuHeight = 0;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) wmPtr->winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenubarReqProc --
 *
 *	This procedure is invoked by the Tk geometry management code
 *	whenever a menubar calls Tk_GeometryRequest to request a new
 *	size.
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
MenubarReqProc(clientData, tkwin)
    ClientData clientData;	/* Pointer to the window manager
				 * information for tkwin's toplevel. */
    Tk_Window tkwin;		/* Handle for menubar window. */
{
    WmInfo *wmPtr = (WmInfo *) clientData;

    wmPtr->menuHeight = Tk_ReqHeight(tkwin);
    if (wmPtr->menuHeight <= 0) {
	wmPtr->menuHeight = 1;
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) wmPtr->winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetWrapperWindow --
 *
 *	Given a toplevel window return the hidden wrapper window for
 *	the toplevel window if available.
 *
 * Results:
 *	The wrapper window.  NULL is we were not passed a toplevel
 *	window or the wrapper has yet to be created.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkpGetWrapperWindow(winPtr)
    TkWindow *winPtr;		/* A toplevel window pointer. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if ((winPtr == NULL) || (wmPtr == NULL)) {
	return NULL;
    }

    return wmPtr->wrapperPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateCommand --
 *
 *	Update the WM_COMMAND property, taking care to translate
 *	the command strings into the external encoding.
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
UpdateCommand(winPtr)
    TkWindow  *winPtr;
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tcl_DString cmds, ds;
    int i, *offsets;
    char **cmdArgv;

    /*
     * Translate the argv strings into the external encoding.  To avoid
     * allocating lots of memory, the strings are appended to a buffer
     * with nulls between each string.
     *
     * This code is tricky because we need to pass and array of pointers
     * to XSetCommand.  However, we can't compute the pointers as we go
     * because the DString buffer space could get reallocated.  So, store
     * offsets for each element as we go, then compute pointers from the
     * offsets once the entire DString is done.
     */

    cmdArgv = (char **) ckalloc(sizeof(char *) * wmPtr->cmdArgc);
    offsets = (int *) ckalloc( sizeof(int) * wmPtr->cmdArgc);
    Tcl_DStringInit(&cmds);
    for (i = 0; i < wmPtr->cmdArgc; i++) {
	Tcl_UtfToExternalDString(NULL, wmPtr->cmdArgv[i], -1, &ds);
	offsets[i] = Tcl_DStringLength(&cmds);
	Tcl_DStringAppend(&cmds, Tcl_DStringValue(&ds),
		Tcl_DStringLength(&ds)+1);
	Tcl_DStringFree(&ds);
    }
    cmdArgv[0] = Tcl_DStringValue(&cmds);
    for (i = 1; i < wmPtr->cmdArgc; i++) {
	cmdArgv[i] = cmdArgv[0] + offsets[i];
    }

    XSetCommand(winPtr->display, wmPtr->wrapperPtr->window,
	    cmdArgv, wmPtr->cmdArgc);
    Tcl_DStringFree(&cmds);
    ckfree((char *) cmdArgv);
    ckfree((char *) offsets);
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
 *	0 on error, 1 otherwise
 *
 * Side effects:
 *	May minimize, restore, or withdraw a window.
 *
 *----------------------------------------------------------------------
 */

int
TkpWmSetState(winPtr, state)
     TkWindow *winPtr;		/* Toplevel window to operate on. */
     int state;			/* One of IconicState, NormalState,
				 * or WithdrawnState. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (state == WithdrawnState) {
        wmPtr->hints.initial_state = WithdrawnState;
        wmPtr->withdrawn = 1;
        if (wmPtr->flags & WM_NEVER_MAPPED) {
            return 1;
        }
        if (XWithdrawWindow(winPtr->display, wmPtr->wrapperPtr->window,
                winPtr->screenNum) == 0) {
            return 0;
        }
        WaitForMapNotify(winPtr, 0);
    } else if (state == NormalState) {
        wmPtr->hints.initial_state = NormalState;
        wmPtr->withdrawn = 0;
        if (wmPtr->flags & WM_NEVER_MAPPED) {
            return 1;
        }
        UpdateHints(winPtr);
        Tk_MapWindow((Tk_Window) winPtr);
    } else if (state == IconicState) {
        wmPtr->hints.initial_state = IconicState;
        if (wmPtr->flags & WM_NEVER_MAPPED) {
            return 1;
        }
        if (wmPtr->withdrawn) {
            UpdateHints(winPtr);
            Tk_MapWindow((Tk_Window) winPtr);
            wmPtr->withdrawn = 0;
        } else {
            if (XIconifyWindow(winPtr->display, wmPtr->wrapperPtr->window,
                    winPtr->screenNum) == 0) {
                return 0;
            }
            WaitForMapNotify(winPtr, 0);
        }
    }

    return 1;
}
