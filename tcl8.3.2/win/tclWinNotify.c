/* 
 * tclWinNotify.c --
 *
 *	This file contains Windows-specific procedures for the notifier,
 *	which is the lowest-level part of the Tcl event loop.  This file
 *	works together with ../generic/tclNotify.c.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"
#include <winsock.h>

/*
 * The follwing static indicates whether this module has been initialized.
 */

static int initialized = 0;

#define INTERVAL_TIMER 1	/* Handle of interval timer. */

#define WM_WAKEUP WM_USER	/* Message that is send by
				 * Tcl_AlertNotifier. */
/*
 * The following static structure contains the state information for the
 * Windows implementation of the Tcl notifier.  One of these structures
 * is created for each thread that is using the notifier.  
 */

typedef struct ThreadSpecificData {
    CRITICAL_SECTION crit;	/* Monitor for this notifier. */
    DWORD thread;		/* Identifier for thread associated with this
				 * notifier. */
    HANDLE event;		/* Event object used to wake up the notifier
				 * thread. */
    int pending;		/* Alert message pending, this field is
				 * locked by the notifierMutex. */
    HWND hwnd;			/* Messaging window. */
    int timeout;		/* Current timeout value. */
    int timerActive;		/* 1 if interval timer is running. */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

extern TclStubs tclStubs;
/*
 * The following static indicates the number of threads that have
 * initialized notifiers.  It controls the lifetime of the TclNotifier
 * window class.
 *
 * You must hold the notifierMutex lock before accessing this variable.
 */

static int notifierCount = 0;
TCL_DECLARE_MUTEX(notifierMutex)

/*
 * Static routines defined in this file.
 */

static LRESULT CALLBACK	NotifierProc(HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam);


/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitNotifier --
 *
 *	Initializes the platform specific notifier state.
 *
 * Results:
 *	Returns a handle to the notifier state for this thread..
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_InitNotifier()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    WNDCLASS class;

    /*
     * Register Notifier window class if this is the first thread to
     * use this module.
     */

    Tcl_MutexLock(&notifierMutex);
    if (notifierCount == 0) {
	class.style = 0;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance = TclWinGetTclInstance();
	class.hbrBackground = NULL;
	class.lpszMenuName = NULL;
	class.lpszClassName = "TclNotifier";
	class.lpfnWndProc = NotifierProc;
	class.hIcon = NULL;
	class.hCursor = NULL;

	if (!RegisterClassA(&class)) {
	    panic("Unable to register TclNotifier window class");
	}
    }
    notifierCount++;
    Tcl_MutexUnlock(&notifierMutex);

    tsdPtr->pending = 0;
    tsdPtr->timerActive = 0;

    InitializeCriticalSection(&tsdPtr->crit);

    tsdPtr->hwnd = NULL;
    tsdPtr->thread = GetCurrentThreadId();
    tsdPtr->event = CreateEvent(NULL, TRUE /* manual */,
	    FALSE /* !signaled */, NULL);

    return (ClientData) tsdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FinalizeNotifier --
 *
 *	This function is called to cleanup the notifier state before
 *	a thread is terminated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May dispose of the notifier window and class.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FinalizeNotifier(clientData)
    ClientData clientData;	/* Pointer to notifier data. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) clientData;

    DeleteCriticalSection(&tsdPtr->crit);
    CloseHandle(tsdPtr->event);

    /*
     * Clean up the timer and messaging window for this thread.
     */

    if (tsdPtr->hwnd) {
	KillTimer(tsdPtr->hwnd, INTERVAL_TIMER);
	DestroyWindow(tsdPtr->hwnd);
    }

    /*
     * If this is the last thread to use the notifier, unregister
     * the notifier window class.
     */

    Tcl_MutexLock(&notifierMutex);
    notifierCount--;
    if (notifierCount == 0) {
	UnregisterClassA("TclNotifier", TclWinGetTclInstance());
    }
    Tcl_MutexUnlock(&notifierMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AlertNotifier --
 *
 *	Wake up the specified notifier from any thread. This routine
 *	is called by the platform independent notifier code whenever
 *	the Tcl_ThreadAlert routine is called.  This routine is
 *	guaranteed not to be called on a given notifier after
 *	Tcl_FinalizeNotifier is called for that notifier.  This routine
 *	is typically called from a thread other than the notifier's
 *	thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends a message to the messaging window for the notifier
 *	if there isn't already one pending.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AlertNotifier(clientData)
    ClientData clientData;	/* Pointer to thread data. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) clientData;

    /*
     * Note that we do not need to lock around access to the hwnd
     * because the race condition has no effect since any race condition
     * implies that the notifier thread is already awake.
     */

    if (tsdPtr->hwnd) {
	/*
	 * We do need to lock around access to the pending flag.
	 */

	EnterCriticalSection(&tsdPtr->crit);
	if (!tsdPtr->pending) {
	    PostMessage(tsdPtr->hwnd, WM_WAKEUP, 0, 0);
	}
	tsdPtr->pending = 1;
	LeaveCriticalSection(&tsdPtr->crit);
    } else {
	SetEvent(tsdPtr->event);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetTimer --
 *
 *	This procedure sets the current notifier timer value.  The
 *	notifier will ensure that Tcl_ServiceAll() is called after
 *	the specified interval, even if no events have occurred.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Replaces any previous timer.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetTimer(
    Tcl_Time *timePtr)		/* Maximum block time, or NULL. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    UINT timeout;

    /*
     * Allow the notifier to be hooked.  This may not make sense
     * on Windows, but mirrors the UNIX hook.
     */

    if (tclStubs.tcl_SetTimer != Tcl_SetTimer) {
	tclStubs.tcl_SetTimer(timePtr);
	return;
    }

    /*
     * We only need to set up an interval timer if we're being called
     * from an external event loop.  If we don't have a window handle
     * then we just return immediately and let Tcl_WaitForEvent handle
     * timeouts.
     */

    if (!tsdPtr->hwnd) {
	return;
    }

    if (!timePtr) {
	timeout = 0;
    } else {
	/*
	 * Make sure we pass a non-zero value into the timeout argument.
	 * Windows seems to get confused by zero length timers.
	 */

	timeout = timePtr->sec * 1000 + timePtr->usec / 1000;
	if (timeout == 0) {
	    timeout = 1;
	}
    }
    tsdPtr->timeout = timeout;
    if (timeout != 0) {
	tsdPtr->timerActive = 1;
	SetTimer(tsdPtr->hwnd, INTERVAL_TIMER,
		    (unsigned long) tsdPtr->timeout, NULL);
    } else {
	tsdPtr->timerActive = 0;
	KillTimer(tsdPtr->hwnd, INTERVAL_TIMER);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ServiceModeHook --
 *
 *	This function is invoked whenever the service mode changes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If this is the first time the notifier is set into
 *	TCL_SERVICE_ALL, then the communication window is created.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ServiceModeHook(mode)
    int mode;			/* Either TCL_SERVICE_ALL, or
				 * TCL_SERVICE_NONE. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * If this is the first time that the notifier has been used from a
     * modal loop, then create a communication window.  Note that after
     * this point, the application needs to service events in a timely
     * fashion or Windows will hang waiting for the window to respond
     * to synchronous system messages.  At some point, we may want to
     * consider destroying the window if we leave the modal loop, but
     * for now we'll leave it around.
     */

    if (mode == TCL_SERVICE_ALL && !tsdPtr->hwnd) {
	tsdPtr->hwnd = CreateWindowA("TclNotifier", "TclNotifier", WS_TILED,
		0, 0, 0, 0, NULL, NULL, TclWinGetTclInstance(), NULL);
	/*
	 * Send an initial message to the window to ensure that we wake up the
	 * notifier once we get into the modal loop.  This will force the
	 * notifier to recompute the timeout value and schedule a timer
	 * if one is needed.
	 */

	Tcl_AlertNotifier((ClientData)tsdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NotifierProc --
 *
 *	This procedure is invoked by Windows to process events on
 *	the notifier window.  Messages will be sent to this window
 *	in response to external timer events or calls to
 *	TclpAlertTsdPtr->
 *
 * Results:
 *	A standard windows result.
 *
 * Side effects:
 *	Services any pending events.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
NotifierProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (message == WM_WAKEUP) {
	EnterCriticalSection(&tsdPtr->crit);
	tsdPtr->pending = 0;
	LeaveCriticalSection(&tsdPtr->crit);
    } else if (message != WM_TIMER) {
	return DefWindowProc(hwnd, message, wParam, lParam);
    }
	
    /*
     * Process all of the runnable events.
     */

    Tcl_ServiceAll();
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WaitForEvent --
 *
 *	This function is called by Tcl_DoOneEvent to wait for new
 *	events on the message queue.  If the block time is 0, then
 *	Tcl_WaitForEvent just polls the event queue without blocking.
 *
 * Results:
 *	Returns -1 if a WM_QUIT message is detected, returns 1 if
 *	a message was dispatched, otherwise returns 0.
 *
 * Side effects:
 *	Dispatches a message to a window procedure, which could do
 *	anything.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WaitForEvent(
    Tcl_Time *timePtr)		/* Maximum block time, or NULL. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    MSG msg;
    DWORD timeout, result;
    int status;

    /*
     * Allow the notifier to be hooked.  This may not make
     * sense on windows, but mirrors the UNIX hook.
     */

    if (tclStubs.tcl_WaitForEvent != Tcl_WaitForEvent) {
	return tclStubs.tcl_WaitForEvent(timePtr);
    }

    /*
     * Compute the timeout in milliseconds.
     */

    if (timePtr) {
	timeout = timePtr->sec * 1000 + timePtr->usec / 1000;
    } else {
	timeout = INFINITE;
    }

    /*
     * Check to see if there are any messages in the queue before waiting
     * because MsgWaitForMultipleObjects will not wake up if there are events
     * currently sitting in the queue.
     */

    if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
	/*
	 * Wait for something to happen (a signal from another thread, a
	 * message, or timeout).
	 */

	result = MsgWaitForMultipleObjects(1, &tsdPtr->event, FALSE, timeout,
		QS_ALLINPUT);
    }

    /*
     * Check to see if there are any messages to process.
     */

    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
	/*
	 * Retrieve and dispatch the first message.
	 */

	result = GetMessage(&msg, NULL, 0, 0);
	if (result == 0) {
	    /*
	     * We received a request to exit this thread (WM_QUIT), so
	     * propagate the quit message and start unwinding.
	     */

	    PostQuitMessage(msg.wParam);
	    status = -1;
	} else if (result == -1) {
	    /*
	     * We got an error from the system.  I have no idea why this would
	     * happen, so we'll just unwind.
	     */

	    status = -1;
	} else {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	    status = 1;
	}
    } else {
	status = 0;
    }

    ResetEvent(tsdPtr->event);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Sleep --
 *
 *	Delay execution for the specified number of milliseconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Time passes.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Sleep(ms)
    int ms;			/* Number of milliseconds to sleep. */
{
    Sleep(ms);
}
