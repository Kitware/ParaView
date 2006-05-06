/*
 * tkMacOSXCarbonEvents.c --
 *
 *  This file implements functions that register for and handle
 *  various Carbon Events and Timers. Most carbon events of interest
 *  to TkAqua are processed in a handler registered on the dispatcher
 *  event target so that we get first crack at them before HIToolbox
 *  dispatchers/processes them further.
 *  As some events are sent directly to the focus or app event target
 *  and not dispatched normally, we also register a handler on the
 *  application event target.
 *
 * Copyright 2001, Apple Computer, Inc.
 * Copyright (c) 2005 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
 *
 * RCS: @(#) Id
 */

#include "tkMacOSXInt.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXDebug.h"

/*
#ifdef  TK_MAC_DEBUG
#define TK_MAC_DEBUG_CARBON_EVENTS
#endif
*/

/* Declarations of functions used only in this file */
static OSStatus CarbonEventHandlerProc(EventHandlerCallRef callRef,
            EventRef event, void *userData);
static OSStatus InstallStandardApplicationEventHandler();
static void ExitRaelEventHandlerProc (EventHandlerCallRef, EventRef, void*)
  __attribute__ ((__noreturn__));
static void CarbonTimerProc(EventLoopTimerRef timer, void *userData);

/* Static data used by several functions in this file */
static jmp_buf exitRaelJmpBuf;
static EventLoopTimerRef carbonTimer = NULL;
static int carbonTimerEnabled = 0;

/*
 *----------------------------------------------------------------------
 *
 * CarbonEventHandlerProc --
 *
 *    This procedure is the handler for all registered CarbonEvents.
 *
 * Results:
 *    OS status code.
 *
 * Side effects:
 *    Dispatches CarbonEvents.
 *
 *----------------------------------------------------------------------
 */

static OSStatus
CarbonEventHandlerProc (
  EventHandlerCallRef callRef,
  EventRef event,
  void *userData)
{
    OSStatus       result = eventNotHandledErr;
    TkMacOSXEvent    macEvent;
    MacEventStatus   eventStatus;

    macEvent.eventRef = event;
    macEvent.eClass = GetEventClass(macEvent.eventRef);
    macEvent.eKind = GetEventKind(macEvent.eventRef);
    macEvent.interp = (Tcl_Interp *) userData;
    bzero(&eventStatus, sizeof(eventStatus));

#if defined(TK_MAC_DEBUG) && defined(TK_MAC_DEBUG_CARBON_EVENTS)
    char buf [256];
    if (macEvent.eKind != kEventMouseMoved &&
      macEvent.eKind != kEventMouseDragged) {
  CarbonEventToAscii(event, buf);
  fprintf(stderr, "CarbonEventHandlerProc started handling %s\n", buf);
  TkMacOSXInitNamedDebugSymbol(HIToolbox, void, _DebugPrintEvent,
    EventRef inEvent);
  if (_DebugPrintEvent) {
      /* Carbon-internal event debugging (c.f. Technote 2124) */
      _DebugPrintEvent(event);
  }
    }
#endif /* TK_MAC_DEBUG_CARBON_EVENTS */

    TkMacOSXProcessEvent(&macEvent,&eventStatus);
    if (eventStatus.stopProcessing) {
  result = noErr;
    }

#if defined(TK_MAC_DEBUG) && defined(TK_MAC_DEBUG_CARBON_EVENTS)
    if (macEvent.eKind != kEventMouseMoved &&
      macEvent.eKind != kEventMouseDragged) {
  fprintf(stderr,
    "CarbonEventHandlerProc finished handling %s: %s handled\n",
    buf, eventStatus.stopProcessing ? "   " : "not");
    }
#endif /* TK_MAC_DEBUG_CARBON_EVENTS */
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInitCarbonEvents --
 *
 *    This procedure initializes all CarbonEvent handlers.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Handlers for Carbon Events are registered.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXInitCarbonEvents (
  Tcl_Interp *interp)
{
    OSStatus err;
    const EventTypeSpec dispatcherEventTypes[] = {
      {kEventClassMouse,    kEventMouseDown},
      {kEventClassMouse,    kEventMouseUp},
      {kEventClassMouse,    kEventMouseMoved},
      {kEventClassMouse,    kEventMouseDragged},
      {kEventClassMouse,    kEventMouseWheelMoved},
      {kEventClassWindow,    kEventWindowUpdate},
      {kEventClassWindow,    kEventWindowActivated},
      {kEventClassWindow,    kEventWindowDeactivated},
      {kEventClassKeyboard,  kEventRawKeyDown},
      {kEventClassKeyboard,  kEventRawKeyRepeat},
      {kEventClassKeyboard,  kEventRawKeyUp},
      {kEventClassKeyboard,  kEventRawKeyModifiersChanged},
      {kEventClassKeyboard,  kEventRawKeyRepeat},
      {kEventClassApplication,  kEventAppActivated},
      {kEventClassApplication,  kEventAppDeactivated},
      {kEventClassApplication,  kEventAppQuit},
    };
    const EventTypeSpec applicationEventTypes[] = {
      {kEventClassMenu,    kEventMenuBeginTracking},
      {kEventClassMenu,    kEventMenuEndTracking},
      {kEventClassCommand,  kEventCommandProcess},
      {kEventClassCommand,  kEventCommandUpdateStatus},
      {kEventClassMouse,    kEventMouseWheelMoved},
      {kEventClassWindow,    kEventWindowExpanded},
      {kEventClassApplication,  kEventAppHidden},
      {kEventClassApplication,  kEventAppShown},
      {kEventClassApplication,    kEventAppAvailableWindowBoundsChanged},
    };
    EventHandlerUPP handler = NewEventHandlerUPP(CarbonEventHandlerProc);

    err = InstallStandardApplicationEventHandler();
    if (err != noErr) {
#ifdef TK_MAC_DEBUG
  fprintf(stderr, "InstallStandardApplicationEventHandler failed, %d\n",
    (int) err);
#endif
    }
    err = InstallEventHandler(GetEventDispatcherTarget(), handler,
      GetEventTypeCount(dispatcherEventTypes), dispatcherEventTypes,
      (void *) interp, NULL);
    if (err != noErr) {
#ifdef TK_MAC_DEBUG
  fprintf(stderr, "InstallEventHandler failed, %d\n", (int) err);
#endif
    }
    err = InstallEventHandler(GetApplicationEventTarget(), handler,
      GetEventTypeCount(applicationEventTypes), applicationEventTypes,
      (void *) interp, NULL);
    if (err != noErr) {
#ifdef TK_MAC_DEBUG
  fprintf(stderr, "InstallEventHandler failed, %d\n", (int) err);
#endif
    }

#if defined(TK_MAC_DEBUG) && defined(TK_MAC_DEBUG_CARBON_EVENTS)
    TkMacOSXInitNamedDebugSymbol(HIToolbox, void, TraceEventByName, char*);
    if (TraceEventByName) {
  /* Carbon-internal event debugging (c.f. Technote 2124) */
  TraceEventByName("kEventMouseDown");
  TraceEventByName("kEventMouseUp");
  TraceEventByName("kEventMouseWheelMoved");
  TraceEventByName("kEventMouseScroll");
  TraceEventByName("kEventWindowUpdate");
  TraceEventByName("kEventWindowActivated");
  TraceEventByName("kEventWindowDeactivated");
  TraceEventByName("kEventRawKeyDown");
  TraceEventByName("kEventRawKeyRepeat");
  TraceEventByName("kEventRawKeyUp");
  TraceEventByName("kEventRawKeyModifiersChanged");
  TraceEventByName("kEventRawKeyRepeat");
  TraceEventByName("kEventAppActivated");
  TraceEventByName("kEventAppDeactivated");
  TraceEventByName("kEventAppQuit");
  TraceEventByName("kEventMenuBeginTracking");
  TraceEventByName("kEventMenuEndTracking");
  TraceEventByName("kEventCommandProcess");
  TraceEventByName("kEventCommandUpdateStatus");
  TraceEventByName("kEventWindowExpanded");
  TraceEventByName("kEventAppHidden");
  TraceEventByName("kEventAppShown");
  TraceEventByName("kEventAppAvailableWindowBoundsChanged");
    }
#endif /* TK_MAC_DEBUG_CARBON_EVENTS */
}

/*
 *----------------------------------------------------------------------
 *
 * InstallStandardApplicationEventHandler --
 *
 *    This procedure installs the carbon standard application event
 *    handler.
 *
 * Results:
 *    OS status code.
 *
 * Side effects:
 *    Standard handlers for application Carbon Events are registered.
 *
 *----------------------------------------------------------------------
 */

static OSStatus
InstallStandardApplicationEventHandler()
{
   /*
    * This is a hack to workaround missing Carbon API to install the standard
    * application event handler (InstallStandardEventHandler() does not work
    * on the application target). The only way to install the standard app
    * handler is to call RunApplicationEventLoop(), but since we are running
    * our own event loop, we'll immediately need to break out of RAEL again:
    * we do this via longjmp out of the ExitRaelEventHandlerProc event handler
    * called first off from RAEL by posting a high priority dummy event.
    * This workaround is derived from a similar approach in Technical Q&A 1061.
    */
    enum { kExitRaelEvent = 'ExiT' };
    const EventTypeSpec exitRaelEventType =
      { kExitRaelEvent,    kExitRaelEvent};
    EventHandlerUPP exitRaelEventHandler;
    EventHandlerRef exitRaelEventHandlerRef = NULL;
    EventRef exitRaelEvent = NULL;
    OSStatus err = memFullErr;

    exitRaelEventHandler = NewEventHandlerUPP(
      (EventHandlerProcPtr) ExitRaelEventHandlerProc);
    if (exitRaelEventHandler) {
  err = InstallEventHandler(GetEventDispatcherTarget(),
    exitRaelEventHandler, 1, &exitRaelEventType, NULL,
    &exitRaelEventHandlerRef);
    }
    if (err == noErr) {
  err = CreateEvent(NULL, kExitRaelEvent, kExitRaelEvent, 
    GetCurrentEventTime(), kEventAttributeNone, &exitRaelEvent);
    }
    if (err == noErr) {
  err = PostEventToQueue(GetMainEventQueue(), exitRaelEvent,
    kEventPriorityHigh);
    }
    if (err == noErr) {
  if (!setjmp(exitRaelJmpBuf)) {
      RunApplicationEventLoop();
      /* This point should never be reached ! */
      Tcl_Panic("RunApplicationEventLoop exited !");
  }
    }
    if (exitRaelEvent) {
  ReleaseEvent(exitRaelEvent);
    }
    if (exitRaelEventHandlerRef) {
  RemoveEventHandler(exitRaelEventHandlerRef);
    }
    if (exitRaelEventHandler) {
  DisposeEventHandlerUPP(exitRaelEventHandler);
    }
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * ExitRaelEventHandlerProc --
 *
 *    This procedure is the dummy event handler used to break out of
 *    RAEL via longjmp, it is called as the first ever event handler
 *    in RAEL by posting a high priority dummy event.
 *
 * Results:
 *    None. Never returns !
 *
 * Side effects:
 *    longjmp back to InstallStandardApplicationEventHandler().
 *
 *----------------------------------------------------------------------
 */

static void
ExitRaelEventHandlerProc (
  EventHandlerCallRef callRef,
  EventRef event, void *userData)
{
    longjmp(exitRaelJmpBuf, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * CarbonTimerProc --
 *
 *    This procedure is the carbon timer handler that runs the tcl
 *    event loop periodically. It does not process TCL_WINDOW_EVENTS
 *    to avoid reentry issues with Carbon, nor TCL_IDLE_EVENTS since
 *    it is only intended to be called during short periods of busy
 *    time such as during menu tracking.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Runs the Tcl event loop.
 *
 *----------------------------------------------------------------------
 */

static void
CarbonTimerProc (
  EventLoopTimerRef timer,
  void *userData)
{
    while(carbonTimerEnabled && Tcl_DoOneEvent(
      TCL_FILE_EVENTS|TCL_TIMER_EVENTS|TCL_DONT_WAIT)) {
#if defined(TK_MAC_DEBUG) && defined(TK_MAC_DEBUG_CARBON_EVENTS)
  fprintf(stderr, "Processed tcl event from carbon timer\n");
#endif /* TK_MAC_DEBUG_CARBON_EVENTS */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXStartTclEventLoopCarbonTimer --
 *
 *    This procedure installs (if necessary) and starts a carbon
 *    event timer that runs the tcl event loop periodically.
 *    It should be called whenever a nested carbon event loop is
 *    run by HIToolbox (e.g. during menutracking) to ensure that
 *    non-window non-idle tcl events are processed.
 *
 * Results:
 *    OS status code.
 *
 * Side effects:
 *    Carbon event timer is installed and started.
 *
 *----------------------------------------------------------------------
 */

OSStatus
TkMacOSXStartTclEventLoopCarbonTimer()
{
    OSStatus err;

    if(!carbonTimer) {
  EventLoopTimerUPP timerUPP = NewEventLoopTimerUPP(CarbonTimerProc);
  err = InstallEventLoopTimer(GetMainEventLoop(), kEventDurationNoWait,
    5 * kEventDurationMillisecond, timerUPP, NULL, &carbonTimer);
  if (err != noErr) {
#ifdef TK_MAC_DEBUG
      fprintf(stderr, "InstallEventLoopTimer failed, %d\n", (int) err);
#endif
  }
    } else {
  err = SetEventLoopTimerNextFireTime(carbonTimer, kEventDurationNoWait);
  if (err != noErr) {
#ifdef TK_MAC_DEBUG
      fprintf(stderr, "SetEventLoopTimerNextFireTime failed, %d\n",
        (int) err);
#endif
  }
    }
    carbonTimerEnabled = 1;
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXStopTclEventLoopCarbonTimer --
 *
 *    This procedure stops the carbon event timer started by
 *    TkMacOSXStartTclEventLoopCarbonTimer().
 *
 * Results:
 *    OS status code.
 *
 * Side effects:
 *   Carbon event timer is stopped.
 *
 *----------------------------------------------------------------------
 */

OSStatus
TkMacOSXStopTclEventLoopCarbonTimer()
{
    OSStatus err = noErr;

    if(carbonTimer) {
  err = SetEventLoopTimerNextFireTime(carbonTimer, kEventDurationForever);
  if (err != noErr) {
#ifdef TK_MAC_DEBUG
      fprintf(stderr, "SetEventLoopTimerNextFireTime failed, %d\n",
        (int) err);
#endif
  }
    }
    carbonTimerEnabled = 0;
    return err;
}

