/* 
 * tkUnixEvent.c --
 *
 *	This file implements an event source for X displays for the
 *	UNIX version of Tk.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkUnixInt.h"
#include <signal.h>

/*
 * The following static indicates whether this module has been initialized
 * in the current thread.
 */

typedef struct ThreadSpecificData {
    int initialized;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Prototypes for procedures that are referenced only in this file:
 */

static void		DisplayCheckProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static void		DisplayExitHandler _ANSI_ARGS_((
			    ClientData clientData));
static void		DisplayFileProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static void		DisplaySetupProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static void		TransferXEventsToTcl _ANSI_ARGS_((Display *display));


/*
 *----------------------------------------------------------------------
 *
 * TkCreateXEventSource --
 *
 *	This procedure is called during Tk initialization to create
 *	the event source for X Window events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new event source is created.
 *
 *----------------------------------------------------------------------
 */

void
TkCreateXEventSource()
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateEventSource(DisplaySetupProc, DisplayCheckProc, NULL);
	Tcl_CreateExitHandler(DisplayExitHandler, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayExitHandler --
 *
 *	This function is called during finalization to clean up the
 *	display module.
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
DisplayExitHandler(clientData)
    ClientData clientData;	/* Not used. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_DeleteEventSource(DisplaySetupProc, DisplayCheckProc, NULL);
    tsdPtr->initialized = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpOpenDisplay --
 *
 *	Allocates a new TkDisplay, opens the X display, and establishes
 *	the file handler for the connection.
 *
 * Results:
 *	A pointer to a Tk display structure.
 *
 * Side effects:
 *	Opens a display.
 *
 *----------------------------------------------------------------------
 */

TkDisplay *
TkpOpenDisplay(display_name)
    char *display_name;
{
    TkDisplay *dispPtr;
    Display *display = XOpenDisplay(display_name);

    if (display == NULL) {
	return NULL;
    }
    dispPtr = (TkDisplay *) ckalloc(sizeof(TkDisplay));
    dispPtr->display = display;
    Tcl_CreateFileHandler(ConnectionNumber(display), TCL_READABLE,
	    DisplayFileProc, (ClientData) dispPtr);
    return dispPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCloseDisplay --
 *
 *	Cancels notifier callbacks and closes a display.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates the displayPtr.
 *
 *----------------------------------------------------------------------
 */

void
TkpCloseDisplay(displayPtr)
    TkDisplay *displayPtr;
{
    TkDisplay *dispPtr = (TkDisplay *) displayPtr;

    if (dispPtr->display != 0) {
        Tcl_DeleteFileHandler(ConnectionNumber(dispPtr->display));
	
        (void) XSync(dispPtr->display, False);
        (void) XCloseDisplay(dispPtr->display);
    }
    
    ckfree((char *) dispPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DisplaySetupProc --
 *
 *	This procedure implements the setup part of the UNIX X display
 *	event source.  It is invoked by Tcl_DoOneEvent before entering
 *	the notifier to check for events on all displays.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If data is queued on a display inside Xlib, then the maximum
 *	block time will be set to 0 to ensure that the notifier returns
 *	control to Tcl even if there is no more data on the X connection.
 *
 *----------------------------------------------------------------------
 */

static void
DisplaySetupProc(clientData, flags)
    ClientData clientData;	/* Not used. */
    int flags;
{
    TkDisplay *dispPtr;
    static Tcl_Time blockTime = { 0, 0 };

    if (!(flags & TCL_WINDOW_EVENTS)) {
	return;
    }

    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	 dispPtr = dispPtr->nextPtr) {

	/*
	 * Flush the display. If data is pending on the X queue, set
	 * the block time to zero.  This ensures that we won't block
	 * in the notifier if there is data in the X queue, but not on
	 * the server socket.
	 */

	XFlush(dispPtr->display);
	if (QLength(dispPtr->display) > 0) {
	    Tcl_SetMaxBlockTime(&blockTime);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 *  TransferXEventsToTcl
 *
 *      Transfer events from the X event queue to the Tk event queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves queued X events onto the Tcl event queue.
 *
 *----------------------------------------------------------------------
 */


static void
TransferXEventsToTcl(display)
    Display *display;
{
    int numFound;
    XEvent event;

    numFound = QLength(display);

    /*
     * Transfer events from the X event queue to the Tk event queue.
     */

    while (numFound > 0) {
	XNextEvent(display, &event);
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	numFound--;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayCheckProc --
 *
 *	This procedure checks for events sitting in the X event
 *	queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves queued events onto the Tcl event queue.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayCheckProc(clientData, flags)
    ClientData clientData;	/* Not used. */
    int flags;
{
    TkDisplay *dispPtr;

    if (!(flags & TCL_WINDOW_EVENTS)) {
	return;
    }

    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	 dispPtr = dispPtr->nextPtr) {
	XFlush(dispPtr->display);
	TransferXEventsToTcl(dispPtr->display);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * DisplayFileProc --
 *
 *	This procedure implements the file handler for the X connection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Makes entries on the Tcl event queue for all the events available
 *	from all the displays.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayFileProc(clientData, flags)
    ClientData clientData;		/* The display pointer. */
    int flags;				/* Should be TCL_READABLE. */
{
    TkDisplay *dispPtr = (TkDisplay *) clientData;
    Display *display = dispPtr->display;
    int numFound;

    XFlush(display);
    numFound = XEventsQueued(display, QueuedAfterReading);
    if (numFound == 0) {
	
	/*
	 * Things are very tricky if there aren't any events readable
	 * at this point (after all, there was supposedly data
	 * available on the connection).  A couple of things could
	 * have occurred:
	 * 
	 * One possibility is that there were only error events in the
	 * input from the server.  If this happens, we should return
	 * (we don't want to go to sleep in XNextEvent below, since
	 * this would block out other sources of input to the
	 * process).
	 *
	 * Another possibility is that our connection to the server
	 * has been closed.  This will not necessarily be detected in
	 * XEventsQueued (!!), so if we just return then there will be
	 * an infinite loop.  To detect such an error, generate a NoOp
	 * protocol request to exercise the connection to the server,
	 * then return.  However, must disable SIGPIPE while sending
	 * the request, or else the process will die from the signal
	 * and won't invoke the X error function to print a nice (?!)
	 * message.
	 */
	
	void (*oldHandler)();
	
	oldHandler = (void (*)()) signal(SIGPIPE, SIG_IGN);
	XNoOp(display);
	XFlush(display);
	(void) signal(SIGPIPE, oldHandler);
    }
    
    TransferXEventsToTcl(display);
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnixDoOneXEvent --
 *
 *	This routine waits for an X event to be processed or for
 *	a timeout to occur.  The timeout is specified as an absolute
 *	time.  This routine is called when Tk needs to wait for a
 *	particular X event without letting arbitrary events be
 *	processed.  The caller will typically call Tk_RestrictEvents
 *	to set up an event filter before calling this routine.  This
 *	routine will service at most one event per invocation.
 *
 * Results:
 *	Returns 0 if the timeout has expired, otherwise returns 1.
 *
 * Side effects:
 *	Can invoke arbitrary Tcl scripts.
 *
 *----------------------------------------------------------------------
 */

int
TkUnixDoOneXEvent(timePtr)
    Tcl_Time *timePtr;		/* Specifies the absolute time when the
				 * call should time out. */
{
    TkDisplay *dispPtr;
    static fd_mask readMask[MASK_SIZE];
    struct timeval blockTime, *timeoutPtr;
    Tcl_Time now;
    int fd, index, bit, numFound, numFdBits = 0;

    /*
     * Look for queued events first. 
     */

    if (Tcl_ServiceEvent(TCL_WINDOW_EVENTS)) {
	return 1;
    }

    /*
     * Compute the next block time and check to see if we have timed out.
     * Note that HP-UX defines tv_sec to be unsigned so we have to be
     * careful in our arithmetic.
     */

    if (timePtr) {
	TclpGetTime(&now);
	blockTime.tv_sec = timePtr->sec;
	blockTime.tv_usec = timePtr->usec - now.usec;
	if (blockTime.tv_usec < 0) {
	    now.sec += 1;
	    blockTime.tv_usec += 1000000;
	}
	if (blockTime.tv_sec < now.sec) {
	    blockTime.tv_sec = 0;
	    blockTime.tv_usec = 0;
	} else {
	    blockTime.tv_sec -= now.sec;
	}
	timeoutPtr = &blockTime;
    } else {
	timeoutPtr = NULL;
    }

    /*
     * Set up the select mask for all of the displays.  If a display has
     * data pending, then we want to poll instead of blocking.
     */

    memset((VOID *) readMask, 0, MASK_SIZE*sizeof(fd_mask));
    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	 dispPtr = dispPtr->nextPtr) {
	XFlush(dispPtr->display);
	if (QLength(dispPtr->display) > 0) {
	    blockTime.tv_sec = 0;
	    blockTime.tv_usec = 0;
	}
	fd = ConnectionNumber(dispPtr->display);
	index = fd/(NBBY*sizeof(fd_mask));
	bit = 1 << (fd%(NBBY*sizeof(fd_mask)));
	readMask[index] |= bit;
	if (numFdBits <= fd) {
	    numFdBits = fd+1;
	}
    }

    numFound = select(numFdBits, (SELECT_MASK *) &readMask[0], NULL, NULL,
	    timeoutPtr);
    if (numFound <= 0) {
	/*
	 * Some systems don't clear the masks after an error, so
	 * we have to do it here.
	 */

	memset((VOID *) readMask, 0, MASK_SIZE*sizeof(fd_mask));
    }

    /*
     * Process any new events on the display connections.
     */

    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	 dispPtr = dispPtr->nextPtr) {
	fd = ConnectionNumber(dispPtr->display);
	index = fd/(NBBY*sizeof(fd_mask));
	bit = 1 << (fd%(NBBY*sizeof(fd_mask)));
	if ((readMask[index] & bit) || (QLength(dispPtr->display) > 0)) {
	    DisplayFileProc((ClientData)dispPtr, TCL_READABLE);
	}
    }
    if (Tcl_ServiceEvent(TCL_WINDOW_EVENTS)) {
	return 1;
    }

    /*
     * Check to see if we timed out.
     */

    if (timePtr) {
	TclpGetTime(&now);
	if ((now.sec > timePtr->sec) || ((now.sec == timePtr->sec)
		&& (now.usec > timePtr->usec))) {
	    return 0;
	}
    }

    /*
     * We had an event but we did not generate a Tcl event from it. Behave
     * as though we dealt with it. (JYL&SS)
     */

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSync --
 *
 *	This routine ensures that all pending X requests have been
 *	seen by the server, and that any pending X events have been
 *	moved onto the Tk event queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Places new events on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

void
TkpSync(display)
    Display *display;		/* Display to sync. */
{
    XSync(display, False);

    /*
     * Transfer events from the X event queue to the Tk event queue.
     */
    TransferXEventsToTcl(display);

}
