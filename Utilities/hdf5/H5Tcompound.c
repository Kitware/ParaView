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
 * Module Info: This module contains the functionality for compound datatypes
 *      in the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK	H5Tcompound_mask

#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5MMprivate.h"	/*memory management			  */
#include "H5Tpkg.h"		/*data-type functions			  */

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5T_init_compound_interface
static herr_t H5T_init_compound_interface(void);

/* Local macros */
#define H5T_COMPND_INC	64	/*typical max numb of members per struct */

/* Static local functions */
static size_t H5T_get_member_offset(H5T_t *dt, int membno);
static herr_t H5T_pack(H5T_t *dt);


/*--------------------------------------------------------------------------
NAME
   H5T_init_compound_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_compound_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_compound_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_compound_interface);

    FUNC_LEAVE_NOAPI(H5T_init());
} /* H5T_init_compound_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_member_offset
 *
 * Purpose:	Returns the byte offset of the beginning of a member with
 *		respect to the beginning of the compound data type datum.
 *
 * Return:	Success:	Byte offset.
 *
 *		Failure:	Zero. Zero is a valid offset, but this
 *				function will fail only if a call to
 *				H5Tget_member_dims() fails with the same
 *				arguments.
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Tget_member_offset(hid_t type_id, int membno)
{
    H5T_t	*dt = NULL;
    size_t	ret_value;

    FUNC_ENTER_API(H5Tget_member_offset, 0);
    H5TRACE2("z","iIs",type_id,membno);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)) || H5T_COMPOUND != dt->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a compound data type");
    if (membno < 0 || membno >= dt->u.compnd.nmembs)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, 0, "invalid member number");

    /* Value */
    ret_value = H5T_get_member_offset(dt, membno);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_get_member_offset
 *
 * Purpose:	Private function for H5Tget_member_offset.  Returns the byte 
 *              offset of the beginning of a member with respect to the i
 *              beginning of the compound data type datum.
 *
 * Return:	Success:	Byte offset.
 *
 *		Failure:	Zero. Zero is a valid offset, but this
 *				function will fail only if a call to
 *				H5Tget_member_dims() fails with the same
 *				arguments.
 *
 * Programmer:	Raymond Lu
 *		October 8, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5T_get_member_offset(H5T_t *dt, int membno)
{
    size_t	ret_value;

    FUNC_ENTER_NOAPI(H5T_get_member_offset, 0);

    assert(dt);
    assert(membno >= 0 && membno < dt->u.compnd.nmembs);

    /* Value */
    ret_value = dt->u.compnd.memb[membno].offset;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tget_member_class
 *
 * Purpose:	Returns the datatype class of a member of a compound datatype.
 *
 * Return:	Success: Non-negative
 *
 *		Failure: H5T_NO_CLASS
 *
 * Programmer:	Quincey Koziol
 *		Thursday, November  9, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_class_t
H5Tget_member_class(hid_t type_id, int membno)
{
    H5T_t	*dt = NULL;
    H5T_class_t	ret_value;

    FUNC_ENTER_API(H5Tget_member_class, H5T_NO_CLASS);
    H5TRACE2("Tt","iIs",type_id,membno);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)) || H5T_COMPOUND != dt->type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a compound data type");
    if (membno < 0 || membno >= dt->u.compnd.nmembs)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5T_NO_CLASS, "invalid member number");

    /* Value */
    ret_value = dt->u.compnd.memb[membno].type->type;

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Tget_member_class() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_member_type
 *
 * Purpose:	Returns the data type of the specified member.	The caller
 *		should invoke H5Tclose() to release resources associated with
 *		the type.
 *
 * Return:	Success:	An OID of a copy of the member data type;
 *				modifying the returned data type does not
 *				modify the member type.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 * 	Robb Matzke, 4 Jun 1998
 *	If the member type is a named type then this function returns a
 *	handle to the re-opened named type.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tget_member_type(hid_t type_id, int membno)
{
    H5T_t	*dt = NULL, *memb_dt = NULL;
    hid_t	ret_value;

    FUNC_ENTER_API(H5Tget_member_type, FAIL);
    H5TRACE2("i","iIs",type_id,membno);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)) || H5T_COMPOUND != dt->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    if (membno < 0 || membno >= dt->u.compnd.nmembs)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid member number");
    if ((memb_dt=H5T_get_member_type(dt, membno))==NULL)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to retrieve member type");
    if ((ret_value = H5I_register(H5I_DATATYPE, memb_dt)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable register data type atom");
    
done:
    if(ret_value<0) {
        if(memb_dt!=NULL)
            H5T_close(memb_dt);
    } /* end if */

    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_get_member_type
 *
 * Purpose:	Private function for H5Tget_member_type.  Returns the data 
 *              type of the specified member.
 *
 * Return:	Success:	A copy of the member data type;
 *				modifying the returned data type does not
 *				modify the member type.
 *
 *		Failure:        NULL	
 *
 * Programmer:	Raymond Lu
 *	        October 8, 2002	
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_get_member_type(H5T_t *dt, int membno)
{
    H5T_t	*ret_value = NULL;

    FUNC_ENTER_NOAPI(H5T_get_member_type, NULL);

    assert(dt);
    assert(membno >=0 && membno < dt->u.compnd.nmembs);
    
    /* Copy data type into an atom */
    if (NULL == (ret_value = H5T_copy(dt->u.compnd.memb[membno].type, H5T_COPY_REOPEN)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy member data type");
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tinsert
 *
 * Purpose:	Adds another member to the compound data type PARENT_ID.  The
 *		new member has a NAME which must be unique within the
 *		compound data type. The OFFSET argument defines the start of
 *		the member in an instance of the compound data type, and
 *		MEMBER_ID is the type of the new member.
 *
 * Return:	Success:	Non-negative, the PARENT_ID compound data
 *				type is modified to include a copy of the
 *				member type MEMBER_ID.
 *
 *		Failure:	Negative
 *
 * Errors:
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tinsert(hid_t parent_id, const char *name, size_t offset, hid_t member_id)
{
    H5T_t	*parent = NULL;		/*the compound parent data type */
    H5T_t	*member = NULL;		/*the atomic member type	*/
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tinsert, FAIL);
    H5TRACE4("e","iszi",parent_id,name,offset,member_id);

    /* Check args */
    if (parent_id==member_id)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't insert compound datatype within itself");
    if (NULL == (parent = H5I_object_verify(parent_id,H5I_DATATYPE)) || H5T_COMPOUND != parent->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    if (H5T_STATE_TRANSIENT!=parent->state)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "parent type read-only");
    if (!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no member name");
    if (NULL == (member = H5I_object_verify(member_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");

    /* Insert */
    if (H5T_insert(parent, name, offset, member) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "unable to insert member");
    
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tpack
 *
 * Purpose:	Recursively removes padding from within a compound data type
 *		to make it more efficient (space-wise) to store that data.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tpack(hid_t type_id)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tpack, FAIL);
    H5TRACE1("e","i",type_id);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)) || H5T_detect_class(dt,H5T_COMPOUND)<=0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");

    /* Pack */
    if (H5T_pack(dt) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to pack compound data type");
    
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_insert
 *
 * Purpose:	Adds a new MEMBER to the compound data type PARENT.  The new
 *		member will have a NAME that is unique within PARENT and an
 *		instance of PARENT will have the member begin at byte offset
 *		OFFSET from the beginning.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 * Modifications:
 *  Took out arrayness parameters - QAK, 10/6/00
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_insert(H5T_t *parent, const char *name, size_t offset, const H5T_t *member)
{
    int		idx, i;
    size_t		total_size;
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5T_insert, FAIL);

    /* check args */
    assert(parent && H5T_COMPOUND == parent->type);
    assert(H5T_STATE_TRANSIENT==parent->state);
    assert(member);
    assert(name && *name);

    /* Does NAME already exist in PARENT? */
    for (i=0; i<parent->u.compnd.nmembs; i++) {
	if (!HDstrcmp(parent->u.compnd.memb[i].name, name))
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "member name is not unique");
    }

    /* Does the new member overlap any existing member ? */
    total_size=member->size;
    for (i=0; i<parent->u.compnd.nmembs; i++) {
	if ((offset <= parent->u.compnd.memb[i].offset &&
                 offset + total_size > parent->u.compnd.memb[i].offset) ||
                (parent->u.compnd.memb[i].offset <= offset &&
                 parent->u.compnd.memb[i].offset +
                 parent->u.compnd.memb[i].size > offset))
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "member overlaps with another member");
    }

    /* Does the new member overlap the end of the compound type? */
    if(offset+total_size>parent->size)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "member extends past end of compound type");

    /* Increase member array if necessary */
    if (parent->u.compnd.nmembs >= parent->u.compnd.nalloc) {
        size_t na = parent->u.compnd.nalloc + H5T_COMPND_INC;
        H5T_cmemb_t *x = H5MM_realloc (parent->u.compnd.memb,
                           na * sizeof(H5T_cmemb_t));

        if (!x)
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
        parent->u.compnd.nalloc = (int)na;
        parent->u.compnd.memb = x;
    }

    /* Add member to end of member array */
    idx = parent->u.compnd.nmembs;
    parent->u.compnd.memb[idx].name = H5MM_xstrdup(name);
    parent->u.compnd.memb[idx].offset = offset;
    parent->u.compnd.memb[idx].size = total_size;
    parent->u.compnd.memb[idx].type = H5T_copy (member, H5T_COPY_ALL);

    parent->u.compnd.sorted = H5T_SORT_NONE;
    parent->u.compnd.nmembs++;

    /* Determine if the compound datatype stayed packed */
    if(parent->u.compnd.packed) {
        /* Check if the member type is packed */
        if(H5T_is_packed(parent->u.compnd.memb[idx].type)>0) {
            if(idx==0) {
                /* If the is the first member, the datatype is not packed
                 * if the first member isn't at offset 0
                 */
                if(parent->u.compnd.memb[idx].offset>0)
                    parent->u.compnd.packed=FALSE;
            } /* end if */
            else {
                /* If the is not the first member, the datatype is not
                 * packed if the new member isn't adjoining the previous member
                 */
                if(parent->u.compnd.memb[idx].offset!=(parent->u.compnd.memb[idx-1].offset+parent->u.compnd.memb[idx-1].size))
                    parent->u.compnd.packed=FALSE;
            } /* end else */
        } /* end if */
        else
            parent->u.compnd.packed=FALSE;
    } /* end if */

    /*
     * Set the "force conversion" flag if the field's datatype indicates
     */
    if(member->force_conv==TRUE)
        parent->force_conv=TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_pack
 *
 * Purpose:	Recursively packs a compound data type by removing padding
 *		bytes. This is done in place (that is, destructively).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_pack(H5T_t *dt)
{
    int		i;
    size_t	offset;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_pack);

    assert(dt);

    if(H5T_detect_class(dt,H5T_COMPOUND)>0) {
        /* If datatype has been packed, skip packing it and indicate success */
        if(H5T_is_packed(dt)== TRUE)
            HGOTO_DONE(SUCCEED);

        /* Check for packing unmodifiable datatype */
        if (H5T_STATE_TRANSIENT!=dt->state)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "datatype is read-only");
	
        if(dt->parent) {
            if (H5T_pack(dt->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to pack parent of datatype");

            /* Adjust size of datatype appropriately */
            if(dt->type==H5T_ARRAY)
                dt->size = dt->parent->size * dt->u.array.nelem;
            else if(dt->type!=H5T_VLEN)
                dt->size = dt->parent->size;
        } /* end if */
        else if(dt->type==H5T_COMPOUND) {
            /* Recursively pack the members */
            for (i=0; i<dt->u.compnd.nmembs; i++)
                if (H5T_pack(dt->u.compnd.memb[i].type) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to pack part of a compound data type");

            /* Remove padding between members */
            H5T_sort_value(dt, NULL);
            for (i=0, offset=0; i<dt->u.compnd.nmembs; i++) {
                dt->u.compnd.memb[i].offset = offset;
                offset += dt->u.compnd.memb[i].size;
            }

            /* Change total size */
            dt->size = MAX(1, offset);

            /* Mark the type as packed now */
            dt->u.compnd.packed=TRUE;
        } /* end if */
    } /* end if */
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_is_packed
 *
 * Purpose:	Checks whether a datatype which is compound (or has compound
 *              components) is packed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, September 11, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_packed(H5T_t *dt)
{
    htri_t      ret_value=TRUE;       /* Return value */

    FUNC_ENTER_NOAPI(H5T_is_packed,FAIL);

    assert(dt);

    /* Go up the chain as far as possible */
    while(dt->parent)
        dt=dt->parent;

    /* If this is a compound datatype, check if it is packed */
    if(dt->type==H5T_COMPOUND)
        ret_value=dt->u.compnd.packed;

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5T_is_packed() */

