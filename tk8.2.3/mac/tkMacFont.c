/* 
 * tkMacFont.c --
 *
 *	Contains the Macintosh implementation of the platform-independant
 *	font package interface.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */
 
#include <Windows.h>
#include <Strings.h>
#include <Fonts.h>
#include <Script.h>
#include <Resources.h>
#include <TextUtils.h>

#include "tkMacInt.h"
#include "tkFont.h"

/*
 * For doing things with Mac strings and Fixed numbers.  This probably should move 
 * the mac header file.
 */

#ifndef StrLength
#define StrLength(s) 		(*((unsigned char *) (s)))
#endif
#ifndef StrBody
#define StrBody(s)		((char *) (s) + 1)
#endif
#define pstrcmp(s1, s2)		RelString((s1), (s2), 1, 1)
#define pstrcasecmp(s1, s2)	RelString((s1), (s2), 0, 1)

#ifndef Fixed2Int
#define Fixed2Int(f)	((f) >> 16)
#define Int2Fixed(i)	((i) << 16)
#endif

/*
 * The preferred font encodings.
 */

static CONST char *encodingList[] = {
    "macRoman", "macJapan", NULL
};

/*
 * The following structures are used to map the script/language codes of a 
 * font to the name that should be passed to Tcl_GetTextEncoding() to obtain
 * the encoding for that font.  The set of numeric constants is fixed and 
 * defined by Apple.
 */
 
static TkStateMap scriptMap[] = {
    {smRoman,		"macRoman"},
    {smJapanese,	"macJapan"},
    {smTradChinese,	"macChinese"},
    {smKorean,		"macKorean"},
    {smArabic,		"macArabic"},
    {smHebrew,		"macHebrew"},
    {smGreek,		"macGreek"},
    {smCyrillic,	"macCyrillic"},
    {smRSymbol,		"macRSymbol"},
    {smDevanagari,	"macDevanagari"},
    {smGurmukhi,	"macGurmukhi"},
    {smGujarati,	"macGujarati"},
    {smOriya,		"macOriya"},
    {smBengali,		"macBengali"},
    {smTamil,		"macTamil"},
    {smTelugu,		"macTelugu"},
    {smKannada,		"macKannada"},
    {smMalayalam,	"macMalayalam"},
    {smSinhalese,	"macSinhalese"},
    {smBurmese,		"macBurmese"},
    {smKhmer,		"macKhmer"},
    {smThai,		"macThailand"},
    {smLaotian,		"macLaos"},
    {smGeorgian,	"macGeorgia"},
    {smArmenian,	"macArmenia"},
    {smSimpChinese,	"macSimpChinese"},
    {smTibetan,		"macTIbet"},
    {smMongolian,	"macMongolia"},
    {smGeez,		"macEthiopia"},
    {smEastEurRoman,	"macCentEuro"},
    {smVietnamese,	"macVietnam"},
    {smExtArabic,	"macSindhi"},
    {NULL,		NULL}
};    

static TkStateMap romanMap[] = {
    {langCroatian,	"macCroatian"},
    {langSlovenian,	"macCroatian"},
    {langIcelandic,	"macIceland"},
    {langRomanian,	"macRomania"},
    {langTurkish,	"macTurkish"},
    {langGreek,		"macGreek"},
    {NULL,		NULL}
};

static TkStateMap cyrillicMap[] = {
    {langUkrainian,	"macUkraine"},
    {langBulgarian,	"macBulgaria"},
    {NULL,		NULL}
};

/*
 * The following structure represents a font family.  It is assumed that
 * all screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a
 * shared instance of this structure.  The most important shared property
 * is the character existence metrics, used to determine if a screen font
 * can display a given Unicode character.
 *
 * Under Macintosh, a "font family" is uniquely identified by its face number.
 */


#define FONTMAP_SHIFT	    	10

#define FONTMAP_PAGES	    	(1 << (sizeof(Tcl_UniChar) * 8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE	(1 << FONTMAP_SHIFT)

typedef struct FontFamily {
    struct FontFamily *nextPtr;	/* Next in list of all known font families. */
    int refCount;		/* How many SubFonts are referring to this
				 * FontFamily.  When the refCount drops to
				 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */

    short faceNum;		/* Unique face number key for this FontFamily. */
    
    /*
     * Derived properties.
     */
     
    Tcl_Encoding encoding;	/* Encoding for this font family. */
    int isSymbolFont;		/* Non-zero if this is a symbol family. */
    int isMultiByteFont;	/* Non-zero if this is a multi-byte family. */
    char typeTable[256];	/* Table that identfies all lead bytes for a
    				 * multi-byte family, used when measuring chars.
    				 * If a byte is a lead byte, the value at the 
    				 * corresponding position in the typeTable is 1, 
    				 * otherwise 0.  If this is a single-byte font, 
    				 * all entries are 0. */
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
} FontFamily;

/*
 * The following structure encapsulates an individual screen font.  A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */

typedef struct SubFont {
    char **fontMap;		/* Pointer to font map from the FontFamily, 
				 * cached here to save a dereference. */
    FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Macintosh's implementation of a font
 * object.
 */

#define SUBFONT_SPACE		3

typedef struct MacFont {
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

    short size;			/* Font size in pixels, constructed from
				 * font attributes. */
    short style;		/* Style bits, constructed from font
				 * attributes. */
} MacFont;

/*
 * The following structure is used to map between the UTF-8 name for a font and
 * the name that the Macintosh uses to refer to the font, in order to determine
 * if a font exists.  The Macintosh names for fonts are stored in the encoding 
 * of the font itself.
 */
 
typedef struct FontNameMap {
    Tk_Uid utfName;		/* The name of the font in UTF-8. */
    StringPtr nativeName;	/* The name of the font in the font's encoding. */
    short faceNum;		/* Unique face number for this font. */
} FontNameMap;

/*
 * The list of font families that are currently loaded.  As screen fonts
 * are loaded, this list grows to hold information about what characters
 * exist in each font family.
 */

static FontFamily *fontFamilyList = NULL;

/*
 * Information cached about the system at startup time.
 */
 
static FontNameMap *gFontNameMap = NULL;
static GWorldPtr gWorld = NULL;

/*
 * Procedures used only in this file.
 */

static FontFamily *	AllocFontFamily(CONST MacFont *fontPtr, int family);
static SubFont *	CanUseFallback(MacFont *fontPtr,
			    CONST char *fallbackName, int ch);
static SubFont *	CanUseFallbackWithAliases(MacFont *fontPtr, 
			    char *faceName, int ch, Tcl_DString *nameTriedPtr);
static SubFont *	FindSubFontForChar(MacFont *fontPtr, int ch);
static void		FontMapInsert(SubFont *subFontPtr, int ch);
static void		FontMapLoadPage(SubFont *subFontPtr, int row);
static int		FontMapLookup(SubFont *subFontPtr, int ch);
static void 		FreeFontFamily(FontFamily *familyPtr);
static void		InitFont(Tk_Window tkwin, int family, int size, 
			    int style, MacFont *fontPtr);
static void		InitSubFont(CONST MacFont *fontPtr, int family, 
			    SubFont *subFontPtr);
static void		MultiFontDrawText(MacFont *fontPtr,
			    CONST char *source, int numBytes, int x, int y);
static void		ReleaseFont(MacFont *fontPtr);
static void		ReleaseSubFont(SubFont *subFontPtr);
static int		SeenName(CONST char *name, Tcl_DString *dsPtr);

static char *      	BreakLine(FontFamily *familyPtr, int flags, 
			    CONST char *source, int numBytes, int *widthPtr);
static int		GetFamilyNum(CONST char *faceName, short *familyPtr);
static int		GetFamilyOrAliasNum(CONST char *faceName, 
			    short *familyPtr);
static Tcl_Encoding	GetFontEncoding(int faceNum, int allowSymbol,
			    int *isSymbolPtr);
static Tk_Uid		GetUtfFaceName(StringPtr faceNameStr);


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
 *	See comments below.
 *
 *-------------------------------------------------------------------------
 */

void
TkpFontPkgInit(mainPtr)
    TkMainInfo *mainPtr;	/* The application being created. */
{
    MenuHandle fontMenu;
    FontNameMap *tmpFontNameMap, *newFontNameMap, *mapPtr;
    int i, j, numFonts, fontMapOffset, isSymbol;
    Str255 nativeName;
    Tcl_DString ds;
    Tcl_Encoding encoding;
    Tcl_Encoding *encodings;
    
    if (gWorld == NULL) {
	/* 
	 * Do the following one time only.
	 */

	Rect rect = {0, 0, 1, 1};

	SetFractEnable(0);
	
	/*
	 * Used for saving and restoring state while drawing and measuring.
	 */
	 
	if (NewGWorld(&gWorld, 0, &rect, NULL, NULL, 0) != noErr) {
	    panic("TkpFontPkgInit: NewGWorld failed");
	}
	
	/*
	 * The name of each font is stored in the encoding of that font.
	 * How would we translate a name from UTF-8 into the native encoding
	 * of the font unless we knew the encoding of that font?  We can't.
	 * So, precompute the UTF-8 and native names of all fonts on the 
	 * system.  The when the user asks for font by its UTF-8 name, we
	 * lookup the name in that table and really ask for the font by its
	 * native name.  Any unknown UTF-8 names will be mapped to the system 
	 * font.
	 */
	
	fontMenu = NewMenu('FT', "\px");
	AddResMenu(fontMenu, 'FONT');
	
	numFonts = CountMItems(fontMenu);
	tmpFontNameMap = (FontNameMap *) ckalloc(sizeof(FontNameMap) * numFonts);
	encodings = (Tcl_Encoding *) ckalloc(sizeof(Tcl_Encoding) * numFonts);

	mapPtr = tmpFontNameMap;
	for (i = 0; i < numFonts; i++) {
       	    GetMenuItemText(fontMenu, i + 1, nativeName);
       	    GetFNum(nativeName, &mapPtr->faceNum);
       	    encodings[i] = GetFontEncoding(mapPtr->faceNum, 0, &isSymbol);
       	    Tcl_ExternalToUtfDString(encodings[i], StrBody(nativeName), 
       	    	    StrLength(nativeName), &ds);
       	    mapPtr->utfName = Tk_GetUid(Tcl_DStringValue(&ds));
       	    mapPtr->nativeName = (StringPtr) ckalloc(StrLength(nativeName) + 1);
       	    memcpy(mapPtr->nativeName, nativeName, StrLength(nativeName) + 1);
       	    Tcl_DStringFree(&ds);
       	    mapPtr++;
       	}
       	DisposeMenu(fontMenu);
       	
       	/*
       	 * Reorder FontNameMap so fonts with the preferred encodings are at 
       	 * the front of the list.  The relative order of fonts that all have
       	 * the same encoding is preserved.  Fonts with unknown encodings get
       	 * stuck at the end.
       	 */
       	 
       	newFontNameMap = (FontNameMap *) ckalloc(sizeof(FontNameMap) * (numFonts + 1));
       	fontMapOffset = 0;
       	for (i = 0; encodingList[i] != NULL; i++) {
       	    encoding = Tcl_GetEncoding(NULL, encodingList[i]);
       	    if (encoding == NULL) {
       	    	continue;
       	    }
       	    for (j = 0; j < numFonts; j++) {
       	    	if (encodings[j] == encoding) {
       	    	    newFontNameMap[fontMapOffset] = tmpFontNameMap[j];
       	    	    fontMapOffset++;
       	    	    Tcl_FreeEncoding(encodings[j]);
       	    	    tmpFontNameMap[j].utfName = NULL;
       	    	}
       	    }
       	    Tcl_FreeEncoding(encoding);
       	} 
       	for (i = 0; i < numFonts; i++) {
       	    if (tmpFontNameMap[i].utfName != NULL) {
       	        newFontNameMap[fontMapOffset] = tmpFontNameMap[i];
       	        fontMapOffset++;
       	        Tcl_FreeEncoding(encodings[i]);
       	    }
       	}
       	if (fontMapOffset != numFonts) {
       	    panic("TkpFontPkgInit: unexpected number of fonts");
       	}

       	mapPtr = &newFontNameMap[numFonts];
       	mapPtr->utfName = NULL;
       	mapPtr->nativeName = NULL;
       	mapPtr->faceNum = 0;

       	ckfree((char *) tmpFontNameMap);
       	ckfree((char *) encodings);
       	
       	gFontNameMap = newFontNameMap;
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
 *	the contents of the generics TkFont before calling TkpDeleteFont().
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkFont *
TkpGetNativeFont(
    Tk_Window tkwin,	/* For display where font will be used. */
    CONST char *name)	/* Platform-specific font name. */
{
    short family;
    MacFont *fontPtr;
    
    if (strcmp(name, "system") == 0) {
	family = GetSysFont();
    } else if (strcmp(name, "application") == 0) {
	family = GetAppFont();
    } else {
	return NULL;
    }
    
    fontPtr = (MacFont *) ckalloc(sizeof(MacFont));
    InitFont(tkwin, family, 0, 0, fontPtr);
    
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
 *	substituted automatically.
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
 *	None.
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
    short faceNum, style;
    int i, j;
    char *faceName, *fallback;
    char ***fallbacks;
    MacFont *fontPtr;
        
    /*
     * Algorithm to get the closest font to the one requested.
     *
     * try fontname
     * try all aliases for fontname
     * foreach fallback for fontname
     *	    try the fallback
     *	    try all aliases for the fallback
     */
     
    faceNum = 0;
    faceName = faPtr->family;
    if (faceName != NULL) {
        if (GetFamilyOrAliasNum(faceName, &faceNum) != 0) {
            goto found;
        }
        fallbacks = TkFontGetFallbacks();
	for (i = 0; fallbacks[i] != NULL; i++) {
	    for (j = 0; (fallback = fallbacks[i][j]) != NULL; j++) {
		if (strcasecmp(faceName, fallback) == 0) {
		    for (j = 0; (fallback = fallbacks[i][j]) != NULL; j++) {
		        if (GetFamilyOrAliasNum(fallback, &faceNum)) {
		            goto found;
		        }
		    }
		}
		break;
	    }
	}
    }
    
    found:    
    style = 0;
    if (faPtr->weight != TK_FW_NORMAL) {
	style |= bold;
    }
    if (faPtr->slant != TK_FS_ROMAN) {
	style |= italic;
    }
    if (faPtr->underline) {
	style |= underline;
    }
    if (tkFontPtr == NULL) {
	fontPtr = (MacFont *) ckalloc(sizeof(MacFont));
    } else {
	fontPtr = (MacFont *) tkFontPtr;
	ReleaseFont(fontPtr);
    }
    InitFont(tkwin, faceNum, faPtr->size, style, fontPtr);
    
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
    MacFont *fontPtr;
    
    fontPtr = (MacFont *) tkFontPtr;
    ReleaseFont(fontPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetFontFamilies --
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
    FontNameMap *mapPtr;
    Tcl_Obj *resultPtr, *strPtr;
        
    resultPtr = Tcl_GetObjResult(interp);
    for (mapPtr = gFontNameMap; mapPtr->utfName != NULL; mapPtr++) {
        strPtr = Tcl_NewStringObj(mapPtr->utfName, -1);
        Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
    }
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
TkpGetSubFonts(interp, tkfont)
    Tcl_Interp *interp;		/* Interp to hold result. */
    Tk_Font tkfont;		/* Font object to query. */
{
    int i;
    Tcl_Obj *resultPtr, *strPtr;
    MacFont *fontPtr;
    FontFamily *familyPtr;
    Str255 nativeName;

    resultPtr = Tcl_GetObjResult(interp);    
    fontPtr = (MacFont *) tkfont;
    for (i = 0; i < fontPtr->numSubFonts; i++) {
	familyPtr = fontPtr->subFontArray[i].familyPtr;
    	GetFontName(familyPtr->faceNum, nativeName);
	strPtr = Tcl_NewStringObj(GetUtfFaceName(nativeName), -1);
	Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  Tk_MeasureChars --
 *
 *	Determine the number of characters from the string that will fit
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
				 * permissible line length; don't consider any
				 * character that would cross this
				 * x-position.  If < 0, then line length is
				 * unbounded and the flags argument is
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
    MacFont *fontPtr;
    FontFamily *lastFamilyPtr;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    int curX, curByte;

    /*
     * According to "Inside Macintosh: Text", the Macintosh may
     * automatically substitute
     * ligatures or context-sensitive presentation forms when
     * measuring/displaying text within a font run.  We cannot safely
     * measure individual characters and add up the widths w/o errors.
     * However, if we convert a range of text from UTF-8 to, say,
     * Shift-JIS, and get the offset into the Shift-JIS string as to
     * where a word or line break would occur, then can we map that
     * number back to UTF-8?
     */
     
    fontPtr = (MacFont *) tkfont;

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(gWorld, NULL);
    
    TextSize(fontPtr->size);
    TextFace(fontPtr->style);

    lastFamilyPtr = fontPtr->subFontArray[0].familyPtr; 
    
    if (numBytes == 0) {
    	curX = 0;
    	curByte = 0;
    } else if (maxLength < 0) {
    	CONST char *p, *end, *next;
    	Tcl_UniChar ch;
    	FontFamily *thisFamilyPtr;
    	Tcl_DString runString;
    	 
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
            thisFamilyPtr = FindSubFontForChar(fontPtr, ch)->familyPtr;
            if (thisFamilyPtr != lastFamilyPtr) {
                TextFont(lastFamilyPtr->faceNum);
                Tcl_UtfToExternalDString(lastFamilyPtr->encoding, source, 
                	p - source, &runString);
                curX += TextWidth(Tcl_DStringValue(&runString), 0, 
                	Tcl_DStringLength(&runString));
                Tcl_DStringFree(&runString);
                lastFamilyPtr = thisFamilyPtr;
                source = p;
            }
            p = next;
        }
	TextFont(lastFamilyPtr->faceNum);
        Tcl_UtfToExternalDString(lastFamilyPtr->encoding, source, p - source, 
        	&runString);
        curX += TextWidth(Tcl_DStringValue(&runString), 0, 
        	Tcl_DStringLength(&runString));
        Tcl_DStringFree(&runString);
	curByte = numBytes;
    } else {
        CONST char *p, *end, *next, *sourceOrig;
        int widthLeft;
        FontFamily *thisFamilyPtr;
        Tcl_UniChar ch;
        char *rest;
        
	/*
	 * How many chars will fit in the space allotted? 
	 */
	
	if (maxLength > 32767) {
            maxLength = 32767;
        }
        
        widthLeft = maxLength; 
        sourceOrig = source;
        end = source + numBytes;      
	for (p = source; p < end; p = next) {
	    next = p + Tcl_UtfToUniChar(p, &ch);
  	    thisFamilyPtr = FindSubFontForChar(fontPtr, ch)->familyPtr;
  	    if (thisFamilyPtr != lastFamilyPtr) {
  	        if (p > source) {
  	            rest = BreakLine(lastFamilyPtr, flags, source, 
  	            	    p - source, &widthLeft);
  	            flags &= ~TK_AT_LEAST_ONE;
  	            if (rest != NULL) {
  	                p = source;
  	                break;
  	            }
  	        }
                lastFamilyPtr = thisFamilyPtr;
                source = p;
            }
        }
        
        if (p > source) {
            rest = BreakLine(lastFamilyPtr, flags, source, p - source, 
            	    &widthLeft);
        }
        
        if (rest == NULL) {
            curByte = numBytes;
        } else {
            curByte = rest - sourceOrig;
        }
        curX = maxLength - widthLeft;
    }

    SetGWorld(saveWorld, saveDevice);

    *lengthPtr = curX;
    return curByte;
}

/*
 *---------------------------------------------------------------------------
 *
 * BreakLine --
 *
 *	Determine where the given line of text should be broken so that it
 *	fits in the specified range.  Before calling this function, the 
 *	font values and graphics port must be set.
 *
 * Results:
 *	The return value is NULL if the specified range is larger that the
 *	space the text needs, and *widthLeftPtr is filled with how much 
 *	space is left in the range after measuring the whole text buffer.
 *	Otherwise, the return value is a pointer into the text buffer that 
 *	indicates where the line should be broken (up to, but not including 
 *	that character), and *widthLeftPtr is filled with how much space is 
 *	left in the range after measuring up to that character.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
 
static char *      	
BreakLine(
    FontFamily *familyPtr,	/* FontFamily that describes the font values
    				 * that are already selected into the graphics
    				 * port. */
    int flags,			/* Various flag bits OR-ed together:
				 * TK_PARTIAL_OK means include the last char
				 * which only partially fit on this line.
				 * TK_WHOLE_WORDS means stop on a word
				 * boundary, if possible.
				 * TK_AT_LEAST_ONE means return at least one
				 * character even if no characters fit. */				 
    CONST char *source,		/* UTF-8 string to be displayed.  Need not be
				 * '\0' terminated. */
    int numBytes,		/* Maximum number of bytes to consider
				 * from source string. */
    int *widthLeftPtr) 		/* On input, specifies size of range into 
    				 * which characters from source buffer should
    				 * be fit.  On output, filled with how much
    				 * space is left after fitting as many 
    				 * characters as possible into the range. 
    				 * Result may be negative if TK_AT_LEAST_ONE
    				 * was specified in the flags argument. */
{
    Fixed pixelWidth, widthLeft;
    StyledLineBreakCode breakCode;
    Tcl_DString runString;
    long textOffset;
    Boolean leadingEdge;
    Point point;
    int charOffset, thisCharWasDoubleByte;
    char *p, *end, *typeTable;
    
    TextFont(familyPtr->faceNum);
    Tcl_UtfToExternalDString(familyPtr->encoding, source, numBytes,
    	    &runString);
    pixelWidth = Int2Fixed(*widthLeftPtr) + 1;
    if (flags & TK_WHOLE_WORDS) {
        textOffset = (flags & TK_AT_LEAST_ONE);  
        widthLeft = pixelWidth;
	breakCode = StyledLineBreak(Tcl_DStringValue(&runString),
		Tcl_DStringLength(&runString), 0, Tcl_DStringLength(&runString), 
		0, &widthLeft, &textOffset);
        if (breakCode != smBreakOverflow) {
            /* 
             * StyledLineBreak includes all the space characters at the end of 
             * line that we want to suppress.
             */
             
            textOffset = VisibleLength(Tcl_DStringValue(&runString), textOffset);
            goto getoffset;
        }
    } else {
        point.v = 1;
        point.h = 1;
	textOffset = PixelToChar(Tcl_DStringValue(&runString),
		Tcl_DStringLength(&runString), 0, pixelWidth, &leadingEdge, 
		&widthLeft, smOnlyStyleRun, point, point);        
	if (Fixed2Int(widthLeft) < 0) {
	    goto getoffset;
	}
    }
    *widthLeftPtr = Fixed2Int(widthLeft);
    Tcl_DStringFree(&runString);
    return NULL;

    /*
     * The conversion routine that converts UTF-8 to the target encoding
     * must map one UTF-8 character to exactly one encoding-specific
     * character, so that the following algorithm works:
     *  
     * 1. Get byte offset of where line should be broken.
     * 2. Get char offset corresponding to that byte offset.
     * 3. Map that char offset to byte offset in UTF-8 string.
     */ 

    getoffset:
    thisCharWasDoubleByte = 0;
    if (familyPtr->isMultiByteFont == 0) {
        charOffset = textOffset;
    } else {
        charOffset = 0;
        typeTable = familyPtr->typeTable;
        
        p = Tcl_DStringValue(&runString);
        end = p + textOffset;
        thisCharWasDoubleByte = typeTable[*((unsigned char *) p)];
        for ( ; p < end; p++) {
            thisCharWasDoubleByte = typeTable[*((unsigned char *) p)];
            p += thisCharWasDoubleByte;
            charOffset++;
        }
    }
    
    if ((flags & TK_WHOLE_WORDS) == 0) {
    	if ((flags & TK_PARTIAL_OK) && (leadingEdge != 0)) {
	    textOffset += thisCharWasDoubleByte;
	    textOffset++;
	    charOffset++;
        } else if (((flags & TK_PARTIAL_OK) == 0) && (leadingEdge == 0)) {
	    textOffset -= thisCharWasDoubleByte;
	    textOffset--;
	    charOffset--;
	}
    }
    if ((textOffset == 0) && (Tcl_DStringLength(&runString) > 0) 
    	    && (flags & TK_AT_LEAST_ONE)) {
    	p = Tcl_DStringValue(&runString);
        textOffset += familyPtr->typeTable[*((unsigned char *) p)];
        textOffset++;
        charOffset++;
    }
    *widthLeftPtr = Fixed2Int(pixelWidth) 
    	    - TextWidth(Tcl_DStringValue(&runString), 0, textOffset);
    Tcl_DStringFree(&runString);
    return Tcl_UtfAtIndex(source, charOffset);
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
    MacFont *fontPtr;
    MacDrawable *macWin;
    RGBColor macColor, origColor;
    GWorldPtr destPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    short txFont, txFace, txSize;
    BitMapPtr stippleMap;

    fontPtr = (MacFont *) tkfont;
    macWin = (MacDrawable *) drawable;

    destPort = TkMacGetDrawablePort(drawable);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    TkMacSetUpClippingRgn(drawable);
    TkMacSetUpGraphicsPort(gc);
    
    txFont = tcl_macQdPtr->thePort->txFont;
    txFace = tcl_macQdPtr->thePort->txFace;
    txSize = tcl_macQdPtr->thePort->txSize;
    GetForeColor(&origColor);
    
    if ((gc->fill_style == FillStippled
	    || gc->fill_style == FillOpaqueStippled)
	    && gc->stipple != None) {
	Pixmap pixmap;
	GWorldPtr bufferPort;
	
	stippleMap = TkMacMakeStippleMap(drawable, gc->stipple);

	pixmap = Tk_GetPixmap(display, drawable, 	
		stippleMap->bounds.right, stippleMap->bounds.bottom, 0);
		
	bufferPort = TkMacGetDrawablePort(pixmap);
	SetGWorld(bufferPort, NULL);
	
	if (TkSetMacColor(gc->foreground, &macColor) == true) {
	    RGBForeColor(&macColor);
	}
	ShowPen();
	FillRect(&stippleMap->bounds, &tcl_macQdPtr->white);
	MultiFontDrawText(fontPtr, source, numBytes, 0, 0);

	SetGWorld(destPort, NULL);
	CopyDeepMask(&((GrafPtr) bufferPort)->portBits, stippleMap, 
		&((GrafPtr) destPort)->portBits, &stippleMap->bounds,
		&stippleMap->bounds, &((GrafPtr) destPort)->portRect,
		srcOr, NULL);
	
	/* TODO: this doesn't work quite right - it does a blend.   you can't
	 * draw white text when you have a stipple.
	 */
		
	Tk_FreePixmap(display, pixmap);
	ckfree(stippleMap->baseAddr);
	ckfree((char *)stippleMap);
    } else {	
	if (TkSetMacColor(gc->foreground, &macColor) == true) {
	    RGBForeColor(&macColor);
	}
	ShowPen();
	MultiFontDrawText(fontPtr, source, numBytes, macWin->xOff + x,
		macWin->yOff + y);
    }

    TextFont(txFont);
    TextSize(txSize);
    TextFace(txFace);
    RGBForeColor(&origColor);
    SetGWorld(saveWorld, saveDevice);
}

/*
 *-------------------------------------------------------------------------
 *
 * MultiFontDrawText --
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
MultiFontDrawText(
    MacFont *fontPtr,		/* Contains set of fonts to use when drawing
				 * following string. */
    CONST char *source,		/* Potentially multilingual UTF-8 string. */
    int numBytes,		/* Length of string in bytes. */
    int x, int y)		/* Coordinates at which to place origin *
				 * of string when drawing. */
{
    FontFamily *lastFamilyPtr, *thisFamilyPtr;
    Tcl_DString runString;
    CONST char *p, *end, *next;
    Tcl_UniChar ch;
    
    TextSize(fontPtr->size);
    TextFace(fontPtr->style);

    lastFamilyPtr = fontPtr->subFontArray[0].familyPtr;
    
    end = source + numBytes;
    for (p = source; p < end; ) {
        next = p + Tcl_UtfToUniChar(p, &ch);
        thisFamilyPtr = FindSubFontForChar(fontPtr, ch)->familyPtr;
        if (thisFamilyPtr != lastFamilyPtr) {
            if (p > source) {
		TextFont(lastFamilyPtr->faceNum);
 		Tcl_UtfToExternalDString(lastFamilyPtr->encoding, source, 
		        p - source, &runString);
		MoveTo((short) x, (short) y);
		DrawText(Tcl_DStringValue(&runString), 0, 
		        Tcl_DStringLength(&runString));
		x += TextWidth(Tcl_DStringValue(&runString), 0, 
		        Tcl_DStringLength(&runString));
		Tcl_DStringFree(&runString);
                source = p;
	    }
            lastFamilyPtr = thisFamilyPtr;
        }
        p = next;
    }
    if (p > source) {
        TextFont(thisFamilyPtr->faceNum);
	Tcl_UtfToExternalDString(lastFamilyPtr->encoding, source, 
	        p - source, &runString);
	MoveTo((short) x, (short) y);
    	DrawText(Tcl_DStringValue(&runString), 0, 
	        Tcl_DStringLength(&runString));
	Tcl_DStringFree(&runString);
    }
}        

/*
 *---------------------------------------------------------------------------
 *
 * TkMacIsCharacterMissing --
 *
 *	Given a tkFont and a character determines whether the character has
 *	a glyph defined in the font or not. Note that this is potentially
 *	not compatible with Mac OS 8 as it looks at the font handle
 *	structure directly. Looks into the character array of the font
 *	handle to determine whether the glyph is defined or not.
 *
 * Results:
 *	Returns a 1 if the character is missing, a 0 if it is not.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkMacIsCharacterMissing(
    Tk_Font tkfont,		/* The font we are looking in. */
    unsigned int searchChar)	/* The character we are looking for. */
{
    MacFont *fontPtr = (MacFont *) tkfont;
    FMInput fm;
    FontRec **fontRecHandle;
    
    fm.family = fontPtr->subFontArray[0].familyPtr->faceNum;
    fm.size = fontPtr->size;
    fm.face = fontPtr->style;
    fm.needBits = 0;
    fm.device = 0;
    fm.numer.h = fm.numer.v = fm.denom.h = fm.denom.v = 1;
    
#if !defined(UNIVERSAL_INTERFACES_VERSION) || (UNIVERSAL_INTERFACES_VERSION < 0x0300)
    fontRecHandle = (FontRec **) FMSwapFont(&fm)->fontResult;
#else
    fontRecHandle = (FontRec **) FMSwapFont(&fm)->fontHandle;
#endif
    return *(short *) ((long) &(*fontRecHandle)->owTLoc 
    	    + ((long)((*fontRecHandle)->owTLoc + searchChar 
    	    - (*fontRecHandle)->firstChar) * sizeof(short))) == -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitFont --
 *
 *	Helper for TkpGetNativeFont() and TkpGetFontFromAttributes().
 *	Initializes the memory for a MacFont that wraps the platform-specific
 *	data.
 *
 *	The caller is responsible for initializing the fields of the
 *	TkFont that are used exclusively by the generic TkFont code, and
 *	for releasing those fields before calling TkpDeleteFont().
 *
 * Results:
 *	Fills the MacFont structure.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */ 

static void
InitFont(
    Tk_Window tkwin,		/* For display where font will be used. */
    int faceNum,		/* Macintosh font number. */
    int size,			/* Point size for Macintosh font. */
    int style,			/* Macintosh style bits. */
    MacFont *fontPtr)		/* Filled with information constructed from
				 * the above arguments. */
{
    Str255 nativeName;
    FontInfo fi;
    TkFontAttributes *faPtr;
    TkFontMetrics *fmPtr;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    short pixels;

    if (size == 0) {
    	size = -GetDefFontSize();
    }
    pixels = (short) TkFontGetPixels(tkwin, size);
    
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(gWorld, NULL);
    TextFont(faceNum);
    TextSize(pixels);
    TextFace(style);

    GetFontInfo(&fi);
    GetFontName(faceNum, nativeName);

    fontPtr->font.fid	= (Font) fontPtr;

    faPtr 		= &fontPtr->font.fa;
    faPtr->family	= GetUtfFaceName(nativeName);
    faPtr->size		= TkFontGetPoints(tkwin, size);
    faPtr->weight	= (style & bold) ? TK_FW_BOLD : TK_FW_NORMAL;
    faPtr->slant	= (style & italic) ? TK_FS_ITALIC : TK_FS_ROMAN;
    faPtr->underline	= ((style & underline) != 0);
    faPtr->overstrike	= 0;

    fmPtr 		= &fontPtr->font.fm;
    fmPtr->ascent	= fi.ascent;	
    fmPtr->descent	= fi.descent;	
    fmPtr->maxWidth	= fi.widMax;
    fmPtr->fixed	= (CharWidth('i') == CharWidth('w'));
    
    fontPtr->size	= pixels;
    fontPtr->style	= (short) style;
        
    fontPtr->numSubFonts 	= 1;
    fontPtr->subFontArray	= fontPtr->staticSubFonts;
    InitSubFont(fontPtr, faceNum, &fontPtr->subFontArray[0]);

    SetGWorld(saveWorld, saveDevice);
}

/*
 *-------------------------------------------------------------------------
 *
 * ReleaseFont --
 * 
 *	Called to release the Macintosh-specific contents of a TkFont.
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
    MacFont *fontPtr)		/* The font to delete. */
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
    CONST MacFont *fontPtr,	/* Font object in which the SubFont will be
    				 * used. */
    int faceNum,		/* The font number. */
    SubFont *subFontPtr)	/* Filled with SubFont constructed from 
    				 * above attributes. */
{
    subFontPtr->familyPtr = AllocFontFamily(fontPtr, faceNum);
    subFontPtr->fontMap = subFontPtr->familyPtr->fontMap;
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
    FreeFontFamily(subFontPtr->familyPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * AllocFontFamily --
 *
 *	Find the FontFamily structure associated with the given font 
 *	family.  The information should be stored by the caller in a 
 *	SubFont and used when determining if that SubFont supports a 
 *	character. 
 *
 * Results:
 *	A pointer to a FontFamily.  The reference count in the FontFamily
 *	is automatically incremented.  When the SubFont is released, the
 *	reference count is decremented.  When no SubFont is using this
 *	FontFamily, it may be deleted.
 *
 * Side effects:
 *	A new FontFamily structure will be allocated if this font family
 *	has not been seen.  
 *
 *-------------------------------------------------------------------------
 */

static FontFamily *
AllocFontFamily(
    CONST MacFont *fontPtr,	/* Font object in which the FontFamily will
    				 * be used. */
    int faceNum)		/* The font number. */
{
    FontFamily *familyPtr;
    int i;
    
    familyPtr = fontFamilyList;
    for (; familyPtr != NULL; familyPtr = familyPtr->nextPtr) {
	if (familyPtr->faceNum == faceNum) {
	    familyPtr->refCount++;
	    return familyPtr;
	}
    }

    familyPtr = (FontFamily *) ckalloc(sizeof(FontFamily));
    memset(familyPtr, 0, sizeof(FontFamily));
    familyPtr->nextPtr = fontFamilyList;
    fontFamilyList = familyPtr;

    /* 
     * Set key for this FontFamily. 
     */
     
    familyPtr->faceNum = faceNum;

    /* 
     * An initial refCount of 2 means that FontFamily information will
     * persist even when the SubFont that loaded the FontFamily is released.
     * Change it to 1 to cause FontFamilies to be unloaded when not in use.
     */
     
    familyPtr->refCount = 2;
    familyPtr->encoding = GetFontEncoding(faceNum, 1, &familyPtr->isSymbolFont);
    familyPtr->isMultiByteFont = 0;
    FillParseTable(familyPtr->typeTable, FontToScript(faceNum));
    for (i = 0; i < 256; i++) {
        if (familyPtr->typeTable[i] != 0) {
            familyPtr->isMultiByteFont = 1;
            break;
        }
    }
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
    FontFamily **familyPtrPtr;
    int i;

    if (familyPtr == NULL) {
        return;
    }
    familyPtr->refCount--;
    if (familyPtr->refCount > 0) {
    	return;
    }
    Tcl_FreeEncoding(familyPtr->encoding);
    for (i = 0; i < FONTMAP_PAGES; i++) {
        if (familyPtr->fontMap[i] != NULL) {
            ckfree((char *) familyPtr->fontMap[i]);
        }
    }
    
    /* 
     * Delete from list. 
     */
         
    for (familyPtrPtr = &fontFamilyList; ; ) {
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
 *	Determine which physical screen font is necessary to use to 
 *	display the given character.  If the font object does not have
 *	a screen font that can display the character, another screen font
 *	may be loaded into the font object, following a set of preferred
 *	fallback rules.
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
    MacFont *fontPtr,		/* The font object with which the character
				 * will be displayed. */
    int ch)			/* The Unicode character to be displayed. */
{
    int i, j, k;
    char *fallbackName;
    char **aliases;
    SubFont *subFontPtr;
    FontNameMap *mapPtr;
    Tcl_DString faceNames;
    char ***fontFallbacks;
    char **anyFallbacks;
    
    if (FontMapLookup(&fontPtr->subFontArray[0], ch)) {
	return &fontPtr->subFontArray[0];
    }

    for (i = 1; i < fontPtr->numSubFonts; i++) {
	if (FontMapLookup(&fontPtr->subFontArray[i], ch)) {
	    return &fontPtr->subFontArray[i];
	}
    }

    /*
     * Keep track of all face names that we check, so we don't check some
     * name multiple times if it can be reached by multiple paths.
     */
     
    Tcl_DStringInit(&faceNames);
        
    aliases = TkFontGetAliasList(fontPtr->font.fa.family);

    subFontPtr = NULL;
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
	    subFontPtr = CanUseFallbackWithAliases(fontPtr, fallbackName,
		    ch, &faceNames);
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
	subFontPtr = CanUseFallbackWithAliases(fontPtr, fallbackName, ch,
		&faceNames);
	if (subFontPtr != NULL) {
	    goto end;
	}
    }

    /*
     * Try all face names available in the whole system until we
     * find one that can be used.
     */

    for (mapPtr = gFontNameMap; mapPtr->utfName != NULL; mapPtr++) {
        fallbackName = mapPtr->utfName;
	if (SeenName(fallbackName, &faceNames) == 0) {
	    subFontPtr = CanUseFallback(fontPtr, fallbackName, ch);
	    if (subFontPtr != NULL) {
		goto end;
	    }
	}
    }
    
    end:
    Tcl_DStringFree(&faceNames);
    
    if (subFontPtr == NULL) {
        /* 
         * No font can display this character.  We will use the base font
         * and have it display the "unknown" character.
         */

	subFontPtr = &fontPtr->subFontArray[0];
        FontMapInsert(subFontPtr, ch);
    }
    return subFontPtr;
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
    FMInput fm;
    FontRec *fontRecPtr;
    short *widths;
    int i, end, bitOffset, isMultiByteFont;
    char src[TCL_UTF_MAX];
    unsigned char buf[16];
    int srcRead, dstWrote;
    Tcl_Encoding encoding;
     
    subFontPtr->fontMap[row] = (char *) ckalloc(FONTMAP_BITSPERPAGE / 8);
    memset(subFontPtr->fontMap[row], 0, FONTMAP_BITSPERPAGE / 8);
    
    encoding = subFontPtr->familyPtr->encoding;
    
    fm.family 	= subFontPtr->familyPtr->faceNum;
    fm.size 	= 12;
    fm.face 	= 0;
    fm.needBits = 0;
    fm.device	= 0;
    fm.numer.h	= 1;
    fm.numer.v	= 1;
    fm.denom.h	= 1;
    fm.denom.v	= 1;
    
#if !defined(UNIVERSAL_INTERFACES_VERSION) || (UNIVERSAL_INTERFACES_VERSION < 0x0300)
    fontRecPtr = *((FontRec **) FMSwapFont(&fm)->fontResult);
#else
    fontRecPtr = *((FontRec **) FMSwapFont(&fm)->fontHandle);
#endif
    widths = (short *) ((long) &fontRecPtr->owTLoc 
    	    + ((long) (fontRecPtr->owTLoc - fontRecPtr->firstChar) 
    	    		* sizeof(short)));
    isMultiByteFont = subFontPtr->familyPtr->isMultiByteFont;
    	    		
    end = (row + 1) << FONTMAP_SHIFT;
    for (i = row << FONTMAP_SHIFT; i < end; i++) {
        if (Tcl_UtfToExternal(NULL, encoding, src, Tcl_UniCharToUtf(i, src), 
        	TCL_ENCODING_STOPONERROR, NULL, (char *) buf, sizeof(buf), 
		&srcRead, &dstWrote, NULL) == TCL_OK) {
            
            if (((isMultiByteFont != 0) && (buf[0] > 31))
            	    || (widths[buf[0]] != -1)) {
            	if ((buf[0] == 0x11) && (widths[0x12] == -1)) {
            	    continue;
            	}
            	
                /* 
                 * Mac's char existence metrics are only for one-byte
                 * characters.  If we have a double-byte char, just 
                 * assume that the font supports that char if the font's 
                 * encoding supports that char.
                 */
                
                bitOffset = i & (FONTMAP_BITSPERPAGE - 1);
		subFontPtr->fontMap[row][bitOffset >> 3] |= 1 << (bitOffset & 7);
	    }
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
    MacFont *fontPtr,		/* The font object that will own the new
				 * screen font. */
    char *faceName,		/* Desired face name for new screen font. */
    int ch,			/* The Unicode character that the new
				 * screen font must be able to display. */
    Tcl_DString *nameTriedPtr)	/* Records face names that have already
				 * been tried.  It is possible for the same
				 * face name to be queried multiple times when
				 * trying to find a suitable screen font. */
{
    SubFont *subFontPtr;
    char **aliases;
    int i;
    
    if (SeenName(faceName, nameTriedPtr) == 0) {
	subFontPtr = CanUseFallback(fontPtr, faceName, ch);
	if (subFontPtr != NULL) {
	    return subFontPtr;
	}
    }
    aliases = TkFontGetAliasList(faceName);
    if (aliases != NULL) {
	for (i = 0; aliases[i] != NULL; i++) {
	    if (SeenName(aliases[i], nameTriedPtr) == 0) {
		subFontPtr = CanUseFallback(fontPtr, aliases[i], ch);
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
 *	If the specified physical screen font has not already been loaded 
 *	into the font object, determine if the specified physical screen 
 *	font can display the given character.
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
    MacFont *fontPtr,		/* The font object that will own the new
				 * screen font. */
    CONST char *faceName,	/* Desired face name for new screen font. */
    int ch)			/* The Unicode character that the new
				 * screen font must be able to display. */
{
    int i;
    SubFont subFont;
    short faceNum;

    if (GetFamilyNum(faceName, &faceNum) == 0) {
        return NULL;
    }
    
    /* 
     * Skip all fonts we've already used.
     */
     
    for (i = 0; i < fontPtr->numSubFonts; i++) {
	if (faceNum == fontPtr->subFontArray[i].familyPtr->faceNum) {
	    return NULL;
	}
    }
    
    /*
     * Load this font and see if it has the desired character.
     */
     
    InitSubFont(fontPtr, faceNum, &subFont);
    if (((ch < 256) && (subFont.familyPtr->isSymbolFont)) 
	    || (FontMapLookup(&subFont, ch) == 0)) {
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
 *-------------------------------------------------------------------------
 *
 * GetFamilyNum --
 *
 *	Determines if any physical screen font exists on the system with 
 *	the given family name.  If the family exists, then it should be
 *	possible to construct some physical screen font with that family
 *	name.
 *
 * Results:
 *	The return value is 0 if the specified font family does not exist,
 *	non-zero otherwise.  *faceNumPtr is filled with the unique face
 *	number that identifies the screen font, or 0 if the font family
 *	did not exist.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
GetFamilyNum(
    CONST char *faceName, 	/* UTF-8 name of font family to query. */
    short *faceNumPtr)		/* Filled with font number for above family. */
{
    FontNameMap *mapPtr;
    
    if (faceName != NULL) {
        for (mapPtr = gFontNameMap; mapPtr->utfName != NULL; mapPtr++) {
            if (strcasecmp(faceName, mapPtr->utfName) == 0) {
                *faceNumPtr = mapPtr->faceNum;
                return 1;
            }
        }
    }
    *faceNumPtr = 0;    
    return 0;
}

static int
GetFamilyOrAliasNum(
    CONST char *faceName, 	/* UTF-8 name of font family to query. */
    short *faceNumPtr)		/* Filled with font number for above family. */
{
    char **aliases;
    int i;
    
    if (GetFamilyNum(faceName, faceNumPtr) != 0) {
        return 1;
    }
    aliases = TkFontGetAliasList(faceName);
    if (aliases != NULL) {
        for (i = 0; aliases[i] != NULL; i++) {
            if (GetFamilyNum(aliases[i], faceNumPtr) != 0) {
		return 1;
	    }
	}
    }
    return 0;
}

/*
 *-------------------------------------------------------------------------
 *
 * GetUtfFaceName --
 *
 *	Given the native name for a Macintosh font (in which the name of
 *	the font is in the encoding of the font itself), return the UTF-8
 *	name that corresponds to that font.  The specified font name must
 *	refer to a font that actually exists on the machine.  
 *
 *	This function is used to obtain the UTF-8 name when querying the
 *	properties of a Macintosh font object.
 *
 * Results:
 *	The return value is a pointer to the UTF-8 of the specified font.
 *
 * Side effects:
 *	None.
 *
 *------------------------------------------------------------------------
 */
 
static Tk_Uid
GetUtfFaceName(
    StringPtr nativeName)	/* Pascal name for font in native encoding. */
{
    FontNameMap *mapPtr;
    
    for (mapPtr = gFontNameMap; mapPtr->utfName != NULL; mapPtr++) {
        if (pstrcmp(nativeName, mapPtr->nativeName) == 0) {
            return mapPtr->utfName;
        }
    }
    panic("GetUtfFaceName: unexpected nativeName");
    return NULL;
}

/*
 *------------------------------------------------------------------------
 *
 * GetFontEncoding --
 *
 *	Return a string that can be passed to Tcl_GetTextEncoding() and
 *	used to convert bytes from UTF-8 into the encoding  of the 
 *	specified font.
 *
 *	The desired encoding to use to convert the name of a symbolic 
 *	font into UTF-8 is macRoman, while the desired encoding to use
 *	to convert bytes in a symbolic font to UTF-8 is the corresponding
 *	symbolic encoding.  Due to this dual interpretatation of symbolic
 *	fonts, the caller can specify what type of encoding to return 
 *	should the specified font be symbolic.  
 *
 * Results:
 *	The return value is a string that specifies the font's encoding.
 *	If the font's encoding could not be identified, NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *------------------------------------------------------------------------
 */
  
static Tcl_Encoding
GetFontEncoding(
    int faceNum,		/* Macintosh font number. */
    int allowSymbol,		/* If non-zero, then the encoding string
    				 * for symbol fonts will be the corresponding
    				 * symbol encoding.  Otherwise, the encoding
    				 * string for symbol fonts will be 
    				 * "macRoman". */
    int *isSymbolPtr)		/* Filled with non-zero if this font is a
    				 * symbol font, 0 otherwise. */
{
    Str255 faceName;
    int script, lang;
    char *name;   
    
    if (allowSymbol != 0) {
        GetFontName(faceNum, faceName);
        if (pstrcasecmp(faceName, "\psymbol") == 0) {
            *isSymbolPtr = 1;
    	    return Tcl_GetEncoding(NULL, "symbol");
        }
        if (pstrcasecmp(faceName, "\pzapf dingbats") == 0) {
            *isSymbolPtr = 1;
            return Tcl_GetEncoding(NULL, "macDingbats");
        }
    }
    
    *isSymbolPtr = 0;
    
    script = FontToScript(faceNum);
    lang = GetScriptVariable(script, smScriptLang);
    name = NULL;
    if (script == smRoman) {
        name = TkFindStateString(romanMap, lang);
    } else if (script == smCyrillic) {
    	name = TkFindStateString(cyrillicMap, lang);
    }
    if (name == NULL) {
    	name = TkFindStateString(scriptMap, script);
    }
    return Tcl_GetEncoding(NULL, name);
}
