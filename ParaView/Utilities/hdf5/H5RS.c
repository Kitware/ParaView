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
 * Reference counted string algorithms.
 *
 * These are used for various internal strings which get copied multiple times.
 *
 */

#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5RSprivate.h"        /* Reference-counted strings            */

/* Pablo information */
#define PABLO_MASK	H5RS_mask

/* Interface initialization */
static int		interface_initialize_g = 0;
#define INTERFACE_INIT	NULL

/* Private typedefs & structs */
struct H5RS_str_t {
    char *s;            /* String to be reference counted */
    unsigned wrapped;   /* Indicates that the string to be ref-counted is not copied */
    unsigned n;         /* Reference count of number of pointers sharing string */
};

/* Declare a free list to manage the H5RS_str_t struct */
H5FL_DEFINE_STATIC(H5RS_str_t);

/* Declare the PQ free list for the wrapped strings */
H5FL_BLK_DEFINE(str_buf);


/*--------------------------------------------------------------------------
 NAME
    H5RS_xstrdup
 PURPOSE
    Duplicate the string being reference counted
 USAGE
    char *H5RS_xstrdup(s)
        const char *s;          IN: String to duplicate

 RETURNS
    Returns a pointer to a new string on success, NULL on failure.
 DESCRIPTION
    Duplicate a string buffer being reference counted.  Use this instead of
    [H5MM_][x]strdup, in order to use the free-list memory routines.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static char *
H5RS_xstrdup(const char *s)
{
    char *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5RS_xstrdup);

    if (s) {
        ret_value = H5FL_BLK_MALLOC(str_buf,HDstrlen(s) + 1);
        assert (ret_value);
        HDstrcpy(ret_value, s);
    } /* end if */
    else
        ret_value=NULL;

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_xstrdup() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_create
 PURPOSE
    Create a reference counted string
 USAGE
    H5RS_str_t *H5RS_create(s)
        const char *s;          IN: String to initialize ref-counted string with

 RETURNS
    Returns a pointer to a new ref-counted string on success, NULL on failure.
 DESCRIPTION
    Create a reference counted string.  The string passed in is copied into an
    internal buffer.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5RS_str_t *
H5RS_create(const char *s)
{
    H5RS_str_t *ret_value=NULL;   /* Return value */

    FUNC_ENTER_NOAPI(H5RS_create,NULL);

    /* Allocate ref-counted string structure */
    if((ret_value=H5FL_MALLOC(H5RS_str_t))==NULL)
        HGOTO_ERROR(H5E_RS,H5E_NOSPACE,NULL,"memory allocation failed");

    /* Set the internal fields */
    ret_value->s=H5RS_xstrdup(s);
    ret_value->wrapped=0;
    ret_value->n=1;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_create() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_wrap
 PURPOSE
    "Wrap" a reference counted string around an existing string
 USAGE
    H5RS_str_t *H5RS_wrap(s)
        const char *s;          IN: String to wrap ref-counted string around

 RETURNS
    Returns a pointer to a new ref-counted string on success, NULL on failure.
 DESCRIPTION
    Wrap a reference counted string around an existing string, which is not
    duplicated, unless its reference count gets incremented.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5RS_str_t *
H5RS_wrap(const char *s)
{
    H5RS_str_t *ret_value=NULL;   /* Return value */

    FUNC_ENTER_NOAPI(H5RS_wrap,NULL);

    /* Allocate ref-counted string structure */
    if((ret_value=H5FL_MALLOC(H5RS_str_t))==NULL)
        HGOTO_ERROR(H5E_RS,H5E_NOSPACE,NULL,"memory allocation failed");

    /* Set the internal fields */
    ret_value->s=(char*)s;
    ret_value->wrapped=1;
    ret_value->n=1;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_wrap() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_own
 PURPOSE
    Transfer ownership of a regular string to  a reference counted string
 USAGE
    H5RS_str_t *H5RS_own(s)
        const char *s;          IN: String to transfer ownership of

 RETURNS
    Returns a pointer to a new ref-counted string on success, NULL on failure.
 DESCRIPTION
    Transfer ownership of a dynamically allocated string to a reference counted
    string.  The routine which passed in the string should not attempt to free
    it, the reference counting string routines will do that when the reference
    count drops to zero.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5RS_str_t *
H5RS_own(char *s)
{
    H5RS_str_t *ret_value=NULL;   /* Return value */

    FUNC_ENTER_NOAPI(H5RS_own,NULL);

    /* Allocate ref-counted string structure */
    if((ret_value=H5FL_MALLOC(H5RS_str_t))==NULL)
        HGOTO_ERROR(H5E_RS,H5E_NOSPACE,NULL,"memory allocation failed");

    /* Set the internal fields */
    ret_value->s=s;
    ret_value->wrapped=0;
    ret_value->n=1;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_own() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_decr
 PURPOSE
    Decrement the reference count for a ref-counted string
 USAGE
    herr_t H5RS_decr(rs)
        H5RS_str_t *rs;     IN/OUT: Ref-counted string to decrement count of

 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Decrement the reference count for a reference counted string.  If the
    reference count drops to zero, the reference counted string is deleted.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5RS_decr(H5RS_str_t *rs)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5RS_decr,FAIL);

    /* Sanity check */
    assert(rs);
    assert(rs->n > 0);

    /* Decrement reference count for string */
    if((--rs->n)==0) {
        if(!rs->wrapped)
            H5FL_BLK_FREE(str_buf,rs->s);
        H5FL_FREE(H5RS_str_t,rs);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_decr() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_incr
 PURPOSE
    Increment the reference count for a ref-counted string
 USAGE
    herr_t H5RS_incr(rs)
        H5RS_str_t *rs;     IN/OUT: Ref-counted string to increment count of

 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Increment the reference count for a reference counted string.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5RS_incr(H5RS_str_t *rs)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5RS_incr,FAIL);

    /* Sanity check */
    assert(rs);
    assert(rs->n > 0);

    /* If the ref-counted string started life as a wrapper around an existing
     * string, duplicate the string now, so that the wrapped string can go out
     * scope appropriately.
     */
    if(rs->wrapped) {
        rs->s=H5RS_xstrdup(rs->s);
        rs->wrapped=0;
    } /* end if */

    /* Increment reference count for string */
    rs->n++;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_incr() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_dup
 PURPOSE
    "Duplicate" a ref-counted string
 USAGE
    H5RS_str_t H5RS_incr(rs)
        H5RS_str_t *rs;     IN/OUT: Ref-counted string to "duplicate"

 RETURNS
    Returns a pointer to ref-counted string on success, NULL on failure.
 DESCRIPTION
    Increment the reference count for the reference counted string and return
    a pointer to it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5RS_str_t *
H5RS_dup(H5RS_str_t *ret_value)
{
    FUNC_ENTER_NOAPI(H5RS_dup,NULL);

    /* Check for valid reference counted string */
    if(ret_value!=NULL)
        /* Increment reference count for string */
        ret_value->n++;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_dup() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_cmp
 PURPOSE
    Compare two ref-counted strings
 USAGE
    int H5RS_cmp(rs1,rs2)
        const H5RS_str_t *rs1;  IN: First Ref-counted string to compare
        const H5RS_str_t *rs2;  IN: Second Ref-counted string to compare

 RETURNS
    Returns positive, negative or 0 for comparison of two r-strings [same as
    strcmp()]
 DESCRIPTION
    Compare two ref-counted strings and return a value indicating their sort
    order [same as strcmp()]
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5RS_cmp(const H5RS_str_t *rs1, const H5RS_str_t *rs2)
{
    /* Can't return invalid value from this function */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5RS_cmp);

    /* Sanity check */
    assert(rs1);
    assert(rs1->s);
    assert(rs2);
    assert(rs2->s);

    FUNC_LEAVE_NOAPI(HDstrcmp(rs1->s,rs2->s));
} /* end H5RS_cmp() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_len
 PURPOSE
    Compute the length of a ref-counted string
 USAGE
    ssize_t H5RS_cmp(rs)
        const H5RS_str_t *rs;  IN: Ref-counted string to compute length of

 RETURNS
    Returns non-negative value on success, negative value on failure
 DESCRIPTION
    Compute the length of a ref-counted string.  [same as strlen()]
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
ssize_t
H5RS_len(const H5RS_str_t *rs)
{
    ssize_t ret_value;     /* Return value */

    FUNC_ENTER_NOAPI(H5RS_len,FAIL);

    /* Sanity check */
    assert(rs);
    assert(rs->s);

    ret_value=(ssize_t)HDstrlen(rs->s);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_len() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_get_str
 PURPOSE
    Get a pointer to the internal string contained in a ref-counted string
 USAGE
    char *H5RS_get_str(rs)
        const H5RS_str_t *rs;   IN: Ref-counted string to get internal string from

 RETURNS
    Returns a pointer to the internal string being ref-counted on success,
        NULL on failure.
 DESCRIPTION
    Gets a pointer to the internal string being reference counted.  This
    pointer is volatile and might be invalid is further calls to the H5RS
    API are made.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5RS_get_str(const H5RS_str_t *rs)
{
    char *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(H5RS_get_str,NULL);

    /* Sanity check */
    assert(rs);
    assert(rs->s);

    ret_value=rs->s;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_get_str() */


/*--------------------------------------------------------------------------
 NAME
    H5RS_get_count
 PURPOSE
    Get the reference count for a ref-counted string
 USAGE
    unsigned H5RS_get_count(rs)
        const H5RS_str_t *rs;   IN: Ref-counted string to get internal count from

 RETURNS
    Returns the number of references to the internal string being ref-counted on success,
        0 on failure.
 DESCRIPTION
    Gets the count of references to the reference counted string.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
unsigned
H5RS_get_count(const H5RS_str_t *rs)
{
    unsigned ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(H5RS_get_count,0);

    /* Sanity check */
    assert(rs);
    assert(rs->n>0);

    ret_value=rs->n;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RS_get_count() */

