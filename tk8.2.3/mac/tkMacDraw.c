/* 
 * tkMacDraw.c --
 *
 *	This file contains functions that preform drawing to
 *	Xlib windows.  Most of the functions simple emulate
 *	Xlib functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "X.h"
#include "Xlib.h"
#include <stdio.h>
#include <tcl.h>

#include <Windows.h>
#include <Fonts.h>
#include <QDOffscreen.h>
#include "tkMacInt.h"
#include "tkPort.h"

#ifndef PI
#    define PI 3.14159265358979323846
#endif

/*
 * Temporary regions that can be reused.
 */
static RgnHandle tmpRgn = NULL;
static RgnHandle tmpRgn2 = NULL;

static PixPatHandle gPenPat = NULL;

/*
 * Prototypes for functions used only in this file.
 */
static unsigned char    InvertByte _ANSI_ARGS_((unsigned char data));

/*
 *----------------------------------------------------------------------
 *
 * XCopyArea --
 *
 *	Copies data from one drawable to another using block transfer
 *	routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data is moved from a window or bitmap to a second window or
 *	bitmap.
 *
 *----------------------------------------------------------------------
 */

void 
XCopyArea(
    Display* display,		/* Display. */
    Drawable src,		/* Source drawable. */
    Drawable dest,		/* Destination drawable. */
    GC gc,			/* GC to use. */
    int src_x,			/* X & Y, width & height */
    int src_y,			/* define the source rectangle */
    unsigned int width,		/* the will be copied. */
    unsigned int height,
    int dest_x,			/* Dest X & Y on dest rect. */
    int dest_y)
{
    Rect srcRect, destRect;
    BitMapPtr srcBit, destBit;
    MacDrawable *srcDraw = (MacDrawable *) src;
    MacDrawable *destDraw = (MacDrawable *) dest;
    GWorldPtr srcPort, destPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    short tmode;
    RGBColor origForeColor, origBackColor, whiteColor, blackColor;

    destPort = TkMacGetDrawablePort(dest);
    srcPort = TkMacGetDrawablePort(src);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    GetForeColor(&origForeColor);
    GetBackColor(&origBackColor);
    whiteColor.red = 0;
    whiteColor.blue = 0;
    whiteColor.green = 0;
    RGBForeColor(&whiteColor);
    blackColor.red = 0xFFFF;
    blackColor.blue = 0xFFFF;
    blackColor.green = 0xFFFF;
    RGBBackColor(&blackColor);
    

    TkMacSetUpClippingRgn(dest);
    
    /*
     *  We will change the clip rgn in this routine, so we need to 
     *  be able to restore it when we exit.
     */
     
    if (tmpRgn2 == NULL) {
        tmpRgn2 = NewRgn();
    }
    GetClip(tmpRgn2);

    if (((TkpClipMask*)gc->clip_mask)->type == TKP_CLIP_REGION) {
	RgnHandle clipRgn = (RgnHandle)
	        ((TkpClipMask*)gc->clip_mask)->value.region;
	
	int xOffset, yOffset;
	
	if (tmpRgn == NULL) {
	    tmpRgn = NewRgn();
	}
	
	xOffset = destDraw->xOff + gc->clip_x_origin;
	yOffset = destDraw->yOff + gc->clip_y_origin;
	
	OffsetRgn(clipRgn, xOffset, yOffset);
	
	GetClip(tmpRgn);
	SectRgn(tmpRgn, clipRgn, tmpRgn);
	
	SetClip(tmpRgn);
	
	OffsetRgn(clipRgn, -xOffset, -yOffset);
    }
    
    srcBit = &((GrafPtr) srcPort)->portBits;
    destBit = &((GrafPtr) destPort)->portBits;
    SetRect(&srcRect, (short) (srcDraw->xOff + src_x),
	    (short) (srcDraw->yOff + src_y),
	    (short) (srcDraw->xOff + src_x + width),
	    (short) (srcDraw->yOff + src_y + height));	
    SetRect(&destRect, (short) (destDraw->xOff + dest_x),
	    (short) (destDraw->yOff + dest_y), 
	    (short) (destDraw->xOff + dest_x + width),
	    (short) (destDraw->yOff + dest_y + height));	
    tmode = srcCopy;

    CopyBits(srcBit, destBit, &srcRect, &destRect, tmode, NULL);
    RGBForeColor(&origForeColor);
    RGBBackColor(&origBackColor);
    SetClip(tmpRgn2);
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XCopyPlane --
 *
 *	Copies a bitmap from a source drawable to a destination
 *	drawable.  The plane argument specifies which bit plane of
 *	the source contains the bitmap.  Note that this implementation
 *	ignores the gc->function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the destination drawable.
 *
 *----------------------------------------------------------------------
 */

void
XCopyPlane(
    Display* display,		/* Display. */
    Drawable src,		/* Source drawable. */
    Drawable dest,		/* Destination drawable. */
    GC gc,			/* The GC to use. */
    int src_x,			/* X, Y, width & height */
    int src_y,			/* define the source rect. */
    unsigned int width,
    unsigned int height,
    int dest_x,			/* X & Y on dest where we will copy. */
    int dest_y,
    unsigned long plane)	/* Which plane to copy. */
{
    Rect srcRect, destRect;
    BitMapPtr srcBit, destBit, maskBit;
    MacDrawable *srcDraw = (MacDrawable *) src;
    MacDrawable *destDraw = (MacDrawable *) dest;
    GWorldPtr srcPort, destPort, maskPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    RGBColor macColor; 
    TkpClipMask *clipPtr = (TkpClipMask*)gc->clip_mask;
    short tmode;

    destPort = TkMacGetDrawablePort(dest);
    srcPort = TkMacGetDrawablePort(src);
    
    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(dest);

    srcBit = &((GrafPtr) srcPort)->portBits;
    destBit = &((GrafPtr) destPort)->portBits;
    SetRect(&srcRect, (short) (srcDraw->xOff + src_x),
	    (short) (srcDraw->yOff + src_y),
	    (short) (srcDraw->xOff + src_x + width),
	    (short) (srcDraw->yOff + src_y + height));
    SetRect(&destRect, (short) (destDraw->xOff + dest_x),
	    (short) (destDraw->yOff + dest_y), 
	    (short) (destDraw->xOff + dest_x + width),
	    (short) (destDraw->yOff + dest_y + height));
    tmode = srcOr;
    tmode = srcCopy + transparent;

    if (TkSetMacColor(gc->foreground, &macColor) == true) {
	RGBForeColor(&macColor);
    }

    if (clipPtr == NULL || clipPtr->type == TKP_CLIP_REGION) {

	/*
	 * Case 1: opaque bitmaps.
	 */

	TkSetMacColor(gc->background, &macColor);
	RGBBackColor(&macColor);
	tmode = srcCopy;
	CopyBits(srcBit, destBit, &srcRect, &destRect, tmode, NULL);
    } else if (clipPtr->type == TKP_CLIP_PIXMAP) {
	if (clipPtr->value.pixmap == src) {
	    /*
	     * Case 2: transparent bitmaps.  If it's color we ignore
	     * the forecolor.
	     */
	    if ((**(srcPort->portPixMap)).pixelSize == 1) {
		tmode = srcOr;
	    } else {
		tmode = transparent;
	    }
	    CopyBits(srcBit, destBit, &srcRect, &destRect, tmode, NULL);
	} else {
	    /*
	     * Case 3: two arbitrary bitmaps.	 
	     */
	    tmode = srcCopy;
	    maskPort = TkMacGetDrawablePort(clipPtr->value.pixmap);
	    maskBit = &((GrafPtr) maskPort)->portBits;
	    CopyDeepMask(srcBit, maskBit, destBit, &srcRect, &srcRect, &destRect, tmode, NULL);
	}
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * TkPutImage --
 *
 *	Copies a subimage from an in-memory image to a rectangle of
 *	of the specified drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws the image on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
TkPutImage(
    unsigned long *colors,	/* Unused on Macintosh. */
    int ncolors,		/* Unused on Macintosh. */
    Display* display,		/* Display. */
    Drawable d,			/* Drawable to place image on. */
    GC gc,			/* GC to use. */
    XImage* image,		/* Image to place. */
    int src_x,			/* Source X & Y. */
    int src_y,
    int dest_x,			/* Destination X & Y. */
    int dest_y,
    unsigned int width,		/* Same width & height for both */
    unsigned int height)	/* distination and source. */
{
    MacDrawable *destDraw = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    int i, j;
    BitMap bitmap;
    char *newData = NULL;
    Rect destRect, srcRect;

    destPort = TkMacGetDrawablePort(d);
    SetRect(&destRect, dest_x, dest_y, dest_x + width, dest_y + height);
    SetRect(&srcRect, src_x, src_y, src_x + width, src_y + height);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(d);

    if (image->depth == 1) {

	/* 
	 * This code assumes a pixel depth of 1 
	 */

	bitmap.bounds.top = bitmap.bounds.left = 0;
	bitmap.bounds.right = (short) image->width;
	bitmap.bounds.bottom = (short) image->height;
	if ((image->bytes_per_line % 2) == 1) {
	    char *newPtr, *oldPtr;
	    newData = (char *) ckalloc(image->height *
		    (image->bytes_per_line + 1));
	    newPtr = newData;
	    oldPtr = image->data;
	    for (i = 0; i < image->height; i++) {
		for (j = 0; j < image->bytes_per_line; j++) {
		    *newPtr = InvertByte((unsigned char) *oldPtr);
		    newPtr++, oldPtr++;
		}
	    *newPtr = 0;
	    newPtr++;
	    }
	    bitmap.baseAddr = newData;
	    bitmap.rowBytes = image->bytes_per_line + 1;
	} else {
	    newData = (char *) ckalloc(image->height * image->bytes_per_line);
	    for (i = 0; i < image->height * image->bytes_per_line; i++) {
		newData[i] = InvertByte((unsigned char) image->data[i]);
	    }		
	    bitmap.baseAddr = newData;
	    bitmap.rowBytes = image->bytes_per_line;
	}

	CopyBits(&bitmap, &((GrafPtr) destPort)->portBits, 
		&srcRect, &destRect, srcCopy, NULL);

    } else {
    	/* Color image */
    	PixMap pixmap;
    	
	pixmap.bounds.left = 0;
	pixmap.bounds.top = 0;
	pixmap.bounds.right = (short) image->width;
	pixmap.bounds.bottom = (short) image->height;
	pixmap.pixelType = RGBDirect;
	pixmap.pmVersion = 4;	/* 32bit clean */
	pixmap.packType = 0;
	pixmap.packSize = 0;
	pixmap.hRes = 0x00480000;
	pixmap.vRes = 0x00480000;
	pixmap.pixelSize = 32;
	pixmap.cmpCount = 3;
	pixmap.cmpSize = 8;
	pixmap.planeBytes = 0;
	pixmap.pmTable = NULL;
	pixmap.pmReserved = 0;
	pixmap.baseAddr = image->data;
	pixmap.rowBytes = image->bytes_per_line | 0x8000;
	
	CopyBits((BitMap *) &pixmap, &((GrafPtr) destPort)->portBits, 
	    &srcRect, &destRect, srcCopy, NULL);
    }
    
    if (newData != NULL) {
	ckfree(newData);
    }
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XFillRectangles --
 *
 *	Fill multiple rectangular areas in the given drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws onto the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XFillRectangles(
    Display* display,		/* Display. */
    Drawable d,			/* Draw on this. */
    GC gc,			/* Use this GC. */
    XRectangle *rectangles,	/* Rectangle array. */
    int n_rectangels)		/* Number of rectangles. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Rect theRect;
    int i;

    destPort = TkMacGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(d);

    TkMacSetUpGraphicsPort(gc);

    for (i=0; i<n_rectangels; i++) {
	theRect.left = (short) (macWin->xOff + rectangles[i].x);
	theRect.top = (short) (macWin->yOff + rectangles[i].y);
	theRect.right = (short) (theRect.left + rectangles[i].width);
	theRect.bottom = (short) (theRect.top + rectangles[i].height);
	FillCRect(&theRect, gPenPat);
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawLines --
 *
 *	Draw connected lines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Renders a series of connected lines.
 *
 *----------------------------------------------------------------------
 */

void 
XDrawLines(
    Display* display,		/* Display. */
    Drawable d,			/* Draw on this. */
    GC gc,			/* Use this GC. */
    XPoint* points,		/* Array of points. */
    int npoints,		/* Number of points. */
    int mode)			/* Line drawing mode. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GWorldPtr destPort;
    GDHandle saveDevice;
    int i;

    destPort = TkMacGetDrawablePort(d);

    display->request++;
    if (npoints < 2) {
    	return;  /* TODO: generate BadValue error. */
    }
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    TkMacSetUpClippingRgn(d);

    TkMacSetUpGraphicsPort(gc);

    ShowPen();

    PenPixPat(gPenPat);
    MoveTo((short) (macWin->xOff + points[0].x),
	    (short) (macWin->yOff + points[0].y));
    for (i = 1; i < npoints; i++) {
	if (mode == CoordModeOrigin) {
	    LineTo((short) (macWin->xOff + points[i].x),
		    (short) (macWin->yOff + points[i].y));
	} else {
	    Line((short) (macWin->xOff + points[i].x),
		    (short) (macWin->yOff + points[i].y));
	}
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawSegments --
 *
 *	Draw unconnected lines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Renders a series of connected lines.
 *
 *----------------------------------------------------------------------
 */

void XDrawSegments(
    Display *display,
    Drawable  d,
    GC gc,
    XSegment *segments,
    int  nsegments)
{
    MacDrawable *macWin = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GWorldPtr destPort;
    GDHandle saveDevice;
    int i;

    destPort = TkMacGetDrawablePort(d);

    display->request++;

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    TkMacSetUpClippingRgn(d);

    TkMacSetUpGraphicsPort(gc);

    ShowPen();

    PenPixPat(gPenPat);
    for (i = 0; i < nsegments; i++) {
        MoveTo((short) (macWin->xOff + segments[i].x1),
	        (short) (macWin->yOff + segments[i].y1));
	LineTo((short) (macWin->xOff + segments[i].x2),
		(short) (macWin->yOff + segments[i].y2));
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XFillPolygon --
 *
 *	Draws a filled polygon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws a filled polygon on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XFillPolygon(
    Display* display,		/* Display. */
    Drawable d,			/* Draw on this. */
    GC gc,			/* Use this GC. */
    XPoint* points,		/* Array of points. */
    int npoints,		/* Number of points. */
    int shape,			/* Shape to draw. */
    int mode)			/* Drawing mode. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    PolyHandle polygon;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    int i;

    destPort = TkMacGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(d);
    
    TkMacSetUpGraphicsPort(gc);

    PenNormal();
    polygon = OpenPoly();

    MoveTo((short) (macWin->xOff + points[0].x),
	    (short) (macWin->yOff + points[0].y));
    for (i = 1; i < npoints; i++) {
	if (mode == CoordModePrevious) {
	    Line((short) (macWin->xOff + points[i].x),
		    (short) (macWin->yOff + points[i].y));
	} else {
	    LineTo((short) (macWin->xOff + points[i].x),
		    (short) (macWin->yOff + points[i].y));
	}
    }

    ClosePoly();

    FillCPoly(polygon, gPenPat);

    KillPoly(polygon);
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawRectangle --
 *
 *	Draws a rectangle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws a rectangle on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XDrawRectangle(
    Display* display,		/* Display. */
    Drawable d,			/* Draw on this. */
    GC gc,			/* Use this GC. */
    int x,			/* Upper left corner. */
    int y,
    unsigned int width,		/* Width & height of rect. */
    unsigned int height)
{
    MacDrawable *macWin = (MacDrawable *) d;
    Rect theRect;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    destPort = TkMacGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(d);

    TkMacSetUpGraphicsPort(gc);

    theRect.left = (short) (macWin->xOff + x);
    theRect.top = (short) (macWin->yOff + y);
    theRect.right = (short) (theRect.left + width);
    theRect.bottom = (short) (theRect.top + height);
	
    ShowPen();
    PenPixPat(gPenPat);
    FrameRect(&theRect);

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawArc --
 *
 *	Draw an arc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws an arc on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XDrawArc(
    Display* display,		/* Display. */
    Drawable d,			/* Draw on this. */
    GC gc,			/* Use this GC. */
    int x,			/* Upper left of */
    int y,			/* bounding rect. */
    unsigned int width,		/* Width & height. */
    unsigned int height,
    int angle1,			/* Staring angle of arc. */
    int angle2)			/* Ending angle of arc. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    Rect theRect;
    short start, extent;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    destPort = TkMacGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(d);

    TkMacSetUpGraphicsPort(gc);

    theRect.left = (short) (macWin->xOff + x);
    theRect.top = (short) (macWin->yOff + y);
    theRect.right = (short) (theRect.left + width);
    theRect.bottom = (short) (theRect.top + height);
    start = (short) (90 - (angle1 / 64));
    extent = (short) (-(angle2 / 64));

    ShowPen();
    PenPixPat(gPenPat);
    FrameArc(&theRect, start, extent);

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XFillArc --
 *
 *	Draw a filled arc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws a filled arc on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void
XFillArc(
    Display* display,		/* Display. */
    Drawable d,			/* Draw on this. */
    GC gc,			/* Use this GC. */
    int x,			/* Upper left of */
    int y,			/* bounding rect. */
    unsigned int width,		/* Width & height. */
    unsigned int height,
    int angle1,			/* Staring angle of arc. */
    int angle2)			/* Ending angle of arc. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    Rect theRect;
    short start, extent;
    PolyHandle polygon;
    double sin1, cos1, sin2, cos2, angle;
    double boxWidth, boxHeight;
    double vertex[2], center1[2], center2[2];
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    destPort = TkMacGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(d);

    TkMacSetUpGraphicsPort(gc);

    theRect.left = (short) (macWin->xOff + x);
    theRect.top = (short) (macWin->yOff + y);
    theRect.right = (short) (theRect.left + width);
    theRect.bottom = (short) (theRect.top + height);
    start = (short) (90 - (angle1 / 64));
    extent = (short) (- (angle2 / 64));

    if (gc->arc_mode == ArcChord) {
    	boxWidth = theRect.right - theRect.left;
    	boxHeight = theRect.bottom - theRect.top;
    	angle = -(angle1/64.0)*PI/180.0;
    	sin1 = sin(angle);
    	cos1 = cos(angle);
    	angle -= (angle2/64.0)*PI/180.0;
    	sin2 = sin(angle);
    	cos2 = cos(angle);
    	vertex[0] = (theRect.left + theRect.right)/2.0;
    	vertex[1] = (theRect.top + theRect.bottom)/2.0;
    	center1[0] = vertex[0] + cos1*boxWidth/2.0;
    	center1[1] = vertex[1] + sin1*boxHeight/2.0;
    	center2[0] = vertex[0] + cos2*boxWidth/2.0;
    	center2[1] = vertex[1] + sin2*boxHeight/2.0;

	polygon = OpenPoly();
	MoveTo((short) ((theRect.left + theRect.right)/2),
		(short) ((theRect.top + theRect.bottom)/2));
	
	LineTo((short) (center1[0] + 0.5), (short) (center1[1] + 0.5));
	LineTo((short) (center2[0] + 0.5), (short) (center2[1] + 0.5));
	ClosePoly();

	ShowPen();
	FillCArc(&theRect, start, extent, gPenPat);
	FillCPoly(polygon, gPenPat);

	KillPoly(polygon);
    } else {
	ShowPen();
	FillCArc(&theRect, start, extent, gPenPat);
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * TkScrollWindow --
 *
 *	Scroll a rectangle of the specified window and accumulate
 *	a damage region.
 *
 * Results:
 *	Returns 0 if the scroll genereated no additional damage.
 *	Otherwise, sets the region that needs to be repainted after
 *	scrolling and returns 1.
 *
 * Side effects:
 *	Scrolls the bits in the window.
 *
 *----------------------------------------------------------------------
 */

int
TkScrollWindow(
    Tk_Window tkwin,		/* The window to be scrolled. */
    GC gc,			/* GC for window to be scrolled. */
    int x,			/* Position rectangle to be scrolled. */
    int y,
    int width,
    int height,
    int dx,			/* Distance rectangle should be moved. */
    int dy,
    TkRegion damageRgn)		/* Region to accumulate damage in. */
{
    MacDrawable *destDraw = (MacDrawable *) Tk_WindowId(tkwin);
    RgnHandle rgn = (RgnHandle) damageRgn;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Rect srcRect, scrollRect;
    
    destPort = TkMacGetDrawablePort(Tk_WindowId(tkwin));

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacSetUpClippingRgn(Tk_WindowId(tkwin));

    /*
     * Due to the implementation below the behavior may be differnt
     * than X in certain cases that should never occur in Tk.  The 
     * scrollRect is the source rect extended by the offset (the union 
     * of the source rect and the offset rect).  Everything
     * in the extended scrollRect is scrolled.  On X, it's possible
     * to "skip" over an area if the offset makes the source and
     * destination rects disjoint and non-aligned.
     */
       
    SetRect(&srcRect, (short) (destDraw->xOff + x),
	    (short) (destDraw->yOff + y),
	    (short) (destDraw->xOff + x + width),
	    (short) (destDraw->yOff + y + height));
    scrollRect = srcRect;
    if (dx < 0) {
	scrollRect.left += dx;
    } else {
	scrollRect.right += dx;
    }
    if (dy < 0) {
	scrollRect.top += dy;
    } else {
	scrollRect.bottom += dy;
    }

    /*
     * Adjust clip region so that we don't copy any windows
     * that may overlap us.
     */
    RectRgn(rgn, &srcRect);
    DiffRgn(rgn, destPort->visRgn, rgn);
    OffsetRgn(rgn, dx, dy);
    DiffRgn(destPort->clipRgn, rgn, destPort->clipRgn);
    SetEmptyRgn(rgn);
    
    /*
     * When a menu is up, the Mac does not expect drawing to occur and
     * does not clip out the menu. We have to do it ourselves. This
     * is pretty gross.
     */

    if (tkUseMenuCascadeRgn == 1) {
    	Point scratch = {0, 0};
    	MacDrawable *macDraw = (MacDrawable *) Tk_WindowId(tkwin);

	LocalToGlobal(&scratch);
	CopyRgn(tkMenuCascadeRgn, rgn);
	OffsetRgn(rgn, -scratch.h, -scratch.v);
	DiffRgn(destPort->clipRgn, rgn, destPort->clipRgn);
	SetEmptyRgn(rgn);
	macDraw->toplevel->flags |= TK_DRAWN_UNDER_MENU;
    }
	
    ScrollRect(&scrollRect, dx, dy, rgn);
    
    SetGWorld(saveWorld, saveDevice);
    
    /*
     * Fortunantly, the region returned by ScrollRect is symanticlly
     * the same as what we need to return in this function.  If the
     * region is empty we return zero to denote that no damage was
     * created.
     */
    if (EmptyRgn(rgn)) {
	return 0;
    } else {
	return 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacSetUpGraphicsPort --
 *
 *	Set up the graphics port from the given GC.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current port is adjusted.
 *
 *----------------------------------------------------------------------
 */

void
TkMacSetUpGraphicsPort(
    GC gc)		/* GC to apply to current port. */
{
    RGBColor macColor;

    if (gPenPat == NULL) {
	gPenPat = NewPixPat();
    }
    
    if (TkSetMacColor(gc->foreground, &macColor) == true) {
        /* TODO: cache RGBPats for preformace - measure gains...  */
	MakeRGBPat(gPenPat, &macColor);
    }
    
    PenNormal();
    if(gc->function == GXxor) {
	PenMode(patXor);
    }
    if (gc->line_width > 1) {
	PenSize(gc->line_width, gc->line_width);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacSetUpClippingRgn --
 *
 *	Set up the clipping region so that drawing only occurs on the
 *	specified X subwindow.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The clipping region in the current port is changed.
 *
 *----------------------------------------------------------------------
 */

void
TkMacSetUpClippingRgn(
    Drawable drawable)		/* Drawable to update. */
{
    MacDrawable *macDraw = (MacDrawable *) drawable;

    if (macDraw->winPtr != NULL) {
	if (macDraw->flags & TK_CLIP_INVALID) {
	    TkMacUpdateClipRgn(macDraw->winPtr);
	}

	/*
	 * When a menu is up, the Mac does not expect drawing to occur and
	 * does not clip out the menu. We have to do it ourselves. This
	 * is pretty gross.
	 */

	if (macDraw->clipRgn != NULL) {
	    if (tkUseMenuCascadeRgn == 1) {
	    	Point scratch = {0, 0};
	    	GDHandle saveDevice;
	    	GWorldPtr saveWorld;
	    	
	    	GetGWorld(&saveWorld, &saveDevice);
	    	SetGWorld(TkMacGetDrawablePort(drawable), NULL);
	    	LocalToGlobal(&scratch);
	    	SetGWorld(saveWorld, saveDevice);
	    	if (tmpRgn == NULL) {
	    	    tmpRgn = NewRgn();
	    	}
	    	CopyRgn(tkMenuCascadeRgn, tmpRgn);
	    	OffsetRgn(tmpRgn, -scratch.h, -scratch.v);
	    	DiffRgn(macDraw->clipRgn, tmpRgn, tmpRgn);
	    	SetClip(tmpRgn);
	    	macDraw->toplevel->flags |= TK_DRAWN_UNDER_MENU;
	    } else {
	    	SetClip(macDraw->clipRgn);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacMakeStippleMap --
 *
 *	Given a drawable and a stipple pattern this function draws the
 *	pattern repeatedly over the drawable.  The drawable can then
 *	be used as a mask for bit-bliting a stipple pattern over an
 *	object.
 *
 * Results:
 *	A BitMap data structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

BitMapPtr
TkMacMakeStippleMap(
    Drawable drawable,		/* Window to apply stipple. */
    Drawable stipple)		/* The stipple pattern. */
{
    MacDrawable *destDraw = (MacDrawable *) drawable;
    GWorldPtr destPort;
    BitMapPtr bitmapPtr;
    int width, height, stippleHeight, stippleWidth;
    int i, j;
    char * data;
    Rect bounds;

    destPort = TkMacGetDrawablePort(drawable);
    width = destPort->portRect.right - destPort->portRect.left;
    height = destPort->portRect.bottom - destPort->portRect.top;
    
    bitmapPtr = (BitMap *) ckalloc(sizeof(BitMap));
    data = (char *) ckalloc(height * ((width / 8) + 1));
    bitmapPtr->bounds.top = bitmapPtr->bounds.left = 0;
    bitmapPtr->bounds.right = (short) width;
    bitmapPtr->bounds.bottom = (short) height;
    bitmapPtr->baseAddr = data;
    bitmapPtr->rowBytes = (width / 8) + 1;

    destPort = TkMacGetDrawablePort(stipple);
    stippleWidth = destPort->portRect.right - destPort->portRect.left;
    stippleHeight = destPort->portRect.bottom - destPort->portRect.top;

    for (i = 0; i < height; i += stippleHeight) {
	for (j = 0; j < width; j += stippleWidth) {
	    bounds.left = j;
	    bounds.top = i;
	    bounds.right = j + stippleWidth;
	    bounds.bottom = i + stippleHeight;
	    
	    CopyBits(&((GrafPtr) destPort)->portBits, bitmapPtr, 
		&((GrafPtr) destPort)->portRect, &bounds, srcCopy, NULL);
	}
    }
    return bitmapPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * InvertByte --
 *
 *      This function reverses the bits in the passed in Byte of data.
 *
 * Results:
 *      The incoming byte in reverse bit order.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static unsigned char
InvertByte(
    unsigned char data)		/* Byte of data. */
{
    unsigned char i;
    unsigned char mask = 1, result = 0;
 
    for (i = (1 << 7); i != 0; i /= 2) {
        if (data & mask) {
            result |= i;
        }
        mask = mask << 1;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDrawpHighlightBorder --
 *
 *	This procedure draws a rectangular ring around the outside of
 *	a widget to indicate that it has received the input focus.
 *
 *      On the Macintosh, this puts a 1 pixel border in the bgGC color
 *      between the widget and the focus ring, except in the case where 
 *      highlightWidth is 1, in which case the border is left out.
 *
 *      For proper Mac L&F, use highlightWidth of 3.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A rectangle "width" pixels wide is drawn in "drawable",
 *	corresponding to the outer area of "tkwin".
 *
 *----------------------------------------------------------------------
 */

void 
TkpDrawHighlightBorder (
        Tk_Window tkwin, 
        GC fgGC, 
        GC bgGC, 
        int highlightWidth,
        Drawable drawable)
{
    if (highlightWidth == 1) {
        TkDrawInsetFocusHighlight (tkwin, fgGC, highlightWidth, drawable, 0);
    } else {
        TkDrawInsetFocusHighlight (tkwin, bgGC, highlightWidth, drawable, 0);
        if (fgGC != bgGC) {
            TkDrawInsetFocusHighlight (tkwin, fgGC, highlightWidth - 1, drawable, 0);
        }
    }
}
        