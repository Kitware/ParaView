/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5MM.c
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Memory management functions.
 *
 * Modifications:       
 *
 *-------------------------------------------------------------------------
 */
#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"

/* Interface initialization? */
#define PABLO_MASK H5MM_mask
static int interface_initialize_g = 0;
#define INTERFACE_INIT NULL


/*-------------------------------------------------------------------------
 * Function:    H5MM_realloc
 *
 * Purpose:     Just like the POSIX version of realloc(3). Specifically, the
 *              following calls are equivalent
 *
 *              H5MM_realloc (NULL, size) <==> H5MM_malloc (size)
 *              H5MM_realloc (ptr, 0)     <==> H5MM_xfree (ptr)
 *              H5MM_realloc (NULL, 0)    <==> NULL
 *
 * Return:      Success:        Ptr to new memory or NULL if the memory
 *                              was freed.
 *
 *              Failure:        abort()
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_realloc(void *mem, size_t size)
{
    if (!mem) {
        if (0 == size) return NULL;
        mem = H5MM_malloc(size);

    } else if (0 == size) {
        mem = H5MM_xfree(mem);

    } else {
        mem = HDrealloc(mem, size);
        assert(mem);
    }

    return mem;
}


/*-------------------------------------------------------------------------
 * Function:    H5MM_xstrdup
 *
 * Purpose:     Duplicates a string.  If the string to be duplicated is the
 *              null pointer, then return null.  If the string to be duplicated
 *              is the empty string then return a new empty string.
 *
 * Return:      Success:        Ptr to a new string (or null if no string).
 *
 *              Failure:        abort()
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
char *
H5MM_xstrdup(const char *s)
{
    char        *mem;

    if (!s) return NULL;
    mem = H5MM_malloc(HDstrlen(s) + 1);
    assert (mem);
    HDstrcpy(mem, s);
    return mem;
}


/*-------------------------------------------------------------------------
 * Function:    H5MM_strdup
 *
 * Purpose:     Duplicates a string.  If the string to be duplicated is the
 *              null pointer, then return null.  If the string to be duplicated
 *              is the empty string then return a new empty string.
 *
 * Return:      Success:        Ptr to a new string (or null if no string).
 *
 *              Failure:        abort()
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
char *
H5MM_strdup(const char *s)
{
    char        *mem;

    FUNC_ENTER (H5MM_strdup, NULL);

    if (!s) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, NULL,
                       "null string");
    }
    if (NULL==(mem = H5MM_malloc(HDstrlen(s) + 1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    HDstrcpy(mem, s);

    FUNC_LEAVE (mem);
}


/*-------------------------------------------------------------------------
 * Function:    H5MM_xfree
 *
 * Purpose:     Just like free(3) except null pointers are allowed as
 *              arguments, and the return value (always NULL) can be
 *              assigned to the pointer whose memory was just freed:
 *
 *                      thing = H5MM_xfree (thing);
 *
 * Return:      Success:        NULL
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_xfree(void *mem)
{
    if (mem) HDfree(mem);
    return NULL;
}
