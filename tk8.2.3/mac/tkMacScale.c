/* 
 * tkMacScale.c --
 *
 *	This file implements the Macintosh specific portion of the 
 *	scale widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkScale.h"
#include "tkInt.h"
#include <Controls.h>
#include "tkMacInt.h"

/*
 * Defines used in this file.
 */
#define slider		1110
#define inSlider	1
#define inInc		2
#define inDecr		3

/*
 * Declaration of Macintosh specific scale structure.
 */

typedef struct MacScale {
    TkScale info;		/* Generic scale info. */
    int flags;			/* Flags. */
    ControlRef scaleHandle;	/* Handle to the Scale control struct. */
} MacScale;

/*
 * Globals uses locally in this file.
 */
static ControlActionUPP scaleActionProc = NULL; /* Pointer to func. */

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		MacScaleEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static pascal void	ScaleActionProc _ANSI_ARGS_((ControlRef theControl,
			    ControlPartCode partCode));

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateScale --
 *
 *	Allocate a new TkScale structure.
 *
 * Results:
 *	Returns a newly allocated TkScale structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkScale *
TkpCreateScale(tkwin)
    Tk_Window tkwin;
{
    MacScale *macScalePtr;;
    
    macScalePtr = (MacScale *) ckalloc(sizeof(MacScale));
    macScalePtr->scaleHandle = NULL;
    if (scaleActionProc == NULL) {
	scaleActionProc = NewControlActionProc(ScaleActionProc);
    }
    
    Tk_CreateEventHandler(tkwin, ButtonPressMask,
	    MacScaleEventProc, (ClientData) macScalePtr);
	    
    return (TkScale *) macScalePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScale --
 *
 *	Free Macintosh specific resources.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The slider control is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScale(scalePtr)
    TkScale *scalePtr;
{
    MacScale *macScalePtr = (MacScale *) scalePtr;
    
    /*
     * Free Macintosh control.
     */
    if (macScalePtr->scaleHandle != NULL) {
        DisposeControl(macScalePtr->scaleHandle);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayScale --
 *
 *	This procedure is invoked as an idle handler to redisplay
 *	the contents of a scale widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The scale gets redisplayed.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayScale(clientData)
    ClientData clientData;	/* Widget record for scale. */
{
    TkScale *scalePtr = (TkScale *) clientData;
    Tk_Window tkwin = scalePtr->tkwin;
    Tcl_Interp *interp = scalePtr->interp;
    int result;
    char string[PRINT_CHARS];
    MacScale *macScalePtr = (MacScale *) clientData;
    Rect r;
    WindowRef windowRef;
    GWorldPtr destPort;        
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    MacDrawable *macDraw;
    
    if ((scalePtr->tkwin == NULL) || !Tk_IsMapped(scalePtr->tkwin)) {
	goto done;
    }

    /*
     * Invoke the scale's command if needed.
     */

    Tcl_Preserve((ClientData) scalePtr);
    if ((scalePtr->flags & INVOKE_COMMAND) && (scalePtr->commandPtr != NULL)) {
	Tcl_Preserve((ClientData) interp);
	sprintf(string, scalePtr->format, scalePtr->value);
	result = Tcl_VarEval(interp, scalePtr->commandPtr," ", string,
                             (char *) NULL);
	if (result != TCL_OK) {
	    Tcl_AddErrorInfo(interp, "\n    (command executed by scale)");
	    Tcl_BackgroundError(interp);
	}
	Tcl_Release((ClientData) interp);
    }
    scalePtr->flags &= ~INVOKE_COMMAND;
    if (scalePtr->tkwin == NULL) {
	Tcl_Release((ClientData) scalePtr);
	return;
    }
    Tcl_Release((ClientData) scalePtr);

    /*
     * Now handle the part of redisplay that is the same for
     * horizontal and vertical scales:  border and traversal
     * highlight.
     */

    if (scalePtr->highlightWidth != 0) {
	GC gc;
    
	if (scalePtr->flags & GOT_FOCUS) {
	    gc = Tk_GCForColor(scalePtr->highlightColorPtr, Tk_WindowId(tkwin));
	} else {
	    gc = Tk_GCForColor(scalePtr->highlightColorPtr, Tk_WindowId(tkwin));
	}
	Tk_DrawFocusHighlight(tkwin, gc, scalePtr->highlightWidth, Tk_WindowId(tkwin));
    }
    Tk_Draw3DRectangle(tkwin, Tk_WindowId(tkwin), scalePtr->bgBorder,
	    scalePtr->highlightWidth, scalePtr->highlightWidth,
	    Tk_Width(tkwin) - 2*scalePtr->highlightWidth,
	    Tk_Height(tkwin) - 2*scalePtr->highlightWidth,
	    scalePtr->borderWidth, scalePtr->relief);

    /*
     * Set up port for drawing Macintosh control.
     */
    macDraw = (MacDrawable *) Tk_WindowId(tkwin);
    destPort = TkMacGetDrawablePort(Tk_WindowId(tkwin));
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    TkMacSetUpClippingRgn(Tk_WindowId(tkwin));

    /*
     * Create Macintosh control.
     */
    if (macScalePtr->scaleHandle == NULL) {
        r.left = r.top = 0;
        r.right = r.bottom = 1;
        /* TODO: initial value. */
        /* 16*slider+4 */
	macScalePtr->scaleHandle = NewControl((WindowRef) destPort, 
		&r, "\p", false, (short) 35, 0, 1000,
		16*slider, (SInt32) macScalePtr);

	/*
	 * If we are foremost than make us active.
	 */
	if ((WindowPtr) destPort == FrontWindow()) {
	    macScalePtr->flags |= ACTIVE;
	}
    }
    windowRef  = (**macScalePtr->scaleHandle).contrlOwner;

    /*
     * We can't use the Macintosh commands SizeControl and MoveControl as these
     * calls will also cause a redraw which in our case will also cause
     * flicker.  To avoid this we adjust the control record directly.  The
     * Draw1Control command appears to just draw where ever the control says to
     * draw so this seems right.
     *
     * NOTE: changing the control record directly may not work when
     * Apple releases the Copland version of the MacOS in late 1996.
     */
     
    (**macScalePtr->scaleHandle).contrlRect.left = macDraw->xOff + scalePtr->inset;
    (**macScalePtr->scaleHandle).contrlRect.top = macDraw->yOff + scalePtr->inset;
    (**macScalePtr->scaleHandle).contrlRect.right = macDraw->xOff + Tk_Width(tkwin)
	- scalePtr->inset;
    (**macScalePtr->scaleHandle).contrlRect.bottom = macDraw->yOff +
	Tk_Height(tkwin) - scalePtr->inset;

    /*
     * Set the thumb and resolution etc.
     */
    (**macScalePtr->scaleHandle).contrlMin = (SInt16) scalePtr->toValue;
    (**macScalePtr->scaleHandle).contrlMax = (SInt16) scalePtr->fromValue;
    (**macScalePtr->scaleHandle).contrlValue = (SInt16) scalePtr->value;

    /*
     * Finally draw the control.
     */
    (**macScalePtr->scaleHandle).contrlVis = 255;
    (**macScalePtr->scaleHandle).contrlHilite = 0;
    Draw1Control(macScalePtr->scaleHandle);

    SetGWorld(saveWorld, saveDevice);

    done:
    scalePtr->flags &= ~REDRAW_ALL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpScaleElement --
 *
 *	Determine which part of a scale widget lies under a given
 *	point.
 *
 * Results:
 *	The return value is either TROUGH1, SLIDER, TROUGH2, or
 *	OTHER, depending on which of the scale's active elements
 *	(if any) is under the point at (x,y).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpScaleElement(scalePtr, x, y)
    TkScale *scalePtr;		/* Widget record for scale. */
    int x, y;			/* Coordinates within scalePtr's window. */
{
    MacScale *macScalePtr = (MacScale *) scalePtr;
    ControlPartCode part;
    Point where;
    Rect bounds;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;        

    destPort = TkMacGetDrawablePort(Tk_WindowId(scalePtr->tkwin));
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    /*
     * All of the calculations in this procedure mirror those in
     * DisplayScrollbar.  Be sure to keep the two consistent.
     */

    TkMacWinBounds((TkWindow *) scalePtr->tkwin, &bounds);		
    where.h = x + bounds.left;
    where.v = y + bounds.top;
    part = TestControl(macScalePtr->scaleHandle, where);
    
    SetGWorld(saveWorld, saveDevice);
    
    switch (part) {
    	case inSlider:
	    return SLIDER;
    	case inInc:
	    if (scalePtr->orient == ORIENT_VERTICAL) {
		return TROUGH1;
	    } else {
		return TROUGH2;
	    }
    	case inDecr:
	    if (scalePtr->orient == ORIENT_VERTICAL) {
		return TROUGH2;
	    } else {
		return TROUGH1;
	    }
    	default:
	    return OTHER;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpSetScaleValue --
 *
 *	This procedure changes the value of a scale and invokes
 *	a Tcl command to reflect the current position of a scale
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional error-processing
 *	command may also be invoked.  The scale's slider is redrawn.
 *
 *--------------------------------------------------------------
 */

void
TkpSetScaleValue(scalePtr, value, setVar, invokeCommand)
    register TkScale *scalePtr;	/* Info about widget. */
    double value;		/* New value for scale.  Gets adjusted
				 * if it's off the scale. */
    int setVar;			/* Non-zero means reflect new value through
				 * to associated variable, if any. */
    int invokeCommand;		/* Non-zero means invoked -command option
				 * to notify of new value, 0 means don't. */
{
    char string[PRINT_CHARS];

    value = TkRoundToResolution(scalePtr, value);
    if ((value < scalePtr->fromValue)
	    ^ (scalePtr->toValue < scalePtr->fromValue)) {
	value = scalePtr->fromValue;
    }
    if ((value > scalePtr->toValue)
	    ^ (scalePtr->toValue < scalePtr->fromValue)) {
	value = scalePtr->toValue;
    }
    if (scalePtr->flags & NEVER_SET) {
	scalePtr->flags &= ~NEVER_SET;
    } else if (scalePtr->value == value) {
	return;
    }
    scalePtr->value = value;
    if (invokeCommand) {
	scalePtr->flags |= INVOKE_COMMAND;
    }
    TkEventuallyRedrawScale(scalePtr, REDRAW_SLIDER);

    if (setVar && (scalePtr->varNamePtr != NULL)) {
	sprintf(string, scalePtr->format, scalePtr->value);
	scalePtr->flags |= SETTING_VAR;
	Tcl_ObjSetVar2(scalePtr->interp, scalePtr->varNamePtr, NULL, Tcl_NewStringObj(string, -1),
	       TCL_GLOBAL_ONLY);
	scalePtr->flags &= ~SETTING_VAR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPixelToValue --
 *
 *	Given a pixel within a scale window, return the scale
 *	reading corresponding to that pixel.
 *
 * Results:
 *	A double-precision scale reading.  If the value is outside
 *	the legal range for the scale then it's rounded to the nearest
 *	end of the scale.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double
TkpPixelToValue(scalePtr, x, y)
    register TkScale *scalePtr;		/* Information about widget. */
    int x, y;				/* Coordinates of point within
					 * window. */
{
    double value, pixelRange;

    if (scalePtr->orient == ORIENT_VERTICAL) {
	pixelRange = Tk_Height(scalePtr->tkwin) - scalePtr->sliderLength
		- 2*scalePtr->inset - 2*scalePtr->borderWidth;
	value = y;
    } else {
	pixelRange = Tk_Width(scalePtr->tkwin) - scalePtr->sliderLength
		- 2*scalePtr->inset - 2*scalePtr->borderWidth;
	value = x;
    }

    if (pixelRange <= 0) {
	/*
	 * Not enough room for the slider to actually slide:  just return
	 * the scale's current value.
	 */

	return scalePtr->value;
    }
    value -= scalePtr->sliderLength/2 + scalePtr->inset
		+ scalePtr->borderWidth;
    value /= pixelRange;
    if (value < 0) {
	value = 0;
    }
    if (value > 1) {
	value = 1;
    }
    value = scalePtr->fromValue +
		value * (scalePtr->toValue - scalePtr->fromValue);
    return TkRoundToResolution(scalePtr, value);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpValueToPixel --
 *
 *	Given a reading of the scale, return the x-coordinate or
 *	y-coordinate corresponding to that reading, depending on
 *	whether the scale is vertical or horizontal, respectively.
 *
 * Results:
 *	An integer value giving the pixel location corresponding
 *	to reading.  The value is restricted to lie within the
 *	defined range for the scale.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpValueToPixel(scalePtr, value)
    register TkScale *scalePtr;		/* Information about widget. */
    double value;			/* Reading of the widget. */
{
    int y, pixelRange;
    double valueRange;

    valueRange = scalePtr->toValue - scalePtr->fromValue;
    pixelRange = (scalePtr->orient == ORIENT_VERTICAL ? Tk_Height(scalePtr->tkwin)
	    : Tk_Width(scalePtr->tkwin)) - scalePtr->sliderLength
	    - 2*scalePtr->inset - 2*scalePtr->borderWidth;
    if (valueRange == 0) {
	y = 0;
    } else {
	y = (int) ((value - scalePtr->fromValue) * pixelRange
		  / valueRange + 0.5);
	if (y < 0) {
	    y = 0;
	} else if (y > pixelRange) {
	    y = pixelRange;
	}
    }
    y += scalePtr->sliderLength/2 + scalePtr->inset + scalePtr->borderWidth;
    return y;
}

/*
 *--------------------------------------------------------------
 *
 * MacScaleEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for 
 *	ButtonPress events on scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
MacScaleEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    MacScale *macScalePtr = (MacScale *) clientData;
    Point where;
    Rect bounds;
    int part, x, y, dummy;
    unsigned int state;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Window dummyWin;

    /*
     * To call Macintosh control routines we must have the port
     * set to the window containing the control.  We will then test
     * which part of the control was hit and act accordingly.
     */
    destPort = TkMacGetDrawablePort(Tk_WindowId(macScalePtr->info.tkwin));
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    TkMacSetUpClippingRgn(Tk_WindowId(macScalePtr->info.tkwin));

    TkMacWinBounds((TkWindow *) macScalePtr->info.tkwin, &bounds);		
    where.h = eventPtr->xbutton.x + bounds.left;
    where.v = eventPtr->xbutton.y + bounds.top;
    part = TestControl(macScalePtr->scaleHandle, where);
    if (part == 0) {
	return;
    }
    
    part = TrackControl(macScalePtr->scaleHandle, where, scaleActionProc);
    
    /*
     * Update the value for the widget.
     */
    macScalePtr->info.value = (**macScalePtr->scaleHandle).contrlValue;
    /* TkpSetScaleValue(&macScalePtr->info, macScalePtr->info.value, 1, 0); */

    /*
     * The TrackControl call will "eat" the ButtonUp event.  We now
     * generate a ButtonUp event so Tk will unset implicit grabs etc.
     */
    GetMouse(&where);
    XQueryPointer(NULL, None, &dummyWin, &dummyWin, &x,
	&y, &dummy, &dummy, &state);
    TkGenerateButtonEvent(x, y, Tk_WindowId(macScalePtr->info.tkwin), state);

    SetGWorld(saveWorld, saveDevice);
}

/*
 *--------------------------------------------------------------
 *
 * ScaleActionProc --
 *
 *	Callback procedure used by the Macintosh toolbox call
 *	TrackControl.  This call will update the display while
 *	the scrollbar is being manipulated by the user.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change the display.
 *
 *--------------------------------------------------------------
 */

static pascal void
ScaleActionProc(ControlRef theControl, ControlPartCode partCode)
    /* ControlRef theControl;	/* Handle to scrollbat control */
    /* ControlPartCode partCode;	/* Part of scrollbar that was "hit" */
{
    register int value;
    register TkScale *scalePtr = (TkScale *) GetCRefCon(theControl);

    value = (**theControl).contrlValue;
    TkpSetScaleValue(scalePtr, value, 1, 1);
    Tcl_Preserve((ClientData) scalePtr);
    Tcl_DoOneEvent(TCL_IDLE_EVENTS);
    Tcl_Release((ClientData) scalePtr);
}

