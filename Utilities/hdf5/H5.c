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

#define H5I_PACKAGE             /*suppress error about including H5Ipkg   */

#include "H5private.h"          /* Generic Functions                    */
#include "H5Bprivate.h"         /* B-link trees                         */
#include "H5Dprivate.h"         /* Datasets                             */
#include "H5Eprivate.h"         /* Error handling                       */
#include "H5FDprivate.h"        /* File drivers                         */
#include "H5FLprivate.h"        /* Free lists                           */
#include "H5Ipkg.h"             /* IDs                                  */
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Rpublic.h"          /* References                           */
#include "H5Sprivate.h"         /* Dataspaces                           */
#include "H5Tprivate.h"         /* Datatypes                            */
#include "H5Zprivate.h"         /* I/O pipeline filters                 */

/* datatypes of predefined drivers needed by H5_trace() */
#include "H5FDmpio.h"

/* we need this for the struct rusage declaration */
#if defined(H5_HAVE_GETRUSAGE) && defined(H5_HAVE_SYS_RESOURCE_H)
#   include <sys/resource.h>
#endif

#define PABLO_MASK      H5_mask

/* statically initialize block for pthread_once call used in initializing */
/* the first global mutex                                                 */
#ifdef H5_HAVE_THREADSAFE
H5_api_t H5_g;
#else
hbool_t H5_libinit_g = FALSE;   /* Library hasn't been initialized */
#endif

#ifdef H5_HAVE_MPE
hbool_t H5_MPEinit_g = FALSE;   /* MPE Library hasn't been initialized */
#endif

char                    H5_lib_vers_info_g[] = H5_VERS_INFO;
hbool_t                 dont_atexit_g = FALSE;
H5_debug_t              H5_debug_g;             /*debugging info        */
static void             H5_debug_mask(const char*);

/* Interface initialization */
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

/*--------------------------------------------------------------------------
 * NAME
 *   H5_init_library -- Initialize library-global information
 * USAGE
 *    herr_t H5_init_library()
 *   
 * RETURNS
 *    Non-negative on success/Negative on failure
 *
 * DESCRIPTION
 *    Initializes any library-global data or routines.
 *
 *--------------------------------------------------------------------------
 */
herr_t 
H5_init_library(void)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5_init_library, FAIL);
    /*
     * Make sure the package information is updated.
     */
    HDmemset(&H5_debug_g, 0, sizeof H5_debug_g);
    H5_debug_g.pkg[H5_PKG_A].name = "a";
    H5_debug_g.pkg[H5_PKG_AC].name = "ac";
    H5_debug_g.pkg[H5_PKG_B].name = "b";
    H5_debug_g.pkg[H5_PKG_D].name = "d";
    H5_debug_g.pkg[H5_PKG_E].name = "e";
    H5_debug_g.pkg[H5_PKG_F].name = "f";
    H5_debug_g.pkg[H5_PKG_G].name = "g";
    H5_debug_g.pkg[H5_PKG_HG].name = "hg";
    H5_debug_g.pkg[H5_PKG_HL].name = "hl";
    H5_debug_g.pkg[H5_PKG_I].name = "i";
    H5_debug_g.pkg[H5_PKG_MF].name = "mf";
    H5_debug_g.pkg[H5_PKG_MM].name = "mm";
    H5_debug_g.pkg[H5_PKG_O].name = "o";
    H5_debug_g.pkg[H5_PKG_P].name = "p";
    H5_debug_g.pkg[H5_PKG_S].name = "s";
    H5_debug_g.pkg[H5_PKG_T].name = "t";
    H5_debug_g.pkg[H5_PKG_V].name = "v";
    H5_debug_g.pkg[H5_PKG_Z].name = "z";

#ifdef H5_HAVE_MPE
    /* Initialize MPE instrumentation library.  May need to move this
     * up earlier if any of the above initialization involves using
     * the instrumentation code.
     */
    if (!H5_MPEinit_g)
    {
        int mpe_code;
        int mpi_initialized;
        MPI_Initialized(&mpi_initialized);
        if (mpi_initialized){
            mpe_code = MPE_Init_log();
            assert(mpe_code >=0);
            H5_MPEinit_g = TRUE;
        }
    }
#endif

    /*
     * Install atexit() library cleanup routine unless the H5dont_atexit()
     * has been called.  Once we add something to the atexit() list it stays
     * there permanently, so we set dont_atexit_g after we add it to prevent
     * adding it again later if the library is cosed and reopened.
     */
    if (!dont_atexit_g) {
        HDatexit(H5_term_library);
        dont_atexit_g = TRUE;
    }

    /*
     * Initialize interfaces that might not be able to initialize themselves
     * soon enough.  The file & dataset interfaces must be initialized because
     * calling H5P_create() might require the file/dataset property classes to be
     * initialized.  The property interface must be initialized before the file 
     * & dataset interfaces though, in order to provide them with the proper
     * property classes.
     */
    if (H5P_init()<0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize property list interface");
    if (H5F_init()<0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize file interface");
    if (H5T_init()<0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize datatype interface");
    if (H5D_init()<0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize dataset interface");
    if (H5AC_init()<0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize metadata caching interface");

    /* Debugging? */
    H5_debug_mask("-all");
    H5_debug_mask(HDgetenv("HDF5_DEBUG"));

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5_term_library
 *
 * Purpose:     Terminate interfaces in a well-defined order due to
 *              dependencies among the interfaces, then terminate
 *              library-specific data.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Friday, November 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_term_library(void)
{
    int pending, ntries=0, n;
    unsigned    at=0;
    char        loop[1024];
    H5E_auto_t func;
    
#ifdef H5_HAVE_THREADSAFE
    /* explicit locking of the API */
    H5_FIRST_THREAD_INIT
    H5_API_LOCK
#endif

    /* Don't do anything if the library is already closed */
    if (!(H5_INIT_GLOBAL))
        goto done;

    /* Check if we should display error output */
    H5Eget_auto(&func,NULL);

    /*
     * Terminate each interface. The termination functions return a positive
     * value if they do something that might affect some other interface in a
     * way that would necessitate some cleanup work in the other interface.
     */
#define DOWN(F)                                                               \
    (((n=H5##F##_term_interface()) && at+8<sizeof loop)?                      \
     (sprintf(loop+at, "%s%s", at?",":"", #F),                                \
      at += HDstrlen(loop+at),                                                \
      n):                                                                     \
     ((n>0 && at+5<sizeof loop)?                      \
     (sprintf(loop+at, "..."),                                \
      at += HDstrlen(loop+at),                                                \
     n):n))
    
    do {
        pending = 0;
        /* Try to organize these so the "higher" level components get shut
         * down before "lower" level components that they might rely on. -QAK
         */
        pending += DOWN(R);
        pending += DOWN(D);
        pending += DOWN(G);
        pending += DOWN(A);
        pending += DOWN(S);
        pending += DOWN(TN);
        pending += DOWN(T);
        /* Don't shut down the file code until objects in files are shut down */
        if(pending==0)
            pending += DOWN(F);

        /* Don't shut down "low-level" components until "high-level" components
         * have successfully shut down.  This prevents property lists and IDs
         * from being closed "out from underneath" of the high-level objects
         * that depend on them. -QAK
         */
        if(pending==0) {
            pending += DOWN(AC);
            pending += DOWN(Z);
            pending += DOWN(FD);
            pending += DOWN(P);
            /* Don't shut down the ID code until other APIs which use them are shut down */
            if(pending==0)
                pending += DOWN(I);
            /* Don't shut down the free list code until _everything_ else is down */
            if(pending==0)
                pending += DOWN(FL);
        }
    } while (pending && ntries++ < 100);

    if (pending) {
        /* Only display the error message if the user is interested in them. */
        if (func) {
            fprintf(stderr, "HDF5: infinite loop closing library\n");
            fprintf(stderr, "      %s\n", loop);
        }
    }

#ifdef H5_HAVE_MPE
    /* Close MPE instrumentation library.  May need to move this
     * down if any of the below code involves using the instrumentation code.
     */
    if (H5_MPEinit_g)
    {
        int mpe_code;
        int mpi_initialized;
        MPI_Initialized(&mpi_initialized);
        if (mpi_initialized){
            mpe_code = MPE_Finish_log("cpilog");
            assert(mpe_code >=0);
        }
        H5_MPEinit_g = FALSE;   /* turn it off no matter what */
    }
#endif
    
    /* Mark library as closed */
    H5_INIT_GLOBAL = FALSE;
done:
#ifdef H5_HAVE_THREADSAFE
    H5_API_UNLOCK
#endif
    return;
}


/*-------------------------------------------------------------------------
 * Function:    H5dont_atexit
 *
 * Purpose:     Indicates that the library is not to clean up after itself
 *              when the application exits by calling exit() or returning
 *              from main().  This function must be called before any other
 *              HDF5 function or constant is used or it will have no effect.
 *
 *              If this function is used then certain memory buffers will not
 *              be de-allocated nor will open files be flushed automatically.
 *              The application may still call H5close() explicitly to
 *              accomplish these things.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative if this function is called more than
 *                              once or if it is called too late.
 *
 * Programmer:  Robb Matzke
 *              Friday, November 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5dont_atexit(void)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API_NOINIT(H5dont_atexit);
    H5TRACE0("e","");

    if (dont_atexit_g)
        HGOTO_DONE(FAIL);

    dont_atexit_g = TRUE;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5garbage_collect
 *
 * Purpose:     Walks through all the garbage collection routines for the
 *              library, which are supposed to free any unused memory they have
 *              allocated.
 *
 *      These should probably be registered dynamicly in a linked list of
 *          functions to call, but there aren't that many right now, so we
 *          hard-wire them...
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 11, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5garbage_collect(void)
{
    herr_t                  ret_value = SUCCEED;

    FUNC_ENTER_API(H5garbage_collect, FAIL);
    H5TRACE0("e","");

    /* Call the garbage collection routines in the library */
    H5FL_garbage_coll();

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5garbage_collect() */


/*-------------------------------------------------------------------------
 * Function:    H5set_free_list_limits
 *
 * Purpose:     Sets limits on the different kinds of free lists.  Setting a value
 *      of -1 for a limit means no limit of that type.  These limits are global
 *      for the entire library.  Each "global" limit only applies to free lists
 *      of that type, so if an application sets a limit of 1 MB on each of the
 *      global lists, up to 3 MB of total storage might be allocated (1MB on
 *      each of regular, array and block type lists).
 *
 * Parameters:
 *  int reg_global_lim;  IN: The limit on all "regular" free list memory used
 *  int reg_list_lim;    IN: The limit on memory used in each "regular" free list
 *  int arr_global_lim;  IN: The limit on all "array" free list memory used
 *  int arr_list_lim;    IN: The limit on memory used in each "array" free list
 *  int blk_global_lim;  IN: The limit on all "block" free list memory used
 *  int blk_list_lim;    IN: The limit on memory used in each "block" free list
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, August 2, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5set_free_list_limits(int reg_global_lim, int reg_list_lim, int arr_global_lim,
    int arr_list_lim, int blk_global_lim, int blk_list_lim)
{
    herr_t                  ret_value = SUCCEED;

    FUNC_ENTER_API(H5set_free_list_limits, FAIL);
    H5TRACE6("e","IsIsIsIsIsIs",reg_global_lim,reg_list_lim,arr_global_lim,
             arr_list_lim,blk_global_lim,blk_list_lim);

    /* Call the free list function to actually set the limits */
    H5FL_set_free_list_limits(reg_global_lim, reg_list_lim, arr_global_lim, arr_list_lim, blk_global_lim, blk_list_lim);

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5set_free_list_limits() */


/*-------------------------------------------------------------------------
 * Function:    H5_debug_mask
 *
 * Purpose:     Set runtime debugging flags according to the string S.  The
 *              string should contain file numbers and package names
 *              separated by other characters. A file number applies to all
 *              following package names up to the next file number. The
 *              initial file number is `2' (the standard error stream). Each
 *              package name can be preceded by a `+' or `-' to add or remove
 *              the package from the debugging list (`+' is the default). The
 *              special name `all' means all packages.
 *
 *              The name `trace' indicates that API tracing is to be turned
 *              on or off.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August 19, 1998
 *
 * Modifications:
 *              Robb Matzke, 2002-08-08
 *              Accepts the `ttop' word. If enabled then show only the
 *              top level API calls, otherwise show all API calls.  Also
 *              turns on tracing as if the `trace' word was present.
 *-------------------------------------------------------------------------
 */
static void
H5_debug_mask(const char *s)
{
    FILE        *stream = stderr;
    char        pkg_name[32], *rest;
    size_t      i;
    int         clear;
        
    while (s && *s) {
        if (HDisalpha(*s) || '-'==*s || '+'==*s) {
            /* Enable or Disable debugging? */
            if ('-'==*s) {
                clear = TRUE;
                s++;
            } else if ('+'==*s) {
                clear = FALSE;
                s++;
            } else {
                clear = FALSE;
            }

            /* Get the name */
            for (i=0; HDisalpha(*s); i++, s++) {
                if (i<sizeof pkg_name) pkg_name[i] = *s;
            }
            pkg_name[MIN(sizeof(pkg_name)-1, i)] = '\0';

            /* Trace, all, or one? */
            if (!HDstrcmp(pkg_name, "trace")) {
                H5_debug_g.trace = clear?NULL:stream;
            } else if (!HDstrcmp(pkg_name, "ttop")) {
                H5_debug_g.trace = stream;
                H5_debug_g.ttop = !clear;
            } else if (!HDstrcmp(pkg_name, "ttimes")) {
                H5_debug_g.trace = stream;
                H5_debug_g.ttimes = !clear;
            } else if (!HDstrcmp(pkg_name, "all")) {
                for (i=0; i<H5_NPKGS; i++) {
                    H5_debug_g.pkg[i].stream = clear?NULL:stream;
                }
            } else {
                for (i=0; i<H5_NPKGS; i++) {
                    if (!HDstrcmp(H5_debug_g.pkg[i].name, pkg_name)) {
                        H5_debug_g.pkg[i].stream = clear?NULL:stream;
                        break;
                    }
                }
                if (i>=H5_NPKGS) {
                    fprintf(stderr, "HDF5_DEBUG: ignored %s\n", pkg_name);
                }
            }

        } else if (HDisdigit(*s)) {
            int fd = (int)HDstrtol (s, &rest, 0);
            if ((stream=HDfdopen(fd, "w"))) {
                HDsetvbuf (stream, NULL, _IOLBF, 0);
            }
            s = rest;
        } else {
            s++;
        }
    }
}
    

/*-------------------------------------------------------------------------
 * Function:    H5get_libversion
 *
 * Purpose:     Returns the library version numbers through arguments. MAJNUM
 *              will be the major revision number of the library, MINNUM the
 *              minor revision number, and RELNUM the release revision number.
 *
 * Note:        When printing an HDF5 version number it should be printed as
 *
 *              printf("%u.%u.%u", maj, min, rel)               or
 *              printf("version %u.%u release %u", maj, min, rel)
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *      Robb Matzke, 4 Mar 1998
 *      Now use "normal" data types for the interface.  Any of the arguments
 *      may be null pointers
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5get_libversion(unsigned *majnum, unsigned *minnum, unsigned *relnum)
{
    herr_t                  ret_value = SUCCEED;

    FUNC_ENTER_API(H5get_libversion, FAIL);
    H5TRACE3("e","*Iu*Iu*Iu",majnum,minnum,relnum);

    /* Set the version information */
    if (majnum) *majnum = H5_VERS_MAJOR;
    if (minnum) *minnum = H5_VERS_MINOR;
    if (relnum) *relnum = H5_VERS_RELEASE;

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5check_version
 *
 * Purpose:     Verifies that the arguments match the version numbers
 *              compiled into the library.  This function is intended to be
 *              called from user to verify that the versions of header files
 *              compiled into the application match the version of the hdf5
 *              library.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        abort()
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April 21, 1998
 *
 * Modifications:
 *      Albert Cheng, May 12, 2001
 *      Added verification of H5_VERS_INFO.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5check_version(unsigned majnum, unsigned minnum, unsigned relnum)
{
    char        lib_str[256];
    char        substr[] = H5_VERS_SUBRELEASE;
    static int  checked = 0;            /* If we've already checked the version info */
    static int  disable_version_check = 0;      /* Set if the version check should be disabled */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API_NOINIT(H5check_version);
    H5TRACE3("e","IuIuIu",majnum,minnum,relnum);
    
    /* Don't check again, if we already have */
    if (checked)
        HGOTO_DONE(SUCCEED);
    
    if (H5_VERS_MAJOR!=majnum || H5_VERS_MINOR!=minnum ||
            H5_VERS_RELEASE!=relnum) {
        const char *s;  /* Environment string for disabling version check */

        /* Allow different versions of the header files and library? */
        s = HDgetenv ("HDF5_DISABLE_VERSION_CHECK");

        if (s && HDisdigit(*s))
            disable_version_check = (int)HDstrtol (s, NULL, 0);

        if(disable_version_check) {
            HDfputs ("Warning! The HDF5 header files included by this application "
                     "do not match the\nversion used by the HDF5 library to which "
                     "this application is linked. Data\ncorruption or "
                     "segmentation faults may occur if the application "
                     "continues.\n'HDF5_DISABLE_VERSION_CHECK' "
                     "environment variable set, application will\n"
                     "continue.\n", stderr);
        } /* end if */
        else {
            HDfputs ("Warning! The HDF5 header files included by this application "
                     "do not match the\nversion used by the HDF5 library to which "
                     "this application is linked. Data\ncorruption or "
                     "segmentation faults may occur if the application "
                     "is\nallowed to continue.  You can, at your own risk, "
                     "disable this check by setting\nthe environment variable "
                     "'HDF5_DISABLE_VERSION_CHECK' to a value of '1'.\n", stderr);
        } /* end else */

        /* Mention the versions we are referring to */
        HDfprintf (stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n",
                 majnum, minnum, relnum, 
                 H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE);

        /* Bail out now, if the version check isn't disabled */
        if(!disable_version_check) {
            HDfputs ("Bye...\n", stderr);
            HDabort ();
        } /* end if */
    } /* end if */

    /* Indicate that the version check has been performed */
    checked = 1;

    /*
     *verify if H5_VERS_INFO is consistent with the other version information.
     *Check only the first sizeof(lib_str) char.  Assume the information
     *will fit within this size or enough significance.
     */
    sprintf(lib_str, "HDF5 library version: %d.%d.%d",
        H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE);
    if (*substr){
        HDstrcat(lib_str, "-");
        HDstrncat(lib_str, substr, sizeof(lib_str) - HDstrlen(lib_str) - 1);
    } /* end if */
    if (HDstrcmp(lib_str, H5_lib_vers_info_g)){
        HDfputs ("Warning!  Library version information error.\n"
                 "The HDF5 library version information are not "
                 "consistent in its source code.\nThis is NOT a fatal error "
                 "but should be corrected.\n", stderr);
        HDfprintf (stderr, "Library version information are:\n"
                 "H5_VERS_MAJOR=%d, H5_VERS_MINOR=%d, H5_VERS_RELEASE=%d, "
                 "H5_VERS_SUBRELEASE=%s,\nH5_VERS_INFO=%s\n",
                 H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE,
                 H5_VERS_SUBRELEASE, H5_VERS_INFO);
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5open
 *
 * Purpose:     Initialize the library.  This is normally called
 *              automatically, but if you find that an HDF5 library function
 *              is failing inexplicably, then try calling this function
 *              first.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5open(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5open, FAIL);
    H5TRACE0("e","");
    /* all work is done by FUNC_ENTER() */
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5close
 *
 * Purpose:     Terminate the library and release all resources.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5close(void)
{
    /*
     * Don't call FUNC_ENTER() since we don't want to initialize the whole
     * thing just to release it all right away.  It is safe to call this
     * function for an uninitialized library.
     */
    FUNC_ENTER_API_NOINIT(H5close);
    H5TRACE0("e","");

    H5_term_library();

    FUNC_LEAVE_API(SUCCEED);
}


#ifndef H5_HAVE_SNPRINTF
/*-------------------------------------------------------------------------
 * Function:    HDsnprintf
 *
 * Purpose:     Writes output to the string BUF under control of the format
 *              FMT that specifies how subsequent arguments are converted for
 *              output.  It is similar to sprintf except that SIZE specifies
 *              the maximum number of characters to produce.  The trailing
 *              null character is counted towards this limit, so you should
 *              allocated at least SIZE characters for the string BUF.
 *
 * Note:        This function is for compatibility on systems that don't have
 *              snprintf(3). It doesn't actually check for overflow like the
 *              real snprintf() would.
 *
 * Return:      Success:        Number of characters stored, not including
 *                              the terminating null.  If this value equals
 *                              SIZE then there was not enough space in BUF
 *                              for all the output.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, November 24, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
HDsnprintf(char *buf, size_t UNUSED size, const char *fmt, ...)
{
    int         n;
    va_list     ap;

    va_start(ap, fmt);
    n = HDvsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}
#endif /* H5_HAVE_SNPRINTF */


#ifndef H5_HAVE_VSNPRINTF
/*-------------------------------------------------------------------------
 * Function:    HDvsnprintf
 *
 * Purpose:     The same as HDsnprintf() except the variable arguments are
 *              passed as a va_list.
 *
 * Note:        This function is for compatibility on systems that don't have
 *              vsnprintf(3). It doesn't actually check for overflow like the
 *              real vsnprintf() would.
 *
 * Return:      Success:        Number of characters stored, not including
 *                              the terminating null. If this value equals
 *                              SIZE then there was not enough space in BUF
 *                              for all the output.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Monday, April 26, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
HDvsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    return HDvsprintf(buf, fmt, ap);
}
#endif /* H5_HAVE_VSNPRINTF */


/*-------------------------------------------------------------------------
 * Function:    HDfprintf
 *
 * Purpose:     Prints the optional arguments under the control of the format
 *              string FMT to the stream STREAM.  This function takes the
 *              same format as fprintf(3c) with a few added features:
 *
 *              The conversion modifier `H' refers to the size of an
 *              `hsize_t' or `hssize_t' type.  For instance, "0x%018Hx"
 *              prints an `hsize_t' value as a hex number right justified and
 *              zero filled in an 18-character field.
 *
 *              The conversion `a' refers to an `haddr_t' type.
 *
 * Return:      Success:        Number of characters printed
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  9, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-07-27
 *              The `%a' format refers to an argument of `haddr_t' type
 *              instead of `haddr_t*' and the return value is correct.
 *-------------------------------------------------------------------------
 */
int
HDfprintf(FILE *stream, const char *fmt, ...)
{
    int         n=0, nout = 0;
    int         fwidth, prec;
    int         zerofill;
    int         leftjust;
    int         plussign;
    int         ldspace;
    int         prefix;
    char        modifier[8];
    int         conv;
    char        *rest, format_templ[128];
    const char  *s;
    va_list     ap;
    
    assert (stream);
    assert (fmt);

    va_start (ap, fmt);
    while (*fmt) {
        fwidth = prec = 0;
        zerofill = 0;
        leftjust = 0;
        plussign = 0;
        prefix = 0;
        ldspace = 0;
        modifier[0] = '\0';

        if ('%'==fmt[0] && '%'==fmt[1]) {
            HDputc ('%', stream);
            fmt += 2;
            nout++;
        } else if ('%'==fmt[0]) {
            s = fmt + 1;

            /* Flags */
            while (HDstrchr ("-+ #", *s)) {
                switch (*s) {
                case '-':
                    leftjust = 1;
                    break;
                case '+':
                    plussign = 1;
                    break;
                case ' ':
                    ldspace = 1;
                    break;
                case '#':
                    prefix = 1;
                    break;
                }
                s++;
            }
            
            /* Field width */
            if (HDisdigit (*s)) {
                zerofill = ('0'==*s);
                fwidth = (int)HDstrtol (s, &rest, 10);
                s = rest;
            } else if ('*'==*s) {
                fwidth = va_arg (ap, int);
                if (fwidth<0) {
                    leftjust = 1;
                    fwidth = -fwidth;
                }
                s++;
            }

            /* Precision */
            if ('.'==*s) {
                s++;
                if (HDisdigit (*s)) {
                    prec = (int)HDstrtol (s, &rest, 10);
                    s = rest;
                } else if ('*'==*s) {
                    prec = va_arg (ap, int);
                    s++;
                }
                if (prec<1) prec = 1;
            }

            /* Type modifier */
            if (HDstrchr ("ZHhlqLI", *s)) {
                switch (*s) {
                case 'H':
                    if (sizeof(hsize_t)<sizeof(long)) {
                        modifier[0] = '\0';
                    } else if (sizeof(hsize_t)==sizeof(long)) {
                        HDstrcpy (modifier, "l");
                    } else {
                        HDstrcpy (modifier, H5_PRINTF_LL_WIDTH);
                    }
                    break;
                case 'Z':
                    if (sizeof(size_t)<sizeof(long)) {
                        modifier[0] = '\0';
                    } else if (sizeof(size_t)==sizeof(long)) {
                        HDstrcpy (modifier, "l");
                    } else {
                        HDstrcpy (modifier, H5_PRINTF_LL_WIDTH);
                    }
                    break;
                default:
                    /* Handle 'I64' modifier for Microsoft's "__int64" type */
                    if(*s=='I' && *(s+1)=='6' && *(s+2)=='4') {
                        modifier[0] = *s;
                        modifier[1] = *(s+1);
                        modifier[2] = *(s+2);
                        modifier[3] = '\0';
                        s+=2; /* Increment over 'I6', the '4' is taken care of below */
                    } /* end if */
                    else {
                        /* Handle 'll' for long long types */
                        if(*s=='l' && *(s+1)=='l') {
                            modifier[0] = *s;
                            modifier[1] = *s;
                            modifier[2] = '\0';
                            s++; /* Increment over first 'l', second is taken care of below */
                        } /* end if */
                        else {
                            modifier[0] = *s;
                            modifier[1] = '\0';
                        } /* end else */
                    } /* end else */
                    break;
                }
                s++;
            }

            /* Conversion */
            conv = *s++;

            /* Create the format template */
            sprintf (format_templ, "%%%s%s%s%s%s",
                     leftjust?"-":"", plussign?"+":"",
                     ldspace?" ":"", prefix?"#":"", zerofill?"0":"");
            if (fwidth>0) {
                sprintf (format_templ+HDstrlen(format_templ), "%d", fwidth);
            }
            if (prec>0) {
                sprintf (format_templ+HDstrlen(format_templ), ".%d", prec);
            }
            if (*modifier) {
                sprintf (format_templ+HDstrlen(format_templ), "%s", modifier);
            }
            sprintf (format_templ+HDstrlen(format_templ), "%c", conv);
            

            /* Conversion */
            switch (conv) {
            case 'd':
            case 'i':
                if (!HDstrcmp(modifier, "h")) {
                    short x = va_arg (ap, int);
                    n = fprintf (stream, format_templ, x);
                } else if (!*modifier) {
                    int x = va_arg (ap, int);
                    n = fprintf (stream, format_templ, x);
                } else if (!HDstrcmp (modifier, "l")) {
                    long x = va_arg (ap, long);
                    n = fprintf (stream, format_templ, x);
                } else {
                    int64_t x = va_arg(ap, int64_t);
                    n = fprintf (stream, format_templ, x);
                }
                break;

            case 'o':
            case 'u':
            case 'x':
            case 'X':
                if (!HDstrcmp (modifier, "h")) {
                    unsigned short x = va_arg (ap, unsigned int);
                    n = fprintf (stream, format_templ, x);
                } else if (!*modifier) {
                    unsigned int x = va_arg (ap, unsigned int);
                    n = fprintf (stream, format_templ, x);
                } else if (!HDstrcmp (modifier, "l")) {
                    unsigned long x = va_arg (ap, unsigned long);
                    n = fprintf (stream, format_templ, x);
                } else {
                    uint64_t x = va_arg(ap, uint64_t);
                    n = fprintf (stream, format_templ, x);
                }
                break;

            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                if (!HDstrcmp (modifier, "h")) {
                    float x = (float) va_arg (ap, double);
                    n = fprintf (stream, format_templ, x);
                } else if (!*modifier || !HDstrcmp (modifier, "l")) {
                    double x = va_arg (ap, double);
                    n = fprintf (stream, format_templ, x);
                } else {
                    /*
                     * Some compilers complain when `long double' and
                     * `double' are the same thing.
                     */
#if H5_SIZEOF_LONG_DOUBLE != H5_SIZEOF_DOUBLE
                    long double x = va_arg (ap, long double);
                    n = fprintf (stream, format_templ, x);
#else
                    double x = va_arg (ap, double);
                    n = fprintf (stream, format_templ, x);
#endif
                }
                break;

            case 'a':
                if (1) {
                    haddr_t x = va_arg (ap, haddr_t);
                    if (H5F_addr_defined(x)) {
                        sprintf(format_templ, "%%%s%s%s%s%s",
                                leftjust?"-":"", plussign?"+":"",
                                ldspace?" ":"", prefix?"#":"",
                                zerofill?"0":"");
                        if (fwidth>0) {
                            sprintf(format_templ+HDstrlen(format_templ), "%d", fwidth);
                        }
                        if (sizeof(x)==H5_SIZEOF_INT) {
                            HDstrcat(format_templ, "d");
                        } else if (sizeof(x)==H5_SIZEOF_LONG) {
                            HDstrcat(format_templ, "ld");
                        } else if (sizeof(x)==H5_SIZEOF_LONG_LONG) {
                            HDstrcat(format_templ, H5_PRINTF_LL_WIDTH);
                            HDstrcat(format_templ, "d");
                        }
                        n = fprintf(stream, format_templ, x);
                    } else {
                        HDstrcpy(format_templ, "%");
                        if (leftjust) HDstrcat(format_templ, "-");
                        if (fwidth) {
                            sprintf(format_templ+HDstrlen(format_templ), "%d", fwidth);
                        }
                        HDstrcat(format_templ, "s");
                        fprintf(stream, format_templ, "UNDEF");
                    }
                }
                break;

            case 'c':
                if (1) {
                    char x = (char)va_arg (ap, int);
                    n = fprintf (stream, format_templ, x);
                }
                break;

            case 's':
            case 'p':
                if (1) {
                    char *x = va_arg (ap, char*);
                    n = fprintf (stream, format_templ, x);
                }
                break;

            case 'n':
                if (1) {
                    format_templ[HDstrlen(format_templ)-1] = 'u';
                    n = fprintf (stream, format_templ, nout);
                }
                break;

            default:
                HDfputs (format_templ, stream);
                n = (int)HDstrlen (format_templ);
                break;
            }
            nout += n;
            fmt = s;
        } else {
            HDputc (*fmt, stream);
            fmt++;
            nout++;
        }
    }
    va_end (ap);
    return nout;
}


/*-------------------------------------------------------------------------
 * Function:    HDstrtoll
 *
 * Purpose:     Converts the string S to an int64_t value according to the
 *              given BASE, which must be between 2 and 36 inclusive, or be
 *              the special value zero.
 *
 *              The string must begin with an arbitrary amount of white space
 *              (as determined by isspace(3c)) followed by a single optional
 *              `+' or `-' sign.  If BASE is zero or 16 the string may then
 *              include a `0x' or `0X' prefix, and the number will be read in
 *              base 16; otherwise a zero BASE is taken as 10 (decimal)
 *              unless the next character is a `0', in which case it is taken
 *              as 8 (octal).
 *
 *              The remainder of the string is converted to an int64_t in the
 *              obvious manner, stopping at the first character which is not
 *              a valid digit in the given base.  (In bases above 10, the
 *              letter `A' in either upper or lower case represetns 10, `B'
 *              represents 11, and so forth, with `Z' representing 35.)
 *
 *              If REST is not null, the address of the first invalid
 *              character in S is stored in *REST.  If there were no digits
 *              at all, the original value of S is stored in *REST.  Thus, if
 *              *S is not `\0' but **REST is `\0' on return the entire string
 *              was valid.
 *
 * Return:      Success:        The result.
 *
 *              Failure:        If the input string does not contain any
 *                              digits then zero is returned and REST points
 *                              to the original value of S.  If an overflow
 *                              or underflow occurs then the maximum or
 *                              minimum possible value is returned and the
 *                              global `errno' is set to ERANGE.  If BASE is
 *                              incorrect then zero is returned.
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  9, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int64_t
HDstrtoll(const char *s, const char **rest, int base)
{
    int64_t     sign=1, acc=0;
    hbool_t     overflow = FALSE;
    
    errno = 0;
    if (!s || (base && (base<2 || base>36))) {
        if (rest) *rest = s;
        return 0;
    }
    
    /* Skip white space */
    while (HDisspace (*s)) s++;

    /* Optional minus or plus sign */
    if ('+'==*s) {
        s++;
    } else if ('-'==*s) {
        sign = -1;
        s++; 
   }

    /* Zero base prefix */
    if (0==base && '0'==*s && ('x'==s[1] || 'X'==s[1])) {
        base = 16;
        s += 2;
    } else if (0==base && '0'==*s) {
        base = 8;
        s++;
    } else if (0==base) {
        base = 10;
    }
    
    /* Digits */
    while ((base<=10 && *s>='0' && *s<'0'+base) ||
           (base>10 && ((*s>='0' && *s<='9') ||
                        (*s>='a' && *s<'a'+base-10) ||
                        (*s>='A' && *s<'A'+base-10)))) {
        if (!overflow) {
            int64_t digit = 0;
            if (*s>='0' && *s<='9') digit = *s - '0';
            else if (*s>='a' && *s<='z') digit = *s-'a'+10;
            else digit = *s-'A'+10;

            if (acc*base+digit < acc) {
                overflow = TRUE;
            } else {
                acc = acc*base + digit;
            }
        }
        s++;
    }

    /* Overflow */
    if (overflow) {
        if (sign>0) {
            acc = ((uint64_t)1<<(8*sizeof(int64_t)-1))-1;
        } else {
            acc = (int64_t)1<<(8*sizeof(int64_t)-1);
        }
        errno = ERANGE;
    }

    /* Return values */
    acc *= sign;
    if (rest) *rest = s;
    return acc;
}


/*-------------------------------------------------------------------------
 * Function:    H5_timer_reset
 *
 * Purpose:     Resets the timer struct to zero.  Use this to reset a timer
 *              that's being used as an accumulator for summing times.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_reset (H5_timer_t *timer)
{
    assert (timer);
    HDmemset (timer, 0, sizeof *timer);
}


/*-------------------------------------------------------------------------
 * Function:    H5_timer_begin
 *
 * Purpose:     Initialize a timer to time something.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_begin (H5_timer_t *timer)
{
#ifdef H5_HAVE_GETRUSAGE
    struct rusage       rusage;
#endif
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval      etime;
#endif

    assert (timer);

#ifdef H5_HAVE_GETRUSAGE
    HDgetrusage (RUSAGE_SELF, &rusage);
    timer->utime = (double)rusage.ru_utime.tv_sec +
                   (double)rusage.ru_utime.tv_usec/1e6;
    timer->stime = (double)rusage.ru_stime.tv_sec +
                   (double)rusage.ru_stime.tv_usec/1e6;
#else
    timer->utime = 0.0;
    timer->stime = 0.0;
#endif
#ifdef H5_HAVE_GETTIMEOFDAY
    HDgettimeofday (&etime, NULL);
    timer->etime = (double)etime.tv_sec + (double)etime.tv_usec/1e6;
#else
    timer->etime = 0.0;
#endif
}


/*-------------------------------------------------------------------------
 * Function:    H5_timer_end
 *
 * Purpose:     This function should be called at the end of a timed region.
 *              The SUM is an optional pointer which will accumulate times.
 *              TMS is the same struct that was passed to H5_timer_start().
 *              On return, TMS will contain total times for the timed region.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_end (H5_timer_t *sum/*in,out*/, H5_timer_t *timer/*in,out*/)
{
    H5_timer_t          now;
    
    assert (timer);
    H5_timer_begin (&now);

    timer->utime = MAX(0.0, now.utime - timer->utime);
    timer->stime = MAX(0.0, now.stime - timer->stime);
    timer->etime = MAX(0.0, now.etime - timer->etime);

    if (sum) {
        sum->utime += timer->utime;
        sum->stime += timer->stime;
        sum->etime += timer->etime;
    }
}


/*-------------------------------------------------------------------------
 * Function:    H5_bandwidth
 *
 * Purpose:     Prints the bandwidth (bytes per second) in a field 10
 *              characters wide widh four digits of precision like this:
 *
 *                             NaN      If <=0 seconds
 *                      1234. TB/s
 *                      123.4 TB/s
 *                      12.34 GB/s
 *                      1.234 MB/s
 *                      4.000 kB/s
 *                      1.000  B/s
 *                      0.000  B/s      If NBYTES==0
 *                      1.2345e-10      For bandwidth less than 1
 *                      6.7893e+94      For exceptionally large values
 *                      6.678e+106      For really big values
 *                      
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_bandwidth(char *buf/*out*/, double nbytes, double nseconds)
{
    double      bw;

    if (nseconds<=0.0) {
        HDstrcpy(buf, "       NaN");
    } else {
        bw = nbytes/nseconds;
        if (fabs(bw) < 0.0000000001) {
            /* That is == 0.0, but direct comparison between floats is bad */
            HDstrcpy(buf, "0.000  B/s");
        } else if (bw<1.0) {
            sprintf(buf, "%10.4e", bw);
        } else if (bw<1024.0) {
            sprintf(buf, "%05.4f", bw);
            HDstrcpy(buf+5, "  B/s");
        } else if (bw<1024.0*1024.0) {
            sprintf(buf, "%05.4f", bw/1024.0);
            HDstrcpy(buf+5, " kB/s");
        } else if (bw<1024.0*1024.0*1024.0) {
            sprintf(buf, "%05.4f", bw/(1024.0*1024.0));
            HDstrcpy(buf+5, " MB/s");
        } else if (bw<1024.0*1024.0*1024.0*1024.0) {
            sprintf(buf, "%05.4f",
                    bw/(1024.0*1024.0*1024.0));
            HDstrcpy(buf+5, " GB/s");
        } else if (bw<1024.0*1024.0*1024.0*1024.0*1024.0) {
            sprintf(buf, "%05.4f",
                    bw/(1024.0*1024.0*1024.0*1024.0));
            HDstrcpy(buf+5, " TB/s");
        } else {
            sprintf(buf, "%10.4e", bw);
            if (HDstrlen(buf)>10) {
                sprintf(buf, "%10.3e", bw);
            }
        }
    }
}


/*-------------------------------------------------------------------------
 * Function:    H5_trace
 *
 * Purpose:     This function is called whenever an API function is called
 *              and tracing is turned on.  If RETURNING is non-zero then
 *              the caller is about to return and RETURNING points to the
 *              time for the corresponding function call event.  Otherwise
 *              we print the function name and the arguments.
 *
 *              The TYPE argument is a string which gives the type of each of
 *              the following argument pairs.  Each type is zero or more
 *              asterisks (one for each level of indirection, although some
 *              types have one level of indirection already implied) followed
 *              by either one letter (lower case) or two letters (first one
 *              uppercase).
 *
 *              The variable argument list consists of pairs of values. Each
 *              pair is a string which is the formal argument name in the
 *              calling function, followed by the argument value.  The type
 *              of the argument value is given by the TYPE string.
 *
 * Note:        The TYPE string is meant to be terse and is generated by a
 *              separate perl script.
 *
 * WARNING:     DO NOT CALL ANY HDF5 FUNCTION THAT CALLS FUNC_ENTER(). DOING
 *              SO MAY CAUSE H5_trace() TO BE INVOKED RECURSIVELY OR MAY
 *              CAUSE LIBRARY INITIALIZATIONS THAT ARE NOT DESIRED.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-08-02
 *              Added the `a' type letter for haddr_t arguments and `Mt' for
 *              H5FD_mem_t arguments.
 *
 *              Robb Matzke, 1999-10-25
 *              The `Ej' and `En' types are H5E_major_t and H5E_minor_t error
 *              types. We only print the integer value here.
 *
 *              Robb Matzke, 2002-08-08
 *              Better output for nested calls.  Show only top-level calls
 *              if so desired. Show event times if so desired.
 *-------------------------------------------------------------------------
 */
double
H5_trace (double *returning, const char *func, const char *type, ...)
{
    va_list             ap;
    char                buf[64], *rest;
    const char          *argname;
    int         argno=0, ptr, asize_idx;
    hssize_t            asize[16];
    hssize_t            i;
    void                *vp = NULL;
    FILE                *out = H5_debug_g.trace;
    H5_timer_t          event_time;
    static H5_timer_t   first_time;
    static int          current_depth=0;
    static int          last_call_depth=0;

    /* FUNC_ENTER() should not be called */

    if (!out) return 0.0;       /*tracing is off*/
    va_start (ap, type);

    if (H5_debug_g.ttop) {
        if (returning) {
            if (current_depth>1) {
                --current_depth;
                return 0.0;
            }
        } else {
            if (current_depth>0) {
                /*do not update last_call_depth*/
                current_depth++;
                return 0.0;
            }
        }
    }

    /* Get tim for event */
    if (fabs(first_time.etime) < 0.0000000001)
        /* That is == 0.0, but direct comparison between floats is bad */
        H5_timer_begin(&first_time);
    if (H5_debug_g.ttimes) {
        H5_timer_begin(&event_time);
    } else {
        HDmemset(&event_time, 0, sizeof event_time);
    }
    
    /* Print the first part of the line.  This is the indication of the
     * nesting depth followed by the function name and either start of
     * argument list or start of return value.  If this call is for a
     * function return and no other calls have been made to H5_trace()
     * since the one for the function call, then we're continuing
     * the same line. */
    if (returning) {
        assert(current_depth>0);
        --current_depth;
        if (current_depth<last_call_depth) {
            /* We are at the beginning of a line */
            if (H5_debug_g.ttimes) {
                char tmp[128];
                sprintf(tmp, "%.6f", event_time.etime-first_time.etime);
                fprintf(out, " %*s ", (int)strlen(tmp), "");
            }
            for (i=0; i<current_depth; i++)
                fputc('+', out);
            fprintf(out, "%*s%s = ", 2*current_depth, "", func);
        } else {
            /* Continue current line with return value */
            fprintf(out, " = ");
        }
    } else {
        if (current_depth>last_call_depth)
            fputs(" = <delayed>\n", out);
        if (H5_debug_g.ttimes)
            fprintf(out, "@%.6f ", event_time.etime-first_time.etime);
        for (i=0; i<current_depth; i++)
            fputc('+', out);
        fprintf(out, "%*s%s(", 2*current_depth, "", func);
    }
    
    /* Clear array sizes */
    for (i=0; i<(hssize_t)NELMTS(asize); i++) asize[i] = -1;

    /* Parse the argument types */
    for (argno=0; *type; argno++, type+=HDisupper(*type)?2:1) {
        /* Count levels of indirection */
        for (ptr=0; '*'==*type; type++) ptr++;
        if ('['==*type) {
            if ('a'==type[1]) {
                asize_idx = (int)HDstrtol(type+2, &rest, 10);
                assert(']'==*rest);
                type = rest+1;
            } else {
                rest = HDstrchr(type, ']');
                assert(rest);
                type = rest+1;
                asize_idx = -1;
            }
        } else {
            asize_idx = -1;
        }
        
        /*
         * The argument name.  Leave off the `_id' part.  If the argument
         * name is the null pointer then don't print the argument or the
         * following `='.  This is used for return values.
         */
        argname = va_arg (ap, char*);
        if (argname) {
            unsigned n = MAX (0, (int)HDstrlen(argname)-3);
            if (!HDstrcmp (argname+n, "_id")) {
                HDstrncpy (buf, argname, MIN ((int)sizeof(buf)-1, n));
                buf[MIN((int)sizeof(buf)-1, n)] = '\0';
                argname = buf;
            }
            fprintf (out, "%s%s=", argno?", ":"", argname);
        } else {
            argname = "";
        }

        /* The value */
        if (ptr) vp = va_arg (ap, void*);
        switch (type[0]) {
        case 'a':
            if (ptr) {
                if (vp) {
                    fprintf(out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                haddr_t addr = va_arg(ap, haddr_t);
                HDfprintf(out, "%a", addr);
            }
            break;

        case 'b':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                hbool_t bool_var = va_arg (ap, hbool_t);
                if (TRUE==bool_var) fprintf (out, "TRUE");
                else if (!bool_var) fprintf (out, "FALSE");
                else fprintf (out, "TRUE(%u)", (unsigned)bool_var);
            }
            break;

        case 'd':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                double dbl = va_arg (ap, double);
                fprintf (out, "%g", dbl);
            }
            break;

        case 'D':
            switch (type[1]) {
            case 'a':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5D_alloc_time_t alloc_time = va_arg (ap, H5D_alloc_time_t);
                    switch (alloc_time) {
                    case H5D_ALLOC_TIME_ERROR:
                        fprintf (out, "H5D_ALLOC_TIME_ERROR");
                        break;
                    case H5D_ALLOC_TIME_DEFAULT:
                        fprintf (out, "H5D_ALLOC_TIME_DEFAULT");
                        break;
                    case H5D_ALLOC_TIME_EARLY:
                        fprintf (out, "H5D_ALLOC_TIME_EARLY");
                        break;
                    case H5D_ALLOC_TIME_LATE:
                        fprintf (out, "H5D_ALLOC_TIME_LATE");
                        break;
                    case H5D_ALLOC_TIME_INCR:
                        fprintf (out, "H5D_ALLOC_TIME_INCR");
                        break;
                    }
                }
                break;

            case 'f':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5D_fill_time_t fill_time = va_arg (ap, H5D_fill_time_t);
                    switch (fill_time) {
                    case H5D_FILL_TIME_ERROR:
                        fprintf (out, "H5D_FILL_TIME_ERROR");
                        break;
                    case H5D_FILL_TIME_ALLOC:
                        fprintf (out, "H5D_FILL_TIME_ALLOC");
                        break;
                    case H5D_FILL_TIME_NEVER:
                        fprintf (out, "H5D_FILL_TIME_NEVER");
                        break;
                    case H5D_FILL_TIME_IFSET:
                        fprintf (out, "H5D_FILL_TIME_IFSET");
                        break;
                    }
                }
                break;

            case 'F':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5D_fill_value_t fill_value = va_arg (ap, H5D_fill_value_t);
                    switch (fill_value) {
                    case H5D_FILL_VALUE_ERROR:
                        fprintf (out, "H5D_FILL_VALUE_ERROR");
                        break;
                    case H5D_FILL_VALUE_UNDEFINED:
                        fprintf (out, "H5D_FILL_VALUE_UNDEFINED");
                        break;
                    case H5D_FILL_VALUE_DEFAULT:
                        fprintf (out, "H5D_FILL_VALUE_DEFAULT");
                        break;
                    case H5D_FILL_VALUE_USER_DEFINED:
                        fprintf (out, "H5D_FILL_VALUE_USER_DEFINED");
                        break;
                    }
                }
                break;

            case 'l':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5D_layout_t layout = va_arg (ap, H5D_layout_t);
                    switch (layout) {
                    case H5D_LAYOUT_ERROR:
                        fprintf (out, "H5D_LAYOUT_ERROR");
                        break;
                    case H5D_COMPACT:
                        fprintf (out, "H5D_COMPACT");
                        break;
                    case H5D_CONTIGUOUS:
                        fprintf (out, "H5D_CONTIGUOUS");
                        break;
                    case H5D_CHUNKED:
                        fprintf (out, "H5D_CHUNKED");
                        break;
                    default:
                        fprintf (out, "%ld", (long)layout);
                        break;
                    }
                }
                break;
                
            case 't':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5FD_mpio_xfer_t transfer = va_arg(ap, H5FD_mpio_xfer_t);
                    switch (transfer) {
                    case H5FD_MPIO_INDEPENDENT:
                        fprintf (out, "H5FD_MPIO_INDEPENDENT");
                        break;
                    case H5FD_MPIO_COLLECTIVE:
                        fprintf (out, "H5FD_MPIO_COLLECTIVE");
                        break;
                    default:
                        fprintf (out, "%ld", (long)transfer);
                        break;
                    }
                }
                break;
                
            default:
                fprintf (out, "BADTYPE(D%c)", type[1]);
                goto error;
            }
            break;

        case 'e':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                herr_t status = va_arg (ap, herr_t);
                if (status>=0) fprintf (out, "SUCCEED");
                else if (status<0) fprintf (out, "FAIL");
                else fprintf (out, "%d", (int)status);
            }
            break;
            
        case 'E':
            switch (type[1]) {
            case 'd':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5E_direction_t direction = va_arg (ap, H5E_direction_t);
                    switch (direction) {
                    case H5E_WALK_UPWARD:
                        fprintf (out, "H5E_WALK_UPWARD");
                        break;
                    case H5E_WALK_DOWNWARD:
                        fprintf (out, "H5E_WALK_DOWNWARD");
                        break;
                    default:
                        fprintf (out, "%ld", (long)direction);
                        break;
                    }
                }
                break;
                
            case 'e':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5E_error_t *error = va_arg (ap, H5E_error_t*);
                    fprintf (out, "0x%lx", (unsigned long)error);
                }
                break;

            case 'j':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5E_major_t emaj = va_arg(ap, H5E_major_t);
                    fprintf(out, "%d", (int)emaj);
                }
                break;

            case 'n':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5E_minor_t emin = va_arg(ap, H5E_minor_t);
                    fprintf(out, "%d", (int)emin);
                }
                break;
                
            default:
                fprintf (out, "BADTYPE(E%c)", type[1]);
                goto error;
            }
            break;

        case 'F':
            switch (type[1]) {
            case 'd':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5F_close_degree_t degree = va_arg(ap, H5F_close_degree_t);
                    switch (degree) {
                    case H5F_CLOSE_DEFAULT:
                        fprintf(out, "H5F_CLOSE_DEFAULT");
                        break;
                    case H5F_CLOSE_WEAK:
                        fprintf(out, "H5F_CLOSE_WEAK");
                        break;
                    case H5F_CLOSE_SEMI:
                        fprintf(out, "H5F_CLOSE_SEMI");
                        break;
                    case H5F_CLOSE_STRONG:
                        fprintf(out, "H5F_CLOSE_STRONG");
                        break;
                    }
                }
                break;

            case 's':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5F_scope_t scope = va_arg(ap, H5F_scope_t);
                    switch (scope) {
                    case H5F_SCOPE_LOCAL:
                        fprintf(out, "H5F_SCOPE_LOCAL");
                        break;
                    case H5F_SCOPE_GLOBAL:
                        fprintf(out, "H5F_SCOPE_GLOBAL");
                        break;
                    case H5F_SCOPE_DOWN:
                        fprintf(out, "H5F_SCOPE_DOWN "
                                "/*FOR INTERNAL USE ONLY!*/");
                        break;
                    }
                }
                break;

            default:
                fprintf(out, "BADTYPE(F%c)", type[1]);
                goto error;
            }
            break;

        case 'G':
            switch (type[1]) {
            case 'l':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5G_link_t link_type = va_arg (ap, H5G_link_t);
                    switch (link_type) {
                    case H5G_LINK_ERROR:
                        fprintf (out, "H5G_LINK_ERROR");
                        break;
                    case H5G_LINK_HARD:
                        fprintf (out, "H5G_LINK_HARD");
                        break;
                    case H5G_LINK_SOFT:
                        fprintf (out, "H5G_LINK_SOFT");
                        break;
                    default:
                        fprintf (out, "%ld", (long)link_type);
                        break;
                    }
                }
                break;

            case 's':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5G_stat_t *statbuf = va_arg (ap, H5G_stat_t*);
                    fprintf (out, "0x%lx", (unsigned long)statbuf);
                }
                break;

            default:
                fprintf (out, "BADTYPE(G%c)", type[1]);
                goto error;
            }
            break;

        case 'h':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                    if (asize_idx>=0 && asize[asize_idx]>=0) {
                        hsize_t *p = (hsize_t*)vp;
                        fprintf(out, " {");
                        for (i=0; i<asize[asize_idx]; i++) {
                            if (H5S_UNLIMITED==p[i]) {
                                HDfprintf(out, "%sH5S_UNLIMITED", i?", ":"");
                            } else {
                                HDfprintf(out, "%s%Hu", i?", ":"", p[i]);
                            }
                        }
                        fprintf(out, "}");
                    }
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                hsize_t hsize = va_arg (ap, hsize_t);
                if (H5S_UNLIMITED==hsize) {
                    HDfprintf(out, "H5S_UNLIMITED");
                } else {
                    HDfprintf (out, "%Hu", hsize);
                    asize[argno] = (hssize_t)hsize;
                }
            }
            break;

        case 'H':
            switch (type[1]) {
            case 's':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                        if (asize_idx>=0 && asize[asize_idx]>=0) {
                            hssize_t *p = (hssize_t*)vp;
                            fprintf(out, " {");
                            for (i=0; i<asize[asize_idx]; i++) {
                                HDfprintf(out, "%s%Hd", i?", ":"", p[i]);
                            }
                            fprintf(out, "}");
                        }
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    hssize_t hssize = va_arg (ap, hssize_t);
                    HDfprintf (out, "%Hd", hssize);
                    asize[argno] = (hssize_t)hssize;
                }
                break;
                
            default:
                fprintf (out, "BADTYPE(H%c)", type[1]);
                goto error;
            }
            break;

        case 'i':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                hid_t obj = va_arg (ap, hid_t);
                if (H5P_DEFAULT == obj) {
                    fprintf (out, "H5P_DEFAULT");
                } else if (obj<0) {
                    fprintf (out, "FAIL");
                } else {
                    switch (H5I_GROUP(obj)) { /* Use internal H5I macro instead of function call */
                        case H5I_BADID:
                            fprintf (out, "%ld (error)", (long)obj);
                            break;
                        case H5I_FILE:
                            fprintf(out, "%ld", (long)obj);
                            if (HDstrcmp (argname, "file")) {
                                fprintf (out, " (file)");
                            }
                            break;
                        case H5I_GROUP:
                            fprintf(out, "%ld", (long)obj);
                            if (HDstrcmp (argname, "group")) {
                                fprintf (out, " (group)");
                            }
                            break;
                        case H5I_DATATYPE:
                            if (obj==H5T_NATIVE_SCHAR_g) {
                                fprintf(out, "H5T_NATIVE_SCHAR");
                            } else if (obj==H5T_NATIVE_UCHAR_g) {
                                fprintf(out, "H5T_NATIVE_UCHAR");
                            } else if (obj==H5T_NATIVE_SHORT_g) {
                                fprintf(out, "H5T_NATIVE_SHORT");
                            } else if (obj==H5T_NATIVE_USHORT_g) {
                                fprintf(out, "H5T_NATIVE_USHORT");
                            } else if (obj==H5T_NATIVE_INT_g) {
                                fprintf(out, "H5T_NATIVE_INT");
                            } else if (obj==H5T_NATIVE_UINT_g) {
                                fprintf(out, "H5T_NATIVE_UINT");
                            } else if (obj==H5T_NATIVE_LONG_g) {
                                fprintf(out, "H5T_NATIVE_LONG");
                            } else if (obj==H5T_NATIVE_ULONG_g) {
                                fprintf(out, "H5T_NATIVE_ULONG");
                            } else if (obj==H5T_NATIVE_LLONG_g) {
                                fprintf(out, "H5T_NATIVE_LLONG");
                            } else if (obj==H5T_NATIVE_ULLONG_g) {
                                fprintf(out, "H5T_NATIVE_ULLONG");
                            } else if (obj==H5T_NATIVE_FLOAT_g) {
                                fprintf(out, "H5T_NATIVE_FLOAT");
                            } else if (obj==H5T_NATIVE_DOUBLE_g) {
                                fprintf(out, "H5T_NATIVE_DOUBLE");
                            } else if (obj==H5T_NATIVE_LDOUBLE_g) {
                                fprintf(out, "H5T_NATIVE_LDOUBLE");
                            } else if (obj==H5T_IEEE_F32BE_g) {
                                fprintf(out, "H5T_IEEE_F32BE");
                            } else if (obj==H5T_IEEE_F32LE_g) {
                                fprintf(out, "H5T_IEEE_F32LE");
                            } else if (obj==H5T_IEEE_F64BE_g) {
                                fprintf(out, "H5T_IEEE_F64BE");
                            } else if (obj==H5T_IEEE_F64LE_g) {
                                fprintf(out, "H5T_IEEE_F64LE");
                            } else if (obj==H5T_STD_I8BE_g) {
                                fprintf(out, "H5T_STD_I8BE");
                            } else if (obj==H5T_STD_I8LE_g) {
                                fprintf(out, "H5T_STD_I8LE");
                            } else if (obj==H5T_STD_I16BE_g) {
                                fprintf(out, "H5T_STD_I16BE");
                            } else if (obj==H5T_STD_I16LE_g) {
                                fprintf(out, "H5T_STD_I16LE");
                            } else if (obj==H5T_STD_I32BE_g) {
                                fprintf(out, "H5T_STD_I32BE");
                            } else if (obj==H5T_STD_I32LE_g) {
                                fprintf(out, "H5T_STD_I32LE");
                            } else if (obj==H5T_STD_I64BE_g) {
                                fprintf(out, "H5T_STD_I64BE");
                            } else if (obj==H5T_STD_I64LE_g) {
                                fprintf(out, "H5T_STD_I64LE");
                            } else if (obj==H5T_STD_U8BE_g) {
                                fprintf(out, "H5T_STD_U8BE");
                            } else if (obj==H5T_STD_U8LE_g) {
                                fprintf(out, "H5T_STD_U8LE");
                            } else if (obj==H5T_STD_U16BE_g) {
                                fprintf(out, "H5T_STD_U16BE");
                            } else if (obj==H5T_STD_U16LE_g) {
                                fprintf(out, "H5T_STD_U16LE");
                            } else if (obj==H5T_STD_U32BE_g) {
                                fprintf(out, "H5T_STD_U32BE");
                            } else if (obj==H5T_STD_U32LE_g) {
                                fprintf(out, "H5T_STD_U32LE");
                            } else if (obj==H5T_STD_U64BE_g) {
                                fprintf(out, "H5T_STD_U64BE");
                            } else if (obj==H5T_STD_U64LE_g) {
                                fprintf(out, "H5T_STD_U64LE");
                            } else if (obj==H5T_STD_B8BE_g) {
                                fprintf(out, "H5T_STD_B8BE");
                            } else if (obj==H5T_STD_B8LE_g) {
                                fprintf(out, "H5T_STD_B8LE");
                            } else if (obj==H5T_STD_B16BE_g) {
                                fprintf(out, "H5T_STD_B16BE");
                            } else if (obj==H5T_STD_B16LE_g) {
                                fprintf(out, "H5T_STD_B16LE");
                            } else if (obj==H5T_STD_B32BE_g) {
                                fprintf(out, "H5T_STD_B32BE");
                            } else if (obj==H5T_STD_B32LE_g) {
                                fprintf(out, "H5T_STD_B32LE");
                            } else if (obj==H5T_STD_B64BE_g) {
                                fprintf(out, "H5T_STD_B64BE");
                            } else if (obj==H5T_STD_B64LE_g) {
                                fprintf(out, "H5T_STD_B64LE");
                            } else if (obj==H5T_C_S1_g) {
                                fprintf(out, "H5T_C_S1");
                            } else if (obj==H5T_FORTRAN_S1_g) {
                                fprintf(out, "H5T_FORTRAN_S1");
                            } else {
                                fprintf(out, "%ld", (long)obj);
                                if (HDstrcmp (argname, "type")) {
                                    fprintf (out, " (type)");
                                }
                            }
                            break;
                        case H5I_DATASPACE:
                            fprintf(out, "%ld", (long)obj);
                            if (HDstrcmp (argname, "space")) {
                                fprintf (out, " (space)");
                            }
                            /* Save the rank of simple data spaces for arrays */
                            /* This may generate recursive call to the library... -QAK */
                            {
                                H5S_t *space = H5I_object(obj);
                                if (H5S_SIMPLE==H5S_get_simple_extent_type(space)) {
                                    asize[argno] = H5S_get_simple_extent_ndims(space);
                                }
                            }
                            break;
                        case H5I_DATASET:
                            fprintf(out, "%ld", (long)obj);
                            if (HDstrcmp (argname, "dset")) {
                                fprintf (out, " (dset)");
                            }
                            break;
                        case H5I_ATTR:
                            fprintf(out, "%ld", (long)obj);
                            if (HDstrcmp (argname, "attr")) {
                                fprintf (out, " (attr)");
                            }
                            break;
                        case H5I_TEMPBUF:
                            fprintf(out, "%ld", (long)obj);
                            if (HDstrcmp(argname, "tbuf")) {
                                fprintf(out, " (tbuf");
                            }
                            break;
                        case H5I_REFERENCE:
                            fprintf(out, "%ld (reference)", (long)obj);
                            break;
                        case H5I_VFL:
                            fprintf(out, "%ld (file driver)", (long)obj);
                            break;
                        default:
                            fprintf(out, "%ld", (long)obj);
                            fprintf (out, " (unknown class)");
                            break;
                    }
                }
            }
            break;

        case 'I':
            switch (type[1]) {
            case 's':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                        if (asize_idx>=0 && asize[asize_idx]>=0) {
                            int *p = (int*)vp;
                            fprintf(out, " {");
                            for (i=0; i<asize[asize_idx]; i++) {
                                fprintf(out, "%s%d", i?", ":"", p[i]);
                            }
                            fprintf(out, "}");
                        }
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    int is = va_arg (ap, int);
                    fprintf (out, "%d", is);
                    asize[argno] = is;
                }
                break;
                
            case 'u':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                        if (asize_idx>=0 && asize[asize_idx]>=0) {
                            int *p = (int*)vp;
                            fprintf(out, " {");
                            for (i=0; i<asize[asize_idx]; i++) {
                                HDfprintf(out, "%s%Hu", i?", ":"", p[i]);
                            }
                            fprintf(out, "}");
                        }
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    unsigned iu = va_arg (ap, unsigned);
                    fprintf (out, "%u", iu);
                    asize[argno] = iu;
                }
                break;

            case 't':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5I_type_t id_type = va_arg (ap, H5I_type_t);
                    switch (id_type) {
                    case H5I_BADID:
                        fprintf (out, "H5I_BADID");
                        break;
                    case H5I_FILE:
                        fprintf (out, "H5I_FILE");
                        break;
                    case H5I_GROUP:
                        fprintf (out, "H5I_GROUP");
                        break;
                    case H5I_DATATYPE:
                        fprintf (out, "H5I_DATATYPE");
                        break;
                    case H5I_DATASPACE:
                        fprintf (out, "H5I_DATASPACE");
                        break;
                    case H5I_DATASET:
                        fprintf (out, "H5I_DATASET");
                        break;
                    case H5I_ATTR:
                        fprintf (out, "H5I_ATTR");
                        break;
                    case H5I_TEMPBUF:
                        fprintf (out, "H5I_TEMPBUF");
                        break;
                    case H5I_REFERENCE:
                        fprintf (out, "H5I_REFERENCE");
                        break;
                    case H5I_VFL:
                        fprintf (out, "H5I_VFL");
                        break;
                    case H5I_GENPROP_CLS:
                        fprintf (out, "H5I_GENPROP_CLS");
                        break;
                    case H5I_GENPROP_LST:
                        fprintf (out, "H5I_GENPROP_LST");
                        break;
                    case H5I_NGROUPS:
                        fprintf (out, "H5I_NGROUPS");
                        break;
                    default:
                        fprintf (out, "%ld", (long)id_type);
                        break;
                    }
                }
                break;

            default:
                fprintf (out, "BADTYPE(I%c)", type[1]);
                goto error;
            }
            break;

        case 'M':
            switch (type[1]) {
            case 'c':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
#ifdef H5_HAVE_PARALLEL
                    MPI_Comm comm = va_arg (ap, MPI_Comm);
                    fprintf (out, "%ld", (long)comm);
#endif
                }
                break;
            case 'i':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
#ifdef H5_HAVE_PARALLEL
                    MPI_Info info = va_arg (ap, MPI_Info);
                    fprintf (out, "%ld", (long)info);
#endif
                }
                break;
            case 't':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5FD_mem_t mt = va_arg(ap, H5FD_mem_t);
                    switch (mt) {
                    case H5FD_MEM_NOLIST:
                        fprintf(out, "H5FD_MEM_NOLIST");
                        break;
                    case H5FD_MEM_DEFAULT:
                        fprintf(out, "H5FD_MEM_DEFAULT");
                        break;
                    case H5FD_MEM_SUPER:
                        fprintf(out, "H5FD_MEM_SUPER");
                        break;
                    case H5FD_MEM_BTREE:
                        fprintf(out, "H5FD_MEM_BTREE");
                        break;
                    case H5FD_MEM_DRAW:
                        fprintf(out, "H5FD_MEM_DRAW");
                        break;
                    case H5FD_MEM_GHEAP:
                        fprintf(out, "H5FD_MEM_GHEAP");
                        break;
                    case H5FD_MEM_LHEAP:
                        fprintf(out, "H5FD_MEM_LHEAP");
                        break;
                    case H5FD_MEM_OHDR:
                        fprintf(out, "H5FD_MEM_OHDR");
                        break;
                    default:
                        fprintf(out, "%lu", (unsigned long)mt);
                        break;
                    }
                }
                break;

            default:
                goto error;
            }
            break;

        case 'o':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                off_t offset = va_arg (ap, off_t);
                fprintf (out, "%ld", (long)offset);
            }
            break;

        case 'p':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                hid_t pclass_id = va_arg (ap, hid_t);
                char *class_name=NULL;
                H5P_genclass_t *pclass;

                /* Get the class name and print it */
                /* This may generate recursive call to the library... -QAK */
                if(NULL != (pclass = H5I_object(pclass_id)) &&
                        (class_name=H5P_get_class_name(pclass))!=NULL) {
                    fprintf (out, class_name);
                    H5MM_xfree(class_name);
                } /* end if */
                else {
                    fprintf (out, "%ld", (long)pclass_id);
                } /* end else */
            }
            break;

        case 'r':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                hobj_ref_t ref = va_arg (ap, hobj_ref_t);
                fprintf (out, "Reference Object=%p", (void *)&ref);
            }
            break;

        case 'R':
            switch (type[1]) {
                case 't':
                    if (ptr) {
                        if (vp) {
                            fprintf(out, "0x%lx", (unsigned long)vp);
                        } else {
                            fprintf(out, "NULL");
                        }
                    } else {
                        H5R_type_t reftype = va_arg(ap, H5R_type_t);
                        switch (reftype) {
                            case H5R_BADTYPE:
                                fprintf(out, "H5R_BADTYPE");
                                break;
                            case H5R_OBJECT:
                                fprintf(out, "H5R_OBJECT");
                                break;
                            case H5R_DATASET_REGION:
                                fprintf(out, "H5R_DATASET_REGION");
                                break;
                            case H5R_INTERNAL:
                                fprintf(out, "H5R_INTERNAL");
                                break;
                            case H5R_MAXTYPE:
                                fprintf(out, "H5R_MAXTYPE");
                                break;
                            default:
                                fprintf(out, "BADTYPE(%ld)", (long)reftype);
                                break;
                        }
                    }
                    break;
                            
                default:
                    fprintf(out, "BADTYPE(S%c)", type[1]);
                    goto error;
            }
            break;

        case 'S':
            switch (type[1]) {
            case 'c':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5S_class_t cls = va_arg(ap, H5S_class_t);
                    switch (cls) {
                        case H5S_NO_CLASS:
                            fprintf(out, "H5S_NO_CLASS");
                            break;
                        case H5S_SCALAR:
                            fprintf(out, "H5S_SCALAR");
                            break;
                        case H5S_SIMPLE:
                            fprintf(out, "H5S_SIMPLE");
                            break;
                        case H5S_COMPLEX:
                            fprintf(out, "H5S_COMPLEX");
                            break;
                        default:
                            fprintf(out, "%ld", (long)cls);
                            break;
                    }
                }
                break;
                        
            case 's':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5S_seloper_t so = va_arg(ap, H5S_seloper_t);
                    switch (so) {
                        case H5S_SELECT_NOOP:
                            fprintf(out, "H5S_NOOP");
                            break;
                        case H5S_SELECT_SET:
                            fprintf(out, "H5S_SELECT_SET");
                            break;
                        case H5S_SELECT_OR:
                            fprintf(out, "H5S_SELECT_OR");
                            break;
                        default:
                            fprintf(out, "%ld", (long)so);
                            break;
                    }
                }
                break;

            case 't':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5S_sel_type st = va_arg(ap, H5S_sel_type);
                    switch (st) {
                    case H5S_SEL_ERROR:
                        fprintf(out, "H5S_SEL_ERROR");
                        break;
                    case H5S_SEL_NONE:
                        fprintf(out, "H5S_SEL_NONE");
                        break;
                    case H5S_SEL_POINTS:
                        fprintf(out, "H5S_SEL_POINTS");
                        break;
                    case H5S_SEL_HYPERSLABS:
                        fprintf(out, "H5S_SEL_HYPERSLABS");
                        break;
                    case H5S_SEL_ALL:
                        fprintf(out, "H5S_SEL_ALL");
                        break;
                    default:
                        fprintf(out, "%ld", (long)st);
                        break;
                    }
                }
                break;

            default:
                fprintf(out, "BADTYPE(S%c)", type[1]);
                goto error;
            }
            break;

        case 's':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                const char *str = va_arg (ap, const char*);
                fprintf (out, "\"%s\"", str);
            }
            break;

        case 'T':
            switch (type[1]) {
            case 'c':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_cset_t cset = va_arg (ap, H5T_cset_t);
                    switch (cset) {
                        case H5T_CSET_ERROR:
                            fprintf (out, "H5T_CSET_ERROR");
                            break;
                        case H5T_CSET_ASCII:
                            fprintf (out, "H5T_CSET_ASCII");
                            break;
                        default:
                            fprintf (out, "%ld", (long)cset);
                            break;
                    }
                }
                break;

            case 'd':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_direction_t direct = va_arg (ap, H5T_direction_t);
                    switch (direct) {
                        case H5T_DIR_DEFAULT:
                            fprintf (out, "H5T_DIR_DEFAULT");
                            break;
                        case H5T_DIR_ASCEND:
                            fprintf (out, "H5T_DIR_ASCEND");
                            break;
                        case H5T_DIR_DESCEND:
                            fprintf (out, "H5T_DIR_DESCEND");
                            break;
                        default:
                            fprintf (out, "%ld", (long)direct);
                            break;
                    }
                }
                break;

            case 'e':
                if (ptr) {
                    if (vp) {
                        fprintf(out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_pers_t pers = va_arg(ap, H5T_pers_t);
                    switch (pers) {
                        case H5T_PERS_DONTCARE:
                            fprintf(out, "H5T_PERS_DONTCARE");
                            break;
                        case H5T_PERS_SOFT:
                            fprintf(out, "H5T_PERS_SOFT");
                            break;
                        case H5T_PERS_HARD:
                            fprintf(out, "H5T_PERS_HARD");
                            break;
                        default:
                            fprintf(out, "%ld", (long)pers);
                            break;
                    }
                }
                break;

            case 'n':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_norm_t norm = va_arg (ap, H5T_norm_t);
                    switch (norm) {
                        case H5T_NORM_ERROR:
                            fprintf (out, "H5T_NORM_ERROR");
                            break;
                        case H5T_NORM_IMPLIED:
                            fprintf (out, "H5T_NORM_IMPLIED");
                            break;
                        case H5T_NORM_MSBSET:
                            fprintf (out, "H5T_NORM_MSBSET");
                            break;
                        case H5T_NORM_NONE:
                            fprintf (out, "H5T_NORM_NONE");
                            break;
                        default:
                            fprintf (out, "%ld", (long)norm);
                            break;
                    }
                }
                break;

            case 'o':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_order_t order = va_arg (ap, H5T_order_t);
                    switch (order) {
                        case H5T_ORDER_ERROR:
                            fprintf (out, "H5T_ORDER_ERROR");
                            break;
                        case H5T_ORDER_LE:
                            fprintf (out, "H5T_ORDER_LE");
                            break;
                        case H5T_ORDER_BE:
                            fprintf (out, "H5T_ORDER_BE");
                            break;
                        case H5T_ORDER_VAX:
                            fprintf (out, "H5T_ORDER_VAX");
                            break;
                        case H5T_ORDER_NONE:
                            fprintf (out, "H5T_ORDER_NONE");
                            break;
                        default:
                            fprintf (out, "%ld", (long)order);
                            break;
                    }
                }
                break;

            case 'p':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_pad_t pad = va_arg (ap, H5T_pad_t);
                    switch (pad) {
                        case H5T_PAD_ERROR:
                            fprintf (out, "H5T_PAD_ERROR");
                            break;
                        case H5T_PAD_ZERO:
                            fprintf (out, "H5T_PAD_ZERO");
                            break;
                        case H5T_PAD_ONE:
                            fprintf (out, "H5T_PAD_ONE");
                            break;
                        case H5T_PAD_BACKGROUND:
                            fprintf (out, "H5T_PAD_BACKGROUND");
                            break;
                        default:
                            fprintf (out, "%ld", (long)pad);
                            break;
                    }
                }
                break;

            case 's':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_sign_t sign = va_arg (ap, H5T_sign_t);
                    switch (sign) {
                        case H5T_SGN_ERROR:
                            fprintf (out, "H5T_SGN_ERROR");
                            break;
                        case H5T_SGN_NONE:
                            fprintf (out, "H5T_SGN_NONE");
                            break;
                        case H5T_SGN_2:
                            fprintf (out, "H5T_SGN_2");
                            break;
                        default:
                            fprintf (out, "%ld", (long)sign);
                            break;
                    }
                }
                break;

            case 't':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_class_t type_class = va_arg(ap, H5T_class_t);
                    switch (type_class) {
                        case H5T_NO_CLASS:
                            fprintf(out, "H5T_NO_CLASS");
                            break;
                        case H5T_INTEGER:
                            fprintf(out, "H5T_INTEGER");
                            break;
                        case H5T_FLOAT:
                            fprintf(out, "H5T_FLOAT");
                            break;
                        case H5T_TIME:
                            fprintf(out, "H5T_TIME");
                            break;
                        case H5T_STRING:
                            fprintf(out, "H5T_STRING");
                            break;
                        case H5T_BITFIELD:
                            fprintf(out, "H5T_BITFIELD");
                            break;
                        case H5T_OPAQUE:
                            fprintf(out, "H5T_OPAQUE");
                            break;
                        case H5T_COMPOUND:
                            fprintf(out, "H5T_COMPOUND");
                            break;
                        case H5T_ENUM:
                            fprintf(out, "H5T_ENUM");
                            break;
                        default:
                            fprintf(out, "%ld", (long)type_class);
                            break;
                    }
                }
                break;

            case 'z':
                if (ptr) {
                    if (vp) {
                        fprintf (out, "0x%lx", (unsigned long)vp);
                    } else {
                        fprintf(out, "NULL");
                    }
                } else {
                    H5T_str_t str = va_arg(ap, H5T_str_t);
                    switch (str) {
                        case H5T_STR_ERROR:
                            fprintf(out, "H5T_STR_ERROR");
                            break;
                        case H5T_STR_NULLTERM:
                            fprintf(out, "H5T_STR_NULLTERM");
                            break;
                        case H5T_STR_NULLPAD:
                            fprintf(out, "H5T_STR_NULLPAD");
                            break;
                        case H5T_STR_SPACEPAD:
                            fprintf(out, "H5T_STR_SPACEPAD");
                            break;
                        default:
                            fprintf(out, "%ld", (long)str);
                            break;
                    }
                }
                break;

            default:
                fprintf (out, "BADTYPE(T%c)", type[1]);
                goto error;
            }
            break;

        case 'x':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                    if (asize_idx>=0 && asize[asize_idx]>=0) {
                        void **p = (void**)vp;
                        fprintf(out, " {");
                        for (i=0; i<asize[asize_idx]; i++) {
                            if (p[i]) {
                                fprintf(out, "%s0x%lx", i?", ":"",
                                        (unsigned long)(p[i]));
                            } else {
                                fprintf(out, "%sNULL", i?", ":"");
                            }
                        }
                        fprintf(out, "}");
                    }
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                vp = va_arg (ap, void*);
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                } else {
                    fprintf(out, "NULL");
                }
            }
            break;

        case 'z':
            if (ptr) {
                if (vp) {
                    fprintf (out, "0x%lx", (unsigned long)vp);
                    if (asize_idx>=0 && asize[asize_idx]>=0) {
                        size_t *p = (size_t*)vp;
                        fprintf(out, " {");
                        for (i=0; i<asize[asize_idx]; i++) {
                            HDfprintf(out, "%s%Zu", i?", ":"", p[i]);
                        }
                        fprintf(out, "}");
                    }
                } else {
                    fprintf(out, "NULL");
                }
            } else {
                size_t size = va_arg (ap, size_t);

                HDfprintf (out, "%Zu", size);
                asize[argno] = (hssize_t)size;
            }
            break;

        case 'Z':
            switch (type[1]) {
                case 'e':
                    if (ptr) {
                        if (vp) {
                            fprintf (out, "0x%lx", (unsigned long)vp);
                        } else {
                            fprintf(out, "NULL");
                        }
                    } else {
                        H5Z_EDC_t edc = va_arg (ap, H5Z_EDC_t);

                        if (H5Z_DISABLE_EDC==edc) {
                            fprintf (out, "H5Z_DISABLE_EDC");
                        } else if (H5Z_ENABLE_EDC==edc) {
                            fprintf (out, "H5Z_ENABLE_EDC");
                        } else {
                            fprintf (out, "%ld", (long)edc);
                        }
                    }
                    break;

                case 'f':
                    if (ptr) {
                        if (vp) {
                            fprintf (out, "0x%lx", (unsigned long)vp);
                        } else {
                            fprintf(out, "NULL");
                        }
                    } else {
                        H5Z_filter_t id = va_arg (ap, H5Z_filter_t);

                        if (H5Z_FILTER_DEFLATE==id) {
                            fprintf (out, "H5Z_FILTER_DEFLATE");
                        } else {
                            fprintf (out, "%ld", (long)id);
                        }
                    }
                    break;

                case 's':
                    if (ptr) {
                        if (vp) {
                            fprintf (out, "0x%lx", (unsigned long)vp);
                            if (vp && asize_idx>=0 && asize[asize_idx]>=0) {
                                ssize_t *p = (ssize_t*)vp;
                                fprintf(out, " {");
                                for (i=0; i<asize[asize_idx]; i++) {
                                    HDfprintf(out, "%s%Zd", i?", ":"", p[i]);
                                }
                                fprintf(out, "}");
                            }
                        } else {
                            fprintf(out, "NULL");
                        }
                    } else {
                        ssize_t ssize = va_arg (ap, ssize_t);

                        HDfprintf (out, "%Zd", ssize);
                        asize[argno] = (hssize_t)ssize;
                    }
                    break;

                default:
                    fprintf (out, "BADTYPE(Z%c)", type[1]);
                    goto error;
            }
            break;

        default:
            if (HDisupper (type[0])) {
                fprintf (out, "BADTYPE(%c%c)", type[0], type[1]);
            } else {
                fprintf (out, "BADTYPE(%c)", type[0]);
            }
            goto error;
        }
    }


    /* Display event time for return */
    if (returning && H5_debug_g.ttimes)
        fprintf(out, " @%.6f [dt=%.6f]",
                event_time.etime - first_time.etime,
                event_time.etime - *returning);

  error:
    va_end (ap);
    if (returning) {
        fprintf (out, ";\n");
    } else {
        last_call_depth = current_depth++;
        fprintf (out, ")");
    }
    HDfflush (out);
    return event_time.etime;
}
