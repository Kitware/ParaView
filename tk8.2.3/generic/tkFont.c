/* 
 * tkFont.c --
 *
 *	This file maintains a database of fonts for the Tk toolkit.
 *	It also provides several utility procedures for measuring and
 *	displaying text.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkPort.h"
#include "tkInt.h"
#include "tkFont.h"

/*
 * The following structure is used to keep track of all the fonts that
 * exist in the current application.  It must be stored in the
 * TkMainInfo for the application.
 */
 
typedef struct TkFontInfo {
    Tcl_HashTable fontCache;	/* Map a string to an existing Tk_Font.
				 * Keys are string font names, values are
				 * TkFont pointers. */
    Tcl_HashTable namedTable;	/* Map a name to a set of attributes for a
				 * font, used when constructing a Tk_Font from
				 * a named font description.  Keys are
				 * strings, values are NamedFont pointers. */
    TkMainInfo *mainPtr;	/* Application that owns this structure. */
    int updatePending;		/* Non-zero when a World Changed event has
				 * already been queued to handle a change to
				 * a named font. */
} TkFontInfo;

/*
 * The following data structure is used to keep track of the font attributes
 * for each named font that has been defined.  The named font is only deleted
 * when the last reference to it goes away.
 */

typedef struct NamedFont {
    int refCount;		/* Number of users of named font. */
    int deletePending;		/* Non-zero if font should be deleted when
				 * last reference goes away. */
    TkFontAttributes fa;	/* Desired attributes for named font. */
} NamedFont;
    
/*
 * The following two structures are used to keep track of string
 * measurement information when using the text layout facilities.
 *
 * A LayoutChunk represents a contiguous range of text that can be measured
 * and displayed by low-level text calls.  In general, chunks will be
 * delimited by newlines and tabs.  Low-level, platform-specific things
 * like kerning and non-integer character widths may occur between the
 * characters in a single chunk, but not between characters in different
 * chunks.
 *
 * A TextLayout is a collection of LayoutChunks.  It can be displayed with
 * respect to any origin.  It is the implementation of the Tk_TextLayout
 * opaque token.
 */

typedef struct LayoutChunk {
    CONST char *start;		/* Pointer to simple string to be displayed.
				 * This is a pointer into the TkTextLayout's
				 * string. */
    int numBytes;		/* The number of bytes in this chunk. */
    int numChars;		/* The number of characters in this chunk. */
    int numDisplayChars;	/* The number of characters to display when
				 * this chunk is displayed.  Can be less than
				 * numChars if extra space characters were
				 * absorbed by the end of the chunk.  This
				 * will be < 0 if this is a chunk that is
				 * holding a tab or newline. */
    int x, y;			/* The origin of the first character in this
				 * chunk with respect to the upper-left hand
				 * corner of the TextLayout. */
    int totalWidth;		/* Width in pixels of this chunk.  Used
				 * when hit testing the invisible spaces at
				 * the end of a chunk. */
    int displayWidth;		/* Width in pixels of the displayable
				 * characters in this chunk.  Can be less than
				 * width if extra space characters were
				 * absorbed by the end of the chunk. */
} LayoutChunk;

typedef struct TextLayout {
    Tk_Font tkfont;		/* The font used when laying out the text. */
    CONST char *string;		/* The string that was layed out. */
    int width;			/* The maximum width of all lines in the
				 * text layout. */
    int numChunks;		/* Number of chunks actually used in
				 * following array. */
    LayoutChunk chunks[1];	/* Array of chunks.  The actual size will
				 * be maxChunks.  THIS FIELD MUST BE THE LAST
				 * IN THE STRUCTURE. */
} TextLayout;

/*
 * The following structures are used as two-way maps between the values for
 * the fields in the TkFontAttributes structure and the strings used in
 * Tcl, when parsing both option-value format and style-list format font
 * name strings.
 */

static TkStateMap weightMap[] = {
    {TK_FW_NORMAL,	"normal"},
    {TK_FW_BOLD,	"bold"},
    {TK_FW_UNKNOWN,	NULL}
};

static TkStateMap slantMap[] = {
    {TK_FS_ROMAN,	"roman"},
    {TK_FS_ITALIC,	"italic"},
    {TK_FS_UNKNOWN,	NULL}
};

static TkStateMap underlineMap[] = {
    {1,			"underline"},
    {0,			NULL}
};

static TkStateMap overstrikeMap[] = {
    {1,			"overstrike"},
    {0,			NULL}
};

/*
 * The following structures are used when parsing XLFD's into a set of
 * TkFontAttributes.
 */

static TkStateMap xlfdWeightMap[] = {
    {TK_FW_NORMAL,	"normal"},
    {TK_FW_NORMAL,	"medium"},
    {TK_FW_NORMAL,	"book"},
    {TK_FW_NORMAL,	"light"},
    {TK_FW_BOLD,	"bold"},
    {TK_FW_BOLD,	"demi"},
    {TK_FW_BOLD,	"demibold"},
    {TK_FW_NORMAL,	NULL}		/* Assume anything else is "normal". */
}; 

static TkStateMap xlfdSlantMap[] = {
    {TK_FS_ROMAN,	"r"},
    {TK_FS_ITALIC,	"i"},
    {TK_FS_OBLIQUE,	"o"},
    {TK_FS_ROMAN,	NULL}		/* Assume anything else is "roman". */
};

static TkStateMap xlfdSetwidthMap[] = {
    {TK_SW_NORMAL,	"normal"},
    {TK_SW_CONDENSE,	"narrow"},
    {TK_SW_CONDENSE,	"semicondensed"},
    {TK_SW_CONDENSE,	"condensed"},
    {TK_SW_UNKNOWN,	NULL}
};

/*
 * The following structure and defines specify the valid builtin options 
 * when configuring a set of font attributes.
 */

static char *fontOpt[] = {
    "-family",
    "-size",
    "-weight",
    "-slant",
    "-underline",
    "-overstrike",
    NULL
};

#define FONT_FAMILY	0
#define FONT_SIZE	1
#define FONT_WEIGHT	2
#define FONT_SLANT	3
#define FONT_UNDERLINE	4
#define FONT_OVERSTRIKE	5
#define FONT_NUMFIELDS	6

/*
 * Hardcoded font aliases.  These are used to describe (mostly) identical
 * fonts whose names differ from platform to platform.  If the
 * user-supplied font name matches any of the names in one of the alias
 * lists, the other names in the alias list are also automatically tried.
 */

static char *timesAliases[] = {
    "Times",			/* Unix. */
    "Times New Roman",		/* Windows. */
    "New York",			/* Mac. */
    NULL
};

static char *helveticaAliases[] = {
    "Helvetica",		/* Unix. */
    "Arial",			/* Windows. */
    "Geneva",			/* Mac. */
    NULL
};

static char *courierAliases[] = {
    "Courier",			/* Unix and Mac. */
    "Courier New",		/* Windows. */
    NULL
};

static char *minchoAliases[] = {
    "mincho",			/* Unix. */
    "\357\274\255\357\274\263 \346\230\216\346\234\235",
				/* Windows (MS mincho). */
    "\346\234\254\346\230\216\346\234\235\342\210\222\357\274\255",
				/* Mac (honmincho-M). */
    NULL
};

static char *gothicAliases[] = {
    "gothic",			/* Unix. */
    "\357\274\255\357\274\263 \343\202\264\343\202\267\343\203\203\343\202\257",
				/* Windows (MS goshikku). */
    "\344\270\270\343\202\264\343\202\267\343\203\203\343\202\257\342\210\222\357\274\255",
				/* Mac (goshikku-M). */
    NULL    
};

static char *dingbatsAliases[] = {
    "dingbats", "zapfdingbats", "itc zapfdingbats",
				/* Unix. */
				/* Windows. */
    "zapf dingbats",		/* Mac. */
    NULL
};

static char **fontAliases[] = {
    timesAliases,
    helveticaAliases,
    courierAliases,
    minchoAliases,
    gothicAliases,
    dingbatsAliases,
    NULL
};  

/*
 * Hardcoded font classes.  If the character cannot be found in the base 
 * font, the classes are examined in order to see if some other similar 
 * font should be examined also.  
 */

static char *systemClass[] = {
    "fixed",				/* Unix. */
					/* Windows. */
    "chicago", "osaka", "sistemny",	/* Mac. */
    NULL
};

static char *serifClass[] = {
    "times", "palatino", "mincho",	/* All platforms. */
    "song ti",				/* Unix. */
    "ms serif", "simplified arabic", 	/* Windows. */
    "latinski",				/* Mac. */
    NULL
};

static char *sansClass[] = {
    "helvetica", "gothic",		/* All platforms. */
					/* Unix. */
    "ms sans serif", "traditional arabic",
					/* Windows. */
    "bastion",				/* Mac. */
    NULL
};

static char *monoClass[] = {
    "courier", "gothic",		/* All platforms. */
    "fangsong ti",			/* Unix. */
    "simplified arabic fixed",		/* Windows. */
    "monaco", "pryamoy",		/* Mac. */
    NULL
};

static char *symbolClass[] = {
    "symbol", "dingbats", "wingdings", NULL
};

static char **fontFallbacks[] = {
    systemClass,
    serifClass,
    sansClass,
    monoClass,
    symbolClass,
    NULL
};

/*
 * Global fallbacks.  If the character could not be found in the preferred
 * fallback list, this list is examined.  If the character still cannot be
 * found, all font families in the system are examined. 
 */

static char *globalFontClass[] = {
    "symbol",			/* All platforms. */
				/* Unix. */
    "lucida sans unicode",	/* Windows. */
    "chicago",			/* Mac. */
    NULL
};

#define GetFontAttributes(tkfont) \
		((CONST TkFontAttributes *) &((TkFont *) (tkfont))->fa)

#define GetFontMetrics(tkfont)    \
		((CONST TkFontMetrics *) &((TkFont *) (tkfont))->fm)


static int		ConfigAttributesObj _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, int objc, Tcl_Obj *CONST objv[],
			    TkFontAttributes *faPtr));
static int		CreateNamedFont _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, CONST char *name,
			    TkFontAttributes *faPtr));
static void		DupFontObjProc _ANSI_ARGS_((Tcl_Obj *srcObjPtr,
			    Tcl_Obj *dupObjPtr));
static int		FieldSpecified _ANSI_ARGS_((CONST char *field));
static void		FreeFontObjProc _ANSI_ARGS_((Tcl_Obj *objPtr));
static int		GetAttributeInfoObj _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST TkFontAttributes *faPtr, Tcl_Obj *objPtr));
static LayoutChunk *	NewChunk _ANSI_ARGS_((TextLayout **layoutPtrPtr,
			    int *maxPtr, CONST char *start, int numChars,
			    int curX, int newX, int y));
static int		ParseFontNameObj _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tcl_Obj *objPtr,
			    TkFontAttributes *faPtr));
static void		RecomputeWidgets _ANSI_ARGS_((TkWindow *winPtr));
static int		SetFontFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
static void		TheWorldHasChanged _ANSI_ARGS_((
			    ClientData clientData));
static void		UpdateDependentFonts _ANSI_ARGS_((TkFontInfo *fiPtr,
			    Tk_Window tkwin, Tcl_HashEntry *namedHashPtr));

/*
 * The following structure defines the implementation of the "font" Tcl
 * object, used for drawing. The internalRep.twoPtrValue.ptr1 field of
 * each font object points to the TkFont structure for the font, or
 * NULL.
 */

static Tcl_ObjType fontObjType = {
    "font",			/* name */
    FreeFontObjProc,		/* freeIntRepProc */
    DupFontObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    SetFontFromAny		/* setFromAnyProc */
};


/*
 *---------------------------------------------------------------------------
 *
 * TkFontPkgInit --
 *
 *	This procedure is called when an application is created.  It
 *	initializes all the structures that are used by the font
 *	package on a per application basis.
 *
 * Results:
 *	Stores a token in the mainPtr to hold information needed by this 
 *	package on a per application basis. 
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */
void
TkFontPkgInit(mainPtr)
    TkMainInfo *mainPtr;	/* The application being created. */
{
    TkFontInfo *fiPtr;

    fiPtr = (TkFontInfo *) ckalloc(sizeof(TkFontInfo));
    Tcl_InitHashTable(&fiPtr->fontCache, TCL_STRING_KEYS);
    Tcl_InitHashTable(&fiPtr->namedTable, TCL_STRING_KEYS);
    fiPtr->mainPtr = mainPtr;
    fiPtr->updatePending = 0;
    mainPtr->fontInfoPtr = fiPtr;

    TkpFontPkgInit(mainPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFontPkgFree --
 *
 *	This procedure is called when an application is deleted.  It
 *	deletes all the structures that were used by the font package
 *	for this application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

void
TkFontPkgFree(mainPtr)
    TkMainInfo *mainPtr;	/* The application being deleted. */
{
    TkFontInfo *fiPtr;
    Tcl_HashEntry *hPtr, *searchPtr;
    Tcl_HashSearch search;
    int fontsLeft;

    fiPtr = mainPtr->fontInfoPtr;

    fontsLeft = 0;
    for (searchPtr = Tcl_FirstHashEntry(&fiPtr->fontCache, &search);
	    searchPtr != NULL;
	    searchPtr = Tcl_NextHashEntry(&search)) {
	fontsLeft++;
	fprintf(stderr, "Font %s still in cache.\n", 
		Tcl_GetHashKey(&fiPtr->fontCache, searchPtr));
    }
    if (fontsLeft) {
	panic("TkFontPkgFree: all fonts should have been freed already");
    }
    Tcl_DeleteHashTable(&fiPtr->fontCache);

    hPtr = Tcl_FirstHashEntry(&fiPtr->namedTable, &search);
    while (hPtr != NULL) {
	ckfree((char *) Tcl_GetHashValue(hPtr));
	hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&fiPtr->namedTable);
    if (fiPtr->updatePending != 0) {
	Tcl_CancelIdleCall(TheWorldHasChanged, (ClientData) fiPtr);
    }
    ckfree((char *) fiPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FontObjCmd -- 
 *
 *	This procedure is implemented to process the "font" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_FontObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int index;
    Tk_Window tkwin;
    TkFontInfo *fiPtr;
    static char *optionStrings[] = {
	"actual",	"configure",	"create",	"delete",
	"families",	"measure",	"metrics",	"names",
	NULL
    };
    enum options {
	FONT_ACTUAL,	FONT_CONFIGURE,	FONT_CREATE,	FONT_DELETE,
	FONT_FAMILIES,	FONT_MEASURE,	FONT_METRICS,	FONT_NAMES
    };

    tkwin = (Tk_Window) clientData;
    fiPtr = ((TkWindow *) tkwin)->mainPtr->fontInfoPtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
	case FONT_ACTUAL: {
	    int skip, result;
	    Tk_Font tkfont;
	    Tcl_Obj *objPtr;
	    CONST TkFontAttributes *faPtr;

	    skip = TkGetDisplayOf(interp, objc - 3, objv + 3, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if ((objc < 3) || (objc - skip > 4)) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"font ?-displayof window? ?option?");
		return TCL_ERROR;
	    }
	    tkfont = Tk_AllocFontFromObj(interp, tkwin, objv[2]);
	    if (tkfont == NULL) {
		return TCL_ERROR;
	    }
	    objc -= skip;
	    objv += skip;
	    faPtr = GetFontAttributes(tkfont);
	    objPtr = NULL;
	    if (objc > 3) {
		objPtr = objv[3];
	    }
	    result = GetAttributeInfoObj(interp, faPtr, objPtr);
	    Tk_FreeFont(tkfont);
	    return result;
	}
	case FONT_CONFIGURE: {
	    int result;
	    char *string;
	    Tcl_Obj *objPtr;
	    NamedFont *nfPtr;
	    Tcl_HashEntry *namedHashPtr;

	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "fontname ?options?");
		return TCL_ERROR;
	    }
	    string = Tcl_GetString(objv[2]);
	    namedHashPtr = Tcl_FindHashEntry(&fiPtr->namedTable, string);
	    nfPtr = NULL;		/* lint. */
	    if (namedHashPtr != NULL) {
		nfPtr = (NamedFont *) Tcl_GetHashValue(namedHashPtr);
	    }
	    if ((namedHashPtr == NULL) || (nfPtr->deletePending != 0)) {
		Tcl_AppendResult(interp, "named font \"", string,
			"\" doesn't exist", NULL);
		return TCL_ERROR;
	    }
	    if (objc == 3) {
		objPtr = NULL;
	    } else if (objc == 4) {
		objPtr = objv[3];
	    } else {
		result = ConfigAttributesObj(interp, tkwin, objc - 3,
			objv + 3, &nfPtr->fa);
		UpdateDependentFonts(fiPtr, tkwin, namedHashPtr);
		return result;
	    }
	    return GetAttributeInfoObj(interp, &nfPtr->fa, objPtr);
	}
	case FONT_CREATE: {
	    int skip, i;
	    char *name;
	    char buf[16 + TCL_INTEGER_SPACE];
	    TkFontAttributes fa;
	    Tcl_HashEntry *namedHashPtr;

	    skip = 3;
	    if (objc < 3) {
		name = NULL;
	    } else {
		name = Tcl_GetString(objv[2]);
		if (name[0] == '-') {
		    name = NULL;
		}
	    }
	    if (name == NULL) {
		/*
		 * No font name specified.  Generate one of the form "fontX".
		 */

		for (i = 1; ; i++) {
		    sprintf(buf, "font%d", i);
		    namedHashPtr = Tcl_FindHashEntry(&fiPtr->namedTable, buf);
		    if (namedHashPtr == NULL) {
			break;
		    }
		}
		name = buf;
		skip = 2;
	    }
	    TkInitFontAttributes(&fa);
	    if (ConfigAttributesObj(interp, tkwin, objc - skip, objv + skip,
		    &fa) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (CreateNamedFont(interp, tkwin, name, &fa) != TCL_OK) {
		return TCL_ERROR;
	    }
	    Tcl_AppendResult(interp, name, NULL);
	    break;
	}
	case FONT_DELETE: {
	    int i;
	    char *string;
	    NamedFont *nfPtr;
	    Tcl_HashEntry *namedHashPtr;

	    /*
	     * Delete the named font.  If there are still widgets using this
	     * font, then it isn't deleted right away.
	     */

	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "fontname ?fontname ...?");
		return TCL_ERROR;
	    }
	    for (i = 2; i < objc; i++) {
		string = Tcl_GetString(objv[i]);
		namedHashPtr = Tcl_FindHashEntry(&fiPtr->namedTable, string);
		if (namedHashPtr == NULL) {
		    Tcl_AppendResult(interp, "named font \"", string,
			    "\" doesn't exist", (char *) NULL);
		    return TCL_ERROR;
		}
		nfPtr = (NamedFont *) Tcl_GetHashValue(namedHashPtr);
		if (nfPtr->refCount != 0) {
		    nfPtr->deletePending = 1;
		} else {
		    Tcl_DeleteHashEntry(namedHashPtr);
		    ckfree((char *) nfPtr);
		}
	    }
	    break;
	}
	case FONT_FAMILIES: {
	    int skip;

	    skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window?");
		return TCL_ERROR;
	    }
	    TkpGetFontFamilies(interp, tkwin);
	    break;
	}
	case FONT_MEASURE: {
	    char *string;
	    Tk_Font tkfont;
	    int length, skip;
	    Tcl_Obj *resultPtr;
	    
	    skip = TkGetDisplayOf(interp, objc - 3, objv + 3, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if (objc - skip != 4) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"font ?-displayof window? text");
		return TCL_ERROR;
	    }
	    tkfont = Tk_AllocFontFromObj(interp, tkwin, objv[2]);
	    if (tkfont == NULL) {
		return TCL_ERROR;
	    }
	    string = Tcl_GetStringFromObj(objv[3 + skip], &length);
	    resultPtr = Tcl_GetObjResult(interp);
	    Tcl_SetIntObj(resultPtr, Tk_TextWidth(tkfont, string, length));
	    Tk_FreeFont(tkfont);
	    break;
	}
	case FONT_METRICS: {
	    Tk_Font tkfont;
	    int skip, index, i;
	    CONST TkFontMetrics *fmPtr;
	    static char *switches[] = {
		"-ascent", "-descent", "-linespace", "-fixed", NULL
	    };

	    skip = TkGetDisplayOf(interp, objc - 3, objv + 3, &tkwin);
	    if (skip < 0) {
		return TCL_ERROR;
	    }
	    if ((objc < 3) || ((objc - skip) > 4)) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"font ?-displayof window? ?option?");
		return TCL_ERROR;
	    }
	    tkfont = Tk_AllocFontFromObj(interp, tkwin, objv[2]);
	    if (tkfont == NULL) {
		return TCL_ERROR;
	    }
	    objc -= skip;
	    objv += skip;
	    fmPtr = GetFontMetrics(tkfont);
	    if (objc == 3) {
		char buf[64 + TCL_INTEGER_SPACE * 4];

		sprintf(buf, "-ascent %d -descent %d -linespace %d -fixed %d",
			fmPtr->ascent, fmPtr->descent,
			fmPtr->ascent + fmPtr->descent,
			fmPtr->fixed);
		Tcl_AppendResult(interp, buf, NULL);
	    } else {
		if (Tcl_GetIndexFromObj(interp, objv[3], switches,
			"metric", 0, &index) != TCL_OK) {
		    Tk_FreeFont(tkfont);
		    return TCL_ERROR;
		}
		i = 0;			/* Needed only to prevent compiler
					 * warning. */
		switch (index) {
		    case 0: i = fmPtr->ascent;			break;
		    case 1: i = fmPtr->descent;			break;
		    case 2: i = fmPtr->ascent + fmPtr->descent;	break;
		    case 3: i = fmPtr->fixed;			break;
		}
		Tcl_SetIntObj(Tcl_GetObjResult(interp), i);
	    }
	    Tk_FreeFont(tkfont);
	    break;
	}
	case FONT_NAMES: {
	    char *string;
	    NamedFont *nfPtr;
	    Tcl_HashSearch search;
	    Tcl_HashEntry *namedHashPtr;
	    Tcl_Obj *strPtr, *resultPtr;
	    
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "names");
		return TCL_ERROR;
	    }
	    resultPtr = Tcl_GetObjResult(interp);
	    namedHashPtr = Tcl_FirstHashEntry(&fiPtr->namedTable, &search);
	    while (namedHashPtr != NULL) {
		nfPtr = (NamedFont *) Tcl_GetHashValue(namedHashPtr);
		if (nfPtr->deletePending == 0) {
		    string = Tcl_GetHashKey(&fiPtr->namedTable, namedHashPtr);
		    strPtr = Tcl_NewStringObj(string, -1);
		    Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
		}
		namedHashPtr = Tcl_NextHashEntry(&search);
	    }
	    break;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UpdateDependentFonts, TheWorldHasChanged, RecomputeWidgets --
 *
 *	Called when the attributes of a named font changes.  Updates all
 *	the instantiated fonts that depend on that named font and then
 *	uses the brute force approach and prepares every widget to
 *	recompute its geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Things get queued for redisplay.
 *
 *---------------------------------------------------------------------------
 */

static void
UpdateDependentFonts(fiPtr, tkwin, namedHashPtr)
    TkFontInfo *fiPtr;		/* Info about application's fonts. */
    Tk_Window tkwin;		/* A window in the application. */
    Tcl_HashEntry *namedHashPtr;/* The named font that is changing. */
{
    Tcl_HashEntry *cacheHashPtr;
    Tcl_HashSearch search;
    TkFont *fontPtr;
    NamedFont *nfPtr;

    nfPtr = (NamedFont *) Tcl_GetHashValue(namedHashPtr);
    if (nfPtr->refCount == 0) {
	/*
	 * Well nobody's using this named font, so don't have to tell
	 * any widgets to recompute themselves.
	 */

	return;
    }

    cacheHashPtr = Tcl_FirstHashEntry(&fiPtr->fontCache, &search);
    while (cacheHashPtr != NULL) {
	for (fontPtr = (TkFont *) Tcl_GetHashValue(cacheHashPtr);
		fontPtr != NULL; fontPtr = fontPtr->nextPtr) {
	    if (fontPtr->namedHashPtr == namedHashPtr) {
		TkpGetFontFromAttributes(fontPtr, tkwin, &nfPtr->fa);
		if (fiPtr->updatePending == 0) {
		    fiPtr->updatePending = 1;
		    Tcl_DoWhenIdle(TheWorldHasChanged, (ClientData) fiPtr);
		}
	    }
	}
	cacheHashPtr = Tcl_NextHashEntry(&search);
    }
}

static void
TheWorldHasChanged(clientData)
    ClientData clientData;	/* Info about application's fonts. */
{
    TkFontInfo *fiPtr;

    fiPtr = (TkFontInfo *) clientData;
    fiPtr->updatePending = 0;

    RecomputeWidgets(fiPtr->mainPtr->winPtr);
}

static void
RecomputeWidgets(winPtr)
    TkWindow *winPtr;		/* Window to which command is sent. */
{
    if ((winPtr->classProcsPtr != NULL)
	    && (winPtr->classProcsPtr->geometryProc != NULL)) {
	(*winPtr->classProcsPtr->geometryProc)(winPtr->instanceData);
    }
    for (winPtr = winPtr->childList; winPtr != NULL; winPtr = winPtr->nextPtr) {
	RecomputeWidgets(winPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateNamedFont --
 *
 *	Create the specified named font with the given attributes in the
 *	named font table associated with the interp.  
 *
 * Results:
 *	Returns TCL_OK if the font was successfully created, or TCL_ERROR
 *	if the named font already existed.  If TCL_ERROR is returned, an
 *	error message is left in the interp's result.
 *
 * Side effects:
 *	Assume there used to exist a named font by the specified name, and
 *	that the named font had been deleted, but there were still some
 *	widgets using the named font at the time it was deleted.  If a
 *	new named font is created with the same name, all those widgets
 *	that were using the old named font will be redisplayed using
 *	the new named font's attributes.
 *
 *---------------------------------------------------------------------------
 */

static int
CreateNamedFont(interp, tkwin, name, faPtr)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tk_Window tkwin;		/* A window associated with interp. */
    CONST char *name;		/* Name for the new named font. */
    TkFontAttributes *faPtr;	/* Attributes for the new named font. */
{
    TkFontInfo *fiPtr;
    Tcl_HashEntry *namedHashPtr;
    int new;
    NamedFont *nfPtr;    

    fiPtr = ((TkWindow *) tkwin)->mainPtr->fontInfoPtr;

    namedHashPtr = Tcl_CreateHashEntry(&fiPtr->namedTable, name, &new);
		    
    if (new == 0) {
	nfPtr = (NamedFont *) Tcl_GetHashValue(namedHashPtr);
	if (nfPtr->deletePending == 0) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "named font \"", name,
		    "\" already exists", (char *) NULL);
	    return TCL_ERROR;
	}

	/*
	 * Recreating a named font with the same name as a previous
	 * named font.  Some widgets were still using that named
	 * font, so they need to get redisplayed.
	 */

	nfPtr->fa = *faPtr;
	nfPtr->deletePending = 0;
	UpdateDependentFonts(fiPtr, tkwin, namedHashPtr);
	return TCL_OK;
    }

    nfPtr = (NamedFont *) ckalloc(sizeof(NamedFont));
    nfPtr->deletePending = 0;
    Tcl_SetHashValue(namedHashPtr, nfPtr);
    nfPtr->fa = *faPtr;
    nfPtr->refCount = 0;	
    nfPtr->deletePending = 0;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetFont -- 
 *
 *	Given a string description of a font, map the description to a
 *	corresponding Tk_Font that represents the font.
 *
 * Results:
 *	The return value is token for the font, or NULL if an error
 *	prevented the font from being created.  If NULL is returned, an
 *	error message will be left in the interp's result.
 *
 * Side effects:
 *	The font is added to an internal database with a reference
 *	count.  For each call to this procedure, there should eventually
 *	be a call to Tk_FreeFont() or Tk_FreeFontFromObj() so that the
 *	database is cleaned up when fonts aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */

Tk_Font
Tk_GetFont(interp, tkwin, string)
    Tcl_Interp *interp;		/* Interp for database and error return. */
    Tk_Window tkwin;		/* For display on which font will be used. */
    CONST char *string;		/* String describing font, as: named font,
				 * native format, or parseable string. */
{
    Tk_Font tkfont; 
    Tcl_Obj *strPtr;

    strPtr = Tcl_NewStringObj((char *) string, -1);
    Tcl_IncrRefCount(strPtr);
    tkfont = Tk_AllocFontFromObj(interp, tkwin, strPtr);
    Tcl_DecrRefCount(strPtr);	
    return tkfont;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocFontFromObj -- 
 *
 *	Given a string description of a font, map the description to a
 *	corresponding Tk_Font that represents the font.
 *
 * Results:
 *	The return value is token for the font, or NULL if an error
 *	prevented the font from being created.  If NULL is returned, an
 *	error message will be left in interp's result object.
 *
 * Side effects:
 * 	The font is added to an internal database with a reference
 *	count.  For each call to this procedure, there should eventually
 *	be a call to Tk_FreeFont() or Tk_FreeFontFromObj() so that the
 *	database is cleaned up when fonts aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */

Tk_Font
Tk_AllocFontFromObj(interp, tkwin, objPtr)
    Tcl_Interp *interp;		/* Interp for database and error return. */
    Tk_Window tkwin;		/* For screen on which font will be used. */
    Tcl_Obj *objPtr;		/* Object describing font, as: named font,
				 * native format, or parseable string. */
{
    TkFontInfo *fiPtr;
    Tcl_HashEntry *cacheHashPtr, *namedHashPtr;
    TkFont *fontPtr, *firstFontPtr, *oldFontPtr;
    int new, descent;
    NamedFont *nfPtr;

    fiPtr = ((TkWindow *) tkwin)->mainPtr->fontInfoPtr;
    if (objPtr->typePtr != &fontObjType) {
	SetFontFromAny(interp, objPtr);
    }

    oldFontPtr = (TkFont *) objPtr->internalRep.twoPtrValue.ptr1;

    if (oldFontPtr != NULL) {
	if (oldFontPtr->resourceRefCount == 0) {
	    /*
	     * This is a stale reference: it refers to a TkFont that's
	     * no longer in use.  Clear the reference.
	     */

	    FreeFontObjProc(objPtr);
	    oldFontPtr = NULL;
	} else if (Tk_Screen(tkwin) == oldFontPtr->screen) {
	    oldFontPtr->resourceRefCount++;
	    return (Tk_Font) oldFontPtr;
	}
    }

    /*
     * Next, search the list of fonts that have the name we want, to see
     * if one of them is for the right screen.
     */

    new = 0;
    if (oldFontPtr != NULL) {
	cacheHashPtr = oldFontPtr->cacheHashPtr;
	FreeFontObjProc(objPtr);
    } else {
	cacheHashPtr = Tcl_CreateHashEntry(&fiPtr->fontCache,
		Tcl_GetString(objPtr), &new);
    }
    firstFontPtr = (TkFont *) Tcl_GetHashValue(cacheHashPtr);
    for (fontPtr = firstFontPtr; (fontPtr != NULL);
	    fontPtr = fontPtr->nextPtr) {
	if (Tk_Screen(tkwin) == fontPtr->screen) {
	    fontPtr->resourceRefCount++;
	    fontPtr->objRefCount++;
	    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) fontPtr;
	    return (Tk_Font) fontPtr;
	}
    }

    /*
     * The desired font isn't in the table.  Make a new one.
     */

    namedHashPtr = Tcl_FindHashEntry(&fiPtr->namedTable,
	    Tcl_GetString(objPtr));
    if (namedHashPtr != NULL) {
	/*
	 * Construct a font based on a named font.
	 */

	nfPtr = (NamedFont *) Tcl_GetHashValue(namedHashPtr);
	nfPtr->refCount++;

	fontPtr = TkpGetFontFromAttributes(NULL, tkwin, &nfPtr->fa);
    } else {
	/*
	 * Native font?
	 */

	fontPtr = TkpGetNativeFont(tkwin, Tcl_GetString(objPtr));
	if (fontPtr == NULL) {
	    TkFontAttributes fa;
	    Tcl_Obj *dupObjPtr = Tcl_DuplicateObj(objPtr);

	    if (ParseFontNameObj(interp, tkwin, dupObjPtr, &fa) != TCL_OK) {
		if (new) {
		    Tcl_DeleteHashEntry(cacheHashPtr);
		}
		Tcl_DecrRefCount(dupObjPtr);
		return NULL;
	    }
	    Tcl_DecrRefCount(dupObjPtr);

	    /*
	     * String contained the attributes inline.
	     */

	    fontPtr = TkpGetFontFromAttributes(NULL, tkwin, &fa);
	}
    }

    fontPtr->resourceRefCount = 1;
    fontPtr->objRefCount = 1;
    fontPtr->cacheHashPtr = cacheHashPtr;
    fontPtr->namedHashPtr = namedHashPtr;
    fontPtr->screen = Tk_Screen(tkwin);
    fontPtr->nextPtr = firstFontPtr;
    Tcl_SetHashValue(cacheHashPtr, fontPtr);

    Tk_MeasureChars((Tk_Font) fontPtr, "0", 1, -1, 0, &fontPtr->tabWidth);
    if (fontPtr->tabWidth == 0) {
	fontPtr->tabWidth = fontPtr->fm.maxWidth;
    }
    fontPtr->tabWidth *= 8;

    /*
     * Make sure the tab width isn't zero (some fonts may not have enough
     * information to set a reasonable tab width).
     */

    if (fontPtr->tabWidth == 0) {
	fontPtr->tabWidth = 1;
    }

    /*
     * Get information used for drawing underlines in generic code on a
     * non-underlined font.
     */
    
    descent = fontPtr->fm.descent;
    fontPtr->underlinePos = descent / 2;
    fontPtr->underlineHeight = TkFontGetPixels(tkwin, fontPtr->fa.size) / 10;
    if (fontPtr->underlineHeight == 0) {
	fontPtr->underlineHeight = 1;
    }
    if (fontPtr->underlinePos + fontPtr->underlineHeight > descent) {
	/*
	 * If this set of values would cause the bottom of the underline
	 * bar to stick below the descent of the font, jack the underline
	 * up a bit higher.
	 */

	fontPtr->underlineHeight = descent - fontPtr->underlinePos;
	if (fontPtr->underlineHeight == 0) {
	    fontPtr->underlinePos--;
	    fontPtr->underlineHeight = 1;
	}
    }
    
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) fontPtr;
    return (Tk_Font) fontPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetFontFromObj --
 *
 *	Find the font that corresponds to a given object.  The font must
 *	have already been created by Tk_GetFont or Tk_AllocFontFromObj.
 *
 * Results:
 *	The return value is a token for the font that matches objPtr
 *	and is suitable for use in tkwin.
 *
 * Side effects:
 *	If the object is not already a font ref, the conversion will free
 *	any old internal representation. 
 *
 *----------------------------------------------------------------------
 */

Tk_Font
Tk_GetFontFromObj(tkwin, objPtr)
    Tk_Window tkwin;		/* The window that the font will be used in. */
    Tcl_Obj *objPtr;		/* The object from which to get the font. */
{
    TkFontInfo *fiPtr = ((TkWindow *) tkwin)->mainPtr->fontInfoPtr;
    TkFont *fontPtr;
    Tcl_HashEntry *hashPtr;
 
    if (objPtr->typePtr != &fontObjType) {
	SetFontFromAny((Tcl_Interp *) NULL, objPtr);
    }

    fontPtr = (TkFont *) objPtr->internalRep.twoPtrValue.ptr1;

    if (fontPtr != NULL) {
	if (fontPtr->resourceRefCount == 0) {
	    /*
	     * This is a stale reference: it refers to a TkFont that's
	     * no longer in use.  Clear the reference.
	     */

	    FreeFontObjProc(objPtr);
	    fontPtr = NULL;
	} else if (Tk_Screen(tkwin) == fontPtr->screen) {
	    return (Tk_Font) fontPtr;
	}
    }

    /*
     * Next, search the list of fonts that have the name we want, to see
     * if one of them is for the right screen.
     */

    if (fontPtr != NULL) {
	hashPtr = fontPtr->cacheHashPtr;
	FreeFontObjProc(objPtr);
    } else {
	hashPtr = Tcl_FindHashEntry(&fiPtr->fontCache, Tcl_GetString(objPtr));
    }
    if (hashPtr != NULL) {
	for (fontPtr = (TkFont *) Tcl_GetHashValue(hashPtr); fontPtr != NULL;
		fontPtr = fontPtr->nextPtr) {
	    if (Tk_Screen(tkwin) == fontPtr->screen) {
		fontPtr->objRefCount++;
		objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) fontPtr;
		return (Tk_Font) fontPtr;
	    }
	}
    }

    panic("Tk_GetFontFromObj called with non-existent font!");
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * SetFontFromAny --
 *
 *	Convert the internal representation of a Tcl object to the
 *	font internal form.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	The object is left with its typePtr pointing to fontObjType.
 *	The TkFont pointer is NULL.
 *
 *----------------------------------------------------------------------
 */

static int
SetFontFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    Tcl_ObjType *typePtr;

    /*
     * Free the old internalRep before setting the new one. 
     */

    Tcl_GetString(objPtr);
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }
    objPtr->typePtr = &fontObjType;
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_NameOfFont --
 *
 *	Given a font, return a textual string identifying it.
 *
 * Results:
 *	The return value is the description that was passed to
 *	Tk_GetFont() to create the font.  The storage for the returned
 *	string is only guaranteed to persist until the font is deleted.
 *	The caller should not modify this string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

char *
Tk_NameOfFont(tkfont)
    Tk_Font tkfont;		/* Font whose name is desired. */
{
    TkFont *fontPtr;

    fontPtr = (TkFont *) tkfont;
    return fontPtr->cacheHashPtr->key.string;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeFont -- 
 *
 *	Called to release a font allocated by Tk_GetFont().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with font is decremented, and
 *	only deallocated when no one is using it.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_FreeFont(tkfont)
    Tk_Font tkfont;		/* Font to be released. */
{
    TkFont *fontPtr, *prevPtr;
    NamedFont *nfPtr;

    if (tkfont == NULL) {
	return;
    }
    fontPtr = (TkFont *) tkfont;
    fontPtr->resourceRefCount--;
    if (fontPtr->resourceRefCount > 0) {
	return;
    }
    if (fontPtr->namedHashPtr != NULL) {
	/*
	 * This font derived from a named font.  Reduce the reference
	 * count on the named font and free it if no-one else is
	 * using it.
	 */

	nfPtr = (NamedFont *) Tcl_GetHashValue(fontPtr->namedHashPtr);
	nfPtr->refCount--;
	if ((nfPtr->refCount == 0) && (nfPtr->deletePending != 0)) {
	    Tcl_DeleteHashEntry(fontPtr->namedHashPtr);
	    ckfree((char *) nfPtr);
	}
    }

    prevPtr = (TkFont *) Tcl_GetHashValue(fontPtr->cacheHashPtr);
    if (prevPtr == fontPtr) {
	if (fontPtr->nextPtr == NULL) {
	    Tcl_DeleteHashEntry(fontPtr->cacheHashPtr);
	} else  {
	    Tcl_SetHashValue(fontPtr->cacheHashPtr, fontPtr->nextPtr);
	}
    } else {
	while (prevPtr->nextPtr != fontPtr) {
	    prevPtr = prevPtr->nextPtr;
	}
	prevPtr->nextPtr = fontPtr->nextPtr;
    }

    TkpDeleteFont(fontPtr);
    if (fontPtr->objRefCount == 0) {
	ckfree((char *) fontPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeFontFromObj -- 
 *
 *	Called to release a font inside a Tcl_Obj *. Decrements the refCount
 *	of the font and removes it from the hash tables if necessary.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with font is decremented, and
 *	only deallocated when no one is using it.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_FreeFontFromObj(tkwin, objPtr)
    Tk_Window tkwin;		/* The window this font lives in. Needed
				 * for the screen value. */
    Tcl_Obj *objPtr;		/* The Tcl_Obj * to be freed. */
{
    Tk_FreeFont(Tk_GetFontFromObj(tkwin, objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeFontObjProc -- 
 *
 *	This proc is called to release an object reference to a font.
 *	Called when the object's internal rep is released or when
 *	the cached fontPtr needs to be changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object reference count is decremented. When both it
 *	and the hash ref count go to zero, the font's resources
 *	are released.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeFontObjProc(objPtr)
    Tcl_Obj *objPtr;		/* The object we are releasing. */
{
    TkFont *fontPtr = (TkFont *) objPtr->internalRep.twoPtrValue.ptr1;

    if (fontPtr != NULL) {
	fontPtr->objRefCount--;
	if ((fontPtr->resourceRefCount == 0) && (fontPtr->objRefCount == 0)) {
	    ckfree((char *) fontPtr);
	    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DupFontObjProc -- 
 *
 *	When a cached font object is duplicated, this is called to
 *	update the internal reps.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The font's objRefCount is incremented and the internal rep
 *	of the copy is set to point to it.
 *
 *---------------------------------------------------------------------------
 */

static void
DupFontObjProc(srcObjPtr, dupObjPtr)
    Tcl_Obj *srcObjPtr;		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr;		/* The object we are copying to. */
{
    TkFont *fontPtr = (TkFont *) srcObjPtr->internalRep.twoPtrValue.ptr1;
    
    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.twoPtrValue.ptr1 = (VOID *) fontPtr;

    if (fontPtr != NULL) {
	fontPtr->objRefCount++;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FontId --
 *
 *	Given a font, return an opaque handle that should be selected
 *	into the XGCValues structure in order to get the constructed
 *	gc to use this font.  This procedure would go away if the
 *	XGCValues structure were replaced with a TkGCValues structure.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Font
Tk_FontId(tkfont)
    Tk_Font tkfont;	/* Font that is going to be selected into GC. */
{
    TkFont *fontPtr;

    fontPtr = (TkFont *) tkfont;
    return fontPtr->fid;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetFontMetrics --
 *
 *	Returns overall ascent and descent metrics for the given font.
 *	These values can be used to space multiple lines of text and
 *	to align the baselines of text in different fonts.
 *
 * Results:
 *	If *heightPtr is non-NULL, it is filled with the overall height
 *	of the font, which is the sum of the ascent and descent.
 *	If *ascentPtr or *descentPtr is non-NULL, they are filled with
 *	the ascent and/or descent information for the font.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Tk_GetFontMetrics(tkfont, fmPtr)
    Tk_Font tkfont;		/* Font in which metrics are calculated. */
    Tk_FontMetrics *fmPtr;	/* Pointer to structure in which font
				 * metrics for tkfont will be stored. */
{
    TkFont *fontPtr;

    fontPtr = (TkFont *) tkfont;
    fmPtr->ascent = fontPtr->fm.ascent;
    fmPtr->descent = fontPtr->fm.descent;
    fmPtr->linespace = fontPtr->fm.ascent + fontPtr->fm.descent;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_PostscriptFontName --
 *
 *	Given a Tk_Font, return the name of the corresponding Postscript
 *	font.
 *
 * Results:
 *	The return value is the pointsize of the given Tk_Font.
 *	The name of the Postscript font is appended to dsPtr.
 *
 * Side effects:
 *	If the font does not exist on the printer, the print job will
 *	fail at print time.  Given a "reasonable" Postscript printer,
 *	the following Tk_Font font families should print correctly:
 *
 *	    Avant Garde, Arial, Bookman, Courier, Courier New, Geneva,
 *	    Helvetica, Monaco, New Century Schoolbook, New York,
 *	    Palatino, Symbol, Times, Times New Roman, Zapf Chancery,
 *	    and Zapf Dingbats.
 *
 *	Any other Tk_Font font families may not print correctly
 *	because the computed Postscript font name may be incorrect.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_PostscriptFontName(tkfont, dsPtr)
    Tk_Font tkfont;		/* Font in which text will be printed. */
    Tcl_DString *dsPtr;		/* Pointer to an initialized Tcl_DString to
				 * which the name of the Postscript font that
				 * corresponds to tkfont will be appended. */
{
    TkFont *fontPtr;
    char *family, *weightString, *slantString;
    char *src, *dest;
    int upper, len;

    len = Tcl_DStringLength(dsPtr);
    fontPtr = (TkFont *) tkfont;

    /*
     * Convert the case-insensitive Tk_Font family name to the
     * case-sensitive Postscript family name.  Take out any spaces and
     * capitalize the first letter of each word.
     */

    family = fontPtr->fa.family;
    if (strncasecmp(family, "itc ", 4) == 0) {
	family = family + 4;
    }
    if ((strcasecmp(family, "Arial") == 0)
	    || (strcasecmp(family, "Geneva") == 0)) {
	family = "Helvetica";
    } else if ((strcasecmp(family, "Times New Roman") == 0)
	    || (strcasecmp(family, "New York") == 0)) {
	family = "Times";
    } else if ((strcasecmp(family, "Courier New") == 0)
	    || (strcasecmp(family, "Monaco") == 0)) {
	family = "Courier";
    } else if (strcasecmp(family, "AvantGarde") == 0) {
	family = "AvantGarde";
    } else if (strcasecmp(family, "ZapfChancery") == 0) {
	family = "ZapfChancery";
    } else if (strcasecmp(family, "ZapfDingbats") == 0) {
	family = "ZapfDingbats";
    } else {
	Tcl_UniChar ch;

	/*
	 * Inline, capitalize the first letter of each word, lowercase the
	 * rest of the letters in each word, and then take out the spaces
	 * between the words.  This may make the DString shorter, which is
	 * safe to do.
	 */

	Tcl_DStringAppend(dsPtr, family, -1);

	src = dest = Tcl_DStringValue(dsPtr) + len;
	upper = 1;
	for (; *src != '\0'; ) {
	    while (isspace(UCHAR(*src))) { /* INTL: ISO space */
		src++;
		upper = 1;
	    }
	    src += Tcl_UtfToUniChar(src, &ch);
	    if (upper) {
		ch = Tcl_UniCharToUpper(ch);
		upper = 0;
	    } else {
	        ch = Tcl_UniCharToLower(ch);
	    }
	    dest += Tcl_UniCharToUtf(ch, dest);
	}
	*dest = '\0';
	Tcl_DStringSetLength(dsPtr, dest - Tcl_DStringValue(dsPtr));
	family = Tcl_DStringValue(dsPtr) + len;
    }
    if (family != Tcl_DStringValue(dsPtr) + len) {
	Tcl_DStringAppend(dsPtr, family, -1);
	family = Tcl_DStringValue(dsPtr) + len;
    }

    if (strcasecmp(family, "NewCenturySchoolbook") == 0) {
	Tcl_DStringSetLength(dsPtr, len);
	Tcl_DStringAppend(dsPtr, "NewCenturySchlbk", -1);
	family = Tcl_DStringValue(dsPtr) + len;
    }

    /*
     * Get the string to use for the weight.
     */

    weightString = NULL;
    if (fontPtr->fa.weight == TK_FW_NORMAL) {
	if (strcmp(family, "Bookman") == 0) {
	    weightString = "Light";
	} else if (strcmp(family, "AvantGarde") == 0) {
	    weightString = "Book";
	} else if (strcmp(family, "ZapfChancery") == 0) {
	    weightString = "Medium";
	}
    } else {
	if ((strcmp(family, "Bookman") == 0)
		|| (strcmp(family, "AvantGarde") == 0)) {
	    weightString = "Demi";
	} else {
	    weightString = "Bold";
	}
    }

    /*
     * Get the string to use for the slant.
     */

    slantString = NULL;
    if (fontPtr->fa.slant == TK_FS_ROMAN) {
	;
    } else {
	if ((strcmp(family, "Helvetica") == 0)
		|| (strcmp(family, "Courier") == 0)
		|| (strcmp(family, "AvantGarde") == 0)) {
	    slantString = "Oblique";
	} else {
	    slantString = "Italic";
	}
    }

    /*
     * The string "Roman" needs to be added to some fonts that are not bold
     * and not italic.
     */

    if ((slantString == NULL) && (weightString == NULL)) {
	if ((strcmp(family, "Times") == 0) 
		|| (strcmp(family, "NewCenturySchlbk") == 0)
		|| (strcmp(family, "Palatino") == 0)) {
	    Tcl_DStringAppend(dsPtr, "-Roman", -1);
	}
    } else {
	Tcl_DStringAppend(dsPtr, "-", -1);
	if (weightString != NULL) {
	    Tcl_DStringAppend(dsPtr, weightString, -1);
	}
	if (slantString != NULL) {
	    Tcl_DStringAppend(dsPtr, slantString, -1);
	}
    }

    return fontPtr->fa.size;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_TextWidth --
 *
 *	A wrapper function for the more complicated interface of
 *	Tk_MeasureChars.  Computes how much space the given
 *	simple string needs.
 *
 * Results:
 *	The return value is the width (in pixels) of the given string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_TextWidth(tkfont, string, numBytes)
    Tk_Font tkfont;		/* Font in which text will be measured. */
    CONST char *string;		/* String whose width will be computed. */
    int numBytes;		/* Number of bytes to consider from
				 * string, or < 0 for strlen(). */
{
    int width;

    if (numBytes < 0) {
	numBytes = strlen(string);
    }
    Tk_MeasureChars(tkfont, string, numBytes, -1, 0, &width);
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_UnderlineChars --
 *
 *	This procedure draws an underline for a given range of characters
 *	in a given string.  It doesn't draw the characters (which are
 *	assumed to have been displayed previously); it just draws the
 *	underline.  This procedure would mainly be used to quickly
 *	underline a few characters without having to construct an
 *	underlined font.  To produce properly underlined text, the
 *	appropriate underlined font should be constructed and used. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets displayed in "drawable".
 *
 *----------------------------------------------------------------------
 */

void
Tk_UnderlineChars(display, drawable, gc, tkfont, string, x, y, firstByte,
	lastByte)
    Display *display;		/* Display on which to draw. */
    Drawable drawable;		/* Window or pixmap in which to draw. */
    GC gc;			/* Graphics context for actually drawing
				 * line. */
    Tk_Font tkfont;		/* Font used in GC;  must have been allocated
				 * by Tk_GetFont().  Used for character
				 * dimensions, etc. */
    CONST char *string;		/* String containing characters to be
				 * underlined or overstruck. */
    int x, y;			/* Coordinates at which first character of
				 * string is drawn. */
    int firstByte;		/* Index of first byte of first character. */
    int lastByte;		/* Index of first byte after the last
				 * character. */
{
    TkFont *fontPtr;
    int startX, endX;

    fontPtr = (TkFont *) tkfont;
    
    Tk_MeasureChars(tkfont, string, firstByte, -1, 0, &startX);
    Tk_MeasureChars(tkfont, string, lastByte, -1, 0, &endX);

    XFillRectangle(display, drawable, gc, x + startX,
	    y + fontPtr->underlinePos, (unsigned int) (endX - startX),
	    (unsigned int) fontPtr->underlineHeight);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_ComputeTextLayout --
 *
 *	Computes the amount of screen space needed to display a
 *	multi-line, justified string of text.  Records all the
 *	measurements that were done to determine to size and
 *	positioning of the individual lines of text; this information
 *	can be used by the Tk_DrawTextLayout() procedure to
 *	display the text quickly (without remeasuring it).
 *
 *	This procedure is useful for simple widgets that want to
 *	display single-font, multi-line text and want Tk to handle the
 *	details.
 *
 * Results:
 *	The return value is a Tk_TextLayout token that holds the
 *	measurement information for the given string.  The token is
 *	only valid for the given string.  If the string is freed,
 *	the token is no longer valid and must also be freed.  To free
 *	the token, call Tk_FreeTextLayout().
 *
 *	The dimensions of the screen area needed to display the text
 *	are stored in *widthPtr and *heightPtr.
 *
 * Side effects:
 *	Memory is allocated to hold the measurement information.  
 *
 *---------------------------------------------------------------------------
 */

Tk_TextLayout
Tk_ComputeTextLayout(tkfont, string, numChars, wrapLength, justify, flags,
	widthPtr, heightPtr)
    Tk_Font tkfont;		/* Font that will be used to display text. */
    CONST char *string;		/* String whose dimensions are to be
				 * computed. */
    int numChars;		/* Number of characters to consider from
				 * string, or < 0 for strlen(). */
    int wrapLength;		/* Longest permissible line length, in
				 * pixels.  <= 0 means no automatic wrapping:
				 * just let lines get as long as needed. */
    Tk_Justify justify;		/* How to justify lines. */
    int flags;			/* Flag bits OR-ed together.
				 * TK_IGNORE_TABS means that tab characters
				 * should not be expanded.  TK_IGNORE_NEWLINES
				 * means that newline characters should not
				 * cause a line break. */
    int *widthPtr;		/* Filled with width of string. */
    int *heightPtr;		/* Filled with height of string. */
{
    TkFont *fontPtr;
    CONST char *start, *end, *special;
    int n, y, bytesThisChunk, maxChunks;
    int baseline, height, curX, newX, maxWidth;
    TextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    CONST TkFontMetrics *fmPtr;
    Tcl_DString lineBuffer;
    int *lineLengths;
    int curLine, layoutHeight;

    Tcl_DStringInit(&lineBuffer);
    
    fontPtr = (TkFont *) tkfont;
    fmPtr = &fontPtr->fm;

    height = fmPtr->ascent + fmPtr->descent;

    if (numChars < 0) {
	numChars = Tcl_NumUtfChars(string, -1);
    }
    if (wrapLength == 0) {
	wrapLength = -1;
    }

    maxChunks = 1;

    layoutPtr = (TextLayout *) ckalloc(sizeof(TextLayout)
	    + (maxChunks - 1) * sizeof(LayoutChunk));
    layoutPtr->tkfont	    = tkfont;
    layoutPtr->string	    = string;
    layoutPtr->numChunks    = 0;

    baseline = fmPtr->ascent;
    maxWidth = 0;

    /*
     * Divide the string up into simple strings and measure each string.
     */

    curX = 0;

    end = Tcl_UtfAtIndex(string, numChars);
    special = string;

    flags &= TK_IGNORE_TABS | TK_IGNORE_NEWLINES;
    flags |= TK_WHOLE_WORDS | TK_AT_LEAST_ONE;	    
    for (start = string; start < end; ) {
	if (start >= special) {
	    /*
	     * Find the next special character in the string.
	     *
	     * INTL: Note that it is safe to increment by byte, because we are
	     * looking for 7-bit characters that will appear unchanged in
	     * UTF-8.  At some point we may need to support the full Unicode
	     * whitespace set.
	     */

	    for (special = start; special < end; special++) {
		if (!(flags & TK_IGNORE_NEWLINES)) {
		    if ((*special == '\n') || (*special == '\r')) {
			break;
		    }
		}
		if (!(flags & TK_IGNORE_TABS)) {
		    if (*special == '\t') {
			break;
		    }
		}
	    }
	}

	/*
	 * Special points at the next special character (or the end of the
	 * string).  Process characters between start and special.
	 */

	chunkPtr = NULL;
	if (start < special) {
	    bytesThisChunk = Tk_MeasureChars(tkfont, start, special - start,
		    wrapLength - curX, flags, &newX);
	    newX += curX;
	    flags &= ~TK_AT_LEAST_ONE;
	    if (bytesThisChunk > 0) {
		chunkPtr = NewChunk(&layoutPtr, &maxChunks, start,
			bytesThisChunk, curX, newX, baseline);
			
		start += bytesThisChunk;
		curX = newX;
	    }
	}

	if ((start == special) && (special < end)) {
	    /*
	     * Handle the special character.
	     *
	     * INTL: Special will be pointing at a 7-bit character so we
	     * can safely treat it as a single byte.
	     */

	    chunkPtr = NULL;
	    if (*special == '\t') {
		newX = curX + fontPtr->tabWidth;
		newX -= newX % fontPtr->tabWidth;
		NewChunk(&layoutPtr, &maxChunks, start, 1, curX, newX,
			baseline)->numDisplayChars = -1;
		start++;
		if ((start < end) &&
			((wrapLength <= 0) || (newX <= wrapLength))) {
		    /*
		     * More chars can still fit on this line.
		     */

		    curX = newX;
		    flags &= ~TK_AT_LEAST_ONE;
		    continue;
		}
	    } else {	
		NewChunk(&layoutPtr, &maxChunks, start, 1, curX, 1000000000,
			baseline)->numDisplayChars = -1;
		start++;
		goto wrapLine;
	    }
	}

	/*
	 * No more characters are going to go on this line, either because
	 * no more characters can fit or there are no more characters left.
	 * Consume all extra spaces at end of line.  
	 */

	while ((start < end) && isspace(UCHAR(*start))) { /* INTL: ISO space */
	    if (!(flags & TK_IGNORE_NEWLINES)) {
		if ((*start == '\n') || (*start == '\r')) {
		    break;
		}
	    }
	    if (!(flags & TK_IGNORE_TABS)) {
		if (*start == '\t') {
		    break;
		}
	    }
	    start++;
	}
	if (chunkPtr != NULL) {
	    CONST char *end;

	    /*
	     * Append all the extra spaces on this line to the end of the
	     * last text chunk.  This is a little tricky because we are
	     * switching back and forth between characters and bytes.
	     */

	    end = chunkPtr->start + chunkPtr->numBytes;
	    bytesThisChunk = start - end;
	    if (bytesThisChunk > 0) {
		bytesThisChunk = Tk_MeasureChars(tkfont, end, bytesThisChunk,
			-1, 0, &chunkPtr->totalWidth);
		chunkPtr->numBytes += bytesThisChunk;
		chunkPtr->numChars += Tcl_NumUtfChars(end, bytesThisChunk);
		chunkPtr->totalWidth += curX;
	    }
	}

        wrapLine: 
	flags |= TK_AT_LEAST_ONE;

	/*
	 * Save current line length, then move current position to start of
	 * next line.
	 */

	if (curX > maxWidth) {
	    maxWidth = curX;
	}

	/*
	 * Remember width of this line, so that all chunks on this line
	 * can be centered or right justified, if necessary.
	 */

	Tcl_DStringAppend(&lineBuffer, (char *) &curX, sizeof(curX));

	curX = 0;
	baseline += height;
    }

    /*
     * If last line ends with a newline, then we need to make a 0 width
     * chunk on the next line.  Otherwise "Hello" and "Hello\n" are the
     * same height.
     */

    if ((layoutPtr->numChunks > 0) && ((flags & TK_IGNORE_NEWLINES) == 0)) {
	if (layoutPtr->chunks[layoutPtr->numChunks - 1].start[0] == '\n') {
	    chunkPtr = NewChunk(&layoutPtr, &maxChunks, start, 0, curX,
		    1000000000, baseline);
	    chunkPtr->numDisplayChars = -1;
	    Tcl_DStringAppend(&lineBuffer, (char *) &curX, sizeof(curX));
	    baseline += height;
	}
    }	    

    layoutPtr->width = maxWidth;
    layoutHeight = baseline - fmPtr->ascent;
    if (layoutPtr->numChunks == 0) {
	layoutHeight = height;

	/*
	 * This fake chunk is used by the other procedures so that they can
	 * pretend that there is a chunk with no chars in it, which makes
	 * the coding simpler.
	 */

	layoutPtr->numChunks = 1;
	layoutPtr->chunks[0].start		= string;
	layoutPtr->chunks[0].numBytes		= 0;
	layoutPtr->chunks[0].numChars		= 0;
	layoutPtr->chunks[0].numDisplayChars	= -1;
	layoutPtr->chunks[0].x			= 0;
	layoutPtr->chunks[0].y			= fmPtr->ascent;
	layoutPtr->chunks[0].totalWidth		= 0;
	layoutPtr->chunks[0].displayWidth	= 0;
    } else {
	/*
	 * Using maximum line length, shift all the chunks so that the lines
	 * are all justified correctly.
	 */
    
	curLine = 0;
	chunkPtr = layoutPtr->chunks;
	y = chunkPtr->y;
	lineLengths = (int *) Tcl_DStringValue(&lineBuffer);
	for (n = 0; n < layoutPtr->numChunks; n++) {
	    int extra;

	    if (chunkPtr->y != y) {
		curLine++;
		y = chunkPtr->y;
	    }
	    extra = maxWidth - lineLengths[curLine];
	    if (justify == TK_JUSTIFY_CENTER) {
		chunkPtr->x += extra / 2;
	    } else if (justify == TK_JUSTIFY_RIGHT) {
		chunkPtr->x += extra;
	    }
	    chunkPtr++;
	}
    }

    if (widthPtr != NULL) {
	*widthPtr = layoutPtr->width;
    }
    if (heightPtr != NULL) {
	*heightPtr = layoutHeight;
    }
    Tcl_DStringFree(&lineBuffer);

    return (Tk_TextLayout) layoutPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeTextLayout --
 *
 *	This procedure is called to release the storage associated with
 *	a Tk_TextLayout when it is no longer needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_FreeTextLayout(textLayout)
    Tk_TextLayout textLayout;	/* The text layout to be released. */
{
    TextLayout *layoutPtr;

    layoutPtr = (TextLayout *) textLayout;
    if (layoutPtr != NULL) {
	ckfree((char *) layoutPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_DrawTextLayout --
 *
 *	Use the information in the Tk_TextLayout token to display a
 *	multi-line, justified string of text.
 *
 *	This procedure is useful for simple widgets that need to
 *	display single-font, multi-line text and want Tk to handle
 *	the details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Text drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_DrawTextLayout(display, drawable, gc, layout, x, y, firstChar, lastChar)
    Display *display;		/* Display on which to draw. */
    Drawable drawable;		/* Window or pixmap in which to draw. */
    GC gc;			/* Graphics context to use for drawing text. */
    Tk_TextLayout layout;	/* Layout information, from a previous call
				 * to Tk_ComputeTextLayout(). */
    int x, y;			/* Upper-left hand corner of rectangle in
				 * which to draw (pixels). */
    int firstChar;		/* The index of the first character to draw
				 * from the given text item.  0 specfies the
				 * beginning. */
    int lastChar;		/* The index just after the last character
				 * to draw from the given text item.  A number
				 * < 0 means to draw all characters. */
{
    TextLayout *layoutPtr;
    int i, numDisplayChars, drawX;
    CONST char *firstByte;
    CONST char *lastByte;
    LayoutChunk *chunkPtr;

    layoutPtr = (TextLayout *) layout;
    if (layoutPtr == NULL) {
	return;
    }

    if (lastChar < 0) {
	lastChar = 100000000;
    }
    chunkPtr = layoutPtr->chunks;
    for (i = 0; i < layoutPtr->numChunks; i++) {
	numDisplayChars = chunkPtr->numDisplayChars;
	if ((numDisplayChars > 0) && (firstChar < numDisplayChars)) {
	    if (firstChar <= 0) {
		drawX = 0;
		firstChar = 0;
		firstByte = chunkPtr->start;
	    } else {
		firstByte = Tcl_UtfAtIndex(chunkPtr->start, firstChar);
		Tk_MeasureChars(layoutPtr->tkfont, chunkPtr->start,
			firstByte - chunkPtr->start, -1, 0, &drawX);
	    }
	    if (lastChar < numDisplayChars) {
		numDisplayChars = lastChar;
	    }
	    lastByte = Tcl_UtfAtIndex(chunkPtr->start, numDisplayChars);
	    Tk_DrawChars(display, drawable, gc, layoutPtr->tkfont,
		    firstByte, lastByte - firstByte,
		    x + chunkPtr->x + drawX, y + chunkPtr->y);
	}
	firstChar -= chunkPtr->numChars;
	lastChar -= chunkPtr->numChars;
	if (lastChar <= 0) {
	    break;
	}
	chunkPtr++;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_UnderlineTextLayout --
 *
 *	Use the information in the Tk_TextLayout token to display an
 *	underline below an individual character.  This procedure does
 *	not draw the text, just the underline.
 *
 *	This procedure is useful for simple widgets that need to
 *	display single-font, multi-line text with an individual
 *	character underlined and want Tk to handle the details.
 *	To display larger amounts of underlined text, construct
 *	and use an underlined font.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Underline drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_UnderlineTextLayout(display, drawable, gc, layout, x, y, underline)
    Display *display;		/* Display on which to draw. */
    Drawable drawable;		/* Window or pixmap in which to draw. */
    GC gc;			/* Graphics context to use for drawing text. */
    Tk_TextLayout layout;	/* Layout information, from a previous call
				 * to Tk_ComputeTextLayout(). */
    int x, y;			/* Upper-left hand corner of rectangle in
				 * which to draw (pixels). */
    int underline;		/* Index of the single character to
				 * underline, or -1 for no underline. */
{
    TextLayout *layoutPtr;
    TkFont *fontPtr;
    int xx, yy, width, height;

    if ((Tk_CharBbox(layout, underline, &xx, &yy, &width, &height) != 0)
	    && (width != 0)) {
	layoutPtr = (TextLayout *) layout;
	fontPtr = (TkFont *) layoutPtr->tkfont;

	XFillRectangle(display, drawable, gc, x + xx, 
		y + yy + fontPtr->fm.ascent + fontPtr->underlinePos,
		(unsigned int) width, (unsigned int) fontPtr->underlineHeight);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_PointToChar --
 *
 *	Use the information in the Tk_TextLayout token to determine the
 *	character closest to the given point.  The point must be
 *	specified with respect to the upper-left hand corner of the
 *	text layout, which is considered to be located at (0, 0).
 *
 *	Any point whose y-value is less that 0 will be considered closest
 *	to the first character in the text layout; any point whose y-value
 *	is greater than the height of the text layout will be considered
 *	closest to the last character in the text layout.
 *
 *	Any point whose x-value is less than 0 will be considered closest
 *	to the first character on that line; any point whose x-value is
 *	greater than the width of the text layout will be considered
 *	closest to the last character on that line.
 *
 * Results:
 *	The return value is the index of the character that was
 *	closest to the point.  Given a text layout with no characters,
 *	the value 0 will always be returned, referring to a hypothetical
 *	zero-width placeholder character.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_PointToChar(layout, x, y)
    Tk_TextLayout layout;	/* Layout information, from a previous call
				 * to Tk_ComputeTextLayout(). */
    int x, y;			/* Coordinates of point to check, with
				 * respect to the upper-left corner of the
				 * text layout. */
{
    TextLayout *layoutPtr;
    LayoutChunk *chunkPtr, *lastPtr;
    TkFont *fontPtr;
    int i, n, dummy, baseline, pos, numChars;

    if (y < 0) {
	/*
	 * Point lies above any line in this layout.  Return the index of
	 * the first char.
	 */

	return 0;
    }

    /*
     * Find which line contains the point.
     */

    layoutPtr = (TextLayout *) layout;
    fontPtr = (TkFont *) layoutPtr->tkfont;
    lastPtr = chunkPtr = layoutPtr->chunks;
    numChars = 0;
    for (i = 0; i < layoutPtr->numChunks; i++) {
	baseline = chunkPtr->y;
	if (y < baseline + fontPtr->fm.descent) {
	    if (x < chunkPtr->x) {
		/*
		 * Point is to the left of all chunks on this line.  Return
		 * the index of the first character on this line.
		 */

		return numChars;
	    }
	    if (x >= layoutPtr->width) {
		/*
		 * If point lies off right side of the text layout, return
		 * the last char in the last chunk on this line.  Without
		 * this, it might return the index of the first char that
		 * was located outside of the text layout.
		 */

		x = INT_MAX;
	    }

	    /*
	     * Examine all chunks on this line to see which one contains
	     * the specified point.
	     */

	    lastPtr = chunkPtr;
	    while ((i < layoutPtr->numChunks) && (chunkPtr->y == baseline))  {
		if (x < chunkPtr->x + chunkPtr->totalWidth) {
		    /*
		     * Point falls on one of the characters in this chunk.
		     */

		    if (chunkPtr->numDisplayChars < 0) {
			/*
			 * This is a special chunk that encapsulates a single
			 * tab or newline char.
			 */

			return numChars;
		    }
		    n = Tk_MeasureChars((Tk_Font) fontPtr, chunkPtr->start,
			    chunkPtr->numBytes, x - chunkPtr->x,
			    0, &dummy);
		    return numChars + Tcl_NumUtfChars(chunkPtr->start, n);
		}
		numChars += chunkPtr->numChars;
		lastPtr = chunkPtr;
		chunkPtr++;
		i++;
	    }

	    /*
	     * Point is to the right of all chars in all the chunks on this
	     * line.  Return the index just past the last char in the last
	     * chunk on this line.
	     */

	    pos = numChars;
	    if (i < layoutPtr->numChunks) {
		pos--;
	    }
	    return pos;
	}
	numChars += chunkPtr->numChars;
	lastPtr = chunkPtr;
	chunkPtr++;
    }

    /*
     * Point lies below any line in this text layout.  Return the index
     * just past the last char.
     */

    return (lastPtr->start + lastPtr->numChars) - layoutPtr->string;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_CharBbox --
 *
 *	Use the information in the Tk_TextLayout token to return the
 *	bounding box for the character specified by index.  
 *
 *	The width of the bounding box is the advance width of the
 *	character, and does not include and left- or right-bearing.
 *	Any character that extends partially outside of the
 *	text layout is considered to be truncated at the edge.  Any
 *	character which is located completely outside of the text
 *	layout is considered to be zero-width and pegged against
 *	the edge.
 *
 *	The height of the bounding box is the line height for this font,
 *	extending from the top of the ascent to the bottom of the
 *	descent.  Information about the actual height of the individual
 *	letter is not available.
 *
 *	A text layout that contains no characters is considered to
 *	contain a single zero-width placeholder character.
 * 
 * Results:
 *	The return value is 0 if the index did not specify a character
 *	in the text layout, or non-zero otherwise.  In that case,
 *	*bbox is filled with the bounding box of the character.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_CharBbox(layout, index, xPtr, yPtr, widthPtr, heightPtr)
    Tk_TextLayout layout;   /* Layout information, from a previous call to
			     * Tk_ComputeTextLayout(). */
    int index;		    /* The index of the character whose bbox is
			     * desired. */
    int *xPtr, *yPtr;	    /* Filled with the upper-left hand corner, in
			     * pixels, of the bounding box for the character
			     * specified by index, if non-NULL. */
    int *widthPtr, *heightPtr;
			    /* Filled with the width and height of the
			     * bounding box for the character specified by
			     * index, if non-NULL. */
{
    TextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    int i, x, w;
    Tk_Font tkfont;
    TkFont *fontPtr;
    CONST char *end;

    if (index < 0) {
	return 0;
    }

    layoutPtr = (TextLayout *) layout;
    chunkPtr = layoutPtr->chunks;
    tkfont = layoutPtr->tkfont;
    fontPtr = (TkFont *) tkfont;

    for (i = 0; i < layoutPtr->numChunks; i++) {
	if (chunkPtr->numDisplayChars < 0) {
	    if (index == 0) {
		x = chunkPtr->x;
		w = chunkPtr->totalWidth;
		goto check;
	    }
	} else if (index < chunkPtr->numChars) {
	    end = Tcl_UtfAtIndex(chunkPtr->start, index);
	    if (xPtr != NULL) {
		Tk_MeasureChars(tkfont, chunkPtr->start,
			end -  chunkPtr->start, -1, 0, &x);
		x += chunkPtr->x;
	    }
	    if (widthPtr != NULL) {
		Tk_MeasureChars(tkfont, end, Tcl_UtfNext(end) - end,
			-1, 0, &w);
	    }
	    goto check;
	}
	index -= chunkPtr->numChars;
	chunkPtr++;
    }
    if (index == 0) {
	/*
	 * Special case to get location just past last char in layout.
	 */

	chunkPtr--;
	x = chunkPtr->x + chunkPtr->totalWidth;
	w = 0;
    } else {
	return 0;
    }

    /*
     * Ensure that the bbox lies within the text layout.  This forces all
     * chars that extend off the right edge of the text layout to have
     * truncated widths, and all chars that are completely off the right
     * edge of the text layout to peg to the edge and have 0 width.
     */
    check:
    if (yPtr != NULL) {
	*yPtr = chunkPtr->y - fontPtr->fm.ascent;
    }
    if (heightPtr != NULL) {
	*heightPtr = fontPtr->fm.ascent + fontPtr->fm.descent;
    }

    if (x > layoutPtr->width) {
	x = layoutPtr->width;
    }
    if (xPtr != NULL) {
	*xPtr = x;
    }
    if (widthPtr != NULL) {
	if (x + w > layoutPtr->width) {
	    w = layoutPtr->width - x;
	}
	*widthPtr = w;
    }

    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_DistanceToTextLayout --
 *
 *	Computes the distance in pixels from the given point to the
 *	given text layout.  Non-displaying space characters that occur
 *	at the end of individual lines in the text layout are ignored
 *	for hit detection purposes.
 *
 * Results:
 *	The return value is 0 if the point (x, y) is inside the text
 *	layout.  If the point isn't inside the text layout then the
 *	return value is the distance in pixels from the point to the
 *	text item.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_DistanceToTextLayout(layout, x, y)
    Tk_TextLayout layout;	/* Layout information, from a previous call
				 * to Tk_ComputeTextLayout(). */
    int x, y;			/* Coordinates of point to check, with
				 * respect to the upper-left corner of the
				 * text layout (in pixels). */
{
    int i, x1, x2, y1, y2, xDiff, yDiff, dist, minDist, ascent, descent;
    LayoutChunk *chunkPtr;
    TextLayout *layoutPtr;
    TkFont *fontPtr;

    layoutPtr = (TextLayout *) layout;
    fontPtr = (TkFont *) layoutPtr->tkfont;
    ascent = fontPtr->fm.ascent;
    descent = fontPtr->fm.descent;
    
    minDist = 0;
    chunkPtr = layoutPtr->chunks;
    for (i = 0; i < layoutPtr->numChunks; i++) {
	if (chunkPtr->start[0] == '\n') {
	    /*
	     * Newline characters are not counted when computing distance
	     * (but tab characters would still be considered).
	     */

	    chunkPtr++;
	    continue;
	}

	x1 = chunkPtr->x;
	y1 = chunkPtr->y - ascent;
	x2 = chunkPtr->x + chunkPtr->displayWidth;
	y2 = chunkPtr->y + descent;

	if (x < x1) {
	    xDiff = x1 - x;
	} else if (x >= x2) {
	    xDiff = x - x2 + 1;
	} else {
	    xDiff = 0;
	}

	if (y < y1) {
	    yDiff = y1 - y;
	} else if (y >= y2) {
	    yDiff = y - y2 + 1;
	} else {
	    yDiff = 0;
	}
	if ((xDiff == 0) && (yDiff == 0)) {
	    return 0;
	}
	dist = (int) hypot((double) xDiff, (double) yDiff);
	if ((dist < minDist) || (minDist == 0)) {
	    minDist = dist;
	}
	chunkPtr++;
    }
    return minDist;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_IntersectTextLayout --
 *
 *	Determines whether a text layout lies entirely inside,
 *	entirely outside, or overlaps a given rectangle.  Non-displaying
 *	space characters that occur at the end of individual lines in
 *	the text layout are ignored for intersection calculations.
 *
 * Results:
 *	The return value is -1 if the text layout is entirely outside of
 *	the rectangle, 0 if it overlaps, and 1 if it is entirely inside
 *	of the rectangle.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_IntersectTextLayout(layout, x, y, width, height)
    Tk_TextLayout layout;	/* Layout information, from a previous call
				 * to Tk_ComputeTextLayout(). */
    int x, y;			/* Upper-left hand corner, in pixels, of
				 * rectangular area to compare with text
				 * layout.  Coordinates are with respect to
				 * the upper-left hand corner of the text
				 * layout itself. */
    int width, height;		/* The width and height of the above
				 * rectangular area, in pixels. */
{
    int result, i, x1, y1, x2, y2;
    TextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    TkFont *fontPtr;
    int left, top, right, bottom;

    /*
     * Scan the chunks one at a time, seeing whether each is entirely in,
     * entirely out, or overlapping the rectangle.  If an overlap is
     * detected, return immediately; otherwise wait until all chunks have
     * been processed and see if they were all inside or all outside.
     */
    
    layoutPtr = (TextLayout *) layout;
    chunkPtr = layoutPtr->chunks;
    fontPtr = (TkFont *) layoutPtr->tkfont;

    left    = x;
    top	    = y;
    right   = x + width;
    bottom  = y + height;

    result = 0;
    for (i = 0; i < layoutPtr->numChunks; i++) {
	if (chunkPtr->start[0] == '\n') {
	    /*
	     * Newline characters are not counted when computing area
	     * intersection (but tab characters would still be considered).
	     */

	    chunkPtr++;
	    continue;
	}

	x1 = chunkPtr->x;
	y1 = chunkPtr->y - fontPtr->fm.ascent;
	x2 = chunkPtr->x + chunkPtr->displayWidth;
	y2 = chunkPtr->y + fontPtr->fm.descent;

	if ((right < x1) || (left >= x2)
		|| (bottom < y1) || (top >= y2)) {
	    if (result == 1) {
		return 0;
	    }
	    result = -1;
	} else if ((x1 < left) || (x2 >= right)
		|| (y1 < top) || (y2 >= bottom)) {
	    return 0;
	} else if (result == -1) {
	    return 0;
	} else {
	    result = 1;
	}
	chunkPtr++;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_TextLayoutToPostscript --
 *
 *	Outputs the contents of a text layout in Postscript format.
 *	The set of lines in the text layout will be rendered by the user
 *	supplied Postscript function.  The function should be of the form:
 *
 *	    justify x y string  function  --
 *
 *	Justify is -1, 0, or 1, depending on whether the following string
 *	should be left, center, or right justified, x and y is the
 *	location for the origin of the string, string is the sequence
 *	of characters to be printed, and function is the name of the
 *	caller-provided function; the function should leave nothing
 *	on the stack.
 *
 *	The meaning of the origin of the string (x and y) depends on
 *	the justification.  For left justification, x is where the
 *	left edge of the string should appear.  For center justification,
 *	x is where the center of the string should appear.  And for right
 *	justification, x is where the right edge of the string should
 *	appear.  This behavior is necessary because, for example, right
 *	justified text on the screen is justified with screen metrics.
 *	The same string needs to be justified with printer metrics on
 *	the printer to appear in the correct place with respect to other
 *	similarly justified strings.  In all circumstances, y is the
 *	location of the baseline for the string.
 *
 * Results:
 *	The interp's result is modified to hold the Postscript code that
 *	will render the text layout.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_TextLayoutToPostscript(interp, layout)
    Tcl_Interp *interp;		/* Filled with Postscript code. */
    Tk_TextLayout layout;	/* The layout to be rendered. */
{
#define MAXUSE 128
    char buf[MAXUSE+10];
    LayoutChunk *chunkPtr;
    int i, j, used, c, baseline;
    Tcl_UniChar ch;
    CONST char *p;
    TextLayout *layoutPtr;

    layoutPtr = (TextLayout *) layout;
    chunkPtr = layoutPtr->chunks;
    baseline = chunkPtr->y;
    used = 0;
    buf[used++] = '(';
    for (i = 0; i < layoutPtr->numChunks; i++) {
	if (baseline != chunkPtr->y) {
	    buf[used++] = ')';
	    buf[used++] = '\n';
	    buf[used++] = '(';
	    baseline = chunkPtr->y;
	}
	if (chunkPtr->numDisplayChars <= 0) {
	    if (chunkPtr->start[0] == '\t') {
		buf[used++] = '\\';
		buf[used++] = 't';
	    }
	} else {
	    p = chunkPtr->start;
	    for (j = 0; j < chunkPtr->numDisplayChars; j++) {
		/*
		 * INTL: For now we just treat the characters as binary
		 * data and display the lower byte.  Eventually this should
		 * be revised to handle international postscript fonts.
		 */

		p += Tcl_UtfToUniChar(p, &ch);
		c = UCHAR(ch & 0xff);
		if ((c == '(') || (c == ')') || (c == '\\') || (c < 0x20)
			|| (c >= UCHAR(0x7f))) {
		    /*
		     * Tricky point:  the "03" is necessary in the sprintf
		     * below, so that a full three digits of octal are
		     * always generated.  Without the "03", a number
		     * following this sequence could be interpreted by
		     * Postscript as part of this sequence.
		     */

		    sprintf(buf + used, "\\%03o", c);
		    used += 4;
		} else {
		    buf[used++] = c;
		}
		if (used >= MAXUSE) {
		    buf[used] = '\0';
		    Tcl_AppendResult(interp, buf, (char *) NULL);
		    used = 0;
		}
	    }
	}
	if (used >= MAXUSE) {
	    /*
	     * If there are a whole bunch of returns or tabs in a row,
	     * then buf[] could get filled up.
	     */
	     
	    buf[used] = '\0';
	    Tcl_AppendResult(interp, buf, (char *) NULL);
	    used = 0;
	}
	chunkPtr++;
    }
    buf[used++] = ')';
    buf[used++] = '\n';
    buf[used] = '\0';
    Tcl_AppendResult(interp, buf, (char *) NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigAttributesObj --
 *
 *	Process command line options to fill in fields of a properly
 *	initialized font attributes structure.
 *
 * Results:
 *	A standard Tcl return value.  If TCL_ERROR is returned, an
 *	error message will be left in interp's result object.
 *
 * Side effects:
 *	The fields of the font attributes structure get filled in with
 *	information from argc/argv.  If an error occurs while parsing,
 *	the font attributes structure will contain all modifications
 *	specified in the command line options up to the point of the
 *	error.
 *
 *---------------------------------------------------------------------------
 */

static int
ConfigAttributesObj(interp, tkwin, objc, objv, faPtr)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tk_Window tkwin;		/* For display on which font will be used. */
    int objc;			/* Number of elements in argv. */
    Tcl_Obj *CONST objv[];	/* Command line options. */
    TkFontAttributes *faPtr;	/* Font attributes structure whose fields
				 * are to be modified.  Structure must already
				 * be properly initialized. */
{
    int i, n, index;
    Tcl_Obj *optionPtr, *valuePtr;
    char *value;
    
    for (i = 0; i < objc; i += 2) {
	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, fontOpt, "option", 1,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((i+2 >= objc) && (objc & 1)) {
	    /*
	     * This test occurs after Tcl_GetIndexFromObj() so that
	     * "font create xyz -xyz" will return the error message
	     * that "-xyz" is a bad option, rather than that the value
	     * for "-xyz" is missing.
	     */

	    Tcl_AppendResult(interp, "value for \"",
		    Tcl_GetString(optionPtr), "\" option missing",
		    (char *) NULL);
	    return TCL_ERROR;
	}

	switch (index) {
	    case FONT_FAMILY: {
		value = Tcl_GetString(valuePtr);
		faPtr->family = Tk_GetUid(value);
		break;
	    }
	    case FONT_SIZE: {
		if (Tcl_GetIntFromObj(interp, valuePtr, &n) != TCL_OK) {
		    return TCL_ERROR;
		}
		faPtr->size = n;
		break;
	    }
	    case FONT_WEIGHT: {
		n = TkFindStateNumObj(interp, optionPtr, weightMap, valuePtr);
		if (n == TK_FW_UNKNOWN) {
		    return TCL_ERROR;
		}
		faPtr->weight = n;
		break;
	    }
	    case FONT_SLANT: {
		n = TkFindStateNumObj(interp, optionPtr, slantMap, valuePtr);
		if (n == TK_FS_UNKNOWN) {
		    return TCL_ERROR;
		}
		faPtr->slant = n;
		break;
	    }
	    case FONT_UNDERLINE: {
		if (Tcl_GetBooleanFromObj(interp, valuePtr, &n) != TCL_OK) {
		    return TCL_ERROR;
		}
		faPtr->underline = n;
		break;
	    }
	    case FONT_OVERSTRIKE: {
		if (Tcl_GetBooleanFromObj(interp, valuePtr, &n) != TCL_OK) {
		    return TCL_ERROR;
		}
		faPtr->overstrike = n;
		break;
	    }
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetAttributeInfoObj --
 *
 *	Return information about the font attributes as a Tcl list.
 *
 * Results:
 *	The return value is TCL_OK if the objPtr was non-NULL and
 *	specified a valid font attribute, TCL_ERROR otherwise.  If TCL_OK
 *	is returned, the interp's result object is modified to hold a
 *	description of either the current value of a single option, or a
 *	list of all options and their current values for the given font
 *	attributes.  If TCL_ERROR is returned, the interp's result is
 *	set to an error message describing that the objPtr did not refer
 *	to a valid option.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
GetAttributeInfoObj(interp, faPtr, objPtr)
    Tcl_Interp *interp;		  	/* Interp to hold result. */
    CONST TkFontAttributes *faPtr;	/* The font attributes to inspect. */
    Tcl_Obj *objPtr;		  	/* If non-NULL, indicates the single
					 * option whose value is to be
					 * returned. Otherwise information is
					 * returned for all options. */
{
    int i, index, start, end;
    char *str;
    Tcl_Obj *optionPtr, *valuePtr, *resultPtr;

    resultPtr = Tcl_GetObjResult(interp);

    start = 0;
    end = FONT_NUMFIELDS;
    if (objPtr != NULL) {
	if (Tcl_GetIndexFromObj(interp, objPtr, fontOpt, "option", TCL_EXACT,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	start = index;
	end = index + 1;
    }

    valuePtr = NULL;
    for (i = start; i < end; i++) {
	switch (i) {
	    case FONT_FAMILY:
		str = faPtr->family;
		valuePtr = Tcl_NewStringObj(str, ((str == NULL) ? 0 : -1));
		break;

	    case FONT_SIZE:
		valuePtr = Tcl_NewIntObj(faPtr->size);
		break;

	    case FONT_WEIGHT:
		str = TkFindStateString(weightMap, faPtr->weight);
		valuePtr = Tcl_NewStringObj(str, -1);
		break;
	
	    case FONT_SLANT:
		str = TkFindStateString(slantMap, faPtr->slant);
		valuePtr = Tcl_NewStringObj(str, -1);
		break;

	    case FONT_UNDERLINE:
		valuePtr = Tcl_NewBooleanObj(faPtr->underline);
		break;

	    case FONT_OVERSTRIKE:
		valuePtr = Tcl_NewBooleanObj(faPtr->overstrike);
		break;
	}
	if (objPtr != NULL) {
	    Tcl_SetObjResult(interp, valuePtr);
	    return TCL_OK;
	}
	optionPtr = Tcl_NewStringObj(fontOpt[i], -1);
	Tcl_ListObjAppendElement(NULL, resultPtr, optionPtr);
	Tcl_ListObjAppendElement(NULL, resultPtr, valuePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseFontNameObj --
 *
 *	Converts a object into a set of font attributes that can be used
 *	to construct a font.
 *
 *	The string rep of the object can be one of the following forms:
 *		XLFD (see X documentation)
 *		"family [size] [style1 [style2 ...]"
 *		"-option value [-option value ...]"
 *
 * Results:
 *	The return value is TCL_ERROR if the object was syntactically
 *	invalid.  In that case an error message is left in interp's
 *	result object.  Otherwise, fills the font attribute buffer with
 *	the values parsed from the string and returns TCL_OK;
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
ParseFontNameObj(interp, tkwin, objPtr, faPtr)
    Tcl_Interp *interp;		/* Interp for error return.  Must not be
				 * NULL. */
    Tk_Window tkwin;		/* For display on which font is used. */
    Tcl_Obj *objPtr;		/* Parseable font description object. */
    TkFontAttributes *faPtr;	/* Filled with attributes parsed from font
				 * name.  Any attributes that were not
				 * specified in font name are filled with
				 * default values. */
{
    char *dash;
    int objc, result, i, n;
    Tcl_Obj **objv;
    Tcl_Obj *resultPtr;
    char *string;
    
    TkInitFontAttributes(faPtr);
    resultPtr = Tcl_GetObjResult(interp);

    string = Tcl_GetString(objPtr);
    if (*string == '-') {
	/*
	 * This may be an XLFD or an "-option value" string.
	 *
	 * If the string begins with "-*" or a "-foundry-family-*" pattern,
	 * then consider it an XLFD.  
	 */

	if (string[1] == '*') {
	    goto xlfd;
	}
	dash = strchr(string + 1, '-');
	if ((dash != NULL)
		&& (!isspace(UCHAR(dash[-1])))) { /* INTL: ISO space */
	    goto xlfd;
	}

	if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;
	}

	return ConfigAttributesObj(interp, tkwin, objc, objv, faPtr);
    }
    
    if (*string == '*') {
	/*
	 * This is appears to be an XLFD.  Under Unix, all valid XLFDs were
	 * already handled by TkpGetNativeFont.  If we are here, either we
	 * have something that initially looks like an XLFD but isn't or we
	 * have encountered an XLFD on Windows or Mac.
	 */

	xlfd:
	result = TkFontParseXLFD(string, faPtr, NULL);
	if (result == TCL_OK) {
	    return TCL_OK;
	}
    }

    /*
     * Wasn't an XLFD or "-option value" string.  Try it as a
     * "font size style" list.
     */

    if ((Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv) != TCL_OK)
	    || (objc < 1)) {
	Tcl_AppendResult(interp, "font \"", string, "\" doesn't exist",
		(char *) NULL);
	return TCL_ERROR;
    }

    faPtr->family = Tk_GetUid(Tcl_GetString(objv[0]));
    if (objc > 1) {
	if (Tcl_GetIntFromObj(interp, objv[1], &n) != TCL_OK) {
	    return TCL_ERROR;
	}
	faPtr->size = n;
    }

    i = 2;
    if (objc == 3) {
	if (Tcl_ListObjGetElements(interp, objv[2], &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	i = 0;
    }
    for ( ; i < objc; i++) {
	n = TkFindStateNumObj(NULL, NULL, weightMap, objv[i]);
	if (n != TK_FW_UNKNOWN) {
	    faPtr->weight = n;
	    continue;
	}
	n = TkFindStateNumObj(NULL, NULL, slantMap, objv[i]);
	if (n != TK_FS_UNKNOWN) {
	    faPtr->slant = n;
	    continue;
	}
	n = TkFindStateNumObj(NULL, NULL, underlineMap, objv[i]);
	if (n != 0) {
	    faPtr->underline = n;
	    continue;
	}
	n = TkFindStateNumObj(NULL, NULL, overstrikeMap, objv[i]);
	if (n != 0) {
	    faPtr->overstrike = n;
	    continue;
	}

	/*
	 * Unknown style.
	 */

	Tcl_AppendResult(interp, "unknown font style \"",
		Tcl_GetString(objv[i]), "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewChunk --
 *
 *	Helper function for Tk_ComputeTextLayout().  Encapsulates a
 *	measured set of characters in a chunk that can be quickly
 *	drawn.
 *
 * Results:
 *	A pointer to the new chunk in the text layout.
 *
 * Side effects:
 *	The text layout is reallocated to hold more chunks as necessary.
 *
 *	Currently, Tk_ComputeTextLayout() stores contiguous ranges of
 *	"normal" characters in a chunk, along with individual tab
 *	and newline chars in their own chunks.  All characters in the
 *	text layout are accounted for.
 *
 *---------------------------------------------------------------------------
 */
static LayoutChunk *
NewChunk(layoutPtrPtr, maxPtr, start, numBytes, curX, newX, y)
    TextLayout **layoutPtrPtr;
    int *maxPtr;
    CONST char *start;
    int numBytes;
    int curX;
    int newX;
    int y;
{
    TextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    int maxChunks, numChars;
    size_t s;
    
    layoutPtr = *layoutPtrPtr;
    maxChunks = *maxPtr;
    if (layoutPtr->numChunks == maxChunks) {
	maxChunks *= 2;
	s = sizeof(TextLayout) + ((maxChunks - 1) * sizeof(LayoutChunk));
	layoutPtr = (TextLayout *) ckrealloc((char *) layoutPtr, s);

	*layoutPtrPtr = layoutPtr;
	*maxPtr = maxChunks;
    }
    numChars = Tcl_NumUtfChars(start, numBytes);
    chunkPtr = &layoutPtr->chunks[layoutPtr->numChunks];
    chunkPtr->start		= start;
    chunkPtr->numBytes		= numBytes;
    chunkPtr->numChars		= numChars;
    chunkPtr->numDisplayChars	= numChars;
    chunkPtr->x			= curX;
    chunkPtr->y			= y;
    chunkPtr->totalWidth	= newX - curX;
    chunkPtr->displayWidth	= newX - curX;
    layoutPtr->numChunks++;

    return chunkPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFontParseXLFD --
 *
 *	Break up a fully specified XLFD into a set of font attributes.
 *
 * Results:
 *	Return value is TCL_ERROR if string was not a fully specified XLFD.
 *	Otherwise, fills font attribute buffer with the values parsed
 *	from the XLFD and returns TCL_OK.  
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkFontParseXLFD(string, faPtr, xaPtr)
    CONST char *string;		/* Parseable font description string. */
    TkFontAttributes *faPtr;	/* Filled with attributes parsed from font
				 * name.  Any attributes that were not
				 * specified in font name are filled with
				 * default values. */
    TkXLFDAttributes *xaPtr;	/* Filled with X-specific attributes parsed
				 * from font name.  Any attributes that were
				 * not specified in font name are filled with
				 * default values.  May be NULL if such
				 * information is not desired. */
{
    char *src;
    CONST char *str;
    int i, j;
    char *field[XLFD_NUMFIELDS + 2];
    Tcl_DString ds;
    TkXLFDAttributes xa;
    
    if (xaPtr == NULL) {
	xaPtr = &xa;
    }
    TkInitFontAttributes(faPtr);
    TkInitXLFDAttributes(xaPtr);

    memset(field, '\0', sizeof(field));

    str = string;
    if (*str == '-') {
	str++;
    }

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, (char *) str, -1);
    src = Tcl_DStringValue(&ds);

    field[0] = src;
    for (i = 0; *src != '\0'; src++) {
	if (!(*src & 0x80)
		&& Tcl_UniCharIsUpper(UCHAR(*src))) {
	    *src = (char) Tcl_UniCharToLower(UCHAR(*src));
	}
	if (*src == '-') {
	    i++;
	    if (i == XLFD_NUMFIELDS) {
		continue;
	    }
	    *src = '\0';
	    field[i] = src + 1;
	    if (i > XLFD_NUMFIELDS) {
		break;
	    }
	}
    }

    /*
     * An XLFD of the form -adobe-times-medium-r-*-12-*-* is pretty common,
     * but it is (strictly) malformed, because the first * is eliding both
     * the Setwidth and the Addstyle fields.  If the Addstyle field is a
     * number, then assume the above incorrect form was used and shift all
     * the rest of the fields right by one, so the number gets interpreted
     * as a pixelsize.  This fix is so that we don't get a million reports
     * that "it works under X (as a native font name), but gives a syntax
     * error under Windows (as a parsed set of attributes)".
     */

    if ((i > XLFD_ADD_STYLE) && (FieldSpecified(field[XLFD_ADD_STYLE]))) {
	if (atoi(field[XLFD_ADD_STYLE]) != 0) {
	    for (j = XLFD_NUMFIELDS - 1; j >= XLFD_ADD_STYLE; j--) {
		field[j + 1] = field[j];
	    }
	    field[XLFD_ADD_STYLE] = NULL;
	    i++;
	}
    }

    /*
     * Bail if we don't have enough of the fields (up to pointsize).
     */

    if (i < XLFD_FAMILY) {
	Tcl_DStringFree(&ds);
	return TCL_ERROR;
    }

    if (FieldSpecified(field[XLFD_FOUNDRY])) {
	xaPtr->foundry = Tk_GetUid(field[XLFD_FOUNDRY]);
    }

    if (FieldSpecified(field[XLFD_FAMILY])) {
	faPtr->family = Tk_GetUid(field[XLFD_FAMILY]);
    }
    if (FieldSpecified(field[XLFD_WEIGHT])) {
	faPtr->weight = TkFindStateNum(NULL, NULL, xlfdWeightMap,
		field[XLFD_WEIGHT]);
    }
    if (FieldSpecified(field[XLFD_SLANT])) {
	xaPtr->slant = TkFindStateNum(NULL, NULL, xlfdSlantMap,
		field[XLFD_SLANT]);
	if (xaPtr->slant == TK_FS_ROMAN) {
	    faPtr->slant = TK_FS_ROMAN;
	} else {
	    faPtr->slant = TK_FS_ITALIC;
	}
    }
    if (FieldSpecified(field[XLFD_SETWIDTH])) {
	xaPtr->setwidth = TkFindStateNum(NULL, NULL, xlfdSetwidthMap,
		field[XLFD_SETWIDTH]);
    }

    /* XLFD_ADD_STYLE ignored. */

    /*
     * Pointsize in tenths of a point, but treat it as tenths of a pixel
     * for historical compatibility.
     */

    faPtr->size = 12;

    if (FieldSpecified(field[XLFD_POINT_SIZE])) {
	if (field[XLFD_POINT_SIZE][0] == '[') {
	    /*
	     * Some X fonts have the point size specified as follows:
	     *
	     *	    [ N1 N2 N3 N4 ]
	     *
	     * where N1 is the point size (in points, not decipoints!), and
	     * N2, N3, and N4 are some additional numbers that I don't know
	     * the purpose of, so I ignore them.
	     */

	    faPtr->size = atoi(field[XLFD_POINT_SIZE] + 1);
	} else if (Tcl_GetInt(NULL, field[XLFD_POINT_SIZE],
		&faPtr->size) == TCL_OK) {
	    faPtr->size /= 10;
	} else {
	    return TCL_ERROR;
	}
    }

    /*
     * Pixel height of font.  If specified, overrides pointsize.
     */

    if (FieldSpecified(field[XLFD_PIXEL_SIZE])) {
	if (field[XLFD_PIXEL_SIZE][0] == '[') {
	    /*
	     * Some X fonts have the pixel size specified as follows:
	     *
	     *	    [ N1 N2 N3 N4 ]
	     *
	     * where N1 is the pixel size, and where N2, N3, and N4 
	     * are some additional numbers that I don't know
	     * the purpose of, so I ignore them.
	     */

	    faPtr->size = atoi(field[XLFD_PIXEL_SIZE] + 1);
	} else if (Tcl_GetInt(NULL, field[XLFD_PIXEL_SIZE],
		&faPtr->size) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    faPtr->size = -faPtr->size;

    /* XLFD_RESOLUTION_X ignored. */

    /* XLFD_RESOLUTION_Y ignored. */

    /* XLFD_SPACING ignored. */

    /* XLFD_AVERAGE_WIDTH ignored. */

    if (FieldSpecified(field[XLFD_CHARSET])) {
	xaPtr->charset = Tk_GetUid(field[XLFD_CHARSET]);
    } else {
	xaPtr->charset = Tk_GetUid("iso8859-1");
    }
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FieldSpecified --
 *
 *	Helper function for TkParseXLFD().  Determines if a field in the
 *	XLFD was set to a non-null, non-don't-care value.
 *
 * Results:
 *	The return value is 0 if the field in the XLFD was not set and
 *	should be ignored, non-zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
FieldSpecified(field)
    CONST char *field;	/* The field of the XLFD to check.  Strictly
			 * speaking, only when the string is "*" does it mean
			 * don't-care.  However, an unspecified or question
			 * mark is also interpreted as don't-care. */
{
    char ch;

    if (field == NULL) {
	return 0;
    }
    ch = field[0];
    return (ch != '*' && ch != '?');
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFontGetPixels --
 *
 *	Given a font size specification (as described in the TkFontAttributes
 *	structure) return the number of pixels it represents.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
 
int
TkFontGetPixels(tkwin, size)
    Tk_Window tkwin;		/* For point->pixel conversion factor. */
    int size;			/* Font size. */
{
    double d;

    if (size < 0) {
	return -size;
    }

    d = size * 25.4 / 72.0;
    d *= WidthOfScreen(Tk_Screen(tkwin));
    d /= WidthMMOfScreen(Tk_Screen(tkwin));
    return (int) (d + 0.5);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFontGetPoints --
 *
 *	Given a font size specification (as described in the TkFontAttributes
 *	structure) return the number of points it represents.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
 
int
TkFontGetPoints(tkwin, size)
    Tk_Window tkwin;		/* For pixel->point conversion factor. */
    int size;			/* Font size. */
{
    double d;

    if (size >= 0) {
	return size;
    }

    d = -size * 72.0 / 25.4;
    d *= WidthMMOfScreen(Tk_Screen(tkwin));
    d /= WidthOfScreen(Tk_Screen(tkwin));
    return (int) (d + 0.5);
}

/*
 *-------------------------------------------------------------------------
 *
 * TkFontGetAliasList --
 *
 *	Given a font name, find the list of all aliases for that font
 *	name.  One of the names in this list will probably be the name
 *	that this platform expects when asking for the font.
 *
 * Results:
 *	As above.  The return value is NULL if the font name has no 
 *	aliases.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
	
char **
TkFontGetAliasList(faceName)
    CONST char *faceName;	/* Font name to test for aliases. */
{   
    int i, j;

    for (i = 0; fontAliases[i] != NULL; i++) {
	for (j = 0; fontAliases[i][j] != NULL; j++) {
	    if (strcasecmp(faceName, fontAliases[i][j]) == 0) {
		return fontAliases[i];
	    }
	}
    }
    return NULL;
}

/*
 *-------------------------------------------------------------------------
 *
 * TkFontGetFallbacks --
 *
 *	Get the list of font fallbacks that the platform-specific code
 *	can use to try to find the closest matching font the name 
 *	requested.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
	
char ***
TkFontGetFallbacks()
{
    return fontFallbacks;
}

/*
 *-------------------------------------------------------------------------
 *
 * TkFontGetGlobalClass --
 *
 *	Get the list of fonts to try if the requested font name does not
 *	exist and no fallbacks for that font name could be used either.
 *	The names in this list are considered preferred over all the other
 *	font names in the system when looking for a last-ditch fallback.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
	
char **
TkFontGetGlobalClass()
{
    return globalFontClass;
}

/*
 *-------------------------------------------------------------------------
 *
 * TkFontGetSymbolClass --
 *
 *	Get the list of fonts that are symbolic; used if the operating 
 *	system cannot apriori identify symbolic fonts on its own.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
	
char **
TkFontGetSymbolClass()
{
    return symbolClass;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDebugFont --
 *
 *	This procedure returns debugging information about a font.
 *
 * Results:
 *	The return value is a list with one sublist for each TkFont
 *	corresponding to "name".  Each sublist has two elements that
 *	contain the resourceRefCount and objRefCount fields from the
 *	TkFont structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkDebugFont(tkwin, name)
    Tk_Window tkwin;		/* The window in which the font will be
				 * used (not currently used). */
    char *name;			/* Name of the desired color. */
{
    TkFont *fontPtr;
    Tcl_HashEntry *hashPtr;
    Tcl_Obj *resultPtr, *objPtr;

    resultPtr = Tcl_NewObj();
    hashPtr = Tcl_FindHashEntry(
	    &((TkWindow *) tkwin)->mainPtr->fontInfoPtr->fontCache, name);
    if (hashPtr != NULL) {
	fontPtr = (TkFont *) Tcl_GetHashValue(hashPtr);
	if (fontPtr == NULL) {
	    panic("TkDebugFont found empty hash table entry");
	}
	for ( ; (fontPtr != NULL); fontPtr = fontPtr->nextPtr) {
	    objPtr = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(fontPtr->resourceRefCount));
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(fontPtr->objRefCount)); 
	    Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
	}
    }
    return resultPtr;
}
