/* 
 * tkMacColor.c --
 *
 *	This file maintains a database of color values for the Tk
 *	toolkit, in order to avoid round-trips to the server to
 *	map color names to pixel values.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <tkColor.h>
#include "tkMacInt.h"

#include <LowMem.h>
#include <Palettes.h>
#include <Quickdraw.h>

/*
 * Default Auxillary Control Record for all controls.  This is cached once
 * and is updated by the system.  We use this to get the default system
 * colors used by controls.
 */
static AuxCtlHandle defaultAuxCtlHandle = NULL;

/*
 * Forward declarations for procedures defined later in this file:
 */

static int	GetControlPartColor _ANSI_ARGS_((short part, RGBColor *macColor));
static int	GetMenuPartColor _ANSI_ARGS_((int part, RGBColor *macColor));
static int	GetWindowPartColor _ANSI_ARGS_((short part, RGBColor *macColor));

/*
 *----------------------------------------------------------------------
 *
 * TkSetMacColor --
 *
 *	Populates a Macintosh RGBColor structure from a X style
 *	pixel value.
 *
 * Results:
 *	Returns false if not a real pixel, true otherwise.
 *
 * Side effects:
 *	The variable macColor is updated to the pixels value.
 *
 *----------------------------------------------------------------------
 */

int
TkSetMacColor(
    unsigned long pixel,	/* Pixel value to convert. */
    RGBColor *macColor)		/* Mac color struct to modify. */
{
    switch (pixel >> 24) {
	case HIGHLIGHT_PIXEL:
	    LMGetHiliteRGB(macColor);
	    return true;
	case HIGHLIGHT_TEXT_PIXEL:
	    LMGetHiliteRGB(macColor);
	    if ((macColor->red == 0) && (macColor->green == 0)
		    && (macColor->blue == 0)) {
		macColor->red = macColor->green = macColor->blue = 0xFFFFFFFF;
	    } else {
		macColor->red = macColor->green = macColor->blue = 0;
	    }
	    return true;
	case CONTROL_TEXT_PIXEL:
	    GetControlPartColor(cTextColor, macColor);
	    return true;
	case CONTROL_BODY_PIXEL:
	    GetControlPartColor(cBodyColor, macColor);
	    return true;
	case CONTROL_FRAME_PIXEL:
	    GetControlPartColor(cFrameColor, macColor);
	    return true;
	case WINDOW_BODY_PIXEL:
	    GetWindowPartColor(wContentColor, macColor);
	    return true;
	case MENU_ACTIVE_PIXEL:
	case MENU_ACTIVE_TEXT_PIXEL:
	case MENU_BACKGROUND_PIXEL:
	case MENU_DISABLED_PIXEL:
	case MENU_TEXT_PIXEL:
	    return GetMenuPartColor((pixel >> 24), macColor);
	case APPEARANCE_PIXEL:
	    return false;
	case PIXEL_MAGIC:
	default:
	    macColor->blue = (unsigned short) ((pixel & 0xFF) << 8);
	    macColor->green = (unsigned short) (((pixel >> 8) & 0xFF) << 8);
	    macColor->red = (unsigned short) (((pixel >> 16) & 0xFF) << 8);
	    return true;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Stub functions --
 *
 *	These functions are just stubs for functions that either
 *	don't make sense on the Mac or have yet to be implemented.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	These calls do nothing - which may not be expected.
 *
 *----------------------------------------------------------------------
 */

Status
XAllocColor(
    Display *display,		/* Display. */
    Colormap map,		/* Not used. */
    XColor *colorPtr)		/* XColor struct to modify. */
{
    display->request++;
    colorPtr->pixel = TkpGetPixel(colorPtr);
    return 1;
}

Colormap
XCreateColormap(
    Display *display,		/* Display. */
    Window window,		/* X window. */
    Visual *visual,		/* Not used. */
    int alloc)			/* Not used. */
{
    static Colormap index = 1;
    
    /*
     * Just return a new value each time.
     */
    return index++;
}

void
XFreeColormap(
    Display* display,		/* Display. */
    Colormap colormap)		/* Colormap. */
{
}

void
XFreeColors(
    Display* display,		/* Display. */
    Colormap colormap,		/* Colormap. */
    unsigned long* pixels,	/* Array of pixels. */
    int npixels,		/* Number of pixels. */
    unsigned long planes)	/* Number of pixel planes. */
{
    /*
     * The Macintosh version of Tk uses TrueColor.  Nothing
     * needs to be done to release colors as there really is
     * no colormap in the Tk sense.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColor --
 *
 *	Allocate a new TkColor for the color with the given name.
 *
 * Results:
 *	Returns a newly allocated TkColor, or NULL on failure.
 *
 * Side effects:
 *	May invalidate the colormap cache associated with tkwin upon
 *	allocating a new colormap entry.  Allocates a new TkColor
 *	structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColor(
    Tk_Window tkwin,		/* Window in which color will be used. */
    Tk_Uid name)		/* Name of color to allocated (in form
				 * suitable for passing to XParseColor). */
{
    Display *display = Tk_Display(tkwin);
    Colormap colormap = Tk_Colormap(tkwin);
    TkColor *tkColPtr;
    XColor color;

    /*
     * Check to see if this is a system color.  Otherwise, XParseColor
     * will do all the work.
     */
    if (strncasecmp(name, "system", 6) == 0) {
	int foundSystemColor = false;
	RGBColor rgbValue;
	char pixelCode;
	
	if (!strcasecmp(name+6, "Highlight")) {
	    LMGetHiliteRGB(&rgbValue);
	    pixelCode = HIGHLIGHT_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "HighlightText")) {
	    LMGetHiliteRGB(&rgbValue);
	    if ((rgbValue.red == 0) && (rgbValue.green == 0)
		    && (rgbValue.blue == 0)) {
		rgbValue.red = rgbValue.green = rgbValue.blue = 0xFFFFFFFF;
	    } else {
		rgbValue.red = rgbValue.green = rgbValue.blue = 0;
	    }
	    pixelCode = HIGHLIGHT_TEXT_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "ButtonText")) {
	    GetControlPartColor(cTextColor, &rgbValue);
	    pixelCode = CONTROL_TEXT_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "ButtonFace")) {
	    GetControlPartColor(cBodyColor, &rgbValue);
	    pixelCode = CONTROL_BODY_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "ButtonFrame")) {
	    GetControlPartColor(cFrameColor, &rgbValue);
	    pixelCode = CONTROL_FRAME_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "WindowBody")) {
	    GetWindowPartColor(wContentColor, &rgbValue);
	    pixelCode = WINDOW_BODY_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "MenuActive")) {
	    GetMenuPartColor(MENU_ACTIVE_PIXEL, &rgbValue);
	    pixelCode = MENU_ACTIVE_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "MenuActiveText")) {
	    GetMenuPartColor(MENU_ACTIVE_TEXT_PIXEL, &rgbValue);
	    pixelCode = MENU_ACTIVE_TEXT_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "Menu")) {
	    GetMenuPartColor(MENU_BACKGROUND_PIXEL, &rgbValue);
	    pixelCode = MENU_BACKGROUND_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "MenuDisabled")) {
	    GetMenuPartColor(MENU_DISABLED_PIXEL, &rgbValue);
	    pixelCode = MENU_DISABLED_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "MenuText")) {
	    GetMenuPartColor(MENU_TEXT_PIXEL, &rgbValue);
	    pixelCode = MENU_TEXT_PIXEL;
	    foundSystemColor = true;
	} else if (!strcasecmp(name+6, "AppearanceColor")) {
	    color.red = 0;
	    color.green = 0;
	    color.blue = 0;
	    pixelCode = APPEARANCE_PIXEL;
	    foundSystemColor = true;
	}
	
	if (foundSystemColor) {
	    color.red = rgbValue.red;
	    color.green = rgbValue.green;
	    color.blue = rgbValue.blue;
	    color.pixel = ((((((pixelCode << 8)
		| ((color.red >> 8) & 0xff)) << 8)
		| ((color.green >> 8) & 0xff)) << 8)
		| ((color.blue >> 8) & 0xff));
	    
	    tkColPtr = (TkColor *) ckalloc(sizeof(TkColor));
	    tkColPtr->color = color;
	    return tkColPtr;
	}
    }
    
    if (XParseColor(display, colormap, name, &color) == 0) {
	return (TkColor *) NULL;
    }
    
    tkColPtr = (TkColor *) ckalloc(sizeof(TkColor));
    tkColPtr->color = color;

    return tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColorByValue --
 *
 *	Given a desired set of red-green-blue intensities for a color,
 *	locate a pixel value to use to draw that color in a given
 *	window.
 *
 * Results:
 *	The return value is a pointer to an TkColor structure that
 *	indicates the closest red, blue, and green intensities available
 *	to those specified in colorPtr, and also specifies a pixel
 *	value to use to draw in that color.
 *
 * Side effects:
 *	May invalidate the colormap cache for the specified window.
 *	Allocates a new TkColor structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColorByValue(
    Tk_Window tkwin,		/* Window in which color will be used. */
    XColor *colorPtr)		/* Red, green, and blue fields indicate
				 * desired color. */
{
    TkColor *tkColPtr = (TkColor *) ckalloc(sizeof(TkColor));

    tkColPtr->color.red = colorPtr->red;
    tkColPtr->color.green = colorPtr->green;
    tkColPtr->color.blue = colorPtr->blue;
    tkColPtr->color.pixel = TkpGetPixel(&tkColPtr->color);
    return tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetControlPartColor --
 *
 *	Given a part number this function will return the standard
 *	system default color for that part.  It does this by looking
 *	in the system's 'cctb' resource.
 *
 * Results:
 *	True if a color is found, false otherwise.
 *
 * Side effects:
 *	If a color is found then the RGB variable will be changed to
 *	the parts color.
 *
 *----------------------------------------------------------------------
 */

static int 
GetControlPartColor(
    short part, 		/* Part code. */
    RGBColor *macColor)		/* Pointer to Mac color. */
{
    short index;
    CCTabHandle ccTab;

    if (defaultAuxCtlHandle == NULL) {
	GetAuxiliaryControlRecord(NULL, &defaultAuxCtlHandle);
    }
    ccTab = (**defaultAuxCtlHandle).acCTable;
    if(ccTab && (ResError() == noErr)) {
	for(index = 0; index <= (**ccTab).ctSize; index++) {
	    if((**ccTab).ctTable[index].value == part) {
		*macColor = (**ccTab).ctTable[index].rgb;
		return true;
	    }
	}
    }
    return false;
}

/*
 *----------------------------------------------------------------------
 *
 * GetWindowPartColor --
 *
 *	Given a part number this function will return the standard
 *	system default color for that part.  It does this by looking
 *	in the system's 'wctb' resource.
 *
 * Results:
 *	True if a color is found, false otherwise.
 *
 * Side effects:
 *	If a color is found then the RGB variable will be changed to
 *	the parts color.
 *
 *----------------------------------------------------------------------
 */

static int 
GetWindowPartColor(
    short part, 		/* Part code. */
    RGBColor *macColor)		/* Pointer to Mac color. */
{
    short index;
    WCTabHandle wcTab;
	
    wcTab = (WCTabHandle) GetResource('wctb', 0);
    if(wcTab && (ResError() == noErr)) {
	for(index = 0; index <= (**wcTab).ctSize; index++) {
	    if((**wcTab).ctTable[index].value == part) {
		*macColor = (**wcTab).ctTable[index].rgb;
		return true;
	    }
	}
    }
    return false;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuPartColor --
 *
 *	Given a magic pixel value, returns the RGB color associated
 *	with it by looking the value up in the system's 'mctb' resource.
 *
 * Results:
 *	True if a color is found, false otherwise.
 *
 * Side effects:
 *	If a color is found then the RGB variable will be changed to
 *	the parts color.
 *
 *----------------------------------------------------------------------
 */

static int
GetMenuPartColor(
    int pixel,			/* The magic pixel value */
    RGBColor *macColor)		/* Pointer to Mac color */
{
    RGBColor backColor, foreColor;
    GDHandle maxDevice;
    Rect globalRect;
    MCEntryPtr mcEntryPtr;
    
    /* Under Appearance, we don't want to set any menu colors when we
       are asked for the standard menu colors.  So we return false (which
       means don't use this color... */
       
    if (TkMacHaveAppearance()) {
        macColor->red = 0xFFFF;
        macColor->green = 0;
        macColor->blue = 0;
        return false;
    } else {
        mcEntryPtr = GetMCEntry(0, 0);
    switch (pixel) {
    	case MENU_ACTIVE_PIXEL:
    	    if (mcEntryPtr == NULL) {
    		macColor->red = macColor->blue = macColor->green = 0;
    	    } else {
    	    	*macColor = mcEntryPtr->mctRGB3;
    	    }
    	    return true;
    	case MENU_ACTIVE_TEXT_PIXEL:
    	    if (mcEntryPtr == NULL) {
    		macColor->red = macColor->blue = macColor->green = 0xFFFF;
    	    } else {
    	        *macColor = mcEntryPtr->mctRGB2;
    	    }
    	    return true;
    	case MENU_BACKGROUND_PIXEL:
    	    if (mcEntryPtr == NULL) {
    		macColor->red = macColor->blue = macColor->green = 0xFFFF;
    	    } else {
    	        *macColor = mcEntryPtr->mctRGB2;
    	    }
    	    return true;
    	case MENU_DISABLED_PIXEL:
    	    if (mcEntryPtr == NULL) {
    		backColor.red = backColor.blue = backColor.green = 0xFFFF;
    		foreColor.red = foreColor.blue = foreColor.green = 0x0000;
    	    } else {
    	    	backColor = mcEntryPtr->mctRGB2;
    	    	foreColor = mcEntryPtr->mctRGB3;
    	    }
    	    SetRect(&globalRect, SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX);
    	    maxDevice = GetMaxDevice(&globalRect);
    	    if (GetGray(maxDevice, &backColor, &foreColor)) {
    	    	*macColor = foreColor;
    	    } else {
    	    
    	    	/*
    	    	 * Pointer may have been moved by GetMaxDevice or GetGray.
    	    	 */
    	    	 
    	    	mcEntryPtr = GetMCEntry(0,0);
    	    	if (mcEntryPtr == NULL) {
    	   	    macColor->red = macColor->green = macColor->blue = 0x7777;
    	   	} else {
    	    	    *macColor = mcEntryPtr->mctRGB2;
    	    	}
    	    }
    	    return true;
    	case MENU_TEXT_PIXEL:
    	    if (mcEntryPtr == NULL) {
    	    	macColor->red = macColor->green = macColor->blue = 0;
    	    } else {
    	    	*macColor = mcEntryPtr->mctRGB3;
    	    }
    	    return true;
    }
    return false;
}
}
