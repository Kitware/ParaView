/* 
 * tkWinWm.c --
 *
 *	This module takes care of the interactions between a Tk-based
 *	application and the window manager.  Among other things, it
 *	implements the "wm" command and passes geometry information
 *	to the window manager.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"

/*
 * Event structure for synthetic activation events.  These events are
 * placed on the event queue whenever a toplevel gets a WM_MOUSEACTIVATE
 * message.
 */

typedef struct ActivateEvent {
    Tcl_Event ev;
    TkWindow *winPtr;
} ActivateEvent;

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
    HWND wrapper;		/* This is the decorative frame window
				 * created by the window manager to wrap
				 * a toplevel window.  This window is
				 * a direct child of the root window. */
    Tk_Uid titleUid;		/* Title to display in window caption.  If
				 * NULL, use name of widget. */
    Tk_Uid iconName;		/* Name to display in icon. */
    TkWindow *masterPtr;	/* Master window for TRANSIENT_FOR property,
				 * or NULL. */
    XWMHints hints;		/* Various pieces of information for
				 * window manager. */
    char *leaderName;		/* Path name of leader of window group
				 * (corresponds to hints.window_group).
				 * Malloc-ed. Note:  this field doesn't
				 * get updated if leader is destroyed. */
    Tk_Window icon;		/* Window to use as icon for this window,
				 * or NULL. */
    Tk_Window iconFor;		/* Window for which this window is icon, or
				 * NULL if this isn't an icon for anyone. */

    /*
     * Information used to construct an XSizeHints structure for
     * the window manager:
     */

    int defMinWidth, defMinHeight, defMaxWidth, defMaxHeight;
				/* Default resize limits given by system. */
    int sizeHintsFlags;		/* Flags word for XSizeHints structure.
				 * If the PBaseSize flag is set then the
				 * window is gridded;  otherwise it isn't
				 * gridded. */
    int minWidth, minHeight;	/* Minimum dimensions of window, in
				 * grid units, not pixels. */
    int maxWidth, maxHeight;	/* Maximum dimensions of window, in
				 * grid units, not pixels, or 0 to default. */
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
    int borderWidth, borderHeight;
				/* Width and height of window dressing, in
				 * pixels for the current style/exStyle.  This
				 * includes the border on both sides of the
				 * window. */
    int configWidth, configHeight;
				/* Dimensions passed to last request that we
				 * issued to change geometry of window.  Used
				 * to eliminate redundant resize operations. */
    HMENU hMenu;		/* the hMenu associated with this menu */
    DWORD style, exStyle;	/* Style flags for the wrapper window. */

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
 * WM_MOVE_PENDING -		non-zero means the application has requested
 *				a new position for the window, but it hasn't
 *				been reflected through the window manager
 *				yet.
 * WM_COLORAMPS_EXPLICIT -	non-zero means the colormap windows were
 *				set explicitly via "wm colormapwindows".
 * WM_ADDED_TOPLEVEL_COLORMAP - non-zero means that when "wm colormapwindows"
 *				was called the top-level itself wasn't
 *				specified, so we added it implicitly at
 *				the end of the list.
 */

#define WM_NEVER_MAPPED			(1<<0)
#define WM_UPDATE_PENDING		(1<<1)
#define WM_NEGATIVE_X			(1<<2)
#define WM_NEGATIVE_Y			(1<<3)
#define WM_UPDATE_SIZE_HINTS		(1<<4)
#define WM_SYNC_PENDING			(1<<5)
#define WM_CREATE_PENDING		(1<<6)
#define WM_MOVE_PENDING			(1<<7)
#define WM_COLORMAPS_EXPLICIT		(1<<8)
#define WM_ADDED_TOPLEVEL_COLORMAP	(1<<9)
#define WM_WIDTH_NOT_RESIZABLE		(1<<10)
#define WM_HEIGHT_NOT_RESIZABLE		(1<<11)

/*
 * Window styles for various types of toplevel windows.
 */

#define WM_OVERRIDE_STYLE (WS_POPUP|WS_CLIPCHILDREN|CS_DBLCLKS)
#define EX_OVERRIDE_STYLE (WS_EX_TOOLWINDOW)

#define WM_TOPLEVEL_STYLE (WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|CS_DBLCLKS)
#define EX_TOPLEVEL_STYLE (0)

#define WM_TRANSIENT_STYLE \
		(WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPSIBLINGS|CS_DBLCLKS)
#define EX_TRANSIENT_STYLE \
		(WS_EX_TOOLWINDOW|WS_EX_DLGMODALFRAME)

/*
 * The following structure is the official type record for geometry
 * management of top-level windows.
 */

static void		TopLevelReqProc(ClientData dummy, Tk_Window tkwin);

static Tk_GeomMgr wmMgrType = {
    "wm",				/* name */
    TopLevelReqProc,			/* requestProc */
    (Tk_GeomLostSlaveProc *) NULL,	/* lostSlaveProc */
};

typedef struct ThreadSpecificData {
    HPALETTE systemPalette;      /* System palette; refers to the 
				  * currently installed foreground logical
				  * palette. */
    TkWindow *createWindow;      /* Window that is being constructed.  This
				  * value is set immediately before a
				  * call to CreateWindowEx, and is used
				  * by SetLimits.  This is a gross hack
				  * needed to work around Windows brain
				  * damage where it sends the
				  * WM_GETMINMAXINFO message before the
				  * WM_CREATE window. */
    int initialized;             /* Flag indicating whether thread-
				  * specific elements of module have 
				  * been initialized. */
    int firstWindow;             /* Flag, cleared when the first window
				  * is mapped in a non-iconic state. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The following variables cannot be placed in thread local storage
 * because they must be shared across threads.
 */

static WNDCLASS toplevelClass; /* Class for toplevel windows. */
static int initialized;        /* Flag indicating whether module has
				* been initialized. */
TCL_DECLARE_MUTEX(winWmMutex)


/*
 * Forward declarations for procedures defined in this file:
 */

static int		ActivateWindow _ANSI_ARGS_((Tcl_Event *evPtr,
			    int flags));
static void		ConfigureEvent _ANSI_ARGS_((TkWindow *winPtr,
			    XConfigureEvent *eventPtr));
static void		ConfigureTopLevel _ANSI_ARGS_((WINDOWPOS *pos));
static void		GenerateConfigureNotify _ANSI_ARGS_((
			    TkWindow *winPtr));
static void		GetMaxSize _ANSI_ARGS_((WmInfo *wmPtr,
			    int *maxWidthPtr, int *maxHeightPtr));
static void		GetMinSize _ANSI_ARGS_((WmInfo *wmPtr,
			    int *minWidthPtr, int *minHeightPtr));
static TkWindow *	GetTopLevel _ANSI_ARGS_((HWND hwnd));
static void		InitWm _ANSI_ARGS_((void));
static int		InstallColormaps _ANSI_ARGS_((HWND hwnd, int message,
			    int isForemost));
static void		InvalidateSubTree _ANSI_ARGS_((TkWindow *winPtr,
			    Colormap colormap));
static int		ParseGeometry _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, TkWindow *winPtr));
static void		RefreshColormap _ANSI_ARGS_((Colormap colormap,
	                    TkDisplay *dispPtr));
static void		SetLimits _ANSI_ARGS_((HWND hwnd, MINMAXINFO *info));
static LRESULT CALLBACK	TopLevelProc _ANSI_ARGS_((HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam));
static void		TopLevelEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		TopLevelReqProc _ANSI_ARGS_((ClientData dummy,
			    Tk_Window tkwin));
static void		UpdateGeometryInfo _ANSI_ARGS_((
			    ClientData clientData));
static void		UpdateWrapper _ANSI_ARGS_((TkWindow *winPtr));
static LRESULT CALLBACK	WmProc _ANSI_ARGS_((HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam));

/*
 *----------------------------------------------------------------------
 *
 * InitWm --
 *
 *	This routine creates the Wm toplevel decorative frame class.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Registers a new window class.
 *
 *----------------------------------------------------------------------
 */

static void
InitWm(void)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    WNDCLASS * classPtr;

    if (! tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	tsdPtr->firstWindow = 1;
    }
    if (! initialized) {
	Tcl_MutexLock(&winWmMutex);
	if (! initialized) {
	    initialized = 1;
	    classPtr = &toplevelClass;

    /*
     * When threads are enabled, we cannot use CLASSDC because
     * threads will then write into the same device context.
     * 
     * This is a hack; we should add a subsystem that manages
     * device context on a per-thread basis.  See also tkWinX.c,
     * which also initializes a WNDCLASS structure.
     */

#ifdef TCL_THREADS
	    classPtr->style = CS_HREDRAW | CS_VREDRAW;
#else
	    classPtr->style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC;
#endif
	    classPtr->cbClsExtra = 0;
	    classPtr->cbWndExtra = 0;
	    classPtr->hInstance = Tk_GetHINSTANCE();
	    classPtr->hbrBackground = NULL;
	    classPtr->lpszMenuName = NULL;
	    classPtr->lpszClassName = TK_WIN_TOPLEVEL_CLASS_NAME;
	    classPtr->lpfnWndProc = WmProc;
	    classPtr->hIcon = LoadIcon(Tk_GetHINSTANCE(), "tk");
	    classPtr->hCursor = LoadCursor(NULL, IDC_ARROW);

	    if (!RegisterClass(classPtr)) {
		panic("Unable to register TkTopLevel class");
	    }
	}
	Tcl_MutexUnlock(&winWmMutex);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetTopLevel --
 *
 *	This function retrieves the TkWindow associated with the
 *	given HWND.
 *
 * Results:
 *	Returns the matching TkWindow.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkWindow *
GetTopLevel(hwnd)
    HWND hwnd;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * If this function is called before the CreateWindowEx call
     * has completed, then the user data slot will not have been
     * set yet, so we use the global createWindow variable.
     */

    if (tsdPtr->createWindow) {
	return tsdPtr->createWindow;
    }
    return (TkWindow *) GetWindowLong(hwnd, GWL_USERDATA);
}

/*
 *----------------------------------------------------------------------
 *
 * SetLimits --
 *
 *	Updates the minimum and maximum window size constraints.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the values of the info pointer to reflect the current
 *	minimum and maximum size values.
 *
 *----------------------------------------------------------------------
 */

static void
SetLimits(hwnd, info)
    HWND hwnd;
    MINMAXINFO *info;
{
    register WmInfo *wmPtr;
    int maxWidth, maxHeight;
    int minWidth, minHeight;
    int base;
    TkWindow *winPtr = GetTopLevel(hwnd);

    if (winPtr == NULL) {
	return;
    }

    wmPtr = winPtr->wmInfoPtr;
    
    /*
     * Copy latest constraint info.
     */

    wmPtr->defMinWidth = info->ptMinTrackSize.x;
    wmPtr->defMinHeight = info->ptMinTrackSize.y;
    wmPtr->defMaxWidth = info->ptMaxTrackSize.x;
    wmPtr->defMaxHeight = info->ptMaxTrackSize.y;
    
    GetMaxSize(wmPtr, &maxWidth, &maxHeight);
    GetMinSize(wmPtr, &minWidth, &minHeight);

    if (wmPtr->gridWin != NULL) {
	base = winPtr->reqWidth - (wmPtr->reqGridWidth * wmPtr->widthInc);
	if (base < 0) {
	    base = 0;
	}
	base += wmPtr->borderWidth;
	info->ptMinTrackSize.x = base + (minWidth * wmPtr->widthInc);
	info->ptMaxTrackSize.x = base + (maxWidth * wmPtr->widthInc);

	base = winPtr->reqHeight - (wmPtr->reqGridHeight * wmPtr->heightInc);
	if (base < 0) {
	    base = 0;
	}
	base += wmPtr->borderHeight;
	info->ptMinTrackSize.y = base + (minHeight * wmPtr->heightInc);
	info->ptMaxTrackSize.y = base + (maxHeight * wmPtr->heightInc);
    } else {
	info->ptMaxTrackSize.x = maxWidth + wmPtr->borderWidth;
	info->ptMaxTrackSize.y = maxHeight + wmPtr->borderHeight;
	info->ptMinTrackSize.x = minWidth + wmPtr->borderWidth;
	info->ptMinTrackSize.y = minHeight + wmPtr->borderHeight;
    }

    /*
     * If the window isn't supposed to be resizable, then set the
     * minimum and maximum dimensions to be the same as the current size.
     */

    if (!(wmPtr->flags & WM_SYNC_PENDING)) {
	if (wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) {
	    info->ptMinTrackSize.x = winPtr->changes.width
		+ wmPtr->borderWidth;
	    info->ptMaxTrackSize.x = info->ptMinTrackSize.x;
	}
	if (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE) {
	    info->ptMinTrackSize.y = winPtr->changes.height 
		+ wmPtr->borderHeight;
	    info->ptMaxTrackSize.y = info->ptMinTrackSize.y;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinWmCleanup --
 *
 *	Unregisters classes registered by the window manager. This is
 *	called from the DLL main entry point when the DLL is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window classes are discarded.
 *
 *----------------------------------------------------------------------
 */

void
TkWinWmCleanup(hInstance)
    HINSTANCE hInstance;
{
    ThreadSpecificData *tsdPtr;

    /*
     * If we're using stubs to access the Tcl library, and they
     * haven't been initialized, we can't call Tcl_GetThreadData.
     */

#ifdef USE_TCL_STUBS
    if (tclStubsPtr == NULL) {
        return;
    }
#endif

    tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
        return;
    }
    tsdPtr->initialized = 0;
    
    UnregisterClass(TK_WIN_TOPLEVEL_CLASS_NAME, hInstance);
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

    wmPtr = (WmInfo *) ckalloc(sizeof(WmInfo));
    winPtr->wmInfoPtr = wmPtr;
    wmPtr->winPtr = winPtr;
    wmPtr->wrapper = NULL;
    wmPtr->titleUid = NULL;
    wmPtr->iconName = NULL;
    wmPtr->masterPtr = NULL;
    wmPtr->hints.flags = InputHint | StateHint;
    wmPtr->hints.input = True;
    wmPtr->hints.initial_state = NormalState;
    wmPtr->hints.icon_pixmap = None;
    wmPtr->hints.icon_window = None;
    wmPtr->hints.icon_x = wmPtr->hints.icon_y = 0;
    wmPtr->hints.icon_mask = None;
    wmPtr->hints.window_group = None;
    wmPtr->leaderName = NULL;
    wmPtr->icon = NULL;
    wmPtr->iconFor = NULL;
    wmPtr->sizeHintsFlags = 0;

    /*
     * Default the maximum dimensions to the size of the display.
     */

    wmPtr->defMinWidth = wmPtr->defMinHeight = 0;
    wmPtr->defMaxWidth = DisplayWidth(winPtr->display,
	    winPtr->screenNum);
    wmPtr->defMaxHeight = DisplayHeight(winPtr->display,
	    winPtr->screenNum);
    wmPtr->minWidth = wmPtr->minHeight = 1;
    wmPtr->maxWidth = wmPtr->maxHeight = 0;
    wmPtr->gridWin = NULL;
    wmPtr->widthInc = wmPtr->heightInc = 1;
    wmPtr->minAspect.x = wmPtr->minAspect.y = 1;
    wmPtr->maxAspect.x = wmPtr->maxAspect.y = 1;
    wmPtr->reqGridWidth = wmPtr->reqGridHeight = -1;
    wmPtr->gravity = NorthWestGravity;
    wmPtr->width = -1;
    wmPtr->height = -1;
    wmPtr->hMenu = NULL;
    wmPtr->x = winPtr->changes.x;
    wmPtr->y = winPtr->changes.y;
    wmPtr->borderWidth = 0;
    wmPtr->borderHeight = 0;
    
    wmPtr->cmapList = NULL;
    wmPtr->cmapCount = 0;

    wmPtr->configWidth = -1;
    wmPtr->configHeight = -1;
    wmPtr->protPtr = NULL;
    wmPtr->cmdArgv = NULL;
    wmPtr->clientMachine = NULL;
    wmPtr->flags = WM_NEVER_MAPPED;
    wmPtr->nextPtr = winPtr->dispPtr->firstWmPtr;
    winPtr->dispPtr->firstWmPtr = wmPtr;

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
 *----------------------------------------------------------------------
 *
 * UpdateWrapper --
 *
 *	This function creates the wrapper window that contains the
 *	window decorations and menus for a toplevel.  This function
 *	may be called after a window is mapped to change the window
 *	style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys any old wrapper window and replaces it with a newly
 *	created wrapper.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateWrapper(winPtr)
    TkWindow *winPtr;		/* Top-level window to redecorate. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    HWND parentHWND = NULL, oldWrapper;
    HWND child = TkWinGetHWND(winPtr->window);
    int x, y, width, height, state;
    WINDOWPLACEMENT place;
    Tcl_DString titleString;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    parentHWND = NULL;
    child = TkWinGetHWND(winPtr->window); 

    if (winPtr->flags & TK_EMBEDDED) {
	wmPtr->wrapper = (HWND) winPtr->privatePtr;
	if (wmPtr->wrapper == NULL) {
	    panic("TkWmMapWindow: Cannot find container window");
	}
	if (!IsWindow(wmPtr->wrapper)) {
	    panic("TkWmMapWindow: Container was destroyed");
	}

    } else {
	/*
	 * Pick the decorative frame style.  Override redirect windows get
	 * created as undecorated popups.  Transient windows get a modal
	 * dialog frame.  Neither override, nor transient windows appear in
	 * the Win95 taskbar.  Note that a transient window does not resize
	 * by default, so we need to explicitly add the WS_THICKFRAME style
	 * if we want it to be resizeable.
	 */
	
	if (winPtr->atts.override_redirect) {
	    wmPtr->style = WM_OVERRIDE_STYLE;
	    wmPtr->exStyle = EX_OVERRIDE_STYLE;
	} else if (wmPtr->masterPtr) {
	    wmPtr->style = WM_TRANSIENT_STYLE;
	    wmPtr->exStyle = EX_TRANSIENT_STYLE;
	    parentHWND = Tk_GetHWND(Tk_WindowId(wmPtr->masterPtr));
	    if (! ((wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) && 
		    (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE))) {
		wmPtr->style |= WS_THICKFRAME;
	    }
	} else {
	    wmPtr->style = WM_TOPLEVEL_STYLE;
	    wmPtr->exStyle = EX_TOPLEVEL_STYLE;
	}

	if ((wmPtr->flags & WM_WIDTH_NOT_RESIZABLE)
		&& (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE)) {
	    wmPtr->style &= ~ (WS_MAXIMIZEBOX | WS_SIZEBOX);
	}

	/*
	 * Compute the geometry of the parent and child windows.
	 */

	wmPtr->flags |= WM_CREATE_PENDING|WM_MOVE_PENDING;
	UpdateGeometryInfo((ClientData)winPtr);
	wmPtr->flags &= ~(WM_CREATE_PENDING|WM_MOVE_PENDING);

	width = wmPtr->borderWidth + winPtr->changes.width;
	height = wmPtr->borderHeight + winPtr->changes.height;

	/*
	 * Set the initial position from the user or program specified
	 * location.  If nothing has been specified, then let the system
	 * pick a location.
	 */

   
	if (!(wmPtr->sizeHintsFlags & (USPosition | PPosition))
		&& (wmPtr->flags & WM_NEVER_MAPPED)) {
	    x = CW_USEDEFAULT;
	    y = CW_USEDEFAULT;
	} else {
	    x = winPtr->changes.x;
	    y = winPtr->changes.y;
	}

	/*
	 * Create the containing window, and set the user data to point
	 * to the TkWindow.
	 */

	tsdPtr->createWindow = winPtr;
	Tcl_UtfToExternalDString(NULL, wmPtr->titleUid, -1, &titleString);
	wmPtr->wrapper = CreateWindowEx(wmPtr->exStyle,
		TK_WIN_TOPLEVEL_CLASS_NAME,
		Tcl_DStringValue(&titleString), wmPtr->style, x, y, width, 
		height, parentHWND, NULL, Tk_GetHINSTANCE(), NULL);
	Tcl_DStringFree(&titleString);
	SetWindowLong(wmPtr->wrapper, GWL_USERDATA, (LONG) winPtr);
	tsdPtr->createWindow = NULL;

	place.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(wmPtr->wrapper, &place);
	wmPtr->x = place.rcNormalPosition.left;
	wmPtr->y = place.rcNormalPosition.top;

	TkInstallFrameMenu((Tk_Window) winPtr);
    }

    /*
     * Now we need to reparent the contained window and set its
     * style appropriately.  Be sure to update the style first so that
     * Windows doesn't try to set the focus to the child window.
     */

    SetWindowLong(child, GWL_STYLE,
	    WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    if (winPtr->flags & TK_EMBEDDED) {
	SetWindowLong(child, GWL_WNDPROC, (LONG) TopLevelProc);
    }
    oldWrapper = SetParent(child, wmPtr->wrapper);
    if (oldWrapper && (oldWrapper != wmPtr->wrapper) 
	    && (oldWrapper != GetDesktopWindow())) {
	SetWindowLong(oldWrapper, GWL_USERDATA, (LONG) NULL);

	/*
	 * Remove the menubar before destroying the window so the menubar
	 * isn't destroyed.
	 */

	SetMenu(oldWrapper, NULL);
	DestroyWindow(oldWrapper);
    }
    wmPtr->flags &= ~WM_NEVER_MAPPED;
    SendMessage(wmPtr->wrapper, TK_ATTACHWINDOW, (WPARAM) child, 0);

    /*
     * Force an initial transition from withdrawn to the real
     * initial state.	 
     */

    state = wmPtr->hints.initial_state;
    wmPtr->hints.initial_state = WithdrawnState;
    TkpWmSetState(winPtr, state);

    /*
     * If we are embedded then force a mapping of the window now,
     * because we do not necessarily own the wrapper and may not
     * get another opportunity to map ourselves. We should not be
     * in either iconified or zoomed states when we get here, so
     * it is safe to just check for TK_EMBEDDED without checking
     * what state we are supposed to be in (default to NormalState).
     */

    if (winPtr->flags & TK_EMBEDDED) {
	XMapWindow(winPtr->display, winPtr->window);
    }

    /*
     * Set up menus on the wrapper if required.
     */
        
    if (wmPtr->hMenu != NULL) {
	wmPtr->flags = WM_SYNC_PENDING;
	SetMenu(wmPtr->wrapper, wmPtr->hMenu);
	wmPtr->flags &= ~WM_SYNC_PENDING;
    }

    /*
     * If this is the first window created by the application, then
     * we should activate the initial window.
     */

    if (tsdPtr->firstWindow) {
	tsdPtr->firstWindow = 0;
	SetActiveWindow(wmPtr->wrapper);
    }
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
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	InitWm();
    }

    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	if (wmPtr->hints.initial_state == WithdrawnState) {
	    return;
	}

	/*
	 * Map the window in either the iconified or normal state.  Note that
	 * we only send a map event if the window is in the normal state.
	 */

	TkpWmSetState(winPtr, wmPtr->hints.initial_state);
    }

    /*
     * This is the first time this window has ever been mapped.
     * Store all the window-manager-related information for the
     * window.
     */

    if (wmPtr->titleUid == NULL) {
	wmPtr->titleUid = winPtr->nameUid;
    }
    UpdateWrapper(winPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TkWmUnmapWindow --
 *
 *	This procedure is invoked to unmap a top-level window.  The
 *	only thing it does special is unmap the decorative frame before
 *	unmapping the toplevel window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unmaps the decorative frame and the window.
 *
 *--------------------------------------------------------------
 */

void
TkWmUnmapWindow(winPtr)
    TkWindow *winPtr;		/* Top-level window that's about to
				 * be unmapped. */
{
    TkpWmSetState(winPtr, WithdrawnState);
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
    int cmd;
    

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	wmPtr->hints.initial_state = state;
	return;
    }

    wmPtr->flags |= WM_SYNC_PENDING;
    if (state == WithdrawnState) {
	cmd = SW_HIDE;
    } else if (state == IconicState) {
	cmd = SW_SHOWMINNOACTIVE;
    } else if (state == NormalState) {
	cmd = SW_SHOWNOACTIVATE;
    } else if (state == ZoomState) {
	cmd = SW_SHOWMAXIMIZED;
    }

    ShowWindow(wmPtr->wrapper, cmd);
    wmPtr->flags &= ~WM_SYNC_PENDING;
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

    /*
     * Clean up event related window info.
     */

    if (winPtr->dispPtr->firstWmPtr == wmPtr) {
	winPtr->dispPtr->firstWmPtr = wmPtr->nextPtr;
    } else {
	register WmInfo *prevPtr;
	for (prevPtr = winPtr->dispPtr->firstWmPtr; ; prevPtr
		 = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		panic("couldn't unlink window in TkWmDeadWindow");
	    }
	    if (prevPtr->nextPtr == wmPtr) {
		prevPtr->nextPtr = wmPtr->nextPtr;
		break;
	    }
	}
    }

    /*
     * Reset all transient windows whose master is the dead window.
     */

    for (wmPtr2 = winPtr->dispPtr->firstWmPtr; wmPtr2 != NULL; wmPtr2
	     = wmPtr2->nextPtr) {
	if (wmPtr2->masterPtr == winPtr) {
	    wmPtr2->masterPtr = NULL;
	    if ((wmPtr2->wrapper != None)
		    && !(wmPtr2->flags & (WM_NEVER_MAPPED))) {
		UpdateWrapper(wmPtr2->winPtr);
	    }
	}
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
	Tcl_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
    }

    /*
     * Destroy the decorative frame window.
     */

    if (!(winPtr->flags & TK_EMBEDDED)) {
	if (wmPtr->wrapper != NULL) {
	    DestroyWindow(wmPtr->wrapper);
	} else {
	    DestroyWindow(Tk_GetHWND(winPtr->window));
	}
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
Tk_WmCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr = NULL;
    register WmInfo *wmPtr;
    int c;
    size_t length;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

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
	    Tcl_SetResult(interp, ((dispPtr->wmTracing) ? "on" : "off"), TCL_STATIC);
	    return TCL_OK;
	}
	return Tcl_GetBoolean(interp, argv[2], &dispPtr->wmTracing);
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
		if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		    XDeleteProperty(winPtr->display, winPtr->window,
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
		ckalloc((unsigned) (strlen(argv[3]) + 1));
	strcpy(wmPtr->clientMachine, argv[3]);
	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    XTextProperty textProp;
	    if (XStringListToTextProperty(&wmPtr->clientMachine, 1, &textProp)
		    != 0) {
		XSetWMClientMachine(winPtr->display, winPtr->window,
			&textProp);
		XFree((char *) textProp.value);
	    }
	}
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
	 * Now we need to force the updated colormaps to be installed.
	 */

	if (wmPtr == winPtr->dispPtr->foregroundWmPtr) {
	    InstallColormaps(wmPtr->wrapper, WM_QUERYNEWPALETTE, 1);
	} else {
	    InstallColormaps(wmPtr->wrapper, WM_PALETTECHANGED, 0);
	}
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
		if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		    XDeleteProperty(winPtr->display, winPtr->window,
			    Tk_InternAtom((Tk_Window) winPtr, "WM_COMMAND"));
		}
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
	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    XSetCommand(winPtr->display, winPtr->window, cmdArgv, cmdArgc);
	}
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
	TkpWmSetState(winPtr, NormalState);
	/*
	 * Follow Windows-like style here:
	 * raise the window to the top, and if it isn't overridden,
	 * then force the focus on it
	 */
	TkWmRestackToplevel(winPtr, Above, NULL);
	if (!(Tk_Attributes((Tk_Window) winPtr)->override_redirect)) {
	    TkSetFocusWin(winPtr, 1);
	}
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
	HWND hwnd;
	char buf[TCL_INTEGER_SPACE];

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " frame window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (Tk_WindowId((Tk_Window) winPtr) == None) {
	    Tk_MakeWindowExist((Tk_Window) winPtr);
	}
	hwnd = wmPtr->wrapper;
	if (hwnd == NULL) {
	    hwnd = Tk_GetHWND(Tk_WindowId((Tk_Window) winPtr));
	}
	sprintf(buf, "0x%x", (unsigned int) hwnd);
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
	if (wmPtr->masterPtr != NULL) {
	    Tcl_AppendResult(interp, "can't iconify \"", winPtr->pathName,
		    "\": it is a transient", (char *) NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->iconFor != NULL) {
	    Tcl_AppendResult(interp, "can't iconify ", argv[2],
		    ": it is an icon for ", winPtr->pathName, (char *) NULL);
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
	XSetWindowAttributes atts;

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
		/*
		 * Let the window use button events again, then remove
		 * it as icon window.
		 */

		atts.event_mask = Tk_Attributes(wmPtr->icon)->event_mask
			| ButtonPressMask;
		Tk_ChangeWindowAttributes(wmPtr->icon, CWEventMask, &atts);
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

		/*
		 * Let the window use button events again.
		 */

		atts.event_mask = Tk_Attributes(wmPtr->icon)->event_mask
			| ButtonPressMask;
		Tk_ChangeWindowAttributes(wmPtr->icon, CWEventMask, &atts);
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
	    wmPtr->hints.icon_window = Tk_WindowId(tkwin2);
	    wmPtr->hints.flags |= IconWindowHint;
	    wmPtr->icon = tkwin2;
	    wmPtr2->iconFor = (Tk_Window) winPtr;
	    if (!(wmPtr2->flags & WM_NEVER_MAPPED)) {
		if (XWithdrawWindow(Tk_Display(tkwin2), Tk_WindowId(tkwin2),
			Tk_ScreenNumber(tkwin2)) == 0) {
		    Tcl_SetResult(interp,
			    "couldn't send withdraw message to window manager",
			    TCL_STATIC);
		    return TCL_ERROR;
		}
	    }
	}
    } else if ((c == 'm') && (strncmp(argv[1], "maxsize", length) == 0)
	    && (length >= 2)) {
	int width, height;
	if ((argc != 3) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " maxsize window ?width height?\"",
                    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    char buf[TCL_INTEGER_SPACE * 2];
	    
	    GetMaxSize(wmPtr, &width, &height);
	    sprintf(buf, "%d %d", width, height);
    	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_OK;
	}
	if ((Tcl_GetInt(interp, argv[3], &width) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[4], &height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	wmPtr->maxWidth = width;
	wmPtr->maxHeight = height;
	goto updateGeom;
    } else if ((c == 'm') && (strncmp(argv[1], "minsize", length) == 0)
	    && (length >= 2)) {
	int width, height;
	if ((argc != 3) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " minsize window ?width height?\"",
                    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    char buf[TCL_INTEGER_SPACE * 2];
	    
	    GetMinSize(wmPtr, &width, &height);
	    sprintf(buf, "%d %d", width, height);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_OK;
	}
	if ((Tcl_GetInt(interp, argv[3], &width) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[4], &height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	wmPtr->minWidth = width;
	wmPtr->minHeight = height;
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
	if (!(wmPtr->flags & (WM_NEVER_MAPPED)
		&& !(winPtr->flags & TK_EMBEDDED))) {
	    UpdateWrapper(winPtr);
	}
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
	    } else if ((c == 'p')
                    && (strncmp(argv[3], "program", length) == 0)) {
		wmPtr->sizeHintsFlags &= ~USPosition;
		wmPtr->sizeHintsFlags |= PPosition;
	    } else {
		Tcl_AppendResult(interp, "bad argument \"", argv[3],
			"\": must be program or user", (char *) NULL);
		return TCL_ERROR;
	    }
	}
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
	    if (!(wmPtr->flags & WM_NEVER_MAPPED) && wmPtr->wrapper != NULL) {
		Tcl_DString titleString;
		Tcl_UtfToExternalDString(NULL, wmPtr->titleUid, -1, 
			&titleString);
		SetWindowText(wmPtr->wrapper, Tcl_DStringValue(&titleString));
		Tcl_DStringFree(&titleString);
	    }
	}
    } else if ((c == 't') && (strncmp(argv[1], "transient", length) == 0)
	    && (length >= 3)) {
	TkWindow *masterPtr;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
		    argv[0], " transient window ?master?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (wmPtr->masterPtr != NULL) {
		Tcl_SetResult(interp, Tk_PathName(wmPtr->masterPtr),
			TCL_STATIC);
	    }
	    return TCL_OK;
	}
	if (argv[3][0] == '\0') {
	    wmPtr->masterPtr = NULL;
	} else {
	    masterPtr = (TkWindow*) Tk_NameToWindow(interp, argv[3], tkwin);
	    if (masterPtr == NULL) {
		return TCL_ERROR;
	    }
	    if (masterPtr == winPtr) {
		wmPtr->masterPtr = NULL;
	    } else {
		Tk_MakeWindowExist((Tk_Window)masterPtr);

		/*
		 * Ensure that the master window is actually a Tk toplevel.
		 */

		while (!(masterPtr->flags & TK_TOP_LEVEL)) {
		    masterPtr = masterPtr->parentPtr;
		}
		wmPtr->masterPtr = masterPtr;

		/*
		 * Ensure that the transient window is either mapped or 
		 * unmapped like its master.
		 */

		TkpWmSetState(winPtr, NormalState);
	    }
	}
	if (!(wmPtr->flags & (WM_NEVER_MAPPED)
		&& !(winPtr->flags & TK_EMBEDDED))) {
	    UpdateWrapper(winPtr);
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
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
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

    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
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
TopLevelEventProc(clientData, eventPtr)
    ClientData clientData;		/* Window for which event occurred. */
    XEvent *eventPtr;			/* Event that just happened. */
{
    register TkWindow *winPtr = (TkWindow *) clientData;

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
    }
    else if (eventPtr->type == ConfigureNotify) {
	WmInfo *wmPtr;
	wmPtr = winPtr->wmInfoPtr;

	if (winPtr->flags & TK_EMBEDDED) {
	    Tk_Window tkwin = (Tk_Window)winPtr;
	    SendMessage(wmPtr->wrapper, TK_GEOMETRYREQ, Tk_ReqWidth(tkwin),
	        Tk_ReqHeight(tkwin));
	}
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
TopLevelReqProc(dummy, tkwin)
    ClientData dummy;			/* Not used. */
    Tk_Window tkwin;			/* Information about window. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr;

    wmPtr = winPtr->wmInfoPtr;
    if (winPtr->flags & TK_EMBEDDED) {
	SendMessage(wmPtr->wrapper, TK_GEOMETRYREQ, Tk_ReqWidth(tkwin),
	    Tk_ReqHeight(tkwin));
    }
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
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
 *	This procedure doesn't return until the system has
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
UpdateGeometryInfo(clientData)
    ClientData clientData;		/* Pointer to the window's record. */
{
    int x, y;			/* Position of border on desktop. */
    int width, height;		/* Size of client area. */
    RECT rect;
    register TkWindow *winPtr = (TkWindow *) clientData;
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    wmPtr->flags &= ~WM_UPDATE_PENDING;

    /*
     * If the window is minimized or maximized, we should not update
     * our geometry since it will end up with the wrong values.
     * ConfigureToplevel will reschedule UpdateGeometryInfo when the
     * state of the window changes.
     */

    if (IsIconic(wmPtr->wrapper) || IsZoomed(wmPtr->wrapper)) {
	return;
    }

    if (wmPtr->flags & WM_UPDATE_SIZE_HINTS) {
	wmPtr->flags &= ~WM_UPDATE_SIZE_HINTS;
	UpdateWrapper(winPtr);
    }
	   
    /*
     * Compute the border size for the current window style.  This
     * size will include the resize handles, the title bar and the
     * menubar.  Note that this size will not be correct if the
     * menubar spans multiple lines.  The height will be off by a
     * multiple of the menubar height.  It really only measures the
     * minimum size of the border.
     */

    rect.left = rect.right = rect.top = rect.bottom = 0;
    AdjustWindowRectEx(&rect, wmPtr->style, wmPtr->hMenu != NULL,
	    wmPtr->exStyle);
    wmPtr->borderWidth = rect.right - rect.left;
    wmPtr->borderHeight = rect.bottom - rect.top;

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
	x = DisplayWidth(winPtr->display, winPtr->screenNum) - wmPtr->x
		- (width + wmPtr->borderWidth);
    } else {
	x =  wmPtr->x;
    }
    if (wmPtr->flags & WM_NEGATIVE_Y) {
	y = DisplayHeight(winPtr->display, winPtr->screenNum) - wmPtr->y
		- (height + wmPtr->borderHeight);
    } else {
	y =  wmPtr->y;
    }

    /*
     * If this window is embedded and the container is also in this
     * process, we don't need to do anything special about the
     * geometry, except to make sure that the desired size is known
     * by the container.  Also, zero out any position information,
     * since embedded windows are not allowed to move.
     */

    if (winPtr->flags & TK_BOTH_HALVES) {
	wmPtr->x = wmPtr->y = 0;
	wmPtr->flags &= ~(WM_NEGATIVE_X|WM_NEGATIVE_Y);
	Tk_GeometryRequest((Tk_Window) TkpGetOtherWindow(winPtr),
		width, height);
	return;
    }

    /*
     * Reconfigure the window if it isn't already configured correctly.  Base
     * the size check on what we *asked for* last time, not what we got.
     * Return immediately if there have been no changes in the requested
     * geometry of the toplevel.
     */
    /* TODO: need to add flag for possible menu size change */

    if (!((wmPtr->flags & WM_MOVE_PENDING)
	    || (width != wmPtr->configWidth)
	    || (height != wmPtr->configHeight))) {
	return;
    }
    wmPtr->flags &= ~WM_MOVE_PENDING;

    wmPtr->configWidth = width;
    wmPtr->configHeight = height;
	    
    /*
     * Don't bother moving the window if we are in the process of
     * creating it.  Just update the geometry info based on what
     * we asked for.
     */

    if (wmPtr->flags & WM_CREATE_PENDING) {
	winPtr->changes.x = x;
	winPtr->changes.y = y;
	winPtr->changes.width = width;
	winPtr->changes.height = height;
	return;
    }

    wmPtr->flags |= WM_SYNC_PENDING;
    if (winPtr->flags & TK_EMBEDDED) {
	/*
	 * The wrapper window is in a different process, so we need
	 * to send it a geometry request.  This protocol assumes that
	 * the other process understands this Tk message, otherwise
	 * our requested geometry will be ignored.
	 */

	SendMessage(wmPtr->wrapper, TK_GEOMETRYREQ, width, height);
    } else {
	int reqHeight, reqWidth;
	RECT windowRect;
	int menuInc = GetSystemMetrics(SM_CYMENU);
	int newHeight;

	/*
	 * We have to keep resizing the window until we get the
	 * requested height in the client area. If the client
	 * area has zero height, then the window rect is too
	 * small by definition. Try increasing the border height
	 * and try again. Once we have a positive size, then
	 * we can adjust the height exactly. If the window
	 * rect comes back smaller than we requested, we have
	 * hit the maximum constraints that Windows imposes.
	 * Once we find a positive client size, the next size
	 * is the one we try no matter what.
	 */

	reqHeight = height + wmPtr->borderHeight;
	reqWidth = width + wmPtr->borderWidth;

	while (1) {
	    MoveWindow(wmPtr->wrapper, x, y, reqWidth, reqHeight, TRUE);
	    GetWindowRect(wmPtr->wrapper, &windowRect);
	    newHeight = windowRect.bottom - windowRect.top;

	    /*
	     * If the request wasn't satisfied, we have hit an external
	     * constraint and must stop.
	     */

	    if (newHeight < reqHeight) {
		break;
	    }

	    /*
	     * Now check the size of the client area against our ideal.
	     */

	    GetClientRect(wmPtr->wrapper, &windowRect);
	    newHeight = windowRect.bottom - windowRect.top;
	    
	    if (newHeight == height) {
		/*
		 * We're done.
		 */
		break;
	    } else if (newHeight > height) {
		/*
		 * One last resize to get rid of the extra space.
		 */
		menuInc = newHeight - height;
		reqHeight -= menuInc;
		if (wmPtr->flags & WM_NEGATIVE_Y) {
		    y += menuInc;
		}
		MoveWindow(wmPtr->wrapper, x, y, reqWidth, reqHeight, TRUE);
		break;
	    }

	    /*
	     * We didn't get enough space to satisfy our requested
	     * height, so the menu must have wrapped.  Increase the
	     * size of the window by one menu height and move the
	     * window if it is positioned relative to the lower right
	     * corner of the screen.
	     */

	    reqHeight += menuInc;
	    if (wmPtr->flags & WM_NEGATIVE_Y) {
		y -= menuInc;
	    }
	}
	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    DrawMenuBar(wmPtr->wrapper);
	}
    }
    wmPtr->flags &= ~WM_SYNC_PENDING;
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
 *	the root coordinates of the (0,0) point in tkwin.
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
    register TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * If the window is mapped, let Windows figure out the translation.
     */

    if (winPtr->window != None) {
	HWND hwnd = Tk_GetHWND(winPtr->window);
	POINT point;

	point.x = 0;
	point.y = 0;

	ClientToScreen(hwnd, &point);

	*xPtr = point.x;
	*yPtr = point.y;
    } else {
	*xPtr = 0;
	*yPtr = 0;
    }
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
    POINT pos;
    HWND hwnd;
    TkWindow *winPtr;

    pos.x = rootX;
    pos.y = rootY;
    hwnd = WindowFromPoint(pos);

    winPtr = (TkWindow *) Tk_HWNDToWindow(hwnd);
    if (winPtr && (winPtr->mainPtr == ((TkWindow *) tkwin)->mainPtr)) {
	return (Tk_Window) winPtr;
    }
    return NULL;
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
    TkWindow *winPtr = (TkWindow *) tkwin;

    *xPtr = 0;
    *yPtr = 0;
    *widthPtr = DisplayWidth(winPtr->display, winPtr->screenNum);
    *heightPtr = DisplayHeight(winPtr->display, winPtr->screenNum);
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
    Tcl_Interp *interp;

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
TkWmRestackToplevel(winPtr, aboveBelow, otherPtr)
    TkWindow *winPtr;		/* Window to restack. */
    int aboveBelow;		/* Gives relative position for restacking;
				 * must be Above or Below. */
    TkWindow *otherPtr;		/* Window relative to which to restack;
				 * if NULL, then winPtr gets restacked
				 * above or below *all* siblings. */
{
    HWND hwnd, insertAfter;

    /*
     * Can't set stacking order properly until the window is on the
     * screen (mapping it may give it a reparent window).
     */

    if (winPtr->window == None) {
	Tk_MakeWindowExist((Tk_Window) winPtr);
    }
    if (winPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	TkWmMapWindow(winPtr);
    }
    hwnd = (winPtr->wmInfoPtr->wrapper != NULL)
	? winPtr->wmInfoPtr->wrapper : Tk_GetHWND(winPtr->window);

    
    if (otherPtr != NULL) {
	if (otherPtr->window == None) {
	    Tk_MakeWindowExist((Tk_Window) otherPtr);
	}
	if (otherPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	    TkWmMapWindow(otherPtr);
	}
	insertAfter = (otherPtr->wmInfoPtr->wrapper != NULL)
	    ? otherPtr->wmInfoPtr->wrapper : Tk_GetHWND(otherPtr->window);
    } else {
	insertAfter = NULL;
    }

    TkWinSetWindowPos(hwnd, insertAfter, aboveBelow);
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
     * Now we need to force the updated colormaps to be installed.
     */

    if (topPtr->wmInfoPtr == winPtr->dispPtr->foregroundWmPtr) {
	InstallColormaps(topPtr->wmInfoPtr->wrapper, WM_QUERYNEWPALETTE, 1);
    } else {
	InstallColormaps(topPtr->wmInfoPtr->wrapper, WM_PALETTECHANGED, 0);
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
	    topPtr->wmInfoPtr->cmapCount = count-1;
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinSetMenu--
 *
 *	Associcates a given HMENU to a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu will end up being drawn in the window, and the geometry
 *	of the window will have to be changed.
 *
 *----------------------------------------------------------------------
 */

void
TkWinSetMenu(tkwin, hMenu)
    Tk_Window tkwin;		/* the window to put the menu in */
    HMENU hMenu;		/* the menu to set */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    wmPtr->hMenu = hMenu;

    if (!(wmPtr->flags & TK_EMBEDDED)) {
	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    int syncPending = wmPtr->flags & WM_SYNC_PENDING;

	    wmPtr->flags |= WM_SYNC_PENDING;
	    SetMenu(wmPtr->wrapper, hMenu);
	    if (!syncPending) {
		wmPtr->flags &= ~WM_SYNC_PENDING;
	    }
	}
	if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	    Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
	    wmPtr->flags |= WM_UPDATE_PENDING|WM_MOVE_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureTopLevel --
 *
 *	Generate a ConfigureNotify event based on the current position
 *	information.  This procedure is called by TopLevelProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Queues a new event.
 *
 *----------------------------------------------------------------------
 */

static void
ConfigureTopLevel(pos)
    WINDOWPOS *pos;
{
    TkWindow *winPtr = GetTopLevel(pos->hwnd);
    WmInfo *wmPtr;
    int state;			/* Current window state. */
    RECT rect;
    WINDOWPLACEMENT windowPos;
    
    if (winPtr == NULL) {
	return;
    }

    wmPtr = winPtr->wmInfoPtr;

    /*
     * Determine the current window state.
     */

    if (!IsWindowVisible(wmPtr->wrapper)) {
	state = WithdrawnState;
    } else {
	windowPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(wmPtr->wrapper, &windowPos);
	switch (windowPos.showCmd) {
	    case SW_SHOWMAXIMIZED:
		state = ZoomState;
		break;
	    case SW_SHOWMINIMIZED:
		state = IconicState;
		break;
	    case SW_SHOWNORMAL:
		state = NormalState;
		break;
	}
    }

    /*
     * If the state of the window just changed, be sure to update the
     * child window information.
     */

    if (wmPtr->hints.initial_state != state) {
	wmPtr->hints.initial_state = state;
	switch (state) {
	    case WithdrawnState:
	    case IconicState:
		XUnmapWindow(winPtr->display, winPtr->window);
		break;

	    case NormalState:
		/*
		 * Schedule a geometry update.  Since we ignore geometry
		 * requests while in any other state, the geometry info
		 * may be stale.
		 */

		if (!(wmPtr->flags & WM_UPDATE_PENDING)) {
		    Tcl_DoWhenIdle(UpdateGeometryInfo,
			    (ClientData) winPtr);
		    wmPtr->flags |= WM_UPDATE_PENDING;
		}
		/* fall through */
	    case ZoomState:
		XMapWindow(winPtr->display, winPtr->window);
		pos->flags |= SWP_NOMOVE | SWP_NOSIZE;
		break;
	}
    }

    /*
     * Don't report geometry changes in the Iconic or Withdrawn states.
     */

    if (state == WithdrawnState || state == IconicState) {
	return;
    }


    /*
     * Compute the current geometry of the client area, reshape the
     * Tk window and generate a ConfigureNotify event.
     */

    GetClientRect(wmPtr->wrapper, &rect);
    winPtr->changes.x = pos->x;
    winPtr->changes.y = pos->y;
    winPtr->changes.width = rect.right - rect.left;
    winPtr->changes.height = rect.bottom - rect.top;
    wmPtr->borderHeight = pos->cy - winPtr->changes.height;
    MoveWindow(Tk_GetHWND(winPtr->window), 0, 0,
	    winPtr->changes.width, winPtr->changes.height, TRUE);
    GenerateConfigureNotify(winPtr);

    /*
     * Update window manager geometry info if needed.
     */

    if (state == NormalState) {

	/* 
	 * Update size information from the event.  There are a couple of
	 * tricky points here:
	 *
	 * 1. If the user changed the size externally then set wmPtr->width
	 *    and wmPtr->height just as if a "wm geometry" command had been
	 *    invoked with the same information.
	 * 2. However, if the size is changing in response to a request
	 *    coming from us (sync is set), then don't set
	 *    wmPtr->width or wmPtr->height (otherwise the window will stop
	 *    tracking geometry manager requests).
	 */

	if (!(wmPtr->flags & WM_SYNC_PENDING)) {
	    if (!(pos->flags & SWP_NOSIZE)) {
		if ((wmPtr->width == -1)
			&& (winPtr->changes.width == winPtr->reqWidth)) {
		    /*
		     * Don't set external width, since the user didn't
		     * change it from what the widgets asked for.
		     */
		} else {
		    if (wmPtr->gridWin != NULL) {
			wmPtr->width = wmPtr->reqGridWidth
			    + (winPtr->changes.width - winPtr->reqWidth)
			    / wmPtr->widthInc;
			if (wmPtr->width < 0) {
			    wmPtr->width = 0;
			}
		    } else {
			wmPtr->width = winPtr->changes.width;
		    }
		}
		if ((wmPtr->height == -1)
			&& (winPtr->changes.height == winPtr->reqHeight)) {
		    /*
		     * Don't set external height, since the user didn't change
		     * it from what the widgets asked for.
		     */
		} else {
		    if (wmPtr->gridWin != NULL) {
			wmPtr->height = wmPtr->reqGridHeight
			    + (winPtr->changes.height - winPtr->reqHeight)
			    / wmPtr->heightInc;
			if (wmPtr->height < 0) {
			    wmPtr->height = 0;
			}
		    } else {
			wmPtr->height = winPtr->changes.height;
		    }
		}
		wmPtr->configWidth = winPtr->changes.width;
		wmPtr->configHeight = winPtr->changes.height;
	    }
	    /*
	     * If the user moved the window, we should switch back
	     * to normal coordinates.
	     */

	    if (!(pos->flags & SWP_NOMOVE)) {
		wmPtr->flags &= ~(WM_NEGATIVE_X | WM_NEGATIVE_Y);
	    }
	}
   
	/*
	 * Update the wrapper window location information. 
	 */

	if (wmPtr->flags & WM_NEGATIVE_X) {
	    wmPtr->x = DisplayWidth(winPtr->display, winPtr->screenNum)
		- winPtr->changes.x - (winPtr->changes.width
			+ wmPtr->borderWidth);
	} else {
	    wmPtr->x = winPtr->changes.x;
	}
	if (wmPtr->flags & WM_NEGATIVE_Y) {
	    wmPtr->y = DisplayHeight(winPtr->display, winPtr->screenNum)
		- winPtr->changes.y - (winPtr->changes.height
			+ wmPtr->borderHeight);
	} else {
	    wmPtr->y = winPtr->changes.y;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateConfigureNotify --
 *
 *	Generate a ConfigureNotify event from the current geometry
 *	information for the specified toplevel window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends an X event.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateConfigureNotify(winPtr)
    TkWindow *winPtr;
{
    XEvent event;

    /*
     * Generate a ConfigureNotify event.
     */

    event.type = ConfigureNotify;
    event.xconfigure.serial = winPtr->display->request;
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.border_width = winPtr->changes.border_width;
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.above = None;
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * InstallColormaps --
 *
 *	Installs the colormaps associated with the toplevel which is
 *	currently active.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change the system palette and generate damage.
 *
 *----------------------------------------------------------------------
 */

static int
InstallColormaps(hwnd, message, isForemost)
    HWND hwnd;			/* Toplevel wrapper window whose colormaps
				 * should be installed. */
    int message;		/* Either WM_PALETTECHANGED or
				 * WM_QUERYNEWPALETTE */
    int isForemost;		/* 1 if window is foremost, else 0 */
{
    int i;
    HDC dc;
    HPALETTE oldPalette;
    TkWindow *winPtr = GetTopLevel(hwnd);
    WmInfo *wmPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
	    
    if (winPtr == NULL) {
	return 0;
    }

    wmPtr = winPtr->wmInfoPtr;

    if (message == WM_QUERYNEWPALETTE) {
	/*
	 * Case 1: This window is about to become the foreground window, so we
	 * need to install the primary palette. If the system palette was
	 * updated, then Windows will generate a WM_PALETTECHANGED message.
	 * Otherwise, we have to synthesize one in order to ensure that the
	 * secondary palettes are installed properly.
	 */

	winPtr->dispPtr->foregroundWmPtr = wmPtr;

	if (wmPtr->cmapCount > 0) {
	    winPtr = wmPtr->cmapList[0];
	}

	tsdPtr->systemPalette = TkWinGetPalette(winPtr->atts.colormap);
	dc = GetDC(hwnd);
	oldPalette = SelectPalette(dc, tsdPtr->systemPalette, FALSE);
	if (RealizePalette(dc)) {
	    RefreshColormap(winPtr->atts.colormap, winPtr->dispPtr);
	} else if (wmPtr->cmapCount > 1) {
	    SelectPalette(dc, oldPalette, TRUE);
	    RealizePalette(dc);
	    ReleaseDC(hwnd, dc);
	    SendMessage(hwnd, WM_PALETTECHANGED, (WPARAM)hwnd,
		    (LPARAM)NULL);
	    return TRUE;
	}

    } else {
	/*
	 * Window is being notified of a change in the system palette.
	 * If this window is the foreground window, then we should only
	 * install the secondary palettes, since the primary was installed
	 * in response to the WM_QUERYPALETTE message.  Otherwise, install
	 * all of the palettes.
	 */


	if (!isForemost) {
	    if (wmPtr->cmapCount > 0) {
		winPtr = wmPtr->cmapList[0];
	    }
	    i = 1;
	} else {
	    if (wmPtr->cmapCount <= 1) {
		return TRUE;
	    }
	    winPtr = wmPtr->cmapList[1];
	    i = 2;
	}
	dc = GetDC(hwnd);
	oldPalette = SelectPalette(dc,
		TkWinGetPalette(winPtr->atts.colormap), TRUE);
	if (RealizePalette(dc)) {
	    RefreshColormap(winPtr->atts.colormap, winPtr->dispPtr);
	}
	for (; i < wmPtr->cmapCount; i++) {
	    winPtr = wmPtr->cmapList[i];
	    SelectPalette(dc, TkWinGetPalette(winPtr->atts.colormap), TRUE);
	    if (RealizePalette(dc)) {
		RefreshColormap(winPtr->atts.colormap, winPtr->dispPtr);
	    }
	}
    }

    SelectPalette(dc, oldPalette, TRUE);
    RealizePalette(dc);
    ReleaseDC(hwnd, dc);
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * RefreshColormap --
 *
 *	This function is called to force all of the windows that use
 *	a given colormap to redraw themselves.  The quickest way to
 *	do this is to iterate over the toplevels, looking in the
 *	cmapList for matches.  This will quickly eliminate subtrees
 *	that don't use a given colormap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes damage events to be generated.
 *
 *----------------------------------------------------------------------
 */

static void
RefreshColormap(colormap, dispPtr)
    Colormap colormap;
    TkDisplay *dispPtr;
{
    WmInfo *wmPtr;
    int i;

    for (wmPtr = dispPtr->firstWmPtr; wmPtr != NULL; wmPtr = wmPtr->nextPtr) {
	if (wmPtr->cmapCount > 0) {
	    for (i = 0; i < wmPtr->cmapCount; i++) {
		if ((wmPtr->cmapList[i]->atts.colormap == colormap)
			&& Tk_IsMapped(wmPtr->cmapList[i])) {
		    InvalidateSubTree(wmPtr->cmapList[i], colormap);
		}
	    }
	} else if ((wmPtr->winPtr->atts.colormap == colormap)
		&& Tk_IsMapped(wmPtr->winPtr)) {
	    InvalidateSubTree(wmPtr->winPtr, colormap);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InvalidateSubTree --
 *
 *	This function recursively generates damage for a window and
 *	all of its mapped children that belong to the same toplevel and
 *	are using the specified colormap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates damage for the specified subtree.
 *
 *----------------------------------------------------------------------
 */

static void
InvalidateSubTree(winPtr, colormap)
    TkWindow *winPtr;
    Colormap colormap;
{
    TkWindow *childPtr;

    /*
     * Generate damage for the current window if it is using the
     * specified colormap.
     */

    if (winPtr->atts.colormap == colormap) {
	InvalidateRect(Tk_GetHWND(winPtr->window), NULL, FALSE);
    }

    for (childPtr = winPtr->childList; childPtr != NULL;
	    childPtr = childPtr->nextPtr) {
	/*
	 * We can stop the descent when we hit an unmapped or
	 * toplevel window.  
	 */

	if (!Tk_IsTopLevel(childPtr) && Tk_IsMapped(childPtr)) {
	    InvalidateSubTree(childPtr, colormap);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetSystemPalette --
 *
 *	Retrieves the currently installed foreground palette.
 *
 * Results:
 *	Returns the global foreground palette, if there is one.
 *	Otherwise, returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HPALETTE
TkWinGetSystemPalette()
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    return tsdPtr->systemPalette;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMinSize --
 *
 *	This procedure computes the current minWidth and minHeight
 *	values for a window, taking into account the possibility
 *	that they may be defaulted.
 *
 * Results:
 *	The values at *minWidthPtr and *minHeightPtr are filled
 *	in with the minimum allowable dimensions of wmPtr's window,
 *	in grid units.  If the requested minimum is smaller than the
 *	system required minimum, then this procedure computes the
 *	smallest size that will satisfy both the system and the
 *	grid constraints.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMinSize(wmPtr, minWidthPtr, minHeightPtr)
    WmInfo *wmPtr;		/* Window manager information for the
				 * window. */
    int *minWidthPtr;		/* Where to store the current minimum
				 * width of the window. */
    int *minHeightPtr;		/* Where to store the current minimum
				 * height of the window. */
{
    int tmp, base;
    TkWindow *winPtr = wmPtr->winPtr;

    /*
     * Compute the minimum width by taking the default client size
     * and rounding it up to the nearest grid unit.  Return the greater
     * of the default minimum and the specified minimum.
     */

    tmp = wmPtr->defMinWidth - wmPtr->borderWidth;
    if (tmp < 0) {
	tmp = 0;
    }
    if (wmPtr->gridWin != NULL) {
	base = winPtr->reqWidth - (wmPtr->reqGridWidth * wmPtr->widthInc);
	if (base < 0) {
	    base = 0;
	}
	tmp = ((tmp - base) + wmPtr->widthInc - 1)/wmPtr->widthInc;
    }
    if (tmp < wmPtr->minWidth) {
	tmp = wmPtr->minWidth;
    }
    *minWidthPtr = tmp;

    /*
     * Compute the minimum height in a similar fashion.
     */

    tmp = wmPtr->defMinHeight - wmPtr->borderHeight;
    if (tmp < 0) {
	tmp = 0;
    }
    if (wmPtr->gridWin != NULL) {
	base = winPtr->reqHeight - (wmPtr->reqGridHeight * wmPtr->heightInc);
	if (base < 0) {
	    base = 0;
	}
	tmp = ((tmp - base) + wmPtr->heightInc - 1)/wmPtr->heightInc;
    }
    if (tmp < wmPtr->minHeight) {
	tmp = wmPtr->minHeight;
    }
    *minHeightPtr = tmp;
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

	tmp = wmPtr->defMaxWidth - wmPtr->borderWidth;
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
	tmp = wmPtr->defMaxHeight - wmPtr->borderHeight;
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
 * TopLevelProc --
 *
 *	Callback from Windows whenever an event occurs on a top level
 *	window.
 *
 * Results:
 *	Standard Windows return value.
 *
 * Side effects:
 *	Default window behavior.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
TopLevelProc(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    if (message == WM_WINDOWPOSCHANGED) {
	WINDOWPOS *pos = (WINDOWPOS *) lParam;
	TkWindow *winPtr = (TkWindow *) Tk_HWNDToWindow(pos->hwnd);
    
	if (winPtr == NULL) {
	    return 0;
	}

	/*
	 * Update the shape of the contained window.
	 */

	if (!(pos->flags & SWP_NOSIZE)) {
	    winPtr->changes.width = pos->cx;
	    winPtr->changes.height = pos->cy;
	}
	if (!(pos->flags & SWP_NOMOVE)) {
	    winPtr->changes.x = pos->x;
	    winPtr->changes.y = pos->y;
	}

	GenerateConfigureNotify(winPtr);

	Tcl_ServiceAll();
	return 0;
    }
    return TkWinChildProc(hwnd, message, wParam, lParam);
}

/*
 *----------------------------------------------------------------------
 *
 * WmProc --
 *
 *	Callback from Windows whenever an event occurs on the decorative
 *	frame.
 *
 * Results:
 *	Standard Windows return value.
 *
 * Side effects:
 *	Default window behavior.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
WmProc(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    static int inMoveSize = 0;
    static oldMode;	/* This static is set upon entering move/size mode
			 * and is used to reset the service mode after
			 * leaving move/size mode.  Note that this mechanism
			 * assumes move/size is only one level deep. */
    LRESULT result;
    TkWindow *winPtr = NULL;

    if (TkWinHandleMenuEvent(&hwnd, &message, &wParam, &lParam, &result)) {
	goto done;
    }

    switch (message) {
	case WM_KILLFOCUS:
	case WM_ERASEBKGND:
	    result = 0;
	    goto done;

	case WM_ENTERSIZEMOVE:
	    inMoveSize = 1;

	    /*
	     * Cancel any current mouse timer.  If the mouse timer
	     * fires during the size/move mouse capture, it will
	     * release the capture, which is wrong.
	     */

	    TkWinCancelMouseTimer();

	    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
	    break;

	case WM_ACTIVATE:
	case WM_EXITSIZEMOVE:
	    if (inMoveSize) {
		inMoveSize = 0;
		Tcl_SetServiceMode(oldMode);
	    }
	    break;

	case WM_GETMINMAXINFO: 
	    SetLimits(hwnd, (MINMAXINFO *) lParam);
	    result = 0;
	    goto done;

	case WM_PALETTECHANGED:
	    result = InstallColormaps(hwnd, WM_PALETTECHANGED,
		    hwnd == (HWND)wParam);
	    goto done;

	case WM_QUERYNEWPALETTE:
	    result = InstallColormaps(hwnd, WM_QUERYNEWPALETTE, TRUE);
	    goto done;
	    
	case WM_WINDOWPOSCHANGED:
	    ConfigureTopLevel((WINDOWPOS *) lParam);
	    result = 0;
	    goto done;

	case WM_NCHITTEST: {
	    winPtr = GetTopLevel(hwnd);
	    if (winPtr && (TkGrabState(winPtr) == TK_GRAB_EXCLUDED)) {
		/*
		 * This window is outside the grab heirarchy, so don't let any
		 * of the normal non-client processing occur.  Note that this
		 * implementation is not strictly correct because the grab
		 * might change between now and when the event would have been
		 * processed by Tk, but it's close enough.
		 */

		result = HTCLIENT;
		goto done;
	    }
	    break;
	}

	case WM_MOUSEACTIVATE: {
	    ActivateEvent *eventPtr;
	    winPtr = GetTopLevel((HWND) wParam);

	    /*
	     * Don't activate the window yet since there may be grabs
	     * that should take precedence.  Instead we need to queue
	     * an event so we can check the grab state right before we
	     * handle the mouse event.
	     */

	    if (winPtr) { 
		eventPtr = (ActivateEvent *)ckalloc(sizeof(ActivateEvent));
		eventPtr->ev.proc = ActivateWindow;
		eventPtr->winPtr = winPtr;
		Tcl_QueueEvent((Tcl_Event*)eventPtr, TCL_QUEUE_TAIL);
	    }
	    result = MA_NOACTIVATE;
	    goto done;
	}

	default:
	    break;
    }

    winPtr = GetTopLevel(hwnd);
    if (winPtr && winPtr->window) {
	HWND child = Tk_GetHWND(winPtr->window);
	if (message == WM_SETFOCUS) {
	    SetFocus(child);
	    result = 0;
	} else if (!Tk_TranslateWinEvent(child, message, wParam, lParam,
		&result)) {
	    result = DefWindowProc(hwnd, message, wParam, lParam);
	}
    } else {
	result = DefWindowProc(hwnd, message, wParam, lParam);
    }

    done:
    Tcl_ServiceAll();
    return result;
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
    XSetWindowAttributes atts;

    if (transient) {
	atts.override_redirect = True;
	atts.save_under = True;
    } else {
	atts.override_redirect = False;
	atts.save_under = False;
    }
	
    if ((atts.override_redirect != Tk_Attributes(tkwin)->override_redirect)
	    || (atts.save_under != Tk_Attributes(tkwin)->save_under)) {
	Tk_ChangeWindowAttributes(tkwin,
		CWOverrideRedirect|CWSaveUnder, &atts);
    }
	
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetWrapperWindow --
 *
 *	Gets the Windows HWND for a given window.
 *
 * Results:
 *	Returns the wrapper window for a Tk window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HWND
TkWinGetWrapperWindow(
    Tk_Window tkwin)		/* The window we need the wrapper from */
{
    TkWindow *winPtr = (TkWindow *)tkwin;
    return (winPtr->wmInfoPtr->wrapper);
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
TkWmFocusToplevel(winPtr)
    TkWindow *winPtr;		/* Window that received a focus-related
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
 * ActivateWindow --
 *
 *	This function is called when an ActivateEvent is processed.
 *
 * Results:
 *	Returns 1 to indicate that the event was handled, else 0.
 *
 * Side effects:
 *	May activate the toplevel window associated with the event.
 *
 *----------------------------------------------------------------------
 */

static int
ActivateWindow(
    Tcl_Event *evPtr,		/* Pointer to ActivateEvent. */
    int flags)			/* Notifier event mask. */
{
    TkWindow *winPtr;

    if (! (flags & TCL_WINDOW_EVENTS)) {
	return 0;
    }

    winPtr = ((ActivateEvent *) evPtr)->winPtr;

    /*
     * Ensure that the window is not excluded by a grab.
     */

    if (winPtr && (TkGrabState(winPtr) != TK_GRAB_EXCLUDED)) {
	SetFocus(Tk_GetHWND(winPtr->window));
    }
    
    return 1;
}


/*
 *----------------------------------------------------------------------
 *
 * TkWinSetForegroundWindow --
 *
 *	This function is a wrapper for SetForegroundWindow, calling
 *      it on the wrapper window because it has no affect on child
 *      windows.
 *
 * Results:
 *	none
 *
 * Side effects:
 *	May activate the toplevel window.
 *
 *----------------------------------------------------------------------
 */

void
TkWinSetForegroundWindow(winPtr)
    TkWindow *winPtr;
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    
    if (wmPtr->wrapper != NULL) {
	SetForegroundWindow(wmPtr->wrapper);
    } else {
	SetForegroundWindow(Tk_GetHWND(winPtr->window));
    }
}
