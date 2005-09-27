/*
 * tkMacOSXWindowEvent.c --
 *
 *	This file defines the routines for both creating and handling
 *      Window Manager class events for Tk.
 *
 *      Copyright 2001, Apple Computer, Inc.
 *
 *      The following terms apply to all files originating from Apple
 *      Computer, Inc. ("Apple") and associated with the software
 *      unless explicitly disclaimed in individual files.
 *
 *
 *      Apple hereby grants permission to use, copy, modify,
 *      distribute, and license this software and its documentation
 *      for any purpose, provided that existing copyright notices are
 *      retained in all copies and that this notice is included
 *      verbatim in any distributions. No written agreement, license,
 *      or royalty fee is required for any of the authorized
 *      uses. Modifications to this software may be copyrighted by
 *      their authors and need not follow the licensing terms
 *      described here, provided that the new terms are clearly
 *      indicated on the first page of each file where they apply.
 *
 *
 *      IN NO EVENT SHALL APPLE, THE AUTHORS OR DISTRIBUTORS OF THE
 *      SOFTWARE BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
 *      INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 *      THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
 *      EVEN IF APPLE OR THE AUTHORS HAVE BEEN ADVISED OF THE
 *      POSSIBILITY OF SUCH DAMAGE.  APPLE, THE AUTHORS AND
 *      DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
 *      BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS
 *      SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND APPLE,THE
 *      AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *      MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *      GOVERNMENT USE: If you are acquiring this software on behalf
 *      of the U.S. government, the Government shall have only
 *      "Restricted Rights" in the software and related documentation
 *      as defined in the Federal Acquisition Regulations (FARs) in
 *      Clause 52.227.19 (c) (2).  If you are acquiring the software
 *      on behalf of the Department of Defense, the software shall be
 *      classified as "Commercial Computer Software" and the
 *      Government shall have only "Restricted Rights" as defined in
 *      Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 *      foregoing, the authors grant the U.S. Government and others
 *      acting in its behalf permission to use and distribute the
 *      software in accordance with the terms specified in this
 *      license.
 */

#include "tkMacOSXInt.h"
#include "tkPort.h"
#include "tkMacOSXWm.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXDebug.h"

/*
 * Declarations of global variables defined in this file.
 */

int tkMacOSXAppInFront = true;     /* Boolean variable for determining if 
                                    * we are the frontmost app.  Only set 
                                    * in TkMacOSXProcessApplicationEvent
                                    */
static RgnHandle gDamageRgn;
static RgnHandle visRgn;

/*
 * Declaration of functions used only in this file
 */
 
static int GenerateUpdateEvent( Window window);
static void GenerateUpdates( RgnHandle updateRgn, TkWindow *winPtr);
static int GenerateActivateEvents( Window window, int activeFlag);
static int GenerateFocusEvent( Window window, int activeFlag);



/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessApplicationEvent --
 *
 *        This processes Application level events, mainly activate
 *        and deactivate.
 *
 * Results:
 *        o.
 *
 * Side effects:
 *        Hide or reveal floating windows, and set tkMacOSXAppInFront.
 *
 *----------------------------------------------------------------------
 */
 
int
TkMacOSXProcessApplicationEvent(
        TkMacOSXEvent *eventPtr, 
        MacEventStatus *statusPtr)
{
    switch (eventPtr->eKind) {
        case kEventAppActivated:
            tkMacOSXAppInFront = true;
            ShowFloatingWindows();
            break;
        case kEventAppDeactivated:
            TkSuspendClipboard();
            tkMacOSXAppInFront = false;
            HideFloatingWindows();
            break;
        case kEventAppQuit:
        case kEventAppLaunchNotification:
        case kEventAppLaunched:
        case kEventAppTerminated:
        case kEventAppFrontSwitched:
            break;
    }
    return 0;
}
/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessWindowEvent --
 *
 *        This processes Window level events, mainly activate
 *        and deactivate.
 *
 * Results:
 *        0.
 *
 * Side effects:
 *        Cause Windows to be moved forward or backward in the 
 *        window stack.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXProcessWindowEvent(
        TkMacOSXEvent * eventPtr, 
        MacEventStatus * statusPtr)
{ 
    OSStatus  status;
    WindowRef whichWindow;
    Window    window;
    int       eventFound;
    
    switch (eventPtr->eKind) { 
        case kEventWindowActivated:
        case kEventWindowDeactivated:
        case kEventWindowUpdate:
            break;
        default:
            return 0;
            break;
    }
    status = GetEventParameter(eventPtr->eventRef,
            kEventParamDirectObject,
            typeWindowRef, NULL,
            sizeof(whichWindow), NULL,
            &whichWindow);
    if (status != noErr) {
        fprintf ( stderr, "TkMacOSXHandleWindowEvent:Failed to retrieve window" );
        return 0;
    }
    
    window = TkMacOSXGetXWindow(whichWindow);

    switch (eventPtr->eKind) {
        case kEventWindowActivated:
            eventFound |= GenerateActivateEvents(window, 1);
            eventFound |= GenerateFocusEvent(window, 1);
            break;
        case kEventWindowDeactivated:
            eventFound |= GenerateActivateEvents(window, 0);
            eventFound |= GenerateFocusEvent(window, 0);
            break;
        case kEventWindowUpdate:
            if (GenerateUpdateEvent(window)) {
                eventFound = true;
            }
            break;
    }
    return 0;
}

/*         
 *----------------------------------------------------------------------
 *                      
 * GenerateUpdateEvent --
 *                      
 *      Given a Macintosh window update event this function generates all the
 *      X update events needed by Tk.
 *
 * Results:     
 *      True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *      Additional events may be place on the Tk event queue.
 *          
 *----------------------------------------------------------------------
 */     
static int
GenerateUpdateEvent(Window window)
{
    WindowRef   macWindow;
    TkDisplay * dispPtr;
    TkWindow  * winPtr;
    Rect        bounds;
    
    dispPtr = TkGetDisplayList();
    winPtr = (TkWindow *)Tk_IdToWindow(dispPtr->display, window);
 
    if (winPtr ==NULL ){
        return false;
    }
    if (gDamageRgn == NULL) {
        gDamageRgn = NewRgn();
    }
    if (visRgn == NULL) {
        visRgn = NewRgn();
    }
    macWindow = GetWindowFromPort(TkMacOSXGetDrawablePort(window));
    BeginUpdate(macWindow);
    /*
     * In the Classic version of the code, this was the "visRgn" field of the WindowRec
     * This no longer exists in OS X, so retrieve the content region instead
     * Note that this is in screen coordinates
     * We therefore convert it to window relative coordinates
     */
    GetWindowRegion (macWindow, kWindowContentRgn, visRgn );
    GetRegionBounds(visRgn,&bounds);
    bounds.right -= bounds.left;
    bounds.bottom -= bounds.top;
    bounds.left=
    bounds.top=0;
    RectRgn(visRgn, &bounds);
    GenerateUpdates(visRgn, winPtr);
    EndUpdate(macWindow);
    return true;
 }

/*
 *----------------------------------------------------------------------
 *
 * GenerateUpdates --
 *
 *        Given a Macintosh update region and a Tk window this function
 *        geneates a X damage event for the window if it is within the
 *        update region.  The function will then recursivly have each
 *        damaged window generate damage events for its child windows.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
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
    Rect bounds, updateBounds, damageBounds;

    TkMacOSXWinBounds(winPtr, &bounds);
    GetRegionBounds(updateRgn,&updateBounds);
        
    if (bounds.top > updateBounds.bottom ||
        updateBounds.top > bounds.bottom ||
        bounds.left > updateBounds.right ||
        updateBounds.left > bounds.right ||
        !RectInRgn(&bounds, updateRgn)) {
        return;
    }
    if (!RectInRgn(&bounds, updateRgn)) {
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
     * CopyRgn(TkMacOSXVisableClipRgn(winPtr), rgn);
     * TODO: this call doesn't work doing resizes!!!
     */
    RectRgn(gDamageRgn, &bounds);
    SectRgn(gDamageRgn, updateRgn, gDamageRgn);
    OffsetRgn(gDamageRgn, -bounds.left, -bounds.top);
    GetRegionBounds(gDamageRgn,&damageBounds);
    event.xexpose.x = damageBounds.left;
    event.xexpose.y = damageBounds.top;
    event.xexpose.width = damageBounds.right-damageBounds.left;
    event.xexpose.height = damageBounds.bottom-damageBounds.top;
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
 * GenerateActivateEvents --
 *                      
 *      Given a Macintosh window activate event this function generates all the
 *      X Activate events needed by Tk.
 *
 * Results:     
 *      True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *      Additional events may be place on the Tk event queue.
 *          
 *----------------------------------------------------------------------
 */
    
int
GenerateActivateEvents(
    Window window,                /* Root X window for event. */
    int    activeFlag )
{
    TkWindow *winPtr;
    TkDisplay *dispPtr;
    
    dispPtr = TkGetDisplayList();
    winPtr = (TkWindow *) Tk_IdToWindow(dispPtr->display, window);
    if (winPtr == NULL || winPtr->window == None) {
        return false;
    }

    TkGenerateActivateEvents(winPtr,activeFlag);
    return true;
}

/*         
 *----------------------------------------------------------------------
 *                      
 * GenerateFocusEvent --
 *                      
 *      Given a Macintosh window activate event this function generates all the
 *      X Focus events needed by Tk.
 *
 * Results:     
 *      True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *      Additional events may be place on the Tk event queue.
 *          
 *----------------------------------------------------------------------
 */     

int
GenerateFocusEvent(
    Window window,              /* Root X window for event. */
    int    activeFlag )
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

    if (activeFlag) {
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
    TkMacOSXInvalClipRgns(winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenWMDestroyEvent --
 *
 *	Generate a WM Destroy event for Tk.   *
 * Results:
 *	None.
 *
 * Side effects:
 *	A WM_PROTOCOL/WM_DELETE_WINDOW event is sent to Tk.
 *
 *----------------------------------------------------------------------
 */

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
    ProtocolHandler *protPtr;
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
 * Tk_MacOSXIsAppInFront --
 *
 *	Returns 1 if this app is the foreground app. 
 *
 * Results:
 *	1 if app is in front, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_MacOSXIsAppInFront (void)
{
    return tkMacOSXAppInFront;
}
