/* 

 * tclWin32Dll.c --
 *
 *	This file contains the DLL entry point which sets up the 32-to-16-bit
 *	thunking code for SynchSpawn if the library is running under Win32s.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"

/*
 * The following data structures are used when loading the thunking 
 * library for execing child processes under Win32s.
 */

typedef DWORD (WINAPI UT32PROC)(LPVOID lpBuff, DWORD dwUserDefined,
	LPVOID *lpTranslationList);

typedef BOOL (WINAPI UTREGISTER)(HANDLE hModule, LPCSTR SixteenBitDLL,
	LPCSTR InitName, LPCSTR ProcName, UT32PROC **ThirtyTwoBitThunk,
	FARPROC UT32Callback, LPVOID Buff);

typedef VOID (WINAPI UTUNREGISTER)(HANDLE hModule);

/* 
 * The following variables keep track of information about this DLL
 * on a per-instance basis.  Each time this DLL is loaded, it gets its own 
 * new data segment with its own copy of all static and global information.
 */

static HINSTANCE hInstance;	/* HINSTANCE of this DLL. */
static int platformId;		/* Running under NT, 95, or Win32s? */

/*
 * The following function tables are used to dispatch to either the
 * wide-character or multi-byte versions of the operating system calls,
 * depending on whether the Unicode calls are available.
 */

static TclWinProcs asciiProcs = {
    0,

    (BOOL (WINAPI *)(CONST TCHAR *, LPDCB)) BuildCommDCBA,
    (TCHAR *(WINAPI *)(TCHAR *)) CharLowerA,
    (BOOL (WINAPI *)(CONST TCHAR *, CONST TCHAR *, BOOL)) CopyFileA,
    (BOOL (WINAPI *)(CONST TCHAR *, LPSECURITY_ATTRIBUTES)) CreateDirectoryA,
    (HANDLE (WINAPI *)(CONST TCHAR *, DWORD, DWORD, SECURITY_ATTRIBUTES *, 
	    DWORD, DWORD, HANDLE)) CreateFileA,
    (BOOL (WINAPI *)(CONST TCHAR *, TCHAR *, LPSECURITY_ATTRIBUTES, 
	    LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, CONST TCHAR *, 
	    LPSTARTUPINFOA, LPPROCESS_INFORMATION)) CreateProcessA,
    (BOOL (WINAPI *)(CONST TCHAR *)) DeleteFileA,
    (HANDLE (WINAPI *)(CONST TCHAR *, WIN32_FIND_DATAT *)) FindFirstFileA,
    (BOOL (WINAPI *)(HANDLE, WIN32_FIND_DATAT *)) FindNextFileA,
    (BOOL (WINAPI *)(WCHAR *, LPDWORD)) GetComputerNameA,
    (DWORD (WINAPI *)(DWORD, WCHAR *)) GetCurrentDirectoryA,
    (DWORD (WINAPI *)(CONST TCHAR *)) GetFileAttributesA,
    (DWORD (WINAPI *)(CONST TCHAR *, DWORD nBufferLength, WCHAR *, 
	    TCHAR **)) GetFullPathNameA,
    (DWORD (WINAPI *)(HMODULE, WCHAR *, int)) GetModuleFileNameA,
    (DWORD (WINAPI *)(CONST TCHAR *, WCHAR *, DWORD)) GetShortPathNameA,
    (UINT (WINAPI *)(CONST TCHAR *, CONST TCHAR *, UINT uUnique, 
	    WCHAR *)) GetTempFileNameA,
    (DWORD (WINAPI *)(DWORD, WCHAR *)) GetTempPathA,
    (BOOL (WINAPI *)(CONST TCHAR *, WCHAR *, DWORD, LPDWORD, LPDWORD, LPDWORD,
	    WCHAR *, DWORD)) GetVolumeInformationA,
    (HINSTANCE (WINAPI *)(CONST TCHAR *)) LoadLibraryA,
    (TCHAR (WINAPI *)(WCHAR *, CONST TCHAR *)) lstrcpyA,
    (BOOL (WINAPI *)(CONST TCHAR *, CONST TCHAR *)) MoveFileA,
    (BOOL (WINAPI *)(CONST TCHAR *)) RemoveDirectoryA,
    (DWORD (WINAPI *)(CONST TCHAR *, CONST TCHAR *, CONST TCHAR *, DWORD, 
	    WCHAR *, TCHAR **)) SearchPathA,
    (BOOL (WINAPI *)(CONST TCHAR *)) SetCurrentDirectoryA,
    (BOOL (WINAPI *)(CONST TCHAR *, DWORD)) SetFileAttributesA,
};

static TclWinProcs unicodeProcs = {
    1,

    (BOOL (WINAPI *)(CONST TCHAR *, LPDCB)) BuildCommDCBW,
    (TCHAR *(WINAPI *)(TCHAR *)) CharLowerW,
    (BOOL (WINAPI *)(CONST TCHAR *, CONST TCHAR *, BOOL)) CopyFileW,
    (BOOL (WINAPI *)(CONST TCHAR *, LPSECURITY_ATTRIBUTES)) CreateDirectoryW,
    (HANDLE (WINAPI *)(CONST TCHAR *, DWORD, DWORD, SECURITY_ATTRIBUTES *, 
	    DWORD, DWORD, HANDLE)) CreateFileW,
    (BOOL (WINAPI *)(CONST TCHAR *, TCHAR *, LPSECURITY_ATTRIBUTES, 
	    LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, CONST TCHAR *, 
	    LPSTARTUPINFOA, LPPROCESS_INFORMATION)) CreateProcessW,
    (BOOL (WINAPI *)(CONST TCHAR *)) DeleteFileW,
    (HANDLE (WINAPI *)(CONST TCHAR *, WIN32_FIND_DATAT *)) FindFirstFileW,
    (BOOL (WINAPI *)(HANDLE, WIN32_FIND_DATAT *)) FindNextFileW,
    (BOOL (WINAPI *)(WCHAR *, LPDWORD)) GetComputerNameW,
    (DWORD (WINAPI *)(DWORD, WCHAR *)) GetCurrentDirectoryW,
    (DWORD (WINAPI *)(CONST TCHAR *)) GetFileAttributesW,
    (DWORD (WINAPI *)(CONST TCHAR *, DWORD nBufferLength, WCHAR *, 
	    TCHAR **)) GetFullPathNameW,
    (DWORD (WINAPI *)(HMODULE, WCHAR *, int)) GetModuleFileNameW,
    (DWORD (WINAPI *)(CONST TCHAR *, WCHAR *, DWORD)) GetShortPathNameW,
    (UINT (WINAPI *)(CONST TCHAR *, CONST TCHAR *, UINT uUnique, 
	    WCHAR *)) GetTempFileNameW,
    (DWORD (WINAPI *)(DWORD, WCHAR *)) GetTempPathW,
    (BOOL (WINAPI *)(CONST TCHAR *, WCHAR *, DWORD, LPDWORD, LPDWORD, LPDWORD, 
	    WCHAR *, DWORD)) GetVolumeInformationW,
    (HINSTANCE (WINAPI *)(CONST TCHAR *)) LoadLibraryW,
    (TCHAR (WINAPI *)(WCHAR *, CONST TCHAR *)) lstrcpyW,
    (BOOL (WINAPI *)(CONST TCHAR *, CONST TCHAR *)) MoveFileW,
    (BOOL (WINAPI *)(CONST TCHAR *)) RemoveDirectoryW,
    (DWORD (WINAPI *)(CONST TCHAR *, CONST TCHAR *, CONST TCHAR *, DWORD, 
	    WCHAR *, TCHAR **)) SearchPathW,
    (BOOL (WINAPI *)(CONST TCHAR *)) SetCurrentDirectoryW,
    (BOOL (WINAPI *)(CONST TCHAR *, DWORD)) SetFileAttributesW,
};

TclWinProcs *tclWinProcs;
static Tcl_Encoding tclWinTCharEncoding;

/*
 * The following declaration is for the VC++ DLL entry point.
 */

BOOL APIENTRY		DllMain(HINSTANCE hInst, DWORD reason, 
				LPVOID reserved);


#ifdef __WIN32__
#ifndef STATIC_BUILD


/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *	This wrapper function is used by Borland to invoke the
 *	initialization code for Tcl.  It simply calls the DllMain
 *	routine.
 *
 * Results:
 *	See DllMain.
 *
 * Side effects:
 *	See DllMain.
 *
 *----------------------------------------------------------------------
 */

BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;		/* Library instance handle. */
    DWORD reason;		/* Reason this function is being called. */
    LPVOID reserved;		/* Not used. */
{
    return DllMain(hInst, reason, reserved);
}

/*
 *----------------------------------------------------------------------
 *
 * DllMain --
 *
 *	This routine is called by the VC++ C run time library init
 *	code, or the DllEntryPoint routine.  It is responsible for
 *	initializing various dynamically loaded libraries.
 *
 * Results:
 *	TRUE on sucess, FALSE on failure.
 *
 * Side effects:
 *	Establishes 32-to-16 bit thunk and initializes sockets library.
 *
 *----------------------------------------------------------------------
 */
BOOL APIENTRY
DllMain(hInst, reason, reserved)
    HINSTANCE hInst;		/* Library instance handle. */
    DWORD reason;		/* Reason this function is being called. */
    LPVOID reserved;		/* Not used. */
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
	if (hInstance != NULL) {
	    /*
	     * Prevents DLL from being loaded multiple times under Win32s,
	     * since all copies of the DLL share the same data segment and
	     * Tcl isn't set up to handle that.  Under NT or 95, each time 
	     * the DLL is loaded, it gets its own private copy of the data 
	     * segment.
	     */

	    return FALSE;
	}

	TclWinInit(hInst);
	return TRUE;

    case DLL_PROCESS_DETACH:
	if (hInst == hInstance) {
	    Tcl_Finalize();
	}
	break;
    }

    return TRUE; 
}

#endif /* !STATIC_BUILD */
#endif /* __WIN32__ */

/*
 *----------------------------------------------------------------------
 *
 * TclWinSynchSpawn --
 *
 *	32-bit entry point to the 16-bit SynchSpawn code.
 *
 * Results:
 *	1 on success, 0 on failure.
 *
 * Side effects:
 *	Spawns a command and waits for it to complete.
 *
 *----------------------------------------------------------------------
 */
int 
TclWinSynchSpawn(void *args, int type, void **trans, Tcl_Pid *pidPtr)
{
    HINSTANCE hKernel;
    UTREGISTER *utRegisterProc;
    UTUNREGISTER *utUnRegisterProc;
    UT32PROC *ut32Proc;
    char buffer[] = "TCL16xx.DLL";
    int result;

    hKernel = LoadLibraryA("kernel32.dll");
    if (hKernel == NULL) {
	return 0;
    }

    /*
     * Load the Universal Thunking routines from kernel32.dll.
     */

    utRegisterProc = (UTREGISTER *) GetProcAddress(hKernel, "UTRegister");
    utUnRegisterProc = (UTUNREGISTER *) GetProcAddress(hKernel, "UTUnRegister");
    if ((utRegisterProc == NULL) || (utUnRegisterProc == NULL)) {
	result = 0;
	goto done;
    }

    /*
     * Construct the complete name of tcl16xx.dll.
     */

    buffer[5] = '0' + TCL_MAJOR_VERSION;
    buffer[6] = '0' + TCL_MINOR_VERSION;

    /*
     * Register the Tcl thunk.
     */

    if ((*utRegisterProc)(hInstance, buffer, NULL, "UTProc", &ut32Proc, 
	    NULL, NULL) == FALSE) {
	result = 0;
	goto done;
    }
    if (ut32Proc != NULL) {
	/*
	 * Invoke the thunk.
	 */

	*pidPtr = 0;
	(*ut32Proc)(args, type, trans);
	result = 1;
    } else {
	/*
	 * The 16-bit thunking DLL wasn't found.  Return error code that
	 * indicates this problem.
	 */

	result = 0;
    }
    (*utUnRegisterProc)(hInstance);

    done:
    FreeLibrary(hKernel);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclWinGetTclInstance --
 *
 *	Retrieves the global library instance handle.
 *
 * Results:
 *	Returns the global library instance handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HINSTANCE
TclWinGetTclInstance()
{
    return hInstance;
}

/*
 *----------------------------------------------------------------------
 *
 * TclWinInit --
 *
 *	This function initializes the internal state of the tcl library.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the 16-bit thunking library, and the tclPlatformId
 *	variable.
 *
 *----------------------------------------------------------------------
 */

void
TclWinInit(hInst)
    HINSTANCE hInst;		/* Library instance handle. */
{
    OSVERSIONINFO os;

    hInstance = hInst;
    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionEx(&os);
    platformId = os.dwPlatformId;

    /*
     * The following code stops Windows 3.x from automatically putting 
     * up Sharing Violation dialogs, e.g, when someone tries to
     * access a file that is locked or a drive with no disk in it.
     * Tcl already returns the appropriate error to the caller, and they 
     * can decide to put up their own dialog in response to that failure.  
     *
     * Under 95 and NT, the system doesn't automatically put up dialogs 
     * when the above operations fail.
     */

    if (platformId == VER_PLATFORM_WIN32s) {
	SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);
    }

    tclWinProcs = &asciiProcs;
}

/*
 *----------------------------------------------------------------------
 *
 * TclWinGetPlatformId --
 *
 *	Determines whether running under NT, 95, or Win32s, to allow 
 *	runtime conditional code.
 *
 * Results:
 *	The return value is one of:
 *	    VER_PLATFORM_WIN32s		Win32s on Windows 3.1. 
 *	    VER_PLATFORM_WIN32_WINDOWS	Win32 on Windows 95.
 *	    VER_PLATFORM_WIN32_NT	Win32 on Windows NT
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int		
TclWinGetPlatformId()
{
    return platformId;
}

/*
 *-------------------------------------------------------------------------
 *
 * TclWinNoBackslash --
 *
 *	We're always iterating through a string in Windows, changing the
 *	backslashes to slashes for use in Tcl.
 *
 * Results:
 *	All backslashes in given string are changed to slashes.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

char *
TclWinNoBackslash(
    char *path)			/* String to change. */
{
    char *p;

    for (p = path; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }
    return path;
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
     * We can recurse only if there is at least TCL_WIN_STACK_THRESHOLD
     * bytes of stack space left.  alloca() is cheap on windows; basically
     * it just subtracts from the stack pointer causing the OS to throw an
     * exception if the stack pointer is set below the bottom of the stack.
     */

    __try {
	alloca(TCL_WIN_STACK_THRESHOLD);
	return 1;
    } __except (1) {}

    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * TclWinGetPlatform --
 *
 *	This is a kludge that allows the test library to get access
 *	the internal tclPlatform variable.
 *
 * Results:
 *	Returns a pointer to the tclPlatform variable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclPlatformType *
TclWinGetPlatform()
{
    return &tclPlatform;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclWinSetInterfaces --
 *
 *	A helper proc that allows the test library to change the
 *	tclWinProcs structure to dispatch to either the wide-character
 *	or multi-byte versions of the operating system calls, depending
 *	on whether Unicode is the system encoding.
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
TclWinSetInterfaces(
    int wide)			/* Non-zero to use wide interfaces, 0
				 * otherwise. */
{
    Tcl_FreeEncoding(tclWinTCharEncoding);

    if (wide) {
	tclWinProcs = &unicodeProcs;
	tclWinTCharEncoding = Tcl_GetEncoding(NULL, "unicode");
    } else {
	tclWinProcs = &asciiProcs;
	tclWinTCharEncoding = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_WinUtfToTChar, Tcl_WinTCharToUtf --
 *
 *	Convert between UTF-8 and Unicode when running Windows NT or 
 *	the current ANSI code page when running Windows 95.
 *
 *	On Mac, Unix, and Windows 95, all strings exchanged between Tcl
 *	and the OS are "char" oriented.  We need only one Tcl_Encoding to
 *	convert between UTF-8 and the system's native encoding.  We use
 *	NULL to represent that encoding.
 *
 *	On NT, some strings exchanged between Tcl and the OS are "char"
 *	oriented, while others are in Unicode.  We need two Tcl_Encoding
 *	APIs depending on whether we are targeting a "char" or Unicode
 *	interface.  
 *
 *	Calling Tcl_UtfToExternal() or Tcl_ExternalToUtf() with an
 *	encoding of NULL should always used to convert between UTF-8
 *	and the system's "char" oriented encoding.  The following two
 *	functions are used in Windows-specific code to convert between
 *	UTF-8 and Unicode strings (NT) or "char" strings(95).  This saves
 *	you the trouble of writing the following type of fragment over and
 *	over:
 *
 *		if (running NT) {
 *		    encoding <- Tcl_GetEncoding("unicode");
 *		    nativeBuffer <- UtfToExternal(encoding, utfBuffer);
 *		    Tcl_FreeEncoding(encoding);
 *		} else {
 *		    nativeBuffer <- UtfToExternal(NULL, utfBuffer);
 *		}
 *
 *	By convention, in Windows a TCHAR is a character in the ANSI code
 *	page on Windows 95, a Unicode character on Windows NT.  If you
 *	plan on targeting a Unicode interfaces when running on NT and a
 *	"char" oriented interface while running on 95, these functions
 *	should be used.  If you plan on targetting the same "char"
 *	oriented function on both 95 and NT, use Tcl_UtfToExternal()
 *	with an encoding of NULL.
 *
 * Results:
 *	The result is a pointer to the string in the desired target
 *	encoding.  Storage for the result string is allocated in
 *	dsPtr; the caller must call Tcl_DStringFree() when the result
 *	is no longer needed.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TCHAR *
Tcl_WinUtfToTChar(string, len, dsPtr)
    CONST char *string;		/* Source string in UTF-8. */
    int len;			/* Source string length in bytes, or < 0 for
				 * strlen(). */
    Tcl_DString *dsPtr;		/* Uninitialized or free DString in which 
				 * the converted string is stored. */
{
    return (TCHAR *) Tcl_UtfToExternalDString(tclWinTCharEncoding, 
	    string, len, dsPtr);
}

char *
Tcl_WinTCharToUtf(string, len, dsPtr)
    CONST TCHAR *string;	/* Source string in Unicode when running
				 * NT, ANSI when running 95. */
    int len;			/* Source string length in bytes, or < 0 for
				 * platform-specific string length. */
    Tcl_DString *dsPtr;		/* Uninitialized or free DString in which 
				 * the converted string is stored. */
{
    return Tcl_ExternalToUtfDString(tclWinTCharEncoding, 
	    (CONST char *) string, len, dsPtr);
}
