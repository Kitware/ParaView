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

#define H5A_PACKAGE		/*suppress error about including H5Apkg	*/
#define H5S_PACKAGE		/*suppress error about including H5Spkg	*/

/* Private header files */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Apkg.h"		/* Attributes				*/
#include "H5Bprivate.h"		/* B-tree subclass names	  	*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Oprivate.h"     	/* Object Headers       		*/
#include "H5Pprivate.h"		/* Property lists			*/
#include "H5Spkg.h"		/* Dataspace functions			*/
#include "H5Tprivate.h"		/* Datatypes				*/

#define PABLO_MASK	H5A_mask

/* Is the interface initialized? */
static int		interface_initialize_g = 0;
#define INTERFACE_INIT	H5A_init_interface
static herr_t		H5A_init_interface(void);

/* PRIVATE PROTOTYPES */
static hid_t H5A_create(const H5G_entry_t *ent, const char *name,
			const H5T_t *type, const H5S_t *space, hid_t dxpl_id);
static hid_t H5A_open(H5G_entry_t *ent, unsigned idx, hid_t dxpl_id);
static herr_t H5A_write(H5A_t *attr, const H5T_t *mem_type, const void *buf, hid_t dxpl_id);
static herr_t H5A_read(H5A_t *attr, const H5T_t *mem_type, void *buf, hid_t dxpl_id);
static int H5A_get_index(H5G_entry_t *ent, const char *name, hid_t dxpl_id);
static hsize_t H5A_get_storage_size(H5A_t *attr);
static herr_t H5A_rename(H5G_entry_t *ent, const char *old_name, const char *new_name, hid_t dxpl_id);

/* The number of reserved IDs in dataset ID group */
#define H5A_RESERVED_ATOMS  0


/*--------------------------------------------------------------------------
NAME
   H5A_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5A_init_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5A_init_interface(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5A_init_interface);

    /*
     * Create attribute group.
     */
    if (H5I_init_group(H5I_ATTR, H5I_ATTRID_HASHSIZE, H5A_RESERVED_ATOMS, (H5I_free_t)H5A_close)<0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL, "unable to initialize interface");
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5A_term_interface
 PURPOSE
    Terminate various H5A objects
 USAGE
    void H5A_term_interface()
 RETURNS
 DESCRIPTION
    Release any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5A_term_interface(void)
{
    int	n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5A_term_interface);
    
    if (interface_initialize_g) {
	if ((n=H5I_nmembers(H5I_ATTR))) {
	    H5I_clear_group(H5I_ATTR, FALSE);
	} else {
	    H5I_destroy_group(H5I_ATTR);
	    interface_initialize_g = 0;
	    n = 1;
	}
    }
    FUNC_LEAVE_NOAPI(n);
}


/*--------------------------------------------------------------------------
 NAME
    H5Acreate
 PURPOSE
    Creates a dataset as an attribute of another dataset or group
 USAGE
    hid_t H5Acreate (loc_id, name, type_id, space_id, plist_id)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        const char *name;   IN: Name of attribute to create
        hid_t type_id;      IN: ID of datatype for attribute
        hid_t space_id;     IN: ID of dataspace for attribute
        hid_t plist_id;     IN: ID of creation property list (currently not used)
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
        This function creates an attribute which is attached to the object
    specified with 'location_id'.  The name specified with 'name' for each
    attribute for an object must be unique for that object.  The 'type_id'
    and 'space_id' are created with the H5T and H5S interfaces respectively.
    Currently only simple dataspaces are allowed for attribute dataspaces.
    The 'plist_id' property list is currently un-used, but will be
    used int the future for optional properties of attributes.  The attribute
    ID returned from this function must be released with H5Aclose or resource
    leaks will develop.
        The link created (see H5G API documentation for more information on
    link types) is a hard link, so the attribute may be shared among datasets
    and will not be removed from the file until the reference count for the
    attribute is reduced to zero.
        The location object may be either a group or a dataset, both of
    which may have any sort of attribute.
 *
 * Modifications:
 * 	Robb Matzke, 5 Jun 1998
 *	The LOC_ID can also be a committed data type.
 *	
--------------------------------------------------------------------------*/
hid_t
H5Acreate(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id,
	  hid_t UNUSED plist_id)
{
    H5G_entry_t    	*ent = NULL;
    H5T_t		*type = NULL;
    H5S_t		*space = NULL;
    hid_t		ret_value = FAIL;

    FUNC_ENTER_API(H5Acreate, FAIL);
    H5TRACE5("i","isiii",loc_id,name,type_id,space_id,plist_id);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if (NULL==(ent=H5G_loc(loc_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if (!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    if (NULL == (type = H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a type");
    if (NULL == (space = H5I_object_verify(space_id, H5I_DATASPACE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");

    /* Go do the real work for attaching the attribute to the dataset */
    if ((ret_value=H5A_create(ent,name,type,space, H5AC_dxpl_id))<0)
	HGOTO_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL, "unable to create attribute");

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Acreate() */


/*-------------------------------------------------------------------------
 * Function:	H5A_create
 *
 * Purpose:	
 *      This is the guts of the H5Acreate function.
 * Usage:
 *  hid_t H5A_create (ent, name, type, space)
 *      const H5G_entry_t *ent;   IN: Pointer to symbol table entry for object to attribute
 *      const char *name;   IN: Name of attribute
 *      H5T_t *type;        IN: Datatype of attribute
 *      H5S_t *space;       IN: Dataspace of attribute
 *
 * Return: Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		April 2, 1998
 *
 * Modifications:
 *
 *	 Pedro Vicente, <pvn@ncsa.uiuc.edu> 22 Aug 2002
 *	 Added a deep copy of the symbol table entry
 *
 *-------------------------------------------------------------------------
 */
static hid_t
H5A_create(const H5G_entry_t *ent, const char *name, const H5T_t *type,
	   const H5S_t *space, hid_t dxpl_id)
{
    H5A_t	*attr = NULL;
    H5A_t	found_attr;
    int	seq=0;
    hid_t	ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT(H5A_create);

    /* check args */
    assert(ent);
    assert(name);
    assert(type);
    assert(space);

    /* Build the attribute information */
    if((attr = H5MM_calloc(sizeof(H5A_t)))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for attribute info");

    /* Copy the attribute name */
    attr->name=HDstrdup(name);

    /* Copy the attribute's datatype */
    attr->dt=H5T_copy(type, H5T_COPY_ALL);

    /* Mark any VL datatypes as being on disk now */
    if (H5T_vlen_mark(attr->dt, ent->file, H5T_VLEN_DISK)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");

    /* Copy the dataspace for the attribute */
    attr->ds=H5S_copy(space);

    /* Mark it initially set to initialized */
    attr->initialized = TRUE; /*for now, set to false later*/

    /* Deep copy of the symbol table entry */
    if (H5G_ent_copy(&(attr->ent),ent,H5G_COPY_DEEP)<0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to copy entry");

    /* Compute the size of pieces on disk */
    if(H5T_committed(attr->dt)) {
        H5O_shared_t	sh_mesg;

        /* Reset shared message information */
        HDmemset(&sh_mesg,0,sizeof(H5O_shared_t));

        /* Get shared message information for datatype */
        if (H5O_get_share(H5O_DTYPE_ID,attr->ent.file, type, &sh_mesg/*out*/)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to copy entry")

        /* Compute shared message size for datatype */
        attr->dt_size=H5O_raw_size(H5O_SHARED_ID,attr->ent.file,&sh_mesg);
    } /* end if */
    else
        attr->dt_size=H5O_raw_size(H5O_DTYPE_ID,attr->ent.file,type);
    assert(attr->dt_size>0);
    attr->ds_size=H5O_raw_size(H5O_SDSPACE_ID,attr->ent.file,&(space->extent.u.simple));
    assert(attr->ds_size>0);
    H5_ASSIGN_OVERFLOW(attr->data_size,H5S_get_simple_extent_npoints(attr->ds)*H5T_get_size(attr->dt),hssize_t,size_t);

    /* Hold the symbol table entry (and file) open */
    if (H5O_open(&(attr->ent)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open");
    attr->ent_opened=1;

    /* Read in the existing attributes to check for duplicates */
    seq=0;
    while(H5O_read(&(attr->ent), H5O_ATTR_ID, seq, &found_attr, dxpl_id)!=NULL) {
        /*
	 * Compare found attribute name to new attribute name reject creation
	 * if names are the same.
	 */
	if(HDstrcmp(found_attr.name,attr->name)==0) {
	    H5O_reset (H5O_ATTR_ID, &found_attr);
	    HGOTO_ERROR(H5E_ATTR, H5E_ALREADYEXISTS, FAIL, "attribute already exists");
	}
	H5O_reset (H5O_ATTR_ID, &found_attr);
	seq++;
    }
    H5E_clear ();

    /* Create the attribute message and save the attribute index */
    if (H5O_modify(&(attr->ent), H5O_ATTR_ID, H5O_NEW_MESG, 0, 1, attr, dxpl_id) < 0) 
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to update attribute header messages");

    /* Register the new attribute and get an ID for it */
    if ((ret_value = H5I_register(H5I_ATTR, attr)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID");

    /* Now it's safe to say it's uninitialized */
    attr->initialized = FALSE;

done:
    if (ret_value < 0) {
        if(attr)
            H5A_close(attr);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5A_create() */


/*--------------------------------------------------------------------------
 NAME
    H5A_get_index
 PURPOSE
    Determine the index of an attribute in an object header
 USAGE
    int H5A_get_index (ent, name)
        H5G_entry_t    *ent; IN: Symbol table entry of object
        const char *name;    IN: Name of dataset to find
 RETURNS
    non-negative on success, negative on failure
 
 ERRORS

 DESCRIPTION
        This function determines the index of the attribute within an object
    header.  This is not stored in the attribute structure because it is only
    a subjective measure and can change if attributes are deleted from the
    object header.
--------------------------------------------------------------------------*/
static int
H5A_get_index(H5G_entry_t *ent, const char *name, hid_t dxpl_id)
{
    H5A_t      	found_attr;
    int		i;                      /* Index variable */
    int		ret_value=FAIL;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5A_get_index);

    assert(ent);
    assert(name);

    /* Look up the attribute for the object */
    i=0;
    while(H5O_read(ent, H5O_ATTR_ID, i, &found_attr, dxpl_id)!=NULL) {
	/*
	 * Compare found attribute name to new attribute name reject creation
	 * if names are the same.
	 */
	if(HDstrcmp(found_attr.name,name)==0) {
	    H5O_reset (H5O_ATTR_ID, &found_attr);
	    ret_value = i;
	    break;
	}
	H5O_reset (H5O_ATTR_ID, &found_attr);
	i++;
    }
    H5E_clear ();
    
    if(ret_value<0)
        HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "attribute not found");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5A_get_index() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen_name
 PURPOSE
    Opens an attribute for an object by looking up the attribute name
 USAGE
    hid_t H5Aopen_name (loc_id, name)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        const char *name;   IN: Name of attribute to locate and open
 RETURNS
    ID of attribute on success, negative on failure
 
 ERRORS

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    name specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
        The location object may be either a group or a dataset, both of
    which may have any sort of attribute.
 *
 * Modifications:
 * 	Robb Matzke, 5 Jun 1998
 *	The LOC_ID can also be a named (committed) data type.
--------------------------------------------------------------------------*/
hid_t
H5Aopen_name(hid_t loc_id, const char *name)
{
    H5G_entry_t    	*ent = NULL;   /*Symtab entry of object to attribute*/
    int            	idx=0;
    hid_t		ret_value;

    FUNC_ENTER_API(H5Aopen_name, FAIL);
    H5TRACE2("i","is",loc_id,name);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if (NULL==(ent=H5G_loc(loc_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if (!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");

    /* Look up the attribute for the object */
    if((idx=H5A_get_index(ent,name, H5AC_dxpl_id))<0)
        HGOTO_ERROR(H5E_ATTR, H5E_BADVALUE, FAIL, "attribute not found");

    /* Go do the real work for opening the attribute */
    if ((ret_value=H5A_open(ent, (unsigned)idx, H5AC_dxpl_id))<0)
	HGOTO_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL, "unable to open attribute");
    
done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aopen_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen_idx
 PURPOSE
    Opens the n'th attribute for an object
 USAGE
    hid_t H5Aopen_idx (loc_id, idx)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        unsigned idx;       IN: Index (0-based) attribute to open
 RETURNS
    ID of attribute on success, negative on failure
 
 ERRORS

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    index specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
        The location object may be either a group or a dataset, both of
    which may have any sort of attribute.
 *
 * Modifications:
 * 	Robb Matzke, 5 Jun 1998
 *	The LOC_ID can also be a named (committed) data type.
 *	
--------------------------------------------------------------------------*/
hid_t
H5Aopen_idx(hid_t loc_id, unsigned idx)
{
    H5G_entry_t	*ent = NULL;	/*Symtab entry of object to attribute */
    hid_t	ret_value;

    FUNC_ENTER_API(H5Aopen_idx, FAIL);
    H5TRACE2("i","iIu",loc_id,idx);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if (NULL==(ent=H5G_loc(loc_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");

    /* Go do the real work for opening the attribute */
    if ((ret_value=H5A_open(ent, idx, H5AC_dxpl_id))<0)
	HGOTO_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL, "unable to open attribute");
    
done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aopen_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5A_open
 *
 * Purpose:	
 *      This is the guts of the H5Aopen_xxx functions
 * Usage:
 *  herr_t H5A_open (ent, idx)
 *      const H5G_entry_t *ent;   IN: Pointer to symbol table entry for object to attribute
 *      unsigned idx;       IN: index of attribute to open (0-based)
 *
 * Return: Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		April 2, 1998
 *
 * Modifications:
 *
 *	 Pedro Vicente, <pvn@ncsa.uiuc.edu> 22 Aug 2002
 *	 Added a deep copy of the symbol table entry
 *
 *-------------------------------------------------------------------------
 */
static hid_t
H5A_open(H5G_entry_t *ent, unsigned idx, hid_t dxpl_id)
{
    H5A_t       *attr = NULL;
    hid_t	    ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT(H5A_open);

    /* check args */
    assert(ent);

    /* Read in attribute with H5O_read() */
    H5_CHECK_OVERFLOW(idx,unsigned,int);
    if (NULL==(attr=H5O_read(ent, H5O_ATTR_ID, (int)idx, attr, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to load attribute info from dataset header");
    attr->initialized=1;
   
    /* Deep copy of the symbol table entry */
    if (H5G_ent_copy(&(attr->ent),ent,H5G_COPY_DEEP)<0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to copy entry");

    /* Hold the symbol table entry (and file) open */
    if (H5O_open(&(attr->ent)) < 0) {
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open");
    }
    attr->ent_opened=1;

    /* Register the new attribute and get an ID for it */
    if ((ret_value = H5I_register(H5I_ATTR, attr)) < 0) {
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
		    "unable to register attribute for ID");
    }

done:
    if (ret_value < 0) {
        if(attr) H5A_close(attr);
    }

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5A_open() */


/*--------------------------------------------------------------------------
 NAME
    H5Awrite
 PURPOSE
    Write out data to an attribute
 USAGE
    herr_t H5Awrite (attr_id, type_id, buf)
        hid_t attr_id;       IN: Attribute to write
        hid_t type_id;       IN: Memory datatype of buffer
        const void *buf;     IN: Buffer of data to write
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
        This function writes a complete attribute to disk.
--------------------------------------------------------------------------*/
herr_t
H5Awrite(hid_t attr_id, hid_t type_id, const void *buf)
{
    H5A_t		   *attr = NULL;
    const H5T_t    *mem_type = NULL;
    herr_t		    ret_value;

    FUNC_ENTER_API(H5Awrite, FAIL);
    H5TRACE3("e","iix",attr_id,type_id,buf);

    /* check arguments */
    if (NULL == (attr = H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    if (NULL == (mem_type = H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null attribute buffer");

    /* Go write the actual data to the attribute */
    if ((ret_value=H5A_write(attr,mem_type,buf, H5AC_dxpl_id))<0)
        HGOTO_ERROR(H5E_ATTR, H5E_WRITEERROR, FAIL, "unable to write attribute");

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Awrite() */


/*--------------------------------------------------------------------------
 NAME
    H5A_write
 PURPOSE
    Actually write out data to an attribute
 USAGE
    herr_t H5A_write (attr, mem_type, buf)
        H5A_t *attr;         IN: Attribute to write
        const H5T_t *mem_type;     IN: Memory datatype of buffer
        const void *buf;           IN: Buffer of data to write
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
    This function writes a complete attribute to disk.
--------------------------------------------------------------------------*/
static herr_t
H5A_write(H5A_t *attr, const H5T_t *mem_type, const void *buf, hid_t dxpl_id)
{
    uint8_t		*tconv_buf = NULL;	/* data type conv buffer */
    uint8_t		*bkg_buf = NULL;	/* temp conversion buffer */
    hsize_t		nelmts;		    	/* elements in attribute */
    H5T_path_t		*tpath = NULL;		/* conversion information*/
    hid_t		src_id = -1, dst_id = -1;/* temporary type atoms */
    size_t		src_type_size;		/* size of source type	*/
    size_t		dst_type_size;		/* size of destination type*/
    size_t		buf_size;		/* desired buffer size	*/
    int         idx;	      /* index of attribute in object header */
    herr_t		ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT(H5A_write);

    assert(attr);
    assert(mem_type);
    assert(buf);

    /* Create buffer for data to store on disk */
    nelmts=H5S_get_simple_extent_npoints (attr->ds);

    /* Get the memory and file datatype sizes */
    src_type_size = H5T_get_size(mem_type);
    dst_type_size = H5T_get_size(attr->dt);

    /* Get the maximum buffer size needed and allocate it */
    H5_ASSIGN_OVERFLOW(buf_size,nelmts*MAX(src_type_size,dst_type_size),hsize_t,size_t);
    if (NULL==(tconv_buf = H5MM_malloc (buf_size)) || NULL==(bkg_buf = H5MM_calloc(buf_size)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

    /* Copy the user's data into the buffer for conversion */
    H5_CHECK_OVERFLOW((src_type_size*nelmts),hsize_t,size_t);
    HDmemcpy(tconv_buf,buf,(size_t)(src_type_size*nelmts));

    /* Convert memory buffer into disk buffer */
    /* Set up type conversion function */
    if (NULL == (tpath = H5T_path_find(mem_type, attr->dt, NULL, NULL, dxpl_id))) {
        HGOTO_ERROR(H5E_ATTR, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest data types");
    } else if (!H5T_path_noop(tpath)) {
        if ((src_id = H5I_register(H5I_DATATYPE, H5T_copy(mem_type, H5T_COPY_ALL)))<0 ||
                (dst_id = H5I_register(H5I_DATATYPE, H5T_copy(attr->dt, H5T_COPY_ALL)))<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTREGISTER, FAIL, "unable to register types for conversion");
    }

    /* Perform data type conversion */
    if (H5T_convert(tpath, src_id, dst_id, nelmts, 0, 0, tconv_buf, bkg_buf,
                    dxpl_id)<0) {
        HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL,
		    "data type conversion failed");
    }

    /* Free the previous attribute data buffer, if there is one */
    if(attr->data)
        H5MM_xfree(attr->data);

    /* Look up the attribute for the object */
    if((idx=H5A_get_index(&(attr->ent),attr->name,dxpl_id))<0)
        HGOTO_ERROR(H5E_ATTR, H5E_BADVALUE, FAIL, "attribute not found");

    /* Modify the attribute data */
    attr->data=tconv_buf;   /* Set the data pointer temporarily */
    if (H5O_modify(&(attr->ent), H5O_ATTR_ID, idx, 0, 1, attr, dxpl_id) < 0) 
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL,
		    "unable to update attribute header messages");

    /* Indicate the the attribute doesn't need fill-values */
    attr->initialized=TRUE;

    ret_value=SUCCEED;

done:
    /* Release resources */
    if (src_id >= 0) 
        H5I_dec_ref(src_id);
    if (dst_id >= 0) 
        H5I_dec_ref(dst_id);
    if (bkg_buf)
        H5MM_xfree(bkg_buf);

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5A_write() */


/*--------------------------------------------------------------------------
 NAME
    H5Aread
 PURPOSE
    Read in data from an attribute
 USAGE
    herr_t H5Aread (attr_id, type_id, buf)
        hid_t attr_id;       IN: Attribute to read
        hid_t type_id;       IN: Memory datatype of buffer
        void *buf;           IN: Buffer for data to read
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
        This function reads a complete attribute from disk.
--------------------------------------------------------------------------*/
herr_t
H5Aread(hid_t attr_id, hid_t type_id, void *buf)
{
    H5A_t		*attr = NULL;
    const H5T_t    	*mem_type = NULL;
    herr_t		ret_value;

    FUNC_ENTER_API(H5Aread, FAIL);
    H5TRACE3("e","iix",attr_id,type_id,buf);

    /* check arguments */
    if (NULL == (attr = H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    if (NULL == (mem_type = H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    if (NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null attribute buffer");

    /* Go write the actual data to the attribute */
    if ((ret_value=H5A_read(attr,mem_type,buf,H5AC_dxpl_id))<0)
        HGOTO_ERROR(H5E_ATTR, H5E_READERROR, FAIL, "unable to read attribute");

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aread() */


/*--------------------------------------------------------------------------
 NAME
    H5A_read
 PURPOSE
    Actually read in data from an attribute
 USAGE
    herr_t H5A_read (attr, mem_type, buf)
        H5A_t *attr;         IN: Attribute to read
        const H5T_t *mem_type;     IN: Memory datatype of buffer
        void *buf;           IN: Buffer for data to read
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
    This function reads a complete attribute from disk.
--------------------------------------------------------------------------*/
static herr_t
H5A_read(H5A_t *attr, const H5T_t *mem_type, void *buf, hid_t dxpl_id)
{
    uint8_t		*tconv_buf = NULL;	/* data type conv buffer*/
    uint8_t		*bkg_buf = NULL;	/* background buffer */
    hsize_t		nelmts;			/* elements in attribute*/
    H5T_path_t		*tpath = NULL;		/* type conversion info	*/
    hid_t		src_id = -1, dst_id = -1;/* temporary type atoms*/
    size_t		src_type_size;		/* size of source type 	*/
    size_t		dst_type_size;		/* size of destination type */
    size_t		buf_size;		/* desired buffer size	*/
    herr_t		ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT(H5A_read);

    assert(attr);
    assert(mem_type);
    assert(buf);

    /* Create buffer for data to store on disk */
    nelmts=H5S_get_simple_extent_npoints (attr->ds);

    /* Get the memory and file datatype sizes */
    src_type_size = H5T_get_size(attr->dt);
    dst_type_size = H5T_get_size(mem_type);

    /* Check if the attribute has any data yet, if not, fill with zeroes */
    H5_CHECK_OVERFLOW((dst_type_size*nelmts),hsize_t,size_t);
    if(attr->ent_opened && !attr->initialized) {
        HDmemset(buf,0,(size_t)(dst_type_size*nelmts));
    }   /* end if */
    else {  /* Attribute exists and has a value */
        /* Get the maximum buffer size needed and allocate it */
        H5_ASSIGN_OVERFLOW(buf_size,(nelmts*MAX(src_type_size,dst_type_size)),hsize_t,size_t);
        if (NULL==(tconv_buf = H5MM_malloc (buf_size)) || NULL==(bkg_buf = H5MM_calloc(buf_size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

        /* Copy the attribute data into the buffer for conversion */
        H5_CHECK_OVERFLOW((src_type_size*nelmts),hsize_t,size_t);
        HDmemcpy(tconv_buf,attr->data,(size_t)(src_type_size*nelmts));

        /* Convert memory buffer into disk buffer */
        /* Set up type conversion function */
        if (NULL == (tpath = H5T_path_find(attr->dt, mem_type, NULL, NULL, dxpl_id))) {
            HGOTO_ERROR(H5E_ATTR, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest data types");
        } else if (!H5T_path_noop(tpath)) {
            if ((src_id = H5I_register(H5I_DATATYPE, H5T_copy(attr->dt, H5T_COPY_ALL)))<0 ||
                    (dst_id = H5I_register(H5I_DATATYPE, H5T_copy(mem_type, H5T_COPY_ALL)))<0)
                HGOTO_ERROR(H5E_ATTR, H5E_CANTREGISTER, FAIL, "unable to register types for conversion");
        }

        /* Perform data type conversion.  */
        if (H5T_convert(tpath, src_id, dst_id, nelmts, 0, 0, tconv_buf, bkg_buf, dxpl_id)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "data type conversion failed");

        /* Copy the converted data into the user's buffer */
        HDmemcpy(buf,tconv_buf,(size_t)(dst_type_size*nelmts));
    } /* end else */

    ret_value=SUCCEED;

done:
    /* Release resources */
    if (src_id >= 0) 
        H5I_dec_ref(src_id);
    if (dst_id >= 0) 
        H5I_dec_ref(dst_id);
    if (tconv_buf)
        H5MM_xfree(tconv_buf);
    if (bkg_buf)
	H5MM_xfree(bkg_buf);

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5A_read() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_space
 PURPOSE
    Gets a copy of the dataspace for an attribute
 USAGE
    hid_t H5Aget_space (attr_id)
        hid_t attr_id;       IN: Attribute to get dataspace of
 RETURNS
    A dataspace ID on success, negative on failure
 
 ERRORS

 DESCRIPTION
        This function retrieves a copy of the dataspace for an attribute.
    The dataspace ID returned from this function must be released with H5Sclose
    or resource leaks will develop.
--------------------------------------------------------------------------*/
hid_t
H5Aget_space(hid_t attr_id)
{
    H5A_t	*attr = NULL;
    H5S_t	*dst = NULL;
    hid_t	ret_value;

    FUNC_ENTER_API(H5Aget_space, FAIL);
    H5TRACE1("i","i",attr_id);

    /* check arguments */
    if (NULL == (attr = H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");

    /* Copy the attribute's dataspace */
    if (NULL==(dst=H5S_copy (attr->ds)))
	HGOTO_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL, "unable to copy dataspace");

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, dst))<0)
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace atom");

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aget_space() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_type
 PURPOSE
    Gets a copy of the datatype for an attribute
 USAGE
    hid_t H5Aget_type (attr_id)
        hid_t attr_id;       IN: Attribute to get datatype of
 RETURNS
    A datatype ID on success, negative on failure
 
 ERRORS

 DESCRIPTION
        This function retrieves a copy of the datatype for an attribute.
    The datatype ID returned from this function must be released with H5Tclose
    or resource leaks will develop.
 *
 * Modifications:
 * 	Robb Matzke, 4 Jun 1998
 *	The data type is reopened if it's a named type before returning it to
 *	the application.  The data types returned by this function are always
 *	read-only. If an error occurs when atomizing the return data type
 *	then the data type is closed.
--------------------------------------------------------------------------*/
hid_t
H5Aget_type(hid_t attr_id)
{
    H5A_t	*attr = NULL;
    H5T_t	*dst = NULL;
    hid_t	 ret_value;

    FUNC_ENTER_API(H5Aget_type, FAIL);
    H5TRACE1("i","i",attr_id);

    /* check arguments */
    if (NULL == (attr = H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");

    /*
     * Copy the attribute's data type.  If the type is a named type then
     * reopen the type before returning it to the user. Make the type
     * read-only.
     */
    if (NULL==(dst=H5T_copy(attr->dt, H5T_COPY_REOPEN)))
	HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to copy datatype");

    /* Mark any VL datatypes as being in memory now */
    if (H5T_vlen_mark(dst, NULL, H5T_VLEN_MEMORY)<0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");
    if (H5T_lock(dst, FALSE)<0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to lock transient data type");
    
    /* Atomize */
    if ((ret_value=H5I_register(H5I_DATATYPE, dst))<0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register datatype atom");

done:
    if(ret_value<0) {
        if(dst!=NULL)
            H5T_close(dst);
    } /* end if */

    FUNC_LEAVE_API(ret_value);
} /* H5Aget_type() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_name
 PURPOSE
    Gets a copy of the name for an attribute
 USAGE
    hssize_t H5Aget_name (attr_id, buf_size, buf)
        hid_t attr_id;      IN: Attribute to get name of
        size_t buf_size;    IN: The size of the buffer to store the string in.
        char *buf;          IN: Buffer to store name in
 RETURNS
    This function returns the length of the attribute's name (which may be
    longer than 'buf_size') on success or negative for failure.
 
 ERRORS

 DESCRIPTION
        This function retrieves the name of an attribute for an attribute ID.
    Up to 'buf_size' characters are stored in 'buf' followed by a '\0' string
    terminator.  If the name of the attribute is longer than 'buf_size'-1,
    the string terminator is stored in the last position of the buffer to
    properly terminate the string.
--------------------------------------------------------------------------*/
ssize_t
H5Aget_name(hid_t attr_id, size_t buf_size, char *buf)
{
    H5A_t		*attr = NULL;
    size_t              copy_len, nbytes;
    ssize_t		ret_value;

    FUNC_ENTER_API(H5Aget_name, FAIL);
    H5TRACE3("Zs","izs",attr_id,buf_size,buf);

    /* check arguments */
    if (NULL == (attr = H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    if (!buf && buf_size)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer");

    /* get the real attribute length */
    nbytes = HDstrlen(attr->name);
    assert((ssize_t)nbytes>=0); /*overflow, pretty unlikey --rpm*/

    /* compute the string length which will fit into the user's buffer */
    copy_len = MIN(buf_size-1, nbytes);

    /* Copy all/some of the name */
    if(buf && copy_len>0) {
        HDmemcpy(buf, attr->name, copy_len);

        /* Terminate the string */
        buf[copy_len]='\0';
    }

    /* Set return value */
    ret_value = (ssize_t)nbytes;

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aget_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Aget_storage_size
 *
 * Purpose:	Returns the amount of storage size that is required for this
 *		attribute. 
 *
 * Return:	Success:	The amount of storage size allocated for the
 *				attribute.  The return value may be zero 
 *                              if no data has been stored.
 *
 *		Failure:	Zero
 *
 * Programmer:	Raymond Lu
 *              October 23, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5Aget_storage_size(hid_t attr_id)
{
    H5A_t	*attr=NULL;
    hsize_t	ret_value;      /* Return value */
    
    FUNC_ENTER_API(H5Aget_storage_size, 0);
    H5TRACE1("h","i",attr_id);

    /* Check args */
    if (NULL==(attr=H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not an attribute");

    /* Set return value */
    ret_value = H5A_get_storage_size(attr);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5A_get_storage_size
 *
 * Purpose:	Private function for H5Aget_storage_size.  Returns the 
 *              amount of storage size that is required for this
 *		attribute. 
 *
 * Return:	Success:	The amount of storage size allocated for the
 *				attribute.  The return value may be zero 
 *                              if no data has been stored.
 *
 *		Failure:	Zero
 *
 * Programmer:	Raymond Lu
 *              October 23, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5A_get_storage_size(H5A_t *attr)
{
    hsize_t	ret_value;      /* Return value */
    
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5A_get_storage_size);

    /* Set return value */
    ret_value = attr->data_size;

    FUNC_LEAVE_NOAPI(ret_value);
}
 

/*--------------------------------------------------------------------------
 NAME
    H5Aget_num_attrs
 PURPOSE
    Determines the number of attributes attached to an object
 USAGE
    int H5Aget_num_attrs (loc_id)
        hid_t loc_id;       IN: Object (dataset or group) to be queried
 RETURNS
    Number of attributes on success, negative on failure
 
 ERRORS

 DESCRIPTION
        This function returns the number of attributes attached to a dataset or
    group, 'location_id'.
 *
 * Modifications:
 * 	Robb Matzke, 5 Jun 1998
 *	The LOC_ID can also be a named (committed) data type.
--------------------------------------------------------------------------*/
int
H5Aget_num_attrs(hid_t loc_id)
{
    H5G_entry_t    	*ent = NULL;	/*symtab ent of object to attribute */
    void           	*obj = NULL;
    int			ret_value;

    FUNC_ENTER_API(H5Aget_num_attrs, FAIL);
    H5TRACE1("Is","i",loc_id);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if(NULL == (obj = H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADATOM, FAIL, "illegal object atom");
    switch (H5I_get_type (loc_id)) {
        case H5I_DATASET:
            ent = H5D_entof ((H5D_t*)obj);
            break;
        case H5I_DATATYPE:
            if (NULL==(ent=H5T_entof ((H5T_t*)obj)))
                HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "target data type is not committed");
            break;
        case H5I_GROUP:
            ent = H5G_entof ((H5G_t*)obj);
            break;
        default:
            HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "inappropriate attribute target");
    }

    /* Look up the attribute for the object */
    ret_value=H5O_count(ent, H5O_ATTR_ID, H5AC_ind_dxpl_id);

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aget_num_attrs() */


/*-------------------------------------------------------------------------
 * Function:	H5Arename
 *
 * Purpose:     Rename an attribute	
 *
 * Return:	Success:             Non-negative	
 *
 *		Failure:             Negative
 *
 * Programmer:	Raymond Lu
 *              October 23, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Arename(hid_t loc_id, const char *old_name, const char *new_name)
{
    H5G_entry_t	*ent = NULL;	/*symtab ent of object to attribute */
    herr_t	ret_value;      /* Return value */
    
    FUNC_ENTER_API(H5Arename, FAIL);
    H5TRACE3("e","iss",loc_id,old_name,new_name);

    /* check arguments */
    if (!old_name || !new_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "name is nil");
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if (NULL==(ent=H5G_loc(loc_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");

    /* Call private function */
    ret_value = H5A_rename(ent, old_name, new_name, H5AC_dxpl_id);

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Arename() */


/*-------------------------------------------------------------------------
 * Function:	H5A_rename
 *
 * Purpose:     Private function for H5Arename.  Rename an attribute	
 *
 * Return:	Success:             Non-negative	
 *
 *		Failure:             Negative
 *
 * Programmer:	Raymond Lu
 *              October 23, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5A_rename(H5G_entry_t *ent, const char *old_name, const char *new_name, hid_t dxpl_id)
{
    int         seq, idx=FAIL;  /* Index of attribute being querried */
    H5A_t       *found_attr;    /* Attribute with OLD_NAME */
    herr_t	ret_value=SUCCEED;      /* Return value */
    
    FUNC_ENTER_NOAPI_NOINIT(H5A_rename);

    /* Check arguments */
    assert(ent);
    assert(old_name);
    assert(new_name);
    
    /* Build the attribute information */
    if((found_attr = HDcalloc(1, sizeof(H5A_t)))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for attribute info");
        
    /* Read in the existing attributes to check for duplicates */
    seq=0;
    while(H5O_read(ent, H5O_ATTR_ID, seq, found_attr, dxpl_id)!=NULL) {
        /*
	 * Compare found attribute name.
	 */
	if(HDstrcmp(found_attr->name,old_name)==0) {
            idx = seq;
            break;
	}
	H5O_reset (H5O_ATTR_ID, found_attr);
	seq++;
    }
 
    if(idx<0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "attribute cannot be found");
        
    /* Copy the attribute name. */
    if(found_attr->name)
        HDfree(found_attr->name);
    found_attr->name = HDstrdup(new_name); 
    if(!found_attr->name) 
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "String copy failed");

    /* Indicate entry is not opened and the attribute doesn't need fill-values. */
    found_attr->ent_opened=FALSE;
    found_attr->initialized=TRUE;

    /* Modify the attribute message */
    if (H5O_modify(ent, H5O_ATTR_ID, idx, 0, 1, found_attr, dxpl_id) < 0) 
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to update attribute header messages");
   
    /* Close the attribute */
    H5A_close(found_attr);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Aiterate
 PURPOSE
    Calls a user's function for each attribute on an object
 USAGE
    herr_t H5Aiterate (loc_id, attr_num, op, data)
        hid_t loc_id;       IN: Object (dataset or group) to be iterated over
        unsigned *attr_num; IN/OUT: Starting (IN) & Ending (OUT) attribute number
        H5A_operator_t op;  IN: User's function to pass each attribute to
        void *op_data;      IN/OUT: User's data to pass through to iterator operator function
 RETURNS
        Returns a negative value if something is wrong, the return value of the
    last operator if it was non-zero, or zero if all attributes were processed.
 
 ERRORS

 DESCRIPTION
        This function interates over the attributes of dataset or group
    specified with 'loc_id'.  For each attribute of the object, the
    'op_data' and some additional information (specified below) are passed
    to the 'op' function.  The iteration begins with the '*attr_number'
    object in the group and the next attribute to be processed by the operator
    is returned in '*attr_number'.
        The operation receives the ID for the group or dataset being iterated
    over ('loc_id'), the name of the current attribute about the object
    ('attr_name') and the pointer to the operator data passed in to H5Aiterate
    ('op_data').  The return values from an operator are:
        A. Zero causes the iterator to continue, returning zero when all 
            attributes have been processed.
        B. Positive causes the iterator to immediately return that positive
            value, indicating short-circuit success.  The iterator can be
            restarted at the next attribute.
        C. Negative causes the iterator to immediately return that value,
            indicating failure.  The iterator can be restarted at the next
            attribute.
 *
 * Modifications:
 * 	Robb Matzke, 5 Jun 1998
 *	The LOC_ID can also be a named (committed) data type.
 *
 * 	Robb Matzke, 5 Jun 1998
 *	Like the group iterator, if ATTR_NUM is the null pointer then all
 *	attributes are processed.
 *	
--------------------------------------------------------------------------*/
herr_t
H5Aiterate(hid_t loc_id, unsigned *attr_num, H5A_operator_t op, void *op_data)
{
    H5G_entry_t		*ent = NULL;	/*symtab ent of object to attribute */
    H5A_t          	found_attr;
    herr_t	        ret_value = 0;
    int		idx, start_idx;

    FUNC_ENTER_API(H5Aiterate, FAIL);
    H5TRACE4("e","i*Iuxx",loc_id,attr_num,op,op_data);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if (NULL==(ent=H5G_loc(loc_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");

    /*
     * Look up the attribute for the object. Make certain the start point is
     * reasonable.
     */
    start_idx = idx = attr_num ? (int)*attr_num : 0;
    if (idx<0)
	HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index specified");
    if(idx<H5O_count(ent, H5O_ATTR_ID, H5AC_dxpl_id)) {
        while(H5O_read(ent, H5O_ATTR_ID, idx++, &found_attr, H5AC_dxpl_id)!=NULL) {
	    /*
	     * Compare found attribute name to new attribute name reject
	     * creation if names are the same.
	     */
	    if((ret_value=(op)(loc_id,found_attr.name,op_data))!=0) {
		H5O_reset (H5O_ATTR_ID, &found_attr);
		break;
	    }
	    H5O_reset (H5O_ATTR_ID, &found_attr);
	}
	H5E_clear ();
    }
    else
        if(start_idx>0)
            HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index specified");

    if (attr_num)
        *attr_num = (unsigned)idx;

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aiterate() */


/*--------------------------------------------------------------------------
 NAME
    H5Adelete
 PURPOSE
    Deletes an attribute from a location
 USAGE
    herr_t H5Adelete (loc_id, name)
        hid_t loc_id;       IN: Object (dataset or group) to have attribute deleted from
        const char *name;   IN: Name of attribute to delete
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
        This function removes the named attribute from a dataset or group.
    This function should not be used when attribute IDs are open on 'loc_id'
    as it may cause the internal indexes of the attributes to change and future 
    writes to the open attributes to produce incorrect results.
 *
 * Modifications:
 * 	Robb Matzke, 5 Jun 1998
 *	The LOC_ID can also be a named (committed) data type.
 *	
--------------------------------------------------------------------------*/
herr_t
H5Adelete(hid_t loc_id, const char *name)
{
    H5A_t       found_attr;
    H5G_entry_t	*ent = NULL;		/*symtab ent of object to attribute */
    int        idx=0, found=-1;
    herr_t	ret_value;

    FUNC_ENTER_API(H5Aopen_name, FAIL);
    H5TRACE2("e","is",loc_id,name);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) || H5I_ATTR==H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute");
    if (NULL==(ent=H5G_loc(loc_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if (!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");

    /* Look up the attribute for the object */
    idx=0;
    while(H5O_read(ent, H5O_ATTR_ID, idx, &found_attr, H5AC_dxpl_id)!=NULL) {
	/*
	 * Compare found attribute name to new attribute name reject
	 * creation if names are the same.
	 */
	if(HDstrcmp(found_attr.name,name)==0) {
	    H5O_reset (H5O_ATTR_ID, &found_attr);
	    found = idx;
	    break;
	}
	H5O_reset (H5O_ATTR_ID, &found_attr);
	idx++;
    }
    H5E_clear ();
    if (found<0)
        HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "attribute not found");

    /* Delete the attribute from the location */
    if ((ret_value=H5O_remove(ent, H5O_ATTR_ID, found, H5AC_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute header message");
    
done:
    FUNC_LEAVE_API(ret_value);
} /* H5Adelete() */


/*--------------------------------------------------------------------------
 NAME
    H5Aclose
 PURPOSE
    Close an attribute ID
 USAGE
    herr_t H5Aclose (attr_id)
        hid_t attr_id;       IN: Attribute to release access to
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
        This function releases an attribute from use.  Further use of the
    attribute ID will result in undefined behavior.
--------------------------------------------------------------------------*/
herr_t
H5Aclose(hid_t attr_id)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Aclose, FAIL);
    H5TRACE1("e","i",attr_id);

    /* check arguments */
    if (NULL == H5I_object_verify(attr_id, H5I_ATTR))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");

    /* Decrement references to that atom (and close it) */
    H5I_dec_ref (attr_id);

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Aclose() */


/*-------------------------------------------------------------------------
 * Function:	H5A_copy
 *
 * Purpose:	Copies attribute OLD_ATTR.
 *
 * Return:	Success:	Pointer to a new copy of the OLD_ATTR argument.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5A_t *
H5A_copy(const H5A_t *old_attr)
{
    H5A_t	*new_attr=NULL;
    H5A_t	*ret_value=NULL;        /* Return value */

    FUNC_ENTER_NOAPI(H5A_copy, NULL);

    /* check args */
    assert(old_attr);

    /* get space */
    if (NULL==(new_attr = H5MM_calloc(sizeof(H5A_t))))
	HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy the top level of the attribute */
    *new_attr = *old_attr;

    /* Don't open the object header for a copy */
    new_attr->ent_opened=0;

    /* Copy the guts of the attribute */
    new_attr->name=HDstrdup(old_attr->name);
    new_attr->dt=H5T_copy(old_attr->dt, H5T_COPY_ALL);
    new_attr->ds=H5S_copy(old_attr->ds);
    if(old_attr->data) {
        if (NULL==(new_attr->data=H5MM_malloc(old_attr->data_size)))
	    HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
        HDmemcpy(new_attr->data,old_attr->data,old_attr->data_size);
    } /* end if */

#ifndef LATER
    /* Copy the share info? */
#endif

    /* Set the return value */
    ret_value=new_attr;
    
done:
    if(ret_value==NULL) {
        if(new_attr!=NULL)
            H5A_close(new_attr);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5A_close
 *
 * Purpose:	Frees an attribute and all associated memory.  
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_close(H5A_t *attr)
{
    herr_t ret_value=SUCCEED;           /* Return value */

    FUNC_ENTER_NOAPI(H5A_close, FAIL);

    assert(attr);

    /* Check if the attribute has any data yet, if not, fill with zeroes */
    if(attr->ent_opened && !attr->initialized) {
        uint8_t *tmp_buf=H5MM_calloc(attr->data_size);
        if (NULL == tmp_buf)
            HGOTO_ERROR(H5E_ATTR, H5E_NOSPACE, FAIL, "memory allocation failed for attribute fill-value");

        /* Go write the fill data to the attribute */
        if (H5A_write(attr,attr->dt,tmp_buf,H5AC_dxpl_id)<0)
            HGOTO_ERROR(H5E_ATTR, H5E_WRITEERROR, FAIL, "unable to write attribute");

        /* Free temporary buffer */
        H5MM_xfree(tmp_buf);
    } /* end if */

    /* Free dynamicly allocated items */
    if(attr->name)
        H5MM_xfree(attr->name);
    if(attr->dt)
        H5T_close(attr->dt);
    if(attr->ds)
        H5S_close(attr->ds);
    if(attr->data)
        H5MM_xfree(attr->data);

    /* Close the object's symbol-table entry */
    if(attr->ent_opened)
        H5O_close(&(attr->ent));

#ifndef LATER
    /* Do something with the shared information? */
#endif

    H5MM_xfree(attr);
    
done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5A_entof
 *
 * Purpose:	Return the symbol table entry for an attribute.  It's the
 *		symbol table entry for the object to which the attribute
 *		belongs, not the attribute itself.
 *
 * Return:	Success:	Ptr to entry
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, August  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_entry_t *
H5A_entof(H5A_t *attr)
{
    H5G_entry_t *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(H5A_entof, NULL);
    assert(attr);

    /* Set return value */
    ret_value=&(attr->ent);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}
