/* 
 * tclUnixInit.c --
 *
 *	Contains the Unix-specific interpreter initialization functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 * All rights reserved.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include <locale.h>
#if defined(__FreeBSD__)
#   include <floatingpoint.h>
#endif
#if defined(__bsdi__)
#   include <sys/param.h>
#   if _BSDI_VERSION > 199501
#	include <dlfcn.h>
#   endif
#endif

/*
 * The Init script (common to Windows and Unix platforms) is
 * defined in tkInitScript.h
 */
#include "tclInitScript.h"


/*
 * Default directory in which to look for Tcl library scripts.  The
 * symbol is defined by Makefile.
 */

static char defaultLibraryDir[sizeof(TCL_LIBRARY)+200] = TCL_LIBRARY;

/*
 * Directory in which to look for packages (each package is typically
 * installed as a subdirectory of this directory).  The symbol is
 * defined by Makefile.
 */

static char pkgPath[sizeof(TCL_PACKAGE_PATH)+200] = TCL_PACKAGE_PATH;

/*
 * The following table is used to map from Unix locale strings to
 * encoding files.
 */

typedef struct LocaleTable {
    CONST char *lang;
    CONST char *encoding;
} LocaleTable;

static CONST LocaleTable localeTable[] = {
    {"ja_JP.SJIS",	"shiftjis"},
    {"ja_JP.EUC",	"euc-jp"},
    {"ja_JP.JIS",	"iso2022-jp"},
    {"ja_JP.mscode",	"shiftjis"},
    {"ja_JP.ujis",	"euc-jp"},
    {"ja_JP",		"euc-jp"},
    {"Ja_JP",		"shiftjis"},
    {"Jp_JP",		"shiftjis"},
    {"japan",		"euc-jp"},
#ifdef hpux
    {"japanese",	"shiftjis"},	
    {"ja",		"shiftjis"},	
#else
    {"japanese",	"euc-jp"},
    {"ja",		"euc-jp"},
#endif
    {"japanese.sjis",	"shiftjis"},
    {"japanese.euc",	"euc-jp"},
    {"japanese-sjis",	"shiftjis"},
    {"japanese-ujis",	"euc-jp"},

    {"ko",              "euc-kr"},
    {"ko_KR",           "euc-kr"},
    {"ko_KR.EUC",       "euc-kr"},
    {"ko_KR.euc",       "euc-kr"},
    {"ko_KR.eucKR",     "euc-kr"},
    {"korean",          "euc-kr"},

    {"ru",		"iso8859-5"},		
    {"ru_RU",		"iso8859-5"},		
    {"ru_SU",		"iso8859-5"},		

    {"zh",		"cp936"},

    {NULL, NULL}
};

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
    tclPlatform = TCL_PLATFORM_UNIX;

    /*
     * The code below causes SIGPIPE (broken pipe) errors to
     * be ignored.  This is needed so that Tcl processes don't
     * die if they create child processes (e.g. using "exec" or
     * "open") that terminate prematurely.  The signal handler
     * is only set up when the first interpreter is created;
     * after this the application can override the handler with
     * a different one of its own, if it wants.
     */

#ifdef SIGPIPE
    (void) signal(SIGPIPE, SIG_IGN);
#endif /* SIGPIPE */

#ifdef __FreeBSD__
    fpsetround(FP_RN);
    fpsetmask(0L);
#endif

#if defined(__bsdi__) && (_BSDI_VERSION > 199501)
    /*
     * Find local symbols. Don't report an error if we fail.
     */
    (void) dlopen (NULL, RTLD_NOW);			/* INTL: Native. */
#endif
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
 *	    to look for the last slash character in a path in the encoding.
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
TclpInitLibraryPath(path)
CONST char *path;		/* Path to the executable in native 
				 * multi-byte encoding. */
{
#define LIBRARY_SIZE	    32
    Tcl_Obj *pathPtr, *objPtr;
    char *str;
    Tcl_DString buffer, ds;
    int pathc;
    char **pathv;
    char installLib[LIBRARY_SIZE], developLib[LIBRARY_SIZE];

    Tcl_DStringInit(&ds);
    pathPtr = Tcl_NewObj();

    /*
     * Initialize the substrings used when locating an executable.  The
     * installLib variable computes the path as though the executable
     * is installed.  The developLib computes the path as though the
     * executable is run from a develpment directory.
     */
     
    sprintf(installLib, "lib/tcl%s", TCL_VERSION);
    sprintf(developLib, "tcl%s/library",
	    ((TCL_RELEASE_LEVEL < 2) ? TCL_PATCH_LEVEL : TCL_VERSION));

    /*
     * Look for the library relative to default encoding dir.
     */

    str = Tcl_GetDefaultEncodingDir();
    if ((str != NULL) && (str[0] != '\0')) {
	objPtr = Tcl_NewStringObj(str, -1);
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
    }

    /*
     * Look for the library relative to the TCL_LIBRARY env variable.
     * If the last dirname in the TCL_LIBRARY path does not match the
     * last dirname in the installLib variable, use the last dir name
     * of installLib in addition to the orginal TCL_LIBRARY path.
     */

    str = getenv("TCL_LIBRARY");			/* INTL: Native. */
    Tcl_ExternalToUtfDString(NULL, str, -1, &buffer);
    str = Tcl_DStringValue(&buffer);

    if ((str != NULL) && (str[0] != '\0')) {
	/*
	 * If TCL_LIBRARY is set, search there.
	 */
	 
	objPtr = Tcl_NewStringObj(str, -1);
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);

	Tcl_SplitPath(str, &pathc, &pathv);
	if ((pathc > 0) && (strcasecmp(installLib + 4, pathv[pathc-1]) != 0)) {
	    /*
	     * If TCL_LIBRARY is set but refers to a different tcl
	     * installation than the current version, try fiddling with the
	     * specified directory to make it refer to this installation by
	     * removing the old "tclX.Y" and substituting the current
	     * version string.
	     */
	    
	    pathv[pathc - 1] = installLib + 4;
	    str = Tcl_JoinPath(pathc, pathv, &ds);
	    objPtr = Tcl_NewStringObj(str, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	ckfree((char *) pathv);
    }

    /*
     * Look for the library relative to the executable.  This algorithm
     * should be the same as the one in the tcl_findLibrary procedure.
     *
     * This code looks in the following directories:
     *
     *	<bindir>/../<installLib>
     *		(e.g. /usr/local/bin/../lib/tcl8.2)
     *	<bindir>/../../<installLib>
     *		(e.g. /usr/local/TclPro/solaris-sparc/bin/../../lib/tcl8.2)
     *	<bindir>/../library
     *		(e.g. /usr/src/tcl8.2/unix/../library)
     *	<bindir>/../../library
     *		(e.g. /usr/src/tcl8.2/unix/solaris-sparc/../../library)
     *	<bindir>/../../<developLib>
     *		(e.g. /usr/src/tcl8.2/unix/../../tcl8.2/library)
     *	<bindir>/../../../<devlopLib>
     *		(e.g. /usr/src/tcl8.2/unix/solaris-sparc/../../../tcl8.2/library)
     */
     
    if (path != NULL) {
	Tcl_SplitPath(path, &pathc, &pathv);
	if (pathc > 1) {
	    pathv[pathc - 2] = installLib;
	    path = Tcl_JoinPath(pathc - 1, pathv, &ds);
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 2) {
	    pathv[pathc - 3] = installLib;
	    path = Tcl_JoinPath(pathc - 2, pathv, &ds);
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 1) {
	    pathv[pathc - 2] = "library";
	    path = Tcl_JoinPath(pathc - 1, pathv, &ds);
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 2) {
	    pathv[pathc - 3] = "library";
	    path = Tcl_JoinPath(pathc - 2, pathv, &ds);
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 1) {
	    pathv[pathc - 3] = developLib;
	    path = Tcl_JoinPath(pathc - 2, pathv, &ds);
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 3) {
	    pathv[pathc - 4] = developLib;
	    path = Tcl_JoinPath(pathc - 3, pathv, &ds);
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	ckfree((char *) pathv);
    }

    /*
     * Finally, look for the library relative to the compiled-in path.
     * This is needed when users install Tcl with an exec-prefix that
     * is different from the prtefix.
     */
			      
    str = defaultLibraryDir;
    if (str[0] != '\0') {
        objPtr = Tcl_NewStringObj(str, -1);
        Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
    }

    TclSetLibraryPath(pathPtr);    
    Tcl_DStringFree(&buffer);
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
    int i;
    Tcl_Obj *pathPtr;
    char *langEnv;
    Tcl_DString ds;

    /*
     * Determine the current encoding from the LC_* or LANG environment
     * variables.  We previously used setlocale() to determine the locale,
     * but this does not work on some systems (e.g. Linux/i386 RH 5.0).
     */

    langEnv = getenv("LC_ALL");

    if (langEnv == NULL || langEnv[0] == '\0') {
	langEnv = getenv("LC_CTYPE");
    }
    if (langEnv == NULL || langEnv[0] == '\0') {
	langEnv = getenv("LANG");
    }
    if (langEnv == NULL || langEnv[0] == '\0') {
	langEnv = NULL;
    }

    encoding = NULL;
    if (langEnv != NULL) {
	for (i = 0; localeTable[i].lang != NULL; i++) {
	    if (strcmp(localeTable[i].lang, langEnv) == 0) {
		encoding = localeTable[i].encoding;
		break;
	    }
	}
	/*
	 * There was no mapping in the locale table.  If there is an
	 * encoding subfield, we can try to guess from that.
	 */

	if (encoding == NULL) {
	    char *p;
	    for (p = langEnv; *p != '\0'; p++) {
		if (*p == '.') {
		    p++;
		    break;
		}
	    }
	    if (*p != '\0') {
		Tcl_DString ds;
		Tcl_DStringInit(&ds);
		Tcl_DStringAppend(&ds, p, -1);

		encoding = Tcl_DStringValue(&ds);
		Tcl_UtfToLower(Tcl_DStringValue(&ds));
		if (Tcl_SetSystemEncoding(NULL, encoding) == TCL_OK) {
		    Tcl_DStringFree(&ds);
		    goto resetPath;
		}
		Tcl_DStringFree(&ds);
		encoding = NULL;
	    }
	}
    }
    if (encoding == NULL) {
	encoding = "iso8859-1";
    }

    Tcl_SetSystemEncoding(NULL, encoding);

    /*
     * Initialize the C library's locale subsystem.  This is required
     * for input methods to work properly on X11. Note that we need to
     * retore the initial "C" locale so that Tcl can parse numbers
     * properly.  The side effect of setting the default locale should be to
     * load any locale specific modules that are needed by X.
     */
 
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, setlocale(LC_ALL, NULL), -1);
    setlocale(LC_ALL, "");
    setlocale(LC_ALL, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);

    /*
     * In case the initial locale is not "C", ensure that the numeric
     * processing is done in "C" locale regardless.  This is needed because
     * Tcl relies on routines like strtod, but should not have locale
     * dependent behavior.
     */

    setlocale(LC_NUMERIC, "C");
 

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

    resetPath:
    pathPtr = TclGetLibraryPath();
    if (pathPtr != NULL) {
	int objc;
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
 *	Sets "tclDefaultLibrary", "tcl_pkgPath", and "tcl_platform" Tcl
 *	variables.
 *
 *----------------------------------------------------------------------
 */

void
TclpSetVariables(interp)
    Tcl_Interp *interp;
{
#ifndef NO_UNAME
    struct utsname name;
#endif
    int unameOK;
    char *user;
    Tcl_DString ds;

    Tcl_SetVar(interp, "tclDefaultLibrary", defaultLibraryDir, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tcl_pkgPath", pkgPath, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, "tcl_platform", "platform", "unix", TCL_GLOBAL_ONLY);
    unameOK = 0;
#ifndef NO_UNAME
    if (uname(&name) >= 0) {
	char *native;
	
	unameOK = 1;

	native = Tcl_ExternalToUtfDString(NULL, name.sysname, -1, &ds);
	Tcl_SetVar2(interp, "tcl_platform", "os", native, TCL_GLOBAL_ONLY);
	Tcl_DStringFree(&ds);
	
	/*
	 * The following code is a special hack to handle differences in
	 * the way version information is returned by uname.  On most
	 * systems the full version number is available in name.release.
	 * However, under AIX the major version number is in
	 * name.version and the minor version number is in name.release.
	 */

	if ((strchr(name.release, '.') != NULL)
		|| !isdigit(UCHAR(name.version[0]))) {	/* INTL: digit */
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.release,
		    TCL_GLOBAL_ONLY);
	} else {
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.version,
		    TCL_GLOBAL_ONLY);
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", ".",
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.release,
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);
	}
	Tcl_SetVar2(interp, "tcl_platform", "machine", name.machine,
		TCL_GLOBAL_ONLY);
    }
#endif
    if (!unameOK) {
	Tcl_SetVar2(interp, "tcl_platform", "os", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "tcl_platform", "osVersion", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "tcl_platform", "machine", "", TCL_GLOBAL_ONLY);
    }

    /*
     * Copy USER or LOGNAME environment variable into tcl_platform(user)
     */

    Tcl_DStringInit(&ds);
    user = TclGetEnv("USER", &ds);
    if (user == NULL) {
	user = TclGetEnv("LOGNAME", &ds);
	if (user == NULL) {
	    user = "";
	}
    }
    Tcl_SetVar2(interp, "tcl_platform", "user", user, TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&ds);

}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindVariable --
 *
 *	Locate the entry in environ for a given name.  On Unix this 
 *	routine is case sensetive, on Windows this matches mioxed case.
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
 *	to find and source the "init.tcl" script, which should exist
 *	somewhere on the Tcl library path.
 *
 * Results:
 *	Returns a standard Tcl completion code and sets the interp's
 *	result if there is an error.
 *
 * Side effects:
 *	Depends on what's in the init.tcl script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Init(interp)
    Tcl_Interp *interp;		/* Interpreter to initialize. */
{
    Tcl_Obj *pathPtr;

    if (tclPreInitScript != NULL) {
	if (Tcl_Eval(interp, tclPreInitScript) == TCL_ERROR) {
	    return (TCL_ERROR);
	};
    }
    
    pathPtr = TclGetLibraryPath();
    if (pathPtr == NULL) {
	pathPtr = Tcl_NewObj();
    }
    Tcl_SetVar2Ex(interp, "tcl_libPath", NULL, pathPtr, TCL_GLOBAL_ONLY);
    return Tcl_Eval(interp, initScript);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SourceRCFile --
 *
 *	This procedure is typically invoked by Tcl_Main of Tk_Main
 *	procedure to source an application specific rc file into the
 *	interpreter at startup time.
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
Tcl_SourceRCFile(interp)
    Tcl_Interp *interp;		/* Interpreter to source rc file into. */
{
    Tcl_DString temp;
    char *fileName;
    Tcl_Channel errChannel;

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
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCheckStackSpace --
 *
 *	Detect if we are about to blow the stack.  Called before an 
 *	evaluation can happen when nesting depth is checked.
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
    /*
     * This function is unimplemented on Unix platforms.
     */

    return 1;
}
