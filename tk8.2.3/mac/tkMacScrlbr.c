/* 
 * tkMacScrollbar.c --
 *
 *	This file implements the Macintosh specific portion of the scrollbar
 *	widget.  The Macintosh scrollbar may also draw a windows grow
 *	region under certain cases.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkScrollbar.h"
#include "tkMacInt.h"
#include <Controls.h>

/*
 * The following definitions should really be in MacOS
 * header files.  They are included here as this is the only
 * file that needs the declarations.
 */
typedef	pascal void (*ThumbActionFunc)(void);

#if GENERATINGCFM
typedef UniversalProcPtr ThumbActionUPP;
#else
typedef ThumbActionFunc ThumbActionUPP;
#endif

enum {
	uppThumbActionProcInfo = kPascalStackBased
};

#if GENERATINGCFM
#define NewThumbActionProc(userRoutine)		\
		(ThumbActionUPP) NewRoutineDescriptor((ProcPtr)(userRoutine), uppThumbActionProcInfo, GetCurrentArchitecture())
#else
#define NewThumbActionProc(userRoutine)		\
		((ThumbActionUPP) (userRoutine))
#endif

/*
 * Minimum slider length, in pixels (designed to make sure that the slider
 * is always easy to grab with the mouse).
 */

#define MIN_SLIDER_LENGTH	5

/*
 * Declaration of Windows specific scrollbar structure.
 */

typedef struct MacScrollbar {
    TkScrollbar info;		/* Generic scrollbar info. */
    ControlRef sbHandle;	/* Handle to the Scrollbar control struct. */
    int macFlags;		/* Various flags; see below. */
} MacScrollbar;

/*
 * Flag bits for scrollbars on the Mac:
 * 
 * ALREADY_DEAD:		Non-zero means this scrollbar has been
 *				destroyed, but has not been cleaned up.
 * IN_MODAL_LOOP:		Non-zero means this scrollbar is in the middle
 *				of a modal loop.
 * ACTIVE:			Non-zero means this window is currently
 *				active (in the foreground).
 * FLUSH_TOP:			Flush with top of Mac window.
 * FLUSH_BOTTOM:		Flush with bottom of Mac window.
 * FLUSH_RIGHT:			Flush with right of Mac window.
 * FLUSH_LEFT:			Flush with left of Mac window.
 * SCROLLBAR_GROW:		Non-zero means this window draws the grow
 *				region for the toplevel window.
 * AUTO_ADJUST:			Non-zero means we automatically adjust
 *				the size of the widget to align correctly
 *				along a Mac window.
 * DRAW_GROW:			Non-zero means we draw the grow region.
 */

#define ALREADY_DEAD		1
#define IN_MODAL_LOOP		2
#define ACTIVE			4
#define FLUSH_TOP		8
#define FLUSH_BOTTOM		16
#define FLUSH_RIGHT		32
#define FLUSH_LEFT		64
#define SCROLLBAR_GROW		128
#define AUTO_ADJUST		256
#define DRAW_GROW		512

/*
 * Globals uses locally in this file.
 */
static ControlActionUPP scrollActionProc = NULL; /* Pointer to func. */
static ThumbActionUPP thumbActionProc = NULL;    /* Pointer to func. */
static TkScrollbar *activeScrollPtr = NULL;        /* Non-null when in thumb */
						 /* proc. */
/*
 * Forward declarations for procedures defined later in this file:
 */

static pascal void	ScrollbarActionProc _ANSI_ARGS_((ControlRef theControl,
			    ControlPartCode partCode));
static int		ScrollbarBindProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, XEvent *eventPtr,
			    Tk_Window tkwin, KeySym keySym));
static void		ScrollbarEventProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static pascal void	ThumbActionProc _ANSI_ARGS_((void));
static void		UpdateControlValues _ANSI_ARGS_((MacScrollbar *macScrollPtr));
		    
/*
 * The class procedure table for the scrollbar widget.
 */

TkClassProcs tkpScrollbarProcs = { 
    NULL,			/* createProc. */
    NULL,			/* geometryProc. */
    NULL			/* modalProc */
};

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateScrollbar --
 *
 *	Allocate a new TkScrollbar structure.
 *
 * Results:
 *	Returns a newly allocated TkScrollbar structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkScrollbar *
TkpCreateScrollbar(
    Tk_Window tkwin)	/* New Tk Window. */
{
    MacScrollbar * macScrollPtr;
    TkWindow *winPtr = (TkWindow *)tkwin;
    
    if (scrollActionProc == NULL) {
	scrollActionProc = NewControlActionProc(ScrollbarActionProc);
	thumbActionProc = NewThumbActionProc(ThumbActionProc);
    }

    macScrollPtr = (MacScrollbar *) ckalloc(sizeof(MacScrollbar));
    macScrollPtr->sbHandle = NULL;
    macScrollPtr->macFlags = 0;

    Tk_CreateEventHandler(tkwin, ActivateMask|ExposureMask|
	    StructureNotifyMask|FocusChangeMask,
	    ScrollbarEventProc, (ClientData) macScrollPtr);

    if (!Tcl_GetAssocData(winPtr->mainPtr->interp, "TkScrollbar", NULL)) {
	Tcl_SetAssocData(winPtr->mainPtr->interp, "TkScrollbar", NULL,
		(ClientData)1);
	TkCreateBindingProcedure(winPtr->mainPtr->interp,
		winPtr->mainPtr->bindingTable,
		(ClientData)Tk_GetUid("Scrollbar"), "<ButtonPress>",
		ScrollbarBindProc, NULL, NULL);
    }

    return (TkScrollbar *) macScrollPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkpDisplayScrollbar --
 *
 *	This procedure redraws the contents of a scrollbar window.
 *	It is invoked as a do-when-idle handler, so it only runs
 *	when there's nothing else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

void
TkpDisplayScrollbar(
    ClientData clientData)	/* Information about window. */
{
    register TkScrollbar *scrollPtr = (TkScrollbar *) clientData;
    register MacScrollbar *macScrollPtr = (MacScrollbar *) clientData;
    register Tk_Window tkwin = scrollPtr->tkwin;
    
    MacDrawable *macDraw;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    WindowRef windowRef;
    
    if ((scrollPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	goto done;
    }

    /*
     * Draw the focus or any 3D relief we may have.
     */
    if (scrollPtr->highlightWidth != 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(scrollPtr->highlightBgColorPtr,
		Tk_WindowId(tkwin));

	if (scrollPtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(scrollPtr->highlightColorPtr,
		    Tk_WindowId(tkwin));
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, scrollPtr->highlightWidth,
		Tk_WindowId(tkwin));
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, scrollPtr->highlightWidth,
		Tk_WindowId(tkwin));
	}
    }
    Tk_Draw3DRectangle(tkwin, Tk_WindowId(tkwin), scrollPtr->bgBorder,
	    scrollPtr->highlightWidth, scrollPtr->highlightWidth,
	    Tk_Width(tkwin) - 2*scrollPtr->highlightWidth,
	    Tk_Height(tkwin) - 2*scrollPtr->highlightWidth,
	    scrollPtr->borderWidth, scrollPtr->relief);

    /*
     * Set up port for drawing Macintosh control.
     */
    macDraw = (MacDrawable *) Tk_WindowId(tkwin);
    destPort = TkMacGetDrawablePort(Tk_WindowId(tkwin));
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    TkMacSetUpClippingRgn(Tk_WindowId(tkwin));

    if (macScrollPtr->sbHandle == NULL) {
        Rect r;
        
        r.left = r.top = 0;
        r.right = r.bottom = 1;
	macScrollPtr->sbHandle = NewControl((WindowRef) destPort, &r, "\p",
		false, (short) 500, 0, 1000,
		scrollBarProc, (SInt32) scrollPtr);

	/*
	 * If we are foremost than make us active.
	 */
	if ((WindowPtr) destPort == FrontWindow()) {
	    macScrollPtr->macFlags |= ACTIVE;
	}
    }

    /*
     * Update the control values before we draw.
     */
    windowRef  = (**macScrollPtr->sbHandle).contrlOwner;    
    UpdateControlValues(macScrollPtr);
    
    if (macScrollPtr->macFlags & ACTIVE) {
	Draw1Control(macScrollPtr->sbHandle);
	if (macScrollPtr->macFlags & DRAW_GROW) {
	    DrawGrowIcon(windowRef);
	}
    } else {
	(**macScrollPtr->sbHandle).contrlHilite = 255;
	Draw1Control(macScrollPtr->sbHandle);
	if (macScrollPtr->macFlags & DRAW_GROW) {
	    DrawGrowIcon(windowRef);
	    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), scrollPtr->bgBorder,
		Tk_Width(tkwin) - 13, Tk_Height(tkwin) - 13,
		Tk_Width(tkwin), Tk_Height(tkwin),
		0, TK_RELIEF_FLAT);
	}
    }
    
    SetGWorld(saveWorld, saveDevice);
     
    done:
    scrollPtr->flags &= ~REDRAW_PENDING;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureScrollbar --
 *
 *	This procedure is called after the generic code has finished
 *	processing configuration options, in order to configure
 *	platform specific options.
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
TkpConfigureScrollbar(scrollPtr)
    register TkScrollbar *scrollPtr;	/* Information about widget;  may or
					 * may not already have values for
					 * some fields. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeScrollbarGeometry --
 *
 *	After changes in a scrollbar's size or configuration, this
 *	procedure recomputes various geometry information used in
 *	displaying the scrollbar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The scrollbar will be displayed differently.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeScrollbarGeometry(
    register TkScrollbar *scrollPtr)	/* Scrollbar whose geometry may
					 * have changed. */
{
    MacScrollbar *macScrollPtr = (MacScrollbar *) scrollPtr;
    int width, fieldLength, adjust = 0;

    if (scrollPtr->highlightWidth < 0) {
	scrollPtr->highlightWidth = 0;
    }
    scrollPtr->inset = scrollPtr->highlightWidth + scrollPtr->borderWidth;
    width = (scrollPtr->vertical) ? Tk_Width(scrollPtr->tkwin)
	    : Tk_Height(scrollPtr->tkwin);
    scrollPtr->arrowLength = width - 2*scrollPtr->inset + 1;
    fieldLength = (scrollPtr->vertical ? Tk_Height(scrollPtr->tkwin)
	    : Tk_Width(scrollPtr->tkwin))
	    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
    if (fieldLength < 0) {
	fieldLength = 0;
    }
    scrollPtr->sliderFirst = fieldLength*scrollPtr->firstFraction;
    scrollPtr->sliderLast = fieldLength*scrollPtr->lastFraction;

    /*
     * Adjust the slider so that some piece of it is always
     * displayed in the scrollbar and so that it has at least
     * a minimal width (so it can be grabbed with the mouse).
     */

    if (scrollPtr->sliderFirst > (fieldLength - 2*scrollPtr->borderWidth)) {
	scrollPtr->sliderFirst = fieldLength - 2*scrollPtr->borderWidth;
    }
    if (scrollPtr->sliderFirst < 0) {
	scrollPtr->sliderFirst = 0;
    }
    if (scrollPtr->sliderLast < (scrollPtr->sliderFirst
	    + MIN_SLIDER_LENGTH)) {
	scrollPtr->sliderLast = scrollPtr->sliderFirst + MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderLast > fieldLength) {
	scrollPtr->sliderLast = fieldLength;
    }
    scrollPtr->sliderFirst += scrollPtr->arrowLength + scrollPtr->inset;
    scrollPtr->sliderLast += scrollPtr->arrowLength + scrollPtr->inset;

    /*
     * Register the desired geometry for the window (leave enough space
     * for the two arrows plus a minimum-size slider, plus border around
     * the whole window, if any).  Then arrange for the window to be
     * redisplayed.
     */

    if (scrollPtr->vertical) {
	if ((macScrollPtr->macFlags & AUTO_ADJUST) &&
		(macScrollPtr->macFlags & (FLUSH_RIGHT|FLUSH_LEFT))) {
	    adjust--;
	}
	Tk_GeometryRequest(scrollPtr->tkwin,
		scrollPtr->width + 2*scrollPtr->inset + adjust,
		2*(scrollPtr->arrowLength + scrollPtr->borderWidth
		+ scrollPtr->inset));
    } else {
	if ((macScrollPtr->macFlags & AUTO_ADJUST) &&
		(macScrollPtr->macFlags & (FLUSH_TOP|FLUSH_BOTTOM))) {
	    adjust--;
	}
	Tk_GeometryRequest(scrollPtr->tkwin,
		2*(scrollPtr->arrowLength + scrollPtr->borderWidth
		+ scrollPtr->inset), scrollPtr->width + 2*scrollPtr->inset + adjust);
    }
    Tk_SetInternalBorder(scrollPtr->tkwin, scrollPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScrollbar --
 *
 *	Free data structures associated with the scrollbar control.
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
TkpDestroyScrollbar(
    TkScrollbar *scrollPtr)	/* Scrollbar to destroy. */
{
    MacScrollbar *macScrollPtr = (MacScrollbar *)scrollPtr;

    if (macScrollPtr->sbHandle != NULL) {
	if (!(macScrollPtr->macFlags & IN_MODAL_LOOP)) {
	    DisposeControl(macScrollPtr->sbHandle);
	    macScrollPtr->sbHandle = NULL;
	}
    }
    macScrollPtr->macFlags |= ALREADY_DEAD;
}

/*
 *--------------------------------------------------------------
 *
 * TkpScrollbarPosition --
 *
 *	Determine the scrollbar element corresponding to a
 *	given position.
 *
 * Results:
 *	One of TOP_ARROW, TOP_GAP, etc., indicating which element
 *	of the scrollbar covers the position given by (x, y).  If
 *	(x,y) is outside the scrollbar entirely, then OUTSIDE is
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkpScrollbarPosition(
    TkScrollbar *scrollPtr,	/* Scrollbar widget record. */
    int x, int y)		/* Coordinates within scrollPtr's
				 * window. */
{
    MacScrollbar *macScrollPtr = (MacScrollbar *) scrollPtr;
    GWorldPtr destPort;
    int length, width, tmp, inactive = false;
    ControlPartCode part;
    Point where;
    Rect bounds;

    if (scrollPtr->vertical) {
	length = Tk_Height(scrollPtr->tkwin);
	width = Tk_Width(scrollPtr->tkwin);
    } else {
	tmp = x;
	x = y;
	y = tmp;
	length = Tk_Width(scrollPtr->tkwin);
	width = Tk_Height(scrollPtr->tkwin);
    }

    if ((x < scrollPtr->inset) || (x >= (width - scrollPtr->inset))
	    || (y < scrollPtr->inset) || (y >= (length - scrollPtr->inset))) {
	return OUTSIDE;
    }

    /*
     * All of the calculations in this procedure mirror those in
     * DisplayScrollbar.  Be sure to keep the two consistent.  On the 
     * Macintosh we use the OS call TestControl to do this mapping.
     * For TestControl to work, the scrollbar must be active and must
     * be in the current port.
     */

    destPort = TkMacGetDrawablePort(Tk_WindowId(scrollPtr->tkwin));
    SetGWorld(destPort, NULL);
    UpdateControlValues(macScrollPtr);
    if ((**macScrollPtr->sbHandle).contrlHilite == 255) {
	inactive = true;
	(**macScrollPtr->sbHandle).contrlHilite = 0;
    }

    TkMacWinBounds((TkWindow *) scrollPtr->tkwin, &bounds);		
    where.h = x + bounds.left;
    where.v = y + bounds.top;
    part = TestControl(((MacScrollbar *) scrollPtr)->sbHandle, where);
    if (inactive) {
	(**macScrollPtr->sbHandle).contrlHilite = 255;
    }
    switch (part) {
    	case inUpButton:
	    return TOP_ARROW;
    	case inPageUp:
	    return TOP_GAP;
    	case inThumb:
	    return SLIDER;
    	case inPageDown:
	    return BOTTOM_GAP;
    	case inDownButton:
	    return BOTTOM_ARROW;
    	default:
	    return OUTSIDE;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ThumbActionProc --
 *
 *	Callback procedure used by the Macintosh toolbox call
 *	TrackControl.  This call is used to track the thumb of
 *	the scrollbar.  Unlike the ScrollbarActionProc function
 *	this function is called once and basically takes over
 *	tracking the scrollbar from the control.  This is done
 *	to avoid conflicts with what the control plans to draw.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change the display.
 *
 *--------------------------------------------------------------
 */

static pascal void
ThumbActionProc()
{
    register TkScrollbar *scrollPtr = activeScrollPtr;
    register MacScrollbar *macScrollPtr = (MacScrollbar *) activeScrollPtr;
    Tcl_DString cmdString;
    Rect nullRect = {0,0,0,0};
    int origValue, trackBarPin;
    double thumbWidth, newFirstFraction, trackBarSize;
    char vauleString[40];
    Point currentPoint = { 0, 0 };
    Point lastPoint = { 0, 0 };
    Rect trackRect;
    Tcl_Interp *interp;
    
    if (scrollPtr == NULL) {
	return;
    }

    Tcl_DStringInit(&cmdString);
    
    /*
     * First compute values that will remain constant during the tracking
     * of the thumb.  The variable trackBarSize is the length of the scrollbar
     * minus the 2 arrows and half the width of the thumb on both sides
     * (3 * arrowLength).  The variable trackBarPin is the lower starting point
     * of the drag region.
     *
     * Note: the arrowLength is equal to the thumb width of a Mac scrollbar.
     */
    origValue = GetControlValue(macScrollPtr->sbHandle);
    trackRect = (**macScrollPtr->sbHandle).contrlRect;
    if (scrollPtr->vertical == true) {
	trackBarSize = (double) (trackRect.bottom - trackRect.top
		- (scrollPtr->arrowLength * 3));
	trackBarPin = trackRect.top + scrollPtr->arrowLength
	    + (scrollPtr->arrowLength / 2);
	InsetRect(&trackRect, -25, -113);
	
    } else {
	trackBarSize = (double) (trackRect.right - trackRect.left
		- (scrollPtr->arrowLength * 3));
	trackBarPin = trackRect.left + scrollPtr->arrowLength
	    + (scrollPtr->arrowLength / 2);
	InsetRect(&trackRect, -113, -25);
    }

    /*
     * Track the mouse while the button is held down.  If the mouse is moved,
     * we calculate the value that should be passed to the "command" part of
     * the scrollbar.
     */
    while (StillDown()) {
	GetMouse(&currentPoint);
	if (EqualPt(currentPoint, lastPoint)) {
	    continue;
	}
	lastPoint = currentPoint;

	/*
	 * Calculating this value is a little tricky.  We need to calculate a
	 * value for where the thumb would be in a Motif widget (variable
	 * thumb).  This value is what the "command" expects and is what will
	 * be resent to the scrollbar to update its value.
	 */
	thumbWidth = scrollPtr->lastFraction - scrollPtr->firstFraction;
	if (PtInRect(currentPoint, &trackRect)) {
	    if (scrollPtr->vertical == true) {
		newFirstFraction =  (1.0 - thumbWidth) *
		    ((double) (currentPoint.v - trackBarPin) / trackBarSize);
	    } else {
		newFirstFraction =  (1.0 - thumbWidth) *
		    ((double) (currentPoint.h - trackBarPin) / trackBarSize);
	    }
	} else {
	    newFirstFraction = ((double) origValue / 1000.0)
		* (1.0 - thumbWidth);
	}
	
	sprintf(vauleString, "%g", newFirstFraction);

	Tcl_DStringSetLength(&cmdString, 0);
	Tcl_DStringAppend(&cmdString, scrollPtr->command,
		scrollPtr->commandSize);
	Tcl_DStringAppendElement(&cmdString, "moveto");
	Tcl_DStringAppendElement(&cmdString, vauleString);

        interp = scrollPtr->interp;
        Tcl_Preserve((ClientData) interp);
	Tcl_GlobalEval(interp, cmdString.string);
        Tcl_Release((ClientData) interp);
	
	Tcl_DStringSetLength(&cmdString, 0);
	Tcl_DStringAppend(&cmdString, "update idletasks",
		strlen("update idletasks"));
        Tcl_Preserve((ClientData) interp);
	Tcl_GlobalEval(interp, cmdString.string);
        Tcl_Release((ClientData) interp);
    }
    
    /*
     * This next bit of code is a bit of a hack - but needed.  The problem is
     * that the control wants to draw the drag outline if the control value
     * changes during the drag (which it does).  What we do here is change the
     * clip region to hide this drawing from the user.
     */
    ClipRect(&nullRect);
    
    Tcl_DStringFree(&cmdString);
    return;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarActionProc --
 *
 *	Callback procedure used by the Macintosh toolbox call
 *	TrackControl.  This call will update the display while
 *	the scrollbar is being manipulated by the user.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change the display.
 *
 *--------------------------------------------------------------
 */

static pascal void
ScrollbarActionProc(
    ControlRef theControl, 	/* Handle to scrollbat control */
    ControlPartCode partCode)	/* Part of scrollbar that was "hit" */
{
    register TkScrollbar *scrollPtr = (TkScrollbar *) GetCRefCon(theControl);
    Tcl_DString cmdString;
    
    Tcl_DStringInit(&cmdString);
    Tcl_DStringAppend(&cmdString, scrollPtr->command,
	    scrollPtr->commandSize);

    if (partCode == inUpButton || partCode == inDownButton) {
	Tcl_DStringAppendElement(&cmdString, "scroll");
	Tcl_DStringAppendElement(&cmdString,
		(partCode == inUpButton ) ? "-1" : "1");
	Tcl_DStringAppendElement(&cmdString, "unit");
    } else if (partCode == inPageUp || partCode == inPageDown) {
	Tcl_DStringAppendElement(&cmdString, "scroll");
	Tcl_DStringAppendElement(&cmdString,
		(partCode == inPageUp ) ? "-1" : "1");
	Tcl_DStringAppendElement(&cmdString, "page");
    }
    Tcl_Preserve((ClientData) scrollPtr->interp);
    Tcl_DStringAppend(&cmdString, "; update idletasks",
	strlen("; update idletasks"));
    Tcl_GlobalEval(scrollPtr->interp, cmdString.string);
    Tcl_Release((ClientData) scrollPtr->interp);

    Tcl_DStringFree(&cmdString);
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarBindProc --
 *
 *	This procedure is invoked when the default <ButtonPress>
 *	binding on the Scrollbar bind tag fires.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The event enters a modal loop.
 *
 *--------------------------------------------------------------
 */

static int
ScrollbarBindProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interp with binding. */
    XEvent *eventPtr,		/* X event that triggered binding. */
    Tk_Window tkwin,		/* Target window for event. */
    KeySym keySym)		/* The KeySym if a key event. */
{
    TkWindow *winPtr = (TkWindow*)tkwin;
    TkScrollbar *scrollPtr = (TkScrollbar *) winPtr->instanceData;
    MacScrollbar *macScrollPtr = (MacScrollbar *) winPtr->instanceData;

    Tcl_Preserve((ClientData)scrollPtr);
    macScrollPtr->macFlags |= IN_MODAL_LOOP;
    
    if (eventPtr->type == ButtonPress) {
    	Point where;
    	Rect bounds;
    	int part, x, y, dummy;
    	unsigned int state;
	CGrafPtr saveWorld;
	GDHandle saveDevice;
	GWorldPtr destPort;
	Window window;

	/*
	 * To call Macintosh control routines we must have the port
	 * set to the window containing the control.  We will then test
	 * which part of the control was hit and act accordingly.
	 */
	destPort = TkMacGetDrawablePort(Tk_WindowId(scrollPtr->tkwin));
	GetGWorld(&saveWorld, &saveDevice);
	SetGWorld(destPort, NULL);
	TkMacSetUpClippingRgn(Tk_WindowId(scrollPtr->tkwin));

	TkMacWinBounds((TkWindow *) scrollPtr->tkwin, &bounds);		
    	where.h = eventPtr->xbutton.x + bounds.left;
    	where.v = eventPtr->xbutton.y + bounds.top;
	part = TestControl(macScrollPtr->sbHandle, where);
	if (part == inThumb && scrollPtr->jump == false) {
	    /*
	     * Case 1: In thumb, no jump scrolling.  Call track control
	     * with the thumb action proc which will do most of the work.
	     * Set the global activeScrollPtr to the current control
	     * so the callback may have access to it.
	     */
	    activeScrollPtr = scrollPtr;
	    part = TrackControl(macScrollPtr->sbHandle, where,
		    (ControlActionUPP) thumbActionProc);
	    activeScrollPtr = NULL;
	} else if (part == inThumb) {
	    /*
	     * Case 2: in thumb with jump scrolling.  Call TrackControl
	     * with a NULL action proc.  Use the new value of the control
	     * to set update the control.
	     */
	    part = TrackControl(macScrollPtr->sbHandle, where, NULL);
	    if (part == inThumb) {
	    	double newFirstFraction, thumbWidth;
		Tcl_DString cmdString;
		char vauleString[TCL_DOUBLE_SPACE];

		/*
		 * The following calculation takes the new control
		 * value and maps it to what Tk needs for its variable
		 * thumb size representation.
		 */
		thumbWidth = scrollPtr->lastFraction
		     - scrollPtr->firstFraction;
		newFirstFraction = (1.0 - thumbWidth) *
		    ((double) GetControlValue(macScrollPtr->sbHandle) / 1000.0);
		sprintf(vauleString, "%g", newFirstFraction);

		Tcl_DStringInit(&cmdString);
		Tcl_DStringAppend(&cmdString, scrollPtr->command,
			strlen(scrollPtr->command));
		Tcl_DStringAppendElement(&cmdString, "moveto");
		Tcl_DStringAppendElement(&cmdString, vauleString);
		Tcl_DStringAppend(&cmdString, "; update idletasks",
			strlen("; update idletasks"));
		
                interp = scrollPtr->interp;
                Tcl_Preserve((ClientData) interp);
		Tcl_GlobalEval(interp, cmdString.string);
                Tcl_Release((ClientData) interp);
		Tcl_DStringFree(&cmdString);		
	    }
	} else if (part != 0) {
	    /*
	     * Case 3: in any other part of the scrollbar.  We call
	     * TrackControl with the scrollActionProc which will do
	     * most all the work.
	     */
	    TrackControl(macScrollPtr->sbHandle, where, scrollActionProc);
	    HiliteControl(macScrollPtr->sbHandle, 0);
	}
	
	/*
	 * The TrackControl call will "eat" the ButtonUp event.  We now
	 * generate a ButtonUp event so Tk will unset implicit grabs etc.
	 */
	GetMouse(&where);
	XQueryPointer(NULL, None, &window, &window, &x,
	    &y, &dummy, &dummy, &state);
	window = Tk_WindowId(scrollPtr->tkwin);
	TkGenerateButtonEvent(x, y, window, state);

	SetGWorld(saveWorld, saveDevice);
    }

    if (macScrollPtr->sbHandle && (macScrollPtr->macFlags & ALREADY_DEAD)) {
	DisposeControl(macScrollPtr->sbHandle);
	macScrollPtr->sbHandle = NULL;
    }
    macScrollPtr->macFlags &= ~IN_MODAL_LOOP;
    Tcl_Release((ClientData)scrollPtr);
    
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on scrollbars.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ScrollbarEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkScrollbar *scrollPtr = (TkScrollbar *) clientData;
    MacScrollbar *macScrollPtr = (MacScrollbar *) clientData;

    if (eventPtr->type == UnmapNotify) {
	TkMacSetScrollbarGrow((TkWindow *) scrollPtr->tkwin, false);
    } else if (eventPtr->type == ActivateNotify) {
	macScrollPtr->macFlags |= ACTIVE;
	TkScrollbarEventuallyRedraw((ClientData) scrollPtr);
    } else if (eventPtr->type == DeactivateNotify) {
	macScrollPtr->macFlags &= ~ACTIVE;
	TkScrollbarEventuallyRedraw((ClientData) scrollPtr);
    } else {
	TkScrollbarEventProc(clientData, eventPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdateControlValues --
 *
 *	This procedure updates the Macintosh scrollbar control
 *	to display the values defined by the Tk scrollbar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Macintosh control is updated.
 *
 *--------------------------------------------------------------
 */

static void
UpdateControlValues(
    MacScrollbar *macScrollPtr)		/* Scrollbar data struct. */
{
    TkScrollbar *scrollPtr = (TkScrollbar *) macScrollPtr;
    Tk_Window tkwin = scrollPtr->tkwin;
    MacDrawable * macDraw = (MacDrawable *) Tk_WindowId(scrollPtr->tkwin);
    WindowRef windowRef  = (**macScrollPtr->sbHandle).contrlOwner;    
    double middle;
    int drawGrowRgn = false;
    int flushRight = false;
    int flushBottom = false;

    /*
     * We can't use the Macintosh commands SizeControl and MoveControl as these
     * calls will also cause a redraw which in our case will also cause
     * flicker.  To avoid this we adjust the control record directly.  The
     * Draw1Control command appears to just draw where ever the control says to
     * draw so this seems right.
     *
     * NOTE: changing the control record directly may not work when
     * Apple releases the Copland version of the MacOS (or when hell is cold).
     */
     
    (**macScrollPtr->sbHandle).contrlRect.left = macDraw->xOff + scrollPtr->inset;
    (**macScrollPtr->sbHandle).contrlRect.top = macDraw->yOff + scrollPtr->inset;
    (**macScrollPtr->sbHandle).contrlRect.right = macDraw->xOff + Tk_Width(tkwin)
	- scrollPtr->inset;
    (**macScrollPtr->sbHandle).contrlRect.bottom = macDraw->yOff +
	Tk_Height(tkwin) - scrollPtr->inset;
    
    /*
     * To make Tk applications look more like Macintosh applications without 
     * requiring additional work by the Tk developer we do some cute tricks.
     * The first trick plays with the size of the widget to get it to overlap
     * with the side of the window by one pixel (we don't do this if the placer
     * is the geometry manager).  The second trick shrinks the scrollbar if it
     * it covers the area of the grow region ao the scrollbar can also draw
     * the grow region if need be.
     */
    if (!strcmp(macDraw->winPtr->geomMgrPtr->name, "place")) {
	macScrollPtr->macFlags &= AUTO_ADJUST;
    } else {
	macScrollPtr->macFlags |= AUTO_ADJUST;
    }
    /* TODO: use accessor function!!! */
    if (windowRef->portRect.left == (**macScrollPtr->sbHandle).contrlRect.left) {
	if (macScrollPtr->macFlags & AUTO_ADJUST) {
	    (**macScrollPtr->sbHandle).contrlRect.left--;
	}
	if (!(macScrollPtr->macFlags & FLUSH_LEFT)) {
	    macScrollPtr->macFlags |= FLUSH_LEFT;
	    if (scrollPtr->vertical) {
		TkpComputeScrollbarGeometry(scrollPtr);
	    }
	}
    } else if (macScrollPtr->macFlags & FLUSH_LEFT) {
	macScrollPtr->macFlags &= ~FLUSH_LEFT;
	if (scrollPtr->vertical) {
	    TkpComputeScrollbarGeometry(scrollPtr);
	}
    }
    
    if (windowRef->portRect.top == (**macScrollPtr->sbHandle).contrlRect.top) {
	if (macScrollPtr->macFlags & AUTO_ADJUST) {
	    (**macScrollPtr->sbHandle).contrlRect.top--;
	}
	if (!(macScrollPtr->macFlags & FLUSH_TOP)) {
	    macScrollPtr->macFlags |= FLUSH_TOP;
	    if (! scrollPtr->vertical) {
		TkpComputeScrollbarGeometry(scrollPtr);
	    }
	}
    } else if (macScrollPtr->macFlags & FLUSH_TOP) {
	macScrollPtr->macFlags &= ~FLUSH_TOP;
	if (! scrollPtr->vertical) {
	    TkpComputeScrollbarGeometry(scrollPtr);
	}
    }
	
    if (windowRef->portRect.right == (**macScrollPtr->sbHandle).contrlRect.right) {
	flushRight = true;
	if (macScrollPtr->macFlags & AUTO_ADJUST) {
	    (**macScrollPtr->sbHandle).contrlRect.right++;
	}
	if (!(macScrollPtr->macFlags & FLUSH_RIGHT)) {
	    macScrollPtr->macFlags |= FLUSH_RIGHT;
	    if (scrollPtr->vertical) {
		TkpComputeScrollbarGeometry(scrollPtr);
	    }
	}
    } else if (macScrollPtr->macFlags & FLUSH_RIGHT) {
	macScrollPtr->macFlags &= ~FLUSH_RIGHT;
	if (scrollPtr->vertical) {
	    TkpComputeScrollbarGeometry(scrollPtr);
	}
    }
	
    if (windowRef->portRect.bottom == (**macScrollPtr->sbHandle).contrlRect.bottom) {
	flushBottom = true;
	if (macScrollPtr->macFlags & AUTO_ADJUST) {
	    (**macScrollPtr->sbHandle).contrlRect.bottom++;
	}
	if (!(macScrollPtr->macFlags & FLUSH_BOTTOM)) {
	    macScrollPtr->macFlags |= FLUSH_BOTTOM;
	    if (! scrollPtr->vertical) {
		TkpComputeScrollbarGeometry(scrollPtr);
	    }
	}
    } else if (macScrollPtr->macFlags & FLUSH_BOTTOM) {
	macScrollPtr->macFlags &= ~FLUSH_BOTTOM;
	if (! scrollPtr->vertical) {
	    TkpComputeScrollbarGeometry(scrollPtr);
	}
    }

    /*
     * If the scrollbar is flush against the bottom right hand coner then
     * it may need to draw the grow region for the window so we let the
     * wm code know about this scrollbar.  We don't actually draw the grow
     * region, however, unless we are currently resizable.
     */
    macScrollPtr->macFlags &= ~DRAW_GROW;
    if (flushBottom && flushRight) {
	TkMacSetScrollbarGrow((TkWindow *) tkwin, true);
	if (TkMacResizable(macDraw->toplevel->winPtr)) {
	    if (scrollPtr->vertical) {
		(**macScrollPtr->sbHandle).contrlRect.bottom -= 14;
	    } else {
		(**macScrollPtr->sbHandle).contrlRect.right -= 14;
	    }
	    macScrollPtr->macFlags |= DRAW_GROW;
	}
    } else {
	TkMacSetScrollbarGrow((TkWindow *) tkwin, false);
    }
    
    /*
     * Given the Tk parameters for the fractions of the start and
     * end of the thumb, the following calculation determines the
     * location for the fixed sized Macintosh thumb.
     */
    middle = scrollPtr->firstFraction / (scrollPtr->firstFraction +
	    (1.0 - scrollPtr->lastFraction));

    (**macScrollPtr->sbHandle).contrlValue = (short) (middle * 1000);
    if ((**macScrollPtr->sbHandle).contrlHilite == 0 || 
		(**macScrollPtr->sbHandle).contrlHilite == 255) {
	if (scrollPtr->firstFraction == 0.0 &&
		scrollPtr->lastFraction == 1.0) {
	    (**macScrollPtr->sbHandle).contrlHilite = 255;
	} else {
	    (**macScrollPtr->sbHandle).contrlHilite = 0;
	}
    }
    if ((**macScrollPtr->sbHandle).contrlVis != 255) {
	(**macScrollPtr->sbHandle).contrlVis = 255;
    }
}
