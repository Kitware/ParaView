/* 
 * tkMenubutton.c --
 *
 *	This module implements button-like widgets that are used
 *	to invoke pull-down menus.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkMenubutton.h"
#include "tkPort.h"
#include "default.h"

/*
 * The following table defines the legal values for the -direction 
 * option.  It is used together with the "enum direction" declaration 
 * in tkMenubutton.h.
 */

static char *directionStrings[] = {
    "above", "below", "flush", "left", "right", (char *) NULL
};

/*
 * The following table defines the legal values for the -state option.
 * It is used together with the "enum state" declaration in tkMenubutton.h.
 */

static char *stateStrings[] = {
    "active", "disabled", "normal", (char *) NULL
};

/*
 * Information used for parsing configuration specs:
 */

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BORDER, "-activebackground", "activeBackground", "Foreground",
        DEF_MENUBUTTON_ACTIVE_BG_COLOR, -1, 
        Tk_Offset(TkMenuButton, activeBorder), 0, 
        (ClientData) DEF_MENUBUTTON_ACTIVE_BG_MONO, 0},
    {TK_OPTION_COLOR, "-activeforeground", "activeForeground", "Background",
	DEF_MENUBUTTON_ACTIVE_FG_COLOR, -1, 
         Tk_Offset(TkMenuButton, activeFg),
         0, (ClientData) DEF_MENUBUTTON_ACTIVE_FG_MONO, 0},
    {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor",
	DEF_MENUBUTTON_ANCHOR, -1, 
        Tk_Offset(TkMenuButton, anchor), 0, 0, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_MENUBUTTON_BG_COLOR, -1, Tk_Offset(TkMenuButton, normalBorder),
        0, (ClientData) DEF_MENUBUTTON_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_BITMAP, "-bitmap", "bitmap", "Bitmap",
	DEF_MENUBUTTON_BITMAP, -1, Tk_Offset(TkMenuButton, bitmap),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_MENUBUTTON_BORDER_WIDTH, -1, 
        Tk_Offset(TkMenuButton, borderWidth), 0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_MENUBUTTON_CURSOR, -1, Tk_Offset(TkMenuButton, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-direction", "direction", "Direction",
    	DEF_MENUBUTTON_DIRECTION, -1, Tk_Offset(TkMenuButton, direction), 
	0, (ClientData) directionStrings, 0},
    {TK_OPTION_COLOR, "-disabledforeground", "disabledForeground",
	"DisabledForeground", DEF_MENUBUTTON_DISABLED_FG_COLOR,
	-1, Tk_Offset(TkMenuButton, disabledFg), TK_OPTION_NULL_OK,
	(ClientData) DEF_MENUBUTTON_DISABLED_FG_MONO, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_MENUBUTTON_FONT, -1, Tk_Offset(TkMenuButton, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_MENUBUTTON_FG, -1, Tk_Offset(TkMenuButton, normalFg), 0, 0, 0},
    {TK_OPTION_STRING, "-height", "height", "Height",
	DEF_MENUBUTTON_HEIGHT, -1, Tk_Offset(TkMenuButton, heightString), 
        0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_MENUBUTTON_HIGHLIGHT_BG_COLOR,
	-1, Tk_Offset(TkMenuButton, highlightBgColorPtr), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_MENUBUTTON_HIGHLIGHT, -1, 
        Tk_Offset(TkMenuButton, highlightColorPtr),	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_MENUBUTTON_HIGHLIGHT_WIDTH,
	-1, Tk_Offset(TkMenuButton, highlightWidth), 0, 0, 0},
    {TK_OPTION_STRING, "-image", "image", "Image",
	DEF_MENUBUTTON_IMAGE, -1, Tk_Offset(TkMenuButton, imageString), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-indicatoron", "indicatorOn", "IndicatorOn",
	DEF_MENUBUTTON_INDICATOR, -1, Tk_Offset(TkMenuButton, indicatorOn),
        0, 0, 0},
    {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
	DEF_BUTTON_JUSTIFY, -1, Tk_Offset(TkMenuButton, justify), 0, 0, 0},
    {TK_OPTION_STRING, "-menu", "menu", "Menu",
	DEF_MENUBUTTON_MENU, -1, Tk_Offset(TkMenuButton, menuName), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-padx", "padX", "Pad",
	DEF_MENUBUTTON_PADX, -1, Tk_Offset(TkMenuButton, padX),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-pady", "padY", "Pad",
	DEF_MENUBUTTON_PADY, -1, Tk_Offset(TkMenuButton, padY),
	0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_MENUBUTTON_RELIEF, -1, Tk_Offset(TkMenuButton, relief), 
        0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	DEF_MENUBUTTON_STATE, -1, Tk_Offset(TkMenuButton, state),
	0, (ClientData) stateStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_MENUBUTTON_TAKE_FOCUS, -1, 
        Tk_Offset(TkMenuButton, takeFocus), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-text", "text", "Text",
	DEF_MENUBUTTON_TEXT, -1, Tk_Offset(TkMenuButton, text), 0, 0, 0},
    {TK_OPTION_STRING, "-textvariable", "textVariable", "Variable",
	DEF_MENUBUTTON_TEXT_VARIABLE, -1, 
        Tk_Offset(TkMenuButton, textVarName), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_INT, "-underline", "underline", "Underline",
	DEF_MENUBUTTON_UNDERLINE, -1, Tk_Offset(TkMenuButton, underline),
         0, 0, 0},
    {TK_OPTION_STRING, "-width", "width", "Width",
	DEF_MENUBUTTON_WIDTH, -1, Tk_Offset(TkMenuButton, widthString), 
        0, 0, 0},
    {TK_OPTION_PIXELS, "-wraplength", "wrapLength", "WrapLength",
	DEF_MENUBUTTON_WRAP_LENGTH, -1, Tk_Offset(TkMenuButton, wrapLength),
        0, 0, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * The following tables define the menubutton widget commands and map the 
 * indexes into the string tables into a single enumerated type used 
 * to dispatch the scale widget command.
 */

static char *commandNames[] = {
    "cget", "configure", (char *) NULL
};

enum command {
    COMMAND_CGET, COMMAND_CONFIGURE
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		MenuButtonCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		MenuButtonEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		MenuButtonImageProc _ANSI_ARGS_((ClientData clientData,
			    int x, int y, int width, int height, int imgWidth,
			    int imgHeight));
static char *		MenuButtonTextVarProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    char *name1, char *name2, int flags));
static int		MenuButtonWidgetObjCmd _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp, 
			    int objc, Tcl_Obj *CONST objv[]));
static int		ConfigureMenuButton _ANSI_ARGS_((Tcl_Interp *interp,
			    TkMenuButton *mbPtr, int objc, 
                            Tcl_Obj *CONST objv[]));
static void		DestroyMenuButton _ANSI_ARGS_((char *memPtr));

/*
 *--------------------------------------------------------------
 *
 * Tk_MenubuttonObjCmd --
 *
 *	This procedure is invoked to process the "button", "label",
 *	"radiobutton", and "checkbutton" Tcl commands.  See the
 *	user documentation for details on what it does.
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
Tk_MenubuttonObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Either NULL or pointer to 
				 * option table. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register TkMenuButton *mbPtr;
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

    /*
     * Create the new window.
     */

    tkwin = Tk_CreateWindowFromPath(interp, 
        Tk_MainWindow(interp), Tcl_GetString(objv[1]), (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    Tk_SetClass(tkwin, "Menubutton");
    mbPtr = TkpCreateMenuButton(tkwin);

    TkSetClassProcs(tkwin, &tkpMenubuttonClass, (ClientData) mbPtr);

    /*
     * Initialize the data structure for the button.
     */

    mbPtr->tkwin = tkwin;
    mbPtr->display = Tk_Display (tkwin);
    mbPtr->interp = interp;
    mbPtr->widgetCmd = Tcl_CreateObjCommand(interp, 
            Tk_PathName(mbPtr->tkwin), MenuButtonWidgetObjCmd, 
            (ClientData) mbPtr, MenuButtonCmdDeletedProc);
    mbPtr->optionTable = optionTable;
    mbPtr->menuName = NULL;
    mbPtr->text = NULL;
    mbPtr->underline = -1;
    mbPtr->textVarName = NULL;
    mbPtr->bitmap = None;
    mbPtr->imageString = NULL;
    mbPtr->image = NULL;
    mbPtr->state = STATE_NORMAL;
    mbPtr->normalBorder = NULL;
    mbPtr->activeBorder = NULL;
    mbPtr->borderWidth = 0;
    mbPtr->relief = TK_RELIEF_FLAT;
    mbPtr->highlightWidth = 0;
    mbPtr->highlightBgColorPtr = NULL;
    mbPtr->highlightColorPtr = NULL;
    mbPtr->inset = 0;
    mbPtr->tkfont = NULL;
    mbPtr->normalFg = NULL;
    mbPtr->activeFg = NULL;
    mbPtr->disabledFg = NULL;
    mbPtr->normalTextGC = None;
    mbPtr->activeTextGC = None;
    mbPtr->gray = None;
    mbPtr->disabledGC = None;
    mbPtr->leftBearing = 0;
    mbPtr->rightBearing = 0;
    mbPtr->widthString = NULL;
    mbPtr->heightString = NULL;
    mbPtr->width = 0;
    mbPtr->width = 0;
    mbPtr->wrapLength = 0;
    mbPtr->padX = 0;
    mbPtr->padY = 0;
    mbPtr->anchor = TK_ANCHOR_CENTER;
    mbPtr->justify = TK_JUSTIFY_CENTER;
    mbPtr->textLayout = NULL;
    mbPtr->indicatorOn = 0;
    mbPtr->indicatorWidth = 0;
    mbPtr->indicatorHeight = 0;
    mbPtr->direction = DIRECTION_FLUSH;
    mbPtr->cursor = None;
    mbPtr->takeFocus = NULL;
    mbPtr->flags = 0;

    Tk_CreateEventHandler(mbPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    MenuButtonEventProc, (ClientData) mbPtr);

    if (Tk_InitOptions(interp, (char *) mbPtr, optionTable, tkwin)
            != TCL_OK) {
	Tk_DestroyWindow(mbPtr->tkwin);
	return TCL_ERROR;
    }

    if (ConfigureMenuButton(interp, mbPtr, objc-2, objv+2) != TCL_OK) {
	Tk_DestroyWindow(mbPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(mbPtr->tkwin),
            -1);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonWidgetObjCmd --
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
MenuButtonWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about button widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    int result, index;
    Tcl_Obj *objPtr;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    result = Tcl_GetIndexFromObj(interp, objv[1], 
            commandNames, "option", 0, &index);
    if (result != TCL_OK) {
        return result;
    }
    Tcl_Preserve((ClientData) mbPtr);

    switch (index) {
        case COMMAND_CGET: {
	    if (objc != 3) {
	        Tcl_WrongNumArgs(interp, 1, objv, "cget option");
	    goto error;
	    }

	    objPtr = Tk_GetOptionValue(interp, (char *) mbPtr,
                    mbPtr->optionTable, objv[2], mbPtr->tkwin);
	    if (objPtr == NULL) {
	        goto error;
	    } else {
	        Tcl_SetObjResult(interp, objPtr);
	    }
	    break;
	}

        case COMMAND_CONFIGURE: {
	    if (objc <= 3) {
		objPtr = Tk_GetOptionInfo(interp, (char *) mbPtr,
			mbPtr->optionTable,
			(objc == 3) ? objv[2] : (Tcl_Obj *) NULL,
			mbPtr->tkwin);
		if (objPtr == NULL) {
		    goto error;
		} else {
		    Tcl_SetObjResult(interp, objPtr);
		}
	    } else {
		result = ConfigureMenuButton(interp, mbPtr, objc-2, 
		        objv+2);
	    }
	    break;
	}
    }
    Tcl_Release((ClientData) mbPtr);
    return result;

    error:
    Tcl_Release((ClientData) mbPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenuButton --
 *
 *	This procedure is invoked to recycle all of the resources
 *	associated with a menubutton widget.  It is invoked as a
 *	when-idle handler in order to make sure that there is no
 *	other use of the menubutton pending at the time of the deletion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenuButton(memPtr)
    char *memPtr;		/* Info about button widget. */
{
    register TkMenuButton *mbPtr = (TkMenuButton *) memPtr;
    TkpDestroyMenuButton(mbPtr);

    if (mbPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(TkpDisplayMenuButton, (ClientData) mbPtr);
    }

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    Tcl_DeleteCommandFromToken(mbPtr->interp, mbPtr->widgetCmd);
    if (mbPtr->textVarName != NULL) {
	Tcl_UntraceVar(mbPtr->interp, mbPtr->textVarName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuButtonTextVarProc, (ClientData) mbPtr);
    }
    if (mbPtr->image != NULL) {
	Tk_FreeImage(mbPtr->image);
    }
    if (mbPtr->normalTextGC != None) {
	Tk_FreeGC(mbPtr->display, mbPtr->normalTextGC);
    }
    if (mbPtr->activeTextGC != None) {
	Tk_FreeGC(mbPtr->display, mbPtr->activeTextGC);
    }
    if (mbPtr->disabledGC != None) {
	Tk_FreeGC(mbPtr->display, mbPtr->disabledGC);
    }
    if (mbPtr->gray != None) {
	Tk_FreeBitmap(mbPtr->display, mbPtr->gray);
    }
    if (mbPtr->textLayout != NULL) {
        Tk_FreeTextLayout(mbPtr->textLayout);
    }
    Tk_FreeConfigOptions((char *) mbPtr, mbPtr->optionTable,
	    mbPtr->tkwin);
    mbPtr->tkwin = NULL;
    Tcl_EventuallyFree((ClientData) mbPtr, TCL_DYNAMIC);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenuButton --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a menubutton widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for mbPtr;  old resources get freed, if there
 *	were any.  The menubutton is redisplayed.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenuButton(interp, mbPtr, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register TkMenuButton *mbPtr;	
                                /* Information about widget;  may or may
				 * not already have values for some 
				 * fields. */
    int objc;			/* Number of valid entries in objv. */
    Tcl_Obj *CONST objv[];	/* Arguments. */
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error;
    Tk_Image image;

    /*
     * Eliminate any existing trace on variables monitored by the 
     * menubutton.
     */

    if (mbPtr->textVarName != NULL) {
	Tcl_UntraceVar(interp, mbPtr->textVarName, 
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuButtonTextVarProc, (ClientData) mbPtr);
    }

    /*
     * The following loop is potentially executed twice.  During the
     * first pass configuration options get set to their new values.
     * If there is an error in this pass, we execute a second pass
     * to restore all the options to their previous values.
     */

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) mbPtr,
		    mbPtr->optionTable, objc, objv,
		    mbPtr->tkwin, &savedOptions, (int *) NULL) != TCL_OK) {
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
	 * A few options need special processing, such as setting the
	 * background from a 3-D border, or filling in complicated
	 * defaults that couldn't be specified to Tk_SetOptions.
	 */

	if ((mbPtr->state == STATE_ACTIVE)
		&& !Tk_StrictMotif(mbPtr->tkwin)) {
	    Tk_SetBackgroundFromBorder(mbPtr->tkwin, mbPtr->activeBorder);
	} else {
	    Tk_SetBackgroundFromBorder(mbPtr->tkwin, mbPtr->normalBorder);
	}

	if (mbPtr->highlightWidth < 0) {
	    mbPtr->highlightWidth = 0;
	}

	if (mbPtr->padX < 0) {
	    mbPtr->padX = 0;
	}
	if (mbPtr->padY < 0) {
	    mbPtr->padY = 0;
	}

	/*
	 * Get the image for the widget, if there is one.  Allocate the
	 * new image before freeing the old one, so that the reference
	 * count doesn't go to zero and cause image data to be discarded.
	 */

	if (mbPtr->imageString != NULL) {
	    image = Tk_GetImage(mbPtr->interp, mbPtr->tkwin,
		    mbPtr->imageString, MenuButtonImageProc, 
		    (ClientData) mbPtr);
	    if (image == NULL) {
	        return TCL_ERROR;
	    }
	} else {
	    image = NULL;
	}
	if (mbPtr->image != NULL) {
	    Tk_FreeImage(mbPtr->image);
	}
	mbPtr->image = image;

	/*
	 * Recompute the geometry for the button.
	 */

	if ((mbPtr->bitmap != None) || (mbPtr->image != NULL)) {
	    if (Tk_GetPixels(interp, mbPtr->tkwin, mbPtr->widthString,
                    &mbPtr->width) != TCL_OK) {
                widthError:
	        Tcl_AddErrorInfo(interp, "\n    (processing -width option)");
		continue;
	    }
	    if (Tk_GetPixels(interp, mbPtr->tkwin, mbPtr->heightString,
		    &mbPtr->height) != TCL_OK) {
	        heightError:
	        Tcl_AddErrorInfo(interp, "\n    (processing -height option)");
		continue;
	    }
	} else {
	    if (Tcl_GetInt(interp, mbPtr->widthString, &mbPtr->width)
		    != TCL_OK) {
	        goto widthError;
	    }
	    if (Tcl_GetInt(interp, mbPtr->heightString, &mbPtr->height)
		    != TCL_OK) {
	        goto heightError;
	    }
	}
	break;
    }

    if (!error) {
      Tk_FreeSavedOptions(&savedOptions);
    }

    if ((mbPtr->image == NULL) && (mbPtr->bitmap == None)
	    && (mbPtr->textVarName != NULL)) {

      /*
       * The menubutton displays the value of a variable.  
       * Set up a trace to watch for any changes in it, create
       * the variable if it doesn't exist, and fetch its
       * current value.
       */

      char *value;

      value = Tcl_GetVar(interp, mbPtr->textVarName, TCL_GLOBAL_ONLY);
      if (value == NULL) {
	  Tcl_SetVar(interp, mbPtr->textVarName, mbPtr->text,
		     TCL_GLOBAL_ONLY);
      } else {
	  if (mbPtr->text != NULL) {
	      ckfree(mbPtr->text);
	  }
	  mbPtr->text = (char *) ckalloc((unsigned) (strlen(value) + 1));
	  strcpy(mbPtr->text, value);
      }
      Tcl_TraceVar(interp, mbPtr->textVarName,
		   TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		   MenuButtonTextVarProc, (ClientData) mbPtr);
    }

    TkMenuButtonWorldChanged((ClientData) mbPtr);
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
 * TkMenuButtonWorldChanged --
 *
 *      This procedure is called when the world has changed in some
 *      way and the widget needs to recompute all its graphics contexts
 *	and determine its new geometry.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TkMenuButton will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */
 
void
TkMenuButtonWorldChanged(instanceData)
    ClientData instanceData;	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc;
    unsigned long mask;
    TkMenuButton *mbPtr;

    mbPtr = (TkMenuButton *) instanceData;

    gcValues.font = Tk_FontId(mbPtr->tkfont);
    gcValues.foreground = mbPtr->normalFg->pixel;
    gcValues.background = Tk_3DBorderColor(mbPtr->normalBorder)->pixel;

    /*
     * Note: GraphicsExpose events are disabled in GC's because they're
     * used to copy stuff from an off-screen pixmap onto the screen (we know
     * that there's no problem with obscured areas).
     */

    gcValues.graphics_exposures = False;
    mask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
    gc = Tk_GetGC(mbPtr->tkwin, mask, &gcValues);
    if (mbPtr->normalTextGC != None) {
	Tk_FreeGC(mbPtr->display, mbPtr->normalTextGC);
    }
    mbPtr->normalTextGC = gc;

    gcValues.font = Tk_FontId(mbPtr->tkfont);
    gcValues.foreground = mbPtr->activeFg->pixel;
    gcValues.background = Tk_3DBorderColor(mbPtr->activeBorder)->pixel;
    mask = GCForeground | GCBackground | GCFont;
    gc = Tk_GetGC(mbPtr->tkwin, mask, &gcValues);
    if (mbPtr->activeTextGC != None) {
	Tk_FreeGC(mbPtr->display, mbPtr->activeTextGC);
    }
    mbPtr->activeTextGC = gc;

    gcValues.font = Tk_FontId(mbPtr->tkfont);
    gcValues.background = Tk_3DBorderColor(mbPtr->normalBorder)->pixel;
    if ((mbPtr->disabledFg != NULL) && (mbPtr->imageString == NULL)) {
	gcValues.foreground = mbPtr->disabledFg->pixel;
	mask = GCForeground | GCBackground | GCFont;
    } else {
	gcValues.foreground = gcValues.background;
	mask = GCForeground;
	if (mbPtr->gray == None) {
	    mbPtr->gray = Tk_GetBitmap(NULL, mbPtr->tkwin,
		    Tk_GetUid("gray50"));
	}
	if (mbPtr->gray != None) {
	    gcValues.fill_style = FillStippled;
	    gcValues.stipple = mbPtr->gray;
	    mask |= GCFillStyle | GCStipple;
	}
    }
    gc = Tk_GetGC(mbPtr->tkwin, mask, &gcValues);
    if (mbPtr->disabledGC != None) {
	Tk_FreeGC(mbPtr->display, mbPtr->disabledGC);
    }
    mbPtr->disabledGC = gc;

    TkpComputeMenuButtonGeometry(mbPtr);

    /*
     * Lastly, arrange for the button to be redisplayed.
     */

    if (Tk_IsMapped(mbPtr->tkwin) && !(mbPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(TkpDisplayMenuButton, (ClientData) mbPtr);
	mbPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on buttons.
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
MenuButtonEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	goto redraw;
    } else if (eventPtr->type == ConfigureNotify) {
	/*
	 * Must redraw after size changes, since layout could have changed
	 * and borders will need to be redrawn.
	 */

	goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
        DestroyMenuButton((char *) mbPtr);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    mbPtr->flags |= GOT_FOCUS;
	    if (mbPtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    mbPtr->flags &= ~GOT_FOCUS;
	    if (mbPtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    }
    return;

    redraw:
    if ((mbPtr->tkwin != NULL) && !(mbPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(TkpDisplayMenuButton, (ClientData) mbPtr);
	mbPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenuButtonCmdDeletedProc --
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
MenuButtonCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    Tk_Window tkwin = mbPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonTextVarProc --
 *
 *	This procedure is invoked when someone changes the variable
 *	whose contents are to be displayed in a menu button.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The text displayed in the menu button will change to match the
 *	variable.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
MenuButtonTextVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about button. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    register TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    char *value;

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_SetVar(interp, mbPtr->textVarName, mbPtr->text,
		    TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, mbPtr->textVarName,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    MenuButtonTextVarProc, clientData);
	}
	return (char *) NULL;
    }

    value = Tcl_GetVar(interp, mbPtr->textVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    if (mbPtr->text != NULL) {
	ckfree(mbPtr->text);
    }
    mbPtr->text = (char *) ckalloc((unsigned) (strlen(value) + 1));
    strcpy(mbPtr->text, value);
    TkpComputeMenuButtonGeometry(mbPtr);

    if ((mbPtr->tkwin != NULL) && Tk_IsMapped(mbPtr->tkwin)
	    && !(mbPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(TkpDisplayMenuButton, (ClientData) mbPtr);
	mbPtr->flags |= REDRAW_PENDING;
    }
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * MenuButtonImageProc --
 *
 *	This procedure is invoked by the image code whenever the manager
 *	for an image does something that affects the size of contents
 *	of an image displayed in a button.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the button to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
MenuButtonImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;		/* Pointer to widget record. */
    int x, y;				/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;			/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;		/* New dimensions of image. */
{
    register TkMenuButton *mbPtr = (TkMenuButton *) clientData;

    if (mbPtr->tkwin != NULL) {
	TkpComputeMenuButtonGeometry(mbPtr);
	if (Tk_IsMapped(mbPtr->tkwin) && !(mbPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(TkpDisplayMenuButton, (ClientData) mbPtr);
	    mbPtr->flags |= REDRAW_PENDING;
	}
    }
}
