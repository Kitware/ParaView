/*
 * Copyright (C) 1998 NCSA HDF
 *                    All rights reserved.
 *                    
 * Purpose:     Provides error handling in the form of a stack.  The
 *              FUNC_ENTER() macro clears the error stack whenever an API
 *              function is entered.  When an error is detected, an entry is
 *              pushed onto the stack.  As the functions unwind additional
 *              entries are pushed onto the stack. The API function will
 *              return some indication that an error occurred and the
 *              application can print the error stack.
 *
 *              Certain API functions in the H5E package (such as H5Eprint())
 *              do not clear the error stack.  Otherwise, any function which
 *              doesn't have an underscore immediately after the package name
 *              will clear the error stack.  For instance, H5Fopen() clears
 *              the error stack while H5F_open() does not.
 *
 *              An error stack has a fixed maximum size.  If this size is
 *              exceeded then the stack will be truncated and only the
 *              inner-most functions will have entries on the stack. This is
 *              expected to be a rare condition.
 *
 *              Each thread has its own error stack, but since
 *              multi-threading has not been added to the library yet, this
 *              package maintains a single error stack. The error stack is
 *              statically allocated to reduce the complexity of handling
 *              errors within the H5E package.
 *
 */
#include "H5private.h"          /* Generic Functions                      */
#include "H5Iprivate.h"         /* IDs                                    */
#include "H5Eprivate.h"         /* Private error routines                 */
#include "H5MMprivate.h"        /* Memory management                      */

#define PABLO_MASK      H5E_mask

static const H5E_major_mesg_t H5E_major_mesg_g[] = {
    {H5E_NONE_MAJOR,    "No error"},
    {H5E_ARGS,          "Function arguments"},
    {H5E_RESOURCE,      "Resource unavailable"},
    {H5E_INTERNAL,      "Internal HDF5 error"},
    {H5E_FILE,          "File interface"},
    {H5E_IO,            "Low-level I/O layer"},
    {H5E_FUNC,          "Function entry/exit"},
    {H5E_ATOM,          "Atom layer"},
    {H5E_CACHE,         "Meta data cache layer"},
    {H5E_BTREE,         "B-tree layer"},
    {H5E_SYM,           "Symbol table layer"},
    {H5E_HEAP,          "Heap layer"},
    {H5E_OHDR,          "Object header layer"},
    {H5E_DATATYPE,      "Datatype interface"},
    {H5E_DATASPACE,     "Dataspace interface"},
    {H5E_DATASET,       "Dataset interface"},
    {H5E_STORAGE,       "Data storage layer"},
    {H5E_PLIST,         "Property list interface"},
    {H5E_ATTR,          "Attribute layer"},
    {H5E_PLINE,         "Data filters layer"},
    {H5E_EFL,           "External file list"},
    {H5E_REFERENCE,     "References layer"},
    {H5E_VFL,           "Virtual File Layer"},
    {H5E_TBBT,          "Threaded, Balanced, Binary Trees"},
};

static const H5E_minor_mesg_t H5E_minor_mesg_g[] = {
    {H5E_NONE_MINOR,    "No error"},

    /* Argument errors */
    {H5E_UNINITIALIZED, "Information is uninitialized"},
    {H5E_UNSUPPORTED,   "Feature is unsupported"},
    {H5E_BADTYPE,       "Inappropriate type"},
    {H5E_BADRANGE,      "Out of range"},
    {H5E_BADVALUE,      "Bad value"},

    /* Resource errors */
    {H5E_NOSPACE,       "No space available for allocation"},
    {H5E_CANTCOPY,      "Unable to copy object"},
    {H5E_CANTFREE,      "Unable to free object"},

    /* File accessability errors */
    {H5E_FILEEXISTS,    "File already exists"},
    {H5E_FILEOPEN,      "File already open"},
    {H5E_CANTCREATE,    "Unable to create file"},
    {H5E_CANTOPENFILE,  "Unable to open file"},
    {H5E_CANTCLOSEFILE,         "Unable to close file"},
    {H5E_NOTHDF5,       "Not an HDF5 file"},
    {H5E_BADFILE,       "Bad file ID accessed"},
    {H5E_TRUNCATED,     "File has been truncated"},
    {H5E_MOUNT,         "File mount error"},

    /* Generic low-level file I/O errors */
    {H5E_SEEKERROR,     "Seek failed"},
    {H5E_READERROR,     "Read failed"},
    {H5E_WRITEERROR,    "Write failed"},
    {H5E_CLOSEERROR,    "Close failed"},
    {H5E_OVERFLOW,      "Address overflowed"},

    /* Function entry/exit interface errors */
    {H5E_CANTINIT,      "Unable to initialize"},
    {H5E_ALREADYINIT,   "Object already initialized"},

    /* Object atom related errors */
    {H5E_BADATOM,       "Unable to find atom information (already closed?)"},
    {H5E_CANTREGISTER,  "Unable to  register new atom"},

    /* Cache related errors */
    {H5E_CANTFLUSH,     "Unable to flush data from cache"},
    {H5E_CANTLOAD,      "Unable to load meta data into cache"},
    {H5E_PROTECT,       "Protected meta data error"},
    {H5E_NOTCACHED,     "Meta data not currently cached"},

    /* B-tree related errors */
    {H5E_NOTFOUND,      "Object not found"},
    {H5E_EXISTS,        "Object already exists"},
    {H5E_CANTENCODE,    "Unable to encode value"},
    {H5E_CANTDECODE,    "Unable to decode value"},
    {H5E_CANTSPLIT,     "Unable to split node"},
    {H5E_CANTINSERT,    "Unable to insert object"},
    {H5E_CANTLIST,      "Unable to list node"},

    /* Object header related errors */
    {H5E_LINKCOUNT,     "Bad object header link count"},
    {H5E_VERSION,       "Wrong version number"},
    {H5E_ALIGNMENT,     "Alignment error"},
    {H5E_BADMESG,       "Unrecognized message"},
    {H5E_CANTDELETE,    "Can't delete message"},

    /* Group related errors */
    {H5E_CANTOPENOBJ,   "Can't open object"},
    {H5E_COMPLEN,       "Name component is too long"},
    {H5E_CWG,           "Problem with current working group"},
    {H5E_LINK,          "Link count failure"},
    {H5E_SLINK,         "Symbolic link error"},

    /* Datatype conversion errors */
    {H5E_CANTCONVERT,           "Can't convert datatypes"},

    /* Datatype conversion errors */
    {H5E_MPI,           "Some MPI function failed"}
};

/* Interface initialization? */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5E_init_interface
static herr_t H5E_init_interface (void);
const hbool_t H5E_clearable_g = TRUE;   /* DO NOT CHANGE */

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
H5E_t *H5E_get_stack(void);
#define H5E_get_my_stack()  H5E_get_stack()
#else
/*
 * The error stack.  Eventually we'll have some sort of global table so each
 * thread has it's own stack.  The stacks will be created on demand when the
 * thread first calls H5E_push().  */
H5E_t           H5E_stack_g[1];
#define H5E_get_my_stack()      (H5E_stack_g+0)
#endif

/*
 * Automatic error stack traversal occurs if the traversal callback function
 * is non null and an API function is about to return an error.  These should
 * probably be part of the error stack so they're local to a thread.
 */
herr_t (*H5E_auto_g)(void*) = (herr_t(*)(void*))H5Eprint;
void *H5E_auto_data_g = NULL;


#ifdef H5_HAVE_THREADSAFE
/*-------------------------------------------------------------------------
 * Function:    H5E_get_stack
 *
 * Purpose:     Support function for H5E_get_my_stack() to initialize and
 *              acquire per-thread error stack.
 *
 * Return:      Success:        error stack (H5E_t *)
 *
 *              Failure:        NULL
 *
 * Programmer:  Chee Wai LEE
 *              April 24, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5E_t *H5E_get_stack() {
  H5E_t *estack;

  if ((estack = pthread_getspecific(H5TS_errstk_key_g))!=NULL) {
    return estack;
  } else {
    /* no associated value with current thread - create one */
    estack = (H5E_t *)malloc(sizeof(H5E_t));
    pthread_setspecific(H5TS_errstk_key_g, (void *)estack);
    return estack;
  }
}
#endif


/*-------------------------------------------------------------------------
 * Function:    H5E_init_interface
 *
 * Purpose:     Initialize the H5E interface. `stderr' is an extern or
 *              function on some systems so we can't initialize
 *              H5E_auto_data_g statically.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
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
  FUNC_ENTER(H5E_init_interface, FAIL);
  H5E_auto_data_g = stderr;
  FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Eset_auto
 *
 * Purpose:     Turns on or off automatic printing of errors.  When turned on
 *              (non-null FUNC pointer) any API function which returns an
 *              error indication will first call FUNC passing it CLIENT_DATA
 *              as an argument.
 *
 *              The default values before this function is called are
 *              H5Eprint() with client data being the standard error stream,
 *              stderr.
 *
 *              Automatic stack traversal is always in the H5E_WALK_DOWNWARD
 *              direction.
 *              
 * See Also:    H5Ewalk()
 *
 * Return:      Non-negative on success/Negative on failure
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
    FUNC_ENTER (H5Eset_auto, FAIL);
    H5TRACE2("e","xx",func,client_data);
    
    H5E_auto_g = func;
    H5E_auto_data_g = client_data;

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Eget_auto
 *
 * Purpose:     Returns the current settings for the automatic error stack
 *              traversal function and its data.  Either (or both) arguments
 *              may be null in which case the value is not returned.
 *
 * Return:      Non-negative on success/Negative on failure
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
    FUNC_ENTER (H5Eget_auto, FAIL);
    H5TRACE2("e","*x*x",func,client_data);

    if (func) *func = H5E_auto_g;
    if (client_data) *client_data = H5E_auto_data_g;

    FUNC_LEAVE (SUCCEED);
}



/*-------------------------------------------------------------------------
 * Function:    H5Eclear
 *
 * Purpose:     Clears the error stack for the current thread.
 *
 * Return:      Non-negative on success/Negative on failure
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
    FUNC_ENTER (H5Eclear, FAIL);
    H5TRACE0("e","");
    /* FUNC_ENTER() does all the work */
    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Eprint
 *
 * Purpose:     Prints the error stack in some default way.  This is just a
 *              convenience function for H5Ewalk() with a function that
 *              prints error messages.  Users are encouraged to write there
 *              own more specific error handlers.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *      Albert Cheng, 2000/12/02
 *      Show MPI process rank id if applicable.
 *      Albert Cheng, 2001/07/14
 *      Show HDF5 library version information string too.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eprint(FILE *stream)
{
    H5E_t       *estack = H5E_get_my_stack ();
    hbool_t     H5E_clearable_g = FALSE; /*override global*/
    herr_t      status = FAIL;
    
    FUNC_ENTER (H5Eprint, FAIL);
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
    fprintf (stream, "thread %d.", (int)pthread_self());
#else
    fprintf (stream, "thread 0.");
#endif
    if (estack && estack->nused>0) fprintf (stream, "  Back trace follows.");
    HDfputc ('\n', stream);
    status = H5E_walk (H5E_WALK_DOWNWARD, H5Ewalk_cb, (void*)stream);
    
    FUNC_LEAVE (status);
}


/*-------------------------------------------------------------------------
 * Function:    H5Ewalk
 *
 * Purpose:     Walks the error stack for the current thread and calls some
 *              function for each error along the way.
 *
 * Return:      Non-negative on success/Negative on failure
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
    hbool_t     H5E_clearable_g = FALSE; /*override global*/
    herr_t      status = FAIL;

    FUNC_ENTER (H5Ewalk, FAIL);
    H5TRACE3("e","Edxx",direction,func,client_data);
    status = H5E_walk (direction, func, client_data);
    FUNC_LEAVE (status);
}


/*-------------------------------------------------------------------------
 * Function:    H5Ewalk_cb
 *
 * Purpose:     This is a default error stack traversal callback function
 *              that prints error messages to the specified output stream.
 *              It is not meant to be called directly but rather as an
 *              argument to the H5Ewalk() function.  This function is called
 *              also by H5Eprint().  Application writers are encouraged to
 *              use this function as a model for their own error stack
 *              walking functions.
 *
 *              N is a counter for how many times this function has been
 *              called for this particular traversal of the stack.  It always
 *              begins at zero for the first error on the stack (either the
 *              top or bottom error, or even both, depending on the traversal
 *              direction and the size of the stack).
 *
 *              ERR_DESC is an error description.  It contains all the
 *              information about a particular error.
 *
 *              CLIENT_DATA is the same pointer that was passed as the
 *              CLIENT_DATA argument of H5Ewalk().  It is expected to be a
 *              file pointer (or stderr if null).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ewalk_cb(int n, H5E_error_t *err_desc, void *client_data)
{
    FILE                *stream = (FILE *)client_data;
    const char          *maj_str = NULL;
    const char          *min_str = NULL;
    const int           indent = 2;

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

    return SUCCEED;
}


/*-------------------------------------------------------------------------
 * Function:    H5Eget_major
 *
 * Purpose:     Given a major error number return a constant character string
 *              that describes the error.
 *
 * Return:      Success:        Ptr to a character string.
 *
 *              Failure:        Ptr to "Invalid major error number"
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
    unsigned    i;
    
    /*
     * WARNING: Do not call the FUNC_ENTER() or FUNC_LEAVE() macros since
     *          they might interact badly with the error stack.  We are
     *          probably calling this function during an error stack
     *          traversal and adding/removing entries as the result of an
     *          error would most likely mess things up.
     */
    for (i=0; i<NELMTS (H5E_major_mesg_g); i++) {
        if (H5E_major_mesg_g[i].error_code==n) {
            return H5E_major_mesg_g[i].str;
        }
    }

    return "Invalid major error number";
}


/*-------------------------------------------------------------------------
 * Function:    H5Eget_minor
 *
 * Purpose:     Given a minor error number return a constant character string
 *              that describes the error.
 *
 * Return:      Success:        Ptr to a character string.
 *
 *              Failure:        Ptr to "Invalid minor error number"
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
    unsigned    i;
    
    /*
     * WARNING: Do not call the FUNC_ENTER() or FUNC_LEAVE() macros since
     *          they might interact badly with the error stack.  We are
     *          probably calling this function during an error stack
     *          traversal and adding/removing entries as the result of an
     *          error would most likely mess things up.
     */
    for (i=0; i<NELMTS (H5E_minor_mesg_g); i++) {
        if (H5E_minor_mesg_g[i].error_code==n) {
            return H5E_minor_mesg_g[i].str;
        }
    }

    return "Invalid minor error number";
}


/*-------------------------------------------------------------------------
 * Function:    H5E_push
 *
 * Purpose:     Pushes a new error record onto error stack for the current
 *              thread.  The error has major and minor numbers MAJ_NUM and
 *              MIN_NUM, the name of a function where the error was detected,
 *              the name of the file where the error was detected, the
 *              line within that file, and an error description string.  The
 *              function name, file name, and error description strings must
 *              be statically allocated (the FUNC_ENTER() macro takes care of
 *              the function name and file name automatically, but the
 *              programmer is responsible for the description string).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_push(H5E_major_t maj_num, H5E_minor_t min_num, const char *function_name,
         const char *file_name, unsigned line, const char *desc)
{
    H5E_t       *estack = H5E_get_my_stack ();
    
    /*
     * WARNING: We cannot call HERROR() from within this function or else we
     *          could enter infinite recursion.  Furthermore, we also cannot
     *          call any other HDF5 macro or function which might call
     *          HERROR().  HERROR() is called by HRETURN_ERROR() which could
     *          be called by FUNC_ENTER().
     */

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
    
    return SUCCEED; /*don't use FUNC_LEAVE() here */
}


/*-------------------------------------------------------------------------
 * Function:    H5Epush
 *
 * Purpose:     Pushes a new error record onto error stack for the current
 *              thread.  The error has major and minor numbers MAJ_NUM and
 *              MIN_NUM, the name of a function where the error was detected,
 *              the name of the file where the error was detected, the
 *              line within that file, and an error description string.  The
 *              function name, file name, and error description strings must
 *              be statically allocated.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, October 18, 1999
 *
 * Notes:       Basically a public API wrapper around the H5E_push function.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Epush(const char *file, const char *func, unsigned line, H5E_major_t maj,
        H5E_minor_t min, const char *str)
{
    herr_t      ret_value;
    
    FUNC_ENTER(H5Epush, FAIL);
    H5TRACE6("e","ssIuEjEns",file,func,line,maj,min,str);
    ret_value = H5E_push(maj, min, func, file, line, str);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5E_clear
 *
 * Purpose:     Clears the error stack for the current thread.
 *
 * Return:      Non-negative on success/Negative on failure
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
    H5E_t       *estack = H5E_get_my_stack ();

    FUNC_ENTER(H5E_clear, FAIL);
    if (estack) estack->nused = 0;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5E_walk
 *
 * Purpose:     Walks the error stack, calling the specified function for
 *              each error on the stack.  The DIRECTION argument determines
 *              whether the stack is walked from the inside out or the
 *              outside in.  The value H5E_WALK_UPWARD means begin with the
 *              most specific error and end at the API; H5E_WALK_DOWNWARD
 *              means to start at the API and end at the inner-most function
 *              where the error was first detected.
 *
 *              The function pointed to by FUNC will be called for each error
 *              in the error stack. It's arguments will include an index
 *              number (beginning at zero regardless of stack traversal
 *              direction), an error stack entry, and the CLIENT_DATA pointer
 *              passed to H5E_print.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_walk (H5E_direction_t direction, H5E_walk_t func, void *client_data)
{
    H5E_t       *estack = H5E_get_my_stack ();
    int         i;
    herr_t      status;

    FUNC_ENTER(H5E_walk, FAIL);

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
    
    FUNC_LEAVE(SUCCEED);
}

