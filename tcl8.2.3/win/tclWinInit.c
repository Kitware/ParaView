/* 
 * tclWinInit.c --
 *
 *	Contains the Windows-specific interpreter initialization functions.
 *
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * All rights reserved.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"
#include <winreg.h>
#include <winnt.h>
#include <winbase.h>

/*
 * The following macro can be defined at compile time to specify
 * the root of the Tcl registry keys.
 */
 
#ifndef TCL_REGISTRY_KEY
#define TCL_REGISTRY_KEY "Software\\Scriptics\\Tcl\\" TCL_VERSION
#endif

/*
 * The following declaration is a workaround for some Microsoft brain damage.
 * The SYSTEM_INFO structure is different in various releases, even though the
 * layout is the same.  So we overlay our own structure on top of it so we
 * can access the interesting slots in a uniform way.
 */

typedef struct {
    WORD wProcessorArchitecture;
    WORD wReserved;
} OemId;

/*
 * The following macros are missing from some versions of winnt.h.
 */

#ifndef PROCESSOR_ARCHITECTURE_INTEL
#define PROCESSOR_ARCHITECTURE_INTEL 0
#endif
#ifndef PROCESSOR_ARCHITECTURE_MIPS
#define PROCESSOR_ARCHITECTURE_MIPS  1
#endif
#ifndef PROCESSOR_ARCHITECTURE_ALPHA
#define PROCESSOR_ARCHITECTURE_ALPHA 2
#endif
#ifndef PROCESSOR_ARCHITECTURE_PPC
#define PROCESSOR_ARCHITECTURE_PPC   3
#endif
#ifndef PROCESSOR_ARCHITECTURE_UNKNOWN
#define PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF
#endif

/*
 * The following arrays contain the human readable strings for the Windows
 * platform and processor values.
 */


#define NUMPLATFORMS 3
static char* platforms[NUMPLATFORMS] = {
    "Win32s", "Windows 95", "Windows NT"
};

#define NUMPROCESSORS 4
static char* processors[NUMPROCESSORS] = {
    "intel", "mips", "alpha", "ppc"
};

/*
 * Thread id used for asynchronous notification from signal handlers.
 */

static DWORD mainThreadId;

/*
 * The Init script (common to Windows and Unix platforms) is
 * defined in tkInitScript.h
 */

#include "tclInitScript.h"

static void		AppendEnvironment(Tcl_Obj *listPtr, CONST char *lib);
static void		AppendDllPath(Tcl_Obj *listPtr, HMODULE hModule,
			    CONST char *lib);
static void		AppendRegistry(Tcl_Obj *listPtr, CONST char *lib);
static int		ToUtf(CONST WCHAR *wSrc, char *dst);

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
    tclPlatform = TCL_PLATFORM_WINDOWS;

    /*
     * The following code stops Windows 3.X and Windows NT 3.51 from 
     * automatically putting up Sharing Violation dialogs, e.g, when 
     * someone tries to access a file that is locked or a drive with no 
     * disk in it.  Tcl already returns the appropriate error to the 
     * caller, and they can decide to put up their own dialog in response 
     * to that failure.  
     *
     * Under 95 and NT 4.0, this is a NOOP because the system doesn't 
     * automatically put up dialogs when the above operations fail.
     */

    SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

    /*
     * Save the id of the first thread to intialize the Tcl library.  This
     * thread will be used to handle notifications from async event
     * procedures.  This is not strictly correct.  A better solution involves
     * using a designated "main" notifier that is kept up to date as threads
     * come and go.
     */

    mainThreadId = GetCurrentThreadId();

#ifdef STATIC_BUILD
    /*
     * If we are in a statically linked executable, then we need to
     * explicitly initialize the Windows function tables here since
     * DllMain() will not be invoked.
     */

    TclWinInit(GetModuleHandle(NULL));
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpInitLibraryPath --
 *
 *	Initialize the library path at startup.  
 *
 *	This call sets the library path to strings in UTF-8. Any 
 *	pre-existing library path information is assumed to have been 
 *	in the native multibyte encoding.
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
    CONST char *path;		/* Potentially dirty UTF string that is */
				/* the path to the executable name.     */
{
#define LIBRARY_SIZE	    32
    Tcl_Obj *pathPtr, *objPtr;
    char *str;
    Tcl_DString ds;
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
    sprintf(developLib, "../tcl%s/library",
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

    AppendEnvironment(pathPtr, installLib);

    /*
     * Look for the library relative to the DLL.  Only use the installLib
     * because in practice, the DLL is always installed.
     */

    AppendDllPath(pathPtr, TclWinGetTclInstance(), installLib);
    

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

    TclSetLibraryPath(pathPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendEnvironment --
 *
 *	Append the value of the TCL_LIBRARY environment variable onto the
 *	path pointer.  If the env variable points to another version of
 *	tcl (e.g. "tcl7.6") also append the path to this version (e.g.,
 *	"tcl7.6/../tcl8.2")
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
AppendEnvironment(
    Tcl_Obj *pathPtr,
    CONST char *lib)
{
    int pathc;
    WCHAR wBuf[MAX_PATH];
    char buf[MAX_PATH * TCL_UTF_MAX];
    Tcl_Obj *objPtr;
    char *str;
    Tcl_DString ds;
    char **pathv;

    /*
     * The "L" preceeding the TCL_LIBRARY string is used to tell VC++
     * that this is a unicode string.
     */
    
    if (GetEnvironmentVariableW(L"TCL_LIBRARY", wBuf, MAX_PATH) == 0) {
        buf[0] = '\0';
	GetEnvironmentVariableA("TCL_LIBRARY", buf, MAX_PATH);
    } else {
	ToUtf(wBuf, buf);
    }

    if (buf[0] != '\0') {
	objPtr = Tcl_NewStringObj(buf, -1);
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);

	TclWinNoBackslash(buf);
	Tcl_SplitPath(buf, &pathc, &pathv);

	/* 
	 * The lstrcmpi() will work even if pathv[pathc - 1] is random
	 * UTF-8 chars because I know lib is ascii.
	 */

	if ((pathc > 0) && (lstrcmpiA(lib + 4, pathv[pathc - 1]) != 0)) {
	    /*
	     * TCL_LIBRARY is set but refers to a different tcl
	     * installation than the current version.  Try fiddling with the
	     * specified directory to make it refer to this installation by
	     * removing the old "tclX.Y" and substituting the current
	     * version string.
	     */
	    
	    pathv[pathc - 1] = (char *) (lib + 4);
	    Tcl_DStringInit(&ds);
	    str = Tcl_JoinPath(pathc, pathv, &ds);
	    objPtr = Tcl_NewStringObj(str, Tcl_DStringLength(&ds));
	    Tcl_DStringFree(&ds);
	} else {
	    objPtr = Tcl_NewStringObj(buf, -1);
	}
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	ckfree((char *) pathv);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendDllPath --
 *
 *	Append a path onto the path pointer that tries to locate the Tcl
 *	library relative to the location of the Tcl DLL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void 
AppendDllPath(
    Tcl_Obj *pathPtr, 
    HMODULE hModule,
    CONST char *lib)
{
    WCHAR wName[MAX_PATH + LIBRARY_SIZE];
    char name[(MAX_PATH + LIBRARY_SIZE) * TCL_UTF_MAX];

    if (GetModuleFileNameW(hModule, wName, MAX_PATH) == 0) {
	GetModuleFileNameA(hModule, name, MAX_PATH);
    } else {
	ToUtf(wName, name);
    }
    if (lib != NULL) {
	char *end, *p;

	end = strrchr(name, '\\');
	*end = '\0';
	p = strrchr(name, '\\');
	if (p != NULL) {
	    end = p;
	}
	*end = '\\';
	strcpy(end + 1, lib);
    }
    TclWinNoBackslash(name);
    Tcl_ListObjAppendElement(NULL, pathPtr, Tcl_NewStringObj(name, -1));
}

/*
 *---------------------------------------------------------------------------
 *
 * ToUtf --
 *
 *	Convert a char string to a UTF string.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
ToUtf(
    CONST WCHAR *wSrc,
    char *dst)
{
    char *start;

    start = dst;
    while (*wSrc != '\0') {
	dst += Tcl_UniCharToUtf(*wSrc, dst);
	wSrc++;
    }
    *dst = '\0';
    return dst - start;
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
    char buf[4 + TCL_INTEGER_SPACE];
    int platformId;
    Tcl_Obj *pathPtr;

    platformId = TclWinGetPlatformId();

    TclWinSetInterfaces(platformId == VER_PLATFORM_WIN32_NT);

    wsprintfA(buf, "cp%d", GetACP());
    Tcl_SetSystemEncoding(NULL, buf);

    if (platformId != VER_PLATFORM_WIN32_NT) {
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
    }

    /*
     * Keep this encoding preloaded.  The IO package uses it for gets on a
     * binary channel.  
     */

    encoding = "iso8859-1";
    Tcl_GetEncoding(NULL, encoding);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpSetVariables --
 *
 *	Performs platform-specific interpreter initialization related to
 *	the tcl_platform and env variables, and other platform-specific
 *	things.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets "tclDefaultLibrary", "tcl_platform", and "env(HOME)" Tcl
 *	variables.
 *
 *----------------------------------------------------------------------
 */

void
TclpSetVariables(interp)
    Tcl_Interp *interp;		/* Interp to initialize. */	
{	    
    char *ptr;
    char buffer[TCL_INTEGER_SPACE * 2];
    SYSTEM_INFO sysInfo;
    OemId *oemId;
    OSVERSIONINFOA osInfo;
    Tcl_DString ds;

    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    GetVersionExA(&osInfo);

    oemId = (OemId *) &sysInfo;
    if (osInfo.dwPlatformId == VER_PLATFORM_WIN32s) {
	/*
	 * Since Win32s doesn't support GetSystemInfo, we use a default value.
	 */

	oemId->wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    } else {
	GetSystemInfo(&sysInfo);
    }

    /*
     * Initialize the tclDefaultLibrary variable from the registry.
     */

    Tcl_SetVar(interp, "tclDefaultLibrary", "", TCL_GLOBAL_ONLY);

    /*
     * Define the tcl_platform array.
     */

    Tcl_SetVar2(interp, "tcl_platform", "platform", "windows",
	    TCL_GLOBAL_ONLY);
    if (osInfo.dwPlatformId < NUMPLATFORMS) {
	Tcl_SetVar2(interp, "tcl_platform", "os",
		platforms[osInfo.dwPlatformId], TCL_GLOBAL_ONLY);
    }
    wsprintfA(buffer, "%d.%d", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
    Tcl_SetVar2(interp, "tcl_platform", "osVersion", buffer, TCL_GLOBAL_ONLY);
    if (oemId->wProcessorArchitecture < NUMPROCESSORS) {
	Tcl_SetVar2(interp, "tcl_platform", "machine",
		processors[oemId->wProcessorArchitecture],
		TCL_GLOBAL_ONLY);
    }

#ifdef _DEBUG
    /*
     * The existence of the "debug" element of the tcl_platform array indicates
     * that this particular Tcl shell has been compiled with debug information.
     * Using "info exists tcl_platform(debug)" a Tcl script can direct the 
     * interpreter to load debug versions of DLLs with the load command.
     */

    Tcl_SetVar2(interp, "tcl_platform", "debug", "1",
	    TCL_GLOBAL_ONLY);
#endif

    /*
     * Set up the HOME environment variable from the HOMEDRIVE & HOMEPATH
     * environment variables, if necessary.
     */

    Tcl_DStringInit(&ds);
    ptr = Tcl_GetVar2(interp, "env", "HOME", TCL_GLOBAL_ONLY);
    if (ptr == NULL) {
	ptr = Tcl_GetVar2(interp, "env", "HOMEDRIVE", TCL_GLOBAL_ONLY);
	if (ptr != NULL) {
	    Tcl_DStringAppend(&ds, ptr, -1);
	}
	ptr = Tcl_GetVar2(interp, "env", "HOMEPATH", TCL_GLOBAL_ONLY);
	if (ptr != NULL) {
	    Tcl_DStringAppend(&ds, ptr, -1);
	}
	if (Tcl_DStringLength(&ds) > 0) {
	    Tcl_SetVar2(interp, "env", "HOME", Tcl_DStringValue(&ds),
		    TCL_GLOBAL_ONLY);
	} else {
	    Tcl_SetVar2(interp, "env", "HOME", "c:\\", TCL_GLOBAL_ONLY);
	}
    }

    /*
     * Initialize the user name from the environment first, since this is much
     * faster than asking the system.
     */

    Tcl_DStringSetLength(&ds, 100);
    if (TclGetEnv("USERNAME", &ds) == NULL) {
	if (GetUserName(Tcl_DStringValue(&ds), &Tcl_DStringLength(&ds)) == 0) {
	    Tcl_DStringSetLength(&ds, 0);
	}
    }
    Tcl_SetVar2(interp, "tcl_platform", "user", Tcl_DStringValue(&ds),
	    TCL_GLOBAL_ONLY);
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
				 * (UTF-8). */
    int *lengthPtr;		/* Used to return length of name (for
				 * successful searches) or number of non-NULL
				 * entries in environ (for unsuccessful
				 * searches). */
{
    int i, length, result = -1;
    register CONST char *env, *p1, *p2;
    char *envUpper, *nameUpper;
    Tcl_DString envString;

    /*
     * Convert the name to all upper case for the case insensitive
     * comparison.
     */

    length = strlen(name);
    nameUpper = (char *) ckalloc((unsigned) length+1);
    memcpy((VOID *) nameUpper, (VOID *) name, (size_t) length+1);
    Tcl_UtfToUpper(nameUpper);
    
    Tcl_DStringInit(&envString);
    for (i = 0, env = environ[i]; env != NULL; i++, env = environ[i]) {
	/*
	 * Chop the env string off after the equal sign, then Convert
	 * the name to all upper case, so we do not have to convert
	 * all the characters after the equal sign.
	 */
	
	envUpper = Tcl_ExternalToUtfDString(NULL, env, -1, &envString);
	p1 = strchr(envUpper, '=');
	if (p1 == NULL) {
	    continue;
	}
	length = p1 - envUpper;
	Tcl_DStringSetLength(&envString, length+1);
	Tcl_UtfToUpper(envUpper);

	p1 = envUpper;
	p2 = nameUpper;
	for (; *p2 == *p1; p1++, p2++) {
	    /* NULL loop body. */
	}
	if ((*p1 == '=') && (*p2 == '\0')) {
	    *lengthPtr = length;
	    result = i;
	    goto done;
	}
	
	Tcl_DStringFree(&envString);
    }
    
    *lengthPtr = i;

    done:
    Tcl_DStringFree(&envString);
    ckfree(nameUpper);
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
 * TclpAsyncMark --
 *
 *	Wake up the main thread from a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends a message to the main thread.
 *
 *----------------------------------------------------------------------
 */

void
TclpAsyncMark(async)
    Tcl_AsyncHandler async;		/* Token for handler. */
{
    /*
     * Need a way to kick the Windows event loop and tell it to go look at
     * asynchronous events.
     */

    PostThreadMessage(mainThreadId, WM_USER, 0, 0);
}
