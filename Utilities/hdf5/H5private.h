/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Friday, October 30, 1998
 *
 * Purpose:     This file is included by all HDF5 library source files to
 *              define common things which are not defined in the HDF5 API.
 *              The configuration constants like H5_HAVE_UNISTD_H etc. are
 *              defined in H5config.h which is included by H5public.h.
 *
 */

#ifndef _H5private_H
#define _H5private_H

#include "H5public.h"           /* Include Public Definitions           */

/* include the pthread header */
#ifdef H5_HAVE_THREADSAFE
#include <pthread.h>
#endif

/*
 * Include ANSI-C header files.
 */
#ifdef H5_STDC_HEADERS
#   include <assert.h>
#   include <ctype.h>
#   include <errno.h>
#   include <fcntl.h>
#   include <float.h>
#   include <limits.h>
#   include <math.h>
#   include <signal.h>
#   include <stdarg.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#endif

/*
 * If _POSIX_VERSION is defined in unistd.h then this system is Posix.1
 * compliant. Otherwise all bets are off.
 */
#ifdef H5_HAVE_UNISTD_H
#   include <sys/types.h>
#   include <unistd.h>
#endif
#ifdef _POSIX_VERSION
#   include <sys/wait.h>
#   include <pwd.h>
#endif

/*
 * C9x integer types
 */
#ifdef H5_HAVE_STDINT_H
#   include <stdint.h>
#endif

/*
 * The `struct stat' data type for stat() and fstat(). This is a Posix file
 * but often apears on non-Posix systems also.  The `struct stat' is required
 * for hdf5 to compile, although only a few fields are actually used.
 */
#ifdef H5_HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif

/*
 * If a program may include both `time.h' and `sys/time.h' then
 * TIME_WITH_SYS_TIME is defined (see AC_HEADER_TIME in configure.in).
 * On some older systems, `sys/time.h' includes `time.h' but `time.h' is not
 * protected against multiple inclusion, so programs should not explicitly
 * include both files. This macro is useful in programs that use, for example,
 * `struct timeval' or `struct timezone' as well as `struct tm'.  It is best
 * used in conjunction with `HAVE_SYS_TIME_H', whose existence is checked
 * by `AC_CHECK_HEADERS(sys/time.h)' in configure.in.
 */
#if defined(H5_TIME_WITH_SYS_TIME)
#   include <sys/time.h>
#   include <time.h>
#elif defined(H5_HAVE_SYS_TIME_H)
#   include <sys/time.h>
#else
#   include <time.h>
#endif

/*
 * Longjumps are used to detect alignment constrants
 */
#ifdef H5_HAVE_SETJMP_H
#   include <setjmp.h>
#endif

/*
 * Resource usage is not Posix.1 but HDF5 uses it anyway for some performance
 * and debugging code if available.
 */
#ifdef H5_HAVE_SYS_RESOURCE_H
#   include <sys/resource.h>
#endif

/*
 * Unix ioctls.  These are used by h5ls (and perhaps others) to determine a
 * resonable output width.
 */
#ifdef H5_HAVE_SYS_IOCTL_H
#   include <sys/ioctl.h>
#endif

/*
 * System information. These are needed on the DEC Alpha to turn off fixing
 * of unaligned accesses by the operating system during detection of
 * alignment constraints in H5detect.c:main().
 */
#ifdef H5_HAVE_SYS_SYSINFO_H
#   include <sys/sysinfo.h>
#endif
#ifdef H5_HAVE_SYS_PROC_H
#   include <sys/proc.h>
#endif
#ifdef H5_HAVE_IO_H
#   include <io.h>
#endif


#ifdef WIN32

#define VC_EXTRALEAN            /*Exclude rarely-used stuff from Windows headers */
#include <windows.h>

#endif /*WIN32*/

/* H5_inline */
#ifndef H5_inline
#define H5_inline
#endif /* H5_inline */


#ifndef F_OK
#   define F_OK 00
#   define W_OK 02
#   define R_OK 04
#endif

/*
 * Pablo support files.
 */
#ifdef H5_HAVE_PABLO
#   define IOTRACE
#   define HDFIOTRACE
#   include "HDFIOTrace.h"
#   include "ProcIDs.h"
#endif

/*
 * MPE Instrumentation support
 * Do not #if the following header file because it contains
 * the needed null definitions for the H5-MPE macros when MPE
 * support is not configured.
 *
 */
#include "H5MPprivate.h"

/*
 * dmalloc (debugging malloc) support
 */
#ifdef H5_HAVE_DMALLOC_H
#include "dmalloc.h"
#endif /* H5_HAVE_DMALLOC_H */

/*
 * NT doesn't define SIGBUS, but since NT only runs on processors             
 * that do not have alignment constraints a SIGBUS would never be              
 * raised, so we just replace it with SIGILL (which also should               
 * never be raised by the hdf5 library).                                       
 */  
#ifndef SIGBUS
#       define SIGBUS SIGILL
#endif

/*
 * Does the compiler support the __attribute__(()) syntax?  This is how gcc
 * suppresses warnings about unused function arguments.  It's no big deal if
 * we don't.
 */
#ifdef __cplusplus
#   define __attribute__(X)     /*void*/
#   define UNUSED               /*void*/
#else /* __cplusplus */
#ifdef H5_HAVE_ATTRIBUTE
#   define UNUSED               __attribute__((unused))
#else
#   define __attribute__(X)     /*void*/
#   define UNUSED               /*void*/
#endif
#endif /* __cplusplus */

/*
 * Does the compiler expand __FUNCTION__ to be the name of the function
 * currently being defined?  If not then define it to be some constant
 * string.
 */
#ifndef H5_HAVE_FUNCTION
#   define __FUNCTION__  "NoFunctionName"
#endif

/* Version #'s of the major components of the file format */
#define HDF5_SUPERBLOCK_VERSION_DEF     0       /* The default super block format         */
#define HDF5_SUPERBLOCK_VERSION_MAX     1       /* The maximum super block format         */
#define HDF5_FREESPACE_VERSION  0       /* of the Free-Space Info         */
#define HDF5_OBJECTDIR_VERSION  0       /* of the Object Directory format */
#define HDF5_SHAREDHEADER_VERSION 0     /* of the Shared-Header Info      */
#define HDF5_DRIVERINFO_VERSION 0       /* of the Driver Information Block*/

/* B-tree internal 'K' values */
#define HDF5_BTREE_SNODE_IK_DEF         16
#define HDF5_BTREE_ISTORE_IK_DEF        32      /* Note! this value is assumed
                                                    to be 32 for older versions
                                                    of the superblock (<1) and
                                                    if it is changed, the code
                                                    must compensate. -QAK
                                                 */

/*
 * Status return values for the `herr_t' type.
 * Since some unix/c routines use 0 and -1 (or more precisely, non-negative
 * vs. negative) as their return code, and some assumption had been made in
 * the code about that, it is important to keep these constants the same
 * values.  When checking the success or failure of an integer-valued
 * function, remember to compare against zero and not one of these two
 * values.
 */
#define SUCCEED         0
#define FAIL            (-1)
#define UFAIL           (unsigned)(-1)

/* number of members in an array */
#ifndef NELMTS
#    define NELMTS(X)           (sizeof(X)/sizeof(X[0]))
#endif

/* minimum of two, three, or four values */
#undef MIN
#define MIN(a,b)                (((a)<(b)) ? (a) : (b))
#define MIN2(a,b)               MIN(a,b)
#define MIN3(a,b,c)             MIN(a,MIN(b,c))
#define MIN4(a,b,c,d)           MIN(MIN(a,b),MIN(c,d))

/* maximum of two, three, or four values */
#undef MAX
#define MAX(a,b)                (((a)>(b)) ? (a) : (b))
#define MAX2(a,b)               MAX(a,b)
#define MAX3(a,b,c)             MAX(a,MAX(b,c))
#define MAX4(a,b,c,d)           MAX(MAX(a,b),MAX(c,d))

/* limit the middle value to be within a range (inclusive) */
#define RANGE(LO,X,HI)          MAX(LO,MIN(X,HI))

/* absolute value */
#ifndef ABS
#   define ABS(a)               (((a)>=0) ? (a) : -(a))
#endif

/* sign of argument */
#ifndef SIGN
#   define SIGN(a)              ((a)>0 ? 1 : (a)<0 ? -1 : 0)
#endif

/*
 * HDF Boolean type.
 */
#ifndef FALSE
#   define FALSE 0
#endif
#ifndef TRUE
#   define TRUE (!FALSE)
#endif

/*
 * Although `long long' is part of the revised ANSI-C some compilers don't
 * support it yet.  We define `long_long' as the longest integral integer type
 * supported by the compiler, usually 64 bits.  It must be legal to qualify
 * `long_long' with `unsigned'.
 */
#if H5_SIZEOF_LONG_LONG>0
#   define long_long    long long
#elif H5_SIZEOF___INT64>0
#   define long_long    __int64 /*Win32*/
#   undef H5_SIZEOF_LONG_LONG
#   define H5_SIZEOF_LONG_LONG H5_SIZEOF___INT64
#else
#   define long_long    long int
#   undef H5_SIZEOF_LONG_LONG
#   define H5_SIZEOF_LONG_LONG H5_SIZEOF_LONG
#endif

/*
 * Numeric data types.  Some of these might be defined in Posix.1g, otherwise
 * we define them with the closest available type which is at least as large
 * as the number of bits indicated in the type name.  The `int8' types *must*
 * be exactly one byte wide because we use it for pointer calculations to
 * void* memory.
 */
#if H5_SIZEOF_INT8_T==0
    typedef signed char int8_t;
#   undef H5_SIZEOF_INT8_T
#   define H5_SIZEOF_INT8_T H5_SIZEOF_CHAR
#elif H5_SIZEOF_INT8_T==1
#else
#   error "the int8_t type must be 1 byte wide"
#endif

#if H5_SIZEOF_UINT8_T==0
    typedef unsigned char uint8_t;
#   undef H5_SIZEOF_UINT8_T
#   define H5_SIZEOF_UINT8_T H5_SIZEOF_CHAR
#elif H5_SIZEOF_UINT8_T==1
#else
#   error "the uint8_t type must be 1 byte wide"
#endif

#if H5_SIZEOF_INT16_T>=2
#elif H5_SIZEOF_SHORT>=2
    typedef short int16_t;
#   undef H5_SIZEOF_INT16_T
#   define H5_SIZEOF_INT16_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=2
    typedef int int16_t;
#   undef H5_SIZEOF_INT16_T
#   define H5_SIZEOF_INT16_T H5_SIZEOF_INT
#else
#   error "nothing appropriate for int16_t"
#endif

#if H5_SIZEOF_UINT16_T>=2
#elif H5_SIZEOF_SHORT>=2
    typedef unsigned short uint16_t;
#   undef H5_SIZEOF_UINT16_T
#   define H5_SIZEOF_UINT16_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=2
    typedef unsigned uint16_t;
#   undef H5_SIZEOF_UINT16_T
#   define H5_SIZEOF_UINT16_T H5_SIZEOF_INT
#else
#   error "nothing appropriate for uint16_t"
#endif

#if H5_SIZEOF_INT32_T>=4
#elif H5_SIZEOF_SHORT>=4
    typedef short int32_t;
#   undef H5_SIZEOF_INT32_T
#   define H5_SIZEOF_INT32_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=4
    typedef int int32_t;
#   undef H5_SIZEOF_INT32_T
#   define H5_SIZEOF_INT32_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=4
    typedef long int32_t;
#   undef H5_SIZEOF_INT32_T
#   define H5_SIZEOF_INT32_T H5_SIZEOF_LONG
#else
#   error "nothing appropriate for int32_t"
#endif

#if H5_SIZEOF_UINT32_T>=4
#elif H5_SIZEOF_SHORT>=4
    typedef short uint32_t;
#   undef H5_SIZEOF_UINT32_T
#   define H5_SIZEOF_UINT32_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=4
    typedef unsigned int uint32_t;
#   undef H5_SIZEOF_UINT32_T
#   define H5_SIZEOF_UINT32_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=4
    typedef unsigned long uint32_t;
#   undef H5_SIZEOF_UINT32_T
#   define H5_SIZEOF_UINT32_T H5_SIZEOF_LONG
#else
#   error "nothing appropriate for uint32_t"
#endif

#if H5_SIZEOF_INT64_T>=8
#elif H5_SIZEOF_INT>=8
    typedef int int64_t;
#   undef H5_SIZEOF_INT64_T
#   define H5_SIZEOF_INT64_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=8
    typedef long int64_t;
#   undef H5_SIZEOF_INT64_T
#   define H5_SIZEOF_INT64_T H5_SIZEOF_LONG
#elif H5_SIZEOF_LONG_LONG>=8
    typedef long_long int64_t;
#   undef H5_SIZEOF_INT64_T
#   define H5_SIZEOF_INT64_T H5_SIZEOF_LONG_LONG
#else
#   error "nothing appropriate for int64_t"
#endif

#if H5_SIZEOF_UINT64_T>=8
#elif H5_SIZEOF_INT>=8
    typedef unsigned uint64_t;
#   undef H5_SIZEOF_UINT64_T
#   define H5_SIZEOF_UINT64_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=8
    typedef unsigned long uint64_t;
#   undef H5_SIZEOF_UINT64_T
#   define H5_SIZEOF_UINT64_T H5_SIZEOF_LONG
#elif H5_SIZEOF_LONG_LONG>=8
    typedef unsigned long_long uint64_t;
#   undef H5_SIZEOF_UINT64_T
#   define H5_SIZEOF_UINT64_T H5_SIZEOF_LONG_LONG
#else
#   error "nothing appropriate for uint64_t"
#endif

/*
 * Maximum and minimum values.  These should be defined in <limits.h> for the
 * most part.
 */
#ifndef LLONG_MAX
#   define LLONG_MAX    ((long_long)(((unsigned long_long)1                   \
                                      <<(8*sizeof(long_long)-1))-1))
#   define ULLONG_MAX   ((unsigned long_long)((long_long)(-1)))
#endif
#ifndef SIZET_MAX
#   define SIZET_MAX    ((size_t)(ssize_t)(-1))
#   define SSIZET_MAX   ((ssize_t)(((size_t)1<<(8*sizeof(ssize_t)-1))-1))
#endif
#ifdef H5_HAVE_LARGE_HSIZET
#define HSIZET_MAX      ((hsize_t)ULLONG_MAX)
#define HSSIZET_MAX     ((hssize_t)LLONG_MAX)
#else /* H5_HAVE_LARGE_HSIZET */
#define HSIZET_MAX      ((hsize_t)SIZET_MAX)
#define HSSIZET_MAX     ((hssize_t)SSIZET_MAX)
#endif /* H5_HAVE_LARGE_HSIZET */
#define HSSIZET_MIN     (~(HSSIZET_MAX))

/*
 * Some compilers have problems declaring auto variables that point
 * to string constants.  Use the CONSTR() macro so it's easy to fix
 * those compilers.
 */
#ifndef CONSTR
#  define CONSTR(VAR,STR) static const char VAR[]=STR
#endif

/*
 * A macro to portably increment enumerated types.
 */
#ifndef H5_INC_ENUM
#  define H5_INC_ENUM(TYPE,VAR) (VAR)=((TYPE)((VAR)+1))
#endif

/*
 * A macro to portably decrement enumerated types.
 */
#ifndef H5_DEC_ENUM
#  define H5_DEC_ENUM(TYPE,VAR) (VAR)=((TYPE)((VAR)-1))
#endif

/*
 * A macro for detecting over/under-flow when casting between types
 */
#ifndef NDEBUG
#define H5_CHECK_OVERFLOW(var,vartype,casttype) \
{                                               \
    casttype _tmp_overflow=(casttype)(var);     \
    assert((var)==(vartype)_tmp_overflow);      \
}
#else /* NDEBUG */
#define H5_CHECK_OVERFLOW(var,vartype,casttype)
#endif /* NDEBUG */

/*
 * A macro for detecting over/under-flow when assigning between types
 */
#ifndef NDEBUG
#define H5_ASSIGN_OVERFLOW(var,expr,vartype,casttype)   \
{                                                       \
    vartype _tmp_overflow=(vartype)(expr);              \
    casttype _tmp_overflow2=(casttype)(_tmp_overflow);  \
    assert((casttype)_tmp_overflow==_tmp_overflow2);    \
    (var)=_tmp_overflow2;                               \
}
#else /* NDEBUG */
#define H5_ASSIGN_OVERFLOW(var,expr,vartype,casttype)   \
    (var)=(casttype)(expr);
#endif /* NDEBUG */

/*
 * Data types and functions for timing certain parts of the library.
 */
typedef struct {
    double      utime;          /*user time                     */
    double      stime;          /*system time                   */
    double      etime;          /*elapsed wall-clock time       */
} H5_timer_t;

H5_DLL void H5_timer_reset (H5_timer_t *timer);
H5_DLL void H5_timer_begin (H5_timer_t *timer);
H5_DLL void H5_timer_end (H5_timer_t *sum/*in,out*/,
                           H5_timer_t *timer/*in,out*/);
H5_DLL void H5_bandwidth(char *buf/*out*/, double nbytes, double nseconds);

/*
 * Redefine all the POSIX functions.  We should never see a POSIX
 * function (or any other non-HDF5 function) in the source!
 */
#define HDabort()               abort()
#define HDabs(X)                abs(X)
#define HDaccess(F,M)           access(F, M)
#define HDacos(X)               acos(X)
#define HDalarm(N)              alarm(N)
#define HDasctime(T)            asctime(T)
#define HDasin(X)               asin(X)
#define HDassert(X)             assert(X)
#define HDatan(X)               atan(X)
#define HDatan2(X,Y)            atan2(X,Y)
#define HDatexit(F)             atexit(F)
#define HDatof(S)               atof(S)
#define HDatoi(S)               atoi(S)
#define HDatol(S)               atol(S)
#define HDBSDgettimeofday(S,P)  BSDgettimeofday(S,P)
#define HDbsearch(K,B,N,Z,F)    bsearch(K,B,N,Z,F)
#define HDcalloc(N,Z)           calloc(N,Z)
#define HDceil(X)               ceil(X)
#define HDcfgetispeed(T)        cfgetispeed(T)
#define HDcfgetospeed(T)        cfgetospeed(T)
#define HDcfsetispeed(T,S)      cfsetispeed(T,S)
#define HDcfsetospeed(T,S)      cfsetospeed(T,S)
#define HDchdir(S)              chdir(S)
#define HDchmod(S,M)            chmod(S,M)
#define HDchown(S,O,G)          chown(S,O,G)
#define HDclearerr(F)           clearerr(F)
#define HDclock()               clock()
#define HDclose(F)              close(F)
#define HDclosedir(D)           closedir(D)
#define HDcos(X)                cos(X)
#define HDcosh(X)               cosh(X)
#define HDcreat(S,M)            creat(S,M)
#define HDctermid(S)            ctermid(S)
#define HDctime(T)              ctime(T)
#define HDcuserid(S)            cuserid(S)
#ifdef H5_HAVE_DIFFTIME
#define HDdifftime(X,Y)         difftime(X,Y)
#else
#define HDdifftime(X,Y)         ((double)(X)-(double)(Y))
#endif
#define HDdiv(X,Y)              div(X,Y)
#define HDdup(F)                dup(F)
#define HDdup2(F,I)             dup2(F,I)
/* execl() variable arguments */
/* execle() variable arguments */
/* execlp() variable arguments */
#define HDexecv(S,AV)           execv(S,AV)
#define HDexecve(S,AV,E)        execve(S,AV,E)
#define HDexecvp(S,AV)          execvp(S,AV)
#define HDexit(N)               exit(N)
#if defined __MWERKS__
#include <abort_exit.h>
#define HD_exit(N)              __exit(N)
#else /* __MWERKS __ */
#define HD_exit(N)              _exit(N)
#endif /* __MWERKS __ */
#define HDexp(X)                exp(X)
#define HDfabs(X)               fabs(X)
#define HDfclose(F)             fclose(F)
/* fcntl() variable arguments */
#define HDfdopen(N,S)           fdopen(N,S)
#define HDfeof(F)               feof(F)
#define HDferror(F)             ferror(F)
#define HDfflush(F)             fflush(F)
#define HDfgetc(F)              fgetc(F)
#define HDfgetpos(F,P)          fgetpos(F,P)
#define HDfgets(S,N,F)          fgets(S,N,F)
#define HDfileno(F)             fileno(F)
#define HDfloor(X)              floor(X)
#define HDfmod(X,Y)             fmod(X,Y)
#define HDfopen(S,M)            fopen(S,M)
#define HDfork()                fork()
#define HDfpathconf(F,N)        fpathconf(F,N)
H5_DLL int HDfprintf (FILE *stream, const char *fmt, ...);
#define HDfputc(C,F)            fputc(C,F)
#define HDfputs(S,F)            fputs(S,F)
#define HDfread(M,Z,N,F)        fread(M,Z,N,F)
#define HDfree(M)               free(M)
#define HDfreopen(S,M,F)        freopen(S,M,F)
#define HDfrexp(X,N)            frexp(X,N)
/* Check for Cray-specific 'frexpf()' and 'frexpl()' routines */
#ifdef H5_HAVE_FREXPF
#define HDfrexpf(X,N)           frexpf(X,N)
#else /* H5_HAVE_FREXPF */
#define HDfrexpf(X,N)           frexp(X,N)
#endif /* H5_HAVE_FREXPF */
#ifdef H5_HAVE_FREXPL
#define HDfrexpl(X,N)           frexpl(X,N)
#else /* H5_HAVE_FREXPL */
#define HDfrexpl(X,N)           frexp(X,N)
#endif /* H5_HAVE_FREXPL */
/* fscanf() variable arguments */
#define HDfseek(F,O,W)          fseek(F,O,W)
#define HDfsetpos(F,P)          fsetpos(F,P)
/* definitions related to the file stat utilities */
#if defined( WIN32 ) && !defined( __CYGWIN__ )
     #ifdef __MWERKS__
     #define HDfstat(F,B)            fstat(F,B)
     typedef struct stat                h5_stat_t;
     #else /*MSVC*/
     #define HDfstat(F,B)               _fstati64(F,B)
     typedef struct _stati64            h5_stat_t;
     #endif
#else
#define HDfstat(F,B)            fstat(F,B)
typedef struct stat             h5_stat_t;
#endif

#define HDftell(F)              ftell(F)
#define HDftruncate(F,L)        ftruncate(F,L)
#define HDfwrite(M,Z,N,F)       fwrite(M,Z,N,F)
#define HDgetc(F)               getc(F)
#define HDgetchar()             getchar()
#define HDgetcwd(S,Z)           getcwd(S,Z)
#define HDgetegid()             getegid()
#define HDgetenv(S)             getenv(S)
#define HDgeteuid()             geteuid()
#define HDgetgid()              getgid()
#define HDgetgrgid(G)           getgrgid(G)
#define HDgetgrnam(S)           getgrnam(S)
#define HDgetgroups(Z,G)        getgroups(Z,G)
#define HDgetlogin()            getlogin()
#define HDgetpgrp()             getpgrp()
#define HDgetpid()              getpid()
#define HDgetppid()             getppid()
#define HDgetpwnam(S)           getpwnam(S)
#define HDgetpwuid(U)           getpwuid(U)
#define HDgetrusage(X,S)        getrusage(X,S)
#define HDgets(S)               gets(S)
#define HDgettimeofday(S,P)     gettimeofday(S,P)
#define HDgetuid()              getuid()
#define HDgmtime(T)             gmtime(T)
#define HDisalnum(C)            isalnum((int)(C)) /*cast for solaris warning*/
#define HDisalpha(C)            isalpha((int)(C)) /*cast for solaris warning*/
#define HDisatty(F)             isatty(F)
#define HDiscntrl(C)            iscntrl((int)(C)) /*cast for solaris warning*/
#define HDisdigit(C)            isdigit((int)(C)) /*cast for solaris warning*/
#define HDisgraph(C)            isgraph((int)(C)) /*cast for solaris warning*/
#define HDislower(C)            islower((int)(C)) /*cast for solaris warning*/
#define HDisprint(C)            isprint((int)(C)) /*cast for solaris warning*/
#define HDispunct(C)            ispunct((int)(C)) /*cast for solaris warning*/
#define HDisspace(C)            isspace((int)(C)) /*cast for solaris warning*/
#define HDisupper(C)            isupper((int)(C)) /*cast for solaris warning*/
#define HDisxdigit(C)           isxdigit((int)(C)) /*cast for solaris warning*/
#define HDkill(P,S)             kill(P,S)
#define HDlabs(X)               labs(X)
#define HDldexp(X,N)            ldexp(X,N)
#define HDldiv(X,Y)             ldiv(X,Y)
#define HDlink(OLD,NEW)         link(OLD,NEW)
#define HDlocaleconv()          localeconv()
#define HDlocaltime(T)          localtime(T)
#define HDlog(X)                log(X)
#define HDlog10(X)              log10(X)
#define HDlongjmp(J,N)          longjmp(J,N)
#if defined( WIN32 ) && !defined(__CYGWIN__)
     #ifdef __MWERKS__    
        #define HDlseek(F,O,W)  lseek(F,O,W)
     #else /*MSVS */
        #define HDlseek(F,O,W)  _lseeki64(F,O,W)
     #endif
#else
#define HDlseek(F,O,W)          lseek(F,O,W)
#endif
#define HDmalloc(Z)             malloc(Z)
#define HDmblen(S,N)            mblen(S,N)
#define HDmbstowcs(P,S,Z)       mbstowcs(P,S,Z)
#define HDmbtowc(P,S,Z)         mbtowc(P,S,Z)
#define HDmemchr(S,C,Z)         memchr(S,C,Z)
#define HDmemcmp(X,Y,Z)         memcmp(X,Y,Z)
/*
 * The (char*) casts are required for the DEC when optimizations are turned
 * on and the source and/or destination are not aligned.
 */
#define HDmemcpy(X,Y,Z)         memcpy((char*)(X),(const char*)(Y),Z)
#define HDmemmove(X,Y,Z)        memmove((char*)(X),(const char*)(Y),Z)
/*
 * The (void*) cast just avoids a compiler warning in WIN32
 */
#ifdef WIN32
#define HDmemset(X,C,Z)         memset((void*)(X),C,Z)
#else /* WIN32 */
#define HDmemset(X,C,Z)         memset(X,C,Z)
#endif /* WIN32 */
#ifdef WIN32
#define HDmkdir(S,M)            _mkdir(S)
#else /* WIN32 */
#define HDmkdir(S,M)            mkdir(S,M)
#endif /* WIN32 */
#define HDmkfifo(S,M)           mkfifo(S,M)
#define HDmktime(T)             mktime(T)
#define HDmodf(X,Y)             modf(X,Y)
#ifdef O_BINARY
#define HDopen(S,F,M)           open(S,F|_O_BINARY,M)
#else
#define HDopen(S,F,M)           open(S,F,M)
#endif
#define HDopendir(S)            opendir(S)
#define HDpathconf(S,N)         pathconf(S,N)
#define HDpause()               pause()
#define HDperror(S)             perror(S)
#define HDpipe(F)               pipe(F)
#define HDpow(X,Y)              pow(X,Y)
/* printf() variable arguments */
#define HDputc(C,F)             putc(C,F)
#define HDputchar(C)            putchar(C)
#define HDputs(S)               puts(S)
#define HDqsort(M,N,Z,F)        qsort(M,N,Z,F)
#define HDraise(N)              raise(N)
#define HDrand()                rand()
#ifdef  WIN32
#define HDrandom()              rand()
#else
#define HDrandom()              random()
#endif
#define HDread(F,M,Z)           read(F,M,Z)
#define HDreaddir(D)            readdir(D)
#define HDrealloc(M,Z)          realloc(M,Z)
#define HDremove(S)             remove(S)
#define HDrename(OLD,NEW)       rename(OLD,NEW)
#define HDrewind(F)             rewind(F)
#define HDrewinddir(D)          rewinddir(D)
#define HDrmdir(S)              rmdir(S)
/* scanf() variable arguments */
#define HDsetbuf(F,S)           setbuf(F,S)
#define HDsetgid(G)             setgid(G)
#define HDsetjmp(J)             setjmp(J)
#define HDsetlocale(N,S)        setlocale(N,S)
#define HDsetpgid(P,PG)         setpgid(P,PG)
#define HDsetsid()              setsid()
#define HDsetuid(U)             setuid(U)
#define HDsetvbuf(F,S,M,Z)      setvbuf(F,S,M,Z)
#define HDsigaction(N,A)        sigaction(N,A)
#define HDsigaddset(S,N)        sigaddset(S,N)
#define HDsigdelset(S,N)        sigdelset(S,N)
#define HDsigemptyset(S)        sigemptyset(S)
#define HDsigfillset(S)         sigfillset(S)
#define HDsigismember(S,N)      sigismember(S,N)
#define HDsiglongjmp(J,N)       siglongjmp(J,N)
#define HDsignal(N,F)           signal(N,F)
#define HDsigpending(S)         sigpending(S)
#define HDsigprocmask(H,S,O)    sigprocmask(H,S,O)
#define HDsigsetjmp(J,N)        sigsetjmp(J,N)
#define HDsigsuspend(S)         sigsuspend(S)
#define HDsin(X)                sin(X)
#define HDsinh(X)               sinh(X)
#define HDsleep(N)              sleep(N)
#ifdef H5_HAVE_SNPRINTF
#ifdef __WATCOMC__
#   define HDsnprintf           _snprintf /*varargs*/
#else /* __WATCOMC__ */
#   define HDsnprintf           snprintf /*varargs*/
#endif /* __WATCOMC__ */
#endif
/* sprintf() variable arguments */
#define HDsqrt(X)               sqrt(X)
#define HDsrand(N)              srand(N)
#ifdef WIN32
#define HDsrandom(N)            srand(N)
#else
#define HDsrandom(N)            srandom(N)
#endif
/* sscanf() variable arguments */


#ifdef WIN32
     #ifdef __MWERKS__
     #define HDstat(S,B)   stat(S,B)
     #else /*MSVC*/
     #define HDstat(S,B)                _stati64(S,B)
     #endif
#else
#define HDstat(S,B)  stat(S,B)
#endif

#define HDstrcat(X,Y)           strcat(X,Y)
#define HDstrchr(S,C)           strchr(S,C)
#define HDstrcmp(X,Y)           strcmp(X,Y)
#define HDstrcoll(X,Y)          strcoll(X,Y)
#define HDstrcpy(X,Y)           strcpy(X,Y)
#define HDstrcspn(X,Y)          strcspn(X,Y)
#define HDstrerror(N)           strerror(N)
#define HDstrftime(S,Z,F,T)     strftime(S,Z,F,T)
#define HDstrlen(S)             strlen(S)
#define HDstrncat(X,Y,Z)        strncat(X,Y,Z)
#define HDstrncmp(X,Y,Z)        strncmp(X,Y,Z)
#define HDstrncpy(X,Y,Z)        strncpy(X,Y,Z)
#define HDstrpbrk(X,Y)          strpbrk(X,Y)
#define HDstrrchr(S,C)          strrchr(S,C)
#define HDstrspn(X,Y)           strspn(X,Y)
#define HDstrstr(X,Y)           strstr(X,Y)
#define HDstrtod(S,R)           strtod(S,R)
#define HDstrtok(X,Y)           strtok(X,Y)
#define HDstrtol(S,R,N)         strtol(S,R,N)
H5_DLL int64_t HDstrtoll (const char *s, const char **rest, int base);
#define HDstrtoul(S,R,N)        strtoul(S,R,N)
#define HDstrxfrm(X,Y,Z)        strxfrm(X,Y,Z)
#define HDsysconf(N)            sysconf(N)
#define HDsystem(S)             system(S)
#define HDtan(X)                tan(X)
#define HDtanh(X)               tanh(X)
#define HDtcdrain(F)            tcdrain(F)
#define HDtcflow(F,A)           tcflow(F,A)
#define HDtcflush(F,N)          tcflush(F,N)
#define HDtcgetattr(F,T)        tcgetattr(F,T)
#define HDtcgetpgrp(F)          tcgetpgrp(F)
#define HDtcsendbreak(F,N)      tcsendbreak(F,N)
#define HDtcsetattr(F,O,T)      tcsetattr(F,O,T)
#define HDtcsetpgrp(F,N)        tcsetpgrp(F,N)
#define HDtime(T)               time(T)
#define HDtimes(T)              times(T)
#define HDtmpfile()             tmpfile()
#define HDtmpnam(S)             tmpnam(S)
#define HDtolower(C)            tolower(C)
#define HDtoupper(C)            toupper(C)
#define HDttyname(F)            ttyname(F)
#define HDtzset()               tzset()
#define HDumask(N)              umask(N)
#define HDuname(S)              uname(S)
#define HDungetc(C,F)           ungetc(C,F)
#define HDunlink(S)             unlink(S)
#define HDutime(S,T)            utime(S,T)
#define HDva_arg(A,T)           va_arg(A,T)
#define HDva_end(A)             va_end(A)
#define HDva_start(A,P)         va_start(A,P)
#define HDvfprintf(F,FMT,A)     vfprintf(F,FMT,A)
#define HDvprintf(FMT,A)        vprintf(FMT,A)
#define HDvsprintf(S,FMT,A)     vsprintf(S,FMT,A)
#ifdef H5_HAVE_VSNPRINTF
#   define HDvsnprintf(S,N,FMT,A) vsnprintf(S,N,FMT,A)
#endif
#define HDwait(W)               wait(W)
#define HDwaitpid(P,W,O)        waitpid(P,W,O)
#define HDwcstombs(S,P,Z)       wcstombs(S,P,Z)
#define HDwctomb(S,C)           wctomb(S,C)

#if defined (__MWERKS__)
/* workaround for a bug in the Metrowerks version 6.0 header file for write
 which is not defined as const void*
 */
#define HDwrite(F,M,Z)          write(F,(void*)M,Z)
#else
#define HDwrite(F,M,Z)          write(F,M,Z)
#endif

/*
 * And now for a couple non-Posix functions...  Watch out for systems that
 * define these in terms of macros.
 */
#ifdef WIN32
#define HDstrdup(S)    _strdup(S)
#else /* WIN32 */

#if !defined strdup && !defined H5_HAVE_STRDUP 
extern char *strdup(const char *s);
#endif

#define HDstrdup(S)     strdup(S)

#endif /* WIN32 */


#ifndef H5_HAVE_SNPRINTF
H5_DLL int HDsnprintf(char *buf, size_t size, const char *fmt, ...);
#endif
#ifndef H5_HAVE_VSNPRINTF
H5_DLL int HDvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
#endif

/*
 * These macros check whether debugging has been requested for a certain
 * package at run-time.  Code for debugging is conditionally compiled by
 * defining constants like `H5X_DEBUG'.  In order to see the output though
 * the code must be enabled at run-time with an environment variable
 * HDF5_DEBUG which is a list of packages to debug.
 *
 * Note:  If you add/remove items from this enum then be sure to update the
 *        information about the package in H5_init_library().
 */
typedef enum {
    H5_PKG_A,                           /*Attributes                    */
    H5_PKG_AC,                          /*Meta data cache               */
    H5_PKG_B,                           /*B-trees                       */
    H5_PKG_D,                           /*Datasets                      */
    H5_PKG_E,                           /*Error handling                */
    H5_PKG_F,                           /*Files                         */
    H5_PKG_G,                           /*Groups                        */
    H5_PKG_HG,                          /*Global heap                   */
    H5_PKG_HL,                          /*Local heap                    */
    H5_PKG_I,                           /*Interface                     */
    H5_PKG_MF,                          /*File memory management        */
    H5_PKG_MM,                          /*Core memory management        */
    H5_PKG_O,                           /*Object headers                */
    H5_PKG_P,                           /*Property lists                */
    H5_PKG_S,                           /*Data spaces                   */
    H5_PKG_T,                           /*Data types                    */
    H5_PKG_V,                           /*Vector functions              */
    H5_PKG_Z,                           /*Raw data filters              */
    H5_NPKGS                            /*Must be last                  */
} H5_pkg_t;

typedef struct H5_debug_t {
    FILE                *trace;         /*API trace output stream       */
    hbool_t             ttop;           /*Show only top-level calls?    */
    hbool_t             ttimes;         /*Show trace event times?       */
    struct {
        const char      *name;          /*package name                  */
        FILE            *stream;        /*output stream or NULL         */
    } pkg[H5_NPKGS];
} H5_debug_t;

extern H5_debug_t               H5_debug_g;
#define H5DEBUG(X)              (H5_debug_g.pkg[H5_PKG_##X].stream)

/*-------------------------------------------------------------------------
 * Purpose:     These macros are inserted automatically just after the
 *              FUNC_ENTER() macro of API functions and are used to trace
 *              application program execution. Unless H5_DEBUG_API has been
 *              defined they are no-ops.
 *
 * Arguments:   R       - Return type encoded as a string
 *              T       - Argument types encoded as a string
 *              A0-An   - Arguments.  The number at the end of the macro name
 *                        indicates the number of arguments.
 *
 * Programmer:  Robb Matzke
 *
 * Modifications:
 *------------------------------------------------------------------------- 
 */
#ifdef H5_DEBUG_API
#define H5TRACE_DECL                       const char *RTYPE=NULL;                                      \
                                           double CALLTIME;
#define H5TRACE0(R,T)                      RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T)
#define H5TRACE1(R,T,A0)                   RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0)
#define H5TRACE2(R,T,A0,A1)                RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1)
#define H5TRACE3(R,T,A0,A1,A2)             RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2)
#define H5TRACE4(R,T,A0,A1,A2,A3)          RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3)
#define H5TRACE5(R,T,A0,A1,A2,A3,A4)       RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,   \
                                                             #A4,A4)
#define H5TRACE6(R,T,A0,A1,A2,A3,A4,A5)    RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,   \
                                                             #A4,A4,#A5,A5)
#define H5TRACE7(R,T,A0,A1,A2,A3,A4,A5,A6) RTYPE=R;                                                     \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,   \
                                                             #A4,A4,#A5,A5,#A6,A6)
#define H5TRACE8(R,T,A0,A1,A2,A3,A4,A5,A6,A7) RTYPE=R;                                                  \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,   \
                                                             #A4,A4,#A5,A5,#A6,A6,#A7,A7)
#define H5TRACE9(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8) RTYPE=R;                                               \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,   \
                                                             #A4,A4,#A5,A5,#A6,A6,#A7,A7,#A8,A8)
#define H5TRACE10(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9) RTYPE=R;                                               \
                                           CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,   \
                                                             #A4,A4,#A5,A5,#A6,A6,#A7,A7,#A8,A8,#A9,A9)
#define H5TRACE_RETURN(V)                  if (RTYPE) {                                                 \
                                              H5_trace(&CALLTIME,FUNC,RTYPE,NULL,V);                    \
                                              RTYPE=NULL;                                               \
                                           }
#else
#define H5TRACE_DECL                                    /*void*/
#define H5TRACE0(R,T)                                   /*void*/
#define H5TRACE1(R,T,A0)                                /*void*/
#define H5TRACE2(R,T,A0,A1)                             /*void*/
#define H5TRACE3(R,T,A0,A1,A2)                          /*void*/
#define H5TRACE4(R,T,A0,A1,A2,A3)                       /*void*/
#define H5TRACE5(R,T,A0,A1,A2,A3,A4)                    /*void*/
#define H5TRACE6(R,T,A0,A1,A2,A3,A4,A5)                 /*void*/
#define H5TRACE7(R,T,A0,A1,A2,A3,A4,A5,A6)              /*void*/
#define H5TRACE8(R,T,A0,A1,A2,A3,A4,A5,A6,A7)           /*void*/
#define H5TRACE9(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8)        /*void*/
#define H5TRACE10(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9)    /*void*/
#define H5TRACE_RETURN(V)                               /*void*/
#endif

H5_DLL double H5_trace(double *calltime, const char *func, const char *type, ...);


/*-------------------------------------------------------------------------
 * Purpose:     Register function entry for library initialization and code
 *              profiling.
 *
 * Notes:       Every file must have a file-scope variable called
 *              `initialize_interface_g' of type hbool_t which is initialized
 *              to FALSE.
 *
 *              Don't use local variable initializers which contain
 *              calls to other library functions since the initializer
 *              would happen before the FUNC_ENTER() gets called.  Don't
 *              use initializers that require special cleanup code to
 *              execute if FUNC_ENTER() fails since a failing FUNC_ENTER()
 *              returns immediately without branching to the `done' label.
 *
 * Programmer:  Quincey Koziol
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Aug 1997
 *      The `interface_init_func' can be the null pointer.  Changed
 *      HGOTO_ERROR() to HRETURN_ERROR() since no clean-up needs to occur
 *      when an error is detected at this point since this must be the
 *      first executable statement in a function.  This allows functions
 *      to omit the `done:' label when convenient to do so.
 *
 *      Robb Matzke, 4 Aug 1997
 *      The pablo mask comes from the constant PABLO_MASK defined on
 *      a per-file basis.  The `pablo_func_id' is generated from the
 *      `func_name' argument by prepending an `ID_' to the name.  The
 *      pablo function identifier should be saved in a local variable
 *      so FUNC_LEAVE() can access it.
 *
 *      Robb Matzke, 4 Aug 1997
 *      It is safe to call this function even inside various library
 *      initializing functions.  Infinite recursion is no longer a
 *      danger.
 *
 *      Robb Matzke, 3 Dec 1997
 *      The interface initialization function is no longer passed as an
 *      argument unless the `FUNC_ENTER_INIT' form is called.  Instead, the
 *      function comes from the `INTERFACE_INIT' constant which must be
 *      defined in every source file.
 *
 *      Robb Matzke, 17 Jun 1998
 *      Added auto variable RTYPE which is initialized by the tracing macros.
 *
 *      Quincey Koziol, 28 May 2002
 *      Split FUNC_ENTER macro into FUNC_ENTER_API, FUNC_ENTER_NOAPI and
 *      FUNC_ENTER_NOAPI_NOINIT.
 *-------------------------------------------------------------------------
 */

/* `S' is the name of a function which is being tested to check if its */
/*      an API function */
#define H5_IS_API(S) ('_'!=S[2] && '_'!=S[3] && (!S[4] || '_'!=S[4]))

/* global library version information string */
extern char     H5_lib_vers_info_g[];

/* Lock headers */
#ifdef H5_HAVE_THREADSAFE

/* Include required thread-safety header */
#include "H5TSprivate.h"

/* replacement structure for original global variable */
typedef struct H5_api_struct {
    H5TS_mutex_t init_lock;  /* API entrance mutex */
    hbool_t H5_libinit_g;    /* Has the library been initialized? */
} H5_api_t;

/* Macros for accessing the global variables */
#define H5_INIT_GLOBAL H5_g.H5_libinit_g

/* Macro for first thread initialization */
#define H5_FIRST_THREAD_INIT                                                  \
   pthread_once(&H5TS_first_init_g, H5TS_first_thread_init);

/* Macros for threadsafe HDF-5 Phase I locks */
#define H5_API_LOCK                                                           \
     H5TS_mutex_lock(&H5_g.init_lock);
#define H5_API_UNLOCK                                                         \
     H5TS_mutex_unlock(&H5_g.init_lock);

/* Macros for thread cancellation-safe mechanism */
#define H5_API_UNSET_CANCEL                                                   \
    H5TS_cancel_count_inc();

#define H5_API_SET_CANCEL                                                     \
    H5TS_cancel_count_dec();

extern H5_api_t H5_g;

#else /* H5_HAVE_THREADSAFE */

/* disable any first thread init mechanism */
#define H5_FIRST_THREAD_INIT

/* disable locks (sequential version) */
#define H5_API_LOCK
#define H5_API_UNLOCK

/* disable cancelability (sequential version) */
#define H5_API_UNSET_CANCEL
#define H5_API_SET_CANCEL

/* extern global variables */
extern hbool_t H5_libinit_g;    /* Has the library been initialized? */


/* Macros for accessing the global variables */
#define H5_INIT_GLOBAL H5_libinit_g

#endif /* H5_HAVE_THREADSAFE */

#ifdef H5_HAVE_FUNCSTACK

/* Include required function stack header */
#include "H5FSprivate.h"

#define H5_PUSH_FUNC(func_name) H5FS_push(#func_name)
#define H5_POP_FUNC             H5FS_pop()
#else /* H5_HAVE_FUNCSTACK */
#define H5_PUSH_FUNC(func_name) /* void */
#define H5_POP_FUNC             /* void */
#endif /* H5_HAVE_FUNCSTACK */

#ifdef H5_HAVE_MPE
extern hbool_t H5_MPEinit_g;   /* Has the MPE Library been initialized? */
#endif

#define FUNC_ENTER_COMMON_NOFUNC(func_name,asrt)                              \
   PABLO_SAVE (ID_ ## func_name)                                              \
                                                                              \
   /* Check API status */                                                     \
   assert(asrt);                                                              \
                                                                              \
   /* Start tracing */                                                        \
   PABLO_TRACE_ON (PABLO_MASK, pablo_func_id)

#define FUNC_ENTER_COMMON(func_name,asrt)                                     \
   CONSTR (FUNC, #func_name);                                                 \
   FUNC_ENTER_COMMON_NOFUNC(func_name,asrt);                                  \
   (void)FUNC;                                                                \

/* Threadsafety initialization code for API routines */
#define FUNC_ENTER_API_THREADSAFE                                             \
   /* Initialize the thread-safe code */                                      \
   H5_FIRST_THREAD_INIT                                                       \
                                                                              \
   /* Grab the mutex for the library */                                       \
   H5_API_UNSET_CANCEL                                                        \
   H5_API_LOCK

/* Threadsafety termination code for API routines */
#define FUNC_LEAVE_API_THREADSAFE                                             \
    H5_API_UNLOCK                                                             \
    H5_API_SET_CANCEL

/* Local variables for API routines */
#define FUNC_ENTER_API_VARS(func_name)                                        \
    MPE_LOG_VARS(func_name)                                                   \
    H5TRACE_DECL

/* Use this macro for all "normal" API functions */
#define FUNC_ENTER_API(func_name,err) {{                                      \
    FUNC_ENTER_API_VARS(func_name)                                            \
    FUNC_ENTER_COMMON(func_name,H5_IS_API(#func_name));                       \
    FUNC_ENTER_API_THREADSAFE;                                                \
    FUNC_ENTER_API_COMMON(func_name,INTERFACE_INIT,err);                      \
    /* Clear thread error stack entering public functions */                  \
    H5E_clear();                                                              \
    {

/*
 * Use this macro for API functions that shouldn't clear the error stack
 *      like H5Eprint and H5Ewalk.
 */
#define FUNC_ENTER_API_NOCLEAR(func_name,err) {{                              \
    FUNC_ENTER_API_VARS(func_name)                                            \
    FUNC_ENTER_COMMON(func_name,H5_IS_API(#func_name));                       \
    FUNC_ENTER_API_THREADSAFE;                                                \
    FUNC_ENTER_API_COMMON(func_name,INTERFACE_INIT,err);                      \
    {

/*
 * Use this macro for API functions that shouldn't perform _any_ initialization
 *      of the library or an interface, just perform tracing, etc.  Examples
 *      are: H5close, H5check_version, H5Eget_major, H5Eget_minor.
 *
 */
#define FUNC_ENTER_API_NOINIT(func_name) {{                                   \
    FUNC_ENTER_API_VARS(func_name)                                            \
    FUNC_ENTER_COMMON(func_name,H5_IS_API(#func_name));                       \
    FUNC_ENTER_API_THREADSAFE;                                                \
    H5_PUSH_FUNC(func_name);                                                  \
    BEGIN_MPE_LOG(func_name);                                                 \
    {

/* Use this macro for all "normal" non-API functions */
#define FUNC_ENTER_NOAPI(func_name,err) {                                     \
    FUNC_ENTER_COMMON(func_name,!H5_IS_API(#func_name));                      \
    FUNC_ENTER_NOAPI_INIT(func_name,INTERFACE_INIT,err)                       \
    {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - static functions, since they must be called from a function in the
 *              interface, the library and interface must already be
 *              initialized.
 *      - functions which are called during library shutdown, since we don't
 *              want to re-initialize the library.
 */
#define FUNC_ENTER_NOAPI_NOINIT(func_name) {                                        \
    FUNC_ENTER_COMMON(func_name,!H5_IS_API(#func_name));                      \
    H5_PUSH_FUNC(func_name);                                                  \
    {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - static functions, since they must be called from a function in the
 *              interface, the library and interface must already be
 *              initialized.
 *      - functions which are called during library shutdown, since we don't
 *              want to re-initialize the library.
 * 
 * This macro is used for functions which fit the above categories _and_
 * also don't use the 'FUNC' variable (i.e. don't push errors on the error stack)
 * 
 */
#define FUNC_ENTER_NOAPI_NOINIT_NOFUNC(func_name) {                           \
    FUNC_ENTER_COMMON_NOFUNC(func_name,!H5_IS_API(#func_name));               \
    H5_PUSH_FUNC(func_name);                                                  \
    {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - functions which shouldn't push their name on the function stack
 *              (so far, just the H5FS routines themselves)
 */
#define FUNC_ENTER_NOAPI_NOFS(func_name) {                                    \
    FUNC_ENTER_COMMON(func_name,!H5_IS_API(#func_name));                      \
    {

#define FUNC_ENTER_API_COMMON(func_name,interface_init_func,err)              \
   /* Initialize the library */                                               \
   if (!(H5_INIT_GLOBAL)) {                                                   \
       H5_INIT_GLOBAL = TRUE;                                                 \
       if (H5_init_library()<0)                                               \
          HGOTO_ERROR (H5E_FUNC, H5E_CANTINIT, err,                           \
            "library initialization failed");                                 \
   }                                                                          \
                                                                              \
   /* Initialize this interface or bust */                                    \
   if (!interface_initialize_g) {                                             \
      interface_initialize_g = 1;                                             \
      if (interface_init_func &&                                              \
              ((herr_t(*)(void))interface_init_func)()<0) {                   \
         interface_initialize_g = 0;                                          \
         HGOTO_ERROR (H5E_FUNC, H5E_CANTINIT, err,                            \
            "interface initialization failed");                               \
      }                                                                       \
   }                                                                          \
                                                                              \
   /* Push the name of this function on the function stack */                 \
   H5_PUSH_FUNC(func_name);                                                   \
                                                                              \
   BEGIN_MPE_LOG(func_name)

#define FUNC_ENTER_NOAPI_INIT(func_name,interface_init_func,err)              \
   /* Initialize this interface or bust */                                    \
   if (!interface_initialize_g) {                                             \
      interface_initialize_g = 1;                                             \
      if (interface_init_func &&                                              \
              ((herr_t(*)(void))interface_init_func)()<0) {                   \
         interface_initialize_g = 0;                                          \
         HGOTO_ERROR (H5E_FUNC, H5E_CANTINIT, err,                            \
            "interface initialization failed");                               \
      }                                                                       \
   }                                                                          \
                                                                              \
   /* Push the name of this function on the function stack */                 \
   H5_PUSH_FUNC(func_name);

/*-------------------------------------------------------------------------
 * Purpose:     Register function exit for code profiling.  This should be
 *              the last statement executed by a function.
 *
 * Programmer:  Quincey Koziol
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Aug 1997
 *      The pablo mask comes from the constant PABLO_MASK defined on a
 *      per-file basis.  The pablo_func_id comes from an auto variable
 *      defined by FUNC_ENTER.
 *
 *-------------------------------------------------------------------------
 */
#define FUNC_LEAVE_API(ret_value)                                             \
        FINISH_MPE_LOG;                                                       \
        PABLO_TRACE_OFF (PABLO_MASK, pablo_func_id);                          \
        H5TRACE_RETURN(ret_value);                                            \
        H5_POP_FUNC;                                                          \
        FUNC_LEAVE_API_THREADSAFE                                             \
        return (ret_value);                                                   \
    } /*end scope from end of FUNC_ENTER*/                                    \
}} /*end scope from beginning of FUNC_ENTER*/

#define FUNC_LEAVE_NOAPI(ret_value)                                           \
        PABLO_TRACE_OFF (PABLO_MASK, pablo_func_id);                          \
        H5_POP_FUNC;                                                          \
        return (ret_value);                                                   \
    } /*end scope from end of FUNC_ENTER*/                                    \
} /*end scope from beginning of FUNC_ENTER*/

#define FUNC_LEAVE_NOAPI_VOID                                                 \
        PABLO_TRACE_OFF (PABLO_MASK, pablo_func_id);                          \
        H5_POP_FUNC;                                                          \
        return;                                                               \
    } /*end scope from end of FUNC_ENTER*/                                    \
} /*end scope from beginning of FUNC_ENTER*/

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - functions which didn't push their name on the function stack
 *              (so far, just the H5FS routines themselves)
 */
#define FUNC_LEAVE_NOAPI_NOFS(ret_value)                                      \
        PABLO_TRACE_OFF (PABLO_MASK, pablo_func_id);                          \
        return (ret_value);                                                   \
    } /*end scope from end of FUNC_ENTER*/                                    \
} /*end scope from beginning of FUNC_ENTER*/

/*
 * The FUNC_ENTER() and FUNC_LEAVE() macros make calls to Pablo functions
 * through one of these two sets of macros.
 */
#ifdef H5_HAVE_PABLO
#  define PABLO_SAVE(func_id)   const int pablo_func_id = func_id;
#  define PABLO_TRACE_ON(m, f)  TRACE_ON(m,f)
#  define PABLO_TRACE_OFF(m, f) TRACE_OFF(m,f)
#else
#  define PABLO_SAVE(func_id)   /*void */
#  define PABLO_TRACE_ON(m, f)  /*void */
#  define PABLO_TRACE_OFF(m, f) /*void */
#endif

/* Macro for "glueing" together items, for re-scanning macros */
#define H5_GLUE(x,y)       x##y
#define H5_GLUE3(x,y,z)    x##y##z

/* Private functions, not part of the publicly documented API */
H5_DLL herr_t H5_init_library(void);
H5_DLL void H5_term_library(void);

/* Functions to terminate interfaces */
H5_DLL int H5A_term_interface(void);
H5_DLL int H5AC_term_interface(void);
H5_DLL int H5D_term_interface(void);
H5_DLL int H5F_term_interface(void);
H5_DLL int H5G_term_interface(void);
H5_DLL int H5I_term_interface(void);
H5_DLL int H5P_term_interface(void);
H5_DLL int H5R_term_interface(void);
H5_DLL int H5S_term_interface(void);
H5_DLL int H5TN_term_interface(void);
H5_DLL int H5T_term_interface(void);
H5_DLL int H5Z_term_interface(void);



#endif
