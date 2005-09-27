/*
 * tkImgPhoto.c --
 *
 *	Implements images of type "photo" for Tk.  Photo images are
 *	stored in full color (32 bits per pixel including alpha channel)
 *	and displayed using dithering if necessary.
 *
 * Copyright (c) 1994 The Australian National University.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2002 Donal K. Fellows
 * Copyright (c) 2003 ActiveState Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Author: Paul Mackerras (paulus@cs.anu.edu.au),
 *	   Department of Computer Science,
 *	   Australian National University.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkPort.h"
#include "tclMath.h"
#include <ctype.h>

#ifdef __WIN32__
#include "tkWinInt.h"
#endif

/*
 * Declaration for internal Xlib function used here:
 */

extern int _XInitImageFuncPtrs _ANSI_ARGS_((XImage *image));

/*
 * A signed 8-bit integral type.  If chars are unsigned and the compiler
 * isn't an ANSI one, then we have to use short instead (which wastes
 * space) to get signed behavior.
 */

#if defined(__STDC__) || defined(_AIX)
    typedef signed char schar;
#else
#   ifndef __CHAR_UNSIGNED__
	typedef char schar;
#   else
	typedef short schar;
#   endif
#endif

/*
 * An unsigned 32-bit integral type, used for pixel values.
 * We use int rather than long here to accommodate those systems
 * where longs are 64 bits.
 */

typedef unsigned int pixel;

/*
 * The maximum number of pixels to transmit to the server in a
 * single XPutImage call.
 */

#define MAX_PIXELS 65536

/*
 * The set of colors required to display a photo image in a window depends on:
 *	- the visual used by the window
 *	- the palette, which specifies how many levels of each primary
 *	  color to use, and
 *	- the gamma value for the image.
 *
 * Pixel values allocated for specific colors are valid only for the
 * colormap in which they were allocated.  Sets of pixel values
 * allocated for displaying photos are re-used in other windows if
 * possible, that is, if the display, colormap, palette and gamma
 * values match.  A hash table is used to locate these sets of pixel
 * values, using the following data structure as key:
 */

typedef struct {
    Display *display;		/* Qualifies the colormap resource ID */
    Colormap colormap;		/* Colormap that the windows are using. */
    double gamma;		/* Gamma exponent value for images. */
    Tk_Uid palette;		/* Specifies how many shades of each primary
				 * we want to allocate. */
} ColorTableId;

/*
 * For a particular (display, colormap, palette, gamma) combination,
 * a data structure of the following type is used to store the allocated
 * pixel values and other information:
 */

typedef struct ColorTable {
    ColorTableId id;		/* Information used in selecting this
				 * color table. */
    int	flags;			/* See below. */
    int	refCount;		/* Number of instances using this map. */
    int liveRefCount;		/* Number of instances which are actually
				 * in use, using this map. */
    int	numColors;		/* Number of colors allocated for this map. */

    XVisualInfo	visualInfo;	/* Information about the visual for windows
				 * using this color table. */

    pixel redValues[256];	/* Maps 8-bit values of red intensity
				 * to a pixel value or index in pixelMap. */
    pixel greenValues[256];	/* Ditto for green intensity */
    pixel blueValues[256];	/* Ditto for blue intensity */
    unsigned long *pixelMap;	/* Actual pixel values allocated. */

    unsigned char colorQuant[3][256];
				/* Maps 8-bit intensities to quantized
				 * intensities.  The first index is 0 for
				 * red, 1 for green, 2 for blue. */
} ColorTable;

/*
 * Bit definitions for the flags field of a ColorTable.
 * BLACK_AND_WHITE:		1 means only black and white colors are
 *				available.
 * COLOR_WINDOW:		1 means a full 3-D color cube has been
 *				allocated.
 * DISPOSE_PENDING:		1 means a call to DisposeColorTable has
 *				been scheduled as an idle handler, but it
 *				hasn't been invoked yet.
 * MAP_COLORS:			1 means pixel values should be mapped
 *				through pixelMap.
 */
#ifdef COLOR_WINDOW
#undef COLOR_WINDOW
#endif

#define BLACK_AND_WHITE		1
#define COLOR_WINDOW		2
#define DISPOSE_PENDING		4
#define MAP_COLORS		8

/*
 * Definition of the data associated with each photo image master.
 */

typedef struct PhotoMaster {
    Tk_ImageMaster tkMaster;	/* Tk's token for image master.  NULL means
				 * the image is being deleted. */
    Tcl_Interp *interp;		/* Interpreter associated with the
				 * application using this image. */
    Tcl_Command imageCmd;	/* Token for image command (used to delete
				 * it when the image goes away).  NULL means
				 * the image command has already been
				 * deleted. */
    int	flags;			/* Sundry flags, defined below. */
    int	width, height;		/* Dimensions of image. */
    int userWidth, userHeight;	/* User-declared image dimensions. */
    Tk_Uid palette;		/* User-specified default palette for
				 * instances of this image. */
    double gamma;		/* Display gamma value to correct for. */
    char *fileString;		/* Name of file to read into image. */
    Tcl_Obj *dataString;	/* Object to use as contents of image. */
    Tcl_Obj *format;		/* User-specified format of data in image
				 * file or string value. */
    unsigned char *pix32;	/* Local storage for 32-bit image. */
    int ditherX, ditherY;	/* Location of first incorrectly
				 * dithered pixel in image. */
    TkRegion validRegion;	/* Tk region indicating which parts of
				 * the image have valid image data. */
    struct PhotoInstance *instancePtr;
				/* First in the list of instances
				 * associated with this master. */
} PhotoMaster;

/*
 * Bit definitions for the flags field of a PhotoMaster.
 * COLOR_IMAGE:			1 means that the image has different color
 *				components.
 * IMAGE_CHANGED:		1 means that the instances of this image
 *				need to be redithered.
 * COMPLEX_ALPHA:		1 means that the instances of this image
 *				have alpha values that aren't 0 or 255.
 */

#define COLOR_IMAGE		1
#define IMAGE_CHANGED		2
#define COMPLEX_ALPHA		4

/*
 * The following data structure represents all of the instances of
 * a photo image in windows on a given screen that are using the
 * same colormap.
 */

typedef struct PhotoInstance {
    PhotoMaster *masterPtr;	/* Pointer to master for image. */
    Display *display;		/* Display for windows using this instance. */
    Colormap colormap;		/* The image may only be used in windows with
				 * this particular colormap. */
    struct PhotoInstance *nextPtr;
				/* Pointer to the next instance in the list
				 * of instances associated with this master. */
    int refCount;		/* Number of instances using this structure. */
    Tk_Uid palette;		/* Palette for these particular instances. */
    double gamma;		/* Gamma value for these instances. */
    Tk_Uid defaultPalette;	/* Default palette to use if a palette
				 * is not specified for the master. */
    ColorTable *colorTablePtr;	/* Pointer to information about colors
				 * allocated for image display in windows
				 * like this one. */
    Pixmap pixels;		/* X pixmap containing dithered image. */
    int width, height;		/* Dimensions of the pixmap. */
    schar *error;		/* Error image, used in dithering. */
    XImage *imagePtr;		/* Image structure for converted pixels. */
    XVisualInfo visualInfo;	/* Information about the visual that these
				 * windows are using. */
    GC gc;			/* Graphics context for writing images
				 * to the pixmap. */
} PhotoInstance;

/*
 * The following data structure is used to return information
 * from ParseSubcommandOptions:
 */

struct SubcommandOptions {
    int options;		/* Individual bits indicate which
				 * options were specified - see below. */
    Tcl_Obj *name;		/* Name specified without an option. */
    int fromX, fromY;		/* Values specified for -from option. */
    int fromX2, fromY2;		/* Second coordinate pair for -from option. */
    int toX, toY;		/* Values specified for -to option. */
    int toX2, toY2;		/* Second coordinate pair for -to option. */
    int zoomX, zoomY;		/* Values specified for -zoom option. */
    int subsampleX, subsampleY;	/* Values specified for -subsample option. */
    Tcl_Obj *format;		/* Value specified for -format option. */
    XColor *background;		/* Value specified for -background option. */
    int compositingRule;	/* Value specified for -compositingrule opt */
};

/*
 * Bit definitions for use with ParseSubcommandOptions:
 * Each bit is set in the allowedOptions parameter on a call to
 * ParseSubcommandOptions if that option is allowed for the current
 * photo image subcommand.  On return, the bit is set in the options
 * field of the SubcommandOptions structure if that option was specified.
 *
 * OPT_BACKGROUND:		Set if -format option allowed/specified.
 * OPT_COMPOSITE:		Set if -compositingrule option allowed/spec'd.
 * OPT_FORMAT:			Set if -format option allowed/specified.
 * OPT_FROM:			Set if -from option allowed/specified.
 * OPT_GRAYSCALE:		Set if -grayscale option allowed/specified.
 * OPT_SHRINK:			Set if -shrink option allowed/specified.
 * OPT_SUBSAMPLE:		Set if -subsample option allowed/spec'd.
 * OPT_TO:			Set if -to option allowed/specified.
 * OPT_ZOOM:			Set if -zoom option allowed/specified.
 */

#define OPT_BACKGROUND	1
#define OPT_COMPOSITE	2
#define OPT_FORMAT	4
#define OPT_FROM	8
#define OPT_GRAYSCALE	0x10
#define OPT_SHRINK	0x20
#define OPT_SUBSAMPLE	0x40
#define OPT_TO		0x80
#define OPT_ZOOM	0x100

/*
 * List of option names.  The order here must match the order of
 * declarations of the OPT_* constants above.
 */

static char *optionNames[] = {
    "-background",
    "-compositingrule",
    "-format",
    "-from",
    "-grayscale",
    "-shrink",
    "-subsample",
    "-to",
    "-zoom",
    (char *) NULL
};

/*
 * Message to generate when an attempt to resize an image fails due
 * to memory problems.
 */
#define TK_PHOTO_ALLOC_FAILURE_MESSAGE \
	"not enough free memory for image buffer"

/*
 * Functions used in the type record for photo images.
 */

static int		ImgPhotoCreate _ANSI_ARGS_((Tcl_Interp *interp,
			    char *name, int objc, Tcl_Obj *CONST objv[],
			    Tk_ImageType *typePtr, Tk_ImageMaster master,
			    ClientData *clientDataPtr));
static ClientData	ImgPhotoGet _ANSI_ARGS_((Tk_Window tkwin,
			    ClientData clientData));
static void		ImgPhotoDisplay _ANSI_ARGS_((ClientData clientData,
			    Display *display, Drawable drawable,
			    int imageX, int imageY, int width, int height,
			    int drawableX, int drawableY));
static void		ImgPhotoFree _ANSI_ARGS_((ClientData clientData,
			    Display *display));
static void		ImgPhotoDelete _ANSI_ARGS_((ClientData clientData));
static int		ImgPhotoPostscript _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tk_PostscriptInfo psInfo, int x, int y, int width,
			    int height, int prepass));

/*
 * The type record itself for photo images:
 */

Tk_ImageType tkPhotoImageType = {
    "photo",			/* name */
    ImgPhotoCreate,		/* createProc */
    ImgPhotoGet,		/* getProc */
    ImgPhotoDisplay,		/* displayProc */
    ImgPhotoFree,		/* freeProc */
    ImgPhotoDelete,		/* deleteProc */
    ImgPhotoPostscript,		/* postscriptProc */
    (Tk_ImageType *) NULL	/* nextPtr */
};

typedef struct ThreadSpecificData {
    Tk_PhotoImageFormat *formatList;  /* Pointer to the first in the 
				       * list of known photo image formats.*/
    Tk_PhotoImageFormat *oldFormatList;  /* Pointer to the first in the 
				       * list of known photo image formats.*/
    int initialized;	/* set to 1 if we've initialized the strucuture */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Default configuration
 */

#define DEF_PHOTO_GAMMA		"1"
#define DEF_PHOTO_HEIGHT	"0"
#define DEF_PHOTO_PALETTE	""
#define DEF_PHOTO_WIDTH		"0"

/*
 * Information used for parsing configuration specifications:
 */
static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_STRING, "-file", (char *) NULL, (char *) NULL,
	 (char *) NULL, Tk_Offset(PhotoMaster, fileString), TK_CONFIG_NULL_OK},
    {TK_CONFIG_DOUBLE, "-gamma", (char *) NULL, (char *) NULL,
	 DEF_PHOTO_GAMMA, Tk_Offset(PhotoMaster, gamma), 0},
    {TK_CONFIG_INT, "-height", (char *) NULL, (char *) NULL,
	 DEF_PHOTO_HEIGHT, Tk_Offset(PhotoMaster, userHeight), 0},
    {TK_CONFIG_UID, "-palette", (char *) NULL, (char *) NULL,
	 DEF_PHOTO_PALETTE, Tk_Offset(PhotoMaster, palette), 0},
    {TK_CONFIG_INT, "-width", (char *) NULL, (char *) NULL,
	 DEF_PHOTO_WIDTH, Tk_Offset(PhotoMaster, userWidth), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	 (char *) NULL, 0, 0}
};

/*
 * Hash table used to hash from (display, colormap, palette, gamma)
 * to ColorTable address.
 */

static Tcl_HashTable imgPhotoColorHash;
static int imgPhotoColorHashInitialized;
#define N_COLOR_HASH	(sizeof(ColorTableId) / sizeof(int))

/*
 * Forward declarations
 */

static void		PhotoFormatThreadExitProc _ANSI_ARGS_((
			    ClientData clientData));
static int		ImgPhotoCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static int		ParseSubcommandOptions _ANSI_ARGS_((
			    struct SubcommandOptions *optPtr,
			    Tcl_Interp *interp, int allowedOptions,
			    int *indexPtr, int objc, Tcl_Obj *CONST objv[]));
static void		ImgPhotoCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static int		ImgPhotoConfigureMaster _ANSI_ARGS_((
			    Tcl_Interp *interp, PhotoMaster *masterPtr,
			    int objc, Tcl_Obj *CONST objv[], int flags));
static void		ImgPhotoConfigureInstance _ANSI_ARGS_((
			    PhotoInstance *instancePtr));
static int              ToggleComplexAlphaIfNeeded _ANSI_ARGS_((
                            PhotoMaster *mPtr));
static void             ImgPhotoBlendComplexAlpha _ANSI_ARGS_((
			    XImage *bgImg, PhotoInstance *iPtr,
			    int xOffset, int yOffset, int width, int height));
static int		ImgPhotoSetSize _ANSI_ARGS_((PhotoMaster *masterPtr,
			    int width, int height));
static void		ImgPhotoInstanceSetSize _ANSI_ARGS_((
			    PhotoInstance *instancePtr));
static int		ImgStringWrite _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *formatString,
			    Tk_PhotoImageBlock *blockPtr));
static char *		ImgGetPhoto _ANSI_ARGS_((PhotoMaster *masterPtr,
			    Tk_PhotoImageBlock *blockPtr,
			    struct SubcommandOptions *optPtr));
static int		IsValidPalette _ANSI_ARGS_((PhotoInstance *instancePtr,
			    CONST char *palette));
static int		CountBits _ANSI_ARGS_((pixel mask));
static void		GetColorTable _ANSI_ARGS_((PhotoInstance *instancePtr));
static void		FreeColorTable _ANSI_ARGS_((ColorTable *colorPtr,
			    int force));
static void		AllocateColors _ANSI_ARGS_((ColorTable *colorPtr));
static void		DisposeColorTable _ANSI_ARGS_((ClientData clientData));
static void		DisposeInstance _ANSI_ARGS_((ClientData clientData));
static int		ReclaimColors _ANSI_ARGS_((ColorTableId *id,
			    int numColors));
static int		MatchFileFormat _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Channel chan, char *fileName, Tcl_Obj *formatString,
			    Tk_PhotoImageFormat **imageFormatPtr,
			    int *widthPtr, int *heightPtr, int *oldformat));
static int		MatchStringFormat _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *data, Tcl_Obj *formatString,
			    Tk_PhotoImageFormat **imageFormatPtr,
			    int *widthPtr, int *heightPtr, int *oldformat));
static Tcl_ObjCmdProc *	PhotoOptionFind _ANSI_ARGS_((Tcl_Interp * interp,
			    Tcl_Obj *obj));
static void		DitherInstance _ANSI_ARGS_((PhotoInstance *instancePtr,
			    int x, int y, int width, int height));
static void		PhotoOptionCleanupProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));

#undef MIN
#define MIN(a, b)	((a) < (b)? (a): (b))
#undef MAX
#define MAX(a, b)	((a) > (b)? (a): (b))

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateOldPhotoImageFormat, Tk_CreatePhotoImageFormat --
 *
 *	This procedure is invoked by an image file handler to register
 *	a new photo image format and the procedures that handle the
 *	new format.  The procedure is typically invoked during
 *	Tcl_AppInit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The new image file format is entered into a table used in the
 *	photo image "read" and "write" subcommands.
 *
 *----------------------------------------------------------------------
 */

static void
PhotoFormatThreadExitProc(clientData)
    ClientData clientData;	/* not used */
{
    Tk_PhotoImageFormat *freePtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    while (tsdPtr->oldFormatList != NULL) {
	freePtr = tsdPtr->oldFormatList;
	tsdPtr->oldFormatList = tsdPtr->oldFormatList->nextPtr;
	ckfree((char *) freePtr->name);
	ckfree((char *) freePtr);
    }
    while (tsdPtr->formatList != NULL) {
	freePtr = tsdPtr->formatList;
	tsdPtr->formatList = tsdPtr->formatList->nextPtr;
	ckfree((char *) freePtr->name);
	ckfree((char *) freePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateOldPhotoImageFormat, Tk_CreatePhotoImageFormat --
 *
 *	This procedure is invoked by an image file handler to register
 *	a new photo image format and the procedures that handle the
 *	new format.  The procedure is typically invoked during
 *	Tcl_AppInit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The new image file format is entered into a table used in the
 *	photo image "read" and "write" subcommands.
 *
 *----------------------------------------------------------------------
 */
void
Tk_CreateOldPhotoImageFormat(formatPtr)
    Tk_PhotoImageFormat *formatPtr;
				/* Structure describing the format.  All of
				 * the fields except "nextPtr" must be filled
				 * in by caller.  Must not have been passed
				 * to Tk_CreatePhotoImageFormat previously. */
{
    Tk_PhotoImageFormat *copyPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateThreadExitHandler(PhotoFormatThreadExitProc, NULL);
    }
    copyPtr = (Tk_PhotoImageFormat *) ckalloc(sizeof(Tk_PhotoImageFormat));
    *copyPtr = *formatPtr;
    copyPtr->name = (char *) ckalloc((unsigned) (strlen(formatPtr->name) + 1));
    strcpy(copyPtr->name, formatPtr->name);
    copyPtr->nextPtr = tsdPtr->oldFormatList;
    tsdPtr->oldFormatList = copyPtr;
}

void
Tk_CreatePhotoImageFormat(formatPtr)
    Tk_PhotoImageFormat *formatPtr;
				/* Structure describing the format.  All of
				 * the fields except "nextPtr" must be filled
				 * in by caller.  Must not have been passed
				 * to Tk_CreatePhotoImageFormat previously. */
{
    Tk_PhotoImageFormat *copyPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateThreadExitHandler(PhotoFormatThreadExitProc, NULL);
    }
    copyPtr = (Tk_PhotoImageFormat *) ckalloc(sizeof(Tk_PhotoImageFormat));
    *copyPtr = *formatPtr;
    copyPtr->name = (char *) ckalloc((unsigned) (strlen(formatPtr->name) + 1));
    strcpy(copyPtr->name, formatPtr->name);
    if (isupper((unsigned char) *formatPtr->name)) {
	copyPtr->nextPtr = tsdPtr->oldFormatList;
	tsdPtr->oldFormatList = copyPtr;
    } else {
	copyPtr->nextPtr = tsdPtr->formatList;
	tsdPtr->formatList = copyPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoCreate --
 *
 *	This procedure is called by the Tk image code to create
 *	a new photo image.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The data structure for a new photo image is allocated and
 *	initialized.
 *
 *----------------------------------------------------------------------
 */

static int
ImgPhotoCreate(interp, name, objc, objv, typePtr, master, clientDataPtr)
    Tcl_Interp *interp;		/* Interpreter for application containing
				 * image. */
    char *name;			/* Name to use for image. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects for options (doesn't
				 * include image name or type). */
    Tk_ImageType *typePtr;	/* Pointer to our type record (not used). */
    Tk_ImageMaster master;	/* Token for image, to be used by us in
				 * later callbacks. */
    ClientData *clientDataPtr;	/* Store manager's token for image here;
				 * it will be returned in later callbacks. */
{
    PhotoMaster *masterPtr;

    /*
     * Allocate and initialize the photo image master record.
     */

    masterPtr = (PhotoMaster *) ckalloc(sizeof(PhotoMaster));
    memset((void *) masterPtr, 0, sizeof(PhotoMaster));
    masterPtr->tkMaster = master;
    masterPtr->interp = interp;
    masterPtr->imageCmd = Tcl_CreateObjCommand(interp, name, ImgPhotoCmd,
	    (ClientData) masterPtr, ImgPhotoCmdDeletedProc);
    masterPtr->palette = NULL;
    masterPtr->pix32 = NULL;
    masterPtr->instancePtr = NULL;
    masterPtr->validRegion = TkCreateRegion();

    /*
     * Process configuration options given in the image create command.
     */

    if (ImgPhotoConfigureMaster(interp, masterPtr, objc, objv, 0) != TCL_OK) {
	ImgPhotoDelete((ClientData) masterPtr);
	return TCL_ERROR;
    }

    *clientDataPtr = (ClientData) masterPtr;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoCmd --
 *
 *	This procedure is invoked to process the Tcl command that
 *	corresponds to a photo image.  See the user documentation
 *	for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ImgPhotoCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about photo master. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int oldformat = 0;
    static CONST char *photoOptions[] = {
	"blank", "cget", "configure", "copy", "data", "get", "put",
	"read", "redither", "transparency", "write", (char *) NULL
    };
    enum options {
	PHOTO_BLANK, PHOTO_CGET, PHOTO_CONFIGURE, PHOTO_COPY, PHOTO_DATA,
	PHOTO_GET, PHOTO_PUT, PHOTO_READ, PHOTO_REDITHER, PHOTO_TRANS,
	PHOTO_WRITE
    };

    PhotoMaster *masterPtr = (PhotoMaster *) clientData;
    int result, index;
    int x, y, width, height;
    int dataWidth, dataHeight;
    struct SubcommandOptions options;
    int listArgc;
    CONST char **listArgv;
    CONST char **srcArgv;
    unsigned char *pixelPtr;
    Tk_PhotoImageBlock block;
    Tk_Window tkwin;
    XColor color;
    Tk_PhotoImageFormat *imageFormat;
    int imageWidth, imageHeight;
    int matched;
    Tcl_Channel chan;
    Tk_PhotoHandle srcHandle;
    size_t length;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], photoOptions, "option", 0,
	    &index) != TCL_OK) {
	Tcl_ObjCmdProc *proc;
	proc = PhotoOptionFind(interp, objv[1]);
	if (proc == (Tcl_ObjCmdProc *) NULL) {
	    return TCL_ERROR;
	}
	return proc(clientData, interp, objc, objv);
    }

    switch ((enum options) index) {
    case PHOTO_BLANK:
	/*
	 * photo blank command - just call Tk_PhotoBlank.
	 */

	if (objc == 2) {
	    Tk_PhotoBlank(masterPtr);
	    return TCL_OK;
	} else {
	    Tcl_WrongNumArgs(interp, 2, objv, (char *) NULL);
	    return TCL_ERROR;
	}

    case PHOTO_CGET: {
	char *arg;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    return TCL_ERROR;
	}
	arg = Tcl_GetStringFromObj(objv[2], (int *) &length);
	if (strncmp(arg,"-data", length) == 0) {
	    if (masterPtr->dataString) {
		Tcl_SetObjResult(interp, masterPtr->dataString);
	    }
	} else if (strncmp(arg,"-format", length) == 0) {
	    if (masterPtr->format) {
		Tcl_SetObjResult(interp, masterPtr->format);
	    }
	} else {
	    Tk_ConfigureValue(interp, Tk_MainWindow(interp), configSpecs,
		    (char *) masterPtr, Tcl_GetString(objv[2]), 0);
	}
	return TCL_OK;
    }

    case PHOTO_CONFIGURE:
	/*
	 * photo configure command - handle this in the standard way.
	 */

	if (objc == 2) {
	    Tcl_Obj *obj, *subobj;
	    result = Tk_ConfigureInfo(interp, Tk_MainWindow(interp),
		    configSpecs, (char *) masterPtr, (char *) NULL, 0);
	    if (result != TCL_OK) {
		return result;
	    }
	    obj = Tcl_NewObj();
	    subobj = Tcl_NewStringObj("-data {} {} {}", 14);
	    if (masterPtr->dataString) {
		Tcl_ListObjAppendElement(interp, subobj, masterPtr->dataString);
	    } else {
		Tcl_AppendStringsToObj(subobj, " {}", (char *) NULL);
	    }
	    Tcl_ListObjAppendElement(interp, obj, subobj);
	    subobj = Tcl_NewStringObj("-format {} {} {}", 16);
	    if (masterPtr->format) {
		Tcl_ListObjAppendElement(interp, subobj, masterPtr->format);
	    } else {
		Tcl_AppendStringsToObj(subobj, " {}", (char *) NULL);
	    }
	    Tcl_ListObjAppendElement(interp, obj, subobj);
	    Tcl_ListObjAppendList(interp, obj, Tcl_GetObjResult(interp));
	    Tcl_SetObjResult(interp, obj);
	    return TCL_OK;
	}
	if (objc == 3) {
	    char *arg = Tcl_GetStringFromObj(objv[2], (int *) &length);
	    if (!strncmp(arg, "-data", length)) {
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"-data {} {} {}", (char *) NULL);
		if (masterPtr->dataString) {
		    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp),
			    masterPtr->dataString);
		} else {
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			    " {}", (char *) NULL);
		}
		return TCL_OK;
	    } else if (!strncmp(arg, "-format", length)) {
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"-format {} {} {}", (char *) NULL);
		if (masterPtr->format) {
		    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp),
			    masterPtr->format);
		} else {
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			    " {}", (char *) NULL);
		}
		return TCL_OK;
	    } else {
		return Tk_ConfigureInfo(interp, Tk_MainWindow(interp),
			configSpecs, (char *) masterPtr, arg, 0);
	    }
	}
	return ImgPhotoConfigureMaster(interp, masterPtr, objc-2, objv+2,
		TK_CONFIG_ARGV_ONLY);

    case PHOTO_COPY:
	/*
	 * photo copy command - first parse options.
	 */

	index = 2;
	memset((VOID *) &options, 0, sizeof(options));
	options.zoomX = options.zoomY = 1;
	options.subsampleX = options.subsampleY = 1;
	options.name = NULL;
	options.compositingRule = TK_PHOTO_COMPOSITE_OVERLAY;
	if (ParseSubcommandOptions(&options, interp,
		OPT_FROM | OPT_TO | OPT_ZOOM | OPT_SUBSAMPLE | OPT_SHRINK |
		OPT_COMPOSITE, &index, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (options.name == NULL || index < objc) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "source-image ?-compositingrule rule? ?-from x1 y1 x2 y2? ?-to x1 y1 x2 y2? ?-zoom x y? ?-subsample x y?");
	    return TCL_ERROR;
	}

	/*
	 * Look for the source image and get a pointer to its image data.
	 * Check the values given for the -from option.
	 */

	srcHandle = Tk_FindPhoto(interp, Tcl_GetString(options.name));
	if (srcHandle == NULL) {
	    Tcl_AppendResult(interp, "image \"",
		    Tcl_GetString(options.name), "\" doesn't",
		    " exist or is not a photo image", (char *) NULL);
	    return TCL_ERROR;
	}
	Tk_PhotoGetImage(srcHandle, &block);
	if ((options.fromX2 > block.width) || (options.fromY2 > block.height)
		|| (options.fromX2 > block.width)
		|| (options.fromY2 > block.height)) {
	    Tcl_AppendResult(interp, "coordinates for -from option extend ",
		    "outside source image", (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Fill in default values for unspecified parameters.
	 */

	if (!(options.options & OPT_FROM) || (options.fromX2 < 0)) {
	    options.fromX2 = block.width;
	    options.fromY2 = block.height;
	}
	if (!(options.options & OPT_TO) || (options.toX2 < 0)) {
	    width = options.fromX2 - options.fromX;
	    if (options.subsampleX > 0) {
		width = (width + options.subsampleX - 1) / options.subsampleX;
	    } else if (options.subsampleX == 0) {
		width = 0;
	    } else {
		width = (width - options.subsampleX - 1) / -options.subsampleX;
	    }
	    options.toX2 = options.toX + width * options.zoomX;

	    height = options.fromY2 - options.fromY;
	    if (options.subsampleY > 0) {
		height = (height + options.subsampleY - 1)
			/ options.subsampleY;
	    } else if (options.subsampleY == 0) {
		height = 0;
	    } else {
		height = (height - options.subsampleY - 1)
			/ -options.subsampleY;
	    }
	    options.toY2 = options.toY + height * options.zoomY;
	}

	/*
	 * Set the destination image size if the -shrink option was specified.
	 */

	if (options.options & OPT_SHRINK) {
	    if (ImgPhotoSetSize(masterPtr, options.toX2,
		    options.toY2) != TCL_OK) {
		Tcl_ResetResult(interp);
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			TK_PHOTO_ALLOC_FAILURE_MESSAGE, (char *) NULL);
		return TCL_ERROR;
	    }
	}

	/*
	 * Copy the image data over using Tk_PhotoPutZoomedBlock.
	 */

	block.pixelPtr += options.fromX * block.pixelSize
		+ options.fromY * block.pitch;
	block.width = options.fromX2 - options.fromX;
	block.height = options.fromY2 - options.fromY;
	Tk_PhotoPutZoomedBlock((Tk_PhotoHandle) masterPtr, &block,
		options.toX, options.toY, options.toX2 - options.toX,
		options.toY2 - options.toY, options.zoomX, options.zoomY,
		options.subsampleX, options.subsampleY,
		options.compositingRule);
	return TCL_OK;

    case PHOTO_DATA: {
	char *data;

	/*
	 * photo data command - first parse and check any options given.
	 */
	Tk_ImageStringWriteProc *stringWriteProc = NULL;

	index = 2;
	memset((VOID *) &options, 0, sizeof(options));
	options.name = NULL;
	options.format = NULL;
	options.fromX = 0;
	options.fromY = 0;
	if (ParseSubcommandOptions(&options, interp,
		OPT_FORMAT | OPT_FROM | OPT_GRAYSCALE | OPT_BACKGROUND,
		&index, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((options.name != NULL) || (index < objc)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?options?");
	    return TCL_ERROR;
	}
	if ((options.fromX > masterPtr->width)
		|| (options.fromY > masterPtr->height)
		|| (options.fromX2 > masterPtr->width)
		|| (options.fromY2 > masterPtr->height)) {
	    Tcl_AppendResult(interp, "coordinates for -from option extend ",
		    "outside image", (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Fill in default values for unspecified parameters.
	 */

	if (((options.options & OPT_FROM) == 0) || (options.fromX2 < 0)) {
	    options.fromX2 = masterPtr->width;
	    options.fromY2 = masterPtr->height;
	}

	/*
	 * Search for an appropriate image string format handler.
	 */

	if (options.options & OPT_FORMAT) {
	    for (imageFormat = tsdPtr->formatList; imageFormat != NULL;
	 	imageFormat = imageFormat->nextPtr) {
		if ((strncasecmp(Tcl_GetString(options.format),
			imageFormat->name, strlen(imageFormat->name)) == 0)) {
		    if (imageFormat->stringWriteProc != NULL) {
			stringWriteProc = imageFormat->stringWriteProc;
			break;
		    }
		}
	    }
	    if (stringWriteProc == NULL) {
		Tcl_AppendResult(interp, "image string format \"",
			Tcl_GetString(options.format),
			"\" is not supported", (char *) NULL);
		return TCL_ERROR;
	    }
	} else {
	    stringWriteProc = ImgStringWrite;
	}

	/*
	 * Call the handler's string write procedure to write out
	 * the image.
	 */

	data = ImgGetPhoto(masterPtr, &block, &options);

	result = ((int (*) _ANSI_ARGS_((Tcl_Interp *interp, Tcl_Obj *formatString,
		Tk_PhotoImageBlock *blockPtr, VOID *dummy))) stringWriteProc)
		(interp, options.format, &block, (VOID *) NULL);
	if (options.background) {
	    Tk_FreeColor(options.background);
	}
	if (data) {
	    ckfree(data);
	}
	return result;
    }

    case PHOTO_GET: {
	/*
	 * photo get command - first parse and check parameters.
	 */

	char string[TCL_INTEGER_SPACE * 3];

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "x y");
	    return TCL_ERROR;
	}
	if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if ((x < 0) || (x >= masterPtr->width)
		|| (y < 0) || (y >= masterPtr->height)) {
	    Tcl_AppendResult(interp, Tcl_GetString(objv[0]), " get: ",
		    "coordinates out of range", (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Extract the value of the desired pixel and format it as a string.
	 */

	pixelPtr = masterPtr->pix32 + (y * masterPtr->width + x) * 4;
	sprintf(string, "%d %d %d", pixelPtr[0], pixelPtr[1],
		pixelPtr[2]);
	Tcl_AppendResult(interp, string, (char *) NULL);
	return TCL_OK;
    }

    case PHOTO_PUT:
	/*
	 * photo put command - first parse the options and colors specified.
	 */

	index = 2;
	memset((VOID *) &options, 0, sizeof(options));
	options.name = NULL;
	if (ParseSubcommandOptions(&options, interp, OPT_TO|OPT_FORMAT,
		&index, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((options.name == NULL) || (index < objc)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?options?");
	    return TCL_ERROR;
	}

	if (MatchStringFormat(interp, options.name ? objv[2]:NULL, 
		options.format, &imageFormat, &imageWidth,
		&imageHeight, &oldformat) == TCL_OK) {
	    Tcl_Obj *format, *data;

	    if (((options.options & OPT_TO) == 0) || (options.toX2 < 0)) {
		options.toX2 = options.toX + imageWidth;
		options.toY2 = options.toY + imageHeight;
	    }
	    if (imageWidth > options.toX2 - options.toX) {
		imageWidth = options.toX2 - options.toX;
	    }
	    if (imageHeight > options.toY2 - options.toY) {
		imageHeight = options.toY2 - options.toY;
	    }
	    format = options.format;
	    data = objv[2];
	    if (oldformat) {
		if (format) {
		    format = (Tcl_Obj *) Tcl_GetString(format);
		}
		data = (Tcl_Obj *) Tcl_GetString(data);
	    }
	    if ((*imageFormat->stringReadProc)(interp, data,
		    format, (Tk_PhotoHandle) masterPtr,
		    options.toX, options.toY, imageWidth, imageHeight,
		    0, 0) != TCL_OK) {
		return TCL_ERROR;
	    }
	    masterPtr->flags |= IMAGE_CHANGED;
	    return TCL_OK;
	}
	if (options.options & OPT_FORMAT) {
	    return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
	if (Tcl_SplitList(interp, Tcl_GetString(options.name),
		&dataHeight, &srcArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
	tkwin = Tk_MainWindow(interp);
	block.pixelPtr = NULL;
	dataWidth = 0;
	pixelPtr = NULL;
	for (y = 0; y < dataHeight; ++y) {
	    if (Tcl_SplitList(interp, srcArgv[y], &listArgc, &listArgv)
		    != TCL_OK) {
		break;
	    }
	    if (y == 0) {
		if (listArgc == 0) {
		    /*
		     * Lines must be non-empty...
		     */
		    break;
		}
		dataWidth = listArgc;
		pixelPtr = (unsigned char *)
			ckalloc((unsigned) dataWidth * dataHeight * 3);
		block.pixelPtr = pixelPtr;
	    } else if (listArgc != dataWidth) {
		Tcl_AppendResult(interp, "all elements of color list must",
			" have the same number of elements", (char *) NULL);
		ckfree((char *) listArgv);
		break;
	    }
	    for (x = 0; x < dataWidth; ++x) {
		if (!XParseColor(Tk_Display(tkwin), Tk_Colormap(tkwin),
			listArgv[x], &color)) {
		    Tcl_AppendResult(interp, "can't parse color \"",
			    listArgv[x], "\"", (char *) NULL);
		    break;
		}
		*pixelPtr++ = color.red >> 8;
		*pixelPtr++ = color.green >> 8;
		*pixelPtr++ = color.blue >> 8;
	    }
	    ckfree((char *) listArgv);
	    if (x < dataWidth) {
		break;
	    }
	}
	ckfree((char *) srcArgv);
	if (y < dataHeight || dataHeight == 0 || dataWidth == 0) {
	    if (block.pixelPtr != NULL) {
		ckfree((char *) block.pixelPtr);
	    }
	    if (y < dataHeight) {
		return TCL_ERROR;
	    }
	    return TCL_OK;
	}

	/*
	 * Fill in default values for the -to option, then
	 * copy the block in using Tk_PhotoPutBlock.
	 */

	if (!(options.options & OPT_TO) || (options.toX2 < 0)) {
	    options.toX2 = options.toX + dataWidth;
	    options.toY2 = options.toY + dataHeight;
	}
	block.width = dataWidth;
	block.height = dataHeight;
	block.pitch = dataWidth * 3;
	block.pixelSize = 3;
	block.offset[0] = 0;
	block.offset[1] = 1;
	block.offset[2] = 2;
	block.offset[3] = 0;
	Tk_PhotoPutBlock((ClientData)masterPtr, &block,
		options.toX, options.toY, options.toX2 - options.toX,
		options.toY2 - options.toY, TK_PHOTO_COMPOSITE_SET);
	ckfree((char *) block.pixelPtr);
	return TCL_OK;

    case PHOTO_READ: {
	Tcl_Obj *format;

	/*
	 * photo read command - first parse the options specified.
	 */

	index = 2;
	memset((VOID *) &options, 0, sizeof(options));
	options.name = NULL;
	options.format = NULL;
	if (ParseSubcommandOptions(&options, interp,
		OPT_FORMAT | OPT_FROM | OPT_TO | OPT_SHRINK,
		&index, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((options.name == NULL) || (index < objc)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "fileName ?options?");
	    return TCL_ERROR;
	}

        /*
         * Prevent file system access in safe interpreters.
         */

        if (Tcl_IsSafe(interp)) {
            Tcl_AppendResult(interp, "can't get image from a file in a",
		    " safe interpreter", (char *) NULL);
            return TCL_ERROR;
        }
        
	/*
	 * Open the image file and look for a handler for it.
	 */

	chan = Tcl_OpenFileChannel(interp,
		Tcl_GetString(options.name), "r", 0);
	if (chan == NULL) {
	    return TCL_ERROR;
	}
        if (Tcl_SetChannelOption(interp, chan, "-translation", "binary")
		!= TCL_OK) {
	    Tcl_Close(NULL, chan);
            return TCL_ERROR;
        }
        if (Tcl_SetChannelOption(interp, chan, "-encoding", "binary")
		!= TCL_OK) {
	    Tcl_Close(NULL, chan);
            return TCL_ERROR;
        }
    
	if (MatchFileFormat(interp, chan,
		Tcl_GetString(options.name), options.format, &imageFormat,
		&imageWidth, &imageHeight, &oldformat) != TCL_OK) {
	    Tcl_Close(NULL, chan);
	    return TCL_ERROR;
	}

	/*
	 * Check the values given for the -from option.
	 */

	if ((options.fromX > imageWidth) || (options.fromY > imageHeight)
		|| (options.fromX2 > imageWidth)
		|| (options.fromY2 > imageHeight)) {
	    Tcl_AppendResult(interp, "coordinates for -from option extend ",
		    "outside source image", (char *) NULL);
	    Tcl_Close(NULL, chan);
	    return TCL_ERROR;
	}
	if (((options.options & OPT_FROM) == 0) || (options.fromX2 < 0)) {
	    width = imageWidth - options.fromX;
	    height = imageHeight - options.fromY;
	} else {
	    width = options.fromX2 - options.fromX;
	    height = options.fromY2 - options.fromY;
	}

	/*
	 * If the -shrink option was specified, set the size of the image.
	 */

	if (options.options & OPT_SHRINK) {
	    if (ImgPhotoSetSize(masterPtr, options.toX + width,
		    options.toY + height) != TCL_OK) {
		Tcl_ResetResult(interp);
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			TK_PHOTO_ALLOC_FAILURE_MESSAGE, (char *) NULL);
		return TCL_ERROR;
	    }
	}

	/*
	 * Call the handler's file read procedure to read the data
	 * into the image.
	 */

	format = options.format;
	if (oldformat && format) {
	    format = (Tcl_Obj *) Tcl_GetString(format);
	}
	result = (*imageFormat->fileReadProc)(interp, chan,
		Tcl_GetString(options.name),
		format, (Tk_PhotoHandle) masterPtr, options.toX,
		options.toY, width, height, options.fromX, options.fromY);
	if (chan != NULL) {
	    Tcl_Close(NULL, chan);
	}
	return result;
    }

    case PHOTO_REDITHER:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Call Dither if any part of the image is not correctly
	 * dithered at present.
	 */

	x = masterPtr->ditherX;
	y = masterPtr->ditherY;
	if (masterPtr->ditherX != 0) {
	    Tk_DitherPhoto((Tk_PhotoHandle) masterPtr, x, y,
		    masterPtr->width - x, 1);
	}
	if (masterPtr->ditherY < masterPtr->height) {
	    x = 0;
	    Tk_DitherPhoto((Tk_PhotoHandle)masterPtr, 0,
		    masterPtr->ditherY, masterPtr->width,
		    masterPtr->height - masterPtr->ditherY);
	}

	if (y < masterPtr->height) {
	    /*
	     * Tell the core image code that part of the image has changed.
	     */

	    Tk_ImageChanged(masterPtr->tkMaster, x, y,
		    (masterPtr->width - x), (masterPtr->height - y),
		    masterPtr->width, masterPtr->height);
	}
	return TCL_OK;

    case PHOTO_TRANS: {
	static CONST char *photoTransOptions[] = {
	    "get", "set", (char *) NULL
	};
	enum transOptions {
	    PHOTO_TRANS_GET, PHOTO_TRANS_SET
	};

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option ?arg arg ...?");
	    return TCL_ERROR;
	}
	if (Tcl_GetIndexFromObj(interp, objv[2], photoTransOptions, "option",
		0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}

	switch ((enum transOptions) index) {
	case PHOTO_TRANS_GET: {
	    XRectangle testBox;
	    TkRegion testRegion;

	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "x y");
		return TCL_ERROR;
	    }
	    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK)
		    || (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
		return TCL_ERROR;
	    }
	    if ((x < 0) || (x >= masterPtr->width)
		|| (y < 0) || (y >= masterPtr->height)) {
		Tcl_AppendResult(interp, Tcl_GetString(objv[0]),
			" transparency get: coordinates out of range",
			(char *) NULL);
		return TCL_ERROR;
	    }

	    testBox.x = x;
	    testBox.y = y;
	    testBox.width = 1;
	    testBox.height = 1;
	    /* What a way to do a test! */
	    testRegion = TkCreateRegion();
	    TkUnionRectWithRegion(&testBox, testRegion, testRegion);
	    TkIntersectRegion(testRegion, masterPtr->validRegion, testRegion);
	    TkClipBox(testRegion, &testBox);
	    TkDestroyRegion(testRegion);

	    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
		    (testBox.width==0 && testBox.height==0));
	    return TCL_OK;
	}

	case PHOTO_TRANS_SET: {
	    int transFlag;
	    XRectangle setBox;

	    if (objc != 6) {
		Tcl_WrongNumArgs(interp, 3, objv, "x y boolean");
		return TCL_ERROR;
	    }
	    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK)
		    || (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)
		    || (Tcl_GetBooleanFromObj(interp, objv[5],
		    &transFlag) != TCL_OK)) {
		return TCL_ERROR;
	    }
	    if ((x < 0) || (x >= masterPtr->width)
		|| (y < 0) || (y >= masterPtr->height)) {
		Tcl_AppendResult(interp, Tcl_GetString(objv[0]),
			" transparency set: coordinates out of range",
			(char *) NULL);
		return TCL_ERROR;
	    }

	    setBox.x = x;
	    setBox.y = y;
	    setBox.width = 1;
	    setBox.height = 1;
	    pixelPtr = masterPtr->pix32 + (y * masterPtr->width + x) * 4;

	    if (transFlag) {
		/*
		 * Make pixel transparent.
		 */
		TkRegion clearRegion = TkCreateRegion();

		TkUnionRectWithRegion(&setBox, clearRegion, clearRegion);
		TkSubtractRegion(masterPtr->validRegion, clearRegion,
			masterPtr->validRegion);
		TkDestroyRegion(clearRegion);
		/*
		 * Set the alpha value correctly.
		 */
		pixelPtr[3] = 0;
	    } else {
		/*
		 * Make pixel opaque.
		 */
		TkUnionRectWithRegion(&setBox, masterPtr->validRegion,
			masterPtr->validRegion);
		pixelPtr[3] = 255;
	    }

	    /*
	     * Inform the generic image code that the image
	     * has (potentially) changed.
	     */

	    Tk_ImageChanged(masterPtr->tkMaster, x, y, 1, 1,
		    masterPtr->width, masterPtr->height);
	    masterPtr->flags &= ~IMAGE_CHANGED;
	    return TCL_OK;
	}
	}

	panic("unexpected fallthrough");
    }

    case PHOTO_WRITE: {
	char *data;
	Tcl_Obj *format;

        /*
         * Prevent file system access in safe interpreters.
         */

        if (Tcl_IsSafe(interp)) {
            Tcl_AppendResult(interp, "can't write image to a file in a",
		    " safe interpreter", (char *) NULL);
            return TCL_ERROR;
        }
        
	/*
	 * photo write command - first parse and check any options given.
	 */

	index = 2;
	memset((VOID *) &options, 0, sizeof(options));
	options.name = NULL;
	options.format = NULL;
	if (ParseSubcommandOptions(&options, interp,
		OPT_FORMAT | OPT_FROM | OPT_GRAYSCALE | OPT_BACKGROUND,
		&index, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((options.name == NULL) || (index < objc)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "fileName ?options?");
	    return TCL_ERROR;
	}
	if ((options.fromX > masterPtr->width)
		|| (options.fromY > masterPtr->height)
		|| (options.fromX2 > masterPtr->width)
		|| (options.fromY2 > masterPtr->height)) {
	    Tcl_AppendResult(interp, "coordinates for -from option extend ",
		    "outside image", (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Fill in default values for unspecified parameters.
	 */

	if (!(options.options & OPT_FROM) || (options.fromX2 < 0)) {
	    options.fromX2 = masterPtr->width;
	    options.fromY2 = masterPtr->height;
	}

	/*
	 * Search for an appropriate image file format handler,
	 * and give an error if none is found.
	 */

	matched = 0;
	for (imageFormat = tsdPtr->formatList; imageFormat != NULL;
		imageFormat = imageFormat->nextPtr) {
	    if ((options.format == NULL)
		    || (strncasecmp(Tcl_GetString(options.format),
		    imageFormat->name, strlen(imageFormat->name)) == 0)) {
		matched = 1;
		if (imageFormat->fileWriteProc != NULL) {
		    break;
		}
	    }
	}
	if (imageFormat == NULL) {
	    oldformat = 1;
	    for (imageFormat = tsdPtr->oldFormatList; imageFormat != NULL;
		    imageFormat = imageFormat->nextPtr) {
		if ((options.format == NULL)
			|| (strncasecmp(Tcl_GetString(options.format),
			imageFormat->name, strlen(imageFormat->name)) == 0)) {
		    matched = 1;
		    if (imageFormat->fileWriteProc != NULL) {
			break;
		    }
		}
	    }
	}
	if (imageFormat == NULL) {
	    if (options.format == NULL) {
		Tcl_AppendResult(interp, "no available image file format ",
			"has file writing capability", (char *) NULL);
	    } else if (!matched) {
		Tcl_AppendResult(interp, "image file format \"",
			Tcl_GetString(options.format),
			"\" is unknown", (char *) NULL);
	    } else {
		Tcl_AppendResult(interp, "image file format \"",
			Tcl_GetString(options.format),
			"\" has no file writing capability",
			(char *) NULL);
	    }
	    return TCL_ERROR;
	}

	/*
	 * Call the handler's file write procedure to write out
	 * the image.
	 */

	data = ImgGetPhoto(masterPtr, &block, &options);
	format = options.format;
	if (oldformat && format) {
	    format = (Tcl_Obj *) Tcl_GetString(options.format);
	}
	result = (*imageFormat->fileWriteProc)(interp,
		Tcl_GetString(options.name), format, &block);
	if (options.background) {
	    Tk_FreeColor(options.background);
	}
	if (data) {
	    ckfree(data);
	}
	return result;
    }

    }
    panic("unexpected fallthrough");
    return TCL_ERROR; /* NOT REACHED */
}

/*
 *----------------------------------------------------------------------
 *
 * ParseSubcommandOptions --
 *
 *	This procedure is invoked to process one of the options
 *	which may be specified for the photo image subcommands,
 *	namely, -from, -to, -zoom, -subsample, -format, -shrink,
 *	and -compositingrule.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Fields in *optPtr get filled in.
 *
 *----------------------------------------------------------------------
 */

static int
ParseSubcommandOptions(optPtr, interp, allowedOptions, optIndexPtr, objc, objv)
    struct SubcommandOptions *optPtr;
				/* Information about the options specified
				 * and the values given is returned here. */
    Tcl_Interp *interp;		/* Interpreter to use for reporting errors. */
    int allowedOptions;		/* Indicates which options are valid for
				 * the current command. */
    int *optIndexPtr;		/* Points to a variable containing the
				 * current index in objv; this variable is
				 * updated by this procedure. */
    int objc;			/* Number of arguments in objv[]. */
    Tcl_Obj *CONST objv[];	/* Arguments to be parsed. */
{
    int index, c, bit, currentBit;
    int length;
    char *option, **listPtr;
    int values[4];
    int numValues, maxValues, argIndex;

    for (index = *optIndexPtr; index < objc; *optIndexPtr = ++index) {
	/*
	 * We can have one value specified without an option;
	 * it goes into optPtr->name.
	 */

	option = Tcl_GetStringFromObj(objv[index], &length);
	if (option[0] != '-') {
	    if (optPtr->name == NULL) {
		optPtr->name = objv[index];
		continue;
	    }
	    break;
	}

	/*
	 * Work out which option this is.
	 */

	c = option[0];
	bit = 0;
	currentBit = 1;
	for (listPtr = optionNames; *listPtr != NULL; ++listPtr) {
	    if ((c == *listPtr[0])
		    && (strncmp(option, *listPtr, (size_t) length) == 0)) {
		if (bit != 0) {
		    bit = 0;	/* An ambiguous option. */
		    break;
		}
		bit = currentBit;
	    }
	    currentBit <<= 1;
	}

	/*
	 * If this option is not recognized and allowed, put
	 * an error message in the interpreter and return.
	 */

	if ((allowedOptions & bit) == 0) {
	    Tcl_AppendResult(interp, "unrecognized option \"",
	    	    Tcl_GetString(objv[index]),
		    "\": must be ", (char *)NULL);
	    bit = 1;
	    for (listPtr = optionNames; *listPtr != NULL; ++listPtr) {
		if ((allowedOptions & bit) != 0) {
		    if ((allowedOptions & (bit - 1)) != 0) {
			Tcl_AppendResult(interp, ", ", (char *) NULL);
			if ((allowedOptions & ~((bit << 1) - 1)) == 0) {
			    Tcl_AppendResult(interp, "or ", (char *) NULL);
			}
		    }
		    Tcl_AppendResult(interp, *listPtr, (char *) NULL);
		}
		bit <<= 1;
	    }
	    return TCL_ERROR;
	}

	/*
	 * For the -from, -to, -zoom and -subsample options,
	 * parse the values given.  Report an error if too few
	 * or too many values are given.
	 */

	if (bit == OPT_BACKGROUND) {
	    /*
	     * The -background option takes a single XColor value.
	     */

	    if (index + 1 < objc) {
		*optIndexPtr = ++index;
		optPtr->background = Tk_GetColor(interp, Tk_MainWindow(interp),
			Tk_GetUid(Tcl_GetString(objv[index])));
		if (!optPtr->background) {
		    return TCL_ERROR;
		}
	    } else {
		Tcl_AppendResult(interp, "the \"-background\" option ",
			"requires a value", (char *) NULL);
		return TCL_ERROR;
	    }
	} else if (bit == OPT_FORMAT) {
	    /*
	     * The -format option takes a single string value.  Note
	     * that parsing this is outside the scope of this
	     * function.
	     */

	    if (index + 1 < objc) {
		*optIndexPtr = ++index;
		optPtr->format = objv[index];
	    } else {
		Tcl_AppendResult(interp, "the \"-format\" option ",
			"requires a value", (char *) NULL);
		return TCL_ERROR;
	    }
	} else if (bit == OPT_COMPOSITE) {
	    /*
	     * The -compositingrule option takes a single value from
	     * a well-known set.
	     */

	    if (index + 1 < objc) {
		/*
		 * Note that these must match the TK_PHOTO_COMPOSITE_*
		 * constants.
		 */
		static CONST char *compositingRules[] = {
		    "overlay", "set",
		    NULL
		};

		index++;
		if (Tcl_GetIndexFromObj(interp, objv[index], compositingRules,
			"compositing rule", 0, &optPtr->compositingRule)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
		*optIndexPtr = index;
	    } else {
		Tcl_AppendResult(interp, "the \"-compositingrule\" option ",
			"requires a value", (char *) NULL);
		return TCL_ERROR;
	    }
	} else if ((bit != OPT_SHRINK) && (bit != OPT_GRAYSCALE)) {
	    char *val;
	    maxValues = ((bit == OPT_FROM) || (bit == OPT_TO))? 4: 2;
	    argIndex = index + 1;
	    for (numValues = 0; numValues < maxValues; ++numValues) {
		if (argIndex >= objc) {
		    break;
		}
	        val = Tcl_GetString(objv[argIndex]);
		if ((argIndex < objc) && (isdigit(UCHAR(val[0]))
			|| ((val[0] == '-') && isdigit(UCHAR(val[1]))))) {
		    if (Tcl_GetInt(interp, val, &values[numValues])
			    != TCL_OK) {
			return TCL_ERROR;
		    }
		} else {
		    break;
		}
		++argIndex;
	    }

	    if (numValues == 0) {
		Tcl_AppendResult(interp, "the \"", option, "\" option ",
			 "requires one ", maxValues == 2? "or two": "to four",
			 " integer values", (char *) NULL);
		return TCL_ERROR;
	    }
	    *optIndexPtr = (index += numValues);

	    /*
	     * Y values default to the corresponding X value if not specified.
	     */

	    if (numValues == 1) {
		values[1] = values[0];
	    }
	    if (numValues == 3) {
		values[3] = values[2];
	    }

	    /*
	     * Check the values given and put them in the appropriate
	     * field of the SubcommandOptions structure.
	     */

	    switch (bit) {
		case OPT_FROM:
		    if ((values[0] < 0) || (values[1] < 0) || ((numValues > 2)
			    && ((values[2] < 0) || (values[3] < 0)))) {
			Tcl_AppendResult(interp, "value(s) for the -from",
				" option must be non-negative", (char *) NULL);
			return TCL_ERROR;
		    }
		    if (numValues <= 2) {
			optPtr->fromX = values[0];
			optPtr->fromY = values[1];
			optPtr->fromX2 = -1;
			optPtr->fromY2 = -1;
		    } else {
			optPtr->fromX = MIN(values[0], values[2]);
			optPtr->fromY = MIN(values[1], values[3]);
			optPtr->fromX2 = MAX(values[0], values[2]);
			optPtr->fromY2 = MAX(values[1], values[3]);
		    }
		    break;
		case OPT_SUBSAMPLE:
		    optPtr->subsampleX = values[0];
		    optPtr->subsampleY = values[1];
		    break;
		case OPT_TO:
		    if ((values[0] < 0) || (values[1] < 0) || ((numValues > 2)
			    && ((values[2] < 0) || (values[3] < 0)))) {
			Tcl_AppendResult(interp, "value(s) for the -to",
				" option must be non-negative", (char *) NULL);
			return TCL_ERROR;
		    }
		    if (numValues <= 2) {
			optPtr->toX = values[0];
			optPtr->toY = values[1];
			optPtr->toX2 = -1;
			optPtr->toY2 = -1;
		    } else {
			optPtr->toX = MIN(values[0], values[2]);
			optPtr->toY = MIN(values[1], values[3]);
			optPtr->toX2 = MAX(values[0], values[2]);
			optPtr->toY2 = MAX(values[1], values[3]);
		    }
		    break;
		case OPT_ZOOM:
		    if ((values[0] <= 0) || (values[1] <= 0)) {
			Tcl_AppendResult(interp, "value(s) for the -zoom",
				" option must be positive", (char *) NULL);
			return TCL_ERROR;
		    }
		    optPtr->zoomX = values[0];
		    optPtr->zoomY = values[1];
		    break;
	    }
	}

	/*
	 * Remember that we saw this option.
	 */

	optPtr->options |= bit;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoConfigureMaster --
 *
 *	This procedure is called when a photo image is created or
 *	reconfigured.  It processes configuration options and resets
 *	any instances of the image.
 *
 * Results:
 *	A standard Tcl return value.  If TCL_ERROR is returned then
 *	an error message is left in the masterPtr->interp's result.
 *
 * Side effects:
 *	Existing instances of the image will be redisplayed to match
 *	the new configuration options.
 *
 *----------------------------------------------------------------------
 */

static int
ImgPhotoConfigureMaster(interp, masterPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Interpreter to use for reporting errors. */
    PhotoMaster *masterPtr;	/* Pointer to data structure describing
				 * overall photo image to (re)configure. */
    int objc;			/* Number of entries in objv. */
    Tcl_Obj *CONST objv[];	/* Pairs of configuration options for image. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget,
				 * such as TK_CONFIG_ARGV_ONLY. */
{
    PhotoInstance *instancePtr;
    CONST char *oldFileString, *oldPaletteString;
    Tcl_Obj *oldData, *data = NULL, *oldFormat, *format = NULL;
    int length, i, j;
    double oldGamma;
    int result;
    Tcl_Channel chan;
    Tk_PhotoImageFormat *imageFormat;
    int imageWidth, imageHeight;
    CONST char **args;
    int oldformat;
    Tcl_Obj *tempdata, *tempformat;

    args = (CONST char **) ckalloc((objc + 1) * sizeof(char *));
    for (i = 0, j = 0; i < objc; i++,j++) {
	args[j] = Tcl_GetStringFromObj(objv[i], &length);
	if ((length > 1) && (args[j][0] == '-')) {
	    if ((args[j][1] == 'd') &&
		    !strncmp(args[j], "-data", (size_t) length)) {
		if (++i < objc) {
		    data = objv[i];
		    j--;
		} else {
		    Tcl_AppendResult(interp,
			    "value for \"-data\" missing", (char *) NULL);
		    return TCL_ERROR;
		}
	    } else if ((args[j][1] == 'f') &&
		    !strncmp(args[j], "-format", (size_t) length)) {
		if (++i < objc) {
		    format = objv[i];
		    j--;
		} else {
		    Tcl_AppendResult(interp,
			    "value for \"-format\" missing", (char *) NULL);
		    return TCL_ERROR;
		}
	    }
	}
    }

    /*
     * Save the current values for fileString and dataString, so we
     * can tell if the user specifies them anew.
     * IMPORTANT: if the format changes we have to interpret
     * "-file" and "-data" again as well!!!!!!! It might be
     * that the format string influences how "-data" or "-file"
     * is interpreted.
     */

    oldFileString = masterPtr->fileString;
    if (oldFileString == NULL) {
	oldData = masterPtr->dataString;
	if (oldData != NULL) {
	    Tcl_IncrRefCount(oldData);
	}
    } else {
	oldData = NULL;
    }
    oldFormat = masterPtr->format;
    if (oldFormat != NULL) {
	Tcl_IncrRefCount(oldFormat);
    }
    oldPaletteString = masterPtr->palette;
    oldGamma = masterPtr->gamma;

    /*
     * Process the configuration options specified.
     */

    if (Tk_ConfigureWidget(interp, Tk_MainWindow(interp), configSpecs,
	    j, args, (char *) masterPtr, flags) != TCL_OK) {
	ckfree((char *) args);
	goto errorExit;
    }
    ckfree((char *) args);

    /*
     * Regard the empty string for -file, -data or -format as the null
     * value.
     */

    if ((masterPtr->fileString != NULL) && (masterPtr->fileString[0] == 0)) {
	ckfree(masterPtr->fileString);
	masterPtr->fileString = NULL;
    }
    if (data) {
	if (data->length
		|| (data->typePtr == Tcl_GetObjType("bytearray")
			&& data->internalRep.otherValuePtr != NULL)) {
	    Tcl_IncrRefCount(data);
	} else {
	    data = NULL;
	}
	if (masterPtr->dataString) {
	    Tcl_DecrRefCount(masterPtr->dataString);
	}
	masterPtr->dataString = data;
    }
    if (format) {
	if (format->length) {
	    Tcl_IncrRefCount(format);
	} else {
	    format = NULL;
	}
	if (masterPtr->format) {
	    Tcl_DecrRefCount(masterPtr->format);
	}
	masterPtr->format = format;
    }
    /*
     * Set the image to the user-requested size, if any,
     * and make sure storage is correctly allocated for this image.
     */

    if (ImgPhotoSetSize(masterPtr, masterPtr->width,
	    masterPtr->height) != TCL_OK) {
	Tcl_ResetResult(interp);
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		TK_PHOTO_ALLOC_FAILURE_MESSAGE, (char *) NULL);
	goto errorExit;
    }

    /*
     * Read in the image from the file or string if the user has
     * specified the -file or -data option.
     */

    if ((masterPtr->fileString != NULL)
	    && ((masterPtr->fileString != oldFileString)
	    || (masterPtr->format != oldFormat))) {

        /*
         * Prevent file system access in a safe interpreter.
         */

        if (Tcl_IsSafe(interp)) {
	    Tcl_ResetResult(interp);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "can't get image from a file in a safe interpreter",
		    (char *) NULL);
	    goto errorExit;
        }
        
	chan = Tcl_OpenFileChannel(interp, masterPtr->fileString, "r", 0);
	if (chan == NULL) {
	    goto errorExit;
	}
	/*
	 * -translation binary also sets -encoding binary
	 */
        if ((Tcl_SetChannelOption(interp, chan,
		"-translation", "binary") != TCL_OK) ||
		(MatchFileFormat(interp, chan, masterPtr->fileString,
			masterPtr->format, &imageFormat, &imageWidth,
			&imageHeight, &oldformat) != TCL_OK)) {
	    Tcl_Close(NULL, chan);
	    goto errorExit;
	}
	result = ImgPhotoSetSize(masterPtr, imageWidth, imageHeight);
	if (result != TCL_OK) {
	    Tcl_Close(NULL, chan);
	    Tcl_ResetResult(interp);
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    TK_PHOTO_ALLOC_FAILURE_MESSAGE, (char *) NULL);
	    goto errorExit;
	}
	tempformat = masterPtr->format;
	if (oldformat && tempformat) {
	    tempformat = (Tcl_Obj *) Tcl_GetString(tempformat);
	}
	result = (*imageFormat->fileReadProc)(interp, chan,
		masterPtr->fileString, tempformat,
		(Tk_PhotoHandle) masterPtr, 0, 0,
		imageWidth, imageHeight, 0, 0);
	Tcl_Close(NULL, chan);
	if (result != TCL_OK) {
	    goto errorExit;
	}

	Tcl_ResetResult(interp);
	masterPtr->flags |= IMAGE_CHANGED;
    }

    if ((masterPtr->fileString == NULL) && (masterPtr->dataString != NULL)
	    && ((masterPtr->dataString != oldData)
		    || (masterPtr->format != oldFormat))) {

	if (MatchStringFormat(interp, masterPtr->dataString,
		masterPtr->format, &imageFormat, &imageWidth,
		&imageHeight, &oldformat) != TCL_OK) {
	    goto errorExit;
	}
	if (ImgPhotoSetSize(masterPtr, imageWidth, imageHeight) != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    TK_PHOTO_ALLOC_FAILURE_MESSAGE, (char *) NULL);
	    goto errorExit;
	}
	tempformat = masterPtr->format;
	tempdata = masterPtr->dataString;
	if (oldformat) {
	    if (tempformat) {
		tempformat = (Tcl_Obj *) Tcl_GetString(tempformat);
	    }
	    tempdata = (Tcl_Obj *) Tcl_GetString(tempdata);
	}
	if ((*imageFormat->stringReadProc)(interp, tempdata,
		tempformat, (Tk_PhotoHandle) masterPtr,
		0, 0, imageWidth, imageHeight, 0, 0) != TCL_OK) {
	    goto errorExit;
	}

	Tcl_ResetResult(interp);
	masterPtr->flags |= IMAGE_CHANGED;
    }

    /*
     * Enforce a reasonable value for gamma.
     */

    if (masterPtr->gamma <= 0) {
	masterPtr->gamma = 1.0;
    }

    if ((masterPtr->gamma != oldGamma)
	    || (masterPtr->palette != oldPaletteString)) {
	masterPtr->flags |= IMAGE_CHANGED;
    }

    /*
     * Cycle through all of the instances of this image, regenerating
     * the information for each instance.  Then force the image to be
     * redisplayed everywhere that it is used.
     */

    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	ImgPhotoConfigureInstance(instancePtr);
    }

    /*
     * Inform the generic image code that the image
     * has (potentially) changed.
     */

    Tk_ImageChanged(masterPtr->tkMaster, 0, 0, masterPtr->width,
	    masterPtr->height, masterPtr->width, masterPtr->height);
    masterPtr->flags &= ~IMAGE_CHANGED;

    if (oldData != NULL) {
	Tcl_DecrRefCount(oldData);
    }
    if (oldFormat != NULL) {
	Tcl_DecrRefCount(oldFormat);
    }

    ToggleComplexAlphaIfNeeded(masterPtr);

    return TCL_OK;

  errorExit:
    if (oldData != NULL) {
	Tcl_DecrRefCount(oldData);
    }
    if (oldFormat != NULL) {
	Tcl_DecrRefCount(oldFormat);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoConfigureInstance --
 *
 *	This procedure is called to create displaying information for
 *	a photo image instance based on the configuration information
 *	in the master.  It is invoked both when new instances are
 *	created and when the master is reconfigured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates errors via Tcl_BackgroundError if there are problems
 *	in setting up the instance.
 *
 *----------------------------------------------------------------------
 */

static void
ImgPhotoConfigureInstance(instancePtr)
    PhotoInstance *instancePtr;	/* Instance to reconfigure. */
{
    PhotoMaster *masterPtr = instancePtr->masterPtr;
    XImage *imagePtr;
    int bitsPerPixel;
    ColorTable *colorTablePtr;
    XRectangle validBox;

    /*
     * If the -palette configuration option has been set for the master,
     * use the value specified for our palette, but only if it is
     * a valid palette for our windows.  Use the gamma value specified
     * the master.
     */

    if ((masterPtr->palette && masterPtr->palette[0])
	    && IsValidPalette(instancePtr, masterPtr->palette)) {
	instancePtr->palette = masterPtr->palette;
    } else {
	instancePtr->palette = instancePtr->defaultPalette;
    }
    instancePtr->gamma = masterPtr->gamma;

    /*
     * If we don't currently have a color table, or if the one we
     * have no longer applies (e.g. because our palette or gamma
     * has changed), get a new one.
     */

    colorTablePtr = instancePtr->colorTablePtr;
    if ((colorTablePtr == NULL)
	    || (instancePtr->colormap != colorTablePtr->id.colormap)
	    || (instancePtr->palette != colorTablePtr->id.palette)
	    || (instancePtr->gamma != colorTablePtr->id.gamma)) {
	/*
	 * Free up our old color table, and get a new one.
	 */

	if (colorTablePtr != NULL) {
	    colorTablePtr->liveRefCount -= 1;
	    FreeColorTable(colorTablePtr, 0);
	}
	GetColorTable(instancePtr);

	/*
	 * Create a new XImage structure for sending data to
	 * the X server, if necessary.
	 */

	if (instancePtr->colorTablePtr->flags & BLACK_AND_WHITE) {
	    bitsPerPixel = 1;
	} else {
	    bitsPerPixel = instancePtr->visualInfo.depth;
	}

	if ((instancePtr->imagePtr == NULL)
		|| (instancePtr->imagePtr->bits_per_pixel != bitsPerPixel)) {
	    if (instancePtr->imagePtr != NULL) {
		XFree((char *) instancePtr->imagePtr);
	    }
	    imagePtr = XCreateImage(instancePtr->display,
		    instancePtr->visualInfo.visual, (unsigned) bitsPerPixel,
		    (bitsPerPixel > 1? ZPixmap: XYBitmap), 0, (char *) NULL,
		    1, 1, 32, 0);
	    instancePtr->imagePtr = imagePtr;

	    /*
	     * Determine the endianness of this machine.
	     * We create images using the local host's endianness, rather
	     * than the endianness of the server; otherwise we would have
	     * to byte-swap any 16 or 32 bit values that we store in the
	     * image in those situations where the server's endianness
	     * is different from ours.
	     *
	     * FIXME: use autoconf to figure this out.
	     */

	    if (imagePtr != NULL) {
		union {
		    int i;
		    char c[sizeof(int)];
		} kludge;

		imagePtr->bitmap_unit = sizeof(pixel) * NBBY;
		kludge.i = 0;
		kludge.c[0] = 1;
		imagePtr->byte_order = (kludge.i == 1) ? LSBFirst : MSBFirst;
		_XInitImageFuncPtrs(imagePtr);
	    }
	}
    }

    /*
     * If the user has specified a width and/or height for the master
     * which is different from our current width/height, set the size
     * to the values specified by the user.  If we have no pixmap, we
     * do this also, since it has the side effect of allocating a
     * pixmap for us.
     */

    if ((instancePtr->pixels == None) || (instancePtr->error == NULL)
	    || (instancePtr->width != masterPtr->width)
	    || (instancePtr->height != masterPtr->height)) {
	ImgPhotoInstanceSetSize(instancePtr);
    }

    /*
     * Redither this instance if necessary.
     */

    if ((masterPtr->flags & IMAGE_CHANGED)
	    || (instancePtr->colorTablePtr != colorTablePtr)) {
	TkClipBox(masterPtr->validRegion, &validBox);
	if ((validBox.width > 0) && (validBox.height > 0)) {
	    DitherInstance(instancePtr, validBox.x, validBox.y,
		    validBox.width, validBox.height);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoGet --
 *
 *	This procedure is called for each use of a photo image in a
 *	widget.
 *
 * Results:
 *	The return value is a token for the instance, which is passed
 *	back to us in calls to ImgPhotoDisplay and ImgPhotoFree.
 *
 * Side effects:
 *	A data structure is set up for the instance (or, an existing
 *	instance is re-used for the new one).
 *
 *----------------------------------------------------------------------
 */

static ClientData
ImgPhotoGet(tkwin, masterData)
    Tk_Window tkwin;		/* Window in which the instance will be
				 * used. */
    ClientData masterData;	/* Pointer to our master structure for the
				 * image. */
{
    PhotoMaster *masterPtr = (PhotoMaster *) masterData;
    PhotoInstance *instancePtr;
    Colormap colormap;
    int mono, nRed, nGreen, nBlue;
    XVisualInfo visualInfo, *visInfoPtr;
    char buf[TCL_INTEGER_SPACE * 3];
    int numVisuals;
    XColor *white, *black;
    XGCValues gcValues;

    /*
     * Table of "best" choices for palette for PseudoColor displays
     * with between 3 and 15 bits/pixel.
     */

    static int paletteChoice[13][3] = {
	/*  #red, #green, #blue */
	 {2,  2,  2,			/* 3 bits, 8 colors */},
	 {2,  3,  2,			/* 4 bits, 12 colors */},
	 {3,  4,  2,			/* 5 bits, 24 colors */},
	 {4,  5,  3,			/* 6 bits, 60 colors */},
	 {5,  6,  4,			/* 7 bits, 120 colors */},
	 {7,  7,  4,			/* 8 bits, 198 colors */},
	 {8, 10,  6,			/* 9 bits, 480 colors */},
	{10, 12,  8,			/* 10 bits, 960 colors */},
	{14, 15,  9,			/* 11 bits, 1890 colors */},
	{16, 20, 12,			/* 12 bits, 3840 colors */},
	{20, 24, 16,			/* 13 bits, 7680 colors */},
	{26, 30, 20,			/* 14 bits, 15600 colors */},
	{32, 32, 30,			/* 15 bits, 30720 colors */}
    };

    /*
     * See if there is already an instance for windows using
     * the same colormap.  If so then just re-use it.
     */

    colormap = Tk_Colormap(tkwin);
    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	if ((colormap == instancePtr->colormap)
		&& (Tk_Display(tkwin) == instancePtr->display)) {

	    /*
	     * Re-use this instance.
	     */

	    if (instancePtr->refCount == 0) {
		/*
		 * We are resurrecting this instance.
		 */

		Tcl_CancelIdleCall(DisposeInstance, (ClientData) instancePtr);
		if (instancePtr->colorTablePtr != NULL) {
		    FreeColorTable(instancePtr->colorTablePtr, 0);
		}
		GetColorTable(instancePtr);
	    }
	    instancePtr->refCount++;
	    return (ClientData) instancePtr;
	}
    }

    /*
     * The image isn't already in use in a window with the same colormap.
     * Make a new instance of the image.
     */

    instancePtr = (PhotoInstance *) ckalloc(sizeof(PhotoInstance));
    instancePtr->masterPtr = masterPtr;
    instancePtr->display = Tk_Display(tkwin);
    instancePtr->colormap = Tk_Colormap(tkwin);
    Tk_PreserveColormap(instancePtr->display, instancePtr->colormap);
    instancePtr->refCount = 1;
    instancePtr->colorTablePtr = NULL;
    instancePtr->pixels = None;
    instancePtr->error = NULL;
    instancePtr->width = 0;
    instancePtr->height = 0;
    instancePtr->imagePtr = 0;
    instancePtr->nextPtr = masterPtr->instancePtr;
    masterPtr->instancePtr = instancePtr;

    /*
     * Obtain information about the visual and decide on the
     * default palette.
     */

    visualInfo.screen = Tk_ScreenNumber(tkwin);
    visualInfo.visualid = XVisualIDFromVisual(Tk_Visual(tkwin));
    visInfoPtr = XGetVisualInfo(Tk_Display(tkwin),
	    VisualScreenMask | VisualIDMask, &visualInfo, &numVisuals);
    nRed = 2;
    nGreen = nBlue = 0;
    mono = 1;
    if (visInfoPtr != NULL) {
	instancePtr->visualInfo = *visInfoPtr;
	switch (visInfoPtr->class) {
	    case DirectColor:
	    case TrueColor:
		nRed = 1 << CountBits(visInfoPtr->red_mask);
		nGreen = 1 << CountBits(visInfoPtr->green_mask);
		nBlue = 1 << CountBits(visInfoPtr->blue_mask);
		mono = 0;
		break;
	    case PseudoColor:
	    case StaticColor:
		if (visInfoPtr->depth > 15) {
		    nRed = 32;
		    nGreen = 32;
		    nBlue = 32;
		    mono = 0;
		} else if (visInfoPtr->depth >= 3) {
		    int *ip = paletteChoice[visInfoPtr->depth - 3];
    
		    nRed = ip[0];
		    nGreen = ip[1];
		    nBlue = ip[2];
		    mono = 0;
		}
		break;
	    case GrayScale:
	    case StaticGray:
		nRed = 1 << visInfoPtr->depth;
		break;
	}
	XFree((char *) visInfoPtr);

    } else {
	panic("ImgPhotoGet couldn't find visual for window");
    }

    sprintf(buf, ((mono) ? "%d": "%d/%d/%d"), nRed, nGreen, nBlue);
    instancePtr->defaultPalette = Tk_GetUid(buf);

    /*
     * Make a GC with background = black and foreground = white.
     */

    white = Tk_GetColor(masterPtr->interp, tkwin, "white");
    black = Tk_GetColor(masterPtr->interp, tkwin, "black");
    gcValues.foreground = (white != NULL)? white->pixel:
	    WhitePixelOfScreen(Tk_Screen(tkwin));
    gcValues.background = (black != NULL)? black->pixel:
	    BlackPixelOfScreen(Tk_Screen(tkwin));
    gcValues.graphics_exposures = False;
    instancePtr->gc = Tk_GetGC(tkwin,
	    GCForeground|GCBackground|GCGraphicsExposures, &gcValues);

    /*
     * Set configuration options and finish the initialization of the instance.
     * This will also dither the image if necessary.
     */

    ImgPhotoConfigureInstance(instancePtr);

    /*
     * If this is the first instance, must set the size of the image.
     */

    if (instancePtr->nextPtr == NULL) {
	Tk_ImageChanged(masterPtr->tkMaster, 0, 0, 0, 0,
		masterPtr->width, masterPtr->height);
    }

    return (ClientData) instancePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ToggleComplexAlphaIfNeeded --
 *
 *	This procedure is called when an image is modified to
 *	check if any partially transparent pixels exist, which
 *	requires blending instead of straight copy.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	(Re)sets COMPLEX_ALPHA flag of master.
 *
 *----------------------------------------------------------------------
 */

static int
ToggleComplexAlphaIfNeeded(PhotoMaster *mPtr)
{
    size_t len = MAX(mPtr->userWidth, mPtr->width) *
	MAX(mPtr->userHeight, mPtr->height) * 4;
    unsigned char *c   = mPtr->pix32;
    unsigned char *end = c + len;

    /*
     * Set the COMPLEX_ALPHA flag if we have an image with partially
     * transparent bits.
     */
    mPtr->flags &= ~COMPLEX_ALPHA;
    c += 3; /* start at first alpha byte */
    for (; c < end; c += 4) {
	if (*c && *c != 255) {
     	    mPtr->flags |= COMPLEX_ALPHA;
	    break;
	}
    }
    return (mPtr->flags & COMPLEX_ALPHA);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoBlendComplexAlpha --
 *
 *	This procedure is called when an image with partially
 *	transparent pixels must be drawn over another image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Background image passed in gets drawn over with image data.
 *
 *----------------------------------------------------------------------
 */
/*
 * This should work on all platforms that set mask and shift data properly
 * from the visualInfo.
 * RGB is really only a 24+ bpp version whereas RGB15 is the correct version
 * and works for 15bpp+, but it slower, so it's only used for 15bpp+.
 */
#ifndef __WIN32__
#define GetRValue(rgb)	(UCHAR((rgb & red_mask) >> red_shift))
#define GetGValue(rgb)	(UCHAR((rgb & green_mask) >> green_shift))
#define GetBValue(rgb)	(UCHAR((rgb & blue_mask) >> blue_shift))
#define RGB(r,g,b)      ((unsigned)((UCHAR(r)<<red_shift)|(UCHAR(g)<<green_shift)|(UCHAR(b)<<blue_shift)))
#define RGB15(r,g,b)    ((unsigned)(((r*red_mask/255)&red_mask)|((g*green_mask/255)&green_mask)|((b*blue_mask/255)&blue_mask)))
#endif

static void ImgPhotoBlendComplexAlpha (
    XImage *bgImg,            /* background image to draw on */
    PhotoInstance *iPtr,      /* image instance to draw */
    int xOffset, int yOffset, /* X & Y offset into image instance to draw */
    int width, int height     /* width & height of image to draw */
    )
{
    int x, y, line;
    unsigned long pixel;
    unsigned char r, g, b, alpha, unalpha;
    unsigned char *alphaAr = iPtr->masterPtr->pix32;
    unsigned char *masterPtr;

#ifndef __WIN32__
    /*
     * We have to get the mask and shift info from the visual.
     * This might be cached for better performance.
     */
    unsigned long red_mask, green_mask, blue_mask;
    unsigned long red_shift, green_shift, blue_shift;
    Visual *visual = iPtr->visualInfo.visual;

    red_mask    = visual->red_mask;
    green_mask  = visual->green_mask;
    blue_mask   = visual->blue_mask;
    red_shift   = 0;
    green_shift = 0;
    blue_shift  = 0;
    while ((0x0001 & (red_mask >> red_shift)) == 0)	red_shift++;
    while ((0x0001 & (green_mask >> green_shift)) == 0)	green_shift++;
    while ((0x0001 & (blue_mask >> blue_shift)) == 0)	blue_shift++;
#endif

#define ALPHA_BLEND(bgPix, imgPix, alpha, unalpha) \
		((bgPix * unalpha + imgPix * alpha) / 255)

#if !(defined(__WIN32__) || defined(MAC_OSX_TK))
    /*
     * Only unix requires the special case for <24bpp.  It varies with
     * 3 extra shifts and uses RGB15.  The 24+bpp version could also
     * then be further optimized.
     */
    if (bgImg->depth < 24) {
	unsigned char red_mlen, green_mlen, blue_mlen;

	red_mlen   = 8 - CountBits(red_mask >> red_shift);
	green_mlen = 8 - CountBits(green_mask >> green_shift);
	blue_mlen  = 8 - CountBits(blue_mask >> blue_shift);
	for (y = 0; y < height; y++) {
	    line = (y + yOffset) * iPtr->masterPtr->width;
	    for (x = 0; x < width; x++) {
		masterPtr = alphaAr + ((line + x + xOffset) * 4);
		alpha     = masterPtr[3];
		/*
		 * Ignore pixels that are fully transparent
		 */
		if (alpha) {
		    /*
		     * We could perhaps be more efficient than XGetPixel for
		     * 24 and 32 bit displays, but this seems "fast enough".
		     */
		    r = masterPtr[0];
		    g = masterPtr[1];
		    b = masterPtr[2];
		    if (alpha != 255) {
			/*
			 * Only blend pixels that have some transparency
			 */
			unsigned char ra, ga, ba;

			pixel = XGetPixel(bgImg, x, y);
			ra = GetRValue(pixel) << red_mlen;
			ga = GetGValue(pixel) << green_mlen;
			ba = GetBValue(pixel) << blue_mlen;
			unalpha = 255 - alpha;
			r = ALPHA_BLEND(ra, r, alpha, unalpha);
			g = ALPHA_BLEND(ga, g, alpha, unalpha);
			b = ALPHA_BLEND(ba, b, alpha, unalpha);
		    }
		    XPutPixel(bgImg, x, y, RGB15(r, g, b));
		}
	    }
	}
    } else
#endif
	for (y = 0; y < height; y++) {
	    line = (y + yOffset) * iPtr->masterPtr->width;
	    for (x = 0; x < width; x++) {
		masterPtr = alphaAr + ((line + x + xOffset) * 4);
		alpha     = masterPtr[3];
		/*
		 * Ignore pixels that are fully transparent
		 */
		if (alpha) {
		    /*
		     * We could perhaps be more efficient than XGetPixel for
		     * 24 and 32 bit displays, but this seems "fast enough".
		     */
		    r = masterPtr[0];
		    g = masterPtr[1];
		    b = masterPtr[2];
		    if (alpha != 255) {
			/*
			 * Only blend pixels that have some transparency
			 */
			unsigned char ra, ga, ba;

			pixel = XGetPixel(bgImg, x, y);
			ra = GetRValue(pixel);
			ga = GetGValue(pixel);
			ba = GetBValue(pixel);
			unalpha = 255 - alpha;
			r = ALPHA_BLEND(ra, r, alpha, unalpha);
			g = ALPHA_BLEND(ga, g, alpha, unalpha);
			b = ALPHA_BLEND(ba, b, alpha, unalpha);
		    }
		    XPutPixel(bgImg, x, y, RGB(r, g, b));
		}
	    }
	}
#undef ALPHA_BLEND
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoDisplay --
 *
 *	This procedure is invoked to draw a photo image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A portion of the image gets rendered in a pixmap or window.
 *
 *----------------------------------------------------------------------
 */

static void
ImgPhotoDisplay(clientData, display, drawable, imageX, imageY, width,
	height, drawableX, drawableY)
    ClientData clientData;	/* Pointer to PhotoInstance structure for
				 * for instance to be displayed. */
    Display *display;		/* Display on which to draw image. */
    Drawable drawable;		/* Pixmap or window in which to draw image. */
    int imageX, imageY;		/* Upper-left corner of region within image
				 * to draw. */
    int width, height;		/* Dimensions of region within image to draw. */
    int drawableX, drawableY;	/* Coordinates within drawable that
				 * correspond to imageX and imageY. */
{
    PhotoInstance *instancePtr = (PhotoInstance *) clientData;
    XVisualInfo visInfo = instancePtr->visualInfo;

    /*
     * If there's no pixmap, it means that an error occurred
     * while creating the image instance so it can't be displayed.
     */

    if (instancePtr->pixels == None) {
	return;
    }

    if (
#if defined(MAC_TCL) || defined(MAC_OSX_TK)
	/*
	 * The retrieval of bgImg is currently not functional on OSX
	 * (and likely not OS9 either), so skip attempts to alpha blend.
	 */
	0 &&
#endif
	(instancePtr->masterPtr->flags & COMPLEX_ALPHA)
	    && visInfo.depth >= 15
	    && (visInfo.class == DirectColor || visInfo.class == TrueColor)) {
	XImage *bgImg = NULL;

	/*
	 * Pull the current background from the display to blend with
	 */
	bgImg = XGetImage(display, drawable, drawableX, drawableY,
		(unsigned int)width, (unsigned int)height, AllPlanes, ZPixmap);
	if (bgImg == NULL) {
	    return;
	}

	ImgPhotoBlendComplexAlpha(bgImg, instancePtr,
		imageX, imageY, width, height);

	/*
	 * Color info is unimportant as we only do this operation for
	 * depth >= 15.
	 */
	TkPutImage(NULL, 0, display, drawable, instancePtr->gc,
		bgImg, 0, 0, drawableX, drawableY,
		(unsigned int) width, (unsigned int) height);
	XDestroyImage(bgImg);
    } else {
	/*
	 * masterPtr->region describes which parts of the image contain
	 * valid data.  We set this region as the clip mask for the gc,
	 * setting its origin appropriately, and use it when drawing the
	 * image.
	 */
	TkSetRegion(display, instancePtr->gc, instancePtr->masterPtr->validRegion);
	XSetClipOrigin(display, instancePtr->gc, drawableX - imageX,
	               drawableY - imageY);
	XCopyArea(display, instancePtr->pixels, drawable, instancePtr->gc,
	          imageX, imageY, (unsigned) width, (unsigned) height,
	          drawableX, drawableY);
	XSetClipMask(display, instancePtr->gc, None);
	XSetClipOrigin(display, instancePtr->gc, 0, 0);
    }
    XFlush (display);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoFree --
 *
 *	This procedure is called when a widget ceases to use a
 *	particular instance of an image.  We don't actually get
 *	rid of the instance until later because we may be about
 *	to get this instance again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Internal data structures get cleaned up, later.
 *
 *----------------------------------------------------------------------
 */

static void
ImgPhotoFree(clientData, display)
    ClientData clientData;	/* Pointer to PhotoInstance structure for
				 * for instance to be displayed. */
    Display *display;		/* Display containing window that used image. */
{
    PhotoInstance *instancePtr = (PhotoInstance *) clientData;
    ColorTable *colorPtr;

    instancePtr->refCount -= 1;
    if (instancePtr->refCount > 0) {
	return;
    }

    /*
     * There are no more uses of the image within this widget.
     * Decrement the count of live uses of its color table, so
     * that its colors can be reclaimed if necessary, and
     * set up an idle call to free the instance structure.
     */

    colorPtr = instancePtr->colorTablePtr;
    if (colorPtr != NULL) {
	colorPtr->liveRefCount -= 1;
    }
    
    Tcl_DoWhenIdle(DisposeInstance, (ClientData) instancePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoDelete --
 *
 *	This procedure is called by the image code to delete the
 *	master structure for an image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with the image get freed.
 *
 *----------------------------------------------------------------------
 */

static void
ImgPhotoDelete(masterData)
    ClientData masterData;	/* Pointer to PhotoMaster structure for
				 * image.  Must not have any more instances. */
{
    PhotoMaster *masterPtr = (PhotoMaster *) masterData;
    PhotoInstance *instancePtr;

    while ((instancePtr = masterPtr->instancePtr) != NULL) {
	if (instancePtr->refCount > 0) {
	    panic("tried to delete photo image when instances still exist");
	}
	Tcl_CancelIdleCall(DisposeInstance, (ClientData) instancePtr);
	DisposeInstance((ClientData) instancePtr);
    }
    masterPtr->tkMaster = NULL;
    if (masterPtr->imageCmd != NULL) {
	Tcl_DeleteCommandFromToken(masterPtr->interp, masterPtr->imageCmd);
    }
    if (masterPtr->pix32 != NULL) {
	ckfree((char *) masterPtr->pix32);
    }
    if (masterPtr->validRegion != NULL) {
	TkDestroyRegion(masterPtr->validRegion);
    }
    if (masterPtr->dataString != NULL) {
	Tcl_DecrRefCount(masterPtr->dataString);
    }
    if (masterPtr->format != NULL) {
	Tcl_DecrRefCount(masterPtr->format);
    }
    Tk_FreeOptions(configSpecs, (char *) masterPtr, (Display *) NULL, 0);
    ckfree((char *) masterPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoCmdDeletedProc --
 *
 *	This procedure is invoked when the image command for an image
 *	is deleted.  It deletes the image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
ImgPhotoCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to PhotoMaster structure for
				 * image. */
{
    PhotoMaster *masterPtr = (PhotoMaster *) clientData;

    masterPtr->imageCmd = NULL;
    if (masterPtr->tkMaster != NULL) {
	Tk_DeleteImage(masterPtr->interp, Tk_NameOfImage(masterPtr->tkMaster));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoSetSize --
 *
 *	This procedure reallocates the image storage and instance
 *	pixmaps for a photo image, as necessary, to change the
 *	image's size to `width' x `height' pixels.
 *
 * Results:
 *	TCL_OK if successful, TCL_ERROR if failure occurred (currently
 *	just with memory allocation.)
 *
 * Side effects:
 *	Storage gets reallocated, for the master and all its instances.
 *
 *----------------------------------------------------------------------
 */

static int
ImgPhotoSetSize(masterPtr, width, height)
    PhotoMaster *masterPtr;
    int width, height;
{
    unsigned char *newPix32 = NULL;
    int h, offset, pitch;
    unsigned char *srcPtr, *destPtr;
    XRectangle validBox, clipBox;
    TkRegion clipRegion;
    PhotoInstance *instancePtr;

    if (masterPtr->userWidth > 0) {
	width = masterPtr->userWidth;
    }
    if (masterPtr->userHeight > 0) {
	height = masterPtr->userHeight;
    }

    pitch = width * 4;

    /*
     * Test if we're going to (re)allocate the main buffer now, so
     * that any failures will leave the photo unchanged.
     */
    if ((width != masterPtr->width) || (height != masterPtr->height)
	    || (masterPtr->pix32 == NULL)) {
	/*
	 * Not a u-long, but should be one.
	 */
	unsigned /*long*/ newPixSize = (unsigned /*long*/) (height * pitch);

	/*
	 * Some mallocs() really hate allocating zero bytes. [Bug 619544]
	 */
	if (newPixSize == 0) {
	    newPix32 = NULL;
	} else {
	    newPix32 = (unsigned char *) attemptckalloc(newPixSize);
	    if (newPix32 == NULL) {
		return TCL_ERROR;
	    }
	}
    }

    /*
     * We have to trim the valid region if it is currently
     * larger than the new image size.
     */

    TkClipBox(masterPtr->validRegion, &validBox);
    if ((validBox.x + validBox.width > width)
	    || (validBox.y + validBox.height > height)) {
	clipBox.x = 0;
	clipBox.y = 0;
	clipBox.width = width;
	clipBox.height = height;
	clipRegion = TkCreateRegion();
	TkUnionRectWithRegion(&clipBox, clipRegion, clipRegion);
	TkIntersectRegion(masterPtr->validRegion, clipRegion,
		masterPtr->validRegion);
	TkDestroyRegion(clipRegion);
	TkClipBox(masterPtr->validRegion, &validBox);
    }

    /*
     * Use the reallocated storage (allocation above) for the 32-bit
     * image and copy over valid regions.  Note that this test is true
     * precisely when the allocation has already been done.
     */
    if (newPix32 != NULL) {
	/*
	 * Zero the new array.  The dithering code shouldn't read the
	 * areas outside validBox, but they might be copied to another
	 * photo image or written to a file.
	 */

	if ((masterPtr->pix32 != NULL)
	    && ((width == masterPtr->width) || (width == validBox.width))) {
	    if (validBox.y > 0) {
		memset((VOID *) newPix32, 0, (size_t) (validBox.y * pitch));
	    }
	    h = validBox.y + validBox.height;
	    if (h < height) {
		memset((VOID *) (newPix32 + h * pitch), 0,
			(size_t) ((height - h) * pitch));
	    }
	} else {
	    memset((VOID *) newPix32, 0, (size_t) (height * pitch));
	}

	if (masterPtr->pix32 != NULL) {

	    /*
	     * Copy the common area over to the new array array and
	     * free the old array.
	     */

	    if (width == masterPtr->width) {

		/*
		 * The region to be copied is contiguous.
		 */

		offset = validBox.y * pitch;
		memcpy((VOID *) (newPix32 + offset),
			(VOID *) (masterPtr->pix32 + offset),
			(size_t) (validBox.height * pitch));

	    } else if ((validBox.width > 0) && (validBox.height > 0)) {

		/*
		 * Area to be copied is not contiguous - copy line by line.
		 */

		destPtr = newPix32 + (validBox.y * width + validBox.x) * 4;
		srcPtr = masterPtr->pix32 + (validBox.y * masterPtr->width
			+ validBox.x) * 4;
		for (h = validBox.height; h > 0; h--) {
		    memcpy((VOID *) destPtr, (VOID *) srcPtr,
			    (size_t) (validBox.width * 4));
		    destPtr += width * 4;
		    srcPtr += masterPtr->width * 4;
		}
	    }

	    ckfree((char *) masterPtr->pix32);
	}

	masterPtr->pix32 = newPix32;
	masterPtr->width = width;
	masterPtr->height = height;

	/*
	 * Dithering will be correct up to the end of the last
	 * pre-existing complete scanline.
	 */

	if ((validBox.x > 0) || (validBox.y > 0)) {
	    masterPtr->ditherX = 0;
	    masterPtr->ditherY = 0;
	} else if (validBox.width == width) {
	    if ((int) validBox.height < masterPtr->ditherY) {
		masterPtr->ditherX = 0;
		masterPtr->ditherY = validBox.height;
	    }
	} else if ((masterPtr->ditherY > 0)
		|| ((int) validBox.width < masterPtr->ditherX)) {
	    masterPtr->ditherX = validBox.width;
	    masterPtr->ditherY = 0;
	}
    }

    ToggleComplexAlphaIfNeeded(masterPtr);

    /*
     * Now adjust the sizes of the pixmaps for all of the instances.
     */

    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	ImgPhotoInstanceSetSize(instancePtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgPhotoInstanceSetSize --
 *
 * 	This procedure reallocates the instance pixmap and dithering
 *	error array for a photo instance, as necessary, to change the
 *	image's size to `width' x `height' pixels.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage gets reallocated, here and in the X server.
 *
 *----------------------------------------------------------------------
 */

static void
ImgPhotoInstanceSetSize(instancePtr)
    PhotoInstance *instancePtr;		/* Instance whose size is to be
					 * changed. */
{
    PhotoMaster *masterPtr;
    schar *newError;
    schar *errSrcPtr, *errDestPtr;
    int h, offset;
    XRectangle validBox;
    Pixmap newPixmap;

    masterPtr = instancePtr->masterPtr;
    TkClipBox(masterPtr->validRegion, &validBox);

    if ((instancePtr->width != masterPtr->width)
	    || (instancePtr->height != masterPtr->height)
	    || (instancePtr->pixels == None)) {
	newPixmap = Tk_GetPixmap(instancePtr->display,
		RootWindow(instancePtr->display,
		    instancePtr->visualInfo.screen),
		(masterPtr->width > 0) ? masterPtr->width: 1,
		(masterPtr->height > 0) ? masterPtr->height: 1,
		instancePtr->visualInfo.depth);
        if (!newPixmap) {
            panic("Fail to create pixmap with Tk_GetPixmap in ImgPhotoInstanceSetSize.\n");
            return;
        }

	/*
	 * The following is a gross hack needed to properly support colormaps
	 * under Windows.  Before the pixels can be copied to the pixmap,
	 * the relevent colormap must be associated with the drawable.
	 * Normally we can infer this association from the window that
	 * was used to create the pixmap.  However, in this case we're
	 * using the root window, so we have to be more explicit.
	 */

	TkSetPixmapColormap(newPixmap, instancePtr->colormap);

	if (instancePtr->pixels != None) {
	    /*
	     * Copy any common pixels from the old pixmap and free it.
	     */
	    XCopyArea(instancePtr->display, instancePtr->pixels, newPixmap,
		    instancePtr->gc, validBox.x, validBox.y,
		    validBox.width, validBox.height, validBox.x, validBox.y);
	    Tk_FreePixmap(instancePtr->display, instancePtr->pixels);
	}
	instancePtr->pixels = newPixmap;
    }

    if ((instancePtr->width != masterPtr->width)
	    || (instancePtr->height != masterPtr->height)
	    || (instancePtr->error == NULL)) {

	if (masterPtr->height > 0 && masterPtr->width > 0) {
	    newError = (schar *) ckalloc((unsigned)
		    masterPtr->height * masterPtr->width * 3 * sizeof(schar));

	    /*
	     * Zero the new array so that we don't get bogus error
	     * values propagating into areas we dither later.
	     */

	    if ((instancePtr->error != NULL)
		    && ((instancePtr->width == masterPtr->width)
		    || (validBox.width == masterPtr->width))) {
		if (validBox.y > 0) {
		    memset((VOID *) newError, 0, (size_t)
			    validBox.y * masterPtr->width * 3 * sizeof(schar));
		}
		h = validBox.y + validBox.height;
		if (h < masterPtr->height) {
		    memset((VOID *) (newError + h * masterPtr->width * 3), 0,
			    (size_t) (masterPtr->height - h)
			    * masterPtr->width * 3 * sizeof(schar));
		}
	    } else {
		memset((VOID *) newError, 0, (size_t)
			masterPtr->height * masterPtr->width * 3 * sizeof(schar));
	    }
	} else {
	    newError = NULL;
	}

	if (instancePtr->error != NULL) {

	    /*
	     * Copy the common area over to the new array
	     * and free the old array.
	     */

	    if (masterPtr->width == instancePtr->width) {

		offset = validBox.y * masterPtr->width * 3;
		memcpy((VOID *) (newError + offset),
			(VOID *) (instancePtr->error + offset),
			(size_t) (validBox.height
			* masterPtr->width * 3 * sizeof(schar)));

	    } else if (validBox.width > 0 && validBox.height > 0) {

		errDestPtr = newError
			+ (validBox.y * masterPtr->width + validBox.x) * 3;
		errSrcPtr = instancePtr->error
			+ (validBox.y * instancePtr->width + validBox.x) * 3;
		for (h = validBox.height; h > 0; --h) {
		    memcpy((VOID *) errDestPtr, (VOID *) errSrcPtr,
			    validBox.width * 3 * sizeof(schar));
		    errDestPtr += masterPtr->width * 3;
		    errSrcPtr += instancePtr->width * 3;
		}
	    }
	    ckfree((char *) instancePtr->error);
	}

	instancePtr->error = newError;
    }

    instancePtr->width = masterPtr->width;
    instancePtr->height = masterPtr->height;
}

/*
 *----------------------------------------------------------------------
 *
 * IsValidPalette --
 *
 *	This procedure is called to check whether a value given for
 *	the -palette option is valid for a particular instance
 * 	of a photo image.
 *
 * Results:
 *	A boolean value: 1 if the palette is acceptable, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
IsValidPalette(instancePtr, palette)
    PhotoInstance *instancePtr;		/* Instance to which the palette
					 * specification is to be applied. */
    CONST char *palette;		/* Palette specification string. */
{
    int nRed, nGreen, nBlue, mono, numColors;
    char *endp;

    /*
     * First parse the specification: it must be of the form
     * %d or %d/%d/%d.
     */

    nRed = strtol(palette, &endp, 10);
    if ((endp == palette) || ((*endp != 0) && (*endp != '/'))
	    || (nRed < 2) || (nRed > 256)) {
	return 0;
    }

    if (*endp == 0) {
	mono = 1;
	nGreen = nBlue = nRed;
    } else {
	palette = endp + 1;
	nGreen = strtol(palette, &endp, 10);
	if ((endp == palette) || (*endp != '/') || (nGreen < 2)
		|| (nGreen > 256)) {
	    return 0;
	}
	palette = endp + 1;
	nBlue = strtol(palette, &endp, 10);
	if ((endp == palette) || (*endp != 0) || (nBlue < 2)
		|| (nBlue > 256)) {
	    return 0;
	}
	mono = 0;
    }

    switch (instancePtr->visualInfo.class) {
	case DirectColor:
	case TrueColor:
	    if ((nRed > (1 << CountBits(instancePtr->visualInfo.red_mask)))
		    || (nGreen > (1
			<< CountBits(instancePtr->visualInfo.green_mask)))
		    || (nBlue > (1
			<< CountBits(instancePtr->visualInfo.blue_mask)))) {
		return 0;
	    }
	    break;
	case PseudoColor:
	case StaticColor:
	    numColors = nRed;
	    if (!mono) {
		numColors *= nGreen*nBlue;
	    }
	    if (numColors > (1 << instancePtr->visualInfo.depth)) {
		return 0;
	    }
	    break;
	case GrayScale:
	case StaticGray:
	    if (!mono || (nRed > (1 << instancePtr->visualInfo.depth))) {
		return 0;
	    }
	    break;
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * CountBits --
 *
 *	This procedure counts how many bits are set to 1 in `mask'.
 *
 * Results:
 *	The integer number of bits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CountBits(mask)
    pixel mask;			/* Value to count the 1 bits in. */
{
    int n;

    for( n = 0; mask != 0; mask &= mask - 1 )
	n++;
    return n;
}

/*
 *----------------------------------------------------------------------
 *
 * GetColorTable --
 *
 *	This procedure is called to allocate a table of colormap
 *	information for an instance of a photo image.  Only one such
 *	table is allocated for all photo instances using the same
 *	display, colormap, palette and gamma values, so that the
 *	application need only request a set of colors from the X
 *	server once for all such photo widgets.  This procedure
 *	maintains a hash table to find previously-allocated
 *	ColorTables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new ColorTable may be allocated and placed in the hash
 *	table, and have colors allocated for it.
 *
 *----------------------------------------------------------------------
 */

static void
GetColorTable(instancePtr)
    PhotoInstance *instancePtr;		/* Instance needing a color table. */
{
    ColorTable *colorPtr;
    Tcl_HashEntry *entry;
    ColorTableId id;
    int isNew;

    /*
     * Look for an existing ColorTable in the hash table.
     */

    memset((VOID *) &id, 0, sizeof(id));
    id.display = instancePtr->display;
    id.colormap = instancePtr->colormap;
    id.palette = instancePtr->palette;
    id.gamma = instancePtr->gamma;
    if (!imgPhotoColorHashInitialized) {
	Tcl_InitHashTable(&imgPhotoColorHash, N_COLOR_HASH);
	imgPhotoColorHashInitialized = 1;
    }
    entry = Tcl_CreateHashEntry(&imgPhotoColorHash, (char *) &id, &isNew);

    if (!isNew) {
	/*
	 * Re-use the existing entry.
	 */

	colorPtr = (ColorTable *) Tcl_GetHashValue(entry);

    } else {
	/*
	 * No color table currently available; need to make one.
	 */

	colorPtr = (ColorTable *) ckalloc(sizeof(ColorTable));

	/*
	 * The following line of code should not normally be needed due
	 * to the assignment in the following line.  However, it compensates
	 * for bugs in some compilers (HP, for example) where
	 * sizeof(ColorTable) is 24 but the assignment only copies 20 bytes,
	 * leaving 4 bytes uninitialized;  these cause problems when using
	 * the id for lookups in imgPhotoColorHash, and can result in
	 * core dumps.
	 */

	memset((VOID *) &colorPtr->id, 0, sizeof(ColorTableId));
	colorPtr->id = id;
	Tk_PreserveColormap(colorPtr->id.display, colorPtr->id.colormap);
	colorPtr->flags = 0;
	colorPtr->refCount = 0;
	colorPtr->liveRefCount = 0;
	colorPtr->numColors = 0;
	colorPtr->visualInfo = instancePtr->visualInfo;
	colorPtr->pixelMap = NULL;
	Tcl_SetHashValue(entry, colorPtr);
    }

    colorPtr->refCount++;
    colorPtr->liveRefCount++;
    instancePtr->colorTablePtr = colorPtr;
    if (colorPtr->flags & DISPOSE_PENDING) {
	Tcl_CancelIdleCall(DisposeColorTable, (ClientData) colorPtr);
	colorPtr->flags &= ~DISPOSE_PENDING;
    }

    /*
     * Allocate colors for this color table if necessary.
     */

    if ((colorPtr->numColors == 0)
	    && ((colorPtr->flags & BLACK_AND_WHITE) == 0)) {
	AllocateColors(colorPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeColorTable --
 *
 *	This procedure is called when an instance ceases using a
 *	color table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If no other instances are using this color table, a when-idle
 *	handler is registered to free up the color table and the colors
 *	allocated for it.
 *
 *----------------------------------------------------------------------
 */

static void
FreeColorTable(colorPtr, force)
    ColorTable *colorPtr;	/* Pointer to the color table which is
				 * no longer required by an instance. */
    int force;			/* Force free to happen immediately. */
{
    colorPtr->refCount--;
    if (colorPtr->refCount > 0) {
	return;
    }
    if (force) {
	if ((colorPtr->flags & DISPOSE_PENDING) != 0) {
	    Tcl_CancelIdleCall(DisposeColorTable, (ClientData) colorPtr);
	    colorPtr->flags &= ~DISPOSE_PENDING;
	}
	DisposeColorTable((ClientData) colorPtr);
    } else if ((colorPtr->flags & DISPOSE_PENDING) == 0) {
	Tcl_DoWhenIdle(DisposeColorTable, (ClientData) colorPtr);
	colorPtr->flags |= DISPOSE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AllocateColors --
 *
 *	This procedure allocates the colors required by a color table,
 *	and sets up the fields in the color table data structure which
 *	are used in dithering.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Colors are allocated from the X server.  Fields in the
 *	color table data structure are updated.
 *
 *----------------------------------------------------------------------
 */

static void
AllocateColors(colorPtr)
    ColorTable *colorPtr;	/* Pointer to the color table requiring
				 * colors to be allocated. */
{
    int i, r, g, b, rMult, mono;
    int numColors, nRed, nGreen, nBlue;
    double fr, fg, fb, igam;
    XColor *colors;
    unsigned long *pixels;

    /* 16-bit intensity value for i/n of full intensity. */
#   define CFRAC(i, n)	((i) * 65535 / (n))

    /* As for CFRAC, but apply exponent of g. */
#   define CGFRAC(i, n, g)	((int)(65535 * pow((double)(i) / (n), (g))))

    /*
     * First parse the palette specification to get the required number of
     * shades of each primary.
     */

    mono = sscanf(colorPtr->id.palette, "%d/%d/%d", &nRed, &nGreen, &nBlue)
	    <= 1;
    igam = 1.0 / colorPtr->id.gamma;

    /*
     * Each time around this loop, we reduce the number of colors we're
     * trying to allocate until we succeed in allocating all of the colors
     * we need.
     */

    for (;;) {
	/*
	 * If we are using 1 bit/pixel, we don't need to allocate
	 * any colors (we just use the foreground and background
	 * colors in the GC).
	 */

	if (mono && (nRed <= 2)) {
	    colorPtr->flags |= BLACK_AND_WHITE;
	    return;
	}

	/*
	 * Calculate the RGB coordinates of the colors we want to
	 * allocate and store them in *colors.
	 */

	if ((colorPtr->visualInfo.class == DirectColor)
	    || (colorPtr->visualInfo.class == TrueColor)) {

	    /*
	     * Direct/True Color: allocate shades of red, green, blue
	     * independently.
	     */

	    if (mono) {
		numColors = nGreen = nBlue = nRed;
	    } else {
		numColors = MAX(MAX(nRed, nGreen), nBlue);
	    }
	    colors = (XColor *) ckalloc(numColors * sizeof(XColor));

	    for (i = 0; i < numColors; ++i) {
		if (igam == 1.0) {
		    colors[i].red = CFRAC(i, nRed - 1);
		    colors[i].green = CFRAC(i, nGreen - 1);
		    colors[i].blue = CFRAC(i, nBlue - 1);
		} else {
		    colors[i].red = CGFRAC(i, nRed - 1, igam);
		    colors[i].green = CGFRAC(i, nGreen - 1, igam);
		    colors[i].blue = CGFRAC(i, nBlue - 1, igam);
		}
	    }
	} else {
	    /*
	     * PseudoColor, StaticColor, GrayScale or StaticGray visual:
	     * we have to allocate each color in the color cube separately.
	     */

	    numColors = (mono) ? nRed: (nRed * nGreen * nBlue);
	    colors = (XColor *) ckalloc(numColors * sizeof(XColor));

	    if (!mono) {
		/*
		 * Color display using a PseudoColor or StaticColor visual.
		 */

		i = 0;
		for (r = 0; r < nRed; ++r) {
		    for (g = 0; g < nGreen; ++g) {
			for (b = 0; b < nBlue; ++b) {
			    if (igam == 1.0) {
				colors[i].red = CFRAC(r, nRed - 1);
				colors[i].green = CFRAC(g, nGreen - 1);
				colors[i].blue = CFRAC(b, nBlue - 1);
			    } else {
				colors[i].red = CGFRAC(r, nRed - 1, igam);
				colors[i].green = CGFRAC(g, nGreen - 1, igam);
				colors[i].blue = CGFRAC(b, nBlue - 1, igam);
			    }
			    i++;
			}
		    }
		}
	    } else {
		/*
		 * Monochrome display - allocate the shades of grey we want.
		 */

		for (i = 0; i < numColors; ++i) {
		    if (igam == 1.0) {
			r = CFRAC(i, numColors - 1);
		    } else {
			r = CGFRAC(i, numColors - 1, igam);
		    }
		    colors[i].red = colors[i].green = colors[i].blue = r;
		}
	    }
	}

	/*
	 * Now try to allocate the colors we've calculated.
	 */

	pixels = (unsigned long *) ckalloc(numColors * sizeof(unsigned long));
	for (i = 0; i < numColors; ++i) {
	    if (!XAllocColor(colorPtr->id.display, colorPtr->id.colormap,
		    &colors[i])) {

		/*
		 * Can't get all the colors we want in the default colormap;
		 * first try freeing colors from other unused color tables.
		 */

		if (!ReclaimColors(&colorPtr->id, numColors - i)
			|| !XAllocColor(colorPtr->id.display,
			colorPtr->id.colormap, &colors[i])) {
		    /*
		     * Still can't allocate the color.
		     */
		    break;
		}
	    }
	    pixels[i] = colors[i].pixel;
	}

	/*
	 * If we didn't get all of the colors, reduce the
	 * resolution of the color cube, free the ones we got,
	 * and try again.
	 */

	if (i >= numColors) {
	    break;
	}
	XFreeColors(colorPtr->id.display, colorPtr->id.colormap, pixels, i, 0);
	ckfree((char *) colors);
	ckfree((char *) pixels);

	if (!mono) {
	    if ((nRed == 2) && (nGreen == 2) && (nBlue == 2)) {
		/*
		 * Fall back to 1-bit monochrome display.
		 */

		mono = 1;
	    } else {
		/*
		 * Reduce the number of shades of each primary to about
		 * 3/4 of the previous value.  This should reduce the
		 * total number of colors required to about half the
		 * previous value for PseudoColor displays.
		 */

		nRed = (nRed * 3 + 2) / 4;
		nGreen = (nGreen * 3 + 2) / 4;
		nBlue = (nBlue * 3 + 2) / 4;
	    }
	} else {
	    /*
	     * Reduce the number of shades of gray to about 1/2.
	     */

	    nRed = nRed / 2;
	}
    }
    
    /*
     * We have allocated all of the necessary colors:
     * fill in various fields of the ColorTable record.
     */

    if (!mono) {
	colorPtr->flags |= COLOR_WINDOW;

	/*
	 * The following is a hairy hack.  We only want to index into
	 * the pixelMap on colormap displays.  However, if the display
	 * is on Windows, then we actually want to store the index not
	 * the value since we will be passing the color table into the
	 * TkPutImage call.
	 */
	
#ifndef __WIN32__
	if ((colorPtr->visualInfo.class != DirectColor)
		&& (colorPtr->visualInfo.class != TrueColor)) {
	    colorPtr->flags |= MAP_COLORS;
	}
#endif /* __WIN32__ */
    }

    colorPtr->numColors = numColors;
    colorPtr->pixelMap = pixels;

    /*
     * Set up quantization tables for dithering.
     */
    rMult = nGreen * nBlue;
    for (i = 0; i < 256; ++i) {
	r = (i * (nRed - 1) + 127) / 255;
	if (mono) {
	    fr = (double) colors[r].red / 65535.0;
	    if (colorPtr->id.gamma != 1.0 ) {
		fr = pow(fr, colorPtr->id.gamma);
	    }
	    colorPtr->colorQuant[0][i] = (int)(fr * 255.99);
	    colorPtr->redValues[i] = colors[r].pixel;
	} else {
	    g = (i * (nGreen - 1) + 127) / 255;
	    b = (i * (nBlue - 1) + 127) / 255;
	    if ((colorPtr->visualInfo.class == DirectColor)
		    || (colorPtr->visualInfo.class == TrueColor)) {
		colorPtr->redValues[i] = colors[r].pixel
		    & colorPtr->visualInfo.red_mask;
		colorPtr->greenValues[i] = colors[g].pixel
		    & colorPtr->visualInfo.green_mask;
		colorPtr->blueValues[i] = colors[b].pixel
		    & colorPtr->visualInfo.blue_mask;
	    } else {
		r *= rMult;
		g *= nBlue;
		colorPtr->redValues[i] = r;
		colorPtr->greenValues[i] = g;
		colorPtr->blueValues[i] = b;
	    }
	    fr = (double) colors[r].red / 65535.0;
	    fg = (double) colors[g].green / 65535.0;
	    fb = (double) colors[b].blue / 65535.0;
	    if (colorPtr->id.gamma != 1.0) {
		fr = pow(fr, colorPtr->id.gamma);
		fg = pow(fg, colorPtr->id.gamma);
		fb = pow(fb, colorPtr->id.gamma);
	    }
	    colorPtr->colorQuant[0][i] = (int)(fr * 255.99);
	    colorPtr->colorQuant[1][i] = (int)(fg * 255.99);
	    colorPtr->colorQuant[2][i] = (int)(fb * 255.99);
	}
    }

    ckfree((char *) colors);
}

/*
 *----------------------------------------------------------------------
 *
 * DisposeColorTable --
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The colors in the argument color table are freed, as is the
 *	color table structure itself.  The color table is removed
 *	from the hash table which is used to locate color tables.
 *
 *----------------------------------------------------------------------
 */

static void
DisposeColorTable(clientData)
    ClientData clientData;	/* Pointer to the ColorTable whose
				 * colors are to be released. */
{
    ColorTable *colorPtr;
    Tcl_HashEntry *entry;

    colorPtr = (ColorTable *) clientData;
    if (colorPtr->pixelMap != NULL) {
	if (colorPtr->numColors > 0) {
	    XFreeColors(colorPtr->id.display, colorPtr->id.colormap,
		    colorPtr->pixelMap, colorPtr->numColors, 0);
	    Tk_FreeColormap(colorPtr->id.display, colorPtr->id.colormap);
	}
	ckfree((char *) colorPtr->pixelMap);
    }

    entry = Tcl_FindHashEntry(&imgPhotoColorHash, (char *) &colorPtr->id);
    if (entry == NULL) {
	panic("DisposeColorTable couldn't find hash entry");
    }
    Tcl_DeleteHashEntry(entry);

    ckfree((char *) colorPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ReclaimColors --
 *
 *	This procedure is called to try to free up colors in the
 *	colormap used by a color table.  It looks for other color
 *	tables with the same colormap and with a zero live reference
 *	count, and frees their colors.  It only does so if there is
 *	the possibility of freeing up at least `numColors' colors.
 *
 * Results:
 *	The return value is TRUE if any colors were freed, FALSE
 *	otherwise.
 *
 * Side effects:
 *	ColorTables which are not currently in use may lose their
 *	color allocations.
 *
 *---------------------------------------------------------------------- */

static int
ReclaimColors(id, numColors)
    ColorTableId *id;		/* Pointer to information identifying
				 * the color table which needs more colors. */
    int numColors;		/* Number of colors required. */
{
    Tcl_HashSearch srch;
    Tcl_HashEntry *entry;
    ColorTable *colorPtr;
    int nAvail;

    /*
     * First scan through the color hash table to get an
     * upper bound on how many colors we might be able to free.
     */

    nAvail = 0;
    entry = Tcl_FirstHashEntry(&imgPhotoColorHash, &srch);
    while (entry != NULL) {
	colorPtr = (ColorTable *) Tcl_GetHashValue(entry);
	if ((colorPtr->id.display == id->display)
	    && (colorPtr->id.colormap == id->colormap)
	    && (colorPtr->liveRefCount == 0 )&& (colorPtr->numColors != 0)
	    && ((colorPtr->id.palette != id->palette)
		|| (colorPtr->id.gamma != id->gamma))) {

	    /*
	     * We could take this guy's colors off him.
	     */

	    nAvail += colorPtr->numColors;
	}
	entry = Tcl_NextHashEntry(&srch);
    }

    /*
     * nAvail is an (over)estimate of the number of colors we could free.
     */

    if (nAvail < numColors) {
	return 0;
    }

    /*
     * Scan through a second time freeing colors.
     */

    entry = Tcl_FirstHashEntry(&imgPhotoColorHash, &srch);
    while ((entry != NULL) && (numColors > 0)) {
	colorPtr = (ColorTable *) Tcl_GetHashValue(entry);
	if ((colorPtr->id.display == id->display)
		&& (colorPtr->id.colormap == id->colormap)
		&& (colorPtr->liveRefCount == 0) && (colorPtr->numColors != 0)
		&& ((colorPtr->id.palette != id->palette)
		    || (colorPtr->id.gamma != id->gamma))) {

	    /*
	     * Free the colors that this ColorTable has.
	     */

	    XFreeColors(colorPtr->id.display, colorPtr->id.colormap,
		    colorPtr->pixelMap, colorPtr->numColors, 0);
	    numColors -= colorPtr->numColors;
	    colorPtr->numColors = 0;
	    ckfree((char *) colorPtr->pixelMap);
	    colorPtr->pixelMap = NULL;
	}

	entry = Tcl_NextHashEntry(&srch);
    }
    return 1;			/* we freed some colors */
}

/*
 *----------------------------------------------------------------------
 *
 * DisposeInstance --
 *
 *	This procedure is called to finally free up an instance
 *	of a photo image which is no longer required.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The instance data structure and the resources it references
 *	are freed.
 *
 *----------------------------------------------------------------------
 */

static void
DisposeInstance(clientData)
    ClientData clientData;	/* Pointer to the instance whose resources
				 * are to be released. */
{
    PhotoInstance *instancePtr = (PhotoInstance *) clientData;
    PhotoInstance *prevPtr;

    if (instancePtr->pixels != None) {
	Tk_FreePixmap(instancePtr->display, instancePtr->pixels);
    }
    if (instancePtr->gc != None) {
	Tk_FreeGC(instancePtr->display, instancePtr->gc);
    }
    if (instancePtr->imagePtr != NULL) {
	XFree((char *) instancePtr->imagePtr);
    }
    if (instancePtr->error != NULL) {
	ckfree((char *) instancePtr->error);
    }
    if (instancePtr->colorTablePtr != NULL) {
	FreeColorTable(instancePtr->colorTablePtr, 1);
    }

    if (instancePtr->masterPtr->instancePtr == instancePtr) {
	instancePtr->masterPtr->instancePtr = instancePtr->nextPtr;
    } else {
	for (prevPtr = instancePtr->masterPtr->instancePtr;
		prevPtr->nextPtr != instancePtr; prevPtr = prevPtr->nextPtr) {
	    /* Empty loop body */
	}
	prevPtr->nextPtr = instancePtr->nextPtr;
    }
    Tk_FreeColormap(instancePtr->display, instancePtr->colormap);
    ckfree((char *) instancePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * MatchFileFormat --
 *
 *	This procedure is called to find a photo image file format
 *	handler which can parse the image data in the given file.
 *	If a user-specified format string is provided, only handlers
 *	whose names match a prefix of the format string are tried.
 *
 * Results:
 *	A standard TCL return value.  If the return value is TCL_OK, a
 *	pointer to the image format record is returned in
 *	*imageFormatPtr, and the width and height of the image are
 *	returned in *widthPtr and *heightPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
MatchFileFormat(interp, chan, fileName, formatObj, imageFormatPtr,
	widthPtr, heightPtr, oldformat)
    Tcl_Interp *interp;		/* Interpreter to use for reporting errors. */
    Tcl_Channel chan;		/* The image file, open for reading. */
    char *fileName;		/* The name of the image file. */
    Tcl_Obj *formatObj;		/* User-specified format string, or NULL. */
    Tk_PhotoImageFormat **imageFormatPtr;
				/* A pointer to the photo image format
				 * record is returned here. */
    int *widthPtr, *heightPtr;	/* The dimensions of the image are
				 * returned here. */
    int *oldformat;
{
    int matched;
    int useoldformat = 0;
    Tk_PhotoImageFormat *formatPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    char *formatString = NULL;

    if (formatObj) {
	formatString = Tcl_GetString(formatObj);
    }

    /*
     * Scan through the table of file format handlers to find
     * one which can handle the image.
     */

    matched = 0;
    for (formatPtr = tsdPtr->formatList; formatPtr != NULL;
	 formatPtr = formatPtr->nextPtr) {
	if (formatObj != NULL) {
	    if (strncasecmp(formatString,
		    formatPtr->name, strlen(formatPtr->name)) != 0) {
		continue;
	    }
	    matched = 1;
	    if (formatPtr->fileMatchProc == NULL) {
		Tcl_AppendResult(interp, "-file option isn't supported for ",
			formatString, " images", (char *) NULL);
		return TCL_ERROR;
	    }
	}
	if (formatPtr->fileMatchProc != NULL) {
	    (void) Tcl_Seek(chan, Tcl_LongAsWide(0L), SEEK_SET);
	    
	    if ((*formatPtr->fileMatchProc)(chan, fileName, formatObj,
		    widthPtr, heightPtr, interp)) {
		if (*widthPtr < 1) {
		    *widthPtr = 1;
		}
		if (*heightPtr < 1) {
		    *heightPtr = 1;
		}
		break;
	    }
	}
    }
    if (formatPtr == NULL) {
	useoldformat = 1;
	for (formatPtr = tsdPtr->oldFormatList; formatPtr != NULL;
		formatPtr = formatPtr->nextPtr) {
	    if (formatString != NULL) {
		if (strncasecmp(formatString,
			formatPtr->name, strlen(formatPtr->name)) != 0) {
		    continue;
		}
		matched = 1;
		if (formatPtr->fileMatchProc == NULL) {
		    Tcl_AppendResult(interp, "-file option isn't supported",
			    " for ", formatString, " images", (char *) NULL);
		    return TCL_ERROR;
		}
	    }
	    if (formatPtr->fileMatchProc != NULL) {
		(void) Tcl_Seek(chan, Tcl_LongAsWide(0L), SEEK_SET);
		if ((*formatPtr->fileMatchProc)(chan, fileName, (Tcl_Obj *)
			formatString, widthPtr, heightPtr, interp)) {
		    if (*widthPtr < 1) {
			*widthPtr = 1;
		    }
		    if (*heightPtr < 1) {
			*heightPtr = 1;
		    }
		    break;
		}
	    }
	}
    }

    if (formatPtr == NULL) {
	if ((formatObj != NULL) && !matched) {
	    Tcl_AppendResult(interp, "image file format \"",
		    formatString,
		    "\" is not supported", (char *) NULL);
	} else {
	    Tcl_AppendResult(interp,
		    "couldn't recognize data in image file \"",
		    fileName, "\"", (char *) NULL);
	}
	return TCL_ERROR;
    }

    *imageFormatPtr = formatPtr;
    *oldformat = useoldformat;
    (void) Tcl_Seek(chan, Tcl_LongAsWide(0L), SEEK_SET);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * MatchStringFormat --
 *
 *	This procedure is called to find a photo image file format
 *	handler which can parse the image data in the given string.
 *	If a user-specified format string is provided, only handlers
 *	whose names match a prefix of the format string are tried.
 *
 * Results:
 *	A standard TCL return value.  If the return value is TCL_OK, a
 *	pointer to the image format record is returned in
 *	*imageFormatPtr, and the width and height of the image are
 *	returned in *widthPtr and *heightPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
MatchStringFormat(interp, data, formatObj, imageFormatPtr,
	widthPtr, heightPtr, oldformat)
    Tcl_Interp *interp;		/* Interpreter to use for reporting errors. */
    Tcl_Obj *data;		/* Object containing the image data. */
    Tcl_Obj *formatObj;		/* User-specified format string, or NULL. */
    Tk_PhotoImageFormat **imageFormatPtr;
				/* A pointer to the photo image format
				 * record is returned here. */
    int *widthPtr, *heightPtr;	/* The dimensions of the image are
				 * returned here. */
    int *oldformat;		/* returns 1 if the old image API is used */
{
    int matched;
    int useoldformat = 0;
    Tk_PhotoImageFormat *formatPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    char *formatString = NULL;
    
    if (formatObj) {
	formatString = Tcl_GetString(formatObj);
    }

    /*
     * Scan through the table of file format handlers to find
     * one which can handle the image.
     */

    matched = 0;
    for (formatPtr = tsdPtr->formatList; formatPtr != NULL;
	    formatPtr = formatPtr->nextPtr) {
	if (formatObj != NULL) {
	    if (strncasecmp(formatString,
		    formatPtr->name, strlen(formatPtr->name)) != 0) {
		continue;
	    }
	    matched = 1;
	    if (formatPtr->stringMatchProc == NULL) {
		Tcl_AppendResult(interp, "-data option isn't supported for ",
			formatString, " images", (char *) NULL);
		return TCL_ERROR;
	    }
	}
	if ((formatPtr->stringMatchProc != NULL)
		&& (formatPtr->stringReadProc != NULL)
		&& (*formatPtr->stringMatchProc)(data, formatObj,
		widthPtr, heightPtr, interp)) {
	    break;
	}
    }

    if (formatPtr == NULL) {
	useoldformat = 1;
	for (formatPtr = tsdPtr->oldFormatList; formatPtr != NULL;
		formatPtr = formatPtr->nextPtr) {
	    if (formatObj != NULL) {
		if (strncasecmp(formatString,
			formatPtr->name, strlen(formatPtr->name)) != 0) {
		    continue;
		}
		matched = 1;
		if (formatPtr->stringMatchProc == NULL) {
		    Tcl_AppendResult(interp, "-data option isn't supported",
			    " for ", formatString, " images", (char *) NULL);
		    return TCL_ERROR;
		}
	    }
	    if ((formatPtr->stringMatchProc != NULL)
		    && (formatPtr->stringReadProc != NULL)
		    && (*formatPtr->stringMatchProc)(
			    (Tcl_Obj *) Tcl_GetString(data),
			    (Tcl_Obj *) formatString,
			    widthPtr, heightPtr, interp)) {
		break;
	    }
	}
    }
    if (formatPtr == NULL) {
	if ((formatObj != NULL) && !matched) {
	    Tcl_AppendResult(interp, "image format \"", formatString,
		    "\" is not supported", (char *) NULL);
	} else {
	    Tcl_AppendResult(interp, "couldn't recognize image data",
		    (char *) NULL);
	}
	return TCL_ERROR;
    }

    *imageFormatPtr = formatPtr;
    *oldformat = useoldformat;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FindPhoto --
 *
 *	This procedure is called to get an opaque handle (actually a
 *	PhotoMaster *) for a given image, which can be used in
 *	subsequent calls to Tk_PhotoPutBlock, etc.  The `name'
 *	parameter is the name of the image.
 *
 * Results:
 *	The handle for the photo image, or NULL if there is no
 *	photo image with the name given.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_PhotoHandle
Tk_FindPhoto(interp, imageName)
    Tcl_Interp *interp;		/* Interpreter (application) in which image
				 * exists. */
    CONST char *imageName;	/* Name of the desired photo image. */
{
    ClientData clientData;
    Tk_ImageType *typePtr;

    clientData = Tk_GetImageMasterData(interp, imageName, &typePtr);
    if (typePtr != &tkPhotoImageType) {
	return NULL;
    }
    return (Tk_PhotoHandle) clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoPutBlock --
 *
 *	This procedure is called to put image data into a photo image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image data is stored.  The image may be expanded.
 *	The Tk image code is informed that the image has changed.
 *
 *---------------------------------------------------------------------- */

void
Tk_PhotoPutBlock(handle, blockPtr, x, y, width, height, compRule)
    Tk_PhotoHandle handle;	/* Opaque handle for the photo image
				 * to be updated. */
    register Tk_PhotoImageBlock *blockPtr;
				/* Pointer to a structure describing the
				 * pixel data to be copied into the image. */
    int x, y;			/* Coordinates of the top-left pixel to
				 * be updated in the image. */
    int width, height;		/* Dimensions of the area of the image
				 * to be updated. */
    int compRule;		/* Compositing rule to use when processing
				 * transparent pixels. */
{
    register PhotoMaster *masterPtr;
    int xEnd, yEnd;
    int greenOffset, blueOffset, alphaOffset;
    int wLeft, hLeft;
    int wCopy, hCopy;
    unsigned char *srcPtr, *srcLinePtr;
    unsigned char *destPtr, *destLinePtr;
    int pitch;
    XRectangle rect;

    masterPtr = (PhotoMaster *) handle;

    if ((masterPtr->userWidth != 0) && ((x + width) > masterPtr->userWidth)) {
	width = masterPtr->userWidth - x;
    }
    if ((masterPtr->userHeight != 0)
	    && ((y + height) > masterPtr->userHeight)) {
	height = masterPtr->userHeight - y;
    }
    if ((width <= 0) || (height <= 0)) {
	return;
    }

    xEnd = x + width;
    yEnd = y + height;
    if ((xEnd > masterPtr->width) || (yEnd > masterPtr->height)) {
	if (ImgPhotoSetSize(masterPtr, MAX(xEnd, masterPtr->width),
		MAX(yEnd, masterPtr->height)) == TCL_ERROR) {
	    panic(TK_PHOTO_ALLOC_FAILURE_MESSAGE);
	}
    }

    if ((y < masterPtr->ditherY) || ((y == masterPtr->ditherY)
	    && (x < masterPtr->ditherX))) {
	/*
	 * The dithering isn't correct past the start of this block.
	 */
	masterPtr->ditherX = x;
	masterPtr->ditherY = y;
    }

    /*
     * If this image block could have different red, green and blue
     * components, mark it as a color image.
     */

    greenOffset = blockPtr->offset[1] - blockPtr->offset[0];
    blueOffset = blockPtr->offset[2] - blockPtr->offset[0];
    alphaOffset = blockPtr->offset[3];
    if ((alphaOffset >= blockPtr->pixelSize) || (alphaOffset < 0)) {
	alphaOffset = 0;
    } else {
	alphaOffset -= blockPtr->offset[0];
    }
    if ((greenOffset != 0) || (blueOffset != 0)) {
	masterPtr->flags |= COLOR_IMAGE;
    }

    /*
     * Copy the data into our local 32-bit/pixel array.
     * If we can do it with a single memcpy, we do.
     */

    destLinePtr = masterPtr->pix32 + (y * masterPtr->width + x) * 4;
    pitch = masterPtr->width * 4;

    /*
     * This test is probably too restrictive.  We should also be able to
     * do a memcpy if pixelSize == 3 and alphaOffset == 0.  Maybe other cases
     * too.
     */
    if ((blockPtr->pixelSize == 4)
	    && (greenOffset == 1) && (blueOffset == 2) && (alphaOffset == 3)
	    && (width <= blockPtr->width) && (height <= blockPtr->height)
	    && ((height == 1) || ((x == 0) && (width == masterPtr->width)
		&& (blockPtr->pitch == pitch)))
	    && (compRule == TK_PHOTO_COMPOSITE_SET)) {
	memcpy((VOID *) destLinePtr,
		(VOID *) (blockPtr->pixelPtr + blockPtr->offset[0]),
		(size_t) (height * width * 4));
    } else {
	int alpha;
	for (hLeft = height; hLeft > 0;) {
	    srcLinePtr = blockPtr->pixelPtr + blockPtr->offset[0];
	    hCopy = MIN(hLeft, blockPtr->height);
	    hLeft -= hCopy;
	    for (; hCopy > 0; --hCopy) {
		if ((blockPtr->pixelSize == 4) && (greenOffset == 1)
		    && (blueOffset == 2) && (alphaOffset == 3)
		    && (width <= blockPtr->width)
		    && (compRule == TK_PHOTO_COMPOSITE_SET)) {
		    memcpy((VOID *) destLinePtr, (VOID *) srcLinePtr,
			   (size_t) (width * 4));
		} else {
		    destPtr = destLinePtr;
		    for (wLeft = width; wLeft > 0;) {
			wCopy = MIN(wLeft, blockPtr->width);
			wLeft -= wCopy;
			srcPtr = srcLinePtr;
			for (; wCopy > 0; --wCopy) {
			    alpha = srcPtr[alphaOffset];
			    /*
			     * In the easy case, we can just copy.
			     */
			    if (!alphaOffset || (alpha == 255)) {
				/* new solid part of the image */
				*destPtr++ = srcPtr[0];
				*destPtr++ = srcPtr[greenOffset];
				*destPtr++ = srcPtr[blueOffset];
				*destPtr++ = 255;
				srcPtr += blockPtr->pixelSize;
				continue;
			    }

			    /*
			     * Combine according to the compositing rule.
			     */
			    switch (compRule) {
			    case TK_PHOTO_COMPOSITE_SET:
				*destPtr++ = srcPtr[0];
				*destPtr++ = srcPtr[greenOffset];
				*destPtr++ = srcPtr[blueOffset];
				*destPtr++ = alpha;
				break;

			    case TK_PHOTO_COMPOSITE_OVERLAY:
				if (!destPtr[3]) {
				    /*
				     * There must be a better way to select a
				     * background colour!
				     */
				    destPtr[0] = destPtr[1] = destPtr[2] = 0xd9;
				}

				if (alpha) {
				    destPtr[0] += (srcPtr[0] - destPtr[0]) * alpha / 255;
				    destPtr[1] += (srcPtr[greenOffset] - destPtr[1]) * alpha / 255;
				    destPtr[2] += (srcPtr[blueOffset] - destPtr[2]) * alpha / 255;
				    destPtr[3] += (255 - destPtr[3]) * alpha / 255;
				}
				/*
				 * else should be empty space
				 */
				destPtr += 4;
				break;

			    default:
				panic("unknown compositing rule: %d", compRule);
			    }
			    srcPtr += blockPtr->pixelSize;
			}
		    }
		}
		srcLinePtr += blockPtr->pitch;
		destLinePtr += pitch;
	    }
	}
    }

    /*
     * Add this new block to the region which specifies which data is valid.
     */

    if (alphaOffset) {
	int x1, y1, end;

	/*
	 * This block is grossly inefficient.  For each row in the image, it
	 * finds each continguous string of nontransparent pixels, then marks
	 * those areas as valid in the validRegion mask.  This makes drawing
	 * very efficient, because of the way we use X: we just say, here's
	 * your mask, and here's your data.  We need not worry about the
	 * current background color, etc.  But this costs us a lot on the
	 * image setup.  Still, image setup only happens once, whereas the
	 * drawing happens many times, so this might be the best way to go.
	 *
	 * An alternative might be to not set up this mask, and instead, at
	 * drawing time, for each transparent pixel, set its color to the
	 * color of the background behind that pixel.  This is what I suspect
	 * most of programs do.  However, they don't have to deal with the
	 * canvas, which could have many different background colors.
	 * Determining the correct bg color for a given pixel might be
	 * expensive.
	 */

	if (compRule != TK_PHOTO_COMPOSITE_OVERLAY) {
	    /*
	     * Don't need this when using the OVERLAY compositing rule,
	     * which always strictly increases the valid region.
	     */
	    TkRegion workRgn = TkCreateRegion();

	    rect.x = x;
	    rect.y = y;
	    rect.width = width;
	    rect.height = height;
	    TkUnionRectWithRegion(&rect, workRgn, workRgn);
	    TkSubtractRegion(masterPtr->validRegion, workRgn,
		    masterPtr->validRegion);
	    TkDestroyRegion(workRgn);
	}

	destLinePtr = masterPtr->pix32 + (y * masterPtr->width + x) * 4 + 3;
	for (y1 = 0; y1 < height; y1++) {
	    x1 = 0;
	    destPtr = destLinePtr;
	    while (x1 < width) {
		/* search for first non-transparent pixel */
		while ((x1 < width) && !*destPtr) {
		    x1++;
		    destPtr += 4;
		}
		end = x1;
		/* search for first transparent pixel */
		while ((end < width) && *destPtr) {
		    end++;
		    destPtr += 4;
		}
		if (end > x1) {
		    rect.x = x + x1;
		    rect.y = y + y1;
		    rect.width = end - x1;
		    rect.height = 1;
		    TkUnionRectWithRegion(&rect, masterPtr->validRegion,
			    masterPtr->validRegion);
		}
		x1 = end;
	    }
	    destLinePtr += masterPtr->width * 4;
	}
    } else {
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;
	TkUnionRectWithRegion(&rect, masterPtr->validRegion,
		masterPtr->validRegion);
    }

    /*
     * Update each instance.
     */

    Tk_DitherPhoto((Tk_PhotoHandle)masterPtr, x, y, width, height);

    /*
     * Tell the core image code that this image has changed.
     */

    Tk_ImageChanged(masterPtr->tkMaster, x, y, width, height, masterPtr->width,
	    masterPtr->height);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoPutZoomedBlock --
 *
 *	This procedure is called to put image data into a photo image,
 *	with possible subsampling and/or zooming of the pixels.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image data is stored.  The image may be expanded.
 *	The Tk image code is informed that the image has changed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PhotoPutZoomedBlock(handle, blockPtr, x, y, width, height, zoomX, zoomY,
	subsampleX, subsampleY, compRule)
    Tk_PhotoHandle handle;	/* Opaque handle for the photo image
				 * to be updated. */
    register Tk_PhotoImageBlock *blockPtr;
				/* Pointer to a structure describing the
				 * pixel data to be copied into the image. */
    int x, y;			/* Coordinates of the top-left pixel to
				 * be updated in the image. */
    int width, height;		/* Dimensions of the area of the image
				 * to be updated. */
    int zoomX, zoomY;		/* Zoom factors for the X and Y axes. */
    int subsampleX, subsampleY;	/* Subsampling factors for the X and Y axes. */
    int compRule;		/* Compositing rule to use when processing
				 * transparent pixels. */
{
    register PhotoMaster *masterPtr;
    int xEnd, yEnd;
    int greenOffset, blueOffset, alphaOffset;
    int wLeft, hLeft;
    int wCopy, hCopy;
    int blockWid, blockHt;
    unsigned char *srcPtr, *srcLinePtr, *srcOrigPtr;
    unsigned char *destPtr, *destLinePtr;
    int pitch;
    int xRepeat, yRepeat;
    int blockXSkip, blockYSkip;
    XRectangle rect;

    if (zoomX==1 && zoomY==1 && subsampleX==1 && subsampleY==1) {
	Tk_PhotoPutBlock(handle, blockPtr, x, y, width, height, compRule);
	return;
    }

    masterPtr = (PhotoMaster *) handle;

    if (zoomX <= 0 || zoomY <= 0) {
	return;
    }
    if ((masterPtr->userWidth != 0) && ((x + width) > masterPtr->userWidth)) {
	width = masterPtr->userWidth - x;
    }
    if ((masterPtr->userHeight != 0)
	    && ((y + height) > masterPtr->userHeight)) {
	height = masterPtr->userHeight - y;
    }
    if (width <= 0 || height <= 0) {
	return;
    }

    xEnd = x + width;
    yEnd = y + height;
    if ((xEnd > masterPtr->width) || (yEnd > masterPtr->height)) {
	int sameSrc = (blockPtr->pixelPtr == masterPtr->pix32);
	if (ImgPhotoSetSize(masterPtr, MAX(xEnd, masterPtr->width),
		MAX(yEnd, masterPtr->height)) == TCL_ERROR) {
	    panic(TK_PHOTO_ALLOC_FAILURE_MESSAGE);
	}
	if (sameSrc) {
	    blockPtr->pixelPtr = masterPtr->pix32;
	}
    }

    if ((y < masterPtr->ditherY) || ((y == masterPtr->ditherY)
	   && (x < masterPtr->ditherX))) {
	/*
	 * The dithering isn't correct past the start of this block.
	 */

	masterPtr->ditherX = x;
	masterPtr->ditherY = y;
    }

    /*
     * If this image block could have different red, green and blue
     * components, mark it as a color image.
     */

    greenOffset = blockPtr->offset[1] - blockPtr->offset[0];
    blueOffset = blockPtr->offset[2] - blockPtr->offset[0];
    alphaOffset = blockPtr->offset[3];
    if ((alphaOffset >= blockPtr->pixelSize) || (alphaOffset < 0)) {
	alphaOffset = 0;
    } else {
	alphaOffset -= blockPtr->offset[0];
    }
    if ((greenOffset != 0) || (blueOffset != 0)) {
	masterPtr->flags |= COLOR_IMAGE;
    }

    /*
     * Work out what area the pixel data in the block expands to after
     * subsampling and zooming.
     */

    blockXSkip = subsampleX * blockPtr->pixelSize;
    blockYSkip = subsampleY * blockPtr->pitch;
    if (subsampleX > 0) {
	blockWid = ((blockPtr->width + subsampleX - 1) / subsampleX) * zoomX;
    } else if (subsampleX == 0) {
	blockWid = width;
    } else {
	blockWid = ((blockPtr->width - subsampleX - 1) / -subsampleX) * zoomX;
    }
    if (subsampleY > 0) {
	blockHt = ((blockPtr->height + subsampleY - 1) / subsampleY) * zoomY;
    } else if (subsampleY == 0) {
	blockHt = height;
    } else {
	blockHt = ((blockPtr->height - subsampleY - 1) / -subsampleY) * zoomY;
    }

    /*
     * Copy the data into our local 32-bit/pixel array.
     */

    destLinePtr = masterPtr->pix32 + (y * masterPtr->width + x) * 4;
    srcOrigPtr = blockPtr->pixelPtr + blockPtr->offset[0];
    if (subsampleX < 0) {
	srcOrigPtr += (blockPtr->width - 1) * blockPtr->pixelSize;
    }
    if (subsampleY < 0) {
	srcOrigPtr += (blockPtr->height - 1) * blockPtr->pitch;
    }

    pitch = masterPtr->width * 4;
    for (hLeft = height; hLeft > 0; ) {
	hCopy = MIN(hLeft, blockHt);
	hLeft -= hCopy;
	yRepeat = zoomY;
	srcLinePtr = srcOrigPtr;
	for (; hCopy > 0; --hCopy) {
	    destPtr = destLinePtr;
	    for (wLeft = width; wLeft > 0;) {
		wCopy = MIN(wLeft, blockWid);
		wLeft -= wCopy;
		srcPtr = srcLinePtr;
		for (; wCopy > 0; wCopy -= zoomX) {
		    for (xRepeat = MIN(wCopy, zoomX); xRepeat > 0; xRepeat--) {
			/*
			 * Common case (solid pixels) first
			 */
			if (!alphaOffset || (srcPtr[alphaOffset] == 255)) {
			    *destPtr++ = srcPtr[0];
			    *destPtr++ = srcPtr[greenOffset];
			    *destPtr++ = srcPtr[blueOffset];
			    *destPtr++ = 255;
			    continue;
 			}

			switch (compRule) {
			case TK_PHOTO_COMPOSITE_SET:
			    *destPtr++ = srcPtr[0];
			    *destPtr++ = srcPtr[greenOffset];
			    *destPtr++ = srcPtr[blueOffset];
			    *destPtr++ = srcPtr[alphaOffset];
			    break;
			case TK_PHOTO_COMPOSITE_OVERLAY:
			    if (!destPtr[3]) {
				/*
				 * There must be a better way to select a
				 * background colour!
				 */
				destPtr[0] = destPtr[1] = destPtr[2] = 0xd9;
			    }
			    if (srcPtr[alphaOffset]) {
				destPtr[0] += (srcPtr[0] - destPtr[0]) * srcPtr[alphaOffset] / 255;
				destPtr[1] += (srcPtr[greenOffset] - destPtr[1]) * srcPtr[alphaOffset] / 255;
				destPtr[2] += (srcPtr[blueOffset] - destPtr[2]) * srcPtr[alphaOffset] / 255;
				destPtr[3] += (255 - destPtr[3]) * srcPtr[alphaOffset] / 255;
			    }
			    destPtr += 4;
			    break;
			default:
			    panic("unknown compositing rule: %d", compRule);
			}
		    }
		    srcPtr += blockXSkip;
		}
	    }
	    destLinePtr += pitch;
	    yRepeat--;
	    if (yRepeat <= 0) {
		srcLinePtr += blockYSkip;
		yRepeat = zoomY;
	    }
	}
    }

    /*
     * Recompute the region of data for which we have valid pixels to plot.
     */

    if (alphaOffset) {
	int x1, y1, end;

	if (compRule != TK_PHOTO_COMPOSITE_OVERLAY) {
	    /*
	     * Don't need this when using the OVERLAY compositing rule, which
	     * always strictly increases the valid region.
	     */
	    TkRegion workRgn = TkCreateRegion();

	    rect.x = x;
	    rect.y = y;
	    rect.width = width;
	    rect.height = 1;
	    TkUnionRectWithRegion(&rect, workRgn, workRgn);
	    TkSubtractRegion(masterPtr->validRegion, workRgn,
		    masterPtr->validRegion);
	    TkDestroyRegion(workRgn);
	}

	destLinePtr = masterPtr->pix32 + (y * masterPtr->width + x) * 4 + 3;
	for (y1 = 0; y1 < height; y1++) {
	    x1 = 0;
	    destPtr = destLinePtr;
	    while (x1 < width) {
		/* search for first non-transparent pixel */
		while ((x1 < width) && !*destPtr) {
		    x1++;
		    destPtr += 4;
		}
		end = x1;
		/* search for first transparent pixel */
		while ((end < width) && *destPtr) {
		    end++;
		    destPtr += 4;
		}
		if (end > x1) {
		    rect.x = x + x1;
		    rect.y = y + y1;
		    rect.width = end - x1;
		    rect.height = 1;
		    TkUnionRectWithRegion(&rect, masterPtr->validRegion,
			    masterPtr->validRegion);
		}
		x1 = end;
	    }
	    destLinePtr += masterPtr->width * 4;
	}
    } else {
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;
	TkUnionRectWithRegion(&rect, masterPtr->validRegion,
		masterPtr->validRegion);
    }

    /*
     * Update each instance.
     */

    Tk_DitherPhoto((Tk_PhotoHandle)masterPtr, x, y, width, height);

    /*
     * Tell the core image code that this image has changed.
     */

    Tk_ImageChanged(masterPtr->tkMaster, x, y, width, height, masterPtr->width,
	    masterPtr->height);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DitherPhoto --
 *
 *	This procedure is called to update an area of each instance's
 *	pixmap by dithering the corresponding area of the image master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pixmap of each instance of this image gets updated.
 *	The fields in *masterPtr indicating which area of the image
 *	is correctly dithered get updated.
 *
 *----------------------------------------------------------------------
 */

void
Tk_DitherPhoto(photo, x, y, width, height)
    Tk_PhotoHandle photo;	/* Image master whose instances are
				 * to be updated. */
    int x, y;			/* Coordinates of the top-left pixel
				 * in the area to be dithered. */
    int width, height;		/* Dimensions of the area to be dithered. */
{
    PhotoMaster *masterPtr = (PhotoMaster *) photo;
    PhotoInstance *instancePtr;

    if ((width <= 0) || (height <= 0)) {
	return;
    }

    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	DitherInstance(instancePtr, x, y, width, height);
    }

    /*
     * Work out whether this block will be correctly dithered
     * and whether it will extend the correctly dithered region.
     */

    if (((y < masterPtr->ditherY)
	    || ((y == masterPtr->ditherY) && (x <= masterPtr->ditherX)))
	    && ((y + height) > (masterPtr->ditherY))) {

	/*
	 * This block starts inside (or immediately after) the correctly
	 * dithered region, so the first scan line at least will be right.
	 * Furthermore this block extends into scanline masterPtr->ditherY.
	 */

	if ((x == 0) && (width == masterPtr->width)) {
	    /*
	     * We are doing the full width, therefore the dithering
	     * will be correct to the end.
	     */

	    masterPtr->ditherX = 0;
	    masterPtr->ditherY = y + height;
	} else {
	    /*
	     * We are doing partial scanlines, therefore the
	     * correctly-dithered region will be extended by
	     * at most one scan line.
	     */

	    if (x <= masterPtr->ditherX) {
		masterPtr->ditherX = x + width;
		if (masterPtr->ditherX >= masterPtr->width) {
		    masterPtr->ditherX = 0;
		    masterPtr->ditherY++;
		}
	    }
	}
    }

}    

/*
 *----------------------------------------------------------------------
 *
 * DitherInstance --
 *
 *	This procedure is called to update an area of an instance's
 *	pixmap by dithering the corresponding area of the master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The instance's pixmap gets updated.
 *
 *----------------------------------------------------------------------
 */

static void
DitherInstance(instancePtr, xStart, yStart, width, height)
    PhotoInstance *instancePtr;	/* The instance to be updated. */
    int xStart, yStart;		/* Coordinates of the top-left pixel in the
				 * block to be dithered. */
    int width, height;		/* Dimensions of the block to be dithered. */
{
    PhotoMaster *masterPtr;
    ColorTable *colorPtr;
    XImage *imagePtr;
    int nLines, bigEndian;
    int i, c, x, y;
    int xEnd, yEnd;
    int bitsPerPixel, bytesPerLine, lineLength;
    unsigned char *srcLinePtr, *srcPtr;
    schar *errLinePtr, *errPtr;
    unsigned char *destBytePtr, *dstLinePtr;
    pixel *destLongPtr;
    pixel firstBit, word, mask;
    int col[3];
    int doDithering = 1;

    colorPtr = instancePtr->colorTablePtr;
    masterPtr = instancePtr->masterPtr;

    /*
     * Turn dithering off in certain cases where it is not
     * needed (TrueColor, DirectColor with many colors).
     */

    if ((colorPtr->visualInfo.class == DirectColor)
	    || (colorPtr->visualInfo.class == TrueColor)) {
	int nRed, nGreen, nBlue, result;

	result = sscanf(colorPtr->id.palette, "%d/%d/%d", &nRed,
		&nGreen, &nBlue);
	if ((nRed >= 256)
		&& ((result == 1) || ((nGreen >= 256) && (nBlue >= 256)))) {
	    doDithering = 0;
	}
    }

    /*
     * First work out how many lines to do at a time,
     * then how many bytes we'll need for pixel storage,
     * and allocate it.
     */

    nLines = (MAX_PIXELS + width - 1) / width;
    if (nLines < 1) {
	nLines = 1;
    }
    if (nLines > height ) {
	nLines = height;
    }

    imagePtr = instancePtr->imagePtr;
    if (imagePtr == NULL) {
	return;			/* we must be really tight on memory */
    }
    bitsPerPixel = imagePtr->bits_per_pixel;
    bytesPerLine = ((bitsPerPixel * width + 31) >> 3) & ~3;
    imagePtr->width = width;
    imagePtr->height = nLines;
    imagePtr->bytes_per_line = bytesPerLine;
    imagePtr->data = (char *) ckalloc((unsigned) (imagePtr->bytes_per_line * nLines));
    bigEndian = imagePtr->bitmap_bit_order == MSBFirst;
    firstBit = bigEndian? (1 << (imagePtr->bitmap_unit - 1)): 1;

    lineLength = masterPtr->width * 3;
    srcLinePtr = masterPtr->pix32 + (yStart * masterPtr->width + xStart) * 4;
    errLinePtr = instancePtr->error + yStart * lineLength + xStart * 3;
    xEnd = xStart + width;

    /*
     * Loop over the image, doing at most nLines lines before
     * updating the screen image.
     */

    for (; height > 0; height -= nLines) {
	if (nLines > height) {
	    nLines = height;
	}
	dstLinePtr = (unsigned char *) imagePtr->data;
	yEnd = yStart + nLines;
	for (y = yStart; y < yEnd; ++y) {
	    srcPtr = srcLinePtr;
	    errPtr = errLinePtr;
	    destBytePtr = dstLinePtr;
	    destLongPtr = (pixel *) dstLinePtr;
	    if (colorPtr->flags & COLOR_WINDOW) {
		/*
		 * Color window.  We dither the three components
		 * independently, using Floyd-Steinberg dithering,
		 * which propagates errors from the quantization of
		 * pixels to the pixels below and to the right.
		 */

		for (x = xStart; x < xEnd; ++x) {
		    if (doDithering) {
			for (i = 0; i < 3; ++i) {
			    /*
			     * Compute the error propagated into this pixel
			     * for this component.
			     * If e[x,y] is the array of quantization error
			     * values, we compute
			     *     7/16 * e[x-1,y] + 1/16 * e[x-1,y-1]
			     *   + 5/16 * e[x,y-1] + 3/16 * e[x+1,y-1]
			     * and round it to an integer.
			     *
			     * The expression ((c + 2056) >> 4) - 128
			     * computes round(c / 16), and works correctly on
			     * machines without a sign-extending right shift.
			     */
			    
			    c = (x > 0) ? errPtr[-3] * 7: 0;
			    if (y > 0) {
				if (x > 0) {
				    c += errPtr[-lineLength-3];
				}
				c += errPtr[-lineLength] * 5;
				if ((x + 1) < masterPtr->width) {
				    c += errPtr[-lineLength+3] * 3;
				}
			    }
			    
			    /*
			     * Add the propagated error to the value of this
			     * component, quantize it, and store the
			     * quantization error.
			     */
			    
			    c = ((c + 2056) >> 4) - 128 + *srcPtr++;
			    if (c < 0) {
				c = 0;
			    } else if (c > 255) {
				c = 255;
			    }
			    col[i] = colorPtr->colorQuant[i][c];
			    *errPtr++ = c - col[i];
			}
		    } else {
			/* 
			 * Output is virtually continuous in this case,
			 * so don't bother dithering.
			 */

			col[0] = *srcPtr++;
			col[1] = *srcPtr++;
			col[2] = *srcPtr++;
		    }
		    srcPtr++;

		    /*
		     * Translate the quantized component values into
		     * an X pixel value, and store it in the image.
		     */

		    i = colorPtr->redValues[col[0]]
			    + colorPtr->greenValues[col[1]]
			    + colorPtr->blueValues[col[2]];
		    if (colorPtr->flags & MAP_COLORS) {
			i = colorPtr->pixelMap[i];
		    }
		    switch (bitsPerPixel) {
			case NBBY:
			    *destBytePtr++ = i;
			    break;
#ifndef __WIN32__
/*
 * This case is not valid for Windows because the image format is different
 * from the pixel format in Win32.  Eventually we need to fix the image
 * code in Tk to use the Windows native image ordering.  This would speed
 * up the image code for all of the common sizes.
 */

			case NBBY * sizeof(pixel):
			    *destLongPtr++ = i;
			    break;
#endif
			default:
			    XPutPixel(imagePtr, x - xStart, y - yStart,
				    (unsigned) i);
		    }
		}

	    } else if (bitsPerPixel > 1) {
		/*
		 * Multibit monochrome window.  The operation here is similar
		 * to the color window case above, except that there is only
		 * one component.  If the master image is in color, use the
		 * luminance computed as
		 *	0.344 * red + 0.5 * green + 0.156 * blue.
		 */

		for (x = xStart; x < xEnd; ++x) {
		    c = (x > 0) ? errPtr[-1] * 7: 0;
		    if (y > 0) {
			if (x > 0)  {
			    c += errPtr[-lineLength-1];
			}
			c += errPtr[-lineLength] * 5;
			if (x + 1 < masterPtr->width) {
			    c += errPtr[-lineLength+1] * 3;
			}
		    }
		    c = ((c + 2056) >> 4) - 128;

		    if ((masterPtr->flags & COLOR_IMAGE) == 0) {
			c += srcPtr[0];
		    } else {
			c += (unsigned)(srcPtr[0] * 11 + srcPtr[1] * 16
					+ srcPtr[2] * 5 + 16) >> 5;
		    }
		    srcPtr += 4;

		    if (c < 0) {
			c = 0;
		    } else if (c > 255) {
			c = 255;
		    }
		    i = colorPtr->colorQuant[0][c];
		    *errPtr++ = c - i;
		    i = colorPtr->redValues[i];
		    switch (bitsPerPixel) {
			case NBBY:
			    *destBytePtr++ = i;
			    break;
#ifndef __WIN32__
/*
 * This case is not valid for Windows because the image format is different
 * from the pixel format in Win32.  Eventually we need to fix the image
 * code in Tk to use the Windows native image ordering.  This would speed
 * up the image code for all of the common sizes.
 */

			case NBBY * sizeof(pixel):
			    *destLongPtr++ = i;
			    break;
#endif
			default:
			    XPutPixel(imagePtr, x - xStart, y - yStart,
				    (unsigned) i);
		    }
		}
	    } else {
		/*
		 * 1-bit monochrome window.  This is similar to the
		 * multibit monochrome case above, except that the
		 * quantization is simpler (we only have black = 0
		 * and white = 255), and we produce an XY-Bitmap.
		 */

		word = 0;
		mask = firstBit;
		for (x = xStart; x < xEnd; ++x) {
		    /*
		     * If we have accumulated a whole word, store it
		     * in the image and start a new word.
		     */

		    if (mask == 0) {
			*destLongPtr++ = word;
			mask = firstBit;
			word = 0;
		    }

		    c = (x > 0) ? errPtr[-1] * 7: 0;
		    if (y > 0) {
			if (x > 0) {
			    c += errPtr[-lineLength-1];
			}
			c += errPtr[-lineLength] * 5;
			if (x + 1 < masterPtr->width) {
			    c += errPtr[-lineLength+1] * 3;
			}
		    }
		    c = ((c + 2056) >> 4) - 128;

		    if ((masterPtr->flags & COLOR_IMAGE) == 0) {
			c += srcPtr[0];
		    } else {
			c += (unsigned)(srcPtr[0] * 11 + srcPtr[1] * 16
					+ srcPtr[2] * 5 + 16) >> 5;
		    }
		    srcPtr += 4;

		    if (c < 0) {
			c = 0;
		    } else if (c > 255) {
			c = 255;
		    }
		    if (c >= 128) {
			word |= mask;
			*errPtr++ = c - 255;
		    } else {
			*errPtr++ = c;
		    }
		    mask = bigEndian? (mask >> 1): (mask << 1);
		}
		*destLongPtr = word;
	    }
	    srcLinePtr += masterPtr->width * 4;
	    errLinePtr += lineLength;
	    dstLinePtr += bytesPerLine;
	}

	/*
	 * Update the pixmap for this instance with the block of
	 * pixels that we have just computed.
	 */

	TkPutImage(colorPtr->pixelMap, colorPtr->numColors,
		instancePtr->display, instancePtr->pixels,
		instancePtr->gc, imagePtr, 0, 0, xStart, yStart,
		(unsigned) width, (unsigned) nLines);
	yStart = yEnd;
	
    }

    ckfree(imagePtr->data);
    imagePtr->data = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoBlank --
 *
 *	This procedure is called to clear an entire photo image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The valid region for the image is set to the null region.
 *	The generic image code is notified that the image has changed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PhotoBlank(handle)
    Tk_PhotoHandle handle;	/* Handle for the image to be blanked. */
{
    PhotoMaster *masterPtr;
    PhotoInstance *instancePtr;

    masterPtr = (PhotoMaster *) handle;
    masterPtr->ditherX = masterPtr->ditherY = 0;
    masterPtr->flags = 0;

    /*
     * The image has valid data nowhere.
     */

    if (masterPtr->validRegion != NULL) {
	TkDestroyRegion(masterPtr->validRegion);
    }
    masterPtr->validRegion = TkCreateRegion();

    /*
     * Clear out the 32-bit pixel storage array.
     * Clear out the dithering error arrays for each instance.
     */

    memset((VOID *) masterPtr->pix32, 0,
	    (size_t) (masterPtr->width * masterPtr->height * 4));
    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	if (instancePtr->error) {
	    memset((VOID *) instancePtr->error, 0,
		    (size_t) (masterPtr->width * masterPtr->height
		    * 3 * sizeof(schar)));
	}
    }

    /*
     * Tell the core image code that this image has changed.
     */

    Tk_ImageChanged(masterPtr->tkMaster, 0, 0, masterPtr->width,
	    masterPtr->height, masterPtr->width, masterPtr->height);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoExpand --
 *
 *	This procedure is called to request that a photo image be
 *	expanded if necessary to be at least `width' pixels wide and
 *	`height' pixels high.  If the user has declared a definite
 *	image size (using the -width and -height configuration
 *	options) then this call has no effect.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size of the photo image may change; if so the generic
 *	image code is informed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PhotoExpand(handle, width, height)
    Tk_PhotoHandle handle;	/* Handle for the image to be expanded. */
    int width, height;		/* Desired minimum dimensions of the image. */
{
    PhotoMaster *masterPtr;

    masterPtr = (PhotoMaster *) handle;

    if (width <= masterPtr->width) {
	width = masterPtr->width;
    }
    if (height <= masterPtr->height) {
	height = masterPtr->height;
    }
    if ((width != masterPtr->width) || (height != masterPtr->height)) {
	if (ImgPhotoSetSize(masterPtr, MAX(width, masterPtr->width),
		MAX(height, masterPtr->height)) == TCL_ERROR) {
	    panic(TK_PHOTO_ALLOC_FAILURE_MESSAGE);
	}
	Tk_ImageChanged(masterPtr->tkMaster, 0, 0, 0, 0, masterPtr->width,
		masterPtr->height);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoGetSize --
 *
 *	This procedure is called to obtain the current size of a photo
 *	image.
 *
 * Results:
 *	The image's width and height are returned in *widthp
 *	and *heightp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PhotoGetSize(handle, widthPtr, heightPtr)
    Tk_PhotoHandle handle;	/* Handle for the image whose dimensions
				 * are requested. */
    int *widthPtr, *heightPtr;	/* The dimensions of the image are returned
				 * here. */
{
    PhotoMaster *masterPtr;

    masterPtr = (PhotoMaster *) handle;
    *widthPtr = masterPtr->width;
    *heightPtr = masterPtr->height;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoSetSize --
 *
 *	This procedure is called to set size of a photo image.
 *	This call is equivalent to using the -width and -height
 *	configuration options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size of the image may change; if so the generic
 *	image code is informed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PhotoSetSize(handle, width, height)
    Tk_PhotoHandle handle;	/* Handle for the image whose size is to
				 * be set. */
    int width, height;		/* New dimensions for the image. */
{
    PhotoMaster *masterPtr;

    masterPtr = (PhotoMaster *) handle;

    masterPtr->userWidth = width;
    masterPtr->userHeight = height;
    if (ImgPhotoSetSize(masterPtr, ((width > 0) ? width: masterPtr->width),
	    ((height > 0) ? height: masterPtr->height)) == TCL_ERROR) {
	panic(TK_PHOTO_ALLOC_FAILURE_MESSAGE);
    }
    Tk_ImageChanged(masterPtr->tkMaster, 0, 0, 0, 0,
	    masterPtr->width, masterPtr->height);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetPhotoValidRegion --
 *
 *	This procedure is called to get the part of the photo where
 *	there is valid data.  Or, conversely, the part of the photo
 *	which is transparent.
 *
 * Results:
 *	A TkRegion value that indicates the current area of the photo
 *	that is valid.  This value should not be used after any
 *	modification to the photo image.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkRegion
TkPhotoGetValidRegion(handle)
    Tk_PhotoHandle handle; /* Handle for the image whose valid region
			    * is to obtained. */
{
    PhotoMaster *masterPtr;

    masterPtr = (PhotoMaster *) handle;
    return masterPtr->validRegion;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgGetPhoto --
 *
 *	This procedure is called to obtain image data from a photo
 *	image.  This procedure fills in the Tk_PhotoImageBlock structure
 *	pointed to by `blockPtr' with details of the address and
 *	layout of the image data in memory.
 *
 * Results:
 *	A pointer to the allocated data which should be freed later.
 *	NULL if there is no need to free data because
 *	blockPtr->pixelPtr points directly to the image data.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
ImgGetPhoto(masterPtr, blockPtr, optPtr)
    PhotoMaster *masterPtr;	/* Handle for the photo image from which
				 * image data is desired. */
    Tk_PhotoImageBlock *blockPtr;
				/* Information about the address and layout
				 * of the image data is returned here. */
    struct SubcommandOptions *optPtr;
{
    unsigned char *pixelPtr;
    int x, y, greenOffset, blueOffset, alphaOffset;

    Tk_PhotoGetImage((Tk_PhotoHandle) masterPtr, blockPtr);
    blockPtr->pixelPtr += optPtr->fromY * blockPtr->pitch
	    + optPtr->fromX * blockPtr->pixelSize;
    blockPtr->width = optPtr->fromX2 - optPtr->fromX;
    blockPtr->height = optPtr->fromY2 - optPtr->fromY;

    if (!(masterPtr->flags & COLOR_IMAGE) &&
	    (!(optPtr->options & OPT_BACKGROUND)
	    || ((optPtr->background->red == optPtr->background->green)
	    && (optPtr->background->red == optPtr->background->blue)))) {
	blockPtr->offset[0] = blockPtr->offset[1] =
		blockPtr->offset[2];
    }
    alphaOffset = 0;
    for (y = 0; y < blockPtr->height; y++) {
	pixelPtr = blockPtr->pixelPtr + (y * blockPtr->pitch)
		+ blockPtr->pixelSize - 1;
	for (x = 0; x < blockPtr->width; x++) {
	    if (*pixelPtr != 255) {
		alphaOffset = 3;
		break;
	    }
	    pixelPtr += blockPtr->pixelSize;
	}
	if (alphaOffset) {
	    break;
	}
    }
    if (!alphaOffset) {
	blockPtr->pixelPtr--;
	blockPtr->offset[0]++;
	blockPtr->offset[1]++;
	blockPtr->offset[2]++;
    }
    greenOffset = blockPtr->offset[1] - blockPtr->offset[0];
    blueOffset = blockPtr->offset[2] - blockPtr->offset[0];
    if (((optPtr->options & OPT_BACKGROUND) && alphaOffset) ||
	    ((optPtr->options & OPT_GRAYSCALE) && (greenOffset || blueOffset))) {
	int newPixelSize,x,y;
	unsigned char *srcPtr, *destPtr;
	char *data;

	newPixelSize =  (!(optPtr->options & OPT_BACKGROUND) && alphaOffset) ? 2 : 1;
	if ((greenOffset || blueOffset) && !(optPtr->options & OPT_GRAYSCALE)) {
	    newPixelSize += 2;
	}
	data = ckalloc((unsigned int) (newPixelSize *
		blockPtr->width * blockPtr->height));
	srcPtr = blockPtr->pixelPtr + blockPtr->offset[0];
	destPtr = (unsigned char *) data;
	if (!greenOffset && !blueOffset) {
	    for (y = blockPtr->height; y > 0; y--) {
		for (x = blockPtr->width; x > 0; x--) {
		    *destPtr = *srcPtr;
		    srcPtr += blockPtr->pixelSize;
		    destPtr += newPixelSize;
		}
		srcPtr += blockPtr->pitch - (blockPtr->width * blockPtr->pixelSize);
	    }
	} else if (optPtr->options & OPT_GRAYSCALE) {
	    for (y = blockPtr->height; y > 0; y--) {
		for (x = blockPtr->width; x > 0; x--) {
		    *destPtr = (unsigned char) ((srcPtr[0] * 11 + srcPtr[1] * 16
			    + srcPtr[2] * 5 + 16) >> 5);
		    srcPtr += blockPtr->pixelSize;
		    destPtr += newPixelSize;
		}
		srcPtr += blockPtr->pitch - (blockPtr->width * blockPtr->pixelSize);
	    }
	} else {
	    for (y = blockPtr->height; y > 0; y--) {
		for (x = blockPtr->width; x > 0; x--) {
		    destPtr[0] = srcPtr[0];
		    destPtr[1] = srcPtr[1];
		    destPtr[2] = srcPtr[2];
		    srcPtr += blockPtr->pixelSize;
		    destPtr += newPixelSize;
		}
		srcPtr += blockPtr->pitch - (blockPtr->width * blockPtr->pixelSize);
	    }
	}
	srcPtr = blockPtr->pixelPtr + alphaOffset;
	destPtr = (unsigned char *) data;
	if (!alphaOffset) {
	    /* nothing to be done */
	} else if (optPtr->options & OPT_BACKGROUND) {
	    if (newPixelSize > 2) {
	        int red = optPtr->background->red>>8;
	        int green = optPtr->background->green>>8;
	        int blue = optPtr->background->blue>>8;
		for (y = blockPtr->height; y > 0; y--) {
		    for (x = blockPtr->width; x > 0; x--) {
			destPtr[0] += (unsigned char) (((255 - *srcPtr) *
				(red-destPtr[0])) / 255);
			destPtr[1] += (unsigned char) (((255 - *srcPtr) *
				(green-destPtr[1])) / 255);
			destPtr[2] += (unsigned char) (((255 - *srcPtr) *
				(blue-destPtr[2])) / 255);
			srcPtr += blockPtr->pixelSize;
			destPtr += newPixelSize;
		    }
		    srcPtr += blockPtr->pitch - (blockPtr->width * blockPtr->pixelSize);
		}
	    } else {
	 	int gray = (unsigned char) (((optPtr->background->red>>8) * 11
			    + (optPtr->background->green>>8) * 16
			    + (optPtr->background->blue>>8) * 5 + 16) >> 5);
		for (y = blockPtr->height; y > 0; y--) {
		    for (x = blockPtr->width; x > 0; x--) {
			destPtr[0] += ((255 - *srcPtr) *
				(gray-destPtr[0])) / 255;
			srcPtr += blockPtr->pixelSize;
			destPtr += newPixelSize;
		    }
		    srcPtr += blockPtr->pitch - (blockPtr->width * blockPtr->pixelSize);
		}
	    }
	} else {
	    destPtr += newPixelSize-1;
	    for (y = blockPtr->height; y > 0; y--) {
		for (x = blockPtr->width; x > 0; x--) {
		    *destPtr = *srcPtr;
		    srcPtr += blockPtr->pixelSize;
		    destPtr += newPixelSize;
		}
		srcPtr += blockPtr->pitch - (blockPtr->width * blockPtr->pixelSize);
	    }
	}
	blockPtr->pixelPtr = (unsigned char *) data;
	blockPtr->pixelSize = newPixelSize;
	blockPtr->pitch = newPixelSize * blockPtr->width;
	blockPtr->offset[0] = 0;
	if (newPixelSize>2) {
	    blockPtr->offset[1]= 1;
	    blockPtr->offset[2]= 2;
	} else {
	    blockPtr->offset[1]= 0;
	    blockPtr->offset[2]= 0;
	}
	return data;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgStringWrite --
 *
 *	Default string write function. The data is formatted in
 *	the default format as accepted by the "<img> put" command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ImgStringWrite(interp, formatString, blockPtr)
    Tcl_Interp *interp;
    Tcl_Obj *formatString;
    Tk_PhotoImageBlock *blockPtr;
{
    int row,col;
    char *line, *linePtr;
    unsigned char *pixelPtr;
    int greenOffset, blueOffset;
    Tcl_DString data;

    greenOffset = blockPtr->offset[1] - blockPtr->offset[0];
    blueOffset = blockPtr->offset[2] - blockPtr->offset[0];

    Tcl_DStringInit(&data);
    if ((blockPtr->width > 0) && (blockPtr->height > 0)) {
	line = (char *) ckalloc((unsigned int) ((8 * blockPtr->width) + 2));
	for (row=0; row<blockPtr->height; row++) {
	    pixelPtr = blockPtr->pixelPtr + blockPtr->offset[0] +
		    row * blockPtr->pitch;
	    linePtr = line;
	    for (col=0; col<blockPtr->width; col++) {
		sprintf(linePtr, " #%02x%02x%02x", *pixelPtr,
			pixelPtr[greenOffset], pixelPtr[blueOffset]);
		pixelPtr += blockPtr->pixelSize;
		linePtr += 8;
	    }
	    Tcl_DStringAppendElement(&data, line+1);
	}
	ckfree (line);
    }
    Tcl_DStringResult(interp, &data);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoGetImage --
 *
 *	This procedure is called to obtain image data from a photo
 *	image.  This procedure fills in the Tk_PhotoImageBlock structure
 *	pointed to by `blockPtr' with details of the address and
 *	layout of the image data in memory.
 *
 * Results:
 *	TRUE (1) indicating that image data is available,
 *	for backwards compatibility with the old photo widget.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_PhotoGetImage(handle, blockPtr)
    Tk_PhotoHandle handle;	/* Handle for the photo image from which
				 * image data is desired. */
    Tk_PhotoImageBlock *blockPtr;
				/* Information about the address and layout
				 * of the image data is returned here. */
{
    PhotoMaster *masterPtr;

    masterPtr = (PhotoMaster *) handle;
    blockPtr->pixelPtr = masterPtr->pix32;
    blockPtr->width = masterPtr->width;
    blockPtr->height = masterPtr->height;
    blockPtr->pitch = masterPtr->width * 4;
    blockPtr->pixelSize = 4;
    blockPtr->offset[0] = 0;
    blockPtr->offset[1] = 1;
    blockPtr->offset[2] = 2;
    blockPtr->offset[3] = 3;
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * PhotoOptionFind --
 *
 *	Finds a specific Photo option.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	After commands are removed.
 *
 *----------------------------------------------------------------------
 */

typedef struct OptionAssocData {
    struct OptionAssocData *nextPtr;	/* pointer to next OptionAssocData */
    Tcl_ObjCmdProc *command;		/* command associated with this
					 * option */
    char name[1];			/* name of option (remaining chars) */
} OptionAssocData;

static Tcl_ObjCmdProc *
PhotoOptionFind(interp, obj)
    Tcl_Interp *interp;		/* Interpreter that is being deleted. */
    Tcl_Obj *obj;			/* Name of option to be found. */
{
    size_t length;
    char *name = Tcl_GetStringFromObj(obj, (int *) &length);
    OptionAssocData *list;
    char *prevname = NULL;
    Tcl_ObjCmdProc *proc = (Tcl_ObjCmdProc *) NULL;
    list = (OptionAssocData *) Tcl_GetAssocData(interp, "photoOption",
	    (Tcl_InterpDeleteProc **) NULL);
    while (list != (OptionAssocData *) NULL) {
	if (strncmp(name, list->name, length) == 0) {
	    if (proc != (Tcl_ObjCmdProc *) NULL) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "ambiguous option \"", name,
			"\": must be ", prevname, (char *) NULL);
		while (list->nextPtr != (OptionAssocData *) NULL) {
		    Tcl_AppendResult(interp, prevname, ", ",(char *) NULL);
		    list = list->nextPtr;
		    prevname = list->name;
		}
		Tcl_AppendResult(interp, ", or", prevname, (char *) NULL);
		return (Tcl_ObjCmdProc *) NULL;
	    }
	    proc = list->command;
	    prevname = list->name;
	}
	list = list->nextPtr;
    }
    if (proc != (Tcl_ObjCmdProc *) NULL) {
	Tcl_ResetResult(interp);
    }
    return proc;
}

/*
 *----------------------------------------------------------------------
 *
 * PhotoOptionCleanupProc --
 *
 *	This procedure is invoked whenever an interpreter is deleted
 *	to cleanup the AssocData for "photoVisitor".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Photo Visitor options are removed.
 *
 *----------------------------------------------------------------------
 */

static void
PhotoOptionCleanupProc(clientData, interp)
    ClientData clientData;	/* Points to "photoVisitor" AssocData
				 * for the interpreter. */
    Tcl_Interp *interp;		/* Interpreter that is being deleted. */
{
    OptionAssocData *list = (OptionAssocData *) clientData;
    OptionAssocData *ptr;

    while (list != NULL) {
	list = (ptr = list)->nextPtr;
	ckfree((char *) ptr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreatePhotoOption --
 *
 *	This procedure may be invoked to add a new kind of photo
 *	option to the core photo command supported by Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, the new option will be useable by the
 *	photo command.
 *
 *--------------------------------------------------------------
 */

void
Tk_CreatePhotoOption(interp, name, proc)
    Tcl_Interp *interp;			/* interpreter */
    CONST char *name;			/* option name */
    Tcl_ObjCmdProc *proc;		/* proc to execute command */
{
    OptionAssocData *typePtr2, *prevPtr, *ptr;
    OptionAssocData *list;

    list = (OptionAssocData *) Tcl_GetAssocData(interp, "photoOption",
	    (Tcl_InterpDeleteProc **) NULL);

    /*
     * If there's already a photo option with the given name, remove it.
     */

    for (typePtr2 = list, prevPtr = NULL; typePtr2 != NULL;
	    prevPtr = typePtr2, typePtr2 = typePtr2->nextPtr) {
	if (strcmp(typePtr2->name, name) == 0) {
	    if (prevPtr == NULL) {
		list = typePtr2->nextPtr;
	    } else {
		prevPtr->nextPtr = typePtr2->nextPtr;
	    }
	    ckfree((char *) typePtr2);
	    break;
	}
    }
    ptr = (OptionAssocData *) ckalloc(sizeof(OptionAssocData) + strlen(name));
    strcpy(&(ptr->name[0]), name);
    ptr->command = proc;
    ptr->nextPtr = list;
    Tcl_SetAssocData(interp, "photoOption", PhotoOptionCleanupProc,
		(ClientData) ptr);
}

/*
 *--------------------------------------------------------------
 *
 * TkPostscriptPhoto --
 *
 *	This procedure is called to output the contents of a
 *	photo image in Postscript by calling the Tk_PostscriptPhoto
 *	function.
 *
 * Results:
 *	Returns a standard Tcl return value.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
static int
ImgPhotoPostscript(clientData, interp, tkwin, psInfo,
        x, y, width, height, prepass)
     ClientData clientData;	/* Handle for the photo image */
    Tcl_Interp *interp;		/* Interpreter */
    Tk_Window tkwin;		/* (unused) */
    Tk_PostscriptInfo psInfo;	/* postscript info */
    int x, y;			/* First pixel to output */
    int width, height;		/* Width and height of area */
    int prepass;		/* (unused) */
{
    Tk_PhotoImageBlock block;

    Tk_PhotoGetImage((Tk_PhotoHandle) clientData, &block);
    block.pixelPtr += y * block.pitch + x * block.pixelSize;

    return Tk_PostscriptPhoto(interp, &block, psInfo, width, height);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PhotoPutBlock_NoComposite, Tk_PhotoPutZoomedBlock_NoComposite --
 *
 * These backward-compatability functions just exist to fill slots in
 * stubs table.  For the behaviour of *_NoComposite, refer to the
 * corresponding function without the extra suffix.
 *
 *----------------------------------------------------------------------
 */
void
Tk_PhotoPutBlock_NoComposite(handle, blockPtr, x, y, width, height)
     Tk_PhotoHandle handle;
     Tk_PhotoImageBlock *blockPtr;
     int x, y, width, height;
{
    Tk_PhotoPutBlock(handle, blockPtr, x, y, width, height,
	    TK_PHOTO_COMPOSITE_OVERLAY);
}

void
Tk_PhotoPutZoomedBlock_NoComposite(handle, blockPtr, x, y, width, height,
				   zoomX, zoomY, subsampleX, subsampleY)
     Tk_PhotoHandle handle;
     Tk_PhotoImageBlock *blockPtr;
     int x, y, width, height, zoomX, zoomY, subsampleX, subsampleY;
{
    Tk_PhotoPutZoomedBlock(handle, blockPtr, x, y, width, height,
	    zoomX, zoomY, subsampleX, subsampleY, TK_PHOTO_COMPOSITE_OVERLAY);
}
