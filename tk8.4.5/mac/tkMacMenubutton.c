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

#define kShadowOffset	(3)	/* amount to offset shadow from frame */
#define kTriangleWidth	(11)	/* width of the triangle */
#define kTriangleHeight	(6)	/* height of the triangle */
#define kTriangleMargin	(5)	/* margin around triangle */

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

Tk_ClassProcs tkpMenubuttonClass = {
    sizeof(Tk_ClassProcs),	/* size */
    TkMenuButtonWorldChanged,	/* worldChangedProc */
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
    int width, height, fullWidth, fullHeight;
    int imageWidth, imageHeight;
    int imageXOffset, imageYOffset, textXOffset, textYOffset;
    int haveImage = 0, haveText = 0;
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

    if (mbPtr->image != None) {
        Tk_SizeOfImage(mbPtr->image, &width, &height);
        haveImage = 1;
    } else if (mbPtr->bitmap != None) {
        Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
        haveImage = 1;
    }
    imageWidth  = width;
    imageHeight = height;

    haveText = (mbPtr->textWidth != 0 && mbPtr->textHeight != 0);

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the menu button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), border, 0, 0,
	    Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    imageXOffset = 0;
    imageYOffset = 0;
    textXOffset = 0;
    textYOffset = 0;
    fullWidth = 0;
    fullHeight = 0;

    if (mbPtr->compound != COMPOUND_NONE && haveImage && haveText) {
        switch ((enum compound) mbPtr->compound) {
            case COMPOUND_TOP:
            case COMPOUND_BOTTOM: {
                /* Image is above or below text */
                if (mbPtr->compound == COMPOUND_TOP) {
                    textYOffset = height + mbPtr->padY;
                } else {
                    imageYOffset = mbPtr->textHeight + mbPtr->padY;
                }
                fullHeight = height + mbPtr->textHeight + mbPtr->padY;
                fullWidth = (width > mbPtr->textWidth ? width :
                        mbPtr->textWidth);
                textXOffset = (fullWidth - mbPtr->textWidth)/2;
                imageXOffset = (fullWidth - width)/2;
                break;
            }
            case COMPOUND_LEFT:
            case COMPOUND_RIGHT: {
                /* Image is left or right of text */
                if (mbPtr->compound == COMPOUND_LEFT) {
                    textXOffset = width + mbPtr->padX;
                } else {
                    imageXOffset = mbPtr->textWidth + mbPtr->padX;
                }
                fullWidth = mbPtr->textWidth + mbPtr->padX + width;
                fullHeight = (height > mbPtr->textHeight ? height :
                        mbPtr->textHeight);
                textYOffset = (fullHeight - mbPtr->textHeight)/2;
                imageYOffset = (fullHeight - height)/2;
                break;
            }
            case COMPOUND_CENTER: {
                /* Image and text are superimposed */
                fullWidth = (width > mbPtr->textWidth ? width :
                        mbPtr->textWidth);
                fullHeight = (height > mbPtr->textHeight ? height :
                        mbPtr->textHeight);
                textXOffset = (fullWidth - mbPtr->textWidth)/2;
                imageXOffset = (fullWidth - width)/2;
                textYOffset = (fullHeight - mbPtr->textHeight)/2;
                imageYOffset = (fullHeight - height)/2;
                break;
            }
            case COMPOUND_NONE: {break;}
        }


        TkComputeAnchor(mbPtr->anchor, tkwin, 0, 0,
                mbPtr->indicatorWidth + fullWidth, fullHeight,
                &x, &y);

	imageXOffset += x;
	imageYOffset += y;
        if (mbPtr->image != NULL) {
            Tk_RedrawImage(mbPtr->image, 0, 0, width, height, Tk_WindowId(tkwin),
                    imageXOffset, imageYOffset);
        } else if (mbPtr->bitmap != None) {
            XCopyPlane(mbPtr->display, mbPtr->bitmap, Tk_WindowId(tkwin),
                    gc, 0, 0, (unsigned) width, (unsigned) height,
                    imageXOffset, imageYOffset, 1);
        }

	Tk_DrawTextLayout(mbPtr->display, Tk_WindowId(tkwin), gc,
		mbPtr->textLayout, x + textXOffset, y + textYOffset, 0, -1);
	Tk_UnderlineTextLayout(mbPtr->display, Tk_WindowId(tkwin), gc,
		mbPtr->textLayout, x + textXOffset, y + textYOffset,
		mbPtr->underline);
    } else if (haveImage) {
	TkComputeAnchor(mbPtr->anchor, tkwin, 0, 0,
		width + mbPtr->indicatorWidth, height, &x, &y);
	imageXOffset += x;
	imageYOffset += y;
	if (mbPtr->image != NULL) {
	    Tk_RedrawImage(mbPtr->image, 0, 0, width, height, Tk_WindowId(tkwin),
		    imageXOffset, imageYOffset);
	} else if (mbPtr->bitmap != None) {
	    XCopyPlane(mbPtr->display, mbPtr->bitmap, Tk_WindowId(tkwin),
		    gc, 0, 0, (unsigned) width, (unsigned) height, 
		    x, y, 1);
	}
    } else {
	TkComputeAnchor(mbPtr->anchor, tkwin, mbPtr->padX, mbPtr->padY,
		mbPtr->textWidth + mbPtr->indicatorWidth,
		mbPtr->textHeight, &x, &y);
	Tk_DrawTextLayout(mbPtr->display, Tk_WindowId(tkwin), gc,
		mbPtr->textLayout, x + textXOffset, y + textYOffset, 0, -1);
	Tk_UnderlineTextLayout(mbPtr->display, Tk_WindowId(tkwin), gc,
		mbPtr->textLayout, x + textXOffset, y + textYOffset,
		mbPtr->underline);
    }

#if 0		/* this is the original code */
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
#endif

    /*
     * If the menu button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.
     */

    if ((mbPtr->state == STATE_DISABLED)
	    && ((mbPtr->disabledFg != NULL) || (mbPtr->image != NULL))) {
	/*
	 * Stipple the whole button if no disabledFg was specified,
	 * otherwise restrict stippling only to displayed image
	 */
	if (mbPtr->disabledFg == NULL) {
	    XFillRectangle(mbPtr->display, Tk_WindowId(tkwin),
		    mbPtr->stippleGC, mbPtr->inset, mbPtr->inset,
		    (unsigned) (Tk_Width(tkwin) - 2*mbPtr->inset),
		    (unsigned) (Tk_Height(tkwin) - 2*mbPtr->inset));
	} else {
	    XFillRectangle(mbPtr->display, Tk_WindowId(tkwin),
		    mbPtr->stippleGC, imageXOffset, imageYOffset,
		    (unsigned) imageWidth, (unsigned) imageHeight);
	}
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
    int width=0, height=0, textwidth=0, textheight=0, mm, pixels, noimage=0;

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
    noimage=1;
    }
    
    if ( noimage || mbPtr->compound != COMPOUND_NONE ) {
	Tk_FreeTextLayout(mbPtr->textLayout);
	mbPtr->textLayout = Tk_ComputeTextLayout(mbPtr->tkfont, mbPtr->text,
		-1, mbPtr->wrapLength, mbPtr->justify, 0, &mbPtr->textWidth,
		&mbPtr->textHeight);
	textwidth = mbPtr->textWidth;
	textheight = mbPtr->textHeight;
	if (mbPtr->width > 0) {
	    textwidth = mbPtr->width * Tk_TextWidth(mbPtr->tkfont, "0", 1);
	}
	if (mbPtr->height > 0) {
	    Tk_FontMetrics fm;

	    Tk_GetFontMetrics(mbPtr->tkfont, &fm);
	    textheight = mbPtr->height * fm.linespace;
	}
	textwidth += 2*mbPtr->padX;
	textheight +=  2*mbPtr->padY;
    }
    
	switch ((enum compound) mbPtr->compound) {
	  case COMPOUND_TOP:
	  case COMPOUND_BOTTOM: {
	      height += textheight + mbPtr->padY;
	      width = (width > textwidth ? width : textwidth);
	      break;
	  }
	  case COMPOUND_LEFT:
	  case COMPOUND_RIGHT: {
	      height = (height > textheight ? height : textheight);
	      width += textwidth + mbPtr->padX;
	      break;
	  }
	  case COMPOUND_CENTER: {
	      height = (height > textheight ? height : textheight);
	      width = (width > textwidth ? width : textwidth);
	      break;
	  }
	  case COMPOUND_NONE: {
	     if (noimage) {
	     	height = textheight;
	     	width = textwidth;
	     }
	     break;
	}
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
