/* 
 * tkPointer.c --
 *
 *	This file contains functions for emulating the X server
 *	pointer and grab state machine.  This file is used by the
 *	Mac and Windows platforms to generate appropriate enter/leave
 *	events, and to update the global grab window information.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"

#ifdef __WIN32__
#include "tkWinInt.h"
#endif

#ifdef MAC_TCL
#define Cursor XCursor
#endif

/*
 * Mask that selects any of the state bits corresponding to buttons,
 * plus masks that select individual buttons' bits:
 */

#define ALL_BUTTONS \
	(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)
static unsigned int buttonMasks[] = {
    Button1Mask, Button2Mask, Button3Mask, Button4Mask, Button5Mask
};
#define ButtonMask(b) (buttonMasks[(b)-Button1])

typedef struct ThreadSpecificData {
    TkWindow *grabWinPtr;	        /* Window that defines the top of the
					 * grab tree in a global grab. */
    int lastState;		        /* Last known state flags. */
    XPoint lastPos;	                /* Last reported mouse position. */
    TkWindow *lastWinPtr;	        /* Last reported mouse window. */
    TkWindow *restrictWinPtr;    	/* Window to which all mouse events
					 * will be reported. */
    TkWindow *cursorWinPtr;	        /* Window that is currently
					 * controlling the global cursor. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations of procedures used in this file.
 */

static int		GenerateEnterLeave _ANSI_ARGS_((TkWindow *winPtr,
			    int x, int y, int state));
static void		InitializeEvent _ANSI_ARGS_((XEvent* eventPtr,
			    TkWindow *winPtr, int type, int x, int y,
			    int state, int detail));
static void		UpdateCursor _ANSI_ARGS_((TkWindow *winPtr));

/*
 *----------------------------------------------------------------------
 *
 * InitializeEvent --
 *
 *	Initializes the common fields for several X events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in the specified event structure.
 *
 *----------------------------------------------------------------------
 */

static void
InitializeEvent(eventPtr, winPtr, type, x, y, state, detail)
    XEvent* eventPtr;		/* Event structure to initialize. */
    TkWindow *winPtr;		/* Window to make event relative to. */
    int type;			/* Message type. */
    int x, y;			/* Root coords of event. */
    int state;			/* State flags. */
    int detail;			/* Detail value. */
{
    eventPtr->type = type;
    eventPtr->xany.serial = LastKnownRequestProcessed(winPtr->display);
    eventPtr->xany.send_event = False;
    eventPtr->xany.display = winPtr->display;

    eventPtr->xcrossing.root = RootWindow(winPtr->display, winPtr->screenNum);
    eventPtr->xcrossing.time = TkpGetMS();
    eventPtr->xcrossing.x_root = x;
    eventPtr->xcrossing.y_root = y;

    switch (type) {
	case EnterNotify:
	case LeaveNotify:
	    eventPtr->xcrossing.mode = NotifyNormal;
	    eventPtr->xcrossing.state = state;
	    eventPtr->xcrossing.detail = detail;
	    eventPtr->xcrossing.focus = False;
	    break;
	case MotionNotify:
	    eventPtr->xmotion.state = state;
	    eventPtr->xmotion.is_hint = detail;
	    break;
	case ButtonPress:
	case ButtonRelease:
	    eventPtr->xbutton.state = state;
	    eventPtr->xbutton.button = detail;
	    break;
    }
    TkChangeEventWindow(eventPtr, winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateEnterLeave --
 *
 *	Update the current mouse window and position, and generate
 *	any enter/leave events that are needed.
 *
 * Results:
 *	Returns 1 if enter/leave events were generated.
 *
 * Side effects:
 *	May insert events into the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateEnterLeave(winPtr, x, y, state)
    TkWindow *winPtr;		/* Current Tk window (or NULL). */
    int x,y;			/* Current mouse position in root coords. */
    int state;			/* State flags. */
{
    int crossed = 0;		/* 1 if mouse crossed a window boundary */
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    TkWindow *restrictWinPtr = tsdPtr->restrictWinPtr;
    TkWindow *lastWinPtr = tsdPtr->lastWinPtr;

    if (winPtr != tsdPtr->lastWinPtr) {
	if (restrictWinPtr) {
	    int newPos, oldPos;

	    newPos = TkPositionInTree(winPtr, restrictWinPtr);
	    oldPos = TkPositionInTree(lastWinPtr, restrictWinPtr);

	    /*
	     * Check if the mouse crossed into or out of the restrict
	     * window.  If so, we need to generate an Enter or Leave event.
	     */

	    if ((newPos != oldPos) && ((newPos == TK_GRAB_IN_TREE)
		    || (oldPos == TK_GRAB_IN_TREE))) {
		XEvent event;
		int type, detail;

		if (newPos == TK_GRAB_IN_TREE) {
		    type = EnterNotify;
		} else {
		    type = LeaveNotify;
		}
		if ((oldPos == TK_GRAB_ANCESTOR)
			|| (newPos == TK_GRAB_ANCESTOR)) {
		    detail = NotifyAncestor;
		} else {
		    detail = NotifyVirtual;
		}
		InitializeEvent(&event, restrictWinPtr, type, x, y,
			state, detail);
		Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	    }

	} else {
	    TkWindow *targetPtr;

	    if ((lastWinPtr == NULL)
		|| (lastWinPtr->window == None)) {
		targetPtr = winPtr;
	    } else {
		targetPtr = lastWinPtr;
	    }

	    if (targetPtr && (targetPtr->window != None)) {
		XEvent event;

		/*
		 * Generate appropriate Enter/Leave events.
		 */

		InitializeEvent(&event, targetPtr, LeaveNotify, x, y, state,
			NotifyNormal);

		TkInOutEvents(&event, lastWinPtr, winPtr, LeaveNotify,
			EnterNotify, TCL_QUEUE_TAIL);
		crossed = 1;
	    }
	}
	tsdPtr->lastWinPtr = winPtr;
    }

    return crossed;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UpdatePointer --
 *
 *	This function updates the pointer state machine given an
 *	the current window, position and modifier state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May queue new events and update the grab state.
 *
 *----------------------------------------------------------------------
 */

void
Tk_UpdatePointer(tkwin, x, y, state)
    Tk_Window tkwin;		/* Window to which pointer event
				 * is reported. May be NULL. */
    int x, y;			/* Pointer location in root coords. */
    int state;			/* Modifier state mask. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    TkWindow *winPtr = (TkWindow *)tkwin;
    TkWindow *targetWinPtr;
    XPoint pos;
    XEvent event;
    int changes = (state ^ tsdPtr->lastState) & ALL_BUTTONS;
    int type, b, mask;

    pos.x = x;
    pos.y = y;

    /*
     * Use the current keyboard state, but the old mouse button
     * state since we haven't generated the button events yet.
     */

    tsdPtr->lastState = (state & ~ALL_BUTTONS) | (tsdPtr->lastState
	    & ALL_BUTTONS);

    /*
     * Generate Enter/Leave events.  If the pointer has crossed window
     * boundaries, update the current mouse position so we don't generate
     * redundant motion events.
     */

    if (GenerateEnterLeave(winPtr, x, y, tsdPtr->lastState)) {
	tsdPtr->lastPos = pos;
    }

    /*
     * Generate ButtonPress/ButtonRelease events based on the differences
     * between the current button state and the last known button state.
     */

    for (b = Button1; b <= Button3; b++) {
	mask = ButtonMask(b);
	if (changes & mask) {
	    if (state & mask) {	
		type = ButtonPress;

	        /*
		 * ButtonPress - Set restrict window if we aren't grabbed, or
		 * if this is the first button down.
		 */

		if (!tsdPtr->restrictWinPtr) {
		    if (!tsdPtr->grabWinPtr) {

			/*
			 * Mouse is not grabbed, so set a button grab.
			 */

			tsdPtr->restrictWinPtr = winPtr;
			TkpSetCapture(tsdPtr->restrictWinPtr);

		    } else if ((tsdPtr->lastState & ALL_BUTTONS) == 0) {

			/*
			 * Mouse is in a non-button grab, so ensure
			 * the button grab is inside the grab tree.
			 */

			if (TkPositionInTree(winPtr, tsdPtr->grabWinPtr)
				== TK_GRAB_IN_TREE) {
			    tsdPtr->restrictWinPtr = winPtr;
			} else {
			    tsdPtr->restrictWinPtr = tsdPtr->grabWinPtr;
			}
			TkpSetCapture(tsdPtr->restrictWinPtr);
		    }
		}

	    } else {
		type = ButtonRelease;

	        /*
		 * ButtonRelease - Release the mouse capture and clear the
		 * restrict window when the last button is released and we
		 * aren't in a global grab.
		 */

		if ((tsdPtr->lastState & ALL_BUTTONS) == mask) {
		    if (!tsdPtr->grabWinPtr) {
			TkpSetCapture(NULL);
		    }
		}

		/*
		 * If we are releasing a restrict window, then we need
		 * to send the button event followed by mouse motion from
		 * the restrict window to the current mouse position.
		 */

		if (tsdPtr->restrictWinPtr) {
		    InitializeEvent(&event, tsdPtr->restrictWinPtr, type, x, y,
			    tsdPtr->lastState, b);
		    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
		    tsdPtr->lastState &= ~mask;
		    tsdPtr->lastWinPtr = tsdPtr->restrictWinPtr;
		    tsdPtr->restrictWinPtr = NULL;

		    GenerateEnterLeave(winPtr, x, y, tsdPtr->lastState);
		    tsdPtr->lastPos = pos;
		    continue;
		}		
	    }

	    /*
	     * If a restrict window is set, make sure the pointer event
	     * is reported relative to that window.  Otherwise, if a
	     * global grab is in effect then events outside of windows
	     * managed by Tk should be reported to the grab window.
	     */

	    if (tsdPtr->restrictWinPtr) {
		targetWinPtr = tsdPtr->restrictWinPtr;
	    } else if (tsdPtr->grabWinPtr && !winPtr) {
		targetWinPtr = tsdPtr->grabWinPtr;
	    } else {
		targetWinPtr = winPtr;
	    }

	    /*
	     * If we still have a target window, send the event.
	     */

	    if (winPtr != NULL) {
		InitializeEvent(&event, targetWinPtr, type, x, y,
			tsdPtr->lastState, b);
		Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	    }

	    /*
	     * Update the state for the next iteration.
	     */

	    tsdPtr->lastState = (type == ButtonPress)
		? (tsdPtr->lastState | mask) : (tsdPtr->lastState & ~mask);
	    tsdPtr->lastPos = pos;
	}
    }

    /*
     * Make sure the cursor window is up to date.
     */

    if (tsdPtr->restrictWinPtr) {
	targetWinPtr = tsdPtr->restrictWinPtr;
    } else if (tsdPtr->grabWinPtr) {
	targetWinPtr = (TkPositionInTree(winPtr, tsdPtr->grabWinPtr)
		== TK_GRAB_IN_TREE) ? winPtr : tsdPtr->grabWinPtr;
    } else {
	targetWinPtr = winPtr;
    }
    UpdateCursor(targetWinPtr);

    /*
     * If no other events caused the position to be updated,
     * generate a motion event.
     */

    if (tsdPtr->lastPos.x != pos.x || tsdPtr->lastPos.y != pos.y) {
	if (tsdPtr->restrictWinPtr) {
	    targetWinPtr = tsdPtr->restrictWinPtr;
	} else if (tsdPtr->grabWinPtr && !winPtr) {
	    targetWinPtr = tsdPtr->grabWinPtr;
	}

	if (targetWinPtr != NULL) {
	    InitializeEvent(&event, targetWinPtr, MotionNotify, x, y,
		    tsdPtr->lastState, NotifyNormal);
	    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	}
	tsdPtr->lastPos = pos;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XGrabPointer --
 *
 *	Capture the mouse so event are reported outside of toplevels.
 *	Note that this is a very limited implementation that only
 *	supports GrabModeAsync and owner_events True.
 *
 * Results:
 *	Always returns GrabSuccess.
 *
 * Side effects:
 *	Turns on mouse capture, sets the global grab pointer, and
 *	clears any window restrictions.
 *
 *----------------------------------------------------------------------
 */

int
XGrabPointer(display, grab_window, owner_events, event_mask, pointer_mode,
	keyboard_mode, confine_to, cursor, time)
    Display* display;
    Window grab_window;
    Bool owner_events;
    unsigned int event_mask;
    int pointer_mode;
    int keyboard_mode;
    Window confine_to;
    Cursor cursor;
    Time time;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    display->request++;
    tsdPtr->grabWinPtr = (TkWindow *) Tk_IdToWindow(display, grab_window);
    tsdPtr->restrictWinPtr = NULL;
    TkpSetCapture(tsdPtr->grabWinPtr);
    if (TkPositionInTree(tsdPtr->lastWinPtr, tsdPtr->grabWinPtr) 
            != TK_GRAB_IN_TREE) {
	UpdateCursor(tsdPtr->grabWinPtr);
    }
    return GrabSuccess;
}

/*
 *----------------------------------------------------------------------
 *
 * XUngrabPointer --
 *
 *	Release the current grab.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases the mouse capture.
 *
 *----------------------------------------------------------------------
 */

void
XUngrabPointer(display, time)
    Display* display;
    Time time;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    display->request++;
    tsdPtr->grabWinPtr = NULL;
    tsdPtr->restrictWinPtr = NULL;
    TkpSetCapture(NULL);
    UpdateCursor(tsdPtr->lastWinPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkPointerDeadWindow --
 *
 *	Clean up pointer module state when a window is destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May release the current capture window.
 *
 *----------------------------------------------------------------------
 */

void
TkPointerDeadWindow(winPtr)
    TkWindow *winPtr;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (winPtr == tsdPtr->lastWinPtr) {
	tsdPtr->lastWinPtr = NULL;
    }
    if (winPtr == tsdPtr->grabWinPtr) {
	tsdPtr->grabWinPtr = NULL;
    }
    if (winPtr == tsdPtr->restrictWinPtr) {
	tsdPtr->restrictWinPtr = NULL;
    }
    if (!(tsdPtr->restrictWinPtr || tsdPtr->grabWinPtr)) {
	TkpSetCapture(NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateCursor --
 *
 *	Set the windows global cursor to the cursor associated with
 *	the given Tk window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the mouse cursor.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateCursor(winPtr)
    TkWindow *winPtr;
{
    Cursor cursor = None;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * A window inherits its cursor from its parent if it doesn't
     * have one of its own.  Top level windows inherit the default
     * cursor.
     */

    tsdPtr->cursorWinPtr = winPtr;
    while (winPtr != NULL) {
	if (winPtr->atts.cursor != None) {
	    cursor = winPtr->atts.cursor;
	    break;
	} else if (winPtr->flags & TK_TOP_LEVEL) {
	    break;
	}
	winPtr = winPtr->parentPtr;
    }
    TkpSetCursor((TkpCursor) cursor);
}

/*
 *----------------------------------------------------------------------
 *
 * XDefineCursor --
 *
 *	This function is called to update the cursor on a window.
 *	Since the mouse might be in the specified window, we need to
 *	check the specified window against the current mouse position
 *	and grab state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May update the cursor.
 *
 *----------------------------------------------------------------------
 */

void
XDefineCursor(display, w, cursor)
    Display* display;
    Window w;
    Cursor cursor;
{
    TkWindow *winPtr = (TkWindow *)Tk_IdToWindow(display, w);
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->cursorWinPtr == winPtr) {
	UpdateCursor(winPtr);
    }
    display->request++;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenerateActivateEvents --
 *
 *	This function is called by the Mac and Windows window manager
 *	routines when a toplevel window is activated or deactivated.
 *	Activate/Deactivate events will be sent to every subwindow of
 *	the toplevel followed by a FocusIn/FocusOut message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates X events.
 *
 *----------------------------------------------------------------------
 */

void
TkGenerateActivateEvents(winPtr, active)
    TkWindow *winPtr;		/* Toplevel to activate. */
    int active;			/* Non-zero if the window is being
				 * activated, else 0.*/
{
    XEvent event;
    
    /* 
     * Generate Activate and Deactivate events.  This event
     * is sent to every subwindow in a toplevel window.
     */

    event.xany.serial = winPtr->display->request++;
    event.xany.send_event = False;
    event.xany.display = winPtr->display;
    event.xany.window = winPtr->window;

    event.xany.type = active ? ActivateNotify : DeactivateNotify;
    TkQueueEventForAllChildren(winPtr, &event);
    
}
