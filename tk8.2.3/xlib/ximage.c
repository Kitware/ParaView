/* 
 * ximage.c --
 *
 *	X bitmap and image routines.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"


/*
 *----------------------------------------------------------------------
 *
 * XCreateBitmapFromData --
 *
 *	Construct a single plane pixmap from bitmap data.
 *
 *	NOTE: This procedure has the correct behavior on Windows and
 *	the Macintosh, but not on UNIX.  This is probably because the
 *	emulation for XPutImage on those platforms compensates for whatever
 *	is wrong here :-)
 *
 * Results:
 *	Returns a new Pixmap.
 *
 * Side effects:
 *	Allocates a new bitmap and drawable.
 *
 *----------------------------------------------------------------------
 */

Pixmap
XCreateBitmapFromData(display, d, data, width, height)
    Display* display;
    Drawable d;
    _Xconst char* data;
    unsigned int width;
    unsigned int height;
{
    XImage ximage;
    GC gc;
    Pixmap pix;

    pix = Tk_GetPixmap(display, d, width, height, 1);
    gc = XCreateGC(display, pix, 0, NULL);
    if (gc == NULL) {
	return None;
    }
    ximage.height = height;
    ximage.width = width;
    ximage.depth = 1;
    ximage.bits_per_pixel = 1;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = (char *)data;
    ximage.byte_order = LSBFirst;
    ximage.bitmap_unit = 8;
    ximage.bitmap_bit_order = LSBFirst;
    ximage.bitmap_pad = 8;
    ximage.bytes_per_line = (width+7)/8;

    TkPutImage(NULL, 0, display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
    XFreeGC(display, gc);
    return pix;
}
