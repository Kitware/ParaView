/****************************************************************************
* NCSA HDF                                                                  *
* Software Development Group                                                *
* National Center for Supercomputing Applications                           *
* University of Illinois at Urbana-Champaign                                *
* 605 E. Springfield, Champaign IL 61820                                    *
*                                                                           *
* For conditions of distribution and use, see the accompanying              *
* hdf/COPYING file.                                                         *
*                                                                           *
****************************************************************************/

/* Id */

#define H5T_PACKAGE             /*suppress error about including H5Tpkg    */

#include "H5private.h"          /* Generic Functions                       */
#include "H5Eprivate.h"         /* Errors                                  */
#include "H5HGprivate.h"        /* Global Heaps                            */
#include "H5Iprivate.h"         /* IDs                                     */
#include "H5MMprivate.h"        /* Memory Allocation                       */
#include "H5Tpkg.h"             /* Datatypes                               */

#define PABLO_MASK      H5Tvlen_mask

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT NULL

/* Local functions */
static herr_t H5T_vlen_reclaim_recurse(void *elem, H5T_t *dt, H5MM_free_t free_func, void *free_info);


/*-------------------------------------------------------------------------
 * Function: H5T_vlen_set_loc
 *
 * Purpose:     Sets the location of a VL datatype to be either on disk or in memory
 *
 * Return:      
 *  One of two values on success:
 *      TRUE - If the location of any vlen types changed
 *      FALSE - If the location of any vlen types is the same
 *  <0 is returned on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, June 4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5T_vlen_set_loc(H5T_t *dt, H5F_t *f, H5T_vlen_loc_t loc)
{
    htri_t ret_value = 0;       /* Indicate that success, but no location change */

    FUNC_ENTER (H5T_vlen_set_loc, FAIL);

    /* check parameters */
    assert(dt);
    assert(loc>H5T_VLEN_BADLOC && loc<H5T_VLEN_MAXLOC);

    /* Only change the location if it's different */
    if(loc!=dt->u.vlen.loc) {
        /* Indicate that the location changed */
        ret_value=TRUE;

        switch(loc) {
            case H5T_VLEN_MEMORY:   /* Memory based VL datatype */
                assert(f==NULL);

                /* Mark this type as being stored in memory */
                dt->u.vlen.loc=H5T_VLEN_MEMORY;

                if(dt->u.vlen.type==H5T_VLEN_SEQUENCE) {
                    /* size in memory, disk size is different */
                    dt->size = sizeof(hvl_t);

                    /* Set up the function pointers to access the VL sequence in memory */
                    dt->u.vlen.getlen=H5T_vlen_seq_mem_getlen;
                    dt->u.vlen.read=H5T_vlen_seq_mem_read;
                    dt->u.vlen.write=H5T_vlen_seq_mem_write;
                } else if(dt->u.vlen.type==H5T_VLEN_STRING) {
                    /* size in memory, disk size is different */
                    dt->size = sizeof(char *);

                    /* Set up the function pointers to access the VL string in memory */
                    dt->u.vlen.getlen=H5T_vlen_str_mem_getlen;
                    dt->u.vlen.read=H5T_vlen_str_mem_read;
                    dt->u.vlen.write=H5T_vlen_str_mem_write;
                } else {
                    assert(0 && "Invalid VL type");
                }

                /* Reset file ID (since this VL is in memory) */
                dt->u.vlen.f=NULL;
                break;

            case H5T_VLEN_DISK:   /* Disk based VL datatype */
                assert(f);

                /* Mark this type as being stored on disk */
                dt->u.vlen.loc=H5T_VLEN_DISK;

                /* 
                 * Size of element on disk is 4 bytes for the length, plus the size
                 * of an address in this file, plus 4 bytes for the size of a heap
                 * ID.  Memory size is different
                 */
                dt->size = 4 + H5F_SIZEOF_ADDR(f) + 4;

                /* Set up the function pointers to access the VL information on disk */
                /* VL sequences and VL strings are stored identically on disk, so use the same functions */
                dt->u.vlen.getlen=H5T_vlen_disk_getlen;
                dt->u.vlen.read=H5T_vlen_disk_read;
                dt->u.vlen.write=H5T_vlen_disk_write;

                /* Set file ID (since this VL is on disk) */
                dt->u.vlen.f=f;
                break;

            default:
                HRETURN_ERROR (H5E_DATATYPE, H5E_BADRANGE, FAIL, "invalid VL datatype location");
        } /* end switch */
    } /* end if */

    FUNC_LEAVE (ret_value);
}   /* end H5T_vlen_set_loc() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_seq_mem_getlen
 *
 * Purpose:     Retrieves the length of a memory based VL element.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hssize_t H5T_vlen_seq_mem_getlen(H5F_t UNUSED *f, void *vl_addr)
{
    hvl_t *vl=(hvl_t *)vl_addr;   /* Pointer to the user's hvl_t information */
    hssize_t    ret_value = FAIL;       /*return value                  */

    FUNC_ENTER (H5T_vlen_seq_mem_getlen, FAIL);

    /* check parameters */
    assert(vl);

    ret_value=(hssize_t)vl->len;

    FUNC_LEAVE (ret_value);
  
    f = 0;
}   /* end H5T_vlen_seq_mem_getlen() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_seq_mem_read
 *
 * Purpose:     "Reads" the memory based VL sequence into a buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5T_vlen_seq_mem_read(H5F_t UNUSED *f, void *vl_addr, void *buf, size_t len)
{
    hvl_t *vl=(hvl_t *)vl_addr;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER (H5T_vlen_seq_mem_read, FAIL);

    /* check parameters */
    assert(vl && vl->p);
    assert(buf);

    HDmemcpy(buf,vl->p,len);

    FUNC_LEAVE (SUCCEED);
  
    f = 0;
}   /* end H5T_vlen_seq_mem_read() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_seq_mem_write
 *
 * Purpose:     "Writes" the memory based VL sequence from a buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5T_vlen_seq_mem_write(const H5D_xfer_t *xfer_parms, H5F_t UNUSED *f, void *vl_addr, void *buf, hsize_t seq_len, hsize_t base_size)
{
    hvl_t *vl=(hvl_t *)vl_addr;   /* Pointer to the user's hvl_t information */
    size_t len=seq_len*base_size;

    FUNC_ENTER (H5T_vlen_seq_mem_write, FAIL);

    /* check parameters */
    assert(vl);
    assert(buf);

    if(seq_len!=0) {
        /* Use the user's memory allocation routine is one is defined */
        assert((seq_len*base_size)==(hsize_t)((size_t)(seq_len*base_size))); /*check for overflow*/
        if(xfer_parms->vlen_alloc!=NULL) {
            if(NULL==(vl->p=(xfer_parms->vlen_alloc)((size_t)(seq_len*base_size),xfer_parms->alloc_info)))
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data");
          } /* end if */
        else {  /* Default to system malloc */
            if(NULL==(vl->p=H5MM_malloc((size_t)(seq_len*base_size))))
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data");
          } /* end else */

        /* Copy the data into the newly allocated buffer */
        HDmemcpy(vl->p,buf,len);

    } /* end if */
    else
        vl->p=NULL;

    /* Set the sequence length */
    vl->len=seq_len;

    FUNC_LEAVE (SUCCEED);
  
    f = 0;
}   /* end H5T_vlen_seq_mem_write() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_str_mem_getlen
 *
 * Purpose:     Retrieves the length of a memory based VL string.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hssize_t H5T_vlen_str_mem_getlen(H5F_t UNUSED *f, void *vl_addr)
{
    char *s=*(char **)vl_addr;   /* Pointer to the user's hvl_t information */
    hssize_t    ret_value = FAIL;       /*return value                  */

    FUNC_ENTER (H5T_vlen_str_mem_getlen, FAIL);

    /* check parameters */
    assert(s);

    ret_value=(hssize_t)HDstrlen(s);

    FUNC_LEAVE (ret_value);
  
    f = 0;
}   /* end H5T_vlen_str_mem_getlen() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_str_mem_read
 *
 * Purpose:     "Reads" the memory based VL string into a buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5T_vlen_str_mem_read(H5F_t UNUSED *f, void *vl_addr, void *buf, size_t len)
{
    char *s=*(char **)vl_addr;   /* Pointer to the user's hvl_t information */

    FUNC_ENTER (H5T_vlen_str_mem_read, FAIL);

    /* check parameters */
    assert(s);
    assert(buf);

    HDmemcpy(buf,s,len);

    FUNC_LEAVE (SUCCEED);
  
    f = 0;
}   /* end H5T_vlen_str_mem_read() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_str_mem_write
 *
 * Purpose:     "Writes" the memory based VL string from a buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5T_vlen_str_mem_write(const H5D_xfer_t *xfer_parms, H5F_t UNUSED *f, void *vl_addr, void *buf, hsize_t seq_len, hsize_t base_size)
{
    char **s=(char **)vl_addr;   /* Pointer to the user's hvl_t information */
    size_t len=seq_len*base_size;

    FUNC_ENTER (H5T_vlen_str_mem_write, FAIL);

    /* check parameters */
    assert(buf);

    /* Use the user's memory allocation routine is one is defined */
    assert(((seq_len+1)*base_size)==(hsize_t)((size_t)((seq_len+1)*base_size))); /*check for overflow*/
    if(xfer_parms->vlen_alloc!=NULL) {
        if(NULL==(*s=(xfer_parms->vlen_alloc)((size_t)((seq_len+1)*base_size),xfer_parms->alloc_info)))
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data");
      } /* end if */
    else {  /* Default to system malloc */
        if(NULL==(*s=H5MM_malloc((size_t)((seq_len+1)*base_size))))
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for VL data");
      } /* end else */

    HDmemcpy(*s,buf,len);
    (*s)[len]='\0';

    FUNC_LEAVE (SUCCEED);
  
    f = 0;
}   /* end H5T_vlen_str_mem_write() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_disk_getlen
 *
 * Purpose:     Retrieves the length of a disk based VL element.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hssize_t H5T_vlen_disk_getlen(H5F_t UNUSED *f, void *vl_addr)
{
    uint8_t *vl=(uint8_t *)vl_addr;   /* Pointer to the disk VL information */
    hssize_t    ret_value = FAIL;       /*return value                  */

    FUNC_ENTER (H5T_vlen_disk_getlen, FAIL);

    /* check parameters */
    assert(vl);

    UINT32DECODE(vl, ret_value);

    FUNC_LEAVE (ret_value);
  
    f = 0;
}   /* end H5T_vlen_disk_getlen() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_disk_read
 *
 * Purpose:     Reads the disk based VL element into a buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5T_vlen_disk_read(H5F_t *f, void *vl_addr, void *buf, size_t UNUSED len)
{
    uint8_t *vl=(uint8_t *)vl_addr;   /* Pointer to the user's hvl_t information */
    H5HG_t hobjid;
    uint32_t seq_len;

    FUNC_ENTER (H5T_vlen_disk_read, FAIL);

    /* check parameters */
    assert(vl);
    assert(buf);
    assert(f);

    /* Get the length of the sequence */
    UINT32DECODE(vl, seq_len); /* Not used */
    
    /* Check if this sequence actually has any data */
    if(seq_len!=0) {
        /* Get the heap information */
        H5F_addr_decode(f,(const uint8_t **)&vl,&(hobjid.addr));
        INT32DECODE(vl,hobjid.idx);

        /* Read the VL information from disk */
        if(H5HG_read(f,&hobjid,buf)==NULL)
            HRETURN_ERROR(H5E_DATATYPE, H5E_READERROR, FAIL, "Unable to read VL information");
    } /* end if */

    FUNC_LEAVE (SUCCEED);
  
    len = 0;
}   /* end H5T_vlen_disk_read() */


/*-------------------------------------------------------------------------
 * Function:    H5T_vlen_disk_write
 *
 * Purpose:     Writes the disk based VL element from a buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, June 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5T_vlen_disk_write(const H5D_xfer_t UNUSED *xfer_parms, H5F_t *f, void *vl_addr, void *buf, hsize_t seq_len, hsize_t base_size)
{
    uint8_t *vl=(uint8_t *)vl_addr;   /* Pointer to the user's hvl_t information */
    H5HG_t hobjid;
    size_t len=seq_len*base_size;

    FUNC_ENTER (H5T_vlen_disk_write, FAIL);

    /* check parameters */
    assert(vl);
    assert(buf);
    assert(f);

    /* Set the length of the sequence */
    UINT32ENCODE(vl, seq_len);
    
    /* Check if this sequence actually has any data */
    if(seq_len!=0) {
        /* Write the VL information to disk (allocates space also) */
        if(H5HG_insert(f,len,buf,&hobjid)<0)
            HRETURN_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "Unable to write VL information");
    } /* end if */
    else
        HDmemset(&hobjid,0,sizeof(H5HG_t));

    /* Get the heap information */
    H5F_addr_encode(f,&vl,hobjid.addr);
    INT32ENCODE(vl,hobjid.idx);

    FUNC_LEAVE (SUCCEED);
    xfer_parms = 0;
}   /* end H5T_vlen_disk_write() */


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
H5T_vlen_reclaim_recurse(void *elem, H5T_t *dt, H5MM_free_t free_func, void *free_info)
{
    int i;     /* local index variable */
    size_t j;   /* local index variable */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER(H5T_vlen_reclaim_recurse, FAIL);

    assert(elem);
    assert(dt);

    /* Check the datatype of this element */
    switch(dt->type) {
        case H5T_ARRAY:
            /* Recurse on each element, if the array's base type is array, VL or compound */
            if(dt->parent->type==H5T_COMPOUND || dt->parent->type==H5T_VLEN || dt->parent->type==H5T_ARRAY) {
                void *off;     /* offset of field */

                /* Calculate the offset member and recurse on it */
                for(j=0; j<dt->u.array.nelem; j++) {
                    off=((uint8_t *)elem)+j*(dt->parent->size);
                    if(H5T_vlen_reclaim_recurse(off,dt->parent,free_func,free_info)<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Unable to free array element");
                } /* end for */
            } /* end if */
            break;

        case H5T_COMPOUND:
            /* Check each field and recurse on VL, compound or array ones */
            for (i=0; i<dt->u.compnd.nmembs; i++) {
                /* Recurse if it's VL, compound or array */
                if(dt->u.compnd.memb[i].type->type==H5T_COMPOUND || dt->u.compnd.memb[i].type->type==H5T_VLEN || dt->u.compnd.memb[i].type->type==H5T_ARRAY) {
                    void *off;     /* offset of field */

                    /* Calculate the offset member and recurse on it */
                    off=((uint8_t *)elem)+dt->u.compnd.memb[i].offset;
                    if(H5T_vlen_reclaim_recurse(off,dt->u.compnd.memb[i].type,free_func,free_info)<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Unable to free compound field");
                } /* end if */
            } /* end for */
            break;

        case H5T_VLEN:
            /* Recurse on the VL information if it's VL, compound or array, then free VL sequence */
            if(dt->u.vlen.type==H5T_VLEN_SEQUENCE) {
                hvl_t *vl=(hvl_t *)elem;    /* Temp. ptr to the vl info */

                /* Check if there is anything actually in this sequence */
                if(vl->len!=0) {
                    /* Recurse if it's VL or compound */
                    if(dt->parent->type==H5T_COMPOUND || dt->parent->type==H5T_VLEN || dt->parent->type==H5T_ARRAY) {
                        void *off;     /* offset of field */

                        /* Calculate the offset of each array element and recurse on it */
                        while(vl->len>0) {
                            off=((uint8_t *)vl->p)+(vl->len-1)*dt->parent->size;
                            if(H5T_vlen_reclaim_recurse(off,dt->parent,free_func,free_info)<0)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Unable to free VL element");
                            vl->len--;
                        } /* end while */
                    } /* end if */

                    /* Free the VL sequence */
                    if(free_func!=NULL)
                        (*free_func)(vl->p,free_info);
                    else
                        H5MM_xfree(vl->p);
                } /* end if */
            } else if(dt->u.vlen.type==H5T_VLEN_STRING) {
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
    } /* end switch */

done:
    FUNC_LEAVE(ret_value);
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
        hsize_t ndim;    IN: Number of dimensions in dataspace
        hssize_t *point; IN: Coordinate location of element in dataspace
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
herr_t 
H5T_vlen_reclaim(void *elem, hid_t type_id, hsize_t UNUSED ndim, hssize_t UNUSED *point, void *op_data)
{
    H5D_xfer_t     *xfer_parms = (H5D_xfer_t *)op_data; /* Dataset transfer plist from iterator */
    H5T_t       *dt = NULL;
    herr_t ret_value = FAIL;

    FUNC_ENTER(H5T_vlen_reclaim, FAIL);

    assert(elem);
    assert(H5I_DATATYPE == H5I_get_type(type_id));

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type_id) || NULL==(dt=H5I_object(type_id)))
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");

    /* Pull the free function and free info pointer out of the op_data and call the recurse datatype free function */
    ret_value=H5T_vlen_reclaim_recurse(elem,dt,xfer_parms->vlen_free,xfer_parms->free_info);

#ifdef LATER
done:
#endif /* LATER */
    FUNC_LEAVE(ret_value);

    ndim = 0;
    point = 0;
}   /* end H5T_vlen_reclaim() */


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
    int i;                 /* Local index variable */
    int accum_change=0;    /* Amount of change in the offset of the fields */
    size_t old_size;        /* Previous size of a field */

    FUNC_ENTER(H5T_vlen_mark, FAIL);

    assert(dt);
    assert(loc>H5T_VLEN_BADLOC && loc<H5T_VLEN_MAXLOC);

    /* Check the datatype of this element */
    switch(dt->type) {
        case H5T_ARRAY:  /* Recurse on VL, compound and array base element type */
            /* Recurse if it's VL, compound or array */
            /* (If the type is compound and the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
            if((dt->parent->type==H5T_COMPOUND && dt->parent->force_conv) || dt->parent->type==H5T_VLEN || dt->parent->type==H5T_ARRAY) {
                /* Keep the old base element size for later */
                old_size=dt->parent->size;

                /* Mark the VL, compound or array type */
                if((vlen_changed=H5T_vlen_mark(dt->parent,f,loc))<0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                if(vlen_changed>0)
                    ret_value=vlen_changed;
                
                /* Check if the field changed size */
                if(old_size != dt->parent->size) {
                    /* Adjust the size of the array */
                    dt->size = dt->u.array.nelem*dt->parent->size;
                } /* end if */
            } /* end if */
            break;

        case H5T_COMPOUND:  /* Check each field and recurse on VL, compound and array type */
            /* Compound datatypes can't change in size if the force_conv flag is not set */
            if(dt->force_conv) {
                /* Sort the fields based on offsets */
                H5T_sort_value(dt,NULL);
        
                for (i=0; i<dt->u.compnd.nmembs; i++) {
                    /* Apply the accumulated size change to the offset of the field */
                    dt->u.compnd.memb[i].offset += accum_change;

                    /* Recurse if it's VL, compound or array */
                    /* (If the type is compound and the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
                    if((dt->u.compnd.memb[i].type->type==H5T_COMPOUND && dt->u.compnd.memb[i].type->force_conv) || dt->u.compnd.memb[i].type->type==H5T_VLEN || dt->u.compnd.memb[i].type->type==H5T_ARRAY) {
                        /* Keep the old field size for later */
                        old_size=dt->u.compnd.memb[i].type->size;

                        /* Mark the VL, compound or array type */
                        if((vlen_changed=H5T_vlen_mark(dt->u.compnd.memb[i].type,f,loc))<0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                        if(vlen_changed>0)
                            ret_value=vlen_changed;
                        
                        /* Check if the field changed size */
                        if(old_size != dt->u.compnd.memb[i].type->size) {
                            /* Adjust the size of the member */
                            dt->u.compnd.memb[i].size = (dt->u.compnd.memb[i].size*dt->u.compnd.memb[i].type->size)/old_size;

                            /* Add that change to the accumulated size change */
                            accum_change += (dt->u.compnd.memb[i].type->size - (int)old_size);
                        } /* end if */
                    } /* end if */
                } /* end for */

                /* Apply the accumulated size change to the datatype */
                dt->size += accum_change;
            } /* end if */
            break;

        case H5T_VLEN: /* Recurse on the VL information if it's VL, compound or array, then free VL sequence */
            /* Recurse if it's VL, compound or array */
            /* (If the type is compound and the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
            if((dt->parent->type==H5T_COMPOUND && dt->parent->force_conv) || dt->parent->type==H5T_VLEN || dt->parent->type==H5T_ARRAY) {
                if((vlen_changed=H5T_vlen_mark(dt->parent,f,loc))<0)
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
    FUNC_LEAVE(ret_value);
}   /* end H5T_vlen_mark() */

