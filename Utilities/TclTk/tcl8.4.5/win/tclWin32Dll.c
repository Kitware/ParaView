/* 
 * tclWin32Dll.c --
 *
 *	This file contains the DLL entry point.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Scriptics Corporation.
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
static int platformId;		/* Running under NT, or 95/98? */

#if defined(HAVE_NO_SEH) && defined(TCL_MEM_DEBUG)
static void *INITIAL_ESP,
            *INITIAL_EBP,
            *INITIAL_HANDLER,
            *RESTORED_ESP,
            *RESTORED_EBP,
            *RESTORED_HANDLER;
#endif /* HAVE_NO_SEH && TCL_MEM_DEBUG */

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
    /* 
     * The three NULL function pointers will only be set when
     * Tcl_FindExecutable is called.  If you don't ever call that
     * function, the application will crash whenever WinTcl tries to call
     * functions through these null pointers.  That is not a bug in Tcl
     * -- Tcl_FindExecutable is obligatory in recent Tcl releases.
     */
    NULL,
    NULL,
    (int (__cdecl*)(CONST TCHAR *, struct _utimbuf *)) _utime,
    NULL,
    NULL,
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
    /* 
     * The three NULL function pointers will only be set when
     * Tcl_FindExecutable is called.  If you don't ever call that
     * function, the application will crash whenever WinTcl tries to call
     * functions through these null pointers.  That is not a bug in Tcl
     * -- Tcl_FindExecutable is obligatory in recent Tcl releases.
     */
    NULL,
    NULL,
    (int (__cdecl*)(CONST TCHAR *, struct _utimbuf *)) _wutime,
    NULL,
    NULL,
};

TclWinProcs *tclWinProcs;
static Tcl_Encoding tclWinTCharEncoding;

/*
 * The following declaration is for the VC++ DLL entry point.
 */

BOOL APIENTRY		DllMain(HINSTANCE hInst, DWORD reason, 
				LPVOID reserved);

/*
 * The following structure and linked list is to allow us to map between
 * volume mount points and drive letters on the fly (no Win API exists
 * for this).
 */
typedef struct MountPointMap {
    CONST WCHAR* volumeName;       /* Native wide string volume name */
    char driveLetter;              /* Drive letter corresponding to
                                    * the volume name. */
    struct MountPointMap* nextPtr; /* Pointer to next structure in list,
                                    * or NULL */
} MountPointMap;

/* 
 * This is the head of the linked list, which is protected by the
 * mutex which follows, for thread-enabled builds.
 */
MountPointMap *driveLetterLookup = NULL;
TCL_DECLARE_MUTEX(mountPointMap)

/* We will need this below */
extern Tcl_FSDupInternalRepProc NativeDupInternalRep;

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
 *	Initializes the tclPlatformId variable.
 *
 *----------------------------------------------------------------------
 */

void
TclWinInit(hInst)
    HINSTANCE hInst;		/* Library instance handle. */
{
    OSVERSIONINFO os;

    hInstance = hInst;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    platformId = os.dwPlatformId;

    /*
     * We no longer support Win32s, so just in case someone manages to
     * get a runtime there, make sure they know that.
     */

    if (platformId == VER_PLATFORM_WIN32s) {
	panic("Win32s is not a supported platform");	
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
 *	    VER_PLATFORM_WIN32s		Win32s on Windows 3.1. (not supported)
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
    int retval = 0;

    /*
     * We can recurse only if there is at least TCL_WIN_STACK_THRESHOLD
     * bytes of stack space left.  alloca() is cheap on windows; basically
     * it just subtracts from the stack pointer causing the OS to throw an
     * exception if the stack pointer is set below the bottom of the stack.
     */

#ifdef HAVE_NO_SEH
# ifdef TCL_MEM_DEBUG
    __asm__ __volatile__ (
            "movl %%esp,  %0" "\n\t"
            "movl %%ebp,  %1" "\n\t"
            "movl %%fs:0, %2" "\n\t"
            : "=m"(INITIAL_ESP),
              "=m"(INITIAL_EBP),
              "=r"(INITIAL_HANDLER) );
# endif /* TCL_MEM_DEBUG */

    __asm__ __volatile__ (
            "pushl %ebp" "\n\t"
            "pushl $__except_checkstackspace_handler" "\n\t"
            "pushl %fs:0" "\n\t"
            "movl  %esp, %fs:0");
#else
    __try {
#endif /* HAVE_NO_SEH */
#ifdef HAVE_ALLOCA_GCC_INLINE
    __asm__ __volatile__ (
            "movl  %0, %%eax" "\n\t"
            "call  __alloca" "\n\t"
            :
            : "i"(TCL_WIN_STACK_THRESHOLD)
            : "%eax");
#else
	alloca(TCL_WIN_STACK_THRESHOLD);
#endif /* HAVE_ALLOCA_GCC_INLINE */
	retval = 1;
#ifdef HAVE_NO_SEH
    __asm__ __volatile__ (
            "movl %%fs:0, %%esp" "\n\t"
            "jmp  checkstackspace_pop" "\n"
        "checkstackspace_reentry:" "\n\t"
            "movl %%fs:0, %%eax" "\n\t"
            "movl 0x8(%%eax), %%esp" "\n\t"
            "movl 0x8(%%esp), %%ebp" "\n"
        "checkstackspace_pop:" "\n\t"
            "movl (%%esp), %%eax" "\n\t"
            "movl %%eax, %%fs:0" "\n\t"
            "add  $12, %%esp" "\n\t"
            :
            :
            : "%eax");

# ifdef TCL_MEM_DEBUG
    __asm__ __volatile__ (
            "movl  %%esp,  %0" "\n\t"
            "movl  %%ebp,  %1" "\n\t"
            "movl  %%fs:0, %2" "\n\t"
            : "=m"(RESTORED_ESP),
              "=m"(RESTORED_EBP),
              "=r"(RESTORED_HANDLER) );

    if (INITIAL_ESP != RESTORED_ESP)
        panic("ESP restored incorrectly");
    if (INITIAL_EBP != RESTORED_EBP)
        panic("EBP restored incorrectly");
    if (INITIAL_HANDLER != RESTORED_HANDLER)
        panic("HANDLER restored incorrectly");
# endif /* TCL_MEM_DEBUG */
#else
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif /* HAVE_NO_SEH */

    /*
     * Avoid using control flow statements in the SEH guarded block!
     */
    return retval;
}
#ifdef HAVE_NO_SEH
static
__attribute__ ((cdecl))
EXCEPTION_DISPOSITION
_except_checkstackspace_handler(
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void *EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    void *DispatcherContext)
{
    __asm__ __volatile__ (
            "jmp checkstackspace_reentry");
    /* Nuke compiler warning about unused static function */
    _except_checkstackspace_handler(NULL, NULL, NULL, NULL);
    return 0; /* Function does not return */
}
#endif /* HAVE_NO_SEH */

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
 *	As well as this, we can also try to load in some additional
 *	procs which may/may not be present depending on the current
 *	Windows version (e.g. Win95 will not have the procs below).
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
	if (tclWinProcs->getFileAttributesExProc == NULL) {
	    HINSTANCE hInstance = LoadLibraryA("kernel32");
	    if (hInstance != NULL) {
	        tclWinProcs->getFileAttributesExProc = 
		  (BOOL (WINAPI *)(CONST TCHAR *, GET_FILEEX_INFO_LEVELS, 
		  LPVOID)) GetProcAddress(hInstance, "GetFileAttributesExW");
		tclWinProcs->createHardLinkProc = 
		  (BOOL (WINAPI *)(CONST TCHAR *, CONST TCHAR*, 
		  LPSECURITY_ATTRIBUTES)) GetProcAddress(hInstance, 
		  "CreateHardLinkW");
	        tclWinProcs->findFirstFileExProc = 
		  (HANDLE (WINAPI *)(CONST TCHAR*, UINT,
		  LPVOID, UINT, LPVOID, DWORD)) GetProcAddress(hInstance, 
		  "FindFirstFileExW");
	        tclWinProcs->getVolumeNameForVMPProc = 
		  (BOOL (WINAPI *)(CONST TCHAR*, TCHAR*, 
		  DWORD)) GetProcAddress(hInstance, 
		  "GetVolumeNameForVolumeMountPointW");
		FreeLibrary(hInstance);
	    }
	}
    } else {
	tclWinProcs = &asciiProcs;
	tclWinTCharEncoding = NULL;
	if (tclWinProcs->getFileAttributesExProc == NULL) {
	    HINSTANCE hInstance = LoadLibraryA("kernel32");
	    if (hInstance != NULL) {
		tclWinProcs->getFileAttributesExProc = 
		  (BOOL (WINAPI *)(CONST TCHAR *, GET_FILEEX_INFO_LEVELS, 
		  LPVOID)) GetProcAddress(hInstance, "GetFileAttributesExA");
		tclWinProcs->createHardLinkProc = 
		  (BOOL (WINAPI *)(CONST TCHAR *, CONST TCHAR*, 
		  LPSECURITY_ATTRIBUTES)) GetProcAddress(hInstance, 
		  "CreateHardLinkA");
		tclWinProcs->findFirstFileExProc = 
		  (HANDLE (WINAPI *)(CONST TCHAR*, UINT,
		  LPVOID, UINT, LPVOID, DWORD)) GetProcAddress(hInstance, 
		  "FindFirstFileExA");
		tclWinProcs->getVolumeNameForVMPProc = 
		  (BOOL (WINAPI *)(CONST TCHAR*, TCHAR*, 
		  DWORD)) GetProcAddress(hInstance, 
		  "GetVolumeNameForVolumeMountPointA");
		FreeLibrary(hInstance);
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TclWinResetInterfaceEncodings --
 *
 *	Called during finalization to free up any encodings we use.
 *	The tclWinProcs-> look up table is still ok to use after
 *	this call, provided no encoding conversion is required.
 *
 *      We also clean up any memory allocated in our mount point
 *      map which is used to follow certain kinds of symlinks.
 *      That code should never be used once encodings are taken
 *      down.
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
TclWinResetInterfaceEncodings()
{
    MountPointMap *dlIter, *dlIter2;
    if (tclWinTCharEncoding != NULL) {
	Tcl_FreeEncoding(tclWinTCharEncoding);
	tclWinTCharEncoding = NULL;
    }
    /* Clean up the mount point map */
    Tcl_MutexLock(&mountPointMap);
    dlIter = driveLetterLookup; 
    while (dlIter != NULL) {
	dlIter2 = dlIter->nextPtr;
	ckfree((char*)dlIter->volumeName);
	ckfree((char*)dlIter);
	dlIter = dlIter2;
    }
    Tcl_MutexUnlock(&mountPointMap);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclWinResetInterfaces --
 *
 *	Called during finalization to reset us to a safe state for reuse.
 *	After this call, it is best not to use the tclWinProcs-> look
 *	up table since it is likely to be different to what is expected.
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
TclWinResetInterfaces()
{
    tclWinProcs = &asciiProcs;
}

/*
 *--------------------------------------------------------------------
 *
 * TclWinDriveLetterForVolMountPoint
 *
 * Unfortunately, Windows provides no easy way at all to get hold
 * of the drive letter for a volume mount point, but we need that
 * information to understand paths correctly.  So, we have to 
 * build an associated array to find these correctly, and allow
 * quick and easy lookup from volume mount points to drive letters.
 * 
 * We assume here that we are running on a system for which the wide
 * character interfaces are used, which is valid for Win 2000 and WinXP
 * which are the only systems on which this function will ever be called.
 * 
 * Result: the drive letter, or -1 if no drive letter corresponds to
 * the given mount point.
 * 
 *--------------------------------------------------------------------
 */
char 
TclWinDriveLetterForVolMountPoint(CONST WCHAR *mountPoint)
{
    MountPointMap *dlIter, *dlPtr2;
    WCHAR Target[55];         /* Target of mount at mount point */
    WCHAR drive[4] = { L'A', L':', L'\\', L'\0' };
    
    /* 
     * Detect the volume mounted there.  Unfortunately, there is no
     * simple way to map a unique volume name to a DOS drive letter.  
     * So, we have to build an associative array.
     */
    
    Tcl_MutexLock(&mountPointMap);
    dlIter = driveLetterLookup; 
    while (dlIter != NULL) {
	if (wcscmp(dlIter->volumeName, mountPoint) == 0) {
	    /* 
	     * We need to check whether this information is
	     * still valid, since either the user or various
	     * programs could have adjusted the mount points on
	     * the fly.
	     */
	    drive[0] = L'A' + (dlIter->driveLetter - 'A');
	    /* Try to read the volume mount point and see where it points */
	    if ((*tclWinProcs->getVolumeNameForVMPProc)((TCHAR*)drive, 
					       (TCHAR*)Target, 55) != 0) {
		if (wcscmp((WCHAR*)dlIter->volumeName, Target) == 0) {
		    /* Nothing has changed */
		    Tcl_MutexUnlock(&mountPointMap);
		    return dlIter->driveLetter;
		}
	    }
	    /* 
	     * If we reach here, unfortunately, this mount point is
	     * no longer valid at all
	     */
	    if (driveLetterLookup == dlIter) {
		dlPtr2 = dlIter;
		driveLetterLookup = dlIter->nextPtr;
	    } else {
		for (dlPtr2 = driveLetterLookup; 
		     dlPtr2 != NULL; dlPtr2 = dlPtr2->nextPtr) {
		    if (dlPtr2->nextPtr == dlIter) {
			dlPtr2->nextPtr = dlIter->nextPtr;
			dlPtr2 = dlIter;
			break;
		    }
		}
	    }
	    /* Now dlPtr2 points to the structure to free */
	    ckfree((char*)dlPtr2->volumeName);
	    ckfree((char*)dlPtr2);
	    /* 
	     * Restart the loop --- we could try to be clever
	     * and continue half way through, but the logic is a 
	     * bit messy, so it's cleanest just to restart
	     */
	    dlIter = driveLetterLookup;
	    continue;
	}
	dlIter = dlIter->nextPtr;
    }
   
    /* We couldn't find it, so we must iterate over the letters */
    
    for (drive[0] = L'A'; drive[0] <= L'Z'; drive[0]++) {
	/* Try to read the volume mount point and see where it points */
	if ((*tclWinProcs->getVolumeNameForVMPProc)((TCHAR*)drive, 
					   (TCHAR*)Target, 55) != 0) {
	    int alreadyStored = 0;
	    for (dlIter = driveLetterLookup; dlIter != NULL; 
		 dlIter = dlIter->nextPtr) {
		if (wcscmp((WCHAR*)dlIter->volumeName, Target) == 0) {
		    alreadyStored = 1;
		    break;
		}
	    }
	    if (!alreadyStored) {
		dlPtr2 = (MountPointMap*) ckalloc(sizeof(MountPointMap));
		dlPtr2->volumeName = NativeDupInternalRep(Target);
		dlPtr2->driveLetter = 'A' + (drive[0] - L'A');
		dlPtr2->nextPtr = driveLetterLookup;
		driveLetterLookup  = dlPtr2;
	    }
	}
    }
    /* Try again */
    for (dlIter = driveLetterLookup; dlIter != NULL; 
					dlIter = dlIter->nextPtr) {
	if (wcscmp(dlIter->volumeName, mountPoint) == 0) {
	    Tcl_MutexUnlock(&mountPointMap);
	    return dlIter->driveLetter;
	}
    }
    /* 
     * The volume doesn't appear to correspond to a drive letter -- we
     * remember that fact and store '-1' so we don't have to look it
     * up each time.
     */
    dlPtr2 = (MountPointMap*) ckalloc(sizeof(MountPointMap));
    dlPtr2->volumeName = NativeDupInternalRep((ClientData)mountPoint);
    dlPtr2->driveLetter = -1;
    dlPtr2->nextPtr = driveLetterLookup;
    driveLetterLookup  = dlPtr2;
    Tcl_MutexUnlock(&mountPointMap);
    return -1;
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
