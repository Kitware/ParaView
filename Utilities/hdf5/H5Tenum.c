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
 * Module Info: This module contains the functionality for enumerated datatypes
 *      in the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5FLprivate.h"	/*Free Lists	  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5MMprivate.h"	/*memory management			  */
#include "H5Tpkg.h"		/*data-type functions			  */

#define PABLO_MASK	H5Tenum_mask

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5T_init_enum_interface
static herr_t H5T_init_enum_interface(void);

/* Declare extern the free list for H5T_t's */
H5FL_EXTERN(H5T_t);

/* Static local functions */
static char *H5T_enum_nameof(H5T_t *dt, void *value, char *name/*out*/,
			      size_t size);
static herr_t H5T_enum_valueof(H5T_t *dt, const char *name,
				void *value/*out*/);


/*--------------------------------------------------------------------------
NAME
   H5T_init_enum_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_enum_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_enum_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_enum_interface);

    FUNC_LEAVE_NOAPI(H5T_init());
} /* H5T_init_enum_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tenum_create
 *
 * Purpose:	Create a new enumeration data type based on the specified
 *		TYPE, which must be an integer type.
 *
 * Return:	Success:	ID of new enumeration data type
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, December 22, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tenum_create(hid_t parent_id)
{
    H5T_t	*parent = NULL;		/*base integer data type	*/
    H5T_t	*dt = NULL;		/*new enumeration data type	*/
    hid_t	ret_value;	        /*return value			*/
    
    FUNC_ENTER_API(H5Tenum_create, FAIL);
    H5TRACE1("i","i",parent_id);

    /* Check args */
    if (NULL==(parent=H5I_object_verify(parent_id,H5I_DATATYPE)) || H5T_INTEGER!=parent->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an integer data type");

    /* Build new type */
    if((dt=H5T_enum_create(parent))==NULL)
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "cannot create enum type");
    /* Atomize the type */
    if ((ret_value=H5I_register(H5I_DATATYPE, dt))<0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register data type atom");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_enum_create
 *
 * Purpose:	Private function for H5Tenum_create.  Create a new 
 *              enumeration data type based on the specified
 *		TYPE, which must be an integer type.
 *
 * Return:	Success:	new enumeration data type
 *
 *		Failure:        NULL	
 *
 * Programmer:	Raymond Lu
 *              October 9, 2002 
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_enum_create(H5T_t *parent)
{
    H5T_t	*ret_value;		/*new enumeration data type	*/
    
    FUNC_ENTER_NOAPI(H5T_enum_create, NULL);

    assert(parent);

    /* Build new type */
    if (NULL==(ret_value = H5FL_CALLOC(H5T_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    ret_value->type = H5T_ENUM;
    ret_value->parent = H5T_copy(parent, H5T_COPY_ALL);
    assert(ret_value->parent);
    ret_value->size = ret_value->parent->size;
    ret_value->ent.header = HADDR_UNDEF;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tenum_insert
 *
 * Purpose:	Insert a new enumeration data type member into an enumeration
 *		type. TYPE is the enumeration type, NAME is the name of the
 *		new member, and VALUE points to the value of the new member.
 *		The NAME and VALUE must both be unique within the TYPE. VALUE
 *		points to data which is of the data type defined when the
 *		enumeration type was created.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tenum_insert(hid_t type, const char *name, void *value)
{
    H5T_t	*dt=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_API(H5Tenum_insert, FAIL);
    H5TRACE3("e","isx",type,name,value);

    /* Check args */
    if (NULL==(dt=H5I_object_verify(type,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (H5T_ENUM!=dt->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an enumeration data type");
    if (!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    if (!value)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no value specified");

    /* Do work */
    if (H5T_enum_insert(dt, name, value)<0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to insert new enumeration member");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_enum_insert
 *
 * Purpose:	Insert a new member having a NAME and VALUE into an
 *		enumeration data TYPE.  The NAME and VALUE must both be
 *		unique. The VALUE points to data of the data type defined for
 *		the enumeration base type.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_enum_insert(H5T_t *dt, const char *name, void *value)
{
    int		i;
    char	**names=NULL;
    uint8_t	*values=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5T_enum_insert, FAIL);

    assert(dt);
    assert(name && *name);
    assert(value);

    /* The name and value had better not already exist */
    for (i=0; i<dt->u.enumer.nmembs; i++) {
	if (!HDstrcmp(dt->u.enumer.name[i], name))
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "name redefinition");
	if (!HDmemcmp(dt->u.enumer.value+i*dt->size, value, dt->size))
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "value redefinition");
    }

    /* Increase table sizes */
    if (dt->u.enumer.nmembs >= dt->u.enumer.nalloc) {
	int n = MAX(32, 2*dt->u.enumer.nalloc);
	if (NULL==(names=H5MM_realloc(dt->u.enumer.name, n*sizeof(char*))))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
	dt->u.enumer.name = names;

	if (NULL==(values=H5MM_realloc(dt->u.enumer.value, n*dt->size)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
	dt->u.enumer.value = values;
	dt->u.enumer.nalloc = n;
    }

    /* Insert new member at end of member arrays */
    dt->u.enumer.sorted = H5T_SORT_NONE;
    i = dt->u.enumer.nmembs++;
    dt->u.enumer.name[i] = H5MM_xstrdup(name);
    HDmemcpy(dt->u.enumer.value+i*dt->size, value, dt->size);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tget_member_value
 *
 * Purpose:	Return the value for an enumeration data type member.
 *
 * Return:	Success:	non-negative with the member value copied
 *				into the memory pointed to by VALUE.
 *
 *		Failure:	negative, VALUE memory is undefined.
 *
 * Programmer:	Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tget_member_value(hid_t type, int membno, void *value/*out*/)
{
    H5T_t	*dt=NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_API(H5Tget_member_value, FAIL);
    H5TRACE3("e","iIsx",type,membno,value);

    if (NULL==(dt=H5I_object_verify(type,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (H5T_ENUM!=dt->type)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not defined for data type class");
    if (membno<0 || membno>=dt->u.enumer.nmembs)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid member number");
    if (!value)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null value buffer");

    if (H5T_get_member_value(dt, membno, value)<0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get member value");
done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_get_member_value
 *
 * Purpose:	Private function for H5T_get_member_value.  Return the 
 *              value for an enumeration data type member.
 *
 * Return:	Success:	non-negative with the member value copied
 *				into the memory pointed to by VALUE.
 *
 *		Failure:	negative, VALUE memory is undefined.
 *
 * Programmer:	Raymond Lu
 *              October 9, 2002 
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_get_member_value(H5T_t *dt, int membno, void *value/*out*/)
{
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5T_get_member_value, FAIL);

    assert(dt);
    assert(value);

    HDmemcpy(value, dt->u.enumer.value + membno*dt->size, dt->size);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}



/*-------------------------------------------------------------------------
 * Function:	H5Tenum_nameof
 *
 * Purpose:	Finds the symbol name that corresponds to the specified VALUE
 *		of an enumeration data type TYPE. At most SIZE characters of
 *		the symbol name are copied into the NAME buffer. If the
 *		entire symbol anem and null terminator do not fit in the NAME
 *		buffer then as many characters as possible are copied (not
 *		null terminated) and the function fails.
 *
 * Return:	Success:	Non-negative.
 *
 *		Failure:	Negative, first character of NAME is set to
 *				null if SIZE allows it.
 *
 * Programmer:	Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tenum_nameof(hid_t type, void *value, char *name/*out*/, size_t size)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tenum_nameof, FAIL);
    H5TRACE4("e","ixxz",type,value,name,size);

    /* Check args */
    if (NULL==(dt=H5I_object_verify(type,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (H5T_ENUM!=dt->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an enumeration data type");
    if (!value)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no value supplied");
    if (!name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name buffer supplied");

    if (NULL==H5T_enum_nameof(dt, value, name, size))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "nameof query failed");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_enum_nameof
 *
 * Purpose:	Finds the symbol name that corresponds the the specified
 *		VALUE of an enumeration data type DT. At most SIZE characters
 *		of the symbol name are copied into the NAME buffer. If the
 *		entire symbol name and null terminator do not fit in the NAME
 *		buffer then as many characters as possible are copied and the
 *		function returns failure.
 *
 *		If NAME is the null pointer and SIZE is zero then enough
 *		space is allocated to hold the result and a pointer to that
 *		memory is returned.
 *
 * Return:	Success:	Pointer to NAME
 *
 *		Failure:	NULL, name[0] is set to null.
 *
 * Programmer:	Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static char *
H5T_enum_nameof(H5T_t *dt, void *value, char *name/*out*/, size_t size)
{
    int	lt, md, rt;		/*indices for binary search	*/
    int	cmp=(-1);		/*comparison result		*/
    char *ret_value;            /* Return value */
    
    FUNC_ENTER_NOAPI(H5T_enum_nameof, NULL);

    /* Check args */
    assert(dt && H5T_ENUM==dt->type);
    assert(value);
    assert(name || 0==size);
    if (name && size>0) *name = '\0';

    /* Sanity check */
    if (dt->u.enumer.nmembs == 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL, "datatype has no members");

    /* Do a binary search over the values to find the correct one */
    H5T_sort_value(dt, NULL);
    lt = 0;
    rt = dt->u.enumer.nmembs;
    md = -1;

    while (lt<rt) {
	md = (lt+rt)/2;
	cmp = HDmemcmp(value, dt->u.enumer.value+md*dt->size, dt->size);
	if (cmp<0) {
	    rt = md;
	} else if (cmp>0) {
	    lt = md+1;
	} else {
	    break;
	}
    }
    /* Value was not yet defined. This fixes bug # 774, 2002/06/05 EIP */
    if (cmp!=0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL, "value is currently not defined");

    /* Save result name */
    if (!name && NULL==(name=H5MM_malloc(HDstrlen(dt->u.enumer.name[md])+1)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    HDstrncpy(name, dt->u.enumer.name[md], size);
    if (HDstrlen(dt->u.enumer.name[md])>=size)
	HGOTO_ERROR(H5E_DATATYPE, H5E_NOSPACE, NULL, "name has been truncated");

    /* Set return value */
    ret_value=name;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Tenum_valueof
 *
 * Purpose:	Finds the value that corresponds to the specified NAME f an
 *		enumeration TYPE. The VALUE argument should be at least as
 *		large as the value of H5Tget_size(type) in order to hold the
 *		result.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tenum_valueof(hid_t type, const char *name, void *value/*out*/)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tenum_valueof, FAIL);
    H5TRACE3("e","isx",type,name,value);

    /* Check args */
    if (NULL==(dt=H5I_object_verify(type,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (H5T_ENUM!=dt->type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an enumeration data type");
    if (!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    if (!value)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no value buffer");

    if (H5T_enum_valueof(dt, name, value)<0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "valueof query failed");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5T_enum_valueof
 *
 * Purpose:	Finds the value that corresponds the the specified symbol
 *		NAME of an enumeration data type DT and copy it to the VALUE
 *		result buffer. The VALUE should be allocated by the caller to
 *		be large enough for the result.
 *
 * Return:	Success:	Non-negative, value stored in VALUE.
 *
 *		Failure:	Negative, VALUE is undefined.
 *
 * Programmer:	Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_enum_valueof(H5T_t *dt, const char *name, void *value/*out*/)
{
    int	lt, md, rt;		/*indices for binary search	*/
    int	cmp=(-1);		/*comparison result		*/
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_NOAPI(H5T_enum_valueof, FAIL);

    /* Check args */
    assert(dt && H5T_ENUM==dt->type);
    assert(name && *name);
    assert(value);

    /* Sanity check */
    if (dt->u.enumer.nmembs == 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "datatype has no members");

    /* Do a binary search over the names to find the correct one */
    H5T_sort_name(dt, NULL);
    lt = 0;
    rt = dt->u.enumer.nmembs;
    md = -1;

    while (lt<rt) {
	md = (lt+rt)/2;
	cmp = HDstrcmp(name, dt->u.enumer.name[md]);
	if (cmp<0) {
	    rt = md;
	} else if (cmp>0) {
	    lt = md+1;
	} else {
	    break;
	}
    }
    /* Value was not yet defined. This fixes bug # 774, 2002/06/05 EIP */
    if (cmp!=0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "string doesn't exist in the enumeration type");

    HDmemcpy(value, dt->u.enumer.value+md*dt->size, dt->size);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

