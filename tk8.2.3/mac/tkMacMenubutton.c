/* 
 * tkMacMenubutton.c --
 *
 *	This file implements the Macintosh specific portion of the
 *	menubutton widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkMenubutton.h"
#include "tkMacInt.h"
#include <Controls.h>

#define kShadowOffset				(3)	/* amount to offset shadow from frame */
#define kTriangleWidth				(11)	/* width of the triangle */
#define kTriangleHeight				(6)	/* height of the triangle */
#define kTriangleMargin				(5)	/* margin around triangle */

/*
 * Declaration of Unix specific button structure.
 */

typedef struct MacMenuButton {
    TkMenuButton info;		/* Generic button info. */
} MacMenuButton;

/*
 * The structure below defines menubutton class behavior by means of
 * procedures that can be invoked from generic window code.
 */

TkClassProcs tkpMenubuttonClass = {
    NULL,			/* createProc. */
    TkMenuButtonWorldChanged,	/* geometryProc. */
    NULL			/* modalProc. */
};

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateMenuButton --
 *
 *	Allocate a new TkMenuButton structure.
 *
 * Results:
 *	Returns a newly allocated TkMenuButton structure.
 *
 * Side effects:
 *	Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkMenuButton *
TkpCreateMenuButton(
    Tk_Window tkwin)
{
    MacMenuButton *butPtr = (MacMenuButton *)ckalloc(sizeof(MacMenuButton));

    return (TkMenuButton *) butPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayMenuButton --
 *
 *	This procedure is invoked to display a menubutton widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menubutton in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayMenuButton(
    ClientData clientData)	/* Information about widget. */
{
    TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    GC gc;
    Tk_3DBorder border;
    int x = 0;			/* Initialization needed only to stop
				 * compiler warning. */
    int y;
    Tk_Window tkwin = mbPtr->tkwin;
    int width, height;
    MacMenuButton * macMBPtr = (MacMenuButton *) mbPtr;
    GWorldPtr destPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    MacDrawable *macDraw;

    mbPtr->flags &= ~REDRAW_PENDING;
    if ((mbPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    GetGWorld(&saveWorld, &saveDevice);
    destPort = TkMacGetDrawablePort(Tk_WindowId(tkwin));
    SetGWorld(destPort, NULL);
    macDraw = (MacDrawable *) Tk_WindowId(tkwin);

    if ((mbPtr->state == STATE_DISABLED) && (mbPtr->disabledFg != NULL)) {
	gc = mbPtr->disabledGC;
    } else if ((mbPtr->state == STATE_ACTIVE)
	    && !Tk_StrictMotif(mbPtr->tkwin)) {
	gc = mbPtr->activeTextGC;
    } else {
	gc = mbPtr->normalTextGC;
    }
    border = mbPtr->normalBorder;

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the menu button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), border, 0, 0,
	    Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /*
     * Display image or bitmap or text for button.
     */

    if (mbPtr->image != None) {
	Tk_SizeOfImage(mbPtr->image, &width, &height);

	imageOrBitmap:
	TkComputeAnchor(mbPtr->anchor, tkwin, 0, 0, 
		width + mbPtr->indicatorWidth, height, &x, &y);
	if (mbPtr->image != NULL) {
	    Tk_RedrawImage(mbPtr->image, 0, 0, width, height,
		    Tk_WindowId(tkwin), x, y);
	} else {
	    XCopyPlane(mbPtr->display, mbPtr->bitmap, Tk_WindowId(tkwin),
		    gc, 0, 0, (unsigned) width, (unsigned) height, x, y, 1);
	}
    } else if (mbPtr->bitmap != None) {
	Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
	goto imageOrBitmap;
    } else {
	TkComputeAnchor(mbPtr->anchor, tkwin, mbPtr->padX, mbPtr->padY,
		mbPtr->textWidth + mbPtr->indicatorWidth, mbPtr->textHeight,
		&x, &y);
	Tk_DrawTextLayout(mbPtr->display, Tk_WindowId(tkwin), gc,
		mbPtr->textLayout, x, y, 0, -1);
    }

    /*
     * If the menu button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.
     */

    if ((mbPtr->state == STATE_DISABLED)
	    && ((mbPtr->disabledFg != NULL) || (mbPtr->image != NULL))) {
	XFillRectangle(mbPtr->display, Tk_WindowId(tkwin), 
                mbPtr->disabledGC, mbPtr->inset, mbPtr->inset,
		(unsigned) (Tk_Width(tkwin) - 2*mbPtr->inset),
		(unsigned) (Tk_Height(tkwin) - 2*mbPtr->inset));
    }

    /*
     * Draw the cascade indicator for the menu button on the
     * right side of the window, if desired.
     */

    if (mbPtr->indicatorOn) {
	int w, h, i;
	Rect r;

	r.left = macDraw->xOff + Tk_Width(tkwin) - mbPtr->inset
	    - mbPtr->indicatorWidth;
	r.top = macDraw->yOff + Tk_Height(tkwin)/2
	    - mbPtr->indicatorHeight/2;
	r.right = macDraw->xOff + Tk_Width(tkwin) - mbPtr->inset
	    - kTriangleMargin;
	r.bottom = macDraw->yOff + Tk_Height(tkwin)/2
	    + mbPtr->indicatorHeight/2;

	h = mbPtr->indicatorHeight;
	w = mbPtr->indicatorWidth - 1 - kTriangleMargin;
	for (i = 0; i < h; i++) {
	    MoveTo(r.left + i, r.top + i);
	    LineTo(r.left + i + w, r.top + i);
	    w -= 2;
	}
    }

    /*
     * Draw the border and traversal highlight last.  This way, if the
     * menu button's contents overflow onto the border they'll be covered
     * up by the border.
     */

    TkMacSetUpClippingRgn(Tk_WindowId(tkwin));
    if (mbPtr->borderWidth > 0) {
	Rect r;
	
	r.left = macDraw->xOff + mbPtr->highlightWidth + mbPtr->borderWidth;
	r.top = macDraw->yOff + mbPtr->highlightWidth + mbPtr->borderWidth;
	r.right = macDraw->xOff + Tk_Width(tkwin) - mbPtr->highlightWidth
	    - mbPtr->borderWidth;
	r.bottom = macDraw->yOff + Tk_Height(tkwin) - mbPtr->highlightWidth
	    - mbPtr->borderWidth;
	FrameRect(&r);

	PenSize(mbPtr->borderWidth - 1, mbPtr->borderWidth - 1);
	MoveTo(r.right, r.top + kShadowOffset);
	LineTo(r.right, r.bottom);
	LineTo(r.left + kShadowOffset, r.bottom);
    }
    
    if (mbPtr->highlightWidth != 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(mbPtr->highlightBgColorPtr, Tk_WindowId(tkwin));
	if (mbPtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(mbPtr->highlightColorPtr, Tk_WindowId(tkwin));
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, mbPtr->highlightWidth,
		    Tk_WindowId(tkwin));
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, mbPtr->highlightWidth,
		    Tk_WindowId(tkwin));
	}
    }

    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuButton --
 *
 *	Free data structures associated with the menubutton control.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Restores the default control state.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenuButton(
    TkMenuButton *mbPtr)
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeMenuButtonGeometry --
 *
 *	After changes in a menu button's text or bitmap, this procedure
 *	recomputes the menu button's geometry and passes this information
 *	along to the geometry manager for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu button's window may change size.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeMenuButtonGeometry(mbPtr)
    register TkMenuButton *mbPtr;		/* Widget record for menu button. */
{
    int width, height, mm, pixels;

    mbPtr->inset = mbPtr->highlightWidth + mbPtr->borderWidth;
    if (mbPtr->image != None) {
	Tk_SizeOfImage(mbPtr->image, &width, &height);
	if (mbPtr->width > 0) {
	    width = mbPtr->width;
	}
	if (mbPtr->height > 0) {
	    height = mbPtr->height;
	}
    } else if (mbPtr->bitmap != None) {
	Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
	if (mbPtr->width > 0) {
	    width = mbPtr->width;
	}
	if (mbPtr->height > 0) {
	    height = mbPtr->height;
	}
    } else {
	Tk_FreeTextLayout(mbPtr->textLayout);
	mbPtr->textLayout = Tk_ComputeTextLayout(mbPtr->tkfont, mbPtr->text,
		-1, mbPtr->wrapLength, mbPtr->justify, 0, &mbPtr->textWidth,
		&mbPtr->textHeight);
	width = mbPtr->textWidth;
	height = mbPtr->textHeight;
	if (mbPtr->width > 0) {
	    width = mbPtr->width * Tk_TextWidth(mbPtr->tkfont, "0", 1);
	}
	if (mbPtr->height > 0) {
	    Tk_FontMetrics fm;

	    Tk_GetFontMetrics(mbPtr->tkfont, &fm);
	    height = mbPtr->height * fm.linespace;
	}
	width += 2*mbPtr->padX;
	height += 2*mbPtr->padY;
    }

    if (mbPtr->indicatorOn) {
	mm = WidthMMOfScreen(Tk_Screen(mbPtr->tkwin));
	pixels = WidthOfScreen(Tk_Screen(mbPtr->tkwin));
	mbPtr->indicatorHeight= kTriangleHeight;
	mbPtr->indicatorWidth = kTriangleWidth + kTriangleMargin;
	width += mbPtr->indicatorWidth;
    } else {
	mbPtr->indicatorHeight = 0;
	mbPtr->indicatorWidth = 0;
    }

    Tk_GeometryRequest(mbPtr->tkwin, (int) (width + 2*mbPtr->inset),
	    (int) (height + 2*mbPtr->inset));
    Tk_SetInternalBorder(mbPtr->tkwin, mbPtr->inset);
}
