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
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *         Tuesday, November 25, 1997
 */

#define H5O_PACKAGE    /*suppress error about including H5Opkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fprivate.h"    /* File access        */
#include "H5HLprivate.h"  /* Local Heaps        */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Opkg.h"             /* Object headers      */

/* PRIVATE PROTOTYPES */
static void *H5O_efl_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t *sh);
static herr_t H5O_efl_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_efl_copy(const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_efl_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_efl_reset(void *_mesg);
static herr_t H5O_efl_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE * stream,
          int indent, int fwidth);

/* This message derives from H5O */
const H5O_class_t H5O_EFL[1] = {{
    H5O_EFL_ID,          /*message id number    */
    "external file list",     /*message name for debugging    */
    sizeof(H5O_efl_t),        /*native message size        */
    H5O_efl_decode,        /*decode message    */
    H5O_efl_encode,        /*encode message    */
    H5O_efl_copy,        /*copy native value    */
    H5O_efl_size,        /*size of message on disk  */
    H5O_efl_reset,        /*reset method          */
    NULL,                /* free method      */
    NULL,            /* file delete method    */
    NULL,      /* link method      */
    NULL,            /*get share method    */
    NULL,      /*set share method    */
    H5O_efl_debug,        /*debug the message    */
}};

#define H5O_EFL_VERSION    1


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_decode
 *
 * Purpose:  Decode an external file list message and return a pointer to
 *    the message (and some other data).
 *
 * Return:  Success:  Ptr to a new message struct.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *    Tuesday, November 25, 1997
 *
 * Modifications:
 *  Robb Matzke, 1998-07-20
 *  Rearranged the message to add a version number near the beginning.
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_efl_decode(H5F_t *f, hid_t dxpl_id, const uint8_t *p, H5O_shared_t UNUSED *sh)
{
    H5O_efl_t    *mesg = NULL;
    int      version;
    const char    *s = NULL;
    const H5HL_t        *heap;
    size_t    u;      /* Local index variable */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_efl_decode);

    /* Check args */
    assert(f);
    assert(p);
    assert (!sh);

    if (NULL==(mesg = H5MM_calloc(sizeof(H5O_efl_t))))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Version */
    version = *p++;
    if (version!=H5O_EFL_VERSION)
  HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for external file list message");

    /* Reserved */
    p += 3;

    /* Number of slots */
    UINT16DECODE(p, mesg->nalloc);
    assert(mesg->nalloc>0);
    UINT16DECODE(p, mesg->nused);
    assert(mesg->nused <= mesg->nalloc);

    /* Heap address */
    H5F_addr_decode(f, &p, &(mesg->heap_addr));

#ifndef NDEBUG
    assert (H5F_addr_defined(mesg->heap_addr));

    if (NULL == (heap = H5HL_protect(f, dxpl_id, mesg->heap_addr)))
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read protect link value")

    s = H5HL_offset_into(f, heap, 0);

    assert (s && !*s);

    if (H5HL_unprotect(f, dxpl_id, heap, mesg->heap_addr) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read unprotect link value")
#endif

    /* Decode the file list */
    mesg->slot = H5MM_calloc(mesg->nalloc*sizeof(H5O_efl_entry_t));
    if (NULL==mesg->slot)
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    for (u=0; u<mesg->nused; u++) {
  /* Name */
  H5F_DECODE_LENGTH (f, p, mesg->slot[u].name_offset);

        if (NULL == (heap = H5HL_protect(f, dxpl_id, mesg->heap_addr)))
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read protect link value")

        s = H5HL_offset_into(f, heap, mesg->slot[u].name_offset);
  assert (s && *s);
  mesg->slot[u].name = H5MM_xstrdup (s);
        assert(mesg->slot[u].name);

        if (H5HL_unprotect(f, dxpl_id, heap, mesg->heap_addr) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read unprotect link value")

  /* File offset */
  H5F_DECODE_LENGTH (f, p, mesg->slot[u].offset);

  /* Size */
  H5F_DECODE_LENGTH (f, p, mesg->slot[u].size);
  assert (mesg->slot[u].size>0);
    }

    /* Set return value */
    ret_value=mesg;

done:
    if(ret_value==NULL) {
        if(mesg!=NULL)
            H5MM_xfree (mesg);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_encode
 *
 * Purpose:  Encodes a message.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Tuesday, November 25, 1997
 *
 * Modifications:
 *  Robb Matzke, 1998-07-20
 *  Rearranged the message to add a version number near the beginning.
 *
 *   Robb Matzke, 1999-10-14
 *  Entering the name into the local heap happens when the dataset is
 *  created. Otherwise we could end up in infinite recursion if the heap
 *  happens to hash to the same cache slot as the object header.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_efl_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5O_efl_t  *mesg = (const H5O_efl_t *)_mesg;
    size_t    u;      /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_efl_encode);

    /* check args */
    assert(f);
    assert(mesg);
    assert(p);

    /* Version */
    *p++ = H5O_EFL_VERSION;

    /* Reserved */
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    /* Number of slots */
    assert (mesg->nalloc>0);
    UINT16ENCODE(p, mesg->nused); /*yes, twice*/
    assert (mesg->nused>0 && mesg->nused<=mesg->nalloc);
    UINT16ENCODE(p, mesg->nused);

    /* Heap address */
    assert (H5F_addr_defined(mesg->heap_addr));
    H5F_addr_encode(f, &p, mesg->heap_addr);

    /* Encode file list */
    for (u=0; u<mesg->nused; u++) {
  /*
   * The name should have been added to the heap when the dataset was
   * created.
   */
  assert(mesg->slot[u].name_offset);
  H5F_ENCODE_LENGTH (f, p, mesg->slot[u].name_offset);
  H5F_ENCODE_LENGTH (f, p, mesg->slot[u].offset);
  H5F_ENCODE_LENGTH (f, p, mesg->slot[u].size);
    }

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_copy
 *
 * Purpose:  Copies a message from _MESG to _DEST, allocating _DEST if
 *    necessary.
 *
 * Return:  Success:  Ptr to _DEST
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *    Tuesday, November 25, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_efl_copy(const void *_mesg, void *_dest, unsigned UNUSED update_flags)
{
    const H5O_efl_t  *mesg = (const H5O_efl_t *) _mesg;
    H5O_efl_t    *dest = (H5O_efl_t *) _dest;
    size_t    u;              /* Local index variable */
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_efl_copy);

    /* check args */
    assert(mesg);
    if (!dest) {
  if (NULL==(dest = H5MM_calloc(sizeof(H5O_efl_t))) ||
                NULL==(dest->slot=H5MM_malloc(mesg->nalloc* sizeof(H5O_efl_entry_t))))
      HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    } else if (dest->nalloc<mesg->nalloc) {
  H5MM_xfree(dest->slot);
  if (NULL==(dest->slot = H5MM_malloc(mesg->nalloc*
              sizeof(H5O_efl_entry_t))))
      HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    }
    dest->heap_addr = mesg->heap_addr;
    dest->nalloc = mesg->nalloc;
    dest->nused = mesg->nused;

    for (u = 0; u < mesg->nused; u++) {
  dest->slot[u] = mesg->slot[u];
  dest->slot[u].name = H5MM_xstrdup (mesg->slot[u].name);
    }

    /* Set return value */
    ret_value=dest;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_size
 *
 * Purpose:  Returns the size of the raw message in bytes not counting the
 *    message type or size fields, but only the data fields.  This
 *    function doesn't take into account message alignment. This
 *    function doesn't count unused slots.
 *
 * Return:  Success:  Message data size in bytes.
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *    Tuesday, November 25, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_efl_size(const H5F_t *f, const void *_mesg)
{
    const H5O_efl_t  *mesg = (const H5O_efl_t *) _mesg;
    size_t    ret_value = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_efl_size);

    /* check args */
    assert(f);
    assert(mesg);

    ret_value = H5F_SIZEOF_ADDR(f) +      /*heap address  */
    2 +          /*slots allocated*/
    2 +          /*num slots used*/
    4 +          /*reserved  */
    mesg->nused * (H5F_SIZEOF_SIZE(f) +  /*name offset  */
             H5F_SIZEOF_SIZE(f) +  /*file offset  */
             H5F_SIZEOF_SIZE(f));  /*file size  */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_reset
 *
 * Purpose:  Frees internal pointers and resets the message to an
 *    initialial state.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Tuesday, November 25, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_efl_reset(void *_mesg)
{
    H5O_efl_t  *mesg = (H5O_efl_t *) _mesg;
    size_t  u;              /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_efl_reset);

    /* check args */
    assert(mesg);

    /* reset */
    for (u=0; u<mesg->nused; u++)
  mesg->slot[u].name = H5MM_xfree (mesg->slot[u].name);
    mesg->heap_addr = HADDR_UNDEF;
    mesg->nused = mesg->nalloc = 0;
    if(mesg->slot)
        mesg->slot = H5MM_xfree(mesg->slot);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_total_size
 *
 * Purpose:  Return the total size of the external file list by summing
 *    the sizes of all of the files.
 *
 * Return:  Success:  Total reserved size for external data.
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March  3, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5O_efl_total_size (H5O_efl_t *efl)
{
    hsize_t  ret_value = 0, tmp;

    FUNC_ENTER_NOAPI_NOINIT(H5O_efl_total_size);

    if (efl->nused>0 &&
  H5O_EFL_UNLIMITED==efl->slot[efl->nused-1].size) {
  ret_value = H5O_EFL_UNLIMITED;
    } else {
        size_t    u;      /* Local index variable */

  for (u=0; u<efl->nused; u++, ret_value=tmp) {
      tmp = ret_value + efl->slot[u].size;
      if (tmp<=ret_value)
    HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, 0, "total external storage size overflowed");
  }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_efl_debug
 *
 * Purpose:  Prints debugging info for a message.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *    Tuesday, November 25, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_efl_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg, FILE * stream,
        int indent, int fwidth)
{
    const H5O_efl_t     *mesg = (const H5O_efl_t *) _mesg;
    char        buf[64];
    size_t        u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_efl_debug);

    /* check args */
    assert(f);
    assert(mesg);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
        "Heap address:", mesg->heap_addr);

    HDfprintf(stream, "%*s%-*s %u/%u\n", indent, "", fwidth,
        "Slots used/allocated:",
        mesg->nused, mesg->nalloc);

    for (u = 0; u < mesg->nused; u++) {
  sprintf (buf, "File %u", (unsigned)u);
  HDfprintf (stream, "%*s%s:\n", indent, "", buf);

  HDfprintf(stream, "%*s%-*s \"%s\"\n", indent+3, "", MAX (fwidth-3, 0),
      "Name:",
      mesg->slot[u].name);

  HDfprintf(stream, "%*s%-*s %lu\n", indent+3, "", MAX (fwidth-3, 0),
      "Name offset:",
      (unsigned long)(mesg->slot[u].name_offset));

  HDfprintf (stream, "%*s%-*s %lu\n", indent+3, "", MAX (fwidth-3, 0),
       "Offset of data in file:",
       (unsigned long)(mesg->slot[u].offset));

  HDfprintf (stream, "%*s%-*s %lu\n", indent+3, "", MAX (fwidth-3, 0),
       "Bytes reserved for data:",
       (unsigned long)(mesg->slot[u].size));
    }

    FUNC_LEAVE_NOAPI(SUCCEED);
}
