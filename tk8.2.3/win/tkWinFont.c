/* 
 * tkWinFont.c --
 *
 *	Contains the Windows implementation of the platform-independant
 *	font package interface.
 *
 * Copyright (c) 1994 Software Research Associates, Inc. 
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"
#include "tkFont.h"

/*
 * The following structure represents a font family.  It is assumed that
 * all screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a
 * shared instance of this structure.  The most important shared property
 * is the character existence metrics, used to determine if a screen font
 * can display a given Unicode character.
 *
 * Under Windows, a "font family" is uniquely identified by its face name.
 */

#define FONTMAP_SHIFT	    10

#define FONTMAP_PAGES	    	(1 << (sizeof(Tcl_UniChar)*8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE	(1 << FONTMAP_SHIFT)

typedef struct FontFamily {
    struct FontFamily *nextPtr;	/* Next in list of all known font families. */
    int refCount;		/* How many SubFonts are referring to this
				 * FontFamily.  When the refCount drops to
				 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */
     
    Tk_Uid faceName;		/* Face name key for this FontFamily. */

    /*
     * Derived properties.
     */
     
    Tcl_Encoding encoding;	/* Encoding for this font family. */
    int isSymbolFont;		/* Non-zero if this is a symbol font. */
    int isWideFont;		/* 1 if this is a double-byte font, 0 
				 * otherwise. */
    BOOL (WINAPI *textOutProc)(HDC, int, int, TCHAR *, int);
				/* The procedure to use to draw text after
				 * it has been converted from UTF-8 to the 
				 * encoding of this font. */
    BOOL (WINAPI *getTextExtentPoint32Proc)(HDC, TCHAR *, int, LPSIZE);
				/* The procedure to use to measure text after
				 * it has been converted from UTF-8 to the 
				 * encoding of this font. */

    char *fontMap[FONTMAP_PAGES];
				/* Two-level sparse table used to determine
				 * quickly if the specified character exists.
				 * As characters are encountered, more pages
				 * in this table are dynamically added.  The
				 * contents of each page is a bitmask
				 * consisting of FONTMAP_BITSPERPAGE bits,
				 * representing whether this font can be used
				 * to display the given character at the
				 * corresponding bit position.  The high bits
				 * of the character are used to pick which
				 * page of the table is used. */

    /*
     * Cached Truetype font info.
     */
     
    int segCount;		/* The length of the following arrays. */
    USHORT *startCount;		/* Truetype information about the font, */
    USHORT *endCount;		/* indicating which characters this font
				 * can display (malloced).  The format of
				 * this information is (relatively) compact,
				 * but would take longer to search than 
				 * indexing into the fontMap[][] table. */
} FontFamily;

/*
 * The following structure encapsulates an individual screen font.  A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */

typedef struct SubFont {
    char **fontMap;		/* Pointer to font map from the FontFamily, 
				 * cached here to save a dereference. */
    HFONT hFont;		/* The specific screen font that will be
				 * used when displaying/measuring chars
				 * belonging to the FontFamily. */
    FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Windows' implementation of a font
 * object.
 */

#define SUBFONT_SPACE		3
#define BASE_CHARS		128

typedef struct WinFont {
    TkFont font;		/* Stuff used by generic font package.  Must
				 * be first in structure. */
    SubFont staticSubFonts[SUBFONT_SPACE];
				/* Builtin space for a limited number of
				 * SubFonts. */
    int numSubFonts;		/* Length of following array. */
    SubFont *subFontArray;	/* Array of SubFonts that have been loaded
				 * in order to draw/measure all the characters
				 * encountered by this font so far.  All fonts
				 * start off with one SubFont initialized by
				 * AllocFont() from the original set of font
				 * attributes.  Usually points to
				 * staticSubFonts, but may point to malloced
				 * space if there are lots of SubFonts. */

    HWND hwnd;			/* Toplevel window of application that owns
				 * this font, used for getting HDC for
				 * offscreen measurements. */
    int pixelSize;		/* Original pixel size used when font was
				 * constructed. */
    int widths[BASE_CHARS];	/* Widths of first 128 chars in the base
				 * font, for handling common case.  The base
				 * font is always used to draw characters
				 * between 0x0000 and 0x007f. */
} WinFont;

/*
 * The following structure is passed as the LPARAM when calling the font
 * enumeration procedure to determine if a font can support the given
 * character.
 */

typedef struct CanUse {
    HDC hdc;
    WinFont *fontPtr;
    Tcl_DString *nameTriedPtr;
    int ch;
    SubFont *subFontPtr;
} CanUse;

/*
 * The following structure is used to map between the Tcl strings that
 * represent the system fonts and the numbers used by Windows.
 */

static TkStateMap systemMap[] = {
    {ANSI_FIXED_FONT,	    "ansifixed"},
    {ANSI_VAR_FONT,	    "ansi"},
    {DEVICE_DEFAULT_FONT,   "device"},
    {OEM_FIXED_FONT,	    "oemfixed"},
    {SYSTEM_FIXED_FONT,	    "systemfixed"},
    {SYSTEM_FONT,	    "system"},
    {-1,		    NULL}
};

typedef struct ThreadSpecificData {
    FontFamily *fontFamilyList; /* The list of font families that are 
				 * currently loaded.  As screen fonts
				 * are loaded, this list grows to hold 
				 * information about what characters
				 * exist in each font family.  */
    Tcl_HashTable uidTable;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Information cached about the system at startup time.
 */
 
static int platformId;
static Tcl_Encoding unicodeEncoding;
static Tcl_Encoding systemEncoding;

/*
 * Procedures used only in this file.
 */

static FontFamily *	AllocFontFamily(HDC hdc, HFONT hFont, int base);
static SubFont *	CanUseFallback(HDC hdc, WinFont *fontPtr, 
			    char *fallbackName,	int ch);
static SubFont *	CanUseFallbackWithAliases(HDC hdc, WinFont *fontPtr, 
			    char *faceName, int ch, Tcl_DString *nameTriedPtr);
static int		FamilyExists(HDC hdc, CONST char *faceName);
static char *		FamilyOrAliasExists(HDC hdc, CONST char *faceName);
static SubFont *	FindSubFontForChar(WinFont *fontPtr, int ch);
static void		FontMapInsert(SubFont *subFontPtr, int ch);
static void		FontMapLoadPage(SubFont *subFontPtr, int row);
static int		FontMapLookup(SubFont *subFontPtr, int ch);
static void		FreeFontFamily(FontFamily *familyPtr);
static HFONT		GetScreenFont(CONST TkFontAttributes *faPtr,
			    CONST char *faceName, int pixelSize);
static void		InitFont(Tk_Window tkwin, HFONT hFont, 
			    int overstrike, WinFont *tkFontPtr);
static void		InitSubFont(HDC hdc, HFONT hFont, int base, 
			    SubFont *subFontPtr);
static int		LoadFontRanges(HDC hdc, HFONT hFont, 
			    USHORT **startCount, USHORT **endCount,
			    int *symbolPtr);
static void		MultiFontTextOut(HDC hdc, WinFont *fontPtr, 
			    CONST char *source, int numBytes, int x, int y);
static void		ReleaseFont(WinFont *fontPtr);
static void		ReleaseSubFont(SubFont *subFontPtr);
static int		SeenName(CONST char *name, Tcl_DString *dsPtr);
static void		SwapLong(PULONG p);
static void		SwapShort(USHORT *p);
static int CALLBACK	WinFontCanUseProc(ENUMLOGFONT *lfPtr, 
			    NEWTEXTMETRIC *tmPtr, int fontType, 
			    LPARAM lParam);
static int CALLBACK	WinFontExistProc(ENUMLOGFONT *lfPtr, 
			    NEWTEXTMETRIC *tmPtr, int fontType, 
			    LPARAM lParam);
static int CALLBACK	WinFontFamilyEnumProc(ENUMLOGFONT *lfPtr, 
			    NEWTEXTMETRIC *tmPtr, int fontType, 
			    LPARAM lParam);

/*
 *-------------------------------------------------------------------------
 * 
 * TkpFontPkgInit --
 *
 *	This procedure is called when an application is created.  It
 *	initializes all the structures that are used by the 
 *	platform-dependant code on a per application basis.
 *
 * Results:
 *	None.  
 *
 * Side effects:
 *	
 *	None.
 *
 *-------------------------------------------------------------------------
 */

void
TkpFontPkgInit(
    TkMainInfo *mainPtr)	/* The application being created. */
{
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionEx(&os);
    platformId = os.dwPlatformId;
    unicodeEncoding = Tcl_GetEncoding(NULL, "unicode");
    if (platformId == VER_PLATFORM_WIN32_NT) {
	/*
	 * If running NT, then we will be calling some Unicode functions 
	 * explictly.  So, even if the Tcl system encoding isn't Unicode, 
	 * make sure we convert to/from the Unicode char set. 
	 */

	systemEncoding = unicodeEncoding;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetNativeFont --
 *
 *	Map a platform-specific native font name to a TkFont.
 *
 * Results:
 * 	The return value is a pointer to a TkFont that represents the
 *	native font.  If a native font by the given name could not be
 *	found, the return value is NULL.  
 *
 *	Every call to this procedure returns a new TkFont structure,
 *	even if the name has already been seen before.  The caller should
 *	call TkpDeleteFont() when the font is no longer needed.
 *
 *	The caller is responsible for initializing the memory associated
 *	with the generic TkFont when this function returns and releasing
 *	the contents of the generic TkFont before calling TkpDeleteFont().
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

TkFont *
TkpGetNativeFont(
    Tk_Window tkwin,		/* For display where font will be used. */
    CONST char *name)		/* Platform-specific font name. */
{
    int object;
    WinFont *fontPtr;

    object = TkFindStateNum(NULL, NULL, systemMap, name);
    if (object < 0) {
	return NULL;
    }

    tkwin = (Tk_Window) ((TkWindow *) tkwin)->mainPtr->winPtr;
    fontPtr = (WinFont *) ckalloc(sizeof(WinFont));
    InitFont(tkwin, GetStockObject(object), 0, fontPtr);

    return (TkFont *) fontPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetFontFromAttributes -- 
 *
 *	Given a desired set of attributes for a font, find a font with
 *	the closest matching attributes.
 *
 * Results:
 * 	The return value is a pointer to a TkFont that represents the
 *	font with the desired attributes.  If a font with the desired
 *	attributes could not be constructed, some other font will be
 *	substituted automatically.  NULL is never returned.
 *
 *	Every call to this procedure returns a new TkFont structure,
 *	even if the specified attributes have already been seen before.
 *	The caller should call TkpDeleteFont() to free the platform-
 *	specific data when the font is no longer needed.  
 *
 *	The caller is responsible for initializing the memory associated
 *	with the generic TkFont when this function returns and releasing
 *	the contents of the generic TkFont before calling TkpDeleteFont().
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

TkFont *
TkpGetFontFromAttributes(
    TkFont *tkFontPtr,		/* If non-NULL, store the information in
				 * this existing TkFont structure, rather than
				 * allocating a new structure to hold the
				 * font; the existing contents of the font
				 * will be released.  If NULL, a new TkFont
				 * structure is allocated. */
    Tk_Window tkwin,		/* For display where font will be used. */
    CONST TkFontAttributes *faPtr)
				/* Set of attributes to match. */
{
    int i, j;
    HDC hdc;
    HWND hwnd;
    HFONT hFont;
    Window window;
    WinFont *fontPtr;
    char ***fontFallbacks;
    char *faceName, *fallback, *actualName;

    tkwin   = (Tk_Window) ((TkWindow *) tkwin)->mainPtr->winPtr;
    window  = Tk_WindowId(tkwin);
    hwnd    = (window == None) ? NULL : TkWinGetHWND(window);
    hdc	    = GetDC(hwnd);

    /*
     * Algorithm to get the closest font name to the one requested.
     *
     * try fontname
     * try all aliases for fontname
     * foreach fallback for fontname
     *	    try the fallback
     *	    try all aliases for the fallback
     */

    faceName = faPtr->family;
    if (faceName != NULL) {
	actualName = FamilyOrAliasExists(hdc, faceName);
	if (actualName != NULL) {
	    faceName = actualName;
	    goto found;
	}
	fontFallbacks = TkFontGetFallbacks();
	for (i = 0; fontFallbacks[i] != NULL; i++) {
	    for (j = 0; (fallback = fontFallbacks[i][j]) != NULL; j++) {
		if (strcasecmp(faceName, fallback) == 0) {
		    break;
		}
	    }
	    if (fallback != NULL) {
		for (j = 0; (fallback = fontFallbacks[i][j]) != NULL; j++) {
		    actualName = FamilyOrAliasExists(hdc, fallback);
		    if (actualName != NULL) {
			faceName = actualName;
			goto found;
		    }
		}
	    }
	}
    }

    found:
    ReleaseDC(hwnd, hdc);

    hFont = GetScreenFont(faPtr, faceName, TkFontGetPixels(tkwin, faPtr->size));
    if (tkFontPtr == NULL) {
	fontPtr = (WinFont *) ckalloc(sizeof(WinFont));
    } else {
	fontPtr = (WinFont *) tkFontPtr;
	ReleaseFont(fontPtr);
    }
    InitFont(tkwin, hFont, faPtr->overstrike, fontPtr);

    return (TkFont *) fontPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpDeleteFont --
 *
 *	Called to release a font allocated by TkpGetNativeFont() or
 *	TkpGetFontFromAttributes().  The caller should have already
 *	released the fields of the TkFont that are used exclusively by
 *	the generic TkFont code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TkFont is deallocated.
 *
 *---------------------------------------------------------------------------
 */

void
TkpDeleteFont(
    TkFont *tkFontPtr)		/* Token of font to be deleted. */
{
    WinFont *fontPtr;

    fontPtr = (WinFont *) tkFontPtr;
    ReleaseFont(fontPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetFontFamilies, WinFontFamilyEnumProc --
 *
 *	Return information about the font families that are available
 *	on the display of the given window.
 *
 * Results:
 *	Modifies interp's result object to hold a list of all the available
 *	font families.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
 
void
TkpGetFontFamilies(
    Tcl_Interp *interp,		/* Interp to hold result. */
    Tk_Window tkwin)		/* For display to query. */
{    
    HDC hdc;
    HWND hwnd;
    Window window;

    window  = Tk_WindowId(tkwin);
    hwnd    = (window == None) ? NULL : TkWinGetHWND(window);
    hdc	    = GetDC(hwnd);

    /*
     * On any version NT, there may fonts with international names.  
     * Use the NT-only Unicode version of EnumFontFamilies to get the 
     * font names.  If we used the ANSI version on a non-internationalized 
     * version of NT, we would get font names with '?' replacing all 
     * the international characters.
     *
     * On a non-internationalized verson of 95, fonts with international
     * names are not allowed, so the ANSI version of EnumFontFamilies will 
     * work.  On an internationalized version of 95, there may be fonts with 
     * international names; the ANSI version will work, fetching the 
     * name in the system code page.  Can't use the Unicode version of 
     * EnumFontFamilies because it only exists under NT.
     */

    if (platformId == VER_PLATFORM_WIN32_NT) {
	EnumFontFamiliesW(hdc, NULL, (FONTENUMPROCW) WinFontFamilyEnumProc,
		(LPARAM) interp);
    } else {
	EnumFontFamiliesA(hdc, NULL, (FONTENUMPROCA) WinFontFamilyEnumProc,
		(LPARAM) interp);
    }	    
    ReleaseDC(hwnd, hdc);
}

static int CALLBACK
WinFontFamilyEnumProc(
    ENUMLOGFONT *lfPtr,		/* Logical-font data. */
    NEWTEXTMETRIC *tmPtr,	/* Physical-font data (not used). */
    int fontType,		/* Type of font (not used). */
    LPARAM lParam)		/* Result object to hold result. */
{
    char *faceName;
    Tcl_DString faceString;
    Tcl_Obj *strPtr;
    Tcl_Interp *interp;

    interp = (Tcl_Interp *) lParam;
    faceName = lfPtr->elfLogFont.lfFaceName;
    Tcl_ExternalToUtfDString(systemEncoding, faceName, -1, &faceString);
    strPtr = Tcl_NewStringObj(Tcl_DStringValue(&faceString),
	    Tcl_DStringLength(&faceString));
    Tcl_ListObjAppendElement(NULL, Tcl_GetObjResult(interp), strPtr);
    Tcl_DStringFree(&faceString);
    return 1;
}

/*
 *-------------------------------------------------------------------------
 *
 * TkpGetSubFonts --
 *
 *	A function used by the testing package for querying the actual 
 *	screen fonts that make up a font object.
 *
 * Results:
 *	Modifies interp's result object to hold a list containing the 
 *	names of the screen fonts that make up the given font object.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
	
void
TkpGetSubFonts(
    Tcl_Interp *interp,		/* Interp to hold result. */
    Tk_Font tkfont)		/* Font object to query. */
{
    int i;
    WinFont *fontPtr;
    FontFamily *familyPtr;
    Tcl_Obj *resultPtr, *strPtr;

    resultPtr = Tcl_GetObjResult(interp);    
    fontPtr = (WinFont *) tkfont;
    for (i = 0; i < fontPtr->numSubFonts; i++) {
	familyPtr = fontPtr->subFontArray[i].familyPtr;
	strPtr = Tcl_NewStringObj(familyPtr->faceName, -1);
	Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  Tk_MeasureChars --
 *
 *	Determine the number of bytes from the string that will fit
 *	in the given horizontal span.  The measurement is done under the
 *	assumption that Tk_DrawChars() will be used to actually display
 *	the characters.
 *
 * Results:
 *	The return value is the number of bytes from source that
 *	fit into the span that extends from 0 to maxLength.  *lengthPtr is
 *	filled with the x-coordinate of the right edge of the last
 *	character that did fit.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_MeasureChars(
    Tk_Font tkfont,		/* Font in which characters will be drawn. */
    CONST char *source,		/* UTF-8 string to be displayed.  Need not be
				 * '\0' terminated. */
    int numBytes,		/* Maximum number of bytes to consider
				 * from source string. */
    int maxLength,		/* If >= 0, maxLength specifies the longest
				 * permissible line length in pixels; don't
				 * consider any character that would cross
				 * this x-position.  If < 0, then line length
				 * is unbounded and the flags argument is
				 * ignored. */
    int flags,			/* Various flag bits OR-ed together:
				 * TK_PARTIAL_OK means include the last char
				 * which only partially fit on this line.
				 * TK_WHOLE_WORDS means stop on a word
				 * boundary, if possible.
				 * TK_AT_LEAST_ONE means return at least one
				 * character even if no characters fit. */
    int *lengthPtr)		/* Filled with x-location just after the
				 * terminating character. */
{
    HDC hdc;
    HFONT oldFont;
    WinFont *fontPtr;
    int curX, curByte;
    SubFont *lastSubFontPtr;

    /*
     * According to Microsoft tech support, Windows does not use kerning
     * or fractional character widths when displaying text on the screen.
     * So that means we can safely measure individual characters or spans
     * of characters and add up the widths w/o any "off-by-one-pixel" 
     * errors.  
     */

    fontPtr = (WinFont *) tkfont;

    hdc = GetDC(fontPtr->hwnd);
    lastSubFontPtr = &fontPtr->subFontArray[0];
    oldFont = SelectObject(hdc, lastSubFontPtr->hFont);

    if (numBytes == 0) {
	curX = 0;
	curByte = 0;
    } else if (maxLength < 0) {				 
	Tcl_UniChar ch;
	SIZE size;
	FontFamily *familyPtr;
	Tcl_DString runString;
	SubFont *thisSubFontPtr;
	CONST char *p, *end, *next;

    	/*
    	 * A three step process:
    	 * 1. Find a contiguous range of characters that can all be 
    	 *    represented by a single screen font.
    	 * 2. Convert those chars to the encoding of that font.
	 * 3. Measure converted chars.
    	 */

        curX = 0;
        end = source + numBytes;
        for (p = source; p < end; ) {
            next = p + Tcl_UtfToUniChar(p, &ch);
            thisSubFontPtr = FindSubFontForChar(fontPtr, ch);
            if (thisSubFontPtr != lastSubFontPtr) {
		familyPtr = lastSubFontPtr->familyPtr;
		Tcl_UtfToExternalDString(familyPtr->encoding, source, 
			p - source, &runString);
		(*familyPtr->getTextExtentPoint32Proc)(hdc, 
			Tcl_DStringValue(&runString),
			Tcl_DStringLength(&runString) >> familyPtr->isWideFont, 
			&size);
		curX += size.cx;
		Tcl_DStringFree(&runString);
                lastSubFontPtr = thisSubFontPtr;
                source = p;

		SelectObject(hdc, lastSubFontPtr->hFont);
            }
            p = next;
        }
	familyPtr = lastSubFontPtr->familyPtr;
	Tcl_UtfToExternalDString(familyPtr->encoding, source, p - source, 
		&runString);
	(*familyPtr->getTextExtentPoint32Proc)(hdc,
		Tcl_DStringValue(&runString),
		Tcl_DStringLength(&runString) >> familyPtr->isWideFont, 
		&size);
	curX += size.cx;
	Tcl_DStringFree(&runString);
	curByte = numBytes;
    } else {
	Tcl_UniChar ch;
	SIZE size;
	char buf[16];
	FontFamily *familyPtr;
	SubFont *thisSubFontPtr;
	CONST char *term, *end, *p, *next;
	int newX, termX, sawNonSpace, srcRead, dstWrote;

	/*
	 * How many chars will fit in the space allotted? 
	 * This first version may be inefficient because it measures
	 * every character individually.  There is a function call that
	 * can measure multiple characters at once and return the
	 * offset of each of them, but it only works on NT, even though
	 * the documentation claims it works for 95.
	 * TODO: verify that GetTextExtentExPoint is still broken in '95, and
	 * possibly use it for NT anyway since it should be much faster and
	 * more accurate.
	 */

	next = source + Tcl_UtfToUniChar(source, &ch);
	newX = curX = termX = 0;
	
	term = source;
	end = source + numBytes;

	sawNonSpace = (ch > 255) || !isspace(ch);
	for (p = source; ; ) {
	    if (ch < BASE_CHARS) {
		newX += fontPtr->widths[ch];
	    } else {
		thisSubFontPtr = FindSubFontForChar(fontPtr, ch);
		if (thisSubFontPtr != lastSubFontPtr) {
		    SelectObject(hdc, thisSubFontPtr->hFont);
		    lastSubFontPtr = thisSubFontPtr;
		}
		familyPtr = lastSubFontPtr->familyPtr;
		Tcl_UtfToExternal(NULL, familyPtr->encoding, p, next - p, 
			0, NULL, buf, sizeof(buf), &srcRead, &dstWrote, NULL);
		(*familyPtr->getTextExtentPoint32Proc)(hdc, buf, 
			dstWrote >> familyPtr->isWideFont, &size);
		newX += size.cx;
	    }
	    if (newX > maxLength) {
		break;
	    }
	    curX = newX;
	    p = next;
	    if (p >= end) {
		term = end;
		termX = curX;
		break;
	    }

	    next += Tcl_UtfToUniChar(next, &ch);
	    if ((ch < 256) && isspace(ch)) {
		if (sawNonSpace) {
		    term = p;
		    termX = curX;
		    sawNonSpace = 0;
		}
	    } else {
		sawNonSpace = 1;
	    }
	}

	/*
	 * P points to the first character that doesn't fit in the desired
	 * span.  Use the flags to figure out what to return.
	 */

	if ((flags & TK_PARTIAL_OK) && (p < end) && (curX < maxLength)) {
	    /*
	     * Include the first character that didn't quite fit in the desired
	     * span.  The width returned will include the width of that extra
	     * character.
	     */

	    curX = newX;
	    p += Tcl_UtfToUniChar(p, &ch);
	}
	if ((flags & TK_AT_LEAST_ONE) && (term == source) && (p < end)) {
	    term = p;
	    termX = curX;
	    if (term == source) {
		term += Tcl_UtfToUniChar(term, &ch);
		termX = newX;
	    }
	} else if ((p >= end) || !(flags & TK_WHOLE_WORDS)) {
	    term = p;
	    termX = curX;
	}

	curX = termX;
	curByte = term - source;	
    }

    SelectObject(hdc, oldFont);
    ReleaseDC(fontPtr->hwnd, hdc);

    *lengthPtr = curX;
    return curByte;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_DrawChars --
 *
 *	Draw a string of characters on the screen.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_DrawChars(
    Display *display,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    GC gc,			/* Graphics context for drawing characters. */
    Tk_Font tkfont,		/* Font in which characters will be drawn;
				 * must be the same as font used in GC. */
    CONST char *source,		/* UTF-8 string to be displayed.  Need not be
				 * '\0' terminated.  All Tk meta-characters
				 * (tabs, control characters, and newlines)
				 * should be stripped out of the string that
				 * is passed to this function.  If they are
				 * not stripped out, they will be displayed as
				 * regular printing characters. */
    int numBytes,		/* Number of bytes in string. */
    int x, int y)		/* Coordinates at which to place origin of
				 * string when drawing. */
{
    HDC dc;
    WinFont *fontPtr;
    TkWinDCState state;

    fontPtr = (WinFont *) gc->font;
    display->request++;

    if (drawable == None) {
	return;
    }

    dc = TkWinGetDrawableDC(display, drawable, &state);

    SetROP2(dc, tkpWinRopModes[gc->function]);

    if ((gc->fill_style == FillStippled
	    || gc->fill_style == FillOpaqueStippled)
	    && gc->stipple != None) {
	TkWinDrawable *twdPtr = (TkWinDrawable *)gc->stipple;
	HBRUSH oldBrush, stipple;
	HBITMAP oldBitmap, bitmap;
	HDC dcMem;
	TEXTMETRIC tm;
	SIZE size;

	if (twdPtr->type != TWD_BITMAP) {
	    panic("unexpected drawable type in stipple");
	}

	/*
	 * Select stipple pattern into destination dc.
	 */
	
	dcMem = CreateCompatibleDC(dc);

	stipple = CreatePatternBrush(twdPtr->bitmap.handle);
	SetBrushOrgEx(dc, gc->ts_x_origin, gc->ts_y_origin, NULL);
	oldBrush = SelectObject(dc, stipple);

	SetTextAlign(dcMem, TA_LEFT | TA_BASELINE);
	SetTextColor(dcMem, gc->foreground);
	SetBkMode(dcMem, TRANSPARENT);
	SetBkColor(dcMem, RGB(0, 0, 0));

	/*
	 * Compute the bounding box and create a compatible bitmap.
	 */

	GetTextExtentPoint(dcMem, source, numBytes, &size);
	GetTextMetrics(dcMem, &tm);
	size.cx -= tm.tmOverhang;
	bitmap = CreateCompatibleBitmap(dc, size.cx, size.cy);
	oldBitmap = SelectObject(dcMem, bitmap);

	/*
	 * The following code is tricky because fonts are rendered in multiple
	 * colors.  First we draw onto a black background and copy the white
	 * bits.  Then we draw onto a white background and copy the black bits.
	 * Both the foreground and background bits of the font are ANDed with
	 * the stipple pattern as they are copied.
	 */

	PatBlt(dcMem, 0, 0, size.cx, size.cy, BLACKNESS);
	MultiFontTextOut(dc, fontPtr, source, numBytes, x, y);
	BitBlt(dc, x, y - tm.tmAscent, size.cx, size.cy, dcMem,
		0, 0, 0xEA02E9);
	PatBlt(dcMem, 0, 0, size.cx, size.cy, WHITENESS);
	MultiFontTextOut(dc, fontPtr, source, numBytes, x, y);
	BitBlt(dc, x, y - tm.tmAscent, size.cx, size.cy, dcMem,
		0, 0, 0x8A0E06);

	/*
	 * Destroy the temporary bitmap and restore the device context.
	 */

	SelectObject(dcMem, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(dcMem);
	SelectObject(dc, oldBrush);
	DeleteObject(stipple);
    } else {
	SetTextAlign(dc, TA_LEFT | TA_BASELINE);
	SetTextColor(dc, gc->foreground);
	SetBkMode(dc, TRANSPARENT);
	MultiFontTextOut(dc, fontPtr, source, numBytes, x, y);
    }
    TkWinReleaseDrawableDC(drawable, dc, &state);
}

/*
 *-------------------------------------------------------------------------
 *
 * MultiFontTextOut --
 *
 *	Helper function for Tk_DrawChars.  Draws characters, using the 
 *	various screen fonts in fontPtr to draw multilingual characters.
 *	Note: No bidirectional support.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.  
 *	Contents of fontPtr may be modified if more subfonts were loaded 
 *	in order to draw all the multilingual characters in the given 
 *	string.
 *
 *-------------------------------------------------------------------------
 */

static void
MultiFontTextOut(
    HDC hdc,			/* HDC to draw into. */
    WinFont *fontPtr,		/* Contains set of fonts to use when drawing
				 * following string. */
    CONST char *source,		/* Potentially multilingual UTF-8 string. */
    int numBytes,		/* Length of string in bytes. */
    int x, int y)		/* Coordinates at which to place origin *
				 * of string when drawing. */
{
    Tcl_UniChar ch;
    SIZE size;
    HFONT oldFont;
    FontFamily *familyPtr;
    Tcl_DString runString;
    CONST char *p, *end, *next;
    SubFont *lastSubFontPtr, *thisSubFontPtr;

    lastSubFontPtr = &fontPtr->subFontArray[0];
    oldFont = SelectObject(hdc, lastSubFontPtr->hFont);

    end = source + numBytes;
    for (p = source; p < end; ) {
        next = p + Tcl_UtfToUniChar(p, &ch);
        thisSubFontPtr = FindSubFontForChar(fontPtr, ch);
        if (thisSubFontPtr != lastSubFontPtr) {
            if (p > source) {
		familyPtr = lastSubFontPtr->familyPtr;
 		Tcl_UtfToExternalDString(familyPtr->encoding, source,
			p - source, &runString);
		(*familyPtr->textOutProc)(hdc, x, y, 
			Tcl_DStringValue(&runString),
			Tcl_DStringLength(&runString) >> familyPtr->isWideFont);
		(*familyPtr->getTextExtentPoint32Proc)(hdc, 
			Tcl_DStringValue(&runString),
			Tcl_DStringLength(&runString) >> familyPtr->isWideFont, 
			&size);
		x += size.cx;
		Tcl_DStringFree(&runString);
	    }
            lastSubFontPtr = thisSubFontPtr;
            source = p;
	    SelectObject(hdc, lastSubFontPtr->hFont);
	}
	p = next;
    }
    if (p > source) {
	familyPtr = lastSubFontPtr->familyPtr;
 	Tcl_UtfToExternalDString(familyPtr->encoding, source, p - source,
		&runString);
	(*familyPtr->textOutProc)(hdc, x, y, Tcl_DStringValue(&runString),
		Tcl_DStringLength(&runString) >> familyPtr->isWideFont);
	Tcl_DStringFree(&runString);
    }
    SelectObject(hdc, oldFont);
}

/*
 *---------------------------------------------------------------------------
 *
 * InitFont --
 *
 *	Helper for TkpGetNativeFont() and TkpGetFontFromAttributes().
 *	Initializes the memory for a new WinFont that wraps the 
 *	platform-specific data.
 *
 *	The caller is responsible for initializing the fields of the
 *	WinFont that are used exclusively by the generic TkFont code, and
 *	for releasing those fields before calling TkpDeleteFont().
 *
 * Results:
 *	Fills the WinFont structure.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */ 

static void
InitFont(
    Tk_Window tkwin,		/* Main window of interp in which font will 
				 * be used, for getting HDC. */
    HFONT hFont,		/* Windows token for font. */
    int overstrike,		/* The overstrike attribute of logfont used
				 * to allocate this font.  For some reason,
				 * the TEXTMETRICs may contain incorrect info
				 * in the tmStruckOut field. */
    WinFont *fontPtr)		/* Filled with information constructed from
				 * the above arguments. */
{
    HDC hdc;
    HWND hwnd;
    HFONT oldFont;
    TEXTMETRIC tm;
    Window window;
    TkFontMetrics *fmPtr;
    Tcl_Encoding encoding;
    Tcl_DString faceString;
    TkFontAttributes *faPtr;
    char buf[LF_FACESIZE * sizeof(WCHAR)];
 
    window  = Tk_WindowId(tkwin);
    hwnd    = (window == None) ? NULL : TkWinGetHWND(window);
    hdc	    = GetDC(hwnd);
    oldFont = SelectObject(hdc, hFont);

    GetTextMetrics(hdc, &tm);

    /*
     * On any version NT, there may fonts with international names.  
     * Use the NT-only Unicode version of GetTextFace to get the font's 
     * name.  If we used the ANSI version on a non-internationalized 
     * version of NT, we would get a font name with '?' replacing all 
     * the international characters.
     *
     * On a non-internationalized verson of 95, fonts with international
     * names are not allowed, so the ANSI version of GetTextFace will work.
     * On an internationalized version of 95, there may be fonts with 
     * international names; the ANSI version will work, fetching the 
     * name in the international system code page.  Can't use the Unicode 
     * version of GetTextFace because it only exists under NT.
     */

    if (platformId == VER_PLATFORM_WIN32_NT) {
	GetTextFaceW(hdc, LF_FACESIZE, (WCHAR *) buf);
    } else {
	GetTextFaceA(hdc, LF_FACESIZE, (char *) buf);
    }
    Tcl_ExternalToUtfDString(systemEncoding, buf, -1, &faceString);

    fontPtr->font.fid	= (Font) fontPtr;

    faPtr		= &fontPtr->font.fa;
    faPtr->family	= Tk_GetUid(Tcl_DStringValue(&faceString));
    faPtr->size		= TkFontGetPoints(tkwin, -(tm.tmHeight - tm.tmInternalLeading));
    faPtr->weight	= (tm.tmWeight > FW_MEDIUM) ? TK_FW_BOLD : TK_FW_NORMAL;
    faPtr->slant	= (tm.tmItalic != 0) ? TK_FS_ITALIC : TK_FS_ROMAN;
    faPtr->underline	= (tm.tmUnderlined != 0) ? 1 : 0;
    faPtr->overstrike	= overstrike;
    
    fmPtr		= &fontPtr->font.fm;
    fmPtr->ascent	= tm.tmAscent;
    fmPtr->descent	= tm.tmDescent;
    fmPtr->maxWidth	= tm.tmMaxCharWidth;
    fmPtr->fixed	= !(tm.tmPitchAndFamily & TMPF_FIXED_PITCH);

    fontPtr->hwnd	= hwnd;
    fontPtr->pixelSize	= tm.tmHeight - tm.tmInternalLeading;

    fontPtr->numSubFonts 	= 1;
    fontPtr->subFontArray	= fontPtr->staticSubFonts;
    InitSubFont(hdc, hFont, 1, &fontPtr->subFontArray[0]);

    encoding = fontPtr->subFontArray[0].familyPtr->encoding;
    if (encoding == unicodeEncoding) {
	GetCharWidthW(hdc, 0, BASE_CHARS - 1, fontPtr->widths);
    } else {
	GetCharWidthA(hdc, 0, BASE_CHARS - 1, fontPtr->widths);
    } 
    Tcl_DStringFree(&faceString);

    SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);
}

/*
 *-------------------------------------------------------------------------
 *
 * ReleaseFont --
 * 
 *	Called to release the windows-specific contents of a TkFont.
 *	The caller is responsible for freeing the memory used by the
 *	font itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
ReleaseFont(
    WinFont *fontPtr)		/* The font to delete. */
{
    int i;

    for (i = 0; i < fontPtr->numSubFonts; i++) {
	ReleaseSubFont(&fontPtr->subFontArray[i]);
    }
    if (fontPtr->subFontArray != fontPtr->staticSubFonts) {
	ckfree((char *) fontPtr->subFontArray);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * InitSubFont --
 *
 *	Wrap a screen font and load the FontFamily that represents
 *	it.  Used to prepare a SubFont so that characters can be mapped
 *	from UTF-8 to the charset of the font.
 *
 * Results:
 *	The subFontPtr is filled with information about the font.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static void
InitSubFont(
    HDC hdc,			/* HDC in which font can be selected. */
    HFONT hFont,		/* The screen font. */
    int base,			/* Non-zero if this SubFont is being used
				 * as the base font for a font object. */
    SubFont *subFontPtr)	/* Filled with SubFont constructed from 
    				 * above attributes. */
{
    subFontPtr->hFont	    = hFont;
    subFontPtr->familyPtr   = AllocFontFamily(hdc, hFont, base);
    subFontPtr->fontMap	    = subFontPtr->familyPtr->fontMap;
}

/*
 *-------------------------------------------------------------------------
 *
 * ReleaseSubFont --
 *
 *	Called to release the contents of a SubFont.  The caller is 
 *	responsible for freeing the memory used by the SubFont itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and resources are freed.
 *
 *---------------------------------------------------------------------------
 */

static void
ReleaseSubFont(
    SubFont *subFontPtr)	/* The SubFont to delete. */
{
    DeleteObject(subFontPtr->hFont);
    FreeFontFamily(subFontPtr->familyPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * AllocFontFamily --
 *
 *	Find the FontFamily structure associated with the given font
 *	name.  The information should be stored by the caller in a 
 *	SubFont and used when determining if that SubFont supports a 
 *	character.
 *
 *	Cannot use the string name used to construct the font as the 
 *	key, because the capitalization may not be canonical.  Therefore
 *	use the face name actually retrieved from the font metrics as
 *	the key.
 *
 * Results:
 *	A pointer to a FontFamily.  The reference count in the FontFamily
 *	is automatically incremented.  When the SubFont is released, the
 *	reference count is decremented.  When no SubFont is using this
 *	FontFamily, it may be deleted.
 *
 * Side effects:
 *	A new FontFamily structure will be allocated if this font family
 *	has not been seen.  TrueType character existence metrics are
 *	loaded into the FontFamily structure.
 *
 *-------------------------------------------------------------------------
 */

static FontFamily *
AllocFontFamily(
    HDC hdc,			/* HDC in which font can be selected. */
    HFONT hFont,		/* Screen font whose FontFamily is to be
				 * returned. */
    int base)			/* Non-zero if this font family is to be
				 * used in the base font of a font object. */
{
    Tk_Uid faceName;
    FontFamily *familyPtr;
    Tcl_DString faceString;
    Tcl_Encoding encoding;
    char buf[LF_FACESIZE * sizeof(WCHAR)];
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    hFont = SelectObject(hdc, hFont);
    if (platformId == VER_PLATFORM_WIN32_NT) {
	GetTextFaceW(hdc, LF_FACESIZE, (WCHAR *) buf);
    } else {
	GetTextFaceA(hdc, LF_FACESIZE, (char *) buf);
    }
    Tcl_ExternalToUtfDString(systemEncoding, buf, -1, &faceString);
    faceName = Tk_GetUid(Tcl_DStringValue(&faceString));
    Tcl_DStringFree(&faceString);
    hFont = SelectObject(hdc, hFont);

    familyPtr = tsdPtr->fontFamilyList; 
    for ( ; familyPtr != NULL; familyPtr = familyPtr->nextPtr) {
	if (familyPtr->faceName == faceName) {
	    familyPtr->refCount++;
	    return familyPtr;
	}
    }

    familyPtr = (FontFamily *) ckalloc(sizeof(FontFamily));
    memset(familyPtr, 0, sizeof(FontFamily));
    familyPtr->nextPtr = tsdPtr->fontFamilyList;
    tsdPtr->fontFamilyList = familyPtr;

    /* 
     * Set key for this FontFamily. 
     */

    familyPtr->faceName = faceName;

    /* 
     * An initial refCount of 2 means that FontFamily information will
     * persist even when the SubFont that loaded the FontFamily is released.
     * Change it to 1 to cause FontFamilies to be unloaded when not in use.
     */

    familyPtr->refCount = 2;

    familyPtr->segCount = LoadFontRanges(hdc, hFont, &familyPtr->startCount, 
	    &familyPtr->endCount, &familyPtr->isSymbolFont);

    encoding = NULL;
    if (familyPtr->isSymbolFont != 0) {
	/*
	 * Symbol fonts are handled specially.  For instance, Unicode 0393
	 * (GREEK CAPITAL GAMMA) must be mapped to Symbol character 0047
	 * (GREEK CAPITAL GAMMA), because the Symbol font doesn't have a
	 * GREEK CAPITAL GAMMA at location 0393.  If Tk interpreted the
	 * Symbol font using the Unicode encoding, it would decide that
	 * the Symbol font has no GREEK CAPITAL GAMMA, because the Symbol
	 * encoding (of course) reports that character 0393 doesn't exist.  
	 *
	 * With non-symbol Windows fonts, such as Times New Roman, if the
	 * font has a GREEK CAPITAL GAMMA, it will be found in the correct
	 * Unicode location (0393); the GREEK CAPITAL GAMMA will not be off
	 * hiding at some other location.
	 */

	encoding = Tcl_GetEncoding(NULL, faceName);
    }

    if (encoding == NULL) {
	encoding = Tcl_GetEncoding(NULL, "unicode");
	familyPtr->textOutProc =
	    (BOOL (WINAPI *)(HDC, int, int, TCHAR *, int)) TextOutW;
	familyPtr->getTextExtentPoint32Proc = 
	    (BOOL (WINAPI *)(HDC, TCHAR *, int, LPSIZE)) GetTextExtentPoint32W;
	familyPtr->isWideFont = 1;
    } else {
	familyPtr->textOutProc = 
	    (BOOL (WINAPI *)(HDC, int, int, TCHAR *, int)) TextOutA;
	familyPtr->getTextExtentPoint32Proc = 
	    (BOOL (WINAPI *)(HDC, TCHAR *, int, LPSIZE)) GetTextExtentPoint32A;
	familyPtr->isWideFont = 0;
    } 

    familyPtr->encoding = encoding;

    return familyPtr;
}

/*
 *-------------------------------------------------------------------------
 *
 * FreeFontFamily --
 *
 *	Called to free a FontFamily when the SubFont is finished using it.
 *	Frees the contents of the FontFamily and the memory used by the
 *	FontFamily itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
 
static void
FreeFontFamily(
    FontFamily *familyPtr)	/* The FontFamily to delete. */
{
    int i;
    FontFamily **familyPtrPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (familyPtr == NULL) {
        return;
    }
    familyPtr->refCount--;
    if (familyPtr->refCount > 0) {
    	return;
    }
    for (i = 0; i < FONTMAP_PAGES; i++) {
        if (familyPtr->fontMap[i] != NULL) {
            ckfree(familyPtr->fontMap[i]);
        }
    }
    if (familyPtr->startCount != NULL) {
	ckfree((char *) familyPtr->startCount);
    }
    if (familyPtr->endCount != NULL) {
	ckfree((char *) familyPtr->endCount);
    }
    if (familyPtr->encoding != unicodeEncoding) {
	Tcl_FreeEncoding(familyPtr->encoding);
    }
    
    /* 
     * Delete from list. 
     */
         
    for (familyPtrPtr = &tsdPtr->fontFamilyList; ; ) {
        if (*familyPtrPtr == familyPtr) {
  	    *familyPtrPtr = familyPtr->nextPtr;
	    break;
	}
	familyPtrPtr = &(*familyPtrPtr)->nextPtr;
    }
    
    ckfree((char *) familyPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * FindSubFontForChar --
 *
 *	Determine which screen font is necessary to use to display the 
 *	given character.  If the font object does not have a screen font 
 *	that can display the character, another screen font may be loaded 
 *	into the font object, following a set of preferred fallback rules.
 *
 * Results:
 *	The return value is the SubFont to use to display the given 
 *	character. 
 *
 * Side effects:
 *	The contents of fontPtr are modified to cache the results
 *	of the lookup and remember any SubFonts that were dynamically 
 *	loaded.
 *
 *-------------------------------------------------------------------------
 */

static SubFont *
FindSubFontForChar(
    WinFont *fontPtr,		/* The font object with which the character
				 * will be displayed. */
    int ch)			/* The Unicode character to be displayed. */
{
    HDC hdc;
    int i, j, k;
    CanUse canUse;
    char **aliases, **anyFallbacks;
    char ***fontFallbacks;
    char *fallbackName;
    SubFont *subFontPtr;
    Tcl_DString ds;
    
    if (ch < BASE_CHARS) {
	return &fontPtr->subFontArray[0];
    }

    for (i = 0; i < fontPtr->numSubFonts; i++) {
	if (FontMapLookup(&fontPtr->subFontArray[i], ch)) {
	    return &fontPtr->subFontArray[i];
	}
    }

    /*
     * Keep track of all face names that we check, so we don't check some
     * name multiple times if it can be reached by multiple paths.
     */
     
    Tcl_DStringInit(&ds);
    hdc = GetDC(fontPtr->hwnd);
        
    aliases = TkFontGetAliasList(fontPtr->font.fa.family);

    fontFallbacks = TkFontGetFallbacks();
    for (i = 0; fontFallbacks[i] != NULL; i++) {
	for (j = 0; fontFallbacks[i][j] != NULL; j++) {
	    fallbackName = fontFallbacks[i][j];
	    if (strcasecmp(fallbackName, fontPtr->font.fa.family) == 0) {
		/*
		 * If the base font has a fallback...
		 */

		goto tryfallbacks;
	    } else if (aliases != NULL) {
		/* 
		 * Or if an alias for the base font has a fallback...
		 */

		for (k = 0; aliases[k] != NULL; k++) {
		    if (strcasecmp(aliases[k], fallbackName) == 0) {
		        goto tryfallbacks;
		    }
		}
	    }
	}
	continue;

	/* 
	 * ...then see if we can use one of the fallbacks, or an
	 * alias for one of the fallbacks.
	 */

	tryfallbacks:
	for (j = 0; fontFallbacks[i][j] != NULL; j++) {
	    fallbackName = fontFallbacks[i][j];
	    subFontPtr = CanUseFallbackWithAliases(hdc, fontPtr, fallbackName,
		    ch, &ds);
	    if (subFontPtr != NULL) {
		goto end;
	    }
	}
    }

    /*
     * See if we can use something from the global fallback list. 
     */

    anyFallbacks = TkFontGetGlobalClass();
    for (i = 0; anyFallbacks[i] != NULL; i++) {
	fallbackName = anyFallbacks[i];
	subFontPtr = CanUseFallbackWithAliases(hdc, fontPtr, fallbackName, 
		ch, &ds);
	if (subFontPtr != NULL) {
	    goto end;
	}
    }

    /*
     * Try all face names available in the whole system until we
     * find one that can be used.
     */

    canUse.hdc = hdc;
    canUse.fontPtr = fontPtr;
    canUse.nameTriedPtr = &ds;
    canUse.ch = ch;
    canUse.subFontPtr = NULL;
    if (platformId == VER_PLATFORM_WIN32_NT) {
	EnumFontFamiliesW(hdc, NULL, (FONTENUMPROCW) WinFontCanUseProc,
		(LPARAM) &canUse);
    } else {
	EnumFontFamiliesA(hdc, NULL, (FONTENUMPROCA) WinFontCanUseProc,
		(LPARAM) &canUse);
    }
    subFontPtr = canUse.subFontPtr;

    end:
    Tcl_DStringFree(&ds);
    
    if (subFontPtr == NULL) {
        /* 
         * No font can display this character.  We will use the base font
         * and have it display the "unknown" character.
         */

	subFontPtr = &fontPtr->subFontArray[0];
        FontMapInsert(subFontPtr, ch);
    }
    ReleaseDC(fontPtr->hwnd, hdc);
    return subFontPtr;
}

static int CALLBACK
WinFontCanUseProc(
    ENUMLOGFONT *lfPtr,		/* Logical-font data. */
    NEWTEXTMETRIC *tmPtr,	/* Physical-font data (not used). */
    int fontType,		/* Type of font (not used). */
    LPARAM lParam)		/* Result object to hold result. */
{
    int ch;
    HDC hdc;
    WinFont *fontPtr;
    CanUse *canUsePtr;
    char *fallbackName;
    SubFont *subFontPtr;
    Tcl_DString faceString;
    Tcl_DString *nameTriedPtr;

    canUsePtr	    = (CanUse *) lParam;
    ch		    = canUsePtr->ch;
    hdc		    = canUsePtr->hdc;
    fontPtr	    = canUsePtr->fontPtr;
    nameTriedPtr    = canUsePtr->nameTriedPtr;

    fallbackName = lfPtr->elfLogFont.lfFaceName;
    Tcl_ExternalToUtfDString(systemEncoding, fallbackName, -1, &faceString);
    fallbackName = Tcl_DStringValue(&faceString);

    if (SeenName(fallbackName, nameTriedPtr) == 0) {
	subFontPtr = CanUseFallback(hdc, fontPtr, fallbackName, ch);
	if (subFontPtr != NULL) {
	    canUsePtr->subFontPtr = subFontPtr;
	    Tcl_DStringFree(&faceString);
	    return 0;
	}
    }
    Tcl_DStringFree(&faceString);
    return 1;
}

/*
 *-------------------------------------------------------------------------
 *
 * FontMapLookup --
 *
 *	See if the screen font can display the given character.
 *
 * Results:
 *	The return value is 0 if the screen font cannot display the
 *	character, non-zero otherwise.
 *
 * Side effects:
 *	New pages are added to the font mapping cache whenever the
 *	character belongs to a page that hasn't been seen before.
 *	When a page is loaded, information about all the characters on
 *	that page is stored, not just for the single character in
 *	question.
 *
 *-------------------------------------------------------------------------
 */

static int
FontMapLookup(
    SubFont *subFontPtr,	/* Contains font mapping cache to be queried
				 * and possibly updated. */
    int ch)			/* Character to be tested. */
{
    int row, bitOffset;

    row = ch >> FONTMAP_SHIFT;
    if (subFontPtr->fontMap[row] == NULL) {
	FontMapLoadPage(subFontPtr, row);
    }
    bitOffset = ch & (FONTMAP_BITSPERPAGE - 1);
    return (subFontPtr->fontMap[row][bitOffset >> 3] >> (bitOffset & 7)) & 1;
}

/*
 *-------------------------------------------------------------------------
 *
 * FontMapInsert --
 *
 *	Tell the font mapping cache that the given screen font should be
 *	used to display the specified character.  This is called when no
 *	font on the system can be be found that can display that 
 *	character; we lie to the font and tell it that it can display
 *	the character, otherwise we would end up re-searching the entire
 *	fallback hierarchy every time that character was seen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New pages are added to the font mapping cache whenever the
 *	character belongs to a page that hasn't been seen before.
 *	When a page is loaded, information about all the characters on
 *	that page is stored, not just for the single character in
 *	question.
 *
 *-------------------------------------------------------------------------
 */

static void
FontMapInsert(
    SubFont *subFontPtr,	/* Contains font mapping cache to be 
				 * updated. */
    int ch)			/* Character to be added to cache. */
{
    int row, bitOffset;

    row = ch >> FONTMAP_SHIFT;
    if (subFontPtr->fontMap[row] == NULL) {
	FontMapLoadPage(subFontPtr, row);
    }
    bitOffset = ch & (FONTMAP_BITSPERPAGE - 1);
    subFontPtr->fontMap[row][bitOffset >> 3] |= 1 << (bitOffset & 7);
}

/*
 *-------------------------------------------------------------------------
 *
 * FontMapLoadPage --
 *
 *	Load information about all the characters on a given page.
 *	This information consists of one bit per character that indicates
 *	whether the associated HFONT can (1) or cannot (0) display the
 *	characters on the page.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Mempry allocated.
 *
 *-------------------------------------------------------------------------
 */
static void 
FontMapLoadPage(
    SubFont *subFontPtr,	/* Contains font mapping cache to be 
				 * updated. */
    int row)			/* Index of the page to be loaded into 
				 * the cache. */
{
    FontFamily *familyPtr;
    Tcl_Encoding encoding;
    char src[TCL_UTF_MAX], buf[16];
    USHORT *startCount, *endCount;
    int i, j, bitOffset, end, segCount;

    subFontPtr->fontMap[row] = (char *) ckalloc(FONTMAP_BITSPERPAGE / 8);
    memset(subFontPtr->fontMap[row], 0, FONTMAP_BITSPERPAGE / 8);

    familyPtr = subFontPtr->familyPtr;
    encoding = familyPtr->encoding;

    if (familyPtr->encoding == unicodeEncoding) {
	/*
	 * Font is Unicode.  Few fonts are going to have all characters, so 
	 * examine the TrueType character existence metrics to determine 
	 * what characters actually exist in this font.
	 */

	segCount    = familyPtr->segCount;
	startCount  = familyPtr->startCount;
	endCount    = familyPtr->endCount;

	j = 0;
	end = (row + 1) << FONTMAP_SHIFT;
	for (i = row << FONTMAP_SHIFT; i < end; i++) {
	    for ( ; j < segCount; j++) {
		if (endCount[j] >= i) {
		    if (startCount[j] <= i) {
			bitOffset = i & (FONTMAP_BITSPERPAGE - 1);
			subFontPtr->fontMap[row][bitOffset >> 3] |= 1 << (bitOffset & 7);
		    }
		    break;
		}
	    }
	}
    } else if (familyPtr->isSymbolFont) {
	/*
	 * Assume that a symbol font with a known encoding has all the 
	 * characters that its encoding claims it supports.  
	 *	 
	 * The test for "encoding == unicodeEncoding"
	 * must occur before this case, to catch all symbol fonts (such 
	 * as {Comic Sans MS} or Wingdings) for which we don't have 
	 * encoding information; those symbol fonts are treated as if
	 * they were in the Unicode encoding and their symbolic
	 * character existence metrics are treated as if they were Unicode
	 * character existence metrics.  This way, although we don't know
	 * the proper Unicode -> symbol font mapping, we can install the
	 * symbol font as the base font and access its glyphs.
	 */

        end = (row + 1) << FONTMAP_SHIFT;
	for (i = row << FONTMAP_SHIFT; i < end; i++) {
	    if (Tcl_UtfToExternal(NULL, encoding, src, 
		    Tcl_UniCharToUtf(i, src), TCL_ENCODING_STOPONERROR, NULL, 
		    buf, sizeof(buf), NULL, NULL, NULL) != TCL_OK) {
		continue;
	    }
	    bitOffset = i & (FONTMAP_BITSPERPAGE - 1);
	    subFontPtr->fontMap[row][bitOffset >> 3] |= 1 << (bitOffset & 7);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CanUseFallbackWithAliases --
 *
 *	Helper function for FindSubFontForChar.  Determine if the
 *	specified face name (or an alias of the specified face name)
 *	can be used to construct a screen font that can display the
 *	given character.
 *
 * Results:
 *	See CanUseFallback().
 *
 * Side effects:
 *	If the name and/or one of its aliases was rejected, the
 *	rejected string is recorded in nameTriedPtr so that it won't
 *	be tried again.
 *
 *---------------------------------------------------------------------------
 */

static SubFont *
CanUseFallbackWithAliases(
    HDC hdc,			/* HDC in which font can be selected. */
    WinFont *fontPtr,		/* The font object that will own the new
				 * screen font. */
    char *faceName,		/* Desired face name for new screen font. */
    int ch,			/* The Unicode character that the new
				 * screen font must be able to display. */
    Tcl_DString *nameTriedPtr)	/* Records face names that have already
				 * been tried.  It is possible for the same
				 * face name to be queried multiple times when
				 * trying to find a suitable screen font. */
{
    int i;
    char **aliases;
    SubFont *subFontPtr;
    
    if (SeenName(faceName, nameTriedPtr) == 0) {
	subFontPtr = CanUseFallback(hdc, fontPtr, faceName, ch);
	if (subFontPtr != NULL) {
	    return subFontPtr;
	}
    }
    aliases = TkFontGetAliasList(faceName);
    if (aliases != NULL) {
	for (i = 0; aliases[i] != NULL; i++) {
	    if (SeenName(aliases[i], nameTriedPtr) == 0) {
		subFontPtr = CanUseFallback(hdc, fontPtr, aliases[i], ch);
		if (subFontPtr != NULL) {
		    return subFontPtr;
		}
	    }
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * SeenName --
 *
 *	Used to determine we have already tried and rejected the given
 *	face name when looking for a screen font that can support some
 *	Unicode character.
 *
 * Results:
 *	The return value is 0 if this face name has not already been seen,
 *	non-zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
SeenName(
    CONST char *name,		/* The name to check. */
    Tcl_DString *dsPtr)		/* Contains names that have already been
				 * seen. */
{
    CONST char *seen, *end;

    seen = Tcl_DStringValue(dsPtr);
    end = seen + Tcl_DStringLength(dsPtr);
    while (seen < end) {
	if (strcasecmp(seen, name) == 0) {
	    return 1;
	}
	seen += strlen(seen) + 1;
    }
    Tcl_DStringAppend(dsPtr, (char *) name, (int) (strlen(name) + 1));
    return 0;
}

/*
 *-------------------------------------------------------------------------
 *
 * CanUseFallback --
 *
 *	If the specified screen font has not already been loaded into 
 *	the font object, determine if it can display the given character.
 *
 * Results:
 *	The return value is a pointer to a newly allocated SubFont, owned
 *	by the font object.  This SubFont can be used to display the given
 *	character.  The SubFont represents the screen font with the base set 
 *	of font attributes from the font object, but using the specified 
 *	font name.  NULL is returned if the font object already holds
 *	a reference to the specified physical font or if the specified 
 *	physical font cannot display the given character.
 *
 * Side effects:				       
 *	The font object's subFontArray is updated to contain a reference
 *	to the newly allocated SubFont.
 *
 *-------------------------------------------------------------------------
 */

static SubFont *
CanUseFallback(
    HDC hdc,			/* HDC in which font can be selected. */
    WinFont *fontPtr,		/* The font object that will own the new
				 * screen font. */
    char *faceName,		/* Desired face name for new screen font. */
    int ch)			/* The Unicode character that the new
				 * screen font must be able to display. */
{
    int i;
    HFONT hFont;
    SubFont subFont;

    if (FamilyExists(hdc, faceName) == 0) {
	return NULL;
    }

    /* 
     * Skip all fonts we've already used.
     */
     
    for (i = 0; i < fontPtr->numSubFonts; i++) {
	if (faceName == fontPtr->subFontArray[i].familyPtr->faceName) {
	    return NULL;
	}
    }

    /*
     * Load this font and see if it has the desired character.
     */

    hFont = GetScreenFont(&fontPtr->font.fa, faceName, fontPtr->pixelSize);
    InitSubFont(hdc, hFont, 0, &subFont);
    if (((ch < 256) && (subFont.familyPtr->isSymbolFont)) 
	    || (FontMapLookup(&subFont, ch) == 0)) {
	/*
	 * Don't use a symbol font as a fallback font for characters below
	 * 256.
	 */

	ReleaseSubFont(&subFont);
	return NULL;
    }

    if (fontPtr->numSubFonts >= SUBFONT_SPACE) {
	SubFont *newPtr;
    	
    	newPtr = (SubFont *) ckalloc(sizeof(SubFont) 
		* (fontPtr->numSubFonts + 1));
	memcpy((char *) newPtr, fontPtr->subFontArray,
		fontPtr->numSubFonts * sizeof(SubFont));
	if (fontPtr->subFontArray != fontPtr->staticSubFonts) {
	    ckfree((char *) fontPtr->subFontArray);
	}
	fontPtr->subFontArray = newPtr;
    }
    fontPtr->subFontArray[fontPtr->numSubFonts] = subFont;
    fontPtr->numSubFonts++;
    return &fontPtr->subFontArray[fontPtr->numSubFonts - 1];
}

/*
 *---------------------------------------------------------------------------
 *
 * GetScreenFont --
 *
 *	Given the name and other attributes, construct an HFONT.
 *	This is where all the alias and fallback substitution bottoms
 *	out.
 *
 * Results:
 *	The screen font that corresponds to the attributes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static HFONT 
GetScreenFont(
    CONST TkFontAttributes *faPtr,
				/* Desired font attributes for new HFONT. */
    CONST char *faceName,	/* Overrides font family specified in font
				 * attributes. */
    int pixelSize)		/* Overrides size specified in font 
				 * attributes. */
{
    Tcl_DString ds;
    HFONT hFont;
    LOGFONTW lf;

    lf.lfHeight		= -pixelSize;
    lf.lfWidth		= 0;
    lf.lfEscapement	= 0;
    lf.lfOrientation	= 0;
    lf.lfWeight		= (faPtr->weight == TK_FW_NORMAL) ? FW_NORMAL : FW_BOLD;
    lf.lfItalic		= faPtr->slant;
    lf.lfUnderline	= faPtr->underline;
    lf.lfStrikeOut	= faPtr->overstrike;
    lf.lfCharSet	= DEFAULT_CHARSET;
    lf.lfOutPrecision	= OUT_TT_PRECIS;
    lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
    lf.lfQuality	= DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    Tcl_UtfToExternalDString(systemEncoding, faceName, -1, &ds);

    if (platformId == VER_PLATFORM_WIN32_NT) {    
	Tcl_UniChar *src, *dst;
	src = (Tcl_UniChar *) Tcl_DStringValue(&ds);
	dst = (Tcl_UniChar *) lf.lfFaceName;
	while (*src != '\0') {
	    *dst++ = *src++;
	}
	*dst = '\0';
	hFont = CreateFontIndirectW(&lf);
    } else {
	strcpy((char *) lf.lfFaceName, Tcl_DStringValue(&ds));
	hFont = CreateFontIndirectA((LOGFONTA *) &lf);
    }
    Tcl_DStringFree(&ds);
    return hFont;
}

/*
 *-------------------------------------------------------------------------
 *
 * FamilyExists, FamilyOrAliasExists, WinFontExistsProc --
 *
 *	Determines if any physical screen font exists on the system with 
 *	the given family name.  If the family exists, then it should be
 *	possible to construct some physical screen font with that family
 *	name.
 *
 * Results:
 *	The return value is 0 if the specified font family does not exist,
 *	non-zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
FamilyExists(
    HDC hdc,			/* HDC in which font family will be used. */
    CONST char *faceName)	/* Font family to query. */
{
    int result;
    Tcl_DString faceString;

    /*
     * Just immediately rule out the following fonts, because they look so
     * ugly on windows.  The caller's fallback mechanism will cause the
     * corresponding appropriate TrueType fonts to be selected.
     */

    if (strcasecmp(faceName, "Courier") == 0) {
	return 0;
    }
    if (strcasecmp(faceName, "Times") == 0) {
	return 0;
    }
    if (strcasecmp(faceName, "Helvetica") == 0) {
	return 0;
    }
    
    Tcl_UtfToExternalDString(systemEncoding, faceName, -1, &faceString);

    /*
     * If the family exists, WinFontExistProc() will be called and 
     * EnumFontFamilies() will return whatever WinFontExistProc() returns.  
     * If the family doesn't exist, EnumFontFamilies() will just return a 
     * non-zero value.
     */

    if (platformId == VER_PLATFORM_WIN32_NT) {
	result = EnumFontFamiliesW(hdc, (WCHAR *) Tcl_DStringValue(&faceString),
		(FONTENUMPROCW) WinFontExistProc, 0);
    } else {
	result = EnumFontFamiliesA(hdc, (char *) Tcl_DStringValue(&faceString),
		(FONTENUMPROCA) WinFontExistProc, 0);
    }
    Tcl_DStringFree(&faceString);
    return (result == 0);
}

static char *
FamilyOrAliasExists(
    HDC hdc, 
    CONST char *faceName)
{
    char **aliases;
    int i;

    if (FamilyExists(hdc, faceName) != 0) {
	return (char *) faceName;
    }
    aliases = TkFontGetAliasList(faceName);
    if (aliases != NULL) {
	for (i = 0; aliases[i] != NULL; i++) {
	    if (FamilyExists(hdc, aliases[i]) != 0) {
		return aliases[i];
	    }
	}
    }
    return NULL;
}

static int CALLBACK
WinFontExistProc(
    ENUMLOGFONT *lfPtr,		/* Logical-font data. */
    NEWTEXTMETRIC *tmPtr,	/* Physical-font data (not used). */
    int fontType,		/* Type of font (not used). */
    LPARAM lParam)		/* EnumFontData to hold result. */
{
    return 0;
}

/*
 * The following data structures are used when querying a TrueType font file
 * to determine which characters the font supports.
 */

#pragma pack(1)			/* Structures are byte aligned in file. */

#define CMAPHEX  0x636d6170	/* Key for character map resource. */

typedef struct CMAPTABLE {
    USHORT version;		/* Table version number (0). */
    USHORT numTables;		/* Number of encoding tables following. */
} CMAPTABLE;

typedef struct ENCODINGTABLE {
    USHORT platform;		/* Platform for which data is targeted.  
				 * 3 means data is for Windows. */
    USHORT encoding;		/* How characters in font are encoded.  
				 * 1 means that the following subtable is 
				 * keyed based on Unicode. */
    ULONG offset;		/* Byte offset from beginning of CMAPTABLE 
				 * to the subtable for this encoding. */
} ENCODINGTABLE;

typedef struct ANYTABLE {
    USHORT format;		/* Format number. */
    USHORT length;		/* The actual length in bytes of this 
				 * subtable. */
    USHORT version;		/* Version number (starts at 0). */
} ANYTABLE;

typedef struct BYTETABLE {
    USHORT format;		/* Format number is set to 0. */
    USHORT length;		/* The actual length in bytes of this 
				 * subtable. */
    USHORT version;		/* Version number (starts at 0). */
    BYTE glyphIdArray[256];	/* Array that maps up to 256 single-byte char
				 * codes to glyph indices. */
} BYTETABLE;

typedef struct SUBHEADER {
    USHORT firstCode;		/* First valid low byte for subHeader. */
    USHORT entryCount;		/* Number valid low bytes for subHeader. */
    SHORT idDelta;		/* Constant adder to get base glyph index. */
    USHORT idRangeOffset;	/* Byte offset from here to appropriate 
				 * glyphIndexArray. */
} SUBHEADER;

typedef struct HIBYTETABLE {
    USHORT format;  		/* Format number is set to 2. */
    USHORT length;		/* The actual length in bytes of this
				 * subtable. */
    USHORT version;		/* Version number (starts at 0). */
    USHORT subHeaderKeys[256];	/* Maps high bytes to subHeaders: value is 
				 * subHeader index * 8. */
#if 0
    SUBHEADER subHeaders[];	/* Variable-length array of SUBHEADERs. */
    USHORT glyphIndexArray[];	/* Variable-length array containing subarrays 
				 * used for mapping the low byte of 2-byte 
				 * characters. */
#endif
} HIBYTETABLE;

typedef struct SEGMENTTABLE {
    USHORT format;		/* Format number is set to 4. */
    USHORT length;		/* The actual length in bytes of this
				 * subtable. */
    USHORT version;		/* Version number (starts at 0). */
    USHORT segCountX2;		/* 2 x segCount. */
    USHORT searchRange;		/* 2 x (2**floor(log2(segCount))). */
    USHORT entrySelector;	/* log2(searchRange/2). */
    USHORT rangeShift;		/* 2 x segCount - searchRange. */
#if 0
    USHORT endCount[segCount]	/* End characterCode for each segment. */
    USHORT reservedPad;		/* Set to 0. */
    USHORT startCount[segCount];/* Start character code for each segment. */
    USHORT idDelta[segCount];	/* Delta for all character in segment. */
    USHORT idRangeOffset[segCount]; /* Offsets into glyphIdArray or 0. */
    USHORT glyphIdArray[]	/* Glyph index array. */
#endif
} SEGMENTTABLE;

typedef struct TRIMMEDTABLE {
    USHORT format;		/* Format number is set to 6. */
    USHORT length;		/* The actual length in bytes of this
				 * subtable. */
    USHORT version;		/* Version number (starts at 0). */
    USHORT firstCode;		/* First character code of subrange. */
    USHORT entryCount;		/* Number of character codes in subrange. */
#if 0
    USHORT glyphIdArray[];	/* Array of glyph index values for 
				        character codes in the range. */
#endif
} TRIMMEDTABLE;

typedef union SUBTABLE {
    ANYTABLE any;
    BYTETABLE byte;
    HIBYTETABLE hiByte;
    SEGMENTTABLE segment;
    TRIMMEDTABLE trimmed;
} SUBTABLE;

#pragma pack()

/*
 *-------------------------------------------------------------------------
 *
 * LoadFontRanges --
 *
 *	Given an HFONT, get the information about the characters that 
 *	this font can display.
 *
 * Results:
 *	If the font has no Unicode character information, the return value
 *	is 0 and *startCountPtr and *endCountPtr are filled with NULL.  
 *	Otherwise, *startCountPtr and *endCountPtr are set to pointers to 
 *	arrays of TrueType character existence information and the return 
 *	value is the length of the arrays (the two arrays are always the 
 *	same length as each other).
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
LoadFontRanges(
    HDC hdc,			/* HDC into which font can be selected. */
    HFONT hFont,		/* HFONT to query. */
    USHORT **startCountPtr,	/* Filled with malloced pointer to 
				 * character range information. */
    USHORT **endCountPtr,	/* Filled with malloced pointer to 
				 * character range information. */
    int *symbolPtr)
 {
    int n, i, swapped, offset, cbData, segCount;
    DWORD cmapKey;
    USHORT *startCount, *endCount;
    CMAPTABLE cmapTable;
    ENCODINGTABLE encTable;
    SUBTABLE subTable;
    char *s;

    segCount = 0;
    startCount = NULL;
    endCount = NULL;
    *symbolPtr = 0;

    hFont = SelectObject(hdc, hFont);

    i = 0;
    s = (char *) &i;
    *s = '\1';
    swapped = 0;

    if (i == 1) {
	swapped = 1;
    }

    cmapKey = CMAPHEX;
    if (swapped) {
	SwapLong(&cmapKey);
    }

    n = GetFontData(hdc, cmapKey, 0, &cmapTable, sizeof(cmapTable));
    if (n != GDI_ERROR) {
	if (swapped) {
	    SwapShort(&cmapTable.numTables);
	}
	for (i = 0; i < cmapTable.numTables; i++) {
	    offset = sizeof(cmapTable) + i * sizeof(encTable);
	    GetFontData(hdc, cmapKey, offset, &encTable, sizeof(encTable));
	    if (swapped) {
		SwapShort(&encTable.platform);
		SwapShort(&encTable.encoding);
		SwapLong(&encTable.offset);
	    }
	    if (encTable.platform != 3) {
		/* 
		 * Not Microsoft encoding.
		 */

		continue;
	    }
	    if (encTable.encoding == 0) {
		*symbolPtr = 1;
	    } else if (encTable.encoding != 1) {
		continue;
	    }

	    GetFontData(hdc, cmapKey, encTable.offset, &subTable, 
		    sizeof(subTable));
	    if (swapped) {
		SwapShort(&subTable.any.format);
	    }
	    if (subTable.any.format == 4) {
		if (swapped) {
		    SwapShort(&subTable.segment.segCountX2);
		}
		segCount = subTable.segment.segCountX2 / 2;
		cbData = segCount * sizeof(USHORT);

		startCount = (USHORT *) ckalloc(cbData);
		endCount = (USHORT *) ckalloc(cbData);

		offset = encTable.offset + sizeof(subTable.segment);
		GetFontData(hdc, cmapKey, offset, endCount, cbData);
		offset += cbData + sizeof(USHORT);
		GetFontData(hdc, cmapKey, offset, startCount, cbData);
		if (swapped) {
		    for (i = 0; i < segCount; i++) {
			SwapShort(&endCount[i]);
			SwapShort(&startCount[i]);
		    }
		}
		if (*symbolPtr != 0) {
		    /*
		     * Empirically determined:  When a symbol font is
		     * loaded, the character existence metrics obtained
		     * from the system are mildly wrong.  If the real range
		     * of the symbol font is from 0020 to 00FE, then the
		     * metrics are reported as F020 to F0FE.  When we load
		     * a symbol font, we must fix the character existence
		     * metrics.
		     */

		    for (i = 0; i < segCount; i++) {
			if ((startCount[i] & 0xff00) == 0xf000) {
			    startCount[i] &= 0xff;
			}
			if ((endCount[i] & 0xff00) == 0xf000) {
			    endCount[i] &= 0xff;
			}
		    }
		}
	    }
	}
    }
    SelectObject(hdc, hFont);

    *startCountPtr = startCount;
    *endCountPtr = endCount;
    return segCount;
}

/*
 *-------------------------------------------------------------------------
 * 
 * SwapShort, SwapLong --
 *
 *	Helper functions to convert the data loaded from TrueType font
 *	files to Intel byte ordering.
 *
 * Results:
 *	Bytes of input value are swapped and stored back in argument.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static void
SwapShort(PUSHORT p)
{
    *p = (SHORT)(HIBYTE(*p) + (LOBYTE(*p) << 8));
}

static void 
SwapLong(PULONG p)
{					     
    ULONG temp;

    temp = (LONG) ((BYTE) *p);
    temp <<= 8;
    *p >>=8;

    temp += (LONG) ((BYTE) *p);
    temp <<= 8;
    *p >>=8;

    temp += (LONG) ((BYTE) *p);
    temp <<= 8;
    *p >>=8;

    temp += (LONG) ((BYTE) *p);
    *p = temp;
}
