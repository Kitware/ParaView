/*
 * tclWinPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between Windows and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 *
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCLWINPORT
#define _TCLWINPORT

#ifndef _TCLINT
#   include "tclInt.h"
#endif

#ifdef CHECK_UNICODE_CALLS

#define _UNICODE
#define UNICODE

#define __TCHAR_DEFINED
typedef float *_TCHAR;

#define _TCHAR_DEFINED
typedef float *TCHAR;

#endif

/*
 *---------------------------------------------------------------------------
 * The following sets of #includes and #ifdefs are required to get Tcl to
 * compile under the windows compilers.
 *---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <io.h>
#include <malloc.h>
#include <process.h>
#include <signal.h>
#include <string.h>

/*
 * Need to block out these includes for building extensions with MetroWerks
 * compiler for Win32.
 */

#ifndef __MWERKS__
#include <sys/stat.h>
#include <sys/timeb.h>
#endif

#include <tchar.h>
#include <time.h>
#include <winsock2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#ifdef BUILD_tcl
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * Define EINPROGRESS in terms of WSAEINPROGRESS.
 */

#ifndef	EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

/*
 * If ENOTSUP is not defined, define it to a value that will never occur.
 */

#ifndef ENOTSUP
#define	ENOTSUP		-1030507
#endif

/*
 * The following defines redefine the Windows Socket errors as
 * BSD errors so Tcl_PosixError can do the right thing.
 */

#ifndef EWOULDBLOCK
#define EWOULDBLOCK             EAGAIN
#endif
#ifndef EALREADY
#define EALREADY	149	/* operation already in progress */
#endif
#ifndef ENOTSOCK
#define ENOTSOCK	95	/* Socket operation on non-socket */
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ	96	/* Destination address required */
#endif
#ifndef EMSGSIZE
#define EMSGSIZE	97	/* Message too long */
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE	98	/* Protocol wrong type for socket */
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT	99	/* Protocol not available */
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT	120	/* Protocol not supported */
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT	121	/* Socket type not supported */
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP	122	/* Operation not supported on socket */
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT	123	/* Protocol family not supported */
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT	124	/* Address family not supported */
#endif
#ifndef EADDRINUSE
#define EADDRINUSE	125	/* Address already in use */
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL	126	/* Can't assign requested address */
#endif
#ifndef ENETDOWN
#define ENETDOWN	127	/* Network is down */
#endif
#ifndef ENETUNREACH
#define ENETUNREACH	128	/* Network is unreachable */
#endif
#ifndef ENETRESET
#define ENETRESET	129	/* Network dropped connection on reset */
#endif
#ifndef ECONNABORTED
#define ECONNABORTED	130	/* Software caused connection abort */
#endif
#ifndef ECONNRESET
#define ECONNRESET	131	/* Connection reset by peer */
#endif
#ifndef ENOBUFS
#define ENOBUFS		132	/* No buffer space available */
#endif
#ifndef EISCONN
#define EISCONN		133	/* Socket is already connected */
#endif
#ifndef ENOTCONN
#define ENOTCONN	134	/* Socket is not connected */
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN	143	/* Can't send after socket shutdown */
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS	144	/* Too many references: can't splice */
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT	145	/* Connection timed out */
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED	146	/* Connection refused */
#endif
#ifndef ELOOP
#define ELOOP		90	/* Symbolic link loop */
#endif
#ifndef EHOSTDOWN
#define EHOSTDOWN	147	/* Host is down */
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH	148	/* No route to host */
#endif
#ifndef ENOTEMPTY
#define ENOTEMPTY 	93	/* directory not empty */
#endif
#ifndef EUSERS
#define EUSERS		94	/* Too many users (for UFS) */
#endif
#ifndef EDQUOT
#define EDQUOT		49	/* Disc quota exceeded */
#endif
#ifndef ESTALE
#define ESTALE		151	/* Stale NFS file handle */
#endif
#ifndef EREMOTE
#define EREMOTE		66	/* The object is remote */
#endif

/*
 * Supply definitions for macros to query wait status, if not already
 * defined in header files above.
 */

#if TCL_UNION_WAIT
#   define WAIT_STATUS_TYPE union wait
#else
#   define WAIT_STATUS_TYPE int
#endif

#ifndef WIFEXITED
#   define WIFEXITED(stat)  (((*((int *) &(stat))) & 0xff) == 0)
#endif

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat) (((*((int *) &(stat))) >> 8) & 0xff)
#endif

#ifndef WIFSIGNALED
#   define WIFSIGNALED(stat) (((*((int *) &(stat)))) && ((*((int *) &(stat))) == ((*((int *) &(stat))) & 0x00ff)))
#endif

#ifndef WTERMSIG
#   define WTERMSIG(stat)    ((*((int *) &(stat))) & 0x7f)
#endif

#ifndef WIFSTOPPED
#   define WIFSTOPPED(stat)  (((*((int *) &(stat))) & 0xff) == 0177)
#endif

#ifndef WSTOPSIG
#   define WSTOPSIG(stat)    (((*((int *) &(stat))) >> 8) & 0xff)
#endif

/*
 * Define constants for waitpid() system call if they aren't defined
 * by a system header file.
 */

#ifndef WNOHANG
#   define WNOHANG 1
#endif
#ifndef WUNTRACED
#   define WUNTRACED 2
#endif

/*
 * Define access mode constants if they aren't already defined.
 */

#ifndef F_OK
#    define F_OK 00
#endif
#ifndef X_OK
#    define X_OK 01
#endif
#ifndef W_OK
#    define W_OK 02
#endif
#ifndef R_OK
#    define R_OK 04
#endif

/*
 * Define macros to query file type bits, if they're not already
 * defined.
 */

#ifndef S_ISREG
#   ifdef S_IFREG
#       define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#   else
#       define S_ISREG(m) 0
#   endif
# endif
#ifndef S_ISDIR
#   ifdef S_IFDIR
#       define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#   else
#       define S_ISDIR(m) 0
#   endif
# endif
#ifndef S_ISCHR
#   ifdef S_IFCHR
#       define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#   else
#       define S_ISCHR(m) 0
#   endif
# endif
#ifndef S_ISBLK
#   ifdef S_IFBLK
#       define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#   else
#       define S_ISBLK(m) 0
#   endif
# endif
#ifndef S_ISFIFO
#   ifdef S_IFIFO
#       define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#   else
#       define S_ISFIFO(m) 0
#   endif
# endif

/*
 * Define MAXPATHLEN in terms of MAXPATH if available
 */

#ifndef MAXPATH
#define MAXPATH MAX_PATH
#endif /* MAXPATH */

#ifndef MAXPATHLEN
#define MAXPATHLEN MAXPATH
#endif /* MAXPATHLEN */

/*
 * Define pid_t and uid_t if they're not already defined.
 */

#if ! TCL_PID_T
#   define pid_t int
#endif
#if ! TCL_UID_T
#   define uid_t int
#endif

/*
 * Visual C++ has some odd names for common functions, so we need to
 * define a few macros to handle them.  Also, it defines EDEADLOCK and
 * EDEADLK as the same value, which confuses Tcl_ErrnoId().
 */

#ifdef _MSC_VER
#    define environ _environ
#    define hypot _hypot
#    define exception _exception
#    undef EDEADLOCK
#endif /* _MSC_VER */

/*
 *---------------------------------------------------------------------------
 * The following macros and declarations represent the interface between 
 * generic and windows-specific parts of Tcl.  Some of the macros may 
 * override functions declared in tclInt.h.
 *---------------------------------------------------------------------------
 */

/*
 * The default platform eol translation on Windows is TCL_TRANSLATE_CRLF:
 */

#define	TCL_PLATFORM_TRANSLATION	TCL_TRANSLATE_CRLF

/*
 * Declare dynamic loading extension macro.
 */

#define TCL_SHLIB_EXT ".dll"

/*
 * The following define ensures that we use the native putenv
 * implementation to modify the environment array.  This keeps
 * the C level environment in synch with the system level environment.
 */

#define USE_PUTENV	1

/*
 * The following defines wrap the system memory allocation routines for
 * use by tclAlloc.c.
 */

#define TclpSysAlloc(size, isBin)	((void*)HeapAlloc(GetProcessHeap(), \
					    (DWORD)0, (DWORD)size))
#define TclpSysFree(ptr)		(HeapFree(GetProcessHeap(), \
					    (DWORD)0, (HGLOBAL)ptr))
#define TclpSysRealloc(ptr, size)	((void*)HeapReAlloc(GetProcessHeap(), \
					    (DWORD)0, (LPVOID)ptr, (DWORD)size))

/*
 * The following defines map from standard socket names to our internal
 * wrappers that redirect through the winSock function table (see the
 * file tclWinSock.c).
 */

#define getservbyname	TclWinGetServByName
#define getsockopt	TclWinGetSockOpt
#define ntohs		TclWinNToHS
#define setsockopt	TclWinSetSockOpt

/*
 * The following macros have trivial definitions, allowing generic code to 
 * address platform-specific issues.
 */

#define TclpReleaseFile(file)	ckfree((char *) file)

/*
 * The following macros and declarations wrap the C runtime library 
 * functions.
 */

#define TclpExit		exit
#define TclpLstat		TclpStat

/*
 * Declarations for Windows-only functions.
 */

EXTERN Tcl_Channel  TclWinOpenSerialChannel _ANSI_ARGS_((HANDLE handle,
                        char *channelName, int permissions));
					 
EXTERN Tcl_Channel  TclWinOpenConsoleChannel _ANSI_ARGS_((HANDLE handle,
                        char *channelName, int permissions));

EXTERN Tcl_Channel  TclWinOpenFileChannel _ANSI_ARGS_((HANDLE handle,
                        char *channelName, int permissions, int appendMode));

EXTERN TclFile TclWinMakeFile _ANSI_ARGS_((HANDLE handle));

/*
 * Platform specific mutex definition used by memory allocators.
 * These mutexes are statically allocated and explicitly initialized.
 * Most modules do not use this, but instead use Tcl_Mutex types and
 * Tcl_MutexLock and Tcl_MutexUnlock that are self-initializing.
 */

#ifdef TCL_THREADS
typedef CRITICAL_SECTION TclpMutex;
EXTERN void	TclpMutexInit _ANSI_ARGS_((TclpMutex *mPtr));
EXTERN void	TclpMutexLock _ANSI_ARGS_((TclpMutex *mPtr));
EXTERN void	TclpMutexUnlock _ANSI_ARGS_((TclpMutex *mPtr));
#else
typedef int TclpMutex;
#define	TclpMutexInit(a)
#define	TclpMutexLock(a)
#define	TclpMutexUnlock(a)
#endif /* TCL_THREADS */

#include "tclPlatDecls.h"
#include "tclIntPlatDecls.h"

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TCLWINPORT */
