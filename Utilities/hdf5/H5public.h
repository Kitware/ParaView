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

/* Id */


/*
 * This file contains public declarations for the HDF5 module.
 */
#ifndef _H5public_H
#define _H5public_H

/* Include files for public use... */
/*
 * Since H5pubconf.h is a generated header file, it is messy to try
 * to put a #ifndef _H5pubconf_H ... #endif guard in it.
 * HDF5 has set an internal rule that it is being included here.
 * Source files should NOT include H5pubconf.h directly but include
 * it via H5public.h.  The #ifndef _H5public_H guard above would
 * prevent repeated include.
 */
#include "H5pubconf.h"		/*from configure                             */

#ifdef H5_HAVE_FEATURES_H
#include <features.h>           /*for setting POSIX, BSD, etc. compatibility */
#endif
#ifdef H5_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef H5_STDC_HEADERS
#   include <limits.h>		/*for H5T_NATIVE_CHAR defn in H5Tpublic.h    */
#endif
#ifdef H5_HAVE_STDINT_H
#   include <stdint.h>		/*for C9x types				     */
#endif
#ifdef H5_HAVE_INTTYPES_H 
#   include <inttypes.h>        /* For uint64_t on some platforms            */ 
#endif
#ifdef H5_HAVE_STDDEF_H
#   include <stddef.h>
#endif
#ifdef H5_HAVE_PARALLEL
#   include <mpi.h>
#ifndef MPI_FILE_NULL		/*MPIO may be defined in mpi.h already       */
#   include <mpio.h>
#endif
#endif

#ifdef H5_HAVE_GASS             /*for Globus GASS I/O                        */
#include "globus_common.h"
#include "globus_gass_file.h"
#endif

#ifdef H5_HAVE_SRB              /*for SRB I/O                                */
#include <srbClient.h>
#endif

#include "H5api_adpt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Version numbers */
#define H5_VERS_MAJOR	1	/* For major interface/format changes  	     */
#define H5_VERS_MINOR	6	/* For minor interface/format changes  	     */
#define H5_VERS_RELEASE	2	/* For tweaks, bug-fixes, or development     */
#define H5_VERS_SUBRELEASE ""	/* For pre-releases like snap0       */
				/* Empty string for real releases.           */
#define H5_VERS_INFO    "HDF5 library version: 1.6.2"      /* Full version string */

#define H5check()	H5check_version(H5_VERS_MAJOR,H5_VERS_MINOR,	      \
				        H5_VERS_RELEASE)

/*
 * Status return values.  Failed integer functions in HDF5 result almost
 * always in a negative value (unsigned failing functions sometimes return
 * zero for failure) while successfull return is non-negative (often zero).
 * The negative failure value is most commonly -1, but don't bet on it.  The
 * proper way to detect failure is something like:
 *
 * 	if ((dset = H5Dopen (file, name))<0) {
 *	    fprintf (stderr, "unable to open the requested dataset\n");
 *	}
 */
typedef int herr_t;


/*
 * Boolean type.  Successful return values are zero (false) or positive
 * (true). The typical true value is 1 but don't bet on it.  Boolean
 * functions cannot fail.  Functions that return `htri_t' however return zero
 * (false), positive (true), or negative (failure). The proper way to test
 * for truth from a htri_t function is:
 *
 * 	if ((retval = H5Tcommitted(type))>0) {
 *	    printf("data type is committed\n");
 *	} else if (!retval) {
 * 	    printf("data type is not committed\n");
 *	} else {
 * 	    printf("error determining whether data type is committed\n");
 *	}
 */
typedef unsigned int hbool_t;
typedef int htri_t;

/* Define the ssize_t type if it not is defined */
#if H5_SIZEOF_SSIZE_T==0
/* Undefine this size, we will re-define it in one of the sections below */
#undef H5_SIZEOF_SSIZE_T
#if H5_SIZEOF_SIZE_T==H5_SIZEOF_INT
typedef int ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF_INT
#elif H5_SIZEOF_SIZE_T==H5_SIZEOF_LONG
typedef long ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF_LONG
#elif H5_SIZEOF_SIZE_T==H5_SIZEOF_LONG_LONG
typedef long long ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF_LONG_LONG
#elif H5_SIZEOF_SIZE_T==H5_SIZEOF___INT64
typedef __int64 ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF___INT64
#else /* Can't find matching type for ssize_t */
#   error "nothing appropriate for ssize_t"
#endif
#endif

/*
 * The sizes of file objects have their own types defined here.  If large
 * sizes are enabled then use a 64-bit data type, otherwise use the size of
 * memory objects.
 */
#ifdef H5_HAVE_LARGE_HSIZET
#   if H5_SIZEOF_LONG_LONG>=8
typedef unsigned long long 	hsize_t;
typedef signed long long	hssize_t;
#       define H5_SIZEOF_HSIZE_T H5_SIZEOF_LONG_LONG
#   elif H5_SIZEOF___INT64>=8
typedef unsigned __int64	hsize_t;
typedef signed __int64		hssize_t;
#       define H5_SIZEOF_HSIZE_T H5_SIZEOF___INT64
#   endif
#else /* H5_HAVE_LARGE_HSIZET */
typedef size_t			hsize_t;
typedef ssize_t			hssize_t;
#       define H5_SIZEOF_HSIZE_T H5_SIZEOF_SIZE_T
#endif /* H5_HAVE_LARGE_HSIZET */

/*
 * File addresses have there own types.
 */
#if H5_SIZEOF_UINT64_T>=8
    typedef uint64_t                haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(int64_t)(-1))
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_LONG_LONG_INT
#   endif  /* H5_HAVE_PARALLEL */
#elif H5_SIZEOF_INT>=8
    typedef unsigned                haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(-1))
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_UNSIGNED
#   endif  /* H5_HAVE_PARALLEL */
#elif H5_SIZEOF_LONG>=8
    typedef unsigned long           haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(long)(-1))
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_UNSIGNED_LONG
#   endif  /* H5_HAVE_PARALLEL */
#elif H5_SIZEOF_LONG_LONG>=8
    typedef unsigned long long      haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(long long)(-1))
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_LONG_LONG_INT
#   endif  /* H5_HAVE_PARALLEL */
#elif H5_SIZEOF___INT64>=8
    typedef unsigned __int64        haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(__int64)(-1))
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_LONG_LONG_INT
#   endif  /* H5_HAVE_PARALLEL */
#else
#   error "nothing appropriate for haddr_t"
#endif
#define HADDR_MAX		(HADDR_UNDEF-1)

/* Functions in H5.c */
H5_DLL herr_t H5open(void);
H5_DLL herr_t H5close(void);
H5_DLL herr_t H5dont_atexit(void);
H5_DLL herr_t H5garbage_collect(void);
H5_DLL herr_t H5set_free_list_limits (int reg_global_lim, int reg_list_lim,
                int arr_global_lim, int arr_list_lim, int blk_global_lim,
                int blk_list_lim);
H5_DLL herr_t H5get_libversion(unsigned *majnum, unsigned *minnum,
				unsigned *relnum);
H5_DLL herr_t H5check_version(unsigned majnum, unsigned minnum,
			       unsigned relnum);

#ifdef __cplusplus
}
#endif
#endif
