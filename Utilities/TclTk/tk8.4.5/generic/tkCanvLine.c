/* 
 * tkCanvLine.c --
 *
 *	This file implements line items for canvas widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <stdio.h>
#include "tkInt.h"
#include "tkPort.h"
#include "tkCanvas.h"

/*
 * The structure below defines the record for each line item.
 */

typedef enum {
    ARROWS_NONE, ARROWS_FIRST, ARROWS_LAST, ARROWS_BOTH
} Arrows;

typedef struct LineItem  {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types.  MUST BE FIRST IN STRUCTURE. */
    Tk_Outline outline;		/* Outline structure */
    Tk_Canvas canvas;		/* Canvas containing item.  Needed for
				 * parsing arrow shapes. */
    int numPoints;		/* Number of points in line (always >= 0). */
    double *coordPtr;		/* Pointer to malloc-ed array containing
				 * x- and y-coords of all points in line.
				 * X-coords are even-valued indices, y-coords
				 * are corresponding odd-valued indices. If
				 * the line has arrowheads then the first
				 * and last points have been adjusted to refer
				 * to the necks of the arrowheads rather than
				 * their tips.  The actual endpoints are
				 * stored in the *firstArrowPtr and
				 * *lastArrowPtr, if they exist. */
    int capStyle;		/* Cap style for line. */
    int joinStyle;		/* Join style for line. */
    GC arrowGC;			/* Graphics context for drawing arrowheads. */
    Arrows arrow;		/* Indicates whether or not to draw arrowheads:
				 * "none", "first", "last", or "both". */
    float arrowShapeA;		/* Distance from tip of arrowhead to center. */
    float arrowShapeB;		/* Distance from tip of arrowhead to trailing
				 * point, measured along shaft. */
    float arrowShapeC;		/* Distance of trailing points from outside
				 * edge of shaft. */
    double *firstArrowPtr;	/* Points to array of PTS_IN_ARROW points
				 * describing polygon for arrowhead at first
				 * point in line.  First point of arrowhead
				 * is tip.  Malloc'ed.  NULL means no arrowhead
				 * at first point. */
    double *lastArrowPtr;	/* Points to polygon for arrowhead at last
				 * point in line (PTS_IN_ARROW points, first
				 * of which is tip).  Malloc'ed.  NULL means
				 * no arrowhead at last point. */
    Tk_SmoothMethod *smooth;	/* Non-zero means draw line smoothed (i.e.
				 * with Bezier splines). */
    int splineSteps;		/* Number of steps in each spline segment. */
} LineItem;

/*
 * Number of points in an arrowHead:
 */

#define PTS_IN_ARROW 6

/*
 * Prototypes for procedures defined in this file:
 */

static int		ArrowheadPostscript _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, LineItem *linePtr,
			    double *arrowPtr));
static void		ComputeLineBbox _ANSI_ARGS_((Tk_Canvas canvas,
			    LineItem *linePtr));
static int		ConfigureLine _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *CONST objv[], int flags));
static int		ConfigureArrows _ANSI_ARGS_((Tk_Canvas canvas,
			    LineItem *linePtr));
static int		CreateLine _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int objc, Tcl_Obj *CONST objv[]));
static void		DeleteLine _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display));
static void		DisplayLine _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height));
static int		GetLineIndex _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr,
			    Tcl_Obj *obj, int *indexPtr));
static int		LineCoords _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr,
			    int objc, Tcl_Obj *CONST objv[]));
static void		LineDeleteCoords _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, int first, int last));
static void		LineInsert _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, int beforeThis, Tcl_Obj *obj));
static int		LineToArea _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *rectPtr));
static double		LineToPoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *coordPtr));
static int		LineToPostscript _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass));
static int		ArrowParseProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    CONST char *value, char *recordPtr, int offset));
static char *		ArrowPrintProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int offset,
			    Tcl_FreeProc **freeProcPtr));
static int		ParseArrowShape _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    CONST char *value, char *recordPtr, int offset));
static char *		PrintArrowShape _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int offset,
			    Tcl_FreeProc **freeProcPtr));
static void		ScaleLine _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY));
static void		TranslateLine _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY));

/*
 * Information used for parsing configuration specs.  If you change any
 * of the default strings, be sure to change the corresponding default
 * values in CreateLine.
 */

static Tk_CustomOption arrowShapeOption = {
    (Tk_OptionParseProc *) ParseArrowShape,
    PrintArrowShape, (ClientData) NULL
};
static Tk_CustomOption arrowOption = {
    (Tk_OptionParseProc *) ArrowParseProc,
    ArrowPrintProc, (ClientData) NULL
};
static Tk_CustomOption smoothOption = {
    (Tk_OptionParseProc *) TkSmoothParseProc,
    TkSmoothPrintProc, (ClientData) NULL
};
static Tk_CustomOption stateOption = {
    (Tk_OptionParseProc *) TkStateParseProc,
    TkStatePrintProc, (ClientData) 2
};
static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
};
static Tk_CustomOption dashOption = {
    (Tk_OptionParseProc *) TkCanvasDashParseProc,
    TkCanvasDashPrintProc, (ClientData) NULL
};
static Tk_CustomOption offsetOption = {
    (Tk_OptionParseProc *) TkOffsetParseProc,
    TkOffsetPrintProc,
    (ClientData) (TK_OFFSET_RELATIVE|TK_OFFSET_INDEX)
};
static Tk_CustomOption pixelOption = {
    (Tk_OptionParseProc *) TkPixelParseProc,
    TkPixelPrintProc, (ClientData) NULL
};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_CUSTOM, "-activedash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.activeDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-activefill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.activeColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-activestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.activeStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-activewidth", (char *) NULL, (char *) NULL,
	"0.0", Tk_Offset(LineItem, outline.activeWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CUSTOM, "-arrow", (char *) NULL, (char *) NULL,
	"none", Tk_Offset(LineItem, arrow), TK_CONFIG_DONT_SET_DEFAULT, &arrowOption},
    {TK_CONFIG_CUSTOM, "-arrowshape", (char *) NULL, (char *) NULL,
	"8 10 3", Tk_Offset(LineItem, arrowShapeA),
	TK_CONFIG_DONT_SET_DEFAULT, &arrowShapeOption},
    {TK_CONFIG_CAP_STYLE, "-capstyle", (char *) NULL, (char *) NULL,
	"butt", Tk_Offset(LineItem, capStyle), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_COLOR, "-fill", (char *) NULL, (char *) NULL,
	"black", Tk_Offset(LineItem, outline.color), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-dash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.dash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_PIXELS, "-dashoffset", (char *) NULL, (char *) NULL,
	"0", Tk_Offset(LineItem, outline.offset),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, "-disableddash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.disabledDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-disabledfill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.disabledColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-disabledstipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.disabledStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-disabledwidth", (char *) NULL, (char *) NULL,
	"0.0", Tk_Offset(LineItem, outline.disabledWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_JOIN_STYLE, "-joinstyle", (char *) NULL, (char *) NULL,
	"round", Tk_Offset(LineItem, joinStyle), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, "-offset", (char *) NULL, (char *) NULL,
	"0,0", Tk_Offset(LineItem, outline.tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_CUSTOM, "-smooth", (char *) NULL, (char *) NULL,
	"0", Tk_Offset(LineItem, smooth),
	TK_CONFIG_DONT_SET_DEFAULT, &smoothOption},
    {TK_CONFIG_INT, "-splinesteps", (char *) NULL, (char *) NULL,
	"12", Tk_Offset(LineItem, splineSteps), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, "-state", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(Tk_Item, state), TK_CONFIG_NULL_OK,
	&stateOption},
    {TK_CONFIG_BITMAP, "-stipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(LineItem, outline.stipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-tags", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_CUSTOM, "-width", (char *) NULL, (char *) NULL,
	"1.0", Tk_Offset(LineItem, outline.width),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * The structures below defines the line item type by means
 * of procedures that can be invoked by generic item code.
 */

Tk_ItemType tkLineType = {
    "line",				/* name */
    sizeof(LineItem),			/* itemSize */
    CreateLine,				/* createProc */
    configSpecs,			/* configSpecs */
    ConfigureLine,			/* configureProc */
    LineCoords,				/* coordProc */
    DeleteLine,				/* deleteProc */
    DisplayLine,			/* displayProc */
    TK_CONFIG_OBJS,			/* flags */
    LineToPoint,			/* pointProc */
    LineToArea,				/* areaProc */
    LineToPostscript,			/* postscriptProc */
    ScaleLine,				/* scaleProc */
    TranslateLine,			/* translateProc */
    (Tk_ItemIndexProc *) GetLineIndex,	/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* icursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) LineInsert,	/* insertProc */
    LineDeleteCoords,			/* dTextProc */
    (Tk_ItemType *) NULL,		/* nextPtr */
};

/*
 * The definition below determines how large are static arrays
 * used to hold spline points (splines larger than this have to
 * have their arrays malloc-ed).
 */

#define MAX_STATIC_POINTS 200

/*
 *--------------------------------------------------------------
 *
 * CreateLine --
 *
 *	This procedure is invoked to create a new line item in
 *	a canvas.
 *
 * Results:
 *	A standard Tcl return value.  If an error occurred in
 *	creating the item, then an error message is left in
 *	the interp's result;  in this case itemPtr is left uninitialized,
 *	so it can be safely freed by the caller.
 *
 * Side effects:
 *	A new line item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateLine(interp, canvas, itemPtr, objc, objv)
    Tcl_Interp *interp;			/* Interpreter for error reporting. */
    Tk_Canvas canvas;			/* Canvas to hold new item. */
    Tk_Item *itemPtr;			/* Record to hold new item;  header
					 * has been initialized by caller. */
    int objc;				/* Number of arguments in objv. */
    Tcl_Obj *CONST objv[];		/* Arguments describing line. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    int i;

    if (objc == 0) {
	panic("canvas did not pass any coords\n");
    }

    /*
     * Carry out initialization that is needed to set defaults and to
     * allow proper cleanup after errors during the the remainder of
     * this procedure.
     */

    Tk_CreateOutline(&(linePtr->outline));
    linePtr->canvas = canvas;
    linePtr->numPoints = 0;
    linePtr->coordPtr = NULL;
    linePtr->capStyle = CapButt;
    linePtr->joinStyle = JoinRound;
    linePtr->arrowGC = None;
    linePtr->arrow = ARROWS_NONE;
    linePtr->arrowShapeA = (float)8.0;
    linePtr->arrowShapeB = (float)10.0;
    linePtr->arrowShapeC = (float)3.0;
    linePtr->firstArrowPtr = NULL;
    linePtr->lastArrowPtr = NULL;
    linePtr->smooth = (Tk_SmoothMethod *) NULL;
    linePtr->splineSteps = 12;

    /*
     * Count the number of points and then parse them into a point
     * array.  Leading arguments are assumed to be points if they
     * start with a digit or a minus sign followed by a digit.
     */

    for (i = 1; i < objc; i++) {
	char *arg = Tcl_GetString(objv[i]);
	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    break;
	}
    }
    if (LineCoords(interp, canvas, itemPtr, i, objv) != TCL_OK) {
	goto error;
    }
    if (ConfigureLine(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
	return TCL_OK;
    }

    error:
    DeleteLine(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * LineCoords --
 *
 *	This procedure is invoked to process the "coords" widget
 *	command on lines.  See the user documentation for details
 *	on what it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets the interp's result.
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *--------------------------------------------------------------
 */

static int
LineCoords(interp, canvas, itemPtr, objc, objv)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item whose coordinates are to be
					 * read or modified. */
    int objc;				/* Number of coordinates supplied in
					 * objv. */
    Tcl_Obj *CONST objv[];		/* Array of coordinates: x1, y1,
					 * x2, y2, ... */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    int i, numPoints;
    double *coordPtr;

    if (objc == 0) {
	int numCoords;
	Tcl_Obj *subobj, *obj = Tcl_NewObj();

	numCoords = 2*linePtr->numPoints;
	if (linePtr->firstArrowPtr != NULL) {
	    coordPtr = linePtr->firstArrowPtr;
	} else {
	    coordPtr = linePtr->coordPtr;
	}
	for (i = 0; i < numCoords; i++, coordPtr++) {
	    if (i == 2) {
		coordPtr = linePtr->coordPtr+2;
	    }
	    if ((linePtr->lastArrowPtr != NULL) && (i == (numCoords-2))) {
		coordPtr = linePtr->lastArrowPtr;
	    }
	    subobj = Tcl_NewDoubleObj(*coordPtr);
	    Tcl_ListObjAppendElement(interp, obj, subobj);
	}
	Tcl_SetObjResult(interp, obj);
	return TCL_OK;
    }
    if (objc == 1) {
	if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		(Tcl_Obj ***) &objv) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (objc & 1) {
	char buf[64 + TCL_INTEGER_SPACE];
	sprintf(buf, "wrong # coordinates: expected an even number, got %d",
		objc);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_ERROR;
    } else if (objc < 4) {
	char buf[64 + TCL_INTEGER_SPACE];
	sprintf(buf, "wrong # coordinates: expected at least 4, got %d", objc);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_ERROR;
    } else {
	numPoints = objc/2;
	if (linePtr->numPoints != numPoints) {
	    coordPtr = (double *) ckalloc((unsigned)
		    (sizeof(double) * objc));
	    if (linePtr->coordPtr != NULL) {
		ckfree((char *) linePtr->coordPtr);
	    }
	    linePtr->coordPtr = coordPtr;
	    linePtr->numPoints = numPoints;
	}
	coordPtr = linePtr->coordPtr;
	for (i = 0; i <objc; i++) {
	    if (Tk_CanvasGetCoordFromObj(interp, canvas, objv[i],
		    coordPtr++) != TCL_OK) {
  		return TCL_ERROR;
  	    }
  	}

	/*
	 * Update arrowheads by throwing away any existing arrow-head
	 * information and calling ConfigureArrows to recompute it.
	 */

	if (linePtr->firstArrowPtr != NULL) {
	    ckfree((char *) linePtr->firstArrowPtr);
	    linePtr->firstArrowPtr = NULL;
	}
	if (linePtr->lastArrowPtr != NULL) {
	    ckfree((char *) linePtr->lastArrowPtr);
	    linePtr->lastArrowPtr = NULL;
	}
	if (linePtr->arrow != ARROWS_NONE) {
	    ConfigureArrows(canvas, linePtr);
	}
	ComputeLineBbox(canvas, linePtr);
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureLine --
 *
 *	This procedure is invoked to configure various aspects
 *	of a line item such as its background color.
 *
 * Results:
 *	A standard Tcl result code.  If an error occurs, then
 *	an error message is left in the interp's result.
 *
 * Side effects:
 *	Configuration information, such as colors and stipple
 *	patterns, may be set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureLine(interp, canvas, itemPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Line item to reconfigure. */
    int objc;			/* Number of elements in objv.  */
    Tcl_Obj *CONST objv[];	/* Arguments describing things to configure. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    XGCValues gcValues;
    GC newGC, arrowGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);
    if (TCL_OK != Tk_ConfigureWidget(interp, tkwin, configSpecs, objc,
	    (CONST char **) objv, (char *) linePtr, flags|TK_CONFIG_OBJS)) {
	return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */

    state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    if (linePtr->outline.activeWidth > linePtr->outline.width ||
	    linePtr->outline.activeDash.number != 0 ||
	    linePtr->outline.activeColor != NULL ||
	    linePtr->outline.activeStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }
    mask = Tk_ConfigOutlineGC(&gcValues, canvas, itemPtr,
	    &(linePtr->outline));
    if (mask) {
	if (linePtr->arrow == ARROWS_NONE) {
	    gcValues.cap_style = linePtr->capStyle;
	    mask |= GCCapStyle;
	}
	gcValues.join_style = linePtr->joinStyle;
	mask |= GCJoinStyle;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
	gcValues.line_width = 0;
	arrowGC = Tk_GetGC(tkwin, mask, &gcValues);
    } else {
	newGC = arrowGC = None;
    }
    if (linePtr->outline.gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), linePtr->outline.gc);
    }
    if (linePtr->arrowGC != None) {
	Tk_FreeGC(Tk_Display(tkwin), linePtr->arrowGC);
    }
    linePtr->outline.gc = newGC;
    linePtr->arrowGC = arrowGC;

    /*
     * Keep spline parameters within reasonable limits.
     */

    if (linePtr->splineSteps < 1) {
	linePtr->splineSteps = 1;
    } else if (linePtr->splineSteps > 100) {
	linePtr->splineSteps = 100;
    }

    if ((!linePtr->numPoints) || (state==TK_STATE_HIDDEN)) {
	ComputeLineBbox(canvas, linePtr);
	return TCL_OK;
    }

    /*
     * Setup arrowheads, if needed.  If arrowheads are turned off,
     * restore the line's endpoints (they were shortened when the
     * arrowheads were added).
     */

    if ((linePtr->firstArrowPtr != NULL) && (linePtr->arrow != ARROWS_FIRST)
	    && (linePtr->arrow != ARROWS_BOTH)) {
	linePtr->coordPtr[0] = linePtr->firstArrowPtr[0];
	linePtr->coordPtr[1] = linePtr->firstArrowPtr[1];
	ckfree((char *) linePtr->firstArrowPtr);
	linePtr->firstArrowPtr = NULL;
    }
    if ((linePtr->lastArrowPtr != NULL) && (linePtr->arrow != ARROWS_LAST)
	    && (linePtr->arrow != ARROWS_BOTH)) {
	int i;

	i = 2*(linePtr->numPoints-1);
	linePtr->coordPtr[i] = linePtr->lastArrowPtr[0];
	linePtr->coordPtr[i+1] = linePtr->lastArrowPtr[1];
	ckfree((char *) linePtr->lastArrowPtr);
	linePtr->lastArrowPtr = NULL;
    }
    if (linePtr->arrow != ARROWS_NONE) {
	ConfigureArrows(canvas, linePtr);
    }

    /*
     * Recompute bounding box for line.
     */

    ComputeLineBbox(canvas, linePtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteLine --
 *
 *	This procedure is called to clean up the data structure
 *	associated with a line item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *--------------------------------------------------------------
 */

static void
DeleteLine(canvas, itemPtr, display)
    Tk_Canvas canvas;			/* Info about overall canvas widget. */
    Tk_Item *itemPtr;			/* Item that is being deleted. */
    Display *display;			/* Display containing window for
					 * canvas. */
{
    LineItem *linePtr = (LineItem *) itemPtr;

    Tk_DeleteOutline(display, &(linePtr->outline));
    if (linePtr->coordPtr != NULL) {
	ckfree((char *) linePtr->coordPtr);
    }
    if (linePtr->arrowGC != None) {
	Tk_FreeGC(display, linePtr->arrowGC);
    }
    if (linePtr->firstArrowPtr != NULL) {
	ckfree((char *) linePtr->firstArrowPtr);
    }
    if (linePtr->lastArrowPtr != NULL) {
	ckfree((char *) linePtr->lastArrowPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeLineBbox --
 *
 *	This procedure is invoked to compute the bounding box of
 *	all the pixels that may be drawn as part of a line.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header
 *	for itemPtr.
 *
 *--------------------------------------------------------------
 */

static void
ComputeLineBbox(canvas, linePtr)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    LineItem *linePtr;			/* Item whose bbos is to be
					 * recomputed. */
{
    double *coordPtr;
    int i, intWidth;
    double width;
    Tk_State state = linePtr->header.state;
    Tk_TSOffset *tsoffset;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    if (!(linePtr->numPoints) || (state==TK_STATE_HIDDEN)) {
	linePtr->header.x1 = -1;
	linePtr->header.x2 = -1;
	linePtr->header.y1 = -1;
	linePtr->header.y2 = -1;
	return;
    }

    width = linePtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)linePtr) {
	if (linePtr->outline.activeWidth>width) {
	    width = linePtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.disabledWidth>0) {
	    width = linePtr->outline.disabledWidth;
	}
    }

    coordPtr = linePtr->coordPtr;
    linePtr->header.x1 = linePtr->header.x2 = (int) *coordPtr;
    linePtr->header.y1 = linePtr->header.y2 = (int) coordPtr[1];

    /*
     * Compute the bounding box of all the points in the line,
     * then expand in all directions by the line's width to take
     * care of butting or rounded corners and projecting or
     * rounded caps.  This expansion is an overestimate (worst-case
     * is square root of two over two) but it's simple.  Don't do
     * anything special for curves.  This causes an additional
     * overestimate in the bounding box, but is faster.
     */

    for (i = 1, coordPtr = linePtr->coordPtr+2; i < linePtr->numPoints;
	    i++, coordPtr += 2) {
	TkIncludePoint((Tk_Item *) linePtr, coordPtr);
    }
    width = linePtr->outline.width;
    if (width < 1.0) {
	width = 1.0;
    }
    if (linePtr->arrow != ARROWS_NONE) {
	if (linePtr->arrow != ARROWS_LAST) {
	    TkIncludePoint((Tk_Item *) linePtr, linePtr->firstArrowPtr);
	}
	if (linePtr->arrow != ARROWS_FIRST) {
	    TkIncludePoint((Tk_Item *) linePtr, linePtr->lastArrowPtr);
	}
    }

    tsoffset = &linePtr->outline.tsoffset;
    if (tsoffset->flags & TK_OFFSET_INDEX) {
	double *coordPtr = linePtr->coordPtr + (tsoffset->flags & ~TK_OFFSET_INDEX);
	if (tsoffset->flags <= 0) {
	    coordPtr = linePtr->coordPtr;
	    if ((linePtr->arrow == ARROWS_FIRST) || (linePtr->arrow == ARROWS_BOTH)) {
		coordPtr = linePtr->firstArrowPtr;
	    }
	}
	if (tsoffset->flags > (linePtr->numPoints * 2)) {
	    coordPtr = linePtr->coordPtr + (linePtr->numPoints * 2);
	    if ((linePtr->arrow == ARROWS_LAST) || (linePtr->arrow == ARROWS_BOTH)) {
		coordPtr = linePtr->lastArrowPtr;
	    }
	}
	tsoffset->xoffset = (int) (coordPtr[0] + 0.5);
	tsoffset->yoffset = (int) (coordPtr[1] + 0.5);
    } else {
	if (tsoffset->flags & TK_OFFSET_LEFT) {
	    tsoffset->xoffset = linePtr->header.x1;
	} else if (tsoffset->flags & TK_OFFSET_CENTER) {
	    tsoffset->xoffset = (linePtr->header.x1 + linePtr->header.x2)/2;
	} else if (tsoffset->flags & TK_OFFSET_RIGHT) {
	    tsoffset->xoffset = linePtr->header.x2;
	}
	if (tsoffset->flags & TK_OFFSET_TOP) {
	    tsoffset->yoffset = linePtr->header.y1;
	} else if (tsoffset->flags & TK_OFFSET_MIDDLE) {
	    tsoffset->yoffset = (linePtr->header.y1 + linePtr->header.y2)/2;
	} else if (tsoffset->flags & TK_OFFSET_BOTTOM) {
	    tsoffset->yoffset = linePtr->header.y2;
	}
    }

    intWidth = (int) (width + 0.5);
    linePtr->header.x1 -= intWidth;
    linePtr->header.x2 += intWidth;
    linePtr->header.y1 -= intWidth;
    linePtr->header.y2 += intWidth;

    if (linePtr->numPoints==1) {
	linePtr->header.x1 -= 1;
	linePtr->header.x2 += 1;
	linePtr->header.y1 -= 1;
	linePtr->header.y2 += 1;
	return;
    }

    /*
     * For mitered lines, make a second pass through all the points.
     * Compute the locations of the two miter vertex points and add
     * those into the bounding box.
     */

    if (linePtr->joinStyle == JoinMiter) {
	for (i = linePtr->numPoints, coordPtr = linePtr->coordPtr; i >= 3;
		i--, coordPtr += 2) {
	    double miter[4];
	    int j;
    
	    if (TkGetMiterPoints(coordPtr, coordPtr+2, coordPtr+4,
		    width, miter, miter+2)) {
		for (j = 0; j < 4; j += 2) {
		    TkIncludePoint((Tk_Item *) linePtr, miter+j);
		}
	    }
	}
    }

    /*
     * Add in the sizes of arrowheads, if any.
     */

    if (linePtr->arrow != ARROWS_NONE) {
	if (linePtr->arrow != ARROWS_LAST) {
	    for (i = 0, coordPtr = linePtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint((Tk_Item *) linePtr, coordPtr);
	    }
	}
	if (linePtr->arrow != ARROWS_FIRST) {
	    for (i = 0, coordPtr = linePtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint((Tk_Item *) linePtr, coordPtr);
	    }
	}
    }

    /*
     * Add one more pixel of fudge factor just to be safe (e.g.
     * X may round differently than we do).
     */

    linePtr->header.x1 -= 1;
    linePtr->header.x2 += 1;
    linePtr->header.y1 -= 1;
    linePtr->header.y2 += 1;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayLine --
 *
 *	This procedure is invoked to draw a line item in a given
 *	drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation
 *	information in canvas.
 *
 *--------------------------------------------------------------
 */

static void
DisplayLine(canvas, itemPtr, display, drawable, x, y, width, height)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    Tk_Item *itemPtr;			/* Item to be displayed. */
    Display *display;			/* Display on which to draw item. */
    Drawable drawable;			/* Pixmap or window in which to draw
					 * item. */
    int x, y, width, height;		/* Describes region of canvas that
					 * must be redisplayed (not used). */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    XPoint staticPoints[MAX_STATIC_POINTS*3];
    XPoint *pointPtr;
    double linewidth;
    int numPoints;
    Tk_State state = itemPtr->state;
    Pixmap stipple = linePtr->outline.stipple;

    if ((!linePtr->numPoints)||(linePtr->outline.gc==None)) {
	return;
    }

    if (state == TK_STATE_NULL) {
	    state = ((TkCanvas *)canvas)->canvas_state;
    }
    linewidth = linePtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (linePtr->outline.activeStipple != None) {
	    stipple = linePtr->outline.activeStipple;
	}
	if (linePtr->outline.activeWidth != linewidth) {
	    linewidth = linePtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.disabledStipple != None) {
	    stipple = linePtr->outline.disabledStipple;
	}
	if (linePtr->outline.disabledWidth != linewidth) {
	    linewidth = linePtr->outline.disabledWidth;
	}
    }
    /*
     * Build up an array of points in screen coordinates.  Use a
     * static array unless the line has an enormous number of points;
     * in this case, dynamically allocate an array.  For smoothed lines,
     * generate the curve points on each redisplay.
     */

    if ((linePtr->smooth) && (linePtr->numPoints > 2)) {
	numPoints = linePtr->smooth->coordProc(canvas, (double *) NULL,
		linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		(double *) NULL);
    } else {
	numPoints = linePtr->numPoints;
    }

    if (numPoints <= MAX_STATIC_POINTS) {
	pointPtr = staticPoints;
    } else {
	pointPtr = (XPoint *)ckalloc((unsigned)(numPoints * 3*sizeof(XPoint)));
    }

    if ((linePtr->smooth) && (linePtr->numPoints > 2)) {
	numPoints = linePtr->smooth->coordProc(canvas, linePtr->coordPtr,
		linePtr->numPoints, linePtr->splineSteps, pointPtr,
		(double *) NULL);
    } else {
	numPoints = TkCanvTranslatePath((TkCanvas*)canvas, numPoints,
		linePtr->coordPtr, 0, pointPtr);
    }

    /*
     * Display line, the free up line storage if it was dynamically
     * allocated.  If we're stippling, then modify the stipple offset
     * in the GC.  Be sure to reset the offset when done, since the
     * GC is supposed to be read-only.
     */

    if (Tk_ChangeOutlineGC(canvas, itemPtr, &(linePtr->outline))) {
	Tk_CanvasSetOffset(canvas, linePtr->arrowGC, &linePtr->outline.tsoffset);
    }
    if (numPoints>1) {
	XDrawLines(display, drawable, linePtr->outline.gc, pointPtr, numPoints,
	    CoordModeOrigin);
    } else {
	int intwidth = (int) (linewidth + 0.5);
        if (intwidth<1) {
	    intwidth=1;
	}
	XFillArc(display, drawable, linePtr->outline.gc,
		pointPtr->x - intwidth/2, pointPtr->y - intwidth/2,
		(unsigned int)intwidth+1, (unsigned int)intwidth+1, 0, 64*360);
    }
    if (pointPtr != staticPoints) {
	ckfree((char *) pointPtr);
    }

    /*
     * Display arrowheads, if they are wanted.
     */

    if (linePtr->firstArrowPtr != NULL) {
	TkFillPolygon(canvas, linePtr->firstArrowPtr, PTS_IN_ARROW,
		display, drawable, linePtr->arrowGC, NULL);
    }
    if (linePtr->lastArrowPtr != NULL) {
	TkFillPolygon(canvas, linePtr->lastArrowPtr, PTS_IN_ARROW,
		display, drawable, linePtr->arrowGC, NULL);
    }
    if (Tk_ResetOutlineGC(canvas, itemPtr, &(linePtr->outline))) {
	XSetTSOrigin(display, linePtr->arrowGC, 0, 0);
    }
}

/*
 *--------------------------------------------------------------
 *
 * LineInsert --
 *
 *	Insert coords into a line item at a given index.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The coords in the given item is modified.
 *
 *--------------------------------------------------------------
 */

static void
LineInsert(canvas, itemPtr, beforeThis, obj)
    Tk_Canvas canvas;		/* Canvas containing text item. */
    Tk_Item *itemPtr;		/* Line item to be modified. */
    int beforeThis;		/* Index before which new coordinates
				 * are to be inserted. */
    Tcl_Obj *obj;		/* New coordinates to be inserted. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    int length, objc, i;
    double *new, *coordPtr;
    Tk_State state = itemPtr->state;
    Tcl_Obj **objv;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    if (!obj || (Tcl_ListObjGetElements((Tcl_Interp *) NULL, obj, &objc, &objv) != TCL_OK)
	    || !objc || objc&1) {
	return;
    }
    length = 2*linePtr->numPoints;
    if (beforeThis < 0) {
	beforeThis = 0;
    }
    if (beforeThis > length) {
	beforeThis = length;
    }
    if (linePtr->firstArrowPtr != NULL) {
	linePtr->coordPtr[0] = linePtr->firstArrowPtr[0];
	linePtr->coordPtr[1] = linePtr->firstArrowPtr[1];
    }
    if (linePtr->lastArrowPtr != NULL) {
	linePtr->coordPtr[length-2] = linePtr->lastArrowPtr[0];
	linePtr->coordPtr[length-1] = linePtr->lastArrowPtr[1];
    }
    new = (double *) ckalloc((unsigned)(sizeof(double) * (length + objc)));
    for(i=0; i<beforeThis; i++) {
	new[i] = linePtr->coordPtr[i];
    }
    for(i=0; i<objc; i++) {
	if (Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,objv[i],
		new+(i+beforeThis))!=TCL_OK) {
	    Tcl_ResetResult(((TkCanvas *)canvas)->interp);
	    ckfree((char *) new);
	    return;
	}
    }

    for(i=beforeThis; i<length; i++) {
	new[i+objc] = linePtr->coordPtr[i];
    }
    if(linePtr->coordPtr) ckfree((char *)linePtr->coordPtr);
    linePtr->coordPtr = new;
    linePtr->numPoints = (length + objc)/2;

    if ((length>3) && (state != TK_STATE_HIDDEN)) {
	/*
	 * This is some optimizing code that will result that only the part
	 * of the polygon that changed (and the objects that are overlapping
	 * with that part) need to be redrawn. A special flag is set that
	 * instructs the general canvas code not to redraw the whole
	 * object. If this flag is not set, the canvas will do the redrawing,
	 * otherwise I have to do it here.
	 */
	itemPtr->redraw_flags |= TK_ITEM_DONT_REDRAW;

	if (beforeThis>0) {beforeThis -= 2; objc+=2; }
	if ((beforeThis+objc)<length) objc+=2;
	if (linePtr->smooth) {
	    if(beforeThis>0) {
		beforeThis-=2; objc+=2;
	    }
	    if((beforeThis+objc+2)<length) {
		objc+=2;
	    }
	}
	itemPtr->x1 = itemPtr->x2 = (int) linePtr->coordPtr[beforeThis];
	itemPtr->y1 = itemPtr->y2 = (int) linePtr->coordPtr[beforeThis+1];
	if ((linePtr->firstArrowPtr != NULL) && (beforeThis<1)) {
	    /* include old first arrow */
	    for (i = 0, coordPtr = linePtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((linePtr->lastArrowPtr != NULL) && ((beforeThis+objc)>=length)) {
		/* include old last arrow */
	    for (i = 0, coordPtr = linePtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	coordPtr = linePtr->coordPtr+beforeThis+2;
	for(i=2; i<objc; i+=2) {
	    TkIncludePoint(itemPtr, coordPtr);
		coordPtr+=2;
	}
    }
    if (linePtr->firstArrowPtr != NULL) {
	ckfree((char *) linePtr->firstArrowPtr);
	linePtr->firstArrowPtr = NULL;
    }
    if (linePtr->lastArrowPtr != NULL) {
	ckfree((char *) linePtr->lastArrowPtr);
	linePtr->lastArrowPtr = NULL;
    }
    if (linePtr->arrow != ARROWS_NONE) {
	    ConfigureArrows(canvas, linePtr);
    }

    if(itemPtr->redraw_flags & TK_ITEM_DONT_REDRAW) {
	double width;
	int intWidth;
	if ((linePtr->firstArrowPtr != NULL) && (beforeThis>2)) {
	    /* include new first arrow */
	    for (i = 0, coordPtr = linePtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((linePtr->lastArrowPtr != NULL) && ((beforeThis+objc)<(length-2))) {
	    /* include new right arrow */
	    for (i = 0, coordPtr = linePtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	width = linePtr->outline.width;
	if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
		if (linePtr->outline.activeWidth>width) {
		    width = linePtr->outline.activeWidth;
		}
	} else if (state==TK_STATE_DISABLED) {
		if (linePtr->outline.disabledWidth>0) {
		    width = linePtr->outline.disabledWidth;
		}
	}
	intWidth = (int) (width + 0.5);
	if (intWidth < 1) {
	    intWidth = 1;
	}
	itemPtr->x1 -= intWidth; itemPtr->y1 -= intWidth;
	itemPtr->x2 += intWidth; itemPtr->y2 += intWidth;
	Tk_CanvasEventuallyRedraw(canvas, itemPtr->x1, itemPtr->y1,
		itemPtr->x2, itemPtr->y2);
    }

    ComputeLineBbox(canvas, linePtr);
}

/*
 *--------------------------------------------------------------
 *
 * LineDeleteCoords --
 *
 *	Delete one or more coordinates from a line item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters between "first" and "last", inclusive, get
 *	deleted from itemPtr.
 *
 *--------------------------------------------------------------
 */

static void
LineDeleteCoords(canvas, itemPtr, first, last)
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Item in which to delete characters. */
    int first;			/* Index of first character to delete. */
    int last;			/* Index of last character to delete. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    int count, i, first1, last1;
    int length = 2*linePtr->numPoints;
    double *coordPtr;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    first &= -2;
    last &= -2;

    if (first < 0) {
	first = 0;
    }
    if (last >= length) {
	last = length-2;
    }
    if (first > last) {
	return;
    }
    if (linePtr->firstArrowPtr != NULL) {
	linePtr->coordPtr[0] = linePtr->firstArrowPtr[0];
	linePtr->coordPtr[1] = linePtr->firstArrowPtr[1];
    }
    if (linePtr->lastArrowPtr != NULL) {
	linePtr->coordPtr[length-2] = linePtr->lastArrowPtr[0];
	linePtr->coordPtr[length-1] = linePtr->lastArrowPtr[1];
    }
    first1 = first; last1 = last;
    if(first1>0) first1 -= 2;
    if(last1<length-2) last1 += 2;
    if (linePtr->smooth) {
	if(first1>0) first1 -= 2;
	if(last1<length-2) last1 += 2;
    }

    if((first1<2) && (last1 >= length-2)) {
	/*
	 * This is some optimizing code that will result that only the part
	 * of the line that changed (and the objects that are overlapping
	 * with that part) need to be redrawn. A special flag is set that
	 * instructs the general canvas code not to redraw the whole
	 * object. If this flag is set, the redrawing has to be done here,
	 * otherwise the general Canvas code will take care of it.
	 */

	itemPtr->redraw_flags |= TK_ITEM_DONT_REDRAW;
	itemPtr->x1 = itemPtr->x2 = (int) linePtr->coordPtr[first1];
	itemPtr->y1 = itemPtr->y2 = (int) linePtr->coordPtr[first1+1];
	if ((linePtr->firstArrowPtr != NULL) && (first1<2)) {
	    /* include old first arrow */
	    for (i = 0, coordPtr = linePtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((linePtr->lastArrowPtr != NULL) && (last1>=length-2)) {
		/* include old last arrow */
	    for (i = 0, coordPtr = linePtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	coordPtr = linePtr->coordPtr+first1+2;
	for (i=first1+2; i<=last1; i+=2) {
	    TkIncludePoint(itemPtr, coordPtr);
	    coordPtr+=2;
	}
    }

    count = last + 2 - first;
    for (i=last+2; i<length; i++) {
	linePtr->coordPtr[i-count] = linePtr->coordPtr[i];
    }
    linePtr->numPoints -= count/2;
    if (linePtr->firstArrowPtr != NULL) {
	ckfree((char *) linePtr->firstArrowPtr);
	linePtr->firstArrowPtr = NULL;
    }
    if (linePtr->lastArrowPtr != NULL) {
	ckfree((char *) linePtr->lastArrowPtr);
	linePtr->lastArrowPtr = NULL;
    }
    if (linePtr->arrow != ARROWS_NONE) {
	    ConfigureArrows(canvas, linePtr);
    }
    if(itemPtr->redraw_flags & TK_ITEM_DONT_REDRAW) {
	double width;
	int intWidth;
	if ((linePtr->firstArrowPtr != NULL) && (first1<4)) {
	    /* include new first arrow */
	    for (i = 0, coordPtr = linePtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((linePtr->lastArrowPtr != NULL) && (last1>(length-4))) {
	    /* include new right arrow */
	    for (i = 0, coordPtr = linePtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	width = linePtr->outline.width;
	if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
		if (linePtr->outline.activeWidth>width) {
		    width = linePtr->outline.activeWidth;
		}
	} else if (state==TK_STATE_DISABLED) {
		if (linePtr->outline.disabledWidth>0) {
		    width = linePtr->outline.disabledWidth;
		}
	}
	intWidth = (int) (width + 0.5);
	if (intWidth < 1) {
	    intWidth = 1;
	}
	itemPtr->x1 -= intWidth; itemPtr->y1 -= intWidth;
	itemPtr->x2 += intWidth; itemPtr->y2 += intWidth;
	Tk_CanvasEventuallyRedraw(canvas, itemPtr->x1, itemPtr->y1,
		itemPtr->x2, itemPtr->y2);
    }
    ComputeLineBbox(canvas, linePtr);
}

/*
 *--------------------------------------------------------------
 *
 * LineToPoint --
 *
 *	Computes the distance from a given point to a given
 *	line, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are pointPtr[0] and pointPtr[1] is inside the line.  If the
 *	point isn't inside the line then the return value is the
 *	distance from the point to the line.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
LineToPoint(canvas, itemPtr, pointPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    Tk_State state = itemPtr->state;
    LineItem *linePtr = (LineItem *) itemPtr;
    double *coordPtr, *linePoints;
    double staticSpace[2*MAX_STATIC_POINTS];
    double poly[10];
    double bestDist, dist, width;
    int numPoints, count;
    int changedMiterToBevel;	/* Non-zero means that a mitered corner
				 * had to be treated as beveled after all
				 * because the angle was < 11 degrees. */

    bestDist = 1.0e36;

    /*
     * Handle smoothed lines by generating an expanded set of points
     * against which to do the check.
     */

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = linePtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (linePtr->outline.activeWidth>width) {
	    width = linePtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.disabledWidth>0) {
	    width = linePtr->outline.disabledWidth;
	}
    }

    if ((linePtr->smooth) && (linePtr->numPoints > 2)) {
	numPoints = linePtr->smooth->coordProc(canvas, (double *) NULL,
		linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		(double *) NULL);
	if (numPoints <= MAX_STATIC_POINTS) {
	    linePoints = staticSpace;
	} else {
	    linePoints = (double *) ckalloc((unsigned)
		    (2*numPoints*sizeof(double)));
	}
	numPoints = linePtr->smooth->coordProc(canvas, linePtr->coordPtr,
		linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		linePoints);
    } else {
	numPoints = linePtr->numPoints;
	linePoints = linePtr->coordPtr;
    }

    if (width < 1.0) {
	width = 1.0;
    }

    if (!numPoints || itemPtr->state==TK_STATE_HIDDEN) {
	return bestDist;
    } else if (numPoints == 1) {
	bestDist = hypot(linePoints[0] - pointPtr[0], linePoints[1] - pointPtr[1])
		    - width/2.0;
	if (bestDist < 0) bestDist = 0;
	return bestDist;
    }

    /*
     * The overall idea is to iterate through all of the edges of
     * the line, computing a polygon for each edge and testing the
     * point against that polygon.  In addition, there are additional
     * tests to deal with rounded joints and caps.
     */

    changedMiterToBevel = 0;
    for (count = numPoints, coordPtr = linePoints; count >= 2;
	    count--, coordPtr += 2) {

	/*
	 * If rounding is done around the first point then compute
	 * the distance between the point and the point.
	 */

	if (((linePtr->capStyle == CapRound) && (count == numPoints))
		|| ((linePtr->joinStyle == JoinRound)
			&& (count != numPoints))) {
	    dist = hypot(coordPtr[0] - pointPtr[0], coordPtr[1] - pointPtr[1])
		    - width/2.0;
	    if (dist <= 0.0) {
		bestDist = 0.0;
		goto done;
	    } else if (dist < bestDist) {
		bestDist = dist;
	    }
	}

	/*
	 * Compute the polygonal shape corresponding to this edge,
	 * consisting of two points for the first point of the edge
	 * and two points for the last point of the edge.
	 */

	if (count == numPoints) {
	    TkGetButtPoints(coordPtr+2, coordPtr, width,
		    linePtr->capStyle == CapProjecting, poly, poly+2);
	} else if ((linePtr->joinStyle == JoinMiter) && !changedMiterToBevel) {
	    poly[0] = poly[6];
	    poly[1] = poly[7];
	    poly[2] = poly[4];
	    poly[3] = poly[5];
	} else {
	    TkGetButtPoints(coordPtr+2, coordPtr, width, 0,
		    poly, poly+2);

	    /*
	     * If this line uses beveled joints, then check the distance
	     * to a polygon comprising the last two points of the previous
	     * polygon and the first two from this polygon;  this checks
	     * the wedges that fill the mitered joint.
	     */

	    if ((linePtr->joinStyle == JoinBevel) || changedMiterToBevel) {
		poly[8] = poly[0];
		poly[9] = poly[1];
		dist = TkPolygonToPoint(poly, 5, pointPtr);
		if (dist <= 0.0) {
		    bestDist = 0.0;
		    goto done;
		} else if (dist < bestDist) {
		    bestDist = dist;
		}
		changedMiterToBevel = 0;
	    }
	}
	if (count == 2) {
	    TkGetButtPoints(coordPtr, coordPtr+2, width,
		    linePtr->capStyle == CapProjecting, poly+4, poly+6);
	} else if (linePtr->joinStyle == JoinMiter) {
	    if (TkGetMiterPoints(coordPtr, coordPtr+2, coordPtr+4,
		    width, poly+4, poly+6) == 0) {
		changedMiterToBevel = 1;
		TkGetButtPoints(coordPtr, coordPtr+2, width,
			0, poly+4, poly+6);
	    }
	} else {
	    TkGetButtPoints(coordPtr, coordPtr+2, width, 0,
		    poly+4, poly+6);
	}
	poly[8] = poly[0];
	poly[9] = poly[1];
	dist = TkPolygonToPoint(poly, 5, pointPtr);
	if (dist <= 0.0) {
	    bestDist = 0.0;
	    goto done;
	} else if (dist < bestDist) {
	    bestDist = dist;
	}
    }

    /*
     * If caps are rounded, check the distance to the cap around the
     * final end point of the line.
     */

    if (linePtr->capStyle == CapRound) {
	dist = hypot(coordPtr[0] - pointPtr[0], coordPtr[1] - pointPtr[1])
		- width/2.0;
	if (dist <= 0.0) {
	    bestDist = 0.0;
	    goto done;
	} else if (dist < bestDist) {
	    bestDist = dist;
	}
    }

    /*
     * If there are arrowheads, check the distance to the arrowheads.
     */

    if (linePtr->arrow != ARROWS_NONE) {
	if (linePtr->arrow != ARROWS_LAST) {
	    dist = TkPolygonToPoint(linePtr->firstArrowPtr, PTS_IN_ARROW,
		    pointPtr);
	    if (dist <= 0.0) {
		bestDist = 0.0;
		goto done;
	    } else if (dist < bestDist) {
		bestDist = dist;
	    }
	}
	if (linePtr->arrow != ARROWS_FIRST) {
	    dist = TkPolygonToPoint(linePtr->lastArrowPtr, PTS_IN_ARROW,
		    pointPtr);
	    if (dist <= 0.0) {
		bestDist = 0.0;
		goto done;
	    } else if (dist < bestDist) {
		bestDist = dist;
	    }
	}
    }

    done:
    if ((linePoints != staticSpace) && (linePoints != linePtr->coordPtr)) {
	ckfree((char *) linePoints);
    }
    return bestDist;
}

/*
 *--------------------------------------------------------------
 *
 * LineToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangular area.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the
 *	area, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
LineToArea(canvas, itemPtr, rectPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against line. */
    double *rectPtr;
{
    LineItem *linePtr = (LineItem *) itemPtr;
    double staticSpace[2*MAX_STATIC_POINTS];
    double *linePoints;
    int numPoints, result;
    double radius, width;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    width = linePtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (linePtr->outline.activeWidth>width) {
	    width = linePtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.disabledWidth>0) {
	    width = linePtr->outline.disabledWidth;
	}
    }

    radius = (width+1.0)/2.0;

    if ((state==TK_STATE_HIDDEN) || !linePtr->numPoints) {
	return -1;
    } else if (linePtr->numPoints == 1) {
	double oval[4];
	oval[0] = linePtr->coordPtr[0]-radius;
	oval[1] = linePtr->coordPtr[1]-radius;
	oval[2] = linePtr->coordPtr[0]+radius;
	oval[3] = linePtr->coordPtr[1]+radius;
	return TkOvalToArea(oval, rectPtr);
    }

    /*
     * Handle smoothed lines by generating an expanded set of points
     * against which to do the check.
     */

    if ((linePtr->smooth) && (linePtr->numPoints > 2)) {
	numPoints = linePtr->smooth->coordProc(canvas, (double *) NULL,
		linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		(double *) NULL);
	if (numPoints <= MAX_STATIC_POINTS) {
	    linePoints = staticSpace;
	} else {
	    linePoints = (double *) ckalloc((unsigned)
		    (2*numPoints*sizeof(double)));
	}
	numPoints = linePtr->smooth->coordProc(canvas, linePtr->coordPtr,
		linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		linePoints);
    } else {
	numPoints = linePtr->numPoints;
	linePoints = linePtr->coordPtr;
    }

    /*
     * Check the segments of the line.
     */

     if (width < 1.0) {
	width = 1.0;
    }

    result = TkThickPolyLineToArea(linePoints, numPoints, 
	    width, linePtr->capStyle, linePtr->joinStyle,
	    rectPtr);
    if (result == 0) {
	goto done;
    }

    /*
     * Check arrowheads, if any.
     */

    if (linePtr->arrow != ARROWS_NONE) {
	if (linePtr->arrow != ARROWS_LAST) {
	    if (TkPolygonToArea(linePtr->firstArrowPtr, PTS_IN_ARROW,
		    rectPtr) != result) {
		result = 0;
		goto done;
	    }
	}
	if (linePtr->arrow != ARROWS_FIRST) {
	    if (TkPolygonToArea(linePtr->lastArrowPtr, PTS_IN_ARROW,
		    rectPtr) != result) {
		result = 0;
		goto done;
	    }
	}
    }

    done:
    if ((linePoints != staticSpace) && (linePoints != linePtr->coordPtr)) {
	ckfree((char *) linePoints);
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleLine --
 *
 *	This procedure is invoked to rescale a line item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The line referred to by itemPtr is rescaled so that the
 *	following transformation is applied to all point
 *	coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleLine(canvas, itemPtr, originX, originY, scaleX, scaleY)
    Tk_Canvas canvas;			/* Canvas containing line. */
    Tk_Item *itemPtr;			/* Line to be scaled. */
    double originX, originY;		/* Origin about which to scale rect. */
    double scaleX;			/* Amount to scale in X direction. */
    double scaleY;			/* Amount to scale in Y direction. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    double *coordPtr;
    int i;

    /*
     * Delete any arrowheads before scaling all the points (so that
     * the end-points of the line get restored).
     */

    if (linePtr->firstArrowPtr != NULL) {
	linePtr->coordPtr[0] = linePtr->firstArrowPtr[0];
	linePtr->coordPtr[1] = linePtr->firstArrowPtr[1];
	ckfree((char *) linePtr->firstArrowPtr);
	linePtr->firstArrowPtr = NULL;
    }
    if (linePtr->lastArrowPtr != NULL) {
	int i;

	i = 2*(linePtr->numPoints-1);
	linePtr->coordPtr[i] = linePtr->lastArrowPtr[0];
	linePtr->coordPtr[i+1] = linePtr->lastArrowPtr[1];
	ckfree((char *) linePtr->lastArrowPtr);
	linePtr->lastArrowPtr = NULL;
    }
    for (i = 0, coordPtr = linePtr->coordPtr; i < linePtr->numPoints;
	    i++, coordPtr += 2) {
	coordPtr[0] = originX + scaleX*(*coordPtr - originX);
	coordPtr[1] = originY + scaleY*(coordPtr[1] - originY);
    }
    if (linePtr->arrow != ARROWS_NONE) {
	ConfigureArrows(canvas, linePtr);
    }
    ComputeLineBbox(canvas, linePtr);
}

/*
 *--------------------------------------------------------------
 *
 * GetLineIndex --
 *
 *	Parse an index into a line item and return either its value
 *	or an error.
 *
 * Results:
 *	A standard Tcl result.  If all went well, then *indexPtr is
 *	filled in with the index (into itemPtr) corresponding to
 *	string.  Otherwise an error message is left in
 *	interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetLineIndex(interp, canvas, itemPtr, obj, indexPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item for which the index is being
				 * specified. */
    Tcl_Obj *obj;		/* Specification of a particular coord
				 * in itemPtr's line. */
    int *indexPtr;		/* Where to store converted index. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    size_t length;
    char *string = Tcl_GetStringFromObj(obj, (int *) &length);

    if (string[0] == 'e') {
	if (strncmp(string, "end", length) == 0) {
	    *indexPtr = 2*linePtr->numPoints;
	} else {
	    badIndex:

	    /*
	     * Some of the paths here leave messages in interp->result,
	     * so we have to clear it out before storing our own message.
	     */

	    Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
	    Tcl_AppendResult(interp, "bad index \"", string, "\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
    } else if (string[0] == '@') {
	int i;
	double x ,y, bestDist, dist, *coordPtr;
	char *end, *p;

	p = string+1;
	x = strtod(p, &end);
	if ((end == p) || (*end != ',')) {
	    goto badIndex;
	}
	p = end+1;
	y = strtod(p, &end);
	if ((end == p) || (*end != 0)) {
	    goto badIndex;
	}
	bestDist = 1.0e36;
	coordPtr = linePtr->coordPtr;
	*indexPtr = 0;
	for(i=0; i<linePtr->numPoints; i++) {
	    dist = hypot(coordPtr[0] - x, coordPtr[1] - y);
	    if (dist<bestDist) {
		bestDist = dist;
		*indexPtr = 2*i;
	    }
	    coordPtr += 2;
	}
    } else {
	if (Tcl_GetIntFromObj(interp, obj, indexPtr) != TCL_OK) {
	    goto badIndex;
	}
	*indexPtr &= -2; /* if index is odd, make it even */
	if (*indexPtr < 0){
	    *indexPtr = 0;
	} else if (*indexPtr > (2*linePtr->numPoints)) {
	    *indexPtr = (2*linePtr->numPoints);
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TranslateLine --
 *
 *	This procedure is called to move a line by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the line is offset by (xDelta, yDelta), and
 *	the bounding box is updated in the generic part of the item
 *	structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateLine(canvas, itemPtr, deltaX, deltaY)
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item that is being moved. */
    double deltaX, deltaY;		/* Amount by which item is to be
					 * moved. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    double *coordPtr;
    int i;

    for (i = 0, coordPtr = linePtr->coordPtr; i < linePtr->numPoints;
	    i++, coordPtr += 2) {
	coordPtr[0] += deltaX;
	coordPtr[1] += deltaY;
    }
    if (linePtr->firstArrowPtr != NULL) {
	for (i = 0, coordPtr = linePtr->firstArrowPtr; i < PTS_IN_ARROW;
		i++, coordPtr += 2) {
	    coordPtr[0] += deltaX;
	    coordPtr[1] += deltaY;
	}
    }
    if (linePtr->lastArrowPtr != NULL) {
	for (i = 0, coordPtr = linePtr->lastArrowPtr; i < PTS_IN_ARROW;
		i++, coordPtr += 2) {
	    coordPtr[0] += deltaX;
	    coordPtr[1] += deltaY;
	}
    }
    ComputeLineBbox(canvas, linePtr);
}

/*
 *--------------------------------------------------------------
 *
 * ParseArrowShape --
 *
 *	This procedure is called back during option parsing to
 *	parse arrow shape information.
 *
 * Results:
 *	The return value is a standard Tcl result:  TCL_OK means
 *	that the arrow shape information was parsed ok, and
 *	TCL_ERROR means it couldn't be parsed.
 *
 * Side effects:
 *	Arrow information in recordPtr is updated.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ParseArrowShape(clientData, interp, tkwin, value, recordPtr, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Window tkwin;		/* Not used. */
    CONST char *value;		/* Textual specification of arrow shape. */
    char *recordPtr;		/* Pointer to item record in which to
				 * store arrow information. */
    int offset;			/* Offset of shape information in widget
				 * record. */
{
    LineItem *linePtr = (LineItem *) recordPtr;
    double a, b, c;
    int argc;
    CONST char **argv = NULL;

    if (offset != Tk_Offset(LineItem, arrowShapeA)) {
	panic("ParseArrowShape received bogus offset");
    }

    if (Tcl_SplitList(interp, (char *) value, &argc, &argv) != TCL_OK) {
	syntaxError:
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "bad arrow shape \"", value,
		"\": must be list with three numbers", (char *) NULL);
	if (argv != NULL) {
	    ckfree((char *) argv);
	}
	return TCL_ERROR;
    }
    if (argc != 3) {
	goto syntaxError;
    }
    if ((Tk_CanvasGetCoord(interp, linePtr->canvas, argv[0], &a) != TCL_OK)
	    || (Tk_CanvasGetCoord(interp, linePtr->canvas, argv[1], &b)
		!= TCL_OK)
	    || (Tk_CanvasGetCoord(interp, linePtr->canvas, argv[2], &c)
		!= TCL_OK)) {
	goto syntaxError;
    }
    linePtr->arrowShapeA = (float)a;
    linePtr->arrowShapeB = (float)b;
    linePtr->arrowShapeC = (float)c;
    ckfree((char *) argv);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * PrintArrowShape --
 *
 *	This procedure is a callback invoked by the configuration
 *	code to return a printable value describing an arrow shape.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

    /* ARGSUSED */
static char *
PrintArrowShape(clientData, tkwin, recordPtr, offset, freeProcPtr)
    ClientData clientData;	/* Not used. */
    Tk_Window tkwin;		/* Window associated with linePtr's widget. */
    char *recordPtr;		/* Pointer to item record containing current
				 * shape information. */
    int offset;			/* Offset of arrow information in record. */
    Tcl_FreeProc **freeProcPtr;	/* Store address of procedure to call to
				 * free string here. */
{
    LineItem *linePtr = (LineItem *) recordPtr;
    char *buffer;

    buffer = (char *) ckalloc(120);
    sprintf(buffer, "%.5g %.5g %.5g", linePtr->arrowShapeA,
	    linePtr->arrowShapeB, linePtr->arrowShapeC);
    *freeProcPtr = TCL_DYNAMIC;
    return buffer;
}


/*
 *--------------------------------------------------------------
 *
 * ArrowParseProc --
 *
 *	This procedure is invoked during option processing to handle
 *	the "-arrow" option.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The arrow for a given item gets replaced by the arrow
 *	indicated in the value argument.
 *
 *--------------------------------------------------------------
 */

static int
ArrowParseProc(clientData, interp, tkwin, value, widgRec, offset)
    ClientData clientData;		/* some flags.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    CONST char *value;			/* Value of option. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
{
    int c;
    size_t length;

    register Arrows *arrowPtr = (Arrows *) (widgRec + offset);

    if(value == NULL || *value == 0) {
	*arrowPtr = ARROWS_NONE;
	return TCL_OK;
    }

    c = value[0];
    length = strlen(value);

    if ((c == 'n') && (strncmp(value, "none", length) == 0)) {
	*arrowPtr = ARROWS_NONE;
	return TCL_OK;
    }
    if ((c == 'f') && (strncmp(value, "first", length) == 0)) {
	*arrowPtr = ARROWS_FIRST;
	return TCL_OK;
    }
    if ((c == 'l') && (strncmp(value, "last", length) == 0)) {
	*arrowPtr = ARROWS_LAST;
	return TCL_OK;
    }
    if ((c == 'b') && (strncmp(value, "both", length) == 0)) {
	*arrowPtr = ARROWS_BOTH;
	return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad arrow spec \"", value, 
	    "\": must be none, first, last, or both",
	    (char *) NULL);
    *arrowPtr = ARROWS_NONE;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * ArrowPrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-arrow"
 *	configuration option.
 *
 * Results:
 *	The return value is a string describing the arrows for
 *	the item referred to by "widgRec".  In addition, *freeProcPtr
 *	is filled in with the address of a procedure to call to free
 *	the result string when it's no longer needed (or NULL to
 *	indicate that the string doesn't need to be freed).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static char *
ArrowPrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    register Arrows *arrowPtr = (Arrows *) (widgRec + offset);

    switch (*arrowPtr) {
      case ARROWS_FIRST:
	return "first";
      case ARROWS_LAST:
	return "last";
      case ARROWS_BOTH:
	return "both";
      default:
	return "none";
    }
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureArrows --
 *
 *	If arrowheads have been requested for a line, this
 *	procedure makes arrangements for the arrowheads.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	Information in linePtr is set up for one or two arrowheads.
 *	the firstArrowPtr and lastArrowPtr polygons are allocated
 *	and initialized, if need be, and the end points of the line
 *	are adjusted so that a thick line doesn't stick out past
 *	the arrowheads.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConfigureArrows(canvas, linePtr)
    Tk_Canvas canvas;			/* Canvas in which arrows will be
					 * displayed (interp and tkwin
					 * fields are needed). */
    LineItem *linePtr;			/* Item to configure for arrows. */
{
    double *poly, *coordPtr;
    double dx, dy, length, sinTheta, cosTheta, temp;
    double fracHeight;			/* Line width as fraction of
					 * arrowhead width. */
    double backup;			/* Distance to backup end points
					 * so the line ends in the middle
					 * of the arrowhead. */
    double vertX, vertY;		/* Position of arrowhead vertex. */
    double shapeA, shapeB, shapeC;	/* Adjusted coordinates (see
					 * explanation below). */
    double width;
    Tk_State state = linePtr->header.state;

    if (linePtr->numPoints <2) {
	return TCL_OK;
    }

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = linePtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)linePtr) {
	if (linePtr->outline.activeWidth>width) {
	    width = linePtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.disabledWidth>0) {
	    width = linePtr->outline.disabledWidth;
	}
    }

    /*
     * The code below makes a tiny increase in the shape parameters
     * for the line.  This is a bit of a hack, but it seems to result
     * in displays that more closely approximate the specified parameters.
     * Without the adjustment, the arrows come out smaller than expected.
     */

    shapeA = linePtr->arrowShapeA + 0.001;
    shapeB = linePtr->arrowShapeB + 0.001;
    shapeC = linePtr->arrowShapeC + width/2.0 + 0.001;

    /*
     * If there's an arrowhead on the first point of the line, compute
     * its polygon and adjust the first point of the line so that the
     * line doesn't stick out past the leading edge of the arrowhead.
     */

    fracHeight = (width/2.0)/shapeC;
    backup = fracHeight*shapeB + shapeA*(1.0 - fracHeight)/2.0;
    if (linePtr->arrow != ARROWS_LAST) {
	poly = linePtr->firstArrowPtr;
	if (poly == NULL) {
	    poly = (double *) ckalloc((unsigned)
		    (2*PTS_IN_ARROW*sizeof(double)));
	    poly[0] = poly[10] = linePtr->coordPtr[0];
	    poly[1] = poly[11] = linePtr->coordPtr[1];
	    linePtr->firstArrowPtr = poly;
	}
	dx = poly[0] - linePtr->coordPtr[2];
	dy = poly[1] - linePtr->coordPtr[3];
	length = hypot(dx, dy);
	if (length == 0) {
	    sinTheta = cosTheta = 0.0;
	} else {
	    sinTheta = dy/length;
	    cosTheta = dx/length;
	}
	vertX = poly[0] - shapeA*cosTheta;
	vertY = poly[1] - shapeA*sinTheta;
	temp = shapeC*sinTheta;
	poly[2] = poly[0] - shapeB*cosTheta + temp;
	poly[8] = poly[2] - 2*temp;
	temp = shapeC*cosTheta;
	poly[3] = poly[1] - shapeB*sinTheta - temp;
	poly[9] = poly[3] + 2*temp;
	poly[4] = poly[2]*fracHeight + vertX*(1.0-fracHeight);
	poly[5] = poly[3]*fracHeight + vertY*(1.0-fracHeight);
	poly[6] = poly[8]*fracHeight + vertX*(1.0-fracHeight);
	poly[7] = poly[9]*fracHeight + vertY*(1.0-fracHeight);

	/*
	 * Polygon done.  Now move the first point towards the second so
	 * that the corners at the end of the line are inside the
	 * arrowhead.
	 */

	linePtr->coordPtr[0] = poly[0] - backup*cosTheta;
	linePtr->coordPtr[1] = poly[1] - backup*sinTheta;
    }

    /*
     * Similar arrowhead calculation for the last point of the line.
     */

    if (linePtr->arrow != ARROWS_FIRST) {
	coordPtr = linePtr->coordPtr + 2*(linePtr->numPoints-2);
	poly = linePtr->lastArrowPtr;
	if (poly == NULL) {
	    poly = (double *) ckalloc((unsigned)
		    (2*PTS_IN_ARROW*sizeof(double)));
	    poly[0] = poly[10] = coordPtr[2];
	    poly[1] = poly[11] = coordPtr[3];
	    linePtr->lastArrowPtr = poly;
	}
	dx = poly[0] - coordPtr[0];
	dy = poly[1] - coordPtr[1];
	length = hypot(dx, dy);
	if (length == 0) {
	    sinTheta = cosTheta = 0.0;
	} else {
	    sinTheta = dy/length;
	    cosTheta = dx/length;
	}
	vertX = poly[0] - shapeA*cosTheta;
	vertY = poly[1] - shapeA*sinTheta;
	temp = shapeC*sinTheta;
	poly[2] = poly[0] - shapeB*cosTheta + temp;
	poly[8] = poly[2] - 2*temp;
	temp = shapeC*cosTheta;
	poly[3] = poly[1] - shapeB*sinTheta - temp;
	poly[9] = poly[3] + 2*temp;
	poly[4] = poly[2]*fracHeight + vertX*(1.0-fracHeight);
	poly[5] = poly[3]*fracHeight + vertY*(1.0-fracHeight);
	poly[6] = poly[8]*fracHeight + vertX*(1.0-fracHeight);
	poly[7] = poly[9]*fracHeight + vertY*(1.0-fracHeight);
	coordPtr[2] = poly[0] - backup*cosTheta;
	coordPtr[3] = poly[1] - backup*sinTheta;
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * LineToPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	line items.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in the interp's result, replacing whatever used
 *	to be there.  If no error occurs, then Postscript for the
 *	item is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
LineToPostscript(interp, canvas, itemPtr, prepass)
    Tcl_Interp *interp;			/* Leave Postscript or error message
					 * here. */
    Tk_Canvas canvas;			/* Information about overall canvas. */
    Tk_Item *itemPtr;			/* Item for which Postscript is
					 * wanted. */
    int prepass;			/* 1 means this is a prepass to
					 * collect font information;  0 means
					 * final Postscript is being created. */
{
    LineItem *linePtr = (LineItem *) itemPtr;
    char buffer[64 + TCL_INTEGER_SPACE];
    char *style;

    double width;
    XColor *color;
    Pixmap stipple;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = linePtr->outline.width;
    color = linePtr->outline.color;
    stipple = linePtr->outline.stipple;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (linePtr->outline.activeWidth>width) {
	    width = linePtr->outline.activeWidth;
	}
	if (linePtr->outline.activeColor!=NULL) {
	    color = linePtr->outline.activeColor;
	}
	if (linePtr->outline.activeStipple!=None) {
	    stipple = linePtr->outline.activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.disabledWidth>0) {
	    width = linePtr->outline.disabledWidth;
	}
	if (linePtr->outline.disabledColor!=NULL) {
	    color = linePtr->outline.disabledColor;
	}
	if (linePtr->outline.disabledStipple!=None) {
	    stipple = linePtr->outline.disabledStipple;
	}
    }

    if (color == NULL || linePtr->numPoints<1 || linePtr->coordPtr==NULL) {
	return TCL_OK;
    }

    if (linePtr->numPoints==1) {
	sprintf(buffer, "%.15g %.15g translate %.15g %.15g",
		linePtr->coordPtr[0], Tk_CanvasPsY(canvas, linePtr->coordPtr[1]),
		width/2.0, width/2.0);
	Tcl_AppendResult(interp, "matrix currentmatrix\n",buffer,
		" scale 1 0 moveto 0 0 1 0 360 arc\nsetmatrix\n", (char *) NULL);
	if (Tk_CanvasPsColor(interp, canvas, color)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if (stipple != None) {
	    Tcl_AppendResult(interp, "clip ", (char *) NULL);
	    if (Tk_CanvasPsStipple(interp, canvas, stipple) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    Tcl_AppendResult(interp, "fill\n", (char *) NULL);
	}
	return TCL_OK;
    }
    /*
     * Generate a path for the line's center-line (do this differently
     * for straight lines and smoothed lines).
     */

    if ((!linePtr->smooth) || (linePtr->numPoints < 3)) {
	Tk_CanvasPsPath(interp, canvas, linePtr->coordPtr, linePtr->numPoints);
    } else {
	if ((stipple == None) && linePtr->smooth->postscriptProc) {
	    linePtr->smooth->postscriptProc(interp, canvas,
		    linePtr->coordPtr, linePtr->numPoints, linePtr->splineSteps);
	} else {
	    /*
	     * Special hack: Postscript printers don't appear to be able
	     * to turn a path drawn with "curveto"s into a clipping path
	     * without exceeding resource limits, so TkMakeBezierPostscript
	     * won't work for stippled curves.  Instead, generate all of
	     * the intermediate points here and output them into the
	     * Postscript file with "lineto"s instead.
	     */

	    double staticPoints[2*MAX_STATIC_POINTS];
	    double *pointPtr;
	    int numPoints;

	    numPoints = linePtr->smooth->coordProc(canvas, (double *) NULL,
		    linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		    (double *) NULL);
	    pointPtr = staticPoints;
	    if (numPoints > MAX_STATIC_POINTS) {
		pointPtr = (double *) ckalloc((unsigned)
			(numPoints * 2 * sizeof(double)));
	    }
	    numPoints = linePtr->smooth->coordProc(canvas, linePtr->coordPtr,
		    linePtr->numPoints, linePtr->splineSteps, (XPoint *) NULL,
		    pointPtr);
	    Tk_CanvasPsPath(interp, canvas, pointPtr, numPoints);
	    if (pointPtr != staticPoints) {
		ckfree((char *) pointPtr);
	    }
	}
    }

    /*
     * Set other line-drawing parameters and stroke out the line.
     */

    style = "0 setlinecap\n";
    if (linePtr->capStyle == CapRound) {
	style = "1 setlinecap\n";
    } else if (linePtr->capStyle == CapProjecting) {
	style = "2 setlinecap\n";
    }
    Tcl_AppendResult(interp, style, (char *) NULL);
    style = "0 setlinejoin\n";
    if (linePtr->joinStyle == JoinRound) {
	style = "1 setlinejoin\n";
    } else if (linePtr->joinStyle == JoinBevel) {
	style = "2 setlinejoin\n";
    }
    Tcl_AppendResult(interp, style, (char *) NULL);

    if (Tk_CanvasPsOutline(canvas, itemPtr,
	    &(linePtr->outline)) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Output polygons for the arrowheads, if there are any.
     */

    if (linePtr->firstArrowPtr != NULL) {
	if (stipple != None) {
	    Tcl_AppendResult(interp, "grestore gsave\n",
		    (char *) NULL);
	}
	if (ArrowheadPostscript(interp, canvas, linePtr,
		linePtr->firstArrowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (linePtr->lastArrowPtr != NULL) {
	if (stipple != None) {
	    Tcl_AppendResult(interp, "grestore gsave\n", (char *) NULL);
	}
	if (ArrowheadPostscript(interp, canvas, linePtr,
		linePtr->lastArrowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ArrowheadPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	an arrowhead for a line item.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in the interp's result, replacing whatever used
 *	to be there.  If no error occurs, then Postscript for the
 *	arrowhead is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
ArrowheadPostscript(interp, canvas, linePtr, arrowPtr)
    Tcl_Interp *interp;			/* Leave Postscript or error message
					 * here. */
    Tk_Canvas canvas;			/* Information about overall canvas. */
    LineItem *linePtr;			/* Line item for which Postscript is
					 * being generated. */
    double *arrowPtr;			/* Pointer to first of five points
					 * describing arrowhead polygon. */
{
    Pixmap stipple;
    Tk_State state = linePtr->header.state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    stipple = linePtr->outline.stipple;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)linePtr) {
	if (linePtr->outline.activeStipple!=None) {
	    stipple = linePtr->outline.activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.activeStipple!=None) {
	    stipple = linePtr->outline.disabledStipple;
	}
    }

    Tk_CanvasPsPath(interp, canvas, arrowPtr, PTS_IN_ARROW);
    if (stipple != None) {
	Tcl_AppendResult(interp, "clip ", (char *) NULL);
	if (Tk_CanvasPsStipple(interp, canvas, stipple)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "fill\n", (char *) NULL);
    }
    return TCL_OK;
}
