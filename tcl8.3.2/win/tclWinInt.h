/*
 * tclWinInt.h --
 *
 *	Declarations of Windows-specific shared variables and procedures.
 *
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCLWININT
#define _TCLWININT

#ifndef _TCLINT
#include "tclInt.h"
#endif
#ifndef _TCLPORT
#include "tclPort.h"
#endif

/*
 * The following specifies how much stack space TclpCheckStackSpace()
 * ensures is available.  TclpCheckStackSpace() is called by Tcl_EvalObj()
 * to help avoid overflowing the stack in the case of infinite recursion.
 */

#define TCL_WIN_STACK_THRESHOLD 0x2000

#ifdef BUILD_tcl
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * Some versions of Borland C have a define for the OSVERSIONINFO for
 * Win32s and for NT, but not for Windows 95.
 */

#ifndef VER_PLATFORM_WIN32_WINDOWS
#define VER_PLATFORM_WIN32_WINDOWS 1
#endif

/*
 * The following structure keeps track of whether we are using the 
 * multi-byte or the wide-character interfaces to the operating system.
 * System calls should be made through the following function table.
 */

typedef union {
    WIN32_FIND_DATAA a;
    WIN32_FIND_DATAW w;
} WIN32_FIND_DATAT;

typedef struct TclWinProcs {
    int useWide;

    BOOL (WINAPI *buildCommDCBProc)(CONST TCHAR *, LPDCB);
    TCHAR *(WINAPI *charLowerProc)(TCHAR *);
    BOOL (WINAPI *copyFileProc)(CONST TCHAR *, CONST TCHAR *, BOOL);
    BOOL (WINAPI *createDirectoryProc)(CONST TCHAR *, LPSECURITY_ATTRIBUTES);
    HANDLE (WINAPI *createFileProc)(CONST TCHAR *, DWORD, DWORD, 
	    LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    BOOL (WINAPI *createProcessProc)(CONST TCHAR *, TCHAR *, 
	    LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, 
	    LPVOID, CONST TCHAR *, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
    BOOL (WINAPI *deleteFileProc)(CONST TCHAR *);
    HANDLE (WINAPI *findFirstFileProc)(CONST TCHAR *, WIN32_FIND_DATAT *);
    BOOL (WINAPI *findNextFileProc)(HANDLE, WIN32_FIND_DATAT *);
    BOOL (WINAPI *getComputerNameProc)(WCHAR *, LPDWORD);
    DWORD (WINAPI *getCurrentDirectoryProc)(DWORD, WCHAR *);
    DWORD (WINAPI *getFileAttributesProc)(CONST TCHAR *);
    DWORD (WINAPI *getFullPathNameProc)(CONST TCHAR *, DWORD nBufferLength, 
	    WCHAR *, TCHAR **);
    DWORD (WINAPI *getModuleFileNameProc)(HMODULE, WCHAR *, int);
    DWORD (WINAPI *getShortPathNameProc)(CONST TCHAR *, WCHAR *, DWORD); 
    UINT (WINAPI *getTempFileNameProc)(CONST TCHAR *, CONST TCHAR *, UINT, 
	    WCHAR *);
    DWORD (WINAPI *getTempPathProc)(DWORD, WCHAR *);
    BOOL (WINAPI *getVolumeInformationProc)(CONST TCHAR *, WCHAR *, DWORD, 
	    LPDWORD, LPDWORD, LPDWORD, WCHAR *, DWORD);
    HINSTANCE (WINAPI *loadLibraryProc)(CONST TCHAR *);
    TCHAR (WINAPI *lstrcpyProc)(WCHAR *, CONST TCHAR *);
    BOOL (WINAPI *moveFileProc)(CONST TCHAR *, CONST TCHAR *);
    BOOL (WINAPI *removeDirectoryProc)(CONST TCHAR *);
    DWORD (WINAPI *searchPathProc)(CONST TCHAR *, CONST TCHAR *, 
	    CONST TCHAR *, DWORD, WCHAR *, TCHAR **);
    BOOL (WINAPI *setCurrentDirectoryProc)(CONST TCHAR *);
    BOOL (WINAPI *setFileAttributesProc)(CONST TCHAR *, DWORD);
} TclWinProcs;

EXTERN TclWinProcs *tclWinProcs;
EXTERN Tcl_Encoding tclWinTCharEncoding;

/*
 * Declarations of functions that are not accessible by way of the
 * stubs table.
 */

EXTERN void		TclWinInit(HINSTANCE hInst);

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#include "tclIntPlatDecls.h"

#endif	/* _TCLWININT */
