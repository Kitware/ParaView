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

#define H5A_PACKAGE             /*suppress error about including H5Apkg */
#define H5S_PACKAGE             /*suppress error about including H5Spkg */

/* Private header files */
#include "H5private.h"          /* Generic Functions                    */
#include "H5Iprivate.h"         /* IDs                                  */
#include "H5Bprivate.h"         /* B-tree subclass names                */
#include "H5Dprivate.h"         /* Datasets                             */
#include "H5Gprivate.h"         /* Groups                               */
#include "H5Tprivate.h"         /* Datatypes                            */
#include "H5Eprivate.h"         /* Error handling                       */
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Oprivate.h"         /* Object Headers                       */
#include "H5Spkg.h"             /* Data-space functions                 */
#include "H5Apkg.h"             /* Attributes                           */

#define PABLO_MASK      H5A_mask

/* Is the interface initialized? */
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  H5A_init_interface
static herr_t           H5A_init_interface(void);

/* PRIVATE PROTOTYPES */
static hid_t H5A_create(const H5G_entry_t *ent, const char *name,
                        const H5T_t *type, const H5S_t *space);
static hid_t H5A_open(H5G_entry_t *ent, unsigned idx);
static herr_t H5A_write(H5A_t *attr, const H5T_t *mem_type, const void *buf);
static herr_t H5A_read(H5A_t *attr, const H5T_t *mem_type, void *buf);
static int H5A_get_index(H5G_entry_t *ent, const char *name);


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
    FUNC_ENTER(H5A_init_interface, FAIL);

    /*
     * Create attribute group.
     */
    if (H5I_init_group(H5I_ATTR, H5I_ATTRID_HASHSIZE, H5A_RESERVED_ATOMS,
                       (H5I_free_t)H5A_close)<0) {
        HRETURN_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL,
                      "unable to initialize interface");
    }
    
    FUNC_LEAVE(SUCCEED);
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
    int n=0;
    
    if (interface_initialize_g) {
        if ((n=H5I_nmembers(H5I_ATTR))) {
            H5I_clear_group(H5I_ATTR, FALSE);
        } else {
            H5I_destroy_group(H5I_ATTR);
            interface_initialize_g = 0;
            n = 1;
        }
    }
    return n;
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
 *      Robb Matzke, 5 Jun 1998
 *      The LOC_ID can also be a committed data type.
 *      
--------------------------------------------------------------------------*/
hid_t
H5Acreate(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id,
          hid_t plist_id)
{
    H5G_entry_t         *ent = NULL;
    H5T_t               *type = NULL;
    H5S_t               *space = NULL;
    hid_t               ret_value = FAIL;

    FUNC_ENTER(H5Acreate, FAIL);
    H5TRACE5("i","isiii",loc_id,name,type_id,space_id,plist_id);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) ||
        H5I_ATTR==H5I_get_type(loc_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "location is not valid for an attribute");
    }
    if (NULL==(ent=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (type = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a type");
    }
    if (H5I_DATASPACE != H5I_get_type(space_id) ||
        NULL == (space = H5I_object(space_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space");
    }
    if (H5P_DEFAULT!=plist_id &&
        (H5P_DATASET_CREATE != H5P_get_class(plist_id) ||
         NULL == H5I_object(plist_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not a dataset creation property list");
    }

    /* Go do the real work for attaching the attribute to the dataset */
    if ((ret_value=H5A_create(ent,name,type,space))<0) {
        HRETURN_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL,
                       "unable to create attribute");
    }

    FUNC_LEAVE(ret_value);
} /* H5Acreate() */


/*-------------------------------------------------------------------------
 * Function:    H5A_create
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
 * Programmer:  Quincey Koziol
 *              April 2, 1998
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
static hid_t
H5A_create(const H5G_entry_t *ent, const char *name, const H5T_t *type,
           const H5S_t *space)
{
    H5A_t       *attr = NULL;
    H5A_t       found_attr;
    int seq=0;
    hid_t       ret_value = FAIL;

    FUNC_ENTER(H5A_create, FAIL);

    /* check args */
    assert(ent);
    assert(name);
    assert(type);
    assert(space);

    /* Build the attribute information */
    if((attr = H5MM_calloc(sizeof(H5A_t)))==NULL)
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                      "memory allocation failed for attribute info");
    attr->name=HDstrdup(name);
    attr->dt=H5T_copy(type, H5T_COPY_ALL);
    /* Mark any VL datatypes as being on disk now */
    if (H5T_vlen_mark(attr->dt, ent->file, H5T_VLEN_DISK)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");
    }
    attr->ds=H5S_copy(space);
    attr->initialized = TRUE; /*for now, set to false later*/

    /* Copy the symbol table entry */
    attr->ent=*ent;

    /* Compute the internal sizes */
    attr->dt_size=(H5O_DTYPE[0].raw_size)(attr->ent.file,type);
    attr->ds_size=(H5O_SDSPACE[0].raw_size)(attr->ent.file,&(space->extent.u.simple));
    attr->data_size=H5S_get_simple_extent_npoints(attr->ds)*H5T_get_size(attr->dt);

    /* Hold the symbol table entry (and file) open */
    if (H5O_open(&(attr->ent)) < 0) {
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open");
    }
    attr->ent_opened=1;

    /* Read in the existing attributes to check for duplicates */
    seq=0;
    while(H5O_read(&(attr->ent), H5O_ATTR, seq, &found_attr)!=NULL) {
        /*
         * Compare found attribute name to new attribute name reject creation
         * if names are the same.
         */
        if(HDstrcmp(found_attr.name,attr->name)==0) {
            H5O_reset (H5O_ATTR, &found_attr);
            HGOTO_ERROR(H5E_ATTR, H5E_CANTCREATE, FAIL,
                        "attribute already exists");
        }
        H5O_reset (H5O_ATTR, &found_attr);
        seq++;
    }
    H5E_clear ();

    /* Create the attribute message and save the attribute index */
    if (H5O_modify(&(attr->ent), H5O_ATTR, H5O_NEW_MESG, 0, attr) < 0) 
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL,
                    "unable to update attribute header messages");

    /* Register the new attribute and get an ID for it */
    if ((ret_value = H5I_register(H5I_ATTR, attr)) < 0) {
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                    "unable to register attribute for ID");
    }

    /* Now it's safe to say it's uninitialized */
    attr->initialized = FALSE;

done:
    if (ret_value < 0) {
        if(attr) H5A_close(attr);
    }

    FUNC_LEAVE(ret_value);
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
H5A_get_index(H5G_entry_t *ent, const char *name)
{
    H5A_t       found_attr;
    int         ret_value=FAIL, i;

    FUNC_ENTER(H5A_get_index, FAIL);

    assert(ent);
    assert(name);

    /* Look up the attribute for the object */
    i=0;
    while(H5O_read(ent, H5O_ATTR, i, &found_attr)!=NULL) {
        /*
         * Compare found attribute name to new attribute name reject creation
         * if names are the same.
         */
        if(HDstrcmp(found_attr.name,name)==0) {
            H5O_reset (H5O_ATTR, &found_attr);
            ret_value = i;
            break;
        }
        H5O_reset (H5O_ATTR, &found_attr);
        i++;
    }
    H5E_clear ();
    
    if(ret_value<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL,
                      "attribute not found");
    }

    FUNC_LEAVE(ret_value);
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
 *      Robb Matzke, 5 Jun 1998
 *      The LOC_ID can also be a named (committed) data type.
--------------------------------------------------------------------------*/
hid_t
H5Aopen_name(hid_t loc_id, const char *name)
{
    H5G_entry_t         *ent = NULL;   /*Symtab entry of object to attribute*/
    int                 idx=0;
    hid_t               ret_value = FAIL;

    FUNC_ENTER(H5Aopen_name, FAIL);
    H5TRACE2("i","is",loc_id,name);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) ||
        H5I_ATTR==H5I_get_type(loc_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "location is not valid for an attribute");
    }
    if (NULL==(ent=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }

    /* Look up the attribute for the object */
    if((idx=H5A_get_index(ent,name))<0)
        HRETURN_ERROR(H5E_ATTR, H5E_BADVALUE, FAIL, "attribute not found");

    /* Go do the real work for opening the attribute */
    if ((ret_value=H5A_open(ent, (unsigned)idx))<0) {
        HRETURN_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL,
                       "unable to open attribute");
    }
    
    FUNC_LEAVE(ret_value);
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
 *      Robb Matzke, 5 Jun 1998
 *      The LOC_ID can also be a named (committed) data type.
 *      
--------------------------------------------------------------------------*/
hid_t
H5Aopen_idx(hid_t loc_id, unsigned idx)
{
    H5G_entry_t *ent = NULL;    /*Symtab entry of object to attribute */
    hid_t       ret_value = FAIL;

    FUNC_ENTER(H5Aopen_idx, FAIL);
    H5TRACE2("i","iIu",loc_id,idx);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) ||
        H5I_ATTR==H5I_get_type(loc_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "location is not valid for an attribute");
    }
    if (NULL==(ent=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }

    /* Go do the real work for opening the attribute */
    if ((ret_value=H5A_open(ent, idx))<0) {
        HRETURN_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL,
                       "unable to open attribute");
    }
    
    FUNC_LEAVE(ret_value);
} /* H5Aopen_idx() */


/*-------------------------------------------------------------------------
 * Function:    H5A_open
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
 * Programmer:  Quincey Koziol
 *              April 2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hid_t
H5A_open(H5G_entry_t *ent, unsigned idx)
{
    H5A_t       *attr = NULL;
    hid_t           ret_value = FAIL;

    FUNC_ENTER(H5A_open, FAIL);

    /* check args */
    assert(ent);

    /* Read in attribute with H5O_read() */
    H5_CHECK_OVERFLOW(idx,unsigned,int);
    if (NULL==(attr=H5O_read(ent, H5O_ATTR, (int)idx, attr))) {
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL,
                    "unable to load attribute info from dataset header");
    }
    attr->initialized=1;

    /* Copy the symbol table entry */
    attr->ent=*ent;

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

    FUNC_LEAVE(ret_value);
} /* H5A_open() */


/*--------------------------------------------------------------------------
 NAME
    H5Awrite
 PURPOSE
    Write out data to an attribute
 USAGE
    herr_t H5Awrite (attr_id, type_id, buf)
        hid_t attr_id;       IN: Attribute to write
        hid_t type_id;        IN: Memory datatype of buffer
        void *buf;           IN: Buffer of data to write
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
        This function writes a complete attribute to disk.
--------------------------------------------------------------------------*/
herr_t
H5Awrite(hid_t attr_id, hid_t type_id, const void *buf)
{
    H5A_t                  *attr = NULL;
    const H5T_t    *mem_type = NULL;
    herr_t                  ret_value = FAIL;

    FUNC_ENTER(H5Awrite, FAIL);
    H5TRACE3("e","iix",attr_id,type_id,buf);

    /* check arguments */
    if (H5I_ATTR != H5I_get_type(attr_id) ||
        (NULL == (attr = H5I_object(attr_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    }
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (mem_type = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (NULL == buf) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null attribute buffer");
    }

    /* Go write the actual data to the attribute */
    if ((ret_value=H5A_write(attr,mem_type,buf))<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_WRITEERROR, FAIL,
                      "unable to write attribute");
    }

    FUNC_LEAVE(ret_value);
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
        void *buf;           IN: Buffer of data to write
 RETURNS
    Non-negative on success/Negative on failure
 
 ERRORS

 DESCRIPTION
    This function writes a complete attribute to disk.
--------------------------------------------------------------------------*/
static herr_t
H5A_write(H5A_t *attr, const H5T_t *mem_type, const void *buf)
{
    uint8_t             *tconv_buf = NULL;      /* data type conv buffer */
    uint8_t             *bkg_buf = NULL;        /* temp conversion buffer */
    hsize_t             nelmts;                 /* elements in attribute */
    H5T_path_t          *tpath = NULL;          /* conversion information*/
    hid_t               src_id = -1, dst_id = -1;/* temporary type atoms */
    size_t              src_type_size;          /* size of source type  */
    size_t              dst_type_size;          /* size of destination type*/
    hsize_t             buf_size;               /* desired buffer size  */
    int         idx;          /* index of attribute in object header */
    herr_t              ret_value = FAIL;

    FUNC_ENTER(H5A_write, FAIL);

    assert(attr);
    assert(mem_type);
    assert(buf);

    /* Create buffer for data to store on disk */
    nelmts=H5S_get_simple_extent_npoints (attr->ds);

    /* Get the memory and file datatype sizes */
    src_type_size = H5T_get_size(mem_type);
    dst_type_size = H5T_get_size(attr->dt);

    /* Get the maximum buffer size needed and allocate it */
    buf_size = nelmts*MAX(src_type_size,dst_type_size);
    assert(buf_size==(hsize_t)((size_t)buf_size)); /*check for overflow*/
    if (NULL==(tconv_buf = H5MM_malloc ((size_t)buf_size)) ||
            NULL==(bkg_buf = H5MM_calloc((size_t)buf_size))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }

    /* Copy the user's data into the buffer for conversion */
    assert((src_type_size*nelmts)==(hsize_t)((size_t)(src_type_size*nelmts))); /*check for overflow*/
    HDmemcpy(tconv_buf,buf,(size_t)(src_type_size*nelmts));

    /* Convert memory buffer into disk buffer */
    /* Set up type conversion function */
    if (NULL == (tpath = H5T_path_find(mem_type, attr->dt, NULL, NULL))) {
        HGOTO_ERROR(H5E_ATTR, H5E_UNSUPPORTED, FAIL,
                    "unable to convert between src and dest data types");
    } else if (!H5T_IS_NOOP(tpath)) {
        if ((src_id = H5I_register(H5I_DATATYPE,
                                   H5T_copy(mem_type, H5T_COPY_ALL)))<0 ||
            (dst_id = H5I_register(H5I_DATATYPE,
                                   H5T_copy(attr->dt, H5T_COPY_ALL)))<0) {
            HGOTO_ERROR(H5E_ATTR, H5E_CANTREGISTER, FAIL,
                        "unable to register types for conversion");
        }
    }

    /* Perform data type conversion */
    if (H5T_convert(tpath, src_id, dst_id, nelmts, 0, 0, tconv_buf, bkg_buf,
                    H5P_DEFAULT)<0) {
        HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL,
                    "data type conversion failed");
    }

    /* Free the previous attribute data buffer, if there is one */
    if(attr->data)
        H5MM_xfree(attr->data);

    /* Look up the attribute for the object */
    if((idx=H5A_get_index(&(attr->ent),attr->name))<0)
        HGOTO_ERROR(H5E_ATTR, H5E_BADVALUE, FAIL, "attribute not found");

    /* Modify the attribute data */
    attr->data=tconv_buf;   /* Set the data pointer temporarily */
    if (H5O_modify(&(attr->ent), H5O_ATTR, idx, 0, attr) < 0) 
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

    FUNC_LEAVE(ret_value);
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
    H5A_t               *attr = NULL;
    const H5T_t         *mem_type = NULL;
    herr_t              ret_value = FAIL;

    FUNC_ENTER(H5Aread, FAIL);
    H5TRACE3("e","iix",attr_id,type_id,buf);

    /* check arguments */
    if (H5I_ATTR != H5I_get_type(attr_id) ||
        (NULL == (attr = H5I_object(attr_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    }
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (mem_type = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (NULL == buf) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null attribute buffer");
    }

    /* Go write the actual data to the attribute */
    if ((ret_value=H5A_read(attr,mem_type,buf))<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_READERROR, FAIL,
                      "unable to read attribute");
    }

    FUNC_LEAVE(ret_value);
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
H5A_read(H5A_t *attr, const H5T_t *mem_type, void *buf)
{
    uint8_t             *tconv_buf = NULL;      /* data type conv buffer*/
    uint8_t             *bkg_buf = NULL;        /* background buffer */
    hsize_t             nelmts;                 /* elements in attribute*/
    H5T_path_t          *tpath = NULL;          /* type conversion info */
    hid_t               src_id = -1, dst_id = -1;/* temporary type atoms*/
    size_t              src_type_size;          /* size of source type  */
    size_t              dst_type_size;          /* size of destination type */
    hsize_t             buf_size;               /* desired buffer size  */
    herr_t              ret_value = FAIL;

    FUNC_ENTER(H5A_read, FAIL);

    assert(attr);
    assert(mem_type);
    assert(buf);

    /* Create buffer for data to store on disk */
    nelmts=H5S_get_simple_extent_npoints (attr->ds);

    /* Get the memory and file datatype sizes */
    src_type_size = H5T_get_size(attr->dt);
    dst_type_size = H5T_get_size(mem_type);

    /* Check if the attribute has any data yet, if not, fill with zeroes */
    assert((dst_type_size*nelmts)==(hsize_t)((size_t)(dst_type_size*nelmts))); /*check for overflow*/
    if(attr->ent_opened && !attr->initialized) {
        HDmemset(buf,0,(size_t)(dst_type_size*nelmts));
    }   /* end if */
    else {  /* Attribute exists and has a value */
        /* Get the maximum buffer size needed and allocate it */
        buf_size = nelmts*MAX(src_type_size,dst_type_size);
        assert(buf_size==(hsize_t)((size_t)buf_size)); /*check for overflow*/
        if (NULL==(tconv_buf = H5MM_malloc ((size_t)buf_size)) ||
                NULL==(bkg_buf = H5MM_calloc((size_t)buf_size))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                 "memory allocation failed");
        }

        /* Copy the attribute data into the buffer for conversion */
        assert((src_type_size*nelmts)==(hsize_t)((size_t)(src_type_size*nelmts))); /*check for overflow*/
        HDmemcpy(tconv_buf,attr->data,(size_t)(src_type_size*nelmts));

        /* Convert memory buffer into disk buffer */
        /* Set up type conversion function */
        if (NULL == (tpath = H5T_path_find(attr->dt, mem_type, NULL, NULL))) {
            HGOTO_ERROR(H5E_ATTR, H5E_UNSUPPORTED, FAIL,
                "unable to convert between src and dest data types");
        } else if (!H5T_IS_NOOP(tpath)) {
            if ((src_id = H5I_register(H5I_DATATYPE,
                       H5T_copy(attr->dt, H5T_COPY_ALL)))<0 ||
                    (dst_id = H5I_register(H5I_DATATYPE,
                       H5T_copy(mem_type, H5T_COPY_ALL)))<0) {
                HGOTO_ERROR(H5E_ATTR, H5E_CANTREGISTER, FAIL,
                "unable to register types for conversion");
            }
        }

        /* Perform data type conversion.  */
        if (H5T_convert(tpath, src_id, dst_id, nelmts, 0, 0, tconv_buf, bkg_buf,
                        H5P_DEFAULT)<0) {
            HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL,
                "data type conversion failed");
        }

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

    FUNC_LEAVE(ret_value);
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
    H5A_t       *attr = NULL;
    H5S_t       *dst = NULL;
    hid_t       ret_value = FAIL;

    FUNC_ENTER(H5Aget_space, FAIL);
    H5TRACE1("i","i",attr_id);

    /* check arguments */
    if (H5I_ATTR != H5I_get_type(attr_id) ||
        (NULL == (attr = H5I_object(attr_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    }

    /* Copy the attribute's dataspace */
    if (NULL==(dst=H5S_copy (attr->ds))) {
        HRETURN_ERROR (H5E_ATTR, H5E_CANTINIT, FAIL,
                       "unable to copy dataspace");
    }

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, dst))<0) {
        HRETURN_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL,
                       "unable to register dataspace atom");
    }

    FUNC_LEAVE(ret_value);
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
 *      Robb Matzke, 4 Jun 1998
 *      The data type is reopened if it's a named type before returning it to
 *      the application.  The data types returned by this function are always
 *      read-only. If an error occurs when atomizing the return data type
 *      then the data type is closed.
--------------------------------------------------------------------------*/
hid_t
H5Aget_type(hid_t attr_id)
{
    H5A_t                  *attr = NULL;
    H5T_t       *dst = NULL;
    hid_t                       ret_value = FAIL;

    FUNC_ENTER(H5Aget_type, FAIL);
    H5TRACE1("i","i",attr_id);

    /* check arguments */
    if (H5I_ATTR != H5I_get_type(attr_id) ||
        (NULL == (attr = H5I_object(attr_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    }

    /*
     * Copy the attribute's data type.  If the type is a named type then
     * reopen the type before returning it to the user. Make the type
     * read-only.
     */
    if (NULL==(dst=H5T_copy(attr->dt, H5T_COPY_REOPEN))) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL,
                      "unable to copy datatype");
    }
    /* Mark any VL datatypes as being in memory now */
        if (H5T_vlen_mark(dst, NULL, H5T_VLEN_MEMORY)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "invalid VL location");
    }
    if (H5T_lock(dst, FALSE)<0) {
        H5T_close(dst);
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to lock transient data type");
    }
    
    /* Atomize */
    if ((ret_value=H5I_register(H5I_DATATYPE, dst))<0) {
        H5T_close(dst);
        HRETURN_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                      "unable to register datatype atom");
    }

    FUNC_LEAVE(ret_value);
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
    H5A_t               *attr = NULL;
    size_t              copy_len, nbytes;
    ssize_t             ret_value = FAIL;

    FUNC_ENTER(H5Aget_name, FAIL);
    H5TRACE3("Zs","izs",attr_id,buf_size,buf);

    /* check arguments */
    if (H5I_ATTR != H5I_get_type(attr_id) ||
        (NULL == (attr = H5I_object(attr_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    }
    if (!buf || buf_size<1) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer");
    }

    /* get the real attribute length */
    nbytes = HDstrlen(attr->name);
    assert((ssize_t)nbytes>=0); /*overflow, pretty unlikey --rpm*/

    /* compute the string length which will fit into the user's buffer */
    copy_len = MIN(buf_size-1, nbytes);

    /* Copy all/some of the name */
    HDmemcpy(buf, attr->name, copy_len);

    /* Terminate the string */
    buf[copy_len]='\0';

    ret_value = (ssize_t)nbytes;
    FUNC_LEAVE(ret_value);
} /* H5Aget_type() */


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
 *      Robb Matzke, 5 Jun 1998
 *      The LOC_ID can also be a named (committed) data type.
--------------------------------------------------------------------------*/
int
H5Aget_num_attrs(hid_t loc_id)
{
    H5G_entry_t         *ent = NULL;    /*symtab ent of object to attribute */
    void                *obj = NULL;
    int                 ret_value = 0;

    FUNC_ENTER(H5Aget_num_attrs, FAIL);
    H5TRACE1("Is","i",loc_id);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) ||
        H5I_ATTR==H5I_get_type(loc_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "location is not valid for an attribute");
    }
    if(NULL == (obj = H5I_object(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADATOM, FAIL, "illegal object atom");
    }
    switch (H5I_get_type (loc_id)) {
    case H5I_DATASET:
        ent = H5D_entof ((H5D_t*)obj);
        break;
    case H5I_DATATYPE:
        if (NULL==(ent=H5T_entof ((H5T_t*)obj))) {
            HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL,
                           "target data type is not committed");
        }
        break;
    case H5I_GROUP:
        ent = H5G_entof ((H5G_t*)obj);
        break;
    default:
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL,
                       "inappropriate attribute target");
    }

    /* Look up the attribute for the object */
    ret_value=H5O_count(ent, H5O_ATTR);

    FUNC_LEAVE(ret_value);
} /* H5Aget_num_attrs() */


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
 *      Robb Matzke, 5 Jun 1998
 *      The LOC_ID can also be a named (committed) data type.
 *
 *      Robb Matzke, 5 Jun 1998
 *      Like the group iterator, if ATTR_NUM is the null pointer then all
 *      attributes are processed.
 *      
--------------------------------------------------------------------------*/
herr_t
H5Aiterate(hid_t loc_id, unsigned *attr_num, H5A_operator_t op, void *op_data)
{
    H5G_entry_t         *ent = NULL;    /*symtab ent of object to attribute */
    H5A_t               found_attr;
    herr_t              ret_value = 0;
    int         idx;

    FUNC_ENTER(H5Aiterate, FAIL);
    H5TRACE4("e","i*Iuxx",loc_id,attr_num,op,op_data);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) ||
        H5I_ATTR==H5I_get_type(loc_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "location is not valid for an attribute");
    }
    if (NULL==(ent=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }

    /*
     * Look up the attribute for the object. Make certain the start point is
     * reasonable.
     */
    idx = attr_num ? (int)*attr_num : 0;
    if(idx<H5O_count(ent, H5O_ATTR)) {
        while(H5O_read(ent, H5O_ATTR, idx++, &found_attr)!=NULL) {
            /*
             * Compare found attribute name to new attribute name reject
             * creation if names are the same.
             */
            if((ret_value=(op)(loc_id,found_attr.name,op_data))!=0) {
                H5O_reset (H5O_ATTR, &found_attr);
                break;
            }
            H5O_reset (H5O_ATTR, &found_attr);
        }
        H5E_clear ();
    }

    if (attr_num) *attr_num = (unsigned)idx;
    FUNC_LEAVE(ret_value);
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
 *      Robb Matzke, 5 Jun 1998
 *      The LOC_ID can also be a named (committed) data type.
 *      
--------------------------------------------------------------------------*/
herr_t
H5Adelete(hid_t loc_id, const char *name)
{
    H5A_t       found_attr;
    H5G_entry_t *ent = NULL;            /*symtab ent of object to attribute */
    int        idx=0, found=-1;
    herr_t      ret_value = FAIL;

    FUNC_ENTER(H5Aopen_name, FAIL);
    H5TRACE2("e","is",loc_id,name);

    /* check arguments */
    if (H5I_FILE==H5I_get_type(loc_id) ||
        H5I_ATTR==H5I_get_type(loc_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "location is not valid for an attribute");
    }
    if (NULL==(ent=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }

    /* Look up the attribute for the object */
    idx=0;
    while(H5O_read(ent, H5O_ATTR, idx, &found_attr)!=NULL) {
        /*
         * Compare found attribute name to new attribute name reject
         * creation if names are the same.
         */
        if(HDstrcmp(found_attr.name,name)==0) {
            H5O_reset (H5O_ATTR, &found_attr);
            found = idx;
            break;
        }
        H5O_reset (H5O_ATTR, &found_attr);
        idx++;
    }
    H5E_clear ();
    if (found<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "attribute not found");
    }

    /* Delete the attribute from the location */
    if ((ret_value=H5O_remove(ent, H5O_ATTR, found)) < 0) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL,
                      "unable to delete attribute header message");
    }
    
    FUNC_LEAVE(ret_value);
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
    FUNC_ENTER(H5Aclose, FAIL);
    H5TRACE1("e","i",attr_id);

    /* check arguments */
    if (H5I_ATTR != H5I_get_type(attr_id) || NULL == H5I_object(attr_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute");
    }

    /* Decrement references to that atom (and close it) */
    H5I_dec_ref (attr_id);
    FUNC_LEAVE(SUCCEED);
} /* H5Aclose() */


/*-------------------------------------------------------------------------
 * Function:    H5A_copy
 *
 * Purpose:     Copies attribute OLD_ATTR.
 *
 * Return:      Success:        Pointer to a new copy of the OLD_ATTR argument.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, December  4, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5A_t *
H5A_copy(const H5A_t *old_attr)
{
    H5A_t       *new_attr=NULL;

    FUNC_ENTER(H5A_copy, NULL);

    /* check args */
    assert(old_attr);

    /* get space */
    if (NULL==(new_attr = H5MM_calloc(sizeof(H5A_t)))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    /* Copy the top level of the attribute */
    *new_attr = *old_attr;

    /* Don't open the object header for a copy */
    new_attr->ent_opened=0;

    /* Copy the guts of the attribute */
    new_attr->name=HDstrdup(old_attr->name);
    new_attr->dt=H5T_copy(old_attr->dt, H5T_COPY_ALL);
    new_attr->ds=H5S_copy(old_attr->ds);
    if(old_attr->data) {
        if (NULL==(new_attr->data=H5MM_malloc(old_attr->data_size))) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                           "memory allocation failed");
        }
        HDmemcpy(new_attr->data,old_attr->data,old_attr->data_size);
    } /* end if */

#ifndef LATER
    /* Copy the share info? */
#endif
    
    FUNC_LEAVE(new_attr);
}


/*-------------------------------------------------------------------------
 * Function:    H5A_close
 *
 * Purpose:     Frees a attribute and all associated memory.  
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_close(H5A_t *attr)
{
    FUNC_ENTER(H5A_close, FAIL);

    assert(attr);

    /* Check if the attribute has any data yet, if not, fill with zeroes */
    if(attr->ent_opened && !attr->initialized) {
        uint8_t *tmp_buf=H5MM_calloc(attr->data_size);
        if (NULL == tmp_buf) {
            HRETURN_ERROR(H5E_ATTR, H5E_NOSPACE, FAIL,
                          "memory allocation failed for attribute fill-value");
        }

        /* Go write the fill data to the attribute */
        if (H5A_write(attr,attr->dt,tmp_buf)<0) {
            HRETURN_ERROR(H5E_ATTR, H5E_WRITEERROR, FAIL,
                          "unable to write attribute");
        }

        /* Free temporary buffer */
        H5MM_xfree(tmp_buf);
    } /* end if */

    /* Free dynamicly allocated items */
    if(attr->name)
        H5MM_xfree(attr->name);
    if(attr->dt) H5T_close(attr->dt);
    if(attr->ds) H5S_close(attr->ds);
    if(attr->data) H5MM_xfree(attr->data);

    /* Close the object's symbol-table entry */
    if(attr->ent_opened) H5O_close(&(attr->ent));

#ifndef LATER
    /* Do something with the shared information? */
#endif

    H5MM_xfree(attr);
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5A_entof
 *
 * Purpose:     Return the symbol table entry for an attribute.  It's the
 *              symbol table entry for the object to which the attribute
 *              belongs, not the attribute itself.
 *
 * Return:      Success:        Ptr to entry
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, August  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_entry_t *
H5A_entof(H5A_t *attr)
{
    FUNC_ENTER(H5A_entof, NULL);
    assert(attr);
    FUNC_LEAVE(&(attr->ent));
}
