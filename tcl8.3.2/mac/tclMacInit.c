/*
 * tclMacInit.c --
 *
 *	Contains the Mac-specific interpreter initialization functions.
 *
 * Copyright (c) 1995-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <AppleEvents.h>
#include <AEDataModel.h>
#include <AEObjects.h>
#include <AEPackObject.h>
#include <AERegistry.h>
#include <Files.h>
#include <Folders.h>
#include <Gestalt.h>
#include <TextUtils.h>
#include <Resources.h>
#include <Strings.h>
#include "tclInt.h"
#include "tclMacInt.h"
#include "tclPort.h"

/*
 * The following string is the startup script executed in new
 * interpreters.  It looks on the library path and in the resource fork for
 * a script "init.tcl" that is compatible with this version of Tcl.  The
 * init.tcl script does all of the real work of initialization.
 */
 
static char initCmd[] = "\
proc sourcePath {file} {\n\
  set dirs {}\n\
  foreach i $::auto_path {\n\
    set init [file join $i $file.tcl]\n\
    if {[catch {uplevel #0 [list source $init]}] == 0} {\n\
      return\n\
    }\n\
  }\n\
  if {[catch {uplevel #0 [list source -rsrc $file]}] == 0} {\n\
    return\n\
  }\n\
  rename sourcePath {}\n\
  set msg \"can't find $file resource or a usable $file.tcl file\n\"\n\
  append msg \"in the following directories:\n\"\n\
  append msg \"    $::auto_path\n\"\n\
  append msg \" perhaps you need to install Tcl or set your \n\"\n\
  append msg \"TCL_LIBRARY environment variable?\"\n\
  error $msg\n\
}\n\
if {[info exists env(EXT_FOLDER)]} {\n\
  lappend tcl_pkgPath [file join $env(EXT_FOLDER) {:Tool Command Language}]\n\
}\n\
if {[info exists tcl_pkgPath] == 0} {\n\
  set tcl_pkgPath {no extension folder}\n\
}\n\
sourcePath Init\n\
sourcePath Auto\n\
sourcePath Package\n\
sourcePath History\n\
sourcePath Word\n\
rename sourcePath {}";

/*
 * The following structures are used to map the script/language codes of a 
 * font to the name that should be passed to Tcl_GetEncoding() to obtain
 * the encoding for that font.  The set of numeric constants is fixed and 
 * defined by Apple.
 */
 
typedef struct Map {
    int numKey;
    char *strKey;
} Map;
 
static Map scriptMap[] = {
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

static Map romanMap[] = {
    {langCroatian,	"macCroatian"},
    {langSlovenian,	"macCroatian"},
    {langIcelandic,	"macIceland"},
    {langRomanian,	"macRomania"},
    {langTurkish,	"macTurkish"},
    {langGreek,		"macGreek"},
    {NULL,		NULL}
};

static Map cyrillicMap[] = {
    {langUkrainian,	"macUkraine"},
    {langBulgarian,	"macBulgaria"},
    {NULL,		NULL}
};

static int		GetFinderFont(int *finderID);


/*
 *----------------------------------------------------------------------
 *
 * GetFinderFont --
 *
 *	Gets the "views" font of the Macintosh Finder
 *
 * Results:
 *	Standard Tcl result, and sets finderID to the font family
 *      id for the current finder font.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
GetFinderFont(int *finderID)
{
    OSErr err = noErr;
    OSType finderPrefs, viewFont = 'vfnt';
    DescType returnType;
    Size returnSize;
    long result, sys8Mask = 0x0800;
    static AppleEvent outgoingAevt = {typeNull, NULL};
    AppleEvent returnAevt;
    AEAddressDesc fndrAddress;
    AEDesc nullContainer = {typeNull, NULL}, 
           tempDesc = {typeNull, NULL}, 
           tempDesc2 = {typeNull, NULL}, 
           finalDesc = {typeNull, NULL};
    const OSType finderSignature = 'MACS';
    
    
    if (outgoingAevt.descriptorType == typeNull) {
        if ((Gestalt(gestaltSystemVersion, &result) != noErr)
	        || (result >= sys8Mask)) {
            finderPrefs = 'pfrp';
        } else {
	    finderPrefs = 'pvwp';
        }
        
        AECreateDesc(typeApplSignature, &finderSignature,
		sizeof(finderSignature), &fndrAddress);
            
        err = AECreateAppleEvent(kAECoreSuite, kAEGetData, &fndrAddress, 
                kAutoGenerateReturnID, kAnyTransactionID, &outgoingAevt);
                
        AEDisposeDesc(&fndrAddress);
    
        /*
         * The structure is:
         * the property view font ('vfnt')
         *    of the property view preferences ('pvwp')
         *        of the Null Container (i.e. the Finder itself). 
         */
         
        AECreateDesc(typeType, &finderPrefs, sizeof(finderPrefs), &tempDesc);
        err = CreateObjSpecifier(typeType, &nullContainer, formPropertyID,
		&tempDesc, true, &tempDesc2);
        AECreateDesc(typeType, &viewFont, sizeof(viewFont), &tempDesc);
        err = CreateObjSpecifier(typeType, &tempDesc2, formPropertyID,
		&tempDesc, true, &finalDesc);
    
        AEPutKeyDesc(&outgoingAevt, keyDirectObject, &finalDesc);
        AEDisposeDesc(&finalDesc);
    }
             
    err = AESend(&outgoingAevt, &returnAevt, kAEWaitReply, kAEHighPriority,
	    kAEDefaultTimeout, NULL, NULL);
    if (err == noErr) {
        err = AEGetKeyPtr(&returnAevt, keyDirectObject, typeInteger, 
                &returnType, (void *) finderID, sizeof(int), &returnSize);
        if (err == noErr) {
            return TCL_OK;
        }
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclMacGetFontEncoding --
 *
 *	Determine the encoding of the specified font.  The encoding
 *	can be used to convert bytes from UTF-8 into the encoding of
 *	that font.
 *
 * Results:
 *	The return value is a string that specifies the font's encoding
 *	and that can be passed to Tcl_GetEncoding() to construct the
 *	encoding.  If the font's encoding could not be identified, NULL
 *	is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
 
char *
TclMacGetFontEncoding(
    int fontId)
{
    int script, lang;
    char *name;
    Map *mapPtr;
    
    script = FontToScript(fontId);    
    lang = GetScriptVariable(script, smScriptLang);
    name = NULL;
    if (script == smRoman) {
        for (mapPtr = romanMap; mapPtr->strKey != NULL; mapPtr++) {
            if (mapPtr->numKey == lang) {
                name = mapPtr->strKey;
                break;
            }
        }
    } else if (script == smCyrillic) {
        for (mapPtr = cyrillicMap; mapPtr->strKey != NULL; mapPtr++) {
            if (mapPtr->numKey == lang) {
                name = mapPtr->strKey;
                break;
            }
        }
    }
    if (name == NULL) {
        for (mapPtr = scriptMap; mapPtr->strKey != NULL; mapPtr++) {
            if (mapPtr->numKey == script) {
                name = mapPtr->strKey;
                break;
            }
        }
    }
    return name;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpInitPlatform --
 *
 *	Initialize all the platform-dependant things like signals and
 *	floating-point error handling.
 *
 *	Called at process initialization time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TclpInitPlatform()
{
    tclPlatform = TCL_PLATFORM_MAC;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpInitLibraryPath --
 *
 *	Initialize the library path at startup.  We have a minor
 *	metacircular problem that we don't know the encoding of the
 *	operating system but we may need to talk to operating system
 *	to find the library directories so that we know how to talk to
 *	the operating system.
 *
 *	We do not know the encoding of the operating system.
 *	We do know that the encoding is some multibyte encoding.
 *	In that multibyte encoding, the characters 0..127 are equivalent
 *	    to ascii.
 *
 *	So although we don't know the encoding, it's safe:
 *	    to look for the last colon character in a path in the encoding.
 *	    to append an ascii string to a path.
 *	    to pass those strings back to the operating system.
 *
 *	But any strings that we remembered before we knew the encoding of
 *	the operating system must be translated to UTF-8 once we know the
 *	encoding so that the rest of Tcl can use those strings.
 *
 *	This call sets the library path to strings in the unknown native
 *	encoding.  TclpSetInitialEncodings() will translate the library
 *	path from the native encoding to UTF-8 as soon as it determines
 *	what the native encoding actually is.
 *
 *	Called at process initialization time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TclpInitLibraryPath(argv0)
    CONST char *argv0;		/* Name of executable from argv[0] to main().
				 * Not used because we can determine the name
				 * by querying the module handle. */
{
    Tcl_Obj *objPtr, *pathPtr;
    char *str;
    Tcl_DString ds;
    
    TclMacCreateEnv();

    pathPtr = Tcl_NewObj();
    
    str = TclGetEnv("TCL_LIBRARY", &ds);
    if ((str != NULL) && (str[0] != '\0')) {
	/*
	 * If TCL_LIBRARY is set, search there.
	 */
	 
	objPtr = Tcl_NewStringObj(str, -1);
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	Tcl_DStringFree(&ds);
    }
    
    objPtr = TclGetLibraryPath();
    if (objPtr != NULL) {
        Tcl_ListObjAppendList(NULL, pathPtr, objPtr);
    }
    
    /*
     * lappend path [file join $env(EXT_FOLDER) \
     *      ":Tool Command Language:tcl[info version]"
     */

    str = TclGetEnv("EXT_FOLDER", &ds);
    if ((str != NULL) && (str[0] != '\0')) {
        objPtr = Tcl_NewStringObj(str, -1);
        if (str[strlen(str) - 1] != ':') {
            Tcl_AppendToObj(objPtr, ":", 1);
        }
        Tcl_AppendToObj(objPtr, "Tool Command Language:tcl" TCL_VERSION, -1);
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	Tcl_DStringFree(&ds);
    }    
    TclSetLibraryPath(pathPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpSetInitialEncodings --
 *
 *	Based on the locale, determine the encoding of the operating
 *	system and the default encoding for newly opened files.
 *
 *	Called at process initialization time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl library path is converted from native encoding to UTF-8.
 *
 *---------------------------------------------------------------------------
 */

void
TclpSetInitialEncodings()
{
    CONST char *encoding;
    Tcl_Obj *pathPtr;
    int fontId;
    
    fontId = 0;
    GetFinderFont(&fontId);
    encoding = TclMacGetFontEncoding(fontId);
    if (encoding == NULL) {
        encoding = "macRoman";
    }
    
    Tcl_SetSystemEncoding(NULL, encoding);
    
    /*
     * Until the system encoding was actually set, the library path was
     * actually in the native multi-byte encoding, and not really UTF-8
     * as advertised.  We cheated as follows:
     *
     * 1. It was safe to allow the Tcl_SetSystemEncoding() call to 
     * append the ASCII chars that make up the encoding's filename to 
     * the names (in the native encoding) of directories in the library 
     * path, since all Unix multi-byte encodings have ASCII in the
     * beginning.
     *
     * 2. To open the encoding file, the native bytes in the file name
     * were passed to the OS, without translating from UTF-8 to native,
     * because the name was already in the native encoding.
     *
     * Now that the system encoding was actually successfully set,
     * translate all the names in the library path to UTF-8.  That way,
     * next time we search the library path, we'll translate the names 
     * from UTF-8 to the system encoding which will be the native 
     * encoding.
     */

    pathPtr = TclGetLibraryPath();
    if (pathPtr != NULL) {
    	int i, objc;
	Tcl_Obj **objv;
	
	objc = 0;
	Tcl_ListObjGetElements(NULL, pathPtr, &objc, &objv);
	for (i = 0; i < objc; i++) {
	    int length;
	    char *string;
	    Tcl_DString ds;

	    string = Tcl_GetStringFromObj(objv[i], &length);
	    Tcl_ExternalToUtfDString(NULL, string, length, &ds);
	    Tcl_SetStringObj(objv[i], Tcl_DStringValue(&ds), 
		    Tcl_DStringLength(&ds));
	    Tcl_DStringFree(&ds);
	}
    }

    /*
     * Keep the iso8859-1 encoding preloaded.  The IO package uses it for
     * gets on a binary channel.
     */

    Tcl_GetEncoding(NULL, "iso8859-1"); 
}   

/*
 *---------------------------------------------------------------------------
 *
 * TclpSetVariables --
 *
 *	Performs platform-specific interpreter initialization related to
 *	the tcl_library and tcl_platform variables, and other platform-
 *	specific things.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets "tcl_library" and "tcl_platform" Tcl variables.
 *
 *----------------------------------------------------------------------
 */

void
TclpSetVariables(interp)
    Tcl_Interp *interp;
{
    long int gestaltResult;
    int minor, major, objc;
    Tcl_Obj **objv;
    char versStr[2 * TCL_INTEGER_SPACE];
    char *str;
    Tcl_Obj *pathPtr;
    Tcl_DString ds;

    str = "no library";
    pathPtr = TclGetLibraryPath();
    if (pathPtr != NULL) {
        objc = 0;
        Tcl_ListObjGetElements(NULL, pathPtr, &objc, &objv);
        if (objc > 0) {
            str = Tcl_GetStringFromObj(objv[0], NULL);
        }
    }
    Tcl_SetVar(interp, "tcl_library", str, TCL_GLOBAL_ONLY);
    
    if (pathPtr != NULL) {
        Tcl_SetVar2Ex(interp, "tcl_pkgPath", NULL, pathPtr, TCL_GLOBAL_ONLY);
    }
    
    Tcl_SetVar2(interp, "tcl_platform", "platform", "macintosh",
	    TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, "tcl_platform", "os", "MacOS", TCL_GLOBAL_ONLY);
    Gestalt(gestaltSystemVersion, &gestaltResult);
    major = (gestaltResult & 0x0000FF00) >> 8;
    minor = (gestaltResult & 0x000000F0) >> 4;
    sprintf(versStr, "%d.%d", major, minor);
    Tcl_SetVar2(interp, "tcl_platform", "osVersion", versStr, TCL_GLOBAL_ONLY);
#if GENERATINGPOWERPC
    Tcl_SetVar2(interp, "tcl_platform", "machine", "ppc", TCL_GLOBAL_ONLY);
#else
    Tcl_SetVar2(interp, "tcl_platform", "machine", "68k", TCL_GLOBAL_ONLY);
#endif

    /*
     * Copy USER or LOGIN environment variable into tcl_platform(user)
     * These are set by SystemVariables in tclMacEnv.c
     */

    Tcl_DStringInit(&ds);
    str = TclGetEnv("USER", &ds);
    if (str == NULL) {
	str = TclGetEnv("LOGIN", &ds);
	if (str == NULL) {
	    str = "";
	}
    }
    Tcl_SetVar2(interp, "tcl_platform", "user", str, TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&ds);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCheckStackSpace --
 *
 *	On a 68K Mac, we can detect if we are about to blow the stack.
 *	Called before an evaluation can happen when nesting depth is
 *	checked.
 *
 * Results:
 *	1 if there is enough stack space to continue; 0 if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpCheckStackSpace()
{
    return StackSpace() > TCL_MAC_STACK_THRESHOLD;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindVariable --
 *
 *	Locate the entry in environ for a given name.  On Unix and Macthis 
 *	routine is case sensitive, on Windows this matches mixed case.
 *
 * Results:
 *	The return value is the index in environ of an entry with the
 *	name "name", or -1 if there is no such entry.   The integer at
 *	*lengthPtr is filled in with the length of name (if a matching
 *	entry is found) or the length of the environ array (if no matching
 *	entry is found).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpFindVariable(name, lengthPtr)
    CONST char *name;		/* Name of desired environment variable
				 * (native). */
    int *lengthPtr;		/* Used to return length of name (for
				 * successful searches) or number of non-NULL
				 * entries in environ (for unsuccessful
				 * searches). */
{
    int i, result = -1;
    register CONST char *env, *p1, *p2;
    Tcl_DString envString;

    Tcl_DStringInit(&envString);
    for (i = 0, env = environ[i]; env != NULL; i++, env = environ[i]) {
	p1 = Tcl_ExternalToUtfDString(NULL, env, -1, &envString);
	p2 = name;

	for (; *p2 == *p1; p1++, p2++) {
	    /* NULL loop body. */
	}
	if ((*p1 == '=') && (*p2 == '\0')) {
	    *lengthPtr = p2 - name;
	    result = i;
	    goto done;
	}
	
	Tcl_DStringFree(&envString);
    }
    
    *lengthPtr = i;

    done:
    Tcl_DStringFree(&envString);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Init --
 *
 *	This procedure is typically invoked by Tcl_AppInit procedures
 *	to perform additional initialization for a Tcl interpreter,
 *	such as sourcing the "init.tcl" script.
 *
 * Results:
 *	Returns a standard Tcl completion code and sets the interp's result
 *	if there is an error.
 *
 * Side effects:
 *	Depends on what's in the init.tcl script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Init(
    Tcl_Interp *interp)		/* Interpreter to initialize. */
{
    Tcl_Obj *pathPtr;

    /*
     * For Macintosh applications the Init function may be contained in
     * the application resources.  If it exists we use it - otherwise we
     * look in the tcl_library directory.  Ditto for the history command.
     */

    pathPtr = TclGetLibraryPath();
    if (pathPtr == NULL) {
	pathPtr = Tcl_NewObj();
    }
    Tcl_SetVar2Ex(interp, "auto_path", NULL, pathPtr, TCL_GLOBAL_ONLY);
    return Tcl_Eval(interp, initCmd);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SourceRCFile --
 *
 *	This procedure is typically invoked by Tcl_Main or Tk_Main
 *	procedure to source an application specific rc file into the
 *	interpreter at startup time.  This will either source a file
 *	in the "tcl_rcFileName" variable or a TEXT resource in the
 *	"tcl_rcRsrcName" variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what's in the rc script.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SourceRCFile(
    Tcl_Interp *interp)		/* Interpreter to source rc file into. */
{
    Tcl_DString temp;
    char *fileName;
    Tcl_Channel errChannel;
    Handle h;

    fileName = Tcl_GetVar(interp, "tcl_rcFileName", TCL_GLOBAL_ONLY);

    if (fileName != NULL) {
        Tcl_Channel c;
	char *fullName;

        Tcl_DStringInit(&temp);
	fullName = Tcl_TranslateFileName(interp, fileName, &temp);
	if (fullName == NULL) {
	    /*
	     * Couldn't translate the file name (e.g. it referred to a
	     * bogus user or there was no HOME environment variable).
	     * Just do nothing.
	     */
	} else {

	    /*
	     * Test for the existence of the rc file before trying to read it.
	     */

            c = Tcl_OpenFileChannel(NULL, fullName, "r", 0);
            if (c != (Tcl_Channel) NULL) {
                Tcl_Close(NULL, c);
		if (Tcl_EvalFile(interp, fullName) != TCL_OK) {
		    errChannel = Tcl_GetStdChannel(TCL_STDERR);
		    if (errChannel) {
			Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
			Tcl_WriteChars(errChannel, "\n", 1);
		    }
		}
	    }
	}
        Tcl_DStringFree(&temp);
    }

    fileName = Tcl_GetVar(interp, "tcl_rcRsrcName", TCL_GLOBAL_ONLY);

    if (fileName != NULL) {
	c2pstr(fileName);
	h = GetNamedResource('TEXT', (StringPtr) fileName);
	p2cstr((StringPtr) fileName);
	if (h != NULL) {
	    if (Tcl_MacEvalResource(interp, fileName, 0, NULL) != TCL_OK) {
		errChannel = Tcl_GetStdChannel(TCL_STDERR);
		if (errChannel) {
		    Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
		    Tcl_WriteChars(errChannel, "\n", 1);
		}
	    }
	    Tcl_ResetResult(interp);
	    ReleaseResource(h);
	}
    }
}
