/* 
 * tkMacWindowMgr.c --
 *
 *	Implements common window manager functions for the Macintosh.
 *
 * Copyright (c) 1995-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Events.h>
#include <Dialogs.h>
#include <EPPC.h>
#include <Windows.h>
#include <ToolUtils.h>
#include <DiskInit.h>
#include <LowMem.h>
#include <Timer.h>
#include <Sound.h>

#include "tkInt.h"
#include "tkPort.h"
#include "tkMacInt.h"

#define TK_DEFAULT_ABOUT 128

/*
 * Declarations of global variables defined in this file.
 */

int tkMacAppInFront = true;		 /* Boolean variable for determining 
					  * if we are the frontmost app. */

/*
 *  Non-standard event types that can be passed to HandleEvent.
 * These are defined and used by Netscape's plugin architecture.
 */
#define getFocusEvent       (osEvt + 16)
#define loseFocusEvent      (osEvt + 17)
#define adjustCursorEvent   (osEvt + 18)

/*
 * Declarations of static variables used in this file.
 */

static int	 gEatButtonUp = 0;	 /* 1 if we need to eat the next
					  * up event */
static Tk_Window gGrabWinPtr = NULL;	 /* Current grab window, NULL if no grab. */
static Tk_Window gKeyboardWinPtr = NULL; /* Current keyboard grab window. */
static RgnHandle gDamageRgn = NULL;	 /* Damage region used for handling
					  * screen updates. */
/*
 * Forward declarations of procedures used in this file.
 */

static void	BringWindowForward _ANSI_ARGS_((WindowRef wRef));
static int 	CheckEventsAvail _ANSI_ARGS_((void));
static int 	GenerateActivateEvents _ANSI_ARGS_((EventRecord *eventPtr,
			Window window));
static int 	GenerateFocusEvent _ANSI_ARGS_((EventRecord *eventPtr,
			Window window));
static int	GenerateKeyEvent _ANSI_ARGS_((EventRecord *eventPtr,
			Window window, UInt32 savedCode));
static int	GenerateUpdateEvent _ANSI_ARGS_((EventRecord *eventPtr,
			Window window));
static void 	GenerateUpdates _ANSI_ARGS_((RgnHandle updateRgn,
			TkWindow *winPtr));
static int 	GeneratePollingEvents _ANSI_ARGS_((void));	
static int 	GeneratePollingEvents2 _ANSI_ARGS_((Window window,
	                int adjustCursor));	
static OSErr	TellWindowDefProcToCalcRegions _ANSI_ARGS_((WindowRef wRef));
static int	WindowManagerMouse _ANSI_ARGS_((EventRecord *theEvent,
		    Window window));


/*
 *----------------------------------------------------------------------
 *
 * WindowManagerMouse --
 *
 *	This function determines if a button event is a "Window Manager"
 *	function or an event that should be passed to Tk's event
 *	queue.
 *
 * Results:
 *	Return true if event was placed on Tk's event queue.
 *
 * Side effects:
 *	Depends on where the button event occurs.
 *
 *----------------------------------------------------------------------
 */

static int
WindowManagerMouse(
    EventRecord *eventPtr,	/* Macintosh event record. */
    Window window)		/* Window pointer. */
{
    WindowRef whichWindow, frontWindow;
    Tk_Window tkwin;
    Point where, where2;
    int xOffset, yOffset;
    short windowPart;
    TkDisplay *dispPtr;
				
    frontWindow = FrontWindow();

    /* 
     * The window manager only needs to know about mouse down events
     * and sometimes we need to "eat" the mouse up.  Otherwise, we
     * just pass the event to Tk.
     */
    if (eventPtr->what == mouseUp) {
	if (gEatButtonUp) {
	    gEatButtonUp = false;
	    return false;
	}
	return TkGenerateButtonEvent(eventPtr->where.h, eventPtr->where.v, 
		window, TkMacButtonKeyState());
    }

    windowPart = FindWindow(eventPtr->where, &whichWindow);
    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    switch (windowPart) {
	case inSysWindow:
	    SystemClick(eventPtr, (GrafPort *) whichWindow);
	    return false;
	case inDrag:
	    if (whichWindow != frontWindow) {
		if (!(eventPtr->modifiers & cmdKey)) {
		    if ((gGrabWinPtr != NULL) && (gGrabWinPtr != tkwin)) {
			SysBeep(1);
			return false;
		    }
		}
	    }

	    /*
	     * Call DragWindow to move the window around.  It will
	     * also eat the mouse up event.
	     */
	    SetPort((GrafPort *) whichWindow);
	    where.h = where.v = 0;
	    LocalToGlobal(&where);
	    DragWindow(whichWindow, eventPtr->where,
		    &tcl_macQdPtr->screenBits.bounds);
	    gEatButtonUp = false;
			
	    where2.h = where2.v = 0;
	    LocalToGlobal(&where2);
	    if (EqualPt(where, where2)) {
		return false;
	    }

	    TkMacWindowOffset(whichWindow, &xOffset, &yOffset);
	    where2.h -= xOffset;
	    where2.v -= yOffset;
	    TkGenWMConfigureEvent(tkwin, where2.h, where2.v,
		    -1, -1, TK_LOCATION_CHANGED);
	    return true;
	case inGrow:
	case inContent:
	    if (whichWindow != frontWindow ) {
		/*
		 * This click moves the window forward.  We don't want
		 * the corasponding mouse-up to be reported to the application
		 * or else it will mess up some Tk scripts.
		 */
		if ((gGrabWinPtr != NULL) && (gGrabWinPtr != tkwin)) {
		    SysBeep(1);
		    return false;
		}
		BringWindowForward(whichWindow);
		gEatButtonUp = true;
		SetPort((GrafPort *) whichWindow);
		return false;
	    } else {
		/*
		 * Generally the content region is the domain of Tk
		 * sub-windows.  However, one exception is the grow
		 * region.  A button down in this area will be handled
		 * by the window manager.  Note: this means that Tk 
		 * may not get button down events in this area!
		 */

		if (TkMacGrowToplevel(whichWindow, eventPtr->where) == true) {
		    return true;
		} else {
		    return TkGenerateButtonEvent(eventPtr->where.h,
			    eventPtr->where.v, window, TkMacButtonKeyState());
		}
	    }
	case inGoAway:
	    if (TrackGoAway( whichWindow, eventPtr->where)) {
		if (tkwin == NULL) {
		    return false;
		}
		TkGenWMDestroyEvent(tkwin);
		return true;
	    }
	    return false;
	case inMenuBar:
	    {
		int oldMode;
		KeyMap theKeys;

		GetKeys(theKeys);
		oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
		TkMacClearMenubarActive();
		TkMacHandleMenuSelect(MenuSelect(eventPtr->where),
			theKeys[1] & 4);
		Tcl_SetServiceMode(oldMode);
		return true; /* TODO: may not be on event on queue. */
	    }
	case inZoomIn:
	case inZoomOut:
	    if (TkMacZoomToplevel(whichWindow, eventPtr->where, windowPart)
		    == true) {
		return true;
	    } else {
		return false;
	    }
	default:
	    return false;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkAboutDlg --
 *
 *	Displays the default Tk About box.  This code uses Macintosh
 *	resources to define the content of the About Box.
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
TkAboutDlg()
{
    DialogPtr aboutDlog;
    short itemHit = -9;
	
    aboutDlog = GetNewDialog(128, NULL, (void*)(-1));
	
    if (!aboutDlog) {
	return;
    }
	
    SelectWindow((WindowRef) aboutDlog);
	
    while (itemHit != 1) {
	ModalDialog( NULL, &itemHit);
    }
    DisposDialog(aboutDlog);
    aboutDlog = NULL;
	
    SelectWindow(FrontWindow());

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateUpdateEvent --
 *
 *	Given a Macintosh update event this function generates all the
 *	X update events needed by Tk.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.  
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateUpdateEvent(
    EventRecord *eventPtr,	/* Incoming Mac event */
    Window window)		/* Root X window for event. */
{
    WindowRef macWindow;
    register TkWindow *winPtr;
    TkDisplay *dispPtr;
	
    dispPtr = TkGetDisplayList();
    winPtr = (TkWindow *) Tk_IdToWindow(dispPtr->display, window);

    if (winPtr == NULL) {
	 return false;
    }
    
    if (gDamageRgn == NULL) {
	gDamageRgn = NewRgn();
    }

    /*
     * After the call to BeginUpdate the visable region (visRgn) of the 
     * window is equal to the intersection of the real visable region and
     * the update region for this event.  We use this region in all of our
     * calculations.
     */

    if (eventPtr->message != NULL) {
	macWindow = (WindowRef) TkMacGetDrawablePort(window);
	BeginUpdate(macWindow);
	GenerateUpdates(macWindow->visRgn, winPtr);
	EndUpdate(macWindow);
	return true;
    } else {
	/*
	 * This event didn't come from the system.  This might
	 * occur if we are running from inside of Netscape.
	 * In this we shouldn't call BeginUpdate as the vis region
	 * may be NULL.
	 */
	RgnHandle rgn;
	Rect bounds;
	
	rgn = NewRgn();
	TkMacWinBounds(winPtr, &bounds);
	RectRgn(rgn, &bounds);
	GenerateUpdates(rgn, winPtr);
	DisposeRgn(rgn);
	return true;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateUpdates --
 *
 *	Given a Macintosh update region and a Tk window this function
 *	geneates a X damage event for the window if it is within the
 *	update region.  The function will then recursivly have each
 *	damaged window generate damage events for its child windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateUpdates(
    RgnHandle updateRgn,
    TkWindow *winPtr)
{
    TkWindow *childPtr;
    XEvent event;
    Rect bounds;

    TkMacWinBounds(winPtr, &bounds);
	
    if (bounds.top > (*updateRgn)->rgnBBox.bottom ||
	    (*updateRgn)->rgnBBox.top > bounds.bottom ||
	    bounds.left > (*updateRgn)->rgnBBox.right ||
	    (*updateRgn)->rgnBBox.left > bounds.right ||
	    !RectInRgn(&bounds, updateRgn)) {
	return;
    }

    event.xany.serial = Tk_Display(winPtr)->request;
    event.xany.send_event = false;
    event.xany.window = Tk_WindowId(winPtr);
    event.xany.display = Tk_Display(winPtr);
	
    event.type = Expose;

    /* 
     * Compute the bounding box of the area that the damage occured in.
     */

    /*
     * CopyRgn(TkMacVisableClipRgn(winPtr), rgn);
     * TODO: this call doesn't work doing resizes!!!
     */
    RectRgn(gDamageRgn, &bounds);
    SectRgn(gDamageRgn, updateRgn, gDamageRgn);
    OffsetRgn(gDamageRgn, -bounds.left, -bounds.top);
    event.xexpose.x = (**gDamageRgn).rgnBBox.left;
    event.xexpose.y = (**gDamageRgn).rgnBBox.top;
    event.xexpose.width = (**gDamageRgn).rgnBBox.right -
	(**gDamageRgn).rgnBBox.left;
    event.xexpose.height = (**gDamageRgn).rgnBBox.bottom -
	(**gDamageRgn).rgnBBox.top;
    event.xexpose.count = 0;
    
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

    /*
     * Generate updates for the children of this window
     */
     
    for (childPtr = winPtr->childList; childPtr != NULL;
				       childPtr = childPtr->nextPtr) {
	if (!Tk_IsMapped(childPtr) || Tk_IsTopLevel(childPtr)) {
	    continue;
	}

	GenerateUpdates(updateRgn, childPtr);
    }
    
    /*
     * Generate updates for any contained windows
     */

    if (Tk_IsContainer(winPtr)) {
	childPtr = TkpGetOtherWindow(winPtr);
	if (childPtr != NULL && Tk_IsMapped(childPtr)) {
	    GenerateUpdates(updateRgn, childPtr);
	}
	    
	/*
	 * NOTE: Here we should handle out of process embedding.
	 */
		    
    }	

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenerateButtonEvent --
 *
 *	Given a global x & y position and the button key status this 
 *	procedure generates the appropiate X button event.  It also 
 *	handles the state changes needed to implement implicit grabs.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *	Grab state may also change.
 *
 *----------------------------------------------------------------------
 */

int
TkGenerateButtonEvent(
    int x,		/* X location of mouse */
    int y,		/* Y location of mouse */
    Window window,	/* X Window containing button event. */
    unsigned int state)	/* Button Key state suitable for X event */
{
    WindowRef whichWin, frontWin;
    Point where;
    Tk_Window tkwin;
    int dummy;
    TkDisplay *dispPtr;

    /* 
     * ButtonDown events will always occur in the front
     * window.  ButtonUp events, however, may occur anywhere
     * on the screen.  ButtonUp events should only be sent
     * to Tk if in the front window or during an implicit grab.
     */
    where.h = x;
    where.v = y;
    FindWindow(where, &whichWin);
    frontWin = FrontWindow();
			
    if ((frontWin == NULL) || (frontWin != whichWin && gGrabWinPtr == NULL)) {
	return false;
    }

    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    
    GlobalToLocal(&where);
    if (tkwin != NULL) {
	tkwin = Tk_TopCoordsToWindow(tkwin, where.h, where.v, &dummy, &dummy);
    }

    Tk_UpdatePointer(tkwin, x,  y, state);

    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateActivateEvents --
 *
 *	Generate Activate/Deactivate events from a Macintosh Activate 
 *	event.  Note, the activate-on-foreground bit must be set in the 
 *	SIZE flags to ensure we get Activate/Deactivate in addition to 
 *	Susspend/Resume events.
 *
 * Results:
 *	Returns true if events were generate.
 *
 * Side effects:
 *	Queue events on Tk's event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateActivateEvents(
    EventRecord *eventPtr,	/* Incoming Mac event */
    Window window)		/* Root X window for event. */
{
    TkWindow *winPtr;
    TkDisplay *dispPtr;
    
    dispPtr = TkGetDisplayList();
    winPtr = (TkWindow *) Tk_IdToWindow(dispPtr->display, window);
    if (winPtr == NULL || winPtr->window == None) {
	return false;
    }

    TkGenerateActivateEvents(winPtr,
	    (eventPtr->modifiers & activeFlag) ? 1 : 0);
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * XSetInputFocus --
 *
 *	Change the focus window for the application.
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
XSetInputFocus(
    Display* display,
    Window focus,
    int revert_to,
    Time time)
{
    /*
     * Don't need to do a thing.  Tk manages the focus for us.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkpChangeFocus --
 *
 *	This procedure is a stub on the Mac because we always own the
 *	focus if we are a front most application.
 *
 * Results:
 *	The return value is the serial number of the command that
 *	changed the focus.  It may be needed by the caller to filter
 *	out focus change events that were queued before the command.
 *	If the procedure doesn't actually change the focus then
 *	it returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpChangeFocus(winPtr, force)
    TkWindow *winPtr;		/* Window that is to receive the X focus. */
    int force;			/* Non-zero means claim the focus even
				 * if it didn't originally belong to
				 * topLevelPtr's application. */
{
    /*
     * We don't really need to do anything on the Mac.  Tk will
     * keep all this state for us.
     */

    if (winPtr->atts.override_redirect) {
	return 0;
    }

    /*
     * Remember the current serial number for the X server and issue
     * a dummy server request.  This marks the position at which we
     * changed the focus, so we can distinguish FocusIn and FocusOut
     * events on either side of the mark.
     */

    return NextRequest(winPtr->display);
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateFocusEvent --
 *
 *	Generate FocusIn/FocusOut events from a Macintosh Activate 
 *	event.  Note, the activate-on-foreground bit must be set in 
 *	the SIZE flags to ensure we get Activate/Deactivate in addition 
 *	to Susspend/Resume events.
 *
 * Results:
 *	Returns true if events were generate.
 *
 * Side effects:
 *	Queue events on Tk's event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateFocusEvent(
    EventRecord *eventPtr,	/* Incoming Mac event */
    Window window)		/* Root X window for event. */
{
    XEvent event;
    Tk_Window tkwin;
    TkDisplay *dispPtr;
    
    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    if (tkwin == NULL) {
	return false;
    }

    /* 
     * Generate FocusIn and FocusOut events.  This event
     * is only sent to the toplevel window.
     */

    if (eventPtr->modifiers & activeFlag) {
	event.xany.type = FocusIn;
    } else {
	event.xany.type = FocusOut;
    }

    event.xany.serial = dispPtr->display->request;
    event.xany.send_event = False;
    event.xfocus.display = dispPtr->display;
    event.xfocus.window = window;
    event.xfocus.mode = NotifyNormal;
    event.xfocus.detail = NotifyDetailNone;

    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateKeyEvent --
 *
 *	Given Macintosh keyUp, keyDown & autoKey events this function
 *	generates the appropiate X key events.  The window that is passed
 *	should represent the frontmost window - which will recieve the
 *	event.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateKeyEvent(
    EventRecord *eventPtr,	/* Incoming Mac event */
    Window window,		/* Root X window for event. */
    UInt32 savedKeyCode)	/* If non-zero, this is a lead byte which
    				 * should be combined with the character
    				 * in this event to form one multi-byte 
    				 * character. */
{
    Point where;
    Tk_Window tkwin;
    XEvent event;
    unsigned char byte;
    char buf[16];
    TkDisplay *dispPtr;
    
    /*
     * The focus must be in the FrontWindow on the Macintosh.
     * We then query Tk to determine the exact Tk window
     * that owns the focus.
     */

    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    tkwin = (Tk_Window) ((TkWindow *) tkwin)->dispPtr->focusPtr;
    if (tkwin == NULL) {
	return false;
    }
    byte = (unsigned char) (eventPtr->message & charCodeMask);
    if ((savedKeyCode == 0) && 
            (Tcl_ExternalToUtf(NULL, NULL, (char *) &byte, 1, 0, NULL, 
            	    buf, sizeof(buf), NULL, NULL, NULL) != TCL_OK)) {
        /*
         * This event specifies a lead byte.  Wait for the second byte
         * to come in before sending the XEvent.
         */
         
        return false;
    }   

    where.v = eventPtr->where.v;
    where.h = eventPtr->where.h;

    event.xany.send_event = False;
    event.xkey.same_screen = true;
    event.xkey.subwindow = None;
    event.xkey.time = TkpGetMS();

    event.xkey.x_root = where.h;
    event.xkey.y_root = where.v;
    GlobalToLocal(&where);
    Tk_TopCoordsToWindow(tkwin, where.h, where.v, 
	    &event.xkey.x, &event.xkey.y);
    
    event.xkey.keycode = byte |
            ((savedKeyCode & charCodeMask) << 8) |
            ((eventPtr->message & keyCodeMask) << 8);

    event.xany.serial = Tk_Display(tkwin)->request;
    event.xkey.window = Tk_WindowId(tkwin);
    event.xkey.display = Tk_Display(tkwin);
    event.xkey.root = XRootWindow(Tk_Display(tkwin), 0);
    event.xkey.state = TkMacButtonKeyState();

    if (eventPtr->what == keyDown) {
	event.xany.type = KeyPress;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    } else if (eventPtr->what == keyUp) {
	event.xany.type = KeyRelease;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    } else {
	/*
	 * Autokey events send multiple XKey events.
	 *
	 * Note: the last KeyRelease will always be missed with
	 * this scheme.  However, most Tk scripts don't look for
	 * KeyUp events so we should be OK.
	 */
	event.xany.type = KeyRelease;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	event.xany.type = KeyPress;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    }
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GeneratePollingEvents --
 *
 *	This function polls the mouse position and generates X Motion,
 *	Enter & Leave events.  The cursor is also updated at this
 *	time.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.  
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *	The cursor may be changed.
 *
 *----------------------------------------------------------------------
 */

static int
GeneratePollingEvents()
{
    Tk_Window tkwin, rootwin;
    Window window;
    WindowRef whichwindow, frontWin;
    Point whereLocal, whereGlobal;
    Boolean inContentRgn;
    short part;
    int local_x, local_y;
    int generatedEvents = false;
    TkDisplay *dispPtr;

    /*
     * First we get the current mouse position and determine
     * what Tk window the mouse is over (if any).
     */
    frontWin = FrontWindow();
    if (frontWin == NULL) {
	return false;
    }
    SetPort((GrafPort *) frontWin);
   
    GetMouse(&whereLocal);
    whereGlobal = whereLocal;
    LocalToGlobal(&whereGlobal);
	
    part = FindWindow(whereGlobal, &whichwindow);
    inContentRgn = (part == inContent || part == inGrow);

    if ((frontWin != whichwindow) || !inContentRgn) {
	tkwin = NULL;
    } else {
	window = TkMacGetXWindow(whichwindow);
	dispPtr = TkGetDisplayList();
	rootwin = Tk_IdToWindow(dispPtr->display, window);
	if (rootwin == NULL) {
	    tkwin = NULL;
	} else {
	    tkwin = Tk_TopCoordsToWindow(rootwin, whereLocal.h, whereLocal.v, 
		    &local_x, &local_y);
	}
    }
    
    /*
     * The following call will generate the appropiate X events and
     * adjust any state that Tk must remember.
     */

    if ((tkwin == NULL) && (gGrabWinPtr != NULL)) {
	tkwin = gGrabWinPtr;
    }
    Tk_UpdatePointer(tkwin, whereGlobal.h,  whereGlobal.v,
	    TkMacButtonKeyState());
    
    /*
     * Finally, we make sure the proper cursor is installed.  The installation
     * is polled to 1) make our resize hack work, and 2) make sure we have the 
     * proper cursor even if someone else changed the cursor out from under
     * us.
     */
    if ((gGrabWinPtr == NULL) && (part == inGrow) && 
	    TkMacResizable((TkWindow *) tkwin) && 
	    (TkMacGetScrollbarGrowWindow((TkWindow *) tkwin) == NULL)) {
	TkMacInstallCursor(1);
    } else {
	TkMacInstallCursor(0);
    }

    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GeneratePollingEvents2 --
 *
 *	This function polls the mouse position and generates X Motion,
 *	Enter & Leave events.  The cursor is also updated at this
 *	time.  NOTE: this version is for Netscape!!!
 *
 * Results:
 *	True if event(s) are generated - false otherwise.  
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *	The cursor may be changed.
 *
 *----------------------------------------------------------------------
 */

static int
GeneratePollingEvents2(
    Window window,
    int adjustCursor)
{
    Tk_Window tkwin, rootwin;
    WindowRef whichwindow, frontWin;
    Point whereLocal, whereGlobal;
    int local_x, local_y;
    int generatedEvents = false;
    Rect bounds;
    TkDisplay *dispPtr;
    
    /*
     * First we get the current mouse position and determine
     * what Tk window the mouse is over (if any).
     */
    frontWin = FrontWindow();
    if (frontWin == NULL) {
	return false;
    }
    SetPort((GrafPort *) frontWin);
   
    GetMouse(&whereLocal);
    whereGlobal = whereLocal;
    LocalToGlobal(&whereGlobal);

    /*
     * Determine if we are in a Tk window or not.
     */
    whichwindow = (WindowRef) TkMacGetDrawablePort(window);
    if (whichwindow != frontWin) {
	tkwin = NULL;
    } else {
        dispPtr = TkGetDisplayList();
	rootwin = Tk_IdToWindow(dispPtr->display, window);
	TkMacWinBounds((TkWindow *) rootwin, &bounds);
	if (!PtInRect(whereLocal, &bounds)) {
	    tkwin = NULL;
	} else {
	    tkwin = Tk_TopCoordsToWindow(rootwin, whereLocal.h, whereLocal.v, 
		    &local_x, &local_y);
	}
    }

    
    /*
     * The following call will generate the appropiate X events and
     * adjust any state that Tk must remember.
     */

    if ((tkwin == NULL) && (gGrabWinPtr != NULL)) {
	tkwin = gGrabWinPtr;
    }
    Tk_UpdatePointer(tkwin, whereGlobal.h,  whereGlobal.v,
	    TkMacButtonKeyState());
    
    /*
     * Finally, we make sure the proper cursor is installed.  The installation
     * is polled to 1) make our resize hack work, and 2) make sure we have the 
     * proper cursor even if someone else changed the cursor out from under
     * us.
     */
     
    if (adjustCursor) {
        TkMacInstallCursor(0);
    }
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacButtonKeyState --
 *
 *	Returns the current state of the button & modifier keys.
 *
 * Results:
 *	A bitwise inclusive OR of a subset of the following:
 *	Button1Mask, ShiftMask, LockMask, ControlMask, Mod?Mask,
 *	Mod?Mask.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
TkMacButtonKeyState()
{
    unsigned int state = 0;
    KeyMap theKeys;

    if (Button() & !gEatButtonUp) {
	state |= Button1Mask;
    }

    GetKeys(theKeys);

    if (theKeys[1] & 2) {
	state |= LockMask;
    }

    if (theKeys[1] & 1) {
	state |= ShiftMask;
    }

    if (theKeys[1] & 8) {
	state |= ControlMask;
    }

    if (theKeys[1] & 32768) {
	state |= Mod1Mask;		/* command key */
    }

    if (theKeys[1] & 4) {
	state |= Mod2Mask;		/* option key */
    }

    return state;
}

/*
 *----------------------------------------------------------------------
 *
 * XGrabKeyboard --
 *
 *	Simulates a keyboard grab by setting the focus.
 *
 * Results:
 *	Always returns GrabSuccess.
 *
 * Side effects:
 *	Sets the keyboard focus to the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XGrabKeyboard(
    Display* display,
    Window grab_window,
    Bool owner_events,
    int pointer_mode,
    int keyboard_mode,
    Time time)
{
    gKeyboardWinPtr = Tk_IdToWindow(display, grab_window);
    return GrabSuccess;
}

/*
 *----------------------------------------------------------------------
 *
 * XUngrabKeyboard --
 *
 *	Releases the simulated keyboard grab.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the keyboard focus back to the value before the grab.
 *
 *----------------------------------------------------------------------
 */

void
XUngrabKeyboard(
    Display* display,
    Time time)
{
    gKeyboardWinPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * XQueryPointer --
 *
 *	Check the current state of the mouse.  This is not a complete
 *	implementation of this function.  It only computes the root
 *	coordinates and the current mask.
 *
 * Results:
 *	Sets root_x_return, root_y_return, and mask_return.  Returns
 *	true on success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Bool
XQueryPointer(
    Display* display,
    Window w,
    Window* root_return,
    Window* child_return,
    int* root_x_return,
    int* root_y_return,
    int* win_x_return,
    int* win_y_return,
    unsigned int* mask_return)
{
    Point where;

    GetMouse(&where);
    LocalToGlobal(&where);
    *root_x_return = where.h;
    *root_y_return = where.v;
    *mask_return = TkMacButtonKeyState();    
    return True;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacGenerateTime --
 *
 *	Returns the total number of ticks from startup  This function
 *	is used to generate the time of generated X events.
 *
 * Results:
 *	Returns the current time (ticks from startup).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Time
TkMacGenerateTime()
{
    return (Time) LMGetTicks();
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacConvertEvent --
 *
 *	This function converts a Macintosh event into zero or more
 *	Tcl events.
 *
 * Results:
 *	Returns 1 if event added to Tcl queue, 0 otherwse.
 *
 * Side effects:
 *	May add events to Tcl's event queue.
 *
 *----------------------------------------------------------------------
 */

int
TkMacConvertEvent(
    EventRecord *eventPtr)
{
    WindowRef whichWindow;
    Window window;
    int eventFound = false;
    static UInt32 savedKeyCode;
    
    switch (eventPtr->what) {
	case nullEvent:
	case adjustCursorEvent:
	    if (GeneratePollingEvents()) {
		eventFound = true;
	    }
	    break;
	case updateEvt:
	    whichWindow = (WindowRef)eventPtr->message;	
	    window = TkMacGetXWindow(whichWindow);
	    if (GenerateUpdateEvent(eventPtr, window)) {
		eventFound = true;
	    }
	    break;
	case mouseDown:
	case mouseUp:
	    FindWindow(eventPtr->where, &whichWindow);
	    window = TkMacGetXWindow(whichWindow);
	    if (WindowManagerMouse(eventPtr, window)) {
		eventFound = true;
	    }
	    break;
	case autoKey:
	case keyDown:
	    /*
	     * Handle menu-key events here.  If it is *not*
	     * a menu key - just fall through to handle as a
	     * normal key event.
	     */
	    if ((eventPtr->modifiers & cmdKey) == cmdKey) {
		long menuResult;
		int oldMode;

		oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
		menuResult = MenuKey(eventPtr->message & charCodeMask);
		Tcl_SetServiceMode(oldMode);

		if (HiWord(menuResult) != 0) {
		    TkMacHandleMenuSelect(menuResult, false);
		    break;
		}
	    }
	    /* fall through */
	    
	case keyUp:
	    whichWindow = FrontWindow();
	    if (whichWindow == NULL) {
	        /*
	         * This happens if we get a key event before Tk has had a
	         * chance to actually create and realize ".", if they type
	         * when "." is withdrawn(!), or between the time "." is 
	         * destroyed and the app exits.
	         */
	         
	        return false;
	    }
	    window = TkMacGetXWindow(whichWindow);
	    if (GenerateKeyEvent(eventPtr, window, savedKeyCode) == 0) {
	        savedKeyCode = eventPtr->message;
	        return false;
	    }
	    eventFound = true;
	    break;
	    	    
	case activateEvt:
	    window = TkMacGetXWindow((WindowRef) eventPtr->message);
	    eventFound |= GenerateActivateEvents(eventPtr, window);
	    eventFound |= GenerateFocusEvent(eventPtr, window);
	    break;
	case getFocusEvent:
	    eventPtr->modifiers |= activeFlag;
	    window = TkMacGetXWindow((WindowRef) eventPtr->message);
	    eventFound |= GenerateFocusEvent(eventPtr, window);
	    break;
	case loseFocusEvent:
	    eventPtr->modifiers &= ~activeFlag;
	    window = TkMacGetXWindow((WindowRef) eventPtr->message);
	    eventFound |= GenerateFocusEvent(eventPtr, window);
	    break;
	case kHighLevelEvent:
	    TkMacDoHLEvent(eventPtr);
	    /* TODO: should return true if events were placed on event queue. */
	    break;
	case osEvt:
	    /*
	     * Do clipboard conversion.
	     */
	    switch ((eventPtr->message & osEvtMessageMask) >> 24) {
		case mouseMovedMessage:
		    if (GeneratePollingEvents()) {
			eventFound = true;
		    }
		    break;
		case suspendResumeMessage:
		    if (!(eventPtr->message & resumeFlag)) {
			TkSuspendClipboard();
		    }
		    tkMacAppInFront = (eventPtr->message & resumeFlag);
		    break;
	    }
	    break;
	case diskEvt:
	    /* 
	     * Disk insertion. 
	     */
	    if (HiWord(eventPtr->message) != noErr) {
		Point pt;
			
		DILoad();
		pt.v = pt.h = 120;	  /* parameter ignored in sys 7 */
		DIBadMount(pt, eventPtr->message);
		DIUnload();
	    }
	    break;
    }
    
    savedKeyCode = 0;
    return eventFound;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacConvertTkEvent --
 *
 *	This function converts a Macintosh event into zero or more
 *	Tcl events.  It is intended for use in Netscape-style embedding.
 *
 * Results:
 *	Returns 1 if event added to Tcl queue, 0 otherwse.
 *
 * Side effects:
 *	May add events to Tcl's event queue.
 *
 *----------------------------------------------------------------------
 */

int
TkMacConvertTkEvent(
    EventRecord *eventPtr,
    Window window)
{
    int eventFound = false;
    Point where;
    static UInt32 savedKeyCode;
    
    /*
     * By default, assume it is legal for us to set the cursor 
     */
     
    Tk_MacTkOwnsCursor(1);
    
    switch (eventPtr->what) {
	case nullEvent:
        /*
         * We get NULL events only when the cursor is NOT over
	 * the plugin.  Otherwise we get updateCursor events.
	 * We will not generate polling events or move the cursor
	 * in this case.
         */
            
	    eventFound = false;
	    break;
	case adjustCursorEvent:
	    if (GeneratePollingEvents2(window, 1)) {
		eventFound = true;
	    }
	    break;
	case updateEvt:
        /*
         * It is possibly not legal for us to set the cursor 
         */
     
            Tk_MacTkOwnsCursor(0);
	    if (GenerateUpdateEvent(eventPtr, window)) {
		eventFound = true;
	    }
	    break;
	case mouseDown:
	case mouseUp:
	    GetMouse(&where);
	    LocalToGlobal(&where);
	    eventFound |= TkGenerateButtonEvent(where.h, where.v, 
		window, TkMacButtonKeyState());
	    break;
	case autoKey:
	case keyDown:
	    /*
	     * Handle menu-key events here.  If it is *not*
	     * a menu key - just fall through to handle as a
	     * normal key event.
	     */
	    if ((eventPtr->modifiers & cmdKey) == cmdKey) {
		long menuResult = MenuKey(eventPtr->message & charCodeMask);
		
		if (HiWord(menuResult) != 0) {
		    TkMacHandleMenuSelect(menuResult, false);
		    break;
		}
	    }
	    /* fall through. */
	    
	case keyUp:
	    if (GenerateKeyEvent(eventPtr, window, savedKeyCode) == 0) {
	        savedKeyCode = eventPtr->message;
	        return false;
	    }	        
	    eventFound = true;
	    break;
	    
	case activateEvt:
        /*
         * It is probably not legal for us to set the cursor
	 * here, since we don't know where the mouse is in the
	 * window that is being activated.
         */
     
            Tk_MacTkOwnsCursor(0);
	    eventFound |= GenerateActivateEvents(eventPtr, window);
	    eventFound |= GenerateFocusEvent(eventPtr, window);
	    break;
	case getFocusEvent:
	    eventPtr->modifiers |= activeFlag;
	    eventFound |= GenerateFocusEvent(eventPtr, window);
	    break;
	case loseFocusEvent:
	    eventPtr->modifiers &= ~activeFlag;
	    eventFound |= GenerateFocusEvent(eventPtr, window);
	    break;
	case kHighLevelEvent:
	    TkMacDoHLEvent(eventPtr);
	    /* TODO: should return true if events were placed on event queue. */
	    break;
	case osEvt:
	    /*
	     * Do clipboard conversion.
	     */
	    switch ((eventPtr->message & osEvtMessageMask) >> 24) {
        /*
         * It is possibly not legal for us to set the cursor.
         * Netscape sends us these events all the time... 
         */
     
                Tk_MacTkOwnsCursor(0);
        
		case mouseMovedMessage:
		    /* if (GeneratePollingEvents2(window, 0)) {
			eventFound = true;
		    }  NEXT LINE IS TEMPORARY */
		    eventFound = false;
		    break;
		case suspendResumeMessage:
		    if (!(eventPtr->message & resumeFlag)) {
			TkSuspendClipboard();
		    }
		    tkMacAppInFront = (eventPtr->message & resumeFlag);
		    break;
	    }
	    break;
	case diskEvt:
	    /* 
	     * Disk insertion. 
	     */
	    if (HiWord(eventPtr->message) != noErr) {
		Point pt;
			
		DILoad();
		pt.v = pt.h = 120;	  /* parameter ignored in sys 7 */
		DIBadMount(pt, eventPtr->message);
		DIUnload();
	    }
	    break;
    }
    savedKeyCode = 0;    
    return eventFound;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckEventsAvail --
 *
 *	Checks to see if events are available on the Macintosh queue.
 *	This function looks for both queued events (eg. key & button)
 *	and generated events (update).
 *
 * Results:
 *	True is events exist, false otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CheckEventsAvail()
{
    QHdrPtr evPtr;
    WindowPeek macWinPtr;
    
    evPtr = GetEvQHdr();
    if (evPtr->qHead != NULL) {
	return true;
    }
    
    macWinPtr = (WindowPeek) FrontWindow();
    while (macWinPtr != NULL) {
	if (!EmptyRgn(macWinPtr->updateRgn)) {
	    return true;
	}
	macWinPtr = macWinPtr->nextWindow;
    }
    return false;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetCapture --
 *
 *	This function captures the mouse so that all future events
 *	will be reported to this window, even if the mouse is outside
 *	the window.  If the specified window is NULL, then the mouse
 *	is released. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the capture flag and captures the mouse.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetCapture(
    TkWindow *winPtr)			/* Capture window, or NULL. */
{
    while ((winPtr != NULL) && !Tk_IsTopLevel(winPtr)) {
	winPtr = winPtr->parentPtr;
    }
    gGrabWinPtr = (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacWindowOffset --
 *
 *	Determines the x and y offset from the orgin of the toplevel
 *	window dressing (the structure region, ie. title bar) and the
 *	orgin of the content area.
 *
 * Results:
 *	The x & y offset in pixels.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkMacWindowOffset(
    WindowRef wRef,
    int *xOffset,
    int *yOffset)
{
    OSErr err = noErr;
    WindowPeek wPeek = (WindowPeek) wRef;
    RgnHandle strucRgn = wPeek->strucRgn;
    RgnHandle contRgn = wPeek->contRgn;
    Rect strucRect, contRect;

    if (!EmptyRgn(strucRgn) && !EmptyRgn(contRgn)) {
	strucRect = (**strucRgn).rgnBBox;
	contRect = (**contRgn).rgnBBox;
    } else {		
	/*
	 * The current window's regions are not up to date.
	 * Probably because the window isn't visable.  What we
	 * will do is save the old regions, have the window calculate
	 * what the regions should be, and then restore it self.
	 */
	strucRgn = NewRgn( );
	contRgn = NewRgn( );

	if (!strucRgn || !contRgn) {
	    err = MemError( );
	} else {
	    CopyRgn(wPeek->strucRgn, strucRgn);
	    CopyRgn(wPeek->contRgn, contRgn);

	    if (!(err = TellWindowDefProcToCalcRegions(wRef))) {
		strucRect = (**(wPeek->strucRgn)).rgnBBox;
		contRect = (**(wPeek->contRgn)).rgnBBox;
	    }

	    CopyRgn(strucRgn, wPeek->strucRgn);
	    CopyRgn(contRgn, wPeek->contRgn);
	}

	if (contRgn) {
	    DisposeRgn(contRgn);
	}
		
	if (strucRgn) {
	    DisposeRgn(strucRgn);
	}
    }

    if (!err) {
	*xOffset = contRect.left - strucRect.left;
	*yOffset = contRect.top - strucRect.top;
    } else {
	*xOffset = 0;
	*yOffset = 0;
    }

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TellWindowDefProcToCalcRegions --
 *
 *	Force a Macintosh window to recalculate it's content and
 *	structure regions.
 *
 * Results:
 *	An OS error.
 *
 * Side effects:
 *	The windows content and structure regions may be updated.
 *
 *----------------------------------------------------------------------
 */

static OSErr 
TellWindowDefProcToCalcRegions(
    WindowRef wRef)
{
    OSErr err = noErr;
    SInt8 hState;
    Handle wdef = ((WindowPeek) wRef)->windowDefProc;

    /*
     * Load and lock the window definition procedure for
     * the window.
     */
    hState = HGetState(wdef);
    if (!(err = MemError())) {
	LoadResource(wdef);
	if (!(err = ResError())) {
	    MoveHHi(wdef);
	    err = MemError();
	    if (err == memLockedErr) {
	        err = noErr;
	    } else if (!err) {
		HLock(wdef);
		err = MemError();
	    }
	}
    }
    
    /*
     * Assuming there are no errors we now call the window definition 
     * procedure to tell it to calculate the regions for the window.
     */
    if (err == noErr) {
 	(void) CallWindowDefProc((UniversalProcPtr) *wdef,
		GetWVariant(wRef), wRef, wCalcRgns, 0);

	HSetState(wdef, hState);
	if (!err) {
	     err = MemError();
	}
    }

    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * BringWindowForward --
 *
 *	Bring this background window to the front.  We also set state
 *	so Tk thinks the button is currently up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is brought forward.
 *
 *----------------------------------------------------------------------
 */

static void 
BringWindowForward(
    WindowRef wRef)
{
    SelectWindow(wRef);
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
    long long * int64Ptr;
    UnsignedWide micros;
    
    Microseconds(&micros);
    int64Ptr = (long long *) &micros;

    /*
     * We need 64 bit math to do this.  This is available in CW 11
     * and on.  Other's will need to use a different scheme.
     */

    *int64Ptr /= 1000;

    return (long) *int64Ptr;
}
