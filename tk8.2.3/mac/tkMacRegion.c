/* 
 * tkMacRegion.c --
 *
 *	Implements X window calls for manipulating regions
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkMacInt.h"
#include "X.h"
#include "Xlib.h"

#include <Windows.h>
#include <QDOffscreen.h>

/*
 * Temporary region that can be reused.
 */
static RgnHandle tmpRgn = NULL;


/*
 *----------------------------------------------------------------------
 *
 * TkCreateRegion --
 *
 *	Implements the equivelent of the X window function
 *	XCreateRegion.  See X window documentation for more details.
 *
 * Results:
 *      Returns an allocated region handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkRegion
TkCreateRegion()
{
    RgnHandle rgn;

    rgn = NewRgn();
    return (TkRegion) rgn;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDestroyRegion --
 *
 *	Implements the equivelent of the X window function
 *	XDestroyRegion.  See X window documentation for more details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */

void 
TkDestroyRegion(
    TkRegion r)
{
    RgnHandle rgn = (RgnHandle) r;

    DisposeRgn(rgn);
}

/*
 *----------------------------------------------------------------------
 *
 * TkIntersectRegion --
 *
 *	Implements the equivilent of the X window function
 *	XIntersectRegion.  See X window documentation for more details.
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
TkIntersectRegion(
    TkRegion sra,
    TkRegion srb,
    TkRegion dr_return)
{
    RgnHandle srcRgnA = (RgnHandle) sra;
    RgnHandle srcRgnB = (RgnHandle) srb;
    RgnHandle destRgn = (RgnHandle) dr_return;

    SectRgn(srcRgnA, srcRgnB, destRgn);
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnionRectWithRegion --
 *
 *	Implements the equivelent of the X window function
 *	XUnionRectWithRegion.  See X window documentation for more
 *	details.
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
TkUnionRectWithRegion(
    XRectangle* rectangle,
    TkRegion src_region,
    TkRegion dest_region_return)
{
    RgnHandle srcRgn = (RgnHandle) src_region;
    RgnHandle destRgn = (RgnHandle) dest_region_return;

    if (tmpRgn == NULL) {
	tmpRgn = NewRgn();
    }
    SetRectRgn(tmpRgn, rectangle->x, rectangle->y,
	    rectangle->x + rectangle->width, rectangle->y + rectangle->height);
    UnionRgn(srcRgn, tmpRgn, destRgn);
}

/*
 *----------------------------------------------------------------------
 *
 * TkRectInRegion --
 *
 *	Implements the equivelent of the X window function
 *	XRectInRegion.  See X window documentation for more details.
 *
 * Results:
 *	Returns one of: RectangleOut, RectangleIn, RectanglePart.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int 
TkRectInRegion(
    TkRegion region,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    RgnHandle rgn = (RgnHandle) region;
    RgnHandle rectRgn, destRgn;
    int result;
    
    rectRgn = NewRgn();
    destRgn = NewRgn();
    SetRectRgn(rectRgn, x,  y, x + width, y + height);
    SectRgn(rgn, rectRgn, destRgn);
    if (EmptyRgn(destRgn)) {
    	result = RectangleOut;
    } else if (EqualRgn(rgn, destRgn)) {
    	result = RectangleIn;
    } else {
    	result = RectanglePart;
    }
    DisposeRgn(rectRgn);
    DisposeRgn(destRgn);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkClipBox --
 *
 *	Implements the equivelent of the X window function XClipBox.
 *	See X window documentation for more details.
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
TkClipBox(
    TkRegion r,
    XRectangle* rect_return)
{
    RgnHandle rgn = (RgnHandle) r;

    rect_return->x = (**rgn).rgnBBox.left;
    rect_return->y = (**rgn).rgnBBox.top;
    rect_return->width = (**rgn).rgnBBox.right - (**rgn).rgnBBox.left;
    rect_return->height = (**rgn).rgnBBox.bottom - (**rgn).rgnBBox.top;
}
