/*
 * tkMacOSXMouseEvent.c --
 *
 *	This file implements functions that decode & handle mouse events
 *      on MacOS X.
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

#include "tkInt.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include "tkMacOSXEvent.h"
#include "tkMacOSXInt.h"
#include "tkPort.h"
#include "tkMacOSXDebug.h"

typedef struct {
    WindowRef      whichWin;
    WindowRef      activeNonFloating;
    WindowPartCode windowPart;
    Point          global;
    Point          local;   
    unsigned int   state;
    long           delta;
} MouseEventData;

/*
 * Declarations of static variables used in this file.
 */

static int gEatButtonUp = 0;       /* 1 if we need to eat the next * up event */

/*
 * Declarations of functions used only in this file.
 */

static void BringWindowForward _ANSI_ARGS_((WindowRef wRef));
static int GeneratePollingEvents(MouseEventData * medPtr);
static int GenerateMouseWheelEvent(MouseEventData * medPtr);
static int HandleInGoAway(Tk_Window tkwin, WindowRef winPtr, Point where);
static OSErr HandleInCollapse(WindowRef win);

extern int TkMacOSXGetEatButtonUp();
extern void TkMacOSXSetEatButtonUp(int f);

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessMouseEvent --
 *
 *        This routine processes the event in eventPtr, and
 *        generates the appropriate Tk events from it.
 *
 * Results:
 *        True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */
 
int
TkMacOSXProcessMouseEvent(TkMacOSXEvent *eventPtr, MacEventStatus * statusPtr)
{
    WindowRef      frontWindow;
    Tk_Window      tkwin;
    Point          where, where2;
    int            xOffset, yOffset;
    TkDisplay *    dispPtr;
    Window         window;
    int            status,err;
    MouseEventData mouseEventData, * medPtr = &mouseEventData;
    KeyMap         keyMap;

    switch (eventPtr->eKind) {
        case kEventMouseUp:
        case kEventMouseDown:
        case kEventMouseMoved:
        case kEventMouseDragged:
        case kEventMouseWheelMoved:
            break;
        default:
            return 0;
            break;
    }
    status = GetEventParameter(eventPtr->eventRef, 
            kEventParamMouseLocation,
            typeQDPoint, NULL, 
            sizeof(where), NULL,
            &where);
    if (status != noErr) {
        fprintf (stderr, "Failed to retrieve mouse location,%d\n", status);
        return 0;
    }
    medPtr->state = 0;
    GetKeys(keyMap);
    if (keyMap[1] & 2) { 
        medPtr->state |= LockMask;
    }
    if (keyMap[1] & 1) {
        medPtr->state |= ShiftMask;
    }
    if (keyMap[1] & 8) {
        medPtr->state |= ControlMask;
    }
    if (keyMap[1] & 32768) {
        medPtr->state |= Mod1Mask;              /* command key */
    }
    if (keyMap[1] & 4) {
        medPtr->state |= Mod2Mask;              /* option key */
    }
    if (eventPtr->eKind == kEventMouseDown 
            || eventPtr->eKind== kEventMouseDragged ) {
        EventMouseButton mouseButton;
        if ((status=GetEventParameter(eventPtr->eventRef,
             kEventParamMouseButton,
             typeMouseButton, NULL,
             sizeof(mouseButton), NULL,&mouseButton)) != noErr ) {
             fprintf (stderr, "Failed to retrieve mouse button, %d\n", status);
             statusPtr->err = 1;
             return 0;
         }
         medPtr->state |= 1 << ((mouseButton-1)+8);
    }

    medPtr->windowPart= FindWindow(where, &medPtr->whichWin);
    window = TkMacOSXGetXWindow(medPtr->whichWin);
    if (medPtr->whichWin != NULL && window == None) {
        return 0;
    }
    
    frontWindow = FrontWindow();
    medPtr->activeNonFloating = ActiveNonFloatingWindow();

    /*
     * The window manager only needs to know about mouse down events
     * and sometimes we need to "eat" the mouse up.  Otherwise, we
     * just pass the event to Tk.

     */
    if (eventPtr->eKind == kEventMouseUp) {
        if (TkMacOSXGetEatButtonUp()) {
            TkMacOSXSetEatButtonUp(false);
            return false;
        }
        return TkGenerateButtonEvent(where.h, where.v,
            window, medPtr->state);
    }
    if (eventPtr->eKind == kEventMouseWheelMoved) {
        if ((status=GetEventParameter(eventPtr->eventRef,
             kEventParamMouseWheelDelta,
             typeLongInteger, NULL,
             sizeof(medPtr->delta), NULL,&medPtr->delta)) != noErr ) {
             fprintf (stderr,
                 "Failed to retrieve mouse wheel delta, %d\n", status);
             statusPtr->err = 1;
             return false;
         }
    }
    
    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);

    if (eventPtr->eKind != kEventMouseDown ) {
        /* 
         * MouseMoved, MouseDragged or kEventMouseWheelMoved 
         */
         
        medPtr->global = where;
        medPtr->local = where;
	/* 
	 * We must set the port to the right window -- the one
	 * we are actually going to use -- before finding
	 * the local coordinates, otherwise we will have completely
	 * wrong local x,y!
	 * 
	 * I'm pretty sure this window is medPtr->whichWin, unless
	 * perhaps there is a grab.  Certainly 'frontWindow' or
	 * 'medPtr->activeNonFloating' are wrong.
	 */ 
	SetPortWindowPort(medPtr->whichWin);
        GlobalToLocal(&medPtr->local);
        if (eventPtr->eKind == kEventMouseWheelMoved ) {
            return GenerateMouseWheelEvent(medPtr); 
        } else {
            return GeneratePollingEvents(medPtr); 
        }
    }

    if (medPtr->whichWin && eventPtr->eKind==kEventMouseDown) {
        ProcessSerialNumber frontPsn, ourPsn;
        Boolean             flag;
        if ((err=GetFrontProcess(&frontPsn))!=noErr) {
            fprintf(stderr, "GetFrontProcess failed, %d\n", err);
            statusPtr->err = 1;
            return 1;
        }
        
        GetCurrentProcess(&ourPsn);
        if ((err=SameProcess(&frontPsn, &ourPsn, &flag))!=noErr) {
            fprintf(stderr, "SameProcess failed, %d\n", err);
            statusPtr->err = 1;
            return 1;
        } else {
            if (!flag) {
                if ((err=SetFrontProcess(&ourPsn)) != noErr) {
                    fprintf(stderr,"SetFrontProcess failed,%d\n", err);
                    statusPtr->err = 1;
                    return 1;
                }
            }
        }

    }

    if (medPtr->whichWin) {
        /*
         * We got a mouse down in a window
         * See if this is the activate click
         * This click moves the window forward.  We don't want
         * the corresponding mouse-up to be reported to the application
         * or else it will mess up some Tk scripts.
         */
         
         if (!(TkpIsWindowFloating(medPtr->whichWin))
             && (medPtr->whichWin != medPtr->activeNonFloating)) {
             Tk_Window grabWin = TkMacOSXGetCapture();
             if ((grabWin != NULL) && (grabWin != tkwin)) {
                 TkWindow * tkw, * grb;
                 tkw = (TkWindow *)tkwin;
                 grb = (TkWindow *)grabWin;
                 SysBeep(1);  
                 return false;
             }

             /*
              * Clicks in the stoplights on a MacOS X title bar are processed
              * directly even for background windows.  Do that here.
              */
             
             switch (medPtr->windowPart) {
                 case inGoAway:
                     return HandleInGoAway(tkwin, medPtr->whichWin, where);
                     break;
                 case inCollapseBox:
                     err = HandleInCollapse(medPtr->whichWin);
                     if (err = noErr) {
                         statusPtr->err = 1;
                     }
                     statusPtr->stopProcessing = 1;
                     return false;
                     break;
                 case inZoomIn:
                     return false;
                     break;
                 case inZoomOut:
                     return false;
                     break;
                 default:
                     TkMacOSXSetEatButtonUp(true);
                     BringWindowForward(medPtr->whichWin);
                     return false;
             }
         }
    }

    switch (medPtr->windowPart) {
        case inDrag:
            DragWindow(medPtr->whichWin, where, NULL);
            where2.h = where2.v = 0;
            LocalToGlobal(&where2);
            if (EqualPt(where, where2)) {
                return false;
            }
            TkMacOSXWindowOffset(medPtr->whichWin, &xOffset, &yOffset);
            where2.h -= xOffset;
            where2.v -= yOffset;
            TkGenWMConfigureEvent(tkwin, where2.h, where2.v,
                    -1, -1, TK_LOCATION_CHANGED);
            return true;
            break;
        case inContent:
            return TkGenerateButtonEvent(where.h, where.v, 
                    window, medPtr->state);
            break;
        case inGrow:
            /*      
             * Generally the content region is the domain of Tk
             * sub-windows.  However, one exception is the grow   
             * region.  A button down in this area will be handled
             * by the window manager.  Note: this means that Tk
             * may not get button down events in this area!
             */
            if (TkMacOSXGrowToplevel(medPtr->whichWin, where) == true) {
                return true;
            } else {
                return TkGenerateButtonEvent(where.h,
                           where.v, window, medPtr->state);
            }
            break;
        case inGoAway:
            return HandleInGoAway(tkwin, medPtr->whichWin, where);
            break;
        case inMenuBar:
            {
                int oldMode;
                KeyMap theKeys;

                GetKeys(theKeys);
                oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
                TkMacOSXClearMenubarActive();
                
                /*
                 * Handle -postcommand
                 */
                 
                TkMacOSXPreprocessMenu();
                TkMacOSXHandleMenuSelect(MenuSelect(where),
                        theKeys[1] & 4);
                Tcl_SetServiceMode(oldMode);
                return true; /* TODO: may not be on event on queue. */
            }
            break;
        case inZoomIn:
        case inZoomOut:
            if (TkMacOSXZoomToplevel(medPtr->whichWin, where, 
                    medPtr->windowPart) == true) {
                return true;
            } else {
                return false;
            }
            break;
        case inCollapseBox:
            err = HandleInCollapse(medPtr->whichWin);
            if (err == noErr) {
                statusPtr->err = 1;
            }
            statusPtr->stopProcessing = 1;
            break;
        default:
            return false;
            break;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * HandleInGoAway --
 *
 *        Tracks the cursor in the go away box and deletes the window
 *        if the button stays depressed on button up.
 *
 * Results:
 *        True if no errors - false otherwise.
 *
 * Side effects:
 *        The window tkwin may be destroyed.
 *
 *----------------------------------------------------------------------
 */
int
HandleInGoAway(Tk_Window tkwin, WindowRef win, Point where)
{
    if (TrackGoAway(win, where)) {
        if (tkwin == NULL) {
            return false;
        }
        TkGenWMDestroyEvent(tkwin);
        return true;
    }
    return false;    
}

/*
 *----------------------------------------------------------------------
 *
 * HandleInCollapse --
 *
 *        Tracks the cursor in the collapse box and colapses the window
 *        if the button stays depressed on button up.
 *
 * Results:
 *        Error return from CollapseWindow
 *
 * Side effects:
 *        The window win may be collapsed.
 *
 *----------------------------------------------------------------------
 */
OSErr
HandleInCollapse(WindowRef win)
{
    OSErr err;
    
    err = CollapseWindow(win,
                         !IsWindowCollapsed(win));
    if (err != noErr) {
        fprintf(stderr,"CollapseWindow failed,%d\n", err);
    }
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * GeneratePollingEvents --
 *
 *        This function polls the mouse position and generates X Motion,
 *        Enter & Leave events.  The cursor is also updated at this
 *        time.
 *
 * Results:
 *        True if event(s) are generated - false otherwise.  
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
 *        The cursor may be changed.
 *
 *----------------------------------------------------------------------
 */

static int
GeneratePollingEvents(MouseEventData * medPtr)
{
    Tk_Window tkwin, rootwin, grabWin, topPtr;
    Window window;
    int local_x, local_y;
    TkDisplay *dispPtr;


    grabWin = TkMacOSXGetCapture();

    if ((!TkpIsWindowFloating(medPtr->whichWin) 
            && (medPtr->activeNonFloating != medPtr->whichWin))) {
        /*
         * If the window for this event is not floating, and is not the
         * active non-floating window, don't generate polling events.
         * We don't send events to backgrounded windows.  So either send
         * it to the grabWin, or NULL if there is no grabWin.
         */

        tkwin = grabWin;
    } else {
        /*
         * First check whether the toplevel containing this mouse
         * event is the grab window.  If not, then send the event
         * to the grab window.  Otherwise, set tkWin to the subwindow  
         * which most closely contains the mouse event.
         */
       
        window = TkMacOSXGetXWindow(medPtr->whichWin);
        dispPtr = TkGetDisplayList();
        rootwin = Tk_IdToWindow(dispPtr->display, window);
        if ((rootwin == NULL) 
                || ((grabWin != NULL) && (rootwin != grabWin))) {
            tkwin = grabWin;
        } else {
            tkwin = Tk_TopCoordsToWindow(rootwin, 
                    medPtr->local.h, medPtr->local.v, 
                    &local_x, &local_y);
        }
    }
    
    /*
     * The following call will generate the appropiate X events and
     * adjust any state that Tk must remember.
     */

    Tk_UpdatePointer(tkwin, medPtr->global.h, medPtr->global.v,
            medPtr->state);
    
    return true;
}


/*           
 *----------------------------------------------------------------------
 *
 * BringWindowForward --
 *           
 *      Bring this background window to the front.  We also set state
 *      so Tk thinks the button is currently up.
 *
 * Results:
 *      None.
 *      
 * Side effects:
 *      The window is brought forward.
 *          
 *----------------------------------------------------------------------
 */         
  
static void     
BringWindowForward(WindowRef wRef)
{           
    if (!TkpIsWindowFloating(wRef)) {
        if (IsValidWindowPtr(wRef))
        SelectWindow(wRef);
    }
}

static int
GenerateMouseWheelEvent(MouseEventData * medPtr)
{
    Tk_Window tkwin, rootwin, grabWin;
    Window window;
    int local_x, local_y;
    TkDisplay *dispPtr;
    TkWindow  *winPtr;
    XEvent     xEvent;

    if ((!TkpIsWindowFloating(medPtr->whichWin) 
            && (medPtr->activeNonFloating != medPtr->whichWin))) {
        tkwin = NULL;
    } else {
        window = TkMacOSXGetXWindow(medPtr->whichWin);
        dispPtr = TkGetDisplayList();
        rootwin = Tk_IdToWindow(dispPtr->display, window);
        if (rootwin == NULL) {
            tkwin = NULL;
        } else {
            tkwin = Tk_TopCoordsToWindow(rootwin, 
                    medPtr->local.h, medPtr->local.v, 
                    &local_x, &local_y);
        }
    }
    
    /*
     * The following call will generate the appropiate X events and
     * adjust any state that Tk must remember.
     */

    grabWin = TkMacOSXGetCapture();

    if ((tkwin == NULL) && (grabWin != NULL)) {
        tkwin = grabWin;
    }
    if (!tkwin) {
       return true;
    }
    winPtr = ( TkWindow *)tkwin;
    xEvent.type = MouseWheelEvent;
    xEvent.xkey.keycode = medPtr->delta;
    xEvent.xany.serial = LastKnownRequestProcessed(winPtr->display);
    xEvent.xany.send_event = false;
    xEvent.xany.display = winPtr->display;
    xEvent.xany.window = Tk_WindowId(winPtr);
    Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);

    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetEatButtonUp --
 *
 * Results:
 *      Returns the flag indicating if we need to eat the
 *      next mouse up event
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */
int
TkMacOSXGetEatButtonUp()
{
    return gEatButtonUp;
}

/*
 * TkMacOSXSetEatButtonUp --
 *
 * Results:
 *        None.
 *
 * Side effects:
 *      Sets the flag indicating if we need to eat the
 *      next mouse up event
 *
 */
void
TkMacOSXSetEatButtonUp(int f)
{
    gEatButtonUp = f;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXButtonKeyState --
 *
 *        Returns the current state of the button & modifier keys.
 *
 * Results:
 *        A bitwise inclusive OR of a subset of the following:
 *        Button1Mask, ShiftMask, LockMask, ControlMask, Mod?Mask,
 *        Mod?Mask.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
TkMacOSXButtonKeyState()
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
        state |= Mod1Mask;                /* command key */
    }

    if (theKeys[1] & 4) {
        state |= Mod2Mask;                /* option key */
    }

    return state;
}

/*
 *----------------------------------------------------------------------
 *
 * XQueryPointer --
 *
 *        Check the current state of the mouse.  This is not a complete
 *        implementation of this function.  It only computes the root
 *        coordinates and the current mask.
 *
 * Results:
 *        Sets root_x_return, root_y_return, and mask_return.  Returns
 *        true on success.
 *
 * Side effects:
 *        None.
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
    CGrafPtr port;
    GDHandle dev;

    GetGWorld(&port,&dev);
    GetMouse(&where);
    LocalToGlobal(&where);

    *root_x_return = where.h;
    *root_y_return = where.v;
    *mask_return = TkMacOSXButtonKeyState();    
    return True;
}


/*
 *----------------------------------------------------------------------
 *
 * TkGenerateButtonEvent --
 *
 *        Given a global x & y position and the button key status this 
 *        procedure generates the appropiate X button event.  It also 
 *        handles the state changes needed to implement implicit grabs.
 *
 * Results:
 *        True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
 *        Grab state may also change.
 *
 *----------------------------------------------------------------------
 */

int
TkGenerateButtonEvent(
    int x,                /* X location of mouse */
    int y,                /* Y location of mouse */
    Window window,        /* X Window containing button event. */
    unsigned int state)   /* Button Key state suitable for X event */
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
    frontWin = FrontNonFloatingWindow();
                        
    if (0 && ((frontWin == NULL) || ((!(TkpIsWindowFloating(whichWin)) 
            && (frontWin != whichWin))
            && TkMacOSXGetCapture() == NULL))) {
        return false;
    }

    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    
    /* SetPortWindowPort(ActiveNonFloatingWindow()); */
    SetPortWindowPort(whichWin);
    GlobalToLocal(&where);
    if (tkwin != NULL) {
        tkwin = Tk_TopCoordsToWindow(tkwin, where.h, where.v, 
                &dummy, &dummy);
    }

    Tk_UpdatePointer(tkwin, x,  y, state);

    return true;
}
