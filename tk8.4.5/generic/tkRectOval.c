/* 
 * tkRectOval.c --
 *
 *	This file implements rectangle and oval items for canvas
 *	widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <stdio.h>
#include "tk.h"
#include "tkInt.h"
#include "tkPort.h"
#include "tkCanvas.h"

/*
 * The structure below defines the record for each rectangle/oval item.
 */

typedef struct RectOvalItem  {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types.  MUST BE FIRST IN STRUCTURE. */
    Tk_Outline outline;		/* Outline structure */
    double bbox[4];		/* Coordinates of bounding box for rectangle
				 * or oval (x1, y1, x2, y2).  Item includes
				 * x1 and x2 but not y1 and y2. */
    Tk_TSOffset tsoffset;
    XColor *fillColor;		/* Color for filling rectangle/oval. */
    XColor *activeFillColor;	/* Color for filling rectangle/oval if state is active. */
    XColor *disabledFillColor;	/* Color for filling rectangle/oval if state is disabled. */
    Pixmap fillStipple;		/* Stipple bitmap for filling item. */
    Pixmap activeFillStipple;	/* Stipple bitmap for filling item if state is active. */
    Pixmap disabledFillStipple;	/* Stipple bitmap for filling item if state is disabled. */
    GC fillGC;			/* Graphics context for filling item. */
} RectOvalItem;

/*
 * Information used for parsing configuration specs:
 */

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
    TkOffsetPrintProc, (ClientData) TK_OFFSET_RELATIVE
};
static Tk_CustomOption pixelOption = {
    (Tk_OptionParseProc *) TkPixelParseProc,
    TkPixelPrintProc, (ClientData) NULL
};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_CUSTOM, "-activedash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.activeDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-activefill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, activeFillColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-activeoutline", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.activeColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-activeoutlinestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.activeStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-activestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, activeFillStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-activewidth", (char *) NULL, (char *) NULL,
	"0.0", Tk_Offset(RectOvalItem, outline.activeWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CUSTOM, "-dash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.dash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_PIXELS, "-dashoffset", (char *) NULL, (char *) NULL,
	"0", Tk_Offset(RectOvalItem, outline.offset),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, "-disableddash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.disabledDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-disabledfill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, disabledFillColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-disabledoutline", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.disabledColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-disabledoutlinestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.disabledStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-disabledstipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, disabledFillStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-disabledwidth", (char *) NULL, (char *) NULL,
	"0.0", Tk_Offset(RectOvalItem, outline.disabledWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_COLOR, "-fill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, fillColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-offset", (char *) NULL, (char *) NULL,
	"0,0", Tk_Offset(RectOvalItem, tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_COLOR, "-outline", (char *) NULL, (char *) NULL,
	"black", Tk_Offset(RectOvalItem, outline.color), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-outlineoffset", (char *) NULL, (char *) NULL,
	"0,0", Tk_Offset(RectOvalItem, outline.tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_BITMAP, "-outlinestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, outline.stipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-state", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(Tk_Item, state),TK_CONFIG_NULL_OK,
	&stateOption},
    {TK_CONFIG_BITMAP, "-stipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, fillStipple), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-tags", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_CUSTOM, "-width", (char *) NULL, (char *) NULL,
	"1.0", Tk_Offset(RectOvalItem, outline.width),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Prototypes for procedures defined in this file:
 */

static void		ComputeRectOvalBbox _ANSI_ARGS_((Tk_Canvas canvas,
			    RectOvalItem *rectOvalPtr));
static int		ConfigureRectOval _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *CONST objv[], int flags));
static int		CreateRectOval _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int objc, Tcl_Obj *CONST objv[]));
static void		DeleteRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display));
static void		DisplayRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height));
static int		OvalToArea _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *areaPtr));
static double		OvalToPoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *pointPtr));
static int		RectOvalCoords _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static int		RectOvalToPostscript _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass));
static int		RectToArea _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *areaPtr));
static double		RectToPoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *pointPtr));
static void		ScaleRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY));
static void		TranslateRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY));

/*
 * The structures below defines the rectangle and oval item types
 * by means of procedures that can be invoked by generic item code.
 */

Tk_ItemType tkRectangleType = {
    "rectangle",			/* name */
    sizeof(RectOvalItem),		/* itemSize */
    CreateRectOval,			/* createProc */
    configSpecs,			/* configSpecs */
    ConfigureRectOval,			/* configureProc */
    RectOvalCoords,			/* coordProc */
    DeleteRectOval,			/* deleteProc */
    DisplayRectOval,			/* displayProc */
    TK_CONFIG_OBJS,			/* flags */
    RectToPoint,			/* pointProc */
    RectToArea,				/* areaProc */
    RectOvalToPostscript,		/* postscriptProc */
    ScaleRectOval,			/* scaleProc */
    TranslateRectOval,			/* translateProc */
    (Tk_ItemIndexProc *) NULL,		/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* icursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) NULL,		/* insertProc */
    (Tk_ItemDCharsProc *) NULL,		/* dTextProc */
    (Tk_ItemType *) NULL,		/* nextPtr */
};

Tk_ItemType tkOvalType = {
    "oval",				/* name */
    sizeof(RectOvalItem),		/* itemSize */
    CreateRectOval,			/* createProc */
    configSpecs,			/* configSpecs */
    ConfigureRectOval,			/* configureProc */
    RectOvalCoords,			/* coordProc */
    DeleteRectOval,			/* deleteProc */
    DisplayRectOval,			/* displayProc */
    TK_CONFIG_OBJS,			/* flags */
    OvalToPoint,			/* pointProc */
    OvalToArea,				/* areaProc */
    RectOvalToPostscript,		/* postscriptProc */
    ScaleRectOval,			/* scaleProc */
    TranslateRectOval,			/* translateProc */
    (Tk_ItemIndexProc *) NULL,		/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* cursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) NULL,		/* insertProc */
    (Tk_ItemDCharsProc *) NULL,		/* dTextProc */
    (Tk_ItemType *) NULL,		/* nextPtr */
};

/*
 *--------------------------------------------------------------
 *
 * CreateRectOval --
 *
 *	This procedure is invoked to create a new rectangle
 *	or oval item in a canvas.
 *
 * Results:
 *	A standard Tcl return value.  If an error occurred in
 *	creating the item, then an error message is left in
 *	the interp's result;  in this case itemPtr is left uninitialized,
 *	so it can be safely freed by the caller.
 *
 * Side effects:
 *	A new rectangle or oval item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateRectOval(interp, canvas, itemPtr, objc, objv)
    Tcl_Interp *interp;			/* For error reporting. */
    Tk_Canvas canvas;			/* Canvas to hold new item. */
    Tk_Item *itemPtr;			/* Record to hold new item;  header
					 * has been initialized by caller. */
    int objc;				/* Number of arguments in objv. */
    Tcl_Obj *CONST objv[];		/* Arguments describing rectangle. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    int i;

    if (objc == 0) {
	panic("canvas did not pass any coords\n");
    }

    /*
     * Carry out initialization that is needed in order to clean
     * up after errors during the the remainder of this procedure.
     */

    Tk_CreateOutline(&(rectOvalPtr->outline));
    rectOvalPtr->tsoffset.flags = 0;
    rectOvalPtr->tsoffset.xoffset = 0;
    rectOvalPtr->tsoffset.yoffset = 0;
    rectOvalPtr->fillColor = NULL;
    rectOvalPtr->activeFillColor = NULL;
    rectOvalPtr->disabledFillColor = NULL;
    rectOvalPtr->fillStipple = None;
    rectOvalPtr->activeFillStipple = None;
    rectOvalPtr->disabledFillStipple = None;
    rectOvalPtr->fillGC = None;

    /*
     * Process the arguments to fill in the item record.
     */

    for (i = 1; i < objc; i++) {
	char *arg = Tcl_GetString(objv[i]);
	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    break;
	}
    }
    if ((RectOvalCoords(interp, canvas, itemPtr, i, objv) != TCL_OK)) {
	goto error;
    }
    if (ConfigureRectOval(interp, canvas, itemPtr, objc-i, objv+i, 0)
	    == TCL_OK) {
	return TCL_OK;
    }

    error:
    DeleteRectOval(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalCoords --
 *
 *	This procedure is invoked to process the "coords" widget
 *	command on rectangles and ovals.  See the user documentation
 *	for details on what it does.
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
RectOvalCoords(interp, canvas, itemPtr, objc, objv)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item whose coordinates are to be
					 * read or modified. */
    int objc;				/* Number of coordinates supplied in
					 * objv. */
    Tcl_Obj *CONST objv[];		/* Array of coordinates: x1, y1,
					 * x2, y2, ... */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *obj = Tcl_NewObj();
	Tcl_Obj *subobj = Tcl_NewDoubleObj(rectOvalPtr->bbox[0]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(rectOvalPtr->bbox[1]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(rectOvalPtr->bbox[2]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(rectOvalPtr->bbox[3]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	Tcl_SetObjResult(interp, obj);
    } else if ((objc == 1)||(objc == 4)) {
 	if (objc==1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		    (Tcl_Obj ***) &objv) != TCL_OK) {
		return TCL_ERROR;
	    } else if (objc != 4) {
		char buf[64 + TCL_INTEGER_SPACE];

		sprintf(buf, "wrong # coordinates: expected 0 or 4, got %d", objc);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		return TCL_ERROR;
	    }
	}
	if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0],
 		    &rectOvalPtr->bbox[0]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
		    &rectOvalPtr->bbox[1]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[2],
			&rectOvalPtr->bbox[2]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[3],
			&rectOvalPtr->bbox[3]) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeRectOvalBbox(canvas, rectOvalPtr);
    } else {
	char buf[64 + TCL_INTEGER_SPACE];
	
	sprintf(buf, "wrong # coordinates: expected 0 or 4, got %d", objc);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureRectOval --
 *
 *	This procedure is invoked to configure various aspects
 *	of a rectangle or oval item, such as its border and
 *	background colors.
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
ConfigureRectOval(interp, canvas, itemPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Rectangle item to reconfigure. */
    int objc;			/* Number of elements in objv.  */
    Tcl_Obj *CONST objv[];	/* Arguments describing things to configure. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_TSOffset *tsoffset;
    XColor *color;
    Pixmap stipple;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);

    if (TCL_OK != Tk_ConfigureWidget(interp, tkwin, configSpecs, objc,
	    (CONST char **) objv, (char *) rectOvalPtr, flags|TK_CONFIG_OBJS)) {
	return TCL_ERROR;
    }
    state = itemPtr->state;

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */

    if (rectOvalPtr->outline.activeWidth > rectOvalPtr->outline.width ||
	    rectOvalPtr->outline.activeDash.number != 0 ||
	    rectOvalPtr->outline.activeColor != NULL ||
	    rectOvalPtr->outline.activeStipple != None ||
	    rectOvalPtr->activeFillColor != NULL ||
	    rectOvalPtr->activeFillStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }

    tsoffset = &rectOvalPtr->outline.tsoffset;
    flags = tsoffset->flags;
    if (flags & TK_OFFSET_LEFT) {
	tsoffset->xoffset = (int) (rectOvalPtr->bbox[0] + 0.5);
    } else if (flags & TK_OFFSET_CENTER) {
	tsoffset->xoffset = (int) ((rectOvalPtr->bbox[0]+rectOvalPtr->bbox[2]+1)/2);
    } else if (flags & TK_OFFSET_RIGHT) {
	tsoffset->xoffset = (int) (rectOvalPtr->bbox[2] + 0.5);
    }
    if (flags & TK_OFFSET_TOP) {
	tsoffset->yoffset = (int) (rectOvalPtr->bbox[1] + 0.5);
    } else if (flags & TK_OFFSET_MIDDLE) {
	tsoffset->yoffset = (int) ((rectOvalPtr->bbox[1]+rectOvalPtr->bbox[3]+1)/2);
    } else if (flags & TK_OFFSET_BOTTOM) {
	tsoffset->yoffset = (int) (rectOvalPtr->bbox[2] + 0.5);
    }

    /*
     * Configure the outline graphics context.  If mask is non-zero,
     * the gc has changed and must be reallocated, provided that the
     * new settings specify a valid outline (non-zero width and non-NULL
     * color)
     */

    mask = Tk_ConfigOutlineGC(&gcValues, canvas, itemPtr,
	     &(rectOvalPtr->outline));
    if (mask && \
	    rectOvalPtr->outline.width != 0 && \
	    rectOvalPtr->outline.color != NULL) {
	gcValues.cap_style = CapProjecting;
	mask |= GCCapStyle;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    } else {
	newGC = None;
    }
    if (rectOvalPtr->outline.gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), rectOvalPtr->outline.gc);
    }
    rectOvalPtr->outline.gc = newGC;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    if (state==TK_STATE_HIDDEN) {
	ComputeRectOvalBbox(canvas, rectOvalPtr);
	return TCL_OK;
    }

    color = rectOvalPtr->fillColor;
    stipple = rectOvalPtr->fillStipple;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectOvalPtr->activeFillColor!=NULL) {
	    color = rectOvalPtr->activeFillColor;
	}
	if (rectOvalPtr->activeFillStipple!=None) {
	    stipple = rectOvalPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectOvalPtr->disabledFillColor!=NULL) {
	    color = rectOvalPtr->disabledFillColor;
	}
	if (rectOvalPtr->disabledFillStipple!=None) {
	    stipple = rectOvalPtr->disabledFillStipple;
	}
    }

    if (color == NULL) {
	newGC = None;
    } else {
	gcValues.foreground = color->pixel;
	if (stipple != None) {
	    gcValues.stipple = stipple;
	    gcValues.fill_style = FillStippled;
	    mask = GCForeground|GCStipple|GCFillStyle;
	} else {
	    mask = GCForeground;
	}
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (rectOvalPtr->fillGC != None) {
	Tk_FreeGC(Tk_Display(tkwin), rectOvalPtr->fillGC);
    }
    rectOvalPtr->fillGC = newGC;

    tsoffset = &rectOvalPtr->tsoffset;
    flags = tsoffset->flags;
    if (flags & TK_OFFSET_LEFT) {
	tsoffset->xoffset = (int) (rectOvalPtr->bbox[0] + 0.5);
    } else if (flags & TK_OFFSET_CENTER) {
	tsoffset->xoffset = (int) ((rectOvalPtr->bbox[0]+rectOvalPtr->bbox[2]+1)/2);
    } else if (flags & TK_OFFSET_RIGHT) {
	tsoffset->xoffset = (int) (rectOvalPtr->bbox[2] + 0.5);
    }
    if (flags & TK_OFFSET_TOP) {
	tsoffset->yoffset = (int) (rectOvalPtr->bbox[1] + 0.5);
    } else if (flags & TK_OFFSET_MIDDLE) {
	tsoffset->yoffset = (int) ((rectOvalPtr->bbox[1]+rectOvalPtr->bbox[3]+1)/2);
    } else if (flags & TK_OFFSET_BOTTOM) {
	tsoffset->yoffset = (int) (rectOvalPtr->bbox[3] + 0.5);
    }

    ComputeRectOvalBbox(canvas, rectOvalPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteRectOval --
 *
 *	This procedure is called to clean up the data structure
 *	associated with a rectangle or oval item.
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
DeleteRectOval(canvas, itemPtr, display)
    Tk_Canvas canvas;			/* Info about overall widget. */
    Tk_Item *itemPtr;			/* Item that is being deleted. */
    Display *display;			/* Display containing window for
					 * canvas. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    Tk_DeleteOutline(display, &(rectOvalPtr->outline));
    if (rectOvalPtr->fillColor != NULL) {
	Tk_FreeColor(rectOvalPtr->fillColor);
    }
    if (rectOvalPtr->activeFillColor != NULL) {
	Tk_FreeColor(rectOvalPtr->activeFillColor);
    }
    if (rectOvalPtr->disabledFillColor != NULL) {
	Tk_FreeColor(rectOvalPtr->disabledFillColor);
    }
    if (rectOvalPtr->fillStipple != None) {
	Tk_FreeBitmap(display, rectOvalPtr->fillStipple);
    }
    if (rectOvalPtr->activeFillStipple != None) {
	Tk_FreeBitmap(display, rectOvalPtr->activeFillStipple);
    }
    if (rectOvalPtr->disabledFillStipple != None) {
	Tk_FreeBitmap(display, rectOvalPtr->disabledFillStipple);
    }
    if (rectOvalPtr->fillGC != None) {
	Tk_FreeGC(display, rectOvalPtr->fillGC);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeRectOvalBbox --
 *
 *	This procedure is invoked to compute the bounding box of
 *	all the pixels that may be drawn as part of a rectangle
 *	or oval.
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

	/* ARGSUSED */
static void
ComputeRectOvalBbox(canvas, rectOvalPtr)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    RectOvalItem *rectOvalPtr;		/* Item whose bbox is to be
					 * recomputed. */
{
    int bloat, tmp;
    double dtmp, width;
    Tk_State state = rectOvalPtr->header.state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = rectOvalPtr->outline.width;
    if (state==TK_STATE_HIDDEN) {
	rectOvalPtr->header.x1 = rectOvalPtr->header.y1 =
	rectOvalPtr->header.x2 = rectOvalPtr->header.y2 = -1;
	return;
    }
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)rectOvalPtr) {
	if (rectOvalPtr->outline.activeWidth>width) {
	    width = rectOvalPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectOvalPtr->outline.disabledWidth>0) {
	    width = rectOvalPtr->outline.disabledWidth;
	}
    }

    /*
     * Make sure that the first coordinates are the lowest ones.
     */

    if (rectOvalPtr->bbox[1] > rectOvalPtr->bbox[3]) {
	double tmp;
	tmp = rectOvalPtr->bbox[3];
	rectOvalPtr->bbox[3] = rectOvalPtr->bbox[1];
	rectOvalPtr->bbox[1] = tmp;
    }
    if (rectOvalPtr->bbox[0] > rectOvalPtr->bbox[2]) {
	double tmp;
	tmp = rectOvalPtr->bbox[2];
	rectOvalPtr->bbox[2] = rectOvalPtr->bbox[0];
	rectOvalPtr->bbox[0] = tmp;
    }

    if (rectOvalPtr->outline.gc == None) {
	/*
	 * The Win32 switch was added for 8.3 to solve a problem
	 * with ovals leaving traces on bottom and right of 1 pixel.
	 * This may not be the correct place to solve it, but it works.
	 */
#ifdef __WIN32__
	bloat = 1;
#else
	bloat = 0;
#endif
    } else {
	bloat = (int) (width+1)/2;
    }

    /*
     * Special note:  the rectangle is always drawn at least 1x1 in
     * size, so round up the upper coordinates to be at least 1 unit
     * greater than the lower ones.
     */

    tmp = (int) ((rectOvalPtr->bbox[0] >= 0) ? rectOvalPtr->bbox[0] + .5
	    : rectOvalPtr->bbox[0] - .5);
    rectOvalPtr->header.x1 = tmp - bloat;
    tmp = (int) ((rectOvalPtr->bbox[1] >= 0) ? rectOvalPtr->bbox[1] + .5
	    : rectOvalPtr->bbox[1] - .5);
    rectOvalPtr->header.y1 = tmp - bloat;
    dtmp = rectOvalPtr->bbox[2];
    if (dtmp < (rectOvalPtr->bbox[0] + 1)) {
	dtmp = rectOvalPtr->bbox[0] + 1;
    }
    tmp = (int) ((dtmp >= 0) ? dtmp + .5 : dtmp - .5);
    rectOvalPtr->header.x2 = tmp + bloat;
    dtmp = rectOvalPtr->bbox[3];
    if (dtmp < (rectOvalPtr->bbox[1] + 1)) {
	dtmp = rectOvalPtr->bbox[1] + 1;
    }
    tmp = (int) ((dtmp >= 0) ? dtmp + .5 : dtmp - .5);
    rectOvalPtr->header.y2 = tmp + bloat;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayRectOval --
 *
 *	This procedure is invoked to draw a rectangle or oval
 *	item in a given drawable.
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
DisplayRectOval(canvas, itemPtr, display, drawable, x, y, width, height)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    Tk_Item *itemPtr;			/* Item to be displayed. */
    Display *display;			/* Display on which to draw item. */
    Drawable drawable;			/* Pixmap or window in which to draw
					 * item. */
    int x, y, width, height;		/* Describes region of canvas that
					 * must be redisplayed (not used). */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    short x1, y1, x2, y2;
    Pixmap fillStipple;
    Tk_State state = itemPtr->state;

    /*
     * Compute the screen coordinates of the bounding box for the item.
     * Make sure that the bbox is at least one pixel large, since some
     * X servers will die if it isn't.
     */

    Tk_CanvasDrawableCoords(canvas, rectOvalPtr->bbox[0], rectOvalPtr->bbox[1],
	    &x1, &y1);
    Tk_CanvasDrawableCoords(canvas, rectOvalPtr->bbox[2], rectOvalPtr->bbox[3],
	    &x2, &y2);
    if (x2 <= x1) {
	x2 = x1+1;
    }
    if (y2 <= y1) {
	y2 = y1+1;
    }

    /*
     * Display filled part first (if wanted), then outline.  If we're
     * stippling, then modify the stipple offset in the GC.  Be sure to
     * reset the offset when done, since the GC is supposed to be
     * read-only.
     */

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    fillStipple = rectOvalPtr->fillStipple;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)rectOvalPtr) {
	if (rectOvalPtr->activeFillStipple!=None) {
	    fillStipple = rectOvalPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectOvalPtr->disabledFillStipple!=None) {
	    fillStipple = rectOvalPtr->disabledFillStipple;
	}
    }

    if (rectOvalPtr->fillGC != None) {
	if (fillStipple != None) {
	    Tk_TSOffset *tsoffset;
	    int w=0; int h=0;
	    tsoffset = &rectOvalPtr->tsoffset;
	    if (tsoffset) {
		int flags = tsoffset->flags;
		if (flags & (TK_OFFSET_CENTER|TK_OFFSET_MIDDLE)) {
		    Tk_SizeOfBitmap(display, fillStipple, &w, &h);
		    if (flags & TK_OFFSET_CENTER) {
			w /= 2;
		    } else {
			w = 0;
		    }
		    if (flags & TK_OFFSET_MIDDLE) {
			h /= 2;
		    } else {
			h = 0;
		    }
		}
		tsoffset->xoffset -= w;
		tsoffset->yoffset -= h;
	    }
	    Tk_CanvasSetOffset(canvas, rectOvalPtr->fillGC, tsoffset);
	    if (tsoffset) {
		tsoffset->xoffset += w;
		tsoffset->yoffset += h;
	    }
	}
	if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	    XFillRectangle(display, drawable, rectOvalPtr->fillGC,
		    x1, y1, (unsigned int) (x2-x1), (unsigned int) (y2-y1));
	} else {
	    XFillArc(display, drawable, rectOvalPtr->fillGC,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1),
		    0, 360*64);
	}
	if (fillStipple != None) {
	    XSetTSOrigin(display, rectOvalPtr->fillGC, 0, 0);
	}
    }
    if (rectOvalPtr->outline.gc != None) {
	Tk_ChangeOutlineGC(canvas, itemPtr, &(rectOvalPtr->outline));
	if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	    XDrawRectangle(display, drawable, rectOvalPtr->outline.gc,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1));
	} else {
	    XDrawArc(display, drawable, rectOvalPtr->outline.gc,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1), 0, 360*64);
	}
	Tk_ResetOutlineGC(canvas, itemPtr, &(rectOvalPtr->outline));
    }
}

/*
 *--------------------------------------------------------------
 *
 * RectToPoint --
 *
 *	Computes the distance from a given point to a given
 *	rectangle, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are coordPtr[0] and coordPtr[1] is inside the rectangle.  If the
 *	point isn't inside the rectangle then the return value is the
 *	distance from the point to the rectangle.  If itemPtr is filled,
 *	then anywhere in the interior is considered "inside"; if
 *	itemPtr isn't filled, then "inside" means only the area
 *	occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
RectToPoint(canvas, itemPtr, pointPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    RectOvalItem *rectPtr = (RectOvalItem *) itemPtr;
    double xDiff, yDiff, x1, y1, x2, y2, inc, tmp;
    double width;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = rectPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectPtr->outline.activeWidth>width) {
	    width = rectPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectPtr->outline.disabledWidth>0) {
	    width = rectPtr->outline.disabledWidth;
	}
    }

    /*
     * Generate a new larger rectangle that includes the border
     * width, if there is one.
     */

    x1 = rectPtr->bbox[0];
    y1 = rectPtr->bbox[1];
    x2 = rectPtr->bbox[2];
    y2 = rectPtr->bbox[3];
    if (rectPtr->outline.gc != None) {
	inc = width/2.0;
	x1 -= inc;
	y1 -= inc;
	x2 += inc;
	y2 += inc;
    }

    /*
     * If the point is inside the rectangle, handle specially:
     * distance is 0 if rectangle is filled, otherwise compute
     * distance to nearest edge of rectangle and subtract width
     * of edge.
     */

    if ((pointPtr[0] >= x1) && (pointPtr[0] < x2)
		&& (pointPtr[1] >= y1) && (pointPtr[1] < y2)) {
	if ((rectPtr->fillGC != None) || (rectPtr->outline.gc == None)) {
	    return 0.0;
	}
	xDiff = pointPtr[0] - x1;
	tmp = x2 - pointPtr[0];
	if (tmp < xDiff) {
	    xDiff = tmp;
	}
	yDiff = pointPtr[1] - y1;
	tmp = y2 - pointPtr[1];
	if (tmp < yDiff) {
	    yDiff = tmp;
	}
	if (yDiff < xDiff) {
	    xDiff = yDiff;
	}
	xDiff -= width;
	if (xDiff < 0.0) {
	    return 0.0;
	}
	return xDiff;
    }

    /*
     * Point is outside rectangle.
     */

    if (pointPtr[0] < x1) {
	xDiff = x1 - pointPtr[0];
    } else if (pointPtr[0] > x2)  {
	xDiff = pointPtr[0] - x2;
    } else {
	xDiff = 0;
    }

    if (pointPtr[1] < y1) {
	yDiff = y1 - pointPtr[1];
    } else if (pointPtr[1] > y2)  {
	yDiff = pointPtr[1] - y2;
    } else {
	yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * OvalToPoint --
 *
 *	Computes the distance from a given point to a given
 *	oval, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are coordPtr[0] and coordPtr[1] is inside the oval.  If the
 *	point isn't inside the oval then the return value is the
 *	distance from the point to the oval.  If itemPtr is filled,
 *	then anywhere in the interior is considered "inside"; if
 *	itemPtr isn't filled, then "inside" means only the area
 *	occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
OvalToPoint(canvas, itemPtr, pointPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    RectOvalItem *ovalPtr = (RectOvalItem *) itemPtr;
    double width;
    int filled;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = (double) ovalPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (ovalPtr->outline.activeWidth>width) {
	    width = (double) ovalPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (ovalPtr->outline.disabledWidth>0) {
	    width = (double) ovalPtr->outline.disabledWidth;
	}
    }


    filled = ovalPtr->fillGC != None;
    if (ovalPtr->outline.gc == None) {
	width = 0.0;
	filled = 1;
    }
    return TkOvalToPoint(ovalPtr->bbox, width, filled, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangle.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area
 *	given by rectPtr, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
RectToArea(canvas, itemPtr, areaPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against rectangle. */
    double *areaPtr;		/* Pointer to array of four coordinates
				 * (x1, y1, x2, y2) describing rectangular
				 * area.  */
{
    RectOvalItem *rectPtr = (RectOvalItem *) itemPtr;
    double halfWidth;
    double width;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = rectPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectPtr->outline.activeWidth>width) {
	    width = rectPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectPtr->outline.disabledWidth>0) {
	    width = rectPtr->outline.disabledWidth;
	}
    }

    halfWidth = width/2.0;
    if (rectPtr->outline.gc == None) {
	halfWidth = 0.0;
    }

    if ((areaPtr[2] <= (rectPtr->bbox[0] - halfWidth))
	    || (areaPtr[0] >= (rectPtr->bbox[2] + halfWidth))
	    || (areaPtr[3] <= (rectPtr->bbox[1] - halfWidth))
	    || (areaPtr[1] >= (rectPtr->bbox[3] + halfWidth))) {
	return -1;
    }
    if ((rectPtr->fillGC == None) && (rectPtr->outline.gc != None)
	    && (areaPtr[0] >= (rectPtr->bbox[0] + halfWidth))
	    && (areaPtr[1] >= (rectPtr->bbox[1] + halfWidth))
	    && (areaPtr[2] <= (rectPtr->bbox[2] - halfWidth))
	    && (areaPtr[3] <= (rectPtr->bbox[3] - halfWidth))) {
	return -1;
    }
    if ((areaPtr[0] <= (rectPtr->bbox[0] - halfWidth))
	    && (areaPtr[1] <= (rectPtr->bbox[1] - halfWidth))
	    && (areaPtr[2] >= (rectPtr->bbox[2] + halfWidth))
	    && (areaPtr[3] >= (rectPtr->bbox[3] + halfWidth))) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * OvalToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangular area.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area
 *	given by rectPtr, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
OvalToArea(canvas, itemPtr, areaPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against oval. */
    double *areaPtr;		/* Pointer to array of four coordinates
				 * (x1, y1, x2, y2) describing rectangular
				 * area.  */
{
    RectOvalItem *ovalPtr = (RectOvalItem *) itemPtr;
    double oval[4], halfWidth;
    int result;
    double width;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = ovalPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (ovalPtr->outline.activeWidth>width) {
	    width = ovalPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (ovalPtr->outline.disabledWidth>0) {
	    width = ovalPtr->outline.disabledWidth;
	}
    }

    /*
     * Expand the oval to include the width of the outline, if any.
     */

    halfWidth = width/2.0;
    if (ovalPtr->outline.gc == None) {
	halfWidth = 0.0;
    }
    oval[0] = ovalPtr->bbox[0] - halfWidth;
    oval[1] = ovalPtr->bbox[1] - halfWidth;
    oval[2] = ovalPtr->bbox[2] + halfWidth;
    oval[3] = ovalPtr->bbox[3] + halfWidth;

    result = TkOvalToArea(oval, areaPtr);

    /*
     * If the rectangle appears to overlap the oval and the oval
     * isn't filled, do one more check to see if perhaps all four
     * of the rectangle's corners are totally inside the oval's
     * unfilled center, in which case we should return "outside".
     */

    if ((result == 0) && (ovalPtr->outline.gc != None)
	    && (ovalPtr->fillGC == None)) {
	double centerX, centerY, height;
	double xDelta1, yDelta1, xDelta2, yDelta2;

	centerX = (ovalPtr->bbox[0] + ovalPtr->bbox[2])/2.0;
	centerY = (ovalPtr->bbox[1] + ovalPtr->bbox[3])/2.0;
	width = (ovalPtr->bbox[2] - ovalPtr->bbox[0])/2.0 - halfWidth;
	height = (ovalPtr->bbox[3] - ovalPtr->bbox[1])/2.0 - halfWidth;
	xDelta1 = (areaPtr[0] - centerX)/width;
	xDelta1 *= xDelta1;
	yDelta1 = (areaPtr[1] - centerY)/height;
	yDelta1 *= yDelta1;
	xDelta2 = (areaPtr[2] - centerX)/width;
	xDelta2 *= xDelta2;
	yDelta2 = (areaPtr[3] - centerY)/height;
	yDelta2 *= yDelta2;
	if (((xDelta1 + yDelta1) < 1.0)
		&& ((xDelta1 + yDelta2) < 1.0)
		&& ((xDelta2 + yDelta1) < 1.0)
		&& ((xDelta2 + yDelta2) < 1.0)) {
	    return -1;
	}
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleRectOval --
 *
 *	This procedure is invoked to rescale a rectangle or oval
 *	item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The rectangle or oval referred to by itemPtr is rescaled
 *	so that the following transformation is applied to all
 *	point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleRectOval(canvas, itemPtr, originX, originY, scaleX, scaleY)
    Tk_Canvas canvas;			/* Canvas containing rectangle. */
    Tk_Item *itemPtr;			/* Rectangle to be scaled. */
    double originX, originY;		/* Origin about which to scale rect. */
    double scaleX;			/* Amount to scale in X direction. */
    double scaleY;			/* Amount to scale in Y direction. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    rectOvalPtr->bbox[0] = originX + scaleX*(rectOvalPtr->bbox[0] - originX);
    rectOvalPtr->bbox[1] = originY + scaleY*(rectOvalPtr->bbox[1] - originY);
    rectOvalPtr->bbox[2] = originX + scaleX*(rectOvalPtr->bbox[2] - originX);
    rectOvalPtr->bbox[3] = originY + scaleY*(rectOvalPtr->bbox[3] - originY);
    ComputeRectOvalBbox(canvas, rectOvalPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateRectOval --
 *
 *	This procedure is called to move a rectangle or oval by a
 *	given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the rectangle or oval is offset by
 *	(xDelta, yDelta), and the bounding box is updated in the
 *	generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateRectOval(canvas, itemPtr, deltaX, deltaY)
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item that is being moved. */
    double deltaX, deltaY;		/* Amount by which item is to be
					 * moved. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    rectOvalPtr->bbox[0] += deltaX;
    rectOvalPtr->bbox[1] += deltaY;
    rectOvalPtr->bbox[2] += deltaX;
    rectOvalPtr->bbox[3] += deltaY;
    ComputeRectOvalBbox(canvas, rectOvalPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalToPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	rectangle and oval items.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in the interp's result, replacing whatever used to be there.
 *	If no error occurs, then Postscript for the rectangle is
 *	appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalToPostscript(interp, canvas, itemPtr, prepass)
    Tcl_Interp *interp;			/* Interpreter for error reporting. */
    Tk_Canvas canvas;			/* Information about overall canvas. */
    Tk_Item *itemPtr;			/* Item for which Postscript is
					 * wanted. */
    int prepass;			/* 1 means this is a prepass to
					 * collect font information;  0 means
					 * final Postscript is being created. */
{
    char pathCmd[500];
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    double y1, y2;
    XColor *color;
    XColor *fillColor;
    Pixmap fillStipple;
    Tk_State state = itemPtr->state;

    y1 = Tk_CanvasPsY(canvas, rectOvalPtr->bbox[1]);
    y2 = Tk_CanvasPsY(canvas, rectOvalPtr->bbox[3]);

    /*
     * Generate a string that creates a path for the rectangle or oval.
     * This is the only part of the procedure's code that is type-
     * specific.
     */


    if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	sprintf(pathCmd, "%.15g %.15g moveto %.15g 0 rlineto 0 %.15g rlineto %.15g 0 rlineto closepath\n",
		rectOvalPtr->bbox[0], y1,
		rectOvalPtr->bbox[2]-rectOvalPtr->bbox[0], y2-y1,
		rectOvalPtr->bbox[0]-rectOvalPtr->bbox[2]);
    } else {
	sprintf(pathCmd, "matrix currentmatrix\n%.15g %.15g translate %.15g %.15g scale 1 0 moveto 0 0 1 0 360 arc\nsetmatrix\n",
		(rectOvalPtr->bbox[0] + rectOvalPtr->bbox[2])/2, (y1 + y2)/2,
		(rectOvalPtr->bbox[2] - rectOvalPtr->bbox[0])/2, (y1 - y2)/2);
    }

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    color = rectOvalPtr->outline.color;
    fillColor = rectOvalPtr->fillColor;
    fillStipple = rectOvalPtr->fillStipple;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectOvalPtr->outline.activeColor!=NULL) {
	    color = rectOvalPtr->outline.activeColor;
	}
	if (rectOvalPtr->activeFillColor!=NULL) {
	    fillColor = rectOvalPtr->activeFillColor;
	}
	if (rectOvalPtr->activeFillStipple!=None) {
	    fillStipple = rectOvalPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectOvalPtr->outline.disabledColor!=NULL) {
	    color = rectOvalPtr->outline.disabledColor;
	}
	if (rectOvalPtr->disabledFillColor!=NULL) {
	    fillColor = rectOvalPtr->disabledFillColor;
	}
	if (rectOvalPtr->disabledFillStipple!=None) {
	    fillStipple = rectOvalPtr->disabledFillStipple;
	}
    }

    /*
     * First draw the filled area of the rectangle.
     */

    if (fillColor != NULL) {
	Tcl_AppendResult(interp, pathCmd, (char *) NULL);
	if (Tk_CanvasPsColor(interp, canvas, fillColor)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if (fillStipple != None) {
	    Tcl_AppendResult(interp, "clip ", (char *) NULL);
	    if (Tk_CanvasPsStipple(interp, canvas, fillStipple)
		    != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (color != NULL) {
		Tcl_AppendResult(interp, "grestore gsave\n", (char *) NULL);
	    }
	} else {
	    Tcl_AppendResult(interp, "fill\n", (char *) NULL);
	}
    }

    /*
     * Now draw the outline, if there is one.
     */

    if (color != NULL) {
	Tcl_AppendResult(interp, pathCmd, "0 setlinejoin 2 setlinecap\n",
		(char *) NULL);
	if (Tk_CanvasPsOutline(canvas, itemPtr,
		&(rectOvalPtr->outline))!= TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}
