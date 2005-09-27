/* 
 * tkUnixInit.c --
 *
 *        This file contains Unix-specific interpreter initialization
 *        functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkMacOSXInt.h"

/*
 * The Init script (common to Windows and Unix platforms) is
 * defined in tkInitScript.h
 */
#include "tkInitScript.h"

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

Tcl_Encoding TkMacOSXCarbonEncoding = NULL;


/*
 *----------------------------------------------------------------------
 *
 * TkpInit --
 *
 *        Performs Mac-specific interpreter initialization related to the
 *      tk_library variable.
 *
 * Results:
 *        Returns a standard Tcl result.  Leaves an error message or result
 *        in the interp's result.
 *
 * Side effects:
 *        Sets "tk_library" Tcl variable, runs "tk.tcl" script.
 *
 *----------------------------------------------------------------------
 */

int
TkpInit(interp)
    Tcl_Interp *interp;
{
    char tkLibPath[1024];
    int result;
    static int menusInitialized = false;
    static int carbonEncodingInitialized = false;

    /* 
     * Since it is possible for TkInit to be called multiple times
     * and we don't want to do the menu initialization multiple times
     * we protect against doing it more than once.
     */

    if (menusInitialized == false) {
    	menusInitialized = true;
        Tk_MacOSXSetupTkNotifier();
        TkMacOSXInitAppleEvents(interp);
        TkMacOSXInitMenus(interp);
        TkMacOSXUseAntialiasedText(interp, TRUE);
    }
 
    if (carbonEncodingInitialized == false) {
	CFStringEncoding encoding;
	char *encodingStr = NULL;
	int  i;
	
	encoding = CFStringGetSystemEncoding();
	
	for (i = 0; scriptMap[i].strKey != NULL; i++) {
	    if (scriptMap[i].numKey == encoding) {
		encodingStr = scriptMap[i].strKey;
		break;
	    }
	}
	if (encodingStr == NULL) {
	    encodingStr = "macRoman";
	}
	
	TkMacOSXCarbonEncoding = Tcl_GetEncoding (NULL, encodingStr);
	if (TkMacOSXCarbonEncoding == NULL) {
	    TkMacOSXCarbonEncoding = Tcl_GetEncoding (NULL, NULL);
	}
    }
    
    /*
     * When Tk is in a framework, force tcl_findLibrary to look in the 
     * framework scripts directory.
     * FIXME: Should we come up with a more generic way of doing this?
     */
     
    result = Tcl_MacOSXOpenVersionedBundleResources(interp, 
    	    "com.tcltk.tklibrary", TK_VERSION, 1, 1024, tkLibPath);
     
    if (result != TCL_ERROR) {
        Tcl_SetVar(interp, "tk_library", tkLibPath, TCL_GLOBAL_ONLY);
    }
    
    return Tcl_Eval(interp, initScript);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetAppName --
 *
 *        Retrieves the name of the current application from a platform
 *        specific location.  For Unix, the application name is the tail
 *        of the path contained in the tcl variable argv0.
 *
 * Results:
 *        Returns the application name in the given Tcl_DString.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetAppName(interp, namePtr)
    Tcl_Interp *interp;
    Tcl_DString *namePtr;        /* A previously initialized Tcl_DString. */
{
    CONST char *p, *name;

    name = Tcl_GetVar(interp, "argv0", TCL_GLOBAL_ONLY);
    if ((name == NULL) || (*name == 0)) {
        name = "tk";
    } else {
        p = strrchr(name, '/');
        if (p != NULL) {
            name = p+1;
        }
    }
    Tcl_DStringAppend(namePtr, name, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayWarning --
 *
 *        This routines is called from Tk_Main to display warning
 *        messages that occur during startup.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Generates messages on stdout.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayWarning(msg, title)
    CONST char *msg;                  /* Message to be displayed. */
    CONST char *title;                /* Title of warning. */
{
    Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);
    if (errChannel) {
        Tcl_WriteChars(errChannel, title, -1);
        Tcl_WriteChars(errChannel, ": ", 2);
        Tcl_WriteChars(errChannel, msg, -1);
        Tcl_WriteChars(errChannel, "\n", 1);
    }
}
