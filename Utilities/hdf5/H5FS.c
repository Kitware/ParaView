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
 * Purpose:  Provides internal function tracing in the form of a stack.
 *    The FUNC_ENTER() macro adds the function name to the function
 *              stack whenever a function is entered.
 *    As the functions return with FUNC_LEAVE,
 *    entries are removed from the stack.
 *
 *    A function stack has a fixed maximum size.  If this size is
 *    exceeded then the stack will be truncated and only the
 *    first called functions will have entries on the stack. This is
 *    expected to be a rare condition.
 *
 */


#include "H5private.h"    /* Generic Functions        */
#include "H5FSprivate.h"  /* Private function stack routines    */

#ifdef H5_HAVE_FUNCSTACK

#ifdef H5_HAVE_THREADSAFE
/*
 * The per-thread function stack. pthread_once() initializes a special
 * key that will be used by all threads to create a stack specific to
 * each thread individually. The association of stacks to threads will
 * be handled by the pthread library.
 *
 * In order for this macro to work, H5FS_get_my_stack() must be preceeded
 * by "H5FS_t *fstack =".
 */
static H5FS_t *H5FS_get_stack(void);
#define H5FS_get_my_stack()  H5FS_get_stack()
#else /* H5_HAVE_THREADSAFE */
/*
 * The function stack.  Eventually we'll have some sort of global table so each
 * thread has it's own stack.  The stacks will be created on demand when the
 * thread first calls H5FS_push().  */
H5FS_t    H5FS_stack_g[1];
#define H5FS_get_my_stack()  (H5FS_stack_g+0)
#endif /* H5_HAVE_THREADSAFE */


#ifdef H5_HAVE_THREADSAFE
/*-------------------------------------------------------------------------
 * Function:  H5FS_get_stack
 *
 * Purpose:  Support function for H5FS_get_my_stack() to initialize and
 *              acquire per-thread function stack.
 *
 * Return:  Success:  function stack (H5FS_t *)
 *
 *    Failure:  NULL
 *
 * Programmer:  Quincey Koziol
 *              February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FS_t *
H5FS_get_stack(void)
{
    H5FS_t *fstack;

    FUNC_ENTER_NOAPI_NOFUNC_NOFS(H5FS_get_stack);

    fstack = pthread_getspecific(H5TS_funcstk_key_g);
    if (!fstack) {
        /* no associated value with current thread - create one */
        fstack = (H5FS_t *)HDmalloc(sizeof(H5FS_t));  /* Don't use H5MM_malloc() here, it causes infinite recursion */
        pthread_setspecific(H5TS_funcstk_key_g, (void *)fstack);
  fstack->nused=0;
    }

    FUNC_LEAVE_NOAPI_NOFS(fstack);
} /* end H5FS_get_stack() */
#endif  /* H5_HAVE_THREADSAFE */


/*-------------------------------------------------------------------------
 * Function:  H5FS_print
 *
 * Purpose:  Prints the function stack in some default way.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              THursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_print(FILE *stream)
{
    H5FS_t  *fstack = H5FS_get_my_stack (); /* Get the correct function stack */
    const int  indent = 2;             /* Indention level */
    int         i;                      /* Local index ariable */

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOFUNC_NOFS(H5FS_print);

    /* Sanity check */
    assert(fstack);

    /* Default to outputting information to stderr */
    if (!stream)
        stream = stderr;

    HDfprintf (stream, "HDF5-DIAG: Function stack from %s ", H5_lib_vers_info_g);
    /* try show the process or thread id in multiple processes cases*/
#ifdef H5_HAVE_THREADSAFE
    HDfprintf (stream, "thread %d.", (int)pthread_self());
#else  /* H5_HAVE_THREADSAFE */
    HDfprintf (stream, "thread 0.");
#endif  /* H5_HAVE_THREADSAFE */
    if (fstack && fstack->nused>0)
        HDfprintf (stream, "  Back trace follows.");
    HDfputc ('\n', stream);

    for (i=fstack->nused-1; i>=0; --i)
        HDfprintf(stream, "%*s#%03d: Routine: %s\n", indent, "", i, fstack->slot[i]);

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5FS_print() */


/*-------------------------------------------------------------------------
 * Function:  H5FS_push
 *
 * Purpose:  Pushes a new record onto function stack for the current
 *    thread.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Thursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_push(const char *func_name)
{
    H5FS_t  *fstack = H5FS_get_my_stack ();

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOFUNC_NOFS(H5FS_push);

    /* Sanity check */
    assert (fstack);
    assert (func_name);

    /*
     * Push the function if there's room.  Otherwise just increment count
     */
    if (fstack->nused<H5FS_NSLOTS)
  fstack->slot[fstack->nused] = func_name;
    fstack->nused++;

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5FS_push() */


/*-------------------------------------------------------------------------
 * Function:  H5FS_pop
 *
 * Purpose:  Pops a record off function stack for the current thread.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Thursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_pop(void)
{
    H5FS_t  *fstack = H5FS_get_my_stack ();

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOFUNC_NOFS(H5FS_pop);

    /* Sanity check */
    assert (fstack);
    assert (fstack->nused>0);

    /* Pop the function. */
    fstack->nused--;

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5FS_pop() */

#endif /* H5_HAVE_FUNCSTACK */
