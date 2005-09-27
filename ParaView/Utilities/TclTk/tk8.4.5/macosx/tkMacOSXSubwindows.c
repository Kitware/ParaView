/* 
 * tkMacOSXSubwindows.c --
 *
 *	Implements subwindows for the macintosh version of Tk.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "X11/X.h"
#include "X11/Xlib.h"
#include <stdio.h>

#include <Carbon/Carbon.h>
#include "tkMacOSXInt.h"
#include "tkMacOSXDebug.h"

/*
 * Temporary region that can be reused.
 */
static RgnHandle tmpRgn = NULL;

static void UpdateOffsets _ANSI_ARGS_((TkWindow *winPtr, int deltaX, int deltaY));

/*
 *----------------------------------------------------------------------
 *
 * XDestroyWindow --
 *
 *	Dealocates the given X Window.
 *
 * Results:
 *	The window id is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void 
XDestroyWindow(
    Display* display,		/* Display. */
    Window window)		/* Window. */
{
    MacDrawable *macWin = (MacDrawable *) window;
    CGrafPtr     destPort;
    /*
     * Remove any dangling pointers that may exist if
     * the window we are deleting is being tracked by
     * the grab code.
     */

    TkPointerDeadWindow(macWin->winPtr);
    macWin->toplevel->referenceCount--;
    
    
    if (Tk_IsTopLevel(macWin->winPtr)) {
	DisposeRgn(macWin->clipRgn);
	DisposeRgn(macWin->aboveClipRgn);
	
	/*
	 * Delete the Mac window and remove it from the windowTable.
	 * The window could be NULL if the window was never mapped.
	 * However, we don't do this for embedded windows, they don't
	 * go in the window list, and they do not own their portPtr's.
	 */
	 
	if (!(Tk_IsEmbedded(macWin->winPtr))) {
            destPort = TkMacOSXGetDrawablePort(window);
	    if (destPort != NULL) {
	        TkMacOSXWindowList *listPtr, *prevPtr;
                WindowRef        winRef;
                winRef = GetWindowFromPort(destPort);
	        TkMacOSXUnregisterMacWindow(winRef);
	        DisposeWindow(winRef);
	    
	        for (listPtr = tkMacOSXWindowListPtr, prevPtr = NULL;
	    	        tkMacOSXWindowListPtr != NULL;
	    	        prevPtr = listPtr, listPtr = listPtr->nextPtr) {
	            if (listPtr->winPtr == macWin->winPtr) {
	                if (prevPtr == NULL) {
	            	    tkMacOSXWindowListPtr = listPtr->nextPtr;
	                } else {
	            	    prevPtr->nextPtr = listPtr->nextPtr;
	                }
	                ckfree((char *) listPtr);
	                break;
	            }
	        }
	    }
	}
	
	macWin->grafPtr = NULL;
	
	/*
	 * Delay deletion of a toplevel data structure untill all
	 * children have been deleted.
	 */
	if (macWin->toplevel->referenceCount == 0) {
	    ckfree((char *) macWin->toplevel);
	}
    } else {
        CGrafPtr destPort;
        destPort = TkMacOSXGetDrawablePort(window);
	if (destPort != NULL) {
	    SetGWorld(destPort, NULL);
	    TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);
	}
	if (macWin->winPtr->parentPtr != NULL) {
	    TkMacOSXInvalClipRgns(macWin->winPtr->parentPtr);
	}
	DisposeRgn(macWin->clipRgn);
	DisposeRgn(macWin->aboveClipRgn);
	
	if (macWin->toplevel->referenceCount == 0) {
	    ckfree((char *) macWin->toplevel);
	}
	ckfree((char *) macWin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XMapWindow --
 *
 *	Map the given X Window to the screen.  See X window documentation 
 *  for more details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The subwindow or toplevel may appear on the screen.
 *
 *----------------------------------------------------------------------
 */

void 
XMapWindow(
    Display* display,		/* Display. */
    Window window)		/* Window. */
{
    MacDrawable *macWin = (MacDrawable *) window;
    XEvent event;
    CGrafPtr  destPort;

    /*
     * Under certain situations it's possible for this function to be
     * called before the toplevel window it's associated with has actually
     * been mapped.  In that case we need to create the real Macintosh
     * window now as this function as well as other X functions assume that
     * the portPtr is valid.
     */
    if (!TkMacOSXHostToplevelExists(macWin->toplevel->winPtr)) {
	TkMacOSXMakeRealWindowExist(macWin->toplevel->winPtr);
    }
    destPort = TkMacOSXGetDrawablePort (window);

    display->request++;
    macWin->winPtr->flags |= TK_MAPPED;
    if (Tk_IsTopLevel(macWin->winPtr)) {
	if (!Tk_IsEmbedded(macWin->winPtr)) {
        	    ShowWindow(GetWindowFromPort(destPort));
	}

	/* 
	 * We only need to send the MapNotify event
	 * for toplevel windows.
	 */
	event.xany.serial = display->request;
	event.xany.send_event = False;
	event.xany.display = display;
	
	event.xmap.window = window;
	event.xmap.type = MapNotify;
	event.xmap.event = window;
	event.xmap.override_redirect = macWin->winPtr->atts.override_redirect;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    } else {
	TkMacOSXInvalClipRgns(macWin->winPtr->parentPtr);
    }

    /* 
     * Generate damage for that area of the window 
     */
    SetGWorld (destPort, NULL);
    TkMacOSXUpdateClipRgn(macWin->winPtr);
    TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);
}

/*
 *----------------------------------------------------------------------
 *
 * XUnmapWindow --
 *
 *	Unmap the given X Window to the screen.  See X window
 *	documentation for more details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The subwindow or toplevel may be removed from the screen.
 *
 *----------------------------------------------------------------------
 */

void 
XUnmapWindow(
    Display* display,		/* Display. */
    Window window)		/* Window. */
{
    MacDrawable *macWin = (MacDrawable *) window;
    XEvent event;
    CGrafPtr destPort;

    destPort = TkMacOSXGetDrawablePort(window);

    display->request++;
    macWin->winPtr->flags &= ~TK_MAPPED;
    if (Tk_IsTopLevel(macWin->winPtr)) {
	if (!Tk_IsEmbedded(macWin->winPtr)) {
	    HideWindow(GetWindowFromPort(destPort));
	}

	/* 
	 * We only need to send the UnmapNotify event
	 * for toplevel windows.
	 */
	event.xany.serial = display->request;
	event.xany.send_event = False;
	event.xany.display = display;
	
	event.xunmap.type = UnmapNotify;
	event.xunmap.window = window;
	event.xunmap.event = window;
	event.xunmap.from_configure = false;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    } else {
	/* 
	 * Generate damage for that area of the window.
	 */
	SetGWorld(destPort, NULL);
	TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW); /* TODO: may not be valid */
	TkMacOSXInvalClipRgns(macWin->winPtr->parentPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XResizeWindow --
 *
 *	Resize a given X window.  See X windows documentation for
 *	further details.
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
XResizeWindow(
    Display* display,		/* Display. */
    Window window, 		/* Window. */
    unsigned int width,
    unsigned int height)
{
    MacDrawable *macWin = (MacDrawable *) window;
    CGrafPtr     destPort;

    destPort = TkMacOSXGetDrawablePort(window);
    if (destPort == NULL) {
	return;
    }

    display->request++;
    SetPort( destPort);
    if (Tk_IsTopLevel(macWin->winPtr)) {
	if (!Tk_IsEmbedded(macWin->winPtr)) {
	    /* 
	     * NOTE: we are not adding the new space to the update
	     * region.  It is currently assumed that Tk will need
	     * to completely redraw anway.
	     */
	    SizeWindow(GetWindowFromPort(destPort),
		    (short) width, (short) height, false);
	    TkMacOSXInvalidateWindow(macWin, TK_WINDOW_ONLY);
	    TkMacOSXInvalClipRgns(macWin->winPtr);
	} else {
	    int deltaX, deltaY;
	    
	    /*
	     * Find the Parent window -
	     *    For an embedded window this will be its container.
	     */
	    TkWindow *contWinPtr;
	    
	    contWinPtr = TkpGetOtherWindow(macWin->winPtr);
	    
	    if (contWinPtr != NULL) {
	        MacDrawable *macParent = contWinPtr->privatePtr;

		TkMacOSXInvalClipRgns(macParent->winPtr);	
		TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);
		
		deltaX = macParent->xOff +
		    macWin->winPtr->changes.x - macWin->xOff;
		deltaY = macParent->yOff +
		    macWin->winPtr->changes.y - macWin->yOff;
		
		UpdateOffsets(macWin->winPtr, deltaX, deltaY);
	    } else {
	        /*
	         * This is the case where we are embedded in
	         * another app.  At this point, we are assuming that
	         * the changes.x,y is not maintained, if you need
		 * the info get it from Tk_GetRootCoords,
	         * and that the toplevel sits at 0,0 when it is drawn.
	         */
		
		TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);
		UpdateOffsets(macWin->winPtr, 0, 0);
	    }
	         
	}   
    } else {
	/* TODO: update all xOff & yOffs */
	int deltaX, deltaY, parentBorderwidth;
	MacDrawable *macParent = macWin->winPtr->parentPtr->privatePtr;
	
	if (macParent == NULL) {
	    return; /* TODO: Probably should be a panic */
	}
	
	TkMacOSXInvalClipRgns(macParent->winPtr);	
	TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);

	deltaX = - macWin->xOff;
	deltaY = - macWin->yOff;

	parentBorderwidth = macWin->winPtr->parentPtr->changes.border_width;
	
	deltaX += macParent->xOff + parentBorderwidth +
	    macWin->winPtr->changes.x;
	deltaY += macParent->yOff + parentBorderwidth +
	    macWin->winPtr->changes.y;
        
	UpdateOffsets(macWin->winPtr, deltaX, deltaY);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XMoveResizeWindow --
 *
 *	Move or resize a given X window.  See X windows documentation
 *	for further details.
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
XMoveResizeWindow(
    Display* display,		/* Display. */
    Window window, 		/* Window. */
    int x, int y,
    unsigned int width,
    unsigned int height)
{	
    MacDrawable * macWin = (MacDrawable *) window;
    CGrafPtr      destPort;

    destPort   = TkMacOSXGetDrawablePort(window);
    if (destPort == NULL) {
	return;
    }

    SetPort( destPort);
    if (Tk_IsTopLevel(macWin->winPtr) && !Tk_IsEmbedded(macWin->winPtr)) {	
	/* 
	 * NOTE: we are not adding the new space to the update
	 * region.  It is currently assumed that Tk will need
	 * to completely redraw anway.
	 */
	
	SizeWindow(GetWindowFromPort(destPort),
		(short) width, (short) height, false);
	MoveWindowStructure(GetWindowFromPort(destPort), x, y);
	
	/* TODO: is the following right? */
	TkMacOSXInvalidateWindow(macWin, TK_WINDOW_ONLY);
	TkMacOSXInvalClipRgns(macWin->winPtr);
    } else {
	int deltaX, deltaY, parentBorderwidth;
	Rect bounds;
	MacDrawable *macParent;
	
        /*
         * Find the Parent window -
         *    For an embedded window this will be its container.
         */
         
	if (Tk_IsEmbedded(macWin->winPtr)) {
	    TkWindow *contWinPtr;
	    
	    contWinPtr = TkpGetOtherWindow(macWin->winPtr);
	    if (contWinPtr == NULL) {
	            panic("XMoveResizeWindow could not find container");
	    }
	    macParent = contWinPtr->privatePtr;
	    
	    /*
	     * NOTE: Here we should handle out of process embedding.
	     */
	
	    
	} else {
	    macParent = macWin->winPtr->parentPtr->privatePtr;   
	    if (macParent == NULL) {
	        return; /* TODO: Probably should be a panic */
	    }
	}
	        
	TkMacOSXInvalClipRgns(macParent->winPtr);
	TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);

	deltaX = - macWin->xOff;
	deltaY = - macWin->yOff;
	
        /*
	 * If macWin->winPtr is an embedded window, don't offset by its
	 *  parent's borderwidth...
	 */
	 
	if (!Tk_IsEmbedded(macWin->winPtr)) {
	    parentBorderwidth = macWin->winPtr->parentPtr->changes.border_width;
	} else {
	    parentBorderwidth = 0;
	}
	deltaX += macParent->xOff + parentBorderwidth +
	    macWin->winPtr->changes.x;
	deltaY += macParent->yOff + parentBorderwidth +
	    macWin->winPtr->changes.y;
		
	UpdateOffsets(macWin->winPtr, deltaX, deltaY);
	TkMacOSXWinBounds(macWin->winPtr, &bounds);
	InvalWindowRect(GetWindowFromPort(destPort),&bounds);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XMoveWindow --
 *
 *	Move a given X window.  See X windows documentation for further
 *  details.
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
XMoveWindow(
    Display* display,		/* Display. */
    Window window,		/* Window. */
    int x,
    int y)
{
    MacDrawable *macWin = (MacDrawable *) window;
    CGrafPtr  destPort;

    destPort   = TkMacOSXGetDrawablePort(window);
    if (destPort == NULL) {
	return;
    }

    SetPort( destPort);
    if (Tk_IsTopLevel(macWin->winPtr) && !Tk_IsEmbedded(macWin->winPtr)) {
	/* 
	 * NOTE: we are not adding the new space to the update
	 * region.  It is currently assumed that Tk will need
	 * to completely redraw anway.
	 */
	MoveWindowStructure( GetWindowFromPort(destPort), x, y);

	/* TODO: is the following right? */
	TkMacOSXInvalidateWindow(macWin, TK_WINDOW_ONLY);
	TkMacOSXInvalClipRgns(macWin->winPtr);
    } else {
	int deltaX, deltaY, parentBorderwidth;
	Rect bounds;
	MacDrawable *macParent;
	
        /*
         * Find the Parent window -
         * For an embedded window this will be its container.
         */
         
	if (Tk_IsEmbedded(macWin->winPtr)) {
	    TkWindow *contWinPtr;
	    
	    contWinPtr = TkpGetOtherWindow(macWin->winPtr);
	    if (contWinPtr == NULL) {
	            panic("XMoveWindow could not find container");
	    }
	    macParent = contWinPtr->privatePtr;
	    
	    /*
	     * NOTE: Here we should handle out of process embedding.
	     */
		    
	} else {
	    macParent = macWin->winPtr->parentPtr->privatePtr;   
	    if (macParent == NULL) {
	        return; /* TODO: Probably should be a panic */
	    }
	}

	TkMacOSXInvalClipRgns(macParent->winPtr);
	TkMacOSXInvalidateWindow(macWin, TK_PARENT_WINDOW);

	deltaX = - macWin->xOff;
	deltaY = - macWin->yOff;
	
        /*
	 * If macWin->winPtr is an embedded window, don't offset by its
	 *  parent's borderwidth...
	 */
	 
	if (!Tk_IsEmbedded(macWin->winPtr)) {
	    parentBorderwidth = macWin->winPtr->parentPtr->changes.border_width;
	} else {
	    parentBorderwidth = 0;
	}
	deltaX += macParent->xOff + parentBorderwidth +
	    macWin->winPtr->changes.x;
	deltaY += macParent->yOff + parentBorderwidth +
	    macWin->winPtr->changes.y;
		
	UpdateOffsets(macWin->winPtr, deltaX, deltaY);
	TkMacOSXWinBounds(macWin->winPtr, &bounds);
	InvalWindowRect(GetWindowFromPort(destPort),&bounds);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XRaiseWindow --
 *
 *	Change the stacking order of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the stacking order of the specified window.
 *
 *----------------------------------------------------------------------
 */

void 
XRaiseWindow(
    Display* display,		/* Display. */
    Window window)		/* Window. */
{
    MacDrawable *macWin = (MacDrawable *) window;
    
    display->request++;
    if (Tk_IsTopLevel(macWin->winPtr) && !Tk_IsEmbedded(macWin->winPtr)) {
	TkWmRestackToplevel(macWin->winPtr, Above, NULL);
    } else {
    	/* TODO: this should generate damage */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XLowerWindow --
 *
 *	Change the stacking order of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the stacking order of the specified window.
 *
 *----------------------------------------------------------------------
 */

void 
XLowerWindow(
    Display* display,		/* Display. */
    Window window)		/* Window. */
{
    MacDrawable *macWin = (MacDrawable *) window;
    
    display->request++;
    if (Tk_IsTopLevel(macWin->winPtr) && !Tk_IsEmbedded(macWin->winPtr)) {
	TkWmRestackToplevel(macWin->winPtr, Below, NULL);
    } else {
    	/* TODO: this should generate damage */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XConfigureWindow --
 *
 *	Change the size, position, stacking, or border of the specified
 *	window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the attributes of the specified window.  Note that we
 *	ignore the passed in values and use the values stored in the
 *	TkWindow data structure.
 *
 *----------------------------------------------------------------------
 */

void
XConfigureWindow(
    Display* display,		/* Display. */
    Window w,			/* Window. */
    unsigned int value_mask,
    XWindowChanges* values)
{
    MacDrawable *macWin = (MacDrawable *) w;
    TkWindow *winPtr = macWin->winPtr;

    display->request++;

    /*
     * Change the shape and/or position of the window.
     */

    if (value_mask & (CWX|CWY|CWWidth|CWHeight)) {
	XMoveResizeWindow(display, w, winPtr->changes.x, winPtr->changes.y,
		winPtr->changes.width, winPtr->changes.height);
    }

    /*
     * Change the stacking order of the window.  Tk actuall keeps all
     * the information we need for stacking order.  All we need to do
     * is make sure the clipping regions get updated and generate damage
     * that will ensure things get drawn correctly.
     */

    if (value_mask & CWStackMode) {
	Rect bounds;
	CGrafPtr destPort;
	
	destPort = TkMacOSXGetDrawablePort(w);
	if (destPort != NULL) {
	    SetPort( destPort);
	    TkMacOSXInvalClipRgns(winPtr->parentPtr);
	    TkMacOSXWinBounds(winPtr, &bounds);
	    InvalWindowRect(GetWindowFromPort(destPort),&bounds);
	}
    } 

    /* TkGenWMMoveRequestEvent(macWin->winPtr, 
	    macWin->winPtr->changes.x, macWin->winPtr->changes.y); */
}

/*
 *----------------------------------------------------------------------
 *
 *  TkMacOSXUpdateClipRgn --
 *
 *	This function updates the cliping regions for a given window
 *	and all of its children.  Once updated the TK_CLIP_INVALID flag
 *	in the subwindow data structure is unset.  The TK_CLIP_INVALID 
 *	flag should always be unset before any drawing is attempted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The clip regions for the window and its children are updated.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXUpdateClipRgn(
    TkWindow *winPtr)
{
    RgnHandle rgn;
    int x, y;
    TkWindow *win2Ptr;

    if (winPtr == NULL) {
	return;
    }
    
    if (winPtr->privatePtr->flags & TK_CLIP_INVALID) {
	rgn = winPtr->privatePtr->aboveClipRgn;
	if (tmpRgn == NULL) {
	    tmpRgn = NewRgn();
	}
	
	/* 
	 * Start with a region defined by the window bounds.  
	 */

        x = winPtr->privatePtr->xOff;
        y = winPtr->privatePtr->yOff;
        SetRectRgn(rgn, (short) x, (short) y,
	    (short) (winPtr->changes.width  + x), 
	    (short) (winPtr->changes.height + y));
	    
	/* 
	 * Clip away the area of any windows that may obscure this
	 * window.  
	 * For a non-toplevel window, first, clip to the parents visable
	 * clip region.
	 * Second, clip away any siblings that are higher in the
	 * stacking order.
	 * For an embedded toplevel, just clip to the container's visible
	 * clip region.  Remember, we only allow one contained window 
	 * in a frame, and don't support any other widgets in the frame either.
	 * This is not currently enforced, however.
	 */
	
	if (!Tk_IsTopLevel(winPtr)) { 
	    TkMacOSXUpdateClipRgn(winPtr->parentPtr);
	    SectRgn(rgn, 
		    winPtr->parentPtr->privatePtr->aboveClipRgn, rgn);
				
	    win2Ptr = winPtr->nextPtr;
	    while (win2Ptr != NULL) {
		if (Tk_IsTopLevel(win2Ptr) || !Tk_IsMapped(win2Ptr)) {
		    win2Ptr = win2Ptr->nextPtr;
		    continue;
		}
		x = win2Ptr->privatePtr->xOff;
		y = win2Ptr->privatePtr->yOff;
		SetRectRgn(tmpRgn, (short) x, (short) y,
			(short) (win2Ptr->changes.width  + x), 
			(short) (win2Ptr->changes.height + y));
		DiffRgn(rgn, tmpRgn, rgn);
							  
		win2Ptr = win2Ptr->nextPtr;
	    }
	} else if (Tk_IsEmbedded(winPtr)) {
            TkWindow *contWinPtr;
        
	    contWinPtr = TkpGetOtherWindow(winPtr);
    	     
    	    if (contWinPtr != NULL) {
 	        TkMacOSXUpdateClipRgn(contWinPtr);
	        SectRgn(rgn, 
		        contWinPtr->privatePtr->aboveClipRgn, rgn);
   	    } else if (gMacEmbedHandler != NULL) {
   	        gMacEmbedHandler->getClipProc((Tk_Window) winPtr, tmpRgn);
   	        SectRgn(rgn, tmpRgn, rgn);
   	    }
	    
	    /*
	     * NOTE: Here we should handle out of process embedding.
	     */
		    
	}
	
	/* 
	 * The final clip region is the aboveClip region (or visable
	 * region) minus all the children of this window.
	 * Alternatively, if the window is a container, we must also 
	 * subtract the region of the embedded window.
	 */
	 
	rgn = winPtr->privatePtr->clipRgn;
	CopyRgn(winPtr->privatePtr->aboveClipRgn, rgn);
		
	win2Ptr = winPtr->childList;
	while (win2Ptr != NULL) {
	    if (Tk_IsTopLevel(win2Ptr) || !Tk_IsMapped(win2Ptr)) {
		win2Ptr = win2Ptr->nextPtr;
		continue;
	    }
	    x = win2Ptr->privatePtr->xOff;
	    y = win2Ptr->privatePtr->yOff;
	    SetRectRgn(tmpRgn, (short) x, (short) y,
		    (short) (win2Ptr->changes.width  + x), 
		    (short) (win2Ptr->changes.height + y));
	    DiffRgn(rgn, tmpRgn, rgn);
							  
	    win2Ptr = win2Ptr->nextPtr;
	}
	
	if (Tk_IsContainer(winPtr)) {
	    win2Ptr = TkpGetOtherWindow(winPtr);
	    if (win2Ptr != NULL) {
		if (Tk_IsMapped(win2Ptr)) {
		    x = win2Ptr->privatePtr->xOff;
		    y = win2Ptr->privatePtr->yOff;
		    SetRectRgn(tmpRgn, (short) x, (short) y,
			    (short) (win2Ptr->changes.width  + x), 
			    (short) (win2Ptr->changes.height + y));
		    DiffRgn(rgn, tmpRgn, rgn);
		}
	    } 
	    
	    /*
	     * NOTE: Here we should handle out of process embedding.
	     */
		    
	}
		
	winPtr->privatePtr->flags &= ~TK_CLIP_INVALID;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXVisableClipRgn --
 *
 *	This function returnd the Macintosh cliping region for the 
 *	given window.  A NULL Rgn means the window is not visable.
 *
 * Results:
 *	The region.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

RgnHandle
TkMacOSXVisableClipRgn(
    TkWindow *winPtr)
{
    if (winPtr->privatePtr->flags & TK_CLIP_INVALID) {
	TkMacOSXUpdateClipRgn(winPtr);
    }

    return winPtr->privatePtr->clipRgn;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInvalidateWindow --
 *
 *	This function makes the window as invalid will generate damage
 *	for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Damage is created.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXInvalidateWindow(
    MacDrawable *macWin,        /* Make window that's causing damage. */
    int flag)			/* Should be TK_WINDOW_ONLY or
				 * TK_PARENT_WINDOW */
{
    WindowRef windowRef;
    CGrafPtr  grafPtr;

    grafPtr=TkMacOSXGetDrawablePort((Drawable)macWin);
    windowRef=GetWindowFromPort(grafPtr);
    
    if (flag == TK_WINDOW_ONLY) {
	InvalWindowRgn(windowRef,macWin->clipRgn);
    } else {
	if (!EmptyRgn(macWin->aboveClipRgn)) {
	    InvalWindowRgn(windowRef,macWin->aboveClipRgn);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetDrawablePort --
 *
 *	This function returns the Graphics Port for a given X drawable.
 *
 * Results:
 *	A CGrafPort .  Either an off screen pixmap or a Window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

CGrafPtr
TkMacOSXGetDrawablePort(
    Drawable drawable)
{
    MacDrawable *macWin = (MacDrawable *) drawable;
    GWorldPtr resultPort = NULL;
    
    if (macWin == NULL) {
        return NULL;
    }
    
    /*
     * This is NULL for off-screen pixmaps.  Then the portPtr
     * always points to the off-screen port, and we don't
     * have to worry about containment
     */
     
    if (macWin->clipRgn == NULL) {
	return macWin->grafPtr;
    }
    
    /*
     * If the Drawable is in an embedded window, use the Port of its container.
     *  
     * TRICKY POINT: we can have cases when a toplevel is being destroyed
     * where the winPtr for the toplevel has been freed, but the children 
     * are not all the way destroyed.  The children will call this function
     * as they are being destroyed, but Tk_IsEmbedded will return garbage.
     * So we check the copy of the TK_EMBEDDED flag we put into the 
     * toplevel's macWin flags.
     */
     
     
     
    
    if (!(macWin->toplevel->flags & TK_EMBEDDED)) {
        return macWin->toplevel->grafPtr;
    } else {
    	TkWindow *contWinPtr;

	contWinPtr = TkpGetOtherWindow(macWin->toplevel->winPtr);
	
    	if (contWinPtr != NULL) {
    	    resultPort = TkMacOSXGetDrawablePort(
		(Drawable) contWinPtr->privatePtr);
    	} else if (gMacEmbedHandler != NULL) {
	    resultPort = gMacEmbedHandler->getPortProc(
                    (Tk_Window) macWin->winPtr);
    	} 
	
	if (resultPort == NULL) {
	    /*
	     * FIXME:
	     *
	     * So far as I can tell, the only time that this happens is when
	     * we are tearing down an embedded child interpreter, and most
	     * of the time, this is harmless...  However, we really need to
	     * find why the embedding loses.
	     */
	    DebugStr("\pTkMacOSXGetDrawablePort couldn't find container");
    	    return NULL;
    	}	
	    
	/*
	 * NOTE: Here we should handle out of process embedding.
	 */
		    
    }
    return resultPort;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetRootControl --
 *
 *	This function returns the Root Control for a given X drawable.
 *
 * Results:
 *	A ControlRef .
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ControlRef
TkMacOSXGetRootControl(
    Drawable drawable)
{
    /*
     * will probably need to fix this up for embedding
     */
    MacDrawable *macWin = (MacDrawable *) drawable;
    ControlRef result = NULL;
    
    if (macWin == NULL) {
        return NULL;
    }
    if (!(macWin->toplevel->flags & TK_EMBEDDED)) {
        return macWin->toplevel->rootControl;
    } else {
        TkWindow *contWinPtr;

        contWinPtr = TkpGetOtherWindow(macWin->toplevel->winPtr);

        if (contWinPtr != NULL) {
            result = TkMacOSXGetRootControl(
                (Drawable) contWinPtr->privatePtr);
        } else if (gMacEmbedHandler != NULL) {
            result = NULL;
        }
   }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInvalClipRgns --
 *
 *	This function invalidates the clipping regions for a given
 *	window and all of its children.  This function should be
 *	called whenever changes are made to subwindows that would
 *	effect the size or position of windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cliping regions for the window and its children are
 *	mark invalid.  (Make sure they are valid before drawing.)
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXInvalClipRgns(
    TkWindow *winPtr)
{
    TkWindow *childPtr;
	
    /* 
     * If already marked we can stop because all 
     * decendants will also already be marked.
     */
    if (winPtr->privatePtr->flags & TK_CLIP_INVALID) {
	return;
    }
	
    winPtr->privatePtr->flags |= TK_CLIP_INVALID;
	
    /* 
     * Invalidate clip regions for all children & 
     * their decendants - unless the child is a toplevel.
     */
    childPtr = winPtr->childList;
    while (childPtr != NULL) {
	if (!Tk_IsTopLevel(childPtr) && Tk_IsMapped(childPtr)) {
	    TkMacOSXInvalClipRgns(childPtr);
	}
	childPtr = childPtr->nextPtr;
    }
    
    /*
     * Also, if the window is a container, mark its embedded window
     */
     
    if (Tk_IsContainer(winPtr)) {
	childPtr = TkpGetOtherWindow(winPtr);

	if (childPtr != NULL && Tk_IsMapped(childPtr)) {
	    TkMacOSXInvalClipRgns(childPtr);
	}
	
	/*
	 * NOTE: Here we should handle out of process embedding.
	 */
		    	
    }     	    
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXWinBounds --
 *
 *	Given a Tk window this function determines the windows
 *	bounds in relation to the Macintosh window's coordinate
 *	system.  This is also the same coordinate system as the
 *	Tk toplevel window in which this window is contained.
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
TkMacOSXWinBounds(
    TkWindow *winPtr,
    Rect *bounds)
{
    bounds->left = (short) winPtr->privatePtr->xOff;
    bounds->top = (short) winPtr->privatePtr->yOff;
    bounds->right = (short) (winPtr->privatePtr->xOff +
	    winPtr->changes.width);
    bounds->bottom = (short) (winPtr->privatePtr->yOff +
	    winPtr->changes.height);
}
/*
 *----------------------------------------------------------------------
 *
 * UpdateOffsets --
 *
 *	Updates the X & Y offsets of the given TkWindow from the
 *	TopLevel it is a decendant of.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The xOff & yOff fields for the Mac window datastructure
 *	is updated to the proper offset.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateOffsets(
    TkWindow *winPtr,
    int deltaX,
    int deltaY)
{
    TkWindow *childPtr;

    if (winPtr->privatePtr == NULL) {
	/*
	 * We havn't called Tk_MakeWindowExist for this window yet.  The
	 * offset information will be postponed and calulated at that 
	 * time.  (This will usually only happen when a mapped parent is
	 * being moved but has child windows that have yet to be mapped.)
	 */
	return;
    }
    
    winPtr->privatePtr->xOff += deltaX;
    winPtr->privatePtr->yOff += deltaY;

    childPtr = winPtr->childList;
    while (childPtr != NULL) {
	if (!Tk_IsTopLevel(childPtr)) {
	    UpdateOffsets(childPtr, deltaX, deltaY);
	}
	childPtr = childPtr->nextPtr;
    }
    
    if (Tk_IsContainer(winPtr)) {
	childPtr = TkpGetOtherWindow(winPtr);
	if (childPtr != NULL) {
	    UpdateOffsets(childPtr,deltaX,deltaY);
	}
	    
	/*
	 * NOTE: Here we should handle out of process embedding.
	 */
		    
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetPixmap --
 *
 *	Creates an in memory drawing surface.
 *
 * Results:
 *	Returns a handle to a new pixmap.
 *
 * Side effects:
 *	Allocates a new Macintosh GWorld.
 *
 *----------------------------------------------------------------------
 */

Pixmap
Tk_GetPixmap(
    Display *display,	/* Display for new pixmap (can be null). */
    Drawable d,		/* Drawable where pixmap will be used (ignored). */
    int width,		/* Dimensions of pixmap. */
    int height,
    int depth)		/* Bits per pixel for pixmap. */
{
    QDErr err;
    GWorldPtr gWorld;
    Rect bounds;
    MacDrawable *macPix;
    PixMapHandle pixels;
    
    if (display != NULL) {
	display->request++;
    }
    macPix = (MacDrawable *) ckalloc(sizeof(MacDrawable));
    macPix->winPtr = NULL;
    macPix->xOff = 0;
    macPix->yOff = 0;
    macPix->clipRgn = NULL;
    macPix->aboveClipRgn = NULL;
    macPix->referenceCount = 0;
    macPix->toplevel = NULL;
    macPix->flags = 0;

    bounds.top = bounds.left = 0;
    bounds.right = (short) width;
    bounds.bottom = (short) height;
    if (depth != 1) {
	depth = 0;
    }
    /*
     * Allocate memory for the off screen pixmap.  If we fail
     * try again from system memory.  Eventually, we may have
     * to panic.
     */
    err = NewGWorld(&gWorld, depth, &bounds, NULL, NULL, 0);
    if (err != noErr) {
	err = NewGWorld(&gWorld, depth, &bounds, NULL, NULL, useTempMem);
    }
    if (err != noErr) {
        panic("Out of memory: NewGWorld failed in Tk_GetPixmap");
    }

    /*
     * Lock down the pixels so they don't move out from under us.
     */
    pixels = GetGWorldPixMap(gWorld);
    LockPixels(pixels);
    macPix->grafPtr = gWorld;

    return (Pixmap) macPix;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreePixmap --
 *
 *	Release the resources associated with a pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the Macintosh GWorld created by Tk_GetPixmap.
 *
 *----------------------------------------------------------------------
 */

void 
Tk_FreePixmap(
    Display *display,		/* Display. */
    Pixmap pixmap)     		/* Pixmap to destroy */
{
    MacDrawable *macPix = (MacDrawable *) pixmap;
    PixMapHandle pixels;

    display->request++;
    pixels = GetGWorldPixMap(macPix->grafPtr);
    UnlockPixels(pixels);
    DisposeGWorld(macPix->grafPtr);
    ckfree((char *) macPix);
}
