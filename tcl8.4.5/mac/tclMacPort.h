/*
 * tclMacPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between the Mac and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */


#ifndef _MACPORT
#define _MACPORT

#ifndef _TCLINT
#   include "tclInt.h"
#endif

/*
 *---------------------------------------------------------------------------
 * The following sets of #includes and #ifdefs are required to get Tcl to
 * compile on the macintosh.
 *---------------------------------------------------------------------------
 */

#include "tclErrno.h"

#ifndef EOVERFLOW
#   ifdef EFBIG
#      define EOVERFLOW	EFBIG	/* The object couldn't fit in the datatype */
#   else /* !EFBIG */
#      define EOVERFLOW	EINVAL	/* Better than nothing! */
#   endif /* EFBIG */
#endif /* !EOVERFLOW */

#include <float.h>

#ifdef THINK_C
	/*
	 * The Symantic C code has not been tested
	 * and probably will not work.
	 */
#   include <pascal.h>
#   include <posix.h>
#   include <string.h>
#   include <fcntl.h>
#   include <pwd.h>
#   include <sys/param.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <unistd.h>
#elif defined(__MWERKS__)
#   include <time.h>
#   include <unistd.h>
#   include <utime.h>
#   include <fcntl.h>
#   include <stat.h>

#if __MSL__ < 0x6000
#   define isatty(arg) 		1

/* 
 * Defines used by access function.  This function is provided
 * by Mac Tcl as the function TclpAccess.
 */
 
#   define F_OK			0	/* test for existence of file */
#   define X_OK			0x01	/* test for execute or search permission */
#   define W_OK			0x02	/* test for write permission */
#   define R_OK			0x04	/* test for read permission */
#endif

#endif	/* __MWERKS__ */

#if defined(S_IFBLK) && !defined(S_ISLNK)
#define S_ISLNK(m)	(((m)&(S_IFMT)) == (S_IFLNK))
#endif

/*
 * Many signals are not supported on the Mac and are thus not defined in
 * <signal.h>.  They are defined here so that Tcl will compile with less
 * modification.
  */

#ifndef SIGQUIT
#define SIGQUIT 300
#endif

#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#ifndef SIGHUP
#define SIGHUP  100
#endif

/*
 * waitpid doesn't work on a Mac - the following makes
 * Tcl compile without errors.  These would normally
 * be defined in sys/wait.h on UNIX systems.
 */

#define WAIT_STATUS_TYPE 	int
#define WNOHANG 		1
#define WIFSTOPPED(stat) 	(1)
#define WIFSIGNALED(stat) 	(1)
#define WIFEXITED(stat) 	(1)
#define WIFSTOPSIG(stat) 	(1)
#define WIFTERMSIG(stat) 	(1)
#define WIFEXITSTATUS(stat) 	(1)
#define WEXITSTATUS(stat) 	(1)
#define WTERMSIG(status) 	(1)
#define WSTOPSIG(status) 	(1)

#ifdef BUILD_tcl
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * Make sure that MAXPATHLEN is defined.
 */

#ifndef MAXPATHLEN
#   ifdef PATH_MAX
#       define MAXPATHLEN PATH_MAX
#   else
#       define MAXPATHLEN 2048
#   endif
#endif

/*
 * Define "NBBY" (number of bits per byte) if it's not already defined.
 */

#ifndef NBBY
#   define NBBY 8
#endif

/*
 * These functions always return dummy values on Mac.
 */
#ifndef geteuid
#   define geteuid() 1
#endif
#ifndef getpid
#   define getpid() -1
#endif

/*
 * Variables provided by the C library.
 */
 
extern char **environ;

/*
 *---------------------------------------------------------------------------
 * The following macros and declarations represent the interface between 
 * generic and mac-specific parts of Tcl.  Some of the macros may override 
 * functions declared in tclInt.h.
 *---------------------------------------------------------------------------
 */

/*
 * The default platform eol translation on Mac is TCL_TRANSLATE_CR:
 */

#define	TCL_PLATFORM_TRANSLATION	TCL_TRANSLATE_CR

/*
 * Declare dynamic loading extension macro.
 */

#define TCL_SHLIB_EXT ".shlb"

/*
 * The following define is defined as a workaround on the mac.  It claims that
 * struct tm has the timezone string in it, which is not true.  However,
 * the code that works around this fact does not compile on the Mac, since
 * it relies on the fact that time.h has a "timezone" variable, which the
 * Metrowerks time.h does not have...
 * 
 * The Mac timezone stuff is implemented via the TclpGetTZName() routine in
 * tclMacTime.c
 * 
 */
 
#define HAVE_TM_ZONE 
 
 
/*
 * If we're using the Metrowerks MSL, we need to convert time_t values from
 * the mac epoch to the msl epoch (== unix epoch) by adding the offset from
 * <time.mac.h> to mac time_t values, as MSL is using its epoch for file
 * access routines such as stat or utime
 */

#ifdef __MSL__
#include <time.mac.h>
#ifdef _mac_msl_epoch_offset_
#define tcl_mac_epoch_offset  _mac_msl_epoch_offset_
#define TCL_MAC_USE_MSL_EPOCH  /* flag for TclDate.c */
#else
#define tcl_mac_epoch_offset 0L
#endif
#else
#define tcl_mac_epoch_offset 0L
#endif
 
/*
 * The following macros have trivial definitions, allowing generic code to 
 * address platform-specific issues.
 */
 
#define TclpGetPid(pid)	    	((unsigned long) (pid))
#define TclSetSystemEnv(a,b)
#define tzset()

char *TclpFindExecutable(const char *argv0);
int TclpFindVariable(CONST char *name, int *lengthPtr);

#define fopen(path, mode) TclMacFOpenHack(path, mode)
#define readlink(fileName, buffer, size) TclMacReadlink(fileName, buffer, size)
#ifdef TCL_TEST
#define chmod(path, mode) TclMacChmod(path, mode)
#endif

/*
 * Prototypes needed for compatability
 */

/* EXTERN int	strncasecmp _ANSI_ARGS_((CONST char *s1,
			    CONST char *s2, size_t n)); */

/*
 * These definitions force putenv & company to use the version
 * supplied with Tcl.
 */
#ifndef putenv
#   define unsetenv	TclUnsetEnv
#   define putenv	Tcl_PutEnv
#   define setenv	TclSetEnv
void	TclSetEnv(CONST char *name, CONST char *value);
/* int	Tcl_PutEnv(CONST char *string); */
void	TclUnsetEnv(CONST char *name);
#endif

/*
 * Platform specific mutex definition used by memory allocators.
 * These are all no-ops on the Macintosh, since the threads are
 * all cooperative.
 */

#ifdef TCL_THREADS
typedef int TclpMutex;
#define	TclpMutexInit(a)
#define	TclpMutexLock(a)
#define	TclpMutexUnlock(a)
#else
typedef int TclpMutex;
#define	TclpMutexInit(a)
#define	TclpMutexLock(a)
#define	TclpMutexUnlock(a)
#endif /* TCL_THREADS */

typedef pascal void (*ExitToShellProcPtr)(void);

#include "tclMac.h" // contains #include "tclPlatDecls.h"
#include "tclIntPlatDecls.h"

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _MACPORT */
