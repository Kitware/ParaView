/* 
 * tkImage.c --
 *
 *	This file contains code that allows images to be
 *	nested inside text widgets.  It also implements the "image"
 *	widget command for texts.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tk.h"
#include "tkText.h"
#include "tkPort.h"

/*
 * Definitions for alignment values:
 */

#define ALIGN_BOTTOM		0
#define ALIGN_CENTER		1
#define ALIGN_TOP		2
#define ALIGN_BASELINE		3

/*
 * Macro that determines the size of an embedded image segment:
 */

#define EI_SEG_SIZE ((unsigned) (Tk_Offset(TkTextSegment, body) \
	+ sizeof(TkTextEmbImage)))

/*
 * Prototypes for procedures defined in this file:
 */

static int		AlignParseProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin, char *value,
			    char *widgRec, int offset));
static char *		AlignPrintProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin, char *widgRec, int offset,
			    Tcl_FreeProc **freeProcPtr));
static TkTextSegment *	EmbImageCleanupProc _ANSI_ARGS_((TkTextSegment *segPtr,
			    TkTextLine *linePtr));
static void		EmbImageCheckProc _ANSI_ARGS_((TkTextSegment *segPtr,
			    TkTextLine *linePtr));
static void		EmbImageBboxProc _ANSI_ARGS_((TkTextDispChunk *chunkPtr,
			    int index, int y, int lineHeight, int baseline,
			    int *xPtr, int *yPtr, int *widthPtr,
			    int *heightPtr));
static int		EmbImageConfigure _ANSI_ARGS_((TkText *textPtr,
			    TkTextSegment *eiPtr, int argc, char **argv));
static int		EmbImageDeleteProc _ANSI_ARGS_((TkTextSegment *segPtr,
			    TkTextLine *linePtr, int treeGone));
static void		EmbImageDisplayProc _ANSI_ARGS_((
			    TkTextDispChunk *chunkPtr, int x, int y,
			    int lineHeight, int baseline, Display *display,
			    Drawable dst, int screenY));
static int		EmbImageLayoutProc _ANSI_ARGS_((TkText *textPtr,
			    TkTextIndex *indexPtr, TkTextSegment *segPtr,
			    int offset, int maxX, int maxChars,
			    int noCharsYet, Tk_Uid wrapMode,
			    TkTextDispChunk *chunkPtr));
static void		EmbImageProc _ANSI_ARGS_((ClientData clientData,
			    int x, int y, int width, int height,
			    int imageWidth, int imageHeight));

/*
 * The following structure declares the "embedded image" segment type.
 */

static Tk_SegType tkTextEmbImageType = {
    "image",					/* name */
    0,						/* leftGravity */
    (Tk_SegSplitProc *) NULL,			/* splitProc */
    EmbImageDeleteProc,				/* deleteProc */
    EmbImageCleanupProc,			/* cleanupProc */
    (Tk_SegLineChangeProc *) NULL,		/* lineChangeProc */
    EmbImageLayoutProc,				/* layoutProc */
    EmbImageCheckProc				/* checkProc */
};

/*
 * Information used for parsing image configuration options:
 */

static Tk_CustomOption alignOption = {AlignParseProc, AlignPrintProc,
	(ClientData) NULL};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_CUSTOM, "-align", (char *) NULL, (char *) NULL,
	"center", 0, TK_CONFIG_DONT_SET_DEFAULT, &alignOption},
    {TK_CONFIG_PIXELS, "-padx", (char *) NULL, (char *) NULL,
	"0", Tk_Offset(TkTextEmbImage, padX),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_PIXELS, "-pady", (char *) NULL, (char *) NULL,
	"0", Tk_Offset(TkTextEmbImage, padY),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_STRING, "-image", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextEmbImage, imageString),
	TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-name", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextEmbImage, imageName),
	TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 *--------------------------------------------------------------
 *
 * TkTextImageCmd --
 *
 *	This procedure implements the "image" widget command
 *	for text widgets.  See the user documentation for details
 *	on what it does.
 *
 * Results:
 *	A standard Tcl result or error.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
TkTextImageCmd(textPtr, interp, argc, argv)
    register TkText *textPtr;	/* Information about text widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings.  Someone else has already
				 * parsed this command enough to know that
				 * argv[1] is "image". */
{
    size_t length;
    register TkTextSegment *eiPtr;

    if (argc < 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " image option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    length = strlen(argv[2]);
    if ((strncmp(argv[2], "cget", length) == 0) && (length >= 2)) {
	TkTextIndex index;
	TkTextSegment *eiPtr;

	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " image cget index option\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, argv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	eiPtr = TkTextIndexToSeg(&index, (int *) NULL);
	if (eiPtr->typePtr != &tkTextEmbImageType) {
	    Tcl_AppendResult(interp, "no embedded image at index \"",
		    argv[3], "\"", (char *) NULL);
	    return TCL_ERROR;
	}
	return Tk_ConfigureValue(interp, textPtr->tkwin, configSpecs,
		(char *) &eiPtr->body.ei, argv[4], 0);
    } else if ((strncmp(argv[2], "configure", length) == 0) && (length >= 2)) {
	TkTextIndex index;
	TkTextSegment *eiPtr;

	if (argc < 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " image configure index ?option value ...?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, argv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	eiPtr = TkTextIndexToSeg(&index, (int *) NULL);
	if (eiPtr->typePtr != &tkTextEmbImageType) {
	    Tcl_AppendResult(interp, "no embedded image at index \"",
		    argv[3], "\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 4) {
	    return Tk_ConfigureInfo(interp, textPtr->tkwin, configSpecs,
		    (char *) &eiPtr->body.ei, (char *) NULL, 0);
	} else if (argc == 5) {
	    return Tk_ConfigureInfo(interp, textPtr->tkwin, configSpecs,
		    (char *) &eiPtr->body.ei, argv[4], 0);
	} else {
	    TkTextChanged(textPtr, &index, &index);
	    return EmbImageConfigure(textPtr, eiPtr, argc-4, argv+4);
	}
    } else if ((strncmp(argv[2], "create", length) == 0) && (length >= 2)) {
	TkTextIndex index;
	int lineIndex;

	/*
	 * Add a new image.  Find where to put the new image, and
	 * mark that position for redisplay.
	 */

	if (argc < 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " image create index ?option value ...?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, argv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * Don't allow insertions on the last (dummy) line of the text.
	 */
    
	lineIndex = TkBTreeLineIndex(index.linePtr);
	if (lineIndex == TkBTreeNumLines(textPtr->tree)) {
	    lineIndex--;
	    TkTextMakeByteIndex(textPtr->tree, lineIndex, 1000000, &index);
	}

	/*
	 * Create the new image segment and initialize it.
	 */

	eiPtr = (TkTextSegment *) ckalloc(EI_SEG_SIZE);
	eiPtr->typePtr = &tkTextEmbImageType;
	eiPtr->size = 1;
	eiPtr->body.ei.textPtr = textPtr;
	eiPtr->body.ei.linePtr = NULL;
	eiPtr->body.ei.imageName = NULL;
	eiPtr->body.ei.imageString = NULL;
	eiPtr->body.ei.name = NULL;
	eiPtr->body.ei.image = NULL;
	eiPtr->body.ei.align = ALIGN_CENTER;
	eiPtr->body.ei.padX = eiPtr->body.ei.padY = 0;
	eiPtr->body.ei.chunkCount = 0;

	/*
	 * Link the segment into the text widget, then configure it (delete
	 * it again if the configuration fails).
	 */

	TkTextChanged(textPtr, &index, &index);
	TkBTreeLinkSegment(eiPtr, &index);
	if (EmbImageConfigure(textPtr, eiPtr, argc-4, argv+4) != TCL_OK) {
	    TkTextIndex index2;

	    TkTextIndexForwChars(&index, 1, &index2);
	    TkBTreeDeleteChars(&index, &index2);
	    return TCL_ERROR;
	}
    } else if (strncmp(argv[2], "names", length) == 0) {
	Tcl_HashSearch search;
	Tcl_HashEntry *hPtr;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " image names\"", (char *) NULL);
	    return TCL_ERROR;
	}
	for (hPtr = Tcl_FirstHashEntry(&textPtr->imageTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_AppendElement(interp,
		    Tcl_GetHashKey(&textPtr->markTable, hPtr));
	}
    } else {
	Tcl_AppendResult(interp, "bad image option \"", argv[2],
		"\": must be cget, configure, create, or names",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageConfigure --
 *
 *	This procedure is called to handle configuration options
 *	for an embedded image, using an argc/argv list.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message..
 *
 * Side effects:
 *	Configuration information for the embedded image changes,
 *	such as alignment, or name of the image.
 *
 *--------------------------------------------------------------
 */

static int
EmbImageConfigure(textPtr, eiPtr, argc, argv)
    TkText *textPtr;		/* Information about text widget that
				 * contains embedded image. */
    TkTextSegment *eiPtr;	/* Embedded image to be configured. */
    int argc;			/* Number of strings in argv. */
    char **argv;		/* Array of strings describing configuration
				 * options. */
{
    Tk_Image image;
    Tcl_DString newName;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    int new;
    char *name;
    int count = 0;		/* The counter for picking a unique name */
    int conflict = 0;		/* True if we have a name conflict */
    unsigned int len;		/* length of image name */

    if (Tk_ConfigureWidget(textPtr->interp, textPtr->tkwin, configSpecs,
	    argc, argv, (char *) &eiPtr->body.ei,TK_CONFIG_ARGV_ONLY)
	    != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Create the image.  Save the old image around and don't free it
     * until after the new one is allocated.  This keeps the reference
     * count from going to zero so the image doesn't have to be recreated
     * if it hasn't changed.
     */

    if (eiPtr->body.ei.imageString != NULL) {
	image = Tk_GetImage(textPtr->interp, textPtr->tkwin, eiPtr->body.ei.imageString,
		EmbImageProc, (ClientData) eiPtr);
	if (image == NULL) {
	    return TCL_ERROR;
	}
    } else {
	image = NULL;
    }
    if (eiPtr->body.ei.image != NULL) {
	Tk_FreeImage(eiPtr->body.ei.image);
    }
    eiPtr->body.ei.image = image;

    if (eiPtr->body.ei.name != NULL) {
    	return TCL_OK;
    }

    /* 
     * Find a unique name for this image.  Use imageName (or imageString)
     * if available, otherwise tack on a #nn and use it.  If a name is already
     * associated with this image, delete the name.
     */

    name = eiPtr->body.ei.imageName;
    if (name == NULL) {
    	name = eiPtr->body.ei.imageString;
    }
    if (name == NULL) {
        Tcl_AppendResult(textPtr->interp,"Either a \"-name\" ",
		"or a \"-image\" argument must be provided ",
		"to the \"image create\" subcommand.",
		(char *) NULL);
	return TCL_ERROR;
    }
    len = strlen(name);
    for (hPtr = Tcl_FirstHashEntry(&textPtr->imageTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	char *haveName = Tcl_GetHashKey(&textPtr->imageTable, hPtr);
	if (strncmp(name, haveName, len) == 0) {
	    new = 0;
	    sscanf(haveName+len,"#%d",&new);
	    if (new > count) {
		count = new;
	    }
	    if (len == (int) strlen(haveName)) {
	    	conflict = 1;
	    }
	}
    }

    Tcl_DStringInit(&newName);
    Tcl_DStringAppend(&newName,name, -1);

    if (conflict) {
    	char buf[4 + TCL_INTEGER_SPACE];
	sprintf(buf, "#%d",count+1);
	Tcl_DStringAppend(&newName,buf, -1);
    }
    name = Tcl_DStringValue(&newName);
    hPtr = Tcl_CreateHashEntry(&textPtr->imageTable, name, &new);
    Tcl_SetHashValue(hPtr, eiPtr);
    Tcl_AppendResult(textPtr->interp, name , (char *) NULL);
    eiPtr->body.ei.name = ckalloc((unsigned) Tcl_DStringLength(&newName)+1);
    strcpy(eiPtr->body.ei.name,name);
    Tcl_DStringFree(&newName);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * AlignParseProc --
 *
 *	This procedure is invoked by Tk_ConfigureWidget during
 *	option processing to handle "-align" options for embedded
 *	images.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The alignment for the embedded image may change.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
AlignParseProc(clientData, interp, tkwin, value, widgRec, offset)
    ClientData clientData;		/* Not used.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window for text widget. */
    char *value;			/* Value of option. */
    char *widgRec;			/* Pointer to TkTextEmbWindow
					 * structure. */
    int offset;				/* Offset into item (ignored). */
{
    register TkTextEmbImage *embPtr = (TkTextEmbImage *) widgRec;

    if (strcmp(value, "baseline") == 0) {
	embPtr->align = ALIGN_BASELINE;
    } else if (strcmp(value, "bottom") == 0) {
	embPtr->align = ALIGN_BOTTOM;
    } else if (strcmp(value, "center") == 0) {
	embPtr->align = ALIGN_CENTER;
    } else if (strcmp(value, "top") == 0) {
	embPtr->align = ALIGN_TOP;
    } else {
	Tcl_AppendResult(interp, "bad alignment \"", value,
		"\": must be baseline, bottom, center, or top",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * AlignPrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-align" configuration
 *	option for embedded images.
 *
 * Results:
 *	The return value is a string describing the embedded
 *	images's current alignment.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
AlignPrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window for text widget. */
    char *widgRec;			/* Pointer to TkTextEmbImage
					 * structure. */
    int offset;				/* Ignored. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    switch (((TkTextEmbImage *) widgRec)->align) {
	case ALIGN_BASELINE:
	    return "baseline";
	case ALIGN_BOTTOM:
	    return "bottom";
	case ALIGN_CENTER:
	    return "center";
	case ALIGN_TOP:
	    return "top";
	default:
	    return "??";
    }
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageDeleteProc --
 *
 *	This procedure is invoked by the text B-tree code whenever
 *	an embedded image lies in a range of characters being deleted.
 *
 * Results:
 *	Returns 0 to indicate that the deletion has been accepted.
 *
 * Side effects:
 *	The embedded image is deleted, if it exists, and any resources
 *	associated with it are released.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
EmbImageDeleteProc(eiPtr, linePtr, treeGone)
    TkTextSegment *eiPtr;		/* Segment being deleted. */
    TkTextLine *linePtr;		/* Line containing segment. */
    int treeGone;			/* Non-zero means the entire tree is
					 * being deleted, so everything must
					 * get cleaned up. */
{
    Tcl_HashEntry *hPtr;

    if (eiPtr->body.ei.image != NULL) {
	hPtr = Tcl_FindHashEntry(&eiPtr->body.ei.textPtr->imageTable,
		eiPtr->body.ei.name);
	if (hPtr != NULL) {
	    /*
	     * (It's possible for there to be no hash table entry for this
	     * image, if an error occurred while creating the image segment
	     * but before the image got added to the table)
	     */

	    Tcl_DeleteHashEntry(hPtr);
	}
	Tk_FreeImage(eiPtr->body.ei.image);
    }
    Tk_FreeOptions(configSpecs, (char *) &eiPtr->body.ei,
	    eiPtr->body.ei.textPtr->display, 0);
    if (eiPtr->body.ei.name != NULL) {
	ckfree(eiPtr->body.ei.name);
    }
    ckfree((char *) eiPtr);
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageCleanupProc --
 *
 *	This procedure is invoked by the B-tree code whenever a
 *	segment containing an embedded image is moved from one
 *	line to another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The linePtr field of the segment gets updated.
 *
 *--------------------------------------------------------------
 */

static TkTextSegment *
EmbImageCleanupProc(eiPtr, linePtr)
    TkTextSegment *eiPtr;		/* Mark segment that's being moved. */
    TkTextLine *linePtr;		/* Line that now contains segment. */
{
    eiPtr->body.ei.linePtr = linePtr;
    return eiPtr;
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageLayoutProc --
 *
 *	This procedure is the "layoutProc" for embedded image
 *	segments.
 *
 * Results:
 *	1 is returned to indicate that the segment should be
 *	displayed.  The chunkPtr structure is filled in.
 *
 * Side effects:
 *	None, except for filling in chunkPtr.
 *
 *--------------------------------------------------------------
 */

	/*ARGSUSED*/
static int
EmbImageLayoutProc(textPtr, indexPtr, eiPtr, offset, maxX, maxChars,
	noCharsYet, wrapMode, chunkPtr)
    TkText *textPtr;		/* Text widget being layed out. */
    TkTextIndex *indexPtr;	/* Identifies first character in chunk. */
    TkTextSegment *eiPtr;	/* Segment corresponding to indexPtr. */
    int offset;			/* Offset within segPtr corresponding to
				 * indexPtr (always 0). */
    int maxX;			/* Chunk must not occupy pixels at this
				 * position or higher. */
    int maxChars;		/* Chunk must not include more than this
				 * many characters. */
    int noCharsYet;		/* Non-zero means no characters have been
				 * assigned to this line yet. */
    Tk_Uid wrapMode;		/* Wrap mode to use for line: char, 
				 * text, or word. */
    register TkTextDispChunk *chunkPtr;
				/* Structure to fill in with information
				 * about this chunk.  The x field has already
				 * been set by the caller. */
{
    int width, height;

    if (offset != 0) {
	panic("Non-zero offset in EmbImageLayoutProc");
    }

    /*
     * See if there's room for this image on this line.
     */

    if (eiPtr->body.ei.image == NULL) {
	width = 0;
	height = 0;
    } else {
	Tk_SizeOfImage(eiPtr->body.ei.image, &width, &height);
	width += 2*eiPtr->body.ei.padX;
	height += 2*eiPtr->body.ei.padY;
    }
    if ((width > (maxX - chunkPtr->x))
	    && !noCharsYet && (textPtr->wrapMode != Tk_GetUid("none"))) {
	return 0;
    }

    /*
     * Fill in the chunk structure.
     */

    chunkPtr->displayProc = EmbImageDisplayProc;
    chunkPtr->undisplayProc = (Tk_ChunkUndisplayProc *) NULL;
    chunkPtr->measureProc = (Tk_ChunkMeasureProc *) NULL;
    chunkPtr->bboxProc = EmbImageBboxProc;
    chunkPtr->numBytes = 1;
    if (eiPtr->body.ei.align == ALIGN_BASELINE) {
	chunkPtr->minAscent = height - eiPtr->body.ei.padY;
	chunkPtr->minDescent = eiPtr->body.ei.padY;
	chunkPtr->minHeight = 0;
    } else {
	chunkPtr->minAscent = 0;
	chunkPtr->minDescent = 0;
	chunkPtr->minHeight = height;
    }
    chunkPtr->width = width;
    chunkPtr->breakIndex = -1;
    chunkPtr->breakIndex = 1;
    chunkPtr->clientData = (ClientData) eiPtr;
    eiPtr->body.ei.chunkCount += 1;
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageCheckProc --
 *
 *	This procedure is invoked by the B-tree code to perform
 *	consistency checks on embedded images.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The procedure panics if it detects anything wrong with
 *	the embedded image.
 *
 *--------------------------------------------------------------
 */

static void
EmbImageCheckProc(eiPtr, linePtr)
    TkTextSegment *eiPtr;		/* Segment to check. */
    TkTextLine *linePtr;		/* Line containing segment. */
{
    if (eiPtr->nextPtr == NULL) {
	panic("EmbImageCheckProc: embedded image is last segment in line");
    }
    if (eiPtr->size != 1) {
	panic("EmbImageCheckProc: embedded image has size %d", eiPtr->size);
    }
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageDisplayProc --
 *
 *	This procedure is invoked by the text displaying code
 *	when it is time to actually draw an embedded image
 *	chunk on the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The embedded image gets moved to the correct location
 *	and drawn onto the display.
 *
 *--------------------------------------------------------------
 */

static void
EmbImageDisplayProc(chunkPtr, x, y, lineHeight, baseline, display, dst, screenY)
    TkTextDispChunk *chunkPtr;		/* Chunk that is to be drawn. */
    int x;				/* X-position in dst at which to
					 * draw this chunk (differs from
					 * the x-position in the chunk because
					 * of scrolling). */
    int y;				/* Top of rectangular bounding box
					 * for line: tells where to draw this
					 * chunk in dst (x-position is in
					 * the chunk itself). */
    int lineHeight;			/* Total height of line. */
    int baseline;			/* Offset of baseline from y. */
    Display *display;			/* Display to use for drawing. */
    Drawable dst;			/* Pixmap or window in which to draw */
    int screenY;			/* Y-coordinate in text window that
					 * corresponds to y. */
{
    TkTextSegment *eiPtr = (TkTextSegment *) chunkPtr->clientData;
    int lineX, imageX, imageY, width, height;
    Tk_Image image;

    image = eiPtr->body.ei.image;
    if (image == NULL) {
	return;
    }
    if ((x + chunkPtr->width) <= 0) {
	return;
    }

    /*
     * Compute the image's location and size in the text widget, taking
     * into account the align value for the image.
     */

    EmbImageBboxProc(chunkPtr, 0, y, lineHeight, baseline, &lineX,
	    &imageY, &width, &height);
    imageX = lineX - chunkPtr->x + x;

    Tk_RedrawImage(image, 0, 0, width, height, dst,
	    imageX, imageY);
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageBboxProc --
 *
 *	This procedure is called to compute the bounding box of
 *	the area occupied by an embedded image.
 *
 * Results:
 *	There is no return value.  *xPtr and *yPtr are filled in
 *	with the coordinates of the upper left corner of the
 *	image, and *widthPtr and *heightPtr are filled in with
 *	the dimensions of the image in pixels.  Note:  not all
 *	of the returned bbox is necessarily visible on the screen
 *	(the rightmost part might be off-screen to the right,
 *	and the bottommost part might be off-screen to the bottom).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
EmbImageBboxProc(chunkPtr, index, y, lineHeight, baseline, xPtr, yPtr,
	widthPtr, heightPtr)
    TkTextDispChunk *chunkPtr;		/* Chunk containing desired char. */
    int index;				/* Index of desired character within
					 * the chunk. */
    int y;				/* Topmost pixel in area allocated
					 * for this line. */
    int lineHeight;			/* Total height of line. */
    int baseline;			/* Location of line's baseline, in
					 * pixels measured down from y. */
    int *xPtr, *yPtr;			/* Gets filled in with coords of
					 * character's upper-left pixel. */
    int *widthPtr;			/* Gets filled in with width of
					 * character, in pixels. */
    int *heightPtr;			/* Gets filled in with height of
					 * character, in pixels. */
{
    TkTextSegment *eiPtr = (TkTextSegment *) chunkPtr->clientData;
    Tk_Image image;

    image = eiPtr->body.ei.image;
    if (image != NULL) {
	Tk_SizeOfImage(image, widthPtr, heightPtr);
    } else {
	*widthPtr = 0;
	*heightPtr = 0;
    }
    *xPtr = chunkPtr->x + eiPtr->body.ei.padX;
    switch (eiPtr->body.ei.align) {
	case ALIGN_BOTTOM:
	    *yPtr = y + (lineHeight - *heightPtr - eiPtr->body.ei.padY);
	    break;
	case ALIGN_CENTER:
	    *yPtr = y + (lineHeight - *heightPtr)/2;
	    break;
	case ALIGN_TOP:
	    *yPtr = y + eiPtr->body.ei.padY;
	    break;
	case ALIGN_BASELINE:
	    *yPtr = y + (baseline - *heightPtr);
	    break;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkTextImageIndex --
 *
 *	Given the name of an embedded image within a text widget,
 *	returns an index corresponding to the image's position
 *	in the text.
 *
 * Results:
 *	The return value is 1 if there is an embedded image by
 *	the given name in the text widget, 0 otherwise.  If the
 *	image exists, *indexPtr is filled in with its index.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkTextImageIndex(textPtr, name, indexPtr)
    TkText *textPtr;		/* Text widget containing image. */
    char *name;			/* Name of image. */
    TkTextIndex *indexPtr;	/* Index information gets stored here. */
{
    Tcl_HashEntry *hPtr;
    TkTextSegment *eiPtr;

    hPtr = Tcl_FindHashEntry(&textPtr->imageTable, name);
    if (hPtr == NULL) {
	return 0;
    }
    eiPtr = (TkTextSegment *) Tcl_GetHashValue(hPtr);
    indexPtr->tree = textPtr->tree;
    indexPtr->linePtr = eiPtr->body.ei.linePtr;
    indexPtr->byteIndex = TkTextSegToOffset(eiPtr, indexPtr->linePtr);
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * EmbImageProc --
 *
 *	This procedure is called by the image code whenever an
 *	image or its contents changes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image will be redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
EmbImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;              /* Pointer to widget record. */
    int x, y;                           /* Upper left pixel (within image)
                                         * that must be redisplayed. */
    int width, height;                  /* Dimensions of area to redisplay
                                         * (may be <= 0). */
    int imgWidth, imgHeight;            /* New dimensions of image. */

{
    TkTextSegment *eiPtr = (TkTextSegment *) clientData;
    TkTextIndex index;

    index.tree = eiPtr->body.ei.textPtr->tree;
    index.linePtr = eiPtr->body.ei.linePtr;
    index.byteIndex = TkTextSegToOffset(eiPtr, eiPtr->body.ei.linePtr);
    TkTextChanged(eiPtr->body.ei.textPtr, &index, &index);
}
