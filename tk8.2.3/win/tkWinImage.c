/* 
 * tkWinImage.c --
 *
 *	This file contains routines for manipulation full-color images.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"

static int		DestroyImage _ANSI_ARGS_((XImage* data));
static unsigned long	ImageGetPixel _ANSI_ARGS_((XImage *image, int x, int y));
static int		PutPixel _ANSI_ARGS_((XImage *image, int x, int y,
			    unsigned long pixel));

/*
 *----------------------------------------------------------------------
 *
 * DestroyImage --
 *
 *	This is a trivial wrapper around ckfree to make it possible to
 *	pass ckfree as a pointer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates the image.
 *
 *----------------------------------------------------------------------
 */

int
DestroyImage(imagePtr)
     XImage *imagePtr;		/* image to free */
{
    if (imagePtr) {
	if (imagePtr->data) {
	    ckfree((char*)imagePtr->data);
	}
	ckfree((char*)imagePtr);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageGetPixel --
 *
 *	Get a single pixel from an image.
 *
 * Results:
 *	Returns the 32 bit pixel value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
ImageGetPixel(image, x, y)
    XImage *image;
    int x, y;
{
    unsigned long pixel = 0;
    unsigned char *srcPtr = &(image->data[(y * image->bytes_per_line)
	    + ((x * image->bits_per_pixel) / NBBY)]);

    switch (image->bits_per_pixel) {
	case 32:
	case 24:
	    pixel = RGB(srcPtr[2], srcPtr[1], srcPtr[0]);
	    break;
	case 16:
	    pixel = RGB(((((WORD*)srcPtr)[0]) >> 7) & 0xf8,
		    ((((WORD*)srcPtr)[0]) >> 2) & 0xf8,
		    ((((WORD*)srcPtr)[0]) << 3) & 0xf8);
	    break;
	case 8:
	    pixel = srcPtr[0];
	    break;
	case 4:
	    pixel = ((x%2) ? (*srcPtr) : ((*srcPtr) >> 4)) & 0x0f;
	    break;
	case 1:
	    pixel = ((*srcPtr) & (0x80 >> (x%8))) ? 1 : 0;
	    break;
    }
    return pixel;
}

/*
 *----------------------------------------------------------------------
 *
 * PutPixel --
 *
 *	Set a single pixel in an image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PutPixel(image, x, y, pixel)
    XImage *image;
    int x, y;
    unsigned long pixel;
{
    unsigned char *destPtr = &(image->data[(y * image->bytes_per_line)
	    + ((x * image->bits_per_pixel) / NBBY)]);

    switch  (image->bits_per_pixel) {
	case 32:
	    /*
	     * Pixel is DWORD: 0x00BBGGRR
	     */

	    destPtr[3] = 0;
	case 24:
	    /*
	     * Pixel is triplet: 0xBBGGRR.
	     */

	    destPtr[0] = (unsigned char) GetBValue(pixel);
	    destPtr[1] = (unsigned char) GetGValue(pixel);
	    destPtr[2] = (unsigned char) GetRValue(pixel);
	    break;
	case 16:
	    /*
	     * Pixel is WORD: 5-5-5 (R-G-B)
	     */

	    (*(WORD*)destPtr) = 
		((GetRValue(pixel) & 0xf8) << 7)
		| ((GetGValue(pixel) & 0xf8) <<2)
		| ((GetBValue(pixel) & 0xf8) >> 3);
	    break;
	case 8:
	    /*
	     * Pixel is 8-bit index into color table.
	     */

	    (*destPtr) = (unsigned char) pixel;
	    break;
	case 4:
	    /*
	     * Pixel is 4-bit index in MSBFirst order.
	     */
	    if (x%2) {
		(*destPtr) = (unsigned char) (((*destPtr) & 0xf0)
		    | (pixel & 0x0f));
	    } else {
		(*destPtr) = (unsigned char) (((*destPtr) & 0x0f)
		    | ((pixel << 4) & 0xf0));
	    }
	    break;
	case 1: {
	    /*
	     * Pixel is bit in MSBFirst order.
	     */

	    int mask = (0x80 >> (x%8));
	    if (pixel) {
		(*destPtr) |= mask;
	    } else {
		(*destPtr) &= ~mask;
	    }
	}
	break;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * XCreateImage --
 *
 *	Allocates storage for a new XImage.
 *
 * Results:
 *	Returns a newly allocated XImage.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

XImage *
XCreateImage(display, visual, depth, format, offset, data, width, height,
	bitmap_pad, bytes_per_line)
    Display* display;
    Visual* visual;
    unsigned int depth;
    int format;
    int offset;
    char* data;
    unsigned int width;
    unsigned int height;
    int bitmap_pad;
    int bytes_per_line;
{
    XImage* imagePtr = (XImage *) ckalloc(sizeof(XImage));
    imagePtr->width = width;
    imagePtr->height = height;
    imagePtr->xoffset = offset;
    imagePtr->format = format;
    imagePtr->data = data;
    imagePtr->byte_order = LSBFirst;
    imagePtr->bitmap_unit = 8;
    imagePtr->bitmap_bit_order = MSBFirst;
    imagePtr->bitmap_pad = bitmap_pad;
    imagePtr->bits_per_pixel = depth;
    imagePtr->depth = depth;

    /*
     * Under Windows, bitmap_pad must be on an LONG data-type boundary.
     */

#define LONGBITS    (sizeof(LONG) * 8)

    bitmap_pad = (bitmap_pad + LONGBITS - 1) / LONGBITS * LONGBITS;

    /*
     * Round to the nearest bitmap_pad boundary.
     */

    if (bytes_per_line) {
	imagePtr->bytes_per_line = bytes_per_line;
    } else {
	imagePtr->bytes_per_line = (((depth * width)
		+ (bitmap_pad - 1)) >> 3) & ~((bitmap_pad >> 3) - 1);
    }

    imagePtr->red_mask = 0;
    imagePtr->green_mask = 0;
    imagePtr->blue_mask = 0;

    imagePtr->f.put_pixel = PutPixel;
    imagePtr->f.get_pixel = ImageGetPixel;
    imagePtr->f.destroy_image = DestroyImage;
    imagePtr->f.create_image = NULL;
    imagePtr->f.sub_image = NULL;
    imagePtr->f.add_pixel = NULL;
    
    return imagePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetImage --
 *
 *	This function copies data from a pixmap or window into an
 *	XImage.
 *
 * Results:
 *	Returns a newly allocated image containing the data from the
 *	given rectangle of the given drawable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

XImage *
XGetImage(display, d, x, y, width, height, plane_mask, format)
    Display* display;
    Drawable d;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned long plane_mask;
    int	format;
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)d;
    XImage *imagePtr;
    HDC dc;
    char infoBuf[sizeof(BITMAPINFO) + sizeof(RGBQUAD)];
    BITMAPINFO *infoPtr = (BITMAPINFO*)infoBuf;

    if ((twdPtr->type != TWD_BITMAP) || (twdPtr->bitmap.handle == NULL)
	    || (format != XYPixmap) || (plane_mask != 1)) {
	panic("XGetImage: not implemented");
    }


    imagePtr = XCreateImage(display, NULL, 1, XYBitmap, 0, NULL,
	    width, height, 32, 0);
    imagePtr->data = ckalloc(imagePtr->bytes_per_line * imagePtr->height);

    dc = GetDC(NULL);

    GetDIBits(dc, twdPtr->bitmap.handle, 0, height, NULL,
	    infoPtr, DIB_RGB_COLORS);

    infoPtr->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoPtr->bmiHeader.biWidth = width;
    infoPtr->bmiHeader.biHeight = -(LONG)height;
    infoPtr->bmiHeader.biPlanes = 1;
    infoPtr->bmiHeader.biBitCount = 1;
    infoPtr->bmiHeader.biCompression = BI_RGB;
    infoPtr->bmiHeader.biCompression = 0;
    infoPtr->bmiHeader.biXPelsPerMeter = 0;
    infoPtr->bmiHeader.biYPelsPerMeter = 0;
    infoPtr->bmiHeader.biClrUsed = 0;
    infoPtr->bmiHeader.biClrImportant = 0;

    GetDIBits(dc, twdPtr->bitmap.handle, 0, height, imagePtr->data,
	    infoPtr, DIB_RGB_COLORS);
    ReleaseDC(NULL, dc);

    return imagePtr;
}
