/* 
 * tkUtil.c --
 *
 *	This file contains miscellaneous utility procedures that
 *	are used by the rest of Tk, such as a procedure for drawing
 *	a focus highlight.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkPort.h"

/*
 * The structure below defines the implementation of the "statekey"
 * Tcl object, used for quickly finding a mapping in a TkStateMap.
 */

static Tcl_ObjType stateKeyType = {
    "statekey",				/* name */
    (Tcl_FreeInternalRepProc *) NULL,	/* freeIntRepProc */
    (Tcl_DupInternalRepProc *) NULL,	/* dupIntRepProc */
    (Tcl_UpdateStringProc *) NULL,	/* updateStringProc */
    (Tcl_SetFromAnyProc *) NULL		/* setFromAnyProc */
};


/*
 *----------------------------------------------------------------------
 *
 * TkDrawInsetFocusHighlight --
 *
 *	This procedure draws a rectangular ring around the outside of
 *	a widget to indicate that it has received the input focus.  It
 *	takes an additional padding argument that specifies how much
 *	padding is present outside th widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A rectangle "width" pixels wide is drawn in "drawable",
 *	corresponding to the outer area of "tkwin".
 *
 *----------------------------------------------------------------------
 */

void
TkDrawInsetFocusHighlight(tkwin, gc, width, drawable, padding)
    Tk_Window tkwin;		/* Window whose focus highlight ring is
				 * to be drawn. */
    GC gc;			/* Graphics context to use for drawing
				 * the highlight ring. */
    int width;			/* Width of the highlight ring, in pixels. */
    Drawable drawable;		/* Where to draw the ring (typically a
				 * pixmap for double buffering). */
    int padding;		/* Width of padding outside of widget. */
{
    XRectangle rects[4];

    rects[0].x = padding;
    rects[0].y = padding;
    rects[0].width = Tk_Width(tkwin) - (2 * padding);
    rects[0].height = width;
    rects[1].x = padding;
    rects[1].y = Tk_Height(tkwin) - width - padding;
    rects[1].width = Tk_Width(tkwin) - (2 * padding);
    rects[1].height = width;
    rects[2].x = padding;
    rects[2].y = width + padding;
    rects[2].width = width;
    rects[2].height = Tk_Height(tkwin) - 2*width - 2*padding;
    rects[3].x = Tk_Width(tkwin) - width - padding;
    rects[3].y = rects[2].y;
    rects[3].width = width;
    rects[3].height = rects[2].height;
    XFillRectangles(Tk_Display(tkwin), drawable, gc, rects, 4);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DrawFocusHighlight --
 *
 *	This procedure draws a rectangular ring around the outside of
 *	a widget to indicate that it has received the input focus.
 *
 *      This function is now deprecated.  Use TkpDrawHighlightBorder instead,
 *      since this function does not handle drawing the Focus ring properly
 *      on the Macintosh - you need to know the background GC as well 
 *      as the foreground since the Mac focus ring separated from the widget
 *      by a 1 pixel border.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A rectangle "width" pixels wide is drawn in "drawable",
 *	corresponding to the outer area of "tkwin".
 *
 *----------------------------------------------------------------------
 */

void
Tk_DrawFocusHighlight(tkwin, gc, width, drawable)
    Tk_Window tkwin;		/* Window whose focus highlight ring is
				 * to be drawn. */
    GC gc;			/* Graphics context to use for drawing
				 * the highlight ring. */
    int width;			/* Width of the highlight ring, in pixels. */
    Drawable drawable;		/* Where to draw the ring (typically a
				 * pixmap for double buffering). */
{
    TkDrawInsetFocusHighlight(tkwin, gc, width, drawable, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetScrollInfo --
 *
 *	This procedure is invoked to parse "xview" and "yview"
 *	scrolling commands for widgets using the new scrolling
 *	command syntax ("moveto" or "scroll" options).
 *
 * Results:
 *	The return value is either TK_SCROLL_MOVETO, TK_SCROLL_PAGES,
 *	TK_SCROLL_UNITS, or TK_SCROLL_ERROR.  This indicates whether
 *	the command was successfully parsed and what form the command
 *	took.  If TK_SCROLL_MOVETO, *dblPtr is filled in with the
 *	desired position;  if TK_SCROLL_PAGES or TK_SCROLL_UNITS,
 *	*intPtr is filled in with the number of lines to move (may be
 *	negative);  if TK_SCROLL_ERROR, the interp's result contains an
 *	error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetScrollInfo(interp, argc, argv, dblPtr, intPtr)
    Tcl_Interp *interp;			/* Used for error reporting. */
    int argc;				/* # arguments for command. */
    char **argv;			/* Arguments for command. */
    double *dblPtr;			/* Filled in with argument "moveto"
					 * option, if any. */
    int *intPtr;			/* Filled in with number of pages
					 * or lines to scroll, if any. */
{
    int c;
    size_t length;

    length = strlen(argv[2]);
    c = argv[2][0];
    if ((c == 'm') && (strncmp(argv[2], "moveto", length) == 0)) {
	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " ", argv[1], " moveto fraction\"",
		    (char *) NULL);
	    return TK_SCROLL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[3], dblPtr) != TCL_OK) {
	    return TK_SCROLL_ERROR;
	}
	return TK_SCROLL_MOVETO;
    } else if ((c == 's')
	    && (strncmp(argv[2], "scroll", length) == 0)) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " ", argv[1], " scroll number units|pages\"",
		    (char *) NULL);
	    return TK_SCROLL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[3], intPtr) != TCL_OK) {
	    return TK_SCROLL_ERROR;
	}
	length = strlen(argv[4]);
	c = argv[4][0];
	if ((c == 'p') && (strncmp(argv[4], "pages", length) == 0)) {
	    return TK_SCROLL_PAGES;
	} else if ((c == 'u')
		&& (strncmp(argv[4], "units", length) == 0)) {
	    return TK_SCROLL_UNITS;
	} else {
	    Tcl_AppendResult(interp, "bad argument \"", argv[4],
		    "\": must be units or pages", (char *) NULL);
	    return TK_SCROLL_ERROR;
	}
    }
    Tcl_AppendResult(interp, "unknown option \"", argv[2],
	    "\": must be moveto or scroll", (char *) NULL);
    return TK_SCROLL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetScrollInfoObj --
 *
 *	This procedure is invoked to parse "xview" and "yview"
 *	scrolling commands for widgets using the new scrolling
 *	command syntax ("moveto" or "scroll" options).
 *
 * Results:
 *	The return value is either TK_SCROLL_MOVETO, TK_SCROLL_PAGES,
 *	TK_SCROLL_UNITS, or TK_SCROLL_ERROR.  This indicates whether
 *	the command was successfully parsed and what form the command
 *	took.  If TK_SCROLL_MOVETO, *dblPtr is filled in with the
 *	desired position;  if TK_SCROLL_PAGES or TK_SCROLL_UNITS,
 *	*intPtr is filled in with the number of lines to move (may be
 *	negative);  if TK_SCROLL_ERROR, the interp's result contains an
 *	error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetScrollInfoObj(interp, objc, objv, dblPtr, intPtr)
    Tcl_Interp *interp;			/* Used for error reporting. */
    int objc;				/* # arguments for command. */
    Tcl_Obj *CONST objv[];		/* Arguments for command. */
    double *dblPtr;			/* Filled in with argument "moveto"
					 * option, if any. */
    int *intPtr;			/* Filled in with number of pages
					 * or lines to scroll, if any. */
{
    int c;
    size_t length;
    char *arg2, *arg4;

    arg2 = Tcl_GetString(objv[2]);
    length = strlen(arg2);
    c = arg2[0];
    if ((c == 'm') && (strncmp(arg2, "moveto", length) == 0)) {
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "moveto fraction");
	    return TK_SCROLL_ERROR;
	}
	if (Tcl_GetDoubleFromObj(interp, objv[3], dblPtr) != TCL_OK) {
	    return TK_SCROLL_ERROR;
	}
	return TK_SCROLL_MOVETO;
    } else if ((c == 's')
	    && (strncmp(arg2, "scroll", length) == 0)) {
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "scroll number units|pages");
	    return TK_SCROLL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[3], intPtr) != TCL_OK) {
	    return TK_SCROLL_ERROR;
	}
	arg4 = Tcl_GetString(objv[4]);
	length = (strlen(arg4));
	c = arg4[0];
	if ((c == 'p') && (strncmp(arg4, "pages", length) == 0)) {
	    return TK_SCROLL_PAGES;
	} else if ((c == 'u')
		&& (strncmp(arg4, "units", length) == 0)) {
	    return TK_SCROLL_UNITS;
	} else {
	    Tcl_AppendResult(interp, "bad argument \"", arg4,
		    "\": must be units or pages", (char *) NULL);
	    return TK_SCROLL_ERROR;
	}
    }
    Tcl_AppendResult(interp, "unknown option \"", arg2,
	    "\": must be moveto or scroll", (char *) NULL);
    return TK_SCROLL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkComputeAnchor --
 *
 *	Determine where to place a rectangle so that it will be properly
 *	anchored with respect to the given window.  Used by widgets
 *	to align a box of text inside a window.  When anchoring with
 *	respect to one of the sides, the rectangle be placed inside of
 *	the internal border of the window.
 *
 * Results:
 *	*xPtr and *yPtr set to the upper-left corner of the rectangle
 *	anchored in the window.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
TkComputeAnchor(anchor, tkwin, padX, padY, innerWidth, innerHeight, xPtr, yPtr)
    Tk_Anchor anchor;		/* Desired anchor. */
    Tk_Window tkwin;		/* Anchored with respect to this window. */
    int padX, padY;		/* Use this extra padding inside window, in
				 * addition to the internal border. */
    int innerWidth, innerHeight;/* Size of rectangle to anchor in window. */
    int *xPtr, *yPtr;		/* Returns upper-left corner of anchored
				 * rectangle. */
{
    switch (anchor) {
	case TK_ANCHOR_NW:
	case TK_ANCHOR_W:
	case TK_ANCHOR_SW:
	    *xPtr = Tk_InternalBorderWidth(tkwin) + padX;
	    break;

	case TK_ANCHOR_N:
	case TK_ANCHOR_CENTER:
	case TK_ANCHOR_S:
	    *xPtr = (Tk_Width(tkwin) - innerWidth) / 2;
	    break;

	default:
	    *xPtr = Tk_Width(tkwin) - (Tk_InternalBorderWidth(tkwin) + padX)
		    - innerWidth;
	    break;
    }

    switch (anchor) {
	case TK_ANCHOR_NW:
	case TK_ANCHOR_N:
	case TK_ANCHOR_NE:
	    *yPtr = Tk_InternalBorderWidth(tkwin) + padY;
	    break;

	case TK_ANCHOR_W:
	case TK_ANCHOR_CENTER:
	case TK_ANCHOR_E:
	    *yPtr = (Tk_Height(tkwin) - innerHeight) / 2;
	    break;

	default:
	    *yPtr = Tk_Height(tkwin) - Tk_InternalBorderWidth(tkwin) - padY
		    - innerHeight;
	    break;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFindStateString --
 *
 *	Given a lookup table, map a number to a string in the table.
 *
 * Results:
 *	If numKey was equal to the numeric key of one of the elements
 *	in the table, returns the string key of that element.
 *	Returns NULL if numKey was not equal to any of the numeric keys
 *	in the table.
 *
 * Side effects.
 *	None.
 *
 *---------------------------------------------------------------------------
 */

char *
TkFindStateString(mapPtr, numKey)
    CONST TkStateMap *mapPtr;	/* The state table. */
    int numKey;			/* The key to try to find in the table. */
{
    for ( ; mapPtr->strKey != NULL; mapPtr++) {
	if (numKey == mapPtr->numKey) {
	    return mapPtr->strKey;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFindStateNum --
 *
 *	Given a lookup table, map a string to a number in the table.
 *
 * Results:
 *	If strKey was equal to the string keys of one of the elements
 *	in the table, returns the numeric key of that element.
 *	Returns the numKey associated with the last element (the NULL
 *	string one) in the table if strKey was not equal to any of the
 *	string keys in the table.  In that case, an error message is
 *	also left in the interp's result (if interp is not NULL).
 *
 * Side effects.
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkFindStateNum(interp, option, mapPtr, strKey)
    Tcl_Interp *interp;		/* Interp for error reporting. */
    CONST char *option;		/* String to use when constructing error. */
    CONST TkStateMap *mapPtr;	/* Lookup table. */
    CONST char *strKey;		/* String to try to find in lookup table. */
{
    CONST TkStateMap *mPtr;

    for (mPtr = mapPtr; mPtr->strKey != NULL; mPtr++) {
	if (strcmp(strKey, mPtr->strKey) == 0) {
	    return mPtr->numKey;
	}
    }
    if (interp != NULL) {
	mPtr = mapPtr;
	Tcl_AppendResult(interp, "bad ", option, " value \"", strKey,
		"\": must be ", mPtr->strKey, (char *) NULL);
	for (mPtr++; mPtr->strKey != NULL; mPtr++) {
	    Tcl_AppendResult(interp, 
		    ((mPtr[1].strKey != NULL) ? ", " : ", or "), 
		    mPtr->strKey, (char *) NULL);
	}
    }
    return mPtr->numKey;
}

int
TkFindStateNumObj(interp, optionPtr, mapPtr, keyPtr)
    Tcl_Interp *interp;		/* Interp for error reporting. */
    Tcl_Obj *optionPtr;		/* String to use when constructing error. */
    CONST TkStateMap *mapPtr;	/* Lookup table. */
    Tcl_Obj *keyPtr;		/* String key to find in lookup table. */
{
    CONST TkStateMap *mPtr;
    CONST char *key;
    CONST Tcl_ObjType *typePtr;

    if ((keyPtr->typePtr == &stateKeyType)
	    && (keyPtr->internalRep.twoPtrValue.ptr1 == (VOID *) mapPtr)) {
	return (int) keyPtr->internalRep.twoPtrValue.ptr2;
    }

    key = Tcl_GetStringFromObj(keyPtr, NULL);
    for (mPtr = mapPtr; mPtr->strKey != NULL; mPtr++) {
	if (strcmp(key, mPtr->strKey) == 0) {
	    typePtr = keyPtr->typePtr;
	    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
		(*typePtr->freeIntRepProc)(keyPtr);
	    }
	    keyPtr->internalRep.twoPtrValue.ptr1 = (VOID *) mapPtr;
	    keyPtr->internalRep.twoPtrValue.ptr2 = (VOID *) mPtr->numKey;
	    keyPtr->typePtr = &stateKeyType;	    
	    return mPtr->numKey;
	}
    }
    if (interp != NULL) {
	mPtr = mapPtr;
	Tcl_AppendResult(interp, "bad ",
		Tcl_GetStringFromObj(optionPtr, NULL), " value \"", key,
		"\": must be ", mPtr->strKey, (char *) NULL);
	for (mPtr++; mPtr->strKey != NULL; mPtr++) {
	    Tcl_AppendResult(interp, 
		((mPtr[1].strKey != NULL) ? ", " : ", or "), 
		mPtr->strKey, (char *) NULL);
	}
    }
    return mPtr->numKey;
}
