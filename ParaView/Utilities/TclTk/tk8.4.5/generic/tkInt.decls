	# tkInt.decls --
#
#	This file contains the declarations for all unsupported
#	functions that are exported by the Tk library.  This file
#	is used to generate the tkIntDecls.h, tkIntPlatDecls.h,
#	tkIntStub.c, and tkPlatStub.c files.
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) Id

library tk

# Define the unsupported generic interfaces.

interface tkInt

# Declare each of the functions in the unsupported internal Tcl
# interface.  These interfaces are allowed to changed between versions.
# Use at your own risk.  Note that the position of functions should not
# be changed between versions to avoid gratuitous incompatibilities.

declare 0 generic {
    TkWindow * TkAllocWindow (TkDisplay *dispPtr, int screenNum, \
	    TkWindow *parentPtr)
}

declare 1 generic {
    void TkBezierPoints (double control[], int numSteps, double *coordPtr)
}

declare 2 generic {
    void TkBezierScreenPoints (Tk_Canvas canvas, double control[], \
	    int numSteps, XPoint *xPointPtr)
}

declare 3 generic {
    void TkBindDeadWindow (TkWindow *winPtr)
}

declare 4 generic {
    void TkBindEventProc (TkWindow *winPtr, XEvent *eventPtr)
}

declare 5 generic {
    void TkBindFree (TkMainInfo *mainPtr)
}

declare 6 generic {
    void TkBindInit (TkMainInfo *mainPtr)
}

declare 7 generic {
    void TkChangeEventWindow (XEvent *eventPtr, TkWindow *winPtr)
}

declare 8 generic {
    int TkClipInit (Tcl_Interp *interp, TkDisplay *dispPtr)
}

declare 9 generic {
    void TkComputeAnchor (Tk_Anchor anchor, Tk_Window tkwin, \
	    int padX, int padY, int innerWidth, int innerHeight, \
	    int *xPtr, int *yPtr)
}

declare 10 generic {
    int TkCopyAndGlobalEval (Tcl_Interp *interp, char *script)
}

declare 11 generic {
    unsigned long TkCreateBindingProcedure (Tcl_Interp *interp, \
	    Tk_BindingTable bindingTable, \
	    ClientData object, CONST char *eventString, \
	    TkBindEvalProc *evalProc, TkBindFreeProc *freeProc, \
	    ClientData clientData)
}

declare 12 generic {
    TkCursor * TkCreateCursorFromData (Tk_Window tkwin, \
	    CONST char *source, CONST char *mask, int width, int height, \
	    int xHot, int yHot, XColor fg, XColor bg)
}

declare 13 generic {
    int TkCreateFrame (ClientData clientData, \
	    Tcl_Interp *interp, int argc, char **argv, \
	    int toplevel, char *appName)
}

declare 14 generic {
    Tk_Window TkCreateMainWindow (Tcl_Interp *interp, \
	    CONST char *screenName, char *baseName)
}

declare 15 generic {
    Time TkCurrentTime (TkDisplay *dispPtr)
}

declare 16 generic {
    void TkDeleteAllImages (TkMainInfo *mainPtr)
}

declare 17 generic {
    void TkDoConfigureNotify (TkWindow *winPtr)
}

declare 18 generic {
    void TkDrawInsetFocusHighlight (Tk_Window tkwin, GC gc, int width, \
	    Drawable drawable, int padding)
}

declare 19 generic {
    void TkEventDeadWindow (TkWindow *winPtr)
}

declare 20 generic {
    void TkFillPolygon (Tk_Canvas canvas, \
	    double *coordPtr, int numPoints, Display *display, \
	    Drawable drawable, GC gc, GC outlineGC)
}

declare 21 generic {
    int TkFindStateNum (Tcl_Interp *interp, \
	    CONST char *option, CONST TkStateMap *mapPtr, \
	    CONST char *strKey)
}

declare 22 generic {
    char * TkFindStateString (CONST TkStateMap *mapPtr, int numKey)
}

declare 23 generic {
    void TkFocusDeadWindow (TkWindow *winPtr)
}

declare 24 generic {
    int TkFocusFilterEvent (TkWindow *winPtr, XEvent *eventPtr)
}

declare 25 generic {
    TkWindow * TkFocusKeyEvent (TkWindow *winPtr, XEvent *eventPtr)
}

declare 26 generic {
    void TkFontPkgInit (TkMainInfo *mainPtr)
}

declare 27 generic {
    void TkFontPkgFree (TkMainInfo *mainPtr)
}

declare 28 generic {
    void TkFreeBindingTags (TkWindow *winPtr)
}

# Name change only, TkFreeCursor in Tcl 8.0.x now TkpFreeCursor
declare 29 generic {
    void TkpFreeCursor (TkCursor *cursorPtr)
}

declare 30 generic {
    char * TkGetBitmapData (Tcl_Interp *interp, \
	    char *string, char *fileName, int *widthPtr, \
	    int *heightPtr, int *hotXPtr, int *hotYPtr)
}

declare 31 generic {
    void TkGetButtPoints (double p1[], double p2[], \
	    double width, int project, double m1[], double m2[])
}

declare 32 generic {
    TkCursor * TkGetCursorByName (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tk_Uid string)
}

declare 33 generic {
    CONST84_RETURN char * TkGetDefaultScreenName (Tcl_Interp *interp, \
	    CONST char *screenName)
}

declare 34 generic {
    TkDisplay * TkGetDisplay (Display *display)
}

declare 35 generic {
    int TkGetDisplayOf (Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], \
	    Tk_Window *tkwinPtr)
}

declare 36 generic {
    TkWindow * TkGetFocusWin (TkWindow *winPtr)
}

declare 37 generic {
    int TkGetInterpNames (Tcl_Interp *interp, Tk_Window tkwin)
}

declare 38 generic {
    int TkGetMiterPoints (double p1[], double p2[], double p3[], \
	    double width, double m1[],double m2[])
}

declare 39 generic {
    void TkGetPointerCoords (Tk_Window tkwin, int *xPtr, int *yPtr)
}

declare 40 generic {
    void TkGetServerInfo (Tcl_Interp *interp, Tk_Window tkwin)
}

declare 41 generic {
    void TkGrabDeadWindow (TkWindow *winPtr)
}

declare 42 generic {
    int TkGrabState (TkWindow *winPtr)
}

declare 43 generic {
    void TkIncludePoint (Tk_Item *itemPtr, double *pointPtr)
}

declare 44 generic {
    void TkInOutEvents (XEvent *eventPtr, TkWindow *sourcePtr, \
	    TkWindow *destPtr, int leaveType, int enterType, \
	    Tcl_QueuePosition position)
}

declare 45 generic {
    void TkInstallFrameMenu (Tk_Window tkwin)
}

declare 46 generic {
    char * TkKeysymToString (KeySym keysym)
}

declare 47 generic {
    int TkLineToArea (double end1Ptr[], double end2Ptr[], double rectPtr[])
}

declare 48 generic {
    double TkLineToPoint (double end1Ptr[], \
	    double end2Ptr[], double pointPtr[])
}

declare 49 generic {
    int TkMakeBezierCurve (Tk_Canvas canvas, \
	    double *pointPtr, int numPoints, int numSteps, \
	    XPoint xPoints[], double dblPoints[])
}

declare 50 generic {
    void TkMakeBezierPostscript (Tcl_Interp *interp, \
	    Tk_Canvas canvas, double *pointPtr, int numPoints)
}

declare 51 generic {
    void TkOptionClassChanged (TkWindow *winPtr)
}

declare 52 generic {
    void TkOptionDeadWindow (TkWindow *winPtr)
}

declare 53 generic {
    int TkOvalToArea (double *ovalPtr, double *rectPtr)
}

declare 54 generic {
    double TkOvalToPoint (double ovalPtr[], \
	    double width, int filled, double pointPtr[])
}

declare 55 generic {
    int TkpChangeFocus (TkWindow *winPtr, int force)
}

declare 56 generic {
    void TkpCloseDisplay (TkDisplay *dispPtr)
}

declare 57 generic {
    void TkpClaimFocus (TkWindow *topLevelPtr, int force)
}

declare 58 generic {
    void TkpDisplayWarning (CONST char *msg, CONST char *title)
}

declare 59 generic {
    void TkpGetAppName (Tcl_Interp *interp, Tcl_DString *name)
}

declare 60 generic {
    TkWindow * TkpGetOtherWindow (TkWindow *winPtr)
}

declare 61 generic {
    TkWindow * TkpGetWrapperWindow (TkWindow *winPtr)
}

declare 62 generic {
    int TkpInit (Tcl_Interp *interp)
}

declare 63 generic {
    void TkpInitializeMenuBindings (Tcl_Interp *interp, \
	    Tk_BindingTable bindingTable)
}

declare 64 generic {
    void TkpMakeContainer (Tk_Window tkwin)
}

declare 65 generic {
    void TkpMakeMenuWindow (Tk_Window tkwin, int transient)
}

declare 66 generic {
    Window TkpMakeWindow (TkWindow *winPtr, Window parent)
}

declare 67 generic {
    void TkpMenuNotifyToplevelCreate (Tcl_Interp *interp1, char *menuName)
}

declare 68 generic {
    TkDisplay * TkpOpenDisplay (CONST char *display_name)
}

declare 69 generic {
    int TkPointerEvent (XEvent *eventPtr, TkWindow *winPtr)
}

declare 70 generic {
    int TkPolygonToArea (double *polyPtr, int numPoints, double *rectPtr)
}

declare 71 generic {
    double TkPolygonToPoint (double *polyPtr, int numPoints, double *pointPtr)
}

declare 72 generic {
    int TkPositionInTree (TkWindow *winPtr, TkWindow *treePtr)
}

declare 73 generic {
    void TkpRedirectKeyEvent (TkWindow *winPtr, XEvent *eventPtr)
}

declare 74 generic {
    void TkpSetMainMenubar (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *menuName)
}

declare 75 generic {
    int TkpUseWindow (Tcl_Interp *interp, Tk_Window tkwin, CONST char *string)
}

declare 76 generic {
    int TkpWindowWasRecentlyDeleted (Window win, TkDisplay *dispPtr)
}

declare 77 generic {
    void TkQueueEventForAllChildren (TkWindow *winPtr, XEvent *eventPtr)
}

declare 78 generic {
    int TkReadBitmapFile (Display* display, Drawable d, CONST char* filename, \
	    unsigned int* width_return, unsigned int* height_return, \
	    Pixmap* bitmap_return, int* x_hot_return, int* y_hot_return)
}

declare 79 generic {
    int TkScrollWindow (Tk_Window tkwin, GC gc, \
	    int x, int y, int width, int height, int dx, \
	    int dy, TkRegion damageRgn)
}

declare 80 generic {
    void TkSelDeadWindow (TkWindow *winPtr)
}

declare 81 generic {
    void TkSelEventProc (Tk_Window tkwin, XEvent *eventPtr)
}

declare 82 generic {
    void TkSelInit (Tk_Window tkwin)
}

declare 83 generic {
    void TkSelPropProc (XEvent *eventPtr)
}

# Exported publically as Tk_SetClassProcs in 8.4a2
#declare 84 generic {
#    void TkSetClassProcs (Tk_Window tkwin, \
#	    TkClassProcs *procs, ClientData instanceData)
#}

declare 85 generic {
    void TkSetWindowMenuBar (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *oldMenuName, char *menuName)
}

declare 86 generic {
    KeySym TkStringToKeysym (char *name)
}

declare 87 generic {
    int TkThickPolyLineToArea (double *coordPtr, \
	    int numPoints, double width, int capStyle, \
	    int joinStyle, double *rectPtr)
}

declare 88 generic {
    void TkWmAddToColormapWindows (TkWindow *winPtr)
}

declare 89 generic {
    void TkWmDeadWindow (TkWindow *winPtr)
}

declare 90 generic {
    TkWindow * TkWmFocusToplevel (TkWindow *winPtr)
}

declare 91 generic {
    void TkWmMapWindow (TkWindow *winPtr)
}

declare 92 generic {
    void TkWmNewWindow (TkWindow *winPtr)
}

declare 93 generic {
    void TkWmProtocolEventProc (TkWindow *winPtr, XEvent *evenvPtr)
}

declare 94 generic {
    void TkWmRemoveFromColormapWindows (TkWindow *winPtr)
}

declare 95 generic {
    void TkWmRestackToplevel (TkWindow *winPtr, int aboveBelow, \
	    TkWindow *otherPtr)
}

declare 96 generic {
    void TkWmSetClass (TkWindow *winPtr)
}

declare 97 generic {
    void TkWmUnmapWindow (TkWindow *winPtr)
}

# new for 8.1

declare 98 generic {
    Tcl_Obj * TkDebugBitmap ( Tk_Window tkwin, char *name)
}

declare 99 generic {
    Tcl_Obj * TkDebugBorder ( Tk_Window tkwin, char *name)
}

declare 100 generic {
    Tcl_Obj * TkDebugCursor ( Tk_Window tkwin, char *name)
}

declare 101 generic {
    Tcl_Obj * TkDebugColor ( Tk_Window tkwin, char *name)
}

declare 102 generic {
    Tcl_Obj * TkDebugConfig (Tcl_Interp *interp, Tk_OptionTable table)
}

declare 103 generic {
    Tcl_Obj * TkDebugFont ( Tk_Window tkwin, char *name)
}

declare 104 generic {
    int  TkFindStateNumObj (Tcl_Interp *interp, \
	    Tcl_Obj *optionPtr, CONST TkStateMap *mapPtr, \
	    Tcl_Obj *keyPtr)
}

declare 105 generic {
    Tcl_HashTable *  TkGetBitmapPredefTable (void)
}

declare 106 generic {
    TkDisplay * TkGetDisplayList (void)
}

declare 107 generic {
    TkMainInfo * TkGetMainInfoList (void)
}

declare 108 generic {
    int  TkGetWindowFromObj (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tcl_Obj *objPtr, \
	    Tk_Window *windowPtr)
}

declare 109 generic {
    char *  TkpGetString (TkWindow *winPtr, \
	    XEvent *eventPtr, Tcl_DString *dsPtr)
}

declare 110 generic {
    void  TkpGetSubFonts (Tcl_Interp *interp, Tk_Font tkfont)
}

declare 111 generic {
    Tcl_Obj * TkpGetSystemDefault (Tk_Window tkwin, \
	    CONST char *dbName, CONST char *className)
}

declare 112 generic {
    void TkpMenuThreadInit (void)
}

declare 113 {mac aqua win}  {
    void TkClipBox (TkRegion rgn, XRectangle* rect_return)
}

declare 114 {mac aqua win}  {
    TkRegion TkCreateRegion (void)
}

declare 115 {mac aqua win} {
    void TkDestroyRegion (TkRegion rgn)
}

declare 116 {mac aqua win} {
    void TkIntersectRegion (TkRegion sra, TkRegion srcb, TkRegion dr_return)
}

declare 117 {mac aqua win} {
    int TkRectInRegion (TkRegion rgn, int x, int y, unsigned int width, \
	    unsigned int height)
}

declare 118 {mac aqua win} {
    void TkSetRegion (Display* display, GC gc, TkRegion rgn)
}

declare 119 {mac aqua win} {
    void TkUnionRectWithRegion (XRectangle* rect, \
	    TkRegion src, TkRegion dr_return)
}

# removed duplicate from tkIntPlat table
#declare 120 mac {
#    void TkGenerateActivateEvents (TkWindow *winPtr, int active)
#}

declare 121 {mac aqua} {
    Pixmap TkpCreateNativeBitmap (Display *display, CONST char * source) 
}

declare 122 {mac aqua} {
    void TkpDefineNativeBitmaps (void)
}

# removed duplicate from tkIntPlat table
#declare 123 mac {
#    unsigned long TkpGetMS (void)
#}

declare 124 {mac aqua} {
    Pixmap TkpGetNativeAppBitmap (Display *display, \
 	    CONST char *name, int *width, int *height)
}

# removed duplicates from tkIntPlat table
#declare 125 mac {
#    void TkPointerDeadWindow (TkWindow *winPtr)
#}
#
#declare 126 mac {
#    void TkpSetCapture (TkWindow *winPtr)
#}
#
#declare 127 mac {
#    void TkpSetCursor (TkpCursor cursor)
#}
#
#declare 128 mac {
#    void TkpWmSetState (TkWindow *winPtr, int state)
#}
#
#declare 130 mac {
#    Window  TkGetTransientMaster (TkWindow *winPtr)
#}
#
#declare 131 mac {
#    int  TkGenerateButtonEvent (int x, int y, \
# 	    Window window, unsigned int state)
#}
#
#declare 133 mac {
#    void  TkGenWMDestroyEvent (Tk_Window tkwin)
#}
#
#declare 134 mac {
#    void  TkGenWMConfigureEvent (Tk_Window tkwin, int x, int y, \
# 	    int width, int height, int flags)
#}

declare 135 generic {
    void TkpDrawHighlightBorder (Tk_Window tkwin, GC fgGC, GC bgGC, \
        int highlightWidth, Drawable drawable)
}

declare 136 generic {
    void TkSetFocusWin (TkWindow *winPtr, int force) 
}

declare 137 generic {
    void TkpSetKeycodeAndState (Tk_Window tkwin, KeySym keySym, \
            XEvent *eventPtr)
}

declare 138 generic {
    KeySym TkpGetKeySym (TkDisplay *dispPtr, XEvent *eventPtr)
}

declare 139 generic {
    void TkpInitKeymapInfo (TkDisplay *dispPtr)
}

declare 140 generic {
    TkRegion TkPhotoGetValidRegion (Tk_PhotoHandle handle)
}

declare 141 generic {
    TkWindow ** TkWmStackorderToplevel(TkWindow *parentPtr)
}

declare 142 generic {
    void TkFocusFree(TkMainInfo *mainPtr)
}

declare 143 generic {
    void TkClipCleanup(TkDisplay *dispPtr)
}

declare 144 generic {
    void TkGCCleanup(TkDisplay *dispPtr)
}

declare 145 {mac win aqua} {
    void TkSubtractRegion (TkRegion sra, TkRegion srcb, TkRegion dr_return)
}

declare 146 generic {
    void TkStylePkgInit (TkMainInfo *mainPtr)
}
declare 147 generic {
    void TkStylePkgFree (TkMainInfo *mainPtr)
}

declare 148 generic {
    Tk_Window TkToplevelWindowForCommand(Tcl_Interp *interp,
	    CONST char *cmdName)
}

declare 149 generic {
    CONST Tk_OptionSpec * TkGetOptionSpec (CONST char *name,
					   Tk_OptionTable optionTable)
}

##############################################################################

# Define the platform specific internal Tcl interface. These functions are
# only available on the designated platform.

interface tkIntPlat

#########################
# Unix specific functions

declare 0 x11 {
    void TkCreateXEventSource (void)
}

declare 1 x11 {
    void TkFreeWindowId (TkDisplay *dispPtr, Window w)
}

declare 2 x11 {
    void TkInitXId (TkDisplay *dispPtr)
}

declare 3 x11 {
    int TkpCmapStressed (Tk_Window tkwin, Colormap colormap)
}

declare 4 x11 {
    void TkpSync (Display *display)
}

declare 5 x11 {
    Window TkUnixContainerId (TkWindow *winPtr)
}

declare 6 x11 {
    int TkUnixDoOneXEvent (Tcl_Time *timePtr)
}

declare 7 x11 {
    void TkUnixSetMenubar (Tk_Window tkwin, Tk_Window menubar)
}

declare 8 x11 {
    int TkpScanWindowId (Tcl_Interp *interp, CONST char *string, Window *idPtr)
}

declare 9 x11 {
    void TkWmCleanup (TkDisplay *dispPtr)
}

declare 10 x11 {
    void TkSendCleanup (TkDisplay *dispPtr)
}

declare 11 x11 {
    void TkFreeXId (TkDisplay *dispPtr)
}

declare 12 x11 {
    int TkpWmSetState (TkWindow *winPtr, int state)
}

############################
# Windows specific functions

declare 0 win {
    char * TkAlignImageData (XImage *image, int alignment, int bitOrder)
}

declare 2 win {
    void TkGenerateActivateEvents (TkWindow *winPtr, int active)
}

declare 3 win {
    unsigned long TkpGetMS (void)
}

declare 4 win {
    void TkPointerDeadWindow (TkWindow *winPtr)
}

declare 5 win {
    void TkpPrintWindowId (char *buf, Window window)
}

declare 6 win {
    int TkpScanWindowId (Tcl_Interp *interp, CONST char *string, Window *idPtr)
}

declare 7 win {
    void TkpSetCapture (TkWindow *winPtr)
}

declare 8 win {
    void TkpSetCursor (TkpCursor cursor)
}

declare 9 win {
    void TkpWmSetState (TkWindow *winPtr, int state)
}

declare 10 win {
    void TkSetPixmapColormap (Pixmap pixmap, Colormap colormap)
}

declare 11 win {
    void  TkWinCancelMouseTimer (void)
}

declare 12 win {
    void  TkWinClipboardRender (TkDisplay *dispPtr, UINT format)
}

declare 13 win {
    LRESULT  TkWinEmbeddedEventProc (HWND hwnd, UINT message, \
	    WPARAM wParam, LPARAM lParam)
}

declare 14 win {
    void  TkWinFillRect (HDC dc, int x, int y, int width, int height, \
	    int pixel)
}

declare 15 win {
    COLORREF  TkWinGetBorderPixels (Tk_Window tkwin, Tk_3DBorder border, \
	    int which)
}

declare 16 win {
    HDC  TkWinGetDrawableDC (Display *display, Drawable d, TkWinDCState* state)
}

declare 17 win {
    int  TkWinGetModifierState (void)
}

declare 18 win {
    HPALETTE  TkWinGetSystemPalette (void)
}

declare 19 win {
    HWND  TkWinGetWrapperWindow (Tk_Window tkwin)
}

declare 20 win {
    int  TkWinHandleMenuEvent (HWND *phwnd, \
	    UINT *pMessage, WPARAM *pwParam, LPARAM *plParam, \
	    LRESULT *plResult)
}

declare 21 win {
    int  TkWinIndexOfColor (XColor *colorPtr)
}

declare 22 win {
    void  TkWinReleaseDrawableDC (Drawable d, HDC hdc, TkWinDCState* state)
}

declare 23 win {
    LRESULT  TkWinResendEvent (WNDPROC wndproc, HWND hwnd, XEvent *eventPtr)
}

declare 24 win {
    HPALETTE  TkWinSelectPalette (HDC dc, Colormap colormap)
}

declare 25 win {
    void  TkWinSetMenu (Tk_Window tkwin, HMENU hMenu)
}

declare 26 win {
    void  TkWinSetWindowPos (HWND hwnd, HWND siblingHwnd, int pos)
}

declare 27 win {
    void  TkWinWmCleanup (HINSTANCE hInstance)
}

declare 28 win {
    void  TkWinXCleanup (HINSTANCE hInstance)
}

declare 29 win {
    void   TkWinXInit (HINSTANCE hInstance)
}

# new for 8.1

declare 30 win {
    void TkWinSetForegroundWindow (TkWindow *winPtr)
}

declare 31 win {
    void TkWinDialogDebug (int debug)
}

declare 32 win {
    Tcl_Obj * TkWinGetMenuSystemDefault (Tk_Window tkwin, \
	    CONST char *dbName, CONST char *className)
}

declare 33 win {
    int TkWinGetPlatformId(void)
}

# new for 8.4.1

declare 34 win {
    void TkWinSetHINSTANCE (HINSTANCE hInstance)
}

########################
# Mac specific functions

declare 0 mac {
    void TkGenerateActivateEvents (TkWindow *winPtr, int active)
}

# removed duplicates from tkInt table
#declare 1 mac {
#    Pixmap TkpCreateNativeBitmap (Display *display, CONST char * source)
#}
#
#declare 2 mac {
#    void TkpDefineNativeBitmaps (void)
#}

declare 3 mac {
    unsigned long TkpGetMS (void)
}

declare 5 mac {
    void TkPointerDeadWindow (TkWindow *winPtr)
}

declare 6 mac {
    void TkpSetCapture (TkWindow *winPtr)
}

declare 7 mac {
    void TkpSetCursor (TkpCursor cursor)
}

declare 8 mac {
    void TkpWmSetState (TkWindow *winPtr, int state)
}

declare 10 mac {
    void   TkAboutDlg (void)
}

declare 13 mac {
    Window  TkGetTransientMaster (TkWindow *winPtr)
}

declare 14 mac {
    int  TkGenerateButtonEvent (int x, int y, \
	    Window window, unsigned int state)
}

declare 16 mac {
    void  TkGenWMDestroyEvent (Tk_Window tkwin)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 17 mac {
#    void  TkGenWMConfigureEvent (Tk_Window tkwin, int x, int y, \
#	    int width, int height, int flags)
#}

declare 18 mac {
    unsigned int TkMacButtonKeyState (void)
}

declare 19 mac {
    void  TkMacClearMenubarActive (void)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 20 mac {
#    int  TkMacConvertEvent (EventRecord *eventPtr)
#}

declare 21 mac {
    int  TkMacDispatchMenuEvent (int menuID, int index)
}

declare 22 mac {
    void  TkMacInstallCursor (int resizeOverride)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 23 mac {
#    int  TkMacConvertTkEvent (EventRecord *eventPtr, Window window)
#}

declare 24 mac {
    void  TkMacHandleTearoffMenu (void)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 26 mac {
#    void  TkMacInvalClipRgns (TkWindow *winPtr)
#}

declare 27 mac {
    void  TkMacDoHLEvent (EventRecord *theEvent)
}

declare 29 mac {
    Time  TkMacGenerateTime (void)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 30 mac {
#    GWorldPtr  TkMacGetDrawablePort (Drawable drawable)
#}

declare 31 mac {
    TkWindow *  TkMacGetScrollbarGrowWindow (TkWindow *winPtr)
}

declare 32 mac {
    Window   TkMacGetXWindow (WindowRef macWinPtr)
}

declare 33 mac {
    int  TkMacGrowToplevel (WindowRef whichWindow, Point start)
}

declare 34 mac {
    void   TkMacHandleMenuSelect (long mResult, int optionKeyPressed)
}

# removed duplicates from tkPlat table (tk.decls)
#declare 35 mac {
#    int  TkMacHaveAppearance (void)
#}
#
#declare 36 mac {
#    void  TkMacInitAppleEvents (Tcl_Interp *interp)
#}
#
#declare 37 mac {
#    void   TkMacInitMenus (Tcl_Interp  *interp)
#}

declare 38 mac {
    void  TkMacInvalidateWindow (MacDrawable *macWin, int flag)
}

declare 39 mac {
    int  TkMacIsCharacterMissing (Tk_Font tkfont, unsigned int searchChar)
}

declare 40 mac {
    void  TkMacMakeRealWindowExist (TkWindow *winPtr)
}

declare 41 mac {
    BitMapPtr TkMacMakeStippleMap(Drawable d1, Drawable d2)
}

declare 42 mac {
    void  TkMacMenuClick (void)
}

declare 43 mac {
    void  TkMacRegisterOffScreenWindow (Window window, GWorldPtr portPtr)
}

declare 44 mac {
    int  TkMacResizable (TkWindow *winPtr)
}

declare 46 mac {
    void  TkMacSetHelpMenuItemCount (void)
}

declare 47 mac {
    void  TkMacSetScrollbarGrow (TkWindow *winPtr, int flag)
}

declare 48 mac {
    void  TkMacSetUpClippingRgn (Drawable drawable)
}

declare 49 mac {
    void  TkMacSetUpGraphicsPort (GC gc)
}

declare 50 mac {
    void   TkMacUpdateClipRgn (TkWindow *winPtr)
}

declare 51 mac {
    void  TkMacUnregisterMacWindow (GWorldPtr portPtr)
}

declare 52 mac {
    int  TkMacUseMenuID (short macID)
}

declare 53 mac {
    RgnHandle  TkMacVisableClipRgn (TkWindow *winPtr)
}

declare 54 mac {
    void  TkMacWinBounds (TkWindow *winPtr, Rect *geometry)
}

declare 55 mac {
    void  TkMacWindowOffset (WindowRef wRef, int *xOffset, int *yOffset)
}

declare 57 mac {
    int   TkSetMacColor (unsigned long pixel, RGBColor *macColor)
}

declare 58 mac {
    void   TkSetWMName (TkWindow *winPtr, Tk_Uid titleUid)
}

declare 59 mac {
    void  TkSuspendClipboard (void)
}

declare 61 mac {
    int  TkMacZoomToplevel (WindowPtr whichWindow, Point where, short zoomPart)
}

declare 62 mac {
    Tk_Window Tk_TopCoordsToWindow (Tk_Window tkwin, \
	    int rootX, int rootY, int *newX, int *newY)
}

declare 63 mac {
    MacDrawable * TkMacContainerId (TkWindow *winPtr)
}

declare 64 mac {
    MacDrawable * TkMacGetHostToplevel  (TkWindow *winPtr)
}

declare 65 mac {
    void TkMacPreprocessMenu (void)
}

declare 66 mac {
    int TkpIsWindowFloating (WindowRef window)
}

########################
# Mac OS X specific functions

declare 0 aqua {
    void TkGenerateActivateEvents (TkWindow *winPtr, int active)
}

# removed duplicates from tkInt table
#declare 1 aqua {
#    Pixmap TkpCreateNativeBitmap (Display *display, CONST char * source)
#}
#
#declare 2 aqua {
#    void TkpDefineNativeBitmaps (void)
#}

declare 3 aqua {
    void TkPointerDeadWindow (TkWindow *winPtr)
}

declare 4 aqua {
    void TkpSetCapture (TkWindow *winPtr)
}

declare 5 aqua {
    void TkpSetCursor (TkpCursor cursor)
}

declare 6 aqua {
    void TkpWmSetState (TkWindow *winPtr, int state)
}

declare 7 aqua {
    void   TkAboutDlg (void)
}

declare 8 aqua {
    unsigned int TkMacOSXButtonKeyState (void)
}

declare 9 aqua {
    void  TkMacOSXClearMenubarActive (void)
}

declare 10 aqua {
    int  TkMacOSXDispatchMenuEvent (int menuID, int index)
}

declare 11 aqua {
    void  TkMacOSXInstallCursor (int resizeOverride)
}

declare 12 aqua {
    void  TkMacOSXHandleTearoffMenu (void)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 13 aqua {
#    void  TkMacOSXInvalClipRgns (TkWindow *winPtr)
#}

declare 14 aqua {
    int  TkMacOSXDoHLEvent (EventRecord *theEvent)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 15 aqua {
#    GWorldPtr  TkMacOSXGetDrawablePort (Drawable drawable)
#}

declare 16 aqua {
    Window   TkMacOSXGetXWindow (WindowRef macWinPtr)
}

declare 17 aqua {
    int  TkMacOSXGrowToplevel (WindowRef whichWindow, Point start)
}

declare 18 aqua {
    void   TkMacOSXHandleMenuSelect (long mResult, int optionKeyPressed)
}

# removed duplicates from tkPlat table (tk.decls)
#declare 19 aqua {
#    void  TkMacOSXInitAppleEvents (Tcl_Interp *interp)
#}
#
#declare 20 aqua {
#    void   TkMacOSXInitMenus (Tcl_Interp  *interp)
#}

declare 21 aqua {
    void  TkMacOSXInvalidateWindow (MacDrawable *macWin, int flag)
}

declare 22 aqua {
    int  TkMacOSXIsCharacterMissing (Tk_Font tkfont, unsigned int searchChar)
}

declare 23 aqua {
    void  TkMacOSXMakeRealWindowExist (TkWindow *winPtr)
}

declare 24 aqua {
    BitMapPtr TkMacOSXMakeStippleMap(Drawable d1, Drawable d2)
}

declare 25 aqua {
    void  TkMacOSXMenuClick (void)
}

declare 26 aqua {
    void  TkMacOSXRegisterOffScreenWindow (Window window, GWorldPtr portPtr)
}

declare 27 aqua {
    int  TkMacOSXResizable (TkWindow *winPtr)
}

declare 28 aqua {
    void  TkMacOSXSetHelpMenuItemCount (void)
}

declare 29 aqua {
    void  TkMacOSXSetScrollbarGrow (TkWindow *winPtr, int flag)
}

declare 30 aqua {
    void  TkMacOSXSetUpClippingRgn (Drawable drawable)
}

declare 31 aqua {
    void  TkMacOSXSetUpGraphicsPort (GC gc, GWorldPtr destPort)
}

declare 32 aqua {
    void   TkMacOSXUpdateClipRgn (TkWindow *winPtr)
}

declare 33 aqua {
    void  TkMacOSXUnregisterMacWindow (WindowRef portPtr)
}

declare 34 aqua {
    int  TkMacOSXUseMenuID (short macID)
}

declare 35 aqua {
    RgnHandle  TkMacOSXVisableClipRgn (TkWindow *winPtr)
}

declare 36 aqua {
    void  TkMacOSXWinBounds (TkWindow *winPtr, Rect *geometry)
}

declare 37 aqua {
    void  TkMacOSXWindowOffset (WindowRef wRef, int *xOffset, int *yOffset)
}

declare 38 aqua {
    int   TkSetMacColor (unsigned long pixel, RGBColor *macColor)
}

declare 39 aqua {
    void   TkSetWMName (TkWindow *winPtr, Tk_Uid titleUid)
}

declare 40 aqua {
    void  TkSuspendClipboard (void)
}

declare 41 aqua {
    int  TkMacOSXZoomToplevel (WindowPtr whichWindow, Point where, short zoomPart)
}

declare 42 aqua {
    Tk_Window Tk_TopCoordsToWindow (Tk_Window tkwin, \
	    int rootX, int rootY, int *newX, int *newY)
}

declare 43 aqua {
    MacDrawable * TkMacOSXContainerId (TkWindow *winPtr)
}

declare 44 aqua {
    MacDrawable * TkMacOSXGetHostToplevel  (TkWindow *winPtr)
}

declare 45 aqua {
    void TkMacOSXPreprocessMenu (void)
}

declare 46 aqua {
    int  TkpIsWindowFloating (WindowRef window)
}

declare 47 aqua {
    Tk_Window TkMacOSXGetCapture (void)
}

declare 49 aqua {
    Window  TkGetTransientMaster (TkWindow *winPtr)
}

declare 50 aqua {
    int  TkGenerateButtonEvent (int x, int y, \
 	    Window window, unsigned int state)
}

declare 51 aqua {
    void  TkGenWMDestroyEvent (Tk_Window tkwin)
}

# removed duplicate from tkPlat table (tk.decls)
#declare 52 aqua {
#    void  TkGenWMConfigureEvent (Tk_Window tkwin, int x, int y, \
# 	    int width, int height, int flags)
#}

declare 53 aqua {
    unsigned long TkpGetMS (void)
}

##############################################################################

# Define the platform specific internal Xlib interfaces. These functions are
# only available on the designated platform.

interface tkIntXlib

# X functions for Windows

declare 0 win {
    void XSetDashes (Display* display, GC gc, int dash_offset,
	    _Xconst char* dash_list, int n)
}

declare 1 win {
    XModifierKeymap* XGetModifierMapping (Display* d)
}

declare 2 win {
    XImage * XCreateImage (Display* d, Visual* v, unsigned int ui1, int i1, \
	    int i2, char* cp, unsigned int ui2, unsigned int ui3, int i3, \
	    int i4)

}

declare 3 win {
    XImage *XGetImage (Display* d, Drawable dr, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2, unsigned long ul, int i3)
}

declare 4 win {
    char *XGetAtomName (Display* d,Atom a)

}

declare 5 win {
    char *XKeysymToString (KeySym k)
}

declare 6 win {
    Colormap XCreateColormap (Display* d, Window w, Visual* v, int i)

}

declare 7 win {
    Cursor XCreatePixmapCursor (Display* d, Pixmap p1, Pixmap p2, \
	    XColor* x1, XColor* x2, \
	    unsigned int ui1, unsigned int ui2)
}

declare 8 win {
    Cursor XCreateGlyphCursor (Display* d, Font f1, Font f2, \
	    unsigned int ui1, unsigned int ui2, XColor* x1, XColor* x2)
}

declare 9 win {
    GContext XGContextFromGC (GC g)
}

declare 10 win {
    XHostAddress *XListHosts (Display* d, int* i, Bool* b)
}

# second parameter was of type KeyCode
declare 11 win {
    KeySym XKeycodeToKeysym (Display* d, unsigned int k, int i)
}

declare 12 win {
    KeySym XStringToKeysym (_Xconst char* c)
}

declare 13 win {
    Window XRootWindow (Display* d, int i)
}

declare 14 win {
    XErrorHandler XSetErrorHandler  (XErrorHandler x)
}

declare 15 win {
    Status XIconifyWindow (Display* d, Window w, int i)
}

declare 16 win {
    Status XWithdrawWindow (Display* d, Window w, int i)
}

declare 17 win {
    Status XGetWMColormapWindows (Display* d, Window w, Window** wpp, int* ip)
}

declare 18 win {
    Status XAllocColor (Display* d, Colormap c, XColor* xp)
}

declare 19 win {
    void XBell (Display* d, int i)
}

declare 20 win {
    void XChangeProperty (Display* d, Window w, Atom a1, Atom a2, int i1, \
	    int i2, _Xconst unsigned char* c, int i3)
}

declare 21 win {
    void XChangeWindowAttributes (Display* d, Window w, unsigned long ul, \
	    XSetWindowAttributes* x)
}

declare 22 win {
    void XClearWindow (Display* d, Window w)
}

declare 23 win {
    void XConfigureWindow (Display* d, Window w, unsigned int i, \
	    XWindowChanges* x)
}

declare 24 win {
    void XCopyArea (Display* d, Drawable dr1, Drawable dr2, GC g, int i1, \
	    int i2, unsigned int ui1, \
	    unsigned int ui2, int i3, int i4)
}

declare 25 win {
    void XCopyPlane (Display* d, Drawable dr1, Drawable dr2, GC g, int i1, \
	    int i2, unsigned int ui1, \
	    unsigned int ui2, int i3, int i4, unsigned long ul)
}

declare 26 win {
    Pixmap XCreateBitmapFromData(Display* display, Drawable d, \
	    _Xconst char* data, unsigned int width,unsigned int height)
}

declare 27 win {
    void XDefineCursor (Display* d, Window w, Cursor c)
}

declare 28 win {
    void XDeleteProperty (Display* d, Window w, Atom a)
}

declare 29 win {
    void XDestroyWindow (Display* d, Window w)
}

declare 30 win {
    void XDrawArc (Display* d, Drawable dr, GC g, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2, int i3, int i4)
}

declare 31 win {
    void XDrawLines (Display* d, Drawable dr, GC g, XPoint* x, int i1, int i2)
}

declare 32 win {
    void XDrawRectangle (Display* d, Drawable dr, GC g, int i1, int i2,\
	    unsigned int ui1, unsigned int ui2)
}

declare 33 win {
    void XFillArc (Display* d, Drawable dr, GC g, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2, int i3, int i4)
}

declare 34 win {
    void XFillPolygon (Display* d, Drawable dr, GC g, XPoint* x, \
	    int i1, int i2, int i3)
}

declare 35 win {
    void XFillRectangles (Display* d, Drawable dr, GC g, XRectangle* x, int i)
}

declare 36 win {
    void XForceScreenSaver (Display* d, int i)
}

declare 37 win {
    void XFreeColormap (Display* d, Colormap c)
}

declare 38 win {
    void XFreeColors (Display* d, Colormap c, \
	    unsigned long* ulp, int i, unsigned long ul)
}

declare 39 win {
    void XFreeCursor (Display* d, Cursor c)
}

declare 40 win {
    void XFreeModifiermap (XModifierKeymap* x)
}

declare 41 win {
    Status XGetGeometry (Display* d, Drawable dr, Window* w, int* i1, \
	    int* i2, unsigned int* ui1, unsigned int* ui2, unsigned int* ui3, \
	    unsigned int* ui4)
}

declare 42 win {
    void XGetInputFocus (Display* d, Window* w, int* i)
}

declare 43 win {
    int XGetWindowProperty (Display* d, Window w, Atom a1, long l1, long l2, \
	    Bool b, Atom a2, Atom* ap, int* ip, unsigned long* ulp1, \
	    unsigned long* ulp2, unsigned char** cpp)
}

declare 44 win {
    Status XGetWindowAttributes (Display* d, Window w, XWindowAttributes* x)
}

declare 45 win {
    int XGrabKeyboard (Display* d, Window w, Bool b, int i1, int i2, Time t)
}

declare 46 win {
    int XGrabPointer (Display* d, Window w1, Bool b, unsigned int ui, \
	    int i1, int i2, Window w2, Cursor c, Time t)
}

declare 47 win {
    KeyCode XKeysymToKeycode (Display* d, KeySym k)
}

declare 48 win {
    Status XLookupColor (Display* d, Colormap c1, _Xconst char* c2, \
	    XColor* x1, XColor* x2)
}

declare 49 win {
    void XMapWindow (Display* d, Window w)
}

declare 50 win {
    void XMoveResizeWindow (Display* d, Window w, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2)
}

declare 51 win {
    void XMoveWindow (Display* d, Window w, int i1, int i2)
}

declare 52 win {
    void XNextEvent (Display* d, XEvent* x)
}

declare 53 win {
    void XPutBackEvent (Display* d, XEvent* x)
}

declare 54 win {
    void XQueryColors (Display* d, Colormap c, XColor* x, int i)
}

declare 55 win {
    Bool XQueryPointer (Display* d, Window w1, Window* w2, Window* w3, \
	    int* i1, int* i2, int* i3, int* i4, unsigned int* ui)
}

declare 56 win {
    Status XQueryTree (Display* d, Window w1, Window* w2, Window* w3, \
	    Window** w4, unsigned int* ui)
}

declare 57 win {
    void XRaiseWindow (Display* d, Window w)
}

declare 58 win {
    void XRefreshKeyboardMapping (XMappingEvent* x)
}

declare 59 win {
    void XResizeWindow (Display* d, Window w, unsigned int ui1, \
	    unsigned int ui2)
}

declare 60 win {
    void XSelectInput (Display* d, Window w, long l)
}

declare 61 win {
    Status XSendEvent (Display* d, Window w, Bool b, long l, XEvent* x)
}

declare 62 win {
    void XSetCommand (Display* d, Window w, CONST char** c, int i)
}

declare 63 win {
    void XSetIconName (Display* d, Window w, _Xconst char* c)
}

declare 64 win {
    void XSetInputFocus (Display* d, Window w, int i, Time t)
}

declare 65 win {
    void XSetSelectionOwner (Display* d, Atom a, Window w, Time t)
}

declare 66 win {
    void XSetWindowBackground (Display* d, Window w, unsigned long ul)
}

declare 67 win {
    void XSetWindowBackgroundPixmap (Display* d, Window w, Pixmap p)
}

declare 68 win {
    void XSetWindowBorder (Display* d, Window w, unsigned long ul)
}

declare 69 win {
    void XSetWindowBorderPixmap (Display* d, Window w, Pixmap p)
}

declare 70 win {
    void XSetWindowBorderWidth (Display* d, Window w, unsigned int ui)
}

declare 71 win {
    void XSetWindowColormap (Display* d, Window w, Colormap c)
}

declare 72 win {
    Bool XTranslateCoordinates (Display* d, Window w1, Window w2, int i1,\
	    int i2, int* i3, int* i4, Window* w3)
}

declare 73 win {
    void XUngrabKeyboard (Display* d, Time t)
}

declare 74 win {
    void XUngrabPointer (Display* d, Time t) 
}

declare 75 win {
    void XUnmapWindow (Display* d, Window w)
}

declare 76 win {
    void XWindowEvent (Display* d, Window w, long l, XEvent* x)
}

declare 77 win {
    void XDestroyIC (XIC x)
}

declare 78 win {
    Bool XFilterEvent (XEvent* x, Window w)
}

declare 79 win {
    int XmbLookupString (XIC xi, XKeyPressedEvent* xk, \
	    char* c, int i, KeySym* k, Status* s)
}

declare 80 win {
    void TkPutImage (unsigned long *colors, \
	    int ncolors, Display* display, Drawable d, \
	    GC gc, XImage* image, int src_x, int src_y, \
	    int dest_x, int dest_y, unsigned int width, \
	    unsigned int height)
}
# This slot is reserved for use by the clipping rectangle patch:
#  declare 81 win {
#      XSetClipRectangles(Display *display, GC gc, int clip_x_origin, \
#  	    int clip_y_origin, XRectangle rectangles[], int n, int ordering)
#  }

declare 82 win {
    Status XParseColor (Display *display, Colormap map, \
          _Xconst char* spec, XColor *colorPtr)
}

declare 83 win {
    GC XCreateGC(Display* display, Drawable d, \
	    unsigned long valuemask, XGCValues* values)
}

declare 84 win {
    void XFreeGC(Display* display, GC gc)
}

declare 85 win {
    Atom XInternAtom(Display* display,_Xconst char* atom_name, \
	    Bool only_if_exists)
}

declare 86 win {
    void XSetBackground(Display* display, GC gc, \
	    unsigned long foreground)
}

declare 87 win {
    void XSetForeground(Display* display, GC gc, \
	    unsigned long foreground)
}

declare 88 win {
    void XSetClipMask(Display* display, GC gc, Pixmap pixmap)
}

declare 89 win {
    void XSetClipOrigin(Display* display, GC gc, \
	    int clip_x_origin, int clip_y_origin)
}

declare 90 win {
    void XSetTSOrigin(Display* display, GC gc, \
	    int ts_x_origin, int ts_y_origin)
}

declare 91 win {
    void XChangeGC(Display * d, GC gc, unsigned long mask, XGCValues *values)
}

declare 92 win {
    void XSetFont(Display *display, GC gc, Font font)
}

declare 93 win {
    void XSetArcMode(Display *display, GC gc, int arc_mode)
}

declare 94 win {
    void XSetStipple(Display *display, GC gc, Pixmap stipple)
}

declare 95 win {
    void XSetFillRule(Display *display, GC gc, int fill_rule)
}

declare 96 win {
    void XSetFillStyle(Display *display, GC gc, int fill_style)
}

declare 97 win {
    void XSetFunction(Display *display, GC gc, int function)
}

declare 98 win {
    void XSetLineAttributes(Display *display, GC gc, \
	    unsigned int line_width, int line_style, \
	    int cap_style, int join_style)
}

declare 99 win {
    int _XInitImageFuncPtrs(XImage *image)
}

declare 100 win {
    XIC XCreateIC(void)
}

declare 101 win {
    XVisualInfo *XGetVisualInfo(Display* display, long vinfo_mask, \
	    XVisualInfo* vinfo_template, int* nitems_return)
}

declare 102 win {
    void XSetWMClientMachine(Display* display, Window w, XTextProperty* text_prop)
}

declare 103 win {
    Status XStringListToTextProperty(char** list, int count, \
	    XTextProperty* text_prop_return)
}
declare 104 win {
    void XDrawLine (Display* d, Drawable dr, GC g, int x1, int y1, \
	    int x2, int y2)
}
declare 106 win {
    void XFillRectangle (Display* display, Drawable d, GC gc, \
	    int x, int y, unsigned int width, unsigned int height)
}
declare 105 win {
    void XWarpPointer (Display* d, Window s,  Window dw, int sx, int sy, \
	    unsigned int sw, unsigned int sh, int dx, int dy)
}

# X functions for Mac and Aqua

declare 0 {mac aqua} {
    void XSetDashes (Display* display, GC gc, int dash_offset,
	    _Xconst char* dash_list, int n)
}

declare 1 {mac aqua} {
    XModifierKeymap* XGetModifierMapping (Display* d)
}

declare 2 {mac aqua} {
    XImage * XCreateImage (Display* d, Visual* v, unsigned int ui1, int i1, \
	    int i2, char* cp, unsigned int ui2, unsigned int ui3, int i3, \
	    int i4)

}

declare 3 {mac aqua} {
    XImage *XGetImage (Display* d, Drawable dr, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2, unsigned long ul, int i3)
}

declare 4 {mac aqua} {
    char *XGetAtomName (Display* d,Atom a)

}

declare 5 {mac aqua} {
    char *XKeysymToString (KeySym k)
}

declare 6 {mac aqua} {
    Colormap XCreateColormap (Display* d, Window w, Visual* v, int i)

}

declare 7 {mac aqua} {
    GContext XGContextFromGC (GC g)
}

declare 8 {mac aqua} {
    KeySym XKeycodeToKeysym (Display* d, KeyCode k, int i)
}

declare 9 {mac aqua} {
    KeySym XStringToKeysym (_Xconst char* c)
}

declare 10 {mac aqua} {
    Window XRootWindow (Display* d, int i)
}

declare 11 {mac aqua} {
    XErrorHandler XSetErrorHandler  (XErrorHandler x)
}

declare 12 {mac aqua} {
    Status XAllocColor (Display* d, Colormap c, XColor* xp)
}

declare 13 {mac aqua} {
    void XBell (Display* d, int i)
}

declare 14 {mac aqua} {
    void XChangeProperty (Display* d, Window w, Atom a1, Atom a2, int i1, \
	    int i2, _Xconst unsigned char* c, int i3)
}

declare 15 {mac aqua} {
    void XChangeWindowAttributes (Display* d, Window w, unsigned long ul, \
	    XSetWindowAttributes* x)
}

declare 16 {mac aqua} {
    void XConfigureWindow (Display* d, Window w, unsigned int i, \
	    XWindowChanges* x)
}

declare 17 {mac aqua} {
    void XCopyArea (Display* d, Drawable dr1, Drawable dr2, GC g, int i1, \
	    int i2, unsigned int ui1, \
	    unsigned int ui2, int i3, int i4)
}

declare 18 {mac aqua} {
    void XCopyPlane (Display* d, Drawable dr1, Drawable dr2, GC g, int i1, \
	    int i2, unsigned int ui1, \
	    unsigned int ui2, int i3, int i4, unsigned long ul)
}

declare 19 {mac aqua} {
    Pixmap XCreateBitmapFromData(Display* display, Drawable d, \
	    _Xconst char* data, unsigned int width,unsigned int height)
}

declare 20 {mac aqua} {
    void XDefineCursor (Display* d, Window w, Cursor c)
}

declare 21 {mac aqua} {
    void XDestroyWindow (Display* d, Window w)
}

declare 22 {mac aqua} {
    void XDrawArc (Display* d, Drawable dr, GC g, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2, int i3, int i4)
}

declare 23 {mac aqua} {
    void XDrawLines (Display* d, Drawable dr, GC g, XPoint* x, int i1, int i2)
}

declare 24 {mac aqua} {
    void XDrawRectangle (Display* d, Drawable dr, GC g, int i1, int i2,\
	    unsigned int ui1, unsigned int ui2)
}

declare 25 {mac aqua} {
    void XFillArc (Display* d, Drawable dr, GC g, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2, int i3, int i4)
}

declare 26 {mac aqua} {
    void XFillPolygon (Display* d, Drawable dr, GC g, XPoint* x, \
	    int i1, int i2, int i3)
}

declare 27 {mac aqua} {
    void XFillRectangles (Display* d, Drawable dr, GC g, XRectangle* x, int i)
}

declare 28 {mac aqua} {
    void XFreeColormap (Display* d, Colormap c)
}

declare 29 {mac aqua} {
    void XFreeColors (Display* d, Colormap c, \
	    unsigned long* ulp, int i, unsigned long ul)
}

declare 30 {mac aqua} {
    void XFreeModifiermap (XModifierKeymap* x)
}

declare 31 {mac aqua} {
    Status XGetGeometry (Display* d, Drawable dr, Window* w, int* i1, \
	    int* i2, unsigned int* ui1, unsigned int* ui2, unsigned int* ui3, \
	    unsigned int* ui4)
}

declare 32 {mac aqua} {
    int XGetWindowProperty (Display* d, Window w, Atom a1, long l1, long l2, \
	    Bool b, Atom a2, Atom* ap, int* ip, unsigned long* ulp1, \
	    unsigned long* ulp2, unsigned char** cpp)
}

declare 33 {mac aqua} {
    int XGrabKeyboard (Display* d, Window w, Bool b, int i1, int i2, Time t)
}

declare 34 {mac aqua} {
    int XGrabPointer (Display* d, Window w1, Bool b, unsigned int ui, \
	    int i1, int i2, Window w2, Cursor c, Time t)
}

declare 35 {mac aqua} {
    KeyCode XKeysymToKeycode (Display* d, KeySym k)
}

declare 36 {mac aqua} {
    void XMapWindow (Display* d, Window w)
}

declare 37 {mac aqua} {
    void XMoveResizeWindow (Display* d, Window w, int i1, int i2, \
	    unsigned int ui1, unsigned int ui2)
}

declare 38 {mac aqua} {
    void XMoveWindow (Display* d, Window w, int i1, int i2)
}

declare 39 {mac aqua} {
    Bool XQueryPointer (Display* d, Window w1, Window* w2, Window* w3, \
	    int* i1, int* i2, int* i3, int* i4, unsigned int* ui)
}

declare 40 {mac aqua} {
    void XRaiseWindow (Display* d, Window w)
}

declare 41 {mac aqua} {
    void XRefreshKeyboardMapping (XMappingEvent* x)
}

declare 42 {mac aqua} {
    void XResizeWindow (Display* d, Window w, unsigned int ui1, \
	    unsigned int ui2)
}

declare 43 {mac aqua} {
    void XSelectInput (Display* d, Window w, long l)
}

declare 44 {mac aqua} {
    Status XSendEvent (Display* d, Window w, Bool b, long l, XEvent* x)
}

declare 45 {mac aqua} {
    void XSetIconName (Display* d, Window w, _Xconst char* c)
}

declare 46 {mac aqua} {
    void XSetInputFocus (Display* d, Window w, int i, Time t)
}

declare 47 {mac aqua} {
    void XSetSelectionOwner (Display* d, Atom a, Window w, Time t)
}

declare 48 {mac aqua} {
    void XSetWindowBackground (Display* d, Window w, unsigned long ul)
}

declare 49 {mac aqua} {
    void XSetWindowBackgroundPixmap (Display* d, Window w, Pixmap p)
}

declare 50 {mac aqua} {
    void XSetWindowBorder (Display* d, Window w, unsigned long ul)
}

declare 51 {mac aqua} {
    void XSetWindowBorderPixmap (Display* d, Window w, Pixmap p)
}

declare 52 {mac aqua} {
    void XSetWindowBorderWidth (Display* d, Window w, unsigned int ui)
}

declare 53 {mac aqua} {
    void XSetWindowColormap (Display* d, Window w, Colormap c)
}

declare 54 {mac aqua} {
    void XUngrabKeyboard (Display* d, Time t)
}

declare 55 {mac aqua} {
    void XUngrabPointer (Display* d, Time t) 
}

declare 56 {mac aqua} {
    void XUnmapWindow (Display* d, Window w)
}

declare 57 {mac aqua} {
    void TkPutImage (unsigned long *colors, \
	    int ncolors, Display* display, Drawable d, \
	    GC gc, XImage* image, int src_x, int src_y, \
	    int dest_x, int dest_y, unsigned int width, \
	    unsigned int height)
} 
declare 58 {mac aqua} {
    Status XParseColor (Display *display, Colormap map, \
          _Xconst char* spec, XColor *colorPtr)
}

declare 59 {mac aqua} {
    GC XCreateGC(Display* display, Drawable d, \
	    unsigned long valuemask, XGCValues* values)
}

declare 60 {mac aqua} {
    void XFreeGC(Display* display, GC gc)
}

declare 61 {mac aqua} {
    Atom XInternAtom(Display* display,_Xconst char* atom_name, \
	    Bool only_if_exists)
}

declare 62 {mac aqua} {
    void XSetBackground(Display* display, GC gc, \
	    unsigned long foreground)
}

declare 63 {mac aqua} {
    void XSetForeground(Display* display, GC gc, \
	    unsigned long foreground)
}

declare 64 {mac aqua} {
    void XSetClipMask(Display* display, GC gc, Pixmap pixmap)
}

declare 65 {mac aqua} {
    void XSetClipOrigin(Display* display, GC gc, \
	    int clip_x_origin, int clip_y_origin)
}

declare 66 {mac aqua} {
    void XSetTSOrigin(Display* display, GC gc, \
	    int ts_x_origin, int ts_y_origin)
}

declare 67 {mac aqua} {
    void XChangeGC(Display * d, GC gc, unsigned long mask, XGCValues *values)
}

declare 68 {mac aqua} {
    void XSetFont(Display *display, GC gc, Font font)
}

declare 69 {mac aqua} {
    void XSetArcMode(Display *display, GC gc, int arc_mode)
}

declare 70 {mac aqua} {
    void XSetStipple(Display *display, GC gc, Pixmap stipple)
}

declare 71 {mac aqua} {
    void XSetFillRule(Display *display, GC gc, int fill_rule)
}

declare 72 {mac aqua} {
    void XSetFillStyle(Display *display, GC gc, int fill_style)
}

declare 73 {mac aqua} {
    void XSetFunction(Display *display, GC gc, int function)
}

declare 74 {mac aqua} {
    void XSetLineAttributes(Display *display, GC gc, \
	    unsigned int line_width, int line_style, \
	    int cap_style, int join_style)
}

declare 75 {mac aqua} {
    int _XInitImageFuncPtrs(XImage *image)
}

declare 76 {mac aqua} {
    XIC XCreateIC(void)
}

declare 77 {mac aqua} {
    XVisualInfo *XGetVisualInfo(Display* display, long vinfo_mask, \
	    XVisualInfo* vinfo_template, int* nitems_return)
}

declare 78 {mac aqua} {
    void XSetWMClientMachine(Display* display, Window w, \
	    XTextProperty* text_prop)
}

declare 79 {mac aqua} {
    Status XStringListToTextProperty(char** list, int count, \
	    XTextProperty* text_prop_return)
}
declare 80 {mac aqua} {
    void XDrawSegments(Display *display, Drawable  d, GC gc, \
	    XSegment *segments, int  nsegments)
}
declare 81 {mac aqua} {
    void  XForceScreenSaver(Display* display, int mode)
}
declare 82 {mac aqua} {
    void XDrawLine (Display* d, Drawable dr, GC g, int x1, int y1, \
	    int x2, int y2)
}
declare 83 {mac aqua} {
    void XFillRectangle (Display* display, Drawable d, GC gc, \
	    int x, int y, unsigned int width, unsigned int height)
}
declare 84 {mac aqua} {
    void XClearWindow (Display* d, Window w)
}

declare 85 {mac aqua} {
    void XDrawPoint (Display* display, Drawable d, GC gc, int x, int y)
}

declare 86 {mac aqua} {
    void XDrawPoints (Display* display, Drawable d, GC gc, XPoint *points, \
	    int npoints, int mode)
}

declare 87 {mac aqua} {
    void XWarpPointer (Display* display, Window src_w, Window dest_w, \
	    int src_x, int src_y, unsigned int src_width, \
	    unsigned int src_height, int dest_x, int dest_y)
}

declare 88 {mac aqua} {
    void XQueryColor (Display *display, Colormap colormap, XColor *def_in_out)
}

declare 89 {mac aqua} {
    void XQueryColors (Display *display, Colormap colormap, \
	    XColor *defs_in_out, int ncolors)
}

declare 90 {mac aqua} {
    Status XQueryTree (Display* d, Window w1, Window* w2, Window* w3, \
	    Window** w4, unsigned int* ui)
}
