/* 
 * tkWinX.c --
 *
 *	This file contains Windows emulation procedures for X routines. 
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * Copyright (c) 1994 Software Research Associates, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"

/*
 * The w32api 1.1 package (included in Mingw 1.1) does not define _WIN32_IE
 * by default. Define it here to gain access to the InitCommonControlsEx API
 * in commctrl.h.
 */

#ifndef _WIN32_IE
#define _WIN32_IE 0x0300
#endif

#include <commctrl.h>

/*
 * The zmouse.h file includes the definition for WM_MOUSEWHEEL.
 */

#include <zmouse.h>

/*
 * imm.h is needed by HandleIMEComposition
 */

#include <imm.h>

static TkWinProcs asciiProcs = {
    0,

    (LRESULT (WINAPI *)(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg,
	    WPARAM wParam, LPARAM lParam)) CallWindowProcA,
    (LRESULT (WINAPI *)(HWND hWnd, UINT Msg, WPARAM wParam,
	    LPARAM lParam)) DefWindowProcA,
    (ATOM (WINAPI *)(CONST WNDCLASS *lpWndClass)) RegisterClassA,
    (BOOL (WINAPI *)(HWND hWnd, LPCTSTR lpString)) SetWindowTextA,
    (HWND (WINAPI *)(DWORD dwExStyle, LPCTSTR lpClassName,
	    LPCTSTR lpWindowName, DWORD dwStyle, int x, int y,
	    int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
	    HINSTANCE hInstance, LPVOID lpParam)) CreateWindowExA,
};

static TkWinProcs unicodeProcs = {
    1,

    (LRESULT (WINAPI *)(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg,
	    WPARAM wParam, LPARAM lParam)) CallWindowProcW,
    (LRESULT (WINAPI *)(HWND hWnd, UINT Msg, WPARAM wParam,
	    LPARAM lParam)) DefWindowProcW,
    (ATOM (WINAPI *)(CONST WNDCLASS *lpWndClass)) RegisterClassW,
    (BOOL (WINAPI *)(HWND hWnd, LPCTSTR lpString)) SetWindowTextW,
    (HWND (WINAPI *)(DWORD dwExStyle, LPCTSTR lpClassName,
	    LPCTSTR lpWindowName, DWORD dwStyle, int x, int y,
	    int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
	    HINSTANCE hInstance, LPVOID lpParam)) CreateWindowExW,
};

TkWinProcs *tkWinProcs;

/*
 * Declarations of static variables used in this file.
 */

static char winScreenName[] = ":0"; /* Default name of windows display. */
static HINSTANCE tkInstance = NULL; /* Application instance handle. */
static int childClassInitialized;   /* Registered child class? */
static WNDCLASS childClass;	    /* Window class for child windows. */
static int tkPlatformId = 0;	    /* version of Windows platform */
static Tcl_Encoding keyInputEncoding = NULL;/* The current character
				     * encoding for keyboard input */
static int keyInputCharset = -1;    /* The Win32 CHARSET for the keyboard
				     * encoding */
static Tcl_Encoding unicodeEncoding = NULL; /* unicode encoding */

/*
 * Thread local storage.  Notice that now each thread must have its
 * own TkDisplay structure, since this structure contains most of
 * the thread-specific date for threads.
 */
typedef struct ThreadSpecificData {
    TkDisplay *winDisplay;       /* TkDisplay structure that *
				  *  represents Windows screen. */
    int updatingClipboard;	/* If 1, we are updating the clipboard */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations of procedures used in this file.
 */

static void		GenerateXEvent _ANSI_ARGS_((HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam));
static unsigned int	GetState _ANSI_ARGS_((UINT message, WPARAM wParam,
			    LPARAM lParam));
static void 		GetTranslatedKey _ANSI_ARGS_((XKeyEvent *xkey));
static void             UpdateInputLanguage _ANSI_ARGS_((int charset));
static int              HandleIMEComposition _ANSI_ARGS_((HWND hwnd,
			    LPARAM lParam));

/*
 *----------------------------------------------------------------------
 *
 * TkGetServerInfo --
 *
 *	Given a window, this procedure returns information about
 *	the window server for that window.  This procedure provides
 *	the guts of the "winfo server" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkGetServerInfo(interp, tkwin)
    Tcl_Interp *interp;		/* The server information is returned in
				 * this interpreter's result. */
    Tk_Window tkwin;		/* Token for window;  this selects a
				 * particular display and server. */
{
    char buffer[60];
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    sprintf(buffer, "Windows %d.%d %d %s", os.dwMajorVersion,
	    os.dwMinorVersion, os.dwBuildNumber,
#ifdef _WIN64
	    "Win64"
#else
	    "Win32"
#endif
	);
    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetHINSTANCE --
 *
 *	Retrieves the global instance handle used by the Tk library.
 *
 * Results:
 *	Returns the global instance handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HINSTANCE
Tk_GetHINSTANCE()
{
    if (tkInstance == NULL) {
	tkInstance = GetModuleHandle(NULL);
    }
    return tkInstance;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinSetHINSTANCE --
 *
 *	Sets the global instance handle used by the Tk library.
 *	This should be called by DllMain.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkWinSetHINSTANCE(hInstance)
    HINSTANCE hInstance;
{
    tkInstance = hInstance;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinXInit --
 *
 *	Initialize Xlib emulation layer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up various data structures.
 *
 *----------------------------------------------------------------------
 */

void
TkWinXInit(hInstance)
    HINSTANCE hInstance;
{
    if (childClassInitialized != 0) {
	return;
    }
    childClassInitialized = 1;

    if (TkWinGetPlatformId() == VER_PLATFORM_WIN32_NT) {
	/*
	 * This is necessary to enable the use of themeable elements on XP,
	 * so we don't even try and call it for Win9*.
	 */

	INITCOMMONCONTROLSEX comctl;
	ZeroMemory(&comctl, sizeof(comctl));
	(void) InitCommonControlsEx(&comctl);

	tkWinProcs = &unicodeProcs;
    } else {
	tkWinProcs = &asciiProcs;
    }

    /*
     * When threads are enabled, we cannot use CLASSDC because
     * threads will then write into the same device context.
     * 
     * This is a hack; we should add a subsystem that manages
     * device context on a per-thread basis.  See also tkWinWm.c,
     * which also initializes a WNDCLASS structure.
     */

#ifdef TCL_THREADS
    childClass.style = CS_HREDRAW | CS_VREDRAW;
#else
    childClass.style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC;
#endif

    childClass.cbClsExtra = 0;
    childClass.cbWndExtra = 0;
    childClass.hInstance = hInstance;
    childClass.hbrBackground = NULL;
    childClass.lpszMenuName = NULL;

    /*
     * Register the Child window class.
     */

    childClass.lpszClassName = TK_WIN_CHILD_CLASS_NAME;
    childClass.lpfnWndProc = TkWinChildProc;
    childClass.hIcon = NULL;
    childClass.hCursor = NULL;

    if (!RegisterClass(&childClass)) {
	panic("Unable to register TkChild class");
    }

    /*
     * Make sure we cleanup on finalize.
     */
    Tcl_CreateExitHandler((Tcl_ExitProc *) TkWinXCleanup,
	    (ClientData) hInstance);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinXCleanup --
 *
 *	Removes the registered classes for Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes window classes from the system.
 *
 *----------------------------------------------------------------------
 */

void
TkWinXCleanup(hInstance)
    HINSTANCE hInstance;
{
    /*
     * Clean up our own class.
     */
    
    if (childClassInitialized) {
        childClassInitialized = 0;
        UnregisterClass(TK_WIN_CHILD_CLASS_NAME, hInstance);
    }

    if (unicodeEncoding != NULL) {
	Tcl_FreeEncoding(unicodeEncoding);
	unicodeEncoding = NULL;
    }

    /*
     * And let the window manager clean up its own class(es).
     */
    
    TkWinWmCleanup(hInstance);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetPlatformId --
 *
 *	Determines whether running under NT, 95, or Win32s, to allow 
 *	runtime conditional code.  Win32s is no longer supported.
 *
 * Results:
 *	The return value is one of:
 *	    VER_PLATFORM_WIN32s		Win32s on Windows 3.1. 
 *	    VER_PLATFORM_WIN32_WINDOWS	Win32 on Windows 95.
 *	    VER_PLATFORM_WIN32_NT	Win32 on Windows NT
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int		
TkWinGetPlatformId()
{
    if (tkPlatformId == 0) {
	OSVERSIONINFO os;

	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os);
	tkPlatformId = os.dwPlatformId;
    }
    return tkPlatformId;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetDefaultScreenName --
 *
 *	Returns the name of the screen that Tk should use during
 *	initialization.
 *
 * Results:
 *	Returns a statically allocated string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

CONST char *
TkGetDefaultScreenName(interp, screenName)
    Tcl_Interp *interp;		/* Not used. */
    CONST char *screenName;	/* If NULL, use default string. */
{
    if ((screenName == NULL) || (screenName[0] == '\0')) {
	screenName = winScreenName;
    }
    return screenName;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpOpenDisplay --
 *
 *	Create the Display structure and fill it with device
 *	specific information.
 *
 * Results:
 *	Returns a TkDisplay structure on success or NULL on failure.
 *
 * Side effects:
 *	Allocates a new TkDisplay structure.
 *
 *----------------------------------------------------------------------
 */

TkDisplay *
TkpOpenDisplay(display_name)
    CONST char *display_name;
{
    Screen *screen;
    HDC dc;
    TkWinDrawable *twdPtr;
    Display *display;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->winDisplay != NULL) {
	if (strcmp(tsdPtr->winDisplay->display->display_name, display_name) 
                == 0) {
	    return tsdPtr->winDisplay;
	} else {
	    return NULL;
	}
    }

    display = (Display *) ckalloc(sizeof(Display));
    ZeroMemory(display, sizeof(Display));

    display->display_name = (char *) ckalloc(strlen(display_name)+1);
    strcpy(display->display_name, display_name);

    display->cursor_font = 1;
    display->nscreens = 1;
    display->request = 1;
    display->qlen = 0;

    screen = (Screen *) ckalloc(sizeof(Screen));
    screen->display = display;

    dc = GetDC(NULL);
    screen->width = GetDeviceCaps(dc, HORZRES);
    screen->height = GetDeviceCaps(dc, VERTRES);
    screen->mwidth = MulDiv(screen->width, 254,
	    GetDeviceCaps(dc, LOGPIXELSX) * 10);
    screen->mheight = MulDiv(screen->height, 254,
	    GetDeviceCaps(dc, LOGPIXELSY) * 10);
    
    /*
     * Set up the root window.
     */

    twdPtr = (TkWinDrawable*) ckalloc(sizeof(TkWinDrawable));
    if (twdPtr == NULL) {
	return None;
    }
    twdPtr->type = TWD_WINDOW;
    twdPtr->window.winPtr = NULL;
    twdPtr->window.handle = NULL;
    screen->root = (Window)twdPtr;

    /*
     * On windows, when creating a color bitmap, need two pieces of 
     * information: the number of color planes and the number of 
     * pixels per plane.  Need to remember both quantities so that
     * when constructing an HBITMAP for offscreen rendering, we can
     * specify the correct value for the number of planes.  Otherwise
     * the HBITMAP won't be compatible with the HWND and we'll just
     * get blank spots copied onto the screen.
     */

    screen->ext_data = (XExtData *) GetDeviceCaps(dc, PLANES);
    screen->root_depth = GetDeviceCaps(dc, BITSPIXEL) * (int) screen->ext_data;

    screen->root_visual = (Visual *) ckalloc(sizeof(Visual));
    screen->root_visual->visualid = 0;
    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
	screen->root_visual->map_entries = GetDeviceCaps(dc, SIZEPALETTE);
	screen->root_visual->class = PseudoColor;
	screen->root_visual->red_mask = 0x0;
	screen->root_visual->green_mask = 0x0;
	screen->root_visual->blue_mask = 0x0;
    } else {
	if (screen->root_depth == 4) {
	    screen->root_visual->class = StaticColor;
	    screen->root_visual->map_entries = 16;
	} else if (screen->root_depth == 8) {
	    screen->root_visual->class = StaticColor;
	    screen->root_visual->map_entries = 256;
	} else if (screen->root_depth == 12) {
	    screen->root_visual->class = TrueColor;
	    screen->root_visual->map_entries = 32;
	    screen->root_visual->red_mask = 0xf0;
	    screen->root_visual->green_mask = 0xf000;
	    screen->root_visual->blue_mask = 0xf00000;
	} else if (screen->root_depth == 16) {
	    screen->root_visual->class = TrueColor;
	    screen->root_visual->map_entries = 64;
	    screen->root_visual->red_mask = 0xf8;
	    screen->root_visual->green_mask = 0xfc00;
	    screen->root_visual->blue_mask = 0xf80000;
	} else if (screen->root_depth >= 24) {
	    screen->root_visual->class = TrueColor;
	    screen->root_visual->map_entries = 256;
	    screen->root_visual->red_mask = 0xff;
	    screen->root_visual->green_mask = 0xff00;
	    screen->root_visual->blue_mask = 0xff0000;
	}
    }
    screen->root_visual->bits_per_rgb = screen->root_depth;
    ReleaseDC(NULL, dc);

    /*
     * Note that these pixel values are not palette relative.
     */

    screen->white_pixel = RGB(255, 255, 255);
    screen->black_pixel = RGB(0, 0, 0);

    display->screens		= screen;
    display->nscreens		= 1;
    display->default_screen	= 0;
    screen->cmap = XCreateColormap(display, None, screen->root_visual,
	    AllocNone);
    tsdPtr->winDisplay = (TkDisplay *) ckalloc(sizeof(TkDisplay));
    ZeroMemory(tsdPtr->winDisplay, sizeof(TkDisplay));
    tsdPtr->winDisplay->display = display;
    tsdPtr->updatingClipboard = FALSE;
    return tsdPtr->winDisplay;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCloseDisplay --
 *
 *	Closes and deallocates a Display structure created with the
 *	TkpOpenDisplay function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees up memory.
 *
 *----------------------------------------------------------------------
 */

void
TkpCloseDisplay(dispPtr)
    TkDisplay *dispPtr;
{
    Display *display = dispPtr->display;
    HWND hwnd;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (dispPtr != tsdPtr->winDisplay) {
        panic("TkpCloseDisplay: tried to call TkpCloseDisplay on another display");
        return;
    }

    /*
     * Force the clipboard to be rendered if we are the clipboard owner.
     */
    
    if (dispPtr->clipWindow) {
	hwnd = Tk_GetHWND(Tk_WindowId(dispPtr->clipWindow));
	if (GetClipboardOwner() == hwnd) {
	    OpenClipboard(hwnd);
	    EmptyClipboard();
	    TkWinClipboardRender(dispPtr, CF_TEXT);
	    CloseClipboard();
	}
    }

    tsdPtr->winDisplay = NULL;

    if (display->display_name != (char *) NULL) {
        ckfree(display->display_name);
    }
    if (display->screens != (Screen *) NULL) {
        if (display->screens->root_visual != NULL) {
            ckfree((char *) display->screens->root_visual);
        }
        if (display->screens->root != None) {
            ckfree((char *) display->screens->root);
        }
        if (display->screens->cmap != None) {
            XFreeColormap(display, display->screens->cmap);
        }
        ckfree((char *) display->screens);
    }
    ckfree((char *) display);
}

/*
 *----------------------------------------------------------------------
 *
 * XBell --
 *
 *	Generate a beep.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Plays a sounds out the system speakers.
 *
 *----------------------------------------------------------------------
 */

void
XBell(display, percent)
    Display* display;
    int percent;
{
    MessageBeep(MB_OK);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinChildProc --
 *
 *	Callback from Windows whenever an event occurs on a child
 *	window.
 *
 * Results:
 *	Standard Windows return value.
 *
 * Side effects:
 *	May process events off the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

LRESULT CALLBACK
TkWinChildProc(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    LRESULT result;

    switch (message) {
        case WM_INPUTLANGCHANGE:
	    UpdateInputLanguage(wParam);
	    result = 1;
	    break;

        case WM_IME_COMPOSITION:
            result = 0;
            if (HandleIMEComposition(hwnd, lParam) == 0) {
                result = DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;

	case WM_SETCURSOR:
	    /*
	     * Short circuit the WM_SETCURSOR message since we set
	     * the cursor elsewhere.
	     */

	    result = TRUE;
	    break;

	case WM_CREATE:
	case WM_ERASEBKGND:
	    result = 0;
	    break;

	case WM_PAINT:
	    GenerateXEvent(hwnd, message, wParam, lParam);
	    result = DefWindowProc(hwnd, message, wParam, lParam);
	    break;

        case TK_CLAIMFOCUS:
	case TK_GEOMETRYREQ:
	case TK_ATTACHWINDOW:
	case TK_DETACHWINDOW:
	    result =  TkWinEmbeddedEventProc(hwnd, message, wParam, lParam);
	    break;

	default:
	    if (!Tk_TranslateWinEvent(hwnd, message, wParam, lParam,
		    &result)) {
		result = DefWindowProc(hwnd, message, wParam, lParam);
	    }
	    break;
    }

    /*
     * Handle any newly queued events before returning control to Windows.
     */

    Tcl_ServiceAll();
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TranslateWinEvent --
 *
 *	This function is called by widget window procedures to handle
 *	the translation from Win32 events to Tk events.
 *
 * Results:
 *	Returns 1 if the event was handled, else 0.
 *
 * Side effects:
 *	Depends on the event.
 *
 *----------------------------------------------------------------------
 */

int
Tk_TranslateWinEvent(hwnd, message, wParam, lParam, resultPtr)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    LRESULT *resultPtr;
{
    *resultPtr = 0;
    switch (message) {
	case WM_RENDERFORMAT: {
	    TkWindow *winPtr = (TkWindow *) Tk_HWNDToWindow(hwnd);
	    if (winPtr) {
		TkWinClipboardRender(winPtr->dispPtr, wParam);
	    }
	    return 1;
	}

	case WM_COMMAND:
	case WM_NOTIFY:
	case WM_VSCROLL:
	case WM_HSCROLL: {
	    /*
	     * Reflect these messages back to the sender so that they
	     * can be handled by the window proc for the control.  Note
	     * that we need to be careful not to reflect a message that
	     * is targeted to this window, or we will loop.
	     */

	    HWND target = (message == WM_NOTIFY)
		? ((NMHDR*)lParam)->hwndFrom : (HWND) lParam;
	    if (target && target != hwnd) {
		*resultPtr = SendMessage(target, message, wParam, lParam);
		return 1;
	    }
	    break;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	    Tk_PointerEvent(hwnd, (short) LOWORD(lParam),
		    (short) HIWORD(lParam));
	    return 1;

	case WM_CLOSE:
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_DESTROYCLIPBOARD:
	case WM_CHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_MOUSEWHEEL:
 	    GenerateXEvent(hwnd, message, wParam, lParam);
	    return 1;
	case WM_MENUCHAR:
	    GenerateXEvent(hwnd, message, wParam, lParam);
	    /* MNC_CLOSE is the only one that looks right.  This is a hack. */
	    *resultPtr = MAKELONG (0, MNC_CLOSE);
	    return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateXEvent --
 *
 *	This routine generates an X event from the corresponding
 * 	Windows event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Queues one or more X events.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateXEvent(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    XEvent event;
    TkWindow *winPtr = (TkWindow *)Tk_HWNDToWindow(hwnd);
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!winPtr || winPtr->window == None) {
	return;
    }

    event.xany.serial = winPtr->display->request++;
    event.xany.send_event = False;
    event.xany.display = winPtr->display;
    event.xany.window = winPtr->window;

    switch (message) {
	case WM_PAINT: {
	    PAINTSTRUCT ps;

	    event.type = Expose;
	    BeginPaint(hwnd, &ps);
	    event.xexpose.x = ps.rcPaint.left;
	    event.xexpose.y = ps.rcPaint.top;
	    event.xexpose.width = ps.rcPaint.right - ps.rcPaint.left;
	    event.xexpose.height = ps.rcPaint.bottom - ps.rcPaint.top;
	    EndPaint(hwnd, &ps);
	    event.xexpose.count = 0;
	    break;
	}

	case WM_CLOSE:
	    event.type = ClientMessage;
	    event.xclient.message_type =
		Tk_InternAtom((Tk_Window) winPtr, "WM_PROTOCOLS");
	    event.xclient.format = 32;
	    event.xclient.data.l[0] =
		Tk_InternAtom((Tk_Window) winPtr, "WM_DELETE_WINDOW");
	    break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS: {
	    TkWindow *otherWinPtr = (TkWindow *)Tk_HWNDToWindow((HWND) wParam);
	    
	    /*
	     * Compare toplevel windows to avoid reporting focus
	     * changes within the same toplevel.
	     */

	    while (!(winPtr->flags & TK_TOP_LEVEL)) {
		winPtr = winPtr->parentPtr;
		if (winPtr == NULL) {
		    return;
		}
	    }
	    while (otherWinPtr && !(otherWinPtr->flags & TK_TOP_LEVEL)) {
		otherWinPtr = otherWinPtr->parentPtr;
	    }

	    /*
	     * Do a catch-all Tk_SetCaretPos here to make sure that the
	     * window receiving focus sets the caret at least once.
	     */
	    if (message == WM_SETFOCUS) {
		Tk_SetCaretPos((Tk_Window) winPtr, 0, 0, 0);
	    }

	    if (otherWinPtr == winPtr) {
		return;
	    }

	    event.xany.window = winPtr->window;
	    event.type = (message == WM_SETFOCUS) ? FocusIn : FocusOut;
	    event.xfocus.mode = NotifyNormal;
	    event.xfocus.detail = NotifyNonlinear;

	    /*
	     * Destroy the caret if we own it.  If we are moving to another Tk
	     * window, it will reclaim and reposition it with Tk_SetCaretPos.
	     */
	    if (message == WM_KILLFOCUS) {
		DestroyCaret();
	    }
	    break;
	}

	case WM_DESTROYCLIPBOARD:
	    if (tsdPtr->updatingClipboard == TRUE) {
		/*
		 * We want to avoid this event if we are the ones that caused
		 * this event.
		 */
		return;
	    }
	    event.type = SelectionClear;
	    event.xselectionclear.selection =
		Tk_InternAtom((Tk_Window)winPtr, "CLIPBOARD");
	    event.xselectionclear.time = TkpGetMS();
	    break;
	    
	case WM_MOUSEWHEEL:
	    /*
	     * The mouse wheel event is closer to a key event than a
	     * mouse event in that the message is sent to the window
	     * that has focus.
	     */
	    
	case WM_CHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP: {
	    unsigned int state = GetState(message, wParam, lParam);
	    Time time = TkpGetMS();
	    POINT clientPoint;
	    POINTS rootPoint;	/* Note: POINT and POINTS are different */
	    DWORD msgPos;

	    /*
	     * Compute the screen and window coordinates of the event.
	     */
	    
	    msgPos = GetMessagePos();
	    rootPoint = MAKEPOINTS(msgPos);
	    clientPoint.x = rootPoint.x;
	    clientPoint.y = rootPoint.y;
	    ScreenToClient(hwnd, &clientPoint);

	    /*
	     * Set up the common event fields.
	     */

	    event.xbutton.root = RootWindow(winPtr->display,
		    winPtr->screenNum);
	    event.xbutton.subwindow = None;
	    event.xbutton.x = clientPoint.x;
	    event.xbutton.y = clientPoint.y;
	    event.xbutton.x_root = rootPoint.x;
	    event.xbutton.y_root = rootPoint.y;
	    event.xbutton.state = state;
	    event.xbutton.time = time;
	    event.xbutton.same_screen = True;

	    /*
	     * Now set up event specific fields.
	     */

	    switch (message) {
		case WM_MOUSEWHEEL:
		    /*
		     * We have invented a new X event type to handle
		     * this event.  It still uses the KeyPress struct.
		     * However, the keycode field has been overloaded
		     * to hold the zDelta of the wheel.
		     */
		    
		    event.type = MouseWheelEvent;
		    event.xany.send_event = -1;
		    event.xkey.keycode = (short) HIWORD(wParam);
		    break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		    /*
		     * Check for translated characters in the event queue.
		     * Setting xany.send_event to -1 indicates to the
		     * Windows implementation of TkpGetString() that this
		     * event was generated by windows and that the Windows
		     * extension xkey.trans_chars is filled with the
		     * MBCS characters that came from the TranslateMessage
		     * call. 
		     */

		    event.type = KeyPress;
		    event.xany.send_event = -1;
		    event.xkey.keycode = wParam;
		    GetTranslatedKey(&event.xkey);
		    break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
		    /*
		     * We don't check for translated characters on keyup
		     * because Tk won't know what to do with them.  Instead, we
		     * wait for the WM_CHAR messages which will follow.
		     */
		    event.type = KeyRelease;
		    event.xkey.keycode = wParam;
		    event.xkey.nbytes = 0;
		    break;

		case WM_CHAR:
		    /*
		     * Synthesize both a KeyPress and a KeyRelease.
		     * Strings generated by Input Method Editor are handled
		     * in the following manner:
		     * 1. A series of WM_KEYDOWN & WM_KEYUP messages that 
		     *    cause GetTranslatedKey() to be called and return
		     *    immediately because the WM_KEYDOWNs have no 
		     *	  associated WM_CHAR messages -- the IME window is 
		     *	  accumulating the characters and translating them 
		     *    itself.  In the "bind" command, you get an event
		     *	  with a mystery keysym and %A == "" for each 
		     *	  WM_KEYDOWN that actually was meant for the IME.
		     * 2. A WM_KEYDOWN corresponding to the "confirm typing"
		     *    character.  This causes GetTranslatedKey() to be 
		     *	  called.
		     * 3. A WM_IME_NOTIFY message saying that the IME is 
		     *	  done.  A side effect of this message is that 
		     *    GetTranslatedKey() thinks this means that there
		     *	  are no WM_CHAR messages and returns immediately.
		     *    In the "bind" command, you get an another event
		     *	  with a mystery keysym and %A == "".
		     * 4. A sequence of WM_CHAR messages that correspond to 
		     *	  the characters in the IME window.  A bunch of 
		     *    simulated KeyPress/KeyRelease events will be 
		     *    generated, one for each character.  Adjacent 
		     *    WM_CHAR messages may actually specify the high
		     *	  and low bytes of a multi-byte character -- in that
		     *    case the two WM_CHAR messages will be combined into
		     *	  one event.  It is the event-consumer's 
		     *	  responsibility to convert the string returned from
		     *	  XLookupString from system encoding to UTF-8.
		     * 5. And finally we get the WM_KEYUP for the "confirm
		     *    typing" character.
		     */

		    event.type = KeyPress;
		    event.xany.send_event = -1;
		    event.xkey.keycode = 0;
		    event.xkey.nbytes = 1;
		    event.xkey.trans_chars[0] = (char) wParam;

		    if (IsDBCSLeadByte((BYTE) wParam)) {
			MSG msg;

			if ((PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) != 0)
				&& (msg.message == WM_CHAR)) {
			    GetMessage(&msg, NULL, 0, 0);
			    event.xkey.nbytes = 2;
			    event.xkey.trans_chars[1] = (char) msg.wParam;
			}
		    }
		    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
		    event.type = KeyRelease;
		    break;
	    }
	    break;
	}

	default:
	    return;
    }
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * GetState --
 *
 *	This function constructs a state mask for the mouse buttons 
 *	and modifier keys as they were before the event occured.
 *
 * Results:
 *	Returns a composite value of all the modifier and button state
 *	flags that were set at the time the event occurred.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
GetState(message, wParam, lParam)
    UINT message;		/* Win32 message type */
    WPARAM wParam;		/* wParam of message, used if key message */
    LPARAM lParam;		/* lParam of message, used if key message */
{
    int mask;
    int prevState;		/* 1 if key was previously down */
    unsigned int state = TkWinGetModifierState();

    /*
     * If the event is a key press or release, we check for modifier
     * keys so we can report the state of the world before the event.
     */

    if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN
	    || message == WM_SYSKEYUP || message == WM_KEYUP) {
	mask = 0;
	prevState = HIWORD(lParam) & KF_REPEAT;
	switch(wParam) {
	    case VK_SHIFT:
		mask = ShiftMask;
		break;
	    case VK_CONTROL:
		mask = ControlMask;
		break;
	    case VK_MENU:
		mask = ALT_MASK;
		break;
	    case VK_CAPITAL:
		if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN) {
		    mask = LockMask;
		    prevState = ((state & mask) ^ prevState) ? 0 : 1;
		}
		break;
	    case VK_NUMLOCK:
		if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN) {
		    mask = Mod1Mask;
		    prevState = ((state & mask) ^ prevState) ? 0 : 1;
		}
		break;
	    case VK_SCROLL:
		if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN) {
		    mask = Mod3Mask;
		    prevState = ((state & mask) ^ prevState) ? 0 : 1;
		}
		break;
	}
	if (prevState) {
	    state |= mask;
	} else {
	    state &= ~mask;
	}
    }
    return state;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTranslatedKey --
 *
 *	Retrieves WM_CHAR messages that are placed on the system queue
 *	by the TranslateMessage system call and places them in the
 *	given KeyPress event.
 *
 * Results:
 *	Sets the trans_chars and nbytes member of the key event.
 *
 * Side effects:
 *	Removes any WM_CHAR messages waiting on the top of the system
 *	event queue.
 *
 *----------------------------------------------------------------------
 */

static void
GetTranslatedKey(xkey)
    XKeyEvent *xkey;
{
    MSG msg;
    
    xkey->nbytes = 0;

    while ((xkey->nbytes < XMaxTransChars)
	    && PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
	if ((msg.message == WM_CHAR) || (msg.message == WM_SYSCHAR)) {
	    GetMessage(&msg, NULL, 0, 0);

	    /*
	     * If this is a normal character message, we may need to strip
	     * off the Alt modifier (e.g. Alt-digits).  Note that we don't
	     * want to do this for system messages, because those were
	     * presumably generated as an Alt-char sequence (e.g. accelerator
	     * keys).
	     */

	    if ((msg.message == WM_CHAR) && (msg.lParam & 0x20000000)) {
		xkey->state = 0;
	    }
	    xkey->trans_chars[xkey->nbytes] = (char) msg.wParam;
	    xkey->nbytes++;

	    if (((unsigned short) msg.wParam) > ((unsigned short) 0xff)) {
                /*
                 * Some "addon" input devices, such as the popular
                 * PenPower Chinese writing pad, generate 16 bit
                 * values in WM_CHAR messages (instead of passing them
                 * in two separate WM_CHAR messages containing two
                 * 8-bit values.
                 */

	        xkey->trans_chars[xkey->nbytes] = (char) (msg.wParam >> 8);
	        xkey->nbytes ++;
	    }
	} else {
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateInputLanguage --
 *
 *	Gets called when a WM_INPUTLANGCHANGE message is received
 *      by the TK child window procedure. This message is sent
 *      by the Input Method Editor system when the user chooses
 *      a different input method. All subsequent WM_CHAR
 *      messages will contain characters in the new encoding. We record
 *      the new encoding so that TkpGetString() knows how to
 *      correctly translate the WM_CHAR into unicode.
 *
 * Results:
 *	Records the new encoding in keyInputEncoding.
 *
 * Side effects:
 *	Old value of keyInputEncoding is freed.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateInputLanguage(charset)
    int charset;
{
    CHARSETINFO charsetInfo;
    Tcl_Encoding encoding;
    char codepage[4 + TCL_INTEGER_SPACE];

    if (keyInputCharset == charset) {
	return;
    }
    if (TranslateCharsetInfo((DWORD*)charset, &charsetInfo, TCI_SRCCHARSET)
            == 0) {
	/*
	 * Some mysterious failure.
	 */

	return;
    }

    wsprintfA(codepage, "cp%d", charsetInfo.ciACP);

    if ((encoding = Tcl_GetEncoding(NULL, codepage)) == NULL) {
	/*
	 * The encoding is not supported by Tcl.
	 */

	return;
    }

    if (keyInputEncoding != NULL) {
	Tcl_FreeEncoding(keyInputEncoding);
    }

    keyInputEncoding = encoding;
    keyInputCharset = charset;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetKeyInputEncoding --
 *
 *	Returns the current keyboard input encoding selected by the
 *      user (with WM_INPUTLANGCHANGE events).
 *
 * Results:
 *	The current keyboard input encoding.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Encoding
TkWinGetKeyInputEncoding()
{
    return keyInputEncoding;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetUnicodeEncoding --
 *
 *	Returns the cached unicode encoding.
 *
 * Results:
 *	The unicode encoding.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Encoding
TkWinGetUnicodeEncoding()
{
    if (unicodeEncoding == NULL) {
	unicodeEncoding = Tcl_GetEncoding(NULL, "unicode");
    }
    return unicodeEncoding;
}

/*
 *----------------------------------------------------------------------
 *
 * HandleIMEComposition --
 *
 *      This function works around a definciency in some versions
 *      of Windows 2000 to make it possible to entry multi-lingual
 *      characters under all versions of Windows 2000.
 *
 *      When an Input Method Editor (IME) is ready to send input
 *      characters to an application, it sends a WM_IME_COMPOSITION
 *      message with the GCS_RESULTSTR. However, The DefWindowProc()
 *      on English Windows 2000 arbitrarily converts all non-Latin-1
 *      characters in the composition to "?".
 *
 *      This function correctly processes the composition data and
 *      sends the UNICODE values of the composed characters to
 *      TK's event queue. 
 *
 * Results:
 *	If this function has processed the composition data, returns 1.
 *      Otherwise returns 0.
 *
 * Side effects:
 *	Key events are put into the TK event queue.
 *
 *----------------------------------------------------------------------
 */

static int
HandleIMEComposition(hwnd, lParam)
    HWND hwnd;                          /* Window receiving the message. */
    LPARAM lParam;                      /* Flags for the WM_IME_COMPOSITION
                                         * message */
{
    HIMC hIMC;
    int i, n;
    XEvent event;
    char * buff;
    TkWindow *winPtr;
    Tcl_Encoding unicodeEncoding = TkWinGetUnicodeEncoding();
    BOOL isWinNT = (TkWinGetPlatformId() == VER_PLATFORM_WIN32_NT);

    if ((lParam & GCS_RESULTSTR) == 0) {
        /*
         * Composition is not finished yet.
         */

        return 0;
    }

    hIMC = ImmGetContext(hwnd);
    if (hIMC) {
	if (isWinNT) {
	    n = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
	} else {
	    n = ImmGetCompositionStringA(hIMC, GCS_RESULTSTR, NULL, 0);
	}

        if ((n > 0) && ((buff = (char *) ckalloc(n)) != NULL)) {
	    if (isWinNT) {
		n = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, buff, n);
	    } else {
		Tcl_DString utfString, unicodeString;

		n = ImmGetCompositionStringA(hIMC, GCS_RESULTSTR, buff, n);
		Tcl_DStringInit(&utfString);
		Tcl_ExternalToUtfDString(keyInputEncoding, buff, n,
			&utfString);
		Tcl_UtfToExternalDString(unicodeEncoding,
			Tcl_DStringValue(&utfString), -1, &unicodeString);
		i = Tcl_DStringLength(&unicodeString);
		if (n < i) {
		    /*
		     * Only alloc more space if we need, otherwise just
		     * use what we've created.  Don't realloc as that may
		     * copy data we no longer need.
		     */
		    ckfree((char *) buff);
		    buff = (char *) ckalloc(i);
		}
		n = i;
		memcpy(buff, Tcl_DStringValue(&unicodeString), n);
		Tcl_DStringFree(&utfString);
		Tcl_DStringFree(&unicodeString);
	    }

	    /*
	     * Set up the fields pertinent to key event.
             *
             * We set send_event to the special value of -2, so that
             * TkpGetString() in tkWinKey.c knows that trans_chars[]
             * already contains a UNICODE char and there's no need to
             * do encoding conversion.
	     */

            winPtr = (TkWindow *)Tk_HWNDToWindow(hwnd);

            event.xkey.serial = winPtr->display->request++;
            event.xkey.send_event = -2;
            event.xkey.display = winPtr->display;
            event.xkey.window = winPtr->window;
	    event.xkey.root = RootWindow(winPtr->display, winPtr->screenNum);
	    event.xkey.subwindow = None;
	    event.xkey.state = TkWinGetModifierState();
	    event.xkey.time = TkpGetMS();
	    event.xkey.same_screen = True;
            event.xkey.keycode = 0;
            event.xkey.nbytes = 2;

            for (i=0; i<n;) {
                /*
                 * Simulate a pair of KeyPress and KeyRelease events
                 * for each UNICODE character in the composition.
                 */

                event.xkey.trans_chars[0] = (char) buff[i++];
                event.xkey.trans_chars[1] = (char) buff[i++];

                event.type = KeyPress;
                Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

                event.type = KeyRelease;
                Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
            }

            ckfree(buff);
        }
        ImmReleaseContext(hwnd, hIMC);
        return 1;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeXId --
 *
 *	This interface is not needed under Windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeXId(display, xid)
    Display *display;
    XID xid;
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinResendEvent --
 *
 *	This function converts an X event into a Windows event and
 *	invokes the specified windo procedure.
 *
 * Results:
 *	A standard Windows result.
 *
 * Side effects:
 *	Invokes the window procedure
 *
 *----------------------------------------------------------------------
 */

LRESULT
TkWinResendEvent(wndproc, hwnd, eventPtr)
    WNDPROC wndproc;
    HWND hwnd;
    XEvent *eventPtr;
{
    UINT msg;
    WPARAM wparam;
    LPARAM lparam;

    if (eventPtr->type == ButtonPress) {
	switch (eventPtr->xbutton.button) {
	    case Button1:
		msg = WM_LBUTTONDOWN;
		wparam = MK_LBUTTON;
		break;
	    case Button2:
		msg = WM_MBUTTONDOWN;
		wparam = MK_MBUTTON;
		break;
	    case Button3:
		msg = WM_RBUTTONDOWN;
		wparam = MK_RBUTTON;
		break;
	    default:
		return 0;
	}
	if (eventPtr->xbutton.state & Button1Mask) {
	    wparam |= MK_LBUTTON;
	}
	if (eventPtr->xbutton.state & Button2Mask) {
	    wparam |= MK_MBUTTON;
	}
	if (eventPtr->xbutton.state & Button3Mask) {
	    wparam |= MK_RBUTTON;
	}
	if (eventPtr->xbutton.state & ShiftMask) {
	    wparam |= MK_SHIFT;
	}
	if (eventPtr->xbutton.state & ControlMask) {
	    wparam |= MK_CONTROL;
	}
	lparam = MAKELPARAM((short) eventPtr->xbutton.x,
		(short) eventPtr->xbutton.y);
    } else {
	return 0;
    }
    return CallWindowProc(wndproc, hwnd, msg, wparam, lparam);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetMS --
 *
 *	Return a relative time in milliseconds.  It doesn't matter
 *	when the epoch was.
 *
 * Results:
 *	Number of milliseconds.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
TkpGetMS()
{
    return GetTickCount();
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinUpdatingClipboard --
 *
 *
 * Results:
 *	Number of milliseconds.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkWinUpdatingClipboard(int mode)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->updatingClipboard = mode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetCaretPos --
 *
 *	This enables correct movement of focus in the MS Magnifier, as well
 *	as allowing us to correctly position the IME Window.  The following
 *	Win32 APIs are used to work with MS caret:
 *
 *	CreateCaret	DestroyCaret	SetCaretPos	GetCaretPos
 *
 *	Only one instance of caret can be active at any time
 *	(e.g. DestroyCaret API does not take any argument such as handle).
 *	Since do-it-right approach requires to track the create/destroy
 *	caret status all the time in a global scope among windows (or
 *	widgets), we just implement this minimal setup to get the job done.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Sets the global Windows caret position.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetCaretPos(Tk_Window tkwin, int x, int y, int height)
{
    static HWND caretHWND = NULL;
    TkCaret *caretPtr = &(((TkWindow *) tkwin)->dispPtr->caret);
    Window win;

    /*
     * Prevent processing anything if the values haven't changed.
     * Windows only has one display, so we can do this with statics.
     */
    if ((caretPtr->winPtr == ((TkWindow *) tkwin))
	    && (caretPtr->x == x) && (caretPtr->y == y)) {
	return;
    }

    caretPtr->winPtr = ((TkWindow *) tkwin);
    caretPtr->x = x;
    caretPtr->y = y;
    caretPtr->height = height;

    /*
     * We adjust to the toplevel to get the coords right, as setting
     * the IME composition window is based on the toplevel hwnd, so
     * ignore height.
     */

    while (!Tk_IsTopLevel(tkwin)) {
	x += Tk_X(tkwin);
	y += Tk_Y(tkwin);
	tkwin = Tk_Parent(tkwin);
	if (tkwin == NULL) {
	    return;
	}
    }

    win = Tk_WindowId(tkwin);
    if (win) {
	HIMC hIMC;
	HWND hwnd = Tk_GetHWND(win);

	if (hwnd != caretHWND) {
	    DestroyCaret();
	    if (CreateCaret(hwnd, NULL, 0, 0)) {
		caretHWND = hwnd;
	    }
	}

	if (!SetCaretPos(x, y) && CreateCaret(hwnd, NULL, 0, 0)) {
	    caretHWND = hwnd;
	    SetCaretPos(x, y);
	}

	/*
	 * The IME composition window should be updated whenever the caret
	 * position is changed because a clause of the composition string may
	 * be converted to the final characters and the other clauses still
	 * stay on the composition window.  -- yamamoto
	 */
	hIMC = ImmGetContext(hwnd);
	if (hIMC) {
	    COMPOSITIONFORM cform;
	    cform.dwStyle = CFS_POINT;
	    cform.ptCurrentPos.x = x;
	    cform.ptCurrentPos.y = y;
	    ImmSetCompositionWindow(hIMC, &cform);
	    ImmReleaseContext(hwnd, hIMC);
	}
    }
}
