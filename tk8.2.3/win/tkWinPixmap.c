/* 
 * tkWinPixmap.c --
 *
 *	This file contains the Xlib emulation functions pertaining to
 *	creating and destroying pixmaps.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Tk_GetPixmap --
 *
 *	Creates an in memory drawing surface.
 *
 * Results:
 *	Returns a handle to a new pixmap.
 *
 * Side effects:
 *	Allocates a new Win32 bitmap.
 *
 *----------------------------------------------------------------------
 */

Pixmap
Tk_GetPixmap(display, d, width, height, depth)
    Display* display;
    Drawable d;
    int width;
    int height;
    int depth;
{
    TkWinDrawable *newTwdPtr, *twdPtr;
    int planes;
    Screen *screen;
    
    display->request++;

    newTwdPtr = (TkWinDrawable*) ckalloc(sizeof(TkWinDrawable));
    newTwdPtr->type = TWD_BITMAP;
    newTwdPtr->bitmap.depth = depth;
    twdPtr = (TkWinDrawable *)d;
    if (twdPtr->type != TWD_BITMAP) {
	if (twdPtr->window.winPtr == NULL) {
	    newTwdPtr->bitmap.colormap = DefaultColormap(display,
		    DefaultScreen(display));
	} else {
	    newTwdPtr->bitmap.colormap = twdPtr->window.winPtr->atts.colormap;
	}
    } else {
	newTwdPtr->bitmap.colormap = twdPtr->bitmap.colormap;
    }
    screen = &display->screens[0];
    planes = 1;
    if (depth == screen->root_depth) {
	planes = (int) screen->ext_data;
	depth /= planes;
    }
    newTwdPtr->bitmap.handle = CreateBitmap(width, height, planes, depth, NULL);

    if (newTwdPtr->bitmap.handle == NULL) {
	ckfree((char *) newTwdPtr);
	return None;
    }
    
    return (Pixmap)newTwdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreePixmap --
 *
 *	Release the resources associated with a pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the bitmap created by Tk_GetPixmap.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreePixmap(display, pixmap)
    Display* display;
    Pixmap pixmap;
{
    TkWinDrawable *twdPtr = (TkWinDrawable *) pixmap;

    display->request++;
    if (twdPtr != NULL) {
	DeleteObject(twdPtr->bitmap.handle);
	ckfree((char *)twdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetPixmapColormap --
 *
 *	The following function is a hack used by the photo widget to
 *	explicitly set the colormap slot of a Pixmap.
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
TkSetPixmapColormap(pixmap, colormap)
    Pixmap pixmap;
    Colormap colormap;
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)pixmap;
    twdPtr->bitmap.colormap = colormap;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetGeometry --
 *
 *	Retrieve the geometry of the given drawable.  Note that
 *	this is a degenerate implementation that only returns the
 *	size of a pixmap.
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
XGetGeometry(display, d, root_return, x_return, y_return, width_return,
	height_return, border_width_return, depth_return)
    Display* display;
    Drawable d;
    Window* root_return;
    int* x_return;
    int* y_return;
    unsigned int* width_return;
    unsigned int* height_return;
    unsigned int* border_width_return;
    unsigned int* depth_return;
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)d;
    HDC dc;
    BITMAPINFO info;

    if ((twdPtr->type != TWD_BITMAP) || (twdPtr->bitmap.handle == NULL)) {
	panic("XGetGeometry: invalid pixmap");
    }
    dc = GetDC(NULL);
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biBitCount = 0;
    if (!GetDIBits(dc, twdPtr->bitmap.handle, 0, 0, NULL, &info,
	    DIB_RGB_COLORS)) {
	panic("XGetGeometry: unable to get bitmap size");
    }
    ReleaseDC(NULL, dc);

    *width_return = info.bmiHeader.biWidth;
    *height_return = info.bmiHeader.biHeight;
    return 1;
}
