/* 
 * tkMacOSXInit.c --
 *
 *        This file contains Mac OS X -specific interpreter initialization
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

#include "tkMacOSXInt.h"

#include "tclInt.h" /* for TclGetStartupScript() & TclSetStartupScript() */

#include <sys/stat.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>

/*
 * The Init script (common to Windows and Unix platforms) is
 * defined in tkInitScript.h
 */
#include "tkInitScript.h"

/*
 * Define the following to 0 to not attempt to use an undocumented SPI
 * to notify the window server that an unbundled executable is a full
 * GUI application after loading Tk.
 */
#ifndef MAC_OSX_TK_USE_CPS_SPI
#define MAC_OSX_TK_USE_CPS_SPI 1
#endif

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
    {smRoman,    "macRoman"},
    {smJapanese,  "macJapan"},
    {smTradChinese,  "macChinese"},
    {smKorean,    "macKorean"},
    {smArabic,    "macArabic"},
    {smHebrew,    "macHebrew"},
    {smGreek,    "macGreek"},
    {smCyrillic,  "macCyrillic"},
    {smRSymbol,    "macRSymbol"},
    {smDevanagari,  "macDevanagari"},
    {smGurmukhi,  "macGurmukhi"},
    {smGujarati,  "macGujarati"},
    {smOriya,    "macOriya"},
    {smBengali,    "macBengali"},
    {smTamil,    "macTamil"},
    {smTelugu,    "macTelugu"},
    {smKannada,    "macKannada"},
    {smMalayalam,  "macMalayalam"},
    {smSinhalese,  "macSinhalese"},
    {smBurmese,    "macBurmese"},
    {smKhmer,    "macKhmer"},
    {smThai,    "macThailand"},
    {smLaotian,    "macLaos"},
    {smGeorgian,  "macGeorgia"},
    {smArmenian,  "macArmenia"},
    {smSimpChinese,  "macSimpChinese"},
    {smTibetan,    "macTIbet"},
    {smMongolian,  "macMongolia"},
    {smGeez,    "macEthiopia"},
    {smEastEurRoman,  "macCentEuro"},
    {smVietnamese,  "macVietnam"},
    {smExtArabic,  "macSindhi"},
    {0,     NULL}
};

Tcl_Encoding TkMacOSXCarbonEncoding = NULL;

/*
 * If the App is in an App package, then we want to add the Scripts
 * directory to the auto_path.      
 */
static char scriptPath[PATH_MAX + 1] = "";

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
    static char tkLibPath[PATH_MAX + 1];
    static int tkMacOSXInitialized = false;

    Tk_MacOSXSetupTkNotifier();

    /* 
     * Since it is possible for TkInit to be called multiple times
     * and we don't want to do the initialization multiple times
     * we protect against doing it more than once.
     */

    if (tkMacOSXInitialized == false) {
  CFStringEncoding encoding;
  char *encodingStr = NULL;
  int  i;

      tkMacOSXInitialized = true;

        TkMacOSXInitAppleEvents(interp);
        TkMacOSXInitCarbonEvents(interp);
        TkMacOSXInitMenus(interp);
        TkMacOSXUseAntialiasedText(interp, TRUE);
        TkMacOSXInitCGDrawing(interp, TRUE, 3);
  
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
        
        /*
         * When Tk is in a framework, force tcl_findLibrary to look in the 
         * framework scripts directory.
         * FIXME: Should we come up with a more generic way of doing this?
         */
         
#ifdef TK_FRAMEWORK
        if (Tcl_MacOSXOpenVersionedBundleResources(interp, 
                "com.tcltk.tklibrary", TK_FRAMEWORK_VERSION, 1, PATH_MAX, tkLibPath) != TCL_OK)
#endif
            {
            /* Tk.framework not found, check if resource file is open */
            Handle rsrc = Get1NamedResource('CURS', "\phand");
            if (rsrc) {
                ReleaseResource(rsrc);
            } else {
#ifndef __LP64__
                const struct mach_header *image;
                char *data = NULL;
                uint32_t size;
                int fd = -1;
                char fileName[L_tmpnam + 15];
                uint32_t i, n;

                /* Get resource data from __tk_rsrc section of tk library file */
                n = _dyld_image_count();
                for (i = 0; i < n; i++) {
                    image = _dyld_get_image_header(i);
                    if (image) {
                        data = getsectdatafromheader(image, SEG_TEXT, "__tk_rsrc", &size);
                        if (data) {
                            data += _dyld_get_image_vmaddr_slide(i);
                            break;
                        }
                    }
                }
                while (data) {
                    OSStatus err;
                    FSRef ref;
                    SInt16 refNum;
                    
                    /* Write resource data to temporary file and open it */
                    strcpy(fileName, P_tmpdir);
                    if (fileName[strlen(fileName) - 1] != '/') {
                        strcat(fileName, "/");
                    }
                    strcat(fileName, "tkMacOSX_XXXXXX");
                    fd = mkstemp(fileName);
                    if (fd == -1) break;
                    fcntl(fd, F_SETFD, FD_CLOEXEC);
                    if (write(fd, data, size) == -1) break;
                    err = FSPathMakeRef((unsigned char*)fileName, &ref, NULL);
                    if (err != noErr) break;
                    err = FSOpenResourceFile(&ref, 0, NULL, fsRdPerm, &refNum);
#ifdef TK_MAC_DEBUG
                    if (err != noErr) fprintf(stderr,"FSOpenResourceFile error %ld\n",err);
#endif
                    break;
                }
                if (fd != -1) {
                    unlink(fileName);
                    close(fd);
                }
#endif /* __LP64__ */
            }
        }
                 
        /*
         * If we don't have a TTY and stdin is a special character file of length 0,
         * (e.g. /dev/null, which is what Finder sets when double clicking Wish)
         * then use the Tk based console interpreter.
         */
    
        if (!isatty(0)) {
            struct stat st;
            if (fstat(0, &st) || (S_ISCHR(st.st_mode) && st.st_blocks == 0)) {
                Tk_InitConsoleChannels(interp);
                Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDIN));
                Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDOUT));
                Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDERR));
                /* Only show the console if we don't have a startup script */
                if (TclGetStartupScriptFileName() == NULL) {
                    Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
                }
                if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
                    return TCL_ERROR;
                }
            }
        }
#if MAC_OSX_TK_USE_CPS_SPI
        /*
         * If we are loaded into an executable that is not a bundled application, 
         * the window server does not let us come to the foreground.
         * For such an executable, we attempt to use an undocumented SPI to
         * notify the window server that we are now a full GUI application.
         */
        {
            /* Check whether we are a bundled executable: */
            int bundledExecutable = 0;
            CFBundleRef bundleRef = CFBundleGetMainBundle();
            CFURLRef bundleUrl = NULL;
            if (bundleRef) {
                bundleUrl = CFBundleCopyBundleURL(bundleRef);
            }
            if (bundleUrl) {
                /*
                 * A bundled executable is two levels down from its main bundle
                 * directory (e.g. Wish.app/Contents/MacOS/Wish), whereas
                 * an unbundled executable's main bundle directory is just
                 * the directory containing the executable.
                 * So to check whether we are bundled, we delete the last three
                 * path components of the executable's url and compare the
                 * resulting url with the main bundle url.
                 */
                int j = 3;
                CFURLRef url = CFBundleCopyExecutableURL(bundleRef);
                while (url && j--) {
                    CFURLRef parent = CFURLCreateCopyDeletingLastPathComponent(NULL, url);
                    CFRelease(url);
                    url = parent;
                }
                if (url) {
                    bundledExecutable = CFEqual(bundleUrl, url);
                    CFRelease(url);
                }
                CFRelease(bundleUrl);
            }
            
            /* If we are not a bundled executable, attempt to use the CPS SPI: */
            if (!bundledExecutable) {
                /*
                 * Load the CPS SPI symbol dynamically, so that we don't break
                 * if it every disappears or changes its name.
                 */
                TkMacOSXInitNamedSymbol(CoreGraphics, OSErr, \
                        CPSEnableForegroundOperation, ProcessSerialNumberPtr);
                if (CPSEnableForegroundOperation) {
                    ProcessSerialNumber psn = { 0, kCurrentProcess };
                    /*
                     * Let the window server know that we are a foregroundable app
                     */
                    CPSEnableForegroundOperation(&psn);
                }
            }
        }
#endif /* MAC_OSX_TK_USE_CPS_SPI */
    }

    if (tkLibPath[0] != '\0') {
        Tcl_SetVar(interp, "tk_library", tkLibPath, TCL_GLOBAL_ONLY);
    }

    if (scriptPath[0] != '\0') {
        Tcl_SetVar(interp, "auto_path", scriptPath,
                TCL_GLOBAL_ONLY|TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
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

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXDefaultStartupScript --
 *
 *
 *        On MacOS X, we look for a file in the Resources/Scripts
 *        directory called AppMain.tcl and if found, we set argv[1] to
 *        that, so that the rest of the code will find it, and add the
 *        Scripts folder to the auto_path.  If we don't find the startup
 *        script, we just bag it, assuming the user is starting up some
 *        other way.
 *       
 * Results:
 *        None.
 *
 * Side effects:
 *        TclSetStartupScriptFileName() called when AppMain.tcl found.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXDefaultStartupScript(void)
{
    CFBundleRef bundleRef;
    
    bundleRef = CFBundleGetMainBundle();
    
    if (bundleRef != NULL) {
        CFURLRef appMainURL;
        appMainURL = CFBundleCopyResourceURL(bundleRef, 
                CFSTR("AppMain"), 
                CFSTR("tcl"), 
                CFSTR("Scripts"));

        if (appMainURL != NULL) {
            CFURLRef scriptFldrURL;
            char startupScript[PATH_MAX + 1];
                            
            if (CFURLGetFileSystemRepresentation (appMainURL, true,
                    (unsigned char*) startupScript, PATH_MAX)) {
                TclSetStartupScriptFileName(startupScript);
                scriptFldrURL = CFURLCreateCopyDeletingLastPathComponent(
                        NULL, appMainURL);
                if (scriptFldrURL != NULL) {
                    CFURLGetFileSystemRepresentation(scriptFldrURL, 
                            true, (unsigned char*) scriptPath, PATH_MAX);
                    CFRelease(scriptFldrURL);
                }
            }
            CFRelease(appMainURL);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetNamedSymbol --
 *
 *
 *        Dynamically acquire address of a named symbol from a loaded
 *        dynamic library, so that we can use API that may not be
 *        available on all OS versions.
 *        If module is non-NULL and not the empty string, use twolevel
 *        namespace lookup.
 *       
 * Results:
 *        Address of given symbol or NULL if unavailable.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE void*
TkMacOSXGetNamedSymbol(const char* module, const char* symbol)
{
    NSSymbol nsSymbol = NULL;
    if (module && *module) {
  if(NSIsSymbolNameDefinedWithHint(symbol, module)) {
      nsSymbol = NSLookupAndBindSymbolWithHint(symbol, module);
  }
    } else {
        if(NSIsSymbolNameDefined(symbol)) {
            nsSymbol = NSLookupAndBindSymbol(symbol);
        }
    }
    if(nsSymbol) {
  return NSAddressOfSymbol(nsSymbol);
    } else {
        return NULL;
    }
}
