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

/* Id */

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */

#include "H5private.h"		/* Generic Functions */
#include "H5Iprivate.h"		/* ID Functions */
#include "H5Dprivate.h"		/* Datasets */
#include "H5Eprivate.h"		/* Error handling */
#include "H5Fpkg.h"		/* Files */
#include "H5Gprivate.h"		/* Groups */
#include "H5HGprivate.h"    /* Global Heaps */
#include "H5MMprivate.h"    /* Memory Management */
#include "H5Rprivate.h"		/* References */
#include "H5Spkg.h"		/* Dataspaces */
#include "H5Tprivate.h"		/* Datatypes */

/* Interface initialization */
#define PABLO_MASK	H5R_mask
#define INTERFACE_INIT	H5R_init_interface
static int		interface_initialize_g = 0;
static herr_t		H5R_init_interface(void);

/* Static functions */
static herr_t H5R_create(void *ref, H5G_entry_t *loc, const char *name,
        H5R_type_t ref_type, H5S_t *space, hid_t dxpl_id);
static hid_t H5R_dereference(H5F_t *file, hid_t dxpl_id, H5R_type_t ref_type, void *_ref);
static H5S_t * H5R_get_region(H5F_t *file, hid_t dxpl_id, H5R_type_t ref_type, void *_ref);
static int H5R_get_obj_type(H5F_t *file, hid_t dxpl_id, H5R_type_t ref_type, void *_ref);


/*--------------------------------------------------------------------------
NAME
   H5R_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5R_init_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5R_init_interface(void)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5R_init_interface);

    /* Initialize the atom group for the file IDs */
    if (H5I_init_group(H5I_REFERENCE, H5I_REFID_HASHSIZE, H5R_RESERVED_ATOMS,
		       (H5I_free_t)NULL)<0)
	HGOTO_ERROR (H5E_REFERENCE, H5E_CANTINIT, FAIL, "unable to initialize interface");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5R_term_interface
 PURPOSE
    Terminate various H5R objects
 USAGE
    void H5R_term_interface()
 RETURNS
    void
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5R_term_interface(void)
{
    int	n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5R_term_interface);
    
    if (interface_initialize_g) {
	if ((n=H5I_nmembers(H5I_REFERENCE))) {
	    H5I_clear_group(H5I_REFERENCE, FALSE);
	} else {
	    H5I_destroy_group(H5I_REFERENCE);
	    interface_initialize_g = 0;
	    n = 1; /*H5I*/
	}
    }
    
    FUNC_LEAVE_NOAPI(n);
}


/*--------------------------------------------------------------------------
 NAME
    H5R_create
 PURPOSE
    Creates a particular kind of reference for the user
 USAGE
    herr_t H5R_create(ref, loc, name, ref_type, space)
        void *ref;          OUT: Reference created
        H5G_entry_t *loc;   IN: File location used to locate object pointed to
        const char *name;   IN: Name of object at location LOC_ID of object
                                    pointed to
        H5R_type_t ref_type;    IN: Type of reference to create
        H5S_t *space;       IN: Dataspace ID with selection, used for Dataset
                                    Region references.
        
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Creates a particular type of reference specified with REF_TYPE, in the
    space pointed to by REF.  The LOC_ID and NAME are used to locate the object
    pointed to and the SPACE_ID is used to choose the region pointed to (for
    Dataset Region references).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5R_create(void *_ref, H5G_entry_t *loc, const char *name, H5R_type_t ref_type, H5S_t *space, hid_t dxpl_id)
{
    H5G_stat_t sb;              /* Stat buffer for retrieving OID */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5R_create);

    assert(_ref);
    assert(loc);
    assert(name);
    assert(ref_type>H5R_BADTYPE || ref_type<H5R_MAXTYPE);

    if (H5G_get_objinfo (loc, name, 0, &sb, dxpl_id)<0)
        HGOTO_ERROR (H5E_REFERENCE, H5E_NOTFOUND, FAIL, "unable to stat object");

    switch(ref_type) {
        case H5R_OBJECT:
        {
            haddr_t addr;
            hobj_ref_t *ref=(hobj_ref_t *)_ref; /* Get pointer to correct type of reference struct */
            uint8_t *p;       /* Pointer to OID to store */

            /* Set information for reference */
            p=(uint8_t *)ref->oid;
            H5F_addr_pack(loc->file,&addr,&sb.objno[0]);
            H5F_addr_encode(loc->file,&p,addr);
            break;
        }

        case H5R_DATASET_REGION:
        {
            haddr_t addr;
            H5HG_t hobjid;      /* Heap object ID */
            hdset_reg_ref_t *ref=(hdset_reg_ref_t *)_ref; /* Get pointer to correct type of reference struct */
            hssize_t buf_size;  /* Size of buffer needed to serialize selection */
            uint8_t *p;       /* Pointer to OID to store */
            uint8_t *buf;     /* Buffer to store serialized selection in */
            unsigned heapid_found;  /* Flag for non-zero heap ID found */
            unsigned u;        /* local index */

            /* Set up information for dataset region */

            /* Return any previous heap block to the free list if we are garbage collecting */
            if(loc->file->shared->gc_ref) {
                /* Check for an existing heap ID in the reference */
                for(u=0, heapid_found=0; u<H5R_DSET_REG_REF_BUF_SIZE; u++)
                    if(ref->heapid[u]!=0) {
                        heapid_found=1;
                        break;
                    } /* end if */
                
                if(heapid_found!=0) {
/* Return heap block to free list */
                } /* end if */
            } /* end if */

            /* Zero the heap ID out, may leak heap space if user is re-using reference and doesn't have garbage collection on */
            HDmemset(ref->heapid,H5R_DSET_REG_REF_BUF_SIZE,0);

            /* Get the amount of space required to serialize the selection */
            if ((buf_size = (*space->select.serial_size)(space)) < 0)
                HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, FAIL, "Invalid amount of space for serializing selection");

            /* Increase buffer size to allow for the dataset OID */
            buf_size+=sizeof(haddr_t);

            /* Allocate the space to store the serialized information */
            H5_CHECK_OVERFLOW(buf_size,hssize_t,size_t);
            if (NULL==(buf = H5MM_malloc((size_t)buf_size)))
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

            /* Serialize information for dataset OID */
            p=(uint8_t *)buf;
            H5F_addr_pack(loc->file,&addr,&sb.objno[0]);
            H5F_addr_encode(loc->file,&p,addr);

            /* Serialize the selection */
            if ((*space->select.serialize)(space,p) < 0)
                HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCOPY, FAIL, "Unable to serialize selection");

            /* Save the serialized buffer for later */
            H5_CHECK_OVERFLOW(buf_size,hssize_t,size_t);
            if(H5HG_insert(loc->file,dxpl_id,(size_t)buf_size,buf,&hobjid)<0)
                HGOTO_ERROR(H5E_REFERENCE, H5E_WRITEERROR, FAIL, "Unable to serialize selection");

            /* Serialize the heap ID and index for storage in the file */
            p=(uint8_t *)ref->heapid;
            H5F_addr_encode(loc->file,&p,hobjid.addr);
            INT32ENCODE(p,hobjid.idx);

            /* Free the buffer we serialized data in */
            H5MM_xfree(buf);
            break;
        }

        case H5R_INTERNAL:
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "Internal references are not yet supported");

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            assert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5R_create() */


/*--------------------------------------------------------------------------
 NAME
    H5Rcreate
 PURPOSE
    Creates a particular kind of reference for the user
 USAGE
    herr_t H5Rcreate(ref, loc_id, name, ref_type, space_id)
        void *ref;          OUT: Reference created
        hid_t loc_id;       IN: Location ID used to locate object pointed to
        const char *name;   IN: Name of object at location LOC_ID of object
                                    pointed to
        H5R_type_t ref_type;    IN: Type of reference to create
        hid_t space_id;     IN: Dataspace ID with selection, used for Dataset
                                    Region references.
        
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Creates a particular type of reference specified with REF_TYPE, in the
    space pointed to by REF.  The LOC_ID and NAME are used to locate the object
    pointed to and the SPACE_ID is used to choose the region pointed to (for
    Dataset Region references).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Rcreate(void *ref, hid_t loc_id, const char *name, H5R_type_t ref_type, hid_t space_id)
{
    H5G_entry_t *loc = NULL;        /* File location */
    H5S_t	*space = NULL;          /* Pointer to dataspace containing region */
    herr_t      ret_value;       /* Return value */

    FUNC_ENTER_API(H5Rcreate, FAIL);
    H5TRACE5("e","xisRti",ref,loc_id,name,ref_type,space_id);

    /* Check args */
    if(ref==NULL)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer");
    if (NULL==(loc=H5G_loc (loc_id)))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name given");
    if(ref_type<=H5R_BADTYPE || ref_type>=H5R_MAXTYPE)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type");
    if(ref_type!=H5R_OBJECT && ref_type!=H5R_DATASET_REGION)
        HGOTO_ERROR (H5E_ARGS, H5E_UNSUPPORTED, FAIL, "reference type not supported");
    if (space_id!=(-1) && (NULL==(space=H5I_object_verify(space_id,H5I_DATASPACE))))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace");

    /* Create reference */
    if ((ret_value=H5R_create(ref,loc,name,ref_type,space, H5AC_dxpl_id))<0)
        HGOTO_ERROR (H5E_REFERENCE, H5E_CANTINIT, FAIL, "unable to create reference");

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Rcreate() */


/*--------------------------------------------------------------------------
 NAME
    H5R_dereference
 PURPOSE
    Opens the HDF5 object referenced.
 USAGE
    hid_t H5R_dereference(ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        H5R_type_t ref_type;    IN: Type of reference
        void *ref;          IN: Reference to open.
        
 RETURNS
    Valid ID on success, Negative on failure
 DESCRIPTION
    Given a reference to some object, open that object and return an ID for
    that object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Currently only set up to work with references to datasets
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static hid_t
H5R_dereference(H5F_t *file, hid_t dxpl_id, H5R_type_t ref_type, void *_ref)
{
    H5G_t *group;               /* Pointer to group to open */
    H5T_t *datatype;            /* Pointer to datatype to open */
    H5G_entry_t ent;            /* Symbol table entry */
    uint8_t *p;                 /* Pointer to OID to store */
    int oid_type;               /* type of object being dereferenced */
    hid_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5R_dereference);

    assert(_ref);
    assert(ref_type>H5R_BADTYPE || ref_type<H5R_MAXTYPE);
    assert(file);

    /* Initialize the symbol table entry */
    HDmemset(&ent,0,sizeof(H5G_entry_t));
    ent.type=H5G_NOTHING_CACHED;
    ent.file=file;

    switch(ref_type) {
        case H5R_OBJECT:
        {
            hobj_ref_t *ref=(hobj_ref_t *)_ref; /* Only object references currently supported */
            /*
             * Switch on object type, when we implement that feature, always try to
             *  open a dataset for now
             */
            /* Get the object oid */
            p=(uint8_t *)ref->oid;
            H5F_addr_decode(ent.file,(const uint8_t **)&p,&(ent.header));
        } /* end case */
        break;

        case H5R_DATASET_REGION:
        {
            hdset_reg_ref_t *ref=(hdset_reg_ref_t *)_ref; /* Get pointer to correct type of reference struct */
            H5HG_t hobjid;  /* Heap object ID */
            uint8_t *buf;   /* Buffer to store serialized selection in */

            /* Get the heap ID for the dataset region */
            p=(uint8_t *)ref->heapid;
            H5F_addr_decode(ent.file,(const uint8_t **)&p,&(hobjid.addr));
            INT32DECODE(p,hobjid.idx);

            /* Get the dataset region from the heap (allocate inside routine) */
            if((buf=H5HG_read(ent.file,dxpl_id,&hobjid,NULL))==NULL)
                HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, FAIL, "Unable to read dataset region information");

            /* Get the object oid for the dataset */
            p=(uint8_t *)buf;
            H5F_addr_decode(ent.file,(const uint8_t **)&p,&(ent.header));

            /* Free the buffer allocated in H5HG_read() */
            H5MM_xfree(buf);
        } /* end case */
        break;

        case H5R_INTERNAL:
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "Internal references are not yet supported");

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            assert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)");
    } /* end switch */

    /* Check to make certain that this object hasn't been deleted since the reference was created */
    if(H5O_link(&ent,0,dxpl_id)<=0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_LINKCOUNT, FAIL, "dereferencing deleted object");

    /* Open the dataset object */
    oid_type=H5G_get_type(&ent,dxpl_id);
    switch(oid_type) {
        case H5G_GROUP:
            if ((group=H5G_open_oid(&ent,dxpl_id)) == NULL)
                HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "not found");

            /* Create an atom for the dataset */
            if ((ret_value = H5I_register(H5I_GROUP, group)) < 0) {
                H5G_close(group);
                HGOTO_ERROR(H5E_SYM, H5E_CANTREGISTER, FAIL, "can't register group");
            }
            break;

        case H5G_TYPE:
            if ((datatype=H5T_open_oid(&ent, dxpl_id)) == NULL)
                HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "not found");

            /* Create an atom for the dataset */
            if ((ret_value = H5I_register(H5I_DATATYPE, datatype)) < 0) {
                H5T_close(datatype);
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "can't register group");
            }
            break;

        case H5G_DATASET:
            /* Open the dataset */
            if ((ret_value=H5D_open(&ent,dxpl_id)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, FAIL, "not found");
            break;

        default:
            HGOTO_ERROR(H5E_REFERENCE, H5E_BADTYPE, FAIL, "can't identify type of object referenced");
     } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5R_dereference() */


/*--------------------------------------------------------------------------
 NAME
    H5Rdereference
 PURPOSE
    Opens the HDF5 object referenced.
 USAGE
    hid_t H5Rdereference(ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to create
        void *ref;      IN: Reference to open.
        
 RETURNS
    Valid ID on success, Negative on failure
 DESCRIPTION
    Given a reference to some object, open that object and return an ID for
    that object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Rdereference(hid_t id, H5R_type_t ref_type, void *_ref)
{
    H5G_entry_t *loc = NULL;    /* Symbol table entry */
    H5F_t *file=NULL;       /* File object */
    hid_t ret_value;

    FUNC_ENTER_API(H5Rdereference, FAIL);
    H5TRACE3("i","iRtx",id,ref_type,_ref);

    /* Check args */
    if (NULL == (loc = H5G_loc(id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if(ref_type<=H5R_BADTYPE || ref_type>=H5R_MAXTYPE)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type");
    if(_ref==NULL)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer");

    /* Get the file pointer from the entry */
    file=loc->file;

    /* Create reference */
    if ((ret_value=H5R_dereference(file, H5AC_dxpl_id, ref_type, _ref))<0)
        HGOTO_ERROR (H5E_REFERENCE, H5E_CANTINIT, FAIL, "unable dereference object");

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Rdereference() */


/*--------------------------------------------------------------------------
 NAME
    H5R_get_region
 PURPOSE
    Retrieves a dataspace with the region pointed to selected.
 USAGE
    H5S_t *H5R_get_region(file, ref_type, ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        H5R_type_t ref_type;    UNUSED
        void *ref;          IN: Reference to open.
        
 RETURNS
    Pointer to the dataspace on success, NULL on failure
 DESCRIPTION
    Given a reference to some object, creates a copy of the dataset pointed
    to's dataspace and defines a selection in the copy which is the region
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5S_t *
H5R_get_region(H5F_t *file, hid_t dxpl_id, H5R_type_t UNUSED ref_type, void *_ref)
{
    H5G_entry_t ent;            /* Symbol table entry */
    uint8_t *p;                 /* Pointer to OID to store */
    hdset_reg_ref_t *ref=(hdset_reg_ref_t *)_ref; /* Get pointer to correct type of reference struct */
    H5HG_t hobjid;  /* Heap object ID */
    uint8_t *buf;   /* Buffer to store serialized selection in */
    H5S_t *ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5R_get_region);

    assert(_ref);
    assert(ref_type==H5R_DATASET_REGION);
    assert(file);

    /* Initialize the symbol table entry */
    HDmemset(&ent,0,sizeof(H5G_entry_t));
    ent.type=H5G_NOTHING_CACHED;
    ent.file=file;

    /* Get the heap ID for the dataset region */
    p=(uint8_t *)ref->heapid;
    H5F_addr_decode(ent.file,(const uint8_t **)&p,&(hobjid.addr));
    INT32DECODE(p,hobjid.idx);

    /* Get the dataset region from the heap (allocate inside routine) */
    if((buf=H5HG_read(ent.file,dxpl_id,&hobjid,NULL))==NULL)
        HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, NULL, "Unable to read dataset region information");

    /* Get the object oid for the dataset */
    p=(uint8_t *)buf;
    H5F_addr_decode(ent.file,(const uint8_t **)&p,&(ent.header));

    /* Open and copy the dataset's dataspace */
    if ((ret_value=H5S_read(&ent, dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_DATASPACE, H5E_NOTFOUND, NULL, "not found");

    /* Unserialize the selection */
    if (H5S_select_deserialize(ret_value,p) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTDECODE, NULL, "can't deserialize selection");

    /* Free the buffer allocated in H5HG_read() */
    H5MM_xfree(buf);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5R_get_region() */


/*--------------------------------------------------------------------------
 NAME
    H5Rget_region
 PURPOSE
    Retrieves a dataspace with the region pointed to selected.
 USAGE
    hid_t H5Rget_region(id, ref_type, ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to get region of
        void *ref;        IN: Reference to open.
        
 RETURNS
    Valid ID on success, Negative on failure
 DESCRIPTION
    Given a reference to some object, creates a copy of the dataset pointed
    to's dataspace and defines a selection in the copy which is the region
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Rget_region(hid_t id, H5R_type_t ref_type, void *_ref)
{
    H5G_entry_t *loc = NULL;    /* Symbol table entry */
    H5S_t *space = NULL;    /* dataspace object */
    H5F_t *file=NULL;       /* File object */
    hid_t ret_value;

    FUNC_ENTER_API(H5Rget_region, FAIL);
    H5TRACE3("i","iRtx",id,ref_type,_ref);

    /* Check args */
    if (NULL == (loc = H5G_loc(id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if(ref_type!=H5R_DATASET_REGION)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type");
    if(_ref==NULL)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer");

    /* Get the file pointer from the entry */
    file=loc->file;

    /* Get the dataspace with the correct region selected */
    if ((space=H5R_get_region(file,H5AC_ind_dxpl_id,ref_type,_ref))==NULL)
        HGOTO_ERROR (H5E_REFERENCE, H5E_CANTCREATE, FAIL, "unable to create dataspace");

    /* Atomize */
    if ((ret_value=H5I_register (H5I_DATASPACE, space))<0)
        HGOTO_ERROR (H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace atom");

done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Rget_region() */

#ifdef H5_WANT_H5_V1_4_COMPAT

/*--------------------------------------------------------------------------
 NAME
    H5R_get_object_type
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    int H5R_get_object_type(file, ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        void *ref;          IN: Reference to query.
        
 RETURNS
    Success:	An object type defined in H5Gpublic.h
    Failure:	H5G_UNKNOWN
 DESCRIPTION
    Given a reference to some object, this function returns the type of object
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5R_get_object_type(H5F_t *file, hid_t dxpl_id, void *_ref)
{
    H5G_entry_t ent;            /* Symbol table entry */
    hobj_ref_t *ref=(hobj_ref_t *)_ref; /* Only object references currently supported */
    uint8_t *p;                 /* Pointer to OID to store */
    int ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5R_get_object_type);

    assert(ref);
    assert(file);

    /* Initialize the symbol table entry */
    HDmemset(&ent,0,sizeof(H5G_entry_t));
    ent.type=H5G_NOTHING_CACHED;
    ent.file=file;

    /* Get the object oid */
    p=(uint8_t *)ref->oid;
    H5F_addr_decode(ent.file,(const uint8_t **)&p,&(ent.header));

    /* Get the OID type */
    ret_value=H5G_get_type(&ent, dxpl_id);

#ifdef LATER
done:
#endif /* LATER */
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5R_get_object_type() */


/*--------------------------------------------------------------------------
 NAME
    H5Rget_object_type
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    int H5Rget_object_type(id, ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        void *ref;          IN: Reference to query.
        
 RETURNS
    Success:	An object type defined in H5Gpublic.h
    Failure:	H5G_UNKNOWN
 DESCRIPTION
    Given a reference to some object, this function returns the type of object
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5Rget_object_type(hid_t id, void *_ref)
{
    H5G_entry_t *loc = NULL;    /* Symbol table entry */
    H5F_t *file=NULL;       /* File object */
    hid_t ret_value;

    FUNC_ENTER_API(H5Rget_object_type, H5G_UNKNOWN);
    H5TRACE2("Is","ix",id,_ref);

    /* Check args */
    if (NULL == (loc = H5G_loc(id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    if(_ref==NULL)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, H5G_UNKNOWN, "invalid reference pointer");

    /* Get the file pointer from the entry */
    file=loc->file;

    /* Get the object information */
    if ((ret_value=H5R_get_object_type(file,H5AC_ind_dxpl_id,_ref))<0)
	HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, H5G_UNKNOWN, "unable to determine object type");
    
done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Rget_object_type() */
#endif /* H5_WANT_H5_V1_4_COMPAT */


/*--------------------------------------------------------------------------
 NAME
    H5R_get_obj_type
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    int H5R_get_obj_type(file, ref_type, ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        H5R_type_t ref_type;    IN: Type of reference to query
        void *ref;          IN: Reference to query.
        
 RETURNS
    Success:	An object type defined in H5Gpublic.h
    Failure:	H5G_UNKNOWN
 DESCRIPTION
    Given a reference to some object, this function returns the type of object
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5R_get_obj_type(H5F_t *file, hid_t dxpl_id, H5R_type_t ref_type, void *_ref)
{
    H5G_entry_t ent;            /* Symbol table entry */
    uint8_t *p;                 /* Pointer to OID to store */
    int ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5R_get_obj_type);

    assert(file);
    assert(_ref);

    /* Initialize the symbol table entry */
    HDmemset(&ent,0,sizeof(H5G_entry_t));
    ent.type=H5G_NOTHING_CACHED;
    ent.file=file;

    switch(ref_type) {
        case H5R_OBJECT:
        {
            hobj_ref_t *ref=(hobj_ref_t *)_ref; /* Only object references currently supported */

            /* Get the object oid */
            p=(uint8_t *)ref->oid;
            H5F_addr_decode(ent.file,(const uint8_t **)&p,&(ent.header));
        } /* end case */
        break;

        case H5R_DATASET_REGION:
        {
            hdset_reg_ref_t *ref=(hdset_reg_ref_t *)_ref; /* Get pointer to correct type of reference struct */
            H5HG_t hobjid;  /* Heap object ID */
            uint8_t *buf;   /* Buffer to store serialized selection in */

            /* Get the heap ID for the dataset region */
            p=(uint8_t *)ref->heapid;
            H5F_addr_decode(ent.file,(const uint8_t **)&p,&(hobjid.addr));
            INT32DECODE(p,hobjid.idx);

            /* Get the dataset region from the heap (allocate inside routine) */
            if((buf=H5HG_read(ent.file,dxpl_id,&hobjid,NULL))==NULL)
                HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, H5G_UNKNOWN, "Unable to read dataset region information");

            /* Get the object oid for the dataset */
            p=(uint8_t *)buf;
            H5F_addr_decode(ent.file,(const uint8_t **)&p,&(ent.header));

            /* Free the buffer allocated in H5HG_read() */
            H5MM_xfree(buf);
        } /* end case */
        break;

        case H5R_INTERNAL:
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, H5G_UNKNOWN, "Internal references are not yet supported");

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            assert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, H5G_UNKNOWN, "internal error (unknown reference type)");
    } /* end switch */

    /* Check to make certain that this object hasn't been deleted since the reference was created */
    if(H5O_link(&ent,0,dxpl_id)<=0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_LINKCOUNT, H5G_UNKNOWN, "dereferencing deleted object");

    /* Get the OID type */
    ret_value=H5G_get_type(&ent,dxpl_id);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5R_get_obj_type() */


/*--------------------------------------------------------------------------
 NAME
    H5Rget_obj_type
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    int H5Rget_obj_type(id, ref_type, ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to query
        void *ref;          IN: Reference to query.
        
 RETURNS
    Success:	An object type defined in H5Gpublic.h
    Failure:	H5G_UNKNOWN
 DESCRIPTION
    Given a reference to some object, this function returns the type of object
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
#ifdef H5_WANT_H5_V1_4_COMPAT
int
H5Rget_obj_type(hid_t id, H5R_type_t ref_type, void *_ref)
#else /* H5_WANT_H5_V1_4_COMPAT */
H5G_obj_t
H5Rget_obj_type(hid_t id, H5R_type_t ref_type, void *_ref)
#endif /* H5_WANT_H5_V1_4_COMPAT */
{
    H5G_entry_t *loc = NULL;    /* Symbol table entry */
    H5F_t *file=NULL;       /* File object */
#ifdef H5_WANT_H5_V1_4_COMPAT
    int ret_value;
#else /* H5_WANT_H5_V1_4_COMPAT */
    H5G_obj_t ret_value;
#endif /* H5_WANT_H5_V1_4_COMPAT */

    FUNC_ENTER_API(H5Rget_obj_type, H5G_UNKNOWN);
    H5TRACE3("Is","iRtx",id,ref_type,_ref);

    /* Check args */
    if (NULL == (loc = H5G_loc(id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5G_UNKNOWN, "not a location");
    if(ref_type<=H5R_BADTYPE || ref_type>=H5R_MAXTYPE)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, H5G_UNKNOWN, "invalid reference type");
    if(_ref==NULL)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, H5G_UNKNOWN, "invalid reference pointer");

    /* Get the file pointer from the entry */
    file=loc->file;

    /* Get the object information */
    if ((ret_value=H5R_get_obj_type(file,H5AC_ind_dxpl_id,ref_type,_ref))<0)
	HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, H5G_UNKNOWN, "unable to determine object type");
    
done:
    FUNC_LEAVE_API(ret_value);
}   /* end H5Rget_obj_type() */

