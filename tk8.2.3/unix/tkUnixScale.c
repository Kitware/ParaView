/* 
 * tkUnixScale.c --
 *
 *	This file implements the X specific portion of the scrollbar
 *	widget.
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

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		DisplayHorizontalScale _ANSI_ARGS_((TkScale *scalePtr,
			    Drawable drawable, XRectangle *drawnAreaPtr));
static void		DisplayHorizontalValue _ANSI_ARGS_((TkScale *scalePtr,
			    Drawable drawable, double value, int top));
static void		DisplayVerticalScale _ANSI_ARGS_((TkScale *scalePtr,
			    Drawable drawable, XRectangle *drawnAreaPtr));
static void		DisplayVerticalValue _ANSI_ARGS_((TkScale *scalePtr,
			    Drawable drawable, double value, int rightEdge));

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
    return (TkScale *) ckalloc(sizeof(TkScale));
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScale --
 *
 *	Destroy a TkScale structure.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScale(scalePtr)
    TkScale *scalePtr;
{
    ckfree((char *) scalePtr);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayVerticalScale --
 *
 *	This procedure redraws the contents of a vertical scale
 *	window.  It is invoked as a do-when-idle handler, so it only
 *	runs when there's nothing else for the application to do.
 *
 * Results:
 *	There is no return value.  If only a part of the scale needs
 *	to be redrawn, then drawnAreaPtr is modified to reflect the
 *	area that was actually modified.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayVerticalScale(scalePtr, drawable, drawnAreaPtr)
    TkScale *scalePtr;			/* Widget record for scale. */
    Drawable drawable;			/* Where to display scale (window
					 * or pixmap). */
    XRectangle *drawnAreaPtr;		/* Initally contains area of window;
					 * if only a part of the scale is
					 * redrawn, gets modified to reflect
					 * the part of the window that was
					 * redrawn. */
{
    Tk_Window tkwin = scalePtr->tkwin;
    int x, y, width, height, shadowWidth;
    double tickValue;
    Tk_3DBorder sliderBorder;

    /*
     * Display the information from left to right across the window.
     */

    if (!(scalePtr->flags & REDRAW_OTHER)) {
	drawnAreaPtr->x = scalePtr->vertTickRightX;
	drawnAreaPtr->y = scalePtr->inset;
	drawnAreaPtr->width = scalePtr->vertTroughX + scalePtr->width
		+ 2*scalePtr->borderWidth - scalePtr->vertTickRightX;
	drawnAreaPtr->height -= 2*scalePtr->inset;
    }
    Tk_Fill3DRectangle(tkwin, drawable, scalePtr->bgBorder,
	    drawnAreaPtr->x, drawnAreaPtr->y, drawnAreaPtr->width,
	    drawnAreaPtr->height, 0, TK_RELIEF_FLAT);
    if (scalePtr->flags & REDRAW_OTHER) {
	/*
	 * Display the tick marks.
	 */

	if (scalePtr->tickInterval != 0) {
	    for (tickValue = scalePtr->fromValue; ;
		    tickValue += scalePtr->tickInterval) {
		/*
		 * The TkRoundToResolution call gets rid of accumulated
		 * round-off errors, if any.
		 */

		tickValue = TkRoundToResolution(scalePtr, tickValue);
		if (scalePtr->toValue >= scalePtr->fromValue) {
		    if (tickValue > scalePtr->toValue) {
			break;
		    }
		} else {
		    if (tickValue < scalePtr->toValue) {
			break;
		    }
		}
		DisplayVerticalValue(scalePtr, drawable, tickValue,
			scalePtr->vertTickRightX);
	    }
	}
    }

    /*
     * Display the value, if it is desired.
     */

    if (scalePtr->showValue) {
	DisplayVerticalValue(scalePtr, drawable, scalePtr->value,
		scalePtr->vertValueRightX);
    }

    /*
     * Display the trough and the slider.
     */

    Tk_Draw3DRectangle(tkwin, drawable,
	    scalePtr->bgBorder, scalePtr->vertTroughX, scalePtr->inset,
	    scalePtr->width + 2*scalePtr->borderWidth,
	    Tk_Height(tkwin) - 2*scalePtr->inset, scalePtr->borderWidth,
	    TK_RELIEF_SUNKEN);
    XFillRectangle(scalePtr->display, drawable, scalePtr->troughGC,
	    scalePtr->vertTroughX + scalePtr->borderWidth,
	    scalePtr->inset + scalePtr->borderWidth,
	    (unsigned) scalePtr->width,
	    (unsigned) (Tk_Height(tkwin) - 2*scalePtr->inset
		- 2*scalePtr->borderWidth));
    if (scalePtr->state == STATE_ACTIVE) {
	sliderBorder = scalePtr->activeBorder;
    } else {
	sliderBorder = scalePtr->bgBorder;
    }
    width = scalePtr->width;
    height = scalePtr->sliderLength/2;
    x = scalePtr->vertTroughX + scalePtr->borderWidth;
    y = TkpValueToPixel(scalePtr, scalePtr->value) - height;
    shadowWidth = scalePtr->borderWidth/2;
    if (shadowWidth == 0) {
	shadowWidth = 1;
    }
    Tk_Draw3DRectangle(tkwin, drawable, sliderBorder, x, y, width,
	    2*height, shadowWidth, scalePtr->sliderRelief);
    x += shadowWidth;
    y += shadowWidth;
    width -= 2*shadowWidth;
    height -= shadowWidth;
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x, y, width,
	    height, shadowWidth, scalePtr->sliderRelief);
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x, y+height,
	    width, height, shadowWidth, scalePtr->sliderRelief);

    /*
     * Draw the label to the right of the scale.
     */

    if ((scalePtr->flags & REDRAW_OTHER) && (scalePtr->labelLength != 0)) {
	Tk_FontMetrics fm;

	Tk_GetFontMetrics(scalePtr->tkfont, &fm);
	Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
		scalePtr->tkfont, Tcl_GetString(scalePtr->labelPtr), 
                scalePtr->labelLength, scalePtr->vertLabelX,
                scalePtr->inset + (3*fm.ascent)/2);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayVerticalValue --
 *
 *	This procedure is called to display values (scale readings)
 *	for vertically-oriented scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The numerical value corresponding to value is displayed with
 *	its right edge at "rightEdge", and at a vertical position in
 *	the scale that corresponds to "value".
 *
 *----------------------------------------------------------------------
 */

static void
DisplayVerticalValue(scalePtr, drawable, value, rightEdge)
    register TkScale *scalePtr;	/* Information about widget in which to
				 * display value. */
    Drawable drawable;		/* Pixmap or window in which to draw
				 * the value. */
    double value;		/* Y-coordinate of number to display,
				 * specified in application coords, not
				 * in pixels (we'll compute pixels). */
    int rightEdge;		/* X-coordinate of right edge of text,
				 * specified in pixels. */
{
    register Tk_Window tkwin = scalePtr->tkwin;
    int y, width, length;
    char valueString[PRINT_CHARS];
    Tk_FontMetrics fm;

    Tk_GetFontMetrics(scalePtr->tkfont, &fm);
    y = TkpValueToPixel(scalePtr, value) + fm.ascent/2;
    sprintf(valueString, scalePtr->format, value);
    length = strlen(valueString);
    width = Tk_TextWidth(scalePtr->tkfont, valueString, length);

    /*
     * Adjust the y-coordinate if necessary to keep the text entirely
     * inside the window.
     */

    if ((y - fm.ascent) < (scalePtr->inset + SPACING)) {
	y = scalePtr->inset + SPACING + fm.ascent;
    }
    if ((y + fm.descent) > (Tk_Height(tkwin) - scalePtr->inset - SPACING)) {
	y = Tk_Height(tkwin) - scalePtr->inset - SPACING - fm.descent;
    }
    Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
	    scalePtr->tkfont, valueString, length, rightEdge - width, y);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayHorizontalScale --
 *
 *	This procedure redraws the contents of a horizontal scale
 *	window.  It is invoked as a do-when-idle handler, so it only
 *	runs when there's nothing else for the application to do.
 *
 * Results:
 *	There is no return value.  If only a part of the scale needs
 *	to be redrawn, then drawnAreaPtr is modified to reflect the
 *	area that was actually modified.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayHorizontalScale(scalePtr, drawable, drawnAreaPtr)
    TkScale *scalePtr;			/* Widget record for scale. */
    Drawable drawable;			/* Where to display scale (window
					 * or pixmap). */
    XRectangle *drawnAreaPtr;		/* Initally contains area of window;
					 * if only a part of the scale is
					 * redrawn, gets modified to reflect
					 * the part of the window that was
					 * redrawn. */
{
    register Tk_Window tkwin = scalePtr->tkwin;
    int x, y, width, height, shadowWidth;
    double tickValue;
    Tk_3DBorder sliderBorder;

    /*
     * Display the information from bottom to top across the window.
     */

    if (!(scalePtr->flags & REDRAW_OTHER)) {
	drawnAreaPtr->x = scalePtr->inset;
	drawnAreaPtr->y = scalePtr->horizValueY;
	drawnAreaPtr->width -= 2*scalePtr->inset;
	drawnAreaPtr->height = scalePtr->horizTroughY + scalePtr->width
		+ 2*scalePtr->borderWidth - scalePtr->horizValueY;
    }
    Tk_Fill3DRectangle(tkwin, drawable, scalePtr->bgBorder,
	    drawnAreaPtr->x, drawnAreaPtr->y, drawnAreaPtr->width,
	    drawnAreaPtr->height, 0, TK_RELIEF_FLAT);
    if (scalePtr->flags & REDRAW_OTHER) {
	/*
	 * Display the tick marks.
	 */

	if (scalePtr->tickInterval != 0) {
	    for (tickValue = scalePtr->fromValue; ;
		    tickValue += scalePtr->tickInterval) {
		/*
		 * The TkRoundToResolution call gets rid of accumulated
		 * round-off errors, if any.
		 */

		tickValue = TkRoundToResolution(scalePtr, tickValue);
		if (scalePtr->toValue >= scalePtr->fromValue) {
		    if (tickValue > scalePtr->toValue) {
			break;
		    }
		} else {
		    if (tickValue < scalePtr->toValue) {
			break;
		    }
		}
		DisplayHorizontalValue(scalePtr, drawable, tickValue,
			scalePtr->horizTickY);
	    }
	}
    }

    /*
     * Display the value, if it is desired.
     */

    if (scalePtr->showValue) {
	DisplayHorizontalValue(scalePtr, drawable, scalePtr->value,
		scalePtr->horizValueY);
    }

    /*
     * Display the trough and the slider.
     */

    y = scalePtr->horizTroughY;
    Tk_Draw3DRectangle(tkwin, drawable,
	    scalePtr->bgBorder, scalePtr->inset, y,
	    Tk_Width(tkwin) - 2*scalePtr->inset,
	    scalePtr->width + 2*scalePtr->borderWidth,
	    scalePtr->borderWidth, TK_RELIEF_SUNKEN);
    XFillRectangle(scalePtr->display, drawable, scalePtr->troughGC,
	    scalePtr->inset + scalePtr->borderWidth,
	    y + scalePtr->borderWidth,
	    (unsigned) (Tk_Width(tkwin) - 2*scalePtr->inset
		- 2*scalePtr->borderWidth),
	    (unsigned) scalePtr->width);
    if (scalePtr->state == STATE_ACTIVE) {
	sliderBorder = scalePtr->activeBorder;
    } else {
	sliderBorder = scalePtr->bgBorder;
    }
    width = scalePtr->sliderLength/2;
    height = scalePtr->width;
    x = TkpValueToPixel(scalePtr, scalePtr->value) - width;
    y += scalePtr->borderWidth;
    shadowWidth = scalePtr->borderWidth/2;
    if (shadowWidth == 0) {
	shadowWidth = 1;
    }
    Tk_Draw3DRectangle(tkwin, drawable, sliderBorder,
	    x, y, 2*width, height, shadowWidth, scalePtr->sliderRelief);
    x += shadowWidth;
    y += shadowWidth;
    width -= shadowWidth;
    height -= 2*shadowWidth;
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x, y, width, height,
	    shadowWidth, scalePtr->sliderRelief);
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x+width, y,
	    width, height, shadowWidth, scalePtr->sliderRelief);

    /*
     * Draw the label at the top of the scale.
     */

    if ((scalePtr->flags & REDRAW_OTHER) && (scalePtr->labelLength != 0)) {
	Tk_FontMetrics fm;

	Tk_GetFontMetrics(scalePtr->tkfont, &fm);
	Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
		scalePtr->tkfont, Tcl_GetString(scalePtr->labelPtr), 
                scalePtr->labelLength, scalePtr->inset + fm.ascent/2, 
                scalePtr->horizLabelY + fm.ascent);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayHorizontalValue --
 *
 *	This procedure is called to display values (scale readings)
 *	for horizontally-oriented scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The numerical value corresponding to value is displayed with
 *	its bottom edge at "bottom", and at a horizontal position in
 *	the scale that corresponds to "value".
 *
 *----------------------------------------------------------------------
 */

static void
DisplayHorizontalValue(scalePtr, drawable, value, top)
    register TkScale *scalePtr;	/* Information about widget in which to
				 * display value. */
    Drawable drawable;		/* Pixmap or window in which to draw
				 * the value. */
    double value;		/* X-coordinate of number to display,
				 * specified in application coords, not
				 * in pixels (we'll compute pixels). */
    int top;			/* Y-coordinate of top edge of text,
				 * specified in pixels. */
{
    register Tk_Window tkwin = scalePtr->tkwin;
    int x, y, length, width;
    char valueString[PRINT_CHARS];
    Tk_FontMetrics fm;

    x = TkpValueToPixel(scalePtr, value);
    Tk_GetFontMetrics(scalePtr->tkfont, &fm);
    y = top + fm.ascent;
    sprintf(valueString, scalePtr->format, value);
    length = strlen(valueString);
    width = Tk_TextWidth(scalePtr->tkfont, valueString, length);

    /*
     * Adjust the x-coordinate if necessary to keep the text entirely
     * inside the window.
     */

    x -= (width)/2;
    if (x < (scalePtr->inset + SPACING)) {
	x = scalePtr->inset + SPACING;
    }
    if (x > (Tk_Width(tkwin) - scalePtr->inset)) {
	x = Tk_Width(tkwin) - scalePtr->inset - SPACING - width;
    }
    Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
	    scalePtr->tkfont, valueString, length, x, y);
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
    Pixmap pixmap;
    int result;
    char string[PRINT_CHARS];
    XRectangle drawnArea;

    if ((scalePtr->tkwin == NULL) || !Tk_IsMapped(scalePtr->tkwin)) {
	goto done;
    }

    /*
     * Invoke the scale's command if needed.
     */

    Tcl_Preserve((ClientData) scalePtr);
    Tcl_Preserve((ClientData) interp);
    if ((scalePtr->flags & INVOKE_COMMAND) 
            && (scalePtr->commandPtr != NULL)) {
	sprintf(string, scalePtr->format, scalePtr->value);

	result = Tcl_VarEval(interp, Tcl_GetString(scalePtr->commandPtr),
                " ", string, (char *) NULL);
	if (result != TCL_OK) {
	    Tcl_AddErrorInfo(interp, "\n    (command executed by scale)");
	    Tcl_BackgroundError(interp);
	}
    }
    Tcl_Release((ClientData) interp);
    scalePtr->flags &= ~INVOKE_COMMAND;
    if (scalePtr->tkwin == NULL) {
	Tcl_Release((ClientData) scalePtr);
	return;
    }
    Tcl_Release((ClientData) scalePtr);

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the scale in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    pixmap = Tk_GetPixmap(scalePtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
    drawnArea.x = 0;
    drawnArea.y = 0;
    drawnArea.width = Tk_Width(tkwin);
    drawnArea.height = Tk_Height(tkwin);

    /*
     * Much of the redisplay is done totally differently for
     * horizontal and vertical scales.  Handle the part that's
     * different.
     */

    if (scalePtr->orient == ORIENT_VERTICAL) {
	DisplayVerticalScale(scalePtr, pixmap, &drawnArea);
    } else {
	DisplayHorizontalScale(scalePtr, pixmap, &drawnArea);
    }

    /*
     * Now handle the part of redisplay that is the same for
     * horizontal and vertical scales:  border and traversal
     * highlight.
     */

    if (scalePtr->flags & REDRAW_OTHER) {
	if (scalePtr->relief != TK_RELIEF_FLAT) {
	    Tk_Draw3DRectangle(tkwin, pixmap, scalePtr->bgBorder,
		    scalePtr->highlightWidth, scalePtr->highlightWidth,
		    Tk_Width(tkwin) - 2*scalePtr->highlightWidth,
		    Tk_Height(tkwin) - 2*scalePtr->highlightWidth,
		    scalePtr->borderWidth, scalePtr->relief);
	}
	if (scalePtr->highlightWidth != 0) {
	    GC gc;
    
	    if (scalePtr->flags & GOT_FOCUS) {
		gc = Tk_GCForColor(scalePtr->highlightColorPtr, pixmap);
	    } else {
		gc = Tk_GCForColor(
                        Tk_3DBorderColor(scalePtr->highlightBorder), pixmap);
	    }
	    Tk_DrawFocusHighlight(tkwin, gc, scalePtr->highlightWidth, pixmap);
	}
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */

    XCopyArea(scalePtr->display, pixmap, Tk_WindowId(tkwin),
	    scalePtr->copyGC, drawnArea.x, drawnArea.y, drawnArea.width,
	    drawnArea.height, drawnArea.x, drawnArea.y);
    Tk_FreePixmap(scalePtr->display, pixmap);

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
    int sliderFirst;

    if (scalePtr->orient == ORIENT_VERTICAL) {
	if ((x < scalePtr->vertTroughX)
		|| (x >= (scalePtr->vertTroughX + 2*scalePtr->borderWidth +
		scalePtr->width))) {
	    return OTHER;
	}
	if ((y < scalePtr->inset)
		|| (y >= (Tk_Height(scalePtr->tkwin) - scalePtr->inset))) {
	    return OTHER;
	}
	sliderFirst = TkpValueToPixel(scalePtr, scalePtr->value)
		- scalePtr->sliderLength/2;
	if (y < sliderFirst) {
	    return TROUGH1;
	}
	if (y < (sliderFirst+scalePtr->sliderLength)) {
	    return SLIDER;
	}
	return TROUGH2;
    }

    if ((y < scalePtr->horizTroughY)
	    || (y >= (scalePtr->horizTroughY + 2*scalePtr->borderWidth +
	    scalePtr->width))) {
	return OTHER;
    }
    if ((x < scalePtr->inset)
	    || (x >= (Tk_Width(scalePtr->tkwin) - scalePtr->inset))) {
	return OTHER;
    }
    sliderFirst = TkpValueToPixel(scalePtr, scalePtr->value)
	    - scalePtr->sliderLength/2;
    if (x < sliderFirst) {
	return TROUGH1;
    }
    if (x < (sliderFirst+scalePtr->sliderLength)) {
	return SLIDER;
    }
    return TROUGH2;
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
	Tcl_SetVar(scalePtr->interp, Tcl_GetString(scalePtr->varNamePtr), 
	        string, TCL_GLOBAL_ONLY);
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
    pixelRange = (scalePtr->orient == ORIENT_VERTICAL 
            ? Tk_Height(scalePtr->tkwin)
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
