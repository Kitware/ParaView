/* 
 * tkMacOSXDraw.c --
 *
 *        This file contains functions that perform drawing to
 *        Xlib windows.  Most of the functions simple emulate
 *        Xlib functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "X11/X.h"
#include "X11/Xlib.h"
#include <stdio.h>

#include <Carbon/Carbon.h>
#include "tkMacOSXInt.h"
#include "tkPort.h"
#include "tkMacOSXDebug.h"

#ifndef PI
#    define PI 3.14159265358979323846
#endif
#define RGBFLOATRED( c )	(float)((float)(c.red) / 65535.0)
#define RGBFLOATGREEN( c )	(float)((float)(c.green) / 65535.0)
#define RGBFLOATBLUE( c )	(float)((float)(c.blue) / 65535.0)

/*
 * Temporary regions that can be reused.
 */

static RgnHandle tmpRgn = NULL;
static RgnHandle tmpRgn2 = NULL;

static PixPatHandle gPenPat = NULL;

static int useCGDrawing = 0;

/*
 * Prototypes for functions used only in this file.
 */
static unsigned char    InvertByte _ANSI_ARGS_((unsigned char data));

void TkMacOSXSetUpCGContext(MacDrawable *macWin,
    CGrafPtr destPort, GC gc,  CGContextRef *contextPtr);
void TkMacOSXReleaseCGContext(MacDrawable *macWin, CGrafPtr destPort, 
        CGContextRef *context);
/*
 *----------------------------------------------------------------------
 *
 * XCopyArea --
 *
 *        Copies data from one drawable to another using block transfer
 *        routines.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Data is moved from a window or bitmap to a second window or
 *        bitmap.
 *
 *----------------------------------------------------------------------
 */

void 
XCopyArea(
    Display* display,                /* Display. */
    Drawable src,                /* Source drawable. */
    Drawable dst,                /* Destination drawable. */
    GC gc,                        /* GC to use. */
    int src_x,                        /* X & Y, width & height */
    int src_y,                        /* define the source rectangle */
    unsigned int width,                /* the will be copied. */
    unsigned int height,
    int dest_x,                        /* Dest X & Y on dest rect. */
    int dest_y)
{
    Rect srcRect, dstRect;
    Rect * srcPtr, * dstPtr;
    const BitMap * srcBit;
    const BitMap * dstBit;
    MacDrawable *srcDraw = (MacDrawable *) src;
    MacDrawable *dstDraw = (MacDrawable *) dst;
    CGrafPtr srcPort, dstPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    short tmode;
    RGBColor origForeColor, origBackColor, whiteColor, blackColor;
    Rect clpRect;

    dstPort = TkMacOSXGetDrawablePort(dst);
    srcPort = TkMacOSXGetDrawablePort(src);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(dstPort, NULL);
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
    
    if (tmpRgn2 == NULL) {
        tmpRgn2 = NewRgn();
    }
    srcPtr = &srcRect;
    SetRect(&srcRect, (short) (srcDraw->xOff + src_x),
            (short) (srcDraw->yOff + src_y),
            (short) (srcDraw->xOff + src_x + width ),
            (short) (srcDraw->yOff + src_y + height));        
    if (tkPictureIsOpen ) {
        dstPtr = &srcRect;
    } else {
        dstPtr = &dstRect;
        SetRect(&dstRect, (short) (dstDraw->xOff + dest_x),
            (short) (dstDraw->yOff + dest_y), 
            (short) (dstDraw->xOff + dest_x + width ),
            (short) (dstDraw->yOff + dest_y + height));        
    }
    TkMacOSXSetUpClippingRgn(dst);
    /*
     *  We will change the clip rgn in this routine, so we need to 
     *  be able to restore it when we exit.
     */
 
    GetClip(tmpRgn2);
    if (tkPictureIsOpen) {
        /*
         * When rendering into a picture, after a call to "OpenCPicture"
         * the clipping is seriously WRONG and also INCONSISTENT with the
         * clipping for single plane bitmaps.
         * To circumvent this problem,  we clip to the whole window 
         * In this case, would have also clipped to the srcRect
         * ClipRect(&srcRect);
         */
        GetPortBounds(dstPort,&clpRect);
        dstPtr = &srcRect;
        ClipRect(&clpRect);
    }
    if (!gc->clip_mask ) { 
    } else if (((TkpClipMask*)gc->clip_mask)->type == TKP_CLIP_REGION) {
        RgnHandle clipRgn = (RgnHandle)
            ((TkpClipMask*)gc->clip_mask)->value.region;
 
        int xOffset, yOffset;
        if (tmpRgn == NULL) {
            tmpRgn = NewRgn();
        }
        if (!tkPictureIsOpen) {
            xOffset = dstDraw->xOff + gc->clip_x_origin;
            yOffset = dstDraw->yOff + gc->clip_y_origin;
            OffsetRgn(clipRgn, xOffset, yOffset);
        }
        GetClip(tmpRgn);
        SectRgn(tmpRgn, clipRgn, tmpRgn);
        SetClip(tmpRgn);
        if (!tkPictureIsOpen) {
             OffsetRgn(clipRgn, -xOffset, -yOffset);
        }
    }
    srcBit = GetPortBitMapForCopyBits( srcPort );
    dstBit = GetPortBitMapForCopyBits( dstPort );
    tmode = srcCopy;

    CopyBits(srcBit, dstBit, srcPtr, dstPtr, tmode, NULL);
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
 *        Copies a bitmap from a source drawable to a destination
 *        drawable.  The plane argument specifies which bit plane of
 *        the source contains the bitmap.  Note that this implementation
 *        ignores the gc->function.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Changes the destination drawable.
 *
 *----------------------------------------------------------------------
 */

void
XCopyPlane(
    Display* display,                /* Display. */
    Drawable src,                /* Source drawable. */
    Drawable dst,                /* Destination drawable. */
    GC gc,                        /* The GC to use. */
    int src_x,                        /* X, Y, width & height */
    int src_y,                        /* define the source rect. */
    unsigned int width,
    unsigned int height,
    int dest_x,                        /* X & Y on dest where we will copy. */
    int dest_y,
    unsigned long plane)        /* Which plane to copy. */
{
    Rect srcRect, dstRect;
    Rect * srcPtr, * dstPtr;
    const BitMap * srcBit;
    const BitMap * dstBit;
    const BitMap * mskBit;
    MacDrawable *srcDraw = (MacDrawable *) src;
    MacDrawable *dstDraw = (MacDrawable *) dst;
    GWorldPtr srcPort, dstPort, mskPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    RGBColor macColor; 
    TkpClipMask *clipPtr = (TkpClipMask*)gc->clip_mask;
    short tmode;

    srcPort = TkMacOSXGetDrawablePort(src);
    dstPort = TkMacOSXGetDrawablePort(dst);
    
    if (tmpRgn == NULL) {
        tmpRgn = NewRgn();
    }
    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(dstPort, NULL);

    TkMacOSXSetUpClippingRgn(dst);


    srcBit = GetPortBitMapForCopyBits ( srcPort );
    dstBit = GetPortBitMapForCopyBits ( dstPort );
    SetRect(&srcRect, (short) (srcDraw->xOff + src_x),
            (short) (srcDraw->yOff + src_y),
            (short) (srcDraw->xOff + src_x + width),
            (short) (srcDraw->yOff + src_y + height));
    srcPtr = &srcRect;
    if (tkPictureIsOpen) {
        /*
         * When rendering into a picture, after a call to "OpenCPicture"
         * the clipping is seriously WRONG and also INCONSISTENT with the
         * clipping for color bitmaps.
         * To circumvent this problem,  we clip to the whole window 
         */
        Rect clpRect;
        GetPortBounds(dstPort,&clpRect);
        ClipRect(&clpRect);
        dstPtr = &srcRect;
    } else {
        dstPtr = &dstRect;
        SetRect(&dstRect, (short) (dstDraw->xOff + dest_x),
            (short) (dstDraw->yOff + dest_y), 
            (short) (dstDraw->xOff + dest_x + width),
            (short) (dstDraw->yOff + dest_y + height));
    }
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
        CopyBits(srcBit, dstBit, srcPtr, dstPtr, tmode, NULL);
    } else if (clipPtr->type == TKP_CLIP_PIXMAP) {
        if (clipPtr->value.pixmap == src) {
            PixMapHandle pm;
            /*
             * Case 2: transparent bitmaps.  If it's color we ignore
             * the forecolor.
             */
            pm=GetPortPixMap(srcPort);
            if (GetPixDepth(pm)== 1) {
                tmode = srcOr;
            } else {
                tmode = transparent;
            }
            CopyBits(srcBit, dstBit, srcPtr, dstPtr, tmode, NULL);
        } else {
            /*
             * Case 3: two arbitrary bitmaps.         
             */
            tmode = srcCopy;
            mskPort = TkMacOSXGetDrawablePort(clipPtr->value.pixmap);
            mskBit = GetPortBitMapForCopyBits ( mskPort );
            CopyDeepMask(srcBit, mskBit, dstBit,
                srcPtr, srcPtr, dstPtr, tmode, NULL);
        }
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * TkPutImage --
 *
 *        Copies a subimage from an in-memory image to a rectangle of
 *        of the specified drawable.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws the image on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
TkPutImage(
    unsigned long *colors,        /* Unused on Macintosh. */
    int ncolors,                /* Unused on Macintosh. */
    Display* display,                /* Display. */
    Drawable d,                        /* Drawable to place image on. */
    GC gc,                        /* GC to use. */
    XImage* image,                /* Image to place. */
    int src_x,                        /* Source X & Y. */
    int src_y,
    int dest_x,                        /* Destination X & Y. */
    int dest_y,
    unsigned int width,                /* Same width & height for both */
    unsigned int height)        /* distination and source. */
{
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    const BitMap * destBits;
    int i, j;
    BitMap bitmap;
    char *newData = NULL;
    Rect destRect, srcRect;

    destPort = TkMacOSXGetDrawablePort(d);
    SetRect(&destRect, dest_x, dest_y, dest_x + width, dest_y + height);
    SetRect(&srcRect, src_x, src_y, src_x + width, src_y + height);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

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
        destBits = GetPortBitMapForCopyBits ( destPort );
        CopyBits(&bitmap, destBits, 
                &srcRect, &destRect, srcCopy, NULL);

    } else {
            /* Color image */
            PixMap pixmap;
            
        pixmap.bounds.left = 0;
        pixmap.bounds.top = 0;
        pixmap.bounds.right = (short) image->width;
        pixmap.bounds.bottom = (short) image->height;
        pixmap.pixelType = RGBDirect;
        pixmap.pmVersion = 4;        /* 32bit clean */
        pixmap.packType = 0;
        pixmap.packSize = 0;
        pixmap.hRes = 0x00480000;
        pixmap.vRes = 0x00480000;
        pixmap.pixelSize = 32;
        pixmap.cmpCount = 3;
        pixmap.cmpSize = 8;
        pixmap.pixelFormat = 0;
        pixmap.pmTable = NULL;
        pixmap.pmExt = 0;
        pixmap.baseAddr = image->data;
        pixmap.rowBytes = image->bytes_per_line | 0x8000;
        
        CopyBits((BitMap *) &pixmap, GetPortBitMapForCopyBits ( destPort ), 
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
 *        Fill multiple rectangular areas in the given drawable.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws onto the specified drawable.
 *
 *----------------------------------------------------------------------
 */
void 
XFillRectangles(
    Display* display,                /* Display. */
    Drawable d,                        /* Draw on this. */
    GC gc,                        /* Use this GC. */
    XRectangle *rectangles,        /* Rectangle array. */
    int n_rectangels)                /* Number of rectangles. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Rect theRect;
    int i;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    TkMacOSXSetUpGraphicsPort(gc, destPort);

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
 *        Draw connected lines.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Renders a series of connected lines.
 *
 *----------------------------------------------------------------------
 */

void 
XDrawLines(
    Display* display,                /* Display. */
    Drawable d,                        /* Draw on this. */
    GC gc,                        /* Use this GC. */
    XPoint* points,                /* Array of points. */
    int npoints,                /* Number of points. */
    int mode)                        /* Line drawing mode. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GWorldPtr destPort;
    GDHandle saveDevice;
    int i;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    if (npoints < 2) {
            return;  /* TODO: generate BadValue error. */
    }
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    TkMacOSXSetUpClippingRgn(d);

    if (useCGDrawing) {
        CGContextRef outContext;
        
        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);
        
        CGContextBeginPath(outContext);
        CGContextMoveToPoint(outContext, (float) points[0].x,
                (float) points[0].y);
        if (mode == CoordModeOrigin) {
            for (i = 1; i < npoints; i++) {
                CGContextAddLineToPoint(outContext,
                        (float) points[i].x,
                        (float) points[i].y); 
            }
        }
        
        CGContextStrokePath(outContext);
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
        TkMacOSXSetUpGraphicsPort(gc, destPort);
    
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
        HidePen();

    }
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawSegments --
 *
 *        Draw unconnected lines.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Renders a series of connected lines.
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

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    TkMacOSXSetUpClippingRgn(d);

    if (useCGDrawing) {
        CGContextRef outContext;

        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

        CGContextBeginPath(outContext);
        for (i = 0; i < nsegments; i++) {
            CGContextMoveToPoint(outContext,
                    (float) segments[i].x1,
                    (float) segments[i].y1);
            CGContextAddLineToPoint (outContext,
                    (float) segments[i].x2,
                    (float) segments[i].y2);
        }
        CGContextStrokePath(outContext);
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
        TkMacOSXSetUpGraphicsPort(gc, destPort);

        ShowPen();

        PenPixPat(gPenPat);
        for (i = 0; i < nsegments; i++) {
            MoveTo((short) (macWin->xOff + segments[i].x1),
                   (short) (macWin->yOff + segments[i].y1));
            LineTo((short) (macWin->xOff + segments[i].x2),
                   (short) (macWin->yOff + segments[i].y2));
        }
        HidePen();
    }
    
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XFillPolygon --
 *
 *        Draws a filled polygon.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws a filled polygon on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XFillPolygon(
    Display* display,                /* Display. */
    Drawable d,                        /* Draw on this. */
    GC gc,                        /* Use this GC. */
    XPoint* points,                /* Array of points. */
    int npoints,                /* Number of points. */
    int shape,                        /* Shape to draw. */
    int mode)                        /* Drawing mode. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    PolyHandle polygon;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    int i;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);
    
    if (useCGDrawing) {
        CGContextRef outContext;
        
        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);
        
        CGContextBeginPath(outContext);
        CGContextMoveToPoint(outContext, (float) (points[0].x),
                (float) (points[0].y));
        for (i = 1; i < npoints; i++) {
            
            if (mode == CoordModePrevious) {
                CGContextAddLineToPoint(outContext, (float) points[i].x,
                        (float) points[i].y);
            } else {
            }
        }
        //CGContextStrokePath(outContext);
        CGContextFillPath(outContext);
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
        TkMacOSXSetUpGraphicsPort(gc, destPort);
    
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
    }
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawRectangle --
 *
 *        Draws a rectangle.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws a rectangle on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XDrawRectangle(
    Display* display,                /* Display. */
    Drawable d,                        /* Draw on this. */
    GC gc,                        /* Use this GC. */
    int x,                        /* Upper left corner. */
    int y,
    unsigned int width,                /* Width & height of rect. */
    unsigned int height)
{
    MacDrawable *macWin = (MacDrawable *) d;
    Rect theRect;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    TkMacOSXSetUpGraphicsPort(gc, destPort);

    theRect.left = (short) (macWin->xOff + x);
    theRect.top = (short) (macWin->yOff + y);
    theRect.right = (short) (theRect.left + width);
    theRect.bottom = (short) (theRect.top + height);
        
    ShowPen();
    PenPixPat(gPenPat);
    FrameRect(&theRect);
    HidePen();

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawRectangles --
 *
 *       Draws the outlines of the specified rectangles as if a
 *       five-point PolyLine protocol request were specified for each
 *       rectangle:
 *
 *             [x,y] [x+width,y] [x+width,y+height] [x,y+height]
 *             [x,y]
 *
 *      For the specified rectangles, these functions do not draw a
 *      pixel more than once.  XDrawRectangles draws the rectangles in
 *      the order listed in the array.  If rectangles intersect, the
 *      intersecting pixels are drawn multiple times.  Draws a
 *      rectangle.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws rectangles on the specified drawable.
 *
 *----------------------------------------------------------------------
 */
void
XDrawRectangles(
    Display *display,
    Drawable drawable,
    GC gc,
    XRectangle *rectArr,
    int nRects)
{
    MacDrawable *macWin = (MacDrawable *) drawable;
    Rect     rect;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    XRectangle * rectPtr;
    int       i;

    destPort = TkMacOSXGetDrawablePort(drawable);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(drawable);

    TkMacOSXSetUpGraphicsPort(gc, destPort);


    ShowPen();
    PenPixPat(gPenPat);

    for (i = 0, rectPtr = rectArr; i < nRects;i++, rectPtr++ ) {
        rect.left = (short) (macWin->xOff + rectPtr->x);
        rect.top = (short) (macWin->yOff + rectPtr->y);
        rect.right = (short) (rect.left + rectPtr->width);
        rect.bottom = (short) (rect.top + rectPtr->height);
        FrameRect(&rect);
    }
    HidePen();

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawArc --
 *
 *        Draw an arc.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws an arc on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void 
XDrawArc(
    Display* display,                /* Display. */
    Drawable d,                        /* Draw on this. */
    GC gc,                        /* Use this GC. */
    int x,                        /* Upper left of */
    int y,                        /* bounding rect. */
    unsigned int width,                /* Width & height. */
    unsigned int height,
    int angle1,                        /* Staring angle of arc. */
    int angle2)                        /* Ending angle of arc. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    Rect theRect;
    short start, extent;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    float fX = (float) x,
          fY = (float) y,
          fWidth = (float) width,
          fHeight = (float) height;

    if (width == 0 || height == 0) {
        return;
    }
    
    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    if (useCGDrawing) {
        CGContextRef outContext;
        CGAffineTransform transform;
        int clockwise = angle1 ? 0 : 1;

        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

        CGContextBeginPath(outContext);
        
        /*
         * If we are drawing an oval, we have to squash the coordinate
         * system before drawing, since CGContextAddArcToPoint only draws
         * circles.
         */

        CGContextSaveGState(outContext);
        transform = CGAffineTransformMakeTranslation((float) (x + width/2),
                (float) (y + height/2));
        transform = CGAffineTransformScale(transform, 1.0, fHeight/fWidth);
        CGContextConcatCTM(outContext, transform);

        CGContextAddArc(outContext, 0.0, 0.0,
                (float) width/2,
                (float) angle1, (float) angle2, clockwise);

        CGContextRestoreGState(outContext);

        CGContextStrokePath(outContext);
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
        TkMacOSXSetUpGraphicsPort(gc, destPort);


        theRect.left = (short) (macWin->xOff + x);
        theRect.top = (short) (macWin->yOff + y);
        theRect.right = (short) (theRect.left + width);
        theRect.bottom = (short) (theRect.top + height);
        start = (short) (90 - (angle1 / 64));
        extent = (short) (-(angle2 / 64));

        ShowPen();
        PenPixPat(gPenPat);
        FrameArc(&theRect, start, extent);
        HidePen();
    }
    
    SetGWorld(saveWorld, saveDevice);
}

/* 
 *----------------------------------------------------------------------
 * 
 * XDrawArcs --
 * 
 *      Draws multiple circular or elliptical arcs.  Each arc is
 *      specified by a rectangle and two angles.  The center of the
 *      circle or ellipse is the center of the rect- angle, and the
 *      major and minor axes are specified by the width and height.
 *      Positive angles indicate counterclock- wise motion, and
 *      negative angles indicate clockwise motion.  If the magnitude
 *      of angle2 is greater than 360 degrees, XDrawArcs truncates it
 *      to 360 degrees.
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      Draws an arc for each array element on the specified drawable.
 *  
 *----------------------------------------------------------------------
 */ 
void
XDrawArcs(
    Display *display,
    Drawable d,
    GC gc,
    XArc *arcArr,
    int nArcs)
{

    MacDrawable *macWin = (MacDrawable *) d;
    Rect rect;
    short start, extent;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    XArc *    arcPtr;
    int       i;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    TkMacOSXSetUpGraphicsPort(gc, destPort);


    ShowPen();
    PenPixPat(gPenPat);
    for (i = 0, arcPtr = arcArr;i < nArcs;i++, arcPtr++ ) {
        rect.left = (short) (macWin->xOff + arcPtr->x);
        rect.top = (short) (macWin->yOff + arcPtr->y);
        rect.right = (short) (rect.left + arcPtr->width);
        rect.bottom = (short) (rect.top + arcPtr->height);
        start = (short) (90 - (arcPtr->angle1 / 64));
        extent = (short) (-(arcPtr->angle2 / 64));
        FrameArc(&rect, start, extent);
    }
    HidePen();

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * XFillArc --
 *
 *        Draw a filled arc.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Draws a filled arc on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

void
XFillArc(
    Display* display,                /* Display. */
    Drawable d,                        /* Draw on this. */
    GC gc,                        /* Use this GC. */
    int x,                        /* Upper left of */
    int y,                        /* bounding rect. */
    unsigned int width,                /* Width & height. */
    unsigned int height,
    int angle1,                        /* Staring angle of arc. */
    int angle2)                        /* Ending angle of arc. */
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

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    TkMacOSXSetUpGraphicsPort(gc, destPort);

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
        HidePen();

        KillPoly(polygon);
    } else {
        ShowPen();
        FillCArc(&theRect, start, extent, gPenPat);
        HidePen();
    }

    SetGWorld(saveWorld, saveDevice);
}

/*  
 *----------------------------------------------------------------------
 *  
 * XFillArcs --  
 *
 *      Draw a filled arc.
 *  
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws a filled arc for each array element on the specified drawable.
 *      
 *----------------------------------------------------------------------
 */     
void 
XFillArcs(
    Display *display,
    Drawable d,
    GC gc,
    XArc *arcArr,
    int nArcs)
{
    MacDrawable *macWin = (MacDrawable *) d;
    Rect rect;
    short start, extent;
    PolyHandle polygon;
    double sin1, cos1, sin2, cos2, angle;
    double boxWidth, boxHeight;
    double vertex[2], center1[2], center2[2];
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    int       i;
    XArc      * arcPtr;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    TkMacOSXSetUpGraphicsPort(gc, destPort);

    for (i = 0, arcPtr = arcArr;i<nArcs;i++, arcPtr++ ) {
        rect.left = (short) (macWin->xOff + arcPtr->x);
        rect.top = (short) (macWin->yOff + arcPtr->y);
        rect.right = (short) (rect.left + arcPtr->width);
        rect.bottom = (short) (rect.top + arcPtr->height);
        start = (short) (90 - (arcPtr->angle1 / 64));
        extent = (short) (- (arcPtr->angle2 / 64));

        if (gc->arc_mode == ArcChord) {
            boxWidth = rect.right - rect.left;
            boxHeight = rect.bottom - rect.top;
            angle = -(arcPtr->angle1/64.0)*PI/180.0;
            sin1 = sin(angle);
            cos1 = cos(angle);
            angle -= (arcPtr->angle2/64.0)*PI/180.0;
            sin2 = sin(angle);
            cos2 = cos(angle);
            vertex[0] = (rect.left + rect.right)/2.0;
            vertex[1] = (rect.top + rect.bottom)/2.0;
            center1[0] = vertex[0] + cos1*boxWidth/2.0;
            center1[1] = vertex[1] + sin1*boxHeight/2.0;
            center2[0] = vertex[0] + cos2*boxWidth/2.0;
            center2[1] = vertex[1] + sin2*boxHeight/2.0;
    
            polygon = OpenPoly();
            MoveTo((short) ((rect.left + rect.right)/2),
                    (short) ((rect.top + rect.bottom)/2));
    
            LineTo((short) (center1[0] + 0.5), (short) (center1[1] + 0.5));
            LineTo((short) (center2[0] + 0.5), (short) (center2[1] + 0.5));
            ClosePoly();
    
            ShowPen();
            FillCArc(&rect, start, extent, gPenPat);
            FillCPoly(polygon, gPenPat);
            HidePen();

            KillPoly(polygon);
        } else {
            ShowPen();
            FillCArc(&rect, start, extent, gPenPat);
            HidePen();
        }
    }
    SetGWorld(saveWorld, saveDevice);
}

/* 
 *----------------------------------------------------------------------
 * 
 * XMaxRequestSize --
 * 
 *----------------------------------------------------------------------
 */
long
XMaxRequestSize(Display *display)
{
    return (SHRT_MAX / 4);
}

/*
 *----------------------------------------------------------------------
 *
 * TkScrollWindow --
 *
 *        Scroll a rectangle of the specified window and accumulate
 *        a damage region.
 *
 * Results:
 *        Returns 0 if the scroll genereated no additional damage.
 *        Otherwise, sets the region that needs to be repainted after
 *        scrolling and returns 1.
 *
 * Side effects:
 *        Scrolls the bits in the window.
 *
 *----------------------------------------------------------------------
 */

int
TkScrollWindow(
    Tk_Window tkwin,                /* The window to be scrolled. */
    GC gc,                        /* GC for window to be scrolled. */
    int x,                        /* Position rectangle to be scrolled. */
    int y,
    int width,
    int height,
    int dx,                        /* Distance rectangle should be moved. */
    int dy,
    TkRegion damageRgn)                /* Region to accumulate damage in. */
{
    MacDrawable *destDraw = (MacDrawable *) Tk_WindowId(tkwin);
    RgnHandle rgn = (RgnHandle) damageRgn;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Rect srcRect, scrollRect;
    RgnHandle visRgn, clipRgn;
    
    destPort = TkMacOSXGetDrawablePort(Tk_WindowId(tkwin));

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(Tk_WindowId(tkwin));

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
    visRgn = NewRgn();
    clipRgn = NewRgn();
    RectRgn(rgn, &srcRect);
    GetPortVisibleRegion(destPort,visRgn);
    DiffRgn(rgn, visRgn, rgn);
    OffsetRgn(rgn, dx, dy);
    GetPortClipRegion(destPort, clipRgn);
    DiffRgn(clipRgn, rgn, clipRgn);
    SetPortClipRegion(destPort, clipRgn);
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
        DiffRgn(clipRgn, rgn, clipRgn);
        SetPortClipRegion(destPort, clipRgn);
        SetEmptyRgn(rgn);
        macDraw->toplevel->flags |= TK_DRAWN_UNDER_MENU;
    }
        
    ScrollRect(&scrollRect, dx, dy, rgn);
    
    SetGWorld(saveWorld, saveDevice);
    
    DisposeRgn(clipRgn);
    DisposeRgn(visRgn);
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
 * TkMacOSXSetUpGraphicsPort --
 *
 *        Set up the graphics port from the given GC.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The current port is adjusted.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXSetUpGraphicsPort(
    GC gc,
    GWorldPtr destPort)                /* GC to apply to current port. */
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
    if (gc->line_style != LineSolid) {
        unsigned char *p = (unsigned char *) &(gc->dashes);
        /*
         * Here the dash pattern should be set in the drawing,
         * environment, but I don't know how to do that for the Mac.
         *
         * p[] is an array of unsigned chars containing the dash list.
         * A '\0' indicates the end of this list.
         *
         * Someone knows how to implement this? If you have a more
         * complete implementation of SetUpGraphicsPort() for
         * the Mac (or for Windows), please let me know.
         *
         *        Jan Nijtmans
         *        CMG Arnhem, B.V.
         *        email: j.nijtmans@chello.nl (private)
         *               jan.nijtmans@cmg.nl (work)
         *        url:   http://purl.oclc.org/net/nijtmans/
         *
         * FIXME:
         * This is not possible with QuickDraw line drawing, we either
         * have to convert all line drawings to regions, or, on Mac OS X
         * we can use CG to draw our lines instead of QuickDraw.
         */
    }
}
/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXSetUpGraphicsPort --
 *
 *        Set up the graphics port from the given GC.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The current port is adjusted.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXSetUpCGContext(
    MacDrawable *macWin,
    CGrafPtr destPort,
    GC gc,
    CGContextRef *contextPtr)                /* GC to apply to current port. */
{
    RGBColor macColor;
    CGContextRef outContext;
    OSStatus err;
    Rect boundsRect;
    CGAffineTransform coordsTransform;

    err = QDBeginCGContext(destPort, contextPtr);
    outContext = *contextPtr;
    
    CGContextSaveGState(outContext);
    
    GetPortBounds(destPort, &boundsRect);
    
    CGContextResetCTM(outContext);
    coordsTransform = CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0, 
            (float)(boundsRect.bottom - boundsRect.top));
    CGContextConcatCTM(outContext, coordsTransform);
    
    if (macWin->clipRgn != NULL) {
        ClipCGContextToRegion(outContext, &boundsRect, macWin->clipRgn);
    } else {
        RgnHandle clipRgn = NewRgn();
        GetPortClipRegion(destPort, clipRgn);
        ClipCGContextToRegion(outContext, &boundsRect, 
                clipRgn);
        DisposeRgn(clipRgn);
    }

    /* Now offset the CTM to the subwindow offset */
    
    CGContextTranslateCTM(outContext, macWin->xOff, macWin->yOff);

    if (TkSetMacColor(gc->foreground, &macColor) == true) {                
        CGContextSetRGBStrokeColor(outContext, RGBFLOATRED(macColor), 
                RGBFLOATGREEN(macColor), 
                RGBFLOATBLUE(macColor), 1.0);
    }
    if (TkSetMacColor(gc->background, &macColor) == true) {                
        CGContextSetRGBFillColor(outContext, RGBFLOATRED(macColor), 
                RGBFLOATGREEN(macColor), 
                RGBFLOATBLUE(macColor), 1.0);
    }
    
    if(gc->function == GXxor) {
    }
    
    CGContextSetLineWidth(outContext, (float) gc->line_width);

    if (gc->line_style != LineSolid) {
        unsigned char *p = (unsigned char *) &(gc->dashes);
        /*
         * Here the dash pattern should be set in the drawing,
         * environment, but I don't know how to do that for the Mac.
         *
         * p[] is an array of unsigned chars containing the dash list.
         * A '\0' indicates the end of this list.
         *
         * Someone knows how to implement this? If you have a more
         * complete implementation of SetUpGraphicsPort() for
         * the Mac (or for Windows), please let me know.
         *
         *        Jan Nijtmans
         *        CMG Arnhem, B.V.
         *        email: j.nijtmans@chello.nl (private)
         *               jan.nijtmans@cmg.nl (work)
         *        url:   http://purl.oclc.org/net/nijtmans/
         *
         * FIXME:
         * This is not possible with QuickDraw line drawing, we either
         * have to convert all line drawings to regions, or, on Mac OS X
         * we can use CG to draw our lines instead of QuickDraw.
         */
    }
}

void
TkMacOSXReleaseCGContext(
        MacDrawable *macWin,
        CGrafPtr destPort, 
        CGContextRef *outContext)
{
    CGContextResetCTM(*outContext);
    CGContextRestoreGState(*outContext);
    QDEndCGContext(destPort, outContext);

}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXSetUpClippingRgn --
 *
 *        Set up the clipping region so that drawing only occurs on the
 *        specified X subwindow.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The clipping region in the current port is changed.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXSetUpClippingRgn(
    Drawable drawable)                /* Drawable to update. */
{
    MacDrawable *macDraw = (MacDrawable *) drawable;

    if (macDraw->winPtr != NULL) {
        if (macDraw->flags & TK_CLIP_INVALID) {
            TkMacOSXUpdateClipRgn(macDraw->winPtr);
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
                    SetGWorld(TkMacOSXGetDrawablePort(drawable), NULL);
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
 * TkMacOSXMakeStippleMap --
 *
 *        Given a drawable and a stipple pattern this function draws the
 *        pattern repeatedly over the drawable.  The drawable can then
 *        be used as a mask for bit-bliting a stipple pattern over an
 *        object.
 *
 * Results:
 *        A BitMap data structure.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

BitMapPtr
TkMacOSXMakeStippleMap(
    Drawable drawable,                /* Window to apply stipple. */
    Drawable stipple)                /* The stipple pattern. */
{
    GWorldPtr destPort;
    BitMapPtr bitmapPtr;
    Rect      portRect;
    int width, height, stippleHeight, stippleWidth;
    int i, j;
    char * data;
    Rect bounds;

    destPort = TkMacOSXGetDrawablePort(drawable);
    
    GetPortBounds ( destPort, &portRect );
    width = portRect.right - portRect.left;
    height = portRect.bottom - portRect.top;
    
    bitmapPtr = (BitMap *) ckalloc(sizeof(BitMap));
    data = (char *) ckalloc(height * ((width / 8) + 1));
    bitmapPtr->bounds.top = bitmapPtr->bounds.left = 0;
    bitmapPtr->bounds.right = (short) width;
    bitmapPtr->bounds.bottom = (short) height;
    bitmapPtr->baseAddr = data;
    bitmapPtr->rowBytes = (width / 8) + 1;

    destPort = TkMacOSXGetDrawablePort(stipple);
    stippleWidth = portRect.right - portRect.left;
    stippleHeight = portRect.bottom - portRect.top;

    for (i = 0; i < height; i += stippleHeight) {
        for (j = 0; j < width; j += stippleWidth) {
            bounds.left = j;
            bounds.top = i;
            bounds.right = j + stippleWidth;
            bounds.bottom = i + stippleHeight;
            
            CopyBits(GetPortBitMapForCopyBits ( destPort ), bitmapPtr, 
                &portRect, &bounds, srcCopy, NULL);
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
    unsigned char data)                /* Byte of data. */
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
 *        This procedure draws a rectangular ring around the outside of
 *        a widget to indicate that it has received the input focus.
 *
 *      On the Macintosh, this puts a 1 pixel border in the bgGC color
 *      between the widget and the focus ring, except in the case where 
 *      highlightWidth is 1, in which case the border is left out.
 *
 *      For proper Mac L&F, use highlightWidth of 3.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        A rectangle "width" pixels wide is drawn in "drawable",
 *        corresponding to the outer area of "tkwin".
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
