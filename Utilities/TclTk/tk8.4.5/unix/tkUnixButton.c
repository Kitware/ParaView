/* 
 * tkUnixButton.c --
 *
 *	This file implements the Unix specific portion of the button
 *	widgets.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkButton.h"

/*
 * Declaration of Unix specific button structure.
 */

typedef struct UnixButton {
    TkButton info;		/* Generic button info. */
} UnixButton;

/*
 * The class procedure table for the button widgets.
 */

Tk_ClassProcs tkpButtonProcs = {
    sizeof(Tk_ClassProcs),	/* size */
    TkButtonWorldChanged,	/* worldChangedProc */
};

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateButton --
 *
 *	Allocate a new TkButton structure.
 *
 * Results:
 *	Returns a newly allocated TkButton structure.
 *
 * Side effects:
 *	Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkButton *
TkpCreateButton(tkwin)
    Tk_Window tkwin;
{
    UnixButton *butPtr = (UnixButton *)ckalloc(sizeof(UnixButton));
    return (TkButton *) butPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayButton --
 *
 *	This procedure is invoked to display a button widget.  It is
 *	normally invoked as an idle handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the button in its
 *	current mode.  The REDRAW_PENDING flag is cleared.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayButton(clientData)
    ClientData clientData;	/* Information about widget. */
{
    register TkButton *butPtr = (TkButton *) clientData;
    GC gc;
    Tk_3DBorder border;
    Pixmap pixmap;
    int x = 0;			/* Initialization only needed to stop
				 * compiler warning. */
    int y, relief;
    Tk_Window tkwin = butPtr->tkwin;
    int width, height, fullWidth, fullHeight;
    int textXOffset, textYOffset;
    int haveImage = 0, haveText = 0;
    int offset;			/* 1 means this is a button widget, so we
				 * offset the text to make the button appear
				 * to move up and down as the relief changes.
				 */
    int imageWidth, imageHeight;
    int imageXOffset = 0, imageYOffset = 0; /* image information that will
					     * be used to restrict disabled
					     * pixmap as well */

    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    border = butPtr->normalBorder;
    if ((butPtr->state == STATE_DISABLED) && (butPtr->disabledFg != NULL)) {
	gc = butPtr->disabledGC;
    } else if ((butPtr->state == STATE_ACTIVE)
	    && !Tk_StrictMotif(butPtr->tkwin)) {
	gc = butPtr->activeTextGC;
	border = butPtr->activeBorder;
    } else {
	gc = butPtr->normalTextGC;
    }
    if ((butPtr->flags & SELECTED) && (butPtr->state != STATE_ACTIVE)
	    && (butPtr->selectBorder != NULL) && !butPtr->indicatorOn) {
	border = butPtr->selectBorder;
    }

    /*
     * Override the relief specified for the button if this is a
     * checkbutton or radiobutton and there's no indicator.  The new
     * relief is as follows:
     *      If the button is select  --> "sunken"
     *      If relief==overrelief    --> relief
     *      Otherwise                --> overrelief
     *
     * The effect we are trying to achieve is as follows:
     *
     *      value    mouse-over?   -->   relief
     *     -------  ------------        --------
     *       off        no               flat
     *       off        yes              raised
     *       on         no               sunken
     *       on         yes              sunken
     *
     * This is accomplished by configuring the checkbutton or radiobutton
     * like this:
     *
     *     -indicatoron 0 -overrelief raised -offrelief flat
     *
     * Bindings (see library/button.tcl) will copy the -overrelief into
     * -relief on mouseover.  Hence, we can tell if we are in mouse-over by
     * comparing relief against overRelief.  This is an aweful kludge, but
     * it gives use the desired behavior while keeping the code backwards
     * compatible.
     */

    relief = butPtr->relief;
    if ((butPtr->type >= TYPE_CHECK_BUTTON) && !butPtr->indicatorOn) {
	if (butPtr->flags & SELECTED) {
	    relief = TK_RELIEF_SUNKEN;
	} else if (butPtr->overRelief != relief) {
	    relief = butPtr->offRelief;
	}
    }

    offset = (butPtr->type == TYPE_BUTTON) && !Tk_StrictMotif(butPtr->tkwin);

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    pixmap = Tk_GetPixmap(butPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
    Tk_Fill3DRectangle(tkwin, pixmap, border, 0, 0, Tk_Width(tkwin),
	    Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /*
     * Display image or bitmap or text for button.
     */

    if (butPtr->image != NULL) {
	Tk_SizeOfImage(butPtr->image, &width, &height);
	haveImage = 1;
    } else if (butPtr->bitmap != None) {
	Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
	haveImage = 1;
    }
    imageWidth  = width;
    imageHeight = height;

    haveText = (butPtr->textWidth != 0 && butPtr->textHeight != 0);
    
    if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
	textXOffset = 0;
	textYOffset = 0;
	fullWidth = 0;
	fullHeight = 0;

	switch ((enum compound) butPtr->compound) {
	    case COMPOUND_TOP: 
	    case COMPOUND_BOTTOM: {
		/* Image is above or below text */
		if (butPtr->compound == COMPOUND_TOP) {
		    textYOffset = height + butPtr->padY;
		} else {
		    imageYOffset = butPtr->textHeight + butPtr->padY;
		}
		fullHeight = height + butPtr->textHeight + butPtr->padY;
		fullWidth = (width > butPtr->textWidth ? width :
			butPtr->textWidth);
		textXOffset = (fullWidth - butPtr->textWidth)/2;
		imageXOffset = (fullWidth - width)/2;
		break;
	    }
	    case COMPOUND_LEFT:
	    case COMPOUND_RIGHT: {
		/* Image is left or right of text */
		if (butPtr->compound == COMPOUND_LEFT) {
		    textXOffset = width + butPtr->padX;
		} else {
		    imageXOffset = butPtr->textWidth + butPtr->padX;
		}
		fullWidth = butPtr->textWidth + butPtr->padX + width;
		fullHeight = (height > butPtr->textHeight ? height :
			butPtr->textHeight);
		textYOffset = (fullHeight - butPtr->textHeight)/2;
		imageYOffset = (fullHeight - height)/2;
		break;
	    }
	    case COMPOUND_CENTER: {
		/* Image and text are superimposed */
		fullWidth = (width > butPtr->textWidth ? width :
			butPtr->textWidth);
		fullHeight = (height > butPtr->textHeight ? height :
			butPtr->textHeight);
		textXOffset = (fullWidth - butPtr->textWidth)/2;
		imageXOffset = (fullWidth - width)/2;
		textYOffset = (fullHeight - butPtr->textHeight)/2;
		imageYOffset = (fullHeight - height)/2;
		break;
	    }
	    case COMPOUND_NONE: {break;}
	}
	
	TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
		butPtr->indicatorSpace + fullWidth, fullHeight, &x, &y);

	x += butPtr->indicatorSpace;
	
	x += offset;
	y += offset;
	if (relief == TK_RELIEF_RAISED) {
	    x -= offset;
	    y -= offset;
	} else if (relief == TK_RELIEF_SUNKEN) {
	    x += offset;
	    y += offset;
	}

	imageXOffset += x;
	imageYOffset += y;

	if (butPtr->image != NULL) {
	    if ((butPtr->selectImage != NULL) && (butPtr->flags & SELECTED)) {
		Tk_RedrawImage(butPtr->selectImage, 0, 0,
			width, height, pixmap, imageXOffset, imageYOffset);
	    } else {
		Tk_RedrawImage(butPtr->image, 0, 0, width,
			height, pixmap, imageXOffset, imageYOffset);
	    }
	} else {
	    XSetClipOrigin(butPtr->display, gc, imageXOffset, imageYOffset);
	    XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc,
		    0, 0, (unsigned int) width, (unsigned int) height,
		    imageXOffset, imageYOffset, 1);
	    XSetClipOrigin(butPtr->display, gc, 0, 0);
	}

	Tk_DrawTextLayout(butPtr->display, pixmap, gc,
		butPtr->textLayout, x + textXOffset, y + textYOffset, 0, -1);
	Tk_UnderlineTextLayout(butPtr->display, pixmap, gc,
		butPtr->textLayout, x + textXOffset, y + textYOffset,
		butPtr->underline);
	y += fullHeight/2;
    } else {
	if (haveImage) {
	    TkComputeAnchor(butPtr->anchor, tkwin, 0, 0,
		    butPtr->indicatorSpace + width, height, &x, &y);
	    x += butPtr->indicatorSpace;
	    
	    x += offset;
	    y += offset;
	    if (relief == TK_RELIEF_RAISED) {
		x -= offset;
		y -= offset;
	    } else if (relief == TK_RELIEF_SUNKEN) {
		x += offset;
		y += offset;
	    }
	    imageXOffset += x;
	    imageYOffset += y;
	    if (butPtr->image != NULL) {
		if ((butPtr->selectImage != NULL) &&
			(butPtr->flags & SELECTED)) {
		    Tk_RedrawImage(butPtr->selectImage, 0, 0, width,
			    height, pixmap, imageXOffset, imageYOffset);
		} else {
		    Tk_RedrawImage(butPtr->image, 0, 0, width, height, pixmap,
			    imageXOffset, imageYOffset);
		}
	    } else {
		XSetClipOrigin(butPtr->display, gc, x, y);
		XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc, 0, 0,
			(unsigned int) width, (unsigned int) height, x, y, 1);
		XSetClipOrigin(butPtr->display, gc, 0, 0);
	    }
	    y += height/2;
	} else {
 	    TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
		    butPtr->indicatorSpace + butPtr->textWidth,
		    butPtr->textHeight, &x, &y);
	    
	    x += butPtr->indicatorSpace;
	    
	    x += offset;
	    y += offset;
	    if (relief == TK_RELIEF_RAISED) {
		x -= offset;
		y -= offset;
	    } else if (relief == TK_RELIEF_SUNKEN) {
		x += offset;
		y += offset;
	    }
	    Tk_DrawTextLayout(butPtr->display, pixmap, gc, butPtr->textLayout,
		    x, y, 0, -1);
	    Tk_UnderlineTextLayout(butPtr->display, pixmap, gc,
		    butPtr->textLayout, x, y, butPtr->underline);
	    y += butPtr->textHeight/2;
	}
    }
    
    /*
     * Draw the indicator for check buttons and radio buttons.  At this
     * point x and y refer to the top-left corner of the text or image
     * or bitmap.
     */

    if ((butPtr->type == TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
	int dim;

	dim = butPtr->indicatorDiameter;
	x -= butPtr->indicatorSpace;
	y -= dim/2;
	if (dim > 2*butPtr->borderWidth) {
	    Tk_Draw3DRectangle(tkwin, pixmap, border, x, y, dim, dim,
		    butPtr->borderWidth,
		    (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN :
		    TK_RELIEF_RAISED);
	    x += butPtr->borderWidth;
	    y += butPtr->borderWidth;
	    dim -= 2*butPtr->borderWidth;
	    if (butPtr->flags & SELECTED) {
		GC gc;
		if (butPtr->state != STATE_DISABLED) {
		    if (butPtr->selectBorder != NULL) {
			gc = Tk_3DBorderGC(tkwin, butPtr->selectBorder,
				TK_3D_FLAT_GC);
		    } else {
			gc = Tk_3DBorderGC(tkwin, butPtr->normalBorder,
				TK_3D_FLAT_GC);
		    }
		} else {
		    if (butPtr->disabledFg != NULL) {
			gc = butPtr->disabledGC;
		    } else {
			gc = butPtr->normalTextGC; 
			XSetForeground(butPtr->display, butPtr->disabledGC,
				Tk_3DBorderColor(butPtr->normalBorder)->pixel);
		    }
		}

		XFillRectangle(butPtr->display, pixmap, gc, x, y,
			(unsigned int) dim, (unsigned int) dim);
	    } else {
		Tk_Fill3DRectangle(tkwin, pixmap, butPtr->normalBorder, x, y,
			dim, dim, butPtr->borderWidth, TK_RELIEF_FLAT);
	    }
	}
    } else if ((butPtr->type == TYPE_RADIO_BUTTON) && butPtr->indicatorOn) {
	XPoint points[4];
	int radius;

	radius = butPtr->indicatorDiameter/2;
	points[0].x = x - butPtr->indicatorSpace;
	points[0].y = y;
	points[1].x = points[0].x + radius;
	points[1].y = points[0].y + radius;
	points[2].x = points[1].x + radius;
	points[2].y = points[0].y;
	points[3].x = points[1].x;
	points[3].y = points[0].y - radius;
	if (butPtr->flags & SELECTED) {
	    GC gc;

	    if (butPtr->state != STATE_DISABLED) {
		if (butPtr->selectBorder != NULL) {
		    gc = Tk_3DBorderGC(tkwin, butPtr->selectBorder,
			    TK_3D_FLAT_GC);
		} else {
		    gc = Tk_3DBorderGC(tkwin, butPtr->normalBorder,
			    TK_3D_FLAT_GC);
		}
	    } else {
		if (butPtr->disabledFg != NULL) {
		    gc = butPtr->disabledGC;
		} else {
		    gc = butPtr->normalTextGC; 
		    XSetForeground(butPtr->display, butPtr->disabledGC,
			    Tk_3DBorderColor(butPtr->normalBorder)->pixel);
		}
	    }

	    XFillPolygon(butPtr->display, pixmap, gc, points, 4, Convex,
		    CoordModeOrigin);
	} else {
	    Tk_Fill3DPolygon(tkwin, pixmap, butPtr->normalBorder, points,
		    4, butPtr->borderWidth, TK_RELIEF_FLAT);
	}
	Tk_Draw3DPolygon(tkwin, pixmap, border, points, 4, butPtr->borderWidth,
		(butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN :
		TK_RELIEF_RAISED);
    }

    /*
     * If the button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.  If the widget
     * is selected and we use a different background color when selected,
     * must temporarily modify the GC so the stippling is the right color.
     */

    if ((butPtr->state == STATE_DISABLED)
	    && ((butPtr->disabledFg == NULL) || (butPtr->image != NULL))) {
	if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
		&& (butPtr->selectBorder != NULL)) {
	    XSetForeground(butPtr->display, butPtr->stippleGC,
		    Tk_3DBorderColor(butPtr->selectBorder)->pixel);
	}
	/*
	 * Stipple the whole button if no disabledFg was specified,
	 * otherwise restrict stippling only to displayed image
	 */
	if (butPtr->disabledFg == NULL) {
	    XFillRectangle(butPtr->display, pixmap, butPtr->stippleGC, 0, 0,
		    (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin));
	} else {
	    XFillRectangle(butPtr->display, pixmap, butPtr->stippleGC,
		    imageXOffset, imageYOffset,
		    (unsigned) imageWidth, (unsigned) imageHeight);
	}
	if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
		&& (butPtr->selectBorder != NULL)) {
	    XSetForeground(butPtr->display, butPtr->stippleGC,
		    Tk_3DBorderColor(butPtr->normalBorder)->pixel);
	}
    }

    /*
     * Draw the border and traversal highlight last.  This way, if the
     * button's contents overflow they'll be covered up by the border.
     * This code is complicated by the possible combinations of focus
     * highlight and default rings.  We draw the focus and highlight rings
     * using the highlight border and highlight foreground color.
     */

    if (relief != TK_RELIEF_FLAT) {
	int inset = butPtr->highlightWidth;

	if (butPtr->defaultState == DEFAULT_ACTIVE) {
	    /*
	     * Draw the default ring with 2 pixels of space between the
	     * default ring and the button and the default ring and the
	     * focus ring.  Note that we need to explicitly draw the space
	     * in the highlightBorder color to ensure that we overwrite any
	     * overflow text and/or a different button background color.
	     */

	    Tk_Draw3DRectangle(tkwin, pixmap, butPtr->highlightBorder, inset,
		    inset, Tk_Width(tkwin) - 2*inset,
		    Tk_Height(tkwin) - 2*inset, 2, TK_RELIEF_FLAT);
	    inset += 2;
	    Tk_Draw3DRectangle(tkwin, pixmap, butPtr->highlightBorder, inset,
		    inset, Tk_Width(tkwin) - 2*inset,
		    Tk_Height(tkwin) - 2*inset, 1, TK_RELIEF_SUNKEN);
	    inset++;
	    Tk_Draw3DRectangle(tkwin, pixmap, butPtr->highlightBorder, inset,
		    inset, Tk_Width(tkwin) - 2*inset,
		    Tk_Height(tkwin) - 2*inset, 2, TK_RELIEF_FLAT);

	    inset += 2;
	} else if (butPtr->defaultState == DEFAULT_NORMAL) {
	    /*
	     * Leave room for the default ring and write over any text or
	     * background color.
	     */

	    Tk_Draw3DRectangle(tkwin, pixmap, butPtr->highlightBorder, 0,
		    0, Tk_Width(tkwin), Tk_Height(tkwin), 5, TK_RELIEF_FLAT);
	    inset += 5;
	}

	/*
	 * Draw the button border.
	 */

	Tk_Draw3DRectangle(tkwin, pixmap, border, inset, inset,
		Tk_Width(tkwin) - 2*inset, Tk_Height(tkwin) - 2*inset,
		butPtr->borderWidth, relief);
    }
    if (butPtr->highlightWidth > 0) {
	GC gc;

	if (butPtr->flags & GOT_FOCUS) {
	    gc = Tk_GCForColor(butPtr->highlightColorPtr, pixmap);
	} else {
	    gc = Tk_GCForColor(Tk_3DBorderColor(butPtr->highlightBorder),
		    pixmap);
	}

	/*
	 * Make sure the focus ring shrink-wraps the actual button, not the
	 * padding space left for a default ring.
	 */

	if (butPtr->defaultState == DEFAULT_NORMAL) {
	    TkDrawInsetFocusHighlight(tkwin, gc, butPtr->highlightWidth,
		    pixmap, 5);
	} else {
	    Tk_DrawFocusHighlight(tkwin, gc, butPtr->highlightWidth, pixmap);
	}
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */

    XCopyArea(butPtr->display, pixmap, Tk_WindowId(tkwin),
	    butPtr->copyGC, 0, 0, (unsigned) Tk_Width(tkwin),
	    (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(butPtr->display, pixmap);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeButtonGeometry --
 *
 *	After changes in a button's text or bitmap, this procedure
 *	recomputes the button's geometry and passes this information
 *	along to the geometry manager for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The button's window may change size.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeButtonGeometry(butPtr)
    register TkButton *butPtr;	/* Button whose geometry may have changed. */
{
    int width, height, avgWidth, txtWidth, txtHeight;
    int haveImage = 0, haveText = 0;
    Tk_FontMetrics fm;

    butPtr->inset = butPtr->highlightWidth + butPtr->borderWidth;

    /*
     * Leave room for the default ring if needed.
     */

    if (butPtr->defaultState != DEFAULT_DISABLED) {
	butPtr->inset += 5;
    }
    butPtr->indicatorSpace = 0;

    width = 0;
    height = 0;
    txtWidth = 0;
    txtHeight = 0;
    avgWidth = 0;
    
    if (butPtr->image != NULL) {
	Tk_SizeOfImage(butPtr->image, &width, &height);
	haveImage = 1;
    } else if (butPtr->bitmap != None) {
	Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
	haveImage = 1;
    }
    
    if (haveImage == 0 || butPtr->compound != COMPOUND_NONE) {
	Tk_FreeTextLayout(butPtr->textLayout);
	    
	butPtr->textLayout = Tk_ComputeTextLayout(butPtr->tkfont,
		Tcl_GetString(butPtr->textPtr), -1, butPtr->wrapLength,
		butPtr->justify, 0, &butPtr->textWidth, &butPtr->textHeight);
	
	txtWidth = butPtr->textWidth;
	txtHeight = butPtr->textHeight;
	avgWidth = Tk_TextWidth(butPtr->tkfont, "0", 1);
	Tk_GetFontMetrics(butPtr->tkfont, &fm);
	haveText = (txtWidth != 0 && txtHeight != 0);
    }
    
    /*
     * If the button is compound (ie, it shows both an image and text),
     * the new geometry is a combination of the image and text geometry.
     * We only honor the compound bit if the button has both text and an
     * image, because otherwise it is not really a compound button.
     */

    if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
	switch ((enum compound) butPtr->compound) {
	    case COMPOUND_TOP:
	    case COMPOUND_BOTTOM: {
		/* Image is above or below text */
		height += txtHeight + butPtr->padY;
		width = (width > txtWidth ? width : txtWidth);
		break;
	    }
	    case COMPOUND_LEFT:
	    case COMPOUND_RIGHT: {
		/* Image is left or right of text */
		width += txtWidth + butPtr->padX;
		height = (height > txtHeight ? height : txtHeight);
		break;
	    }
	    case COMPOUND_CENTER: {
		/* Image and text are superimposed */
		width = (width > txtWidth ? width : txtWidth);
		height = (height > txtHeight ? height : txtHeight);
		break;
	    }
	    case COMPOUND_NONE: {break;}
	}
	if (butPtr->width > 0) {
	    width = butPtr->width;
	}
	if (butPtr->height > 0) {
	    height = butPtr->height;
	}

	if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
	    butPtr->indicatorSpace = height;
	    if (butPtr->type == TYPE_CHECK_BUTTON) {
		butPtr->indicatorDiameter = (65*height)/100;
	    } else {
		butPtr->indicatorDiameter = (75*height)/100;
	    }
	}

	width += 2*butPtr->padX;
	height += 2*butPtr->padY;

    } else {
	if (haveImage) {
	    if (butPtr->width > 0) {
		width = butPtr->width;
	    }
	    if (butPtr->height > 0) {
		height = butPtr->height;
	    }
	    
	    if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
		butPtr->indicatorSpace = height;
		if (butPtr->type == TYPE_CHECK_BUTTON) {
		    butPtr->indicatorDiameter = (65*height)/100;
		} else {
		    butPtr->indicatorDiameter = (75*height)/100;
		}
	    }
	} else {
	    width = txtWidth;
	    height = txtHeight;
	    
	    if (butPtr->width > 0) {
		width = butPtr->width * avgWidth;
	    }
	    if (butPtr->height > 0) {
		height = butPtr->height * fm.linespace;
	    }
	    if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
		butPtr->indicatorDiameter = fm.linespace;
		if (butPtr->type == TYPE_CHECK_BUTTON) {
		    butPtr->indicatorDiameter =
			(80*butPtr->indicatorDiameter)/100;
		}
		butPtr->indicatorSpace = butPtr->indicatorDiameter + avgWidth;
	    }
	}
    }

    /*
     * When issuing the geometry request, add extra space for the indicator,
     * if any, and for the border and padding, plus two extra pixels so the
     * display can be offset by 1 pixel in either direction for the raised
     * or lowered effect.
     */

    if ((butPtr->image == NULL) && (butPtr->bitmap == None)) {
	width += 2*butPtr->padX;
	height += 2*butPtr->padY;
    }
    if ((butPtr->type == TYPE_BUTTON) && !Tk_StrictMotif(butPtr->tkwin)) {
	width += 2;
	height += 2;
    }
    Tk_GeometryRequest(butPtr->tkwin, (int) (width + butPtr->indicatorSpace
	    + 2*butPtr->inset), (int) (height + 2*butPtr->inset));
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}
