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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *    Wednesday, April  1, 1998
 *
 * Purpose:  Functions that operate on a shared message.  The shared
 *    message doesn't ever actually appear in the object header as
 *    a normal message.  Instead, if a message is shared, the
 *    H5O_FLAG_SHARED bit is set and the message body is that
 *    defined here for H5O_SHARED.  The message ID is the ID of the
 *    pointed-to message and the pointed-to message is stored in
 *    the global heap.
 */

#define H5F_PACKAGE  /*suppress error about including H5Fpkg    */
#define H5O_PACKAGE  /*suppress error about including H5Opkg    */


#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fpkg.h"             /* File access        */
#include "H5Gprivate.h"    /* Groups        */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Opkg.h"             /* Object headers      */

static void *H5O_shared_decode (H5F_t*, hid_t dxpl_id, const uint8_t*, H5O_shared_t *sh);
static herr_t H5O_shared_encode (H5F_t*, uint8_t*, const void*);
static void *H5O_shared_copy(const void *_mesg, void *_dest, unsigned update_flags);
static size_t H5O_shared_size (const H5F_t*, const void *_mesg);
static herr_t H5O_shared_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg, hbool_t adj_link);
static herr_t H5O_shared_link(H5F_t *f, hid_t dxpl_id, const void *_mesg);
static herr_t H5O_shared_debug (H5F_t*, hid_t dxpl_id, const void*, FILE*, int, int);

/* This message derives from H5O */
const H5O_class_t H5O_SHARED[1] = {{
    H5O_SHARED_ID,        /*message id number      */
    "shared",          /*message name for debugging    */
    sizeof(H5O_shared_t),   /*native message size      */
    H5O_shared_decode,        /*decode method        */
    H5O_shared_encode,        /*encode method        */
    H5O_shared_copy,    /*copy the native value      */
    H5O_shared_size,        /*size method        */
    NULL,          /*no reset method      */
    NULL,            /*no free method      */
    H5O_shared_delete,    /*file delete method      */
    H5O_shared_link,    /*link method        */
    NULL,      /*get share method      */
    NULL,       /*set share method      */
    H5O_shared_debug,        /*debug method        */
}};

/* Old version, with full symbol table entry as link for object header sharing */
#define H5O_SHARED_VERSION_1  1

/* New version, with just address of object as link for object header sharing */
#define H5O_SHARED_VERSION  2


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_read
 *
 * Purpose:  Reads a message referred to by a shared message.
 *
 * Return:  Success:  Ptr to message in native format.  The message
 *        should be freed by calling H5O_reset().  If
 *        MESG is a null pointer then the caller should
 *        also call H5MM_xfree() on the return value
 *        after calling H5O_reset().
 *
 *    Failure:  NULL
 *
 * Programmer:  Quincey Koziol
 *    koziol@ncsa.uiuc.edu
 *    Sep 24 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_shared_read(H5F_t *f, hid_t dxpl_id, H5O_shared_t *shared, const H5O_class_t *type, void *mesg)
{
    void *ret_value = NULL;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_shared_read);

    /* check args */
    assert(f);
    assert(shared);
    assert(type);

    /* Get the shared message */
    if (shared->in_gh) {
        void *tmp_buf, *tmp_mesg;

        if (NULL==(tmp_buf = H5HG_read (f, dxpl_id, &(shared->u.gh), NULL)))
            HGOTO_ERROR (H5E_OHDR, H5E_CANTLOAD, NULL, "unable to read shared message from global heap");
        tmp_mesg = (type->decode)(f, dxpl_id, tmp_buf, shared);
        tmp_buf = H5MM_xfree (tmp_buf);
        if (!tmp_mesg)
            HGOTO_ERROR (H5E_OHDR, H5E_CANTLOAD, NULL, "unable to decode object header shared message");
        if (mesg) {
            HDmemcpy (mesg, tmp_mesg, type->native_size);
            H5MM_xfree (tmp_mesg);
        } /* end if */
        else
            ret_value = tmp_mesg;
    } /* end if */
    else {
        ret_value = H5O_read_real(&(shared->u.ent), type, 0, mesg, dxpl_id);
        if (type->set_share &&
                (type->set_share)(f, ret_value, shared)<0)
            HGOTO_ERROR (H5E_OHDR, H5E_CANTINIT, NULL, "unable to set sharing information");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_shared_read() */


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_link_adj
 *
 * Purpose:  Changes the link count for the object referenced by a shared
 *              message.
 *
 * Return:  Success:  New link count
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *    koziol@ncsa.uiuc.edu
 *    Sep 26 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5O_shared_link_adj(H5F_t *f, hid_t dxpl_id, const H5O_shared_t *shared, int adjust)
{
    int ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_shared_link_adj);

    /* check args */
    assert(f);
    assert(shared);

    if (shared->in_gh) {
        /*
         * The shared message is stored in the global heap.
         * Adjust the reference count on the global heap message.
         */
        if ((ret_value = H5HG_link (f, dxpl_id, &(shared->u.gh), adjust))<0)
            HGOTO_ERROR (H5E_OHDR, H5E_LINK, FAIL, "unable to adjust shared object link count");
    } /* end if */
    else {
        /*
         * The shared message is stored in some other object header.
         * The other object header must be in the same file as the
         * new object header. Adjust the reference count on that
         * object header.
         */
        if (shared->u.ent.file->shared != f->shared)
            HGOTO_ERROR(H5E_OHDR, H5E_LINK, FAIL, "interfile hard links are not allowed");
        if ((ret_value = H5O_link (&(shared->u.ent), adjust, dxpl_id))<0)
            HGOTO_ERROR (H5E_OHDR, H5E_LINK, FAIL, "unable to adjust shared object link count");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_shared_link_adj() */


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_decode
 *
 * Purpose:  Decodes a shared object message and returns it.
 *
 * Return:  Success:  Ptr to a new shared object message.
 *
 *    Failure:  NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *  Robb Matzke, 1998-07-20
 *  Added a version number to the beginning of the message.
 *-------------------------------------------------------------------------
 */
static void *
H5O_shared_decode (H5F_t *f, hid_t UNUSED dxpl_id, const uint8_t *buf, H5O_shared_t UNUSED *sh)
{
    H5O_shared_t  *mesg=NULL;
    unsigned    flags, version;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_shared_decode);

    /* Check args */
    assert (f);
    assert (buf);
    assert (!sh);

    /* Decode */
    if (NULL==(mesg = H5MM_calloc (sizeof(H5O_shared_t))))
  HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Version */
    version = *buf++;
    if (version!=H5O_SHARED_VERSION_1 && version!=H5O_SHARED_VERSION)
  HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for shared object message");

    /* Get the shared information flags */
    flags = *buf++;
    mesg->in_gh = (flags & 0x01);

    /* Skip reserved bytes (for version 1) */
    if(version==H5O_SHARED_VERSION_1)
        buf += 6;

    /* Body */
    if (mesg->in_gh) {
        H5F_addr_decode (f, &buf, &(mesg->u.gh.addr));
        INT32DECODE (buf, mesg->u.gh.idx);
    } /* end if */
    else {
        if(version==H5O_SHARED_VERSION_1)
            H5G_ent_decode (f, &buf, &(mesg->u.ent));
        else {
            assert(version==H5O_SHARED_VERSION);
            H5F_addr_decode (f, &buf, &(mesg->u.ent.header));
            mesg->u.ent.file=f;
        } /* end else */
    } /* end else */

    /* Set return value */
    ret_value=mesg;

done:
    if(ret_value==NULL) {
        if(mesg!=NULL)
            H5MM_xfree(mesg);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_encode
 *
 * Purpose:  Encodes message _MESG into buffer BUF.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *  Robb Matzke, 1998-07-20
 *  Added a version number to the beginning of the message.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_shared_encode (H5F_t *f, uint8_t *buf/*out*/, const void *_mesg)
{
    const H5O_shared_t  *mesg = (const H5O_shared_t *)_mesg;
    unsigned    flags;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_shared_encode);

    /* Check args */
    assert (f);
    assert (buf);
    assert (mesg);

    /* Encode */
    *buf++ = H5O_SHARED_VERSION;
    flags = mesg->in_gh ? 0x01 : 0x00;
    *buf++ = flags;
#ifdef OLD_WAY
    *buf++ = 0; /*reserved 1*/
    *buf++ = 0; /*reserved 2*/
    *buf++ = 0; /*reserved 3*/
    *buf++ = 0; /*reserved 4*/
    *buf++ = 0; /*reserved 5*/
    *buf++ = 0; /*reserved 6*/

    if (mesg->in_gh) {
  H5F_addr_encode (f, &buf, mesg->u.gh.addr);
  INT32ENCODE (buf, mesg->u.gh.idx);
    } /* end if */
    else
  H5G_ent_encode (f, &buf, &(mesg->u.ent));
#else /* OLD_WAY */
    if (mesg->in_gh) {
  H5F_addr_encode (f, &buf, mesg->u.gh.addr);
  INT32ENCODE (buf, mesg->u.gh.idx);
    } /* end if */
    else
        H5F_addr_encode (f, &buf, mesg->u.ent.header);
#endif /* OLD_WAY */

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_copy
 *
 * Purpose:  Copies a message from _MESG to _DEST, allocating _DEST if
 *    necessary.
 *
 * Return:  Success:  Ptr to _DEST
 *
 *    Failure:  NULL
 *
 * Programmer:  Quincey Koziol
 *    koziol@ncsa.uiuc.edu
 *    Sep 26 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_shared_copy(const void *_mesg, void *_dest, unsigned UNUSED update_flags)
{
    const H5O_shared_t  *mesg = (const H5O_shared_t *) _mesg;
    H5O_shared_t  *dest = (H5O_shared_t *) _dest;
    void        *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_shared_copy);

    /* check args */
    assert(mesg);
    if (!dest && NULL==(dest = H5MM_malloc (sizeof(H5O_shared_t))))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* copy */
    *dest = *mesg;

    /* Set return value */
    ret_value=dest;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_shared_copy() */


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_size
 *
 * Purpose:  Returns the length of a shared object message.
 *
 * Return:  Success:  Length
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_shared_size (const H5F_t *f, const void *_mesg)
{
    const H5O_shared_t  *shared = (const H5O_shared_t *) _mesg;
    size_t  ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_shared_size);

    ret_value = 1 +      /*version      */
            1 +        /*the flags field    */
            (shared->in_gh ?
               (H5F_SIZEOF_ADDR(f)+4) :  /*sharing via global heap  */
    H5F_SIZEOF_ADDR(f));  /*sharing by another obj hdr  */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_shared_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 26, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_shared_delete(H5F_t *f, hid_t dxpl_id, const void *_mesg, hbool_t adj_link)
{
    const H5O_shared_t       *shared = (const H5O_shared_t *) _mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_shared_delete);

    /* check args */
    assert(f);
    assert(shared);

    /* Decrement the reference count on the shared object, if requested */
    if(adj_link)
        if(H5O_shared_link_adj(f, dxpl_id, shared, -1)<0)
            HGOTO_ERROR (H5E_OHDR, H5E_LINK, FAIL, "unable to adjust shared object link count")

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_shared_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O_shared_link
 *
 * Purpose:     Increment reference count on any objects referenced by
 *              message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 26, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_shared_link(H5F_t *f, hid_t dxpl_id, const void *_mesg)
{
    const H5O_shared_t       *shared = (const H5O_shared_t *) _mesg;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_shared_link);

    /* check args */
    assert(f);
    assert(shared);

    /* Decrement the reference count on the shared object */
    if(H5O_shared_link_adj(f,dxpl_id,shared,1)<0)
        HGOTO_ERROR (H5E_OHDR, H5E_LINK, FAIL, "unable to adjust shared object link count");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_shared_link() */


/*-------------------------------------------------------------------------
 * Function:  H5O_shared_debug
 *
 * Purpose:  Prints debugging info for the message
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_shared_debug (H5F_t UNUSED *f, hid_t dxpl_id, const void *_mesg,
      FILE *stream, int indent, int fwidth)
{
    const H5O_shared_t  *mesg = (const H5O_shared_t *)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_shared_debug);

    /* Check args */
    assert (f);
    assert (mesg);
    assert (stream);
    assert (indent>=0);
    assert (fwidth>=0);

    if (mesg->in_gh) {
  HDfprintf (stream, "%*s%-*s %s\n", indent, "", fwidth,
       "Sharing method",
       "Global heap");
  HDfprintf (stream, "%*s%-*s %a\n", indent, "", fwidth,
       "Collection address:",
       mesg->u.gh.addr);
  HDfprintf (stream, "%*s%-*s %d\n", indent, "", fwidth,
       "Object ID within collection:",
       mesg->u.gh.idx);
    } else {
  HDfprintf (stream, "%*s%-*s %s\n", indent, "", fwidth,
       "Sharing method",
       "Obj Hdr");
  H5G_ent_debug (f, dxpl_id, &(mesg->u.ent), stream, indent, fwidth,
           HADDR_UNDEF);
    }

    FUNC_LEAVE_NOAPI(SUCCEED);
}
