/* 
 * tkAppInit.c --
 *
 *        Provides a default version of the Tcl_AppInit procedure for
 *        use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */
#include <pthread.h>
#include <sys/stat.h>
#include "tk.h"
#include "tclInt.h"
#include "locale.h"

#include <Carbon/Carbon.h>
#include "tkPort.h"
#include "tkMacOSX.h"
#include "tkMacOSXEvent.h"

/*
 * If the App is in an App package, then we want to add the Scripts
 * directory to the auto_path.  But we have to wait till after the
 * Tcl_Init is run, or it gets blown away.  This stores what we
 * figured out in main.
 */
 
char scriptPath[PATH_MAX + 1];

extern Tcl_Interp *gStdoutInterp;

#ifdef TK_TEST
extern int                Tktest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TK_TEST */

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *        This is the main program for the application.
 *
 * Results:
 *        None: Tk_Main never returns here, so this procedure never
 *        returns either.
 *
 * Side effects:
 *        Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;                        /* Number of command-line arguments. */
    char **argv;                /* Values of command-line arguments. */
{
    int textEncoding; /* 
                       * Variable used to take care of
                       * lazy font initialization
                       */
    CFBundleRef bundleRef;

    /*
     * The following #if block allows you to change the AppInit
     * function by using a #define of TCL_LOCAL_APPINIT instead
     * of rewriting this entire file.  The #if checks for that
     * #define and uses Tcl_AppInit if it doesn't exist.
     */
    
#ifndef TK_LOCAL_APPINIT
#define TK_LOCAL_APPINIT Tcl_AppInit    
#endif
    extern int TK_LOCAL_APPINIT _ANSI_ARGS_((Tcl_Interp *interp));

    scriptPath[0] = '\0';

    /*
     * The following #if block allows you to change how Tcl finds the startup
     * script, prime the library or encoding paths, fiddle with the argv,
     * etc., without needing to rewrite Tk_Main().  Note, if you use this
     * hook, then I won't do the CFBundle lookup, since if you are messing
     * around at this level, you probably don't want me to do this for you...
     */
    
#ifdef TK_LOCAL_MAIN_HOOK
    extern int TK_LOCAL_MAIN_HOOK _ANSI_ARGS_((int *argc, char ***argv));
    TK_LOCAL_MAIN_HOOK(&argc, &argv);
#else

    /*
     * On MacOS X, we look for a file in the Resources/Scripts directory
     * called AppMain.tcl and if found, we set argv[1] to that, so that
     * the rest of the code will find it, and add the Scripts folder to
     * the auto_path.  If we don't find the startup script, we just bag
     * it, assuming the user is starting up some other way.
     */
    
    bundleRef = CFBundleGetMainBundle();
    
    if (bundleRef != NULL) {
        CFURLRef appMainURL;
        appMainURL = CFBundleCopyResourceURL(bundleRef, 
                CFSTR("AppMain"), 
                CFSTR("tcl"), 
                CFSTR("Scripts"));

        if (appMainURL != NULL) {
            CFURLRef scriptFldrURL;
            char *startupScript = malloc(PATH_MAX + 1);
                            
            if (CFURLGetFileSystemRepresentation (appMainURL, true,
                    startupScript, PATH_MAX)) {
                TclSetStartupScriptFileName(startupScript);
                scriptFldrURL = CFBundleCopyResourceURL(bundleRef,
                        CFSTR("Scripts"),
                        NULL,
                        NULL);
                CFURLGetFileSystemRepresentation(scriptFldrURL, 
                        true, scriptPath, PATH_MAX);
                CFRelease(scriptFldrURL);
            } else {
                free(startupScript);
            }
            CFRelease(appMainURL);
        }
    }

#endif
    textEncoding=GetApplicationTextEncoding();
    
    /*
     * Now add the scripts folder to the auto_path.
     */
     
    Tk_Main(argc,argv,TK_LOCAL_APPINIT);
    return 0;                        /* Needed only to prevent compiler warning. */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *        This procedure performs application-specific initialization.
 *        Most applications, especially those that incorporate additional
 *        packages, will have their own version of this procedure.
 *
 * Results:
 *        Returns a standard Tcl completion code, and leaves an error
 *        message in the interp's result if an error occurs.
 *
 * Side effects:
 *        Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;                /* Interpreter for application. */
{        
    if (Tcl_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }    
    if (Tk_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);

    if (scriptPath[0] != '\0') {
        Tcl_SetVar(interp, "auto_path", scriptPath,
                TCL_GLOBAL_ONLY|TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
    }
    
#ifdef TK_TEST
    if (Tktest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tktest", Tktest_Init,
            (Tcl_PackageInitProc *) NULL);
#endif /* TK_TEST */

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
	    if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
		goto error;
	    }
	    /* Only show the console if we don't have a startup script */
	    if (TclGetStartupScriptPath() == NULL) {
		Tcl_Eval(interp, "console show");
	    }
	}
    }
    
    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    
    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */
     
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.wishrc", TCL_GLOBAL_ONLY);

    return TCL_OK;

    error:
    return TCL_ERROR;
}
