/* 
 * tkWinButton.c --
 *
 *	This file implements the Windows specific portion of the button
 *	widgets.
 *
 * Copyright (c) 1996-1998 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define OEMRESOURCE
#include "tkWinInt.h"
#include "tkButton.h"

/*
 * These macros define the base style flags for the different button types.
 */

#define LABEL_STYLE (BS_OWNERDRAW | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS)
#define PUSH_STYLE (BS_OWNERDRAW | BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS)
#define CHECK_STYLE (BS_OWNERDRAW | BS_CHECKBOX | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS)
#define RADIO_STYLE (BS_OWNERDRAW | BS_RADIOBUTTON | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS)

static DWORD buttonStyles[] = {
    LABEL_STYLE, PUSH_STYLE, CHECK_STYLE, RADIO_STYLE
};

/*
 * Declaration of Windows specific button structure.
 */

typedef struct WinButton {
    TkButton info;		/* Generic button info. */
    WNDPROC oldProc;		/* Old window procedure. */
    HWND hwnd;			/* Current window handle. */
    Pixmap pixmap;		/* Bitmap for rendering the button. */
    DWORD style;		/* Window style flags. */
} WinButton;


/*
 * The following macro reverses the order of RGB bytes to convert
 * between RGBQUAD and COLORREF values.
 */

#define FlipColor(rgb) (RGB(GetBValue(rgb),GetGValue(rgb),GetRValue(rgb)))

/*
 * The following enumeration defines the meaning of the palette entries
 * in the "buttons" image used to draw checkbox and radiobutton indicators.
 */

enum {
    PAL_CHECK = 0,
    PAL_TOP_OUTER = 1,
    PAL_BOTTOM_OUTER = 2,
    PAL_BOTTOM_INNER = 3,
    PAL_INTERIOR = 4,
    PAL_TOP_INNER = 5,
    PAL_BACKGROUND = 6
};

/*
 * Cached information about the boxes bitmap, and the default border 
 * width for a button in string form for use in Tk_OptionSpec for 
 * the various button widget classes.
 */

typedef struct ThreadSpecificData { 
    BITMAPINFOHEADER *boxesPtr;   /* Information about the bitmap. */
    DWORD *boxesPalette;	  /* Pointer to color palette. */
    LPSTR boxesBits;		  /* Pointer to bitmap data. */
    DWORD boxHeight;              /* Height of each sub-image. */
    DWORD boxWidth ;              /* Width of each sub-image. */
    char defWidth[TCL_INTEGER_SPACE];
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Declarations for functions defined in this file.
 */

static int		ButtonBindProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, XEvent *eventPtr,
			    Tk_Window tkwin, KeySym keySym));
static LRESULT CALLBACK	ButtonProc _ANSI_ARGS_((HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam));
static DWORD		ComputeStyle _ANSI_ARGS_((WinButton* butPtr));
static Window		CreateProc _ANSI_ARGS_((Tk_Window tkwin,
			    Window parent, ClientData instanceData));
static void		InitBoxes _ANSI_ARGS_((void));

/*
 * The class procedure table for the button widgets.
 */

TkClassProcs tkpButtonProcs = { 
    CreateProc,			/* createProc. */
    TkButtonWorldChanged,	/* geometryProc. */
    NULL			/* modalProc. */ 
};


/*
 *----------------------------------------------------------------------
 *
 * InitBoxes --
 *
 *	This function load the Tk 3d button bitmap.  "buttons" is a 16 
 *	color bitmap that is laid out such that the top row contains 
 *	the 4 checkbox images, and the bottom row contains the radio 
 *	button images. Note that the bitmap is stored in bottom-up 
 *	format.  Also, the first seven palette entries are used to 
 *	identify the different parts of the bitmaps so we can do the 
 *	appropriate color mappings based on the current button colors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Loads the "buttons" resource.
 *
 *----------------------------------------------------------------------
 */

static void
InitBoxes()
{
    /*
     * For DLLs like Tk, the HINSTANCE is the same as the HMODULE.
     */

    HMODULE module = (HINSTANCE) Tk_GetHINSTANCE();
    HRSRC hrsrc;
    HGLOBAL hblk;
    LPBITMAPINFOHEADER newBitmap;
    DWORD size;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    hrsrc = FindResource(module, "buttons", RT_BITMAP);
    if (hrsrc) {
	hblk = LoadResource(module, hrsrc);
	tsdPtr->boxesPtr = (LPBITMAPINFOHEADER)LockResource(hblk);
    }

    /*
     * Copy the DIBitmap into writable memory.
     */

    if (tsdPtr->boxesPtr != NULL && !(tsdPtr->boxesPtr->biWidth % 4)
	    && !(tsdPtr->boxesPtr->biHeight % 2)) {
	size = tsdPtr->boxesPtr->biSize + (1 << tsdPtr->boxesPtr->biBitCount) 
                * sizeof(RGBQUAD) + tsdPtr->boxesPtr->biSizeImage;
	newBitmap = (LPBITMAPINFOHEADER) ckalloc(size);
	memcpy(newBitmap, tsdPtr->boxesPtr, size);
	tsdPtr->boxesPtr = newBitmap;
	tsdPtr->boxWidth = tsdPtr->boxesPtr->biWidth / 4;
	tsdPtr->boxHeight = tsdPtr->boxesPtr->biHeight / 2;
	tsdPtr->boxesPalette = (DWORD*) (((LPSTR) tsdPtr->boxesPtr) 
                + tsdPtr->boxesPtr->biSize);
	tsdPtr->boxesBits = ((LPSTR) tsdPtr->boxesPalette)
	    + ((1 << tsdPtr->boxesPtr->biBitCount) * sizeof(RGBQUAD));
    } else {
	tsdPtr->boxesPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpButtonSetDefaults --
 *
 *	This procedure is invoked before option tables are created for
 *	buttons.  It modifies some of the default values to match the
 *	current values defined for this platform.
 *
 * Results:
 *	Some of the default values in *specPtr are modified.
 *
 * Side effects:
 *	Updates some of.
 *
 *----------------------------------------------------------------------
 */

void
TkpButtonSetDefaults(specPtr)
    Tk_OptionSpec *specPtr;	/* Points to an array of option specs,
				 * terminated by one with type
				 * TK_OPTION_END. */
{
    int width;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->defWidth[0] == 0) {
	width = GetSystemMetrics(SM_CXEDGE);
	if (width == 0) {
	    width = 1;
	}
	sprintf(tsdPtr->defWidth, "%d", width);
    }
    for ( ; specPtr->type != TK_OPTION_END; specPtr++) {
	if (specPtr->internalOffset == Tk_Offset(TkButton, borderWidth)) {
	    specPtr->defValue = tsdPtr->defWidth;
	}
    }
}

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
    WinButton *butPtr;

    butPtr = (WinButton *)ckalloc(sizeof(WinButton));
    butPtr->hwnd = NULL;
    return (TkButton *) butPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateProc --
 *
 *	This function creates a new Button control, subclasses
 *	the instance, and generates a new Window object.
 *
 * Results:
 *	Returns the newly allocated Window object, or None on failure.
 *
 * Side effects:
 *	Causes a new Button control to come into existence.
 *
 *----------------------------------------------------------------------
 */

static Window
CreateProc(tkwin, parentWin, instanceData)
    Tk_Window tkwin;		/* Token for window. */
    Window parentWin;		/* Parent of new window. */
    ClientData instanceData;	/* Button instance data. */
{
    Window window;
    HWND parent;
    char *class;
    WinButton *butPtr = (WinButton *)instanceData;

    parent = Tk_GetHWND(parentWin);
    if (butPtr->info.type == TYPE_LABEL) {
	class = "STATIC";
	butPtr->style = SS_OWNERDRAW | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    } else {
	class = "BUTTON";
	butPtr->style = BS_OWNERDRAW | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    }
    butPtr->hwnd = CreateWindow(class, NULL, butPtr->style,
	    Tk_X(tkwin), Tk_Y(tkwin), Tk_Width(tkwin), Tk_Height(tkwin),
	    parent, NULL, Tk_GetHINSTANCE(), NULL);
    SetWindowPos(butPtr->hwnd, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    butPtr->oldProc = (WNDPROC)SetWindowLong(butPtr->hwnd, GWL_WNDPROC,
	    (DWORD) ButtonProc);

    window = Tk_AttachHWND(tkwin, butPtr->hwnd);
    return window;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyButton --
 *
 *	Free data structures associated with the button control.
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
TkpDestroyButton(butPtr)
    TkButton *butPtr;
{
    WinButton *winButPtr = (WinButton *)butPtr;
    HWND hwnd = winButPtr->hwnd;
    if (hwnd) {
	SetWindowLong(hwnd, GWL_WNDPROC, (DWORD) winButPtr->oldProc);
    }
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
 *	Information appears on the screen.  The REDRAW_PENDING flag
 *	is cleared.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayButton(clientData)
    ClientData clientData;	/* Information about widget. */
{
    TkWinDCState state;
    HDC dc;
    register TkButton *butPtr = (TkButton *) clientData;
    GC gc;
    Tk_3DBorder border;
    Pixmap pixmap;
    int x = 0;			/* Initialization only needed to stop
				 * compiler warning. */
    int y, relief;
    register Tk_Window tkwin = butPtr->tkwin;
    int width, height;
    int defaultWidth;		/* Width of default ring. */
    int offset;			/* 0 means this is a label widget.  1 means
				 * it is a flavor of button, so we offset
				 * the text to make the button appear to
				 * move up and down as the relief changes. */
    DWORD *boxesPalette;

    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    boxesPalette= tsdPtr->boxesPalette;
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
     * checkbutton or radiobutton and there's no indicator.
     */

    relief = butPtr->relief;
    if ((butPtr->type >= TYPE_CHECK_BUTTON) && !butPtr->indicatorOn) {
	relief = (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN
		: TK_RELIEF_RAISED;
    }

    /*
     * Compute width of default ring and offset for pushed buttons.
     */

    if (butPtr->type == TYPE_BUTTON) {
	defaultWidth = ((butPtr->defaultState == DEFAULT_ACTIVE)
		? butPtr->highlightWidth : 0);
	offset = 1;
    } else {
	defaultWidth = 0;
	if ((butPtr->type >= TYPE_CHECK_BUTTON) && !butPtr->indicatorOn) {
	    offset = 1;
	} else {
	    offset = 0;
	}
    }

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

    if (butPtr->image != None) {
	Tk_SizeOfImage(butPtr->image, &width, &height);

	imageOrBitmap:
	TkComputeAnchor(butPtr->anchor, tkwin, 0, 0,
		butPtr->indicatorSpace + width, height, &x, &y);
	x += butPtr->indicatorSpace;

	if (relief == TK_RELIEF_SUNKEN) {
	    x += offset;
	    y += offset;
	}
	if (butPtr->image != NULL) {
	    if ((butPtr->selectImage != NULL) && (butPtr->flags & SELECTED)) {
		Tk_RedrawImage(butPtr->selectImage, 0, 0, width, height,
			pixmap, x, y);
	    } else {
		Tk_RedrawImage(butPtr->image, 0, 0, width, height, pixmap,
			x, y);
	    }
	} else {
	    XSetClipOrigin(butPtr->display, gc, x, y);
	    XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc, 0, 0,
		    (unsigned int) width, (unsigned int) height, x, y, 1);
	    XSetClipOrigin(butPtr->display, gc, 0, 0);
	}
	y += height/2;
    } else if (butPtr->bitmap != None) {
	Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
	goto imageOrBitmap;
    } else {
	RECT rect;
	TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
		butPtr->indicatorSpace + butPtr->textWidth, butPtr->textHeight,
		&x, &y);

	x += butPtr->indicatorSpace;

	if (relief == TK_RELIEF_SUNKEN) {
	    x += offset;
	    y += offset;
	}
	Tk_DrawTextLayout(butPtr->display, pixmap, gc, butPtr->textLayout,
		x, y, 0, -1);
	Tk_UnderlineTextLayout(butPtr->display, pixmap, gc,
		butPtr->textLayout, x, y, butPtr->underline);

	/*
	 * Draw the focus ring.  If this is a push button then we need to put
	 * it around the inner edge of the border, otherwise we put it around
	 * the text.
	 */

	if (butPtr->flags & GOT_FOCUS && butPtr->type != TYPE_LABEL) {
	    dc = TkWinGetDrawableDC(butPtr->display, pixmap, &state);
	    if (butPtr->type == TYPE_BUTTON || !butPtr->indicatorOn) {
		rect.top = butPtr->borderWidth + 1 + defaultWidth;
		rect.left = rect.top;
		rect.right = Tk_Width(tkwin) - rect.left;
		rect.bottom = Tk_Height(tkwin) - rect.top;
	    } else {
		rect.top = y-2;
		rect.left = x-2;
		rect.right = x+butPtr->textWidth + 1;
		rect.bottom = y+butPtr->textHeight + 1;
	    }
	    SetTextColor(dc, gc->foreground);
	    SetBkColor(dc, gc->background);
	    DrawFocusRect(dc, &rect);
	    TkWinReleaseDrawableDC(pixmap, dc, &state);
	}
	y += butPtr->textHeight/2;
    }

    /*
     * Draw the indicator for check buttons and radio buttons.  At this
     * point x and y refer to the top-left corner of the text or image
     * or bitmap.
     */

    if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn
	    && tsdPtr->boxesPtr) {
	int xSrc, ySrc;

	x -= butPtr->indicatorSpace;
	y -= butPtr->indicatorDiameter / 2;

	xSrc = (butPtr->flags & SELECTED) ? tsdPtr->boxWidth : 0;
	if (butPtr->state == STATE_ACTIVE) {
	    xSrc += tsdPtr->boxWidth*2;
	}
	ySrc = (butPtr->type == TYPE_RADIO_BUTTON) ? 0 : tsdPtr->boxHeight;
		
	/*
	 * Update the palette in the boxes bitmap to reflect the current
	 * button colors.  Note that this code relies on the layout of the
	 * bitmap's palette.  Also, all of the colors used to draw the
	 * bitmap must be in the palette that is selected into the DC of
	 * the offscreen pixmap.  This requires that the static colors
	 * be placed into the palette.
	 */

	boxesPalette[PAL_CHECK] = FlipColor(gc->foreground);
	boxesPalette[PAL_TOP_OUTER] = FlipColor(TkWinGetBorderPixels(tkwin,
		border, TK_3D_DARK_GC));
	boxesPalette[PAL_TOP_INNER] = FlipColor(TkWinGetBorderPixels(tkwin,
		border, TK_3D_DARK2));
	boxesPalette[PAL_BOTTOM_INNER] = FlipColor(TkWinGetBorderPixels(tkwin,
		border, TK_3D_LIGHT2));
	boxesPalette[PAL_BOTTOM_OUTER] = FlipColor(TkWinGetBorderPixels(tkwin,
		border, TK_3D_LIGHT_GC));
	if (butPtr->state == STATE_DISABLED) {
	    boxesPalette[PAL_INTERIOR] = FlipColor(TkWinGetBorderPixels(tkwin,
		border, TK_3D_LIGHT2));
	} else if (butPtr->selectBorder != NULL) {
	    boxesPalette[PAL_INTERIOR] = FlipColor(TkWinGetBorderPixels(tkwin,
		    butPtr->selectBorder, TK_3D_FLAT_GC));
	} else {
	    boxesPalette[PAL_INTERIOR] = FlipColor(GetSysColor(COLOR_WINDOW));
	}
	boxesPalette[PAL_BACKGROUND] = FlipColor(TkWinGetBorderPixels(tkwin,
		border, TK_3D_FLAT_GC));

	dc = TkWinGetDrawableDC(butPtr->display, pixmap, &state);
	StretchDIBits(dc, x, y, tsdPtr->boxWidth, tsdPtr->boxHeight, 
                xSrc, ySrc, tsdPtr->boxWidth, tsdPtr->boxHeight, 
                tsdPtr->boxesBits, (LPBITMAPINFO) tsdPtr->boxesPtr, 
                DIB_RGB_COLORS, SRCCOPY);
	TkWinReleaseDrawableDC(pixmap, dc, &state);
    }

    /*
     * If the button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.  If the widget
     * is selected and we use a different background color when selected,
     * must temporarily modify the GC.
     */

    if ((butPtr->state == STATE_DISABLED)
	    && ((butPtr->disabledFg == NULL) || (butPtr->image != NULL))) {
	if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
		&& (butPtr->selectBorder != NULL)) {
	    XSetForeground(butPtr->display, butPtr->disabledGC,
		    Tk_3DBorderColor(butPtr->selectBorder)->pixel);
	}
	XFillRectangle(butPtr->display, pixmap, butPtr->disabledGC,
		butPtr->inset, butPtr->inset,
		(unsigned) (Tk_Width(tkwin) - 2*butPtr->inset),
		(unsigned) (Tk_Height(tkwin) - 2*butPtr->inset));
	if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
		&& (butPtr->selectBorder != NULL)) {
	    XSetForeground(butPtr->display, butPtr->disabledGC,
		    Tk_3DBorderColor(butPtr->normalBorder)->pixel);
	}
    }

    /*
     * Draw the border and traversal highlight last.  This way, if the
     * button's contents overflow they'll be covered up by the border.
     */

    if (relief != TK_RELIEF_FLAT) {
	Tk_Draw3DRectangle(tkwin, pixmap, border,
		defaultWidth, defaultWidth,
		Tk_Width(tkwin) - 2*defaultWidth,
		Tk_Height(tkwin) - 2*defaultWidth,
		butPtr->borderWidth, relief);
    }
    if (defaultWidth != 0) {
	dc = TkWinGetDrawableDC(butPtr->display, pixmap, &state);
	TkWinFillRect(dc, 0, 0, Tk_Width(tkwin), defaultWidth,
		butPtr->highlightColorPtr->pixel);
	TkWinFillRect(dc, 0, 0, defaultWidth, Tk_Height(tkwin),
		butPtr->highlightColorPtr->pixel);
	TkWinFillRect(dc, 0, Tk_Height(tkwin) - defaultWidth,
		Tk_Width(tkwin), defaultWidth,
		butPtr->highlightColorPtr->pixel);
	TkWinFillRect(dc, Tk_Width(tkwin) - defaultWidth, 0,
		defaultWidth, Tk_Height(tkwin),
		butPtr->highlightColorPtr->pixel);
	TkWinReleaseDrawableDC(pixmap, dc, &state);
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
    int width, height, avgWidth;
    Tk_FontMetrics fm;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (butPtr->highlightWidth < 0) {
	butPtr->highlightWidth = 0;
    }
    butPtr->inset = butPtr->highlightWidth + butPtr->borderWidth;
    butPtr->indicatorSpace = 0;

    if (!tsdPtr->boxesPtr) {
	InitBoxes();
    }

    if (butPtr->image != NULL) {
	Tk_SizeOfImage(butPtr->image, &width, &height);
	imageOrBitmap:
	if (butPtr->width > 0) {
	    width = butPtr->width;
	}
	if (butPtr->height > 0) {
	    height = butPtr->height;
	}
	if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
	    butPtr->indicatorSpace = tsdPtr->boxWidth * 2;
	    butPtr->indicatorDiameter = tsdPtr->boxHeight;
	}
    } else if (butPtr->bitmap != None) {
	Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
	goto imageOrBitmap;
    } else {
	Tk_FreeTextLayout(butPtr->textLayout);
	butPtr->textLayout = Tk_ComputeTextLayout(butPtr->tkfont,
		Tcl_GetString(butPtr->textPtr), -1, butPtr->wrapLength,
		butPtr->justify, 0, &butPtr->textWidth, &butPtr->textHeight);

	width = butPtr->textWidth;
	height = butPtr->textHeight;
	avgWidth = Tk_TextWidth(butPtr->tkfont, "0", 1);
	Tk_GetFontMetrics(butPtr->tkfont, &fm);

	if (butPtr->width > 0) {
	    width = butPtr->width * avgWidth;
	}
	if (butPtr->height > 0) {
	    height = butPtr->height * fm.linespace;
	}

	if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
	    butPtr->indicatorDiameter = tsdPtr->boxHeight;
	    butPtr->indicatorSpace = butPtr->indicatorDiameter + avgWidth;
	}

	/*
	 * Increase the inset to allow for the focus ring.
	 */

	if (butPtr->type != TYPE_LABEL) {
	    butPtr->inset += 3;
	}
    }

    /*
     * When issuing the geometry request, add extra space for the indicator,
     * if any, and for the border and padding, plus an extra pixel so the
     * display can be offset by 1 pixel in either direction for the raised
     * or lowered effect.
     */

    if ((butPtr->image == NULL) && (butPtr->bitmap == None)) {
	width += 2*butPtr->padX;
	height += 2*butPtr->padY;
    }
    if ((butPtr->type == TYPE_BUTTON)
	    || ((butPtr->type >= TYPE_CHECK_BUTTON) && !butPtr->indicatorOn)) {
	width += 1;
	height += 1;
    }
    Tk_GeometryRequest(butPtr->tkwin, (int) (width + butPtr->indicatorSpace
	    + 2*butPtr->inset), (int) (height + 2*butPtr->inset));
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonProc --
 *
 *	This function is call by Windows whenever an event occurs on
 *	a button control created by Tk.
 *
 * Results:
 *	Standard Windows return value.
 *
 * Side effects:
 *	May generate events.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
ButtonProc(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    LRESULT result;
    WinButton *butPtr;
    Tk_Window tkwin = Tk_HWNDToWindow(hwnd);

    if (tkwin == NULL) {
	panic("ButtonProc called on an invalid HWND");
    }
    butPtr = (WinButton *)((TkWindow*)tkwin)->instanceData;

    switch(message) {
	case WM_ERASEBKGND:
	    return 0;

	case BM_GETCHECK:
	    if (((butPtr->info.type == TYPE_CHECK_BUTTON)
		    || (butPtr->info.type == TYPE_RADIO_BUTTON))
		    && butPtr->info.indicatorOn) {
		return (butPtr->info.flags & SELECTED)
		    ? BST_CHECKED : BST_UNCHECKED;
	    }
	    return 0;

	case BM_GETSTATE: {
	    DWORD state = 0;
	    if (((butPtr->info.type == TYPE_CHECK_BUTTON)
		    || (butPtr->info.type == TYPE_RADIO_BUTTON))
		    && butPtr->info.indicatorOn) {
		state = (butPtr->info.flags & SELECTED)
		    ? BST_CHECKED : BST_UNCHECKED;
	    }
	    if (butPtr->info.flags & GOT_FOCUS) {
		state |= BST_FOCUS;
	    }
	    return state;
	}
	case WM_ENABLE:
	    break;

	case WM_PAINT: {
	    PAINTSTRUCT ps;
	    BeginPaint(hwnd, &ps);
	    EndPaint(hwnd, &ps);
	    TkpDisplayButton((ClientData)butPtr);

	    /*
	     * Special note: must cancel any existing idle handler
	     * for TkpDisplayButton;  it's no longer needed, and
	     * TkpDisplayButton cleared the REDRAW_PENDING flag.
	     */
           
	    Tcl_CancelIdleCall(TkpDisplayButton, (ClientData)butPtr);
	    return 0;
	}
	case BN_CLICKED: {
	    int code;
	    Tcl_Interp *interp = butPtr->info.interp;
	    if (butPtr->info.state != STATE_DISABLED) {
		Tcl_Preserve((ClientData)interp);
		code = TkInvokeButton((TkButton*)butPtr);
		if (code != TCL_OK && code != TCL_CONTINUE
			&& code != TCL_BREAK) {
		    Tcl_AddErrorInfo(interp, "\n    (button invoke)");
		    Tcl_BackgroundError(interp);
		}
		Tcl_Release((ClientData)interp);
	    }
	    Tcl_ServiceAll();
	    return 0;
	}

	default:
	    if (Tk_TranslateWinEvent(hwnd, message, wParam, lParam, &result)) {
		return result;
	    }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
