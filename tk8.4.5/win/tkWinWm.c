/*
 * tkWinWm.c --
 *
 *	This module takes care of the interactions between a Tk-based
 *	application and the window manager.  Among other things, it
 *	implements the "wm" command and passes geometry information
 *	to the window manager.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"
#include <shellapi.h>

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
 * Helper type passed via lParam to TkWmStackorderToplevelEnumProc
 */
typedef struct TkWmStackorderToplevelPair {
    Tcl_HashTable *table;
    TkWindow **window_ptr;
} TkWmStackorderToplevelPair;

/*
 * This structure represents the contents of a icon, in terms of its
 * image.  The HICON is an internal Windows format.  Most of these
 * icon-specific-structures originated with the Winico extension.
 * We stripped out unused parts of that code, and integrated the
 * code more naturally with Tcl.
 */
typedef struct {
	UINT			Width, Height, Colors; /*  Width, Height and bpp */
	LPBYTE			lpBits;                /*  ptr to DIB bits */
	DWORD			dwNumBytes;            /*  how many bytes? */
	LPBITMAPINFO	lpbi;                          /*  ptr to header */
	LPBYTE			lpXOR;                 /*  ptr to XOR image bits */
	LPBYTE			lpAND;                 /*  ptr to AND image bits */
	HICON			hIcon;                 /*  DAS ICON */
} ICONIMAGE, *LPICONIMAGE;
/*
 * This structure is how we represent a block of the above
 * items.  We will reallocate these structures according to
 * how many images they need to contain.
 */
typedef struct {
	int		nNumImages;                      /*  How many images? */
	ICONIMAGE	IconImages[1];                   /*  Image entries */
} BlockOfIconImages, *BlockOfIconImagesPtr;
/*
 * These two structures are used to read in icons from an
 * 'icon directory' (i.e. the contents of a .icr file, say).
 * We only use these structures temporarily, since we copy
 * the information we want into a BlockOfIconImages.
 */
typedef struct {
	BYTE	bWidth;               /*  Width of the image */
	BYTE	bHeight;              /*  Height of the image (times 2) */
	BYTE	bColorCount;          /*  Number of colors in image (0 if >=8bpp) */
	BYTE	bReserved;            /*  Reserved */
	WORD	wPlanes;              /*  Color Planes */
	WORD	wBitCount;            /*  Bits per pixel */
	DWORD	dwBytesInRes;         /*  how many bytes in this resource? */
	DWORD	dwImageOffset;        /*  where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;
typedef struct {
	WORD		idReserved;   /*  Reserved */
	WORD		idType;       /*  resource type (1 for icons) */
	WORD		idCount;      /*  how many images? */
	ICONDIRENTRY	idEntries[1]; /*  the entries for each image */
} ICONDIR, *LPICONDIR;

/*
 * A pointer to one of these strucutures is associated with each
 * toplevel.  This allows us to free up all memory associated with icon
 * resources when a window is deleted or if the window's icon is
 * changed.  They are simply reference counted according to:
 *
 * (i) how many WmInfo structures point to this object
 * (ii) whether the ThreadSpecificData defined in this file contains
 * a pointer to this object.
 *
 * The former count is for windows whose icons are individually
 * set, and the latter is for the global default icon choice.
 *
 * Icons loaded from .icr/.icr use the iconBlock field, icons
 * loaded from .exe/.dll use the hIcon field.
 */
typedef struct WinIconInstance {
    int refCount;		 /* Number of instances that share this
				  * data structure. */
    BlockOfIconImagesPtr iconBlock;
                                 /* Pointer to icon resource data for
                                  * image. */
} WinIconInstance;

typedef struct WinIconInstance *WinIconPtr;

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
    LONG styleConfig;		/* Extra user requested style bits */
    LONG exStyleConfig;		/* Extra user requested extended style bits */

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
    CONST char **cmdArgv;	/* Array of strings to store in the
				 * WM_COMMAND property.  NULL means nothing
				 * available. */
    char *clientMachine;	/* String to store in WM_CLIENT_MACHINE
				 * property, or NULL. */
    int flags;			/* Miscellaneous flags, defined below. */
    int numTransients;		/* number of transients on this window */
    WinIconPtr iconPtr;         /* pointer to titlebar icon structure for
                                 * this window, or NULL. */
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
 *				propagated to window manager. Not used on Win.
 * WM_SYNC_PENDING -		set to non-zero while waiting for the window
 *				manager to respond to some state change.
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
#define WM_WITHDRAWN			(1<<12)

/*
 * Window styles for various types of toplevel windows.
 */

#define WM_OVERRIDE_STYLE (WS_POPUP|WS_CLIPCHILDREN|CS_DBLCLKS)
#define EX_OVERRIDE_STYLE (WS_EX_TOOLWINDOW)

#define WM_TOPLEVEL_STYLE (WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|CS_DBLCLKS)
#define EX_TOPLEVEL_STYLE (0)

#define WM_TRANSIENT_STYLE \
		(WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPSIBLINGS|CS_DBLCLKS)
#define EX_TRANSIENT_STYLE (WS_EX_DLGMODALFRAME)

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
    WinIconPtr iconPtr;          /* IconPtr being used as default for all
                                  * toplevels, or NULL. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The following variables cannot be placed in thread local storage
 * because they must be shared across threads.
 */

static int initialized;        /* Flag indicating whether module has
				* been initialized. */

/* 
 * A pointer to a shell proc which allows us to extract icons from
 * any file.  We just initialize this when we start up (if we can)
 * and then it never changes
 */
DWORD* (WINAPI *shgetfileinfoProc) (LPCTSTR pszPath, DWORD dwFileAttributes,
    SHFILEINFO* psfi, UINT cbFileInfo, UINT uFlags) = NULL;

TCL_DECLARE_MUTEX(winWmMutex)

/*
 * Forward declarations for procedures defined in this file:
 */

static int		ActivateWindow _ANSI_ARGS_((Tcl_Event *evPtr,
			    int flags));
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
static void		TkWmStackorderToplevelWrapperMap _ANSI_ARGS_((
			    TkWindow *winPtr,
			    Tcl_HashTable *table));
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
static void		WmWaitVisibilityOrMapProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static BlockOfIconImagesPtr   ReadIconOrCursorFromFile _ANSI_ARGS_((
			    Tcl_Interp *interp, Tcl_Obj* fileName, BOOL isIcon));
static WinIconPtr       ReadIconFromFile _ANSI_ARGS_((
			    Tcl_Interp *interp, Tcl_Obj *fileName));
static WinIconPtr       GetIconFromPixmap _ANSI_ARGS_((Display *dsPtr,
						       Pixmap pixmap));
static int     		ReadICOHeader _ANSI_ARGS_((Tcl_Channel channel));
static BOOL 		AdjustIconImagePointers _ANSI_ARGS_((LPICONIMAGE lpImage));
static HICON 		MakeIconOrCursorFromResource 
                            _ANSI_ARGS_((LPICONIMAGE lpIcon, BOOL isIcon));
static HICON 		GetIcon _ANSI_ARGS_((WinIconPtr titlebaricon, 
			    int icon_size));
static int 		WinSetIcon _ANSI_ARGS_((Tcl_Interp *interp,
			    WinIconPtr titlebaricon, Tk_Window tkw));
static void 		FreeIconBlock _ANSI_ARGS_((BlockOfIconImagesPtr lpIR));
static void  	 	DecrIconRefCount _ANSI_ARGS_((WinIconPtr titlebaricon));

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

/* Used in BytesPerLine */
#define WIDTHBYTES(bits)      ((((bits) + 31)>>5)<<2)

/*
 *----------------------------------------------------------------------
 *
 * DIBNumColors --
 *
 *	Calculates the number of entries in the color table, given by
 *	LPSTR lpbi - pointer to the CF_DIB memory block.  Used by
 *	titlebar icon code.
 *
 * Results:
 *
 *      WORD - Number of entries in the color table.
 *
 * Side effects: None.
 *
 *
 *----------------------------------------------------------------------
 */
static WORD 
DIBNumColors( LPSTR lpbi )
{
    WORD wBitCount;
    DWORD dwClrUsed;

    dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;

    if (dwClrUsed)
	return (WORD) dwClrUsed;

    wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;

    switch (wBitCount)
    {
	case 1: return 2;
	case 4: return 16;
	case 8:	return 256;
	default:return 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PaletteSize --
 *
 *	Calculates the number of bytes in the color table, as given by
 *	LPSTR lpbi - pointer to the CF_DIB memory block.  Used by
 *	titlebar icon code.
 *
 * Results:
 *	number of bytes in the color table
 *
 * Side effects: None.
 *
 *
 *----------------------------------------------------------------------
 */
static WORD 
PaletteSize( LPSTR lpbi )
{
    return ((WORD)( DIBNumColors( lpbi ) * sizeof( RGBQUAD )) );
}

/*
 *----------------------------------------------------------------------
 *
 * FindDIBits --
 *
 *	Locate the image bits in a CF_DIB format DIB, as given by
 *	LPSTR lpbi - pointer to the CF_DIB memory block.  Used by
 *	titlebar icon code.
 *
 * Results:
 *	pointer to the image bits
 *
 * Side effects: None
 *
 *
 *----------------------------------------------------------------------
 */
static LPSTR 
FindDIBBits( LPSTR lpbi )
{
   return ( lpbi + *(LPDWORD)lpbi + PaletteSize( lpbi ) );
}

/*
 *----------------------------------------------------------------------
 *
 * BytesPerLine --
 *
 *	Calculates the number of bytes in one scan line, as given by
 *	LPBITMAPINFOHEADER lpBMIH - pointer to the BITMAPINFOHEADER
 *	that begins the CF_DIB block.  Used by titlebar icon code.
 *
 * Results:
 *	number of bytes in one scan line (DWORD aligned)
 *
 * Side effects: None
 *
 *
 *----------------------------------------------------------------------
 */
static DWORD 
BytesPerLine( LPBITMAPINFOHEADER lpBMIH )
{
    return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}

/*
 *----------------------------------------------------------------------
 *
 * AdjustIconImagePointers --
 *
 *	Adjusts internal pointers in icon resource struct, as given
 *	by LPICONIMAGE lpImage - the resource to handle.  Used by
 *	titlebar icon code.
 *
 * Results:
 *	BOOL - TRUE for success, FALSE for failure
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */
static BOOL 
AdjustIconImagePointers( LPICONIMAGE lpImage )
{
    /*  Sanity check */
    if (lpImage==NULL)
	return FALSE;
    /*  BITMAPINFO is at beginning of bits */
    lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;
    /*  Width - simple enough */
    lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
    /* 
     * Icons are stored in funky format where height is doubled 
     * so account for that 
     */
    lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight)/2;
    /*  How many colors? */
    lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * 
                            lpImage->lpbi->bmiHeader.biBitCount;
    /*  XOR bits follow the header and color table */
    lpImage->lpXOR = (LPBYTE)FindDIBBits(((LPSTR)lpImage->lpbi));
    /*  AND bits follow the XOR bits */
    lpImage->lpAND = lpImage->lpXOR + (lpImage->Height*
		BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeIconOrCursorFromResource --
 *
 *	Construct an actual HICON structure from the information
 *	in a resource.
 *
 * Results:
 *
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */
static HICON 
MakeIconOrCursorFromResource(LPICONIMAGE lpIcon, BOOL isIcon) {
    HICON hIcon ;
    static FARPROC pfnCreateIconFromResourceEx=NULL;
    static int initinfo=0;
    /*  Sanity Check */
    if (lpIcon == NULL)
	return NULL;
    if (lpIcon->lpBits == NULL)
	return NULL;
    if (!initinfo) {
	HMODULE hMod = GetModuleHandleA("USER32.DLL");
	initinfo=1;
	if (hMod){
	    pfnCreateIconFromResourceEx = 
	      GetProcAddress(hMod, "CreateIconFromResourceEx");
	}
    }
    /*  Let the OS do the real work :) */
    if (pfnCreateIconFromResourceEx!=NULL) {
	hIcon = (HICON) (pfnCreateIconFromResourceEx)
	(lpIcon->lpBits, lpIcon->dwNumBytes, isIcon, 0x00030000,
	 (*(LPBITMAPINFOHEADER)(lpIcon->lpBits)).biWidth,
	 (*(LPBITMAPINFOHEADER)(lpIcon->lpBits)).biHeight/2, 0);
    } else {
	 hIcon = NULL;
    }
    /*  It failed, odds are good we're on NT so try the non-Ex way */
    if (hIcon == NULL)    {
	/*  We would break on NT if we try with a 16bpp image */
	if (lpIcon->lpbi->bmiHeader.biBitCount != 16) {
	    hIcon = CreateIconFromResource(lpIcon->lpBits, lpIcon->dwNumBytes, 
					   isIcon, 0x00030000);
	}
    }
    return hIcon;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadICOHeader --
 *
 *	Reads the header from an ICO file, as specfied by channel.
 *
 * Results:
 *	UINT - Number of images in file, -1 for failure.
 *	If this succeeds, there is a decent chance this is a
 *	valid icon file.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */
static int 
ReadICOHeader( Tcl_Channel channel )
{
    WORD    Input;
    DWORD	dwBytesRead;

    /*  Read the 'reserved' WORD */
    dwBytesRead = Tcl_Read( channel, (char*)&Input, sizeof( WORD ));
    /*  Did we get a WORD? */
    if (dwBytesRead != sizeof( WORD ))
	return -1;
    /*  Was it 'reserved' ?   (ie 0) */
    if (Input != 0)
	return -1;
    /*  Read the type WORD */
    dwBytesRead = Tcl_Read( channel, (char*)&Input, sizeof( WORD ));
    /*  Did we get a WORD? */
    if (dwBytesRead != sizeof( WORD ))
	return -1;
    /*  Was it type 1? */
    if (Input != 1)
	return -1;
    /*  Get the count of images */
    dwBytesRead = Tcl_Read( channel, (char*)&Input, sizeof( WORD ));
    /*  Did we get a WORD? */
    if (dwBytesRead != sizeof( WORD ))
	return -1;
    /*  Return the count */
    return (int)Input;
}

/*
 *----------------------------------------------------------------------
 *
 * InitWindowClass --
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
static int 
InitWindowClass(WinIconPtr titlebaricon) {
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (! tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	tsdPtr->firstWindow = 1;
	tsdPtr->iconPtr = NULL;
    }
    if (! initialized) {
	Tcl_MutexLock(&winWmMutex);
	if (! initialized) {
	    Tcl_DString classString;
	    WNDCLASS class;
	    initialized = 1;

	    if (shgetfileinfoProc == NULL) {
		HINSTANCE hInstance = LoadLibraryA("shell32");
		if (hInstance != NULL) {
		    shgetfileinfoProc = 
		    (DWORD* (WINAPI *) (LPCTSTR pszPath, DWORD dwFileAttributes,
    SHFILEINFO* psfi, UINT cbFileInfo, UINT uFlags)) GetProcAddress(hInstance, 
		      "SHGetFileInfo");
		    FreeLibrary(hInstance);
		}
	    }
	    /*
	     * The only difference between WNDCLASSW and WNDCLASSA are
	     * in pointers, so we can use the generic structure WNDCLASS.
	     */
	    ZeroMemory(&class, sizeof(WNDCLASS));

	    /*
	     * When threads are enabled, we cannot use CLASSDC because
	     * threads will then write into the same device context.
	     *
	     * This is a hack; we should add a subsystem that manages
	     * device context on a per-thread basis.  See also tkWinX.c,
	     * which also initializes a WNDCLASS structure.
	     */

#ifdef TCL_THREADS
	    class.style = CS_HREDRAW | CS_VREDRAW;
#else
	    class.style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC;
#endif
	    class.hInstance = Tk_GetHINSTANCE();
	    Tcl_WinUtfToTChar(TK_WIN_TOPLEVEL_CLASS_NAME, -1, &classString);
	    class.lpszClassName = (LPCTSTR) Tcl_DStringValue(&classString);
	    class.lpfnWndProc = WmProc;
	    if (titlebaricon == NULL) {
		class.hIcon = LoadIcon(Tk_GetHINSTANCE(), "tk");
	    } else {
		class.hIcon = GetIcon(titlebaricon, ICON_BIG);
		if (class.hIcon == NULL) {
		    return TCL_ERROR;
		}
		/*
		 * Store pointer to default icon so we know when
		 * we need to free that information
		 */
		tsdPtr->iconPtr = titlebaricon;
	    }
	    class.hCursor = LoadCursor(NULL, IDC_ARROW);

	    if (!(*tkWinProcs->registerClass)(&class)) {
		panic("Unable to register TkTopLevel class");
	    }
	    Tcl_DStringFree(&classString);
	}
	Tcl_MutexUnlock(&winWmMutex);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * InitWm --
 *
 *	This initialises the window manager
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
    /* Ignore return result */
    (void) InitWindowClass(NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * WinSetIcon --
 *
 *	Sets either the default toplevel titlebar icon, or the icon
 *	for a specific toplevel (if tkw is given, then only that
 *	window is used).
 *
 *	The ref-count of the titlebaricon is NOT changed.  If this
 *	function returns successfully, the caller should assume
 *	the icon was used (and therefore the ref-count should
 *	be adjusted to reflect that fact).  If the function returned
 *	an error, the caller should assume the icon was not used
 *	(and may wish to free the memory associated with it).
 *
 * Results:
 *	A standard Tcl return code.
 *
 * Side effects:
 *	One or all windows may have their icon changed.
 *	The Tcl result may be modified.
 *	The window-manager will be initialised if it wasn't already.
 *	The given window will be forced into existence.
 *
 *----------------------------------------------------------------------
 */
static int
WinSetIcon(interp, titlebaricon, tkw)
    Tcl_Interp *interp;
    WinIconPtr titlebaricon;
    Tk_Window tkw;
{
    WmInfo *wmPtr;
    HWND hwnd;
    int application = 0;

    if (tkw == NULL) {
        tkw = Tk_MainWindow(interp);
	application = 1;
    }

    if (!(Tk_IsTopLevel(tkw))) {
	Tcl_AppendResult(interp, "window \"", Tk_PathName(tkw),
		"\" isn't a top-level window", (char *) NULL);
	return TCL_ERROR;
    }
    if (Tk_WindowId(tkw) == None) {
	Tk_MakeWindowExist(tkw);
    }
    /* We must get the window's wrapper, not the window itself */
    wmPtr = ((TkWindow*)tkw)->wmInfoPtr;
    hwnd = wmPtr->wrapper;

    if (application) {
	if (hwnd == NULL) {
	    /*
	     * I don't actually think this is ever the correct thing, unless
	     * perhaps the window doesn't have a wrapper.  But I believe all
	     * windows have wrappers.
	     */
	    hwnd = Tk_GetHWND(Tk_WindowId(tkw));
	}
	/*
	 * If we aren't initialised, then just initialise with the user's
	 * icon.  Otherwise our icon choice will be ignored moments later
	 * when Tk finishes initialising.
	 */
	if (!initialized) {
	    if (InitWindowClass(titlebaricon) != TCL_OK) {
		Tcl_AppendResult(interp,"Unable to set icon", (char*)NULL);
		return TCL_ERROR;
	    }
	} else {
	    ThreadSpecificData *tsdPtr;
	    if (
#ifdef _WIN64
		!SetClassLongPtr(hwnd, GCLP_HICONSM,
			(LPARAM)GetIcon(titlebaricon, ICON_SMALL))
#else
		!SetClassLong(hwnd, GCL_HICONSM,
			(LPARAM)GetIcon(titlebaricon, ICON_SMALL))
#endif
		) {
		/*
		 * For some reason this triggers, even though it seems
		 * to be successful This is probably related to the
		 * WNDCLASS vs WNDCLASSEX difference.  Anyway it seems
		 * we have to ignore errors returned here.
		 */

		/*
		 * Tcl_AppendResult(interp,"Unable to set new small icon", (char*)NULL);
		 * return TCL_ERROR;
		 */
	    }
	    if (
#ifdef _WIN64
		!SetClassLongPtr(hwnd, GCLP_HICON,
			(LPARAM)GetIcon(titlebaricon, ICON_BIG))
#else
		!SetClassLong(hwnd, GCL_HICON,
			(LPARAM)GetIcon(titlebaricon, ICON_BIG))
#endif
		) {
		Tcl_AppendResult(interp,"Unable to set new icon", (char*)NULL);
		return TCL_ERROR;
	    }
	    tsdPtr = (ThreadSpecificData *)
		    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
	    if (tsdPtr->iconPtr != NULL) {
		DecrIconRefCount(tsdPtr->iconPtr);
	    }
	    tsdPtr->iconPtr = titlebaricon;
	}
    } else {
	if (!initialized) {
	    /*
	     * Need to initialise the wm otherwise we will fail on
	     * code which tries to set a toplevel's icon before that
	     * happens.  Ignore return result.
	     */
	    (void)InitWindowClass(NULL);
	}
	/*
	 * The following code is exercised if you do
	 *
	 *   toplevel .t ; wm titlebaricon .t foo.icr
	 *
	 * i.e. the wm hasn't had time to properly create
	 * the '.t' window before you set the icon.
	 */
	if (hwnd == NULL) {
	    /*
	     * This little snippet is copied from the 'Map' function,
	     * and should probably be placed in one proper location
	     */
	    UpdateWrapper(wmPtr->winPtr);
	    wmPtr = ((TkWindow*)tkw)->wmInfoPtr;
	    hwnd = wmPtr->wrapper;
	    if (hwnd == NULL) {
		Tcl_AppendResult(interp,
			"Can't set icon; window has no wrapper.", (char*)NULL);
		return TCL_ERROR;
	    }
	}
	SendMessage(hwnd, WM_SETICON, ICON_SMALL,
		(LPARAM) GetIcon(titlebaricon, ICON_SMALL));
	SendMessage(hwnd, WM_SETICON, ICON_BIG,
		(LPARAM) GetIcon(titlebaricon, ICON_BIG));

	/* Update the iconPtr we keep for each WmInfo structure. */
	if (wmPtr->iconPtr != NULL) {
	    /* Free any old icon ptr which is associated with this window. */
	    DecrIconRefCount(wmPtr->iconPtr);
	}
	/*
	 * We do not need to increment the ref count for the
	 * titlebaricon, because it was already incremented when we
	 * retrieved it.
	 */
	wmPtr->iconPtr = titlebaricon;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadIconFromFile --
 *
 *	Read the contents of a file (usually .ico, .icr) and extract an
 *	icon resource, if possible, otherwise check if the shell has an
 *	icon assigned to the given file and use that.  If both of those
 *	fail, then NULL is returned, and an error message will already be
 *	in the interpreter.
 *	
 * Results:
 *	A WinIconPtr structure containing the icons in the file, with
 *	its ref count already incremented. The calling procedure should
 *	either place this structure inside a WmInfo structure, or it should
 *	pass it on to DecrIconRefCount() to ensure no memory leaks occur.
 *
 *	If the given fileName did not contain a valid icon structure,
 *	return NULL.
 *
 * Side effects:
 *	Memory is allocated for the returned structure and the icons
 *	it contains.  If the structure is not wanted, it should be
 *	passed to DecrIconRefCount, and in any case a valid ref count
 *	should be ensured to avoid memory leaks.
 *
 *	Currently icon resources are not shared, so the ref count of
 *	one of these structures will always be 0 or 1.  However all we
 *	need do is implement some sort of lookup function between
 *	filenames and WinIconPtr structures and no other code will need
 *	to be changed.  The pseudo-code for this is implemented below
 *	in the 'if (0)' branch.  It did not seem necessary to implement
 *	this optimisation here, since moving to icon<->image
 *	conversions will probably make it obsolete.
 *
 *----------------------------------------------------------------------
 */
static WinIconPtr
ReadIconFromFile(interp, fileName)
    Tcl_Interp *interp;
    Tcl_Obj *fileName;
{
    WinIconPtr titlebaricon = NULL;

    if (0 /* If we already have an icon for this filename */) {
	titlebaricon = NULL; /* Get the real value from a lookup */
	titlebaricon->refCount++;
	return titlebaricon;
    } else {
	/* First check if it is a .ico file */
	BlockOfIconImagesPtr lpIR;
	lpIR = ReadIconOrCursorFromFile(interp, fileName, TRUE);
	
	/* Then see if we can ask the shell for the icon for this file */
	if (lpIR == NULL && shgetfileinfoProc != NULL) {
	    SHFILEINFO sfi;
	    Tcl_DString ds, ds2;
	    DWORD *res;
	    CONST char *file;
	    
	    file = Tcl_TranslateFileName(interp, Tcl_GetString(fileName), &ds);
	    if (file == NULL) { return NULL; }
	    Tcl_UtfToExternalDString(NULL, file, -1, &ds2);
	    Tcl_DStringFree(&ds);
	    
	    res = (*shgetfileinfoProc)(Tcl_DStringValue(&ds2), 0, &sfi, 
			  sizeof(SHFILEINFO), SHGFI_SMALLICON|SHGFI_ICON);
	   
	    Tcl_DStringFree(&ds2);
	    
	    if (res != 0) {
		Tcl_ResetResult(interp);
			      
		lpIR = (BlockOfIconImagesPtr) ckalloc(sizeof(BlockOfIconImages));
		if (lpIR == NULL) {
		    DestroyIcon(sfi.hIcon);
		    return NULL;
		}
	    
		lpIR->nNumImages = 1;
		lpIR->IconImages[0].Width = 16;
		lpIR->IconImages[0].Height = 16;
		lpIR->IconImages[0].Colors = 4;
		lpIR->IconImages[0].hIcon = sfi.hIcon;
		/* These fields are ignored */
		lpIR->IconImages[0].lpBits = 0;
		lpIR->IconImages[0].dwNumBytes = 0;
		lpIR->IconImages[0].lpXOR = 0;
		lpIR->IconImages[0].lpAND = 0;
	    }
	}
	if (lpIR != NULL) {
	    titlebaricon = (WinIconPtr) ckalloc(sizeof(WinIconInstance));
	    titlebaricon->iconBlock = lpIR;
	    titlebaricon->refCount = 1;
	}
	return titlebaricon;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetIconFromPixmap --
 *
 *	Turn a Tk Pixmap (i.e. a bitmap) into an icon resource, if
 *	possible, otherwise NULL is returned.
 *
 * Results:
 *	A WinIconPtr structure containing a conversion of the given
 *	bitmap into an icon, with its ref count already incremented.  The
 *	calling procedure should either place this structure inside a
 *	WmInfo structure, or it should pass it on to DecrIconRefCount()
 *	to ensure no memory leaks occur.
 *	
 *	If the given pixmap did not contain a valid icon structure,
 *	return NULL.
 *
 * Side effects:
 *	Memory is allocated for the returned structure and the icons
 *	it contains.  If the structure is not wanted, it should be
 *	passed to DecrIconRefCount, and in any case a valid ref count
 *	should be ensured to avoid memory leaks.
 *	
 *	Currently icon resources are not shared, so the ref count of
 *	one of these structures will always be 0 or 1.  However all we
 *	need do is implement some sort of lookup function between
 *	pixmaps and WinIconPtr structures and no other code will need
 *	to be changed.
 *
 *----------------------------------------------------------------------
 */
static WinIconPtr       
GetIconFromPixmap(dsPtr, pixmap)
    Display *dsPtr;
    Pixmap pixmap;
{
    WinIconPtr titlebaricon = NULL;
    TkWinDrawable* twdPtr = (TkWinDrawable*) pixmap;
    
    if (twdPtr == NULL) {
        return NULL;
    }
    
    if (0 /* If we already have an icon for this pixmap */) {
	titlebaricon = NULL; /* Get the real value from a lookup */
	titlebaricon->refCount++;
	return titlebaricon;
    } else {
	BlockOfIconImagesPtr lpIR;
	ICONINFO icon;
	HICON hIcon;
	int width, height;

	Tk_SizeOfBitmap(dsPtr, pixmap, &width, &height);
    
	icon.fIcon = TRUE;
	icon.xHotspot = 0;
	icon.yHotspot = 0;
	icon.hbmMask = twdPtr->bitmap.handle;
	icon.hbmColor = twdPtr->bitmap.handle;

	hIcon = CreateIconIndirect(&icon);
	if (hIcon == NULL) {
	    return NULL;
	}

	lpIR = (BlockOfIconImagesPtr) ckalloc(sizeof(BlockOfIconImages));
	if (lpIR == NULL) {
	    DestroyIcon(hIcon);
	    return NULL;
	}
	
	lpIR->nNumImages = 1;
	lpIR->IconImages[0].Width = width;
	lpIR->IconImages[0].Height = height;
	lpIR->IconImages[0].Colors = 1 << twdPtr->bitmap.depth;
	lpIR->IconImages[0].hIcon = hIcon;
	/* These fields are ignored */
	lpIR->IconImages[0].lpBits = 0;
	lpIR->IconImages[0].dwNumBytes = 0;
	lpIR->IconImages[0].lpXOR = 0;
	lpIR->IconImages[0].lpAND = 0;
	
	titlebaricon = (WinIconPtr) ckalloc(sizeof(WinIconInstance));
	titlebaricon->iconBlock = lpIR;
	titlebaricon->refCount = 1;
	return titlebaricon;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DecrIconRefCount --
 *
 *	Reduces the reference count.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the ref count falls to zero, free the memory associated
 *	with the icon resource structures.  In this case the pointer
 *	passed into this function is no longer valid.
 *
 *----------------------------------------------------------------------
 */
static void 
DecrIconRefCount(WinIconPtr titlebaricon) {
    titlebaricon->refCount--;

    if (titlebaricon->refCount <= 0) {
	if (titlebaricon->iconBlock != NULL) {
	    FreeIconBlock(titlebaricon->iconBlock);
	}
	titlebaricon->iconBlock = NULL;

	ckfree((char*)titlebaricon);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeIconBlock --
 *
 *	Frees all memory associated with a previously loaded
 *	titlebaricon.  The icon block pointer is no longer
 *	valid once this function returns.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */
static void 
FreeIconBlock(BlockOfIconImagesPtr lpIR) {
    int i;

    /* Free all the bits */
    for (i=0; i< lpIR->nNumImages; i++) {
	if (lpIR->IconImages[i].lpBits != NULL) {
	    ckfree((char*)lpIR->IconImages[i].lpBits);
	}
	if (lpIR->IconImages[i].hIcon != NULL) {
	    DestroyIcon(lpIR->IconImages[i].hIcon);
	}
    }
    ckfree ((char*)lpIR);
}

/*
 *----------------------------------------------------------------------
 *
 * GetIcon --
 *
 *	Extracts an icon of a given size from an icon resource
 *
 * Results:
 *	Returns the icon, if found, else NULL.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */
static HICON 
GetIcon(WinIconPtr titlebaricon, int icon_size) {
    BlockOfIconImagesPtr lpIR;
    
    if (titlebaricon == NULL) {
        return NULL;
    }

    lpIR = titlebaricon->iconBlock;
    if (lpIR == NULL) {
	return NULL;
    } else {
	unsigned int size = (icon_size == 0 ? 16 : 32);
	int i;

	for (i = 0; i < lpIR->nNumImages; i++) {
	    /* Take the first or a 32x32 16 color icon*/
	    if ((lpIR->IconImages[i].Height == size)
	       && (lpIR->IconImages[i].Width == size)
	       && (lpIR->IconImages[i].Colors >= 4)) {
		return lpIR->IconImages[i].hIcon;
	    }
	}

	/* 
	 * If we get here, then just return the first one, 
	 * it will have to do!
	 */
	if (lpIR->nNumImages >= 1) {
	    return lpIR->IconImages[0].hIcon;
	}
    }
    return NULL;
}

static HCURSOR 
TclWinReadCursorFromFile(Tcl_Interp* interp, Tcl_Obj* fileName) {
    BlockOfIconImagesPtr lpIR;
    HICON res = NULL;
    
    lpIR = ReadIconOrCursorFromFile(interp, fileName, FALSE);
    if (lpIR == NULL) {
        return NULL;
    }
    if (lpIR->nNumImages >= 1) {
	res = CopyImage(lpIR->IconImages[0].hIcon, IMAGE_CURSOR,0,0,0);
    }
    FreeIconBlock(lpIR);
    return res;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadIconOrCursorFromFile --
 *
 *	Reads an Icon Resource from an ICO file, as given by
 *	char* fileName - Name of the ICO file. This name should
 *	be in Utf format.
 *
 * Results:
 *	Returns an icon resource, if found, else NULL.
 *
 * Side effects:
 *      May leave error messages in the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */
static BlockOfIconImagesPtr 
ReadIconOrCursorFromFile(Tcl_Interp* interp, Tcl_Obj* fileName, BOOL isIcon) {
    BlockOfIconImagesPtr lpIR, lpNew;
    Tcl_Channel          channel;
    int                  i;
    DWORD            	 dwBytesRead;
    LPICONDIRENTRY    	 lpIDE;

    /*  Open the file */
    channel = Tcl_FSOpenFileChannel(interp, fileName, "r", 0);
    if (channel == NULL) {
	Tcl_AppendResult(interp,"Error opening file \"", 
			 Tcl_GetString(fileName), 
	                 "\" for reading",(char*)NULL);
	return NULL;
    }
    if (Tcl_SetChannelOption(interp, channel, "-translation", "binary")
	    != TCL_OK) {
	Tcl_Close(NULL, channel);
	return NULL;
    }
    if (Tcl_SetChannelOption(interp, channel, "-encoding", "binary")
	    != TCL_OK) {
	Tcl_Close(NULL, channel);
	return NULL;
    }
    /*  Allocate memory for the resource structure */
    lpIR = (BlockOfIconImagesPtr) ckalloc(sizeof(BlockOfIconImages));
    if (lpIR == NULL)    {
	Tcl_AppendResult(interp,"Error allocating memory",(char*)NULL);
	Tcl_Close(NULL, channel);
	return NULL;
    }
    /*  Read in the header */
    if ((lpIR->nNumImages = ReadICOHeader( channel )) == -1)    {
	Tcl_AppendResult(interp,"Invalid file header",(char*)NULL);
	Tcl_Close(NULL, channel);
	ckfree((char*) lpIR );
	return NULL;
    }
    /*  Adjust the size of the struct to account for the images */
    lpNew = (BlockOfIconImagesPtr) ckrealloc((char*)lpIR, 
	sizeof(BlockOfIconImages) + ((lpIR->nNumImages-1) * sizeof(ICONIMAGE)));
    if (lpNew == NULL) {
	Tcl_AppendResult(interp,"Error allocating memory",(char*)NULL);
	Tcl_Close(NULL, channel);
	ckfree( (char*)lpIR );
	return NULL;
    }
    lpIR = lpNew;
    /*  Allocate enough memory for the icon directory entries */
    lpIDE = (LPICONDIRENTRY) ckalloc(lpIR->nNumImages * sizeof(ICONDIRENTRY));
    if (lpIDE == NULL) {
	Tcl_AppendResult(interp,"Error allocating memory",(char*)NULL);
	Tcl_Close(NULL, channel);
	ckfree( (char*)lpIR );
	return NULL;
    }
    /*  Read in the icon directory entries */
    dwBytesRead = Tcl_Read(channel, (char*)lpIDE, 
			   lpIR->nNumImages * sizeof( ICONDIRENTRY ));
    if (dwBytesRead != lpIR->nNumImages * sizeof( ICONDIRENTRY ))    {
	Tcl_AppendResult(interp,"Error reading file",(char*)NULL);
	Tcl_Close(NULL, channel);
	ckfree( (char*)lpIR );
	return NULL;
    }
    /*  Loop through and read in each image */
    for( i = 0; i < lpIR->nNumImages; i++ )    {
	/*  Allocate memory for the resource */
	lpIR->IconImages[i].lpBits = (LPBYTE) ckalloc(lpIDE[i].dwBytesInRes);
	if (lpIR->IconImages[i].lpBits == NULL) {
	    Tcl_AppendResult(interp,"Error allocating memory",(char*)NULL);
	    Tcl_Close(NULL, channel);
	    ckfree( (char*)lpIR );
	    ckfree( (char*)lpIDE );
	    return NULL;
	}
	lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;
	/*  Seek to beginning of this image */
	if (Tcl_Seek(channel, lpIDE[i].dwImageOffset, FILE_BEGIN) == -1) {
	    Tcl_AppendResult(interp,"Error seeking in file",(char*)NULL);
	    Tcl_Close(NULL, channel);
	    ckfree( (char*)lpIR );
	    ckfree( (char*)lpIDE );
	    return NULL;
	}
	/*  Read it in */
	dwBytesRead = Tcl_Read( channel, lpIR->IconImages[i].lpBits, 
			       lpIDE[i].dwBytesInRes);
	if (dwBytesRead != lpIDE[i].dwBytesInRes) {
	    Tcl_AppendResult(interp,"Error reading file",(char*)NULL);
	    Tcl_Close(NULL, channel);
	    ckfree( (char*)lpIDE );
	    ckfree( (char*)lpIR );
	    return NULL;
	}
	/*  Set the internal pointers appropriately */
	if (!AdjustIconImagePointers( &(lpIR->IconImages[i]))) {
	    Tcl_AppendResult(interp,"Error converting to internal format",
			     (char*)NULL);
	    Tcl_Close(NULL, channel);
	    ckfree( (char*)lpIDE );
	    ckfree( (char*)lpIR );
	    return NULL;
	}
	lpIR->IconImages[i].hIcon =
	    MakeIconOrCursorFromResource(&(lpIR->IconImages[i]), isIcon);
    }
    /*  Clean up */
    ckfree((char*)lpIDE);
    Tcl_Close(NULL, channel);
    if (lpIR == NULL){
	Tcl_AppendResult(interp,"Reading of ", Tcl_GetString(fileName),
	" failed!",(char*)NULL);
	return NULL;
    }
    return lpIR;
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
#ifdef _WIN64
    return (TkWindow *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
    return (TkWindow *) GetWindowLong(hwnd, GWL_USERDATA);
#endif
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

    if (!initialized) {
	return;
    }
    initialized = 0;

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

    /*
     * Initialize full structure, then set what isn't NULL
     */
    ZeroMemory(wmPtr, sizeof(WmInfo));
    winPtr->wmInfoPtr = wmPtr;
    wmPtr->winPtr = winPtr;
    wmPtr->hints.flags = InputHint | StateHint;
    wmPtr->hints.input = True;
    wmPtr->hints.initial_state = NormalState;
    wmPtr->hints.icon_pixmap = None;
    wmPtr->hints.icon_window = None;
    wmPtr->hints.icon_x = wmPtr->hints.icon_y = 0;
    wmPtr->hints.icon_mask = None;
    wmPtr->hints.window_group = None;

    /*
     * Default the maximum dimensions to the size of the display.
     */

    wmPtr->defMinWidth = wmPtr->defMinHeight = 0;
    wmPtr->defMaxWidth = DisplayWidth(winPtr->display, winPtr->screenNum);
    wmPtr->defMaxHeight = DisplayHeight(winPtr->display, winPtr->screenNum);
    wmPtr->minWidth = wmPtr->minHeight = 1;
    wmPtr->maxWidth = wmPtr->maxHeight = 0;
    wmPtr->widthInc = wmPtr->heightInc = 1;
    wmPtr->minAspect.x = wmPtr->minAspect.y = 1;
    wmPtr->maxAspect.x = wmPtr->maxAspect.y = 1;
    wmPtr->reqGridWidth = wmPtr->reqGridHeight = -1;
    wmPtr->gravity = NorthWestGravity;
    wmPtr->width = -1;
    wmPtr->height = -1;
    wmPtr->x = winPtr->changes.x;
    wmPtr->y = winPtr->changes.y;

    wmPtr->configWidth = -1;
    wmPtr->configHeight = -1;
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
    HWND parentHWND, oldWrapper;
    HWND child;
    int x, y, width, height, state;
    WINDOWPLACEMENT place;
    HICON hSmallIcon = NULL;
    HICON hBigIcon = NULL;
    Tcl_DString titleString, classString;
    int *childStateInfo = NULL;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (winPtr->window == None) {
	/*
	 * Ensure existence of the window to update the wrapper for.
	 */
	Tk_MakeWindowExist((Tk_Window) winPtr);
    }

    child = TkWinGetHWND(winPtr->window);
    parentHWND = NULL;

    if (winPtr->flags & TK_EMBEDDED) {
	wmPtr->wrapper = (HWND) winPtr->privatePtr;
	if (wmPtr->wrapper == NULL) {
	    panic("UpdateWrapper: Cannot find container window");
	}
	if (!IsWindow(wmPtr->wrapper)) {
	    panic("UpdateWrapper: Container was destroyed");
	}

    } else {
	/*
	 * Pick the decorative frame style.  Override redirect windows get
	 * created as undecorated popups.  Transient windows get a modal
	 * dialog frame.  Neither override, nor transient windows appear in
	 * the Windows taskbar.  Note that a transient window does not resize
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

	wmPtr->style   |= wmPtr->styleConfig;
	wmPtr->exStyle |= wmPtr->exStyleConfig;

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
	Tcl_WinUtfToTChar(((wmPtr->title != NULL) ?
                           wmPtr->title : winPtr->nameUid), -1, &titleString);
	Tcl_WinUtfToTChar(TK_WIN_TOPLEVEL_CLASS_NAME, -1, &classString);
	wmPtr->wrapper = (*tkWinProcs->createWindowEx)(wmPtr->exStyle,
		(LPCTSTR) Tcl_DStringValue(&classString),
		(LPCTSTR) Tcl_DStringValue(&titleString),
		wmPtr->style, x, y, width, height,
		parentHWND, NULL, Tk_GetHINSTANCE(), NULL);
	Tcl_DStringFree(&classString);
	Tcl_DStringFree(&titleString);
#ifdef _WIN64
	SetWindowLongPtr(wmPtr->wrapper, GWLP_USERDATA, (LONG_PTR) winPtr);
#else
	SetWindowLong(wmPtr->wrapper, GWL_USERDATA, (LONG) winPtr);
#endif
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

#ifdef _WIN64
    SetWindowLongPtr(child, GWL_STYLE,
	    WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
#else
    SetWindowLong(child, GWL_STYLE,
	    WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
#endif
    if (winPtr->flags & TK_EMBEDDED) {
#ifdef _WIN64
	SetWindowLongPtr(child, GWLP_WNDPROC, (LONG_PTR) TopLevelProc);
#else
	SetWindowLong(child, GWL_WNDPROC, (LONG) TopLevelProc);
#endif
    }
    oldWrapper = SetParent(child, wmPtr->wrapper);
    if (oldWrapper) {
	hSmallIcon = (HICON) SendMessage(oldWrapper, WM_GETICON, ICON_SMALL,
		(LPARAM) NULL);
	hBigIcon = (HICON) SendMessage(oldWrapper, WM_GETICON, ICON_BIG,
		(LPARAM) NULL);
    }

    if (oldWrapper && (oldWrapper != wmPtr->wrapper)
	    && (oldWrapper != GetDesktopWindow())) {
#ifdef _WIN64
	SetWindowLongPtr(oldWrapper, GWLP_USERDATA, (LONG) NULL);
#else
	SetWindowLong(oldWrapper, GWL_USERDATA, (LONG) NULL);
#endif

	if (wmPtr->numTransients > 0) {
	    /*
	     * Unset the current wrapper as the parent for all transient
	     * children for whom this is the master
	     */
	    WmInfo *wmPtr2;

	    childStateInfo = (int *)ckalloc((unsigned) wmPtr->numTransients
		* sizeof(int));
	    state = 0;
	    for (wmPtr2 = winPtr->dispPtr->firstWmPtr; wmPtr2 != NULL;
		 wmPtr2 = wmPtr2->nextPtr) {
		if (wmPtr2->masterPtr == winPtr) {
		    if (!(wmPtr2->flags & WM_NEVER_MAPPED)) {
			childStateInfo[state++] = wmPtr2->hints.initial_state;
			SetParent(TkWinGetHWND(wmPtr2->winPtr->window), NULL);
		    }
		}
	    }
	}
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

    if (hSmallIcon != NULL) {
	SendMessage(wmPtr->wrapper,WM_SETICON,ICON_SMALL,(LPARAM)hSmallIcon);
    }
    if (hBigIcon != NULL) {
	SendMessage(wmPtr->wrapper,WM_SETICON,ICON_BIG,(LPARAM)hBigIcon);
    }

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

    if (childStateInfo) {
	if (wmPtr->numTransients > 0) {
	    /*
	     * Reset all transient children for whom this is the master
	     */
	    WmInfo *wmPtr2;

	    state = 0;
	    for (wmPtr2 = winPtr->dispPtr->firstWmPtr; wmPtr2 != NULL;
		 wmPtr2 = wmPtr2->nextPtr) {
		if (wmPtr2->masterPtr == winPtr) {
		    if (!(wmPtr2->flags & WM_NEVER_MAPPED)) {
			UpdateWrapper(wmPtr2->winPtr);
			TkpWmSetState(wmPtr2->winPtr, childStateInfo[state++]);
		    }
		}
	    }
	}

	ckfree((char *) childStateInfo);
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

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	/*
	 * Don't map a transient if the master is not mapped.
	 */

	if (wmPtr->masterPtr != NULL &&
	        !Tk_IsMapped(wmPtr->masterPtr)) {
	    wmPtr->hints.initial_state = WithdrawnState;
	    return;
	}
    } else {
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
	for (prevPtr = winPtr->dispPtr->firstWmPtr; ;
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

    /*
     * Reset all transient windows whose master is the dead window.
     */

    for (wmPtr2 = winPtr->dispPtr->firstWmPtr; wmPtr2 != NULL;
	 wmPtr2 = wmPtr2->nextPtr) {
	if (wmPtr2->masterPtr == winPtr) {
	    wmPtr->numTransients--;
	    Tk_DeleteEventHandler((Tk_Window) wmPtr2->masterPtr,
	            VisibilityChangeMask|StructureNotifyMask,
	            WmWaitVisibilityOrMapProc, (ClientData) wmPtr2->winPtr);
	    wmPtr2->masterPtr = NULL;
	    if ((wmPtr2->wrapper != None)
		    && !(wmPtr2->flags & (WM_NEVER_MAPPED))) {
		UpdateWrapper(wmPtr2->winPtr);
	    }
	}
    }
    if (wmPtr->numTransients != 0)
        panic("numTransients should be 0");

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
		VisibilityChangeMask|StructureNotifyMask,
		WmWaitVisibilityOrMapProc, (ClientData) winPtr);
	wmPtr->masterPtr = NULL;
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
    if (wmPtr->iconPtr != NULL) {
	/*
	 * This may delete the icon resource data.  I believe we
	 * should do this after destroying the decorative frame,
	 * because the decorative frame is using this icon.
	 */
        DecrIconRefCount(wmPtr->iconPtr);
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
    int index, length;
    char *argv1;
    TkWindow *winPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objc < 2) {
	wrongNumArgs:
	Tcl_WrongNumArgs(interp, 1, objv, "option window ?arg ...?");
	return TCL_ERROR;
    }

    argv1 = Tcl_GetStringFromObj(objv[1], &length);
    if ((argv1[0] == 't') && (strncmp(argv1, "tracing", length) == 0)
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
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    LONG style, exStyle, styleBit, *stylePtr;
    char buf[TCL_INTEGER_SPACE], *string;
    int i, boolean, length;

    if (objc < 3) {
        configArgs:
	Tcl_WrongNumArgs(interp, 2, objv,
		"window"
		" ?-disabled ?bool??"
		" ?-toolwindow ?bool??"
		" ?-topmost ?bool??");
	return TCL_ERROR;
    }
    exStyle = wmPtr->exStyleConfig;
    style   = wmPtr->styleConfig;
    if (objc == 3) {
	sprintf(buf, "%d", ((style & WS_DISABLED) != 0));
	Tcl_AppendResult(interp, "-disabled ", buf, (char *) NULL);
	sprintf(buf, "%d", ((exStyle & WS_EX_TOOLWINDOW) != 0));
	Tcl_AppendResult(interp, " -toolwindow ", buf, (char *) NULL);
	sprintf(buf, "%d", ((exStyle & WS_EX_TOPMOST) != 0));
	Tcl_AppendResult(interp, " -topmost ", buf, (char *) NULL);
	return TCL_OK;
    }
    for (i = 3; i < objc; i += 2) {
	string = Tcl_GetStringFromObj(objv[i], &length);
	if ((length < 2) || (string[0] != '-')) {
	    goto configArgs;
	}
	if ((i < objc-1) &&
		(Tcl_GetBooleanFromObj(interp, objv[i+1], &boolean) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (strncmp(string, "-disabled", length) == 0) {
	    stylePtr = &style;
	    styleBit = WS_DISABLED;
	} else if ((strncmp(string, "-toolwindow", length) == 0)
		   && (length >= 3)) {
	    stylePtr = &exStyle;
	    styleBit = WS_EX_TOOLWINDOW;
	} else if ((strncmp(string, "-topmost", length) == 0)
		   && (length >= 3)) {
	    stylePtr = &exStyle;
	    styleBit = WS_EX_TOPMOST;
	    if ((i < objc-1) && (winPtr->flags & TK_EMBEDDED)) {
		Tcl_AppendResult(interp, "can't set topmost flag on ",
			winPtr->pathName, ": it is an embedded window",
			(char *) NULL);
		return TCL_ERROR;
	    }
	} else {
	    goto configArgs;
	}
	if (i == objc-1) {
	    Tcl_SetIntObj(Tcl_GetObjResult(interp),
		    ((*stylePtr & styleBit) != 0));
	} else if (boolean) {
	    *stylePtr |= styleBit;
	} else {
	    *stylePtr &= ~styleBit;
	}
    }
    if ((wmPtr->styleConfig != style) ||
	    (wmPtr->exStyleConfig != exStyle)) {
	wmPtr->styleConfig = style;
	wmPtr->exStyleConfig = exStyle;
	UpdateWrapper(winPtr);
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
	    ckalloc((unsigned) (length + 1));
    strcpy(wmPtr->clientMachine, argv3);
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	XTextProperty textProp;
	if (XStringListToTextProperty(&wmPtr->clientMachine, 1, &textProp)
		!= 0) {
	    XSetWMClientMachine(winPtr->display, winPtr->window,
		    &textProp);
	    XFree((char *) textProp.value);
	}
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
    TkWindow **cmapList;
    TkWindow *winPtr2;
    int i, windowObjc, gotToplevel;
    Tcl_Obj **windowObjv;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?windowList?");
	return TCL_ERROR;
    }
    if (objc == 3) {
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
    if (Tcl_ListObjGetElements(interp, objv[3], &windowObjc, &windowObjv)
	    != TCL_OK) {
	return TCL_ERROR;
    }
    cmapList = (TkWindow **) ckalloc((unsigned)
	    ((windowObjc+1)*sizeof(TkWindow*)));
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
	cmapList[i] = winPtr2;
    }
    if (!gotToplevel) {
	wmPtr->flags |= WM_ADDED_TOPLEVEL_COLORMAP;
	cmapList[windowObjc] = winPtr;
	windowObjc++;
    } else {
	wmPtr->flags &= ~WM_ADDED_TOPLEVEL_COLORMAP;
    }
    wmPtr->flags |= WM_COLORMAPS_EXPLICIT;
    if (wmPtr->cmapList != NULL) {
	ckfree((char *)wmPtr->cmapList);
    }
    wmPtr->cmapList = cmapList;
    wmPtr->cmapCount = windowObjc;

    /*
     * Now we need to force the updated colormaps to be installed.
     */

    if (wmPtr == winPtr->dispPtr->foregroundWmPtr) {
	InstallColormaps(wmPtr->wrapper, WM_QUERYNEWPALETTE, 1);
    } else {
	InstallColormaps(wmPtr->wrapper, WM_PALETTECHANGED, 0);
    }
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
		XDeleteProperty(winPtr->display, winPtr->window,
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
	XSetCommand(winPtr->display, winPtr->window, cmdArgv, cmdArgc);
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

    /*
     * If WM_UPDATE_PENDING is true, a pending UpdateGeometryInfo may
     * need to be called first to update a withdrawn toplevel's geometry
     * before it is deiconified by TkpWmSetState.
     * Don't bother if we've never been mapped.
     */
    if ((wmPtr->flags & WM_UPDATE_PENDING) &&
	    !(wmPtr->flags & WM_NEVER_MAPPED)) {
	Tcl_CancelIdleCall(UpdateGeometryInfo, (ClientData) winPtr);
	UpdateGeometryInfo((ClientData) winPtr);
    }

    /*
     * If we were in the ZoomState (maximized), 'wm deiconify'
     * should not cause the window to shrink
     */
    if (wmPtr->hints.initial_state == ZoomState) {
	TkpWmSetState(winPtr, ZoomState);
    } else {
	TkpWmSetState(winPtr, NormalState);
    }

    /*
     * An unmapped window will be mapped at idle time
     * by a call to MapFrame. That calls CreateWrapper
     * which sets the focus and raises the window.
     */
    if (wmPtr->flags & WM_NEVER_MAPPED) {
	return TCL_OK;
    }

    /*
     * Follow Windows-like style here, raising the window to the top.
     */
    TkWmRestackToplevel(winPtr, Above, NULL);
    if (!(Tk_Attributes((Tk_Window) winPtr)->override_redirect)) {
	TkSetFocusWin(winPtr, 1);
    }
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
    HWND hwnd;
    char buf[TCL_INTEGER_SPACE];

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
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
	Tk_MakeWindowExist(tkwin2);
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	wmPtr->hints.window_group = Tk_WindowId(tkwin2);
	wmPtr->hints.flags |= WindowGroupHint;
	wmPtr->leaderName = ckalloc((unsigned) (length + 1));
	strcpy(wmPtr->leaderName, argv3);
    }
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
    /* If true, then set for all windows. */
    int isDefault = 0;
    char *string;

    if ((objc < 3) || (objc > 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?-default? ?image?");
	return TCL_ERROR;
    } else if (objc == 5) {
	/* If we have 5 arguments, we must have a '-default' flag */
	char *argv3 = Tcl_GetString(objv[3]);
	if (strcmp(argv3, "-default")) {
	    Tcl_AppendResult(interp, "illegal option \"",
		    argv3, "\" must be \"-default\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	isDefault = 1;
    } else if (objc == 3) {
	/* No arguments were given */
	if (wmPtr->hints.flags & IconPixmapHint) {
	    Tcl_SetResult(interp, (char *)
		    Tk_NameOfBitmap(winPtr->display, wmPtr->hints.icon_pixmap),
		    TCL_STATIC);
	}
	return TCL_OK;
    }

    string = Tcl_GetString(objv[objc-1]);
    if (*string == '\0') {
	if (wmPtr->hints.icon_pixmap != None) {
	    Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_pixmap);
	    wmPtr->hints.icon_pixmap = None;
	}
	wmPtr->hints.flags &= ~IconPixmapHint;
	if (WinSetIcon(interp, NULL, 
		       (isDefault ? NULL : (Tk_Window) winPtr)) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	/*
	 * In the future this block of code will use Tk's 'image'
	 * functionality to allow all supported image formats.
	 * However, this will require a change to the way icons are
	 * handled.  We will need to add icon<->image conversions
	 * routines.
	 *
	 * Until that happens we simply try to find an icon in the
	 * given argument, and if that fails, we use the older
	 * bitmap code.  We do things this way round (icon then
	 * bitmap), because the bitmap code actually seems to have
	 * no visible effect, so we want to give the icon code the
	 * first try at doing something.
	 */

	/*
	 * Either return NULL, or return a valid titlebaricon with its
	 * ref count already incremented.
	 */
	WinIconPtr titlebaricon = ReadIconFromFile(interp, objv[objc-1]);
	if (titlebaricon != NULL) {
	    /*
	     * Try to set the icon for the window.  If it is a '-default'
	     * icon, we must pass in NULL
	     */
	    if (WinSetIcon(interp, titlebaricon,
		    (isDefault ? NULL : (Tk_Window) winPtr)) != TCL_OK) {
		/* We didn't use the titlebaricon after all */
		DecrIconRefCount(titlebaricon);
		titlebaricon = NULL;
	    }
	}
	if (titlebaricon == NULL) {
	    /*
	     * We didn't manage to handle the argument as a valid
	     * icon.  Try as a bitmap.  First we must clear the
	     * error message which was placed in the interpreter
	     */
	    Pixmap pixmap;
	    Tcl_ResetResult(interp);
	    pixmap = Tk_GetBitmap(interp, (Tk_Window) winPtr, string);
	    if (pixmap == None) {
		return TCL_ERROR;
	    }
	    wmPtr->hints.icon_pixmap = pixmap;
	    wmPtr->hints.flags |= IconPixmapHint;
	    titlebaricon = GetIconFromPixmap(Tk_Display(winPtr), pixmap);
	    if (titlebaricon != NULL) {
		if (WinSetIcon(interp, titlebaricon, 
			(isDefault ? NULL : (Tk_Window) winPtr)) != TCL_OK) {
		    /* We didn't use the titlebaricon after all */
		    DecrIconRefCount(titlebaricon);
		    titlebaricon = NULL;
		}
	    }
	}
    }
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
    TkpWmSetState(winPtr, IconicState);
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
	    XSetIconName(winPtr->display, winPtr->window, wmPtr->iconName);
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

	GetMinSize(wmPtr, &width, &height);
	sprintf(buf, "%d %d", width, height);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[4], &height) != TCL_OK)) {
	return TCL_ERROR;
    }
    wmPtr->minWidth = width;
    wmPtr->minHeight = height;
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
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
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
	if (!(wmPtr->flags & (WM_NEVER_MAPPED)
		&& !(winPtr->flags & TK_EMBEDDED))) {
	    UpdateWrapper(winPtr);
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
    if (!((wmPtr->flags & WM_NEVER_MAPPED)
	    && !(winPtr->flags & TK_EMBEDDED))) {
	UpdateWrapper(winPtr);
    }
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
	"normal", "iconic", "withdrawn", "zoomed", (char *) NULL };
    enum options {
	OPT_NORMAL, OPT_ICONIC, OPT_WITHDRAWN, OPT_ZOOMED };
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
	if (winPtr->flags & TK_EMBEDDED) {
	    Tcl_AppendResult(interp, "can't change state of ",
		    winPtr->pathName, ": it is an embedded window",
		    (char *) NULL);
	    return TCL_ERROR;
	}

	if (Tcl_GetIndexFromObj(interp, objv[3], optionStrings, "argument", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (index == OPT_NORMAL) {
	    wmPtr->flags &= ~WM_WITHDRAWN;
	    TkpWmSetState(winPtr, NormalState);
	    /*
	     * This varies from 'wm deiconify' because it does not
	     * force the window to be raised and receive focus
	     */
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
	    TkpWmSetState(winPtr, IconicState);
	} else if (index == OPT_WITHDRAWN) {
	    wmPtr->flags |= WM_WITHDRAWN;
	    TkpWmSetState(winPtr, WithdrawnState);
	} else { /* OPT_ZOOMED */
	    TkpWmSetState(winPtr, ZoomState);
	}
    } else {
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

	if (!(wmPtr->flags & WM_NEVER_MAPPED) && wmPtr->wrapper != NULL) {
	    Tcl_DString titleString;
	    Tcl_WinUtfToTChar(wmPtr->title, -1, &titleString);
	    (*tkWinProcs->setWindowText)(wmPtr->wrapper,
		    (LPCTSTR) Tcl_DStringValue(&titleString));
	    Tcl_DStringFree(&titleString);
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
		    VisibilityChangeMask|StructureNotifyMask,
		    WmWaitVisibilityOrMapProc, (ClientData) winPtr);
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
			VisibilityChangeMask|StructureNotifyMask,
			WmWaitVisibilityOrMapProc, (ClientData) winPtr);
	    }

	    masterPtr->wmInfoPtr->numTransients++;
	    Tk_CreateEventHandler((Tk_Window) masterPtr,
		    VisibilityChangeMask|StructureNotifyMask,
		    WmWaitVisibilityOrMapProc, (ClientData) winPtr);

	    wmPtr->masterPtr = masterPtr;
	}
    }
    if (!((wmPtr->flags & WM_NEVER_MAPPED)
	    && !(winPtr->flags & TK_EMBEDDED))) {
	if (wmPtr->masterPtr != NULL &&
		!Tk_IsMapped(wmPtr->masterPtr)) {
	    TkpWmSetState(winPtr, WithdrawnState);
	} else {
	    UpdateWrapper(winPtr);
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
    TkpWmSetState(winPtr, WithdrawnState);
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

	/*ARGSUSED*/
static void
WmWaitVisibilityOrMapProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to window. */
    XEvent *eventPtr;		/* Information about event. */
{
    TkWindow *winPtr = (TkWindow *) clientData;
    TkWindow *masterPtr = winPtr->wmInfoPtr->masterPtr;

    if (masterPtr == NULL)
	return;

    if (eventPtr->type == MapNotify) {
	if (!(winPtr->wmInfoPtr->flags & WM_WITHDRAWN))
	    TkpWmSetState(winPtr, NormalState);
    } else if (eventPtr->type == UnmapNotify) {
	TkpWmSetState(winPtr, WithdrawnState);
    }

    if (eventPtr->type == VisibilityNotify) {
	int state = masterPtr->wmInfoPtr->hints.initial_state;

	if ((state == NormalState) || (state == ZoomState)) {
	    state = winPtr->wmInfoPtr->hints.initial_state;
	    if ((state == NormalState) || (state == ZoomState)) {
		UpdateWrapper(winPtr);
	    }
	}
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
    if ((winPtr->flags & TK_EMBEDDED) && (wmPtr->wrapper != NULL)) {
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
	    /*
	     * Cache atom name, as we might destroy the window as a
	     * result of the eval.
	     */
	    CONST char *name = Tk_GetAtomName((Tk_Window) winPtr, protocol);

	    Tcl_Preserve((ClientData) protPtr);
            interp = protPtr->interp;
            Tcl_Preserve((ClientData) interp);
	    result = Tcl_GlobalEval(interp, protPtr->command);
	    if (result != TCL_OK) {
		Tcl_AddErrorInfo(interp, "\n    (command for \"");
		Tcl_AddErrorInfo(interp, name);
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
 * TkWmStackorderToplevelEnumProc --
 *
 *	This procedure is invoked once for each HWND Window on the
 *	display as a result of calling EnumWindows from
 *	TkWmStackorderToplevel.
 *
 * Results:
 *	TRUE to request further iteration.
 *
 * Side effects:
 *	Adds entries to the passed array of TkWindows.
 *
 *----------------------------------------------------------------------
 */

BOOL CALLBACK TkWmStackorderToplevelEnumProc(hwnd, lParam)
    HWND hwnd;     /* handle to parent window */
    LPARAM lParam; /* application-defined value */
{
    Tcl_HashEntry *hPtr;
    TkWindow *childWinPtr;

    TkWmStackorderToplevelPair *pair =
        (TkWmStackorderToplevelPair *) lParam;

    /*fprintf(stderr, "Looking up HWND %d\n", hwnd);*/

    hPtr = Tcl_FindHashEntry(pair->table, (char *) hwnd);
    if (hPtr != NULL) {
        childWinPtr = (TkWindow *) Tcl_GetHashValue(hPtr);
        /* Double check that same HWND does not get passed twice */
        if (childWinPtr == NULL) {
            panic("duplicate HWND in TkWmStackorderToplevelEnumProc");
        } else {
            Tcl_SetHashValue(hPtr, NULL);
        }
        /*fprintf(stderr, "Found mapped HWND %d -> %x (%s)\n", hwnd,
	  childWinPtr, childWinPtr->pathName);*/
        *(pair->window_ptr)-- = childWinPtr;
    }
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmStackorderToplevelWrapperMap --
 *
 *	This procedure will create a table that maps the wrapper
 *	HWND id for a toplevel to the TkWindow structure that is wraps.
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
    Tcl_HashTable *table;		/* Table to maps HWND to TkWindow */
{
    TkWindow *childPtr;
    Tcl_HashEntry *hPtr;
    HWND wrapper;
    int newEntry;

    if (Tk_IsMapped(winPtr) && Tk_IsTopLevel(winPtr) &&
            !Tk_IsEmbedded(winPtr)) {
        wrapper = TkWinGetWrapperWindow((Tk_Window) winPtr);

        /*fprintf(stderr, "Mapped HWND %d to %x (%s)\n", wrapper,
	  winPtr, winPtr->pathName);*/

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
    TkWmStackorderToplevelPair pair;
    TkWindow **windows;
    Tcl_HashTable table;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    /*
     * Map HWND ids to a TkWindow of the wrapped toplevel.
     */

    Tcl_InitHashTable(&table, TCL_ONE_WORD_KEYS);
    TkWmStackorderToplevelWrapperMap(parentPtr, &table);

    windows = (TkWindow **) ckalloc((table.numEntries+1)
        * sizeof(TkWindow *));

    /*
     * Special cases: If zero or one toplevels were mapped
     * there is no need to call EnumWindows.
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

    /*
     * We will be inserting into the array starting at the end
     * and working our way to the beginning since EnumWindows
     * returns windows in highest to lowest order.
     */

    pair.table = &table;
    pair.window_ptr = windows + table.numEntries;
    *pair.window_ptr-- = NULL;

    if (EnumWindows((WNDENUMPROC) TkWmStackorderToplevelEnumProc,
	    (LPARAM) &pair) == 0) {
        ckfree((char *) windows);
        windows = NULL;
    } else {
        if (pair.window_ptr != (windows-1))
            panic("num matched toplevel windows does not equal num children");
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

    if (topPtr->wmInfoPtr == NULL) {
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
		    Tcl_DoWhenIdle(UpdateGeometryInfo, (ClientData) winPtr);
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

	if (!Tk_TopWinHierarchy(childPtr) && Tk_IsMapped(childPtr)) {
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
    static int oldMode;	/* This static is set upon entering move/size mode
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

	    if (winPtr && (TkGrabState(winPtr) != TK_GRAB_EXCLUDED)) {
		/*
		 * This allows us to pass the message onto the
		 * native menus [Bug: 2272]
		 */
		result = (*tkWinProcs->defWindowProc)(hwnd, message,
			wParam, lParam);
		goto done;
	    }

	    /*
	     * Don't activate the window yet since there is a grab
	     * that takes precedence.  Instead we need to queue
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
	    result = (*tkWinProcs->defWindowProc)(hwnd, message,
		    wParam, lParam);
	}
    } else {
	result = (*tkWinProcs->defWindowProc)(hwnd, message, wParam, lParam);
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
 *	Changes the style bit used to create a new toplevel.
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
    if (!(winPtr->flags & TK_TOP_HIERARCHY)) {
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
    if (!(winPtr->flags & TK_TOP_HIERARCHY)) {
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
