/* 
 * tkMacEmbed.c --
 *
 *	This file contains platform-specific procedures for theMac to provide
 *	basic operations needed for application embedding (where one
 *	application can use as its main window an internal window from
 *	some other application).
 *	Currently only Toplevel embedding within the same Tk application is
 *      allowed on the Macintosh.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkPort.h"
#include "X.h"
#include "Xlib.h"
#include <stdio.h>

#include <Windows.h>
#include <QDOffscreen.h>
#include "tkMacInt.h"

/*
 * One of the following structures exists for each container in this
 * application.  It keeps track of the container window and its
 * associated embedded window.
 */

typedef struct Container {
    Window parent;		/* The Mac Drawable for the parent of
				 * the pair (the container). */
    TkWindow *parentPtr;	/* Tk's information about the container,
				 * or NULL if the container isn't
				 * in this process. */
    Window embedded;		/* The MacDrawable for the embedded
				 * window.  Starts off as None, but
				 * gets filled in when the window is
				 * eventually created. */
    TkWindow *embeddedPtr;	/* Tk's information about the embedded
				 * window, or NULL if the
				 * embedded application isn't in
				 * this process. */
    struct Container *nextPtr;	/* Next in list of all containers in
				 * this process. */
} Container;

static Container *firstContainerPtr = NULL;
					/* First in list of all containers
					 * managed by this process.  */
/*
 * Globals defined in this file
 */

TkMacEmbedHandler *gMacEmbedHandler = NULL;

/*
 * Prototypes for static procedures defined in this file:
 */

static void		ContainerEventProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static void		EmbeddedEventProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static void		EmbedActivateProc _ANSI_ARGS_((ClientData clientData, 
                            XEvent *eventPtr));
static void		EmbedFocusProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		EmbedGeometryRequest _ANSI_ARGS_((
			    Container * containerPtr, int width, int height));
static void		EmbedSendConfigure _ANSI_ARGS_((
			    Container *containerPtr));
static void		EmbedStructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		EmbedWindowDeleted _ANSI_ARGS_((TkWindow *winPtr));


/*
 *----------------------------------------------------------------------
 *
 * Tk_MacSetEmbedHandler --
 *
 *	Registers a handler for an in process form of embedding, like 
 *	Netscape plugins, where Tk is loaded into the process, but does
 *	not control the main window
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The embed handler is set.
 *
 *----------------------------------------------------------------------
 */
void
Tk_MacSetEmbedHandler(
    Tk_MacEmbedRegisterWinProc *registerWinProc,
    Tk_MacEmbedGetGrafPortProc *getPortProc,
    Tk_MacEmbedMakeContainerExistProc *containerExistProc,
    Tk_MacEmbedGetClipProc *getClipProc,
    Tk_MacEmbedGetOffsetInParentProc *getOffsetProc)
{
    if (gMacEmbedHandler == NULL) {
    	gMacEmbedHandler = (TkMacEmbedHandler *) ckalloc(sizeof(TkMacEmbedHandler));
    }
    gMacEmbedHandler->registerWinProc = registerWinProc;
    gMacEmbedHandler->getPortProc = getPortProc;
    gMacEmbedHandler->containerExistProc = containerExistProc;
    gMacEmbedHandler->getClipProc = getClipProc;
    gMacEmbedHandler->getOffsetProc = getOffsetProc;    
}


/*
 *----------------------------------------------------------------------
 *
 * TkpMakeWindow --
 *
 *	Creates an X Window (Mac subwindow).
 *
 * Results:
 *	The window id is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Window
TkpMakeWindow(
    TkWindow *winPtr,
    Window parent)
{
    MacDrawable *macWin;
    XEvent event;

    /*
     * If this window is marked as embedded then
     * the window structure should have already been
     * created in the TkpUseWindow function.
     */
    
    if (Tk_IsEmbedded(winPtr)) {
	return (Window) winPtr->privatePtr;
    }
    
    /*
     * Allocate sub window
     */
    
    macWin = (MacDrawable *) ckalloc(sizeof(MacDrawable));
    if (macWin == NULL) {
	winPtr->privatePtr = NULL;
	return None;
    }
    macWin->winPtr = winPtr;
    winPtr->privatePtr = macWin;
    macWin->clipRgn = NewRgn();
    macWin->aboveClipRgn = NewRgn();
    macWin->referenceCount = 0;
    macWin->flags = TK_CLIP_INVALID;

    if (Tk_IsTopLevel(macWin->winPtr)) {
	
	/*
	 *This will be set when we are mapped.
	 */
	
	macWin->portPtr = (GWorldPtr) NULL;  
	macWin->toplevel = macWin;
	macWin->xOff = 0;
	macWin->yOff = 0;
    } else {
	macWin->portPtr = NULL;
	macWin->xOff = winPtr->parentPtr->privatePtr->xOff +
	    winPtr->parentPtr->changes.border_width +
	    winPtr->changes.x;
	macWin->yOff = winPtr->parentPtr->privatePtr->yOff +
	    winPtr->parentPtr->changes.border_width +
	    winPtr->changes.y;
	macWin->toplevel = winPtr->parentPtr->privatePtr->toplevel;
    }

    macWin->toplevel->referenceCount++;
    
    /* 
     * TODO: need general solution for visibility events.
     */
    event.xany.serial = Tk_Display(winPtr)->request;
    event.xany.send_event = False;
    event.xany.display = Tk_Display(winPtr);
	
    event.xvisibility.type = VisibilityNotify;
    event.xvisibility.window = (Window) macWin;;
    event.xvisibility.state = VisibilityUnobscured;
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

    return (Window) macWin;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpUseWindow --
 *
 *	This procedure causes a Tk window to use a given X window as
 *	its parent window, rather than the root window for the screen.
 *	It is invoked by an embedded application to specify the window
 *	in which it is embedded.
 *
 * Results:
 *	The return value is normally TCL_OK.  If an error occurs (such
 *	as string not being a valid window spec), then the return value
 *	is TCL_ERROR and an error message is left in the interp's result if
 *	interp is non-NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpUseWindow(
    Tcl_Interp *interp,		/* If not NULL, used for error reporting
				 * if string is bogus. */
    Tk_Window tkwin,		/* Tk window that does not yet have an
				 * associated X window. */
    char *string)		/* String identifying an X window to use
				 * for tkwin;  must be an integer value. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    MacDrawable *parent, *macWin;
    Container *containerPtr;
    XEvent event;
    int result;

    if (winPtr->window != None) {
	panic("TkpUseWindow: X window already assigned");
    }
    
    /*
     * Decode the container pointer, and look for it among the 
     *list of available containers.
     *
     * N.B. For now, we are limiting the containers to be in the same Tk
     * application as tkwin, since otherwise they would not be in our list
     * of containers.
     *
     */
     
    if (Tcl_GetInt(interp, string, &result) != TCL_OK) {
	return TCL_ERROR;
    }

    parent = (MacDrawable *) result;

    /*
     * Save information about the container and the embedded window
     * in a Container structure.  Currently, there must already be an existing
     * Container structure, since we only allow the case where both container 
     * and embedded app. are in the same process.
     */

    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->parent == (Window) parent) {
	    winPtr->flags |= TK_BOTH_HALVES;
	    containerPtr->parentPtr->flags |= TK_BOTH_HALVES;
	    break;
	}
    }
    
    /*
     * Make the embedded window.  
     */

    macWin = (MacDrawable *) ckalloc(sizeof(MacDrawable));
    if (macWin == NULL) {
	winPtr->privatePtr = NULL;
	return TCL_ERROR;
    }
    
    macWin->winPtr = winPtr;
    winPtr->privatePtr = macWin;

    /*
     * The portPtr will be NULL for a Tk in Tk embedded window.
     * It is none of our business what it is for a Tk not in Tk embedded window,
     * but we will initialize it to NULL, and let the registerWinProc 
     * set it.  In any case, you must always use TkMacGetDrawablePort 
     * to get the portPtr.  It will correctly find the container's port.
     */

    macWin->portPtr = (GWorldPtr) NULL;

    macWin->clipRgn = NewRgn();
    macWin->aboveClipRgn = NewRgn();
    macWin->referenceCount = 0;
    macWin->flags = TK_CLIP_INVALID;
    macWin->toplevel = macWin;
    macWin->toplevel->referenceCount++;
   
    winPtr->flags |= TK_EMBEDDED;
    
    
    /*
     * Make a copy of the TK_EMBEDDED flag, since sometimes
     * we need this to get the port after the TkWindow structure
     * has been freed.
     */
     
    macWin->flags |= TK_EMBEDDED;
    
    /*
     * Now check whether it is embedded in another Tk widget.  If not (the first
     * case below) we see if there is an in-process embedding handler registered,
     * and if so, let that fill in the rest of the macWin.
     */
    
    if (containerPtr == NULL) {
	/*
	 * If someone has registered an in process embedding handler, then 
	 * see if it can handle this window...
	 */
	
	if (gMacEmbedHandler == NULL ||
		gMacEmbedHandler->registerWinProc(result, (Tk_Window) winPtr) != TCL_OK) {
	    Tcl_AppendResult(interp, "The window ID ", string,
	            " does not correspond to a valid Tk Window.",
		     (char *) NULL);
	    return TCL_ERROR;	
	} else {
	    containerPtr = (Container *) ckalloc(sizeof(Container));

	    containerPtr->parentPtr = NULL;
	    containerPtr->embedded = (Window) macWin;
	    containerPtr->embeddedPtr = macWin->winPtr;
	    containerPtr->nextPtr = firstContainerPtr;
	    firstContainerPtr = containerPtr;
	    
	}    
    } else {
        
	/* 
         * The window is embedded in another Tk window.
         */ 
	
	macWin->xOff = parent->winPtr->privatePtr->xOff +
	        parent->winPtr->changes.border_width +
	        winPtr->changes.x;
	macWin->yOff = parent->winPtr->privatePtr->yOff +
	        parent->winPtr->changes.border_width +
	        winPtr->changes.y;
    
    
        /*
         * Finish filling up the container structure with the embedded window's 
         * information.
         */
     
	containerPtr->embedded = (Window) macWin;
	containerPtr->embeddedPtr = macWin->winPtr;

	/*
         * Create an event handler to clean up the Container structure when
         * tkwin is eventually deleted.
         */

        Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbeddedEventProc,
	        (ClientData) winPtr);

    }

   /* 
     * TODO: need general solution for visibility events.
     */
     
    event.xany.serial = Tk_Display(winPtr)->request;
    event.xany.send_event = False;
    event.xany.display = Tk_Display(winPtr);
	
    event.xvisibility.type = VisibilityNotify;
    event.xvisibility.window = (Window) macWin;;
    event.xvisibility.state = VisibilityUnobscured;
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

    
    /* 
     * TODO: need general solution for visibility events.
     */
     
    event.xany.serial = Tk_Display(winPtr)->request;
    event.xany.send_event = False;
    event.xany.display = Tk_Display(winPtr);
	
    event.xvisibility.type = VisibilityNotify;
    event.xvisibility.window = (Window) macWin;;
    event.xvisibility.state = VisibilityUnobscured;
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
     
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeContainer --
 *
 *	This procedure is called to indicate that a particular window
 *	will be a container for an embedded application.  This changes
 *	certain aspects of the window's behavior, such as whether it
 *	will receive events anymore.
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
TkpMakeContainer(
    Tk_Window tkwin)		/* Token for a window that is about to
				 * become a container. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Container *containerPtr;

    /*
     * Register the window as a container so that, for example, we can
     * make sure the argument to -use is valid.
     */


    Tk_MakeWindowExist(tkwin);
    containerPtr = (Container *) ckalloc(sizeof(Container));
    containerPtr->parent = Tk_WindowId(tkwin);
    containerPtr->parentPtr = winPtr;
    containerPtr->embedded = None;
    containerPtr->embeddedPtr = NULL;
    containerPtr->nextPtr = firstContainerPtr;
    firstContainerPtr = containerPtr;
    winPtr->flags |= TK_CONTAINER;
    
    /*
     * Request SubstructureNotify events so that we can find out when
     * the embedded application creates its window or attempts to
     * resize it.  Also watch Configure events on the container so that
     * we can resize the child to match.  Also, pass activate events from
     * the container down to the embedded toplevel.
     */

    Tk_CreateEventHandler(tkwin,
	    SubstructureNotifyMask|SubstructureRedirectMask,
	    ContainerEventProc, (ClientData) winPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbedStructureProc,
	    (ClientData) containerPtr);
    Tk_CreateEventHandler(tkwin, ActivateMask, EmbedActivateProc,
	    (ClientData) containerPtr);
    Tk_CreateEventHandler(tkwin, FocusChangeMask, EmbedFocusProc,
	    (ClientData) containerPtr);
 	
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacContainerId --
 *
 *	Given an embedded window, this procedure returns the MacDrawable
 *	identifier for the associated container window.
 *
 * Results:
 *	The return value is the MacDrawable for winPtr's
 *	container window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MacDrawable *
TkMacContainerId(winPtr)
    TkWindow *winPtr;		/* Tk's structure for an embedded window. */
{
    Container *containerPtr;

    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr == winPtr) {
	    return (MacDrawable *) containerPtr->parent;
	}
    }
    panic("TkMacContainerId couldn't find window");
    return None;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacGetHostToplevel --
 *
 *	Given the TkWindow, return the MacDrawable for the outermost
 *      toplevel containing it.  This will be a real Macintosh window.
 *
 * Results:
 *	Returns a MacDrawable corresponding to a Macintosh Toplevel
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MacDrawable *
TkMacGetHostToplevel(
    TkWindow *winPtr)		/* Tk's structure for a window. */
{
    TkWindow *contWinPtr, *topWinPtr;

    topWinPtr = winPtr->privatePtr->toplevel->winPtr;
    if (!Tk_IsEmbedded(topWinPtr)) {
    	return winPtr->privatePtr->toplevel;
    } else {
	contWinPtr = TkpGetOtherWindow(topWinPtr);

	/*
	 * NOTE: Here we should handle out of process embedding.
	 */
	
	if (contWinPtr != NULL) {
	    return TkMacGetHostToplevel(contWinPtr);
	} else {
	    return None;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpClaimFocus --
 *
 *	This procedure is invoked when someone asks for the input focus
 *	to be put on a window in an embedded application, but the
 *	application doesn't currently have the focus.  It requests the
 *	input focus from the container application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The input focus may change.
 *
 *----------------------------------------------------------------------
 */

void
TkpClaimFocus(
    TkWindow *topLevelPtr,		/* Top-level window containing desired
					 * focus window; should be embedded. */
    int force)				/* One means that the container should
					 * claim the focus if it doesn't
					 * currently have it. */
{
    XEvent event;
    Container *containerPtr;

    if (!(topLevelPtr->flags & TK_EMBEDDED)) {
	return;
    }

    for (containerPtr = firstContainerPtr;
	    containerPtr->embeddedPtr != topLevelPtr;
	    containerPtr = containerPtr->nextPtr) {
	/* Empty loop body. */
    }
    

    event.xfocus.type = FocusIn;
    event.xfocus.serial = LastKnownRequestProcessed(topLevelPtr->display);
    event.xfocus.send_event = 1;
    event.xfocus.display = topLevelPtr->display;
    event.xfocus.window = containerPtr->parent;
    event.xfocus.mode = EMBEDDED_APP_WANTS_FOCUS;
    event.xfocus.detail = force;
    Tk_QueueWindowEvent(&event,TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpTestembedCmd --
 *
 *	This procedure implements the "testembed" command.  It returns
 *	some or all of the information in the list pointed to by
 *	firstContainerPtr.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpTestembedCmd(
    ClientData clientData,		/* Main window for application. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int argc,				/* Number of arguments. */
    char **argv)			/* Argument strings. */
{
    int all;
    Container *containerPtr;
    Tcl_DString dString;
    char buffer[50];

    if ((argc > 1) && (strcmp(argv[1], "all") == 0)) {
	all = 1;
    } else {
	all = 0;
    }
    Tcl_DStringInit(&dString);
    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	Tcl_DStringStartSublist(&dString);
	if (containerPtr->parent == None) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    if (all) {
		sprintf(buffer, "0x%x", (int) containerPtr->parent);
		Tcl_DStringAppendElement(&dString, buffer);
	    } else {
		Tcl_DStringAppendElement(&dString, "XXX");
	    }
	}
	if (containerPtr->parentPtr == NULL) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    Tcl_DStringAppendElement(&dString,
		    containerPtr->parentPtr->pathName);
	}
	if (containerPtr->embedded == None) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    if (all) {
		sprintf(buffer, "0x%x", (int) containerPtr->embedded);
		Tcl_DStringAppendElement(&dString, buffer);
	    } else {
		Tcl_DStringAppendElement(&dString, "XXX");
	    }
	}
	if (containerPtr->embeddedPtr == NULL) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    Tcl_DStringAppendElement(&dString,
		    containerPtr->embeddedPtr->pathName);
	}
	Tcl_DStringEndSublist(&dString);
    }
    Tcl_DStringResult(interp, &dString);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpRedirectKeyEvent --
 *
 *	This procedure is invoked when a key press or release event
 *	arrives for an application that does not believe it owns the
 *	input focus.  This can happen because of embedding; for example,
 *	X can send an event to an embedded application when the real
 *	focus window is in the container application and is an ancestor
 *	of the container.  This procedure's job is to forward the event
 *	back to the application where it really belongs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The event may get sent to a different application.
 *
 *----------------------------------------------------------------------
 */

void
TkpRedirectKeyEvent(
    TkWindow *winPtr,		/* Window to which the event was originally
				 * reported. */
    XEvent *eventPtr)		/* X event to redirect (should be KeyPress
				 * or KeyRelease). */
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetOtherWindow --
 *
 *	If both the container and embedded window are in the same
 *	process, this procedure will return either one, given the other.
 *
 * Results:
 *	If winPtr is a container, the return value is the token for the
 *	embedded window, and vice versa.  If the "other" window isn't in
 *	this process, NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkpGetOtherWindow(
    TkWindow *winPtr)		/* Tk's structure for a container or
				 * embedded window. */
{
    Container *containerPtr;

    /*
     * TkpGetOtherWindow returns NULL if both windows are not
     * in the same process...
     */

    if (!(winPtr->flags & TK_BOTH_HALVES)) {
	return NULL;
    }
    
    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr == winPtr) {
	    return containerPtr->parentPtr;
	} else if (containerPtr->parentPtr == winPtr) {
	    return containerPtr->embeddedPtr;
	}
    }
    return NULL;
}
/*
 *----------------------------------------------------------------------
 *
 * EmbeddedEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when various
 *	useful events are received for a window that is embedded in
 *	another application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Our internal state gets cleaned up when an embedded window is
 *	destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
EmbeddedEventProc(clientData, eventPtr)
    ClientData clientData;		/* Token for container window. */
    XEvent *eventPtr;			/* ResizeRequest event. */
{
    TkWindow *winPtr = (TkWindow *) clientData;

    if (eventPtr->type == DestroyNotify) {
	EmbedWindowDeleted(winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ContainerEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when various
 *	useful events are received for the children of a container
 *	window.  It forwards relevant information, such as geometry
 *	requests, from the events into the container's application.
 *
 *	NOTE: on the Mac, only the DestroyNotify branch is ever taken.
 *      We don't synthesize the other events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the event.  For example, when ConfigureRequest events
 *	occur, geometry information gets set for the container window.
 *
 *----------------------------------------------------------------------
 */

static void
ContainerEventProc(clientData, eventPtr)
    ClientData clientData;		/* Token for container window. */
    XEvent *eventPtr;			/* ResizeRequest event. */
{
    TkWindow *winPtr = (TkWindow *) clientData;
    Container *containerPtr;
    Tk_ErrorHandler errHandler;

    /*
     * Ignore any X protocol errors that happen in this procedure
     * (almost any operation could fail, for example, if the embedded
     * application has deleted its window).
     */

    errHandler = Tk_CreateErrorHandler(eventPtr->xfocus.display, -1,
	    -1, -1, (Tk_ErrorProc *) NULL, (ClientData) NULL);

    /*
     * Find the Container structure associated with the parent window.
     */

    for (containerPtr = firstContainerPtr;
	    containerPtr->parent != eventPtr->xmaprequest.parent;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr == NULL) {
	    panic("ContainerEventProc couldn't find Container record");
	}
    }

    if (eventPtr->type == CreateNotify) {
	/*
	 * A new child window has been created in the container. Record
	 * its id in the Container structure (if more than one child is
	 * created, just remember the last one and ignore the earlier
	 * ones).
	 */

	containerPtr->embedded = eventPtr->xcreatewindow.window;
    } else if (eventPtr->type == ConfigureRequest) {
	if ((eventPtr->xconfigurerequest.x != 0)
		|| (eventPtr->xconfigurerequest.y != 0)) {
	    /*
	     * The embedded application is trying to move itself, which
	     * isn't legal.  At this point, the window hasn't actually
	     * moved, but we need to send it a ConfigureNotify event to
	     * let it know that its request has been denied.  If the
	     * embedded application was also trying to resize itself, a
	     * ConfigureNotify will be sent by the geometry management
	     * code below, so we don't need to do anything.  Otherwise,
	     * generate a synthetic event.
	     */

	    if ((eventPtr->xconfigurerequest.width == winPtr->changes.width)
		    && (eventPtr->xconfigurerequest.height
		    == winPtr->changes.height)) {
		EmbedSendConfigure(containerPtr);
	    }
	}
	EmbedGeometryRequest(containerPtr,
		eventPtr->xconfigurerequest.width,
		eventPtr->xconfigurerequest.height);
    } else if (eventPtr->type == MapRequest) {
	/*
	 * The embedded application's map request was ignored and simply
	 * passed on to us, so we have to map the window for it to appear
	 * on the screen.
	 */

	XMapWindow(eventPtr->xmaprequest.display,
		eventPtr->xmaprequest.window);
    } else if (eventPtr->type == DestroyNotify) {
	/*
	 * The embedded application is gone.  Destroy the container window.
	 */

	Tk_DestroyWindow((Tk_Window) winPtr);
    }
    Tk_DeleteErrorHandler(errHandler);
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedStructureProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when
 *	a container window owned by this application gets resized
 *	(and also at several other times that we don't care about).
 *	This procedure reflects the size change in the embedded
 *	window that corresponds to the container.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The embedded window gets resized to match the container.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedStructureProc(clientData, eventPtr)
    ClientData clientData;		/* Token for container window. */
    XEvent *eventPtr;			/* ResizeRequest event. */
{
    Container *containerPtr = (Container *) clientData;
    Tk_ErrorHandler errHandler;

    if (eventPtr->type == ConfigureNotify) {
	if (containerPtr->embedded != None) {
	    /*
	     * Ignore errors, since the embedded application could have
	     * deleted its window.
	     */

	    errHandler = Tk_CreateErrorHandler(eventPtr->xfocus.display, -1,
		    -1, -1, (Tk_ErrorProc *) NULL, (ClientData) NULL);
	    Tk_MoveResizeWindow((Tk_Window) containerPtr->embeddedPtr, 0, 0,
		    (unsigned int) Tk_Width(
			    (Tk_Window) containerPtr->parentPtr),
		    (unsigned int) Tk_Height(
			    (Tk_Window) containerPtr->parentPtr));
	    Tk_DeleteErrorHandler(errHandler);
	}
    } else if (eventPtr->type == DestroyNotify) {
	EmbedWindowDeleted(containerPtr->parentPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedActivateProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when
 *	Activate and Deactivate events occur for a container window owned
 *	by this application.  It is responsible for forwarding an activate 
 *      event down into the embedded toplevel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The X focus may change.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedActivateProc(clientData, eventPtr)
    ClientData clientData;		/* Token for container window. */
    XEvent *eventPtr;			/* ResizeRequest event. */
{
    Container *containerPtr = (Container *) clientData;
    
    if (containerPtr->embeddedPtr != NULL) {
        if (eventPtr->type == ActivateNotify) {
            TkGenerateActivateEvents(containerPtr->embeddedPtr,1);
        } else if (eventPtr->type == DeactivateNotify) {
            TkGenerateActivateEvents(containerPtr->embeddedPtr,0);
        }        
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedFocusProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when
 *	FocusIn and FocusOut events occur for a container window owned
 *	by this application.  It is responsible for moving the focus
 *	back and forth between a container application and an embedded
 *	application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The X focus may change.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedFocusProc(clientData, eventPtr)
    ClientData clientData;		/* Token for container window. */
    XEvent *eventPtr;			/* ResizeRequest event. */
{
    Container *containerPtr = (Container *) clientData;
    Display *display;
    XEvent event;

    if (containerPtr->embeddedPtr != NULL) {
    display = Tk_Display(containerPtr->parentPtr);
        event.xfocus.serial = LastKnownRequestProcessed(display);
        event.xfocus.send_event = false;
        event.xfocus.display = display;
        event.xfocus.mode = NotifyNormal;
        event.xfocus.window = containerPtr->embedded; 
        
    if (eventPtr->type == FocusIn) {
	/*
	 * The focus just arrived at the container.  Change the X focus
	 * to move it to the embedded application, if there is one. 
	 * Ignore X errors that occur during this operation (it's
	 * possible that the new focus window isn't mapped).
	 */
    
            event.xfocus.detail = NotifyNonlinear;
            event.xfocus.type = FocusIn;

        } else if (eventPtr->type == FocusOut) {
        /* When the container gets a FocusOut event, it has to  tell the embedded app
         * that it has lost the focus.
         */
         
            event.xfocus.type = FocusOut;
            event.xfocus.detail = NotifyNonlinear;
         }
         
        Tk_QueueWindowEvent(&event, TCL_QUEUE_MARK);
    } 
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedGeometryRequest --
 *
 *	This procedure is invoked when an embedded application requests
 *	a particular size.  It processes the request (which may or may
 *	not actually honor the request) and reflects the results back
 *	to the embedded application.
 *
 *      NOTE: On the Mac, this is a stub, since we don't synthesize
 *      ConfigureRequest events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If we deny the child's size change request, a Configure event
 *	is synthesized to let the child know how big it ought to be.
 *	Events get processed while we're waiting for the geometry
 *	managers to do their thing.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedGeometryRequest(containerPtr, width, height)
    Container *containerPtr;	/* Information about the embedding. */
    int width, height;		/* Size that the child has requested. */
{
    TkWindow *winPtr = containerPtr->parentPtr;

    /*
     * Forward the requested size into our geometry management hierarchy
     * via the container window.  We need to send a Configure event back
     * to the embedded application if we decide not to honor its
     * request; to make this happen, process all idle event handlers
     * synchronously here (so that the geometry managers have had a
     * chance to do whatever they want to do), and if the window's size
     * didn't change then generate a configure event.
     */

    Tk_GeometryRequest((Tk_Window) winPtr, width, height);
    while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)) {
	/* Empty loop body. */
    }
    if ((winPtr->changes.width != width)
	    || (winPtr->changes.height != height)) {
	EmbedSendConfigure(containerPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedSendConfigure --
 *
 *	This is currently a stub.  It is called to notify an
 *	embedded application of its current size and location.  This
 *	procedure is called when the embedded application made a
 *	geometry request that we did not grant, so that the embedded
 *	application knows that its geometry didn't change after all.
 *      It is a response to ConfigureRequest events, which we do not
 *      currently synthesize on the Mac
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
EmbedSendConfigure(containerPtr)
    Container *containerPtr;	/* Information about the embedding. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedWindowDeleted --
 *
 *	This procedure is invoked when a window involved in embedding
 *	(as either the container or the embedded application) is
 *	destroyed.  It cleans up the Container structure for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Container structure may be freed.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedWindowDeleted(winPtr)
    TkWindow *winPtr;		/* Tk's information about window that
				 * was deleted. */
{
    Container *containerPtr, *prevPtr;

    /*
     * Find the Container structure for this window.  Delete the
     * information about the embedded application and free the container's
     * record.
     */

    prevPtr = NULL;
    containerPtr = firstContainerPtr;
    while (1) {
	if (containerPtr->embeddedPtr == winPtr) {

	    /*
	     * We also have to destroy our parent, to clean up the container.
	     * Fabricate an event to do this.
	     */
	     
	    if (containerPtr->parentPtr != NULL &&
		    containerPtr->parentPtr->flags & TK_BOTH_HALVES) {
		XEvent event;
		
	        event.xany.serial = 
	            Tk_Display(containerPtr->parentPtr)->request;
    		event.xany.send_event = False;
    		event.xany.display = Tk_Display(containerPtr->parentPtr);
	
    		event.xany.type = DestroyNotify;
    		event.xany.window = containerPtr->parent;
    		event.xdestroywindow.event = containerPtr->parent;
    		Tk_QueueWindowEvent(&event, TCL_QUEUE_HEAD);

	    }
	    
	    containerPtr->embedded = None;
	    containerPtr->embeddedPtr = NULL;
	    
	    break;
	}
	if (containerPtr->parentPtr == winPtr) {
	    containerPtr->parentPtr = NULL;
	    break;
	}
	prevPtr = containerPtr;
	containerPtr = containerPtr->nextPtr;
    }
    if ((containerPtr->embeddedPtr == NULL)
	    && (containerPtr->parentPtr == NULL)) {
	if (prevPtr == NULL) {
	    firstContainerPtr = containerPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = containerPtr->nextPtr;
	}
	ckfree((char *) containerPtr);
    }
}

