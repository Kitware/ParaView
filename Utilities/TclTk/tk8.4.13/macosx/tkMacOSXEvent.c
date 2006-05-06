/* 
 * tkMacOSXEvent.c --
 *
 * This file contains the basic Mac OS X Event handling routines.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkMacOSXInt.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXDebug.h"

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXFlushWindows --
 *
 *      This routine flushes all the Carbon windows of the application.  It
 *      is called by the setup procedure for the Tcl/Carbon event source.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Flushes all Carbon windows
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXFlushWindows ()
{
    WindowRef wRef = GetWindowList();
    
    while (wRef) {
        CGrafPtr portPtr = GetWindowPort(wRef);
        if (QDIsPortBuffered(portPtr)) {
            QDFlushPortBuffer(portPtr, NULL);
        }
        wRef = GetNextWindow(wRef);
    }
}

/*      
 *----------------------------------------------------------------------
 *   
 * TkMacOSXProcessEvent --
 *   
 *      This dispatches a filtered Carbon event to the appropriate handler
 *
 *      Note on MacEventStatus.stopProcessing: Please be conservative in the
 *      individual handlers and don't assume the event is fully handled
 *      unless you *really* need to ensure that other handlers don't see the
 *      event anymore.  Some OS manager or library might be interested in
 *      events even after they are already handled on the Tk level.
 *
 * Results: 
 *      0 on success
 *      -1 on failure
 *
 * Side effects:
 *      Converts a Carbon event to a Tk event
 *   
 *----------------------------------------------------------------------
 */

int  
TkMacOSXProcessEvent(TkMacOSXEvent * eventPtr, MacEventStatus * statusPtr)
{
    switch (eventPtr->eClass) {
        case kEventClassMouse:
            TkMacOSXProcessMouseEvent(eventPtr, statusPtr);
            break;
        case kEventClassWindow:
            TkMacOSXProcessWindowEvent(eventPtr, statusPtr);
            break;  
        case kEventClassKeyboard:
            TkMacOSXProcessKeyboardEvent(eventPtr, statusPtr);
            break;
        case kEventClassApplication:
            TkMacOSXProcessApplicationEvent(eventPtr, statusPtr);
            break;
        case kEventClassMenu:
            TkMacOSXProcessMenuEvent(eventPtr, statusPtr);
            break;  
        case kEventClassCommand:
            TkMacOSXProcessCommandEvent(eventPtr, statusPtr);
            break;  
        default:
#ifdef TK_MAC_DEBUG
            {
                char buf [256];
                fprintf(stderr,
                    "Unrecognised event : %s\n",
                    CarbonEventToAscii(eventPtr->eventRef, buf));
            }
#endif
            break;
    }   
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessMenuEvent --
 *
 *    This routine processes the event in eventPtr, and
 *    generates the appropriate Tk events from it.
 *
 * Results:
 *    True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *    Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXProcessMenuEvent(TkMacOSXEvent *eventPtr, MacEventStatus * statusPtr)
{
    int        menuContext;
    OSStatus      status;

    switch (eventPtr->eKind) {
  case kEventMenuBeginTracking:
  case kEventMenuEndTracking:
      break;
  default:
      return 0;
      break;
    }
    status = GetEventParameter(eventPtr->eventRef, 
      kEventParamMenuContext,
      typeUInt32, NULL, 
      sizeof(menuContext), NULL,
      &menuContext);
    if (status == noErr && (menuContext & kMenuContextMenuBar)) {
        static int oldMode = TCL_SERVICE_ALL;
        if (eventPtr->eKind == kEventMenuBeginTracking) {
            oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
            TkMacOSXClearMenubarActive();
        
            /*
             * Handle -postcommand
             */
        
            TkMacOSXPreprocessMenu();
        } else {
            Tcl_SetServiceMode(oldMode);   
        }
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessCommandEvent --
 *
 *    This routine processes the event in eventPtr, and
 *    generates the appropriate Tk events from it.
 *
 * Results:
 *    True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *    Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXProcessCommandEvent(TkMacOSXEvent *eventPtr, MacEventStatus * statusPtr)
{
    HICommand      command;
    int       menuContext;
    OSStatus      status;

    switch (eventPtr->eKind) {
  case kEventCommandProcess:
  case kEventCommandUpdateStatus:
      break;
  default:
      return 0;
      break;
    }
    status = GetEventParameter(eventPtr->eventRef, 
      kEventParamDirectObject,
      typeHICommand, NULL, 
      sizeof(command), NULL,
      &command);
    if (status == noErr && (command.attributes & kHICommandFromMenu)) {
  if (eventPtr->eKind == kEventCommandProcess) {
      status = GetEventParameter(eventPtr->eventRef, 
        kEventParamMenuContext,
        typeUInt32, NULL, 
        sizeof(menuContext), NULL,
        &menuContext);
      if (status == noErr && (menuContext & kMenuContextMenuBar) &&
        (menuContext & kMenuContextMenuBarTracking)) {
    TkMacOSXHandleMenuSelect(GetMenuID(command.menu.menuRef),
      command.menu.menuItemIndex,
      GetCurrentEventKeyModifiers() & optionKey);
    return 1;
      }
  } else {
      Tcl_CmdInfo dummy;
      if (command.commandID == kHICommandPreferences && eventPtr->interp) {
    if (Tcl_GetCommandInfo(eventPtr->interp, 
      "::tk::mac::ShowPreferences", &dummy)) {
        if (!IsMenuItemEnabled(command.menu.menuRef, 
          command.menu.menuItemIndex)) {
      EnableMenuItem(command.menu.menuRef,
        command.menu.menuItemIndex);
        }
    } else {
        if (IsMenuItemEnabled(command.menu.menuRef, 
          command.menu.menuItemIndex)) {
      DisableMenuItem(command.menu.menuRef,
        command.menu.menuItemIndex);
        }
    }
    return 1;
      }
  }
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXReceiveAndProcessEvent --
 *
 *      This receives a carbon event and converts it to a Tk event
 *
 * Results:
 *      0 on success
 *      Mac OS error number on failure
 *
 * Side effects:
 *      This receives the next Carbon event and converts it to the
 *      appropriate Tk event
 *
 *----------------------------------------------------------------------
 */

OSStatus
TkMacOSXReceiveAndProcessEvent()
{
    static EventTargetRef targetRef = NULL;
    EventRef eventRef;
    OSStatus err;

    /*
     * This is a poll, since we have already counted the events coming
     * into this routine, and are guaranteed to have one waiting.
     */
     
    err = ReceiveNextEvent(0, NULL, kEventDurationNoWait, true, &eventRef);
    if (err == noErr) {
        if (!targetRef) {
            targetRef = GetEventDispatcherTarget();
        }
        TkMacOSXStartTclEventLoopCarbonTimer();
        err = SendEventToEventTarget(eventRef,targetRef);
        TkMacOSXStopTclEventLoopCarbonTimer();
#ifdef TK_MAC_DEBUG
        if (err != noErr && err != eventLoopTimedOutErr
                && err != eventNotHandledErr
        ) {
            char buf [256];
            fprintf(stderr,
                    "RCNE SendEventToEventTarget (%s) failed, %d\n",
                    CarbonEventToAscii(eventRef, buf), (int)err);
        }
#endif
        ReleaseEvent(eventRef);
    }
    return err;
}
