/* 
 * tkUnixMenubu.c --
 *
 *	This file implements the Unix specific portion of the
 *	menubutton widget.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkMenubutton.h"

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
TkpCreateMenuButton(tkwin)
    Tk_Window tkwin;
{
    return (TkMenuButton *)ckalloc(sizeof(TkMenuButton));
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
TkpDisplayMenuButton(clientData)
    ClientData clientData;	/* Information about widget. */
{
    register TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    GC gc;
    Tk_3DBorder border;
    Pixmap pixmap;
    int x = 0;			/* Initialization needed only to stop
				 * compiler warning. */
    int y;
    register Tk_Window tkwin = mbPtr->tkwin;
    int width, height;

    mbPtr->flags &= ~REDRAW_PENDING;
    if ((mbPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    if ((mbPtr->state == STATE_DISABLED) && (mbPtr->disabledFg != NULL)) {
	gc = mbPtr->disabledGC;
	border = mbPtr->normalBorder;
    } else if ((mbPtr->state == STATE_ACTIVE)
	       && !Tk_StrictMotif(mbPtr->tkwin)) {
	gc = mbPtr->activeTextGC;
	border = mbPtr->activeBorder;
    } else {
	gc = mbPtr->normalTextGC;
	border = mbPtr->normalBorder;
    }

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the menu button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    pixmap = Tk_GetPixmap(mbPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
    Tk_Fill3DRectangle(tkwin, pixmap, border, 0, 0, Tk_Width(tkwin),
	    Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /*
     * Display image or bitmap or text for button.
     */

    if (mbPtr->image != None) {
	Tk_SizeOfImage(mbPtr->image, &width, &height);

	imageOrBitmap:
	TkComputeAnchor(mbPtr->anchor, tkwin, 0, 0, 
		width + mbPtr->indicatorWidth, height, &x, &y);
	if (mbPtr->image != NULL) {
	    Tk_RedrawImage(mbPtr->image, 0, 0, width, height, pixmap,
		    x, y);
	} else {
	    XCopyPlane(mbPtr->display, mbPtr->bitmap, pixmap,
		    gc, 0, 0, (unsigned) width, (unsigned) height, x, y, 1);
	}
    } else if (mbPtr->bitmap != None) {
	Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
	goto imageOrBitmap;
    } else {
	TkComputeAnchor(mbPtr->anchor, tkwin, mbPtr->padX, mbPtr->padY,
		mbPtr->textWidth + mbPtr->indicatorWidth,
		mbPtr->textHeight, &x, &y);
	Tk_DrawTextLayout(mbPtr->display, pixmap, gc, mbPtr->textLayout, x, y,
		0, -1);
	Tk_UnderlineTextLayout(mbPtr->display, pixmap, gc, mbPtr->textLayout,
		x, y, mbPtr->underline);
    }

    /*
     * If the menu button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.
     */

    if ((mbPtr->state == STATE_DISABLED) 
            && ((mbPtr->disabledFg == NULL) || (mbPtr->image != NULL))) {
	XFillRectangle(mbPtr->display, pixmap, mbPtr->disabledGC,
		mbPtr->inset, mbPtr->inset,
		(unsigned) (Tk_Width(tkwin) - 2*mbPtr->inset),
		(unsigned) (Tk_Height(tkwin) - 2*mbPtr->inset));
    }

    /*
     * Draw the cascade indicator for the menu button on the
     * right side of the window, if desired.
     */

    if (mbPtr->indicatorOn) {
	int borderWidth;

	borderWidth = (mbPtr->indicatorHeight+1)/3;
	if (borderWidth < 1) {
	    borderWidth = 1;
	}
	/*y += mbPtr->textHeight / 2;*/
	Tk_Fill3DRectangle(tkwin, pixmap, border,
		Tk_Width(tkwin) - mbPtr->inset - mbPtr->indicatorWidth
		+ mbPtr->indicatorHeight,
		((int) (Tk_Height(tkwin) - mbPtr->indicatorHeight))/2,
		mbPtr->indicatorWidth - 2*mbPtr->indicatorHeight,
		mbPtr->indicatorHeight, borderWidth, TK_RELIEF_RAISED);
    }

    /*
     * Draw the border and traversal highlight last.  This way, if the
     * menu button's contents overflow onto the border they'll be covered
     * up by the border.
     */

    if (mbPtr->relief != TK_RELIEF_FLAT) {
	Tk_Draw3DRectangle(tkwin, pixmap, border,
		mbPtr->highlightWidth, mbPtr->highlightWidth,
		Tk_Width(tkwin) - 2*mbPtr->highlightWidth,
		Tk_Height(tkwin) - 2*mbPtr->highlightWidth,
		mbPtr->borderWidth, mbPtr->relief);
    }
    if (mbPtr->highlightWidth != 0) {
	GC gc;

	if (mbPtr->flags & GOT_FOCUS) {
	    gc = Tk_GCForColor(mbPtr->highlightColorPtr, pixmap);
	} else {
	    gc = Tk_GCForColor(mbPtr->highlightBgColorPtr, pixmap);
	}
	Tk_DrawFocusHighlight(tkwin, gc, mbPtr->highlightWidth, pixmap);
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */

    XCopyArea(mbPtr->display, pixmap, Tk_WindowId(tkwin),
	    mbPtr->normalTextGC, 0, 0, (unsigned) Tk_Width(tkwin),
	    (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(mbPtr->display, pixmap);
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
TkpDestroyMenuButton(mbPtr)
    TkMenuButton *mbPtr;
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
    TkMenuButton *mbPtr;	/* Widget record for menu button. */
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
	mbPtr->indicatorHeight= (INDICATOR_HEIGHT * pixels)/(10*mm);
	mbPtr->indicatorWidth = (INDICATOR_WIDTH * pixels)/(10*mm)
		+ 2*mbPtr->indicatorHeight;
	width += mbPtr->indicatorWidth;
    } else {
	mbPtr->indicatorHeight = 0;
	mbPtr->indicatorWidth = 0;
    }

    Tk_GeometryRequest(mbPtr->tkwin, (int) (width + 2*mbPtr->inset),
	    (int) (height + 2*mbPtr->inset));
    Tk_SetInternalBorder(mbPtr->tkwin, mbPtr->inset);
}
