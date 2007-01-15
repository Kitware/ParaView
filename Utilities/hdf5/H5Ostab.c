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

/*-------------------------------------------------------------------------
 *
 * Created:             H5Ostab.c
 *                      Aug  6 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Symbol table messages.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#define H5O_PACKAGE  /*suppress error about including H5Opkg    */
#define H5G_PACKAGE  /*suppress error about including H5Gpkg    */

#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free lists                           */
#include "H5Gpkg.h"    /* Groups        */
#include "H5Opkg.h"             /* Object headers      */


/* PRIVATE PROTOTYPES */
static void *H5O_stab_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_stab_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_stab_copy(const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_stab_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_stab_free(void *_mesg);
static herr_t H5O_stab_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg, hbool_t adj_link);
static herr_t H5O_stab_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
           FILE * stream, int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_STAB[1] = {{
    H5O_STAB_ID,              /*message id number             */
    "stab",                   /*message name for debugging    */
    sizeof(H5O_stab_t),       /*native message size           */
    H5O_stab_decode,          /*decode message                */
    H5O_stab_encode,          /*encode message                */
    H5O_stab_copy,            /*copy the native value         */
    H5O_stab_size,            /*size of symbol table entry    */
    NULL,                     /*default reset method          */
    H5O_stab_free,          /* free method      */
    H5O_stab_delete,          /* file delete method    */
    NULL,      /* link method      */
    NULL,          /*get share method    */
    NULL,       /*set share method    */
    H5O_stab_debug,           /*debug the message             */
}};

/* Declare a free list to manage the H5O_stab_t struct */
H5FL_DEFINE_STATIC(H5O_stab_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_decode
 *
 * Purpose:     Decode a symbol table message and return a pointer to
 *              a newly allocated one.
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_stab_decode(H5F_t *f, hid_t UNUSED dxpl_id, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5O_stab_t          *stab=NULL;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_decode);

    /* check args */
    assert(f);
    assert(p);
    assert(!sh);

    /* decode */
    if (NULL==(stab = H5FL_CALLOC(H5O_stab_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    H5F_addr_decode(f, &p, &(stab->btree_addr));
    H5F_addr_decode(f, &p, &(stab->heap_addr));

    /* Set return value */
    ret_value=stab;

done:
    if(ret_value==NULL) {
        if(stab!=NULL)
            H5FL_FREE(H5O_stab_t,stab);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_encode
 *
 * Purpose:     Encodes a symbol table message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5O_stab_t       *stab = (const H5O_stab_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_encode);

    /* check args */
    assert(f);
    assert(p);
    assert(stab);

    /* encode */
    H5F_addr_encode(f, &p, stab->btree_addr);
    H5F_addr_encode(f, &p, stab->heap_addr);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_fast
 *
 * Purpose:     Initializes a new message struct with info from the cache of
 *              a symbol table entry.
 *
 * Return:      Success:        Ptr to message struct, allocated if none
 *                              supplied.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_stab_fast(const H5G_cache_t *cache, const H5O_class_t *type, void *_mesg)
{
    H5O_stab_t          *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_fast);

    /* check args */
    assert(cache);
    assert(type);

    if (H5O_STAB == type) {
        if (_mesg) {
      ret_value = (H5O_stab_t *) _mesg;
        } else if (NULL==(ret_value = H5FL_MALLOC(H5O_stab_t))) {
      HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
  }
        ret_value->btree_addr = cache->stab.btree_addr;
        ret_value->heap_addr = cache->stab.heap_addr;
    }
    else
        ret_value=NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_stab_copy(const void *_mesg, void *_dest, unsigned UNUSED update_flags)
{
    const H5O_stab_t       *stab = (const H5O_stab_t *) _mesg;
    H5O_stab_t             *dest = (H5O_stab_t *) _dest;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_copy);

    /* check args */
    assert(stab);
    if (!dest && NULL==(dest = H5FL_MALLOC(H5O_stab_t)))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* copy */
    *dest = *stab;

    /* Set return value */
    ret_value=dest;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting
 *              the message type or size fields, but only the data fields.
 *              This function doesn't take into account alignment.
 *
 * Return:      Success:        Message data size in bytes without alignment.
 *
 *              Failure:        zero
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_stab_size(const H5F_t *f, const void UNUSED *_mesg)
{
    size_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_size);

    /* Set return value */
    ret_value=2 * H5F_SIZEOF_ADDR(f);

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_stab_free
 *
 * Purpose:  Free's the message
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 30, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_free (void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_free);

    assert (mesg);

    H5FL_FREE(H5O_stab_t,mesg);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 20, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_delete(H5F_t *f, hid_t dxpl_id, const void *mesg, hbool_t adj_link)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_delete)

    /* check args */
    assert(f);
    assert(mesg);

    /* Free the file space for the symbol table */
    if (H5G_stab_delete(f, dxpl_id, mesg, adj_link)<0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free symbol table")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_stab_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_debug
 *
 * Purpose:     Prints debugging info for a symbol table message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg, FILE * stream,
         int indent, int fwidth)
{
    const H5O_stab_t       *stab = (const H5O_stab_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_debug);

    /* check args */
    assert(f);
    assert(stab);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
        "B-tree address:", stab->btree_addr);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
        "Name heap address:", stab->heap_addr);

    FUNC_LEAVE_NOAPI(SUCCEED);
}
