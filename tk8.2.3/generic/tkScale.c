/* 
 * tkScale.c --
 *
 *	This module implements a scale widgets for the Tk toolkit.
 *	A scale displays a slider that can be adjusted to change a
 *	value;  it also displays numeric labels and a textual label,
 *	if desired.
 *	
 *	The modifications to use floating-point values are based on
 *	an implementation by Paul Mackerras.  The -variable option
 *	is due to Henning Schulzrinne.  All of these are used with
 *	permission.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkPort.h"
#include "default.h"
#include "tkInt.h"
#include "tclMath.h"
#include "tkScale.h"

/*
 * The following table defines the legal values for the -orient option.
 * It is used together with the "enum orient" declaration in tkScale.h.
 */

static char *orientStrings[] = {
    "horizontal", "vertical", (char *) NULL
};

/*
 * The following table defines the legal values for the -state option.
 * It is used together with the "enum state" declaration in tkScale.h.
 */

static char *stateStrings[] = {
    "active", "disabled", "normal", (char *) NULL
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCALE_ACTIVE_BG_COLOR, -1, Tk_Offset(TkScale, activeBorder),
	0, (ClientData) DEF_SCALE_ACTIVE_BG_MONO, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_SCALE_BG_COLOR, -1, Tk_Offset(TkScale, bgBorder),
	0, (ClientData) DEF_SCALE_BG_MONO, 0},
    {TK_OPTION_DOUBLE, "-bigincrement", "bigIncrement", "BigIncrement",
        DEF_SCALE_BIG_INCREMENT, -1, Tk_Offset(TkScale, bigIncrement), 
        0, 0, 0},
    {TK_OPTION_SYNONYM, "-bd", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_SCALE_BORDER_WIDTH, -1, Tk_Offset(TkScale, borderWidth), 
        0, 0, 0},
    {TK_OPTION_STRING, "-command", "command", "Command",
	DEF_SCALE_COMMAND, Tk_Offset(TkScale, commandPtr), -1,
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_SCALE_CURSOR, -1, Tk_Offset(TkScale, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_INT, "-digits", "digits", "Digits", 
	DEF_SCALE_DIGITS, -1, Tk_Offset(TkScale, digits), 
        0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_SCALE_FONT, -1, Tk_Offset(TkScale, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_SCALE_FG_COLOR, -1, Tk_Offset(TkScale, textColorPtr), 0, 
        (ClientData) DEF_SCALE_FG_MONO, 0},
    {TK_OPTION_DOUBLE, "-from", "from", "From", DEF_SCALE_FROM, -1, 
        Tk_Offset(TkScale, fromValue), 0, 0, 0},
    {TK_OPTION_BORDER, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_SCALE_HIGHLIGHT_BG_COLOR,
	-1, Tk_Offset(TkScale, highlightBorder), 
        0, (ClientData) DEF_SCALE_HIGHLIGHT_BG_MONO, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_SCALE_HIGHLIGHT, -1, Tk_Offset(TkScale, highlightColorPtr),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_SCALE_HIGHLIGHT_WIDTH, -1, 
	Tk_Offset(TkScale, highlightWidth), 0, 0, 0},
    {TK_OPTION_STRING, "-label", "label", "Label",
	DEF_SCALE_LABEL, Tk_Offset(TkScale, labelPtr), -1, 0, 0, 0},
    {TK_OPTION_PIXELS, "-length", "length", "Length",
	DEF_SCALE_LENGTH, -1, Tk_Offset(TkScale, length), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient",
        DEF_SCALE_ORIENT, -1, Tk_Offset(TkScale, orient), 
        0, (ClientData) orientStrings, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_SCALE_RELIEF, -1, Tk_Offset(TkScale, relief), 0, 0, 0},
    {TK_OPTION_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
        DEF_SCALE_REPEAT_DELAY, -1, Tk_Offset(TkScale, repeatDelay),
        0, 0, 0},
    {TK_OPTION_INT, "-repeatinterval", "repeatInterval", "RepeatInterval",
        DEF_SCALE_REPEAT_INTERVAL, -1, Tk_Offset(TkScale, repeatInterval),
        0, 0, 0},
    {TK_OPTION_DOUBLE, "-resolution", "resolution", "Resolution",
        DEF_SCALE_RESOLUTION, -1, Tk_Offset(TkScale, resolution),
        0, 0, 0},
    {TK_OPTION_BOOLEAN, "-showvalue", "showValue", "ShowValue",
        DEF_SCALE_SHOW_VALUE, -1, Tk_Offset(TkScale, showValue),
        0, 0, 0},
    {TK_OPTION_PIXELS, "-sliderlength", "sliderLength", "SliderLength",
        DEF_SCALE_SLIDER_LENGTH, -1, Tk_Offset(TkScale, sliderLength),
        0, 0, 0},
    {TK_OPTION_RELIEF, "-sliderrelief", "sliderRelief", "SliderRelief",
	DEF_SCALE_SLIDER_RELIEF, -1, Tk_Offset(TkScale, sliderRelief), 
        0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
        DEF_SCALE_STATE, -1, Tk_Offset(TkScale, state), 
        0, (ClientData) stateStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_SCALE_TAKE_FOCUS, Tk_Offset(TkScale, takeFocusPtr), -1,
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-tickinterval", "tickInterval", "TickInterval",
        DEF_SCALE_TICK_INTERVAL, -1, Tk_Offset(TkScale, tickInterval),
        0, 0, 0},
    {TK_OPTION_DOUBLE, "-to", "to", "To",
        DEF_SCALE_TO, -1, Tk_Offset(TkScale, toValue), 0, 0, 0},
    {TK_OPTION_COLOR, "-troughcolor", "troughColor", "Background",
        DEF_SCALE_TROUGH_COLOR, -1, Tk_Offset(TkScale, troughColorPtr),
        0, (ClientData) DEF_SCALE_TROUGH_MONO, 0},
    {TK_OPTION_STRING, "-variable", "variable", "Variable",
	DEF_SCALE_VARIABLE, Tk_Offset(TkScale, varNamePtr), -1,
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	DEF_SCALE_WIDTH, -1, Tk_Offset(TkScale, width), 0, 0, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, 0, 0}
};

/*
 * The following tables define the scale widget commands and map the 
 * indexes into the string tables into a single enumerated type used 
 * to dispatch the scale widget command.
 */

static char *commandNames[] = {
    "cget", "configure", "coords", "get", "identify", "set", (char *) NULL
};

enum command {
    COMMAND_CGET, COMMAND_CONFIGURE, COMMAND_COORDS, COMMAND_GET,
    COMMAND_IDENTIFY, COMMAND_SET
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		ComputeFormat _ANSI_ARGS_((TkScale *scalePtr));
static void		ComputeScaleGeometry _ANSI_ARGS_((TkScale *scalePtr));
static int		ConfigureScale _ANSI_ARGS_((Tcl_Interp *interp,
			    TkScale *scalePtr, int objc,
			    Tcl_Obj *CONST objv[]));
static void		DestroyScale _ANSI_ARGS_((char *memPtr));
static void		ScaleCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		ScaleEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static char *		ScaleVarProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
static int		ScaleWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static void		ScaleWorldChanged _ANSI_ARGS_((
			    ClientData instanceData));

/*
 * The structure below defines scale class behavior by means of procedures
 * that can be invoked from generic window code.
 */

static TkClassProcs scaleClass = {
    NULL,			/* createProc. */
    ScaleWorldChanged,		/* geometryProc. */
    NULL			/* modalProc. */
};


/*
 *--------------------------------------------------------------
 *
 * Tk_ScaleObjCmd --
 *
 *	This procedure is invoked to process the "scale" Tcl
 *	command.  See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_ScaleObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument values. */
{
    register TkScale *scalePtr;
    Tk_OptionTable optionTable;
    Tk_Window tkwin;

    optionTable = (Tk_OptionTable) clientData;
    if (optionTable == NULL) {
	Tcl_CmdInfo info;
	char *name;

	/*
	 * We haven't created the option table for this widget class
	 * yet.  Do it now and save the table as the clientData for
	 * the command, so we'll have access to it in future
	 * invocations of the command.
	 */

	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
	name = Tcl_GetString(objv[0]);
	Tcl_GetCommandInfo(interp, name, &info);
	info.objClientData = (ClientData) optionTable;
	Tcl_SetCommandInfo(interp, name, &info);
    }

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
            Tcl_GetString(objv[1]), (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    Tk_SetClass(tkwin, "Scale");
    scalePtr = TkpCreateScale(tkwin);

    /*
     * Initialize fields that won't be initialized by ConfigureScale,
     * or which ConfigureScale expects to have reasonable values
     * (e.g. resource pointers).
     */

    scalePtr->tkwin = tkwin;
    scalePtr->display = Tk_Display(tkwin);
    scalePtr->interp = interp;
    scalePtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(scalePtr->tkwin), ScaleWidgetObjCmd,
	    (ClientData) scalePtr, ScaleCmdDeletedProc);
    scalePtr->optionTable = optionTable;
    scalePtr->orient = ORIENT_VERTICAL;
    scalePtr->width = 0;
    scalePtr->length = 0;
    scalePtr->value = 0.0;
    scalePtr->varNamePtr = NULL;
    scalePtr->fromValue = 0.0;
    scalePtr->toValue = 0.0;
    scalePtr->tickInterval = 0.0;
    scalePtr->resolution = 1.0;
    scalePtr->digits = 0;
    scalePtr->bigIncrement = 0.0;
    scalePtr->commandPtr = NULL;
    scalePtr->repeatDelay = 0;
    scalePtr->repeatInterval = 0;
    scalePtr->labelPtr = NULL;
    scalePtr->labelLength = 0;
    scalePtr->state = STATE_NORMAL;
    scalePtr->borderWidth = 0;
    scalePtr->bgBorder = NULL;
    scalePtr->activeBorder = NULL;
    scalePtr->sliderRelief = TK_RELIEF_RAISED;
    scalePtr->troughColorPtr = NULL;
    scalePtr->troughGC = None;
    scalePtr->copyGC = None;
    scalePtr->tkfont = NULL;
    scalePtr->textColorPtr = NULL;
    scalePtr->textGC = None;
    scalePtr->relief = TK_RELIEF_FLAT;
    scalePtr->highlightWidth = 0;
    scalePtr->highlightBorder = NULL;
    scalePtr->highlightColorPtr = NULL;
    scalePtr->inset = 0;
    scalePtr->sliderLength = 0;
    scalePtr->showValue = 0;
    scalePtr->horizLabelY = 0;
    scalePtr->horizValueY = 0;
    scalePtr->horizTroughY = 0;
    scalePtr->horizTickY = 0;
    scalePtr->vertTickRightX = 0;
    scalePtr->vertValueRightX = 0;
    scalePtr->vertTroughX = 0;
    scalePtr->vertLabelX = 0;
    scalePtr->cursor = None;
    scalePtr->takeFocusPtr = NULL;
    scalePtr->flags = NEVER_SET;

    TkSetClassProcs(scalePtr->tkwin, &scaleClass, (ClientData) scalePtr);
    Tk_CreateEventHandler(scalePtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    ScaleEventProc, (ClientData) scalePtr);

    if (Tk_InitOptions(interp, (char *) scalePtr, optionTable, tkwin)
	    != TCL_OK) {
	Tk_DestroyWindow(scalePtr->tkwin);
	return TCL_ERROR;
    }
    if (ConfigureScale(interp, scalePtr, objc - 2, objv + 2) != TCL_OK) {
	Tk_DestroyWindow(scalePtr->tkwin);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(scalePtr->tkwin),
	    -1);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleWidgetObjCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
ScaleWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;		/* Information about scale
					 * widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument strings. */
{
    TkScale *scalePtr = (TkScale *) clientData;
    Tcl_Obj *objPtr;
    int index, result;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    result = Tcl_GetIndexFromObj(interp, objv[1], commandNames,
            "option", 0, &index);
    if (result != TCL_OK) {
	return result;
    }
    Tcl_Preserve((ClientData) scalePtr);

    switch (index) {
        case COMMAND_CGET: {
  	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 1, objv, "cget option");
		goto error;
	    }
	    objPtr = Tk_GetOptionValue(interp, (char *) scalePtr,
		    scalePtr->optionTable, objv[2], scalePtr->tkwin);
	    if (objPtr == NULL) {
		 goto error;
	    } else {
		Tcl_SetObjResult(interp, objPtr);
	    }
	    break;
	}
        case COMMAND_CONFIGURE: {
	    if (objc <= 3) {
		objPtr = Tk_GetOptionInfo(interp, (char *) scalePtr,
			scalePtr->optionTable,
			(objc == 3) ? objv[2] : (Tcl_Obj *) NULL,
			scalePtr->tkwin);
		if (objPtr == NULL) {
		    goto error;
		} else {
		    Tcl_SetObjResult(interp, objPtr);
		}
	    } else {
		result = ConfigureScale(interp, scalePtr, objc-2, objv+2);
	    }
	    break;
	}
        case COMMAND_COORDS: {
	    int x, y ;
	    double value;
	    char buf[TCL_INTEGER_SPACE * 2];

	    if ((objc != 2) && (objc != 3)) {
	        Tcl_WrongNumArgs(interp, 1, objv, "coords ?value?");
		goto error;
	    }
	    if (objc == 3) {
	        if (Tcl_GetDoubleFromObj(interp, objv[2], &value) 
                        != TCL_OK) {
		    goto error;
		}
	    } else {
	        value = scalePtr->value;
	    }
	    if (scalePtr->orient == ORIENT_VERTICAL) {
	        x = scalePtr->vertTroughX + scalePtr->width/2
		        + scalePtr->borderWidth;
		y = TkpValueToPixel(scalePtr, value);
	    } else {
	        x = TkpValueToPixel(scalePtr, value);
		y = scalePtr->horizTroughY + scalePtr->width/2
                        + scalePtr->borderWidth;
	    }
	    sprintf(buf, "%d %d", x, y);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
            break;
        }
        case COMMAND_GET: {
	    double value;
	    int x, y;
	    char buf[TCL_DOUBLE_SPACE];

	    if ((objc != 2) && (objc != 4)) {
	        Tcl_WrongNumArgs(interp, 1, objv, "get ?x y?");
		goto error;
	    }
	    if (objc == 2) {
	        value = scalePtr->value;
	    } else {
	        if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		        || (Tcl_GetIntFromObj(interp, objv[3], &y) 
                        != TCL_OK)) {
		    goto error;
		}
		value = TkpPixelToValue(scalePtr, x, y);
	    }
	    sprintf(buf, scalePtr->format, value);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
            break;
        }
        case COMMAND_IDENTIFY: {
	    int x, y, thing;

	    if (objc != 4) {
	        Tcl_WrongNumArgs(interp, 1, objv, "identify x y");
		goto error;
	    }
	    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
                    || (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	        goto error;
	    }
	    thing = TkpScaleElement(scalePtr, x,y);
	    switch (thing) {
	        case TROUGH1:
		    Tcl_SetResult(interp, "trough1", TCL_STATIC);
		    break;
	        case SLIDER:
		    Tcl_SetResult(interp, "slider", TCL_STATIC);
		    break;
	        case TROUGH2:
		    Tcl_SetResult(interp, "trough2", TCL_STATIC);
		    break;
	    }
            break;
        }
        case COMMAND_SET: {
	    double value;

	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 1, objv, "set value");
		goto error;
	    }
	    if (Tcl_GetDoubleFromObj(interp, objv[2], &value) != TCL_OK) {
	        goto error;
	    }
	    if ((scalePtr->state != STATE_DISABLED)) {
	      TkpSetScaleValue(scalePtr, value, 1, 1);
	    }
	    break;
        } 
    }
    Tcl_Release((ClientData) scalePtr);
    return result;

    error:
    Tcl_Release((ClientData) scalePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyScale --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a button at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the scale is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyScale(memPtr)
    char *memPtr;	/* Info about scale widget. */
{
    register TkScale *scalePtr = (TkScale *) memPtr;

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    if (scalePtr->varNamePtr != NULL) {
	Tcl_UntraceVar(scalePtr->interp, Tcl_GetString(scalePtr->varNamePtr),
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ScaleVarProc, (ClientData) scalePtr);
    }
    if (scalePtr->troughGC != None) {
	Tk_FreeGC(scalePtr->display, scalePtr->troughGC);
    }
    if (scalePtr->copyGC != None) {
	Tk_FreeGC(scalePtr->display, scalePtr->copyGC);
    }
    if (scalePtr->textGC != None) {
	Tk_FreeGC(scalePtr->display, scalePtr->textGC);
    }
    Tk_FreeConfigOptions((char *) scalePtr, scalePtr->optionTable,
	    scalePtr->tkwin);
    TkpDestroyScale(scalePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureScale --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a scale widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for scalePtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureScale(interp, scalePtr, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register TkScale *scalePtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int objc;			/* Number of valid entries in objv. */
    Tcl_Obj *CONST objv[];	/* Argument values. */
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error;
    char *label;

    /*
     * Eliminate any existing trace on a variable monitored by the scale.
     */

    if (scalePtr->varNamePtr != NULL) {
	Tcl_UntraceVar(interp, Tcl_GetString(scalePtr->varNamePtr), 
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ScaleVarProc, (ClientData) scalePtr);
    }

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) scalePtr,
		    scalePtr->optionTable, objc, objv,
		    scalePtr->tkwin, &savedOptions, (int *) NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: restore options to old values.
	     */

	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}

	/*
	 * If the scale is tied to the value of a variable, then set 
	 * the scale's value from the value of the variable, if it exists
	 * and it holds a valid double value.
	 */

	if (scalePtr->varNamePtr != NULL) {
	    char *name;
	    double value;
	    Tcl_Obj *valuePtr;

	    name = Tcl_GetString(scalePtr->varNamePtr);
	    valuePtr = Tcl_GetVar2Ex(interp, name, NULL, TCL_GLOBAL_ONLY);
	    if ((valuePtr != NULL) &&
		    (Tcl_GetDoubleFromObj(interp, valuePtr, &value)
		    == TCL_OK)) {
		scalePtr->value = TkRoundToResolution(scalePtr, value);
	    }
	}

	/*
	 * Several options need special processing, such as parsing the
	 * orientation and creating GCs.
	 */

	scalePtr->fromValue = TkRoundToResolution(scalePtr, 
                scalePtr->fromValue);
	scalePtr->toValue = TkRoundToResolution(scalePtr, scalePtr->toValue);
	scalePtr->tickInterval = TkRoundToResolution(scalePtr,
	        scalePtr->tickInterval);

	/*
	 * Make sure that the tick interval has the right sign so that
	 * addition moves from fromValue to toValue.
	 */

	if ((scalePtr->tickInterval < 0)
	    ^     ((scalePtr->toValue - scalePtr->fromValue) <  0)) {
	  scalePtr->tickInterval = -scalePtr->tickInterval;
	}

	/*
	 * Set the scale value to itself;  all this does is to make sure
	 * that the scale's value is within the new acceptable range for
	 * the scale and reflect the value in the associated variable,
	 * if any.
	 */

	ComputeFormat(scalePtr);
	TkpSetScaleValue(scalePtr, scalePtr->value, 1, 1);

	if (scalePtr->labelPtr != NULL) {
	    label = Tcl_GetString(scalePtr->labelPtr);
	    scalePtr->labelLength = strlen(label);
	} else {
	    scalePtr->labelLength = 0;
	}

	Tk_SetBackgroundFromBorder(scalePtr->tkwin, scalePtr->bgBorder);

	if (scalePtr->highlightWidth < 0) {
	    scalePtr->highlightWidth = 0;
	}
	scalePtr->inset = scalePtr->highlightWidth + scalePtr->borderWidth;
	break;
    }
    if (!error) {
        Tk_FreeSavedOptions(&savedOptions);
    }

    /*
     * Reestablish the variable trace, if it is needed.
     */

    if (scalePtr->varNamePtr != NULL) {
        Tcl_TraceVar(interp, Tcl_GetString(scalePtr->varNamePtr),
	        TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	        ScaleVarProc, (ClientData) scalePtr);
    }

    ScaleWorldChanged((ClientData) scalePtr);
    if (error) {
        Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    } else {
      return TCL_OK;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleWorldChanged --
 *
 *      This procedure is called when the world has changed in some
 *      way and the widget needs to recompute all its graphics contexts
 *	and determine its new geometry.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Scale will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
ScaleWorldChanged(instanceData)
    ClientData instanceData;	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc;
    TkScale *scalePtr;

    scalePtr = (TkScale *) instanceData;

    gcValues.foreground = scalePtr->troughColorPtr->pixel;
    gc = Tk_GetGC(scalePtr->tkwin, GCForeground, &gcValues);
    if (scalePtr->troughGC != None) {
	Tk_FreeGC(scalePtr->display, scalePtr->troughGC);
    }
    scalePtr->troughGC = gc;

    gcValues.font = Tk_FontId(scalePtr->tkfont);
    gcValues.foreground = scalePtr->textColorPtr->pixel;
    gc = Tk_GetGC(scalePtr->tkwin, GCForeground | GCFont, &gcValues);
    if (scalePtr->textGC != None) {
	Tk_FreeGC(scalePtr->display, scalePtr->textGC);
    }
    scalePtr->textGC = gc;

    if (scalePtr->copyGC == None) {
	gcValues.graphics_exposures = False;
	scalePtr->copyGC = Tk_GetGC(scalePtr->tkwin, GCGraphicsExposures,
	    &gcValues);
    }
    scalePtr->inset = scalePtr->highlightWidth + scalePtr->borderWidth;

    /*
     * Recompute display-related information, and let the geometry
     * manager know how much space is needed now.
     */

    ComputeScaleGeometry(scalePtr);

    TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeFormat --
 *
 *	This procedure is invoked to recompute the "format" field
 *	of a scale's widget record, which determines how the value
 *	of the scale is converted to a string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The format field of scalePtr is modified.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeFormat(scalePtr)
    TkScale *scalePtr;			/* Information about scale widget. */
{
    double maxValue, x;
    int mostSigDigit, numDigits, leastSigDigit, afterDecimal;
    int eDigits, fDigits;

    /*
     * Compute the displacement from the decimal of the most significant
     * digit required for any number in the scale's range.
     */

    maxValue = fabs(scalePtr->fromValue);
    x = fabs(scalePtr->toValue);
    if (x > maxValue) {
	maxValue = x;
    }
    if (maxValue == 0) {
	maxValue = 1;
    }
    mostSigDigit = (int) floor(log10(maxValue));

    /*
     * If the number of significant digits wasn't specified explicitly,
     * compute it. It's the difference between the most significant
     * digit needed to represent any number on the scale and the
     * most significant digit of the smallest difference between
     * numbers on the scale.  In other words, display enough digits so
     * that at least one digit will be different between any two adjacent
     * positions of the scale.
     */

    numDigits = scalePtr->digits;
    if (numDigits <= 0) {
	if  (scalePtr->resolution > 0) {
	    /*
	     * A resolution was specified for the scale, so just use it.
	     */

	    leastSigDigit = (int) floor(log10(scalePtr->resolution));
	} else {
	    /*
	     * No resolution was specified, so compute the difference
	     * in value between adjacent pixels and use it for the least
	     * significant digit.
	     */

	    x = fabs(scalePtr->fromValue - scalePtr->toValue);
	    if (scalePtr->length > 0) {
		x /= scalePtr->length;
	    }
	    if (x > 0){
		leastSigDigit = (int) floor(log10(x));
	    } else {
		leastSigDigit = 0;
	    }
	}
	numDigits = mostSigDigit - leastSigDigit + 1;
	if (numDigits < 1) {
	    numDigits = 1;
	}
    }

    /*
     * Compute the number of characters required using "e" format and
     * "f" format, and then choose whichever one takes fewer characters.
     */

    eDigits = numDigits + 4;
    if (numDigits > 1) {
	eDigits++;			/* Decimal point. */
    }
    afterDecimal = numDigits - mostSigDigit - 1;
    if (afterDecimal < 0) {
	afterDecimal = 0;
    }
    fDigits = (mostSigDigit >= 0) ? mostSigDigit + afterDecimal : afterDecimal;
    if (afterDecimal > 0) {
	fDigits++;			/* Decimal point. */
    }
    if (mostSigDigit < 0) {
	fDigits++;			/* Zero to left of decimal point. */
    }
    if (fDigits <= eDigits) {
	sprintf(scalePtr->format, "%%.%df", afterDecimal);
    } else {
	sprintf(scalePtr->format, "%%.%de", numDigits-1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeScaleGeometry --
 *
 *	This procedure is called to compute various geometrical
 *	information for a scale, such as where various things get
 *	displayed.  It's called when the window is reconfigured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Display-related numbers get changed in *scalePtr.  The
 *	geometry manager gets told about the window's preferred size.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeScaleGeometry(scalePtr)
    register TkScale *scalePtr;		/* Information about widget. */
{
    char valueString[PRINT_CHARS];
    int tmp, valuePixels, x, y, extraSpace;
    Tk_FontMetrics fm;
    char *label;

    /*
     * Horizontal scales are simpler than vertical ones because
     * all sizes are the same (the height of a line of text);
     * handle them first and then quit.
     */

    Tk_GetFontMetrics(scalePtr->tkfont, &fm);
    if (!scalePtr->orient == ORIENT_VERTICAL) {
	y = scalePtr->inset;
	extraSpace = 0;
	if (scalePtr->labelLength != 0) {
	    scalePtr->horizLabelY = y + SPACING;
	    y += fm.linespace + SPACING;
	    extraSpace = SPACING;
	}
	if (scalePtr->showValue) {
	    scalePtr->horizValueY = y + SPACING;
	    y += fm.linespace + SPACING;
	    extraSpace = SPACING;
	} else {
	    scalePtr->horizValueY = y;
	}
	y += extraSpace;
	scalePtr->horizTroughY = y;
	y += scalePtr->width + 2*scalePtr->borderWidth;
	if (scalePtr->tickInterval != 0) {
	    scalePtr->horizTickY = y + SPACING;
	    y += fm.linespace + 2*SPACING;
	}
	Tk_GeometryRequest(scalePtr->tkwin,
		scalePtr->length + 2*scalePtr->inset, y + scalePtr->inset);
	Tk_SetInternalBorder(scalePtr->tkwin, scalePtr->inset);
	return;
    }

    /*
     * Vertical scale:  compute the amount of space needed to display
     * the scales value by formatting strings for the two end points;
     * use whichever length is longer.
     */

    sprintf(valueString, scalePtr->format, scalePtr->fromValue);
    valuePixels = Tk_TextWidth(scalePtr->tkfont, valueString, -1);

    sprintf(valueString, scalePtr->format, scalePtr->toValue);
    tmp = Tk_TextWidth(scalePtr->tkfont, valueString, -1);
    if (valuePixels < tmp) {
	valuePixels = tmp;
    }

    /*
     * Assign x-locations to the elements of the scale, working from
     * left to right.
     */

    x = scalePtr->inset;
    if ((scalePtr->tickInterval != 0) && (scalePtr->showValue)) {
	scalePtr->vertTickRightX = x + SPACING + valuePixels;
	scalePtr->vertValueRightX = scalePtr->vertTickRightX + valuePixels
		+ fm.ascent/2;
	x = scalePtr->vertValueRightX + SPACING;
    } else if (scalePtr->tickInterval != 0) {
	scalePtr->vertTickRightX = x + SPACING + valuePixels;
	scalePtr->vertValueRightX = scalePtr->vertTickRightX;
	x = scalePtr->vertTickRightX + SPACING;
    } else if (scalePtr->showValue) {
	scalePtr->vertTickRightX = x;
	scalePtr->vertValueRightX = x + SPACING + valuePixels;
	x = scalePtr->vertValueRightX + SPACING;
    } else {
	scalePtr->vertTickRightX = x;
	scalePtr->vertValueRightX = x;
    }
    scalePtr->vertTroughX = x;
    x += 2*scalePtr->borderWidth + scalePtr->width;
    if (scalePtr->labelLength == 0) {
	scalePtr->vertLabelX = 0;
    } else {
	scalePtr->vertLabelX = x + fm.ascent/2;
	label = Tcl_GetString(scalePtr->labelPtr);
	x = scalePtr->vertLabelX + fm.ascent/2
		+ Tk_TextWidth(scalePtr->tkfont, label,
			scalePtr->labelLength);
    }
    Tk_GeometryRequest(scalePtr->tkwin, x + scalePtr->inset,
	    scalePtr->length + 2*scalePtr->inset);
    Tk_SetInternalBorder(scalePtr->tkwin, scalePtr->inset);
}

/*
 *--------------------------------------------------------------
 *
 * ScaleEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on scales.
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
ScaleEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    TkScale *scalePtr = (TkScale *) clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
    } else if (eventPtr->type == DestroyNotify) {
	if (scalePtr->tkwin != NULL) {
	    scalePtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(scalePtr->interp, scalePtr->widgetCmd);
	}
	if (scalePtr->flags & REDRAW_ALL) {
	    Tcl_CancelIdleCall(TkpDisplayScale, (ClientData) scalePtr);
	}
	Tcl_EventuallyFree((ClientData) scalePtr, DestroyScale);
    } else if (eventPtr->type == ConfigureNotify) {
	ComputeScaleGeometry(scalePtr);
	TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scalePtr->flags |= GOT_FOCUS;
	    if (scalePtr->highlightWidth > 0) {
		TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scalePtr->flags &= ~GOT_FOCUS;
	    if (scalePtr->highlightWidth > 0) {
		TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ScaleCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
ScaleCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TkScale *scalePtr = (TkScale *) clientData;
    Tk_Window tkwin = scalePtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	scalePtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkEventuallyRedrawScale --
 *
 *	Arrange for part or all of a scale widget to redrawn at
 *	the next convenient time in the future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If "what" is REDRAW_SLIDER then just the slider and the
 *	value readout will be redrawn;  if "what" is REDRAW_ALL
 *	then the entire widget will be redrawn.
 *
 *--------------------------------------------------------------
 */

void
TkEventuallyRedrawScale(scalePtr, what)
    register TkScale *scalePtr;	/* Information about widget. */
    int what;			/* What to redraw:  REDRAW_SLIDER
				 * or REDRAW_ALL. */
{
    if ((what == 0) || (scalePtr->tkwin == NULL)
	    || !Tk_IsMapped(scalePtr->tkwin)) {
	return;
    }
    if ((scalePtr->flags & REDRAW_ALL) == 0) {
	Tcl_DoWhenIdle(TkpDisplayScale, (ClientData) scalePtr);
    }
    scalePtr->flags |= what;
}

/*
 *--------------------------------------------------------------
 *
 * TkRoundToResolution --
 *
 *	Round a given floating-point value to the nearest multiple
 *	of the scale's resolution.
 *
 * Results:
 *	The return value is the rounded result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
TkRoundToResolution(scalePtr, value)
    TkScale *scalePtr;		/* Information about scale widget. */
    double value;		/* Value to round. */
{
    double rem, new;

    if (scalePtr->resolution <= 0) {
	return value;
    }
    new = scalePtr->resolution * floor(value/scalePtr->resolution);
    rem = value - new;
    if (rem < 0) {
	if (rem <= -scalePtr->resolution/2) {
	    new -= scalePtr->resolution;
	}
    } else {
	if (rem >= scalePtr->resolution/2) {
	    new += scalePtr->resolution;
	}
    }
    return new;
}

/*
 *----------------------------------------------------------------------
 *
 * ScaleVarProc --
 *
 *	This procedure is invoked by Tcl whenever someone modifies a
 *	variable associated with a scale widget.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The value displayed in the scale will change to match the
 *	variable's new value.  If the variable has a bogus value then
 *	it is reset to the value of the scale.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static char *
ScaleVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about button. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    register TkScale *scalePtr = (TkScale *) clientData;
    char *resultStr, *name;
    double value;
    Tcl_Obj *valuePtr;
    int result;

    name = Tcl_GetString(scalePtr->varNamePtr);

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_TraceVar(interp, name,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    ScaleVarProc, clientData);
	    scalePtr->flags |= NEVER_SET;
	    TkpSetScaleValue(scalePtr, scalePtr->value, 1, 0);
	}
	return (char *) NULL;
    }

    /*
     * If we came here because we updated the variable (in TkpSetScaleValue),
     * then ignore the trace.  Otherwise update the scale with the value
     * of the variable.
     */

    if (scalePtr->flags & SETTING_VAR) {
	return (char *) NULL;
    }
    resultStr = NULL;
    valuePtr = Tcl_GetVar2Ex(interp, name, NULL, 
            TCL_GLOBAL_ONLY);
    result = Tcl_GetDoubleFromObj(interp, valuePtr, &value);
    if (result != TCL_OK) {
        resultStr = "can't assign non-numeric value to scale variable";
    } else {
      scalePtr->value = TkRoundToResolution(scalePtr, value);
      
      /*
       * This code is a bit tricky because it sets the scale's value before
       * calling TkpSetScaleValue.  This way, TkpSetScaleValue won't bother 
       * to set the variable again or to invoke the -command.  However, it
       * also won't redisplay the scale, so we have to ask for that
       * explicitly.
       */

      TkpSetScaleValue(scalePtr, scalePtr->value, 1, 0);
      TkEventuallyRedrawScale(scalePtr, REDRAW_SLIDER);
    }

    return resultStr;
}
