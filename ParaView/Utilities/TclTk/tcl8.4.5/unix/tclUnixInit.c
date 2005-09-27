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

#if defined(HAVE_CFBUNDLE)
#include <CoreFoundation/CoreFoundation.h>
#endif
#include "tclInt.h"
#include "tclPort.h"
#include <locale.h>
#ifdef HAVE_LANGINFO
#include <langinfo.h>
#endif
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

/* Used to store the encoding used for binary files */
static Tcl_Encoding binaryEncoding = NULL;
/* Has the basic library path encoding issue been fixed */
static int libraryPathEncodingFixed = 0;

/*
 * Tcl tries to use standard and homebrew methods to guess the right
 * encoding on the platform.  However, there is always a final fallback,
 * and this value is it.  Make sure it is a real Tcl encoding.
 */

#ifndef TCL_DEFAULT_ENCODING
#define TCL_DEFAULT_ENCODING "iso8859-1"
#endif

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
 * encoding files.  If HAVE_LANGINFO is defined, then this is a fallback
 * table when the result from nl_langinfo isn't a recognized encoding.
 * Otherwise this is the first list checked for a mapping from env
 * encoding to Tcl encoding name.
 */

typedef struct LocaleTable {
    CONST char *lang;
    CONST char *encoding;
} LocaleTable;

static CONST LocaleTable localeTable[] = {
#ifdef HAVE_LANGINFO
    {"gb2312-1980",	"gb2312"},
#ifdef __hpux
    {"SJIS",		"shiftjis"},
    {"eucjp",		"euc-jp"},
    {"euckr",		"euc-kr"},
    {"euctw",		"euc-cn"},
    {"greek8",		"cp869"},
    {"iso88591",	"iso8859-1"},
    {"iso88592",	"iso8859-2"},
    {"iso88595",	"iso8859-5"},
    {"iso88596",	"iso8859-6"},
    {"iso88597",	"iso8859-7"},
    {"iso88598",	"iso8859-8"},
    {"iso88599",	"iso8859-9"},
    {"iso885915",	"iso8859-15"},
    {"roman8",		"iso8859-1"},
    {"tis620",		"tis-620"},
    {"turkish8",	"cp857"},
    {"utf8",		"utf-8"},
#endif /* __hpux */
#endif /* HAVE_LANGINFO */

    {"ja_JP.SJIS",	"shiftjis"},
    {"ja_JP.EUC",	"euc-jp"},
    {"ja_JP.eucJP",     "euc-jp"},
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

#ifdef HAVE_CFBUNDLE
static int Tcl_MacOSXGetLibraryPath(Tcl_Interp *interp, int maxPathLen, char *tclLibPath);
#endif /* HAVE_CFBUNDLE */


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
    CONST char *str;
    Tcl_DString buffer, ds;
    int pathc;
    CONST char **pathv;
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
    sprintf(developLib, "tcl%s/library", TCL_PATCH_LEVEL);

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
     *	  (e.g. /usr/local/bin/../lib/tcl8.4)
     *	<bindir>/../../<installLib>
     *	  (e.g. /usr/local/TclPro/solaris-sparc/bin/../../lib/tcl8.4)
     *	<bindir>/../library
     *	  (e.g. /usr/src/tcl8.4.0/unix/../library)
     *	<bindir>/../../library
     *	  (e.g. /usr/src/tcl8.4.0/unix/solaris-sparc/../../library)
     *	<bindir>/../../<developLib>
     *	  (e.g. /usr/src/tcl8.4.0/unix/../../tcl8.4.0/library)
     *	<bindir>/../../../<developLib>
     *	  (e.g. /usr/src/tcl8.4.0/unix/solaris-sparc/../../../tcl8.4.0/library)
     */
     

     /*
      * The variable path holds an absolute path.  Take care not to
      * overwrite pathv[0] since that might produce a relative path.
      */

    if (path != NULL) {
	int i, origc;
	CONST char **origv;

	Tcl_SplitPath(path, &origc, &origv);
	pathc = 0;
	pathv = (CONST char **) ckalloc((unsigned int)(origc * sizeof(char *)));
	for (i=0; i< origc; i++) {
	    if (origv[i][0] == '.') {
		if (strcmp(origv[i], ".") == 0) {
		    /* do nothing */
		} else if (strcmp(origv[i], "..") == 0) {
		    pathc--;
		} else {
		    pathv[pathc++] = origv[i];
		}
	    } else {
		pathv[pathc++] = origv[i];
	    }
	}
	if (pathc > 2) {
	    str = pathv[pathc - 2];
	    pathv[pathc - 2] = installLib;
	    path = Tcl_JoinPath(pathc - 1, pathv, &ds);
	    pathv[pathc - 2] = str;
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 3) {
	    str = pathv[pathc - 3];
	    pathv[pathc - 3] = installLib;
	    path = Tcl_JoinPath(pathc - 2, pathv, &ds);
	    pathv[pathc - 3] = str;
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 2) {
	    str = pathv[pathc - 2];
	    pathv[pathc - 2] = "library";
	    path = Tcl_JoinPath(pathc - 1, pathv, &ds);
	    pathv[pathc - 2] = str;
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 3) {
	    str = pathv[pathc - 3];
	    pathv[pathc - 3] = "library";
	    path = Tcl_JoinPath(pathc - 2, pathv, &ds);
	    pathv[pathc - 3] = str;
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 3) {
	    str = pathv[pathc - 3];
	    pathv[pathc - 3] = developLib;
	    path = Tcl_JoinPath(pathc - 2, pathv, &ds);
	    pathv[pathc - 3] = str;
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	if (pathc > 4) {
	    str = pathv[pathc - 4];
	    pathv[pathc - 4] = developLib;
	    path = Tcl_JoinPath(pathc - 3, pathv, &ds);
	    pathv[pathc - 4] = str;
	    objPtr = Tcl_NewStringObj(path, Tcl_DStringLength(&ds));
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	    Tcl_DStringFree(&ds);
	}
	ckfree((char *) origv);
	ckfree((char *) pathv);
    }

    /*
     * Finally, look for the library relative to the compiled-in path.
     * This is needed when users install Tcl with an exec-prefix that
     * is different from the prtefix.
     */
			      
    {
#ifdef HAVE_CFBUNDLE
    char tclLibPath[MAXPATHLEN + 1];
    
    if (Tcl_MacOSXGetLibraryPath(NULL, MAXPATHLEN, tclLibPath) == TCL_OK) {
        str = tclLibPath;
    } else
#endif /* HAVE_CFBUNDLE */
    {
        str = defaultLibraryDir;
    }
    if (str[0] != '\0') {
        objPtr = Tcl_NewStringObj(str, -1);
        Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
    }
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
 *	Called at process initialization time, and part way through
 *	startup, we verify that the initial encodings were correctly
 *	setup.  Depending on Tcl's environment, there may not have been
 *	enough information first time through (above).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl library path is converted from native encoding to UTF-8,
 *	on the first call, and the encodings may be changed on first or
 *	second call.
 *
 *---------------------------------------------------------------------------
 */

void
TclpSetInitialEncodings()
{
    if (libraryPathEncodingFixed == 0) {
	CONST char *encoding = NULL;
	int i, setSysEncCode = TCL_ERROR;
	Tcl_Obj *pathPtr;

	/*
	 * Determine the current encoding from the LC_* or LANG environment
	 * variables.  We previously used setlocale() to determine the locale,
	 * but this does not work on some systems (e.g. Linux/i386 RH 5.0).
	 */
#ifdef HAVE_LANGINFO
	if (setlocale(LC_CTYPE, "") != NULL) {
	    Tcl_DString ds;

	    /*
	     * Use a DString so we can overwrite it in name compatability
	     * checks below.
	     */

	    Tcl_DStringInit(&ds);
	    encoding = Tcl_DStringAppend(&ds, nl_langinfo(CODESET), -1);

	    Tcl_UtfToLower(Tcl_DStringValue(&ds));
#ifdef HAVE_LANGINFO_DEBUG
	    fprintf(stderr, "encoding '%s'", encoding);
#endif
	    if (encoding[0] == 'i' && encoding[1] == 's' && encoding[2] == 'o'
		    && encoding[3] == '-') {
		char *p, *q;
		/* need to strip '-' from iso-* encoding */
		for(p = Tcl_DStringValue(&ds)+3, q = Tcl_DStringValue(&ds)+4;
		    *p; *p++ = *q++);
	    } else if (encoding[0] == 'i' && encoding[1] == 'b'
		    && encoding[2] == 'm' && encoding[3] >= '0'
		    && encoding[3] <= '9') {
		char *p, *q;
		/* if langinfo reports "ibm*" we should use "cp*" */
		p = Tcl_DStringValue(&ds);
		*p++ = 'c'; *p++ = 'p';
		for(q = p+1; *p ; *p++ = *q++);
	    } else if ((*encoding == '\0')
		    || !strcmp(encoding, "ansi_x3.4-1968")) {
		/* Use iso8859-1 for empty or 'ansi_x3.4-1968' encoding */
		encoding = "iso8859-1";
	    }
#ifdef HAVE_LANGINFO_DEBUG
	    fprintf(stderr, " ?%s?", encoding);
#endif
	    setSysEncCode = Tcl_SetSystemEncoding(NULL, encoding);
	    if (setSysEncCode != TCL_OK) {
		/*
		 * If this doesn't return TCL_OK, the encoding returned by
		 * nl_langinfo or as we translated it wasn't accepted.  Do
		 * this fallback check.  If this fails, we will enter the
		 * old fallback below.
		 */

		for (i = 0; localeTable[i].lang != NULL; i++) {
		    if (strcmp(localeTable[i].lang, encoding) == 0) {
			setSysEncCode = Tcl_SetSystemEncoding(NULL,
				localeTable[i].encoding);
			break;
		    }
		}
	    }
#ifdef HAVE_LANGINFO_DEBUG
	    fprintf(stderr, " => '%s'\n", encoding);
#endif
	    Tcl_DStringFree(&ds);
	}
#ifdef HAVE_LANGINFO_DEBUG
	else {
	    fprintf(stderr, "setlocale returned NULL\n");
	}
#endif
#endif /* HAVE_LANGINFO */

	if (setSysEncCode != TCL_OK) {
	    /*
	     * Classic fallback check.  This tries a homebrew algorithm to
	     * determine what encoding should be used based on env vars.
	     */
	    char *langEnv = getenv("LC_ALL");
	    encoding = NULL;

	    if (langEnv == NULL || langEnv[0] == '\0') {
		langEnv = getenv("LC_CTYPE");
	    }
	    if (langEnv == NULL || langEnv[0] == '\0') {
		langEnv = getenv("LANG");
	    }
	    if (langEnv == NULL || langEnv[0] == '\0') {
		langEnv = NULL;
	    }

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
			encoding = Tcl_DStringAppend(&ds, p, -1);

			Tcl_UtfToLower(Tcl_DStringValue(&ds));
			setSysEncCode = Tcl_SetSystemEncoding(NULL, encoding);
			if (setSysEncCode != TCL_OK) {
			    encoding = NULL;
			}
			Tcl_DStringFree(&ds);
		    }
		}
#ifdef HAVE_LANGINFO_DEBUG
		fprintf(stderr, "encoding fallback check '%s' => '%s'\n",
			langEnv, encoding);
#endif
	    }
	    if (setSysEncCode != TCL_OK) {
		if (encoding == NULL) {
		    encoding = TCL_DEFAULT_ENCODING;
		}

		Tcl_SetSystemEncoding(NULL, encoding);
	    }

	    /*
	     * Initialize the C library's locale subsystem.  This is required
	     * for input methods to work properly on X11.  We only do this for
	     * LC_CTYPE because that's the necessary one, and we don't want to
	     * affect LC_TIME here.  The side effect of setting the default
	     * locale should be to load any locale specific modules that are
	     * needed by X.  [BUG: 5422 3345 4236 2522 2521].
	     * In HAVE_LANGINFO, this call is already done above.
	     */
#ifndef HAVE_LANGINFO
	    setlocale(LC_CTYPE, "");
#endif
	}

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

	libraryPathEncodingFixed = 1;
    }
    
    /* This is only ever called from the startup thread */
    if (binaryEncoding == NULL) {
	/*
	 * Keep the iso8859-1 encoding preloaded.  The IO package uses
	 * it for gets on a binary channel.
	 */
	binaryEncoding = Tcl_GetEncoding(NULL, "iso8859-1");
    }
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
    CONST char *user;
    Tcl_DString ds;

#ifdef HAVE_CFBUNDLE
    char tclLibPath[MAXPATHLEN + 1];
    
    if (Tcl_MacOSXGetLibraryPath(interp, MAXPATHLEN, tclLibPath) == TCL_OK) {
        CONST char *str;
        Tcl_DString ds;
        CFBundleRef bundleRef;

        Tcl_SetVar(interp, "tclDefaultLibrary", tclLibPath, 
                TCL_GLOBAL_ONLY);
        Tcl_SetVar(interp, "tcl_pkgPath", tclLibPath,
                TCL_GLOBAL_ONLY);
        Tcl_SetVar(interp, "tcl_pkgPath", " ",
                TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
        str = TclGetEnv("DYLD_FRAMEWORK_PATH", &ds);
        if ((str != NULL) && (str[0] != '\0')) {
            char *p = Tcl_DStringValue(&ds);
            /* convert DYLD_FRAMEWORK_PATH from colon to space separated */
            do {
                if(*p == ':') *p = ' ';
            } while (*p++);
            Tcl_SetVar(interp, "tcl_pkgPath", Tcl_DStringValue(&ds),
                    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
            Tcl_SetVar(interp, "tcl_pkgPath", " ",
                    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
            Tcl_DStringFree(&ds);
        }
        if ((bundleRef = CFBundleGetMainBundle())) {
            CFURLRef frameworksURL;
            Tcl_StatBuf statBuf;
            if((frameworksURL = CFBundleCopyPrivateFrameworksURL(bundleRef))) {
                if(CFURLGetFileSystemRepresentation(frameworksURL, TRUE,
                            tclLibPath, MAXPATHLEN) &&
                        ! TclOSstat(tclLibPath, &statBuf) &&
                        S_ISDIR(statBuf.st_mode)) {
                    Tcl_SetVar(interp, "tcl_pkgPath", tclLibPath,
                            TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
                    Tcl_SetVar(interp, "tcl_pkgPath", " ",
                            TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
                }
                CFRelease(frameworksURL);
            }
            if((frameworksURL = CFBundleCopySharedFrameworksURL(bundleRef))) {
                if(CFURLGetFileSystemRepresentation(frameworksURL, TRUE,
                            tclLibPath, MAXPATHLEN) &&
                        ! TclOSstat(tclLibPath, &statBuf) &&
                        S_ISDIR(statBuf.st_mode)) {
                    Tcl_SetVar(interp, "tcl_pkgPath", tclLibPath,
                            TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
                    Tcl_SetVar(interp, "tcl_pkgPath", " ",
                            TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
                }
                CFRelease(frameworksURL);
            }
        }
        Tcl_SetVar(interp, "tcl_pkgPath", pkgPath,
                TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
    } else
#endif /* HAVE_CFBUNDLE */
    {
        Tcl_SetVar(interp, "tclDefaultLibrary", defaultLibraryDir, 
                TCL_GLOBAL_ONLY);
        Tcl_SetVar(interp, "tcl_pkgPath", pkgPath, TCL_GLOBAL_ONLY);
    }

#ifdef DJGPP
    Tcl_SetVar2(interp, "tcl_platform", "platform", "dos", TCL_GLOBAL_ONLY);
#else
    Tcl_SetVar2(interp, "tcl_platform", "platform", "unix", TCL_GLOBAL_ONLY);
#endif
    unameOK = 0;
#ifndef NO_UNAME
    if (uname(&name) >= 0) {
	CONST char *native;
	
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
 *	routine is case sensetive, on Windows this matches mixed case.
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
    CONST char *fileName;
    Tcl_Channel errChannel;

    fileName = Tcl_GetVar(interp, "tcl_rcFileName", TCL_GLOBAL_ONLY);

    if (fileName != NULL) {
        Tcl_Channel c;
	CONST char *fullName;

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

#ifdef HAVE_CFBUNDLE
/*
 *----------------------------------------------------------------------
 *
 * Tcl_MacOSXGetLibraryPath --
 *
 *	If we have a bundle structure for the Tcl installation,
 *	then check there first to see if we can find the libraries
 *	there.
 *
 * Results:
 *	TCL_OK if we have found the tcl library; TCL_ERROR otherwise.
 *
 * Side effects:
 *	Same as for Tcl_MacOSXOpenVersionedBundleResources.
 *
 *----------------------------------------------------------------------
 */
static int Tcl_MacOSXGetLibraryPath(Tcl_Interp *interp, int maxPathLen, char *tclLibPath)
{
    int foundInFramework = TCL_ERROR;
    if (strcmp(defaultLibraryDir, "@TCL_IN_FRAMEWORK@") == 0) {
	foundInFramework = Tcl_MacOSXOpenVersionedBundleResources(interp, 
	    "com.tcltk.tcllibrary", TCL_VERSION, 0, maxPathLen, tclLibPath);
    }
    return foundInFramework;
}
#endif /* HAVE_CFBUNDLE */

