/* 
 * tkWinEmbed.c --
 *
 *	This file contains platform specific procedures for Windows platforms
 *	to provide basic operations needed for application embedding (where
 *	one application can use as its main window an internal window from
 *	another application).
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"


/*
 * One of the following structures exists for each container in this
 * application.  It keeps track of the container window and its
 * associated embedded window.
 */

typedef struct Container {
    HWND parentHWnd;			/* Windows HWND to the parent window */
    TkWindow *parentPtr;		/* Tk's information about the container
					 * or NULL if the container isn't
					 * in this process. */
    HWND embeddedHWnd;			/* Windows HWND to the embedded window
					 */
    TkWindow *embeddedPtr;		/* Tk's information about the embedded
					 * window, or NULL if the
					 * embedded application isn't in
					 * this process. */
    struct Container *nextPtr;		/* Next in list of all containers in
					 * this process. */
} Container;

typedef struct ThreadSpecificData {
    Container *firstContainerPtr;       /* First in list of all containers
					 * managed by this process.  */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

static void		CleanupContainerList _ANSI_ARGS_((
    			    ClientData clientData));
static void		ContainerEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		EmbeddedEventProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static void		EmbedGeometryRequest _ANSI_ARGS_((
    			    Container*containerPtr, int width, int height));
static void		EmbedWindowDeleted _ANSI_ARGS_((TkWindow *winPtr));

/*
 *----------------------------------------------------------------------
 *
 * CleanupContainerList --
 *
 *	Finalizes the list of containers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases memory occupied by containers of embedded windows.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
CleanupContainerList(clientData)
    ClientData clientData;
{
    Container *nextPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    
    for (;
        tsdPtr->firstContainerPtr != (Container *) NULL;
        tsdPtr->firstContainerPtr = nextPtr) {
        nextPtr = tsdPtr->firstContainerPtr->nextPtr;
        ckfree((char *) tsdPtr->firstContainerPtr);
    }
    tsdPtr->firstContainerPtr = (Container *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpTestembedCmd --
 *
 *	Test command for the embedding facility.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	Currently it does not do anything.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkpTestembedCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char **argv;
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpUseWindow --
 *
 *	This procedure causes a Tk window to use a given Windows handle
 *	for a window as its underlying window, rather than a new Windows
 *	window being created automatically. It is invoked by an embedded
 *	application to specify the window in which the application is
 *	embedded.
 *
 * Results:
 *	The return value is normally TCL_OK. If an error occurred (such as
 *	if the argument does not identify a legal Windows window handle),
 *	the return value is TCL_ERROR and an error message is left in the
 *	the interp's result if interp is not NULL.
 *
 * Side effects:
 *	None.
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
    int id;
    HWND hwnd;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (winPtr->window != None) {
        panic("TkpUseWindow: Already assigned a window");
    }

    if (Tcl_GetInt(interp, string, &id) != TCL_OK) {
        return TCL_ERROR;
    }
    hwnd = (HWND) id;

    /*
     * Check if the window is a valid handle. If it is invalid, return
     * TCL_ERROR and potentially leave an error message in the interp's
     * result.
     */

    if (!IsWindow(hwnd)) {
        if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp, "window \"", string,
                    "\" doesn't exist", (char *) NULL);
        }
        return TCL_ERROR;
    }

    /*
     * Store the parent window in the platform private data slot so
     * TkWmMapWindow can use it when creating the wrapper window.
     */

    winPtr->privatePtr = (struct TkWindowPrivate*) hwnd;

    /*
     * Create an event handler to clean up the Container structure when
     * tkwin is eventually deleted.
     */

    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbeddedEventProc,
	    (ClientData) winPtr);

    /*
     * If this is the first container, register an exit handler so that
     * things will get cleaned up at finalization.
     */

    if (tsdPtr->firstContainerPtr == (Container *) NULL) {
        Tcl_CreateExitHandler(CleanupContainerList, (ClientData) NULL);
    }
    
    /*
     * Save information about the container and the embedded window
     * in a Container structure.  If there is already an existing
     * Container structure, it means that both container and embedded
     * app. are in the same process.
     */

    for (containerPtr = tsdPtr->firstContainerPtr; 
            containerPtr != NULL; containerPtr = containerPtr->nextPtr) {
	if (containerPtr->parentHWnd == hwnd) {
	    winPtr->flags |= TK_BOTH_HALVES;
	    containerPtr->parentPtr->flags |= TK_BOTH_HALVES;
	    break;
	}
    }
    if (containerPtr == NULL) {
	containerPtr = (Container *) ckalloc(sizeof(Container));
	containerPtr->parentPtr = NULL;
	containerPtr->parentHWnd = hwnd;
	containerPtr->nextPtr = tsdPtr->firstContainerPtr;
	tsdPtr->firstContainerPtr = containerPtr;
    }

    /*
     * embeddedHWnd is not created yet. It will be created by TkWmMapWindow(),
     * which will send a TK_ATTACHWINDOW to the container window.
     * TkWinEmbeddedEventProc will process this message and set the embeddedHWnd
     * variable
     */

    containerPtr->embeddedPtr = winPtr;
    containerPtr->embeddedHWnd = NULL;

    winPtr->flags |= TK_EMBEDDED;
    winPtr->flags &= (~(TK_MAPPED));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeContainer --
 *
 *	This procedure is called to indicate that a particular window will
 *	be a container for an embedded application. This changes certain
 *	aspects of the window's behavior, such as whether it will receive
 *	events anymore.
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
    Tk_Window tkwin;
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * If this is the first container, register an exit handler so that
     * things will get cleaned up at finalization.
     */

    if (tsdPtr->firstContainerPtr == (Container *) NULL) {
        Tcl_CreateExitHandler(CleanupContainerList, (ClientData) NULL);
    }
    
    /*
     * Register the window as a container so that, for example, we can
     * find out later if the embedded app. is in the same process.
     */

    Tk_MakeWindowExist(tkwin);
    containerPtr = (Container *) ckalloc(sizeof(Container));
    containerPtr->parentPtr = winPtr;
    containerPtr->parentHWnd = Tk_GetHWND(Tk_WindowId(tkwin));
    containerPtr->embeddedHWnd = NULL;
    containerPtr->embeddedPtr = NULL;
    containerPtr->nextPtr = tsdPtr->firstContainerPtr;
    tsdPtr->firstContainerPtr = containerPtr;
    winPtr->flags |= TK_CONTAINER;

    /*
     * Unlike in tkUnixEmbed.c, we don't make any requests for events
     * in the embedded window here.  Now we just allow the embedding
     * of another TK application into TK windows. When the embedded
     * window makes a request, that will be done by sending to the
     * container window a WM_USER message, which will be intercepted
     * by TkWinContainerProc.
     *
     * We need to get structure events of the container itself, though.
     */

    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	ContainerEventProc, (ClientData) containerPtr);
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
 * TkWinEmbeddedEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when
 *	various useful events are received for the *children* of a
 *	container window. It forwards relevant information, such as
 *	geometry requests, from the events into the container's
 *	application.
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

LRESULT
TkWinEmbeddedEventProc(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Find the Container structure associated with the parent window.
     */

    for (containerPtr = tsdPtr->firstContainerPtr;
	    containerPtr->parentHWnd != hwnd;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr == NULL) {
	    panic("TkWinContainerProc couldn't find Container record");
	}
    }

    switch (message) {
      case TK_ATTACHWINDOW:
	/* An embedded window (either from this application or from
	 * another application) is trying to attach to this container.
	 * We attach it only if this container is not yet containing any
	 * window.
	 */
	if (containerPtr->embeddedHWnd == NULL) {
	    containerPtr->embeddedHWnd = (HWND)wParam;
	} else {
	    return 0;
	}

	break;
      case TK_GEOMETRYREQ:
	EmbedGeometryRequest(containerPtr, wParam, lParam);
	break;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedGeometryRequest --
 *
 *	This procedure is invoked when an embedded application requests
 *	a particular size.  It processes the request (which may or may
 *	not actually resize the window) and reflects the results back
 *	to the embedded application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If we deny the child's size change request, a Configure event
 *	is synthesized to let the child know that the size is the same
 *	as it used to be.  Events get processed while we're waiting for
 *	the geometry managers to do their thing.
 *
 *----------------------------------------------------------------------
 */

void
EmbedGeometryRequest(containerPtr, width, height)
    Container *containerPtr;	/* Information about the container window. */
    int width, height;		/* Size that the child has requested. */
{
    TkWindow * winPtr = containerPtr->parentPtr;
    
    /*
     * Forward the requested size into our geometry management hierarchy
     * via the container window.  We need to send a Configure event back
     * to the embedded application even if we decide not to resize
     * the window;  to make this happen, process all idle event handlers
     * synchronously here (so that the geometry managers have had a
     * chance to do whatever they want to do), and if the window's size
     * didn't change then generate a configure event.
     */
    Tk_GeometryRequest((Tk_Window)winPtr, width, height);

    if (containerPtr->embeddedHWnd != NULL) {
	while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)) {
	    /* Empty loop body. */
	}

	SetWindowPos(containerPtr->embeddedHWnd, NULL,
	    0, 0, winPtr->changes.width, winPtr->changes.height, SWP_NOZORDER);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ContainerEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when
 *	various useful events are received for the container window.
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
    Container *containerPtr = (Container *)clientData;
    Tk_Window tkwin = (Tk_Window)containerPtr->parentPtr;

    if (eventPtr->type == ConfigureNotify) {
	if (containerPtr->embeddedPtr == NULL) {
	    return;
	}
	/* Resize the embedded window, if there is any */
	if (containerPtr->embeddedHWnd) {
	    SetWindowPos(containerPtr->embeddedHWnd, NULL,
	        0, 0, Tk_Width(tkwin), Tk_Height(tkwin), SWP_NOZORDER);
	}
    } else if (eventPtr->type == DestroyNotify) {
	/* The container is gone, remove it from the list */
	EmbedWindowDeleted(containerPtr->parentPtr);
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

    for (containerPtr = tsdPtr->firstContainerPtr; containerPtr != NULL;
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
    HWND hwnd = GetParent(Tk_GetHWND(topLevelPtr->window));
    SendMessage(hwnd, TK_CLAIMFOCUS, (WPARAM) force, 0);
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
    /* not implemented */
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
	    containerPtr->embeddedHWnd = NULL;
	    containerPtr->embeddedPtr = NULL;
	    break;
	}
	if (containerPtr->parentPtr == winPtr) {
	    containerPtr->parentPtr = NULL;
	    break;
	}
	prevPtr = containerPtr;
	containerPtr = containerPtr->nextPtr;
	if (containerPtr == NULL) {
	    panic("EmbedWindowDeleted couldn't find window");
	}
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
