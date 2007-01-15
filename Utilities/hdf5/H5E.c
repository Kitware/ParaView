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

/*
 * Purpose:  Provides error handling in the form of a stack.  The
 *    FUNC_ENTER() macro clears the error stack whenever an API
 *    function is entered.  When an error is detected, an entry is
 *    pushed onto the stack.  As the functions unwind additional
 *    entries are pushed onto the stack. The API function will
 *    return some indication that an error occurred and the
 *    application can print the error stack.
 *
 *    Certain API functions in the H5E package (such as H5Eprint())
 *    do not clear the error stack.  Otherwise, any function which
 *    doesn't have an underscore immediately after the package name
 *    will clear the error stack.  For instance, H5Fopen() clears
 *    the error stack while H5F_open() does not.
 *
 *    An error stack has a fixed maximum size.  If this size is
 *    exceeded then the stack will be truncated and only the
 *    inner-most functions will have entries on the stack. This is
 *    expected to be a rare condition.
 *
 *    Each thread has its own error stack, but since
 *    multi-threading has not been added to the library yet, this
 *    package maintains a single error stack. The error stack is
 *    statically allocated to reduce the complexity of handling
 *    errors within the H5E package.
 *
 */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5E_init_interface


#include "H5private.h"    /* Generic Functions        */
#include "H5Iprivate.h"    /* IDs                                    */
#include "H5Eprivate.h"    /* Private error routines      */
#include "H5MMprivate.h"  /* Memory management        */

static const H5E_major_mesg_t H5E_major_mesg_g[] = {
    {H5E_NONE_MAJOR,  "No error"},
    {H5E_ARGS,    "Function arguments"},
    {H5E_RESOURCE,  "Resource unavailable"},
    {H5E_INTERNAL,  "Internal HDF5 error"},
    {H5E_FILE,    "File interface"},
    {H5E_IO,    "Low-level I/O layer"},
    {H5E_FUNC,    "Function entry/exit"},
    {H5E_ATOM,    "Atom layer"},
    {H5E_CACHE,    "Meta data cache layer"},
    {H5E_BTREE,    "B-tree layer"},
    {H5E_SYM,    "Symbol table layer"},
    {H5E_HEAP,    "Heap layer"},
    {H5E_OHDR,    "Object header layer"},
    {H5E_DATATYPE,  "Datatype interface"},
    {H5E_DATASPACE,  "Dataspace interface"},
    {H5E_DATASET,  "Dataset interface"},
    {H5E_STORAGE,  "Data storage layer"},
    {H5E_PLIST,    "Property list interface"},
    {H5E_ATTR,     "Attribute layer"},
    {H5E_PLINE,    "Data filters layer"},
    {H5E_EFL,     "External file list"},
    {H5E_REFERENCE,  "References layer"},
    {H5E_VFL,    "Virtual File Layer"},
    {H5E_TBBT,    "Threaded, Balanced, Binary Trees"},
    {H5E_TST,    "Ternary Search Trees"},
    {H5E_RS,    "Reference Counted Strings"},
    {H5E_ERROR,    "Error API"},
    {H5E_SLIST,    "Skip Lists"},
};

static const H5E_minor_mesg_t H5E_minor_mesg_g[] = {
    {H5E_NONE_MINOR,   "No error"},

    /* Argument errors */
    {H5E_UNINITIALIZED, "Information is uninitialized"},
    {H5E_UNSUPPORTED,   "Feature is unsupported"},
    {H5E_BADTYPE,   "Inappropriate type"},
    {H5E_BADRANGE,   "Out of range"},
    {H5E_BADVALUE,   "Bad value"},

    /* Resource errors */
    {H5E_NOSPACE,   "No space available for allocation"},
    {H5E_CANTCOPY,   "Unable to copy object"},
    {H5E_CANTFREE,   "Unable to free object"},
    {H5E_ALREADYEXISTS, "Object already exists"},
    {H5E_CANTLOCK,   "Unable to lock object"},
    {H5E_CANTUNLOCK,   "Unable to unlock object"},
    {H5E_CANTGC,   "Unable to garbage collect"},
    {H5E_CANTGETSIZE,   "Unable to compute size"},

    /* File accessability errors */
    {H5E_FILEEXISTS,   "File already exists"},
    {H5E_FILEOPEN,   "File already open"},
    {H5E_CANTCREATE,   "Unable to create file"},
    {H5E_CANTOPENFILE,   "Unable to open file"},
    {H5E_CANTCLOSEFILE,   "Unable to close file"},
    {H5E_NOTHDF5,   "Not an HDF5 file"},
    {H5E_BADFILE,   "Bad file ID accessed"},
    {H5E_TRUNCATED,   "File has been truncated"},
    {H5E_MOUNT,    "File mount error"},

    /* Generic low-level file I/O errors */
    {H5E_SEEKERROR,  "Seek failed"},
    {H5E_READERROR,   "Read failed"},
    {H5E_WRITEERROR,   "Write failed"},
    {H5E_CLOSEERROR,   "Close failed"},
    {H5E_OVERFLOW,   "Address overflowed"},
    {H5E_FCNTL,         "File control (fcntl) failed"},

    /* Function entry/exit interface errors */
    {H5E_CANTINIT,   "Unable to initialize object"},
    {H5E_ALREADYINIT,   "Object already initialized"},
    {H5E_CANTRELEASE,   "Unable to release object"},

    /* Object atom related errors */
    {H5E_BADATOM,   "Unable to find atom information (already closed?)"},
    {H5E_BADGROUP,   "Unable to find ID group information"},
    {H5E_CANTREGISTER,   "Unable to register new atom"},
    {H5E_CANTINC,        "Unable to increment reference count"},
    {H5E_CANTDEC,        "Unable to decrement reference count"},
    {H5E_NOIDS,        "Out of IDs for group"},

    /* Cache related errors */
    {H5E_CANTFLUSH,   "Unable to flush data from cache"},
    {H5E_CANTSERIALIZE, "Unable to serialize data from cache"},
    {H5E_CANTLOAD,   "Unable to load metadata into cache"},
    {H5E_PROTECT,   "Protected metadata error"},
    {H5E_NOTCACHED,   "Metadata not currently cached"},
    {H5E_SYSTEM,   "Internal error detected"},
    {H5E_CANTINS,   "Unable to insert metadata into cache"},
    {H5E_CANTRENAME,   "Unable to rename metadata"},
    {H5E_CANTPROTECT,   "Unable to protect metadata"},
    {H5E_CANTUNPROTECT,  "Unable to unprotect metadata"},

    /* B-tree related errors */
    {H5E_NOTFOUND,   "Object not found"},
    {H5E_EXISTS,   "Object already exists"},
    {H5E_CANTENCODE,   "Unable to encode value"},
    {H5E_CANTDECODE,   "Unable to decode value"},
    {H5E_CANTSPLIT,   "Unable to split node"},
    {H5E_CANTINSERT,   "Unable to insert object"},
    {H5E_CANTLIST,   "Unable to list node"},

    /* Object header related errors */
    {H5E_LINKCOUNT,   "Bad object header link count"},
    {H5E_VERSION,   "Wrong version number"},
    {H5E_ALIGNMENT,   "Alignment error"},
    {H5E_BADMESG,   "Unrecognized message"},
    {H5E_CANTDELETE,   "Can't delete message"},
    {H5E_BADITER,   "Iteration failed"},

    /* Group related errors */
    {H5E_CANTOPENOBJ,   "Can't open object"},
    {H5E_CANTCLOSEOBJ,   "Can't close object"},
    {H5E_COMPLEN,   "Name component is too long"},
    {H5E_LINK,     "Link count failure"},
    {H5E_SLINK,    "Symbolic link error"},
    {H5E_PATH,    "Problem with path to object"},

    /* Datatype conversion errors */
    {H5E_CANTCONVERT,  "Can't convert datatypes"},
    {H5E_BADSIZE,  "Bad size for object"},

    /* Dataspace errors */
    {H5E_CANTCLIP,  "Can't clip hyperslab region"},
    {H5E_CANTCOUNT,  "Can't count elements"},
    {H5E_CANTSELECT,    "Can't select hyperslab"},
    {H5E_CANTNEXT,      "Can't move to next iterator location"},
    {H5E_BADSELECT,     "Invalid selection"},
    {H5E_CANTCOMPARE,   "Can't compare objects"},

    /* Property list errors */
    {H5E_CANTGET,  "Can't get value"},
    {H5E_CANTSET,  "Can't set value"},
    {H5E_DUPCLASS,  "Duplicate class name in parent class"},

    /* Parallel MPI errors */
    {H5E_MPI,    "Some MPI function failed"},
    {H5E_MPIERRSTR,     "MPI Error String"},

    /* Heap errors */
    {H5E_CANTRESTORE,  "Can't restore condition"},

    /* TBBT errors */
    {H5E_CANTMAKETREE,  "Can't create TBBT tree"},

    /* I/O pipeline errors */
    {H5E_NOFILTER,      "Requested filter is not available"},
    {H5E_CALLBACK,      "Callback failed"},
    {H5E_CANAPPLY,      "Error from filter \"can apply\" callback"},
    {H5E_SETLOCAL,      "Error from filter \"set local\" callback"},
    {H5E_NOENCODER,     "Filter present, but encoder not enabled"},

    /* I/O pipeline errors */
    {H5E_SYSERRSTR,     "System error message"}
};

/* Interface initialization? */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5E_init_interface

#ifdef H5_HAVE_THREADSAFE
/*
 * The per-thread error stack. pthread_once() initializes a special
 * key that will be used by all threads to create a stack specific to
 * each thread individually. The association of stacks to threads will
 * be handled by the pthread library.
 *
 * In order for this macro to work, H5E_get_my_stack() must be preceeded
 * by "H5E_t *estack =".
 */
static H5E_t *    H5E_get_stack(void);
#define H5E_get_my_stack()  H5E_get_stack()
#else /* H5_HAVE_THREADSAFE */
/*
 * The current error stack.
 */
H5E_t    H5E_stack_g[1];
#define H5E_get_my_stack() (H5E_stack_g+0)
#endif /* H5_HAVE_THREADSAFE */


#ifdef H5_HAVE_PARALLEL
/*
 * variables used for MPI error reporting
 */
char  H5E_mpi_error_str[MPI_MAX_ERROR_STRING];
int  H5E_mpi_error_str_len;
#endif

/* Static function declarations */
static herr_t H5E_walk (H5E_direction_t direction, H5E_walk_t func, void *client_data);
static herr_t H5E_walk_cb (int n, H5E_error_t *err_desc, void *client_data);


/*-------------------------------------------------------------------------
 * Function:  H5E_init_interface
 *
 * Purpose:  Initialize the H5E interface. `stderr' is an extern or
 *    function on some systems so we can't initialize
 *    H5E_auto_data_g statically.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, June 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E_init_interface (void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5E_init_interface)

#ifndef H5_HAVE_THREADSAFE
    H5E_stack_g[0].nused = 0;
    H5E_stack_g[0].auto_func = (H5E_auto_t)H5Eprint;
    H5E_stack_g[0].auto_data = stderr;
#endif /* H5_HAVE_THREADSAFE */

    FUNC_LEAVE_NOAPI(SUCCEED)
}


#ifdef H5_HAVE_THREADSAFE
/*-------------------------------------------------------------------------
 * Function:  H5E_get_stack
 *
 * Purpose:  Support function for H5E_get_my_stack() to initialize and
 *              acquire per-thread error stack.
 *
 * Return:  Success:  error stack (H5E_t *)
 *
 *    Failure:  NULL
 *
 * Programmer:  Chee Wai LEE
 *              April 24, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5E_t *
H5E_get_stack(void)
{
    H5E_t *estack;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5E_get_stack)

    estack = pthread_getspecific(H5TS_errstk_key_g);

    if (!estack) {
        /* no associated value with current thread - create one */
        estack = (H5E_t *)H5MM_malloc(sizeof(H5E_t));
        assert(estack);

        /* Set thread specific information */
        estack->nused = 0;
        estack->auto_func = (H5E_auto_t)H5Eprint;
        estack->auto_data = stderr;

        pthread_setspecific(H5TS_errstk_key_g, (void *)estack);
    }

    FUNC_LEAVE_NOAPI(estack)
}
#endif  /* H5_HAVE_THREADSAFE */


/*-------------------------------------------------------------------------
 * Function:  H5Eset_auto
 *
 * Purpose:  Turns on or off automatic printing of errors.  When turned on
 *    (non-null FUNC pointer) any API function which returns an
 *    error indication will first call FUNC passing it CLIENT_DATA
 *    as an argument.
 *
 *    The default values before this function is called are
 *    H5Eprint() with client data being the standard error stream,
 *    stderr.
 *
 *    Automatic stack traversal is always in the H5E_WALK_DOWNWARD
 *    direction.
 *
 * See Also:  H5Ewalk()
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eset_auto(H5E_auto_t func, void *client_data)
{
    H5E_t   *estack;            /* Error stack to operate on */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Eset_auto, FAIL)
    H5TRACE2("e","xx",func,client_data);

    /* Get the thread-specific error stack */
    if((estack = H5E_get_my_stack())==NULL) /*lint !e506 !e774 Make lint 'constant value Boolean' in non-threaded case */
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack")

    /* Set the automatic error reporting info */
    estack->auto_func = func;
    estack->auto_data = client_data;

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Eget_auto
 *
 * Purpose:  Returns the current settings for the automatic error stack
 *    traversal function and its data.  Either (or both) arguments
 *    may be null in which case the value is not returned.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Saturday, February 28, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eget_auto(H5E_auto_t *func, void **client_data)
{
    H5E_t   *estack;            /* Error stack to operate on */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Eget_auto, FAIL)
    H5TRACE2("e","*xx",func,client_data);

    /* Get the thread-specific error stack */
    if((estack = H5E_get_my_stack())==NULL) /*lint !e506 !e774 Make lint 'constant value Boolean' in non-threaded case */
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack")

    /* Set the automatic error reporting info */
    if (func) *func = estack->auto_func;
    if (client_data) *client_data = estack->auto_data;

done:
    FUNC_LEAVE_API(ret_value)
}



/*-------------------------------------------------------------------------
 * Function:  H5Eclear
 *
 * Purpose:  Clears the error stack for the current thread.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eclear(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Eclear, FAIL)
    H5TRACE0("e","");
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Eprint
 *
 * Purpose:  Prints the error stack in some default way.  This is just a
 *    convenience function for H5Ewalk() with a function that
 *    prints error messages.  Users are encouraged to write there
 *    own more specific error handlers.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *  Albert Cheng, 2000/12/02
 *  Show MPI process rank id if applicable.
 *  Albert Cheng, 2001/07/14
 *  Show HDF5 library version information string too.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eprint(FILE *stream)
{
    H5E_t  *estack = H5E_get_my_stack ();
    herr_t  ret_value = FAIL;

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(H5Eprint, FAIL)
    /*NO TRACE*/

    if (!stream) stream = stderr;
    fprintf (stream, "HDF5-DIAG: Error detected in %s ", H5_lib_vers_info_g);
    /* try show the process or thread id in multiple processes cases*/
#ifdef H5_HAVE_PARALLEL
    {   int mpi_rank, mpi_initialized;
  MPI_Initialized(&mpi_initialized);
  if (mpi_initialized){
      MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
      fprintf (stream, "MPI-process %d.", mpi_rank);
  }else
      fprintf (stream, "thread 0.");
    }
#elif defined(H5_HAVE_THREADSAFE)
    fprintf (stream, "thread %lu.", (unsigned long)pthread_self());
#else
    fprintf (stream, "thread 0.");
#endif
    if (estack && estack->nused>0) fprintf (stream, "  Back trace follows.");
    HDfputc ('\n', stream);

    ret_value = H5E_walk (H5E_WALK_DOWNWARD, H5E_walk_cb, (void*)stream);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Ewalk
 *
 * Purpose:  Walks the error stack for the current thread and calls some
 *    function for each error along the way.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ewalk(H5E_direction_t direction, H5E_walk_t func, void *client_data)
{
    herr_t  ret_value;

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(H5Ewalk, FAIL)
    H5TRACE3("e","Edxx",direction,func,client_data);

    ret_value = H5E_walk (direction, func, client_data);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5E_walk_cb
 *
 * Purpose:  This is a default error stack traversal callback function
 *    that prints error messages to the specified output stream.
 *    It is not meant to be called directly but rather as an
 *    argument to the H5Ewalk() function.  This function is called
 *    also by H5Eprint().  Application writers are encouraged to
 *    use this function as a model for their own error stack
 *    walking functions.
 *
 *    N is a counter for how many times this function has been
 *    called for this particular traversal of the stack.  It always
 *    begins at zero for the first error on the stack (either the
 *    top or bottom error, or even both, depending on the traversal
 *    direction and the size of the stack).
 *
 *    ERR_DESC is an error description.  It contains all the
 *    information about a particular error.
 *
 *    CLIENT_DATA is the same pointer that was passed as the
 *    CLIENT_DATA argument of H5Ewalk().  It is expected to be a
 *    file pointer (or stderr if null).
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E_walk_cb(int n, H5E_error_t *err_desc, void *client_data)
{
    FILE    *stream = (FILE *)client_data;
    const char    *maj_str = NULL;
    const char    *min_str = NULL;
    const int    indent = 2;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5E_walk_cb)
    /*NO TRACE*/

    /* Check arguments */
    assert (err_desc);
    if (!client_data) client_data = stderr;

    /* Get descriptions for the major and minor error numbers */
    maj_str = H5Eget_major (err_desc->maj_num);
    min_str = H5Eget_minor (err_desc->min_num);

    /* Print error message */
    fprintf (stream, "%*s#%03d: %s line %u in %s(): %s\n",
       indent, "", n, err_desc->file_name, err_desc->line,
       err_desc->func_name, err_desc->desc);
    fprintf (stream, "%*smajor(%02d): %s\n",
       indent*2, "", err_desc->maj_num, maj_str);
    fprintf (stream, "%*sminor(%02d): %s\n",
       indent*2, "", err_desc->min_num, min_str);

    FUNC_LEAVE_NOAPI(SUCCEED)
}


/*-------------------------------------------------------------------------
 * Function:  H5Eget_major
 *
 * Purpose:  Given a major error number return a constant character string
 *    that describes the error.
 *
 * Return:  Success:  Ptr to a character string.
 *
 *    Failure:  Ptr to "Invalid major error number"
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
const char *
H5Eget_major (H5E_major_t n)
{
    unsigned  i;
    const char *ret_value="Invalid major error number";

    /*
     * WARNING: Do not call the FUNC_ENTER() or FUNC_LEAVE() macros since
     *    they might interact badly with the error stack.  We are
     *    probably calling this function during an error stack
     *    traversal and adding/removing entries as the result of an
     *    error would most likely mess things up.
     */
    FUNC_ENTER_API_NOINIT(H5Eget_major)

    for (i=0; i<NELMTS (H5E_major_mesg_g); i++)
  if (H5E_major_mesg_g[i].error_code==n)
      HGOTO_DONE(H5E_major_mesg_g[i].str)

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5Eget_minor
 *
 * Purpose:  Given a minor error number return a constant character string
 *    that describes the error.
 *
 * Return:  Success:  Ptr to a character string.
 *
 *    Failure:  Ptr to "Invalid minor error number"
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
const char *
H5Eget_minor (H5E_minor_t n)
{
    unsigned  i;
    const char *ret_value="Invalid minor error number";

    /*
     * WARNING: Do not call the FUNC_ENTER() or FUNC_LEAVE() macros since
     *    they might interact badly with the error stack.  We are
     *    probably calling this function during an error stack
     *    traversal and adding/removing entries as the result of an
     *    error would most likely mess things up.
     */
    FUNC_ENTER_API_NOINIT(H5Eget_minor)

    for (i=0; i<NELMTS (H5E_minor_mesg_g); i++)
  if (H5E_minor_mesg_g[i].error_code==n)
      HGOTO_DONE(H5E_minor_mesg_g[i].str)

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5E_push
 *
 * Purpose:  Pushes a new error record onto error stack for the current
 *    thread.  The error has major and minor numbers MAJ_NUM and
 *    MIN_NUM, the name of a function where the error was detected,
 *    the name of the file where the error was detected, the
 *    line within that file, and an error description string.  The
 *    function name, file name, and error description strings must
 *    be statically allocated (the FUNC_ENTER() macro takes care of
 *    the function name and file name automatically, but the
 *    programmer is responsible for the description string).
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_push(H5E_major_t maj_num, H5E_minor_t min_num, const char *function_name,
   const char *file_name, unsigned line, const char *desc)
{
    H5E_t  *estack = H5E_get_my_stack ();

    /*
     * WARNING: We cannot call HERROR() from within this function or else we
     *    could enter infinite recursion.  Furthermore, we also cannot
     *    call any other HDF5 macro or function which might call
     *    HERROR().  HERROR() is called by HRETURN_ERROR() which could
     *    be called by FUNC_ENTER().
     */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5E_push)

    /*
     * Don't fail if arguments are bad.  Instead, substitute some default
     * value.
     */
    if (!function_name) function_name = "Unknown_Function";
    if (!file_name) file_name = "Unknown_File";
    if (!desc) desc = "No description given";

    /*
     * Push the error if there's room.  Otherwise just forget it.
     */
    assert (estack);
    if (estack->nused<H5E_NSLOTS) {
  estack->slot[estack->nused].maj_num = maj_num;
  estack->slot[estack->nused].min_num = min_num;
  estack->slot[estack->nused].func_name = function_name;
  estack->slot[estack->nused].file_name = file_name;
  estack->slot[estack->nused].line = line;
  estack->slot[estack->nused].desc = desc;
  estack->nused++;
    }

    FUNC_LEAVE_NOAPI(SUCCEED)
}


/*-------------------------------------------------------------------------
 * Function:  H5Epush
 *
 * Purpose:  Pushes a new error record onto error stack for the current
 *    thread.  The error has major and minor numbers MAJ_NUM and
 *    MIN_NUM, the name of a function where the error was detected,
 *    the name of the file where the error was detected, the
 *    line within that file, and an error description string.  The
 *    function name, file name, and error description strings must
 *    be statically allocated.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Monday, October 18, 1999
 *
 * Notes:   Basically a public API wrapper around the H5E_push function.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Epush(const char *file, const char *func, unsigned line, H5E_major_t maj,
  H5E_minor_t min, const char *str)
{
    herr_t  ret_value;

    FUNC_ENTER_API(H5Epush, FAIL)
    H5TRACE6("e","ssIuEjEns",file,func,line,maj,min,str);

    ret_value = H5E_push(maj, min, func, file, line, str);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5E_clear
 *
 * Purpose:  Clears the error stack for the current thread.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_clear(void)
{
    H5E_t  *estack = H5E_get_my_stack ();
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5E_clear, FAIL)

    if (estack) estack->nused = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5E_walk
 *
 * Purpose:  Walks the error stack, calling the specified function for
 *    each error on the stack.  The DIRECTION argument determines
 *    whether the stack is walked from the inside out or the
 *    outside in.  The value H5E_WALK_UPWARD means begin with the
 *    most specific error and end at the API; H5E_WALK_DOWNWARD
 *    means to start at the API and end at the inner-most function
 *    where the error was first detected.
 *
 *    The function pointed to by FUNC will be called for each error
 *    in the error stack. It's arguments will include an index
 *    number (beginning at zero regardless of stack traversal
 *    direction), an error stack entry, and the CLIENT_DATA pointer
 *    passed to H5E_print.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E_walk (H5E_direction_t direction, H5E_walk_t func, void *client_data)
{
    H5E_t  *estack = H5E_get_my_stack ();
    int    i;
    herr_t  status;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5E_walk, FAIL)

    /* check args, but rather than failing use some default value */
    if (direction!=H5E_WALK_UPWARD && direction!=H5E_WALK_DOWNWARD) {
  direction = H5E_WALK_UPWARD;
    }

    /* walk the stack */
    assert (estack);
    if (func && H5E_WALK_UPWARD==direction) {
  for (i=0, status=SUCCEED; i<estack->nused && status>=0; i++) {
      status = (func)(i, estack->slot+i, client_data);
  }
    } else if (func && H5E_WALK_DOWNWARD==direction) {
  for (i=estack->nused-1, status=SUCCEED; i>=0 && status>=0; --i) {
      status = (func)(estack->nused-(i+1), estack->slot+i, client_data);
  }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5E_dump_api_stack
 *
 * Purpose:     Private function to dump the error stack during an error in
 *              an API function if a callback function is defined for the
 *              current error stack.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, January 20, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_dump_api_stack(int is_api)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5E_dump_api_stack, FAIL)

    /* Only dump the error stack during an API call */
    if(is_api) {
        H5E_t *estack = H5E_get_my_stack();

        assert(estack);
        if (estack->auto_func)
            (void)((estack->auto_func)(estack->auto_data));
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


