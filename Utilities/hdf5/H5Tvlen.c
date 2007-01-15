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
 * Module Info: This module contains the functionality for variable-length
 *      datatypes in the H5T interface.
 */

#define H5T_PACKAGE    /*suppress error about including H5Tpkg       */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5T_init_vlen_interface


#include "H5private.h"    /* Generic Functions      */
#include "H5Dprivate.h"    /* Dataset functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5FLprivate.h"  /* Free Lists                           */
#include "H5HGprivate.h"  /* Global Heaps        */
#include "H5Iprivate.h"    /* IDs            */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Pprivate.h"    /* Property lists      */
#include "H5Tpkg.h"    /* Datatypes        */

/* Local functions */
static htri_t H5T_vlen_set_loc(const H5T_t *dt, H5F_t *f, H5T_vlen_loc_t loc);
static herr_t H5T_vlen_reclaim_recurse(void *elem, const H5T_t *dt, H5MM_free_t free_func, void *free_info);
static ssize_t H5T_vlen_seq_mem_getlen(const void *_vl);
static void * H5T_vlen_seq_mem_getptr(void *_vl);
static htri_t H5T_vlen_seq_mem_isnull(const H5F_t *f, void *_vl);
static herr_t H5T_vlen_seq_mem_read(H5F_t *f, hid_t dxpl_id, void *_vl, void *_buf, size_t len);
static herr_t H5T_vlen_seq_mem_write(H5F_t *f, hid_t dxpl_id, const H5T_vlen_alloc_info_t *vl_alloc_info, void *_vl, void *_buf, void *_bg, size_t seq_len, size_t base_size);
static herr_t H5T_vlen_seq_mem_setnull(H5F_t *f, hid_t dxpl_id, void *_vl, void *_bg);
static ssize_t H5T_vlen_str_mem_getlen(const void *_vl);
static void * H5T_vlen_str_mem_getptr(void *_vl);
static htri_t H5T_vlen_str_mem_isnull(const H5F_t *f, void *_vl);
static herr_t H5T_vlen_str_mem_read(H5F_t *f, hid_t dxpl_id, void *_vl, void *_buf, size_t len);
static herr_t H5T_vlen_str_mem_write(H5F_t *f, hid_t dxpl_id, const H5T_vlen_alloc_info_t *vl_alloc_info, void *_vl, void *_buf, void *_bg, size_t seq_len, size_t base_size);
static herr_t H5T_vlen_str_mem_setnull(H5F_t *f, hid_t dxpl_id, void *_vl, void *_bg);
static ssize_t H5T_vlen_disk_getlen(const void *_vl);
static void * H5T_vlen_disk_getptr(void *_vl);
static htri_t H5T_vlen_disk_isnull(const H5F_t *f, void *_vl);
static herr_t H5T_vlen_disk_read(H5F_t *f, hid_t dxpl_id, void *_vl, void *_buf, size_t len);
static herr_t H5T_vlen_disk_write(H5F_t *f, hid_t dxpl_id, const H5T_vlen_alloc_info_t *vl_alloc_info, void *_vl, void *_buf, void *_bg, size_t seq_len, size_t base_size);
static herr_t H5T_vlen_disk_setnull(H5F_t *f, hid_t dxpl_id, void *_vl, void *_bg);

/* Local variables */

/* Default settings for variable-length allocation routines */
static H5T_vlen_alloc_info_t H5T_vlen_def_vl_alloc_info ={
    H5D_XFER_VLEN_ALLOC_DEF,
    H5D_XFER_VLEN_ALLOC_INFO_DEF,
    H5D_XFER_VLEN_FREE_DEF,
    H5D_XFER_VLEN_FREE_INFO_DEF
};

/* Declare extern the free lists for H5T_t's and H5T_shared_t's */
H5FL_EXTERN(H5T_t);
H5FL_EXTERN(H5T_shared_t);


/*--------------------------------------------------------------------------
NAME
   H5T_init_vlen_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_vlen_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_vlen_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_vlen_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_vlen_interface() */


/*-------------------------------------------------------------------------
 * Function:  H5Tvlen_create
 *
 * Purpose:  Create a new variable-length datatype based on the specified
 *    BASE_TYPE.
 *
 * Return:  Success:  ID of new VL datatype
 *
 *    Failure:  Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, May 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tvlen_create(hid_t base_id)
{
    H5T_t  *base = NULL;    /*base datatype  */
    H5T_t  *dt = NULL;    /*new datatype  */
    hid_t  ret_value;          /*return value      */

    FUNC_ENTER_API(H5Tvlen_create, FAIL)
    H5TRACE1("i","i",base_id);

    /* Check args */
    if (NULL==(base=H5I_object_verify(base_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype")

    /* Create up VL datatype */
    if ((dt=H5T_vlen_create(base))==NULL)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location")

    /* Atomize the type */
    if ((ret_value=H5I_register(H5I_DATATYPE, dt))<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register datatype")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_create
 *
 * Purpose:  Create a new variable-length datatype based on the specified
 *    BASE_TYPE.
 *
 * Return:  Success:  new VL datatype
 *
 *    Failure:  NULL
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, November 20, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_vlen_create(const H5T_t *base)
{
    H5T_t  *dt = NULL;    /*new VL datatype  */
    H5T_t  *ret_value;  /*return value      */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_create)

    /* Check args */
    assert(base);

    /* Build new type */
    if(NULL == (dt = H5T_alloc()))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    dt->shared->type = H5T_VLEN;

    /*
     * Force conversions (i.e. memory to memory conversions should duplicate
     * data, not point to the same VL sequences)
     */
    dt->shared->force_conv = TRUE;
    dt->shared->parent = H5T_copy(base, H5T_COPY_ALL);

    /* This is a sequence, not a string */
    dt->shared->u.vlen.type = H5T_VLEN_SEQUENCE;

    /* Set up VL information */
    if (H5T_vlen_mark(dt, NULL, H5T_VLEN_MEMORY)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "invalid VL location")

    /* Set return value */
    ret_value=dt;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function: H5T_vlen_set_loc
 *
 * Purpose:  Sets the location of a VL datatype to be either on disk or in memory
 *
 * Return:
 *  One of two values on success:
 *      TRUE - If the location of any vlen types changed
 *      FALSE - If the location of any vlen types is the same
 *  <0 is returned on failure
 *
 * Programmer:  Quincey Koziol
 *    Friday, June 4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5T_vlen_set_loc(const H5T_t *dt, H5F_t *f, H5T_vlen_loc_t loc)
{
    htri_t ret_value = 0;       /* Indicate that success, but no location change */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_set_loc)

    /* check parameters */
    assert(dt);
    assert(loc>H5T_VLEN_BADLOC && loc<H5T_VLEN_MAXLOC);

    /* Only change the location if it's different */
    if(loc!=dt->shared->u.vlen.loc) {
        /* Indicate that the location changed */
        ret_value=TRUE;

        switch(loc) {
            case H5T_VLEN_MEMORY:   /* Memory based VL datatype */
                assert(f==NULL);

                /* Mark this type as being stored in memory */
                dt->shared->u.vlen.loc=H5T_VLEN_MEMORY;

                if(dt->shared->u.vlen.type==H5T_VLEN_SEQUENCE) {
                    /* size in memory, disk size is different */
                    dt->shared->size = sizeof(hvl_t);

                    /* Set up the function pointers to access the VL sequence in memory */
                    dt->shared->u.vlen.getlen=H5T_vlen_seq_mem_getlen;
                    dt->shared->u.vlen.getptr=H5T_vlen_seq_mem_getptr;
                    dt->shared->u.vlen.isnull=H5T_vlen_seq_mem_isnull;
                    dt->shared->u.vlen.read=H5T_vlen_seq_mem_read;
                    dt->shared->u.vlen.write=H5T_vlen_seq_mem_write;
                    dt->shared->u.vlen.setnull=H5T_vlen_seq_mem_setnull;
                } else if(dt->shared->u.vlen.type==H5T_VLEN_STRING) {
                    /* size in memory, disk size is different */
                    dt->shared->size = sizeof(char *);

                    /* Set up the function pointers to access the VL string in memory */
                    dt->shared->u.vlen.getlen=H5T_vlen_str_mem_getlen;
                    dt->shared->u.vlen.getptr=H5T_vlen_str_mem_getptr;
                    dt->shared->u.vlen.isnull=H5T_vlen_str_mem_isnull;
                    dt->shared->u.vlen.read=H5T_vlen_str_mem_read;
                    dt->shared->u.vlen.write=H5T_vlen_str_mem_write;
                    dt->shared->u.vlen.setnull=H5T_vlen_str_mem_setnull;
                } else {
                    assert(0 && "Invalid VL type");
                }

                /* Reset file ID (since this VL is in memory) */
                dt->shared->u.vlen.f=NULL;
                break;

            case H5T_VLEN_DISK:   /* Disk based VL datatype */
                assert(f);

                /* Mark this type as being stored on disk */
                dt->shared->u.vlen.loc=H5T_VLEN_DISK;

                /*
                 * Size of element on disk is 4 bytes for the length, plus the size
                 * of an address in this file, plus 4 bytes for the size of a heap
                 * ID.  Memory size is different
                 */
                dt->shared->size = 4 + H5F_SIZEOF_ADDR(f) + 4;

                /* Set up the function pointers to access the VL information on disk */
                /* VL sequences and VL strings are stored identically on disk, so use the same functions */
                dt->shared->u.vlen.getlen=H5T_vlen_disk_getlen;
                dt->shared->u.vlen.getptr=H5T_vlen_disk_getptr;
                dt->shared->u.vlen.isnull=H5T_vlen_disk_isnull;
                dt->shared->u.vlen.read=H5T_vlen_disk_read;
                dt->shared->u.vlen.write=H5T_vlen_disk_write;
                dt->shared->u.vlen.setnull=H5T_vlen_disk_setnull;

                /* Set file ID (since this VL is on disk) */
                dt->shared->u.vlen.f=f;
                break;

            default:
                HGOTO_ERROR (H5E_DATATYPE, H5E_BADRANGE, FAIL, "invalid VL datatype location")
        } /* end switch */ /*lint !e788 All appropriate cases are covered */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_set_loc() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_seq_mem_getlen
 *
 * Purpose:  Retrieves the length of a memory based VL element.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5T_vlen_seq_mem_getlen(const void *_vl)
{
    const hvl_t *vl=(const hvl_t *)_vl;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_seq_mem_getlen)

    /* check parameters */
    assert(vl);

    FUNC_LEAVE_NOAPI((ssize_t)vl->len)
}   /* end H5T_vlen_seq_mem_getlen() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_seq_mem_getptr
 *
 * Purpose:  Retrieves the pointer for a memory based VL element.
 *
 * Return:  Non-NULL on success/NULL on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, June 12, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5T_vlen_seq_mem_getptr(void *_vl)
{
    hvl_t *vl=(hvl_t *)_vl;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_seq_mem_getptr)

    /* check parameters */
    assert(vl);

    FUNC_LEAVE_NOAPI(vl->p)
}   /* end H5T_vlen_seq_mem_getptr() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_seq_mem_isnull
 *
 * Purpose:  Checks if a memory sequence is the "null" sequence
 *
 * Return:  TRUE/FALSE on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, November 8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static htri_t
H5T_vlen_seq_mem_isnull(const H5F_t UNUSED *f, void *_vl)
{
    hvl_t *vl=(hvl_t *)_vl;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_seq_mem_isnull)

    /* check parameters */
    assert(vl);

    FUNC_LEAVE_NOAPI((vl->len==0 || vl->p==NULL) ? TRUE : FALSE)
}   /* end H5T_vlen_seq_mem_isnull() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_seq_mem_read
 *
 * Purpose:  "Reads" the memory based VL sequence into a buffer
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_seq_mem_read(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_vl, void *buf, size_t len)
{
    hvl_t *vl=(hvl_t *)_vl;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_seq_mem_read)

    /* check parameters */
    assert(vl && vl->p);
    assert(buf);

    HDmemcpy(buf,vl->p,len);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* end H5T_vlen_seq_mem_read() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_seq_mem_write
 *
 * Purpose:  "Writes" the memory based VL sequence from a buffer
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_seq_mem_write(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const H5T_vlen_alloc_info_t *vl_alloc_info, void *_vl, void *buf, void UNUSED *_bg, size_t seq_len, size_t base_size)
{
    hvl_t vl;                       /* Temporary hvl_t to use during operation */
    size_t len;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_seq_mem_write)

    /* check parameters */
    assert(_vl);
    assert(buf);

    if(seq_len!=0) {
        len=seq_len*base_size;

        /* Use the user's memory allocation routine is one is defined */
        if(vl_alloc_info->alloc_func!=NULL) {
            if(NULL==(vl.p=(vl_alloc_info->alloc_func)(len,vl_alloc_info->alloc_info)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data")
          } /* end if */
        else {  /* Default to system malloc */
            if(NULL==(vl.p=H5MM_malloc(len)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data")
          } /* end else */

        /* Copy the data into the newly allocated buffer */
        HDmemcpy(vl.p,buf,len);

    } /* end if */
    else
        vl.p=NULL;

    /* Set the sequence length */
    vl.len=seq_len;

    /* Set pointer in user's buffer with memcpy, to avoid alignment issues */
    HDmemcpy(_vl,&vl,sizeof(hvl_t));

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_seq_mem_write() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_seq_mem_setnull
 *
 * Purpose:  Sets a VL info object in memory to the "nil" value
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, November 8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_seq_mem_setnull(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_vl, void UNUSED *_bg)
{
    hvl_t vl;                       /* Temporary hvl_t to use during operation */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_seq_mem_setnull)

    /* check parameters */
    assert(_vl);

    /* Set the "nil" hvl_t */
    vl.len=0;
    vl.p=NULL;

    /* Set pointer in user's buffer with memcpy, to avoid alignment issues */
    HDmemcpy(_vl,&vl,sizeof(hvl_t));

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* end H5T_vlen_seq_mem_setnull() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_str_mem_getlen
 *
 * Purpose:  Retrieves the length of a memory based VL string.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5T_vlen_str_mem_getlen(const void *_vl)
{
    const char *s=*(const char * const *)_vl;   /* Pointer to the user's string information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_str_mem_getlen)

    /* check parameters */
    assert(s);

    FUNC_LEAVE_NOAPI((ssize_t)HDstrlen(s))
}   /* end H5T_vlen_str_mem_getlen() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_str_mem_getptr
 *
 * Purpose:  Retrieves the pointer for a memory based VL string.
 *
 * Return:  Non-NULL on success/NULL on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, June 12, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5T_vlen_str_mem_getptr(void *_vl)
{
    char *s=*(char **)_vl;   /* Pointer to the user's string information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_str_mem_getptr)

    /* check parameters */
    assert(s);

    FUNC_LEAVE_NOAPI(s)
}   /* end H5T_vlen_str_mem_getptr() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_str_mem_isnull
 *
 * Purpose:  Checks if a memory string is a NULL pointer
 *
 * Return:  TRUE/FALSE on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, November 8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static htri_t
H5T_vlen_str_mem_isnull(const H5F_t UNUSED *f, void *_vl)
{
    char *s=*(char **)_vl;   /* Pointer to the user's string information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_str_mem_isnull)

    FUNC_LEAVE_NOAPI(s==NULL ? TRUE : FALSE)
}   /* end H5T_vlen_str_mem_isnull() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_str_mem_read
 *
 * Purpose:  "Reads" the memory based VL string into a buffer
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_str_mem_read(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_vl, void *buf, size_t len)
{
    char *s=*(char **)_vl;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_str_mem_read)

    if(len>0) {
        /* check parameters */
        assert(s);
        assert(buf);

        HDmemcpy(buf,s,len);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* end H5T_vlen_str_mem_read() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_str_mem_write
 *
 * Purpose:  "Writes" the memory based VL string from a buffer
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_str_mem_write(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const H5T_vlen_alloc_info_t *vl_alloc_info, void *_vl, void *buf, void UNUSED *_bg, size_t seq_len, size_t base_size)
{
    char *t;                        /* Pointer to temporary buffer allocated */
    size_t len;                     /* Maximum length of the string to copy */
    herr_t      ret_value=SUCCEED;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_str_mem_write)

    /* check parameters */
    assert(buf);

    /* Use the user's memory allocation routine if one is defined */
    if(vl_alloc_info->alloc_func!=NULL) {
        if(NULL==(t=(vl_alloc_info->alloc_func)((seq_len+1)*base_size,vl_alloc_info->alloc_info)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data")
      } /* end if */
    else {  /* Default to system malloc */
        if(NULL==(t=H5MM_malloc((seq_len+1)*base_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data")
      } /* end else */

    len=(seq_len*base_size);
    HDmemcpy(t,buf,len);
    t[len]='\0';

    /* Set pointer in user's buffer with memcpy, to avoid alignment issues */
    HDmemcpy(_vl,&t,sizeof(char *));

done:
    FUNC_LEAVE_NOAPI(ret_value) /*lint !e429 The pointer in 't' has been copied */
}   /* end H5T_vlen_str_mem_write() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_str_mem_setnull
 *
 * Purpose:  Sets a VL info object in memory to the "null" value
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, November 8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_str_mem_setnull(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, void *_vl, void UNUSED *_bg)
{
    char *t=NULL;                   /* Pointer to temporary buffer allocated */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_str_mem_setnull)

    /* Set pointer in user's buffer with memcpy, to avoid alignment issues */
    HDmemcpy(_vl,&t,sizeof(char *));

    FUNC_LEAVE_NOAPI(SUCCEED) /*lint !e429 The pointer in 't' has been copied */
}   /* end H5T_vlen_str_mem_setnull() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_disk_getlen
 *
 * Purpose:  Retrieves the length of a disk based VL element.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5T_vlen_disk_getlen(const void *_vl)
{
    const uint8_t *vl=(const uint8_t *)_vl; /* Pointer to the disk VL information */
    size_t  seq_len;        /* Sequence length */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_disk_getlen)

    /* check parameters */
    assert(vl);

    UINT32DECODE(vl, seq_len);

    FUNC_LEAVE_NOAPI((ssize_t)seq_len)
}   /* end H5T_vlen_disk_getlen() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_disk_getptr
 *
 * Purpose:  Retrieves the pointer to a disk based VL element.
 *
 * Return:  Non-NULL on success/NULL on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, June 12, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static void *
H5T_vlen_disk_getptr(void UNUSED *vl)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_disk_getptr)

    /* check parameters */
    assert(vl);

    FUNC_LEAVE_NOAPI(NULL)
}   /* end H5T_vlen_disk_getptr() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_disk_isnull
 *
 * Purpose:  Checks if a disk VL info object is the "nil" object
 *
 * Return:  TRUE/FALSE on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, November 8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5T_vlen_disk_isnull(const H5F_t *f, void *_vl)
{
    uint8_t *vl=(uint8_t *)_vl; /* Pointer to the disk VL information */
    haddr_t addr;               /* Sequence's heap address */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_vlen_disk_isnull)

    /* check parameters */
    assert(vl);

    /* Skip the sequence's length */
    vl+=4;

    /* Get the heap address */
    H5F_addr_decode(f,(const uint8_t **)&vl,&addr);

    FUNC_LEAVE_NOAPI(addr==0 ? TRUE : FALSE)
}   /* end H5T_vlen_disk_isnull() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_disk_read
 *
 * Purpose:  Reads the disk based VL element into a buffer
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_disk_read(H5F_t *f, hid_t dxpl_id, void *_vl, void *buf, size_t UNUSED len)
{
    uint8_t *vl=(uint8_t *)_vl;   /* Pointer to the user's hvl_t information */
    H5HG_t hobjid;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_disk_read)

    /* check parameters */
    assert(vl);
    assert(buf);
    assert(f);

    /* Skip the length of the sequence */
    vl += 4;

    /* Get the heap information */
    H5F_addr_decode(f,(const uint8_t **)&vl,&(hobjid.addr));
    INT32DECODE(vl,hobjid.idx);

    /* Check if this sequence actually has any data */
    if(hobjid.addr>0) {
        /* Read the VL information from disk */
        if(H5HG_read(f,dxpl_id,&hobjid,buf)==NULL)
            HGOTO_ERROR(H5E_DATATYPE, H5E_READERROR, FAIL, "Unable to read VL information")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_disk_read() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_disk_write
 *
 * Purpose:  Writes the disk based VL element from a buffer
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *    Raymond Lu
 *    Thursday, June 26, 2002
 *    Free heap objects storing old data.
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5T_vlen_disk_write(H5F_t *f, hid_t dxpl_id, const H5T_vlen_alloc_info_t UNUSED *vl_alloc_info, void *_vl, void *buf, void *_bg, size_t seq_len, size_t base_size)
{
    uint8_t *vl=(uint8_t *)_vl; /*Pointer to the user's hvl_t information*/
    uint8_t *bg=(uint8_t *)_bg; /*Pointer to the old data hvl_t          */
    H5HG_t hobjid;              /* New VL sequence's heap ID */
    size_t len;                 /* Size of new sequence on disk (in bytes) */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_disk_write)

    /* check parameters */
    assert(vl);
    assert(seq_len==0 || buf);
    assert(f);

    /* Free heap object for old data.  */
    if(bg!=NULL) {
        H5HG_t bg_hobjid;       /* "Background" VL info sequence's ID info */

        /* Skip the length of the sequence and heap object ID from background data. */
        bg += 4;

        /* Get heap information */
        H5F_addr_decode(f, (const uint8_t **)&bg, &(bg_hobjid.addr));
        INT32DECODE(bg, bg_hobjid.idx);

        /* Free heap object for old data */
        if(bg_hobjid.addr>0) {
            /* Free heap object */
            if(H5HG_remove(f, dxpl_id, &bg_hobjid)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "Unable to remove heap object")
        } /* end if */
    } /* end if */

    /* Set the length of the sequence */
    UINT32ENCODE(vl, seq_len);

    /* Write the VL information to disk (allocates space also) */
    len=(seq_len*base_size);
    if(H5HG_insert(f,dxpl_id,len,buf,&hobjid)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "Unable to write VL information")

    /* Encode the heap information */
    H5F_addr_encode(f,&vl,hobjid.addr);
    INT32ENCODE(vl,hobjid.idx);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_disk_write() */


/*-------------------------------------------------------------------------
 * Function:  H5T_vlen_disk_setnull
 *
 * Purpose:  Sets a VL info object on disk to the "nil" value
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *    Saturday, November 8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_vlen_disk_setnull(H5F_t *f, hid_t dxpl_id, void *_vl, void *_bg)
{
    uint8_t *vl=(uint8_t *)_vl; /*Pointer to the user's hvl_t information*/
    uint8_t *bg=(uint8_t *)_bg; /*Pointer to the old data hvl_t          */
    uint32_t seq_len=0;         /* Sequence length */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_disk_setnull)

    /* check parameters */
    assert(f);
    assert(vl);

    /* Free heap object for old data.  */
    if(bg!=NULL) {
        H5HG_t bg_hobjid;       /* "Background" VL info sequence's ID info */

        /* Skip the length of the sequence and heap object ID from background data. */
        bg += 4;

        /* Get heap information */
        H5F_addr_decode(f, (const uint8_t **)&bg, &(bg_hobjid.addr));
        INT32DECODE(bg, bg_hobjid.idx);

        /* Free heap object for old data */
        if(bg_hobjid.addr>0) {
            /* Free heap object */
            if(H5HG_remove(f, dxpl_id, &bg_hobjid)<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "Unable to remove heap object")
         } /* end if */
    } /* end if */

    /* Set the length of the sequence */
    UINT32ENCODE(vl, seq_len);

    /* Encode the "nil" heap pointer information */
    H5F_addr_encode(f,&vl,(haddr_t)0);
    INT32ENCODE(vl,0);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_disk_setnull() */


/*--------------------------------------------------------------------------
 NAME
    H5T_vlen_reclaim_recurse
 PURPOSE
    Internal recursive routine to free VL datatypes
 USAGE
    herr_t H5T_vlen_reclaim(elem,dt)
        void *elem;  IN/OUT: Pointer to the dataset element
        H5T_t *dt;   IN: Datatype of dataset element

 RETURNS
    SUCCEED/FAIL
 DESCRIPTION
    Frees any dynamic memory used by VL datatypes in the current dataset
    element.  Performs a recursive depth-first traversal of all compound
    datatypes to free all VL datatype information allocated by any field.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5T_vlen_reclaim_recurse(void *elem, const H5T_t *dt, H5MM_free_t free_func, void *free_info)
{
    unsigned i;     /* local index variable */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5T_vlen_reclaim_recurse)

    assert(elem);
    assert(dt);

    /* Check the datatype of this element */
    switch(dt->shared->type) {
        case H5T_ARRAY:
            /* Recurse on each element, if the array's base type is array, VL, enum or compound */
            if(H5T_IS_COMPLEX(dt->shared->parent->shared->type)) {
                void *off;     /* offset of field */

                /* Calculate the offset member and recurse on it */
                for(i=0; i<dt->shared->u.array.nelem; i++) {
                    off=((uint8_t *)elem)+i*(dt->shared->parent->shared->size);
                    if(H5T_vlen_reclaim_recurse(off,dt->shared->parent,free_func,free_info)<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Unable to free array element")
                } /* end for */
            } /* end if */
            break;

        case H5T_COMPOUND:
            /* Check each field and recurse on VL, compound, enum or array ones */
            for (i=0; i<dt->shared->u.compnd.nmembs; i++) {
                /* Recurse if it's VL, compound, enum or array */
                if(H5T_IS_COMPLEX(dt->shared->u.compnd.memb[i].type->shared->type)) {
                    void *off;     /* offset of field */

                    /* Calculate the offset member and recurse on it */
                    off=((uint8_t *)elem)+dt->shared->u.compnd.memb[i].offset;
                    if(H5T_vlen_reclaim_recurse(off,dt->shared->u.compnd.memb[i].type,free_func,free_info)<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Unable to free compound field")
                } /* end if */
            } /* end for */
            break;

        case H5T_VLEN:
            /* Recurse on the VL information if it's VL, compound, enum or array, then free VL sequence */
            if(dt->shared->u.vlen.type==H5T_VLEN_SEQUENCE) {
                hvl_t *vl=(hvl_t *)elem;    /* Temp. ptr to the vl info */

                /* Check if there is anything actually in this sequence */
                if(vl->len!=0) {
                    /* Recurse if it's VL, array, enum or compound */
                    if(H5T_IS_COMPLEX(dt->shared->parent->shared->type)) {
                        void *off;     /* offset of field */

                        /* Calculate the offset of each array element and recurse on it */
                        while(vl->len>0) {
                            off=((uint8_t *)vl->p)+(vl->len-1)*dt->shared->parent->shared->size;
                            if(H5T_vlen_reclaim_recurse(off,dt->shared->parent,free_func,free_info)<0)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Unable to free VL element")
                            vl->len--;
                        } /* end while */
                    } /* end if */

                    /* Free the VL sequence */
                    if(free_func!=NULL)
                        (*free_func)(vl->p,free_info);
                    else
                        H5MM_xfree(vl->p);
                } /* end if */
            } else if(dt->shared->u.vlen.type==H5T_VLEN_STRING) {
                /* Free the VL string */
                if(free_func!=NULL)
                    (*free_func)(*(char **)elem,free_info);
                else
                    H5MM_xfree(*(char **)elem);
            } else {
                assert(0 && "Invalid VL type");
            } /* end else */
            break;

        default:
            break;
    } /* end switch */ /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_reclaim_recurse() */


/*--------------------------------------------------------------------------
 NAME
    H5T_vlen_reclaim
 PURPOSE
    Default method to reclaim any VL data for a buffer element
 USAGE
    herr_t H5T_vlen_reclaim(elem,type_id,ndim,point,op_data)
        void *elem;  IN/OUT: Pointer to the dataset element
        hid_t type_id;   IN: Datatype of dataset element
        unsigned ndim;    IN: Number of dimensions in dataspace
        hsize_t *point; IN: Coordinate location of element in dataspace
        void *op_data    IN: Operator data

 RETURNS
    SUCCEED/FAIL
 DESCRIPTION
    Frees any dynamic memory used by VL datatypes in the current dataset
    element.  Recursively descends compound datatypes to free all VL datatype
    information allocated by any field.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
/* ARGSUSED */
herr_t
H5T_vlen_reclaim(void *elem, hid_t type_id, unsigned UNUSED ndim, const hsize_t UNUSED *point, void *op_data)
{
    H5T_vlen_alloc_info_t *vl_alloc_info = (H5T_vlen_alloc_info_t *)op_data; /* VL allocation info from iterator */
    H5T_t  *dt;
    herr_t ret_value;

    FUNC_ENTER_NOAPI(H5T_vlen_reclaim, FAIL)

    assert(elem);
    assert(vl_alloc_info);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    /* Check args */
    if (NULL==(dt=H5I_object_verify(type_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Pull the free function and free info pointer out of the op_data and call the recurse datatype free function */
    ret_value=H5T_vlen_reclaim_recurse(elem,dt,vl_alloc_info->free_func,vl_alloc_info->free_info);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_reclaim() */


/*--------------------------------------------------------------------------
 NAME
    H5T_vlen_get_alloc_info
 PURPOSE
    Retrieve allocation info for VL datatypes
 USAGE
    herr_t H5T_vlen_get_alloc_info(dxpl_id,vl_alloc_info)
        hid_t dxpl_id;   IN: Data transfer property list to query
        H5T_vlen_alloc_info_t *vl_alloc_info;  IN/OUT: Pointer to VL allocation information to fill

 RETURNS
    SUCCEED/FAIL
 DESCRIPTION
    Retrieve the VL allocation functions and information from a dataset
    transfer property list.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    The VL_ALLOC_INFO pointer should point at already allocated memory to place
    non-default property list info.  If a default property list is used, the
    VL_ALLOC_INFO pointer will be changed to point at the default information.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5T_vlen_get_alloc_info(hid_t dxpl_id, H5T_vlen_alloc_info_t **vl_alloc_info)
{
    H5P_genplist_t *plist;              /* DX property list */
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5T_vlen_get_alloc_info, FAIL)

    assert(H5I_GENPROP_LST == H5I_get_type(dxpl_id));
    assert(vl_alloc_info);

    /* Check for the default DXPL */
    if(dxpl_id==H5P_DATASET_XFER_DEFAULT)
        *vl_alloc_info=&H5T_vlen_def_vl_alloc_info;
    else {
        /* Check args */
        if(NULL == (plist = H5P_object_verify(dxpl_id,H5P_DATASET_XFER)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

        /* Get the allocation functions & information */
        if (H5P_get(plist,H5D_XFER_VLEN_ALLOC_NAME,&(*vl_alloc_info)->alloc_func)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
        if (H5P_get(plist,H5D_XFER_VLEN_ALLOC_INFO_NAME,&(*vl_alloc_info)->alloc_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
        if (H5P_get(plist,H5D_XFER_VLEN_FREE_NAME,&(*vl_alloc_info)->free_func)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
        if (H5P_get(plist,H5D_XFER_VLEN_FREE_INFO_NAME,&(*vl_alloc_info)->free_info)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_vlen_get_alloc_info() */


/*--------------------------------------------------------------------------
 NAME
    H5T_vlen_mark
 PURPOSE
    Recursively mark any VL datatypes as on disk/in memory
 USAGE
    htri_t H5T_vlen_mark(dt,f,loc)
        H5T_t *dt;              IN/OUT: Pointer to the datatype to mark
        H5F_t *dt;              IN: Pointer to the file the datatype is in
        H5T_vlen_type_t loc     IN: location of VL type

 RETURNS
    One of two values on success:
        TRUE - If the location of any vlen types changed
        FALSE - If the location of any vlen types is the same
    <0 is returned on failure
 DESCRIPTION
    Recursively descends any VL or compound datatypes to mark all VL datatypes
    as either on disk or in memory.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5T_vlen_mark(H5T_t *dt, H5F_t *f, H5T_vlen_loc_t loc)
{
    htri_t vlen_changed;    /* Whether H5T_vlen_mark changed the type (even if the size didn't change) */
    htri_t ret_value = 0;   /* Indicate that success, but no location change */
    unsigned i;             /* Local index variable */
    int accum_change=0;     /* Amount of change in the offset of the fields */
    size_t old_size;        /* Previous size of a field */

    FUNC_ENTER_NOAPI(H5T_vlen_mark, FAIL);

    assert(dt);
    assert(loc>H5T_VLEN_BADLOC && loc<H5T_VLEN_MAXLOC);

    /* Check the datatype of this element */
    switch(dt->shared->type) {
        case H5T_ARRAY:  /* Recurse on VL, compound and array base element type */
            /* Recurse if it's VL, compound or array */
            /* (If the type is compound and the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
            if(dt->shared->parent->shared->force_conv && H5T_IS_COMPLEX(dt->shared->parent->shared->type)) {
                /* Keep the old base element size for later */
                old_size=dt->shared->parent->shared->size;

                /* Mark the VL, compound or array type */
                if((vlen_changed=H5T_vlen_mark(dt->shared->parent,f,loc))<0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                if(vlen_changed>0)
                    ret_value=vlen_changed;

                /* Check if the field changed size */
                if(old_size != dt->shared->parent->shared->size) {
                    /* Adjust the size of the array */
                    dt->shared->size = dt->shared->u.array.nelem*dt->shared->parent->shared->size;
                } /* end if */
            } /* end if */
            break;

        case H5T_COMPOUND:  /* Check each field and recurse on VL, compound and array type */
            /* Compound datatypes can't change in size if the force_conv flag is not set */
            if(dt->shared->force_conv) {
                /* Sort the fields based on offsets */
                H5T_sort_value(dt,NULL);

                for (i=0; i<dt->shared->u.compnd.nmembs; i++) {
                    /* Apply the accumulated size change to the offset of the field */
                    dt->shared->u.compnd.memb[i].offset += accum_change;

                    /* Recurse if it's VL, compound, enum or array */
                    /* (If the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
                    if(dt->shared->u.compnd.memb[i].type->shared->force_conv && H5T_IS_COMPLEX(dt->shared->u.compnd.memb[i].type->shared->type)) {
                        /* Keep the old field size for later */
                        old_size=dt->shared->u.compnd.memb[i].type->shared->size;

                        /* Mark the VL, compound, enum or array type */
                        if((vlen_changed=H5T_vlen_mark(dt->shared->u.compnd.memb[i].type,f,loc))<0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                        if(vlen_changed>0)
                            ret_value=vlen_changed;

                        /* Check if the field changed size */
                        if(old_size != dt->shared->u.compnd.memb[i].type->shared->size) {
                            /* Adjust the size of the member */
                            dt->shared->u.compnd.memb[i].size = (dt->shared->u.compnd.memb[i].size*dt->shared->u.compnd.memb[i].type->shared->size)/old_size;

                            /* Add that change to the accumulated size change */
                            accum_change += (dt->shared->u.compnd.memb[i].type->shared->size - (int)old_size);
                        } /* end if */
                    } /* end if */
                } /* end for */

                /* Apply the accumulated size change to the datatype */
                dt->shared->size += accum_change;
            } /* end if */
            break;

        case H5T_VLEN: /* Recurse on the VL information if it's VL, compound or array, then free VL sequence */
            /* Recurse if it's VL, compound or array */
            /* (If the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
            if(dt->shared->parent->shared->force_conv && H5T_IS_COMPLEX(dt->shared->parent->shared->type)) {
                if((vlen_changed=H5T_vlen_mark(dt->shared->parent,f,loc))<0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                if(vlen_changed>0)
                    ret_value=vlen_changed;
            } /* end if */

            /* Mark this VL sequence */
            if((vlen_changed=H5T_vlen_set_loc(dt,f,loc))<0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
            if(vlen_changed>0)
                ret_value=vlen_changed;
            break;

        default:
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5T_vlen_mark() */

