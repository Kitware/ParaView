/* 
 * tkUnixEmbed.c --
 *
 *	This file contains platform-specific procedures for UNIX to provide
 *	basic operations needed for application embedding (where one
 *	application can use as its main window an internal window from
 *	some other application).
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkUnixInt.h"

/*
 * One of the following structures exists for each container in this
 * application.  It keeps track of the container window and its
 * associated embedded window.
 */

typedef struct Container {
    Window parent;			/* X's window id for the parent of
					 * the pair (the container). */
    Window parentRoot;			/* Id for the root window of parent's
					 * screen. */
    TkWindow *parentPtr;		/* Tk's information about the container,
					 * or NULL if the container isn't
					 * in this process. */
    Window wrapper;			/* X's window id for the wrapper
					 * window for the embedded window.
					 * Starts off as None, but gets
					 * filled in when the window is
					 * eventually created. */
    TkWindow *embeddedPtr;		/* Tk's information about the embedded
					 * window, or NULL if the embedded
					 * application isn't in this process.
					 * Note that this is *not* the
					 * same window as wrapper: wrapper is
					 * the parent of embeddedPtr. */
    struct Container *nextPtr;		/* Next in list of all containers in
					 * this process. */
} Container;

typedef struct ThreadSpecificData {
    Container *firstContainerPtr;       /* First in list of all containers
					 * managed by this process.  */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Prototypes for static procedures defined in this file:
 */

static void		ContainerEventProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static void		EmbeddedEventProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static int		EmbedErrorProc _ANSI_ARGS_((ClientData clientData,
			    XErrorEvent *errEventPtr));
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
 *	Changes the colormap and other visual information to match that
 *	of the parent window given by "string".
 *
 *----------------------------------------------------------------------
 */

int
TkpUseWindow(interp, tkwin, string)
    Tcl_Interp *interp;		/* If not NULL, used for error reporting
				 * if string is bogus. */
    Tk_Window tkwin;		/* Tk window that does not yet have an
				 * associated X window. */
    char *string;		/* String identifying an X window to use
				 * for tkwin;  must be an integer value. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    int id, anyError;
    Window parent;
    Tk_ErrorHandler handler;
    Container *containerPtr;
    XWindowAttributes parentAtts;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (winPtr->window != None) {
	panic("TkUseWindow: X window already assigned");
    }
    if (Tcl_GetInt(interp, string, &id) != TCL_OK) {
	return TCL_ERROR;
    }
    parent = (Window) id;

    /*
     * Tk sets the window colormap to the screen default colormap in
     * tkWindow.c:AllocWindow. This doesn't work well for embedded
     * windows. So we override the colormap and visual settings to be
     * the same as the parent window (which is in the container app).
     */

    anyError = 0;
    handler = Tk_CreateErrorHandler(winPtr->display, -1, -1, -1,
	    EmbedErrorProc, (ClientData) &anyError);
    if (!XGetWindowAttributes(winPtr->display, parent, &parentAtts)) {
        anyError =  1;
    }
    XSync(winPtr->display, False);
    Tk_DeleteErrorHandler(handler);
    if (anyError) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "couldn't create child of window \"",
		    string, "\"", (char *) NULL);
	}
	return TCL_ERROR;
    }
    Tk_SetWindowVisual(tkwin, parentAtts.visual, parentAtts.depth,
	    parentAtts.colormap);

    /*
     * Create an event handler to clean up the Container structure when
     * tkwin is eventually deleted.
     */

    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbeddedEventProc,
	    (ClientData) winPtr);

    /*
     * Save information about the container and the embedded window
     * in a Container structure.  If there is already an existing
     * Container structure, it means that both container and embedded
     * app. are in the same process.
     */

    for (containerPtr = tsdPtr->firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->parent == parent) {
	    winPtr->flags |= TK_BOTH_HALVES;
	    containerPtr->parentPtr->flags |= TK_BOTH_HALVES;
	    break;
	}
    }
    if (containerPtr == NULL) {
	containerPtr = (Container *) ckalloc(sizeof(Container));
	containerPtr->parent = parent;
	containerPtr->parentRoot = parentAtts.root;
	containerPtr->parentPtr = NULL;
	containerPtr->wrapper = None;
	containerPtr->nextPtr = tsdPtr->firstContainerPtr;
	tsdPtr->firstContainerPtr = containerPtr;
    }
    containerPtr->embeddedPtr = winPtr;
    winPtr->flags |= TK_EMBEDDED;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeWindow --
 *
 *	Create an actual window system window object based on the
 *	current attributes of the specified TkWindow.
 *
 * Results:
 *	Returns the handle to the new window, or None on failure.
 *
 * Side effects:
 *	Creates a new X window.
 *
 *----------------------------------------------------------------------
 */

Window
TkpMakeWindow(winPtr, parent)
    TkWindow *winPtr;		/* Tk's information about the window that
				 * is to be instantiated. */
    Window parent;		/* Window system token for the parent in
				 * which the window is to be created. */
{
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (winPtr->flags & TK_EMBEDDED) {
	/*
	 * This window is embedded.  Don't create the new window in the
	 * given parent; instead, create it as a child of the root window
	 * of the container's screen.  The window will get reparented
	 * into a wrapper window later.
	 */

	for (containerPtr = tsdPtr->firstContainerPtr; ;
		containerPtr = containerPtr->nextPtr) {
	    if (containerPtr == NULL) {
		panic("TkMakeWindow couldn't find container for window");
	    }
	    if (containerPtr->embeddedPtr == winPtr) {
		break;
	    }
	}
	parent = containerPtr->parentRoot;
    }

    return XCreateWindow(winPtr->display, parent, winPtr->changes.x,
	    winPtr->changes.y, (unsigned) winPtr->changes.width,
	    (unsigned) winPtr->changes.height,
	    (unsigned) winPtr->changes.border_width, winPtr->depth,
	    InputOutput, winPtr->visual, winPtr->dirtyAtts,
	    &winPtr->atts);
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
TkpMakeContainer(tkwin)
    Tk_Window tkwin;		/* Token for a window that is about to
				 * become a container. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Register the window as a container so that, for example, we can
     * find out later if the embedded app. is in the same process.
     */

    Tk_MakeWindowExist(tkwin);
    containerPtr = (Container *) ckalloc(sizeof(Container));
    containerPtr->parent = Tk_WindowId(tkwin);
    containerPtr->parentRoot = RootWindowOfScreen(Tk_Screen(tkwin));
    containerPtr->parentPtr = winPtr;
    containerPtr->wrapper = None;
    containerPtr->embeddedPtr = NULL;
    containerPtr->nextPtr = tsdPtr->firstContainerPtr;
    tsdPtr->firstContainerPtr = containerPtr;
    winPtr->flags |= TK_CONTAINER;

    /*
     * Request SubstructureNotify events so that we can find out when
     * the embedded application creates its window or attempts to
     * resize it.  Also watch Configure events on the container so that
     * we can resize the child to match.
     */

    winPtr->atts.event_mask |= SubstructureRedirectMask|SubstructureNotifyMask;
    XSelectInput(winPtr->display, winPtr->window, winPtr->atts.event_mask);
    Tk_CreateEventHandler(tkwin,
	    SubstructureNotifyMask|SubstructureRedirectMask,
	    ContainerEventProc, (ClientData) winPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbedStructureProc,
	    (ClientData) containerPtr);
    Tk_CreateEventHandler(tkwin, FocusChangeMask, EmbedFocusProc,
	    (ClientData) containerPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedErrorProc --
 *
 *	This procedure is invoked if an error occurs while creating
 *	an embedded window.
 *
 * Results:
 *	Always returns 0 to indicate that the error has been properly
 *	handled.
 *
 * Side effects:
 *	The integer pointed to by the clientData argument is set to 1.
 *
 *----------------------------------------------------------------------
 */

static int
EmbedErrorProc(clientData, errEventPtr)
    ClientData clientData;	/* Points to integer to set. */
    XErrorEvent *errEventPtr;	/* Points to information about error
				 * (not used). */
{
    int *iPtr = (int *) clientData;

    *iPtr = 1;
    return 0;
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
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

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

    for (containerPtr = tsdPtr->firstContainerPtr;
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
	 * ones).  Also set the child's size to match the container.
	 */

	containerPtr->wrapper = eventPtr->xcreatewindow.window;
	XMoveResizeWindow(eventPtr->xcreatewindow.display,
		containerPtr->wrapper, 0, 0,
		(unsigned int) Tk_Width(
			(Tk_Window) containerPtr->parentPtr),
		(unsigned int) Tk_Height(
			(Tk_Window) containerPtr->parentPtr));
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
	if (containerPtr->wrapper != None) {
	    /*
	     * Ignore errors, since the embedded application could have
	     * deleted its window.
	     */

	    errHandler = Tk_CreateErrorHandler(eventPtr->xfocus.display, -1,
		    -1, -1, (Tk_ErrorProc *) NULL, (ClientData) NULL);
	    XMoveResizeWindow(eventPtr->xconfigure.display,
		    containerPtr->wrapper, 0, 0,
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
    Tk_ErrorHandler errHandler;
    Display *display;

    display = Tk_Display(containerPtr->parentPtr);
    if (eventPtr->type == FocusIn) {
	/*
	 * The focus just arrived at the container.  Change the X focus
	 * to move it to the embedded application, if there is one. 
	 * Ignore X errors that occur during this operation (it's
	 * possible that the new focus window isn't mapped).
	 */
    
	if (containerPtr->wrapper != None) {
	    errHandler = Tk_CreateErrorHandler(eventPtr->xfocus.display, -1,
		    -1, -1, (Tk_ErrorProc *) NULL, (ClientData) NULL);
	    XSetInputFocus(display, containerPtr->wrapper, RevertToParent,
		    CurrentTime);
	    Tk_DeleteErrorHandler(errHandler);
	}
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
 *	This procedure synthesizes a ConfigureNotify event to notify an
 *	embedded application of its current size and location.  This
 *	procedure is called when the embedded application made a
 *	geometry request that we did not grant, so that the embedded
 *	application knows that its geometry didn't change after all.
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
    TkWindow *winPtr = containerPtr->parentPtr;
    XEvent event;

    event.xconfigure.type = ConfigureNotify;
    event.xconfigure.serial =
	    LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = True;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = containerPtr->wrapper;
    event.xconfigure.window = containerPtr->wrapper;
    event.xconfigure.x = 0;
    event.xconfigure.y = 0;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.above = None;
    event.xconfigure.override_redirect = False;

    /*
     * Note:  when sending the event below, the ButtonPressMask
     * causes the event to be sent only to applications that have
     * selected for ButtonPress events, which should be just the
     * embedded application.
     */

    XSendEvent(winPtr->display, containerPtr->wrapper, False,
	    0, &event);

    /*
     * The following needs to be done if the embedded window is
     * not in the same application as the container window.
     */

    if (containerPtr->embeddedPtr == NULL) {
	XMoveResizeWindow(winPtr->display, containerPtr->wrapper, 0, 0,
	    (unsigned int) winPtr->changes.width,
	    (unsigned int) winPtr->changes.height);
    }
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
TkpGetOtherWindow(winPtr)
    TkWindow *winPtr;		/* Tk's structure for a container or
				 * embedded window. */
{
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (containerPtr = tsdPtr->firstContainerPtr; 
            containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr == winPtr) {
	    return containerPtr->parentPtr;
	} else if (containerPtr->parentPtr == winPtr) {
	    return containerPtr->embeddedPtr;
	}
    }
    panic("TkpGetOtherWindow couldn't find window");
    return NULL;
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
TkpRedirectKeyEvent(winPtr, eventPtr)
    TkWindow *winPtr;		/* Window to which the event was originally
				 * reported. */
    XEvent *eventPtr;		/* X event to redirect (should be KeyPress
				 * or KeyRelease). */
{
    Container *containerPtr;
    Window saved;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * First, find the top-level window corresponding to winPtr.
     */

    while (1) {
	if (winPtr == NULL) {
	    /*
	     * This window is being deleted.  This is too confusing a
	     * case to handle so discard the event.
	     */

	    return;
	}
	if (winPtr->flags & TK_TOP_LEVEL) {
	    break;
	}
	winPtr = winPtr->parentPtr;
    }

    if (winPtr->flags & TK_EMBEDDED) {
	/*
	 * This application is embedded.  If we got a key event without
	 * officially having the focus, it means that the focus is
	 * really in the container, but the mouse was over the embedded
	 * application.  Send the event back to the container.
	 */

	for (containerPtr = tsdPtr->firstContainerPtr;
		containerPtr->embeddedPtr != winPtr;
		containerPtr = containerPtr->nextPtr) {
	    /* Empty loop body. */
	}
	saved = eventPtr->xkey.window;
	eventPtr->xkey.window = containerPtr->parent;
	XSendEvent(eventPtr->xkey.display, eventPtr->xkey.window, False,
	    KeyPressMask|KeyReleaseMask, eventPtr);
	eventPtr->xkey.window = saved;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpClaimFocus --
 *
 *	This procedure is invoked when someone asks or the input focus
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
TkpClaimFocus(topLevelPtr, force)
    TkWindow *topLevelPtr;		/* Top-level window containing desired
					 * focus window; should be embedded. */
    int force;				/* One means that the container should
					 * claim the focus if it doesn't
					 * currently have it. */
{
    XEvent event;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!(topLevelPtr->flags & TK_EMBEDDED)) {
	return;
    }

    for (containerPtr = tsdPtr->firstContainerPtr;
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
    XSendEvent(event.xfocus.display, event.xfocus.window, False, 0, &event);
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
TkpTestembedCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int all;
    Container *containerPtr;
    Tcl_DString dString;
    char buffer[50];
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if ((argc > 1) && (strcmp(argv[1], "all") == 0)) {
	all = 1;
    } else {
	all = 0;
    }
    Tcl_DStringInit(&dString);
    for (containerPtr = tsdPtr->firstContainerPtr; containerPtr != NULL;
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
	if (containerPtr->wrapper == None) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    if (all) {
		sprintf(buffer, "0x%x", (int) containerPtr->wrapper);
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
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Find the Container structure for this window work.  Delete the
     * information about the embedded application and free the container's
     * record.
     */

    prevPtr = NULL;
    containerPtr = tsdPtr->firstContainerPtr;
    while (1) {
	if (containerPtr->embeddedPtr == winPtr) {
	    containerPtr->wrapper = None;
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
	    tsdPtr->firstContainerPtr = containerPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = containerPtr->nextPtr;
	}
	ckfree((char *) containerPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnixContainerId --
 *
 *	Given an embedded window, this procedure returns the X window
 *	identifier for the associated container window.
 *
 * Results:
 *	The return value is the X window identifier for winPtr's
 *	container window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Window
TkUnixContainerId(winPtr)
    TkWindow *winPtr;		/* Tk's structure for an embedded window. */
{
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (containerPtr = tsdPtr->firstContainerPtr; 
            containerPtr != NULL; containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr == winPtr) {
	    return containerPtr->parent;
	}
    }
    panic("TkUnixContainerId couldn't find window");
    return None;
}
