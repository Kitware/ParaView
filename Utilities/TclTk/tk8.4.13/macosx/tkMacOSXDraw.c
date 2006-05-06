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

#include "tkMacOSXInt.h"
#include "tkMacOSXDebug.h"

#include "tclInt.h" /* for Tcl_CreateNamespace() */

#ifndef PI
#    define PI 3.14159265358979323846
#endif
#define RGBFLOATRED(c)   (float)((float)(c.red)   / 65535.0f)
#define RGBFLOATGREEN(c) (float)((float)(c.green) / 65535.0f)
#define RGBFLOATBLUE(c)  (float)((float)(c.blue)  / 65535.0f)

/*
 * Temporary regions that can be reused.
 */

static RgnHandle tmpRgn = NULL;
static RgnHandle tmpRgn2 = NULL;

static PixPatHandle gPenPat = NULL;

static int useCGDrawing = 1;
static int tkMacOSXCGAntiAliasLimit = 1;

static int useThemedToplevel = 0;
static int useThemedFrame = 0;

/*
 * Prototypes for functions used only in this file.
 */
static unsigned char    InvertByte _ANSI_ARGS_((unsigned char data));

static void TkMacOSXSetUpCGContext(MacDrawable *macWin,
  CGrafPtr destPort, GC gc,  CGContextRef *contextPtr);
static void TkMacOSXReleaseCGContext(MacDrawable *macWin, CGrafPtr destPort, 
  CGContextRef *context);
static inline double radians(double degrees) { return degrees * PI / 180.0f; }

int 
TkMacOSXInitCGDrawing(interp, enable, limit)
        Tcl_Interp *interp;
        int enable;
        int limit;
{
    static Boolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;
        
        if (Tcl_CreateNamespace(interp, "::tk::mac", NULL, NULL) == NULL) {
            Tcl_ResetResult(interp);
        }
        if (Tcl_LinkVar(interp, "::tk::mac::useCGDrawing",
                (char *) &useCGDrawing, TCL_LINK_BOOLEAN) != TCL_OK) {
            Tcl_ResetResult(interp);
        }
        useCGDrawing = enable;

        if (Tcl_LinkVar(interp, "::tk::mac::CGAntialiasLimit",
                (char *) &tkMacOSXCGAntiAliasLimit, TCL_LINK_INT) != TCL_OK) {
            Tcl_ResetResult(interp);
        }
        tkMacOSXCGAntiAliasLimit = limit;

  /*
   * Piggy-back the themed drawing var init here.
   */
        if (Tcl_LinkVar(interp, "::tk::mac::useThemedToplevel",
        (char *) &useThemedToplevel, TCL_LINK_BOOLEAN) != TCL_OK) {
            Tcl_ResetResult(interp);
        }
        if (Tcl_LinkVar(interp, "::tk::mac::useThemedFrame",
        (char *) &useThemedFrame, TCL_LINK_BOOLEAN) != TCL_OK) {
            Tcl_ResetResult(interp);
        }
    }
    return TCL_OK;
}

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
            (short) (srcDraw->xOff + src_x + width),
            (short) (srcDraw->yOff + src_y + height));
    if (tkPictureIsOpen) {
        dstPtr = &srcRect;
    } else {
        dstPtr = &dstRect;
        SetRect(&dstRect, (short) (dstDraw->xOff + dest_x),
            (short) (dstDraw->yOff + dest_y), 
            (short) (dstDraw->xOff + dest_x + width),
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
    if (!gc->clip_mask) { 
    } else if (((TkpClipMask*)gc->clip_mask)->type == TKP_CLIP_REGION) {
        RgnHandle clipRgn = (RgnHandle)
                ((TkpClipMask*)gc->clip_mask)->value.region;
 
        int xOffset = 0, yOffset = 0;
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
    srcBit = GetPortBitMapForCopyBits(srcPort);
    dstBit = GetPortBitMapForCopyBits(dstPort);
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
    TkpClipMask *clipPtr = (TkpClipMask *) gc->clip_mask;
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


    srcBit = GetPortBitMapForCopyBits(srcPort);
    dstBit = GetPortBitMapForCopyBits(dstPort);
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

            pm = GetPortPixMap(srcPort);
            if (GetPixDepth(pm) == 1) {
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
            mskBit = GetPortBitMapForCopyBits(mskPort);
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
    unsigned long *colors,  /* Unused on Macintosh. */
    int ncolors,    /* Unused on Macintosh. */
    Display* display,    /* Display. */
    Drawable d,      /* Drawable to place image on. */
    GC gc,      /* GC to use. */
    XImage* image,    /* Image to place. */
    int src_x,      /* Source X & Y. */
    int src_y,
    int dest_x,      /* Destination X & Y. */
    int dest_y,
    unsigned int width,    /* Same width & height for both */
    unsigned int height)  /* distination and source. */
{
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    const BitMap * destBits;
    MacDrawable *dstDraw = (MacDrawable *) d;
    int i, j;
    BitMap bitmap;
    char *newData = NULL;
    Rect destRect, srcRect;

    destPort = TkMacOSXGetDrawablePort(d);
    SetRect(&destRect, dstDraw->xOff + dest_x, dstDraw->yOff + dest_y, 
            dstDraw->xOff + dest_x + width, dstDraw->yOff + dest_y + height);
    SetRect(&srcRect, src_x, src_y, src_x + width, src_y + height);

    display->request++;
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    if (image->obdata) {
        /* Image from XGetImage, copy from containing GWorld directly */
        GWorldPtr srcPort = TkMacOSXGetDrawablePort((Drawable)image->obdata);
        CopyBits(GetPortBitMapForCopyBits(srcPort),
                GetPortBitMapForCopyBits(destPort),
                &srcRect, &destRect, srcCopy, NULL);
    } else if (image->depth == 1) {
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
      size_t size = image->height * image->bytes_per_line;
            newData = (char *) ckalloc((int) size);
            for (i = 0; i < size; i++) {
                newData[i] = InvertByte((unsigned char) image->data[i]);
            }
            bitmap.baseAddr = newData;
            bitmap.rowBytes = image->bytes_per_line;
        }
        destBits = GetPortBitMapForCopyBits(destPort);
        CopyBits(&bitmap, destBits, &srcRect, &destRect, srcCopy, NULL);
    } else {
        /*
         * Color image
         */
        PixMap pixmap;

        pixmap.bounds.left = 0;
        pixmap.bounds.top = 0;
        pixmap.bounds.right = (short) image->width;
        pixmap.bounds.bottom = (short) image->height;
        pixmap.pixelType = RGBDirect;
        pixmap.pmVersion = baseAddr32;        /* 32bit clean */
        pixmap.packType = 0;
        pixmap.packSize = 0;
        pixmap.hRes = 0x00480000;
        pixmap.vRes = 0x00480000;
        pixmap.pixelSize = 32;
        pixmap.cmpCount = 3;
        pixmap.cmpSize = 8;
#ifdef WORDS_BIGENDIAN
        pixmap.pixelFormat = k32ARGBPixelFormat;
#else
        pixmap.pixelFormat = k32BGRAPixelFormat;
#endif
        pixmap.pmTable = NULL;
        pixmap.pmExt = 0;
        pixmap.baseAddr = image->data;
        pixmap.rowBytes = image->bytes_per_line | 0x8000;

        CopyBits((BitMap *) &pixmap, GetPortBitMapForCopyBits(destPort), 
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
    int n_rectangles)                /* Number of rectangles. */
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
    if (useCGDrawing) {
        CGContextRef outContext;
  CGRect  rect;
        
        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

  for (i = 0; i < n_rectangles; i++) {
      rect = CGRectMake((float)(macWin->xOff + rectangles[i].x),
        (float)(macWin->yOff + rectangles[i].y),
        (float)rectangles[i].width,
        (float)rectangles[i].height);
      
      CGContextFillRect(outContext, rect);
  }
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
        TkMacOSXSetUpGraphicsPort(gc, destPort);

  for (i = 0; i < n_rectangles; i++) {
            theRect.left = (short) (macWin->xOff + rectangles[i].x);
            theRect.top = (short) (macWin->yOff + rectangles[i].y);
            theRect.right = (short) (theRect.left + rectangles[i].width);
            theRect.bottom = (short) (theRect.top + rectangles[i].height);
            FillCRect(&theRect, gPenPat);
        }
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
    Display* display,    /* Display. */
    Drawable d,      /* Draw on this. */
    GC gc,      /* Use this GC. */
    XPoint* points,    /* Array of points. */
    int npoints,    /* Number of points. */
    int mode)      /* Line drawing mode. */
{
    MacDrawable *macWin = (MacDrawable *) d;
    CGrafPtr saveWorld;
    GWorldPtr destPort;
    GDHandle saveDevice;
    int i;

    destPort = TkMacOSXGetDrawablePort(d);

    display->request++;
    if (npoints < 2) {
  return;   /* TODO: generate BadValue error. */
    }
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    TkMacOSXSetUpClippingRgn(d);

    if (useCGDrawing) {
  CGContextRef outContext;
  float prevx, prevy;

  TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

  CGContextBeginPath(outContext);
  prevx = (float) (macWin->xOff + points[0].x);
  prevy = (float) (macWin->yOff + points[0].y);
  CGContextMoveToPoint(outContext, prevx, prevy);

  for (i = 1; i < npoints; i++) {
      if (mode == CoordModeOrigin) {
    CGContextAddLineToPoint(outContext,
      (float) (macWin->xOff + points[i].x),
      (float) (macWin->yOff + points[i].y));
      } else {
    prevx += (float) points[i].x;
    prevy += (float) points[i].y;
    CGContextAddLineToPoint(outContext, prevx, prevy);
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
 *        Renders a series of unconnected lines.
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

        for (i = 0; i < nsegments; i++) {
      CGContextBeginPath(outContext);
            CGContextMoveToPoint(outContext,
                    (float)(macWin->xOff + segments[i].x1),
                    (float)(macWin->yOff + segments[i].y1));
            CGContextAddLineToPoint (outContext,
                    (float)(macWin->xOff + segments[i].x2),
                    (float)(macWin->yOff + segments[i].y2));
            CGContextStrokePath(outContext);

        }
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
    Display* display,    /* Display. */
    Drawable d,      /* Draw on this. */
    GC gc,      /* Use this GC. */
    XPoint* points,    /* Array of points. */
    int npoints,    /* Number of points. */
    int shape,      /* Shape to draw. */
    int mode)      /* Drawing mode. */
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
  float prevx, prevy;

  TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

  CGContextBeginPath(outContext);
  prevx = (float) (macWin->xOff + points[0].x);
  prevy = (float) (macWin->yOff + points[0].y);
  CGContextMoveToPoint(outContext, prevx, prevy);
  for (i = 1; i < npoints; i++) {
      if (mode == CoordModeOrigin) {
    CGContextAddLineToPoint(outContext, 
      (float)(macWin->xOff + points[i].x),
      (float)(macWin->yOff + points[i].y));
      } else {
    prevx += (float) points[i].x;
    prevy += (float) points[i].y;
    CGContextAddLineToPoint(outContext, prevx, prevy);
      }
  }
  CGContextEOFillPath(outContext);
  TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
  TkMacOSXSetUpGraphicsPort(gc, destPort);

  PenNormal();
  polygon = OpenPoly();

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
    if (useCGDrawing) {
        CGContextRef outContext;
  CGRect  rect;
        
        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

  rect = CGRectMake((float) ((float) macWin->xOff + (float) x),
          (float) ((float) macWin->yOff + (float) y),
          (float) width,
    (float) height);
  CGContextStrokeRect(outContext, rect);
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
        TkMacOSXSetUpGraphicsPort(gc, destPort);

        theRect.left = (short) (macWin->xOff + x);
        theRect.top = (short) (macWin->yOff + y);
        theRect.right = (short) (theRect.left + width);
        theRect.bottom = (short) (theRect.top + height);
        
        ShowPen();
        PenPixPat(gPenPat);
        FrameRect(&theRect);
        HidePen();
    }
    SetGWorld(saveWorld, saveDevice);
}

#if 0
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
    Rect     theRect;
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

    if (useCGDrawing) {
        CGContextRef outContext;
  CGRect  rect;

        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

        for (i = 0, rectPtr = rectArr; i < nRects; i++, rectPtr++) {
      rect = CGRectMake((float) ((float) macWin->xOff 
                    + (float) rectPtr->x),
        (float) ((float) macWin->yOff + (float) rectPtr->y),
        (float) rectPtr->width,
        (float) rectPtr->height);
      CGContextStrokeRect(outContext, rect);
  }
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
  TkMacOSXSetUpGraphicsPort(gc, destPort);

        ShowPen();
        PenPixPat(gPenPat);

        for (i = 0, rectPtr = rectArr; i < nRects;i++, rectPtr++) {
            theRect.left = (short) (macWin->xOff + rectPtr->x);
            theRect.top = (short) (macWin->yOff + rectPtr->y);
            theRect.right = (short) (theRect.left + rectPtr->width);
            theRect.bottom = (short) (theRect.top + rectPtr->height);
            FrameRect(&theRect);
        }
        HidePen();
    }
    SetGWorld(saveWorld, saveDevice);
}
#endif

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
  CGRect  boundingRect;
        int clockwise;
  float a,b;
  CGPoint  center;
  float arc1, arc2;

  if (angle2 > 0) {
      clockwise = 1;
  } else {
      clockwise = 0;
        }

        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

        /*
         * If we are drawing an oval, we have to squash the coordinate
         * system before drawing, since CGContextAddArcToPoint only draws
         * circles.
         */

        CGContextSaveGState(outContext);
  boundingRect = CGRectMake(  (float)(macWin->xOff + x),
          (float)(macWin->yOff + y),
          (float)(width),
          (float)(height));

  center = CGPointMake(CGRectGetMidX(boundingRect), 
                CGRectGetMidY(boundingRect));
  a = CGRectGetWidth(boundingRect)/2;
  b = CGRectGetHeight(boundingRect)/2;

  CGContextTranslateCTM(outContext, center.x, center.y);
  CGContextBeginPath(outContext);
  CGContextScaleCTM(outContext, a, b);
  arc1 = radians(-(angle1/64));
  arc2 = radians(-(angle2/64)) + arc1;
  CGContextAddArc(outContext, 0.0, 0.0, 1, arc1, arc2, clockwise);

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

#if 0
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
    if (useCGDrawing) {
        CGContextRef outContext;

        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);

  for (i=0, arcPtr = arcArr; i < nArcs; i++, arcPtr++) {
      CGRect  boundingRect;
      int clockwise;
      float a,b, arc1, arc2;
      CGPoint center;
     
      if (arcPtr[i].angle2 > 0) {
    clockwise = 1;
      } else {
    clockwise = 0;
      }

        /*
         * If we are drawing an oval, we have to squash the coordinate
         * system before drawing, since CGContextAddArcToPoint only draws
         * circles.
         */

      CGContextSaveGState(outContext);
      CGContextBeginPath(outContext);
      boundingRect = CGRectMake(  (float)(macWin->xOff + arcPtr[i].x),
          (float)(macWin->yOff + arcPtr[i].y),
          (float)arcPtr[i].width,
          (float)arcPtr[i].height);
      
      center = CGPointMake(CGRectGetMidX(boundingRect), 
                    CGRectGetMidY(boundingRect));
      a = CGRectGetWidth(boundingRect)/2;
      b = CGRectGetHeight(boundingRect)/2;
      
      CGContextTranslateCTM(outContext, center.x, center.y);
      CGContextScaleCTM(outContext, a, b);
      arc1 = radians(-(arcPtr[i].angle1/64));
      arc2 = radians(-(arcPtr[i].angle2/64)) + arc1;
      CGContextAddArc(outContext, 0.0, 0.0, 1, arc1, arc2, clockwise);
      CGContextRestoreGState(outContext);
      CGContextStrokePath(outContext);
  }
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
  TkMacOSXSetUpGraphicsPort(gc, destPort);

        ShowPen();
        PenPixPat(gPenPat);
        for (i = 0, arcPtr = arcArr;i < nArcs;i++, arcPtr++) {
            rect.left = (short) (macWin->xOff + arcPtr->x);
            rect.top = (short) (macWin->yOff + arcPtr->y);
            rect.right = (short) (rect.left + arcPtr->width);
            rect.bottom = (short) (rect.top + arcPtr->height);
            start = (short) (90 - (arcPtr->angle1 / 64));
            extent = (short) (-(arcPtr->angle2 / 64));
            FrameArc(&rect, start, extent);
        }
        HidePen();
    }
    SetGWorld(saveWorld, saveDevice);
}
#endif

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

    if (useCGDrawing) {
        CGContextRef outContext;
  CGRect  boundingRect;
        int clockwise;
  float a,b;
  CGPoint  center;

  if (angle2 > 0) {
            clockwise = 1;
  } else {
            clockwise = 0;
        }

        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);
  
  boundingRect = CGRectMake((float) (macWin->xOff + x),
          (float) (macWin->yOff + y),
    (float) width,
    (float) height);
  center = CGPointMake(CGRectGetMidX(boundingRect), 
                CGRectGetMidY(boundingRect));
  a = CGRectGetWidth(boundingRect)/2;
  b = CGRectGetHeight(boundingRect)/2;

  if (gc->arc_mode == ArcChord) {
    float arc1, arc2;

      CGContextBeginPath(outContext);
      CGContextTranslateCTM(outContext, center.x, center.y);
      CGContextScaleCTM(outContext, a, b);
      arc1 = radians(-(angle1/64));
      arc2 = radians(-(angle2/64)) + arc1;
      CGContextAddArc(outContext, 0.0, 0.0, 1, arc1, arc2, clockwise);
      CGContextFillPath(outContext);
  } else if (gc->arc_mode == ArcPieSlice) {
            float arc1, arc2;

      CGContextTranslateCTM(outContext, center.x, center.y);
      CGContextScaleCTM(outContext,a,b);
      arc1 = radians(-(angle1/64));
      arc2 = radians(-(angle2/64)) + arc1;
      CGContextAddArc(outContext, 0.0, 0.0, 1, arc1, arc2, clockwise);
      CGContextAddLineToPoint(outContext, 0.0f, 0.0f);
      CGContextClosePath(outContext);
      CGContextFillPath(outContext);
  }
  
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
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
    }
    SetGWorld(saveWorld, saveDevice);
}

#if 0
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

    if (useCGDrawing) {
        CGContextRef outContext;
        
        TkMacOSXSetUpCGContext(macWin, destPort, gc, &outContext);
  for (i = 0, arcPtr = arcArr; i < nArcs; i++, arcPtr++) {
      CGRect  boundingRect;
      int clockwise = arcPtr[i].angle1;
      float a,b;
      CGPoint center;
      
      if (arcPtr[i].angle2 > 0) {
    clockwise = 1;
      } else {
    clockwise = 0;
            }

      /*
       * If we are drawing an oval, we have to squash the coordinate
       * system before drawing, since CGContextAddArcToPoint only draws
       * circles.
       */

      if (gc->arc_mode == ArcChord) {
      
    CGContextBeginPath(outContext);
    boundingRect = CGRectMake((float)(macWin->xOff + arcPtr[i].x),
                  (float)(macWin->yOff + arcPtr[i].y),
      (float)arcPtr[i].width,
      (float)arcPtr[i].height);
    center = CGPointMake(CGRectGetMidX(boundingRect), 
                        CGRectGetMidY(boundingRect));
    a = CGRectGetWidth(boundingRect)/2.0;
    b = CGRectGetHeight(boundingRect)/2.0;
    
    angle = -(arcPtr->angle1/64.0)*PI/180.0;
    sin1 = sin(angle);
    cos1 = cos(angle);
    angle -= (arcPtr->angle2/64.0)*PI/180.0;
    sin2 = sin(angle);
    cos2 = cos(angle);
    vertex[0] = (CGRectGetMinX(boundingRect) 
                        + CGRectGetMaxX(boundingRect))/2.0;
    vertex[1] = (CGRectGetMaxY(boundingRect) 
                        + CGRectGetMinY(boundingRect))/2.0;
    center1[0] = vertex[0] + cos1*a;
    center1[1] = vertex[1] + sin1*b;
    center2[0] = vertex[0] + cos2*a;
    center2[1] = vertex[1] + sin2*b;

    CGContextScaleCTM(outContext, a, b);
    
    CGContextBeginPath(outContext);
    CGContextMoveToPoint(outContext, (float) vertex[0],
            (float) vertex[1]);
    CGContextAddLineToPoint(outContext,
      (float) (center1[0] + 0.5),
      (float) (center1[1] + 0.5));
    CGContextAddLineToPoint(outContext,
      (float) (center2[0] + 0.5),
            (float) (center2[1] + 0.5));
    CGContextFillPath(outContext);
      } else if (gc->arc_mode == ArcPieSlice) {
          float arc1, arc2;
    CGContextBeginPath(outContext);
    boundingRect = CGRectMake((float)(macWin->xOff + arcPtr[i].x),
            (float)(macWin->yOff + arcPtr[i].y),
      (float)arcPtr[i].width,
      (float)arcPtr[i].height);
    center = CGPointMake(CGRectGetMidX(boundingRect), 
                        CGRectGetMidY(boundingRect));
    a = CGRectGetWidth(boundingRect)/2;
    b = CGRectGetHeight(boundingRect)/2;
    
    CGContextTranslateCTM(outContext, center.x, center.y);
    CGContextScaleCTM(outContext, a, b);
    arc1 = radians(-(arcPtr[i].angle1/64));
    arc2 = radians(-(arcPtr[i].angle2/64)) + arc1;
    CGContextAddArc(outContext, 0.0, 0.0, 1, 
                        arc1, arc2, clockwise);
    CGContextAddLineToPoint(outContext, 0.0f, 0.0f);
    CGContextClosePath(outContext);
    CGContextFillPath(outContext);
      }
  }
        
        TkMacOSXReleaseCGContext(macWin, destPort, &outContext);
    } else {
    
        TkMacOSXSetUpGraphicsPort(gc, destPort);

        for (i = 0, arcPtr = arcArr;i<nArcs;i++, arcPtr++) {
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
    }
    SetGWorld(saveWorld, saveDevice);
}
#endif

#if 0
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
#endif

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
         * This is not possible with QuickDraw line drawing.  As of
         * Tk 8.4.7 we have a complete set of drawing routines using
         * CG, so there is no reason to support this here.
         */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXSetUpCGContext --
 *
 *        Set up a CGContext for the given graphics port.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

static void
TkMacOSXSetUpCGContext(
    MacDrawable *macWin,
    CGrafPtr destPort,
    GC gc,
    CGContextRef *contextPtr)
{
    RGBColor macColor;
    CGContextRef outContext;
    OSStatus err;
    Rect boundsRect;
    CGAffineTransform coordsTransform;
    static RgnHandle clipRgn = NULL;

    err = QDBeginCGContext(destPort, contextPtr);
    outContext = *contextPtr;

    /*
     * Now clip the CG Context to the port.  Note, we have already
     * set up the port with our clip region, so we can just get
     * the clip back out of there.  If we use the macWin->clipRgn
     * directly at this point, we get some odd drawing effects.
     * 
     * We also have to intersect our clip region with the port
     * visible region so we don't overwrite the window decoration.
     */

    if (!clipRgn) {
  clipRgn = NewRgn();
    }

    GetPortBounds(destPort, &boundsRect);

    RectRgn(clipRgn, &boundsRect);
    SectRegionWithPortClipRegion(destPort, clipRgn);
    SectRegionWithPortVisibleRegion(destPort, clipRgn);
    ClipCGContextToRegion(outContext, &boundsRect, clipRgn);
    SetEmptyRgn(clipRgn);

    /*
     * Note: You have to call SyncCGContextOriginWithPort
     * AFTER all the clip region manipulations.
     */

    SyncCGContextOriginWithPort(outContext, destPort);

    coordsTransform = CGAffineTransformMake(1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
      (float) (boundsRect.bottom - boundsRect.top));
    CGContextConcatCTM(outContext, coordsTransform);
    
    /* Now offset the CTM to the subwindow offset */

    if (TkSetMacColor(gc->foreground, &macColor) == true) {
  CGContextSetRGBFillColor(outContext,
    RGBFLOATRED(macColor), 
    RGBFLOATGREEN(macColor), 
    RGBFLOATBLUE(macColor),
    1.0f);
  CGContextSetRGBStrokeColor(outContext,
    RGBFLOATRED(macColor),
    RGBFLOATGREEN(macColor),
    RGBFLOATBLUE(macColor),
    1.0f);
    }
    
    if(gc->function == GXxor) {
    }
    
    CGContextSetLineWidth(outContext, (float) gc->line_width);

    /* When should we antialias? */
    if (gc->line_width < tkMacOSXCGAntiAliasLimit) {
  CGContextSetShouldAntialias(outContext, 0);
    } else {
  CGContextSetShouldAntialias(outContext, 1);
    }

    if (gc->line_style != LineSolid) { 
  int num = 0;
  char *p = &(gc->dashes);
  float dashOffset = (float) gc->dash_offset;
  float lengths[10];

  while (p[num] != '\0') {
      lengths[num] = (float) (p[num]);
      num++;
  }
  CGContextSetLineDash(outContext, dashOffset, lengths, num);
    }
    
    if (gc->cap_style == CapButt) {
  /*
   *  What about CapNotLast, CapProjecting?
   */

  CGContextSetLineCap(outContext, kCGLineCapButt);
    } else if (gc->cap_style == CapRound) {
  CGContextSetLineCap(outContext, kCGLineCapRound);
    } else if (gc->cap_style == CapProjecting) {
  CGContextSetLineCap(outContext, kCGLineCapSquare);
    }
   
    if (gc->join_style == JoinMiter) {
  CGContextSetLineJoin(outContext, kCGLineJoinMiter);
    } else if (gc->join_style == JoinRound) {
  CGContextSetLineJoin(outContext, kCGLineJoinRound);
    } else if (gc->join_style == JoinBevel) {
  CGContextSetLineJoin(outContext, kCGLineJoinBevel);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXReleaseCGContext --
 *
 *        Release the CGContext for the given graphics port.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

static void
TkMacOSXReleaseCGContext(
        MacDrawable *macWin,
        CGrafPtr destPort, 
        CGContextRef *outContext)
{
    CGContextSynchronize(*outContext);
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
    
    GetPortBounds (destPort, &portRect);
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
            
            CopyBits(GetPortBitMapForCopyBits(destPort), bitmapPtr, 
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
 * TkpDrawHighlightBorder --
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
            TkDrawInsetFocusHighlight (tkwin, fgGC, highlightWidth - 1, 
                    drawable, 0);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDrawFrame --
 *
 *  This procedure draws the rectangular frame area.  If the user
 *  has request themeing, it draws with a the background theme.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Draws inside the tkwin area.
 *
 *----------------------------------------------------------------------
 */

void
TkpDrawFrame (Tk_Window tkwin, Tk_3DBorder border,
  int highlightWidth, int borderWidth, int relief)
{
    if (useThemedToplevel && Tk_IsTopLevel(tkwin)) {
  /*
   * Currently only support themed toplevels, until we can better
   * factor this to handle individual windows (blanket theming of
   * frames will work for very few UIs).
   */
  Rect bounds;
  Point origin;
  CGrafPtr saveWorld;
  GDHandle saveDevice;
  XGCValues gcValues;
  GC gc;
  Pixmap pixmap;
  Display *display = Tk_Display(tkwin);

  pixmap = Tk_GetPixmap(display, Tk_WindowId(tkwin),
    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

  gc = Tk_GetGC(tkwin, 0, &gcValues);
  TkMacOSXWinBounds((TkWindow *) tkwin, &bounds);
  origin.v = -bounds.top;
  origin.h = -bounds.left;
  bounds.top = bounds.left = 0;
  bounds.right = Tk_Width(tkwin);
  bounds.bottom = Tk_Height(tkwin);

  GetGWorld(&saveWorld, &saveDevice);
  SetGWorld(TkMacOSXGetDrawablePort(pixmap), 0);
  ApplyThemeBackground(kThemeBackgroundWindowHeader, &bounds,
    kThemeStateActive, 32 /* depth */, true /* inColor */);
  QDSetPatternOrigin(origin);
  EraseRect(&bounds);
  SetGWorld(saveWorld, saveDevice);

  XCopyArea(display, pixmap, Tk_WindowId(tkwin),
    gc, 0, 0, bounds.right, bounds.bottom, 0, 0);
  Tk_FreePixmap(display, pixmap);
  Tk_FreeGC(display, gc);
    } else {
  Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin),
    border, highlightWidth, highlightWidth,
    Tk_Width(tkwin) - 2 * highlightWidth,
    Tk_Height(tkwin) - 2 * highlightWidth,
    borderWidth, relief);
    }
}
