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

/*
 * The following definitions are usually found if fcntl.h.
 * However, MetroWerks has screwed that file up a couple of times
 * and all we need are the defines.
 */

#   define O_RDWR	  	0x0	/* open the file in read/write mode */
#   define O_RDONLY  		0x1	/* open the file in read only mode */
#   define O_WRONLY  		0x2	/* open the file in write only mode */
#   define O_APPEND  		0x0100	/* open the file in append mode */
#   define O_CREAT	  	0x0200	/* create the file if it doesn't exist */
#   define O_EXCL	  	0x0400	/* if the file exists don't create it again */
#   define O_TRUNC	  	0x0800	/* truncate the file after opening it */

/*
 * MetroWerks stat.h file is rather weak.  The defines
 * after the include are needed to fill in the missing
 * defines.
 */

#   include <stat.h>
#   ifndef S_IFIFO
#	define S_IFIFO		0x0100
#   endif
#   ifndef S_IFBLK
#	define S_IFBLK		0x0600
#   endif
#   ifndef S_ISLNK
#	define S_ISLNK(m)	(((m)&(S_IFMT)) == (S_IFLNK))
#   endif
#   ifndef S_ISSOCK
#	define S_ISSOCK(m)	(((m)&(S_IFMT)) == (S_IFSOCK))
#   endif
#   ifndef S_IRWXU
#	define S_IRWXU		00007	/* read, write, execute: owner */
#   	define S_IRUSR		00004	/* read permission: owner */
#   	define S_IWUSR		00002	/* write permission: owner */
#   	define S_IXUSR		00001	/* execute permission: owner */
#   	define S_IRWXG		00007	/* read, write, execute: group */
#   	define S_IRGRP		00004	/* read permission: group */
#   	define S_IWGRP		00002	/* write permission: group */
#   	define S_IXGRP		00001	/* execute permission: group */
#   	define S_IRWXO		00007	/* read, write, execute: other */
#   	define S_IROTH		00004	/* read permission: other */
#   	define S_IWOTH		00002	/* write permission: other */
#   	define S_IXOTH		00001	/* execute permission: other */
#   endif

#   define isatty(arg) 		1

/* 
 * Defines used by access function.  This function is provided
 * by Mac Tcl as the function TclpAccess.
 */
 
#   define F_OK			0	/* test for existence of file */
#   define X_OK			0x01	/* test for execute or search permission */
#   define W_OK			0x02	/* test for write permission */
#   define R_OK			0x04	/* test for read permission */

#endif	/* __MWERKS__ */

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
 * The following define is bogus and needs to be fixed.  It claims that
 * struct tm has the timezone string in it, which is not true.  However,
 * the code that works around this fact does not compile on the Mac, since
 * it relies on the fact that time.h has a "timezone" variable, which the
 * Metrowerks time.h does not have...
 * 
 * The Mac timezone stuff never worked (clock format 0 -format %Z returns "Z")
 * so this just keeps the status quo.  The real answer is to not use the
 * MSL strftime, and provide the needed compat functions...
 * 
 */
 
#define HAVE_TM_ZONE 
 
/*
 * The following macros have trivial definitions, allowing generic code to 
 * address platform-specific issues.
 */
 
#define TclpAsyncMark(async)
#define TclpGetPid(pid)	    	((unsigned long) (pid))
#define TclSetSystemEnv(a,b)
#define tzset()

/*
 * The following defines replace the Macintosh version of the POSIX
 * functions "stat" and "access".  The various compilier vendors
 * don't implement this function well nor consistantly.
 */
/* int TclpStat(const char *path, struct stat *bufPtr); */
int TclpLstat(const char *path, struct stat *bufPtr);

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
#include "tclMac.h"
#include "tclMacInt.h"
/* #include "tclPlatDecls.h"
   #include "tclIntPlatDecls.h" */

#endif /* _MACPORT */
