/* 
 * tclMacPanic.c --
 *
 *	Source code for the "Tcl_Panic" library procedure used in "Simple
 *	Shell";	other Mac applications will probably call Tcl_SetPanicProc
 *	to set a more robust application-specific panic procedure.
 *
 * Copyright (c) 1993-1994 Lockheed Missle & Space Company, AI Center
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */


#include <Events.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Fonts.h>
#include <Dialogs.h>
#include <Icons.h>
#include <Sound.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "tclInt.h"
#include "tclMacInt.h"

/*
 * constants for panic dialog
 */
#define PANICHEIGHT 150				/* Height of dialog */
#define PANICWIDTH 350				/* Width of dialog */
#define PANIC_BUTTON_RECT {125, 260, 145, 335}	/* Rect for button. */
#define PANIC_ICON_RECT   {10, 20, 42, 52}	/* Rect for icon. */
#define PANIC_TEXT_RECT   {10, 65, 140, 330}	/* Rect for text. */
#define	ENTERCODE  (0x03)
#define	RETURNCODE (0x0D)


/*
 *----------------------------------------------------------------------
 *
 * TclpPanic --
 *
 *	Displays panic info, then aborts
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

        /* VARARGS ARGSUSED */
void
TclpPanic TCL_VARARGS_DEF(CONST char *, format)
{
    va_list varg;
    char msg[256];
    WindowRef macWinPtr, foundWinPtr;
    Rect macRect;
    Rect buttonRect = PANIC_BUTTON_RECT;
    Rect iconRect = PANIC_ICON_RECT;
    Rect textRect = PANIC_TEXT_RECT;
    ControlHandle okButtonHandle;
    EventRecord event;
    Handle stopIconHandle;
    int	part;
    Boolean done = false;

    va_start(varg, format);
    vsprintf(msg, format, varg);
    va_end(varg);

    /*
     * Put up an alert without using the Resource Manager (there may 
     * be no resources to load). Use the Window and Control Managers instead.
     * We want the window centered on the main monitor. The following 
     * should be tested with multiple monitors. Look and see if there is a way
     * not using qd.screenBits.
     */
 
    macRect.top = (qd.screenBits.bounds.top + qd.screenBits.bounds.bottom)
	/ 2 - (PANICHEIGHT / 2);
    macRect.bottom = (qd.screenBits.bounds.top + qd.screenBits.bounds.bottom)
	/ 2 + (PANICHEIGHT / 2);
    macRect.left = (qd.screenBits.bounds.left + qd.screenBits.bounds.right)
	/ 2 - (PANICWIDTH / 2);
    macRect.right = (qd.screenBits.bounds.left + qd.screenBits.bounds.right)
	/ 2 + (PANICWIDTH / 2);
    
    macWinPtr = NewWindow(NULL, &macRect, "\p", true, dBoxProc, (WindowRef) -1,
            false, 0);
    if (macWinPtr == NULL) {
	goto exitNow;
    }
    
    okButtonHandle = NewControl(macWinPtr, &buttonRect, "\pOK", true,
	    0, 0, 1, pushButProc, 0);
    if (okButtonHandle == NULL) {
	CloseWindow(macWinPtr);
	goto exitNow;
    }
    
    SelectWindow(macWinPtr);
    SetCursor(&qd.arrow);
    stopIconHandle = GetIcon(kStopIcon);
            
    while (!done) {
	if (WaitNextEvent(mDownMask | keyDownMask | updateMask,
		&event, 0, NULL)) {
	    switch(event.what) {
		case mouseDown:
		    part = FindWindow(event.where, &foundWinPtr);
    
		    if ((foundWinPtr != macWinPtr) || (part != inContent)) {
		    	SysBeep(1);
		    } else {
		    	SetPortWindowPort(macWinPtr);
		    	GlobalToLocal(&event.where);
		    	part = FindControl(event.where, macWinPtr,
				&okButtonHandle);
    	
			if ((kControlButtonPart == part) && 
				(TrackControl(okButtonHandle,
					event.where, NULL))) {
			    done = true;
			}
		    }
		    break;
		case keyDown:
		    switch (event.message & charCodeMask) {
			case ENTERCODE:
			case RETURNCODE:
			    HiliteControl(okButtonHandle, 1);
			    HiliteControl(okButtonHandle, 0);
			    done = true;
		    }
		    break;
		case updateEvt:   
		    SetPortWindowPort(macWinPtr);
		    TextFont(systemFont);
		    
		    BeginUpdate(macWinPtr);
		    if (stopIconHandle != NULL) {
			PlotIcon(&iconRect, stopIconHandle);
		    }
		    TETextBox(msg, strlen(msg), &textRect, teFlushDefault);
		    DrawControls(macWinPtr);
		    EndUpdate(macWinPtr);
	    }
	}
    }

    CloseWindow(macWinPtr);

  exitNow:
#ifdef TCL_DEBUG
    Debugger();
#else
    abort();
#endif
}

