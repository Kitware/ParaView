/* 
 * tkCanvPs.c --
 *
 *	This module provides Postscript output support for canvases,
 *	including the "postscript" widget command plus a few utility
 *	procedures used for generating Postscript.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkCanvas.h"
#include "tkPort.h"

/*
 * See tkCanvas.h for key data structures used to implement canvases.
 */

/*
 * The following definition is used in generating postscript for images
 * and windows.
 */

typedef struct TkColormapData {	/* Hold color information for a window */
    int separated;		/* Whether to use separate color bands */
    int color;			/* Whether window is color or black/white */
    int ncolors;		/* Number of color values stored */
    XColor *colors;		/* Pixel value -> RGB mappings */
    int red_mask, green_mask, blue_mask;	/* Masks and shifts for each */
    int red_shift, green_shift, blue_shift;	/* color band */
} TkColormapData;

/*
 * One of the following structures is created to keep track of Postscript
 * output being generated.  It consists mostly of information provided on
 * the widget command line.
 */

typedef struct TkPostscriptInfo {
    int x, y, width, height;	/* Area to print, in canvas pixel
				 * coordinates. */
    int x2, y2;			/* x+width and y+height. */
    char *pageXString;		/* String value of "-pagex" option or NULL. */
    char *pageYString;		/* String value of "-pagey" option or NULL. */
    double pageX, pageY;	/* Postscript coordinates (in points)
				 * corresponding to pageXString and
				 * pageYString. Don't forget that y-values
				 * grow upwards for Postscript! */
    char *pageWidthString;	/* Printed width of output. */
    char *pageHeightString;	/* Printed height of output. */
    double scale;		/* Scale factor for conversion: each pixel
				 * maps into this many points. */
    Tk_Anchor pageAnchor;	/* How to anchor bbox on Postscript page. */
    int rotate;			/* Non-zero means output should be rotated
				 * on page (landscape mode). */
    char *fontVar;		/* If non-NULL, gives name of global variable
				 * containing font mapping information.
				 * Malloc'ed. */
    char *colorVar;		/* If non-NULL, give name of global variable
				 * containing color mapping information.
				 * Malloc'ed. */
    char *colorMode;		/* Mode for handling colors:  "monochrome",
				 * "gray", or "color".  Malloc'ed. */
    int colorLevel;		/* Numeric value corresponding to colorMode:
				 * 0 for mono, 1 for gray, 2 for color. */
    char *fileName;		/* Name of file in which to write Postscript;
				 * NULL means return Postscript info as
				 * result. Malloc'ed. */
    char *channelName;		/* If -channel is specified, the name of
                                 * the channel to use. */
    Tcl_Channel chan;		/* Open channel corresponding to fileName. */
    Tcl_HashTable fontTable;	/* Hash table containing names of all font
				 * families used in output.  The hash table
				 * values are not used. */
    int prepass;		/* Non-zero means that we're currently in
				 * the pre-pass that collects font information,
				 * so the Postscript generated isn't
				 * relevant. */
    int prolog;			/* Non-zero means output should contain
				   the file prolog.ps in the header. */
} TkPostscriptInfo;

/*
 * The table below provides a template that's used to process arguments
 * to the canvas "postscript" command and fill in TkPostscriptInfo
 * structures.
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_STRING, "-colormap", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, colorVar), 0},
    {TK_CONFIG_STRING, "-colormode", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, colorMode), 0},
    {TK_CONFIG_STRING, "-file", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, fileName), 0},
    {TK_CONFIG_STRING, "-channel", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, channelName), 0},
    {TK_CONFIG_STRING, "-fontmap", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, fontVar), 0},
    {TK_CONFIG_PIXELS, "-height", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, height), 0},
    {TK_CONFIG_ANCHOR, "-pageanchor", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, pageAnchor), 0},
    {TK_CONFIG_STRING, "-pageheight", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, pageHeightString), 0},
    {TK_CONFIG_STRING, "-pagewidth", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, pageWidthString), 0},
    {TK_CONFIG_STRING, "-pagex", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, pageXString), 0},
    {TK_CONFIG_STRING, "-pagey", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, pageYString), 0},
    {TK_CONFIG_BOOLEAN, "-prolog", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, prolog), 0},
    {TK_CONFIG_BOOLEAN, "-rotate", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, rotate), 0},
    {TK_CONFIG_PIXELS, "-width", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, width), 0},
    {TK_CONFIG_PIXELS, "-x", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, x), 0},
    {TK_CONFIG_PIXELS, "-y", (char *) NULL, (char *) NULL,
	"", Tk_Offset(TkPostscriptInfo, y), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		GetPostscriptPoints _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, double *doublePtr));

/*
 *--------------------------------------------------------------
 *
 * TkCanvPostscriptCmd --
 *
 *	This procedure is invoked to process the "postscript" options
 *	of the widget command for canvas widgets. See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

    /* ARGSUSED */
int
TkCanvPostscriptCmd(canvasPtr, interp, argc, argv)
    TkCanvas *canvasPtr;		/* Information about canvas widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings.  Caller has
					 * already parsed this command enough
					 * to know that argv[1] is
					 * "postscript". */
{
    TkPostscriptInfo psInfo;
    Tk_PostscriptInfo oldInfoPtr;
    int result;
    Tk_Item *itemPtr;
#define STRING_LENGTH 400
    char string[STRING_LENGTH+1];
    CONST char *p;
    time_t now;
    size_t length;
    Tk_Window tkwin = canvasPtr->tkwin;
    int deltaX = 0, deltaY = 0;		/* Offset of lower-left corner of
					 * area to be marked up, measured
					 * in canvas units from the positioning
					 * point on the page (reflects
					 * anchor position).  Initial values
					 * needed only to stop compiler
					 * warnings. */
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_DString buffer;
    char psenccmd[]="::tk::ensure_psenc_is_loaded";

    /*
     *----------------------------------------------------------------
     * Initialize the data structure describing Postscript generation,
     * then process all the arguments to fill the data structure in.
     *----------------------------------------------------------------
     */
    result = Tcl_EvalEx(interp,psenccmd,-1,TCL_EVAL_GLOBAL);
    if (result != TCL_OK) {
        return result;
    }
    oldInfoPtr = canvasPtr->psInfo;
    canvasPtr->psInfo = (Tk_PostscriptInfo) &psInfo;
    psInfo.x = canvasPtr->xOrigin;
    psInfo.y = canvasPtr->yOrigin;
    psInfo.width = -1;
    psInfo.height = -1;
    psInfo.pageXString = NULL;
    psInfo.pageYString = NULL;
    psInfo.pageX = 72*4.25;
    psInfo.pageY = 72*5.5;
    psInfo.pageWidthString = NULL;
    psInfo.pageHeightString = NULL;
    psInfo.scale = 1.0;
    psInfo.pageAnchor = TK_ANCHOR_CENTER;
    psInfo.rotate = 0;
    psInfo.fontVar = NULL;
    psInfo.colorVar = NULL;
    psInfo.colorMode = NULL;
    psInfo.colorLevel = 0;
    psInfo.fileName = NULL;
    psInfo.channelName = NULL;
    psInfo.chan = NULL;
    psInfo.prepass = 0;
    psInfo.prolog = 1;
    Tcl_InitHashTable(&psInfo.fontTable, TCL_STRING_KEYS);
    result = Tk_ConfigureWidget(interp, tkwin,
	    configSpecs, argc-2, argv+2, (char *) &psInfo,
	    TK_CONFIG_ARGV_ONLY);
    if (result != TCL_OK) {
	goto cleanup;
    }

    if (psInfo.width == -1) {
	psInfo.width = Tk_Width(tkwin);
    }
    if (psInfo.height == -1) {
	psInfo.height = Tk_Height(tkwin);
    }
    psInfo.x2 = psInfo.x + psInfo.width;
    psInfo.y2 = psInfo.y + psInfo.height;

    if (psInfo.pageXString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageXString,
		&psInfo.pageX) != TCL_OK) {
	    goto cleanup;
	}
    }
    if (psInfo.pageYString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageYString,
		&psInfo.pageY) != TCL_OK) {
	    goto cleanup;
	}
    }
    if (psInfo.pageWidthString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageWidthString,
		&psInfo.scale) != TCL_OK) {
	    goto cleanup;
	}
	psInfo.scale /= psInfo.width;
    } else if (psInfo.pageHeightString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageHeightString,
		&psInfo.scale) != TCL_OK) {
	    goto cleanup;
	}
	psInfo.scale /= psInfo.height;
    } else {
	psInfo.scale = (72.0/25.4)*WidthMMOfScreen(Tk_Screen(tkwin));
	psInfo.scale /= WidthOfScreen(Tk_Screen(tkwin));
    }
    switch (psInfo.pageAnchor) {
	case TK_ANCHOR_NW:
	case TK_ANCHOR_W:
	case TK_ANCHOR_SW:
	    deltaX = 0;
	    break;
	case TK_ANCHOR_N:
	case TK_ANCHOR_CENTER:
	case TK_ANCHOR_S:
	    deltaX = -psInfo.width/2;
	    break;
	case TK_ANCHOR_NE:
	case TK_ANCHOR_E:
	case TK_ANCHOR_SE:
	    deltaX = -psInfo.width;
	    break;
    }
    switch (psInfo.pageAnchor) {
	case TK_ANCHOR_NW:
	case TK_ANCHOR_N:
	case TK_ANCHOR_NE:
	    deltaY = - psInfo.height;
	    break;
	case TK_ANCHOR_W:
	case TK_ANCHOR_CENTER:
	case TK_ANCHOR_E:
	    deltaY = -psInfo.height/2;
	    break;
	case TK_ANCHOR_SW:
	case TK_ANCHOR_S:
	case TK_ANCHOR_SE:
	    deltaY = 0;
	    break;
    }

    if (psInfo.colorMode == NULL) {
	psInfo.colorLevel = 2;
    } else {
	length = strlen(psInfo.colorMode);
	if (strncmp(psInfo.colorMode, "monochrome", length) == 0) {
	    psInfo.colorLevel = 0;
	} else if (strncmp(psInfo.colorMode, "gray", length) == 0) {
	    psInfo.colorLevel = 1;
	} else if (strncmp(psInfo.colorMode, "color", length) == 0) {
	    psInfo.colorLevel = 2;
	} else {
	    Tcl_AppendResult(interp, "bad color mode \"",
		    psInfo.colorMode, "\": must be monochrome, ",
		    "gray, or color", (char *) NULL);
	    goto cleanup;
	}
    }

    if (psInfo.fileName != NULL) {

        /*
         * Check that -file and -channel are not both specified.
         */

        if (psInfo.channelName != NULL) {
            Tcl_AppendResult(interp, "can't specify both -file",
                    " and -channel", (char *) NULL);
            result = TCL_ERROR;
            goto cleanup;
        }

        /*
         * Check that we are not in a safe interpreter. If we are, disallow
         * the -file specification.
         */

        if (Tcl_IsSafe(interp)) {
            Tcl_AppendResult(interp, "can't specify -file in a",
                    " safe interpreter", (char *) NULL);
            result = TCL_ERROR;
            goto cleanup;
        }
        
	p = Tcl_TranslateFileName(interp, psInfo.fileName, &buffer);
	if (p == NULL) {
	    goto cleanup;
	}
	psInfo.chan = Tcl_OpenFileChannel(interp, p, "w", 0666);
	Tcl_DStringFree(&buffer);
	if (psInfo.chan == NULL) {
	    goto cleanup;
	}
    }

    if (psInfo.channelName != NULL) {
        int mode;
        
        /*
         * Check that the channel is found in this interpreter and that it
         * is open for writing.
         */

        psInfo.chan = Tcl_GetChannel(interp, psInfo.channelName,
                &mode);
        if (psInfo.chan == (Tcl_Channel) NULL) {
            result = TCL_ERROR;
            goto cleanup;
        }
        if ((mode & TCL_WRITABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"",
                    psInfo.channelName, "\" wasn't opened for writing",
                    (char *) NULL);
            result = TCL_ERROR;
            goto cleanup;
        }
    }
    
    /*
     *--------------------------------------------------------
     * Make a pre-pass over all of the items, generating Postscript
     * and then throwing it away.  The purpose of this pass is just
     * to collect information about all the fonts in use, so that
     * we can output font information in the proper form required
     * by the Document Structuring Conventions.
     *--------------------------------------------------------
     */

    psInfo.prepass = 1;
    for (itemPtr = canvasPtr->firstItemPtr; itemPtr != NULL;
	    itemPtr = itemPtr->nextPtr) {
	if ((itemPtr->x1 >= psInfo.x2) || (itemPtr->x2 < psInfo.x)
		|| (itemPtr->y1 >= psInfo.y2) || (itemPtr->y2 < psInfo.y)) {
	    continue;
	}
	if (itemPtr->typePtr->postscriptProc == NULL) {
	    continue;
	}
	result = (*itemPtr->typePtr->postscriptProc)(interp,
		(Tk_Canvas) canvasPtr, itemPtr, 1);
	Tcl_ResetResult(interp);
	if (result != TCL_OK) {
	    /*
	     * An error just occurred.  Just skip out of this loop.
	     * There's no need to report the error now;  it can be
	     * reported later (errors can happen later that don't
	     * happen now, so we still have to check for errors later
	     * anyway).
	     */
	    break;
	}
    }
    psInfo.prepass = 0;

    /*
     *--------------------------------------------------------
     * Generate the header and prolog for the Postscript.
     *--------------------------------------------------------
     */

    if (psInfo.prolog) {
      Tcl_AppendResult(interp, "%!PS-Adobe-3.0 EPSF-3.0\n",
		       "%%Creator: Tk Canvas Widget\n", (char *) NULL);
#ifdef HAVE_PW_GECOS
    if (!Tcl_IsSafe(interp)) {
	struct passwd *pwPtr = getpwuid(getuid());	/* INTL: Native. */
	Tcl_AppendResult(interp, "%%For: ",
		(pwPtr != NULL) ? pwPtr->pw_gecos : "Unknown", "\n",
		(char *) NULL);
	endpwent();
    }
#endif /* HAVE_PW_GECOS */
    Tcl_AppendResult(interp, "%%Title: Window ",
	    Tk_PathName(tkwin), "\n", (char *) NULL);
    time(&now);
    Tcl_AppendResult(interp, "%%CreationDate: ",
	    ctime(&now), (char *) NULL);		/* INTL: Native. */
    if (!psInfo.rotate) {
	sprintf(string, "%d %d %d %d",
		(int) (psInfo.pageX + psInfo.scale*deltaX),
		(int) (psInfo.pageY + psInfo.scale*deltaY),
		(int) (psInfo.pageX + psInfo.scale*(deltaX + psInfo.width)
			+ 1.0),
		(int) (psInfo.pageY + psInfo.scale*(deltaY + psInfo.height)
			+ 1.0));
    } else {
	sprintf(string, "%d %d %d %d",
		(int) (psInfo.pageX - psInfo.scale*(deltaY + psInfo.height)),
		(int) (psInfo.pageY + psInfo.scale*deltaX),
		(int) (psInfo.pageX - psInfo.scale*deltaY + 1.0),
		(int) (psInfo.pageY + psInfo.scale*(deltaX + psInfo.width)
			+ 1.0));
    }
    Tcl_AppendResult(interp, "%%BoundingBox: ", string,
	    "\n", (char *) NULL);
    Tcl_AppendResult(interp, "%%Pages: 1\n", 
	    "%%DocumentData: Clean7Bit\n", (char *) NULL);
    Tcl_AppendResult(interp, "%%Orientation: ",
	    psInfo.rotate ? "Landscape\n" : "Portrait\n", (char *) NULL);
    p = "%%DocumentNeededResources: font ";
    for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	Tcl_AppendResult(interp, p,
		Tcl_GetHashKey(&psInfo.fontTable, hPtr),
		"\n", (char *) NULL);
	p = "%%+ font ";
    }
    Tcl_AppendResult(interp, "%%EndComments\n\n", (char *) NULL);

    /*
     * Insert the prolog
     */
    Tcl_AppendResult(interp, Tcl_GetVar(interp,"::tk::ps_preamable",
	    TCL_GLOBAL_ONLY), (char *) NULL);

    if (psInfo.chan != NULL) {
        Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	Tcl_ResetResult(canvasPtr->interp);
    }

    /*
     *-----------------------------------------------------------
     * Document setup:  set the color level and include fonts.
     *-----------------------------------------------------------
     */

    sprintf(string, "/CL %d def\n", psInfo.colorLevel);
    Tcl_AppendResult(interp, "%%BeginSetup\n", string,
	    (char *) NULL);
    for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	Tcl_AppendResult(interp, "%%IncludeResource: font ",
		Tcl_GetHashKey(&psInfo.fontTable, hPtr), "\n", (char *) NULL);
    }
    Tcl_AppendResult(interp, "%%EndSetup\n\n", (char *) NULL);

    /*
     *-----------------------------------------------------------
     * Page setup:  move to page positioning point, rotate if
     * needed, set scale factor, offset for proper anchor position,
     * and set clip region.
     *-----------------------------------------------------------
     */

    Tcl_AppendResult(interp, "%%Page: 1 1\n", "save\n",
	    (char *) NULL);
    sprintf(string, "%.1f %.1f translate\n", psInfo.pageX, psInfo.pageY);
    Tcl_AppendResult(interp, string, (char *) NULL);
    if (psInfo.rotate) {
	Tcl_AppendResult(interp, "90 rotate\n", (char *) NULL);
    }
    sprintf(string, "%.4g %.4g scale\n", psInfo.scale, psInfo.scale);
    Tcl_AppendResult(interp, string, (char *) NULL);
    sprintf(string, "%d %d translate\n", deltaX - psInfo.x, deltaY);
    Tcl_AppendResult(interp, string, (char *) NULL);
    sprintf(string, "%d %.15g moveto %d %.15g lineto %d %.15g lineto %d %.15g",
	    psInfo.x,
	    Tk_PostscriptY((double) psInfo.y, (Tk_PostscriptInfo) &psInfo),
	    psInfo.x2,
	    Tk_PostscriptY((double) psInfo.y, (Tk_PostscriptInfo) &psInfo),
	    psInfo.x2, 
	    Tk_PostscriptY((double) psInfo.y2, (Tk_PostscriptInfo) &psInfo),
	    psInfo.x,
	    Tk_PostscriptY((double) psInfo.y2, (Tk_PostscriptInfo) &psInfo));
    Tcl_AppendResult(interp, string,
	" lineto closepath clip newpath\n", (char *) NULL);
    }
    if (psInfo.chan != NULL) {
	Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	Tcl_ResetResult(canvasPtr->interp);
    }

    /*
     *---------------------------------------------------------------------
     * Iterate through all the items, having each relevant one draw itself.
     * Quit if any of the items returns an error.
     *---------------------------------------------------------------------
     */

    result = TCL_OK;
    for (itemPtr = canvasPtr->firstItemPtr; itemPtr != NULL;
	    itemPtr = itemPtr->nextPtr) {
	if ((itemPtr->x1 >= psInfo.x2) || (itemPtr->x2 < psInfo.x)
		|| (itemPtr->y1 >= psInfo.y2) || (itemPtr->y2 < psInfo.y)) {
	    continue;
	}
	if (itemPtr->typePtr->postscriptProc == NULL) {
	    continue;
	}
	if (itemPtr->state == TK_STATE_HIDDEN) {
	    continue;
	}
	Tcl_AppendResult(interp, "gsave\n", (char *) NULL);
	result = (*itemPtr->typePtr->postscriptProc)(interp,
		(Tk_Canvas) canvasPtr, itemPtr, 0);
	if (result != TCL_OK) {
	    char msg[64 + TCL_INTEGER_SPACE];

	    sprintf(msg, "\n    (generating Postscript for item %d)",
		    itemPtr->id);
	    Tcl_AddErrorInfo(interp, msg);
	    goto cleanup;
	}
	Tcl_AppendResult(interp, "grestore\n", (char *) NULL);
	if (psInfo.chan != NULL) {
	    Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	    Tcl_ResetResult(interp);
	}
    }

    /*
     *---------------------------------------------------------------------
     * Output page-end information, such as commands to print the page
     * and document trailer stuff.
     *---------------------------------------------------------------------
     */

    if (psInfo.prolog) {
      Tcl_AppendResult(interp, "restore showpage\n\n",
	    "%%Trailer\nend\n%%EOF\n", (char *) NULL);
    }
    if (psInfo.chan != NULL) {
	Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	Tcl_ResetResult(canvasPtr->interp);
    }

    /*
     * Clean up psInfo to release malloc'ed stuff.
     */

    cleanup:
    if (psInfo.pageXString != NULL) {
	ckfree(psInfo.pageXString);
    }
    if (psInfo.pageYString != NULL) {
	ckfree(psInfo.pageYString);
    }
    if (psInfo.pageWidthString != NULL) {
	ckfree(psInfo.pageWidthString);
    }
    if (psInfo.pageHeightString != NULL) {
	ckfree(psInfo.pageHeightString);
    }
    if (psInfo.fontVar != NULL) {
	ckfree(psInfo.fontVar);
    }
    if (psInfo.colorVar != NULL) {
	ckfree(psInfo.colorVar);
    }
    if (psInfo.colorMode != NULL) {
	ckfree(psInfo.colorMode);
    }
    if (psInfo.fileName != NULL) {
	ckfree(psInfo.fileName);
    }
    if ((psInfo.chan != NULL) && (psInfo.channelName == NULL)) {
	Tcl_Close(interp, psInfo.chan);
    }
    if (psInfo.channelName != NULL) {
        ckfree(psInfo.channelName);
    }
    Tcl_DeleteHashTable(&psInfo.fontTable);
    canvasPtr->psInfo = (Tk_PostscriptInfo) oldInfoPtr;
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptColor --
 *
 *	This procedure is called by individual canvas items when
 *	they want to set a color value for output.  Given information
 *	about an X color, this procedure will generate Postscript
 *	commands to set up an appropriate color in Postscript.
 *
 * Results:
 *	Returns a standard Tcl return value.  If an error occurs
 *	then an error message will be left in the interp's result.
 *	If no error occurs, then additional Postscript will be
 *	appended to the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptColor(interp, psInfo, colorPtr)
    Tcl_Interp *interp;
    Tk_PostscriptInfo psInfo;		/* Postscript info. */
    XColor *colorPtr;			/* Information about color. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    int tmp;
    double red, green, blue;
    char string[200];

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    /*
     * If there is a color map defined, then look up the color's name
     * in the map and use the Postscript commands found there, if there
     * are any.
     */

    if (psInfoPtr->colorVar != NULL) {
	CONST char *cmdString;

	cmdString = Tcl_GetVar2(interp, psInfoPtr->colorVar,
		Tk_NameOfColor(colorPtr), 0);
	if (cmdString != NULL) {
	    Tcl_AppendResult(interp, cmdString, "\n", (char *) NULL);
	    return TCL_OK;
	}
    }

    /*
     * No color map entry for this color.  Grab the color's intensities
     * and output Postscript commands for them.  Special note:  X uses
     * a range of 0-65535 for intensities, but most displays only use
     * a range of 0-255, which maps to (0, 256, 512, ... 65280) in the
     * X scale.  This means that there's no way to get perfect white,
     * since the highest intensity is only 65280 out of 65535.  To
     * work around this problem, rescale the X intensity to a 0-255
     * scale and use that as the basis for the Postscript colors.  This
     * scheme still won't work if the display only uses 4 bits per color,
     * but most diplays use at least 8 bits.
     */

    tmp = colorPtr->red;
    red = ((double) (tmp >> 8))/255.0;
    tmp = colorPtr->green;
    green = ((double) (tmp >> 8))/255.0;
    tmp = colorPtr->blue;
    blue = ((double) (tmp >> 8))/255.0;
    sprintf(string, "%.3f %.3f %.3f setrgbcolor AdjustColor\n",
	    red, green, blue);
    Tcl_AppendResult(interp, string, (char *) NULL);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptFont --
 *
 *	This procedure is called by individual canvas items when
 *	they want to output text.  Given information about an X
 *	font, this procedure will generate Postscript commands
 *	to set up an appropriate font in Postscript.
 *
 * Results:
 *	Returns a standard Tcl return value.  If an error occurs
 *	then an error message will be left in the interp's result.
 *	If no error occurs, then additional Postscript will be
 *	appended to the interp's result.
 *
 * Side effects:
 *	The Postscript font name is entered into psInfoPtr->fontTable
 *	if it wasn't already there.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptFont(interp, psInfo, tkfont)
    Tcl_Interp *interp;
    Tk_PostscriptInfo psInfo;		/* Postscript Info. */
    Tk_Font tkfont;			/* Information about font in which text
					 * is to be printed. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    char *end;
    char pointString[TCL_INTEGER_SPACE];
    Tcl_DString ds;
    int i, points;

    /*
     * First, look up the font's name in the font map, if there is one.
     * If there is an entry for this font, it consists of a list
     * containing font name and size.  Use this information.
     */

    Tcl_DStringInit(&ds);
    
    if (psInfoPtr->fontVar != NULL) {
	CONST char *list;
	int argc;
	double size;
	CONST char **argv;
	CONST char *name;

	name = Tk_NameOfFont(tkfont);
	list = Tcl_GetVar2(interp, psInfoPtr->fontVar, name, 0);
	if (list != NULL) {
	    if (Tcl_SplitList(interp, list, &argc, &argv) != TCL_OK) {
		badMapEntry:
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "bad font map entry for \"", name,
			"\": \"", list, "\"", (char *) NULL);
		return TCL_ERROR;
	    }
	    if (argc != 2) {
		goto badMapEntry;
	    }
	    size = strtod(argv[1], &end);
	    if ((size <= 0) || (*end != 0)) {
		goto badMapEntry;
	    }

	    Tcl_DStringAppend(&ds, argv[0], -1);
	    points = (int) size;
	    
	    ckfree((char *) argv);
	    goto findfont;
	}
    } 

    points = Tk_PostscriptFontName(tkfont, &ds);

    findfont:
    sprintf(pointString, "%d", points);
    Tcl_AppendResult(interp, "/", Tcl_DStringValue(&ds), " findfont ",
	    pointString, " scalefont ", (char *) NULL);
    if (strncasecmp(Tcl_DStringValue(&ds), "Symbol", 7) != 0) {
	Tcl_AppendResult(interp, "ISOEncode ", (char *) NULL);
    }
    Tcl_AppendResult(interp, "setfont\n", (char *) NULL);
    Tcl_CreateHashEntry(&psInfoPtr->fontTable, Tcl_DStringValue(&ds), &i);
    Tcl_DStringFree(&ds);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptBitmap --
 *
 *	This procedure is called to output the contents of a
 *	sub-region of a bitmap in proper image data format for
 *	Postscript (i.e. data between angle brackets, one bit
 *	per pixel).
 *
 * Results:
 *	Returns a standard Tcl return value.  If an error occurs
 *	then an error message will be left in the interp's result.
 *	If no error occurs, then additional Postscript will be
 *	appended to the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptBitmap(interp, tkwin, psInfo, bitmap, startX, startY, width,
	height)
    Tcl_Interp *interp;
    Tk_Window tkwin;
    Tk_PostscriptInfo psInfo;		/* Postscript info. */
    Pixmap bitmap;			/* Bitmap for which to generate
					 * Postscript. */
    int startX, startY;			/* Coordinates of upper-left corner
					 * of rectangular region to output. */
    int width, height;			/* Height of rectangular region. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    XImage *imagePtr;
    int charsInLine, x, y, lastX, lastY, value, mask;
    unsigned int totalWidth, totalHeight;
    char string[100];
    Window dummyRoot;
    int dummyX, dummyY;
    unsigned dummyBorderwidth, dummyDepth;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    /*
     * The following call should probably be a call to Tk_SizeOfBitmap
     * instead, but it seems that we are occasionally invoked by custom
     * item types that create their own bitmaps without registering them
     * with Tk.  XGetGeometry is a bit slower than Tk_SizeOfBitmap, but
     * it shouldn't matter here.
     */

    XGetGeometry(Tk_Display(tkwin), bitmap, &dummyRoot,
	    (int *) &dummyX, (int *) &dummyY, (unsigned int *) &totalWidth,
	    (unsigned int *) &totalHeight, &dummyBorderwidth, &dummyDepth);
    imagePtr = XGetImage(Tk_Display(tkwin), bitmap, 0, 0,
	    totalWidth, totalHeight, 1, XYPixmap);
    Tcl_AppendResult(interp, "<", (char *) NULL);
    mask = 0x80;
    value = 0;
    charsInLine = 0;
    lastX = startX + width - 1;
    lastY = startY + height - 1;
    for (y = lastY; y >= startY; y--) {
	for (x = startX; x <= lastX; x++) {
	    if (XGetPixel(imagePtr, x, y)) {
		value |= mask;
	    }
	    mask >>= 1;
	    if (mask == 0) {
		sprintf(string, "%02x", value);
		Tcl_AppendResult(interp, string, (char *) NULL);
		mask = 0x80;
		value = 0;
		charsInLine += 2;
		if (charsInLine >= 60) {
		    Tcl_AppendResult(interp, "\n", (char *) NULL);
		    charsInLine = 0;
		}
	    }
	}
	if (mask != 0x80) {
	    sprintf(string, "%02x", value);
	    Tcl_AppendResult(interp, string, (char *) NULL);
	    mask = 0x80;
	    value = 0;
	    charsInLine += 2;
	}
    }
    Tcl_AppendResult(interp, ">", (char *) NULL);
    XDestroyImage(imagePtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptStipple --
 *
 *	This procedure is called by individual canvas items when
 *	they have created a path that they'd like to be filled with
 *	a stipple pattern.  Given information about an X bitmap,
 *	this procedure will generate Postscript commands to fill
 *	the current clip region using a stipple pattern defined by the
 *	bitmap.
 *
 * Results:
 *	Returns a standard Tcl return value.  If an error occurs
 *	then an error message will be left in the interp's result.
 *	If no error occurs, then additional Postscript will be
 *	appended to the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptStipple(interp, tkwin, psInfo, bitmap)
    Tcl_Interp *interp;
    Tk_Window tkwin;
    Tk_PostscriptInfo psInfo;		/* Interpreter for returning Postscript
					 * or error message. */
    Pixmap bitmap;			/* Bitmap to use for stippling. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    int width, height;
    char string[TCL_INTEGER_SPACE * 2];
    Window dummyRoot;
    int dummyX, dummyY;
    unsigned dummyBorderwidth, dummyDepth;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    /*
     * The following call should probably be a call to Tk_SizeOfBitmap
     * instead, but it seems that we are occasionally invoked by custom
     * item types that create their own bitmaps without registering them
     * with Tk.  XGetGeometry is a bit slower than Tk_SizeOfBitmap, but
     * it shouldn't matter here.
     */

    XGetGeometry(Tk_Display(tkwin), bitmap, &dummyRoot,
	    (int *) &dummyX, (int *) &dummyY, (unsigned *) &width,
	    (unsigned *) &height, &dummyBorderwidth, &dummyDepth);
    sprintf(string, "%d %d ", width, height);
    Tcl_AppendResult(interp, string, (char *) NULL);
    if (Tk_PostscriptBitmap(interp, tkwin, psInfo, bitmap, 0, 0,
	    width, height) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_AppendResult(interp, " StippleFill\n", (char *) NULL);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptY --
 *
 *	Given a y-coordinate in local coordinates, this procedure
 *	returns a y-coordinate to use for Postscript output.
 *
 * Results:
 *	Returns the Postscript coordinate that corresponds to
 *	"y".
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
Tk_PostscriptY(y, psInfo)
    double y;				/* Y-coordinate in canvas coords. */
    Tk_PostscriptInfo psInfo;		/* Postscript info */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;

    return psInfoPtr->y2 - y;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptPath --
 *
 *	Given an array of points for a path, generate Postscript
 *	commands to create the path.
 *
 * Results:
 *	Postscript commands get appended to what's in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
Tk_PostscriptPath(interp, psInfo, coordPtr, numPoints)
    Tcl_Interp *interp;
    Tk_PostscriptInfo psInfo;		/* Canvas on whose behalf Postscript
					 * is being generated. */
    double *coordPtr;			/* Pointer to first in array of
					 * 2*numPoints coordinates giving
					 * points for path. */
    int numPoints;			/* Number of points at *coordPtr. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    char buffer[200];

    if (psInfoPtr->prepass) {
	return;
    }
    sprintf(buffer, "%.15g %.15g moveto\n", coordPtr[0],
	    Tk_PostscriptY(coordPtr[1], psInfo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);
    for (numPoints--, coordPtr += 2; numPoints > 0;
	    numPoints--, coordPtr += 2) {
	sprintf(buffer, "%.15g %.15g lineto\n", coordPtr[0],
		Tk_PostscriptY(coordPtr[1], psInfo));
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    }
}

/*
 *--------------------------------------------------------------
 *
 * GetPostscriptPoints --
 *
 *	Given a string, returns the number of Postscript points
 *	corresponding to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result.  If
 *	TCL_OK is returned, then everything went well and the
 *	screen distance is stored at *doublePtr;  otherwise
 *	TCL_ERROR is returned and an error message is left in
 *	the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetPostscriptPoints(interp, string, doublePtr)
    Tcl_Interp *interp;		/* Use this for error reporting. */
    char *string;		/* String describing a screen distance. */
    double *doublePtr;		/* Place to store converted result. */
{
    char *end;
    double d;

    d = strtod(string, &end);
    if (end == string) {
	error:
	Tcl_AppendResult(interp, "bad distance \"", string,
		"\"", (char *) NULL);
	return TCL_ERROR;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    switch (*end) {
	case 'c':
	    d *= 72.0/2.54;
	    end++;
	    break;
	case 'i':
	    d *= 72.0;
	    end++;
	    break;
	case 'm':
	    d *= 72.0/25.4;
	    end++;
	    break;
	case 0:
	    break;
	case 'p':
	    end++;
	    break;
	default:
	    goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    if (*end != 0) {
	goto error;
    }
    *doublePtr = d;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkImageGetColor --
 *
 *	This procedure converts a pixel value to three floating
 *      point numbers, representing the amount of red, green, and 
 *      blue in that pixel on the screen.  It makes use of colormap
 *      data passed as an argument, and should work for all Visual
 *      types.
 *
 *	This implementation is bogus on Windows because the colormap
 *	data is never filled in.  Instead all postscript generated
 *	data coming through here is expected to be RGB color data.
 *	To handle lower bit-depth images properly, XQueryColors
 *	must be implemented for Windows.
 *
 * Results:
 *	Returns red, green, and blue color values in the range 
 *      0 to 1.  There are no error returns.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
#ifdef WIN32
#include <windows.h>

/*
 * We could just define these instead of pulling in windows.h.
 #define GetRValue(rgb)	((BYTE)(rgb))
 #define GetGValue(rgb)	((BYTE)(((WORD)(rgb)) >> 8))
 #define GetBValue(rgb)	((BYTE)((rgb)>>16))
*/

static void
TkImageGetColor(cdata, pixel, red, green, blue)
    TkColormapData *cdata;              /* Colormap data */
    unsigned long pixel;                /* Pixel value to look up */
    double *red, *green, *blue;         /* Color data to return */
{
    *red   = (double) GetRValue(pixel) / 255.0;
    *green = (double) GetGValue(pixel) / 255.0;
    *blue  = (double) GetBValue(pixel) / 255.0;
}
#else
static void
TkImageGetColor(cdata, pixel, red, green, blue)
    TkColormapData *cdata;              /* Colormap data */
    unsigned long pixel;                /* Pixel value to look up */
    double *red, *green, *blue;         /* Color data to return */
{
    if (cdata->separated) {
	int r = (pixel & cdata->red_mask) >> cdata->red_shift;
	int g = (pixel & cdata->green_mask) >> cdata->green_shift;
	int b = (pixel & cdata->blue_mask) >> cdata->blue_shift;
	*red   = cdata->colors[r].red / 65535.0;
	*green = cdata->colors[g].green / 65535.0;
	*blue  = cdata->colors[b].blue / 65535.0;
    } else {
	*red   = cdata->colors[pixel].red / 65535.0;
	*green = cdata->colors[pixel].green / 65535.0;
	*blue  = cdata->colors[pixel].blue / 65535.0;
    }
}
#endif

/*
 *--------------------------------------------------------------
 *
 * TkPostscriptImage --
 *
 *	This procedure is called to output the contents of an
 *	image in Postscript, using a format appropriate for the 
 *      current color mode (i.e. one bit per pixel in monochrome, 
 *      one byte per pixel in gray, and three bytes per pixel in
 *      color).
 *
 * Results:
 *	Returns a standard Tcl return value.  If an error occurs
 *	then an error message will be left in interp->result.
 *	If no error occurs, then additional Postscript will be
 *	appended to interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkPostscriptImage(interp, tkwin, psInfo, ximage, x, y, width, height)
    Tcl_Interp *interp;
    Tk_Window tkwin;
    Tk_PostscriptInfo psInfo;	/* postscript info */
    XImage *ximage;		/* Image to draw */
    int x, y;			/* First pixel to output */
    int width, height;		/* Width and height of area */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    char buffer[256];
    int xx, yy, band, maxRows;
    double red, green, blue;
    int bytesPerLine=0, maxWidth=0;
    int level = psInfoPtr->colorLevel;
    Colormap cmap;
    int i, ncolors;
    Visual *visual;
    TkColormapData cdata;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    cmap = Tk_Colormap(tkwin);
    visual = Tk_Visual(tkwin);

    /*
     * Obtain information about the colormap, ie the mapping between
     * pixel values and RGB values.  The code below should work
     * for all Visual types.
     */

    ncolors = visual->map_entries;
    cdata.colors = (XColor *) ckalloc(sizeof(XColor) * ncolors);
    cdata.ncolors = ncolors;

    if (visual->class == DirectColor || visual->class == TrueColor) {
	cdata.separated = 1;
	cdata.red_mask = visual->red_mask;
	cdata.green_mask = visual->green_mask;
	cdata.blue_mask = visual->blue_mask;
	cdata.red_shift = 0;
	cdata.green_shift = 0;
	cdata.blue_shift = 0;
	while ((0x0001 & (cdata.red_mask >> cdata.red_shift)) == 0)
	    cdata.red_shift ++;
	while ((0x0001 & (cdata.green_mask >> cdata.green_shift)) == 0)
	    cdata.green_shift ++;
	while ((0x0001 & (cdata.blue_mask >> cdata.blue_shift)) == 0)
	    cdata.blue_shift ++;
	for (i = 0; i < ncolors; i ++)
	    cdata.colors[i].pixel =
		((i << cdata.red_shift) & cdata.red_mask) |
		((i << cdata.green_shift) & cdata.green_mask) |
		((i << cdata.blue_shift) & cdata.blue_mask);
    } else {
	cdata.separated=0;
	for (i = 0; i < ncolors; i ++)
	    cdata.colors[i].pixel = i;
    }
    if (visual->class == StaticGray || visual->class == GrayScale)
	cdata.color = 0;
    else
	cdata.color = 1;

    XQueryColors(Tk_Display(tkwin), cmap, cdata.colors, ncolors);

    /*
     * Figure out which color level to use (possibly lower than the 
     * one specified by the user).  For example, if the user specifies
     * color with monochrome screen, use gray or monochrome mode instead. 
     */

    if (!cdata.color && level == 2) {
	level = 1;
    }

    if (!cdata.color && cdata.ncolors == 2) {
	level = 0;
    }

    /*
     * Check that at least one row of the image can be represented
     * with a string less than 64 KB long (this is a limit in the 
     * Postscript interpreter).
     */

    switch (level) {
	case 0: bytesPerLine = (width + 7) / 8;  maxWidth = 240000;  break;
	case 1: bytesPerLine = width;  maxWidth = 60000;  break;
	case 2: bytesPerLine = 3 * width;  maxWidth = 20000;  break;
    }

    if (bytesPerLine > 60000) {
	Tcl_ResetResult(interp);
	sprintf(buffer,
		"Can't generate Postscript for images more than %d pixels wide",
		maxWidth);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	ckfree((char *) cdata.colors);
	return TCL_ERROR;
    }

    maxRows = 60000 / bytesPerLine;

    for (band = height-1; band >= 0; band -= maxRows) {
	int rows = (band >= maxRows) ? maxRows : band + 1;
	int lineLen = 0;
	switch (level) {
	    case 0:
		sprintf(buffer, "%d %d 1 matrix {\n<", width, rows);
		Tcl_AppendResult(interp, buffer, (char *) NULL);
		break;
	    case 1:
		sprintf(buffer, "%d %d 8 matrix {\n<", width, rows);
		Tcl_AppendResult(interp, buffer, (char *) NULL);
		break;
	    case 2:
		sprintf(buffer, "%d %d 8 matrix {\n<",
			width, rows);
		Tcl_AppendResult(interp, buffer, (char *) NULL);
		break;
	}
	for (yy = band; yy > band - rows; yy--) {
	    switch (level) {
		case 0: {
		    /*
		     * Generate data for image in monochrome mode.
		     * No attempt at dithering is made--instead, just
		     * set a threshold.
		     */
		    unsigned char mask=0x80;
		    unsigned char data=0x00;
		    for (xx = x; xx< x+width; xx++) {
			TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
					&red, &green, &blue);
			if (0.30 * red + 0.59 * green + 0.11 * blue > 0.5)
			    data |= mask;
			mask >>= 1;
			if (mask == 0) {
			    sprintf(buffer, "%02X", data);
			    Tcl_AppendResult(interp, buffer, (char *) NULL);
			    lineLen += 2;
			    if (lineLen > 60) {
			        lineLen = 0;
			        Tcl_AppendResult(interp, "\n", (char *) NULL);
			    }
			    mask=0x80;
			    data=0x00;
			}
		    }
		    if ((width % 8) != 0) {
		        sprintf(buffer, "%02X", data);
		        Tcl_AppendResult(interp, buffer, (char *) NULL);
		        mask=0x80;
		        data=0x00;
		    }
		    break;
		}
		case 1: {
		    /*
		     * Generate data in gray mode--in this case, take a 
		     * weighted sum of the red, green, and blue values.
		     */
		    for (xx = x; xx < x+width; xx ++) {
			TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
					&red, &green, &blue);
			sprintf(buffer, "%02X", (int) floor(0.5 + 255.0 *
				(0.30 * red + 0.59 * green + 0.11 * blue)));
			Tcl_AppendResult(interp, buffer, (char *) NULL);
			lineLen += 2;
			if (lineLen > 60) {
			    lineLen = 0;
			    Tcl_AppendResult(interp, "\n", (char *) NULL);
			}
		    }
		    break;
		}
		case 2: {
		    /*
		     * Finally, color mode.  Here, just output the red, green,
		     * and blue values directly.
		     */
		    for (xx = x; xx < x+width; xx++) {
			TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
				&red, &green, &blue);
			sprintf(buffer, "%02X%02X%02X",
				(int) floor(0.5 + 255.0 * red),
				(int) floor(0.5 + 255.0 * green),
				(int) floor(0.5 + 255.0 * blue));
			Tcl_AppendResult(interp, buffer, (char *) NULL);
			lineLen += 6;
			if (lineLen > 60) {
			    lineLen = 0;
			    Tcl_AppendResult(interp, "\n", (char *) NULL);
			}
		    }
		    break;
		}
	    }
	}
	switch (level) {
	    case 0: sprintf(buffer, ">\n} image\n"); break;
	    case 1: sprintf(buffer, ">\n} image\n"); break;
	    case 2: sprintf(buffer, ">\n} false 3 colorimage\n"); break;
	}
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	sprintf(buffer, "0 %d translate\n", rows);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    }
    ckfree((char *) cdata.colors);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptPhoto --
 *
 *	This procedure is called to output the contents of a
 *	photo image in Postscript, using a format appropriate for
 *	the requested postscript color mode (i.e. one byte per pixel
 *	in gray, and three bytes per pixel in color).
 *
 * Results:
 *	Returns a standard Tcl return value.  If an error occurs
 *	then an error message will be left in interp->result.
 *	If no error occurs, then additional Postscript will be
 *	appended to the interpreter's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
int
Tk_PostscriptPhoto(interp, blockPtr, psInfo, width, height)
    Tcl_Interp *interp;
    Tk_PhotoImageBlock *blockPtr;
    Tk_PostscriptInfo psInfo;
    int width, height;
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    int colorLevel = psInfoPtr->colorLevel;
    static int codeIncluded = 0;

    unsigned char *pixelPtr;
    char buffer[256], cspace[40], decode[40];
    int bpc;
    int xx, yy, lineLen;
    float red, green, blue;
    int alpha;
    int bytesPerLine=0, maxWidth=0;

    unsigned char opaque = 255;
    unsigned char *alphaPtr;
    int alphaOffset, alphaPitch, alphaIncr;

    if (psInfoPtr->prepass) {
	codeIncluded = 0;
	return TCL_OK;
    }

    /*
     * Define the "TkPhoto" function, which is a modified version
     * of the original "transparentimage" function posted
     * by ian@five-d.com (Ian Kemmish) to comp.lang.postscript.
     * For a monochrome colorLevel this is a slightly different
     * version that uses the imagemask command instead of image.
     */

    if( !codeIncluded && (colorLevel != 0) ) {
	/*
	 * Color and gray-scale code.
	 */

	codeIncluded = !0;
	Tcl_AppendResult( interp,
		"/TkPhoto { \n",
		"  gsave \n",
		"  32 dict begin \n",
		"  /tinteger exch def \n",
		"  /transparent 1 string def \n",
		"  transparent 0 tinteger put \n",
		"  /olddict exch def \n",
		"  olddict /DataSource get dup type /filetype ne { \n",
		"    olddict /DataSource 3 -1 roll \n",
		"    0 () /SubFileDecode filter put \n",
		"  } { \n",
		"    pop \n",
		"  } ifelse \n",
		"  /newdict olddict maxlength dict def \n",
		"  olddict newdict copy pop \n",
		"  /w newdict /Width get def \n",
		"  /crpp newdict /Decode get length 2 idiv def \n",
		"  /str w string def \n",
		"  /pix w crpp mul string def \n",
		"  /substrlen 2 w log 2 log div floor exp cvi def \n",
		"  /substrs [ \n",
		"  { \n",
		"     substrlen string \n",
		"     0 1 substrlen 1 sub { \n",
		"       1 index exch tinteger put \n",
		"     } for \n",
		"     /substrlen substrlen 2 idiv def \n",
		"     substrlen 0 eq {exit} if \n",
		"  } loop \n",
		"  ] def \n",
		"  /h newdict /Height get def \n",
		"  1 w div 1 h div matrix scale \n",
		"  olddict /ImageMatrix get exch matrix concatmatrix \n",
		"  matrix invertmatrix concat \n",
		"  newdict /Height 1 put \n",
		"  newdict /DataSource pix put \n",
		"  /mat [w 0 0 h 0 0] def \n",
		"  newdict /ImageMatrix mat put \n",
		"  0 1 h 1 sub { \n",
		"    mat 5 3 -1 roll neg put \n",
		"    olddict /DataSource get str readstring pop pop \n",
		"    /tail str def \n",
		"    /x 0 def \n",
		"    olddict /DataSource get pix readstring pop pop \n",
		"    { \n",
		"      tail transparent search dup /done exch not def \n",
		"      {exch pop exch pop} if \n",
		"      /w1 exch length def \n",
		"      w1 0 ne { \n",
		"        newdict /DataSource ",
		          " pix x crpp mul w1 crpp mul getinterval put \n",
		"        newdict /Width w1 put \n",
		"        mat 4 x neg put \n",
		"        /x x w1 add def \n",
		"        newdict image \n",
		"        /tail tail w1 tail length w1 sub getinterval def \n",
		"      } if \n",
		"      done {exit} if \n",
		"      tail substrs { \n",
		"        anchorsearch {pop} if \n",
		"      } forall \n",
		"      /tail exch def \n",
		"      tail length 0 eq {exit} if \n",
		"      /x w tail length sub def \n",
		"    } loop \n",
		"  } for \n",
		"  end \n",
		"  grestore \n",
		"} bind def \n\n\n", (char *) NULL);
    } else if( !codeIncluded && (colorLevel == 0) ) {
	/*
	 * Monochrome-only code
	 */

	codeIncluded = !0;
	Tcl_AppendResult( interp,
		"/TkPhoto { \n",
		"  gsave \n",
		"  32 dict begin \n",
		"  /dummyInteger exch def \n",
		"  /olddict exch def \n",
		"  olddict /DataSource get dup type /filetype ne { \n",
		"    olddict /DataSource 3 -1 roll \n",
		"    0 () /SubFileDecode filter put \n",
		"  } { \n",
		"    pop \n",
		"  } ifelse \n",
		"  /newdict olddict maxlength dict def \n",
		"  olddict newdict copy pop \n",
		"  /w newdict /Width get def \n",
		"  /pix w 7 add 8 idiv string def \n",
		"  /h newdict /Height get def \n",
		"  1 w div 1 h div matrix scale \n",
		"  olddict /ImageMatrix get exch matrix concatmatrix \n",
		"  matrix invertmatrix concat \n",
		"  newdict /Height 1 put \n",
		"  newdict /DataSource pix put \n",
		"  /mat [w 0 0 h 0 0] def \n",
		"  newdict /ImageMatrix mat put \n",
		"  0 1 h 1 sub { \n",
		"    mat 5 3 -1 roll neg put \n",
		"    0.000 0.000 0.000 setrgbcolor \n",
		"    olddict /DataSource get pix readstring pop pop \n",
		"    newdict /DataSource pix put \n",
		"    newdict imagemask \n",
		"    1.000 1.000 1.000 setrgbcolor \n",
		"    olddict /DataSource get pix readstring pop pop \n",
		"    newdict /DataSource pix put \n",
		"    newdict imagemask \n",
		"  } for \n",
		"  end \n",
		"  grestore \n",
		"} bind def \n\n\n", (char *) NULL);
    }

    /*
     * Check that at least one row of the image can be represented
     * with a string less than 64 KB long (this is a limit in the
     * Postscript interpreter).
     */

    switch (colorLevel)
	{
	    case 0: bytesPerLine = (width + 7) / 8;  maxWidth = 240000;  break;
	    case 1: bytesPerLine = width;  maxWidth = 60000;  break;
	    case 2: bytesPerLine = 3 * width;  maxWidth = 20000;  break;
	}
    if (bytesPerLine > 60000) {
	Tcl_ResetResult(interp);
	sprintf(buffer,
		"Can't generate Postscript for images more than %d pixels wide",
		maxWidth);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Set up the postscript code except for the image-data stream.
     */

    switch (colorLevel) {
	case 0: 
	    strcpy( cspace, "/DeviceGray");
	    strcpy( decode, "[1 0]");
	    bpc = 1;
	    break;
	case 1: 
	    strcpy( cspace, "/DeviceGray");
	    strcpy( decode, "[0 1]");
	    bpc = 8;
	    break;
	default:
	    strcpy( cspace, "/DeviceRGB");
	    strcpy( decode, "[0 1 0 1 0 1]");
	    bpc = 8;
	    break;
    }


    Tcl_AppendResult(interp,
	    cspace, " setcolorspace\n\n", (char *) NULL);

    sprintf(buffer,
	    "  /Width %d\n  /Height %d\n  /BitsPerComponent %d\n",
	    width, height,  bpc);
    Tcl_AppendResult(interp,
	    "<<\n  /ImageType 1\n", buffer,
	    "  /DataSource currentfile",
	    "  /ASCIIHexDecode filter\n", (char *) NULL);


    sprintf(buffer,
	    "  /ImageMatrix [1 0 0 -1 0 %d]\n", height);
    Tcl_AppendResult(interp, buffer,
	    "  /Decode ", decode, "\n>>\n1 TkPhoto\n", (char *) NULL);


    /*
     * Check the PhotoImageBlock information.
     * We assume that:
     *     if pixelSize is 1,2 or 4, the image is R,G,B,A;
     *     if pixelSize is 3, the image is R,G,B and offset[3] is bogus.
     */

    if (blockPtr->pixelSize == 3) {
	/*
	 * No alpha information: the whole image is opaque.
	 */

	alphaPtr = &opaque;
	alphaPitch = alphaIncr = alphaOffset = 0;
    } else {
	/*
	 * Set up alpha handling.
	 */

	alphaPtr = blockPtr->pixelPtr;
	alphaPitch = blockPtr->pitch;
	alphaIncr = blockPtr->pixelSize;
	alphaOffset = blockPtr->offset[3];
    }


    for (yy = 0, lineLen=0; yy < height; yy++) {
	switch (colorLevel) {
	    case 0: {
		/*
		 * Generate data for image in monochrome mode.
		 * No attempt at dithering is made--instead, just
		 * set a threshold.
		 * To handle transparecies we need to output two lines:
		 * one for the black pixels, one for the white ones.
		 */

		unsigned char mask=0x80;
		unsigned char data=0x00;
		for (xx = 0; xx< width; xx ++) {
		    pixelPtr = blockPtr->pixelPtr 
			+ (yy * blockPtr->pitch) 
			+ (xx *blockPtr->pixelSize);

		    red = pixelPtr[blockPtr->offset[0]];
		    green = pixelPtr[blockPtr->offset[1]];
		    blue = pixelPtr[blockPtr->offset[2]];

		    alpha = *(alphaPtr + (yy * alphaPitch)
			    + (xx * alphaIncr) + alphaOffset);

		    /*
		     * If pixel is less than threshold, then it is black.
		     */

		    if ((alpha != 0) && 
			    ( 0.3086 * red 
				    + 0.6094 * green 
				    + 0.082 * blue < 128)) {
			data |= mask;
		    }
		    mask >>= 1;
		    if (mask == 0) {
			sprintf(buffer, "%02X", data);
			Tcl_AppendResult(interp, buffer, (char *) NULL);
			lineLen += 2;
			if (lineLen >= 60) {
			    lineLen = 0;
			    Tcl_AppendResult(interp, "\n", (char *) NULL);
			}
			mask=0x80;
			data=0x00;
		    }
		}
		if ((width % 8) != 0) {
		    sprintf(buffer, "%02X", data);
		    Tcl_AppendResult(interp, buffer, (char *) NULL);
		    mask=0x80;
		    data=0x00;
		}

		mask=0x80;
		data=0x00;
		for (xx = 0; xx< width; xx ++) {
		    pixelPtr = blockPtr->pixelPtr 
			+ (yy * blockPtr->pitch) 
			+ (xx *blockPtr->pixelSize);

		    red = pixelPtr[blockPtr->offset[0]];
		    green = pixelPtr[blockPtr->offset[1]];
		    blue = pixelPtr[blockPtr->offset[2]];

		    alpha = *(alphaPtr + (yy * alphaPitch)
			    + (xx * alphaIncr) + alphaOffset);
			    
		    /*
		     * If pixel is greater than threshold, then it is white.
		     */

		    if ((alpha != 0) && 
			    (  0.3086 * red 
				    + 0.6094 * green 
				    + 0.082 * blue >= 128)) {
			data |= mask;
		    }
		    mask >>= 1;
		    if (mask == 0) {
			sprintf(buffer, "%02X", data);
			Tcl_AppendResult(interp, buffer, (char *) NULL);
			lineLen += 2;
			if (lineLen >= 60) {
			    lineLen = 0;
			    Tcl_AppendResult(interp, "\n", (char *) NULL);
			}
			mask=0x80;
			data=0x00;
		    }
		}
		if ((width % 8) != 0) {
		    sprintf(buffer, "%02X", data);
		    Tcl_AppendResult(interp, buffer, (char *) NULL);
		    mask=0x80;
		    data=0x00;
		}
		break;
	    }
	    case 1: {
		/*
		 * Generate transparency data.
		 * We must prevent a transparent value of 0
		 * because of a bug in some HP printers.
		 */

		for (xx = 0; xx < width; xx ++) {
		    alpha = *(alphaPtr + (yy * alphaPitch)
			    + (xx * alphaIncr) + alphaOffset);
		    sprintf(buffer, "%02X", alpha | 0x01);
		    Tcl_AppendResult(interp, buffer, (char *) NULL);
		    lineLen += 2;
		    if (lineLen >= 60) {
			lineLen = 0;
			Tcl_AppendResult(interp, "\n", (char *) NULL);
		    }
		}


		/*
		 * Generate data in gray mode--in this case, take a 
		 * weighted sum of the red, green, and blue values.
		 */

		for (xx = 0; xx < width; xx ++) {
		    pixelPtr = blockPtr->pixelPtr 
			+ (yy * blockPtr->pitch) 
			+ (xx *blockPtr->pixelSize);

		    red = pixelPtr[blockPtr->offset[0]];
		    green = pixelPtr[blockPtr->offset[1]];
		    blue = pixelPtr[blockPtr->offset[2]];

		    sprintf(buffer, "%02X", (int) floor(0.5 +
			    ( 0.3086 * red + 0.6094 * green + 0.0820 * blue)));
		    Tcl_AppendResult(interp, buffer, (char *) NULL);
		    lineLen += 2;
		    if (lineLen >= 60) {
			lineLen = 0;
			Tcl_AppendResult(interp, "\n", (char *) NULL);
		    }
		}
		break;
	    }
	    default: {
		/*
		 * Generate transparency data.
		 * We must prevent a transparent value of 0
		 * because of a bug in some HP printers.
		 */

		for (xx = 0; xx < width; xx ++) {
		    alpha = *(alphaPtr + (yy * alphaPitch)
			    + (xx * alphaIncr) + alphaOffset);
		    sprintf(buffer, "%02X", alpha | 0x01);
		    Tcl_AppendResult(interp, buffer, (char *) NULL);
		    lineLen += 2;
		    if (lineLen >= 60) {
			lineLen = 0;
			Tcl_AppendResult(interp, "\n", (char *) NULL);
		    }
		}


		/*
		 * Finally, color mode.  Here, just output the red, green,
		 * and blue values directly.
		 */

		for (xx = 0; xx < width; xx ++) {
		    pixelPtr = blockPtr->pixelPtr 
			+ (yy * blockPtr->pitch) 
			+ (xx *blockPtr->pixelSize);

		    sprintf(buffer, "%02X%02X%02X",
			    pixelPtr[blockPtr->offset[0]],
			    pixelPtr[blockPtr->offset[1]],
			    pixelPtr[blockPtr->offset[2]]);
		    Tcl_AppendResult(interp, buffer, (char *) NULL);
		    lineLen += 6;
		    if (lineLen >= 60) {
			lineLen = 0;
			Tcl_AppendResult(interp, "\n", (char *) NULL);
		    }
		}
		break;
	    }
	}
    }

    Tcl_AppendResult(interp, ">\n", (char *) NULL);
    return TCL_OK;
}
