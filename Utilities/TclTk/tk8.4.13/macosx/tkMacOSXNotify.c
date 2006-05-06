/*
 * tkMacOSXNotify.c --
 *
 *  This file contains the implementation of a tcl event source 
 *  for the Carbon event loop.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 * Copyright 2005, Tcl Core Team.
 * Copyright (c) 2005 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkMacOSXInt.h"
#include "tkMacOSXEvent.h"
#include <pthread.h>

/*
 * The following static indicates whether this module has been initialized
 * in the current thread.
 */

typedef struct ThreadSpecificData {
    int initialized;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

static void TkMacOSXNotifyExitHandler(ClientData clientData);
static void CarbonEventsSetupProc(ClientData clientData, int flags);
static void CarbonEventsCheckProc(ClientData clientData, int flags);

/*
 *----------------------------------------------------------------------
 *
 * Tk_MacOSXSetupTkNotifier --
 *
 *  This procedure is called during Tk initialization to create
 *  the event source for Carbon events.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  A new event source is created.
 *
 *----------------------------------------------------------------------
 */

void
Tk_MacOSXSetupTkNotifier()
{
    ThreadSpecificData *tsdPtr = Tcl_GetThreadData(&dataKey,
      sizeof(ThreadSpecificData));
    
    if (!tsdPtr->initialized) {
        /* HACK ALERT: There is a bug in Jaguar where when it goes to make
         * the event queue for the Main Event Loop, it stores the Current
         * event loop rather than the Main Event Loop in the Queue structure.
         * So we have to make sure that the Main Event Queue gets set up on
         * the main thread.  Calling GetMainEventQueue will force this to
         * happen.
         */
        GetMainEventQueue();

        tsdPtr->initialized = 1;
        /* Install Carbon events event source in main event loop thread. */
        if (GetCurrentEventLoop() == GetMainEventLoop()) {
            if (!pthread_main_np()) {
                /* 
                 * Panic if the Carbon main event loop thread (i.e. the 
                 * thread  where HIToolbox was first loaded) is not the
                 * main application thread, as Carbon does not support
                 * this properly.
                 */
                Tcl_Panic("Tk_MacOSXSetupTkNotifier: %s",
                    "first [load] of TkAqua has to occur in the main thread!");
            }
            Tcl_CreateEventSource(CarbonEventsSetupProc, 
                    CarbonEventsCheckProc, GetMainEventQueue());
            TkCreateExitHandler(TkMacOSXNotifyExitHandler, NULL);
  }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXNotifyExitHandler --
 *
 *  This function is called during finalization to clean up the
 *  TkMacOSXNotify module.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
TkMacOSXNotifyExitHandler(clientData)
    ClientData clientData;  /* Not used. */
{
    ThreadSpecificData *tsdPtr = Tcl_GetThreadData(&dataKey,
      sizeof(ThreadSpecificData));

    Tcl_DeleteEventSource(CarbonEventsSetupProc, 
            CarbonEventsCheckProc, GetMainEventQueue());
    tsdPtr->initialized = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CarbonEventsSetupProc --
 *
 *  This procedure implements the setup part of the Carbon Events
 *  event source.  It is invoked by Tcl_DoOneEvent before entering
 *  the notifier to check for events.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  If Carbon events are queued, then the maximum block time will be
 *  set to 0 to ensure that the notifier returns control to Tcl.
 *
 *----------------------------------------------------------------------
 */

static void
CarbonEventsSetupProc(clientData, flags)
    ClientData clientData;
    int flags;
{
    static Tcl_Time blockTime = { 0, 0 };

    if (!(flags & TCL_WINDOW_EVENTS)) {
  return;
    }

    if (GetNumEventsInQueue((EventQueueRef)clientData)) {
        Tcl_SetMaxBlockTime(&blockTime);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CarbonEventsCheckProc --
 *
 *  This procedure processes events sitting in the Carbon event
 *  queue.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Moves applicable queued Carbon events onto the Tcl event queue.
 *
 *----------------------------------------------------------------------
 */

static void
CarbonEventsCheckProc(clientData, flags)
    ClientData clientData;
    int flags;
{
    int numFound;
    OSStatus err = noErr;
    
    if (!(flags & TCL_WINDOW_EVENTS)) {
  return;
    }

    numFound = GetNumEventsInQueue((EventQueueRef)clientData);
    
    /* Avoid starving other event sources: */
    if (numFound > 4) {
        numFound = 4;
    }
    while (numFound > 0 && err == noErr) {
        err = TkMacOSXReceiveAndProcessEvent();
        numFound--;
    }
}
