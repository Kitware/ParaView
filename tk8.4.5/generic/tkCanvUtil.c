/* 
 * tkCanvUtil.c --
 *
 *	This procedure contains a collection of utility procedures
 *	used by the implementations of various canvas item types.
 *
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkCanvas.h"
#include "tkPort.h"
#include <assert.h>


/*
 *----------------------------------------------------------------------
 *
 * Tk_CanvasTkwin --
 *
 *	Given a token for a canvas, this procedure returns the
 *	widget that represents the canvas.
 *
 * Results:
 *	The return value is a handle for the widget.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_CanvasTkwin(canvas)
    Tk_Canvas canvas;			/* Token for the canvas. */
{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    return canvasPtr->tkwin;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CanvasDrawableCoords --
 *
 *	Given an (x,y) coordinate pair within a canvas, this procedure
 *	returns the corresponding coordinates at which the point should
 *	be drawn in the drawable used for display.
 *
 * Results:
 *	There is no return value.  The values at *drawableXPtr and
 *	*drawableYPtr are filled in with the coordinates at which
 *	x and y should be drawn.  These coordinates are clipped
 *	to fit within a "short", since this is what X uses in
 *	most cases for drawing.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CanvasDrawableCoords(canvas, x, y, drawableXPtr, drawableYPtr)
    Tk_Canvas canvas;			/* Token for the canvas. */
    double x, y;			/* Coordinates in canvas space. */
    short *drawableXPtr, *drawableYPtr;	/* Screen coordinates are stored
					 * here. */
{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    double tmp;

    tmp = x - canvasPtr->drawableXOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*drawableXPtr = 32767;
    } else if (tmp < -32768) {
	*drawableXPtr = -32768;
    } else {
	*drawableXPtr = (short) tmp;
    }

    tmp = y  - canvasPtr->drawableYOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*drawableYPtr = 32767;
    } else if (tmp < -32768) {
	*drawableYPtr = -32768;
    } else {
	*drawableYPtr = (short) tmp;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CanvasWindowCoords --
 *
 *	Given an (x,y) coordinate pair within a canvas, this procedure
 *	returns the corresponding coordinates in the canvas's window.
 *
 * Results:
 *	There is no return value.  The values at *screenXPtr and
 *	*screenYPtr are filled in with the coordinates at which
 *	(x,y) appears in the canvas's window.  These coordinates
 *	are clipped to fit within a "short", since this is what X
 *	uses in most cases for drawing.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CanvasWindowCoords(canvas, x, y, screenXPtr, screenYPtr)
    Tk_Canvas canvas;			/* Token for the canvas. */
    double x, y;			/* Coordinates in canvas space. */
    short *screenXPtr, *screenYPtr;	/* Screen coordinates are stored
					 * here. */
{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    double tmp;

    tmp = x - canvasPtr->xOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*screenXPtr = 32767;
    } else if (tmp < -32768) {
	*screenXPtr = -32768;
    } else {
	*screenXPtr = (short) tmp;
    }

    tmp = y  - canvasPtr->yOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*screenYPtr = 32767;
    } else if (tmp < -32768) {
	*screenYPtr = -32768;
    } else {
	*screenYPtr = (short) tmp;
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CanvasGetCoord --
 *
 *	Given a string, returns a floating-point canvas coordinate
 *	corresponding to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result.  If
 *	TCL_OK is returned, then everything went well and the
 *	canvas coordinate is stored at *doublePtr;  otherwise
 *	TCL_ERROR is returned and an error message is left in
 *	the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_CanvasGetCoord(interp, canvas, string, doublePtr)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Canvas canvas;		/* Canvas to which coordinate applies. */
    CONST char *string;		/* Describes coordinate (any screen
				 * coordinate form may be used here). */
    double *doublePtr;		/* Place to store converted coordinate. */
{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    if (Tk_GetScreenMM(canvasPtr->interp, canvasPtr->tkwin, string,
	    doublePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *doublePtr *= canvasPtr->pixelsPerMM;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CanvasGetCoordFromObj --
 *
 *	Given a string, returns a floating-point canvas coordinate
 *	corresponding to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result.  If
 *	TCL_OK is returned, then everything went well and the
 *	canvas coordinate is stored at *doublePtr;  otherwise
 *	TCL_ERROR is returned and an error message is left in
 *	interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_CanvasGetCoordFromObj(interp, canvas, obj, doublePtr)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Canvas canvas;		/* Canvas to which coordinate applies. */
    Tcl_Obj *obj;		/* Describes coordinate (any screen
				 * coordinate form may be used here). */
    double *doublePtr;		/* Place to store converted coordinate. */
{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    if (Tk_GetMMFromObj(canvasPtr->interp, canvasPtr->tkwin, obj,
	    doublePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *doublePtr *= canvasPtr->pixelsPerMM;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CanvasSetStippleOrigin --
 *
 *	This procedure sets the stipple origin in a graphics context
 *	so that stipples drawn with the GC will line up with other
 *	stipples previously drawn in the canvas.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The graphics context is modified.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CanvasSetStippleOrigin(canvas, gc)
    Tk_Canvas canvas;		/* Token for a canvas. */
    GC gc;			/* Graphics context that is about to be
				 * used to draw a stippled pattern as
				 * part of redisplaying the canvas. */

{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;

    XSetTSOrigin(canvasPtr->display, gc, -canvasPtr->drawableXOrigin,
	    -canvasPtr->drawableYOrigin);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CanvasSetOffset--
 *
 *	This procedure sets the stipple offset in a graphics
 *	context so that stipples drawn with the GC will
 *	line up with other stipples with the same offset.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The graphics context is modified.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CanvasSetOffset(canvas, gc, offset)
    Tk_Canvas canvas;		/* Token for a canvas. */
    GC gc;			/* Graphics context that is about to be
				 * used to draw a stippled pattern as
				 * part of redisplaying the canvas. */
    Tk_TSOffset *offset;	/* offset (may be NULL pointer)*/
{
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    int flags = 0;
    int x = - canvasPtr->drawableXOrigin;
    int y = - canvasPtr->drawableYOrigin;

    if (offset != NULL) {
	flags = offset->flags;
	x += offset->xoffset;
	y += offset->yoffset;
    }
    if ((flags & TK_OFFSET_RELATIVE) && !(flags & TK_OFFSET_INDEX)) {
	Tk_SetTSOrigin(canvasPtr->tkwin, gc, x - canvasPtr->xOrigin,
		y - canvasPtr->yOrigin);
    } else {
	XSetTSOrigin(canvasPtr->display, gc, x, y);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CanvasGetTextInfo --
 *
 *	This procedure returns a pointer to a structure containing
 *	information about the selection and insertion cursor for
 *	a canvas widget.  Items such as text items save the pointer
 *	and use it to share access to the information with the generic
 *	canvas code.
 *
 * Results:
 *	The return value is a pointer to the structure holding text
 *	information for the canvas.  Most of the fields should not
 *	be modified outside the generic canvas code;  see the user
 *	documentation for details.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_CanvasTextInfo *
Tk_CanvasGetTextInfo(canvas)
    Tk_Canvas canvas;			/* Token for the canvas widget. */
{
    return &((TkCanvas *) canvas)->textInfo;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CanvasTagsParseProc --
 *
 *	This procedure is invoked during option processing to handle
 *	"-tags" options for canvas items.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The tags for a given item get replaced by those indicated
 *	in the value argument.
 *
 *--------------------------------------------------------------
 */

int
Tk_CanvasTagsParseProc(clientData, interp, tkwin, value, widgRec, offset)
    ClientData clientData;		/* Not used.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    CONST char *value;			/* Value of option (list of tag
					 * names). */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item (ignored). */
{
    register Tk_Item *itemPtr = (Tk_Item *) widgRec;
    int argc, i;
    CONST char **argv;
    Tk_Uid *newPtr;

    /*
     * Break the value up into the individual tag names.
     */

    if (Tcl_SplitList(interp, value, &argc, &argv) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Make sure that there's enough space in the item to hold the
     * tag names.
     */

    if (itemPtr->tagSpace < argc) {
	newPtr = (Tk_Uid *) ckalloc((unsigned) (argc * sizeof(Tk_Uid)));
	for (i = itemPtr->numTags-1; i >= 0; i--) {
	    newPtr[i] = itemPtr->tagPtr[i];
	}
	if (itemPtr->tagPtr != itemPtr->staticTagSpace) {
	    ckfree((char *) itemPtr->tagPtr);
	}
	itemPtr->tagPtr = newPtr;
	itemPtr->tagSpace = argc;
    }
    itemPtr->numTags = argc;
    for (i = 0; i < argc; i++) {
	itemPtr->tagPtr[i] = Tk_GetUid(argv[i]);
    }
    ckfree((char *) argv);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CanvasTagsPrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-tags" configuration
 *	option for canvas items.
 *
 * Results:
 *	The return value is a string describing all the tags for
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

char *
Tk_CanvasTagsPrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Ignored. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    register Tk_Item *itemPtr = (Tk_Item *) widgRec;

    if (itemPtr->numTags == 0) {
	*freeProcPtr = (Tcl_FreeProc *) NULL;
	return "";
    }
    if (itemPtr->numTags == 1) {
	*freeProcPtr = (Tcl_FreeProc *) NULL;
	return (char *) itemPtr->tagPtr[0];
    }
    *freeProcPtr = TCL_DYNAMIC;
    return Tcl_Merge(itemPtr->numTags, (CONST char **) itemPtr->tagPtr);
}


static int	DashConvert _ANSI_ARGS_((char *l, CONST char *p,
			int n, double width));
#define ABS(a) ((a>=0)?(a):(-(a)))

/*
 *--------------------------------------------------------------
 *
 * TkCanvasDashParseProc --
 *
 *	This procedure is invoked during option processing to handle
 *	"-dash", "-activedash" and "-disableddash" options for canvas
 *	objects.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The dash list for a given canvas object gets replaced by
 *	those indicated in the value argument.
 *
 *--------------------------------------------------------------
 */

int
TkCanvasDashParseProc(clientData, interp, tkwin, value, widgRec, offset)
    ClientData clientData;		/* Not used.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    CONST char *value;			/* Value of option. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
{
    return Tk_GetDash(interp, value, (Tk_Dash *)(widgRec+offset));
}

/*
 *--------------------------------------------------------------
 *
 * TkCanvasDashPrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-dash", "-activedash"
 *	and "-disableddash" configuration options for canvas items.
 *
 * Results:
 *	The return value is a string describing all the dash list for
 *	the item referred to by "widgRec"and "offset".  In addition,
 *	*freeProcPtr is filled in with the address of a procedure to
 *	call to free the result string when it's no longer needed (or
 *	NULL to indicate that the string doesn't need to be freed).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

char *
TkCanvasDashPrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset in record for item. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    Tk_Dash *dash = (Tk_Dash *) (widgRec+offset);
    char *buffer;
    char *p;
    int i = dash->number;

    if (i < 0) {
	i = -i;
	*freeProcPtr = TCL_DYNAMIC;
	buffer = (char *) ckalloc((unsigned int) (i+1));
	p = (i > sizeof(char *)) ? dash->pattern.pt : dash->pattern.array;
	memcpy(buffer, p, (unsigned int) i);
	buffer[i] = 0;
	return buffer;
    } else if (!i) {
	*freeProcPtr = (Tcl_FreeProc *) NULL;
	return "";
    }
    buffer = (char *)ckalloc((unsigned int) (4*i));
    *freeProcPtr = TCL_DYNAMIC;

    p = (i > sizeof(char *)) ? dash->pattern.pt : dash->pattern.array;
    sprintf(buffer, "%d", *p++ & 0xff);
    while(--i) {
	sprintf(buffer+strlen(buffer), " %d", *p++ & 0xff);
    }
    return buffer;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateSmoothMethod --
 *
 *	This procedure is invoked to add additional values
 *	for the "-smooth" option to the list.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	In the future "-smooth <name>" will be accepted as
 *	smooth method for the line and polygon.
 *
 *--------------------------------------------------------------
 */

Tk_SmoothMethod tkBezierSmoothMethod = {
    "bezier",
    TkMakeBezierCurve,
    (void (*) _ANSI_ARGS_((Tcl_Interp *interp, Tk_Canvas canvas,
	    double *coordPtr, int numPoints, int numSteps)))
		TkMakeBezierPostscript,
};

static void SmoothMethodCleanupProc _ANSI_ARGS_((ClientData clientData,
		Tcl_Interp *interp));

typedef struct SmoothAssocData {
    struct SmoothAssocData *nextPtr;	/* pointer to next SmoothAssocData */
    Tk_SmoothMethod smooth;		/* name and functions associated with this
					 * option */
} SmoothAssocData;

void
Tk_CreateSmoothMethod(interp, smooth)
    Tcl_Interp *interp;
    Tk_SmoothMethod *smooth;
{
    SmoothAssocData *methods, *typePtr2, *prevPtr, *ptr;
    methods = (SmoothAssocData *) Tcl_GetAssocData(interp, "smoothMethod",
	    (Tcl_InterpDeleteProc **) NULL);

    /*
     * If there's already a smooth method with the given name, remove it.
     */

    for (typePtr2 = methods, prevPtr = NULL; typePtr2 != NULL;
	    prevPtr = typePtr2, typePtr2 = typePtr2->nextPtr) {
	if (!strcmp(typePtr2->smooth.name, smooth->name)) {
	    if (prevPtr == NULL) {
		methods = typePtr2->nextPtr;
	    } else {
		prevPtr->nextPtr = typePtr2->nextPtr;
	    }
	    ckfree((char *) typePtr2);
	    break;
	}
    }
    ptr = (SmoothAssocData *) ckalloc(sizeof(SmoothAssocData));
    ptr->smooth.name = smooth->name;
    ptr->smooth.coordProc = smooth->coordProc;
    ptr->smooth.postscriptProc = smooth->postscriptProc;
    ptr->nextPtr = methods;
    Tcl_SetAssocData(interp, "smoothMethod", SmoothMethodCleanupProc,
		(ClientData) ptr);
}
/*
 *----------------------------------------------------------------------
 *
 * SmoothMethodCleanupProc --
 *
 *	This procedure is invoked whenever an interpreter is deleted
 *	to cleanup the smooth methods.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Smooth methods are removed.
 *
 *----------------------------------------------------------------------
 */

static void
SmoothMethodCleanupProc(clientData, interp)
    ClientData clientData;	/* Points to "smoothMethod" AssocData
				 * for the interpreter. */
    Tcl_Interp *interp;		/* Interpreter that is being deleted. */
{
    SmoothAssocData *ptr, *methods = (SmoothAssocData *) clientData;

    while (methods != NULL) {
	methods = (ptr = methods)->nextPtr;
	ckfree((char *) ptr);
    }
}
/*
 *--------------------------------------------------------------
 *
 * TkSmoothParseProc --
 *
 *	This procedure is invoked during option processing to handle
 *	the "-smooth" option.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The smooth option for a given item gets replaced by the value
 *	indicated in the value argument.
 *
 *--------------------------------------------------------------
 */

int
TkSmoothParseProc(clientData, interp, tkwin, value, widgRec, offset)
    ClientData clientData;		/* some flags.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    CONST char *value;			/* Value of option. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
{
    register Tk_SmoothMethod **smoothPtr =
	(Tk_SmoothMethod **) (widgRec + offset);
    Tk_SmoothMethod *smooth = NULL;
    int b;
    size_t length;
    SmoothAssocData *methods;

    if (value == NULL || *value == 0) {
	*smoothPtr = (Tk_SmoothMethod *) NULL;
	return TCL_OK;
    }
    length = strlen(value);
    methods = (SmoothAssocData *) Tcl_GetAssocData(interp, "smoothMethod",
	    (Tcl_InterpDeleteProc **) NULL);
    while (methods != (SmoothAssocData *) NULL) {
	if (strncmp(value, methods->smooth.name, length) == 0) {
	    if (smooth != (Tk_SmoothMethod *) NULL) {
		Tcl_AppendResult(interp, "ambigeous smooth method \"", value,
			"\"", (char *) NULL);
		return TCL_ERROR;
	    }
	    smooth = &methods->smooth;
	}
	methods = methods->nextPtr;
    }
    if (smooth) {
	*smoothPtr = smooth;
	return TCL_OK;
    } else if (strncmp(value, tkBezierSmoothMethod.name, length) == 0) {
	/*
	 * We need to do handle the built-in bezier method.
	 */
	*smoothPtr = &tkBezierSmoothMethod;
	return TCL_OK;
    }


    if (Tcl_GetBoolean(interp, (char *) value, &b) != TCL_OK) {
	return TCL_ERROR;
    }
    *smoothPtr = b ? &tkBezierSmoothMethod : (Tk_SmoothMethod*) NULL;
    return TCL_OK;
}
/*
 *--------------------------------------------------------------
 *
 * TkSmoothPrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-smooth"
 *	configuration option.
 *
 * Results:
 *	The return value is a string describing the smooth option for
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

char *
TkSmoothPrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    register Tk_SmoothMethod **smoothPtr = (Tk_SmoothMethod **) (widgRec + offset);

    return (*smoothPtr) ? (*smoothPtr)->name : "0";
}
/*
 *--------------------------------------------------------------
 *
 * Tk_GetDash
 *
 *	This procedure is used to parse a string, assuming
 *	it is dash information.
 *
 * Results:
 *	The return value is a standard Tcl result:  TCL_OK means
 *	that the dash information was parsed ok, and
 *	TCL_ERROR means it couldn't be parsed.
 *
 * Side effects:
 *	Dash information in the dash structure is updated.
 *
 *--------------------------------------------------------------
 */

int
Tk_GetDash(interp, value, dash)
    Tcl_Interp *interp;		/* Used for error reporting. */
    CONST char *value;		/* Textual specification of dash list. */
    Tk_Dash *dash;		/* Pointer to record in which to
				 * store dash information. */
{
    int argc, i;
    CONST char **largv, **argv = NULL;
    char *pt;

    if ((value==(char *) NULL) || (*value==0) ) {
	dash->number = 0;
	return TCL_OK;
    }
    if ((*value == '.') || (*value == ',') ||
	    (*value == '-') || (*value == '_')) {
	i = DashConvert((char *) NULL, value, -1, 0.0);
	if (i>0) {
	    i = strlen(value);
	} else {
	    goto badDashList;
	}
	if (i > sizeof(char *)) {
	    dash->pattern.pt = pt = (char *) ckalloc(strlen(value));
	} else {
	    pt = dash->pattern.array;
	}
	memcpy(pt,value, (unsigned int) i);
	dash->number = -i;
	return TCL_OK;
    }
    if (Tcl_SplitList(interp, (char *) value, &argc, &argv) != TCL_OK) {
	Tcl_ResetResult(interp);
    badDashList:
	Tcl_AppendResult(interp, "bad dash list \"", value,
		"\": must be a list of integers or a format like \"-..\"",
		(char *) NULL);
    syntaxError:
	if (argv != NULL) {
	    ckfree((char *) argv);
	}
	if (ABS(dash->number) > sizeof(char *))
	    ckfree((char *) dash->pattern.pt);
	dash->number = 0;
	return TCL_ERROR;
    }

    if (ABS(dash->number) > sizeof(char *)) {
	ckfree((char *) dash->pattern.pt);
    }
    if (argc > sizeof(char *)) {
	dash->pattern.pt = pt = (char *) ckalloc((unsigned int) argc);
    } else {
	pt = dash->pattern.array;
    }
    dash->number = argc;

    largv = argv;
    while(argc>0) {
	if (Tcl_GetInt(interp, *largv, &i) != TCL_OK ||
	    i < 1 || i>255) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "expected integer in the range 1..255 but got \"",
			 *largv, "\"", (char *) NULL);
	    goto syntaxError;
	}
	*pt++ = i;
	argc--; largv++; 
    }
  
    if (argv != NULL) {
	ckfree((char *) argv);
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateOutline
 *
 *	This procedure initializes the Tk_Outline structure
 *	with default values.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

void Tk_CreateOutline(outline)
    Tk_Outline *outline;
{
    outline->gc = None;
    outline->width = 1.0;
    outline->activeWidth = 0.0;
    outline->disabledWidth = 0.0;
    outline->offset = 0;
    outline->dash.number = 0;
    outline->activeDash.number = 0;
    outline->disabledDash.number = 0;
    outline->tsoffset.flags = 0;
    outline->tsoffset.xoffset = 0;
    outline->tsoffset.yoffset = 0;
    outline->color = NULL;
    outline->activeColor = NULL;
    outline->disabledColor = NULL;
    outline->stipple = None;
    outline->activeStipple = None;
    outline->disabledStipple = None;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DeleteOutline
 *
 *	This procedure frees all memory that might be
 *	allocated and referenced in the Tk_Outline structure.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

void Tk_DeleteOutline(display, outline)
    Display *display;			/* Display containing window */
    Tk_Outline *outline;
{
    if (outline->gc != None) {
	Tk_FreeGC(display, outline->gc);
    }
    if (ABS(outline->dash.number) > sizeof(char *)) {
	ckfree((char *) outline->dash.pattern.pt);
    }
    if (ABS(outline->activeDash.number) > sizeof(char *)) {
	ckfree((char *) outline->activeDash.pattern.pt);
    }
    if (ABS(outline->disabledDash.number) > sizeof(char *)) {
	ckfree((char *) outline->disabledDash.pattern.pt);
    }
    if (outline->color != NULL) {
	Tk_FreeColor(outline->color);
    }
    if (outline->activeColor != NULL) {
	Tk_FreeColor(outline->activeColor);
    }
    if (outline->disabledColor != NULL) {
	Tk_FreeColor(outline->disabledColor);
    }
    if (outline->stipple != None) {
	Tk_FreeBitmap(display, outline->stipple);
    }
    if (outline->activeStipple != None) {
	Tk_FreeBitmap(display, outline->activeStipple);
    }
    if (outline->disabledStipple != None) {
	Tk_FreeBitmap(display, outline->disabledStipple);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_ConfigOutlineGC
 *
 *	This procedure should be called in the canvas object
 *	during the configure command. The graphics context
 *	description in gcValues is updated according to the
 *	information in the dash structure, as far as possible.
 *
 * Results:
 *	The return-value is a mask, indicating which
 *	elements of gcValues have been updated.
 *	0 means there is no outline.
 *
 * Side effects:
 *	GC information in gcValues is updated.
 *
 *--------------------------------------------------------------
 */

int Tk_ConfigOutlineGC(gcValues, canvas, item, outline)
    XGCValues *gcValues;
    Tk_Canvas canvas;
    Tk_Item *item;
    Tk_Outline *outline;
{
    int mask = 0;
    double width;
    Tk_Dash *dash;
    XColor *color;
    Pixmap stipple;
    Tk_State state = item->state;

    if (outline->width < 0.0) {
	outline->width = 0.0;
    }
    if (outline->activeWidth < 0.0) {
	outline->activeWidth = 0.0;
    }
    if (outline->disabledWidth < 0) {
	outline->disabledWidth = 0.0;
    }
    if (state==TK_STATE_HIDDEN) {
	return 0;
    }

    width = outline->width;
    if (width < 1.0) {
	width = 1.0;
    }
    dash = &(outline->dash);
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    if (((TkCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth>width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDash.number != 0) {
	    dash = &(outline->activeDash);
	}
	if (outline->activeColor!=NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple!=None) {
	    stipple = outline->activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (outline->disabledWidth>0) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDash.number != 0) {
	    dash = &(outline->disabledDash);
	}
	if (outline->disabledColor!=NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple!=None) {
	    stipple = outline->disabledStipple;
	}
    }

    if (color==NULL) {
	return 0;
    }

    gcValues->line_width = (int) (width + 0.5);
    if (color != NULL) {
	gcValues->foreground = color->pixel;
	mask = GCForeground|GCLineWidth;
	if (stipple != None) {
	    gcValues->stipple = stipple;
	    gcValues->fill_style = FillStippled;
	    mask |= GCStipple|GCFillStyle;
	}
    }
    if (mask && (dash->number != 0)) {
	gcValues->line_style = LineOnOffDash;
	gcValues->dash_offset = outline->offset;
	if (dash->number >= 2) {
	    gcValues->dashes = 4;
	} else if (dash->number > 0) {
	    gcValues->dashes = dash->pattern.array[0];
	} else {
	    gcValues->dashes = (char) (4 * width);
	}
	mask |= GCLineStyle|GCDashList|GCDashOffset;
    }
    return mask;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_ChangeOutlineGC
 *
 *	Updates the GC to represent the full information of
 *	the dash structure. Partly this is already done in
 *	Tk_ConfigOutlineGC().
 *	This function should be called just before drawing
 *	the dashed item.
 *
 * Results:
 *	1 if there is a stipple pattern.
 *	0 otherwise.
 *
 * Side effects:
 *	GC is updated.
 *
 *--------------------------------------------------------------
 */

int
Tk_ChangeOutlineGC(canvas, item, outline)
    Tk_Canvas canvas;
    Tk_Item *item;
    Tk_Outline *outline;
{
    CONST char *p;
    double width;
    Tk_Dash *dash;
    XColor *color;
    Pixmap stipple;
    Tk_State state = item->state;

    width = outline->width;
    if (width < 1.0) {
	width = 1.0;
    }
    dash = &(outline->dash);
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    if (((TkCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth > width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDash.number != 0) {
	    dash = &(outline->activeDash);
	}
	if (outline->activeColor != NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple != None) {
	    stipple = outline->activeStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (outline->disabledWidth > width) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDash.number != 0) {
	    dash = &(outline->disabledDash);
	}
	if (outline->disabledColor != NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple != None) {
	    stipple = outline->disabledStipple;
	}
    }
    if (color==NULL) {
	return 0;
    }

    if ((dash->number<-1) || ((dash->number == -1) && (dash->pattern.array[1]!=','))) {
	char *q;
	int i = -dash->number;

        p = (i > sizeof(char *)) ? dash->pattern.pt : dash->pattern.array;
	q = (char *) ckalloc(2*(unsigned int)i);
	i = DashConvert(q, p, i, width);
	XSetDashes(((TkCanvas *)canvas)->display, outline->gc, outline->offset, q, i);
	ckfree(q);
    } else if ( dash->number>2 || (dash->number==2 &&
		(dash->pattern.array[0]!=dash->pattern.array[1]))) {
        p = (char *) (dash->number > sizeof(char *)) ? dash->pattern.pt : dash->pattern.array;
	XSetDashes(((TkCanvas *)canvas)->display, outline->gc, outline->offset, p, dash->number);
    }
    if (stipple!=None) {
	int w=0; int h=0;
	Tk_TSOffset *tsoffset = &outline->tsoffset;
	int flags = tsoffset->flags;
	if (!(flags & TK_OFFSET_INDEX) && (flags & (TK_OFFSET_CENTER|TK_OFFSET_MIDDLE))) {
	    Tk_SizeOfBitmap(((TkCanvas *)canvas)->display, stipple, &w, &h);
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
	Tk_CanvasSetOffset(canvas, outline->gc, tsoffset);
	tsoffset->xoffset += w;
	tsoffset->yoffset += h;
	return 1;
    }
    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * Tk_ResetOutlineGC
 *
 *	Restores the GC to the situation before 
 *	Tk_ChangeDashGC() was called.
 *	This function should be called just after the dashed
 *	item is drawn, because the GC is supposed to be
 *	read-only.
 *
 * Results:
 *	1 if there is a stipple pattern.
 *	0 otherwise.
 *
 * Side effects:
 *	GC is updated.
 *
 *--------------------------------------------------------------
 */
int
Tk_ResetOutlineGC(canvas, item, outline)
    Tk_Canvas canvas;
    Tk_Item *item;
    Tk_Outline *outline;
{
    char dashList;
    double width;
    Tk_Dash *dash;
    XColor *color;
    Pixmap stipple;
    Tk_State state = item->state;

    width = outline->width;
    if (width < 1.0) {
	width = 1.0;
    }
    dash = &(outline->dash);
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    if (((TkCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth>width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDash.number != 0) {
	    dash = &(outline->activeDash);
	}
	if (outline->activeColor!=NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple!=None) {
	    stipple = outline->activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (outline->disabledWidth>width) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDash.number != 0) {
	    dash = &(outline->disabledDash);
	}
	if (outline->disabledColor!=NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple!=None) {
	    stipple = outline->disabledStipple;
	}
    }
    if (color==NULL) {
	return 0;
    }

    if ((dash->number > 2) || (dash->number < -1) || (dash->number==2 &&
		(dash->pattern.array[0] != dash->pattern.array[1])) ||
		((dash->number == -1) && (dash->pattern.array[1] != ','))) {
	if (dash->number < 0) {
	    dashList = (int) (4 * width + 0.5);
	} else if (dash->number<3) {
	    dashList = dash->pattern.array[0];
	} else {
	    dashList = 4;
	}
	XSetDashes(((TkCanvas *)canvas)->display, outline->gc,
		outline->offset, &dashList , 1);
    }
    if (stipple != None) {
	XSetTSOrigin(((TkCanvas *)canvas)->display, outline->gc, 0, 0);
	return 1;
    }
    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * Tk_CanvasPsOutline
 *
 *	Creates the postscript command for the correct
 *	Outline-information (width, dash, color and stipple).
 *
 * Results:
 *	TCL_OK if succeeded, otherwise TCL_ERROR.
 *
 * Side effects:
 *	canvas->interp->result contains the postscript string,
 *	or an error message if the result was TCL_ERROR.
 *
 *--------------------------------------------------------------
 */
int
Tk_CanvasPsOutline(canvas, item, outline)
    Tk_Canvas canvas;
    Tk_Item *item;
    Tk_Outline *outline;
{
    char string[41];
    char pattern[11];
    int i;
    char *ptr;
    char *str = string;
    char *lptr = pattern;
    Tcl_Interp *interp = ((TkCanvas *)canvas)->interp;
    double width;
    Tk_Dash *dash;
    XColor *color;
    Pixmap stipple;
    Tk_State state = item->state;

    width = outline->width;
    dash = &(outline->dash);
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    if (((TkCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth > width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDash.number > 0) {
	    dash = &(outline->activeDash);
	}
	if (outline->activeColor != NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple != None) {
	    stipple = outline->activeStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (outline->disabledWidth > 0) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDash.number > 0) {
	    dash = &(outline->disabledDash);
	}
	if (outline->disabledColor != NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple != None) {
	    stipple = outline->disabledStipple;
	}
    }
    sprintf(string, "%.15g setlinewidth\n", width);
    Tcl_AppendResult(interp, string, (char *) NULL);

    if (dash->number > 10) {
	str = (char *)ckalloc((unsigned int) (1 + 4*dash->number));
    } else if (dash->number < -5) {
	str = (char *)ckalloc((unsigned int) (1 - 8*dash->number));
	lptr = (char *)ckalloc((unsigned int) (1 - 2*dash->number));
    }
    ptr = (char *) ((ABS(dash->number) > sizeof(char *)) ) ?
	dash->pattern.pt : dash->pattern.array;
    if (dash->number > 0) {
	char *ptr0 = ptr;
	sprintf(str, "[%d", *ptr++ & 0xff);
	i = dash->number-1;
	while (i--) {
	    sprintf(str+strlen(str), " %d", *ptr++ & 0xff);
	}
	Tcl_AppendResult(interp, str, (char *)NULL);
	if (dash->number&1) {
	    Tcl_AppendResult(interp, " ", str+1, (char *)NULL);
	}
	sprintf(str, "] %d setdash\n", outline->offset);
	Tcl_AppendResult(interp, str, (char *)NULL);
	ptr = ptr0;
    } else if (dash->number < 0) {
	if ((i = DashConvert(lptr, ptr, -dash->number, width)) != 0) {
	    char *lptr0 = lptr;
	    sprintf(str, "[%d", *lptr++ & 0xff);
	    while (--i) {
		sprintf(str+strlen(str), " %d", *lptr++ & 0xff);
	    }
	    Tcl_AppendResult(interp, str, (char *)NULL);
	    sprintf(str, "] %d setdash\n", outline->offset);
	    Tcl_AppendResult(interp, str, (char *)NULL);
	    lptr = lptr0;
	} else {
	    Tcl_AppendResult(interp, "[] 0 setdash\n", (char *)NULL);
	}
    } else {
	Tcl_AppendResult(interp, "[] 0 setdash\n", (char *)NULL);
    }
    if (str != string) {
	ckfree(str);
    }
    if (lptr != pattern) {
	ckfree(lptr);
    }
    if (Tk_CanvasPsColor(interp, canvas, color) != TCL_OK) {
	return TCL_ERROR;
    }
    if (stipple != None) {
	Tcl_AppendResult(interp, "StrokeClip ", (char *) NULL);
	if (Tk_CanvasPsStipple(interp, canvas, stipple) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "stroke\n", (char *) NULL);
    }

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * DashConvert
 *
 *	Converts a character-like dash-list (e.g. "-..")
 *	into an X11-style. l must point to a string that
 *	holds room to at least 2*n characters. if
 *	l == NULL, this function can be used for
 *	syntax checking only.
 *
 * Results:
 *	The length of the resulting X11 compatible
 *	dash-list. -1 if failed.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

static int
DashConvert (l, p, n, width)
    char *l;
    CONST char *p;
    int n;
    double width;
{
    int result = 0;
    int size, intWidth;

    if (n<0) {
	n = strlen(p);
    }
    intWidth = (int) (width + 0.5);
    if (intWidth < 1) {
	intWidth = 1;
    }
    while (n-- && *p) {
	switch (*p++) {
	    case ' ':
		if (result) {
		    if (l) {
			l[-1] += intWidth + 1;
		    }
		    continue;
		} else {
		    return 0;
		}
		break;
	    case '_':
		size = 8;
		break;
	    case '-':
		size = 6;
		break;
	    case ',':
		size = 4;
		break;
	    case '.':
		size = 2;
		break;
	    default:
		return -1;
	}
	if (l) {
	    *l++ = size * intWidth;
	    *l++ = 4 * intWidth;
	}
	result += 2;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * translateAndAppendCoords --
 *
 *	This is a helper routine for TkCanvTranslatePath() below.
 *
 *	Given an (x,y) coordinate pair within a canvas, this procedure
 *	computes the corresponding coordinates at which the point should
 *	be drawn in the drawable used for display.  Those coordinates are
 *	then written into outArr[numOut*2] and outArr[numOut*2+1].
 *
 * Results:
 *	There is no return value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
translateAndAppendCoords(canvPtr, x, y, outArr, numOut)
    TkCanvas *canvPtr;			/* The canvas. */
    double x, y;			/* Coordinates in canvas space. */
    XPoint *outArr;                     /* Write results into this array */
    int numOut;                         /* Num of prior entries in outArr[] */
{
    double tmp;

    tmp = x - canvPtr->drawableXOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    outArr[numOut].x = (short) tmp;

    tmp = y  - canvPtr->drawableYOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    outArr[numOut].y = (short) tmp;
}

/*
 *--------------------------------------------------------------
 *
 * TkCanvTranslatePath
 *
 *	Translate a line or polygon path so that all vertices are
 *	within a rectangle that is 1000 pixels larger than the total
 *	size of the canvas window.  This will prevent pixel coordinates
 *	from overflowing the 16-bit integer size limitation imposed by
 *	most windowing systems.
 *
 *      coordPtr must point to an array of doubles, two doubles per
 *      vertex.  There are a total of numVertex vertices, or 2*numVertex
 *      entries in coordPtr.  The result vertices written into outArr
 *      have their coordinate origin shifted to canvPtr->drawableXOrigin
 *      by canvPtr->drawableYOrigin.  There might be as many as 3 times
 *      more output vertices than there are input vertices.  The calling
 *      function should allocate space accordingly.
 *
 *	This routine limits the width and height of a canvas window
 *	to 31767 pixels.  At the highest resolution display devices
 *	available today (210 ppi in Jan 2003) that's a window that is
 *      over 13 feet wide and tall.  Should be enough for the near
 *	future.
 *
 * Results:
 *      Clipped and translated path vertices are written into outArr[].
 *      There might be as many as twice the vertices in outArr[] as there
 *      are in coordPtr[].  The return value is the number of vertices
 *      actually written into outArr[].
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
int
TkCanvTranslatePath (canvPtr, numVertex, coordArr, closedPath, outArr)
    TkCanvas *canvPtr;  /* The canvas */
    int numVertex;      /* Number of vertices specified by coordArr[] */
    double *coordArr;   /* X and Y coordinates for each vertex */
    int closedPath;     /* True if this is a closed polygon */
    XPoint *outArr;     /* Write results here, if not NULL */
{
    int numOutput = 0;  /* Number of output coordinates */
    double lft, rgh;    /* Left and right sides of the bounding box */
    double top, btm;    /* Top and bottom sizes of the bounding box */
    double *tempArr;    /* Temporary storage used by the clipper */
    double *a, *b, *t;  /* Pointers to parts of the temporary storage */
    int i, j;           /* Loop counters */
    int maxOutput;      /* Maximum number of outputs that we will allow */
    double limit[4];          /* Boundries at which clipping occurs */
    double staticSpace[480];  /* Temp space from the stack */

    /*
    ** Constrain all vertices of the path to be within a box that is no
    ** larger than 32000 pixels wide or height.  The top-left corner of
    ** this clipping box is 1000 pixels above and to the left of the top
    ** left corner of the window on which the canvas is displayed.
    **
    ** This means that a canvas will not display properly on a canvas
    ** window that is larger than 31000 pixels wide or high.  That is not
    ** a problem today, but might someday become a factor for ultra-high
    ** resolutions displays.
    **
    ** The X11 protocol allows us (in theory) to expand the size of the
    ** clipping box to 32767 pixels.  But we have found experimentally that
    ** XFree86 sometimes fails to draw lines correctly if they are longer
    ** than about 32500 pixels.  So we have left a little margin in the
    ** size to mask that bug.
    */
    lft = canvPtr->xOrigin - 1000.0;
    top = canvPtr->yOrigin - 1000.0;
    rgh = lft + 32000.0;
    btm = top + 32000.0;

    /* Try the common case first - no clipping.  Loop over the input
    ** coordinates and translate them into appropriate output coordinates.
    ** But if a vertex outside of the bounding box is seen, break out of
    ** the loop.
    **
    ** Most of the time, no clipping is needed, so this one loop is 
    ** sufficient to do the translation.
    */
    for(i=0; i<numVertex; i++){
        double x, y;
        x = coordArr[i*2];
        y = coordArr[i*2+1];
        if( x<lft || x>rgh || y<top || y>btm ) break;
        translateAndAppendCoords(canvPtr, x, y, outArr, numOutput++);
    }
    if( i==numVertex ){
        assert( numOutput==numVertex );
        return numOutput;
    }

    /* If we reach this point, it means that some clipping is required.
    ** Begin by allocating some working storage - at least 6 times as much space
    ** as coordArr[] requires.  Divide this space into two separate arrays
    ** a[] and b[].  Initialize a[] to be equal to coordArr[].
    */
    if( numVertex*12 <= sizeof(staticSpace)/sizeof(staticSpace[0]) ){
        tempArr = staticSpace;
    } else {
        tempArr = (double*)ckalloc( numVertex*12*sizeof(tempArr[0]) );
    }
    for(i=0; i<numVertex*2; i++){
        tempArr[i] = coordArr[i];
    }
    a = tempArr;
    b = &tempArr[numVertex*6];

    /* We will make four passes through the input data.  On each pass,
    ** we copy the contents of a[] over into b[].  As we copy, we clip
    ** any line segments that extend to the right past xClip then we
    ** rotate the coordinate system 90 degrees clockwise.  After each
    ** pass is complete, we interchange a[] and b[] in preparation for
    ** the next pass.
    **
    ** Each pass clips line segments that extend beyond a single side
    ** of the bounding box, and four passes rotate the coordinate system
    ** back to its original value.  I'm not an expert on graphics 
    ** algorithms, but I think this is called Cohen-Sutherland polygon
    ** clipping.
    **
    ** The limit[] array contains the xClip value used for each of the
    ** four passes.
    */
    limit[0] = rgh;
    limit[1] = -top;
    limit[2] = -lft;
    limit[3] = btm;

    /* This is the loop that makes the four passes through the data.
    */
    maxOutput = numVertex*3;
    for(j=0; j<4; j++){
        double xClip = limit[j];
        int inside = a[0]<xClip;
        double priorY = a[1];
        numOutput = 0;

        /* Clip everything to the right of xClip.  Store the results in
        ** b[] rotated by 90 degrees clockwise.
        */
        for(i=0; i<numVertex; i++){
            double x = a[i*2];
            double y = a[i*2+1];
            if( x>=xClip ){
                /* The current vertex is to the right of xClip.
                */
                if( inside ){
                    /* If the current vertex is to the right of xClip but
                    ** the previous vertex was left of xClip, then draw a
                    ** line segment from the previous vertex to until it
                    ** intersects the vertical at xClip.
                    */
                    double x0, y0, yN;
                    assert( i>0 );
                    x0 = a[i*2-2];
                    y0 = a[i*2-1];
                    yN = y0 + (y - y0)*(xClip-x0)/(x-x0);
                    b[numOutput*2] = -yN;
                    b[numOutput*2+1] = xClip;
                    numOutput++;
                    assert( numOutput<=maxOutput );
                    priorY = yN;
                    inside = 0;
                }else if( i==0 ){
                    /* If the first vertex is to the right of xClip, add
                    ** a vertex that is the projection of the first vertex
                    ** onto the vertical xClip line.
                    */
                    b[0] = -y;
                    b[1] = xClip;
                    numOutput = 1;
                    priorY = y;
                }
            }else{
                /* The current vertex is to the left of xClip
                */
                if( !inside ){
                    /* If the current vertex is on the left of xClip and
                    ** one or more prior vertices where to the right, then
                    ** we have to draw a line segment along xClip that extends
                    ** from the spot where we first crossed from left to right
                    ** to the spot where we cross back from right to left.
                    */
                    double x0, y0, yN;
                    assert( i>0 );
                    x0 = a[i*2-2];
                    y0 = a[i*2-1];
                    yN = y0 + (y - y0)*(xClip-x0)/(x-x0);
                    if( yN!=priorY ){
                        b[numOutput*2] = -yN;
                        b[numOutput*2+1] = xClip;
                        numOutput++;
                        assert( numOutput<=maxOutput );
                    }
                    inside = 1;
                }
                b[numOutput*2] = -y;
                b[numOutput*2+1] = x;
                numOutput++;
                assert( numOutput<=maxOutput );
            }
        }

        /* Interchange a[] and b[] in preparation for the next pass.
        */
        t = a;
        a = b;
        b = t;
        numVertex = numOutput;
    }

    /* All clipping is now finished.  Convert the coordinates from doubles
    ** into XPoints and translate the origin for the drawable.
    */
    for(i=0; i<numVertex; i++){
        translateAndAppendCoords(canvPtr, a[i*2], a[i*2+1], outArr, i);
    }
    if( tempArr!=staticSpace ){
        ckfree((char *) tempArr);
    }
    return numOutput;
}
